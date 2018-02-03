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
    using rank_t = std::unordered_map<T,std::size_t>;
    using parent_t = std::unordered_map<T,T>;;
    rank_t rank_map(data_size);
    parent_t parent_map(data_size);

    boost::associative_property_map<rank_t> rank_pmap(rank_map);
    boost::associative_property_map<parent_t> parent_pmap(parent_map);

    boost::disjoint_sets<boost::associative_property_map<rank_t>, boost::associative_property_map<parent_t> > sets(rank_pmap, parent_pmap);
    MMArray<std::pair<T, T>, 1> remap_data(filename, std::array<size_t, 1>({data_size}));
    auto remap = remap_data.data();
    std::unordered_set<T> nodes(data_size);
    for (size_t i = 0; i != data_size; i++) {
        if ((i+1) % 100000000 == 0) {
            std::cout << i << " remaps processed" << std::endl;
        }
        T s1 = remap[i].first;
        T s2 = remap[i].second;
        if (nodes.count(s1) == 0) {
            nodes.insert(s1);
            sets.make_set(s1);

        }
        if (nodes.count(s2) == 0) {
            nodes.insert(s2);
            sets.make_set(s2);

        }
        sets.link(s1, s2);
    }
    sets.compress_sets(std::begin(nodes), std::end(nodes));
    auto count_segs = sets.count_sets(std::begin(nodes), std::end(nodes));
    std::unordered_map<T, T> normalized_root(count_segs);
    for (size_t i = 0; i != data_size; i++) {
        T s1 = remap[i].first;
        T s2 = remap[i].second;
        T p = parent_map[s2];
        if (normalized_root[p] == 0) {
            normalized_root[p] = s2;
            count_segs--;
        }
        if (count_segs == 0) {
            break;
        }
    }
    for (auto & k : nodes) {
        parent_map[k] = normalized_root[parent_map[k]];
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
    auto map = generate_remap<seg_t>("remap.data", remap_size);
    std::cout << "generated remap" << std::endl;
    update_supervoxels(map, xdim*ydim*zdim, argv[2]);

}

