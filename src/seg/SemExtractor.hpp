#ifndef SEM_EXTRACTOR_HPP
#define SEM_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_map>

using sem_array_t = std::array<seg_t, 3>;
using sem_dict_t = std::unordered_map<seg_t, size_t>;

template<typename Tseg, typename Tsem, typename Chunk>
class SemExtractor
{
public:
    explicit SemExtractor(const Chunk & sem)
        :m_sem(sem){}
    void collectVoxel(Coord c, Tseg segid)
    {
        auto sem_label = m_sem[c[0]][c[1]][c[2]];
        if (not m_labels.contains(segid)) {
            m_labels[segid] = sem_dict_t();
        }
        auto & sem_dict = m_labels[segid];
        if (sem_dict.contains(sem_label)) {
            sem_dict[sem_label] += 1;
        } else {
            sem_dict[sem_label] = 1;
        }
    }

    void collectBoundary(int face, Coord c, Tseg segid) {}
    void collectContactingSurface(int nv, Coord c, Tseg segid1, Tseg segid2) {}
    const MapContainer<Tseg, sem_dict_t> & sem_labels() {
        return m_labels;
    }

    void output(const MapContainer<Tseg, Tseg> & chunkMap, const std::string & outputFileName)
    {
        MapContainer<Tseg, sem_array_t> remapped_labels;
        std::ofstream of(outputFileName, std::ios_base::binary);
        assert(of.is_open());
        for (const auto & [k, v]: m_labels) {
            auto svid = k;
            auto sem_labels = mapToArray(v);
            if (chunkMap.count(k) > 0) {
                svid = chunkMap.at(k);
            }
            if ((remapped_labels.count(svid) == 0) or (remapped_labels[svid][1] < sem_labels[1])) {
                    remapped_labels[svid] = sem_labels;
            }
        }
        for (const auto & [k,v] : remapped_labels) {
            write_sem(of, k ,v);
        }
        of.close();
    }

private:
    sem_array_t mapToArray(const sem_dict_t & entries) {
        std::vector<std::pair<seg_t, size_t> > vec(entries.begin(), entries.end());
        std::sort(vec.begin(), vec.end(), [](const auto & a, const auto & b) {
            return a.second > b.second;
        });
        auto & top = vec[0];
        auto sum = std::accumulate(vec.begin(), vec.end(), static_cast<size_t>(0),
                              [](auto partialSum, const auto & element) {
                                  return partialSum + element.second;
                              });
        return  sem_array_t{top.first, top.second, sum};
    }

    void write_sem(auto & io, const auto & k, const auto & v) {
        io.write(reinterpret_cast<const char *>(&k), sizeof(k));
        io.write(reinterpret_cast<const char *>(&v), sizeof(v));
        assert(!io.bad());
    }
    //static constexpr std::array<int, 6> sem_map = {-1,0,1,2,3,4};
    // dummy, soma, axon, dendrite, glia, bv
    static constexpr std::array<int, 6> sem_map = {-1,-1,1,0,2,2};
    // dummy, axon, bv, dendrite, glia, soma
    //static constexpr std::array<int, 6> sem_map = {-1,1,2,0,2,-1};
    const Chunk & m_sem;
    MapContainer<Tseg, sem_dict_t> m_labels;
};


#endif
