#include <gtest/gtest.h>
#include <apus/small_vector.hpp>
#include <string>

namespace
{

    TEST(SmallVectorTest, DefaultConstructor)
    {
        apus::small_vector<int, 4> v;
        EXPECT_EQ(v.size(), 0);
        EXPECT_EQ(v.capacity(), 4);
        EXPECT_TRUE(v.empty());
    }

    TEST(SmallVectorTest, PushBackInline)
    {
        apus::small_vector<int, 4> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);

        EXPECT_EQ(v.size(), 4);
        EXPECT_EQ(v.capacity(), 4);
        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[3], 4);
    }

    TEST(SmallVectorTest, PushBackHeap)
    {
        apus::small_vector<int, 2> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3); // should trigger grow

        EXPECT_EQ(v.size(), 3);
        EXPECT_GT(v.capacity(), 2);
        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[1], 2);
        EXPECT_EQ(v[2], 3);
    }

    TEST(SmallVectorTest, EmplaceBack)
    {
        struct Foo
        {
            Foo(int a, std::string b)
                : a(a), b(b) {}
            int         a;
            std::string b;
        };

        apus::small_vector<Foo, 2> v;
        v.emplace_back(1, "hello");
        v.emplace_back(2, "world");

        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v[0].a, 1);
        EXPECT_EQ(v[0].b, "hello");
        EXPECT_EQ(v[1].a, 2);
        EXPECT_EQ(v[1].b, "world");
    }

    TEST(SmallVectorTest, PopBack)
    {
        apus::small_vector<int, 4> v;
        v.push_back(1);
        v.push_back(2);
        v.pop_back();

        EXPECT_EQ(v.size(), 1);
        EXPECT_EQ(v[0], 1);
    }

    TEST(SmallVectorTest, Clear)
    {
        apus::small_vector<int, 4> v;
        v.push_back(1);
        v.push_back(2);
        v.clear();

        EXPECT_EQ(v.size(), 0);
        EXPECT_TRUE(v.empty());
    }

    TEST(SmallVectorTest, Resize)
    {
        apus::small_vector<int, 4> v;
        v.resize(2, 10);
        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v[0], 10);
        EXPECT_EQ(v[1], 10);

        v.resize(5, 20);
        EXPECT_EQ(v.size(), 5);
        EXPECT_EQ(v[4], 20);
        EXPECT_GT(v.capacity(), 4);

        v.resize(1);
        EXPECT_EQ(v.size(), 1);
        EXPECT_EQ(v[0], 10);
    }

    TEST(SmallVectorTest, Iterators)
    {
        apus::small_vector<int, 4> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);

        int sum = 0;
        for (int x : v) {
            sum += x;
        }
        EXPECT_EQ(sum, 6);

        auto it = v.begin();
        EXPECT_EQ(*it, 1);
        ++it;
        EXPECT_EQ(*it, 2);
        it++;
        EXPECT_EQ(*it, 3);
        ++it;
        EXPECT_EQ(it, v.end());
    }

    TEST(SmallVectorTest, CopyConstructor)
    {
        apus::small_vector<int, 2> v1;
        v1.push_back(1);
        v1.push_back(2);
        v1.push_back(3); // heap

        apus::small_vector<int, 2> v2 = v1;
        EXPECT_EQ(v2.size(), 3);
        EXPECT_EQ(v2[0], 1);
        EXPECT_EQ(v2[2], 3);
    }

    TEST(SmallVectorTest, MoveConstructor)
    {
        apus::small_vector<int, 2> v1;
        v1.push_back(1);
        v1.push_back(2);
        v1.push_back(3); // heap

        apus::small_vector<int, 2> v2 = std::move(v1);
        EXPECT_EQ(v2.size(), 3);
        EXPECT_EQ(v2[0], 1);
        EXPECT_EQ(v1.size(), 0);
    }

    TEST(SmallVectorTest, InitializerList)
    {
        apus::small_vector<int, 4> v = {1, 2, 3};
        EXPECT_EQ(v.size(), 3);
        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[2], 3);
    }

    TEST(SmallVectorTest, Insert)
    {
        apus::small_vector<int, 4> v = {1, 3};
        auto                      it = v.insert(v.begin() + 1, 2);
        EXPECT_EQ(v.size(), 3);
        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[1], 2);
        EXPECT_EQ(v[2], 3);
        EXPECT_EQ(*it, 2);
    }

    TEST(SmallVectorTest, Erase)
    {
        apus::small_vector<int, 4> v = {1, 2, 3};
        auto                      it = v.erase(v.begin() + 1);
        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[1], 3);
        EXPECT_EQ(*it, 3);
    }

    TEST(SmallVectorTest, Find)
    {
        apus::small_vector<int, 4> v = {1, 2, 3};
        auto                      it = v.find(2);
        EXPECT_NE(it, v.end());
        EXPECT_EQ(*it, 2);

        auto it_not_found = v.find(4);
        EXPECT_EQ(it_not_found, v.end());

        const auto& cv  = v;
        auto        cit = cv.find(3);
        EXPECT_NE(cit, cv.end());
        EXPECT_EQ(*cit, 3);
    }

    TEST(SmallVectorTest, Contains)
    {
        apus::small_vector<int, 4> v = {1, 2, 3};
        EXPECT_TRUE(v.contains(1));
        EXPECT_TRUE(v.contains(2));
        EXPECT_TRUE(v.contains(3));
        EXPECT_FALSE(v.contains(4));
    }

    TEST(SmallVectorTest, Remove)
    {
        apus::small_vector<int, 4> v = {1, 2, 3, 2};

        // Remove existing item
        EXPECT_TRUE(v.remove(2));
        EXPECT_EQ(v.size(), 3);
        EXPECT_EQ(v[1], 3); // {1, 3, 2}

        // Remove another existing item
        EXPECT_TRUE(v.remove(2));
        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v[0], 1);
        EXPECT_EQ(v[1], 3);

        // Remove non-existing item
        EXPECT_FALSE(v.remove(4));
        EXPECT_EQ(v.size(), 2);
    }

} // namespace
