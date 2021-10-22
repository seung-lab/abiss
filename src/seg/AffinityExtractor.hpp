#ifndef AFFINITY_EXTRACTOR_HPP
#define AFFINITY_EXTRACTOR_HPP

#include "Types.h"
#include <fstream>
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

    const SetContainer<Ts> & boundarySupervoxels(int face) const {return m_boundarySupervoxels[face];}
    const MapContainer<SegPair<Ts>, Edge<Ta>, HashFunction<SegPair<Ts> > > & edges() const {return m_edges;}
private:
    const Chunk & m_aff;
    std::array<SetContainer<Ts>, 6> m_boundarySupervoxels;
    MapContainer<SegPair<Ts>, Edge<Ta>, HashFunction<SegPair<Ts> > > m_edges;
};


#endif
