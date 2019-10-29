#ifndef MEAN_EDGE_HPP
#define MEAN_EDGE_HPP

#include "Types.h"

template <typename Ta, typename Ts>
std::pair<Ta, Ts> meanAffinity(const Edge<Ta> & edge)
{
    Ta affinity = 0;
    Ts area = 0;
    for (int i = 0; i != 3; i++) {
        for (auto & kv : edge[i]) {
            affinity += kv.second;
            area += 1;
        }
    }
    return std::make_pair(affinity, area);
}

#endif
