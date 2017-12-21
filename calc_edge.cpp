#include "Types.h"
#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <boost/format.hpp>
#include <QtConcurrent>

template<typename Ta, typename Ts>
Edge<Ta> loadEdge(Ts seg1, Ts seg2)
{
    int i,x,y,z;
    Ta affinity;
    Edge<Ta> edge;
    std::ifstream in(str(boost::format("edges/%1%_%2%.dat") % seg1 % seg2));
    while (in.read(reinterpret_cast<char *>(&i), sizeof(i))) {
        in.read(reinterpret_cast<char *>(&x), sizeof(x));
        in.read(reinterpret_cast<char *>(&y), sizeof(y));
        in.read(reinterpret_cast<char *>(&z), sizeof(z));
        in.read(reinterpret_cast<char *>(&affinity), sizeof(affinity));
        Coord coord({x,y,z});
        edge[i][coord] = affinity;
    }
    in.close();
    return edge;
}

int main(int argc, char * argv[])
{
    using namespace std::placeholders;

    seg_t seg1, seg2;
    std::vector<SegPair<seg_t> > complete_segpairs;
    std::unordered_map<SegPair<seg_t>, Edge<aff_t>, boost::hash<SegPair<seg_t> > > edges;
    std::ifstream edge_list(argv[1]);
    std::vector<std::pair<aff_t, seg_t> > me;
    while (edge_list >> seg1 >> seg2){
        auto p = std::make_pair(seg1, seg2);
        complete_segpairs.push_back(p);
        edges[p] = loadEdge<aff_t, seg_t>(seg1, seg2);
        me.push_back(meanAffinity<aff_t, seg_t>(edges.at(p)));
    }
    edge_list.close();

    std::vector<size_t> areas(complete_segpairs.size(),0);
    std::vector<aff_t> affinities(complete_segpairs.size(),0);
    std::vector<size_t> idx(complete_segpairs.size());
    std::iota(idx.begin(), idx.end(), 1);

    auto f_rlme = QtConcurrent::map(idx, [&](auto i)
            {auto s = reweightedLocalMeanAffinity<aff_t,seg_t>(edges.at(complete_segpairs.at(i-1)));
             affinities[i-1] = s.first;
             areas[i-1] = s.second;
             });
    f_rlme.waitForFinished();

    std::ofstream ofs("new_edges.dat", std::ios_base::binary);

    for (size_t i = 0; i != complete_segpairs.size(); i++) {
        auto & p = complete_segpairs[i];
        ofs << std::setprecision (17) << p.first << " " << p.second << " " << me[i].first << " " << me[i].second << " ";
        ofs << std::setprecision (17) << p.first << " " << p.second << " " << affinities[i] << " " << areas[i] << "\n";
    }
    ofs.close();
}
