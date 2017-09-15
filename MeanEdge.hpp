#ifndef MEAN_EDGE_HPP
#define MEAN_EDGE_HPP

#include "Types.h"

template <typename Ta, typename Ts>
class MeanEdge
{
public:
    MeanEdge(const Edge<Ta> & edge)
        :m_affinity(0), m_area(0) {
        processEdge(edge);
    }
    Ts area() const { return m_area; }
    Ta affinity() const { return m_affinity; }
private:
    void processEdge(const Edge<Ta> & edge) {
        for (int i = 0; i != 3; i++) {
            for (auto & kv : edge[i]) {
                m_affinity += kv.second;
                m_area += 1;
            }
        }
    }
    Ta m_affinity;
    Ts m_area;
};

#endif
