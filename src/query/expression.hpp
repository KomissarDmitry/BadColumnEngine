#pragma once

#include <cstdint>
#include <string>

namespace columnar {

enum class CompareOp { Eq, Ne, Lt, Le, Gt, Ge };

struct Int64Predicate {
    size_t col_index;
    CompareOp op;
    int64_t value;
};

enum class StrOp { Eq, Ne, Contains, NotEmpty };
struct StringPredicate {
    size_t col_index;
    StrOp op;
    std::string value;
};

inline bool apply_str(const std::string& x, StrOp op, const std::string& v) {
    switch (op) {
        case StrOp::Eq:       return x == v;
        case StrOp::Ne:       return x != v;
        case StrOp::Contains: return x.find(v) != std::string::npos;
        case StrOp::NotEmpty: return !x.empty();
    }
    return false;
}

inline bool apply_cmp(int64_t x, CompareOp op, int64_t v) {
    switch (op) {
        case CompareOp::Eq: return x == v;
        case CompareOp::Ne: return x != v;
        case CompareOp::Lt: return x <  v;
        case CompareOp::Le: return x <= v;
        case CompareOp::Gt: return x >  v;
        case CompareOp::Ge: return x >= v;
    }
    return false;
}

inline bool range_can_match(CompareOp op, int64_t v, int64_t mn, int64_t mx) {
    switch (op) {
        case CompareOp::Eq: return mn <= v && v <= mx;
        case CompareOp::Ne: return !(mn == v && mx == v);
        case CompareOp::Lt: return mn <  v;
        case CompareOp::Le: return mn <= v;
        case CompareOp::Gt: return mx >  v;
        case CompareOp::Ge: return mx >= v;
    }
    return true;
}

}
