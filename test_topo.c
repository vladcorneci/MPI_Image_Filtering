#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#include "topology.h"
#include "filters.h"

#define N 8

#define TERM_TAG 100

// int adiacent_matrix[N][N] = {
//   {0, 1, 1, 0, 0, 0 ,0, 1}, // 0
//   {1, 0, 0, 1, 1, 1, 0, 0}, // 1
//   {1, 0, 0, 0, 0, 0, 1, 0}, // 2
//   {0, 1, 0, 0, 0, 0, 0, 0}, // 3
//   {0, 1, 0, 0, 0, 1, 0, 0}, // 4
//   {0, 1, 0, 0, 1, 0, 0, 0}, // 5
//   {0, 0, 1, 0, 0, 0, 0, 0}, // 6
//   {1, 0, 0, 0, 0, 0, 0, 0}  // 7
// };

int adiacent_matrix[N][N] = {
  {0, 1, 0, 0, 0, 0 ,0, 0}, // 0
  {1, 0, 0, 1, 0, 0, 0, 0}, // 1
  {0, 0, 0, 0, 1, 0, 0, 0}, // 2
  {0, 1, 0, 0, 1, 0, 0, 0}, // 3
  {0, 0, 1, 1, 0, 1, 1, 1}, // 4
  {0, 0, 0, 0, 1, 0, 0, 0}, // 5
  {0, 0, 0, 0, 1, 0, 0, 0}, // 6
  {0, 0, 0, 0, 1, 0, 0, 0}  // 7
};

void receive_term(int * new_count, int n, int source)
{
  int * recv_array = alloc_1d_int(n);
  MPI_Status status;

  MPI_Recv(&(recv_array[0]), n, MPI_INT, source, TERM_TAG, MPI_COMM_WORLD, &status);
  for (int i = 0; i < n; ++i) {
    new_count[i] += recv_array[i];
  }
  dealloc_1d_int(recv_array);
}

void send_term(int destination, int * count, int n)
{
  MPI_Send(&(count[0]), n, MPI_INT, destination,
          TERM_TAG, MPI_COMM_WORLD);
}

int main(int argc, char * argv[])
{
  int rank, nProcesses;
  int parent = -1, i, j;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
  int ** adjacent_matrix = alloc_2d_int(N, N);
  int ** null_matrix = alloc_2d_int(N, N);


  for (i = 0; i < N; ++i) {
    for (j = 0; j < N; ++j) {
      if (i == rank) adjacent_matrix[i][j] = adiacent_matrix[i][j];
      else adjacent_matrix[i][j] = 0;

      null_matrix[i][j] = 0;
    }
  }

  find_topology(adjacent_matrix, N, rank, null_matrix, &parent);
  int * lines_processed = alloc_1d_int(N);
  {
    enum filter_types filter = MEAN_REMOVAL;
    char * input_image  = "pics/mario.pgm";
    char * output_image = "output.pgm";
    apply_filter(adjacent_matrix, N, rank, parent,
      input_image, output_image, filter, lines_processed);
  }
  {
    enum filter_types filter = MEAN_REMOVAL;
    char * input_image  = "pics/vled.pgm";
    char * output_image = "output2.pgm";
    apply_filter(adjacent_matrix, N, rank, parent,
      input_image, output_image, filter, lines_processed);
  }
  {
    enum filter_types filter = MEAN_REMOVAL;
    char * input_image  = "pics/image1.pgm";
    char * output_image = "output3.pgm";
    apply_filter(adjacent_matrix, N, rank, parent,
      input_image, output_image, filter, lines_processed);
  }


  // int neighbors_size, no_echos = 0;
  // int * neighbors = get_neighbors(adjacent_matrix, N, rank, &neighbors_size);
  //

  int child_nodes_size, no_echos = 0;
  int * child_nodes =
    get_child_nodes(adjacent_matrix, N, rank, &child_nodes_size, parent);
  if (rank != 0) {
    receive_term(lines_processed, N, parent);
  //   int tag;
  //   parent = receive_sonda_ecou(adjacent_matrix, N, MPI_ANY_SOURCE, &tag);
  }


  for (int i = 0; i < child_nodes_size; ++i) {
    send_term(child_nodes[i], lines_processed, N);
    no_echos ++;
  }

  while (no_echos > 0) {
    receive_term(lines_processed, N, MPI_ANY_SOURCE);
    no_echos--;
  }

  if (rank != 0) {
    send_term(parent, lines_processed, N);
  }

  if (rank == 0) {
    int sum = 0;
    for (int i = 0; i < N; ++i) {
      sum += lines_processed[i];
      printf("%d: %d\n", i, lines_processed[i]);
    }
    printf("Total sum: %d\n", sum);
  }


  //
  // // Send sondaj
  // for (i = 0; i < neighbors_size; ++i) {
  //   if (neighbors[i] != parent) {
  //     send_sonda_ecou(SONDAJ, neighbors[i], null_matrix, N);
  //     no_echos ++;
  //   }
  // }
  //
  // // Wait for echos
  // while (no_echos > 0) {
  //   int tag;
  //   int source = receive_sonda_ecou(adjacent_matrix, N, MPI_ANY_SOURCE, &tag);
  //   if (tag == EMPTY_ECHO) {
  //     adjacent_matrix[rank][source] = 0;
  //   }
  //   if (tag == SONDAJ) {
  //     send_sonda_ecou(EMPTY_ECHO, source, null_matrix, N);
  //   } else {
  //     no_echos--;
  //   }
  // }
  //
  // // Propaga in sus
  // if (rank != 0) {
  //   send_sonda_ecou(ECHO, parent, adjacent_matrix, N);
  // }



  dealloc_1d_int(lines_processed);
  dealloc_2d_int(adjacent_matrix);
  dealloc_2d_int(null_matrix);

  // parent must have the solution


  // Send images

  // if (p <> inițiator)
  //     receive sondă-ecou[p](k, top_nou);
  //     mark as  parent source
  //
  //  /* transmite sondaje celorlalte noduri vecine, copiii lui p */
  //  for [q = 1 to N st (leg[q] AND (q <> părinte))] {
  //     send sondă-ecou[q](sondă, p, O);  /* topologie nulă */
  //     nr_ecouri = nr_ecouri + 1;
  //  }
  // while (nr_ecouri > 0) {
  //     receive sondă-ecou[p](k, transm, top_nou);
  //
  ////// fara sonda
  //     if (k == sondă)
  //        send sondă-ecou[transm](ecou, p, O);
  //     else if (k == ecou) {
  //        top = top OR top_nou;
  //        nr_ecouri = nr_ecouri – 1;
  //     }
  //  }
  //
  //  if (p <> inițiator)
  //     send sondă-ecou[părinte](ecou, p, top);

  // if (rank == 0) {
  //   for (i = 0; i < N; ++i) {
  //     printf("Leaf:%d, %d : ",is_leaf(adjacent_matrix, N, i), i);
  //     for (j = 0; j < N; ++j) {
  //       if (adjacent_matrix[i][j] != adiacent_matrix[i][j]) {
  //         // printf("error\n" );
  //       }
  //       if (adjacent_matrix[i][j]) {
  //         printf("%d ", j);
  //       } else {
  //         // printf("%d ", adjacent_matrix[i][j]);
  //       }
  //     }
  //     printf("\n" );
  //   }
  // }

  MPI_Finalize();
  return 0;
}
