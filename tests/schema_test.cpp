#include <gtest/gtest.h>
#include "../src/core/schema.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class SchemaTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "columnar_test";
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
};

TEST_F(SchemaTest, CreateSchema) {
    columnar::Schema schema;
    schema.add_column("id", columnar::DataType::Int64);
    schema.add_column("name", columnar::DataType::String);

    ASSERT_EQ(schema.column_count(), 2);
    EXPECT_EQ(schema.column(0).name, "id");
    EXPECT_EQ(schema.column(0).type, columnar::DataType::Int64);
    EXPECT_EQ(schema.column(1).name, "name");
    EXPECT_EQ(schema.column(1).type, columnar::DataType::String);
}

TEST_F(SchemaTest, ReadFromCsv) {
    auto path = test_dir_ / "schema.csv";
    {
        std::ofstream f(path);
        f << "a,int64\nb,string\n";
    }

    auto schema = columnar::Schema::read_from_csv(path);

    ASSERT_EQ(schema.column_count(), 2);
    EXPECT_EQ(schema.column(0).name, "a");
    EXPECT_EQ(schema.column(0).type, columnar::DataType::Int64);
    EXPECT_EQ(schema.column(1).name, "b");
    EXPECT_EQ(schema.column(1).type, columnar::DataType::String);
}

TEST_F(SchemaTest, WriteToCsv) {
    columnar::Schema schema;
    schema.add_column("x", columnar::DataType::Int64);
    schema.add_column("y", columnar::DataType::String);

    auto path = test_dir_ / "output_schema.csv";
    schema.write_to_csv(path);

    auto loaded = columnar::Schema::read_from_csv(path);

    ASSERT_EQ(loaded.column_count(), 2);
    EXPECT_EQ(loaded.column(0).name, "x");
    EXPECT_EQ(loaded.column(1).name, "y");
}

TEST(TypesTest, StringToType) {
    EXPECT_EQ(columnar::string_to_type("int64"), columnar::DataType::Int64);
    EXPECT_EQ(columnar::string_to_type("string"), columnar::DataType::String);
    EXPECT_THROW(columnar::string_to_type("unknown"), std::runtime_error);
}

TEST(TypesTest, TypeToString) {
    EXPECT_EQ(columnar::type_to_string(columnar::DataType::Int64), "int64");
    EXPECT_EQ(columnar::type_to_string(columnar::DataType::String), "string");
}
