/* SPDX-License-Identifier: CC-BY-4.0 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ran_perf_lab.h"

static void json_escape(FILE *fp, const char *s)
{
  fputc('"', fp);
  for (const char *p = s; *p; p++) {
    if (*p == '"' || *p == '\\') {
      fputc('\\', fp);
    }
    fputc(*p, fp);
  }
  fputc('"', fp);
}

static void emit_timing_json(FILE *fp, const char *key, const timing_stats_t *st, double extra_mops)
{
  fprintf(fp,
          "\"%s\": {\"mean_ns\": %.3f, \"stdev_ns\": %.3f, \"min_ns\": %.3f, \"max_ns\": %.3f, \"mops\": %.3f}",
          key,
          st->mean_ns,
          st->stdev_ns,
          st->min_ns,
          st->max_ns,
          extra_mops);
}

typedef struct {
  int mode;
  union {
    struct {
      const float *x;
      const float *y;
      size_t len;
      volatile float result;
      int simd;
    } dot;
    struct {
      const cfloat_t *a;
      const cfloat_t *b;
      size_t len;
      volatile cfloat_t result;
      int simd;
    } cplx;
    struct {
      const cfloat_t *x;
      size_t len;
      volatile double result;
      int simd;
    } nrg;
    struct {
      cfloat_t *out;
      const cfloat_t *in;
      cfloat_t r;
      size_t len;
      int simd;
    } rot;
  } u;
} kernel_ctx_t;

enum { K_DOT, K_CPLX, K_ENERGY, K_ROT };

static void kernel_fn(void *p)
{
  kernel_ctx_t *c = (kernel_ctx_t *)p;
  switch (c->mode) {
    case K_DOT:
      c->u.dot.result = c->u.dot.simd ? dot_real_simd(c->u.dot.x, c->u.dot.y, c->u.dot.len)
                                      : dot_real_scalar(c->u.dot.x, c->u.dot.y, c->u.dot.len);
      break;
    case K_CPLX:
      c->u.cplx.result = c->u.cplx.simd ? dot_cplx_simd(c->u.cplx.a, c->u.cplx.b, c->u.cplx.len)
                                        : dot_cplx_scalar(c->u.cplx.a, c->u.cplx.b, c->u.cplx.len);
      break;
    case K_ENERGY:
      c->u.nrg.result = c->u.nrg.simd ? energy_simd(c->u.nrg.x, c->u.nrg.len) : energy_scalar(c->u.nrg.x, c->u.nrg.len);
      break;
    case K_ROT:
      if (c->u.rot.simd) {
        phase_rotate_simd(c->u.rot.out, c->u.rot.in, c->u.rot.r, c->u.rot.len);
      } else {
        phase_rotate_scalar(c->u.rot.out, c->u.rot.in, c->u.rot.r, c->u.rot.len);
      }
      break;
    default:
      break;
  }
}

static void bench_pair(FILE *fp,
                       const char *kernel_name,
                       size_t n,
                       kernel_ctx_t *scalar_ctx,
                       kernel_ctx_t *simd_ctx,
                       const bench_config_t *cfg,
                       double ops_per_call)
{
  timing_stats_t st_s;
  timing_stats_t st_v;
  time_kernel(kernel_fn, scalar_ctx, cfg->warmup, cfg->iterations, &st_s);
  time_kernel(kernel_fn, simd_ctx, cfg->warmup, cfg->iterations, &st_v);

  if (cfg->subtract_noise_floor) {
    timing_stats_t nf = measure_noise_floor(cfg->warmup, cfg->iterations);
    st_s.mean_ns -= nf.mean_ns;
    if (st_s.mean_ns < 0) {
      st_s.mean_ns = 0;
    }
    st_v.mean_ns -= nf.mean_ns;
    if (st_v.mean_ns < 0) {
      st_v.mean_ns = 0;
    }
  }

  double mops_s = st_s.mean_ns > 0 ? ops_per_call / st_s.mean_ns : 0;
  double mops_v = st_v.mean_ns > 0 ? ops_per_call / st_v.mean_ns : 0;
  double speedup = st_v.mean_ns > 0 ? st_s.mean_ns / st_v.mean_ns : 0;

  fprintf(fp, "    {\n");
  fprintf(fp, "      \"kernel\": ");
  json_escape(fp, kernel_name);
  fprintf(fp, ",\n      \"vector_len\": %zu,\n", n);
  fprintf(fp, "      ");
  emit_timing_json(fp, "scalar", &st_s, mops_s);
  fprintf(fp, ",\n      ");
  emit_timing_json(fp, "vectorized", &st_v, mops_v);
  fprintf(fp, ",\n      \"speedup_scalar_over_vectorized\": %.4f\n", speedup);
  fprintf(fp, "    }\n");
}

static void noop_kernel(void *p)
{
  (void)p;
}

timing_stats_t measure_noise_floor(uint64_t warmup, uint64_t iterations)
{
  timing_stats_t out;
  memset(&out, 0, sizeof(out));
  time_kernel(noop_kernel, NULL, warmup, iterations, &out);
  return out;
}

const char *ran_perf_build_flags(void)
{
#if defined(RAN_PERF_HAVE_AVX2)
  return "O3 -march=native -mavx2 -mfma";
#else
  return "O3 portable";
#endif
}

int run_benchmark_suite(const char *output_path, const bench_config_t *cfg)
{
  const size_t lengths[] = {4096, 16384, 65536, 262144};
  const size_t n_lengths = sizeof(lengths) / sizeof(lengths[0]);

  FILE *fp = fopen(output_path, "w");
  if (!fp) {
    return 1;
  }

  time_t now = time(NULL);
  struct tm tm_buf;
  gmtime_r(&now, &tm_buf);
  char iso[32];
  strftime(iso, sizeof(iso), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);

  timing_stats_t nf = measure_noise_floor(cfg->warmup, cfg->iterations);

  fprintf(fp, "{\n");
  fprintf(fp, "  \"schema\": ");
  json_escape(fp, RAN_PERF_SCHEMA_BENCH);
  fprintf(fp, ",\n");
  fprintf(fp, "  \"timestamp_utc\": ");
  json_escape(fp, iso);
  fprintf(fp, ",\n");
  fprintf(fp, "  \"build_flags\": ");
  json_escape(fp, ran_perf_build_flags());
  fprintf(fp, ",\n");
  fprintf(fp, "  \"config\": {\n");
  fprintf(fp, "    \"warmup_iterations\": %llu,\n", (unsigned long long)cfg->warmup);
  fprintf(fp, "    \"timed_iterations\": %llu,\n", (unsigned long long)cfg->iterations);
  fprintf(fp, "    \"seed\": %u,\n", cfg->seed);
  fprintf(fp, "    \"subtract_noise_floor\": %s\n", cfg->subtract_noise_floor ? "true" : "false");
  fprintf(fp, "  },\n");
  fprintf(fp, "  \"noise_floor_ns\": {\"mean_ns\": %.3f, \"stdev_ns\": %.3f},\n", nf.mean_ns, nf.stdev_ns);
  fprintf(fp, "  \"backends\": {\n");
  fprintf(fp, "    \"dot_real\": ");
  json_escape(fp, dot_real_simd_name());
  fprintf(fp, ",\n    \"dot_cplx\": ");
  json_escape(fp, dot_cplx_simd_name());
  fprintf(fp, ",\n    \"phase_rotate\": ");
  json_escape(fp, phase_rotate_simd_name());
  fprintf(fp, "\n  },\n");
  fprintf(fp, "  \"benchmarks\": [\n");

  int first_row = 1;
  for (size_t li = 0; li < n_lengths; li++) {
    size_t n = lengths[li];
    float *a = calloc(n, sizeof(float));
    float *b = calloc(n, sizeof(float));
    cfloat_t *ca = calloc(n, sizeof(cfloat_t));
    cfloat_t *cb = calloc(n, sizeof(cfloat_t));
    cfloat_t *out_s = calloc(n, sizeof(cfloat_t));
    cfloat_t *out_v = calloc(n, sizeof(cfloat_t));
    if (!a || !b || !ca || !cb || !out_s || !out_v) {
      fclose(fp);
      return 1;
    }
    bench_fill_random(a, b, ca, cb, n, cfg->seed + (uint32_t)li);
    cfloat_t rot = {0.9238795f, 0.3826834f};

    kernel_ctx_t dot_s = {.mode = K_DOT};
    dot_s.u.dot = (typeof(dot_s.u.dot)){a, b, n, 0.f, 0};
    kernel_ctx_t dot_v = {.mode = K_DOT};
    dot_v.u.dot = (typeof(dot_v.u.dot)){a, b, n, 0.f, 1};
    if (!first_row) {
      fprintf(fp, ",\n");
    }
    first_row = 0;
    bench_pair(fp, "dot_real", n, &dot_s, &dot_v, cfg, (double)n);

    kernel_ctx_t cplx_s = {.mode = K_CPLX};
    cplx_s.u.cplx = (typeof(cplx_s.u.cplx)){ca, cb, n, {0, 0}, 0};
    kernel_ctx_t cplx_v = {.mode = K_CPLX};
    cplx_v.u.cplx = (typeof(cplx_v.u.cplx)){ca, cb, n, {0, 0}, 1};
    fprintf(fp, ",\n");
    bench_pair(fp, "dot_cplx_conj", n, &cplx_s, &cplx_v, cfg, (double)n);

    kernel_ctx_t nrg_s = {.mode = K_ENERGY};
    nrg_s.u.nrg = (typeof(nrg_s.u.nrg)){ca, n, 0.0, 0};
    kernel_ctx_t nrg_v = {.mode = K_ENERGY};
    nrg_v.u.nrg = (typeof(nrg_v.u.nrg)){ca, n, 0.0, 1};
    fprintf(fp, ",\n");
    bench_pair(fp, "channel_energy", n, &nrg_s, &nrg_v, cfg, (double)n);

    kernel_ctx_t rot_s = {.mode = K_ROT};
    rot_s.u.rot = (typeof(rot_s.u.rot)){out_s, ca, rot, n, 0};
    kernel_ctx_t rot_v = {.mode = K_ROT};
    rot_v.u.rot = (typeof(rot_v.u.rot)){out_v, ca, rot, n, 1};
    fprintf(fp, ",\n");
    bench_pair(fp, "phase_rotate", n, &rot_s, &rot_v, cfg, (double)n);

    free(a);
    free(b);
    free(ca);
    free(cb);
    free(out_s);
    free(out_v);
  }

  fprintf(fp, "  ]\n");
  fprintf(fp, "}\n");
  fclose(fp);
  return 0;
}
