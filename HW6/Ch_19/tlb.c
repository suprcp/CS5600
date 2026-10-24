// tlb.c - measure the average cost of touching one int per page (OSTEP ch.19)
//
// Usage: ./tlb <num_pages> <num_trials> [noinit]
// Output (one CSV line): pages,trials,ns_per_access,checksum,cpu
//
// Design choices (see README):
//  * Pins itself to CPU 0 with sched_setaffinity() on Linux (Q6).
//  * Uses a volatile pointer so the compiler cannot delete or collapse
//    the access loop, and prints a checksum of the array (Q5).
//  * Touches every page before timing so demand-zeroing / page-fault costs
//    are not measured; "noinit" skips this to show the effect (Q7).
//  * Uses clock_gettime(CLOCK_MONOTONIC) (ns resolution, never jumps
//    backwards) instead of gettimeofday() (us resolution) (Q1).

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sched.h>
#endif

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static int pin_to_cpu(int cpu) {
#ifdef __linux__
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set) != 0) {
        perror("sched_setaffinity");
        exit(1);
    }
    return cpu;
#else
    (void)cpu;
    fprintf(stderr, "warning: CPU pinning is not supported on this OS\n");
    return -1;
#endif
}

static int current_cpu(void) {
#ifdef __linux__
    return sched_getcpu();
#else
    return -1;
#endif
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <num_pages> <num_trials> [noinit]\n", argv[0]);
        return 1;
    }
    long num_pages = atol(argv[1]);
    long trials    = atol(argv[2]);
    int  do_init   = !(argc > 3 && strcmp(argv[3], "noinit") == 0);
    if (num_pages <= 0 || trials <= 0) {
        fprintf(stderr, "num_pages and num_trials must be positive\n");
        return 1;
    }

    pin_to_cpu(0);

    long pagesize = sysconf(_SC_PAGESIZE);       // 4096 on x86 Linux, 16384 on Apple Silicon
    long jump     = pagesize / (long)sizeof(int); // ints per page
    long n        = num_pages * jump;             // total ints in the array

    int *buf;
    if (posix_memalign((void **)&buf, (size_t)pagesize, (size_t)n * sizeof(int)) != 0) {
        fprintf(stderr, "allocation of %ld pages failed\n", num_pages);
        return 1;
    }
    volatile int *a = buf;

    if (do_init) {
        // Touch every page once: page faults + zeroing happen here, untimed.
        for (long i = 0; i < n; i += jump)
            a[i] = 0;
    }

    double start = now_ns();
    for (long t = 0; t < trials; t++)
        for (long i = 0; i < n; i += jump)
            a[i] += 1;
    double end = now_ns();

    long checksum = 0;
    for (long i = 0; i < n; i += jump)
        checksum += a[i];

    double ns_per_access = (end - start) / ((double)trials * (double)num_pages);
    printf("%ld,%ld,%.4f,%ld,%d\n", num_pages, trials, ns_per_access, checksum, current_cpu());

    free(buf);
    return 0;
}
