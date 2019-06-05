#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

int main(int argc, char** argv) {
	int opt;
	int length;
	char *charset;
	char *hash;

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
  
	while ((opt = getopt(argc, argv, "l:c:")) != -1){
		switch (opt){
		case 'l':
			length = atoi(optarg);
			break;
		case 'c':
			charset = malloc(strlen(optarg));
			strcpy(charset, optarg);
			break;
		case ':':
			break;
		case '?' :
			printf("unknown option: %c\n", optopt);
			break;
		}
	}
	hash = malloc(strlen(argv[optind]));
	strcpy(hash, argv[optind]);

	printf("Starting Compute for Hash '%s' with Charset'%c' for passwords with max length '%d' on Node %s\n", hash, charset, length, world_rank);

	// Finalize the MPI environment.
	MPI_Finalize();

	free(charset);
	free(hash);
}