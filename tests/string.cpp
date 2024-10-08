#include "common.h"

#include <immutable_string/string.hxx>

#include <algorithm>
#include <sstream>

using namespace ims;

static const char* const EMPTY_STRING = "";

static const char* const SHORT_STRING = "test_string_123"; // suitable for SSO
static const size_t SHORT_STRING_LEN = std::strlen(SHORT_STRING);

static const char* const SHORT_STRING_PART = "test_"; // suitable for SSO
static const size_t SHORT_STRING_PART_LEN = std::strlen(SHORT_STRING_PART);

static const char EMBEDDED_NULLS_STRING[] = { '0', '1', '2', '3', '4', '\0', '5', '6', '7', '8', '9', '\0', 'a', 'b', 'c', 'd', 'e', 'f' };
static const size_t EMBEDDED_NULLS_STRING_LEN = sizeof(EMBEDDED_NULLS_STRING);

static const char* const LONG_STRING = "Some very long string, can not fit into SSO buf";
static const size_t LONG_STRING_LEN = std::strlen(LONG_STRING);
static const char* const LONG_STRING_PART = "Some very long string, can not ";
static const size_t LONG_STRING_PART_LEN = std::strlen(LONG_STRING_PART);
static const char* const LONG_STRING_SHORT_PART = "Some ";
static const size_t LONG_STRING_SHORT_PART_LEN = std::strlen(LONG_STRING_SHORT_PART);


TEST(immutable_string, create)
{
    // default ctor
    {
        immutable_string src;
        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");
        EXPECT_FALSE(src._is_shared());
        EXPECT_FALSE(src._is_short());
        EXPECT_TRUE(src._has_null_terminator());
    }

    // from empty string literal
    {
        immutable_string src(EMPTY_STRING, immutable_string::FromStringLiteral);
        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        EXPECT_EQ(src.c_str(), EMPTY_STRING);
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_FALSE(src._is_shared());
        EXPECT_FALSE(src._is_short());
        EXPECT_TRUE(src._has_null_terminator());
    }

    // from string literal
    {
        immutable_string src(SHORT_STRING, immutable_string::FromStringLiteral);
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), SHORT_STRING_LEN);
        EXPECT_EQ(src.size(), SHORT_STRING_LEN);
        EXPECT_EQ(src.c_str(), SHORT_STRING);
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_FALSE(src._is_shared());
        EXPECT_FALSE(src._is_short());
        EXPECT_TRUE(src._has_null_terminator());
    }

    // from NULL
    {
        const char* d = nullptr;
        immutable_string src(d, 0);
        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");
        EXPECT_FALSE(src._is_shared());
        EXPECT_FALSE(src._is_short());
        EXPECT_TRUE(src._has_null_terminator());
    }

    // from empty C string
    {
        immutable_string src(EMPTY_STRING);
        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        EXPECT_NE(src.c_str(), EMPTY_STRING);
        EXPECT_STREQ(src.c_str(), EMPTY_STRING);
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_FALSE(src._is_shared());
        EXPECT_FALSE(src._is_short());
        EXPECT_TRUE(src._has_null_terminator());
    }

    // from short C string
    {
        immutable_string src(SHORT_STRING);
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), SHORT_STRING_LEN);
        EXPECT_EQ(src.size(), SHORT_STRING_LEN);
        EXPECT_NE(src.c_str(), SHORT_STRING);
        EXPECT_STREQ(src.c_str(), SHORT_STRING);
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_FALSE(src._is_shared());
        EXPECT_TRUE(src._is_short());
        EXPECT_TRUE(src._has_null_terminator());
    }

    // from C string with length
    {
        immutable_string src(LONG_STRING, LONG_STRING_LEN);
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), LONG_STRING_LEN);
        EXPECT_EQ(src.size(), LONG_STRING_LEN);
        EXPECT_NE(src.c_str(), LONG_STRING);
        EXPECT_STREQ(src.c_str(), LONG_STRING);
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_TRUE(src._is_shared());
        EXPECT_FALSE(src._is_short());
        EXPECT_TRUE(src._has_null_terminator());
    }

    // from C string (no terminator)
    {
        immutable_string src(LONG_STRING, LONG_STRING_PART_LEN);
        EXPECT_TRUE(src._has_null_terminator());
        EXPECT_FALSE(src._is_short());
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), LONG_STRING_PART_LEN);
        EXPECT_EQ(src.size(), src.length());
        
        EXPECT_NE(src.c_str(), LONG_STRING);
        EXPECT_STREQ(src.c_str(), LONG_STRING_PART);
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_TRUE(src._is_shared());
        EXPECT_FALSE(src._is_short());
    }

    // from C string with embedded nulls
    {
        immutable_string src(EMBEDDED_NULLS_STRING, EMBEDDED_NULLS_STRING_LEN);
        EXPECT_TRUE(src._has_null_terminator());
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), EMBEDDED_NULLS_STRING_LEN);
        EXPECT_EQ(src.size(), EMBEDDED_NULLS_STRING_LEN);
        EXPECT_NE(src.c_str(), EMBEDDED_NULLS_STRING);
        EXPECT_STREQ(src.c_str(), std::string(EMBEDDED_NULLS_STRING).c_str());
        EXPECT_EQ(0, std::memcmp(src.data(), EMBEDDED_NULLS_STRING, EMBEDDED_NULLS_STRING_LEN));
    }

    // from std::string_view
    {
        immutable_string src(std::string_view(LONG_STRING, LONG_STRING_PART_LEN));
        EXPECT_TRUE(src._has_null_terminator());
        EXPECT_FALSE(src._is_short());
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), LONG_STRING_PART_LEN);
        EXPECT_EQ(src.size(), src.length());

        EXPECT_NE(src.c_str(), LONG_STRING);
        EXPECT_STREQ(src.c_str(), LONG_STRING_PART);
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_TRUE(src._is_shared());
        EXPECT_FALSE(src._is_short());
    }

    // from std::string
    {
        immutable_string src(std::string(LONG_STRING, LONG_STRING_PART_LEN));
        EXPECT_TRUE(src._has_null_terminator());
        EXPECT_FALSE(src._is_short());
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), LONG_STRING_PART_LEN);
        EXPECT_EQ(src.size(), src.length());

        EXPECT_NE(src.c_str(), LONG_STRING);
        EXPECT_STREQ(src.c_str(), LONG_STRING_PART);
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_TRUE(src._is_shared());
        EXPECT_FALSE(src._is_short());
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

        dst = dst;
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");
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

        dst = dst;
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");
    }

    // copy-construct from string made from string literal
    {
        immutable_string src(SHORT_STRING, immutable_string::FromStringLiteral);
        immutable_string dst(src);

        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), SHORT_STRING_LEN);
        EXPECT_EQ(src.size(), SHORT_STRING_LEN);
        EXPECT_EQ(src.c_str(), SHORT_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_EQ(dst.data(), src.data());

        dst = dst;
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // copy-assign from string made from string literal
    {
        immutable_string src(SHORT_STRING, immutable_string::FromStringLiteral);
        immutable_string dst("not this");
        dst = src;

        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), SHORT_STRING_LEN);
        EXPECT_EQ(src.size(), SHORT_STRING_LEN);
        EXPECT_EQ(src.c_str(), SHORT_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_EQ(dst.data(), src.data());

        dst = dst;
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // copy-construct from short string
    {
        immutable_string src(SHORT_STRING);
        immutable_string dst(src);

        EXPECT_TRUE(src._is_short());
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), SHORT_STRING_LEN);
        EXPECT_EQ(src.size(), SHORT_STRING_LEN);
        EXPECT_NE(src.c_str(), SHORT_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_TRUE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_NE(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_NE(dst.data(), src.data());

        dst = dst;
        EXPECT_TRUE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_NE(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

    }

    // copy-assign from short string
    {
        immutable_string src(SHORT_STRING);
        immutable_string dst("not this");
        dst = src;

        EXPECT_TRUE(src._is_short());
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), SHORT_STRING_LEN);
        EXPECT_EQ(src.size(), SHORT_STRING_LEN);
        EXPECT_NE(src.c_str(), SHORT_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_TRUE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_NE(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_NE(dst.data(), src.data());

        dst = dst;
        EXPECT_TRUE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_NE(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // copy-construct from long string
    {
        immutable_string src(LONG_STRING);
        immutable_string dst(src);

        EXPECT_FALSE(src._is_short());
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), LONG_STRING_LEN);
        EXPECT_EQ(src.size(), LONG_STRING_LEN);
        EXPECT_NE(src.c_str(), LONG_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.size(), LONG_STRING_LEN);
        EXPECT_NE(dst.c_str(), LONG_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_EQ(dst.data(), src.data());

        dst = dst;
        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.size(), LONG_STRING_LEN);
        EXPECT_NE(dst.c_str(), LONG_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

    }

    // copy-assign from long string
    {
        immutable_string src(LONG_STRING);
        immutable_string dst("not this");
        dst = src;

        EXPECT_FALSE(src._is_short());
        EXPECT_FALSE(src.empty());
        EXPECT_EQ(src.length(), LONG_STRING_LEN);
        EXPECT_EQ(src.size(), LONG_STRING_LEN);
        EXPECT_NE(src.c_str(), LONG_STRING);
        EXPECT_EQ(src.data(), src.c_str());

        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.size(), LONG_STRING_LEN);
        EXPECT_NE(dst.c_str(), LONG_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        EXPECT_EQ(dst.data(), src.data());

        dst = dst;
        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.size(), LONG_STRING_LEN);
        EXPECT_NE(dst.c_str(), LONG_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
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

        EXPECT_FALSE(dst._is_short());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        EXPECT_EQ(dst.data(), src.data());

        dst = std::move(dst);
        EXPECT_FALSE(dst._is_short());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");
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

        EXPECT_FALSE(dst._is_short());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        EXPECT_EQ(dst.data(), src.data());

        dst = std::move(dst);
        EXPECT_FALSE(dst._is_short());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");
    }

    // move-construct from string made from string literal
    {
        immutable_string src(SHORT_STRING, immutable_string::FromStringLiteral);
        immutable_string dst(std::move(src));

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        dst = std::move(dst);
        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // move-assign from from string made from string literal
    {
        immutable_string src(SHORT_STRING, immutable_string::FromStringLiteral);
        immutable_string dst("not this");
        dst = std::move(src);

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        dst = std::move(dst);
        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // move-construct from short string
    {
        immutable_string src(SHORT_STRING);
        immutable_string dst(std::move(src));

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst.empty());
        EXPECT_TRUE(dst._is_short());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_NE(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        dst = std::move(dst);
        EXPECT_FALSE(dst.empty());
        EXPECT_TRUE(dst._is_short());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_NE(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // move-assign from short string
    {
        immutable_string src(SHORT_STRING);
        immutable_string dst("not this");
        dst = std::move(src);

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst.empty());
        EXPECT_TRUE(dst._is_short());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_NE(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        dst = std::move(dst);
        EXPECT_TRUE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.size(), SHORT_STRING_LEN);
        EXPECT_NE(dst.c_str(), SHORT_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // move-construct from long string
    {
        immutable_string src(LONG_STRING);
        immutable_string dst(std::move(src));

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst.empty());
        EXPECT_FALSE(dst._is_short());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.size(), LONG_STRING_LEN);
        EXPECT_NE(dst.c_str(), LONG_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        dst = std::move(dst);
        EXPECT_FALSE(dst.empty());
        EXPECT_FALSE(dst._is_short());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.size(), LONG_STRING_LEN);
        EXPECT_NE(dst.c_str(), LONG_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }

    // move-assign from long string
    {
        immutable_string src(LONG_STRING);
        immutable_string dst("not this");
        dst = std::move(src);

        EXPECT_TRUE(src.empty());
        EXPECT_EQ(src.length(), 0);
        EXPECT_EQ(src.size(), 0);
        ASSERT_TRUE(!!src.c_str());
        EXPECT_EQ(src.data(), src.c_str());
        EXPECT_STREQ(src.c_str(), "");

        EXPECT_FALSE(dst.empty());
        EXPECT_FALSE(dst._is_short());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.size(), LONG_STRING_LEN);
        EXPECT_NE(dst.c_str(), LONG_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());

        dst = std::move(dst);
        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.size(), LONG_STRING_LEN);
        EXPECT_NE(dst.c_str(), LONG_STRING);
        EXPECT_EQ(dst.data(), dst.c_str());
    }
}

TEST(immutable_string, substr)
{
    // from empty string
    {
        immutable_string src;
        
        auto dst = src.substr(0);
        EXPECT_FALSE(dst._is_short());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        // substr length exceeds present length
        dst = src.substr(0, 1);
        EXPECT_FALSE(dst._is_short());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        // substr of substr
        dst = dst.substr(0, 1);
        EXPECT_FALSE(dst._is_short());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), 0);
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_EQ(dst.data(), dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        // substr start exceeds present length
        EXPECT_THROW(dst = src.substr(1), std::out_of_range);
    }

    // from string made from literal
    {
        immutable_string src(LONG_STRING, immutable_string::FromStringLiteral);
        ASSERT_TRUE(src._has_null_terminator());
        
        // entire string
        auto dst = src.substr(0);
        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.length(), src.length());
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_STREQ(dst.c_str(), LONG_STRING);

        // part of string
        dst = src.substr(0, LONG_STRING_PART_LEN);
        EXPECT_FALSE(dst._is_short());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_PART_LEN);
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        ASSERT_TRUE(dst._has_null_terminator()); // should have added '\0' in c_str()
        EXPECT_STREQ(dst.c_str(), LONG_STRING_PART);

        // substr of substr
        dst = dst.substr(0, LONG_STRING_SHORT_PART_LEN);
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_SHORT_PART_LEN);
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_STREQ(dst.c_str(), LONG_STRING_SHORT_PART);

        // nothing left
        dst = src.substr(LONG_STRING_LEN);
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        // substr start exceeds present length
        EXPECT_THROW(dst = src.substr(LONG_STRING_LEN + 1), std::out_of_range);

    }

    // from short string 
    {
        immutable_string src(SHORT_STRING);

        // entire string
        auto dst = src.substr(0);
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_LEN);
        EXPECT_EQ(dst.length(), src.length());
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_STREQ(dst.c_str(), SHORT_STRING);

        // part of string
        dst = src.substr(0, SHORT_STRING_PART_LEN);
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), SHORT_STRING_PART_LEN);
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_STREQ(dst.c_str(), SHORT_STRING_PART);

        // substr start exceeds present length
        EXPECT_THROW(dst = src.substr(SHORT_STRING_LEN + 1), std::out_of_range);
    }

    // from long string 
    {
        immutable_string src(LONG_STRING);
        ASSERT_TRUE(src._has_null_terminator());

        // entire string
        auto dst = src.substr(0);
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_LEN);
        EXPECT_EQ(dst.length(), src.length());
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_STREQ(dst.c_str(), LONG_STRING);

        // part of string
        dst = src.substr(0, LONG_STRING_PART_LEN);
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_PART_LEN);
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        ASSERT_TRUE(dst._has_null_terminator()); // should have added '\0' in c_str()
        EXPECT_STREQ(dst.c_str(), LONG_STRING_PART);

        // substr of substr
        dst = dst.substr(0, LONG_STRING_SHORT_PART_LEN);
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(dst.length(), LONG_STRING_SHORT_PART_LEN);
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_STREQ(dst.c_str(), LONG_STRING_SHORT_PART);

        // nothing left
        dst = src.substr(LONG_STRING_LEN);
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.length(), 0);
        EXPECT_EQ(dst.size(), dst.length());
        ASSERT_TRUE(!!dst.c_str());
        EXPECT_STREQ(dst.c_str(), "");

        // substr start exceeds present length
        EXPECT_THROW(dst = src.substr(LONG_STRING_LEN + 1), std::out_of_range);
    }
}

TEST(immutable_string, iterators)
{
    auto collect_chars = [](const immutable_string& src)
    {
        std::ostringstream ss;
        for (auto it = src.begin(); it != src.end(); ++it)
        {
            ss << *it;
        }

        return ss.str();
    };
    

    // from string literal
    {
        immutable_string src(LONG_STRING, immutable_string::FromStringLiteral);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(LONG_STRING));
    }

    // short string (SSO)
    {
        immutable_string src(SHORT_STRING);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(SHORT_STRING));
    }

    // long string
    {
        immutable_string src(LONG_STRING);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(LONG_STRING));
    }
}

TEST(immutable_string, range_for)
{
    auto collect_chars = [](const immutable_string& src)
    {
        std::ostringstream ss;
        for (auto c : src)
        {
            ss << c;
        }

        return ss.str();
    };

    // empty string
    {
        immutable_string src;
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string());
    }

    // from string literal
    {
        immutable_string src(LONG_STRING, immutable_string::FromStringLiteral);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(LONG_STRING));
    }

    // short string (SSO)
    {
        immutable_string src(SHORT_STRING);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(SHORT_STRING));
    }

    // long string
    {
        immutable_string src(LONG_STRING);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(LONG_STRING));
    }
}

TEST(immutable_string, reverse_iterators)
{
    auto collect_chars = [](const immutable_string& src)
    {
        std::ostringstream ss;
        for (auto it = src.rbegin(); it != src.rend(); ++it)
        {
            ss << *it;
        }

        auto result = ss.str();
        std::reverse(result.begin(), result.end());
        return result;
    };

    // empty string
    {
        immutable_string src;
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string());
    }

    // from string literal
    {
        immutable_string src(LONG_STRING, immutable_string::FromStringLiteral);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(LONG_STRING));
    }

    // short string (SSO)
    {
        immutable_string src(SHORT_STRING);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(SHORT_STRING));
    }

    // long string
    {
        immutable_string src(LONG_STRING);
        auto chrs = collect_chars(src);
        EXPECT_EQ(chrs, std::string(LONG_STRING));
    }
}

TEST(immutable_string, rfind)
{
    // in empty string
    {
        immutable_string str;
        EXPECT_EQ(str.rfind(""), 0);
        EXPECT_EQ(str.rfind("", 1), 0);
        EXPECT_EQ(str.rfind("wow"), immutable_string::npos);
        EXPECT_EQ(str.rfind('w'), immutable_string::npos);
    }

    // in short string
    {
        immutable_string str("shosho");
        ASSERT_TRUE(str._is_short());
        EXPECT_EQ(str.rfind(""), 6);
        EXPECT_EQ(str.rfind("sho"), 3);
        EXPECT_EQ(str.rfind("sho", 2), 0);
        EXPECT_EQ(str.rfind("long"), immutable_string::npos);
        EXPECT_EQ(str.rfind('w'), immutable_string::npos);
        EXPECT_EQ(str.rfind('s'), 3);
        EXPECT_EQ(str.rfind('s', 2), 0);
    }

    // in long string
    {
        immutable_string str("Someone asked me yesterday Someone asked me yesterday");
        ASSERT_FALSE(str._is_short());
        EXPECT_EQ(str.rfind("Som"), 27);
        EXPECT_EQ(str.rfind("Som", 26), 0);
        EXPECT_EQ(str.rfind("som"), immutable_string::npos);
        EXPECT_EQ(str.rfind('w'), immutable_string::npos);
        EXPECT_EQ(str.rfind('S'), 27);
        EXPECT_EQ(str.rfind('S', 26), 0);
    }
}

TEST(immutable_string, find)
{
    // in empty string
    {
        immutable_string str;
        EXPECT_EQ(str.find(""), 0);
        EXPECT_EQ(str.find("", 1), immutable_string::npos);
        EXPECT_EQ(str.find("wow"), immutable_string::npos);
        EXPECT_EQ(str.find("wow", 1), immutable_string::npos);
        EXPECT_EQ(str.find('w'), immutable_string::npos);
        EXPECT_EQ(str.find('w', 1), immutable_string::npos);
    }

    // in short string
    {
        immutable_string str("short");
        ASSERT_TRUE(str._is_short());
        EXPECT_EQ(str.find(""), 0);
        EXPECT_EQ(str.find("", 1), 1);
        EXPECT_EQ(str.find("", str.length() + 1), immutable_string::npos);
        EXPECT_EQ(str.find("sho"), 0);
        EXPECT_EQ(str.find("sho", 1), immutable_string::npos);
        EXPECT_EQ(str.find("rt"), 3);
        EXPECT_EQ(str.find("rt", 4), immutable_string::npos);
        EXPECT_EQ(str.find("short"), 0);
        EXPECT_EQ(str.find("long"), immutable_string::npos);
        EXPECT_EQ(str.find('w'), immutable_string::npos);
        EXPECT_EQ(str.find('s'), 0);
        EXPECT_EQ(str.find('t'), 4);
        EXPECT_EQ(str.find('t', 5), immutable_string::npos);
    }

    // in long string
    {
        immutable_string str("Someone asked me yesterday: \"have you got a pet?\" I sadly had to answer them: \"no I haven\'t yet\"");
        ASSERT_FALSE(str._is_short());
        EXPECT_EQ(str.find(""), 0);
        EXPECT_EQ(str.find("", 1), 1);
        EXPECT_EQ(str.find("", str.length() + 1), immutable_string::npos);
        EXPECT_EQ(str.find("Som"), 0);
        EXPECT_EQ(str.find("Som", 1), immutable_string::npos);
        EXPECT_EQ(str.find("eone"), 3);
        EXPECT_EQ(str.find("eone", 4), immutable_string::npos);
        EXPECT_EQ(str.find("long"), immutable_string::npos);
        EXPECT_EQ(str.find('!'), immutable_string::npos);
        EXPECT_EQ(str.find('S'), 0);
        EXPECT_EQ(str.find('o'), 1);
    }
}

TEST(immutable_string, builder)
{
    {
        immutable_string::builder b;

        b.append(immutable_string());
        b.append(immutable_string("", immutable_string::FromStringLiteral));
        b.append(immutable_string("This", immutable_string::FromStringLiteral));
        b.append(immutable_string(" is a")); // SSO
        b.append(immutable_string(" long test string that does not fit into SSO but still valuable nevertheless"));
        immutable_string str(" and more");
        b.append(str); // l-value ref
        b.append(str); // l-value ref

        auto result = b.str();
        ASSERT_FALSE(result.empty());
        ASSERT_TRUE(result._has_null_terminator());
        EXPECT_STREQ(result.data(), "This is a long test string that does not fit into SSO but still valuable nevertheless and more and more");
    }

    {
        immutable_string a("This is a ");
        immutable_string result = a + "long" + immutable_string(" test string that does not fit into SSO but still valuable nevertheless and more and more");

        ASSERT_FALSE(result.empty());
        ASSERT_TRUE(result._has_null_terminator());
        EXPECT_STREQ(result.data(), "This is a long test string that does not fit into SSO but still valuable nevertheless and more and more");
    }
}
