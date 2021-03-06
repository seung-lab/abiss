#include "Types.h"
#include "Utils.hpp"
#include "SizeExtractor.hpp"
#include "AffinityExtractor.hpp"
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
#include <QtConcurrent>

namespace bio = boost::iostreams;

template<typename Ta, typename Ts>
SetContainer<Ts> processMetaData(const AffinityExtractor<Ts, Ta, ConstChunkRef<Ta,4> > & affinityExtractor, const char * tag)
{
    SetContainer<Ts> incomplete_segments;
    for (int i= 0; i != 6; i++) {
        std::ofstream out(str(boost::format("boundary_%1%_%2%.data") % i % tag), std::ios_base::binary);
        for (auto x : affinityExtractor.boundarySupervoxels(i)) {
            incomplete_segments.insert(x);
            //out << x << "\n";
            out.write(reinterpret_cast<const char *>(&x), sizeof(x));
        }
        assert(!out.bad());
        out.close();
    }
    return incomplete_segments;
}

template<typename Ta, typename Ts>
void writeEdge(const SegPair<Ts> & p, const MapContainer<SegPair<Ts>, Edge<Ta>, HashFunction<SegPair<Ts> > > & edges, const char * path)
{
    std::ofstream out(str(boost::format("%1%/%2%_%3%.data") % path % p.first % p.second), std::ios_base::binary);
    const auto & edge = edges.at(p);
    for (int i = 0; i != 3; i++) {
        for (const auto & kv : edge[i]) {
            const auto & k = kv.first;
            const auto & v = kv.second;
            out.write(reinterpret_cast<const char *>(&i), sizeof(i));
            out.write(reinterpret_cast<const char *>(k.data()), sizeof(int)*3);
            out.write(reinterpret_cast<const char *>(&v), sizeof(v));
        }
    }
    assert(!out.bad());
    out.close();
}

template<typename Ta, typename Ts>
void processData(const AffinityExtractor<Ts, Ta, ConstChunkRef<Ta,4> > & affinityExtractor, const char * path)
{
    using namespace std::placeholders;

    auto incomplete_segments = processMetaData(affinityExtractor, path);

    std::vector<SegPair<Ts> > incomplete_segpairs;
    std::vector<SegPair<Ts> > complete_segpairs;
    const auto & edges = affinityExtractor.edges();

    std::ofstream incomplete(str(boost::format("incomplete_edges_%1%.data") % path), std::ios_base::binary);
    std::ofstream complete(str(boost::format("complete_edges_%1%.data") % path), std::ios_base::binary);

    std::vector<std::pair<Ta, Ts> > me;
    for (const auto & kv : edges) {
        const auto k = kv.first;
        if (incomplete_segments.count(k.first) > 0 && incomplete_segments.count(k.second) > 0) {
            incomplete_segpairs.push_back(k);
        } else {
            complete_segpairs.push_back(k);
            me.push_back(meanAffinity<Ta, size_t>(edges.at(k)));
        }
    }
    std::vector<size_t> areas(complete_segpairs.size(),0);
    std::vector<Ta> affinities(complete_segpairs.size(),0);
    std::vector<size_t> idx(complete_segpairs.size());
    std::iota(idx.begin(), idx.end(), 0);

    auto f_rlme = QtConcurrent::map(idx, [&](auto i)
            {auto s = reweightedLocalMeanAffinity<Ta,size_t>(edges.at(complete_segpairs.at(i)));
             affinities[i] = s.first;
             areas[i] = s.second;
             });

    for (const auto & k: incomplete_segpairs) {
        incomplete << k.first << " " << k.second << "\n";
        writeEdge(k, edges, path);
    }

    f_rlme.waitForFinished();

    for (size_t i = 0; i != complete_segpairs.size(); i++) {
        const auto & p = complete_segpairs[i];
        complete.write(reinterpret_cast<const char *>(&(p.first)), sizeof(Ts));
        complete.write(reinterpret_cast<const char *>(&(p.second)), sizeof(Ts));
        complete.write(reinterpret_cast<const char *>(&(me[i].first)), sizeof(Ta));
        complete.write(reinterpret_cast<const char *>(&(me[i].second)), sizeof(size_t));
        complete.write(reinterpret_cast<const char *>(&(p.first)), sizeof(Ts));
        complete.write(reinterpret_cast<const char *>(&(p.second)), sizeof(Ts));
        complete.write(reinterpret_cast<const char *>(&(affinities[i])), sizeof(Ta));
        complete.write(reinterpret_cast<const char *>(&(areas[i])), sizeof(size_t));
        //complete << std::setprecision (17) << p.first << " " << p.second << " " << me[i].first << " " << me[i].second << " ";
        //complete << std::setprecision (17) << p.first << " " << p.second << " " << affinities[i]<< " " << areas[i] << std::endl;
    }
    assert(!incomplete.bad());
    assert(!complete.bad());
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
    assert(seg_file.is_open());
    ConstChunkRef<seg_t, 3> seg_chunk (reinterpret_cast<const seg_t*>(seg_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])], boost::fortran_storage_order());
    std::cout << "mmap seg data" << std::endl;

    bio::mapped_file_source aff_file;
    size_t aff_bytes = sizeof(aff_t)*dim[0]*dim[1]*dim[2]*3;
    aff_file.open("aff.raw", aff_bytes);
    assert(aff_file.is_open());
    ConstChunkRef<aff_t,4> aff_chunk (reinterpret_cast<const aff_t*>(aff_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])][3], boost::fortran_storage_order());
    std::cout << "mmap aff data" << std::endl;

    AffinityExtractor<seg_t, aff_t, ConstChunkRef<aff_t, 4> > affinity_extractor(aff_chunk);
    traverseSegments(seg_chunk, true, affinity_extractor);
    processData(affinity_extractor, output_path);
    std::cout << "finished" << std::endl;
    aff_file.close();
    seg_file.close();
}
