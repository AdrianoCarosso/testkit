// drv_twi.c - ADC driver tasks
//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//

//
// This module is provided as a ADC port driver.
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

#define NULLSEMA ((SEMA)0)

// Coefficient for adjust reference
float ADcoeff = 1.0 ;
//----------------------------------------------------------------------------
// only if FAST adc has been selected

#ifdef USE_ADC_FAST_ON_ARM

// function for XYZ accelerometer handling
extern void XYZ_accel_init(void) ;
extern void XYZ_accel_end(void) ;
extern void XYZ_accel_step(unsigned short *ptr, int len) ;

// local data

#define ADC1_FREQUENCY  1000    // Sampling frequency in Hz
#define ADC1_BUFSIZE    512     // Sample buffer
#define ADC1_CHANNELS   3       // 3 axis accelerometer
#define ADC1_HALF       ((ADC1_BUFSIZE * ADC1_CHANNELS)/2)
unsigned short adc1buff[ADC1_BUFSIZE * ADC1_CHANNELS] ;
unsigned char adc1buffidx ;
unsigned char adcrunning ;

// counters useful to identify overrun situations
unsigned char adcpush ;         // counter of how many created buffers
unsigned char adcpop ;          // counter of how many used buffers
#else
#ifndef AD_ADC1_MASK
#define AD_ADC1_MASK 0x13
#endif
#endif // USE_ADC_FAST_ON_ARM

//----------------------------------------------------------------------------
// functions in drv_clk.c

extern void AT91F_AIC_Configure(int irq_id, int priority, int src_type, FRAME *(*newHandler)(FRAME *frame)) ;

//----------------------------------------------------------------------------
// internal functions

void adcstart(void) ;
void adcstop(void) ;

//----------------------------------------------------------------------------
// Timer Interrupt for MUX handling

#ifdef USE_ADC_MUX_ON_ARM
FRAME *tc0drv(FRAME * frame) ;

unsigned char muxnum ;
unsigned short adcmuxval[8] ;

FRAME *tc0drv(FRAME * frame)
{
    register volatile unsigned long status ;

// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    status = AT91C_BASE_TCB0->TCB_TC0.TC_SR ;   // get actual status
    
//                // let led to blink
//                if (AT91C_BASE_PIOA->PIO_PDSR & PIOA_LED) {
//                    // set at 0 (turn led on)
//                    AT91C_BASE_PIOA->PIO_CODR = PIOA_LED ;
//                } else {
//                    // set at 1 (turn led off)
//                    AT91C_BASE_PIOA->PIO_SODR = PIOA_LED ;
//                }

    // read last MUX value
    adcmuxval[muxnum] = *(AT91C_ADC0_CDR0 + 7) ;
    
    // set new val
    muxnum++ ;
    muxnum &= 7 ;       // max 8 MUX channels
    
    // prepare new MUX
    AT91C_BASE_PIOA->PIO_CODR = ((muxnum ^ 7)<<19) ;    // set at 0
    AT91C_BASE_PIOA->PIO_SODR = (muxnum<<19) ;          // set at 1

    return(KS_ISRexit(frame, NULLSEMA)) ;
}
#endif // USE_ADC_MUX_ON_ARM

//----------------------------------------------------------------------------
// only if FAST adc has been selected

#ifdef USE_ADC_FAST_ON_ARM

void adctask(void) TASK_ATTRIBUTE ;

FRAME *adcdrv(FRAME * frame) ;

//----------------------------------------------------------------------------
// Interrupt routine for ADC -1-

FRAME *adcdrv(FRAME * frame)
{
    register volatile unsigned long status ;
// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    status = AT91C_BASE_ADC1->ADC_SR ;   // get actual status

    // Check for received buffer
    if (status & (AT91C_BASE_ADC1->ADC_IMR) & AT91C_ADC_ENDRX) {
    
        // change ping-pong pointer
        adc1buffidx ^= 1 ;
        // prepare next buffer
        AT91C_BASE_ADC1->ADC_RNPR = (unsigned long)(&adc1buff[adc1buffidx * ADC1_HALF]) ;
        AT91C_BASE_ADC1->ADC_RNCR = ADC1_HALF ;

        // one more buffer generated
        adcpush++ ;
        return(KS_ISRexit(frame, ADCSEM)) ;
    }

    return(KS_ISRexit(frame, NULLSEMA)) ;
}

//----------------------------------------------------------------------------
// ADC task

void adctask(void)
{
#ifdef USE_PERFORMANCE_EVALUATION
    KS_delay(SELFTASK, ((TICKS)12000*CLKRATE/1000)) ;   // skip time
#endif // USE_PERFORMANCE_EVALUATION

    // start XYZ accelerometer handler
    XYZ_accel_init() ;

    // enable DMA
    AT91C_BASE_ADC1->ADC_PTCR = AT91C_PDC_RXTEN ;
    // Start first conversion
    AT91C_BASE_ADC1->ADC_CR = AT91C_ADC_START ;
    // Enable ADC 1 interrupt
    AT91C_BASE_ADC1->ADC_IER = AT91C_ADC_ENDRX ;

    adcrunning = YES ;

    for( ; ; ) {
        KS_wait(ADCSEM) ;   // wait for data
        // call periodic handler
        XYZ_accel_step(&adc1buff[adc1buffidx * ADC1_HALF], ADC1_BUFSIZE/2) ;

        // check for overrun
        adcpop++ ;      // one more buffer generated
        if (adcpop != adcpush) {
#ifdef CBUG
            pdebugt(1,"ADC overrun") ;
#endif // CBUG
            adcpop = adcpush - 1 ;
        }
    }
}

#endif // USE_ADC_FAST_ON_ARM

//----------------------------------------------------------------------------
// ADC initializer

void adcstart(void)
{
    // ------------------------------------------------------------
    // init Timer 0 at 10Hz for ADC0 and, eventually, ADC1

    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC0) ;

#if defined(USE_AT91SAM7A3)
    // Timer 0 (0 of A)
    // mode: WAVE (WSEL=10), input is TIMER_CLOCK5: current_clock/1024
    AT91C_BASE_TCB0->TCB_TC0.TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK |
                                      AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_WAVE |
                                      AT91C_TC_ACPA_SET |   // RA sets TIOA
                                      AT91C_TC_ACPC_CLEAR | // RC clears TIOA
                                      AT91C_TC_ASWTRG_SET ; // SwTrig sets TIOA

    // Timer 0 reference registers (A and B something in between)
    AT91C_BASE_TCB0->TCB_TC0.TC_RA = current_clock / 1024 / 30 ;
    AT91C_BASE_TCB0->TCB_TC0.TC_RB = current_clock / 1024 / 20 ;
    // NOUSB: 60MHz / 1024 / 5859 = 10 Hz
    // USB:   48MHz / 1024 / 4687 = 10 Hz
    //         1MHz / 1024 /   98 = 10 Hz
    AT91C_BASE_TCB0->TCB_TC0.TC_RC = current_clock / 1024 / 10 ;
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    // Timer 0 (0 of A)
    // mode: WAVE (WSEL=10), input is TIMER_CLOCK5: current_clock/1024
    AT91C_BASE_TCB->TCB_TC0.TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK |
                                     AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_WAVE |
                                     AT91C_TC_ACPA_SET |   // RA sets TIOA
                                     AT91C_TC_ACPC_CLEAR | // RC clears TIOA
                                     AT91C_TC_ASWTRG_SET ; // SwTrig sets TIOA

    // Timer 0 reference registers (A and B something in between)
    AT91C_BASE_TCB->TCB_TC0.TC_RA = current_clock / 1024 / 30 ;
    AT91C_BASE_TCB->TCB_TC0.TC_RB = current_clock / 1024 / 20 ;
    // NOUSB: 60MHz / 1024 / 5859 = 10 Hz
    // USB:   48MHz / 1024 / 4687 = 10 Hz
    //         1MHz / 1024 /   98 = 10 Hz
    AT91C_BASE_TCB->TCB_TC0.TC_RC = current_clock / 1024 / 10 ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#ifdef USE_ADC_MUX_ON_ARM
    // Init data  at 0xffff (not value)
    for(muxnum=0;muxnum<8;muxnum++) adcmuxval[muxnum] = 0xffff ;

    // Install interrupt on Timer 0 for MUX Handling
    AT91F_AIC_Configure(AT91C_ID_TC0, TC0_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL/*AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE*/, tc0drv) ;
    // Enable Interrupt on TA match (1/3 T)
    AT91C_BASE_TCB0->TCB_TC0.TC_IER = AT91C_TC_CPAS ;
    //muxnum = 7 ;        // first MUX BRIGNOLO
    muxnum = 4 ;        // first MUX ROVERA

#endif // USE_ADC_MUX_ON_ARM

#if defined(USE_AT91SAM7A3)
    // Timer 0 control: enable
    AT91C_BASE_TCB0->TCB_TC0.TC_CCR = AT91C_TC_CLKEN ;

    // Timer 0 control: start
    AT91C_BASE_TCB0->TCB_TC0.TC_CCR = AT91C_TC_SWTRG ;
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    // Timer 0 control: enable
    AT91C_BASE_TCB->TCB_TC0.TC_CCR = AT91C_TC_CLKEN ;

    // Timer 0 control: start
    AT91C_BASE_TCB->TCB_TC0.TC_CCR = AT91C_TC_SWTRG ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

    // ------------------------------------------------------------
    // init ADC controller(s)
    // used for 8 general purpose A/D

    // Peripheral Clock Enable Register
#if defined(USE_AT91SAM7A3)
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_ADC0) ;
    // disable pull-up
    AT91C_BASE_PIOB->PIO_PPUDR = (0xff << 14) ; // no pull up

    // Reset peripheral
    AT91C_BASE_ADC0->ADC_CR = AT91C_ADC_SWRST ;

    // ATTENTION current_clock must be > 2MHz
    // configure mode register
    // A/D trigger is Timer 0
    // Conversion at full 10 bit
    // Conversion clock = 1 MHz (max 5 MHz)
    //  NOUSB: PRESCA = (60MHz / 2MHz) - 1 = 29
    //  USB:   PRESCA = (48MHz / 2MHz) - 1 = 23
    // Startup time = 104 ussec (min 20 usec)
    // Track+Hold = 10 usec (min 0.6 usec)
//    AT91C_BASE_ADC0->ADC_MR = AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_TIOA0 |
//                              AT91C_ADC_LOWRES_10_BIT | AT91C_ADC_SLEEP_MODE | 
//                              ((((current_clock/2000000)-1) <<  8) & AT91C_ADC_PRESCAL) | //  M3008 -> 27 ; M1109 -> 3
//                              ((12 << 16) & AT91C_ADC_STARTUP) |
//                              ((9  << 24) & AT91C_ADC_SHTIM) ;

    AT91C_BASE_ADC0->ADC_MR = AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_TIOA0 |
                              AT91C_ADC_LOWRES_10_BIT | AT91C_ADC_SLEEP_MODE | 
                              ((((current_clock/4000000)-1) <<  8) & AT91C_ADC_PRESCAL) | //  M3008 -> 13 ; M1109 -> 1
                              ((16 << 16) & AT91C_ADC_STARTUP) |
                              ((3  << 24) & AT91C_ADC_SHTIM) ;

    // enable only channel used
    AT91C_BASE_ADC0->ADC_CHER = AD_ADC0_MASK ;

    // Start first conversion
    AT91C_BASE_ADC0->ADC_CR = AT91C_ADC_START ;
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_ADC) ;

    // Reset peripheral
    AT91C_BASE_ADC->ADC_CR = AT91C_ADC_SWRST ;

    // ATTENTION current_clock must be > 2MHz
    // configure mode register
    // A/D trigger is Timer 0
    // Conversion at full 10 bit
    // Conversion clock = 1 MHz (max 5 MHz)
    //  NOUSB: PRESCA = (60MHz / 2MHz) - 1 = 29
    //  USB:   PRESCA = (48MHz / 2MHz) - 1 = 23
    // Startup time = 104 ussec (min 20 usec)
    // Track+Hold = 10 usec (min 0.6 usec)
// UIp to 1.31    
//    AT91C_BASE_ADC->ADC_MR = AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_TIOA0 |
//                             AT91C_ADC_LOWRES_10_BIT | AT91C_ADC_SLEEP_MODE | 
//                             ((((current_clock/2000000)-1) <<  8) & AT91C_ADC_PRESCAL) | 
//                             ((12 << 16) & AT91C_ADC_STARTUP) |
//                             ((9  << 24) & AT91C_ADC_SHTIM) ;

    AT91C_BASE_ADC->ADC_MR = AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_TIOA0 |
                             AT91C_ADC_LOWRES_10_BIT | AT91C_ADC_SLEEP_MODE | 
                             ((((current_clock/4000000)-1) <<  8) & AT91C_ADC_PRESCAL) | 
                             ((16 << 16) & AT91C_ADC_STARTUP) |
                             ((3  << 24) & AT91C_ADC_SHTIM) ;

    // enable only channel used
    AT91C_BASE_ADC->ADC_CHER = AD_ADC0_MASK ;

    // Start first conversion
    AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)


    // read values with
    // val = AT91C_BASE_ADC0->ADC_CDRx ;

#if defined(USE_AT91SAM7A3)
#ifndef USE_ADC_FAST_ON_ARM  // if ADC1 is NOT used for hi speed DMA, configure at 10Hz
    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_ADC1) ;

    // disable pull-up
    AT91C_BASE_PIOB->PIO_PPUDR = (AD_ADC1_MASK << 22) ; // no pull up

    // Reset peripheral
    AT91C_BASE_ADC1->ADC_CR = AT91C_ADC_SWRST ;

    // ATTENTION current_clock must be > 2MHz
    // configure mode register
    // A/D trigger is Timer 0
    // Conversion at full 10 bit
    // Conversion clock = 1 MHz (max 5 MHz)
    //  NOUSB: PRESCA = (60MHz / 2MHz) - 1 = 29
    //  USB:   PRESCA = (48MHz / 2MHz) - 1 = 23
    // Startup time = 104 ussec (min 20 usec)
    // Track+Hold = 10 usec (min 0.6 usec)
    AT91C_BASE_ADC1->ADC_MR = AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_TIOA0 |
                              AT91C_ADC_LOWRES_10_BIT | AT91C_ADC_SLEEP_MODE | 
                              ((((current_clock/2000000)-1) <<  8) & AT91C_ADC_PRESCAL) |
                              ((12 << 16) & AT91C_ADC_STARTUP) |
                              ((9  << 24) & AT91C_ADC_SHTIM) ;

    // enable channels 0, 1, 4
    AT91C_BASE_ADC1->ADC_CHER = AD_ADC1_MASK ;

    // Start first conversion
    AT91C_BASE_ADC1->ADC_CR = AT91C_ADC_START ;

    // read values with
    // val = AT91C_BASE_ADC1->ADC_CDRx ;
#endif // USE_ADC_FAST_ON_ARM

#ifdef USE_ADC_FAST_ON_ARM
    // ------------------------------------------------------------
    // init Timer 1 for ADC1 (1000Hz)

    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC1) ;

    // Timer 1 (1 of A)
    // mode: WAVE (WSEL=10), input is TIMER_CLOCK3: current_clock/32
    AT91C_BASE_TCB0->TCB_TC1.TC_CMR = AT91C_TC_CLKS_TIMER_DIV3_CLOCK |
                                      AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_WAVE |
                                      AT91C_TC_ACPA_SET |   // RA sets TIOA
                                      AT91C_TC_ACPC_CLEAR | // RC clears TIOA
                                      AT91C_TC_ASWTRG_SET ; // SwTrig sets TIOA

    // Timer 1 reference registers (A and B something in between)
    AT91C_BASE_TCB0->TCB_TC1.TC_RA = current_clock / 32 / 3000 ;
    AT91C_BASE_TCB0->TCB_TC1.TC_RB = current_clock / 32 / 2000 ;
    // NOUSB: 60MHz / 32 / 1875 = 1000 Hz
    // USB:   48MHz / 32 / 1500 = 1000 Hz
    //         1MHz / 32 / 31   = 1000 Hz
    AT91C_BASE_TCB0->TCB_TC1.TC_RC = current_clock / 32 / 1000 ;

    // Timer 1 control: enable
    AT91C_BASE_TCB0->TCB_TC1.TC_CCR = AT91C_TC_CLKEN ;

    // Timer 1 control: start
    AT91C_BASE_TCB0->TCB_TC1.TC_CCR = AT91C_TC_SWTRG ;

    // ------------------------------------------------------------
    // init ADC controller -1-
    // used for 3 axis accelerometer

    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_ADC1) ;

    // disable pull-up
    AT91C_BASE_PIOB->PIO_PPUDR = (0x13 << 22) ; // no pull up

    // Reset peripheral
    AT91C_BASE_ADC1->ADC_CR = AT91C_ADC_SWRST ;

    // ATTENTION current_clock must be > 2MHz
    // configure mode register
    // A/D trigger is Timer1
    // Conversion at full 10 bit
    // Conversion clock = 1 MHz (max 5 MHz)
    //  NOUSB: PRESCA = (60MHz / 2MHz) - 1 = 29
    //  USB:   PRESCA = (48MHz / 2MHz) - 1 = 23
    // Startup time = 104 ussec (min 20 usec)
    // Track+Hold = 10 usec (min 0.6 usec)
    AT91C_BASE_ADC1->ADC_MR = AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_TIOA1 |
                              AT91C_ADC_LOWRES_10_BIT | AT91C_ADC_SLEEP_MODE |
                              ((((current_clock/2000000)-1) <<  8) & AT91C_ADC_PRESCAL) |
                              ((12 << 16) & AT91C_ADC_STARTUP) |
                              ((9  << 24) & AT91C_ADC_SHTIM) ;

    // enable 3 channels: 0, 1, 4
    AT91C_BASE_ADC1->ADC_CHER = 0x13 ;

    // init DMA
    AT91C_BASE_ADC1->ADC_RPR = (unsigned long)(&adc1buff[0]) ;
    AT91C_BASE_ADC1->ADC_RCR = ADC1_HALF ;
    // prepare next
    AT91C_BASE_ADC1->ADC_RNPR = (unsigned long)(&adc1buff[ADC1_HALF]) ;
    AT91C_BASE_ADC1->ADC_RNCR = ADC1_HALF ;
    // init ping-pong pointer
    adc1buffidx = 1 ;

    // Install handler
    AT91F_AIC_Configure(AT91C_ID_ADC1, ADC1_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL/*AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE*/, adcdrv) ;

    // OK, here we are
    adcpush = 0 ;       // counter of how many created buffers
    adcpop = 0 ;        // counter of how many used buffers
#endif // USE_ADC_FAST_ON_ARM
#endif // defined(USE_AT91SAM7A3)
}

//----------------------------------------------------------------------------
// ADC stop

void adcstop(void)
{
#ifdef USE_ADC_FAST_ON_ARM
    if ( !adcrunning) return ;

    // stop ADC
    adcrunning = NO ;

    // Disable timer
    AT91C_BASE_TCB0->TCB_TC1.TC_CCR = AT91C_TC_CLKDIS ;

    // Disable peripheral
    AT91C_BASE_ADC1->ADC_MR = 0 ;

    // Reset peripheral
    AT91C_BASE_ADC1->ADC_CR = AT91C_ADC_SWRST ;

    // Disable ADC 1 interrupt
    AT91C_BASE_ADC1->ADC_IDR = AT91C_ADC_ENDRX ;

#ifdef CBUG
    {
        extern char cbugflag ;
        // wait for end of activities
        if (!cbugflag) {    // only if not in debugger
            KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;     // skip time
        }
    }
#else // CBUG
    // wait for end of activities
    KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;     // skip time
#endif // CBUG

    // stop XYZ accelerometer handler
    XYZ_accel_end() ;
    
    // stop task
    KS_terminate(ADCTASK) ;	// terminate task

    // disable clocks
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_TC1) | (1 << AT91C_ID_ADC1) ;
#endif // USE_ADC_FAST_ON_ARM

    // Disable ADC -0-

#if defined(USE_AT91SAM7A3)
#ifdef USE_ADC_MUX_ON_ARM
    AT91C_BASE_TCB0->TCB_TC0.TC_IDR = -1 ;      // disable all interrupts
#endif // USE_ADC_MUX_ON_ARM

    // Disable timer -0- (ADC -0- trigger)
    AT91C_BASE_TCB0->TCB_TC1.TC_CCR = AT91C_TC_CLKDIS ;

    // Reset peripheral ADC -0-
    AT91C_BASE_ADC0->ADC_CR = AT91C_ADC_SWRST ;

    // disable channels
    AT91C_BASE_ADC0->ADC_CHER = 0 ;
    AT91C_BASE_ADC1->ADC_CHER = 0 ;

    // Disable clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_ADC0) | (1 << AT91C_ID_TC0) ;

#ifndef USE_ADC_FAST_ON_ARM
    // if ADC1 is NOT used for hi speed DMA, disable

    // Disable clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_ADC1) ;
#endif // USE_ADC_FAST_ON_ARM  // if ADC1 is NOT used for hi speed DMA, configure at 10Hz
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    // Disable timer -0- (ADC -0- trigger)
    AT91C_BASE_TCB->TCB_TC1.TC_CCR = AT91C_TC_CLKDIS ;

    // Reset peripheral ADC -0-
    AT91C_BASE_ADC->ADC_CR = AT91C_ADC_SWRST ;

    // disable channels
    AT91C_BASE_ADC->ADC_CHER = 0 ;

    // Disable clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_ADC) | (1 << AT91C_ID_TC0) ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
}

// *********************************************************************
// AD converter handler
// read from converter AD number -0- or -1- depending on range
// converter is free running at 10 Hz, reading at higher speed means read
// same value more times
//
// range 0-6    ADC0
// range 8-15   adcmuxval[0-7]
// range 16-18  ACC XYZ
int ADC_read(int chn)
{
    int rawval = 0 ;
#ifdef MTS_CODE
    float new_ad, vref_coef ;
#endif // MTS_CODE

#if defined(USE_AT91SAM7A3)
#ifdef USE_ADC_MUX_ON_ARM
    if ((chn>=0) && (chn<=6)) {
#else
    if ((chn>=0) && (chn<=7)) {
#endif    	
        //return(*(AT91C_ADC0_CDR0 + (chn & 0x7))) ;
        rawval = (*(AT91C_ADC0_CDR0 + (chn & 0x7))) ;
    }

    else if ((chn>=8) && (chn<=15)) {
#ifdef USE_ADC_MUX_ON_ARM
        //return(adcmuxval[chn-8]) ;
        rawval =  (adcmuxval[chn-8]) ;
#else
        rawval = (*(AT91C_ADC1_CDR0 + ((chn-8) & 0x7))) ;
#endif // USE_ADC_MUX_ON_ARM
    }

#ifndef USE_ADC_FAST_ON_ARM
    else if ((chn>=16) && (chn<=18)) {
        if (chn >= 18) chn = 20 ;       // skip fake channels
        //return(*(AT91C_ADC1_CDR0 + ((chn-16) & 0x7))) ;
        rawval = (*(AT91C_ADC1_CDR0 + ((chn-16) & 0x7))) ;
    }
#endif // USE_ADC_FAST_ON_ARM
#endif // defined(USE_AT91SAM7A3)

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    if ((chn>=0) && (chn<=7)) {
        rawval = (*(AT91C_ADC_CDR0 + (chn & 0x7))) ;
    }
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#ifdef MTS_CODE
    if (rawval){ // correct with kAD
    	new_ad = rawval ;
    	vref_coef = 1.015 ;  /// WARNING: WHY this NEEDED ? 
#ifdef M3008
    	vref_coef = 1.0 ;
    	if (!HwRevBRD) vref_coef = 1.015 ;  /// WARNING: WHY this NEEDED ? 
#endif    	
#ifdef M2102
		if (HwRevBRD>1) vref_coef = 1.005 ;
#endif
#ifdef M2015
    	vref_coef = 1.0 ;
#endif
		new_ad /= vref_coef ;
		
    	new_ad *= ADcoeff ;
    	if (new_ad>1023.0) new_ad = 1023.0 ;
    	rawval = new_ad ;
    }
#endif //  MTS_CODE
    return(rawval) ;
}
// end of file - drv_adc.c

