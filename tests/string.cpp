#include "common.h"

#include <immutable_string/string.hxx>

static const char* const EMPTY_STRING = "";
static const char* const TEST_STRING = "test_string";
static const size_t TEST_STRING_LEN = std::strlen(TEST_STRING);
static const char EMBEDDED_NULLS_STRING[] = { '0', '1', '2', '3', '4', '\0', '5', '6', '7', '8', '9', '\0', 'a', 'b', 'c', 'd', 'e', 'f' };
static const size_t EMBEDDED_NULLS_STRING_LEN = sizeof(EMBEDDED_NULLS_STRING);

TEST(immutable_string, create)
{
    // default ctor
    {
        immutable_string s;
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.length(), 0);
        EXPECT_EQ(s.size(), 0);
        ASSERT_TRUE(!!s.c_str());
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_STREQ(s.c_str(), "");
        EXPECT_FALSE(s.is_shared());
    }

    // from empty string literal
    {
        auto s = immutable_string::from_string_literal(EMPTY_STRING);
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.length(), 0);
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.c_str(), EMPTY_STRING);
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_FALSE(s.is_shared());
    }

    // from string literal
    {
        auto s = immutable_string::from_string_literal(TEST_STRING);
        EXPECT_FALSE(s.empty());
        EXPECT_EQ(s.length(), TEST_STRING_LEN);
        EXPECT_EQ(s.size(), TEST_STRING_LEN);
        EXPECT_EQ(s.c_str(), TEST_STRING);
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_FALSE(s.is_shared());
    }

    // from NULL
    {
        immutable_string s(nullptr);
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.length(), 0);
        EXPECT_EQ(s.size(), 0);
        ASSERT_TRUE(!!s.c_str());
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_STREQ(s.c_str(), "");
        EXPECT_FALSE(s.is_shared());
    }

    // from empty C string
    {
        immutable_string s(EMPTY_STRING);
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.length(), 0);
        EXPECT_EQ(s.size(), 0);
        EXPECT_NE(s.c_str(), EMPTY_STRING);
        EXPECT_STREQ(s.c_str(), EMPTY_STRING);
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_FALSE(s.is_shared());
    }

    // from C string
    {
        immutable_string s(TEST_STRING);
        EXPECT_FALSE(s.empty());
        EXPECT_EQ(s.length(), TEST_STRING_LEN);
        EXPECT_EQ(s.size(), TEST_STRING_LEN);
        EXPECT_NE(s.c_str(), TEST_STRING);
        EXPECT_STREQ(s.c_str(), TEST_STRING);
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_TRUE(s.is_shared());
    }

    // from C string with length
    {
        immutable_string s(TEST_STRING, TEST_STRING_LEN);
        EXPECT_FALSE(s.empty());
        EXPECT_EQ(s.length(), TEST_STRING_LEN);
        EXPECT_EQ(s.size(), TEST_STRING_LEN);
        EXPECT_NE(s.c_str(), TEST_STRING);
        EXPECT_STREQ(s.c_str(), TEST_STRING);
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_TRUE(s.is_shared());
    }

    // from C string with embedded nulls
    {
        immutable_string s(EMBEDDED_NULLS_STRING, EMBEDDED_NULLS_STRING_LEN);
        EXPECT_FALSE(s.empty());
        EXPECT_EQ(s.length(), EMBEDDED_NULLS_STRING_LEN);
        EXPECT_EQ(s.size(), EMBEDDED_NULLS_STRING_LEN);
        EXPECT_NE(s.c_str(), EMBEDDED_NULLS_STRING);
        EXPECT_STREQ(s.c_str(), std::string(EMBEDDED_NULLS_STRING).c_str());
        EXPECT_EQ(0, std::memcmp(s.data(), EMBEDDED_NULLS_STRING, EMBEDDED_NULLS_STRING_LEN));
        EXPECT_TRUE(s.is_shared());
    }

    // from string data
    {
        immutable_string src(TEST_STRING);
        auto dst = immutable_string::attach(src.detach());

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        EXPECT_TRUE(!!src.c_str());
        EXPECT_STREQ(src.c_str(), "");
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_TRUE(!src.is_shared());

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_NE(dst.c_str(), TEST_STRING);
        EXPECT_STREQ(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_TRUE(dst.is_shared());
    }
}

TEST(immutable_string, copy)
{

    // copy-construct from empty string
    {
        immutable_string src;
        immutable_string dst(src);
        
        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        EXPECT_EQ(dst.data(), src.data());
    }

    // copy-assign from empty string
    {
        immutable_string src;
        immutable_string dst("not this");
        dst = src;

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        EXPECT_EQ(dst.data(), src.data());
    }

    // copy-construct from string made from string literal
    {
        auto src = immutable_string::from_string_literal(TEST_STRING);
        immutable_string dst(src);

        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), TEST_STRING_LEN);
        EXPECT_EQ(src.size(), TEST_STRING_LEN);
        EXPECT_EQ(src.c_str(), TEST_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_EQ(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_EQ(dst.data(), src.data());
    }

    // copy-assign from string made from string literal
    {
        auto src = immutable_string::from_string_literal(TEST_STRING);
        immutable_string dst("not this");
        dst = src;

        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), TEST_STRING_LEN);
        EXPECT_EQ(src.size(), TEST_STRING_LEN);
        EXPECT_EQ(src.c_str(), TEST_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_EQ(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_EQ(dst.data(), src.data());
    }

    // copy-construct from non-empty string
    {
        immutable_string src(TEST_STRING);
        immutable_string dst(src);

        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), TEST_STRING_LEN);
        EXPECT_EQ(src.size(), TEST_STRING_LEN);
        EXPECT_NE(src.c_str(), TEST_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_NE(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_EQ(dst.data(), src.data());
    }

    // copy-assign from non-empty string
    {
        immutable_string src(TEST_STRING);
        immutable_string dst("not this");
        dst = src;

        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), TEST_STRING_LEN);
        EXPECT_EQ(src.size(), TEST_STRING_LEN);
        EXPECT_NE(src.c_str(), TEST_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_NE(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_EQ(dst.data(), src.data());
    }
}

TEST(immutable_string, move)
{
    // move-construct from empty string
    {
        immutable_string src;
        immutable_string dst(std::move(src));

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        EXPECT_EQ(dst.data(), src.data());
    }

    // move-assign from empty string
    {
        immutable_string src;
        immutable_string dst("not this");
        dst = std::move(src);

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        EXPECT_EQ(dst.data(), src.data());
    }

    // move-construct from string made from string literal
    {
        auto src = immutable_string::from_string_literal(TEST_STRING);
        immutable_string dst(std::move(src));

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_EQ(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // move-assign from from string made from string literal
    {
        auto src = immutable_string::from_string_literal(TEST_STRING);
        immutable_string dst("not this");
        dst = std::move(src);

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_EQ(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // move-construct from non-empty string
    {
        immutable_string src(TEST_STRING);
        immutable_string dst(std::move(src));

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_NE(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // move-assign from non-empty string
    {
        immutable_string src(TEST_STRING);
        immutable_string dst("not this");
        dst = std::move(src);

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), TEST_STRING_LEN);
        EXPECT_EQ(dst.size(), TEST_STRING_LEN);
        EXPECT_NE(dst.c_str(), TEST_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }
}

TEST(immutable_string, substr)
{
    // from empty string
    {
        immutable_string src;
        
        auto dst = src.substr(0);
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        // substr length exceeds present length
        dst = src.substr(0, 1);
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        // substr start exceeds present length
        EXPECT_THROW(dst = src.substr(1), std::out_of_range);
    }

    
}