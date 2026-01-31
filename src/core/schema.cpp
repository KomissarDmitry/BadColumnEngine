#include "schema.hpp"
#include <fstream>
#include <sstream>

namespace columnar {

void Schema::add_column(const std::string& name, DataType type) {
    columns_.push_back({name, type});
}

Schema Schema::read_from_csv(const std::string& path) {
    Schema schema;
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open schema file: " + path);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        
        size_t pos = line.find(',');
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid schema line: " + line);
        }
        
        std::string name = line.substr(0, pos);
        std::string type_str = line.substr(pos + 1);
        
        schema.add_column(name, string_to_type(type_str));
    }
    
    return schema;
}

void Schema::write_to_csv(const std::string& path) const {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open file for writing: " + path);
    }
    
    for (const auto& col : columns_) {
        file << col.name << "," << type_to_string(col.type) << "\n";
    }
}

}
