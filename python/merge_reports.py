#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""Merge ran_perf_lab JSON + optional OAI physim JSON into one portfolio report."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--perf-lab", type=Path, required=True)
    ap.add_argument("--oai", type=Path, default=None)
    ap.add_argument("--manifest", type=Path, default=None)
    ap.add_argument("--output", type=Path, default=Path("results/combined_report.json"))
    args = ap.parse_args()

    combined = {
        "schema": "ran_perf_lab_combined/1",
        "microbenchmarks": json.loads(args.perf_lab.read_text(encoding="utf-8")),
    }
    if args.manifest and args.manifest.is_file():
        combined["platform"] = json.loads(args.manifest.read_text(encoding="utf-8"))
    if args.oai and args.oai.is_file():
        combined["oai_physim"] = json.loads(args.oai.read_text(encoding="utf-8"))

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(combined, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
