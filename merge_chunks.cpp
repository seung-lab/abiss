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
#include <cstdlib>
#include <boost/format.hpp>
#include <parallel/algorithm>

template<typename T>
std::vector<std::pair<T, T> > load_remaps(size_t data_size)
{
    std::vector<std::pair<T, T> > remap_vector;
    if (data_size > 0) {
        MMArray<std::pair<T, T>, 1> remap_data("ongoing.data", std::array<size_t, 1>({data_size}));
        auto data = remap_data.data();
        std::copy(std::begin(data), std::end(data), std::back_inserter(remap_vector));

        __gnu_parallel::stable_sort(std::begin(remap_vector), std::end(remap_vector), [](auto & a, auto & b) { return std::get<0>(a) < std::get<0>(b); });
    }
    return remap_vector;
}

template<typename T>
std::vector<std::pair< T, size_t> >  load_sizes(size_t data_size)
{
    if (data_size > 0) {
        MMArray<std::pair<T, size_t>, 1> count_data("counts.data",std::array<size_t, 1>({data_size}));
        auto counts = count_data.data();
        std::vector<std::pair<T, size_t> > sizes(counts.begin(), counts.end());
        return sizes;
    } else {
        return std::vector<std::pair<T, size_t> >();
    }
}

template<typename ID, typename F>
region_graph<ID,F> load_dend(size_t data_size)
{
    if (data_size > 0) {
        MMArray<std::tuple<F, ID, ID>, 1> dend_data("dend.data",std::array<size_t, 1>({data_size}));
        auto dend_tuple = dend_data.data();
        region_graph<ID, F> rg(dend_tuple.begin(), dend_tuple.end());
        return rg;
    } else {
        return region_graph<ID,F>();
    }
}

template<typename ID, typename F>
std::tuple<std::unordered_map<ID, ID>, size_t, size_t>
process_chunk_borders(size_t face_size, std::vector<std::pair<ID, size_t> > & size_pairs, region_graph<ID, F> & rg, auto high_threshold, auto low_threshold, auto size_threshold, const std::string & tag, size_t remap_size, size_t ac_offset)
{
    std::vector<size_t> sizes;
    std::vector<ID> segids;
    std::vector<std::pair<size_t, ID>> reverse_lookup_pairs;
    sizes.reserve(size_pairs.size());
    segids.reserve(size_pairs.size());
    reverse_lookup_pairs.reserve(size_pairs.size());
    segids.push_back(0);
    sizes.push_back(0);
    reverse_lookup_pairs.push_back(std::make_pair(size_t(0),ID(0)));
    __gnu_parallel::stable_sort(std::begin(size_pairs), std::end(size_pairs), [](auto & a, auto & b) { return a.first < b.first; });
    clock_t begin = clock();
    std::cout << size_pairs.size() << "supervoxels to populate" << std::endl;
    for (auto & kv : size_pairs) {
        if (kv.first == 0 || kv.second == 0) {
            std::cerr << "Impossible segid: " << kv.first << " or size: " << kv.second << std::endl;
            std::abort();
        }
        reverse_lookup_pairs.push_back(std::make_pair(kv.first, segids.size()));
        segids.push_back(kv.first);
        sizes.push_back(kv.second);
    }
    std::unordered_map<size_t, ID> reverse_lookup(reverse_lookup_pairs.begin(), reverse_lookup_pairs.end());
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "populate maps in " << elapsed_secs << " seconds" << std::endl;

    using traits = watershed_traits<ID>;
    using rank_t = std::unordered_map<ID,std::size_t>;
    using parent_t = std::unordered_map<ID,ID>;
    std::vector<ID> rank(sizes.size());
    std::vector<ID> parent(sizes.size());
    boost::disjoint_sets<ID*, ID*> sets(&rank[0], &parent[0]);

    std::vector<F> descent(sizes.size(), high_threshold);

    sets.make_set(0);

    for (size_t i = 0; i != segids.size(); i++) {
        sets.make_set(i);
    }

    __gnu_parallel::for_each(std::begin(rg), std::end(rg), [&reverse_lookup](auto & a) { std::get<1>(a) = reverse_lookup.at(std::get<1>(a)); std::get<2>(a) = reverse_lookup.at(std::get<2>(a));});

    std::vector<id_pair<size_t> > same;
    std::unordered_map<id_pair<ID>, F, boost::hash<id_pair<ID> > > edges;

    begin = clock();
    std::vector<ID> vfi;
    std::vector<ID> vfo;
    std::vector<ID> vbi;
    std::vector<ID> vbo;
    auto conn_data = MMArray<F, 1>("aff_b.data", std::array<size_t, 1>({face_size}));
    auto conn = conn_data.data();
{
    auto fi_data = MMArray<ID, 1>("seg_fi.data", std::array<size_t, 1>({face_size}));
    auto fo_data = MMArray<ID, 1>("seg_fo.data", std::array<size_t, 1>({face_size}));
    auto bi_data = MMArray<ID, 1>("seg_bi.data", std::array<size_t, 1>({face_size}));
    auto bo_data = MMArray<ID, 1>("seg_bo.data", std::array<size_t, 1>({face_size}));

    auto fi = fi_data.data();
    auto fo = fo_data.data();
    auto bi = bi_data.data();
    auto bo = bo_data.data();

    __gnu_parallel::transform(fi.begin(), fi.end(), std::back_inserter(vfi), [&reverse_lookup](ID a){
            return reverse_lookup.at(a);
            });

    __gnu_parallel::transform(fo.begin(), fo.end(), std::back_inserter(vfo), [&reverse_lookup](ID a){
            return reverse_lookup.at(a);
            });

    __gnu_parallel::transform(bi.begin(), bi.end(), std::back_inserter(vbi), [&reverse_lookup](ID a){
            return reverse_lookup.at(a);
            });

    __gnu_parallel::transform(bo.begin(), bo.end(), std::back_inserter(vbo), [&reverse_lookup](ID a){
            return reverse_lookup.at(a);
            });

}
    for (size_t idx = 0; idx != face_size; idx++) {
        if ( vfi[idx] && vbi[idx] ) {
            bool needs_an_edge = false;
            //std::cout << "id: " << fi[idx] << ", id: " << bi[idx] << std::endl;
            id_pair<size_t> xp = std::minmax(vfi[idx], vbi[idx]);
            if ( conn[idx] >= low_threshold ) {
                if ( vfo[idx] ) {
                    if (vfi[idx] != vfo[idx]) {
                        std::cerr << "something is wrong in fo" << std::endl;
                        std::abort();
                    }
                    if ( conn[idx] >= high_threshold ) {
                        if (vbi[idx] != vbo[idx]) {
                            std::cerr << "something is wrong in merge" << std::endl;
                            std::abort();
                        }
                        same.push_back(xp);
                    } else {
                        needs_an_edge = true;
                        if (descent[vfi[idx]] != high_threshold && descent[vfi[idx]] != conn[idx]) {
                            std::cerr << "This should not happen in a" << std::endl;
                            std::cerr << vfi[idx] << " " << vbi[idx] << std::endl;
                            std::cerr << descent[vfi[idx]] << " " << conn[idx] << std::endl;
                            std::abort();
                        }
                        descent[vfi[idx]] = conn[idx];
                    }
                } else if ( vbo[idx] ) {
                    if (vbi[idx] != vbo[idx]) {
                        std::cerr << "something is wrong in bo" << std::endl;
                        std::abort();
                    }
                    if ( conn[idx] >= high_threshold ) {
                        if (vfi[idx] != vfo[idx]) {
                            std::cerr << "something is wrong in merge" << std::endl;
                            std::abort();
                        }
                        same.push_back(xp);
                    } else {
                        needs_an_edge = true;
                        if (descent[vbi[idx]] != high_threshold && descent[vbi[idx]] != conn[idx]) {
                            std::cerr << "This should not happen in b" << std::endl;
                            std::cerr << vfi[idx] << " " << vbi[idx] << std::endl;
                            std::cerr << descent[vbi[idx]] << " " << conn[idx] << std::endl;
                            std::abort();
                        }
                        descent[vbi[idx]] = conn[idx];
                    }
                } else {
                    if (conn[idx] >= high_threshold) {
                        std::cerr << "something is wrong in edge" << std::endl;
                        std::abort();
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
    free_container(vfi);
    free_container(vfo);
    free_container(vbi);
    free_container(vbo);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

    std::cout << "merge faces in " << elapsed_secs << " seconds" << std::endl;

    std::cout << edges.size() << " edges and " << same.size() << " mergers" << std::endl;
    begin = clock();
    for (auto & kv : edges) {
        auto & p = kv.first;
        rg.emplace_back(kv.second, p.first, p.second);
    }
    free_container(edges);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "populate region graph in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    __gnu_parallel::stable_sort(std::begin(rg), std::end(rg), [](auto & a, auto & b) { return std::get<0>(a) > std::get<0>(b); });
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
            descent[vr] = high_threshold;
        }
    }
    free_container(same);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "merge plateau in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    size_t n_merger = 0;
    for (auto & t : rg) {
        const F val = std::get<0>(t);
        const ID v1 = sets.find_set( std::get<1>(t) );
        const ID v2 = sets.find_set( std::get<2>(t) );

        if (val < low_threshold) {
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
                descent[vr] = high_threshold;
            }
        }
    }
    free_container(descent);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "merge descent in " << elapsed_secs << " seconds" << std::endl;
    std::cout << n_merger << " mergers" << std::endl;

    std::cout << "merge" << std::endl;
    begin = clock();
    n_merger = 0;
    region_graph<ID,F> res_rg;
    for (auto & t : rg) {
        const F val = std::get<0>(t);
        const ID v1 = sets.find_set( std::get<1>(t) );
        const ID v2 = sets.find_set( std::get<2>(t) );

        if (val < low_threshold) {
            break;
        }

        if ( v1 != v2 && v1 && v2 ) {
            if (try_merge(sizes, sets, v1, v2, size_threshold)) {
                n_merger += 1;
            }
            else {
                res_rg.push_back(t);
            }
        }
    }
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "rg size: " << rg.size() << std::endl;
    std::cout << "res rg size: " << res_rg.size() << std::endl;
    free_container(rg);
    std::cout << "merge region graph in " << elapsed_secs << " seconds" << std::endl;
    std::cout << n_merger << " mergers" << std::endl;

    std::vector<size_t> remaps(sizes.size(), 0);

    ID next_id = 0;

    begin = clock();

    for (size_t v = 0; v != sizes.size(); v++) {
        size_t size = sizes[v];
        const ID s = sets.find_set(v);
        if (sizes[s] >= size_threshold) {
            remaps[v] = s;
        }

        if ( (size & (~traits::on_border)) && size >= size_threshold  ) {
            if (s != v) {
                std::cout << "s("<<s<<") != v("<<v<<")" << std::endl;
            }
            ++next_id;
        }
    }

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "generate remap in " << elapsed_secs << " seconds" << std::endl;

    std::unordered_map<size_t, std::set<size_t> > in_rg;

    region_graph<ID,F> unique_rg;
    region_graph<ID,F> new_rg;

    begin = clock();

    __gnu_parallel::for_each(std::begin(res_rg), std::end(res_rg), [&remaps](auto & a) {
            auto mm = std::minmax(remaps[std::get<1>(a)], remaps[std::get<2>(a)]);
            std::get<1>(a) = mm.first;
            std::get<2>(a) = mm.second;
            });

    __gnu_parallel::stable_sort(std::begin(res_rg), std::end(res_rg), [](auto & a, auto & b) {
            return (std::get<1>(a) < std::get<1>(b)) \
            || ((std::get<1>(a) == std::get<1>(b)) && (std::get<2>(a) < std::get<2>(b))) \
            || ((std::get<1>(a) == std::get<1>(b)) && (std::get<2>(a) == std::get<2>(b)) && (std::get<0>(a) > std::get<0>(b))); });

    __gnu_parallel::unique_copy(std::begin(res_rg), std::end(res_rg), std::back_inserter(unique_rg), [](auto & a, auto & b) {return (std::get<1>(a) == std::get<1>(b) && std::get<2>(a) == std::get<2>(b));});

    __gnu_parallel::stable_sort(std::begin(unique_rg), std::end(unique_rg), [](auto & a, auto & b) {return std::get<0>(a) > std::get<0>(b);});
    //rank_t rank_mst_map;
    //parent_t parent_mst_map;

    //boost::associative_property_map<rank_t> rank_mst_pmap(rank_mst_map);
    //boost::associative_property_map<parent_t> parent_mst_pmap(parent_mst_map);

    //boost::disjoint_sets<boost::associative_property_map<rank_t>, boost::associative_property_map<parent_t> > mst(rank_mst_pmap, parent_mst_pmap);

    for ( auto& it: unique_rg )
    {
        ID s1 = std::get<1>(it);
        ID s2 = std::get<2>(it);
        ID a1 = sets.find_set(s1);
        ID a2 = sets.find_set(s2);

        if ( a1 != a2 && a1 && a2 )
        {
            sets.link(a1, a2);
            if (((sizes[s1] & traits::on_border) && (sizes[s2] & traits::on_border)))
            {
                new_rg.emplace_back(std::get<0>(it), segids[s1], segids[s2]);
            }
        }
    }

    auto d = write_vector(str(boost::format("dend_%1%.data") % tag), new_rg);
    free_container(res_rg);
    free_container(in_rg);
    free_container(new_rg);
    free_container(rank);
    free_container(parent);

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "generate MST in " << elapsed_secs << " seconds" << std::endl;

    begin = clock();
    std::vector<std::pair<ID, size_t> > counts;
    for (size_t v = 0; v != sizes.size(); v++) {
        if (sizes[v] & traits::on_border) {
            counts.emplace_back(segids[v],sizes[v]&(~traits::on_border));
        }
    }

    auto c = write_vector(str(boost::format("counts_%1%.data") % tag), counts);
    free_container(counts);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "write supervoxel sizes in " << elapsed_secs << " seconds" << std::endl;

    size_t current_ac = std::numeric_limits<std::size_t>::max();
    std::ofstream of_done;
    std::ofstream of_ongoing;
    of_ongoing.open(str(boost::format("ongoing_%1%.data") % tag));
    if (!of_ongoing.is_open()) {
        std::cerr << "Failed to open ongoing remap file for " << tag << std::endl;
        std::abort();
    }

    auto remap_vector = load_remaps<ID>(remap_size);
    __gnu_parallel::for_each(std::begin(remap_vector), std::end(remap_vector), [&reverse_lookup](auto & a) { a.second = reverse_lookup.at(a.second);});

    std::unordered_map<ID, ID> reps;

    begin = clock();

    for (auto & kv : remap_vector) {
        auto & s = kv.first;
        if (current_ac != (s - (s % ac_offset))) {
            if (of_done.is_open()) {
                of_done.close();
                if (of_done.is_open()) {
                    std::cerr << "Failed to close done remap file for " << tag << " " << current_ac << std::endl;
                    std::abort();
                }
                reps.clear();
            }
            current_ac = s - (s % ac_offset);
            of_done.open(str(boost::format("remap/done_%1%_%2%.data") % tag % current_ac));
            if (!of_done.is_open()) {
                std::cerr << "Failed to open done remap file for " << tag << " " << current_ac << std::endl;
                std::abort();
            }
        }
        const auto seg = remaps[kv.second];
        const auto size = sizes[seg];
        if (size & traits::on_border) {
            if (reps.count(seg) == 0) {
                of_ongoing.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
                of_ongoing.write(reinterpret_cast<const char *>(&(segids[seg])), sizeof(ID));
                reps[seg] = s;
            } else {
                of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
                of_done.write(reinterpret_cast<const char *>(&(reps.at(seg))), sizeof(ID));
            }
        } else {
            of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
            of_done.write(reinterpret_cast<const char *>(&(segids[seg])), sizeof(ID));
        }
        if (of_done.bad()) {
            std::cerr << "Error occurred when writing done remap file for " << tag << " " << current_ac << std::endl;
            std::abort();
        }
        if (of_ongoing.bad()) {
            std::cerr << "Error occurred when writing ongoing remap file for " << tag << " " << current_ac << std::endl;
            std::abort();
        }
    }

    free_container(remap_vector);

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "update remaps in " << elapsed_secs << " seconds" << std::endl;

    current_ac = std::numeric_limits<std::size_t>::max();

    begin = clock();
    std::vector<ID> segid_tmp(segids.begin(), segids.end());
    __gnu_parallel::for_each(std::begin(segid_tmp), std::end(segid_tmp), [&reverse_lookup](auto & a) { a = reverse_lookup.at(a);});
    for (size_t i = 1; i != segid_tmp.size(); i++) {
        auto s = segids[i];
        auto s_reversed = segid_tmp[i];
        if (current_ac != (s - (s % ac_offset))) {
            if (of_done.is_open()) {
                of_done.close();
                if (of_done.is_open()) {
                    std::cerr << "Failed to close done remap file for " << tag << " " << current_ac << std::endl;
                    std::abort();
                }
                reps.clear();
            }
            current_ac = s - (s % ac_offset);
            of_done.open(str(boost::format("remap/done_%1%_%2%.data") % tag % current_ac), std::ios_base::app);
            if (!of_done.is_open()) {
                std::cerr << "Failed to open done remap file for " << tag << " " << current_ac << std::endl;
                std::abort();
            }
        }
        if (s == 0) {
            std::cerr << "svid = 0, should not happen" << std::endl;
            std::abort();
        }
        const auto seg = remaps[s_reversed];
        const auto size = sizes[seg];
        if (size & traits::on_border) {
            if (reps.count(seg) == 0) {
                of_ongoing.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
                of_ongoing.write(reinterpret_cast<const char *>(&(segids[seg])), sizeof(ID));
                reps[seg] = s;
            } else {
                of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
                of_done.write(reinterpret_cast<const char *>(&(reps.at(seg))), sizeof(ID));
            }
        } else {
            of_done.write(reinterpret_cast<const char *>(&(s)), sizeof(ID));
            of_done.write(reinterpret_cast<const char *>(&(segids[seg])), sizeof(ID));
        }
        if (of_done.bad()) {
            std::cerr << "Error occurred when writing done remap file for " << tag << " " << current_ac << std::endl;
            std::abort();
        }
        if (of_ongoing.bad()) {
            std::cerr << "Error occurred when writing ongoing remap file for " << tag << " " << current_ac << std::endl;
            std::abort();
        }
    }

    of_done.close();
    of_ongoing.close();

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "generate new remap in " << elapsed_secs << " seconds" << std::endl;

    std::cout << "number of supervoxels:" << remaps.size() << "," << next_id << std::endl;
    std::vector<std::pair<ID, ID> > remap_pairs;
    for (size_t i = 0; i != segids.size(); i++) {
        remap_pairs.push_back(std::make_pair(segids[i], segids[remaps[i]]));
    }
    std::unordered_map<ID, ID> remaps_map(remap_pairs.begin(), remap_pairs.end());
    return std::make_tuple(std::move(remaps_map), c, d);
}

template<typename T>
void mark_border_supervoxels(std::vector<std::pair<T, size_t> > & sizes, const std::array<bool,6> & boundary_flags, const std::array<size_t, 6> & face_dims, const std::string & tag)
{
    using traits = watershed_traits<T>;
    for (size_t i = 0; i != 6; i++) {
        if (!boundary_flags[i]) {
            auto fn = str(boost::format("seg_i_%1%_%2%.data") % i % tag);
            std::cout << "loading: " << fn << std::endl;
            MMArray<T, 1> face_array(fn,std::array<size_t, 1>({face_dims[i]}));
            auto face_data = face_array.data();
            std::unordered_set<T> boundary_segs(face_data.begin(), face_data.end());
            __gnu_parallel::for_each(sizes.begin(), sizes.end(), [&boundary_segs](auto & p) {
                if (p.first != 0 && boundary_segs.find(p.first) != boundary_segs.end()) {
                    p.second |= traits::on_border;
                }
            });
            //for (size_t j = 0; j != face_dims[i]; j++) {
            //    if (sizes.count(face_data[j]) != 0 && face_data[j]!=0) {
            //        sizes[face_data[j]] |= traits::on_border;
            //    } else {
            //        if (face_data[j] != 0) {
            //            std::cout << "supervoxels does not exist" << std::endl;
            //        }
            //    }
            //}
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
            __gnu_parallel::for_each(face_i_data.begin(), face_i_data.end(), [&remaps](size_t & a) {
                    auto newid = remaps.find(a);
                    if (newid != remaps.end()) {
                        a = newid->second;
                    }
            });
            auto fo = str(boost::format("seg_o_%1%_%2%.data") % i % tag);
            std::cout << "update: " << fo << ",size:" << face_dims[i] << std::endl;
            MMArray<T, 1> face_o_array(fo,std::array<size_t, 1>({face_dims[i]}));
            auto face_o_data = face_o_array.data();
            __gnu_parallel::for_each(face_o_data.begin(), face_o_data.end(), [&remaps](size_t & a) {
                    auto newid = remaps.find(a);
                    if (newid != remaps.end()) {
                        a = newid->second;
                    }
            });
        }
    }
}

int main(int argc, char* argv[])
{
    size_t xdim,ydim,zdim;
    int flag;
    size_t face_size, counts, dend_size, remap_size, ac_offset;
    std::ifstream param_file(argv[1]);
    std::string ht(argv[2]);
    std::string lt(argv[3]);
    std::string st(argv[4]);
    std::cout << "thresholds: "<< ht << " " << lt << " " << st << std::endl;
    const char * tag = argv[5];
    auto high_threshold = read_float<aff_t>(ht);
    auto low_threshold = read_float<aff_t>(lt);
    auto size_threshold = read_int(st);
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
    clock_t begin = clock();
    auto sizes = load_sizes<seg_t>(counts);
    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "load size in " << elapsed_secs << " seconds" << std::endl;
    begin = clock();
    mark_border_supervoxels(sizes, flags, std::array<size_t, 6>({ydim*zdim, xdim*zdim, xdim*ydim, ydim*zdim, xdim*zdim, xdim*ydim}), tag);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "mark boundary supervoxels in " << elapsed_secs << " seconds" << std::endl;
    begin = clock();
    auto dend = load_dend<seg_t, aff_t>(dend_size);
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    std::cout << "load dend in " << elapsed_secs << " seconds" << std::endl;

    std::unordered_map<seg_t, seg_t> remaps;
    size_t c = 0;
    size_t d = 0;

    std::tie(remaps, c, d) = process_chunk_borders<seg_t, aff_t>(face_size, sizes, dend, high_threshold, low_threshold, size_threshold, tag, remap_size, ac_offset);
    update_border_supervoxels(remaps, flags, std::array<size_t, 6>({ydim*zdim, xdim*zdim, xdim*ydim, ydim*zdim, xdim*zdim, xdim*ydim}), tag);
    //auto m = write_remap(remaps, tag);
    std::vector<size_t> meta({xdim,ydim,zdim,c,d,0});
    write_vector(str(boost::format("meta_%1%.data") % tag), meta);
    std::cout << "num of sv:" << c << std::endl;
    std::cout << "size of rg:" << d << std::endl;
    //std::cout << "num of remaps:" << m << std::endl;
}
