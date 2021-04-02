#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "map_file.h"

static unsigned char *BWTS;
static unsigned char *outBuf;
static long len;
static unsigned int counts[256];
static unsigned int *prev;
static unsigned int *next;
static char *seen;

int main(int argc, char **argv)
{
	if(argc < 2) {
		fprintf(stderr, "Usage: unbwts <infile.bwts> [<outfile>]\n");
		fprintf(stderr, "If unspecified, output is written to standard output\n");
		exit(1);
	}

	char *outfilename = argc < 3 ? NULL : argv[2];
	map_in(BWTS, len, argv[1]);

    memset(counts, 0, sizeof(counts));

    int i;
    for(i = 0; i < len; i++) {
        counts[BWTS[i]]++;
    }

    int sum = 0;
    for(i = 0; i < 256; i++) {
        int currentCount = counts[i];
        counts[i] = sum;
        sum += currentCount;
    }

    prev = (unsigned int*)malloc(len * sizeof(unsigned int));
    next = (unsigned int*)malloc(len * sizeof(unsigned int));
    seen = (char*)malloc(len);

    outBuf = (unsigned char*)malloc(len);

    memset(seen, 0, len);

    for(i = 0; i < len; i++) {
        prev[i] = counts[BWTS[i]]++;
    }

    for(i = 0; i < len; i++) {
        next[prev[i]] = i;
    }

    int start = 0;
    int writtenCount = 0;
    int lwCount = 0;

    while(writtenCount < len)
    {

        while(seen[start] == 1 && start < len) {
            start++;
        }


        int pos = start;
        int lyndonWordLength = 0;

        do {
            pos = next[pos];

            outBuf[lyndonWordLength++] = BWTS[pos];

            seen[pos] = 1;

        } while(pos != start);

        int lwDest = len - writtenCount - lyndonWordLength;
        memmove(&(outBuf[lwDest]), &(outBuf[0]), lyndonWordLength);

        writtenCount += lyndonWordLength;

        lwCount++;
    }

	FILE *unbwts_out = outfilename ? fopen(outfilename, "w") : stdout;
	if(!unbwts_out) {
		fprintf(stderr, "Couldn't open output file for writing\n");
		perror(outfilename);
		exit(1);
	}
	fwrite(outBuf, 1, len, unbwts_out);

    return 0;
}

