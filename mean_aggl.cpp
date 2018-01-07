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

#include <boost/heap/binomial_heap.hpp>
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

using seg_t = uint64_t;
using aff_t = float;

template <class T>
struct edge_t
{
    seg_t v0, v1;
    T        w;
};

template <class T>
using region_graph = std::vector<edge_t<T>>;

template <class T, class C = std::greater<T>>
struct heapable_edge;

template <class T, class C = std::greater<T>>
struct heapable_edge_compare
{
    bool operator()(heapable_edge<T, C>* const a,
                    heapable_edge<T, C>* const b) const
    {
        C c;
        return c(b->edge.w, a->edge.w);
    }
};

template <class T, class C = std::greater<T>>
using heap_type = boost::heap::binomial_heap<
    heapable_edge<T, C>*, boost::heap::compare<heapable_edge_compare<T, C>>>;

template <class T, class C>
struct heapable_edge
{
    edge_t<T> edge;
    typename heap_type<T, C>::handle_type handle;
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
    }

    container & operator[](key k) {
        if (d_pmap.count(k) == 0) {
            d_pmap.emplace(k, new container);
        }
        return *(d_pmap.at(k));
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
int n_intersection(const std::unordered_set<id> & set1, const std::unordered_set<id> & set2)
{
    //Assuming set1 has smaller number of element, too lazy to add a helper function
    if (set1.size() == 0 || set2.size() == 0) {
        return 0;
    }
    int count = 0;
    for (const auto & e : set1) {
        if (set2.count(e) > 0) {
            count += 1;
        }
    }
    return count;
}

template <class T, class Compare = std::greater<T>, class Plus = std::plus<T>,
          class Limits = std::numeric_limits<T>>
inline void agglomerate(std::vector<edge_t<T>> const& rg, std::unordered_set<seg_t> & frozen_supervoxels,
                                         T const& threshold)
{
    Compare comp;
    Plus    plus;
    heap_type<T, Compare> heap;

    incident_matrix<seg_t, std::map<seg_t, heapable_edge<T, Compare>* > > incident;

    size_t rg_size = rg.size();
    size_t mst_size = 0;
    size_t residue_size = 0;

    std::vector<heapable_edge<T, Compare>> edges(rg.size());
    for (std::size_t i = 0; i < rg.size(); ++i)
    {
        edges[i].edge                = rg[i];
        edges[i].handle              = heap.push(&edges[i]);
        incident[rg[i].v0][rg[i].v1] = &edges[i];
        incident[rg[i].v1][rg[i].v0] = &edges[i];
    }

    std::ofstream of_mst;
    of_mst.open("mst.data", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_remap;
    of_remap.open("remap.data", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_res;
    of_res.open("residual_rg.data", std::ofstream::out | std::ofstream::trunc);

    while (heap.size() && comp(heap.top()->edge.w, threshold))
    {
        auto e = heap.top();
        heap.pop();

        auto v0 = e->edge.v0;
        auto v1 = e->edge.v1;

        if (v0 != v1)
        {

            {
                auto s = v0;
                if (frozen_supervoxels.count(v0) > 0 && frozen_supervoxels.count(v1) > 0) {
                    of_res.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
                    of_res.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
                    write_edge(of_res, e->edge.w);
                    residue_size++;
                    continue;
                }

                if (frozen_supervoxels.count(v0) > 0) {
                    s = v0;
                    if (n_intersection(incident.neighbors(v1), frozen_supervoxels) > 1) {
                        frozen_supervoxels.insert(v1);
                        of_res.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
                        of_res.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
                        write_edge(of_res, e->edge.w);
                        residue_size++;
                        continue;
                    }
                }

                if (frozen_supervoxels.count(v1) > 0) {
                    s = v1;
                    if (n_intersection(incident.neighbors(v0), frozen_supervoxels) > 1) {
                        frozen_supervoxels.insert(v0);
                        of_res.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
                        of_res.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
                        write_edge(of_res, e->edge.w);
                        residue_size++;
                        continue;
                    }
                }

                // std::cout << "Joined " << s0 << " and " << s1 << " to " << s
                //           << " at " << e->edge.w << "\n";
                if (v0 != v1) {
                    of_mst.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
                    of_mst.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
                    of_mst.write(reinterpret_cast<const char *>(&(s)), sizeof(seg_t));
                    mst_size++;
                    write_edge(of_mst, e->edge.w);
                    if (v0 == s) {
                        of_remap.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
                        of_remap.write(reinterpret_cast<const char *>(&(s)), sizeof(seg_t));
                    } else if (v1 == s) {
                        of_remap.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
                        of_remap.write(reinterpret_cast<const char *>(&(s)), sizeof(seg_t));
                    } else {
                        std::cout << "Something is wrong in the MST" << std::endl;
                        std::cout << "s: " << s << ", v0: " << v0 << ", v1: " << v1 << std::endl;
                    }
                }
                if (s == v0)
                {
                    std::swap(v0, v1);
                }
            }

            // v0 is dissapearing from the graph

            // earase the edge e = {v0,v1}
            incident[v0].erase(v1);
            incident[v1].erase(v0);

            // loop over other edges e0 = {v0,v}
            for (auto& e0 : incident[v0])
            {
                if (e0.first == v0)
                    std::cout << "loop in the incident matrix: " << e0.first << std::endl;

                incident[e0.first].erase(v0);
                if (incident[v1].count(e0.first)) // {v0,v} and {v1,v} exist, we
                                                  // need to merge them
                {
                    auto& e1   = incident[v1][e0.first]; // edge {v1,v}
                    e1->edge.w = plus(e1->edge.w, e0.second->edge.w);
                    heap.update(e1->handle);
                    {
                        // std::cout
                        //     << "Removing: " <<
                        //     sets.find_set(e0.second->edge.v0)
                        //     << " to " << sets.find_set(e0.second->edge.v1)
                        //     << " of " << e0.second->edge.w << " ";
                        e0.second->edge.w = Limits::max();
                        heap.increase(e0.second->handle);
                        heap.pop();
                    }
                }
                else
                {
                    if (e0.second->edge.v0 == v0)
                        e0.second->edge.v0 = v1;
                    if (e0.second->edge.v1 == v0)
                        e0.second->edge.v1 = v1;
                    incident[e0.first][v1] = e0.second;
                    incident[v1][e0.first] = e0.second;
                }
                incident[v0].erase(e0.first);
            }
        }
    }

    of_mst.close();

    of_remap.close();

    //std::cout << "Total of " << next << " segments\n";

    while (heap.size())
    {
        auto e = heap.top();
        heap.pop();
        auto v0 = e->edge.v0;
        auto v1 = e->edge.v1;
        if (frozen_supervoxels.count(v0) > 0 || frozen_supervoxels.count(v1) > 0) {
            of_res.write(reinterpret_cast<const char *>(&(v0)), sizeof(seg_t));
            of_res.write(reinterpret_cast<const char *>(&(v1)), sizeof(seg_t));
            residue_size++;
            write_edge(of_res, e->edge.w);
        }
    }
    of_res.close();

    std::ofstream of_meta;
    of_meta.open("meta.data", std::ofstream::out | std::ofstream::trunc);
    size_t dummy = 0;
    of_meta.write(reinterpret_cast<const char *>(&(dummy)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(dummy)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(dummy)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(rg_size)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(residue_size)), sizeof(size_t));
    of_meta.write(reinterpret_cast<const char *>(&(mst_size)), sizeof(size_t));
    of_meta.close();

    return;
}

typedef struct atomic_edge
{
    seg_t u1;
    seg_t u2;
    double sum_aff;
    double area;
    explicit constexpr atomic_edge(seg_t w1 = 0, seg_t w2 = 0, double s_a = 0.0, double a = 0)
        : u1(w1)
        , u2(w2)
        , sum_aff(s_a)
        , area(a)
    {
    }
} atomic_edge_t;

struct mean_edge
{
    double sum;
    double num;
    atomic_edge_t * repr;

    explicit constexpr mean_edge(double s = 0, double n = 1, atomic_edge_t * r = NULL)
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
        atomic_edge_t * new_repr = NULL;
        if (a.repr->area > b.repr->area)
            new_repr = a.repr;
        else
            new_repr = b.repr;
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
        return mean_edge(std::numeric_limits<double>::max(), 1, NULL);
    }
};

template <class CharT, class Traits>
::std::basic_ostream<CharT, Traits>&
operator<<(::std::basic_ostream<CharT, Traits>& os, mean_edge const& v)
{
    os << v.sum << " " << v.num << " " << v.repr->u1 << " " <<  v.repr->u2 << " " << v.repr->sum_aff << " " << v.repr->area;
    return os;
}

template <class CharT, class Traits>
void write_edge(::std::basic_ostream<CharT, Traits>& os, mean_edge const& v)
{
    aff_t aff = v.sum;
    aff_t aff_rw = v.repr->sum_aff;
    size_t num = v.num;
    size_t area = v.repr->area;

    os.write(reinterpret_cast<const char *>(&(aff)), sizeof(aff_t));
    os.write(reinterpret_cast<const char *>(&(num)), sizeof(size_t));
    os.write(reinterpret_cast<const char *>(&(v.repr->u1)), sizeof(seg_t));
    os.write(reinterpret_cast<const char *>(&(v.repr->u2)), sizeof(seg_t));
    os.write(reinterpret_cast<const char *>(&(aff_rw)), sizeof(aff_t));
    os.write(reinterpret_cast<const char *>(&(area)), sizeof(size_t));
}

int main(int argc, char *argv[])
{
    std::vector<edge_t<mean_edge>> rg;
    std::unordered_set<seg_t> frozen_supervoxels;
    double th = atof(argv[1]);
    std::ifstream rg_file(argv[2]);
    if (!rg_file.is_open()) {
        std::cout << "Cannot open the region graph file" << std::endl;
        return -1;
    }
    std::ifstream frozen_file(argv[3]);
    if (!frozen_file.is_open()) {
        std::cout << "Cannot open the frozen supervoxel file" << std::endl;
        return -1;
    }

    seg_t v1, v2;
    aff_t aff;
    size_t area;

    while (rg_file.read(reinterpret_cast<char *>(&v1), sizeof(v1)))
    {
        rg_file.read(reinterpret_cast<char *>(&v2), sizeof(v2));
        rg_file.read(reinterpret_cast<char *>(&(aff)), sizeof(aff));
        rg_file.read(reinterpret_cast<char *>(&(area)), sizeof(area));
        edge_t<mean_edge> e;
        e.v0 = v1;
        e.v1 = v2;
        e.w.sum = aff;
        e.w.num = area;
        rg_file.read(reinterpret_cast<char *>(&v1), sizeof(v1));
        rg_file.read(reinterpret_cast<char *>(&v2), sizeof(v2));
        rg_file.read(reinterpret_cast<char *>(&(aff)), sizeof(aff));
        rg_file.read(reinterpret_cast<char *>(&(area)), sizeof(area));
        atomic_edge_t * ae = new atomic_edge_t(v1,v2,aff,area);
        e.w.repr = ae;
        //std::cout << e.v0 <<" " << e.v1 <<" " << e.w.sum <<" " << e.w.num<<" "  << u1<<" "  << u2<<" "  << sum_aff<<" "  << area << std::endl;
        rg.push_back(e);
    }

    seg_t sv;
    while (frozen_file.read(reinterpret_cast<char *>(&sv), sizeof(sv))) {
        frozen_supervoxels.insert(sv);
    }

    rg_file.close();
    frozen_file.close();
    agglomerate<mean_edge, mean_edge_greater, mean_edge_plus,
                           mean_edge_limits>(rg, frozen_supervoxels, mean_edge(th, 1));

    //for (auto& e : res)
    //{
    //    std::cout << e << "\n";
    //}
}
