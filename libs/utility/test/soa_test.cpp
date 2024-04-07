#include <gtest/gtest.h>
#include <utility/soa.h>

TEST(SoaTests, Basic)
{
    util::Soa<int, double, std::string> soa;
    soa.reserve(3);

    ASSERT_TRUE(soa.capacity() == 3);
    ASSERT_TRUE(soa.empty());

    soa.push_back(5, 2.0, "First");
    soa.push_back(10, 20.0, "Second");
    soa.push_back(15, 200.0, "Third");

    ASSERT_TRUE(soa.size() == 3);

    int* elementOne = soa.data<0>();
    double* elementTwo = soa.data<1>();
    std::string* elementThree = soa.data<2>();

    ASSERT_TRUE(elementOne[0] == 5);
    ASSERT_TRUE(elementOne[1] == 10);
    ASSERT_TRUE(elementOne[2] == 15);

    ASSERT_TRUE(elementTwo[0] == 2.0);
    ASSERT_TRUE(elementTwo[1] == 20.0);
    ASSERT_TRUE(elementTwo[2] == 200.0);

    ASSERT_STREQ(elementThree[0].c_str(), "First");
    ASSERT_STREQ(elementThree[1].c_str(), "Second");
    ASSERT_STREQ(elementThree[2].c_str(), "Third");

    // resizing should preserve the contents
    soa.reserve(10);

    ASSERT_TRUE(soa.capacity() == 10);
    ASSERT_TRUE(soa.size() == 3);

    elementOne = soa.data<0>();
    elementTwo = soa.data<1>();
    elementThree = soa.data<2>();

    ASSERT_TRUE(elementOne[0] == 5);
    ASSERT_TRUE(elementOne[1] == 10);
    ASSERT_TRUE(elementOne[2] == 15);

    ASSERT_TRUE(elementTwo[0] == 2.0);
    ASSERT_TRUE(elementTwo[1] == 20.0);
    ASSERT_TRUE(elementTwo[2] == 200.0);

    ASSERT_STREQ(elementThree[0].c_str(), "First");
    ASSERT_STREQ(elementThree[1].c_str(), "Second");
    ASSERT_STREQ(elementThree[2].c_str(), "Third");

    soa.clear();
    soa.resize(2);

    ASSERT_TRUE(soa.capacity() == 10);
    ASSERT_TRUE(soa.size() == 2);

    soa.at<0>(0) = 1;
    soa.at<1>(0) = 1.0;
    soa.at<2>(0) = "One";

    soa.at<0>(1) = 2;
    soa.at<1>(1) = 2.0;
    soa.at<2>(1) = "Two";

    elementOne = soa.data<0>();
    elementTwo = soa.data<1>();
    elementThree = soa.data<2>();

    ASSERT_TRUE(elementOne[0] == 1);
    ASSERT_TRUE(elementOne[1] == 2);

    ASSERT_TRUE(elementTwo[0] == 1.0);
    ASSERT_TRUE(elementTwo[1] == 2.0);

    ASSERT_STREQ(elementThree[0].c_str(), "One");
    ASSERT_STREQ(elementThree[1].c_str(), "Two");
}