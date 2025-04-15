/* cqueue.h - RTXC Queue include file */

#include "extapi.h"             // must be before cqueue.h

#define NAMMAX 8

enum QUEUELIST {
FIRSTQUEUE = 0, // dummy, never used

COM0IQ,         // COM 0 input
COM0OQ,         // COM 0 output
COM1IQ,         // COM 1 input
COM1OQ,         // COM 1 output
COM2IQ,         // COM 2 input
COM2OQ,         // COM 2 output
COM3IQ,         // COM 3 input
COM3OQ,         // COM 3 output

#ifdef USE_COM4_ON_ARM
COM4IQ,         // COM 4 input
COM4OQ,         // COM 4 output
#endif // USE_COM4_ON_ARM

#ifdef USE_USB_ON_ARM
USBIQ,          // COM 3 input
USBOQ,          // COM 3 output
#endif // USE_USB_ON_ARM

LU0Q,           // LU 0 Queue
LU1Q,           // LU 1 Queue
LU2Q,           // LU 2 Queue
LU3Q,           // LU 3 Queue
LU4Q,           // LU 4 Queue

MAXQUEUES       // evaluate total number
} ;

#define NQUEUES (MAXQUEUES-1)

extern const QUEUE nqueues ;

extern QHEADER qheader[1+NQUEUES] ;

extern const QKHEADER qkkheader[1+NQUEUES] ;

#ifdef CBUG
extern const char queuekname[1+NQUEUES][NAMMAX+1] ;
#endif // CBUG

