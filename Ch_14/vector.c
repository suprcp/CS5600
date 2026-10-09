// Q8: realloc-based vector vs. linked list
// usage: ./vector [N]   (default N = 10,000,000)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int *data;
    size_t size;
    size_t cap;
} vector;

void vec_init(vector *v) { v->data = NULL; v->size = 0; v->cap = 0; }

void vec_push(vector *v, int x) {
    if (v->size == v->cap) {
        size_t newcap = v->cap ? v->cap * 2 : 1;      // double when full
        int *tmp = realloc(v->data, newcap * sizeof(int));
        if (!tmp) { perror("realloc"); exit(1); }    // don't lose old pointer
        v->data = tmp;
        v->cap = newcap;
    }
    v->data[v->size++] = x;
}

void vec_free(vector *v) { free(v->data); vec_init(v); }

typedef struct node { int val; struct node *next; } node;

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char *argv[]) {
    size_t n = (argc > 1) ? strtoul(argv[1], NULL, 10) : 10000000;
    long long sum;
    double t;

    // vector: append + traverse
    vector v; vec_init(&v);
    t = now();
    for (size_t i = 0; i < n; i++) vec_push(&v, (int)i);
    double vpush = now() - t;
    t = now(); sum = 0;
    for (size_t i = 0; i < v.size; i++) sum += v.data[i];
    double vscan = now() - t;
    printf("vector: push %.3fs  scan %.3fs  (sum %lld, cap %zu)\n",
           vpush, vscan, sum, v.cap);
    vec_free(&v);

    // linked list: append at tail + traverse
    node *head = NULL, *tail = NULL;
    t = now();
    for (size_t i = 0; i < n; i++) {
        node *nd = malloc(sizeof(node));
        if (!nd) { perror("malloc"); exit(1); }
        nd->val = (int)i; nd->next = NULL;
        if (tail) tail->next = nd; else head = nd;
        tail = nd;
    }
    double lpush = now() - t;
    t = now(); sum = 0;
    for (node *p = head; p; p = p->next) sum += p->val;
    double lscan = now() - t;
    printf("list:   push %.3fs  scan %.3fs  (sum %lld)\n", lpush, lscan, sum);
    while (head) { node *nx = head->next; free(head); head = nx; }

    return 0;
}
