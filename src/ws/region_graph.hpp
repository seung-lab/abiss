#pragma once

#include "types.hpp"

#include <cstddef>
#include <iostream>
#include <limits>
#include <cmath>
#include <ctime>

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

	// save edges as a uint64 like min(e1,e2)|max(e1,e2)
	// this shift and mask are needed to encoding and decoding
	const uint64_t shift = 8 * static_cast<uint64_t>(std::ceil(std::log2(max_segid)));
	const uint64_t mask = ~(std::numeric_limits<uint64_t>::max() >> shift << shift);    

  MapContainer<ID, F> edges;
  edges.reserve(voxels);

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

					F& curr = edges[edge];
          curr = std::max(curr, aff[(loc-1) + sxy * sz * 0]);
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

					F& curr = edges[edge];
          curr = std::max(curr, aff[(loc-sx) + sxy * sz * 1]);
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

					F& curr = edges[edge];
          curr = std::max(curr, aff[(loc-sxy) + sxy * sz * 2]);
				}
			}
		}
	}
	clock_t end = clock();
	std::cout << "Image scan (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	begin = clock();

	region_graph<ID,F> rg;
	rg.reserve(edges.size());
  for (const uint64_t& edge : edges) {
    uint64_t v = edges[e];
		ID e1 = edge & mask;
		ID e2 = edge >> shift;
    rg.emplace_back(v, e1, e2);
  }

	end = clock();
	std::cout << "Build region graph (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	begin = clock();
	std::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });
	end = clock();
	std::cout << "Sort region graph (sec): " << double(end - begin) / CLOCKS_PER_SEC << std::endl;

	std::cout << "Sorted" << std::endl;
	return rg;
}
