#ifndef SEM_EXTRACTOR_HPP
#define SEM_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_map>

using sem_array_t = std::array<size_t, 3>;

template<typename Tseg, typename Tsem, typename Chunk>
class SemExtractor
{
public:
    SemExtractor(const Chunk & sem)
        :m_sem(sem){}
    void collectVoxel(Coord c, Tseg segid)
    {
        auto sem_label = m_sem[c[0]][c[1]][c[2]];
        if (sem_label >= 0 and sem_map[sem_label] >= 0) {
            m_labels[segid][sem_map[sem_label]] += 1;
        }
    }

    void collectBoundary(int face, Coord c, Tseg segid) {}
    void collectContactingSurface(int nv, Coord c, Tseg segid1, Tseg segid2) {}
    const std::unordered_map<Tseg, sem_array_t> & sem_lables() {
        return m_labels;
    }

    void output(const std::unordered_map<Tseg, Tseg> & chunkMap, const std::string & outputFileName)
    {
        std::unordered_map<Tseg, sem_array_t> remapped_labels;
        std::ofstream of(outputFileName, std::ios_base::binary);
        assert(of.is_open());
        for (const auto & [k, v]: m_labels) {
            auto svid = k;
            if (chunkMap.count(k) > 0) {
                svid = chunkMap.at(k);
            }
            if (remapped_labels.count(svid) == 0) {
                remapped_labels[svid] = v;
            } else {
                std::transform(remapped_labels[svid].begin(), remapped_labels[svid].end(), v.begin(), remapped_labels[svid].begin(), std::plus<size_t>());
            }
        }
        for (const auto & [k,v] : remapped_labels) {
            write_sem(of, k ,v);
        }
        of.close();
    }

private:
    void write_sem(auto & io, const auto & k, const auto & v) {
        io.write(reinterpret_cast<const char *>(&k), sizeof(k));
        io.write(reinterpret_cast<const char *>(&v), sizeof(v));
        assert(!io.bad());
    }
    // dummy, soma, axon, dendrite, glia, bv
    //static constexpr std::array<int, 6> sem_map = {-1,0,1,2,3,4};
    static constexpr std::array<int, 6> sem_map = {-1,-1,1,0,2,2};
    const Chunk & m_sem;
    std::unordered_map<Tseg, sem_array_t> m_labels;
};


#endif
