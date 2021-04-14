/* vim: set noai ts=2 sw=2: */

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


int main(int argc, char **argv) {
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

unsigned char *make_bwts_sa(void) {
	int min, min_i;
	int i;

    // compute ISA
	isa = (int *)malloc(sizeof(int) * len);
	for(i=0; i<len; i++) {
		isa[sa[i]] = i;
	}

	MARK_TIME("Compute ISA");

	min = isa[0];
	min_i = 0; // index of the currently smallest ISA value up so far
	for(i=1; i<len && min>0; i++) { // O(n) -> number of text positions
		if(isa[i] < min) { // only take action if isa[i] < min -> found the next Lyndon factor
			const int lw_start = min_i;
			const int lw_head_rank = move_lyndonword_head(lw_start, i - min_i, min);

			int ref_rank = lw_head_rank; // SA position of the previous suffix we fixed
			// j: text position
			for(int j = i; j-- > lw_start+1; ) { // iterate through the new lyndon word from end to start
				int test_rank = isa[j];
				const int start_rank = test_rank;
				// task: move the suffix T[j..] to the right position in SA within its respective bucket
				while(test_rank < len-1) { // for loop in SA[ISA[j]..len-1]
					const int next_rank_start = sa[test_rank+1]; // lexicographic successor of T[j..], text position
					if(j > next_rank_start  // we only shft suffixes in SA downwards when they start earlier in text order
							|| T[j] != T[next_rank_start]  // if we are the last in the bucket with the same starting character -> stop
							|| ref_rank < isa[next_rank_start+1] // ref_rank is the rank of T[j+1..]. If ref_rank is smaller than the rank of T[next_rank_start..] this means that an inducing step would also put T[j..] to the left of T[next_rank_start..] -> stop
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

				if(start_rank == test_rank) { // did we modify test_rank?
					break; // if not, then we cannot change the order of the subsequent ranks as this would be a contradiction to induce sorting
				}
			}

			min = isa[i];
			min_i = i;
		}
	}
	free(sa);

	MARK_TIME("Fix sort order");

	unsigned char *bwts = (unsigned char *)malloc(len);

	min = len; // text position of current min
	min_i = 0; // index in ISA of current min
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
