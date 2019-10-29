#include <iostream>
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
    std::unordered_map<uint64_t, uint64_t> size_dict(size_array.begin(), size_array.end());
    std::vector<uint8_t> size_map;
    std::transform(seg.begin(), seg.end(), std::back_inserter(size_map), [&size_dict](uint64_t segid) -> uint8_t {
        if (segid == 0){
            return 0;
        }
        auto size = size_dict[segid];
        if (size < 10) {
            return 255;
        } else if (size < 100) {
            return 127;
        } else {
            return 0;
        }
    });
    std::ofstream fout("size_map.data", fout.out | fout.binary);
    fout.write((char*)&size_map[0], size_map.size() * sizeof(uint8_t));
    assert(!fout.bad());
    fout.close();
}
