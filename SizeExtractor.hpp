#ifndef SIZE_EXTRACTOR_HPP
#define SIZE_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
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
    const std::unordered_map<T, size_t> & sizes() {
        return m_sizes;
    }

    const std::unordered_map<T, size_t> svSizes(const std::unordered_map<T, T> & chunkMap) {
        std::unordered_map<T, size_t> mergedSize;
        for (auto & [ k, v ]: m_sizes) {
            auto sid = k;
            if (chunkMap.count(sid) > 0) {
                sid = chunkMap.at(k);
            }
            if (mergedSize.count(sid) == 0) {
                mergedSize[sid] = 1;
            } else {
                mergedSize[sid] += 1;
            }
        }
        return mergedSize;
    }

    void output(const std::unordered_set<T> & incompleteSegments, const std::string & completeFileName, const std::string & incompleteFileName)
    {
        std::ofstream complete(completeFileName, std::ios_base::binary);
        std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
        assert(complete.is_open());
        assert(incomplete.is_open());
        for (const auto & [k,v] : m_sizes) {
            if (incompleteSegments.count(k) > 0) {
                write_size(incomplete, k ,v);
            } else {
                write_size(complete, k ,v);
            }
        }
        complete.close();
        incomplete.close();
    }
private:
    void write_size(auto & io, const auto & k, const auto & v) {
        io.write(reinterpret_cast<const char *>(&k), sizeof(k));
        io.write(reinterpret_cast<const char *>(&k), sizeof(k));
        io.write(reinterpret_cast<const char *>(&v), sizeof(v));
        assert(!io.bad());
    }
    std::unordered_map<T, size_t> m_sizes;
};

template <typename Ts>
std::unordered_map<Ts, size_t> loadSizes(const std::string & fileName)
{
    std::unordered_map<Ts, size_t> sizes;
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
void writeSizes(const std::unordered_set<Ts> & incompleteSegments, const std::unordered_map<Ts, size_t> sizes, const std::string & incompleteFileName, const std::string & completeFileName)
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
void updateSizes(const std::unordered_set<Ts> & incompleteSegments, const std::string & inputFileName, const std::string & incompleteFileName, const std::string & completeFileName)
{
    writeSizes<Ts>(incompleteSegments, loadSizes<Ts>(inputFileName), incompleteFileName, completeFileName);
}

#endif
