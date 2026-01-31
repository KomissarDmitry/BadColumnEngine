#include "io/columnar_file.hpp"
#include <iostream>
#include <string>

void print_usage(const char* text) {
    std::cout << "Usage:\n"
              << "  " << text << " to-columnar <data.csv> <schema.csv> <output.col>\n"
              << "  " << text << " to-csv <input.col> <data.csv> <schema.csv>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    std::string command = argv[1];
    try {
        if (command == "to-columnar" && argc == 5) {
            columnar::ColumnarFile::csv_to_columnar(argv[2], argv[3], argv[4]);
        }
        else if (command == "to-csv" && argc == 5) {
            columnar::ColumnarFile::columnar_to_csv(argv[2], argv[3], argv[4]);
        }
        else {
            print_usage(argv[0]);
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
