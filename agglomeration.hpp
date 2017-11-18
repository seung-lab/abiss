#pragma once

#include "types.hpp"

#include <boost/pending/disjoint_sets.hpp>
#include <map>
#include <vector>
#include <set>
#include <iostream>

template< typename ID, typename F, typename L, typename M >
inline void merge_segments( const volume_ptr<ID>& seg_ptr,
                            const region_graph_ptr<ID,F> rg_ptr,
                            std::vector<std::size_t>& counts,
                            const L& tholds,
                            const M& lowt,
                            const ID& offset)
{
    using traits = watershed_traits<id_t>;
    std::vector<ID> rank(counts.size());
    std::vector<ID> parent(counts.size());
    boost::disjoint_sets<ID*, ID*> sets(&rank[0], &parent[0]);
    for (ID i = 0; i < counts.size(); i++) {
        sets.make_set(i);
    }

    typename region_graph<ID,F>::iterator rit = rg_ptr->begin();

    region_graph<ID,F>& rg  = *rg_ptr;

    std::size_t size = static_cast<std::size_t>(tholds.first);
    //F           thld = static_cast<F>(it.second);

    while ( (rit != rg.end()) && ( std::get<0>(*rit) > tholds.second) )
    {
        ID s1 = sets.find_set(std::get<1>(*rit));
        ID s2 = sets.find_set(std::get<2>(*rit));

        if ( s1 != s2 && s1 && s2 )
        {
            if ( (counts[s1] < size) || (counts[s2] < size) )
            {
                if ((traits::on_border&(counts[s1]|counts[s2]))==0) {
                    counts[s1] += counts[s2];
                    counts[s2]  = 0;
                    sets.link(s1, s2);
                    ID s = sets.find_set(s1);
                    std::swap(counts[s], counts[s1]);
                }
                else {
                    counts[s1] |= counts[s2]&traits::on_border;
                    counts[s2] |= counts[s1]&traits::on_border;
                }
            }
        }
        ++rit;
    }

    std::cout << "Done with merging" << std::endl;

    std::vector<ID> remaps(counts.size());

    counts[0] &= ~traits::on_border;
    remaps[0] = 0;

    ID next_id = 1;

    std::size_t low = static_cast<std::size_t>(lowt);

    for ( ID id = 0; id < counts.size(); ++id )
    {
        ID s = sets.find_set(id);
        if ( counts[id]&(~traits::on_border) ) {
            if ( s && (counts[s] >= low) )
            {
                if (remaps[s] == 0) {
                    remaps[s] = next_id;
                    counts[next_id] = counts[s]&(~traits::on_border);
                    ++next_id;
                }
            } else {
                counts[s] = remaps[s] = 0;
            }
        }
    }

    counts.resize(next_id);

    std::ptrdiff_t xdim = seg_ptr->shape()[0];
    std::ptrdiff_t ydim = seg_ptr->shape()[1];
    std::ptrdiff_t zdim = seg_ptr->shape()[2];

    std::ptrdiff_t total = xdim * ydim * zdim;

    ID* seg_raw = seg_ptr->data();

    for ( std::ptrdiff_t idx = 0; idx < total; ++idx )
    {
        ID s = remaps[sets.find_set(seg_raw[idx])];
        if (s != 0) {
            s += offset;
        }
        seg_raw[idx] = s;
    }

    std::cout << "Done with remapping, total: " << (next_id-1) << std::endl;

    region_graph<ID,F> new_rg;

    std::vector<std::set<ID>> in_rg(next_id);

    std::vector<ID> rank_mst(next_id);
    std::vector<ID> parent_mst(next_id);
    boost::disjoint_sets<ID*, ID*> mst(&rank_mst[0], &parent_mst[0]);
    for (ID i = 0; i < next_id; i++) {
        mst.make_set(i);
    }

    for ( auto& it: rg )
    {
        ID s1 = remaps[sets.find_set(std::get<1>(it))];
        ID s2 = remaps[sets.find_set(std::get<2>(it))];
        ID a1 = mst.find_set(s1);
        ID a2 = mst.find_set(s2);

        if ( a1 != a2 && a1 && a2 )
        {
            mst.link(a1, a2);
            auto mm = std::minmax(s1,s2);
            if ( in_rg[mm.first].count(mm.second) == 0 )
            {
                new_rg.emplace_back(std::get<0>(it), mm.first+offset, mm.second+offset);
                in_rg[mm.first].insert(mm.second);
            }
        }
    }

    rg.swap(new_rg);

    std::cout << "Done with updating the region graph, size: "
              << rg.size() << std::endl;
}
