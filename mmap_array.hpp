#include <numeric>
#include <boost/iostreams/device/mapped_file.hpp>

namespace bio = boost::iostreams;

template <typename T, int N>
class MMArray
{
public:
    MMArray(const std::string & fn, const std::array<size_t, N> & dims)
        :m_mapped_file()
    {
        size_t size = std::accumulate(std::begin(dims), std::end(dims), size_t(1), std::multiplies<size_t>());
        size_t bytes = sizeof(T)*size;
        m_mapped_file.open(fn, bio::mapped_file::mapmode::readwrite, bytes);
        m_data_ptr = new boost::multi_array_ref<T,N>(reinterpret_cast<T*>(m_mapped_file.data()), dims, boost::fortran_storage_order());
    }

    std::shared_ptr<boost::multi_array_ref <T, N> > data_ptr()
    {
        return std::shared_ptr<boost::multi_array_ref <T, N> >(m_data_ptr);
    }

    boost::multi_array_ref<T, N> data()
    {
        return *m_data_ptr;
    }

    bool close()
    {
        if (m_mapped_file.is_open()) {
            m_mapped_file.close();
            if (m_mapped_file.is_open()) {
                return false;
            } else {
                return true;
            }
        }
        return false;
    }

    ~MMArray()
    {
        if (m_mapped_file.is_open()) {
            m_mapped_file.close();
        }
    }

private:
    boost::multi_array_ref <T, N> * m_data_ptr;
    bio::mapped_file m_mapped_file;
};
