// Q4: malloc without free
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *p = malloc(10 * sizeof(int));
    p[0] = 42;
    printf("p[0] = %d\n", p[0]);
    return 0;             // forgot free(p)
}
