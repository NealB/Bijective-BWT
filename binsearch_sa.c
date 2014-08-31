#include <stdint.h>
#include <sys/param.h>
#include <assert.h>
#include <stdio.h>

int suffixcmp(uint32_t s0, uint32_t s1, uint8_t *T, uint32_t len, uint32_t *cp)
{
	uint32_t smax = MAX(s0, s1);
	int d = 0;
	while((smax + *cp < len)) {
		d = T[s0 + *cp] - T[s1 + *cp];
		if(d) {
			return d;
		}
		(*cp)++;
	}
	return s1 - s0;
}

uint32_t binary_search_sa(uint32_t suff, uint8_t *T, uint32_t *sa, uint32_t len)
{
	uint32_t imin=0, imax=len-1;
	uint32_t imin_cp=0, imax_cp=0;

  while (imax > imin)
	{
		uint32_t imid = (imax - imin)/2 + imin;

		if(sa[imid] == suff) {
			return imid;
		}

		uint32_t cp = MIN(imin_cp, imax_cp);
		if (suffixcmp(suff, sa[imid], T, len, &cp) > 0) {
			imin = imid + 1;
			imin_cp = cp;
		}
		else {
			imax = imid - 1;
			imax_cp = cp;
		}
	}

  return imax;
}

uint32_t lw_head_search(uint32_t curr_lw_rank, uint32_t next_lw_rank, uint32_t lw_len, uint8_t *T, uint32_t *sa, uint32_t len)
{
  uint32_t curr_lw_start_pos = sa[curr_lw_rank];
  uint32_t max = next_lw_rank - 1, min = curr_lw_rank + 1;

  uint32_t mid = min;
  int phase = 0;
  while(max >= min) {
    if(phase == 0 && min > curr_lw_rank + 1) {
      int next = (mid - curr_lw_rank) * 2 + curr_lw_rank;
      mid = (next < max) ? next : max;
    }
    else if(phase == 1) {
      mid = (max - min)/2 + min;
    }

    int k = 0;
    while((sa[mid] + k < len) && T[curr_lw_start_pos + (k % lw_len)] == T[sa[mid] + k]) {
      k++;
    }
    if((sa[mid] + k < len) && T[curr_lw_start_pos + (k % lw_len)] < T[sa[mid] + k]) {
			max = mid - 1;
      phase = 1;
		}
		else {
			min = mid + 1;
    }
  }
  return max;
}
