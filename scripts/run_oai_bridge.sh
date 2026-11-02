#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO="$(cd "${ROOT}/../.." && pwd)"
BUILD="${OAI_BUILD:-${REPO}/build}"

if [[ ! -x "${BUILD}/nr_dlsim" ]]; then
  echo "nr_dlsim not found in ${BUILD}"
  echo "Build OAI from repo root:"
  echo "  mkdir -p build && cd build && cmake .. -GNinja && ninja nr_dlsim nr_ulsim"
  exit 2
fi

FRAMES="${OAI_FRAMES:-200}"
python3 "${ROOT}/python/oai_physim_bridge.py" --oai-build "${BUILD}" --frames "${FRAMES}" \
  --output "${ROOT}/results/oai_physim.json"

python3 "${ROOT}/python/merge_reports.py" \
  --perf-lab "${ROOT}/results/benchmark_results.json" \
  --oai "${ROOT}/results/oai_physim.json" \
  --manifest "${ROOT}/results/platform_manifest.json" \
  --output "${ROOT}/results/combined_report.json"

echo "OAI bridge OK: results/combined_report.json"
