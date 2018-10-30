#ifndef AFFINITY_EXTRACTOR_HPP
#define AFFINITY_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

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

    void output(const std::unordered_set<Ts> & incompleteSegments, const std::unordered_map<Ts, Ts> & chunkMap, const std::string & completeFileName, const std::string & incompleteFileName)
    {
        std::vector<SegPair<Ts> > incomplete_segpairs;
        std::vector<SegPair<Ts> > complete_segpairs;

        std::ofstream complete(completeFileName, std::ios_base::binary);
        std::ofstream incomplete(incompleteFileName, std::ios_base::binary);

        std::vector<std::pair<Ta, Ts> > me_complete;
        std::vector<std::pair<Ta, Ts> > me_incomplete;
        std::unordered_map<SegPair<Ts>, std::pair<Ta, size_t>, boost::hash<SegPair<Ts> > > real_edges;
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
            writeEdge(complete, p, me_complete[i]);
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
    std::unordered_map<SegPair<Ts>, std::pair<Ta, size_t>, boost::hash<SegPair<Ts> > > m_edges;
};

template <typename Ts, typename Te>
using RegionGraph = std::unordered_map<SegPair<Ts>, Te, boost::hash<SegPair<Ts> > >;

template<typename Ta>
struct SimpleEdge{
    Ta affinity;
    size_t area;
    explicit constexpr SimpleEdge(Ta a = 0.0, size_t b = 0)
        :affinity(a),area(b) {};
};

template<typename Ts, typename Ta>
RegionGraph<Ts, SimpleEdge<Ta> > loadRegionGraph(const std::string & fileName)
{
    RegionGraph<Ts, SimpleEdge<Ta> > rg;
    std::cout << "loading: " << fileName << std::endl;
    std::ifstream in(fileName);
    Ts s1, s2;
    size_t area;
    Ta aff;
    while (in.read(reinterpret_cast<char *>(&s1), sizeof(s1))) {
        in.read(reinterpret_cast<char *>(&s2), sizeof(s2));
        in.read(reinterpret_cast<char *>(&aff), sizeof(aff));
        in.read(reinterpret_cast<char *>(&area), sizeof(area));
        auto & e = rg[std::minmax(s1,s2)];
        e.affinity +=  aff;
        e.area += area;
    }
    assert(!in.bad());
    in.close();
    return rg;
}

template <typename Ts, typename Ta>
void writeRegionGraph(const std::unordered_set<Ts> & incompleteSegments, const RegionGraph<Ts, SimpleEdge<Ta> >  rg, const std::string & incompleteFileName, const std::string & completeFileName)
{
    std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
    std::ofstream complete(completeFileName, std::ios_base::binary);

    for (const auto &[k, v] : rg) {
        if (incompleteSegments.count(k.first) > 0 && incompleteSegments.count(k.second) > 0) {
            incomplete.write(reinterpret_cast<const char *>(&(k.first)), sizeof(seg_t));
            incomplete.write(reinterpret_cast<const char *>(&(k.second)), sizeof(seg_t));
            incomplete.write(reinterpret_cast<const char *>(&(v.affinity)), sizeof(aff_t));
            incomplete.write(reinterpret_cast<const char *>(&(v.area)), sizeof(size_t));
        } else {
            complete.write(reinterpret_cast<const char *>(&(k.first)), sizeof(seg_t));
            complete.write(reinterpret_cast<const char *>(&(k.second)), sizeof(seg_t));
            complete.write(reinterpret_cast<const char *>(&(v.affinity)), sizeof(aff_t));
            complete.write(reinterpret_cast<const char *>(&(v.area)), sizeof(size_t));
            complete.write(reinterpret_cast<const char *>(&(k.first)), sizeof(seg_t));
            complete.write(reinterpret_cast<const char *>(&(k.second)), sizeof(seg_t));
            complete.write(reinterpret_cast<const char *>(&(v.affinity)), sizeof(aff_t));
            complete.write(reinterpret_cast<const char *>(&(v.area)), sizeof(size_t));
        }
    }
    assert(!incomplete.bad());
    assert(!complete.bad());
    incomplete.close();
    complete.close();
}

template <typename Ts, typename Ta>
void updateRegionGraph(const std::unordered_set<Ts> & incompleteSegments, const std::string & inputFileName, const std::string & incompleteFileName, const std::string & completeFileName)
{
    writeRegionGraph<Ts, Ta>(incompleteSegments, loadRegionGraph<Ts, Ta>(inputFileName), incompleteFileName, completeFileName);
}

#endif
