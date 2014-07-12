#include <stdint.h>
#include <sys/param.h>

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

	while (imax >= imin)
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
  return -1;
}

