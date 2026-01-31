#pragma once

#include "types.hpp"
#include <vector>
#include <string>

namespace columnar {


struct ColumnInfo {
    std::string name;
    DataType type;
};

class Schema {
public:
    Schema() = default;
    
    void add_column(const std::string& name, DataType type);
    
    size_t column_count() const { return columns_.size(); }
    const ColumnInfo& column(size_t i) const { return columns_[i]; }
    const std::vector<ColumnInfo>& columns() const { return columns_; }
    
    static Schema read_from_csv(const std::string& path);
    void write_to_csv(const std::string& path) const;
    
private:
    std::vector<ColumnInfo> columns_;
};

}
