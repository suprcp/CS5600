// CS5600 HW7 - OSTEP Chapter 29: Lock-based Concurrent Data Structures
// Build: gcc -O2 -Wall -pthread -o ch29 ch29.c
// Usage:
//   ./ch29 timer                                   (Q1)
//   ./ch29 counter <threads> <loops>               (Q2) precise counter, one lock
//   ./ch29 approx  <threads> <loops> <S>           (Q3) approximate (sloppy) counter
//   ./ch29 list <single|hoh> <threads> <size> <ops> <work>   (Q4)
//   ./ch29 hash <single|bucket> <threads> <ops>    (Q5, Q6)
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAXT 64
#define MAXCPU 64

static double now(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec / 1e6;
}

typedef void *(*worker_fn)(void *);

// Spawn n threads running f(id), return elapsed seconds (create -> all joined).
static double run_threads(int n, worker_fn f) {
    pthread_t t[MAXT];
    double start = now();
    for (long i = 0; i < n; i++) pthread_create(&t[i], NULL, f, (void *)i);
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    return now() - start;
}

/* ===================== Q1: timer accuracy ===================== */
static void timer_test(void) {
    struct timeval a, b;
    long min_us = 1000000;
    for (int i = 0; i < 1000; i++) {
        gettimeofday(&a, NULL);
        do gettimeofday(&b, NULL);
        while (b.tv_sec == a.tv_sec && b.tv_usec == a.tv_usec);
        long d = (b.tv_sec - a.tv_sec) * 1000000L + (b.tv_usec - a.tv_usec);
        if (d < min_us) min_us = d;
    }
    int N = 1000000;
    double s = now();
    for (int i = 0; i < N; i++) gettimeofday(&a, NULL);
    double e = now();
    printf("gettimeofday: smallest observable change = %ld us\n", min_us);
    printf("gettimeofday: cost per call = %.1f ns\n", (e - s) / N * 1e9);

    struct timespec r, x, y;
    clock_getres(CLOCK_MONOTONIC, &r);
    long min_ns = 1000000000L;
    for (int i = 0; i < 1000; i++) {
        clock_gettime(CLOCK_MONOTONIC, &x);
        do clock_gettime(CLOCK_MONOTONIC, &y);
        while (y.tv_sec == x.tv_sec && y.tv_nsec == x.tv_nsec);
        long d = (y.tv_sec - x.tv_sec) * 1000000000L + (y.tv_nsec - x.tv_nsec);
        if (d < min_ns) min_ns = d;
    }
    printf("clock_gettime(MONOTONIC): reported resolution = %ld ns\n", r.tv_nsec);
    printf("clock_gettime(MONOTONIC): smallest observable change = %ld ns\n", min_ns);
    printf("CPUs online: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
}

/* ===================== Q2: precise counter (Fig 29.2) ===================== */
typedef struct { int value; pthread_mutex_t lock; } counter_t;
static counter_t C;
static long LOOPS;

static void *counter_worker(void *arg) {
    (void)arg;
    for (long i = 0; i < LOOPS; i++) {
        pthread_mutex_lock(&C.lock);
        C.value++;
        pthread_mutex_unlock(&C.lock);
    }
    return NULL;
}

/* ===================== Q3: approximate counter (Fig 29.4) ===================== */
// Each local counter is padded to its own cache line to avoid false sharing.
typedef struct { int value; pthread_mutex_t lock; char pad[64]; } local_t;
typedef struct {
    int global;
    pthread_mutex_t glock;
    local_t local[MAXCPU];
    int threshold;
    int ncpu;
} approx_t;
static approx_t A;

static void approx_init(approx_t *c, int threshold) {
    c->threshold = threshold;
    c->global = 0;
    c->ncpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (c->ncpu > MAXCPU) c->ncpu = MAXCPU;
    pthread_mutex_init(&c->glock, NULL);
    for (int i = 0; i < c->ncpu; i++) {
        c->local[i].value = 0;
        pthread_mutex_init(&c->local[i].lock, NULL);
    }
}

static void approx_update(approx_t *c, int tid, int amt) {
    int cpu = tid % c->ncpu;
    pthread_mutex_lock(&c->local[cpu].lock);
    c->local[cpu].value += amt;
    if (c->local[cpu].value >= c->threshold) {
        pthread_mutex_lock(&c->glock);
        c->global += c->local[cpu].value;
        pthread_mutex_unlock(&c->glock);
        c->local[cpu].value = 0;
    }
    pthread_mutex_unlock(&c->local[cpu].lock);
}

static int approx_get(approx_t *c) {
    pthread_mutex_lock(&c->glock);
    int v = c->global;
    pthread_mutex_unlock(&c->glock);
    return v;
}

static void *approx_worker(void *arg) {
    int id = (int)(long)arg;
    for (long i = 0; i < LOOPS; i++) approx_update(&A, id, 1);
    return NULL;
}

/* ===================== Q4: linked lists ===================== */
typedef struct node { int key; struct node *next; pthread_mutex_t lock; } node_t;

static int WORK;  // simulated per-node work (e.g. expensive key comparison)
static inline void node_work(void) {
    for (volatile int i = 0; i < WORK; i++) ;
}

// Standard list: one lock for the whole list (Fig 29.8)
typedef struct { node_t *head; pthread_mutex_t lock; } list_t;
static list_t SL;

static int slist_insert(list_t *L, int key) {
    node_t *n = malloc(sizeof(node_t));
    if (!n) return -1;
    n->key = key;
    pthread_mutex_lock(&L->lock);
    n->next = L->head;
    L->head = n;
    pthread_mutex_unlock(&L->lock);
    return 0;
}

static int slist_lookup(list_t *L, int key) {
    int rv = -1;
    pthread_mutex_lock(&L->lock);
    for (node_t *c = L->head; c; c = c->next) {
        node_work();
        if (c->key == key) { rv = 0; break; }
    }
    pthread_mutex_unlock(&L->lock);
    return rv;
}

// Hand-over-hand list: one lock per node, plus a dummy head node.
// Traversal grabs the next node's lock before releasing the current one.
typedef struct { node_t dummy; } hlist_t;
static hlist_t HL;

static int hlist_insert(hlist_t *L, int key) {
    node_t *n = malloc(sizeof(node_t));
    if (!n) return -1;
    n->key = key;
    pthread_mutex_init(&n->lock, NULL);
    pthread_mutex_lock(&L->dummy.lock);
    n->next = L->dummy.next;
    L->dummy.next = n;
    pthread_mutex_unlock(&L->dummy.lock);
    return 0;
}

static int hlist_lookup(hlist_t *L, int key) {
    node_t *prev = &L->dummy;
    pthread_mutex_lock(&prev->lock);
    node_t *curr = prev->next;
    while (curr) {
        pthread_mutex_lock(&curr->lock);    // grab next...
        pthread_mutex_unlock(&prev->lock);  // ...then release current
        node_work();
        if (curr->key == key) {
            pthread_mutex_unlock(&curr->lock);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    pthread_mutex_unlock(&prev->lock);
    return -1;
}

static int LIST_HOH, LIST_SIZE;
static long OPS;

static void *list_worker(void *arg) {
    unsigned int seed = (unsigned int)(long)arg * 7919 + 1;
    for (long i = 0; i < OPS; i++) {
        int key = rand_r(&seed) % LIST_SIZE;
        if (LIST_HOH) hlist_lookup(&HL, key);
        else slist_lookup(&SL, key);
    }
    return NULL;
}

/* ===================== Q5/Q6: hash table ===================== */
// Q5: one big lock for the whole table.  Q6: one lock per bucket.
#define BUCKETS 10007
typedef struct hnode { int key; struct hnode *next; } hnode_t;
typedef struct {
    hnode_t *heads[BUCKETS];
    pthread_mutex_t big_lock;
    pthread_mutex_t bucket_lock[BUCKETS];
    int per_bucket;
} hash_t;
static hash_t H;

static pthread_mutex_t *hash_lock_for(hash_t *h, int b) {
    return h->per_bucket ? &h->bucket_lock[b] : &h->big_lock;
}

static void hash_init(hash_t *h, int per_bucket) {
    memset(h->heads, 0, sizeof(h->heads));
    h->per_bucket = per_bucket;
    pthread_mutex_init(&h->big_lock, NULL);
    for (int i = 0; i < BUCKETS; i++) pthread_mutex_init(&h->bucket_lock[i], NULL);
}

static int hash_insert(hash_t *h, int key) {
    hnode_t *n = malloc(sizeof(hnode_t));
    if (!n) return -1;
    n->key = key;
    int b = key % BUCKETS;
    pthread_mutex_t *l = hash_lock_for(h, b);
    pthread_mutex_lock(l);
    n->next = h->heads[b];
    h->heads[b] = n;
    pthread_mutex_unlock(l);
    return 0;
}

static int hash_lookup(hash_t *h, int key) {
    int b = key % BUCKETS, rv = -1;
    pthread_mutex_t *l = hash_lock_for(h, b);
    pthread_mutex_lock(l);
    for (hnode_t *c = h->heads[b]; c; c = c->next)
        if (c->key == key) { rv = 0; break; }
    pthread_mutex_unlock(l);
    return rv;
}

#define HASH_PREFILL 10000
static void *hash_worker(void *arg) {
    long id = (long)arg;
    unsigned int seed = (unsigned int)id * 104729 + 3;
    int next_key = HASH_PREFILL + (int)id * 10000000;
    for (long i = 0; i < OPS; i++) {
        if (rand_r(&seed) % 10 == 0) hash_insert(&H, next_key++);  // 10% inserts
        else hash_lookup(&H, rand_r(&seed) % HASH_PREFILL);         // 90% lookups
    }
    return NULL;
}

/* ===================== main ===================== */
static void usage(void) {
    fprintf(stderr,
        "usage:\n"
        "  ./ch29 timer\n"
        "  ./ch29 counter <threads> <loops>\n"
        "  ./ch29 approx <threads> <loops> <S>\n"
        "  ./ch29 list <single|hoh> <threads> <size> <ops> <work>\n"
        "  ./ch29 hash <single|bucket> <threads> <ops>\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) usage();
    const char *mode = argv[1];

    if (!strcmp(mode, "timer")) {
        timer_test();
    } else if (!strcmp(mode, "counter") && argc == 4) {
        int t = atoi(argv[2]); LOOPS = atol(argv[3]);
        C.value = 0; pthread_mutex_init(&C.lock, NULL);
        double el = run_threads(t, counter_worker);
        printf("counter,%d,%ld,%.4f,final=%d\n", t, LOOPS, el, C.value);
    } else if (!strcmp(mode, "approx") && argc == 5) {
        int t = atoi(argv[2]); LOOPS = atol(argv[3]); int S = atoi(argv[4]);
        approx_init(&A, S);
        double el = run_threads(t, approx_worker);
        int exact = approx_get(&A);
        for (int i = 0; i < A.ncpu; i++) exact += A.local[i].value;
        printf("approx,%d,%ld,S=%d,%.4f,global=%d,exact=%d\n",
               t, LOOPS, S, el, approx_get(&A), exact);
    } else if (!strcmp(mode, "list") && argc == 7) {
        LIST_HOH = !strcmp(argv[2], "hoh");
        int t = atoi(argv[3]); LIST_SIZE = atoi(argv[4]);
        OPS = atol(argv[5]); WORK = atoi(argv[6]);
        SL.head = NULL; pthread_mutex_init(&SL.lock, NULL);
        HL.dummy.next = NULL; pthread_mutex_init(&HL.dummy.lock, NULL);
        for (int k = 0; k < LIST_SIZE; k++) {
            if (LIST_HOH) hlist_insert(&HL, k); else slist_insert(&SL, k);
        }
        double el = run_threads(t, list_worker);
        printf("list,%s,%d,size=%d,ops=%ld,work=%d,%.4f\n",
               argv[2], t, LIST_SIZE, OPS, WORK, el);
    } else if (!strcmp(mode, "hash") && argc == 5) {
        int per_bucket = !strcmp(argv[2], "bucket");
        int t = atoi(argv[3]); OPS = atol(argv[4]);
        hash_init(&H, per_bucket);
        for (int k = 0; k < HASH_PREFILL; k++) hash_insert(&H, k);
        double el = run_threads(t, hash_worker);
        printf("hash,%s,%d,ops=%ld,%.4f\n", argv[2], t, OPS, el);
    } else {
        usage();
    }
    return 0;
}
