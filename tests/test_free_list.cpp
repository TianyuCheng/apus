#include <doctest/doctest.h>
#include <apus/free_list.hpp>
#include <stdexcept>
#include <string>

TEST_SUITE("free_list")
{
    TEST_CASE("empty and size with int")
    {
        apus::free_list<int> fl;
        CHECK(fl.empty() == true);
        CHECK(fl.size() == 0);

        fl.push_back(10);
        CHECK(fl.empty() == false);
        CHECK(fl.size() == 1);

        fl.pop_back();
        CHECK(fl.empty() == true);
        CHECK(fl.size() == 0);
    }

    TEST_CASE("push_back and pop_back LIFO with int")
    {
        apus::free_list<int> fl;
        fl.push_back(1);
        fl.push_back(2);
        fl.push_back(3);

        CHECK(fl.pop_back() == 3);
        CHECK(fl.pop_back() == 2);
        CHECK(fl.pop_back() == 1);
        CHECK(fl.empty() == true);
    }

    TEST_CASE("pop_back on empty list throws with int")
    {
        apus::free_list<int> fl;
        CHECK_THROWS_AS(fl.pop_back(), std::out_of_range);
    }

    TEST_CASE("empty and size with string")
    {
        apus::free_list<std::string> fl;
        CHECK(fl.empty() == true);
        CHECK(fl.size() == 0);

        fl.push_back("hello");
        CHECK(fl.empty() == false);
        CHECK(fl.size() == 1);

        fl.pop_back();
        CHECK(fl.empty() == true);
        CHECK(fl.size() == 0);
    }

    TEST_CASE("push_back and pop_back LIFO with string")
    {
        apus::free_list<std::string> fl;
        fl.push_back("first");
        fl.push_back("second");
        fl.push_back("third");

        CHECK(fl.pop_back() == "third");
        CHECK(fl.pop_back() == "second");
        CHECK(fl.pop_back() == "first");
        CHECK(fl.empty() == true);
    }

    TEST_CASE("reserve capacity")
    {
        apus::free_list<int> fl;
        // Cannot directly check internal vector capacity from outside free_list,
        // but we can observe that push_back operations don't cause reallocations
        // by pushing more elements than initially reserved.
        // For testing purposes, we assume std::vector::reserve works as expected.
        std::size_t initial_capacity = 100;
        fl.reserve(initial_capacity);

        for (std::size_t i = 0; i < initial_capacity; ++i) {
            fl.push_back(static_cast<int>(i));
        }
        CHECK(fl.size() == initial_capacity);

        // Pushing one more should still work without immediate bad_alloc due to reserve
        fl.push_back(999);
        CHECK(fl.size() == initial_capacity + 1);
        CHECK(fl.pop_back() == 999);
    }
}
