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
        size_t size = std::accumulate(std::begin(dims), std::end(dims), size_t(1), std::multiplies<T>());
        size_t bytes = sizeof(T)*size;
        f.open(fn, bytes);
        data_ptr = new boost::const_multi_array_ref<T,N, const T*>(reinterpret_cast<const T*>(f.data()), dims, boost::fortran_storage_order());
    }

    std::shared_ptr<boost::const_multi_array_ref <T, N, const T* > > data(){
        return std::shared_ptr<boost::const_multi_array_ref <T, N, const T* > >(data_ptr);
    }

    ~MMArray()
    {
        f.close();
    }

private:
    boost::const_multi_array_ref <T, N, const T* > * data_ptr;
    bio::mapped_file_source f;
};
