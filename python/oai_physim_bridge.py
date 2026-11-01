#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""
Run OpenAirInterface NR physims (nr_dlsim / nr_ulsim) and export JSON aligned with ran_perf_lab.

Requires OAI built from repo root:
  mkdir -p build && cd build && cmake .. -GNinja && ninja nr_dlsim nr_ulsim
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


import os


def find_oai_build_dir(explicit: str | None) -> Path | None:
    if explicit:
        p = Path(explicit)
        return p if p.is_dir() else None
    candidates: list[Path] = []
    if os.environ.get("OAI_BUILD"):
        candidates.append(Path(os.environ["OAI_BUILD"]))
    repo = Path(__file__).resolve().parents[3]
    candidates += [repo / "build", repo / "cmake_targets" / "ran_build" / "build"]
    for c in candidates:
        if not c.is_dir():
            continue
        for name in ("nr_dlsim", "nr_ulsim"):
            if (c / name).is_file():
                return c
    return None


def parse_physim_output(text: str) -> list[dict]:
    rows: list[dict] = []
    dist_re = re.compile(
        r"^(.+?)\s+(\d+\.\d+) us \((\d+) trials\)\s*$",
        re.MULTILINE,
    )
    stats_re = re.compile(
        r"Statistics std=([\d.]+), min=([\d.]+), q1=([\d.]+), median=([\d.]+), q3=([\d.]+), max=([\d.]+)",
    )
    for m in dist_re.finditer(text):
        name = m.group(1).strip()
        mean_us = float(m.group(2))
        trials = int(m.group(3))
        tail = text[m.end() : m.end() + 200]
        stats = stats_re.search(tail)
        row = {
            "name": name,
            "mean_us": mean_us,
            "mean_ns": mean_us * 1000.0,
            "trials": trials,
        }
        if stats:
            row.update(
                {
                    "stdev_us": float(stats.group(1)),
                    "min_us": float(stats.group(2)),
                    "median_us": float(stats.group(4)),
                    "max_us": float(stats.group(6)),
                }
            )
        rows.append(row)
    return rows


def run_sim(build: Path, binary: str, args: list[str], timeout_s: int) -> str:
    exe = build / binary
    if not exe.is_file():
        raise FileNotFoundError(f"missing {exe}")
    cmd = [str(exe)] + args
    proc = subprocess.run(
        cmd,
        cwd=str(build),
        capture_output=True,
        text=True,
        timeout=timeout_s,
    )
    out = proc.stdout + "\n" + proc.stderr
    if proc.returncode != 0:
        raise RuntimeError(f"{binary} failed ({proc.returncode}):\n{out[-4000:]}")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="OAI physim → JSON bridge")
    ap.add_argument("--oai-build", type=str, default=None, help="Path to OAI build dir")
    ap.add_argument("--output", type=Path, default=Path("results/oai_physim.json"))
    ap.add_argument("--frames", type=int, default=200, help="-n frames (use 1000 for KPI doc parity)")
    ap.add_argument("--timeout", type=int, default=600)
    ap.add_argument("--skip-ul", action="store_true")
    args = ap.parse_args()

    build = find_oai_build_dir(args.oai_build)
    if build is None:
        print(
            "OAI build not found. From repo root:\n"
            "  mkdir -p build && cd build && cmake .. -GNinja && ninja nr_dlsim nr_ulsim\n"
            "Then: OAI_BUILD=$PWD python3 LFX/perf-lab/python/oai_physim_bridge.py",
            file=sys.stderr,
        )
        return 2

    n = str(args.frames)
    dl_args = ["-n" + n, "-s20", "-S20.2", "-e15", "-b106", "-R106", "-P"]
    ul_args = ["-n" + n, "-s20", "-S20", "-m15", "-r106", "-R106", "-C8", "-P"]

    results: dict = {
        "schema": "ran_perf_lab_oai_physim/1",
        "timestamp_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "oai_build_dir": str(build),
        "runs": [],
    }

    print(f"Using OAI build: {build}", file=sys.stderr)
    dl_out = run_sim(build, "nr_dlsim", dl_args, args.timeout)
    results["runs"].append(
        {
            "binary": "nr_dlsim",
            "command": " ".join(["nr_dlsim"] + dl_args),
            "metrics": parse_physim_output(dl_out),
        }
    )

    if not args.skip_ul:
        ul_out = run_sim(build, "nr_ulsim", ul_args, args.timeout)
        results["runs"].append(
            {
                "binary": "nr_ulsim",
                "command": " ".join(["nr_ulsim"] + ul_args),
                "metrics": parse_physim_output(ul_out),
            }
        )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {args.output} ({sum(len(r['metrics']) for r in results['runs'])} metrics)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
