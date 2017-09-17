#ifndef REWEIGHTED_LOCAL_MEAN_EDGE_HPP
#define REWEIGHTED_LOCAL_MEAN_EDGE_HPP

#define THRESHOLD 0.25

#include "Types.h"
#include "Utils.hpp"

ContactRegion mergeContactRegion(const auto & edge)
{
    ContactRegion cr;
    for (int i = 0; i != 3; i++) {
        for (auto &kv: edge.at(i)) {
            cr.insert(kv.first);
        }
    }
    return cr;
}

ContactRegion trimContactRegion(const ContactRegion & cr, const auto & edge)
{
    ContactRegion res;
    for (auto v: cr) {
        for (int i = 0; i != 3; i++) {
            if (edge[i].count(v) > 0 && edge[i].at(v) > THRESHOLD) {
                res.insert(v);
                break;
            }
        }
    }
    return res;
}

template <typename Ta, typename Ts>
std::pair<Ta, Ts> sumAffinity(const ContactRegion & cr, const Edge<Ta> & edge)
{
    Ta affinity = 0;
    Ts area = 0;
    for (auto v: cr) {
        for (int i = 0; i != 3; i++) {
            if (edge[i].count(v) > 0) {
                affinity += edge[i].at(v);
                area += 1;
            }
        }
    }
    //std::cout << "sum affinity:" << affinity << " " << area << std::endl;
    return std::make_pair(affinity, area);
}

template <typename Ta, typename Ts>
std::pair<Ta, Ts> reweightedLocalMeanAffinity(const Edge<Ta> & edge)
{
    std::vector<std::pair<Ta, Ts> > reweighted_pairs;
    auto cr = mergeContactRegion(edge);
    auto cc_sets = connectComponent(cr);

    Ta reweighted_affinity = 0;
    Ts area = 0;
    std::tie(reweighted_affinity, area) = sumAffinity<Ta, Ts>(cr, edge);
    Ta reweighted_area = area;

    for (auto cc: cc_sets) {
        Ta reweighted_cc_affinity = 0;
        Ts cc_area = 0;
        std::tie(reweighted_cc_affinity, cc_area) = sumAffinity<Ta, Ts>(cc, edge);
        Ta reweighted_cc_area = cc_area;

        auto trimmed_cc_sets = connectComponent(trimContactRegion(cc, edge));

        for (auto tcc: trimmed_cc_sets) {
            Ts tcc_area = 0;
            Ta tcc_affinity = 0;
            std::tie(tcc_affinity, tcc_area) = sumAffinity<Ta, Ts>(tcc, edge);
            if (tcc_affinity > 1) {
                reweighted_cc_affinity += (tcc_affinity - 1) * tcc_affinity;
                reweighted_affinity += (tcc_affinity - 1) * tcc_affinity;
                reweighted_cc_area += (tcc_affinity - 1) * tcc_area;
                reweighted_area += (tcc_affinity - 1) * tcc_area;
            }
        }
        reweighted_cc_affinity *= cc_area/reweighted_cc_area;
        if (cc_area > 20) {
            reweighted_pairs.emplace_back(reweighted_cc_affinity, cc_area);
        }
    }

    reweighted_affinity *= area/reweighted_area;
    reweighted_pairs.emplace_back(reweighted_affinity,area);
    //for (auto p: reweighted_pairs) {
    //    std::cout << p.first << " " << p.second << std::endl;
    //}

    return *std::max_element(reweighted_pairs.begin(), reweighted_pairs.end(),
              [](auto && a, auto && b) {
                  return a.first/a.second < b.first/b.second;});
}

template <typename Ta, typename Ts>
std::pair<Ta, Ts> reweightedLocalMeanAffinity_helper(const SegPair<Ts> & segPair, const std::unordered_map<SegPair<Ts>, Edge<Ta>, boost::hash<SegPair<Ts> > > & edges)
{
    return reweightedLocalMeanAffinity<Ta, Ts>(edges.at(segPair));
}
#endif
