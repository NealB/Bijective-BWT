#ifndef __PRINTDEBUG_H
#define __PRINTDEBUG_H

#define eprintf(...) fprintf(stderr, __VA_ARGS__);

#ifdef SHOW_DEBUG
# define dbg_printf(...) eprintf(__VA_ARGS__);
#else
# define dbg_printf(...) /*nothing*/
#endif

#if defined(SHOW_DEBUG) && defined(SHOW_HEAD_DEBUG)
# define DISP_MOVE_HEAD(fromrank, torank) eprintf("head %4d -> %4d (%d)\n", fromrank, torank, torank - fromrank);
#else
# define DISP_MOVE_HEAD(...) /*nothing*/
#endif

#if defined(SHOW_DEBUG) && defined(SHOW_TAIL_DEBUG)
# define DISP_MOVE_TAIL(fromrank, torank) eprintf("tail %4d -> %4d (%d)\n", fromrank, torank, torank - fromrank);
#else
# define DISP_MOVE_TAIL(...) /*nothing*/
#endif

#if defined(SHOW_DEBUG) && defined(SHOW_LWNUM_DEBUG)
# define DISP_LW(num) eprintf("\nLW #%d ===========\n\n", num);
#else
# define DISP_LW(...) /*nothing*/
#endif

#endif
