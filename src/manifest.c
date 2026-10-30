/* SPDX-License-Identifier: CC-BY-4.0 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ran_perf_lab.h"

static void json_escape(FILE *fp, const char *s)
{
  if (!s) {
    fprintf(fp, "null");
    return;
  }
  fputc('"', fp);
  for (const char *p = s; *p; p++) {
    if (*p == '"' || *p == '\\') {
      fputc('\\', fp);
    }
    fputc(*p, fp);
  }
  fputc('"', fp);
}

static void read_cpu_model(char *model, size_t buflen)
{
  strncpy(model, "unknown", buflen);
  FILE *f = fopen("/proc/cpuinfo", "r");
  if (!f) {
    return;
  }
  char line[512];
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "model name", 10) == 0) {
      char *colon = strchr(line, ':');
      if (colon) {
        colon++;
        while (*colon == ' ' || *colon == '\t') {
          colon++;
        }
        strncpy(model, colon, buflen - 1);
        model[buflen - 1] = '\0';
        size_t n = strlen(model);
        while (n > 0 && (model[n - 1] == '\n' || model[n - 1] == '\r')) {
          model[--n] = '\0';
        }
      }
      break;
    }
  }
  fclose(f);
}

static long read_memtotal_kb(void)
{
  FILE *f = fopen("/proc/meminfo", "r");
  if (!f) {
    return -1;
  }
  char line[256];
  long kb = -1;
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "MemTotal:", 9) == 0) {
      sscanf(line, "MemTotal: %ld kB", &kb);
      break;
    }
  }
  fclose(f);
  return kb;
}

int write_platform_manifest(const char *output_path)
{
  char model[256];
  char kernel[512];
  char hostname[256];
  read_cpu_model(model, sizeof(model));
  kernel[0] = '\0';
  FILE *kv = fopen("/proc/version", "r");
  if (kv) {
    if (!fgets(kernel, sizeof(kernel), kv)) {
      strncpy(kernel, "unknown", sizeof(kernel) - 1);
    }
    fclose(kv);
  }
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    strncpy(hostname, "unknown", sizeof(hostname) - 1);
  }
  long mem_kb = read_memtotal_kb();

  FILE *fp = fopen(output_path, "w");
  if (!fp) {
    return 1;
  }
  fprintf(fp, "{\n");
  fprintf(fp, "  \"schema\": ");
  json_escape(fp, RAN_PERF_SCHEMA_MANIFEST);
  fprintf(fp, ",\n");
  fprintf(fp, "  \"hostname\": ");
  json_escape(fp, hostname);
  fprintf(fp, ",\n");
  fprintf(fp, "  \"cpu_model\": ");
  json_escape(fp, model);
  fprintf(fp, ",\n");
  fprintf(fp, "  \"mem_total_kb\": %ld,\n", mem_kb);
  fprintf(fp, "  \"kernel\": ");
  json_escape(fp, kernel);
  fprintf(fp, ",\n");
  fprintf(fp, "  \"compiler\": ");
  json_escape(fp, __VERSION__);
  fprintf(fp, ",\n");
  fprintf(fp, "  \"build_flags\": ");
  json_escape(fp, ran_perf_build_flags());
  fprintf(fp, ",\n");
  fprintf(fp, "  \"simd_backends\": {\n");
  fprintf(fp, "    \"dot_real\": ");
  json_escape(fp, dot_real_simd_name());
  fprintf(fp, ",\n    \"dot_cplx\": ");
  json_escape(fp, dot_cplx_simd_name());
  fprintf(fp, ",\n    \"phase_rotate\": ");
  json_escape(fp, phase_rotate_simd_name());
  fprintf(fp, "\n  }\n");
  fprintf(fp, "}\n");
  fclose(fp);
  return 0;
}
