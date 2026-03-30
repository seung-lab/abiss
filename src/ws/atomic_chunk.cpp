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
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <cctype>
#include <vector>
#include <algorithm>
#include <tuple>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <boost/format.hpp>

using internal_seg_t = uint32_t;

template <typename IT, typename OT>
volume_ptr<OT> relabel_segments(volume_ptr<IT> in_ptr, OT offset)
{
    auto xdim = in_ptr->shape()[0];
    auto ydim = in_ptr->shape()[1];
    auto zdim = in_ptr->shape()[2];
    size_t chunk_size = xdim * ydim * zdim;
    auto out_ptr =  volume_ptr<OT>(new volume<OT>(boost::extents[xdim][ydim][zdim], boost::fortran_storage_order()));
    auto in_data = in_ptr->data();
    auto out_data = out_ptr->data();
    for (size_t i = 0; i < chunk_size; i++) {
        out_data[i] = in_data[i] != 0 ? static_cast<OT>(in_data[i])+offset : 0;
    }
    return out_ptr;
}

template< typename IT, typename OT, typename F>
region_graph<OT, F> relabel_region_graph(region_graph<IT, F> rg, OT offset)
{
    region_graph<OT, F> new_rg;
    for (auto & e : rg) {
        new_rg.emplace_back(std::get<0>(e), static_cast<OT>(std::get<1>(e)) + offset, static_cast<OT>(std::get<2>(e)) + offset);
    }
    return new_rg;
}

int main(int argc, char* argv[])
{
    // load the ground truth and the affinity graph

    size_t xdim,ydim,zdim;
    int flag;
    seg_t offset;
    std::ifstream param_file(argv[1]);
    std::string ht(argv[3]);
    std::string lt(argv[4]);
    std::string st(argv[5]);
    std::string dt(argv[6]);
    const char * tag = argv[7];

    // Parse optional merge function and merge thresholds from argv[8..N].
    // When multiple thresholds are given, watershed + region graph are
    // computed once and the merge step is repeated for each threshold,
    // writing indexed output files.
    //
    // CLI format: ws param aff high low size dust tag [merge_func] [thresholds...]
    // merge_func: "max" (default), "mean", or "pNN" (e.g. "p75", "p90")
    std::vector<aff_t> merge_thresholds;
    auto high_threshold = read_float<aff_t>(ht);
    auto low_threshold = read_float<aff_t>(lt);
    auto size_threshold = read_int(st);
    auto dust_threshold = read_int(dt);

    EdgeScoreConfig score_cfg;  // default: MAX
    int merge_thresh_start = 8;

    // If argv[8] starts with a letter, it's a merge function spec.
    // Merge thresholds start with a digit, dot, or minus.
    if (argc > 8 && std::isalpha(static_cast<unsigned char>(argv[8][0]))) {
        std::string merge_func_str(argv[8]);
        score_cfg = parse_edge_score_config(merge_func_str);
        merge_thresh_start = 9;
        std::cout << "merge function: " << merge_func_str << std::endl;
    }

    if (argc > merge_thresh_start) {
        for (int i = merge_thresh_start; i < argc; i++) {
            std::string ms(argv[i]);
            merge_thresholds.push_back(read_float<aff_t>(ms));
        }
    } else {
        merge_thresholds.push_back(low_threshold);
    }

    std::cout << "thresholds: " << ht << " " << lt << " " << st << " " << dt
              << " merge=[";
    for (size_t i = 0; i < merge_thresholds.size(); i++) {
        if (i > 0) std::cout << ",";
        std::cout << merge_thresholds[i];
    }
    std::cout << "]" << std::endl;

    param_file >> xdim >> ydim >> zdim;
    std::cout << xdim << " " << ydim << " " << zdim << std::endl;

#ifdef USE_MIMALLOC
    size_t huge_pages = xdim * ydim * zdim * 4 * 3 * 4 / 1024 / 1024 / 1024 + 1;
    auto mi_ret = mi_reserve_huge_os_pages_interleave(huge_pages, 0, 0);
    if (mi_ret == ENOMEM) {
       std::cout << "failed to reserve 1GB huge pages" << std::endl;
    }
#endif

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

    size_t chunk_size = xdim * ydim * zdim;

    assert(chunk_size < static_cast<size_t>(watershed_traits<internal_seg_t>::high_bit));

    clock_t begin = clock();
    std::array<size_t, 4> aff_dim({xdim,ydim,zdim,3});
    MMArray<aff_t, 4> aff_data(argv[2], aff_dim);
    affinity_graph_ptr<aff_t> aff = aff_data.data_ptr();
    //    read_affinity_graph<float>(argv[2],
    //                               xdim, ydim, zdim);
    //                               //2050, 2050, 258);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "loaded affinity map in " << elapsed_secs << " seconds" << std::endl;

    volume_ptr<internal_seg_t> seg;
    std::vector<std::size_t> counts;

    begin = clock();
    std::tie(seg , counts) = watershed<internal_seg_t>(aff, low_threshold, high_threshold, flags);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "finished watershed in " << elapsed_secs << " seconds" << std::endl;
    begin = clock();
    auto rg = get_region_graph(aff, seg , counts.size()-1, low_threshold, flags, score_cfg);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "finished region graph in " << elapsed_secs << " seconds" << std::endl;

    if (merge_thresholds.size() == 1) {
        // ------ Single merge threshold: original behaviour ------
        begin = clock();
        merge_segments(seg, rg, counts, std::make_pair(size_threshold, merge_thresholds[0]), dust_threshold);
        end = clock();
        elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

        auto relabeled_seg = relabel_segments(seg, offset);
        free_container(seg);
        auto relabeled_rg = relabel_region_graph(rg, offset);
        free_container(rg);

        std::cout << "finished agglomeration in " << elapsed_secs << " seconds" << std::endl;
        auto c = write_counts(counts, offset, tag);
        free_container(counts);
        auto d = write_vector(str(boost::format("dend_%1%.data") % tag), relabeled_rg);
        free_container(relabeled_rg);
        begin = clock();
        write_volume(str(boost::format("seg_%1%.data") % tag), relabeled_seg);
        write_chunk_boundaries(relabeled_seg, aff, flags, tag);
        std::vector<size_t> meta({xdim,ydim,zdim,c,d,0});
        write_vector(str(boost::format("meta_%1%.data") % tag), meta);
        std::cout << "num of sv:" << c << std::endl;
        std::cout << "size of rg:" << d << std::endl;
        end = clock();
        elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
        std::cout << "finished writing in " << elapsed_secs << " seconds" << std::endl;
    } else {
        // ------ Multiple merge thresholds: reuse watershed + RG ------
        std::cout << "Multi-threshold mode: " << merge_thresholds.size()
                  << " merge thresholds" << std::endl;

        for (size_t mi = 0; mi < merge_thresholds.size(); mi++) {
            clock_t mt_begin = clock();

            // Deep-copy seg, rg, counts so merge_segments can modify them
            auto seg_copy = volume_ptr<internal_seg_t>(
                new volume<internal_seg_t>(
                    boost::extents[xdim][ydim][zdim],
                    boost::fortran_storage_order()));
            std::copy(seg->data(), seg->data() + chunk_size, seg_copy->data());
            auto rg_copy = rg;
            auto counts_copy = counts;

            std::string out_tag = str(boost::format("%1%_%2%") % tag % mi);

            merge_segments(seg_copy, rg_copy, counts_copy,
                           std::make_pair(size_threshold, merge_thresholds[mi]),
                           dust_threshold);

            auto relabeled_seg = relabel_segments(seg_copy, offset);
            free_container(seg_copy);
            auto relabeled_rg = relabel_region_graph(rg_copy, offset);
            free_container(rg_copy);

            auto c = write_counts(counts_copy, offset, out_tag.c_str());
            free_container(counts_copy);
            auto d = write_vector(str(boost::format("dend_%1%.data") % out_tag), relabeled_rg);
            free_container(relabeled_rg);

            write_volume(str(boost::format("seg_%1%.data") % out_tag), relabeled_seg);
            write_chunk_boundaries(relabeled_seg, aff, flags, out_tag.c_str());
            std::vector<size_t> meta({xdim,ydim,zdim,c,d,0});
            write_vector(str(boost::format("meta_%1%.data") % out_tag), meta);

            clock_t mt_end = clock();
            double mt_secs = double(mt_end - mt_begin) / CLOCKS_PER_SEC;
            std::cout << "merge threshold " << mi << " (" << merge_thresholds[mi]
                      << "): sv=" << c << " rg=" << d
                      << " in " << mt_secs << " seconds" << std::endl;
        }

        free_container(seg);
        free_container(rg);
        free_container(counts);
    }

    return 0;
}
