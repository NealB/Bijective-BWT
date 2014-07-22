#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <divsufsort.h>
#include "map_file.h"

#ifdef SHOW_TIMINGS
#include <time.h>
clock_t last_clock, t;
#endif

static unsigned char *T;
static long len;
static saidx_t *sa;
static int *isa;

unsigned char *make_bwts_sa(void);
int move_lyndonword_head(int lw_start, int lw_len, int lw_rank);

#define eprintf(...) fprintf(stderr, __VA_ARGS__);

char *add_extension(char *name)
{
	static char buf[512];

	sprintf(buf, "%s.obwts", name);
	return buf;
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		fprintf(stderr, "Usage: mk_bwts_sa <infile> [<outfile.bwts>]\n");
		fprintf(stderr, "If unspecified, output is written to standard output\n");
		exit(1);
	}

	char *bwts_outfilename = argc < 3 ? add_extension(argv[1]) : argv[2];
	map_in(T, len, argv[1]);

#ifdef SHOW_TIMINGS
	last_clock = clock();
#endif

	sa = (saidx_t *)malloc(sizeof(saidx_t) * len);
	divsufsort(T, sa, len);

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Suffix sort time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	unsigned char *bwts = make_bwts_sa();

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

int move_lyndonword_head(int lw_start, int lw_len, int lw_rank)
{
	int lw_initrank = lw_rank;
	while(lw_rank+1<len && (sa[lw_rank+1] > lw_start+lw_len)) {
		int k = 0;
		int next_rank_start = sa[lw_rank+1];

		while((next_rank_start + k < len) && (k < lw_len) && T[lw_start + k] == T[next_rank_start + k]) {
			k++;
		}
		if((next_rank_start + k < len) && (k < lw_len) && T[lw_start + k] < T[next_rank_start + k]) {
			break;
		}
		if((k == lw_len) && (lw_rank < isa[next_rank_start + k])) {
			break;
		}

		sa[lw_rank] = sa[lw_rank+1];
		isa[sa[lw_rank]] = lw_rank;
		lw_rank++;
	}
	sa[lw_rank] = lw_start;
	isa[sa[lw_rank]] = lw_rank;

	eprintf("head %4d -> %4d (%d)\n", lw_initrank, lw_rank, lw_rank - lw_initrank);

	return lw_rank;
}

unsigned char *make_bwts_sa(void)
{
	int min, min_i;
	int i;


	isa = (int *)malloc(sizeof(int) * len);
	for(i=0; i<len; i++) {
		isa[sa[i]] = i;
	}

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Compute ISA time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	min = isa[0];
	min_i = 0;
	for(i=1; i<len && min>0; i++) {
		if(isa[i] < min) {
			int lw_start = min_i;
			int lw_head_rank = move_lyndonword_head(lw_start, i - min_i, min);

			int ref_rank = lw_head_rank;
			int j;
			for(j=i; j-->lw_start+1; ) { // iterate through the new lyndon word from end to start
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
				isa[sa[test_rank]] = test_rank;

				ref_rank = test_rank;

				if(start_rank == test_rank) {
					break;
				}

				eprintf("tail %4d -> %4d (%d)\n", start_rank, test_rank, test_rank - start_rank);
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
