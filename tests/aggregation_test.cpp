#include <gtest/gtest.h>
#include "../src/query/aggregation.hpp"

using namespace columnar;

TEST(AggregationTest, SumMinMaxAvgInt) {
    Int64Column c;
    for (int v : {3, 1, 4, 1, 5}) c.push(v);
    EXPECT_EQ(Sum(c), 14);
    EXPECT_EQ(Min(c), 1);
    EXPECT_EQ(Max(c), 5);
    EXPECT_EQ(Count(c), 5u);
    EXPECT_DOUBLE_EQ(Avg(c), 14.0 / 5);
}

TEST(AggregationTest, SumFloat) {
    Float64Column c;
    c.push(1.5); c.push(2.5);
    EXPECT_DOUBLE_EQ(Sum(c), 4.0);
    EXPECT_DOUBLE_EQ(Avg(c), 2.0);
}

TEST(AggregationTest, CountDistinct) {
    StringColumn c;
    for (const char* s : {"a", "b", "a", "c", "b"}) c.append_from_string(s);
    EXPECT_EQ(CountDistinct(c), 3u);
}

TEST(AggregationTest, AvgEmptyIsZero) {
    Int64Column c;
    EXPECT_DOUBLE_EQ(Avg(c), 0.0);
}
