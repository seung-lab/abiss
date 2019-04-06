#include "Types.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/format.hpp>

size_t filesize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : 0;
}

template <class T>
std::vector<T> read_array(const char * filename)
{
    std::vector<T> array;

    std::cout << "filesize:" << filesize(filename)/sizeof(T) << std::endl;
    size_t data_size = filesize(filename);
    if (data_size % sizeof(T) != 0) {
        std::cerr << "File incomplete!: " << filename << std::endl;
        std::abort();
    }

    FILE* f = std::fopen(filename, "rbXS");
    if ( !f ) {
        std::cerr << "Cannot open the region graph file" << std::endl;
        std::abort();
    }

    size_t array_size = data_size / sizeof(T);

    array.resize(array_size);
    std::size_t nread = std::fread(array.data(), sizeof(T), array_size, f);
    if (nread != array_size) {
        std::cerr << "Reading: " << nread << " entries, but expecting: " << array_size << std::endl;
        std::abort();
    }
    std::fclose(f);

    return array;
}

template<typename T>
std::vector<std::pair<T, T> > load_remap(const char * filename)
{
    std::unordered_map<T, T> parent_map;
    std::vector<std::pair<T, T> > remap_vector = read_array<std::pair<T, T> >(filename);

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
    std::vector<std::pair<T, size_t> > ssize = read_array<std::pair<T, size_t> >(filename);
    std::vector<T> segids;
    std::transform(ssize.begin(), ssize.end(), std::back_inserter(segids), [](auto p) {return p.first;});
    std::unordered_set<T> segs(segids.begin(), segids.end());
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
