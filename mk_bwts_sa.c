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

#ifdef SHOW_DEBUG
# define eprintf(...) fprintf(stderr, __VA_ARGS__);
#else
# define eprintf(...) /*nothing*/
#endif

uint32_t binary_search_sa(uint32_t suff, uint8_t *T, uint32_t *sa, uint32_t len);
//uint32_t linear_search_sa(uint32_t suff, uint8_t *T, uint32_t *sa, uint32_t off, uint32_t len);

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
	int lwar, lwap;
	int i, j;

	int *lyndonwords = (int *)malloc(sizeof(int) * len);

	int rank, lwnum=0, last_lw_pos=len;
	for(rank=0; SA[rank]!=0; rank++) {
		if(SA[rank] < last_lw_pos) {
			last_lw_pos = SA[rank];
			lyndonwords[lwnum++] = rank;
		}
	}

	MARK_TIME("Find Lyndon words");

	lwar = rank;
	lwap = 0;

	// Scan LWs: rank->zero, position->len
	while(lwnum>0) {
		int lwbr = lyndonwords[--lwnum];
		int lwbp = SA[lwbr];

		int lw_start = lwap;
		int lw_len = lwbp - lwap;

		int endsym = T[lwbp-1];
		int pred_pass_count = 0;

		for(j=lwbr+1; j<lwar; j++) {
			pred_pass_count += (endsym == T[SA[j]-1]);
		}


		int initrank = lwar;
		while(lwar+1<len && (SA[lwar+1] > lw_start+lw_len)) {
			int k = 0;
			while((SA[lwar+1] + k < len) && T[lw_start + (k % lw_len)] == T[SA[lwar+1] + k]) {
				k++;
			}
			if((SA[lwar+1] + k < len) && T[lw_start + (k % lw_len)] < T[SA[lwar+1] + k]) {
				break;
			}

			pred_pass_count += (endsym == T[SA[lwar+1]-1]);

			SA[lwar] = SA[lwar+1];
			lwar++;
		}
		SA[lwar] = lw_start;
		eprintf("head %4d -> %4d (%d)\n", initrank, lwar, lwar - initrank);



		for(j=lwbp; j-->lw_start+1; ) { // iterate through the new lyndon word from end to start
			int num_to_move_down = pred_pass_count;
			int k;

			if(!num_to_move_down) {
				break;
			}
			pred_pass_count = 0;
			endsym = T[j-1];

			uint32_t lwir = binary_search_sa(j, T, (uint32_t*)SA, len);
      while(SA[lwir] != j) lwir--;

			int initlwir = lwir;
			for(k=0; k<num_to_move_down; k++) {
				pred_pass_count += (endsym == T[SA[lwir+1]-1]);

				SA[lwir] = SA[lwir+1];
				lwir++;
			}
			SA[lwir] = j;

			eprintf("tail %4d -> %4d (%d)\n", initlwir, lwir, lwir - initlwir);
		}

		lwar = lwbr;
		lwap = lwbp;
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
