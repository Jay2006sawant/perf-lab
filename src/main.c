/* SPDX-License-Identifier: CC-BY-4.0 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ran_perf_lab.h"

static int nearly_equal_f(float a, float b, float rel)
{
  float d = fabsf(a - b);
  return d <= rel * fabsf(a) + 1e-5f;
}

static int nearly_equal_c(cfloat_t a, cfloat_t b, float rel)
{
  return nearly_equal_f(a.re, b.re, rel) && nearly_equal_f(a.im, b.im, rel);
}

int verify_kernels(void)
{
  const size_t n = 10007;
  float *a = calloc(n, sizeof(float));
  float *b = calloc(n, sizeof(float));
  cfloat_t *ca = calloc(n, sizeof(cfloat_t));
  cfloat_t *cb = calloc(n, sizeof(cfloat_t));
  cfloat_t *out_s = calloc(n, sizeof(cfloat_t));
  cfloat_t *out_v = calloc(n, sizeof(cfloat_t));
  if (!a || !b || !ca || !cb || !out_s || !out_v) {
    return 1;
  }
  bench_fill_random(a, b, ca, cb, n, 42);

  float rs = dot_real_scalar(a, b, n);
  float rv = dot_real_simd(a, b, n);
  if (!nearly_equal_f(rs, rv, 1e-4f)) {
    fprintf(stderr, "FAIL dot_real: scalar=%f simd=%f\n", rs, rv);
    return 1;
  }

  cfloat_t cs = dot_cplx_scalar(ca, cb, n);
  cfloat_t cv = dot_cplx_simd(ca, cb, n);
  if (!nearly_equal_c(cs, cv, 1e-4f)) {
    fprintf(stderr, "FAIL dot_cplx: scalar=(%f,%f) simd=(%f,%f)\n", cs.re, cs.im, cv.re, cv.im);
    return 1;
  }

  double es = energy_scalar(ca, n);
  double ev = energy_simd(ca, n);
  if (fabs(es - ev) > 1e-2 * fabs(es) + 1e-5) {
    fprintf(stderr, "FAIL energy: scalar=%f simd=%f\n", es, ev);
    return 1;
  }

  cfloat_t rot = {0.7071067f, 0.7071067f};
  phase_rotate_scalar(out_s, ca, rot, n);
  phase_rotate_simd(out_v, ca, rot, n);
  for (size_t i = 0; i < n; i++) {
    if (!nearly_equal_c(out_s[i], out_v[i], 1e-4f)) {
      fprintf(stderr, "FAIL phase_rotate at %zu\n", i);
      return 1;
    }
  }

  printf("OK verify_kernels (n=%zu, dot=%s, cplx=%s, rot=%s)\n",
         n,
         dot_real_simd_name(),
         dot_cplx_simd_name(),
         phase_rotate_simd_name());
  free(a);
  free(b);
  free(ca);
  free(cb);
  free(out_s);
  free(out_v);
  return 0;
}

static void usage(const char *prog)
{
  fprintf(stderr,
          "Usage:\n"
          "  %s verify\n"
          "  %s bench [--warmup N] [--iterations N] [--seed N] [--noise-floor] [--output path.json]\n"
          "  %s manifest [--output path.json]\n",
          prog,
          prog,
          prog);
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    usage(argv[0]);
    return 2;
  }

  if (strcmp(argv[1], "verify") == 0) {
    return verify_kernels() == 0 ? 0 : 1;
  }

  if (strcmp(argv[1], "manifest") == 0) {
    const char *out = "platform_manifest.json";
    for (int i = 2; i < argc; i++) {
      if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
        out = argv[++i];
      }
    }
    return write_platform_manifest(out) == 0 ? 0 : 1;
  }

  if (strcmp(argv[1], "bench") == 0) {
    bench_config_t cfg = {.warmup = 10, .iterations = 50, .seed = 42, .subtract_noise_floor = 0};
    const char *out = "benchmark_results.json";
    for (int i = 2; i < argc; i++) {
      if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
        cfg.warmup = (uint64_t)strtoull(argv[++i], NULL, 10);
      } else if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
        cfg.iterations = (uint64_t)strtoull(argv[++i], NULL, 10);
      } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
        cfg.seed = (uint32_t)strtoul(argv[++i], NULL, 10);
      } else if (strcmp(argv[i], "--noise-floor") == 0) {
        cfg.subtract_noise_floor = 1;
      } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
        out = argv[++i];
      }
    }
    if (verify_kernels() != 0) {
      fprintf(stderr, "Refusing to bench: verify failed\n");
      return 1;
    }
    return run_benchmark_suite(out, &cfg) == 0 ? 0 : 1;
  }

  usage(argv[0]);
  return 2;
}
