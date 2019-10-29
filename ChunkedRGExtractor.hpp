#ifndef CHUNKED_RG_EXTRACTOR_HPP
#define CHUNKED_RG_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <boost/format.hpp>

template<typename Ts, typename Ta, typename Chunk>
class ChunkedRGExtractor
{
public:
    ChunkedRGExtractor(const Chunk & aff)
        :m_aff(aff), m_edges() {}

    void collectVoxel(Coord & c, Ts segid) {}

    void collectBoundary(int face, Coord & c, Ts segid) {}

    void collectContactingSurface(int nv, Coord & c, Ts segid1, Ts segid2)
    {
        auto p = std::minmax(segid1, segid2);
        m_edges[p][nv].first += m_aff[c[0]][c[1]][c[2]][nv];
        m_edges[p][nv].second += 1;
    }

    void writeEdge(std::ofstream & out, const std::pair<Ts, Ts> & k, const std::array<std::pair<Ta, size_t>, 3> & v) {
        out.write(reinterpret_cast<const char *>(&(k.first)), sizeof(Ts));
        out.write(reinterpret_cast<const char *>(&(k.second)), sizeof(Ts));
        for (size_t i = 0; i < 3; i++) {
            out.write(reinterpret_cast<const char *>(&(v[i].first)), sizeof(Ta));
            out.write(reinterpret_cast<const char *>(&(v[i].second)), sizeof(size_t));
        }
    }

    void output(const std::unordered_map<Ts, Ts> & chunkMap, const std::string & tag, size_t ac_offset)
    {
        std::ofstream ofInChunk;
        std::ofstream ofBetweenChunks;
        std::ofstream ofFake;
        size_t current_ac1 = std::numeric_limits<std::size_t>::max();
        size_t current_ac2 = std::numeric_limits<std::size_t>::max();

        std::vector<std::pair<SegPair<Ts>, std::array<std::pair<Ta, size_t>, 3> > > sorted_edges(std::begin(m_edges), std::end(m_edges));
        std::stable_sort(std::begin(sorted_edges), std::end(sorted_edges), [ac_offset](auto & a, auto & b) {
                auto c1 = a.first.first - (a.first.first % ac_offset);
                auto c2 = b.first.first - (b.first.first % ac_offset);
                return (c1 < c2) || (c1 == c2 && a.first.second < b.first.second);
                });

        for (const auto & [k,v] : sorted_edges) {
            auto s1 = k.first;
            auto s2 = k.second;
            if (current_ac1 != (s1 - (s1 % ac_offset))) {
                if (ofInChunk.is_open()) {
                    ofInChunk.close();
                    if (ofInChunk.is_open()) {
                        std::abort();
                    }
                }
                if (ofBetweenChunks.is_open()) {
                    ofBetweenChunks.close();
                    if (ofBetweenChunks.is_open()) {
                        std::abort();
                    }
                }
                if (ofFake.is_open()) {
                    ofFake.close();
                    if (ofFake.is_open()) {
                        std::abort();
                    }
                }
                current_ac1 = s1 - (s1 % ac_offset);
                current_ac2 = s2 - (s2 % ac_offset);
                ofInChunk.open(str(boost::format("chunked_rg/in_chunk_%1%_%2%.data") % tag % current_ac1));
                if (!ofInChunk.is_open()) {
                    std::abort();
                }
                if (current_ac1 != current_ac2) {
                    ofBetweenChunks.open(str(boost::format("chunked_rg/between_chunks_%1%_%2%_%3%.data") % tag % current_ac1 % current_ac2));
                    if (!ofBetweenChunks.is_open()) {
                        std::abort();
                    }
                    ofFake.open(str(boost::format("chunked_rg/fake_%1%_%2%_%3%.data") % tag % current_ac1 % current_ac2));
                    if (!ofFake.is_open()) {
                        std::abort();
                    }
                }
            } else if (current_ac2 != (s2 - (s2 % ac_offset))) {
                if (current_ac1 != current_ac2) {
                    if (ofBetweenChunks.is_open()) {
                        ofBetweenChunks.close();
                        if (ofBetweenChunks.is_open()) {
                            std::abort();
                        }
                    }
                    if (ofFake.is_open()) {
                        ofFake.close();
                        if (ofFake.is_open()) {
                            std::abort();
                        }
                    }
                }
                current_ac2 = s2 - (s2 % ac_offset);
                if (current_ac1 != current_ac2) {
                    ofBetweenChunks.open(str(boost::format("chunked_rg/between_chunks_%1%_%2%_%3%.data") % tag % current_ac1 % current_ac2));
                    if (!ofBetweenChunks.is_open()) {
                        std::abort();
                    }
                    ofFake.open(str(boost::format("chunked_rg/fake_%1%_%2%_%3%.data") % tag % current_ac1 % current_ac2));
                    if (!ofFake.is_open()) {
                        std::abort();
                    }
                }
            }

            if (chunkMap.count(s1) > 0) {
                s1 = chunkMap.at(k.first);
            }

            if (chunkMap.count(s2) > 0) {
                s2 = chunkMap.at(k.second);
            }

            if (s1 == s2) {
                writeEdge(ofFake, k, v);
            } else if (current_ac1 == current_ac2) {
                writeEdge(ofInChunk, k, v);
            } else {
                writeEdge(ofBetweenChunks, k, v);
            }
            assert(!ofFake.bad());
            assert(!ofBetweenChunks.bad());
            assert(!ofInChunk.bad());
        }
        if (ofInChunk.is_open()) {
            ofInChunk.close();
            if (ofInChunk.is_open()) {
                std::abort();
            }
        }
        if (ofBetweenChunks.is_open()) {
            ofBetweenChunks.close();
            if (ofBetweenChunks.is_open()) {
                std::abort();
            }
        }
        if (ofFake.is_open()) {
            ofFake.close();
            if (ofFake.is_open()) {
                std::abort();
            }
        }
    }

private:
    const Chunk & m_aff;
    std::unordered_map<SegPair<Ts>, std::array<std::pair<Ta, size_t>, 3>, boost::hash<SegPair<Ts> > > m_edges;
};

#endif
