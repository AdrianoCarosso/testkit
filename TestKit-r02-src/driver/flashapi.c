// flashapi.c - SPI flash driver
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//
// It uses DRV_XXX/drv_spi.c
// It works with:
//  - flashcode=1   ATMEL     AT45DB161D
//  - flashcode=2   NUMONYX   M45PE16
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
#if (defined(USE_SPI_ON_ARM) && defined(USE_SERIALFLASH_ON_ARM))

unsigned char flashcode = FLASHCODE_UNKNOWN ;

#define NULLSEMA ((SEMA)0)

#undef DEBUG_SPI

//----------------------------------------------------------------------------
// external functions from drv_spi.c

extern void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
                     unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;

//----------------------------------------------------------------------------
// internal functions

void SPI_FlashQuery(void) ;
void SPI_FlashWaitForEnd(void) ;
void SPI_FlashErase(unsigned long bbegin, unsigned long bend) ;
void SPI_FlashRead(unsigned char *dst, unsigned long bbegin, int flen) ;
void SPI_FlashWrite(unsigned long bbegin, unsigned char *src, unsigned char *dst, int flen) ;

//----------------------------------------------------------------------------
// SPI_FlashWaitForEnd

void SPI_FlashWaitForEnd(void) 
{
    unsigned char buftx[2], bufrx[2] ;
    int stayinloop ;
    
    for(stayinloop = 1 ; stayinloop ; ) {
        switch(flashcode) {
        case FLASHCODE_ATMEL_AT45DB161D :   buftx[0] = 0xd7 ; break ;
        case FLASHCODE_NUMONYX_M45PE16 :    buftx[0] = 0x05 ; break ;
        }

        SPI_rtx2(buftx, bufrx, 2, NULL, NULL, 0) ;

        if (bufrx[1] == 0xff) {
#if defined(CBUG)
        	pdebugt(1, "SR: 0x%02x -- performing spistart() ", bufrx[1]) ;
#endif // defined(CBUG)
            spistart() ;
        } else {
            switch(flashcode) {
            case FLASHCODE_ATMEL_AT45DB161D :   if   (bufrx[1] & 0x80)  stayinloop = 0 ; break ;
            case FLASHCODE_NUMONYX_M45PE16 :    if (!(bufrx[1] & 0x01)) stayinloop = 0 ; break ;
            }
        }
    }
}

//----------------------------------------------------------------------------
// SPI_FlashQuery
// Check Flash model

void SPI_FlashQuery(void)
{
    unsigned char buftx[5] ;
    unsigned char bufrx[5] ;

    buftx[0] = 0x9f ;
    SPI_rtx2(buftx, bufrx, 5, NULL, NULL, 0) ;
#if defined(CBUG)
    pdebugt(1, "FlashQuery ID: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x",
                bufrx[0], bufrx[1], bufrx[2], bufrx[3], bufrx[4]) ;
#endif // defined(CBUG)

    // check for ATMEL
    if ( (bufrx[1] == 0x1f) &&          // Manufacturer 0x1f --> ATMEL
         (bufrx[2] == 0x26) &&          // Family/Dens. 0x26 --> Family 0, Density 16 Mbit
         (bufrx[3] == 0x00) ) {         // MLC/Version  0x00 --> MLC 1bit/ceil technology, initial version
        flashcode = FLASHCODE_ATMEL_AT45DB161D ;
#if defined(CBUG)
        pdebugt(1, "FlashQuery: ATMEL AT45DB161D") ;
#endif // defined(CBUG)

#if defined(CBUG) && defined(DEBUG_SPI)
        buftx[0] = 0xd7 ;
        SPI_rtx2(buftx, bufrx, 2, NULL, NULL, 0) ;
        pdebugt(1, "FlashQuery SR: 0x%02x", bufrx[1]) ;
#endif // defined(CBUG) && defined(DEBUG_SPI)
    }

    // check for NUMONYX
    if ( (bufrx[1] == 0x20) &&          					// Manufacturer 0x20 --> NUMONYX
         ( (bufrx[2] == 0x40) || (bufrx[2] == 0x80) ) &&	// Family/Dens. 0x40 --> Family 0, Density 16 Mbit
         (bufrx[3] == 0x15) ) {         					// MLC/Version  0x15 (in case of DEBUG, check 0x14 for 1MB chip)
        flashcode = FLASHCODE_NUMONYX_M45PE16 ;
#if defined(CBUG)
        pdebugt(1, "FlashQuery: NUMONYX M45PE16") ;
#endif // defined(CBUG)

#if defined(CBUG) && defined(DEBUG_SPI)
        buftx[0] = 0x05 ;
        SPI_rtx2(buftx, bufrx, 2, NULL, NULL, 0) ;
        pdebugt(1, "FlashQuery SR: 0x%02x", bufrx[1]) ;
#endif // defined(CBUG) && defined(DEBUG_SPI)
    }
}

//----------------------------------------------------------------------------
// SPI_FlashErase
// first address to erase TO last address to erase
// inside FLASH_PAGESIZE boundary
#if defined(CBUG)
extern char cbugflag ;                 // clock ISR ignore flag, initially = 0
#endif
void SPI_FlashErase(unsigned long bbegin, unsigned long bend)
{
    unsigned char buftx[4] ;
    unsigned char bufrx[4] ;
#if defined(CBUG) && defined(DEBUG_SPI)
	unsigned long lbbegin = bbegin ;
#endif

    // check if device is known
    if (flashcode == FLASHCODE_UNKNOWN) {
        SPI_FlashQuery() ;      // who are you?
        if (flashcode == FLASHCODE_UNKNOWN)
            return ;            // no way!
    }

    bbegin &= (~(FLASH_PAGESIZE-1)) ;      // clear page bits
    // Added _FR_ 12/01/11 ->
	bend-- ;
    if (bend<bbegin)
		bend = bbegin + (FLASH_PAGESIZE-1) ;
    // <-Added _FR_ 12/01/11
    bend &= (~(FLASH_PAGESIZE-1)) ;
    // Added _FR_ 12/01/11
	bend += (FLASH_PAGESIZE-1) ;
	// removed _FR_ 12/01/11 (bad  check)
    //if ((bbegin - bend) < (FLASH_PAGESIZE-1)) bend = bbegin + (FLASH_PAGESIZE-1) ;

#if defined(CBUG)
	if (cbugflag) printf("Erase from  0x%lx to 0x%lx\n", bbegin, bend) ;
#endif

#if defined(DEBUG_SPI)
    // wait for end
    SPI_FlashWaitForEnd() ;
#endif // defined(DEBUG_SPI)

    // loop for all bytes
    while(bbegin <= bend) {
        // chip based
        switch(flashcode) {
        case FLASHCODE_ATMEL_AT45DB161D :
            // check if BLOCK size is possible
            if (((bbegin & 0xe00) == 0) &&                  // low boundary is OK
                ((bend - bbegin + FLASH_PAGESIZE) >= 4096) ) { // size is OK
                buftx[0] = 0x50 ;          // block erase
                buftx[1] = bbegin >> 15 ;
                buftx[2] = bbegin >> 7 ;
                buftx[3] = 0 ;
                //pdebugt(1,"Erase block from 0x%08lx", bbegin) ;

                // ready for next
                bbegin += 4096 ;            // block size

            // PAGE erase
            } else {
                buftx[0] = 0x81 ;          // page erase
                buftx[1] = bbegin >> 15 ;
                buftx[2] = bbegin >> 7 ;
                buftx[3] = 0 ;
                //pdebugt(1,"Erase page from 0x%08lx", bbegin) ;

                // ready for next
                bbegin += FLASH_PAGESIZE ;     // page size
            }
            break ;

        case FLASHCODE_NUMONYX_M45PE16 :
            // first of all enable write
            buftx[0] = 0x06 ;   // write enable
            SPI_rtx2(buftx, bufrx, 1, NULL, NULL, 0) ;

            // check if BLOCK size is possible
            if (((bbegin & 0xff00) == 0) &&                  // low boundary is OK
                ((bend - bbegin + FLASH_PAGESIZE) >= 65536) ) { // size is OK
                buftx[0] = 0xd8 ;          // sector erase
                buftx[1] = bbegin >> 16 ;
                buftx[2] = bbegin >> 8 ;
                buftx[3] = bbegin ;
                //pdebugt(1,"Erase sector from 0x%08lx", bbegin) ;

                // ready for next
                bbegin += 65536 ;          // sector size

            // PAGE erase
            } else {
                buftx[0] = 0xdb ;          // page erase
                buftx[1] = bbegin >> 16 ;
                buftx[2] = bbegin >> 8 ;
                buftx[3] = bbegin ;
                //pdebugt(1,"Erase page from 0x%08lx", bbegin) ;

                // ready for next
                bbegin += 256 ;             // page size
            }
            break ;
        }

        // erase command
        SPI_rtx2(buftx, bufrx, 4, NULL, NULL, 0) ;

        // wait for end
        SPI_FlashWaitForEnd() ;
    }
}

//----------------------------------------------------------------------------
// SPI_FlashRead

void SPI_FlashRead(unsigned char *dst, unsigned long bbegin, int flen)
{
    unsigned char buftx[8] ;
    unsigned char bufrx[8] ;
    extern unsigned char _data[] ;      // RAM begin, used as dummy data to send
    int plen = 0 ;

    // check if device is known
    if (flashcode == FLASHCODE_UNKNOWN) {
        SPI_FlashQuery() ;      // who are you?
        if (flashcode == FLASHCODE_UNKNOWN)
            return ;            // no way!
    }

#if defined(DEBUG_SPI)
    // wait for end
    SPI_FlashWaitForEnd() ;
#endif // defined(DEBUG_SPI)

    while(flen) {
        // chip based
        switch(flashcode) {
        case FLASHCODE_ATMEL_AT45DB161D :
            buftx[0] = 0xd2 ;               // page read
            buftx[1] = bbegin >> 15 ;
            buftx[2] = ((bbegin >> 7) & 0xfc) | ((bbegin >> 8) & 1) ;
            buftx[3] = bbegin ;

            // attention to page boundary
            plen = MIN(FLASH_PAGESIZE - (bbegin & (FLASH_PAGESIZE-1)), flen) ;

            // read command
            // second TX buffer has no meaning, use dummy RAM pointer
            SPI_rtx2(buftx, bufrx, 8, _data, dst, plen) ;
            break ;

        case FLASHCODE_NUMONYX_M45PE16 :
            buftx[0] = 0x03 ;               // read
            buftx[1] = bbegin >> 16 ;
            buftx[2] = bbegin >> 8 ;
            buftx[3] = bbegin ;

            plen = flen ;       // read all

            // read command
            // second TX buffer has no meaning, use dummy RAM pointer
            SPI_rtx2(buftx, bufrx, 4, _data, dst, plen) ;
            //if (bbegin) pdebugt(1,"Read from 0x%08lx, len %d, val 0x%x", bbegin, plen, *(int *)(dst)) ;
            break ;
        }

        flen -= plen ;
        dst += plen ;
        bbegin += plen ;
    }

#if defined(DEBUG_SPI)
    SPI_FlashWaitForEnd() ;
#endif // defined(DEBUG_SPI)
}

//----------------------------------------------------------------------------
// SPI_FlashWrite
// WARNING: it is necessary to have a dummy 'dst' buffer for read-back after write

void SPI_FlashWrite(unsigned long bbegin, unsigned char *src, unsigned char *dst, int flen)
{
    unsigned char buftx[4] ;
    unsigned char bufrx[4] ;
    int plen = 0 ;
#if defined(CBUG) && defined(DEBUG_SPI)
	int lflen = flen ;
#endif

    // check if device is known
    if (flashcode == FLASHCODE_UNKNOWN) {
        SPI_FlashQuery() ;      // who are you?
        if (flashcode == FLASHCODE_UNKNOWN)
            return ;            // no way!
    }

#if defined(DEBUG_SPI)
    SPI_FlashWaitForEnd() ;
#endif // defined(DEBUG_SPI)

    while(flen) {
        // chip based
        switch(flashcode) {
        case FLASHCODE_ATMEL_AT45DB161D :
            buftx[0] = 0x53 ;               // page read to buffer 1
            buftx[1] = bbegin >> 15 ;
            buftx[2] = bbegin >> 7 ;
            buftx[3] = 0 ;

            // buffer 1 read command
            SPI_rtx2(buftx, bufrx, 4, NULL, NULL, 0) ;

            // attention to page boundary
            plen = MIN(FLASH_PAGESIZE - (bbegin & (FLASH_PAGESIZE-1)), flen) ;

            // wait for end of page read
            SPI_FlashWaitForEnd() ;

            buftx[0] = 0x84 ;               // write to buffer 1
            buftx[1] = 0 ;
            buftx[2] = (bbegin >> 8) & 1 ;
            buftx[3] = bbegin ;

            // buffer 1 write command
            SPI_rtx2(buftx, bufrx, 4, src, dst, plen) ;

            buftx[0] = 0x88 ;              // page write from buffer 1
            buftx[1] = bbegin >> 15 ;
            buftx[2] = bbegin >> 7 ;
            buftx[3] = 0 ;

            // buffer 1 to main memory write command
            SPI_rtx2(buftx, bufrx, 4, NULL, NULL, 0) ;
            break ;

        case FLASHCODE_NUMONYX_M45PE16 :
            // first of all enable write
            buftx[0] = 0x06 ;   // write enable
            SPI_rtx2(buftx, bufrx, 1, NULL, NULL, 0) ;

            buftx[0] = 0x02 ;               // page program
            buftx[1] = bbegin >> 16 ;
            buftx[2] = bbegin >> 8 ;
            buftx[3] = bbegin ;

            // attention to page boundary
            plen = MIN(256 - (bbegin & (256 - 1)), flen) ;

            // write command
            SPI_rtx2(buftx, bufrx, 4, src, dst, plen) ;
            //pdebugt(1,"Write from 0x%08lx, len %d", bbegin, plen) ;
            break ;
        }

        flen -= plen ;
        src += plen ;
        bbegin += plen ;

        // wait for end of page write
        SPI_FlashWaitForEnd() ;
    }
}

#endif // defined(USE_SPI_ON_ARM)

