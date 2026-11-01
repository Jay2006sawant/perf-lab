#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
# Optional: record perf stat for one dot_real iteration (requires perf, may need sysctl permissions).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
RESULTS="${ROOT}/results"
BIN="${BUILD}/ran_perf_lab"

if ! command -v perf >/dev/null 2>&1; then
  echo "perf not installed; skipping"
  exit 0
fi
if [[ ! -x "${BIN}" ]]; then
  echo "Build ${BIN} first (scripts/run_all.sh)"
  exit 1
fi

mkdir -p "${RESULTS}"
# Smoke: verify is short; perf on full bench would be noisy — document methodology in README.
perf stat -e cycles,instructions,cache-misses,task-clock \
  -o "${RESULTS}/perf_verify.stat" \
  "${BIN}" verify

echo "Wrote ${RESULTS}/perf_verify.stat"
