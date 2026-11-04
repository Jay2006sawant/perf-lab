#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
# Record a short bench with perf and emit fold stack input (flamegraph optional).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
BIN="${BUILD}/ran_perf_lab"
OUT="${ROOT}/results"
DATA="${OUT}/perf_bench.data"
FOLDED="${OUT}/perf_bench.folded"

if ! command -v perf >/dev/null 2>&1; then
  echo "perf not installed"
  exit 1
fi
if [[ ! -x "${BIN}" ]]; then
  echo "Build ${BIN} first"
  exit 1
fi
mkdir -p "${OUT}"

perf record -F 997 -g -o "${DATA}" -- \
  "${BIN}" bench --iterations 8 --warmup 2 --seed 3 \
  --output "${OUT}/flame_bench.json"

perf script -i "${DATA}" >"${OUT}/perf_bench.script"
echo "Wrote ${OUT}/perf_bench.script"

if command -v stackcollapse-perf.pl >/dev/null 2>&1; then
  stackcollapse-perf.pl "${OUT}/perf_bench.script" >"${FOLDED}"
  echo "Wrote ${FOLDED}"
  if command -v flamegraph.pl >/dev/null 2>&1; then
  flamegraph.pl "${FOLDED}" >"${OUT}/flame.svg"
    echo "Wrote ${OUT}/flame.svg"
  else
    echo "Install flamegraph.pl for SVG output"
  fi
else
  echo "Install FlameGraph stackcollapse-perf.pl to build flame.svg"
  perf report -i "${DATA}" --stdio | head -40
fi
