/* enable.h - interrupt enable/disable macros */

/*
 *   RTXC    Version 3.2
 *   Copyright (c) 1986-1997.
 *   Embedded System Products, Inc.
 *   ALL RIGHTS RESERVED
*/

#ifndef _ENABLE_H /* { */
#define _ENABLE_H

#ifdef INLINE_INTS

#error "Inline not available."

#endif // INLINE_INTS {

#define ENABLE	_enable()
#define DISABLE _disable()
#define POPCCR(A) _popccr(A)
#define PUSHCCR _pushccr()

extern void _enable(void);
extern void _disable(void);
extern void _popccr(unsigned long xcc);
extern unsigned long _pushccr(void);

extern char isrcnt ;                    // count of "levels" of interrupts, initially = 0

// ARM SP points to last used location

#endif /* } _ENABLE_H */

/* end of enable.h */

