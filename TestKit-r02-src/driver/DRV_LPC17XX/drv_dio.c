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

// sanity check
#if defined(USE_FREQTIMER1) && defined(USE_CNTTIMER1)
#error "Cannot use USE_FREQTIMER1 and USE_CNTTIMER1 together"
#endif // defined(USE_FREQTIMER1) && defined(USE_CNTTIMER1)

#ifdef USE_MAX7324_TW1  // I/O n. 1
short tw1in_change = 0 ;
#endif

#ifdef USE_MAX7324_TW2  // I/O n. 2
short tw2in_change = 0 ;
#endif

#ifdef USE_TWI_SRV
short tw1in1_change = 0 ;
#endif

// USE_MAX7324_TW1			I/O expander n.1 (on TW0)
// USE_MAX7324_TW2			I/O expander n.2 (on TW0)

// USE_TWI1_AUXILIARY		Acc, other service 	 (on TW1)
// USE_TWI_SRV				I/O on Service Board (on TW1)
//----------------------------------------------------------------------------
// global variebles: MOVED
volatile int moved_flag ;
volatile int acc_flag ;
static unsigned long loc_int ;

//----------------------------------------------------------------------------
// internal functions

void diostart(void) ;
void diostop(void) ;
unsigned short dio_counter(int port) ;
unsigned long dio_read(int port) ;
void dio_write(int port, int pmask, int pval) ;
void dio_beeper(int freq) ;

//----------------------------------------------------------------------------
// Interrupt routine for GPIO - same as Ext Int 3

void EINT3_IRQHandler(void)
{

    // useful after a wakeup
    if (CHKWUPREG==0xffffffL){
        RTC->GPREG2 = (GPIOINT->IO0IntStatF  | GPIOINT->IO0IntStatR ) ;
        RTC->GPREG3 = (GPIOINT->IO2IntStatF  | GPIOINT->IO2IntStatR ) ;
        SETWUPREG(1L) ;

	}else {
		loc_int = (GPIOINT->IO0IntStatF  | GPIOINT->IO0IntStatR ) ;
		if (loc_int & (1<<21)) moved_flag++ ;
		if (loc_int & ((1<<9)|(1<<8))) acc_flag++ ;
	}

	// Clear Interrupts
    GPIOINT->IO0IntClr = -1 ;
    GPIOINT->IO2IntClr = -1 ;

}

//----------------------------------------------------------------------------
// DIO initializer

void diostart(void)
{
    // Port -0-
#ifdef PIO0_OUT_AT0
    GPIO0->FIOCLR = PIO0_OUT_AT0 ;
	GPIO0->FIOSET = (PIO0_MASK_OUT ^ PIO0_OUT_AT0) ;
#else
    GPIO0->FIOSET  = PIO0_MASK_OUT ;
#endif // #ifdef PIO0_OUT_AT0
    GPIO0->FIODIR |= PIO0_MASK_OUT ;

    // Port -1-
#ifdef PIO1_OUT_AT0
    GPIO1->FIOCLR  = PIO1_OUT_AT0 ;
	GPIO1->FIOSET = (PIO1_MASK_OUT ^ PIO1_OUT_AT0) ;
#else
    GPIO1->FIOSET  = PIO1_MASK_OUT ;
#endif // #ifdef PIO1_OUT_AT0
    GPIO1->FIODIR |= PIO1_MASK_OUT ;

    // Port -2-
#ifdef PIO2_OUT_AT0
    GPIO2->FIOCLR  = PIO2_OUT_AT0 ;
	GPIO2->FIOSET = (PIO2_MASK_OUT ^ PIO2_OUT_AT0) ;
#else
    GPIO2->FIOSET  = PIO2_MASK_OUT ;
#endif // #ifdef PIO0_OUT_AT0
    GPIO2->FIODIR |= PIO2_MASK_OUT ;

    // Port -3-
#ifdef PIO3_OUT_AT0
    GPIO3->FIOCLR  = PIO3_OUT_AT0 ;
	GPIO3->FIOSET = (PIO3_MASK_OUT ^ PIO3_OUT_AT0) ;
#else
    GPIO3->FIOSET  = PIO3_MASK_OUT ;
#endif // #ifdef PIO0_OUT_AT0
    GPIO3->FIODIR |= PIO3_MASK_OUT ;

    // Port -4-
#ifdef PIO4_OUT_AT0
    GPIO4->FIOCLR  = PIO4_OUT_AT0 ;
	GPIO4->FIOSET = (PIO4_MASK_OUT ^ PIO4_OUT_AT0) ;
#else
    GPIO4->FIOSET  = PIO4_MASK_OUT ;
#endif // #ifdef PIO0_OUT_AT0
    GPIO4->FIODIR |= PIO4_MASK_OUT ;

    // pin mode
    PINCON->PINMODE0 = PIN0_PMASK ;
    PINCON->PINMODE1 = PIN1_PMASK ;
    PINCON->PINMODE2 = PIN2_PMASK ;
    PINCON->PINMODE3 = PIN3_PMASK ;
    PINCON->PINMODE4 = PIN4_PMASK ;
    PINCON->PINMODE5 = PIN5_PMASK ;
    PINCON->PINMODE6 = PIN6_PMASK ;
    PINCON->PINMODE7 = PIN7_PMASK ;
    PINCON->PINMODE8 = PIN8_PMASK ;
    PINCON->PINMODE9 = PIN9_PMASK ;

    // enable interrupts
    NVIC_EnableIRQ(EINT3_IRQn) ;
    NVIC_SetPriority(EINT3_IRQn, GPIO_INTERRUPT_LEVEL) ;

    // enable interrupts (also for wakeup)
    //GPIOINT->IO0IntEnF = (1<<21) ;      // enable interrupt from Falling P0.21 MOVED

    // ------------------------
    // enable timers
    SC->PCONP |= (
#if defined(USE_CNTTIMER1) || defined(USE_FREQTIMER1)
                    PCONP_PCTIM1
#else // defined(USE_CNTTIMER1) || defined(USE_FREQTIMER1)
                    0
#endif // defined(USE_CNTTIMER1) || defined(USE_FREQTIMER1)

#if defined(USE_CNTTIMER2)
                  | PCONP_PCTIM2
#endif // defined(USE_CNTTIMER2)

#if defined(USE_CNTTIMER0)
                  | PCONP_PCTIM0
#endif // defined(USE_CNTTIMER0)
                 ) ;

#if defined(USE_CNTTIMER2)
    // P0.4 = Timer2 CH0 - odometer
    PINCON->PINSEL0 |= 0x00000300 ;
    TIM2->TCR = 2 ;     // reset
    TIM2->CTCR = 1 ;    // count on rising edge of cap0
    TIM2->TCR = 1 ;     // enable
#endif // defined(USE_CNTTIMER2)

#if defined(USE_CNTTIMER1)
    // P1.18 = Timer1 CH0 - PPS for 3108 CortexM3
    PINCON->PINSEL3 |= 0x00000030 ;
    TIM1->TCR = 2 ;     // reset
    TIM1->CTCR = 1 ;    // count on rising edge of cap0
    TIM1->TCR = 1 ;     // enable
//     // P1.19 = Timer1 CH1 - PPS for 2102 CortexM3
//    PINCON->PINSEL3 |= 0x000000c0 ;
//    TIM1->TCR = 2 ;     // reset
//    TIM1->CTCR = 5 ;    // count on rising edge of cap1
//    TIM1->TCR = 1 ;     // enable
#endif // defined(USE_CNTTIMER1)

#if defined(USE_CNTTIMER0)
    // P1.27 = Timer0 CH1 - auxiliary counter
    PINCON->PINSEL3 |= 0x00c00000 ;
    TIM0->TCR = 2 ;     // reset
    TIM0->CTCR = 5 ;    // count on rising edge of cap1
    TIM0->TCR = 1 ;     // enable
#endif // defined(USE_CNTTIMER0)

#if defined(USE_FREQTIMER1)
        // P1.22 = Timer1 CH0 - waveform generator
        PINCON->PINSEL3 &= ~0x00003000 ;            // digital
        GPIO1->FIODIR |= (1<<22) ;                  // output
        GPIO1->FIOCLR  = (1<<22) ;                  // output at '0'
#endif // defined(USE_FREQTIMER1)
}

//----------------------------------------------------------------------------
// DIO terminator

void diostop(void)
{
    // ------------------------
    // disable timers
#if defined(USE_CNTTIMER0)
    TIM0->TCR = 0 ;     // disable
#endif // defined(USE_CNTTIMER0)
#if defined(USE_CNTTIMER1) || defined(USE_FREQTIMER1)
    TIM1->TCR = 0 ;     // disable
#endif // defined(USE_CNTTIMER1) || defined(USE_FREQTIMER1)
#if defined(USE_CNTTIMER2)
    TIM2->TCR = 0 ;     // disable
#endif // defined(USE_CNTTIMER2)

    SC->PCONP &= ~(
#if defined(USE_CNTTIMER1) || defined(USE_FREQTIMER1)
                     PCONP_PCTIM1
#else // defined(USE_CNTTIMER1) || defined(USE_FREQTIMER1)
                     0
#endif // defined(USE_CNTTIMER1)

#if defined(USE_CNTTIMER2)
                   | PCONP_PCTIM2
#endif // defined(USE_CNTTIMER2)

#if defined(USE_CNTTIMER0)
                   | PCONP_PCTIM0
#endif // defined(USE_CNTTIMER0)
                  ) ;

#if defined(USE_CNTTIMER2)
    PINCON->PINSEL0 &= ~0x00000300 ;
#endif // defined(USE_CNTTIMER2)

#if defined(USE_CNTTIMER1)
    PINCON->PINSEL3 &= ~0x00000030 ;
//    PINCON->PINSEL3 &= ~0x000000c0 ;
#endif // defined(USE_CNTTIMER1)

#if defined(USE_CNTTIMER0)
    PINCON->PINSEL3 &= ~0x00c00000 ;
#endif // defined(USE_CNTTIMER0)

#if defined(USE_FREQTIMER1)
    // P1.22 = Timer1 CH0 - waveform generator
    PINCON->PINSEL3 &= ~0x00003000 ;
#endif // defined(USE_FREQTIMER1)

// not defined
#ifdef SET_ALL_AS_INPUT
    // Port -0-
    GPIO0->FIODIR &= (~(PIO0_MASK_OUT)) ;

    // Port -1-
    GPIO1->FIODIR &= (~(PIO1_MASK_OUT)) ;

    // Port -2-
    GPIO2->FIODIR &= (~(PIO2_MASK_OUT)) ;

    // Port -3-
    GPIO3->FIODIR &= (~(PIO3_MASK_OUT)) ;

    // Port -4-
    GPIO4->FIODIR &= (~(PIO4_MASK_OUT)) ;
#endif // SET_ALL_AS_INPUT

#ifdef M3108
#else // M3108
#ifndef G100
    // PPS as out at '0'
    GPIO1->FIODIR |= (1<<19) ;                  // output
    GPIO1->FIOCLR  = (1<<19) ;                  // output at '0'
    // ANT CC OK as out at '0'
    GPIO2->FIODIR |= (3<<7) ;                   // output
    GPIO2->FIOCLR  = (3<<7) ;                   // output at '0'
#endif
#endif // M3108

// #ifdef PIO0_OUT0_OFF
// 	GPIO0->FIOCLR  = PIO0_OUT0_OFF ;
// #endif
// #ifdef PIO1_OUT0_OFF
// 	GPIO1->FIOCLR  = PIO1_OUT0_OFF ;
// #endif
// #ifdef PIO2_OUT0_OFF
// 	GPIO2->FIOCLR  = PIO2_OUT0_OFF ;
// #endif
// #ifdef PIO3_OUT0_OFF
// 	GPIO3->FIOCLR  = PIO3_OUT0_OFF ;
// #endif
// #ifdef PIO4_OUT0_OFF
// 	GPIO4->FIOCLR  = PIO4_OUT0_OFF ;
// #endif
    // interrupts are not disabled, they are useful for wakeup

}

//----------------------------------------------------------------------------
// Read digital input port
// port values:
//      0 - 4   internal PORTs, 32 bit

unsigned long dio_read(int port)
{
    unsigned long retval = 0 ;

    switch(port) {
    case PORT_PIO0 : retval = GPIO0->FIOPIN ; break ;
    case PORT_PIO1 : retval = GPIO1->FIOPIN ; break ;
    case PORT_PIO2 : retval = GPIO2->FIOPIN ; break ;
    case PORT_PIO3 : retval = GPIO3->FIOPIN ; break ;
    case PORT_PIO4 : retval = GPIO4->FIOPIN ; break ;
#ifdef USE_MAX7324_TW1
    case PORT_TW1 : // TWI-1
        {
            KS_lockw(TWIPORT) ;         // we trust with
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&retval)), 2) ;
            TWI_txrx(MAX7324_R1_ADDR, 0, ((unsigned char *)(&retval)), 2) ;
            //TWI_receive(MAX7324_W1_ADDR, ((unsigned char *)(&retval))+2, 1) ;
            TWI_txrx(MAX7324_W1_ADDR, 0, ((unsigned char *)(&retval))+2, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with

            retval |= (tw1in_change & 0xff00) ;
            tw1in_change = 0 ;
#ifdef USE_DEBUG____
            {
                extern char cbugflag ;
                if (retval & 0xff00) {
                    if (!cbugflag) pdebugt(1,"----FLAG TW1 0x%lx", retval) ;
                }
            }
#endif // #ifdef USE_DEBUG____
        }
        break ;
#endif // USE_MAX7324_TW1

#ifdef USE_MAX7324_TW2
    case PORT_TW2 : // TWI-2
        {
            KS_lockw(TWIPORT) ;         // we trust with
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&retval)), 2) ;
            TWI_txrx(MAX7324_R2_ADDR, 0, ((unsigned char *)(&retval)), 2) ;
            //TWI_receive(MAX7324_W1_ADDR, ((unsigned char *)(&retval))+2, 1) ;
            TWI_txrx(MAX7324_W2_ADDR, 0, ((unsigned char *)(&retval))+2, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with

            retval |= (tw2in_change & 0xff00) ;
            tw2in_change = 0 ;
#ifdef USE_DEBUG____
            {
                extern char cbugflag ;
                if (retval & 0xff00) {
                    if (!cbugflag) pdebugt(1,"----FLAG TW2 0x%lx", retval) ;
                }
            }
#endif // #ifdef USE_DEBUG____
        }
        break ;
#endif // USE_MAX7324_TW2

#ifdef USE_TWI_SRV
    case PORT_TWS : // USE_TWI1_AUXILIARY
        {
            KS_lockw(TWIPORT) ;         // we trust with
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&retval)), 2) ;
            TWI1_txrx(MAX7324_R1_ADDR, 0, ((unsigned char *)(&retval)), 2) ;
            //TWI_receive(MAX7324_W1_ADDR, ((unsigned char *)(&retval))+2, 1) ;
            TWI1_txrx(MAX7324_W1_ADDR, 0, ((unsigned char *)(&retval))+2, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with

            retval |= (tw1in1_change & 0xff00) ;
            tw1in1_change = 0 ;
#ifdef USE_DEBUG____
            {
                extern char cbugflag ;
                if (retval & 0xff00) {
                    if (!cbugflag) pdebugt(1,"----FLAG TWS 0x%lx", retval) ;
                }
            }
#endif // #ifdef USE_DEBUG____
        }
        break ;
#endif // USE_TWI_SRV
    }
    return(retval) ;
}

//----------------------------------------------------------------------------
// Write digital output port
// port values:
//      0 - 4   internal PORTs, 32 bit

void dio_write(int port, int pmask, int pval)
{
    switch(port) {
    case PORT_PIO0 : GPIO0->FIOSET = pmask & pval ; GPIO0->FIOCLR = pmask & (~pval) ; break ;
    case PORT_PIO1 : GPIO1->FIOSET = pmask & pval ; GPIO1->FIOCLR = pmask & (~pval) ; break ;
    case PORT_PIO2 : GPIO2->FIOSET = pmask & pval ; GPIO2->FIOCLR = pmask & (~pval) ; break ;
    case PORT_PIO3 : GPIO3->FIOSET = pmask & pval ; GPIO3->FIOCLR = pmask & (~pval) ; break ;
    case PORT_PIO4 : GPIO4->FIOSET = pmask & pval ; GPIO4->FIOCLR = pmask & (~pval) ; break ;
#ifdef USE_MAX7324_TW1  // I/O n. 1
    case PORT_TW1 : // TWI-1
        {
            unsigned char c ;
            KS_lockw(TWIPORT) ;         // we trust with
            // old value
            //TWI_receive(MAX7324_W1_ADDR, &c, 1) ;
            TWI_txrx(MAX7324_W1_ADDR, 0, &c, 1) ;
            // change value
            c |= (pmask & pval) ;
            c &= ((~pmask) | pval) ;
            // new value
            TWI_send(MAX7324_W1_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_MAX7324_TW1

#ifdef USE_MAX7324_TW2  // I/O n. 2
    case PORT_TW2 : // TWI-1
        {
            unsigned char c ;
            KS_lockw(TWIPORT) ;         // we trust with
            // old value
            //TWI_receive(MAX7324_W1_ADDR, &c, 1) ;
            TWI_txrx(MAX7324_W2_ADDR, 0, &c, 1) ;
            // change value
            c |= (pmask & pval) ;
            c &= ((~pmask) | pval) ;
            // new value
            TWI_send(MAX7324_W2_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_MAX7324_TW2

#ifdef USE_TWI_SRV  // I/O n. 2
    case PORT_TWS : // USE_TWI1_AUXILIARY
        {
            unsigned char c ;
            KS_lockw(TWIPORT) ;         // we trust with
            // old value
            //TWI_receive(MAX7324_W1_ADDR, &c, 1) ;
            TWI1_txrx(MAX7324_W1_ADDR, 0, &c, 1) ;
            // change value
            c |= (pmask & pval) ;
            c &= ((~pmask) | pval) ;
            // new value
            TWI1_send(MAX7324_W1_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_TWI_SRV

    }
}

//----------------------------------------------------------------------------
// Write digital input port mask
// port values:
//      5       external optional MAX7324 TW1, 8 output data bit

void dio_mask(int port, int pmask)
{
    switch(port) {
#ifdef USE_MAX7324_TW1  // I/O n. 1
    case PORT_TW1 : // TWI-1
        {
            unsigned char c = (unsigned char)(pmask) ;
            KS_lockw(TWIPORT) ;         // we trust with
            // before get input modification
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&tw1in_change)), 2) ;
            TWI_txrx(MAX7324_R1_ADDR, 0, ((unsigned char *)(&tw1in_change)), 2) ;
            // new value
            TWI_send(MAX7324_R1_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_MAX7324_TW1

#ifdef USE_MAX7324_TW2  // I/O n. 2
    case PORT_TW2 : // TWI-1
        {
            unsigned char c = (unsigned char)(pmask) ;
            KS_lockw(TWIPORT) ;         // we trust with
            // before get input modification
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&tw1in_change)), 2) ;
            TWI_txrx(MAX7324_R2_ADDR, 0, ((unsigned char *)(&tw2in_change)), 2) ;
            // new value
            TWI_send(MAX7324_R2_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_MAX7324_TW2

#ifdef USE_TWI_SRV  // I/O n. 2
    case PORT_TWS : // USE_TWI1_AUXILIARY
        {
            unsigned char c = (unsigned char)(pmask) ;
            KS_lockw(TWIPORT) ;         // we trust with
            // before get input modification
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&tw1in_change)), 2) ;
            TWI1_txrx(MAX7324_R1_ADDR, 0, ((unsigned char *)(&tw1in1_change)), 2) ;
            // new value
            TWI1_send(MAX7324_R1_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_TWI_SRV 
	}
}

//#if defined(USE_CNTTIMER0) || defined(USE_CNTTIMER1) || defined(USE_CNTTIMER2)
//----------------------------------------------------------------------------
// Read counter value
// port values:
//      0       connected to GPS 1 PPS
//      1       connected to odometer
//      2       auxiliary

unsigned short dio_counter(int port)
{
    switch(port) {
#if defined(USE_CNTTIMER1)
    case 0:
        return(TIM1->TC) ;
#endif // defined(USE_CNTTIMER1)

#if defined(USE_CNTTIMER2)
    case 1:
        return(TIM2->TC) ;
#endif // defined(USE_CNTTIMER2)

#if defined(USE_CNTTIMER0)
    case 2:
        return(TIM0->TC) ;
#endif // defined(USE_CNTTIMER0)

    default :
        break ;
    }
    return(0) ;
}
//#endif // defined(USE_CNTTIMER0) || defined(USE_CNTTIMER1) || defined(USE_CNTTIMER2)

#if defined(USE_FREQTIMER1)
//----------------------------------------------------------------------------
void dio_beeper(int freq)
{
    if (freq) {     // enable
        // P1.22 = Timer1 CH0 - waveform generator
        PINCON->PINSEL3 |= 0x00003000 ;
        TIM1->TCR = 2 ;     // reset
        TIM1->CTCR = 0 ;    // count at PCLK
        TIM1->MR0 = current_clock/4/freq/2 ;  // desired frequency
        TIM1->MCR = 2 ;     // reset TC on match with MR0
        TIM1->EMR = 0x30 ;  // toggle pin on match
        TIM1->TCR = 1 ;     // enable

    } else {        // disable

        // P1.22 = Timer1 CH0 - waveform generator
        PINCON->PINSEL3 &= ~0x00003000 ;            // digital
        GPIO1->FIODIR |= (1<<22) ;                  // output
        GPIO1->FIOCLR  = (1<<22) ;                  // output at '0'
    }
}
#endif // defined(USE_FREQTIMER1)
