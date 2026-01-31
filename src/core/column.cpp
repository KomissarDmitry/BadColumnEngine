#include "column.hpp"
#include <stdexcept>

/*
TODO:
Возможно переделать бинарный формат

*/


namespace columnar {

std::string Int64Column::get_string(size_t i) const {
    return std::to_string(data_[i]);
}

void Int64Column::append_from_string(const std::string& s) {
    if (s.empty()) {
        data_.push_back(0);
    } else {
        data_.push_back(std::stoll(s));
    }
}

void Int64Column::write_binary(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(data_.data()), 
              data_.size() * sizeof(int64_t));
}

void Int64Column::read_binary(std::istream& in, size_t count) {
    data_.resize(count);
    in.read(reinterpret_cast<char*>(data_.data()), 
            count * sizeof(int64_t));
}





std::string StringColumn::get_string(size_t i) const {
    return data_[i];
}

void StringColumn::append_from_string(const std::string& s) {
    data_.push_back(s);
}

// Формат: [длина1][строка1][длина2][строка2]...
void StringColumn::write_binary(std::ostream& out) const {
    for (const auto& s : data_) {
        uint32_t len = s.size();
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(s.data(), len);
    }
}

void StringColumn::read_binary(std::istream& in, size_t count) {
    data_.clear();
    data_.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        uint32_t len;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        
        std::string s(len, '\0');
        in.read(s.data(), len);
        data_.push_back(std::move(s));
    }
}






std::unique_ptr<Column> make_column(DataType type) {
    switch (type) {
        case DataType::Int64:  return std::make_unique<Int64Column>();
        case DataType::String: return std::make_unique<StringColumn>();
    }
    throw std::runtime_error("Unknown type");
}

}
