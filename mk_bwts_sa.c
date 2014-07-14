#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <divsufsort.h>
#include <stdint.h>
#include "map_file.h"

#ifdef SHOW_TIMINGS
#	include <time.h>
	static clock_t t, last_t;
#	define MARK_TIME(msg) \
		last_t=t, fprintf(stderr, "%-25s %6.2f\n", msg " time", (double)((t=clock()) - last_t) / (double)CLOCKS_PER_SEC);
#else
#	define MARK_TIME(msg) /*nothing*/
#endif

void make_bwts_sa(unsigned char *T, int len);

char *text_infilename;
char *bwts_outfilename;

void make_outfilename(int argc, char **argv)
{
	if(argc < 3) {
		asprintf(&bwts_outfilename, "%s.bwts", text_infilename);
	}
	else {
		bwts_outfilename = argv[2];
	}
}

int main(int argc, char **argv)
{
	unsigned char *T;
	long len;

	if(argc < 2) {
		fprintf(stderr, "Usage: mk_bwts_sa <infile> [<outfile.bwts>]\n");
		fprintf(stderr, "If unspecified, output is written to standard output\n");
		exit(1);
	}
	text_infilename = argv[1];

#ifdef SHOW_TIMINGS
	t = clock();
#endif

	make_outfilename(argc, argv);

	map_in(T, len, argv[1]);

	make_bwts_sa(T, len);

	return 0;
}

void make_bwts_sa(unsigned char *T, int len)
{
	int min, min_i;
	int i, j;

	saidx_t *sa = (saidx_t *)malloc(sizeof(saidx_t) * len);
	divsufsort(T, sa, len);

	MARK_TIME("Suffix sort");

	int *lyndonwords = (int *)malloc(sizeof(int) * len);

	int rank, factors=0, last_lw_pos=len;
	for(rank=0; sa[rank]!=0; rank++) {
		if(sa[rank] < last_lw_pos) {
			last_lw_pos = sa[rank];
			lyndonwords[factors++] = rank;
		}
	}

	MARK_TIME("Find Lyndon words");

	min = rank;
	min_i = 0;

	while(factors>0) {
		int isa_i = lyndonwords[--factors];
		i = sa[isa_i];

		int endsym = T[i-1];
		int pred_pass_count = 0;

		for(j=isa_i; j<min-1; j++) {
			pred_pass_count += (endsym == T[sa[j+1]-1]);
		}

		int lw_start = min_i;
		int lw_len = i - min_i;
		int test_rank = min;

		while(test_rank+1<len && (sa[test_rank+1] > lw_start+lw_len)) {
			int k = 0;
			while((sa[test_rank+1] + k < len) && T[lw_start + (k % lw_len)] == T[sa[test_rank+1] + k]) {
				k++;
			}
			if((sa[test_rank+1] + k < len) && T[lw_start + (k % lw_len)] < T[sa[test_rank+1] + k]) {
				break;
			}

			pred_pass_count += (endsym == T[sa[test_rank+1]-1]);

			sa[test_rank] = sa[test_rank+1];
			test_rank++;
		}
		sa[test_rank] = lw_start;



		for(j=i; j-->lw_start+1; ) { // iterate through the new lyndon word from end to start
			int num_to_move_down = pred_pass_count;
			int k;

			if(!num_to_move_down) {
				break;
			}
			pred_pass_count = 0;
			endsym = T[j-1];

			test_rank = binary_search_sa(j, T, sa, len);
			for(k=0; k<num_to_move_down; k++) {
				pred_pass_count += (endsym == T[sa[test_rank+1]-1]);

				sa[test_rank] = sa[test_rank+1];
				test_rank++;
			}
			sa[test_rank] = j;
		}

		min = isa_i;
		min_i = i;
	}

	MARK_TIME("Fix sort order");

	FILE *bwtsout = (strcmp(bwts_outfilename, "-")==0) ? stdout : fopen(bwts_outfilename, "w");
	if(!bwtsout) {
		fprintf(stderr, "Couldn't open BWTS file for writing\n");
		perror(bwts_outfilename);
		exit(1);
	}

	last_lw_pos = len;
	for(i=0; i<len; i++) {
		int outc;
		if(sa[i] < last_lw_pos) {
			outc = T[last_lw_pos-1];
			last_lw_pos = sa[i];
		}
		else {
			outc = T[sa[i]-1];
		}
		putc_unlocked(outc, bwtsout);
	}

	MARK_TIME("Write BWTS");
}
