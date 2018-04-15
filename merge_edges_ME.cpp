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

    std::unordered_set<seg_t> incomplete_segments;
    for (size_t i = 0; i != 6; i++) {
        incomplete_segments.merge(updateBoundarySegments<seg_t>(i, tag));
    }
    std::cout << incomplete_segments.size() << " incomplete segments" << std::endl;
    updateRegionGraph<seg_t, aff_t>(incomplete_segments,
            str(boost::format("incomplete_edges_%1%.tmp") % tag),
            str(boost::format("incomplete_edges_%1%.data") % tag),
            "new_edges.data");
    updateSizes<seg_t>(incomplete_segments,
            str(boost::format("incomplete_sizes_%1%.tmp") % tag),
            str(boost::format("incomplete_sizes_%1%.data") % tag),
            "sizes.data");
    updateBBoxes<seg_t, int64_t>(incomplete_segments,
            str(boost::format("incomplete_bboxes_%1%.tmp") % tag),
            str(boost::format("incomplete_bboxes_%1%.data") % tag),
            "bboxes.data");

}
