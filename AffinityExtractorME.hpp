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
        :m_aff(aff), m_boundarySupervoxels(), m_edges() {}

    void collectVoxel(Coord & c, Ts segid) {}

    void collectBoundary(int face, Coord & c, Ts segid)
    {
        m_boundarySupervoxels[face].insert(segid);
    }

    void collectContactingSurface(int nv, Coord & c, Ts segid1, Ts segid2)
    {
        auto p = std::minmax(segid1, segid2);
        m_edges[p].first += m_aff[c[0]][c[1]][c[2]][nv];
        m_edges[p].second += 1;
    }

    const std::unordered_set<Ts> & boundarySupervoxels(int face) const {return m_boundarySupervoxels[face];}
    const std::unordered_map<SegPair<Ts>, std::pair<Ta, size_t>, boost::hash<SegPair<Ts> > > & edges() const {return m_edges;}
private:
    const Chunk & m_aff;
    std::array<std::unordered_set<Ts>, 6> m_boundarySupervoxels;
    std::unordered_map<SegPair<Ts>, std::pair<Ta, size_t>, boost::hash<SegPair<Ts> > > m_edges;
};

#endif
