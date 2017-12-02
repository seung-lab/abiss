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
        in.read(reinterpret_cast<char *>(&affinity), sizeof(z));
        Coord coord({x,y,z});
        edge[i][coord] = affinity;
    }
    in.close();
    return edge;
}

int main(int argc, char * argv[])
{
    using namespace std::placeholders;

    uint64_t seg1, seg2;
    std::vector<SegPair<uint64_t> > complete_segpairs;
    std::unordered_map<SegPair<uint64_t>, Edge<float>, boost::hash<SegPair<uint64_t> > > edges;
    std::ifstream edge_list(argv[1]);
    std::vector<std::pair<float,uint64_t> > me;
    while (edge_list >> seg1 >> seg2){
        auto p = std::make_pair(seg1, seg2);
        complete_segpairs.push_back(p);
        edges[p] = loadEdge<float, uint64_t>(seg1, seg2);
        me.push_back(meanAffinity<float, uint64_t>(edges.at(p)));
    }
    edge_list.close();

    std::vector<uint64_t> areas(complete_segpairs.size(),0);
    std::vector<float> affinities(complete_segpairs.size(),0);
    std::vector<uint64_t> idx(complete_segpairs.size());
    std::iota(idx.begin(), idx.end(), 1);

    auto f_rlme = QtConcurrent::map(idx, [&](auto i)
            {auto s = reweightedLocalMeanAffinity<float,uint64_t>(edges.at(complete_segpairs.at(i-1)));
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
