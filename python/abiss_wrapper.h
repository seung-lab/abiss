#ifndef ABISS_WRAPPER_H
#define ABISS_WRAPPER_H

using seg_t = uint64_t;
using aff_t = float;

std::vector<std::size_t> run_watershed(
        size_t          width,
        size_t          height,
        size_t          depth,
        const float*    affinity_data,
        uint64_t*       segmentation_data,
        size_t          size_threshold,
        float           aff_threshold_low,
        float           aff_threshold_high);

#endif
