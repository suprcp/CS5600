// timer_precision.c - how precise are gettimeofday() and clock_gettime()? (Q1)
// For each timer: report the advertised resolution (where available), the
// smallest non-zero difference between two consecutive readings, and the
// average cost of one call.
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#define SAMPLES 1000000

static double tv_us(struct timeval tv) { return tv.tv_sec * 1e6 + tv.tv_usec; }
static double ts_ns(struct timespec ts) { return ts.tv_sec * 1e9 + ts.tv_nsec; }

int main(void) {
    // ---- gettimeofday (microsecond units) ----
    struct timeval a, b;
    double min_tv = 1e18;
    gettimeofday(&a, NULL);
    double t0 = tv_us(a);
    for (int i = 0; i < SAMPLES; i++) {
        gettimeofday(&a, NULL);
        do { gettimeofday(&b, NULL); } while (tv_us(b) == tv_us(a));
        double d = tv_us(b) - tv_us(a);
        if (d < min_tv) min_tv = d;
        if (i == 999) break;    // 1000 tick measurements is plenty
    }
    gettimeofday(&a, NULL);
    t0 = tv_us(a);
    for (int i = 0; i < SAMPLES; i++) gettimeofday(&b, NULL);
    double cost_tv_ns = (tv_us(b) - t0) * 1000.0 / SAMPLES;
    printf("gettimeofday : smallest observed step = %.0f us, cost per call = %.1f ns\n",
           min_tv, cost_tv_ns);

    // ---- clock_gettime(CLOCK_MONOTONIC) (nanosecond units) ----
    struct timespec res, x, y;
    clock_getres(CLOCK_MONOTONIC, &res);
    double min_ts = 1e18;
    for (int i = 0; i < 1000; i++) {
        clock_gettime(CLOCK_MONOTONIC, &x);
        do { clock_gettime(CLOCK_MONOTONIC, &y); } while (ts_ns(y) == ts_ns(x));
        double d = ts_ns(y) - ts_ns(x);
        if (d < min_ts) min_ts = d;
    }
    clock_gettime(CLOCK_MONOTONIC, &x);
    for (int i = 0; i < SAMPLES; i++) clock_gettime(CLOCK_MONOTONIC, &y);
    double cost_ts = (ts_ns(y) - ts_ns(x)) / SAMPLES;
    printf("clock_gettime: advertised resolution = %ld ns, smallest observed step = %.0f ns, "
           "cost per call = %.1f ns\n", res.tv_nsec, min_ts, cost_ts);
    return 0;
}
