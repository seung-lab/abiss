#pragma once

#include "types.hpp"

#include <cstdio>
#include <fstream>
#include <type_traits>
#include <boost/format.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

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
    if ( !f ) return false;

    f.write( reinterpret_cast<const char*>(data), n * sizeof(T));
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

template < typename T >
inline bool
write_volume( const std::string& fname,
              const volume_ptr<T>& vol )
{
    std::ofstream f(fname.c_str(), (std::ios::out | std::ios::binary) );
    if ( !f ) return false;

    f.write( reinterpret_cast<char*>(vol->data()),
             vol->shape()[0] * vol->shape()[1] * vol->shape()[2] * sizeof(T));
    return true;
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

    delete [] data;

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

template<typename T, size_t N>
void write_multi_array(const std::string & fn, boost::multi_array<T,N> slice){
    namespace bio = boost::iostreams;
    bio::mapped_file_params f_param;
    bio::mapped_file_sink f;
    size_t bytes = sizeof(T)*slice.num_elements();
    f_param.path = fn;
    f_param.new_file_size = bytes;
    f.open(f_param);
    memcpy(f.data(), slice.data(), bytes);
    f.close();
}

template<typename ID, typename F>
void write_chunk_boundaries(const volume_ptr<ID>& seg_ptr, const affinity_graph_ptr<F>&  aff_ptr, const char * tag)
{
    using range = boost::multi_array_types::index_range;

    affinity_graph<F>& aff = *aff_ptr;
    write_multi_array(str(boost::format("aff_x1_%1%.data") % tag), boost::multi_array<F,2>(aff[boost::indices[1][range()][range()][0]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("aff_y1_%1%.data") % tag), boost::multi_array<F,2>(aff[boost::indices[range()][1][range()][1]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("aff_z1_%1%.data") % tag), boost::multi_array<F,2>(aff[boost::indices[range()][range()][1][2]], boost::fortran_storage_order()));

    volume<ID>& seg = *seg_ptr;
    auto shape = seg.shape();
    write_multi_array(str(boost::format("seg_fx0_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[0][range()][range()]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_fx1_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[1][range()][range()]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_fy0_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range()][0][range()]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_fy1_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range()][1][range()]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_fz0_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range()][range()][0]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_fz1_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range()][range()][1]], boost::fortran_storage_order()));

    write_multi_array(str(boost::format("seg_bx0_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[shape[0]-2][range()][range()]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_bx1_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[shape[0]-1][range()][range()]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_by0_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range()][shape[1]-2][range()]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_by1_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range()][shape[1]-1][range()]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_bz0_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range()][range()][shape[2]-2]], boost::fortran_storage_order()));
    write_multi_array(str(boost::format("seg_bz1_%1%.data") % tag), boost::multi_array<ID,2>(seg[boost::indices[range()][range()][shape[2]-1]], boost::fortran_storage_order()));
}


template< class C >
struct is_numeric:
    std::integral_constant<bool,
                           std::is_integral<C>::value ||
                           std::is_floating_point<C>::value> {};
