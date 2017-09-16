#include "Types.h"
#include "Utils.hpp"
#include "SizeExtractor.hpp"
#include "AffinityExtractor.hpp"
#include "MeanEdge.hpp"
#include <cstdint>
#include <array>
#include <fstream>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

namespace bio = boost::iostreams;

template<typename Ta, typename Ts>
std::unordered_set<Ts> processMetaData(const AffinityExtractor<Ts, Ta, ConstChunkRef<Ta,4> > & affinityExtractor, const char * tag)
{
    std::unordered_set<Ts> incomplete_segments;
    for (int i= 0; i != 6; i++) {
        std::ofstream out(str(boost::format("boundary_%1%_%2%.dat") % i % tag), std::ios_base::binary);
        for (auto x : affinityExtractor.boundarySupervoxels(i)) {
            incomplete_segments.insert(x);
            out << x << "\n";
            //out.write(reinterpret_cast<const char *>(&x), sizeof(x));
        }
        out.close();
    }
    return incomplete_segments;
}

template<typename Ta, typename Ts>
void writeEdge(const SegPair<Ts> & p, const Edge<Ta> & edge, const char * path)
{
    std::ofstream out(str(boost::format("%1%/%2%_%3%.dat") % path % p.first % p.second), std::ios_base::binary);
    for (int i = 0; i != 3; i++) {
        for (auto kv : edge[i]) {
            auto k = kv.first;
            auto v = kv.second;
            out << i << " ";
            out << k[0] << " " << k[1] << " " << k[2] << " ";
            out << v << "\n";
        }
    }
    out.close();
}

template<typename Ta, typename Ts>
void processData(const AffinityExtractor<Ts, Ta, ConstChunkRef<Ta,4> > & affinityExtractor, const char * path)
{
    auto incomplete_segments = processMetaData(affinityExtractor, path);
    std::ofstream incomplete(str(boost::format("incomplete_edges_%1%.dat") % path), std::ios_base::binary);
    std::ofstream complete(str(boost::format("complete_edges_%1%.dat") % path), std::ios_base::binary);
    for (auto kv : affinityExtractor.edges()) {
        auto p = kv.first;
        if (incomplete_segments.count(p.first) > 0 && incomplete_segments.count(p.second) > 0) {
            incomplete << p.first << " " << p.second << "\n";
            writeEdge(p, kv.second, path);
        } else {
            auto me = meanAffinity<float, int>(kv.second);
            complete << p.first << " " << p.second << " " << me.first << " " << me.second << std::endl;
        }
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
    size_t seg_bytes = sizeof(int32_t)*dim[0]*dim[1]*dim[2];
    seg_file.open("seg.raw", seg_bytes);
    ConstChunkRef<int32_t,3> seg_chunk (reinterpret_cast<const int32_t*>(seg_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])], boost::fortran_storage_order());
    std::cout << "mmap seg data" << std::endl;

    bio::mapped_file_source aff_file;
    size_t aff_bytes = sizeof(float)*dim[0]*dim[1]*dim[2]*3;
    aff_file.open("aff.raw", aff_bytes);
    ConstChunkRef<float,4> aff_chunk (reinterpret_cast<const float*>(aff_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])][3], boost::fortran_storage_order());
    std::cout << "mmap aff data" << std::endl;

    AffinityExtractor<int32_t, float, ConstChunkRef<float,4> > affinity_extractor(aff_chunk);
    traverseSegments(seg_chunk, true, affinity_extractor);
    processData(affinity_extractor, output_path);
    std::cout << "finished" << std::endl;
    aff_file.close();
    seg_file.close();
}
