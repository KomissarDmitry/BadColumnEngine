#include <gtest/gtest.h>
#include "../src/io/columnar_file.hpp"
#include "../src/io/csv.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class RoundtripTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "columnar_roundtrip_test";
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
};

TEST_F(RoundtripTest, SimpleRoundtrip) {
    auto data_csv = test_dir_ / "data.csv";
    auto schema_csv = test_dir_ / "schema.csv";
    auto columnar_file = test_dir_ / "output.bc";
    auto restored_data = test_dir_ / "restored_data.csv";
    auto restored_schema = test_dir_ / "restored_schema.csv";

    {
        std::ofstream f(data_csv);
        f << "1,hello\n2,world\n3,test\n";
    }
    {
        std::ofstream f(schema_csv);
        f << "id,int64\nname,string\n";
    }

    columnar::ColumnarFile::csv_to_columnar(data_csv, schema_csv, columnar_file);

    columnar::ColumnarFile::columnar_to_csv(columnar_file, restored_data, restored_schema);

    columnar::CsvReader reader(restored_data);
    std::vector<std::string> row;

    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[0], "1");
    EXPECT_EQ(row[1], "hello");

    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[0], "2");
    EXPECT_EQ(row[1], "world");

    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[0], "3");
    EXPECT_EQ(row[1], "test");

    EXPECT_FALSE(reader.read_row(row));
}

TEST_F(RoundtripTest, EscapedCharsRoundtrip) {
    auto data_csv = test_dir_ / "data.csv";
    auto schema_csv = test_dir_ / "schema.csv";
    auto columnar_file = test_dir_ / "output.bc";
    auto restored_data = test_dir_ / "restored_data.csv";
    auto restored_schema = test_dir_ / "restored_schema.csv";

    {
        std::ofstream f(data_csv);
        f << "1,\"hello, world\"\n2,\"say \"\"hi\"\"\"\n";
    }
    {
        std::ofstream f(schema_csv);
        f << "id,int64\ntext,string\n";
    }

    columnar::ColumnarFile::csv_to_columnar(data_csv, schema_csv, columnar_file);
    columnar::ColumnarFile::columnar_to_csv(columnar_file, restored_data, restored_schema);

    columnar::CsvReader reader(restored_data);
    std::vector<std::string> row;

    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[1], "hello, world");

    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[1], "say \"hi\"");
}

TEST_F(RoundtripTest, MultipleColumnsRoundtrip) {
    auto data_csv = test_dir_ / "data.csv";
    auto schema_csv = test_dir_ / "schema.csv";
    auto columnar_file = test_dir_ / "output.bc";
    auto restored_data = test_dir_ / "restored_data.csv";
    auto restored_schema = test_dir_ / "restored_schema.csv";

    {
        std::ofstream f(data_csv);
        f << "1,2,first,4\n5,6,second,8\n";
    }
    {
        std::ofstream f(schema_csv);
        f << "a,int64\nb,int64\nname,string\nd,int64\n";
    }

    columnar::ColumnarFile::csv_to_columnar(data_csv, schema_csv, columnar_file);
    columnar::ColumnarFile::columnar_to_csv(columnar_file, restored_data, restored_schema);

    columnar::CsvReader reader(restored_data);
    std::vector<std::string> row;

    ASSERT_TRUE(reader.read_row(row));
    ASSERT_EQ(row.size(), 4);
    EXPECT_EQ(row[0], "1");
    EXPECT_EQ(row[1], "2");
    EXPECT_EQ(row[2], "first");
    EXPECT_EQ(row[3], "4");
}

TEST_F(RoundtripTest, SchemaPreserved) {
    auto data_csv = test_dir_ / "data.csv";
    auto schema_csv = test_dir_ / "schema.csv";
    auto columnar_file = test_dir_ / "output.bc";
    auto restored_data = test_dir_ / "restored_data.csv";
    auto restored_schema = test_dir_ / "restored_schema.csv";

    {
        std::ofstream f(data_csv);
        f << "1,hello\n";
    }
    {
        std::ofstream f(schema_csv);
        f << "my_id,int64\nmy_name,string\n";
    }

    columnar::ColumnarFile::csv_to_columnar(data_csv, schema_csv, columnar_file);
    columnar::ColumnarFile::columnar_to_csv(columnar_file, restored_data, restored_schema);

    auto schema = columnar::Schema::read_from_csv(restored_schema);

    ASSERT_EQ(schema.column_count(), 2);
    EXPECT_EQ(schema.column(0).name, "my_id");
    EXPECT_EQ(schema.column(0).type, columnar::DataType::Int64);
    EXPECT_EQ(schema.column(1).name, "my_name");
    EXPECT_EQ(schema.column(1).type, columnar::DataType::String);
}
