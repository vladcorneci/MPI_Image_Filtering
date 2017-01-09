#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#include "topology.h"
#include "filters.h"

#define N 12
#define TERM_TAG 100

void read_topology(char * file_name, int **adjacent_matrix, int rank)
{
  const char s[4] = ": \n";
  char *token;
  FILE * fp = fopen(file_name, "r");
  char * line = NULL;
  size_t len = 0;
  int current_line = -1, buff;
  if (!fp) {
    printf("Topology file error.\n");
    exit(1);
  }

  while (getline(&line, &len, fp) != -1) {
    int is_first = 1;
    /* get the first token */
    token = strtok(line, s);
    /* walk through other tokens */
    while(token != NULL) {
      if (is_first) {
        current_line = atoi(token);
        if (current_line != rank)
          break;
        is_first = 0;
      } else {
        buff = atoi(token);
        adjacent_matrix[current_line][buff] = 1;
      }
      token = strtok(NULL, s);
    }
  }
}

enum filter_types check_type(char * str_type)
{
  if (!strcmp(str_type, "sharpen")) {
    return SHARPEN;
  } else if (!strcmp(str_type, "smooth")) {
    return SMOOTH;
  } else if (!strcmp(str_type, "mean_removal")) {
    return MEAN_REMOVAL;
  } else if (!strcmp(str_type, "blur")) {
    return BLUR;
  }
  return SHARPEN;
}

int * process_images (char * images_file, int ** adjacent_matrix, int dim,
  int rank, int parent) {
  int n;
  int * lines_processed = alloc_1d_int(dim);
  FILE * fp = fopen(images_file, "r");
  char filter_type_str[255], file_in[255], file_out[255];

  if (!fp) {
    printf("Images file error.\n");
    exit(1);
  }

  fscanf(fp, "%d\n", &n);
  for (int i = 0; i < n; ++i) {
    fscanf(fp, "%s", filter_type_str);
    fscanf(fp, "%s", file_in);
    fscanf(fp, "%s", file_out);
    enum filter_types filter = check_type(&(filter_type_str[0]));
    apply_filter(adjacent_matrix, dim, rank, parent,
      file_in, file_out, filter, lines_processed);
  }
  return lines_processed;
}

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

  read_topology(argv[1], adjacent_matrix, rank);
  find_topology(adjacent_matrix, N, rank, null_matrix, &parent);
  int * lines_processed = process_images(argv[2], adjacent_matrix, N, rank, parent);

  /* Get statistics */
  int child_nodes_size, no_echos = 0;
  int * child_nodes =
    get_child_nodes(adjacent_matrix, N, rank, &child_nodes_size, parent);

  if (rank != 0) {
    receive_term(lines_processed, N, parent);
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


  dealloc_1d_int(lines_processed);
  dealloc_2d_int(adjacent_matrix);
  dealloc_2d_int(null_matrix);

  // parent must have the solution

  MPI_Finalize();
  return 0;
}
