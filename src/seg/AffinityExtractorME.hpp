#ifndef AFFINITY_EXTRACTOR_ME_HPP
#define AFFINITY_EXTRACTOR_ME_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/format.hpp>
#include <execution>

template<typename Ts, typename Ta, typename Chunk>
class AffinityExtractorME
{
public:
    AffinityExtractorME(const Chunk & aff)
        :m_aff(aff), m_edges() {}

    void collectVoxel(Coord & c, Ts segid) {}

    void collectBoundary(int face, Coord & c, Ts segid) {}

    void collectContactingSurface(int nv, Coord & c, Ts segid1, Ts segid2)
    {
        auto p = std::minmax(segid1, segid2);
        m_edges[p].first += m_aff[c[0]][c[1]][c[2]][nv];
        m_edges[p].second += 1;
    }

    void writeEdge(std::ofstream & out, const std::pair<Ts, Ts> & k, const std::pair<Ta, size_t> & v) {
        out.write(reinterpret_cast<const char *>(&(k.first)), sizeof(Ts));
        out.write(reinterpret_cast<const char *>(&(k.second)), sizeof(Ts));
        out.write(reinterpret_cast<const char *>(&(v.first)), sizeof(Ta));
        out.write(reinterpret_cast<const char *>(&(v.second)), sizeof(size_t));
    }

    void output(const SetContainer<Ts> & incompleteSegments, const MapContainer<Ts, Ts> & chunkMap, const std::string & completeFileName, const std::string & incompleteFileName)
    {
        std::vector<SegPair<Ts> > incomplete_segpairs;
        std::vector<SegPair<Ts> > complete_segpairs;

        std::ofstream complete(completeFileName, std::ios_base::binary);
        std::ofstream incomplete(incompleteFileName, std::ios_base::binary);

        std::vector<std::pair<Ta, Ts> > me_complete;
        std::vector<std::pair<Ta, Ts> > me_incomplete;
        MapContainer<SegPair<Ts>, std::pair<Ta, size_t>, HashFunction<SegPair<Ts> > > real_edges;
        for (const auto & kv : m_edges) {
            auto k = kv.first;
            auto v = kv.second;
            if (chunkMap.count(k.first) > 0) {
                k.first = chunkMap.at(k.first);
            }

            if (chunkMap.count(k.second) > 0) {
                k.second = chunkMap.at(k.second);
            }
            if (k.first == k.second) {
                continue;
            }
            auto p = std::minmax(k.first, k.second);
            real_edges[p].first += v.first;
            real_edges[p].second += v.second;
        }

        for (const auto & [k,v] : real_edges) {

            if (incompleteSegments.count(k.first) > 0 && incompleteSegments.count(k.second) > 0) {
                incomplete_segpairs.push_back(k);
                me_incomplete.push_back(v);
            } else {
                complete_segpairs.push_back(k);
                me_complete.push_back(v);
            }
        }

        for (size_t i = 0; i != complete_segpairs.size(); i++) {
            const auto & p = complete_segpairs[i];
            writeEdge(complete, p, me_complete[i]);
#ifdef MST_EDGE
            writeEdge(complete, p, me_complete[i]);
#endif
        }
        for (size_t i = 0; i != incomplete_segpairs.size(); i++) {
            const auto & p = incomplete_segpairs[i];
            writeEdge(incomplete, p, me_incomplete[i]);
        }
        assert(!incomplete.bad());
        assert(!complete.bad());
        incomplete.close();
        complete.close();
    }

private:
    const Chunk & m_aff;
    MapContainer<SegPair<Ts>, std::pair<Ta, size_t>, HashFunction<SegPair<Ts> > > m_edges;
};

template<typename Ts, typename Ta>
struct __attribute__((packed)) SimpleEdge{
    Ts s1;
    Ts s2;
    Ta affinity;
    size_t area;
    explicit constexpr SimpleEdge(Ts v1 = 0 , Ts v2 = 0, Ta a = 0.0, size_t b = 0)
        :s1(v1), s2(v2), affinity(a),area(b) {};
};

template<typename Ts, typename Ta>
std::vector<SimpleEdge<Ts, Ta> > loadRegionGraph(const std::string & fileName)
{
    std::vector<SimpleEdge<Ts, Ta> > output_rg;
    auto input_rg = read_array<SimpleEdge<Ts, Ta> >(fileName);
    std::cout << "loading: " << fileName << std::endl;
    std::for_each(std::execution::par, input_rg.begin(), input_rg.end(), [](auto & a) {
        if (a.s1 < a.s2) {
            auto tmp = a.s1;
            a.s1 = a.s2;
            a.s2 = tmp;
        }
    });

    std::stable_sort(std::execution::par, input_rg.begin(), input_rg.end(), [](const auto & a, const auto & b) {
        return ((a.s1 < b.s1) || (a.s1 == b.s1 && a.s2 < b.s2));
    });

    SimpleEdge<Ts, Ta> current_edge;
    for(const auto & e : input_rg) {
        if (current_edge.s1 != e.s1 || current_edge.s2 != e.s2) {
            if (current_edge.s1 != Ts(0) && current_edge.s2 != Ts(0)) {
                output_rg.push_back(current_edge);
            }
            current_edge = e;
        } else {
            current_edge.affinity += e.affinity;
            current_edge.area += e.area;
        }
    }

    if (current_edge.s1 != Ts(0) && current_edge.s2 != Ts(0)) {
        output_rg.push_back(current_edge);
    }

    return output_rg;
}

template <typename Ts, typename Ta>
void writeRegionGraph(const SetContainer<Ts> & incompleteSegments, const std::vector<SimpleEdge<Ts, Ta> >  rg, const std::string & incompleteFileName, const std::string & completeFileName)
{
    std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
    std::ofstream complete(completeFileName, std::ios_base::binary);

    for (const auto &e : rg) {
        if (incompleteSegments.count(e.s1) > 0 && incompleteSegments.count(e.s2) > 0) {
            incomplete.write(reinterpret_cast<const char *>(&(e)), sizeof(e));
        } else {
            complete.write(reinterpret_cast<const char *>(&(e)), sizeof(e));
#ifdef MST_EDGE
            complete.write(reinterpret_cast<const char *>(&(e)), sizeof(e));
#endif
        }
    }
    assert(!incomplete.bad());
    assert(!complete.bad());
    incomplete.close();
    complete.close();
}

template <typename Ts, typename Ta>
void updateRegionGraph(const SetContainer<Ts> & incompleteSegments, const std::string & inputFileName, const std::string & incompleteFileName, const std::string & completeFileName)
{
    writeRegionGraph<Ts, Ta>(incompleteSegments, loadRegionGraph<Ts, Ta>(inputFileName), incompleteFileName, completeFileName);
}

#endif
