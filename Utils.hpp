#ifndef UTILS_HPP
#define UTILS_HPP

#include "Types.h"
#include <queue>
#include <unordered_map>
#include <fstream>
#include <iostream>

template<typename T>
std::unordered_map<T,T> loadChunkMap(const char * filename)
{
    std::unordered_map<T, T> map;
    std::ifstream cm_file(filename);
    if (!cm_file.is_open()) {
        std::cout << "Cannot open the supervoxel count file" << std::endl;
        std::abort();
    }

    T k,v;
    while (cm_file.read(reinterpret_cast<char *>(&k), sizeof(k))) {
           cm_file.read(reinterpret_cast<char *>(&v), sizeof(v));
           map[k] = v;
    }
    std::cout << "load " << map.size() << " chunk map entries" << std::endl;
    return map;
}


std::vector<ContactRegion> connectComponent(const ContactRegion & cr)
{
    std::vector<ContactRegion> cc_sets;
    ContactRegion visited;
    for (auto root: cr) {
        if (visited.count(root) > 0) {
            continue;
        }
        ContactRegion cc;
        std::queue<Coord> voxel_queue;
        voxel_queue.push(root);
        while (!voxel_queue.empty()) {
            auto node = voxel_queue.front();
            voxel_queue.pop();
            if (visited.count(node) > 0) {
                continue;
            }
            visited.insert(node);
            cc.insert(node);
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    for (int k = -1; k <= 1; k++) {
                        if (i == 0 && j == 0 && k == 0) {
                            continue;
                        }
                        Coord neighbor({node[0]+i,node[1]+j,node[2]+k});
                        if (visited.count(neighbor) == 0 && cr.count(neighbor) > 0) {
                            voxel_queue.push(neighbor);
                        }
                    }
                }
            }
        }
        cc_sets.push_back(cc);
    }
    return cc_sets;
}


template <typename Ts, typename ... Ta>
void traverseSegments(const Ts& seg, bool overlappingChunk, Ta& ... extractors)
{
    auto base = seg.index_bases();
    auto shape = seg.shape();
    auto c = Coord({0,0,0});
    for (int z = base[2]; z != base[2]+shape[2]; z++) {
        c[2] = z;
        for (int y = base[1]; y != base[1]+shape[1]; y++) {
            c[1] = y;
            for (int x = base[0]; x != base[0]+shape[0]; x++) {
                if (seg[x][y][z] == 0) {
                    continue;
                }
                c[0] = x;
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
