//
// Copyright (C) 2013-present  Aleksandar Zlateski <zlateski@mit.edu>
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
#include <sys/stat.h>
#include <parallel/algorithm>

using seg_t = uint64_t;
using aff_t = float;
static const size_t frozen = (1ul<<(std::numeric_limits<std::size_t>::digits-1));

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

auto search_neighbors(auto & neighbors, const auto source, const auto target)
{
    return std::lower_bound(std::begin(neighbors), std::end(neighbors), target, [source](auto & a, auto & b) { return a.segid(source) < b; });
}

bool frozen_neighbors(const auto & neighbors, const auto & supervoxel_counts, const auto source)
{
    for (auto & n : neighbors) {
        auto sid = n.segid(source);
        if (is_frozen(supervoxel_counts[sid])) {
            return true;
        }
    }
    return false;
}

bool erase_neighbors(auto & neighbors, const auto source, auto target)
{
    auto it = search_neighbors(neighbors, source, target);
    if (it != std::end(neighbors)) {
        if ((*it).segid(source) == target) {
            neighbors.erase(it);
            return true;
        } else {
            std::cerr << "edge "<< source << " to " << target << " NOT FOUND" << std::endl;
            print_neighbors(neighbors, source);
            return false;
        }
    }
    std::cerr << "edge " << source << " to " << target << " NOT FOUND" << std::endl;
    print_neighbors(neighbors, source);
    return false;
}

void insert_neighbor(auto & neighbors, const auto source, auto target, auto new_value)
{
    auto it = search_neighbors(neighbors, source, target);
    if (it != std::end(neighbors)) {
        if ((*it).segid(source) == target) {
            std::cerr << "Should not happen, edge: " << source << " to " << target << " is already a neighbors" << std::endl;
            //print_neighbors(neighbors, source);
            it = neighbors.erase(it);
        }
        neighbors.insert(it, new_value);
    } else {
        neighbors.push_back(new_value);
    }
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

struct __attribute__((packed)) mean_edge
{
    aff_t sum;
    size_t num;
    atomic_edge_t repr;

    explicit constexpr mean_edge(aff_t s = 0, size_t n = 1, atomic_edge_t r = atomic_edge_t() )
        : sum(s)
        , num(n)
        , repr(r)
    {
    }
};

struct mean_edge_plus
{
    mean_edge operator()(mean_edge const& a, mean_edge const& b) const
    {
        atomic_edge_t new_repr = a.repr;
        if (a.repr.area < b.repr.area) {
            new_repr = b.repr;
        }
        return mean_edge(a.sum + b.sum, a.num + b.num, new_repr);
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
        return c(b.edge.w, a.edge.w);
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
    edge_t<T> edge;
    explicit constexpr heapable_edge(edge_t<T> & e)
        : edge(e) {};
};

template <class T, class C>
struct handle_wrapper
{
    heap_handle_type<T, C> handle;

    explicit constexpr handle_wrapper(heap_handle_type<T, C> & h)
        : handle(h) {};
    seg_t segid(const seg_t exclude) const {
        return exclude == (*handle).edge.v0 ? (*handle).edge.v1 : (*handle).edge.v0;
    }
};

template <class T, class Compare = std::greater<T> >
struct agglomeration_data_t
{
    std::vector<std::vector<handle_wrapper<T, Compare> > > incident;
    heap_type<T, Compare> heap;
    std::vector<size_t> supervoxel_counts;
    std::vector<seg_t> seg_indices;
};

template <class T>
std::vector<T> read_array(const char * filename)
{
    std::vector<T> array;

    std::cout << "filesize:" << filesize(filename)/sizeof(T) << std::endl;
    size_t data_size = filesize(filename);
    if (data_size % sizeof(T) != 0) {
        std::cerr << "File incomplete!: " << filename << std::endl;
        std::abort();
    }

    FILE* f = std::fopen(filename, "rbXS");
    if ( !f ) {
        std::cerr << "Cannot open the region graph file" << std::endl;
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

template <class T, class Compare = std::greater<T> >
inline agglomeration_data_t<T, Compare> preprocess_inputs(const char * rg_filename, const char * fs_filename, const char * ns_filename)
{
    agglomeration_data_t<T, Compare> agg_data;
    using neighbor_vector = std::vector<handle_wrapper<T, Compare> >;
    auto & supervoxel_counts = agg_data.supervoxel_counts;
    auto & seg_indices = agg_data.seg_indices;

    std::vector<edge_t<T> > rg_vector = read_array<edge_t<T> >(rg_filename);

    std::vector<std::pair<seg_t, size_t> > ns_pair_array = read_array<std::pair<seg_t, size_t> >(ns_filename);
    std::vector<seg_t> fs_array = read_array<seg_t>(fs_filename);

    __gnu_parallel::transform(fs_array.begin(), fs_array.end(), std::back_inserter(ns_pair_array), [](seg_t &a){
            return std::make_pair(a, size_t(1|frozen));
            });

    __gnu_parallel::sort(std::begin(ns_pair_array), std::end(ns_pair_array), [](auto & a, auto & b) { return a.first < b.first;  });

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
            supervoxel_counts.back() += count & (~frozen) - 1;
            supervoxel_counts.back() |= count & frozen;
        }
    }

    std::unordered_map<seg_t, size_t> reverse_lookup;

    __gnu_parallel::for_each(rg_vector.begin(), rg_vector.end(), [&seg_indices](auto & a) {
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

    __gnu_parallel::sort(std::begin(rg_vector), std::end(rg_vector), [](auto & a, auto & b) { return (a.v0 < b.v0) || (a.v0 == b.v0 && a.v1 < b.v1);  });

    std::ofstream fout(rg_filename, (std::ios::out | std::ios::binary) );
    assert(fout);

    fout.write( reinterpret_cast<const char*>(rg_vector.data()), rg_vector.size() * sizeof(edge_t<T>));
    assert(!fout.bad());
    fout.close();

    return agg_data;
}

template <class T, class Compare = std::greater<T> >
inline agglomeration_data_t<T, Compare> load_inputs(const char * rg_filename, const char * fs_filename, const char * ns_filename)
{
    auto agg_data = preprocess_inputs<T, Compare>(rg_filename, fs_filename, ns_filename);
    using neighbor_vector = std::vector<handle_wrapper<T, Compare> >;
    auto & incident = agg_data.incident;
    auto & heap = agg_data.heap;
    auto & supervoxel_counts = agg_data.supervoxel_counts;
    auto & seg_indices = agg_data.seg_indices;

    std::cout << "reading rg:" << sizeof(edge_t<T>) << " " << sizeof(heapable_edge<T, Compare>)<< std::endl;
    std::ifstream rg_file(rg_filename);
    if (!rg_file.is_open()) {
        std::cout << "Cannot open the region graph file" << std::endl;
        std::abort();
    }

    size_t i = 0;
    edge_t<T> e;
    incident.resize(seg_indices.size());
    while (rg_file.read(reinterpret_cast<char *>(&e), sizeof(edge_t<T>))) {
        auto handle = heap.emplace(e);
        incident[e.v0].push_back(handle_wrapper<T, Compare>(handle));
        incident[e.v1].push_back(handle_wrapper<T, Compare>(handle));
        i++;
        if (i % 10000000 == 0) {
            std::cout << "reading " << i << "th edge" << std::endl;
        }
    }
    return agg_data;
}

template <class T, class Compare = std::greater<T>, class Plus = std::plus<T>,
          class Limits = std::numeric_limits<T>>
inline void agglomerate(const char * rg_filename, const char * fs_filename, const char * ns_filename, T const& threshold)
{
    Compare comp;
    Plus    plus;

    size_t mst_size = 0;
    size_t residue_size = 0;

    float print_th = 1.0 - 0.01;
    size_t num_of_edges = 0;

    auto agg_data = load_inputs<T, Compare>(rg_filename, fs_filename, ns_filename);
    auto & incident = agg_data.incident;
    auto & heap = agg_data.heap;
    auto & supervoxel_counts = agg_data.supervoxel_counts;
    auto & seg_indices = agg_data.seg_indices;

    size_t rg_size = heap.size();


    std::ofstream of_mst;
    of_mst.open("mst.data", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_remap;
    of_remap.open("remap.data", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_res;
    of_res.open("residual_rg.data", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_reject;
    of_reject.open("rejected_edges.log", std::ofstream::out | std::ofstream::trunc);

    std::cout << "looping through the heap" << std::endl;

    while (!heap.empty() && comp(heap.top().edge.w, threshold))
    {
        num_of_edges += 1;
        auto e = heap.top();
        auto v0 = e.edge.v0;
        auto v1 = e.edge.v1;
        //std::cout << "process edges related to: " << v0 << " and " << v1 << std::endl;
        //print_neighbors(incident[1262222], 1262222);
        erase_neighbors(incident[v0], v0, v1);
        erase_neighbors(incident[v1], v1, v0);
        heap.pop();

        if (e.edge.w.sum/e.edge.w.num < print_th) {
            std::cout << "Processing threshold: " << print_th << std::endl;
            std::cout << "Numer of edges: " << num_of_edges << "(" << rg_size << ")"<< std::endl;
            print_th -= 0.01;
        }

        if (v0 != v1)
        {

            auto s = v0;

            if (incident[v0].size() < incident[v1].size()) {
                s = v1;
            }

            //std::cout << "Test " << v0 << " and " << v1 << std::endl;
                       //<< " at " << e->edge.w << "\n";
            if (e.edge.w.sum/e.edge.w.num < 0.5) {
                auto p = std::minmax({(supervoxel_counts[v0] & (~frozen)), (supervoxel_counts[v1] & (~frozen))});
                if (p.first > 1000 and p.second > 10000) {
                    //std::cout << "reject edge between " << seg_indices[v0] << "(" << supervoxel_counts[v0] << ")"<< " and " << seg_indices[v1] << "(" << supervoxel_counts[v1] << ")"<< std::endl;
                    of_reject.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
                    of_reject.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
                    write_edge(of_reject, e.edge.w);
                    continue;
                }
            }

            if ((is_frozen(supervoxel_counts[v0]) && is_frozen(supervoxel_counts[v1]))
                || (is_frozen(supervoxel_counts[v0]) && (frozen_neighbors(incident[v1], supervoxel_counts, v1) || (e.edge.w.sum/e.edge.w.num < 0.5 && supervoxel_counts[v1] > 1000)))
                || (is_frozen(supervoxel_counts[v1]) && (frozen_neighbors(incident[v0], supervoxel_counts, v0) || (e.edge.w.sum/e.edge.w.num < 0.5 && supervoxel_counts[v0] > 1000)))) {
                supervoxel_counts[v0] |= frozen;
                supervoxel_counts[v1] |= frozen;
                of_res.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
                of_res.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
                write_edge(of_res, e.edge.w);
                residue_size++;
                continue;
            }

            if (is_frozen(supervoxel_counts[v0])) {
                s = v0;
            } else if (is_frozen(supervoxel_counts[v1])) {
                s = v1;
            }


            //std::cout << "Join " << v0 << " and " << v1 << std::endl;
            supervoxel_counts[v0] += (supervoxel_counts[v1] & (~frozen));
            supervoxel_counts[v0] |= (supervoxel_counts[v1] & frozen);
            supervoxel_counts[v1] = 0;
            std::swap(supervoxel_counts[v0], supervoxel_counts[s]);
            of_mst.write(reinterpret_cast<const char *>(&(seg_indices[s])), sizeof(seg_t));
            of_mst.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
            of_mst.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
            mst_size++;
            write_edge(of_mst, e.edge.w);
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
            std::vector<handle_wrapper<T, Compare> > new_entry;
            for (auto e0 : incident[v0])
            {
                auto v = e0.segid(v0);
                if (v == v0) {
                    std::cerr << "loop in the incident matrix: " << v << std::endl;
                    std::abort();
                }

                erase_neighbors(incident[v], v, v0);

                auto it = search_neighbors(incident[v1], v1, v);
                if (it != std::cend(incident[v1]) && (*it).segid(v1) == v)
                                                  // {v0,v} and {v1,v} exist, we
                                                  // need to merge them
                {
                    auto& e1   = *((*it).handle); // edge {v1,v}
                    e1.edge.w = plus(e1.edge.w, (*(e0.handle)).edge.w);
                    heap.update((*it).handle);
                    heap.erase(e0.handle);
                    {
                        //std::cout
                        //     << "Removing: " <<
                        //     (*(e0.handle)).edge.v0
                        //     << " to " <<
                        //     (*(e0.handle)).edge.v1
                        //     << " of " << (*(e0.handle)).edge.w << std::endl;
                    }
                }
                else
                {
                    auto & e = (*(e0.handle));
                    if (e.edge.v0 == v0)
                        e.edge.v0 = v1;
                    if (e.edge.v1 == v0)
                        e.edge.v1 = v1;
                    insert_neighbor(incident[v], v, v1, e0);
                    new_entry.push_back(e0);
                }
            }
            auto it = incident[v1].insert(incident[v1].end(), new_entry.begin(), new_entry.end());
            std::inplace_merge(incident[v1].begin(), it, incident[v1].end(), [v1](auto & a, auto & b) { return a.segid(v1) < b.segid(v1); });
            incident[v0].clear();
        }
    }

    assert(!of_mst.bad());
    assert(!of_remap.bad());

    of_mst.close();

    of_remap.close();

    //std::cout << "Total of " << next << " segments\n";
    //
    std::cout << "edges frozen above threshold: " << residue_size << std::endl;
    std::ofstream of_frg;
    of_frg.open("final_rg.data", std::ofstream::out | std::ofstream::trunc);

    for (auto it = heap.begin(); it != heap.end(); ++it)
    {
        auto e = *(heap_type<T, Compare>::s_handle_from_iterator(it));
        auto v0 = e.edge.v0;
        auto v1 = e.edge.v1;
        if (is_frozen(supervoxel_counts[v0]) && is_frozen(supervoxel_counts[v1])) {
            of_res.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
            of_res.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
            residue_size++;
            write_edge(of_res, e.edge.w);
        } else {
            of_frg.write(reinterpret_cast<const char *>(&(seg_indices[v0])), sizeof(seg_t));
            of_frg.write(reinterpret_cast<const char *>(&(seg_indices[v1])), sizeof(seg_t));
            write_edge(of_frg, e.edge.w);

        }
    }

    std::cout << "edges frozen: " << residue_size << std::endl;

    assert(!of_res.bad());
    assert(!of_frg.bad());
    assert(!of_reject.bad());
    of_res.close();
    of_frg.close();
    of_reject.close();

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

    std::ofstream of_fs_ongoing, of_fs_done;
    of_fs_ongoing.open("ongoing_segments.data", std::ofstream::out | std::ofstream::trunc);
    of_fs_done.open("done_segments.data", std::ofstream::out | std::ofstream::trunc);

    for (size_t i = 0; i < supervoxel_counts.size(); i++) {
        if (!(supervoxel_counts[i] & (~frozen))) {
            if (supervoxel_counts[i] != 0) {
                std::cout << "SHOULD NOT HAPPEN, frozen supervoxel agglomerated: " << seg_indices[i] << std::endl;
                std::abort();
            }
            continue;
        }
        if (is_frozen(supervoxel_counts[i])) {
            auto size = supervoxel_counts[i] & (~frozen);
            of_fs_ongoing.write(reinterpret_cast<const char *>(&(seg_indices[i])), sizeof(seg_t));
            of_fs_ongoing.write(reinterpret_cast<const char *>(&(size)), sizeof(size_t));
        } else {
            of_fs_done.write(reinterpret_cast<const char *>(&(seg_indices[i])), sizeof(seg_t));
            of_fs_done.write(reinterpret_cast<const char *>(&(supervoxel_counts[i])), sizeof(size_t));
        }
    }

    of_fs_ongoing.close();
    of_fs_done.close();

    return;
}

template <class CharT, class Traits>
::std::basic_ostream<CharT, Traits>&
operator<<(::std::basic_ostream<CharT, Traits>& os, mean_edge const& v)
{
    os << v.sum << " " << v.num << " " << v.repr.u1 << " " <<  v.repr.u2 << " " << v.repr.sum_aff << " " << v.repr.area;
    return os;
}

template <class CharT, class Traits>
void write_edge(::std::basic_ostream<CharT, Traits>& os, mean_edge const& v)
{
    os.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

int main(int argc, char *argv[])
{
    aff_t th = atof(argv[1]);

    std::cout << "agglomerate" << std::endl;
    agglomerate<mean_edge, mean_edge_greater, mean_edge_plus,
                           mean_edge_limits>(argv[2], argv[3], argv[4], mean_edge(th, 1));

    //for (auto& e : res)
    //{
    //    std::cout << e << "\n";
    //}
}
