#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


int rank, size; // comm rank & size
int seed; // for random number
int **matrix; // communication matrix: matrix[src,dest] = amount [KB]
int min_range, max_range; // range for communication amount [KB]
int max_peers; // maxmum number of peers / rank

void init_matrix() {
  matrix = (int **)malloc(size * sizeof(int *));
  if (matrix == NULL) {
    fprintf(stderr, "Failed to allocate memory for matrix rows\n");
    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
  }
  
  for (int i = 0; i < size; i++) {
    matrix[i] = (int *)malloc(size * sizeof(int));
    if (matrix[i] == NULL) {
      fprintf(stderr, "Failed to allocate memory for matrix columns\n");
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    memset(matrix[i], 0, size * sizeof(int));
  }
}

int non_zero_count_row(int i) {
  int count = 0;
  for (int j = 0; j < size; ++j) {
    if (matrix[i][j] != 0) count++;
  }
  return count;
}
int non_zero_count_col(int j) {
  int count = 0;
  for (int i = 0; i < size; ++i) {
    if (matrix[i][j] != 0) count++;
  }
  return count;
}

void check_avail_col(int i, int *num_avail_cols, int *avail_col) {
  int list_index = 0;
  *num_avail_cols = size;
  for (int j = 0; j < size; ++j) {
    if (i == j) {
      (*num_avail_cols)--;
      } else if (matrix[i][j] != 0) {
      (*num_avail_cols)--;
    } else {
      if (non_zero_count_col(j) < max_peers) {
        avail_col[list_index++] = j;
      } else {
        (*num_avail_cols)--;
      }
    }
  }
}

void generate_symmetric_matrix() {
  int num_avail_cols;
  int *avail_col = (int *)malloc(size * sizeof(int));
  for (int i = 0; i < size; ++i) {
    int count_i = non_zero_count_row(i);
    while (count_i < max_peers) {
      check_avail_col(i, &num_avail_cols, avail_col);
      if (num_avail_cols  == 0) {
	fprintf(stderr, "WARN: num_avail_cols is not enough at row=%d\n", i);
	break;
      } else {
	int pos = avail_col[rand() % num_avail_cols];
	int value = rand() % (max_range - min_range + 1) + min_range;
	matrix[i][pos] = value;
	matrix[pos][i] = value;
      }
      count_i = non_zero_count_row(i);
    }
  }
}

void print_matrix() {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            printf("%3d ", matrix[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 5) {
    if (rank == 0) {
      fprintf(stderr, "Usage: %s <max_peers> <min_range> <max_range> <random_number_seed>\n", argv[0]);
    }
    MPI_Finalize();
    return EXIT_FAILURE;
  }


  max_peers = atoi(argv[1]);
  min_range = atoi(argv[2]);
  max_range = atoi(argv[3]);
  seed = atoi(argv[4]);

  
  srand(seed);  // with fixd the random seed for reproducibility
  init_matrix();
  generate_symmetric_matrix();
  printf("Communication matrix for random P2P communications:\n");
  print_matrix();
  
  MPI_Request request[2 * max_peers];
  MPI_Status status[2 * max_peers];
  char *send_buffer[max_peers];
  char *recv_buffer[max_peers];
  int data_size[max_peers];
  int srcdest[max_peers];
  int comm_count=0;
  
  for (int i = 0; i < size; i++) {
    //        src    dest
    if (matrix[rank][i] != 0) {
      data_size[comm_count] = matrix[rank][i] * 1024;
      send_buffer[comm_count] = (char*)malloc(data_size[comm_count]);
      recv_buffer[comm_count] = (char*)malloc(data_size[comm_count]);
      srcdest[comm_count] = i;
      comm_count++;
    }
  }
  

  struct timeval start_time, end_time;
  long long elapsed_time;
  gettimeofday(&start_time, NULL);
  
  for (int loop = 0; loop < 100; loop++) {
    for (int i = 0; i < comm_count; i++) {
      MPI_Isend(send_buffer[i], data_size[i], MPI_CHAR, srcdest[i], 0, MPI_COMM_WORLD, &request[i]);
      MPI_Irecv(recv_buffer[i], data_size[i], MPI_CHAR, srcdest[i], 0, MPI_COMM_WORLD, &request[i+comm_count]);
    }
    MPI_Waitall(2*comm_count, request, status);
  }

  gettimeofday(&end_time, NULL);
  elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000LL +
                 (end_time.tv_usec - start_time.tv_usec);
  printf("Elapsed time: %lld microseconds\n", elapsed_time);


  
  MPI_Finalize();  
  return 0;
}
