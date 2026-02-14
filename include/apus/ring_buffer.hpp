#ifndef APUS_RING_BUFFER_HPP
#define APUS_RING_BUFFER_HPP

#include <cstddef>
#include <utility>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <type_traits>

namespace apus
{

    /**
     * @brief A circular buffer with a fixed capacity.
     *
     * ring_buffer provides a fixed-size FIFO queue with O(1) push and pop
     * at both ends. It uses a contiguous block of memory and wraps around.
     *
     * @tparam T The type of elements to store.
     */
    template <typename T>
    class ring_buffer
    {
    public:
        using value_type      = T;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = T&;
        using const_reference = const T&;
        using pointer         = T*;
        using const_pointer   = const T*;

        /**
         * @brief Custom iterator for ring_buffer to handle wrap-around.
         */
        template <bool IsConst>
        class basic_iterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = std::conditional_t<IsConst, const T*, T*>;
            using reference         = std::conditional_t<IsConst, const T&, T&>;

            basic_iterator() : rb_(nullptr), index_(0) {}
            basic_iterator(std::conditional_t<IsConst, const ring_buffer*, ring_buffer*> rb, size_type index)
                : rb_(rb), index_(index)
            {
            }

            // conversion from non-const to const
            template <bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
            basic_iterator(const basic_iterator<OtherConst>& other)
                : rb_(other.rb_), index_(other.index_)
            {
            }

            reference operator*() const { return (*rb_)[index_]; }
            pointer   operator->() const { return &((*rb_)[index_]); }

            basic_iterator& operator++()
            {
                ++index_;
                return *this;
            }
            basic_iterator operator++(int)
            {
                basic_iterator tmp = *this;
                ++(*this);
                return tmp;
            }
            basic_iterator& operator--()
            {
                --index_;
                return *this;
            }
            basic_iterator operator--(int)
            {
                basic_iterator tmp = *this;
                --(*this);
                return tmp;
            }

            basic_iterator& operator+=(difference_type n)
            {
                index_ += n;
                return *this;
            }
            basic_iterator& operator-=(difference_type n)
            {
                index_ -= n;
                return *this;
            }

            basic_iterator  operator+(difference_type n) const { return basic_iterator(rb_, index_ + n); }
            basic_iterator  operator-(difference_type n) const { return basic_iterator(rb_, index_ - n); }
            difference_type operator-(const basic_iterator& other) const { return index_ - other.index_; }

            reference operator[](difference_type n) const { return *(*this + n); }

            bool operator==(const basic_iterator& other) const { return index_ == other.index_; }
            bool operator!=(const basic_iterator& other) const { return index_ != other.index_; }
            bool operator<(const basic_iterator& other) const { return index_ < other.index_; }
            bool operator<=(const basic_iterator& other) const { return index_ <= other.index_; }
            bool operator>(const basic_iterator& other) const { return index_ > other.index_; }
            bool operator>=(const basic_iterator& other) const { return index_ >= other.index_; }

        private:
            friend class ring_buffer;
            using ring_buffer_t = std::conditional_t<IsConst, const ring_buffer*, ring_buffer*>;
            ring_buffer_t rb_;
            size_type     index_; // index relative to head
        };

        using iterator       = basic_iterator<false>;
        using const_iterator = basic_iterator<true>;

        /**
         * @brief Construct a new ring_buffer object with zero capacity.
         */
        ring_buffer() : ring_buffer(0) {}

        /**
         * @brief Construct a new ring_buffer with a given capacity.
         *
         * @param capacity The maximum number of elements the buffer can hold.
         */
        explicit ring_buffer(size_type capacity)
            : capacity_(capacity), size_(0), head_(0), tail_(0)
        {
            if (capacity_ > 0) {
                data_ = static_cast<T*>(std::malloc(capacity_ * sizeof(T)));
                if (!data_) throw std::bad_alloc();
            } else {
                data_ = nullptr;
            }
        }

        /**
         * @brief Copy constructor.
         */
        ring_buffer(const ring_buffer& other)
            : capacity_(other.capacity_), size_(0), head_(0), tail_(0)
        {
            if (capacity_ > 0) {
                data_ = static_cast<T*>(std::malloc(capacity_ * sizeof(T)));
                if (!data_) throw std::bad_alloc();
                for (size_type i = 0; i < other.size(); ++i) {
                    push_back(other[i]);
                }
            } else {
                data_ = nullptr;
            }
        }

        /**
         * @brief Move constructor.
         */
        ring_buffer(ring_buffer&& other) noexcept
            : data_(other.data_), capacity_(other.capacity_), size_(other.size_), head_(other.head_), tail_(other.tail_)
        {
            other.data_     = nullptr;
            other.capacity_ = 0;
            other.size_     = 0;
            other.head_     = 0;
            other.tail_     = 0;
        }

        /**
         * @brief Destructor.
         */
        ~ring_buffer()
        {
            clear();
            if (data_) {
                std::free(data_);
            }
        }

        /**
         * @brief Copy assignment.
         */
        ring_buffer& operator=(const ring_buffer& other)
        {
            if (this != &other) {
                clear();
                if (capacity_ != other.capacity_) {
                    if (data_) std::free(data_);
                    capacity_ = other.capacity_;
                    if (capacity_ > 0) {
                        data_ = static_cast<T*>(std::malloc(capacity_ * sizeof(T)));
                        if (!data_) throw std::bad_alloc();
                    } else {
                        data_ = nullptr;
                    }
                }
                if (capacity_ > 0) {
                    for (size_type i = 0; i < other.size(); ++i) {
                        push_back(other[i]);
                    }
                }
            }
            return *this;
        }

        /**
         * @brief Move assignment.
         */
        ring_buffer& operator=(ring_buffer&& other) noexcept
        {
            if (this != &other) {
                clear();
                if (data_) std::free(data_);
                data_           = other.data_;
                capacity_       = other.capacity_;
                size_           = other.size_;
                head_           = other.head_;
                tail_           = other.tail_;
                other.data_     = nullptr;
                other.capacity_ = 0;
                other.size_     = 0;
                other.head_     = 0;
                other.tail_     = 0;
            }
            return *this;
        }

        /**
         * @brief Adds an element to the back. If the buffer is full, it overwrites the front.
         *
         * @param value The value to add.
         */
        void push_back(const T& value)
        {
            if (capacity_ == 0) return;
            if (full()) {
                data_[tail_].~T();
                head_ = increment(head_);
            } else {
                ++size_;
            }
            new (data_ + tail_) T(value);
            tail_ = increment(tail_);
        }

        /**
         * @brief Adds an element to the back using move semantics.
         *
         * @param value The value to add.
         */
        void push_back(T&& value)
        {
            if (capacity_ == 0) return;
            if (full()) {
                data_[tail_].~T();
                head_ = increment(head_);
            } else {
                ++size_;
            }
            new (data_ + tail_) T(std::move(value));
            tail_ = increment(tail_);
        }

        /**
         * @brief Removes the front element.
         */
        void pop_front()
        {
            if (empty()) return;
            data_[head_].~T();
            head_ = increment(head_);
            --size_;
        }

        /**
         * @brief Changes the capacity of the ring buffer.
         *
         * If the new capacity is smaller than the current size, elements at the front
         * are removed to fit the new capacity.
         *
         * @param new_capacity The new capacity.
         */
        void set_capacity(size_type new_capacity)
        {
            if (new_capacity == capacity_) return;

            T* new_data = nullptr;
            if (new_capacity > 0) {
                new_data = static_cast<T*>(std::malloc(new_capacity * sizeof(T)));
                if (!new_data) throw std::bad_alloc();
            }

            size_type old_size         = size_;
            size_type elements_to_copy = std::min(old_size, new_capacity);

            // if shrinking below current size, we drop elements from the FRONT
            // to maintain the most recent elements (standard circular buffer behavior)
            size_type start_index = (old_size > new_capacity) ? (old_size - new_capacity) : 0;

            for (size_type i = 0; i < elements_to_copy; ++i) {
                new (new_data + i) T(std::move((*this)[start_index + i]));
            }

            // clean up old storage
            clear();
            if (data_) std::free(data_);

            data_     = new_data;
            capacity_ = new_capacity;
            size_     = elements_to_copy;
            head_     = 0;
            tail_     = (size_ == capacity_) ? 0 : size_;
        }

        /**
         * @brief Resizes the buffer to contain count elements.
         *
         * If count is greater than current size, additional elements are appended.
         * If count is greater than current capacity, the capacity is increased to match count.
         * If count is smaller than current size, elements are removed from the back.
         *
         * @param count The new number of elements.
         * @param value The value to initialize new elements with.
         */
        void resize(size_type count, const T& value = T())
        {
            if (count < size_) {
                while (size_ > count) {
                    size_type last = decrement(tail_);
                    data_[last].~T();
                    tail_ = last;
                    --size_;
                }
            } else if (count > size_) {
                if (count > capacity_) {
                    set_capacity(count);
                }
                while (size_ < count) {
                    push_back(value);
                }
            }
        }

        /**
         * @brief Accesses element at index (relative to head).
         */
        reference operator[](size_type index)
        {
            return data_[(head_ + index) % capacity_];
        }

        /**
         * @brief Accesses element at index (relative to head, const version).
         */
        const_reference operator[](size_type index) const
        {
            return data_[(head_ + index) % capacity_];
        }

        /**
         * @brief Accesses element at index with bounds checking.
         */
        reference at(size_type index)
        {
            if (index >= size_) throw std::out_of_range("ring_buffer::at: index out of range");
            return (*this)[index];
        }

        /**
         * @brief Accesses element at index with bounds checking (const version).
         */
        const_reference at(size_type index) const
        {
            if (index >= size_) throw std::out_of_range("ring_buffer::at: index out of range");
            return (*this)[index];
        }

        void clear() noexcept
        {
            while (!empty()) {
                pop_front();
            }
        }

        // clang-format off
        reference       front()          { return data_[head_];               }
        const_reference front()    const { return data_[head_];               }
        reference       back()           { return data_[decrement(tail_)]; }
        const_reference back()     const { return data_[decrement(tail_)]; }

        bool            full()     const noexcept { return size_ == capacity_; }
        bool            empty()    const noexcept { return size_ == 0;         }
        size_type       size()     const noexcept { return size_;              }
        size_type       capacity() const noexcept { return capacity_;          }

        iterator         begin()        noexcept { return iterator(this, 0);           }
        iterator         end()          noexcept { return iterator(this, size_);       }
        const_iterator   begin()   const noexcept { return const_iterator(this, 0);     }
        const_iterator   end()     const noexcept { return const_iterator(this, size_); }
        const_iterator   cbegin()  const noexcept { return const_iterator(this, 0);     }
        const_iterator   cend()    const noexcept { return const_iterator(this, size_); }
        // clang-format off

    private:
        size_type increment(size_type i) const noexcept
        {
            return (i + 1 == capacity_) ? 0 : i + 1;
        }

        size_type decrement(size_type i) const noexcept
        {
            return (i == 0) ? capacity_ - 1 : i - 1;
        }

        T*        data_;
        size_type capacity_;
        size_type size_;
        size_type head_;
        size_type tail_;
    };

} // namespace apus

#endif // APUS_RING_BUFFER_HPP
