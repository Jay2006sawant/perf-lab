# Architecture

```
perf-lab/
├── src/
│   ├── main.c           CLI: verify | bench | manifest
│   ├── timing.c         CLOCK_MONOTONIC_RAW, stats
│   ├── dotproduct.c     float dot (scalar + AVX2/NEON)
│   ├── cplx_dot.c       complex conj dot + channel energy
│   ├── phaserot.c       CFO-style phase rotation
│   ├── rng.c            deterministic inputs
│   ├── bench.c          JSON suite + noise floor
│   └── manifest.c       /proc platform capture
├── python/
│   ├── validate_json.py CI schema checks
│   └── analyze_results.py plots + report.html
├── scripts/             ci_local, compare flags, perf wrapper
├── Dockerfile           Ubuntu 22.04 reproducible run
├── k8s/job.yaml         optional batch Job
└── docs/                methodology + architecture
```

## Kernel mapping to RAN PHY

| Kernel | RAN interpretation |
|--------|---------------------|
| `dot_real` | Correlation / matched filter building block |
| `dot_cplx_conj` | Channel estimation combine (conj multiply-accumulate) |
| `channel_energy` | RSSI / power measurement |
| `phase_rotate` | CFO or phase compensation on a symbol |

Production analog in OAI: `openair1/PHY/TOOLS` (`dot_product` on `c16_t` samples).

## JSON schema v2

- `config`: warmup, iterations, seed, noise-floor flag
- `noise_floor_ns`: empty-loop timing
- `backends`: SIMD implementation names per kernel family
- `benchmarks[]`: rows per (kernel, vector_len) with scalar vs vectorized
