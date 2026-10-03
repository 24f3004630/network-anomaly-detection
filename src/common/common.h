#ifndef COMMON_H
#define COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static inline float *load_matrix(const char *path, long n, int d) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    size_t cnt = (size_t)n * d;
    float *x = malloc(cnt * sizeof(float));
    if (!x) { fprintf(stderr, "out of memory\n"); exit(1); }
    size_t got = fread(x, sizeof(float), cnt, f);
    if (got != cnt) { fprintf(stderr, "short read: got %zu, expected %zu (check n and d)\n", got, cnt); exit(1); }
    fclose(f);
    return x;
}

static inline uint8_t *load_labels(const char *path, long n) {
    if (!path || path[0] == '-') return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    uint8_t *t = malloc(n);
    if (!t || fread(t, 1, n, f) != (size_t)n) { fprintf(stderr, "bad labels file\n"); exit(1); }
    fclose(f);
    return t;
}

static inline void report(const char *method, long n, const uint8_t *flag,
                          const uint8_t *truth, double t_load, double t_comp) {
    long flagged = 0, tp = 0, fp = 0, fn = 0;
    for (long i = 0; i < n; i++) {
        if (flag[i]) flagged++;
        if (truth) {
            if (flag[i] && truth[i]) tp++;
            else if (flag[i] && !truth[i]) fp++;
            else if (!flag[i] && truth[i]) fn++;
        }
    }
    printf("method=%s n=%ld flagged=%ld\n", method, n, flagged);
    if (truth) {
        double p = (tp + fp) ? (double)tp / (tp + fp) : 0;
        double r = (tp + fn) ? (double)tp / (tp + fn) : 0;
        double f = (p + r) ? 2 * p * r / (p + r) : 0;
        printf("precision=%.4f recall=%.4f f1=%.4f (tp=%ld fp=%ld fn=%ld)\n", p, r, f, tp, fp, fn);
    }
    printf("time_load=%.3f time_compute=%.3f\n", t_load, t_comp);
    printf("RESULT,%s,%ld,%.6f,%.6f\n", method, n, t_load, t_comp);
}
#endif