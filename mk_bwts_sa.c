#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <divsufsort.h>
#include <stdint.h>
#include "map_file.h"

#define BWTS_EXTENSION ".bwts"

#ifdef SHOW_TIMINGS
#	include <time.h>
	static clock_t t, last_t, first_t;
#	define MARK_TIME(msg) \
		last_t=t, fprintf(stderr, "%-25s %6.2f\n", msg " time", (double)((t=clock()) - last_t) / (double)CLOCKS_PER_SEC);
#else
#	define MARK_TIME(msg) /*nothing*/
#endif

uint32_t binary_search_sa(uint32_t suff, uint8_t *T, uint32_t *sa, uint32_t len);

void make_bwts_sa(unsigned char *T, int32_t *SA, int len);

char *bwts_outfilename;

void make_outfilename(int argc, char **argv)
{
	if(argc < 3) {
		bwts_outfilename = (char *)malloc(strlen(BWTS_EXTENSION) + strlen(argv[1]) + 1);
		strcpy(bwts_outfilename, argv[1]);
		strcat(bwts_outfilename, BWTS_EXTENSION);
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
		fprintf(stderr, "If outfile name is -, output is written to standard output\n");
		exit(1);
	}

#ifdef SHOW_TIMINGS
	first_t = t = clock();
#endif

	make_outfilename(argc, argv);

	map_in(T, len, argv[1]);

	int32_t *SA = (int32_t *)malloc(sizeof(int32_t) * len);
	divsufsort(T, SA, len);

	MARK_TIME("Suffix sort");

	make_bwts_sa(T, SA, len);

#ifdef SHOW_TIMINGS
	fprintf(stderr, "%-25s %6.2f\n", "Total time", (double)(clock() - first_t) / (double)CLOCKS_PER_SEC);
#endif

	return 0;
}

void make_bwts_sa(unsigned char *T, int32_t *SA, int len)
{
	int min, min_i;
	int i, j;

	int *lyndonwords = (int *)malloc(sizeof(int) * len);

	int rank, factors=0, last_lw_pos=len;
	for(rank=0; SA[rank]!=0; rank++) {
		if(SA[rank] < last_lw_pos) {
			last_lw_pos = SA[rank];
			lyndonwords[factors++] = rank;
		}
	}

	MARK_TIME("Find Lyndon words");

	min = rank;
	min_i = 0;

	while(factors>0) {
		int isa_i = lyndonwords[--factors];
		i = SA[isa_i];

		int endsym = T[i-1];
		int pred_pass_count = 0;

		for(j=isa_i; j<min-1; j++) {
			pred_pass_count += (endsym == T[SA[j+1]-1]);
		}

		int lw_start = min_i;
		int lw_len = i - min_i;
		int test_rank = min;

		while(test_rank+1<len && (SA[test_rank+1] > lw_start+lw_len)) {
			int k = 0;
			while((SA[test_rank+1] + k < len) && T[lw_start + (k % lw_len)] == T[SA[test_rank+1] + k]) {
				k++;
			}
			if((SA[test_rank+1] + k < len) && T[lw_start + (k % lw_len)] < T[SA[test_rank+1] + k]) {
				break;
			}

			pred_pass_count += (endsym == T[SA[test_rank+1]-1]);

			SA[test_rank] = SA[test_rank+1];
			test_rank++;
		}
		SA[test_rank] = lw_start;



		for(j=i; j-->lw_start+1; ) { // iterate through the new lyndon word from end to start
			int num_to_move_down = pred_pass_count;
			int k;

			if(!num_to_move_down) {
				break;
			}
			pred_pass_count = 0;
			endsym = T[j-1];

			test_rank = binary_search_sa(j, T, (uint32_t*)SA, len);
			for(k=0; k<num_to_move_down; k++) {
				pred_pass_count += (endsym == T[SA[test_rank+1]-1]);

				SA[test_rank] = SA[test_rank+1];
				test_rank++;
			}
			SA[test_rank] = j;
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
		if(SA[i] < last_lw_pos) {
			outc = T[last_lw_pos-1];
			last_lw_pos = SA[i];
		}
		else {
			outc = T[SA[i]-1];
		}
		putc_unlocked(outc, bwtsout);
	}

	MARK_TIME("Write BWTS");
}
