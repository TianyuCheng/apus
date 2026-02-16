#ifndef APUS_SLOT_MAP_HPP
#define APUS_SLOT_MAP_HPP

#include <utility>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include <apus/typed_memory_arena.hpp>

namespace apus
{

    // default page size for slot_map's internal typed_memory_arena
    static constexpr std::size_t DEFAULT_SLOT_MAP_PAGE_SIZE = 1024;

    /**
     * @brief A default deleter for slot_map that only calls the destructor.
     *
     * Since slot_map uses an arena for storage, we must not use 'delete' which
     * would attempt to free the arena's memory. We only want to destruct the object.
     */
    template <typename T>
    struct slot_map_deleter
    {
        void operator()(T* ptr) const
        {
            if (ptr) {
                ptr->~T();
            }
        }
    };

    /**
     * @brief A handle for an element in the slot_map.
     *
     * Contains an index (into the underlying storage) and a version for validation.
     */
    template <typename T>
    struct slot_map_handle
    {
        std::uint32_t index;   // index into the typed_memory_arena storage
        std::uint32_t version; // version for validation

        bool operator==(const slot_map_handle& other) const
        {
            return index == other.index && version == other.version;
        }

        bool operator!=(const slot_map_handle& other) const
        {
            return !(*this == other);
        }
    };

    /**
     * @brief A basic non-compacting slot map data structure.
     *
     * Provides stable slot_map_handle<T>s to objects stored in a typed_memory_arena.
     * slot_map_handle<T>s directly index the underlying storage. Versioning with a "dead bit"
     * (the most significant bit) is used to invalidate slot_map_handle<T>s and track liveliness.
     *
     * @tparam T The type of objects to store.
     * @tparam PageSize The number of elements per page for underlying typed_memory_arena.
     * @tparam Deleter A functor to properly destruct objects when removed.
     */
    template <typename T, typename Deleter = slot_map_deleter<T>, std::size_t PageSize = DEFAULT_SLOT_MAP_PAGE_SIZE>
    class slot_map
    {
        static_assert(PageSize > 0, "PageSize must be greater than 0");

        // top-most bit is the dead bit
        static constexpr std::uint32_t DEAD_BIT     = 0x80000000;
        static constexpr std::uint32_t VERSION_MASK = 0x7FFFFFFF;

    public:
        // unified iterator template for both const and non-const iteration
        template <bool IsConst>
        class basic_iterator
        {
            friend class slot_map;
            using sm_ptr_type = std::conditional_t<IsConst, const slot_map*, slot_map*>;

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = T;
            using difference_type   = std::ptrdiff_t;
            using pointer           = std::conditional_t<IsConst, const T*, T*>;
            using reference         = std::conditional_t<IsConst, const T&, T&>;

            basic_iterator(sm_ptr_type sm_ptr, std::size_t index)
                : sm_ptr_(sm_ptr), current_index_(index) { advance_to_valid(); }

            // support conversion from non-const to const iterator
            template <bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
            basic_iterator(const basic_iterator<OtherConst>& other)
                : sm_ptr_(other.sm_ptr_), current_index_(other.current_index_) {}

            reference operator*() const { return *sm_ptr_->storage_arena_.get_address(current_index_); }
            pointer   operator->() const { return sm_ptr_->storage_arena_.get_address(current_index_); }

            basic_iterator& operator++()
            {
                current_index_++;
                advance_to_valid();
                return *this;
            }

            basic_iterator operator++(int)
            {
                basic_iterator temp = *this;
                ++(*this);
                return temp;
            }

            bool operator==(const basic_iterator& other) const { return current_index_ == other.current_index_; }
            bool operator!=(const basic_iterator& other) const { return current_index_ != other.current_index_; }

        private:
            void advance_to_valid()
            {
                while (current_index_ < sm_ptr_->storage_arena_.size() &&
                       (sm_ptr_->versions_arena_[current_index_] & DEAD_BIT)) {
                    current_index_++;
                }
            }
            sm_ptr_type sm_ptr_;
            std::size_t current_index_;
        };

        using handle         = slot_map_handle<T>;
        using iterator       = basic_iterator<false>;
        using const_iterator = basic_iterator<true>;

        /**
         * @brief Construct a new slot_map object.
         */
        slot_map() = default;

        /**
         * @brief Copy constructor.
         */
        slot_map(const slot_map& other)
            : current_size_(0)
        {
            for (auto it = other.begin(); it != other.end(); ++it) {
                add(*it);
            }
        }

        /**
         * @brief Move constructor.
         */
        slot_map(slot_map&& other) noexcept
            : storage_arena_(std::move(other.storage_arena_)),
              versions_arena_(std::move(other.versions_arena_)),
              current_size_(other.current_size_)
        {
            other.current_size_ = 0;
        }

        /**
         * @brief Copy assignment operator.
         */
        slot_map& operator=(const slot_map& other)
        {
            if (this != &other) {
                // simple but not highly efficient: clear and copy
                // ideally we would reuse already allocated pages
                slot_map temp(other);
                *this = std::move(temp);
            }
            return *this;
        }

        /**
         * @brief Move assignment operator.
         */
        slot_map& operator=(slot_map&& other) noexcept
        {
            if (this != &other) {
                storage_arena_      = std::move(other.storage_arena_);
                versions_arena_     = std::move(other.versions_arena_);
                current_size_       = other.current_size_;
                other.current_size_ = 0;
            }
            return *this;
        }

        /**
         * @brief Adds an object to the slot_map.
         *
         * @param obj The object to add.
         * @return handle A stable handle to the added object.
         */
        handle add(const T& obj)
        {
            return add_impl(obj);
        }

        /**
         * @brief Adds an object to the slot_map using move semantics.
         *
         * @param obj The object to add (rvalue reference).
         * @return handle A stable handle to the added object.
         */
        handle add(T&& obj)
        {
            return add_impl(std::move(obj));
        }

        /**
         * @brief Removes an object from the slot_map.
         *
         * @param h The handle of the object to remove.
         * @throws std::out_of_range If the handle is invalid or the object is already removed.
         */
        void remove(handle h)
        {
            if (h.index >= storage_arena_.size()) {
                throw std::out_of_range("slot_map::remove: handle index out of bounds");
            }

            std::uint32_t& version = versions_arena_[h.index];
            // if the version matches exactly, it must be live (because handles don't have dead_bit set)
            if (version != h.version) {
                throw std::out_of_range("slot_map::remove: handle version mismatch or object already removed");
            }

            // invoke deleter
            Deleter()(storage_arena_.get_address(h.index));

            // set dead bit to invalidate existing handles
            version |= DEAD_BIT;

            // deallocate from arena
            storage_arena_.deallocate(h.index);

            current_size_--;
        }

        /**
         * @brief Accesses an object by its handle with validation.
         *
         * @param h The handle of the object to access.
         * @return T& A reference to the object.
         * @throws std::out_of_range If the handle is invalid or the object is removed.
         */
        T& at(handle h)
        {
            if (h.index >= storage_arena_.size() || versions_arena_[h.index] != h.version) {
                throw std::out_of_range("slot_map::at: invalid handle or object removed");
            }
            return storage_arena_[h.index];
        }

        /**
         * @brief Accesses an object by its handle with validation (const version).
         *
         * @param h The handle of the object to access.
         * @return const T& A reference to the object.
         * @throws std::out_of_range If the handle is invalid or the object is removed.
         */
        const T& at(handle h) const
        {
            if (h.index >= storage_arena_.size() || versions_arena_[h.index] != h.version) {
                throw std::out_of_range("slot_map::at: invalid handle or object removed");
            }
            return storage_arena_[h.index];
        }

        /**
         * @brief Accesses an object by its handle (no validation).
         *
         * Use with caution. The handle must be valid and refer to a live object.
         *
         * @param h The handle of the object to access.
         * @return T& A reference to the object.
         */
        T& operator[](handle h)
        {
            return storage_arena_[h.index];
        }

        /**
         * @brief Accesses an object by its handle (no validation, const version).
         *
         * Use with caution. The handle must be valid and refer to a live object.
         *
         * @param h The handle of the object to access.
         * @return const T& A reference to the object.
         */
        const T& operator[](handle h) const
        {
            return storage_arena_[h.index];
        }

        /**
         * @brief Finds an object by its handle.
         *
         * @param h The handle of the object to find.
         * @return T* A pointer to the object if valid and live, nullptr otherwise.
         */
        T* find(handle h)
        {
            if (h.index >= storage_arena_.size() || versions_arena_[h.index] != h.version) {
                return nullptr;
            }
            return storage_arena_.get_address(h.index);
        }

        /**
         * @brief Finds an object by its handle (const version).
         *
         * @param h The handle of the object to find.
         * @return const T* A pointer to the object if valid and live, nullptr otherwise.
         */
        const T* find(handle h) const
        {
            if (h.index >= storage_arena_.size() || versions_arena_[h.index] != h.version) {
                return nullptr;
            }
            return storage_arena_.get_address(h.index);
        }

        /**
         * @brief Returns the total number of live (active) elements.
         * @return std::size_t The number of active elements.
         */
        std::size_t size() const
        {
            return current_size_;
        }

        /**
         * @brief Checks if the slot_map is empty.
         * @return true If the slot_map contains no active elements.
         */
        bool empty() const
        {
            return current_size_ == 0;
        }

        // clang-format off
        iterator       begin()        { return iterator(this, 0); }
        iterator       end()          { return iterator(this, storage_arena_.size()); }
        const_iterator begin()  const { return const_iterator(this, 0); }
        const_iterator end()    const { return const_iterator(this, storage_arena_.size()); }
        const_iterator cbegin() const { return const_iterator(this, 0); }
        const_iterator cend()   const { return const_iterator(this, storage_arena_.size()); }
        // clang-format on

    private:
        template <typename U>
        handle add_impl(U&& obj)
        {
            auto          alloc_res = storage_arena_.allocate();
            std::uint32_t index     = static_cast<std::uint32_t>(alloc_res.index);

            // sync versions_arena_ with storage_arena_
            while (versions_arena_.size() < storage_arena_.size()) {
                versions_arena_.allocate();
                versions_arena_[versions_arena_.size() - 1] = 0;
            }

            std::uint32_t& version = versions_arena_[index];
            // increment version and clear dead bit
            std::uint32_t next_version = (version & VERSION_MASK) + 1;
            version                    = next_version;

            current_size_++;
            new (alloc_res.ptr) T(std::forward<U>(obj));
            return {index, next_version};
        }

        apus::typed_memory_arena<T, PageSize>             storage_arena_;    // stores actual objects
        apus::typed_memory_arena<std::uint32_t, PageSize> versions_arena_;   // stores versions (metadata)
        std::size_t                                       current_size_ = 0; // tracks number of active elements
    };

} // namespace apus

#endif // APUS_SLOT_MAP_HPP
