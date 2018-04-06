#ifndef AFFINITY_EXTRACTOR_HPP
#define AFFINITY_EXTRACTOR_HPP

#include "Types.h"
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

    void output(const std::unordered_set<Ts> & incompleteSegments, const std::string & completeFileName, const std::string & incompleteFileName)
    {
        std::vector<SegPair<Ts> > incomplete_segpairs;
        std::vector<SegPair<Ts> > complete_segpairs;

        std::ofstream complete(completeFileName, std::ios_base::binary);
        std::ofstream incomplete(incompleteFileName, std::ios_base::binary);

        std::vector<std::pair<Ta, Ts> > me_complete;
        std::vector<std::pair<Ta, Ts> > me_incomplete;

        for (const auto & kv : m_edges) {
            const auto k = kv.first;
            if (incompleteSegments.count(k.first) > 0 && incompleteSegments.count(k.second) > 0) {
                incomplete_segpairs.push_back(k);
                me_incomplete.push_back(kv.second);
            } else {
                complete_segpairs.push_back(k);
                me_complete.push_back(kv.second);
            }
        }

        for (size_t i = 0; i != complete_segpairs.size(); i++) {
            const auto & p = complete_segpairs[i];
            complete.write(reinterpret_cast<const char *>(&(p.first)), sizeof(Ts));
            complete.write(reinterpret_cast<const char *>(&(p.second)), sizeof(Ts));
            complete.write(reinterpret_cast<const char *>(&(me_complete[i].first)), sizeof(Ta));
            complete.write(reinterpret_cast<const char *>(&(me_complete[i].second)), sizeof(size_t));
            complete.write(reinterpret_cast<const char *>(&(p.first)), sizeof(Ts));
            complete.write(reinterpret_cast<const char *>(&(p.second)), sizeof(Ts));
            complete.write(reinterpret_cast<const char *>(&(me_complete[i].first)), sizeof(Ta));
            complete.write(reinterpret_cast<const char *>(&(me_complete[i].second)), sizeof(size_t));
        }
        for (size_t i = 0; i != incomplete_segpairs.size(); i++) {
            const auto & p = incomplete_segpairs[i];
            incomplete.write(reinterpret_cast<const char *>(&(p.first)), sizeof(Ts));
            incomplete.write(reinterpret_cast<const char *>(&(p.second)), sizeof(Ts));
            incomplete.write(reinterpret_cast<const char *>(&(me_incomplete[i].first)), sizeof(Ta));
            incomplete.write(reinterpret_cast<const char *>(&(me_incomplete[i].second)), sizeof(size_t));
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

#endif
