#ifndef APUS_SMALL_VECTOR_HPP
#define APUS_SMALL_VECTOR_HPP

#include <cstddef>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>

namespace apus
{

    /**
     * @brief A vector-like container with a fixed-size inline buffer.
     *
     * small_vector allocates elements on the stack (inline) until its size exceeds N.
     * When the size exceeds N, it switches to heap allocation.
     *
     * @tparam T The type of elements to store.
     * @tparam N The number of elements to store inline.
     */
    template <typename T, std::size_t N>
    class small_vector
    {
    public:
        using value_type      = T;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = T&;
        using const_reference = const T&;
        using pointer         = T*;
        using const_pointer   = const T*;
        using iterator        = T*;
        using const_iterator  = const T*;

        /**
         * @brief Construct a new small_vector object.
         */
        small_vector() noexcept
            : size_(0), capacity_(N), data_(reinterpret_cast<T*>(inline_storage_))
        {
        }

        /**
         * @brief Construct a new small_vector object with a given size and default value.
         *
         * @param count The number of elements to construct.
         * @param value The value to initialize elements with.
         */
        explicit small_vector(size_type count, const T& value = T())
            : small_vector()
        {
            resize(count, value);
        }

        /**
         * @brief Construct a new small_vector object from an initializer list.
         *
         * @param init Initializer list of elements.
         */
        small_vector(std::initializer_list<T> init)
            : small_vector()
        {
            reserve(init.size());
            for (const auto& item : init) {
                push_back(item);
            }
        }

        /**
         * @brief Copy constructor.
         *
         * @param other The other small_vector to copy from.
         */
        small_vector(const small_vector& other)
            : small_vector()
        {
            reserve(other.size_);
            for (size_type i = 0; i < other.size_; ++i) {
                new (data_ + i) T(other.data_[i]);
            }
            size_ = other.size_;
        }

        /**
         * @brief Move constructor.
         *
         * @param other The other small_vector to move from.
         */
        small_vector(small_vector&& other) noexcept
            : size_(0), capacity_(N), data_(reinterpret_cast<T*>(inline_storage_))
        {
            if (other.is_inline()) {
                for (size_type i = 0; i < other.size_; ++i) {
                    new (data_ + i) T(std::move(other.data_[i]));
                    other.data_[i].~T();
                }
                size_       = other.size_;
                other.size_ = 0;
            } else {
                data_     = other.data_;
                size_     = other.size_;
                capacity_ = other.capacity_;

                // reset other to inline state
                other.data_     = reinterpret_cast<T*>(other.inline_storage_);
                other.size_     = 0;
                other.capacity_ = N;
            }
        }

        /**
         * @brief Destructor.
         */
        ~small_vector()
        {
            clear();
            if (!is_inline()) {
                std::free(data_);
            }
        }

        /**
         * @brief Copy assignment operator.
         *
         * @param other The other small_vector to copy from.
         * @return small_vector& Reference to this vector.
         */
        small_vector& operator=(const small_vector& other)
        {
            if (this != &other) {
                clear();
                reserve(other.size_);
                for (size_type i = 0; i < other.size_; ++i) {
                    new (data_ + i) T(other.data_[i]);
                }
                size_ = other.size_;
            }
            return *this;
        }

        /**
         * @brief Move assignment operator.
         *
         * @param other The other small_vector to move from.
         * @return small_vector& Reference to this vector.
         */
        small_vector& operator=(small_vector&& other) noexcept
        {
            if (this != &other) {
                clear();
                if (!is_inline()) {
                    std::free(data_);
                }

                if (other.is_inline()) {
                    data_     = reinterpret_cast<T*>(inline_storage_);
                    capacity_ = N;
                    for (size_type i = 0; i < other.size_; ++i) {
                        new (data_ + i) T(std::move(other.data_[i]));
                        other.data_[i].~T();
                    }
                    size_       = other.size_;
                    other.size_ = 0;
                } else {
                    data_     = other.data_;
                    size_     = other.size_;
                    capacity_ = other.capacity_;

                    other.data_     = reinterpret_cast<T*>(other.inline_storage_);
                    other.size_     = 0;
                    other.capacity_ = N;
                }
            }
            return *this;
        }

        /**
         * @brief Adds an element to the end of the vector.
         *
         * @param value The value to add.
         */
        void push_back(const T& value)
        {
            if (size_ >= capacity_) {
                grow();
            }
            new (data_ + size_) T(value);
            ++size_;
        }

        /**
         * @brief Adds an element to the end of the vector using move semantics.
         *
         * @param value The value to add (rvalue reference).
         */
        void push_back(T&& value)
        {
            if (size_ >= capacity_) {
                grow();
            }
            new (data_ + size_) T(std::move(value));
            ++size_;
        }

        /**
         * @brief Constructs an element in-place at the end of the vector.
         *
         * @tparam Args Types of arguments for construction.
         * @param args Arguments for construction.
         * @return reference Reference to the constructed element.
         */
        template <typename... Args>
        reference emplace_back(Args&&... args)
        {
            if (size_ >= capacity_) {
                grow();
            }
            new (data_ + size_) T(std::forward<Args>(args)...);
            return data_[size_++];
        }

        /**
         * @brief Removes the last element from the vector.
         */
        void pop_back()
        {
            if (size_ > 0) {
                --size_;
                data_[size_].~T();
            }
        }

        /**
         * @brief Resizes the vector to contain count elements.
         *
         * @param count New size of the vector.
         * @param value Value to initialize new elements with.
         */
        void resize(size_type count, const T& value = T())
        {
            if (count < size_) {
                while (size_ > count) {
                    pop_back();
                }
            } else if (count > size_) {
                reserve(count);
                while (size_ < count) {
                    push_back(value);
                }
            }
        }

        /**
         * @brief Reserves capacity for at least count elements.
         *
         * @param new_cap Minimum capacity for the vector.
         */
        void reserve(size_type new_cap)
        {
            if (new_cap <= capacity_) {
                return;
            }

            T* new_data = static_cast<T*>(std::malloc(new_cap * sizeof(T)));
            if (!new_data) {
                throw std::bad_alloc();
            }

            for (size_type i = 0; i < size_; ++i) {
                new (new_data + i) T(std::move(data_[i]));
                data_[i].~T();
            }

            if (!is_inline()) {
                std::free(data_);
            }

            data_     = new_data;
            capacity_ = new_cap;
        }

        /**
         * @brief Removes all elements from the vector.
         */
        void clear() noexcept
        {
            for (size_type i = 0; i < size_; ++i) {
                data_[i].~T();
            }
            size_ = 0;
        }

        /**
         * @brief Returns the number of elements in the vector.
         *
         * @return size_type The number of elements.
         */
        size_type size() const noexcept { return size_; }

        /**
         * @brief Returns the capacity of the vector.
         *
         * @return size_type The capacity.
         */
        size_type capacity() const noexcept { return capacity_; }

        /**
         * @brief Checks if the vector is empty.
         *
         * @return true If the vector contains no elements.
         */
        bool empty() const noexcept { return size_ == 0; }

        /**
         * @brief Accesses an element at a given index with bounds checking.
         *
         * @param index The index of the element.
         * @return reference Reference to the element.
         * @throws std::out_of_range If index is out of bounds.
         */
        reference at(size_type index)
        {
            if (index >= size_) {
                throw std::out_of_range("small_vector::at: index out of range");
            }
            return data_[index];
        }

        /**
         * @brief Accesses an element at a given index with bounds checking (const version).
         *
         * @param index The index of the element.
         * @return const_reference Reference to the element.
         * @throws std::out_of_range If index is out of bounds.
         */
        const_reference at(size_type index) const
        {
            if (index >= size_) {
                throw std::out_of_range("small_vector::at: index out of range");
            }
            return data_[index];
        }

        /**
         * @brief Accesses an element at a given index.
         *
         * @param index The index of the element.
         * @return reference Reference to the element.
         */
        reference operator[](size_type index) noexcept { return data_[index]; }

        /**
         * @brief Accesses an element at a given index (const version).
         *
         * @param index The index of the element.
         * @return const_reference Reference to the element.
         */
        const_reference operator[](size_type index) const noexcept { return data_[index]; }

        /**
         * @brief Returns a reference to the first element.
         *
         * @return reference Reference to the first element.
         */
        reference front() { return data_[0]; }

        /**
         * @brief Returns a reference to the first element (const version).
         *
         * @return const_reference Reference to the first element.
         */
        const_reference front() const { return data_[0]; }

        /**
         * @brief Returns a reference to the last element.
         *
         * @return reference Reference to the last element.
         */
        reference back() { return data_[size_ - 1]; }

        /**
         * @brief Returns a reference to the last element (const version).
         *
         * @return const_reference Reference to the last element.
         */
        const_reference back() const { return data_[size_ - 1]; }

        /**
         * @brief Returns a pointer to the underlying data.
         *
         * @return pointer Pointer to the data.
         */
        pointer data() noexcept { return data_; }

        /**
         * @brief Returns a pointer to the underlying data (const version).
         *
         * @return const_pointer Pointer to the data.
         */
        const_pointer data() const noexcept { return data_; }

        /**
         * @brief Inserts an element at the specified position.
         *
         * @param pos Iterator to the position before which the content will be inserted.
         * @param value Element value to insert.
         * @return iterator Iterator pointing to the inserted element.
         */
        iterator insert(const_iterator pos, const T& value)
        {
            size_type index = static_cast<size_type>(pos - begin());
            if (size_ >= capacity_) {
                grow();
            }

            if (index < size_) {
                new (data_ + size_) T(std::move(data_[size_ - 1]));
                for (size_type i = size_ - 1; i > index; --i) {
                    data_[i] = std::move(data_[i - 1]);
                }
                data_[index] = value;
            } else {
                new (data_ + size_) T(value);
            }
            ++size_;
            return begin() + index;
        }

        /**
         * @brief Removes the element at the specified position.
         *
         * @param pos Iterator to the element to remove.
         * @return iterator Iterator following the last removed element.
         */
        iterator erase(const_iterator pos)
        {
            size_type index = static_cast<size_type>(pos - begin());
            if (index < size_) {
                for (size_type i = index; i < size_ - 1; ++i) {
                    data_[i] = std::move(data_[i + 1]);
                }
                data_[size_ - 1].~T();
                --size_;
            }
            return begin() + index;
        }

        // iterators
        // clang-format off
        iterator       begin()        noexcept { return data_;         }
        iterator       end()          noexcept { return data_ + size_; }
        const_iterator begin()  const noexcept { return data_;         }
        const_iterator end()    const noexcept { return data_ + size_; }
        const_iterator cbegin() const noexcept { return data_;         }
        const_iterator cend()   const noexcept { return data_ + size_; }
        // clang-format on

    private:
        /**
         * @brief Checks if the data is stored in the inline buffer.
         *
         * @return true If data is stored inline.
         */
        bool is_inline() const noexcept
        {
            return data_ == reinterpret_cast<const T*>(inline_storage_);
        }

        /**
         * @brief Grows the vector's capacity.
         */
        void grow()
        {
            size_type new_cap = (capacity_ == 0) ? 1 : capacity_ * 2;
            reserve(new_cap);
        }

        size_type size_;
        size_type capacity_;
        T*        data_;

        // inline storage for elements
        // avoid using std::array<T, N> for unexpected constructors/destructors
        alignas(T) char inline_storage_[N * sizeof(T)];
    };

} // namespace apus

#endif // APUS_SMALL_VECTOR_HPP
