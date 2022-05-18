#ifndef TYPES_H
#define TYPES_H

#include <array>
#include <boost/multi_array.hpp>

#include "../global_types.h"

typedef boost::multi_array_types::extent_range Range;

template <typename T, int n>
using ConstChunkRef = boost::const_multi_array_ref<T, n, const T*>;

template <class Ts>
using SegPair = std::pair<Ts, Ts>;

using Coord = std::array<int64_t, 3>;

using ContactRegion = SetContainer<Coord, HashFunction<Coord> >;
using ContactRegionExt = MapContainer<Coord, int, HashFunction<Coord> >;

template <class Ta>
using Edge = std::array<MapContainer<Coord, Ta, HashFunction<Coord> >, 3>;

using semantic_t = uint64_t;

template <class T>
struct __attribute__((packed)) matching_entry_t
{
    T oid;
    size_t boundary_size;
    T nid;
    size_t agg_size;
};

template <class T_seg, class T_aff>
struct __attribute__((packed)) rg_entry
{
    T_seg s1;
    T_seg s2;
    T_aff aff;
    size_t area;
    rg_entry() = default;
    explicit rg_entry(const std::pair<SegPair<T_seg>, std::pair<T_aff, size_t> >  & p) {
        s1 = p.first.first;
        s2 = p.first.second;
        aff = p.second.first;
        area = p.second.second;
    }
};

#endif
