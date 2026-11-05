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

if [[ -r /proc/sys/kernel/perf_event_paranoid ]] && [[ "$(cat /proc/sys/kernel/perf_event_paranoid)" -ge 2 ]]; then
  echo "perf_event_paranoid=$(cat /proc/sys/kernel/perf_event_paranoid) limits profiling."
  echo "For local dev only: sudo sysctl -w kernel.perf_event_paranoid=-1"
  echo "Or use perf stat via ci_local.sh / run_with_perf.sh when paranoid is 0 or 1."
  exit 0
fi

if ! perf record -F 997 -g -o "${DATA}" -- \
  "${BIN}" bench --iterations 8 --warmup 2 --seed 3 \
  --output "${OUT}/flame_bench.json"; then
  echo "perf record failed (permissions or security policy)"
  exit 0
fi

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
