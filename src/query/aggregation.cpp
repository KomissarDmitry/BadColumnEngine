#include "aggregation.hpp"
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace columnar {

int64_t Sum(const Int64Column& c) {
    int64_t s = 0;
    for (int64_t v : c.data()) s += v;
    return s;
}
double Sum(const Float64Column& c) {
    double s = 0;
    for (double v : c.data()) s += v;
    return s;
}

int64_t Min(const Int64Column& c) {
    if (c.data().empty()) throw std::runtime_error("Min of empty column");
    int64_t m = std::numeric_limits<int64_t>::max();
    for (int64_t v : c.data()) m = std::min(m, v);
    return m;
}
int64_t Max(const Int64Column& c) {
    if (c.data().empty()) throw std::runtime_error("Max of empty column");
    int64_t m = std::numeric_limits<int64_t>::min();
    for (int64_t v : c.data()) m = std::max(m, v);
    return m;
}
double Min(const Float64Column& c) {
    if (c.data().empty()) throw std::runtime_error("Min of empty column");
    double m = std::numeric_limits<double>::infinity();
    for (double v : c.data()) m = std::min(m, v);
    return m;
}
double Max(const Float64Column& c) {
    if (c.data().empty()) throw std::runtime_error("Max of empty column");
    double m = -std::numeric_limits<double>::infinity();
    for (double v : c.data()) m = std::max(m, v);
    return m;
}

uint64_t Count(const Column& c) {
    return static_cast<uint64_t>(c.size());
}

double Avg(const Int64Column& c) {
    if (c.data().empty()) return 0.0;
    return static_cast<double>(Sum(c)) / c.data().size();
}
double Avg(const Float64Column& c) {
    if (c.data().empty()) return 0.0;
    return Sum(c) / c.data().size();
}

uint64_t CountDistinct(const Column& c) {
    std::unordered_set<std::string> seen;
    seen.reserve(c.size());
    for (size_t i = 0; i < c.size(); ++i) seen.insert(c.get_string(i));
    return static_cast<uint64_t>(seen.size());
}

}
