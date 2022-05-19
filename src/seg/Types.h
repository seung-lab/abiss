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

using semantic_t = uint8_t;

#endif
