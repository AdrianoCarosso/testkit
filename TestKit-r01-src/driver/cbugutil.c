// cbugutil.c -- debug interface utility collection

//
//   RTXC    Version 3.2
//   Copyright (c) 1986-1997.
//   Embedded System Products, Inc.
//   ALL RIGHTS RESERVED
//
//
//   Copyright (c) 1997-2004.
//   T.E.S.T. srl
//

#include <stdio_console.h>
#include <stdlib.h>
#include <string.h>

#include "rtxcapi.h"
#include "tstate.h"

#include "rtxcbug.h"

#include "cqueue.h"
#include "cres.h"
#include "csema.h"
#include "cpart.h"
#include "cclock.h"

#include "extapi.h"

#define NULLCLK  ((CLKBLK *)0)
#ifndef NULL
#define NULL	 ((void *)0)
#endif

#define NAMMAX	 8	// max # chars in task/queue/etc names

#define SELFTASK ((TASK)0)

#define ESC	27
#define BS	 8
#define DEL    127

extern SEMA siglist[] ;

// definitions included in rtxcapi.h
//extern TCB rtxtcb[] ;
//extern const char taskname[][NAMMAX+1] ;
//extern const TASK ntasks ;
//#ifdef DYNAMIC_TASKS
//extern const TASK dntasks ;
//#endif

//#ifdef HAS_PARTITIONS
//extern const char partname[][NAMMAX+1] ;
//extern const MAP nparts ;
//#ifdef DYNAMIC_PARTS
//extern const MAP dnparts ;
//#endif
//#endif

extern CLKBLK * clkqptr ;
extern TICKS rtctick ;

extern char isrmax ;

char *xtaskname(TASK task) ;
char *xsemaname(int s) ;
#ifdef HAS_QUEUES
char *xqueuename(int q) ;
#endif
#ifdef HAS_PARTITIONS
char *xpartname(MAP map) ;
#endif
#ifdef HAS_RESOURCES
char *xresname(int r) ;
#endif

void showtime(OBJTYPE objtype, TASK task, int objid) ;

void taskmgr(void) ;

#ifdef TASKDETAIL
TASK gettask(void) ;
#endif	// TASKDETAIL

PRIORITY getpriority(void) ;
#ifdef TIME_SLICE
TICKS getslice(void) ;
#endif

void inputs(char *, short len) ;	// get '\r' terminated line from input

//void outputs(const char *); /* := outputs(buff) */
//void outputf(char *, void *, ...); /* := formatv + output */
//extern int format(char *, int **, char *);
//extern int formatv(char *, char *, ...);

extern char cbugflag ;		// clock ISR ignore flag, initially = 0

// static TICKS et;

// void sgnlcbug(void) ;

#ifdef CBUG /* { */

#ifdef TASKDETAIL
TASK gettask(void)
{
    TASK task ;
    char string[16] ;	// 12345678\0

    printf("\nTask (# or name)> ") ;
    inputs(string) ;
    printf("\n") ;

    if (string[0] == '-')       // if first char is '-'
	return(-1) ;		// assume -1 = ALL for block/unblock

    if ( (string[0] >= '0') && (string[0] <= '9') ) {
	task = atoi(string) ;

#ifdef DYNAMIC_TASKS
	if ( (task < 1) || (task > ntasks + dntasks) )
#else
	if ( (task < 1) || (task > ntasks) )
#endif
	    task = 0 ;

	return(task) ;
    } else {
#ifdef DYNAMIC_TASKS
	for(task=1 ; task <= ntasks + dntasks ; task++) {
#else
	for(task=1 ; task <= ntasks ; task++) {
#endif

	    if (!strcmp(string, xtaskname(task)))
		return(task) ;
	}
	return(0) ;	// not found
    }
}

PRIORITY getpriority(void)
{
    PRIORITY priority = 0 ;
    char string[16] ;		// 123\0

    printf("Priority> ") ;
    inputs(string) ;
    printf("\n") ;

    priority = atoi(string) ;

    return(priority) ;
}

#ifdef TIME_SLICE
TICKS getslice(void)
{
    TICKS tslice = 0 ;
    char string[16] ;		// 123\0

    printf("Time slice (in ticks)> ") ;
    inputs(string) ;
    printf("\n") ;

    tslice = atoi(string) ;

    return(tslice) ;
}
#endif

// extended mode debug functions start here
void taskmgr(void)
{
    char string[16] ;
    TASK task ;
    PRIORITY priority ;

    printf("\n") ;
    for( ; ; ) {
	printf("\n$RTXCbug> ") ;
	inputs(string);
	if (string[0] < ' ')
	    continue ;

	switch(string[0]) {
	case 'S' :              // suspend
	case 's' :
	    task = gettask() ;
	    if ( (task != SELFTASK) && (task != RTXCBUG) )
		KS_suspend(task) ;
	    break ;

	case 'R' :              // resume
	case 'r' :
	    task = gettask() ;
	    if (task != 0)
	       KS_resume(task) ;
	    break ;

	case 'T' :              // terminate
	case 't' :
	    task = gettask() ;
	    if (task != 0)
	       KS_terminate(task) ;
	    break ;

	case 'C' :              // change priority
	case 'c' :
	    task = gettask() ;
#ifdef DYNAMIC_TASKS
	    if ( (task != 0) && (task <= ntasks + dntasks) ) {
#else
	    if ( (task != 0) && (task <= ntasks) ) {
#endif
		priority = getpriority() ;
		KS_defpriority(task, priority) ;
	    }
	    break ;

	case 'E' :              // execute
	case 'e' :
	    task = gettask() ;
	    // note - no defense against executing CBUG, etc
	    if (task != SELFTASK)
		KS_execute(task) ;
	    break ;

	case 'B' :              // block
	case 'b' :
	    task = gettask() ;
	    if (task == -1) {	// special case - everybody blocked!
		cbugflag = 1 ;	// in case clock was running

		// block all except CBUG
#ifdef DYNAMIC_TASKS
		KS_block(1, ntasks + dntasks) ;
#else
		KS_block(1, ntasks) ;
#endif

		// unblock i/o drivers
		KS_unblock(CBUGIDRV, CBUGIDRV) ;
		KS_unblock(CBUGODRV, CBUGODRV) ;

		break ;
	    }
#ifdef DYNAMIC_TASKS
	    if ( (task != 0) && (task <= ntasks + dntasks) )
#else
	    if ( (task != 0) && (task <= ntasks) )
#endif
		KS_block(task, task) ;
	    break ;

	case 'U' :              // unblock
	case 'u' :
	    task = gettask() ;
	    if (task == -1) {	// special case - everybody unblocked
		// unblock all - ! CAUTION !
#ifdef DYNAMIC_TASKS
		KS_unblock(1, ntasks + dntasks) ;
#else
		KS_unblock(1, ntasks) ;
#endif
		cbugflag = 0 ;	// more caution
		break ;
	    }
#ifdef DYNAMIC_TASKS
	    if ( (task != 0) && (task <= ntasks + dntasks) )
#else
	    if ( (task != 0) && (task <= ntasks) )
#endif
		KS_unblock(task, task) ;
	    break ;

#ifdef TIME_SLICE
	case '/' :              // time slice (divide time)
	    task = gettask() ;
#ifdef DYNAMIC_TASKS
	    if ( (task != 0) && (task <= ntasks + dntasks) ) {
#else
	    if ( (task != 0) && (task <= ntasks) ) {
#endif
		KS_defslice(task, getslice()) ;
	    }
	    break ;
#endif

	case 'H' :              // Help
	case 'h' :
	    printf("\n") ;
	    printf("S - Suspend\n") ;
	    printf("R - Resume\n") ;
	    printf("T - Terminate\n") ;
	    printf("E - Execute\n") ;
	    printf("C - Change task priority\n") ;
	    printf("B - Block (-1=All)\n") ;
	    printf("U - Unblock (-1=ALL)\n") ;
#ifdef TIME_SLICE
	    printf("/ - Time slice\n") ;
#endif
	    printf("H - Help\n") ;
	    printf("X - Exit Task Manager Mode\n") ;
	    break;

	case 'X' :              // exit
	case 'x' :
	    printf("\nRe-entering RTXCbug mode\n") ;
	    return;

	default :
	    printf("\nBad command\n") ;
	    break;
      }
   }
}
#endif	// TASKDETAIL

// build input string the hard way - when gets() not supported
void inputs(char *p, short len)
{
    char *q, c, b ;
    int cnt, getflg ;

    getflg = YES ;
    q = p ;
    cnt = 1 ;
    while (getflg) {
	if (cnt >= len) {	// out of buffer
	    c = '\r' ;          // force
	    break ;
	}

	c = (char)(getchar()) ; 		// KS_dequeuew(CBUGIQ, &c) ;
	if (c != '\r') {
	    if (c != ESC) {
		if (c == BS) {
		    if (cnt > 1) {
			--cnt ;
			--q ;
		    }
		}

		// KS_enqueuew(CBUGOQ, &c) ;
	        putchar(c) ;

		if ( (c == BS) || (c == DEL) ) {
		    b = ' ' ;
		    putchar(b) ; // KS_enqueuew(CBUGOQ, &b) ;
		    putchar(c) ; // KS_enqueuew(CBUGOQ, &c) ;
		}
		if ( (c >= 32) && (c <= 126) ) { // printable
		    *q++ = c ;
		     cnt++ ;
		}
	    } else {
		*p = c ;
		cnt = 1 ;
		getflg = NO ;
	    }
	} else
	    getflg = NO ;
    }

    if (c == '\r') {
	putchar('\n'/*c*/) ; // KS_enqueuew(CBUGOQ, &c) ;
	*q = '\0' ;
    }
}

//
// general routine for forcing entry into RTXCbug
// callable from anywhere by anybody including user tasks
//
//
//void sgnlcbug(void)
//{
//    cbugflag = 1 ;	// set bug active flag - suppress clock service
//
//    KS_signal(CBUGSEMA) ;
//}

//
// show corresponding timeout value for objtype, objid and task match
//
void showtime(OBJTYPE objtype, TASK task, int objid)
{
    CLKBLK *next = clkqptr ;
    long time ; // note - use long not TICKS since used for time in ms

    while(next != NULLCLK) {	// walk timer list looking for matches
	if ( (next->objtype == objtype) &&
	     (next->objid == objid) &&
	     (next->task == task) ) {
	    time = next->remain - rtctick ;
//	    time *= clktick ;
	    time = (time * 1000)/CLKRATE ;

	    printf("%8ld ms", time) ;           // output time
	}
	next = next->flink ;
    }
}

//static char xname[NAMMAX+1] ;

char *xtaskname(TASK task)
{
#ifdef DYNAMIC_TASKS
    int i ;

    if (task <= ntasks)
        strcpy(xname, &taskkname[(int)(task)][0]) ;
        return(xname) ;
    else {
	i = task ;	// move to int type to avoid TASK typedef as char, etc.
	sprintf(xname, "TASK%d", i) ;
        return(xname) ;
    }
#else
    return((char *)&taskkname[(int)(task)][0]) ;
#endif
}

char *xsemaname(int s)
{
    return((char *)&semakname[s][0]) ;
}

#ifdef HAS_PARTITIONS
char *xpartname(MAP map)
{
#ifdef DYNAMIC_PARTS
    int i ;

    if (map <= nparts)
        return(&partkname[(int)(map)][0]) ;
    else {
	i = map ;	// move to int type to avoid MAP typedef as char, etc.
	sprintf(xname, "PART%d", i) ;
	return(xname) ;
   }
#else
    return((char *)&partkname[(int)(map)][0]) ;
#endif
}
#endif

#ifdef HAS_QUEUES
char *xqueuename(int q)
{
    return((char *)&queuekname[q][0]) ;
}
#endif

#ifdef HAS_RESOURCES
char *xresname(int r)
{
    return((char *)&reskname[r][0]) ;
}
#endif

#endif /* } CBUG */

// end of file - cbugutil.c

