// drv_spi.c - SPI driver tasks

//
//   Copyright (c) 1997-2009.
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
// functions in drv_clk.c

extern void AT91F_AIC_Configure(int irq_id, int priority, int src_type, FRAME *(*newHandler)(FRAME *frame)) ;

//----------------------------------------------------------------------------
// internal functions

void spistart(void) ;
void spistop(void) ;
void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;

FRAME *spidrv(FRAME * frame) ;

//----------------------------------------------------------------------------
// Interrupt routine for SPI

FRAME *spidrv(FRAME * frame)
{
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI0 ;
#endif // USE_REAL_BOARD

#ifdef USE_EVALUATION_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI1 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91PS_SPI const pSPI = AT91C_BASE_SPI ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    // done ?
    if (pSPI->SPI_SR & (AT91C_SPI_TXBUFE | AT91C_SPI_RXBUFF)) {
        // Disable the interrupt source
        pSPI->SPI_IDR = (AT91C_SPI_TXBUFE | AT91C_SPI_RXBUFF) ;
        // Disable DMA
        pSPI->SPI_PTCR = AT91C_PDC_TXTDIS | AT91C_PDC_RXTDIS ;
        
        return(KS_ISRexit(frame, SPISEM)) ;
    } else {
        return(KS_ISRexit(frame, NULLSEMA)) ;
    }
}

//----------------------------------------------------------------------------
// SPI initializer

void spistart(void)
{
    volatile unsigned char buf ;

	spi0err++ ;
	spi0start++ ;
	
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI0 ;
#endif // USE_REAL_BOARD

#ifdef USE_EVALUATION_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI1 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91PS_SPI const pSPI = AT91C_BASE_SPI ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    // PIO A: Peripheral A select register
    AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA15_SPI0_MISO  | AT91C_PA16_SPI0_MOSI  |
                                               AT91C_PA17_SPI0_SPCK) ;
    // PIO A: disable register
    AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA15_SPI0_MISO  | AT91C_PA16_SPI0_MOSI  |
                                               AT91C_PA17_SPI0_SPCK) ;

    // configure the PIO Lines corresponding to AT91C_PA11_SPI0_NPCS0 to be outputs
    // no need to set these pins to be driven by the PIO because it is GPIO pins only.
    // set at 1
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_SPI0_NPCS0 | AT91C_PA12_SPI0_NPCS1 |
                                AT91C_PA13_SPI0_NPCS2 | AT91C_PA14_SPI0_NPCS3 ;
    // Set in PIO mode
    AT91C_BASE_PIOA->PIO_PER  = AT91C_PA11_SPI0_NPCS0 | AT91C_PA12_SPI0_NPCS1 |
                                AT91C_PA13_SPI0_NPCS2 | AT91C_PA14_SPI0_NPCS3 ;
    // Configure in Output
    AT91C_BASE_PIOA->PIO_OER  = AT91C_PA11_SPI0_NPCS0 | AT91C_PA12_SPI0_NPCS1 |
                                AT91C_PA13_SPI0_NPCS2 | AT91C_PA14_SPI0_NPCS3 ;

    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI0) ;

    // enable
    pSPI->SPI_CR = /*AT91C_SPI_SWRST*/ AT91C_SPI_SPIEN ;

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
    pSPI->SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_MODFDIS /*| (7<<16) */| (SPI_DLYBCS<<24) ;

    // CSx register: mode 3 (CPOL=1, NCPHA=0), 8 bit
//    pSPI->SPI_CSR[0] = AT91C_SPI_CPOL   |
    // CSx register: mode 0 (CPOL=0, NCPHA=1), 8 bit
//{ int i ; for(i=0 ; i<4 ; i++) {
    pSPI->SPI_CSR[0] = AT91C_SPI_NCPHA  |
                       AT91C_SPI_BITS_8 |
                                (SPI_SCBR << 8)   |     // SCBR
                                (SPI_DLYBS << 16) |     // DLYBS
                                (SPI_DLYBCT << 24) ;    // DLYBCT
//} }
    AT91F_AIC_Configure(AT91C_ID_SPI0, SPI_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, spidrv) ;

    // first activity 'by hand', ask dummy SR

    // NPCS0 --\__
    AT91C_BASE_PIOA->PIO_CODR = AT91C_PA11_SPI0_NPCS0 ;

    // tx
    pSPI->SPI_TDR = 0xd7 ;      // SR request

    // wait for rx
    while(!(pSPI->SPI_SR & AT91C_SPI_RDRF))
        ;

    // rx
    buf = pSPI->SPI_RDR ;

    // tx
    pSPI->SPI_TDR = 0 ;         // dummy

    // wait for rx
    while(!(pSPI->SPI_SR & AT91C_SPI_RDRF))
        ;

    // rx
    buf = pSPI->SPI_RDR ;

    // NPCS0 __/--
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_SPI0_NPCS0 ;
#endif // USE_REAL_BOARD

#ifdef USE_EVALUATION_BOARD
    // PIO A: Peripheral B select register
    AT91C_BASE_PIOA->PIO_BSR = (unsigned long)(AT91C_PA8_SPI1_MISO | AT91C_PA9_SPI1_MOSI |
                                               AT91C_PA10_SPI1_SPCK  /*| AT91C_PA7_SPI1_NPCS3*/ ) ;
    // PIO A: disable register
    AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA8_SPI1_MISO | AT91C_PA9_SPI1_MOSI |
                                               AT91C_PA10_SPI1_SPCK  /*| AT91C_PA7_SPI1_NPCS3*/ ) ;

    // configure the PIO Lines corresponding to AT91C_PA7_SPI1_NPCS3 to be outputs
    // no need to set these pins to be driven by the PIO because it is GPIO pins only.
    // set at 1
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PA7_SPI1_NPCS3 ;
    AT91C_BASE_PIOA->PIO_PER = AT91C_PA7_SPI1_NPCS3 ; // Set in PIO mode
    AT91C_BASE_PIOA->PIO_OER = AT91C_PA7_SPI1_NPCS3 ; // Configure in Output

    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI1) ;

    // enable
    pSPI->SPI_CR = AT91C_SPI_SPIEN ;

    // AT45DB321C timings: up to 33MHz
    //  NOUSB: 60MHz -> T=16.7ns ; SPICLK = 60MHz / 3 = 20MHz
    //  USB:   48MHz -> T=20.8ns ; SPICLK = 48MHz / 2 = 24MHz
    //         30MHz -> T=33.3ns ; SPICLK = 30MHz / 2 = 15MHz
    //         16MHz -> T=62.5ns ; SPICLK = 16MHz / 1 = 16MHz
    //          8MHz -> T=125 ns ; SPICLK =  8MHz / 1 =  8MHz
#define SPI_SCBR        ((current_clock/30000000)+1)

    // DLYBCS (delay between CS) = Tcs = 250 ns
    //  NOUSB: 60MHZ --> 267 = 16.7ns * 16
    //  USB:   48MHZ --> 271 = 20.8ns * 13
    //         30MHz --> 267 = 33.3ns * 8
    //         16MHz --> 375 = 62.5ns * 5 -> 6
    //          8MHz --> 750 = 125 ns * 3 -> 6
#define SPI_DLYBCS      (MAX(1+((250*(current_clock/1000000))/1000),6))

    // DLYBS (delay from CS to CLK) = Tcss = 100 ns
    //  NOUSB: 60MHZ --> 117 = 16.7ns * 7
    //  USB:   48MHZ --> 104 = 20.8ns * 5
    //         30MHz --> 133 = 33.3ns * 4
    //         16MHz --> 125 = 62.5ns * 2 -> 3
    //          8MHz --> 125 = 125 ns * 1 -> 3
#define SPI_DLYBS       (MAX(1+((100*(current_clock/1000000))/1000),3))

    // DLYBCT (delay between bytes) = 0 ns = 0
#define SPI_DLYBCT      ((0*(current_clock/1000000))/1000)

    // mode: master, DLYBCS, FixPeriphSelect=7
    pSPI->SPI_MR = AT91C_SPI_MSTR | (7 << 16) | (SPI_DLYBCS << 24) ;

    // CSx register: mode 3 (CPOL=1, NCPHA=0), 8 bit
//    pSPI->SPI_CSR[3] = AT91C_SPI_CPOL   |
    // CSx register: mode 0 (CPOL=0, NCPHA=1), 8 bit
    pSPI->SPI_CSR[3] = AT91C_SPI_NCPHA  |
                       AT91C_SPI_BITS_8 |
                                (SPI_SCBR << 8)   |     // SCBR
                                (SPI_DLYBS << 16) |     // DLYBS
                                (SPI_DLYBCT << 24) ;    // DLYBCT

    AT91F_AIC_Configure(AT91C_ID_SPI1, SPI_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, spidrv) ;

    // first activity 'by hand', ask dummy SR

    // NPCS3 --\__
    AT91C_BASE_PIOA->PIO_CODR = AT91C_PA7_SPI1_NPCS3 ;

    // tx
    pSPI->SPI_TDR = 0xd7 ;      // SR request

    // wait for rx
    while(!(pSPI->SPI_SR & AT91C_SPI_RDRF))
        ;

    // rx
    buf = pSPI->SPI_RDR ;

    // tx
    pSPI->SPI_TDR = 0 ;         // dummy

    // wait for rx
    while(!(pSPI->SPI_SR & AT91C_SPI_RDRF))
        ;

    // rx
    buf = pSPI->SPI_RDR ;

    // NPCS3 __/--
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PA7_SPI1_NPCS3 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    // PIO A: Peripheral A select register
    AT91C_BASE_PIOA->PIO_ASR = (unsigned long)(AT91C_PA12_MISO  | AT91C_PA13_MOSI  |
                                               AT91C_PA14_SPCK) ;
    // PIO A: disable register
    AT91C_BASE_PIOA->PIO_PDR = (unsigned long)(AT91C_PA12_MISO  | AT91C_PA13_MOSI  |
                                               AT91C_PA14_SPCK) ;

    // configure the PIO Lines corresponding to AT91C_PA11_NPCS0 to be outputs
    // no need to set these pins to be driven by the PIO because it is GPIO pins only.
    // set at 1
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_NPCS0 ;
    // Set in PIO mode
    AT91C_BASE_PIOA->PIO_PER  = AT91C_PA11_NPCS0 ;
    // Configure in Output
    AT91C_BASE_PIOA->PIO_OER  = AT91C_PA11_NPCS0 ;

    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_SPI) ;

    // enable
    pSPI->SPI_CR = /*AT91C_SPI_SWRST*/ AT91C_SPI_SPIEN ;

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
    pSPI->SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_MODFDIS /*| (7<<16) */| (SPI_DLYBCS<<24) ;

    // CSx register: mode 3 (CPOL=1, NCPHA=0), 8 bit
//    pSPI->SPI_CSR[0] = AT91C_SPI_CPOL   |
    // CSx register: mode 0 (CPOL=0, NCPHA=1), 8 bit
//{ int i ; for(i=0 ; i<4 ; i++) {
    pSPI->SPI_CSR[0] = AT91C_SPI_NCPHA  |
                       AT91C_SPI_BITS_8 |
                                (SPI_SCBR << 8)   |     // SCBR
                                (SPI_DLYBS << 16) |     // DLYBS
                                (SPI_DLYBCT << 24) ;    // DLYBCT
//} }
    AT91F_AIC_Configure(AT91C_ID_SPI, SPI_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, spidrv) ;

    // first activity 'by hand', ask dummy SR

    // NPCS0 --\__
    AT91C_BASE_PIOA->PIO_CODR = AT91C_PA11_NPCS0 ;

    // tx
    pSPI->SPI_TDR = 0xd7 ;      // SR request

    // wait for rx
    while(!(pSPI->SPI_SR & AT91C_SPI_RDRF))
        ;

    // rx
    buf = pSPI->SPI_RDR ;

    // tx
    pSPI->SPI_TDR = 0 ;         // dummy

    // wait for rx
    while(!(pSPI->SPI_SR & AT91C_SPI_RDRF))
        ;

    // rx
    buf = pSPI->SPI_RDR ;

    // NPCS0 __/--
    AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_NPCS0 ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
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

#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI0 ;
#endif // USE_REAL_BOARD

#ifdef USE_EVALUATION_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI1 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91PS_SPI const pSPI = AT91C_BASE_SPI ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

    for( ; ; ) {
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
        // NPCS0 --\__
        AT91C_BASE_PIOA->PIO_CODR = AT91C_PA11_SPI0_NPCS0 ;
#endif // USE_REAL_BOARD
#ifdef USE_EVALUATION_BOARD
        // NPCS3 --\__
        AT91C_BASE_PIOA->PIO_CODR = AT91C_PA7_SPI1_NPCS3 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        // NPCS0 --\__
        AT91C_BASE_PIOA->PIO_CODR = AT91C_PA11_NPCS0 ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

        pSPI->SPI_TPR = (unsigned long)(buf1tx) ;  // TX buffer 1 ptr
        pSPI->SPI_TCR = len1 ;                     // TX buffer 1 len
        pSPI->SPI_TNPR = (unsigned long)(buf2tx) ; // TX buffer 2 ptr
        pSPI->SPI_TNCR = len2 ;                    // TX buffer 2 len

        pSPI->SPI_RPR = (unsigned long)(buf1rx) ;  // RX buffer 1 ptr
        pSPI->SPI_RCR = len1 ;                     // RX buffer 1 len
        pSPI->SPI_RNPR = (unsigned long)(buf2rx) ; // RX buffer 2 ptr
        pSPI->SPI_RNCR = len2 ;                    // RX buffer 2 len

        pSPI->SPI_IER = AT91C_SPI_RXBUFF ;

        // enable DMA
        pSPI->SPI_PTCR = AT91C_PDC_TXTEN | AT91C_PDC_RXTEN ;

        // wait end
        //KS_wait(SPISEM) ;
        waitval = KS_waitt(SPISEM, ((TICKS)2000/CLKTICK)) ;
        
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
        // NPCS0 __/--
        AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_SPI0_NPCS0 ;
#endif // USE_REAL_BOARD
#ifdef USE_EVALUATION_BOARD
        // NPCS3 __/--
        AT91C_BASE_PIOA->PIO_SODR = AT91C_PA7_SPI1_NPCS3 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        // NPCS0 __/--
        AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_NPCS0 ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

		if (waitval != RC_GOOD) spi0err++ ;
		
        if (waitval == RC_TIMEOUT) {
#ifdef CBUG
            pdebugt(1,"SPI TO=%d, SPI_SR:0x%x, SPI_PTSR:0x%x, SPI_TCR:0x%x, SPI_RCR 0x%x",
                   ++tocount,
                   pSPI->SPI_SR, pSPI->SPI_PTSR,
                   pSPI->SPI_TCR, pSPI->SPI_RCR) ;
#endif // CBUG
        } else {
            break ;
        }
    }
}

//----------------------------------------------------------------------------
// SPI commnunicator
// No Interrupt, No PDC

#ifdef NOT_IMPLEMENTED
void SPI_rtx(unsigned char *buftx, unsigned char *bufrx, int len)
{
#ifdef USE_REAL_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI0 ;
#endif // USE_REAL_BOARD

#ifdef USE_EVALUATION_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI1 ;
#endif // USE_EVALUATION_BOARD

#ifdef USE_EVALUATION_BOARD
    while(len--) {
        // wait for tx
        while(!(pSPI->SPI_SR & AT91C_SPI_TDRE))
            ;

        // tx
        pSPI->SPI_TDR = *buftx++ ;

        // wait for rx
        while(!(pSPI->SPI_SR & AT91C_SPI_RDRF))
            ;

        // rx
        //printf("R=0x%08x'0x%08x\n", AT91C_BASE_SPI1->SPI_RDR, AT91C_BASE_SPI1->SPI_SR) ;
        *bufrx++ = pSPI->SPI_RDR ;
    }
#endif // USE_EVALUATION_BOARD
}
#endif // NOT_IMPLEMENTED

//----------------------------------------------------------------------------
// SPI terminator

void spistop(void)
{
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI0 ;

    // Disable
    pSPI->SPI_CR = AT91C_SPI_SPIDIS ;

    // Disable clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_SPI0) ;
#endif // USE_REAL_BOARD

#ifdef USE_EVALUATION_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI1 ;

    // Disable
    pSPI->SPI_CR = AT91C_SPI_SPIDIS ;

    // Disable clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_SPI1) ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91PS_SPI const pSPI = AT91C_BASE_SPI ;

    // Disable
    pSPI->SPI_CR = AT91C_SPI_SPIDIS ;

    // Disable clock
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_SPI) ;

    // set at 1
    AT91C_BASE_PIOA->PIO_SODR = (unsigned long)(AT91C_PA12_MISO  | AT91C_PA13_MOSI  |
                                                AT91C_PA11_NPCS0 | AT91C_PA14_SPCK) ;
    // Set in PIO mode
    AT91C_BASE_PIOA->PIO_PER  = (unsigned long)(AT91C_PA12_MISO  | AT91C_PA13_MOSI  |
                                                AT91C_PA11_NPCS0 | AT91C_PA14_SPCK) ;
    // Configure in Output
    AT91C_BASE_PIOA->PIO_OER  = (unsigned long)(AT91C_PA12_MISO  | AT91C_PA13_MOSI  |
                                                AT91C_PA11_NPCS0 | AT91C_PA14_SPCK) ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

	spi0err = 0 ; // Reset errors
	spi0start = 0 ;
}
#endif // USE_SPI_ON_ARM
// end of file - drv_spi.c

