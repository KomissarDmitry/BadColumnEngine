#!/usr/bin/env bash
# Читает $INPUT_CSV и пишет файл в нашем формате в $COLUMNAR.
# env.sh даёт: INPUT_CSV, COLUMNAR.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/builddir/columnar"
SCHEMA="$ROOT/script/hits_schema.csv"

: "${INPUT_CSV:?INPUT_CSV is not set}"
: "${COLUMNAR:?COLUMNAR is not set}"

"$BIN" convert "$INPUT_CSV" "$SCHEMA" "$COLUMNAR"
