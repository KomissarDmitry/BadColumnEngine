#include "expr.hpp"
#include <regex>
#include <cmath>
#include <stdexcept>

namespace columnar {

namespace {

inline int64_t as_int(const Column& c, size_t r) {
    return static_cast<const Int64Column&>(c).data()[r];
}
inline const std::string& as_str(const Column& c, size_t r) {
    return static_cast<const StringColumn&>(c).data()[r];
}

class ColExpr : public Expr {
public:
    explicit ColExpr(size_t i) : idx_(i) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        return in.columns[idx_];
    }
private:
    size_t idx_;
};

class IntLit : public Expr {
public:
    explicit IntLit(int64_t v) : v_(v) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto c = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i) c->push(v_);
        return c;
    }
private:
    int64_t v_;
};

class StrLit : public Expr {
public:
    explicit StrLit(std::string v) : v_(std::move(v)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto c = std::make_shared<StringColumn>();
        for (size_t i = 0; i < in.num_rows; ++i) c->append_from_string(v_);
        return c;
    }
private:
    std::string v_;
};

class CmpExpr : public Expr {
public:
    CmpExpr(CmpKind k, ExprPtr l, ExprPtr r) : k_(k), l_(std::move(l)), r_(std::move(r)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto lv = l_->eval(in);
        auto rv = r_->eval(in);
        auto out = std::make_shared<Int64Column>();
        size_t n = in.num_rows;
        if (lv->type() == DataType::String && rv->type() == DataType::String) {
            for (size_t i = 0; i < n; ++i) {
                int c = as_str(*lv, i).compare(as_str(*rv, i));
                bool b = false;
                switch (k_) {
                    case CmpKind::Eq: b = c == 0; break;
                    case CmpKind::Ne: b = c != 0; break;
                    case CmpKind::Lt: b = c <  0; break;
                    case CmpKind::Le: b = c <= 0; break;
                    case CmpKind::Gt: b = c >  0; break;
                    case CmpKind::Ge: b = c >= 0; break;
                }
                out->push(b ? 1 : 0);
            }
        } else {
            for (size_t i = 0; i < n; ++i) {
                int64_t x = as_int(*lv, i), y = as_int(*rv, i);
                bool b = false;
                switch (k_) {
                    case CmpKind::Eq: b = x == y; break;
                    case CmpKind::Ne: b = x != y; break;
                    case CmpKind::Lt: b = x <  y; break;
                    case CmpKind::Le: b = x <= y; break;
                    case CmpKind::Gt: b = x >  y; break;
                    case CmpKind::Ge: b = x >= y; break;
                }
                out->push(b ? 1 : 0);
            }
        }
        return out;
    }
private:
    CmpKind k_;
    ExprPtr l_, r_;
};

class BoolExpr : public Expr {
public:
    enum Kind { And, Or, Not };
    BoolExpr(Kind k, ExprPtr a, ExprPtr b) : k_(k), a_(std::move(a)), b_(std::move(b)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto av = a_->eval(in);
        auto out = std::make_shared<Int64Column>();
        size_t n = in.num_rows;
        if (k_ == Not) {
            for (size_t i = 0; i < n; ++i) out->push(as_int(*av, i) ? 0 : 1);
            return out;
        }
        auto bv = b_->eval(in);
        if (k_ == And) {
            for (size_t i = 0; i < n; ++i) out->push((as_int(*av, i) && as_int(*bv, i)) ? 1 : 0);
        } else {
            for (size_t i = 0; i < n; ++i) out->push((as_int(*av, i) || as_int(*bv, i)) ? 1 : 0);
        }
        return out;
    }
private:
    Kind k_;
    ExprPtr a_, b_;
};

class LikeExpr : public Expr {
public:
    LikeExpr(ExprPtr s, std::string p) : s_(std::move(s)), pat_(std::move(p)) { compile(); }
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto sv = s_->eval(in);
        auto out = std::make_shared<Int64Column>();
        size_t n = in.num_rows;
        for (size_t i = 0; i < n; ++i) {
            const std::string& x = as_str(*sv, i);
            bool ok = false;
            switch (mode_) {
                case Mode::Contains: ok = x.find(needle_) != std::string::npos; break;
                case Mode::Prefix:   ok = x.size() >= needle_.size() && x.compare(0, needle_.size(), needle_) == 0; break;
                case Mode::Suffix:   ok = x.size() >= needle_.size() &&
                                          x.compare(x.size() - needle_.size(), needle_.size(), needle_) == 0; break;
                case Mode::Equal:    ok = x == needle_; break;
                case Mode::Regex:    ok = std::regex_search(x, re_); break;
            }
            out->push(ok ? 1 : 0);
        }
        return out;
    }
private:
    enum class Mode { Contains, Prefix, Suffix, Equal, Regex };
    void compile() {
        const std::string& p = pat_;

        size_t first = p.find('%'), last = p.rfind('%');
        bool body_clean = true;
        for (size_t i = 0; i < p.size(); ++i) {
            char c = p[i];
            if (c == '_' || (c == '%' && i != first && i != last)) { body_clean = false; break; }
        }
        if (body_clean && first != std::string::npos) {
            if (first == 0 && last == p.size() - 1 && first != last) {
                mode_ = Mode::Contains;
                needle_ = p.substr(1, p.size() - 2);
                return;
            }
            if (first == 0 && last == 0) {
                mode_ = Mode::Suffix;
                needle_ = p.substr(1);
                return;
            }
            if (first == p.size() - 1 && last == first) {
                mode_ = Mode::Prefix;
                needle_ = p.substr(0, p.size() - 1);
                return;
            }
        }
        if (p.find('%') == std::string::npos && p.find('_') == std::string::npos) {
            mode_ = Mode::Equal;
            needle_ = p;
            return;
        }

        std::string re;
        re.reserve(p.size() * 2 + 4);
        re += '^';
        for (char c : p) {
            if (c == '%') re += ".*";
            else if (c == '_') re += '.';
            else if (std::string(".^$|()[]{}*+?\\").find(c) != std::string::npos) { re += '\\'; re += c; }
            else re += c;
        }
        re += '$';
        re_ = std::regex(re);
        mode_ = Mode::Regex;
    }
    ExprPtr s_;
    std::string pat_;
    Mode mode_ = Mode::Equal;
    std::string needle_;
    std::regex re_;
};

class LengthExpr : public Expr {
public:
    explicit LengthExpr(ExprPtr s) : s_(std::move(s)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto sv = s_->eval(in);
        auto out = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i)
            out->push(static_cast<int64_t>(as_str(*sv, i).size()));
        return out;
    }
private:
    ExprPtr s_;
};

class ArithExpr : public Expr {
public:
    ArithExpr(ArOp op, ExprPtr l, ExprPtr r) : op_(op), l_(std::move(l)), r_(std::move(r)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto lv = l_->eval(in), rv = r_->eval(in);
        auto out = std::make_shared<Int64Column>();
        size_t n = in.num_rows;
        for (size_t i = 0; i < n; ++i) {
            int64_t x = as_int(*lv, i), y = as_int(*rv, i), z = 0;
            switch (op_) {
                case ArOp::Add: z = x + y; break;
                case ArOp::Sub: z = x - y; break;
                case ArOp::Mul: z = x * y; break;
                case ArOp::Div: z = (y != 0) ? x / y : 0; break;
                case ArOp::Mod: z = (y != 0) ? x % y : 0; break;
            }
            out->push(z);
        }
        return out;
    }
private:
    ArOp op_;
    ExprPtr l_, r_;
};

class InExpr : public Expr {
public:
    InExpr(ExprPtr v, std::vector<int64_t> s) : v_(std::move(v)), set_(std::move(s)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto vv = v_->eval(in);
        auto out = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i) {
            int64_t x = as_int(*vv, i);
            bool ok = false;
            for (int64_t e : set_) if (e == x) { ok = true; break; }
            out->push(ok ? 1 : 0);
        }
        return out;
    }
private:
    ExprPtr v_;
    std::vector<int64_t> set_;
};

class CaseExpr : public Expr {
public:
    CaseExpr(ExprPtr c, ExprPtr a, ExprPtr b)
        : c_(std::move(c)), a_(std::move(a)), b_(std::move(b)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto cv = c_->eval(in);
        auto av = a_->eval(in);
        auto bv = b_->eval(in);

        if (av->type() == DataType::String) {
            auto out = std::make_shared<StringColumn>();
            for (size_t i = 0; i < in.num_rows; ++i)
                out->append_from_string(as_int(*cv, i) ? as_str(*av, i) : as_str(*bv, i));
            return out;
        }
        auto out = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i)
            out->push(as_int(*cv, i) ? as_int(*av, i) : as_int(*bv, i));
        return out;
    }
private:
    ExprPtr c_, a_, b_;
};

class ExtractMin : public Expr {
public:
    explicit ExtractMin(ExprPtr e) : e_(std::move(e)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto ev = e_->eval(in);
        auto out = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i)
            out->push((as_int(*ev, i) / 60) % 60);
        return out;
    }
private:
    ExprPtr e_;
};

class DateTruncMin : public Expr {
public:
    explicit DateTruncMin(ExprPtr e) : e_(std::move(e)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto ev = e_->eval(in);
        auto out = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i) {
            int64_t v = as_int(*ev, i);
            out->push(v - v % 60);
        }
        return out;
    }
private:
    ExprPtr e_;
};

static inline uint64_t expand_bits_32(uint32_t v) {
    uint64_t x = v;
    x = (x | (x << 16)) & 0x0000ffff0000ffffULL;
    x = (x | (x << 8))  & 0x00ff00ff00ff00ffULL;
    x = (x | (x << 4))  & 0x0f0f0f0f0f0f0f0fULL;
    x = (x | (x << 2))  & 0x3333333333333333ULL;
    x = (x | (x << 1))  & 0x5555555555555555ULL;
    return x;
}
class ZOrder2D : public Expr {
public:
    ZOrder2D(ExprPtr a, ExprPtr b) : a_(std::move(a)), b_(std::move(b)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto av = a_->eval(in), bv = b_->eval(in);
        auto out = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i) {
            uint32_t x = static_cast<uint32_t>(as_int(*av, i));
            uint32_t y = static_cast<uint32_t>(as_int(*bv, i));
            uint64_t z = expand_bits_32(x) | (expand_bits_32(y) << 1);
            out->push(static_cast<int64_t>(z));
        }
        return out;
    }
private:
    ExprPtr a_, b_;
};

static std::string json_extract_raw(const std::string& s, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = s.find(needle);
    if (pos == std::string::npos) return "";
    pos = s.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    ++pos;

    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
    if (pos >= s.size()) return "";

    if (s[pos] == '"') {
        size_t end = s.find('"', pos + 1);
        if (end == std::string::npos) return "";
        return s.substr(pos + 1, end - pos - 1);
    }

    size_t end = pos;
    while (end < s.size() && s[end] != ',' && s[end] != '}' && s[end] != ' ') ++end;
    return s.substr(pos, end - pos);
}
class JsonGet : public Expr {
public:
    JsonGet(ExprPtr s, std::string k) : s_(std::move(s)), key_(std::move(k)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto sv = s_->eval(in);
        auto out = std::make_shared<StringColumn>();
        for (size_t i = 0; i < in.num_rows; ++i)
            out->append_from_string(json_extract_raw(as_str(*sv, i), key_));
        return out;
    }
private:
    ExprPtr s_;
    std::string key_;
};
class JsonGetInt : public Expr {
public:
    JsonGetInt(ExprPtr s, std::string k) : s_(std::move(s)), key_(std::move(k)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto sv = s_->eval(in);
        auto out = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i) {
            std::string r = json_extract_raw(as_str(*sv, i), key_);
            int64_t v = 0;
            if (!r.empty()) { try { v = std::stoll(r); } catch (...) {} }
            out->push(v);
        }
        return out;
    }
private:
    ExprPtr s_;
    std::string key_;
};

class ArrayLength : public Expr {
public:
    explicit ArrayLength(ExprPtr s) : s_(std::move(s)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto sv = s_->eval(in);
        auto out = std::make_shared<Int64Column>();
        for (size_t i = 0; i < in.num_rows; ++i) {
            const std::string& s = as_str(*sv, i);

            if (s.size() < 2 || s.front() != '[' || s.back() != ']') {
                out->push(0); continue;
            }
            if (s.size() == 2) { out->push(0); continue; }
            int64_t n = 1;
            for (size_t k = 1; k + 1 < s.size(); ++k) if (s[k] == ',') ++n;
            out->push(n);
        }
        return out;
    }
private:
    ExprPtr s_;
};
class ArrayContainsInt : public Expr {
public:
    ArrayContainsInt(ExprPtr s, int64_t v) : s_(std::move(s)), val_(v) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto sv = s_->eval(in);
        auto out = std::make_shared<Int64Column>();
        std::string vs = std::to_string(val_);
        for (size_t i = 0; i < in.num_rows; ++i) {
            const std::string& s = as_str(*sv, i);
            if (s.size() < 2) { out->push(0); continue; }

            bool found = false;
            for (size_t k = 0; (k = s.find(vs, k)) != std::string::npos; ++k) {
                bool lb = (k > 0 && (s[k - 1] == '[' || s[k - 1] == ','));
                size_t end = k + vs.size();
                bool rb = (end < s.size() && (s[end] == ',' || s[end] == ']'));
                if (lb && rb) { found = true; break; }
            }
            out->push(found ? 1 : 0);
        }
        return out;
    }
private:
    ExprPtr s_;
    int64_t val_;
};

class Haversine : public Expr {
public:
    Haversine(ExprPtr la1, ExprPtr ln1, ExprPtr la2, ExprPtr ln2)
        : la1_(std::move(la1)), ln1_(std::move(ln1)),
          la2_(std::move(la2)), ln2_(std::move(ln2)) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto v1 = la1_->eval(in), v2 = ln1_->eval(in);
        auto v3 = la2_->eval(in), v4 = ln2_->eval(in);
        auto out = std::make_shared<Float64Column>();
        constexpr double R = 6371.0;
        constexpr double D = M_PI / 180.0;
        const auto& a1 = static_cast<const Float64Column&>(*v1).data();
        const auto& b1 = static_cast<const Float64Column&>(*v2).data();
        const auto& a2 = static_cast<const Float64Column&>(*v3).data();
        const auto& b2 = static_cast<const Float64Column&>(*v4).data();
        for (size_t i = 0; i < in.num_rows; ++i) {
            double dlat = (a2[i] - a1[i]) * D;
            double dlng = (b2[i] - b1[i]) * D;
            double s1 = std::sin(dlat / 2);
            double s2 = std::sin(dlng / 2);
            double a = s1*s1 + std::cos(a1[i]*D) * std::cos(a2[i]*D) * s2*s2;
            double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
            out->push(R * c);
        }
        return out;
    }
private:
    ExprPtr la1_, ln1_, la2_, ln2_;
};

static int64_t pow10_i64(int n) {
    int64_t p = 1;
    for (int i = 0; i < n; ++i) p *= 10;
    return p;
}
class ParseDecimal : public Expr {
public:
    ParseDecimal(ExprPtr s, int scale) : s_(std::move(s)), scale_(scale) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto sv = s_->eval(in);
        auto out = std::make_shared<Int64Column>();
        int64_t p = pow10_i64(scale_);
        for (size_t i = 0; i < in.num_rows; ++i) {
            const std::string& s = as_str(*sv, i);

            int sign = 1; size_t pos = 0;
            if (!s.empty() && (s[0] == '-' || s[0] == '+')) { if (s[0]=='-') sign=-1; pos=1; }
            int64_t whole = 0;
            for (; pos < s.size() && s[pos] >= '0' && s[pos] <= '9'; ++pos)
                whole = whole * 10 + (s[pos] - '0');
            int64_t frac = 0, frac_scale = 0;
            if (pos < s.size() && s[pos] == '.') {
                ++pos;
                while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9' && frac_scale < scale_) {
                    frac = frac * 10 + (s[pos] - '0');
                    ++frac_scale; ++pos;
                }
                while (frac_scale < scale_) { frac *= 10; ++frac_scale; }
            }
            out->push(sign * (whole * p + frac));
        }
        return out;
    }
private:
    ExprPtr s_;
    int scale_;
};
class FormatDecimal : public Expr {
public:
    FormatDecimal(ExprPtr v, int scale) : v_(std::move(v)), scale_(scale) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto vv = v_->eval(in);
        auto out = std::make_shared<StringColumn>();
        int64_t p = pow10_i64(scale_);
        for (size_t i = 0; i < in.num_rows; ++i) {
            int64_t x = as_int(*vv, i);
            std::string sign = x < 0 ? "-" : "";
            int64_t a = std::abs(x);
            int64_t w = a / p, f = a % p;
            if (scale_ == 0) {
                out->append_from_string(sign + std::to_string(w));
            } else {
                std::string fs = std::to_string(f);
                while ((int)fs.size() < scale_) fs.insert(fs.begin(), '0');
                out->append_from_string(sign + std::to_string(w) + "." + fs);
            }
        }
        return out;
    }
private:
    ExprPtr v_;
    int scale_;
};
class DecimalMul : public Expr {
public:
    DecimalMul(ExprPtr a, ExprPtr b, int scale)
        : a_(std::move(a)), b_(std::move(b)), scale_(scale) {}
    std::shared_ptr<Column> eval(const Batch& in) const override {
        auto av = a_->eval(in), bv = b_->eval(in);
        auto out = std::make_shared<Int64Column>();
        int64_t p = pow10_i64(scale_);
        for (size_t i = 0; i < in.num_rows; ++i)
            out->push((as_int(*av, i) * as_int(*bv, i)) / p);
        return out;
    }
private:
    ExprPtr a_, b_;
    int scale_;
};

}

ExprPtr col(size_t idx)         { return std::make_shared<ColExpr>(idx); }
ExprPtr lit_i(int64_t v)        { return std::make_shared<IntLit>(v); }
ExprPtr lit_s(std::string s)    { return std::make_shared<StrLit>(std::move(s)); }
ExprPtr cmp(CmpKind k, ExprPtr l, ExprPtr r) { return std::make_shared<CmpExpr>(k, std::move(l), std::move(r)); }
ExprPtr and_(ExprPtr a, ExprPtr b) { return std::make_shared<BoolExpr>(BoolExpr::And, std::move(a), std::move(b)); }
ExprPtr or_ (ExprPtr a, ExprPtr b) { return std::make_shared<BoolExpr>(BoolExpr::Or,  std::move(a), std::move(b)); }
ExprPtr not_(ExprPtr a)            { return std::make_shared<BoolExpr>(BoolExpr::Not, std::move(a), nullptr); }
ExprPtr like(ExprPtr s, std::string p)            { return std::make_shared<LikeExpr>(std::move(s), std::move(p)); }
ExprPtr len_str(ExprPtr s)                        { return std::make_shared<LengthExpr>(std::move(s)); }
ExprPtr arith(ArOp op, ExprPtr l, ExprPtr r)      { return std::make_shared<ArithExpr>(op, std::move(l), std::move(r)); }
ExprPtr in_int(ExprPtr v, std::vector<int64_t> s) { return std::make_shared<InExpr>(std::move(v), std::move(s)); }
ExprPtr case_when(ExprPtr c, ExprPtr a, ExprPtr b){ return std::make_shared<CaseExpr>(std::move(c), std::move(a), std::move(b)); }
ExprPtr extract_minute(ExprPtr e)                 { return std::make_shared<ExtractMin>(std::move(e)); }
ExprPtr date_trunc_minute(ExprPtr e)              { return std::make_shared<DateTruncMin>(std::move(e)); }

ExprPtr z_order_2d(ExprPtr a, ExprPtr b)          { return std::make_shared<ZOrder2D>(std::move(a), std::move(b)); }
ExprPtr json_get(ExprPtr s, std::string k)        { return std::make_shared<JsonGet>(std::move(s), std::move(k)); }
ExprPtr json_get_int(ExprPtr s, std::string k)    { return std::make_shared<JsonGetInt>(std::move(s), std::move(k)); }
ExprPtr array_length(ExprPtr s)                   { return std::make_shared<ArrayLength>(std::move(s)); }
ExprPtr array_contains_int(ExprPtr s, int64_t v)  { return std::make_shared<ArrayContainsInt>(std::move(s), v); }
ExprPtr haversine(ExprPtr la1, ExprPtr ln1, ExprPtr la2, ExprPtr ln2) {
    return std::make_shared<Haversine>(std::move(la1), std::move(ln1), std::move(la2), std::move(ln2));
}
ExprPtr parse_decimal(ExprPtr s, int scale)       { return std::make_shared<ParseDecimal>(std::move(s), scale); }
ExprPtr format_decimal(ExprPtr v, int scale)      { return std::make_shared<FormatDecimal>(std::move(v), scale); }
ExprPtr decimal_mul(ExprPtr a, ExprPtr b, int sc) { return std::make_shared<DecimalMul>(std::move(a), std::move(b), sc); }

}
