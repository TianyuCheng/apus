#include <gtest/gtest.h>
#include <apus/memory_arena.hpp>

TEST(MemoryArenaTest, Allocation) {
    constexpr std::size_t arena_size = 1024;
    apus::memory_arena<arena_size> arena;

    void* p1 = arena.allocate(100);
    EXPECT_NE(p1, nullptr);

    void* p2 = arena.allocate(200);
    EXPECT_NE(p2, nullptr);
    EXPECT_NE(p1, p2);
}

TEST(MemoryArenaTest, Reset) {
    constexpr std::size_t arena_size = 1024;
    apus::memory_arena<arena_size> arena;

    void* p1 = arena.allocate(100);
    arena.reset();
    void* p2 = arena.allocate(100);
    EXPECT_NE(p2, nullptr);
}

TEST(MemoryArenaTest, TypedAllocation) {
    constexpr std::size_t arena_size = 1024;
    apus::memory_arena<arena_size> arena;

    struct alignas(64) large_struct {
        int data[16];
    };

    int* p_int = arena.allocate<int>();
    EXPECT_NE(p_int, nullptr);
    *p_int = 42;
    EXPECT_EQ(*p_int, 42);
    arena.deallocate(p_int);

    double* p_doubles = arena.allocate<double>(10);
    EXPECT_NE(p_doubles, nullptr);
    for (int i = 0; i < 10; ++i) {
        p_doubles[i] = static_cast<double>(i);
    }
    EXPECT_DOUBLE_EQ(p_doubles[9], 9.0);
    arena.deallocate(p_doubles, 10);

    large_struct* p_large = arena.allocate<large_struct>();
    EXPECT_NE(p_large, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p_large) % 64, 0);
    arena.deallocate(p_large);
}
