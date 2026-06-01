#pragma once

#include "../core/column.hpp"
#include "batch.hpp"
#include <string>
#include <vector>

namespace columnar {

class RowEncoder {
public:

    static void encode_cell(std::string& key, const Column& c, size_t row);

    static void encode_row(std::string& key, const Batch& batch,
                           const std::vector<size_t>& group_cols, size_t row);
};

void append_cell(Column& dst, const Column& src, size_t row);
int  compare_cell(const Column& a, size_t ra, const Column& b, size_t rb);

}
