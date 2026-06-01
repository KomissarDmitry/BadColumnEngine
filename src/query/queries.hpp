#pragma once

#include "operators.hpp"
#include "../io/columnar_store.hpp"
#include <memory>
#include <string>

namespace columnar {

std::unique_ptr<IOperator> build_query(int id, StoreReader& reader);

void run_query(int id, StoreReader& reader, std::ostream& out);

}
