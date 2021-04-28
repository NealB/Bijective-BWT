/* vim: set noai ts=2 sw=2: */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <divsufsort.h>
#include "map_file.h"
#include <threads.h>

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
static long len;
static saidx_t *sa;
static int *isa;
static unsigned char *bwts;

void make_bwts_sa(void);
int move_lyndonword_head(int lw_start, int lw_len, int lw_rank);
void separate_lw_cycle(int lw0_start, int lw1_start, int lw0_rank);
void write_bwts_cycle(int cycle_start_pos, int cycle_end_pos);
void compute_ISA(void);

int main(int argc, char **argv) {
	if(argc < 2) {
		fprintf(stderr, "Usage: mk_bwts_sa <infile> [<outfile.bwts>]\n");
		fprintf(stderr, "If unspecified, output is written to a temp file\n");
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

	make_bwts_sa();

	//FILE *bwtsout = bwts_outfilename ? fopen(bwts_outfilename, "w") : stdout;
	FILE *bwtsout;

	if(bwts_outfilename) {
		bwtsout = fopen(bwts_outfilename, "wb");
	}
	else {
		char outfilenamebuf[] = "BWTS_out_XXXXXX.bwts";

		int outFileFd = mkstemps(outfilenamebuf, 5);

		bwts_outfilename = strdup(outfilenamebuf);

		printf("Writing to %s\n", bwts_outfilename);
		bwtsout = fdopen(outFileFd, "w+");
	}

	if(!bwtsout) {
		fprintf(stderr, "Couldn't open BWTS file for writing\n");
		perror(bwts_outfilename);
		exit(1);
	}

	fwrite(bwts, 1, len, bwtsout);

	MARK_TIME("Write BWTS");

	return 0;
}

// Move Lyndon Word at lw_start down to its final rank, updating SA and ISA
// Args:
//  lw_start -- start position of LW in the text
//  lk_len -- length of LW
//  lw_rank -- start rank of LW
// Return:
//  final rank
int move_lyndonword_head(int lw_start, int lw_len, int lw_rank) {
    // Compare Lyndon factor T[lw_start..lw_start+lw_len-1] with the next-ranking suffix (lw_rank+1);
    //  We may stop when
    //  !(sa[lw_rank+1] > lw_start+lw_len)
    //  because any Lyndon factor ranks lexicographically before any suffix earlier in the text (property of the LW factorization) and
    //  before any other position within itself (property of Lyndon words)
	while(lw_rank+1<len && (sa[lw_rank+1] > lw_start+lw_len)) {
		const int next_rank_start = sa[lw_rank+1];

		const int compare_len = MIN(lw_len, len - next_rank_start);

		// TODO: this seems to theorectically work faster if we have an LCP array or an LCE data structure
		const int res = memcmp(&(T[lw_start]), &(T[next_rank_start]), compare_len);

        if(res < 0) { // our Lyndon word is smaller
            break;
        }
        else if(res == 0) {

            // if the next rank runs up to the EOF, then it is smaller
            if(next_rank_start + lw_len < len) {

                // otherwise, compare ranks:
                if(lw_rank < isa[next_rank_start + lw_len]) {
                    break;
                }
            }
        }

		// shift SA/ISA one position to left
		sa[lw_rank] = sa[lw_rank+1];
		isa[sa[lw_rank]] = lw_rank;
		lw_rank++;
	}
	sa[lw_rank] = lw_start;
	isa[lw_start] = lw_rank;

	return lw_rank;
}

// Make the current Lyndon Factor into a BWT cycle by re-ranking positions within the SA/ISA
void separate_lw_cycle(int lw0_start, int lw1_start, int lw0_rank)
{
    const int lw0_head_rank = move_lyndonword_head(lw0_start, lw1_start - lw0_start, lw0_rank);

    int ref_rank = lw0_head_rank;

    for(int j = lw1_start; j-- > lw0_start+1; ) {
      int test_rank = isa[j];
      const int start_rank = test_rank;

      while(test_rank < len-1) {
        const int next_rank_start = sa[test_rank+1];
        if(j > next_rank_start
            || T[j] != T[next_rank_start]
            || ref_rank < isa[next_rank_start+1]
            ) {
          break;
        }

        sa[test_rank] = sa[test_rank+1];
        isa[sa[test_rank]] = test_rank;
        test_rank++;
      }
      sa[test_rank] = j;
      isa[j] = test_rank;

      ref_rank = test_rank;

      if(start_rank == test_rank) {
        break;
      }
    }

}

// Write current cycle to the BWTS in memory
void write_bwts_cycle(int cycle_start_pos, int cycle_end_pos)
{
  int i;

  //attach the beginning and end
  bwts[isa[cycle_start_pos]] = T[cycle_end_pos];

  //internal positions
	for(i = cycle_start_pos + 1; i <= cycle_end_pos; i++) {

    bwts[isa[i]] = T[i - 1];

	}

}

void make_bwts_sa(void) {
	int i, j;

	//isa = (int *)malloc(sizeof(int) * len);

	//MARK_TIME("Allocate ISA");

  compute_ISA();

	MARK_TIME("Compute ISA");

	exit(0);

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

	bwts = (unsigned char *)malloc(len);

	int current_lw_start_rank = isa[0];
	int current_lw_start_pos = 0;
	for(i=1; i<len; i++) {

		if(isa[i] < current_lw_start_rank) {
      //beginning of next LW

      write_bwts_cycle(current_lw_start_pos, i - 1);

			current_lw_start_rank = isa[i];
			current_lw_start_pos = i;
		}

	}

  write_bwts_cycle(current_lw_start_pos, len - 1);

	free(isa);
}

struct ISA_part_arg {
	int rangeStart, rangeEnd;
	int *isa;
};

int compute_ISA_part(void *arg)
{
	struct ISA_part_arg *ipa = (struct ISA_part_arg *)arg;

	for(int j=0; j<10; j++) {
		for(int i=0; i<len; i++) {

			if((sa[i] >= ipa->rangeStart) && (sa[i] < ipa->rangeEnd)) {

				(ipa->isa)[sa[i]] = i;

			}

		}

		//for(int i=ipa->rangeStart; i<ipa->rangeEnd; i++) {
			//isa[sa[i]] = i;
		//}
	}

	//printf("Hi from thread %d - %d\n", ipa->rangeStart, ipa->rangeEnd);

	return 0;
}

void compute_ISA()
{
  int i;
	thrd_t thread0, thread1;

	int *isa1 = (int *)malloc(sizeof(int) * len);
	int *isa2 = (int *)malloc(sizeof(int) * len);

	MARK_TIME("Allocate ISA");
	
	//isa = (int *)malloc(sizeof(int) * len);

	int splitPoint = len / 2;
	struct ISA_part_arg thread0_arg = { 0, splitPoint, isa1 };
	struct ISA_part_arg thread1_arg = { splitPoint, len, isa2 };

	thrd_create(&thread0, compute_ISA_part, &thread0_arg);
	thrd_create(&thread1, compute_ISA_part, &thread1_arg);

	thrd_join(thread0, NULL);
	thrd_join(thread1, NULL);

  // compute ISA
	//isa = (int *)malloc(sizeof(int) * len);
	//for(i=0; i<len; i++) {
		//isa[sa[i]] = i;
	//}
}

