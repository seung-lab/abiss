#ifndef BOUNDARY_EXTRACTOR_HPP
#define BOUNDARY_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <fstream>
#include <unordered_set>
#include <boost/format.hpp>

template<typename Ts>
class BoundaryExtractor
{
public:
    BoundaryExtractor()
        :m_boundarySupervoxels() {}
    void collectVoxel(Coord & c, Ts segid) {}

    void collectBoundary(int face, Coord & c, Ts segid)
    {
        m_boundarySupervoxels[face].insert(segid);
    }

    void collectContactingSurface(int nv, Coord & c, Ts segid1, Ts segid2) {}

    std::unordered_set<Ts> boundarySupervoxels(int face) const {return m_boundarySupervoxels[face];}

    std::unordered_set<Ts> incompleteSupervoxels()
    {
        std::unordered_set<Ts> is;
        for (int i = 0; i != 6; i++) {
            is.merge(boundarySupervoxels(i));
        }
        return is;
    }

    void output(const std::string & fnProto)
    {
        for (int i= 0; i != 6; i++) {
            std::ofstream out(str(boost::format(fnProto) % i), std::ios_base::binary);
            for (auto x : boundarySupervoxels(i)) {
                out.write(reinterpret_cast<const char *>(&x), sizeof(x));
            }
            assert(!out.bad());
            out.close();
        }
    }
private:
    std::array<std::unordered_set<Ts>, 6> m_boundarySupervoxels;
};

#endif

