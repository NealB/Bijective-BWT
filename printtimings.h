#ifndef __PRINTTIMINGS_H
#define __PRINTTIMINGS_H

#ifndef eprintf
#  define eprintf(...) fprintf(stderr, __VA_ARGS__);
#endif

#ifdef SHOW_TIMINGS
#  include <time.h>
  static clock_t t, last_t, first_t;
#  define INIT_TIME \
    first_t = t = clock();
#  define MARK_TIME(msg) \
		last_t=t, eprintf("%-25s %6.2f\n", msg " time", (double)((t=clock()) - last_t) / (double)CLOCKS_PER_SEC);
#  define TOTAL_TIME \
    eprintf("%-25s %6.2f\n", "Total time", (double)(clock() - first_t) / (double)CLOCKS_PER_SEC);
#else
#  define INIT_TIME
#  define MARK_TIME(msg)
#  define TOTAL_TIME
#endif

#endif
