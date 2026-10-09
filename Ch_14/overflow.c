// Q5: write one past the end of a malloc'd array
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *data = malloc(100 * sizeof(int));
    data[100] = 0;        // valid indices are 0..99
    printf("wrote data[100]\n");
    free(data);
    return 0;
}
