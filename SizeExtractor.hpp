#ifndef SIZE_EXTRACTOR_HPP
#define SIZE_EXTRACTOR_HPP

#include "Types.h"
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
    void output(const std::unordered_set<T> & incompleteSegments, const std::string & completeFileName, const std::string & incompleteFileName)
    {
        std::ofstream complete(completeFileName, std::ios_base::binary);
        std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
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

#endif
