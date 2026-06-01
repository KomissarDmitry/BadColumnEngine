#include "query/expr.hpp"
#include "query/batch.hpp"
#include "core/column.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

using namespace columnar;

static std::shared_ptr<Column> mki(std::initializer_list<int64_t> xs) {
    auto c = std::make_shared<Int64Column>();
    for (auto x : xs) c->push(x);
    return c;
}
static std::shared_ptr<Column> mkf(std::initializer_list<double> xs) {
    auto c = std::make_shared<Float64Column>();
    for (auto x : xs) c->push(x);
    return c;
}
static std::shared_ptr<Column> mks(std::initializer_list<const char*> xs) {
    auto c = std::make_shared<StringColumn>();
    for (auto x : xs) c->append_from_string(x);
    return c;
}

static int64_t row_i(const Column& c, size_t r) {
    return static_cast<const Int64Column&>(c).data()[r];
}
static double row_f(const Column& c, size_t r) {
    return static_cast<const Float64Column&>(c).data()[r];
}
static const std::string& row_s(const Column& c, size_t r) {
    return static_cast<const StringColumn&>(c).data()[r];
}

int main() {

    {
        Batch b; b.num_rows = 4;
        b.columns.push_back(mki({0, 1, 0, 1}));
        b.columns.push_back(mki({0, 0, 1, 1}));
        auto r = z_order_2d(col(0), col(1))->eval(b);
        assert(row_i(*r, 0) == 0);
        assert(row_i(*r, 1) == 1);
        assert(row_i(*r, 2) == 2);
        assert(row_i(*r, 3) == 3);
        std::cout << "Task #5  (z_order)        ok\n";
    }

    {
        Batch b; b.num_rows = 3;
        b.columns.push_back(mks({
            "{\"name\":\"alice\",\"age\":30,\"city\":\"NY\"}",
            "{\"name\":\"bob\",\"age\":25}",
            "{\"name\":\"eve\",\"age\":99,\"city\":\"LA\"}",
        }));
        auto names = json_get(col(0), "name")->eval(b);
        auto ages  = json_get_int(col(0), "age")->eval(b);
        auto city  = json_get(col(0), "city")->eval(b);
        assert(row_s(*names, 0) == "alice");
        assert(row_s(*names, 1) == "bob");
        assert(row_s(*names, 2) == "eve");
        assert(row_i(*ages, 0) == 30);
        assert(row_i(*ages, 1) == 25);
        assert(row_i(*ages, 2) == 99);
        assert(row_s(*city, 0) == "NY");
        assert(row_s(*city, 1) == "");
        assert(row_s(*city, 2) == "LA");
        std::cout << "Task #10 (json_get)       ok\n";
    }

    {
        Batch b; b.num_rows = 4;
        b.columns.push_back(mks({"[1,2,3]", "[42]", "[]", "[1,2,3,42,5]"}));
        auto lens   = array_length(col(0))->eval(b);
        auto has42  = array_contains_int(col(0), 42)->eval(b);
        assert(row_i(*lens, 0) == 3);
        assert(row_i(*lens, 1) == 1);
        assert(row_i(*lens, 2) == 0);
        assert(row_i(*lens, 3) == 5);
        assert(row_i(*has42, 0) == 0);
        assert(row_i(*has42, 1) == 1);
        assert(row_i(*has42, 2) == 0);
        assert(row_i(*has42, 3) == 1);
        std::cout << "Task #11 (arrays)         ok\n";
    }

    {
        Batch b; b.num_rows = 1;
        b.columns.push_back(mkf({55.7558}));
        b.columns.push_back(mkf({37.6173}));
        b.columns.push_back(mkf({59.9343}));
        b.columns.push_back(mkf({30.3351}));
        auto d = haversine(col(0), col(1), col(2), col(3))->eval(b);
        double km = row_f(*d, 0);
        assert(std::abs(km - 633.0) < 5.0);
        std::cout << "Task #12 (haversine)      ok (" << (int)km << " km)\n";
    }

    {
        Batch b; b.num_rows = 4;
        b.columns.push_back(mks({"1.23", "10", "-0.5", "0.099"}));
        auto v = parse_decimal(col(0), 2)->eval(b);
        assert(row_i(*v, 0) == 123);
        assert(row_i(*v, 1) == 1000);
        assert(row_i(*v, 2) == -50);
        assert(row_i(*v, 3) == 9);
        auto formatted = format_decimal(col(0) , 2)->eval(Batch{{v}, 4});
        assert(row_s(*formatted, 0) == "1.23");
        assert(row_s(*formatted, 1) == "10.00");
        assert(row_s(*formatted, 2) == "-0.50");
        assert(row_s(*formatted, 3) == "0.09");

        Batch m; m.num_rows = 1;
        m.columns.push_back(mki({123}));
        m.columns.push_back(mki({1000}));
        auto product = decimal_mul(col(0), col(1), 2)->eval(m);
        assert(row_i(*product, 0) == 1230);
        std::cout << "Task #13 (decimal)        ok\n";
    }

    std::cout << "\nAll optional-task expression smoke tests passed.\n";
}
