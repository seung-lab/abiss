#include "Types.h"
#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <boost/format.hpp>

template <typename Ts, typename Te>
using RegionGraph = std::unordered_map<SegPair<Ts>, Te, boost::hash<SegPair<Ts> > >;

template<typename Ta>
struct SimpleEdge{
    Ta affinity;
    size_t area;
    explicit constexpr SimpleEdge(Ta a = 0.0, size_t b = 0)
        :affinity(a),area(b) {};
};

template<typename Ts>
std::unordered_set<Ts> updateBoundarySegments(size_t index, const std::string & tag)
{
    std::unordered_set<Ts> incomplete_segments;
    Ts s;
    std::ifstream in(str(boost::format("boundary_%1%_%2%.tmp") % index % tag));
    std::cout << "update: " << index << std::endl;
    while (in.read(reinterpret_cast<char *>(&s), sizeof(s))) {
        incomplete_segments.insert(s);
    }
    in.close();
    std::ofstream out(str(boost::format("boundary_%1%_%2%.data") % index % tag), std::ios_base::binary);
    for (const auto & v : incomplete_segments) {
        out.write(reinterpret_cast<const char *>(&v), sizeof(v));
    }
    return incomplete_segments;
}

template<typename Ts, typename Ta>
RegionGraph<Ts, SimpleEdge<Ta> > loadRegionGraph(const std::string & tag)
{
    RegionGraph<Ts, SimpleEdge<Ta> > rg;
    std::ifstream in(str(boost::format("incomplete_edges_%1%.tmp") % tag));
    Ts s1, s2;
    size_t area;
    Ta aff;
    while (in.read(reinterpret_cast<char *>(&s1), sizeof(s1))) {
        in.read(reinterpret_cast<char *>(&s2), sizeof(s2));
        in.read(reinterpret_cast<char *>(&aff), sizeof(aff));
        in.read(reinterpret_cast<char *>(&area), sizeof(area));
        auto & e = rg[std::minmax(s1,s2)];
        e.affinity +=  aff;
        e.area += area;
    }
    return rg;
}

int main(int argc, char * argv[])
{
    //size_t xdim,ydim,zdim;
    //int flag;
    //size_t face_size, counts, dend_size;
    //std::ifstream param_file(argv[1]);
    const char * tag = argv[1];
    //param_file >> xdim >> ydim >> zdim;
    //std::cout << xdim << " " << ydim << " " << zdim << std::endl;
    //std::array<bool,6> flags({true,true,true,true,true,true});
    //for (size_t i = 0; i != 6; i++) {
    //    param_file >> flag;
    //    flags[i] = (flag > 0);
    //    if (flags[i]) {
    //        std::cout << "real boundary: " << i << std::endl;
    //    }
    //}

    //if (face_size == 0) {
    //    std::cout << "Nothing to merge, exit!" << std::endl;
    //    return 0;
    //}

    std::unordered_set<seg_t> incomplete_segments;
    for (size_t i = 0; i != 6; i++) {
        incomplete_segments.merge(updateBoundarySegments<seg_t>(i, tag));
    }
    auto rg = loadRegionGraph<seg_t, aff_t>(tag);

    std::ofstream incomplete(str(boost::format("incomplete_edges_%1%.data") % tag), std::ios_base::binary);
    std::ofstream complete("new_edges.data", std::ios_base::binary);

    for (const auto &[k, v] : rg) {
        if (incomplete_segments.count(k.first) > 0 && incomplete_segments.count(k.second) > 0) {
            incomplete.write(reinterpret_cast<const char *>(&(k.first)), sizeof(seg_t));
            incomplete.write(reinterpret_cast<const char *>(&(k.second)), sizeof(seg_t));
            incomplete.write(reinterpret_cast<const char *>(&(v.affinity)), sizeof(aff_t));
            incomplete.write(reinterpret_cast<const char *>(&(v.area)), sizeof(size_t));
        } else {
            complete.write(reinterpret_cast<const char *>(&(k.first)), sizeof(seg_t));
            complete.write(reinterpret_cast<const char *>(&(k.second)), sizeof(seg_t));
            complete.write(reinterpret_cast<const char *>(&(v.affinity)), sizeof(aff_t));
            complete.write(reinterpret_cast<const char *>(&(v.area)), sizeof(size_t));
            complete.write(reinterpret_cast<const char *>(&(k.first)), sizeof(seg_t));
            complete.write(reinterpret_cast<const char *>(&(k.second)), sizeof(seg_t));
            complete.write(reinterpret_cast<const char *>(&(v.affinity)), sizeof(aff_t));
            complete.write(reinterpret_cast<const char *>(&(v.area)), sizeof(size_t));
        }
    }
    incomplete.close();
    complete.close();
}
