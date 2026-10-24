#!/usr/bin/env bash
# OSTEP Chapter 18 (Paging: Introduction) homework driver.
# Part 1 runs the book's commands for Q1-Q4.
# Part 2 probes the limits / edge cases of paging-linear-translate.py.
# Every command and its output is saved to logs/<case>.txt;
# logs/summary.txt lists each case with its exit code (0 = ran, 1 = rejected or crashed).
#
# Usage:  ./explore_limits.sh            (simulator in the same directory)
#         SIM=/path/to/paging-linear-translate.py ./explore_limits.sh

SIM=${SIM:-./paging-linear-translate.py}
PY=${PYTHON:-python3}
LOGDIR=logs
SUMMARY=$LOGDIR/summary.txt

if [[ ! -f "$SIM" ]]; then
  echo "Cannot find $SIM. Copy it from ostep-homework/vm-paging or set SIM=..." >&2
  exit 1
fi

mkdir -p "$LOGDIR"
: > "$SUMMARY"
printf "%-24s %-4s %s\n" "CASE" "RC" "COMMAND" | tee -a "$SUMMARY"

# run_case <name> <simulator args...>
run_case() {
  local name=$1; shift
  local log="$LOGDIR/$name.txt"
  echo "\$ $PY $SIM $*" > "$log"
  echo "------------------------------------------------------------" >> "$log"
  $PY "$SIM" "$@" >> "$log" 2>&1
  local rc=$?
  echo "[exit code: $rc]" >> "$log"
  printf "%-24s %-4s %s\n" "$name" "$rc" "$*" | tee -a "$SUMMARY"
}

######################## Part 1: book questions ########################
# Q1: page-table size vs address-space size
run_case q1_as1m  -P 1k -a 1m -p 512m -v -n 0
run_case q1_as2m  -P 1k -a 2m -p 512m -v -n 0
run_case q1_as4m  -P 1k -a 4m -p 512m -v -n 0
# Q2: page-table size vs page size
run_case q2_ps1k  -P 1k -a 1m -p 512m -v -n 0
run_case q2_ps2k  -P 2k -a 1m -p 512m -v -n 0
run_case q2_ps4k  -P 4k -a 1m -p 512m -v -n 0
# Q3: percentage of valid pages (-c prints the answers)
for u in 0 25 50 75 100; do
  run_case "q3_u$u" -P 1k -a 16k -p 32k -v -u "$u" -c
done
# Q4: unrealistic parameter combinations, different seeds
run_case q4_tiny     -P 8  -a 32   -p 1024 -v -s 1 -c
run_case q4_small    -P 8k -a 32k  -p 1m   -v -s 2 -c
run_case q4_hugepage -P 1m -a 256m -p 512m -v -s 3 -c

######################## Part 2: limits ########################
# 1. Address space vs physical memory: the simulator requires pmem > asize
run_case lim_as_gt_pmem     -P 1k -a 64k -p 32k -v -c   # AS bigger than RAM -> rejected
run_case lim_as_eq_pmem     -P 1k -a 32k -p 32k -v -c   # equal is rejected too (check is <=)

# 2. Page size vs address space
run_case lim_page_gt_as     -P 32k -a 16k -p 64k -v -c  # page bigger than AS -> caught by multiple-of check
run_case lim_page_eq_as     -P 16k -a 16k -p 64k -v -c  # one page, 0 VPN bits: degenerates to base-and-bounds
run_case lim_page_1byte     -P 1   -a 16  -p 64  -v -c  # 0 offset bits: a 4-byte PTE for every 1-byte page
run_case lim_smallest_valid -P 1   -a 1   -p 2   -v -c  # smallest configuration that passes every check

# 3. Powers of two
run_case lim_as_not_pow2    -P 1k   -a 15k -p 32k -v -c # AS must be a power of 2
run_case lim_page_not_pow2  -P 1000 -a 16k -p 32k -v -c # rejected by multiple-of check, not the power-of-2 check
run_case lim_pmem_not_pow2  -P 1k   -a 16k -p 33k -v -c # accepted: pmem only has to be a multiple of the page size

# 4. Inputs the simulator does not validate (Python crashes)
run_case lim_page_zero      -P 0  -a 16k -p 32k -v -c   # ZeroDivisionError
run_case lim_garbage_size   -P 1k -a abc -p 32k -v -c   # ValueError in convert()
run_case lim_page_negative  -P -1k -a 16k -p 32k -v -c  # passes the multiple-of check (16k/-1k = -16), then log() of a negative
run_case lim_as_negative    -P 1k -a -16k -p 32k -v -c  # rejected, but the message says "non-zero"

# 5. Size limits
run_case lim_as_1g          -P 4k -a 1g   -p 2g   -n 1 -c  # everything must be < 1GB
run_case lim_max_as_512m    -P 4k -a 512m -p 768m -n 2 -c  # largest AS that passes every check

# 6. -u is not range-checked
run_case lim_u_over100      -P 1k -a 16k -p 32k -v -u 101 -c  # behaves like -u 100
run_case lim_u_negative     -P 1k -a 16k -p 32k -v -u -1  -c  # behaves like -u 0
run_case lim_n_negative     -P 1k -a 16k -p 32k -n -5 -c      # range(0,-5) is empty: no error, no addresses

# 7. User-supplied addresses (-A): no bounds checking
run_case lim_A_in_range     -P 1k -a 16k -p 32k -v -c -A 0,1023,1024,16383  # page boundaries and last byte
run_case lim_A_eq_asize     -P 1k -a 16k -p 32k -v -c -A 16384              # one past the end -> IndexError
run_case lim_A_minus1       -P 1k -a 16k -p 32k -v -c -A -1                 # -1 is the "random" sentinel
run_case lim_A_hex          -P 1k -a 16k -p 32k -v -c -A 0x10               # output is hex, but hex input is rejected

# 8. Is -u really the fraction of valid pages? (512 virtual pages)
echo
for u in 50 90; do
  for pm in 520k 64m; do
    n=$($PY "$SIM" -P 1k -a 512k -p $pm -u $u -n 0 -v | grep -c '\]  0x8')
    echo "u=$u pmem=$pm : $n / 512 valid ($(( n * 100 / 512 ))%)" | tee -a "$LOGDIR/valid_fraction.txt"
  done
done

echo
echo "Full outputs are in $LOGDIR/ ; summary in $SUMMARY"
