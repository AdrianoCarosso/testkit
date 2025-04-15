// rtxcbug.c -- debug interface utility

//
//   RTXC    Version 3.2
//   Copyright (c) 1986-1997.
//   Embedded System Products, Inc.
//   ALL RIGHTS RESERVED
//
//
//   Copyright (c) 1997-2013.
//   T.E.S.T. srl
//

#undef        AUTOCBUG                 // lets run now !
#undef        USEHELP                  // use help

#include <stdio_console.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rtxcapi.h"
#include "enable.h"
#include "tstate.h"

#include "cqueue.h"
#include "cres.h"
#include "csema.h"
#include "cclock.h"
#include "cpart.h"

#include "rtxcbug.h"

#include "cvtdate.h"
#include "extapi.h"

#include "assign.h"

#if defined(USE_SPI_ON_ARM) || defined(USE_NANDFLASH_ON_ARM)
#define DEBUG_MEMORY_FUNCTIONS  // no longer useful when they work
#endif // defined(USE_SPI_ON_ARM) || defined(USE_NANDFLASH_ON_ARM)

//----------------------------------------------------------------------------
/* CONFIG */
#define STKLOGIC        // indicates task stack limit support

#define NULLTCB  ((TCB *)0)
#define NULLCLK  ((CLKBLK *)0)

#define NAMMAX         8        // max # chars in task/queue/etc names

#define ESC     27
#define BS      8
#define DEL     127

// *********************************************************************
// local buffer
#if defined(CBUG) || defined(USE_TWI_ON_ARM) || defined(USE_SPI_ON_ARM)
// mutex guarantee by EXTAPI resource
extern unsigned char extapibuf[256] __attribute__ ((aligned (2))) ;    // used also as word
// EXTAPI resource locks TWI and PDEBUGactivities
#endif // defined(CBUG) || defined(USE_TWI_ON_ARM) || defined(USE_SPI_ON_ARM)

#ifdef CBUG // RTXCbug desired


//----------------------------------------------------------------------------
// DIO handling
extern unsigned long dio_read(int port) ;
extern void dio_write(int port, int pmask, int pval) ;

//----------------------------------------------------------------------------

#ifdef GSM_COM
static void talkmdm(char * param) ;
#endif // GSM_COM

#ifdef GEMHH
#ifdef DEBUG_HW
static int outctrl(char * param) ;
#endif
#ifdef USE_FREQ_T2MAT2
static void dbgbeep(char * param) ;
#endif // USE_FREQ_T2MAT2
static void dbgLED(char * param) ;
#endif // GEMHH

#ifdef GPS_COM
static void talkgps(char * param) ;
#endif // GSM_COM
#ifdef G100
static void checkkey(char * param) ;
#endif
#ifdef USE_TWI2_ON_ARM
static void checkchbattery(char * param) ;
#endif

extern unsigned short com0err ; // error from drivers
extern unsigned short com1err ;
extern unsigned short com2err ;
extern unsigned short com3err ;
#ifdef USE_USB_ON_ARM
extern unsigned short usberr ;
#endif // USE_USB_ON_ARM

#ifdef STKLOGIC
extern const unsigned long Global_STACK_FILLER ;

#if defined(USE_LPC17XX) || defined(USE_LPC1788) || defined(USE_AT91SAM3S4)
extern const unsigned long __kernel_stack_start, __kernel_stack__end ;
extern const unsigned long _vStackTop ;
#endif // defined(USE_LPC17XX) || defined(USE_LPC1788) || defined(USE_AT91SAM3S4)

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
// Stack info from RTXCstartup.S
extern const unsigned long Global_RAM_TOP ;
extern const unsigned long Global_IRQ_STACK_SIZE ;
extern const unsigned long Global_SVC_STACK_SIZE ;
extern const unsigned long Global_SYSKERNEL_STACK_SIZE ;
extern char __noinit_end[] ;            // main stack, top of
extern char ram_end[] ;                 // end of RAM
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

void cbugstack(void) ;
int UsedStackEval(unsigned long *sstart, int ssize) ;
#endif // STKLOGIC

//extern SSTATE semat[] ;
//extern const char semaname[][NAMMAX+1] ;
//extern const SEMA nsemas ;
//extern SEMA siglist[] ;

extern TCB rtxtcb[] ;
extern const char taskname[][NAMMAX+1] ;
extern const TASK ntasks ;
#ifdef DYNAMIC_TASKS
extern const TASK dntasks ;
#endif

#ifdef HAS_MAILBOXES
extern MHEADER mheader[] ;
extern const char mboxname[][NAMMAX+1] ;
extern const MBOX nmboxes ;
#endif

//#ifdef HAS_PARTITIONS
//extern PHEADER pheader[] ;
//extern const char partname[][NAMMAX+1] ;
//extern const MAP nparts ;
//#ifdef DYNAMIC_PARTS
//extern const MAP dnparts ;
//#endif
//#endif

//#ifdef HAS_QUEUES
//extern QHEADER qheader[] ;
//extern const char queuename[][NAMMAX+1] ;
//extern const QUEUE nqueues ;
//#endif

//#ifdef HAS_RESOURCES
//extern RHEADER rheader[] ;
//extern const char resname[][NAMMAX+1] ;
//extern const RESOURCE nres ;
//#endif

extern CLKBLK * clkqptr ;
extern TICKS rtctick ;
//extern time_t rtctime ;

extern char isrmax ;

#ifdef M2102C
static void dbugacc(char * param) ;
#endif

#ifdef M2023
static void KillMts(void) ;
#endif


#ifdef USE_TRANSACTIONS_ON_ARM
static void cdflash(char * param) ;
#endif // USE_TRANSACTIONS_ON_ARM

#ifdef USE_PARAMETERS_ON_EEPROM
static void cdeedump(char * param) ;
#endif
//#ifdef USE_SPI_ON_ARM
static void cdeeprom(char * param) ;
static void cdeeread(char * param) ;
static void cdeewrite(char add, char * param) ;
static void cdeeclear(char * param) ;
static void cdeeinit(char * param) ;
//#endif // USE_SPI_ON_ARM

static void cddump(char * param) ;
static void cdoff(char * param) ;
static void cdwatch(char * param) ;

#ifdef M2102C
static void dsetvibra(char * param) ;
#endif

static void cbugclock(void) ;
#ifdef HAS_MAILBOXES
static void cbugmbox(void) ;
#endif
#ifdef HAS_PARTITIONS
static void cbugpart(void) ;
#endif
#ifdef HAS_QUEUES
static void cbugqueue(void) ;
#endif
#ifdef HAS_RESOURCES
static void cbugres(void) ;
#endif
static void cbugsema(void) ;
static void cbugtask(void) ;
static void cbugzero(void) ;
#ifdef TASKDETAIL
static void cbugregs(void) ;
extern void taskmgr(void) ;
#endif        // TASKDETAIL

extern char *xtaskname(TASK task) ;
extern char *xsemaname(int s) ;
#ifdef HAS_QUEUES
extern char *xqueuename(int q) ;
#endif
#ifdef HAS_PARTITIONS
extern char *xpartname(MAP map) ;
#endif
#ifdef HAS_RESOURCES
extern char *xresname(int r) ;
#endif

extern void showtime(OBJTYPE objtype, TASK task, int objid) ;

#ifdef TASKDETAIL
extern TASK gettask(void) ;
#endif

extern PRIORITY getpriority(void) ;
#ifdef TIME_SLICE
extern TICKS getslice(void) ;
#endif

#ifdef GPS_I2C
static void cbug_gpsi2c(char * param) ;
#endif


#ifdef G100
extern void SmartCard_Write(int port, int addr, int nbyte, char *txbuf) ;

static void cardsn(char * param) ;
#endif // G100

extern void inputs(char *, short len) ; // get '\r' terminated line from input

char cbugflag ;                 // clock ISR ignore flag, initially = 0

//static TICKS et ;

//extern void sgnlcbug(void) ;

void rtxcbug(void) TASK_ATTRIBUTE ;

static void cbugrtxc(void) ;        // RTXC object menu function

#ifdef USEHELP
static void menurtxc(void) ;        // rtxc menu for "H" help option in rtxc menu
#endif

void rtxcbug(void)
{
#ifdef BSS_NOT_ZERO
	cbugflag = 0 ;                // reset
#endif

#ifdef AUTOCBUG
	// allow for auto entry to CBUG before any tasks start
	KS_signal(CBUGSEMA) ;
#endif

	for( ; ; ) {
//        KS_elapse(&et) ;        // initialize time stamp

		KS_wait(CBUGSEMA) ;        // wait for Cbug event

//        et = KS_elapse(&et) ;        // capture time since last event

#if CBUGRES
		KS_lockw(CBUGRES) ;        // lock and wait for availability
		KS_unlock(CBUGRES) ;        // else deadlock since next will block world
#endif

		cbugflag = 1 ;                // set bug active, in case not already set

		// block everybody except self
#ifdef DYNAMIC_TASKS
		KS_block(1, ntasks + dntasks) ;
#else
		KS_block(1, ntasks) ;
#endif

		// unblock i/o drivers
		MONIUNBLOCK ;
		//KS_unblock(CBUGIDRV, CBUGIDRV);
		//KS_unblock(CBUGODRV, CBUGODRV);

		puts(KS_version()) ;

// +++++++++++++++++++++++++
cbugqueue() ;
cbugstack() ;
// ++++++++++++++++++++++++

		cbugrtxc() ;

		puts("Returning from RTXCBug") ;

		// release all task blocks
#ifdef DYNAMIC_TASKS
		KS_unblock(1, ntasks + dntasks) ;
#else
		KS_unblock(1, ntasks) ;
#endif

		cbugflag = 0 ;                // disable bug on exit
	}
}

static void cbugrtxc(void)
{
	static char string[256] ; // [60] ;


	for( ; ; ) {
		printf("RTXCbug> ") ;
		inputs(string, sizeof(string)-2) ;
		if (string[0] < ' ')
			continue ;

		switch(string[0]) {
#ifdef GSM_COM
		case 'a' : 
		case 'A' :
			talkmdm(&string[0]);
			break;
#endif // GSM_COM

#ifdef GEMHH
#ifdef DEBUG_HW
		case 'a' : 
		case 'A' :
			if (outctrl(&string[1]))  return ;
			break;
#endif
#ifdef USE_FREQ_T2MAT2
		case 'y':
		case 'Y':
			dbgbeep(&string[0]);
			break ;
#endif // USE_FREQ_T2MAT2

		case 'l':
		case 'L':
#define FR_ITM_ENA      (*(volatile unsigned int*)0xE0000E00)      // ITM Enable
#define FR_ITM_TPR      (*(volatile unsigned int*)0xE0000E40)      // Trace Privilege Register
#define FR_ITM_TCR      (*(volatile unsigned int*)0xE0000E80)      // ITM Trace Control Reg.
#define FR_ITM_LSR      (*(volatile unsigned int*)0xE0000FB0)      // ITM Lock Status Register
#define FR_DHCSR        (*(volatile unsigned int*)0xE000EDF0)      // Debug register
#define FR_DEMCR        (*(volatile unsigned int*)0xE000EDFC)      // Debug register
#define FR_TPIU_ACPR    (*(volatile unsigned int*)0xE0040010)      // Async Clock presacler register
#define FR_TPIU_SPPR    (*(volatile unsigned int*)0xE00400F0)       // Selected Pin Protocol Register
#define FR_DWT_CTRL     (*(volatile unsigned int*)0xE0001000)       // DWT Control Register
#define FR_FFCR         (*(volatile unsigned int*)0xE0040304)       // Formatter and flush Control Register

		if (FR_ITM_ENA!=0x0) FR_ITM_ENA = 0x0 ;
		if (FR_ITM_TPR!=0x0) FR_ITM_TPR = 0x0 ;
		if (FR_ITM_TCR!=0x0) FR_ITM_TCR = 0x0 ;
		if (FR_ITM_LSR!=0x0) FR_ITM_LSR = 0x0 ;
		if (FR_DHCSR!=0x3010001) FR_DHCSR = 0x3010001 ;
		if (FR_DEMCR!=0x1000000) FR_DEMCR = 0x1000000 ;
		if (FR_TPIU_ACPR!=0x0) FR_TPIU_ACPR = 0x0 ;
		if (FR_TPIU_SPPR!=0x1) FR_TPIU_SPPR = 0x1 ;
		if (FR_DWT_CTRL!=0x40000000) FR_DWT_CTRL = 0x40000000 ;
		if (FR_FFCR!=0x102) FR_FFCR = 0x102 ;

// 			{
// 				unsigned long *pp ;
// 				pp = (unsigned long *) 0xe0002000 ;
// 				printf("\n0xe0002000:0x%lx\n", *pp ) ;
// 				//if (*pp==0x260){
// 					*pp = 0x3 ;
// 					pp = (unsigned long *) 0xe0002008 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe000200c ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0002010 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0002014 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0002018 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe000201c ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0002020 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0002024 ; *pp = 0x0 ;
// 					
// 					pp = (unsigned long *) 0xe0001020 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001024 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001028 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001030 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001034 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001038 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001040 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001044 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001048 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001050 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001054 ; *pp = 0x0 ;
// 					pp = (unsigned long *) 0xe0001058 ; *pp = 0x0 ;
// 					
// // 					NVIC_SystemReset() ;
// // 					for(;;) {}
// 				//}
// 			}
			dbgLED(&string[0]);
			break ;
#endif // GEMHH

#ifdef G100
		case 'b':
		case 'B':
			checkkey(&string[0]);
			break ;
#endif
#ifdef USE_TWI2_ON_ARM
		case 'b':
		case 'B':
			checkchbattery(&string[1]);
			break ;
#endif
		case 'C' :
		case 'c' :
			cbugclock() ;
			break ;

		case 'D' :
		case 'd' :
			 cddump(&string[1]) ;
			 break ;

//#ifdef USE_SPI_ON_ARM
		case 'E' :
		case 'e' :
			 cdeeprom(&string[1]) ;
			 break ;
//#endif // USE_SPI_ON_ARM

#ifdef USE_TRANSACTIONS_ON_ARM
		case 'F' :
		case 'f' :
			 cdflash(&string[1]) ;
			 break ;
#endif // USE_TRANSACTIONS_ON_ARM

#ifdef USEHELP
		case 'H' :
		case 'h' :
			menurtxc() ;
			break ;
#endif

#ifdef M2102C
		case 'I':
		case 'i':
			dbugacc(&string[1]) ;
			break ;
#endif

#ifdef GPS_I2C
		case 'L':
		case 'l':
			cbug_gpsi2c(&string[2]) ;
			break ;
#endif

#ifdef STKLOGIC
		case 'K' :
		case 'k' :
			cbugstack() ;
			break ;
#endif

#ifdef HAS_MAILBOXES
		case 'M' :
		case 'm' :
			cbugmbox() ;
			break ;
#endif

		case 'O' :
		case 'o' :
			 cdoff(&string[1]) ;
			 //RTC_ReadTime(&rtxc_rtw) ;                   // read from RTC
			 //KS_deftime(date2systime(&rtxc_rtw)) ;       // set new time
			 return ;                  // let other task to turn off

#ifdef HAS_PARTITIONS
		case 'P' :
		case 'p' :
			cbugpart() ;
			break ;
#endif

#ifdef HAS_QUEUES
		case 'Q' :
		case 'q' :
			cbugqueue() ;
			break ;
#endif

#ifdef HAS_RESOURCES
		case 'R' :
		case 'r' :
			cbugres() ;
			break ;
#endif
		case 'S' :
		case 's' :
			cbugsema() ;
			break ;

		case 'T' :
		case 't' :
			cbugtask() ;
			break;
#ifdef M2102C
		case 'V':
		case 'v':
			dsetvibra(&string[1]) ;
			 break ;
#endif
		case 'W' :
		case 'w' :
			 cdwatch(&string[1]) ;
			 break ;

#ifdef GPS_COM
		case 'x' :
		case 'X' :
			talkgps(&string[0]);
			break;
#endif // GSM_COM

		case 'Z' :      // reset statistics
		case 'z' :
			cbugzero() ;
			break ;

#ifdef TASKDETAIL
		case '!' :
		   // block everybody except self
#ifdef DYNAMIC_TASKS
		   KS_block(1, ntasks + dntasks) ;
#else
		   KS_block(1, ntasks) ;
#endif
		   // unblock i/o drivers
		   MONIUNBLOCK ;
		   //KS_unblock(CBUGIDRV, CBUGIDRV) ;
		   //KS_unblock(CBUGODRV, CBUGODRV) ;
		   //KS_unblock(CBUGIODRV, CBUGIODRV);
		   break ;
#endif        // TASKDETAIL

#ifdef TASKDETAIL
		case '#' :      // display task registers
			cbugregs() ;
			break ;

		case '$' :
			taskmgr() ;
			break ;
#endif        // TASKDETAIL

#ifdef M2023
		case 'U' :
		case 'u' :
			KillMts() ;
			break ;
#endif
		default :                        // syntax error
//      case 'G' :
//      case 'g' :
			//RTC_ReadTime(&rtxc_rtw) ;                   // read from RTC
			//KS_deftime(date2systime(&rtxc_rtw)) ;       // set new time
			return ;
	  }
   }
}

#ifdef GSM_COM
static void talkmdm(char * param)
{
char *c, cc, cip, cop, updm, rtscts, dop, dip ;
unsigned long lbaud, old_baud ;
//unsigned short c0err, c1err ;
int port ;
	
	lbaud = old_baud = 0L ;
	
	port = GSM_COM ;
	cip = MONITIPORT ;
	cop = MONITOPORT ;
	
	dip = MODEMIQ ;
	dop = MODEMOQ ;
	
	updm = 0 ;
	rtscts = 0 ;
	c = param ;
	if (*param){

#ifdef CLOSE_SINGLECOM
		if ((c[1]=='o') || (c[1]=='O')){
			
			printf("\nClosing GSM port and free TX p1.10 out\n" ) ;
			uartNstop((1<<GSM_COM)) ;
#ifdef USE_LPC1788
//			{
				volatile uint32_t * const iocon_base = ((uint32_t *)(LPC_IOCON_BASE)) ;
// 			LPC_GPIO2->DIR &= ~(0x00000001) ;
// 			LPC_GPIO1->DIR |= 0x400 ;
				iocon_base[ 32*2 + 5 ] = 0 ;
				LPC_GPIO2->DIR |= 0x00000010 ; // RTS as output
				LPC_GPIO2->CLR = 0x10 ;
//			}
			// PINCON->PINMODE4 &= ~(0x2) ; // FR
#else
			GPIO2->FIODIR &= ~(0x00000001) ;
			GPIO1->FIODIR |= 0x400 ;
			PINCON->PINMODE4 &= ~(0x2) ;
#endif
			return ;
		}
#endif

#ifdef USE_LPC1788
	if (c[1]=='3'){
		port = 3 ;
		dip = COM3IQ ;
		dop = COM3OQ ;
	}
#ifdef USE_COM4_ON_ARM
	if (c[1]=='4'){
		port = 4 ;
		dip = COM4IQ ;
		dop = COM4OQ ;
	}
#endif
#endif

		while(*param != ' ')        // skip START
					param++ ;
					
		while(*param == ' ')        // skip blank
					param++ ;

		lbaud = atol(param) ;
		if (lbaud){
			if ((c[1]=='T')||(c[1]=='t')) rtscts = 1 ;
//			if (GSM_used==GSMCODE_SIEMENS){
				printf("\nSet GSM baud to %lu %s <%s>",lbaud, ((rtscts)? "H8N1":"8N1"), c ) ;
				uartstart( port, lbaud, ((rtscts)? SCOM_H8N1:SCOM_8N1)) ;
				printf("...OK\n" ) ;
//			}

#if defined(USE_AT91SAM7A3)
			if (c[0]=='A'){
			    unsigned long baud_value ;

				// Define the baud rate divisor register
			    baud_value = ((current_clock * 10)/(lbaud * 16)) ;
			    if ((baud_value % 10) >= 5) {
			        baud_value = (baud_value / 10) + 1;
			    } else {
			        baud_value /= 10;
			    }
				updm = 1 ;
#if (MONIT_COM==1)
			    old_baud = AT91C_BASE_US1->US_BRGR ;
			    AT91C_BASE_US1->US_BRGR = baud_value ;
			    if (rtscts){
				    AT91C_BASE_PIOB->PIO_BSR = (unsigned long)(AT91C_PB24_RTS1 | AT91C_PB25_CTS1) ;
				    AT91C_BASE_PIOB->PIO_PDR = (unsigned long)(AT91C_PB24_RTS1 | AT91C_PB25_CTS1) ;
				}
//				cip = COM1IQ ;
//				cop = COM1OQ ;
#endif
#if (MONIT_COM==2)
			    old_baud = AT91C_BASE_US2->US_BRGR ;
			    AT91C_BASE_US2->US_BRGR = baud_value ;
			    // Handshake
			    if (rtscts){
				    AT91C_BASE_PIOB->PIO_BSR = (unsigned long)(AT91C_PB27_RTS2 | AT91C_PB28_CTS2) ;
				    AT91C_BASE_PIOB->PIO_PDR = (unsigned long)(AT91C_PB27_RTS2 | AT91C_PB28_CTS2) ;
				}
//				cip = COM2IQ ;
//				cop = COM2OQ ;
#endif
	
				printf("\nSetted GSM baud to %lu %s\n",lbaud, ((rtscts)? "H8N1":"8N1") ) ;
			}else
				old_baud = lbaud ;
			
#endif // #if defined(USE_AT91SAM7A3)
		}
	}

#if defined(USE_AT91SAM7A3)	
#if (MONIT_COM==0)
	cip = COM0IQ ;
	cop = COM0OQ ;
#endif
#if (MONIT_COM==1)
	cip = COM1IQ ;
	cop = COM1OQ ;
#endif
#if (MONIT_COM==2)
	cip = COM2IQ ;
	cop = COM2OQ ;
#endif
#endif // #if defined(USE_AT91SAM7A3) || defined(USE_LPC17XX)	
//	c0err = com0err ;
//	c1err = com1err ;

	for(;;){
#if defined(USE_AT91SAM7A3)
		if (c1err!=com1err){
			cc = 'E' ;
			KS_enqueuew(COM2OQ, &cc) ;
			cc = '1' ;
			KS_enqueuew(COM2OQ, &cc) ;
			c1err = com1err ;
		}
		if (c0err!=com0err){
			cc = 'E' ;
			KS_enqueuew(COM2OQ, &cc) ;
			cc = '2' ;
			KS_enqueuew(COM2OQ, &cc) ;
			c0err = com0err ;
		}
#endif // #if defined(USE_AT91SAM7A3)

//		if (updm){
//			if (KS_dequeue(MONITIPORT, &c)==RC_GOOD){
//	     		if (c==0x09) break ;
//				if (c=='1'){
//					printf("\nRESET GSM\n");
//					dio_write(1, (1<<1), (0<<1)) ;
//					dio_write(1, (1<<3), (0<<3)) ;
//				}else{
//					dio_write(1, (1<<3), (1<<3)) ;
//					dio_write(1, (1<<1), (1<<1)) ;
//					printf("\nEnd RESET GSM\n");
//				}
//			}
//		}
   		if (KS_dequeue(cip, &cc)==RC_GOOD){
	     	if (!updm) { if (cc==0x09) break ; }
	     	//KS_enqueuew(MONITOPORT, &c) ;
	     	KS_enqueuew(dop, &cc) ;
	   }
	   if (KS_dequeue(dip, &cc)==RC_GOOD) {
//#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
	   		KS_enqueuew(cop, &cc) ;
//#endif
//#if defined(USE_AT91SAM7A3)
//	   		KS_enqueuew(COM1OQ, &cc) ;
//	   		KS_enqueuew(COM2OQ, &cc) ;
//#endif
	   	}
	}
#if defined(USE_AT91SAM7A3)

#if (MONIT_COM==1)
	if (lbaud != old_baud) AT91C_BASE_US1->US_BRGR = old_baud ;
#endif
#if (MONIT_COM==2)
	if (lbaud != old_baud) AT91C_BASE_US2->US_BRGR = old_baud ;
#endif
#endif // #if defined(USE_AT91SAM7A3)
}
#endif // GSM_COM

#ifdef GPS_COM
static void talkgps(char * param)
{
	char *c, cc, cip, cop ;
	unsigned long lbaud, old_baud ;
	unsigned short c0err, c1err ;
	lbaud = old_baud = 0L ;
	cip = MONITIPORT ;
	cop = MONITOPORT ;
	
	c = param ;
	if (*param){
		while(*param != ' ')        // skip START
					param++ ;
					
		while(*param == ' ')        // skip blank
					param++ ;

		lbaud = atol(param) ;
		if (lbaud){
				uartstart( GPS_COM, lbaud, SCOM_8N1) ;

#if defined(USE_AT91SAM7A3)
			if (c[0]=='G'){
			    unsigned long baud_value ;

				// Define the baud rate divisor register
			    baud_value = ((current_clock * 10)/(lbaud * 16)) ;
			    if ((baud_value % 10) >= 5) {
			        baud_value = (baud_value / 10) + 1;
			    } else {
			        baud_value /= 10;
			    }
#if (MONIT_COM==1)
			    old_baud = AT91C_BASE_US1->US_BRGR ;
			    AT91C_BASE_US1->US_BRGR = baud_value ;
// 			    if (rtscts){
// 				    AT91C_BASE_PIOB->PIO_BSR = (unsigned long)(AT91C_PB24_RTS1 | AT91C_PB25_CTS1) ;
// 				    AT91C_BASE_PIOB->PIO_PDR = (unsigned long)(AT91C_PB24_RTS1 | AT91C_PB25_CTS1) ;
// 				}
//				cip = COM1IQ ;
//				cop = COM1OQ ;
#endif
#if (MONIT_COM==2)
			    old_baud = AT91C_BASE_US2->US_BRGR ;
			    AT91C_BASE_US2->US_BRGR = baud_value ;
			    // Handshake
// 			    if (rtscts){
// 				    AT91C_BASE_PIOB->PIO_BSR = (unsigned long)(AT91C_PB27_RTS2 | AT91C_PB28_CTS2) ;
// 				    AT91C_BASE_PIOB->PIO_PDR = (unsigned long)(AT91C_PB27_RTS2 | AT91C_PB28_CTS2) ;
// 				}
//				cip = COM2IQ ;
//				cop = COM2OQ ;
#endif
	
				printf("\nSetted GPS baud to %lu %s\n",lbaud, "8N1" ) ;
			}else
				old_baud = lbaud ;
			
#endif // #if defined(USE_AT91SAM7A3)
		}
	}

#if defined(USE_AT91SAM7A3)	
#if (MONIT_COM==0)
	cip = COM0IQ ;
	cop = COM0OQ ;
#endif
#if (MONIT_COM==1)
	cip = COM1IQ ;
	cop = COM1OQ ;
#endif
#if (MONIT_COM==2)
	cip = COM2IQ ;
	cop = COM2OQ ;
#endif
#endif // #if defined(USE_AT91SAM7A3) || defined(USE_LPC17XX)	
	c0err = com0err ;
	c1err = com1err ;

	for(;;){
#if defined(USE_AT91SAM7A3)		
		if (c1err!=com1err){
			cc = 'E' ;
			KS_enqueuew(COM2OQ, &cc) ;
			cc = '1' ;
			KS_enqueuew(COM2OQ, &cc) ;
			c1err = com1err ;
		}
		if (c0err!=com0err){
			cc = 'E' ;
			KS_enqueuew(COM2OQ, &cc) ;
			cc = '2' ;
			KS_enqueuew(COM2OQ, &cc) ;
			c0err = com0err ;
		}
#endif // #if defined(USE_AT91SAM7A3)

   		if (KS_dequeue(cip, &cc)==RC_GOOD){
	     	//if (!updm) 
			{ if (cc==0x09) break ; }
	     	//KS_enqueuew(MONITOPORT, &c) ;
	     	KS_enqueuew(GPSOPORT, &cc) ;
			if (cc=='\r'){
				cc='\n' ;
				KS_enqueuew(GPSOPORT, &cc) ;
			}
	   }
	   if (KS_dequeue(GPSIPORT, &cc)==RC_GOOD) {
//#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
	   		KS_enqueuew(cop, &cc) ;
//#endif	   		
//#if defined(USE_AT91SAM7A3)
//	   		KS_enqueuew(COM1OQ, &cc) ;
//	   		KS_enqueuew(COM2OQ, &cc) ;
//#endif
	   	}
	}
#if defined(USE_AT91SAM7A3)
	
#if (MONIT_COM==1)
	if (lbaud != old_baud) AT91C_BASE_US1->US_BRGR = old_baud ;
#endif
#if (MONIT_COM==2)
	if (lbaud != old_baud) AT91C_BASE_US2->US_BRGR = old_baud ;
#endif
#endif // #if defined(USE_AT91SAM7A3)
}
#endif // GPS_COM

#ifdef G100
#define MASK_KEY 0x9B8

static void checkkey(char * param)
{
char cc ;
unsigned long keydata, old_keydata ;
short  dt ;

	if (*param){
		while(*param != ' ')        // skip START
					param++ ;
					
		while(*param == ' ')        // skip blank
					param++ ;

		dt = atoi(param) ;
	}else
		dt = 100 ;
	
	old_keydata = 0 ;
	for(;;){
		keydata = dio_read(PORT_PIO2) ;
		if ( (keydata & MASK_KEY) != (old_keydata & MASK_KEY) ){
			
			if ( (keydata & (1<<HW_2i3_KEY_ESC)) != (old_keydata & (1<<HW_2i3_KEY_ESC)) ) 
				printf("KEY ESC %d\n", ((keydata & (1<<HW_2i3_KEY_ESC))? 0:1) ) ;
			
			if ( (keydata & (1<<HW_2i4_KEY_UP)) != (old_keydata & (1<<HW_2i4_KEY_UP)) ) 
				printf("KEY UP %d\n", ((keydata & (1<<HW_2i4_KEY_UP))? 0:1) ) ;
			
			if ( (keydata & (1<<HW_2i5_KEY_DOWN)) != (old_keydata & (1<<HW_2i5_KEY_DOWN)) ) 
				printf("KEY DOWN %d\n", ((keydata & (1<<HW_2i5_KEY_DOWN))? 0:1) ) ;
			
			if ( (keydata & (1<<HW_2i7_KEY_LEFT)) != (old_keydata & (1<<HW_2i7_KEY_LEFT)) ) 
				printf("KEY LEFT %d\n", ((keydata & (1<<HW_2i7_KEY_LEFT))? 0:1) ) ;
			
			if ( (keydata & (1<<HW_2i8_KEY_RIGHT)) != (old_keydata & (1<<HW_2i8_KEY_RIGHT)) ) 
				printf("KEY RIGHT %d\n", ((keydata & (1<<HW_2i8_KEY_RIGHT))? 0:1) ) ;
			
			if ( (keydata & (1<<HW_2i11_KEY_ENTER)) != (old_keydata & (1<<HW_2i11_KEY_ENTER)) ) 
				printf("KEY ENTER %d\n", ((keydata & (1<<HW_2i11_KEY_ENTER))? 0:1) ) ;
			old_keydata = keydata ;
		}
		KS_delay(SELFTASK, ((TICKS)dt/CLKTICK)) ;
		
		// check for end
		if (KS_dequeue(MONITIPORT, &cc)==RC_GOOD){
			if (cc==0x09) break ;
	   }
	}
}
#endif


static void cbugtask(void)
{
	int i, j ;
	int priority;
#if defined(MAILBOX_SEMAS) || defined(QUEUE_SEMAS)
	int k ;
#endif
#if defined(PARTITION_WAITERS) || \
	defined(MAILBOX_WAITERS)   || \
	defined(QUEUE_WAITERS)     || \
	defined(RESOURCE_WAITERS)
	TCB *nexttask;
#endif

	printf("** Task Snapshot **\n") ;
	printf("  #   Name  Priority  State\n") ;
		 // 123 12345678  123          12345678
#ifdef DYNAMIC_TASKS
	for (i = 1 ; i <= ntasks + dntasks ; i++) {
#else
	for (i = 1 ; i <= ntasks ; i++) {
#endif
		priority = rtxtcb[i].priority;
		printf("%3d %8s  %3d     ", i, xtaskname(i), priority) ;

		if (rtxtcb[i].status & SUSPFLG)
			printf("SUSPENDED ") ;

		if (rtxtcb[i].status & INACTIVE)
			printf("INACTIVE ") ;

		if (rtxtcb[i].status & SEMAPHORE_WAIT) {
			printf("Semaphore ") ;      // now find associated sema(s)
			for (j = 1 ; j <= nsemas ; j++) {
				if ((semat[j] == i)) {
					printf(" %8s", xsemaname(j)) ;

					showtime(TIMER_OBJ, i, j) ;

#ifdef SEMAPHORE_TIMEOUTS
					showtime(SEMAPHORE_OBJ, i, j) ;
#endif

#ifdef QUEUE_SEMAS
					for (k=1 ; k <= nqueues ; k++) {
						if (j == qheader[k].esema)
							printf("<QE> %8s", xqueuename(k)) ;

						if (j == qheader[k].fsema)
							printf("<QF> %8s", xqueuename(k)) ;

						if (j == qheader[k].nesema)
							printf("<QNE> %8s", xqueuename(k)) ;

						if (j == qheader[k].nfsema)
							printf("<QNF> %8s", xqueuename(k)) ;
					}
#endif

#ifdef MAILBOX_SEMAS
					for (k=1 ; k <= nmboxes ; k++) {
						if (j == mheader[k].nesema)
							printf("<MBXNE> %8s", &mboxname[k][0]) ;
					}
#endif
				}
			}
		}

#ifdef QUEUE_WAITERS
		if (rtxtcb[i].status & QUEUE_WAIT) {
			for(j=1 ; j <= nqueues ; j++) {        // check all queues

				nexttask = qheader[j].waiters ;
				while(nexttask != NULLTCB) {        // by walking list of waiters
					if (nexttask->task == i) {
						if (qheader[j].curndx == 0)
							printf("QueueEmpty") ;
						else
							printf("QueueFull ") ;
						printf(" %8s", xqueuename(j)) ;
#ifdef QUEUE_TIMEOUTS
						showtime(QUEUE_OBJ, i, j) ;
#endif
					}
					nexttask = nexttask->flink ;
				}
			}
		}
#endif

#ifdef MAILBOX_WAITERS
		if (rtxtcb[i].status & MSG_WAIT) {
			printf("Mailbox    ") ;
			for(j=1 ; j <= nmboxes ; j++) {
				nexttask = mheader[j].waiters;
				while(nexttask != NULLTCB) {
					if (nexttask->task == i) {        // if task i is waiting on mbox j

						printf("%8s", &mboxname[j][0]) ;
#ifdef MAILBOX_TIMEOUTS
						showtime(MAILBOX_OBJ, i, j) ;
#endif
					}
					nexttask = nexttask->flink ;
				}
			}
		}
#endif

		if (rtxtcb[i].status & DELAY_WAIT) {
			printf("DELAY              ") ;     // now find time remaining
			showtime(DELAY_OBJ, i, i) ;
		}

#ifdef RESOURCE_WAITERS
		if (rtxtcb[i].status & RESOURCE_WAIT) {
			printf("Resource   ") ;     // find associated resource
			for(j=1 ; j <= nres ; j++) {        // by checking all resources

				nexttask = rheader[j].waiters ;
				while (nexttask != NULLTCB) {        // by walking list of waiters

					if (nexttask->task == i) {        // if task i is waiting on res j

						printf("%8s", xresname(j)) ; // note resource
#ifdef RESOURCE_TIMEOUTS
						showtime(RESOURCE_OBJ, i, j) ;
#endif
					}
					nexttask = nexttask->flink ;
				}
			}
		}
#endif

#ifdef PARTITION_WAITERS
		if (rtxtcb[i].status & PARTITION_WAIT) {
			printf("Partition  ") ;     // find associated partition
			for(j=1 ; j <= nparts ; j++) {        // by checking each partition

				nexttask = pheader[j].waiters ;
				while(nexttask != NULLTCB) {        // by walking list of waiters

					if (nexttask->task == i) {        // if task i waiting on partition j

						printf("%8s", xpartname(j)) ;   // note partition
#ifdef PARTITION_TIMEOUTS
						showtime(PARTITION_OBJ, i, j) ;
#endif
					}
					nexttask = nexttask->flink ;
				}
			}
		}
#endif

		if (rtxtcb[i].status == BLOCK_WAIT)        // ready but only blocked by Cbug
			printf("-READY ") ;

		if (rtxtcb[i].status == 0)
			printf("READY") ;

#ifdef TIME_SLICE
		if (rtxtcb[i].newslice) { // if time slicing enabled for task

			printf(" (%4d/%4d ticks)", rtxtcb[i].tslice, rtxtcb[i].newslice) ;
		}
#endif

		printf("\n") ;
	}
}

#ifdef HAS_QUEUES
static void cbugqueue(void)
{
	int i ;
#ifdef QUEUE_WAITERS
	TCB *nexttask ;
#endif
#ifdef QUEUE_SEMAS
	int /* SEMA */ sema ;
#endif

	printf("** Queue Snapshot **\n") ;
	printf("  #   Name  Current/Depth Worst Count Waiters\n") ;
				// 123 12345678  12345/12345 12345 12345 12345678 12345678
	for(i=1 ; i <= nqueues ; i++) {
		printf("%3d %8s  %5d/%5d %5d %5u", i, xqueuename(i),
				qheader[i].curndx, qheader[i].depth,
				qheader[i].worst, qheader[i].count) ;

#ifdef QUEUE_WAITERS
		nexttask = qheader[i].waiters ;
		while(nexttask != NULLTCB) {        // by walking list of waiters

			printf(" %8s", xtaskname(nexttask->task)) ;
			nexttask = nexttask->flink ;
		}
#endif

#ifdef QUEUE_SEMAS
		if ( ((sema = qheader[i].esema) != 0) && (semat[sema] > 0) )
			printf(" %8s<E>", xsemaname(sema)) ;

		if ( ((sema = qheader[i].fsema) != 0) && (semat[sema] > 0) )
			printf(" %8s<F>", xsemaname(sema)) ;

		if ( ((sema = qheader[i].nesema) != 0) && (semat[sema] > 0) )
			printf(" %8s<NE>", xsemaname(sema)) ;

		if ( ((sema = qheader[i].nfsema) != 0) && (semat[sema] > 0) )
			printf(" %8s<NF>", xsemaname(sema)) ;
#endif

		printf("\n") ;
	}
}
#endif

static void cbugsema(void)
{
	int i ;
	TASK task ;
#if defined(QUEUE_SEMAS) || defined(MAILBOX_SEMAS)
	int k ;
#endif

	printf("** Semaphore Snapshot **\n") ;
	printf("  #   Name  State  Waiter\n") ;
		//  123 12345678 1234 12345678
		//        xxxxxxxx DONE
		//        xxxxxxxx PEND
		//        xxxxxxxx WAIT xxxxxxxx

	for(i=1 ; i <= nsemas ; i++) {
		printf("%3d %8s ", i, xsemaname(i)) ;

		switch(semat[i]) {
		case SEMA_DONE :
			printf("DONE\n") ;
			break ;

		case SEMA_PENDING :
			printf("PEND\n") ;
			break ;

		default :
			task = semat[i] ;
			printf("WAIT %8s", xtaskname(task)) ;
#ifdef SEMAPHORE_TIMEOUTS
			showtime(SEMAPHORE_OBJ, task, i) ;
#endif

#ifdef QUEUE_SEMAS
			for(k=1 ; k <= nqueues ; k++) {
				if (i == qheader[k].esema)
					printf(" <QE> %8s", xqueuename(k)) ;

				if (i == qheader[k].fsema)
					printf(" <QF> %8s", xqueuename(k)) ;

				if (i == qheader[k].nesema)
					printf(" <QNE> %8s", xqueuename(k)) ;

				if (i == qheader[k].nfsema)
					printf(" <QNF> %8s", xqueuename(k)) ;
			}
#endif

#ifdef MAILBOX_SEMAS
			for(k=1 ; k <= nmboxes ; k++) {
				if (i  == mheader[k].nesema)
					printf(" <MBXNE> %8s", &mboxname[k][0]) ;
			}
#endif
			printf("\n") ;
			break ;
		}
	}
}

#ifdef HAS_RESOURCES
static void cbugres(void)
{
	int i ;
#ifdef RESOURCE_WAITERS
	TCB *nexttask ;
#endif

	printf("** Resource Snapshot **\n") ;
	printf("  #   Name   Count Conflicts   Owner   Waiters\n") ;
		 // 123 12345678 12345         12345          12345678 12345678 12345678

	for(i=1 ; i <= nres; i++) {
#ifdef PRIORITY_INVERSION
		printf("%3d %8s %5u   %5u", i, xresname(i),
			   rheader[i].count, rheader[i].conflict) ;

		if (rheader[i].resattr == PRIORITY_INVERSION_ON)
			printf(" ** ") ;
		else
			printf("    ") ;
#else
		printf("%3d %8s %5u   %5u    ", i, xresname(i),
			   rheader[i].count, rheader[i].conflict) ;
#endif

		if (rheader[i].owner != NULLTCB) {
			printf("%8s", xtaskname(rheader[i].owner->task)) ;

#ifdef RESOURCE_WAITERS
			nexttask = rheader[i].waiters ;
			while(nexttask != NULLTCB) {
				printf(" %8s", xtaskname(nexttask->task)) ;
				nexttask = nexttask->flink ;
			}
#endif
		}
		printf("\n") ;
	}
}
#endif

#ifdef HAS_PARTITIONS
static void cbugpart(void)
{
	int i, j ;
	struct xmap *next ;
#ifdef PARTITION_WAITERS
	TCB *nexttask ;
#endif

#if defined(USE_LPC17XX)
extern const uint32_t my_prot_flag ;

	i = RUNCODE_START + 0x2fc ;
	printf("Code protect 0x%08lx (0x%08lx)\n", *((uint32_t *) i), my_prot_flag ) ;
#endif
	printf("** Partition Snapshot **\n") ;
	printf("  #   Name   Avail/Total Worst Count  Bytes Waiters\n") ;
		 // 123 12345678 12345/12345 12345 12345 123456 12345678 1345678

#ifdef DYNAMIC_PARTS
	for(i=1 ; i <= nparts + dnparts ; i++) {
#else
	for(i=1 ; i <= nparts ; i++) {
#endif
		j = 0 ;
		next = pheader[i].next ;
		while(next) {
			next = next->link ;
			j++ ;
		}

#ifdef DYNAMIC_PARTS
		// if dynamic partitions, free ones will be linked together ...
		// therefore, the above counting of links will not give the number
		// of blocks in a dynamic partition if it is free since free ones
		// have no blocks, so adjust count if necessary
		if (i > nparts)
			if (pheader[i].size == 0)
				j = 0 ;
#endif

		printf("%3d %8s %5d/%5u %5u %5u %6u",
			   i, xpartname(i), j, pheader[i].count,
			   pheader[i].worst, pheader[i].usage, pheader[i].size) ;

#ifdef PARTITION_WAITERS
		nexttask = pheader[i].waiters ;
		while(nexttask != NULLTCB) {
			printf(" %8s", xtaskname(nexttask->task)) ;
			nexttask = nexttask->flink ;
		}
#endif

		printf("\n") ;
	}
}
#endif

#ifdef HAS_MAILBOXES
static void cbugmbox(void)
{
	int i, j ;
	MHEADER *pmh ;
	RTXCMSG *next ;
#ifdef MAILBOX_WAITERS
	TCB *nexttask ;
#endif

	printf("** Mailbox Snapshot **\n") ;
	printf("  #   Name   Current Count Waiters\n") ;
		 // 123 12345678  12345  12345 12345678 12345678

	for(i=1, pmh = &mheader[1] ; i <= nmboxes ; i++,   pmh++) {
		j = 0 ;
		// count # messages in list
		next = pmh->link ;
		while(next != NULL) {
			next = next->link ;
			j++ ;
		}

		printf("%3d %8s  %5u  %5u", i,
				&mboxname[i][0], j, mheader[i].count) ;

#ifdef MAILBOX_WAITERS
		nexttask = pmh->waiters;
		while(nexttask != NULLTCB) {
			printf(" %8s", xtaskname(nexttask->task)) ;
			nexttask = nexttask->flink ;
		}
#endif

		printf("\n") ;
	}
}
#endif

static void cbugclock(void)
{
	CLKBLK *next ;
	long time, m ;

	printf("** Clock Snapshot **\n") ;
//  printf("MCFR=0x%08x\n", AT91C_BASE_PMC->PMC_MCFR) ;

	printf("Clock is %ld Hz, Tick is %d Hz period %d ms", current_clock, CLKRATE, CLKTICK) ;
#ifdef HAS_ALLOC_TIMER /* { */
	printf(", Maximum of %d timers", ntmrs) ;
#endif /* } HAS_ALLOC_TIMER */
	printf("\nrtctick is %d", rtctick) ;
	printf(", rtctime is %lu\n\n", KS_inqtime()) ;

	printf("   Time        Cyclic   Task   Timer      Object\n") ;
	printf(" Remaining      Value   Name   Type        Name\n") ;
		 // 1234567890 1234567890 12345678 xxxxxxxxxx xxxxxxxx

	time = 0 ;
	next = clkqptr ;

	while(next != NULLCLK) {        // walk timer list
		time = next->remain - rtctick ;

//        time *= clktick ;
		time = (time * 1000) / CLKRATE ;

//        m = (long)(next->recycle * clktick) ;
		m = (long)(next->recycle * 1000) / CLKRATE ;
		printf("%10ld %10ld %8s ", time, m, xtaskname(next->task)) ;

		switch(next->objtype) {
		case TIMER_OBJ :
			printf("Timer      %8s", xsemaname((int)(next->objid))) ;
			break ;

		case DELAY_OBJ :
			printf("Delay      %8s", xtaskname((int)(next->objid))) ;
			break ;

#ifdef SEMAPHORE_TIMEOUTS
		case SEMAPHORE_OBJ :
			printf("Semaphore  %8s", xsemaname((int)(next->objid))) ;
			break ;
#endif

#ifdef MAILBOX_TIMEOUTS
		case MAILBOX_OBJ :
			printf("Mailbox    %8s", &mboxname[next->objid][0]) ;
			break ;
#endif

#ifdef PARTITION_TIMEOUTS
		case PARTITION_OBJ :
			printf("Partition  %8s", xpartname(next->objid)) ;
			break ;
#endif

#ifdef QUEUE_TIMEOUTS
		case QUEUE_OBJ :
			printf("Queue") ;
			if (qheader[(int)(next->objid)].curndx == 0)
				printf("Empty") ;
			else
				printf("Full ") ;
			printf(" %8s", xqueuename((int)(next->objid))) ;
			break ;
#endif

#ifdef RESOURCE_TIMEOUTS
		case RESOURCE_OBJ :
			printf("Resource   %8s", xresname((int)(next->objid))) ;
			break ;
#endif

		default :
			printf("Unknown timer type") ;
			break ;
		}
		next = next->flink ;

		printf("\n") ;
	}
}

// zero all statistics
static void cbugzero(void)
{
	int i ;

	com0err = com1err = com2err = com3err = 0 ;
#ifdef USE_COM4_ON_ARM
    com4err = 0 ;
#endif // USE_COM4_ON_ARM

#ifdef USE_USB_ON_ARM
	usberr = 0 ;
#endif // USE_USB_ON_ARM

#ifdef HAS_PARTITIONS
#ifdef DYNAMIC_PARTS
	for(i=1 ; i <= nparts + dnparts ; i++)
#else
	for(i=1 ; i <= nparts ; i++)
#endif
		pheader[i].usage = pheader[i].worst = 0 ;
#endif

#ifdef HAS_QUEUES
	for(i=1 ; i <= nqueues ; i++)
		qheader[i].count = qheader[i].worst = 0 ;
#endif

#ifdef HAS_RESOURCES
	for(i=1 ; i <= nres ; i++)
		rheader[i].count = rheader[i].conflict = 0 ;
#endif

#ifdef HAS_MAILBOXES
	for(i=1 ; i <= nmboxes ; i++)
		mheader[i].count = 0 ;
#endif

	isrmax = 0 ;

	printf("\n") ;      // clear console line
}

#ifdef TASKDETAIL
static void cbugregs(void)
{
	TASK task ;
	FRAME *frame ;

	if ( (task = gettask()) != 0 ) {
		if (task == RTXCBUG) {
			printf("Meaningless\n") ;
			return ;
		}

		if ( (frame = rtxtcb[task].sp) == NULL) {
			printf("Undefined\n") ;
			return ;
		}

		printf("** Task Register Snapshot **\n") ;

		printf(" PC=%04x",    frame->cpupc) ;
		printf(" SP=%04x",    *((unsigned int *)&frame+0)) ;

		// Set all AVR desired registers if TASKDETAIL defined
		printf(" CCR=%02x\n", (unsigned int)(frame->ccr)) ;
		printf(" R0=%02x",    (unsigned int)(frame->r0)) ;
		printf(" R1=%02x",    (unsigned int)(frame->r1)) ;
		// and so on ...
	}
	else
		printf("Undefined\n") ;
}
#endif        // TASKDETAIL

#ifdef STKLOGIC
int UsedStackEval(unsigned long *sstart, int ssize)
{
   int i ;

   for(i=0 ; i<(ssize/4) ; i++)
	   if (sstart[i] != Global_STACK_FILLER)
		   break ;

	return(ssize - (i*4)) ;
}

void cbugstack(void){
    int i ;
    SEMA *psema ;
    int lr ;
    unsigned long used, ssize ;
    unsigned long sstart ;
    extern unsigned long kernellastrunstack ;

    printf("** Stack Snapshot **\n") ;
    printf("  #   Task       Addr   Size  Used  Spare Last\n") ;
       //   123 12345678  123456  1234  1234   1234  xx

// USER tasks stack
#ifdef DYNAMIC_TASKS
    for(i=1 ; i <= ntasks + dntasks ; i++) {
#else
    for(i=1 ; i <= ntasks ; i++) {
#endif
        ssize = rtxkktcb[i].stacksize ;
        used = UsedStackEval((unsigned long *)(rtxkktcb[i].stackbase), ssize) ;
        lr = (rtxtcb[i].lastrunstack == Global_STACK_FILLER) ;

        printf("%3d %8s  %08lx%6lu%6lu%7lu  %s\n", i, xtaskname(i),
                                (unsigned long)rtxkktcb[i].stackbase,
                                ssize, used, ssize - used,
                                (lr ? "ok" : "ERR")) ;
    }

// MAIN task stack
#if defined(USE_LPC17XX) || defined(USE_LPC1788) || defined(USE_AT91SAM3S4)
    sstart = (unsigned long)(&__kernel_stack__end) + 4 ;
    ssize = (unsigned long)(&_vStackTop) - sstart ;
#endif // defined(USE_LPC17XX) || defined(USE_LPC1788) || defined(USE_AT91SAM3S4)
#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    ssize = (unsigned long)(ram_end) - (unsigned long)(__noinit_end)
                                - Global_IRQ_STACK_SIZE
                                - Global_SVC_STACK_SIZE
                                - Global_SYSKERNEL_STACK_SIZE ;
    sstart = (unsigned long)(__noinit_end) ;
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    used = UsedStackEval((unsigned long *)(sstart), ssize) ;
    printf("Main task     %06lx%6lu%6lu%7lu\n", sstart, ssize, used, ssize - used) ;

// KERNEL stack

#if defined(USE_LPC17XX) || defined(USE_LPC1788) || defined(USE_AT91SAM3S4)
    sstart = (unsigned long)(&__kernel_stack_start) ;
    ssize = (unsigned long)(&__kernel_stack__end) - (unsigned long)(&__kernel_stack_start) ;
#endif // defined(USE_LPC17XX) || defined(USE_LPC1788) || defined(USE_AT91SAM3S4)
#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    ssize = Global_SYSKERNEL_STACK_SIZE ;
    sstart = (unsigned long)(ram_end)
                                - Global_IRQ_STACK_SIZE
                                - Global_SVC_STACK_SIZE
                                - Global_SYSKERNEL_STACK_SIZE ;
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    used = UsedStackEval((unsigned long *)(sstart), ssize) ;
    lr = (kernellastrunstack == Global_STACK_FILLER) ;
    printf("RTXC Kernel   %06lx%6lu%6lu%7lu  %s\n", sstart, ssize, used, ssize - used,
                                (lr ? "ok" : "ERR")) ;

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
// SVC stack

    ssize = Global_SVC_STACK_SIZE ;
    sstart = (unsigned long)(ram_end)
                                - Global_IRQ_STACK_SIZE
                                - Global_SVC_STACK_SIZE ;
    used = UsedStackEval((unsigned long *)(sstart), ssize) ;
    printf("SWI stack     %06lx%6lu%6lu%7lu\n", sstart, ssize, used, ssize - used) ;

// IRQ stack

    ssize = Global_IRQ_STACK_SIZE ;
    sstart = (unsigned long)(ram_end)
                                - Global_IRQ_STACK_SIZE ;
    used = UsedStackEval((unsigned long *)(sstart), ssize) ;
    printf("IRQ stack     %06lx%6lu%6lu%7lu\n", sstart, ssize, used, ssize - used) ;
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

// interrupt nexting
    printf("\nWorst case interrupt nesting = %d\n", (int)(isrmax)) ;

// first zero semaphore marks the End of "used" area
    for (i = 0, psema = &siglist[0]; *psema; i++, psema++)
                ;

    printf("Worst case signal list size  = %d\n", i) ;

    printf("RxErrors=%6u%6u%6u%6u", com0err, com1err, com2err, com3err) ;
#ifdef USE_COM4_ON_ARM
    printf("%6u", com4err) ;
#endif // USE_COM4_ON_ARM
#ifdef USE_USB_ON_ARM
    printf("%6u", usberr) ;
#endif // USE_USB_ON_ARM
    putchar('\n') ;
}
#endif

#ifdef USEHELP
void menurtxc(void)
{
	printf("\n") ;

	printf("T - Tasks\n") ;
#ifdef HAS_MAILBOXES
	printf("M - Mailboxes\n") ;
#endif
#ifdef HAS_PARTITIONS
	printf("P - Partitions\n") ;
#endif
#ifdef HAS_QUEUES
	printf("Q - Queues\n") ;
#endif
#ifdef HAS_RESOURCES
	printf("R - Resources\n") ;
#endif
	printf("S - Semaphores\n") ;
	printf("C - Clock / Timers\n") ;
#ifdef STKLOGIC
	printf("K - Stack Limits\n") ;
#endif
#if defined(HAS_PARTITIONS) || \
	defined(HAS_QUEUES) || \
	defined(HAS_RESOURCES)
	printf("Z - Zero Partition/Queue/Resource Statistics\n") ;
#endif
	printf("$ - Enter Task Manager Mode\n");
	printf("# - Task Registers\n") ;
	printf("L - Load & Run @0x7000\n") ;
	printf("O - Off\n") ;
	printf("E - EEPROM\n") ;
	printf("F - FLASH\n") ;
	printf("W - Watch\n") ;
	printf("D - Dump\n") ;
	printf("H - Help\n") ;
	printf("G - Exit RTXCbug\n") ;
}
#endif

#ifdef M2102C
static void dbugacc(char * param)
{
	int i, j ;

	while(*param == ' ')        // skip blank
		param++ ;

	if (*param) {                // if parameter
		unsigned char cc, par30, par32 ;
		unsigned char buf[16] ;


		par30 = (unsigned char)(atoi(param)) ; // get parameter number

		while(*param++ != ' ' ) ;       // search next blank
		while(*param == ' ')            // skip blank
			param++ ;

		par32 = (unsigned char)(atoi(param)) ;

		// configure
		buf[0] = 0x30 ; // register index
		buf[1] = par30 ; // register value
		TWI_send(0x30, buf, 2) ;

		// configure
		buf[0] = 0x32 ; // register index
		buf[1] = par32 ; // register value
		TWI_send(0x30, buf, 2) ;

		// configure
		buf[0] = 0x33 ; // register index
		buf[1] = 0x14 ; // register value (20)
		TWI_send(0x30, buf, 2) ;

		// configure
		buf[0] = 0x22 ; // register index
		buf[1] = 0x2D ; // register value (45)
		TWI_send(0x30, buf, 2) ;

		// Enable interrupt
		GPIOINT->IO0IntEnR |= ((1<<8)|(1<<9)) ;      // enable interrupt from Raising
		GPIOINT->IO0IntEnF |= ((1<<8)|(1<<9)) ;      // enable interrupt from Raising

		// read values
		moved_flag=0 ;
		acc_flag=0 ;
		j = 0 ;
			printf("Setted 30=%d 32=%d\n",  par30, par32) ;
		for(;;){

			TWI_txrx(0x30, 0x31, buf, 1) ;

			printf("%d %d 0x%x\n",  moved_flag, acc_flag, buf[0]) ;
			if (j!=acc_flag){
				buf[0] = 0x30 ; // register index
				buf[1] = par30 ; // register value
				TWI_send(0x30, buf, 2) ;
				j = acc_flag ;
			}
			// Any key to exit
			if (KS_dequeue(MONITIPORT, &cc)==RC_GOOD) break ;
			for(i=0;i<300; i++)	tickwait(1000) ;
		}
	}

}
#endif

#ifdef M2023
static void KillMts(void)
{
int i ;

    DISABLE ; 

	while(1){
		GPIO3->FIOSET = (1<<26) ;
		for(i=0;i<4; i++)	tickwait(120) ; // 1 kHz
		//for(i=0;i<4; i++)	tickwait(180) ; // 600 Hz
		GPIO3->FIOCLR = (1<<26) ;
		for(i=0;i<4; i++)	tickwait(120) ;
	}
}
#endif

#ifdef M2102C
static void dsetvibra(char * param) {
	//unsigned int i ;
	unsigned char parport, parnum, parval ;

	while(*param == ' ')        // skip blank
		param++ ;

	if (*param) {                // if parameter

		parport = (unsigned char)(atoi(param)) + 20 ; // vibra bit


		while(*param++ != ' ' ) ;       // search next blank
		while(*param == ' ')            // skip blank
			param++ ;

		parnum = (unsigned char)(atoi(param)) ; // vibra type 0=out, 1=in
		if (parnum)
			GPIO1->FIODIR &= ~(1<<parport) ; // As Input
		else
			GPIO1->FIODIR |=  (1<<parport) ; // As output
		
		while(*param++ != ' ' ) ;       // search next blank
		while(*param == ' ')            // skip blank
			param++ ;

		parval = (unsigned char)(atoi(param)) ; // vibra pullup 0=pdown, 1=pup
		switch(parval){
			case 0: // pdown : 11
				PINCON->PINMODE1 |= (11<<((parport-16)*2)) ;
				break ;
			case 1: // pup   : 00
				PINCON->PINMODE1 &= (~11<<((parport-16)*2)) ;
				break ;
			case 2: // free  : 10
				PINCON->PINMODE1 &= (~10<<((parport-16)*2)) ;
				break ;
		}
		PINCON->PINMODE1 &= (~11<<((parport-16)*2)) ;
		
		
	}
}
#endif

#if defined(USE_TW1_FAST_ACCELEROMETER) || defined(USE_SPI_FAST_ACCELEROMETER)
#define FAST_ACC_BUFFER_SIZE   (400*6)          // 1 second buffer for 'short int' 3 axis at 400 Hz
extern unsigned char fastaccbuf[FAST_ACC_BUFFER_SIZE] ;
#endif // USE_TW1_FAST_ACCELEROMETER


#ifdef GEMHH
#ifdef DEBUG_HW

extern unsigned char ext_pot ;
extern unsigned short read_time ;
// a <pot> <sec>
static int outctrl(char * param){
unsigned char parpot ;
unsigned char buf[3] ;

	if (!param) return(0) ;
	while(*param == ' ')        // skip blank
		param++ ;
	if (!param) return(0) ;
	
	parpot = (unsigned char)(atoi(param)) ; // get parameter number
	ext_pot = parpot & 0x7f ;
	#warning INCLUDED
	dio_write(PORT_PIO1, (1<<10), 0 ) ;
	
	// Read reg.2 if not 128 set at 128 before change reg.0
	buf[0] = 0xff ;
	TWI1_txrx(0x50, 0x2, buf, 1 ) ;
	if (buf[0]!=0x80){
		buf[0] = 0x02 ;
		buf[1] = 0x80 ;
		TWI1_send(0x50, buf, 2 ) ;
	}
	// Read for verify
	buf[0] = 0xff ;
	TWI1_txrx(0x50, 0x100, buf, 1 ) ;
	printf("Pot is %d\n", buf[0]) ;

	buf[0] = 0x00 ;
	buf[1] = ext_pot ;
	printf("Set Pot to %d", buf[1] ) ;

	TWI1_send(0x50, buf, 2 ) ;

	// Read for verify
	buf[0] = 0xff ;
	TWI1_txrx(0x50, 0x100, buf, 1 ) ;
	printf(" ->(%d) %d\n", ext_pot, buf[0]) ;

	while(*param != ' '){        // skip START
		param++ ;
		if (!param) return(0) ;
	}
	
	while(*param == ' ')        // skip blank
		param++ ;
	if (!param) return(0) ;
	
	read_time = (atoi(param)) ; // get parameter number
	if (read_time<=0) return(0) ;
	
	read_time *= 10 ;
	
	return(1) ;
}
#endif // DEBUG_HW

#ifdef USE_FREQ_T2MAT2
static void dbgbeep(char * param)
{
short i, n, opt ;

	if (!(*param)) return ;
		
	while(*param != ' ')        // skip START
				param++ ;
				
	while(*param == ' ')        // skip blank
				param++ ;

	opt = atoi(param) ;

		while(*param != ' ')        // skip START
				param++ ;
				
	while(*param == ' ')        // skip blank
				param++ ;
	n = atoi(param) ;
	if ((n<1) || (n>20)) n = 1 ;
	
	dio_beeper(opt) ;
	for( i=0;i<(n*100); i++) 	tickwait(1000) ;
	dio_beeper(0) ;
}
#endif // USE_FREQ_T2MAT2

extern void SetIO(short num, short val) ;
static void dbgLED(char * param)
{
short opt ;

	if (!(*param)){
		return ;
	}
		
	while(*param != ' ')        // skip START
				param++ ;
				
	while(*param == ' ')        // skip blank
				param++ ;

	opt = strtol(param, NULL, 0) ;

	SetIO(DOUT_LED234, opt ) ;
	SetIO(DOUT_LED1, opt ) ;
	
}
#endif // GEMHH


static void cddump(char * param)
{
int i ;
unsigned char parport ;

	parport = 0 ;
	
	while(*param == ' ')        // skip blank
		param++ ;

	if (*param) {                // if parameter
		unsigned char parnum, parval ;

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512) || defined(USE_AT91SAM3S4)
		parport = toupper(*param) - 'A' ;       // get port type
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512) || defined(USE_AT91SAM3S4)
#if defined(USE_LPC17XX) || defined(USE_LPC1788)
		parport = toupper((int)*param) - '0' ;       // get port type
#endif // defined(USE_LPC17XX) || defined(USE_LPC1788)

#if defined(USE_TW1_FAST_ACCELEROMETER) || defined(USE_SPI_FAST_ACCELEROMETER)
		if (parport==9){
			int j ;
			for(i=0;i<400;i++){
				printf("i%d %02x%02x %02x%02x %02x%02x\n", i, 
					   fastaccbuf[0+j], fastaccbuf[1+j],
					   fastaccbuf[2+j], fastaccbuf[3+j],
					   fastaccbuf[4+j], fastaccbuf[5+j] ) ;
				j+= 6 ;
			}
			return ;
		}
#endif
		while(*param++ != ' ' ) ;       // search next blank
		while(*param == ' ')            // skip blank
			param++ ;

		parnum = (unsigned char)(atoi(param)) ; // get parameter number

#if defined(GEMHH) // && defined(DEBUG_HW))
		if (parport==9){
		// Set Potentiometer
		// parnum: 0-127
			unsigned char buf[3] ;
			
			dio_write(PORT_PIO1, (1<<10), 0 ) ;
			
			for(i=0;i<300;i++) tickwait(1000) ;
			
			// Read reg.2 if not 128 set at 128 before change reg.0
			buf[0] = 0xff ;
			TWI1_txrx(0x50, 0x2, buf, 1 ) ;
			if (buf[0]!=0x80){
				buf[0] = 0x02 ;
				buf[1] = 0x80 ;
				TWI1_send(0x50, buf, 2 ) ;
			}
			// Read for verify
			buf[0] = 0xff ;
			TWI1_txrx(0x50, 0x100, buf, 1 ) ;
			printf("Pot is %d\n", buf[0]) ;

			buf[0] = 0x00 ;
			buf[1] = (parnum & 0x7f) ;
			printf("Set Pot to %d", buf[1] ) ;

			TWI1_send(0x50, buf, 2 ) ;

			// Read for verify
			buf[0] = 0xff ;
			TWI1_txrx(0x50, 0x100, buf, 1 ) ;
			printf(" ->(%d) %d\n", parnum, buf[0]) ;
			
			dio_write(PORT_PIO1, (1<<10), 1 ) ;
		}
#if defined(DEBUG_HW)
		else if (parport==8){
			extern unsigned short ext_ampl ;
			
			ext_ampl = (unsigned short)(atol(param)) ; // get parameter number
			printf("Set Ampl to %u\n", ext_ampl ) ;
		}else if (parport==7){
			extern long ext_freq ;
			
			ext_freq = (atol(param)) ; // get parameter number
			printf("Set freq to %ld\n", ext_freq ) ;
		}
#endif
		else{
#endif // (defined(GEMHH) && defined(DEBUG_HW))
		while(*param++ != ' ' ) ;       // search next blank
		while(*param == ' ')            // skip blank
			param++ ;

		parval = (unsigned char)(atoi(param)) ;

#if defined(GEMHH)
		}
#endif
#if ( defined(GEMHH) && defined(DEBUG_HW))
		if (parport<7){
#else  // ( defined(GEMHH) && defined(DEBUG_HW))
		if (parport<9){
#endif // (defined(GEMHH) && defined(DEBUG_HW))
			dio_write(parport, (1<<parnum), (parval<<parnum)) ;
		}
// Prova x NewTestKit
// 		else{
// 			if (parval)
// 				AT91C_BASE_PIOB->PIO_OER = AT91C_PIO_PB14 ; // Output Enable Register
// 			else
// 				AT91C_BASE_PIOB->PIO_ODR =  AT91C_PIO_PB14 ; // Output Disable Register
// 		}
	}

	// read analog inputs from ADC -0-
#ifdef USE_ADC_MUX_ON_ARM
	for(i=0 ; i<6 ; i++) {
#else
	for(i=0 ; i<8 ; i++) {
#endif
		if (i) printf(", ") ;
#if defined(USE_FAST_AD_T0)
		mADusemax = USE_MEAN ;
#endif
		printf("Ain%d=%d", i, ADC_read(i)) ;
	}
	printf("\n") ;

#ifdef USE_ADC_MUX_ON_ARM
	// read analog MUX inputs from ADC -0-
	for(i=0 ; i<8 ; i++) {
		if (i) printf(", ") ;
		printf("Amux%d=%d", i, ADC_read(8+i)) ;
	}
	printf("\n") ;
#endif // USE_ADC_MUX_ON_ARM


#ifdef USE_ADC_FAST_ON_ARM
	// read analog inputs from ADC -1-
	for(i=0 ; i<12 ; i+=3) {
		extern unsigned short adc1buff[] ;
		if (i) printf(" - ") ;
		printf("%d %d %d", adc1buff[i+0], adc1buff[i+1], adc1buff[i+2]) ;
	}
	printf("\n") ;
#else // USE_ADC_FAST_ON_ARM
#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
	// read analog inputs from ADC -0- and -1-
	for(i=0 ; i<3 ; i++) {
		if (i) printf(", ") ;
		printf("Axyz%d=%d", i, ADC_read(16+i)) ;
	}
	printf("\n") ;
#endif //defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_ACCEL)
        {
            unsigned char buf[16] ;

			buf[0] = 0  ;
#if (defined(USE_LPC17XX) || defined(USE_LPC1788))
#ifdef USE_TWI_ACCELEROMETER_MMA8451
			short ax, ay, az ;
			TWIACC_TXRX(0x38, 0x1, buf, 6);
			ax = (buf[0]<<8) + buf[1] ;
			ay = (buf[2]<<8) + buf[3] ;
			az = (buf[4]<<8) + buf[5] ;
				//printf("ACC-MMA8451: x=%04hx, y=%04hx, z=%04hx\n", *((short *)(&buf[0])), *((short *)(&buf[2])), *((short *)(&buf[4]))) ;
				//printf("ACC-MMA8451: x=%d, y=%d, z=%d\n", (*((short *)(&buf[0])))>>2, (*((short *)(&buf[2])))>>2, (*((short *)(&buf[4])))>>2 ) ;
			printf("ACC-MMA8451: x=%d(0x%x), y=%d(0x%x), z=%d(0x%x)\n",  ax/4,ax, ay/4, ay, az/4, az) ;
// 	        {
// 				int am ;
// 				am = 0 ;
// 				
//                 printf("abc: %d\n", (am/10)) ;
// 	        }
#endif
#if defined(USE_TWI_ACCELEROMETER_LIS3DH) || defined(USE_SPI_ACCELEROMETER_LIS3DH)
#if defined(USE_TWI_ACCELEROMETER_LIS3DH)
			TWI_txrx(0x30, 0x80 | 0x28, &buf[8], 6) ;
#endif
#if defined(USE_SPI_ACCELEROMETER_LIS3DH)
			buf[0] = 0x0f | 0x80 ; // addr & READ
			//		  buftx   bufrx
			SPI_rtx2(&buf[0], &buf[2], 2, NULL, NULL, 0) ;
		
			buf[0] = 0x28 | 0xC0 ; // addr & READ & ADDR auto incr
			//buf[0] = 0x0A | 0xC0 ; // addr & READ & ADDR auto incr
			//		  buftx   bufrx
			SPI_rtx2(&buf[0], &buf[7], 7, NULL, NULL, 0) ;
#endif
			printf("ACC LIS3DH(%x): x=%d, y=%d, z=%d\n", buf[3], *((short *)(&buf[8])), *((short *)(&buf[10])), *((short *)(&buf[12]))) ;
// 			printf("ACCd: 0x%0hx-0x%0hx, 0x%0hx-0x%0hx, 0x%0hx-0x%0hx\n", 
// 				   buf[0], buf[1], buf[2], buf[3],buf[4], buf[5]) ;
//         {
//                 extern volatile int moved_flag ;
// 				
// //        		TWIACC_TXRX(0x30, 0x31, buf, 1) ;
//                 printf("MOVED: %d (0x%0x)\n", moved_flag, buf[0]) ;
// 				// for debug
// 				
//         }
#endif // USE_TWI_ACCELEROMETER_LIS3DH
#ifdef REAL_GYRO
// 		{
//     		volatile uint32_t * const iocon_base = ((uint32_t *)(LPC_IOCON_BASE)) ;
// 			
// 			for(i=6;i<10;i++){
// 				printf("IOCON%d 0x%lx\n", i, (iocon_base[i] & 0x1ffff)  ) ;
// 			}
// 			for(i=15;i<19;i++){
// 				printf("IOCON%d 0x%lx\n", i, (iocon_base[i] & 0x1ffff)  ) ;
// 			}
// 		}
		buf[0] = 0x0f | 0x80 ; // addr & READ
		buf[1] = 0xbd ;
		SPI1_rtx2(&buf[0], &buf[8], 2, NULL, NULL, 0) ;
		printf("GYRO-L3GD20: id=0x%x\n", buf[9]) ; // Waited 0xD4
		buf[0] = 0x20 | 0x80 ; // addr & READ
		buf[1] = 0x0f ;
		SPI1_rtx2(&buf[0], &buf[8], 2, NULL, NULL, 0) ;
		printf("GYRO-L3GD20: REG1=0x%x\n", buf[9]) ; 
		buf[0] = 0x26 | 0x80 ; // addr & READ
		SPI1_rtx2(&buf[0], &buf[8], 2, NULL, NULL, 0) ;
		printf("GYRO-L3GD20: temp=0x%x\n", buf[9]) ; 
		
// 		{
// 			unsigned long aaaSS[MAX_SSIZE/sizeof(unsigned long)] ;
// 			
// 			EKS_VirtualRAM_Read( &aaaSS, MAX_SSIZE) ;
// 		}
#endif // REAL_GYRO
// #else // ((defined(USE_LPC17XX) || defined(USE_LPC1788))
// 		// configure and read TWI accelerometer LIS331LDH
//                 // configure (just to be sure)
//                 // buf[0] = 0x20 ; // register index
//                 // buf[1] = 0x27 ; // register value
//                 // TWI_send(0x30, buf, 2) ;
//                 // multi read
// 				
// 			TWI_txrx(0x30, 0x80 | 0x28, buf, 6) ;
// 			
// 			printf("ACC: x=%d, y=%d, z=%d\n", *((short *)(&buf[0])), *((short *)(&buf[2])), *((short *)(&buf[4]))) ;
// 			{
// 					extern volatile int moved_flag ;
// 					int am ;
// 					am = 0 ;
// 					
// 					printf("MOVED: %d (%d)\n", moved_flag, (am/10)) ;
// 					// for debug
// 					
// 			}
#endif // ((defined(USE_LPC17XX) || defined(USE_LPC1788))
		}
#endif// USE_ACCEL
#endif // USE_ADC_FAST_ON_ARM
	// read digital inputs

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512) || defined(USE_AT91SAM3S4)
	printf("PIOA: 0x%08lx"
#if defined(USE_AT91SAM7S256)
			" (O=0x%08x,P=0x%08x,E=0x%08x)"
#endif
#if defined(USE_AT91SAM7A3)
	       ", PIOB: 0x%08lx"
#endif // defined(USE_AT91SAM7A3)
	       "\n",
	        dio_read(PORT_PIOA)
#if defined(USE_AT91SAM7S256)
			, AT91C_BASE_PIOA->PIO_OSR, AT91C_BASE_PIOA->PIO_PSR, AT91C_BASE_PIOA->PIO_OWSR

#endif
#if defined(USE_AT91SAM7A3)
                , dio_read(PORT_PIOB)
#endif // defined(USE_AT91SAM7A3)
              ) ;

	printf("PIOAp:0x%08x"
#if defined(USE_AT91SAM7A3)
	       ", PIOBp:0x%08x"
#endif // defined(USE_AT91SAM7A3)
	       "\n",
#if defined(USE_AT91SAM3S4)
                (unsigned int)(AT91C_BASE_PIOA->PIO_PUSR)
#else // defined(USE_AT91SAM3S4)
                AT91C_BASE_PIOA->PIO_PPUSR
#endif // defined(USE_AT91SAM3S4)
#if defined(USE_AT91SAM7A3)
                , AT91C_BASE_PIOB->PIO_PPUSR
#endif // defined(USE_AT91SAM7A3)
              ) ;

#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512) || defined(USE_AT91SAM3S4)

#if defined(USE_LPC17XX)
	printf("PIO0:0x%08lx ", GPIO0->FIOPIN) ;
	printf("PIO1:0x%08lx ", GPIO1->FIOPIN) ;
	printf("PIO2:0x%08lx ", GPIO2->FIOPIN) ;
	printf("PIO3:0x%08lx ", GPIO3->FIOPIN) ;
	printf("PIO4:0x%08lx\n", GPIO4->FIOPIN) ;

// Direction
	printf("0dir:0x%08lx ", GPIO0->FIODIR) ;
	printf("1dir:0x%08lx ", GPIO1->FIODIR) ;
	printf("2dir:0x%08lx ", GPIO2->FIODIR) ;
	printf("3dir:0x%08lx ", GPIO3->FIODIR) ;
	printf("4dir:0x%08lx\n", GPIO4->FIODIR) ;

// Pull
	printf("0fun:0x%08lx ",  PINCON->PINMODE0) ;
	printf("2fun:0x%08lx ",  PINCON->PINMODE2) ;
	printf("4fun:0x%08lx ",  PINCON->PINMODE4) ;
	printf("6fun:0x%08lx ",  PINCON->PINMODE6) ;
	printf("8fun:0x%08lx\n", PINCON->PINMODE8) ;
	printf("1fun:0x%08lx ",  PINCON->PINMODE1) ;
	printf("3fun:0x%08lx ",  PINCON->PINMODE3) ;
	printf("5fun:0x%08lx ",  PINCON->PINMODE5) ;
	printf("7fun:0x%08lx ",  PINCON->PINMODE7) ;
	printf("9fun:0x%08lx\n", PINCON->PINMODE9) ;

// Function
	printf("P0f0:0x%08lx ", PINCON->PINSEL0) ;
	printf("P1f0:0x%08lx ", PINCON->PINSEL2) ;
	printf("P2f0:0x%08lx ", PINCON->PINSEL4) ;
	printf("P3f0:0x%08lx ", PINCON->PINSEL6) ;
	printf("P4f0:0x%08lx\n", PINCON->PINSEL8) ;
	
	printf("P0f1:0x%08lx ", PINCON->PINSEL1) ;
	printf("P1f1:0x%08lx ", PINCON->PINSEL3) ;
	printf("P2f1:0x%08lx ", PINCON->PINSEL5) ;
	printf("P3f1:0x%08lx ", PINCON->PINSEL7) ;
	printf("P4f1:0x%08lx\n", PINCON->PINSEL9);
	//printf("GPREG0:0x%08lx ", RTC->GPREG0) ;
	//printf("GPREG1:0x%08lx\n", RTC->GPREG1) ;
	//RTC->GPREG0 = 0x55aa55aa ;
	//RTC->GPREG1 = 0x55aa55aa ;
#endif // defined(USE_LPC17XX)

#if defined(USE_LPC1788)
    printf("PIO0:0x%08lx ", LPC_GPIO0->PIN) ;
    printf("PIO1:0x%08lx ", LPC_GPIO1->PIN) ;
    printf("PIO2:0x%08lx ", LPC_GPIO2->PIN) ;
    printf("PIO3:0x%08lx ", LPC_GPIO3->PIN) ;
    printf("PIO4:0x%08lx ", LPC_GPIO4->PIN) ;
    printf("PIO5:0x%08lx\n", LPC_GPIO5->PIN) ;
    printf("PIm0:0x%08lx ", LPC_GPIO0->MASK) ;
    printf("PIm1:0x%08lx ", LPC_GPIO1->MASK) ;
    printf("PIm2:0x%08lx ", LPC_GPIO2->MASK) ;
    printf("PIm3:0x%08lx ", LPC_GPIO3->MASK) ;
    printf("PIm4:0x%08lx ", LPC_GPIO4->MASK) ;
    printf("PIm5:0x%08lx\n", LPC_GPIO5->MASK) ;
	
	{
    	volatile uint32_t * const iocon_base = ((uint32_t *)(LPC_IOCON_BASE)) ;
	    volatile uint32_t * const iodir_base = ((uint32_t *)(LPC_GPIO0_BASE)) ;
		int port,pin ;
		port = 0 ; pin = 10 ;
		printf("Func p%d.%d: 0x%lx, d 0x%lx\n", port, pin, iocon_base[ 32*port + pin ], iodir_base[port * 0x8] ) ;
		port = 0 ; pin = 11 ;
		printf("Func p%d.%d: 0x%lx, d 0x%lx\n", port, pin, iocon_base[ 32*port + pin ], iodir_base[port * 0x8] ) ;
	}
#endif // defined(USE_LPC1788)

#ifdef USE_PCAL9555A_TW1_
	{
		unsigned char buf[16] ;

		buf[0] = 0x4a ;
		TWI_txrx(PCAL9555A_ADDR, 0x4A, &buf[1], 2) ;
		TWI_txrx(PCAL9555A_ADDR, 0x4C, &buf[3], 2) ;
		printf("Wakeup1: id=0x%02x%02x 0x%02x%02x\n", buf[2], buf[1], buf[4], buf[3] ) ;
		//TWI_send(PCAL9555A_ADDR, buf, 3) ;
		TWI_txrx(PCAL9555A_ADDR2, 0x4A, &buf[1], 2) ;
		TWI_txrx(PCAL9555A_ADDR2, 0x4C, &buf[3], 2) ;
		printf("Wakeup2: id=0x%02x%02x 0x%02x%02x\n", buf[2], buf[1], buf[4], buf[3] ) ;
		//TWI_send(PCAL9555A_ADDR2, buf, 3) ;
		TWI_txrx(PCAL9555A_ADDR3, 0x4A, &buf[1], 3) ;
		TWI_txrx(PCAL9555A_ADDR3, 0x4C, &buf[3], 2) ;
		printf("Wakeup3: id=0x%02x%02x 0x%02x%02x\n", buf[2], buf[1], buf[4], buf[3] ) ;
		
		buf[1] = 0 ;
		buf[2] = 0 ;
		TWI_send(PCAL9555A_ADDR, buf, 3) ;
		
		buf[1] = (0xff & (~0x3)) ;
		buf[2] = (0xff & (~0x2)) ;
		TWI_send(PCAL9555A_ADDR3, buf, 3) ;
		
		if (parport==7){		// Enable latch on all
			buf[0] = 0x44 ;
			buf[1] = 0xff ;
			buf[2] = 0xff ;
			TWI_send(PCAL9555A_ADDR, buf, 3) ;
			printf("Latch1: 0x%02x%02x\n", buf[2], buf[1] ) ;
		}
		
		if (parport==9){		// Enable latch on all
			buf[0] = 0x44 ;
			buf[1] = 0x3 ;
			buf[2] = 0x2 ;
			TWI_send(PCAL9555A_ADDR3, buf, 3) ;
			printf("Latch3: 0x%02x%02x\n", buf[2], buf[1] ) ;
		}
// 		else{					// Disable latch on all
// 			buf[0] = 0x44 ;
// 			buf[1] = 0x0 ;
// 			buf[2] = 0x0 ;
// 			TWI_send(PCAL9555A_ADDR3, buf, 3) ;
// 		}
	}
#endif

#ifdef USE_PCAL9555A_TW1
    i = dio_read(PORT_TW1) ;
    printf("TW1:  i=0x%08x\n", i) ;
#endif // USE_PCAL9555A_TW1

#ifdef USE_MAX7324_TW1
	i = dio_read(PORT_TW1) ;
	printf("TW1:  i=0x%02x, f=0x%02x, o=0x%02x\n",
						*((unsigned char *)(&i) + 0),
						*((unsigned char *)(&i) + 1),
						*((unsigned char *)(&i) + 2) ) ;
#endif // USE_MAX7324_TW1

#ifdef USE_PCAL9555A_TW2
    i = dio_read(PORT_TW2) ;
    printf("TW2:  i=0x%08x\n", i) ;
#endif // USE_PCAL9555A_TW2

#ifdef USE_MAX7324_TW2
	i = dio_read(PORT_TW2) ;
	printf("TW2:  i=0x%02x, f=0x%02x, o=0x%02x\n",
						*((unsigned char *)(&i) + 0),
						*((unsigned char *)(&i) + 1),
						*((unsigned char *)(&i) + 2) ) ;
#endif // USE_MAX7324_TW2

#ifdef USE_PCAL9555A_TW3
    i = dio_read(PORT_TW3) ;
    printf("TW3:  i=0x%08x\n", i) ;
TWI_txrx(PCAL9555A_ADDR3,  0x44, (unsigned char *) &i, 2) ;
printf("Latch3: %04x\n", i) ;
TWI_txrx(PCAL9555A_ADDR3,  0x4A, (unsigned char *) &i, 2) ;
printf("Int3: %04x\n", i) ;
TWI_txrx(PCAL9555A_ADDR3,  0x06, (unsigned char *) &i, 2) ;
printf("Dir3: %04x\n", i) ;
	
#endif // USE_PCAL9555A_TW3


#ifdef USE_TWI_SRV
	i = dio_read(PORT_TWS) ;
	printf("TWS:  i=0x%02x, f=0x%02x, o=0x%02x\n",
						*((unsigned char *)(&i) + 0),
						*((unsigned char *)(&i) + 1),
						*((unsigned char *)(&i) + 2) ) ;
#endif // #ifdef USE_TWI_SRV

#if defined(CNT_PPS)
	printf("PPS: %5u, ", dio_counter(CNT_PPS) ) ;
#endif // defined(CNT_PPS)
#if defined(CNT_ODOMETER)
	printf("ODOMETER: %5u", dio_counter(CNT_ODOMETER) ) ;
#endif // defined(CNT_ODOMETER) 
#if defined(CNT_AUX)
	printf("AUX: %5u", dio_counter(CNT_AUX)) ;
#endif // defined(CNT_AUX)
	printf("\n") ;

#ifdef RF_ACC_
	printf(", ACC: %5u", acc_flag ) ;
	{
			unsigned char buf[4] ;
			
			for(i=0x20; i<0x38;i++){
				TWI_txrx(0x30, i, buf, 1) ;
				printf(" REGx%2x: 0x%hx\n", i, buf[0] ) ;
			}
// 			TWI_txrx(0x30, 0x36, buf, 1) ;
// 			printf(" th: %d 0x%hx", ((buf[0]&0x40)? 1:0), buf[0] ) ;
			
// 			TWI_txrx(0x30, 0x35, buf, 1) ;
// 			printf(" 2: %d 0x%hx", ((buf[0]&0x40)? 1:0), buf[0] ) ;
// 			
// 			TWI_txrx(0x30, 0x25, buf, 1) ;
// 			
			printf(" INTR: 0x%lx INTF: 0x%lx", GPIOINT->IO0IntEnR , GPIOINT->IO0IntEnF ) ;
// // 	buf[0] = 0x30 ;
// // 	buf[1] = 0xff ;
// // //	KS_lockw(TWIPORT) ; 
// //     TWI_send(0x30, buf, 2) ;
// // //	KS_unlock(TWIPORT) ;

	}
	
#endif

	printf("\n") ;

// USB registers
//    printf("ISR=0x%04x, IMR=0x%04x\n", AT91C_BASE_UDP->UDP_ISR, AT91C_BASE_UDP->UDP_IMR) ;
//    printf("UDP_NUM=%08x, ", AT91C_BASE_UDP->UDP_NUM) ;
//    printf("UDP_GLBSTATE=%08x, ", AT91C_BASE_UDP->UDP_GLBSTATE) ;
//    printf("UDP_FADDR=%08x\n", AT91C_BASE_UDP->UDP_FADDR) ;
//    for(i=0 ; i<4 ; i++) {
//        if (i) printf(", ") ;
//        printf("UDP_CSR[%d]=%06x", i, AT91C_BASE_UDP->UDP_CSR[i]) ;
//    }
//    printf("\n") ;

// counters
//    printf("Cnt0=%6d, Sr=0x%05x", AT91C_BASE_TCB0->TCB_TC0.TC_CV, AT91C_BASE_TCB0->TCB_TC0.TC_SR) ;
//    printf(", a=%6d, b=%6d, c=%6d\n", AT91C_BASE_TCB0->TCB_TC0.TC_RA, AT91C_BASE_TCB0->TCB_TC0.TC_RB, AT91C_BASE_TCB0->TCB_TC0.TC_RC) ;

//    printf("Cnt3=%6d, Sr=0x%05x", AT91C_BASE_TCB1->TCB_TC0.TC_CV, AT91C_BASE_TCB1->TCB_TC0.TC_SR) ;
//    printf(", a=%6d, b=%6d, c=%6d\n", AT91C_BASE_TCB1->TCB_TC0.TC_RA, AT91C_BASE_TCB1->TCB_TC0.TC_RB, AT91C_BASE_TCB1->TCB_TC0.TC_RC) ;
//
//    printf("Cnt4=%6d, Sr=0x%05x", AT91C_BASE_TCB1->TCB_TC1.TC_CV, AT91C_BASE_TCB1->TCB_TC1.TC_SR) ;
//    printf(", a=%6d, b=%6d, c=%6d\n", AT91C_BASE_TCB1->TCB_TC1.TC_RA, AT91C_BASE_TCB1->TCB_TC1.TC_RB, AT91C_BASE_TCB1->TCB_TC1.TC_RC) ;
}

//extern void CodeUpgrade(void) ;

static void cdwatch(char * param)
{
	struct PKTMEMBUF *pm ;
	unsigned char i ;
	struct time_tm rtxc_rtw ;

	//RTC_ReadTime(&rtxc_rtw) ;                   // read from RTC
	//KS_deftime(date2systime(&rtxc_rtw)) ;       // set new time
        KS_deftime(RTC_ReadTime_t()) ;

// 	if (*param == 'u') {                // watch set
// 		unsigned short spar, schk ;
// 		param++ ;
// 		while(*param == ' ')        // skip blank
// 			param++ ;
// 
// 		if (!(*param))              // abort if end of line
// 			return ;
// 
//         spar = strtol(param, NULL, 0) ;
// 		schk = EKS_RamCheckSum() ;
// 		if (spar==schk){
// 			printf("start upgrade: 0x%04x 0x%04x\n", spar, schk) ;
//         	CodeUpgrade() ;     // NO RETURN
// 		}
// 		return ;
// 	}
	
	if (*param == 's') {                // watch set
		param++ ;
		for(i=0 ; i<6 ; i++) {          // scan user parameters
			while(*param == ' ')        // skip blank
				param++ ;

			if (!(*param))              // abort if end of line
				return ;

			switch(i) {                 // handle data properly
			case 0 :                    // year
				rtxc_rtw.tm_yr = atoi(param) ;
				if (rtxc_rtw.tm_yr < 50)
					rtxc_rtw.tm_yr += 2000 ;
				if (rtxc_rtw.tm_yr < 100)
					rtxc_rtw.tm_yr += 1900 ;
				break ;
			case 1 :                // month
				rtxc_rtw.tm_mon = atoi(param) ;
				break ;
			case 2 :                // date
				rtxc_rtw.tm_day = atoi(param) ;
				break ;
			case 3 :                // hours
				rtxc_rtw.tm_hr = atoi(param) ;
				break ;
			case 4 :                // minutes
				rtxc_rtw.tm_min = atoi(param) ;
				break ;
			case 5 :                // seconds
				rtxc_rtw.tm_sec = atoi(param) ;
				break ;
			}
			while((*param) && (*param != ' '))  // next blank
				param++ ;
		}
//        rtxc_rtw.tm_isdst = 0 ;        // no daylight saving
		KS_deftime(date2systime(&rtxc_rtw)) ;           // store
	}
#ifdef USE_NANDFLASH_ON_ARM
	else if (*param == 't'){
		extern void NandWriteTemp(void) ;
		
		NandWriteTemp() ;
	}
#endif
#ifdef ENABLE_CRONO
	else if (*param == 'a'){
		int n ; // j
		unsigned long i, ll, k ;
		unsigned char cc[16] ;
		struct time_tm scrtime ;

		param++ ;
		while(*param == ' ')        // skip blank
			param++ ;

		if (!(*param)){              // general data
			EKS_LCK_FlashRead(CRONOFLASH_START, (unsigned char*) &i, 4) ;
			systime2date(i, &scrtime) ;
			printf("CRONO Dowload at %02d/%02d/%04d %02d:%02d:%02d\n", 
				scrtime.tm_day, scrtime.tm_mon, scrtime.tm_yr,
				scrtime.tm_hr, scrtime.tm_min , scrtime.tm_sec ) ;
			
			EKS_LCK_FlashRead((CRONOFLASH_START+4), (unsigned char*) &i, 4) ;
			printf("Vehicle data size %ld (end at %lx)\n", i-(CRONOFLASH_START+0x80L), i ) ;
			
			EKS_LCK_FlashRead((CRONOFLASH_START+16), (unsigned char*) &ll, 4) ;
			printf("Driver data size %ld (end at %lx)\n", (ll-i-2), ll) ;
			
			EKS_LCK_FlashRead((CRONOFLASH_START+8), cc, 4) ;
			printf("Dowload type:     %d\n", cc[0]) ;
			printf("Dowload error:    %d\n", cc[1]) ;
			printf("Dowload run:      %d\n", cc[2]) ;
			printf("Dowload cronoerr: %d\n", cc[3]) ;
			
			
			return ;
		}else{
			int lsl ;
			
			i = CRONOFLASH_START + atoi(param) ;
			if (atoi(param)==1){ // Vehicle data
				i = (CRONOFLASH_START+0x80L) ;	// Start address
				EKS_LCK_FlashRead((CRONOFLASH_START+4), (unsigned char*) &ll, 4) ; // End address
				//ll++ ;
			}else if (atoi(param)==2){ // Driver data
				EKS_LCK_FlashRead((CRONOFLASH_START+4), (unsigned char*) &i, 4) ;
				i+=2 ;
				EKS_LCK_FlashRead((CRONOFLASH_START+16), (unsigned char*) &ll, 4) ;
				ll++ ;
			}else{
				i = atoi(param) ;
				ll = i+(16*10) ;
			}
			
			for (k=i;k<ll+32;k+=16){
				//SPI_FlashRead(cc, k, 16) ;
				EKS_LCK_FlashRead(k, cc, 16) ;
				//printf("CRONO 0x%lx: ", i) ;
				for(n=0 ; n<16 ; n++) {
					if ((k+n)>ll) break ;
					printf("%02x ", cc[n]) ;
				}
				printf("\n") ;
				for(lsl=0;lsl<300;lsl++) tickwait(1000) ;
			}
			return ;
		}
	}
#endif // ENABLE_CRONO
#ifdef DEBUG_MEMORY_FUNCTIONS
    else if (*param == 'c') {           // ram checksum
// #if defined(USE_NANDFLASH_ON_ARM)
//         unsigned char cval[512] ;
//         unsigned short chk ;
//         int i ;
//         chk = 0 ;
//         for(i=CODEFLASH_START ; i<=(CODEFLASH_STOP/2) ; i+=2) {
//             chk ^= *((unsigned short *)(i)) ;
//         }
//         printf("FlashCheck=0x%04x\n", chk) ;
//         printf("RamCheck  =0x%04x\n", EKS_RamCheckSum(cval, sizeof(cval))) ;
// #else // defined(USE_NANDFLASH_ON_ARM)
        unsigned short chk ;
        int i ;
        chk = 0 ;
        for(i=CODEFLASH_START ; i<=CODEFLASH_STOP ; i+=2) {
            chk ^= *((unsigned short *)(i)) ;
        }
        printf("FlashCheck=0x%04x\n", chk) ;
        printf("RamCheck=0x%04x\n", EKS_RamCheckSum()) ;
//#endif // defined(USE_NANDFLASH_ON_ARM)

    }
    else if (*param == 'f') {           // ram fill
        int rv ;
        rv = EKS_RamFill() ;
        printf("EKS_RamFill()=%d\n", rv) ;
    }
    else if (*param == 'w') {           // ram write
        unsigned long long lasttick ;
        unsigned char cval[512] ;
        int rv ;
        memset(cval, 0, sizeof(cval)) ;
        param++ ;
        while(*param == ' ')    // skip blank
            param++ ;
        memcpy(cval, param, strlen(param)) ;

        lasttick = tickmeasure(0LL) ;
        rv = EKS_RamWrite(512, cval, sizeof(cval)) ;
        lasttick = tickmeasure(lasttick) ;
        printf("EKS_RamWrite()=%d, in usec: %llu\n", rv, lasttick) ;
    }
    else if (*param == 'r') {           // ram read
        unsigned long long lasttick ;
        unsigned char cval[512] ;
        int rv ;
        memset(cval, 0 , sizeof(cval)) ;
        lasttick = tickmeasure(0LL) ;
        rv = EKS_RamRead(512, cval, sizeof(cval)) ;
        lasttick = tickmeasure(lasttick) ;
        printf("EKS_RamRead()=%d, in usec: %llu\n", rv, lasttick) ;
        for(i=0 ; i<8/*sizeof(cval)*/ ; i++) {
            if ((cval[i] >= ' ') && (cval[i] < 127))
                putchar(cval[i]) ;
            else
                printf("_0x%02x_", cval[i]) ;
        }
        putchar('\n') ;
    }
	else if (*param == 'l') {           // ram load
        unsigned long addr ;
        int i, flag, rv ;
        unsigned char cval[512] ;

        rv = 0 ;
		for(addr=0 ; addr<RUNCODE_SIZE ; addr+=sizeof(cval)) {
            flag = 0 ;
            for(i=0 ; i<sizeof(cval) ; i++) {
                cval[i] = *((unsigned char *)(addr+i)) ;
                if (cval[i] != 0xff) flag = 1 ;
            }
            putchar(flag ? 'x' : '.') ;
            if (!flag) continue ;
            rv = EKS_RamWrite(addr, cval, sizeof(cval)) ;
            if (rv) break ;
        }
        putchar('\n') ;
        if (rv) printf("EKS_RamWrite() ERROR\n") ;
    }
#ifdef USE_NANDFLASH_ON_ARM
    else if (*param == 'z'){           // ram dump
		unsigned long addr1 ;
		int i ;
		unsigned char cval[512] ;
		
		if (param[1]){
			addr1= strtol(&param[2], NULL, 0) ; // atol(&param[2]) ;
			memset(cval, 0, 255) ;
			i = nand_write512(addr1, cval) ;
			printf("filled at %06lx with zero (%d)\n", addr1, i) ;
		}
	}
#endif
    else if ((*param == 'd') || (*param == 'b')){           // ram dump
		unsigned long addr1 ;
		int i, rv ;
		unsigned char cval[512] ;

		addr1 = 0L ; // CODEFLASH_START ;
		if (param[1]){
			addr1= strtol(&param[2], NULL, 0) ; // atol(&param[2]) ;
		}

		if (addr1<CODEFLASH_STOP){
			rv = EKS_RamRead(addr1, cval, sizeof(cval)) ;
			printf("Read at %06lx: %d\n", addr1, rv) ;

			for(i=0 ; i<sizeof(cval) ; i++) {
				printf(" %02X%02X", cval[i],*((unsigned char *)(addr1+i)) ) ;
				if ((i & 0x1f)==0x1f) printf("\n") ;
			}
#ifdef USE_NANDFLASH_ON_ARM
		}else{
			if (*param == 'b'){
				extern void NAND_FlashRead(unsigned char *dst, unsigned long bbegin, int flen) ;
				NAND_FlashRead(cval, addr1, sizeof(cval)) ;
				i = 0 ;
			}else
				i = nand_read512(addr1, cval) ;
			
			if (i<0) printf(" Error %d ", i ) ;
			for(i=0 ; i<sizeof(cval) ; i++) {
				printf(" %02X", cval[i] ) ;
				if ((i & 0x1f)==0x1f) printf("\n") ;
			}
#endif // USE_NANDFLASH_ON_ARM
		}
    }
    else if (*param == 'v') {           // ram verify
        unsigned long addr, addr1 ;
        int i ; //  flag ; // , rv ;
        unsigned char cval[512] ;

        addr1 = 0L ; // CODEFLASH_START ;
        if (param[1]){
            addr1= strtol(&param[2], NULL, 0) ; // atol(&param[2]) ;
        }
        for(addr=addr1 ; addr<=CODEFLASH_STOP ; addr+=sizeof(cval)) {
            EKS_RamRead(addr, cval, sizeof(cval)) ;
            for(i=0 ; i<sizeof(cval) ; i++) {
                if ( cval[i] !=  *((unsigned char *)(addr+i)) ) {
                    printf("Error at %04lx: expected 0x%02x, got 0x%02x\n", addr+i, *((unsigned char *)(addr+i)), cval[i]) ;
                    break ;
                }
            }
        }
    }
#endif        // DEBUG_MEMORY_FUNCTIONS

    //KS_deftime(RTC_ReadTime()) ;         // set correct time
    if (!(pm = EKS_PktBufFromTime(0)))     // return if error
        return ;

    while(EKS_PktBufPullup(&pm, &i, sizeof(i)) == sizeof(i))
        putchar(i) ;                        // current time

    putchar('\n') ;

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#if defined(USE_AT91SAM7A3)
	printf("RT timer=%d, RT presca=%d, ",  AT91C_BASE_RTTC->RTTC_RTVR,
                                               AT91C_BASE_RTTC->RTTC_RTMR & 0xffff) ;
	printf("RT alarm=%d\n",                AT91C_BASE_RTTC->RTTC_RTAR) ;
	printf("GPBR0=0x%08x, GPBR1=0x%08x\n", AT91C_BASE_SYS->SYS_GPBR0,
                                               AT91C_BASE_SYS->SYS_GPBR1) ;
#endif // defined(USE_AT91SAM7A3)

	printf("MCFR(%d)=%d\n", (AT91C_BASE_CKGR->CKGR_MCFR & 0x10000)?1:0, AT91C_BASE_CKGR->CKGR_MCFR & 0xffff) ;
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
}

#ifdef USE_TRANSACTIONS_ON_ARM
static void cdflash(char * param)
{
	unsigned char c;
	unsigned short id ;
	short signature ;
	struct PKTMEMBUF * pif ;
	unsigned long ul ;

#ifndef USE_LPC1788
	extern void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
						 unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;
	unsigned char bufrx[5] ;
	unsigned char buftx[5] ;
#endif

    switch (*param) {
    case 0:
#ifndef USE_LPC1788
        buftx[0] = 0x9f ;
        SPI_rtx2(buftx, bufrx, 5, NULL, NULL, 0) ;
        printf("ID: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
                bufrx[0], bufrx[1], bufrx[2], bufrx[3], bufrx[4]) ;
        buftx[0] = 0xd7 ;
        SPI_rtx2(buftx, bufrx, 2, NULL, NULL, 0) ;
        printf("SR: 0x%02x\n", bufrx[1]) ;
#endif
        ul = EKS_FlashFree() ;
        printf("Free/Tot: %6lu", ul) ;
        ul = EKS_FlashTotal() ;
        printf("/%6lu\n", ul) ;
        return ;

#ifdef USE_TRACE
	case '9':
		printf("Erase from  0x%lx to 0x%lx\n", LOGFLASH_START, LOGFLASH_STOP) ;
		//SPI_FlashErase(TRACEFL_START, TRACEFL_STOP ) ;
		EKS_LCK_FlashErase(LOGFLASH_START, LOGFLASH_STOP ) ;
		return ;
		
	case '0':
		printf("Erase from  0x%lx to 0x%lx\n", TRACEFL_START, TRACEFL_STOP) ;
		//SPI_FlashErase(TRACEFL_START, TRACEFL_STOP ) ;
		EKS_LCK_FlashErase(TRACEFL_START, TRACEFL_STOP ) ;
		return ;
		
	case '1':
		param++ ;
		while(*param == ' ')        // skip blank
			param++ ;

		id = atoi(param) ;        // get parameter number
		{
			unsigned char tracebuf[TRACEFL_SITEM] ;
			
			printf("Erase from  0x%lx to 0x%lx\n", (TRACEFL_START+id), (TRACEFL_START+id+TRACEFL_PAGE-1)) ;
			//SPI_FlashErase((TRACEFL_START+id), (TRACEFL_START+id+TRACEFL_PAGE-1)) ;
			EKS_LCK_FlashErase((TRACEFL_START+id), (TRACEFL_START+id+TRACEFL_PAGE-1)) ;
			printf("Dump:\n") ;
			for( id=0 ; id<16 ;id++){
 				//SPI_FlashRead(tracebuf, (TRACEFL_START+(id*TRACEFL_SITEM)),  TRACEFL_SITEM) ;
				EKS_LCK_FlashRead( (TRACEFL_START+(id*TRACEFL_SITEM)),tracebuf, TRACEFL_SITEM) ;
				for(ul=0;ul<TRACEFL_SITEM;ul++) {
					c = tracebuf[ul] ;
					printf("%02x ", (unsigned short) c ) ;
				}
				printf("\n") ;
			}
		}
		return ;
		
	case '2':
		param++ ;
		while(*param == ' ')        // skip blank
			param++ ;

		id = atoi(param) ;        // get parameter number
		
		{
			unsigned char tracebuf[TRACEFL_SITEM] ;
			
			strcpy((char*)tracebuf, "123456") ;
			printf("Write to 0x%lx (%d)\n", (TRACEFL_START+id), id) ;
			//SPI_FlashWrite((TRACEFL_START+id), tracebuf, extapibuf, 6 ) ;
			EKS_LCK_FlashWrite((TRACEFL_START+id), tracebuf, 6 ) ;
			printf("Dump:\n") ;
			for(id=0;id<16;id++){
				//SPI_FlashRead(tracebuf, (TRACEFL_START+(id*TRACEFL_SITEM)),  TRACEFL_SITEM) ;
				EKS_LCK_FlashRead( (TRACEFL_START+(id*TRACEFL_SITEM)), tracebuf, TRACEFL_SITEM) ;
				for(ul=0;ul<TRACEFL_SITEM;ul++) {
					c = tracebuf[ul] ;
					printf("%02x ", (unsigned short) c ) ;
				}
				printf("\n") ;
			}
		}
		return ;
#endif // #ifdef USE_TRACE

    case 'c':
        EKS_FlashClear() ;
        return ;

    case 'd':                   // DATA out
        signature = 0xa004 ;
        param++ ;
        break ;

    case 'o':                   // SMS out
        signature = 0xa002 ;
        param++ ;
        break ;

    case 'i':                   // SMS in
        signature = 0xa001 ;
        param++ ;
        break ;

    default :
        signature = -2 ;        // filler
    }

    while(*param == ' ')        // skip blank
        param++ ;

    id = (unsigned short)(atoi(param)) ;        // get parameter number

    printf("Dump:\n") ;

    if (!(pif = EKS_PktBufRetrive(signature, id, NULL)))
        return ;

    while(EKS_PktBufPullup(&pif, &c, 1) == 1)
        printf(" 0x%02x", (unsigned int)(c)) ;

    putchar('\n') ;
}
#endif // USE_TRANSACTIONS_ON_ARM

//#ifdef USE_SPI_ON_ARM
static void cdeeprom(char * param)
{
	switch(*param) {
#ifdef USE_PARAMETERS_ON_EEPROM
	case 'd' :
	case 'D' :
				cdeedump(&param[1]) ;
				break ;

#endif
	case 'c' :
	case 'C' :
				cdeeclear(&param[1]) ;
				break ;

#ifdef G100
	case 'e' :
	case 'E' :
				cardsn(&param[1]) ;       
				break ;
#endif
	case 'i' :
	case 'I' :
				cdeeinit(&param[1]) ;
				break ;

	case 'w' :
	case 'W' :
				cdeewrite(0, &param[1]) ;
				break ;

	case '+' :
				cdeewrite(1, &param[1]) ;
				break ;

	case 'r' :
	case 'R' :
				cdeeread(&param[1]) ;        // read parameter
				break ;

	default :
				cdeeread((char *)(0)) ;        // read all
				break ;
	}
}

#ifdef G100
static void cardsn(char * param)
{
short i ;
unsigned long csn ;
char *p ;
unsigned char c ;

	while(*param == ' ')        // skip blank
				param++ ;

	csn = atol(param) ;
	p = (char*) &csn ;
	
	c = 0xaa ;                  // turn smart card on
	KS_enqueuew(GPSOPORT, &c) ;
	//SLEEP(20);
	for(i=0;i<20;i++) tickwait(1000) ;
	SmartCard_Write(GPSOPORT,0x1b, 4, p ) ; // from 28.th bit

	printf("Wroted CardSerNum: %8ld\n", csn) ;

}
#endif // #ifdef G100


static void cdeeread(char * param)
{
	unsigned char parnum ;
	unsigned char c ;
	unsigned short parval ;
	long  lparval ;
	struct PKTMEMBUF * pif ;

	if (param) {
		while(*param == ' ')        // skip blank
				param++ ;
		parnum = atol(param) ;
		if (!EKS_ParReadShort(parnum, &parval)) {        // read this one
			printf("%3d %5d 0x%04x\n", (int)(parnum), parval, parval) ;
		}
		return ;
	}

	printf("Parameters:\n") ;

	for(parnum=0 ; parnum<128 ; parnum++) {        // scan parameters
		if (!EKS_ParReadShort(parnum, &parval)) {        // read this one
			printf("%3d %5d 0x%04x\n", (int)(parnum), parval, parval) ;
		}
	}

	for(parnum=128 ; parnum<224 ; parnum++) {        // scan parameters
		pif = EKS_ParRead(parnum) ;                // read this one
		if (pif) {                // if really a parameter
			printf("%3d ", (int)(parnum)) ;
			while(EKS_PktBufPullup(&pif, &c, 1) == 1) {
				putchar(c) ;
			}
			putchar('\n') ;
		}
	}

	for(parnum=224 ; parnum<255 ; parnum++) {        // scan parameters
		if (!EKS_ParReadLong(parnum, &lparval)) {        // read this one
			printf("%3d %10ld 0x%08lx\n", (int)(parnum), lparval, lparval) ;
		}
	}

	printf("Tot=%d\n", EKS_ParSize()) ;
}

static void cdeeclear(char * param)
{
	EKS_ParClear() ;                // erase full EEPROM space
}

#ifdef USE_PARAMETERS_ON_EEPROM
static void cdeedump(char * param)
{
unsigned char eedata[64] ;
int j, i, addr ;

	while(*param == ' ')        // skip blank
				param++ ;
	
	addr = atoi(param) ;        // get parameter number

	EEPROM_Read(EEPROM_PAGE_OFFSET(addr), (addr/EEPROM_PAGE_SIZE), eedata, MODE_8_BIT, sizeof(eedata)) ;
	printf("EEPROM_Read at %d (%d,%d)\n",addr, EEPROM_PAGE_OFFSET(addr), (addr/EEPROM_PAGE_SIZE)) ;
	for(j=0 ; j<4 ; j++) {
		for(i=0 ; i<16 ; i++) {
			printf("0x%02x ", eedata[(j*16)+i]) ;
		}
		printf("\n") ;
	}
}
#endif

static void cdeeinit(char * param)
{
	struct MYSETUP setup ;
	struct MYSETUP * psetup ;

	while(*param == ' ')        // skip blank
				param++ ;

	if ((setup.sernum = atol(param))) {        // set serial number
		EKS_NewSetup(&setup) ;
	} else {                                // read serial number
		psetup = EKS_GetSetup() ;
		printf("SerNum: %8ld\n", (psetup->sernum)) ;
	}
}

static void cdeewrite(char app, char * param)
{
	unsigned char parnum ;
	short parval ;
	long lparval ;
	struct PKTMEMBUF *pmfirst, *pmactual ;        // parameter

	while(*param == ' ')        // skip blank
		param++ ;

	if (!(*param))                // abort if no arguments
		return;

	parnum = (unsigned char)(atoi(param)) ;        // get parameter number

	while((*param != ' ') && (*param))          // search next blank
		param++ ;

	while(*param == ' ')                        // skip blank
		param++ ;

	if (!(*param)) {                // erase if no arguments
		pmfirst = MNULL ;
		EKS_ParWrite(parnum, &pmfirst) ;
		return;
	}

	if (parnum < 128) {                 // parameter is short
		parval = (short) strtol(param, NULL, 0) ; // atoi(param) ;
		EKS_ParWriteShort(parnum, parval) ;
	} else if (parnum >= 224) {         // parameter is long
		lparval = strtol(param, NULL, 0) ; // atol(param) ;
		EKS_ParWriteLong(parnum, lparval) ;
	} else {
		pmfirst=pmactual=MNULL ;// initialize pointers for parameter
		if (app){
			pmfirst=EKS_ParRead(parnum) ;                // read this one
			pmactual = pmfirst ;
			while(pmactual->next)    // search end of first chain
				pmactual = pmactual->next ;
		}
		while(*param) {         // copy buffer
			if (EKS_PktBufAdd(*param, &pmfirst, &pmactual)) {        // another char
				return ;        // error
			}
			param++ ;
		}
		EKS_ParWrite(parnum, &pmfirst) ;
	}
}
//#endif // USE_SPI_ON_ARM

static void cdoff(char * param)
{
    int sleepsec = 0 ;

    if (*param == 'f') {                // immediate off
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ DEBUG
#if defined(USE_AT91SAM7A3)
        // Shutdown
        AT91C_BASE_SHDWC->SHDWC_SHCR = 0xA5000001 ;      // shutdown
        // Hardware turn off
        //dio_write(PORT_PIOA, PIOA_STAYON, 0) ;

        KS_delay(SELFTASK, ((TICKS)100/CLKTICK)) ;
#endif // defined(USE_AT91SAM7A3)

        DISABLE ;       // no more interrupts
        for( ; ; ) ;    // just t be sure

    } else if (*param == 'r') {                // reboot
#ifdef MTS_CODE
        RequestShutdown(SD_REBOOT, SHUTCODE_MANUAL) ;
#else  // MTS_CODE
        EKS_AskShutdown(SD_REBOOT) ;
#endif // MTS_CODE

#ifdef DEBUG_MEMORY_FUNCTIONS
    } else if (*param == '+') {         // user wants code upgrade
#ifdef MTS_CODE
        RequestShutdown(SD_CODEUPGRADE, SHUTCODE_MANUAL) ;
#else  // MTS_CODE
        EKS_AskShutdown(SD_CODEUPGRADE) ;
#endif // MTS_CODE
#endif // DEBUG_MEMORY_FUNCTIONS

#ifdef USE_LOW_POWER
    } else if (*param == 'l') {         // user wants low power
#ifdef MTS_CODE
        RequestShutdown(SD_LOWPOWER, SHUTCODE_MANUAL) ;
#else  // MTS_CODE
        EKS_AskShutdown(SD_LOWPOWER) ;
#endif // MTS_CODE
#endif // USE_LOW_POWER

    } else if (*param == 'c') {         // user wants sleep & charge
#ifdef MTS_CODE
        RequestShutdown(SD_STDBYCH, SHUTCODE_MANUAL) ;
#else  // MTS_CODE
        EKS_AskShutdown(SD_STDBYCH) ;
#endif // MTS_CODE
    } else {                            // user wants sleep without charge
        while(*param == ' ')            // skip blank
            param++ ;
        sleepsec = atoi(param) ;        // num of seconds to wait
        if (sleepsec) {                 // set alarm
            //RTC_ReadTime(&rtxc_rtw) ;   // actual time from RTC
            //systime2date(date2systime(&rtxc_rtw) + sleepsec, &rtxc_rtw) ;
            RTC_WriteAlarm_t(RTC_ReadTime_t() + sleepsec) ;
        }
#ifdef USE_LOW_POWER
#ifdef MTS_CODE
        RequestShutdown(SD_LOWPOWER, SHUTCODE_MANUAL) ;
#else  // MTS_CODE
        EKS_AskShutdown(SD_LOWPOWER) ;
#endif // MTS_CODE
#else // USE_LOW_POWER
#ifdef MTS_CODE
        RequestShutdown(SD_STDBY, SHUTCODE_MANUAL) ;
#else  // MTS_CODE
        EKS_AskShutdown(SD_STDBY) ;
#endif // MTS_CODE
#endif // USE_LOW_POWER
    }

    printf("off sent (%d s)\n", sleepsec) ;
    printf("Status / Control = 0x%04x\n", RTC_ReadStatus()) ;
#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    printf("RTTC_RTAR=%d\n", AT91C_BASE_RTTC->RTTC_RTAR) ;
    printf("RTTC_RTVR=%d\n", AT91C_BASE_RTTC->RTTC_RTVR) ;
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#if defined(USE_AT91SAM3S4)
    printf("RTT_AR=%lu\n", RTT->RTT_AR) ;
    printf("RTT_VR=%lu\n", RTT->RTT_VR) ;
#endif // defined(USE_AT91SAM3S4)
}


#ifdef USE_TWI2_ON_ARM
static void checkchbattery(char * param)
{
	int i ; // , j ;
	unsigned char buf[15] ;

#ifdef NCT75_ADDR
	// Read  temperature sensor
	TWI2_txrx(NCT75_ADDR, 0x100, buf, 2) ;
	printf("Temp MSB 0x%02x LSB 0x%02x (%d.%02d C)\n",  buf[0], buf[1], buf[0], (buf[1]*1000)/256 ) ;
#endif

//	printf("call TWI2_txrx batterycharger 0x%x\n", BQ24193_ADDR) ;
	// Read all register (0-7) to read status
	TWI2_txrx(BQ24193_ADDR, 0x100, buf, 11) ;
	for(i=0;i<11;i++){
		printf("Breg%02x 0x%02x\n",  i, buf[i]) ;
	}
	
	// (Write 1 at MSB of reg.0 to reset watchdog timer (30 sec))
	
	// If pars write value to reg
	while(*param == ' ')        // skip blank
		param++ ;
	
	if (*param) {                // if parameter
		buf[0] = (unsigned char)(atoi(param)) ; // get parameter number

		while(*param++ != ' ' ) ;       // search next blank
			
		while(*param == ' ')            // skip blank
			param++ ;

		buf[1] = (unsigned char) strtol(param, NULL, 0) ; // atoi(param) ;
		printf("Set Breg%d 0x%02x\n", buf[0], buf[1]) ;
		if (buf[0]<8){ // Write reg
			TWI2_send(BQ24193_ADDR, buf, 2) ;
		}
	}
	{
		extern unsigned short ReadVbat(short req) ;
		printf("Vbatt is %d mV\n", ReadVbat(0) ) ;
	}

}
#endif // USE_TWI2_ON_ARM

#ifdef GPS_I2C
static void cbug_gpsi2c(char * param){
short i ;
unsigned char togps[120] ;

	i = 0 ;
	printf("To gps:    ") ;
	while ((param[0]) && (param[1])){
		if (param[0]>0x60)
			togps[i] = param[0] - 0x57 ;
		else if (param[0]>0x40)
			togps[i] = param[0] - 0x37 ;
		else
			togps[i] = param[0] - 0x30 ;
			
		togps[i] *= 16 ;
		if (param[1]>0x60)
			togps[i] += param[1] - 0x57 ;
		else if (param[1]>0x40)
			togps[i] += param[1] - 0x37 ;
		else
			togps[i] += param[1] - 0x30 ;
		
		printf("%02x", togps[i] ) ;
		i++ ;
		param += 2 ;
	}
	printf(" Len=%d\n", i ) ;
	TWI2_send((0x60<<1), togps, i) ;
}
#endif



#endif /* } CBUG */

// end of file - rtxcbug.c

