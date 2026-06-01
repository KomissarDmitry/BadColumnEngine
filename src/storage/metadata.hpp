#pragma once

#include "../core/types.hpp"
#include "../io/columnar_store.hpp"
#include <cstdint>
#include <optional>

namespace columnar {

struct ColumnStats {
    bool has_minmax = false;
    Value min_value;
    Value max_value;
};

class TableMetadata {
public:
    explicit TableMetadata(StoreReader& reader) : reader_(reader) {}

    size_t num_row_groups() const { return reader_.num_row_groups(); }
    size_t num_columns() const { return reader_.num_columns(); }
    uint64_t rows_in_group(size_t rg) const { return reader_.rows_in_group(rg); }

    uint64_t total_rows() const {
        uint64_t n = 0;
        for (size_t rg = 0; rg < reader_.num_row_groups(); ++rg)
            n += reader_.rows_in_group(rg);
        return n;
    }

    ColumnStats stats(size_t rg, size_t col) const {
        ColumnStats s;
        if (reader_.has_stats(rg, col) &&
            reader_.schema().column(col).type == DataType::Int64) {
            auto [mn, mx] = reader_.int_stats(rg, col);
            s.has_minmax = true;
            s.min_value = Value(mn);
            s.max_value = Value(mx);
        }
        return s;
    }

    bool may_contain_int(size_t rg, size_t col, int64_t target) const {
        ColumnStats s = stats(rg, col);
        if (!s.has_minmax) return true;
        return s.min_value.as_int() <= target && target <= s.max_value.as_int();
    }

private:
    StoreReader& reader_;
};

}
