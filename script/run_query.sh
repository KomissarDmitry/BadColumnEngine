#!/usr/bin/env bash
set -e

QUERY_NUM=$1
COLUMNAR=$2
OUTPUT=$3
LOGS=$4

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

mkdir -p "$(dirname "${OUTPUT}")" "$(dirname "${LOGS}")"
"$ROOT/build/columnar" query "${COLUMNAR}" "${QUERY_NUM}" >"${OUTPUT}" 2>"${LOGS}"
