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

struct _DAC_LLI{
    uint32_t SrcAddr ;      // Source Address
    uint32_t DstAddr ;      // Destination address
    uint32_t NextLLI ;      // Next LLI address, otherwise set to '0'
    uint32_t Control ;      // GPDMA Control of this LLI
} ; //lli __attribute__ ((section(".ahbram1"))) ;

struct _DAC_LLI *plli ;

void DACwave(int samplingfreq, signed long * buffer, int samples
#ifdef OSTEO
, int dhz,  signed long * buffer1
#endif
) ;

#ifdef OSTEO
char out_on ;
#endif
//----------------------------------------------------------------------------
// dacstart

void dacstart(void)
{
#ifdef OSTEO
    LPC_SC->PCONP |= CLKPWR_PCONP_PCTIM3  ;
    LPC_TIM3->TCR = 2 ;     // reset
    out_on = NO ;
#endif

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
#ifdef OSTEO
//     // disable use timer 0 interrupt
//     LPC_TIM3->TCR = 2 ;     // reset
//     NVIC_DisableIRQ(TIMER3_IRQn) ;
	
    LPC_SC->PCONP &= ~CLKPWR_PCONP_PCTIM3 ;
#endif

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

#ifdef OSTEO
//uint32_t bk_lli ;
// int bk_samplingfreq ;
// signed long *bk_buffer ;
// int bk_samples ;
struct _DAC_LLI *lli_on, *lli_off ;

void TIMER3_IRQHandler(void)
{
register int ir = LPC_TIM3->IR ;     // don't bother us

	if (ir){
		if (out_on & 0x1){
			lli_off->NextLLI = (uint32_t)(lli_off) ;
			while(LPC_GPDMACH0->CConfig & (1<<17));  // wait for FIFO empty
			plli->NextLLI = (uint32_t)(lli_off) ;          // Next LLI address, otherwise set to '0'
			plli = lli_off ;
//			plli->NextLLI = 0 ;					// To stop at 0 output
			out_on = 0x10 ;
		}else{
			lli_on->NextLLI = (uint32_t)(lli_on) ;
			while(LPC_GPDMACH0->CConfig & (1<<17));  // wait for FIFO empty
			plli->NextLLI = (uint32_t)(lli_on) ;          // Next LLI address, otherwise set to '0'
			plli = lli_on ;
//			DACwave(bk_samplingfreq, bk_buffer, bk_samples, 0 ) ;
			out_on = 0x11 ;
		}
	}
    LPC_TIM3->IR = ir ;         // reset interrupt
}


// void DACosteo(int samplingfreq, signed long * buffer, int samples, int dhz)
// {
//     DACwave(samplingfreq, buffer, samples );
// 	
// 	LPC_TIM3->TCR = 2 ;     // reset
// 	out_on = YES ;
// 	
// 	if ( (samplingfreq==0) || (buffer==NULL) ) {
// 		// disable use timer 0 interrupt
// 		NVIC_DisableIRQ(TIMER3_IRQn) ;
// 		return ;
// 	}
// 	
//     // enable interrupt from timer 3
//     NVIC_EnableIRQ(TIMER3_IRQn) ;
//     NVIC_SetPriority(TIMER3_IRQn, TWI_INTERRUPT_LEVEL) ;
// 	
// 	// use timer 0 interrupt
// 	LPC_TIM3->TCR = 2 ;     // reset
// 	LPC_TIM3->CTCR = 0 ;    // count at PCLK
// 	LPC_TIM3->MR0 = (PERIPHERAL_CLOCK*10)/dhz ;   // desired frequency
// 	LPC_TIM3->IR  = 1 ;     // reset interrrupt
// 	LPC_TIM3->MCR = 3 ;     // reset TC on match with MR0 + INTERRUPT
// 	LPC_TIM3->TCR = 1 ;     // enable
// 
// }
#endif

//----------------------------------------------------------------------------
// DAC wave
//
// possibly buffer is AHBRAM1_BASE
// first 4 long of buffer reserved for LLI
void DACwave(int samplingfreq, signed long * buffer, int samples
#ifdef OSTEO
, int dhz,  signed long * buffer1
#endif
)
{
uint32_t control, presca ;
struct _DAC_LLI * newlli ;
signed long *dac_data ;

	// .........................................................
	// check stop request
	if ( (samplingfreq==0) || (buffer==NULL) ) {
		
#ifdef OSTEO
		// disable use timer 3 interrupt
		if ( (dhz) || (out_on)){
			LPC_TIM3->TCR = 2 ;     // reset
			//NVIC_DisableIRQ(TIMER3_IRQn) ;
			out_on = NO ;
		}
#endif

		if (!LPC_DAC->CTRL) return ;
		
		if (samples==0) {
			LPC_GPDMACH0->CConfig |= (1<<18) ;      // Halt
		} else if (buffer==NULL){
			plli->NextLLI = 0 ;					// To stop at 0 output
		}

		if ((samplingfreq==0) && (buffer!=NULL)){
			newlli = (struct _DAC_LLI *) buffer ;
			dac_data = buffer + 4 ; 
			//plli->SrcAddr = (uint32_t)(dac_data) ;          // Source Address

			// update nr of sample
			if (samples > 4092) samples = 4092 ;
			
			newlli->SrcAddr = (uint32_t)(dac_data) ;         // Source Address
			newlli->DstAddr = (uint32_t)(&LPC_DAC->CR) ;     // Destination address
			newlli->NextLLI = (uint32_t)(newlli) ;           // Next LLI address, otherwise set to '0'
			newlli->Control = (plli->Control & 0xfffff000) ; // GPDMA Control of this LLI
			newlli->Control |= (samples & 0xfff) ; 			 // transfer size
		}
		
		while(LPC_GPDMACH0->CConfig & (1<<17));  // wait for FIFO empty

		if ((samplingfreq==0) && (buffer!=NULL)){
			plli->NextLLI = (uint32_t)(newlli) ;          // Next LLI address, otherwise set to '0'
			plli = newlli ;
			return ;
		}

		LPC_GPDMACH0->CConfig &= (~1) ;         // disable this channel

		LPC_DAC->CTRL = 0 ;                     // disable DAC

		return ;
	}

	dac_data = buffer + 4 ; 
	newlli = (struct _DAC_LLI *) buffer ;

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
	newlli->SrcAddr = (uint32_t)(dac_data) ;        // Source Address
	newlli->DstAddr = (uint32_t)(&LPC_DAC->CR) ;    // Destination address
	newlli->NextLLI = (uint32_t)(newlli) ;          // Next LLI address, otherwise set to '0'
	newlli->Control = control ;                     // GPDMA Control of this LLI
	plli = newlli ;
	
	// Assign Linker List Item value
	LPC_GPDMACH0->CLLI = (uint32_t)(plli) ;

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

#ifdef OSTEO
	if (dhz){
		LPC_TIM3->TCR = 2 ;     // reset
		out_on = 0x11 ;
		
		lli_on = plli ;
		
		lli_off = (struct _DAC_LLI *) buffer1 ;
		dac_data = buffer1 + 4 ; 
		//plli->SrcAddr = (uint32_t)(dac_data) ;          // Source Address

		// update nr of sample
		if (samples > 4092) samples = 4092 ;
		
		lli_off->SrcAddr = (uint32_t)(dac_data) ;         // Source Address
		lli_off->DstAddr = (uint32_t)(&LPC_DAC->CR) ;     // Destination address
		lli_off->NextLLI = (uint32_t)(lli_off) ;           // Next LLI address, otherwise set to '0'
		lli_off->Control = (plli->Control & 0xfffff000) ; // GPDMA Control of this LLI
		lli_off->Control |= (samples & 0xfff) ; 			 // transfer size
		
		// enable interrupt from timer 3
		NVIC_EnableIRQ(TIMER3_IRQn) ;
		NVIC_SetPriority(TIMER3_IRQn, TWI_INTERRUPT_LEVEL) ;
		
		// use timer 0 interrupt
		LPC_TIM3->TCR = 2 ;     // reset
		LPC_TIM3->CTCR = 0 ;    // count at PCLK
		LPC_TIM3->MR0 = (PERIPHERAL_CLOCK*10)/dhz ;   // desired frequency
		LPC_TIM3->IR  = 1 ;     // reset interrrupt
		LPC_TIM3->MCR = 3 ;     // reset TC on match with MR0 + INTERRUPT
		LPC_TIM3->TCR = 1 ;     // enable

// 		bk_samplingfreq = samplingfreq ;
// 		bk_buffer = buffer ;
// 		bk_samples = samples ;
	}
#endif

}
#endif // USE_DAC

