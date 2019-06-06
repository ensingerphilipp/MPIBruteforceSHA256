#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

///*
//	Reset the first Charset to the splitcharset
//	Reset all other Charsets to the normal charset
//*/
//
//void resetArray(char** arrayOfCharsets, char *charset, char *splitCharset, int length) {
//	arrayOfCharsets[0] = splitCharset;
//	for (int i = 1; i <= length; i++) {
//		arrayOfCharsets[i] = charset;
//	}
//}
//
///*
//	Print the first character of every charset in the arrayOfCharsets
//*/
//
//void printArray(char** arrayOfCharsets, int len) {
//	for (int i = 0; i < len; i++) {
//		putchar(*arrayOfCharsets[i]);
//	}
//	putchar('\n');
//}
//
//int bruteForceSha256(char *charset, char *splitCharset, char *hash, int maxLength) {
//	char* charsetBeginPtr = charset;
//	char* splitCharsetBeginPtr = splitCharset;
//	char** arrayOfCharsets = NULL;
//	int currentLength = 1;
//	int i;
//	int counter;
//
//		arrayOfCharsets = (char**)malloc(sizeof(char*) * (maxLength + 1));
//		const char* charsetEndPtr = charsetBeginPtr + strlen(charset);
//		const char* splitCharsetEndPtr = splitCharsetBeginPtr + strlen(splitCharset);
//
//		/*
//			Loop while length of arrayOfCharsets is <= the maximum password length specified
//		*/
//
//		while (currentLength <= maxLength) {
//
//			/*
//				if length is 1 use the splitSet for filling characters
//				(Only fill first position with splitCharset)
//			*/
//
//			if (currentLength == 1) {
//				while (splitCharsetBeginPtr < splitCharsetEndPtr) {
//					arrayOfCharsets[currentLength - 1] = splitCharsetBeginPtr++;
//					printArray(arrayOfCharsets, currentLength);
//				}
//			}
//
//			/*
//				else use the complete Charset for filling charsets
//				(this does not fill the first position where splitCharset is filled)
//			*/
//
//			else {
//				while (charsetBeginPtr < charsetEndPtr) {
//					arrayOfCharsets[currentLength - 1] = charsetBeginPtr++;
//					printArray(arrayOfCharsets, currentLength);
//				}
//			}
//
//			/*
//				For all charsets --> if splitCharset on position 0 has reached the end or any other charset has reached the end -->
//				increment the counter which is used to determine if the array has to be extended
//			*/
//
//			for (i = currentLength - 1, counter = 0; i >= 0; i--) {
//				if (i == 0 && *arrayOfCharsets[i] == *(splitCharsetEndPtr - 1)) {
//					counter++;
//				}
//				else if (*arrayOfCharsets[i] == *(charsetEndPtr - 1)) {
//					counter++;
//				}
//				else break;
//			}
//			/*
//				When the counter is currentLength --> every character has reached the end of the charset -->
//				reset the Array Of Charsets and increase Length
//			*/
//
//			if (counter == currentLength) {
//				resetArray(arrayOfCharsets, charset, splitCharset, currentLength);
//				currentLength++;
//			}
//
//			/*
//				Else increment the outer charsetPointer (splitCharsetPointer) by
//				one (set next character) and reset all inner charsets
//			*/
//
//			else {
//				// Increment Pointer at charset and if it was null print Error
//				if (!(!arrayOfCharsets[currentLength - counter - 1]++)) {
//				}
//				else {
//					printf("arrayOfCharsets on Position %d was null when trying to access and increment", currentLength - counter - 1);
//				}
//
//				for (i = currentLength - 1; i >= currentLength - counter; i--) {
//					arrayOfCharsets[i] = charset;
//				}
//			}
//			charsetBeginPtr = charset;
//		}
//		free(arrayOfCharsets);
//		return 0;
//}
//
//char* splitCharsetFunc(char *charset, int world_rank, int world_size) {
//	char* splitCharset = NULL;
//	for (int i = 0; i < (strlen(charset) / world_size) + 1; i++) {
//		splitCharset[i] = charset[i * world_rank];
//	}
//	return splitCharset;
//}

int main(int argc, char** argv) {
	int opt;
	int length;
	char* splitCharset = NULL;
	char *charset = NULL;
	char *hash = NULL;

	// Initialize the MPI environment
	MPI_Init(NULL, NULL);
	printf("MPI INIT");
	// Get the number of processes
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	printf("World_size = %d", world_size);
	// Get the rank of the process
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	printf("World_rank = %d", world_rank);

	// Get the name of the processor
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);
	printf("processor name_len: = %d", name_len);
	printf("processor_name = %s ", processor_name);
  
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
	printf("Got Arguments: %s %d %s", charset, length, hash);

	// Generate the SplittedCharset
	splitCharset = malloc(strlen(splitCharsetFunc(charset, world_rank, world_size)));
	splitCharset = strcpy(splitCharset, splitCharsetFunc(charset, world_rank, world_size));
	printf("Starting Compute for Hash '%s' with Charset '%s' and splitCharset %s for passwords with max length '%d' on Node %d\n", hash, charset, splitCharset, length, world_rank);
	bruteForceSha256(charset, splitCharset, hash, length);

	// Finalize the MPI environment.
	MPI_Finalize();

	free(charset);
	free(hash);
}