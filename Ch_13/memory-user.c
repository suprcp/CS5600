// memory-user.c -- OSTEP Ch.13 homework
// usage: ./memory-user <megabytes> [seconds]
// Allocates <megabytes> MB and keeps touching every int in the array,
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <megabytes> [seconds]\n", argv[0]);
        exit(1);
    }
    long mb = strtol(argv[1], NULL, 10);
    long secs = (argc == 3) ? strtol(argv[2], NULL, 10) : -1;
    if (mb <= 0) {
        fprintf(stderr, "megabytes must be > 0\n");
        exit(1);
    }

    size_t bytes = (size_t)mb * 1024 * 1024;
    size_t n = bytes / sizeof(int);

    int *arr = malloc(bytes);
    if (arr == NULL) {
        perror("malloc");
        exit(1);
    }
    printf("pid: %d  allocated %ld MB at %p\n", getpid(), mb, (void *)arr);
    fflush(stdout);

    time_t start = time(NULL);
    long passes = 0;
    unsigned long sum = 0;
    while (secs < 0 || time(NULL) - start < secs) {
        for (size_t i = 0; i < n; i++) {
            arr[i] += 1;          // touch every entry (read + write)
            sum += arr[i];
        }
        passes++;
    }
    // print sum so the compiler can't optimize the loop away
    printf("done: %ld passes, checksum %lu\n", passes, sum);
    free(arr);
    return 0;
}
