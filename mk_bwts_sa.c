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
static long len;
static saidx_t *sa;
static int *isa;

unsigned char *make_bwts_sa(void);
int move_lyndonword_head(int lw_start, int lw_len, int lw_rank);


int main(int argc, char **argv)
{
	if(argc < 2) {
		fprintf(stderr, "Usage: mk_bwts_sa <infile> [<outfile.bwts>]\n");
		fprintf(stderr, "If unspecified, output is written to standard output\n");
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

	unsigned char *bwts = make_bwts_sa();

	FILE *bwtsout = bwts_outfilename ? fopen(bwts_outfilename, "w") : stdout;
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
int move_lyndonword_head(int lw_start, int lw_len, int lw_rank)
{
    // Compare LW with the next-ranking suffix (lw_rank+1);
    //  We may stop when
    //  !(sa[lw_rank+1] > lw_start+lw_len)
    //  because any LW ranks lexicographically before any position earlier in the text (property of the LW factorization) and
    //  before any other position within itself (property of Lyndon words)
	while(lw_rank+1<len && (sa[lw_rank+1] > lw_start+lw_len)) {
		int next_rank_start = sa[lw_rank+1];

        int compare_len = MIN(lw_len, len - next_rank_start);

		int res = memcmp(&(T[lw_start]), &(T[next_rank_start]), compare_len);

        if(res < 0) {
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

		sa[lw_rank] = sa[lw_rank+1];
		isa[sa[lw_rank]] = lw_rank;
		lw_rank++;
	}
	sa[lw_rank] = lw_start;
	isa[lw_start] = lw_rank;

	return lw_rank;
}

unsigned char *make_bwts_sa(void)
{
	int min, min_i;
	int i;

    // compute ISA
	isa = (int *)malloc(sizeof(int) * len);
	for(i=0; i<len; i++) {
		isa[sa[i]] = i;
	}

	MARK_TIME("Compute ISA");

	min = isa[0];
	min_i = 0;
	for(i=1; i<len && min>0; i++) {

        // The next LW starts whenever the sort rank hits a new minimum
        //  (isa[i] = the sort rank at text position i)
		if(isa[i] < min) {
			int lw_start = min_i;

            // Re-rank the LW ending at i - 1
			int lw_head_rank = move_lyndonword_head(lw_start, i - min_i, min);

			int ref_rank = lw_head_rank;
			int j;

            // We iterate backward through the just-ranked Lyndon word, adjusting ranks
			for(j=i; j-->lw_start+1; ) {
				int test_rank = isa[j];
				int start_rank = test_rank;

				while(test_rank < len-1) {
					int next_rank_start = sa[test_rank+1];
					if(j > next_rank_start || T[j] != T[next_rank_start] || ref_rank < isa[next_rank_start+1]) {
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

			min = isa[i];
			min_i = i;
		}
	}
	free(sa);

	MARK_TIME("Fix sort order");

	unsigned char *bwts = (unsigned char *)malloc(len);

	min = len;
	min_i = 0;
	for(i=0; i<len; i++) {
		if(isa[i] < min) {
			if(min < len) {
				bwts[min] = T[i - 1];
			}

			min = isa[i];
			min_i = i;
		}
		else {
			bwts[isa[i]] = T[i - 1];
		}
	}
	bwts[0] = T[len - 1];
	
	MARK_TIME("Generate BWTS");

	free(isa);

	return bwts;
}
