# Wireless context for the kernels

This lab uses simplified **float** buffers. Production NR PHY in OpenAirInterface often uses fixed-point IQ samples (`c16_t`) and slot-based scheduling. The mapping below is conceptual, not a bit-exact implementation.

## Kernel to layer intuition

| Lab kernel | Typical PHY building block | NR spec touchpoint (high level) |
|------------|---------------------------|--------------------------------|
| `dot_real` | Matched filter / correlation accumulator | Channel estimation combine steps (TS 38.211 signal concepts) |
| `dot_cplx_conj` | Multiply reference with conjugate channel | Same family as estimation math |
| `channel_energy` | Power measurement on a symbol or RB group | Used in link adaptation and measurement reports |
| `phase_rotate` | CFO or phase correction before demap | Receiver front-end correction |

## Relation to OpenAirInterface

- Dot products and SIMD live under `openair1/PHY/TOOLS` and related TX/RX procedures.
- Unitary timing without RF is reported by `nr_dlsim` / `nr_ulsim` with `-P` (macro `time_meas` blocks).

Use the optional `oai_physim_bridge.py` to place OAI microseconds next to this lab JSON in one merged report.
