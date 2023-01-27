#ifndef CHUNKED_RG_EXTRACTOR_HPP
#define CHUNKED_RG_EXTRACTOR_HPP

#include "Types.h"
#include "SlicedOutput.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/format.hpp>

template<typename Ts, typename Ta, typename Chunk>
class ChunkedRGExtractor
{
public:
    explicit ChunkedRGExtractor(const Chunk & aff)
        :m_aff(aff), m_edges() {}

    void collectVoxel(Coord & c, Ts segid) {}

    void collectBoundary(int face, Coord & c, Ts segid) {}

    void collectContactingSurface(int nv, Coord & c, Ts segid1, Ts segid2)
    {
        auto p = std::minmax(segid1, segid2);
        m_edges[p][nv].first += m_aff[c[0]][c[1]][c[2]][nv];
        m_edges[p][nv].second += 1;
    }

    void writeEdge(auto & out, const std::pair<Ts, Ts> & k, const std::array<std::pair<Ta, size_t>, 3> & v) {
        rg_edge_t payload = {k.first, k.second, v[0].first, v[0].second, v[1].first, v[1].second, v[2].first, v[2].second};
        out.addPayload(std::move(payload));
    }

    void output(const MapContainer<Ts, Ts> & chunkMap, const std::string & tag, size_t ac_offset)
    {
        size_t current_ac1 = std::numeric_limits<std::size_t>::max();
        size_t current_ac2 = std::numeric_limits<std::size_t>::max();

        std::vector<std::pair<SegPair<Ts>, std::array<std::pair<Ta, size_t>, 3> > > sorted_edges(std::begin(m_edges), std::end(m_edges));
        std::stable_sort(std::begin(sorted_edges), std::end(sorted_edges), [ac_offset](auto & a, auto & b) {
                auto c1 = a.first.first - (a.first.first % ac_offset);
                auto c2 = b.first.first - (b.first.first % ac_offset);
                return (c1 < c2) || (c1 == c2 && a.first.second < b.first.second);
                });

        SlicedOutput<rg_edge_t, Ts> inChunkOutput(str(boost::format("chunked_rg/in_chunk_%1%.data") % tag));
        SlicedOutput<rg_edge_t, std::pair<Ts, Ts> > betweenChunksOutput(str(boost::format("chunked_rg/between_chunks_%1%.data") % tag));
        SlicedOutput<rg_edge_t, std::pair<Ts, Ts> > fakeOutput(str(boost::format("chunked_rg/fake_%1%.data") % tag));
        for (const auto & [k,v] : sorted_edges) {
            auto s1 = k.first;
            auto s2 = k.second;
            if (current_ac1 != (s1 - (s1 % ac_offset))) {
                inChunkOutput.flushChunk(current_ac1);
                betweenChunksOutput.flushChunk(std::make_pair(current_ac1, current_ac2));
                fakeOutput.flushChunk(std::make_pair(current_ac1, current_ac2));
                current_ac1 = s1 - (s1 % ac_offset);
                current_ac2 = s2 - (s2 % ac_offset);
            } else if (current_ac2 != (s2 - (s2 % ac_offset))) {
                if (current_ac1 != current_ac2) {
                    betweenChunksOutput.flushChunk(std::make_pair(current_ac1, current_ac2));
                    fakeOutput.flushChunk(std::make_pair(current_ac1, current_ac2));
                }
                current_ac2 = s2 - (s2 % ac_offset);
            }

            if (chunkMap.count(s1) > 0) {
                s1 = chunkMap.at(k.first);
            }

            if (chunkMap.count(s2) > 0) {
                s2 = chunkMap.at(k.second);
            }

            if (s1 == s2) {
                writeEdge(fakeOutput, k, v);
            } else if (current_ac1 == current_ac2) {
                writeEdge(inChunkOutput, k, v);
            } else {
                writeEdge(betweenChunksOutput, k, v);
            }
        }
        inChunkOutput.flushChunk(current_ac1);
        betweenChunksOutput.flushChunk(std::make_pair(current_ac1, current_ac2));
        fakeOutput.flushChunk(std::make_pair(current_ac1, current_ac2));
        inChunkOutput.flushIndex();
        betweenChunksOutput.flushIndex();
        fakeOutput.flushIndex();
    }

private:
    struct __attribute__((packed)) rg_edge_t {
        Ts s1;
        Ts s2;
        Ta aff_x;
        size_t area_x;
        Ta aff_y;
        size_t area_y;
        Ta aff_z;
        size_t area_z;
    };
    const Chunk & m_aff;
    MapContainer<SegPair<Ts>, std::array<std::pair<Ta, size_t>, 3>, HashFunction<SegPair<Ts> > > m_edges;
};

#endif
