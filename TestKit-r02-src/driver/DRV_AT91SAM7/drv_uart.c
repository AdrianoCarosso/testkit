// drv_uart.c - uart driver tasks

//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//

//
// This module is provided as a serial port driver.  It
// provides for fully interrupt driven, full-duplex operation with
// all characters tunneling through input and output character queues.
//

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"      // COMnISEM, COMnOSEM
#include "cqueue.h"     // COMnIQ,   COMnOQ

#include "assign.h"

#define NULLSEMA ((SEMA)0)

#define TX_BUFFER       16      // size of DMA Tx buffer (0=noDMA)
#define RX_BUFFER       16      // size of one half DMA Rx buffer (0=noDMA)

//_FR_ 09/10/09
//#define TX_BUFFER       128      // size of DMA Tx buffer (0=noDMA)
//#define RX_BUFFER       128     // size of one half DMA Rx buffer (0=noDMA)

//----------------------------------------------------------------------------
// Sanity check

#if !defined(USE_TIMER_PIT) && !defined(USE_TIMER_PWM)
#error "One timer at least"
#endif
#if defined(USE_TIMER_PIT) && defined(USE_TIMER_PWM)
#error "Too many timer"
#endif

//----------------------------------------------------------------------------
// functions in drv_clk.c

extern void AT91F_AIC_Configure(int irq_id, int priority, int src_type, FRAME *(*newHandler)(FRAME *frame)) ;

//----------------------------------------------------------------------------
// internal functions

void uartdrv(void) TASK_ATTRIBUTE ;

void uartstart(int num, unsigned long baud_rate, int mode) ;
void uartstop(void) ;

FRAME *com0drv(FRAME * frame) ;
FRAME *com1drv(FRAME * frame) ;

#if defined(USE_AT91SAM7A3)
FRAME *com2drv(FRAME * frame) ;
#endif // defined(USE_AT91SAM7A3)

#ifdef USE_TIMER_PIT
void com3drvpit(void) ;
#else // USE_TIMER_PIT
FRAME *com3drv(FRAME * frame) ;
#endif // USE_TIMER_PIT

#ifdef RX_BUFFER
void uartDMAstartRx(AT91PS_USART puart, int num) ;
#endif // RX_BUFFER

#ifdef TX_BUFFER
void uartDMAstartTx(AT91PS_USART puart, unsigned char* ptr, int plen) ;
#endif // TX_BUFFER

//----------------------------------------------------------------------------

#define ERRMASK (AT91C_US_OVRE | AT91C_US_PARE | AT91C_US_FRAME | AT91C_US_RXBRK)

unsigned short com0err ;        // error counter
unsigned short com1err ;        // error counter
unsigned short com2err ;        // error counter
unsigned short com3err ;        // error counter

#ifdef RX_BUFFER
// remember that DBGU input port has no DMA in any case
// only COM 0,1,2 have, but COM 3 uses periodic timer tick
unsigned char rxbuffidx[4] ;    // input buffer index
unsigned char rxbuff[4][2 * RX_BUFFER] ;
unsigned char iptr[4] ;	        // interrupt RX char pointer
unsigned char cptr[4] ;	        // consumer RX char pointer
//unsigned short iptr[4] ;	        // interrupt RX char pointer
//unsigned short cptr[4] ;	        // consumer RX char pointer _FR_ 09/10/09
volatile unsigned char nptr[3] ;// num of RX chars

#else // RX_BUFFER

// ichar0 - 2 have different meaning in RX_BUFFER
unsigned char ichar0 ;	        // input char
unsigned char ichar1 ;	        // input char
unsigned char ichar2 ;	        // input char
unsigned char ichar3 ;	        // input char
#endif // RX_BUFFER

//----------------------------------------------------------------------------
// Interrupt routine for COM 0

FRAME *com0drv(FRAME * frame)
{
    register AT91PS_USART USART_pt = AT91C_BASE_US0 ;   // <-- COMx
    register unsigned int status ;
#ifdef RX_BUFFER
    register unsigned int ptr ;
    register int num ;
    const int comn = 0 ;                                // <-- COMx
#endif // RX_BUFFER

// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    // get Usart status register
    status = USART_pt->US_CSR ;
    
#ifdef RX_BUFFER
    // Check for receiver timeout
    if (status & (USART_pt->US_IMR) & AT91C_US_TIMEOUT) {
        USART_pt->US_CR = AT91C_US_STTTO ;
        ptr = ((rxbuffidx[comn]^1) * RX_BUFFER) + (RX_BUFFER - USART_pt->US_RCR) ;
        num = ptr - iptr[comn] ;
        if (num) {
            nptr[comn] += (unsigned char)(num) ;
            iptr[comn] = (unsigned char)(ptr) ;
            KS_ISRsignal(COM0ISEM) ;            // <-- COMx
        }
    }
    
    // Check for received buffer
    if (status & (USART_pt->US_IMR) & AT91C_US_ENDRX) {
        // change buffer
        ptr = rxbuffidx[comn] * RX_BUFFER ;
        num = ptr - iptr[comn] ;
        if (num < 0) num += (2 * RX_BUFFER) ;
        uartDMAstartRx(USART_pt, comn) ;
        nptr[comn] += (unsigned char)(num) ;
        iptr[comn] = (unsigned char)(ptr) ;
        KS_ISRsignal(COM0ISEM) ;                // <-- COMx
    }
#else // RX_BUFFER
    // Check for incoming chars
    if (status & AT91C_US_RXRDY) {
        // Get byte
        ichar0 = USART_pt->US_RHR ;             // <-- COMx
        KS_ISRsignal(COM0ISEM) ;                // <-- COMx
    }
#endif // RX_BUFFER

#ifdef TX_BUFFER
    // Check for transmitted buffer
    if (status & (USART_pt->US_IMR) & AT91C_US_ENDTX) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = AT91C_US_ENDTX ;
        KS_ISRsignal(COM0OSEM) ;                // <-- COMx
    }
#else // TX_BUFFER
    // Check for transmitted chars
    if (status & (USART_pt->US_IMR) & AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;
        KS_ISRsignal(COM0OSEM) ;                // <-- COMx
    }
#endif // TX_BUFFER

    // check for errors
    if (status & ERRMASK) {
        com0err++ ;                             // <-- COMx
        // Reset the satus bit
        USART_pt->US_CR = AT91C_US_RSTSTA ;
    }

    return(KS_ISRexit(frame, NULLSEMA)) ;
}

//----------------------------------------------------------------------------
// Interrupt routine for COM 1

FRAME *com1drv(FRAME * frame)
{
    register AT91PS_USART USART_pt = AT91C_BASE_US1 ;   // <-- COMx
    register unsigned int status ;
#ifdef RX_BUFFER
    register unsigned int ptr ;
    register int num ;
    const int comn = 1 ;                                // <-- COMx
#endif // RX_BUFFER

// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    // get Usart status register
    status = USART_pt->US_CSR ;

#ifdef RX_BUFFER
    // Check for receiver timeout
    if (status & (USART_pt->US_IMR) & AT91C_US_TIMEOUT) {
        USART_pt->US_CR = AT91C_US_STTTO ;
        ptr = ((rxbuffidx[comn]^1) * RX_BUFFER) + (RX_BUFFER - USART_pt->US_RCR) ;
        num = ptr - iptr[comn] ;
        if (num) {
            nptr[comn] += (unsigned char)(num) ;
            iptr[comn] = (unsigned char)(ptr) ;
            KS_ISRsignal(COM1ISEM) ;            // <-- COMx
        }
    }

    // Check for received buffer
    if (status & (USART_pt->US_IMR) & AT91C_US_ENDRX) {
        // change buffer
        ptr = rxbuffidx[comn] * RX_BUFFER ;
        num = ptr - iptr[comn] ;
        if (num < 0) num += (2 * RX_BUFFER) ;
        uartDMAstartRx(USART_pt, comn) ;
        nptr[comn] += (unsigned char)(num) ;
        iptr[comn] = (unsigned char)(ptr) ;
        KS_ISRsignal(COM1ISEM) ;                // <-- COMx
    }
#else // RX_BUFFER
    // Check for incoming chars
    if (status & AT91C_US_RXRDY) {
        // Get byte
        ichar1 = USART_pt->US_RHR ;             // <-- COMx
        KS_ISRsignal(COM1ISEM) ;                // <-- COMx
    }
#endif // RX_BUFFER

#ifdef TX_BUFFER
    // Check for transmitted buffer
    if (status & (USART_pt->US_IMR) & AT91C_US_ENDTX) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = AT91C_US_ENDTX ;
        KS_ISRsignal(COM1OSEM) ;                // <-- COMx
    }
#else // TX_BUFFER
    // Check for transmitted chars
    if (status & (USART_pt->US_IMR) & AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;
        KS_ISRsignal(COM1OSEM) ;                // <-- COMx
    }
#endif // TX_BUFFER

    // check for errors
    if (status & ERRMASK) {
        com1err++ ;                             // <-- COMx
        // Reset the satus bit
        USART_pt->US_CR = AT91C_US_RSTSTA ;
    }

    return(KS_ISRexit(frame, NULLSEMA)) ;
}

//----------------------------------------------------------------------------
// Interrupt routine for COM 2

#if defined(USE_AT91SAM7A3)
FRAME *com2drv(FRAME * frame)
{
    register AT91PS_USART USART_pt = AT91C_BASE_US2 ;   // <-- COMx
    register unsigned int status ;
#ifdef RX_BUFFER
    register unsigned int ptr ;
    register int num ;
    const int comn = 2 ;                                // <-- COMx
#endif // RX_BUFFER

// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    // get Usart status register
    status = USART_pt->US_CSR ;

#ifdef RX_BUFFER
    // Check for receiver timeout
    if (status & (USART_pt->US_IMR) & AT91C_US_TIMEOUT) {
        USART_pt->US_CR = AT91C_US_STTTO ;
        ptr = ((rxbuffidx[comn]^1) * RX_BUFFER) + (RX_BUFFER - USART_pt->US_RCR) ;
        num = ptr - iptr[comn] ;
        if (num) {
            nptr[comn] += (unsigned char)(num) ;
            iptr[comn] = (unsigned char)(ptr) ;
            KS_ISRsignal(COM2ISEM) ;            // <-- COMx
        }
    }

    // Check for received buffer
    if (status & (USART_pt->US_IMR) & AT91C_US_ENDRX) {
        // change buffer
        ptr = rxbuffidx[comn] * RX_BUFFER ;
        num = ptr - iptr[comn] ;
        if (num < 0) num += (2 * RX_BUFFER) ;
        uartDMAstartRx(USART_pt, comn) ;
        nptr[comn] += (unsigned char)(num) ;
        iptr[comn] = (unsigned char)(ptr) ;
        KS_ISRsignal(COM2ISEM) ;                // <-- COMx
    }
#else // RX_BUFFER
    // Check for incoming chars
    if (status & AT91C_US_RXRDY) {
        // Get byte
        ichar2 = USART_pt->US_RHR ;             // <-- COMx
        KS_ISRsignal(COM2ISEM) ;                // <-- COMx
    }
#endif // RX_BUFFER

#ifdef TX_BUFFER
    // Check for transmitted buffer
    if (status & (USART_pt->US_IMR) & AT91C_US_ENDTX) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = AT91C_US_ENDTX ;
        KS_ISRsignal(COM2OSEM) ;                // <-- COMx
    }
#else // TX_BUFFER
    // Check for transmitted chars
    if (status & (USART_pt->US_IMR) & AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;
        KS_ISRsignal(COM2OSEM) ;                // <-- COMx
    }
#endif // TX_BUFFER

    // check for errors
    if (status & ERRMASK) {
        com2err++ ;                             // <-- COMx
        // Reset the satus bit
        USART_pt->US_CR = AT91C_US_RSTSTA ;
    }

    return(KS_ISRexit(frame, NULLSEMA)) ;
}
#endif // defined(USE_AT91SAM7A3)

//----------------------------------------------------------------------------
// Interrupt routine for COM 3

#ifdef USE_TIMER_PIT
void com3drvpit(void)
#else // USE_TIMER_PIT
FRAME *com3drv(FRAME * frame)
#endif // USE_TIMER_PIT
{
    register AT91PS_USART USART_pt = (AT91PS_USART)(AT91C_BASE_DBGU) ;    // <-- COMx
    register unsigned int status ;
#ifdef RX_BUFFER
    register unsigned int ptr ;
    register int num ;
    const int comn = 3 ;                                // <-- COMx
#endif // RX_BUFFER

// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    // get Usart status register
    status = USART_pt->US_CSR ;

#ifdef RX_BUFFER
    // Check for receiver timeout
    // COM 3 has no timeout, but it is called every timer tick
    // check actual situation
    ptr = ((rxbuffidx[comn]^1) * RX_BUFFER) + (RX_BUFFER - USART_pt->US_RCR) ;
    num = ptr - iptr[comn] ;
    if (num) {
        nptr[comn] += (unsigned char)(num) ;
        iptr[comn] = (unsigned char)(ptr) ;
        KS_ISRsignal(COM3ISEM) ;                // <-- COMx
    }

    // Check for received buffer
    if (status & /*(USART_pt->US_IMR) &*/ AT91C_US_ENDRX) {
        // change buffer
        ptr = rxbuffidx[comn] * RX_BUFFER ;
        num = ptr - iptr[comn] ;
        if (num < 0) num += (2 * RX_BUFFER) ;
        uartDMAstartRx(USART_pt, comn) ;
        nptr[comn] += (unsigned char)(num) ;
        iptr[comn] = (unsigned char)(ptr) ;
        KS_ISRsignal(COM3ISEM) ;                // <-- COMx
    }
#else // RX_BUFFER
    // Check for incoming chars
    if (status & AT91C_US_RXRDY) {
        // Get byte
        ichar3 = USART_pt->US_RHR ;             // <-- COMx
        KS_ISRsignal(COM3ISEM) ;                // <-- COMx
    }
#endif // RX_BUFFER

#ifdef TX_BUFFER
    // Check for transmitted buffer
    if (status & (USART_pt->US_IMR) & AT91C_US_ENDTX) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = AT91C_US_ENDTX ;
        KS_ISRsignal(COM3OSEM) ;                // <-- COMx
    }
#else // TX_BUFFER
    // Check for transmitted chars
    if (status & (USART_pt->US_IMR) & AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;
        KS_ISRsignal(COM3OSEM) ;                // <-- COMx
    }
#endif // TX_BUFFER

    // check for errors
    if (status & ERRMASK) {
        com3err++ ;                             // <-- COMx
        // Reset the satus bit
        USART_pt->US_CR = AT91C_US_RSTSTA ;
    }

#ifdef USE_TIMER_PIT
    return ;                                    // return to "clkc"
#else // USE_TIMER_PIT
    return(KS_ISRexit(frame, NULLSEMA)) ;       // EOI
#endif // USE_TIMER_PIT
}

//----------------------------------------------------------------------------
// Usart DMA start TX

#ifdef TX_BUFFER
void uartDMAstartTx(AT91PS_USART puart, unsigned char* ptr, int plen)
{
    puart->US_TPR = (unsigned long)(ptr) ;      // TX buffer ptr
    puart->US_TCR = plen ;      // TX buffer len
    puart->US_TNCR = 0 ;        // next void
    
    puart->US_IER = AT91C_US_ENDTX ;
    puart->US_PTCR = AT91C_PDC_TXTEN ;
}
#endif // TX_BUFFER

//----------------------------------------------------------------------------
// Usart DMA start RX

#ifdef RX_BUFFER
void uartDMAstartRx(AT91PS_USART puart, int num)
{
    // next buffer
    rxbuffidx[num] ^= 1 ;

    // prepare next
    puart->US_RNPR = (unsigned long)(&rxbuff[num][rxbuffidx[num] * RX_BUFFER]) ; // RX buffer ptr
    puart->US_RNCR = RX_BUFFER ;        // RX buffer len
}
#endif // RX_BUFFER

//----------------------------------------------------------------------------
// Usart initializer

void uartstart(int num, unsigned long baud_rate, int mode)
{
    AT91PS_USART UART_BASE ;
    unsigned long baud_value ;
    int handshake ;
// _FR_
	SEMA outsema ;
// _FR_
    
    // handshake not allowed with debug port
    handshake = ((mode & AT91C_US_USMODE_HWHSH) && (num < 3)) ;
    
    // Select base address
    // Configure PIO controllers to periph mode
    switch(num) {
    case 0:
        UART_BASE = AT91C_BASE_US0 ;    // base address
        outsema = COM0OSEM ; // _FR_
#if defined(USE_AT91SAM7A3)
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA2_RXD0 | AT91C_PA3_TXD0) ;
        if (handshake) {
            // PIO A: Peripheral A select register
            AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA5_RTS0 | AT91C_PA6_CTS0) ;
        }
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA2_RXD0 | AT91C_PA3_TXD0) ;
        if (handshake) {
            // PIO A: Peripheral A select register
            AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA5_RTS0 | AT91C_PA6_CTS0) ;
        }
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA5_RXD0 | AT91C_PA6_TXD0) ;
        if (handshake) {
            // PIO A: Peripheral A select register
            AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA7_RTS0 | AT91C_PA8_CTS0) ;
        }
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA5_RXD0 | AT91C_PA6_TXD0) ;
        if (handshake) {
            // PIO A: Peripheral A select register
            AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA7_RTS0 | AT91C_PA8_CTS0) ;
        }
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        break ;
        
    case 1:
        UART_BASE = AT91C_BASE_US1 ;    // base address
        outsema = COM1OSEM ; // _FR_
#if defined(USE_AT91SAM7A3)
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA7_RXD1 | AT91C_PA8_TXD1) ;
        if (handshake) {
            // PIO B: Peripheral B select register
            AT91C_BASE_PIOB->PIO_BSR = (unsigned long)(AT91C_PB24_RTS1 | AT91C_PB25_CTS1) ;
        }
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA7_RXD1 | AT91C_PA8_TXD1) ;
        if (handshake) {
            // PIO B: disable register
            AT91C_BASE_PIOB->PIO_PDR = (unsigned long)(AT91C_PB24_RTS1 | AT91C_PB25_CTS1) ;
        }
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA21_RXD1 | AT91C_PA22_TXD1) ;
        if (handshake) {
            // PIO A: Peripheral A select register
            AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA24_RTS1 | AT91C_PA25_CTS1) ;
        }
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA21_RXD1 | AT91C_PA22_TXD1) ;
        if (handshake) {
            // PIO A: disable register
            AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA24_RTS1 | AT91C_PA25_CTS1) ;
        }
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        break ;
        
#if defined(USE_AT91SAM7A3)
    case 2:
        UART_BASE = AT91C_BASE_US2 ;    // base address
        outsema = COM2OSEM ; // _FR_
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA9_RXD2 | AT91C_PA10_TXD2) ;
        if (handshake) {
            // PIO B: Peripheral B select register
            AT91C_BASE_PIOB->PIO_BSR = (unsigned long)(AT91C_PB27_RTS2 | AT91C_PB28_CTS2) ;
        }
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA9_RXD2 | AT91C_PA10_TXD2) ;
        if (handshake) {
            // PIO B: disable register
            AT91C_BASE_PIOB->PIO_PDR = (unsigned long)(AT91C_PB27_RTS2 | AT91C_PB28_CTS2) ;
        }
        break ;
#endif // defined(USE_AT91SAM7A3)

    case 3:     // Handshake not possible
        UART_BASE = (AT91PS_USART) AT91C_BASE_DBGU ;   // base address
        outsema = COM3OSEM ; // _FR_
#if defined(USE_AT91SAM7A3)
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA31_DTXD | AT91C_PA30_DRXD) ;
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA31_DTXD | AT91C_PA30_DRXD) ;
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA10_DTXD | AT91C_PA9_DRXD) ;
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA10_DTXD | AT91C_PA9_DRXD) ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        break ;
        
    default :
        return ;  // error
    }
    
    // Enable Peripheral Clock
    if (num < 3) {      // DBGU is always clocked
        // Peripheral Clock Enable Register
        AT91C_BASE_PMC->PMC_PCER = (1 << (AT91C_ID_US0 + num)) ;
    }
    
    // AT91F_US_Configure();
    // Disable interrupts
    UART_BASE->US_IDR = (unsigned long) -1;

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

    if (num < 3) {      // only if NOT DBGU
        // Write the TX Timeguard register
        UART_BASE->US_TTGR = 0 ;

#ifdef RX_BUFFER
        // Write the RX Timeout register
        UART_BASE->US_RTOR = 200 ;      // 9600=20ms, 115200=2ms
#endif // RX_BUFFER
    }
    
#ifdef RX_BUFFER
    // init RX DMA
    rxbuffidx[num] = 0 ;                // first buffer
    UART_BASE->US_RPR = (unsigned long)(&rxbuff[num][rxbuffidx[num] * RX_BUFFER]) ; // RX buffer ptr
    UART_BASE->US_RCR = RX_BUFFER ;     // RX buffer len
    iptr[num] = 0 ;
    cptr[num] = 0 ;
    nptr[num] = 0 ;

    // prepare next buffer
    uartDMAstartRx(UART_BASE, num) ;
    // enable DMA
    UART_BASE->US_PTCR = AT91C_PDC_RXTEN ;
#endif // RX_BUFFER

    // Define the USART mode
    UART_BASE->US_MR = mode |
                       /* ((num==0) ? AT91C_US_CHMODE_LOCAL : 0) |  DEBUG ONLY LOOPBACK */
                       AT91C_US_CLKS_CLOCK  ;
	if (mode & 0x1){
        UART_BASE->US_TTGR = 10 ; 		        // Write the TX Timeguard register

	}
    // Enable Rx Tx
    UART_BASE->US_CR =
#ifdef RX_BUFFER
                       AT91C_US_STTTO | // ignored by DBGU port
#endif // RX_BUFFER
                       AT91C_US_RXEN | AT91C_US_TXEN ;
                        
    // open USART interrupt
    switch(num) {
    case 0 :
        AT91F_AIC_Configure(AT91C_ID_US0, USART_0_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, com0drv) ;
        break ;

    case 1 :
        AT91F_AIC_Configure(AT91C_ID_US1, USART_1_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, com1drv) ;
        break ;

#if defined(USE_AT91SAM7A3)
    case 2 :
        AT91F_AIC_Configure(AT91C_ID_US2, USART_2_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, com2drv) ;
        break ;
#endif // defined(USE_AT91SAM7A3)

#if defined(USE_TIMER_PWM)
    case 3 :
        // DBGU unit has same interrupt than PIT. Avoid it together
        AT91F_AIC_Configure(AT91C_ID_SYS, USART_3_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, com3drv) ;
        break ;
#endif // USE_TIMER_PWM
    }

    // interrupt enable
#ifdef RX_BUFFER
    if (num < 3) {      // only if NOT DBGU
        // Enable USART interrupts: errors + RX buffer full + Timeout
        UART_BASE->US_IER = AT91C_US_FRAME | AT91C_US_OVRE | AT91C_US_RXBRK | AT91C_US_ENDRX | AT91C_US_TIMEOUT ;
    } else {            // if DBGU
#if defined(USE_TIMER_PWM)
        // only if DBGU is interrupt owner
        // Enable USART interrupts: errors and RXRDY
        UART_BASE->US_IER = AT91C_US_FRAME | AT91C_US_OVRE | AT91C_US_RXBRK | AT91C_US_RXRDY ;
#endif // USE_TIMER_PWM
    }
#else // RX_BUFFER
#if defined(USE_TIMER_PWM)
    // Enable USART interrupts: errors and RXRDY
    UART_BASE->US_IER = AT91C_US_FRAME | AT91C_US_OVRE | AT91C_US_RXBRK | AT91C_US_RXRDY ;
#else // USE_TIMER_PWM
    if (num < 3) {      // only if NOT DBGU
        // Enable USART interrupts: errors and RXRDY
        UART_BASE->US_IER = AT91C_US_FRAME | AT91C_US_OVRE | AT91C_US_RXBRK | AT91C_US_RXRDY ;
    }
#endif // USE_TIMER_PWM
#endif // RX_BUFFER

// _FR_
    KS_ISRsignal(outsema) ;                // <-- COMx

}

//----------------------------------------------------------------------------
// COM driver task RX

void uartdrv(void)
{
    register int i ;
    SEMA cause ;                // wake up reason
    SEMA semalist[(3*4) + 1] ;
#ifdef TX_BUFFER
    static unsigned char txbuff0[TX_BUFFER] ;
    static unsigned char txbuff1[TX_BUFFER] ;
#if defined(USE_AT91SAM7A3)
    static unsigned char txbuff2[TX_BUFFER] ;
#endif // defined(USE_AT91SAM7A3)
    static unsigned char txbuff3[TX_BUFFER] ;
#else // TX_BUFFER
    unsigned char ochar ;       // output char
#endif // TX_BUFFER
    unsigned char flag_full ;   // what to wait

    // Assign sema at queue: wake up on no empty
    KS_defqsema(COM0OQ, COM0QSEM, QNE) ;                        // <-- COMx
    KS_defqsema(COM1OQ, COM1QSEM, QNE) ;                        // <-- COMx
#if defined(USE_AT91SAM7A3)
    KS_defqsema(COM2OQ, COM2QSEM, QNE) ;                        // <-- COMx
#endif // defined(USE_AT91SAM7A3)
    KS_defqsema(COM3OQ, COM3QSEM, QNE) ;                        // <-- COMx

    // init semphore constants
    i = 0 ;             // starting point
    semalist[i++] = COM0ISEM ;
    semalist[i++] = COM1ISEM ;
#if defined(USE_AT91SAM7A3)
    semalist[i++] = COM2ISEM ;
#endif // defined(USE_AT91SAM7A3)
    semalist[i++] = COM3ISEM ;
    semalist[i++] = COM0OSEM ;
    semalist[i++] = COM1OSEM ;
#if defined(USE_AT91SAM7A3)
    semalist[i++] = COM2OSEM ;
#endif // defined(USE_AT91SAM7A3)
    semalist[i++] = COM3OSEM ;

    // true for everyone at beginning
    flag_full = 0xf ;

    for( ; ; ) {
#if defined(USE_AT91SAM7A3)
        i = 8 ;                 // starting point
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        i = 6 ;                 // starting point
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

        if (flag_full & 1) semalist[i++] = COM0QSEM ;
        if (flag_full & 2) semalist[i++] = COM1QSEM ;
#if defined(USE_AT91SAM7A3)
        if (flag_full & 4) semalist[i++] = COM2QSEM ;
#endif // defined(USE_AT91SAM7A3)
        if (flag_full & 8) semalist[i++] = COM3QSEM ;
        semalist[i] = 0 ;       // terminator

        cause = KS_waitm(semalist) ;   // don't waste time
        switch(cause) { 	       // I know who waked me up
        // COM 0
        case COM0ISEM :                 // data arrived         // <-- COMx
#ifdef RX_BUFFER
            while(nptr[0]) {                                    // <-- COMx
                if (KS_enqueue(COM0IQ, &rxbuff[0][cptr[0]++]) != RC_GOOD) { // <-- COMx
	            // one more error
	            com0err++ ;                                 // <-- COMx
	        }
	        if (cptr[0] >= (2*RX_BUFFER)) {                 // <-- COMx
                    cptr[0] = 0 ;                               // <-- COMx
                }
                DISABLE ;
                nptr[0]-- ;
                ENABLE ;
            }
#else // RX_BUFFER
	    // check queue status
	    if (KS_enqueue(COM0IQ, &ichar0) != RC_GOOD) {       // <-- COMx
	        // one more error
	        com0err++ ;                                     // <-- COMx
	    }
#endif // RX_BUFFER
	    break ;

        // COM 1
        case COM1ISEM :                 // data arrived         // <-- COMx
#ifdef RX_BUFFER
            while(nptr[1]) {                                    // <-- COMx
                if (KS_enqueue(COM1IQ, &rxbuff[1][cptr[1]++]) != RC_GOOD) { // <-- COMx
	            // one more error
	            com1err++ ;                                 // <-- COMx
	        }
	        if (cptr[1] >= (2*RX_BUFFER)) {                 // <-- COMx
                    cptr[1] = 0 ;                               // <-- COMx
                }
                DISABLE ;
                nptr[1]-- ;
                ENABLE ;
            }
#else // RX_BUFFER
	    // check queue status
	    if (KS_enqueue(COM1IQ, &ichar1) != RC_GOOD) {       // <-- COMx
	        // one more error
	        com1err++ ;                                     // <-- COMx
	    }
#endif // RX_BUFFER
	    break ;

#if defined(USE_AT91SAM7A3)
        // COM 2
        case COM2ISEM :                 // data arrived         // <-- COMx
#ifdef RX_BUFFER
            while(nptr[2]) {                                    // <-- COMx
                if (KS_enqueue(COM2IQ, &rxbuff[2][cptr[2]++]) != RC_GOOD) { // <-- COMx
	            // one more error
	            com2err++ ;                                 // <-- COMx
	        }
	        if (cptr[2] >= (2*RX_BUFFER)) {                 // <-- COMx
                    cptr[2] = 0 ;                               // <-- COMx
                }
                DISABLE ;
                nptr[2]-- ;
                ENABLE ;
            }
#else // RX_BUFFER
	    // check queue status
	    if (KS_enqueue(COM2IQ, &ichar2) != RC_GOOD) {       // <-- COMx
	        // one more error
	        com2err++ ;                                     // <-- COMx
	    }
#endif // RX_BUFFER
	    break ;
#endif // defined(USE_AT91SAM7A3)

        // COM 3
        case COM3ISEM :                 // data arrived         // <-- COMx
#ifdef RX_BUFFER
            while(nptr[3]) {                                    // <-- COMx
                if (KS_enqueue(COM3IQ, &rxbuff[3][cptr[3]++]) != RC_GOOD) { // <-- COMx
	            // one more error
	            com3err++ ;                                 // <-- COMx
	        }
	        if (cptr[3] >= (2*RX_BUFFER)) {                 // <-- COMx
                    cptr[3] = 0 ;                               // <-- COMx
                }
                DISABLE ;
                nptr[3]-- ;
                ENABLE ;
            }
#else // RX_BUFFER
	    // check queue status
	    if (KS_enqueue(COM3IQ, &ichar3) != RC_GOOD) {       // <-- COMx
	        // one more error
	        com3err++ ;                                     // <-- COMx
	    }
#endif // RX_BUFFER
	    break ;

        // COM 0
        case COM0QSEM :	               // data to TX            // <-- COMx
#ifdef TX_BUFFER
            for(i=0 ; i<TX_BUFFER ; i++) {
                if (KS_dequeue(COM0OQ, &txbuff0[i]) != RC_GOOD) {    // <-- COMx
                    break ;
                }
            }
            uartDMAstartTx(AT91C_BASE_US0, txbuff0, i) ;        // <-- COMx
#else // TX_BUFFER
            KS_dequeue(COM0OQ, &ochar) ;                        // <-- COMx

            DISABLE ;
            // write char to port
            AT91C_BASE_US0->US_THR = ochar ;                  // <-- COMx
            // Enable the interrupt source for TX
            AT91C_BASE_US0->US_IER = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;      // <-- COMx
            ENABLE ;
#endif // TX_BUFFER
            flag_full &= (~1) ;
            break ;

        case COM0OSEM :                 // data sent            // <-- COMx
            flag_full |= 1 ;
            break ;

        // COM 1
        case COM1QSEM :	               // data to TX            // <-- COMx
#ifdef TX_BUFFER
            for(i=0 ; i<TX_BUFFER ; i++) {
                if (KS_dequeue(COM1OQ, &txbuff1[i]) != RC_GOOD) {    // <-- COMx
                    break ;
                }
            }
            uartDMAstartTx(AT91C_BASE_US1, txbuff1, i) ;        // <-- COMx
#else // TX_BUFFER
            KS_dequeue(COM1OQ, &ochar) ;                        // <-- COMx

            DISABLE ;
            // write char to port
            AT91C_BASE_US1->US_THR = ochar ;                  // <-- COMx
            // Enable the interrupt source for TX
            AT91C_BASE_US1->US_IER = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;      // <-- COMx
            ENABLE ;
#endif // TX_BUFFER
            flag_full &= (~2) ;
            break ;

        case COM1OSEM :                 // data sent            // <-- COMx
            flag_full |= 2 ;
            break ;

#if defined(USE_AT91SAM7A3)
        // COM 2
        case COM2QSEM :	               // data to TX            // <-- COMx
#ifdef TX_BUFFER
            for(i=0 ; i<TX_BUFFER ; i++) {
                if (KS_dequeue(COM2OQ, &txbuff2[i]) != RC_GOOD) {    // <-- COMx
                    break ;
                }
            }
            uartDMAstartTx(AT91C_BASE_US2, txbuff2, i) ;        // <-- COMx
#else // TX_BUFFER
            KS_dequeue(COM2OQ, &ochar) ;                        // <-- COMx

            DISABLE ;
            // write char to port
            //AT91C_BASE_US2->DBGU_THR = ochar ;                  // <-- COMx
            AT91C_BASE_US2->US_THR = ochar ;                  // <-- COMx
            // Enable the interrupt source for TX
            //AT91C_BASE_US2->DBGU_IER = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;      // <-- COMx
            AT91C_BASE_US2->US_IER = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;      // <-- COMx
            ENABLE ;
#endif // TX_BUFFER
            flag_full &= (~4) ;
            break ;

        case COM2OSEM :                 // data sent            // <-- COMx
            flag_full |= 4 ;
            break ;
#endif // defined(USE_AT91SAM7A3)

        // COM 3
        case COM3QSEM :	               // data to TX            // <-- COMx
#ifdef TX_BUFFER
            for(i=0 ; i<TX_BUFFER ; i++) {
                if (KS_dequeue(COM3OQ, &txbuff3[i]) != RC_GOOD) {    // <-- COMx
                    break ;
                }
            }
            uartDMAstartTx((AT91PS_USART)(AT91C_BASE_DBGU), txbuff3, i) ; // <-- COMx
#else // TX_BUFFER
            KS_dequeue(COM3OQ, &ochar) ;                        // <-- COMx

            DISABLE ;
            // write char to port
            AT91C_BASE_DBGU->DBGU_THR = ochar ;                 // <-- COMx
            // Enable the interrupt source for TX
            AT91C_BASE_DBGU->DBGU_IER = AT91C_US_TXRDY /*AT91C_US_TXEMPTY*/ ;      // <-- COMx
            ENABLE ;
#endif // TX_BUFFER
            flag_full &= (~8) ;
            break ;

        case COM3OSEM :                 // data sent            // <-- COMx
            flag_full |= 8 ;
            break ;
        }
    }
}

//----------------------------------------------------------------------------
// Usart terminator

void uartstop(void)
{
    // disable UART -0- if enabled
    if (AT91C_BASE_PMC->PMC_PCSR & (1 << (AT91C_ID_US0))) {
        // Disable interrupts
        AT91C_BASE_US0->US_IDR = (unsigned long) -1;

        // Reset receiver and transmitter
        AT91C_BASE_US0->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS ;
        
        // disable clock
        AT91C_BASE_PMC->PMC_PCDR = (1 << (AT91C_ID_US0)) ;
        
//        // Set RTS as Output at 1
//        AT91C_BASE_PIOA->PIO_SODR = AT91C_PA5_RTS0 ;
//        AT91C_BASE_PIOA->PIO_PER  = AT91C_PA5_RTS0 ;
//        AT91C_BASE_PIOA->PIO_OER  = AT91C_PA5_RTS0 ;
    }

    // disable UART -1- if enabled
    if (AT91C_BASE_PMC->PMC_PCSR & (1 << (AT91C_ID_US1))) {
        // Disable interrupts
        AT91C_BASE_US1->US_IDR = (unsigned long) -1;

        // Reset receiver and transmitter
        AT91C_BASE_US1->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS ;

        // disable clock
        AT91C_BASE_PMC->PMC_PCDR = (1 << (AT91C_ID_US1)) ;
    }

#if defined(USE_AT91SAM7A3)
    // disable UART -2- if enabled
    if (AT91C_BASE_PMC->PMC_PCSR & (1 << (AT91C_ID_US2))) {
        // Disable interrupts
        AT91C_BASE_US2->US_IDR = (unsigned long) -1;

        // Reset receiver and transmitter
        AT91C_BASE_US2->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS ;

        // disable clock
        AT91C_BASE_PMC->PMC_PCDR = (1 << (AT91C_ID_US2)) ;
    }
#endif // defined(USE_AT91SAM7A3)

    // disable UART -3- DBGU, always enabled
    // Disable interrupts
    AT91C_BASE_DBGU->DBGU_IDR = (unsigned long) -1;

    // Reset receiver and transmitter
    AT91C_BASE_DBGU->DBGU_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS ;
}
// end of file - drv_uart.c

