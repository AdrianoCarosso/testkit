// drv_spi.c - SPI driver tasks

//
//   Copyright (c) 1997-2011.
//   T.E.S.T. srl
//

//
// This module is provided as a SPI port driver.
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
#ifdef USE_SPI_ON_ARM

#define USE_SPI_POLLING

// Chip select
#define SPI_CHIPSELECT_0    0x00010000
#ifdef USE_DOUBLE_SPI
#define SPI_CHIPSELECT_1    0x00000080  
#else // only -0-
#define SPI_CHIPSELECT_1    0x00000000
#endif // USE_DOUBLE_SPI

#ifdef CBUG
unsigned short spi0tocount = 0 ;        // error counter
#endif // CBUG
unsigned short spi0err = 0 ;

volatile unsigned char *globspi_buftx ;
volatile unsigned char *globspi_bufrx ;
volatile int globspi_lentx ;
volatile int globspi_lenrx ;

#undef DEBUG_SPI

#define NULLSEMA ((SEMA)0)

//----------------------------------------------------------------------------
// internal functions

void spistart(void) ;
void spistop(void) ;
void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;

//----------------------------------------------------------------------------
// Interrupt routine for SPI

void SSP0_IRQHandler(void)
{
    unsigned char intflag ;
    int i ;
    
    intflag = SSP0->MIS ;       // let's analyze
    
    if (intflag & 1) {  // rx overrun
        spi0err++ ;
        SSP0->ICR = 1 ; // reset this
    }

    /*if (intflag & 6)*/ {  //len1 & 0x80000000) rx (fifo or timeout)
        while((globspi_lenrx) && (SSP0->SR & 0x04)) {   // while data to rx
            *globspi_bufrx++ = SSP0->DR ;
            globspi_lenrx-- ;
        }
        if (intflag & 2) {      // rx timeout
            SSP0->ICR = 2 ;     // reset this
        }

        if (globspi_lenrx==0) { // disable RX interrupts
            SSP0->IMSC &= ~0x7 ;
        }
    }

    for(i=0 ; (i<6) && (globspi_lentx) && (SSP0->SR & 0x02) ; i++) {    // while data to tx
        SSP0->DR = *globspi_buftx++ ;
        globspi_lentx-- ;
    }
    if (globspi_lentx==0) {     // disable TX interrupts
        SSP0->IMSC &= ~0x8 ;
    }

    // check if end of activity
    if ((globspi_lenrx==0) && (globspi_lentx==0)) {
        SSP0->IMSC &= ~0xf ;    // disable all interrupts
        KS_ISRsignal(SPISEM) ;
        ASK_CONTEXTSWITCH ;     // set PendSV
    }
}

//----------------------------------------------------------------------------
// SPI initializer

void spistart(void)
{
    int clk ;
    
	spi0err++ ;		// If a restart

	SC->PCONP |= PCONP_PCSSP0 ;         // enable it

    // P0.15 = CLK
    // P0.16 = CS0, P0.7 = CS1 (when enabled)
    // P0.17 = MISO
    // P0.18 = MOSI
    PINCON->PINSEL0 |= 0x80000000 ;     // bit31,30 - P0.15
    PINCON->PINSEL0 &= ~0x40000000 ;    // bit31,30 - P0.15

    PINCON->PINSEL1 |= 0x00000028 ;     // CS handled as GPIO
    PINCON->PINSEL1 &= ~0x00000014 ;    // CS handled as GPIO

    GPIO0->FIOSET = (SPI_CHIPSELECT_0 | SPI_CHIPSELECT_1) ;     // at '1'
    GPIO0->FIODIR |= (SPI_CHIPSELECT_0 | SPI_CHIPSELECT_1) ;    // CS is output

    SC->PCLKSEL1 |= 0x00000400 ;        // bit11,10: 01 -> PCLK == CCLK
    SC->PCLKSEL1 &= ~0x00000800 ;       // bit11,10: 01 -> PCLK == CCLK
    
    // MASTER MODE, CPOL=0, CPHA=0, 8 bit, MSB first
    SSP0->CR0 = 0x00000007 ;
    clk = current_clock / 25000000 ;    // target clock = 25 MHz
    if (clk < 2) clk = 2 ;              // min value
    if (clk & 1) clk++ ;                // must be EVEN
    SSP0->CPSR = clk ;          // real speed is (if current_clock = 100MHz): 100MHz / 4 -> 25 MHz
    SSP0->CR1 = 0x00000002 ;    // enable

    NVIC_EnableIRQ(SSP0_IRQn) ;
    NVIC_SetPriority(SSP0_IRQn, SPI_INTERRUPT_LEVEL) ;
}

//----------------------------------------------------------------------------
// SPI terminator

void spistop(void)
{
    NVIC_DisableIRQ(SSP0_IRQn) ;        // disable interrupt

//    GPIO0->FIODIR &= ~0x00078000 ;      // all inputs
//
//    PINCON->PINSEL0 &= ~0xc0000000 ;    // bit31,30 - P0.15
//    PINCON->PINSEL1 &= ~0x0000003f ;    // CS handled as GPIO

    GPIO0->FIODIR |= 0x00078000 ;       // all outputs
    GPIO0->FIOSET  = 0x00078000 ;       // all outputs at '1'
    PINCON->PINSEL0 &= ~0xc0000000 ;    // bit31,30 - P0.15
    PINCON->PINSEL1 &= ~0x0000003f ;    // CS handled as GPIO

    SC->PCONP &= (~PCONP_PCSSP0) ;      // disable it
}

//----------------------------------------------------------------------------
// SPI commnunicator
// Double buffer with PDC and Interrupt

void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              unsigned char *buf2tx, unsigned char *buf2rx, int len2)
{
    register int csmask ;
#ifndef USE_SPI_POLLING
    register int waitval ;      // returned value
#endif // USE_SPI_POLLING

#ifdef USE_DOUBLE_SPI
    csmask = (len1 & 0x80000000) ? 0 : SPI_CHIPSELECT_0 ;    // real CS ?
    len1 &= (~ 0xf0000000) ;
#else // USE_DOUBLE_SPI
    csmask = SPI_CHIPSELECT_0 ;    // CS at '0'
#endif // USE_DOUBLE_SPI

    for( ; ; ) {
        // CS --\__
        GPIO0->FIOCLR = csmask ;    // CS at '0'

#ifndef USE_SPI_POLLING

        if (len1) {
            globspi_buftx = buf1tx ;
            globspi_bufrx = buf1rx ;
            globspi_lentx = len1 ;
            globspi_lenrx = len1 ;
            SSP0->IMSC |= 0xf ;         // enable all interrupts

            // wait with timeout
            waitval = KS_waitt(SPISEM, ((TICKS)2000/CLKTICK)) ;
			
			if (waitval != RC_GOOD) spi0err++ ;
		
            if (waitval == RC_TIMEOUT) {
#ifdef CBUG
                pdebugt(1, "SSP0a ERR=%d, TO=%d, SR=0x%lx, L:%d, TX:%d, RX:%d", spi0err, ++spi0tocount,
                                SSP0->SR, len1, globspi_lentx, globspi_lenrx) ;
#endif // CBUG
                // CS __/--
                GPIO0->FIOSET = csmask ;    // CS at '1'
                KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;         // skip time
                continue ;
            }
        }

        if (len2) {
            globspi_buftx = buf2tx ;
            globspi_bufrx = buf2rx ;
            globspi_lentx = len2 ;
            globspi_lenrx = len2 ;
            SSP0->IMSC |= 0xf ;         // enable all interrupts

            // wait with timeout
            waitval = KS_waitt(SPISEM, ((TICKS)2000/CLKTICK)) ;
			
			if (waitval != RC_GOOD) spi0err++ ;
		
            if (waitval == RC_TIMEOUT) {
#ifdef CBUG
                pdebugt(1, "SSP0b ERR=%d, TO=%d, SR=0x%lx, L:%d, TX:%d, RX:%d", spi0err, ++spi0tocount,
                                SSP0->SR, len2, globspi_lentx, globspi_lenrx) ;
#endif // CBUG
                // CS __/--
                GPIO0->FIOSET = csmask ;    // CS at '1'
                KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ; // skip time
                continue ;
            }
        }

#else // USE_SPI_POLLING

        // step -1-
        while(len1--) {
            //printf("-1- SSP0->SR=0x%08lx\n", SSP0->SR) ;
            // wait for tx
            while(!(SSP0->SR & 0x01)) ;
            //printf("-2- SSP0->SR=0x%08lx\n", SSP0->SR) ;

            // tx
            SSP0->DR = *buf1tx++ ;

            //printf("-3- SSP0->SR=0x%08lx\n", SSP0->SR) ;
            // wait for rx
            while(!(SSP0->SR & 0x04)) ;
            //printf("-4- SSP0->SR=0x%08lx\n", SSP0->SR) ;

            // rx
            *buf1rx++ = SSP0->DR ;
        }

        // step -2-
        while(len2--) {
            // wait for tx
            while(!(SSP0->SR & 0x01)) ;

            // tx
            SSP0->DR = *buf2tx++ ;

            // wait for rx
            while(!(SSP0->SR & 0x04)) ;

            // rx
            *buf2rx++ = SSP0->DR ;
        }

#endif // USE_SPI_POLLING

        // CS __/--
        GPIO0->FIOSET = csmask ;    // CS at '1'

        break ;
    }
}

#endif // USE_SPI_ON_ARM

