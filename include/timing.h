/* SPDX-License-Identifier: CC-BY-4.0 */
#ifndef RAN_PERF_TIMING_H
#define RAN_PERF_TIMING_H

#include "ran_perf_lab.h"

uint64_t timing_now_ns(void);
void timing_compute_stats(const double *samples_ns, uint64_t n, timing_stats_t *out);

#endif
