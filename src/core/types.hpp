#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

namespace columnar {

enum class DataType {
    Int64,
    Float64,
    String
};

inline DataType string_to_type(const std::string& s) {
    if (s == "int64")   return DataType::Int64;
    if (s == "float64") return DataType::Float64;
    if (s == "string")  return DataType::String;
    throw std::runtime_error("Unknown type: " + s);
}

inline std::string type_to_string(DataType t) {
    switch (t) {
        case DataType::Int64:   return "int64";
        case DataType::Float64: return "float64";
        case DataType::String:  return "string";
    }
    return "unknown";
}

class Value {
public:
    Value() : data_(int64_t{0}) {}
    explicit Value(int64_t v) : data_(v) {}
    explicit Value(double v) : data_(v) {}
    explicit Value(std::string v) : data_(std::move(v)) {}

    DataType type() const {
        switch (data_.index()) {
            case 0: return DataType::Int64;
            case 1: return DataType::Float64;
            default: return DataType::String;
        }
    }

    int64_t as_int() const { return std::get<int64_t>(data_); }
    double  as_float() const { return std::get<double>(data_); }
    const std::string& as_string() const { return std::get<std::string>(data_); }

    bool is_int()    const { return data_.index() == 0; }
    bool is_float()  const { return data_.index() == 1; }
    bool is_string() const { return data_.index() == 2; }

    std::string to_string() const {
        switch (data_.index()) {
            case 0: return std::to_string(std::get<int64_t>(data_));
            case 1: return std::to_string(std::get<double>(data_));
            default: return std::get<std::string>(data_);
        }
    }

    bool operator==(const Value& o) const { return data_ == o.data_; }
    bool operator<(const Value& o) const { return data_ < o.data_; }

private:
    std::variant<int64_t, double, std::string> data_;
};

}
