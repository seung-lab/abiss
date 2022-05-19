#include "Types.h"
#include "Utils.hpp"
#include "SizeExtractor.hpp"
#include "AffinityExtractorME.hpp"
#include "ChunkedRGExtractor.hpp"
#include "BoundaryExtractor.hpp"
#include "SizeExtractor.hpp"
#include "COMExtractor.hpp"
#include "SemExtractor.hpp"
#include "BBoxExtractor.hpp"
#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include <cassert>
#include <cstdint>
#include <array>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

namespace bio = boost::iostreams;

int main(int argc, char * argv[])
{
    std::array<int64_t, 3> offset({0,0,0});
    std::array<int64_t, 3> dim({0,0,0});
    size_t ac_offset;

    //FIXME: check the input is valid
    std::ifstream param_file(argv[1]);
    param_file >> offset[0] >> offset[1] >> offset[2];
    param_file >> dim[0] >> dim[1] >> dim[2];
    param_file >> ac_offset;
    std::cout << offset[0] << " " << offset[1] << " " << offset[2] << std::endl;
    std::cout << dim[0] << " " << dim[1] << " " << dim[2] << std::endl;
    std::string chunk_tag(argv[2]);

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

    auto map = loadChunkMap<seg_t>("chunkmap.data");

    AffinityExtractorME<seg_t, aff_t, ConstChunkRef<aff_t, 4> > affinity_extractor(aff_chunk);
    ChunkedRGExtractor<seg_t, aff_t, ConstChunkRef<aff_t, 4> > chunked_rg_extractor(aff_chunk);
    BoundaryExtractor<seg_t> boundary_extractor;
    COMExtractor<seg_t> com_extractor;

    //traverseSegments(seg_chunk, true, boundary_extractor, affinity_extractor);
#ifdef EXTRACT_SIZE
    SizeExtractor<seg_t> size_extractor;
#endif
#ifdef EXTRACT_BBOX
    BBoxExtractor<seg_t, int64_t> bbox_extractor;
#endif

    if (std::filesystem::exists("sem.raw")) {
        bio::mapped_file_source sem_file;
        size_t sem_bytes = sizeof(semantic_t)*dim[0]*dim[1]*dim[2];
        sem_file.open("sem.raw", sem_bytes);
        assert(sem_file.is_open());
        ConstChunkRef<semantic_t, 3> sem_chunk(reinterpret_cast<const semantic_t*>(sem_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])], boost::fortran_storage_order());
        std::cout << "mmap sem data" << std::endl;
        SemExtractor<seg_t, semantic_t, ConstChunkRef<semantic_t, 3> > sem_extractor(sem_chunk);
        traverseSegments<1>(seg_chunk, boundary_extractor, affinity_extractor, sem_extractor, chunked_rg_extractor, com_extractor
#ifdef EXTRACT_SIZE
                     ,size_extractor
#endif
#ifdef EXTRACT_BBOX
                     ,bbox_extractor
#endif
                     );
        sem_extractor.output(map, "ongoing_semantic_labels.data");
    } else {
        traverseSegments<1>(seg_chunk, boundary_extractor, affinity_extractor, chunked_rg_extractor, com_extractor
#ifdef EXTRACT_SIZE
                     ,size_extractor
#endif
#ifdef EXTRACT_BBOX
                     ,bbox_extractor
#endif
                     );
    }

    auto incomplete_segments = boundary_extractor.incompleteSupervoxels(map);

    boundary_extractor.output("boundary_%1%_"+chunk_tag+".data", map);

    affinity_extractor.output(incomplete_segments, map, "edges_"+chunk_tag+".data", "incomplete_edges_"+chunk_tag+".data");
    chunked_rg_extractor.output(map, chunk_tag, ac_offset);

    chunkedOutput(com_extractor.com(), "chunked_rg/com", chunk_tag, ac_offset);

#ifdef EXTRACT_SIZE
    size_extractor.output(map, "ongoing_seg_size.data");
    auto sv_sizes = size_extractor.svSizes(map);
    std::ofstream nsfile("ns.data", std::ios_base::binary);
    assert(nsfile.is_open());
    for (const auto & [k,v] : sv_sizes) {
        nsfile.write(reinterpret_cast<const char *>(&k), sizeof(k));
        nsfile.write(reinterpret_cast<const char *>(&v), sizeof(v));
        assert(!nsfile.bad());
    }
    nsfile.close();
#endif

#ifdef EXTRACT_BBOX
    bbox_extractor.output(incomplete_segments, "bboxes.data", "incomplete_bboxes_"+chunk_tag+".data");
#endif

    std::cout << "finished" << std::endl;
    aff_file.close();
    seg_file.close();
}
