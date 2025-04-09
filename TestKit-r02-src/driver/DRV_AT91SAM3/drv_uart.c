// drv_uart.c - uart driver tasks

//
//   Copyright (c) 1997-2010.
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

#define TX_BUFFER       32      // size of DMA Tx buffer (0=noDMA)
#define RX_BUFFER       32      // size of one half DMA Rx buffer (0=noDMA)


//----------------------------------------------------------------------------
// internal functions

void uartdrv(void) TASK_ATTRIBUTE ;

void uartstart(int num, unsigned long baud_rate, int mode) ;
void uartstop(void) ;

#ifdef RX_BUFFER
void uartDMAstartRx(Usart *puart, int num) ;
#endif // RX_BUFFER

#ifdef TX_BUFFER
void uartDMAstartTx(Usart *puart, unsigned char* ptr, int plen) ;
#endif // TX_BUFFER

//----------------------------------------------------------------------------

#define ERRMASK (US_CSR_OVRE | US_CSR_PARE | US_CSR_FRAME | US_CSR_RXBRK)

unsigned short com0err ;        // error counter
unsigned short com1err ;        // error counter
unsigned short com2err ;        // error counter
unsigned short com3err ;        // error counter

#ifdef RX_BUFFER
// remember that DBGU input port has no DMA in any case
// only COM 0,1
unsigned char rxbuffidx[2] ;    // input buffer index
unsigned char rxbuff[2][2 * RX_BUFFER] ;
unsigned char iptr[2] ;	        // interrupt RX char pointer
unsigned char cptr[2] ;	        // consumer RX char pointer
volatile unsigned char nptr[2] ;// num of RX chars

#else // RX_BUFFER

// ichar0 - 1 have different meaning in RX_BUFFER
unsigned char ichar0 ;	        // input char
unsigned char ichar1 ;	        // input char
#endif // RX_BUFFER

unsigned char ichar3 ;	        // input char

//----------------------------------------------------------------------------
// Interrupt routine for COM 0

void USART0_IrqHandler(void)
{
    register Usart *USART_pt = USART0 ;     // <-- COMx
    register unsigned int status ;
#ifdef RX_BUFFER
    register unsigned int ptr ;
    register int num ;
    const int comn = 0 ;                    // <-- COMx
#endif // RX_BUFFER


    // get Usart status register
    status = USART_pt->US_CSR ;
    
#ifdef RX_BUFFER
    // Check for receiver timeout
    if (status & (USART_pt->US_IMR) & US_IMR_TIMEOUT) {
        USART_pt->US_CR = US_CR_STTTO ;
        ptr = ((rxbuffidx[comn]^1) * RX_BUFFER) + (RX_BUFFER - USART_pt->US_RCR) ;
        num = ptr - iptr[comn] ;
        if (num) {
            nptr[comn] += (unsigned char)(num) ;
            iptr[comn] = (unsigned char)(ptr) ;
            KS_ISRsignal(COM0ISEM) ;            // <-- COMx
            ASK_CONTEXTSWITCH ; // set PendSV
        }
    }
    
    // Check for received buffer
    if (status & (USART_pt->US_IMR) & US_IMR_ENDRX) {
        // change buffer
        ptr = rxbuffidx[comn] * RX_BUFFER ;
        num = ptr - iptr[comn] ;
        if (num < 0) num += (2 * RX_BUFFER) ;
        uartDMAstartRx(USART_pt, comn) ;
        nptr[comn] += (unsigned char)(num) ;
        iptr[comn] = (unsigned char)(ptr) ;
        KS_ISRsignal(COM0ISEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }
#else // RX_BUFFER
    // Check for incoming chars
    if (status & US_CSR_RXRDY) {
        // Get byte
        ichar0 = USART_pt->US_RHR ;             // <-- COMx
        KS_ISRsignal(COM0ISEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }
#endif // RX_BUFFER

#ifdef TX_BUFFER
    // Check for transmitted buffer
    if (status & (USART_pt->US_IMR) & US_IMR_ENDTX) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = US_IDR_ENDTX ;
        KS_ISRsignal(COM0OSEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }
#else // TX_BUFFER
    // Check for transmitted chars
    if (status & (USART_pt->US_IMR) & US_IMR_TXRDY /*US_IMR_TXEMPTY*/) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = US_IDR_TXRDY /*US_IDR_TXEMPTY*/ ;
        KS_ISRsignal(COM0OSEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }
#endif // TX_BUFFER

    // check for errors
    if (status & ERRMASK) {
        com0err++ ;                             // <-- COMx
        // Reset the satus bit
        USART_pt->US_CR = US_CR_RSTSTA ;
    }
}

//----------------------------------------------------------------------------
// Interrupt routine for COM 1

void USART1_IrqHandler(void){
    register Usart *USART_pt = USART1 ;     // <-- COMx
    register unsigned int status ;
#ifdef RX_BUFFER
    register unsigned int ptr ;
    register int num ;
    const int comn = 1 ;                    // <-- COMx
#endif // RX_BUFFER


    // get Usart status register
    status = USART_pt->US_CSR ;
    
#ifdef RX_BUFFER
    // Check for receiver timeout
    if (status & (USART_pt->US_IMR) & US_IMR_TIMEOUT) {
        USART_pt->US_CR = US_CR_STTTO ;
        ptr = ((rxbuffidx[comn]^1) * RX_BUFFER) + (RX_BUFFER - USART_pt->US_RCR) ;
        num = ptr - iptr[comn] ;
        if (num) {
            nptr[comn] += (unsigned char)(num) ;
            iptr[comn] = (unsigned char)(ptr) ;
            KS_ISRsignal(COM1ISEM) ;            // <-- COMx
            ASK_CONTEXTSWITCH ; // set PendSV
        }
    }
    
    // Check for received buffer
    if (status & (USART_pt->US_IMR) & US_IMR_ENDRX) {
        // change buffer
        ptr = rxbuffidx[comn] * RX_BUFFER ;
        num = ptr - iptr[comn] ;
        if (num < 0) num += (2 * RX_BUFFER) ;
        uartDMAstartRx(USART_pt, comn) ;
        nptr[comn] += (unsigned char)(num) ;
        iptr[comn] = (unsigned char)(ptr) ;
        KS_ISRsignal(COM1ISEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }
#else // RX_BUFFER
    // Check for incoming chars
    if (status & US_CSR_RXRDY) {
        // Get byte
        ichar1 = USART_pt->US_RHR ;             // <-- COMx
        KS_ISRsignal(COM1ISEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }
#endif // RX_BUFFER

#ifdef TX_BUFFER
    // Check for transmitted buffer
    if (status & (USART_pt->US_IMR) & US_IMR_ENDTX) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = US_IDR_ENDTX ;
        KS_ISRsignal(COM1OSEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }
#else // TX_BUFFER
    // Check for transmitted chars
    if (status & (USART_pt->US_IMR) & US_IMR_TXRDY /*US_IMR_TXEMPTY*/) {
        // Disable the interrupt source for TX
        USART_pt->US_IDR = US_IDR_TXRDY /*US_IDR_TXEMPTY*/ ;
        KS_ISRsignal(COM1OSEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }
#endif // TX_BUFFER

    // check for errors
    if (status & ERRMASK) {
        com1err++ ;                             // <-- COMx
        // Reset the satus bit
        USART_pt->US_CR = US_CR_RSTSTA ;
    }
}

//----------------------------------------------------------------------------
// Interrupt routine for COM 3

void UART0_IrqHandler(void)
{
    register Uart *UART_pt = UART0 ;                    // <-- COMx
    register unsigned int status ;

    // get Uart status register
    status = UART_pt->UART_SR ;

    // Check for incoming chars
    if (status & UART_SR_RXRDY) {
        // Get byte
        ichar3 = UART_pt->UART_RHR ;            // <-- COMx
        KS_ISRsignal(COM3ISEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }

    // Check for transmitted chars
    if (status & (UART_pt->UART_IMR) & UART_IMR_TXRDY /*UART_IMR_TXEMPTY*/) {
        // Disable the interrupt source for TX
        UART_pt->UART_IDR = UART_IDR_TXRDY /*UART_IDR_TXEMPTY*/ ;
        KS_ISRsignal(COM3OSEM) ;                // <-- COMx
        ASK_CONTEXTSWITCH ; // set PendSV
    }

    // check for errors
    if (status & ERRMASK) {
        com3err++ ;                             // <-- COMx
        // Reset the satus bit
        UART_pt->UART_CR = UART_CR_RSTSTA ;
    }
}

//----------------------------------------------------------------------------
// Usart DMA start TX

#ifdef TX_BUFFER
void uartDMAstartTx(Usart *puart, unsigned char* ptr, int plen)
{
    puart->US_TPR = (unsigned long)(ptr) ;      // TX buffer ptr
    puart->US_TCR = plen ;      // TX buffer len
    puart->US_TNCR = 0 ;        // next void
    
    puart->US_IER = US_IER_ENDTX ;
    puart->US_PTCR = US_PTCR_TXTEN ;
}
#endif // TX_BUFFER

//----------------------------------------------------------------------------
// Usart DMA start RX

#ifdef RX_BUFFER
void uartDMAstartRx(Usart *puart, int num)
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
    Usart *UART_BASE ;
    unsigned long baud_value ;
    int handshake ;
	SEMA outsema ;  // _FR_
    
    // handshake not allowed with debug port
    handshake = ((mode & US_MR_USART_MODE_HWHSH) && (num < 3)) ;
    
    // Select base address
    // Configure PIO controllers to periph mode
    switch(num) {
    case 0:
        UART_BASE = USART0 ;        // base address
        outsema = COM0OSEM ;        // _FR_
        PMC->PMC_PCER0 = (1 << ID_USART0) ;
        // PIO A: Peripheral A select register: 00
        PIOA->PIO_ABCDSR[0] &= (~(unsigned long)(PIO_PA5A_RXD0 | PIO_PA6A_TXD0)) ;
        PIOA->PIO_ABCDSR[1] &= (~(unsigned long)(PIO_PA5A_RXD0 | PIO_PA6A_TXD0)) ;
        if (handshake) {
            // PIO A: Peripheral A select register
            PIOA->PIO_ABCDSR[0] &= (~(unsigned long)(PIO_PA7A_RTS0 | PIO_PA8A_CTS0)) ;
            PIOA->PIO_ABCDSR[1] &= (~(unsigned long)(PIO_PA7A_RTS0 | PIO_PA8A_CTS0)) ;
        }
        // PIO A: disable register
        PIOA->PIO_PDR = (unsigned long)(PIO_PA5A_RXD0 | PIO_PA6A_TXD0) ;
        if (handshake) {
            // PIO A: disable register
            PIOA->PIO_PDR = (unsigned long)(PIO_PA7A_RTS0 | PIO_PA8A_CTS0) ;
        }
        break ;
        
    case 1:
        UART_BASE = USART1 ;        // base address
        outsema = COM1OSEM ;        // _FR_
        PMC->PMC_PCER0 = (1 << ID_USART1) ;
        // PIO A: Peripheral A select register: 00
        PIOA->PIO_ABCDSR[0] &= (~(unsigned long)(PIO_PA21A_RXD1 | PIO_PA22A_TXD1)) ;
        PIOA->PIO_ABCDSR[1] &= (~(unsigned long)(PIO_PA21A_RXD1 | PIO_PA22A_TXD1)) ;
        if (handshake) {
            // PIO A: Peripheral A select register
            PIOA->PIO_ABCDSR[0] &= (~(unsigned long)(PIO_PA24A_RTS1 | PIO_PA25A_CTS1)) ;
            PIOA->PIO_ABCDSR[1] &= (~(unsigned long)(PIO_PA24A_RTS1 | PIO_PA25A_CTS1)) ;
        }
        // PIO A: disable register
        PIOA->PIO_PDR = (unsigned long)(PIO_PA21A_RXD1 | PIO_PA22A_TXD1) ;
        if (handshake) {
            // PIO A: disable register
            PIOA->PIO_PDR = (unsigned long)(PIO_PA24A_RTS1 | PIO_PA25A_CTS1) ;
        }

        break ;
        
    case 3:     // Handshake not possible
        UART_BASE = (Usart *) UART0 ;   // base address
        outsema = COM3OSEM ;            // _FR_
        PMC->PMC_PCER0 = (1 << ID_UART0) ;
        // PIO A: Peripheral A select register: 00
        PIOA->PIO_ABCDSR[0] &= (~(unsigned long)(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0)) ;
        PIOA->PIO_ABCDSR[1] &= (~(unsigned long)(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0)) ;
        // PIO A: disable register
        PIOA->PIO_PDR = (unsigned long)(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0) ;
        break ;
        
    default :
        return ;  // error
    }
    
    // Disable interrupts
    UART_BASE->US_IDR = (unsigned long) -1;

    // Reset receiver and transmitter
    UART_BASE->US_CR = US_CR_RSTRX | US_CR_RSTTX | US_CR_RXDIS | US_CR_TXDIS ;

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
        UART_BASE->US_PTCR = US_PTCR_RXTEN ;
#endif // RX_BUFFER
    }

    // Define the USART mode
    UART_BASE->US_MR = mode | US_MR_USCLKS_MCK ;

    // Enable Rx Tx
    UART_BASE->US_CR =
#ifdef RX_BUFFER
                       US_CR_STTTO | // ignored by UART0 port
#endif // RX_BUFFER
                       US_CR_RXEN | US_CR_TXEN ;
                        
    // open USART interrupt
    switch(num) {
    case 0 :
        NVIC_EnableIRQ(USART0_IRQn) ;
        NVIC_SetPriority(USART0_IRQn, USART_0_INTERRUPT_LEVEL) ;
        break ;

    case 1 :
        NVIC_EnableIRQ(USART1_IRQn) ;
        NVIC_SetPriority(USART1_IRQn, USART_1_INTERRUPT_LEVEL) ;
        break ;

    case 3 :
        NVIC_EnableIRQ(UART0_IRQn) ;
        NVIC_SetPriority(UART0_IRQn, USART_3_INTERRUPT_LEVEL) ;
        break ;
    }

    // interrupt enable
#ifdef RX_BUFFER
    if (num < 3) {      // only if NOT UART0
        // Enable USART interrupts: errors + RX buffer full + Timeout
        UART_BASE->US_IER = US_IER_FRAME | US_IER_OVRE | US_IER_RXBRK | US_IER_ENDRX | US_IER_TIMEOUT ;
    } else {            // if UART0
        UART_BASE->US_IER = US_IER_FRAME | US_IER_OVRE | US_IER_RXBRK | US_IER_RXRDY ;
    }
#else // RX_BUFFER
    // Enable USART interrupts: errors and RXRDY
    UART_BASE->US_IER = US_IER_FRAME | US_IER_OVRE | US_IER_RXBRK | US_IER_RXRDY ;
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
#endif // TX_BUFFER
    unsigned char flag_full ;   // what to wait
    unsigned char ochar ;       // output char

    // Assign sema at queue: wake up on no empty
    KS_defqsema(COM0OQ, COM0QSEM, QNE) ;                        // <-- COMx
    KS_defqsema(COM1OQ, COM1QSEM, QNE) ;                        // <-- COMx

    KS_defqsema(COM3OQ, COM3QSEM, QNE) ;                        // <-- COMx

    // init semphore constants
    i = 0 ;             // starting point
    semalist[i++] = COM0ISEM ;
    semalist[i++] = COM1ISEM ;

    semalist[i++] = COM3ISEM ;

    semalist[i++] = COM0OSEM ;
    semalist[i++] = COM1OSEM ;

    semalist[i++] = COM3OSEM ;

    // true for everyone at beginning
    flag_full = 0xf ;

    for( ; ; ) {
        i = 6 ;                 // starting point

        if (flag_full & 1) semalist[i++] = COM0QSEM ;
        if (flag_full & 2) semalist[i++] = COM1QSEM ;

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

        // COM 3
        case COM3ISEM :                 // data arrived         // <-- COMx
	    // check queue status
	    if (KS_enqueue(COM3IQ, &ichar3) != RC_GOOD) {       // <-- COMx
	        // one more error
	        com3err++ ;                                     // <-- COMx
	    }
	    break ;

        // COM 0
        case COM0QSEM :	               // data to TX            // <-- COMx
#ifdef TX_BUFFER
            for(i=0 ; i<TX_BUFFER ; i++) {
                if (KS_dequeue(COM0OQ, &txbuff0[i]) != RC_GOOD) {    // <-- COMx
                    break ;
                }
            }
            uartDMAstartTx(USART0, txbuff0, i) ;                // <-- COMx
#else // TX_BUFFER
            KS_dequeue(COM0OQ, &ochar) ;                        // <-- COMx

            DISABLE ;
            // write char to port
            USART0->US_THR = ochar ;                            // <-- COMx
            // Enable the interrupt source for TX
            USART0->US_IER = US_IER_TXRDY /*US_IER_TXEMPTY*/ ;  // <-- COMx
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
            uartDMAstartTx(USART1, txbuff1, i) ;                // <-- COMx
#else // TX_BUFFER
            KS_dequeue(COM1OQ, &ochar) ;                        // <-- COMx

            DISABLE ;
            // write char to port
            USART1->US_THR = ochar ;                            // <-- COMx
            // Enable the interrupt source for TX
            USART1->US_IER = US_IER_TXRDY /*US_IER_TXEMPTY*/ ;  // <-- COMx
            ENABLE ;
#endif // TX_BUFFER
            flag_full &= (~2) ;
            break ;

        case COM1OSEM :                 // data sent            // <-- COMx
            flag_full |= 2 ;
            break ;

        // COM 3
        case COM3QSEM :	               // data to TX            // <-- COMx
            KS_dequeue(COM3OQ, &ochar) ;                        // <-- COMx

            DISABLE ;
            // write char to port
            UART0->UART_THR = ochar ;                           // <-- COMx
            // Enable the interrupt source for TX
            UART0->UART_IER = UART_IER_TXRDY /*UART_IER_TXEMPTY*/ ; // <-- COMx
            ENABLE ;
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
    // disable USART -0- if enabled
    if (PMC->PMC_PCSR0 & (1 << ID_USART0)) {
        // Disable interrupts
        USART0->US_IDR = (unsigned long) -1;
        NVIC_DisableIRQ(USART0_IRQn) ;  // disable interrupt

        // Reset receiver and transmitter
        USART0->US_CR = US_CR_RSTRX | US_CR_RSTTX | US_CR_RXDIS | US_CR_TXDIS ;
        
        // disable clock
        PMC->PMC_PCDR0 = (1 << (ID_USART0)) ;

        // PIO A: set at input with pull-up
        // I/O (not peripheral)
        PIOA->PIO_PER = (unsigned long)(PIO_PA5A_RXD0 | PIO_PA6A_TXD0 | PIO_PA7A_RTS0 | PIO_PA8A_CTS0) ;
        // Input
        PIOA->PIO_ODR = (unsigned long)(PIO_PA5A_RXD0 | PIO_PA6A_TXD0 | PIO_PA7A_RTS0 | PIO_PA8A_CTS0) ;
        // Pull-up
        PIOA->PIO_PUDR = (unsigned long)(PIO_PA7A_RTS0 | PIO_PA8A_CTS0 | PIO_PA7A_RTS0 | PIO_PA8A_CTS0) ;
    }

    // disable USART -1- if enabled
    if (PMC->PMC_PCSR0 & (1 << ID_USART1)) {
        // Disable interrupts
        USART1->US_IDR = (unsigned long) -1;
        NVIC_DisableIRQ(USART1_IRQn) ;  // disable interrupt

        // Reset receiver and transmitter
        USART1->US_CR = US_CR_RSTRX | US_CR_RSTTX | US_CR_RXDIS | US_CR_TXDIS ;

        // disable clock
        PMC->PMC_PCDR0 = (1 << (ID_USART1)) ;

        // PIO A: set at input with pull-down
        // I/O (not peripheral)
        PIOA->PIO_PER = (unsigned long)(PIO_PA21A_RXD1 | PIO_PA22A_TXD1 | PIO_PA24A_RTS1 | PIO_PA25A_CTS1) ;
        // Input
        PIOA->PIO_ODR = (unsigned long)(PIO_PA21A_RXD1 | PIO_PA22A_TXD1 | PIO_PA24A_RTS1 | PIO_PA25A_CTS1) ;
        // No Pull-up
        PIOA->PIO_PUDR = (unsigned long)(PIO_PA21A_RXD1 | PIO_PA22A_TXD1 | PIO_PA24A_RTS1 | PIO_PA25A_CTS1) ;
        // Pull-down
        PIOA->PIO_PPDER = (unsigned long)(PIO_PA21A_RXD1 | PIO_PA22A_TXD1 | PIO_PA24A_RTS1 | PIO_PA25A_CTS1) ;
    }

    // disable USART -3- if enabled
    if (PMC->PMC_PCSR0 & (1 << ID_UART0)) {
        // Disable interrupts
        UART0->UART_IDR = (unsigned long) -1;
        NVIC_DisableIRQ(UART0_IRQn) ;   // disable interrupt

        // Reset receiver and transmitter
        UART0->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS ;

        // disable clock
        PMC->PMC_PCDR0 = (1 << (ID_UART0)) ;

        // PIO A: set at input with pull-down
        // I/O (not peripheral)
        PIOA->PIO_PER = (unsigned long)(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0) ;
        // Input
        PIOA->PIO_ODR = (unsigned long)(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0) ;
        // No Pull-up
        PIOA->PIO_PUDR = (unsigned long)(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0) ;
        // Pull-down
        PIOA->PIO_PPDER = (unsigned long)(PIO_PA9A_URXD0 | PIO_PA10A_UTXD0) ;
    }
}
// end of file - drv_uart.c

