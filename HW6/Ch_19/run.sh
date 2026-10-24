#!/usr/bin/env bash
# Run tlb for 1, 2, 4, ... MAXPAGES pages, REPS times each, and save a CSV.
#
# Usage:  ./run.sh <label>          e.g. ./run.sh vm   ./run.sh mac   ./run.sh khoury
# Tunables (environment variables):
#   MAXPAGES  largest page count            (default 16384)
#   TOTAL     accesses per run; trials = TOTAL / pages, so every run does
#             about the same amount of work   (default 200000000)
#   MINTRIALS lower bound on trials           (default 1000)
#   REPS      repetitions per page count      (default 5)
#   INIT      set to "noinit" to skip pre-touching the array (Q7 experiment)
#   CFLAGS    compiler flags (default -O2), e.g. CFLAGS=-O0 for the Q5 comparison
set -euo pipefail
cd "$(dirname "$0")"

LABEL=${1:-$(hostname -s)}
MAXPAGES=${MAXPAGES:-16384}
TOTAL=${TOTAL:-200000000}
MINTRIALS=${MINTRIALS:-1000}
REPS=${REPS:-5}
INIT=${INIT:-}

CFLAGS=${CFLAGS:--O2}
gcc $CFLAGS -Wall -o tlb tlb.c
mkdir -p results
OUT=results/${LABEL}.csv
INFO=results/${LABEL}_machine.txt

# ---- record what machine this was ----
{
  echo "label: $LABEL"; echo "date: $(date)"; echo "host: $(hostname)"
  uname -a
  echo "page size: $(getconf PAGESIZE 2>/dev/null || getconf PAGE_SIZE)"
  if [[ "$(uname)" == "Linux" ]]; then
    lscpu 2>/dev/null || true
  elif [[ "$(uname)" == "Darwin" ]]; then
    sysctl -n machdep.cpu.brand_string hw.ncpu hw.pagesize 2>/dev/null || true
  fi
  echo "CFLAGS=$CFLAGS TOTAL=$TOTAL MINTRIALS=$MINTRIALS REPS=$REPS INIT=${INIT:-init}"
} > "$INFO"

# ---- pin to one CPU (belt and braces: tlb.c also calls sched_setaffinity) ----
PIN=()
if [[ "$(uname)" == "Linux" ]] && command -v taskset >/dev/null; then
  PIN=(taskset -c 0)
else
  echo "warning: no taskset on $(uname); cannot pin to one CPU" | tee -a "$INFO"
fi

echo "label,pages,trials,ns_per_access,checksum,cpu" > "$OUT"
for (( p = 1; p <= MAXPAGES; p *= 2 )); do
  trials=$(( TOTAL / p ))
  (( trials < MINTRIALS )) && trials=$MINTRIALS
  for (( r = 1; r <= REPS; r++ )); do
    line=$(${PIN[@]+"${PIN[@]}"} ./tlb "$p" "$trials" $INIT)
    echo "$LABEL,$line" >> "$OUT"
    echo "$LABEL,$line"
  done
done
echo "saved $OUT and $INFO"
