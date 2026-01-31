#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace columnar {

enum class DataType {
    Int64,
    String
};


inline DataType string_to_type(const std::string& s) {
    if (s == "int64") return DataType::Int64;
    if (s == "string") return DataType::String;
    throw std::runtime_error("Unknown type: " + s);
}

inline std::string type_to_string(DataType t) {
    switch (t) {
        case DataType::Int64:  return "int64";
        case DataType::String: return "string";
    }
    return "unknown";
}

}
