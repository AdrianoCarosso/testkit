/* cclock.c - RTXC Clock definitions - ARM */

#include "typedef.h"
#include "rtxstruc.h"

#include "cclock.h"

#ifdef HAS_ALLOC_TIMER /* { */
const int ntmrs = NTMRS;
#endif /* } HAS_ALLOC_TIMER */

//const int clktick = CLKTICK;
//const int clkrate = CLKRATE;

#ifdef HAS_ALLOC_TIMER /* { */
CLKBLK clkq[NTMRS];
#endif /* } HAS_ALLOC_TIMER */

