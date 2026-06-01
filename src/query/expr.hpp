#pragma once

#include "batch.hpp"
#include "../core/column.hpp"
#include <memory>
#include <string>
#include <vector>

namespace columnar {

class Expr {
public:
    virtual ~Expr() = default;
    virtual std::shared_ptr<Column> eval(const Batch& in) const = 0;
};

using ExprPtr = std::shared_ptr<Expr>;

ExprPtr col(size_t idx);
ExprPtr lit_i(int64_t v);
ExprPtr lit_s(std::string s);

enum class CmpKind { Eq, Ne, Lt, Le, Gt, Ge };
ExprPtr cmp(CmpKind k, ExprPtr l, ExprPtr r);

ExprPtr and_(ExprPtr a, ExprPtr b);
ExprPtr or_(ExprPtr a, ExprPtr b);
ExprPtr not_(ExprPtr a);

ExprPtr like(ExprPtr s, std::string pattern);

ExprPtr len_str(ExprPtr s);

enum class ArOp { Add, Sub, Mul, Div, Mod };
ExprPtr arith(ArOp op, ExprPtr l, ExprPtr r);

ExprPtr in_int(ExprPtr v, std::vector<int64_t> set);

ExprPtr case_when(ExprPtr cond, ExprPtr a, ExprPtr b);

ExprPtr extract_minute(ExprPtr eventtime);

ExprPtr date_trunc_minute(ExprPtr eventtime);

ExprPtr z_order_2d(ExprPtr a, ExprPtr b);

ExprPtr json_get(ExprPtr s, std::string key);
ExprPtr json_get_int(ExprPtr s, std::string key);

ExprPtr array_length(ExprPtr s);
ExprPtr array_contains_int(ExprPtr s, int64_t value);

ExprPtr haversine(ExprPtr lat1, ExprPtr lng1, ExprPtr lat2, ExprPtr lng2);

ExprPtr parse_decimal(ExprPtr s, int scale);
ExprPtr format_decimal(ExprPtr v, int scale);
ExprPtr decimal_mul(ExprPtr a, ExprPtr b, int scale);

}
