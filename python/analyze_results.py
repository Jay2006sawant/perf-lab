#!/usr/bin/env python3
# SPDX-License-Identifier: CC-BY-4.0
"""Parse ran_perf_lab JSON v2 and emit plots + HTML report."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

try:
    import matplotlib.pyplot as plt
except ImportError:
    plt = None


def load(path: Path) -> dict:
    with path.open(encoding="utf-8") as f:
        return json.load(f)


def rows_by_kernel(data: dict, kernel: str) -> list[dict]:
    return [b for b in data.get("benchmarks", []) if b.get("kernel") == kernel]


def write_summary(data: dict, outdir: Path) -> None:
    lines = [
        "# RAN perf lab — run summary",
        "",
        f"- Schema: `{data.get('schema')}`",
        f"- UTC: `{data.get('timestamp_utc')}`",
        f"- Build: `{data.get('build_flags', 'n/a')}`",
        f"- Seed: `{data.get('config', {}).get('seed')}`",
        f"- Noise floor (mean ns): `{data.get('noise_floor_ns', {}).get('mean_ns')}`",
        "",
    ]
    for kernel in ("dot_real", "dot_cplx_conj", "channel_energy", "phase_rotate"):
        lines.append(f"## {kernel}")
        lines.append("")
        lines.append("| N | scalar ns | vectorized ns | speedup |")
        lines.append("|--:|----------:|--------------:|--------:|")
        for row in rows_by_kernel(data, kernel):
            n = row["vector_len"]
            s = row["scalar"]["mean_ns"]
            v = row["vectorized"]["mean_ns"]
            sp = row.get("speedup_scalar_over_vectorized", s / v if v else 0)
            lines.append(f"| {n} | {s:.1f} | {v:.1f} | {sp:.2f}x |")
        lines.append("")
    (outdir / "SUMMARY.md").write_text("\n".join(lines), encoding="utf-8")


def plot_kernel(data: dict, kernel: str, outdir: Path, title: str) -> None:
    if plt is None:
        return
    rows = rows_by_kernel(data, kernel)
    if not rows:
        return
    lens = [r["vector_len"] for r in rows]
    scalar = [r["scalar"]["mean_ns"] for r in rows]
    vec = [r["vectorized"]["mean_ns"] for r in rows]
    fig, ax = plt.subplots(figsize=(8, 4))
    ax.plot(lens, scalar, "o-", label="scalar")
    ax.plot(lens, vec, "s-", label="vectorized")
    ax.set_xscale("log", base=2)
    ax.set_xlabel("vector length")
    ax.set_ylabel("mean ns / call")
    ax.set_title(title)
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(outdir / f"{kernel}_latency.png", dpi=150)
    plt.close(fig)


def write_html(data: dict, outdir: Path) -> None:
    summary = (outdir / "SUMMARY.md").read_text(encoding="utf-8")
    imgs = [p.name for p in sorted(outdir.glob("*.png"))]
    body = ["<html><head><meta charset='utf-8'><title>RAN perf lab</title></head><body>"]
    body.append("<h1>RAN perf lab report</h1><pre>")
    body.append(summary.replace("<", "&lt;"))
    body.append("</pre>")
    for img in imgs:
        body.append(f"<h2>{img}</h2><img src='{img}' width='800'/>")
    body.append("</body></html>")
    (outdir / "report.html").write_text("\n".join(body), encoding="utf-8")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("json_file", type=Path)
    ap.add_argument("--outdir", type=Path, default=Path("results/plots"))
    args = ap.parse_args()
    data = load(args.json_file)
    args.outdir.mkdir(parents=True, exist_ok=True)
    write_summary(data, args.outdir)
    for k, title in (
        ("dot_real", "Real dot product"),
        ("dot_cplx_conj", "Complex dot (conj)"),
        ("channel_energy", "Channel energy |x|^2"),
        ("phase_rotate", "Phase rotation"),
    ):
        plot_kernel(data, k, args.outdir, title)
    write_html(data, args.outdir)
    print(f"Wrote {args.outdir}/SUMMARY.md and report.html")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
