#include "MeanEdge.hpp"
#include "ReweightedLocalMeanEdge.hpp"
#include <cstdint>
#include <fstream>
#include <boost/format.hpp>

template<typename Ta, typename Ts>
Edge<Ta> load_edge(Ts seg1, Ts seg2)
{
    int i,x,y,z;
    Ta affinity;
    Edge<Ta> edge;
    std::ifstream in(str(boost::format("edges/%1%_%2%.dat") % seg1 % seg2));
    while (in >> i >> x >> y >> z >> affinity) {
        Coord coord({x,y,z});
        edge[i][coord] = affinity;
    }
    in.close();
    return edge;
}

int main(int argc, char * argv[])
{
    int32_t seg1, seg2;
    std::ifstream edge_list(argv[1]);
    while (edge_list >> seg1 >> seg2){
        auto edge = load_edge<float, int32_t>(seg1, seg2);
        auto me = meanAffinity<float, int32_t>(edge);
        auto rlme = reweightedLocalMeanAffinity<float, int32_t>(edge);
        std::cout << seg1 << " " << seg2 << " " << me.first << " " << me.second << " ";
        std::cout << seg1 << " " << seg2 << " " << rlme.first << " " << rlme.second << std::endl;
    }
    edge_list.close();
}
