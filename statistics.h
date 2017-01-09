#include <mpi.h>
#include <stdio.h>
#include "my_alloc.h"
#include "topology.h"


#define TERM_TAG 100	/* Tag pentru mesajul de terminare */


void print_statistics(char *file_name, int *lines_processed, int N, int rank);
void receive_term(int *new_count, int n, int source);
void send_term(int destination, int *count, int n);
void get_statistics(int **adjacent_matrix, int N, int rank, int parent,
		    int *lines_processed);
