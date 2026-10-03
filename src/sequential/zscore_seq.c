// Usage: zscore_seq <data.bin> <n> <d> <labels.labels|-> [threshold]
#include <string.h>
#include "common.h"

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: %s data n d labels|- [threshold]\n", argv[0]); return 1; }
    long n = atol(argv[2]);
    int d = atoi(argv[3]);
    double thr = argc > 5 ? atof(argv[5]) : 4.5;

    double t0 = now_sec();
    float *x = load_matrix(argv[1], n, d);
    uint8_t *truth = load_labels(argv[4], n);
    double t1 = now_sec();

    double *sum = calloc(d, sizeof(double)), *sq = calloc(d, sizeof(double));
    double *mean = malloc(d * sizeof(double)), *sd = malloc(d * sizeof(double));
    for (long i = 0; i < n; i++)
        for (int j = 0; j < d; j++) {
            double v = x[(size_t)i * d + j];
            sum[j] += v;
            sq[j] += v * v;
        }
    for (int j = 0; j < d; j++) {
        mean[j] = sum[j] / n;
        sd[j] = sqrt(sq[j] / n - mean[j] * mean[j]);
        if (sd[j] < 1e-12) sd[j] = 1e-12;
    }

    uint8_t *flag = malloc(n);
    for (long i = 0; i < n; i++) {
        double m = 0;
        for (int j = 0; j < d; j++) {
            double z = fabs((x[(size_t)i * d + j] - mean[j]) / sd[j]);
            if (z > m) m = z;
        }
        flag[i] = m > thr;
    }
    double t2 = now_sec();

    report("zscore_seq", n, flag, truth, t1 - t0, t2 - t1);
    return 0;
}