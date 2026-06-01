#include <gtest/gtest.h>
#include "../src/core/column.hpp"
#include <sstream>

TEST(Int64ColumnTest, AppendAndGet) {
    columnar::Int64Column col;

    col.append_from_string("42");
    col.append_from_string("-10");
    col.append_from_string("0");

    ASSERT_EQ(col.size(), 3);
    EXPECT_EQ(col.get_string(0), "42");
    EXPECT_EQ(col.get_string(1), "-10");
    EXPECT_EQ(col.get_string(2), "0");
}

TEST(Int64ColumnTest, EmptyStringAsZero) {
    columnar::Int64Column col;
    col.append_from_string("");

    EXPECT_EQ(col.get_string(0), "0");
}

TEST(Int64ColumnTest, BinarySerialization) {
    columnar::Int64Column col;
    col.append_from_string("1");
    col.append_from_string("2");
    col.append_from_string("3");

    std::stringstream ss;
    col.write_binary(ss);

    columnar::Int64Column col2;
    col2.read_binary(ss, 3);

    ASSERT_EQ(col2.size(), 3);
    EXPECT_EQ(col2.get_string(0), "1");
    EXPECT_EQ(col2.get_string(1), "2");
    EXPECT_EQ(col2.get_string(2), "3");
}

TEST(StringColumnTest, AppendAndGet) {
    columnar::StringColumn col;

    col.append_from_string("hello");
    col.append_from_string("world");
    col.append_from_string("");

    ASSERT_EQ(col.size(), 3);
    EXPECT_EQ(col.get_string(0), "hello");
    EXPECT_EQ(col.get_string(1), "world");
    EXPECT_EQ(col.get_string(2), "");
}

TEST(StringColumnTest, BinarySerialization) {
    columnar::StringColumn col;
    col.append_from_string("foo");
    col.append_from_string("bar");
    col.append_from_string("baz");

    std::stringstream ss;
    col.write_binary(ss);

    columnar::StringColumn col2;
    col2.read_binary(ss, 3);

    ASSERT_EQ(col2.size(), 3);
    EXPECT_EQ(col2.get_string(0), "foo");
    EXPECT_EQ(col2.get_string(1), "bar");
    EXPECT_EQ(col2.get_string(2), "baz");
}

TEST(ColumnFactoryTest, MakeColumn) {
    auto int_col = columnar::make_column(columnar::DataType::Int64);
    EXPECT_EQ(int_col->type(), columnar::DataType::Int64);

    auto str_col = columnar::make_column(columnar::DataType::String);
    EXPECT_EQ(str_col->type(), columnar::DataType::String);
}
