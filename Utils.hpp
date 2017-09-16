#ifndef UTILS_HPP
#define UTILS_HPP

#include "Types.h"

template <typename Ts, typename ... Ta>
void traverseSegments(const Ts& seg, bool overlappingChunk, Ta& ... extractors)
{
    auto base = seg.index_bases();
    auto shape = seg.shape();
    for (int z = base[2]; z != base[2]+shape[2]; z++) {
        for (int y = base[1]; y != base[1]+shape[1]; y++) {
            for (int x = base[0]; x != base[0]+shape[0]; x++) {
                if (seg[x][y][z] == 0) {
                    continue;
                }
                auto c = Coord({x,y,z});
                for (int i = 0; i < 3; i++) {
                    if (c[i] == base[i]) {
                        (extractors.collectBoundary(i, c, seg[x][y][z]), ...);
                    }
                    if (c[i] == (base[i] + shape[i] - 1)) {
                        (extractors.collectBoundary(i+3, c, seg[x][y][z]), ...);
                    }
                }
                if (overlappingChunk) {
                    if (x == base[0] || y == base[1] || z == base[2]) {
                        continue;
                    }
                }
                (extractors.collectVoxel(c, seg[x][y][z]), ...);
                if (x > seg.index_bases()[0] && seg[x-1][y][z] != 0 && seg[x][y][z] != seg[x-1][y][z]) {
                    (extractors.collectContactingSurface(0, c, seg[x][y][z], seg[x-1][y][z]), ...);
                }
                if (y > seg.index_bases()[1] && seg[x][y-1][z] != 0 && seg[x][y][z] != seg[x][y-1][z]) {
                    (extractors.collectContactingSurface(1, c, seg[x][y][z], seg[x][y-1][z]), ...);
                }
                if (z > seg.index_bases()[2] && seg[x][y][z-1] != 0 && seg[x][y][z] != seg[x][y][z-1]) {
                    (extractors.collectContactingSurface(2, c, seg[x][y][z], seg[x][y][z-1]), ...);
                }
            }
        }
    }
}

#endif
