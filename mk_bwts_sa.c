#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <divsufsort.h>
#include "map_file.h"

#ifdef SHOW_TIMINGS
#include <time.h>
clock_t last_clock, t;
#endif

unsigned char *make_bwts_sa(unsigned char *T, int len);


int main(int argc, char **argv)
{
	unsigned char *T;
	long len;

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

	unsigned char *bwts = make_bwts_sa(T, len);

	FILE *bwtsout = bwts_outfilename ? fopen(bwts_outfilename, "w") : stdout;
	if(!bwtsout) {
		fprintf(stderr, "Couldn't open BWTS file for writing\n");
		perror(bwts_outfilename);
		exit(1);
	}
	fwrite(bwts, 1, len, bwtsout);

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Write BWTS time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	return 0;
}

unsigned char *make_bwts_sa(unsigned char *T, int len)
{
	int min, min_i;
	int i;

	saidx_t *sa = (saidx_t *)malloc(sizeof(saidx_t) * len);
	divsufsort(T, sa, len);

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Suffix sort time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	int *isa = (int *)malloc(sizeof(int) * len);
	for(i=0; i<len; i++) {
		isa[sa[i]] = i;
	}

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Compute ISA time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	min = len-1;
	min_i = 0;
	for(i=0; i<len && min>0; i++) {
		if(isa[i] < min) {
			if(i>0) {
				int lw_start = min_i;
				int lw_len = i - min_i;
				int test_rank = min;

				while(test_rank+1<len && (sa[test_rank+1] > lw_start+lw_len)) {
					int k = 0;
					while((sa[test_rank+1] + k < len) && (k < lw_len) && T[lw_start + k] == T[sa[test_rank+1] + k]) {
						k++;
					}
					if((sa[test_rank+1] + k < len) && (k < lw_len) && T[lw_start + k] < T[sa[test_rank+1] + k]) {
						break;
					}
					if((k == lw_len) && (test_rank < isa[sa[test_rank+1] + k])) {
						break;
					}

					sa[test_rank] = sa[test_rank+1];
					isa[sa[test_rank]] = test_rank;
					test_rank++;
				}
				sa[test_rank] = lw_start;
				isa[sa[test_rank]] = test_rank;

				int ref_rank = test_rank;
				int j;
				for(j=i; j-->lw_start+1; ) { // iterate through the new lyndon word from end to start
					test_rank = isa[j];
					int start_rank = test_rank;
					while(test_rank < len-1) {
						if(j > sa[test_rank+1] || T[j] != T[sa[test_rank+1]] || ref_rank < isa[sa[test_rank+1]+1]) {
							break;
						}

						sa[test_rank] = sa[test_rank+1];
						isa[sa[test_rank]] = test_rank;
						test_rank++;
					}
					sa[test_rank] = j;
					isa[sa[test_rank]] = test_rank;

					ref_rank = test_rank;

					if(start_rank == test_rank) {
						break;
					}
				}
			}

			min = isa[i];
			min_i = i;
		}
	}
	free(sa);

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Fix sort order time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

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
	
#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Generate BWTS time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	free(isa);

	return bwts;
}
