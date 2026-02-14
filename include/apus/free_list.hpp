#ifndef APUS_FREE_LIST_HPP
#define APUS_FREE_LIST_HPP

#include <vector>
#include <utility>
#include <cstddef>
#include <stdexcept>

namespace apus
{

    /**
     * @brief Manages a collection of free objects of type T.
     *
     * This free_list uses a std::vector as its underlying storage, providing
     * LIFO (Last-In, First-Out) behavior for object reuse.
     *
     * @tparam T The type of objects to store in the free list.
     */
    template <typename T>
    class free_list
    {
    public:
        /**
         * @brief Construct a new free_list object.
         */
        free_list() = default;

        // disable copying
        free_list(const free_list&)            = delete;
        free_list& operator=(const free_list&) = delete;

        // enable moving
        free_list(free_list&&)                 = default;
        free_list& operator=(free_list&&)      = default;

        /**
         * @brief Adds an object to the free list.
         *
         * The object is added to the back of the internal vector.
         *
         * @param obj The object to add.
         */
        void push_back(T obj)
        {
            objects_.push_back(std::move(obj));
        }

        /**
         * @brief Retrieves and removes the last object from the free list.
         *
         * @return T The last object added to the list.
         * @throws std::out_of_range If the free list is empty.
         */
        T pop_back()
        {
            if (empty()) {
                throw std::out_of_range("pop_back called on empty free_list");
            }
            T obj = std::move(objects_.back());
            objects_.pop_back();
            return obj;
        }

        /**
         * @brief Checks if the free list is empty.
         *
         * @return true If the free list contains no objects.
         * @return false If the free list contains one or more objects.
         */
        bool empty() const
        {
            return objects_.empty();
        }

        /**
         * @brief Returns the number of objects in the free list.
         *
         * @return std::size_t The current number of free objects.
         */
        std::size_t size() const
        {
            return objects_.size();
        }

        /**
         * @brief Reserves capacity for at least count objects.
         *
         * This function pre-allocates memory for the internal storage to avoid
         * reallocations during subsequent push_back operations.
         *
         * @param count The number of objects for which to reserve space.
         */
        void reserve(std::size_t count)
        {
            objects_.reserve(count);
        }

    private:
        std::vector<T> objects_; // internal storage for free objects
    };

} // namespace apus

#endif // APUS_FREE_LIST_HPP
