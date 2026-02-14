#include <gtest/gtest.h>
#include <apus/paged_memory_arena.hpp>
#include <cmath>

TEST(PagedMemoryArenaTest, SimpleAllocation) {
    constexpr std::size_t page_size = 1024;
    apus::paged_memory_arena<page_size> arena;

    void* p = arena.allocate(100);
    EXPECT_NE(p, nullptr);
}

TEST(PagedMemoryArenaTest, PagingBehavior) {
    constexpr std::size_t page_size = 1024;
    apus::paged_memory_arena<page_size> arena;

    // allocate most of the first page
    void* p1 = arena.allocate(800);
    EXPECT_NE(p1, nullptr);

    // this should trigger a new page allocation
    void* p2 = arena.allocate(400);
    EXPECT_NE(p2, nullptr);
    
    // p1 and p2 should be far apart (different pages)
    EXPECT_GE(std::abs(reinterpret_cast<char*>(p2) - reinterpret_cast<char*>(p1)), static_cast<long>(page_size));
}

TEST(PagedMemoryArenaTest, OversizedAllocation) {
    constexpr std::size_t page_size = 1024;
    apus::paged_memory_arena<page_size> arena;

    void* p = arena.allocate(page_size + 1);
    EXPECT_EQ(p, nullptr);
}

TEST(PagedMemoryArenaTest, TypedAllocationAndReset) {
    constexpr std::size_t page_size = 1024;
    apus::paged_memory_arena<page_size> arena;

    int* p_int = arena.allocate<int>(100); // 400 bytes
    EXPECT_NE(p_int, nullptr);
    
    arena.reset();
    
    int* p_int2 = arena.allocate<int>(100);
    EXPECT_NE(p_int2, nullptr);
}
