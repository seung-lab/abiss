#include <numeric>
#include <boost/iostreams/device/mapped_file.hpp>

namespace bio = boost::iostreams;

template <typename T, int N>
class MMArray
{
public:
    MMArray(const std::string & fn, const std::array<size_t, N> & dims)
        :f()
    {
        size_t size = std::accumulate(std::begin(dims), std::end(dims), size_t(1), std::multiplies<size_t>());
        size_t bytes = sizeof(T)*size;
        f.open(fn, bio::mapped_file::mapmode::readwrite, bytes);
        data_ptr = new boost::multi_array_ref<T,N>(reinterpret_cast<T*>(f.data()), dims, boost::fortran_storage_order());
    }

    std::shared_ptr<boost::multi_array_ref <T, N> > data(){
        return std::shared_ptr<boost::multi_array_ref <T, N> >(data_ptr);
    }

    ~MMArray()
    {
        f.close();
    }

private:
    boost::multi_array_ref <T, N> * data_ptr;
    bio::mapped_file f;
};
