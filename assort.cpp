#include "Types.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <boost/pending/disjoint_sets.hpp>
#include <boost/format.hpp>

const std::unordered_map<std::string, size_t> metaData =
{
    {"mst", (sizeof(seg_t)+sizeof(seg_t)+sizeof(aff_t)+sizeof(size_t))+(sizeof(seg_t)+sizeof(seg_t)+sizeof(aff_t)+sizeof(size_t))},
    {"sizes", sizeof(seg_t)+sizeof(size_t)}
};

std::unordered_map<seg_t, seg_t> load_remap(const char * filename)
{
    using rank_t = std::unordered_map<seg_t,std::size_t>;
    using parent_t = std::unordered_map<seg_t, seg_t>;;
    rank_t rank_map;
    parent_t parent_map;

    boost::associative_property_map<rank_t> rank_pmap(rank_map);
    boost::associative_property_map<parent_t> parent_pmap(parent_map);

    boost::disjoint_sets<boost::associative_property_map<rank_t>, boost::associative_property_map<parent_t> > sets(rank_pmap, parent_pmap);

    std::unordered_set<seg_t> nodes;

    seg_t s1, s2;

    std::ifstream in(filename);

    while (in.read(reinterpret_cast<char *>(&s1), sizeof(s1))) {
        in.read(reinterpret_cast<char *>(&s2), sizeof(s2));
        if (nodes.count(s1) == 0) {
            nodes.insert(s1);
            sets.make_set(s1);
        }
        if (nodes.count(s2) == 0) {
            nodes.insert(s2);
            sets.make_set(s2);
        }
        sets.link(s1, s2);
    }
    sets.compress_sets(std::begin(nodes), std::end(nodes));
    return parent_map;
}

std::vector<seg_t> load_seg(const char * filename)
{
    std::vector<seg_t> segs;
    seg_t s;
    std::ifstream in(filename);
    while (in.read(reinterpret_cast<char *>(&s), sizeof(s))) {
        segs.push_back(s);
    }
    return segs;
}

std::unordered_map<seg_t, std::vector<std::vector<char> > > load_data(const std::string & filename, size_t payloadSize)
{
    std::ifstream in(filename);
    seg_t s;
    std::unordered_map<seg_t, std::vector<std::vector<char> > > data;
    size_t count = 0;
    while (in.read(reinterpret_cast<char *>(&s), sizeof(s))) {
        std::vector<char> payload(payloadSize);
        in.read(reinterpret_cast<char *>(payload.data()), payloadSize);
        data[s].push_back(std::move(payload));
        count += 1;
    }
    std::cout << count << " entries" << std::endl;
    return data;
}

std::unordered_map<seg_t, std::vector<seg_t> > generate_bags(auto & pmap, auto & root)
{
    std::unordered_map<seg_t, seg_t> rep;
    std::unordered_map<seg_t, std::vector<seg_t> > bags;
    std::cout << root.size() << " roots" << std::endl;
    for (const auto & r : root) {
        if (pmap.count(r) > 0) {
            rep[pmap[r]] = r;
        } else {
            rep[r] = r;
            bags[r].push_back(r);
        }
    }
    for (auto & [k, v] : pmap) {
        if (rep.count(v) > 0) {
            bags[rep[v]].push_back(k);
        }
    }

    std::cout << bags.size() << " bags" << std::endl;
    return bags;
}

void write_data(auto & data, auto & k, auto & v, auto & out, size_t payloadSize)
{
    for (auto & s : v) {
        if (data.count(s) > 0) {
            for (auto & p : data.at(s)) {
                out.write(reinterpret_cast<const char *>(&k), sizeof(k));
                out.write(reinterpret_cast<const char *>(p.data()), payloadSize);
            }
        }
    }
    assert(!out.bad());
}

void split_data(const auto & data, const auto & ongoing, const auto & done, size_t payloadSize, const std::string & type, const std::string & tag)
{
    std::ofstream out(str(boost::format("ongoing_%1%_%2%.data")%type%tag), std::ios_base::binary);
    assert(out.is_open());
    for (auto & [k, v] : ongoing) {
        if (v.size() == 1 && data.count(v[0]) == 0) {
            continue;
        }
        write_data(data, k , v, out, payloadSize);
    }
    out.close();

    for (auto & [k, v] : done) {
        if (v.size() == 1 && data.count(v[0]) == 0) {
            continue;
        }
        std::ofstream o(str(boost::format("%1%/%2%.data")%type%k), std::ios_base::binary);
        assert(o.is_open());
        write_data(data, k , v, o, payloadSize);
        o.close();
    }
}

void write_info(auto bags, const std::string & filename)
{
    std::ofstream out(filename);
    assert(out.is_open());
    for (const auto & [k, v] : bags) {
        out << k << " " << v.size() << std::endl;
    }
    assert(!out.bad());
    out.close();
}

int main(int argc, char *argv[])
{
    auto pmap = load_remap("remap.data");
    auto ongoing = load_seg("ongoing_segments.data");
    auto done = load_seg("done_segments.data");
    std::string tag(argv[1]);
    std::cout << "map size: " << pmap.size() << std::endl;
    auto ongoing_bags = generate_bags(pmap, ongoing);
    auto done_bags = generate_bags(pmap, done);
    write_info(done_bags, str(boost::format("info_%1%.txt")%tag));
    for (const auto & [k, v] : metaData) {
        auto data = load_data(str(boost::format("%1%.data")%k), v);
        split_data(data, ongoing_bags, done_bags, v, k, tag);
    }
}
