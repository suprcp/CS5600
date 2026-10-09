// Q6: read an element after freeing the array
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *data = malloc(100 * sizeof(int));
    for (int i = 0; i < 100; i++) data[i] = i;
    free(data);
    printf("data[50] = %d\n", data[50]);   // use after free
    return 0;
}
