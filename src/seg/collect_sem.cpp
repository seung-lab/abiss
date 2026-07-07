#include "../global_types.h"
#include "Types.h"
#include <cassert>
#include <array>
#include <fstream>
#include <iostream>
#include <boost/iostreams/device/mapped_file.hpp>

using sem_array_t = std::array<size_t, 4>;
namespace bio = boost::iostreams;
//                                            dummy, cytoplasma, axon, dendrite, glia, bv, nuclei, ecs, other
static constexpr std::array<int, 9> sem_map = {3,3,1,0,2,2,3,2,2};

int main(int argc, char * argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " seg_file sem_output_file" << std::endl;
        return -1;
    }

    size_t xdim, ydim, zdim;
    std::ifstream param_file("param.txt");
    param_file >> xdim >> ydim >> zdim;
    param_file.close();
    std::cout << "Dimensions: " << xdim << " " << ydim << " " << zdim << std::endl;

    size_t voxels = xdim * ydim * zdim;

    bio::mapped_file_source sem_file;
    size_t sem_bytes = sizeof(semantic_t) * voxels;
    sem_file.open("sem.raw", sem_bytes);
    assert(sem_file.is_open());
    const semantic_t * sem_data = reinterpret_cast<const semantic_t *>(sem_file.data());
    std::cout << "mmap sem data" << std::endl;

    bio::mapped_file_source seg_file;
    size_t seg_bytes = sizeof(seg_t) * voxels;
    seg_file.open(argv[1], seg_bytes);
    assert(seg_file.is_open());
    const seg_t * seg_data = reinterpret_cast<const seg_t *>(seg_file.data());
    std::cout << "mmap seg data" << std::endl;

    MapContainer<seg_t, sem_array_t> sem_labels;

    for (size_t i = 0; i < voxels; i++) {
        seg_t segid = seg_data[i];
        if (segid == 0) continue;

        semantic_t sem_label = sem_data[i];
        if (sem_label >= 0 && sem_map[sem_label] >= 0) {
            sem_labels[segid][static_cast<size_t>(sem_map[sem_label])] += 1;
        }
    }

    std::cout << "Collected semantic labels for " << sem_labels.size() << " segments" << std::endl;

    std::ofstream of(argv[2], std::ios_base::binary);
    assert(of.is_open());
    for (const auto & [k, v] : sem_labels) {
        size_t total = v[0] + v[1] + v[2];
        if (total < 1000) continue;
        of.write(reinterpret_cast<const char *>(&k), sizeof(k));
        of.write(reinterpret_cast<const char *>(&v), sizeof(v));
        assert(!of.bad());
    }
    of.close();

    std::cout << "Wrote " << sem_labels.size() << " entries to " << argv[2] << std::endl;

    sem_file.close();
    seg_file.close();
}
