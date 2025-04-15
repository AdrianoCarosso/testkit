// drv_twi.c - ADC driver tasks
//
//   Copyright (c) 1997-2010.
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

//----------------------------------------------------------------------------
// Coefficient for adjust reference

float ADcoeff = 1.0 ;

//----------------------------------------------------------------------------
// internal functions

void adcstart(void) ;
void adcstop(void) ;

//----------------------------------------------------------------------------
// ADC initializer

void adcstart(void)
{
#ifdef USE_TIMER_AS_TRIGGER
    // ------------------------------------------------------------
    // init Timer 0 at 10Hz for ADC0

    // Peripheral Clock Enable Register
    PMC->PMC_PCER0 = (1 << ID_TC0) ;

    // Timer 0 (0 of A)
    // mode: WAVE (WSEL=10), input is TIMER_CLOCK5: slowclock (ex current_clock/1024)
    TC0->TC_CHANNEL[0].TC_CMR = TC_CMR0_TCCLKS_TIMER_CLOCK5 |
                                /*AT91C_TC_WAVESEL_UP_AUTO*/(0x2 << 13) | TC_CMR0_WAVE |
                                TC_CMR0_ACPA_SET |      // RA sets TIOA
                                TC_CMR0_ACPC_CLEAR |    // RC clears TIOA
                                /*AT91C_TC_ASWTRG_SET*/(0x1 << 22) ; // SwTrig sets TIOA

    // Timer 0 reference registers (A and B something in between)
    TC0->TC_CHANNEL[0].TC_RA = 32768 /*current_clock / 1024*/ / 30 ;
    TC0->TC_CHANNEL[0].TC_RB = 32768 /*current_clock / 1024*/ / 20 ;
    TC0->TC_CHANNEL[0].TC_RC = 32768 /*current_clock / 1024*/ / 10 ;

    // Timer 0 control: enable
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR0_CLKEN ;

    // Timer 0 control: start
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR0_SWTRG ;
#endif // USE_TIMER_AS_TRIGGER

    // ------------------------------------------------------------
    // init ADC controller(s)
    // used for 8 general purpose A/D

    // Peripheral Clock Enable Register
    PMC->PMC_PCER0 = (1 << ID_ADC) ;

    // Reset peripheral
    ADC->ADC_CR = ADC_CR_SWRST ;

    // ATTENTION current_clock must be > 2MHz
    // configure mode register
    // A/D trigger is Timer 0
    // Conversion at full 10 bit
    // Conversion clock = 2 MHz (max 20 MHz) --> 500ns
    //  NOUSB: PRESCA = (64MHz / 4MHz) - 1 = 15
    //  USB:   PRESCA = (48MHz / 4MHz) - 1 = 11
    // Startup time = 256 usec (8 --> 512 ADClock) (min 52 us)
    // Track+Hold   =   5 usec (10 --> 10 ADClock) (min 0.16 usec)
// UIp to 1.31    
//    AT91C_BASE_ADC->ADC_MR = AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_TIOA0 |
//                             AT91C_ADC_LOWRES_10_BIT | AT91C_ADC_SLEEP_MODE | 
//                             ((((current_clock/2000000)-1) <<  8) & AT91C_ADC_PRESCAL) | 
//                             ((12 << 16) & AT91C_ADC_STARTUP) |
//                             ((9  << 24) & AT91C_ADC_SHTIM) ;

    ADC->ADC_MR = /*ADC_MR_TRGEN_EN | ADC_MR_TRGSEL_ADC_TRIG1 / *TIOA0* / | */
                             ADC_MR_LOWRES_10_BIT | ADC_MR_SLEEP | 
                             ((((current_clock/4000000)-1) <<  8) & ADC_MR_PRESCAL) | 
                             ((8 << 16) & ADC_MR_STARTUP) |
                             ((10  << 24) & ADC_MR_TRACKTIM) | 
                             (1<<7) /* free running */;

    // enable only channel used
    ADC->ADC_CHER = AD_ADC0_MASK ;

    // Start first conversion
    ADC->ADC_CR = ADC_CR_START ;

    // read values with
    // val = ADC->ADC_CDR[x] ;
}

//----------------------------------------------------------------------------
// ADC stop

void adcstop(void)
{
#ifdef USE_TIMER_AS_TRIGGER
    // Disable timer -0- (ADC -0- trigger)
    TC0->TC_CHANNEL[0].TC_CCR = TC_CCR0_CLKDIS ;
    // Disable clock
    PMC->PMC_PCDR0 = (1 << ID_TC0) ;
#endif // USE_TIMER_AS_TRIGGER

    // Reset peripheral ADC
    ADC->ADC_CR = ADC_CR_SWRST ;

    // disable channels
    ADC->ADC_CHDR = AD_ADC0_MASK ;

    // Disable clock
    PMC->PMC_PCDR0 = (1 << ID_ADC) ;
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

    if ((chn>=0) && (chn<=7)) {
        rawval = ADC->ADC_CDR[chn] ;
    }

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

