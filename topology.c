#include "topology.h"
#include <stdio.h>

/*
 *  Citeste topologia din fisierul 'file_name' si initilizeaza matricea de
 * adiacaneta.
 */
void read_topology(char *file_name, int **adjacent_matrix, int rank)
{
	const char s[4] = ": \n";
	char *token;
	FILE *fp = fopen(file_name, "r");
	char *line = NULL;
	size_t len = 0;
	int current_line = -1, buff;
	if (!fp) {
		printf("Topology file error.\n");
		exit(1);
	}

	while (getline(&line, &len, fp) != -1) {
		int is_first = 1;
		token = strtok(line, s);
		while (token != NULL) {
			if (is_first) {
				is_first = 0;
				current_line = atoi(token);
				if (current_line != rank) {
					break;
				}
			} else {
				buff = atoi(token);
				adjacent_matrix[current_line][buff] = 1;
			}
			token = strtok(NULL, s);
		}
	}
	fclose(fp);
}

void init_neighbors(int *a, int n)
{
	int i;
	for (i = 0; i < n; ++i)
		a[i] = -1;
}

/*
 *	Pe baza matricei de adiacenta returneaza un vector ce contine vecinii unui
 * nod si dimensiunea acestuia.
 */
int *get_neighbors(int **adjacent_matrix, int n, int rank, int *size)
{
	int k = 0;
	*size = 0;
	int *neighbors = (int *)malloc(n * 4);
	init_neighbors(neighbors, n);
	for (int j = 0; j < n; ++j) {
		if (adjacent_matrix[rank][j]) {
			neighbors[k++] = j;
			(*size)++;
		}
	}
	return neighbors;
}

/*
 *	Pe baza matricei de adiacenta returneaza un vector ce contine copiii unui
 * nod si dimensiunea acestuia.
 */
int *get_child_nodes(int **adjacency_matrix, int n, int rank, int *size,
		     int parent)
{
	int j, k = 0;
	*size = 0;
	int *neighbors = (int *)malloc(n * 4);
	init_neighbors(neighbors, n);
	for (j = 0; j < n; ++j) {
		if (adjacency_matrix[rank][j] && j != parent) {
			neighbors[k++] = j;
			(*size)++;
		}
	}
	return neighbors;
}

/* Verifica daca nodul primit ca argument este frunza */
int is_leaf(int **adjacency_matrix, int n, int rank)
{
	int i, leaf = 0;
	for (i = 0; i < n; ++i) {
		if (adjacency_matrix[rank][i]) {
			leaf++;
		}
	}
	return leaf <= 1;
}

int receive_sonda_ecou(int **new_adjacent_matrix, int n, int source,
		       int *msg_type)
{
	int **recv_matrix = alloc_2d_int(n, n);
	MPI_Status status;

	MPI_Recv(&(recv_matrix[0][0]), n * n, MPI_INT, source,
		 MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	matrix_or(new_adjacent_matrix, recv_matrix, n, n);
	*msg_type = status.MPI_TAG;
	dealloc_2d_int(recv_matrix);
	return status.MPI_SOURCE;
}

void send_sonda_ecou(int type, int destination, int **adjacent_matrix, int n)
{
	MPI_Send(&(adjacent_matrix[0][0]), n * n, MPI_INT, destination,
		 type, MPI_COMM_WORLD);
}

/* Pe baza matricei de adiacenta gaseste arborele de acoperire */
void find_topology(int **adjacent_matrix, int N, int rank, int **null_matrix,
		   int *parent)
{
	int neighbors_size, no_echos = 0, i;
	int *neighbors =
	    get_neighbors(adjacent_matrix, N, rank, &neighbors_size);

	if (rank != 0) {
		int tag;
		*parent =
		    receive_sonda_ecou(adjacent_matrix, N, MPI_ANY_SOURCE,
				       &tag);
	}

	// Send sondaj
	for (i = 0; i < neighbors_size; ++i) {
		if (neighbors[i] != *parent) {
			send_sonda_ecou(SONDAJ, neighbors[i], null_matrix, N);
			no_echos++;
		}
	}

	// Wait for echos
	while (no_echos > 0) {
		int tag;
		int source =
		    receive_sonda_ecou(adjacent_matrix, N, MPI_ANY_SOURCE,
				       &tag);
		if (tag == EMPTY_ECHO) {
			adjacent_matrix[rank][source] = 0;
		}
		if (tag == SONDAJ) {
			send_sonda_ecou(EMPTY_ECHO, source, null_matrix, N);
		} else {
			no_echos--;
		}
	}

	// Propaga in sus
	if (rank != 0) {
		send_sonda_ecou(ECHO, *parent, adjacent_matrix, N);
	}
	free(neighbors);
}

void matrix_or(int **a, int **b, int n, int m)
{
	int i, j;
	for (i = 0; i < n; ++i) {
		for (j = 0; j < m; ++j) {
			a[i][j] |= b[i][j];
		}
	}
}
