#include <gtest/gtest.h>
#include "../src/core/types.hpp"

using namespace columnar;

TEST(ValueTest, IntValue) {
    Value v(int64_t{42});
    EXPECT_TRUE(v.is_int());
    EXPECT_EQ(v.type(), DataType::Int64);
    EXPECT_EQ(v.as_int(), 42);
    EXPECT_EQ(v.to_string(), "42");
}

TEST(ValueTest, StringValue) {
    Value v(std::string{"hi"});
    EXPECT_TRUE(v.is_string());
    EXPECT_EQ(v.as_string(), "hi");
}

TEST(ValueTest, Comparison) {
    EXPECT_TRUE(Value(int64_t{1}) < Value(int64_t{2}));
    EXPECT_TRUE(Value(int64_t{5}) == Value(int64_t{5}));
}
