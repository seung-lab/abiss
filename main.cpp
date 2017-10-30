//#pragma once
#include "agglomeration.hpp"
#include "region_graph.hpp"
#include "basic_watershed.hpp"
#include "types.hpp"
#include "utils.hpp"


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

int main()
{
    // load the ground truth and the affinity graph

    clock_t begin = clock();
    affinity_graph_ptr<float> aff =
        read_affinity_graph<float>("aff.dat",
                                   4801, 3751, 192);
                                   //2050, 2050, 258);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "loaded affinity map in " << elapsed_secs << " seconds" << std::endl;

    volume_ptr<uint32_t>     seg   ;
    std::vector<std::size_t> counts;

    begin = clock();
    std::tie(seg , counts) = watershed<uint32_t>(aff, 0.0001, 0.999988);
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
    merge_segments(seg, rg, counts, tholds, 600);
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
