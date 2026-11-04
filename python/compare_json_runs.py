#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""Compare mean_ns between two ran_perf_lab bench JSON files (host A vs B)."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def index_by_kernel(data: dict) -> dict[str, dict]:
    out: dict[str, dict] = {}
    for row in data.get("benchmarks", []):
        k = row.get("kernel")
        if k:
            out[str(k)] = row
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Compare two benchmark JSON runs")
    ap.add_argument("baseline")
    ap.add_argument("candidate")
    ap.add_argument("--variant", choices=("vectorized", "scalar"), default="vectorized")
    args = ap.parse_args()
    a = json.loads(Path(args.baseline).read_text(encoding="utf-8"))
    b = json.loads(Path(args.candidate).read_text(encoding="utf-8"))
    ka = index_by_kernel(a)
    kb = index_by_kernel(b)
    print(f"variant={args.variant}")
    for kernel in sorted(set(ka) & set(kb)):
        ma = ka[kernel][args.variant]["mean_ns"]
        mb = kb[kernel][args.variant]["mean_ns"]
        if ma <= 0:
            continue
        pct = 100.0 * (mb - ma) / ma
        print(f"{kernel}: baseline={ma:.1f} ns candidate={mb:.1f} ns delta={pct:+.1f}%")
    return 0


if __name__ == "__main__":
    sys.exit(main())
