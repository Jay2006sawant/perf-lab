/* SPDX-License-Identifier: CC-BY-4.0 */
#include "timing.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint64_t timing_now_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

void timing_compute_stats(const double *samples_ns, uint64_t n, timing_stats_t *out)
{
  memset(out, 0, sizeof(*out));
  if (n == 0) {
    return;
  }
  out->samples = n;
  double sum = 0.0;
  for (uint64_t i = 0; i < n; i++) {
    sum += samples_ns[i];
  }
  out->mean_ns = sum / (double)n;
  if (n > 1) {
    double var = 0.0;
    for (uint64_t i = 0; i < n; i++) {
      double d = samples_ns[i] - out->mean_ns;
      var += d * d;
    }
    out->stdev_ns = sqrt(var / (double)(n - 1));
  }
  out->min_ns = samples_ns[0];
  out->max_ns = samples_ns[0];
  for (uint64_t i = 1; i < n; i++) {
    if (samples_ns[i] < out->min_ns) {
      out->min_ns = samples_ns[i];
    }
    if (samples_ns[i] > out->max_ns) {
      out->max_ns = samples_ns[i];
    }
  }
}

void time_kernel(void (*fn)(void *), void *ctx, uint64_t warmup, uint64_t iterations, timing_stats_t *out)
{
  for (uint64_t w = 0; w < warmup; w++) {
    fn(ctx);
  }
  double *samples = calloc(iterations, sizeof(double));
  if (!samples) {
    memset(out, 0, sizeof(*out));
    return;
  }
  for (uint64_t i = 0; i < iterations; i++) {
    uint64_t t0 = timing_now_ns();
    fn(ctx);
    uint64_t t1 = timing_now_ns();
    samples[i] = (double)(t1 - t0);
  }
  timing_compute_stats(samples, iterations, out);
  free(samples);
}
