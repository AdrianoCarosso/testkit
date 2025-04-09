// drv_dio.c - digital I/O driver tasks

//
//   Copyright (c) 1997-2008.
//   T.E.S.T. srl
//

//
// This module is provided as a digital I/O port driver.
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"
#include "cres.h"

#include "extapi.h"

#include "assign.h"

#ifdef MTS_CODE_
#define BUG_ACC_CHIP
#endif

#ifdef BUG_ACC_CHIP
#define ACCMASK_ON  (HW_Bo0_SRV_ON | HW_Bo6_ACC_BIT0 | HW_Bo7_ACC_BIT1)
#define ACCMASK_OFF  (~(HW_Bo0_SRV_ON | HW_Bo6_ACC_BIT0 | HW_Bo7_ACC_BIT1))
#endif

// sanity check
#if defined(USE_MAX7324_TW1) && defined(USE_PCAL9555A_TW1)
#error "Only USE_MAX7324_TW1 or USE_PCAL9555A_TW1"
#endif

#ifdef USE_PCAL9555A_TW1
extern int TWI_txrx(int dev, int sub, unsigned char *buf, int len) ;
#endif


#ifdef USE_MAX7324_TW1  // I/O n. 1
short tw1in_change = 0 ;
#endif

#ifdef USE_MAX7324_TW2  // I/O n. 2
short tw2in_change = 0 ;
#endif

#ifdef CBUG_
#define PORT_TW_DEBUG
#endif

//----------------------------------------------------------------------------
// internal functions

void diostart(void) ;
void diostop(void) ;
unsigned long dio_read(int port) ;
void dio_write(int port, int pmask, int pval) ;
void dio_mask(int port, int pmask) ;

//----------------------------------------------------------------------------
// DIO initializer

void diostart(void)
{
    // ------------------------------------------------------------
    // Enable clock for I/O

    // Peripheral Clock Enable Register
#if defined(USE_AT91SAM7A3)
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOA) | (1 << AT91C_ID_PIOB) ;
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PIOA) ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

    // ------------------------------------------------------------
    // PORT A

    AT91C_BASE_PIOA->PIO_SODR = PIOA_MASK ;     // set at 1
    AT91C_BASE_PIOA->PIO_PER = PIOA_MASK ;      // Set in PIO mode
    AT91C_BASE_PIOA->PIO_OER = PIOA_MASK ;      // Configure in Output
    // disable pull-up for all outputs (no open drain)
    AT91C_BASE_PIOA->PIO_PPUDR = PIOA_PMASK ;
    // disable pull-up for these inputs
    AT91C_BASE_PIOA->PIO_PPUDR = PIOA_INPMASK ;
//    							 AT91C_PIO_PA2  | AT91C_PIO_PA6  | AT91C_PIO_PA7  |
//                                 AT91C_PIO_PA9  |
//                                 // AT91C_PIO_PA15 | // FRFR lasciare pull-up
//                                 // AT91C_PIO_PA17 | // FRFR lasciare pull-up
//                                 AT91C_PIO_PA25 |       // odometer
//                                 AT91C_PIO_PA26 | AT91C_PIO_PA28 | AT91C_PIO_PA30 ;

    // ------------------------------------------------------------
    // PORT B

#if defined(USE_AT91SAM7A3)
#ifdef BUG_ACC_CHIP
    AT91C_BASE_PIOB->PIO_CODR = ( ACCMASK_ON ) ;   // set at 0
    AT91C_BASE_PIOB->PIO_SODR = ( PIOB_MASK & ACCMASK_OFF );     // set at 1
#else
    AT91C_BASE_PIOB->PIO_SODR = (PIOB_MASK & (~AT91C_PIO_PB1));     // set at 1 // NO GSM_SOFT_ON 22/04/2010 _FR_
#endif

    AT91C_BASE_PIOB->PIO_PER = PIOB_MASK ;      // Set in PIO mode
    AT91C_BASE_PIOB->PIO_OER = PIOB_MASK ;      // Configure in Output
    // disable pull-up for this outputs (no open drain)
    AT91C_BASE_PIOB->PIO_PPUDR = PIOB_PMASK ;

    // disable pull-up for these inputs
    AT91C_BASE_PIOB->PIO_PPUDR = PIOB_INPMASK ;
//    							 AT91C_PIO_PB11 | AT91C_PIO_PB14 | AT91C_PIO_PB15 | AT91C_PIO_PB16 |
//                                 AT91C_PIO_PB17 | AT91C_PIO_PB18 | AT91C_PIO_PB19 |
//                                 AT91C_PIO_PB20 | // FRFR Warning: if GYRO mounted set PB20 as AD !!
//                                 AT91C_PIO_PB21 |
//                                 AT91C_PIO_PB22 | AT91C_PIO_PB23 |
//                                 AT91C_PIO_PB25 |
//                                 AT91C_PIO_PB26 |
//                                 AT91C_PIO_PB28 ;

    // ------------------------------------------------------------
    // init Timer 4 for counter from TCLK4 input, used as PPS
    // init Timer 5 for counter from TCLK5 input, used as ODOMETER

    // PIO A: Peripheral B select register
    AT91C_BASE_PIOA->PIO_BSR = (unsigned long)(AT91C_PA24_TCLK4 | AT91C_PA25_TCLK5) ;
    // PIO A: disable register
    AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA24_TCLK4 | AT91C_PA25_TCLK5) ;

    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC4) | (1 << AT91C_ID_TC5) ;

    // Clock source
    AT91C_BASE_TCB1->TCB_BMR = 0 ;      // TCLK4 is XC1 input, TCLK5 is XC2 input

    // Timer 4 mode: WAVE (simply a counter), input is XC1 (a.k.a. TCLK4)
    AT91C_BASE_TCB1->TCB_TC1.TC_CMR = AT91C_TC_CLKS_XC1 |
                                      AT91C_TC_WAVESEL_UP |AT91C_TC_WAVE ;

    // Timer 4 control: enable
    AT91C_BASE_TCB1->TCB_TC1.TC_CCR = AT91C_TC_CLKEN ;

    // Timer 4 control: start
    AT91C_BASE_TCB1->TCB_TC1.TC_CCR = AT91C_TC_SWTRG ;

    // Timer 4 enabled, read current 16 bit value using
    // val = AT91C_BASE_TCB1->TCB_TC1.TC_CV ;

    // Timer 5 mode: WAVE (simply a counter), input is XC2 (a.k.a. TCLK5)
    AT91C_BASE_TCB1->TCB_TC2.TC_CMR = AT91C_TC_CLKS_XC2 |
                                      AT91C_TC_WAVESEL_UP |AT91C_TC_WAVE ;

    // Timer 5 control: enable
    AT91C_BASE_TCB1->TCB_TC2.TC_CCR = AT91C_TC_CLKEN ;

    // Timer 5 control: start
    AT91C_BASE_TCB1->TCB_TC2.TC_CCR = AT91C_TC_SWTRG ;

    // Timer 5 enabled, read current 16 bit value using
    // val = AT91C_BASE_TCB1->TCB_TC2.TC_CV ;
#endif // defined(USE_AT91SAM7A3)
}

//----------------------------------------------------------------------------
// DIO terminator

void diostop(void)
{
    // Disable odometers

#if defined(USE_AT91SAM7A3)
    // Timer 4 control: disable
    AT91C_BASE_TCB1->TCB_TC1.TC_CCR = AT91C_TC_CLKDIS ;
    // Timer 5 control: disable
    AT91C_BASE_TCB1->TCB_TC2.TC_CCR = AT91C_TC_CLKDIS ;

    // Disable odometers clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_TC4) | (1 << AT91C_ID_TC5) ;
#endif // defined(USE_AT91SAM7A3)
}

//----------------------------------------------------------------------------
#ifdef PORT_TW1_CNF
short dioconfTW(void)
{
unsigned char lbuf[5] ;
unsigned short retval ;
// unsigned short * ls ;
// unsigned long * ll ;
// Output port configuration register (4Fh) is good as default

// 	ls = (unsigned short *) &lbuf[1] ;
// 	ll = (unsigned long *) &lbuf[1] ;

	retval = 0 ;
	// Read conf
#ifdef PORT_TW_DEBUG
	printf("dioconfTW start \n") ;
#endif
// // Configure I/O expander (if needed)
	KS_lockw(TWIPORT) ;         // we trust with
	
#ifdef PORT_TW1_CNF
	lbuf[0] = 0x46 ;
	lbuf[1] = PORT_TW1_PUP & 0xff ;
	lbuf[2] = (PORT_TW1_PUP>>8) & 0xff ;
	TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
	lbuf[0] = 0x48 ;
	lbuf[1] = (PORT_TW1_PUP>>16) & 0xff ;
	lbuf[2] = (PORT_TW1_PUP>>24) & 0xff ;
	TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
// #ifdef PORT_TW_DEBUG
// 	printf("dioconfTW %d (0x%02x,0x%02x,0x%02x,0x%02x)\n", i++, lbuf[1], lbuf[2], lbuf[3], lbuf[4]) ;
//     KS_delay(SELFTASK, ((TICKS)200*CLKRATE/1000)) ;     // skip time
// #endif
	
	
	lbuf[0] = 0x0 ; // wait all input
	lbuf[1] = 0x0 ;
	TWI_txrx(PCAL9555A_ADDR, 0x6, lbuf, 2) ; // DIRECTION
#ifdef PORT_TW_DEBUG
	printf("TW1:%02x%02x (%04x)\n", lbuf[1], lbuf[0], PORT_TW1_CNF ) ;
#endif
	if ( ((PORT_TW1_CNF & 0xff) != lbuf[0]) || ( ((PORT_TW1_CNF & 0xff00)>>8) != lbuf[1]) ){
		lbuf[0] = 0x06 ;
		lbuf[1] = PORT_TW1_CNF & 0xff ;
		lbuf[2] = (PORT_TW1_CNF>>8) & 0xff ;
		TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
		retval = 1 ;
	}
	// Disable latch
	lbuf[0] = 0x44 ;
	lbuf[1] = 0x0 ;
	lbuf[2] = 0x0 ;
	TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
#endif

#ifdef PORT_TW2_CNF
	lbuf[0] = 0x46 ;
	lbuf[1] = PORT_TW2_PUP & 0xff ;
	lbuf[2] = (PORT_TW2_PUP>>8) & 0xff ;
	TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
	lbuf[0] = 0x48 ;
	lbuf[1] = (PORT_TW2_PUP>>16) & 0xff ;
	lbuf[2] = (PORT_TW2_PUP>>24) & 0xff ;
	TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
	
	lbuf[0] = 0x0 ;
	lbuf[1] = 0x0 ;
	TWI_txrx(PCAL9555A_ADDR2, 0x6, lbuf, 2) ;
#ifdef PORT_TW_DEBUG
		printf("TW2:%02x%02x (%04x)\n", lbuf[1], lbuf[0], PORT_TW2_CNF) ;
#endif
	if ( ((PORT_TW2_CNF & 0xff) != lbuf[0]) || ( ((PORT_TW2_CNF & 0xff00)>>8) != lbuf[1]) ){
		lbuf[0] = 0x06 ;
		lbuf[1] = PORT_TW2_CNF & 0xff ;
		lbuf[2] = (PORT_TW2_CNF>>8) & 0xff ;
		TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
		retval = 1 ;
	}
	// Disable latch
	lbuf[0] = 0x44 ;
	lbuf[1] = 0x0 ;
	lbuf[2] = 0x0 ;
	TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
#endif

#ifdef PORT_TW3_CNF
	lbuf[0] = 0x46 ;
	lbuf[1] = PORT_TW3_PUP & 0xff ;
	lbuf[2] = (PORT_TW3_PUP>>8) & 0xff ;
	TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
	lbuf[0] = 0x48 ;
	lbuf[1] = (PORT_TW3_PUP>>16) & 0xff ;
	lbuf[2] = (PORT_TW3_PUP>>24) & 0xff ;
	TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
	
	lbuf[0] = 0x0 ;
	lbuf[1] = 0x0 ;
	TWI_txrx(PCAL9555A_ADDR3, 0x6, lbuf, 2) ;
#ifdef PORT_TW_DEBUG
		printf("TW3:%02x%02x (%04x)\n", lbuf[1], lbuf[0], PORT_TW3_CNF) ;
#endif
	if ( ((PORT_TW3_CNF & 0xff) != lbuf[0]) || ( ((PORT_TW3_CNF & 0xff00)>>8) != lbuf[1]) ){
		lbuf[0] = 0x06 ;
		lbuf[1] = PORT_TW3_CNF & 0xff ;
		lbuf[2] = (PORT_TW3_CNF>>8) & 0xff ;
		TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
		retval = 1 ;
	}
	// Disable latch
	
	lbuf[0] = 0x44 ;
	lbuf[1] = 0x0 ;
	lbuf[2] = 0x0 ;
	TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
#endif

	KS_unlock(TWIPORT) ;        // we trust with
	
#ifdef PORT_TW_DEBUG
	printf("dioconfTW end\n") ;
#endif
	return(retval) ;
}
#endif // #ifdef PORT_TW1_CNF

//----------------------------------------------------------------------------
// Read digital input port
// port values:
//      0       internal PORT A, 32 bit
//      1       internal PORT B, 32 bit
//      2       external optional MAX7324 TW1, 8 input data bit + 8 input flag bit + 8 output data bit
//      3       external optional MAX7324 TW2, 8 input data bit + 8 input flag bit + 8 output data bit

unsigned long dio_read(int port)
{
unsigned long retval = 0 ;
#ifdef USE_PCAL9555A_TW1
unsigned char *lcc ;
	
	lcc = (unsigned char *)(&retval) ;
#endif

#if defined(USE_AT91SAM7A3)
    KS_lockw(TWIPORT) ;         // we trust with
#endif // defined(USE_AT91SAM7A3)

    switch(port) {
    case 0:     // PORT A
        retval = AT91C_BASE_PIOA->PIO_PDSR ;
        break ;

#if defined(USE_AT91SAM7A3)
    case 1:     // PORT B
        retval = AT91C_BASE_PIOB->PIO_PDSR ;
        break ;
#endif // defined(USE_AT91SAM7A3)

#ifdef USE_PCAL9555A_TW1
    case PORT_TW1 : // TWI-1
        {
            //KS_lockw(TWIPORT) ;         // we trust with
 			TWI_txrx(PCAL9555A_ADDR,  0x4C, &lcc[2], 2) ;
            TWI_txrx(PCAL9555A_ADDR, 0x100, &lcc[0], 2) ; // ((unsigned char *)(&retval)), 2) ;
            //KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_PCAL9555A_TW1
#ifdef USE_MAX7324_TW1          // I/O n. 1
    case PORT_TW1:     // TWI-1
        {
            TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&retval)), 2) ;
            TWI_receive(MAX7324_W1_ADDR, ((unsigned char *)(&retval))+2, 1) ;
            retval |= (tw1in_change & 0xff00) ;
            tw1in_change = 0 ;
            // PER DEBUG
#ifdef USE_PDEBUG_ // FFFR
    {
        extern char cbugflag ;
		if (retval & 0xff00){
			if (!cbugflag) pdebugt(1,"----FLAG TW1 0x%lx", retval) ;
		}
	}
#endif // #ifdef USE_PDEBUG
            
            // PER DEBUG
        }
        break ;
#endif // USE_MAX7324_TW1

#ifdef USE_PCAL9555A_TW2
    case PORT_TW2 : // TWI-1
        {
            //KS_lockw(TWIPORT) ;         // we trust with
			TWI_txrx(PCAL9555A_ADDR2,  0x4C, &lcc[2], 2) ;
            TWI_txrx(PCAL9555A_ADDR2, 0x100, &lcc[0], 2 ) ; // ((unsigned char *)(&retval)), 2) ;
            //KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_PCAL9555A_TW2
#ifdef USE_MAX7324_TW2          // I/O n. 2
    case PORT_TW2:     // TWI-2
        {
            TWI_receive(MAX7324_R2_ADDR, ((unsigned char *)(&retval)), 2) ;
            TWI_receive(MAX7324_W2_ADDR, ((unsigned char *)(&retval))+2, 1) ;
            retval |= (tw2in_change & 0xff00) ;
            tw2in_change = 0 ;
        }
        break ;
#endif // USE_MAX7324_TW2
#ifdef USE_PCAL9555A_TW2
    case PORT_TW3 : // TWI-1
        {
            //TWI_txrx(PCAL9555A_ADDR3, 0x100, ((unsigned char *)(&retval)), 2) ;
			TWI_txrx(PCAL9555A_ADDR3,  0x4C, &lcc[2], 2) ;
            TWI_txrx(PCAL9555A_ADDR3, 0x100, &lcc[0], 2 ) ; // ((unsigned char *)(&retval)), 2) ;
        }
        break ;
#endif // USE_PCAL9555A_TW3
    }

#if defined(USE_AT91SAM7A3)
    KS_unlock(TWIPORT) ;        // we trust with
#endif // defined(USE_AT91SAM7A3)

    return(retval) ;
}

//----------------------------------------------------------------------------
// Write digital input port
// port values:
//      0       internal PORT A, 32 bit
//      1       internal PORT B, 32 bit
//      2       external optional MAX7324 TW1, 8 output data bit
//      3       external optional MAX7324 TW2, 8 output data bit
// Added ports 10,11,12 for change Pup/Pdn I/O expander
void dio_write(int port, int pmask, int pval)
{
#if defined(USE_AT91SAM7A3)
    KS_lockw(TWIPORT) ;         // we trust with
#endif // defined(USE_AT91SAM7A3)
#ifdef USE_PCAL9555A_TW1
	unsigned char lbuf[5] ;
	int retval ;
	unsigned char *lcc ;
	
	lcc = (unsigned char *)(&retval) ;
#endif

    switch(port) {
    case PORT_PIOA:             // PORT A
        AT91C_BASE_PIOA->PIO_SODR = pmask & pval  ;      // set
        AT91C_BASE_PIOA->PIO_CODR = pmask & (~pval) ;   // clear
        break ;

#if defined(USE_AT91SAM7A3)
    case PORT_PIOB:             // PORT B
        AT91C_BASE_PIOB->PIO_SODR = pmask & pval ;      // set
        AT91C_BASE_PIOB->PIO_CODR = pmask & (~pval) ;   // clear
        break ;
#endif // defined(USE_AT91SAM7A3)

#ifdef USE_PCAL9555A_TW1
	case PORT_TW1 : // TWI-1
		{
// 			unsigned short * ls ;
// 
// 			ls = (unsigned short *) &lbuf[1] ;
// 			//KS_lockw(TWIPORT) ;         // we trust with
			TWI_txrx(PCAL9555A_ADDR, 0x100, ((unsigned char *)(&retval)), 2) ;
			retval &= (~pmask) ;
			retval |= pval ;
			lbuf[0] = 0x02 ;
//			*ls = retval ;
			lbuf[1] = retval & 0xff ;
			lbuf[2] = (retval>>8) & 0xff ;
//			printf("set tw1 to 0x%04x\n", retval ) ;
			TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
			//KS_unlock(TWIPORT) ;        // we trust with
		}
		break ;
#endif // USE_PCAL9555A_TW1
#ifdef PORT_PUPTW1
	case PORT_PUPTW1:
		TWI_txrx(PCAL9555A_ADDR, 0x46, &lcc[0], 2) ; // Read actual status
		TWI_txrx(PCAL9555A_ADDR, 0x48, &lcc[2], 2) ; // Read actual status
// printf("Get Pup1 of R46=%x(%x), R47=%x(%x), R48=%x(%x), R49=%x(%x)\n", (retval & 0xff), lcc[0], 
// 	   		(retval & 0xff00)>>8,  lcc[1], (retval & 0xff0000)>>16,  lcc[2],  (retval & 0xff000000)>>24,  lcc[3]  ) ;
		retval &= (~pmask) ;
		retval |= pval ;
		lbuf[0] = 0x46 ;
		lbuf[1] = lcc[0] & 0xff ;
		lbuf[2] = lcc[1] & 0xff ;
		lbuf[3] = lcc[2] & 0xff ;
		lbuf[4] = lcc[3] & 0xff ;
// printf("Set Pup1 to R46=%x, R47=%x, R48=%x, R49=%x\n", lbuf[1], lbuf[2], lbuf[3], lbuf[4] ) ;
		TWI_send(PCAL9555A_ADDR, lbuf, 3) ; // 5
		lbuf[0] = 0x48 ;
		lbuf[1] = lcc[2] & 0xff ;
		lbuf[2] = lcc[3] & 0xff ;
		TWI_send(PCAL9555A_ADDR, lbuf, 3) ; // 5
		break ;
#endif // PORT_PUPTW1

#ifdef USE_MAX7324_TW1  // I/O n. 1
    case PORT_TW1:              // TWI-1
        {
            unsigned char c ;
            // old value
            TWI_receive(MAX7324_W1_ADDR, &c, 1) ;
            // change value
            c |= (pmask & pval) ;
            c &= ((~pmask) | pval) ;
            // new value
            TWI_send(MAX7324_W1_ADDR, &c, 1) ;
        }
        break ;
#endif // USE_MAX7324_TW1

#ifdef USE_PCAL9555A_TW2
	case PORT_TW2 : // TWI-1
		{
// 			unsigned short * ls ;
// 
// 			ls = (unsigned short *) &lbuf[1] ;
// 			//KS_lockw(TWIPORT) ;         // we trust with
			TWI_txrx(PCAL9555A_ADDR2, 0x100, ((unsigned char *)(&retval)), 2) ;
//			printf("set tw2: 0x%04x 0x%04x 0x%04x\n", retval, (~pmask), pval ) ;
			retval &= (~pmask) ;
			retval |= pval ;
			lbuf[0] = 0x02 ;
//			*ls = retval ;
			lbuf[1] = retval & 0xff ;
			lbuf[2] = (retval>>8) & 0xff ;
//			printf("set tw2 to 0x%04x\n", retval ) ;
			TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
			//KS_unlock(TWIPORT) ;        // we trust with
		}
		break ;
#endif // USE_PCAL9555A_TW2
#ifdef PORT_PUPTW2
	case PORT_PUPTW2:
		TWI_txrx(PCAL9555A_ADDR2, 0x46, &lcc[0], 2) ; // Read actual status
		TWI_txrx(PCAL9555A_ADDR2, 0x48, &lcc[2], 2) ; // Read actual status
// printf("Get Pup2 of R46=%x(%x), R47=%x(%x), R48=%x(%x), R49=%x(%x)\n", (retval & 0xff), lcc[0], 
// 	   		(retval & 0xff00)>>8,  lcc[1], (retval & 0xff0000)>>16,  lcc[2],  (retval & 0xff000000)>>24,  lcc[3]  ) ;
		retval &= (~pmask) ;
		retval |= pval ;
		lbuf[0] = 0x46 ;
		lbuf[1] = lcc[0] & 0xff ;
		lbuf[2] = lcc[1] & 0xff ;
		lbuf[3] = lcc[2] & 0xff ;
		lbuf[4] = lcc[3] & 0xff ;
// printf("Set Pup2 to R46=%x, R47=%x, R48=%x, R49=%x\n", lbuf[1], lbuf[2], lbuf[3], lbuf[4] ) ;
		TWI_send(PCAL9555A_ADDR2, lbuf, 3) ; // 5
		lbuf[0] = 0x48 ;
		lbuf[1] = lcc[2] & 0xff ;
		lbuf[2] = lcc[3] & 0xff ;
		TWI_send(PCAL9555A_ADDR2, lbuf, 3) ; // 5
		break ;
#endif // PORT_PUPTW2

#ifdef USE_MAX7324_TW2  // I/O n. 2
    case PORT_TW2:              // TWI-2
        {
            unsigned char c ;
            // old value
            TWI_receive(MAX7324_W2_ADDR, &c, 1) ;
            // change value
            c |= (pmask & pval) ;
            c &= ((~pmask) | pval) ;
            // new value
            TWI_send(MAX7324_W2_ADDR, &c, 1) ;
        }
        break ;
#endif // USE_MAX7324_TW2

#ifdef USE_PCAL9555A_TW3
		case PORT_TW3 : // TWI-1
			{
// 			unsigned short * ls ;

// 			ls = (unsigned short *) &lbuf[1] ;
// 			//KS_lockw(TWIPORT) ;         // we trust with
			TWI_txrx(PCAL9555A_ADDR3, 0x100, ((unsigned char *)(&retval)), 2) ;
			retval &= (~pmask) ;
			retval |= pval ;
			lbuf[0] = 0x02 ;
			lbuf[1] = retval & 0xff ;
			lbuf[2] = (retval>>8) & 0xff ;
//			*ls = retval ;
//			printf("set tw3 to 0x%04x\n", retval ) ;
			TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
			//KS_unlock(TWIPORT) ;        // we trust with
		}
		break ;
#endif // USE_PCAL9555A_TW3
#ifdef PORT_PUPTW3
	case PORT_PUPTW3:
		TWI_txrx(PCAL9555A_ADDR3, 0x46, &lcc[0], 2) ; // Read actual status
		TWI_txrx(PCAL9555A_ADDR3, 0x48, &lcc[2], 2) ; // Read actual status
// printf("Get Pup1 of R46=%x(%x), R47=%x(%x), R48=%x(%x), R49=%x(%x)\n", (retval & 0xff), lcc[0], 
// 	   		(retval & 0xff00)>>8,  lcc[1], (retval & 0xff0000)>>16,  lcc[2],  (retval & 0xff000000)>>24,  lcc[3]  ) ;
		retval &= (~pmask) ;
		retval |= pval ;
		lbuf[0] = 0x46 ;
		lbuf[1] = lcc[0] & 0xff ;
		lbuf[2] = lcc[1] & 0xff ;
		lbuf[3] = lcc[2] & 0xff ;
		lbuf[4] = lcc[3] & 0xff ;
// printf("Set Pup1 to R46=%x, R47=%x, R48=%x, R49=%x\n", lbuf[1], lbuf[2], lbuf[3], lbuf[4] ) ;
		TWI_send(PCAL9555A_ADDR3, lbuf, 3) ; // 5
		lbuf[0] = 0x48 ;
		lbuf[1] = lcc[2] & 0xff ;
		lbuf[2] = lcc[3] & 0xff ;
		TWI_send(PCAL9555A_ADDR3, lbuf, 3) ; // 5
		break ;
#endif // PORT_PUPTW3
    }

#if defined(USE_AT91SAM7A3)
    KS_unlock(TWIPORT) ;        // we trust with
#endif // defined(USE_AT91SAM7A3)
}

//----------------------------------------------------------------------------
// Write digital input port mask
// port values:
//      0       internal PORT A, 32 bit NOT USED
//      1       internal PORT B, 32 bit NOT USED
//      2       external optional MAX7324 TW1, 8 output data bit
//      3       external optional MAX7324 TW2, 8 output data bit

void dio_mask(int port, int pmask)
{
#if defined(USE_AT91SAM7A3_)
    KS_lockw(TWIPORT) ;         // we trust with
#endif // defined(USE_AT91SAM7A3)

    switch(port) {
    case 0:             // PORT A
        break ;

    case 1:             // PORT B
        break ;

#ifdef USE_MAX7324_TW1  // I/O n. 1
    case 2:             // TWI-1
        {
            unsigned char c = (unsigned char)(pmask) ;
            // before get input modification
            TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&tw1in_change)), 2) ;
            // PER DEBUG
#ifdef USE_PDEBUG // FFFR
			pdebugt(1,"----CHTW1 0x%x 0x%x", (tw1in_change&0xff00)>>8, pmask) ;
#endif // #ifdef USE_PDEBUG

			tw1in_change &= (pmask<<8) ;
            // new value
            TWI_send(MAX7324_R1_ADDR, &c, 1) ;
        }
        break ;
#endif // USE_MAX7324_TW1

#ifdef USE_MAX7324_TW2  // I/O n. 2
    case 3:             // TWI-2
        {
            unsigned char c = (unsigned char)(pmask) ;
            // before get input modification
            TWI_receive(MAX7324_R2_ADDR, ((unsigned char *)(&tw2in_change)), 2) ;
#ifdef USE_PDEBUG // FFFR
			pdebugt(1,"----CHTW2 0x%x 0x%x", (tw2in_change&0xff00)>>8, pmask) ;
#endif // #ifdef USE_PDEBUG
			tw2in_change &= (pmask<<8) ;
            // new value
            TWI_send(MAX7324_R2_ADDR, &c, 1) ;
        }
        break ;
#endif // USE_MAX7324_TW2
    }

#if defined(USE_AT91SAM7A3_)
    KS_unlock(TWIPORT) ;        // we trust with
#endif // defined(USE_AT91SAM7A3)
}

//----------------------------------------------------------------------------
// Read counter value
// port values:
//      0       connected to GPS 1 PPS
//      1       connected to odometer

unsigned short dio_counter(int port)
{
#if defined(USE_AT91SAM7A3)
    if (port) {
        return(AT91C_BASE_TCB1->TCB_TC2.TC_CV) ;        // Odometer
    } else {
        return(AT91C_BASE_TCB1->TCB_TC1.TC_CV) ;        // PPS
    }
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    return(0) ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
}

// end of file - drv_dio.c

