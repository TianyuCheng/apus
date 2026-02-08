#ifndef APUS_PAGED_MEMORY_ARENA_HPP
#define APUS_PAGED_MEMORY_ARENA_HPP

#include <apus/memory_arena.hpp>
#include <vector>
#include <memory>
#include <cstddef>

namespace apus
{

    /**
     * @brief A memory arena that grows by allocating new pages of a fixed size.
     *
     * If an allocation request exceeds the remaining space in the current page,
     * a new page is allocated. If the request is larger than PageSizeInBytes, it returns nullptr.
     *
     * @tparam PageSizeInBytes The size of each memory page in bytes.
     */
    template <std::size_t PageSizeInBytes>
    class paged_memory_arena
    {
    public:
        /**
         * @brief Construct a new paged memory arena.
         */
        paged_memory_arena()
        {
            // start with one page
            pages_.emplace_back(std::make_unique<memory_arena<PageSizeInBytes>>());
        }

        // disable copying and moving
        paged_memory_arena(const paged_memory_arena&)            = delete;
        paged_memory_arena& operator=(const paged_memory_arena&) = delete;
        paged_memory_arena(paged_memory_arena&&)                 = delete;
        paged_memory_arena& operator=(paged_memory_arena&&)      = delete;

        /**
         * @brief Allocate raw memory from the arena.
         *
         * @param bytes Number of bytes to allocate.
         * @param alignment Alignment requirement.
         * @return void* Pointer to the allocated memory, or nullptr if bytes > PageSizeInBytes.
         */
        void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t))
        {
            if (bytes > PageSizeInBytes) {
                return nullptr;
            }

            try {
                return pages_.back()->allocate(bytes, alignment);
            } catch (const std::bad_alloc&) {
                // current page is full, add a new one
                pages_.emplace_back(std::make_unique<memory_arena<PageSizeInBytes>>());
                return pages_.back()->allocate(bytes, alignment);
            }
        }

        /**
         * @brief Allocate memory for a specific type.
         *
         * @tparam T The type of object to allocate for.
         * @param count The number of objects to allocate.
         * @return T* Pointer to the allocated memory.
         */
        template <typename T>
        T* allocate(std::size_t count = 1)
        {
            return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
        }

        /**
         * @brief Reset the arena, reclaiming all pages except the first one.
         */
        void reset()
        {
            if (pages_.size() > 1) {
                pages_.erase(pages_.begin() + 1, pages_.end());
            }
            pages_.back()->reset();
        }

    private:
        std::vector<std::unique_ptr<memory_arena<PageSizeInBytes>>> pages_;
    };

} // namespace apus

#endif // APUS_PAGED_MEMORY_ARENA_HPP
