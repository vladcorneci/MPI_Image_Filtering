#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#include "topology.h"
#include "filters.h"
#include "statistics.h"

int main(int argc, char *argv[])
{
	int rank, N;
	int parent = -1;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &N);

	/* Verifica numarul de argumente */
	if (argc < 3) {
		if (rank == 0) {
			printf
			    ("Usage:\nmpirun -np N %s <topology.in> <images.in> <statistics.out>\n",
			     argv[0]);
		}
		exit(1);
	}
	int **adjacent_matrix = alloc_2d_int(N, N);
	int **null_matrix = alloc_2d_int(N, N);

	/* Citeste lista de adiacenta */
	read_topology(argv[1], adjacent_matrix, rank);
	/* Stabileste arborele de acoperire */
	find_topology(adjacent_matrix, N, rank, null_matrix, &parent);

	/* Prelucreaza imaginile din fisierul desemnat de al 2-lea argument */
	int *lines_processed =
	    process_images(argv[2], adjacent_matrix, N, rank, parent);

	/* Propaga statisticile */
	get_statistics(adjacent_matrix, N, rank, parent, lines_processed);
	/* Scrie statisticile in fisierul desemnat de cel de-al 3-lea argument */
	print_statistics(argv[3], lines_processed, N, rank);

	dealloc_1d_int(lines_processed);
	dealloc_2d_int(adjacent_matrix);
	dealloc_2d_int(null_matrix);

	MPI_Finalize();
	return 0;
}
