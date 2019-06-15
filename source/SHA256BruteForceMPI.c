#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <openssl/sha.h>
#include <assert.h>

char* splitCharsetFunc(char* charset, int world_rank, int world_size);
void bruteForceSha256(char* charset, char* splitCharset, unsigned char* hashHex, int maxLength);
void crackHash(char** arrayOfCharsets, char* passwordString, unsigned char* hashHex, int len);
int hexToBytes(const char* hex, unsigned char* bytes, unsigned int size, unsigned int* convertLen);
int i = 0;
static char* crackedPassword;

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
	crackedPassword = NULL;
	

	// Generate the SplittedCharset
	splitCharset = malloc(strlen(splitCharsetFunc(charset, world_rank, world_size)));
	splitCharset = splitCharsetFunc(charset, world_rank, world_size);
	printf("Starting Compute for Hash ");
	for (int i = 0; i < strlen(hashString) / 2; i++)
	{
		printf("%02hx", hashHex[i]);
	}
	printf(" with Charset %s and splitCharset %s for passwords with max length %d on Node %d\n", charset, splitCharset, length, world_rank);
	bruteForceSha256(charset, splitCharset, hashHex, length);

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


void crackHash(char** arrayOfCharsets, char* passwordString, unsigned char* hashHex, int len) {
	for (i = 0; i < len; i++) {
		passwordString[i] = *arrayOfCharsets[i];
	}
	passwordString[i] = '\0';
	printf("cracking Hash with Password: %s\n", passwordString);
	unsigned char genHash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, passwordString, len);
	SHA256_Final(genHash, &sha256);
	
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		if (*genHash != *hashHex) break;
	}
	if (i == SHA256_DIGEST_LENGTH) printf("HASH FOUND! Password: %s", passwordString);
}

void bruteForceSha256(char* charset, char* splitCharset, unsigned char* hashHex, int maxLength) {
	char* charsetBeginPtr = charset;
	char* splitCharsetBeginPtr = splitCharset;
	char* charsetEndPtr = charsetBeginPtr + strlen(charset);
	char* splitCharsetEndPtr = splitCharsetBeginPtr + strlen(splitCharset);
	char** arrayOfCharsets = (char**)malloc(sizeof(char*) * (maxLength + 1));
	char* passwordString = malloc(maxLength + 1);
	int currentLength = 1;
	int counter = 0;



	/*
		Loop while length of arrayOfCharsets is <= the maximum password length specified
	*/

	while (currentLength <= maxLength) {

		/*
			if length is 1 use the splitSet for filling characters
			(Only fill first position with splitCharset)
		*/

		if (currentLength != 1) {
			while (charsetBeginPtr < charsetEndPtr) {
				arrayOfCharsets[currentLength - 1] = charsetBeginPtr++;
				crackHash(arrayOfCharsets, passwordString, hashHex, currentLength);
			}	
		}

		/*
			else use the complete Charset for filling charsets
			(this does not fill the first position where splitCharset is filled)
		*/

		else {
			while (splitCharsetBeginPtr < splitCharsetEndPtr) {
				arrayOfCharsets[currentLength - 1] = splitCharsetBeginPtr++;
				crackHash(arrayOfCharsets, passwordString, hashHex, currentLength);
			}
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
			for (i = currentLength - 1; i >= currentLength - counter; i--) {
				arrayOfCharsets[i] = charset;
			}

		} else {
			/*
	Reset the first Charset to the splitcharset
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
	
	printf("Node : %d has intervall %d and offset %d\n", world_rank, intervall, offset);
	char* splitCharset = malloc(intervall);
	int i;
	for (i = offset; i < offset + intervall; i++) {
		splitCharset[i - offset] = charset[i];
	}
	printf("Node : %d writing stringEND to %d\n", world_rank, i - offset);
	splitCharset[i - offset] = '\0';
	return splitCharset;
}