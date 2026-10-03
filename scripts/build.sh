#!/bin/bash
mkdir -p bin
gcc -O2 -Wall -Isrc/common -o bin/zscore_seq src/sequential/zscore_seq.c -lm
gcc -O2 -Wall -Isrc/common -o bin/kmeans_seq src/sequential/kmeans_seq.c -lm
mpicc -O2 -fopenmp -Wall -Isrc/common -o bin/kmeans_mpi src/mpi_openmp/kmeans_mpi.c -lm
echo "build done"