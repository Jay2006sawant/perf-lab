#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
mkdir -p "${ROOT}/results/plots"

cmake -S "${ROOT}" -B "${BUILD}" -GNinja
cmake --build "${BUILD}"
ctest --test-dir "${BUILD}" --output-on-failure
python3 "${ROOT}/python/validate_json.py" "${BUILD}/smoke_bench.json"

"${BUILD}/ran_perf_lab" bench --warmup 5 --iterations 25 --seed 99 --noise-floor \
  --output "${ROOT}/results/benchmark_results.json"
python3 "${ROOT}/python/validate_json.py" "${ROOT}/results/benchmark_results.json"
python3 "${ROOT}/python/analyze_results.py" "${ROOT}/results/benchmark_results.json" --outdir "${ROOT}/results/plots"

if command -v perf >/dev/null 2>&1; then
  perf stat -e cycles,instructions,cache-misses "${BUILD}/ran_perf_lab" verify \
    2> "${ROOT}/results/perf_verify.stat" || true
fi

echo "Local CI passed. See ${ROOT}/results/"
