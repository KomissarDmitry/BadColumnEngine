#include "io/columnar_store.hpp"
#include "query/queries.hpp"
#include "query/operators.hpp"
#include "query/logical.hpp"
#include <iostream>
#include <string>

void print_usage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " convert <data.csv> <schema.csv> <out.bc>\n"
              << "  " << prog << " query   <store.bc> <query_id>\n"
              << "  " << prog << " explain <store.bc> <query_id>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) { print_usage(argv[0]); return 1; }
    std::string cmd = argv[1];

    try {
        if (cmd == "convert" && argc == 5) {
            columnar::ColumnStoreWriter::convert_csv(argv[2], argv[3], argv[4]);
        } else if (cmd == "query" && argc == 4) {
            columnar::StoreReader reader(argv[2]);
            int id = std::stoi(argv[3]);
            columnar::run_query(id, reader, std::cout);
        } else if (cmd == "explain" && argc == 4) {
            columnar::StoreReader reader(argv[2]);
            int id = std::stoi(argv[3]);
            auto plan = columnar::build_query(id, reader);
            columnar::explain_plan(*plan, std::cout);
        } else {
            print_usage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
