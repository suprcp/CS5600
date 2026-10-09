#include <stdio.h>
#include <stdlib.h>

int main() {
    int *arr = malloc(10 * sizeof(int));
    if (arr == NULL) return 1;

    for (int i = 0; i < 10; i++) {
        arr[i] = i + 1;
    }

    int *new_arr = realloc(arr, 3 * sizeof(int));
    if (new_arr == NULL) {
        free(arr);
        return 1;
    }
    arr = new_arr;

    printf("Value at index 5: %d\n", arr[5]);

    free(arr);
    return 0;
}
