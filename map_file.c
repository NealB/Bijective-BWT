#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


void map_input_file(const char *filename, void **start, long *len)
{
	FILE *fp;
	long n;
	void *location;

	if((fp = fopen(filename, "rb")) == NULL) {
		perror(filename);
		exit(EXIT_FAILURE);
	}

	/* Get the file size. */
	if(fseek(fp, 0, SEEK_END) == 0) {
		n = ftell(fp);
	}
	else {
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	location = mmap(NULL, n, PROT_READ, MAP_FILE | MAP_PRIVATE, fileno(fp), 0);
	if(location == MAP_FAILED) {
		perror(filename);
		exit(1);
	}

	fclose(fp);

	*start = location;
	*len = n;
}


void *map_temp_writable(size_t size)
{
	FILE *fp;
	void *location;

	if((fp = tmpfile()) == NULL) {
		perror("Attempting to create temporary file");
    return NULL;
	}

	/* Set the file size. */
	if(ftruncate(fileno(fp), size) != 0) {
		perror("ftruncate");
    return NULL;
	}

	location = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fp), 0);
	if(location == MAP_FAILED) {
		perror("Attempting to map temporary file");
    return NULL;
	}

  return location;
}
