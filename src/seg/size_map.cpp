#include <iostream>
#include <filesystem>
#include <fstream>
#include "Utils.hpp"

int main(int argc, char * argv[])
{
    if (argc != 3) {
        std::cerr << "you need to specify the segmentation and the size map" << std::endl;
        return -1;
    }
    auto seg = read_array<uint64_t>(argv[1]);
    auto size_array = read_array<std::pair<uint64_t, uint64_t> >(argv[2]);

    std::sort(size_array.begin(), size_array.end(), [](auto & a, auto & b) {
        return a.first < b.first;
    });
    auto last = std::unique(size_array.begin(), size_array.end(), [](auto & a, auto & b) {
            return a.first == b.first && a.second == b.second;
    });
    size_array.erase(last, size_array.end());
    MapContainer<uint64_t, uint64_t> size_dict(size_array.begin(), size_array.end());
    MapContainer<uint64_t, size_t> cleft_segs;
    bool has_cleft = false;
    std::vector<uint8_t> size_map;
    if (std::filesystem::exists("cleft.raw")) {
        std::cerr << "filter small segments using the cleft map" << std::endl;
        has_cleft = true;
        auto cleft = read_array<uint8_t>("cleft.raw");
        for (size_t i = 0; i != seg.size(); i++) {
            if (cleft[i] > 0) {
                cleft_segs[seg[i]] += 1;
            }
        }
    }

    std::cerr << "create size_map" << std::endl;
    std::transform(seg.begin(), seg.end(), std::back_inserter(size_map), [&size_dict, has_cleft, &cleft_segs](uint64_t segid) -> uint8_t {
        if (segid == 0){
            return 0;
        }
        auto size = size_dict[segid];
        if ((!has_cleft) or (cleft_segs.count(segid) != 0 and cleft_segs.at(segid) > 10)) {
            if (size < 10000) {
                return 255;
            } else {
                return 0;
            }
       } else {
            return 0;
       }
    });
    std::ofstream fout("size_map.data", fout.out | fout.binary);
    fout.write((char*)&size_map[0], size_map.size() * sizeof(uint8_t));
    assert(!fout.bad());
    fout.close();
}
