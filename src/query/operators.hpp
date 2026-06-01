#pragma once

#include "batch.hpp"
#include "expression.hpp"
#include "expr.hpp"
#include "../io/columnar_store.hpp"
#include <optional>
#include <unordered_map>
#include <vector>

namespace columnar {

class ScanOperator : public IOperator {
public:
    ScanOperator(StoreReader& reader,
                 std::vector<size_t> col_ids,
                 std::optional<Int64Predicate> skip = std::nullopt)
        : reader_(reader), col_ids_(std::move(col_ids)), skip_(skip) {}

    std::optional<Batch> Next() override;
    std::string name() const override { return "Scan"; }

private:
    StoreReader& reader_;
    std::vector<size_t> col_ids_;
    std::optional<Int64Predicate> skip_;
    size_t rg_ = 0;
};

class FilterOperator : public IOperator {
public:
    FilterOperator(std::unique_ptr<IOperator> child, Int64Predicate pred)
        : child_(std::move(child)), pred_(pred) {}
    std::string name() const override { return "Filter"; }
    std::vector<const IOperator*> children() const override { return {child_.get()}; }

    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> child_;
    Int64Predicate pred_;
};

class StringFilterOperator : public IOperator {
public:
    StringFilterOperator(std::unique_ptr<IOperator> child, StringPredicate pred)
        : child_(std::move(child)), pred_(std::move(pred)) {}
    std::string name() const override { return "StringFilter"; }
    std::vector<const IOperator*> children() const override { return {child_.get()}; }

    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> child_;
    StringPredicate pred_;
};

class ExprFilterOperator : public IOperator {
public:
    ExprFilterOperator(std::unique_ptr<IOperator> child, ExprPtr predicate)
        : child_(std::move(child)), pred_(std::move(predicate)) {}
    std::string name() const override { return "Filter(expr)"; }
    std::vector<const IOperator*> children() const override { return {child_.get()}; }

    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> child_;
    ExprPtr pred_;
};

class ProjectOperator : public IOperator {
public:
    ProjectOperator(std::unique_ptr<IOperator> child, std::vector<ExprPtr> exprs)
        : child_(std::move(child)), exprs_(std::move(exprs)) {}
    std::string name() const override { return "Project"; }
    std::vector<const IOperator*> children() const override { return {child_.get()}; }

    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> child_;
    std::vector<ExprPtr> exprs_;
};

enum class AggKind { CountStar, Count, Sum, Avg, Min, Max, CountDistinct };

struct AggSpec {
    AggKind kind;
    size_t col_index;
};

class GlobalAggregateOperator : public IOperator {
public:
    GlobalAggregateOperator(std::unique_ptr<IOperator> child, std::vector<AggSpec> specs)
        : child_(std::move(child)), specs_(std::move(specs)) {}
    std::string name() const override { return "GlobalAggregate"; }
    std::vector<const IOperator*> children() const override { return {child_.get()}; }

    std::optional<Batch> Next() override;

private:
    std::unique_ptr<IOperator> child_;
    std::vector<AggSpec> specs_;
    bool done_ = false;
};

class HashAggregateOperator : public IOperator {
public:
    HashAggregateOperator(std::unique_ptr<IOperator> child,
                          std::vector<size_t> group_cols,
                          std::vector<AggSpec> specs)
        : child_(std::move(child)),
          group_cols_(std::move(group_cols)),
          specs_(std::move(specs)) {}

    std::optional<Batch> Next() override;
    std::string name() const override { return "HashAggregate"; }
    std::vector<const IOperator*> children() const override { return {child_.get()}; }

private:
    std::unique_ptr<IOperator> child_;
    std::vector<size_t> group_cols_;
    std::vector<AggSpec> specs_;
    bool done_ = false;
};

struct SortKey {
    size_t col_index;
    bool ascending;
};

class SortOperator : public IOperator {
public:
    SortOperator(std::unique_ptr<IOperator> child,
                 std::vector<SortKey> keys,
                 size_t limit = 0,
                 size_t offset = 0)
        : child_(std::move(child)), keys_(std::move(keys)),
          limit_(limit), offset_(offset) {}

    std::optional<Batch> Next() override;
    std::string name() const override { return limit_ ? "Sort(TopK)" : "Sort"; }
    std::vector<const IOperator*> children() const override { return {child_.get()}; }

private:
    std::unique_ptr<IOperator> child_;
    std::vector<SortKey> keys_;
    size_t limit_;
    size_t offset_;
    bool done_ = false;
};

class HashJoinOperator : public IOperator {
public:
    HashJoinOperator(std::unique_ptr<IOperator> build,
                     std::unique_ptr<IOperator> probe,
                     size_t build_key_col,
                     size_t probe_key_col)
        : build_(std::move(build)), probe_(std::move(probe)),
          build_key_(build_key_col), probe_key_(probe_key_col) {}

    std::optional<Batch> Next() override;
    std::string name() const override { return "HashJoin"; }
    std::vector<const IOperator*> children() const override {
        return {build_.get(), probe_.get()};
    }

private:
    void ensure_build();

    std::unique_ptr<IOperator> build_;
    std::unique_ptr<IOperator> probe_;
    size_t build_key_;
    size_t probe_key_;
    bool   built_ = false;

    std::unordered_multimap<int64_t, std::pair<uint32_t, uint32_t>> table_;
    std::vector<Batch> build_batches_;
};

}
