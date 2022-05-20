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
	const std::size_t N = vec.size();
  std::vector<T> sorted_vec(N);
  for (std::size_t i = 0; i < N; i++) {
  	sorted_vec[p[i]] = vec[i];
  }
  return sorted_vec;
}

template <typename F>
struct Edge {
	uint64_t edge;
	F value;
	Edge(uint64_t e, F v) : edge(e), value(v); 
};

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

	const std::ptrdiff_t sx = aff_ptr->shape()[0];
	const std::ptrdiff_t sy = aff_ptr->shape()[1];
	const std::ptrdiff_t sz = aff_ptr->shape()[2];

	const std::ptrdiff_t sxy = sx * sy;
	const std::ptrdiff_t voxels = sx * sy * sz;

	ID* seg = seg_ptr->data();
	affinity_t* aff = aff_ptr->data();

	std::vector<Edge<affinity_t>> edges;
	edges.reserve(voxels);

	// save edges as a uint64 like min(e1,e2)|max(e1,e2)
	// this shift and mask are needed to encoding and decoding
	const uint64_t shift = 8 * static_cast<uint64_t>(std::ceil(std::log2(max_segid)));
	const uint64_t mask = ~(std::numeric_limits<uint64_t>::max() >> shift << shift);

	clock_t begin = clock();

	for (std::ptrdiff_t z = 1; z < sz - 1; z++) {
		for (std::ptrdiff_t y = 1; y < sy - 1; y++) {
			for (std::ptrdiff_t x = 1; x < sx - 1; x++) {
				std::ptrdiff_t loc = x + sx * y + sxy * z;

				if ( 
					x > boundary_flags[0]
					&& seg[loc]
					&& seg[loc-1]
					&& seg[loc] != seg[loc-1]
				) {
					uint64_t edge = (seg[loc] > seg[loc-1])
						? (seg[loc] | (seg[loc-1] << shift))
						: (seg[loc-1] | (seg[loc] << shift));

					edges.emplace_back(edge, aff[(loc-1) + sxy * sz * 0]);
				}
				if ( 
					y > boundary_flags[1]
					&& seg[loc] 
					&& seg[loc-sx] 
					&& seg[loc] != seg[loc-sx]
				) {
					uint64_t edge = (seg[loc] > seg[loc-sx])
						? (seg[loc] | (seg[loc-sx] << shift))
						: (seg[loc-sx] | (seg[loc] << shift));

					edges.emplace_back(edge, aff[(loc-sx) + sxy * sz * 1]);
				}
				if ( 
					z > boundary_flags[2]
					&& seg[loc] 
					&& seg[loc-sxy] 
					&& seg[loc] != seg[loc-sxy]
				) {
					uint64_t edge = (seg[loc] > seg[loc-sxy])
						? (seg[loc] | (seg[loc-sxy] << shift))
						: (seg[loc-sxy] | (seg[loc] << shift));

					edges.emplace_back(edge, aff[(loc-sxy) + sxy * sz * 2]);
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
	std::sort(std::begin(edges), std::end(edges), [](auto & a, auto & b) { return a.edge > b.edge });
  end = clock();
  std::cout << "Sort vectors (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

  begin = clock();
  Edge edge = edges[0];
  affinity_t max_affinity = edge_values[0];

  for (std::ptrdiff_t i = 0; i < edges.size(); i++) {
  	if (edges[i].edge == edge) {
  		max_affinity = std::max(max_affinity, edges[i].value);
  	}
  	else {
  		ID e1 = edge & mask;
  		ID e2 = edge >> shift;
  		rg.emplace_back(max_affinity, e1, e2);
  		edge = &edges[i];
  	}
  }

  rg.emplace_back(max_affinity, (edge.edge & mask), (edge.value >> shift));

	end = clock();
  std::cout << "Create region graph (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	std::cout << "Region graph size: " << rg.size() << std::endl;

	// begin = clock();
	// std::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });
	// end = clock();
	// std::cout << "Sort region graph (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	std::cout << "Sorted" << std::endl;
	return rg;
}
