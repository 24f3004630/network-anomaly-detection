#!/bin/bash
mkdir -p bin
gcc -O2 -Wall -Isrc/common -o bin/zscore_seq src/sequential/zscore_seq.c -lm
gcc -O2 -Wall -Isrc/common -o bin/kmeans_seq src/sequential/kmeans_seq.c -lm
echo "build done"