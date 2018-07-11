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

using seg_t = uint64_t;
using aff_t = float;

size_t filesize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

void print_neighbors(auto neighbors)
{
    for (auto & e : neighbors) {
        std::cout << e.first << " ";
    }
    std::cout << std::endl;
}

auto search_neighbors(const auto & neighbors, const auto & value)
{
    return std::lower_bound(std::begin(neighbors), std::end(neighbors), value, [](auto & a, auto & b) { return a.first < b; });
}

bool erase_neighbors(auto & neighbors, auto target)
{
    auto it = search_neighbors(neighbors, target);
    if (it != std::end(neighbors)) {
        if ((*it).first == target) {
            neighbors.erase(it);
            return true;
        } else {
            std::cerr << "VALUE " << target << " NOT FOUND" << std::endl;
            return false;
        }
    }
    std::cerr << "VALUE " << target << " NOT FOUND" << std::endl;
    return false;
}

void insert_neighbor(auto & neighbors, auto target, auto new_value)
{
    auto it = search_neighbors(neighbors, target);
    if (it != std::end(neighbors)) {
        if ((*it).first == target) {
            std::cerr << "Should not happen: " << target << " is already a neighbors" << std::endl;
            it = neighbors.erase(it);
        }
        neighbors.insert(it, std::pair(target, new_value));
    } else {
        neighbors.push_back(std::pair(target, new_value));
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
    bool operator()(heapable_edge<T, C> const a,
                    heapable_edge<T, C> const b) const
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
struct heapable_edge
{
    edge_t<T> & edge;
    heap_handle_type<T, C> handle;
    explicit constexpr heapable_edge(edge_t<T> & e)
        : edge(e) {};
};

template <typename key, typename container>
class incident_matrix
{
public:
    incident_matrix()
        :d_pmap() {
    }

    ~incident_matrix() {
        for(auto & p : d_pmap) {
            delete p.second;
            p.second = NULL;
        }
        d_pmap.clear();
    }

    container & operator[](const key & k) {
        if (d_pmap.count(k) == 0) {
            d_pmap.emplace(k, new container);
        }
        return *(d_pmap.at(k));
    }

    void remove(const key & k) {
        if (d_pmap.count(k) == 0) {
            return;
        }
        delete d_pmap.at(k);
        d_pmap[k] = NULL;
        d_pmap.erase(k);
    }

    std::unordered_set<key> neighbors(key k) {
        std::unordered_set<key> nbs;
        if (d_pmap.count(k) > 0) {
            const auto & c = *(d_pmap.at(k));
            nbs.reserve(c.size());
            for (const auto & kv : c) {
                nbs.insert(kv.first);
            }
        }
        return nbs;
    }

private:
    std::unordered_map<key, container * > d_pmap;
};

template <typename id>
int n_intersection_impl(const std::unordered_set<id> & set1, const std::unordered_set<id> & set2)
{
    int count = 0;
    for (const auto & e : set1) {
        if (set2.count(e) > 0) {
            count += 1;
        }
    }
    return count;
}

template <typename id>
int n_intersection(const std::unordered_set<id> & set1, const std::unordered_set<id> & set2)
{
    if (set1.size() == 0 || set2.size() == 0) {
        return 0;
    }
    if (set2.size() > set1.size()) {
        return n_intersection_impl(set1, set2);
    } else {
        return n_intersection_impl(set2, set1);
    }

}

template <class T, class Compare = std::greater<T> >
struct agglomeration_data_t
{
    incident_matrix<seg_t, std::unordered_map<seg_t, heap_handle_type<T, Compare> > > incident;
    heap_type<T, Compare> heap;
    std::unordered_set<seg_t> frozen_supervoxels;
    std::unordered_map<seg_t, size_t> supervoxel_counts;
    std::vector<edge_t<T> > rg_vector;
};

template <class T, class Compare = std::greater<T> >
inline agglomeration_data_t<T, Compare> load_inputs(const char * rg_filename, const char * fs_filename, const char * ns_filename)
{
    agglomeration_data_t<T, Compare> agg_data;
    auto & incident = agg_data.incident;
    auto & heap = agg_data.heap;
    auto & frozen_supervoxels = agg_data.frozen_supervoxels;
    auto & supervoxel_counts = agg_data.supervoxel_counts;
    auto & rg_vector = agg_data.rg_vector;

    std::ifstream ns_file(ns_filename);
    if (!ns_file.is_open()) {
        std::cout << "Cannot open the supervoxel count file" << std::endl;
        std::abort();
    }

    seg_t seg;
    size_t count;
    while (ns_file.read(reinterpret_cast<char *>(&seg), sizeof(seg))) {
        ns_file.read(reinterpret_cast<char *>(&count), sizeof(count));
        if (count == 0) {
            std::cout << "SHOULD NOT HAPPEN, 0 supervoxel for : " << seg << std::endl;
            std::abort();
        }
        if (supervoxel_counts.count(seg) > 0) {
            if (supervoxel_counts.at(seg) != 1 || count != 1) {
                std::cout << "multiple entries for " << seg << " " << supervoxel_counts.at(seg) << ", " << count << std::endl;
                std::abort();
            }
            supervoxel_counts[seg] += (count - 1);
        } else {
            supervoxel_counts[seg] = count;
        }
    }

    std::cout << "filesize:" << filesize(rg_filename)/sizeof(edge_t<T>) << std::endl;
    size_t rg_size = filesize(rg_filename);
    if (rg_size % sizeof(edge_t<T>) != 0) {
        std::cerr << "Region graph file incomplete!" << std::endl;
        std::abort();
    }
    size_t rg_entries = rg_size / sizeof(edge_t<T>);
    //size_t rg_entries = 100000000;


    FILE* f = std::fopen(rg_filename, "rbXS");
    if ( !f ) {
        std::cerr << "Cannot open the region graph file" << std::endl;
        std::abort();
    }

    rg_vector.resize(rg_entries);
    std::size_t nread = std::fread(rg_vector.data(), sizeof(edge_t<T>), rg_entries, f);
    if (nread != rg_entries) {
        std::cerr << "Reading: " << nread << " entries, but expecting: " << rg_entries << std::endl;
        std::abort();
    }
    std::fclose(f);

    for (auto & e : rg_vector) {
        auto v0 = e.v0;
        auto v1 = e.v1;
        if (v0 > v1) {
            e.v0 = v1;
            e.v1 = v0;
        }
    }

    std::sort(std::begin(rg_vector), std::end(rg_vector), [](auto & a, auto & b) { return (a.v0 < b.v0) || (a.v0 == b.v0 && a.v1 < b.v1);  });

    std::cout << "reading rg:" << sizeof(edge_t<T>) << " " << sizeof(heapable_edge<T, Compare>)<< std::endl;
    size_t i = 0;
    edge_t<T> e;
    for (auto & e : rg_vector)
    {
        heapable_edge<T, Compare> he(e);
        //he.edge = e;
        auto handle = heap.push(he);
        (*handle).handle = handle;
        incident[e.v0][e.v1] = handle;
        incident[e.v1][e.v0] = handle;
        if (supervoxel_counts.count(e.v0) == 0) {
            supervoxel_counts[e.v0] = 1;
        }
        if (supervoxel_counts.count(e.v1) == 0) {
            supervoxel_counts[e.v1] = 1;
        }
        i++;
        if (i % 10000000 == 0) {
            std::cout << "reading " << i << "th edge" << std::endl;
        }
    }

    std::ifstream fs_file(fs_filename);
    if (!fs_file.is_open()) {
        std::cout << "Cannot open the frozen supervoxel file" << std::endl;
        std::abort();
    }

    std::cout << "reading frozen sv" << std::endl;
    seg_t sv;
    while (fs_file.read(reinterpret_cast<char *>(&sv), sizeof(sv))) {
        frozen_supervoxels.insert(sv);
        if (supervoxel_counts.count(sv) == 0) {
            supervoxel_counts[sv] = 1;
        }
    }

    fs_file.close();
    ns_file.close();

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
    auto & frozen_supervoxels = agg_data.frozen_supervoxels;
    auto & supervoxel_counts =  agg_data.supervoxel_counts;

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
        heap.pop();

        auto v0 = e.edge.v0;
        auto v1 = e.edge.v1;
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

            // earase the edge e = {v0,v1}
            incident[v0].erase(v1);
            incident[v1].erase(v0);

            if (frozen_supervoxels.count(v0) > 0 || frozen_supervoxels.count(v1) > 0) {
                frozen_supervoxels.insert(v0);
                frozen_supervoxels.insert(v1);
                of_res.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
                of_res.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
                write_edge(of_res, e.edge.w);
                residue_size++;
                continue;
            }

            //std::cout << "Test " << v0 << " and " << v1 << std::endl;
                       //<< " at " << e->edge.w << "\n";
            if (e.edge.w.sum/e.edge.w.num < 0.5) {
                auto p = std::minmax(supervoxel_counts.at(v0), supervoxel_counts.at(v1));
                if (p.first > 1000 and p.second > 10000) {
                    std::cout << "reject edge between " << v0 << "(" << supervoxel_counts.at(v0) << ")"<< " and " << v1 << "(" << supervoxel_counts.at(v1) << ")"<< std::endl;
                    of_reject.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
                    of_reject.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
                    write_edge(of_reject, e.edge.w);
                    continue;
                }
            }
            //std::cout << "Join " << v0 << " and " << v1 << std::endl;
            auto total = supervoxel_counts.at(v0) + supervoxel_counts.at(v1);
            supervoxel_counts[v0] = 0;
            supervoxel_counts[v1] = 0;
            supervoxel_counts[s] = total;
            of_mst.write(reinterpret_cast<const char *>(&(s)), sizeof(seg_t));
            of_mst.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
            of_mst.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
            mst_size++;
            write_edge(of_mst, e.edge.w);
            if (v0 == s) {
                of_remap.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
                of_remap.write(reinterpret_cast<const char *>(&(s)), sizeof(seg_t));
            } else if (v1 == s) {
                of_remap.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
                of_remap.write(reinterpret_cast<const char *>(&(s)), sizeof(seg_t));
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
            for (auto& e0 : incident[v0])
            {
                if (e0.first == v0)
                    std::cout << "loop in the incident matrix: " << e0.first << std::endl;

                incident[e0.first].erase(v0);
                if (incident[v1].count(e0.first)) // {v0,v} and {v1,v} exist, we
                                                  // need to merge them
                {
                    auto& e1   = (*(incident[v1][e0.first])); // edge {v1,v}
                    e1.edge.w = plus(e1.edge.w, (*(e0.second)).edge.w);
                    heap.update(e1.handle);
                    heap.erase(e0.second);
                    {
                        //std::cout
                        //     << "Removing: " <<
                        //     (*(e0.second)).edge.v0
                        //     << " to " <<
                        //     (*(e0.second)).edge.v1
                        //     << " of " << (*(e0.second)).edge.w << std::endl;
                    }
                }
                else
                {
                    auto & e = (*(e0.second));
                    if (e.edge.v0 == v0)
                        e.edge.v0 = v1;
                    if (e.edge.v1 == v0)
                        e.edge.v1 = v1;
                    incident[e0.first][v1] = e0.second;
                    incident[v1][e0.first] = e0.second;
                }
            }
            incident.remove(v0);
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

    while (!heap.empty())
    {
        auto e = heap.top();
        heap.pop();
        auto v0 = e.edge.v0;
        auto v1 = e.edge.v1;
        if (frozen_supervoxels.count(v0) > 0 && frozen_supervoxels.count(v1) > 0) {
            of_res.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
            of_res.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
            residue_size++;
            write_edge(of_res, e.edge.w);
        } else {
            of_frg.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
            of_frg.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
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

    std::ofstream of_fs;
    of_fs.open("ongoing_segments.data", std::ofstream::out | std::ofstream::trunc);

    for (const auto & s : frozen_supervoxels) {
        of_fs.write(reinterpret_cast<const char *>(&(s)), sizeof(seg_t));
        if (supervoxel_counts.at(s) == 0) {
            std::cout << "SHOULD NOT HAPPEN, frozen supervoxel agglomerated: " << s << std::endl;
            std::abort();
        }
        of_fs.write(reinterpret_cast<const char *>(&(supervoxel_counts.at(s))), sizeof(size_t));
    }
    of_fs.close();

    of_fs.open("done_segments.data", std::ofstream::out | std::ofstream::trunc);

    for (const auto & [k, v] : supervoxel_counts) {
        if (v == 0) {
            continue;
        }
        if (frozen_supervoxels.count(k) == 0) {
            of_fs.write(reinterpret_cast<const char *>(&(k)), sizeof(seg_t));
            of_fs.write(reinterpret_cast<const char *>(&(v)), sizeof(size_t));
        }
    }

    of_fs.close();

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
    aff_t aff = v.sum;
    aff_t aff_rw = v.repr.sum_aff;
    size_t num = v.num;
    size_t area = v.repr.area;

    os.write(reinterpret_cast<const char *>(&(aff)), sizeof(aff_t));
    os.write(reinterpret_cast<const char *>(&(num)), sizeof(size_t));
    os.write(reinterpret_cast<const char *>(&(v.repr.u1)), sizeof(seg_t));
    os.write(reinterpret_cast<const char *>(&(v.repr.u2)), sizeof(seg_t));
    os.write(reinterpret_cast<const char *>(&(aff_rw)), sizeof(aff_t));
    os.write(reinterpret_cast<const char *>(&(area)), sizeof(size_t));
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
