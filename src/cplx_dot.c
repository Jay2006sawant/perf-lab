/* SPDX-License-Identifier: CC-BY-4.0 */
#include "ran_perf_lab.h"

#include <immintrin.h>

cfloat_t dot_cplx_scalar(const cfloat_t *a, const cfloat_t *b, size_t n)
{
  cfloat_t acc = {0.f, 0.f};
  for (size_t i = 0; i < n; i++) {
    float br = b[i].re;
    float bi = b[i].im;
    acc.re += a[i].re * br + a[i].im * bi;
    acc.im += a[i].im * br - a[i].re * bi;
  }
  return acc;
}

#if defined(RAN_PERF_HAVE_AVX2)

cfloat_t dot_cplx_simd(const cfloat_t *a, const cfloat_t *b, size_t n)
{
  __m256 acc_re = _mm256_setzero_ps();
  __m256 acc_im = _mm256_setzero_ps();
  size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    __m256 va = _mm256_loadu_ps((const float *)&a[i]);
    __m256 vb = _mm256_loadu_ps((const float *)&b[i]);
    __m256 ar = _mm256_moveldup_ps(va);
    __m256 ai = _mm256_movehdup_ps(va);
    __m256 br = _mm256_moveldup_ps(vb);
    __m256 bi = _mm256_movehdup_ps(vb);
    /* sum a[i] * conj(b[i]) */
    acc_re = _mm256_fmadd_ps(ar, br, acc_re);
    acc_re = _mm256_fmadd_ps(ai, bi, acc_re);
    acc_im = _mm256_fmadd_ps(ai, br, acc_im);
    acc_im = _mm256_fnmadd_ps(ar, bi, acc_im);
  }
  float re_buf[8];
  float im_buf[8];
  _mm256_storeu_ps(re_buf, acc_re);
  _mm256_storeu_ps(im_buf, acc_im);
  cfloat_t acc = {re_buf[0] + re_buf[2] + re_buf[4] + re_buf[6], im_buf[0] + im_buf[2] + im_buf[4] + im_buf[6]};
  cfloat_t tail = dot_cplx_scalar(a + i, b + i, n - i);
  acc.re += tail.re;
  acc.im += tail.im;
  return acc;
}

const char *dot_cplx_simd_name(void)
{
  return "avx2_fma_cplx";
}

#else

cfloat_t dot_cplx_simd(const cfloat_t *a, const cfloat_t *b, size_t n)
{
  return dot_cplx_scalar(a, b, n);
}

const char *dot_cplx_simd_name(void)
{
  return "scalar_fallback";
}

#endif

double energy_scalar(const cfloat_t *x, size_t n)
{
  double e = 0.0;
  for (size_t i = 0; i < n; i++) {
    e += (double)x[i].re * x[i].re + (double)x[i].im * x[i].im;
  }
  return e;
}

#if defined(RAN_PERF_HAVE_AVX2)

double energy_simd(const cfloat_t *x, size_t n)
{
  __m256 vacc = _mm256_setzero_ps();
  size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    __m256 v = _mm256_loadu_ps((const float *)&x[i]);
    __m256 re = _mm256_moveldup_ps(v);
    __m256 im = _mm256_movehdup_ps(v);
    vacc = _mm256_fmadd_ps(re, re, vacc);
    vacc = _mm256_fmadd_ps(im, im, vacc);
  }
  float buf[8];
  _mm256_storeu_ps(buf, vacc);
  double e = (double)buf[0] + buf[2] + buf[4] + buf[6];
  for (; i < n; i++) {
    e += (double)x[i].re * x[i].re + (double)x[i].im * x[i].im;
  }
  return e;
}

#else

double energy_simd(const cfloat_t *x, size_t n)
{
  return energy_scalar(x, n);
}

#endif
