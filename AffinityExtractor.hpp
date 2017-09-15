#ifndef AFFINITY_EXTRACTOR_HPP
#define AFFINITY_EXTRACTOR_HPP

#include "Types.h"
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

template<typename Ts, typename Ta, typename Chunk>
class AffinityExtractor
{
public:
    AffinityExtractor(const Chunk & aff)
        :m_aff(aff), m_boundarySupervoxels(), m_edges() {}

    void collectVoxel(Coord & c, Ts segid) {}

    void collectBoundary(int face, Coord & c, Ts segid)
    {
        m_boundarySupervoxels[face].insert(segid);
    }

    void collectContactingSurface(int nv, Coord & c, Ts segid1, Ts segid2)
    {
        auto p = std::minmax(segid1, segid2);
        m_edges[p][nv][c] = m_aff[c[0]][c[1]][c[2]][nv];
    }

    const std::unordered_set<Ts> & boundarySupervoxels(int face) const {return m_boundarySupervoxels[face];}
    const std::unordered_map<SegPair<Ts>, Edge<Ta>, boost::hash<SegPair<Ts> > > & edges() const {return m_edges;}
private:
    const Chunk & m_aff;
    std::array<std::unordered_set<Ts>, 6> m_boundarySupervoxels;
    std::unordered_map<SegPair<Ts>, Edge<Ta>, boost::hash<SegPair<Ts> > > m_edges;
};


#endif
