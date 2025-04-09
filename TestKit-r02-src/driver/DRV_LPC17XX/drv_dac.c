// drv_dac.c - DAC driver tasks
//
//   Copyright (c) 1997-2011.
//   T.E.S.T. srl
//

//
// This module is provided as a DAC port driver.
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
// only if we are well accepted

#ifdef USE_DAC

//----------------------------------------------------------------------------
// DAC initializer

#define PERIPHERAL_CLOCK    (current_clock/4)

//----------------------------------------------------------------------------
// internal functions

void dacstart(void) ;
void dacstop(void) ;

#define DMAREQSEL (*(uint32_t *)(0x4000C1C4))

struct {
    uint32_t SrcAddr ;      // Source Address
    uint32_t DstAddr ;      // Destination address
    uint32_t NextLLI ;      // Next LLI address, otherwise set to '0'
    uint32_t Control ;      // GPDMA Control of this LLI
} lli __attribute__ ((section(".ahbram1"))) ;

void DACwave(int samplingfreq, signed long * buffer, int samples) ;

//----------------------------------------------------------------------------
// dacstart

void dacstart(void)
{
    // power on
    // DAC always enabled
    SC->PCONP |= PCONP_PCGPDMA ;

    // set related pin to DAC
    PINCON->PINSEL1 |= (2<<20) ;    // P0.26 function 10

    // all the related pins are set with NO pull-up/down
    PINCON->PINMODE1 &= ~(1<<20) ;      // P0.26 function 10
    PINCON->PINMODE1 |=  (2<<20) ;

    // default div by 4
    //SC->PCLKSEL1

    // enable DMA
    GPDMA->DMACConfig = 0x01 ;  // enable
//    GPDMA->DMACSync = (1<<6) ;

    // reset the Interrupt status of channel 0
    GPDMA->DMACIntTCClear = 1 ;
    GPDMA->DMACIntErrClr = 1 ;
}

//----------------------------------------------------------------------------
// dacstop

void dacstop(void)
{
    // disable DAC
    DAC->DACCTRL = 0 ;

    PINCON->PINSEL1 &= ~(3<<20) ;       // P0.26 function 00

    // disable DMA channel
    GPDMACH0->DMACCConfig &= (~1) ;
    GPDMA->DMACConfig &= (~1) ;

    // power off
    SC->PCONP &= ~( PCONP_PCGPDMA ) ;
}

//----------------------------------------------------------------------------
// DAC wave
// 
// possibly buffer is AHBRAM1_BASE

void DACwave(int samplingfreq, signed long * buffer, int samples)
{
    uint32_t control, presca ;

    // .........................................................
    // check stop request
    if ( (samplingfreq==0) || (buffer==NULL) || (samples==0) ) {
        GPDMACH0->DMACCConfig |= (1<<18) ;      // Halt
        while(GPDMACH0->DMACCConfig & (1<<17))  // wait for FIFO empty
            ;
        GPDMACH0->DMACCConfig &= (~1) ;         // disable this channel

        DAC->DACCTRL = 0 ;     // disable DAC

        return ;
    }

    // .........................................................
    // sanity check: range from 2 to 4095
    if (samples > 4095) samples = 4095 ;

    // .........................................................
    // set correct sampling frequency

    presca = PERIPHERAL_CLOCK / samplingfreq ;
    if (presca < 25) presca = 25 ;          // min val
    if (presca > 65530) presca = 65530 ;    // max val
    DAC->DACCNTVAL = presca - 1 ;

    // .........................................................
    // enable DMA

    control = (samples & 0xfff) // transfer size
                  | (0 << 12)         // source burst size (12 - 14) = 1
                  | (0 << 15)         // destination burst size (15 - 17) = 1
                  | (2 << 18)         // source width (18 - 20) = 32 bit
                  | (2 << 21)         // destination width (21 - 23) = 32 bit
                  | (0 << 24)         // source AHB select (24) = AHB 0
                  | (0 << 25)         // destination AHB select (25) = AHB 0
                  | (1 << 26)         // source increment (26) = increment
                  | (0 << 27)         // destination increment (27) = no increment
                  | (0 << 28)         // mode select (28) = access in user mode
                  | (0 << 29)         // (29) = access not bufferable
                  | (0 << 30)         // (30) = access not cacheable
                  | (0 << 31);        // terminal count interrupt disabled


    // rewrite Linker List Item values
    lli.SrcAddr = (uint32_t)(buffer) ;          // Source Address
    lli.DstAddr = (uint32_t)(&DAC->DACR) ;      // Destination address
    lli.NextLLI = (uint32_t)(&lli) ;            // Next LLI address, otherwise set to '0'
    lli.Control = control ;                     // GPDMA Control of this LLI

    // Assign Linker List Item value
    GPDMACH0->DMACCLLI = (uint32_t)(&lli) ;

    // Memory to peripheral
    // Assign physical source
    GPDMACH0->DMACCSrcAddr = (uint32_t)(buffer) ;
    // Assign peripheral destination address
    GPDMACH0->DMACCDestAddr = (uint32_t)(&DAC->DACR) ;
    GPDMACH0->DMACCControl = control ;

//    // reset the Interrupt status of channel 0
//    GPDMA->DMACIntTCClear = 1 ;
//    GPDMA->DMACIntErrClr = 1 ;

    // Configure DMA Channel, enable Error Counter and Terminate counter
    GPDMACH0->DMACCConfig = 1
                  | (0 << 1) 	      // source peripheral (1 - 5) = none
                  | (7 << 6) 	      // destination peripheral (6 - 10) = DAC
                  | (1 << 11)	      // flow control (11 - 13) = mem to per
                  | (0 << 14)	      // (14) = mask out error interrupt
                  | (0 << 15)	      // (15) = mask out terminal count interrupt
                  | (0 << 16)	      // (16) = no locked transfers
                  | (0 << 18);        // (27) = no HALT

//    // Enable channel
//    GPDMACH0->DMACCConfig |= 1 ;

    // DMA, timer running, dbuff
    DAC->DACCTRL = 1<<3 | 1<<2 | 1<<1 ; 
}
#endif // USE_DAC

