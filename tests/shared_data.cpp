#include "common.h"

#include <immutable_string/shared_data.hxx>


struct alignas(64) A // require some large alignment
{
    int32_t v;
};

static const A ka[3] = { { 1 }, { -2 }, { 3 } };

using SA = shared_data<A>;

TEST(shared_data, create)
{
    auto a = SA::create(10, ka, sizeof(ka) / sizeof(ka[0]));
    ASSERT_TRUE(!!a);
    EXPECT_TRUE(!!a->data());
    EXPECT_TRUE(!!const_cast<const SA*>(a.get())->data());
    EXPECT_EQ(a->capacity(), 10);
    EXPECT_EQ(a->size(), 3);
    auto it = a->data();
    EXPECT_EQ(it[0].v, 1);
    EXPECT_EQ(it[1].v, -2);
    EXPECT_EQ(it[2].v, 3);
}