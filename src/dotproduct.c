/* SPDX-License-Identifier: CC-BY-4.0 */
#include "ran_perf_lab.h"

#include <immintrin.h>
#include <stddef.h>

float dot_real_scalar(const float *a, const float *b, size_t n)
{
  float acc = 0.f;
  for (size_t i = 0; i < n; i++) {
    acc += a[i] * b[i];
  }
  return acc;
}

#if defined(RAN_PERF_HAVE_AVX2)

float dot_real_simd(const float *a, const float *b, size_t n)
{
  __m256 vacc = _mm256_setzero_ps();
  size_t i = 0;
  for (; i + 8 <= n; i += 8) {
    __m256 va = _mm256_loadu_ps(a + i);
    __m256 vb = _mm256_loadu_ps(b + i);
    vacc = _mm256_fmadd_ps(va, vb, vacc);
  }
  __m128 sum128 = _mm_add_ps(_mm256_castps256_ps128(vacc), _mm256_extractf128_ps(vacc, 1));
  sum128 = _mm_hadd_ps(sum128, sum128);
  sum128 = _mm_hadd_ps(sum128, sum128);
  float acc = _mm_cvtss_f32(sum128);
  for (; i < n; i++) {
    acc += a[i] * b[i];
  }
  return acc;
}

const char *dot_real_simd_name(void)
{
  return "avx2_fma";
}

#elif defined(__ARM_NEON) || defined(__aarch64__)

#include <arm_neon.h>

float dot_real_simd(const float *a, const float *b, size_t n)
{
  float32x4_t vacc = vdupq_n_f32(0.f);
  size_t i = 0;
  for (; i + 4 <= n; i += 4) {
    float32x4_t va = vld1q_f32(a + i);
    float32x4_t vb = vld1q_f32(b + i);
    vacc = vmlaq_f32(vacc, va, vb);
  }
  float acc = vaddvq_f32(vacc);
  for (; i < n; i++) {
    acc += a[i] * b[i];
  }
  return acc;
}

const char *dot_real_simd_name(void)
{
  return "neon";
}

#else

float dot_real_simd(const float *a, const float *b, size_t n)
{
  return dot_real_scalar(a, b, n);
}

const char *dot_real_simd_name(void)
{
  return "scalar_fallback";
}

#endif
