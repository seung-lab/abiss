#pragma once

#include "types.hpp"

#include <iostream>

#define CW_FOR_2( type, v1, f1, t1, v2, f2, t2 )                \
    for ( type v1 = f1; v1 < t1; ++v1 )                         \
        for ( type v2 = f2; v2 < t2; ++v2 )


#define CW_FOR_3( type, v1, f1, t1, v2, f2, t2, v3, f3, t3 )    \
    CW_FOR_2( type, v1, f1, t1, v2, f2, t2 )                    \
    for ( type v3 = f3; v3 < t3; ++v3 )

template< typename ID, typename F, typename L, typename H >
inline std::tuple< volume_ptr<ID>, std::vector<std::size_t> >
watershed( const affinity_graph_ptr<F>& aff_ptr, const L& lowv, const H& highv )
{
    using affinity_t = F;
    using id_t       = ID;
    using traits     = watershed_traits<id_t>;
    using index = std::ptrdiff_t;

    affinity_t low  = static_cast<affinity_t>(lowv);
    affinity_t high = static_cast<affinity_t>(highv);

    index xdim = aff_ptr->shape()[0];
    index ydim = aff_ptr->shape()[1];
    index zdim = aff_ptr->shape()[2];

    index size = xdim * ydim * zdim;

    std::tuple< volume_ptr<id_t>, std::vector<std::size_t> > result
        ( volume_ptr<id_t>( new volume<id_t>(boost::extents[xdim][ydim][zdim],
                                           boost::fortran_storage_order())),
          std::vector<std::size_t>(1) );

    auto& counts = std::get<1>(result);
    counts[0] = 0;

    affinity_graph<F>& aff = *aff_ptr;
    volume<id_t>&      seg = *std::get<0>(result);

    id_t* seg_raw = seg.data();

    CW_FOR_3( index, z, 0, zdim, y, 0, ydim, x, 0, xdim )
    {
        id_t& id = seg[x][y][z] = 0;

        F negx = (x>0) ? aff[x][y][z][0] : low;
        F negy = (y>0) ? aff[x][y][z][1] : low;
        F negz = (z>0) ? aff[x][y][z][2] : low;
        F posx = (x<(xdim-1)) ? aff[x+1][y][z][0] : low;
        F posy = (y<(ydim-1)) ? aff[x][y+1][z][1] : low;
        F posz = (z<(zdim-1)) ? aff[x][y][z+1][2] : low;

        F m = std::max({negx,negy,negz,posx,posy,posz});

        if ( m > low )
        {
            if ( negx == m || negx >= high ) { id |= 0x01; }
            if ( negy == m || negy >= high ) { id |= 0x02; }
            if ( negz == m || negz >= high ) { id |= 0x04; }
            if ( posx == m || posx >= high ) { id |= 0x08; }
            if ( posy == m || posy >= high ) { id |= 0x10; }
            if ( posz == m || posz >= high ) { id |= 0x20; }
        }
    }


    const index dir[6] = { -1, -xdim, -xdim*ydim, 1, xdim, xdim*ydim };
    const id_t dirmask[6]  = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20 };
    const id_t idirmask[6] = { 0x08, 0x10, 0x20, 0x01, 0x02, 0x04 };

    id_t next_id = 1;
    {
        std::vector<index> bfs(size+1,0);
        index  bfs_index = 0;
        index  bfs_start = 0;
        index  bfs_end   = 0;

        CW_FOR_3( index, iz, 0, zdim, iy, 0, ydim, ix, 0, xdim )
        {
            index idx = &seg[ix][iy][iz] - seg_raw;

            if ( !(seg_raw[ idx ] & traits::high_bit) )
            {
                bfs[ bfs_end++ ] = idx;
                if (seg_raw[idx] == 0) {
                    seg_raw[idx] |= traits::high_bit;
                    bfs_start = bfs_end;
                    bfs_index = bfs_end;
                } else {
                    seg_raw[ idx ] |= 0x40;
                }

                while ( bfs_index != bfs_end )
                {
                    index y = bfs[ bfs_index++ ];

                    for ( index d = 0; d < 6; ++d )
                    {
                        if ( seg_raw[ y ] & dirmask[ d ] )
                        {
                            index z = y + dir[ d ];

                            if ( seg_raw[ z ] & traits::high_bit )
                            {
                                const id_t seg_id = seg_raw[ z ];
                                while ( bfs_start != bfs_end )
                                {
                                    seg_raw[ bfs[ bfs_start++ ] ] = seg_id;
                                }
                                bfs_index = bfs_end;
                                d = 6; // (break)
                            }
                            else if ( !( seg_raw[ z ] & 0x40) )
                            {
                                seg_raw[ z ] |= 0x40;
                                if ( !( seg_raw[ z ] & idirmask[ d ] ) )  // dfs now
                                {
                                    bfs_index = bfs_end;
                                    d = 6; // (break)
                                }
                                bfs[ bfs_end++ ] = z;
                            }
                        }
                    }
                }

                if ( bfs_start != bfs_end )
                {
                    while ( bfs_start != bfs_end )
                    {
                        seg_raw[ bfs[ bfs_start++ ] ] = traits::high_bit | next_id;
                    }
                    ++next_id;
                }
            }
        }
    }

    std::cout << "found: " << (next_id-1) << " components\n";

    counts.resize(next_id,0);

    for ( index idx = 0; idx < size; ++idx )
    {
        if (seg_raw[idx] & traits::high_bit) {
            seg_raw[idx] &= traits::mask;
        } else {
            std::cout << "not assigned" << std::endl;
            seg_raw[idx] = 0;
        }
        ++counts[seg_raw[idx]];
    }

    return result;
}
