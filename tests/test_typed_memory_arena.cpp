#include <gtest/gtest.h>
#include <apus/typed_memory_arena.hpp>

TEST(TypedMemoryArenaTest, BasicAllocationAndDeallocation)
{
    apus::typed_memory_arena<int, 2> arena; // 2 elements per page

    auto res1 = arena.allocate(); // index 0
    auto res2 = arena.allocate(); // index 1
    auto res3 = arena.allocate(); // index 2 (new page)

    *res1.ptr = 10;
    *res2.ptr = 20;
    *res3.ptr = 30;

    EXPECT_EQ(res1.index, 0);
    EXPECT_EQ(res2.index, 1);
    EXPECT_EQ(res3.index, 2);

    // Deallocate res2 (index 1)
    arena.deallocate(res2.index); // freelist: [1]

    // Allocate again, should reuse index 1 (LIFO)
    auto res4 = arena.allocate(); // index 1
    EXPECT_EQ(res4.index, 1);
    *res4.ptr = 40;

    // Allocate another, should be index 3 (new element, freelist empty)
    auto res5 = arena.allocate(); // index 3
    EXPECT_EQ(res5.index, 3);
    *res5.ptr = 50;

    // Deallocate res1 (index 0) and res3 (index 2)
    arena.deallocate(res1.index); // freelist: [1, 0]
    arena.deallocate(res3.index); // freelist: [1, 0, 2]

    // Allocate, should reuse from freelist LIFO
    auto res6 = arena.allocate(); // index 2 (from freelist)
    EXPECT_EQ(res6.index, 2);
    *res6.ptr = 60;

    auto res7 = arena.allocate(); // index 0 (from freelist)
    EXPECT_EQ(res7.index, 0);
    *res7.ptr = 70;

    // Test get_address with current values
    EXPECT_EQ(*arena.get_address(0), 70);
    EXPECT_EQ(*arena.get_address(1), 40);
    EXPECT_EQ(*arena.get_address(2), 60);
    EXPECT_EQ(*arena.get_address(3), 50);
}

TEST(TypedMemoryArenaTest, ObjectLifecycle)
{
    struct MyObject
    {
        int   id;
        bool& constructed;
        bool& destructed;

        // Constructor
        MyObject(int _id, bool& c, bool& d) : id(_id), constructed(c), destructed(d)
        {
            constructed = true;
            destructed  = false;
        }
        // Destructor
        ~MyObject()
        {
            destructed = true;
        }
    };

    bool constructed1 = false, destructed1 = false;
    bool constructed2 = false, destructed2 = false;

    {
        apus::typed_memory_arena<MyObject, 2> arena;

        auto res1 = arena.allocate();
        new (res1.ptr) MyObject(1, constructed1, destructed1);
        EXPECT_TRUE(constructed1);
        EXPECT_FALSE(destructed1);

        arena.deallocate(res1.index);
        // Explicitly call destructor for non-POD types when deallocating,
        // as typed_memory_arena only manages memory, not object lifecycle.
        res1.ptr->~MyObject();
        EXPECT_TRUE(destructed1); // Should be true now after explicit call

        // Allocate another object, potentially reusing the same memory
        auto res2 = arena.allocate();
        new (res2.ptr) MyObject(2, constructed2, destructed2);
        EXPECT_TRUE(constructed2);
        EXPECT_FALSE(destructed2);
    }
    // When arena goes out of scope, the memory for res2 is reclaimed,
    // but MyObject's destructor is not automatically called by memory_arena.
    EXPECT_FALSE(destructed2); // Still false as memory_arena does not call destructors
}
