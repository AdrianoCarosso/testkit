// rtxcapi.h - RTXC interface

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

#ifndef _RTXCAPI_H
#define _RTXCAPI_H

#define RTXC                    // ex sysopts.h

#include "typedef.h"
#include "rtxstruc.h"

// ---------------------------------------------------
// common data structure definitions, added _BM_
// defined in user "main.c"

// only for similitude with AVR family
#define PROGMEM
#define PSTR(A) (A)

#define NAMMAX 8

extern TCB rtxtcb[];
extern const KTCB rtxkktcb[] PROGMEM ;
extern const TASK ntasks ;
#ifdef DYNAMIC_TASKS /* { */
extern const TASK dntasks ;
#endif /* } DYNAMIC_TASKS */
extern const char taskkname[][NAMMAX+1] PROGMEM ;
// ---------------------------------------------------

#define API_PAGING              // where APIs are placed, no longer used

/************************/
/****** SEMAPHORES ******/
/************************/
extern KSRC API_PAGING KS_wait(SEMA);

#ifdef HAS_WAITT /* { */
extern KSRC API_PAGING KS_waitt(SEMA, TICKS);
#endif /* } HAS_WAITT */

#ifdef HAS_WAITM /* { */
extern SEMA API_PAGING KS_waitm(const SEMA *semalist);
#endif /* } HAS_WAITM */

extern KSRC API_PAGING KS_signal(SEMA);

#ifdef HAS_SIGNALM /* { */
extern void API_PAGING KS_signalm(SEMA *);
#endif /* } HAS_SIGNALM */

#ifdef HAS_PEND /* { */
extern void API_PAGING KS_pend(SEMA);
#endif /* } HAS_PEND */

#ifdef HAS_PENDM /* { */
extern void API_PAGING KS_pendm(SEMA *);
#endif /* } HAS_PENDM */

#ifdef HAS_INQSEMA /* { */
extern SSTATE API_PAGING KS_inqsema(SEMA);
#endif /* } HAS_INQSEMA */

/***********************/
/****** MAILBOXES ******/
/***********************/
#ifdef HAS_MAILBOXES /* { */

extern void API_PAGING KS_send(MBOX, RTXCMSG *, PRIORITY, SEMA);

#ifdef HAS_SENDW /* { */
extern void API_PAGING KS_sendw(MBOX, RTXCMSG *, PRIORITY, SEMA);
#endif /* } HAS_SENDW */

#ifdef HAS_SENDT /* { */
extern KSRC API_PAGING KS_sendt(MBOX, RTXCMSG *, PRIORITY, SEMA, TICKS);
#endif /* } HAS_SENDT */

extern RTXCMSG * API_PAGING KS_receive(MBOX, TASK);

#ifdef HAS_RECEIVEW /* { */
extern RTXCMSG * API_PAGING KS_receivew(MBOX, TASK);
#endif /* } HAS_RECEIVEW */

#ifdef HAS_RECEIVET /* { */
extern RTXCMSG * API_PAGING KS_receivet(MBOX, TASK, TICKS, KSRC *);
#endif /* } HAS_RECEIVET */

#ifdef HAS_ACK /* { */
extern void API_PAGING KS_ack(RTXCMSG *);
#endif /* } HAS_ACK */

#ifdef HAS_DEFMBOXSEMA /* { */
extern void API_PAGING KS_defmboxsema(MBOX, SEMA);
#endif /* } HAS_DEFMBOXSEMA */

#endif /* } HAS_MAILBOXES */

/************************/
/****** PARTITIONS ******/
/************************/
#ifdef HAS_PARTITIONS /* { */

extern void * API_PAGING KS_alloc(MAP);

#ifdef HAS_ALLOCW /* { */
extern void * API_PAGING KS_allocw(MAP);
#endif /* } HAS_ALLOCW */

#ifdef HAS_ALLOCT /* { */
extern void * API_PAGING KS_alloct(MAP, TICKS, KSRC *);
#endif /* } HAS_ALLOCT */

extern void API_PAGING KS_free(MAP, void *);

#ifdef HAS_INQMAP /* { */
extern size_t API_PAGING KS_inqmap(MAP);
#endif /* } HAS_INQMAP */

#ifdef HAS_CREATE_PART /* { */
extern MAP API_PAGING KS_create_part(void *, size_t, size_t);
#endif /* } HAS_CREATE_PART */

#ifdef HAS_ALLOC_PART /* { */
extern MAP API_PAGING KS_alloc_part(void);
#endif /* } HAS_ALLOC_PART */

#ifdef HAS_DEFPART /* { */
extern void API_PAGING KS_defpart(MAP, void *, size_t, size_t);
#endif /* } HAS_DEFPART */

#ifdef HAS_FREE_PART /* { */
extern void * API_PAGING KS_free_part(MAP);
#endif /* } HAS_FREE_PART */

#endif /* } HAS_PARTITIONS */

/********************/
/****** QUEUES ******/
/********************/
#ifdef HAS_QUEUES /* { */

extern KSRC API_PAGING KS_enqueue(QUEUE, void *);

#ifdef HAS_ENQUEUEW /* { */
extern void API_PAGING KS_enqueuew(QUEUE, void *);
#endif /* } HAS_ENQUEUEW */

#ifdef HAS_ENQUEUET /* { */
extern KSRC API_PAGING KS_enqueuet(QUEUE, void *, TICKS);
#endif /* } HAS_ENQUEUET */

extern KSRC API_PAGING KS_dequeue(QUEUE, void *);

#ifdef HAS_DEQUEUEW /* { */
extern void API_PAGING KS_dequeuew(QUEUE, void *);
#endif /* } HAS_DEQUEUEW */

#ifdef HAS_DEQUEUET /* { */
extern KSRC API_PAGING KS_dequeuet(QUEUE, void *, TICKS);
#endif /* } HAS_DEQUEUET */

#ifdef HAS_DEFQUEUE /* { */
extern KSRC API_PAGING KS_defqueue(QUEUE, size_t, int, char *, int);
#endif /* } HAS_DEFQUEUE */

#ifdef HAS_DEFQSEMA /* { */
extern void API_PAGING KS_defqsema(QUEUE, SEMA, QCOND);
#endif /* } HAS_DEFQSEMA */

#ifdef HAS_INQQUEUE /* { */
extern int API_PAGING KS_inqqueue(QUEUE);
#endif /* } HAS_INQQUEUE */

#ifdef HAS_PURGEQUEUE /* { */
extern void API_PAGING KS_purgequeue(QUEUE);
#endif /* } HAS_PURGEQUEUE */

#endif /* } HAS_QUEUES */

/**********************/
/***** RESOURCES ******/
/**********************/
#ifdef HAS_RESOURCES /* { */

extern KSRC API_PAGING KS_lock(RESOURCE);

#ifdef HAS_LOCKW /* { */
extern KSRC API_PAGING KS_lockw(RESOURCE);
#endif /* } HAS_LOCKW */

#ifdef HAS_LOCKT /* { */
extern KSRC API_PAGING KS_lockt(RESOURCE, TICKS);
#endif /* } HAS_LOCKT */

extern KSRC API_PAGING KS_unlock(RESOURCE);

#ifdef HAS_INQRES /* { */
extern TASK API_PAGING KS_inqres(RESOURCE);
#endif /* } HAS_INQRES */

#ifdef PRIORITY_INVERSION /* { */
#ifdef HAS_DEFRES /* { */
extern KSRC API_PAGING KS_defres(RESOURCE, RESATTR);
#endif /* } HAS_DEFRES */
#endif /* } PRIORITY_INVERSION */

#endif /* } HAS_RESOURCES */

/***********************/
/****** TASK MGMT ******/
/***********************/
extern void API_PAGING KS_execute(TASK);

#ifdef HAS_TERMINATE /* { */
extern void API_PAGING KS_terminate(TASK);
#endif /* } HAS_TERMINATE */

#ifdef HAS_SUSPEND /* { */
extern void API_PAGING KS_suspend(TASK);
#endif /* } HAS_SUSPEND */

#ifdef HAS_RESUME /* { */
extern void API_PAGING KS_resume(TASK);
#endif /* } HAS_RESUME */

#ifdef HAS_YIELD /* { */
extern KSRC API_PAGING KS_yield(void);
#endif /* } HAS_YIELD */

#ifdef HAS_BLOCK /* { */
extern void API_PAGING KS_block(TASK, TASK);
#endif /* } HAS_BLOCK */

#ifdef HAS_UNBLOCK /* { */
extern void API_PAGING KS_unblock(TASK, TASK);
#endif /* } HAS_UNBLOCK */

#ifdef HAS_INQTASK /* { */
extern TASK API_PAGING KS_inqtask(void);
#endif /* } HAS_INQTASK */

#ifdef HAS_INQPRIORITY /* { */
extern PRIORITY API_PAGING KS_inqpriority(TASK);
#endif /* } HAS_INQPRIORITY */

#ifdef HAS_DEFPRIORITY /* { */
extern void API_PAGING KS_defpriority(TASK, PRIORITY);
#endif /* } HAS_DEFPRIORITY */

#ifdef HAS_DEFTASK /* { */
extern KSRC API_PAGING KS_deftask(TASK, PRIORITY, char *, size_t, void (*)(void));
#endif /* } HAS_DEFTASK */

#ifdef HAS_INQTASK_ARG /* { */
extern void * API_PAGING KS_inqtask_arg(TASK);
#endif /* } HAS_INQTASK_ARG */

#ifdef HAS_DEFTASK_ARG /* { */
extern void API_PAGING KS_deftask_arg(TASK, void *);
#endif /* } HAS_DEFTASK_ARG */

#ifdef HAS_ALLOC_TASK /* { */
extern TASK API_PAGING KS_alloc_task(void);
#endif /* } HAS_ALLOC_TASK */

/***********************/
/******  TIMERS   ******/
/***********************/
#ifdef HAS_ALLOC_TIMER /* { */
extern CLKBLK * API_PAGING KS_alloc_timer(void);
#endif /* } HAS_ALLOC_TIMER */

#ifdef HAS_FREE_TIMER /* { */
extern void API_PAGING KS_free_timer(CLKBLK *);
#endif /* } HAS_FREE_TIMER */

#ifdef HAS_START_TIMER /* { */
extern CLKBLK * API_PAGING KS_start_timer(CLKBLK *, TICKS, TICKS, SEMA);
#endif /* } HAS_START_TIMER */

#ifdef HAS_STOP_TIMER /* { */
extern KSRC API_PAGING KS_stop_timer(CLKBLK *);
#endif /* } HAS_STOP_TIMER */

#ifdef HAS_RESTART_TIMER /* { */
extern KSRC API_PAGING KS_restart_timer(CLKBLK *, TICKS, TICKS);
#endif /* } HAS_RESTART_TIMER */

#ifdef HAS_DELAY /* { */
extern void API_PAGING KS_delay(TASK, TICKS);
#endif /* } HAS_DELAY */

#ifdef HAS_ELAPSE /* { */
extern TICKS API_PAGING KS_elapse(TICKS *);
#endif /* } HAS_ELAPSE */

#ifdef HAS_INQTIMER /* { */
extern TICKS API_PAGING KS_inqtimer(CLKBLK *);
#endif /* } HAS_INQTIMER */

/**********************/
/**** SPECIAL SVCS ****/
/**********************/
extern int API_PAGING KS_user(int (*)(void *), void *);

#ifdef HAS_NOP /* { */
extern void API_PAGING KS_nop(void);
#endif /* } HAS_NOP */

extern const char * API_PAGING KS_version(void) PROGMEM ;

#ifdef HAS_INQTIME /* { */
extern time_t API_PAGING KS_inqtime(void);
#endif /* } HAS_INQTIME */

#ifdef HAS_DEFTIME /* { */
extern void API_PAGING KS_deftime(time_t);
#endif /* } HAS_DEFTIME */

#ifdef TIME_SLICE /* { */
extern void API_PAGING KS_defslice(TASK, TICKS);
#ifdef HAS_INQSLICE /* { */
extern TICKS API_PAGING KS_inqslice(TASK);
#endif /* } HAS_INQSLICE */
#endif /* } TIME_SLICE */

#ifdef HAS_PARTITIONS /* { */
extern void * API_PAGING KS_ISRalloc(MAP);
#endif /* } HAS_PARTITIONS */

/* required ISR functions */
extern FRAME * API_PAGING KS_ISRexit(FRAME *, SEMA);
extern void API_PAGING KS_ISRsignal(SEMA);
extern int  API_PAGING KS_ISRtick(void);

#endif /* } _RTXCAPI_H */

// end of rtxcapi.h

