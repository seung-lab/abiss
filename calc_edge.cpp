#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include <cstdint>
#include <fstream>
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
        in.read(reinterpret_cast<char *>(&x), sizeof(y));
        in.read(reinterpret_cast<char *>(&x), sizeof(z));
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

    int32_t seg1, seg2;
    QList<SegPair<int32_t> > complete_segpairs;
    std::unordered_map<SegPair<int32_t>, Edge<float>, boost::hash<SegPair<int32_t> > > edges;
    std::ifstream edge_list(argv[1]);
    while (edge_list >> seg1 >> seg2){
        auto p = std::make_pair(seg1, seg2);
        complete_segpairs.append(p);
        edges[p] = loadEdge<float, int32_t>(seg1, seg2);
    }

    auto me_helper = std::bind(meanAffinity_helper<float, int>, _1, edges);
    auto rlme_helper = std::bind (reweightedLocalMeanAffinity_helper<float, int>, _1, edges);

    QFuture<std::pair<float,int> > f_me = QtConcurrent::mapped(complete_segpairs, me_helper);
    QFuture<std::pair<float,int> > f_rlme = QtConcurrent::mapped(complete_segpairs, rlme_helper);
    auto me = f_me.results();
    auto rlme = f_rlme.results();

    for (int i = 0; i != complete_segpairs.size(); i++) {
        auto & p = complete_segpairs[i];
        std::cout << p.first << " " << p.second << " " << me[i].first << " " << me[i].second << " ";
        std::cout << p.first << " " << p.second << " " << rlme[i].first << " " << rlme[i].second << std::endl;
    }
    edge_list.close();
}
