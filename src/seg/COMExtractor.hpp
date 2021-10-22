#ifndef COM_EXTRACTOR_HPP
#define COM_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>

struct com_t
{
    std::array<int64_t,3> sumCoord;
    size_t volume;
    com_t() : volume(0), sumCoord({0,0,0}) {}
};

template<typename T>
class COMExtractor
{
public:
    void collectVoxel(Coord c, T segid)
    {
        for (int i = 0; i != 3; i++) {
            m_com[segid].sumCoord[i] += c[i];
        }
        m_com[segid].volume += 1;
    }
    void collectBoundary(int face, Coord c, T segid) {}
    void collectContactingSurface(int nv, Coord c, T segid1, T segid2) {}
    const MapContainer<T, com_t> & com() {
        return m_com;
    }

    void output(const SetContainer<T> & incompleteSegments, const std::string & completeFileName, const std::string & incompleteFileName)
    {
        std::ofstream complete(completeFileName, std::ios_base::binary);
        std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
        assert(complete.is_open());
        assert(incomplete.is_open());
        for (const auto & [k,v] : m_com) {
            if (incompleteSegments.count(k) > 0) {
                write_com(incomplete, k ,v);
            } else {
                write_com(complete, k ,v);
            }
        }
        complete.close();
        incomplete.close();
    }

private:
    void write_com(auto & io, const auto & k, const auto & v) {
        io.write(reinterpret_cast<const char *>(&k), sizeof(k));
        io.write(reinterpret_cast<const char *>(&k), sizeof(k));
        io.write(reinterpret_cast<const char *>(&v), sizeof(v));
        assert(!io.bad());
    }
    MapContainer<T, com_t> m_com;
};

#endif //COM_EXTRACTOR_HPP
