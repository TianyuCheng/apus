#include <doctest/doctest.h>

#include <apus/typed_memory_arena.hpp>

TEST_SUITE("typed_memory_arena")
{
    TEST_CASE("basic allocation and deallocation")
    {
        apus::typed_memory_arena<int, 2> arena; // 2 elements per page

        auto res1 = arena.allocate(); // index 0, value 10
        auto res2 = arena.allocate(); // index 1, value 20
        auto res3 = arena.allocate(); // index 2, value 30 (new page)

        *res1.ptr = 10;
        *res2.ptr = 20;
        *res3.ptr = 30;

        CHECK(res1.index == 0);
        CHECK(res2.index == 1);
        CHECK(res3.index == 2);

        // Deallocate res2 (index 1)
        arena.deallocate(res2.index); // freelist: [1]

        // Allocate again, should reuse index 1 (LIFO)
        auto res4 = arena.allocate(); // index 1, value 40
        CHECK(res4.index == 1);
        *res4.ptr = 40;

        // Allocate another, should be index 3 (new element, freelist empty)
        auto res5 = arena.allocate(); // index 3, value 50
        CHECK(res5.index == 3);
        *res5.ptr = 50;

        // Deallocate res1 (index 0) and res3 (index 2)
        arena.deallocate(res1.index); // freelist: [1, 0]
        arena.deallocate(res3.index); // freelist: [1, 0, 2]

        // Allocate, should reuse from freelist LIFO
        auto res6 = arena.allocate(); // index 2, value 60 (from freelist)
        CHECK(res6.index == 2);
        *res6.ptr = 60;

        auto res7 = arena.allocate(); // index 0, value 70 (from freelist)
        CHECK(res7.index == 0);
        *res7.ptr = 70;

        // At this point:
        // Index 0: contains 70 (from res7)
        // Index 1: contains 40 (from res4)
        // Index 2: contains 60 (from res6)
        // Index 3: contains 50 (from res5)

        // Test get_address with current values
        CHECK(*arena.get_address(0) == 70);
        CHECK(*arena.get_address(1) == 40);
        CHECK(*arena.get_address(2) == 60);
        CHECK(*arena.get_address(3) == 50);
    }

    TEST_CASE("object lifecycle")
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
            CHECK(constructed1 == true);
            CHECK(destructed1 == false);

            arena.deallocate(res1.index);
            // Explicitly call destructor for non-POD types when deallocating,
            // as typed_memory_arena only manages memory, not object lifecycle.
            res1.ptr->~MyObject();
            CHECK(destructed1 == true); // Should be true now after explicit call

            // Allocate another object, potentially reusing the same memory
            auto res2 = arena.allocate();
            new (res2.ptr) MyObject(2, constructed2, destructed2);
            CHECK(constructed2 == true);
            CHECK(destructed2 == false);
        }
        // When arena goes out of scope, the memory for res2 is reclaimed,
        // but MyObject's destructor is not automatically called by memory_arena.
        CHECK(destructed2 == false); // Still false as memory_arena does not call destructors
    }
}
