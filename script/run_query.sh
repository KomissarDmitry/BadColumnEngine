#!/usr/bin/env bash
# Выполняет запрос ClickBench с номером $1, читая $COLUMNAR, и печатает
# результат в stdout (грейдер ловит stdout). env.sh даёт: COLUMNAR.
#
# ВНИМАНИЕ: точный способ вызова (номер запроса в $1 vs передача SQL,
# stdout vs запись в $RESULTS) надо сверить с commands.sh грейдера -
# см. README ниже. При необходимости поправить эту строку.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/builddir/columnar"

: "${COLUMNAR:?COLUMNAR is not set}"
QUERY_ID="${1:?usage: run_query.sh <query_id>}"

"$BIN" query "$COLUMNAR" "$QUERY_ID"
