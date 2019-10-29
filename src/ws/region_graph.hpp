#pragma once

#include "types.hpp"

#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <boost/functional/hash.hpp>

template< typename ID, typename F, typename L>
inline region_graph<ID,F>
get_region_graph( const affinity_graph_ptr<F>& aff_ptr,
                  const volume_ptr<ID> seg_ptr,
                  std::size_t max_segid, const L& lowv, const std::array<bool,6> & boundary_flags)
{
    using affinity_t = F;
    using id_pair = std::pair<ID,ID>;
    affinity_t low  = static_cast<affinity_t>(lowv);

    std::ptrdiff_t xdim = aff_ptr->shape()[0];
    std::ptrdiff_t ydim = aff_ptr->shape()[1];
    std::ptrdiff_t zdim = aff_ptr->shape()[2];

    volume<ID>& seg = *seg_ptr;
    affinity_graph<F> aff = *aff_ptr;

    region_graph<ID,F> rg;

    std::vector<id_pair> pairs;

    std::vector<std::unordered_map<ID, F> > edges(max_segid);
    //std::vector<emilib::HashMap<ID, F> > edges(max_segid);
    for (auto & h : edges) {
        h.reserve(10);
    }

    for ( std::ptrdiff_t z = 1; z < zdim - 1; ++z )
        for ( std::ptrdiff_t y = 1; y < ydim - 1; ++y )
            for ( std::ptrdiff_t x = 1; x < xdim - 1; ++x )
            {
                if ( (x > boundary_flags[0]) && seg[x][y][z] && seg[x-1][y][z] && seg[x][y][z] != seg[x-1][y][z])
                {
                    auto p = std::minmax(seg[x][y][z], seg[x-1][y][z]);
                    F& curr = edges[p.first][p.second];
                    if (!curr) {
                        pairs.push_back(p);
                    }
                    curr = std::max(curr, aff[x][y][z][0]);
                }
                if ( (y > boundary_flags[1]) && seg[x][y][z] && seg[x][y-1][z] && seg[x][y][z] != seg[x][y-1][z])
                {
                    auto p = std::minmax(seg[x][y][z], seg[x][y-1][z]);
                    F& curr = edges[p.first][p.second];
                    if (!curr) {
                        pairs.push_back(p);
                    }
                    curr = std::max(curr, aff[x][y][z][1]);
                }
                if ( (z > boundary_flags[2]) && seg[x][y][z] && seg[x][y][z-1] && seg[x][y][z] != seg[x][y][z-1])
                {
                    auto p = std::minmax(seg[x][y][z], seg[x][y][z-1]);
                    F& curr = edges[p.first][p.second];
                    if (!curr) {
                        pairs.push_back(p);
                    }
                    curr = std::max(curr, aff[x][y][z][2]);
                }
            }

    for ( const auto& p : pairs)
    {
        auto v = edges[p.first][p.second];
        rg.emplace_back(v, p.first, p.second);
    }

    std::cout << "Region graph size: " << rg.size() << std::endl;

    std::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });

    std::cout << "Sorted" << std::endl;
    return rg;
}
