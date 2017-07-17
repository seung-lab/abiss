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
#include <map>
#include <vector>
#include <unordered_set>
#include <iomanip>
#include <boost/pending/disjoint_sets.hpp>

template <class T>
struct edge_t
{
    uint64_t v0, v1;
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

template <class T, class Compare = std::greater<T>, class Plus = std::plus<T>,
          class Limits = std::numeric_limits<T>>
inline void agglomerate(std::vector<edge_t<T>> const& rg, std::unordered_set<uint64_t> & frozen_supervoxels,
                                         T const& threshold, uint64_t const n)
{
    Compare comp;
    Plus    plus;
    heap_type<T, Compare> heap;

    std::vector<uint64_t> rank(n);
    std::vector<uint64_t> parent(n);

    boost::disjoint_sets<uint64_t*, uint64_t*> sets(&rank[0], &parent[0]);
    for (int i =0; i<n; i++) {
        sets.make_set(i);
    }

    std::vector<std::map<uint64_t, heapable_edge<T, Compare>*>> incident(n);

    std::vector<heapable_edge<T, Compare>> edges(rg.size());
    for (std::size_t i = 0; i < rg.size(); ++i)
    {
        edges[i].edge                = rg[i];
        edges[i].handle              = heap.push(&edges[i]);
        incident[rg[i].v0][rg[i].v1] = &edges[i];
        incident[rg[i].v1][rg[i].v0] = &edges[i];
    }

    std::ofstream of_mst;
    of_mst.open("test_mst.in", std::ofstream::out | std::ofstream::trunc);

    std::ofstream of_frozen_edges;
    of_frozen_edges.open("frozen_rg.in", std::ofstream::out | std::ofstream::trunc);

    while (heap.size() && comp(heap.top()->edge.w, threshold))
    {
        auto e = heap.top();
        heap.pop();

        auto v0 = e->edge.v0;
        auto v1 = e->edge.v1;

        if (v0 != v1)
        {

            {
                auto s0 = sets.find_set(v0);
                auto s1 = sets.find_set(v1);
                if (frozen_supervoxels.count(s0) > 0) {
                    frozen_supervoxels.insert(s1);
                    of_frozen_edges << std::setprecision (17) << s0 << " " << s1 << " " << e->edge.w << std::endl;
                    continue;
                }

                if (frozen_supervoxels.count(s1) > 0) {
                    frozen_supervoxels.insert(s0);
                    of_frozen_edges << std::setprecision (17) << s0 << " " << s1 << " " << e->edge.w << std::endl;
                    continue;
                }

                sets.link(s0, s1);
                auto s = sets.find_set(v0);

                // std::cout << "Joined " << s0 << " and " << s1 << " to " << s
                //           << " at " << e->edge.w << "\n";
                if (s0 != s1) {
                    of_mst << std::setprecision (17) << s0 << " " << s1 << " " << s << " " << e->edge.w << std::endl;
                }
            }

            if (incident[v0].size() > incident[v1].size())
            {
                std::swap(v0, v1);
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
    of_frozen_edges.close();

    //std::cout << "Total of " << next << " segments\n";

    std::ofstream of_rg;
    of_rg.open("new_rg.in", std::ofstream::out | std::ofstream::trunc);
    of_rg << n-1 << " " << n << " " << heap.size() << std::endl;
    while (heap.size())
    {
        auto e = heap.top();
        heap.pop();
        auto v0 = e->edge.v0;
        auto v1 = e->edge.v1;
        auto s0 = sets.find_set(v0);
        auto s1 = sets.find_set(v1);
        of_rg << std::setprecision (17) << s0 << " " << s1 << " " << e->edge.w << std::endl;
    }
    of_rg.close();

    return;
}

typedef struct atomic_edge
{
    uint64_t u1;
    uint64_t u2;
    double sum_aff;
    double area;
    explicit constexpr atomic_edge(uint64_t w1 = 0, uint64_t w2 = 0, double s_a = 0.0, double a = 0)
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

int main(int argc, char *argv[])
{
    std::vector<edge_t<mean_edge>> rg;
    std::unordered_set<uint64_t> frozen_supervoxels;
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

    std::size_t m, v, n;
    rg_file >> m >> v >> n;

    for (std::size_t i = 0; i < n; ++i)
    {
        edge_t<mean_edge> e;
        uint64_t u1, u2;
        double sum_aff, area;
        rg_file >> e.v0 >> e.v1 >> e.w.sum >> e.w.num >> u1 >> u2 >> sum_aff >>  area;
        atomic_edge_t * ae = new atomic_edge_t(u1,u2,sum_aff,area);
        e.w.repr = ae;
        rg.push_back(e);
    }

    uint64_t sv;
    while (!frozen_file.eof()) {
        frozen_file >> sv;
        frozen_supervoxels.insert(sv);
    }

    rg_file.close();
    frozen_file.close();
    agglomerate<mean_edge, mean_edge_greater, mean_edge_plus,
                           mean_edge_limits>(rg, frozen_supervoxels, mean_edge(th, 1), v);

    //for (auto& e : res)
    //{
    //    std::cout << e << "\n";
    //}
}
