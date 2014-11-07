#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <divsufsort.h>
#include <stdint.h>
#include <libgen.h>
#include <assert.h>
#include "map_file.h"

#define BWTS_EXTENSION ".bwts"

#include <time.h>
static clock_t t, last_t, first_t;
#define INIT_TIME \
    first_t = t = clock();
#define MARK_TIME(msg) \
		last_t=t, fprintf(stderr, "%-25s %6.2f\n", msg " time", (double)((t=clock()) - last_t) / (double)CLOCKS_PER_SEC);

#define eprintf(...) fprintf(stderr, __VA_ARGS__);

#include "printdebug.h"

int32_t binary_search_sa(int32_t suff, uint8_t *T, int32_t *sa, int32_t len);
int32_t lw_head_search(int32_t curr_lw_rank, int32_t next_lw_rank, int32_t lw_len, uint8_t *T, int32_t *sa, int32_t len);

void make_bwts_sa(unsigned char *T, int32_t *SA, int len);

FILE *bwtsout;

char *get_outfilename(int argc, char **argv)
{
	if(argc >= 3) {
    return argv[2];
	}
  else {
    char *outnamebuf, s[strlen(argv[1]) + 1];
    strcpy(s, argv[1]);
    if(asprintf(&outnamebuf, "%s%s", basename(s), BWTS_EXTENSION) == -1) {
      eprintf("Failed to create output filename. Abort.\n");
      exit(1);
    }
    return outnamebuf;
  }
}

int main(int argc, char **argv)
{
  char *bwts_outfilename;
	unsigned char *T;
	long len;

	if(argc < 2) {
		fprintf(stderr, "Usage: mk_bwts_sa <infile> [<outfile.bwts>]\n");
		fprintf(stderr, "If outfile name is -, output is written to standard output\n");
    fprintf(stderr, "build: %s\n", __TIMESTAMP__);
		exit(1);
	}

  INIT_TIME;

	bwts_outfilename = get_outfilename(argc, argv);

	map_in(T, len, argv[1]);
  if(T == NULL) {
    eprintf("Failed to mmap input file. Abort.\n");
    exit(1);
  }

	bwtsout = (strcmp(bwts_outfilename, "-")==0) ? stdout : fopen(bwts_outfilename, "w");
	if(!bwtsout) {
		perror(bwts_outfilename);
		eprintf("Couldn't open BWTS file for writing\n");
		exit(1);
	}

	size_t SAbs = sizeof(int32_t) * len;
	int32_t *SA = (int32_t *)malloc(SAbs);
	if(SA == NULL) {
		fprintf(stderr, "Failed to allocate %ld bytes for suffix array. Abort.\n", SAbs);
		exit(1);
	}
	divsufsort(T, SA, len);

	MARK_TIME("Suffix sort");

	make_bwts_sa(T, SA, len);

#ifdef SHOW_TIMINGS
	fprintf(stderr, "%-25s %6.2f\n", "Total time", (double)(clock() - first_t) / (double)CLOCKS_PER_SEC);
#endif

	return 0;
}

int32_t rank_suffix(int suff, unsigned char *T, saidx_t *SA, long len) {
  vdbg_printf("$$$$$$ Binary Search $$$$$$\n");
  int32_t rank = binary_search_sa(suff, T, SA, len);
  while(SA[rank] != suff) rank--;
  return rank;
}

void demote_rank(saidx_t *SA, long initial_rank, long num) {
  saidx_t value = SA[initial_rank];
  memmove(&SA[initial_rank], &SA[initial_rank + 1], sizeof(*SA) * num);
  SA[initial_rank + num] = value;
}

void make_bwts_sa(unsigned char *T, int32_t *SA, int len)
{
	int lwar, lwap;
	int i, j;

	size_t LWbs = sizeof(int) * len;
	int *lyndonwords = (int *)malloc(LWbs);
	if(lyndonwords == NULL) {
		fprintf(stderr, "Failed to allocate %ld bytes for lyndon word starting indices. Abort.\n", LWbs);
		exit(1);
	}

	int rank, lwnum=0, last_lw_pos=len;
	for(rank=0; SA[rank]!=0; rank++) {
		if(SA[rank] < last_lw_pos) {
			last_lw_pos = SA[rank];
			lyndonwords[lwnum++] = rank;
		}
	}

	MARK_TIME("Find Lyndon words");

  dbg_printf("Lyndon word count: %d\n", lwnum);

	lwar = rank;
	lwap = 0;

  int lwi = 0;
  int lwzr = len; // rank of the next LW after lwa

	// Scan LWs: rank->zero, position->len
	while(lwnum>0) {
    DISP_LW(lwi) lwi++;

		int lwbr = lyndonwords[--lwnum];
		int lwbp = SA[lwbr];

		int lw_start = lwap;
		int lw_len = lwbp - lwap;

    // Move LW head down further if necessary, adding like symbols passed in BWT column, if any.
		int initrank = lwar;

    lwar = lw_head_search(lwar, lwzr, lw_len, T, SA, len);

    if(lwar != initrank) {
      demote_rank(SA, initrank, lwar - initrank);
    }
		DISP_MOVE_HEAD(initrank, lwar);

		int endsym = T[lwbp-1];
		int pred_pass_count = 0;

    // Simulate move from LWB->LWA, counting like symbols passed in BWT column.
		for(j=lwbr+1; j<lwar; j++) {
			pred_pass_count += (endsym == T[SA[j]-1]);
		}

    int delta = 0;
    int anchor_pos = lwbp-1;
    int anchor_rank;
    while(pred_pass_count > 0) {
      j = anchor_pos - delta;
		  if(j <= lw_start) {
        break;
      }
			int num_to_move_down = pred_pass_count;
			int k;

      if(delta==0) {
        anchor_rank = rank_suffix(anchor_pos, T, SA, len);
      }

//count_passes:
      int new_pass_count = 0;
      for(k=0; k<num_to_move_down; k++) {
        new_pass_count += (T[j-1] == T[SA[anchor_rank+1+k]-1-delta]);
      }
      vdbg_printf(".... num_to_move_down = %d ... passes at position %d: %d\n", num_to_move_down, j, new_pass_count);

      if(new_pass_count == num_to_move_down) {
        delta++;
        vdbg_printf("<<<< skipping move (%d==%d)\n", num_to_move_down, new_pass_count);
        continue;
      }
      vdbg_printf(".... moving down (%d!=%d)\n", num_to_move_down, new_pass_count);

//move_tail_down:
      int lwir = delta==0 ? anchor_rank : rank_suffix(j, T, SA, len);
      demote_rank(SA, lwir, num_to_move_down);

      DISP_MOVE_TAIL(lwir, lwir + num_to_move_down);

      if(new_pass_count == 0) {
        break;
      }

      pred_pass_count = new_pass_count;
      anchor_pos = j-1;
      delta = 0;
		}

    lwzr = lwar;
		lwar = lwbr;
		lwap = lwbp;
	}

	MARK_TIME("Fix sort order");

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
