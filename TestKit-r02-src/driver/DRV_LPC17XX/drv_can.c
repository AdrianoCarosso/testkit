// drv_can.c - CAN driver tasks

//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

//
// This module is provided as a CAN port driver.
//

#include <string.h>
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

#ifdef USE_CAN_ON_ARM

#if (defined(CBUG) && defined(MTS_CODE))
extern unsigned short par71 ;
#define P71CAN 		0x80
#endif

#define NULLSEMA ((SEMA)0)

#define WAIT_USEC       150

#define MAILBOXES   16      // MUST be power of 2
#define TOTCAN      2

#define CANADDRESS(N) ((N) ? CAN2 : CAN1)

//----------------------------------------------------------------------------
// local data

#ifdef USE_CAN_TRANSMIT_ON_ARM
unsigned long can_tx_addr ;
#endif // USE_CAN_TRANSMIT_ON_ARM

unsigned long can_addr[TOTCAN * MAILBOXES] ;
unsigned long can_mask[TOTCAN * MAILBOXES] ;
unsigned char can_flags[TOTCAN * MAILBOXES] ;

unsigned long long can_data[TOTCAN * MAILBOXES] ;
unsigned long can_realaddr[TOTCAN * MAILBOXES] ;
signed char can_len[TOTCAN * MAILBOXES] ;

unsigned short int_pend[TOTCAN] ;

unsigned char CanStatus[TOTCAN] ;   // Diag of CAN

#define USB_DEBUG
#ifdef USB_DEBUG
volatile int candbg_cnt0, candbg_cnt1 ;
#endif // USB_DEBUG

//----------------------------------------------------------------------------
// internal functions

void CAN_ISR_rx(int cannum) ;

void canstart(int cannum) ;
void canstop(void) ;

//----------------------------------------------------------------------------
// CAN_ISR_rx - CAN rx interrupt handler

void CAN_ISR_rx(int cannum)
{
    int i, plen ;
    unsigned long rfs, rid ;
    CAN_TypeDef * pCAN ;

    // force 0 or 1
    cannum &= 1 ;
    pCAN = CANADDRESS(cannum) ;     // get address

    rfs = pCAN->RFS ;       // Frame
    rid = pCAN->RID ;       // ID

    // check extended address
    if (rfs & 0x80000000) { // if 29 bit
        rid &= 0x1fffffff ;
        rid |= 0x20000000 ;
    } else {                // if 11 bit
        rid &= 0x7ff ;
    }

    // scan all mailboxes
    for(i=0 ; i<MAILBOXES ; i++) {
        if ((rid & can_mask[(cannum * MAILBOXES) + i]) == (can_addr[(cannum * MAILBOXES) + i]))
            break ; // found
    }
    // if really found
    if (i < MAILBOXES) {
        can_data[(cannum * MAILBOXES) + i] = (((unsigned long long)(pCAN->RDB)) << 32LL) | (pCAN->RDA) ;
        can_realaddr[(cannum * MAILBOXES) + i] = rid ;
        plen = (rfs >> 16) & 0xf ;
        if (plen > 8) plen = 8 ;
        can_len[(cannum * MAILBOXES) + i] = plen ;

        if (can_flags[(cannum * MAILBOXES) + i] & CAN_FLAG_INTERRUPT) {
            int_pend[cannum] |= (1 << i) ;  // this mailbox has data pending
            KS_ISRsignal(cannum ? CAN1SEM : CAN0SEM) ;
            ASK_CONTEXTSWITCH ;             // set PendSV
        }
    }
	
    pCAN->CMR = (0x01 << 2) ;   // release receive buffer
}

//----------------------------------------------------------------------------
// CAN_Handler generic interrupt handler

void CAN_IRQHandler(void)  
{		
    register unsigned long status ;
    status = CANCR->CANRxSR ;

    if ( status & (1 << 8) ) {
        CAN_ISR_rx(0) ;
#ifdef USB_DEBUG
        candbg_cnt0++ ;
#endif // USB_DEBUG
    }
    if ( status & (1 << 9) ) {
        CAN_ISR_rx(1) ;
#ifdef USB_DEBUG
        candbg_cnt1++ ;
#endif // USB_DEBUG
    }

//    if ( CAN1->GSR & (1 << 6 ) ) {
//        // The error count includes both TX and RX
//        candbg_err0 = CAN1->GSR >> 16 ;
//    }
//    if ( CAN2->GSR & (1 << 6 ) ) {
//        // The error count includes both TX and RX
//        candbg_err1 = CAN2->GSR >> 16 ;
//    }
}

//----------------------------------------------------------------------------
// CAN initializer

void canstart(int cannum)
{
    CAN_TypeDef * pCAN ;

    // force 0 or 1
    cannum &= 1 ;
    pCAN = CANADDRESS(cannum) ;     // get address

    memset(&can_addr[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_addr[0])) ;
    memset(&can_mask[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_mask[0])) ;
    memset(&can_flags[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_flags[0])) ;
    memset(&can_data[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_data[0])) ;
    memset(&can_realaddr[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_realaddr[0])) ;
    memset(&can_len[cannum * MAILBOXES], -1, MAILBOXES * sizeof(can_len[0])) ;

    switch(cannum) {
    case 0 :
        SC->PCONP |= PCONP_PCCAN1 ;         // Enable CAN1 clock 
        PINCON->PINSEL0 &= ~0x0000000F ;    // CAN1 is p0.0 and p0.1
        PINCON->PINSEL0 |= 0x00000005 ;	
        break ;

    case 1 :
        SC->PCONP |= PCONP_PCCAN2 ;         // Enable CAN2 clock 
        PINCON->PINSEL4 &= ~0x0003C000 ;    // CAN2 is p2.7 and p2.8
        PINCON->PINSEL4 |= 0x00014000 ;
        break ;
    }

// already done in RTXCutil.c
//    SC->PCLKSEL0 |=  0x54000000 ;   // CAN1, CAN2, CANAF at 01 -> PCLK == CCLK
//    SC->PCLKSEL0 &= ~0xA8000000 ;

    pCAN->MOD = 1 ;                 // Reset CAN
    pCAN->IER = 0 ;                 // Disable Receive Interrupt
    int_pend[cannum] = 0 ;          // clear pending

    pCAN->GSR = 0 ;                 // Reset error counter when CANxMOD is in reset

    // remember to configure speed before use CAN
    //pCAN->BTR = can_btr ;
    //pCAN->MOD = 0 ;               // CAN in normal operation mode

    if (cannum==0) {
        CANAF->AFMR = 3 ;           // Bypass mode
        NVIC_EnableIRQ(CAN_IRQn) ;  // One interrupt for both
        NVIC_SetPriority(CAN_IRQn, CAN_INTERRUPT_LEVEL) ;
    }
}

//----------------------------------------------------------------------------
// CAN speed initializer

const uint32_t CAN_BIT_TIME[] = {
0,          /*             not used             */
0,          /*             not used             */
0,          /*             not used             */
0,          /*             not used             */
0x0001C000, /* 0+1,  3+1,   1+1,   0+1,  4, 75% */
0,          /*             not used             */
0x0012C000, /* 0+1,  3+1,   2+1,   1+1,  6, 67% */
0,          /*             not used             */
0x0023C000, /* 0+1,  3+1,   3+1,   2+1,  8, 63% */
0,          /*             not used             */
0x0025C000, /* 0+1,  3+1,   5+1,   2+1, 10, 70% */
0,          /*             not used             */
0x0036C000, /* 0+1,  3+1,   6+1,   3+1, 12, 67% */
0,          /*             not used             */
0,          /*             not used             */
0x0048C000, /* 0+1,  3+1,   8+1,   4+1, 15, 67% */
0x0049C000, /* 0+1,  3+1,   9+1,   4+1, 16, 69% */
};

void CAN_speed(int cannum, unsigned long bitrate, int listenonly)
{
    CAN_TypeDef * pCAN ;
	int nominal_time ;
	int result ;

    // force 0 or 1
    cannum &= 1 ;
    pCAN = CANADDRESS(cannum) ;     // get address

    // Disable interrupts
    pCAN->IER = 0 ;                 // Disable Receive Interrupt
    int_pend[cannum] = 0 ;          // clear pending

    // Disable peripheral
    memset(&can_addr[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_addr[0])) ;
    memset(&can_mask[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_mask[0])) ;
    memset(&can_flags[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_flags[0])) ;
    memset(&can_data[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_data[0])) ;
    memset(&can_realaddr[cannum * MAILBOXES], 0, MAILBOXES * sizeof(can_realaddr[0])) ;
    memset(&can_len[cannum * MAILBOXES], -1, MAILBOXES * sizeof(can_len[0])) ;

	if (bitrate == 0L) {
    	CanStatus[cannum] |= CAN_OFF ;
		return ; 			// No baudrate
	}

	CanStatus[cannum] = 0 ;

	// Determine which nominal time to use
    if (bitrate <= 500000) {
        nominal_time = 12 ;
    } else if (((current_clock / 1000000) % 15) == 0)	{
        nominal_time = 15 ;
    } else if (((current_clock / 1000000) % 16) == 0) {
        nominal_time = 16 ;
    } else {
        nominal_time = 10 ;
    }

    // Prepare value appropriate for bit time register
    result = (current_clock / nominal_time) / bitrate - 1 ;
    if (result > 0x3ff) {
    	CanStatus[cannum] |= CAN_OVFRATE ;
#ifdef CBUG
		pdebugt(1,"bad12 CAN clock=%ld, tq=%d, BRP=%d !!", bitrate, nominal_time, result) ;
#endif // CBUG
    }
    result &= 0x000003FF ;
    result |= CAN_BIT_TIME[nominal_time] ;

    // Enter reset mode
    pCAN->MOD = 0x01 ;
    // Set bit timing
    pCAN->BTR = result ;

    // Return to normal operating
    pCAN->MOD = listenonly ? 2 : 0 ;
	// PROVA FR
    //pCAN->MOD = listenonly ? 6 : 0 ;

#if defined(CBUG)
#if defined(MTS_CODE)
    if (par71 & P71CAN) 
#endif // defined(MTS_CODE)
        pdebugt(1, "CAN_%d OK clock=%ld, tq=%d, BRP=0x%x", cannum, bitrate, nominal_time, result) ;
#endif // defined(CBUG)

    pCAN->IER = 0x01 ;              // Enable receive interrupts
    
}

//----------------------------------------------------------------------------
// CAN_configure

void CAN_configure(int cannum, int mailbox, unsigned long addr, unsigned long mask, int flags)
{
    // force 0 or 1
    cannum &= 1 ;

#ifdef USE_CAN_TRANSMIT_ON_ARM
    if (flags & CAN_FLAG_TRANSMIT) {
        can_tx_addr = (flags & CAN_FLAG_EXTENDEDADDR) ? (addr | 0x20000000) : addr ;
        return ;
    }
#endif // USE_CAN_TRANSMIT_ON_ARM

    // force valid mailbox number
    mailbox &= (MAILBOXES - 1) ;
    
    if (flags & CAN_FLAG_EXTENDEDADDR) {
        can_mask[(cannum * MAILBOXES) + mailbox] = (mask & 0x1fffffff) | 0x20000000 ;
        can_addr[(cannum * MAILBOXES) + mailbox] = (addr & mask) | 0x20000000 ;
    } else {
        can_mask[(cannum * MAILBOXES) + mailbox] = (mask & 0x7ff) /* << 18 */ ;
        can_addr[(cannum * MAILBOXES) + mailbox] = ((addr & mask) & 0x7ff) /* << 18 */ ;
    }
    can_flags[(cannum * MAILBOXES) + mailbox] = flags ;
    int_pend[cannum] &= ~(1<<mailbox) ;         // clear pending
    

}

//----------------------------------------------------------------------------
// CAN_read

int CAN_read(int cannum, int mailbox, unsigned long *addr, unsigned char *buffer)
{
    CAN_TypeDef * pCAN ;
    int mlen, idx ;
    
    // force 0 or 1
    cannum &= 1 ;
    pCAN = CANADDRESS(cannum) ;     // get address

    // force valid mailbox number
    mailbox &= (MAILBOXES - 1) ;

    // pointer
    idx = (cannum * MAILBOXES) + mailbox ;

    // check if something present in this mailbox
    mlen = can_len[idx] ;
    if (mlen < 0) {
        return(-1) ;                // no data
    }

    // critical section
    DISABLE ;
    *((unsigned long long *)(buffer)) = can_data[idx] ;
    *addr = can_realaddr[idx] ;
    ENABLE ;
    
    // if under interrupt, re-enable it
    if (can_flags[idx] & CAN_FLAG_INTERRUPT) {
        DISABLE ;
        int_pend[cannum] &= (~(1 << mailbox)) ;
        ENABLE ;
    }

	// FR 08/09/10
	can_len[idx] = -1 ;
    return(mlen) ;                      // return message len
}

#ifdef USE_CAN_TRANSMIT_ON_ARM
//----------------------------------------------------------------------------
// CAN_write

int CAN_write(int cannum, int mailbox, unsigned char *buffer, int msglen)
{
    CAN_TypeDef * pCAN ;
    unsigned long msr ;

    // force 0 or 1
    cannum &= 1 ;
    pCAN = CANADDRESS(cannum) ;     // get address

    msr = pCAN->SR ;                // get our copy of status register

    // check available buffer
    if (msr & (1<<2)) {

        // Transmit Channel 1 is available 
        pCAN->TFI1 &= ~0x400F000 ;  // RTR=0
        pCAN->TFI1 |= (msglen)<<16 ;

        // check extended format
        if(can_tx_addr & 0x20000000) {
            pCAN->TFI1 |= (1<<31) ;     // set bit FF
        } else {
            pCAN->TFI1 &= ~(1<<31) ;
        }

        // Write CAN ID
        pCAN->TID1 = can_tx_addr ;

        // Write first 4 data bytes
        pCAN->TDA1 = *((unsigned long *)(&buffer[0])) ;

        // Write second 4 data bytes
        pCAN->TDB1 = *((unsigned long *)(&buffer[4])) ;

        // Write transmission request channel 1
        pCAN->CMR = 0x21 ;

    } else if (msr & (1<<10)) {

        // Transmit Channel 2 is available 
        pCAN->TFI2 &= ~0x400F000 ;  // RTR=0
        pCAN->TFI2 |= (msglen)<<16 ;

        // check extended format
        if(can_tx_addr & 0x20000000) {
            pCAN->TFI2 |= (1<<31) ;     // set bit FF
        } else {
            pCAN->TFI2 &= ~(1<<31) ;
        }

        // Write CAN ID
        pCAN->TID2 = can_tx_addr ;

        // Write first 4 data bytes
        pCAN->TDA2 = *((unsigned long *)(&buffer[0])) ;

        // Write second 4 data bytes
        pCAN->TDB2 = *((unsigned long *)(&buffer[4])) ;

        // Write transmission request channel 2
        pCAN->CMR = 0x41 ;

    } else if (msr & (1<<18)) {

        // Transmit Channel 3 is available 
        pCAN->TFI3 &= ~0x400F000 ;  // RTR=0
        pCAN->TFI3 |= (msglen)<<16 ;

        // check extended format
        if(can_tx_addr & 0x20000000) {
            pCAN->TFI3 |= (1<<31) ;     // set bit FF
        } else {
            pCAN->TFI3 &= ~(1<<31) ;
        }

        // Write CAN ID
        pCAN->TID3 = can_tx_addr ;

        // Write first 4 data bytes
        pCAN->TDA3 = *((unsigned long *)(&buffer[0])) ;

        // Write second 4 data bytes
        pCAN->TDB3 = *((unsigned long *)(&buffer[4])) ;

        // Write transmission request channel 3
        pCAN->CMR = 0x81 ;
    }
    return(msglen) ;
}
#endif // USE_CAN_TRANSMIT_ON_ARM

//----------------------------------------------------------------------------
// CAN_status

unsigned long CAN_status(int cannum)
{
    // force 0 or 1
    cannum &= 1 ;

    // return bit mask of pending mailboxes
    return(int_pend[cannum]) ;
}

//----------------------------------------------------------------------------
// CAN stop

void canstop(void)
{
    // disable interrupts
    CAN1->MOD = 1 ;                 // Reset CAN
    CAN1->IER = 0 ;                 // Disable Receive Interrupt
    CAN2->MOD = 1 ;                 // Reset CAN
    CAN2->IER = 0 ;                 // Disable Receive Interrupt

    NVIC_DisableIRQ(CAN_IRQn) ;     // disable interrupt

    // disable power
    SC->PCONP &= ~(PCONP_PCCAN1 | PCONP_PCCAN2) ;   // Disable CANx clock 

    // pin in GPIO
    PINCON->PINSEL0 &= ~0x0000000F ;    // CAN1 is p0.0 and p0.1
    PINCON->PINSEL4 &= ~0x0003C000 ;    // CAN2 is p2.7 and p2.8
}
#endif // USE_CAN_ON_ARM
// end of file - drv_can.c

