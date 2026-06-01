#include "core/schema.hpp"
#include "io/columnar_store.hpp"
#include "query/operators.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>

using namespace columnar;

int main() {

    { std::ofstream f("/tmp/sch_users.csv");
      f << "user_id,int64\n" << "name,string\n"; }
    { std::ofstream f("/tmp/d_users.csv");
      f << "1,alice\n" << "2,bob\n" << "3,eve\n"; }
    ColumnStoreWriter::convert_csv("/tmp/d_users.csv", "/tmp/sch_users.csv",
                                   "/tmp/users.bc");

    { std::ofstream f("/tmp/sch_orders.csv");
      f << "order_id,int64\n" << "user_id,int64\n" << "amount,int64\n"; }
    { std::ofstream f("/tmp/d_orders.csv");
      f << "100,1,50\n"
        << "101,2,30\n"
        << "102,1,20\n"
        << "103,99,77\n"
        << "104,3,5\n";
    }
    ColumnStoreWriter::convert_csv("/tmp/d_orders.csv", "/tmp/sch_orders.csv",
                                   "/tmp/orders.bc");

    StoreReader users("/tmp/users.bc");
    StoreReader orders("/tmp/orders.bc");

    auto u_scan = std::make_unique<ScanOperator>(
        users, std::vector<size_t>{
            users.column_index("user_id"),
            users.column_index("name")});
    auto o_scan = std::make_unique<ScanOperator>(
        orders, std::vector<size_t>{
            orders.column_index("order_id"),
            orders.column_index("user_id"),
            orders.column_index("amount")});
    auto join = std::make_unique<HashJoinOperator>(
        std::move(u_scan), std::move(o_scan),
        0,
        1);

    std::set<std::tuple<int64_t, std::string, int64_t, int64_t>> got;
    while (auto b = join->Next()) {
        for (size_t r = 0; r < b->num_rows; ++r) {
            int64_t uid   = static_cast<const Int64Column&>(*b->columns[0]).data()[r];
            std::string n = static_cast<const StringColumn&>(*b->columns[1]).data()[r];
            int64_t oid   = static_cast<const Int64Column&>(*b->columns[2]).data()[r];
            int64_t amt   = static_cast<const Int64Column&>(*b->columns[4]).data()[r];
            got.emplace(uid, n, oid, amt);
        }
    }

    std::set<std::tuple<int64_t, std::string, int64_t, int64_t>> want = {
        {1, "alice", 100, 50},
        {2, "bob",   101, 30},
        {1, "alice", 102, 20},
        {3, "eve",   104, 5},
    };
    if (got != want) {
        std::cerr << "JOIN mismatch.\nGOT (" << got.size() << "):\n";
        for (auto& [u,n,o,a] : got)
            std::cerr << "  " << u << " " << n << " " << o << " " << a << "\n";
        std::cerr << "WANT (" << want.size() << "):\n";
        for (auto& [u,n,o,a] : want)
            std::cerr << "  " << u << " " << n << " " << o << " " << a << "\n";
        return 1;
    }
    std::cout << "Task #6  (HashJoin)       ok (" << got.size() << " matched rows)\n";
}
