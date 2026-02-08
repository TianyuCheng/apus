#ifndef APUS_MEMORY_ARENA_HPP
#define APUS_MEMORY_ARENA_HPP

#include <array>
#include <cstddef>
#include <memory_resource>

namespace apus
{

    /**
     * @brief A memory arena that uses std::pmr::monotonic_buffer_resource.
     *
     * This arena manages a fixed-size buffer and provides fast allocations
     * by incrementing a pointer. Deallocations are no-ops until the arena is reset.
     *
     * @tparam SizeInBytes The size of the internal buffer in bytes.
     */
    template <std::size_t SizeInBytes>
    class memory_arena
    {
    public:
        /**
         * @brief Construct a new memory arena.
         */
        memory_arena()
            : resource_(buffer_.data(), buffer_.size(), std::pmr::null_memory_resource()) {}

        // disable copying and moving
        memory_arena(const memory_arena&)            = delete;
        memory_arena& operator=(const memory_arena&) = delete;
        memory_arena(memory_arena&&)                 = delete;
        memory_arena& operator=(memory_arena&&)      = delete;

        /**
         * @brief Allocate raw memory from the arena.
         *
         * @param bytes Number of bytes to allocate.
         * @param alignment Alignment requirement.
         * @return void* Pointer to the allocated memory.
         */
        void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t))
        {
            return resource_.allocate(bytes, alignment);
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
         * @brief Deallocate raw memory from the arena.
         *
         * Note: For monotonic resources, this is a no-op until reset() is called.
         *
         * @param p Pointer to the memory to deallocate.
         * @param bytes Number of bytes that were allocated.
         * @param alignment Alignment that was used for allocation.
         */
        void deallocate(void* p, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t))
        {
            resource_.deallocate(p, bytes, alignment);
        }

        /**
         * @brief Deallocate memory for a specific type.
         *
         * Note: For monotonic resources, this is a no-op until reset() is called.
         *
         * @tparam T The type of object.
         * @param p Pointer to the memory.
         * @param count The number of objects.
         */
        template <typename T>
        void deallocate(T* p, std::size_t count = 1)
        {
            deallocate(p, sizeof(T) * count, alignof(T));
        }

        /**
         * @brief Reset the arena, invalidating all previously allocated pointers.
         *
         * This allows the internal buffer to be reused for new allocations.
         */
        void reset()
        {
            resource_.release();
        }

        /**
         * @brief Get the underlying PMR memory resource.
         *
         * @return std::pmr::memory_resource* Pointer to the resource.
         */
        std::pmr::memory_resource* resource()
        {
            return &resource_;
        }

        /**
         * @brief Get the base address of the internal buffer.
         *
         * @return void* The base address of the memory managed by the arena.
         */
        void* get_base_address() const
        {
            return const_cast<std::byte*>(buffer_.data());
        }

        /**
         * @brief Get the base address of the internal buffer.
         *
         * @return T* The base address of the memory managed by the arena in T*.
         */
        template <typename T>
        T* get_base_address() const
        {
            return reinterpret_cast<T*>(get_base_address());
        }

    private:
        std::array<std::byte, SizeInBytes>  buffer_;
        std::pmr::monotonic_buffer_resource resource_;
    };

} // namespace apus

#endif // APUS_MEMORY_ARENA_HPP
