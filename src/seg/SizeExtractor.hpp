#ifndef SIZE_EXTRACTOR_HPP
#define SIZE_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>

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
    const MapContainer<T, size_t> & sizes() {
        return m_sizes;
    }

    const MapContainer<T, size_t> svSizes(const MapContainer<T, T> & chunkMap) {
        MapContainer<T, size_t> mergedSize;
        for (auto & [ k, v ]: m_sizes) {
            auto sid = k;
            if (chunkMap.count(sid) > 0) {
                sid = chunkMap.at(k);
            }
            if (mergedSize.count(sid) == 0) {
                mergedSize[sid] = 1;
            } /* else {
                mergedSize[sid] += 1;
            } */
        }
        return mergedSize;
    }

    void output(const MapContainer<T, T> & chunkMap, const std::string & outputFileName)
    {
        MapContainer<T, size_t> remapped_sizes;
        std::ofstream of(outputFileName, std::ios_base::binary);
        assert(of.is_open());
        for (const auto & [k, v]: m_sizes) {
            auto svid = k;
            if (chunkMap.count(k) > 0) {
                svid = chunkMap.at(k);
            }
            if (remapped_sizes.count(svid) == 0) {
                remapped_sizes[svid] = v;
            } else {
                remapped_sizes[svid] += v;
            }
        }
        for (const auto & [k,v] : remapped_sizes) {
            write_size(of, k ,v);
        }
        of.close();
    }

private:
    void write_size(auto & io, const auto & k, const auto & v) {
        io.write(reinterpret_cast<const char *>(&k), sizeof(k));
        io.write(reinterpret_cast<const char *>(&v), sizeof(v));
        assert(!io.bad());
    }
    MapContainer<T, size_t> m_sizes;
};

template <typename Ts>
MapContainer<Ts, size_t> loadSizes(const std::string & fileName)
{
    MapContainer<Ts, size_t> sizes;
    std::cout << "loading: " << fileName << std::endl;
    std::ifstream in(fileName);
    Ts s;
    size_t size;
    while (in.read(reinterpret_cast<char *>(&s), sizeof(s))) {
        in.read(reinterpret_cast<char *>(&s), sizeof(s));
        in.read(reinterpret_cast<char *>(&size), sizeof(size));
        //if (sizes.count(s) > 0) {
        //    std::cout << s << " " << size << std::endl;
        //}
        sizes[s] += size;
    }
    assert(!in.bad());
    in.close();
    return sizes;
}

template <typename Ts>
void writeSizes(const SetContainer<Ts> & incompleteSegments, const MapContainer<Ts, size_t> sizes, const std::string & incompleteFileName, const std::string & completeFileName)
{
    std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
    std::ofstream complete(completeFileName, std::ios_base::binary);
    for (const auto &[k,v] : sizes) {
        if (incompleteSegments.count(k) > 0) {
            incomplete.write(reinterpret_cast<const char *> (&k), sizeof(k));
            incomplete.write(reinterpret_cast<const char *> (&k), sizeof(k));
            incomplete.write(reinterpret_cast<const char *> (&v), sizeof(v));
        } else {
            complete.write(reinterpret_cast<const char *> (&k), sizeof(k));
            complete.write(reinterpret_cast<const char *> (&k), sizeof(k));
            complete.write(reinterpret_cast<const char *> (&v), sizeof(v));
        }
    }
    assert(!incomplete.bad());
    assert(!complete.bad());
    incomplete.close();
    complete.close();
}

template <typename Ts>
void updateSizes(const SetContainer<Ts> & incompleteSegments, const std::string & inputFileName, const std::string & incompleteFileName, const std::string & completeFileName)
{
    writeSizes<Ts>(incompleteSegments, loadSizes<Ts>(inputFileName), incompleteFileName, completeFileName);
}

#endif
