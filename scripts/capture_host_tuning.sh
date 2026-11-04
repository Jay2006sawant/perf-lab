#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
# Snapshot Linux host settings relevant to benchmark comparisons (governor, cmdline).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${ROOT}/results/host_tuning.txt"
mkdir -p "${ROOT}/results"

{
  echo "# captured $(date -Is)"
  echo "hostname: $(hostname)"
  if command -v lscpu >/dev/null 2>&1; then
    echo "--- lscpu (summary)"
    lscpu | sed -n '1,12p'
  fi
  if [[ -r /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]]; then
    echo "scaling_governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
  fi
  if command -v cpupower >/dev/null 2>&1; then
    echo "--- cpupower frequency-info"
    cpupower frequency-info 2>/dev/null | sed -n '1,15p' || true
  fi
  echo "cmdline: $(tr -d '\n' </proc/cmdline)"
} >"${OUT}"

echo "Wrote ${OUT}"
