#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>


void map_input_file(const char *filename, void **start, long *len)
{
	int fd;
	off_t off;
	void *pos;

	if((fd = open(filename, O_RDONLY)) == -1) {
		goto fail;
	}

	/* Get the file size. */
	if((off = lseek(fd, 0, SEEK_END)) == -1) {
		goto fail;
	}

	if((pos = mmap(NULL, off, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
		goto fail;
	}

	close(fd);
	*len = off;
	*start = pos;
	return;

fail:
	perror(filename);
	if(fd > -1) {
		close(fd);
	}
	*start = NULL;
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
