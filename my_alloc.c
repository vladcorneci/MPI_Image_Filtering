#include "my_alloc.h"

int *alloc_1d_int(int n)
{
  int * array = (int *)malloc(n * sizeof(int));
  memset(array, 0, n * sizeof(int));
}

int **alloc_2d_int(int rows, int cols)
{
    int *data = (int *)malloc(rows*cols*sizeof(int));
    int **array= (int **)malloc(rows*sizeof(int*));
    for (int i=0; i<rows; i++)
        array[i] = &(data[cols*i]);

    return array;
}

char **alloc_2d_char(int rows, int cols)
{
    char *data = (char *)malloc(rows*cols*sizeof(char));
    char **array= (char **)malloc(rows*sizeof(char*));
    for (int i=0; i<rows; i++)
        array[i] = &(data[cols*i]);

    return array;
}

void dealloc_1d_int(int * array)
{
  free(array);
}

void dealloc_2d_int(int ** matrix)
{
    free(matrix[0]);
    free(matrix);
}
