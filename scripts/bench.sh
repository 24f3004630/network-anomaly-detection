#!/bin/bash
# Benchmark driver: strong scaling, data-volume scaling, hybrid MPI x OpenMP.
# Usage (from anywhere):
#   MPIFLAGS="--use-hwthread-cpus --bind-to hwthread --map-by hwthread" REPS=5 bash scripts/bench.sh
cd "$(dirname "$0")/.." || exit 1

MPIFLAGS=${MPIFLAGS:---use-hwthread-cpus --bind-to hwthread --map-by hwthread}
REPS=${REPS:-5}
K=8
OUT=results/results.csv
mkdir -p results

# --- sanity checks before spending 15+ minutes
for f in bin/kmeans_seq bin/kmeans_mpi; do
  [ -x "$f" ] || { echo "missing $f, run: bash scripts/build.sh"; exit 1; }
done
for n in 1000000 2000000 4000000 8000000; do
  for ext in bin labels; do
    [ -f "data/flows_$n.$ext" ] || { echo "missing data/flows_$n.$ext, run: python3 scripts/gen_synthetic.py $n"; exit 1; }
  done
done

echo "experiment,method,n,p,threads,t_load,t_compute" > $OUT

run_seq() {  # experiment n
  local D=data/flows_$2
  echo "seq  $1 n=$2"
  for r in $(seq $REPS); do
    ./bin/kmeans_seq $D.bin $2 16 $D.labels $K | grep '^RESULT' \
      | awk -F, -v e=$1 '{print e","$2","$3",1,1,"$4","$5}' >> $OUT
  done
}

run_mpi() {  # experiment n p threads
  local D=data/flows_$2
  local FL="$MPIFLAGS"
  # pinning each rank to one hardware thread would squeeze all its OpenMP
  # threads onto that thread, so hybrid runs (threads > 1) are left unbound
  if [ "$4" -gt 1 ]; then FL="--use-hwthread-cpus --bind-to none"; fi
  echo "mpi  $1 n=$2 p=$3 threads=$4 [$FL]"
  for r in $(seq $REPS); do
    OMP_NUM_THREADS=$4 mpirun -np $3 $FL ./bin/kmeans_mpi $D.bin $2 16 $D.labels $K | grep '^RESULT' \
      | awk -F, -v e=$1 '{print e","$2","$3","$4","$5","$6","$7}' >> $OUT
  done
  sleep 2   # short cooldown between configurations
}

N=4000000

echo "== strong scaling (n=$N)"
run_seq strong $N
for p in 1 2 3 4 6 8 12; do run_mpi strong $N $p 1; done

echo "== data volume"
for n in 1000000 2000000 4000000 8000000; do
  run_seq volume $n
  for p in 1 4 8; do run_mpi volume $n $p 1; done
done

echo "== hybrid, 12 hardware threads total (n=$N)"
for cfg in "12 1" "6 2" "4 3" "3 4" "2 6" "1 12"; do
  set -- $cfg
  run_mpi hybrid $N $1 $2
done

echo "done: $OUT ($(($(wc -l < $OUT) - 1)) rows)"