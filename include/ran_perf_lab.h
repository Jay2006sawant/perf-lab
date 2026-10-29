/* SPDX-License-Identifier: CC-BY-4.0 */
#ifndef RAN_PERF_LAB_H
#define RAN_PERF_LAB_H

#include <stddef.h>
#include <stdint.h>

#define RAN_PERF_SCHEMA_BENCH "ran_perf_lab/2"
#define RAN_PERF_SCHEMA_MANIFEST "ran_perf_lab_manifest/2"

typedef struct {
  float re;
  float im;
} cfloat_t;

typedef struct {
  double mean_ns;
  double stdev_ns;
  double min_ns;
  double max_ns;
  uint64_t samples;
} timing_stats_t;

typedef struct {
  uint64_t warmup;
  uint64_t iterations;
  uint32_t seed;
  int subtract_noise_floor;
} bench_config_t;

/* Real dot product */
float dot_real_scalar(const float *a, const float *b, size_t n);
float dot_real_simd(const float *a, const float *b, size_t n);
const char *dot_real_simd_name(void);

/* Complex dot: sum_i a[i] * conj(b[i]) */
cfloat_t dot_cplx_scalar(const cfloat_t *a, const cfloat_t *b, size_t n);
cfloat_t dot_cplx_simd(const cfloat_t *a, const cfloat_t *b, size_t n);
const char *dot_cplx_simd_name(void);

/* Channel energy sum_i |x[i]|^2 */
double energy_scalar(const cfloat_t *x, size_t n);
double energy_simd(const cfloat_t *x, size_t n);

/* Phase rotation: out[i] = in[i] * rot */
void phase_rotate_scalar(cfloat_t *out, const cfloat_t *in, cfloat_t rot, size_t n);
void phase_rotate_simd(cfloat_t *out, const cfloat_t *in, cfloat_t rot, size_t n);
const char *phase_rotate_simd_name(void);

void bench_fill_random(float *a, float *b, cfloat_t *ca, cfloat_t *cb, size_t n, uint32_t seed);

timing_stats_t measure_noise_floor(uint64_t warmup, uint64_t iterations);

int verify_kernels(void);

void time_kernel(void (*fn)(void *), void *ctx, uint64_t warmup, uint64_t iterations, timing_stats_t *out);

int run_benchmark_suite(const char *output_path, const bench_config_t *cfg);
int write_platform_manifest(const char *output_path);

const char *ran_perf_build_flags(void);

#endif /* RAN_PERF_LAB_H */
