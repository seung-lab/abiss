
#include "ws/basic_watershed.hpp"
#include "ws/region_graph.hpp"
#include "ws/agglomeration.hpp"
#include "ws/utils.hpp"
#include "abiss_wrapper.h"

std::vector<std::size_t> run_watershed(
        std::size_t     width,
        std::size_t     height,
        std::size_t     depth,
        const aff_t*    affinity_data,
        seg_t*          seg_out,
        std::size_t     size_threshold,
        aff_t           aff_threshold_low,
        aff_t           aff_threshold_high) {

    std::shared_ptr<affinity_graph<aff_t>> affs_ptr(
        new affinity_graph<aff_t> (
            const_cast<aff_t*>(affinity_data),  // remove const for compatibility
            boost::extents[width][height][depth][3],
            boost::fortran_storage_order()
        ),
        // remove deleter because the obj is actually owned by Python
        [](affinity_graph<aff_t>*){}
    );

    // Run basic watershed
    volume_ptr<seg_t> ws_out;
    std::vector<std::size_t> counts;
    std::array<bool, 6> boundary_flags {true, true, true, true, true, true};
    std::tie(ws_out, counts) = watershed<seg_t>(affs_ptr, aff_threshold_low, aff_threshold_high, boundary_flags);

    // Run size thresholding
    if (size_threshold > 0) {
        auto rg = get_region_graph<seg_t>(
            affs_ptr, ws_out, counts.size()-1, aff_threshold_low, boundary_flags);
        merge_segments(ws_out, rg, counts, std::make_pair(size_threshold, aff_threshold_low), size_threshold);
    }

    // Copy output to a numpy allocated array
    for (uint64_t i = 0; i < depth*height*width; ++i)
        seg_out[i] = ws_out->data()[i];

    free_container(ws_out);
    return counts;
}
