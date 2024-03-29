#include "Types.h"
#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include "BoundaryExtractor.hpp"
#include "AffinityExtractorME.hpp"
#include "SizeExtractor.hpp"
#include "BBoxExtractor.hpp"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>

int main(int argc, char * argv[])
{
    std::string tag(argv[1]);

#ifdef USE_MIMALLOC
    auto rg_size = filesize(str(boost::format("incomplete_edges_%1%.tmp") % tag));
    size_t huge_pages = rg_size * 4 / 1024 / 1024 / 1024 + 1;
    auto mi_ret = mi_reserve_huge_os_pages_interleave(huge_pages, 0, 0);
    if (mi_ret == ENOMEM) {
       std::cout << "failed to reserve 1GB huge pages" << std::endl;
    }
#endif

    SetContainer<seg_t> incomplete_segments;
    for (size_t i = 0; i != 6; i++) {
        incomplete_segments.merge(updateBoundarySegments<seg_t>(i, tag));
    }
    std::cout << incomplete_segments.size() << " incomplete segments" << std::endl;

    updateRegionGraph<seg_t, aff_t>(incomplete_segments,
            str(boost::format("incomplete_edges_%1%.tmp") % tag),
            str(boost::format("incomplete_edges_%1%.data") % tag),
            "new_edges.data");
    for  (int i = 2; i != argc; i++) {
        auto k = std::string(argv[i]);
        std::cout << "processing: " << k << std::endl;
#ifdef EXTRACT_SIZE
        if (k == "sizes") {
            updateSizes<seg_t>(incomplete_segments,
                    str(boost::format("incomplete_sizes_%1%.tmp") % tag),
                    str(boost::format("incomplete_sizes_%1%.data") % tag),
                    "sizes.data");
        }
#endif
#ifdef EXTRACT_BBOX
        if (k == "bboxes") {
            updateBBoxes<seg_t, int64_t>(incomplete_segments,
                    str(boost::format("incomplete_bboxes_%1%.tmp") % tag),
                    str(boost::format("incomplete_bboxes_%1%.data") % tag),
                    "bboxes.data");
        }
#endif
    }
}
