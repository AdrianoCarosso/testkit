// drv_i2s.c - I2s driver tasks
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

//
// This module is provided as a I2S port driver.
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

#ifdef USE_I2S

//----------------------------------------------------------------------------
// I2S initializer

#define BITRATE        32   // 16 bit per word, mono repetead as stereo
#define SAMPLEFREQ     8000  //44100   // default sample frequency      

#define PERIPHERAL_CLOCK    (current_clock/2)

//----------------------------------------------------------------------------
// internal functions

void i2sstart(void) ;
void i2sstop(void) ;

#define DMAREQSEL (*(uint32_t *)(0x4000C1C4))

struct {
    uint32_t SrcAddr ;      // Source Address
    uint32_t DstAddr ;      // Destination address
    uint32_t NextLLI ;      // Next LLI address, otherwise set to '0'
    uint32_t Control ;      // GPDMA Control of this LLI
} lli __attribute__ ((section(".ahbram1"))) ;

void SetI2sFrequency(int samplefreq) ;
void i2swave(int samplingfreq, signed short * buffer, int samples) ;

//----------------------------------------------------------------------------
// i2sstart

void i2sstart(void)
{
    // power on
    SC->PCONP |= ( PCONP_PCI2S | PCONP_PCGPDMA ) ;

    // all the related pins are set to I2S
    PINCON->PINSEL0 &= ~0x000AAA00 ;    // P0.4~9, function 01
    PINCON->PINSEL0 |=  0x00055500 ;

    // all the related pins are set with NO pull-up/down
    PINCON->PINMODE0 &= ~0x00055500 ;   // P0.4~9, function 10
    PINCON->PINMODE0 |=  0x000AAA00 ;

    // default div by 2
    //SC->PCLKSEL1 |= 0x00800000 ;       // bit23,22: 10 - PCLK = CCLK/2 

    // 16bit, mono, nostop, master, WS=32clk period, nomute, reset
    // Init i2s TX - bitrate | 16 bit | mono | reset /*|  mute  */ | stop
    I2S->I2SDAO =    (15<<6) |   0x1  |  0x4 | 0x10  /*| 0x8000 */ | 0x8 ; 
//    I2S->I2SDAO = (15<<6) | 0x1 | 0x10 ; // 16bit, stereo, nostop, master, WS=32clk period, nomute

    // Init i2s RX
//    I2S->I2SDAI = 0x3CD | 0x10 ; // 16bit, mono, stop, master, WS=32clk period

    I2S->I2STXMODE = 0 ;
//    I2S->I2SRXMODE = 0 ;

//    SetI2sFrequency(SAMPLEFREQ) ;

    I2S->I2SDAO &= ~0x10 ;  // no reset
//    I2S->I2SDAI &= ~0x10 ;  // no reset

    // enable DMA
    GPDMA->DMACConfig = 0x01 ;  // enable
    GPDMA->DMACSync = 0x01 ;

    // reset the Interrupt status of channel 0
    GPDMA->DMACIntTCClear = 1 ;
    GPDMA->DMACIntErrClr = 1 ;

}

//----------------------------------------------------------------------------
// I2S stop

void i2sstop(void)
{

    // GPIO set at '0'
//    GPIO0->FIOCLR  = 0x000003F0 ;
//    GPIO0->FIODIR |= 0x000003F0 ;

    // GPIO again, function 00
//    PINCON->PINSEL0 &= ~0x000FFF00 ;    // P0.4~9, function 00

    // disable I2S
    I2S->I2SDAO = 0x7E1 ;       // stop
//    I2S->I2SDAI = 0x7E1 ;

    // disable DMA channel
    GPDMACH0->DMACCConfig &= (~1) ;
    GPDMA->DMACConfig &= (~1) ;

    // power off
    SC->PCONP &= ~( PCONP_PCI2S | PCONP_PCGPDMA ) ;
}

//----------------------------------------------------------------------------
// SetI2sFrequency

void SetI2sFrequency(int samplefreq)
{
    unsigned long long divider ;
    unsigned long x, y;
    unsigned int dif, ErrorOptimal, error ;
    unsigned char x_divide, y_divide ;

    // Calculate X and Y divider
    // The MCLK rate for the I2S transmitter is determined by the value
    // in the I2STXRATE/I2SRXRATE register. The required I2STXRATE/I2SRXRATE
    // setting depends on the desired audio sample rate desired, the format
    // (stereo/mono) used, and the data size.
    // The formula is:
    //      I2S_MCLK = PCLK * (X/Y) / 2
    // We have:
    //      I2S_MCLK = Freq * bit_rate;
    // So: (X/Y) = (Freq * bit_rate)/PCLK*2
    // We use a loop function to chose the most suitable X,Y value

    samplefreq *= 2 ;   // bitrate divides by 2

    ErrorOptimal = 0xFFFF ;
    y_divide = 0 ;
    divider = ((unsigned long long)(samplefreq * BITRATE * 2ULL)<<16ULL) / PERIPHERAL_CLOCK ;
    for(y = 255; y > 0; y--) {
        x = y * divider ;
        dif = x & 0xFFFF;
        if (dif>0x8000) error = 0x10000 - dif ;
        else error = dif ;
        if (error == 0) {
            y_divide = y;
            break ;
        } else if (error < ErrorOptimal) {
            ErrorOptimal = error ;
            y_divide = y ;
        }
    }
//    x_divide = (y_divide * samplefreq * BITRATE * 2)/PERIPHERAL_CLOCK  + 1 ;
    x_divide = ((y_divide * divider) + 0x8000) >> 16 ;

    I2S->I2STXRATE = y_divide | (x_divide << 8) ;
//    I2S->I2SRXRATE = y_divide | (x_divide << 8) ;

    // Bitrate
    I2S->I2STXBITRATE = 1 /* BITRATE - 1 */ ;
//    I2S->I2SRXBITRATE = BITRATE - 1 ;

}

//----------------------------------------------------------------------------
// I2S wave
// 
// possibly buffer is AHBRAM1_BASE

void i2swave(int samplingfreq, signed short * buffer, int samples)
{
    uint32_t control ;

    // .........................................................
    // check stop request
    if ( (samplingfreq==0) || (buffer==NULL) || (samples==0) ) {
        //GPDMACH0->DMACCConfig &= (~(1<<18)) ;   // Halt
        GPDMACH0->DMACCConfig |= (1<<18) ;      // Halt
        while(GPDMACH0->DMACCConfig & (1<<17))  // wait for FIFO empty
            ;
        GPDMACH0->DMACCConfig &= (~1) ;         // disable this channel

//        I2S->I2SDAO |= 0x10 ;       // reset
//        I2S->I2SDAO |= 0x8000 ;     // mute
        I2S->I2SDAO |= 0x8 ;        // stop

        return ;
    }

//    GPDMACH0->DMACCConfig &= (~1) ;

    // .........................................................
    // sanity check: range from 2 to 4095 (22050 Hz - 10 Hz)
    samples >>= 1 ;     // long counter
    if (samples > 4095) samples = 4095 ;

    // .........................................................
    // set correct sampling frequency

    SetI2sFrequency(samplingfreq) ;

    // .........................................................
    // enable DMA

    control = (samples & 0xfff) // transfer size
            | (4<<12)           // source burst = 32
            | (4<<15)           // destination burst = 32
            | (2<<18)           // source width = 4 byte (word)
            | (2<<21)           // destination width = 4 byte (word)
            | (1<<26)           // source increment
            ;

    // rewrite Linker List Item values
    lli.SrcAddr = (uint32_t)(buffer) ;          // Source Address
    lli.DstAddr = (uint32_t)(&I2S->I2STXFIFO) ; // Destination address
    lli.NextLLI = (uint32_t)(&lli) ;            // Next LLI address, otherwise set to '0'
    lli.Control = control ;                     // GPDMA Control of this LLI

    // Assign Linker List Item value
    GPDMACH0->DMACCLLI = (uint32_t)(&lli) ;

    // Memory to peripheral
    // Assign physical source
    GPDMACH0->DMACCSrcAddr = (uint32_t)(buffer) ;
    // Assign peripheral destination address
    GPDMACH0->DMACCDestAddr = (uint32_t)(&I2S->I2STXFIFO) ;
    GPDMACH0->DMACCControl = control ;

    // Configure DMA Channel, enable Error Counter and Terminate counter
    GPDMACH0->DMACCConfig = (1<<11)         // transfer type memory -> peripheral
                          | (5<<6)          // destination peripheral: I2S channel 0
                          ;

    // Enable channel
    GPDMACH0->DMACCConfig |= 1 ;

    // enble DMA
    I2S->I2SDMA1 = 2 | (4<<16) ;    // enable TX depth=4

//    I2S->I2SDAO &= ~0x10 ;  // no reset
//    I2S->I2SDAO &= ~0x8000 ; // no mute
    I2S->I2SDAO &= ~0x8    ; // no stop
}
#endif // USE_I2S

