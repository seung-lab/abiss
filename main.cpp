//#pragma once
#include "agglomeration.hpp"
#include "region_graph.hpp"
#include "basic_watershed.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "mmap_array.hpp"


#include <memory>
#include <type_traits>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>
#include <algorithm>
#include <tuple>
#include <map>
#include <list>
#include <set>
#include <vector>
#include <chrono>
#include <ctime>
#include <cstdint>

int main(int argc, char* argv[])
{
    // load the ground truth and the affinity graph

    size_t xdim,ydim,zdim;
    int flag;
    uint64_t offset;
    std::ifstream param_file(argv[1]);
    param_file >> xdim >> ydim >> zdim;
    std::cout << xdim << " " << ydim << " " << zdim << std::endl;
    std::array<bool,6> flags({true,true,true,true,true,true});
    for (size_t i = 0; i != 6; i++) {
        param_file >> flag;
        flags[i] = (flag > 0);
        if (flags[i]) {
            std::cout << "real boundary: " << i << std::endl;
        }
    }
    param_file >> offset;
    std::cout << "supervoxel id offset:" << offset << std::endl;

    clock_t begin = clock();
    std::array<size_t, 4> aff_dim({xdim,ydim,zdim,3});
    MMArray<float, 4> aff_data(argv[2], aff_dim);
    affinity_graph_ptr<float> aff = aff_data.data();
    //    read_affinity_graph<float>(argv[2],
    //                               xdim, ydim, zdim);
    //                               //2050, 2050, 258);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "loaded affinity map in " << elapsed_secs << " seconds" << std::endl;

    volume_ptr<uint64_t>     seg   ;
    std::vector<std::size_t> counts;

    begin = clock();
    std::tie(seg , counts) = watershed<uint32_t>(aff, 0.0001, 0.999988, flags);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "finished watershed in " << elapsed_secs << " seconds" << std::endl;
    begin = clock();
    auto rg = get_region_graph(aff, seg , counts.size()-1, 0.0001);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "finished region graph in " << elapsed_secs << " seconds" << std::endl;

    std::vector<std::pair<std::size_t, double>> tholds;
    tholds.push_back(std::make_pair(600,0.0001));

    begin = clock();
    merge_segments(seg, rg, counts, tholds, 600, offset);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

    std::cout << "finished agglomeration in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    write_volume("seg.dat", seg);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "finished writing in " << elapsed_secs << " seconds" << std::endl;

    return 0;

}
