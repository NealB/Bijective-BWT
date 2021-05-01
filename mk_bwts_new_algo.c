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
void separate_lw_cycle(const int lw0_start, int lw1_start);
void write_bwts_out(char *outFilenameArg, char *inFilenameArg);
int find_next_lyndon_factor(int start_pos);


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
void separate_lw_cycle(const int lw0_start, int lw1_start)
{
  const int wordLen = lw1_start - lw0_start;

  //prev_rank is the rank of the next text position in the current LW, modulo the LW length:
  //  Definitions:
  //    T[p] -> character at text position p
  //    rank(T[p]) -> lex. rank of text position p
  //    lw0_start -> start position of current LW in text
  //    lw1_start -> start position of next LW in text, or, if there is no next LW, last position + 1
  //
  //  Example for LW of length 3, starting at T[40]:
  //    prev_rank_0 = rank(T[40])
  //    prev_rank_1 = rank(T[42])
  //    prev_rank_2 = rank(T[41])
  //    prev_rank_3 = rank(T[40])
  //    prev_rank_4 = rank(T[42])
  //    ...

  //int plus1TextPos = lw0_start;

  //iteration counts characters visited
  int iteration = 0;
  while(1) {

    int lwPosition = wordLen - 1 - (iteration % wordLen);
    int textPos = lw0_start + lwPosition;

    int curr_rank = isa[textPos];
    int plus1Rank = isa[lw0_start + ((lwPosition + 1) % wordLen)];
    const int start_rank = isa[textPos];

    while(curr_rank < len-1) {
      const int next_rank_start = sa[curr_rank+1];
      if(textPos > next_rank_start
          || T[textPos] != T[next_rank_start]
          || (wordLen>1 && plus1Rank < isa[next_rank_start+1])
          || (wordLen==1 && curr_rank < isa[next_rank_start+1])
          ) {
        break;
      }

      sa[curr_rank] = sa[curr_rank+1];
      isa[sa[curr_rank]] = curr_rank;
      curr_rank++;
    }
    sa[curr_rank] = textPos;
    isa[textPos] = curr_rank;

    if(start_rank == curr_rank) {
      break;
    }
    else if(wordLen == 1) {
      break;
    }
    else {
      iteration++;
    }
  }
  int visitedChars = iteration + 1;

  printf("Word len: %10d; visited: %10d\n", wordLen, visitedChars);
}


int find_next_lyndon_factor(int start_pos)
{
  int p = start_pos + 1;
  while((p < len) && (isa[p] > isa[start_pos])) {
    p++;
  }
  return p;
}

unsigned char *make_bwts_sa(void) {
	int i, j;

  // compute ISA
	isa = (int *)malloc(sizeof(int) * len);
	for(i=0; i<len; i++) {
		isa[sa[i]] = i;
	}

	MARK_TIME("Compute ISA");

	int lw0_start_pos = 0; 

  while(lw0_start_pos < len) {
    int lw1_start_pos = find_next_lyndon_factor(lw0_start_pos);

    if(lw1_start_pos == len) {
      break;
    }

    separate_lw_cycle(lw0_start_pos, lw1_start_pos);

    lw0_start_pos = lw1_start_pos;
  }

	free(sa);

	MARK_TIME("Fix sort order");

	unsigned char *bwts = (unsigned char *)malloc(len);

  lw0_start_pos = 0;
  
  while(lw0_start_pos < len) {
    int lw1_start_pos = find_next_lyndon_factor(lw0_start_pos);

    bwts[isa[lw0_start_pos]] = T[lw1_start_pos - 1];

    for(i = lw0_start_pos + 1; i < lw1_start_pos; i++) {
      bwts[isa[i]] = T[i - 1];
    }

    lw0_start_pos = lw1_start_pos;
  }

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

