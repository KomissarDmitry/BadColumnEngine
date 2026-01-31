#include <gtest/gtest.h>
#include "../src/io/csv.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

class CsvTest : public ::testing::Test {
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

// Тест простого чтения CSV
TEST_F(CsvTest, ReadSimpleCsv) {
    auto path = test_dir_ / "simple.csv";
    {
        std::ofstream f(path);
        f << "1,2,3\n4,5,6\n";
    }
    
    columnar::CsvReader reader(path);
    std::vector<std::string> row;
    
    ASSERT_TRUE(reader.read_row(row));
    ASSERT_EQ(row.size(), 3);
    EXPECT_EQ(row[0], "1");
    EXPECT_EQ(row[1], "2");
    EXPECT_EQ(row[2], "3");
    
    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[0], "4");
    
    EXPECT_FALSE(reader.read_row(row));
}

// Тест экранированных запятых
TEST_F(CsvTest, ReadQuotedComma) {
    auto path = test_dir_ / "quoted.csv";
    {
        std::ofstream f(path);
        f << "1,\"hello, world\",3\n";
    }
    
    columnar::CsvReader reader(path);
    std::vector<std::string> row;
    
    ASSERT_TRUE(reader.read_row(row));
    ASSERT_EQ(row.size(), 3);
    EXPECT_EQ(row[1], "hello, world");
}

// Тест экранированных кавычек
TEST_F(CsvTest, ReadQuotedQuotes) {
    auto path = test_dir_ / "quotes.csv";
    {
        std::ofstream f(path);
        f << "1,\"say \"\"hi\"\"\",3\n";
    }
    
    columnar::CsvReader reader(path);
    std::vector<std::string> row;
    
    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[1], "say \"hi\"");
}

// Тест записи CSV
TEST_F(CsvTest, WriteSimpleCsv) {
    auto path = test_dir_ / "output.csv";
    {
        columnar::CsvWriter writer(path);
        writer.write_row({"1", "2", "3"});
        writer.write_row({"a", "b", "c"});
    }
    
    columnar::CsvReader reader(path);
    std::vector<std::string> row;
    
    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[0], "1");
    
    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[0], "a");
}

// Тест записи с экранированием
TEST_F(CsvTest, WriteWithEscaping) {
    auto path = test_dir_ / "escape.csv";
    {
        columnar::CsvWriter writer(path);
        writer.write_row({"hello, world", "say \"hi\""});
    }
    
    columnar::CsvReader reader(path);
    std::vector<std::string> row;
    
    ASSERT_TRUE(reader.read_row(row));
    EXPECT_EQ(row[0], "hello, world");
    EXPECT_EQ(row[1], "say \"hi\"");
}

// Тест пустого файла
TEST_F(CsvTest, ReadEmptyFile) {
    auto path = test_dir_ / "empty.csv";
    { std::ofstream f(path); }
    
    columnar::CsvReader reader(path);
    std::vector<std::string> row;
    
    EXPECT_FALSE(reader.read_row(row));
}
