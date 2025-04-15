// drv_spi.c - SPI driver tasks

//
//   Copyright (c) 1997-2010.
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

#define NULLSEMA ((SEMA)0)

unsigned char spi0err ;        // error counter
unsigned char spi0start ;
#undef DEBUG_SPI

//----------------------------------------------------------------------------
// internal functions

void spistart(void) ;
void spistop(void) ;
void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;

//----------------------------------------------------------------------------
// Interrupt routine for SPI

void SPI_IrqHandler(void)
{
    // done ?
    if (SPI->SPI_SR & (SPI_SR_TXBUFE | SPI_SR_RXBUFF)) {
        // Disable the interrupt source
        SPI->SPI_IDR = (SPI_IDR_TXBUFE | SPI_IDR_RXBUFF) ;
        // Disable DMA
        SPI->SPI_PTCR = SPI_PTCR_TXTDIS | SPI_PTCR_RXTDIS ;

        KS_ISRsignal(SPISEM) ;
        ASK_CONTEXTSWITCH ;     // set PendSV
    }
}

//----------------------------------------------------------------------------
// SPI initializer

void spistart(void)
{
    volatile unsigned char buf ;

	spi0err++ ;
	spi0start++ ;
	
    // PIO A: Peripheral A select register: 00
    PIOA->PIO_ABCDSR[0] &= (~(unsigned long)(PIO_PA12A_MISO | PIO_PA13A_MOSI | PIO_PA14A_SPCK)) ;
    PIOA->PIO_ABCDSR[1] &= (~(unsigned long)(PIO_PA12A_MISO | PIO_PA13A_MOSI | PIO_PA14A_SPCK)) ;
    // PIO A: disable register
    PIOA->PIO_PDR = (unsigned long)(PIO_PA12A_MISO | PIO_PA13A_MOSI | PIO_PA14A_SPCK) ;

    // configure the PIO Lines corresponding to PIO_PA11A_NPCS0 to be outputs
    // no need to set these pins to be driven by the PIO because it is GPIO pins only.
    // set at 1
    PIOA->PIO_SODR = PIO_PA11A_NPCS0 ;
    // Set in PIO mode
    PIOA->PIO_PER  = PIO_PA11A_NPCS0 ;
    // Configure in Output
    PIOA->PIO_OER  = PIO_PA11A_NPCS0 ;

    // Peripheral Clock Enable Register
    PMC->PMC_PCER0 = (1 << ID_SPI) ;

    // disable
    SPI->SPI_CR = SPI_CR_SPIDIS;
    // Execute a software reset of the SPI twice
    SPI->SPI_CR = SPI_CR_SWRST;
    SPI->SPI_CR = SPI_CR_SWRST;

    // AT45DB321C timings: up to 33MHz
    //  NOUSB: 60MHz -> T=16.7ns ; SPICLK = 60MHz / 2 = 30MHz
    //  USB:   48MHz -> T=20.8ns ; SPICLK = 48MHz / 2 = 24MHz
    //         30MHz -> T=33.3ns ; SPICLK = 30MHz / 1 = 30MHz
    //         16MHz -> T=62.5ns ; SPICLK = 16MHz / 1 = 16MHz
    //          8MHz -> T=125 ns ; SPICLK =  8MHz / 1 =  8MHz
#define SPI_SCBR        ((current_clock/33000000)+1)

    // DLYBCS (delay between CS) = Tcs = 250 ns
    //  NOUSB: 60MHZ --> 250 = 16.7ns * 16
    //  USB:   48MHZ --> 250 = 20.8ns * 13
    //         30MHz --> 250 = 33.3ns * 8
    //         16MHz --> 250 = 62.5ns * 5 -> 6
    //          8MHz --> 250 = 125 ns * 3 -> 6
#define SPI_DLYBCS      (MAX(1+((250*(current_clock/1000000))/1000),6))

    // DLYBS (delay from CS to CLK) = Tcss = 100 ns
    //  NOUSB: 60MHZ --> 117 = 16.7ns * 7
    //  USB:   48MHZ --> 104 = 20.8ns * 5
    //         16MHz --> 125 = 62.5ns * 2 -> 3
    //          8MHz --> 125 = 125 ns * 1 -> 3
#define SPI_DLYBS       (MAX(1+((100*(current_clock/1000000))/1000),3))

    // DLYBCT (delay between bytes) = 0 ns = 0
#define SPI_DLYBCT      ((0*(current_clock/1000000))/1000)

    // mode: master, DLYBCS, FixPeriphSelect=0
    SPI->SPI_MR = SPI_MR_MSTR | SPI_MR_MODFDIS /*| (7<<16) */| (SPI_DLYBCS<<24) ;

    SPI->SPI_CSR[0] = SPI_CSR_NCPHA  |
                      SPI_CSR_BITS_8_BIT |
                                (SPI_SCBR << 8)   |     // SCBR
                                (SPI_DLYBS << 16) |     // DLYBS
                                (SPI_DLYBCT << 24) ;    // DLYBCT

    NVIC_EnableIRQ(SPI_IRQn) ;
    NVIC_SetPriority(SPI_IRQn, SPI_INTERRUPT_LEVEL) ;

    // enable
    SPI->SPI_CR = SPI_CR_SPIEN ;

    // first activity 'by hand', ask dummy SR

    // NPCS0 --\__
    PIOA->PIO_CODR = PIO_PA11A_NPCS0 ;

    // tx
    SPI->SPI_TDR = 0xd7 ;       // SR request

    // wait for rx
    while(!(SPI->SPI_SR & SPI_SR_RDRF))
        ;

    // rx
    buf = SPI->SPI_RDR ;

    // tx
    SPI->SPI_TDR = 0 ;          // dummy

    // wait for rx
    while(!(SPI->SPI_SR & SPI_SR_RDRF))
        ;

    // rx
    buf = SPI->SPI_RDR ;

    // NPCS0 __/--
    PIOA->PIO_SODR = PIO_PA11A_NPCS0 ;
}

//----------------------------------------------------------------------------
// SPI commnunicator
// Double buffer with PDC and Interrupt

void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              unsigned char *buf2tx, unsigned char *buf2rx, int len2)
{
    register int waitval ;      // returned value
#ifdef CBUG
    static int tocount = 0 ;
#endif // CBUG

#ifdef CBUG
#define CHECK_ADDR(A) {if ((A) && ((int)(A)>0x400000) && ((int)(A)<0x440000)) pdebugt(1,"SPI bad address 0x%x", (int)(A));}
    CHECK_ADDR(buf1tx) ;
    CHECK_ADDR(buf1rx) ;
    CHECK_ADDR(buf2tx) ;
    CHECK_ADDR(buf2rx) ;
#endif // CBUG

    for( ; ; ) {
        // NPCS0 --\__
        PIOA->PIO_CODR = PIO_PA11A_NPCS0 ;

        SPI->SPI_TPR = (unsigned long)(buf1tx) ;    // TX buffer 1 ptr
        SPI->SPI_TCR = len1 ;                       // TX buffer 1 len
        SPI->SPI_TNPR = (unsigned long)(buf2tx) ;   // TX buffer 2 ptr
        SPI->SPI_TNCR = len2 ;                      // TX buffer 2 len

        SPI->SPI_RPR = (unsigned long)(buf1rx) ;    // RX buffer 1 ptr
        SPI->SPI_RCR = len1 ;                       // RX buffer 1 len
        SPI->SPI_RNPR = (unsigned long)(buf2rx) ;   // RX buffer 2 ptr
        SPI->SPI_RNCR = len2 ;                      // RX buffer 2 len

        DISABLE ;               // critical region begin
        SPI->SPI_IER = SPI_IER_RXBUFF ;                     // enable interrupt
        SPI->SPI_PTCR = SPI_PTCR_TXTEN | SPI_PTCR_RXTEN ;   // enable DMA
        ENABLE ;                // critical region end

        // wait end with timeout
        waitval = KS_waitt(SPISEM, ((TICKS)2000/CLKTICK)) ;
        
        // NPCS0 __/--
        PIOA->PIO_SODR = PIO_PA11A_NPCS0 ;

		if (waitval != RC_GOOD) spi0err++ ;
		
        if (waitval == RC_TIMEOUT) {
#ifdef CBUG
            pdebugt(1,"SPI TO=%d, SPI_SR:0x%lx, SPI_PTSR:0x%lx, SPI_TCR:0x%lx, SPI_RCR 0x%lx",
                   ++tocount,
                   SPI->SPI_SR, SPI->SPI_PTSR,
                   SPI->SPI_TCR, SPI->SPI_RCR) ;
#endif // CBUG
        } else {
            break ;
        }
    }
}

//----------------------------------------------------------------------------
// SPI terminator

void spistop(void)
{
    NVIC_DisableIRQ(SPI_IRQn) ;     // disable interrupt

    // Disable
    SPI->SPI_CR = SPI_CR_SPIDIS ;

    // Disable clock
    PMC->PMC_PCDR0 = (1 << (ID_SPI)) ;

    // set at 1
    PIOA->PIO_SODR = (unsigned long)(PIO_PA12A_MISO | PIO_PA13A_MOSI | 
                                     PIO_PA14A_SPCK | PIO_PA11A_NPCS0) ;
    // Set in PIO mode
    PIOA->PIO_PER  = (unsigned long)(PIO_PA12A_MISO | PIO_PA13A_MOSI | 
                                     PIO_PA14A_SPCK | PIO_PA11A_NPCS0) ;
    // Configure in Output
    PIOA->PIO_OER  = (unsigned long)(PIO_PA12A_MISO | PIO_PA13A_MOSI | 
                                     PIO_PA14A_SPCK | PIO_PA11A_NPCS0) ;
	spi0err = 0 ; // Reset errors
	spi0start = 0 ;
}
#endif // USE_SPI_ON_ARM
// end of file - drv_spi.c

