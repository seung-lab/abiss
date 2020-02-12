#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cassert>
#include "Utils.hpp"

int main(int argc, char * argv[])
{
    if (argc != 3) {
        std::cerr << "you need to specify the segmentation layer and gt layer" << std::endl;
        return -1;
    }
    auto seg = read_array<uint64_t>(argv[1]);
    auto gt = read_array<uint64_t>(argv[2]);
    assert("Cannot compare segments with ground truth, the sizes of the two cutouts do not match");

    std::unordered_map<uint64_t, std::unordered_map<uint64_t, uint64_t>> p_ij;
    std::unordered_map<uint64_t, uint64_t> s_i, t_j;

    for (size_t i = 0; i != seg.size(); i++) {
        uint64_t segv = seg[i];
        uint64_t gtv = gt[i];
        if (gtv != 0) {
            p_ij[gtv][segv]++;
            s_i[segv]++;
            t_j[gtv]++;
        }
    }

    std::ofstream fout("evaluation.data", fout.out | fout.binary);

    size_t datasize = s_i.size();
    std::cout << "size of s_i: " << datasize << std::endl;
    fout.write((char*)&datasize, sizeof(size_t));
    for(const auto & p : s_i) {
        fout.write((char*)&p, sizeof(p));
    }
    assert(!fout.bad());

    datasize = t_j.size();
    std::cout << "size of t_j: " << datasize << std::endl;
    fout.write((char*)&datasize, sizeof(size_t));
    for(const auto & p : t_j) {
        fout.write((char*)&p, sizeof(p));
    }
    assert(!fout.bad());

    datasize = 0;
    for (const auto & [k, v] : p_ij) {
        datasize += v.size();
    }
    std::cout << "size of p_ij: " << datasize << std::endl;
    fout.write((char*)&datasize, sizeof(size_t));
    for (const auto & [k1, v1] : p_ij) {
        for (const auto & p : v1) {
            fout.write((char*)&k1, sizeof(k1));
            fout.write((char*)&p, sizeof(p));
        }
    }

    assert(!fout.bad());
    fout.close();
}
