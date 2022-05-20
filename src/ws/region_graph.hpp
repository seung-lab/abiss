#pragma once

#include "types.hpp"

#include <cstddef>
#include <iostream>

template< typename ID, typename F, typename L>
inline region_graph<ID,F>
get_region_graph( const affinity_graph_ptr<F>& aff_ptr,
                  const volume_ptr<ID> seg_ptr,
                  std::size_t max_segid, const L& lowv, const std::array<bool,6> & boundary_flags)
{
    using affinity_t = F;
    using id_pair = std::pair<ID,ID>;
    affinity_t low  = static_cast<affinity_t>(lowv);

    const std::ptrdiff_t sx = aff_ptr->shape()[0];
    const std::ptrdiff_t sy = aff_ptr->shape()[1];
    const std::ptrdiff_t sz = aff_ptr->shape()[2];
    const std::ptrdiff_t sxy = sx * sy;
    const std::ptrdiff_t sxyz = sxy * sz;

		ID* seg = seg_ptr->data();
		affinity_t* aff = aff_ptr->data();

    region_graph<ID,F> rg;

    std::vector<id_pair> pairs;

    std::vector<MapContainer<ID, F> > edges(max_segid);
    for (auto & h : edges) {
        h.reserve(10);
    }

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
            auto p = std::minmax(seg[loc], seg[loc-1]);
            F& curr = edges[p.first][p.second];
            if (!curr) {
                pairs.push_back(p);
            }
            curr = std::max(curr, affaff[(loc-1) + sxyz * 0]);
        }
				if ( 
					y > boundary_flags[1]
					&& seg[loc] 
					&& seg[loc-sx] 
					&& seg[loc] != seg[loc-sx]
				) {
            auto p = std::minmax(seg[loc], seg[loc-sx]);
            F& curr = edges[p.first][p.second];
            if (!curr) {
                pairs.push_back(p);
            }
            curr = std::max(curr, affaff[(loc-sx) + sxyz * 1]);
        }
				if ( 
					z > boundary_flags[2]
					&& seg[loc] 
					&& seg[loc-sxy] 
					&& seg[loc] != seg[loc-sxy]
				) {
            auto p = std::minmax(seg[loc], seg[loc-sxy]);
            F& curr = edges[p.first][p.second];
            if (!curr) {
                pairs.push_back(p);
            }
            curr = std::max(curr, affaff[(loc-sxy) + sxyz * 2]);
        }
			}
		}
	}

    for (const auto& p : pairs) {
      auto v = edges[p.first][p.second];
      rg.emplace_back(v, p.first, p.second);
    }

    std::cout << "Region graph size: " << rg.size() << std::endl;

    std::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });

    std::cout << "Sorted" << std::endl;
    return rg;
}
