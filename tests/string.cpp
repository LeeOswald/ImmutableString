#include "common.h"

#include <immutable_string/string.hxx>

static const char* EMPTY_STRING = "";
static const char* TEST_STRING = "test_string";
static const size_t TEST_STRING_LEN = std::strlen(TEST_STRING);

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
    }

    // from NULL C string + NotOwning
    {
        immutable_string s(nullptr, immutable_string::NotOwning);
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.length(), 0);
        EXPECT_EQ(s.size(), 0);
        ASSERT_TRUE(!!s.c_str());
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_STREQ(s.c_str(), "");
    }

    // from empty C string + NotOwning
    {
        immutable_string s(EMPTY_STRING, immutable_string::NotOwning);
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.length(), 0);
        EXPECT_EQ(s.size(), 0);
        EXPECT_EQ(s.c_str(), EMPTY_STRING);
        EXPECT_EQ(s.data(), s.c_str());
    }

    // from C string + NotOwning
    {
        
        immutable_string s(TEST_STRING, immutable_string::NotOwning);
        EXPECT_FALSE(s.empty());
        EXPECT_EQ(s.length(), TEST_STRING_LEN);
        EXPECT_EQ(s.size(), TEST_STRING_LEN);
        EXPECT_EQ(s.c_str(), TEST_STRING);
        EXPECT_EQ(s.data(), s.c_str());
    }

    // from NULL C string + Clone
    {
        immutable_string s(nullptr, immutable_string::Clone);
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.length(), 0);
        EXPECT_EQ(s.size(), 0);
        ASSERT_TRUE(!!s.c_str());
        EXPECT_EQ(s.data(), s.c_str());
        EXPECT_STREQ(s.c_str(), "");
    }

    // from empty C string + clone
    {
        immutable_string s(EMPTY_STRING, immutable_string::Clone);
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.length(), 0);
        EXPECT_EQ(s.size(), 0);
        EXPECT_NE(s.c_str(), EMPTY_STRING);
        EXPECT_STREQ(s.c_str(), EMPTY_STRING);
        EXPECT_EQ(s.data(), s.c_str());
    }

    // from C string + Clone
    {
        immutable_string s(TEST_STRING, immutable_string::Clone);
        EXPECT_FALSE(s.empty());
        EXPECT_EQ(s.length(), TEST_STRING_LEN);
        EXPECT_EQ(s.size(), TEST_STRING_LEN);
        EXPECT_NE(s.c_str(), TEST_STRING);
        EXPECT_STREQ(s.c_str(), TEST_STRING);
        EXPECT_EQ(s.data(), s.c_str());
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
        immutable_string dst("not this", immutable_string::Clone);
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

    // copy-construct from NotOwning string
    {
        immutable_string src(TEST_STRING, immutable_string::NotOwning);
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

    // copy-assign from NotOwning string
    {
        immutable_string src(TEST_STRING, immutable_string::NotOwning);
        immutable_string dst("not this", immutable_string::Clone);
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

    // copy-construct from Shared string
    {
        immutable_string src(TEST_STRING, immutable_string::Clone);
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

    // copy-assign from Shared string
    {
        immutable_string src(TEST_STRING, immutable_string::Clone);
        immutable_string dst("not this", immutable_string::Clone);
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
        immutable_string dst("not this", immutable_string::Clone);
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

    // move-construct from NotOwning string
    {
        immutable_string src(TEST_STRING, immutable_string::NotOwning);
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

    // move-assign from NotOwning string
    {
        immutable_string src(TEST_STRING, immutable_string::NotOwning);
        immutable_string dst("not this", immutable_string::Clone);
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

    // move-construct from Shared string
    {
        immutable_string src(TEST_STRING, immutable_string::Clone);
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

    // move-assign from Shared string
    {
        immutable_string src(TEST_STRING, immutable_string::Clone);
        immutable_string dst("not this", immutable_string::Clone);
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
