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

//----------------------------------------------------------------------------
// internal functions

void dacstart(void) ;
void dacstop(void) ;

#define DMAREQSEL (*(uint32_t *)(0x400FC1C4))

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

    // pins are already configured by dio.c

    // enable DMA - done by main.c
//    LPC_SC->PCONP |= CLKPWR_PCONP_PCGPDMA ;
//    LPC_GPDMA->Config = 0x01 ;     // enable
    // LPC_GPDMA->DMACSync = (1<<6) ;

    // reset the Interrupt status of channel 0
    LPC_GPDMA->IntTCClear = 1 ;
    LPC_GPDMA->IntErrClr = 1 ;
}

//----------------------------------------------------------------------------
// dacstop

void dacstop(void)
{
    // disable DAC
    LPC_DAC->CTRL = 0 ;

    // pins will be un-configured by dio.c

    // disable DMA channel
    LPC_GPDMACH0->CConfig &= (~1) ;
    // done by main.c
//    LPC_GPDMA->Config &= (~1) ;
//    LPC_SC->PCONP &= ~( CLKPWR_PCONP_PCGPDMA ) ;

    // no DAC power off needed
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
    if ( (samplingfreq==0) || (buffer==NULL) ) {
        if (samples==0) {
            LPC_GPDMACH0->CConfig |= (1<<18) ;      // Halt
        } else if (buffer==NULL){
            lli.NextLLI = 0 ;					// To stop at 0 output
        }
        
        while(LPC_GPDMACH0->CConfig & (1<<17));  // wait for FIFO empty

        if ((samplingfreq==0) && (buffer!=NULL)){
    		lli.SrcAddr = (uint32_t)(buffer) ;          // Source Address
    		return ;
		}
			
		LPC_GPDMACH0->CConfig &= (~1) ;         // disable this channel

        LPC_DAC->CTRL = 0 ;                     // disable DAC

        return ;
    }

    // .........................................................
    // sanity check: range from 2 to 4092
    if (samples > 4092) samples = 4092 ;

    // .........................................................
    // set correct sampling frequency

    presca = PERIPHERAL_CLOCK / samplingfreq ;
    if (presca < 25) presca = 25 ;          // min val
    if (presca > 65530) presca = 65530 ;    // max val
    LPC_DAC->CNTVAL = presca - 1 ;

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
    lli.DstAddr = (uint32_t)(&LPC_DAC->CR) ;    // Destination address
    lli.NextLLI = (uint32_t)(&lli) ;            // Next LLI address, otherwise set to '0'
    lli.Control = control ;                     // GPDMA Control of this LLI

    // Assign Linker List Item value
    LPC_GPDMACH0->CLLI = (uint32_t)(&lli) ;

    // Memory to peripheral
    // Assign physical source
    LPC_GPDMACH0->CSrcAddr = (uint32_t)(buffer) ;
    // Assign peripheral destination address
    LPC_GPDMACH0->CDestAddr = (uint32_t)(&LPC_DAC->CR) ;
    LPC_GPDMACH0->CControl = control ;

//    // reset the Interrupt status of channel 0
//    GPDMA->DMACIntTCClear = 1 ;
//    GPDMA->DMACIntErrClr = 1 ;

    // Configure DMA Channel, enable Error Counter and Terminate counter
    LPC_GPDMACH0->CConfig = 1
                  | (0 << 1) 	      // source peripheral (1 - 5) = none
                  | (9 << 6) 	      // destination peripheral (6 - 10) = DAC (it was 7 on LPC17xx)
                  | (1 << 11)	      // flow control (11 - 13) = mem to per
                  | (0 << 14)	      // (14) = mask out error interrupt
                  | (0 << 15)	      // (15) = mask out terminal count interrupt
                  | (0 << 16)	      // (16) = no locked transfers
                  | (0 << 18);        // (27) = no HALT

//    // Enable channel
//    GPDMACH0->DMACCConfig |= 1 ;

    // DMA, timer running, dbuff
    LPC_DAC->CTRL = 1<<3 | 1<<2 | 1<<1 ;
}
#endif // USE_DAC

