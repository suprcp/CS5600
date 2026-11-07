#!/bin/bash
# Runs all Chapter 29 experiments. Output goes to the screen and results.txt
gcc -O2 -Wall -pthread -o ch29 ch29.c || exit 1
OUT=results.txt
: > $OUT
log() { echo "$@" | tee -a $OUT; }

log "===== Machine ====="
log "CPUs: $(nproc)"
log ""
log "===== Q1: timer ====="
./ch29 timer | tee -a $OUT
log ""
log "===== Q2: precise counter (1M increments per thread) ====="
for t in 1 2 4 8; do timeout 60 ./ch29 counter $t 1000000 | tee -a $OUT; done
log ""
log "===== Q3a: approximate counter, S=1024, vary threads ====="
for t in 1 2 4 8; do timeout 60 ./ch29 approx $t 1000000 1024 | tee -a $OUT; done
log ""
log "===== Q3b: approximate counter, 4 threads, vary S ====="
for s in 1 2 4 8 16 32 64 128 256 512 1024; do timeout 60 ./ch29 approx 4 1000000 $s | tee -a $OUT; done
log ""
log "===== Q4: single-lock list vs hand-over-hand (1000 nodes, 2000 lookups/thread) ====="
for w in 0 200; do
  for t in 1 2 4 8; do
    timeout 60 ./ch29 list single $t 1000 2000 $w | tee -a $OUT
    timeout 60 ./ch29 list hoh    $t 1000 2000 $w | tee -a $OUT
  done
done
log ""
log "===== Q5/Q6: hash table, one big lock vs per-bucket locks (1M ops/thread, 90% lookup / 10% insert) ====="
for t in 1 2 4 8; do
  timeout 60 ./ch29 hash single $t 1000000 | tee -a $OUT
  timeout 60 ./ch29 hash bucket $t 1000000 | tee -a $OUT
done
