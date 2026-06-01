#pragma once

#include <fstream>
#include <string>
#include <vector>

namespace columnar {

class CsvReader {
public:
    explicit CsvReader(const std::string& path);
    ~CsvReader();

    bool read_row(std::vector<std::string>& fields);

private:
    std::ifstream file_;

    std::vector<std::string> parse_line(const std::string& line);
};

class CsvWriter {
public:
    explicit CsvWriter(const std::string& path);
    ~CsvWriter();

    void write_row(const std::vector<std::string>& fields);

private:
    std::ofstream file_;

    std::string escape_field(const std::string& field);
};

}
