// RTXCsys.c
//
//   Copyright (c) 1997-2007.
//   T.E.S.T. srl
//

#include "typedef.h"
#include "cclock.h"     // option definition
#include "rtxcapi.h"

#include "assign.h"

// -----------------------------------------------------------------------------
// internal functions prototype

void AT91F_WATCHDOGinit(void) ;

#ifdef CBUG
void AT91F_DataAbort(int apc, int type) ;
#endif // CBUG

//----------------------------------------------------------------------------
// AT91F_WATCHDOGinit
// This function performs very low level HW WATCHDOG initialization

void AT91F_WATCHDOGinit(void)
{
#ifdef USE_WATCHDOG
    // Enable Watchdog (write once register)
    // Max time: 16 sec., Reset all
    
    // Modified FR 10/05/2010
    AT91C_BASE_WDTC->WDTC_WDMR = (0xfff << 0) | (0xfff<<16) | AT91C_WDTC_WDRSTEN | AT91C_WDTC_WDDBGHLT ;
   
//    AT91C_BASE_WDTC->WDTC_WDMR = (0xfff << 0) | (0xfff<<16) | AT91C_WDTC_WDRSTEN | 
//#if defined(CBUG) || defined(USE_LOW_POWER)
//                                  | AT91C_WDTC_WDDBGHLT | AT91C_WDTC_WDIDLEHLT
//#endif // defined(CBUG) || defined(USE_LOW_POWER)
//                                ;
#else // USE_WATCHDOG
    // Disable Watchdog (write once register)
    AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS ;
#endif // USE_WATCHDOG
}

//----------------------------------------------------------------------------
// AT91F_DataAbort
// Show essential data at Data Abort

#ifdef CBUG
static void PrintHeader(const char *p)
{
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    AT91PS_USART UART_BASE = AT91C_BASE_US1 ;   // base address
#endif // USE_REAL_BOARD
#ifdef USE_EVALUATION_BOARD
    AT91PS_USART UART_BASE = AT91C_BASE_DBGU ;  // base address
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91PS_USART UART_BASE = AT91C_BASE_US0 ;   // base address
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

    while(*p) {
        while (!(UART_BASE->US_CSR & AT91C_US_TXRDY)) ;
        UART_BASE->US_THR = *p++ ;
    }
}

static void PrintHex(unsigned long v)
{
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    AT91PS_USART UART_BASE = AT91C_BASE_US1 ;   // base address
#endif // USE_REAL_BOARD
#ifdef USE_EVALUATION_BOARD
    AT91PS_USART UART_BASE = AT91C_BASE_DBGU ;  // base address
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91PS_USART UART_BASE = AT91C_BASE_US0 ;   // base address
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    int i ;
    char c ;
    
    for(i=0 ; i<8 ; i++) {
        c = (v>>(4*(7-i))) & 0xf ;
        if (c < 10) {
            c += '0' ;
        } else {
            c += ('A' - 10) ;
        }
        while (!(UART_BASE->US_CSR & AT91C_US_TXRDY)) ;
        UART_BASE->US_THR = c ;
    }
}

//unsigned long lastirq ;
unsigned long regcopy[15] ;

void AT91F_DataAbort(int apc, int type)
{
    const char *p1 = "\r\n*** Abort " ;
    const char *p2 = ": PC=" ;
    const char *p3 = ", MC_ASR=" ;
    const char *p4 = ", MC_AASR=" ;
    const char *p5 = " ***\r\n" ;
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    AT91PS_USART UART_BASE = AT91C_BASE_US1 ;   // base address
#endif // USE_REAL_BOARD
#ifdef USE_EVALUATION_BOARD
    AT91PS_USART UART_BASE = AT91C_BASE_DBGU ;  // base address
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91PS_USART UART_BASE = AT91C_BASE_US0 ;   // base address
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    int i ;
    
    // disable DMA on debug port
    UART_BASE->US_TCR = 0 ;

    // write header -1-
    PrintHeader(p1) ;

    // write type
    PrintHex(type) ;

    // write header -2-
    PrintHeader(p2) ;

    // write PC
    PrintHex(apc) ;

    // write header -3-
    PrintHeader(p3) ;

    // write MC Abort Status Register
    PrintHex(AT91C_BASE_MC->MC_ASR) ;

    // write header -4-
    PrintHeader(p4) ;

    // write MC Abort Address Status Register
    PrintHex(AT91C_BASE_MC->MC_AASR) ;

// +++++++++++++++++++++++++++++++++
#ifdef NODEF
{
    extern char isrmax ;
    const char *px = ", isrmax=" ;

    // write header
    PrintHeader(px) ;
    // (int)(isrmax)
    PrintHex((int)(isrmax)) ;
}
{
    const char *px = ", lstIRQ=" ;

    // write header
    PrintHeader(px) ;
    PrintHex(lastirq) ;
}
#endif //  NODEF
// +++++++++++++++++++++++++++++++++
{
    const char *pr = "*** R" ;
    const char *pe = "=" ;

    // write header -5-
    PrintHeader(p5) ;

    // write registers
    for(i=0 ; i<15 ; i++) {
        PrintHeader(pr) ;
        PrintHex(i) ;
        PrintHeader(pe) ;
        PrintHex(regcopy[i]) ;
        PrintHeader(p5) ;
    }
}

// +++++++++++++++++++++++++++++++++
{
    // write stacks
    extern const TASK ntasks ;
    for(i=1 ; i <= ntasks ; i++) {
        extern int UsedStackEval(unsigned long *sstart, int ssize) ;

        PrintHex(*((unsigned long *)(rtxkktcb[i].stackbase))) ;
        //PrintHex(UsedStackEval((unsigned long *)(rtxkktcb[i].stackbase), rtxkktcb[i].stacksize)) ;
        PrintHeader(p5) ;
   }
}
// +++++++++++++++++++++++++++++++++

    // wait for watchdog
    for( ; ; ) ;
}
#endif // CBUG
// end of file - RTXCsys.c

