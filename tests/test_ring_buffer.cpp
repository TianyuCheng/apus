#include <gtest/gtest.h>
#include <apus/ring_buffer.hpp>
#include <numeric>

namespace
{

    TEST(RingBufferTest, BasicOperations)
    {
        apus::ring_buffer<int> rb(3);
        EXPECT_TRUE(rb.empty());
        EXPECT_EQ(rb.size(), 0);
        EXPECT_EQ(rb.capacity(), 3);

        rb.push_back(1);
        rb.push_back(2);
        rb.push_back(3);
        EXPECT_TRUE(rb.full());
        EXPECT_EQ(rb.size(), 3);
        EXPECT_EQ(rb.front(), 1);
        EXPECT_EQ(rb.back(), 3);

        rb.push_back(4); // should overwrite 1
        EXPECT_EQ(rb.size(), 3);
        EXPECT_EQ(rb.front(), 2);
        EXPECT_EQ(rb.back(), 4);

        rb.pop_front();
        EXPECT_EQ(rb.size(), 2);
        EXPECT_EQ(rb.front(), 3);

        rb.clear();
        EXPECT_TRUE(rb.empty());
    }

    TEST(RingBufferTest, Iterators)
    {
        apus::ring_buffer<int> rb(5);
        for (int i = 1; i <= 5; ++i) rb.push_back(i);

        int expected = 1;
        for (int val : rb) {
            EXPECT_EQ(val, expected++);
        }

        rb.push_back(6); // [2, 3, 4, 5, 6]
        expected = 2;
        for (auto it = rb.begin(); it != rb.end(); ++it) {
            EXPECT_EQ(*it, expected++);
        }
    }

    TEST(RingBufferTest, RandomAccess)
    {
        apus::ring_buffer<int> rb(5);
        for (int i = 0; i < 5; ++i) rb.push_back(i);

        EXPECT_EQ(rb[0], 0);
        EXPECT_EQ(rb[4], 4);

        rb.push_back(5); // [1, 2, 3, 4, 5]
        EXPECT_EQ(rb[0], 1);
        EXPECT_EQ(rb[4], 5);

        auto it = rb.begin();
        EXPECT_EQ(it[2], 3);
        EXPECT_EQ(*(it + 3), 4);
    }

} // namespace
