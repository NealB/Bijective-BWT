#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "map_file.h"

static unsigned char *BWTS;
static unsigned char *outBuf;
static long len;
static int counts[256];
static int *prev;
//static int *next;

//void cycleSort(int *prev);
void write_out(const char *data, int len, char *outFilenameArg, char *inFilenameArg);

int main(int argc, char **argv)
{
    if(argc < 2) {
        fprintf(stderr, "Usage: unbwts <infile.bwts> [<outfile>]\n");
        fprintf(stderr, "If output file name is unspecified, a name is generated\n");
        exit(1);
    }

    char *infilename = argv[1];
    char *outfilename = argc < 3 ? NULL : argv[2];
    map_in(BWTS, len, infilename);

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

    prev = (int*)malloc(len * sizeof(int));

    outBuf = (unsigned char*)malloc(len);


    for(i = 0; i < len; i++) {
        prev[i] = counts[BWTS[i]]++;
    }


    //cycleSort(prev);


    //next = prev; // change of name

    //printf("Finished cycle sort\n");

    int bwtsStart = 0;
    int writtenCount = 0;
    int outputPos = len - 1;

    while(writtenCount < len)
    {
        while(prev[bwtsStart] == -1 && bwtsStart < len) {
            bwtsStart++;
        }

        int bwtsPos = bwtsStart;

        do {
            int prevPos = prev[bwtsPos];

            outBuf[outputPos--] = BWTS[bwtsPos];

            prev[bwtsPos] = -1;
            bwtsPos = prevPos;

            writtenCount++;
        } while(bwtsPos != bwtsStart);


    }


    write_out(outBuf, len, outfilename, infilename);

    return 0;
}

//void cycleSort(int *A)
//{
//    int start = 0;
//    int sortedCount = 0;
//    int i;
//
//    char *seen = (char*)malloc(len);
//    memset(seen, 0, len);
//
//
//    while(sortedCount < len) {
//
//        while(seen[start] == 1 && start < len) {
//            start++;
//        }
//
//        int pos1 = start;
//        int pos2 = -1;
//        int pos3 = -1;
//
//        do {
//            pos3 = pos2;
//            pos2 = pos1;
//            pos1 = A[pos1];
//
//            if(pos3 >= 0) {
//                A[pos2] = pos3;
//                seen[pos2] = 1;
//            }
//            if(pos1 == start) {
//                A[pos1] = pos2;
//                seen[pos1] = 1;
//            }
//
//            sortedCount++;
//        } while(pos1 != start);
//    }
//
//    free(seen);
//}


void write_out(const char *data, int len, char *outFilenameArg, char *inFilenameArg)
{
    char *outFilename;
    FILE *out = NULL;

    if(outFilenameArg != NULL) {
        outFilename = outFilenameArg;
        out = fopen(outFilenameArg, "wb");

        if(!out) {
            fprintf(stderr, "Couldn't open output file for writing\n");
            perror(outFilename);
            exit(1);
        }
    }
    else {
        int rv = asprintf(&outFilename, "%s_XXXXXX", inFilenameArg);

        if(rv > 0) {
            int outFileFd = mkstemps(outFilename, 0);

            printf("Writing to %s\n", outFilename);
            out = fdopen(outFileFd, "w");

            if(!out) {
                fprintf(stderr, "Couldn't open output file for writing\n");
                perror(outFilename);
                exit(1);
            }
        }
        else {
            fprintf(stderr, "Allocating outfile name failed. Abort\n");
            exit(1);
        }

    }

    fwrite(data, 1, len, out);

    fclose(out);
}

