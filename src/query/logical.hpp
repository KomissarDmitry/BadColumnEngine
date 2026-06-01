#pragma once

#include "batch.hpp"
#include <ostream>
#include <sstream>
#include <string>

namespace columnar {

inline void explain_plan(const IOperator& op, std::ostream& out, int indent = 0) {
    for (int i = 0; i < indent; ++i) out << "  ";
    out << op.name() << "\n";
    for (const IOperator* child : op.children())
        if (child) explain_plan(*child, out, indent + 1);
}

inline std::string explain_to_string(const IOperator& op) {
    std::ostringstream oss;
    explain_plan(op, oss, 0);
    return oss.str();
}

}
