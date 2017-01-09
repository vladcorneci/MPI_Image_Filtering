#include <mpi.h>
#include "my_alloc.h"

#define SONDAJ     1
#define ECHO       2
#define EMPTY_ECHO 3

void read_topology(char *file_name, int **adjacent_matrix, int rank);

void init_neighbors(int *a, int n);
int *get_neighbors(int **adjacent_matrix, int n, int rank, int *size);
int *get_child_nodes(int **adjacency_matrix, int n, int rank, int *size,
		     int parent);

int is_leaf(int **adjacent_matrix, int n, int rank);
int receive_sonda_ecou(int **new_adjacent_matrix, int n, int source,
		       int *msg_type);
void send_sonda_ecou(int type, int destination, int **adjacent_matrix, int n);
void find_topology(int **adjacent_matrix, int N, int rank, int **null_matrix,
		   int *parent);
void matrix_or(int **a, int **b, int n, int m);
