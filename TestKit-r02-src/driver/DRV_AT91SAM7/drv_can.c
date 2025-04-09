// drv_can.c - CAN driver tasks

//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//

//
// This module is provided as a CAN port driver.
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

#if (defined(CBUG) && defined(MTS_CODE))
extern unsigned short par71 ;
#define P71CAN 		0x80
#endif
//----------------------------------------------------------------------------
// only if we are well accepted
#ifdef USE_CAN_ON_ARM

#define NULLSEMA ((SEMA)0)

#define WAIT_USEC       150


unsigned char CanStatus[2] ;	// Diag of CAN
//----------------------------------------------------------------------------
// local data

unsigned short int_mask[2] ;
unsigned short int_pend[2] ;

//----------------------------------------------------------------------------
// functions in drv_clk.c

extern void AT91F_AIC_Configure(int irq_id, int priority, int src_type, FRAME *(*newHandler)(FRAME *frame)) ;

//----------------------------------------------------------------------------
// internal functions

void canstart(int cannum) ;
void canstop(void) ;
AT91PS_CAN canaddress(int cannum) ;

FRAME *can0drv(FRAME * frame) ;
FRAME *can1drv(FRAME * frame) ;

//----------------------------------------------------------------------------
// canaddress: convert num to address

AT91PS_CAN canaddress(int cannum)
{
    switch(cannum) {
    case 0 : return(AT91C_BASE_CAN0) ;
    default :   // theorically an error
    case 1 : return(AT91C_BASE_CAN1) ;
    }
}

//----------------------------------------------------------------------------
// Interrupt routine for CAN 0

FRAME *can0drv(FRAME * frame)
{
    register unsigned long status ;
// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    // current status
    status = AT91C_BASE_CAN0->CAN_SR ;
    
    // any new interrupt pending ?
    status &= AT91C_BASE_CAN0->CAN_IMR ;

    if (status) {       // if something pending
        // Disable the interrupt source
        AT91C_BASE_CAN0->CAN_IDR = status ;
        int_pend[0] |= status ;
        return(KS_ISRexit(frame, CAN0SEM)) ;
    }

    return(KS_ISRexit(frame, NULLSEMA)) ;
}

//----------------------------------------------------------------------------
// Interrupt routine for CAN 1

FRAME *can1drv(FRAME * frame)
{
    register unsigned long status ;
// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    // current status
    status = AT91C_BASE_CAN1->CAN_SR ;

    // any new interrupt pending ?
    status &= AT91C_BASE_CAN1->CAN_IMR ;

    if (status) {       // if something pending
        // Disable the interrupt source
        AT91C_BASE_CAN1->CAN_IDR = status ;
        int_pend[1] |= status ;
        return(KS_ISRexit(frame, CAN1SEM)) ;
    }

    return(KS_ISRexit(frame, NULLSEMA)) ;
}

//----------------------------------------------------------------------------
// CAN initializer

void canstart(int cannum)
{
    AT91PS_CAN pCAN ;

    // force 0 or 1
    cannum &= 1 ;
    pCAN = canaddress(cannum) ; // get address

    // Disable interrupts
    pCAN->CAN_IDR = (unsigned long) -1 ;
    int_mask[cannum] = 0 ;      // clear mask
    int_pend[cannum] = 0 ;      // clear pending

    // Disable peripheral
    pCAN->CAN_MR = 0 ;          // mode register

    switch(cannum) {
    case 0 :
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA26_CANRX0 | AT91C_PA27_CANTX0) ;
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA26_CANRX0 | AT91C_PA27_CANTX0) ;

        // Peripheral Clock Enable Register
        AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_CAN0) ;

        // install interrupt handler
        AT91F_AIC_Configure(AT91C_ID_CAN0, CAN_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, can0drv) ;
        break ;

    case 1 :
        // PIO A: Peripheral A select register
        AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA28_CANRX1 | AT91C_PA29_CANTX1) ;
        // PIO A: disable register
        AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA28_CANRX1 | AT91C_PA29_CANTX1) ;

        // Peripheral Clock Enable Register
        AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_CAN1) ;

        // install interrupt handler
        AT91F_AIC_Configure(AT91C_ID_CAN1, CAN_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, can1drv) ;
        break ;
    }
}

//----------------------------------------------------------------------------
// CAN speed initializer

void CAN_speed(int cannum, unsigned long bitrate, int listenonly)
{
    int i ;
    AT91PS_CAN pCAN ;
    AT91PS_CAN_MB pCAN_MB ;
    register int BRP ;
#ifdef CBUG
	int lquanta ;
#endif
    // force 0 or 1
    cannum &= 1 ;
    pCAN = canaddress(cannum) ;         // get address
    pCAN_MB = &(pCAN->CAN_MB0) ;        // get mailbox
    
    // Disable interrupts
    pCAN->CAN_IDR = (unsigned long) -1 ;
    int_mask[cannum] = 0 ;              // clear mask
    int_pend[cannum] = 0 ;              // clear pending

    // Disable peripheral
    pCAN->CAN_MR = 0 ;                  // mode register
    for(i=0 ; i<CAN_TOT_MAILBOXES ; i++) {
        pCAN_MB[i].CAN_MB_MMR = 0 ;     // disable mailbox
    }

	CanStatus[cannum] = 0 ;

	if (bitrate==0L){
    	CanStatus[cannum] |= CAN_OFF ;
		return ; 			// No baudrate
	}
	BRP = 0 ;
	
    //  |<--------------------- NOMINAL BIT TIME --------------------->|
    //  |                                                              |
    //  +--------------+--------------+---------------+----------------+
    //  |   SYNC_SEG   |   PROP_SEG   |  PHASE_SEG1   |   PHASE_SEG2   |
    //  +--------------+--------------+---------------+----------------+
    //     fixed at 1                                 A
    //                                                |
    //                                                +-- Sample Point
    //
    // Delay of the bus driver: 50 ns
    // Delay of the receiver: 30ns
    // Delay of the bus line (20m): 110ns
    // The total number of time quanta in a bit time must be comprised between 8
    // and 25.

    // Try time quanta to: 16 (_BM_ added 21/1/2009)
    if ( (!(current_clock % 16)) &&
         (!((current_clock / 16) % bitrate)) &&
         ( ((current_clock / 16) / bitrate) < 127 ) &&
         ( ((current_clock / 16) / bitrate) > 1 )
         ) {
        // Bit rate
        BRP = (current_clock / 16 / bitrate) - 1 ;

        // The propagation segment time is equal to twice the sum of the signal's
        // propagation time on the bus line, the receiver delay and the output driver
        // delay:
        // We use worst case: CAN bus @ 1MHz: Tcsc = 1 us / 16 = 62.5 ns.
        // SYNC_SEG fixed at 1 Tcsc by Atmel
        // Tprs = 2 * (50+30+110) ns = 380 ns = 6 Tcsc
        // => PROPAG = Tprs/Tcsc - 1 = 5
        // The remaining time for the two phase segments is:
        // Tphs1 + Tphs2 = bit time - Tcsc - Tprs = (16 - 1 - 6) Tcsc
        // Tphs1 + Tphs2 = 9 Tcsc
        // Because this number is odd, we choose Tphs2 = Tphs1 + Tcsc
        // Tphs1 = 4 Tcsc => PHASE1 = Tphs1/Tcsc - 1 = 3
        // Tphs2 = 5 Tcsc => PHASE2 = Tphs2/Tcsc - 1 = 4
        // The resynchronization jump width must be comprised between 1 Tcsc and the
        // minimum of 4 Tcsc and Tphs1. We choose its maximum value:
        // Tsjw = Min(4 Tcsc,Tphs1) = 4 Tcsc
        // => SJW = Tsjw/Tcsc - 1 = 3
        // Finally:
        // SJW = 3, PROPAG = 5, PHASE1 = 3, PHASE2 = 4
        pCAN->CAN_BR = 0x00003534 | (BRP << 16) ;
#ifdef CBUG
		lquanta = 16 ;
#endif		
    } else

    // Try time quanta to: 12
    if ( ( (!(current_clock % 12)) && (!((current_clock / 12) % bitrate)) ) ||
         ( (bitrate == 83333) && (current_clock >= 1000000)) ) {
        // Bit rate
        BRP = (current_clock / 12 / bitrate) - 1 ;

        // The propagation segment time is equal to twice the sum of the signal's
        // propagation time on the bus line, the receiver delay and the output driver
        // delay:
        // We use worst case: CAN bus @ 1MHz: Tcsc = 1 us / 12 = 83 ns.
        // SYNC_SEG fixed at 1 Tcsc by Atmel
        // Tprs = 2 * (50+30+110) ns = 380 ns = 5 Tcsc
        // => PROPAG = Tprs/Tcsc - 1 = 4
        // The remaining time for the two phase segments is:
        // Tphs1 + Tphs2 = bit time - Tcsc - Tprs = (12 - 1 - 5) Tcsc
        // Tphs1 + Tphs2 = 6 Tcsc
        // Because this number is even, we choose Tphs2 = Tphs1 (else we would choose
        // Tphs2 = Tphs1 + Tcsc)
        // Tphs1 = 3 Tcsc => PHASE1 = Tphs1/Tcsc - 1 = 2
        // Tphs2 = 3 Tcsc => PHASE2 = Tphs2/Tcsc - 1 = 2
        // The resynchronization jump width must be comprised between 1 Tcsc and the
        // minimum of 4 Tcsc and Tphs1. We choose its maximum value:
        // Tsjw = Min(4 Tcsc,Tphs1) = 3 Tcsc
        // => SJW = Tsjw/Tcsc - 1 = 2
        // Finally:
        // SJW = 2, PROPAG = 4, PHASE1 = 2, PHASE2 = 2
        pCAN->CAN_BR = 0x00002422 | (BRP << 16) ;
#ifdef CBUG
		lquanta = 12 ;
#endif 
    } else
    // Try time quanta to: 10
    if ( (!(current_clock % 10)) && (!((current_clock / 10) % bitrate)) ) {
        // Bit rate
        BRP = (current_clock / 10 / bitrate) - 1 ;

        // The propagation segment time is equal to twice the sum of the signal's
        // propagation time on the bus line, the receiver delay and the output driver
        // delay:
        // We use worst case: CAN bus @ 1MHz: Tcsc = 1 us / 10 = 100 ns.
        // SYNC_SEG fixed at 1 Tcsc by Atmel
        // Tprs = 2 * (50+30+110) ns = 380 ns = 4 Tcsc
        // => PROPAG = Tprs/Tcsc - 1 = 3
        // The remaining time for the two phase segments is:
        // Tphs1 + Tphs2 = bit time - Tcsc - Tprs = (10 - 1 - 4)Tcsc
        // Tphs1 + Tphs2 = 5 Tcsc
        // Because this number is odd, we choose Tphs2 = Tphs1 + Tcsc
        // Tphs1 = 2 Tcsc => PHASE1 = Tphs1/Tcsc - 1 = 1
        // Tphs2 = 3 Tcsc => PHASE2 = Tphs2/Tcsc - 1 = 2
        // The resynchronization jump width must be comprised between 1 Tcsc and the
        // minimum of 4 Tcsc and Tphs1. We choose its maximum value:
        // Tsjw = Min(4 Tcsc,Tphs1) = 3 Tcsc
        // => SJW = Tsjw/Tcsc - 1 = 2
        // Finally:
        // SJW = 2, PROPAG = 3, PHASE1 = 1, PHASE2 = 2
        pCAN->CAN_BR = 0x00002312 | (BRP << 16) ;
#ifdef CBUG
        lquanta = 10 ;
#endif
    } else
    // Try time quanta to: 8
    if ( (!(current_clock % 8)) && (!((current_clock / 8) % bitrate)) ) {
        // Bit rate
        BRP = (current_clock / 8 / bitrate) - 1 ;

        // The propagation segment time is equal to twice the sum of the signal's
        // propagation time on the bus line, the receiver delay and the output driver
        // delay:
        // We use worst case: CAN bus @ 1MHz: Tcsc = 1 us / 8 = 125 ns.
        // SYNC_SEG fixed at 1 Tcsc by Atmel
        // Tprs = 2 * (50+30+110) ns = 380 ns = 3 Tcsc
        // => PROPAG = Tprs/Tcsc - 1 = 2
        // The remaining time for the two phase segments is:
        // Tphs1 + Tphs2 = bit time - Tcsc - Tprs = (8 - 1 - 3)Tcsc
        // Tphs1 + Tphs2 = 4 Tcsc
        // Because this number is even, we choose Tphs2 = Tphs1
        // Tphs1 = 2 Tcsc => PHASE1 = Tphs1/Tcsc - 1 = 1
        // Tphs2 = 2 Tcsc => PHASE2 = Tphs2/Tcsc - 1 = 1
        // The resynchronization jump width must be comprised between 1 Tcsc and the
        // minimum of 4 Tcsc and Tphs1. We choose its maximum value:
        // Tsjw = Min(4 Tcsc,Tphs1) = 2 Tcsc
        // => SJW = Tsjw/Tcsc - 1 = 1
        // Finally:
        // SJW = 1, PROPAG = 2, PHASE1 = 1, PHASE2 = 1
        pCAN->CAN_BR = 0x00001211 | (BRP << 16) ;
#ifdef CBUG
        lquanta = 8 ;
#endif
    } else {
    	CanStatus[cannum] |= CAN_BADRATE ;
#ifdef CBUG
		pdebugt(1,"bad11 CAN clock=%ld, tq=99, BRP=%d !!", bitrate, BRP) ;
        //panic(11) ;
#endif // CBUG
        return ;
    }
    
    if (BRP > 127){
    	CanStatus[cannum] |= CAN_OVFRATE ;
#ifdef CBUG
		pdebugt(1,"bad12 CAN clock=%ld, tq=%d, BRP=%d !!", bitrate, lquanta, BRP) ;
    	//panic(12) ;  // unable to use baud rate    
#endif // CBUG
	}
#if (defined(CBUG) && defined(MTS_CODE))
	else{
		if (par71 & P71CAN) pdebugt(1,"CANok clock=%ld, tq=%d, BRP=%d", bitrate, lquanta, BRP) ;
	}
#endif

    // Enable peripheral
    pCAN->CAN_MR = AT91C_CAN_CANEN | ( listenonly ? AT91C_CAN_ABM : 0 ) ;  // mode register
}

//----------------------------------------------------------------------------
// CAN_configure

void CAN_configure(int cannum, int mailbox, unsigned long addr, unsigned long mask, int flags)
{
    AT91PS_CAN pCAN ;
    AT91PS_CAN_MB pCAN_MB ;

    // force 0 or 1
    cannum &= 1 ;
    pCAN = canaddress(cannum) ;         // get address
    pCAN_MB = &(pCAN->CAN_MB0) ;        // get mailbox
    
    // force valid mailbox number: 0-15
    mailbox &= 0xf ;
    pCAN_MB += mailbox ;                // adjust pointer
    
#ifdef USE_CAN_TRANSMIT_ON_ARM
    pCAN_MB->CAN_MB_MMR = ( (flags & CAN_FLAG_TRANSMIT) ? AT91C_CAN_MOT_TX|AT91C_CAN_PRIOR :
                           ((flags & CAN_FLAG_INTERRUPT) ?
                             AT91C_CAN_MOT_RX : AT91C_CAN_MOT_RXOVERWRITE) ) ;

    pCAN_MB->CAN_MB_MAM = (flags & CAN_FLAG_TRANSMIT) ? 0 : mask | ((flags & CAN_FLAG_EXTENDEDADDR) ? 0x20000000 : 0) ;
    pCAN_MB->CAN_MB_MID = addr | ((flags & CAN_FLAG_EXTENDEDADDR) ? 0x20000000 : 0) ;

    if ( (!(flags & CAN_FLAG_TRANSMIT)) && (flags & CAN_FLAG_INTERRUPT) ) {
        pCAN->CAN_IER = (1 << mailbox) ;
        int_mask[cannum] |= (1 << mailbox) ;    // add to local mask
    }
    
    // enable if Rx
    if ( !(flags & CAN_FLAG_TRANSMIT) ) {
        pCAN_MB->CAN_MB_MCR = AT91C_CAN_MTCR ;
    }
    
#else // USE_CAN_TRANSMIT_ON_ARM

    pCAN_MB->CAN_MB_MMR = ( (flags & CAN_FLAG_INTERRUPT) ?
                             AT91C_CAN_MOT_RX : AT91C_CAN_MOT_RXOVERWRITE ) ;

    if (flags & CAN_FLAG_EXTENDEDADDR){	//_FR_ 04/02/09
	    pCAN_MB->CAN_MB_MAM = mask |  0x20000000  ;
	    pCAN_MB->CAN_MB_MID = addr |  0x20000000  ;
	}else{
	    pCAN_MB->CAN_MB_MAM = (mask & 0x7ff) << 18 ;
	    pCAN_MB->CAN_MB_MID = (addr & 0x7ff) << 18 ;
	}

    // interrupt desired ?
    if (flags & CAN_FLAG_INTERRUPT) {
        pCAN->CAN_IER = (1 << mailbox) ;
        int_mask[cannum] |= (1 << mailbox) ;    // add to local mask
    }

    // enable
    pCAN_MB->CAN_MB_MCR = AT91C_CAN_MTCR ;
#endif // USE_CAN_TRANSMIT_ON_ARM
}

//----------------------------------------------------------------------------
// CAN_read

int CAN_read(int cannum, int mailbox, unsigned long *addr, unsigned char *buffer)
{
    AT91PS_CAN pCAN ;
    AT91PS_CAN_MB pCAN_MB ;
    unsigned long msr ;         // status register copy
    int mlen ;
    
    // force 0 or 1
    cannum &= 1 ;
    pCAN = canaddress(cannum) ;         // get address
    pCAN_MB = &(pCAN->CAN_MB0) ;        // get mailbox

    // force valid mailbox number: 0-15
    mailbox &= 0xf ;
    pCAN_MB += mailbox ;                // adjust pointer

    msr = pCAN_MB->CAN_MB_MSR ;         // get our copy

    // check if something present in this mailbox
    if (!(msr & AT91C_CAN_MRDY)) {
        return(-1) ;    // no data
    }
    
    do {
        mlen = (msr >> 16) & 0xf ;      // message len

        // check if extended _FR_ 05/02/09
        if (pCAN_MB->CAN_MB_MID &  0x20000000) // Extended
        	*addr = pCAN_MB->CAN_MB_MID ;
        else
        	*addr = (pCAN_MB->CAN_MB_MID>>18) ;
        
//        // check if standard _FR_ 04/02/09
//        if (pCAN_MB->CAN_MB_MID & 0x1ffc0000) // standard
//        	*addr = (pCAN_MB->CAN_MB_MID>>18) ;
//        else
//        	*addr = pCAN_MB->CAN_MB_MID ;
        
        *((unsigned long long *)(buffer)) = *((unsigned long long *)(&(pCAN_MB->CAN_MB_MDL))) ;
        
        // repeat until no new messages arrived while reading
        // (verify message consistency)
        msr = pCAN_MB->CAN_MB_MSR ;     // get our copy
    } while(msr & AT91C_CAN_MMI) ;
    
    // handled
    pCAN_MB->CAN_MB_MCR = AT91C_CAN_MTCR ;
    
    // if under interrupt, re-enable it
    if (int_mask[cannum] & (1 << mailbox)) {
        pCAN->CAN_IER = (1 << mailbox) ;
        int_pend[cannum] &= (~(1 << mailbox)) ;
    }

    return(mlen) ;                      // return message len
}

#ifdef USE_CAN_TRANSMIT_ON_ARM
//----------------------------------------------------------------------------
// CAN_write

int CAN_write(int cannum, int mailbox, unsigned char *buffer, int msglen)
{
    AT91PS_CAN pCAN ;
    AT91PS_CAN_MB pCAN_MB ;
    unsigned long msr ;         // status register copy

    // force 0 or 1
    cannum &= 1 ;
    pCAN = canaddress(cannum) ;         // get address
    pCAN_MB = &(pCAN->CAN_MB0) ;        // get mailbox

    // force message len
    msglen &= 0xf ;

    // force valid mailbox number: 0-15
    mailbox &= 0xf ;
    pCAN_MB += mailbox ;                // adjust pointer

    msr = pCAN_MB->CAN_MB_MSR ;         // get our copy

    // check if this mailbox is not ready
    if (!(msr & AT91C_CAN_MRDY)) {
        return(-1) ;    // not ready
    }

    //pCAN_MB->CAN_MB_MID = *addr ;       // address
    *((unsigned long long *)(&(pCAN_MB->CAN_MB_MDL))) = *((unsigned long long *)(buffer)) ; // msg

    // send
    pCAN_MB->CAN_MB_MCR = (AT91C_CAN_MTCR | (msglen << 16)) ;

    return(msglen) ;                    // return message len
}
#endif // USE_CAN_TRANSMIT_ON_ARM

//----------------------------------------------------------------------------
// CAN_status

unsigned long CAN_status(int cannum)
{
    AT91PS_CAN pCAN ;

    // force 0 or 1
    cannum &= 1 ;
    pCAN = canaddress(cannum) ;         // get address

    // return bit mask of pending mailboxes
    //return(~(int_mask[cannum] ^ pCAN->CAN_IMR)) ;
    return(int_pend[cannum]) ;
}

//----------------------------------------------------------------------------
// CAN stop

void canstop(void)
{
    // disable interrupts
    AT91C_BASE_CAN0->CAN_IDR = -1 ;
    AT91C_BASE_CAN1->CAN_IDR = -1 ;

    // disable can
    AT91C_BASE_CAN0->CAN_MR = 0 ;
    AT91C_BASE_CAN1->CAN_MR = 0 ;

    // disable clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_CAN0) | (1 << AT91C_ID_CAN1) ;
}
#endif // USE_CAN_ON_ARM
// end of file - drv_can.c

