#include "Types.h"
#include "Utils.hpp"
#include "ContactSurfaceExtractor.hpp"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <execution>
#include <boost/format.hpp>
#include <boost/pending/disjoint_sets.hpp>

std::vector<seg_t> generate_remaps(const std::vector<std::pair<seg_t, CRInfo> > & incomplete_cs)
{
    std::vector<seg_t> rank(incomplete_cs.size());
    std::vector<seg_t> parent(incomplete_cs.size());
    boost::disjoint_sets<seg_t*, seg_t*> sets(&rank[0], &parent[0]);

    for (size_t i = 0; i != incomplete_cs.size(); i++) {
        sets.make_set(i);
    }

    std::vector<std::vector<seg_t> > bc_fs;
    std::vector<std::vector<seg_t> > bc_bs;

    for (int i = 0; i != 3; i++) {
        auto bc_f = read_array<seg_t>(str(boost::format("bc_f_%1%.data") % i));
        auto bc_b = read_array<seg_t>(str(boost::format("bc_b_%1%.data") % i));
        assert(bc_f.size() == bc_b.size());

        std::for_each(std::execution::par, std::begin(bc_f), std::end(bc_f), [&incomplete_cs](auto & a) {
                if (a == 0) return;
                auto it = std::lower_bound(incomplete_cs.begin(), incomplete_cs.end(), a, [](auto x, auto y) {return x.first < y; });
                if (a == (*it).first) {
                    a = std::distance(incomplete_cs.begin(), it);
                } else {
                    std::cerr << "Cannot find matching cid: " << a << std::endl;
                    if (it != incomplete_cs.end()) {
                        std::cerr << "lower bound cid: " << (*it).first << std::endl;
                    }
                    std::abort();
                }
                });

        std::for_each(std::execution::par, std::begin(bc_b), std::end(bc_b), [&incomplete_cs](auto & a) {
                if (a == 0) return;
                auto it = std::lower_bound(incomplete_cs.begin(), incomplete_cs.end(), a, [](auto x, auto y) {return x.first < y; });
                if (a == (*it).first) {
                    a = std::distance(incomplete_cs.begin(), it);
                } else {
                    std::cerr << "Cannot find matching cid: " << a << std::endl;
                    if (it != incomplete_cs.end()) {
                        std::cerr << "lower bound cid: " << (*it).first << std::endl;
                    }
                    std::abort();
                }
                });
        bc_fs.push_back(bc_f);
        bc_bs.push_back(bc_b);
    }

    for (const auto & bc_f: bc_fs) {
        for (const auto & bc_b: bc_bs) {
            for (size_t i = 0; i != bc_f.size(); i++) {
                if (bc_f[i] == 0 and bc_b[i] == 0) {
                    continue;
                } else if (bc_f[i] != 0 and bc_b[i] != 0) {
                    auto c1 = incomplete_cs[bc_f[i]].second;
                    auto c2 = incomplete_cs[bc_b[i]].second;
                    if (c1.p != c2.p) {
                        continue;
                    }
                    const auto v1 = sets.find_set( bc_f[i] );
                    const auto v2 = sets.find_set( bc_b[i] );
                    if (v1 != v2) {
                        sets.link(v1,v2);
                    }
                }
            }
        }
    }

    std::vector<seg_t> idx(incomplete_cs.size());
    std::iota(idx.begin(), idx.end(), seg_t(0));
    sets.compress_sets(idx.begin(), idx.end());
    return parent;
}

SetContainer<seg_t> update_boundaries(const std::vector<std::pair<seg_t, CRInfo> > & incomplete_cs, std::vector<seg_t> & remaps, const std::array<bool,6> & boundary_flags, const std::string & tag)
{
    std::vector<seg_t> bc;
    for (size_t i = 0; i != 6; i++) {
        if (!boundary_flags[i]) {
            for (int j = 0; j != 3; j++) {
                auto fn = str(boost::format("bc_%1%_%2%_%3%.data") % j % i % tag);
                std::cout << "loading: " << fn << std::endl;

                auto face_array = read_array<seg_t>(fn);
                std::cout << "remapping face: " << fn << std::endl;
                std::for_each(std::execution::par, face_array.begin(), face_array.end(), [&remaps, &incomplete_cs](auto & s) {
                    if (s == 0) {
                        return;
                    }
                    auto it = std::lower_bound(incomplete_cs.begin(), incomplete_cs.end(), s, [](auto x, auto y) {return x.first < y; });
                    if (s == (*it).first) {
                        auto remapped = remaps[std::distance(incomplete_cs.begin(), it)];
                        s = incomplete_cs[remapped].first;
                    } else {
                        std::cerr << "Cannot find matching cid: " << s << std::endl;
                        if (it != incomplete_cs.end()) {
                            std::cerr << "lower bound cid: " << (*it).first << std::endl;
                        }
                        std::abort();
                    }
                });
                std::ofstream f(fn, (std::ios::out | std::ios::binary) );
                assert(f);

                f.write( reinterpret_cast<const char*>(face_array.data()), face_array.size() * sizeof(seg_t));
                assert(!f.bad());
                f.close();
                std::copy_if(face_array.begin(), face_array.end(), std::back_inserter(bc), [](const auto & x) {return x != 0;});

            }
        }
    }
    return SetContainer<seg_t>(bc.begin(), bc.end());
}

std::vector<std::pair<seg_t, CRInfo> > merge_cs(std::vector<std::pair<seg_t, CRInfo> > & cs, const std::vector<seg_t> & remaps)
{
    std::vector<std::pair<seg_t, CRInfo> > merged_cs;
    std::cout << cs.size() << ", " << remaps.size() << std::endl;
    for (size_t i = 1; i != cs.size(); i++) {
        auto target = remaps[i];
        if (target != i) {
            cs[target].second.merge_with(cs[i].second);
        }
    }
    for (size_t i = 1; i != cs.size(); i++) {
        auto target = remaps[i];
        if (target == i) {
            merged_cs.push_back(cs[target]);
        }
    }
    return merged_cs;
}

int main(int argc, char * argv[])
{
    std::ifstream param_file(argv[1]);
    int flag;
    std::array<bool,6> flags({true,true,true,true,true,true});
    for (size_t i = 0; i != 6; i++) {
        param_file >> flag;
        flags[i] = (flag > 0);
        if (flags[i]) {
            std::cout << "real boundary: " << i << std::endl;
        }
    }

    std::string tag(argv[2]);
    auto incomplete_cs = read_array<std::pair<seg_t, CRInfo> >("incomplete_cs.data");
    incomplete_cs.emplace_back(0,CRInfo());
    std::stable_sort(std::execution::par, std::begin(incomplete_cs), std::end(incomplete_cs), [](auto & a, auto & b) { return a.first < b.first; });

    std::cout << "matching the overlapping region!" << std::endl;
    auto remaps = generate_remaps(incomplete_cs);
    std::cout << "update unmerged boundaries!" << std::endl;
    auto bc = update_boundaries(incomplete_cs, remaps, flags, tag);
    std::cout << "merge contact surfaces!" << std::endl;
    auto merged_cs = merge_cs(incomplete_cs, remaps);

    std::cout << "write out merged results!" << std::endl;
    std::ofstream incomplete("incomplete_cs_"+tag+".data", std::ios_base::binary);
    std::ofstream complete("complete_cs_"+tag+".data", std::ios_base::binary);
    for (auto & c: merged_cs) {
        if (bc.count(c.first) != 0) {
            incomplete.write(reinterpret_cast<const char *>(&c), sizeof(c));
        } else {
            complete.write(reinterpret_cast<const char *>(&c.second), sizeof(c.second));
        }
    }
    assert(!complete.bad());
    assert(!incomplete.bad());
    complete.close();
    incomplete.close();
}
