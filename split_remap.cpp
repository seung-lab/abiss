#include "Types.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/format.hpp>
#include <parallel/algorithm>

template <typename T>
struct remap_t{
    enum SegType {undef, ongoing, done};
    std::vector<T> remaps;
    std::vector<T> segids;
    std::vector<SegType> segtype;
};

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
remap_t<T> load_remap(const char * filename)
{
    std::vector<std::pair<T, T> > remap_vector = read_array<std::pair<T, T> >(filename);
    std::vector<T> segids;
    std::transform(remap_vector.begin(), remap_vector.end(), std::back_inserter(segids), [](auto & p){
            return p.first;
    });
    std::transform(remap_vector.begin(), remap_vector.end(), std::back_inserter(segids), [](auto & p){
            return p.second;
    });
    std::sort(segids.begin(), segids.end());
    auto last = std::unique(segids.begin(), segids.end());
    segids.erase(last, segids.end());
    std::cout << segids.size() << " segments to consider" << std::endl;

    std::vector<T> remaps(segids.size());
    std::iota(remaps.begin(), remaps.end(), 0);

    __gnu_parallel::for_each(remap_vector.begin(), remap_vector.end(), [&segids](auto & p) {
            auto it = std::lower_bound(segids.begin(), segids.end(), p.first);
            if (it == segids.end() || *it != p.first) {
                std::cerr << "Should not happen, cannot find first segid: " << p.first << std::endl;
                std::abort();
            }
            p.first = std::distance(segids.begin(), it);
            it = std::lower_bound(segids.begin(), segids.end(), p.second);
            if (it == segids.end() || *it != p.second) {
                std::cerr << "Should not happen, cannot find second segid: " << p.second << std::endl;
                std::abort();
            }
            p.second = std::distance(segids.begin(), it);
    });

    for (size_t i = remap_vector.size(); i != 0; i--) {
        auto & p = remap_vector[i-1];
        remaps[p.first] = remaps[p.second];
    }

    remap_t<T> remap_data;
    remap_data.segtype.resize(segids.size(), remap_t<T>::undef);
    remap_data.remaps.swap(remaps);
    remap_data.segids.swap(segids);

    return remap_data;
}

template<typename T>
std::vector<std::pair<T, size_t> > load_seg(const char * filename)
{
    std::vector<std::pair<T, size_t> > ssize = read_array<std::pair<T, size_t> >(filename);
    return ssize;
}

template<typename T>
void classify_segments(remap_t<T> & remap_data, const char * ongoing_fn, const char * done_fn)
{
    auto done = load_seg<seg_t>(done_fn);
    auto ongoing = load_seg<seg_t>(ongoing_fn);
    std::cout << "seg done:" << done.size() << std::endl;
    std::cout << "seg ongoing:" << ongoing.size() << std::endl;
    auto & segids = remap_data.segids;
    auto & segtype = remap_data.segtype;
    __gnu_parallel::for_each(done.begin(), done.end(), [&segids, &segtype](auto & p) {
            auto s = p.first;
            auto it = std::lower_bound(segids.begin(), segids.end(), s);
            if (it != segids.end() && *it == s) {
                auto ind = std::distance(segids.begin(), it);
                if (segtype[ind] == remap_t<T>::undef) {
                    segtype[ind] = remap_t<T>::done;
                } else {
                    std::cerr << "segment " << segids[ind] << " should be done, but marked as " << segtype[ind] << std::endl;
                    std::abort();
                }
            }
    });
    __gnu_parallel::for_each(ongoing.begin(), ongoing.end(), [&segids, &segtype](auto & p) {
            auto s = p.first;
            auto it = std::lower_bound(segids.begin(), segids.end(), s);
            if (it != segids.end() &&  *it == s) {
                auto ind = std::distance(segids.begin(), it);
                if (segtype[ind] == remap_t<T>::undef) {
                    segtype[ind] = remap_t<T>::ongoing;
                } else {
                    std::cerr << "segment " << segids[ind] << " should be ongoing, but marked as " << segtype[ind] << std::endl;
                    std::abort();
                }
            }
    });
}

template<typename T>
void split_remap(const remap_t<T> & remap_data, size_t ac_offset, const std::string & tag)
{
    size_t current_ac = std::numeric_limits<std::size_t>::max();
    std::ofstream of_done;
    std::ofstream of_ongoing;
    of_ongoing.open(str(boost::format("ongoing_%1%.data") % tag));
    if (!of_ongoing.is_open()) {
        std::cerr << "Failed to open ongoing remap file for " << tag << std::endl;
        std::abort();
    }
    auto & segids = remap_data.segids;
    auto & remaps = remap_data.remaps;
    auto & segtype = remap_data.segtype;

    std::unordered_map<T, T> reps;

    for (size_t i = 0; i != remaps.size(); i++) {
        if (remaps[i] == i) {
            continue;
        }
        auto s = segids[i];
        auto seg = segids[remaps[i]];
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
        if (segtype[remaps[i]] == remap_t<T>::ongoing) {
            if (reps.count(seg) == 0) {
                of_ongoing.write(reinterpret_cast<const char *>(&(s)), sizeof(T));
                of_ongoing.write(reinterpret_cast<const char *>(&(seg)), sizeof(T));
                reps[seg] = s;
            } else {
                of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(T));
                of_done.write(reinterpret_cast<const char *>(&(reps.at(seg))), sizeof(T));
            }
        } else if (segtype[remaps[i]] == remap_t<T>::done){
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
    auto remap_data = load_remap<seg_t>("localmap.data");
    classify_segments(remap_data, "ongoing_segments.data", "done_segments.data");
    size_t ac_offset = 0;
    std::cout << "remap size:" << remap_data.remaps.size() << std::endl;
    std::ifstream param_file(argv[1]);
    param_file >> ac_offset;
    param_file.close();
    std::string tag(argv[2]);
    split_remap(remap_data, ac_offset, tag);
}
