#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>


typedef struct {
	void *sp, *ep;
} ptr_range;

void map_input_file2(const char *filename, void **start, long *len)
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

ptr_range map_input_file(const char *filename)
{
	void *start;
	long nbytes;
       	map_input_file2(filename, &start, &nbytes);
	return (ptr_range){ start, start + nbytes };
}

void unmap_file(ptr_range extent)
{
	munmap(extent.sp, extent.ep - extent.sp);
}
