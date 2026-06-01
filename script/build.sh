#!/usr/bin/env bash
# Сборка проекта. На выходе - бинарь builddir/columnar.
# Пытаемся meson; если его нет - падаем на прямой g++.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

export PATH="$HOME/.local/bin:$PATH"

if command -v meson >/dev/null 2>&1 && command -v ninja >/dev/null 2>&1; then
    if [ ! -d builddir ]; then
        meson setup builddir --buildtype=release
    fi
    meson compile -C builddir
else
    echo "meson/ninja not available; falling back to direct g++" >&2
    mkdir -p builddir
    g++ -std=c++20 -O2 -Wall -Wextra \
        src/core/schema.cpp src/core/column.cpp \
        src/io/csv.cpp src/io/columnar_store.cpp \
        src/query/operators.cpp src/query/queries.cpp src/query/expr.cpp \
        src/query/aggregation.cpp \
        src/storage/row_group.cpp \
        src/compression/codec.cpp src/compression/compression.cpp \
        src/main.cpp \
        -o builddir/columnar
fi

echo "build.sh: built $ROOT/builddir/columnar"
