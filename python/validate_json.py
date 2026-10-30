#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""Validate ran_perf_lab JSON output."""

from __future__ import annotations

import json
import sys
from pathlib import Path

REQUIRED_BENCH = {"schema", "timestamp_utc", "config", "benchmarks"}
KERNELS = {"dot_real", "dot_cplx_conj", "channel_energy", "phase_rotate"}


def validate_bench(data: dict) -> list[str]:
    errs: list[str] = []
    if not REQUIRED_BENCH.issubset(data.keys()):
        errs.append(f"missing keys: {REQUIRED_BENCH - data.keys()}")
    if not str(data.get("schema", "")).startswith("ran_perf_lab/"):
        errs.append("bad schema")
    for row in data.get("benchmarks", []):
        if row.get("kernel") not in KERNELS:
            errs.append(f"unknown kernel {row.get('kernel')}")
        for variant in ("scalar", "vectorized"):
            if variant not in row:
                errs.append(f"missing {variant} in {row.get('kernel')}")
            else:
                m = row[variant].get("mean_ns")
                if m is None or m < 0:
                    errs.append(f"bad mean_ns in {row.get('kernel')} {variant}")
    return errs


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: validate_json.py <file.json>", file=sys.stderr)
        return 2
    path = Path(sys.argv[1])
    data = json.loads(path.read_text(encoding="utf-8"))
    errs = validate_bench(data)
    if errs:
        for e in errs:
            print("ERROR:", e)
        return 1
    print(f"OK {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
