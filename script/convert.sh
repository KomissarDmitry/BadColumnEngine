#!/usr/bin/env bash
set -e

INPUT_CSV=$1
COLUMNAR=$2

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

mkdir -p "$(dirname "${COLUMNAR}")"
"$ROOT/build/columnar" convert "${INPUT_CSV}" "$ROOT/script/hits_schema.csv" "${COLUMNAR}"
