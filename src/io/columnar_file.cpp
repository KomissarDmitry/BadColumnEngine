#include "columnar_file.hpp"
#include "csv.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace columnar {

void ColumnarFile::csv_to_columnar(const std::string& data_csv,
                                   const std::string& schema_csv,
                                   const std::string& output) {
    Schema schema = Schema::read_from_csv(schema_csv);
    
    std::vector<std::unique_ptr<Column>> columns;
    for (size_t i = 0; i < schema.column_count(); ++i) {
        columns.push_back(make_column(schema.column(i).type));
    }
    
    CsvReader reader(data_csv);
    std::vector<std::string> row;
    size_t row_count = 0;
    
    while (reader.read_row(row)) {
        if (row.size() != schema.column_count()) {
            throw std::runtime_error("Row has wrong number of fields");
        }
        
        for (size_t i = 0; i < row.size(); ++i) {
            columns[i]->append_from_string(row[i]);
        }
        ++row_count;
    }
    
    std::cout << "Read " << row_count << " rows from CSV\n";
    
    write_file(output, schema, columns);
    


    std::cout << "Written to " << output << "\n";
}




void ColumnarFile::columnar_to_csv(const std::string& input,
                                   const std::string& data_csv,
                                   const std::string& schema_csv) {
    Schema schema;
    std::vector<std::unique_ptr<Column>> columns;
    read_file(input, schema, columns);
    
    std::cout << "Read " << columns[0]->size() << " rows from columnar\n";
    
    schema.write_to_csv(schema_csv);
    
    CsvWriter writer(data_csv);
    size_t row_count = columns.empty() ? 0 : columns[0]->size();
    
    for (size_t row = 0; row < row_count; ++row) {
        std::vector<std::string> fields;
        for (const auto& col : columns) {
            fields.push_back(col->get_string(row));
        }
        writer.write_row(fields);
    }
    
    std::cout << "Written to " << data_csv << " and " << schema_csv << "\n";
}


void ColumnarFile::write_file(const std::string& path,
                             const Schema& schema,
                             const std::vector<std::unique_ptr<Column>>& columns) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing: " + path);
    }
    
    std::filesystem::path file_extension{path};
    if (file_extension.extension() != ".bc") {
        throw std::runtime_error("Expected a .bc file, got: " + file_extension.string());
    }
    
    uint32_t col_count = schema.column_count();
    file.write(reinterpret_cast<const char*>(&col_count), sizeof(col_count));
    
    uint64_t row_count = columns.empty() ? 0 : columns[0]->size();
    file.write(reinterpret_cast<const char*>(&row_count), sizeof(row_count));
    

    for (size_t i = 0; i < schema.column_count(); ++i) {
        const auto& col_info = schema.column(i);
        
        uint32_t name_len = col_info.name.size();
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
        file.write(col_info.name.data(), name_len);
        
        uint8_t type = static_cast<uint8_t>(col_info.type);
        file.write(reinterpret_cast<const char*>(&type), sizeof(type));
    }
    
    for (const auto& col : columns) {
        col->write_binary(file);
    }
}





void ColumnarFile::read_file(const std::string& path,
                            Schema& schema,
                            std::vector<std::unique_ptr<Column>>& columns) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    
    std::filesystem::path file_extension{path};
    if (file_extension.extension() != ".bc") {
        throw std::runtime_error("Expected a .bc file, got: " + file_extension.string());
    }
    
    uint32_t col_count;
    file.read(reinterpret_cast<char*>(&col_count), sizeof(col_count));
    
    uint64_t row_count;
    file.read(reinterpret_cast<char*>(&row_count), sizeof(row_count));
    
    schema = Schema();
    for (uint32_t i = 0; i < col_count; ++i) {
        uint32_t name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
        std::string name(name_len, '\0');
        file.read(name.data(), name_len);

        uint8_t type;
        file.read(reinterpret_cast<char*>(&type), sizeof(type));
        
        schema.add_column(name, static_cast<DataType>(type));
    }
    
    columns.clear();
    for (uint32_t i = 0; i < col_count; ++i) {
        auto col = make_column(schema.column(i).type);
        col->read_binary(file, row_count);
        columns.push_back(std::move(col));
    }
}

}
