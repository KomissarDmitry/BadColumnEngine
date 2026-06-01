#pragma once

#include "../core/column.hpp"
#include <cstdint>

namespace columnar {

int64_t Sum(const Int64Column& c);
double  Sum(const Float64Column& c);

int64_t Min(const Int64Column& c);
int64_t Max(const Int64Column& c);
double  Min(const Float64Column& c);
double  Max(const Float64Column& c);

uint64_t Count(const Column& c);

double Avg(const Int64Column& c);
double Avg(const Float64Column& c);

uint64_t CountDistinct(const Column& c);

}
