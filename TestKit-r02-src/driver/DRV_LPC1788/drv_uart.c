// drv_uart.c - uart driver tasks

//
//   Copyright (c) 1997-2011.
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

#ifdef USECOM0_RTSCTS
unsigned char COM0_Handshake ;
#endif // USECOM0_RTSCTS

#ifdef USECOM2_RTSCTS
unsigned char COM2_Handshake ;
#endif // USECOM2_RTSCTS

#ifdef USECOM3_RTSCTS
unsigned char COM3_Handshake ;
#endif // USECOM2_RTSCTS

// Per Rovera
// nella routine di tick 10ms "xPortSysTickHandler()" in drv_clk.c aggiungere
// Per Rovera
#include "portat.h"
extern void diosetpin(const uint32_t pinportset) ;


// Per Brignolo
// Per implementare il RTS/CTS su COM2
// Su M3108 e M3208: CTS=P2.5  e RTS=P3.26,
// Su M2202:         CTS=P2.5  mentre RTS=P0.9.    DIVERSO RTS rispetto al M3208
// Per Brignolo


#define NULLSEMA ((SEMA)0)

#define TX_BUFFER       32      // size of Tx buffer (MUST be power of 2)
#define RX_BUFFER       32      // size of Rx buffer (MUST be power of 2)

#ifdef USE_COM4_ON_ARM
#define TOT_UARTS   5
#else // USE_COM4_ON_ARM
#define TOT_UARTS   4
#endif // USE_COM4_ON_ARM

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
#ifdef USE_COM4_ON_ARM
unsigned short UART4Status ;
#endif // USE_COM4_ON_ARM

#define ERRMASK (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI)

unsigned short com0err ;        // error counter
unsigned short com1err ;        // error counter
unsigned short com2err ;        // error counter
unsigned short com3err ;        // error counter
#ifdef USE_COM4_ON_ARM
unsigned short com4err ;        // error counter
#endif // USE_COM4_ON_ARM

unsigned char rxbuff[TOT_UARTS][RX_BUFFER] ;    // real RX buffer
unsigned char rxiptr[TOT_UARTS] ;               // interrupt (producer) RX char pointer
unsigned char rxcptr[TOT_UARTS] ;               // consumer RX char pointer

unsigned char txbuff[TOT_UARTS][TX_BUFFER] ;    // real TX buffer
unsigned char txiptr[TOT_UARTS] ;               // interrupt (consumer) TX char pointer
unsigned char txpptr[TOT_UARTS] ;               // producer TX char pointer

//----------------------------------------------------------------------------
// Interrupt routine for COM 0

void UART0_IRQHandler(void)
{
    LPC_UART_TypeDef * const THISUART = LPC_UART0 ;             // UART -x-
    unsigned short * const ThisUartStatus = &UART0Status ;      // UART -x-
    const int id = 0 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy __attribute__ ((unused)) ;

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
    LPC_UART1_TypeDef * const THISUART = LPC_UART1 ;            // UART -x-
    unsigned short * const ThisUartStatus = &UART1Status ;      // UART -x-
    const int id = 1 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy __attribute__ ((unused)) ;

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
    LPC_UART_TypeDef * const THISUART = LPC_UART2 ;             // UART -x-
    unsigned short * const ThisUartStatus = &UART2Status ;      // UART -x-
    const int id = 2 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy __attribute__ ((unused)) ;

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
    LPC_UART_TypeDef * const THISUART = LPC_UART3 ;             // UART -x-
    unsigned short * const ThisUartStatus = &UART3Status ;      // UART -x-
    const int id = 3 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy __attribute__ ((unused)) ;

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


#ifdef USE_COM4_ON_ARM
//----------------------------------------------------------------------------
// Interrupt routine for COM 3

void UART4_IRQHandler(void)
{
    LPC_UART4_TypeDef * const THISUART = LPC_UART4 ;             // UART -x-
    unsigned short * const ThisUartStatus = &UART4Status ;      // UART -x-
    const int id = 4 ;                                          // UART -x-
    unsigned char IIRValue, LSRValue ;
    volatile unsigned char Dummy __attribute__ ((unused)) ;

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
                com4err++ ;             // buffer overflow      // UART -x-
            }
        }
        KS_ISRsignal(COM4ISEM) ;                                // UART -x-
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
            KS_ISRsignal(COM4OSEM) ;                            // UART -x-
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
    }
}

#endif // USE_COM4_ON_ARM
//----------------------------------------------------------------------------
// Usart initializer

void uartstart(int num, unsigned long baud_rate, int mode)
{
    LPC_UART_TypeDef * THISUART ;
    unsigned long Fdiv ;
	char c ;
	SEMA outsema ;

    THISUART = LPC_UART0 ;
    // Select base address
    // Configure PIO controllers to periph mode
    switch(num) {
    case 0:
        THISUART = LPC_UART0 ;
#ifdef USECOM0_RTSCTS
		if (LPC_SC->PCONP & CLKPWR_PCONP_PCUART0) while(KS_dequeue(COM0OQ, &c) != RC_QUEUE_EMPTY) ;
#endif // #ifdef USECOM0_RTSCTS
        LPC_SC->PCONP |= CLKPWR_PCONP_PCUART0 ;            // enable it

        // pins are already configured by dio.c

#ifdef USECOM0_RTSCTS
		COM0_Handshake = (mode & 0x1) ;
		COM0_UNSETRTS ;
#endif // USECOM0_RTSCTS

        outsema = COM0OSEM ; // _FR_
        break ;

    case 1:
		if (mode & 0x1) diosetpin( SET_PORT_VAL(PORT_PIO2, 7, SET_PIN_OUT_0, SET_PORT_PD | 2) ) ;
        THISUART = (LPC_UART_TypeDef *)(LPC_UART1) ;
		if (LPC_SC->PCONP & CLKPWR_PCONP_PCUART1) while(KS_dequeue(COM1OQ, &c) != RC_QUEUE_EMPTY) ;
        LPC_SC->PCONP |= CLKPWR_PCONP_PCUART1 ;            // enable it

        // pins are already configured by dio.c

        outsema = COM1OSEM ; // _FR_
        break ;

    case 2:
        THISUART = LPC_UART2 ;
#ifdef USECOM2_RTSCTS
		if (LPC_SC->PCONP & CLKPWR_PCONP_PCUART2) while(KS_dequeue(COM2OQ, &c) != RC_QUEUE_EMPTY) ;
#endif // #ifdef USECOM2_RTSCTS
        LPC_SC->PCONP |= CLKPWR_PCONP_PCUART2 ;            // enable it

        // pins are already configured by dio.c

#ifdef USECOM2_RTSCTS
		COM2_Handshake = (mode & 0x1) ;
		COM2_UNSETRTS ;
#endif // #ifdef USECOM2_RTSCTS

        outsema = COM2OSEM ; // _FR_
        break ;

    case 3:
        THISUART = LPC_UART3 ;
#ifdef USECOM0_RTSCTS
		if (LPC_SC->PCONP & CLKPWR_PCONP_PCUART3) while(KS_dequeue(COM3OQ, &c) != RC_QUEUE_EMPTY) ;
#endif // #ifdef USECOM0_RTSCTS
        LPC_SC->PCONP |= CLKPWR_PCONP_PCUART3 ;            // enable it

        // pins are already configured by dio.c

#ifdef USECOM0_RTSCTS
		COM3_Handshake = (mode & 0x1) ;
		COM3_UNSETRTS ;
#endif // USECOM0_RTSCTS

        outsema = COM3OSEM ; // _FR_
        break ;

#ifdef USE_COM4_ON_ARM
    case 4:
//        THISUART = LPC_UART4 ;
        LPC_SC->PCONP |= CLKPWR_PCONP_PCUART4 ;            // enable it

        // pins are already configured by dio.c

        outsema = COM4OSEM ; // _FR_
		break ;
		
#endif // USE_COM4_ON_ARM
    default :
        return ;  // error
    }

    // clear buffers
    rxiptr[num] = 0 ;
    rxcptr[num] = 0 ;
    txiptr[num] = 0 ;
    txpptr[num] = 0 ;

    // check 'handshake' for port '1' only
    if (num==1) {
		if (mode){
			LPC_UART1->MCR = 0xc0 ;             // enable auto handshake
		}else{
			LPC_UART1->MCR = 0x00 ;             // disable auto handshake
			diosetpin( SET_PORT_VAL(PORT_PIO2, 7, SET_PIN_OUT_0, SET_PORT_PN | 0) ) ;
		}
	}
	
	
#ifdef USE_COM4_ON_ARM
	if (num==4){
		// register initialization
		LPC_UART4->LCR = 0x83 ;              // 8 bits, no Parity, 1 Stop bit, DLAB=1
		// test if fractional
		if ((PERIPHERAL_CLOCK == 12000000) && (baud_rate == 115200)) {
			LPC_UART4->DLM = 0 ;
			LPC_UART4->DLL = 4 ;
			LPC_UART4->FDR = (8<<4) + 5 ;
		} else {
			Fdiv = ( PERIPHERAL_CLOCK / (16 * baud_rate) ) ;     // baud rate
			LPC_UART4->DLM = (Fdiv >> 8 ) & 0xff ;
			LPC_UART4->DLL = Fdiv & 0xff ;
			LPC_UART4->FDR = 1<<4 ;
		}
		LPC_UART4->LCR = 0x03 ;              // DLAB = 0
		LPC_UART4->FCR = 0x07 ;              // Enable and reset TX and RX FIFO.
	}else{
#endif
    // register initialization
    if (mode & 0x2)
		THISUART->LCR = 0x9B ;				// 8 bits, EVEN parity, 1 Stop bit, DLAB=1
	else
    	THISUART->LCR = 0x83 ;              // 8 bits, no Parity, 1 Stop bit, DLAB=1
    	
    // test if fractional
    if ((PERIPHERAL_CLOCK == 12000000) && (baud_rate == 115200)) {
        THISUART->DLM = 0 ;
        THISUART->DLL = 4 ;
        THISUART->FDR = (8<<4) + 5 ;
    } else {
        Fdiv = ( PERIPHERAL_CLOCK / (16 * baud_rate) ) ;     // baud rate
        THISUART->DLM = (Fdiv >> 8 ) & 0xff ;
        THISUART->DLL = Fdiv & 0xff ;
        THISUART->FDR = 1<<4 ;
    }
    THISUART->LCR &= (~0x80) ;          // DLAB = 0
    THISUART->FCR = 0x87 ;              // Enable and reset TX and RX FIFO. (old 0x07)

#ifdef USE_COM4_ON_ARM
}
#endif

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

#ifdef USE_COM4_ON_ARM
    case 4:
        NVIC_EnableIRQ(UART4_IRQn) ;
        NVIC_SetPriority(UART4_IRQn, USART_4_INTERRUPT_LEVEL) ;
        break ;
#endif // USE_COM4_ON_ARM
    }

#ifdef USE_COM4_ON_ARM
	if (num==4)
    	LPC_UART4->IER = IER_RBR /*| IER_THRE */ | IER_RLS;  // Enable UART interrupt
	else
#endif
    THISUART->IER = IER_RBR /*| IER_THRE */ | IER_RLS;  // Enable UART interrupt

    KS_ISRsignal(outsema) ;                // <-- COMx

}

//----------------------------------------------------------------------------
// COM driver task RX

void uartdrv(void)
{
    register int i ;
    SEMA cause ;                // wake up reason
    SEMA semalist[(3*TOT_UARTS) + 1] ;

    unsigned char flag_full ;   // what to wait

    // Assign sema at queue: wake up on no empty
    KS_defqsema(COM0OQ, COM0QSEM, QNE) ;        // <-- COMx
    KS_defqsema(COM1OQ, COM1QSEM, QNE) ;        // <-- COMx
    KS_defqsema(COM2OQ, COM2QSEM, QNE) ;        // <-- COMx
    KS_defqsema(COM3OQ, COM3QSEM, QNE) ;        // <-- COMx
#ifdef USE_COM4_ON_ARM
    KS_defqsema(COM4OQ, COM4QSEM, QNE) ;        // <-- COMx
#endif
    // init semphore constants
    i = 0 ;             // starting point
    semalist[i++] = COM0ISEM ;
    semalist[i++] = COM1ISEM ;
    semalist[i++] = COM2ISEM ;
    semalist[i++] = COM3ISEM ;
#ifdef USE_COM4_ON_ARM
    semalist[i++] = COM4ISEM ;
#endif

    semalist[i++] = COM0OSEM ;
    semalist[i++] = COM1OSEM ;
    semalist[i++] = COM2OSEM ;
    semalist[i++] = COM3OSEM ;
#ifdef USE_COM4_ON_ARM
    semalist[i++] = COM4OSEM ;
#endif


    // true for everyone at beginning
#ifdef USE_COM4_ON_ARM
    flag_full = 0x1f ;
#else
    flag_full = 0xf ;
#endif

    for( ; ; ) {
        i = (TOT_UARTS*2) ;                 // starting point

        if (flag_full & 1) semalist[i++] = COM0QSEM ;
        if (flag_full & 2) semalist[i++] = COM1QSEM ;
        if (flag_full & 4) semalist[i++] = COM2QSEM ;
        if (flag_full & 8) semalist[i++] = COM3QSEM ;
#ifdef USE_COM4_ON_ARM
        if (flag_full & 16) semalist[i++] = COM4QSEM ;
#endif
        semalist[i] = 0 ;       // terminator

        cause = KS_waitm(semalist) ;   // don't waste time
        switch(cause) { 	       // I know who waked me up
        // COM 0
        case COM0ISEM :                 // data arrived         // <-- COMx
            {
                const int id = 0 ;                              // <-- COMx
                while(rxiptr[id] != rxcptr[id]) {
#ifdef USECOM0_RTSCTS
					if((qheader[COM0IQ].curndx+30)> qheader[COM0IQ].depth) COM0_SETRTS ;
#endif
                    if (KS_enqueue(COM0IQ, &rxbuff[id][rxcptr[id]]) != RC_GOOD) {     // <-- COMx
#ifdef USECOM0_RTSCTS
						if (COM0_Handshake) {
                            //SignalStopToTX = 1 ;
							COM0_SETRTS ;
							break ;
                        } else
#endif // #ifdef USECOM0_RTSCTS
						{
                            // one more error
                            com0err++ ;                         // <-- COMx
                            rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                        }
                    } else {    // GOOD
#ifdef USECOM0_RTSCTS
                        if (COM0_Handshake) {
                            //SignalStopToTX = 0 ;
							COM0_UNSETRTS ;
                        }
#endif // #ifdef USECOM0_RTSCTS
                        rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                    }
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
#ifdef USECOM2_RTSCTS
					if((qheader[COM2IQ].curndx+30)> qheader[COM2IQ].depth) COM2_SETRTS ;
#endif
                    if (KS_enqueue(COM2IQ, &rxbuff[id][rxcptr[id]]) != RC_GOOD) {     // <-- COMx
#ifdef USECOM2_RTSCTS
						if (COM2_Handshake) {
                            //SignalStopToTX = 1 ;
							COM2_SETRTS ;
							break ;
                        } else
#endif // USECOM2_RTSCTS
						{
                            // one more error
                            com2err++ ;                         // <-- COMx
                            rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                        }
                    } else {    // GOOD
#ifdef USECOM2_RTSCTS
                        if (COM2_Handshake) {
                            //SignalStopToTX = 0 ;
							COM2_UNSETRTS ;
                        }
#endif // USECOM2_RTSCTS
                        rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                    }
                }
            }
	    break ;

        // COM 3
        case COM3ISEM :                 // data arrived         // <-- COMx
            {
                const int id = 3 ;                              // <-- COMx
                while(rxiptr[id] != rxcptr[id]) {
#ifdef USECOM3_RTSCTS
					if((qheader[COM3IQ].curndx+30)> qheader[COM3IQ].depth) COM3_SETRTS ;
#endif
					if (KS_enqueue(COM3IQ, &rxbuff[id][rxcptr[id]]) != RC_GOOD) {     // <-- COMx
#ifdef USECOM3_RTSCTS
						if (COM3_Handshake) {
                            //SignalStopToTX = 1 ;
							COM3_SETRTS ;
							break ;
                        } else
#endif // #ifdef USECOM3_RTSCTS
						{
							// one more error
							com3err++ ;                             // <-- COMx
							rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
						}
                    } else {    // GOOD
#ifdef USECOM3_RTSCTS
                        if (COM3_Handshake) {
                            //SignalStopToTX = 0 ;
							COM3_UNSETRTS ;
                        }
#endif // #ifdef USECOM3_RTSCTS
                    	rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                    }
                }
            }
	    break ;
#ifdef USE_COM4_ON_ARM
        // COM 4
        case COM4ISEM :                 // data arrived         // <-- COMx
            {
                const int id = 4 ;                              // <-- COMx
                while(rxiptr[id] != rxcptr[id]) {
                    if (KS_enqueue(COM4IQ, &rxbuff[id][rxcptr[id]]) != RC_GOOD) {     // <-- COMx
	                // one more error
	                com4err++ ;                             // <-- COMx
	            }
                    rxcptr[id] = (rxcptr[id] + 1 ) & (RX_BUFFER - 1) ;
                }
            }
	    break ;
#endif // USE_COM4_ON_ARM

        // COM 0
        case COM0QSEM :	               // data to TX            // <-- COMx
            {
                const int id = 0 ;                              // <-- COMx
                LPC_UART_TypeDef * const THISUART = LPC_UART0 ; // <-- COMx
#ifdef USECOM0_RTSCTS
				if (COM0_Handshake) {
                    if (COM0_CHECKCTS) {
                        flag_full &= (~1) ;
                        break ;
                    }
                }
#endif // USECOM0_RTSCTS
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
                LPC_UART1_TypeDef * const THISUART = LPC_UART1 ;// <-- COMx
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
                LPC_UART_TypeDef * const THISUART = LPC_UART2 ; // <-- COMx
#ifdef USECOM2_RTSCTS
				if (COM2_Handshake) {
                    if (COM2_CHECKCTS) {
                        flag_full &= (~4) ;
                        break ;
                    }
                }
#endif // USECOM2_RTSCTS
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
                LPC_UART_TypeDef * const THISUART = LPC_UART3 ; // <-- COMx
#ifdef USECOM3_RTSCTS
				if (COM3_Handshake) {
                    if (COM3_CHECKCTS) {
                        flag_full &= (~8) ;
                        break ;
                    }
                }
#endif // USECOM3_RTSCTS
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

#ifdef USE_COM4_ON_ARM
        // COM 4
        case COM4QSEM :	               // data to TX            // <-- COMx
            {
                const int id = 4 ;                              // <-- COMx
                LPC_UART4_TypeDef * const THISUART = LPC_UART4 ; // <-- COMx
                while(KS_dequeue(COM4OQ, &txbuff[id][txpptr[id]]) == RC_GOOD) { // <-- COMx
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
            flag_full &= (~16) ;
            break ;

        case COM4OSEM :                 // data sent            // <-- COMx
            flag_full |= 16 ;
            break ;
#endif // USE_COM4_ON_ARM

        }
    }
}

//----------------------------------------------------------------------------
// Usart terminator

void uartNstop(char commask)
{
// #ifdef USE_PDEBUG
// 			printf( "\nUart CLOSE 0x%x (0x%lx)\n", ncoms, LPC_SC->PCONP) ;
// #endif

	if ((commask & (1<<0)) && (LPC_SC->PCONP & CLKPWR_PCONP_PCUART0)) {
		// interrupts
		LPC_UART0->IER = 0 ;
		NVIC_DisableIRQ(UART0_IRQn) ;               // disable interrupt

        // pins will be un-configured by dio.c

	    LPC_SC->PCONP &= ~(CLKPWR_PCONP_PCUART0) ;  // disable them
	}

    if ((commask & (1<<1)) && (LPC_SC->PCONP & CLKPWR_PCONP_PCUART1)) {
		// interrupts
		LPC_UART1->IER = 0 ;
		NVIC_DisableIRQ(UART1_IRQn) ;               // disable interrupt

        // pins will be un-configured by dio.c

#ifdef MTS
		if (GenPurposeReg & WAKE_GSM){
			// GSM is ON (wakeup also from GSM)

            // TODO it in dio.c

		}else
#endif
		{	// GSM is OFF
		}

		LPC_SC->PCONP &= ~(CLKPWR_PCONP_PCUART1) ;      // disable them
	}

    if ((commask & (1<<2)) && (LPC_SC->PCONP & CLKPWR_PCONP_PCUART2)) {
		// interrupts
		LPC_UART2->IER = 0 ;
		NVIC_DisableIRQ(UART2_IRQn) ;               // disable interrupt

        // pins will be un-configured by dio.c

		LPC_SC->PCONP &= ~(CLKPWR_PCONP_PCUART2) ;  // disable them
	}

    if ((commask & (1<<3)) && (LPC_SC->PCONP & CLKPWR_PCONP_PCUART3)) {
		// interrupts
		LPC_UART3->IER = 0 ;
		NVIC_DisableIRQ(UART3_IRQn) ;               // disable interrupt

        // pins will be un-configured by dio.c

		LPC_SC->PCONP &= ~(CLKPWR_PCONP_PCUART3) ;  // disable them
	}

#ifdef USE_COM4_ON_ARM
    if ((commask & (1<<4)) && (LPC_SC->PCONP & CLKPWR_PCONP_PCUART4)) {
        // interrupts
        LPC_UART4->IER = 0 ;
        NVIC_DisableIRQ(UART4_IRQn) ;               // disable interrupt

        // pins will be un-configured by dio.c

        LPC_SC->PCONP &= ~(CLKPWR_PCONP_PCUART4) ;  // disable them
    }
#endif // USE_COM4_ON_ARM
}

void uartstop(void)
{
	uartNstop(0x1f) ;   // COM0,COM1,COM2,COM3, COM4(if present)
}
// end of file - drv_uart.c
