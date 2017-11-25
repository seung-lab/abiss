//#pragma once
#include "defines.h"
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
#include <cstddef>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <tuple>
#include <vector>
#include <chrono>
#include <ctime>
#include <boost/format.hpp>

int main(int argc, char* argv[])
{
    // load the ground truth and the affinity graph

    size_t xdim,ydim,zdim;
    int flag;
    uint64_t offset;
    std::ifstream param_file(argv[1]);
    const char * tag = argv[3];
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
    affinity_graph_ptr<float> aff = aff_data.data_ptr();
    //    read_affinity_graph<float>(argv[2],
    //                               xdim, ydim, zdim);
    //                               //2050, 2050, 258);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "loaded affinity map in " << elapsed_secs << " seconds" << std::endl;

    volume_ptr<uint64_t>     seg   ;
    std::vector<std::size_t> counts;

    begin = clock();
    std::tie(seg , counts) = watershed<uint64_t>(aff, LOW_THRESHOLD, HIGH_THRESHOLD, flags);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "finished watershed in " << elapsed_secs << " seconds" << std::endl;
    begin = clock();
    auto rg = get_region_graph(aff, seg , counts.size()-1, LOW_THRESHOLD, flags);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "finished region graph in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    merge_segments(seg, rg, counts, std::make_pair(SIZE_THRESHOLD, LOW_THRESHOLD), SIZE_THRESHOLD, offset);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

    std::cout << "finished agglomeration in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    write_volume(str(boost::format("seg_%1%.data") % tag), seg);
    write_chunk_boundaries(seg, aff, flags, tag);
    auto c = write_counts(counts, offset, tag);
    auto d = write_vector(str(boost::format("dend_%1%.data") % tag), *rg);
    std::vector<size_t> meta({xdim,ydim,zdim,c,d,0});
    write_vector(str(boost::format("meta_%1%.data") % tag), meta);
    std::cout << "num of sv:" << c << std::endl;
    std::cout << "size of rg:" << d << std::endl;
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "finished writing in " << elapsed_secs << " seconds" << std::endl;

    return 0;

}
