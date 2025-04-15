// drv_twi.c - ADC driver tasks
//
//   Copyright (c) 1997-2011.
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


#if defined(USE_FAST_AD_T0)
#define AD_SAMPLE_FREQ      100     // Hz
#define AD_CHANNEL1         AD_VPP //  2
#define AD_CHANNEL2         AD_IPP //  2
unsigned int fastAD1buf, fastAD2buf ;
unsigned short fastAD1val, fastAD2val ;
unsigned char fastADvals ;
unsigned char mADusemax ;
unsigned char Ippclear ;
// into extapi.h
// #define USE_MEAN  0  
// #define USE_MIN   1
// #define USE_MAX   2 
//#define READ_MIN
// unsigned short * FastADbuffer ;     // buffer pointer
// int FastADbufsize ;                 // buffer size (in short)
volatile uint32_t * const iodir_base = ((uint32_t *)(LPC_GPIO0_BASE)) ;
volatile uint32_t * const ioclr_base = ((uint32_t *)(LPC_GPIO0_BASE + 0x1c)) ;

#endif // defined(USE_FAST_AD_T0)

// Coefficient for adjust reference
float ADcoeff = 1.0 ;

//----------------------------------------------------------------------------
// internal functions

void adcstart(void) ;
void adcstop(void) ;

#if defined(USE_FAST_AD_T0)
//----------------------------------------------------------------------------
// Pacer interrupt

void TIMER0_IRQHandler(void)
{
register int ir = LPC_TIM0->IR ;     // don't bother us
//unsigned short fastad ;

    // acquire A/D value
	if (ir){
//		fastADvals--;
		if (Ippclear){
			if ( (mADusemax) && (Ippclear>=mADusemax)) Ippclear=10 ;
			if ((++Ippclear)>= 8){
				// set pin P2.3 as input
				iodir_base[2 * 0x8] &= ~(1<<3) ;
				Ippclear = NO ;
			}
			return ;
		}
		if (mADusemax){
			while( !(LPC_ADC->DR[AD_CHANNEL1] & 0x80000000) ) tickwait(10) ;
			fastAD1buf = ((unsigned int)(LPC_ADC->DR[AD_CHANNEL1] & 0xfff0)>>4) ;

// 			if (mADusemax==USE_MIN){
// 				if ( fastAD1buf<fastAD1val) fastAD1val = fastAD1buf ;
// 			}else{
				if ( fastAD1buf>fastAD1val) fastAD1val = fastAD1buf ;
//			}

			while( !(LPC_ADC->DR[AD_CHANNEL2] & 0x80000000) ) tickwait(10) ;
			fastAD2buf = ((unsigned int)(LPC_ADC->DR[AD_CHANNEL2] & 0xfff0)>>4) ;
// 			if (mADusemax==USE_MIN){
// 				if ( fastAD2buf<fastAD2val) fastAD2val = fastAD2buf ;
// 			}else{
				if ( fastAD2buf>fastAD2val) fastAD2val = fastAD2buf ;
//			}

		}else{
			fastADvals--;
			
			while( !(LPC_ADC->DR[AD_CHANNEL1] & 0x80000000) ) tickwait(10) ;
			fastAD1buf +=  (unsigned short)(LPC_ADC->DR[AD_CHANNEL1] & 0xfff0) ; // (LPC_ADC->DR[AD_CHANNEL1] >> 4) ;
			
			while( !(LPC_ADC->DR[AD_CHANNEL2] & 0x80000000) ) tickwait(10) ;
			fastAD2buf +=  (unsigned short)(LPC_ADC->DR[AD_CHANNEL2] & 0xfff0) ; // (LPC_ADC->DR[AD_CHANNEL2] >> 4) ;
			if (!fastADvals){
				fastADvals = 8 ;
				fastAD1val = (fastAD1buf>>7) ; // >>3) ;
				fastAD2val = (fastAD2buf>>7) ; // >>3) ;
				fastAD1buf = fastAD2buf = 0 ;
			}
		}
	}
//     if ( (ir) && (FastADbuffer) ) {
//         *FastADbuffer++ = (unsigned short)(LPC_ADC->DR[AD_CHANNEL]) ;
//        *FastADbuffer++ = (unsigned short)(LPC_TIM0->TC) ;
//        nt = tickmeasure(0) ;
//        *FastADbuffer++ = (unsigned short)((nt-lt)/1000ULL) ;
//        lt = nt ;
//        *FastADbuffer++ = (unsigned short)( ((SysTick->LOAD - SysTick->VAL) / (current_clock / 1000000)) ) ;
//         FastADbufsize-- ;
//         if (! FastADbufsize) {
//             // stop timer
//             LPC_TIM0->TCR = 2 ;     // reset
//             FastADbuffer = NULL ;
//         }
//    }

    LPC_TIM0->IR = ir ;         // reset interrupt
}
#endif // defined(USE_FAST_AD_T0)

//----------------------------------------------------------------------------
// ADC initializer

void adcstart(void)
{
    LPC_SC->PCONP |= ( CLKPWR_PCONP_PCADC     // enable it
#if defined(USE_FAST_AD_T0)
                     | CLKPWR_PCONP_PCTIM0
#endif // defined(USE_FAST_AD_T0)
                     ) ;

    // pins are already configured by dio.c

    LPC_ADC->CR = ( AD_ADC0_MASK << 0 ) |   // Mask of desired channels
                  ( ( PERIPHERAL_CLOCK/1250000 - 1 ) << 8 ) |   // sample at 1.25 MHz
                  ( 1 << 16 ) |             // BURST = 1, BURST ontrolled
                  ( 1 << 21 ) |             // PDN = 1, normal operation
                  ( 0 << 24 ) |             // START = 0 A/D conversion for BURST mode
                  ( 0 << 27 ) ;             // EDGE = 0 (CAP/MAT singal falling,trigger A/D conversion)

#if defined(USE_FAST_AD_T0)
	mADusemax = NO ; // default
    fastADvals = 8 ; 
    LPC_TIM0->TCR = 2 ;     // reset

    // enable interrupt from timer 0
    NVIC_EnableIRQ(TIMER0_IRQn) ;
    NVIC_SetPriority(TIMER0_IRQn, TWI_INTERRUPT_LEVEL) ;
	
	// use timer 0 interrupt
	LPC_TIM0->TCR = 2 ;     // reset
	LPC_TIM0->CTCR = 0 ;    // count at PCLK
	LPC_TIM0->MR0 = PERIPHERAL_CLOCK/AD_SAMPLE_FREQ ;   // desired frequency
	LPC_TIM0->IR  = 1 ;     // reset interrrupt
	LPC_TIM0->MCR = 3 ;     // reset TC on match with MR0 + INTERRUPT
	LPC_TIM0->TCR = 1 ;     // enable

	// set pin P2.3 as output at 0
	ioclr_base[2 * 0x8] = (1<<3) ;
	iodir_base[2 * 0x8] |= (1<<3) ;
	Ippclear = YES ;
#endif // defined(USE_FAST_AD_T0)
}

//----------------------------------------------------------------------------
// ADC stop

void adcstop(void)
{
    LPC_ADC->CR = 0 ;                         // disable

#if defined(USE_FAST_AD_T0)
    // disable use timer 0 interrupt
    LPC_TIM0->TCR = 2 ;     // reset
    NVIC_DisableIRQ(TIMER0_IRQn) ;
#endif // defined(USE_FAST_AD_T0)

    LPC_SC->PCONP &= ~(CLKPWR_PCONP_PCADC     // disable it
#if defined(USE_FAST_AD_T0)
                     | CLKPWR_PCONP_PCTIM0
#endif // defined(USE_FAST_AD_T0)
                      ) ;

    // pins will be un-configured by dio.c
}

// *********************************************************************
// AD converter handler

void ADC_readready(int chn)
{
unsigned long volatile *adc = (unsigned long *)(&(LPC_ADC->DR[0])) ;

    if ((chn >= 0) && (chn <= 5)) {
        // wait for new conversion complete
        while( !(adc[chn] & 0x80000000) )
            tickwait(10) ;
    }
}


// extern unsigned short ExtAD0[16] ; // [4] ;
// extern unsigned short ADVExt[16] ;

int ADC_read(int chn)
{
const unsigned long volatile *adc = (unsigned long *)(&(LPC_ADC->DR[0])) ;
#if defined(USE_FAST_AD_T0)
int retval ;
#endif

	//if (!chn) return((ExtAD0>>4) & 0xfff) ; // Return last stored data
#if defined(USE_FAST_AD_T0)
	if ((chn & 0x7f)==AD_CHANNEL1){	// Vpp
		retval = fastAD1val ;
		if ( (mADusemax) && (chn & AD_CLEAR) ){	// Clear
// 			if (mADusemax==USE_MIN)
// 				fastAD1val = 0x1000 ;
// 			else
				fastAD1val = 0 ;
		}
		return (retval) ;
	}else if ((chn & 0x7f)==AD_CHANNEL2){	//Ip
// 		if ((!fastAD2val) && (mADusemax==USE_MIN)){
// 			fastAD2val = 0x1000 ;
// 		}
		retval = fastAD2val ;
		if (Ippclear){
			retval = 0x1000 ;
// 			// set pin P2.3 as input
// 			iodir_base[2 * 0x8] &= ~(1<<3) ;
// 			Ippclear = NO ;
		}else{
			if (chn & AD_CLEAR){
				// set pin P2.3 as output at 0
				ioclr_base[2 * 0x8] = (1<<3) ;
				iodir_base[2 * 0x8] |= (1<<3) ;
				Ippclear = YES ;
			}
		}
		if ( (mADusemax) && (chn & AD_CLEAR) ){	// Clear
// 			if (mADusemax==USE_MIN)
// 				fastAD2val = 0x1000 ;
// 			else
				fastAD2val = 0 ;
		}
		return (retval) ;
	}else
#endif
    if ((chn >= 0) && (chn <= 9)) {
        return((adc[chn]>>4) & 0xfff) ;
    }

    return(0xfff) ; // old default 0
}


#if defined(USE_FAST_AD_T0_)
//----------------------------------------------------------------------------
// Start/Stop Fast A/D with pacer Timer 0

void FastAD(unsigned short *buffer, int bufsize)
{
    if (buffer) {

        FastADbuffer = buffer ;
        FastADbufsize = bufsize ;   // in short

        // use timer 0 interrupt
        LPC_TIM0->TCR = 2 ;     // reset
        LPC_TIM0->CTCR = 0 ;    // count at PCLK
        LPC_TIM0->MR0 = PERIPHERAL_CLOCK/AD_SAMPLE_FREQ ;   // desired frequency
        LPC_TIM0->IR  = 1 ;     // reset interrrupt
        LPC_TIM0->MCR = 3 ;     // reset TC on match with MR0 + INTERRUPT
        LPC_TIM0->TCR = 1 ;     // enable

    } else {    // disable

        // disable use timer 0 interrupt
        LPC_TIM0->TCR = 2 ;     // reset

        FastADbuffer = NULL ;
    }
}

int FastADremaining(void)
{
    return(FastADbuffer ? FastADbufsize : 0) ;
}
#endif // defined(USE_FAST_AD_T0)
