#pragma once

#include "types.hpp"

#include <cstdio>
#include <cassert>
#include <fstream>
#include <type_traits>
#include <unordered_map>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

namespace bio = boost::iostreams;

template < typename T >
inline void free_container(T& p_container)
{
    T empty;
    using std::swap;
    swap(p_container, empty);
}

template < typename T >
inline bool read_from_file( const std::string& fname, T* data, std::size_t n )
{
    FILE* f = std::fopen(fname.c_str(), "rbXS");
    if ( !f ) return false;

    std::size_t nread = std::fread(data, sizeof(T), n, f);
    std::fclose(f);

    return nread == n;
}

template < typename T >
inline bool
write_to_file( const std::string& fname,
               const T* data, std::size_t n )
{
    std::ofstream f(fname.c_str(), (std::ios::out | std::ios::binary) );
    assert(f);

    f.write( reinterpret_cast<const char*>(data), n * sizeof(T));
    assert(!f.bad());
    f.close();
    return true;
}


template < typename T >
inline volume_ptr<T>
read_volume( const std::string& fname, std::size_t wsize )
{
    volume_ptr<T> vol(new volume<T>
                      (boost::extents[wsize][wsize][wsize],
                       boost::fortran_storage_order()));

    if ( !read_from_file(fname, vol->data(), wsize*wsize*wsize) ) throw 0;
    return vol;
}

template< typename ID, typename F >
inline bool write_region_graph( const std::string& fname,
                                const region_graph<ID,F>& rg )
{
    std::ofstream f(fname.c_str(), (std::ios::out | std::ios::binary) );
    if ( !f ) return false;

    F* data = new F[rg.size() * 3];

    std::size_t idx = 0;

    for ( const auto& e: rg )
    {
        data[idx++] = static_cast<F>(std::get<1>(e));
        data[idx++] = static_cast<F>(std::get<2>(e));
        data[idx++] = static_cast<F>(std::get<0>(e));
    }

    f.write( reinterpret_cast<char*>(data), rg.size() * 3 * sizeof(F));

    assert(!f.bad());

    delete [] data;


    f.close();
    return true;
}

template< typename ID >
inline std::tuple<volume_ptr<ID>, std::vector<std::size_t>>
    get_dummy_segmentation( std::size_t xdim,
                            std::size_t ydim,
                            std::size_t zdim )
{
    std::tuple<volume_ptr<ID>, std::vector<std::size_t>> result
        ( volume_ptr<ID>( new volume<ID>(boost::extents[xdim][ydim][zdim],
                                         boost::fortran_storage_order())),
          std::vector<std::size_t>(xdim*ydim*zdim+1));

    volume<ID>& seg = *(std::get<0>(result));
    auto& counts = std::get<1>(result);

    std::fill_n(counts.begin(), xdim*ydim*zdim*1, 1);
    counts[0] = 0;

    for ( ID i = 0; i < xdim*ydim*zdim; ++i )
    {
        seg.data()[i] = i+1;
    }

    return result;
}

template <typename T>
size_t write_vector(const std::string & fn, std::vector<T> & v)
{
    if (v.empty()) {
        std::ofstream fs;
        fs.open(fn);
        fs.close();
        return 0;
    }
    bio::mapped_file_params f_param;
    bio::mapped_file_sink f;
    size_t bytes = sizeof(T)*v.size();
    f_param.path = fn;
    f_param.new_file_size = bytes;
    f.open(f_param);
    assert(f.is_open());
    memcpy(f.data(), v.data(), bytes);
    f.close();
    return v.size();
}

template <typename T>
size_t write_counts(std::vector<size_t> & counts, T & offset, const char * tag)
{
    std::vector<std::pair<T, size_t> > output;
    for (T i = 1; i != counts.size(); i++) {
        output.emplace_back(i+offset, counts[i]);
    }
    return write_vector(str(boost::format("counts_%1%.data") % tag), output);
}

template <typename K, typename V>
size_t write_remap(const std::unordered_map<K, V> & map, const char * tag)
{
    std::vector<std::pair<K, V> > output;
    for (const auto & kv : map) {
        output.push_back(kv);
    }
    return write_vector(str(boost::format("remap_%1%.data") % tag), output);
}

template<typename T, size_t N>
bool write_multi_array(const std::string & fn, boost::multi_array<T,N> slice){
    bio::mapped_file_params f_param;
    bio::mapped_file_sink f;
    size_t bytes = sizeof(T)*slice.num_elements();
    f_param.path = fn;
    f_param.new_file_size = bytes;
    f.open(f_param);
    assert(f.is_open());
    memcpy(f.data(), slice.data(), bytes);
    f.close();
    return true;
}

template < typename T >
inline bool
write_volume( const std::string& fname,
              const volume_ptr<T>& seg_ptr)
{
    using range = boost::multi_array_types::index_range;

    volume<T>& seg = *seg_ptr;
    auto shape = seg.shape();
    return write_multi_array(fname, boost::multi_array<T,3>(seg[boost::indices[range(1,shape[0]-1)][range(1,shape[1]-1)][range(1,shape[2]-1)]], boost::fortran_storage_order()));
}

template<typename ID, typename F>
void write_chunk_boundaries(const volume_ptr<ID>& seg_ptr, const affinity_graph_ptr<F>&  aff_ptr, std::array<bool,6> & boundary_flags, const char * tag)
{
    using range = boost::multi_array_types::index_range;

    affinity_graph<F>& aff = *aff_ptr;
    volume<ID>& seg = *seg_ptr;
    auto shape = seg.shape();
    if (!boundary_flags[0]) {
        write_multi_array(str(boost::format("aff_i_0_%1%.data") % tag), boost::multi_array<F,2>(aff[boost::indices[1][range(1,shape[1]-1)][range(1,shape[2]-1)][0]], boost::fortran_storage_order()));
    }
    if (!boundary_flags[1]) {
        write_multi_array(str(boost::format("aff_i_1_%1%.data") % tag), boost::multi_array<F,2>(aff[boost::indices[range(1,shape[0]-1)][1][range(1,shape[2]-1)][1]], boost::fortran_storage_order()));
    }
    if (!boundary_flags[2]) {
        write_multi_array(str(boost::format("aff_i_2_%1%.data") % tag), boost::multi_array<F,2>(aff[boost::indices[range(1,shape[0]-1)][range(1,shape[1]-1)][1][2]], boost::fortran_storage_order()));
    }

    if (!boundary_flags[0]) {
        write_multi_array(str(boost::format("seg_o_0_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[0][range(1,shape[1]-1)][range(1,shape[2]-1)]], boost::fortran_storage_order()));
        write_multi_array(str(boost::format("seg_i_0_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[1][range(1,shape[1]-1)][range(1,shape[2]-1)]], boost::fortran_storage_order()));
    }
    if (!boundary_flags[1]) {
        write_multi_array(str(boost::format("seg_o_1_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range(1,shape[0]-1)][0][range(1,shape[2]-1)]], boost::fortran_storage_order()));
        write_multi_array(str(boost::format("seg_i_1_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range(1,shape[0]-1)][1][range(1,shape[2]-1)]], boost::fortran_storage_order()));
    }
    if (!boundary_flags[2]) {
        write_multi_array(str(boost::format("seg_o_2_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range(1,shape[0]-1)][range(1,shape[1]-1)][0]], boost::fortran_storage_order()));
        write_multi_array(str(boost::format("seg_i_2_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range(1,shape[0]-1)][range(1,shape[1]-1)][1]], boost::fortran_storage_order()));
    }

    if (!boundary_flags[3]) {
        write_multi_array(str(boost::format("seg_i_3_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[shape[0]-2][range(1,shape[1]-1)][range(1,shape[2]-1)]], boost::fortran_storage_order()));
        write_multi_array(str(boost::format("seg_o_3_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[shape[0]-1][range(1,shape[1]-1)][range(1,shape[2]-1)]], boost::fortran_storage_order()));
    }
    if (!boundary_flags[4]) {
        write_multi_array(str(boost::format("seg_i_4_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range(1,shape[0]-1)][shape[1]-2][range(1,shape[2]-1)]], boost::fortran_storage_order()));
        write_multi_array(str(boost::format("seg_o_4_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range(1,shape[0]-1)][shape[1]-1][range(1,shape[2]-1)]], boost::fortran_storage_order()));
    }
    if (!boundary_flags[5]) {
        write_multi_array(str(boost::format("seg_i_5_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range(1,shape[0]-1)][range(1,shape[1]-1)][shape[2]-2]], boost::fortran_storage_order()));
        write_multi_array(str(boost::format("seg_o_5_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range(1,shape[0]-1)][range(1,shape[1]-1)][shape[2]-1]], boost::fortran_storage_order()));
    }
}


template< class C >
struct is_numeric:
    std::integral_constant<bool,
                           std::is_integral<C>::value ||
                           std::is_floating_point<C>::value> {};
