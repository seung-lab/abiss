#include "defines.h"
#include "types.hpp"
#include "utils.hpp"
#include "mmap_array.hpp"
#include <vector>
#include <tuple>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <boost/pending/disjoint_sets.hpp>

template<typename T>
std::unordered_map<T,T> generate_remap(std::string filename, size_t data_size)
{
    std::unordered_map<T,T> parent_map(data_size);

    if (data_size > 0) {
        MMArray<std::pair<T, T>, 1> remap_data(filename, std::array<size_t, 1>({data_size}));
        auto remap = remap_data.data();
        for (size_t i = data_size; i != 0; i--) {
            //std::cout << "i:" << i << std::endl;
            T s1 = remap[i-1].first;
            T s2 = remap[i-1].second;
            if (parent_map.count(s2) == 0) {
                parent_map[s1] = s2;
            } else {
                parent_map[s1] = parent_map[s2];
            }
        }
    }
    return parent_map;
}

template<typename T>
void update_supervoxels(const std::unordered_map<T, T> & remaps, size_t data_size, const std::string & filename)
{
    MMArray<T, 1> vol_array(filename,std::array<size_t, 1>({data_size}));
    auto vol = vol_array.data();
    for (size_t i = 0; i != data_size; i++) {
        if (remaps.count(vol[i]) > 0) {
            vol[i] = remaps.at(vol[i]);
        }
    }
}

int main(int argc, char* argv[])
{
    size_t xdim,ydim,zdim;
    size_t remap_size;
    std::ifstream param_file(argv[1]);
    param_file >> xdim >> ydim >> zdim;
    std::cout << xdim << " " << ydim << " " << zdim << std::endl;
    param_file >> remap_size;
    if (remap_size > 0) {
        auto map = generate_remap<seg_t>("remap.data", remap_size);
        std::cout << "generated remap" << std::endl;
        update_supervoxels(map, xdim*ydim*zdim, argv[2]);
    } else {
        std::cout << "nothing to remap" << std::endl;
    }

}

