#include "operators.hpp"
#include "row_format.hpp"
#include "aggregation.hpp"
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

namespace columnar {

static bool trace_skip() {
    static bool v = std::getenv("BCE_TRACE_SKIP") != nullptr;
    return v;
}

std::optional<Batch> ScanOperator::Next() {
    while (rg_ < reader_.num_row_groups()) {
        size_t cur = rg_++;

        if (skip_ && reader_.has_stats(cur, skip_->col_index)) {
            auto [mn, mx] = reader_.int_stats(cur, skip_->col_index);
            if (!range_can_match(skip_->op, skip_->value, mn, mx)) {
                if (trace_skip())
                    std::cerr << "[skip] row group " << cur
                              << " (min=" << mn << " max=" << mx << ")\n";
                continue;
            }
        }

        Batch b;
        b.num_rows = reader_.rows_in_group(cur);
        for (size_t c : col_ids_)
            b.columns.push_back(reader_.read_chunk(cur, c));
        return b;
    }
    return std::nullopt;
}

std::optional<Batch> FilterOperator::Next() {
    while (auto in = child_->Next()) {
        auto* col = dynamic_cast<Int64Column*>(in->columns[pred_.col_index].get());
        if (!col) throw std::runtime_error("Filter expects an Int64 column");

        const auto& d = col->data();
        std::vector<uint32_t> sel;
        for (size_t i = 0; i < in->num_rows; ++i)
            if (apply_cmp(d[i], pred_.op, pred_.value))
                sel.push_back(static_cast<uint32_t>(i));

        if (sel.empty()) continue;

        Batch out;
        out.num_rows = sel.size();
        for (auto& c : in->columns)
            out.columns.push_back(c->gather(sel));
        return out;
    }
    return std::nullopt;
}

std::optional<Batch> StringFilterOperator::Next() {
    while (auto in = child_->Next()) {
        auto* col = dynamic_cast<StringColumn*>(in->columns[pred_.col_index].get());
        if (!col) throw std::runtime_error("StringFilter expects a String column");

        const auto& d = col->data();
        std::vector<uint32_t> sel;
        for (size_t i = 0; i < in->num_rows; ++i)
            if (apply_str(d[i], pred_.op, pred_.value))
                sel.push_back(static_cast<uint32_t>(i));

        if (sel.empty()) continue;

        Batch out;
        out.num_rows = sel.size();
        for (auto& c : in->columns)
            out.columns.push_back(c->gather(sel));
        return out;
    }
    return std::nullopt;
}

namespace {
struct Acc {
    uint64_t count = 0;
    int64_t  sum_i = 0;
    int64_t  min_i = std::numeric_limits<int64_t>::max();
    int64_t  max_i = std::numeric_limits<int64_t>::min();
};
}

std::optional<Batch> GlobalAggregateOperator::Next() {
    if (done_) return std::nullopt;
    done_ = true;

    std::vector<Acc> acc(specs_.size());

    while (auto in = child_->Next()) {
        for (size_t s = 0; s < specs_.size(); ++s) {
            const AggSpec& spec = specs_[s];
            Acc& a = acc[s];

            if (spec.kind == AggKind::CountStar) {
                a.count += in->num_rows;
                continue;
            }

            auto* col = dynamic_cast<Int64Column*>(in->columns[spec.col_index].get());
            if (!col) throw std::runtime_error("Aggregate expects an Int64 column");
            if (col->data().empty()) continue;

            a.count += Count(*col);
            a.sum_i += Sum(*col);
            a.min_i = std::min(a.min_i, Min(*col));
            a.max_i = std::max(a.max_i, Max(*col));
        }
    }

    Batch out;
    out.num_rows = 1;
    for (size_t s = 0; s < specs_.size(); ++s) {
        const AggSpec& spec = specs_[s];
        const Acc& a = acc[s];
        switch (spec.kind) {
            case AggKind::CountStar:
            case AggKind::Count: {
                auto c = std::make_shared<Int64Column>();
                c->push(static_cast<int64_t>(a.count));
                out.columns.push_back(c);
                break;
            }
            case AggKind::Sum: {
                auto c = std::make_shared<Int64Column>();
                c->push(a.sum_i);
                out.columns.push_back(c);
                break;
            }
            case AggKind::Avg: {
                auto c = std::make_shared<Float64Column>();
                c->push(a.count ? static_cast<double>(a.sum_i) / a.count : 0.0);
                out.columns.push_back(c);
                break;
            }
            case AggKind::Min: {
                auto c = std::make_shared<Int64Column>();
                c->push(a.count ? a.min_i : 0);
                out.columns.push_back(c);
                break;
            }
            case AggKind::Max: {
                auto c = std::make_shared<Int64Column>();
                c->push(a.count ? a.max_i : 0);
                out.columns.push_back(c);
                break;
            }
            case AggKind::CountDistinct:
                throw std::runtime_error("CountDistinct: use HashAggregateOperator");
        }
    }
    return out;
}

}

namespace columnar {

void RowEncoder::encode_cell(std::string& key, const Column& c, size_t row) {
    switch (c.type()) {
        case DataType::Int64: {
            int64_t v = static_cast<const Int64Column&>(c).data()[row];
            key.append(reinterpret_cast<const char*>(&v), sizeof(v));
            break;
        }
        case DataType::Float64: {
            double v = static_cast<const Float64Column&>(c).data()[row];
            key.append(reinterpret_cast<const char*>(&v), sizeof(v));
            break;
        }
        case DataType::String: {
            const std::string& s = static_cast<const StringColumn&>(c).data()[row];
            uint32_t len = static_cast<uint32_t>(s.size());
            key.append(reinterpret_cast<const char*>(&len), sizeof(len));
            key.append(s);
            break;
        }
    }
}

void RowEncoder::encode_row(std::string& key, const Batch& batch,
                            const std::vector<size_t>& group_cols, size_t row) {
    for (size_t gc : group_cols) encode_cell(key, *batch.columns[gc], row);
}

void append_cell(Column& dst, const Column& src, size_t row) {
    switch (src.type()) {
        case DataType::Int64:
            static_cast<Int64Column&>(dst).push(static_cast<const Int64Column&>(src).data()[row]);
            break;
        case DataType::Float64:
            static_cast<Float64Column&>(dst).push(static_cast<const Float64Column&>(src).data()[row]);
            break;
        case DataType::String:
            dst.append_from_string(src.get_string(row));
            break;
    }
}

int compare_cell(const Column& a, size_t ra, const Column& b, size_t rb) {
    switch (a.type()) {
        case DataType::Int64: {
            int64_t x = static_cast<const Int64Column&>(a).data()[ra];
            int64_t y = static_cast<const Int64Column&>(b).data()[rb];
            return (x < y) ? -1 : (x > y ? 1 : 0);
        }
        case DataType::Float64: {
            double x = static_cast<const Float64Column&>(a).data()[ra];
            double y = static_cast<const Float64Column&>(b).data()[rb];
            return (x < y) ? -1 : (x > y ? 1 : 0);
        }
        case DataType::String: {
            const std::string& x = static_cast<const StringColumn&>(a).data()[ra];
            const std::string& y = static_cast<const StringColumn&>(b).data()[rb];
            int c = x.compare(y);
            return (c < 0) ? -1 : (c > 0 ? 1 : 0);
        }
    }
    return 0;
}

namespace {

struct AggAcc {
    uint64_t count = 0;
    int64_t  sum_i = 0;
    int64_t  min_i = std::numeric_limits<int64_t>::max();
    int64_t  max_i = std::numeric_limits<int64_t>::min();
    double   sum_f = 0.0;
    double   min_f = std::numeric_limits<double>::infinity();
    double   max_f = -std::numeric_limits<double>::infinity();
    std::string min_s, max_s;
    bool has_str = false;
    DataType src_type = DataType::Int64;
};

struct GroupState {
    std::vector<AggAcc> accs;
    std::vector<std::unordered_set<std::string>> distinct;
};

}

std::optional<Batch> HashAggregateOperator::Next() {
    if (done_) return std::nullopt;
    done_ = true;

    std::unordered_map<std::string, size_t> index;
    std::vector<GroupState> states;

    std::vector<std::unique_ptr<Column>> group_out;
    bool inited = false;

    size_t n_distinct = 0;
    for (auto& s : specs_) if (s.kind == AggKind::CountDistinct) ++n_distinct;

    std::string key;
    while (auto in = child_->Next()) {
        if (!inited) {
            for (size_t gc : group_cols_)
                group_out.push_back(make_column(in->columns[gc]->type()));
            inited = true;
        }

        for (size_t r = 0; r < in->num_rows; ++r) {
            key.clear();
            RowEncoder::encode_row(key, *in, group_cols_, r);

            auto it = index.find(key);
            size_t gi;
            if (it == index.end()) {
                gi = states.size();
                index.emplace(key, gi);
                GroupState gs;
                gs.accs.resize(specs_.size());
                gs.distinct.resize(n_distinct);
                states.push_back(std::move(gs));

                for (size_t k = 0; k < group_cols_.size(); ++k)
                    append_cell(*group_out[k], *in->columns[group_cols_[k]], r);
            } else {
                gi = it->second;
            }

            GroupState& gs = states[gi];
            size_t d = 0;
            for (size_t si = 0; si < specs_.size(); ++si) {
                const AggSpec& spec = specs_[si];
                AggAcc& a = gs.accs[si];
                if (spec.kind == AggKind::CountStar) {
                    a.count += 1;
                    continue;
                }
                if (spec.kind == AggKind::CountDistinct) {
                    gs.distinct[d++].insert(in->columns[spec.col_index]->get_string(r));
                    continue;
                }
                Column& col = *in->columns[spec.col_index];
                a.src_type = col.type();
                a.count += 1;
                if (col.type() == DataType::String) {
                    const std::string& s = static_cast<const StringColumn&>(col).data()[r];
                    if (a.has_str) {
                        if (s < a.min_s) a.min_s = s;
                        if (s > a.max_s) a.max_s = s;
                    } else {
                        a.min_s = a.max_s = s;
                        a.has_str = true;
                    }
                } else if (col.type() == DataType::Float64) {
                    double v = static_cast<const Float64Column&>(col).data()[r];
                    a.sum_f += v;
                    a.min_f = std::min(a.min_f, v);
                    a.max_f = std::max(a.max_f, v);
                } else {
                    int64_t v = static_cast<const Int64Column&>(col).data()[r];
                    a.sum_i += v;
                    a.min_i = std::min(a.min_i, v);
                    a.max_i = std::max(a.max_i, v);
                }
            }
        }
    }

    if (states.empty() && group_cols_.empty()) {
        GroupState gs;
        gs.accs.resize(specs_.size());
        gs.distinct.resize(n_distinct);
        states.push_back(std::move(gs));
    }

    Batch out;
    out.num_rows = states.size();
    for (auto& gc : group_out) out.columns.push_back(std::shared_ptr<Column>(gc.release()));

    for (size_t s = 0; s < specs_.size(); ++s) {
        const AggSpec& spec = specs_[s];
        size_t d = 0;
        for (size_t i = 0; i < s; ++i)
            if (specs_[i].kind == AggKind::CountDistinct) ++d;

        if (spec.kind == AggKind::Avg) {
            auto c = std::make_shared<Float64Column>();
            for (auto& gs : states) {
                const AggAcc& a = gs.accs[s];
                if (!a.count) { c->push(0.0); continue; }
                double sum = (a.src_type == DataType::Float64) ? a.sum_f
                                                                : static_cast<double>(a.sum_i);
                c->push(sum / a.count);
            }
            out.columns.push_back(c);
        } else if ((spec.kind == AggKind::Min || spec.kind == AggKind::Max)
                   && !states.empty() && states[0].accs[s].src_type == DataType::String) {
            auto c = std::make_shared<StringColumn>();
            for (auto& gs : states) {
                const AggAcc& a = gs.accs[s];
                c->append_from_string(a.has_str ? (spec.kind == AggKind::Min ? a.min_s : a.max_s) : "");
            }
            out.columns.push_back(c);
        } else {
            auto c = std::make_shared<Int64Column>();
            for (auto& gs : states) {
                const AggAcc& a = gs.accs[s];
                switch (spec.kind) {
                    case AggKind::CountStar:
                    case AggKind::Count:    c->push(static_cast<int64_t>(a.count)); break;
                    case AggKind::Sum:      c->push(a.sum_i); break;
                    case AggKind::Min:      c->push(a.count ? a.min_i : 0); break;
                    case AggKind::Max:      c->push(a.count ? a.max_i : 0); break;
                    case AggKind::CountDistinct:
                        c->push(static_cast<int64_t>(gs.distinct[d].size())); break;
                    case AggKind::Avg: break;
                }
            }
            out.columns.push_back(c);
        }
    }
    return out;
}

std::optional<Batch> SortOperator::Next() {
    if (done_) return std::nullopt;
    done_ = true;

    std::vector<Batch> batches;
    while (auto in = child_->Next())
        batches.push_back(std::move(*in));

    std::vector<std::pair<size_t, size_t>> refs;
    for (size_t b = 0; b < batches.size(); ++b)
        for (size_t r = 0; r < batches[b].num_rows; ++r)
            refs.emplace_back(b, r);

    auto cmp = [&](const std::pair<size_t, size_t>& A,
                   const std::pair<size_t, size_t>& B) {
        for (const SortKey& k : keys_) {
            const Column& ca = *batches[A.first].columns[k.col_index];
            const Column& cb = *batches[B.first].columns[k.col_index];
            int c = compare_cell(ca, A.second, cb, B.second);
            if (c != 0) return k.ascending ? (c < 0) : (c > 0);
        }
        return false;
    };

    size_t total = refs.size();
    size_t start = std::min(offset_, total);
    size_t end = total;
    if (limit_ > 0 && start + limit_ < end) end = start + limit_;

    if (end < total) {
        std::partial_sort(refs.begin(), refs.begin() + end, refs.end(), cmp);
    } else {
        std::sort(refs.begin(), refs.end(), cmp);
    }

    size_t out_rows = end - start;

    Batch out;
    out.num_rows = out_rows;
    if (!batches.empty()) {
        size_t ncols = batches[0].columns.size();
        std::vector<std::unique_ptr<Column>> cols;
        for (size_t c = 0; c < ncols; ++c)
            cols.push_back(make_column(batches[0].columns[c]->type()));
        for (size_t i = start; i < end; ++i) {
            auto [b, r] = refs[i];
            for (size_t c = 0; c < ncols; ++c)
                append_cell(*cols[c], *batches[b].columns[c], r);
        }
        for (auto& c : cols) out.columns.push_back(std::shared_ptr<Column>(c.release()));
    }
    return out;
}

std::optional<Batch> ExprFilterOperator::Next() {
    while (auto in = child_->Next()) {
        auto mask = pred_->eval(*in);
        const auto& m = static_cast<const Int64Column&>(*mask).data();
        std::vector<uint32_t> sel;
        sel.reserve(in->num_rows);
        for (size_t i = 0; i < in->num_rows; ++i)
            if (m[i]) sel.push_back(static_cast<uint32_t>(i));
        if (sel.empty()) continue;

        Batch out;
        out.num_rows = sel.size();
        for (auto& c : in->columns) out.columns.push_back(c->gather(sel));
        return out;
    }
    return std::nullopt;
}

std::optional<Batch> ProjectOperator::Next() {
    auto in = child_->Next();
    if (!in) return std::nullopt;
    Batch out;
    out.num_rows = in->num_rows;
    for (auto& e : exprs_) out.columns.push_back(e->eval(*in));
    return out;
}

void HashJoinOperator::ensure_build() {
    if (built_) return;
    built_ = true;
    uint32_t bi = 0;
    while (auto b = build_->Next()) {
        const auto& key_col = static_cast<const Int64Column&>(*b->columns[build_key_]);
        for (size_t r = 0; r < b->num_rows; ++r)
            table_.emplace(key_col.data()[r],
                           std::make_pair(bi, static_cast<uint32_t>(r)));
        build_batches_.push_back(std::move(*b));
        ++bi;
    }
}

std::optional<Batch> HashJoinOperator::Next() {
    ensure_build();
    if (build_batches_.empty()) return std::nullopt;

    while (auto p = probe_->Next()) {
        const auto& pkey = static_cast<const Int64Column&>(*p->columns[probe_key_]);

        size_t nbuild = build_batches_[0].columns.size();
        size_t nprobe = p->columns.size();
        Batch out;
        std::vector<std::unique_ptr<Column>> cols;
        cols.reserve(nbuild + nprobe);
        for (size_t c = 0; c < nbuild; ++c)
            cols.push_back(make_column(build_batches_[0].columns[c]->type()));
        for (size_t c = 0; c < nprobe; ++c)
            cols.push_back(make_column(p->columns[c]->type()));

        size_t out_rows = 0;
        for (size_t r = 0; r < p->num_rows; ++r) {
            int64_t k = pkey.data()[r];
            auto range = table_.equal_range(k);
            for (auto it = range.first; it != range.second; ++it) {
                uint32_t bi = it->second.first;
                uint32_t br = it->second.second;
                const Batch& bb = build_batches_[bi];
                for (size_t c = 0; c < nbuild; ++c)
                    append_cell(*cols[c], *bb.columns[c], br);
                for (size_t c = 0; c < nprobe; ++c)
                    append_cell(*cols[nbuild + c], *p->columns[c], r);
                ++out_rows;
            }
        }
        if (out_rows == 0) continue;
        out.num_rows = out_rows;
        for (auto& c : cols) out.columns.push_back(std::shared_ptr<Column>(c.release()));
        return out;
    }
    return std::nullopt;
}

}
