#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <openssl/sha.h>
#include <assert.h>

char* splitCharsetFunc(char* charset, int world_rank, int world_size);
void bruteForceSha256(char* charset, char* splitCharset, unsigned char* hashHex, int maxLength, int world_rank);
int hexToBytes(const char* hex, unsigned char* bytes, unsigned int size, unsigned int* convertLen);
int i = 0;
int statusFlag = -1;
int bufferCount = 1;
MPI_Status recvStatus;
MPI_Request recvRequest = MPI_REQUEST_NULL;
MPI_Status sendStatus;
MPI_Request sendRequest = MPI_REQUEST_NULL;
int sendComplete = 0;
int recvComplete = 0;

int main(int argc, char** argv) {
	int opt;
	int length;
	char* splitCharset;
	char* charset;
	char* hashString;
	unsigned char* hashHex;

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
	hashString = calloc(1, strlen(argv[optind]));
	strcpy(hashString, argv[optind]);
	hashHex = calloc(1, strlen(hashString) / 2);
	hexToBytes(hashString, hashHex, strlen(hashString), NULL);
	

	// Generate the SplittedCharset
	splitCharset = malloc(strlen(splitCharsetFunc(charset, world_rank, world_size)));
	splitCharset = splitCharsetFunc(charset, world_rank, world_size);
	printf("Starting Compute for Hash ");
	for (int i = 0; i < strlen(hashString) / 2; i++)
	{
		printf("%02hx", hashHex[i]);
	}
	printf(" with Charset %s and splitCharset %s for passwords with max length %d on Node %d\n\n", charset, splitCharset, length, world_rank);
	MPI_Irecv(&statusFlag, bufferCount, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &recvRequest);
	bruteForceSha256(charset, splitCharset, hashHex, length, world_rank);
	if (!recvComplete) {
		printf("Node %d waiting", world_rank);
		MPI_Wait(&recvRequest, &recvStatus);
		printf("Node %d done waiting", world_rank);
	}
	if (world_rank == 0){
		printf("Master: Password was found by Node %d", statusFlag);
	}

	// Finalize the MPI environment.
	MPI_Finalize();
	free(charset);
	free(splitCharset);
	free(hashHex);
	free(hashString);
}

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



	/*
		Loop while length of arrayOfCharsets is <= the maximum password length specified
	*/

	while (currentLength <= maxLength) {

		while (charsetBeginPtr < charsetEndPtr) {
			arrayOfCharsets[currentLength - 1] = charsetBeginPtr++;
			// Hash Cracking:
			for (i = 0; i < currentLength; i++) {
				passwordString[i] = *arrayOfCharsets[i];
			}
			passwordString[i] = '\0';
			//printf("cracking Hash with Password: %s\n", passwordString);
			unsigned char genHash[SHA256_DIGEST_LENGTH];
			SHA256_CTX sha256;
			SHA256_Init(&sha256);
			SHA256_Update(&sha256, passwordString, currentLength);
			SHA256_Final(genHash, &sha256);

			for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
				if (genHash[i] != *hashHex) break;
				hashHex++;
			}
			if (i == SHA256_DIGEST_LENGTH){
				printf("\n\nNode %d: Password was found: %s\n\n", world_rank, passwordString);
				statusFlag = world_rank;
				MPI_Send(&statusFlag, bufferCount, MPI_INT, 0, 0, MPI_COMM_WORLD);
				printf("Node %d SEND after PW complete.", world_rank);
				return;
			}
			hashHex = hashHexBeginPtr;
		}

		/*Check MPI COMM */

		if (!recvComplete){
			printf("InnerrecvComplete Node %d = False\n", world_rank);
			MPI_Test(&recvRequest, &recvComplete, &recvStatus);
		} else {
			printf("Node %d return\n", world_rank);
			return;
		}

		/*
			For all charsets --> if splitCharset on position 0 has reached the end or any other charset has reached the end -->
			increment the counter which is used to determine if the array has to be extended
		*/

		for (i = currentLength - 1, counter = 0; i >= 0; i--) {
			if (*arrayOfCharsets[i] == *(charsetEndPtr - 1)) {
				counter++;
			} else if (i == 0 && *arrayOfCharsets[i] == *(splitCharsetEndPtr - 1)) {
				counter++;
			} else break;
		}

		/*
			When the counter is currentLength --> every character has reached the end of the charset -->
			reset the Array Of Charsets and increase Length
		*/

		if (counter != currentLength) {
			if (!arrayOfCharsets[currentLength - counter - 1]++) {
				printf("arrayOfCharsets on Position %d was null when trying to access and increment\n", currentLength - counter - 1);
			}
			for (i = currentLength - 1; i >= currentLength - counter; i--) {
				arrayOfCharsets[i] = charset;
			}
		} else {

		/*
			reset the first Charset to the splitcharset
			Reset all other Charsets to the normal charset
		*/

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