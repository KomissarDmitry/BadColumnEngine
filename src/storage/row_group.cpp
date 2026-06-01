#include "row_group.hpp"
#include <stdexcept>

namespace columnar {

RowGroup RowGroupReader::read_next() {
    if (!has_next()) throw std::runtime_error("RowGroupReader: no more row groups");
    size_t cur = rg_++;
    RowGroup g;
    g.num_rows = reader_.rows_in_group(cur);
    for (size_t c : col_ids_)
        g.columns.push_back(reader_.read_chunk(cur, c));
    return g;
}

}
