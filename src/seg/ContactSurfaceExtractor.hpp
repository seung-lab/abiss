#ifndef CONTACT_SURFACE_EXTRACTOR_HPP
#define CONTACT_SURFACE_EXTRACTOR_HPP

#include "Types.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <boost/format.hpp>

using BBox = std::array<int64_t, 6>;

struct CRInfo {
    SegPair<seg_t> p;
    std::array<size_t, 4> com;
    std::array<size_t, 3> sizes;
    BBox bbox;
    CRInfo() {
        p = {0, 0};
        com = {0,0,0,0};
        bbox = {
            std::numeric_limits<int64_t>::max(),
            std::numeric_limits<int64_t>::max(),
            std::numeric_limits<int64_t>::max(),
            std::numeric_limits<int64_t>::min(),
            std::numeric_limits<int64_t>::min(),
            std::numeric_limits<int64_t>::min()
        };
        sizes = {0,0,0};
    }
    CRInfo(const SegPair<seg_t> & seg_pair, const ContactRegionExt & cre) {
        com = {0,0,0,0};
        bbox = {
            std::numeric_limits<int64_t>::max(),
            std::numeric_limits<int64_t>::max(),
            std::numeric_limits<int64_t>::max(),
            std::numeric_limits<int64_t>::min(),
            std::numeric_limits<int64_t>::min(),
            std::numeric_limits<int64_t>::min()
        };
        sizes = {0,0,0};
        p = seg_pair;
        for (auto & [c, n]: cre) {
            com[3] += 1;
            for (size_t i = 0; i != 3; i++) {
                com[i] += c[i];
            }
            for (size_t i = 0; i != 3; i++) {
                bbox[i] = bbox[i] < c[i] ? bbox[i] : c[i];
                bbox[i+3] = bbox[i+3] > c[i] ? bbox[i+3] : c[i];
            }
            for (size_t i = 0; i != 3; i++) {
                if (n&(1<<i)) {
                    sizes[i] += 1;
                }
            }
        }
    }
    void merge_with(const CRInfo & other) {
        if (p != other.p) {
            std::cerr << "Cannot merge contact surface between different segment pair!" << std::endl;
            std::abort();
        }
        for (int i = 0; i != 4; i++) {
            com[i] += other.com[i];
        }
        for (int i = 0; i != 3; i++) {
            bbox[i] = bbox[i] < other.bbox[i] ? bbox[i] : other.bbox[i];
            bbox[i+3] = bbox[i+3] > other.bbox[i+3] ? bbox[i+3] : other.bbox[i+3];
        }
        for (int i = 0; i != 3; i++) {
            sizes[i] += other.sizes[i];
        }
    }
    void remove_duplicated_voxel(const Coord & c, int n) {
        com[3] -= 1;
        for (int i = 0; i != 3; i++) {
            com[i] -= c[i];
            if (n&(1<<i)) {
                sizes[i] -= 1;
            }
        }
    }
};


template<typename Ts>
class ContactSurfaceExtractor
{
public:
    ContactSurfaceExtractor()
        :m_surfaces() {}

    void collectVoxel(Coord & c, Ts segid) {}

    void collectBoundary(int face, Coord & c, Ts segid) {}

    void collectContactingSurface(int nv, Coord & c, Ts segid1, Ts segid2)
    {
        auto p = std::minmax(segid1, segid2);
        m_surfaces[p][c] |= 1<<nv;
    }

    std::vector<std::pair<CRInfo, ContactRegionExt> > contactSurfaces() {
        std::vector<std::pair<CRInfo, ContactRegionExt> > crs;
        for (const auto & [k, v] : m_surfaces) {
            ContactRegion cr;
            for (const auto & [c,n] : v) {
                cr.insert(c);
            }
            auto ccs = connectComponent(cr);
            for (auto & cc: ccs) {
                ContactRegionExt cre;
                for (const auto c: cc) {
                    cre[c] = v.at(c);
                }
                CRInfo ci(k, cre);
                crs.push_back(std::make_pair(ci, cre));
            }
        }
        return crs;
    }

private:
    std::unordered_map<SegPair<Ts>, ContactRegionExt, boost::hash<SegPair<Ts> > > m_surfaces;
};


#endif
