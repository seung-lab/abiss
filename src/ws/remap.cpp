#include "defines.h"
#include "types.hpp"
#include "utils.hpp"
#include "mmap_array.hpp"
#include "../seg/RemapTable.hpp"
#include <vector>
#include <tuple>
#include <iostream>
#include <unordered_map>
#include <unordered_set>


template<typename T>
ChunkRemap<T> generate_remap(std::string filename, size_t data_size)
{
    ChunkRemap<T> chunk_remap(data_size);

    if (data_size > 0) {
        MMArray<std::pair<T, T>, 1> remap_data(filename, std::array<size_t, 1>({data_size}));
        auto remap = remap_data.data();
        for (size_t i = data_size; i != 0; i--) {
            //std::cout << "i:" << i << std::endl;
            T s1 = remap[i-1].first;
            T s2 = remap[i-1].second;

            chunk_remap.updateRemap(s1, s2);
        }
    }
    return chunk_remap;
}

template<typename T>
void update_supervoxels(const ChunkRemap<T> & remaps, size_t data_size, const std::string & filename)
{
    MMArray<T, 1> vol_array(filename,std::array<size_t, 1>({data_size}));
    auto vol = vol_array.data();
    for (size_t i = 0; i != data_size; i++) {
        vol[i] = remaps.chunkID(vol[i]);
    }
}

int main(int argc, char* argv[])
{
    size_t xdim,ydim,zdim;
    size_t remap_size;
    size_t local;
    std::ifstream param_file(argv[1]);
    param_file >> xdim >> ydim >> zdim;
    std::cout << xdim << " " << ydim << " " << zdim << std::endl;
    param_file >> remap_size;
    param_file >> local;
    if (remap_size > 0) {
        auto map = generate_remap<seg_t>("remap.data", remap_size);
        std::cout << "generated remap" << std::endl;
        if (local == 1) {
            map.generateChunkMap();
        }
        auto v = map.reversedChunkMapVector();
        std::cout << "chunkmap size:" << v.size() << std::endl;
        write_vector("chunkmap.data", v);
        update_supervoxels(map, xdim*ydim*zdim, argv[2]);
    } else {
        std::cout << "nothing to remap" << std::endl;
    }
}
