#include "column.hpp"
#include <cstring>
#include <stdexcept>

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

void Int64Column::write_binary_range(std::ostream& out, size_t start, size_t count) const {
    out.write(reinterpret_cast<const char*>(data_.data() + start),
              count * sizeof(int64_t));
}

void Int64Column::read_binary(std::istream& in, size_t count) {
    data_.resize(count);
    in.read(reinterpret_cast<char*>(data_.data()), count * sizeof(int64_t));
}

std::shared_ptr<Column> Int64Column::gather(const std::vector<uint32_t>& idx) const {
    auto out = std::make_shared<Int64Column>();
    for (uint32_t i : idx) out->data_.push_back(data_[i]);
    return out;
}

std::string Float64Column::get_string(size_t i) const {
    return std::to_string(data_[i]);
}

void Float64Column::append_from_string(const std::string& s) {
    if (s.empty()) {
        data_.push_back(0.0);
    } else {
        data_.push_back(std::stod(s));
    }
}

void Float64Column::write_binary(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(data_.data()),
              data_.size() * sizeof(double));
}

void Float64Column::write_binary_range(std::ostream& out, size_t start, size_t count) const {
    out.write(reinterpret_cast<const char*>(data_.data() + start),
              count * sizeof(double));
}

void Float64Column::read_binary(std::istream& in, size_t count) {
    data_.resize(count);
    in.read(reinterpret_cast<char*>(data_.data()), count * sizeof(double));
}

std::shared_ptr<Column> Float64Column::gather(const std::vector<uint32_t>& idx) const {
    auto out = std::make_shared<Float64Column>();
    for (uint32_t i : idx) out->data_.push_back(data_[i]);
    return out;
}

std::string StringColumn::get_string(size_t i) const {
    return data_[i];
}

void StringColumn::append_from_string(const std::string& s) {
    data_.push_back(s);
}

void StringColumn::write_binary(std::ostream& out) const {
    write_binary_range(out, 0, data_.size());
}

void StringColumn::write_binary_range(std::ostream& out, size_t start, size_t count) const {
    for (size_t i = start; i < start + count; ++i) {
        uint32_t len = static_cast<uint32_t>(data_[i].size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(data_[i].data(), len);
    }
}

void StringColumn::read_binary(std::istream& in, size_t count) {
    data_.clear();
    data_.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        uint32_t len = 0;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string s(len, '\0');
        in.read(s.data(), len);
        data_.push_back(std::move(s));
    }
}

std::shared_ptr<Column> StringColumn::gather(const std::vector<uint32_t>& idx) const {
    auto out = std::make_shared<StringColumn>();
    for (uint32_t i : idx) out->data_.push_back(data_[i]);
    return out;
}

std::unique_ptr<Column> make_column(DataType type) {
    switch (type) {
        case DataType::Int64:   return std::make_unique<Int64Column>();
        case DataType::Float64: return std::make_unique<Float64Column>();
        case DataType::String:  return std::make_unique<StringColumn>();
    }
    throw std::runtime_error("Unknown type");
}

}
