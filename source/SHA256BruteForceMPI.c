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
void resetArray(char** arrayOfCharsets, char* charset, char* splitCharset, int length);
void resetArray(char** arrayOfCharsets, char* charset, char* splitCharset, int length);
void crackHash(char** arrayOfCharsets, char* passwordString, unsigned char* hashHex, int len);
unsigned char* hexstr_to_ucharByte(char* hexstr);

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
	hashString = malloc(strlen(argv[optind]));
	hashHex = malloc(strlen(hashString));
	strcpy(hashString, argv[optind]);
	hashHex = hexstr_to_ucharByte(hashString);

	// Generate the SplittedCharset
	splitCharset = malloc(strlen(splitCharsetFunc(charset, world_rank, world_size)));
	splitCharset = splitCharsetFunc(charset, world_rank, world_size);
	printf("Starting Compute for Hash '%02hx' with Charset '%s' and splitCharset %s for passwords with max length '%d' on Node %d\n", hashHex, charset, splitCharset, length, world_rank);
	bruteForceSha256(charset, splitCharset, hashHex, length);

	// Finalize the MPI environment.
	MPI_Finalize();
	free(charset);
	free(splitCharset);
	free(hashHex);
	free(hashString);
}

unsigned char* hexstr_to_ucharByte(char* hexstr)
{
	size_t len = strlen(hexstr);
	size_t final_len = len / 2;
	unsigned char* chrs = (unsigned char*)malloc((final_len + 1) * sizeof(*chrs));
	for (size_t i = 0, j = 0; j < final_len; i += 2, j++)
		chrs[j] = (hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i + 1] % 32 + 9) % 25;
	chrs[final_len] = '\0';
	return chrs;
}

/*
	Reset the first Charset to the splitcharset
	Reset all other Charsets to the normal charset
*/

void resetArray(char** arrayOfCharsets, char* charset, char* splitCharset, int length) {
	arrayOfCharsets[0] = splitCharset;
	for (int i = 1; i <= length; i++) {
		arrayOfCharsets[i] = charset;
	}
}

/*
	Print the first character of every charset in the arrayOfCharsets
*/

void printArray(char** arrayOfCharsets, int len) {
		for (int i = 0; i < len; i++) {
			putchar(*arrayOfCharsets[i]);
		}
		putchar('\n');
}

void crackHash(char** arrayOfCharsets, char* passwordString, unsigned char* hashHex, int len) {
	int i;
	for (i = 0; i < len; i++) {
		passwordString[i] = *arrayOfCharsets[i];
	}
	passwordString[i] = '\0';

	unsigned char genHash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, passwordString, len);
	SHA256_Final(genHash, &sha256);
	i = 0;

	for (i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		printf("%02hx", genHash[i]);
	}
	printf("\n");
}

void bruteForceSha256(char* charset, char* splitCharset, unsigned char* hashHex, int maxLength) {
	char* charsetBeginPtr = charset;
	char* splitCharsetBeginPtr = splitCharset;
	char* charsetEndPtr = charsetBeginPtr + strlen(charset);
	char* splitCharsetEndPtr = splitCharsetBeginPtr + strlen(splitCharset);
	char** arrayOfCharsets = (char**)malloc(sizeof(char*) * (maxLength + 1));
	char* passwordString = malloc(maxLength + 1);
	int currentLength = 1;
	int i;
	int counter;



	/*
		Loop while length of arrayOfCharsets is <= the maximum password length specified
	*/

	while (currentLength <= maxLength) {

		/*
			if length is 1 use the splitSet for filling characters
			(Only fill first position with splitCharset)
		*/

		if (currentLength == 1) {
			while (splitCharsetBeginPtr < splitCharsetEndPtr) {
				arrayOfCharsets[currentLength - 1] = splitCharsetBeginPtr++;
				crackHash(arrayOfCharsets, passwordString, hashHex, currentLength);
			}
		}

		/*
			else use the complete Charset for filling charsets
			(this does not fill the first position where splitCharset is filled)
		*/

		else {
			while (charsetBeginPtr < charsetEndPtr) {
				arrayOfCharsets[currentLength - 1] = charsetBeginPtr++;
				crackHash(arrayOfCharsets, passwordString, hashHex, currentLength);
			}
		}

		/*
			For all charsets --> if splitCharset on position 0 has reached the end or any other charset has reached the end -->
			increment the counter which is used to determine if the array has to be extended
		*/

		for (i = currentLength - 1, counter = 0; i >= 0; i--) {
			if (i == 0 && *arrayOfCharsets[i] == *(splitCharsetEndPtr - 1)) {
				counter++;
			}
			else if (*arrayOfCharsets[i] == *(charsetEndPtr - 1)) {
				counter++;
			}
			else break;
		}
		/*
			When the counter is currentLength --> every character has reached the end of the charset -->
			reset the Array Of Charsets and increase Length
		*/

		if (counter == currentLength) {
			resetArray(arrayOfCharsets, charset, splitCharset, currentLength);
			currentLength++;
		}

		/*
			Else increment the outer charsetPointer (splitCharsetPointer) by
			one (set next character) and reset all inner charsets
		*/

		else {
			// Increment Pointer at charset and if it was null print Error
			if (!(!arrayOfCharsets[currentLength - counter - 1]++)) {
			}
			else {
				printf("arrayOfCharsets on Position %d was null when trying to access and increment\n", currentLength - counter - 1);
			}

			for (i = currentLength - 1; i >= currentLength - counter; i--) {
				arrayOfCharsets[i] = charset;
			}
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