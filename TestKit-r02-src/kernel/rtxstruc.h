/* rtxstruc.h - RTXC internal table data structures */

/*
 *   RTXC    Version 3.2
 *   Copyright (c) 1986-1997.
 *   Embedded System Products, Inc.
 *   ALL RIGHTS RESERVED
*/

#ifndef _RTXSTRUC_H
#define _RTXSTRUC_H

#include "rtxcopts.h"

//#define SWAP(A) ( ((A)<<8) | ((A)>>8) )

/* RTXC stack frame on interrupt */

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
typedef struct frame
{
    unsigned long userccr ;     // saved register
    void *pksnum ;              // r0: pointer to argument packet
    unsigned long r1 ;
    unsigned long r2 ;
    unsigned long r3 ;
    unsigned long r4 ;
    unsigned long r5 ;
    unsigned long r6 ;
    unsigned long r7 ;
    unsigned long r8 ;
    unsigned long r9 ;
    unsigned long r10 ;
    unsigned long r11 ;
    unsigned long r12 ;
    unsigned long r13 ;
    unsigned long r14 ;
    unsigned long userpc ;
} __attribute__ ((packed)) FRAME ;
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_LPC17XX) || defined(USE_LPC1788) || defined(USE_AT91SAM3S4)
typedef struct frame
{
    unsigned long r4 ;
    unsigned long r5 ;
    unsigned long r6 ;
    unsigned long r7 ;
    unsigned long r8 ;
    unsigned long r9 ;
    unsigned long r10 ;
    unsigned long r11 ;

    void *pksnum ;              // r0: pointer to argument packet
    unsigned long r1 ;
    unsigned long r2 ;
    unsigned long r3 ;
    unsigned long r12 ;
    unsigned long r14 ;         // r14 = lr
    unsigned long userpc ;      // r15 = pc
    unsigned long userccr ;     // saved register
    unsigned long optres ;      // optionally reserved for 8 byte alignment

} __attribute__ ((packed)) FRAME ;
#endif // defined(USE_LPC17XX) || defined(USE_LPC1788) || defined(USE_AT91SAM3S4)

/* RTXC task control block */
typedef struct tcb
{
   struct tcb *flink;	    // note, flink MUST be first element in tcb
   struct tcb *blink;
   struct clkblk *pclkblk;
   TASK task;
   PRIORITY priority;
   struct frame *sp;	    // current stack frame pointer
   volatile TSTATE status;
// #ifdef CBUG 	  // _FR_ 13/02/09
   unsigned long lastrunstack ;   // stack check on last run
// #endif // CBUG // _FR_ 13/02/09
//   void (*userpc_t0)(void) ;    // initial - entry point address
//   char *stackbase;
//   size_t stacksize;
#ifdef TIME_SLICE /* { */
   TICKS tslice;	    /* current time slice remaining */
   TICKS newslice;	    /* reset value for time slice */
#endif /* } TIME_SLICE */

#ifdef HAS_INQTASK_ARG /* { */
   void *arg;		    /* argument "passed" to (*pc_t0)() on execute */
#endif /* } HAS_INQTASK_ARG */

#ifdef FPU /* { */
   struct fpregs *fpregs;   /* optional fpu context */
   char fpumode;	    /* dynamic fpu requirement, 1=fpu in use */
#endif /* } FPU */
} TCB;

/* RTXC const task control block */
typedef struct ktcb
{
   void (*userpc_t0)(void) ;    // initial - entry point address
   char *stackbase;	   /* initial - stack base */
   size_t stacksize;	   /* initial - stack size */
   PRIORITY priority;	   /* initial - priority */
#ifdef FPU /* { */
   struct fpregs *fpregs;  /* optional fpu context */
#endif /* } FPU */
} KTCB;

#ifdef FPU /* { */
/* FPU (or any other extended) context	*/

typedef struct fpregs
{
  char status;	/* token place holder */
} FPREGS;

#endif /* } FPU */

/* RTXC queue header */
typedef struct qheader
{
   char *base;	       /* volatile version of qkheader.base    */
   size_t width;       /* volatile version of qkheader.width   */
   int depth;	       /* volatile version of qkheader.depth   */
   int putndx;	       /* internal use, initially = depth - 1  */
   int curndx;	       /* internal use, initially = 0	       */
#ifdef QUEUE_WAITERS /* { */
   TCB *waiters;       /* priority list of waiting tasks       */
   TCB *dummy;	       /* reserved - must follow *waiters      */
#endif /* } QUEUE_WAITERS */

#ifdef QUEUE_SEMAS /* { */
   SEMA fsema;	       /* queue full semaphore, initially PENDing      */
   SEMA nesema;        /* queue not empty semaphore, initially PENDing */
   SEMA esema;	       /* queue empty semaphore, initially DONE        */
   SEMA nfsema;        /* queue not full semaphore, initially DONE     */
#endif /* } QUEUE_SEMAS */

#ifdef CBUG /* { */
	       /* RTXCbug use */
   unsigned int count; /* statistics - count */
   int worst;	       /* statistics - worst case full level */
#endif /* } CBUG */
} QHEADER;

/* RTXC const queue header */
typedef struct qkheader
{
   char *base;	       /* &queue body */
   size_t width;       /* width in bytes */
   int depth;	       /* maximum depth in entries (not bytes) */
} QKHEADER;

/* RTXC memory block format */
struct xmap
{
   struct xmap *link;
};

/* RTXC partition headers */
typedef struct pheader
{
   struct xmap *next;  /* root of free pool list */
   size_t size;        /* no. of bytes in blocks  */
   int count;	       /* initial no. blocks in map */

#ifdef PARTITION_WAITERS /* { */
   TCB *waiters;       /* root of linked waiting tasks */
   TCB *dummy;	       /* reserved - must follow *waiters */
#endif /* } PARTITION_WAITERS */

#ifdef DYNAMIC_PARTS /* { */
   MAP map;	       /* dynamic partition number */
   char *addr;	       /* address of block of memory to use for partition */
#endif /* } DYNAMIC_PARTS */

#ifdef CBUG /* { */
	       /* RTXCbug use */
   int cur;	       /* statistics - current no. of used blocks */
   int worst;	       /* statistics - worst case low mark   */
   unsigned int usage; /* statistics - no. of KS_free() calls to date */
#endif /* } CBUG */
} PHEADER;

/* RTXC const partition headers */
typedef struct pkheader
{
   struct xmap *next; /* root of free pool list  */
   size_t size;       /* no. of bytes in blocks  */
   int count;	      /* initial no. blocks in map */
} PKHEADER;

/* RTXC message */
typedef struct rtxcmsg
{
   struct rtxcmsg *link;
   TASK task;	      /* sending task no. */
   PRIORITY priority; /* message priority */
   SEMA sema; /* response semaphore */
} RTXCMSG;

/* RTXC mailbox header */
typedef struct mheader
{
   RTXCMSG *link;
#ifdef MAILBOX_WAITERS /* { */
   TCB *waiters;     /* priority list of waiting tasks */
   TCB *dummy;	     /* reserved - must follow *waiters */
#endif /* } MAILBOX_WAITERS */

#ifdef MAILBOX_SEMAS /* { */
   SEMA nesema; /* mailbox not empty semaphore, initially undefined */
#endif /* } MAILBOX_SEMAS */

#ifdef CBUG /* { */
	  /* RTXCbug use */
   unsigned int count; /* statistics - no. of messages sent */
#endif /* } CBUG */
} MHEADER;

/* RTXC clock block */
typedef struct clkblk
{
   struct clkblk *flink;
   struct clkblk *blink;
   TICKS remain;       /* delta time before timer expiration */
   TICKS recycle;      /* cyclic amount if non-zero */
   TASK task;	       /* associated task no. */
   char state;	       /* TIMER_DONE or TIMER_ACTIVE */
   OBJTYPE objtype;    /* object type */
   char objid;	       /* object id */
} CLKBLK;

/* RTXC resource header */
typedef struct rheader
{
   TCB *owner;	      /* current task ownership, 0=no owner */
   int level;	      /* no. of nested locks */

#ifdef RESOURCE_WAITERS /* { */
   TCB *waiters;      /* priority list of waiters, 0=no waiters */
   TCB *dummy;	      /* reserved - must follow *waiters */
#ifdef PRIORITY_INVERSION /* { */
   PRIORITY priority; /* original owner's priority iff inversion */
   RESATTR resattr;   /* priority inversion option switch */
#endif /* } PRIORITY_INVERSION */
#endif /* } RESOURCE_WAITERS */
#ifdef CBUG /* { */
	  /* RTXCbug use */
   unsigned int count;	   /* statistics - no. of locks performed */
   unsigned int conflict;  /* statistics - no. resource conflicts */
#endif /* } CBUG */
} RHEADER;

#endif /* } _RTXSTRUC_H */

/* end of rtxstruc.h */

