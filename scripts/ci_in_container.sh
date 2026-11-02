#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
set -euo pipefail
cd /ran_perf_lab
ITERATIONS="${ITERATIONS:-20}"
WARMUP="${WARMUP:-5}"
mkdir -p results/plots
./build/ran_perf_lab verify
./build/ran_perf_lab manifest --output results/platform_manifest.json
./build/ran_perf_lab bench --warmup "$WARMUP" --iterations "$ITERATIONS" --seed 42 --noise-floor \
  --output results/benchmark_results.json
python3 python/validate_json.py results/benchmark_results.json
python3 python/analyze_results.py results/benchmark_results.json --outdir results/plots
echo "Container CI OK"
