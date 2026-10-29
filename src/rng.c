/* SPDX-License-Identifier: CC-BY-4.0 */
#include "ran_perf_lab.h"

#include <math.h>

/* xorshift32 — deterministic bench inputs */
static uint32_t rng_state;

static uint32_t rng_next(void)
{
  uint32_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rng_state = x;
  return x;
}

static float rng_uniform(void)
{
  return (float)(rng_next() & 0xffff) / 65535.f * 2.f - 1.f;
}

void bench_fill_random(float *a, float *b, cfloat_t *ca, cfloat_t *cb, size_t n, uint32_t seed)
{
  rng_state = seed ? seed : 0xC0FFEEu;
  for (size_t i = 0; i < n; i++) {
    a[i] = rng_uniform();
    b[i] = rng_uniform();
    ca[i].re = a[i];
    ca[i].im = b[i];
    cb[i].re = b[i];
    cb[i].im = a[i];
  }
}
