#ifndef APUS_TYPED_MEMORY_ARENA_HPP
#define APUS_TYPED_MEMORY_ARENA_HPP

#include <vector>
#include <memory>
#include <cstddef>
#include <apus/free_list.hpp>
#include <apus/memory_arena.hpp>

namespace apus
{

    /**
     * @brief A paged memory arena specialized for a single type T, supporting
     * indexed allocation and deallocation using a freelist.
     *
     * This arena manages pages of fixed size. Each page is an apus::memory_arena
     * that stores elements of type T. A freelist is used to track and reuse
     * deallocated indices, providing O(1) allocation and deallocation in most cases.
     *
     * @tparam T The type of objects to store.
     * @tparam PageSizeInElems The number of elements per page.
     */
    template <typename T, std::size_t PageSizeInElems>
    class typed_memory_arena
    {
        static_assert(PageSizeInElems > 0, "PageSizeInElems must be greater than 0");

        // the size of the raw byte buffer for each memory_arena page
        static constexpr std::size_t PageSizeInBytes = PageSizeInElems * sizeof(T);

    public:
        /**
         * @brief Result of an allocation, containing the pointer and global index.
         */
        struct allocation_result
        {
            T*          ptr;
            std::size_t index;
        };

        /**
         * @brief Construct a new typed memory arena.
         */
        typed_memory_arena() = default;

        // disable copying
        typed_memory_arena(const typed_memory_arena&)            = delete;
        typed_memory_arena& operator=(const typed_memory_arena&) = delete;

        // enable moving
        typed_memory_arena(typed_memory_arena&&)                 = default;
        typed_memory_arena& operator=(typed_memory_arena&&)      = default;

        /**
         * @brief Allocate an element of type T.
         *
         * Prioritizes reusing a slot from the freelist. If the freelist is empty,
         * a new slot is allocated. A new page is created if necessary.
         *
         * @return allocation_result containing the pointer and the global index.
         */
        allocation_result allocate()
        {
            // try to reuse an index from the freelist
            if (!freelist_.empty()) {
                std::size_t index = freelist_.pop_back();

                std::size_t page_idx         = index / PageSizeInElems;
                std::size_t elem_idx_in_page = index % PageSizeInElems;

                // calculate the pointer to the reused slot within the page
                T* ptr = pages_[page_idx]->template get_base_address<T>() + elem_idx_in_page;
                return {ptr, index};
            }

            // no free slots in freelist, allocate a new one
            std::size_t allocated_index  = next_global_index_;
            std::size_t current_page_idx = allocated_index / PageSizeInElems;
            std::size_t current_elem_idx = allocated_index % PageSizeInElems;

            // check if we need a new page
            if (current_page_idx >= pages_.size()) {
                // When we create a new page, it should just be raw memory
                // We don't want memory_arena to allocate anything from it yet,
                // just provide the buffer.
                pages_.emplace_back(std::make_unique<memory_arena<PageSizeInBytes>>());
            }

            // Calculate the pointer directly to the indexed slot within the page
            T* ptr = pages_[current_page_idx]->template get_base_address<T>() + current_elem_idx;

            next_global_index_++;
            return {ptr, allocated_index};
        }

        /**
         * @brief Deallocate an element by its global index.
         *
         * Adds the index to the freelist for future reuse.
         *
         * @param index The global index returned at allocation.
         */
        void deallocate(std::size_t index)
        {
            // we only add to freelist, actual memory reclaim happens on page reset/destruction
            freelist_.push_back(index);

            // optionally, we could try to destruct the object if T is not a POD type.
            // for now, we assume simple types or managed destruction by user.
        }

        /**
         * @brief Accesses an element by its global index (no bounds checking).
         * @param index The global index.
         * @return T& Reference to the element.
         */
        T& operator[](std::size_t index) { return *get_address(index); }

        /**
         * @brief Accesses an element by its global index (no bounds checking, const version).
         * @param index The global index.
         * @return const T& Const reference to the element.
         */
        const T& operator[](std::size_t index) const { return *get_address(index); }

        /**
         * @brief Accesses an element by its global index with bounds checking.
         * @param index The global index.
         * @return T& Reference to the element.
         * @throws std::out_of_range If index is out of bounds.
         */
        T& at(std::size_t index)
        {
            if (index >= next_global_index_) throw std::out_of_range("typed_memory_arena::at: index out of bounds");
            return *get_address(index);
        }

        /**
         * @brief Accesses an element by its global index with bounds checking (const version).
         * @param index The global index.
         * @return const T& Const reference to the element.
         * @throws std::out_of_range If index is out of bounds.
         */
        const T& at(std::size_t index) const
        {
            if (index >= next_global_index_) throw std::out_of_range("typed_memory_arena::at: index out of bounds");
            return *get_address(index);
        }

        /**
         * @brief Retrieves the pointer to an element at a specific global index.
         *
         * @param index The global index of the element.
         * @return T* A pointer to the element. No bounds checking is performed.
         */
        T* get_address(std::size_t index) const
        {
            std::size_t page_idx         = index / PageSizeInElems;
            std::size_t elem_idx_in_page = index % PageSizeInElems;
            return pages_[page_idx]->template get_base_address<T>() + elem_idx_in_page;
        }

        /**
         * @brief Returns the total number of elements ever allocated (including deallocated ones).
         * This value is effectively the highest global index ever issued + 1.
         * @return std::size_t The next available global index.
         */
        std::size_t size() const
        {
            return next_global_index_;
        }

    private:
        std::vector<std::unique_ptr<memory_arena<PageSizeInBytes>>> pages_;

        // track the deallocated indices
        apus::free_list<std::size_t> freelist_;

        // tracks the next index for new allocations
        std::size_t next_global_index_ = 0;
    };

} // namespace apus

#endif // APUS_TYPED_MEMORY_ARENA_HPP
