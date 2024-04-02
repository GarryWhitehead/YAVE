#include <gtest/gtest.h>
#include <utility/cstring.h>

TEST(CStringTests, Basic)
{
    util::CString cstr {"test_string"};
    ASSERT_STREQ(cstr.c_str(), "test_string");

    util::CString cstr1 {"test_string"};
    util::CString cstr2 {cstr1};
    ASSERT_STREQ(cstr2.c_str(), "test_string");

    util::CString alt_cstr {"other_string"};
    cstr = alt_cstr;
    ASSERT_STREQ(cstr.c_str(), "other_string");

    ASSERT_TRUE(cstr1 == cstr2);
    ASSERT_FALSE(cstr1 == alt_cstr);

    ASSERT_FALSE(cstr1.empty());
    ASSERT_EQ(cstr1.size(), 11);
}

TEST(CStringTests, SplitString)
{
    util::CString str {"My large test string"};
    auto split_vec = util::CString::split(str, ' ');
    ASSERT_EQ(split_vec.size(), 4);
    ASSERT_STREQ("My", split_vec[0].c_str());
    ASSERT_STREQ("large", split_vec[1].c_str());
    ASSERT_STREQ("test", split_vec[2].c_str());
    ASSERT_STREQ("string", split_vec[3].c_str());

    util::CString nosplit_str {"MyLargeTestString"};
    split_vec = util::CString::split(nosplit_str, ' ');
    ASSERT_EQ(split_vec.size(), 1);
}

TEST(CStringTests, AppendString)
{
    util::CString lhs_str {"First part/"};
    util::CString rhs_str {"Second part"};
    auto append_str = util::CString::append(lhs_str, rhs_str);
    ASSERT_STREQ(append_str.c_str(), "First part/Second part");

    util::CString empty_str;
    append_str = util::CString::append(lhs_str, empty_str);
    ASSERT_STREQ(append_str.c_str(), "First part/");
    append_str = util::CString::append(empty_str, rhs_str);
    ASSERT_STREQ(append_str.c_str(), "Second part");
}