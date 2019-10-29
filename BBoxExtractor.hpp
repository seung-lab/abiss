#ifndef BBOX_EXTRACTOR_HPP
#define BBOX_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <limits>
#include <unordered_map>

template<typename T>
struct BBox
{
    std::array<T, 3> minPt;
    std::array<T, 3> maxPt;
    BBox()
        :minPt({std::numeric_limits<T>::max(),
                 std::numeric_limits<T>::max(),
                 std::numeric_limits<T>::max()})
        ,maxPt({std::numeric_limits<T>::min(),
                 std::numeric_limits<T>::min(),
                 std::numeric_limits<T>::min()})
    {
    }
};

template<typename Ts, typename Ti>
class BBoxExtractor
{
public:
    void collectVoxel(Coord c, Ts segid)
    {
        for (int i = 0; i != 3; i++) {
            m_bbox[segid].minPt[i] = std::min(c[i], m_bbox[segid].minPt[i]);
            m_bbox[segid].maxPt[i] = std::max(c[i], m_bbox[segid].maxPt[i]);
        }
    }
    void collectBoundary(int face, Coord c, Ts segid) {}
    void collectContactingSurface(int nv, Coord c, Ts segid1, Ts segid2) {}
    const std::unordered_map<Ts, BBox<Ti> > & bbox() {
        return m_bbox;
    }
    void output(const std::unordered_set<Ts> & incompleteSegments, const std::string & completeFileName, const std::string & incompleteFileName)
    {
        std::ofstream complete(completeFileName, std::ios_base::binary);
        std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
        assert(complete.is_open());
        assert(incomplete.is_open());
        for (const auto & [k,v] : m_bbox) {
            if (incompleteSegments.count(k) > 0) {
                writeBBox(incomplete, k ,v);
            } else {
                writeBBox(complete, k ,v);
            }
        }
        complete.close();
        incomplete.close();
    }
private:
    std::unordered_map<Ts, BBox<Ti> > m_bbox;
};

void writeBBox(auto & io, const auto & k, const auto & v) {
    io.write(reinterpret_cast<const char *>(&k), sizeof(k));
    io.write(reinterpret_cast<const char *>(&k), sizeof(k));
    io.write(reinterpret_cast<const char *>(v.minPt.data()), sizeof(v.minPt));
    io.write(reinterpret_cast<const char *>(v.maxPt.data()), sizeof(v.maxPt));
    assert(!io.bad());
}

template <typename Ts, typename Ti>
std::unordered_map<Ts, BBox<Ti> > loadBBoxes(const std::string & fileName)
{
    std::unordered_map<Ts, BBox<Ti> > bboxes;
    std::cout << "loading: " << fileName << std::endl;
    std::ifstream in(fileName);
    assert(in.is_open());
    Ts s;
    BBox<Ti> bbox;
    while (in.read(reinterpret_cast<char *>(&s), sizeof(s))) {
        in.read(reinterpret_cast<char *>(&s), sizeof(s));
        in.read(reinterpret_cast<char *>(bbox.minPt.data()), sizeof(bbox.minPt));
        in.read(reinterpret_cast<char *>(bbox.maxPt.data()), sizeof(bbox.maxPt));
        for (int i = 0; i != 3; i++) {
            bboxes[s].minPt[i] = std::min(bbox.minPt[i], bboxes[s].minPt[i]);
            bboxes[s].maxPt[i] = std::max(bbox.maxPt[i], bboxes[s].maxPt[i]);
        }
    }
    assert(!in.bad());
    in.close();
    return bboxes;
}

template <typename Ts, typename Ti>
void writeBBoxes(const std::unordered_set<Ts> & incompleteSegments, const std::unordered_map<Ts, BBox<Ti> > bboxes, const std::string & incompleteFileName, const std::string & completeFileName)
{
    std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
    std::ofstream complete(completeFileName, std::ios_base::binary);
    for (const auto &[k,v] : bboxes) {
        if (incompleteSegments.count(k) > 0) {
            writeBBox(incomplete, k, v);
        } else {
            writeBBox(complete, k, v);
        }
    }
    incomplete.close();
    complete.close();
}

template <typename Ts, typename Ti>
void updateBBoxes(const std::unordered_set<Ts> & incompleteSegments, const std::string & inputFileName, const std::string & incompleteFileName, const std::string & completeFileName)
{
    writeBBoxes<Ts>(incompleteSegments, loadBBoxes<Ts, Ti>(inputFileName), incompleteFileName, completeFileName);
}

#endif //BBOX_EXTRACTOR_HPP
