#include "Types.h"
#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
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

    assert(!in.bad());

    in.close();
    std::ofstream out(str(boost::format("boundary_%1%_%2%.data") % index % tag), std::ios_base::binary);
    for (const auto & v : incomplete_segments) {
        out.write(reinterpret_cast<const char *>(&v), sizeof(v));
    }
    assert(!out.bad());
    out.close();
    return incomplete_segments;
}

template<typename Ts, typename Ta>
RegionGraph<Ts, SimpleEdge<Ta> > loadRegionGraph(const std::string & fileName)
{
    RegionGraph<Ts, SimpleEdge<Ta> > rg;
    std::cout << "loading: " << fileName << std::endl;
    std::ifstream in(fileName);
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
    assert(!in.bad());
    in.close();
    return rg;
}

template <typename Ts>
std::unordered_map<Ts, size_t> loadSizes(const std::string & fileName)
{
    std::unordered_map<Ts, size_t> sizes;
    std::cout << "loading: " << fileName << std::endl;
    std::ifstream in(fileName);
    Ts s;
    size_t size;
    while (in.read(reinterpret_cast<char *>(&s), sizeof(s))) {
        in.read(reinterpret_cast<char *>(&s), sizeof(s));
        in.read(reinterpret_cast<char *>(&size), sizeof(size));
        //if (sizes.count(s) > 0) {
        //    std::cout << s << " " << size << std::endl;
        //}
        sizes[s] += size;
    }
    assert(!in.bad());
    in.close();
    return sizes;
}

template <typename Ts, typename Ta>
void writeRegionGraph(const std::unordered_set<Ts> & incompleteSegments, const RegionGraph<Ts, SimpleEdge<Ta> >  rg, const std::string & incompleteFileName, const std::string & completeFileName)
{
    std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
    std::ofstream complete(completeFileName, std::ios_base::binary);

    for (const auto &[k, v] : rg) {
        if (incompleteSegments.count(k.first) > 0 && incompleteSegments.count(k.second) > 0) {
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
    assert(!incomplete.bad());
    assert(!complete.bad());
    incomplete.close();
    complete.close();
}

template <typename Ts>
void writeSizes(const std::unordered_set<Ts> & incompleteSegments, const std::unordered_map<Ts, size_t> sizes, const std::string & incompleteFileName, const std::string & completeFileName)
{
    std::ofstream incomplete(incompleteFileName, std::ios_base::binary);
    std::ofstream complete(completeFileName, std::ios_base::binary);
    for (const auto &[k,v] : sizes) {
        if (incompleteSegments.count(k) > 0) {
            incomplete.write(reinterpret_cast<const char *> (&k), sizeof(k));
            incomplete.write(reinterpret_cast<const char *> (&k), sizeof(k));
            incomplete.write(reinterpret_cast<const char *> (&v), sizeof(v));
        } else {
            complete.write(reinterpret_cast<const char *> (&k), sizeof(k));
            complete.write(reinterpret_cast<const char *> (&k), sizeof(k));
            complete.write(reinterpret_cast<const char *> (&v), sizeof(v));
        }
    }
    assert(!incomplete.bad());
    assert(!complete.bad());
    incomplete.close();
    complete.close();
}

template <typename Ts, typename Ta>
void updateRegionGraph(const std::unordered_set<Ts> & incompleteSegments, const std::string & inputFileName, const std::string & incompleteFileName, const std::string & completeFileName)
{
    writeRegionGraph<Ts, Ta>(incompleteSegments, loadRegionGraph<Ts, Ta>(inputFileName), incompleteFileName, completeFileName);
}

template <typename Ts>
void updateSizes(const std::unordered_set<Ts> & incompleteSegments, const std::string & inputFileName, const std::string & incompleteFileName, const std::string & completeFileName)
{
    writeSizes<Ts>(incompleteSegments, loadSizes<Ts>(inputFileName), incompleteFileName, completeFileName);
}


int main(int argc, char * argv[])
{
    std::string tag(argv[1]);

    std::unordered_set<seg_t> incomplete_segments;
    for (size_t i = 0; i != 6; i++) {
        incomplete_segments.merge(updateBoundarySegments<seg_t>(i, tag));
    }
    std::cout << incomplete_segments.size() << " incomplete segments" << std::endl;
    updateRegionGraph<seg_t, aff_t>(incomplete_segments,
            str(boost::format("incomplete_edges_%1%.tmp") % tag),
            str(boost::format("incomplete_edges_%1%.data") % tag),
            "new_edges.data");
    updateSizes<seg_t>(incomplete_segments,
            str(boost::format("incomplete_sizes_%1%.tmp") % tag),
            str(boost::format("incomplete_sizes_%1%.data") % tag),
            "sizes.data");

}
