#include <stdlib.h>
#include <string.h>

int *alloc_1d_int(int n);
int **alloc_2d_int(int rows, int cols);

void dealloc_1d_int(int *array);
void dealloc_2d_int(int **matrix);
