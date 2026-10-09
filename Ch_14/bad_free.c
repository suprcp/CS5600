// Q7: pass a pointer into the middle of the block to free()
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *data = malloc(100 * sizeof(int));
    free(data + 50);      // not a pointer returned by malloc
    printf("survived?\n");
    return 0;
}
