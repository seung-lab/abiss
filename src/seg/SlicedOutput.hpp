#ifndef PACKED_OUTPUT_HPP
#define PACKED_OUTPUT_HPP

#include <boost/crc.hpp>
#include <fstream>
#include <iostream>
#include <vector>

template <typename TPayload, typename TIndex>
class SlicedOutput
{
public:
    SlicedOutput(std::string outputFilename)
        :m_offset(0), m_payload_buf(), m_index_buf(), m_output()
    {
        m_output.open(outputFilename);
        if (!m_output.is_open()) {
            std::cerr << "Failed to open output file for " << outputFilename << std::endl;
            std::abort();
        }
        size_t maxval = std::numeric_limits<std::size_t>::max();
        m_output.write(version, 4);
        m_output.write(reinterpret_cast<const char *>(&maxval), sizeof(maxval));
        m_output.write(reinterpret_cast<const char *>(&maxval), sizeof(maxval));
        if (m_output.bad()) {
            std::cerr << "Error occurred when writing sliced file" << std::endl;
            std::abort();
        }
        m_offset = m_output.tellp();
    }
    ~SlicedOutput()
    {
        m_output.close();
    }
    void addPayload(TPayload & payload)
    {
        m_payload_buf.push_back(payload);
    }
    void addPayload(TPayload && payload)
    {
        m_payload_buf.push_back(payload);
    }
    void flushChunk(TIndex chunkId){
        if (m_payload_buf.empty()) {
            std::cout << "Skip empty chunk" << std::endl;
        } else {
            size_t payload_size = m_payload_buf.size() * sizeof(TPayload);
            auto payload_crc32 = calc_checksum(m_payload_buf, payload_size);
            m_output.write(reinterpret_cast<const char *>(m_payload_buf.data()), payload_size);
            m_output.write(reinterpret_cast<const char *>(&payload_crc32), sizeof(payload_crc32));
            payload_size += sizeof(payload_crc32);
            index_t idx = {chunkId, m_offset, payload_size};
            m_index_buf.push_back(idx);
            std::cout << "Write chunk at:" << m_offset << ", " << payload_size << "," << payload_crc32 << std::endl;
            m_payload_buf.clear();
            m_offset += payload_size;
        }
        if (m_output.bad()) {
            std::cerr << "Error occurred when writing sliced file" << std::endl;
            std::abort();
        }
    }
    void flushIndex()
    {
        size_t index_size = m_index_buf.size() * sizeof(index_t);
        auto index_crc32 = calc_checksum(m_index_buf, index_size);
        m_output.write(reinterpret_cast<const char *>(m_index_buf.data()), index_size);
        m_output.write(reinterpret_cast<const char *>(&index_crc32), sizeof(index_crc32));
        index_size += sizeof(index_crc32);
        m_output.seekp(-m_offset-index_size+sizeof(version), std::ios_base::end);
        m_output.write(reinterpret_cast<const char *>(&m_offset), sizeof(m_offset));
        m_output.write(reinterpret_cast<const char *>(&index_size), sizeof(index_size));
        if (m_output.bad()) {
            std::cerr << "Error occurred when writing sliced file" << std::endl;
            std::abort();
        }
    }
private:
    struct index_t {
        TIndex chunkid;
        size_t offset;
        size_t length;
    };
    auto calc_checksum(auto & data, size_t length) {
        boost::crc_32_type result;
        result.process_bytes(data.data(), length);
        return result.checksum();
    }
    const char version[4] = {'S','O','0','1'};
    size_t m_offset;
    std::vector<TPayload> m_payload_buf;
    std::vector<index_t > m_index_buf;
    std::ofstream m_output;
};

#endif
