// Usage: kmeans_seq <data.bin> <n> <d> <labels.labels|-> <k> [mult] [maxit]
#include <string.h>
#include "common.h"

int main(int argc, char **argv) {
    if (argc < 6) { fprintf(stderr, "usage: %s data n d labels|- k [mult] [maxit]\n", argv[0]); return 1; }
    long n = atol(argv[2]);
    int d = atoi(argv[3]);
    int k = atoi(argv[5]);
    double mult = argc > 6 ? atof(argv[6]) : 4.0;
    int maxit = argc > 7 ? atoi(argv[7]) : 20;

    double t0 = now_sec();
    float *x = load_matrix(argv[1], n, d);
    uint8_t *truth = load_labels(argv[4], n);
    double t1 = now_sec();

    // deterministic init: k evenly spaced rows (same rule the parallel version will use)
    double *cent = malloc((size_t)k * d * sizeof(double));
    for (int c = 0; c < k; c++) {
        long row = (long)c * (n / k);
        for (int j = 0; j < d; j++) cent[c * d + j] = x[(size_t)row * d + j];
    }
    double *sum = malloc((size_t)k * d * sizeof(double));
    long *cnt = malloc(k * sizeof(long));

    int it;
    for (it = 0; it < maxit; it++) {
        memset(sum, 0, (size_t)k * d * sizeof(double));
        memset(cnt, 0, k * sizeof(long));
        for (long i = 0; i < n; i++) {
            const float *p = x + (size_t)i * d;
            int best = 0; double bd = 1e300;
            for (int c = 0; c < k; c++) {
                double dist = 0;
                for (int j = 0; j < d; j++) {
                    double diff = p[j] - cent[c * d + j];
                    dist += diff * diff;
                }
                if (dist < bd) { bd = dist; best = c; }
            }
            cnt[best]++;
            for (int j = 0; j < d; j++) sum[best * d + j] += p[j];
        }
        double shift = 0;
        for (int c = 0; c < k; c++) {
            if (cnt[c] == 0) continue;  // empty cluster: keep old centroid
            for (int j = 0; j < d; j++) {
                double nv = sum[c * d + j] / cnt[c];
                double mv = fabs(nv - cent[c * d + j]);
                if (mv > shift) shift = mv;
                cent[c * d + j] = nv;
            }
        }
        if (shift < 1e-4) { it++; break; }
    }

    // score = distance to nearest centroid; flag if > mean + mult * std
    float *score = malloc(n * sizeof(float));
    double s = 0, s2 = 0;
    for (long i = 0; i < n; i++) {
        const float *p = x + (size_t)i * d;
        double bd = 1e300;
        for (int c = 0; c < k; c++) {
            double dist = 0;
            for (int j = 0; j < d; j++) {
                double diff = p[j] - cent[c * d + j];
                dist += diff * diff;
            }
            if (dist < bd) bd = dist;
        }
        double sc = sqrt(bd);
        score[i] = (float)sc;
        s += sc; s2 += sc * sc;
    }
    double mean = s / n, sd = sqrt(s2 / n - mean * mean), thr = mean + mult * sd;
    uint8_t *flag = malloc(n);
    for (long i = 0; i < n; i++) flag[i] = score[i] > thr;
    double t2 = now_sec();

    printf("iterations=%d threshold=%.3f\n", it, thr);
    report("kmeans_seq", n, flag, truth, t1 - t0, t2 - t1);
    return 0;
}