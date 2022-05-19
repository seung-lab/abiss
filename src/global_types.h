#ifndef GLOBAL_TYPES_H
#define GLOBAL_TYPES_H

#ifdef USE_ABSL_HASHMAP
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>
#else
#include <unordered_set>
#include <unordered_map>
#include <boost/functional/hash.hpp>
#endif

using seg_t = uint64_t;
#ifdef DOUBLE
using aff_t = double;
#else
using aff_t = float;
#endif

#ifdef USE_ABSL_HASHMAP
template<typename K, typename V, typename H=absl::Hash<K> >
using MapContainer = absl::flat_hash_map<K, V, H>;
template<typename K, typename H=absl::Hash<K> >
using SetContainer = absl::flat_hash_set<K, H>;
template<typename T>
using HashFunction = absl::Hash<T>;
#else
template<typename K, typename V, typename H=std::hash<K> >
using MapContainer = std::unordered_map<K, V, H>;
template<typename K, typename H=std::hash<K> >
using SetContainer = std::unordered_set<K, H>;
template<typename T>
using HashFunction = boost::hash<T>;
#endif

#endif
