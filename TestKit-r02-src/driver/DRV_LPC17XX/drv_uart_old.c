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

// Per Brignolo
// Per implementare il RTS/CTS su COM2
// Su M3108 e M3208: CTS=P2.5  e RTS=P3.26, 
// Su M2202:         CTS=P2.5  mentre RTS=P0.9.    DIVERSO RTS rispetto al M3208
// Per Brignolo


#define NULLSEMA ((SEMA)0)

#define TX_BUFFER       32      // size of Tx buffer (MUST be power of 2)
#define RX_BUFFER       32      // size of Rx buffer (MUST be power of 2)

//----------------------------------------------------------------------------
// chip defines

#define IER_RBR		0x01
#define IER_THRE	0x02
#define IER_RLS		0x04

#define IIR_PEND	0x01
#define IIR_RLS		0x03
#define IIR_RDA		0x02
#define IIR_CTI		0x06
#define IIR_THRE	0x01

#define LSR_RDR		0x01
#define LSR_OE		0x02
#define LSR_PE		0x04
#define LSR_FE		0x08
#define LSR_BI		0x10
#define LSR_THRE	0x20
#define LSR_TEMT	0x40
#define LSR_RXFE	0x80

//----------------------------------------------------------------------------
// internal functions

void uartdrv(void) TASK_ATTRIBUTE ;

void uartstart(int num, unsigned long baud_rate, int mode) ;
void uartstop(void) ;

//----------------------------------------------------------------------------

unsigned short UART0Status, UART1Status, UART2Status, UART3Status ;

#define ERRMASK (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI)

unsigned short com0err ;        // error counter
unsigned short com1err ;        // error counter
unsigned short com2err ;        // error counter
unsigned short com3err ;        // error counter

unsigned char rxbuff[4][RX_BUFFER] ;    // real RX buffer
unsigned char rxiptr[4] ;               // interrupt (producer) RX char pointer
unsigned char rxcptr[4] ;               // consumer RX char pointer

unsigned char txbuff[4][TX_BUFFER] ;    // real TX buffer
unsigned char txiptr[4] ;               // interrupt (consumer) TX char pointer
unsigned char txpptr[4] ;               // producer TX char pointer

//----------------------------------------------------------------------------
// Interrupt routine for COM 0

void UART0_IRQHandler(void)
{
    UART_TypeDef * const THISUART = UART0 ;                     // UART -x-
    unsigned short * const ThisUartStatus = &UART0Status ;      // UART -x-
    const int id = 0 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy ;

    IIRValue = THISUART->IIR ;
    IIRValue >>= 1 ;        // skip pending bit in IIR
    IIRValue &= 0x07 ;      // check bit 1~3, interrupt identification
    if ( IIRValue == IIR_RLS ) {    // Receive Line Status
        LSRValue = THISUART->LSR ;
        // Receive Line Status
        if ( LSRValue & ERRMASK ) {
            // There are errors or break interrupt
            // Read LSR will clear the interrupt
            *ThisUartStatus = LSRValue ;
            Dummy = THISUART->RBR ;     // Dummy read on RX to clear
                                        // interrupt, then bail out
            return ;
        }

    } else if ((IIRValue == IIR_RDA) || (IIRValue == IIR_CTI)) { // Receive Data Available / Timeout
        while( (THISUART->LSR) & LSR_RDR ) {    // Receive Data Ready
            rxbuff[id][rxiptr[id]] = THISUART->RBR ;
            rxiptr[id] = (rxiptr[id] + 1 ) & (RX_BUFFER - 1) ;
            if ( rxiptr[id] == rxcptr[id] ) {  // error
                com0err++ ;             // buffer overflow      // UART -x-
            }
        }
        KS_ISRsignal(COM0ISEM) ;                                // UART -x-
        ASK_CONTEXTSWITCH ;                     // set PendSV

    } else if (IIRValue == IIR_THRE) {  // THRE, transmit holding register empty
        // THRE interrupt
        //LSRValue = THISUART->LSR ;    // Check status in the LSR to see if
                                        // valid data in U0THR or not
        while(((THISUART->LSR) & LSR_THRE) && (txiptr[id] != txpptr[id])) {
            THISUART->THR = txbuff[id][txiptr[id]] ;
            txiptr[id] = (txiptr[id] + 1 ) & (TX_BUFFER - 1) ;
        }
        if ( txiptr[id] == txpptr[id] ) {       // end of activity
            THISUART->IER &= (~IER_THRE) ;      // disable Tx interrupt
            KS_ISRsignal(COM0OSEM) ;                            // UART -x-
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
    }
}

//----------------------------------------------------------------------------
// Interrupt routine for COM 1

void UART1_IRQHandler(void)
{
    UART1_TypeDef * const THISUART = UART1 ;                    // UART -x-
    unsigned short * const ThisUartStatus = &UART1Status ;      // UART -x-
    const int id = 1 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy ;

    IIRValue = THISUART->IIR ;
    IIRValue >>= 1 ;        // skip pending bit in IIR
    IIRValue &= 0x07 ;      // check bit 1~3, interrupt identification
    if ( IIRValue == IIR_RLS ) {    // Receive Line Status
        LSRValue = THISUART->LSR ;
        // Receive Line Status
        if ( LSRValue & ERRMASK ) {
            // There are errors or break interrupt
            // Read LSR will clear the interrupt
            *ThisUartStatus = LSRValue ;
            Dummy = THISUART->RBR ;     // Dummy read on RX to clear
                                        // interrupt, then bail out
            return ;
        }

    } else if ((IIRValue == IIR_RDA) || (IIRValue == IIR_CTI)) { // Receive Data Available / Timeout
        while( (THISUART->LSR) & LSR_RDR ) {    // Receive Data Ready
            rxbuff[id][rxiptr[id]] = THISUART->RBR ;
            rxiptr[id] = (rxiptr[id] + 1 ) & (RX_BUFFER - 1) ;
            if ( rxiptr[id] == rxcptr[id] ) {  // error
                com1err++ ;             // buffer overflow      // UART -x-
            }
        }
        KS_ISRsignal(COM1ISEM) ;                                // UART -x-
        ASK_CONTEXTSWITCH ;                     // set PendSV

    } else if (IIRValue == IIR_THRE) {  // THRE, transmit holding register empty
        // THRE interrupt
        //LSRValue = THISUART->LSR ;    // Check status in the LSR to see if
                                        // valid data in U0THR or not
        while(((THISUART->LSR) & LSR_THRE) && (txiptr[id] != txpptr[id])) {
            THISUART->THR = txbuff[id][txiptr[id]] ;
            txiptr[id] = (txiptr[id] + 1 ) & (TX_BUFFER - 1) ;
        }
        if ( txiptr[id] == txpptr[id] ) {       // end of activity
            THISUART->IER &= (~IER_THRE) ;      // disable Tx interrupt
            KS_ISRsignal(COM1OSEM) ;                            // UART -x-
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
    }
}

//----------------------------------------------------------------------------
// Interrupt routine for COM 2

void UART2_IRQHandler(void)
{
    UART_TypeDef * const THISUART = UART2 ;                     // UART -x-
    unsigned short * const ThisUartStatus = &UART2Status ;      // UART -x-
    const int id = 2 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy ;

    IIRValue = THISUART->IIR ;
    IIRValue >>= 1 ;        // skip pending bit in IIR
    IIRValue &= 0x07 ;      // check bit 1~3, interrupt identification
    if ( IIRValue == IIR_RLS ) {    // Receive Line Status
        LSRValue = THISUART->LSR ;
        // Receive Line Status
        if ( LSRValue & ERRMASK ) {
            // There are errors or break interrupt
            // Read LSR will clear the interrupt
            *ThisUartStatus = LSRValue ;
            Dummy = THISUART->RBR ;     // Dummy read on RX to clear
                                        // interrupt, then bail out
            return ;
        }

    } else if ((IIRValue == IIR_RDA) || (IIRValue == IIR_CTI)) { // Receive Data Available / Timeout
        while( (THISUART->LSR) & LSR_RDR ) {    // Receive Data Ready
            rxbuff[id][rxiptr[id]] = THISUART->RBR ;
            rxiptr[id] = (rxiptr[id] + 1 ) & (RX_BUFFER - 1) ;
            if ( rxiptr[id] == rxcptr[id] ) {  // error
                com2err++ ;             // buffer overflow      // UART -x-
            }
        }
        KS_ISRsignal(COM2ISEM) ;                                // UART -x-
        ASK_CONTEXTSWITCH ;                     // set PendSV

    } else if (IIRValue == IIR_THRE) {  // THRE, transmit holding register empty
        // THRE interrupt
        //LSRValue = THISUART->LSR ;    // Check status in the LSR to see if
                                        // valid data in U0THR or not
        while(((THISUART->LSR) & LSR_THRE) && (txiptr[id] != txpptr[id])) {
            THISUART->THR = txbuff[id][txiptr[id]] ;
            txiptr[id] = (txiptr[id] + 1 ) & (TX_BUFFER - 1) ;
        }
        if ( txiptr[id] == txpptr[id] ) {       // end of activity
            THISUART->IER &= (~IER_THRE) ;      // disable Tx interrupt
            KS_ISRsignal(COM2OSEM) ;                            // UART -x-
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
    }
}

//----------------------------------------------------------------------------
// Interrupt routine for COM 3

void UART3_IRQHandler(void)
{
    UART_TypeDef * const THISUART = UART3 ;                     // UART -x-
    unsigned short * const ThisUartStatus = &UART3Status ;      // UART -x-
    const int id = 3 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy ;

    IIRValue = THISUART->IIR ;
    IIRValue >>= 1 ;        // skip pending bit in IIR
    IIRValue &= 0x07 ;      // check bit 1~3, interrupt identification
    if ( IIRValue == IIR_RLS ) {    // Receive Line Status
        LSRValue = THISUART->LSR ;
        // Receive Line Status
        if ( LSRValue & ERRMASK ) {
            // There are errors or break interrupt
            // Read LSR will clear the interrupt
            *ThisUartStatus = LSRValue ;
            Dummy = THISUART->RBR ;     // Dummy read on RX to clear
                                        // interrupt, then bail out
            return ;
        }

    } else if ((IIRValue == IIR_RDA) || (IIRValue == IIR_CTI)) { // Receive Data Available / Timeout
        while( (THISUART->LSR) & LSR_RDR ) {    // Receive Data Ready
            rxbuff[id][rxiptr[id]] = THISUART->RBR ;
            rxiptr[id] = (rxiptr[id] + 1 ) & (RX_BUFFER - 1) ;
            if ( rxiptr[id] == rxcptr[id] ) {  // error
                com3err++ ;             // buffer overflow      // UART -x-
            }
        }
        KS_ISRsignal(COM3ISEM) ;                                // UART -x-
        ASK_CONTEXTSWITCH ;                     // set PendSV

    } else if (IIRValue == IIR_THRE) {  // THRE, transmit holding register empty
        // THRE interrupt
        //LSRValue = THISUART->LSR ;    // Check status in the LSR to see if
                                        // valid data in U0THR or not
        while(((THISUART->LSR) & LSR_THRE) && (txiptr[id] != txpptr[id])) {
            THISUART->THR = txbuff[id][txiptr[id]] ;
            txiptr[id] = (txiptr[id] + 1 ) & (TX_BUFFER - 1) ;
        }
        if ( txiptr[id] == txpptr[id] ) {       // end of activity
            THISUART->IER &= (~IER_THRE) ;      // disable Tx interrupt
            KS_ISRsignal(COM3OSEM) ;                            // UART -x-
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
    }
}

//----------------------------------------------------------------------------
// Usart initializer

void uartstart(int num, unsigned long baud_rate, int mode)
{
    UART_TypeDef * THISUART ;
    unsigned long Fdiv ;
// _FR_
	char c ;
	SEMA outsema ;
// _FR_

    // Select base address
    // Configure PIO controllers to periph mode
    switch(num) {
    case 0:
        THISUART = UART0 ;
        SC->PCONP |= PCONP_PCUART0 ;            // enable it

        PINCON->PINSEL0 &= ~0x000000F0 ;
        PINCON->PINSEL0 |=  0x00000050 ;        // RxD0 is P0.3 and TxD0 is P0.2

        outsema = COM0OSEM ; // _FR_
        break ;

    case 1:
        THISUART = (UART_TypeDef *)(UART1) ;
		if (SC->PCONP & PCONP_PCUART1) while(KS_dequeue(COM1OQ, &c) !=RC_QUEUE_EMPTY) ;
        SC->PCONP |= PCONP_PCUART1 ;            // enable it

        // P0.22=RTS1(01), P2.0=Tx1(10), P2.1=Rx1(10), P2.2=CTS1(10)
        PINCON->PINSEL1 &= ~0x00003000 ;        // bit 13,12 = 01
        PINCON->PINSEL1 |=  0x00001000 ;        // bit 13,12 = 01
        PINCON->PINSEL4 &= ~0x0000003F ;
        PINCON->PINSEL4 |=  0x0000002A ;

        outsema = COM1OSEM ; // _FR_
        break ;

    case 2:
        THISUART = UART2 ;
        SC->PCONP |= PCONP_PCUART2 ;            // enable it

        PINCON->PINSEL0 &= ~0x00F00000 ;
        PINCON->PINSEL0 |=  0x00500000 ;        // RxD2 is P0.10 and TxD2 is P0.11

        outsema = COM2OSEM ; // _FR_
        break ;
        
    case 3:
        THISUART = UART3 ;
        SC->PCONP |= PCONP_PCUART3 ;            // enable it

        PINCON->PINSEL9 |=  0x0F000000 ;        // RxD3 is P4.29 and TxD3 is P4.28

        outsema = COM3OSEM ; // _FR_
        break ;

    default :
        return ;  // error
    }

    // clear buffers
    rxiptr[num] = 0 ;
    rxcptr[num] = 0 ;
    txiptr[num] = 0 ;
    txpptr[num] = 0 ;
    
	
	
// ++++++++++++++TODO++++++++++++++++++++++++++++++++
// use 'mode' parameter
// check 'handshake'
    if ((mode) && (num==1)) {
        UART1->MCR = 0xc0 ;             // enable auto handshake
    }

    // register initialization
    THISUART->LCR = 0x83 ;              // 8 bits, no Parity, 1 Stop bit, DLAB=1
    Fdiv = ( current_clock / 16 ) / baud_rate ;     // baud rate
    THISUART->DLM = Fdiv / 256 ;
    THISUART->DLL = Fdiv % 256 ;
    // fractional
    if ((current_clock == 16000000) && (baud_rate == 115200)) {
        THISUART->FDR = (12<<4) + 1 ;
    } else {
        THISUART->FDR = 0 ;
    }
    THISUART->LCR = 0x03 ;              // DLAB = 0
    THISUART->FCR = 0x07 ;              // Enable and reset TX and RX FIFO.

    switch(num) {
    case 0:
        NVIC_EnableIRQ(UART0_IRQn) ;
        NVIC_SetPriority(UART0_IRQn, USART_0_INTERRUPT_LEVEL) ;
        break ;

    case 1:
        NVIC_EnableIRQ(UART1_IRQn) ;
        NVIC_SetPriority(UART1_IRQn, USART_1_INTERRUPT_LEVEL) ;
        break ;

    case 2:
        NVIC_EnableIRQ(UART2_IRQn) ;
        NVIC_SetPriority(UART2_IRQn, USART_2_INTERRUPT_LEVEL) ;
        break ;

    case 3:
        NVIC_EnableIRQ(UART3_IRQn) ;
        NVIC_SetPriority(UART3_IRQn, USART_3_INTERRUPT_LEVEL) ;
        break ;
    }
    
    THISUART->IER = IER_RBR /*| IER_THRE */ | IER_RLS;  // Enable UART interrupt

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

    unsigned char flag_full ;   // what to wait

    // Assign sema at queue: wake up on no empty
    KS_defqsema(COM0OQ, COM0QSEM, QNE) ;        // <-- COMx
    KS_defqsema(COM1OQ, COM1QSEM, QNE) ;        // <-- COMx
    KS_defqsema(COM2OQ, COM2QSEM, QNE) ;        // <-- COMx
    KS_defqsema(COM3OQ, COM3QSEM, QNE) ;        // <-- COMx

    // init semphore constants
    i = 0 ;             // starting point
    semalist[i++] = COM0ISEM ;
    semalist[i++] = COM1ISEM ;
    semalist[i++] = COM2ISEM ;
    semalist[i++] = COM3ISEM ;

    semalist[i++] = COM0OSEM ;
    semalist[i++] = COM1OSEM ;
    semalist[i++] = COM2OSEM ;
    semalist[i++] = COM3OSEM ;

    // true for everyone at beginning
    flag_full = 0xf ;

    for( ; ; ) {
        i = 8 ;                 // starting point

        if (flag_full & 1) semalist[i++] = COM0QSEM ;
        if (flag_full & 2) semalist[i++] = COM1QSEM ;
        if (flag_full & 4) semalist[i++] = COM2QSEM ;
        if (flag_full & 8) semalist[i++] = COM3QSEM ;
        semalist[i] = 0 ;       // terminator

        cause = KS_waitm(semalist) ;   // don't waste time
        switch(cause) { 	       // I know who waked me up
        // COM 0
        case COM0ISEM :                 // data arrived         // <-- COMx
            {
                const int id = 0 ;                              // <-- COMx
                while(rxiptr[id] != rxcptr[id]) {
                    if (KS_enqueue(COM0IQ, &rxbuff[id][rxcptr[id]]) != RC_GOOD) {     // <-- COMx
	                // one more error
	                com0err++ ;                             // <-- COMx
	            }
                    rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                }
            }
	    break ;

        // COM 1
        case COM1ISEM :                 // data arrived         // <-- COMx
            {
                const int id = 1 ;                              // <-- COMx
                while(rxiptr[id] != rxcptr[id]) {
                    if (KS_enqueue(COM1IQ, &rxbuff[id][rxcptr[id]]) != RC_GOOD) {     // <-- COMx
	                // one more error
	                com1err++ ;                             // <-- COMx
	            }
                    rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                }
            }
	    break ;

        // COM 2
        case COM2ISEM :                 // data arrived         // <-- COMx
            {
                const int id = 2 ;                              // <-- COMx
                while(rxiptr[id] != rxcptr[id]) {
                    if (KS_enqueue(COM2IQ, &rxbuff[id][rxcptr[id]]) != RC_GOOD) {     // <-- COMx
	                // one more error
	                com2err++ ;                             // <-- COMx
	            }
                    rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                }
            }
	    break ;

        // COM 3
        case COM3ISEM :                 // data arrived         // <-- COMx
            {
                const int id = 3 ;                              // <-- COMx
                while(rxiptr[id] != rxcptr[id]) {
                    if (KS_enqueue(COM3IQ, &rxbuff[id][rxcptr[id]]) != RC_GOOD) {     // <-- COMx
	                // one more error
	                com3err++ ;                             // <-- COMx
	            }
                    rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                }
            }
	    break ;

        // COM 0
        case COM0QSEM :	               // data to TX            // <-- COMx
            {
                const int id = 0 ;                              // <-- COMx
                UART_TypeDef * const THISUART = UART0 ;         // <-- COMx
                while(KS_dequeue(COM0OQ, &txbuff[id][txpptr[id]]) == RC_GOOD) { // <-- COMx
                    txpptr[id] = (txpptr[id] + 1 ) & (TX_BUFFER - 1) ;
                    if (txiptr[id] == txpptr[id]) {
                        break ;
                    }
                }
                if (!(THISUART->IER & IER_THRE)) {      // Tx interrupt enabled ?
                    DISABLE ;
                    THISUART->THR = txbuff[id][txiptr[id]] ;
                    txiptr[id] = (txiptr[id] + 1 ) & (TX_BUFFER - 1) ;
                    THISUART->IER |= IER_THRE ;         // Enable Tx interrupt
                    ENABLE ;
                }
            }
            flag_full &= (~1) ;
            break ;

        case COM0OSEM :                 // data sent            // <-- COMx
            flag_full |= 1 ;
            break ;

        // COM 1
        case COM1QSEM :	               // data to TX            // <-- COMx
            {
                const int id = 1 ;                              // <-- COMx
                UART1_TypeDef * const THISUART = UART1 ;        // <-- COMx
                while(KS_dequeue(COM1OQ, &txbuff[id][txpptr[id]]) == RC_GOOD) { // <-- COMx
                    txpptr[id] = (txpptr[id] + 1 ) & (TX_BUFFER - 1) ;
                    if (txiptr[id] == txpptr[id]) {
                        break ;
                    }
                }
                if (!(THISUART->IER & IER_THRE)) {      // Tx interrupt enabled ?
                    DISABLE ;
                    THISUART->THR = txbuff[id][txiptr[id]] ;
                    txiptr[id] = (txiptr[id] + 1 ) & (TX_BUFFER - 1) ;
                    THISUART->IER |= IER_THRE ;         // Enable Tx interrupt
                    ENABLE ;
                }
            }
            flag_full &= (~2) ;
            break ;

        case COM1OSEM :                 // data sent            // <-- COMx
            flag_full |= 2 ;
            break ;

        // COM 2
        case COM2QSEM :	               // data to TX            // <-- COMx
            {
                const int id = 2 ;                              // <-- COMx
                UART_TypeDef * const THISUART = UART2 ;         // <-- COMx
                while(KS_dequeue(COM2OQ, &txbuff[id][txpptr[id]]) == RC_GOOD) { // <-- COMx
                    txpptr[id] = (txpptr[id] + 1 ) & (TX_BUFFER - 1) ;
                    if (txiptr[id] == txpptr[id]) {
                        break ;
                    }
                }
                if (!(THISUART->IER & IER_THRE)) {      // Tx interrupt enabled ?
                    DISABLE ;
                    THISUART->THR = txbuff[id][txiptr[id]] ;
                    txiptr[id] = (txiptr[id] + 1 ) & (TX_BUFFER - 1) ;
                    THISUART->IER |= IER_THRE ;         // Enable Tx interrupt
                    ENABLE ;
                }
            }
            flag_full &= (~4) ;
            break ;

        case COM2OSEM :                 // data sent            // <-- COMx
            flag_full |= 4 ;
            break ;

        // COM 3
        case COM3QSEM :	               // data to TX            // <-- COMx
            {
                const int id = 3 ;                              // <-- COMx
                UART_TypeDef * const THISUART = UART3 ;         // <-- COMx
                while(KS_dequeue(COM3OQ, &txbuff[id][txpptr[id]]) == RC_GOOD) { // <-- COMx
                    txpptr[id] = (txpptr[id] + 1 ) & (TX_BUFFER - 1) ;
                    if (txiptr[id] == txpptr[id]) {
                        break ;
                    }
                }
                if (!(THISUART->IER & IER_THRE)) {      // Tx interrupt enabled ?
                    DISABLE ;
                    THISUART->THR = txbuff[id][txiptr[id]] ;
                    txiptr[id] = (txiptr[id] + 1 ) & (TX_BUFFER - 1) ;
                    THISUART->IER |= IER_THRE ;         // Enable Tx interrupt
                    ENABLE ;
                }
            }
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

void uartNstop(char ncoms)
{
// #ifdef USE_PDEBUG
// 			printf( "\nUart CLOSE 0x%x (0x%lx)\n", ncoms, SC->PCONP) ;
// #endif

//	if ((ncoms & 0x1) && (UART0->IER)){
	if ((ncoms & 0x1) && (SC->PCONP & PCONP_PCUART0)){
		// interrupts
		UART0->IER = 0 ;
		NVIC_DisableIRQ(UART0_IRQn) ;               // disable interrupt
		
		// UART 0
		PINCON->PINSEL0 &= ~0x000000F0 ;            // GPIO of RxD0 - P0.3 and TxD0 - P0.2
		PINCON->PINMODE0 &= ~0x000000F0 ;           // TxD0 NO pull-up/down, RxD0 pull-up
		PINCON->PINMODE0 |=  0x00000080 ;           // TxD0 NO pull-up/down, RxD0 pull-up
		GPIO0->FIODIR |= 0x00000004 ;               // TX output
		GPIO0->FIOSET |= 0x00000004 ;               // TX output at '1'
		
	    SC->PCONP &= ~(PCONP_PCUART0) ;     // disable them
	}

//	if ((ncoms & 0x2) && (UART1->IER)){
	if ((ncoms & 0x2) && (SC->PCONP & PCONP_PCUART1)){
		// interrupts
		UART1->IER = 0 ;
		NVIC_DisableIRQ(UART1_IRQn) ;               // disable interrupt
		
		// UART 1
		PINCON->PINSEL1  &= ~0x00003000 ;           // GPIO RTS
		PINCON->PINMODE1 &= ~0x00003000 ;           // NO pull-up/down
		PINCON->PINMODE1 |=  0x00002000 ;           // NO pull-up/down
		GPIO0->FIODIR |= (1<<22) ;                  // RTS output
		GPIO0->FIOCLR |= (1<<22) ;                  // RTS output at '0'

		// no change RX, CTS, RI if wakeup from RING (MOD-ON is 1)
// #ifdef M3208
// #ifdef USE_PDEBUG
// 			printf("Uart GSM_COM (disabled IRQ)\n") ;
// #endif
// 		if (!(GPIO1->FIOPIN & (1<<20))){
// #ifdef USE_PDEBUG
// 			printf("Uart GSM_COM (GSM is ON)\n") ;
// #endif
// #else
// 		if (GPIO1->FIOPIN & (1<<10)){ 
// #endif
		if (GenPurposeReg & WAKE_GSM){
			// GSM is ON (wakeup also from GSM)
			PINCON->PINSEL4  &= ~0x00000003 ;           // GPIO TX
			PINCON->PINMODE4 &= ~0x00000003 ;           // NO pull-up/down
			PINCON->PINMODE4 |=  0x00000002 ;           // NO pull-up/down
			GPIO2->FIODIR |= 0x00000001 ;               // TX output
			GPIO2->FIOCLR |= 0x00000001 ;               // RX, TX, CTS, RI output at '0'
		}else{	// GSM is OFF
			PINCON->PINSEL4  &= ~0x0000303F ;           // GPIO RX, TX, CTS, RI
			PINCON->PINMODE4 &= ~0x0000303F ;           // NO pull-up/down
			PINCON->PINMODE4 |=  0x0000202A ;           // NO pull-up/down
			GPIO2->FIODIR |= 0x00000047 ;               // RX, TX, CTS, RI output
			GPIO2->FIOCLR |= 0x00000047 ;               // RX, TX, CTS, RI output at '0'
		}

		SC->PCONP &= ~(PCONP_PCUART1) ;     // disable them
	}

//	if ((ncoms & 0x4) && (UART2->IER)){
	if ((ncoms & 0x4) && (SC->PCONP & PCONP_PCUART2)){
		// interrupts
		UART2->IER = 0 ;
		NVIC_DisableIRQ(UART2_IRQn) ;               // disable interrupt
		
		// UART 2
		PINCON->PINSEL0  &= ~0x00F00000 ;           // GPIO
		PINCON->PINMODE0 &= ~0x00F00000 ;           // TxD2 NO pull-up/down, RxD2 pull-up
		PINCON->PINMODE0 |=  0x00800000 ;           // TxD2 NO pull-up/down, RxD2 pull-up
		GPIO0->FIODIR |= 0x00000400 ;               // output
		GPIO0->FIOSET |= 0x00000400 ;               // output at '1'
		
		SC->PCONP &= ~(PCONP_PCUART2) ;     // disable them
	}
	
//	if ((ncoms & 0x8) && (UART3->IER)){
	if ((ncoms & 0x8) && (SC->PCONP & PCONP_PCUART3)){
		// interrupts
		UART3->IER = 0 ;

		NVIC_DisableIRQ(UART3_IRQn) ;               // disable interrupt

		// UART 3
		PINCON->PINSEL9  &= ~0x0F000000 ;           // GPIO
		PINCON->PINMODE9 &= ~0x0F000000 ;           // NO pull-up/down
		PINCON->PINMODE9 |=  0x0A000000 ;           // NO pull-up/down
		GPIO4->FIODIR |= 0x30000000 ;               // output
		GPIO4->FIOCLR |= 0x30000000 ;               // output at '0'

		SC->PCONP &= ~(PCONP_PCUART3) ;     // disable them
	}
}

void uartstop(void)
{
	uartNstop(0xf) ; // COM0,COM1,COM2,COM3
}
// end of file - drv_uart.c

