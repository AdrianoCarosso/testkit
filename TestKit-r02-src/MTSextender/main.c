// main.c
//
//
//   Copyright (c) 1997-2008.
//   T.E.S.T. srl
//
//	History:
//	October 1997	Purchase
//	  1/Jun/2006    Reworked for ARM7
//
// ******************************************************** Related definitions

#include <stdlib.h>

#include "rtxcapi.h"
#include "rtxcopts.h"
#include "enable.h"

#include "cclock.h"
#include "cqueue.h"
#include "csema.h"
#include "extapi.h"
#include <cvtdate.h>
#include <stdio_console.h>


#include "assign.h"


#include "_AT91SAM7A3.h"

// -----------------------------------------------------------------------
// debug use only, use #define or #undef

#undef USE_DIRECT_USART

// -----------------------------------------------------------------------
// function declarations

extern void clkstart(int lowpowermode) ;        // initialize RTI
extern void rtxcinit(void) ;                    // initialize RTXC data structures
extern void LowLevelInit(void) ;

// from uartdrv.c
extern void uartstart(int num, unsigned long baud_rate, int mode) ;

// local functions
void maintask(void) TASK_ATTRIBUTE ;

// -----------------------------------------------------------------------
// Task declarations

#ifdef CBUG
#define STKSZ_RTXCbug   512     // RTXCbug
#endif // CBUG

#define STKSZ_UARTrtx   168     // UART task RTX

#ifdef USE_USB_ON_ARM
#define STKSZ_USBtask	512     // USB task
#endif // USE_USB_ON_ARM

#ifdef USE_ADC_FAST_ON_ARM
#define STKSZ_ADCtask	512     // ADC task
#endif // USE_ADC_FAST_ON_ARM

#define STKSZ_TK0       512     // TK0 - Application

const TASK ntasks = NTASKS ;

// -----------------------------------------------------------------------
// Stack declarations

#ifdef CBUG
static char stack_RTXCbug[STKSZ_RTXCbug] NOINIT_ATTRIBUTE ;     // RTXCbug
#endif // CBUG

static char stack_UARTrtx[STKSZ_UARTrtx] NOINIT_ATTRIBUTE ;     // UART task RTX

#ifdef USE_USB_ON_ARM
static char stack_USBtask[STKSZ_USBtask] NOINIT_ATTRIBUTE ;     // USB task
#endif // USE_USB_ON_ARM

#ifdef USE_ADC_FAST_ON_ARM
static char stack_ADCtask[STKSZ_ADCtask] NOINIT_ATTRIBUTE ;     // ADC task
#endif // USE_ADC_FAST_ON_ARM

static char stack_TK0[STKSZ_TK0] NOINIT_ATTRIBUTE ;             // TK0 - Application

// -----------------------------------------------------------------------
// Task startup list

static const TASK startls[] =
{
#ifdef CBUG
   RTXCBUG ,		// RTXCbug
#endif // CBUG

   UARTDRV,		// UART task RTX

#ifdef USE_USB_ON_ARM
   USBTASK,		// USB task
#endif // USE_USB_ON_ARM
#ifdef USE_ADC_FAST_ON_ARM
   ADCTASK,		// ADC task
#endif // USE_ADC_FAST_ON_ARM

   TK0EXTENDER,		// TK0 - extender

   0			// null terminated list
};

#ifdef CBUG
extern void rtxcbug (void) ;	// RTXCbug
#endif // CBUG

extern void uartdrv(void) ;	// COMs task TX

#ifdef USE_USB_ON_ARM
extern void usbtask(void) ;	// USB task
#endif // USE_USB_ON_ARM

#ifdef USE_ADC_FAST_ON_ARM
extern void adctask(void) ;	// ADC task
#endif // USE_ADC_FAST_ON_ARM

extern void tk0extender(void) ;	// TK0 - task extender

const KTCB rtxkktcb[1+NTASKS] =
{
  { NULL    , NULL, 0, 0 },		// null task
#ifdef CBUG
  { rtxcbug , stack_RTXCbug, STKSZ_RTXCbug, 9 },        // RTXCbug
#endif // CBUG

  { uartdrv, stack_UARTrtx, STKSZ_UARTrtx, 4 },	        // UART task RTX

#ifdef USE_USB_ON_ARM
  { usbtask, stack_USBtask, STKSZ_USBtask, 5 },	        // USB task
#endif // USE_USB_ON_ARM

#ifdef USE_ADC_FAST_ON_ARM
  { adctask, stack_ADCtask, STKSZ_ADCtask, 10 },        // ADC task
#endif // USE_ADC_FAST_ON_ARM

  { tk0extender, stack_TK0, STKSZ_TK0, 10 },            // TK0 - task extender
} ;

TCB rtxtcb[1+NTASKS];

#ifdef CBUG
const char taskkname[1+NTASKS][NAMMAX+1] =
{
   " ",
#ifdef CBUG
   "RTXCBUG",           // RTXCbug
#endif // CBUG

   "UARTDRV",           // UART task RTX

#ifdef USE_USB_ON_ARM
   "USBTASK",           // USB task
#endif // USE_USB_ON_ARM
#ifdef USE_ADC_FAST_ON_ARM
   "ADCTASK",           // ADC task
#endif // USE_ADC_FAST_ON_ARM

   "TK0EXTN"            // TK0 - task extender
};
#endif // CBUG

#ifdef USE_PERFORMANCE
volatile unsigned long perf_counter ;
volatile unsigned long perf_last ;
#endif // USE_PERFORMANCE

//----------------------------------------------------------------------------
// AT91F_Console_Init

#ifdef USE_DIRECT_USART
void AT91F_Console_Init(unsigned long baud_rate) {
    AT91PS_USART UART_BASE ;
    unsigned long baud_value ;

    AT91C_BASE_PMC->PMC_PCER = (1 << (AT91C_ID_US1)) ;  // enable

    UART_BASE = AT91C_BASE_US1 ;        // base address
    // PIO A: Peripheral A select register
    AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA7_RXD1 | AT91C_PA8_TXD1) ;
    // PIO A: disable register
    AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA7_RXD1 | AT91C_PA8_TXD1) ;
    // Disable interrupts
    UART_BASE->US_IDR = (unsigned int) -1 ;

    // Reset receiver and transmitter
    UART_BASE->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS ;

    // Define the baud rate divisor register
    baud_value = ((current_clock * 10)/(baud_rate * 16)) ;
    if ((baud_value % 10) >= 5) {
        baud_value = (baud_value / 10) + 1;
    } else {
        baud_value /= 10;
    }
    UART_BASE->US_BRGR = baud_value ;

    // Define the USART mode
    UART_BASE->US_MR = AT91C_US_USMODE_NORMAL |
                       AT91C_US_NBSTOP_1_BIT |
                       AT91C_US_PAR_NONE |
                       AT91C_US_CHRL_8_BITS |
                       AT91C_US_CLKS_CLOCK  ;

    // Enable Rx Tx
    UART_BASE->US_CR = AT91C_US_RXEN | AT91C_US_TXEN ;
}
#endif // USE_DIRECT_USART

//----------------------------------------------------------------------------

void maintask(void){
  int i ;
    
	LowLevelInit() ;
  AT91F_CLOCKinit(48000000) ;	

#ifdef USE_DIRECT_USART
    AT91F_Console_Init(19200) ;
    puts("\nBoot") ;
    //printf("\nUsing printf %lld\n", 1000000000LL*1000LL) ;
#endif // USE_DIRECT_USART

    clkstart(NO) ;	// Init RTI tick driver
    rtxcinit() ;	// Init RTXC, required first
    adcstart() ;        // Init ADC, it differs in case ADC_FAST
    diostart() ;        // Init digital I/O

#ifdef USE_TWI_ON_ARM
    twistart() ;        // init TWI for I/O subsystem connection
#endif // USE_TWI_ON_ARM

#ifdef USE_SPI_ON_ARM
    spistart() ;        // init SPI for Flash subsystem connection
#endif // USE_SPI_ON_ARM

#ifdef USE_CAN_ON_ARM
    canstart(0) ;       // init CAN bus 0
    canstart(1) ;       // init CAN bus 1
#endif // USE_CAN_ON_ARM

    EKS_init(LU_TOT) ;	// Extended functions init
    // GSM
    uartstart(/* com = */ 0, /* baud_rate = */ 9600 /* test rs485 115200 */ /*38400*/ /*57600*/,
              /* mode =  AT91C_US_USMODE_HWHSH | */ AT91C_US_NBSTOP_1_BIT | AT91C_US_PAR_NONE | AT91C_US_CHRL_8_BITS) ;

#ifndef USE_DIRECT_USART
    // LU11 - CONSOLE
    uartstart(/* com = */ 1, /* baud_rate = */ 9600,
              /* mode = */ /*AT91C_US_USMODE_HWHSH | */ AT91C_US_NBSTOP_1_BIT | AT91C_US_PAR_NONE | AT91C_US_CHRL_8_BITS) ;
#endif // USE_DIRECT_USART

    // LU2 - AUX
    uartstart(/* com = */ 2, /* baud_rate = */ 9600,
              /* mode =  AT91C_US_USMODE_HWHSH  | */ AT91C_US_NBSTOP_1_BIT | AT91C_US_PAR_NONE | AT91C_US_CHRL_8_BITS) ;

    // GPS
    uartstart(/* com = */ 3, /* baud_rate = */ 9600,
              /* mode = */ AT91C_US_NBSTOP_1_BIT | AT91C_US_PAR_NONE | AT91C_US_CHRL_8_BITS) ;

// -----------------------------------------------------------------------
// Are we ready? Let's the dance open!

    ENABLE ;		// enable interrupts

// -----------------------------------------------------------------------

// start all declared tasks

    for(i=0 ; i<ntasks ; i++) { // start up tasks in startup list
        PRIORITY pri ;
        int t ;
		t = startls[i] ;	// task number

		if (!t) break ;         // null terminated list

		KS_execute(t) ; 		// let it run

		pri = rtxkktcb[t].priority ;
		if ( pri == 10 ) {
			KS_defslice(t, 2) ; 	// max time slice
		}
    }


// -----------------------------------------------------------------------
// let all task to run

#ifdef SYNC_START
    KS_unblock(1, ntasks) ;
#endif // SYNC_START

// -----------------------------------------------------------------------
//  null process

    for( ; ; ) {
#ifdef USE_PERFORMANCE
        perf_counter++ ;
#else  // USE_PERFORMANCE
        AT91C_BASE_PMC->PMC_SCDR = AT91C_PMC_PCK ;      // idle mode
#endif // USE_PERFORMANCE
    }
    
// no return
}

//----------------------------------------------------------------------------

int putchar(int c) {
#ifdef USE_DIRECT_USART
    AT91PS_USART UART_BASE = AT91C_BASE_US1 ;   // base address
    while (!(UART_BASE->US_CSR & AT91C_US_TXRDY)) ;
    UART_BASE->US_THR = c & 0x1ff ;
#else // USE_DIRECT_USART
    KS_enqueuew(MONITOPORT, &c) ;
#endif // USE_DIRECT_USART
    return(c) ;
}

//----------------------------------------------------------------------------

int getchar(void) {
#ifdef USE_DIRECT_USART
    AT91PS_USART UART_BASE = AT91C_BASE_US1 ;   // base address
    while (!(UART_BASE->US_CSR & AT91C_US_RXRDY)) ;
    return(UART_BASE->US_RHR) ;
#else // USE_DIRECT_USART
    char cc ;
    KS_dequeuew(MONITIPORT, &cc) ;
    return((int)(cc)) ;
#endif // USE_DIRECT_USART
}

