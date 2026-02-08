#include <doctest/doctest.h>
#include <vector>
#include <apus/slot_map.hpp>

struct TestObject
{
    int  value;
    bool constructed = false;
    bool destructed  = false;

    TestObject(int val = 0) : value(val), constructed(true), destructed(false) {}
    TestObject(const TestObject& other) : value(other.value), constructed(true), destructed(false) {}
    TestObject(TestObject&& other) noexcept : value(other.value), constructed(true), destructed(false)
    {
    }

    ~TestObject()
    {
        destructed = true;
    }
};

struct TestDeleter
{
    void operator()(TestObject* obj) const
    {
        // Explicitly call destructor for objects managed by placement new
        obj->~TestObject();
    }
};

TEST_SUITE("slot_map")
{
    TEST_CASE("add and basic access")
    {
        apus::slot_map<TestObject> sm;

        auto h1 = sm.add(TestObject(10));
        auto h2 = sm.add(TestObject(20));
        auto h3 = sm.add(TestObject(30));

        CHECK(sm.at(h1).value == 10);
        CHECK(sm[h2].value == 20);
        CHECK(sm.find(h3)->value == 30);

        CHECK(h1.index == 0);
        CHECK(h1.version == 1);
        CHECK(h2.index == 1);
        CHECK(h2.version == 1);
        CHECK(h3.index == 2);
        CHECK(h3.version == 1);
        CHECK(sm.size() == 3);

        CHECK(sm.find({100, 1}) == nullptr); // Invalid handle (index out of bounds)
        CHECK_THROWS_AS(sm.at({100, 1}), std::out_of_range);
    }

    TEST_CASE("remove and reuse")
    {
        apus::slot_map<TestObject, apus::DEFAULT_SLOT_MAP_PAGE_SIZE, TestDeleter> sm;

        auto h1 = sm.add(TestObject(10)); // index 0, version 1
        auto h2 = sm.add(TestObject(20)); // index 1, version 1
        auto h3 = sm.add(TestObject(30)); // index 2, version 1

        CHECK(sm.size() == 3);
        CHECK(sm.at(h1).value == 10);
        CHECK(sm.at(h2).value == 20);
        CHECK(sm.at(h3).value == 30);

        // Remove h2
        TestObject& obj2_ref = sm.at(h2); // Get reference before removing
        CHECK(obj2_ref.destructed == false);
        sm.remove(h2);                      // h2.index = 1
        CHECK(obj2_ref.destructed == true); // Deleter should have been called
        CHECK(sm.size() == 2);

        // h2 is now invalid
        CHECK(sm.find(h2) == nullptr);
        CHECK_THROWS_AS(sm.at(h2), std::out_of_range);

        // Add a new object, should reuse index 1
        auto h4 = sm.add(TestObject(40)); // index 1, version 2
        CHECK(h4.index == 1);
        CHECK(h4.version == 2); // Version should increment
        CHECK(sm.at(h4).value == 40);
        CHECK(sm.size() == 3);

        // h2 (original handle) should still be invalid due to version mismatch
        CHECK(sm.find(h2) == nullptr);
        CHECK_THROWS_AS(sm.at(h2), std::out_of_range);

        // h1 and h3 should still be valid
        CHECK(sm.find(h1)->value == 10);
        CHECK(sm.find(h3)->value == 30);
    }

    TEST_CASE("iteration over active elements")
    {
        using sm_type = apus::slot_map<TestObject, apus::DEFAULT_SLOT_MAP_PAGE_SIZE, TestDeleter>;
        sm_type sm;

        std::vector<sm_type::handle> handles;
        for (int i = 0; i < 5; ++i) {
            handles.push_back(sm.add(TestObject(i + 1)));
        } // elements: 1, 2, 3, 4, 5 at indices 0, 1, 2, 3, 4

        std::vector<int> expected_initial_values;
        for (const auto& obj : sm) {
            expected_initial_values.push_back(obj.value);
        }
        CHECK(expected_initial_values == std::vector<int>{1, 2, 3, 4, 5});
        CHECK(sm.size() == 5);

        // Remove h[1] (value 2, index 1) and h[3] (value 4, index 3)
        sm.remove(handles[1]);
        sm.remove(handles[3]);
        CHECK(sm.size() == 3); // Active elements are now 1, 3, 5

        // Iteration should skip removed elements
        std::vector<int> expected_after_remove;
        for (const auto& obj : sm) {
            expected_after_remove.push_back(obj.value);
        }
        CHECK(expected_after_remove == std::vector<int>{1, 3, 5});

        // Re-add some items
        auto h6 = sm.add(TestObject(6)); // Reuses index 3, version 2
        auto h7 = sm.add(TestObject(7)); // Reuses index 1, version 2
        CHECK(sm.size() == 5);

        std::vector<int> expected_after_readd;
        for (const auto& obj : sm) {
            expected_after_readd.push_back(obj.value);
        }
        CHECK(expected_after_readd == std::vector<int>{1, 7, 3, 6, 5}); // Order by index: 0, 1, 2, 3, 4

        // Check if handle matches object's value
        CHECK(sm.at(h6).value == 6);
        CHECK(sm.at(h7).value == 7);
    }

    TEST_CASE("handle invalidation")
    {
        apus::slot_map<TestObject> sm;
        auto                       h1 = sm.add(TestObject(10));
        auto                       h2 = sm.add(TestObject(20)); // index 1, version 1

        sm.remove(h1);                    // index 0, version becomes dead bit set
        auto h3 = sm.add(TestObject(30)); // Reuses index 0, version becomes 2

        CHECK(sm.find(h1) == nullptr); // h1 is invalid
        CHECK(sm.find(h2) != nullptr); // h2 is still valid
        CHECK(sm.find(h3) != nullptr); // h3 is valid
        CHECK(h3.index == h1.index);   // Should reuse the index
        CHECK(h3.version == 2);        // Version should be 2
    }

    TEST_CASE("empty and size methods")
    {
        apus::slot_map<TestObject> sm;
        CHECK(sm.empty() == true);
        CHECK(sm.size() == 0);

        auto h1 = sm.add(TestObject(1));
        CHECK(sm.empty() == false);
        CHECK(sm.size() == 1);

        sm.remove(h1);
        CHECK(sm.empty() == true);
        CHECK(sm.size() == 0);

        auto h2 = sm.add(TestObject(2));
        auto h3 = sm.add(TestObject(3));
        CHECK(sm.empty() == false);
        CHECK(sm.size() == 2);
    }
}
