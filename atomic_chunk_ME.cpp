#include "Types.h"
#include "Utils.hpp"
#include "SizeExtractor.hpp"
#include "AffinityExtractorME.hpp"
#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include <cstdint>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

namespace bio = boost::iostreams;

template<typename Ta, typename Ts>
std::unordered_set<Ts> processMetaData(const AffinityExtractorME<Ts, Ta, ConstChunkRef<Ta,4> > & affinityExtractor, const char * tag)
{
    std::unordered_set<Ts> incomplete_segments;
    for (int i= 0; i != 6; i++) {
        std::ofstream out(str(boost::format("boundary_%1%_%2%.data") % i % tag), std::ios_base::binary);
        for (auto x : affinityExtractor.boundarySupervoxels(i)) {
            incomplete_segments.insert(x);
            //out << x << "\n";
            out.write(reinterpret_cast<const char *>(&x), sizeof(x));
        }
        out.close();
    }
    return incomplete_segments;
}

template<typename Ta, typename Ts>
void processData(const AffinityExtractorME<Ts, Ta, ConstChunkRef<Ta,4> > & affinityExtractor, const char * path)
{
    using namespace std::placeholders;

    auto incomplete_segments = processMetaData(affinityExtractor, path);

    std::vector<SegPair<Ts> > incomplete_segpairs;
    std::vector<SegPair<Ts> > complete_segpairs;
    const auto & edges = affinityExtractor.edges();

    std::ofstream incomplete(str(boost::format("incomplete_edges_%1%.data") % path), std::ios_base::binary);
    std::ofstream complete(str(boost::format("complete_edges_%1%.data") % path), std::ios_base::binary);

    std::vector<std::pair<Ta, Ts> > me_complete;
    std::vector<std::pair<Ta, Ts> > me_incomplete;
    for (const auto & kv : edges) {
        const auto k = kv.first;
        if (incomplete_segments.count(k.first) > 0 && incomplete_segments.count(k.second) > 0) {
            incomplete_segpairs.push_back(k);
            me_incomplete.push_back(kv.second);
        } else {
            complete_segpairs.push_back(k);
            me_complete.push_back(kv.second);
        }
    }

    for (size_t i = 0; i != complete_segpairs.size(); i++) {
        const auto & p = complete_segpairs[i];
        complete.write(reinterpret_cast<const char *>(&(p.first)), sizeof(Ts));
        complete.write(reinterpret_cast<const char *>(&(p.second)), sizeof(Ts));
        complete.write(reinterpret_cast<const char *>(&(me_complete[i].first)), sizeof(Ta));
        complete.write(reinterpret_cast<const char *>(&(me_complete[i].second)), sizeof(size_t));
        complete.write(reinterpret_cast<const char *>(&(p.first)), sizeof(Ts));
        complete.write(reinterpret_cast<const char *>(&(p.second)), sizeof(Ts));
        complete.write(reinterpret_cast<const char *>(&(me_complete[i].first)), sizeof(Ta));
        complete.write(reinterpret_cast<const char *>(&(me_complete[i].second)), sizeof(size_t));
    }
    for (size_t i = 0; i != incomplete_segpairs.size(); i++) {
        const auto & p = incomplete_segpairs[i];
        incomplete.write(reinterpret_cast<const char *>(&(p.first)), sizeof(Ts));
        incomplete.write(reinterpret_cast<const char *>(&(p.second)), sizeof(Ts));
        incomplete.write(reinterpret_cast<const char *>(&(me_incomplete[i].first)), sizeof(Ta));
        incomplete.write(reinterpret_cast<const char *>(&(me_incomplete[i].second)), sizeof(size_t));
    }
    incomplete.close();
    complete.close();
}

int main(int argc, char * argv[])
{
    std::array<int, 3> offset({0,0,0});
    std::array<int, 3> dim({0,0,0});

    //FIXME: check the input is valid
    std::ifstream param_file(argv[1]);
    param_file >> offset[0] >> offset[1] >> offset[2];
    param_file >> dim[0] >> dim[1] >> dim[2];
    std::cout << offset[0] << " " << offset[1] << " " << offset[2] << std::endl;
    std::cout << dim[0] << " " << dim[1] << " " << dim[2] << std::endl;
    const char * output_path = argv[2];

    //FIXME: wrap these in classes
    bio::mapped_file_source seg_file;
    size_t seg_bytes = sizeof(seg_t)*dim[0]*dim[1]*dim[2];
    seg_file.open("seg.raw", seg_bytes);
    ConstChunkRef<seg_t, 3> seg_chunk (reinterpret_cast<const seg_t*>(seg_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])], boost::fortran_storage_order());
    std::cout << "mmap seg data" << std::endl;

    bio::mapped_file_source aff_file;
    size_t aff_bytes = sizeof(aff_t)*dim[0]*dim[1]*dim[2]*3;
    aff_file.open("aff.raw", aff_bytes);
    ConstChunkRef<aff_t,4> aff_chunk (reinterpret_cast<const aff_t*>(aff_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])][3], boost::fortran_storage_order());
    std::cout << "mmap aff data" << std::endl;

    AffinityExtractorME<seg_t, aff_t, ConstChunkRef<aff_t, 4> > affinity_extractor(aff_chunk);
    traverseSegments(seg_chunk, true, affinity_extractor);
    processData(affinity_extractor, output_path);
    std::cout << "finished" << std::endl;
    aff_file.close();
    seg_file.close();
}
