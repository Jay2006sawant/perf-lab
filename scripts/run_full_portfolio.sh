#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
# Microbench CI + optional OAI physim + flag comparison (if time).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"${ROOT}/scripts/ci_local.sh"

if [[ -x "${OAI_BUILD:-${ROOT}/../../build}/nr_dlsim" ]]; then
  "${ROOT}/scripts/run_oai_bridge.sh"
else
  echo "Skip OAI bridge (build nr_dlsim first; see scripts/run_oai_bridge.sh)"
fi

if [[ "${RUN_FLAG_COMPARE:-0}" == "1" ]]; then
  "${ROOT}/scripts/compare_build_flags.sh"
fi

echo "Portfolio pipeline done."
