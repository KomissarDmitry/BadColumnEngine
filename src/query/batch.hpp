#pragma once

#include "../core/column.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace columnar {

struct Batch {
    std::vector<std::shared_ptr<Column>> columns;
    size_t num_rows = 0;
};

struct IOperator {
    virtual ~IOperator() = default;
    virtual std::optional<Batch> Next() = 0;

    virtual std::string name() const { return "Operator"; }
    virtual std::vector<const IOperator*> children() const { return {}; }
};

}
