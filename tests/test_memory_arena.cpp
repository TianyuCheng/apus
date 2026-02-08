#include <doctest/doctest.h>
#include <apus/memory_arena.hpp>

TEST_SUITE("memory_arena") {
    TEST_CASE("basic operations") {
        constexpr std::size_t arena_size = 1024;
        apus::memory_arena<arena_size> arena;

        SUBCASE("allocation") {
            void* p1 = arena.allocate(100);
            CHECK(p1 != nullptr);

            void* p2 = arena.allocate(200);
            CHECK(p2 != nullptr);
            CHECK(p1 != p2);
        }

        SUBCASE("reset") {
            void* p1 = arena.allocate(100);
            arena.reset();
            void* p2 = arena.allocate(100);
            CHECK(p2 != nullptr);
        }

        SUBCASE("typed allocation") {
            struct alignas(64) large_struct {
                int data[16];
            };

            int* p_int = arena.allocate<int>();
            CHECK(p_int != nullptr);
            *p_int = 42;
            CHECK(*p_int == 42);
            arena.deallocate(p_int);

            double* p_doubles = arena.allocate<double>(10);
            CHECK(p_doubles != nullptr);
            for (int i = 0; i < 10; ++i) {
                p_doubles[i] = static_cast<double>(i);
            }
            CHECK(p_doubles[9] == 9.0);
            arena.deallocate(p_doubles, 10);

            large_struct* p_large = arena.allocate<large_struct>();
            CHECK(p_large != nullptr);
            CHECK(reinterpret_cast<std::uintptr_t>(p_large) % 64 == 0);
            arena.deallocate(p_large);
        }
    }
}