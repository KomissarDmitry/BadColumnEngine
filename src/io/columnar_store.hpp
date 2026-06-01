#pragma once

#include "../core/column.hpp"
#include "../core/schema.hpp"
#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace columnar {

struct ChunkMeta {
    uint64_t offset = 0;
    uint64_t length = 0;
    bool has_stats = false;
    int64_t min_i = 0, max_i = 0;
    double  min_f = 0, max_f = 0;
};

struct RowGroupMeta {
    uint64_t num_rows = 0;
    std::vector<ChunkMeta> columns;
};

class ColumnStoreWriter {
public:

    static void convert_csv(const std::string& data_csv,
                            const std::string& schema_csv,
                            const std::string& out);
};

class StoreReader {
public:
    explicit StoreReader(const std::string& path);

    const Schema& schema() const { return schema_; }
    size_t num_columns() const { return schema_.column_count(); }
    size_t num_row_groups() const { return groups_.size(); }
    uint64_t rows_in_group(size_t rg) const { return groups_[rg].num_rows; }

    size_t column_index(const std::string& name) const;

    bool has_stats(size_t rg, size_t col) const { return groups_[rg].columns[col].has_stats; }
    std::pair<int64_t, int64_t> int_stats(size_t rg, size_t col) const {
        const auto& c = groups_[rg].columns[col];
        return {c.min_i, c.max_i};
    }

    std::shared_ptr<Column> read_chunk(size_t rg, size_t col);

    uint64_t chunk_offset(size_t rg, size_t col) const { return groups_[rg].columns[col].offset; }
    uint64_t chunk_length(size_t rg, size_t col) const { return groups_[rg].columns[col].length; }

private:
    std::ifstream file_;
    Schema schema_;
    std::vector<RowGroupMeta> groups_;
};

}
