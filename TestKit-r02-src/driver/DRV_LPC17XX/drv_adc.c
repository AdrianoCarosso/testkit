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
    SC->PCONP |= PCONP_PCAD ;           // enable it

    // all the related pins are set to ADC inputs, AD0.0-5 (Don't use A0.3 if DAC desired)
#if defined(USE_DAC)
    PINCON->PINSEL1 &= ~0x000FC000 ;    // P0.23~25, A0.0~2, function 01
    PINCON->PINSEL1 |=  0x00054000 ;
#else // defined(USE_DAC)
    PINCON->PINSEL1 &= ~0x003FC000 ;    // P0.23~26, A0.0~3, function 01
    PINCON->PINSEL1 |=  0x00154000 ;
#endif // defined(USE_DAC)
    PINCON->PINSEL3 |=  0xF0000000 ;    // P1.30~31, A0.4~5, function 11

    // all the related pins are set with NO pull-up/down
#if defined(USE_DAC)
    PINCON->PINMODE1 &= ~0x000FC000 ;   // P0.23~25, A0.0~2, function 10
    PINCON->PINMODE1 |=  0x000A8000 ;
#else // defined(USE_DAC)
    PINCON->PINMODE1 &= ~0x003FC000 ;   // P0.23~26, A0.0~3, function 10
    PINCON->PINMODE1 |=  0x002A8000 ;
#endif // defined(USE_DAC)
    PINCON->PINMODE3 &= ~0xF0000000 ;   // P1.30~31, A0.4~5, function 10
    PINCON->PINMODE3 |=  0xA0000000 ;

    // default div by 4
    //SC->PCLKSEL0 &= ~0x03000000 ;       // bit25,24: 00 - PCLK = CCLK / 4

    ADC->ADCR = ( AD_ADC0_MASK << 0 ) | // Mask of desired channels
                ( ( current_clock/4/1250000 - 1 ) << 8 ) |   // sample at 1.25 MHz
                ( 1 << 16 ) |           // BURST = 1, BURST ontrolled
                ( 1 << 21 ) |           // PDN = 1, normal operation
                ( 0 << 24 ) |           // START = 0 A/D conversion for BURST mode
                ( 0 << 27 ) ;           // EDGE = 0 (CAP/MAT singal falling,trigger A/D conversion)
}

//----------------------------------------------------------------------------
// ADC stop

void adcstop(void)
{

    // GPIO set at '0'
//    GPIO0->FIOCLR  = 0x07800000 ;
//    GPIO0->FIODIR |= 0x07800000 ;
//    GPIO1->FIOCLR  = 0xC0000000 ;
//    GPIO1->FIODIR |= 0xC0000000 ;

    // GPIO again, function 00
//    PINCON->PINSEL1 &= ~0x003FC000 ;    // P0.23~26, A0.0~3, function 00
//    PINCON->PINSEL3 &= ~0xF0000000 ;    // P1.30~31, A0.4~5, function 00

    ADC->ADCR = 0 ;                     // disable

    SC->PCONP &= (~PCONP_PCAD) ;        // disable it
}

// *********************************************************************
// AD converter handler

void ADC_readready(int chn)
{
    unsigned long volatile *adc = (unsigned long *)(&(ADC->ADDR0)) ;

    if ((chn >= 0) && (chn <= 5)) {
        // wait for new conversion complete
        while( !(adc[chn] & 0x80000000) )
            tickwait(10) ;
    }
}

#ifdef M3108

#ifdef USE_ADC_MUX_ON_ARM
volatile unsigned short muxed_adc[8] ;
#endif // USE_ADC_MUX_ON_ARM

int ADC_read(int chn)
{
    unsigned long volatile *adc = (unsigned long *)(&(ADC->ADDR0)) ;

    if ((chn >= 0) && (chn <= 5)) {
        return((adc[chn]>>4) & 0xfff) ;
    }
#ifdef USE_ADC_MUX_ON_ARM
    if ((chn >= 8) && (chn <= 15)) {
        return((muxed_adc[chn - 8]>>4) & 0xfff) ;
    }
#endif // USE_ADC_MUX_ON_ARM
    return(0) ;
}

#else // M3108

extern unsigned short ExtAD0[16] ; // [4] ;
extern unsigned short ADVExt[16] ;

int ADC_read(int chn)
{
#ifdef M2102C
    unsigned short i, ad0 ;
#endif
	const unsigned long *adc = (unsigned long *)(&(ADC->ADDR0)) ;

#ifdef M3208
#ifdef  PRE_RELEASE		// first 4 board  HwRevBRD=0
	switch(chn){
		case 1: chn=4 ; break ;
		case 2: chn=5 ; break ;
		case 4: chn=1 ; break ;
		case 5: chn=2 ; break ;
	}
#else
	if (chn==6) chn = 0 ;	// INAN2 on AD0
#endif	// PRE_RELEASE

#endif // M3208

	//if (!chn) return((ExtAD0>>4) & 0xfff) ; // Return last stored data
#ifdef M2102C
	if ( (chn==FIRST_AD) && (!HwRevBRD) ) {
		ad0 = 0 ;
		for(i=0;i<16;i++) ad0 += ExtAD0[i];
		// //return((ExtAD0>>4) & 0xfff) ; // Return last stored data
		return((ad0>>4)) ;
	}

	if ( (chn==AD_VEXT) && (!HwRevBRD) ) {
		ad0 = 0 ;
		for(i=0;i<16;i++) ad0 += ADVExt[i];
		// //return((ExtAD0>>4) & 0xfff) ; // Return last stored data
		return((ad0>>4)) ;
	}

    if ((chn >= 0) && (chn <= 5)) {
        return((adc[chn]>>4) & 0xfff) ;
    }
#else
    if ((chn >= 0) && (chn <= 5)) {
        return((adc[chn]>>4) & 0xfff) ;
    }
#endif

    return(0xfff) ; // old default 0
}

#endif // M3108

