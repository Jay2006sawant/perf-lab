#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-4.0
# Show a few AVX2 instructions from the SIMD dot product translation unit.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OBJ="${ROOT}/build/CMakeFiles/ran_perf_lab.dir/src/dotproduct.c.o"
OUT="${ROOT}/results/simd_asm_snippet.txt"

if [[ ! -f "${OBJ}" ]]; then
  echo "Build first (ninja in build/)"
  exit 1
fi
mkdir -p "${ROOT}/results"
{
  echo "# dotproduct.c.o SIMD ops (grep vfmadd / vmul)"
  objdump -d "${OBJ}" | grep -E 'vfmadd|vmulpd|vhadd' | head -24
} >"${OUT}"
echo "Wrote ${OUT}"
