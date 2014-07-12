#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <divsufsort.h>
#include <stdint.h>
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

int duval_2_1(unsigned char *str, int len, int *ans) // Duval 1983, linear-time and O(1)-space.
{
	int i, j, k = 0;
	int factors = 0;
	while( k < len )
	{
		i = k; j = k+1;
		while( 1 )
		{
			if( j < len )
			{
				int str_i = str[i], str_j = str[j];
				if( str_i < str_j )
				{
					i = k; j = j+1;
					continue;
				}
				else if( str_i == str_j )
				{
					i = i+1;
					j = j+1;
					continue;
				}
			}
			do {
				k += (j-i);
				ans[ factors++ ] = k;
			} // e.g., ...ababab...
			while( k <= i );
			// k > i
			break;
		}//while true
	}//while k
	return factors;
}//duval_2_1(str)

unsigned char *make_bwts_sa(unsigned char *T, int len)
{
	int min, min_i;
	int i, j;

	saidx_t *sa = (saidx_t *)malloc(sizeof(saidx_t) * len);
	divsufsort(T, sa, len);

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Suffix sort time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	int *lyndonwords = (int *)malloc(sizeof(int) * len);
	//for(i=0; i<len; i++) {
		//isa[sa[i]] = i;
	//}
	int factors = duval_2_1(T, len, lyndonwords);
	factors--;

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Compute ISA time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	min = binary_search_sa(0, T, sa, len);
	min_i = 0;
	//for(i=0; i<len && min>0; i++) {
		//if(isa[i] < min) {
	int n;
	for(n=0; n<factors; n++) {
		i = lyndonwords[n];
		int isa_i = binary_search_sa(i, T, sa, len);

				int endsym = T[i-1];
				int pred_pass_count = 0;

				for(j=isa_i; j<min-1; j++) {
					pred_pass_count += (endsym == T[sa[j+1]-1]);
				}

			if(i>0) {
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
					//isa[sa[test_rank]] = test_rank;
					test_rank++;
				}
				sa[test_rank] = lw_start;
				//isa[sa[test_rank]] = test_rank;



				int ref_rank = test_rank;
				for(j=i; j-->lw_start+1; ) { // iterate through the new lyndon word from end to start
					int num_to_move_down = pred_pass_count;
					pred_pass_count = 0;
					endsym = T[j-1];


					//test_rank = isa[j];
					test_rank = binary_search_sa(j, T, sa, len);
					int start_rank = test_rank;

					while(test_rank < len-1) {
						if(!(num_to_move_down--)) {
							break;
						}
#if 0
						if(j > sa[test_rank+1] || T[j] != T[sa[test_rank+1]] || ref_rank < isa[sa[test_rank+1]+1]) {
							break;
						}
#endif
						pred_pass_count += (endsym == T[sa[test_rank+1]-1]);

						sa[test_rank] = sa[test_rank+1];
						//isa[sa[test_rank]] = test_rank;
						test_rank++;
					}
					sa[test_rank] = j;
					//isa[sa[test_rank]] = test_rank;

					ref_rank = test_rank;

					if(start_rank == test_rank) {
						break;
					}
				}
			}

			min = isa_i;
			min_i = i;
	}
	//free(sa);

#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Fix sort order time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif


	unsigned char *bwts = (unsigned char *)malloc(len);

	j = factors;
	int lw_end_symbol = T[len-1];
	for(i=0; i<len; i++) {
		if(sa[i]==0 || (j>0 && (sa[i] == lyndonwords[j-1]))) {
			bwts[i] = lw_end_symbol;
			if(sa[i]>0) {
				lw_end_symbol = T[sa[i]-1];
			}
			j--;
		}
		else {
			bwts[i] = T[sa[i]-1];
		}
	}

#if 0
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
#endif
	
#ifdef SHOW_TIMINGS
	t = clock();
	fprintf(stderr, "Generate BWTS time %0.3f\n", (double)(t - last_clock) / (double)CLOCKS_PER_SEC);
	last_clock = t;
#endif

	//free(isa);

	return bwts;
}
