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
	F* curr;

	std::ptrdiff_t x, y, z;

	auto maxfn = [&](std::ptrdiff_t ix, std::ptrdiff_t iy, std::ptrdiff_t iz, std::ptrdiff_t channel) {
		if ( 
			   seg[x][y][z] 
			&& seg[ix][iy][iz] 
			&& seg[x][y][z] != seg[ix][iy][iz]
		) {
			id_pair p = std::minmax(seg[x][y][z], seg[ix][iy][iz]);
			if (p != ids) {
				curr = &edges[p.first][p.second];
				ids = p;	
			}
			if (!(*curr)) {
				pairs.push_back(p);
			}
			*curr = std::max(*curr, aff[ix][iy][iz][channel]);
		}
	};

	const std::ptrdiff_t block_size = 128;

	for (std::ptrdiff_t gz = 0; gz <= sz / block_size; gz++) {
		for (std::ptrdiff_t gy = 0; gy <= sy / block_size; gy++) {
			for (std::ptrdiff_t gx = 0; gx <= sx / block_size; gx++) {
				std::ptrdiff_t z0 = (gz * block_size);
				std::ptrdiff_t y0 = (gy * block_size);
				std::ptrdiff_t x0 = (gx * block_size);

				z = std::max(z0, static_cast<std::ptrdiff_t>(1));
				y = std::max(y0, static_cast<std::ptrdiff_t>(1));
				x = std::max(x0, static_cast<std::ptrdiff_t>(1));

				for (int k = 0; k < std::min(static_cast<std::ptrdiff_t>(block_size), sz - z0 - 1); k++, z++) {
					y = std::max(y0, static_cast<std::ptrdiff_t>(1));
					for (int j = 0; j < std::min(static_cast<std::ptrdiff_t>(block_size), sy - y0 - 1); j++, y++) {
						x = std::max(x0, static_cast<std::ptrdiff_t>(1));
						for (int i = 0; i < std::min(static_cast<std::ptrdiff_t>(block_size), sx - x0 - 1); i++, x++) {

							if (x > boundary_flags[0]) {
								maxfn(x-1,y,z,0);
							}
							if (y > boundary_flags[1]) {
								maxfn(x,y-1,z,1);
							}
							if (z > boundary_flags[2]) {
								maxfn(x,y,z-1,2);
							}
						}	
					}
				}
			}
		}
	}

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
