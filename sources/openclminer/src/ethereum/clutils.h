#pragma once

#include <vector>

namespace MinerGate {

#if !defined(__USE_DEV_VECTOR) && !defined(__NO_STD_VECTOR)
#include <vector>
#define VECTOR_CLASS std::vector
#elif !defined(__USE_DEV_VECTOR)
#define VECTOR_CLASS cl::vector
#endif

#if !defined(__MAX_DEFAULT_VECTOR_SIZE)
#define __MAX_DEFAULT_VECTOR_SIZE 10
#endif

/*! \class vector
 * \brief Fixed sized vector implementation that mirroring
 * std::vector functionality.
 */
template <typename T, unsigned int N = __MAX_DEFAULT_VECTOR_SIZE>
class vector
{
private:
    T data_[N];
    unsigned int size_;
    bool empty_;
public:
    vector() :
        size_(-1),
        empty_(true)
    {}

    ~vector() {}

    unsigned int size(void) const
    {
        return size_ + 1;
    }

    void clear()
    {
        size_ = -1;
        empty_ = true;
    }

    void push_back (const T& x)
    {
        if (size() < N) {
            size_++;
            data_[size_] = x;
            empty_ = false;
        }
    }

    void pop_back(void)
    {
        if (!empty_) {
            data_[size_].~T();
            size_--;
            if (size_ == -1) {
                empty_ = true;
            }
        }
    }

    vector(const vector<T, N>& vec) :
        size_(vec.size_),
        empty_(vec.empty_)
    {
        if (!empty_) {
            memcpy(&data_[0], &vec.data_[0], size() * sizeof(T));
        }
    }

    vector(unsigned int size, const T& val = T()) :
        size_(-1),
        empty_(true)
    {
        for (unsigned int i = 0; i < size; i++) {
            push_back(val);
        }
    }

    vector<T, N>& operator=(const vector<T, N>& rhs)
    {
        if (this == &rhs) {
            return *this;
        }

        size_  = rhs.size_;
        empty_ = rhs.empty_;

        if (!empty_) {
            memcpy(&data_[0], &rhs.data_[0], size() * sizeof(T));
        }

        return *this;
    }

    bool operator==(vector<T,N> &vec)
    {
        if (empty_ && vec.empty_) {
            return true;
        }

        if (size() != vec.size()) {
            return false;
        }

        return memcmp(&data_[0], &vec.data_[0], size() * sizeof(T)) == 0 ? true : false;
    }

    operator T* ()             { return data_; }
    operator const T* () const { return data_; }

    bool empty (void) const
    {
        return empty_;
    }

    unsigned int max_size (void) const
    {
        return N;
    }

    unsigned int capacity () const
    {
        return sizeof(T) * N;
    }

    T& operator[](int index)
    {
        return data_[index];
    }

    T operator[](int index) const
    {
        return data_[index];
    }

    template<class I>
    void assign(I start, I end)
    {
        clear();
        while(start < end) {
            push_back(*start);
            start++;
        }
    }

    /*! \class iterator
     * \brief Iterator class for vectors
     */
    class iterator
    {
    private:
        vector<T,N> vec_;
        int index_;
        bool initialized_;
    public:
        iterator(void) :
            index_(-1),
            initialized_(false)
        {
            index_ = -1;
            initialized_ = false;
        }

        ~iterator(void) {}

        static iterator begin(vector<T,N> &vec)
        {
            iterator i;

            if (!vec.empty()) {
                i.index_ = 0;
            }

            i.vec_ = vec;
            i.initialized_ = true;
            return i;
        }

        static iterator end(vector<T,N> &vec)
        {
            iterator i;

            if (!vec.empty()) {
                i.index_ = vec.size();
            }
            i.vec_ = vec;
            i.initialized_ = true;
            return i;
        }

        bool operator==(iterator i)
        {
            return ((vec_ == i.vec_) &&
                    (index_ == i.index_) &&
                    (initialized_ == i.initialized_));
        }

        bool operator!=(iterator i)
        {
            return (!(*this==i));
        }

        void operator++()
        {
            index_++;
        }

        void operator++(int x)
        {
            index_ += x;
        }

        void operator--()
        {
            index_--;
        }

        void operator--(int x)
        {
            index_ -= x;
        }

        T operator *()
        {
            return vec_[index_];
        }
    };

    iterator begin(void)
    {
        return iterator::begin(*this);
    }

    iterator end(void)
    {
        return iterator::end(*this);
    }

    T& front(void)
    {
        return data_[0];
    }

    T& back(void)
    {
        return data_[size_];
    }

    const T& front(void) const
    {
        return data_[0];
    }

    const T& back(void) const
    {
        return data_[size_];
    }
};

template <int N>
struct size_t : public MinerGate::vector< ::size_t, N> { };

class NDRange
{
private:
    size_t<3> sizes_;
    cl_uint dimensions_;

public:
    NDRange()
        : dimensions_(0)
    { }

    NDRange(::size_t size0)
        : dimensions_(1)
    {
        sizes_.push_back(size0);
    }

    NDRange(::size_t size0, ::size_t size1)
        : dimensions_(2)
    {
        sizes_.push_back(size0);
        sizes_.push_back(size1);
    }

    NDRange(::size_t size0, ::size_t size1, ::size_t size2)
        : dimensions_(3)
    {
        sizes_.push_back(size0);
        sizes_.push_back(size1);
        sizes_.push_back(size2);
    }

    operator const ::size_t*() const { return (const ::size_t*) sizes_; }
    ::size_t dimensions() const { return dimensions_; }
};

static const NDRange NullRange;

template <typename T>
struct KernelArgumentHandler
{
    static std::size_t size(const T&) { return sizeof(T); }
    static T* ptr(T& value) { return &value; }
};

}
