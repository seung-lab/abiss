#include "Types.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/format.hpp>

template<typename T>
std::vector<std::pair<T, T> > load_remap(std::string filename)
{
    std::unordered_map<T, T> parent_map;
    std::vector<std::pair<T, T> > remap_vector;

    std::ifstream in(filename);

    T s1,s2;

    while (in.read(reinterpret_cast<char *>(&s1), sizeof(s1))) {
        in.read(reinterpret_cast<char *>(&s2), sizeof(s2));
        remap_vector.push_back(std::make_pair(s1,s2));
    }

    for (size_t i = remap_vector.size(); i != 0; i--) {
        auto & p = remap_vector[i-1];
        if (parent_map.count(p.second) == 0) {
            parent_map[p.first] = p.second;
        } else {
            parent_map[p.first] = parent_map[p.second];
        }
    }
    remap_vector.clear();
    for (auto & kv : parent_map) {
        remap_vector.push_back(kv);
    }
    std::stable_sort(std::begin(remap_vector), std::end(remap_vector), [](auto & a, auto & b) {
        return a.first < b.first;
    });
    return remap_vector;
}

template<typename T>
std::unordered_set<T> load_seg(const char * filename)
{
    std::unordered_set<T> segs;
    seg_t s;
    size_t count;
    std::ifstream in(filename);
    while (in.read(reinterpret_cast<char *>(&s), sizeof(s))) {
        in.read(reinterpret_cast<char *>(&count), sizeof(count));
        segs.insert(s);
    }
    return segs;
}

template<typename T>
void split_remap(const std::vector<std::pair<T, T> > & remap, const std::unordered_set<T> & ongoing, const std::unordered_set<T> & done, size_t ac_offset, const std::string & tag)
{
    size_t current_ac = std::numeric_limits<std::size_t>::max();
    std::ofstream of_done;
    std::ofstream of_ongoing;
    of_ongoing.open(str(boost::format("ongoing_%1%.data") % tag));
    if (!of_ongoing.is_open()) {
        std::cerr << "Failed to open ongoing remap file for " << tag << std::endl;
        std::abort();
    }

    std::unordered_map<T, T> reps;


    for (const auto & [s, seg] : remap) {
        if (current_ac != (s - (s % ac_offset))) {
            if (of_done.is_open()) {
                of_done.close();
                if (of_done.is_open()) {
                    std::cerr << "Failed to close done remap file for " << tag << " " << current_ac << std::endl;
                    std::abort();
                }
                reps.clear();
            }
            current_ac = s - (s % ac_offset);
            of_done.open(str(boost::format("remap/done_%1%_%2%.data") % tag % current_ac));
            if (!of_done.is_open()) {
                std::cerr << "Failed to open done remap file for " << tag << " " << current_ac << std::endl;
                std::abort();
            }
        }
        if (ongoing.count(seg) > 0) {
            if (reps.count(seg) == 0) {
                of_ongoing.write(reinterpret_cast<const char *>(&(s)), sizeof(T));
                of_ongoing.write(reinterpret_cast<const char *>(&(seg)), sizeof(T));
                reps[seg] = s;
            } else {
                of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(T));
                of_done.write(reinterpret_cast<const char *>(&(reps.at(seg))), sizeof(T));
            }
        } else if (done.count(seg) > 0){
            of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(T));
            of_done.write(reinterpret_cast<const char *>(&(seg)), sizeof(T));
        } else {
            std::cerr << "segment "<< seg << " (" << s << ") is neither done or onoging, impossible!" << std::endl;
            std::abort();
        }
        if (of_done.bad()) {
            std::cerr << "Error occurred when writing done remap file for " << tag << " " << current_ac << std::endl;
            std::abort();
        }
        if (of_ongoing.bad()) {
            std::cerr << "Error occurred when writing ongoing remap file for " << tag << " " << current_ac << std::endl;
            std::abort();
        }
    }
}

int main(int argc, char *argv[])
{
    auto remap_vector = load_remap<seg_t>("localmap.data");
    auto ongoing = load_seg<seg_t>("ongoing_segments.data");
    auto done = load_seg<seg_t>("done_segments.data");
    size_t ac_offset = 0;
    std::cout << "remap size:" << remap_vector.size() << std::endl;
    std::cout << "seg done:" << done.size() << std::endl;
    std::cout << "seg ongoing:" << ongoing.size() << std::endl;
    std::ifstream param_file(argv[1]);
    param_file >> ac_offset;
    param_file.close();
    std::string tag(argv[2]);
    split_remap(remap_vector, ongoing, done, ac_offset, tag);
}
