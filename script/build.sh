#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

mkdir -p build
g++ -std=c++20 -O2 \
    src/core/schema.cpp src/core/column.cpp \
    src/io/csv.cpp src/io/columnar_store.cpp \
    src/query/operators.cpp src/query/queries.cpp src/query/expr.cpp \
    src/query/aggregation.cpp \
    src/storage/row_group.cpp \
    src/compression/codec.cpp src/compression/compression.cpp \
    src/main.cpp \
    -o build/columnar
