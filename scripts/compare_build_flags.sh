#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
# Compare portable (-O2) vs native (-march=native) builds.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${ROOT}/results/compare_flags"
mkdir -p "${OUT}"

cmake -S "${ROOT}" -B "${ROOT}/build-O2" -GNinja -DRAN_PERF_LAB_NATIVE=OFF
cmake --build "${ROOT}/build-O2"
"${ROOT}/build-O2/ran_perf_lab" bench --iterations 15 --warmup 3 --seed 7 \
  --output "${OUT}/bench_O2.json"

cmake -S "${ROOT}" -B "${ROOT}/build-native" -GNinja -DRAN_PERF_LAB_NATIVE=ON
cmake --build "${ROOT}/build-native"
"${ROOT}/build-native/ran_perf_lab" bench --iterations 15 --warmup 3 --seed 7 \
  --output "${OUT}/bench_native.json"

python3 "${ROOT}/python/validate_json.py" "${OUT}/bench_O2.json"
python3 "${ROOT}/python/validate_json.py" "${OUT}/bench_native.json"
echo "Wrote ${OUT}/bench_O2.json and bench_native.json"
