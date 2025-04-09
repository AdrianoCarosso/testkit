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

#ifdef USE_SPI_FAST_ACCELEROMETER
unsigned char * spifastbuffer ;     // buffer pointer
int spifastbufsize ;                // buffer size
unsigned char * spifastOUTptr ;     // user buffer
int spifastnum ;                    // elements in user buffer
#endif // USE_SPI_FAST_ACCELEROMETER

#define USE_SPI_POLLING

#undef  DEBUG_SPI

// Chip select
#define SPI0_CHIPSELECT_0   (1<<23)     // port 2

#ifdef USE_DOUBLE_SPI
#define SPI1_CHIPSELECT_0   (1<<6)      // port 0
#endif // USE_DOUBLE_SPI

#ifdef CBUG
unsigned short spi0tocount = 0 ;        // error counter
#ifdef USE_DOUBLE_SPI
unsigned short spi1tocount = 0 ;        // error counter
#endif // USE_DOUBLE_SPI
#endif // CBUG

unsigned short spi0err = 0 ;
#ifdef USE_DOUBLE_SPI
unsigned short spi1err = 0 ;
#endif // USE_DOUBLE_SPI

#ifndef USE_SPI_POLLING
volatile unsigned char *globspi0_buftx ;
volatile unsigned char *globspi0_bufrx ;
volatile int globspi0_lentx ;
volatile int globspi0_lenrx ;
#ifdef USE_DOUBLE_SPI
volatile unsigned char *globspi1_buftx ;
volatile unsigned char *globspi1_bufrx ;
volatile int globspi1_lentx ;
volatile int globspi1_lenrx ;
#endif // USE_DOUBLE_SPI
#endif // USE_SPI_POLLING

#define NULLSEMA ((SEMA)0)

//----------------------------------------------------------------------------
// internal functions

void spistart(void) ;
void spistop(void) ;
void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;
#ifdef USE_DOUBLE_SPI
void SPI1_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
               unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;
#endif // USE_DOUBLE_SPI

#ifndef USE_SPI_POLLING
//----------------------------------------------------------------------------
// Interrupt routine for SPI0

void SSP0_IRQHandler(void)
{
    unsigned char intflag ;
    int i ;

    intflag = LPC_SSP0->MIS ;       // let's analyze

    if (intflag & 1) {              // rx overrun
        spi0err++ ;
        LPC_SSP0->ICR = 1 ;         // reset this
    }

    /*if (intflag & 6)*/ {  //len1 & 0x80000000) rx (fifo or timeout)
        while((globspi0_lenrx) && (LPC_SSP0->SR & 0x04)) {   // while data to rx
            *globspi0_bufrx++ = LPC_SSP0->DR ;
            globspi0_lenrx-- ;
        }
        if (intflag & 2) {      // rx timeout
            LPC_SSP0->ICR = 2 ; // reset this
        }

        if (globspi0_lenrx==0) { // disable RX interrupts
            LPC_SSP0->IMSC &= ~0x7 ;
        }
    }

    for(i=0 ; (i<6) && (globspi0_lentx) && (LPC_SSP0->SR & 0x02) ; i++) {    // while data to tx
        LPC_SSP0->DR = *globspi0_buftx++ ;
        globspi0_lentx-- ;
    }
    if (globspi0_lentx==0) {     // disable TX interrupts
        LPC_SSP0->IMSC &= ~0x8 ;
    }

    // check if end of activity
    if ((globspi0_lenrx==0) && (globspi0_lentx==0)) {
        LPC_SSP0->IMSC &= ~0xf ;    // disable all interrupts
        KS_ISRsignal(SPISEM) ;
        ASK_CONTEXTSWITCH ;     // set PendSV
    }
}

#endif // USE_SPI_POLLING

#ifdef USE_SPI_FAST_ACCELEROMETER
//----------------------------------------------------------------------------
// Interrupt routine for Timer3
void TIMER3_IRQHandler(void)
{
    register int i ;
    volatile int dummy __attribute__ ((unused)) ;
    
    int ir = LPC_TIM3->IR ;         // don't bother us

    // CS --\__
    LPC_GPIO2->CLR = SPI0_CHIPSELECT_0 ;    // CS at '0'

    // read / write loop
    for(i=0 ; i<7 ; i++) {
        // wait for tx
        while(!(LPC_SSP0->SR & 0x01)) ;

        // tx
        LPC_SSP0->DR = (i==0) ? (0x28 | 0xc0) : 0 ;

        // wait for rx
        while(!(LPC_SSP0->SR & 0x04)) ;
        //printf("-4- SSP0->SR=0x%08lx\n", SSP0->SR) ;

        // rx
        if (i && spifastbuffer) {
            *spibufptr++ = LPC_SSP0->DR ;
        } else {
            dummy = LPC_SSP0->DR ;
        }
    }

    // CS __/--
    LPC_GPIO2->SET = SPI0_CHIPSELECT_0 ;    // CS at '1'
    
    // another tripleaxis available
    if (spifastbuffer) {    // fast mode enabled
        spifastnum++ ;      // another tripleaxis
        if (spibufptr >= &spifastbuffer[spifastbufsize])
            spibufptr = spifastbuffer ;    // re-init producer ptr
    }
            
    LPC_TIM3->IR  = ir ;                // reset interrrupt
}
#endif // USE_SPI_FAST_ACCELEROMETER

//----------------------------------------------------------------------------
// SPI initializer

void spistart(void)
{
    int clk ;

	spi0err++ ;		// If a restart

	LPC_SC->PCONP |= (CLKPWR_PCONP_PCSSP0          // enable it
#ifdef USE_DOUBLE_SPI
                    | CLKPWR_PCONP_PCSSP1
#endif // USE_DOUBLE_SPI
                    ) ;

    // pins are already configured by dio.c

    // MASTER MODE, CPOL=0, CPHA=0, 8 bit, MSB first
    LPC_SSP0->CR0 = 0x00000007 ;
    clk = PERIPHERAL_CLOCK / 20000000 ; // target clock = 20 MHz
    if (clk < 2) clk = 2 ;              // min value
    if (clk & 1) clk++ ;                // must be EVEN
    LPC_SSP0->CPSR = clk ;              // real speed is (if current_clock = 120MHz): 120MHz / 6 -> 20 MHz
    LPC_SSP0->CR1 = 0x00000002 ;        // enable

#ifndef USE_SPI_POLLING
    NVIC_EnableIRQ(SSP0_IRQn) ;
    NVIC_SetPriority(SSP0_IRQn, SPI_INTERRUPT_LEVEL) ;
#endif // USE_SPI_POLLING

#ifdef USE_DOUBLE_SPI
    // MASTER MODE, CPOL=0, CPHA=0, 8 bit, MSB first
    LPC_SSP1->CR0 = 0x00000007 ;
    clk = PERIPHERAL_CLOCK / 20000000 ; // target clock = 20 MHz
    if (clk < 2) clk = 2 ;              // min value
    if (clk & 1) clk++ ;                // must be EVEN
    LPC_SSP1->CPSR = clk ;              // real speed is (if current_clock = 120MHz): 120MHz / 6 -> 20 MHz
    LPC_SSP1->CR1 = 0x00000002 ;        // enable

#ifndef USE_SPI_POLLING
    NVIC_EnableIRQ(SSP1_IRQn) ;
    NVIC_SetPriority(SSP1_IRQn, SPI_INTERRUPT_LEVEL) ;
#endif // USE_SPI_POLLING
#endif // USE_DOUBLE_SPI
    
#ifdef USE_SPI_FAST_ACCELEROMETER
    // in case of fast accelerometer, use timer 3 interrupt
    LPC_SC->PCONP |= PCONP_PCTIM3 ;
    LPC_TIM3->TCR = 2 ;         // reset

    spifastbufsize = 0 ;    
    spifastbuffer = NULL ;
    spifastnum = 0 ;

    // enable interrupt from timer 3
    NVIC_EnableIRQ(TIMER3_IRQn) ;
    NVIC_SetPriority(TIMER3_IRQn, SPI_INTERRUPT_LEVEL) ;
#endif // USE_SPI_FAST_ACCELEROMETER    
}

//----------------------------------------------------------------------------
// SPI terminator

void spistop(void)
{
#ifndef USE_SPI_POLLING
    NVIC_DisableIRQ(SSP0_IRQn) ;        // disable interrupt
#ifdef USE_DOUBLE_SPI
    NVIC_DisableIRQ(SSP1_IRQn) ;        // disable interrupt
#endif // USE_DOUBLE_SPI
#endif // USE_SPI_POLLING

    // pins will be un-configured by dio.c

    LPC_SC->PCONP &= (~(CLKPWR_PCONP_PCSSP0          // enable it
#ifdef USE_DOUBLE_SPI
                      | CLKPWR_PCONP_PCSSP1
#endif // USE_DOUBLE_SPI
                    )) ;
                    
#ifdef USE_SPI_FAST_ACCELEROMETER
    // disable interrupt from timer 3
    NVIC_DisableIRQ(TIMER3_IRQn) ;

    // disable use timer 3 interrupt
    LPC_TIM3->TCR = 2 ;     // reset
    LPC_SC->PCONP &= ~PCONP_PCTIM3 ;
#endif // USE_SPI_FAST_ACCELEROMETER
}

//----------------------------------------------------------------------------
// SPI commnunicator
// Double buffer with PDC and Interrupt

void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              unsigned char *buf2tx, unsigned char *buf2rx, int len2)
{
#ifndef USE_SPI_POLLING
    register int waitval ;      // returned value
#endif // USE_SPI_POLLING

#ifdef USE_SPI_FAST_ACCELEROMETER
    if (spifastbuffer) return ; // no data -- busy for fast
#endif // USE_SPI_FAST_ACCELEROMETER

    for( ; ; ) {
        // CS --\__
        LPC_GPIO2->CLR = SPI0_CHIPSELECT_0 ;    // CS at '0'

#ifndef USE_SPI_POLLING

        if (len1) {
            globspi0_buftx = buf1tx ;
            globspi0_bufrx = buf1rx ;
            globspi0_lentx = len1 ;
            globspi0_lenrx = len1 ;
            LPC_SSP0->IMSC |= 0xf ;     // enable all interrupts

            // wait with timeout
            waitval = KS_waitt(SPISEM, ((TICKS)2000/CLKTICK)) ;

			if (waitval != RC_GOOD) spi0err++ ;

            if (waitval == RC_TIMEOUT) {
#ifdef CBUG
                pdebugt(1, "SSP0a ERR=%d, TO=%d, SR=0x%lx, L:%d, TX:%d, RX:%d", spi0err, ++spi0tocount,
                                SSP0->SR, len1, globspi0_lentx, globspi0_lenrx) ;
#endif // CBUG
                // CS __/--
                LPC_GPIO2->SET = SPI0_CHIPSELECT_0 ;    // CS at '1'
                KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;         // skip time
                continue ;
            }
        }

        if (len2) {
            globspi0_buftx = buf2tx ;
            globspi0_bufrx = buf2rx ;
            globspi0_lentx = len2 ;
            globspi0_lenrx = len2 ;
            LPC_SSP0->IMSC |= 0xf ;     // enable all interrupts

            // wait with timeout
            waitval = KS_waitt(SPISEM, ((TICKS)2000/CLKTICK)) ;

			if (waitval != RC_GOOD) spi0err++ ;

            if (waitval == RC_TIMEOUT) {
#ifdef CBUG
                pdebugt(1, "SSP0b ERR=%d, TO=%d, SR=0x%lx, L:%d, TX:%d, RX:%d", spi0err, ++spi0tocount,
                                SSP0->SR, len2, globspi_lentx, globspi0_lenrx) ;
#endif // CBUG
                // CS __/--
                LPC_GPIO2->SET = SPI0_CHIPSELECT_0 ;    // CS at '1'
                KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ; // skip time
                continue ;
            }
        }

#else // USE_SPI_POLLING

        // step -1-
        while(len1--) {
            //printf("-1- SSP0->SR=0x%08lx\n", SSP0->SR) ;
            // wait for tx
            while(!(LPC_SSP0->SR & 0x01)) ;
            //printf("-2- SSP0->SR=0x%08lx\n", SSP0->SR) ;

            // tx
            LPC_SSP0->DR = *buf1tx++ ;

            //printf("-3- SSP0->SR=0x%08lx\n", SSP0->SR) ;
            // wait for rx
            while(!(LPC_SSP0->SR & 0x04)) ;
            //printf("-4- SSP0->SR=0x%08lx\n", SSP0->SR) ;

            // rx
            *buf1rx++ = LPC_SSP0->DR ;
        }

        // step -2-
        while(len2--) {
            // wait for tx
            while(!(LPC_SSP0->SR & 0x01)) ;

            // tx
            LPC_SSP0->DR = *buf2tx++ ;

            // wait for rx
            while(!(LPC_SSP0->SR & 0x04)) ;

            // rx
            *buf2rx++ = LPC_SSP0->DR ;
        }

#endif // USE_SPI_POLLING

        // CS __/--
        LPC_GPIO2->SET = SPI0_CHIPSELECT_0 ;    // CS at '1'

        break ;
    }
}

#ifdef USE_DOUBLE_SPI
//----------------------------------------------------------------------------
// SPI commnunicator
// Double buffer with PDC and Interrupt

void SPI1_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              unsigned char *buf2tx, unsigned char *buf2rx, int len2)
{
#error "SPI1 TODO"
}
#endif // USE_DOUBLE_SPI

#ifdef USE_SPI_FAST_ACCELEROMETER
//----------------------------------------------------------------------------
// SPI RX with interrupt

void SPI_setfast(unsigned char *buffer, int bufsize)
{
    if (buffer) {
        if (bufsize % 6) return ;       // must be multiple of tripleaxis size

        spifastbuffer = buffer ;
        spibufptr     = buffer ;    
        spifastbufsize = bufsize ;

        spifastOUTptr = spifastbuffer ; // init consumer ptr
        spifastnum = 0 ;

        // in case of fast accelerometer, use timer 3 interrupt
        LPC_TIM3->TCR = 2 ;     // reset
        LPC_TIM3->CTCR = 0 ;    // count at PCLK
        LPC_TIM3->MR0 = current_clock/4/ACC_SAMPLE_FREQ ;   // desired frequency
        LPC_TIM3->MCR = 3 ;     // reset TC on match with MR0 + INTERRUPT
        LPC_TIM3->IR  = 0xff;   // reset all interrrupts
        LPC_TIM3->TCR = 1 ;     // enable

    } else {    // disable

        spifastbuffer = NULL ;

        // disable use timer 3 interrupt
        LPC_TIM3->TCR = 2 ;     // reset
    }
}

int SPI_get3axisfast(short *ax, short *ay, short *az)
{
    if (!spifastnum) return(1) ;    // no data

    *ax = (spifastOUTptr[0]<<8) + spifastOUTptr[1] ;
    *ay = (spifastOUTptr[2]<<8) + spifastOUTptr[3] ;
    *az = (spifastOUTptr[4]<<8) + spifastOUTptr[5] ;
    spifastOUTptr += 6 ;
    if (spifastOUTptr >= &spifastbuffer[spifastbufsize])
        spifastOUTptr = spifastbuffer ;   // re-init consumer ptr

    __disable_irq() ;   // critical region
    spifastnum-- ;
    __enable_irq() ;

    return(0) ;     // we have data
}
#endif // USE_SPI_FAST_ACCELEROMETER

#endif // USE_SPI_ON_ARM

