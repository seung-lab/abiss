#include "defines.h"
#include "types.hpp"
#include "utils.hpp"
#include "agglomeration.hpp"
#include "mmap_array.hpp"
#include <vector>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <boost/functional/hash.hpp>
#include <boost/pending/disjoint_sets.hpp>
#include <ctime>
#include <boost/format.hpp>

template<typename T>
std::vector<std::pair<T, T> > load_remaps(size_t data_size)
{
    std::vector<std::pair<T, T> > remap_vector;
    if (data_size > 0) {
        MMArray<std::pair<T, T>, 1> remap_data("ongoing.data", std::array<size_t, 1>({data_size}));
        auto data = remap_data.data();
        std::copy(std::begin(data), std::end(data), std::back_inserter(remap_vector));

        std::stable_sort(std::begin(remap_vector), std::end(remap_vector), [](auto & a, auto & b) { return std::get<0>(a) < std::get<0>(b); });
    }
    return remap_vector;
}

template<typename T>
std::unordered_map<T, size_t> load_sizes(size_t data_size)
{
    MMArray<std::pair<T, size_t>, 1> count_data("counts.data",std::array<size_t, 1>({data_size}));
    auto counts = count_data.data();
    std::unordered_map<T, size_t> sizes(data_size);
    for (size_t i = 0; i != data_size; i++) {
        sizes[counts[i].first] = counts[i].second;
    }
    return sizes;
}

template<typename ID, typename F>
region_graph<ID,F> load_dend(size_t data_size)
{
    region_graph<ID, F> rg;
    MMArray<std::tuple<F, ID, ID>, 1> dend_data("dend.data",std::array<size_t, 1>({data_size}));
    auto dend_tuple = dend_data.data();
    for (size_t i = 0; i != data_size; i++) {
        rg.push_back(dend_tuple[i]);
    }
    return rg;
}

template<typename ID, typename F>
std::tuple<std::unordered_map<ID, ID>, size_t, size_t>
process_chunk_borders(size_t face_size, std::unordered_map<ID, size_t> & sizes, region_graph<ID, F> & rg, const std::string & tag, size_t remap_size, size_t ac_offset)
{
    using traits = watershed_traits<ID>;
    using rank_t = std::unordered_map<ID,std::size_t>;
    using parent_t = std::unordered_map<ID,ID>;
    rank_t rank_map;
    parent_t parent_map;

    boost::associative_property_map<rank_t> rank_pmap(rank_map);
    boost::associative_property_map<parent_t> parent_pmap(parent_map);

    boost::disjoint_sets<boost::associative_property_map<rank_t>, boost::associative_property_map<parent_t> > sets(rank_pmap, parent_pmap);
    std::unordered_map<ID, F> descent(sizes.size());

    sets.make_set(0);
    std::vector<ID> segids(sizes.size());

    for (auto & kv : sizes) {
        sets.make_set(kv.first);
        descent[kv.first] = HIGH_THRESHOLD;
        segids.push_back(kv.first);
    }

    std::stable_sort(std::begin(segids), std::end(segids));

    std::unordered_set<id_pair<ID>, boost::hash<id_pair<ID> > > same;
    std::unordered_map<id_pair<ID>, F, boost::hash<id_pair<ID> > > edges;

    auto fi_data = MMArray<ID, 1>("seg_fi.data", std::array<size_t, 1>({face_size}));
    auto fo_data = MMArray<ID, 1>("seg_fo.data", std::array<size_t, 1>({face_size}));
    auto bi_data = MMArray<ID, 1>("seg_bi.data", std::array<size_t, 1>({face_size}));
    auto bo_data = MMArray<ID, 1>("seg_bo.data", std::array<size_t, 1>({face_size}));
    auto conn_data = MMArray<F, 1>("aff_b.data", std::array<size_t, 1>({face_size}));

    auto fi = fi_data.data();
    auto fo = fo_data.data();
    auto bi = bi_data.data();
    auto bo = bo_data.data();
    auto conn = conn_data.data();

    clock_t begin = clock();

    for (size_t idx = 0; idx != face_size; idx++) {
        if ( fi[idx] && bi[idx] ) {
            bool needs_an_edge = false;
            id_pair<ID> xp = std::minmax(fi[idx], bi[idx]);
            if ( conn[idx] >= LOW_THRESHOLD ) {
                if ( fo[idx] ) {
                    if (fi[idx] != fo[idx]) {
                        std::cout << "something is wrong in fo" << std::endl;
                    }
                    if ( conn[idx] >= HIGH_THRESHOLD ) {
                        if (bi[idx] != bo[idx]) {
                            std::cout << "something is wrong in merge" << std::endl;
                        }
                        same.insert(xp);
                    } else {
                        needs_an_edge = true;
                        if (descent[fi[idx]] != HIGH_THRESHOLD && descent[fi[idx]] != conn[idx]) {
                            std::cout << "This should not happen in a" << std::endl;
                            std::cout << fi[idx] << " " << bi[idx] << std::endl;
                            std::cout << descent[fi[idx]] << " " << conn[idx] << std::endl;
                        }
                        descent[fi[idx]] = conn[idx];
                    }
                } else if ( bo[idx] ) {
                    if (bi[idx] != bo[idx]) {
                        std::cout << "something is wrong in bo" << std::endl;
                    }
                    if ( conn[idx] >= HIGH_THRESHOLD ) {
                        if (fi[idx] != fo[idx]) {
                            std::cout << "something is wrong in merge" << std::endl;
                        }
                        same.insert(xp);
                    } else {
                        needs_an_edge = true;
                        if (descent[bi[idx]] != HIGH_THRESHOLD && descent[bi[idx]] != conn[idx]) {
                            std::cout << "This should not happen in b" << std::endl;
                            std::cout << fi[idx] << " " << bi[idx] << std::endl;
                            std::cout << descent[bi[idx]] << " " << conn[idx] << std::endl;
                        }
                        descent[bi[idx]] = conn[idx];
                    }
                } else {
                    if (conn[idx] >= HIGH_THRESHOLD) {
                        std::cout << "something is wrong in edge" << std::endl;
                    }
                    needs_an_edge = true;
                }
                if (needs_an_edge) {
                    F & f = edges[xp];
                    if (f < conn[idx]) {
                        f = conn[idx];
                    }
                }
            }
        }
    }
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

    std::cout << "merge faces in " << elapsed_secs << " seconds" << std::endl;

    std::cout << edges.size() << " edges and " << same.size() << " mergers" << std::endl;
    begin = clock();
    for (auto & kv : edges) {
        auto & p = kv.first;
        rg.emplace_back(kv.second, p.first, p.second);
    }
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "populate region graph in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    std::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "sort region graph in " << elapsed_secs << " seconds" << std::endl;
    begin = clock();
    for (auto & p : same) {
        const ID v1 = sets.find_set( p.first );
        const ID v2 = sets.find_set( p.second );
        if (v1 != v2) {
            sets.link(v1,v2);
            const ID vr = sets.find_set(v1);
            sizes[v1] += sizes[v2]&(~traits::on_border);
            sizes[v1] |= sizes[v2]&traits::on_border;
            sizes[v2]  = 0;
            std::swap( sizes[vr], sizes[v1] );
            descent[vr] = HIGH_THRESHOLD;
        }
    }
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "merge plateau in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    size_t n_merger = 0;
    for (auto & t : rg) {
        const F val = std::get<0>(t);
        const ID v1 = sets.find_set( std::get<1>(t) );
        const ID v2 = sets.find_set( std::get<2>(t) );

        if (val < LOW_THRESHOLD) {
            break;
        }

        if (v1 == v2) {
            continue;
        }

        if ( descent[v1] == val || descent[v2] == val ) {
            sets.link(v1,v2);
            const ID vr = sets.find_set(v1);
            sizes[v1] += sizes[v2]&(~traits::on_border);
            sizes[v1] |= sizes[v2]&traits::on_border;
            sizes[v2]  = 0;
            std::swap( sizes[vr], sizes[v1] );
            descent[vr] = std::max( descent[v1], descent[v2] );
            n_merger += 1;
            if ( descent[vr] != val )
            {
                descent[vr] = HIGH_THRESHOLD;
            }
        }
    }
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "merge descent in " << elapsed_secs << " seconds" << std::endl;
    std::cout << n_merger << " mergers" << std::endl;

    std::cout << "merge" << std::endl;
    begin = clock();
    n_merger = 0;
    for (auto & t : rg) {
        const F val = std::get<0>(t);
        const ID v1 = sets.find_set( std::get<1>(t) );
        const ID v2 = sets.find_set( std::get<2>(t) );

        if (val < LOW_THRESHOLD) {
            break;
        }

        if ( v1 != v2 && v1 && v2 ) {
            if (try_merge(sizes, sets, v1, v2, SIZE_THRESHOLD))
                n_merger += 1;
        }
    }
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "merge region graph in " << elapsed_secs << " seconds" << std::endl;
    std::cout << n_merger << " mergers" << std::endl;

    parent_t remaps(sizes.size());

    ID next_id = 0;

    std::unordered_set<ID> relevant_supervoxels;

    begin = clock();

    for ( auto & kv : sizes ) {
        const ID v = kv.first;
        size_t size = kv.second;
        const ID s = sets.find_set(v);
        if (sizes[s] >= SIZE_THRESHOLD) {
            remaps[v] = s;
        } else {
            remaps[v] = 0;
        }

        if ( (size & (~traits::on_border)) && size >= SIZE_THRESHOLD  ) {
            if (s != v) {
                std::cout << "s("<<s<<") != v("<<v<<")" << std::endl;
            }
            if (size & traits::on_border) {
                relevant_supervoxels.insert(s);
            }

            ++next_id;
        }
    }

    free_container(rank_map);
    free_container(parent_map);

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "generate remap in " << elapsed_secs << " seconds" << std::endl;

    std::unordered_map<ID, std::set<ID> > in_rg;

    region_graph<ID,F> new_rg;


    rank_t rank_mst_map;
    parent_t parent_mst_map;

    boost::associative_property_map<rank_t> rank_mst_pmap(rank_mst_map);
    boost::associative_property_map<parent_t> parent_mst_pmap(parent_mst_map);

    boost::disjoint_sets<boost::associative_property_map<rank_t>, boost::associative_property_map<parent_t> > mst(rank_mst_pmap, parent_mst_pmap);
    begin = clock();

    mst.make_set(0);

    for (auto & kv : sizes) {
        mst.make_set(kv.first);
    }

    for ( auto& it: rg )
    {
        ID s1 = remaps[std::get<1>(it)];
        ID s2 = remaps[std::get<2>(it)];
        //ID s1 = sets.find_set(std::get<1>(it));
        //ID s2 = sets.find_set(std::get<2>(it));
        ID a1 = mst.find_set(s1);
        ID a2 = mst.find_set(s2);

        if ( a1 != a2 && a1 && a2 )
        {
            mst.link(a1, a2);
            auto mm = std::minmax(s1,s2);
            if ( in_rg[mm.first].count(mm.second) == 0 && ((sizes[s1] & traits::on_border) && (sizes[s2] & traits::on_border)))
            {
                new_rg.emplace_back(std::get<0>(it), mm.first, mm.second);
                in_rg[mm.first].insert(mm.second);
            }
        }
    }

    auto d = write_vector(str(boost::format("dend_%1%.data") % tag), new_rg);
    free_container(in_rg);
    free_container(new_rg);
    free_container(rank_mst_map);
    free_container(parent_mst_map);

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "generate MST in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    std::vector<std::pair<ID, size_t> > counts;
    for (const auto & s : relevant_supervoxels) {
        counts.emplace_back(s,sizes[s]&(~traits::on_border));
    }

    auto c = write_vector(str(boost::format("counts_%1%.data") % tag), counts);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "write supervoxel sizes in " << elapsed_secs << " seconds" << std::endl;

    size_t current_ac = std::numeric_limits<std::size_t>::max();
    std::ofstream of_done;
    std::ofstream of_ongoing;
    of_ongoing.open(str(boost::format("ongoing_%1%.data") % tag));

    auto remap_vector = load_remaps<ID>(remap_size);

    begin = clock();

    for (auto & kv : remap_vector) {
        auto & s = kv.first;
        if (current_ac != (s - (s % ac_offset))) {
            current_ac = s - (s % ac_offset);
            if (of_done.is_open()) {
                of_done.close();
            }
            of_done.open(str(boost::format("done_%1%_%2%.data") % tag % current_ac));
        }
        const auto seg = remaps[kv.second];
        const auto size = sizes[seg];
        if (relevant_supervoxels.count(seg) > 0) {
            of_ongoing.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
            of_ongoing.write(reinterpret_cast<const char *>(&(seg)), sizeof(ID));
        } else {
            of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
            of_done.write(reinterpret_cast<const char *>(&(seg)), sizeof(ID));
        }
    }

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "update remaps in " << elapsed_secs << " seconds" << std::endl;

    current_ac = std::numeric_limits<std::size_t>::max();

    begin = clock();
    for (auto & s : segids) {
        if (current_ac != (s - (s % ac_offset))) {
            current_ac = s - (s % ac_offset);
            if (of_done.is_open()) {
                of_done.close();
            }
            of_done.open(str(boost::format("done_%1%_%2%.data") % tag % current_ac), std::ios_base::app);
        }
        const auto seg = remaps[s];
        const auto size = sizes[seg];
        if (relevant_supervoxels.count(seg) > 0) {
            of_ongoing.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
            of_ongoing.write(reinterpret_cast<const char *>(&(seg)), sizeof(ID));
        } else {
            of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
            of_done.write(reinterpret_cast<const char *>(&(seg)), sizeof(ID));
        }
    }

    of_done.close();
    of_ongoing.close();

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "generate new remap in " << elapsed_secs << " seconds" << std::endl;

    std::cout << "number of supervoxels:" << remaps.size() << "," << next_id << std::endl;
    return std::make_tuple(remaps, c, d);
}

template<typename T>
void mark_border_supervoxels(std::unordered_map<T, size_t> & sizes, const std::array<bool,6> & boundary_flags, const std::array<size_t, 6> & face_dims, const std::string & tag)
{
    using traits = watershed_traits<T>;
    for (size_t i = 0; i != 6; i++) {
        if (!boundary_flags[i]) {
            auto fn = str(boost::format("seg_i_%1%_%2%.data") % i % tag);
            std::cout << "loading: " << fn << std::endl;
            MMArray<T, 1> face_array(fn,std::array<size_t, 1>({face_dims[i]}));
            auto face_data = face_array.data();
            for (size_t j = 0; j != face_dims[i]; j++) {
                if (sizes.count(face_data[j]) != 0 && face_data[j]!=0) {
                    sizes[face_data[j]] |= traits::on_border;
                } else {
                    if (face_data[j] != 0) {
                        std::cout << "supervoxels does not exist" << std::endl;
                    }
                }
            }
        }
    }
    //std::cout << "check supervoxel: 240854 " << sizes[240854] << std::endl;
    //std::cout << "check supervoxel: 240855 " << sizes[240855] << std::endl;
}

template<typename T>
void update_border_supervoxels(const std::unordered_map<T, T> & remaps, const std::array<bool,6> & boundary_flags, const std::array<size_t, 6> & face_dims, const std::string & tag)
{
    for (size_t i = 0; i != 6; i++) {
        if (!boundary_flags[i]) {
            auto fi = str(boost::format("seg_i_%1%_%2%.data") % i % tag);
            std::cout << "update: " << fi << ",size:" << face_dims[i] << std::endl;
            MMArray<T, 1> face_i_array(fi,std::array<size_t, 1>({face_dims[i]}));
            auto face_i_data = face_i_array.data();
            for (size_t j = 0; j != face_dims[i]; j++) {
                if (remaps.count(face_i_data[j]) > 0) {
                    face_i_data[j] = remaps.at(face_i_data[j]);
                }
            }
            auto fo = str(boost::format("seg_o_%1%_%2%.data") % i % tag);
            std::cout << "update: " << fo << ",size:" << face_dims[i] << std::endl;
            MMArray<T, 1> face_o_array(fo,std::array<size_t, 1>({face_dims[i]}));
            auto face_o_data = face_o_array.data();
            for (size_t j = 0; j != face_dims[i]; j++) {
                if (remaps.count(face_o_data[j]) > 0) {
                    face_o_data[j] = remaps.at(face_o_data[j]);
                }
            }
        }
    }
}

int main(int argc, char* argv[])
{
    size_t xdim,ydim,zdim;
    int flag;
    size_t face_size, counts, dend_size, remap_size, ac_offset;
    std::ifstream param_file(argv[1]);
    const char * tag = argv[2];
    param_file >> xdim >> ydim >> zdim;
    std::cout << xdim << " " << ydim << " " << zdim << std::endl;
    std::array<bool,6> flags({true,true,true,true,true,true});
    for (size_t i = 0; i != 6; i++) {
        param_file >> flag;
        flags[i] = (flag > 0);
        if (flags[i]) {
            std::cout << "real boundary: " << i << std::endl;
        }
    }
    param_file >> face_size >> counts >> dend_size >> remap_size >> ac_offset;

    if (face_size == 0) {
        std::cout << "Nothing to merge, exit!" << std::endl;
        return 0;
    }

    std::cout << "supervoxel id offset:" << face_size << " " << counts << " " << dend_size << std::endl;
    auto sizes = load_sizes<seg_t>(counts);
    clock_t begin = clock();
    mark_border_supervoxels(sizes, flags, std::array<size_t, 6>({ydim*zdim, xdim*zdim, xdim*ydim, ydim*zdim, xdim*zdim, xdim*ydim}), tag);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "mark boundary supervoxels in " << elapsed_secs << " seconds" << std::endl;
    begin = clock();
    auto dend = load_dend<seg_t, aff_t>(dend_size);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "load dend in " << elapsed_secs << " seconds" << std::endl;

    std::unordered_map<seg_t, seg_t> remaps;
    size_t c = 0;
    size_t d = 0;

    std::tie(remaps, c, d) = process_chunk_borders<seg_t, aff_t>(face_size, sizes, dend, tag, remap_size, ac_offset);
    update_border_supervoxels(remaps, flags, std::array<size_t, 6>({ydim*zdim, xdim*zdim, xdim*ydim, ydim*zdim, xdim*zdim, xdim*ydim}), tag);
    auto m = write_remap(remaps, tag);
    std::vector<size_t> meta({xdim,ydim,zdim,c,d,m});
    write_vector(str(boost::format("meta_%1%.data") % tag), meta);
    std::cout << "num of sv:" << c << std::endl;
    std::cout << "size of rg:" << d << std::endl;
    std::cout << "num of remaps:" << m << std::endl;
}
