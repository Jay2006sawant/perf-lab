/* SPDX-License-Identifier: CC-BY-4.0 */
#include "ran_perf_lab.h"

#include <immintrin.h>

void phase_rotate_scalar(cfloat_t *out, const cfloat_t *in, cfloat_t rot, size_t n)
{
  for (size_t i = 0; i < n; i++) {
    float ar = in[i].re;
    float ai = in[i].im;
    out[i].re = ar * rot.re - ai * rot.im;
    out[i].im = ar * rot.im + ai * rot.re;
  }
}

#if defined(RAN_PERF_HAVE_AVX2)

void phase_rotate_simd(cfloat_t *out, const cfloat_t *in, cfloat_t rot, size_t n)
{
  __m128 rre = _mm_set1_ps(rot.re);
  __m128 rim = _mm_set1_ps(rot.im);
  size_t i = 0;
  for (; i + 2 <= n; i += 2) {
    __m128 vin = _mm_loadu_ps((const float *)&in[i]);
    __m128 ar = _mm_moveldup_ps(vin);
    __m128 ai = _mm_movehdup_ps(vin);
    __m128 ore = _mm_fmsub_ps(ar, rre, _mm_mul_ps(ai, rim));
    __m128 oim = _mm_fmadd_ps(ar, rim, _mm_mul_ps(ai, rre));
    __m128 lo = _mm_unpacklo_ps(ore, oim);
    __m128 hi = _mm_unpackhi_ps(ore, oim);
    __m128 vout = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(1, 0, 1, 0));
    _mm_storeu_ps((float *)&out[i], vout);
  }
  for (; i < n; i++) {
    float ar = in[i].re;
    float ai = in[i].im;
    out[i].re = ar * rot.re - ai * rot.im;
    out[i].im = ar * rot.im + ai * rot.re;
  }
}

const char *phase_rotate_simd_name(void)
{
  return "sse128_2x_cplx";
}

#elif defined(__ARM_NEON) || defined(__aarch64__)

#include <arm_neon.h>

void phase_rotate_simd(cfloat_t *out, const cfloat_t *in, cfloat_t rot, size_t n)
{
  size_t i = 0;
  for (; i + 2 <= n; i += 2) {
    for (size_t k = 0; k < 2; k++) {
      float ar = in[i + k].re;
      float ai = in[i + k].im;
      out[i + k].re = ar * rot.re - ai * rot.im;
      out[i + k].im = ar * rot.im + ai * rot.re;
    }
  }
  for (; i < n; i++) {
    float ar = in[i].re;
    float ai = in[i].im;
    out[i].re = ar * rot.re - ai * rot.im;
    out[i].im = ar * rot.im + ai * rot.re;
  }
}

const char *phase_rotate_simd_name(void)
{
  return "neon";
}

#else

void phase_rotate_simd(cfloat_t *out, const cfloat_t *in, cfloat_t rot, size_t n)
{
  phase_rotate_scalar(out, in, rot, n);
}

const char *phase_rotate_simd_name(void)
{
  return "scalar_fallback";
}

#endif
