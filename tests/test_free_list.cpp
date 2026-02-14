#include <gtest/gtest.h>
#include <apus/free_list.hpp>
#include <stdexcept>
#include <string>

TEST(FreeListTest, EmptyAndSizeWithInt)
{
    apus::free_list<int> fl;
    EXPECT_TRUE(fl.empty());
    EXPECT_EQ(fl.size(), 0);

    fl.push_back(10);
    EXPECT_FALSE(fl.empty());
    EXPECT_EQ(fl.size(), 1);

    fl.pop_back();
    EXPECT_TRUE(fl.empty());
    EXPECT_EQ(fl.size(), 0);
}

TEST(FreeListTest, PushBackAndPopBackLIFOWithInt)
{
    apus::free_list<int> fl;
    fl.push_back(1);
    fl.push_back(2);
    fl.push_back(3);

    EXPECT_EQ(fl.pop_back(), 3);
    EXPECT_EQ(fl.pop_back(), 2);
    EXPECT_EQ(fl.pop_back(), 1);
    EXPECT_TRUE(fl.empty());
}

TEST(FreeListTest, PopBackOnEmptyListThrowsWithInt)
{
    apus::free_list<int> fl;
    EXPECT_THROW(fl.pop_back(), std::out_of_range);
}

TEST(FreeListTest, EmptyAndSizeWithHighLevelType)
{
    apus::free_list<std::string> fl;
    EXPECT_TRUE(fl.empty());
    EXPECT_EQ(fl.size(), 0);

    fl.push_back("hello");
    EXPECT_FALSE(fl.empty());
    EXPECT_EQ(fl.size(), 1);

    fl.pop_back();
    EXPECT_TRUE(fl.empty());
    EXPECT_EQ(fl.size(), 0);
}

TEST(FreeListTest, PushBackAndPopBackLIFOWithHighLevelType)
{
    apus::free_list<std::string> fl;
    fl.push_back("first");
    fl.push_back("second");
    fl.push_back("third");

    EXPECT_EQ(fl.pop_back(), "third");
    EXPECT_EQ(fl.pop_back(), "second");
    EXPECT_EQ(fl.pop_back(), "first");
    EXPECT_TRUE(fl.empty());
}

TEST(FreeListTest, ReserveCapacity)
{
    apus::free_list<int> fl;
    std::size_t initial_capacity = 100;
    fl.reserve(initial_capacity);

    for (std::size_t i = 0; i < initial_capacity; ++i) {
        fl.push_back(static_cast<int>(i));
    }
    EXPECT_EQ(fl.size(), initial_capacity);

    fl.push_back(999);
    EXPECT_EQ(fl.size(), initial_capacity + 1);
    EXPECT_EQ(fl.pop_back(), 999);
}
