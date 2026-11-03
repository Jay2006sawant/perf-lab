# Measurement methodology

## Goals

Results must be **defensible to a reviewer**: environment is recorded, inputs are deterministic, and variance is reported.

## Procedure

1. **Verify correctness** — `ran_perf_lab verify` compares scalar vs vectorized kernels (relative tolerance 1e-4).
2. **Platform manifest** — `manifest` writes CPU model, memory, kernel, compiler, SIMD backend names.
3. **Warmup** — default 10 iterations (not timed) to stabilize caches and frequency.
4. **Timed iterations** — default 50; report mean, sample stdev, min, max (nanoseconds per kernel call).
5. **Noise floor** — optional `--noise-floor` subtracts empty-loop timing from reported means.
6. **Deterministic inputs** — xorshift32 seed via `--seed` (default 42).
7. **JSON export** — schema `ran_perf_lab/2` for automation and CI validation.

## Compiler experiments

Run `scripts/compare_build_flags.sh` to produce `-O2` portable vs `-march=native` JSON under `results/compare_flags/`.

## Linux tuning (manual, document in report)

Record before benchmarking:

- `cpupower frequency-info`
- `cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor`
- Whether isolcpus are used (`cat /proc/cmdline`)

Aligns with Duranta OAI `doc/tuning_and_security.md` and internship **noise floor** deliverable.

## perf

`scripts/ci_local.sh` captures `perf stat` on `verify` when `perf` is installed. For flamegraphs, run:

```bash
perf record -g -- build/ran_perf_lab bench --iterations 10 --output /tmp/b.json
perf script | stackcollapse-perf.pl | flamegraph.pl > results/flame.svg
```

(Requires FlameGraph tools on the host.)

## Docker / Kubernetes

Reproducible environment:

```bash
docker compose build && docker compose run --rm perf-lab
kubectl apply -f k8s/job.yaml   # after tagging image ran-perf-lab:latest
```
