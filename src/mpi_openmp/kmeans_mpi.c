// Usage: mpirun -np P kmeans_mpi <data.bin> <n> <d> <labels|-> <k> [mult] [maxit]
// Threads per rank: set OMP_NUM_THREADS
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

static float *read_rows(const char *path, long off, long cnt, int d) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); MPI_Abort(MPI_COMM_WORLD, 1); }
    float *x = malloc((size_t)cnt * d * sizeof(float));
    if (!x) { fprintf(stderr, "out of memory\n"); MPI_Abort(MPI_COMM_WORLD, 1); }
    fseeko(f, (off_t)off * d * sizeof(float), SEEK_SET);
    if (fread(x, sizeof(float), (size_t)cnt * d, f) != (size_t)cnt * d) {
        fprintf(stderr, "short read\n"); MPI_Abort(MPI_COMM_WORLD, 1);
    }
    fclose(f);
    return x;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);
    int rank, p;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    if (argc < 6) {
        if (rank == 0) fprintf(stderr, "usage: %s data n d labels|- k [mult] [maxit]\n", argv[0]);
        MPI_Finalize(); return 1;
    }
    const char *path = argv[1];
    long n = atol(argv[2]);
    int d = atoi(argv[3]), k = atoi(argv[5]);
    double mult = argc > 6 ? atof(argv[6]) : 4.0;
    int maxit = argc > 7 ? atoi(argv[7]) : 20;

    long base = n / p, rem = n % p;
    long nloc = base + (rank < rem ? 1 : 0);
    long off = rank * base + (rank < rem ? rank : rem);

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();
    float *x = read_rows(path, off, nloc, d);
    uint8_t *truth = NULL;
    if (argv[4][0] != '-') {
        FILE *f = fopen(argv[4], "rb");
        if (!f) { perror(argv[4]); MPI_Abort(MPI_COMM_WORLD, 1); }
        truth = malloc(nloc);
        fseeko(f, (off_t)off, SEEK_SET);
        if (fread(truth, 1, nloc, f) != (size_t)nloc) MPI_Abort(MPI_COMM_WORLD, 1);
        fclose(f);
    }

    // same init rule as sequential: rows c*(n/k); rank 0 reads them and broadcasts
    double *cent = malloc((size_t)k * d * sizeof(double));
    if (rank == 0) {
        FILE *f = fopen(path, "rb");
        float *row = malloc(d * sizeof(float));
        for (int c = 0; c < k; c++) {
            long r = (long)c * (n / k);
            fseeko(f, (off_t)r * d * sizeof(float), SEEK_SET);
            if (fread(row, sizeof(float), d, f) != (size_t)d) MPI_Abort(MPI_COMM_WORLD, 1);
            for (int j = 0; j < d; j++) cent[c * d + j] = row[j];
        }
        free(row); fclose(f);
    }
    MPI_Bcast(cent, k * d, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    double *sum = malloc((size_t)k * d * sizeof(double));
    long *cnt = malloc(k * sizeof(long));
    int it;
    for (it = 0; it < maxit; it++) {
        memset(sum, 0, (size_t)k * d * sizeof(double));
        memset(cnt, 0, k * sizeof(long));
        #pragma omp parallel
        {
            double *ls = calloc((size_t)k * d, sizeof(double));
            long *lc = calloc(k, sizeof(long));
            #pragma omp for schedule(static)
            for (long i = 0; i < nloc; i++) {
                const float *pt = x + (size_t)i * d;
                int best = 0; double bd = 1e300;
                for (int c = 0; c < k; c++) {
                    double dist = 0;
                    for (int j = 0; j < d; j++) {
                        double diff = pt[j] - cent[c * d + j];
                        dist += diff * diff;
                    }
                    if (dist < bd) { bd = dist; best = c; }
                }
                lc[best]++;
                for (int j = 0; j < d; j++) ls[best * d + j] += pt[j];
            }
            #pragma omp critical
            {
                for (int q = 0; q < k * d; q++) sum[q] += ls[q];
                for (int c = 0; c < k; c++) cnt[c] += lc[c];
            }
            free(ls); free(lc);
        }
        MPI_Allreduce(MPI_IN_PLACE, sum, k * d, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(MPI_IN_PLACE, cnt, k, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
        double shift = 0;
        for (int c = 0; c < k; c++) {
            if (cnt[c] == 0) continue;
            for (int j = 0; j < d; j++) {
                double nv = sum[c * d + j] / cnt[c];
                double mv = fabs(nv - cent[c * d + j]);
                if (mv > shift) shift = mv;
                cent[c * d + j] = nv;
            }
        }
        if (shift < 1e-4) { it++; break; }  // identical on all ranks
    }

    float *score = malloc(nloc * sizeof(float));
    double s = 0, s2 = 0;
    #pragma omp parallel for reduction(+:s,s2) schedule(static)
    for (long i = 0; i < nloc; i++) {
        const float *pt = x + (size_t)i * d;
        double bd = 1e300;
        for (int c = 0; c < k; c++) {
            double dist = 0;
            for (int j = 0; j < d; j++) {
                double diff = pt[j] - cent[c * d + j];
                dist += diff * diff;
            }
            if (dist < bd) bd = dist;
        }
        double sc = sqrt(bd);
        score[i] = (float)sc;
        s += sc; s2 += sc * sc;
    }
    double st[2] = {s, s2};
    MPI_Allreduce(MPI_IN_PLACE, st, 2, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    double mean = st[0] / n, sd = sqrt(st[1] / n - mean * mean), thr = mean + mult * sd;

    long c4[4] = {0, 0, 0, 0};  // flagged, tp, fp, fn
    for (long i = 0; i < nloc; i++) {
        int fl = score[i] > thr;
        c4[0] += fl;
        if (truth) {
            if (fl && truth[i]) c4[1]++;
            else if (fl && !truth[i]) c4[2]++;
            else if (!fl && truth[i]) c4[3]++;
        }
    }
    double t2 = MPI_Wtime();
    long g[4];
    MPI_Reduce(c4, g, 4, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    double tl = t1 - t0, tc = t2 - t1, tlm, tcm;
    MPI_Reduce(&tl, &tlm, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&tc, &tcm, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        int th = omp_get_max_threads();
        printf("iterations=%d threshold=%.3f\n", it, thr);
        printf("method=kmeans_mpi n=%ld p=%d threads=%d flagged=%ld\n", n, p, th, g[0]);
        if (truth) {
            double pr = (g[1] + g[2]) ? (double)g[1] / (g[1] + g[2]) : 0;
            double rc = (g[1] + g[3]) ? (double)g[1] / (g[1] + g[3]) : 0;
            printf("precision=%.4f recall=%.4f f1=%.4f (tp=%ld fp=%ld fn=%ld)\n",
                   pr, rc, (pr + rc) ? 2 * pr * rc / (pr + rc) : 0, g[1], g[2], g[3]);
        }
        printf("time_load=%.3f time_compute=%.3f\n", tlm, tcm);
        printf("RESULT,kmeans_mpi,%ld,%d,%d,%.6f,%.6f\n", n, p, th, tlm, tcm);
    }
    MPI_Finalize();
    return 0;
}