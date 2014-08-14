#ifndef __PRINTDEBUG_H
#define __PRINTDEBUG_H


#define DISP_MOVE_HEAD(fromrank, torank) dbg_printf("head %4d -> %4d (%d)\n", fromrank, torank, torank - fromrank);
#define DISP_MOVE_TAIL(fromrank, torank) dbg_printf("tail %4d -> %4d (%d)\n", fromrank, torank, torank - fromrank);
#define DISP_LW(num) dbg_printf("\nLW #%d ===========\n\n", num);
#endif
