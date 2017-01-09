#include "statistics.h"

void print_statistics(char *file_name, int *lines_processed, int N, int rank)
{
	if (rank == 0) {
		FILE *fp = fopen(file_name, "w");
		if (!fp) {
			printf("Error opening statistics file.");
			exit(1);
		}
		for (int i = 0; i < N; ++i) {
			fprintf(fp, "%d: %d\n", i, lines_processed[i]);
		}
		fclose(fp);
	}
}

/* Primeste mesajul de terminare si propaga statisticile */
void receive_term(int *new_count, int n, int source)
{
	int *recv_array = alloc_1d_int(n);
	MPI_Status status;

	MPI_Recv(&(recv_array[0]), n, MPI_INT, source, TERM_TAG, MPI_COMM_WORLD,
		 &status);
	for (int i = 0; i < n; ++i) {
		new_count[i] += recv_array[i];
	}
	dealloc_1d_int(recv_array);
}

/* Trimite mesajul de terminare si propaga statisticile */
void send_term(int destination, int *count, int n)
{
	MPI_Send(&(count[0]), n, MPI_INT, destination,
		 TERM_TAG, MPI_COMM_WORLD);
}

/* Propaga statisticile */
void get_statistics(int **adjacent_matrix, int N, int rank, int parent,
		    int *lines_processed)
{
	int child_nodes_size, no_echos = 0;
	int *child_nodes =
	    get_child_nodes(adjacent_matrix, N, rank, &child_nodes_size,
			    parent);

	if (rank != 0) {
		receive_term(lines_processed, N, parent);
	}

	for (int i = 0; i < child_nodes_size; ++i) {
		send_term(child_nodes[i], lines_processed, N);
		no_echos++;
	}

	while (no_echos > 0) {
		receive_term(lines_processed, N, MPI_ANY_SOURCE);
		no_echos--;
	}

	if (rank != 0) {
		send_term(parent, lines_processed, N);
	}
	dealloc_1d_int(child_nodes);
}
