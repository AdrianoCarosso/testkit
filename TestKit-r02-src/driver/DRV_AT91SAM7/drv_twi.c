// drv_twi.c - TWI driver tasks

//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//

//
// This module is provided as a TWI port driver.
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

//----------------------------------------------------------------------------
// only if we are well accepted
#ifdef USE_TWI_ON_ARM

#define NULLSEMA ((SEMA)0)

#define WAIT_USEC       150

unsigned short twi0err = 0 ;
//----------------------------------------------------------------------------
// local data

unsigned char * twibufptr ;     // TWI buffer pointer
int twibuflen ;                 // TWI buffer len

//----------------------------------------------------------------------------
// functions in drv_clk.c

extern void AT91F_AIC_Configure(int irq_id, int priority, int src_type, FRAME *(*newHandler)(FRAME *frame)) ;

//----------------------------------------------------------------------------
// internal functions

void twistart(void) ;
void twistop(void) ;
int TWI_send(int dev, unsigned char *buf, int len) ;
int TWI_receive(int dev, unsigned char *buf, int len) ;

FRAME *twidrv(FRAME * frame) ;

//----------------------------------------------------------------------------
// Interrupt routine for TWI

FRAME *twidrv(FRAME * frame)
{
    unsigned long status ;
// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    status = AT91C_BASE_TWI->TWI_SR     // get actual status
           & AT91C_BASE_TWI->TWI_IMR ;  // actual mask
           
    // check for something to tx
    if (status & AT91C_TWI_TXRDY) {     // Tx ready
        if (twibuflen == 1) {           // time to finish ?
            // Stop
            AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP ;
            AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP ;
        }

        // next byte
        if (twibuflen) {                // something to send ?
            AT91C_BASE_TWI->TWI_THR = *twibufptr++ ;
            twibuflen-- ;
        } else {                        // disable this interrupt
            AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_TXRDY ;
        }
    } else
    // check for something to rx
    if (status & AT91C_TWI_RXRDY) {     // Rx ready
        if (twibuflen) {                // something to send ?
            // next byte
            *twibufptr++ = AT91C_BASE_TWI->TWI_RHR ;
            twibuflen-- ;
        } else {                        // disable this interrupt
            AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_RXRDY ;
        }

        if (twibuflen == 1) {     // time to finish ?
            // Stop
            AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP ;
            AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP ;
        }
    }

    // check for end of activity
    if (status & AT91C_TWI_TXCOMP) {    // end of transmission
        // Disable the interrupt source
        AT91C_BASE_TWI->TWI_IDR = AT91C_TWI_TXCOMP | AT91C_TWI_TXRDY | AT91C_TWI_RXRDY ;

        return(KS_ISRexit(frame, TWI0SEM)) ;
    }

    return(KS_ISRexit(frame, NULLSEMA)) ;
}

//----------------------------------------------------------------------------
// TWI initializer

void twistart(void)
{
#if defined(USE_AT91SAM7A3)
    // PIO A: Pull-up enable register
    AT91C_BASE_PIOA->PIO_PPUER = (unsigned long)(AT91C_PA0_TWD | AT91C_PA1_TWCK) ;
    // PIO A: multi-driver enable register (open drain)
    AT91C_BASE_PIOA->PIO_MDER = (unsigned long)(AT91C_PA0_TWD | AT91C_PA1_TWCK) ;
    // PIO A: Peripheral A select register
    AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA0_TWD | AT91C_PA1_TWCK) ;
    // PIO A: disable register
    AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA0_TWD | AT91C_PA1_TWCK) ;
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    // PIO A: Pull-up enable register
    AT91C_BASE_PIOA->PIO_PPUER = (unsigned long)(AT91C_PA3_TWD | AT91C_PA4_TWCK) ;
    // PIO A: multi-driver enable register (open drain)
    AT91C_BASE_PIOA->PIO_MDER = (unsigned long)(AT91C_PA3_TWD | AT91C_PA4_TWCK) ;
    // PIO A: Peripheral A select register
    AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA3_TWD | AT91C_PA4_TWCK) ;
    // PIO A: disable register
    AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA3_TWD | AT91C_PA4_TWCK) ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TWI) ;

    // Disable interrupts
    AT91C_BASE_TWI->TWI_IDR = (unsigned long) -1 ;

    // Reset peripheral
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_SWRST ;

    // Set Master mode
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN ;

    // set 60KHz clock (16.6 us period = 8.3 + 8.3)
    //  NOUSB: 60MHz -> TMCK=17ns
    //  USB:   48MHz -> TMCK=21ns
    // Tlow = (CLDIV * 2^CKDIV + 3)  TMCK
    // Thi  = (CHDIV * 2^CKDIV + 3)  TMCK

    // Using CKDIV=2
#define TWI_CKDIV       2

    // Tlow = Thi = (CLDIV * 4 + 3)  TMCK = 8.5 us
    //  NOUSB: 60MHz -> CLDIV=CHDIV=126 -> Tlow=Thi=8.4us --> 59.5 KHz
    //  USB:   48MHz -> CLDIV=CHDIV=101 -> Tlow=Thi=8.5us --> 58.5 KHz
    //         16MHz -> CLDIV=CHDIV=33 -> Tlow=Thi=8.5us --> 58.5 KHz
#define TWI_CxDIV       ((((current_clock/1000000)*85) - 30)/40)

    // Using CKDIV=2, CLDIV=CHDIV=100 --> Tlow=Thi=8.4us --> 59.5 KHz
    AT91C_BASE_TWI->TWI_CWGR = (TWI_CKDIV << 16)  |
                               (TWI_CxDIV << 8)   |
                               (TWI_CxDIV) ;	  // Clock Waveform Generator Register

    AT91F_AIC_Configure(AT91C_ID_TWI, TWI_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, twidrv) ;
}

//----------------------------------------------------------------------------
// TWI TX with interrupt

int TWI_send(int dev, unsigned char *buf, int len)
{
    // Set Master mode register
    AT91C_BASE_TWI->TWI_MMR = (dev << 16) ;

    // differentiate activity
    if (len == 1) {

        // Set Master mode
        AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START | AT91C_TWI_MSEN | AT91C_TWI_STOP ;
        AT91C_BASE_TWI->TWI_THR = *buf ;

        // enable interrupts
        AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP ;
        
    } else {            // generic length

        // Set Master mode
        AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN | AT91C_TWI_START ;

        twibufptr = buf ;       // TWI buffer pointer
        twibuflen = len ;       // TWI buffer len

        // enable interrupts
        AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXRDY ;
    }

    // wait end
    if (KS_waitt(TWI0SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
        twi0err++ ;
#ifdef CBUG
        pdebugt(1,"TWIsend %d  error %d, len=%d/%d", dev, twi0err, twibuflen, len) ;
#endif // CBUG
        twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;       // waste usec

    // Disable
    //AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSDIS ;

    return(OK) ;
}

//----------------------------------------------------------------------------
// TWI RX with interrupt

int TWI_receive(int dev, unsigned char *buf, int len)
{
    // Set Master mode register
    AT91C_BASE_TWI->TWI_MMR = (dev << 16) | AT91C_TWI_MREAD ;

    // differentiate activity
    if (len == 1) {

        // Set Master mode
        AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN | AT91C_TWI_START | AT91C_TWI_STOP ;

        twibufptr = buf ;       // TWI buffer pointer
        twibuflen = len ;       // TWI buffer len

        // enable interrupts
        AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP | AT91C_TWI_RXRDY ;

    } else {            // generic length

        // Set Master mode
        AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN | AT91C_TWI_START ;

        twibufptr = buf ;       // TWI buffer pointer
        twibuflen = len ;       // TWI buffer len

        // enable interrupts
        AT91C_BASE_TWI->TWI_IER = AT91C_TWI_RXRDY ;
    }

    // wait end
    if (KS_waitt(TWI0SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
		twi0err++ ;
#ifdef CBUG
        pdebugt(1,"TWIreceive %d  error %d, len=%d/%d", dev, twi0err, twibuflen, len) ;
#endif // CBUG
		twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;     // waste usec

    // Disable
    //AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSDIS ;

    return(OK) ;
}

//----------------------------------------------------------------------------
// TWI Tx-Rx with interrupt

int TWI_txrx(int dev, int sub, unsigned char *buf, int len)
{
    //twiaskrestart = 0 ;             // clear restart flag
    //twisuba = sub ;                 // must be 0 in order to bypass
    //if (sub) {
    //    twiaddr = dev & (~RD_BIT) ; // SLA+W
    //} else {
    //    twiaddr = dev | (RD_BIT) ;  // SLA+R
    //}
	
    // Set Master mode register
    if (sub) {
        AT91C_BASE_TWI->TWI_MMR = (dev << 16) | AT91C_TWI_MREAD | (1<<8) ;
        AT91C_BASE_TWI->TWI_IADR = sub ;
    } else {
        AT91C_BASE_TWI->TWI_MMR = (dev << 16) | AT91C_TWI_MREAD ;
    }
    
    // differentiate activity
    if (len == 1) {

        // Set Master mode
        AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN | AT91C_TWI_START | AT91C_TWI_STOP ;

        twibufptr = buf ;       // TWI buffer pointer
        twibuflen = len ;       // TWI buffer len

        // enable interrupts
        AT91C_BASE_TWI->TWI_IER = AT91C_TWI_TXCOMP | AT91C_TWI_RXRDY ;

    } else {            // generic length

        // Set Master mode
        AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN | AT91C_TWI_START ;

        twibufptr = buf ;       // TWI buffer pointer
        twibuflen = len ;       // TWI buffer len

        // enable interrupts
        AT91C_BASE_TWI->TWI_IER = AT91C_TWI_RXRDY ;
    }

    // wait end
    if (KS_waitt(TWI0SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
        twi0err++ ;
#ifdef CBUG
        pdebugt(1,"TWItxrx %d,%d  error %d, len=%d/%d", dev, sub, twi0err, twibuflen, len) ;
#endif // CBUG
        twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;     // waste usec

    // Disable
    //AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSDIS ;

    return(OK) ;
}
//----------------------------------------------------------------------------
// TWI terminator

void twistop(void)
{
    // Disable interrupts
    AT91C_BASE_TWI->TWI_IDR = (unsigned long) -1 ;

    // Reset peripheral
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_SWRST ;

    // Peripheral Clock Disable Register
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_TWI) ;
}
#endif // USE_TWI_ON_ARM
// end of file - drv_twi.c

