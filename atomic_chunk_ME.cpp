#include "Types.h"
#include "Utils.hpp"
#include "SizeExtractor.hpp"
#include "AffinityExtractorME.hpp"
#include "BoundaryExtractor.hpp"
#include "SizeExtractor.hpp"
#include "BBoxExtractor.hpp"
#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include <cassert>
#include <cstdint>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

namespace bio = boost::iostreams;

int main(int argc, char * argv[])
{
    std::array<int64_t, 3> offset({0,0,0});
    std::array<int64_t, 3> dim({0,0,0});

    //FIXME: check the input is valid
    std::ifstream param_file(argv[1]);
    param_file >> offset[0] >> offset[1] >> offset[2];
    param_file >> dim[0] >> dim[1] >> dim[2];
    std::cout << offset[0] << " " << offset[1] << " " << offset[2] << std::endl;
    std::cout << dim[0] << " " << dim[1] << " " << dim[2] << std::endl;
    std::string output_path(argv[2]);

    //FIXME: wrap these in classes
    bio::mapped_file_source seg_file;
    size_t seg_bytes = sizeof(seg_t)*dim[0]*dim[1]*dim[2];
    seg_file.open("seg.raw", seg_bytes);
    assert(seg_file.is_open());
    ConstChunkRef<seg_t, 3> seg_chunk (reinterpret_cast<const seg_t*>(seg_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])], boost::fortran_storage_order());
    std::cout << "mmap seg data" << std::endl;

    bio::mapped_file_source aff_file;
    size_t aff_bytes = sizeof(aff_t)*dim[0]*dim[1]*dim[2]*3;
    aff_file.open("aff.raw", aff_bytes);
    assert(aff_file.is_open());
    ConstChunkRef<aff_t, 4> aff_chunk (reinterpret_cast<const aff_t*>(aff_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])][3], boost::fortran_storage_order());
    std::cout << "mmap aff data" << std::endl;

    AffinityExtractorME<seg_t, aff_t, ConstChunkRef<aff_t, 4> > affinity_extractor(aff_chunk);
    BoundaryExtractor<seg_t> boundary_extractor;

    //traverseSegments(seg_chunk, true, boundary_extractor, affinity_extractor);
#ifdef EXTRACT_SIZE
    SizeExtractor<seg_t> size_extractor;
#endif
#ifdef EXTRACT_BBOX
    BBoxExtractor<seg_t, int64_t> bbox_extractor;
#endif

    traverseSegments(seg_chunk, true, boundary_extractor, affinity_extractor
#ifdef EXTRACT_SIZE
                     ,size_extractor
#endif
#ifdef EXTRACT_BBOX
                     ,bbox_extractor
#endif
                     );

    auto incomplete_segments = boundary_extractor.incompleteSupervoxels();

    boundary_extractor.output("boundary_%1%_"+output_path+".data");

    affinity_extractor.output(incomplete_segments, "edges_"+output_path+".data", "incomplete_edges_"+output_path+".data");

#ifdef EXTRACT_SIZE
    size_extractor.output(incomplete_segments, "sizes.data", "incomplete_sizes_"+output_path+".data");
#endif

#ifdef EXTRACT_BBOX
    bbox_extractor.output(incomplete_segments, "bboxes.data", "incomplete_bboxes_"+output_path+".data");
#endif

    std::cout << "finished" << std::endl;
    aff_file.close();
    seg_file.close();
}
