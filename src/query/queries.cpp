#include "queries.hpp"
#include <stdexcept>

namespace columnar {

static std::unique_ptr<IOperator> scan(StoreReader& r,
                                       std::vector<std::string> names,
                                       std::optional<Int64Predicate> skip = std::nullopt) {
    std::vector<size_t> ids;
    for (auto& n : names) ids.push_back(r.column_index(n));
    return std::make_unique<ScanOperator>(r, std::move(ids), skip);
}

constexpr int64_t D_2013_07_01 = 15887;
constexpr int64_t D_2013_07_31 = 15917;
constexpr int64_t D_2013_07_14 = 15900;
constexpr int64_t D_2013_07_15 = 15901;

std::unique_ptr<IOperator> build_query(int id, StoreReader& r) {
    switch (id) {

    case 0: {
        auto s = scan(r, {});
        return std::make_unique<GlobalAggregateOperator>(
            std::move(s), std::vector<AggSpec>{{AggKind::CountStar, 0}});
    }

    case 1: {
        size_t adv = r.column_index("AdvEngineID");
        Int64Predicate skip{adv, CompareOp::Ne, 0};
        auto s = scan(r, {"AdvEngineID"}, skip);
        auto f = std::make_unique<FilterOperator>(
            std::move(s), Int64Predicate{0, CompareOp::Ne, 0});
        return std::make_unique<GlobalAggregateOperator>(
            std::move(f), std::vector<AggSpec>{{AggKind::CountStar, 0}});
    }

    case 2: {
        auto s = scan(r, {"AdvEngineID", "ResolutionWidth"});
        return std::make_unique<GlobalAggregateOperator>(
            std::move(s), std::vector<AggSpec>{
                {AggKind::Sum, 0}, {AggKind::CountStar, 0}, {AggKind::Avg, 1}});
    }

    case 3: {
        auto s = scan(r, {"UserID"});
        return std::make_unique<GlobalAggregateOperator>(
            std::move(s), std::vector<AggSpec>{{AggKind::Avg, 0}});
    }

    case 4: {
        auto s = scan(r, {"UserID"});
        return std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{},
            std::vector<AggSpec>{{AggKind::CountDistinct, 0}});
    }

    case 5: {
        auto s = scan(r, {"SearchPhrase"});
        return std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{},
            std::vector<AggSpec>{{AggKind::CountDistinct, 0}});
    }

    case 6: {
        auto s = scan(r, {"EventDate"});
        return std::make_unique<GlobalAggregateOperator>(
            std::move(s), std::vector<AggSpec>{{AggKind::Min, 0}, {AggKind::Max, 0}});
    }

    case 7: {
        size_t adv = r.column_index("AdvEngineID");
        Int64Predicate skip{adv, CompareOp::Ne, 0};
        auto s = scan(r, {"AdvEngineID"}, skip);
        auto f = std::make_unique<FilterOperator>(
            std::move(s), Int64Predicate{0, CompareOp::Ne, 0});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}});
    }

    case 8: {
        auto s = scan(r, {"RegionID", "UserID"});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountDistinct, 1}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10);
    }

    case 9: {
        auto s = scan(r, {"RegionID", "AdvEngineID", "ResolutionWidth", "UserID"});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{0},
            std::vector<AggSpec>{
                {AggKind::Sum, 1}, {AggKind::CountStar, 0},
                {AggKind::Avg, 2}, {AggKind::CountDistinct, 3}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 10: {
        auto s = scan(r, {"MobilePhoneModel", "UserID"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{0, StrOp::NotEmpty, ""});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountDistinct, 1}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10);
    }

    case 11: {
        auto s = scan(r, {"MobilePhone", "MobilePhoneModel", "UserID"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{1, StrOp::NotEmpty, ""});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{{AggKind::CountDistinct, 2}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 12: {
        auto s = scan(r, {"SearchPhrase"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{0, StrOp::NotEmpty, ""});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10);
    }

    case 13: {
        auto s = scan(r, {"SearchPhrase", "UserID"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{0, StrOp::NotEmpty, ""});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountDistinct, 1}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10);
    }

    case 14: {
        auto s = scan(r, {"SearchEngineID", "SearchPhrase"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{1, StrOp::NotEmpty, ""});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 15: {
        auto s = scan(r, {"UserID"});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10);
    }

    case 16: {
        auto s = scan(r, {"UserID", "SearchPhrase"});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 17: {
        auto s = scan(r, {"UserID", "SearchPhrase"});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 18: {
        auto s = scan(r, {"UserID", "EventTime", "SearchPhrase"});
        auto p = std::make_unique<ProjectOperator>(
            std::move(s), std::vector<ExprPtr>{col(0), extract_minute(col(1)), col(2)});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(p), std::vector<size_t>{0, 1, 2},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{3, false}}, 10);
    }

    case 19: {
        const int64_t target = 435090932899640449LL;
        size_t uid = r.column_index("UserID");
        Int64Predicate skip{uid, CompareOp::Eq, target};
        auto s = scan(r, {"UserID"}, skip);
        return std::make_unique<FilterOperator>(
            std::move(s), Int64Predicate{0, CompareOp::Eq, target});
    }

    case 20: {
        auto s = scan(r, {"URL"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s), like(col(0), "%google%"));
        return std::make_unique<GlobalAggregateOperator>(
            std::move(f), std::vector<AggSpec>{{AggKind::CountStar, 0}});
    }

    case 21: {
        auto s = scan(r, {"SearchPhrase", "URL"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(like(col(1), "%google%"),
                 cmp(CmpKind::Ne, col(0), lit_s(""))));
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::Min, 1}, {AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 22: {
        auto s = scan(r, {"SearchPhrase", "URL", "Title", "UserID"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(and_(like(col(2), "%Google%"),
                      not_(like(col(1), "%.google.%"))),
                 cmp(CmpKind::Ne, col(0), lit_s(""))));
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{
                {AggKind::Min, 1}, {AggKind::Min, 2},
                {AggKind::CountStar, 0}, {AggKind::CountDistinct, 3}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{3, false}}, 10);
    }

    case 23: {
        std::vector<std::string> all_names;
        for (size_t i = 0; i < r.num_columns(); ++i)
            all_names.push_back(r.schema().column(i).name);
        size_t url_idx = 0, et_idx = 0;
        for (size_t i = 0; i < all_names.size(); ++i) {
            if (all_names[i] == "URL")       url_idx = i;
            if (all_names[i] == "EventTime") et_idx = i;
        }
        auto s = scan(r, all_names);
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s), like(col(url_idx), "%google%"));
        return std::make_unique<SortOperator>(
            std::move(f), std::vector<SortKey>{{et_idx, true}}, 10);
    }

    case 24: {
        auto s = scan(r, {"SearchPhrase", "EventTime"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{0, StrOp::NotEmpty, ""});
        auto sorted = std::make_unique<SortOperator>(
            std::move(f), std::vector<SortKey>{{1, true}}, 10);
        return std::make_unique<ProjectOperator>(
            std::move(sorted), std::vector<ExprPtr>{col(0)});
    }

    case 25: {
        auto s = scan(r, {"SearchPhrase"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{0, StrOp::NotEmpty, ""});
        return std::make_unique<SortOperator>(
            std::move(f), std::vector<SortKey>{{0, true}}, 10);
    }

    case 26: {
        auto s = scan(r, {"SearchPhrase", "EventTime"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{0, StrOp::NotEmpty, ""});
        auto sorted = std::make_unique<SortOperator>(
            std::move(f), std::vector<SortKey>{{1, true}, {0, true}}, 10);
        return std::make_unique<ProjectOperator>(
            std::move(sorted), std::vector<ExprPtr>{col(0)});
    }

    case 27: {
        auto s = scan(r, {"CounterID", "URL"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{1, StrOp::NotEmpty, ""});
        auto p = std::make_unique<ProjectOperator>(
            std::move(f), std::vector<ExprPtr>{col(0), len_str(col(1))});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(p), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::Avg, 1}, {AggKind::CountStar, 0}});
        auto having = std::make_unique<ExprFilterOperator>(
            std::move(agg), cmp(CmpKind::Gt, col(2), lit_i(100000)));
        return std::make_unique<SortOperator>(
            std::move(having), std::vector<SortKey>{{1, false}}, 25);
    }

    case 28: {
        auto s = scan(r, {"Referer"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{0, StrOp::NotEmpty, ""});
        auto p = std::make_unique<ProjectOperator>(
            std::move(f), std::vector<ExprPtr>{col(0), len_str(col(0)), col(0)});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(p), std::vector<size_t>{0},
            std::vector<AggSpec>{
                {AggKind::Avg, 1}, {AggKind::CountStar, 0}, {AggKind::Min, 2}});
        auto having = std::make_unique<ExprFilterOperator>(
            std::move(agg), cmp(CmpKind::Gt, col(2), lit_i(100000)));
        return std::make_unique<SortOperator>(
            std::move(having), std::vector<SortKey>{{1, false}}, 25);
    }

    case 29: {
        auto s = scan(r, {"ResolutionWidth"});
        std::vector<ExprPtr> proj;
        for (int i = 0; i < 90; ++i)
            proj.push_back(i == 0 ? col(0) : arith(ArOp::Add, col(0), lit_i(i)));
        auto p = std::make_unique<ProjectOperator>(std::move(s), std::move(proj));
        std::vector<AggSpec> specs;
        for (size_t i = 0; i < 90; ++i) specs.push_back({AggKind::Sum, i});
        return std::make_unique<GlobalAggregateOperator>(std::move(p), std::move(specs));
    }

    case 30: {
        auto s = scan(r, {"SearchEngineID", "ClientIP", "IsRefresh", "ResolutionWidth", "SearchPhrase"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{4, StrOp::NotEmpty, ""});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{
                {AggKind::CountStar, 0}, {AggKind::Sum, 2}, {AggKind::Avg, 3}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 31: {
        auto s = scan(r, {"WatchID", "ClientIP", "IsRefresh", "ResolutionWidth", "SearchPhrase"});
        auto f = std::make_unique<StringFilterOperator>(
            std::move(s), StringPredicate{4, StrOp::NotEmpty, ""});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{
                {AggKind::CountStar, 0}, {AggKind::Sum, 2}, {AggKind::Avg, 3}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 32: {
        auto s = scan(r, {"WatchID", "ClientIP", "IsRefresh", "ResolutionWidth"});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{
                {AggKind::CountStar, 0}, {AggKind::Sum, 2}, {AggKind::Avg, 3}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 33: {
        auto s = scan(r, {"URL"});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(s), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10);
    }

    case 34: {
        auto s = scan(r, {"URL"});
        auto p = std::make_unique<ProjectOperator>(
            std::move(s), std::vector<ExprPtr>{lit_i(1), col(0)});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(p), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10);
    }

    case 35: {
        auto s = scan(r, {"ClientIP"});
        auto p = std::make_unique<ProjectOperator>(
            std::move(s), std::vector<ExprPtr>{
                col(0),
                arith(ArOp::Sub, col(0), lit_i(1)),
                arith(ArOp::Sub, col(0), lit_i(2)),
                arith(ArOp::Sub, col(0), lit_i(3))});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(p), std::vector<size_t>{0, 1, 2, 3},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{4, false}}, 10);
    }

    case 36: {
        auto s = scan(r, {"URL", "CounterID", "EventDate", "DontCountHits", "IsRefresh"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(and_(cmp(CmpKind::Eq, col(1), lit_i(62)),
                      cmp(CmpKind::Ge, col(2), lit_i(D_2013_07_01))),
                 and_(and_(cmp(CmpKind::Le, col(2), lit_i(D_2013_07_31)),
                           cmp(CmpKind::Eq, col(3), lit_i(0))),
                      and_(cmp(CmpKind::Eq, col(4), lit_i(0)),
                           cmp(CmpKind::Ne, col(0), lit_s(""))))));
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10);
    }

    case 37: {
        auto s = scan(r, {"Title", "CounterID", "EventDate", "DontCountHits", "IsRefresh"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(and_(cmp(CmpKind::Eq, col(1), lit_i(62)),
                      cmp(CmpKind::Ge, col(2), lit_i(D_2013_07_01))),
                 and_(and_(cmp(CmpKind::Le, col(2), lit_i(D_2013_07_31)),
                           cmp(CmpKind::Eq, col(3), lit_i(0))),
                      and_(cmp(CmpKind::Eq, col(4), lit_i(0)),
                           cmp(CmpKind::Ne, col(0), lit_s(""))))));
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10);
    }

    case 38: {
        auto s = scan(r, {"URL", "CounterID", "EventDate", "IsRefresh", "IsLink", "IsDownload"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(and_(cmp(CmpKind::Eq, col(1), lit_i(62)),
                      and_(cmp(CmpKind::Ge, col(2), lit_i(D_2013_07_01)),
                           cmp(CmpKind::Le, col(2), lit_i(D_2013_07_31)))),
                 and_(cmp(CmpKind::Eq, col(3), lit_i(0)),
                      and_(cmp(CmpKind::Ne, col(4), lit_i(0)),
                           cmp(CmpKind::Eq, col(5), lit_i(0))))));
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{1, false}}, 10, 1000);
    }

    case 39: {
        auto s = scan(r, {"TraficSourceID", "SearchEngineID", "AdvEngineID", "Referer", "URL",
                          "CounterID", "EventDate", "IsRefresh"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(cmp(CmpKind::Eq, col(5), lit_i(62)),
                 and_(and_(cmp(CmpKind::Ge, col(6), lit_i(D_2013_07_01)),
                           cmp(CmpKind::Le, col(6), lit_i(D_2013_07_31))),
                      cmp(CmpKind::Eq, col(7), lit_i(0)))));
        auto p = std::make_unique<ProjectOperator>(
            std::move(f), std::vector<ExprPtr>{
                col(0), col(1), col(2),
                case_when(and_(cmp(CmpKind::Eq, col(1), lit_i(0)),
                               cmp(CmpKind::Eq, col(2), lit_i(0))),
                          col(3), lit_s("")),
                col(4)});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(p), std::vector<size_t>{0, 1, 2, 3, 4},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{5, false}}, 10, 1000);
    }

    case 40: {
        const int64_t referer_hash = 3594120000172545465LL;
        auto s = scan(r, {"URLHash", "EventDate", "CounterID", "IsRefresh",
                          "TraficSourceID", "RefererHash"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(and_(cmp(CmpKind::Eq, col(2), lit_i(62)),
                      and_(cmp(CmpKind::Ge, col(1), lit_i(D_2013_07_01)),
                           cmp(CmpKind::Le, col(1), lit_i(D_2013_07_31)))),
                 and_(cmp(CmpKind::Eq, col(3), lit_i(0)),
                      and_(in_int(col(4), {-1, 6}),
                           cmp(CmpKind::Eq, col(5), lit_i(referer_hash))))));
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10, 100);
    }

    case 41: {
        const int64_t url_hash = 2868770270353813622LL;
        auto s = scan(r, {"WindowClientWidth", "WindowClientHeight", "CounterID",
                          "EventDate", "IsRefresh", "DontCountHits", "URLHash"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(and_(cmp(CmpKind::Eq, col(2), lit_i(62)),
                      and_(cmp(CmpKind::Ge, col(3), lit_i(D_2013_07_01)),
                           cmp(CmpKind::Le, col(3), lit_i(D_2013_07_31)))),
                 and_(cmp(CmpKind::Eq, col(4), lit_i(0)),
                      and_(cmp(CmpKind::Eq, col(5), lit_i(0)),
                           cmp(CmpKind::Eq, col(6), lit_i(url_hash))))));
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(f), std::vector<size_t>{0, 1},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{2, false}}, 10, 10000);
    }

    case 42: {
        auto s = scan(r, {"EventTime", "CounterID", "EventDate", "IsRefresh", "DontCountHits"});
        auto f = std::make_unique<ExprFilterOperator>(
            std::move(s),
            and_(and_(cmp(CmpKind::Eq, col(1), lit_i(62)),
                      and_(cmp(CmpKind::Ge, col(2), lit_i(D_2013_07_14)),
                           cmp(CmpKind::Le, col(2), lit_i(D_2013_07_15)))),
                 and_(cmp(CmpKind::Eq, col(3), lit_i(0)),
                      cmp(CmpKind::Eq, col(4), lit_i(0)))));
        auto p = std::make_unique<ProjectOperator>(
            std::move(f), std::vector<ExprPtr>{date_trunc_minute(col(0))});
        auto agg = std::make_unique<HashAggregateOperator>(
            std::move(p), std::vector<size_t>{0},
            std::vector<AggSpec>{{AggKind::CountStar, 0}});
        return std::make_unique<SortOperator>(
            std::move(agg), std::vector<SortKey>{{0, true}}, 10, 1000);
    }

    default:
        throw std::runtime_error("Query " + std::to_string(id) +
                                 " not implemented (valid range: 0..42)");
    }
}

void run_query(int id, StoreReader& r, std::ostream& out) {
    auto root = build_query(id, r);
    while (auto b = root->Next()) {
        for (size_t row = 0; row < b->num_rows; ++row) {
            for (size_t c = 0; c < b->columns.size(); ++c) {
                if (c) out << '\t';
                out << b->columns[c]->get_string(row);
            }
            out << '\n';
        }
    }
}

}
