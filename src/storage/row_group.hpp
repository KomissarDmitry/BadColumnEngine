#pragma once

#include "../core/column.hpp"
#include "../io/columnar_store.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace columnar {

struct RowGroup {
    std::vector<std::shared_ptr<Column>> columns;
    size_t num_rows = 0;
};

class RowGroupReader {
public:

    RowGroupReader(StoreReader& reader, std::vector<size_t> col_ids)
        : reader_(reader), col_ids_(std::move(col_ids)) {}

    bool has_next() const { return rg_ < reader_.num_row_groups(); }

    RowGroup read_next();

    size_t num_row_groups() const { return reader_.num_row_groups(); }

private:
    StoreReader& reader_;
    std::vector<size_t> col_ids_;
    size_t rg_ = 0;
};

}
