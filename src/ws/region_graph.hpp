#pragma once

#include "types.hpp"

#include <cstddef>
#include <iostream>

// template <ID>
// MapContainer<ID, ID> renumber(
// 	volume<ID>& seg,
// 	const ptrdiff_t sx, const ptrdiff_t sy, const ptrdiff_t sz

// ) {
// 	MapContainer<ID, ID> remap;

// 	if (sx * sy * sz == 0) {
// 		return remap;
// 	}

// 	remap.reserve(10000);

// 	ID remap_id = 1;
// 	ID last_elem = ~seg[0][0][0]; // isn't the first value and won't cause an overflow
// 	ID last_remap_id = remap_id;

// 	for (std::ptrdiff_t z = 0; z < sz; ++z) {
// 		for (std::ptrdiff_t y = 1; y < sy; ++y) {
// 			for (std::ptrdiff_t x = 1; x < sx; ++x) {
// 				ID elem = seg[x][y][z];
// 				if (elem == last_elem) {
// 					seg[x][y][z] = last_remap_id;
// 					continue;
// 				}

// 				if (remap.contains(elem)) {
// 					seg[x][y][z] = remap[elem];
// 				}
// 				else {
// 					seg[x][y][z] = remap_id;
// 					remap[elem] = remap_id;
// 					remap_id++;
// 				}

// 				last_elem = elem;
//     		last_remap_id = seg[x][y][z];
// 			}
// 		}		
// 	}

// 	return remap;
// }

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

	volume<ID>& seg = *seg_ptr;
	// MapContainer<ID,ID> remap = renumber(seg, sx, sy, sz);

	affinity_graph<F>& aff = *aff_ptr;
	std::vector<id_pair> pairs;

	std::vector<MapContainer<ID, F> > edges(max_segid);

	// std::vector<emilib::HashMap<ID, F> > edges(remap.size() + 1);
	for (auto& h : edges) {
		h.reserve(10);
	}

	id_pair ids = std::pair<ID,ID>(max_segid + 1, max_segid + 1);
	F& curr;

	for (std::ptrdiff_t z = 1; z < sz - 1; ++z) {
		for (std::ptrdiff_t y = 1; y < sy - 1; ++y) {
			for (std::ptrdiff_t x = 1; x < sx - 1; ++x) {

				if ( 
						(x > boundary_flags[0]) 
						&& seg[x][y][z] 
						&& seg[x-1][y][z] 
						&& seg[x][y][z] != seg[x-1][y][z]
				) {
					id_pair p = std::minmax(seg[x][y][z], seg[x-1][y][z]);
					if (p != ids) {
						curr = edges[p.first][p.second];
						ids = p;	
					}
					if (!curr) {
						pairs.push_back(p);
					}
					curr = std::max(curr, aff[x][y][z][0]);
				}

				if ( 
					(y > boundary_flags[1]) 
					&& seg[x][y][z] 
					&& seg[x][y-1][z] 
					&& seg[x][y][z] != seg[x][y-1][z]
				) {
					id_pair p = std::minmax(seg[x][y][z], seg[x][y-1][z]);
					if (p != ids) {
						curr = edges[p.first][p.second];
						ids = p;	
					}
					if (!curr) {
						pairs.push_back(p);
					}
					curr = std::max(curr, aff[x][y][z][1]);
				}
				if ( 
					(z > boundary_flags[2]) 
					&& seg[x][y][z] 
					&& seg[x][y][z-1] 
					&& seg[x][y][z] != seg[x][y][z-1]
				) {
					id_pair p = std::minmax(seg[x][y][z], seg[x][y][z-1]);
					if (p != ids) {
						curr = edges[p.first][p.second];
						ids = p;	
					}
					if (!curr) {
						pairs.push_back(p);
					}
					curr = std::max(curr, aff[x][y][z][2]);
				}
			}
		}
	}

	// std::vector<ID> inv_remap(remap.size() + 1);
	// for (auto& p : remap) {
	// 	inv_remap[p.second] = p.first;
	// }

	region_graph<ID,F> rg;

	for (const auto& p : pairs) {
		auto v = edges[p.first][p.second];
		rg.emplace_back(v, p.first, p.second);
	}

	std::cout << "Region graph size: " << rg.size() << std::endl;

	std::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });

	std::cout << "Sorted" << std::endl;
	return rg;
}
