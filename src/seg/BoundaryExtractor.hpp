#ifndef BOUNDARY_EXTRACTOR_HPP
#define BOUNDARY_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
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

    SetContainer<Ts> incompleteSupervoxels(const MapContainer<Ts, Ts> & map)
    {
        SetContainer<Ts> is;
        for (int i = 0; i != 6; i++) {
            is.merge(boundarySupervoxels(i, map));
        }
        return is;
    }

    void output(const std::string & fnProto, const MapContainer<Ts, Ts> & map)
    {
        for (int i= 0; i != 6; i++) {
            std::ofstream out(str(boost::format(fnProto) % i), std::ios_base::binary);
            for (auto x : boundarySupervoxels(i, map)) {
                out.write(reinterpret_cast<const char *>(&x), sizeof(x));
            }
            assert(!out.bad());
            out.close();
        }
    }
private:
    SetContainer<Ts> boundarySupervoxels(int face, const MapContainer<Ts, Ts> & map) const
    {
        SetContainer<Ts> res;
        for (auto & s: m_boundarySupervoxels[face]) {
            if (map.count(s) > 0) {
                res.insert(map.at(s));
            } else {
                res.insert(s);
            }
        }
        return res;
    }

    std::array<SetContainer<Ts>, 6> m_boundarySupervoxels;
};

template<typename Ts>
SetContainer<Ts> updateBoundarySegments(size_t index, const std::string & tag)
{
    SetContainer<Ts> incomplete_segments;
    Ts s;
    std::ifstream in(str(boost::format("boundary_%1%_%2%.tmp") % index % tag));
    std::cout << "update: " << index << std::endl;
    while (in.read(reinterpret_cast<char *>(&s), sizeof(s))) {
        incomplete_segments.insert(s);
    }

    assert(!in.bad());

    in.close();
    std::ofstream out(str(boost::format("boundary_%1%_%2%.data") % index % tag), std::ios_base::binary);
    for (const auto & v : incomplete_segments) {
        out.write(reinterpret_cast<const char *>(&v), sizeof(v));
    }
    assert(!out.bad());
    out.close();
    return incomplete_segments;
}

#endif

