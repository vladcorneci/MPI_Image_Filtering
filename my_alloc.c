#include "my_alloc.h"

/*
 *	Aloca o zona de memorie continua de tip intreg de dimensiune n.
 *	Initializeaza aceasta zona cu 0.
*/
int *alloc_1d_int(int n)
{
	int *array = (int *)malloc(n * sizeof(int));
	memset(array, 0, n * sizeof(int));
}

/*
 *	Aloca o zona de memorie continua 2D de tip intreg de dimensiune rows * cols.
 *	Initializeaza aceasta zona cu 0.
 */
int **alloc_2d_int(int rows, int cols)
{
	int *data = (int *)malloc(rows * cols * sizeof(int));
	memset(data, 0, rows * cols * sizeof(int));
	int **array = (int **)malloc(rows * sizeof(int *));
	for (int i = 0; i < rows; i++)
		array[i] = &(data[cols * i]);
	return array;
}

void dealloc_1d_int(int *array)
{
	free(array);
}

void dealloc_2d_int(int **matrix)
{
	free(matrix[0]);
	free(matrix);
}
