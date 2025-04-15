/* rtxcapi.c - RTXC application program interface to kernel directives */

/*
 *   RTXC    Version 3.2
 *   Copyright (c) 1997-2005.
 *   T.E.S.T. srl
 *   Embedded System Products, Inc.
 *   ALL RIGHTS RESERVED
*/

#include "typedef.h"
#include "rtxstruc.h"
#include "rtxcarg.h"
#include "enable.h"

#include "rtxcapi.h"

#define NULL	    ((void *)0)
#define NULLCLK     ((CLKBLK *)0)
#define NULLTCB     ((TCB *)0)

#define SELFTASK    ((TASK)0)

extern SSTATE semat[];
extern TCB rtxtcb[];
extern TCB * hipritsk; /* highest priority task */

#define KS2(p) KS(p)

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
//#define KS(p) asm volatile("mov r0, %0 \n\t" "swi" : /* no outputs */ : "r" (&p))
#define KS(p) asm volatile("mov r0, %0 \n\t" "swi 0" : /* no outputs */ : "r" (&p))
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
#define KS(p) asm volatile("mov r0, %0 \n\t" "svc #0" : /* no outputs */ : "r" (&p))
#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

//__inline__ static void _ks(unsigned char * p) __attribute__ ( ( naked, always_inline ) ) ;
//__inline__ static void _ks(unsigned char * p) { __asm__ __volatile__("mov r0, %0 \n\t" "swi" : /* no outputs */ : "r" (&p)) ; }
//#define KS(p)  _ks((unsigned char *)(&p))

//#define KS(p) _ks((void *)&p);
//extern void /* @near */ _ks(void *p) ;

/*
 * NOTE: The next two FAST switches improve performance at the
 *	 expense of a considerable increase in the size of the
 *	 rtxcapi module considerably.
*/

/* user defined option for speed optimized KS_lockw()/KS_unlock() */
#define FASTLOCK

/* user defined option for speed optimized KS_alloc()/KS_allocw() */
#define FASTALLOC

#ifdef FPU /* { */
extern TCB * fputask; /* current owner of FPU */
#endif /* } FPU */

#ifdef HAS_RESOURCES /* { */
#ifdef FASTLOCK /* { */
extern RHEADER rheader[];
#endif /* } FASTLOCK */
#endif /* } HAS_RESOURCES */

#ifdef HAS_PARTITIONS /* { */
extern PHEADER pheader[];
#endif /* } HAS_PARTITIONS */

#ifdef HAS_QUEUES /* { */
extern QHEADER qheader[];
#endif /* } HAS_QUEUES */

#ifdef CBUG /* { */
extern void stkinit(short int *, size_t);
#endif /* } CBUG */

/*******************************************************/
#ifdef HAS_BLOCK /* { */
void KS_block(TASK starttask, TASK endtask)
{
   volatile struct blkarg p;

   p.ksnum = RTXC_BLOCK;
   p.starttask = starttask;
   p.endtask = endtask;
   KS(p);
}
#endif /* } HAS_BLOCK */

/*******************************************************/
const char * PROGMEM KS_version(void)
{
     return(PSTR("RTXC 3.2a ARM - T.E.S.T. srl")) ;
}

/*******************************************************/
#ifdef HAS_NOP /* { */
void KS_nop(void)
{
   volatile struct targ p;

   p.ksnum = RTXC_NOP;
   KS(p);
}
#endif /* } HAS_NOP */

/*******************************************************/
#ifdef HAS_ALLOC /* { */
void * KS_alloc(MAP map)
{
   volatile struct parg p;
#ifdef FASTALLOC /* { */
   PHEADER *pph;

   pph = &pheader[(int)map];
   DISABLE;

   if ( (p.val = (char *)pph->next) != NULL ) /* if any available */
   {
      pph->next = ((struct xmap *)p.val)->link; /* unlink 1st from list */
#ifdef CBUG /* { */
      if (++pph->cur > pph->worst) /* check for new worst case */
	 pph->worst = pph->cur; 	 /* useage level */
#endif /* } CBUG */
   }
   ENABLE;
#else /* } FASTALLOC { */
   p.ksnum = RTXC_ALLOC;
   p.map = map;
#ifdef PARTITION_WAITERS /* { */
   p.wait = 0;
#endif /* } PARTITION_WAITERS */
   KS(p);
#endif /* } FASTALLOC */
   return(p.val);
}
#endif /* } HAS_ALLOC */

/*******************************************************/
#ifdef HAS_ALLOC_TIMER /* { */
CLKBLK * KS_alloc_timer(void)
{
   volatile struct clkarg p;

   p.ksnum = RTXC_ALLOC_TIMER;
   KS(p);
   return(p.pclkblk);
}
#endif /* } HAS_ALLOC_TIMER */

/*******************************************************/
#ifdef HAS_DEFPRIORITY /* { */
void KS_defpriority(TASK task, PRIORITY priority)
{
   volatile struct targ p;

   p.ksnum = RTXC_DEFPRIORITY;
   p.task = task;
   p.priority = priority;
   KS(p);
}
#endif /* } HAS_DEFPRIORITY */

/*******************************************************/
#ifdef TIME_SLICE /* { */
void KS_defslice(TASK task, TICKS ticks)
{
   TCB *ptcb;

   if (task == SELFTASK)
      ptcb = hipritsk;
   else
      ptcb = &rtxtcb[(int)task];

   /* if slicing is just becoming active */
   if (ptcb->newslice == 0)
   {
      ptcb->tslice = ticks;
   }
   ptcb->newslice = ticks;
}
#endif /* } TIME_SLICE */

/*******************************************************/
#ifdef HAS_DELAY /* { */
void KS_delay(TASK task, TICKS ticks)
{
   volatile struct delayarg p;
   CLKBLK clkblk;

   p.ksnum = RTXC_DELAY;
   p.task = task;
   p.ticks = ticks;
   p.pclkblk = (CLKBLK *)&clkblk;

   KS2(p) ;
}
#endif /* } HAS_DELAY */

/*******************************************************/
#ifdef HAS_DEQUEUE /* { */
KSRC KS_dequeue(QUEUE queue, void *data)
{
   volatile struct qarg p;

   p.ksnum = RTXC_DEQUEUE;
   p.queue = queue;
   p.data = data;
#ifdef QUEUE_WAITERS /* { */
   p.wait = 0;
#endif /* } QUEUE_WAITERS */
   p.ksrc = RC_QUEUE_EMPTY;
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_DEQUEUE */

/*******************************************************/
#ifdef HAS_ENQUEUE /* { */
KSRC KS_enqueue(QUEUE queue, void *data)
{
   volatile struct qarg p;

   p.ksnum = RTXC_ENQUEUE;
   p.queue = queue;
   p.data = data;
#ifdef QUEUE_WAITERS /* { */
   p.wait = 0;
#endif /* } QUEUE_WAITERS */
   p.ksrc = RC_QUEUE_FULL;
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_ENQUEUE */

/*******************************************************/
void KS_execute(TASK task)
{
   volatile struct targ p;

   p.ksnum = RTXC_EXECUTE;
   p.task = task;
   KS(p);
}

/*******************************************************/
#ifdef HAS_DEFTASK /* { */
KSRC KS_deftask(TASK task, PRIORITY priority,
			char *stackbase, size_t stacksize,
			void (*entry)(void))
{
   volatile struct deftaskarg p;

#ifdef CBUG /* { */
    stkinit((short int *)stackbase, stacksize);
#endif /* } CBUG */

   p.ksnum = RTXC_DEFTASK;
   p.task = task;
   p.priority = priority;
   p.stackbase = stackbase;
   p.stacksize = stacksize;
   p.entry = entry;
   p.ksrc = RC_GOOD;
   KS(p);

   return(p.ksrc);
}
#endif /* } HAS_DEFTASK */

/*******************************************************/
#ifdef HAS_ALLOC_TASK /* { */
TASK KS_alloc_task(void)
{
   volatile struct targ p;

   p.ksnum = RTXC_ALLOC_TASK;
   KS(p);

   /* return task # (0 = no tasks available) */
   return(p.task);
}
#endif /* } HAS_ALLOC_TASK */

/*******************************************************/
#ifdef HAS_FREE /* { */
void KS_free(MAP map, void *val)
{
   volatile struct parg p;

   p.ksnum = RTXC_FREE;
   p.map = map;
   p.val = val;
   KS(p);
}
#endif /* } HAS_FREE */

/*******************************************************/
#ifdef HAS_ALLOC_PART /* { */
MAP KS_alloc_part(void)
{
   volatile struct parg p;

   p.ksnum = RTXC_ALLOC_PART;
   KS(p);

   /* return map # (0 = no dynamic partition headers available) */
   return(p.map);
}
#endif /* } HAS_ALLOC_PART */

/*******************************************************/
#ifdef HAS_DEFPART /* { */
void KS_defpart(MAP map, void *addr, size_t blksize, size_t nblocks)
{
   volatile struct parg p;

   p.ksnum = RTXC_DEFPART;
   p.map = map;
   p.addr = addr;
   p.size = blksize;
   p.nblocks = nblocks;
   KS(p);
}
#endif /* } HAS_DEFPART */

/*******************************************************/
#ifdef HAS_FREE_PART /* { */
void * KS_free_part(MAP map)
{
   volatile struct parg p;

   p.ksnum = RTXC_FREE_PART;
   p.map = map;
   KS(p);
   return(p.addr);
}
#endif /* } HAS_FREE_PART */

/*******************************************************/
#ifdef HAS_FREE_TIMER /* { */
void KS_free_timer(CLKBLK *pclkblk)
{
   volatile struct clkarg p;

   p.ksnum = RTXC_FREE_TIMER;
   if ( (p.pclkblk = pclkblk) != NULLCLK)
   {
      KS(p);
   }
}
#endif /* } HAS_FREE_TIMER */

/*******************************************************/
#ifdef HAS_LOCK /* { */
KSRC KS_lock(RESOURCE resource)
{
   volatile struct larg p;
#ifdef FASTLOCK /* { */
   RHEADER *prh;

   prh = &rheader[(int)resource];
   DISABLE;

   if (prh->owner == NULLTCB)  /* if resource not currently owned */
   {
#ifdef CBUG /* { */
      prh->count++;	      /* update statistics */
#endif /* } CBUG */
      prh->owner = hipritsk; /* lock resource with task # */
      prh->level = 1;	      /* set nesting level = 1 */
      ENABLE;		      /* fast return */
      return(RC_GOOD);	      /* mark successful */
   }

   if (prh->owner == hipritsk) /* if already owned by caller */
   {
#ifdef CBUG /* { */
      prh->count++;	      /* update statistics */
#endif /* } CBUG */
      prh->level++;	      /* bump nesting level */
      ENABLE;		      /* fast return */
      return(RC_NESTED);      /* mark successful */
   }
   ENABLE;
#endif /* } FASTLOCK */

   p.ksnum = RTXC_LOCK;
   p.resource = resource;
#ifdef RESOURCE_WAITERS /* { */
   p.wait = 0;
#endif /* } RESOURCE_WAITERS */
   p.ksrc = RC_GOOD;
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_LOCK */

/*******************************************************/
#ifdef HAS_PEND /* { */
void KS_pend(SEMA sema)
{
   volatile struct sarg p;

   p.ksnum = RTXC_PEND;
   p.sema = sema;
   KS(p);
}
#endif /* } HAS_PEND */

/*******************************************************/
#ifdef HAS_RECEIVE /* { */
RTXCMSG * KS_receive(MBOX mbox, TASK task)
{
   volatile struct msgarg p;

   p.ksnum = RTXC_RECEIVE;
   p.mbox = mbox;
   p.task = task;
#ifdef MAILBOX_WAITERS /* { */
   p.wait = 0;
#endif /* } MAILBOX_WAITERS */
   KS(p);
   return(p.prtxcmsg);
}
#endif /* } HAS_RECEIVE */

/*******************************************************/
#ifdef HAS_RESUME /* { */
void KS_resume(TASK task)
{
   volatile struct targ p;

   p.ksnum = RTXC_RESUME;
   p.task = task;
   KS(p);
}
#endif /* } HAS_RESUME */

/*******************************************************/
#ifdef HAS_SEND /* { */
void KS_send(MBOX mbox, RTXCMSG *prtxcmsg, PRIORITY priority, SEMA sema)
{
   volatile struct msgarg p;

   p.ksnum = RTXC_SEND;
   p.mbox = mbox;
   p.prtxcmsg = prtxcmsg;
   p.priority = priority;
   p.sema = sema;
   p.wait = 0;
   KS(p);
}
#endif /* } HAS_SEND */

/*******************************************************/
KSRC KS_signal(SEMA sema)
{
   volatile struct sarg p;

   p.ksnum = RTXC_SIGNAL;
   p.sema = sema;
   p.ksrc = RC_GOOD;
   KS(p);
   return(p.ksrc);
}

/*******************************************************/
#ifdef HAS_ACK /* { */
void KS_ack(RTXCMSG *prtxcmsg)
{

   KS_signal(prtxcmsg->sema);

}
#endif /* } HAS_ACK */

/*******************************************************/
#ifdef HAS_STOP_TIMER /* { */
KSRC KS_stop_timer(CLKBLK *pclkblk)
{
   volatile struct clkarg p;

   p.ksnum = RTXC_STOP_TIMER;
   p.pclkblk = pclkblk;
   p.ksrc = RC_GOOD;
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_STOP_TIMER */

/*******************************************************/
#ifdef HAS_START_TIMER /* { */
CLKBLK * KS_start_timer(CLKBLK *pclkblk, TICKS ticks,
				    TICKS period, SEMA sema)
{
   volatile struct clkarg p;

   p.ksnum = RTXC_START_TIMER;
   p.pclkblk = pclkblk;
   p.ticks = ticks;
   p.period = period;
   p.sema = sema;
   KS(p);
   return(p.pclkblk);
}
#endif /* } HAS_START_TIMER */

/*******************************************************/
#ifdef HAS_SUSPEND /* { */
void KS_suspend(TASK task)
{
   volatile struct targ p;

   p.ksnum = RTXC_SUSPEND;
   p.task = task;
   KS(p);
}
#endif /* } HAS_SUSPEND */

/*******************************************************/
#ifdef HAS_TERMINATE /* { */
void KS_terminate(TASK task)
{
   volatile struct targ p;

   p.ksnum = RTXC_TERMINATE;
   p.task = task;
   KS(p);
}
#endif /* } HAS_TERMINATE */

/*******************************************************/
#ifdef HAS_UNBLOCK /* { */
void KS_unblock(TASK starttask, TASK endtask)
{
   volatile struct blkarg p;

   p.ksnum = RTXC_UNBLOCK;
   p.starttask = starttask;
   p.endtask = endtask;
   KS(p);
}
#endif /* } HAS_UNBLOCK */

/*******************************************************/
#ifdef HAS_UNLOCK /* { */
KSRC KS_unlock(RESOURCE resource)
{
   volatile struct larg p;
#ifdef FASTLOCK /* { */
   RHEADER *prh;

   prh = &rheader[(int)resource];
   DISABLE;

   if (prh->owner != hipritsk) /* if hipritsk not owner, then nop */
   {
      ENABLE; /* fast return */
      return(RC_BUSY);
   }

   if (prh->level > 1) /* if nesting level > 1 */
   {
      prh->level--;   /* decrement nesting level */
      ENABLE;		   /* fast return */
      return(RC_NESTED);
   }

#ifdef RESOURCE_WAITERS /* { */
   if (prh->waiters == NULLTCB) /* if no tasks are waiting */
   {
      prh->level = 0;  /* zero nesting level */
      prh->owner = NULLTCB; /* mark resource not owned */
      ENABLE;		   /* fast return */
      return(RC_GOOD);
   }
#endif /* } RESOURCE_WAITERS */

   ENABLE;		/* fast return */
#endif /* } FASTLOCK */

   p.ksnum = RTXC_UNLOCK;
   p.ksrc = RC_GOOD;
   p.resource = resource;
#ifdef FASTLOCK /* { */
   KS2(p);
#else
   KS(p);
#endif /* } FASTLOCK */
   return(p.ksrc);
}
#endif /* } HAS_UNLOCK */

/*******************************************************/
#ifdef HAS_USER /* { */
int KS_user(int (*fun)(void *), void *arg)
{
   volatile struct userarg p;

   p.ksnum = RTXC_USER;
   p.fun = fun;
   p.arg = arg;
   KS(p);
   return(p.val);
}
#endif /* } HAS_USER */

/*******************************************************/
KSRC KS_wait(SEMA sema)
{
   volatile struct sarg p;

   p.ksnum = RTXC_WAIT;
   p.sema = sema;
   p.ticks = (TICKS)0;
   p.ksrc = RC_GOOD;
   KS(p);
   return(p.ksrc);
}

/*******************************************************/
#ifdef HAS_YIELD /* { */
KSRC KS_yield(void)
{
   volatile struct targ p;

   p.ksnum = RTXC_YIELD;
   p.ksrc = RC_NO_YIELD;
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_YIELD */

#ifdef RTXC_AL /* { */
/*******************************************************/
/*		ADVANCED RTXC LIBRARY		       */
/*******************************************************/
#ifdef HAS_ALLOCW /* { */
void * KS_allocw(MAP map)
{
   volatile struct parg p;
#ifdef FASTALLOC /* { */
   PHEADER *pph;

   pph = &pheader[(int)map];
   DISABLE;

   if ( (p.val = (char *)pph->next) != NULL ) /* if any available */
   {
      pph->next = ((struct xmap *)p.val)->link; /* unlink 1st from list */
#ifdef CBUG /* { */
      if (++pph->cur > pph->worst) /* check for new worst case */
	 pph->worst = pph->cur; 	 /* useage level */
#endif /* } CBUG */
      ENABLE;
      return(p.val);
   }
   ENABLE;
#endif /* } FASTALLOC */

   p.ksnum = RTXC_ALLOC;
   p.map = map;
   p.wait = 1;
#ifdef PARTITION_TIMEOUTS /* { */
   p.ticks = (TICKS)0; /* indicate NO timeout */
#endif /* } PARTITION_TIMEOUTS */
   KS(p);
   return(p.val);
}
#endif /* } HAS_ALLOCW */

/*******************************************************/
#ifdef HAS_CREATE_PART /* { */
MAP KS_create_part(void *addr, size_t blksize, size_t nblocks)
{
   volatile struct parg p;

   p.ksnum = RTXC_CREATE_PART;
   p.addr = addr;
   p.size = blksize;
   p.nblocks = nblocks;
   KS(p);

   /* return map # (0 = no dynamic partition headers available) */
   return(p.map);
}
#endif /* } HAS_CREATE_PART */

/*******************************************************/
#ifdef HAS_INQRES /* { */
TASK KS_inqres(RESOURCE resource)
{
   volatile struct larg p;

   p.ksnum = RTXC_INQRES;
   p.resource = resource;
   p.task = (TASK)0; /* initialize in case no owner exists */
   KS(p);
   return(p.task);
}
#endif /* } HAS_INQRES */

/*******************************************************/
#ifdef PRIORITY_INVERSION /* { */
#ifdef HAS_DEFRES /* { */
KSRC KS_defres(RESOURCE resource, RESATTR resattr)
{
   volatile struct larg p;

   p.ksnum = RTXC_DEFRES;
   p.resource = resource;
   p.resattr = resattr;
   p.ksrc = RC_GOOD;
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_DEFRES */
#endif /* } PRIORITY_INVERSION */

/*******************************************************/
#ifdef HAS_INQTASK_ARG /* { */
void * KS_inqtask_arg(TASK task)
{
   volatile struct deftaskarg p;

   p.ksnum = RTXC_INQTASK_ARG;
   p.task = task;
   KS(p);
   return(p.arg);
}
#endif /* } HAS_INQTASK_ARG */

/*******************************************************/
#ifdef HAS_DEFTASK_ARG /* { */
void KS_deftask_arg(TASK task, void *arg)
{
   volatile struct deftaskarg p;

   p.ksnum = RTXC_DEFTASK_ARG;
   p.task = task;
   p.arg = arg;
   KS(p);
}
#endif /* } HAS_DEFTASK_ARG */

/*******************************************************/
#ifdef HAS_DEFMBOXSEMA /* { */
/* mailbox event management */
void KS_defmboxsema(MBOX mbox, SEMA sema)
{
   volatile struct msgarg p;

   p.ksnum = RTXC_DEFMBOXSEMA;
   p.mbox = mbox;
   p.sema = sema;
   KS(p);
}
#endif /* } HAS_DEFMBOXSEMA */

/*******************************************************/
#ifdef HAS_DEFQSEMA /* { */
/* queue event management */
void KS_defqsema(QUEUE queue, SEMA sema, QCOND qcond)
{
   volatile struct qdefarg p;

   p.ksnum = RTXC_DEFQSEMA;
   p.queue = queue;
   p.sema = sema;
   p.qcond = qcond;
   KS(p);
}
#endif /* } HAS_DEFQSEMA */

/*******************************************************/
#ifdef HAS_DEQUEUEW /* { */
void KS_dequeuew(QUEUE queue, void *data)
{
   volatile struct qarg p;

   p.ksnum = RTXC_DEQUEUE;
   p.queue = queue;
   p.data = data;
#ifdef QUEUE_TIMEOUTS /* { */
   p.ticks = (TICKS)0;
#endif /* } QUEUE_TIMEOUTS */
   p.wait = 1;
   KS(p);
}
#endif /* } HAS_DEQUEUEW */

/*******************************************************/
#ifdef HAS_ELAPSE /* { */
TICKS KS_elapse(TICKS *stamp)
{
   volatile struct etarg p;

   p.ksnum = RTXC_ELAPSE;
   p.stamp = stamp;
   KS(p);
   return(p.val);
}
#endif /* } HAS_ELAPSE */

/*******************************************************/
#ifdef HAS_ENQUEUEW /* { */
void KS_enqueuew(QUEUE queue, void *data)
{
   volatile struct qarg p;

   p.ksnum = RTXC_ENQUEUE;
   p.queue = queue;
   p.data = data;
#ifdef QUEUE_TIMEOUTS /* { */
   p.ticks = (TICKS)0;
#endif /* } QUEUE_TIMEOUTS */
   p.wait = 1;
   KS(p);
}
#endif /* } HAS_ENQUEUEW */

/*******************************************************/
#ifdef HAS_INQMAP /* { */
size_t KS_inqmap(MAP map)
{
   return(pheader[(int)map].size);
}
#endif /* } HAS_INQMAP */

/*******************************************************/
#ifdef HAS_LOCKW /* { */
KSRC KS_lockw(RESOURCE resource)
{
   volatile struct larg p;
#ifdef FASTLOCK /* { */
   RHEADER *prh;

   prh = &rheader[(int)resource];
   DISABLE;

   if (prh->owner == NULLTCB)  /* if resource not currently owned */
   {
#ifdef CBUG /* { */
      prh->count++;	      /* update statistics */
#endif /* } CBUG */
      prh->owner = hipritsk; /* lock resource with task # */
      prh->level = 1;	      /* set nesting level = 1 */
      ENABLE;		      /* fast return */
      return(RC_GOOD);
   }

   if (prh->owner == hipritsk) /* if already owned by caller */
   {
#ifdef CBUG /* { */
      prh->count++;	      /* update statistics */
#endif /* } CBUG */
      prh->level++;	      /* bump nesting level */
      ENABLE;		      /* fast return */
      return(RC_NESTED);
   }
   ENABLE;
#endif /* } FASTLOCK */

   p.ksnum = RTXC_LOCK;
   p.resource = resource;
   p.wait = 1;
   p.ksrc = RC_GOOD;
#ifdef RESOURCE_TIMEOUTS /* { */
   p.ticks = (TICKS)0;
#endif /* } RESOURCE_TIMEOUTS */
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_LOCKW */

/*******************************************************/
#ifdef HAS_PURGEQUEUE /* { */
void KS_purgequeue(QUEUE queue)
{
   volatile struct qarg p;

   p.ksnum = RTXC_PURGEQUEUE;
   p.queue = queue;
   KS(p);
}
#endif /* } HAS_PURGEQUEUE */

/*******************************************************/
#ifdef HAS_RECEIVEW /* { */
RTXCMSG * KS_receivew(MBOX mbox, TASK task)
{
   volatile struct msgarg p;

   p.ksnum = RTXC_RECEIVE;
   p.mbox = mbox;
   p.task = task;
   p.wait = 1;
#ifdef MAILBOX_TIMEOUTS /* { */
   p.ticks = (TICKS)0;
#endif /* } MAILBOX_TIMEOUTS */
   KS(p);
   return(p.prtxcmsg);
}
#endif /* } HAS_RECEIVEW */

/*******************************************************/
#ifdef HAS_SENDW /* { */
void KS_sendw(MBOX mbox, RTXCMSG *prtxcmsg, PRIORITY priority, SEMA sema)
{
   volatile struct msgarg p;

   p.ksnum = RTXC_SEND;
   p.mbox = mbox;
   p.prtxcmsg = prtxcmsg;
   p.priority = priority;
   p.sema = sema;
   p.wait = 1;
#ifdef MAILBOX_TIMEOUTS /* { */
   p.ticks = (TICKS)0; /* indicate NO timeout */
#endif /* } MAILBOX_TIMEOUTS */
   KS(p);
}
#endif /* } HAS_SENDW */

/*******************************************************/
#ifdef HAS_WAITM /* { */
SEMA KS_waitm(const SEMA *semalist)
{
   volatile struct sargm p;

   p.ksnum = RTXC_WAITM;
   p.list = (SEMA *) semalist;
   KS(p);
   return(p.sema);
}
#endif /* } HAS_WAITM */

/*******************************************************/
#ifdef HAS_INQTIME /* { */
time_t KS_inqtime(void)
{
   volatile struct timearg p;

   p.ksnum = RTXC_INQTIME;
   KS(p);
   return(p.time);
}
#endif /* } HAS_INQTIME */

/*******************************************************/
#ifdef HAS_DEFTIME /* { */
void KS_deftime(time_t time)
{
   volatile struct timearg p;

   p.ksnum = RTXC_DEFTIME;
   p.time = time;
   KS(p);
}
#endif /* } HAS_DEFTIME */

#endif /* } RTXC_AL */

#ifdef RTXC_EL /* { */
/*******************************************************/
/* EXTENDED RTXC LIBRARY */
/*******************************************************/
#ifdef HAS_INQTIMER /* { */
TICKS KS_inqtimer(CLKBLK *pclkblk)
{
   volatile struct clkarg p;

   p.ksnum = RTXC_INQTIMER;
   p.pclkblk = pclkblk;
   KS(p);
   return(p.ticks);
}
#endif /* } HAS_INQTIMER */

/*******************************************************/
#ifdef HAS_ALLOCT /* { */
void * KS_alloct(MAP map, TICKS ticks, KSRC *pksrc)
{
   volatile struct parg p;
   CLKBLK clkblk;
#ifdef FASTALLOC /* { */
   PHEADER *pph;

   pph = &pheader[(int)map];
   DISABLE;

   if ( (p.val = (char *)pph->next) != NULL ) /* if any available */
   {
      pph->next = ((struct xmap *)p.val)->link; /* unlink 1st from list */
#ifdef CBUG /* { */
      if (++pph->cur > pph->worst) /* check for new worst case */
	 pph->worst = pph->cur; 	 /* useage level */
#endif /* } CBUG */
      ENABLE;
      *pksrc = RC_GOOD;
      return(p.val);
   }
   ENABLE;
#endif /* } FASTALLOC */

   p.ksnum = RTXC_ALLOC;
   p.map = map;
   p.wait = 1;
   p.ticks = ticks;
   p.ksrc = RC_GOOD;
   p.pclkblk = (CLKBLK *)&clkblk;
   KS2(p);
   *pksrc = p.ksrc;
   return(p.val);
}
#endif /* } HAS_ALLOCT */

/*******************************************************/
#ifdef HAS_DEFQUEUE /* { */
KSRC KS_defqueue(QUEUE queue, size_t width, int depth,
		      char *base, int current_size)
{
   volatile struct qdefarg p;

   p.ksnum = RTXC_DEFQUEUE;
   p.queue = queue;
   p.width = width;
   p.depth = depth;
   p.base = base;
   p.current_size = current_size;
   p.ksrc = RC_GOOD;
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_DEFQUEUE */

/*******************************************************/
#ifdef HAS_DEQUEUET /* { */
KSRC KS_dequeuet(QUEUE queue, void *data, TICKS ticks)
{
   volatile struct qarg p;
   CLKBLK clkblk;

   p.ksnum = RTXC_DEQUEUE;
   p.queue = queue;
   p.data = data;
   p.ticks = ticks;
   p.wait = 1;
   p.ksrc = RC_TIMEOUT;
   p.pclkblk = (CLKBLK *)&clkblk;
   KS2(p);
   return(p.ksrc);
}
#endif /* } HAS_DEQUEUET */

/*******************************************************/
#ifdef HAS_ENQUEUET /* { */
KSRC KS_enqueuet(QUEUE queue, void *data, TICKS ticks)
{
   volatile struct qarg p;
   CLKBLK clkblk;

   p.ksnum = RTXC_ENQUEUE;
   p.queue = queue;
   p.data = data;
   p.ticks = ticks;
   p.wait = 1;
   p.ksrc = RC_TIMEOUT;
   p.pclkblk = (CLKBLK *)&clkblk;
   KS2(p);
   return(p.ksrc);
}
#endif /* } HAS_ENQUEUET */

/*******************************************************/
#ifdef HAS_INQPRIORITY /* { */
PRIORITY KS_inqpriority(TASK task)
{
   if (task == SELFTASK)
      return(hipritsk->priority);
   else
      return(rtxtcb[(int)task].priority);
}
#endif /* } HAS_INQPRIORITY */

/*******************************************************/
#ifdef HAS_INQQUEUE /* { */
int KS_inqqueue(QUEUE queue)
{
   return(qheader[(int)queue].curndx);
}
#endif /* } HAS_INQQUEUE */

/*******************************************************/
#ifdef HAS_INQSEMA /* { */
SSTATE KS_inqsema(SEMA sema)
{
   return(semat[(int)sema]);
}
#endif /* } HAS_INQSEMA */

/*******************************************************/
#ifdef HAS_INQTASK /* { */
TASK KS_inqtask(void)
{
   return(hipritsk->task);
}
#endif /* } HAS_INQTASK */

/*******************************************************/
#ifdef HAS_LOCKT /* { */
KSRC KS_lockt(RESOURCE resource, TICKS ticks)
{
   volatile struct larg p;
   CLKBLK clkblk;
#ifdef FASTLOCK /* { */
   RHEADER *prh;

   prh = &rheader[(int)resource];
   DISABLE;

   if (prh->owner == NULLTCB)  /* if resource not currently owned */
   {
#ifdef CBUG /* { */
      prh->count++;	      /* update statistics */
#endif /* } CBUG */
      prh->owner = hipritsk; /* lock resource with task # */
      prh->level = 1;	      /* set nesting level = 1 */
      ENABLE;		      /* fast return */
      return(RC_GOOD);	      /* mark successful */
   }

   if (prh->owner == hipritsk) /* if already owned by caller */
   {
#ifdef CBUG /* { */
      prh->count++;	      /* update statistics */
#endif /* } CBUG */
      prh->level++;	      /* bump nesting level */
      ENABLE;		      /* fast return */
      return(RC_NESTED);      /* mark successful */
   }
   ENABLE;
#endif /* } FASTLOCK */

   p.ksnum = RTXC_LOCK;
   p.resource = resource;
   p.wait = 1;
   p.ticks = ticks;
   p.ksrc = RC_GOOD;
   p.pclkblk = (CLKBLK *)&clkblk;
   KS2(p);
   return(p.ksrc);
}
#endif /* } HAS_LOCKT */

/*******************************************************/
#ifdef HAS_PENDM /* { */
void KS_pendm(SEMA *semalist)
{
   volatile struct sargm p;

   p.ksnum = RTXC_PENDM;
   p.list = semalist;
   KS(p);
}
#endif /* } HAS_PENDM */

/*******************************************************/
#ifdef HAS_RECEIVET /* { */
RTXCMSG * KS_receivet(MBOX mbox, TASK task, TICKS ticks, KSRC *pksrc)
{
   volatile struct msgarg p;
   CLKBLK clkblk;

   p.ksnum = RTXC_RECEIVE;
   p.mbox = mbox;
   p.task = task;
   p.prtxcmsg = (RTXCMSG *)0; /* preset in case timeout */
   p.wait = 1;
   p.ticks = ticks;
   p.ksrc = RC_GOOD;
   p.pclkblk = (CLKBLK *)&clkblk;
   KS2(p);
   *pksrc = p.ksrc;
   return(p.prtxcmsg);
}
#endif /* } HAS_RECEIVET */

/*******************************************************/
#ifdef HAS_RESTART_TIMER /* { */
KSRC KS_restart_timer(CLKBLK *pclkblk, TICKS ticks, TICKS period)
{
   volatile struct clkarg p;

   p.ksnum = RTXC_RESTART_TIMER;
   p.pclkblk = pclkblk;
   p.ticks = ticks;
   p.period = period;
   p.ksrc = RC_GOOD;
   KS(p);
   return(p.ksrc);
}
#endif /* } HAS_RESTART_TIMER */

/*******************************************************/
#ifdef HAS_SENDT /* { */
KSRC KS_sendt(MBOX mbox, RTXCMSG *prtxcmsg, PRIORITY priority, SEMA sema,
		   TICKS ticks)
{
   volatile struct msgarg p;
   CLKBLK clkblk;

   p.ksnum = RTXC_SEND;
   p.mbox = mbox;
   p.prtxcmsg = prtxcmsg;
   p.priority = priority;
   p.sema = sema;
   p.wait = 1;
   p.ticks = ticks;
   p.ksrc = RC_GOOD;
   p.pclkblk = (CLKBLK *)&clkblk;
   KS2(p);
   return(p.ksrc);
}
#endif /* } HAS_SENDT */

/*******************************************************/
#ifdef HAS_SIGNALM /* { */
void KS_signalm(SEMA *semalist)
{
   volatile struct sargm p;

   p.ksnum = RTXC_SIGNALM;
   p.list = semalist;
   KS(p);
}
#endif /* } HAS_SIGNALM */

/*******************************************************/
#ifdef HAS_WAITT /* { */
KSRC KS_waitt(SEMA sema, TICKS ticks)
{
   volatile struct sarg p;
   CLKBLK clkblk;

   p.ksnum = RTXC_WAIT;
   p.sema = sema;
   p.ticks = ticks;
   p.ksrc = RC_GOOD;
   p.pclkblk = (CLKBLK *)&clkblk;
   KS2(p);
   return(p.ksrc);
}
#endif /* } HAS_WAITT */

/*******************************************************/
#ifdef TIME_SLICE /* { */
#ifdef HAS_INQSLICE /* { */
TICKS KS_inqslice(TASK task)
{
   if (task == SELFTASK)
      task = hipritsk->task;

   return(rtxtcb[(int)task].newslice);
}
#endif /* } HAS_INQSLICE */
#endif /* } TIME_SLICE */

#endif /* } RTXC_EL */

/* end of file - rtxcapi.c */

