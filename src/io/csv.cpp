#include "csv.hpp"
#include <sstream>

namespace columnar {

CsvReader::CsvReader(const std::string& path) : file_(path) {
    if (!file_) {
        throw std::runtime_error("Cannot open CSV file: " + path);
    }
}

CsvReader::~CsvReader() = default;

bool CsvReader::read_row(std::vector<std::string>& fields) {
    std::string line;

    if (!std::getline(file_, line)) {
        return false;
    }

    int quote_count = 0;
    for (char c : line) {
        if (c == '"') quote_count++;
    }

    while (quote_count % 2 != 0) {
        std::string next_line;
        if (!std::getline(file_, next_line)) break;
        line += "\n" + next_line;
        for (char c : next_line) {
            if (c == '"') quote_count++;
        }
    }

    fields = parse_line(line);
    return true;
}

std::vector<std::string> CsvReader::parse_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
    }

    fields.push_back(field);
    return fields;
}

CsvWriter::CsvWriter(const std::string& path) : file_(path) {
    if (!file_) {
        throw std::runtime_error("Cannot open CSV file for writing: " + path);
    }
}

CsvWriter::~CsvWriter() = default;

void CsvWriter::write_row(const std::vector<std::string>& fields) {
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) file_ << ",";
        file_ << escape_field(fields[i]);
    }
    file_ << "\n";
}

std::string CsvWriter::escape_field(const std::string& field) {
    bool need_quotes = false;
    for (char c : field) {
        if (c == ',' || c == '"' || c == '\n') {
            need_quotes = true;
            break;
        }
    }

    if (!need_quotes) {
        return field;
    }

    std::string result = "\"";
    for (char c : field) {
        if (c == '"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

}
