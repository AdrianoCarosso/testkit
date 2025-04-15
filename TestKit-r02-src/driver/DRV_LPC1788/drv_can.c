// drv_can.c - CAN driver tasks

//
//   Copyright (c) 1997-2011.
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

#define CANADDRESS(N) ((N) ? LPC_CAN2 : LPC_CAN1)

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

//#ifdef CRONO_REMOTE
volatile unsigned short int_pend[TOTCAN] ;
// #else
// unsigned short int_pend[TOTCAN] ;
// #endif

// New for 1 channel with CAN_FLAG_BUFDATA
#define NR_BUFFERS 16 
unsigned long long can_bufdata[TOTCAN * NR_BUFFERS] ;
unsigned long can_bufrealaddr[TOTCAN * NR_BUFFERS] ;
unsigned short can_prx[TOTCAN]; // Producer
unsigned short can_ptx[TOTCAN]; // Consumer
short can_buflen[TOTCAN * NR_BUFFERS] ; // Consumer

unsigned char CanStatus[TOTCAN] ;   // Diag of CAN


#ifdef MTS_CODE
unsigned char CanBufStatus[TOTCAN] ;   // Profondity buffer used
unsigned char CanBufMax[TOTCAN] ;      // Max Profondity buffer used
#else
#define USB_DEBUG
#endif

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
int i, plen, j ;
unsigned long rfs, rid ;
LPC_CAN_TypeDef * pCAN ;

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
		if ((rid & can_mask[(cannum * MAILBOXES) + i]) == (can_addr[(cannum * MAILBOXES) + i])){
				break ; // found
		}
	}
	// if really found
	if (i < MAILBOXES) {
		if (can_flags[(cannum * MAILBOXES) + i] & CAN_FLAG_BUFDATA){
			j = (cannum*NR_BUFFERS) + can_prx[cannum] ;
			can_bufdata[j] = (((unsigned long long)(pCAN->RDB)) << 32LL) | (pCAN->RDA) ;
			can_bufrealaddr[j] = rid ;
			plen = (rfs >> 16) & 0xf ;
			if (plen > 8) plen = 8 ;
			can_buflen[j] = plen ;
			can_prx[cannum]++ ;
			if (can_prx[cannum]==NR_BUFFERS) can_prx[cannum] = 0 ;
			CanBufStatus[cannum]++ ;
			if (CanBufStatus[cannum]>CanBufMax[cannum]) CanBufMax[cannum] = CanBufStatus[cannum] ;
		}else{
			can_data[(cannum * MAILBOXES) + i] = (((unsigned long long)(pCAN->RDB)) << 32LL) | (pCAN->RDA) ;
			can_realaddr[(cannum * MAILBOXES) + i] = rid ;
			plen = (rfs >> 16) & 0xf ;
			if (plen > 8) plen = 8 ;
			can_len[(cannum * MAILBOXES) + i] = plen ;
		}

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
    status = LPC_CANCR->RxSR ;

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
    LPC_CAN_TypeDef * pCAN ;

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
        LPC_SC->PCONP |= CLKPWR_PCONP_PCAN1 ;       // Enable CAN1 clock
        // pins are already configured by dio.c
        break ;

    case 1 :
        LPC_SC->PCONP |= CLKPWR_PCONP_PCAN2 ;       // Enable CAN2 clock
        // pins are already configured by dio.c
        break ;
    }

    pCAN->MOD = 1 ;                 // Reset CAN
    pCAN->IER = 0 ;                 // Disable Receive Interrupt
    int_pend[cannum] = 0 ;          // clear pending

    pCAN->GSR = 0 ;                 // Reset error counter when CANxMOD is in reset

    // remember to configure speed before use CAN
    //pCAN->BTR = can_btr ;
    //pCAN->MOD = 0 ;               // CAN in normal operation mode

    if (cannum==0) {
        LPC_CANAF->AFMR = 3 ;       // Bypass mode
        NVIC_EnableIRQ(CAN_IRQn) ;  // One interrupt for both
        NVIC_SetPriority(CAN_IRQn, CAN_INTERRUPT_LEVEL) ;
    }
    CanBufStatus[cannum] = CanBufMax[cannum] = 0 ;
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
    LPC_CAN_TypeDef * pCAN ;
	int nominal_time ;
	int result ;

#ifdef CBUG_
	pdebugt(P71CAN,"CanSpeed%d %ld (%d)", cannum, bitrate, listenonly ) ;
#endif
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
    } else if (((PERIPHERAL_CLOCK / 1000000) % 15) == 0)	{
        nominal_time = 15 ;
    } else if (((PERIPHERAL_CLOCK / 1000000) % 16) == 0) {
        nominal_time = 16 ;
    } else {
        nominal_time = 10 ;
    }

    // Prepare value appropriate for bit time register
    result = (PERIPHERAL_CLOCK / nominal_time) / bitrate - 1 ;
    if (result > 0x3ff) {
    	CanStatus[cannum] |= CAN_OVFRATE ;
#ifdef CBUG
		pdebugt(1,"bad12 CAN clock=%ld, tq=%d, BRP=%d !!", bitrate, nominal_time, result) ;
// #else
// 		printf("\nbad12 CAN clock=%ld, tq=%d, BRP=%d !!\n", bitrate, nominal_time, result) ;
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
        pdebugt(1, "CAN_%d OK clock=%ld, tq=%d, BRP=0x%x %d", cannum, bitrate, nominal_time, result, listenonly) ;
// #else
//         printf("\nCAN_%d OK clock=%ld, tq=%d, BRP=0x%x %d\n", cannum, bitrate, nominal_time, result, listenonly) ;
#endif // defined(CBUG)

    pCAN->IER = 0x01 ;              // Enable receive interrupts

}

//----------------------------------------------------------------------------
// CAN_configure

void CAN_configure(int cannum, int mailbox, unsigned long addr, unsigned long mask, int flags)
{
short j ;
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

	// Initialize CAN_FLAG_BUFDATA
	if (flags & CAN_FLAG_BUFDATA){
		can_prx[cannum] = can_ptx[cannum] = 0 ;
		for(j=0;j<NR_BUFFERS;j++) can_buflen[((cannum*NR_BUFFERS) +j)] = -1 ;
	}
#if defined(CBUG)
#if defined(MTS_CODE)
    if (cannum)
#endif // defined(MTS_CODE)
	pdebugt(1,"Can%dConf-%d addr:0x%08lx mask:0x%08lx flag:%x", cannum, mailbox, addr, mask, flags ) ;
#endif // defined(CBUG)

}

//----------------------------------------------------------------------------
// CAN_read

int CAN_read(int cannum, int mailbox, unsigned long *addr, unsigned char *buffer)
{
//    LPC_CAN_TypeDef * pCAN ;
    int mlen, idx, j ;

    // force 0 or 1
    cannum &= 1 ;
//    pCAN = CANADDRESS(cannum) ;     // get address

    // force valid mailbox number
    mailbox &= (MAILBOXES - 1) ;

    // pointer
    idx = (cannum * MAILBOXES) + mailbox ;

    // check if something present in this mailbox
    if (can_flags[idx] & CAN_FLAG_BUFDATA) {
		mlen = (cannum*NR_BUFFERS) + can_ptx[cannum] ;
		mlen = can_buflen[mlen] ;
	}else{
    	mlen = can_len[idx] ;
	}
    if (mlen < 0) {
        return(-1) ;                // no data
    }

    // critical section
    DISABLE ;
    if (can_flags[idx] & CAN_FLAG_BUFDATA) {
		j =  (cannum*NR_BUFFERS) + can_ptx[cannum] ;
		*((unsigned long long *)(buffer)) = can_bufdata[j] ;
		*addr = can_bufrealaddr[j] ;
		can_ptx[cannum]++ ;
		if (can_ptx[cannum]==NR_BUFFERS) can_ptx[cannum] = 0 ;
		can_buflen[j] = -1 ;
		CanBufStatus[cannum]-- ;
	}else{
		*((unsigned long long *)(buffer)) = can_data[idx] ;
		*addr = can_realaddr[idx] ;
	}
    ENABLE ;

    // if under interrupt, re-enable it
    if (can_flags[idx] & CAN_FLAG_INTERRUPT) {
        DISABLE ;
		if (can_flags[idx] & CAN_FLAG_BUFDATA) {
			if (can_ptx[cannum] == can_prx[cannum]){
				int_pend[cannum] &= (~(1 << mailbox)) ;
			}else{
				KS_ISRsignal(cannum ? CAN1SEM : CAN0SEM) ;
			}
		}else{
	        int_pend[cannum] &= (~(1 << mailbox)) ;
		}
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
    LPC_CAN_TypeDef * pCAN ;
    unsigned long msr ;

    // force 0 or 1
    cannum &= 1 ;
    pCAN = CANADDRESS(cannum) ;     // get address

#if defined(CBUG)
#if defined(MTS_CODE)
    if (par71 & 0x1) // P71CAN)
#endif // defined(MTS_CODE)
	pdebugt(1,"CanSend-%d addr:0x%08lx len:%d {%02x%02x%02x%02x%02x%02x%02x%02x}", cannum, can_tx_addr, msglen, 
					buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7] ) ;
#endif // defined(CBUG)

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
    LPC_CAN1->MOD = 1 ;             // Reset CAN
    LPC_CAN1->IER = 0 ;             // Disable Receive Interrupt
    LPC_CAN2->MOD = 1 ;             // Reset CAN
    LPC_CAN2->IER = 0 ;             // Disable Receive Interrupt

    NVIC_DisableIRQ(CAN_IRQn) ;     // disable interrupt

    // disable power
    LPC_SC->PCONP &= ~(CLKPWR_PCONP_PCAN1 | CLKPWR_PCONP_PCAN2) ;   // Disable CANx clock

    // pins will be un-configured by dio.c
}
#endif // USE_CAN_ON_ARM
// end of file - drv_can.c

