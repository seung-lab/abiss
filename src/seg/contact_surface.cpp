#include "Types.h"
#include "Utils.hpp"
#include "SizeExtractor.hpp"
#include "BoundaryExtractor.hpp"
#include "ContactSurfaceExtractor.hpp"
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

bool boundarySurface(const BBox & bbox, const BBox & vol_bbox, const std::array<bool,6> & flags)
{
    for (int i = 0; i != 3; i++) {
        if ((bbox[i] <= vol_bbox[i]) && (!flags[i])) {
            return true;
        } else if ((bbox[i+3] >= vol_bbox[i+3]) && (!flags[i+3])) {
            return true;
        }
    }
    return false;
}

const int TDIndex[3][2] {{1,2},{0,2},{0,1}};

void markBoundaryCoord(seg_t cid, const Coord & c, std::vector<boost::multi_array<seg_t, 2> > & boundaries, const BBox & vol_bbox)
{
    for (int i = 0; i != 3; i++) {
        if (c[i] == vol_bbox[i]) {
            auto c2d = TDIndex[i];
            boundaries[i][c[c2d[0]]][c[c2d[1]]] = cid;
        }
        if (c[i] == vol_bbox[i+3]) {
            auto c2d = TDIndex[i];
            boundaries[i+3][c[c2d[0]]][c[c2d[1]]] = cid;
        }
    }
}

bool isDoubleCounting(const Coord & c, const BBox & vol_bbox, const std::array<bool,6> & boundary_flags)
{
    for (int i = 0; i != 3; i++) {
        if (c[i] == vol_bbox[i] && (!boundary_flags[i])) {
            return true;
        }
    }
    return false;

}

int main(int argc, char * argv[])
{
    std::array<int64_t, 3> offset({0,0,0});
    std::array<int64_t, 3> dim({0,0,0});
    size_t ac_offset;
    aff_t low_th;
    aff_t high_th;

    //FIXME: check the input is valid
    std::ifstream param_file(argv[1]);
    param_file >> offset[0] >> offset[1] >> offset[2];
    param_file >> dim[0] >> dim[1] >> dim[2];
    param_file >> ac_offset;
    param_file >> low_th >> high_th;
    std::cout << offset[0] << " " << offset[1] << " " << offset[2] << std::endl;
    std::cout << dim[0] << " " << dim[1] << " " << dim[2] << std::endl;
    std::array<bool,6> flags({true,true,true,true,true,true});
    int flag;
    for (size_t i = 0; i != 6; i++) {
        param_file >> flag;
        flags[i] = (flag > 0);
        if (flags[i]) {
            std::cout << "real boundary: " << i << std::endl;
        }
    }
    std::string chunk_tag(argv[2]);

    //FIXME: wrap these in classes
    bio::mapped_file_source seg_file;
    size_t seg_bytes = sizeof(seg_t)*dim[0]*dim[1]*dim[2];
    seg_file.open("seg.raw", seg_bytes);
    assert(seg_file.is_open());
    ConstChunkRef<seg_t, 3> seg_chunk (reinterpret_cast<const seg_t*>(seg_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])], boost::fortran_storage_order());
    std::cout << "mmap seg data" << std::endl;

    std::vector<std::vector<boost::multi_array<seg_t, 2> > > boundaries(3);
    for (int i = 0; i != 3; i++) {
        boundaries[i].push_back(boost::multi_array<seg_t, 2>(boost::extents[Range(offset[1]+1, offset[1]+dim[1])][Range(offset[2]+1, offset[2]+dim[2])], boost::fortran_storage_order()));
        boundaries[i].push_back(boost::multi_array<seg_t, 2>(boost::extents[Range(offset[0]+1, offset[0]+dim[0])][Range(offset[2]+1, offset[2]+dim[2])], boost::fortran_storage_order()));
        boundaries[i].push_back(boost::multi_array<seg_t, 2>(boost::extents[Range(offset[0]+1, offset[0]+dim[0])][Range(offset[1]+1, offset[1]+dim[1])], boost::fortran_storage_order()));
        boundaries[i].push_back(boost::multi_array<seg_t, 2>(boost::extents[Range(offset[1]+1, offset[1]+dim[1])][Range(offset[2]+1, offset[2]+dim[2])], boost::fortran_storage_order()));
        boundaries[i].push_back(boost::multi_array<seg_t, 2>(boost::extents[Range(offset[0]+1, offset[0]+dim[0])][Range(offset[2]+1, offset[2]+dim[2])], boost::fortran_storage_order()));
        boundaries[i].push_back(boost::multi_array<seg_t, 2>(boost::extents[Range(offset[0]+1, offset[0]+dim[0])][Range(offset[1]+1, offset[1]+dim[1])], boost::fortran_storage_order()));
    }

    BBox vol_bbox = {offset[0]+1, offset[1]+1, offset[2]+1,
                                    offset[0]+dim[0]-1, offset[1]+dim[1]-1, offset[2]+dim[2]-1};

    std::vector<std::pair<CRInfo, ContactRegionExt> > cs;
    if (std::filesystem::exists("aff.raw")) {
        bio::mapped_file_source aff_file;
        size_t aff_bytes = sizeof(aff_t)*dim[0]*dim[1]*dim[2]*3;
        aff_file.open("aff.raw", aff_bytes);
        assert(aff_file.is_open());
        ConstChunkRef<aff_t, 4> aff_chunk (reinterpret_cast<const aff_t*>(aff_file.data()), boost::extents[Range(offset[0], offset[0]+dim[0])][Range(offset[1], offset[1]+dim[1])][Range(offset[2], offset[2]+dim[2])][3], boost::fortran_storage_order());
        std::cout << "mmap aff data" << std::endl;
        ContactSurfaceWithAffinityExtractor<seg_t, aff_t, ConstChunkRef<aff_t, 4> > cs_extractor(aff_chunk, low_th, high_th);
        traverseSegments<2>(seg_chunk, cs_extractor);
        cs = cs_extractor.contactSurfaces();
    } else {
        ContactSurfaceExtractor<seg_t> cs_extractor;
        traverseSegments<2>(seg_chunk, cs_extractor);
        cs = cs_extractor.contactSurfaces();
    }

    std::ofstream incomplete("incomplete_cs_"+chunk_tag+".data", std::ios_base::binary);
    std::ofstream complete("complete_cs_"+chunk_tag+".data", std::ios_base::binary);
    seg_t incomplete_index = ac_offset;
    for (auto & c: cs) {
        if (boundarySurface(c.first.bbox, vol_bbox, flags)) {
            incomplete_index++;
            auto segids = c.first.p;
            for (const auto & [coord, n] : c.second) {
                for (int i = 0; i != 3; i++) {
                    if (n & (1<<i)) {
                        markBoundaryCoord(incomplete_index, coord, boundaries[i], vol_bbox);
                    }
                }
                if (isDoubleCounting(coord, vol_bbox, flags)) {
                    c.first.remove_duplicated_voxel(coord, n);
                }
            }
            incomplete.write(reinterpret_cast<const char *>(&incomplete_index), sizeof(incomplete_index));
            incomplete.write(reinterpret_cast<const char *>(&(c.first)), sizeof(c.first));
        } else {
            complete.write(reinterpret_cast<const char *>(&(c.first)), sizeof(c.first));
        }
    }
    assert(!complete.bad());
    assert(!incomplete.bad());
    complete.close();
    incomplete.close();
    for (int j = 0; j != 3; j++) {
        for (int i = 0; i != 6; i++) {
            std::ofstream f(str(boost::format("bc_%1%_%2%_%3%.data") % j % i % chunk_tag), (std::ios::out | std::ios::binary) );
            assert(f);

            f.write( reinterpret_cast<const char*>(boundaries[j][i].data()), boundaries[j][i].num_elements() * sizeof(seg_t));
            assert(!f.bad());
            f.close();
        }
    }

    std::cout << "all cs: " << cs.size() << std::endl;
    std::cout << "incomplete cs: " << incomplete_index - ac_offset << std::endl;

    std::cout << "finished" << std::endl;
    seg_file.close();
}
