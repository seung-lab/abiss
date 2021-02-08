#include "Types.h"
#include "Utils.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <execution>
#include <sys/stat.h>
#include <boost/format.hpp>
#include <boost/pending/disjoint_sets.hpp>
#include "SemExtractor.hpp"

template <class T>
struct __attribute__((packed)) matching_entry_t
{
    T oid;
    size_t boundary_size;
    T nid;
    size_t agg_size;
};

template <class T>
struct __attribute__((packed)) matching_value
{
    T sid;
    size_t size;
    explicit constexpr matching_value(T s = 0, size_t sz = 0)
        :sid(s), size(sz){}
};

template <class T>
struct remap_data
{
    std::vector<T> segids;
    std::vector<matching_value<T> > remaps;
    std::vector<matching_value<T> > extra_remaps;
};

template <class T_seg, class T_aff>
struct __attribute__((packed)) rg_entry
{
    T_seg s1;
    T_seg s2;
    T_aff aff;
    size_t area;
};

template <class T>
remap_data<T> generate_remaps()
{
    auto remap_vector = read_array<matching_entry_t<T> >("matching_faces.data");
    //sort to make sure we process the most advanced agglomerated (largest) segments first
    std::sort(std::execution::par, remap_vector.begin(), remap_vector.end(), [](auto & a, auto & b) {
            return (a.agg_size > b.agg_size) || ((a.agg_size == b.agg_size) && (a.nid < b.nid));
    });
    auto last_remap = std::unique(remap_vector.begin(), remap_vector.end(), [](const auto & a, const auto & b) {
        return ((a.oid == b.oid) /* && (a.boundary_size == b.boundary_size) */ && (a.nid == b.nid) && (a.agg_size == b.agg_size));
    });
    remap_vector.erase(last_remap, remap_vector.end());

    remap_data<T> rep;
    rep.segids.push_back(0); //index 0 is used to mark unmapped segments
    for (auto & e: remap_vector) {
        rep.segids.push_back(e.oid);
        rep.segids.push_back(e.nid);
    }
    std::sort(std::execution::par, rep.segids.begin(), rep.segids.end());
    auto last_segid = std::unique(rep.segids.begin(), rep.segids.end());
    rep.segids.erase(last_segid, rep.segids.end());

    std::for_each(std::execution::par, remap_vector.begin(), remap_vector.end(), [&rep](auto & e) {
        auto it = std::lower_bound(rep.segids.begin(), rep.segids.end(), e.oid);
        if (it == rep.segids.end()) {
            std::cerr << "Should not happen, element does not exist: " << e.oid << std::endl;
            std::abort();
        }
        if (e.oid == *it) {
            e.oid = std::distance(rep.segids.begin(), it);
        } else {
            std::abort();
        }

        it = std::lower_bound(rep.segids.begin(), rep.segids.end(), e.nid);
        if (it == rep.segids.end()) {
            std::cerr << "Should not happen, element does not exist: " << e.nid << std::endl;
            std::abort();
        }
        if (e.nid == *it) {
            e.nid = std::distance(rep.segids.begin(), it);
        } else {
            std::abort();
        }
    });

    std::vector<matching_value<T> > remaps(rep.segids.size());
    std::vector<matching_value<T> > extra_remaps(rep.segids.size());

    std::vector<T> rank(rep.segids.size());
    std::vector<T> parent(rep.segids.size());
    boost::disjoint_sets<T*, T*> sets(&rank[0], &parent[0]);

    for (size_t i = 0; i != rep.segids.size(); i++) {
        sets.make_set(i);
    }

    for (auto & e : remap_vector) {
        if (remaps[e.oid].sid == 0) {
            remaps[e.oid] = matching_value(e.nid, e.agg_size);
        } else {
            // Placeholder for later
            extra_remaps[e.nid] = remaps[e.oid];
        }
        if (remaps[e.nid].sid == 0) {
            remaps[e.nid] = matching_value(e.nid, e.agg_size);
        }
        sets.union_set(e.oid, e.nid);
    }

    {
        std::vector<T> idx(rep.segids.size());
        std::iota(idx.begin(), idx.end(), T(0));
        sets.compress_sets(idx.begin(), idx.end());
    }

    std::vector<T> normalized_parent(rep.segids.size());

    // Pick the largest segment to represent, it is required because the way we populated extra_remaps above
    for (auto & e: remap_vector) {
        auto nid = parent[e.nid];
        if (normalized_parent[nid] == 0) {
            normalized_parent[nid] = e.nid;
        }
    }

    for (size_t i = 0; i != rep.segids.size(); i++) {
        auto p = parent[i];
        if (normalized_parent[p] != 0) {
            parent[i] = normalized_parent[p];
        }
    }

    for (size_t i = 0; i < rep.segids.size(); i++) {
        if (remaps[i].sid != parent[i]) {
            if (i == remaps[i].sid) {
                extra_remaps[remaps[i].sid] = remaps[parent[i]];
            }
            remaps[i] = remaps[parent[i]];
        }
        if (extra_remaps[i].sid != 0) {
            extra_remaps[i] = remaps[parent[extra_remaps[i].sid]];
        }
    }

    rep.remaps = remaps;
    rep.extra_remaps = extra_remaps;
    return rep;
}

template<class T_seg, class T_aff>
void process_edges(std::string & tag, remap_data<T_seg> & rep)
{
    auto rg_vector = read_array<rg_entry<T_seg, T_aff> >("o_residual_rg.data");
    auto extra_edges = read_array<rg_entry<T_seg, T_aff> >(str(boost::format("o_incomplete_edges_%1%.tmp") % tag));
    rg_vector.insert(rg_vector.end(), std::make_move_iterator(extra_edges.begin()), std::make_move_iterator(extra_edges.end()));
    std::vector<T_seg> segids;
    for (const auto & e : rg_vector) {
        segids.push_back(e.s1);
        segids.push_back(e.s2);
    }
    std::sort(std::execution::par, segids.begin(),segids.end());
    auto last = std::unique(segids.begin(), segids.end());
    segids.erase(last, segids.end());
    std::for_each(std::execution::par, segids.begin(), segids.end(), [&rep](const auto & id){
        auto it = std::lower_bound(rep.segids.begin(), rep.segids.end(), id);
        if (it != rep.segids.end() && id == *it) {
            auto idx = std::distance(rep.segids.begin(), it);
            rep.extra_remaps[idx] = rep.remaps[idx];
        }
    });
    std::for_each(std::execution::par, rg_vector.begin(), rg_vector.end(), [&rep](auto & e){
        auto it = std::lower_bound(rep.segids.begin(), rep.segids.end(), e.s1);
        if (it != rep.segids.end() && e.s1 == *it) {
            auto idx = std::distance(rep.segids.begin(), it);
            //if (e.s1 == 72057594172159552) {
            //    std::cout << "remap " << e.s1 << " to " << rep.segids[rep.remaps[idx].sid] << std::endl;
            //}
            if (rep.remaps[idx].sid != idx) {
                e.s1 = rep.segids[rep.remaps[idx].sid];
            }
        }
        it = std::lower_bound(rep.segids.begin(), rep.segids.end(), e.s2);
        if (it != rep.segids.end() && e.s2 == *it) {
            auto idx = std::distance(rep.segids.begin(), it);
            //if (e.s2 == 72057594172159552) {
            //    std::cout << "remap " << e.s2 << " to " << rep.segids[rep.remaps[idx].sid] << std::endl;
            //}
            if (rep.remaps[idx].sid != idx) {
                e.s2 = rep.segids[rep.remaps[idx].sid];
            }
        }
        if (e.s1 > e.s2) {
            auto tmp = e.s1;
            e.s1 = e.s2;
            e.s2 = tmp;
        }
    });

    auto vetoed_edges = read_array<std::pair<T_seg, T_seg> >("vetoed_edges.data");

    std::for_each(std::execution::par, vetoed_edges.begin(), vetoed_edges.end(), [&rep](auto & e){
        auto it = std::lower_bound(rep.segids.begin(), rep.segids.end(), e.first);
        if (it != rep.segids.end() && e.first == *it) {
            auto idx = std::distance(rep.segids.begin(), it);
            if (rep.remaps[idx].sid != idx) {
                e.first = rep.segids[rep.remaps[idx].sid];
            }
        }
        it = std::lower_bound(rep.segids.begin(), rep.segids.end(), e.second);
        if (it != rep.segids.end() && e.second == *it) {
            auto idx = std::distance(rep.segids.begin(), it);
            if (rep.remaps[idx].sid != idx) {
                e.second = rep.segids[rep.remaps[idx].sid];
            }
        }
        if (e.first > e.second) {
            std::swap(e.first, e.second);
        }
    });

    std::sort(std::execution::par, vetoed_edges.begin(), vetoed_edges.end(), [](auto & a, auto & b) { return (a.first < b.first) || (a.first == b.first && a.second < b.second); });
    auto last_veto_edge = std::unique(vetoed_edges.begin(), vetoed_edges.end(), [](auto & a, auto & b) { return (a.first == b.first) && (a.second == b.second); });
    vetoed_edges.erase(last_veto_edge, vetoed_edges.end());

    std::for_each(std::execution::par, rg_vector.begin(), rg_vector.end(), [&vetoed_edges](auto & e){
        T_seg u1 = e.s1;
        T_seg u2 = e.s2;
        auto it = std::lower_bound(vetoed_edges.begin(), vetoed_edges.end(), std::make_pair(u1, u2),  [](const auto & a, const auto & b) {
            return (a.first < b.first) || (a.first == b.first && a.second < b.second);
        });
        if (it != vetoed_edges.end() && u1 == (*it).first && u2 == (*it).second) {
            e.s1 = 0;
            e.s2 = 0;
        }
    });

    std::ofstream ofs(str(boost::format("incomplete_edges_%1%.tmp") % tag), std::ios_base::binary);
    for(const auto & e: rg_vector) {
        if (e.s1 != e.s2) {
            ofs.write(reinterpret_cast<const char *>(&(e)), sizeof(e));
        }
    }
    assert(!ofs.bad());
    ofs.close();

    ofs = std::ofstream(str(boost::format("vetoed_edges_%1%.data") % tag), std::ios_base::binary);
    for(const auto & e: vetoed_edges) {
        ofs.write(reinterpret_cast<const char *>(&(e)), sizeof(e));
    }
    assert(!ofs.bad());
    ofs.close();
}

template <class T>
std::vector<T> process_boundary_supervoxels(const std::string & tag, const remap_data<T> & rep)
{
    std::vector<T> boundary_svs;
    for (size_t i = 0; i < 6; i++) {
        auto remap_vector = read_array<matching_entry_t<T> >(str(boost::format("o_boundary_%1%_%2%.tmp")% i % tag));
        std::for_each(std::execution::par, remap_vector.begin(), remap_vector.end(), [&rep](auto & bs) {
            auto it = std::lower_bound(rep.segids.begin(), rep.segids.end(), bs.nid);
            if (it != rep.segids.end() && bs.nid == *it) {
                auto idx = std::distance(rep.segids.begin(), it);
                auto new_rep = rep.remaps[idx];
                if (new_rep.sid != 0) {
                    bs.nid = rep.segids[new_rep.sid];
                    bs.agg_size = new_rep.size;
                }
            }
        });

        std::sort(std::execution::par, remap_vector.begin(), remap_vector.end(), [](auto & a, auto & b) {
                return a.agg_size > b.agg_size;
        });

        auto last_remap = std::unique(remap_vector.begin(), remap_vector.end(), [](const auto & a, const auto & b) {
            return ((a.oid == b.oid) && (a.boundary_size == b.boundary_size) && (a.nid == b.nid) && (a.agg_size == b.agg_size));
        });

        remap_vector.erase(last_remap, remap_vector.end());

        std::ofstream ofs1(str(boost::format("o_boundary_%1%_%2%.data")% i % tag), std::ios_base::binary);
        std::ofstream ofs2(str(boost::format("boundary_%1%_%2%.tmp")% i % tag), std::ios_base::binary);
        for(const auto & e: remap_vector) {
            ofs1.write(reinterpret_cast<const char *>(&(e)), sizeof(e));
            if (e.boundary_size == 1) {
                ofs2.write(reinterpret_cast<const char *>(&(e.nid)), sizeof(T));
                boundary_svs.push_back(e.oid);
            }
        }
        assert(!ofs1.bad());
        assert(!ofs2.bad());
        ofs1.close();
        ofs2.close();
    }
    std::sort(std::execution::par, boundary_svs.begin(), boundary_svs.end());
    auto last = std::unique(boundary_svs.begin(), boundary_svs.end());
    boundary_svs.erase(last, boundary_svs.end());
    return boundary_svs;
}

template <class T>
void generate_extra_counts(const remap_data<T> & rep, const std::vector<T> & bs)
{
    auto remap_vector = read_array<matching_entry_t<T> >("matching_faces.data");
    std::sort(std::execution::par, remap_vector.begin(), remap_vector.end(), [](auto & a, auto & b) {
            return a.agg_size > b.agg_size;
    });
    auto last_remap = std::unique(remap_vector.begin(), remap_vector.end(), [](const auto & a, const auto & b) {
        return ((a.oid == b.oid) && (a.boundary_size == b.boundary_size) && (a.nid == b.nid) && (a.agg_size == b.agg_size));
    });
    remap_vector.erase(last_remap, remap_vector.end());
    std::vector<std::pair<T, T> > processed_sv(remap_vector.size());
    std::transform(std::execution::par, remap_vector.begin(), remap_vector.end(), processed_sv.begin(), [&rep, &bs](const auto & e) {
        if (e.boundary_size == 0) {
            return std::make_pair<T,T>(0,0);
        }
        auto it_bs = std::lower_bound(bs.begin(), bs.end(), e.oid);
        auto it_rep = std::lower_bound(rep.segids.begin(), rep.segids.end(), e.oid);
        if (it_rep == rep.segids.end() || e.oid != *it_rep) {
            std::abort();
        }
        if (it_bs == bs.end() || e.oid != *it_bs) {
            auto idx = std::distance(rep.segids.begin(), it_rep);
            T sid = rep.remaps[idx].sid == 0 ? e.nid : rep.segids[rep.remaps[idx].sid];
            return std::make_pair(e.oid, sid);
        } else {
            return std::make_pair<T, T>(0,0);
        }
    });
    std::sort(std::execution::par, processed_sv.begin(), processed_sv.end(), [](const auto & a, const auto & b) {
        return (a.first < b.first);
    });
    auto last = std::unique(processed_sv.begin(), processed_sv.end(), [](const auto & a, const auto & b) {
        return (a.first == b.first && a.second == b.second);
    });
    processed_sv.erase(last, processed_sv.end());
    std::ofstream ofs("extra_sv_counts.data", std::ios_base::binary);
    const size_t sv_size = 1;
    for (size_t i = 1; i < processed_sv.size(); i++) {
        ofs.write(reinterpret_cast<const char *>(&(processed_sv[i].second)), sizeof(T));
        ofs.write(reinterpret_cast<const char *>(&(sv_size)), sizeof(size_t));
    }
    assert(!ofs.bad());
    ofs.close();
}

template <class T>
void process_counts(remap_data<T> rep)
{
    auto sizes = read_array<std::pair<T,size_t> >("o_ongoing_supervoxel_counts.data");
    std::for_each(std::execution::par, sizes.begin(), sizes.end(), [&rep](auto & p) {
        auto it = std::lower_bound(rep.segids.begin(), rep.segids.end(), p.first);
        if (it != rep.segids.end() && p.first == *it) {
            auto idx = std::distance(rep.segids.begin(), it);
            if (rep.remaps[idx].sid != 0) {
                p.first = rep.segids[rep.remaps[idx].sid];
            }
        }
    });
    std::ofstream ofs("ongoing_supervoxel_counts.data", std::ios_base::binary);
    for (const auto & p : sizes) {
        ofs.write(reinterpret_cast<const char *>(&(p)), sizeof(p));
    }
    assert(!ofs.bad());
    ofs.close();
}

template <class T>
void process_size(remap_data<T> rep)
{
    auto sizes = read_array<std::pair<T, size_t> >("o_ongoing_seg_size.data");
    std::for_each(std::execution::par, sizes.begin(), sizes.end(), [&rep](auto & p) {
        auto it = std::lower_bound(rep.segids.begin(), rep.segids.end(), p.first);
        if (it != rep.segids.end() && p.first == *it) {
            auto idx = std::distance(rep.segids.begin(), it);
            if (rep.remaps[idx].sid != 0) {
                p.first = rep.segids[rep.remaps[idx].sid];
            }
        }
    });
    std::ofstream ofs("ongoing_seg_size.data", std::ios_base::binary);
    for (const auto & p : sizes) {
        ofs.write(reinterpret_cast<const char *>(&(p)), sizeof(p));
    }
    assert(!ofs.bad());
    ofs.close();
}

template <class T>
void process_sems(remap_data<T> rep)
{
    auto sems = read_array<std::pair<T,sem_array_t> >("o_ongoing_semantic_labels.data");
    std::for_each(std::execution::par, sems.begin(), sems.end(), [&rep](auto & p) {
        auto it = std::lower_bound(rep.segids.begin(), rep.segids.end(), p.first);
        if (it != rep.segids.end() && p.first == *it) {
            auto idx = std::distance(rep.segids.begin(), it);
            if (rep.remaps[idx].sid != 0) {
                p.first = rep.segids[rep.remaps[idx].sid];
            }
        }
    });
    std::ofstream ofs("ongoing_semantic_labels.data", std::ios_base::binary);
    for (const auto & p : sems) {
        ofs.write(reinterpret_cast<const char *>(&(p)), sizeof(p));
    }
    assert(!ofs.bad());
    ofs.close();
}

template <class T>
void write_extra_remaps(remap_data<T> & rep)
{
    std::ofstream ofs("extra_remaps.data", std::ios_base::binary);
    for (size_t i = 0; i < rep.segids.size(); i++) {
        if (rep.extra_remaps[i].sid != i && rep.extra_remaps[i].sid != 0) {
            //std::cout << rep.segids[i] << ", " << rep.extra_remaps[i].sid << std::endl;
            ofs.write(reinterpret_cast<const char *>(&(rep.segids[i])), sizeof(T));
            ofs.write(reinterpret_cast<const char *>(&(rep.segids[rep.extra_remaps[i].sid])), sizeof(T));
        }
    }
    assert(!ofs.bad());
    ofs.close();
}

int main(int argc, char * argv[])
{
    std::string tag(argv[1]);
    auto rep = generate_remaps<seg_t>();
    std::cout << "remaps loaded" << std::endl;
    process_edges<seg_t, aff_t>(tag, rep);
    std::cout << "remap edges" << std::endl;
    auto bs = process_boundary_supervoxels<seg_t>(tag, rep);
    std::cout << "reduce boundary supervoxels" << std::endl;
    generate_extra_counts(rep, bs);
    std::cout << "generate extra supervoxel counts" << std::endl;
    process_counts(rep);
    std::cout << "reduce segment sizes" << std::endl;
    process_sems(rep);
    std::cout << "reduce semantic labels" << std::endl;
    process_size(rep);
    std::cout << "reduce sizes" << std::endl;
    write_extra_remaps<seg_t>(rep);
}
