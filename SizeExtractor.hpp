#ifndef SIZE_EXTRACTOR_HPP
#define SIZE_EXTRACTOR_HPP

#include "Types.h"
#include <unordered_map>

template<typename T>
class SizeExtractor
{
public:
    void collectVoxel(Coord c, T segid)
    {
        m_sizes[segid] += 1;
    }
    void collectBoundary(int face, Coord c, T segid) {}
    void collectContactingSurface(int nv, Coord c, T segid1, T segid2) {}
    void finish() {}
private:
    std::unordered_map<T, int64_t> m_sizes;
};

#endif
