from libcpp.vector cimport vector
from libc.stdint cimport uint64_t, uint32_t
import numpy as np
cimport numpy as np


def watershed(
        affs,  # assuming XYZC
        aff_threshold_low=0.01,
        aff_threshold_high=0.99,
        size_threshold=50,
        # agglomeration_threshold=0.0,
        ):

    if not affs.flags['F_CONTIGUOUS']:
        print("Creating memory-contiguous affinity array (avoid this by passing F_CONTIGUOUS arrays)")
        affs = np.asfortranarray(affs)

    volume_shape = (affs.shape[0], affs.shape[1], affs.shape[2])  # XYZ
    segmentation = np.asfortranarray(np.zeros(volume_shape, dtype=np.uint64))

    # _watershed(affs, segmentation, aff_threshold_low, aff_threshold_high, size_threshold, agglomeration_threshold)
    _watershed(affs, segmentation, aff_threshold_low, aff_threshold_high, size_threshold)
    return segmentation  # XYZ


def _watershed(
        np.ndarray[np.float32_t, ndim=4] affs,
        np.ndarray[uint64_t, ndim=3]     segmentation,
        aff_threshold_low=0.01,
        aff_threshold_high=0.99,
        size_threshold=50,
        # agglomeration_threshold=0.0,
        ):

    cdef float*    aff_data
    cdef uint64_t* segmentation_data
    cdef vector[size_t] sv_sizes

    aff_data = &affs[0,0,0,0]
    segmentation_data = &segmentation[0,0,0]
    width, height, depth = affs.shape[0], affs.shape[1], affs.shape[2]

    with nogil:
        sv_sizes = run_watershed(
            width, height, depth,
            aff_data,
            segmentation_data,
            size_threshold,
            aff_threshold_low,
            aff_threshold_high)

    # if agglomeration_threshold > 0.0:
    #     rg = extract_region_graph(
    #         width, height, depth,
    #         aff_data,
    #         segmentation_data)
    #     remaps = run_agglomeration(rg, sv_sizes, agglomeration_threshold)
    #     run_update_segmentation(
    #         width, height, depth,
    #         segmentation_data,
    #         remaps)

    return

cdef extern from "abiss_wrapper.h" nogil:
    vector[size_t] run_watershed(
            size_t          width,
            size_t          height,
            size_t          depth,
            const float*    affinity_data,
            uint64_t*       segmentation_data,
            size_t          size_threshold,
            float           affThresholdLow,
            float           affThresholdHigh,
            )
