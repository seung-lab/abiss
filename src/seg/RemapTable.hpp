#ifndef REMAP_TABLE_H
#define REMAP_TABLE_H
#include <unordered_map>

template<typename T>
class RemapTable {
public:
    RemapTable()
        : m_globalMap() {}
    RemapTable(size_t data_size)
        : m_globalMap(data_size) {}
    ~RemapTable() {}

    T globalID(const T & id) const {
        if (m_globalMap.count(id) > 0) {
            return m_globalMap.at(id);
        } else {
            return id;
        }
    }

    void updateRemap(const T & s, const T & t) {
        m_globalMap[s] = globalID(t);
    }

    std::unordered_map<T, T> globalMap() const {
        return m_globalMap;
    }

private:
    std::unordered_map<T,T> m_globalMap;
};


template<typename T>
class ChunkRemap : public RemapTable<T> {
public:
    ChunkRemap(size_t data_size)
        : RemapTable<T>(data_size), m_chunkMap(data_size) {}
    ~ChunkRemap() {}

    T chunkID(const T & id) const {
        auto gid = this->globalID(id);
        if (m_chunkMap.count(gid) > 0) {
            return m_chunkMap.at(gid);
        } else {
            return gid;
        }
    }

    void generateChunkMap() {
        auto gmap = this->globalMap();
        for (auto &[k, v]: gmap) {
            if (v != 0 && gmap.count(v) == 0) {
                if (m_chunkMap.count(v) == 0) {
                    m_chunkMap[v] = k;
                    if (k == 0 || v == 0) {
                        std::cout << "!!!!!!!!!!!!!!!this should not happened: " << k << " " << v << std::endl;
                    }
                }
            }
        }
    }

    std::vector<std::pair<T, T> > reversedChunkMapVector() {
        std::vector<std::pair<T, T> > vect;
        for (auto [k,v]: m_chunkMap) {
            vect.push_back(std::make_pair(v,k));
        }
        return vect;
    }

private:
    std::unordered_map<T,T> m_chunkMap;
};


#endif
