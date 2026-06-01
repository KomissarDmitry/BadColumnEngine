#pragma once

#include "../core/column.hpp"
#include "../core/schema.hpp"
#include <memory>
#include <vector>

namespace columnar {

class ColumnarFile {
public:
    const std::string extension_ = ".bc";

    static void csv_to_columnar(const std::string& data_csv,
                                const std::string& schema_csv,
                                const std::string& outp_ut);

    static void columnar_to_csv(const std::string& input,
                                const std::string& data_csv,
                                const std::string& schema_csv);

private:
    static void write_file(const std::string& path,
                          const Schema& schema,
                          const std::vector<std::unique_ptr<Column>>& columns);

    static void read_file(const std::string& path,
                         Schema& schema,
                         std::vector<std::unique_ptr<Column>>& columns);
};

}
