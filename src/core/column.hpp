#pragma once

#include "types.hpp"
#include <cstdint>
#include <memory>
#include <ostream>
#include <istream>
#include <string>
#include <vector>

namespace columnar {

class Column {
public:
    virtual ~Column() = default;

    virtual DataType type() const = 0;
    virtual size_t size() const = 0;

    virtual std::string get_string(size_t i) const = 0;
    virtual void append_from_string(const std::string& s) = 0;

    virtual void write_binary(std::ostream& out) const = 0;
    virtual void read_binary(std::istream& in, size_t count) = 0;

    virtual void write_binary_range(std::ostream& out, size_t start, size_t count) const = 0;

    virtual std::shared_ptr<Column> gather(const std::vector<uint32_t>& idx) const = 0;
};

class Int64Column : public Column {
public:
    DataType type() const override { return DataType::Int64; }
    size_t size() const override { return data_.size(); }

    std::string get_string(size_t i) const override;
    void append_from_string(const std::string& s) override;

    void write_binary(std::ostream& out) const override;
    void read_binary(std::istream& in, size_t count) override;
    void write_binary_range(std::ostream& out, size_t start, size_t count) const override;
    std::shared_ptr<Column> gather(const std::vector<uint32_t>& idx) const override;

    void push(int64_t v) { data_.push_back(v); }
    const std::vector<int64_t>& data() const { return data_; }

private:
    std::vector<int64_t> data_;
};

class Float64Column : public Column {
public:
    DataType type() const override { return DataType::Float64; }
    size_t size() const override { return data_.size(); }

    std::string get_string(size_t i) const override;
    void append_from_string(const std::string& s) override;

    void write_binary(std::ostream& out) const override;
    void read_binary(std::istream& in, size_t count) override;
    void write_binary_range(std::ostream& out, size_t start, size_t count) const override;
    std::shared_ptr<Column> gather(const std::vector<uint32_t>& idx) const override;

    void push(double v) { data_.push_back(v); }
    const std::vector<double>& data() const { return data_; }

private:
    std::vector<double> data_;
};

class StringColumn : public Column {
public:
    DataType type() const override { return DataType::String; }
    size_t size() const override { return data_.size(); }

    std::string get_string(size_t i) const override;
    void append_from_string(const std::string& s) override;

    void write_binary(std::ostream& out) const override;
    void read_binary(std::istream& in, size_t count) override;
    void write_binary_range(std::ostream& out, size_t start, size_t count) const override;
    std::shared_ptr<Column> gather(const std::vector<uint32_t>& idx) const override;

    const std::vector<std::string>& data() const { return data_; }

private:
    std::vector<std::string> data_;
};

std::unique_ptr<Column> make_column(DataType type);

}
