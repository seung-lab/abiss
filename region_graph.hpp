#pragma once

#include "types.hpp"

#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <boost/functional/hash.hpp>

template< typename ID, typename F, typename L>
inline region_graph_ptr<ID,F>
get_region_graph( const affinity_graph_ptr<F>& aff_ptr,
                  const volume_ptr<ID> seg_ptr,
                  std::size_t max_segid, const L& lowv)
{
    using affinity_t = F;
    using id_pair = std::pair<ID,ID>;
    affinity_t low  = static_cast<affinity_t>(lowv);

    std::ptrdiff_t xdim = aff_ptr->shape()[0];
    std::ptrdiff_t ydim = aff_ptr->shape()[1];
    std::ptrdiff_t zdim = aff_ptr->shape()[2];

    volume<ID>& seg = *seg_ptr;
    affinity_graph<F> aff = *aff_ptr;

    region_graph_ptr<ID,F> rg_ptr( new region_graph<ID,F> );

    region_graph<ID,F>& rg = *rg_ptr;

    std::vector<id_pair> pairs;

    std::vector<std::unordered_map<ID, F> > edges(max_segid);
    //std::vector<emilib::HashMap<ID, F> > edges(max_segid);
    for (auto & h : edges) {
        h.reserve(10);
    }

    for ( std::ptrdiff_t z = 0; z < zdim; ++z )
        for ( std::ptrdiff_t y = 0; y < ydim; ++y )
            for ( std::ptrdiff_t x = 0; x < xdim; ++x )
            {
                if ( (x > 0) && seg[x][y][z] && seg[x-1][y][z] && seg[x][y][z] != seg[x-1][y][z])
                {
                    auto p = std::minmax(seg[x][y][z], seg[x-1][y][z]);
                    F& curr = edges[p.first][p.second];
                    if (!curr) {
                        pairs.push_back(p);
                    }
                    curr = std::max(curr, aff[x][y][z][0]);
                }
                if ( (y > 0) && seg[x][y][z] && seg[x][y-1][z] && seg[x][y][z] != seg[x][y-1][z])
                {
                    auto p = std::minmax(seg[x][y][z], seg[x][y-1][z]);
                    F& curr = edges[p.first][p.second];
                    if (!curr) {
                        pairs.push_back(p);
                    }
                    curr = std::max(curr, aff[x][y][z][1]);
                }
                if ( (z > 0) && seg[x][y][z] && seg[x][y][z-1] && seg[x][y][z] != seg[x][y][z-1])
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

    //size_t n = edges.bucket_count();
    //size_t dup = 0;
    //size_t empty = 0;
    //for (size_t i = 0; i < n; i++) {
    //    auto s = edges.bucket_size(i);
    //    if (s > 1) {
    //        dup++;
    //    }
    //    if (s == 0) {
    //        empty++;
    //    }
    //}
    //std::cout << "empty buckets:" << empty << std::endl;
    //std::cout << "filled buckets:" << dup << std::endl;

    std::cout << "Region graph size: " << rg.size() << std::endl;

    std::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });

    std::cout << "Sorted" << std::endl;
    return rg_ptr;
}
