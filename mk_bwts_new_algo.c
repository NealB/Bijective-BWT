/* vim: set noai ts=2 sw=2: */
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <divsufsort.h>
#include "map_file.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifdef SHOW_TIMINGS
#include <time.h>
static clock_t last_clock, t;
#define MARK_TIME(msg) \
	t = clock();                                                                        \
	fprintf(stderr, msg " time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC); \
	last_clock = t;
#else
#define MARK_TIME(msg) /*nothing*/
#endif

static unsigned char *T;
static unsigned char *bwts;
static long len;
static saidx_t *sa;
static int *isa;

unsigned char *make_bwts_sa(void);
void separate_lw_cycle(const int lw0_start, int lw1_start, int lw0_rank);
void write_bwts_out(char *outFilenameArg, char *inFilenameArg);


int main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "Usage: mk_bwts_sa <infile> [<outfile.bwts>]\n");
		fprintf(stderr, "If outfile is not supplied, a unique file name is generated\n");
		exit(1);
	}

	char *bwts_outfilename = argc < 3 ? NULL : argv[2];
	map_in(T, len, argv[1]);

#ifdef SHOW_TIMINGS
	last_clock = clock();
#endif

	sa = (saidx_t *)malloc(sizeof(saidx_t) * len);
	divsufsort(T, sa, len);

	MARK_TIME("Suffix sort");

	bwts = make_bwts_sa();

  write_bwts_out(bwts_outfilename, argv[1]);

	MARK_TIME("Write BWTS");

	return 0;
}

// Make the current Lyndon Factor into a BWT cycle by re-ranking positions within the SA/ISA
void separate_lw_cycle(const int lw0_start, int lw1_start, int lw0_rank)
{
  const int wordLen = lw1_start - lw0_start;

  //prev_rank is the rank of the next position in cyclic order
  int prev_rank = lw0_rank;

  int p = 0;
  for(; ; p++) {

    //j starts at the end of the Lyndon factor and cycles through backwards until ranks stop needing to be shifted
    int j = lw1_start - 1 - (p % wordLen);

    int curr_rank = isa[j];
    const int start_rank = isa[j];

    while(curr_rank < len-1) {
      const int next_rank_start = sa[curr_rank+1];
      if(j > next_rank_start
          || T[j] != T[next_rank_start]
          || prev_rank < isa[next_rank_start+1]
          ) {
        break;
      }

      sa[curr_rank] = sa[curr_rank+1];
      isa[sa[curr_rank]] = curr_rank;
      curr_rank++;
    }
    sa[curr_rank] = j;
    isa[j] = curr_rank;

    prev_rank = curr_rank;


    if(start_rank == curr_rank) {
      break;
    }
  }

  printf("Word len: %10d; visited: %10d\n", wordLen, p);
}


unsigned char *make_bwts_sa(void) {
	int i, j;

  // compute ISA
	isa = (int *)malloc(sizeof(int) * len);
	for(i=0; i<len; i++) {
		isa[sa[i]] = i;
	}

	MARK_TIME("Compute ISA");

	int lw0_start_rank = isa[0];
	int lw0_start_pos = 0; 

	for(j=1; j<len && lw0_start_rank>0; j++) {

		if(isa[j] < lw0_start_rank) {
      int lw1_start_pos = j; 

      separate_lw_cycle(lw0_start_pos, lw1_start_pos, lw0_start_rank);

      lw0_start_pos = lw1_start_pos;
      lw0_start_rank = isa[lw1_start_pos];
		}

	}
	free(sa);

	MARK_TIME("Fix sort order");

	unsigned char *bwts = (unsigned char *)malloc(len);

	int min = len; // text position of current min
	int min_i = 0; // index in ISA of current min
	for(i=0; i<len; i++) {
		if(isa[i] < min) { // found Lyndon boundary 
			if(min < len) { // bwts[0] is treated as a special case afterwards
				// min is the first position of the current Lyndon factor. We want BWTS[ISA[i]] = T[i-1], but if ISA[i] is a Lyndon factor start, then we want to store the last character of the factor in BWTS.
				bwts[min] = T[i - 1];
			}

			min = isa[i];
			min_i = i;
		}
		else {
			bwts[isa[i]] = T[i - 1];
		}
	}
	bwts[0] = T[len - 1]; // this is an invariant
	
	MARK_TIME("Generate BWTS");

	free(isa);

	return bwts;
}

void write_bwts_out(char *outFilenameArg, char *inFilenameArg)
{
  char *outFilename;
	FILE *bwtsout = NULL;

	if(outFilenameArg != NULL) {
    outFilename = outFilenameArg;
		bwtsout = fopen(outFilenameArg, "wb");

    if(!bwtsout) {
      fprintf(stderr, "Couldn't open BWTS file for writing\n");
      perror(outFilename);
      exit(1);
    }
	}
	else {
		//char outfilenamebuf[] = "BWTS_out_XXXXXX.bwts";

    int rv = asprintf(&outFilename, "%s_XXXXXX.bwts", inFilenameArg);

    if(rv > 0) {
      int outFileFd = mkstemps(outFilename, 5);

      printf("Writing to %s\n", outFilename);
      bwtsout = fdopen(outFileFd, "w");

      if(!bwtsout) {
        fprintf(stderr, "Couldn't open BWTS file for writing\n");
        perror(outFilename);
        exit(1);
      }
    }
    else {
      fprintf(stderr, "Allocating outfile name failed. Abort\n");
      exit(1);
    }

	}

	fwrite(bwts, 1, len, bwtsout);

  fclose(bwtsout);
}

