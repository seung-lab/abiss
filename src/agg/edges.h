#ifndef EDGES_H
#define EDGES_H

#include "../global_types.h"

template <class T>
struct __attribute__((packed)) edge_t
{
    seg_t v0, v1;
    T        w;
};

struct __attribute__((packed)) mean_edge
{
    aff_t sum;
    size_t num;

    explicit constexpr mean_edge(aff_t s = 0, size_t n = 1)
        : sum(s)
        , num(n)
    {
    }
};

struct mean_edge_plus
{
    mean_edge operator()(mean_edge const& a, mean_edge const& b) const
    {
        return mean_edge(a.sum + b.sum, a.num + b.num);
    }
};

struct mean_edge_greater
{
    bool operator()(mean_edge const& a, mean_edge const& b) const
    {
        if (b.num == 0) return false;
        if (a.num == 0) return true;
        return a.sum / a.num > b.sum / b.num;
    }
};

struct mean_edge_limits
{
    static constexpr mean_edge max()
    {
        return mean_edge(std::numeric_limits<aff_t>::max());
    }
    static constexpr mean_edge min()
    {
        return mean_edge(0,0);
    }
};

typedef struct __attribute__((packed)) atomic_edge
{
    seg_t u1;
    seg_t u2;
    aff_t sum_aff;
    size_t area;
    explicit constexpr atomic_edge(seg_t w1 = 0, seg_t w2 = 0, aff_t s_a = 0.0, size_t a = 0)
        : u1(w1)
        , u2(w2)
        , sum_aff(s_a)
        , area(a)
    {
    }
} atomic_edge_t;

struct __attribute__((packed)) mst_edge
{
    aff_t sum;
    size_t num;
    atomic_edge_t repr;

    explicit constexpr mst_edge(aff_t s = 0, size_t n = 1, atomic_edge_t r = atomic_edge_t() )
        : sum(s)
        , num(n)
        , repr(r)
    {
    }
};

struct mst_edge_plus
{
    mst_edge operator()(mst_edge const& a, mst_edge const& b) const
    {
        atomic_edge_t new_repr = a.repr;
        if (a.repr.area < b.repr.area) {
            new_repr = b.repr;
        }
        return mst_edge(a.sum + b.sum, a.num + b.num, new_repr);
    }
};

struct mst_edge_greater
{
    bool operator()(mst_edge const& a, mst_edge const& b) const
    {
        if (b.num == 0) return false;
        if (a.num == 0) return true;
        return a.sum / a.num > b.sum / b.num;
    }
};

struct mst_edge_limits
{
    static constexpr mst_edge max()
    {
        return mst_edge(std::numeric_limits<aff_t>::max());
    }
    static constexpr mst_edge min()
    {
        return mst_edge(0,0);
    }
};

template <class CharT, class Traits>
::std::basic_ostream<CharT, Traits>&
operator<<(::std::basic_ostream<CharT, Traits>& os, mean_edge const& v)
{
    os << v.sum << " " << v.num;
    return os;
}

template <class CharT, class Traits>
::std::basic_ostream<CharT, Traits>&
operator<<(::std::basic_ostream<CharT, Traits>& os, mst_edge const& v)
{
    os << v.sum << " " << v.num << " " << v.repr.u1 << " " <<  v.repr.u2 << " " << v.repr.sum_aff << " " << v.repr.area;
    return os;
}

template <class CharT, class Edge, class Traits>
void write_edge(::std::basic_ostream<CharT, Traits>& os, Edge const& v)
{
    os.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

#endif
