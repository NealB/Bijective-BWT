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

void cycleSort(unsigned int *prev, char *seen);

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
    seen = (char*)malloc(len);

    outBuf = (unsigned char*)malloc(len);


    for(i = 0; i < len; i++) {
        prev[i] = counts[BWTS[i]]++;
    }


    cycleSort(prev, seen);


    next = prev; // change of name

    //printf("Finished cycle sort\n");

    int start = 0;
    int writtenCount = 0;
    int lwCount = 0;

    memset(seen, 0, len);

    while(writtenCount < len)
    {
        //printf("Lyndon word %d\n", lwCount);

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

void cycleSort(unsigned int *A, char *seen)
{
    int start = 0;
    int sortedCount = 0;
    int i;
    memset(seen, 0, len);


    while(sortedCount < len) {

        while(seen[start] == 1 && start < len) {
            start++;
        }
        
        int pos1 = start;
        int pos2 = -1;
        int pos3 = -1;

        do {
            pos3 = pos2;
            pos2 = pos1;
            pos1 = A[pos1];

            if(pos3 >= 0) {
                A[pos2] = pos3;
                seen[pos2] = 1;
            }
            if(pos1 == start) {
                A[pos1] = pos2;
                seen[pos1] = 1;
            }

            sortedCount++;
        } while(pos1 != start);
    }

}


