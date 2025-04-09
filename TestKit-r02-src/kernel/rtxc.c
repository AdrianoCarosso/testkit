// rtxc.c - RTXC
//
//   RTXC    Version 3.2
//   Copyright (c) 1986-1997.
//   Embedded System Products, Inc.
//   ALL RIGHTS RESERVED
//
//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//

#include <string.h>

#include "typedef.h"
#include "rtxstruc.h"
#include "rtxcarg.h"
#include "tstate.h"
#include "enable.h"
#include "rtxcapi.h"

#include "cclock.h"

#undef USE_REGISTERDEBUG
#undef USE_LEDDEBUG

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef USE_LEDDEBUG
#include <stdio_console.h>
#include "AT91SAM7A3.h"
#define LED1            0x1000000
#define LED2            0x2000000
#define LED3            0x0100000
#define LED4            0x0200000
#define LED_MASK        (LED1 | LED2 | LED3 | LED4)
#define PUTOUT(A) { while (!(AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_TXRDY)) ; AT91C_BASE_DBGU->DBGU_THR = (A) & 0x1ff ; }
volatile FRAME * xxx ;
#endif // USE_LEDDEBUG

#ifdef USE_REGISTERDEBUG
#include <stdio_console.h>
void DumpReg(char *header, FRAME *f)
{
    extern volatile TICKS rtctick ;
//    extern char cbugflag ;

    if (rtctick < 140) return ;
//    if (f->r11 != 6) return ;
//    cbugflag = 1 ;
//    for( ; ; ) ;
//    return ;
    
    DISABLE ;
    printf("%s %d\n", header, rtctick) ;
    printf("userccr=%08lx\n", f->userccr) ;
    printf("pksnum= %08lx\n", (unsigned long)(f->pksnum)) ;
    printf("user pc=%08lx\n", f->userpc) ;
    printf("user r1=%08lx\n", f->r1) ;
    printf("user r2=%08lx\n", f->r2) ;
    printf("user r3=%08lx\n", f->r3) ;
    printf("user r4=%08lx\n", f->r4) ;
    printf("user r5=%08lx\n", f->r5) ;
    printf("user r6=%08lx\n", f->r6) ;
    printf("user r7=%08lx\n", f->r7) ;
    printf("user r8=%08lx\n", f->r8) ;
    printf("user r9=%08lx\n", f->r9) ;
    printf("userr10=%08lx\n", f->r10) ;
    printf("userr11=%08lx\n", f->r11) ;
    printf("userr12=%08lx\n", f->r12) ;
    printf("user sp=%08lx\n", f->r13) ;
    printf("user lr=%08lx\n", f->r14) ;
//    for( ; ; ) ; // halt
}
#endif // USE_REGISTERDEBUG
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/* state of CLKBLKs */
#define TIMER_ACTIVE	1 /* timer counting down */
#define TIMER_DONE	0 /* timer already expired */

//#define NULL	 ((void *)0)
#define NULLTCB  ((TCB *)0)
#define NULLMAP  ((PHEADER *)0)
#define NULLCLK  ((CLKBLK *)0)
#define NULLSEMA ((SEMA)0)
#define NULLFUNC ((void (*)(void))0)
#ifdef FPU /* { */
#define NULLFPREGS ((FPREGS *)0)
#endif /* } FPU */

#define SELFTASK ((TASK)0)

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#define CCR_T0 0x3f             // CCR at start of task IRQ+FIQ enabled, Thumb, SYS
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
#define CCR_T0 0x01000000       // CCR at start of task: Thumb
#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)

/* stash sema in siglist */
#define SIGNAL(sema) DISABLE; *semaput++ = sema; scheduler = 0; ENABLE

extern SSTATE semat[];
extern SEMA siglist[];
extern const SEMA nsemas;
#ifdef BSS_NOT_ZERO /* { */
extern const int siglistsize;
#endif /* } BSS_NOT_ZERO */

#ifdef HAS_ALLOC_TIMER /* { */
extern CLKBLK clkq[];
extern const int ntmrs;
#endif /* } HAS_ALLOC_TIMER */

#ifdef HAS_MAILBOXES /* { */
extern MHEADER mheader[];
extern const MBOX nmboxes;
#endif /* } HAS_MAILBOXES */

#ifdef HAS_PARTITIONS /* { */
extern PHEADER pheader[];
extern const PKHEADER pkkheader[] PROGMEM ;
extern const MAP nparts;
#ifdef DYNAMIC_PARTS /* { */
extern const MAP dnparts;
#endif /* } DYNAMIC_PARTS */
#endif /* } HAS_PARTITIONS */

#ifdef HAS_QUEUES /* { */
extern QHEADER qheader[];
extern const QKHEADER qkkheader[] PROGMEM ;
extern const QUEUE nqueues;
#endif /* } HAS_QUEUES */

#ifdef HAS_RESOURCES /* { */
extern RHEADER rheader[];
extern const RESOURCE nres;
#endif /* } HAS_RESOURCES */

char scheduler;

TCB * hipritsk; /* highest priority task */
TCB * nsrttcb;	/* list of tcbs pending scheduling */

SEMA * semaput;
SEMA * semaget;

#ifdef DYNAMIC_TASKS /* { */
TCB * dtcbfl; /* head of tcb free list,initially &rtxtcb[NTASKS+1] */
#endif /* } DYNAMIC_TASKS */

#ifdef HAS_PARTITIONS /* { */
#ifdef DYNAMIC_PARTS /* { */
PHEADER * dphfl; /* head of partition header free list, initially
				  &pheader[NPARTS+1] */
static void dn_defpart(PHEADER *pph, KSNUM *p2);
#endif /* } DYNAMIC_PARTS */
#endif /* } HAS_PARTITIONS */

volatile TICKS rtctick; /* no. of ticks since hw reset, initially 0 */

#ifdef HAS_INQTIME /* { */
volatile time_t rtctime ;	 // running seconds maintained by clock driver
int ratecnt ;		         // clkrate counter (0 -> clkrate-1)
#endif /* } HAS_INQTIME */

#ifdef HAS_ALLOC_TIMER /* { */
CLKBLK * clkqfl;  /* ptr to timer free list, initially &clkq[0] */
#endif /* } HAS_ALLOC_TIMER */

CLKBLK * clkqptr; /* ptr to first active timer, initially NULLCLK */
#ifdef TIME_SLICE /* { */
TCB * sliceup;
#endif /* } TIME_SLICE */
extern const int clkrate;

#ifdef FPU /* { */
TCB * fputask; /* current owner of FPU */
extern void fpuswap(FPREGS *oldregs, FPREGS *newregs);
#endif /* } FPU */

unsigned long kernellastrunstack ;

#ifdef CBUG /* { */
//extern char cbugflag; /* RTXCbug active flag */
char isrmax;  /* worst case interrupt depth, initially = 0 */
#endif /* } CBUG */

static void stkinit(int tsk) ; // _FR_ 13/02/09

char isrcnt; /* count of "levels" of interrupts, initially = 0 */

//extern char rtxctos[];

char t_expired;

FRAME * API_PAGING postem(void);

#ifdef HAS_MAILBOXES /* { */
static void insert_message(MHEADER *pmh, RTXCMSG *prtxcmsg);
static RTXCMSG *remove_message(MHEADER *pmh, TASK task);
#endif /* } HAS_MAILBOXES */

#ifdef HAS_ALLOC_TIMER /* { */
static CLKBLK *get_clkblk(void);
#endif /* } HAS_ALLOC_TIMER */

static void insert_timer(CLKBLK *pclkblk);
static void unlink_timer(CLKBLK *pclkblk);

#if defined(MAILBOX_WAITERS)   || \
    defined(PARTITION_WAITERS) || \
    defined(QUEUE_WAITERS)     || \
    defined(RESOURCE_WAITERS) /* { */
#define COMBO_WAIT (MSG_WAIT | PARTITION_WAIT | QUEUE_WAIT | RESOURCE_WAIT)
static void porder(TCB *ptcb);
static void reorder_waiters(TCB *ptcb);
static void fwd_insert(TCB *ptcb);
static void bwd_insert(TCB *ptcb);
#endif /* } - MAILBOX_ || PARTITION_ || QUEUE_ || RESOURCE_WAITERS */

static void chgpriority(TCB *ptcb, PRIORITY priority);

#ifdef PRIORITY_INVERSION /* { */
/*
 *
 * the following variable defines the initial conditions of all
 * PRIORITY_INVERSION resource attributes. Setting to _OFF is
 * default. Use KS_defres() at runtime to set individual resources to _ON.
 *
*/
#define PRIORITY_INVERSION_T0 PRIORITY_INVERSION_OFF
#endif /* } PRIORITY_INVERSION */

/* choice of function (smaller) code or inline (faster) */
#ifndef INLINE_UH_MACRO_EXPANSION /* { */	 // BUGVER _BM_ 20/10/1998
#define UNLINK_HIPRITSK() hipritsk = hipritsk->flink; \
			  hipritsk->blink = (TCB *)&hipritsk;

#else /* } INLINE_UH_MACRO_EXPANSION { */
#define UNLINK_HIPRITSK() unlink_hipritsk()
static void unlink_hipritsk(void)
{
   hipritsk = hipritsk->flink;
   hipritsk->blink = (TCB *)&hipritsk;
}
#endif /* } INLINE_UH_MACRO_EXPANSION */

static void update_sema(SEMA sema)
{
   if (semat[(int)sema] == SEMA_PENDING)
       semat[(int)sema] = SEMA_DONE;
   else if (semat[(int)sema] != SEMA_DONE)
   {
      SIGNAL(sema);
   }
}

/* entry point for all kernel directives */
FRAME *rtxc(FRAME *p)
{
   KSNUM *p2 = p->pksnum ;
   FRAME *frame;
   TCB *ptcb;
   TASK task;
   CLKBLK *pclkblk;
   TICKS tcnt; /* local copy of rtctick */
   SEMA sema;
   SEMA *semalist;
   SSTATE *sema_ptr;
   KSNUM *p2a;

#ifdef HAS_BLOCK /* { */
   TASK endtask;
#endif /* } HAS_BLOCK */

#ifdef HAS_MAILBOXES /* { */
   MHEADER *pmh;
   RTXCMSG *prtxcmsg;
#endif /* } HAS_MAILBOXES */

#ifdef HAS_PARTITIONS /* { */
   PHEADER *pph;
   struct xmap *q;
#endif /* } HAS_PARTITIONS */

#ifdef HAS_QUEUES /* { */
   QHEADER *pqh;
   int qindex, depth;
   size_t width;
#ifdef HAS_DEFQUEUE /* { */
   int cursz;
#endif /* } HAS_DEFQUEUE */
#endif /* } HAS_QUEUES */

#ifdef HAS_RESOURCES /* { */
   RHEADER *prh;
#endif /* } HAS_RESOURCES */

#ifdef FPU /* { */
   FPREGS *fpregs;
#endif /* } FPU */

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef USE_LEDDEBUG
{ xxx = (FRAME *)p ;
  for( ; ; ) ;
}
// configure the PIO Lines corresponding to LED1 to LED4 to be outputs
// no need to set these pins to be driven by the PIO because it is GPIO pins only.
AT91C_BASE_PIOA->PIO_PER = LED_MASK ; // Set in PIO mode
AT91C_BASE_PIOA->PIO_OER = LED_MASK ; // Configure in Output

// set at 1 (turn led off)
AT91C_BASE_PIOA->PIO_SODR = LED_MASK ;

// set at 0 (turn led on)
AT91C_BASE_PIOA->PIO_CODR = LED3 ;
//printf("%d-",*p2);
//PUTOUT('.'+*p2)
#endif // USE_LEDDEBUG
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   hipritsk->sp = (FRAME *)p; /* always save &stk frame */

   switch(*p2)
   {
/************************/
      case RTXC_SIGNAL:
/************************/
	 sema = ((struct sarg *)p2)->sema;

	 if (semat[(int)sema] == SEMA_DONE)
	    ((struct sarg *)p2)->ksrc = RC_MISSED_EVENT;
	 else
	 {
	    SIGNAL(sema);
	 }
	 break;

#ifdef HAS_SIGNALM /* { */
/************************/
      case RTXC_SIGNALM:
/************************/
	 for (semalist = ((struct sargm *)p2)->list;
	     *semalist != NULLSEMA; semalist++)
	 {
	    if (semat[(int)(*semalist)] != SEMA_DONE)
	    {
	       SIGNAL(*semalist);
	    }
	 }
	 break;
#endif /* } HAS_SIGNALM */

/************************/
      case RTXC_WAIT:
/************************/
	 sema = ((struct sarg *)p2)->sema;

	 if (semat[(int)sema] == SEMA_PENDING)
	 {
	    ptcb = hipritsk; /* save for later */

#ifdef SEMAPHORE_TIMEOUTS /* { */
	    if ( ((struct sarg *)p2)->ticks) /* if timeout active */
	    {
	       pclkblk = ptcb->pclkblk = ((struct sarg *)p2)->pclkblk;
	       pclkblk->remain = ((struct sarg *)p2)->ticks;
	       pclkblk->recycle = (TICKS)0; /* non-cyclic */

	       /* mark object type */
	       pclkblk->objtype = SEMAPHORE_OBJ;
	       pclkblk->objid = sema;
	       pclkblk->task = ptcb->task;

	       /* insert timer */
	       insert_timer(pclkblk);
	       ((struct sarg *)p2)->ksrc = RC_TIMEOUT;
	    }
#endif /* } SEMAPHORE_TIMEOUTS */

	    /* put sema into wait state */
	    semat[(int)sema] = (SSTATE)ptcb->task;

	    ptcb->status = SEMAPHORE_WAIT;

	    UNLINK_HIPRITSK(); /* unlink first tcb */
	    break;
	 }

	 if (semat[(int)sema] == SEMA_DONE) /* if sema already done */
	 {
	    semat[(int)sema] = SEMA_PENDING;  /* set sema pending */
	 }
	 else	  /* else sema was already in a wait state */
	 {
	    /* if here, application task design flaw */
	       /* since no corresponding semaphore */
	    ((struct sarg *)p2)->ksrc = RC_WAIT_CONFLICT;
	 }
	 break;

#ifdef HAS_WAITM /* { */
/************************/
      case RTXC_WAITM:
/************************/
	 semalist = ((struct sargm *)p2)->list;
	 sema_ptr = NULL ;     // keep compiler quiet _BM_
	 while (*semalist != NULLSEMA) /* while not at end of waitm list */
	 {
	    sema_ptr = &semat[(int)(*semalist)]; /* get ptr to sema */

	    if (*sema_ptr == SEMA_PENDING)
	    {
	       *sema_ptr = (SSTATE)hipritsk->task; /* set sema waiting */
	       semalist++;			    /* bump list ptr */
	    }
	    else /* found sema in WAIT or DONE state */
	       break;
	 }

	 if (*semalist == NULLSEMA) /* all semas now in WAIT state */
	 {
	    hipritsk->status = SEMAPHORE_WAIT; /* mark status SEMAPHORE_WAIT */

	    UNLINK_HIPRITSK(); /* unlink first tcb */
	    break;	      /* force task switch */
	 }

	 if (*sema_ptr == SEMA_DONE) /* found a sema in list in DONE state */
	 {
	    /* return sema to user */
	    ((struct sargm *)p2)->sema = *semalist;

	       /* set all semas in list to PEND state */
	    for ( ; ((struct sargm *)p2)->list <= semalist;
		    ((struct sargm *)p2)->list++)
	       semat[(int)(*(((struct sargm *)p2)->list))] = SEMA_PENDING;
	 }
	 else /* semaphore was found already in a WAIT state */
	 {
	       /* if arrive here, application task design flaw */
		 /* since no corresponding semaphore waiting */
	    hipritsk->status = SEMAPHORE_WAIT;

	    UNLINK_HIPRITSK(); /* unlink first tcb */
	 }
	 break;
#endif /* } HAS_WAITM */

#ifdef HAS_PEND /* { */
/************************/
      case RTXC_PEND:
/************************/
	 sema = ((struct sarg *)p2)->sema;

	 if (semat[(int)sema] == SEMA_DONE)
	    semat[(int)sema] = SEMA_PENDING;
	 break;
#endif /* } HAS_PEND */

#ifdef HAS_PENDM /* { */
/************************/
      case RTXC_PENDM:
/************************/
	 for (semalist = ((struct sargm *)p2)->list;
	     *semalist != NULLSEMA; semalist++)
	    if (semat[(int)(*semalist)] == SEMA_DONE)
	       semat[(int)(*semalist)] = SEMA_PENDING;
	 break;
#endif /* } HAS_PENDM */

#ifdef HAS_SUSPEND /* { */
/************************/
      case RTXC_SUSPEND:
/************************/
	 if ( (task = ((struct targ *)p2)->task) == SELFTASK)
	    ptcb = hipritsk;
	 else
	 {
	    ptcb = &rtxtcb[(int)task];
	    if (ptcb->status != READY)
	    {
	       ptcb->status |= SUSPFLG; /* mark suspended */
	       break;
	    }
	 }
	 ptcb->status |= SUSPFLG; /* mark suspended */

	 ptcb->flink->blink = ptcb->blink; /* general task unlink */
	 ptcb->blink->flink = ptcb->flink;

	 break;
#endif /* } HAS_SUSPEND */

#ifdef HAS_RESUME /* { */
/************************/
      case RTXC_RESUME:
/************************/
	 ptcb = &rtxtcb[(int)(((struct targ *)p2)->task)];

	 if (ptcb->status != READY)
	 {
	       /* clear suspended and test for ready now */
	     if ( (ptcb->status &= ~SUSPFLG) == READY)
	     {
		 ptcb->flink = nsrttcb;
		 nsrttcb = ptcb;
		 scheduler = 0;
	     }
	 }
	 break;
#endif /* } HAS_RESUME */

#ifdef HAS_PARTITIONS /* { */
#ifdef HAS_ALLOC /* { */
/************************/
      case RTXC_ALLOC:
/************************/
	 pph = &pheader[(int)(((struct parg *)p2)->map)];

	 /* Get 1st free block. If none available, thread waiter */
	 if ( (q = (struct xmap *)KS_ISRalloc(((struct parg *)p2)->map))
				       == NULL)
	 {
#ifdef PARTITION_WAITERS /* { */
	    if ( ((struct parg *)p2)->wait)
	    {
	       ptcb = hipritsk; /* save hipritsk for later use */

	       ptcb->status = PARTITION_WAIT;

	       porder((TCB *)&pph->waiters);

#ifdef PARTITION_TIMEOUTS /* { */
	       if ( ((struct parg *)p2)->ticks)
	       {
		  ptcb->pclkblk = ((struct parg *)p2)->pclkblk;
		  pclkblk = ptcb->pclkblk;
		  pclkblk->remain = ((struct parg *)p2)->ticks;
		  pclkblk->recycle = (TICKS)0;

		  /* mark object type */
		  pclkblk->objtype = PARTITION_OBJ;
		  pclkblk->objid = ((struct parg *)p2)->map;
		  pclkblk->task = ptcb->task;

		  /* insert timer */
		  insert_timer(pclkblk);

		  ((struct parg *)p2)->ksrc = RC_TIMEOUT;
	       }
#endif /* } PARTITION_TIMEOUTS */
	    }
#endif /* } PARTITION_WAITERS */
	 }
	 ((struct parg *)p2)->val = (char *)q;
	 break;
#endif /* } HAS_ALLOC */

#ifdef HAS_FREE /* { */
/************************/
      case RTXC_FREE:
/************************/
	 q = (struct xmap *)((struct parg *)p2)->val;

	 pph = &pheader[(int)(((struct parg *)p2)->map)];

#ifdef CBUG /* { */
	 pph->usage++; /* increment no. frees */
#endif /* } CBUG */

#ifdef PARTITION_WAITERS /* { */
	 if ( (ptcb = pph->waiters) != NULLTCB) /* if any waiters */
	 {
	    /* pass ptr directly from freeing task to allocating task */
	    p2a = ((FRAME *)(ptcb->sp))->pksnum;
	    ((struct parg *)p2a)->val = (char *)q;

	    /* remove first waiter from list */
	    if ( (pph->waiters = ptcb->flink) != NULLTCB)
	       ptcb->flink->blink = (TCB *)&pph->waiters;

	    /* "resume" first waiter */
	    if ( (ptcb->status &= ~PARTITION_WAIT) == READY)
	    {
	       /* insert waiter into READY list */
	       ptcb->flink = nsrttcb;
	       nsrttcb = ptcb;
	       scheduler = 0;
	    }

#ifdef PARTITION_TIMEOUTS /* { */
	    /* cleanup any pending timeout */
	    if ( (pclkblk = ptcb->pclkblk) != NULLCLK)
	    {
	       /* mark waiter GOOD */
	       ((struct parg *)p2a)->ksrc = RC_GOOD;

	       unlink_timer(pclkblk);
	       ptcb->pclkblk = NULLCLK;
	    }
#endif /* } PARTITION_TIMEOUTS */
	 }
	 else
#endif /* } PARTITION_WAITERS */
	 {
	    /* put block back on free list */
	    DISABLE;
	    q->link = pph->next;
	    pph->next = q;
	    ENABLE;

#ifdef CBUG /* { */
	    pph->cur--;   /* track current no. used */
#endif /* } CBUG */
	 }
	 break;
#endif /* } HAS_FREE */

#ifdef HAS_CREATE_PART /* { */
/************************/
      case RTXC_CREATE_PART:
/************************/
	 if ( (pph = dphfl) != NULLMAP)
	 {
	    /* allocate pheader by removing 1st from free list */
	    dphfl = (PHEADER *)pph->next;

	    /* put map number in arg packet */
	    ((struct parg *)p2)->map = pph->map;

	    /* use header to define partition */
	    dn_defpart(pph,p2);
	 }
	 else  /* pheader not available, return 0 */
	    ((struct parg *)p2)->map = (MAP)0;

	 break;
#endif /* } HAS_CREATE_PART */

#ifdef HAS_DEFPART /* { */
/************************/
      case RTXC_DEFPART:
/************************/
	 pph = &pheader[((struct parg *)p2)->map];
	 dn_defpart(pph,p2);  /* use header to define partition */

	 break;
#endif /* } HAS_DEFPART */

#ifdef HAS_ALLOC_PART /* { */
/************************/
      case RTXC_ALLOC_PART:
/************************/
	 if ( (pph = dphfl) != NULLMAP)
	 {
	    /* allocate pheader by removing 1st from free list */
	    dphfl = (PHEADER *)pph->next;

	    /* put map number in arg packet */
	    ((struct parg *)p2)->map = pph->map;
	 }
	 else /* pheader not available, return 0 */
	    ((struct parg *)p2)->map = (MAP)0;

	 break;
#endif /* } HAS_ALLOC_PART */

#ifdef HAS_FREE_PART /* { */
/************************/
      case RTXC_FREE_PART:
/************************/
	 pph = &pheader[((struct parg *)p2)->map];

	 /* reset dimension and stats */
	 pph->size = 0;
	 pph->count = 0;
#ifdef PARTITION_WAITERS /* { */
	 pph->waiters = NULLTCB;
	 pph->dummy = NULLTCB;
#endif /* } PARTITION_WAITERS */
#ifdef CBUG /* { */
	 pph->cur = 0;
	 pph->worst = 0;
	 pph->usage = 0;
#endif /* } CBUG */

	 /* re-insert pheader into pheader free list for dynamic partitions */
	 pph->next = (struct xmap *)dphfl;
	 dphfl = pph;

	 ((struct parg *)p2)->addr = pph->addr;

	 break;
#endif /* } HAS_FREE_PART */

#endif /* } HAS_PARTITIONS */

#ifdef HAS_RESOURCES /* { */
#ifdef HAS_LOCK /* { */
/************************/
      case RTXC_LOCK:
/************************/
	 prh = &rheader[(int)(((struct larg *)p2)->resource)];

#ifdef CBUG /* { */
	 prh->count++; /* update statistics */
#endif /* } CBUG */

	 if (prh->owner == NULLTCB)  /* if resource not currently owned */
	 {
	    prh->owner = hipritsk; /* lock resource with task */
	    prh->level = 1;	    /* set nesting level = 1 */
	    ((struct larg *)p2)->ksrc = RC_GOOD;
	    break;
	 }

	 if (prh->owner == hipritsk) /* if nested lock */
	 {
	    prh->level++; /* bump nesting level only */
		   /* indicate NESTED lock */
	    ((struct larg *)p2)->ksrc = RC_NESTED;
	    break;
	 }

#ifdef CBUG /* { */
	 prh->conflict++; /* count resource bottlenecks */
#endif /* } CBUG */

#ifdef RESOURCE_WAITERS /* { */
	 /* if wait on resource not available */
	 if ( ((struct larg *)p2)->wait)
	 {
#ifdef PRIORITY_INVERSION /* { */
	    if (prh->resattr == PRIORITY_INVERSION_ON)
	    {
	       /* if owner is lower priority than requestor */
	       if ( (ptcb = prh->owner)->priority > hipritsk->priority)
	       {
		  /* priority inversion is necessary */

		  /* save owner's priority only for first conflict */
		  if (prh->priority == (PRIORITY)0)
		     prh->priority = ptcb->priority;

		  /* but always elevate owner's priority to hipri->priority */
		  ptcb->priority = hipritsk->priority;

		  /* change priority - whatever it takes */
		  if (ptcb->status == READY)
		  {
		     /* unlink owner task from RUN list */
		     ptcb->flink->blink = ptcb->blink;
		     ptcb->blink->flink = ptcb->flink;

		     /* re-insert task */
		     ptcb->flink = nsrttcb;
		     nsrttcb = ptcb;
		     scheduler = 0;
		  }
		  else
		  {
		     /* if task being changed is in a WAITER list */
		     if (ptcb->status & COMBO_WAIT)
		     {
			/*
			 * need to re-order within partition, queue, resource,
			 * or mailbox header
			*/
			reorder_waiters(ptcb);
		     }
		     else
		     {
			/* task is suspended, terminated or blocked */
			/* priority change above is all that is needed */
		     }
		  }
	       }
	    }
#endif /* } PRIORITY_INVERSION */

	    ptcb = hipritsk; /* save hipritsk for later use */

	    ptcb->status = RESOURCE_WAIT; /* mark task waiting */

	    /* thread waiting task in priority order */
	    porder((TCB *)&prh->waiters);

#ifdef RESOURCE_TIMEOUTS /* { */
	    if ( ((struct larg *)p2)->ticks) /* if timeout active */
	    {
	       ptcb->pclkblk = ((struct larg *)p2)->pclkblk;
	       pclkblk = ptcb->pclkblk;
	       pclkblk->remain = ((struct larg *)p2)->ticks;
	       pclkblk->recycle = (TICKS)0;

	       /* mark object type */
	       pclkblk->objtype = RESOURCE_OBJ;
	       pclkblk->objid = ((struct larg *)p2)->resource;
	       pclkblk->task = ptcb->task;

	       /* insert timer */
	       insert_timer(pclkblk);

	       ((struct larg *)p2)->ksrc = RC_TIMEOUT;
	    }
#endif /* } RESOURCE_TIMEOUTS */
	 }
	 else
#endif /* } RESOURCE_WAITERS */
	 {
		   /* indicate lock failure */
	    ((struct larg *)p2)->ksrc = RC_BUSY;
	 }
	 break;
#endif /* } HAS_LOCK */

#ifdef HAS_UNLOCK /* { */
/************************/
      case RTXC_UNLOCK:
/************************/
	 prh = &rheader[(int)(((struct larg *)p2)->resource)];

	 if (prh->owner == NULLTCB) /* if no owner, then NOP */
	 {
	    ((struct larg *)p2)->ksrc = RC_GOOD;
	    break;
	 }

	 if (prh->owner != hipritsk) /* if hipritsk not owner, then NOP */
	 {
	    ((struct larg *)p2)->ksrc = RC_BUSY;
	    break;
	 }

	 if (--prh->level) /* if more nesting remaining, then NOP */
	 {
	    ((struct larg *)p2)->ksrc = RC_NESTED;
	    break;
	 }

#ifdef RESOURCE_WAITERS /* { */
#ifdef PRIORITY_INVERSION /* { */
	 if (prh->resattr == PRIORITY_INVERSION_ON)
	 {
	    if (prh->priority != (PRIORITY)0)
	    {
	       /* return owner task to pre-inversion priority
		*
		* note - owner task may have manually changed its priority
		*	 unbeknownst to kernel
	       */

	       /* iff relative priorities are changed then re-prioritize */
	       if ( (hipritsk->priority = prh->priority) >
		    hipritsk->flink->priority)
	       {
		  ptcb = hipritsk; /* save for later */

		  /* remove self from READY list (ALWAYS first in list) */
		  UNLINK_HIPRITSK();

		  /* re-insert task */
		  ptcb->flink = nsrttcb;
		  nsrttcb = ptcb;
		  scheduler = 0;
	       }

	       prh->priority = 0; /* reset priority inversion history flag */
	    }
	 }
#endif /* } PRIORITY_INVERSION */

	 /* if any task is waiting for the resource */
	 if ( (ptcb = prh->waiters) != NULLTCB)
	 {
	    /* remove first waiter from list */
	    if ( (prh->waiters = ptcb->flink) != NULLTCB)
	       ptcb->flink->blink = (TCB *)&prh->waiters;

		 /* "resume" first waiter */
	    if ( (ptcb->status &= ~RESOURCE_WAIT) == READY)
	    {
	       /* insert waiter into READY list */
	       ptcb->flink = nsrttcb;
	       nsrttcb = ptcb;
	       scheduler = 0;
	    }

	    prh->owner = ptcb; /* mark resource owned */
	    prh->level = 1;

#ifdef RESOURCE_TIMEOUTS /* { */
	    /* cleanup any pending timeout */
	    if ( (pclkblk = ptcb->pclkblk) != NULLCLK)
	    {
	       p2a = ((FRAME *)(ptcb->sp))->pksnum;
	       ((struct larg *)p2a)->ksrc = RC_GOOD;

	       unlink_timer(pclkblk);
	       ptcb->pclkblk = NULLCLK;
	    }
#endif /* } RESOURCE_TIMEOUTS */
	 }
	 else
#endif /* } RESOURCE_WAITERS */
	 {
	    prh->owner = NULLTCB; /* mark resource not owned */
	    ((struct larg *)p2)->ksrc = RC_GOOD;
	 }
	 break;
#endif /* } HAS_UNLOCK */

#ifdef HAS_INQRES /* { */
/************************/
      case RTXC_INQRES:
/************************/
	 prh = &rheader[(int)(((struct larg *)p2)->resource)];

	 /* return owner task or 0 if none (0 set in API since faster) */
	 if (prh->owner != NULLTCB)
	    ((struct larg *)p2)->task = prh->owner->task;
	 break;
#endif /* } HAS_INQRES */

#ifdef PRIORITY_INVERSION /* { */
#ifdef HAS_DEFRES /* { */
/************************/
      case RTXC_DEFRES:
/************************/
	 prh = &rheader[(int)(((struct larg *)p2)->resource)];

	 /* note: ksrc initialized to RC_GOOD in API */

	 if (prh->owner == NULLTCB)
	    prh->resattr = ((struct larg *)p2)->resattr;
	 else
	    ((struct larg *)p2)->ksrc = RC_BUSY;
	 break;
#endif /* } HAS_DEFRES */
#endif /* } PRIORITY_INVERSION */

#endif /* } HAS_RESOURCES */

#ifdef HAS_BLOCK /* { */
/************************/
      case RTXC_BLOCK:
/************************/
	 /* handle block world (all tasks) */
	 if ( (endtask = ((struct blkarg *)p2)->endtask) == SELFTASK)
	    endtask = hipritsk->task; /* -1 could block null task 0 */

	 if ( (task = ((struct blkarg *)p2)->starttask) == SELFTASK)
	    task = hipritsk->task; /* +1 could block past last task */

	 for (ptcb = &rtxtcb[(int)task]; task <= endtask; task++, ptcb++)
	 {
	    if (hipritsk->task == task) /* never block self */
	       continue;

	    if (ptcb->status == READY)
	    {
	       ptcb->flink->blink = ptcb->blink; /* general unlink */
	       ptcb->blink->flink = ptcb->flink;
	    }

	    ptcb->status |= BLOCK_WAIT;
	 }
	 break;
#endif /* } HAS_BLOCK */

#ifdef HAS_UNBLOCK /* { */
/************************/
      case RTXC_UNBLOCK:
/************************/
	 if ( (endtask = ((struct blkarg *)p2)->endtask) == SELFTASK)
	    endtask = hipritsk->task; /* -1 could unblock null task */

	 if ( (task = ((struct blkarg *)p2)->starttask) == SELFTASK)
	    task = hipritsk->task; /* +1 could unblock past last task */

	 for (ptcb = &rtxtcb[(int)task]; task <= endtask; task++, ptcb++)
	 {
	    if (ptcb->status == READY)
	       continue;

	    if ( (ptcb->status &= ~BLOCK_WAIT) == READY)
	    {
	       ptcb->flink = nsrttcb;
	       nsrttcb = ptcb;
	       scheduler = 0;
	    }
	 }
	 break;
#endif /* } HAS_UNBLOCK */

#ifdef HAS_ALLOC_TIMER /* { */
/************************/
      case RTXC_ALLOC_TIMER:
/************************/
	 ((struct clkarg *)p2)->pclkblk = get_clkblk();
	 break;
#endif /* } HAS_ALLOC_TIMER */

#ifdef HAS_FREE_TIMER /* { */
/************************/
      case RTXC_FREE_TIMER:
/************************/
	 pclkblk = ((struct clkarg *)p2)->pclkblk;

	 if (pclkblk->state == TIMER_ACTIVE)
	 {
	    unlink_timer(pclkblk);
	 }

	 pclkblk->flink = clkqfl; /* insert block at front of free list */
	 clkqfl = pclkblk;
	 break;
#endif /* } HAS_FREE_TIMER */

#ifdef HAS_STOP_TIMER /* { */
/************************/
      case RTXC_STOP_TIMER:
/************************/
	 /* fall through to common code */
#endif /* } HAS_STOP_TIMER */
#ifdef HAS_RESTART_TIMER /* { */
/************************/
      case RTXC_RESTART_TIMER:
/************************/
	 /* fall through to common code */
#endif /* } HAS_RESTART_TIMER */
#ifdef HAS_START_TIMER /* { */
/************************/
      case RTXC_START_TIMER:
/************************/

	 if ( (pclkblk = ((struct clkarg *)p2)->pclkblk) == NULLCLK)
	 {
#ifdef HAS_ALLOC_TIMER /* { */
	    if (*p2 != RTXC_START_TIMER)
	    {
#ifdef HAS_RESTART_TIMER /* { */
	       /* no clkblk sent for RTXC_STOP_TIMER or RTXC_RESTART_TIMER */
#else /* } HAS_RESTART_TIMER { */
	       /* no clkblk sent for RTXC_STOP_TIMER */
#endif /* } HAS_RESTART_TIMER */
	       ((struct clkarg *)p2)->ksrc = RC_TIMER_ILLEGAL;
	       break;
	    }
	    else /* is RTXC_START_TIMER */
	    {
	       if ( (pclkblk = get_clkblk()) == NULLCLK)
		  break; /* no timer available, NULLCLK returned */
	       else
		  ((struct clkarg *)p2)->pclkblk = pclkblk;
	    }
#else /* } HAS_ALLOC_TIMER */
           // modified by _BM_ 30/8/2006
           ((struct clkarg *)p2)->ksrc = RC_TIMER_ILLEGAL;
	   break;
#endif /* } HAS_ALLOC_TIMER */
	 }
	 else /* clkblk was sent */
	 {
	    if (*p2 != RTXC_START_TIMER)
	    {
	       if (pclkblk->task != hipritsk->task)
	       {
#ifdef HAS_RESTART_TIMER /* { */
		  /* attempt to STOP or RESTART someone else's timer */
#else /* } HAS_RESTART_TIMER { */
		  /* attempt to STOP someone else's timer */
#endif /* } HAS_RESTART_TIMER */
		  ((struct clkarg *)p2)->ksrc = RC_TIMER_ILLEGAL;
		  break;
	       }
	    }
	 }

	 if (pclkblk->state == TIMER_DONE)
	 {
	    if (*p2 == RTXC_STOP_TIMER)
	    {
	       ((struct clkarg *)p2)->ksrc = RC_TIMER_INACTIVE;
	       break;
	    }
	 }
	 else	/* stop active timer */
	 {
	    unlink_timer(pclkblk);

	    if (*p2 == RTXC_STOP_TIMER)
	       break;
	 }

#ifdef HAS_RESTART_TIMER /* { */
	 /* at this point, the function is START_TIMER or RESTART_TIMER */
#else /* } HAS_RESTART_TIMER { */
	 /* at this point, the function should be START_TIMER */
#endif /* } HAS_RESTART_TIMER */

	 if (*p2 == RTXC_START_TIMER)
	 {
	    /* if special case of zero initial and recycle times ... */
	    /* signal semaphore and leave */
	    if ( (((struct clkarg *)p2)->ticks == (TICKS)0) &&
		 (((struct clkarg *)p2)->period == (TICKS)0) )
	    {
	       pclkblk->task = hipritsk->task;
	       sema = ((struct clkarg *)p2)->sema;
	       SIGNAL(sema);
	       break;
	    }

	    /* if zero initial time and non-zero recycle time ... */
	    /* start timer with recycle time & signal semaphore */
	    if ( (((struct clkarg *)p2)->ticks == (TICKS)0) &&
		 (((struct clkarg *)p2)->period != (TICKS)0) )
	    {
	       ((struct clkarg *)p2)->ticks =
			   ((struct clkarg *)p2)->period;
	       sema = ((struct clkarg *)p2)->sema;
	       SIGNAL(sema);
	    }
	 }
#ifdef HAS_RESTART_TIMER /* { */
	 else  /* is RTXC_RESTART_TIMER */
	 {
	    /* get sema from clkblk & put in arg struct */
	    ((struct clkarg *)p2)->sema = pclkblk->objid;
	 }
#endif /* } HAS_RESTART_TIMER */

	 pclkblk->remain = ((struct clkarg *)p2)->ticks;
	 pclkblk->recycle = ((struct clkarg *)p2)->period;

	 /* mark object type */
	 pclkblk->objtype = TIMER_OBJ;
	 pclkblk->task = hipritsk->task;

	 /* force Sema pending in case it was already DONE */
	 if ( (semat[(int)(pclkblk->objid = ((struct clkarg *)p2)->sema)]
			    == SEMA_DONE)
				 &&
	      ( ((struct clkarg *)p2)->period == (TICKS)0) )
	    semat[(int)(pclkblk->objid)] = SEMA_PENDING;

	 /* insert timer */
	 insert_timer(pclkblk);

	 break;
#endif /* } HAS_START_TIMER */

#ifdef HAS_DELAY /* { */
/************************/
      case RTXC_DELAY:
/************************/
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef USE_REGISTERDEBUG
DumpReg("RTXC_DELAY in", hipritsk->sp) ;
#endif // USE_REGISTERDEBUG
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 if ( ((task = ((struct delayarg *)p2)->task) == SELFTASK) ||
	       (task == hipritsk->task) ) /* delaying self */
	 {
	    if ( ((struct delayarg *)p2)->ticks == 0)
	       break;	/* zero time for self = NOP */
	    ptcb = hipritsk;
	    task = ptcb->task;
	    pclkblk = ((struct delayarg *)p2)->pclkblk;
	 }
	 else  /* current task is delaying another task */
	 {
	    ptcb = &rtxtcb[(int)task];

	    if (ptcb->status & DELAY_WAIT)
	    {
	       /* if task is already delayed ... */
	       /* get current clkblk and unlink from active timers */
	       pclkblk = ptcb->pclkblk;
	       unlink_timer(pclkblk);

	       if ( ((struct delayarg *)p2)->ticks == 0)
	       {
		  /* if 0 delay, clear DELAY_WAIT flag ... */
		  if ( (ptcb->status &= ~DELAY_WAIT) == READY)
		  {
		     /* ... if task is now READY, put in ready list */
		     ptcb->flink = nsrttcb;
		     nsrttcb = ptcb;
		     scheduler = 0;
		  }

		  break;
	       }
	    }
	    else  /* task was not already delayed */
	       /* allocate space on task's personal stack for clkblk */
	       pclkblk = (CLKBLK *)(ptcb->sp) - 1;
	 }

	 ptcb->pclkblk = pclkblk; /* put pointer to clkblk in tcb */

	 if (ptcb->status == READY)
	 {
	       /* general unlink from tcb READY list */
	    ptcb->flink->blink = ptcb->blink;
	    ptcb->blink->flink = ptcb->flink;
	 }

	 ptcb->status |= DELAY_WAIT;

	 pclkblk->remain = ((struct delayarg *)p2)->ticks;
	 pclkblk->recycle = (TICKS)0;

	 /* mark object type */
	 pclkblk->objtype = DELAY_OBJ;
	 pclkblk->objid = task;
	 pclkblk->task = task;

	 /* insert timer */
	 insert_timer(pclkblk);
#ifdef SUPPOSEDBUGVERSION
	 if (ptcb == hipritsk) {
	     UNLINK_HIPRITSK() ;    // unlink first tcb _BM_ 04/12/1997
	 }
#endif
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef USE_REGISTERDEBUG
DumpReg("RTXC_DELAY out", hipritsk->sp) ;
#endif // USE_REGISTERDEBUG
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	 break;
#endif /* } HAS_DELAY */

#ifdef HAS_ELAPSE /* { */
/************************/
      case RTXC_ELAPSE:
/************************/
	 DISABLE; /* snapshot rtctick */
	 tcnt = rtctick;
	 ENABLE;
	 ((struct etarg *)p2)->val = tcnt -
			       *(((struct etarg *)p2)->stamp);
	 *(((struct etarg *)p2)->stamp) = tcnt;
	 break;
#endif /* } HAS_ELAPSE */

#ifdef HAS_INQTIMER /* { */
/************************/
      case RTXC_INQTIMER:
/************************/
	 pclkblk = ((struct clkarg *)p2)->pclkblk;

	 if (pclkblk->state == TIMER_ACTIVE)
	 {
	    DISABLE; /* prevent changing rtctick while reading */
	    ((struct clkarg *)p2)->ticks = pclkblk->remain - rtctick;
	    ENABLE;
	 }
	 else
	    ((struct clkarg *)p2)->ticks = (TICKS)0;
	 break;
#endif /* } HAS_INQTIMER */

#ifdef HAS_INQTIME /* { */
/************************/
      case RTXC_INQTIME:
/************************/
	 DISABLE; /* prevent changing while reading */
	 ((struct timearg *)p2)->time = rtctime;
	 ENABLE;
	 break;
#endif /* } HAS_INQTIME */

#ifdef HAS_DEFTIME /* { */
/************************/
      case RTXC_DEFTIME:
/************************/
	 DISABLE; /* prevent changing while writing */
	 rtctime = ((struct timearg *)p2)->time;
	 ENABLE;
	 break;
#endif /* } HAS_DEFTIME */

#ifdef HAS_MAILBOXES /* { */
#ifdef HAS_SEND /* { */
/************************/
      case RTXC_SEND:
/************************/
	 if ( ((struct msgarg *)p2)->priority == (PRIORITY)0)
	 {
	    /* priority = 0 yields sender's priority */
	    ((struct msgarg *)p2)->prtxcmsg->priority =
	       hipritsk->priority;
	 }
	 else
	 {
	    ((struct msgarg *)p2)->prtxcmsg->priority =
	       ((struct msgarg *)p2)->priority;
	 }

	 ((struct msgarg *)p2)->prtxcmsg->sema =
	    ((struct msgarg *)p2)->sema;

	 ((struct msgarg *)p2)->prtxcmsg->task = hipritsk->task;

	 pmh = &mheader[((struct msgarg *)p2)->mbox];

#ifdef MAILBOX_SEMAS /* { */
	 /* if was empty and there is a not_empty semaphore defined */
	 if ( (pmh->link == NULL) && (pmh->nesema != NULLSEMA) )
	    update_sema(pmh->nesema);

#endif /* } MAILBOX_SEMAS */

#ifdef MAILBOX_WAITERS /* { */
	 if ( (ptcb = pmh->waiters) != NULLTCB) /* if a task waiting */
	 {
	    /* pass data directly from sender to receiver */
	    p2a = ((FRAME *)(ptcb->sp))->pksnum;

	    /* if receiving from anybody (0) or a specific task match */
	    if ( (((struct msgarg *)p2a)->task == (TASK)0) ||
		 (((struct msgarg *)p2a)->task ==
		  hipritsk->task) )
	    {
	       /* pass &message directly to receiver */
	       ((struct msgarg *)p2a)->prtxcmsg =
		  ((struct msgarg *)p2)->prtxcmsg;

#ifdef CBUG /* { */
	       pmh->count++;
#endif /* } CBUG */

	       /* remove tcb from waiter's list */
	       if ( (pmh->waiters = ptcb->flink) != NULLTCB)
		  ptcb->flink->blink = (TCB *)&pmh->waiters;

	       /* clear MSG WAIT in receiving task and check if runnable */
	       if ( (ptcb->status &= ~MSG_WAIT) == READY)
	       {
		  /* insert waiter into READY list */
		  ptcb->flink = nsrttcb;
		  nsrttcb = ptcb;
		  scheduler = 0;
	       }

#ifdef MAILBOX_TIMEOUTS /* { */
	       /* cleanup any pending timeout */
	       if ( (pclkblk = ptcb->pclkblk) != NULLCLK)
	       {
		  /* mark waiter GOOD */
		  ((struct msgarg *)p2a)->ksrc = RC_GOOD;

		  unlink_timer(pclkblk);
		  ptcb->pclkblk = NULLCLK;
	       }
#endif /* } MAILBOX_TIMEOUTS */
	    }
	    else
	    {
	       insert_message(pmh, ((struct msgarg *)p2)->prtxcmsg);
	    }
	 }
	 else
#endif /* } MAILBOX_WAITERS */
	 {
	    insert_message(pmh, ((struct msgarg *)p2)->prtxcmsg);
	 }

	 sema = ((struct msgarg *)p2)->sema;

#ifdef MAILBOX_WAITERS /* { */
	 /* if need to wait on response (sema) */
	 if ( ((struct msgarg *)p2)->wait)
	 {
	    ptcb = hipritsk; /* save for later */

	    semat[sema] = (SSTATE)ptcb->task;

	    ptcb->status = SEMAPHORE_WAIT;

	    UNLINK_HIPRITSK(); /* unlink first tcb */

#ifdef SEMAPHORE_TIMEOUTS /* { */
	    if ( ((struct msgarg *)p2)->ticks) /* if timeout active */
	    {
	       ptcb->pclkblk = ((struct msgarg *)p2)->pclkblk;
	       pclkblk = ptcb->pclkblk;
	       pclkblk->remain = ((struct msgarg *)p2)->ticks;
	       pclkblk->recycle = (TICKS)0;

	       /* mark object type */
	       pclkblk->objtype = SEMAPHORE_OBJ;
	       pclkblk->objid = sema;
	       pclkblk->task = ptcb->task;

	       /* insert timer */
	       insert_timer(pclkblk);

	       ((struct msgarg *)p2)->ksrc = RC_TIMEOUT;
	    }
#endif /* } SEMAPHORE_TIMEOUTS */
	 }
	 else
#endif /* } MAILBOX_WAITERS */
	 {
	    if (sema != NULLSEMA)
	       semat[sema] = SEMA_PENDING;
	 }
	 break;
#endif /* } HAS_SEND */

#ifdef HAS_RECEIVE /* { */
/************************/
      case RTXC_RECEIVE:
/************************/
	 pmh = &mheader[((struct msgarg *)p2)->mbox];

	 /* if no messages are available */
	 if ( (prtxcmsg = remove_message(pmh,
			    ((struct msgarg *)p2)->task)) == NULL)
	 {
#ifdef MAILBOX_WAITERS /* { */
	    if ( ((struct msgarg *)p2)->wait)
	    {
	       ptcb = hipritsk; /* save for later */

	       ptcb->status = MSG_WAIT; /* set MSG_WAIT in status word */

	       /* thread waiting task in priority order */
	       porder((TCB *)&pmh->waiters);

#ifdef MAILBOX_TIMEOUTS /* { */
	       if ( ((struct msgarg *)p2)->ticks) /* if timeout active*/
	       {
		  ptcb->pclkblk = ((struct msgarg *)p2)->pclkblk;
		  pclkblk = ptcb->pclkblk;
		  pclkblk->remain = ((struct msgarg *)p2)->ticks;
		  pclkblk->recycle = (TICKS)0;

		  /* mark object type */
		  pclkblk->objtype = MAILBOX_OBJ;
		  pclkblk->objid = ((struct msgarg *)p2)->mbox;
		  pclkblk->task = ptcb->task;

		  /* insert timer */
		  insert_timer(pclkblk);

		  ((struct msgarg *)p2)->ksrc = RC_TIMEOUT;
	       }
#endif /* } MAILBOX_TIMEOUTS */
	    }
#endif /* } MAILBOX_WAITERS */
	 }
	 else /* message returned to caller */
	 {
#ifdef CBUG /* { */
	    pmh->count++;
#endif /* } CBUG */

#ifdef MAILBOX_SEMAS /* { */
	    if (pmh->nesema != NULLSEMA)
	    {
	       /* if now empty */
	       if (pmh->link == NULL)
		  semat[pmh->nesema] = SEMA_PENDING; /* reset */
	       else
		  semat[pmh->nesema] = SEMA_DONE; /* force not empty */
	    }
#endif /* } MAILBOX_SEMAS */
	 }

	 /* return msg to caller */
	 ((struct msgarg *)p2)->prtxcmsg = prtxcmsg;
	 break;
#endif /* } HAS_RECEIVE */
#endif /* } HAS_MAILBOXES */

#ifdef HAS_QUEUES /* { */
#ifdef HAS_ENQUEUE /* { */
/************************/
      case RTXC_ENQUEUE:
/************************/
	 pqh = &qheader[(int)(((struct qarg *)p2)->queue)];

	 if ( (depth = pqh->depth) == pqh->curndx) /* queue was already full */
	 {
#ifdef ENQUEUE_WAITERS /* { */
	    if ( ((struct qarg *)p2)->wait)
	    {
	       ptcb = hipritsk; /* save hipritsk for later */

	       ptcb->status = QUEUE_WAIT; /* set QUEUE_WAIT in status */

	       /* thread waiting task in priority order */
	       porder((TCB *)&pqh->waiters);

#ifdef ENQUEUE_TIMEOUTS /* { */
	       if ( ((struct qarg *)p2)->ticks) /* if timeout active */
	       {
		  ptcb->pclkblk = ((struct qarg *)p2)->pclkblk;
		  pclkblk = ptcb->pclkblk;
		  pclkblk->remain = ((struct qarg *)p2)->ticks;
		  pclkblk->recycle = (TICKS)0;

		  /* mark object type */
		  pclkblk->objtype = QUEUE_OBJ;
		  pclkblk->objid = ((struct qarg *)p2)->queue;
		  pclkblk->task = ptcb->task;

		  /* insert timer */
		  insert_timer(pclkblk);
	       }
#endif /* } ENQUEUE_TIMEOUTS */
	    }
#endif /* } ENQUEUE_WAITERS */
	    break;
	 }

	 ((struct qarg *)p2)->ksrc = RC_GOOD;

	 width = pqh->width;

#ifdef DEQUEUE_WAITERS /* { */
	 /* if queue is empty && if there are waiters */
	 if ( (pqh->curndx == 0) && ((ptcb = pqh->waiters) != NULLTCB) )
	 {
	    /* move data directly to first waiter */

	    /* remove highest priority waiter (1st) */
	    if ( (pqh->waiters = ptcb->flink) != NULLTCB)
		 ptcb->flink->blink = (TCB *)&pqh->waiters;

	    if ( (ptcb->status &= ~QUEUE_WAIT) == READY)
	    {
	       ptcb->flink = nsrttcb;
	       nsrttcb = ptcb;
	       scheduler = 0;
	    }

#ifdef DEQUEUE_TIMEOUTS /* { */
	    /* cleanup any pending timeout */
	    if ( (pclkblk = ptcb->pclkblk) != NULLCLK)
	    {
	       unlink_timer(pclkblk);
	       ptcb->pclkblk = NULLCLK;
	    }
#endif /* } DEQUEUE_TIMEOUTS */

	    p2a = ((FRAME *)(ptcb->sp))->pksnum;

	    ((struct qarg *)p2a)->ksrc = RC_GOOD;

#ifdef QUEUE_MEMCPY /* { */
	    if (width)
	       memcpy( ((struct qarg *)p2a)->data,
		       ((struct qarg *)p2)->data,
		       width);
#else /* } QUEUE_MEMCPY { */
	    switch(width)
	    {
	       case 0:
		  break;

	       case sizeof(char):
		  *(char *)(((struct qarg *)p2a)->data) =
			      *(char *)(((struct qarg *)p2)->data);
		  break;

	       case sizeof(short):
		  *(short *)(((struct qarg *)p2a)->data) =
			      *(short *)(((struct qarg *)p2)->data);
		  break;

	       case sizeof(long):
		  *(long *)(((struct qarg *)p2a)->data) =
			      *(long *)(((struct qarg *)p2)->data);
		  break;

	       default:
		  memcpy( ((struct qarg *)p2a)->data,
			  ((struct qarg *)p2)->data,
			  width);
		  break;
	    }
#endif /* } QUEUE_MEMCPY */

#ifdef CBUG /* { */
	    pqh->count++; /* increment total no. of enqueues to date */
#endif /* } CBUG */

#ifdef QUEUE_SEMAS /* { */
	    /* queue was empty - process not empty sema for consumer */
	    if (pqh->nesema != NULLSEMA)
	       update_sema(pqh->nesema);

	    /* queue was empty - process not full sema for producer */
	    if (pqh->nfsema != NULLSEMA)
	       semat[(int)(pqh->nfsema)] = SEMA_DONE;
#endif /* } QUEUE_SEMAS */
	    break;
	 }
#endif /* } DEQUEUE_WAITERS */

	 /* move data into queue */
	 pqh->curndx++;
	 if (++pqh->putndx == depth)
	    pqh->putndx = 0;  /* recycle index */

	 qindex = pqh->putndx; /* local for speed */

#ifdef QUEUE_MEMCPY /* { */
	 if (width)
	    memcpy(pqh->base + (qindex * width),
		   ((struct qarg *)p2)->data, width);
#else /* } QUEUE_MEMCPY { */
	 switch(width)
	 {
	    case 0:
	       break;

	    case sizeof(char):
	       *(char *)(pqh->base + qindex) =
				  *(char *)(((struct qarg *)p2)->data);
	       break;

	    case sizeof(short):
	       *(short *)(pqh->base + (qindex << 1)) =
				  *(short *)(((struct qarg *)p2)->data);
	       break;

	    case sizeof(long):
	       *(long *)(pqh->base + (qindex << 2)) =
				  *(long *)(((struct qarg *)p2)->data);
	       break;

	    default:
	       memcpy(pqh->base + (qindex * width),
		       ((struct qarg *)p2)->data,
		       width);
	       break;
	 }
#endif /* } QUEUE_MEMCPY */

#ifdef CBUG /* { */
	 pqh->count++; /* increment total no. of enqueues to date */
	 if (pqh->curndx > pqh->worst) /* check for new worst case full condition */
	    pqh->worst = pqh->curndx;
#endif /* } CBUG */

#ifdef QUEUE_SEMAS /* { */
	 if (pqh->curndx == depth)		 /* queue is now full */
	 {
	    if (pqh->nfsema != NULLSEMA)
	       semat[(int)(pqh->nfsema)] = SEMA_PENDING; /*set not full sema pending*/

	    if (pqh->fsema != NULLSEMA)
	       update_sema(pqh->fsema);
	 }
	 else				    /* still room in queue */
	 {
	    if (pqh->nfsema != NULLSEMA)
	       semat[(int)(pqh->nfsema)] = SEMA_DONE;
	 }

	 if (pqh->curndx == 1) /* if was empty */
	 {
	    if (pqh->esema != NULLSEMA)
	       semat[(int)(pqh->esema)] = SEMA_PENDING;

	    if (pqh->nesema != NULLSEMA)
	       update_sema(pqh->nesema);
	 }
#endif /* } QUEUE_SEMAS */
	 break;
#endif /* } HAS_ENQUEUE */

#ifdef HAS_DEQUEUE /* { */
/************************/
      case RTXC_DEQUEUE:
/************************/
	 pqh = &qheader[(int)(((struct qarg *)p2)->queue)];

	 if (pqh->curndx == 0)	/* empty queue */
	 {
#ifdef DEQUEUE_WAITERS /* { */
	    if ( ((struct qarg *)p2)->wait)
	    {
	       ptcb = hipritsk; /* save copy of hipritsk */

	       ptcb->status = QUEUE_WAIT; /* set QUEUE_WAIT in status */

	       /* thread waiting task in priority order */
	       porder((TCB *)&pqh->waiters);

#ifdef DEQUEUE_TIMEOUTS /* { */
	       if ( ((struct qarg *)p2)->ticks) /* if timeout active */
	       {
		  ptcb->pclkblk = ((struct qarg *)p2)->pclkblk;
		  pclkblk = ptcb->pclkblk;
		  pclkblk->remain = ((struct qarg *)p2)->ticks;
		  pclkblk->recycle = (TICKS)0;

		  /* mark object type */
		  pclkblk->objtype = QUEUE_OBJ;
		  pclkblk->objid = ((struct qarg *)p2)->queue;
		  pclkblk->task = ptcb->task;

		  /* insert timer */
		  insert_timer(pclkblk);
	       }

#endif /* } DEQUEUE_TIMEOUTS */
	    }

#endif /* } DEQUEUE_WAITERS */
	    break;
	 }

	 /* queue not empty, get entry */

	 ((struct qarg *)p2)->ksrc = RC_GOOD;

	 qindex = pqh->putndx - --pqh->curndx;
	 if (qindex < 0)
	    qindex += pqh->depth;  /* wrap around */

#ifdef QUEUE_MEMCPY /* { */
	 if ( (width = pqh->width) != 0)
	    memcpy( ((struct qarg *)p2)->data,
		    pqh->base + (qindex * width),
		    width);
#else /* } QUEUE_MEMCPY { */
	 switch(width = pqh->width)
	 {
	    case 0:
	       break;

	    case sizeof(char):
	       *(char *)(((struct qarg *)p2)->data) =
					 *(char *)(pqh->base + qindex);
	       break;

	    case sizeof(short):
	       *(short *)(((struct qarg *)p2)->data) =
					 *(short *)(pqh->base + (qindex << 1));
	       break;

	    case sizeof(long):
	       *(long *)(((struct qarg *)p2)->data) =
					 *(long *)(pqh->base + (qindex << 2));
	       break;

	    default:
	       memcpy( ((struct qarg *)p2)->data,
		       pqh->base + (qindex * width),
		       width);
	       break;
	 }
#endif /* } QUEUE_MEMCPY */

#ifdef QUEUE_SEMAS /* { */
	 if (pqh->curndx == 0)	/* just dequeued last item (now empty) */
	 {
	    if (pqh->nesema != NULLSEMA)
	       semat[(int)(pqh->nesema)] = SEMA_PENDING; /* pend not empty sema */

	    if (pqh->esema != NULLSEMA)
	       update_sema(pqh->esema);
	 }
	 else /* there are still more entries in queue */
	 {
	    if (pqh->nesema != NULLSEMA)
	       semat[(int)(pqh->nesema)] = SEMA_DONE; /* set not empty sema done */
	 }
#endif /* } QUEUE_SEMAS */

#ifdef ENQUEUE_WAITERS /* { */
	 if (pqh->curndx == pqh->depth - 1)  /* queue was full before deq */
	 {
	    /* insert first waiter into insert list */
	    if ( (ptcb = pqh->waiters) != NULLTCB)
	    {
	       /* remove highest priority waiter */
	       if ( (pqh->waiters = ptcb->flink) != NULLTCB)
		  ptcb->flink->blink = (TCB *)&pqh->waiters;

	       if ( (ptcb->status &= ~QUEUE_WAIT) == READY)
	       {
		  ptcb->flink = nsrttcb;
		  nsrttcb = ptcb;
		  scheduler = 0;
	       }

#ifdef ENQUEUE_TIMEOUTS /* { */
	       /* cleanup any pending timeout */
	       if ( (pclkblk = ptcb->pclkblk) != NULLCLK)
	       {
		  unlink_timer(pclkblk);
		  ptcb->pclkblk = NULLCLK;
	       }
#endif /* } ENQUEUE_TIMEOUTS */

	       /* perform enqueue for waiter */

	       pqh->curndx++;
	       if (++pqh->putndx == pqh->depth)
		  pqh->putndx = 0;  /* recycle index */

	       qindex = pqh->putndx; /* local for speed */

	       p2a = ((FRAME *)(ptcb->sp))->pksnum;

	       ((struct qarg *)p2a)->ksrc = RC_GOOD;

#ifdef QUEUE_MEMCPY /* { */
	       if (width)
		  memcpy(pqh->base + (qindex * width),
			 ((struct qarg *)p2a)->data,
			 width);
#else /* } QUEUE_MEMCPY { */
	       switch(width)
	       {
		  case 0:
		     break;

		  case sizeof(char):
		     *(char *)(pqh->base + qindex) =
				 *(char *)(((struct qarg *)p2a)->data);
		     break;

		  case sizeof(short):
		     *(short *)(pqh->base + (qindex << 1)) =
				 *(short *)(((struct qarg *)p2a)->data);
		     break;

		  case sizeof(long):
		     *(long *)(pqh->base + (qindex << 2)) =
				 *(long *)(((struct qarg *)p2a)->data);
		     break;

		  default:
		     memcpy(pqh->base + (qindex * width),
			     ((struct qarg *)p2a)->data,
			     width);
		    break;
	       }
#endif /* } QUEUE_MEMCPY */

#ifdef CBUG /* { */
	       pqh->count++; /* increment total no. of enqueues to date */
	       /* already past worst case */
#endif /* } CBUG */

#ifdef QUEUE_SEMAS /* { */
	       if (pqh->curndx == pqh->depth) /* queue is now full */
	       {
		 if (pqh->nfsema != NULLSEMA)
		    semat[(int)(pqh->nfsema)] = SEMA_PENDING; /*pend not full sema*/

		 if (pqh->fsema != NULLSEMA)
		    update_sema(pqh->fsema);
	       }
	       else				  /* still room in queue */
	       {
		  if (pqh->nfsema != NULLSEMA)
		     semat[(int)(pqh->nfsema)] = SEMA_DONE;
	       }
#endif /* } QUEUE_SEMAS */

#ifdef QUEUE_SEMAS /* { */
	       if (pqh->curndx == 1) /* if was empty */
	       {
		  if (pqh->nesema != NULLSEMA)
		  {
			   /* signal q not empty */
		     if (semat[(int)(pqh->nesema)] == SEMA_PENDING)
			semat[(int)(pqh->nesema)] = SEMA_DONE;
		     else if (semat[(int)(pqh->nesema)] != SEMA_DONE)
		     {
			SIGNAL(pqh->nesema);
		     }
		  }
	       }
#endif /* } QUEUE_SEMAS */
	    }
	    else
	    {
#ifdef QUEUE_SEMAS /* { */
	       if (pqh->fsema != NULLSEMA)
		  semat[(int)(pqh->fsema)] = SEMA_PENDING;

	       if (pqh->nfsema != NULLSEMA)
		  update_sema(pqh->nfsema);
#endif /* } QUEUE_SEMAS */
	    }
	 }
	 else
#endif /* } ENQUEUE_WAITERS */
	 {
#ifdef QUEUE_SEMAS /* { */
	    if (pqh->nfsema != NULLSEMA)
	       update_sema(pqh->nfsema);
#endif /* } QUEUE_SEMAS */
	 }
       break;
#endif /* } HAS_DEQUEUE */
#endif /* } HAS_QUEUES */

#ifdef HAS_DEFQUEUE /* { */
/************************/
      case RTXC_DEFQUEUE:
/************************/
	 pqh = &qheader[(int)(((struct qdefarg *)p2)->queue)];

	 pqh->base   = ((struct qdefarg *)p2)->base;
	 pqh->width  = ((struct qdefarg *)p2)->width;
	 pqh->depth  = ((struct qdefarg *)p2)->depth;
	 cursz = ((struct qdefarg *)p2)->current_size;

	 if ( (cursz < 0) || (cursz > pqh->depth) )
	    ((struct qarg *)p2)->ksrc = RC_ILLEGAL_QUEUE_SIZE;

	 else if (cursz == 0)
	 {
	    pqh->putndx = pqh->depth - 1; /* purge queue */
	    pqh->curndx = 0;
	 }

	 else
	 {
	    pqh->curndx = cursz;
	    pqh->putndx = cursz - 1;
	 }
	 break;
#endif /* } HAS_DEFQUEUE */

#ifdef QUEUE_SEMAS /* { */
#ifdef HAS_DEFQSEMA /* { */
/************************/
      case RTXC_DEFQSEMA:
/************************/
	 pqh = &qheader[(int)(((struct qdefarg *)p2)->queue)];

	 switch(((struct qdefarg *)p2)->qcond)
	 {
	    /* note: semaphore is 0 == undefqsema() and harmless */
	    case QNE:
	       pqh->nesema = ((struct qdefarg *)p2)->sema;
	       if (pqh->curndx != 0) /* if not empty */
		  semat[(int)(pqh->nesema)] = SEMA_DONE;
	       else
		  semat[(int)(pqh->nesema)] = SEMA_PENDING;
	       break;

	    case QNF:
	       pqh->nfsema = ((struct qdefarg *)p2)->sema;
	       if (pqh->curndx != pqh->depth) /* if not full */
		  semat[(int)(pqh->nfsema)] = SEMA_DONE;
	       else
		  semat[(int)(pqh->nfsema)] = SEMA_PENDING;
	       break;

	    case QE:
	       pqh->esema = ((struct qdefarg *)p2)->sema;
	       if (pqh->curndx == 0) /* if empty */
		  semat[(int)(pqh->esema)] = SEMA_DONE;
	       else
		  semat[(int)(pqh->esema)] = SEMA_PENDING;
	       break;

	    case QF:
	       pqh->fsema = ((struct qdefarg *)p2)->sema;
	       if (pqh->curndx == pqh->depth) /* if full */
		  semat[(int)(pqh->fsema)] = SEMA_DONE;
	       else
		  semat[(int)(pqh->fsema)] = SEMA_PENDING;
	       break;
	 }
	 break;
#endif /* } HAS_DEFQSEMA */
#endif /* } QUEUE_SEMAS */

#ifdef MAILBOX_SEMAS /* { */
#ifdef HAS_DEFMBOXSEMA /* { */
/************************/
      case RTXC_DEFMBOXSEMA:
/************************/
	 pmh = &mheader[((struct msgarg *)p2)->mbox];

	 /* note: semaphore is 0 == undefmboxsema() and harmless */
	 pmh->nesema = ((struct msgarg *)p2)->sema;

	 if (pmh->link == NULL)
	     semat[pmh->nesema] = SEMA_PENDING;
	 else
	     semat[pmh->nesema] = SEMA_DONE;
	 break;
#endif /* } HAS_DEFMBOXSEMA */
#endif /* } MAILBOX_SEMAS */

#ifdef HAS_PURGEQUEUE /* { */
/************************/
      case RTXC_PURGEQUEUE:
/************************/
	 pqh = &qheader[(int)(((struct qarg *)p2)->queue)];

	 pqh->putndx = pqh->depth - 1;
	 qindex = pqh->curndx; /* save original size for later use */
	 pqh->curndx = 0;

#ifdef QUEUE_SEMAS /* { */
	 if (pqh->esema != NULLSEMA)
	    update_sema(pqh->esema);
#endif /* } QUEUE_SEMAS */

	 depth = pqh->depth;

	 /* if queue was full */
	 if (qindex == depth)
	 {
#ifdef QUEUE_SEMAS /* { */
	    if (pqh->nfsema != NULLSEMA) /* if not_full sema in use */
	       update_sema(pqh->nfsema);
#endif /* } QUEUE_SEMAS */

	    width = pqh->width;

#ifdef QUEUE_WAITERS /* { */
	    /* might have enq or deq waiters to process */
	    while ( (ptcb = pqh->waiters) != NULLTCB)
	    {
	       /* remove highest priority waiter (1st) */
	       if ( (pqh->waiters = ptcb->flink) != NULLTCB)
		  ptcb->flink->blink = (TCB *)&pqh->waiters;

	       if ( (ptcb->status &= ~QUEUE_WAIT) == READY)
	       {
		  ptcb->flink = nsrttcb;
		  nsrttcb = ptcb;
		  scheduler = 0;
	       }

#ifdef QUEUE_TIMEOUTS /* { */
	       /* cleanup any pending timeout */
	       if ( (pclkblk = ptcb->pclkblk) != NULLCLK)
	       {
		  unlink_timer(pclkblk);
		  ptcb->pclkblk = NULLCLK;
	       }
#endif /* } QUEUE_TIMEOUTS */

	       p2a = ((FRAME *)(ptcb->sp))->pksnum;

	       ((struct qarg *)p2a)->ksrc = RC_GOOD;

	       /* move data into queue */
	       pqh->curndx++;
	       if (++pqh->putndx == depth)
		  pqh->putndx = 0;  /* recycle index */

	       qindex = pqh->putndx;

#ifdef QUEUE_MEMCPY /* { */
	       if (width)
		  memcpy(pqh->base + (qindex * width),
			 ((struct qarg *)p2a)->data,
			 width);
#else /* } QUEUE_MEMCPY { */
	       switch(width)
	       {
		  case 0:
		     break;

		  case sizeof(char):
		     *(char *)(pqh->base + qindex) =
			      *(char *)(((struct qarg *)p2a)->data);
		     break;

		  case sizeof(short):
		     *(short *)(pqh->base + (qindex << 1)) =
			       *(short *)(((struct qarg *)p2a)->data);
		     break;

		  case sizeof(long):
		     *(long *)(pqh->base + (qindex << 2)) =
			      *(long *)(((struct qarg *)p2a)->data);
		     break;

		  default:
		     memcpy(pqh->base + (qindex * width),
			   ((struct qarg *)p2a)->data,
			   width);
		     break;
	       }
#endif /* } QUEUE_MEMCPY */

#ifdef CBUG /* { */
	       pqh->count++; /* increment total no. of enqueues to date */
	       if (pqh->curndx > pqh->worst) /* check for new worst case */
		  pqh->worst = pqh->curndx;
#endif /* } CBUG */

#ifdef QUEUE_SEMAS /* { */
	       /* queue was empty - process not empty sema for consumer */
	       if ( (pqh->curndx == 1) && (pqh->nesema != NULLSEMA) )
		  update_sema(pqh->nesema);
#endif /* } QUEUE_SEMAS */

	    } /* end of while */
#endif /* } QUEUE_WAITERS */

	 }
	 break;
#endif /* } HAS_PURGEQUEUE */

/************************/
      case RTXC_EXECUTE:
/************************/
	 ptcb = &rtxtcb[(int)(task = ((struct targ *)p2)->task)];

	 /* if task is already executing, then stop and restart */
	 if (ptcb->status == READY)
	 {
	    ptcb->flink->blink = ptcb->blink; /* general unlink */
	    ptcb->blink->flink = ptcb->flink;
	 }

	 /* initialize task stack pointer */
	 // frame = (FRAME *)(ptcb->stackbase +
	 //		       ptcb->stacksize - sizeof(FRAME));
         frame = (FRAME *)( (rtxkktcb[(int)(task)].stackbase) +
                            (rtxkktcb[(int)(task)].stacksize) - sizeof(FRAME) ) ;

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef CBUG
//        frame->r1 = 0x01010101 ;
//        frame->r2 = 0x02020202 ;
//        frame->r3 = 0x03030303 ;
//        frame->r4 = 0x04040404 ;
//        frame->r5 = 0x05050505 ;
//        frame->r6 = 0x06060606 ;
//        frame->r7 = 0x07070707 ;
//        frame->r8 = 0x08080808 ;
//        frame->r9 = 0x09090909 ;
//        frame->r10 = 0x10101010 ;
//        frame->r11 = 0x11111111 ;
//        frame->r12 = 0x12121212 ;

//        {
//            int i ;
//            unsigned long *ptr ;
//            ptr = (unsigned long *)(frame) ;
//            ptr++ ; // skip usercc
//            for(i=0 ; i<15 ; i++) {
//                ptr[i] = (i<<24) | (i<<16) | (i<<8) | i ;
//            }
//        }
#endif // CBUG
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	 ptcb->sp = frame ;

	 // initialize Program Counter
	 //frame->userpc = (unsigned long)(ptcb->userpc_t0) /* & (~1L) */ ;
	 frame->userpc = (unsigned long)(rtxkktcb[(int)(task)].userpc_t0) ;

	 frame->userccr = CCR_T0;  // initialize status register at task startup
	 // frame->r1 = 0 ;       // a must for compiler

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
         // init user SP
	 //frame->r13 = (unsigned long)(ptcb->stackbase + ptcb->stacksize) ;
	 frame->r13 = (unsigned long)( (rtxkktcb[(int)(task)].stackbase) +
                                       (rtxkktcb[(int)(task)].stacksize) ) ;
#endif //defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#ifdef TIME_SLICE /* { */
	 ptcb->tslice = 0;
	 ptcb->newslice = 0;
#endif /* } TIME_SLICE */

#ifdef SYNC_START /* { */
	 if (hipritsk->task == 0)
	    ptcb->status = BLOCK_WAIT; /* mark task blocked */
	 else
	    ptcb->status = READY; /* mark task runnable */
#else /* } SYNC_START { */
	 ptcb->status = READY; /* mark task runnable */
#endif /* } SYNC_START */

#ifdef FPU /* { */
	  /* if fpregs area allocated */
	 if ( (fpregs = rtxtcb[task].fpregs) != NULLFPREGS)
	 {
		  /* init fpu to known state */

	    /* processor dependent fpu initialization goes here */
		       /* fpregs->? = ?; */

	    ptcb->fpumode = 1;	       /* mark task as fpu user */
	 }
#ifdef BSS_NOT_ZERO /* { */
	 else
	 {
	    ptcb->fpumode = 0;	       /* mark task as non fpu user */
	 }
#endif /* } BSS_NOT_ZERO */
#endif /* } FPU */

#ifdef SYNC_START /* { */
	 if (ptcb->status == READY)
	 {
	    /* insert new task into list for later */
	    ptcb->flink = nsrttcb;
	    nsrttcb = ptcb;
	    scheduler = 0;
	 }
#else /* } SYNC_START { */
	 /* insert new task into list for later */
	 ptcb->flink = nsrttcb;
	 nsrttcb = ptcb;
	 scheduler = 0;
#endif /* } SYNC_START */
	 break;

#ifdef HAS_DEFTASK /* { */
/************************/
      case RTXC_DEFTASK:
/************************/
	 if ( (task = ((struct deftaskarg *)p2)->task) == SELFTASK)
	 {
	    ((struct deftaskarg *)p2)->ksrc = RC_ILLEGAL_TASK;
	    break;
	 }

	 ptcb = &rtxtcb[(int)task];

	 /* if task is "active", then abort deftask operation */
	 if ( (ptcb->status & INACTIVE) != INACTIVE)
	 {
	    ((struct deftaskarg *)p2)->ksrc = RC_ACTIVE_TASK;
	    break;
	 }

	 ptcb->priority =  ((struct deftaskarg *)p2)->priority;

	 if ( ((struct deftaskarg *)p2)->stackbase) /* new stack */
	 {
	    ptcb->stackbase = ((struct deftaskarg *)p2)->stackbase;
	    ptcb->stacksize = ((struct deftaskarg *)p2)->stacksize;
	 }

#ifdef FPU /* { */
	 ptcb->fpregs = NULLFPREGS;
#endif /* } FPU */

	 ptcb->swpc_t0 = ((struct deftaskarg *)p2)->entry;

	 ptcb->pclkblk = NULLCLK;

#ifdef HAS_INQTASK_ARG /* { */
	 ptcb->arg = NULL;
#endif /* } HAS_INQTASK_ARG */

#ifdef TIME_SLICE /* { */
	 ptcb->tslice = 0;
	 ptcb->newslice = 0;
#endif /* } TIME_SLICE */

	 break;
#endif /* } HAS_DEFTASK */

#ifdef HAS_INQTASK_ARG /* { */
/************************/
      case RTXC_INQTASK_ARG:
/************************/
	 if ( (task = ((struct deftaskarg *)p2)->task) == SELFTASK)
	    task = hipritsk->task;
	 ((struct deftaskarg *)p2)->arg = rtxtcb[(int)task].arg;
	 break;
#endif /* } HAS_INQTASK_ARG */

#ifdef HAS_DEFTASK_ARG /* { */
/************************/
      case RTXC_DEFTASK_ARG:
/************************/
	 if ( (task = ((struct deftaskarg *)p2)->task) == SELFTASK)
	    task = hipritsk->task;
	 rtxtcb[(int)task].arg = ((struct deftaskarg *)p2)->arg;
	 break;
#endif /* } HAS_DEFTASK_ARG */

#ifdef HAS_ALLOC_TASK /* { */
/************************/
      case RTXC_ALLOC_TASK:
/************************/
	 if ( (ptcb = dtcbfl) != NULLTCB) /* allocate tcb */
	 {
	    dtcbfl = ptcb->flink; /* by removing 1st from free list */
	    ((struct targ *)p2)->task = ptcb->task;
	 }
	 else
	    ((struct targ *)p2)->task = (TASK)0;
	 break;
#endif /* } HAS_ALLOC_TASK */

#ifdef HAS_TERMINATE /* { */
/************************/
      case RTXC_TERMINATE:
/************************/
	 if ( (task = ((struct targ *)p2)->task) == SELFTASK)
	 {
	    ptcb = hipritsk;
	    task = hipritsk->task;
	 }
	 else
	    ptcb = &rtxtcb[(int)task];

	 /* if task has a timeout timer */
	 if ( (pclkblk = ptcb->pclkblk) != NULLCLK)
	 {
	    /* if timeout is still active */
	    if (pclkblk->state == TIMER_ACTIVE)
	    {
	       unlink_timer(pclkblk);
	    }
	    ptcb->pclkblk = NULLCLK;
	 }

	 if ( (ptcb->status == READY)

#if defined(MAILBOX_WAITERS)   || \
    defined(PARTITION_WAITERS) || \
    defined(RESOURCE_WAITERS)  || \
    defined(QUEUE_WAITERS) /* { */
	   || (ptcb->status & COMBO_WAIT)
#endif /* } - MAILBOX_ || PARTITION_ || QUEUE_ || RESOURCE_WAITERS */

				      )
	 {
	    /* unlink task from a WAITER or the READY list (all cases have same format) */
	    if ((ptcb->blink->flink = ptcb->flink) != NULLTCB)
	       ptcb->flink->blink = ptcb->blink;
	 }

#ifdef CBUG /* { */
   /* remove all COMBO_WAITs since we already cleaned them up */

#if defined(MAILBOX_WAITERS)   || \
    defined(PARTITION_WAITERS) || \
    defined(RESOURCE_WAITERS)  || \
    defined(QUEUE_WAITERS) /* { */
	 ptcb->status &= ~COMBO_WAIT;
#endif /* } - MAILBOX_ || PARTITION_ || QUEUE_ || RESOURCE_WAITERS */

	 /* add INACTIVE (not set INA) so other blocking bits */
	     /* will show under RTXCbug while debugging */
	 ptcb->status |= INACTIVE;
#else /* } CBUG { */
	 /* set task INACTIVE, awaiting next KS_execute() */
	 ptcb->status = INACTIVE;
#endif /* } CBUG */

	 if (task <= ntasks)
	    ptcb->priority = rtxkktcb[(int)task].priority; /* reset priority */
	    //memcpy(&(ptcb->priority), &(rtxkktcb[(int)task].priority), sizeof(ptcb->priority)) ; /* reset priority */
#ifdef DYNAMIC_TASKS /* { */
	 else
	 {
	    ptcb->priority = NULLTASK_PRIORITY - 1; /* reset priority */

	    /* re-insert tcb into tcb free list for dynamic tasks */
	    ptcb->flink = dtcbfl;
	    dtcbfl = ptcb;
	 }
#endif /* } DYNAMIC_TASKS */

	 break;
#endif /* } HAS_TERMINATE */

#ifdef HAS_YIELD /* { */
/************************/
      case RTXC_YIELD:
/************************/
	 /* yield is NOP unless next READY task at same priority */
	 if (hipritsk->flink->priority != hipritsk->priority)
	    break;

	 ((struct targ *)p2)->ksrc = RC_GOOD;

	 ptcb = hipritsk; /* save */

#ifdef TIME_SLICE /* { */
	 ptcb->tslice = ptcb->newslice; /* reset time allotment */
#endif /* } TIME_SLICE */

	 UNLINK_HIPRITSK(); /* unlink first tcb */

	 /* re-insert task */
	 ptcb->flink = nsrttcb;
	 nsrttcb = ptcb;
	 scheduler = 0;

	 break;
#endif /* } HAS_YIELD */

#ifdef HAS_DEFPRIORITY /* { */
/************************/
      case RTXC_DEFPRIORITY:
/************************/
	 /* limit priority to reasonable value, else real low priority */
	 if ( (((struct targ *)p2)->priority < 1) ||
	      (((struct targ *)p2)->priority >= NULLTASK_PRIORITY) )
	      ((struct targ *)p2)->priority = NULLTASK_PRIORITY - 1;

	 if ( (task = ((struct targ *)p2)->task) == SELFTASK)
	    ptcb = hipritsk;
	 else
	    ptcb = &rtxtcb[(int)task];

	 chgpriority(ptcb, ((struct targ *)p2)->priority);
	 break;
#endif /* } HAS_DEFPRIORITY */

#ifdef HAS_USER /* { */
/************************/
      case RTXC_USER:
/************************/
	 /* call function passing &arg packet */
	 ((struct userarg *)p2)->val =
	    (*((struct userarg *)p2)->fun)
	    (((struct userarg *)p2)->arg);
	 break;
#endif /* } HAS_USER */

#ifdef HAS_NOP /* { */
/************************/
      case RTXC_NOP:
/************************/
	 break;
#endif /* } HAS_NOP */

/************************/
      default:
/************************/
	 break;
   }

   DISABLE;

   /* check to see if system had any "activity" during kernel service */
   if (scheduler)
   {
#ifdef FPU /* { */
     if ( (hipritsk->fpumode == 1) && (fputask != hipritsk) )
     {
	/* swap fpu regs */
	fpuswap(fputask->fpregs, hipritsk->fpregs);
	fputask = hipritsk; /* update new user of fpu */
     }
#endif /* } FPU */

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
     ENABLE ;   // enable flag is not inside CCR
#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
     return(hipritsk->sp); /* exit to hipritsk via tcb.sp */
   }

   ENABLE;
   return(postem());
}

#ifdef HAS_MAILBOXES /* { */
/* thread message into mailbox chain in priority as specified by sender */
/************************/
static void insert_message(MHEADER *pmh, RTXCMSG *prtxcmsg)
/************************/
{
   RTXCMSG *prev, *next;
   PRIORITY priority;

   /* see if the degenerate case exists, i.e., the mailbox is empty */
   if ( (next = pmh->link) == NULL)
      pmh->link = prtxcmsg;
   else /* mailbox is not empty, search for insertion point */
   {
      priority = prtxcmsg->priority;

      prev = (RTXCMSG *)pmh;

      while( ((next = prev->link) != NULL) && (next->priority <= priority) )
	 prev = next;

      prev->link = prtxcmsg;
   }

   prtxcmsg->link = next;
}

/* remove message from task's mailbox */
/************************/
static RTXCMSG *remove_message(MHEADER *pmh, TASK task)
/************************/
{
   RTXCMSG *lptr, *nptr;

   if ( (nptr = pmh->link) == NULL)
      return(NULL);

   if ( (task == (TASK)0) || (nptr->task == task) )
   {
      pmh->link = nptr->link;
      return(nptr);
   }

   while (nptr->link != NULL)
   {
      lptr = nptr;
      nptr = lptr->link;
      if (nptr->task == task)
      {
	 lptr->link = nptr->link;
	 return(nptr);
      }
   }

   return(NULL);
}
#endif /* } HAS_MAILBOXES */

/* clear TCB critical info and initialize task status list */
/************************/
void taskinit(void)
/************************/
{
   TASK i;
   TCB *ptcb;

#ifdef CBUG
#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
// Already done in RTXCstartup_xxx.c
//   extern const unsigned long __kernel_stack_start ;
//   kernellastrunstack = __kernel_stack_start ;
#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
   extern char ram_end[] ;                 // end of RAM
   // Stack info from RTXCstartup.S
   extern const unsigned long Global_IRQ_STACK_SIZE ;
   extern const unsigned long Global_SVC_STACK_SIZE ;
   extern const unsigned long Global_SYSKERNEL_STACK_SIZE ;

   kernellastrunstack = *((unsigned long *)( ((unsigned long)(ram_end))
                                                - Global_IRQ_STACK_SIZE
                                                - Global_SVC_STACK_SIZE
                                                - Global_SYSKERNEL_STACK_SIZE)) ;
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#endif // CBUG

   scheduler = 1;

#ifdef FPU /* { */
   fputask =
#endif /* } FPU */
   hipritsk = &rtxtcb[0];

   hipritsk->blink = (TCB *)&hipritsk;

   hipritsk->priority = NULLTASK_PRIORITY;

#ifdef BSS_NOT_ZERO /* { */
   hipritsk->task = 0;
   hipritsk->flink = NULLTCB;
   hipritsk->status = READY; /* mark null task READY */

#ifdef FPU /* { */
   hipritsk->fpregs = NULLFPREGS;
   hipritsk->fpumode = 0;
#endif /* } FPU */

   nsrttcb = NULLTCB;
#endif /* } BSS_NOT_ZERO */

   for (i = 1, ptcb = &rtxtcb[1]; i <= ntasks; i++, ptcb++)
   {
      ptcb->status = INACTIVE; /* mark each task inactive */
      ptcb->task = i; /* set each task no. */

#ifdef BSS_NOT_ZERO /* { */
      ptcb->pclkblk = NULLCLK;
#endif /* } BSS_NOT_ZERO */

      ptcb->priority = rtxkktcb[(int)i].priority; /* initial priority */
      //memcpy(&(ptcb->priority), &(rtxkktcb[(int)i].priority), sizeof(ptcb->priority)) ; /* initial priority */

#ifdef FPU /* { */
      ptcb->fpregs = rtxktcb[i].fpregs;
      //memcpy(&(ptcb->fpregs), &(rtxkktcb[(int)i].fpregs), sizeof(ptcb->fpregs)) ; /* initial priority */
#endif /* } FPU */

#ifdef TIME_SLICE /* { */
#ifdef BSS_NOT_ZERO /* { */
      ptcb->tslice = 0;
      ptcb->newslice = 0;
#endif /* } BSS_NOT_ZERO */
#endif /* } TIME_SLICE */

      /* copy stackbase, stacksize and entry point from KTCB to TCB */
      //ptcb->stackbase = rtxkktcb[(int)i].stackbase;
      //memcpy(&(ptcb->stackbase), &(rtxkktcb[(int)i].stackbase), sizeof(ptcb->stackbase)) ;

      //ptcb->stacksize = rtxkktcb[(int)i].stacksize;
      //memcpy(&(ptcb->stacksize), &(rtxkktcb[(int)i].stacksize), sizeof(ptcb->stacksize)) ;

//#ifdef CBUG /* { */ // _FR_ 13/02/09
      ptcb->lastrunstack = *((unsigned long *)(rtxkktcb[(int)i].stackbase)) ;
      //stkinit((unsigned long *)ptcb->stackbase, ptcb->stacksize);
      stkinit(i) ;
//#endif /* } CBUG */ // _FR_ 13/02/09

      //ptcb->userpc_t0 = (rtxkktcb[(int)i].userpc_t0) ;
      //memcpy(&(ptcb->swpc_t0), &(rtxkktcb[(int)i].swpc_t0), sizeof(ptcb->swpc_t0)) ;

#ifdef HAS_INQTASK_ARG /* { */
#ifdef BSS_NOT_ZERO /* { */
      ptcb->arg = NULL;
#endif /* } BSS_NOT_ZERO */
#endif /* } HAS_INQTASK_ARG */
   }

#ifdef DYNAMIC_TASKS /* { */
   /* thread dynamic task TCB free pool - possibly dntasks == 0 */
   if (dntasks)
      dtcbfl = &rtxtcb[ntasks + 1];
   else
      dtcbfl = NULLTCB;

   for (i = 1, ptcb = dtcbfl; i <= dntasks; i++, ptcb++)
   {
      ptcb->flink = ptcb + 1; /* insert tcb into linked list */

      ptcb->status = INACTIVE; /* mark each task inactive */
      ptcb->task = ntasks + i; /* set each task no. */

#ifdef BSS_NOT_ZERO /* { */
      ptcb->pclkblk = NULLCLK;
#endif /* } BSS_NOT_ZERO */

      ptcb->priority = NULLTASK_PRIORITY - 1; /* initial priority (low) */

#ifdef FPU /* { */
#ifdef BSS_NOT_ZERO /* { */
      ptcb->fpregs = NULLFPREGS;
#endif /* } BSS_NOT_ZERO */
#endif /* } FPU */

#ifdef TIME_SLICE /* { */
#ifdef BSS_NOT_ZERO /* { */
      ptcb->tslice = 0;
      ptcb->newslice = 0;
#endif /* } BSS_NOT_ZERO */
#endif /* } TIME_SLICE */

#ifdef HAS_INQTASK_ARG /* { */
#ifdef BSS_NOT_ZERO /* { */
      ptcb->arg = NULL;
#endif /* } BSS_NOT_ZERO */
#endif /* } HAS_INQTASK_ARG */
   }

   if (dntasks)
   {
      --ptcb;
      ptcb->flink = NULLTCB; /* null last link */
   }
#endif /* } DYNAMIC_TASKS */

#ifdef CBUG /* { */
    // already done by RTXCstartup.S
    //stkinit((unsigned long *)&rtxctos[0], RTXCSTKSZ); /* init kernel stack */
#endif /* } CBUG */

#ifdef BSS_NOT_ZERO /* { */
   isrcnt = 0;
#ifdef CBUG /* { */
   isrmax = 0;
#endif /* } CBUG */
#endif /* } BSS_NOT_ZERO */

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
   isrcnt = 1 ; // not used +++++++++++++++++++TODO++++++++++++++++++++
#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
}

#ifdef HAS_RESOURCES /* { */
/* startup - initialize resource structures */
/************************/
void resinit(void)
/************************/
{
   RESOURCE i;
   RHEADER *prh;

   for (i = 1, prh = &rheader[1]; i <= nres; i++, prh++)
   {
#ifdef BSS_NOT_ZERO /* { */
      prh->owner = NULLTCB;

#ifdef RESOURCE_WAITERS /* { */
      prh->waiters = NULLTCB;
      prh->dummy = NULLTCB;
#ifdef PRIORITY_INVERSION /* { */
      prh->priority = 0;
#endif /* } PRIORITY_INVERSION */
#endif /* } RESOURCE_WAITERS */

      prh->level = 0;

#ifdef CBUG /* { */
      prh->count = 0;
      prh->conflict = 0;
#endif /* } CBUG */
#endif /* } BSS_NOT_ZERO */

#ifdef PRIORITY_INVERSION /* { */
      prh->resattr = PRIORITY_INVERSION_T0;
#endif /* } PRIORITY_INVERSION */
   }
}
#endif /* } HAS_RESOURCES */

#ifdef HAS_MAILBOXES /* { */
/* startup - initialize mailbox structures */
/************************/
void mboxinit(void)
/************************/
{
#ifdef BSS_NOT_ZERO /* { */
   MBOX i;
   MHEADER *pmh;

   for (i = 1, pmh = &mheader[1]; i <= nmboxes; i++, pmh++)
   {
      pmh->link = NULL;

#ifdef MAILBOX_WAITERS /* { */
      pmh->waiters = NULLTCB;
      pmh->dummy = NULLTCB;
#endif /* } MAILBOX_WAITERS */

#ifdef MAILBOX_SEMAS /* { */
      pmh->nesema = 0;
#endif /* } MAILBOX_SEMAS */

#ifdef CBUG /* { */
      pmh->count = 0;
#endif /* } CBUG */
   }
#endif /* } BSS_NOT_ZERO */
}
#endif /* } HAS_MAILBOXES */

#ifdef HAS_PARTITIONS /* { */
/* startup partition linking logic */
/************************/
void partinit(void)
/************************/
{
#ifdef DYNAMIC_PARTS
   MAP k ;
#endif // DYNAMIC_PARTS
   MAP i ;
   size_t j;
   char **next;
   PHEADER *pph;
   const PKHEADER *ppkh;
   //int cnt ;
   //size_t siz ;
   
   for (i = 1, ppkh = &pkkheader[1], pph = &pheader[1]; i <= nparts;
	i++,   ppkh++,              pph++)
   {
      pph->next = ppkh->next;
      //memcpy_P(&(pph->next), &(pkkheader[(int)(i)].next), sizeof(pph->next)) ;
      //memcpy_P(&cnt, &(pkkheader[(int)(i)].count), sizeof(cnt)) ;
      //memcpy_P(&siz, &(pkkheader[(int)(i)].size), sizeof(siz)) ;

      if ( (next = (char **)pph->next) != NULL)
      {
	 for (j = 1; j < /*cnt*/ ppkh->count ; j++, next = (char **)*next)
	    *next = (char *)next + /*siz*/ ppkh->size ;

#ifdef BSS_NOT_ZERO /* { */
	*next = NULL; /* null last link */
#endif /* } BSS_NOT_ZERO */
      }

      pph->size = /*siz*/ ppkh->size ;
      pph->count = /*cnt*/ ppkh->count ;

#ifdef BSS_NOT_ZERO /* { */
#ifdef PARTITION_WAITERS /* { */
      pph->waiters = NULLTCB;
      pph->dummy = NULLTCB;
#endif /* } PARTITION_WAITERS */

#ifdef CBUG /* { */
      pph->cur = 0;
      pph->worst = 0;
      pph->usage = 0;
#endif /* } CBUG */
#endif /* } BSS_NOT_ZERO */
   }

#ifdef DYNAMIC_PARTS /* { */
   /* thread dynamic partition PHEADER free pool - possibly dnparts == 0 */
   if (dnparts)
      dphfl = &pheader[nparts + 1];
   else
      dphfl = NULLMAP;

   for (i = 1, k = nparts + 1, pph = dphfl; i < dnparts; i++, k++, pph++)
   {
      /* insert pheader into linked list */
      pph->next = (struct xmap *)&pheader[k + 1];
      pph->map = k;	   /* set map number for return to definer */

#ifdef BSS_NOT_ZERO /* { */
#ifdef PARTITION_WAITERS /* { */
      pph->waiters = NULLTCB;
      pph->dummy = NULLTCB;
#endif /* } PARTITION_WAITERS */

#ifdef CBUG /* { */
      pph->cur = 0;
      pph->worst = 0;
      pph->usage = 0;
#endif /* } CBUG */
#endif /* } BSS_NOT_ZERO */
   }

   if (dnparts)
   {
      pph->next = NULL; /* null last link */
      pph->map = k;

#ifdef BSS_NOT_ZERO /* { */
#ifdef PARTITION_WAITERS /* { */
      pph->waiters = NULLTCB;
      pph->dummy = NULLTCB;
#endif /* } PARTITION_WAITERS */

#ifdef CBUG /* { */
      pph->cur = 0;
      pph->worst = 0;
      pph->usage = 0;
#endif /* } CBUG */
#endif /* } BSS_NOT_ZERO */
   }
#endif /* } DYNAMIC_PARTS */

}
#endif /* } HAS_PARTITIONS */

#ifdef HAS_QUEUES /* { */
/* queue startup support */
/************************/
void queueinit(void)
/************************/
{
   QUEUE i;
   QHEADER *pqh;
   const QKHEADER *pqkh;

   for (i = 1, pqkh = &qkkheader[1], pqh = &qheader[1]; i <= nqueues;
	i++,   pqkh++,              pqh++)
   {
      pqh->base = pqkh->base;
      pqh->width = pqkh->width;
      pqh->depth = pqkh->depth;
      //memcpy_P(&(pqh->base), &(qkkheader[(int)(i)].base), sizeof(pqh->base)) ;
      //memcpy_P(&(pqh->width), &(qkkheader[(int)(i)].width), sizeof(pqh->width)) ;
      //memcpy_P(&(pqh->depth), &(qkkheader[(int)(i)].depth), sizeof(pqh->depth)) ;

      pqh->putndx = pqh->depth - 1;
#ifdef BSS_NOT_ZERO /* { */
      pqh->curndx = 0;

#ifdef QUEUE_SEMAS /* { */
      pqh->nesema = 0;
      pqh->fsema = 0;
      pqh->nfsema = 0;
      pqh->esema = 0;
#endif /* } QUEUE_SEMAS */

#ifdef QUEUE_WAITERS /* { */
      pqh->waiters = NULLTCB;
      pqh->dummy = NULLTCB;
#endif /* } QUEUE_WAITERS */

#ifdef CBUG /* { */
      pqh->count = 0;
      pqh->worst = 0;
#endif /* } CBUG */
#endif /* } BSS_NOT_ZERO */
   }
}
#endif /* } HAS_QUEUES */

/* startup all semaphores as PENDing */
/************************/
void semainit(void)
/************************/
{
   SEMA i;
   SSTATE *s;
#ifdef BSS_NOT_ZERO /* { */
   unsigned int j;
#endif /* } BSS_NOT_ZERO */

   for (i = 1, s = &semat[1]; i <= nsemas; i++, s++)
      *s = SEMA_PENDING;

   semaput = semaget = &siglist[0]; /* init empty sema deque */

#ifdef BSS_NOT_ZERO /* { */
   for (j = 0; j < siglistsize; j++)
      siglist[j] = 0;
#endif /* } BSS_NOT_ZERO */
}

/* link all clock blocks into free list */
/************************/
void clockinit(void)
/************************/
{
#ifdef HAS_ALLOC_TIMER /* { */
   unsigned int i;
   CLKBLK *link;

   /* note, RTXCgen guarantees at least 1 timer */

   link = clkqfl = &clkq[0];

   for (i = 1; i < ntmrs; i++, link++)
      link->flink = link + 1;
#endif /* } HAS_ALLOC_TIMER */

#ifdef BSS_NOT_ZERO /* { */
   link->flink = NULLCLK;

   clkqptr = NULLCLK;

   rtctick = (TICKS)0;
   t_expired = 0;

#ifdef HAS_INQTIME /* { */
   rtctime = (time_t)0;
   ratecnt = 0;
#endif /* } HAS_INQTIME */

#ifdef TIME_SLICE /* { */
   sliceup = NULLTCB;
#endif /* } TIME_SLICE */

#endif /* } BSS_NOT_ZERO */
}

#ifdef HAS_PARTITIONS /* { */
#ifdef HAS_ALLOC /* { */
void * KS_ISRalloc(MAP map)
{
   unsigned long xccr ;
   PHEADER *pph;
   struct xmap *q;

   pph = &pheader[(int)map];

   xccr = PUSHCCR; /*  in case called from isr */
   DISABLE;	   /* can also be called from RTXC_ALLOC code */

   if ( (q = pph->next) != NULL) /* if any avail, return 1st */
   {
      pph->next = q->link; /* unlink 1st one from list */
#ifdef CBUG /* { */
      if (++pph->cur > pph->worst) /* check for new worst case */
	 pph->worst = pph->cur; 	 /* usage level */
#endif /* } CBUG */
   }

   POPCCR(xccr);

   return(q);
}
#endif /* } HAS_ALLOC */
#endif /* } HAS_PARTITIONS */

void KS_ISRsignal(SEMA sema)
{
   unsigned long xccr;

   xccr = PUSHCCR;
   DISABLE;
   *semaput++ = sema;
   scheduler = 0;
   POPCCR(xccr);
}

/* rtxc standard interrupt exit logic */
/************************/
FRAME * KS_ISRexit(FRAME *frame, SEMA sema)
/************************/
{
#ifdef CBUG /* { */
    if (isrcnt > isrmax)        // audit worst case interrupt depth
        isrmax = isrcnt ;
#endif /* } CBUG */

    DISABLE ;   // turn off interrupts briefly while checking nest level

    if (sema != NULLSEMA) {     // if non-zero sema passed
        *semaput++ = sema;      // put sema in post list
        scheduler = 0;
    }

    if (isrcnt == 1) {          // interrupted a task, not RTXC or another isr
        if (scheduler) {
#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
            ENABLE ;            // enable flag is not inside CCR
#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
            return(frame) ;     // return to rtxc or isr
        }

        hipritsk->sp = frame ;

        ENABLE ;
        /* return through postem() */
        return(postem());

    } else {
        /* return to isr, postem() will be performed later */
#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
        ENABLE ;        // enable flag is not inside CCR
#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
        return(frame);  // return to rtxc or isr
    }
}

/************************/
FRAME * API_PAGING postem(void) /* returns with interrupts disabled */
/************************/
{
   TICKS tcnt, xx;
   SEMA sema;
   TASK task;
   SSTATE *sema_ptr;
   TCB *ptcb, *prev;
   KSNUM *p2a;
   CLKBLK *pclkblk;
#ifdef HAS_WAITM /* { */
   SEMA *list;
#endif /* } HAS_WAITM */
#ifdef RESOURCE_TIMEOUTS /* { */
   KSNUM *p2;
#ifdef PRIORITY_INVERSION /* { */
    RESOURCE resource;
    RHEADER *prh;
    PRIORITY priority;
#endif /* } PRIORITY_INVERSION */
#endif /* } RESOURCE_TIMEOUTS */

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef USE_LEDDEBUG
// configure the PIO Lines corresponding to LED1 to LED4 to be outputs
// no need to set these pins to be driven by the PIO because it is GPIO pins only.
AT91C_BASE_PIOA->PIO_PER = LED_MASK ; // Set in PIO mode
AT91C_BASE_PIOA->PIO_OER = LED_MASK ; // Configure in Output

// set at 1 (turn led off)
AT91C_BASE_PIOA->PIO_SODR = LED_MASK ;

// set at 0 (turn led on)
AT91C_BASE_PIOA->PIO_CODR = LED3 ;

// let led to blink
if (AT91C_BASE_PIOA->PIO_PDSR & LED2) {
    // set at 0 (turn led on)
    AT91C_BASE_PIOA->PIO_CODR = LED2 ;
} else {
    // set at 1 (turn led off)
    AT91C_BASE_PIOA->PIO_SODR = LED2 ;
}
#endif // USE_LEDDEBUG
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   DISABLE;
   for (;;)
   {
      scheduler = 1;

      if (t_expired) /* if timer expired with last clock interrupt ... */
      {
	 t_expired = 0; /* clear expired timer switch */
	 tcnt = rtctick; /* local copy of rtctick */
	 ENABLE;

/*
 * xx is used in the following statement because some compilers can't
 * properly handle the arithmetic when negative values are involved
 */
	 while ( (clkqptr != NULLCLK) && ((xx = tcnt-clkqptr->remain) >= 0) )
	 {
	    pclkblk = clkqptr; /* save address of expired timer */
	    xx++;	       /* bumped here to eliminate compiler warning */

	    DISABLE;
	    /* unlink expired timer with interrupts disabled */
	    if ( (clkqptr = clkqptr->flink) != NULLCLK)
		clkqptr->blink = (CLKBLK *)&clkqptr;
	    ENABLE;

	    pclkblk->state = TIMER_DONE;

	    switch(pclkblk->objtype)
	    {
	       case TIMER_OBJ:
		  SIGNAL(pclkblk->objid); /* put semaphore in signal list */

		  if (pclkblk->recycle) 	/* if timer is cyclic */
		  {
		     pclkblk->remain = pclkblk->recycle; /* reset counts */

		     /* insert clk timer */
		     insert_timer(pclkblk);
		  }
		  break;

	       case DELAY_OBJ:
		  ptcb = &rtxtcb[(int)(pclkblk->task)];

		  ptcb->pclkblk = NULLCLK;

		  if ( (ptcb->status &= ~DELAY_WAIT) == READY)
		  {
		     /* insert task into insert list */
		     ptcb->flink = nsrttcb;
		     nsrttcb = ptcb;
		  }
		  break;

#ifdef SEMAPHORE_TIMEOUTS /* { */
	       case SEMAPHORE_OBJ: /* KS_waitt() and KS_sendt() */
		  semat[(int)(pclkblk->objid)] = SEMA_PENDING;

		  ptcb = &rtxtcb[(int)(pclkblk->task)];

		  ptcb->pclkblk = NULLCLK;

		  if ( (ptcb->status &= ~SEMAPHORE_WAIT) == READY)
		  {
		     /* insert task into insert list */
		     ptcb->flink = nsrttcb;
		     nsrttcb = ptcb;
		  }
		  break;
#endif /* } SEMAPHORE_TIMEOUTS */

#ifdef QUEUE_TIMEOUTS /* { */
	       case QUEUE_OBJ:	   /* KS_enqueuet() and KS_dequeuet() */
		  /* fall into common code */
#endif /* } QUEUE_TIMEOUTS */

#ifdef PARTITION_TIMEOUTS /* { */
	       case PARTITION_OBJ: /* KS_alloct() */
		  /* fall into common code */
#endif /* } PARTITION_TIMEOUTS */

#ifdef RESOURCE_TIMEOUTS /* { */
	       case RESOURCE_OBJ:  /* KS_lockt() */
		  /* fall into common code */
#endif /* } RESOURCE_TIMEOUTS */

#ifdef MAILBOX_TIMEOUTS /* { */
	       case MAILBOX_OBJ:   /* KS_receivet() */
		  /* fall into common code */
#endif /* } MAILBOX_TIMEOUTS */

#if defined(MAILBOX_TIMEOUTS)	|| \
    defined(PARTITION_TIMEOUTS) || \
    defined(QUEUE_TIMEOUTS)	|| \
    defined(RESOURCE_TIMEOUTS) /* { */
		  /* common code */
		  ptcb = &rtxtcb[(int)(pclkblk->task)];

#ifdef RESOURCE_TIMEOUTS /* { */
		  if (pclkblk->objtype == RESOURCE_OBJ)
		  {
		     /* mark KS_lockt() failure */
		     p2 = ((FRAME *)(ptcb->sp))->pksnum;
		     ((struct larg *)p2)->ksrc = RC_TIMEOUT;

#ifdef PRIORITY_INVERSION /* { */
		     resource = ((struct larg *)p2)->resource;
		     prh = &rheader[(int)resource];

		     /* if task that owns resource is priority inverted */
		     if (prh->priority != (PRIORITY)0)
		     {
			/*
			 * then determine if the task was inverted because
			 * of the lockt() that just timed-out. If so, then
			 * determine what priority to change to.  The new
			 * priority will be either the original or that of
			 * the next task waiting on the resource (if any).
			*/

			if (prh->waiters == ptcb)
			{
			   if (prh->waiters->flink != NULLTCB)
			   {
			      priority = prh->waiters->flink->priority;
			      if (priority > prh->priority)
				 priority = prh->priority;
			   }
			   else
			      priority = prh->priority;

			   chgpriority(prh->owner, priority);
			}
		     }
#endif /* } PRIORITY_INVERSION */
		  }
#endif /* } RESOURCE_TIMEOUTS */

		  ptcb->pclkblk = NULLCLK;

		  /* remove tcb from 2-way waiter list */
		  if ( (ptcb->blink->flink = ptcb->flink) != NULLTCB)
		     ptcb->flink->blink = ptcb->blink;

		  if ( (ptcb->status &= ~COMBO_WAIT) == READY)
		  {
		     /* insert task into insert list */
		     ptcb->flink = nsrttcb;
		     nsrttcb = ptcb;
		  }
		  break;
#endif /* } - MAILBOX_ || PARTITION_ || QUEUE_ || RESOURCE_TIMEOUTS */

	       default: /* serious trouble here - undefined type */
		  break;
	    }
	 }

#ifdef TIME_SLICE /* { */
	 if (sliceup != NULLTCB)
	 {
	    if (sliceup->status == READY)
	    {
	       if (sliceup->priority == sliceup->flink->priority)
	       {
		  /* yield sliceup to next task in list */

		  /* unlink tcb */
		  sliceup->flink->blink = sliceup->blink;
		  sliceup->blink->flink = sliceup->flink;

		  /* re-insert task */
		  sliceup->flink = nsrttcb;
		  nsrttcb = sliceup;
	       }
	    }
	    sliceup = NULLTCB;
	 }
#endif /* } TIME_SLICE */

	 DISABLE;
      }

      /* check to see if any semaphores to process */
      while (semaput != semaget)
      {
	 ENABLE;

	 sema = *semaget++;	/* get sema from siglist ... */
	 sema_ptr = &semat[(int)sema];

	 if ( (*sema_ptr == SEMA_PENDING) || (*sema_ptr == SEMA_DONE) )
	    *sema_ptr = SEMA_DONE;
	 else /* sema was found in wait state */
	 {
	    task = *sema_ptr; /* extract waiting task no. */

	    *sema_ptr = SEMA_PENDING; /* reset semaphore pending */

	    ptcb = &rtxtcb[(int)task];

	    p2a = ((FRAME *)(ptcb->sp))->pksnum;

#ifdef SEMAPHORE_TIMEOUTS /* { */
	    /* if timer associated with semaphore */
	    if ( ((pclkblk = ptcb->pclkblk) != NULLCLK) &&
		  (pclkblk->objtype == SEMAPHORE_OBJ) &&
		  (pclkblk->objid == sema) )
	    {
	       ((struct sarg *)p2a)->ksrc = RC_GOOD;

	       unlink_timer(pclkblk);
	       ptcb->pclkblk = NULLCLK;
	    }
#endif /* } SEMAPHORE_TIMEOUTS */

#ifdef HAS_WAITM /* { */
	    if ( ((struct sarg *)p2a)->ksnum == RTXC_WAITM)
	    {
	       /* pass sema back via task stack arg packet */
	       ((struct sarg *)p2a)->sema = sema;

	       for (list = ((struct sargm *)p2a)->list;
		   *list != NULLSEMA; list++)
	       {
		  sema_ptr = &semat[(int)(*list)]; /* get ptr to sema */
		  if ( (*sema_ptr != SEMA_PENDING) &&
		       (*sema_ptr != SEMA_DONE) )
		     *sema_ptr = SEMA_PENDING;
	       }
	    }
#endif /* } HAS_WAITM */

	    if ( (ptcb->status &= ~SEMAPHORE_WAIT) == READY)
	    {
	       ptcb->flink = nsrttcb;
	       nsrttcb = ptcb;
	    }
	 }

	 DISABLE;

      }
      /* loop exited with interrupts disabled, all semaphores processed */

      semaput = semaget = &siglist[0]; /* reset sema signal list empty */

      ENABLE;

      /* process task list now with interrupts on */
      while ( (ptcb = nsrttcb) != NULLTCB)
      {
	 /* unlink first tcb in thread and insert in READY list */
	 nsrttcb = ptcb->flink;

	 /* insert ptcb in priority order in READY list */
	 prev = (TCB *)&hipritsk;

	 while(ptcb->priority >= prev->flink->priority)
	    prev = prev->flink;

	 ptcb->flink = prev->flink;
	 ptcb->blink = prev;
	 prev->flink->blink = ptcb;
	 prev->flink = ptcb;
      }

      DISABLE;

      /* check to see if any semaphores were posted while processing tasks */
      if (scheduler)
	 break;    /* if not, then all done - exit as fast as possible */
   }

#ifdef FPU /* { */
   if ( (hipritsk->fpumode == 1) && (fputask != hipritsk) )
   {
      /* swap fpu regs */
      fpuswap(fputask->fpregs, hipritsk->fpregs);
      fputask = hipritsk; /* update new user of fpu */
   }
#endif /* } FPU */

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef USE_REGISTERDEBUG
DumpReg("Postem out", hipritsk->sp) ;
#endif // USE_REGISTERDEBUG

#ifdef USE_LEDDEBUG
//if (hipritsk->sp->userpc < 0x9000) {
  // set at 1 (turn led off) +++++++++++++++++++++++++++++++++++++++++++++
// AT91C_BASE_PIOA->PIO_SODR = 0x0100000 ;
//}
// let led to blink
if (AT91C_BASE_PIOA->PIO_PDSR & LED1) {
    // set at 0 (turn led on)
    AT91C_BASE_PIOA->PIO_CODR = LED1 ;
} else {
    // set at 1 (turn led off)
    AT91C_BASE_PIOA->PIO_SODR = LED1 ;
}
#endif // USE_LEDDEBUG
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#if defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
   ENABLE;      // enable flag is not inside CCR
#endif // defined(USE_LPC17XX) || defined(USE_AT91SAM3S4)
   return(hipritsk->sp); /* exit to hipritsk via tcb.sp */
}

#ifdef HAS_ALLOC_TIMER /* { */
/************************/
static CLKBLK *get_clkblk(void)
/************************/
{
   CLKBLK *pclkblk;

   if ( (pclkblk = clkqfl) != NULLCLK) /* allocate timer block */
   {
      clkqfl = pclkblk->flink; /* by removing 1st from free list */
      pclkblk->state = TIMER_DONE;
   }
   return(pclkblk);
}
#endif /* } HAS_ALLOC_TIMER */

/*
 * routine to "process" a tick from clock isr
 *
 * sets t_expired and returns 1(true) if timer expires or time slice has
 * occurred
 *
*/
int KS_ISRtick(void)
{
   TICKS xx;

   rtctick++; /* update tick counter */

#ifdef HAS_INQTIME /* { */
   if (++ratecnt >= clkrate)
   {
      rtctime++ ;               // update second counter
      ratecnt = 0 ;             // reset rate counter (0 - CLKRATE-1)
   }
#endif /* } HAS_INQTIME */

#ifdef TIME_SLICE /* { */
   if (hipritsk->newslice) /* if time slicing enabled for running task */
   {
      if (--hipritsk->tslice == (TICKS)0) /* if time slice is up */
      {
	 sliceup = hipritsk; /* save tcb of task that timed out */
	 sliceup->tslice = sliceup->newslice; /* reset next slice amount */
      }
   }
#endif /* } TIME_SLICE */

/*
 * xx is used in the following statement because some compilers can't
 * properly handle the arithmetic when negative values are involved
 */
   if ( (clkqptr != NULLCLK) && ((xx = rtctick - clkqptr->remain) >= 0) )
   {
      xx++;		 /* bumped here to eliminate compiler warning */
      t_expired = 1;
      scheduler = 0;
      return(1);
   }

#ifdef TIME_SLICE /* { */
   if (sliceup != (TCB *)0)
   {
      t_expired = 1;
      scheduler = 0;
      return(1);
   }
#endif /* } TIME_SLICE */

   return(0);
}

/*
 * initialize task stacks
 *
 * called by taskinit() and KS_deftask()
 *
*/
//#ifdef CBUG /* { */ // _FR_ 13/02/09
/************************/
static void stkinit(int tsk)
/************************/
{
   extern const unsigned long Global_STACK_FILLER ;
   unsigned long *sptr;
   int count, j;

   count = rtxkktcb[tsk].stacksize / sizeof(unsigned long) ;
   sptr = (unsigned long *)(rtxkktcb[tsk].stackbase) ;

   for (j = 0; j < count; j++)
     *sptr++ = Global_STACK_FILLER;
}
//#endif /* } CBUG */ // _FR_ 13/02/09

/*
 * insert timer into timer list
 *
 * called by rtxc() and by postem()
*/
/************************/
static void insert_timer(CLKBLK *pclkblk)
/************************/
{
   TICKS xx;
   CLKBLK *lclkptr, *nclkptr;

   pclkblk->state = TIMER_ACTIVE;

   lclkptr = (CLKBLK *)&clkqptr;

   /* find point of insertion */
/*
 * xx is used in the following statement because some compilers can't
 * properly handle the arithmetic when negative values are involved
*/
   while ( ((nclkptr = lclkptr->flink) != NULLCLK) &&
		  (pclkblk->remain >= (xx = nclkptr->remain - rtctick)) )
      lclkptr = nclkptr;

   xx++;	      /* bumped here to eliminate compiler warning */

   /* insert timer between lptr and nptr */
   pclkblk->remain += rtctick;
   pclkblk->flink = nclkptr;
   pclkblk->blink = lclkptr;
   lclkptr->flink = pclkblk;
   if (nclkptr != NULLCLK)
      nclkptr->blink = pclkblk;
}

/*
 * unlink timer from timer list
*/
/************************/
static void unlink_timer(CLKBLK *pclkblk)
/************************/
{
   pclkblk->state = TIMER_DONE;  /* mark timer done */

   DISABLE;
   /* simply remove block from list */
   if ( (pclkblk->blink->flink = pclkblk->flink) != NULLCLK)
   {
      pclkblk->flink->blink = pclkblk->blink;
   }
   ENABLE;
}

#if defined(MAILBOX_WAITERS)   || \
    defined(PARTITION_WAITERS) || \
    defined(QUEUE_WAITERS)     || \
    defined(RESOURCE_WAITERS)  /* { */
/*
 * insert tcb in priority order
 *
 * unlink hipritsk from READY list
 * and insert hipritsk TCB in priority list (doubly linked)
 * used by mailboxes, partitions, queues, and resources
*/
/************************/
static void porder(TCB *prev)
/************************/
{
   TCB *next, *save;
   PRIORITY priority;

   save = hipritsk;
   priority = save->priority;

   /* remove task from READY list (it is ALWAYS first in list) */
   UNLINK_HIPRITSK();

   /* search and insert in priority order */
   while( ((next = prev->flink) != NULLTCB) && (next->priority <= priority) )
      prev = next;

   prev->flink = save;
   save->blink = prev;

   if ( (save->flink = next) != NULLTCB)
      next->blink = save;
}

/*
 *
 * reorder TCB in respective WAITER list due to priority change
 *
*/
static void reorder_waiters(TCB *ptcb)
{
   /*
    *
    * The cases get a little nastier here than with simply reordering the
    * READY list since there may be no tasks of lower and/or higher priority
    * in the list.  Each case is handled separately (and optimized for speed).
    *
   */

   /* if task is the first waiter in the list */
   if (ptcb->blink->blink == NULLTCB)
   {
      /* if (first and) also last waiter in list */
      if (ptcb->flink == NULLTCB)
      {
	 /* fast return since no shuffling necessary */
	 return;
      }

      /* if (first and) still higher priority than next waiter */
      if (ptcb->priority <= ptcb->flink->priority)
      {
	 /* fast return since no shuffling necessary */
	 return;
      }

      /* insert by walking forward */
      fwd_insert(ptcb);
      return;
   }

   /* if last waiter in list (and not also first) */
   if (ptcb->flink == NULLTCB)
   {
      /* if (last and) still lower priority than next to last */
      if (ptcb->blink->priority <= ptcb->priority)
      {
	 /* fast return since no shuffling necessary */
	 return;
      }

      /* insert by walking backwards */
      bwd_insert(ptcb);
      return;
   }

   /* else task is somewhere in middle of list */

   /* if relative priorities are unchanged */
   if ( (ptcb->blink->priority <= ptcb->priority) &&
	(ptcb->priority <= ptcb->flink->priority) )
   {
      /* fast return since no shuffling necessary */
      return;
   }

   /* if new priority < left-side */
   if (ptcb->priority < ptcb->blink->priority)
   {
      bwd_insert(ptcb);
   }
   else
   {
      fwd_insert(ptcb);
   }
}

/*
 *
 * remove TCB from respective WAITER list and re-insert in forward direction
 *
*/
static void fwd_insert(TCB *ptcb)
{
   TCB *prev, *next;
   PRIORITY priority;

   priority = ptcb->priority;

   prev = ptcb->flink;

   /* unlink task from respective WAITER list */
   ptcb->flink->blink = ptcb->blink;
   ptcb->blink->flink = ptcb->flink;

   while( ((next = prev->flink) != NULLTCB) && (next->priority <= priority) )
      prev = next;

   prev->flink = ptcb;
   ptcb->blink = prev;

   if ( (ptcb->flink = next) != NULLTCB)
      next->blink = ptcb;
}

/*
 *
 * remove TCB from respective WAITER list and re-insert in backward direction
 *
*/
static void bwd_insert(TCB *ptcb)
{
   TCB *prev, *next;
   PRIORITY priority;

   priority = ptcb->priority;

   prev = ptcb->blink;

   /* unlink task from respective WAITER list */
   if ((ptcb->blink->flink = ptcb->flink) != NULLTCB)
       ptcb->flink->blink = ptcb->blink;

   for (next = prev->blink;
      (next->blink != NULLTCB) && (priority < next->priority);
      prev = next, next = prev->blink)
      ; /* yes - a null loop is intended here */

   prev->blink = ptcb;
   ptcb->flink = prev;

   ptcb->blink = next;
   next->flink = ptcb;
}
#endif /* } - MAILBOX_ || PARTITION_ || QUEUE_ || RESOURCE_WAITERS */

/*
 *
 * change priority of a task
 *
*/
static void chgpriority(TCB *ptcb, PRIORITY priority)
{
   /* set new priority */
   ptcb->priority = priority;

   /* if current task is changing its own priority */
   if (hipritsk->task == ptcb->task)
   {
      /* if relative priorities are unchanged */
      if (ptcb->priority <= ptcb->flink->priority)
      {
	 /* fast return since no shuffling necessary */
	 return;
      }

      /* remove task from READY list (it is ALWAYS first in list) */
      UNLINK_HIPRITSK();

      /* re-insert task */
      ptcb->flink = nsrttcb;
      nsrttcb = ptcb;
      scheduler = 0;

      return;
   }

   /* if task being changed is RUNnable */
   if (ptcb->status == READY)
   {
      /* if relative priorities are unchanged */
      if ( (ptcb->blink->priority <= ptcb->priority) &&
	   (ptcb->priority <= ptcb->flink->priority) )
      {
	 /* fast return since no shuffling necessary */
	 return;
      }

      /* else unlink task from RUN list */
      ptcb->flink->blink = ptcb->blink;
      ptcb->blink->flink = ptcb->flink;

      /* re-insert task */
      ptcb->flink = nsrttcb;
      nsrttcb = ptcb;
      scheduler = 0;

      return;
   }

#if defined(MAILBOX_WAITERS)   || \
    defined(PARTITION_WAITERS) || \
    defined(QUEUE_WAITERS)     || \
    defined(RESOURCE_WAITERS)  /* { */

   /* if task being changed is in a WAITER list */
   if (ptcb->status & COMBO_WAIT)
   {
      reorder_waiters(ptcb);
   }
#endif /* } - MAILBOX_ || PARTITION_ || QUEUE_ || RESOURCE_WAITERS */

   return;
}

#ifdef HAS_PARTITIONS /* { */
#ifdef DYNAMIC_PARTS /* { */
static void dn_defpart(PHEADER *pph, KSNUM *p2)
{
   size_t count, blksize;
   char **next;
   int j;

   pph->addr = ((struct parg *)p2)->addr;
   blksize = ((struct parg *)p2)->size;
   count = ((struct parg *)p2)->nblocks;

   pph->next = (struct xmap *)pph->addr;

   next = (char **)pph->next;
   for (j = 1; j < count; j++, next = (char **)*next)
      *next = (char *)next + blksize;

   *next = NULL; /* null last link */
   pph->size = blksize;
   pph->count = count;

#ifdef PARTITION_WAITERS /* { */
   pph->waiters = NULLTCB;
   pph->dummy = NULLTCB;
#endif /* } PARTITION_WAITERS */

#ifdef CBUG /* { */
   pph->cur = 0;
   pph->worst = 0;
   pph->usage = 0;
#endif /* } CBUG */

   return;
}
#endif /* } DYNAMIC_PARTS */
#endif /* } HAS_PARTITIONS */

/* end of file - rtxc.c */

