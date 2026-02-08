#include <doctest/doctest.h>
#include <apus/paged_memory_arena.hpp>

TEST_SUITE("paged_memory_arena") {
    TEST_CASE("basic operations") {
        constexpr std::size_t page_size = 1024;
        apus::paged_memory_arena<page_size> arena;

        SUBCASE("simple allocation") {
            void* p = arena.allocate(100);
            CHECK(p != nullptr);
        }

        SUBCASE("paging behavior") {
            // allocate most of the first page
            void* p1 = arena.allocate(800);
            CHECK(p1 != nullptr);

            // this should trigger a new page allocation
            void* p2 = arena.allocate(400);
            CHECK(p2 != nullptr);
            
            // p1 and p2 should be far apart (different pages)
            CHECK(std::abs(reinterpret_cast<char*>(p2) - reinterpret_cast<char*>(p1)) >= static_cast<long>(page_size));
        }

        SUBCASE("oversized allocation") {
            void* p = arena.allocate(page_size + 1);
            CHECK(p == nullptr);
        }

        SUBCASE("typed allocation and reset") {
            int* p_int = arena.allocate<int>(100); // 400 bytes
            CHECK(p_int != nullptr);
            
            arena.reset();
            
            int* p_int2 = arena.allocate<int>(100);
            CHECK(p_int2 != nullptr);
        }
    }
}