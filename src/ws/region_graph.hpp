#pragma once

#include "types.hpp"

#include <cstddef>
#include <iostream>
#include <limits>
#include <cmath>
#include <ctime>

template <typename T>
std::vector<T> apply_permutation(
  const std::vector<T>& vec,
  const std::vector<std::size_t>& p
) {
  std::vector<T> sorted_vec(vec.size());
  std::transform(
  	p.begin(), p.end(), sorted_vec.begin(),
    [&](std::size_t i){ return vec[i]; }
  );
  return sorted_vec;
}

template< typename ID, typename F, typename L>
inline region_graph<ID,F>
get_region_graph(
	const affinity_graph_ptr<F>& aff_ptr,
	const volume_ptr<ID> seg_ptr,
	std::size_t max_segid, 
	const L& lowv, 
	const std::array<bool,6> & boundary_flags
) {
	using affinity_t = F;
	using id_pair = std::pair<ID,ID>;
	affinity_t low  = static_cast<affinity_t>(lowv);

	const std::ptrdiff_t sx = aff_ptr->shape()[0];
	const std::ptrdiff_t sy = aff_ptr->shape()[1];
	const std::ptrdiff_t sz = aff_ptr->shape()[2];

	const std::ptrdiff_t voxels = sx * sy * sz;

	volume<ID>& seg = *seg_ptr;
	affinity_graph<F>& aff = *aff_ptr;
	std::vector<id_pair> pairs;

	std::vector<uint64_t> edges;
	std::vector<F> edge_values;
	edges.reserve(voxels);
	edge_values.reserve(voxels);

	// save edges as a uint64 like min(e1,e2)|max(e1,e2)
	// this shift and mask are needed to encoding and decoding
	const uint64_t shift = 8 * static_cast<uint64_t>(std::ceil(std::log2(max_segid)));
	const uint64_t mask = ~(std::numeric_limits<uint64_t>::max() >> shift << shift);

	// auto maxfn = [&](std::ptrdiff_t ix, std::ptrdiff_t iy, std::ptrdiff_t iz, std::ptrdiff_t channel) {
	// 	if ( 
	// 		x > boundary_flags[0]
	// 		&& seg[x][y][z] 
	// 		&& seg[ix][iy][iz] 
	// 		&& seg[x][y][z] != seg[ix][iy][iz]
	// 	) {
	// 		uint64_t edge = (seg[x][y][z] > seg[ix][iy][iz])
	// 			? (seg[x][y][z] | (seg[ix][iy][iz] << shift))
	// 			: (seg[ix][iy][iz] | (seg[x][y][z] << shift));

	// 		edges.push_back(edge);
	// 		edge_values.push_back(aff[ix][iy][iz][channel]);
	// 	}
	// };

	clock_t begin = clock();

	for (std::ptrdiff_t z = 1; z < sz - 1; z++) {
		for (std::ptrdiff_t y = 1; y < sy - 1; y++) {
			for (std::ptrdiff_t x = 1; x < sx - 1; x++) {
				if ( 
					x > boundary_flags[0]
					&& seg[x][y][z] 
					&& seg[x-1][y][z] 
					&& seg[x][y][z] != seg[x-1][y][z]
				) {
					uint64_t edge = (seg[x][y][z] > seg[x-1][y][z])
						? (seg[x][y][z] | (seg[x-1][y][z] << shift))
						: (seg[x-1][y][z] | (seg[x][y][z] << shift));

					edges.push_back(edge);
					edge_values.push_back(aff[x-1][y][z][0]);
				}
				if ( 
					y > boundary_flags[1]
					&& seg[x][y][z] 
					&& seg[x][y-1][z] 
					&& seg[x][y][z] != seg[x][y-1][z]
				) {
					uint64_t edge = (seg[x][y][z] > seg[x][y-1][z])
						? (seg[x][y][z] | (seg[x][y-1][z] << shift))
						: (seg[x][y-1][z] | (seg[x][y][z] << shift));

					edges.push_back(edge);
					edge_values.push_back(aff[x][y-1][z][1]);
				}
				if ( 
					z > boundary_flags[2]
					&& seg[x][y][z] 
					&& seg[x][y][z-1] 
					&& seg[x][y][z] != seg[x][y][z-1]
				) {
					uint64_t edge = (seg[x][y][z] > seg[x][y][z-1])
						? (seg[x][y][z] | (seg[x][y][z-1] << shift))
						: (seg[x][y][z-1] | (seg[x][y][z] << shift));

					edges.push_back(edge);
					edge_values.push_back(aff[x][y][z-1][2]);
				}
			}
		}
	}
	clock_t end = clock();
	std::cout << "Image scan (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	region_graph<ID,F> rg;
	if (edges.size() == 0) {
		std::cout << "Region graph size: " << rg.size() << std::endl;
		return rg;
	}

	begin = clock();
	std::vector<std::size_t> edge_permutation(edges.size()); // sort indices
  std::iota(edge_permutation.begin(), edge_permutation.end(), 0);
  std::sort(edge_permutation.begin(), edge_permutation.end(),
        [&](std::size_t i, std::size_t j){ return edges[i] < edges[j]; });
  end = clock();
	std::cout << "Create permutation (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	begin = clock();
  edges = apply_permutation(edges, edge_permutation);
  edge_values = apply_permutation(edge_values, edge_permutation);
  end = clock();
  std::cout << "Sort vectors (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;


  begin = clock();
  uint64_t edge = edges[0];
  affinity_t max_affinity = edge_values[0];

  for (std::ptrdiff_t i = 0; i < edges.size(); i++) {
  	if (edges[i] == edge) {
  		max_affinity = std::max(max_affinity, edge_values[i]);
  	}
  	else {
  		ID e1 = edge & mask;
  		ID e2 = edge >> shift;
  		rg.emplace_back(max_affinity, e1, e2);
  	}
  }

  rg.emplace_back(max_affinity, (edge & mask), (edge >> shift));

	end = clock();
  std::cout << "Create region graph (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	std::cout << "Region graph size: " << rg.size() << std::endl;

	begin = clock();
	std::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });
	end = clock();
	std::cout << "Sort region graph (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	std::cout << "Sorted" << std::endl;
	return rg;
}
