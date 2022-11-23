#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <execution>
#include <boost/format.hpp>
#include "Types.h"
#include "Utils.hpp"
#include "SemExtractor.hpp"

template <typename T, typename S>
struct __attribute__((packed)) sem_data_t
{
    T sid;
    S sem;
    sem_data_t() = default;
    explicit sem_data_t(const std::pair<T, S> & p) {
        sid = p.first;
        sem = p.second;
    }
};

template <typename T>
struct __attribute__((packed)) remap_data_t
{
    T oid;
    T nid;
    remap_data_t() = default;
    explicit remap_data_t(const std::pair<T, T> & p) {
        oid = p.first;
        nid = p.second;
    }
};

template <typename T>
struct __attribute__((packed)) size_data_t
{
    T sid;
    size_t size;
    size_data_t() = default;
    explicit size_data_t(const std::pair<T, size_t> & p) {
        sid = p.first;
        size = p.second;
    }
};

template <typename T>
void write_vector(const std::string & filename, const std::vector<T> & data)
{
    std::ofstream ofs(filename, std::ios_base::binary);
    for (auto const & e : data) {
        ofs.write(reinterpret_cast<const char *>(&(e)), sizeof(T));
    }
    assert(!ofs.bad());
    ofs.close();
}

template <typename T>
std::pair<T, T> minmax_remap_pair(const T & s1, const T & s2, const MapContainer<T, T> & remaps)
{
    auto nid1 = remaps.contains(s1) ? remaps.at(s1) : s1;
    auto nid2 = remaps.contains(s2) ? remaps.at(s2) : s2;
    return std::minmax(nid1, nid2);
}

template <typename T>
MapContainer<T, T> consolidate_remap(const std::vector<remap_data_t<T> > & remap_data)
{
    MapContainer<T, T> remaps;
    for (auto it = remap_data.rbegin(); it != remap_data.rend(); ++it) {
        remaps[it->oid] = remaps.contains(it->nid) ? remaps.at(it->nid) : it->nid;
    }
    return remaps;
}

template <typename T>
MapContainer<T, size_t> populate_size(const std::vector<size_data_t<T> > & ongoing_size_array, const std::vector<size_data_t<T> > & done_size_array)
{
    MapContainer<T, size_t> counts;
    for (auto & e : ongoing_size_array) {
        counts[e.sid] = e.size;
    }

    for (auto & e : done_size_array) {
        counts[e.sid] = e.size;
    }
    return counts;
}

template <typename T>
SetContainer<T> reduce_boundaries(const std::string & tag, const MapContainer<T, T> &remaps, const MapContainer<T, size_t> & counts, MapContainer<T, T> & reduced_map)
{
    SetContainer<T> boundary_sv;
    for (size_t i = 0; i < 6; i++) {
        std::vector<matching_entry_t<T> > new_sids;
        SetContainer<T> nbs_seeds;
        auto data = read_array<T>(str(boost::format("boundary_%1%_%2%.data") % i % tag));
        auto extra_data = read_array<T>(str(boost::format("cut_plane_%1%_%2%.data") % i % tag));
        std::cout << "frozen sv before: " << data.size() << std::endl;
        for (auto const & bs : data) {
            boundary_sv.insert(bs);
            auto nbs = remaps.contains(bs) ? remaps.at(bs): bs;
            if (bs != nbs) {
                reduced_map[bs] = nbs;
            }
            if (counts.contains(nbs)) {
                new_sids.push_back(matching_entry_t<T>{bs, 1, nbs, counts.at(nbs)});
                nbs_seeds.insert(nbs);
            }
        }
        for (auto const & bs : extra_data) {
            auto nbs = remaps.contains(bs) ? remaps.at(bs): bs;
            if (counts.contains(nbs)) {
                nbs_seeds.insert(nbs);
            }
        }
        if (not nbs_seeds.empty()) {
            for (auto const & [k, v] : remaps) {
                if (nbs_seeds.contains(v) and (!reduced_map.contains(k))) {
                    new_sids.push_back(matching_entry_t<T>{k, 0, v, counts.at(v)});
                }
            }
        }
        std::cout << "frozen sv after: " << new_sids.size() << std::endl;
        write_vector(str(boost::format("reduced_boundary_%1%_%2%.data") % i % tag), new_sids);
    }
    return boundary_sv;
}

template <typename T>
void reduce_counts(const std::string & tag, const MapContainer<T, T> & remaps, const SetContainer<T> & bs, MapContainer<T, T> & reduced_map)
{
    auto size_array = read_array<size_data_t<T> >(str(boost::format("ongoing_supervoxel_counts_%1%.data") % tag));
    MapContainer<T, size_t> reduced_size;
    for (auto const & e : size_array) {
        if (bs.contains(e.sid)) {
            continue;
        }
        auto sid = remaps.contains(e.sid) ? remaps.at(e.sid) : e.sid;
        if (sid != e.sid) {
            reduced_map[e.sid] = sid;
        }
        if (reduced_size.contains(sid)) {
            reduced_size[sid] += e.size;
        } else {
            reduced_size[sid] = e.size;
        }
    }
    write_vector(str(boost::format("reduced_ongoing_supervoxel_counts_%1%.data") % tag), std::vector<size_data_t<T> >(reduced_size.cbegin(), reduced_size.cend()));
}

void merge_edges(const auto & edges, const auto & remaps, auto & merged_edges)
{
    for (const auto & e : edges) {
        auto p = minmax_remap_pair(e.s1, e.s2, remaps);
        if (p.first == p.second) {
            continue;
        }
        if (merged_edges.contains(p)) {
            merged_edges[p].first += e.aff;
            merged_edges[p].second += e.area;
        } else {
            merged_edges[p] = std::make_pair(e.aff, e.area);
        }
    }
}

template <typename Ts, typename Ta>
void reduce_edges(const std::string & tag, const MapContainer<Ts, Ts> & remaps)
{
    auto rg_array = read_array<rg_entry<Ts, Ta> >(str(boost::format("residual_rg_%1%.data") % tag));
    auto edge_array = read_array<rg_entry<Ts, Ta> >(str(boost::format("incomplete_edges_%1%.data") % tag));
    std::cout << "Total number of edges: " << rg_array.size() + edge_array.size() << std::endl;
    MapContainer<SegPair<Ts>, std::pair<Ta, size_t>, HashFunction<SegPair<Ts> > > merged_edges;
    merge_edges(rg_array, remaps, merged_edges);
    merge_edges(edge_array, remaps, merged_edges);
    std::cout << "Total number of reduced edges: " << merged_edges.size() << std::endl;
    write_vector(str(boost::format("reduced_edges_%1%.data") % tag), std::vector<rg_entry<Ts, Ta> >(merged_edges.cbegin(), merged_edges.cend()));
}

template <typename T, typename S>
void reduce_sem(const std::string & tag, const MapContainer<T, T> & remaps)
{
    auto sem_array = read_array<sem_data_t<T, S> >(str(boost::format("ongoing_semantic_labels_%1%.data") % tag));
    for (auto & e : sem_array) {
        if (remaps.contains(e.sid)) {
            e.sid = remaps.at(e.sid);
        }
    }
    write_vector(str(boost::format("reduced_ongoing_semantic_labels_%1%.data") % tag), sem_array);
}

template <typename T>
void reduce_size(const std::string & tag, const MapContainer<T, T> & remaps)
{
    auto size_array = read_array<size_data_t<T> >(str(boost::format("ongoing_seg_size_%1%.data") % tag));
    for (auto & e : size_array) {
        if (remaps.contains(e.sid)) {
            e.sid = remaps.at(e.sid);
        }
    }
    write_vector(str(boost::format("reduced_ongoing_seg_size_%1%.data") % tag), size_array);
}

int main(int argc, char ** argv)
{
    std::string tag(argv[1]);
    auto remap_array = read_array<remap_data_t<seg_t> >("remap.data");
    auto remaps = consolidate_remap(remap_array);
    auto ongoing_size_array = read_array<size_data_t<seg_t> >("ongoing_segments.data");
    auto done_size_array = read_array<size_data_t<seg_t> >("done_segments.data");
    auto counts = populate_size(ongoing_size_array, done_size_array);
    MapContainer<seg_t, seg_t> reduced_map;
    auto boundary_sv =  reduce_boundaries(tag, remaps, counts, reduced_map);
    reduce_counts(tag, remaps, boundary_sv, reduced_map);
    reduce_edges<seg_t, aff_t>(tag, remaps);
    reduce_sem<seg_t, sem_array_t>(tag, remaps);
    reduce_size(tag, remaps);
    write_vector("extra_remaps.data", std::vector<remap_data_t<seg_t> >(reduced_map.cbegin(), reduced_map.cend()));
}
