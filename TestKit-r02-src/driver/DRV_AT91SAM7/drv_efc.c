// drv_efc.c - Embedded Flash Controller driver tasks

//
//   Copyright (c) 1997-2009.
//   T.E.S.T. srl
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
#if defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)

#if (FLASH_PAGESIZE != AT91C_IFLASH_PAGE_SIZE)
#error Page size MUST be 256 for AT91SAM7S512
#endif

#define EFC_BASE_ADDRESS        (AT91C_IFLASH + (AT91C_IFLASH_SIZE/2))

#define NULLSEMA ((SEMA)0)

static unsigned char pPageBuffer[AT91C_IFLASH_PAGE_SIZE];

//----------------------------------------------------------------------------
// functions in drv_clk.c

extern void AT91F_AIC_Configure(int irq_id, int priority, int src_type, FRAME *(*newHandler)(FRAME *frame)) ;

//----------------------------------------------------------------------------
// internal functions

void EFC_FlashErase(unsigned long bbegin, unsigned long bend) ;
void EFC_FlashRead(unsigned char *dst, unsigned long bbegin, int flen) ;
void EFC_FlashWrite(unsigned long bbegin, unsigned char *src, unsigned char *dst, int flen) ;

//----------------------------------------------------------------------------
// Interrupt routine for EFC
// same SYSTEM vector

void efcdrvpit(void)
{
    if ( (AT91C_BASE_EFC1->EFC_FMR) & AT91C_MC_FRDY) {          // interrupt enabled ?
        if ( (AT91C_BASE_EFC1->EFC_FSR) & AT91C_MC_FRDY) {      // interrupt pending ?
            AT91C_BASE_EFC1->EFC_FMR &= ~(AT91C_MC_FRDY) ;      // disable interrupt
            KS_ISRsignal(EFCSEM) ;      // Embedded Flash Controller semaphore
        }
    }
}

//----------------------------------------------------------------------------
// EFC_FlashErase
// first address to erase TO last address to erase
// inside FLASH_PAGESIZE boundary

void EFC_FlashErase(unsigned long bbegin, unsigned long bend)
{
    bbegin &= (~(FLASH_PAGESIZE-1)) ;      // clear page bits
    bend &= (~(FLASH_PAGESIZE-1)) ;

    // use same settings of main flash
    AT91C_BASE_EFC1->EFC_FMR = AT91C_BASE_EFC0->EFC_FMR & (~AT91C_MC_NEBP) ;

    // loop for all bytes
    while(bbegin <= bend) {
    
        // what to write
        memcpy((void *)(EFC_BASE_ADDRESS + bbegin), 0xff, FLASH_PAGESIZE) ;

        // start of erase
        AT91C_BASE_EFC1->EFC_FCR = (AT91C_MC_KEY & (0x5a<<24)) | (bbegin & 0x3ff00) | AT91C_MC_FCMD_ERASE_ALL ;
        AT91C_BASE_EFC1->EFC_FMR |= AT91C_MC_FRDY ;     // enable interrupt

        bbegin += FLASH_PAGESIZE ;

        KS_wait(EFCSEM) ;       // Embedded Flash Controller semaphore

    }
}

//----------------------------------------------------------------------------
// EFC_FlashRead

void EFC_FlashRead(unsigned char *dst, unsigned long bbegin, int flen)
{
    memcpy(dst, (void *)(EFC_BASE_ADDRESS + bbegin), flen) ;
}

//----------------------------------------------------------------------------
// EFC_FlashWrite
// dummy 'dst' not used

void EFC_FlashWrite(unsigned long bbegin, unsigned char *src, unsigned char *dst, int flen)
{
    int plen ;

    // use same settings of main flash
    AT91C_BASE_EFC1->EFC_FMR = AT91C_BASE_EFC0->EFC_FMR | AT91C_MC_NEBP ;

    while(flen) {
        // attention to page boundary
        plen = MIN(FLASH_PAGESIZE - (bbegin & (FLASH_PAGESIZE-1)), flen) ;

        // Pre-buffer data (optimizable: copy only desired part)
        memcpy(pPageBuffer, (void *)(EFC_BASE_ADDRESS + (bbegin & ~(FLASH_PAGESIZE-1))), (bbegin & (FLASH_PAGESIZE-1)) ;

        // Buffer data
        memcpy(pPageBuffer + (bbegin & (FLASH_PAGESIZE-1), src, plen);

        // Post-buffer data
        memcpy(pPageBuffer + (bbegin & (FLASH_PAGESIZE-1) + plen,
               (void *)(EFC_BASE_ADDRESS + bbegin + plen),
               FLASH_PAGESIZE - (bbegin & (FLASH_PAGESIZE-1) - plen);

        // real data copy
        memcpy((void *)(EFC_BASE_ADDRESS + bbegin), pPageBuffer, FLASH_PAGESIZE) ;
    
        // start of copy
        AT91C_BASE_EFC1->EFC_FCR = (AT91C_MC_KEY & (0x5a<<24)) | (bbegin & 0x3ff00) | AT91C_MC_FCMD_START_PROG ;
        AT91C_BASE_EFC1->EFC_FMR |= AT91C_MC_FRDY ;     // enable interrupt

        flen -= plen ;
        src += plen ;
        bbegin += plen ;

        KS_wait(EFCSEM) ;       // Embedded Flash Controller semaphore
    }
}
#endif // defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)
// end of file - drv_efc.c

