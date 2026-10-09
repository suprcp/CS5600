// Q1-3: dereference a NULL pointer
#include <stdio.h>

int main(void) {
    int *p = NULL;
    printf("%d\n", *p);   // crash here
    return 0;
}
