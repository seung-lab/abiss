//
// Copyright (C) 2013-2016  Aleksandar Zlateski <zlateski@mit.edu>
// Copyright (C) 2016-present  Ran Lu <ranl@princeton.edu>
// ------------------------------------------------------------------
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <boost/heap/fibonacci_heap.hpp>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <limits>
#include <unordered_map>
#include <map>
#include <vector>
#include <unordered_set>
#include <execution>
#include <sys/stat.h>

#include "../seg/SemExtractor.hpp"
#include "../seg/RemapTable.hpp"

using seg_t = uint64_t;
#ifdef DOUBLE
using aff_t = double;
#else
using aff_t = float;
#endif

static const size_t frozen = (1ul<<(std::numeric_limits<std::size_t>::digits-2));
static const size_t boundary = (1ul<<(std::numeric_limits<std::size_t>::digits-1))|frozen;

size_t filesize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

bool is_frozen(size_t size) {
    return size & frozen;
}

void print_neighbors(auto neighbors, const auto source)
{
    std::cout << "neighbors of " << source << ":";
    for (auto & e : neighbors) {
        std::cout << e.segid(source) << " ";
    }
    std::cout << std::endl;
}

bool frozen_neighbors(const auto & neighbors, const auto & supervoxel_counts, const auto source)
{
    for (auto & kv : neighbors) {
        auto sid = kv.first;
        if (is_frozen(supervoxel_counts[sid])) {
            return true;
        }
    }
    return false;
}

template <class T>
struct __attribute__((packed)) edge_t
{
    seg_t v0, v1;
    T        w;
};

typedef struct __attribute__((packed)) atomic_edge
{
    seg_t u1;
    seg_t u2;
    aff_t sum_aff;
    size_t area;
    explicit constexpr atomic_edge(seg_t w1 = 0, seg_t w2 = 0, aff_t s_a = 0.0, size_t a = 0)
        : u1(w1)
        , u2(w2)
        , sum_aff(s_a)
        , area(a)
    {
    }
} atomic_edge_t;

struct __attribute__((packed)) mst_edge
{
    aff_t sum;
    size_t num;
    atomic_edge_t repr;

    explicit constexpr mst_edge(aff_t s = 0, size_t n = 1, atomic_edge_t r = atomic_edge_t() )
        : sum(s)
        , num(n)
        , repr(r)
    {
    }
};

struct mst_edge_plus
{
    mst_edge operator()(mst_edge const& a, mst_edge const& b) const
    {
        atomic_edge_t new_repr = a.repr;
        if (a.repr.area < b.repr.area) {
            new_repr = b.repr;
        }
        return mst_edge(a.sum + b.sum, a.num + b.num, new_repr);
    }
};

struct mst_edge_greater
{
    bool operator()(mst_edge const& a, mst_edge const& b) const
    {
        return a.sum / a.num > b.sum / b.num;
    }
};

struct mst_edge_limits
{
    static constexpr mst_edge max()
    {
        return mst_edge(std::numeric_limits<aff_t>::max());
    }
    static constexpr mst_edge nil()
    {
        return mst_edge(0,0);
    }
};

struct __attribute__((packed)) mean_edge
{
    aff_t sum;
    size_t num;

    explicit constexpr mean_edge(aff_t s = 0, size_t n = 1)
        : sum(s)
        , num(n)
    {
    }
};

struct mean_edge_plus
{
    mean_edge operator()(mean_edge const& a, mean_edge const& b) const
    {
        return mean_edge(a.sum + b.sum, a.num + b.num);
    }
};

struct mean_edge_greater
{
    bool operator()(mean_edge const& a, mean_edge const& b) const
    {
        return a.sum / a.num > b.sum / b.num;
    }
};

struct mean_edge_limits
{
    static constexpr mean_edge max()
    {
        return mean_edge(std::numeric_limits<aff_t>::max());
    }
    static constexpr mean_edge nil()
    {
        return mean_edge(0,0);
    }
};

template <class T>
using region_graph = std::vector<edge_t<T>>;

template <class T, class C = std::greater<T>>
struct heapable_edge;

template <class T, class C = std::greater<T>>
struct heapable_edge_compare
{
    bool operator()(heapable_edge<T, C> const & a,
                    heapable_edge<T, C> const & b) const
    {
        C c;
        return c(b.edge->w, a.edge->w);
    }
};

template <class T, class C = std::greater<T>>
using heap_type = boost::heap::fibonacci_heap<
    heapable_edge<T, C>, boost::heap::compare<heapable_edge_compare<T, C>>>;

template <class T, class C = std::greater<T>>
using heap_handle_type = typename heap_type<T, C>::handle_type;

template <class T, class C>
struct __attribute__((packed)) heapable_edge
{
    edge_t<T> * edge;
    explicit constexpr heapable_edge(edge_t<T> * e)
        : edge(e) {};
};

template <class T, class C>
struct handle_wrapper
{
    edge_t<T> * edge;
    heap_handle_type<T, C> handle;

    explicit constexpr handle_wrapper(edge_t<T> * e, heap_handle_type<T, C> & h)
        : handle(h) {
            edge = e;
        };
    bool valid_handle() const { return handle != heap_handle_type<T, C>(); }
    seg_t segid(const seg_t exclude) const {
        return exclude == edge->v0 ? edge->v1 : edge->v0;
    }
};

template <class T, class Compare = std::greater<T> >
struct agglomeration_data_t
{
    std::vector<std::unordered_map<seg_t, handle_wrapper<T, Compare> > > incident;
    std::vector<edge_t<T> > rg_vector;
    std::vector<size_t> supervoxel_counts;
    std::vector<seg_t> seg_indices;
    std::vector<sem_array_t> sem_counts;
    std::vector<size_t> seg_size;
};

template <class T>
std::vector<T> read_array(const char * filename)
{
    std::vector<T> array;

    std::cout << "filename: " << filename << ", " << "filesize:" << filesize(filename)/sizeof(T) << std::endl;
    size_t data_size = filesize(filename);
    if (data_size % sizeof(T) != 0) {
        std::cerr << "File incomplete!: " << filename << std::endl;
        std::abort();
    }

    FILE* f = std::fopen(filename, "rbXS");
    if ( !f ) {
        std::cerr << "Cannot open the input file " << filename << std::endl;
        std::abort();
    }

    size_t array_size = data_size / sizeof(T);

    array.resize(array_size);
    std::size_t nread = std::fread(array.data(), sizeof(T), array_size, f);
    if (nread != array_size) {
        std::cerr << "Reading: " << nread << " entries, but expecting: " << array_size << std::endl;
        std::abort();
    }
    std::fclose(f);

    return array;
}

std::vector<size_t >
load_size(const char * size_filename, const std::vector<seg_t> & seg_indices)
{
    std::vector<std::pair<seg_t, size_t> > size_array = read_array<std::pair<seg_t, size_t> >(size_filename);
    if (size_array.empty()) {
        if (not seg_indices.empty()) {
            std::cout << "Error: No seg size data" << std::endl;
        }
    }

    std::vector<size_t > size_counts(seg_indices.size());

    std::for_each(std::execution::par, size_array.begin(), size_array.end(), [&seg_indices](auto & a) {
        auto it = std::lower_bound(seg_indices.begin(), seg_indices.end(), a.first);
        if (it == seg_indices.end()) {
            std::cerr << "Should not happen, size element does not exist: " << a.first << std::endl;
            std::abort();
        }
        if (a.first == *it) {
            a.first = std::distance(seg_indices.begin(), it);
        } else {
            std::cerr << "Should not happen, cannot find size entry: " << a.first  << "," << *it << std::endl;
            std::abort();
        }
    });

    std::sort(std::execution::par, std::begin(size_array), std::end(size_array), [](auto & a, auto & b) { return a.first < b.first; });

    for (auto & [k, v] : size_array) {
        size_counts[k] += v;
    }
    return size_counts;

}


std::vector<sem_array_t> load_sem(const char * sem_filename, const std::vector<seg_t> & seg_indices)
{
    std::vector<std::pair<seg_t, sem_array_t> > sem_array = read_array<std::pair<seg_t, sem_array_t> >(sem_filename);
    if (sem_array.empty()) {
        std::cout << "No semantic labels" << std::endl;
        return std::vector<sem_array_t>();
    }

    std::vector<sem_array_t> sem_counts(seg_indices.size());

    std::for_each(std::execution::par, sem_array.begin(), sem_array.end(), [&seg_indices](auto & a) {
        auto it = std::lower_bound(seg_indices.begin(), seg_indices.end(), a.first);
        if (it == seg_indices.end()) {
            std::cerr << "Should not happen, sem element does not exist: " << a.first << std::endl;
            std::abort();
        }
        if (a.first == *it) {
            a.first = std::distance(seg_indices.begin(), it);
        } else {
            std::cerr << "Should not happen, cannot find sem entry: " << a.first  << "," << *it << std::endl;
            std::abort();
        }
    });

    std::sort(std::execution::par, std::begin(sem_array), std::end(sem_array), [](auto & a, auto & b) { return a.first < b.first; });

    for (auto & [k, v] : sem_array) {
        std::transform(sem_counts[k].begin(), sem_counts[k].end(), v.begin(), sem_counts[k].begin(), std::plus<size_t>());
    }
    return sem_counts;
}

template <class T, class Compare = std::greater<T>, class Plus = std::plus<T> >
void merge_edges(agglomeration_data_t<T, Compare> & agg_data)
{
    Plus plus;
    std::vector<edge_t<T> > new_rg_vector;
    std::vector<edge_t<T> > & rg_vector = agg_data.rg_vector;
    std::sort(std::execution::par, std::begin(rg_vector), std::end(rg_vector), [](auto & a, auto & b) { return (a.v0 < b.v0) || (a.v0 == b.v0 && a.v1 < b.v1);  });
    std::cout << "rg_vector size:" << rg_vector.size() << std::endl;

    edge_t<T> current_edge;
    current_edge.v0 = seg_t(std::numeric_limits<seg_t>::max());
    current_edge.v1 = seg_t(std::numeric_limits<seg_t>::max());
    for (const auto & e : rg_vector) {
        if (e.w.sum == 0 && e.w.num == 0) {
            continue;
        }
        if (current_edge.v0 != e.v0 || current_edge.v1 != e.v1) {
            if (current_edge.v0 != seg_t(std::numeric_limits<seg_t>::max()) && current_edge.v1 != seg_t(std::numeric_limits<seg_t>::max())) {
                new_rg_vector.push_back(current_edge);
            }
            current_edge = e;
        } else {
            current_edge.w = plus(current_edge.w, e.w);
        }
    }
    if (current_edge.v0 != seg_t(std::numeric_limits<seg_t>::max()) && current_edge.v1 != seg_t(std::numeric_limits<seg_t>::max())) {
        new_rg_vector.push_back(current_edge);
    }
    std::cout << "new_rg_vector size:" << new_rg_vector.size() << std::endl;
    std::swap(rg_vector, new_rg_vector);
}

template <class T, class Compare = std::greater<T>, class Plus = std::plus<T> >
void remap_edges(agglomeration_data_t<T, Compare> & agg_data, std::vector<std::vector<std::pair<seg_t, seg_t> > > & remaps)
{
    RemapTable<seg_t> remapTable;
    std::cout << "merge " << remaps.size() << " tables!" << std::endl;
    for (auto & m: remaps) {
        for (auto & p: boost::adaptors::reverse(m)) {
            remapTable.updateRemap(p.first, p.second);
        }
    }
    auto remap = remapTable.globalMap();
    std::cout << remap.size() << " remaps" << std::endl;
    auto & rg_vector = agg_data.rg_vector;
    std::for_each(std::execution::par, rg_vector.begin(), rg_vector.end(), [&remap](auto & a) {
            auto search0 = remap.find(a.v0);
            auto search1 = remap.find(a.v1);
            if (search0 != remap.end()) {
                a.v0 = search0->second;
            }
            if (search1 != remap.end()) {
                a.v1 = search1->second;
            }
            if (a.v1 < a.v0) {
                auto tmp = a.v1;
                a.v1 = a.v0;
                a.v0 = tmp;
            }
    });
    merge_edges<T, Compare, Plus>(agg_data);
}

template <class T, class Compare = std::greater<T>>
std::vector<seg_t> extract_cc(const agglomeration_data_t<T, Compare> & agg_data,  T const & threshold)
{
    Compare comp;
    auto & rg_vector = agg_data.rg_vector;
    auto & seg_indices = agg_data.seg_indices;
    auto & supervoxel_counts = agg_data.supervoxel_counts;

    std::vector<seg_t> bc_rank(seg_indices.size()+1);
    std::vector<seg_t> bc_parent(seg_indices.size()+1);
    boost::disjoint_sets<seg_t*, seg_t*> bc_sets(&bc_rank[0], &bc_parent[0]);
    for (size_t i = 0; i != seg_indices.size(); i++) {
        bc_sets.make_set(i);
    }
    auto boundary_cc = seg_indices.size();
    bc_sets.make_set(boundary_cc);

    for (const auto & e : rg_vector) {
        if (comp(e.w, threshold)){
            bc_sets.union_set(e.v0, e.v1);
            if (is_frozen(supervoxel_counts[e.v0]) || is_frozen(supervoxel_counts[e.v1])) {
                bc_sets.union_set(e.v0, boundary_cc);
            }
        }
    }

    std::vector<seg_t> cc_rank(seg_indices.size());
    std::vector<seg_t> cc_parent(seg_indices.size());
    boost::disjoint_sets<seg_t*, seg_t*> cc_sets(&cc_rank[0], &cc_parent[0]);
    for (size_t i = 0; i != seg_indices.size(); i++) {
        cc_sets.make_set(i);
    }

    for (const auto & e : rg_vector) {
        if (comp(e.w, threshold)){
            cc_sets.union_set(e.v0, e.v1);
        } else {
            if (bc_sets.find_set(e.v0) == bc_sets.find_set(boundary_cc) or bc_sets.find_set(e.v1) == bc_sets.find_set(boundary_cc)) {
                cc_sets.union_set(e.v0, e.v1);
            }
        }
    }

    std::vector<seg_t> idx(seg_indices.size());
    std::iota(idx.begin(), idx.end(), seg_t(0));
    cc_sets.compress_sets(idx.cbegin(), idx.cend());
    std::cout << "connect components: " << cc_sets.count_sets(idx.cbegin(), idx.cend()) << std::endl;
    return cc_parent;
}

template<typename T>
std::vector<std::pair<seg_t, size_t> > cc_edge_offsets(std::vector<edge_t<T> > rg_vector, std::vector<seg_t> & ccids)
{
    seg_t ccid = std::numeric_limits<seg_t>::max();
    std::vector<std::pair<seg_t, size_t> > cc_edges;
    for (size_t i = 0; i != rg_vector.size(); i++) {
        const auto & e = rg_vector[i];
        if (ccids[e.v0] != ccids[e.v1]) {
            cc_edges.emplace_back(ccid, i);
            break;
        }
        if (ccid != ccids[e.v0]) {
            if (ccid != std::numeric_limits<seg_t>::max()) {
                cc_edges.emplace_back(ccid, i);
            }
            ccid = ccids[e.v0];
        }
    }
    if ((cc_edges.empty() or ccid != cc_edges.back().first) and ccid != std::numeric_limits<seg_t>::max()) {
        cc_edges.emplace_back(ccid, rg_vector.size());
    }
    return cc_edges;
}

template<typename T>
void partition_rg(std::vector<edge_t<T> > & rg_vector, const std::vector<seg_t> & ccids, const std::vector<std::pair<seg_t, size_t> > & cc_queue)
{
    std::sort(std::execution::par, std::begin(rg_vector), std::end(rg_vector), [&](auto & a, auto & b) {
            seg_t a0 = ccids[a.v0];
            seg_t a1 = ccids[a.v1];
            seg_t b0 = ccids[b.v0];
            seg_t b1 = ccids[b.v1];
            if (a0 == a1) {
                if (b0 == b1) {
                    auto r1 = std::lower_bound(cc_queue.begin(), cc_queue.end(), a0, [](const auto & p, seg_t target) {return p.first < target;});
                    auto r2 = std::lower_bound(cc_queue.begin(), cc_queue.end(), b0, [](const auto & p, seg_t target) {return p.first < target;});
                    if (r1 == cc_queue.end() or (*r1).first != a0) {
                        std::cout << "failed binary search 1" << std::endl;
                        std::abort();
                    }
                    if (r2 == cc_queue.end() or (*r2).first != b0) {
                        std::cout << "failed binary search 2" << std::endl;
                        std::abort();
                    }
                    return ((*r1).second > (*r2).second) || ((*r1).second == (*r2).second && std::distance(cc_queue.begin(), r1) < std::distance(cc_queue.begin(), r2));
                } else {
                    return true;
                }
            } else {
                return false;
            }
    });
    std::cout << "finished partition the edges" << std::endl;
}

std::vector<std::pair<seg_t, size_t> > sorted_components(std::vector<seg_t> ccids)
{
    std::sort(std::execution::par, ccids.begin(), ccids.end());
    std::vector<std::pair<seg_t, size_t> > cc_comps;

    if (ccids.empty()) {
        std::cerr << "no segments, empty chunk???" << std::endl;
        return cc_comps;
    }
    seg_t cc = ccids[0];
    size_t count = 0;
    for (const auto id: ccids) {
        if (id != cc) {
            if (count > 1)
                cc_comps.emplace_back(cc, count);
            cc = id;
            count = 0;
        }
        count += 1;
    }

    if (count > 1) {
        cc_comps.emplace_back(cc, count);
    }

    if (cc_comps.empty()) {
        std::cerr << "no connect components" << std::endl;
        return cc_comps;
    }

    std::sort(std::execution::par, cc_comps.begin(), cc_comps.end(), [](auto & a, auto & b) {
        return a.first < b.first;
    });

    return cc_comps;
}

template <class T, class Compare = std::greater<T>, class Plus = std::plus<T> >
inline agglomeration_data_t<T, Compare> preprocess_inputs(const char * rg_filename, const char * fs_filename, const char * ns_filename)
{
    Plus plus;
    agglomeration_data_t<T, Compare> agg_data;
    auto & supervoxel_counts = agg_data.supervoxel_counts;
    auto & seg_indices = agg_data.seg_indices;

    auto rg_vector = read_array<edge_t<T> >(rg_filename);

    std::vector<std::pair<seg_t, size_t> > ns_pair_array = read_array<std::pair<seg_t, size_t> >(ns_filename);
    std::vector<seg_t> fs_array = read_array<seg_t>(fs_filename);

    std::transform(fs_array.begin(), fs_array.end(), std::back_inserter(ns_pair_array), [](seg_t &a)->std::pair<seg_t, size_t> {
            return std::make_pair(a, size_t(boundary));
            });

    std::sort(std::execution::par, std::begin(ns_pair_array), std::end(ns_pair_array), [](auto & a, auto & b) { return a.first < b.first || (a.first == b.first && a.second < b.second); });

    seg_t prev_seg = 0;
    for (auto & kv : ns_pair_array) {
        auto seg = kv.first;
        auto count = kv.second;
        if (seg == 0) {
            std::cerr << "Impossible, there is no 0 segment" <<std::endl;
            std::abort();
        }
        if (seg != prev_seg) {
            seg_indices.push_back(seg);
            supervoxel_counts.push_back(count);
            prev_seg = seg;
        } else {
#if defined(OVERLAPPED)
            if (boundary & count) {
                supervoxel_counts.back() |= boundary;
            }
            else {
                supervoxel_counts.back() += count;
            }
#else
            if (count == boundary) {
                supervoxel_counts.back() |= boundary;
            } else {
                supervoxel_counts.back() += (count & (~boundary)) - 1;
                supervoxel_counts.back() |= count & boundary;
            }
#endif
        }
    }

    agg_data.sem_counts = load_sem("ongoing_semantic_labels.data", seg_indices);
    agg_data.seg_size = load_size("ongoing_seg_size.data", seg_indices);
    agg_data.incident.resize(seg_indices.size());

    std::for_each(std::execution::par, rg_vector.begin(), rg_vector.end(), [&seg_indices](auto & a) {
            size_t u0, u1;
            auto it = std::lower_bound(seg_indices.begin(), seg_indices.end(), a.v0);
            if (it == seg_indices.end()) {
                std::cerr << "Should not happen, rg element does not exist: " << a.v0 << std::endl;
                std::abort();
            }
            if (a.v0 == *it) {
                u0 = std::distance(seg_indices.begin(), it);
            } else {
                std::cerr << "Should not happen, cannot find entry: " << a.v0  << "," << *it << std::endl;
                std::abort();
            }
            it = std::lower_bound(seg_indices.begin(), seg_indices.end(), a.v1);
            if (it == seg_indices.end()) {
                std::cerr << "Should not happen, rg element does not exist: " << a.v1 << std::endl;
                std::abort();
            }
            if (a.v1 == *it) {
                u1 = std::distance(seg_indices.begin(), it);
            } else {
                std::cerr << "Should not happen, cannot find entry: " << a.v1  << "," << *it << std::endl;
                std::abort();
            }
            if (u0 < u1) {
                a.v0 = u0;
                a.v1 = u1;
            } else {
                a.v0 = u1;
                a.v1 = u0;
            }
        });
    std::swap(agg_data.rg_vector, rg_vector);

    merge_edges<T, Compare, Plus>(agg_data);

    return agg_data;
}

template <class T, class Compare = std::greater<T> >
inline heap_type<T, Compare> populate_heap(agglomeration_data_t<T, Compare> & agg_data, size_t startpos, size_t endpos, T const & threshold)
{
    Compare comp;
    heap_type<T, Compare> heap;
    auto & incident = agg_data.incident;
    auto & supervoxel_counts = agg_data.supervoxel_counts;
    auto & rg_vector = agg_data.rg_vector;
    auto & seg_indices = agg_data.seg_indices;

    size_t i = 0;

    for (size_t j = startpos; j != endpos; j++) {
        auto & e = rg_vector[j];
        heap_handle_type<T, Compare> handle;
        if (comp(e.w, threshold)){
            handle = heap.emplace(& e);
        }
        auto v0 = e.v0;
        auto v1 = e.v1;
        incident[e.v0].insert({v1,handle_wrapper<T, Compare>(&e, handle)});
        incident[e.v1].insert({v0,handle_wrapper<T, Compare>(&e, handle)});
        i++;
        if (i % 10000000 == 0) {
            std::cout << "reading " << i << "th edge" << std::endl;
        }
    }

    return heap;
}

std::pair<size_t, size_t> sem_label(const sem_array_t & labels)
{
    auto label = std::max_element(labels.begin(), labels.end());
    return std::make_pair(std::distance(labels.begin(), label), (*label));
}

bool sem_can_merge(const sem_array_t & labels1, const sem_array_t & labels2)
{
    auto max_label1 = std::distance(labels1.begin(), std::max_element(labels1.begin(), labels1.end()));
    auto max_label2 = std::distance(labels2.begin(), std::max_element(labels2.begin(), labels2.end()));
    auto total_label1 = std::accumulate(labels1.begin(), labels1.end(), size_t(0));
    auto total_label2 = std::accumulate(labels2.begin(), labels2.end(), size_t(0));
    if (labels1[max_label1] < 0.6 * total_label1 || total_label1 < 100000) { //unsure about the semantic label
        return true;
    }
    if (labels2[max_label2] < 0.6 * total_label2 || total_label2 < 100000) { //unsure about the semantic label
        return true;
    }
    if (max_label1 == max_label2) {
        return true;
    }
    return false;
}

template <class T, class Compare = std::greater<T>, class Plus = std::plus<T>,
          class Limits = std::numeric_limits<T>>
inline void agglomerate(const char * rg_filename, const char * fs_filename, const char * ns_filename, aff_t th)
{
    Compare comp;
    Plus    plus;

    T const final_threshold = T(th,1);
    T const h_threshold = T(0.5,1);
    const size_t small_threshold = 1000000;
    const size_t large_threshold = 10000000;

    size_t mst_size = 0;
    size_t residue_size = 0;

    float print_th = 1.0 - 0.01;
    size_t num_of_edges = 0;

    auto agg_data = preprocess_inputs<T, Compare, Plus>(rg_filename, fs_filename, ns_filename);

    auto & incident = agg_data.incident;
    auto & rg_vector = agg_data.rg_vector;
    auto & supervoxel_counts = agg_data.supervoxel_counts;
    auto & seg_indices = agg_data.seg_indices;
    auto & sem_counts = agg_data.sem_counts;
    auto & seg_size = agg_data.seg_size;

    auto heap = populate_heap<T, Compare>(agg_data, 0, rg_vector.size(), final_threshold);

    size_t rg_size = heap.size();


    std::ofstream of_mst;
    of_mst.open("mst.data", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_remap;
    of_remap.open("remap.data", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_res;
    of_res.open("residual_rg.data", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_reject;
    of_reject.open("rejected_edges.log", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_sem_cuts;
    of_sem_cuts.open("sem_cuts.data", std::ofstream::out | std::ofstream::trunc);

    std::cout << "looping through the heap" << std::endl;


    while (!heap.empty() && comp(heap.top().edge->w, final_threshold))
    {
        num_of_edges += 1;
        auto e = heap.top();
        auto v0 = e.edge->v0;
        auto v1 = e.edge->v1;

        incident[v0].erase(v1);
        incident[v1].erase(v0);
        heap.pop();

        if (e.edge->w.sum/e.edge->w.num < print_th) {
            std::cout << "Processing threshold: " << print_th << std::endl;
            std::cout << "Numer of edges: " << num_of_edges << "(" << rg_size << ")"<< std::endl;
            print_th -= 0.01;
        }

        if (v0 != v1)
        {

            auto s = v0;
#ifdef EXTRA
            if ((is_frozen(supervoxel_counts[v0]) && is_frozen(supervoxel_counts[v1]))
                || (is_frozen(supervoxel_counts[v0]) && (frozen_neighbors(incident[v1], supervoxel_counts, v1) || (!comp(e.edge->w, h_threshold) && (!sem_counts.empty() || seg_size[v1] > small_threshold))))
                || (is_frozen(supervoxel_counts[v1]) && (frozen_neighbors(incident[v0], supervoxel_counts, v0) || (!comp(e.edge->w, h_threshold) && (!sem_counts.empty() || seg_size[v0] > small_threshold))))) {
#else
            if ((is_frozen(supervoxel_counts[v0]) || is_frozen(supervoxel_counts[v1]))) {
#endif

                supervoxel_counts[v0] |= frozen;
                supervoxel_counts[v1] |= frozen;
                of_res.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
                of_res.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
                write_edge(of_res, e.edge->w);
                residue_size++;
                continue;
            }

            if (!comp(e.edge->w, h_threshold)) {
                if (!sem_counts.empty()){
                    //auto sem0 = sem_label(sem_counts[v0]);
                    //auto sem1 = sem_label(sem_counts[v1]);
                    //if (sem0.first != sem1.first && sem0.second > 10000 && sem1.second > 10000) {
                    //    of_sem_cuts.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
                    //    of_sem_cuts.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
                    //    continue;
                    //}
                    if (!sem_can_merge(sem_counts[v0],sem_counts[v1])) {
                        std::cout << seg_indices[v0] << ", " << seg_indices[v1] << ", " << supervoxel_counts[v0] << ", " << supervoxel_counts[v1] << std::endl;
                        std::cout << "reject merge between " << seg_indices[v0] << "(" << sem_counts[v0][1] << "," << sem_counts[v0][2] << ")"<< " and " << seg_indices[v1] << "(" << sem_counts[v1][1] << "," << sem_counts[v1][2] << ")"<< std::endl;
                        of_sem_cuts.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
                        of_sem_cuts.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
                        continue;
                    }
                }

                size_t size0 = seg_size[v0];
                size_t size1 = seg_size[v1];
                auto p = std::minmax({size0, size1});
                if (p.first > small_threshold and p.second > large_threshold) {
                    std::cout << "reject edge between " << seg_indices[v0] << "(" << size0 << ")"<< " and " << seg_indices[v1] << "(" << size1 << ")"<< std::endl;
                    of_reject.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
                    of_reject.write(reinterpret_cast<const char *>(&(size0)), sizeof(size0));
                    of_reject.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
                    of_reject.write(reinterpret_cast<const char *>(&(size1)), sizeof(size1));
                    write_edge(of_reject, e.edge->w);
                    continue;
                }
            }
#ifdef FINAL
            if (incident[v0].size() < incident[v1].size()) {
                s = v1;
            }
#else
            if (supervoxel_counts[v0] < supervoxel_counts[v1]) {
                s = v1;
            }
#endif
            if (is_frozen(supervoxel_counts[v0])) {
                s = v0;
            } else if (is_frozen(supervoxel_counts[v1])) {
                s = v1;
            }

            //std::cout << "Join " << v0 << " and " << v1 << std::endl;
            supervoxel_counts[v0] += supervoxel_counts[v1];
            supervoxel_counts[v1] = 0;
            std::swap(supervoxel_counts[v0], supervoxel_counts[s]);

            seg_size[v0] += seg_size[v1];
            seg_size[v1] = 0;
            std::swap(seg_size[v0], seg_size[s]);

            if (!sem_counts.empty()) {
                std::transform(sem_counts[v0].begin(), sem_counts[v0].end(), sem_counts[v1].begin(), sem_counts[v0].begin(), std::plus<size_t>());
                sem_counts[v1] = sem_array_t();
                std::swap(sem_counts[v0], sem_counts[s]);
            }

            of_mst.write(reinterpret_cast<const char *>(&(seg_indices[s])), sizeof(seg_t));
            of_mst.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
            of_mst.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
            mst_size++;
            write_edge(of_mst, e.edge->w);
            if (v0 == s) {
                of_remap.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
                of_remap.write(reinterpret_cast<const char *>(&(seg_indices[s])), sizeof(seg_t));
            } else if (v1 == s) {
                of_remap.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
                of_remap.write(reinterpret_cast<const char *>(&(seg_indices[s])), sizeof(seg_t));
            } else {
                std::cout << "Something is wrong in the MST" << std::endl;
                std::cout << "s: " << s << ", v0: " << v0 << ", v1: " << v1 << std::endl;
                std::abort();
            }

            if (s == v0)
            {
                std::swap(v0, v1);
            }

            // v0 is dissapearing from the graph

            // loop over other edges e0 = {v0,v}
            for (auto p: incident[v0]) {
                auto v = p.first;
                auto e0 = p.second;
                if (v == v0) {
                    std::cerr << "loop in the incident matrix: " << v << std::endl;
                    std::abort();
                }

                incident[v].erase(v0);

                //auto it = search_neighbors(incident[v1], v1, v);
                //if (it != std::end(incident[v1]) && (*it).segid(v1) == v)
                if (incident[v1].count(v) != 0)
                                                  // {v0,v} and {v1,v} exist, we
                                                  // need to merge them
                {
                    auto & e = incident[v1].at(v);
                    if(e0.edge->v0 == v0) {
                        e0.edge->v0 = v1;
                    }
                    if(e0.edge->v1 == v0) {
                        e0.edge->v1 = v1;
                    }
                    if (!e.valid_handle()) {
                        auto & e_dual = incident[v].at(v1);
                        std::swap(e0.edge, e.edge);
                        std::swap(e0.handle, e.handle);
                        e_dual.edge = e.edge;
                        e_dual.handle = e.handle;
                    }
                    e.edge->w=plus(e.edge->w, e0.edge->w);
                    e0.edge->w = Limits::nil();
                    if (e.valid_handle()) {
                        heap.update(e.handle);
                        if (e0.valid_handle()) {
                            heap.erase(e0.handle);
                        }
                    }
                }
                else
                {
                    auto e = e0.edge;
                    if (e->v0 == v0)
                        e->v0 = v1;
                    if (e->v1 == v0)
                        e->v1 = v1;
                    incident[v].insert({v1,e0});
                    incident[v1].insert({v,e0});
                }
            }
            incident[v0].clear();
        }
    }

    assert(!of_mst.bad());
    assert(!of_remap.bad());

    of_mst.close();
    of_remap.close();

    std::cout << "edges frozen above threshold: " << residue_size << std::endl;
    std::ofstream of_frg;
    of_frg.open("final_rg.data", std::ofstream::out | std::ofstream::trunc);

    for (auto e : agg_data.rg_vector)
    {
        if (e.w.sum == 0 && e.w.num == 0) {
            continue;
        }
        if (comp(e.w, final_threshold)) {
            continue;
        }
        auto v0 = e.v0;
        auto v1 = e.v1;
        if (is_frozen(supervoxel_counts[v0]) && is_frozen(supervoxel_counts[v1])) {
            of_res.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
            of_res.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
            residue_size++;
            write_edge(of_res, e.w);
        } else {
            of_frg.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
            of_frg.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
            write_edge(of_frg, e.w);

        }
    }


    std::cout << "edges frozen: " << residue_size << std::endl;

    assert(!of_res.bad());
    assert(!of_frg.bad());
    assert(!of_reject.bad());
    assert(!of_sem_cuts.bad());
    of_res.close();
    of_frg.close();
    of_reject.close();
    of_sem_cuts.close();

    std::ofstream of_meta;
    of_meta.open("meta.data", std::ofstream::out | std::ofstream::trunc);
    size_t dummy = 0;
    of_meta.write(reinterpret_cast<const char *>(&(dummy)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(dummy)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(dummy)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(rg_size)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(residue_size)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(mst_size)), sizeof(size_t));
    assert(!of_meta.bad());
    of_meta.close();

    std::ofstream of_fs_ongoing, of_fs_done, of_sem_ongoing, of_sem_done, of_size_ongoing, of_size_done;

    of_fs_ongoing.open("ongoing_segments.data", std::ofstream::out | std::ofstream::trunc);
    of_fs_done.open("done_segments.data", std::ofstream::out | std::ofstream::trunc);

    of_sem_ongoing.open("ongoing_sem.data", std::ofstream::out | std::ofstream::trunc);
    of_sem_done.open("done_sem.data", std::ofstream::out | std::ofstream::trunc);

    of_size_ongoing.open("ongoing_size.data", std::ofstream::out | std::ofstream::trunc);
    of_size_done.open("done_size.data", std::ofstream::out | std::ofstream::trunc);

    for (size_t i = 0; i < supervoxel_counts.size(); i++) {
        if (!(supervoxel_counts[i] & (~frozen))) {
            if (supervoxel_counts[i] != 0) {
                std::cout << "SHOULD NOT HAPPEN, frozen supervoxel agglomerated: " << seg_indices[i] << std::endl;
                std::abort();
            }
            continue;
        }
        auto sv_count = supervoxel_counts[i];
#ifndef OVERLAPPED
        if (sv_count == (boundary)) {
            sv_count = 1|boundary;
        }
#endif
        if (is_frozen(sv_count)) {
            auto size = sv_count & (~boundary);
            of_fs_ongoing.write(reinterpret_cast<const char *>(&(seg_indices[i])), sizeof(seg_t));
            of_fs_ongoing.write(reinterpret_cast<const char *>(&(size)), sizeof(size_t));
            if (!sem_counts.empty()) {
                of_sem_ongoing.write(reinterpret_cast<const char *>(&(seg_indices[i])), sizeof(seg_t));
                of_sem_ongoing.write(reinterpret_cast<const char *>(&(sem_counts[i])), sizeof(sem_array_t));
            }
            of_size_ongoing.write(reinterpret_cast<const char *>(&(seg_indices[i])), sizeof(seg_t));
            of_size_ongoing.write(reinterpret_cast<const char *>(&(seg_size[i])), sizeof(size_t));
        } else {
            of_fs_done.write(reinterpret_cast<const char *>(&(seg_indices[i])), sizeof(seg_t));
            of_fs_done.write(reinterpret_cast<const char *>(&(sv_count)), sizeof(size_t));
            if (!sem_counts.empty()) {
                of_sem_done.write(reinterpret_cast<const char *>(&(seg_indices[i])), sizeof(seg_t));
                of_sem_done.write(reinterpret_cast<const char *>(&(sem_counts[i])), sizeof(sem_array_t));
            }
            of_size_done.write(reinterpret_cast<const char *>(&(seg_indices[i])), sizeof(seg_t));
            of_size_done.write(reinterpret_cast<const char *>(&(seg_size[i])), sizeof(size_t));
        }
    }

    assert(!of_fs_ongoing.bad());
    assert(!of_fs_done.bad());

    assert(!of_sem_ongoing.bad());
    assert(!of_sem_done.bad());

    assert(!of_size_ongoing.bad());
    assert(!of_size_done.bad());

    of_fs_ongoing.close();
    of_fs_done.close();

    of_sem_ongoing.close();
    of_sem_done.close();

    of_size_ongoing.close();
    of_size_done.close();

    return;
}

template <class CharT, class Traits>
::std::basic_ostream<CharT, Traits>&
operator<<(::std::basic_ostream<CharT, Traits>& os, mst_edge const& v)
{
    os << v.sum << " " << v.num << " " << v.repr.u1 << " " <<  v.repr.u2 << " " << v.repr.sum_aff << " " << v.repr.area;
    return os;
}

template <class CharT, class Traits>
::std::basic_ostream<CharT, Traits>&
operator<<(::std::basic_ostream<CharT, Traits>& os, mean_edge const& v)
{
    os << v.sum << " " << v.num;
    return os;
}

template <class CharT, class Edge, class Traits>
void write_edge(::std::basic_ostream<CharT, Traits>& os, Edge const& v)
{
    os.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

int main(int argc, char *argv[])
{
    aff_t th = atof(argv[1]);

    std::cout << "agglomerate" << std::endl;
#ifdef MST_EDGE
    agglomerate<mst_edge, mst_edge_greater, mst_edge_plus,
                           mst_edge_limits>(argv[2], argv[3], argv[4], th);
#else
    agglomerate<mean_edge, mean_edge_greater, mean_edge_plus,
                           mean_edge_limits>(argv[2], argv[3], argv[4], th);
#endif

}
