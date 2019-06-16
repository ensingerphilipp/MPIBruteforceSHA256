#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <openssl/sha.h>
#include <assert.h>
#include <time.h>

int i = 0;
int sendFlag = -1;
int recvFlag = -1;
int bufferCount = 1;
MPI_Status recvStatus;
MPI_Request recvRequest = MPI_REQUEST_NULL;
MPI_Status sendStatus;
MPI_Request sendRequest = MPI_REQUEST_NULL;
int sendComplete = 0;
int recvComplete = 0;

/*
	Function to convert the input Hash String to an unsigned char Byte Array
*/

int hexToBytes(const char* hex, unsigned char* bytes, unsigned int size, unsigned int* convertLen) {
	char c;
	int i, len;
	unsigned char val[2];
	if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
		hex += 2;
	}

	len = 0;
	while (size > 0 && *hex != '\0') {
		for (i = 0; i < 2; i++) {
			c = *hex++;
			if (c >= '0' && c <= '9')
				val[i] = c - '0';
			else if (c >= 'a' && c <= 'f')
				val[i] = 10 + c - 'a';
			else if (c >= 'A' && c <= 'F')
				val[i] = 10 + c - 'A';
			else
				return -1;
		}
		*bytes = (val[0] << 4) | val[1];
		len++;
		bytes++;
		size--;
	}

	if (convertLen != NULL) {
		*convertLen = len;
	}
	return 0;
}

void bruteForceSha256(char* charset, char* splitCharset, unsigned char* hashHex, int maxLength, int world_rank) {
	char* charsetBeginPtr = charset;
	char* splitCharsetBeginPtr = splitCharset;
	char* charsetEndPtr = charsetBeginPtr + strlen(charset);
	char* splitCharsetEndPtr = splitCharsetBeginPtr + strlen(splitCharset);
	char** arrayOfCharsets = (char**)malloc(sizeof(char*) * (maxLength + 1));
	char* passwordString = malloc(maxLength + 1);
	unsigned char* hashHexBeginPtr = hashHex;
	int currentLength = 1;
	int counter = 0;
	clock_t t;
	t = clock();
	
	/*
		Loop while length of arrayOfCharsets is <= the maximum password length specified
	*/

	while (currentLength <= maxLength) {

		/*
			While the end of the Charset is not reached -- go through the charset by incrementing the pointer and thus forming
			a new string every time --> then create the String and generate the corresponding hash and try check if the hash equals
			the hash that should be cracked.
		*/
		while (charsetBeginPtr < charsetEndPtr) {
			arrayOfCharsets[currentLength - 1] = charsetBeginPtr++;
			for (i = 0; i < currentLength; i++) {
				passwordString[i] = *arrayOfCharsets[i];
			}
			passwordString[i] = '\0';

			unsigned char genHash[SHA256_DIGEST_LENGTH];
			SHA256_CTX sha256;
			SHA256_Init(&sha256);
			SHA256_Update(&sha256, passwordString, currentLength);
			SHA256_Final(genHash, &sha256);

				for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
					if (genHash[i] != *hashHex) break;
					hashHex++;
				}
				if (i == SHA256_DIGEST_LENGTH) {
					t = clock() - t;
					double time_taken = ((double)t) / CLOCKS_PER_SEC; 
					printf("\n===================================================\n");
					printf("Node %d: Password was found: %s\n", world_rank, passwordString);
					printf("Node %d: It took %f seconds to calculate.\n", world_rank, time_taken);
					printf("=====================================================\n\n");
					sendFlag = world_rank;
					MPI_Send(&sendFlag, bufferCount, MPI_INT, 0, 0, MPI_COMM_WORLD);
					free(arrayOfCharsets);
					return;
				}
			hashHex = hashHexBeginPtr;
		}

		/*Check MPI Communication if Receive was complete --> this is only important for the Master Node */

		if (!recvComplete) {
			MPI_Test(&recvRequest, &recvComplete, &recvStatus);
		} else {
			free(arrayOfCharsets);
			return;
		}

		/*
			For all charsets --> if splitCharset on position 0 has reached the end or any other charset has reached the end -->
			increment the counter which is used to determine if the array has to be extended
		*/

		for (i = currentLength - 1, counter = 0; i >= 0; i--) {
			if (*arrayOfCharsets[i] == *(charsetEndPtr - 1)) {
				counter++;
			}
			else if (i == 0 && *arrayOfCharsets[i] == *(splitCharsetEndPtr - 1)) {
				counter++;
			}
			else break;
		}

		/*
			When the counter is != currentLength --> increment the charset at position currentLength - counter - 1 --> e.g. move from a to b
			and reset all other charsets to start from the beginning

			else

			every character has reached the end of the charset -->
			reset the complete Array Of Charsets and increase Length by 1
		*/
		

		if (counter != currentLength) {
			if (!arrayOfCharsets[currentLength - counter - 1]++) {
				printf("arrayOfCharsets on Position %d was null when trying to access and increment\n", currentLength - counter - 1);
			}
			for (i = currentLength - 1; i >= currentLength - counter; i--) {
				arrayOfCharsets[i] = charset;
			}
		} else {
			arrayOfCharsets[0] = splitCharset;
			for (int i = 1; i <= currentLength; i++) {
				arrayOfCharsets[i] = charset;
			}
			currentLength++;
		}
		charsetBeginPtr = charset;
	}
	free(arrayOfCharsets);
}

/*
	Function for splitting workloads equally across all nodes
*/

char* splitCharsetFunc(char* charset, int world_rank, int world_size) {
	int intervall = (strlen(charset) / world_size);
	int offset = intervall * world_rank;
	int rest = strlen(charset) % world_size;

	if (rest != 0) {
		for (int i = 0; i <= rest; i++) {
			if (world_size % (world_rank + 1) == i) {
				intervall++;
				offset++;
			}
		}
		if (world_rank == 0) {
			offset--;
		}
	}
	char* splitCharset = malloc(intervall);
	int i;
	for (i = offset; i < offset + intervall; i++) {
		splitCharset[i - offset] = charset[i];
	}
	splitCharset[i - offset] = '\0';
	return splitCharset;
}

int main(int argc, char** argv) {
	int opt;
	int length;
	char* splitCharset;
	char* charset;
	char* hashString;
	unsigned char* hashHex;

	/*
		Set up the MPI environment and all necessary information:
		name of processor, world_rank and world_size
	*/

	MPI_Init(NULL, NULL);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);

	/*
		Get all arguments from commandline and save to variables
	*/

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
	hashString = calloc(1, strlen(argv[optind]));
	strcpy(hashString, argv[optind]);
	hashHex = calloc(1, strlen(hashString) / 2);
	hexToBytes(hashString, hashHex, strlen(hashString), NULL);
	
	/*
		Split the charset into parts to equally set the work loads
	*/
	splitCharset = malloc(strlen(splitCharsetFunc(charset, world_rank, world_size)));
	splitCharset = splitCharsetFunc(charset, world_rank, world_size);

	printf("Starting Compute for Hash ");
		for (int i = 0; i < strlen(hashString) / 2; i++)
		{
			printf("%02hx", hashHex[i]);
		}
	printf("on Node %d\n", world_rank);

	/*
		Set up an MPI Receiver for Communication and call the bruteForce Function
	*/
	MPI_Irecv(&recvFlag, bufferCount, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &recvRequest);
	bruteForceSha256(charset, splitCharset, hashHex, length, world_rank);
	
	/*
		After finishing the bruteForce Workload if the Process is the Master it distributes the results to the Nodes 
		while the Nodes listen for Information. This is a synchronized task --> all process have to "hit" MPI_BARRIER first
	*/
		if (world_rank == 0) {
		MPI_Test(&recvRequest, &recvComplete, &recvStatus);
			if (recvFlag == -1 && sendFlag == -1) {
				printf("Master: Waiting for other Nodes to finish.\n");
				MPI_Barrier(MPI_COMM_WORLD);
				MPI_Test(&recvRequest, &recvComplete, &recvStatus);
				if (recvComplete == 0) {
					printf("Master: Password could not be found.\n");
					MPI_Bcast(&recvFlag, bufferCount, MPI_INT, 0, MPI_COMM_WORLD);
				}
			}
			if (recvFlag != -1) {
				printf("Master: Password was found by Node %d.\n", recvFlag);
				printf("Master: Waiting for other Nodes to finish.\n");
				MPI_Barrier(MPI_COMM_WORLD);
				MPI_Bcast(&recvFlag, bufferCount, MPI_INT, 0, MPI_COMM_WORLD);
			}
		}
		if (world_rank != 0) {
			MPI_Barrier(MPI_COMM_WORLD);
			MPI_Bcast(&recvFlag, bufferCount, MPI_INT, 0, MPI_COMM_WORLD);
			if (recvFlag == -1) {
				// Finalize the MPI environment.
				free(charset);
				free(splitCharset);
				free(hashHex);
				free(hashString);
				MPI_Finalize();
				return 0;
			}
			if (recvFlag != -1 && recvFlag != sendFlag) {
				printf("Node %d: Password was found by another Node (%d).\n", world_rank, recvFlag);
			}
		}
	// Finalize the MPI environment.
	free(charset);
	free(splitCharset);
	free(hashHex);
	free(hashString);
	MPI_Finalize();
}