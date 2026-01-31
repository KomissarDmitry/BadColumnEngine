#pragma once

#include "types.hpp"
#include <iostream>
#include <memory>
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
};


class Int64Column : public Column {
public:
    DataType type() const override { return DataType::Int64; }
    size_t size() const override { return data_.size(); }
    
    std::string get_string(size_t i) const override;
    void append_from_string(const std::string& s) override;
    
    void write_binary(std::ostream& out) const override;
    void read_binary(std::istream& in, size_t count) override;
    
    const std::vector<int64_t>& data() const { return data_; }
    
private:
    std::vector<int64_t> data_;
};



class StringColumn : public Column {
public:
    DataType type() const override { return DataType::String; }
    size_t size() const override { return data_.size(); }
    
    std::string get_string(size_t i) const override;
    void append_from_string(const std::string& s) override;
    
    void write_binary(std::ostream& out) const override;
    void read_binary(std::istream& in, size_t count) override;
    
    const std::vector<std::string>& data() const { return data_; }
    
private:
    std::vector<std::string> data_;
};


std::unique_ptr<Column> make_column(DataType type);

}
