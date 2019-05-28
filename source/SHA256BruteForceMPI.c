#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <getopt.h>

int main(int argc, char** argv) {
	int opt;
  
	while ((opt = getopt(argc, argv, ":c:l:")) != -1){
		switch (opt){
		case 'l':
			printf("length: %s\n", optarg);
			break;
		case 'c':
			printf("charset: %s\n", optarg);
			break;
		case ':':
			printf("option needs a value\n");
			break;
		case '?' :
			printf("unknown option: %c\n", optopt);
			break;
		}
			printf("Submitted Hash: %s\n", argv[optind]);
	}
	// Initialize the MPI environment
	MPI_Init(NULL, NULL);

	// Get the number of processes
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Get the rank of the process
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	// Get the name of the processor
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);

	// Print off a hello world message
	printf("SHA256BruteForce HELLO from processor %s, rank %d"
		" out of %d processors\n", processor_name, world_rank, world_size);

	// Finalize the MPI environment.
	MPI_Finalize();
}