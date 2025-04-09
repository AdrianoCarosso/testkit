// drv_nand.c - NAND driver tasks
//
//   Copyright (c) 1997-2013.
//   T.E.S.T. srl
//

//
// This module is provided as a NAND driver.
//

#include <stdio_console.h>
#include <stddef.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include <nand_ecc.h>

#include "extapi.h"

#include "assign.h"

#define NULLSEMA ((SEMA)0)

//#define PROVA_MARK 33L // blocco da dire con errore
//----------------------------------------------------------------------------
// only if we are well accepted

#ifdef USE_NANDFLASH_ON_ARM

unsigned char flashcode = FLASHCODE_NAND ; // FLASHCODE_UNKNOWN ;

#ifdef CBUG
#define USE_NAND_DEBUG
#endif // CBUG

#define NAND_NUM_BLOCKS (blockSize?(totSizeMB*((1024*1024)/blockSize)):0)      // it was fixed at 1024

// #define SET_CLE  {LPC_GPIO4->CLR = 0x1 ; LPC_GPIO4->SET = 0x2; }
// #define SET_ALE  {LPC_GPIO4->CLR = 0x2 ; LPC_GPIO4->SET = 0x1 ;}
// #define SET_DATA {LPC_GPIO4->CLR = 0x3 ;}

#define MYNOPBEGIN   {volatile int _i ; for(_i=0 ; _i<2 ; _i++) __NOP();}
#define MYNOPEND     {volatile int _i ; for(_i=0 ; _i<3 ; _i++) __NOP();}

#define SET_CLE  {MYNOPBEGIN; LPC_GPIO4->CLR = 0x1 ; LPC_GPIO4->SET = 0x2; MYNOPEND;}
#define SET_ALE  {MYNOPBEGIN; LPC_GPIO4->CLR = 0x2 ; LPC_GPIO4->SET = 0x1; MYNOPEND;}
#define SET_DATA {MYNOPBEGIN; LPC_GPIO4->CLR = 0x3 ;                       MYNOPEND;}

// #define K9FXX_READ_1            0x00
// #define K9FXX_READ_2            0x30
// 
// #define K9FXX_READ_RND_1        0x05
// #define K9FXX_READ_RND_2        0xe0
// 
// #define K9FXX_WRITE_RND         0x85
// 
// #define K9FXX_SET_ADDR_A        0x00
// #define K9FXX_SET_ADDR_B        0x01
// #define K9FXX_SET_ADDR_C        0x50
// #define K9FXX_READ_ID           0x90
// #define K9FXX_RESET             0xff
// #define K9FXX_BLOCK_PROGRAM_1   0x80
// #define K9FXX_BLOCK_PROGRAM_2   0x10
// #define K9FXX_BLOCK_ERASE_1     0x60
// #define K9FXX_BLOCK_ERASE_2     0xd0
// #define K9FXX_READ_STATUS       0x70
// #define K9FXX_BUSY              (1 << 6)
// #define K9FXX_OK                (1 << 0)

#define ID_MARKER_CODE (0xEC)
#define ID_SAMSUNG     (0xF1)

#define ID_PAGE_SZ_1KB (0x00)
#define ID_PAGE_SZ_2KB (0x01)
#define ID_PAGE_SZ_4KB (0x02)
#define ID_PAGE_SZ_8KB (0x03)

#define ID_BLOCK_SZ_64KB  (0x00)
#define ID_BLOCK_SZ_128KB (0x01)
#define ID_BLOCK_SZ_256KB (0x02)
#define ID_BLOCK_SZ_512KB (0x03)

#define ID_PAGE_SZ_1KB (0x00)
#define ID_PAGE_SZ_2KB (0x01)
#define ID_PAGE_SZ_4KB (0x02)
#define ID_PAGE_SZ_8KB (0x03)

#define ID_REDUND_SZ_8  (0x00)
#define ID_REDUND_SZ_16 (0x01)

//----------------------------------------------------------------------------
// check NAND ready
#ifdef CBUG
int nandelay = 0 ;
// Wait polling digital signal
#define WAIT_READY(A) {tickwait(2);while(!((LPC_GPIO2->PIN)&(1<<11)))nandelay++;}
#else   // CBUG
// Wait polling digital signal
#define WAIT_READY(A) {tickwait(2);while(!((LPC_GPIO2->PIN)&(1<<11)));}
#endif  // CBUG

// Alternatives - not used
// Wait polling internal register
//#define WAIT_READY(A) {nandWaitReady();}
// Wait fixed time
//#define WAIT_READY(A) {(tickwait(A);}

//----------------------------------------------------------------------------
// Local variables

static uint32_t pageSize   = 0 ;
static uint32_t blockSize  = 0 ;
static uint32_t reduntSize = 0 ;
static uint32_t totSizeMB  = 0 ;    // total size in Mega Bytes
static uint32_t totPages   = 0 ;

//----------------------------------------------------------------------------
// Local Functions

static inline uint8_t nandStatus(void)
{
uint8_t status = 0;

#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_READ_STATUS; // p4.1
	*PTRNAND_ALE = 0;					// p4.0
	status = *PTRNAND_DATA ;
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_READ_STATUS ;
	SET_ALE
	*PTRNAND_DATA = 0 ;
	SET_DATA
	status = *PTRNAND_DATA ;
#endif

	// remove bits not used
	return(status & 0xC1) ;
}

static inline void nandWaitReady(void)
{
	while( !(nandStatus() & (1<<6)) );
}

static inline void ConfigureDMAfromNAND(void *memdst, int size)
{
	// Memory to Memory DMA
	// Assign Memory source: NAND
	SET_DATA
	
	LPC_GPDMACH1->CSrcAddr = (uint32_t)(PTRNAND_DATA) ;
	// Assign Memory destination address
	LPC_GPDMACH1->CDestAddr = (uint32_t)(memdst) ;
	LPC_GPDMACH1->CControl = (size & 0xfff)     // transfer size
					| (1 << 12)         // source burst size (12 - 14) = 4
					| (0 << 15)         // destination burst size (15 - 17) = 1
					| (0 << 18)         // source width (18 - 20) = 8 bit
					| (2 << 21)         // destination width (21 - 23) = 32 bit
					| (0 << 24)         // source AHB select (24) = AHB 0
					| (0 << 25)         // destination AHB select (25) = AHB 0
					| (0 << 26)         // source increment (26) = no increment
					| (1 << 27)         // destination increment (27) = increment
					| (0 << 28)         // mode select (28) = access in user mode
					| (0 << 29)         // (29) = access not bufferable
					| (0 << 30)         // (30) = access not cacheable
					| (1 << 31);        // terminal count interrupt enabled
	// Assign Linker List Item value
	LPC_GPDMACH1->CLLI = 0 ;    // one shot transfer

	// reset the Interrupt status of channel 1
//    LPC_GPDMA->IntTCClear = (1<<1) ;
//    LPC_GPDMA->IntErrClr = (1<<1) ;

	// Configure DMA Channel, disable Error Counter, enable Terminate counter
	LPC_GPDMACH1->CConfig = 0         // enable later
					| (0 << 1)          // source peripheral (1 - 5) = none
					| (0 << 6)          // destination peripheral (6 - 10) = none
					| (0 << 11)         // flow control (11 - 13) = memory to memory
					| (0 << 14)         // (14) = mask out error interrupt
					| (1 << 15)         // (15) = NO mask out terminal count interrupt
					| (0 << 16)         // (16) = no locked transfers
					| (0 << 18);        // (27) = no HALT
}

static inline void ConfigureDMAtoNAND(void *memsrc, int size)
{
	// Memory to Memory DMA
	// Assign Memory source address
	LPC_GPDMACH1->CSrcAddr = (uint32_t)(memsrc) ;
	// Assign Memory destination NAND
	SET_DATA
	LPC_GPDMACH1->CDestAddr = (uint32_t)(PTRNAND_DATA) ;
//    LPC_GPDMACH1->CControl = (size & 0xfff)     // transfer size
	LPC_GPDMACH1->CControl = ((size>>2) & 0xfff)    // transfer size
					| (0 << 12)         // source burst size (12 - 14) = 1
					| (1 << 15)         // destination burst size (15 - 17) = 4
					| (2 << 18)         // source width (18 - 20) = 32 bit
					| (0 << 21)         // destination width (21 - 23) = 8 bit
					| (0 << 24)         // source AHB select (24) = AHB 0
					| (0 << 25)         // destination AHB select (25) = AHB 0
					| (1 << 26)         // source increment (26) = increment
					| (0 << 27)         // destination increment (27) = no increment
					| (0 << 28)         // mode select (28) = access in user mode
					| (0 << 29)         // (29) = access not bufferable
					| (0 << 30)         // (30) = access not cacheable
					| (1 << 31);        // terminal count interrupt enabled
	// Assign Linker List Item value
	LPC_GPDMACH1->CLLI = 0 ;    // one shot transfer

	// reset the Interrupt status of channel 1
//    LPC_GPDMA->IntTCClear = (1<<1) ;
//    LPC_GPDMA->IntErrClr = (1<<1) ;

	// Configure DMA Channel, disable Error Counter, enable Terminate counter
	LPC_GPDMACH1->CConfig = 1         // enable now
					| (0 << 1)          // source peripheral (1 - 5) = none
					| (0 << 6)          // destination peripheral (6 - 10) = none
					| (0 << 11)         // flow control (11 - 13) = memory to memory
					| (0 << 14)         // (14) = mask out error interrupt
					| (1 << 15)         // (15) = NO mask out terminal count interrupt
					| (0 << 16)         // (16) = no locked transfers
					| (0 << 18);        // (27) = no HALT
}

//----------------------------------------------------------------------------
// Interrupt routine for DMA

#ifdef USE_DMA_NAND
void DMA_IRQHandler(void)
{
	// check for DMA transfer end
	if (LPC_GPDMA->IntTCStat & (1<<1)) {
		LPC_GPDMA->IntTCClear = (1<<1) ;
		KS_ISRsignal(NANDSEM) ;
		ASK_CONTEXTSWITCH ;                 // set PendSV
	}
}
#endif // USE_DMA_NAND

//----------------------------------------------------------------------------
// Public Functions

/******************************************************************************
 *
 * Description:
 *    Initialize the NAND Flash
 *
 * Returns:
 *    TRUE if initialization successful; otherwise FALSE
 *
 *****************************************************************************/
int nandstart(void)
{
uint8_t volatile v ;

	// same as SDRAM
	LPC_SC->PCONP |= CLKPWR_PCONP_PCEMC ;
	LPC_EMC->Control = 0x00000001 ;
	LPC_EMC->Config  = 0x00000000 ;

	// pinConfig(); done by DIO

	LPC_EMC->StaticConfig1   = 0x00000080 ;

	LPC_EMC->StaticWaitWen1  = 0x00000002 ;
	LPC_EMC->StaticWaitOen1  = 0x00000002 ;
	LPC_EMC->StaticWaitRd1   = 0x00000008 ;
	LPC_EMC->StaticWaitPage1 = 0x0000001f ;
	LPC_EMC->StaticWaitWr1   = 0x00000008 ;
	LPC_EMC->StaticWaitTurn1 = 0x0000000f ;

#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_READ_ID ;
	*PTRNAND_ALE = 0 ;
	v = *PTRNAND_DATA ;     // byte 1 - marker code
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_READ_ID ;
	SET_ALE
	*PTRNAND_DATA = 0 ;
	SET_DATA
	v = *PTRNAND_DATA ;     // byte 1 - marker code
#endif

#ifdef USE_NAND_DEBUG
		printf("NAND byte1 %02x\n", v) ;
#endif // USE_NAND_DEBUG
	if (v != ID_MARKER_CODE) {
#ifdef USE_NAND_DEBUG
		printf("Unknown marker %02x\n", v) ;
#endif // USE_NAND_DEBUG
		return(FALSE) ;
	}

	v = *PTRNAND_DATA ; // byte 2 - Device Code
#ifdef USE_NAND_DEBUG
		printf("NAND byte2 %02x\n", v) ;
#endif // USE_NAND_DEBUG
	if (v != ID_SAMSUNG) {
#ifdef USE_NAND_DEBUG
		printf("Unknown device %02x\n", v) ;
#endif // USE_NAND_DEBUG
// 		v = *PTRNAND_DATA ; // byte 3 - Internal Chip Number, Cell Type, Number of Simultaneously Programmed Pages, Etc
// #ifdef USE_NAND_DEBUG
// 		printf("NAND byte3 %02x\n", v) ;
// #endif // USE_NAND_DEBUG
// 		v = *PTRNAND_DATA ; // byte 4 - Page Size, Block Size,Redundant Area Size, Organization, Serial Access Minimum
// #ifdef USE_NAND_DEBUG
// 		printf("NAND byte4 %02x\n", v) ;
// #endif // USE_NAND_DEBUG
		return(FALSE) ;
	}

	v = *PTRNAND_DATA ; // byte 3 - Internal Chip Number, Cell Type, Number of Simultaneously Programmed Pages, Etc
#ifdef USE_NAND_DEBUG
		printf("NAND byte3 %02x\n", v) ;
#endif // USE_NAND_DEBUG

	v = *PTRNAND_DATA ; // byte 4 - Page Size, Block Size,Redundant Area Size, Organization, Serial Access Minimum
#ifdef USE_NAND_DEBUG
		printf("NAND byte4 %02x\n", v) ;
#endif // USE_NAND_DEBUG
	pageSize   = 1024 * (1 << (v & 0x03)) ;
	blockSize  = 64*1024 * (1 << ((v >> 4) & 0x03)) ;
	reduntSize = (8 * (1 << ((v >> 2) & 0x1))) * (pageSize/512) ;

	v = *PTRNAND_DATA ; // byte 5 - Plane Number, Plane Size
	totSizeMB = (64/8) << ((v >> 4) & 7) ;  // plane size in MB
	totSizeMB <<= ((v >> 2) & 3) ;          // num of planes

	totPages = (1024*1024)/pageSize ;
	totPages *= totSizeMB ;

#ifdef USE_NAND_DEBUG
	printf("NAND %ld MByte: pageSize=%ld, blockSize=%ld, reduntSize=%ld\n", totSizeMB, pageSize, blockSize, reduntSize) ;
#endif // USE_NAND_DEBUG

#ifdef USE_DMA_NAND
	// If enabled, use channel -1-
	// enable DMA - done by main.c
//    LPC_GPDMA->Config = 0x01 ;     // enable

	// reset the Interrupt status of channel 1
	LPC_GPDMA->IntTCClear = (1<<1) ;
	LPC_GPDMA->IntErrClr = (1<<1) ;

	NVIC_EnableIRQ(DMA_IRQn) ;
	NVIC_SetPriority(DMA_IRQn, DMA_INTERRUPT_LEVEL) ;
#endif // USE_DMA_NAND

return(TRUE) ;
}

//----------------------------------------------------------------------------
// nandstop()

void nandstop(void)
{
	// do nothing if code upgrade
	if (EKS_EnqShutdown() == SD_CODEUPGRADE)
		return ;

	// do nothing if never enabled
	if (!(LPC_SC->PCONP & CLKPWR_PCONP_PCEMC)) return ;

#ifdef USE_DMA_NAND
	// disable DMA channel
	LPC_GPDMACH1->CConfig &= (~1) ;
	// done by main.c
	// LPC_GPDMA->Config &= (~1) ;
#endif // USE_DMA_NAND

	// same as sdramstop()
	LPC_EMC->Control = 0 ;    // disable
	LPC_SC->PCONP   &= ~CLKPWR_PCONP_PCEMC ;
	totSizeMB = 0 ;
}

/******************************************************************************
 *
 * Description:
 *    Get total blocks of the NAND flash
 *
 * Returns:
 *    NAND blocks
 *
 *****************************************************************************/
uint32_t nand_getBlocks(void)
{
	return(NAND_NUM_BLOCKS) ;
}

/******************************************************************************
 *
 * Description:
 *    Get the size of the NAND flash in MegaBytes
 *
 * Returns:
 *    NAND size in MegaBytes
 *
 *****************************************************************************/
uint32_t nand_getSizeMB(void)
{
	return(totSizeMB) ;
}

/******************************************************************************
 *
 * Description:
 *    Get the page size of the NAND flash
 *
 * Returns:
 *    page size in bytes
 *
 *****************************************************************************/
uint32_t nand_getPageSize(void)
{
	return(pageSize) ;
}

/******************************************************************************
 *
 * Description:
 *    Get the block size of the NAND flash
 *
 * Returns:
 *    block size in bytes
 *
 *****************************************************************************/
uint32_t nand_getBlockSize(void)
{
	return(blockSize) ;
}

/******************************************************************************
 *
 * Description:
 *    Get the redundant (spare) size per page
 *
 * Returns:
 *    redundant/spare size in bytes
 *
 *****************************************************************************/
uint32_t nand_getRedundantSize(void)
{
	return(reduntSize) ;
}

/******************************************************************************
 *
 * Description:
 *    Check if a block is valid
 *
 * Returns:
 *    TRUE if the block is valid; otherwise FALSE
 *
 *****************************************************************************/
int nand_isBlockValid(uint32_t block)
{
uint32_t addr = 0;
uint32_t page = 0;

	// do nothing if never enabled
	if (!(LPC_SC->PCONP & CLKPWR_PCONP_PCEMC)) return FALSE ;

	if (block >= NAND_NUM_BLOCKS) {
		return FALSE;
	}

	addr = block * (blockSize/pageSize);

	/*
		* Check page 0 and page 1 in each block. If the first byte
		* in the spare area (of either page 0 or page 1) is != 0xFF
		* the block is invalid.
		*/

/*    WAIT_READY(25) ;  */

	for (page = 0; page < 2; page++) {
		addr += page;

#ifndef NAND_ONEADDR
		*PTRNAND_CLE = K9FXX_READ_1;
		*PTRNAND_ALE = (uint8_t) (pageSize & 0x00FF);
		*PTRNAND_ALE = (uint8_t)((pageSize & 0xFF00) >> 8);
		*PTRNAND_ALE = (uint8_t)((addr & 0x00FF));
		*PTRNAND_ALE = (uint8_t)((addr & 0xFF00) >> 8);
		*PTRNAND_CLE = K9FXX_READ_2 ;
#else
		SET_CLE
		*PTRNAND_DATA = K9FXX_READ_1 ;
		SET_ALE
		*PTRNAND_DATA = (uint8_t) (pageSize & 0x00FF);
		*PTRNAND_DATA = (uint8_t)((pageSize & 0xFF00) >> 8);
		*PTRNAND_DATA = (uint8_t)((addr & 0x00FF));
		*PTRNAND_DATA = (uint8_t)((addr & 0xFF00) >> 8);
		SET_CLE
		*PTRNAND_DATA = K9FXX_READ_2 ;
#endif

		WAIT_READY(25) ;

		SET_DATA
		if (*PTRNAND_DATA != 0xFF) {
			return FALSE;
		}
	}

	return(TRUE) ;
}

#ifdef MARKBADBLOCKS
/******************************************************************************
 *
 * Description:
 *    Declare block as invalid
 *
 * Params:
 *    address - full address (byte pointer) of any byte within block
 *
 * Returns:
 *    none
 *
 *****************************************************************************/
void nand_invalidateBlock(uint64_t addr)
{
    uint64_t blockaddr ;
    int page ;

	// do nothing if never enabled
	if (!(LPC_SC->PCONP & CLKPWR_PCONP_PCEMC)) return ;
	
    // evaluate begin of block
    blockaddr = addr & (~(blockSize -1)) ;

    // .........................................................
    // start writing from correct point

    for(page=0 ; page<2 ; page++) {

#ifndef NAND_ONEADDR
        *PTRNAND_CLE = K9FXX_BLOCK_PROGRAM_1 ;
        *PTRNAND_ALE = (uint8_t) (pageSize & 0x00FF) ; // offset lo
        *PTRNAND_ALE = (uint8_t)((pageSize & 0xFF00) >> 8) ;    // offset hi
        *PTRNAND_ALE = (uint8_t)(blockaddr>>11) ;               // page   lo
        *PTRNAND_ALE = (uint8_t)((blockaddr>>11) >> 8) ;        //        hi
#else
        SET_CLE ;
        *PTRNAND_DATA = K9FXX_BLOCK_PROGRAM_1 ;
        SET_ALE ;
        *PTRNAND_ALE = (uint8_t) (pageSize & 0x00FF) ; // offset lo
        *PTRNAND_ALE = (uint8_t)((pageSize & 0xFF00) >> 8) ;    // offset hi
        *PTRNAND_DATA = (uint8_t)(blockaddr>>11) ;              // page   lo
        *PTRNAND_DATA = (uint8_t)((blockaddr>>11) >> 8) ;       //        hi
#endif

        // mark first 2 bytes of OOB: bad block
        SET_DATA ;
        *PTRNAND_DATA = 0 ;
        *PTRNAND_DATA = 0 ;

#ifndef NAND_ONEADDR
        *PTRNAND_CLE = K9FXX_BLOCK_PROGRAM_2 ;
#else
        SET_CLE ;
        *PTRNAND_DATA = K9FXX_BLOCK_PROGRAM_2 ;
#endif
        WAIT_READY(700) ;

        blockaddr += pageSize ;
    }
}
#endif // MARKBADBLOCKS
#ifndef MTS_CODE
/******************************************************************************
 *
 * Description:
 *    Read a page from the NAND memory
 *
 * Params:
 *    page  - page within block to read from
 *    pageStruct - data+OOB are copied to this buffer.
 *
 * Returns:
 *  -4  unrecoverable ECC error
 *  -3  DMA error
 *  -2  timeout
 *  -1  parameter error
 *  0   read successful
 *  1   1 bit error recovered by ECC
 *  2   2 bit errors recovered by ECC
 *  3   3 bit errors recovered by ECC
 *  4   4 bit errors recovered by ECC
 *  5   probably empty page - ECC at 0xFF's
 *
 *
 *****************************************************************************/
int nand_readPage(uint32_t page, struct _NAND_DATA * pageStruct /*uint8_t* pageBuf*/)
{
int rv ;        // return value

	// sanity checks
	if (page >= totPages) {
		return(-1) ;
	}
	if (sizeof(pageStruct->page) != pageSize) {
		return(-1) ;
	}
	if (sizeof(pageStruct->pageoob) != reduntSize) {
		return(-1) ;
	}

	// .........................................................
	// Always reading from start of a page address.
	// This means that the column address is always 0.

#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_READ_1 ;
	*PTRNAND_ALE = 0;
	*PTRNAND_ALE = 0;
	*PTRNAND_ALE = (uint8_t)(page) ;
	*PTRNAND_ALE = (uint8_t)(page >> 8) ;
	*PTRNAND_CLE = K9FXX_READ_2 ;
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_READ_1 ;
	SET_ALE
	*PTRNAND_DATA = 0 ;
	*PTRNAND_DATA = 0 ;
	*PTRNAND_DATA = (uint8_t)(page) ;
	*PTRNAND_DATA = (uint8_t)(page >> 8) ;
	SET_CLE
	*PTRNAND_DATA = K9FXX_READ_2 ;
#endif
	// .........................................................
	// prepare transfer
#ifdef USE_DMA_NAND
	ConfigureDMAfromNAND(pageStruct, pageSize + reduntSize) ;
#else // USE_DMA_NAND
#endif // USE_DMA_NAND

	// .........................................................
	// wait internal NAND activity
	WAIT_READY(25) ;
	/*printf("NAND page %ld/%ld, delay %d\n", page, totPages, nandelay) ; */

	// .........................................................
	// make transfer
#ifdef USE_DMA_NAND
	// Enable channel
	LPC_GPDMACH1->CConfig |= 1 ;

	// wait until end
	//while(LPC_GPDMACH1->CConfig & 1) ;
	if (KS_waitt(NANDSEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
#ifdef CBUG
		pdebugt(1,"NAND/r dma TIMEOUT") ;
#endif // CBUG
		return(-2) ;
	}
	if (LPC_GPDMA->IntErrStat & (1<<1)) {
		LPC_GPDMA->IntErrClr = (1<<1) ;
#ifdef CBUG
		pdebugt(1,"NAND/r dma ERROR") ;
#endif // CBUG
		return(-3) ;
	}

#else // USE_DMA_NAND
	// transfer without DMA
	{
		int i ;
		unsigned char *p = pageStruct ;
		for (i = 0; i < (pageSize + reduntSize); i++) *p++ = *PTRNAND_DATA ;
	}
#endif // USE_DMA_NAND

	// check if page is empty - based on ECC values at 0xFF's
	if ( (pageStruct->pageoob.ecc[0] == 0xffffffff) &&
			(pageStruct->pageoob.ecc[1] == 0xffffffff) &&
			(pageStruct->pageoob.ecc[2] == 0xffffffff) &&
			(pageStruct->pageoob.ecc[3] == 0xffffffff) ) {
		return(5) ; // probably empty page
	}

	// check ECC
	{
		uint32_t myecc ;
		int i, rc ;

		rv = 0 ;    // defualt no error detected

		// split 2k page in 4 semi-pages of 512 each
		for(i=0 ; i<4 ; i++) {
			// evaluate ECC of read data
			nand_calculate_ecc(&(pageStruct->page.data[i*512]), 512, (unsigned char *)(&myecc), NULL) ;
			// check correctness - in case of 1 bit error, correct it in buffer
			rc = nand_correct_data(&(pageStruct->page.data[i*512]),
									(unsigned char *)(&pageStruct->pageoob.ecc[i]),  /* from chip */
									(unsigned char *)(&myecc),                       /* evaluated now */
									512, NULL) ;
			if (rc < 0) {
				return(-4) ;    // unrecoverable ECC error
			}

			if (rc > 0) rv++ ;  // one more recovered
		}
	}

	//printf("NAND page %ld/%ld, delay %d\n", page, totPages, nandelay) ;
	return(rv) ;
}
#endif // ifndef MTS_CODE
/******************************************************************************
 *
 * Description:
 *    Read 512 Byte (1/4 page) from the NAND memory
 *
 * Params:
 *    address - full address (byte pointer)
 *    buffer  - 512 byte user buffer
 *
 * Returns:
 *  -4  unrecoverable ECC error
 *  -3  DMA error
 *  -2  timeout
 *  -1  parameter error
 *  0   read successful
 *  1   1 bit error recovered by ECC
 *  2   2 bit errors recovered by ECC
 *  3   3 bit errors recovered by ECC
 *  4   4 bit errors recovered by ECC
 *  5   probably empty page - ECC at 0xFF's
 *
 *
 *****************************************************************************/
int nand_read512(uint64_t addr, void * userbuffer)
{
int rv ;        // return value
int subpage ;
uint32_t specc ;
#ifdef MTS_CODE
uint32_t nblock ;

	nblock = addr / blockSize ;
	for(subpage=0;subpage<Nand_save.nand_nrbb;subpage++){
		if (NAND_badblocks[subpage]==nblock) return(-4) ;
	}
#endif

	// do nothing if never enabled
	if (!(LPC_SC->PCONP & CLKPWR_PCONP_PCEMC)) return(-1) ;

	// sanity checks
	if (addr >= (totSizeMB*1024*1024)) {
		return(-1) ;    // out of range
	}
	if (addr & 0x1ff) {
		return(-1) ;    // bad 512 byte alignment
	}
	subpage = (addr>>9) & 0x3 ; // supbage = 0..3

	if (userbuffer) {
		// .........................................................
		// start reading from correct point

#ifndef NAND_ONEADDR
		*PTRNAND_CLE = K9FXX_READ_1 ;
		*PTRNAND_ALE = (uint8_t)(addr & 0x7ff) ;        // offset lo
		*PTRNAND_ALE = (uint8_t)((addr & 0x7ff)>>8) ;   //        hi
		*PTRNAND_ALE = (uint8_t)(addr>>11) ;            // page   lo
		*PTRNAND_ALE = (uint8_t)((addr>>11) >> 8) ;     //        hi
		*PTRNAND_CLE = K9FXX_READ_2 ;
#else
		SET_CLE
		*PTRNAND_DATA = K9FXX_READ_1 ;
		SET_ALE
		*PTRNAND_DATA = (uint8_t)(addr & 0x7ff) ;        // offset lo
		*PTRNAND_DATA = (uint8_t)((addr & 0x7ff)>>8) ;   //        hi
		*PTRNAND_DATA = (uint8_t)(addr>>11) ;            // page   lo
		*PTRNAND_DATA = (uint8_t)((addr>>11) >> 8) ;     //        hi
		SET_CLE
		*PTRNAND_DATA = K9FXX_READ_2 ;
#endif
		// .........................................................
		// prepare transfer
#ifdef USE_DMA_NAND
		ConfigureDMAfromNAND(userbuffer, 512) ;
#else // USE_DMA_NAND
#endif // USE_DMA_NAND

		// .........................................................
		// wait internal NAND activity
		WAIT_READY(25) ;
		/*printf("NAND page %ld/%ld, delay %d\n", page, totPages, nandelay) ; */

		// .........................................................
		// make transfer
#ifdef USE_DMA_NAND
		// Enable channel
		LPC_GPDMACH1->CConfig |= 1 ;

		// wait until end
		//while(LPC_GPDMACH1->CConfig & 1) ;
		if (KS_waitt(NANDSEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
#ifdef CBUG
			pdebugt(1,"NAND/r dma TIMEOUT") ;
#endif // CBUG
			return(-2) ;
		}
		if (LPC_GPDMA->IntErrStat & (1<<1)) {
			LPC_GPDMA->IntErrClr = (1<<1) ;
#ifdef CBUG
			pdebugt(1,"NAND/r dma ERROR") ;
#endif // CBUG
			return(-3) ;
		}

#else // USE_DMA_NAND
		// transfer without DMA
#ifdef NAND_ONEADDR
		SET_DATA
#endif
		{
			int i ;
			unsigned char *p = userbuffer ;
			for (i=0 ; i<512 ; i++) *p++ = *PTRNAND_DATA ;
		}
#endif // USE_DMA_NAND

		// read proper ECC
#ifndef NAND_ONEADDR
		*PTRNAND_CLE = K9FXX_READ_RND_1 ;
		*PTRNAND_ALE = (uint8_t)offsetof(struct _NAND_DATA, pageoob.ecc[subpage]) ;
		*PTRNAND_ALE = (uint8_t)((offsetof(struct _NAND_DATA, pageoob.ecc[subpage])) >> 8);
		*PTRNAND_CLE = K9FXX_READ_RND_2 ;
#else
		SET_CLE
		*PTRNAND_DATA = K9FXX_READ_RND_1 ;
		SET_ALE
		*PTRNAND_DATA = (uint8_t)offsetof(struct _NAND_DATA, pageoob.ecc[subpage]) ;
        *PTRNAND_DATA = (uint8_t)((offsetof(struct _NAND_DATA, pageoob.ecc[subpage])) >> 8);
		SET_CLE
		*PTRNAND_DATA = K9FXX_READ_RND_2 ;
#endif
	} else {    // if userbuffer

		// .........................................................
		// no buffer, start reading from correct point

#ifndef NAND_ONEADDR
		*PTRNAND_CLE = K9FXX_READ_1 ;
		*PTRNAND_ALE = (uint8_t)offsetof(struct _NAND_DATA, pageoob.ecc[subpage]) ;
		*PTRNAND_ALE = (uint8_t)((offsetof(struct _NAND_DATA, pageoob.ecc[subpage])) >> 8);
		*PTRNAND_ALE = (uint8_t)(addr>>11) ;            // page   lo
		*PTRNAND_ALE = (uint8_t)((addr>>11) >> 8) ;     //        hi
		*PTRNAND_CLE = K9FXX_READ_2 ;
#else
		SET_CLE
		*PTRNAND_DATA = K9FXX_READ_1 ;
		SET_ALE
		*PTRNAND_DATA = (uint8_t)offsetof(struct _NAND_DATA, pageoob.ecc[subpage]) ;
		*PTRNAND_DATA = (uint8_t)((offsetof(struct _NAND_DATA, pageoob.ecc[subpage])) >> 8);
		*PTRNAND_DATA = (uint8_t)(addr>>11) ;            // page   lo
		*PTRNAND_DATA = (uint8_t)((addr>>11) >> 8) ;     //        hi
		SET_CLE
		*PTRNAND_DATA = K9FXX_READ_2 ;
#endif
		// .........................................................
		// wait internal NAND activity
		WAIT_READY(25) ;
	}

#ifdef NAND_ONEADDR
	SET_DATA
#endif
	*((uint8_t *)(&specc) + 0) = *PTRNAND_DATA ;
	*((uint8_t *)(&specc) + 1) = *PTRNAND_DATA ;
	*((uint8_t *)(&specc) + 2) = *PTRNAND_DATA ;
	*((uint8_t *)(&specc) + 3) = *PTRNAND_DATA ;  /* even if ECC is 3 byte only */

	// check if subpage is empty - based on ECC values at 0xFF's
	if ( specc == 0xffffffff ) {
		return(5) ; // probably empty page
	}

	// check ECC
	rv = 0 ;    // default no error detected
	if (userbuffer) {
		uint32_t myecc ;
		int rc ;

		// evaluate ECC of read data
		nand_calculate_ecc(userbuffer, 512, (unsigned char *)(&myecc), NULL) ;
		// check correctness - in case of 1 bit error, correct it in buffer
		rc = nand_correct_data(userbuffer,
								(unsigned char *)(&specc),      /* from chip */
								(unsigned char *)(&myecc),      /* evaluated now */
								512, NULL) ;

		if (rc < 0) {
			return(-4) ;    // unrecoverable ECC error
		}

		if (rc > 0) rv++ ;  // one more recovered
	}

#ifdef CBUG
	if (rv) printf("NAND addr %llx, return %d\n", addr, rv) ;
#endif
	return(rv) ;
}

#ifndef MTS_CODE
/******************************************************************************
 *
 * Description:
 *    Write a page of data to the NAND memory
 *
 * Params:
 *    page  - page within block to write to
 *    pageStruct - data+OOB are copied from this buffer.
 *
 * Returns:
 *  -4  write error from NAND chip
 *  -3  DMA error
 *  -2  timeout
 *  -1  parameter error
 *  0   write successful
 *
 *****************************************************************************/
int nand_writePage(uint32_t page, struct _NAND_DATA * pageStruct /*uint8_t* pageBuf*/)
{
int i ;

	// sanity checks
	if (page >= totPages) {
		return(-1) ;
	}
	if (sizeof(pageStruct->page) != pageSize) {
		return(-1) ;
	}
	if (sizeof(pageStruct->pageoob) != reduntSize) {
		return(-1) ;
	}

#ifdef CBUG_
	pdebugt(1,"NAND req write 0x%lx", page) ;
#endif // CBUG
	// .........................................................
	// split 2k page in 4 semi-pages of 512 each for ECC calculation

	for(i=0 ; i<4 ; i++) {
		pageStruct->pageoob.ecc[i] = 0 ;    // preset 4th byte at 0
		// evaluate ECC
		nand_calculate_ecc(&(pageStruct->page.data[i*512]),
							512,
							(unsigned char *)(&pageStruct->pageoob.ecc[i]), NULL) ;
	}

	// .........................................................
	// Always writing to start of a page address.
	// This means that the column address is always 0.

#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_BLOCK_PROGRAM_1;
	*PTRNAND_ALE = 0;
	*PTRNAND_ALE = 0;
	*PTRNAND_ALE = (uint8_t)(page) ;
	*PTRNAND_ALE = (uint8_t)(page >> 8) ;
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_BLOCK_PROGRAM_1 ;
	SET_ALE
	*PTRNAND_DATA = 0 ;
	*PTRNAND_DATA = 0 ;
	*PTRNAND_DATA = (uint8_t)(page) ;
	*PTRNAND_DATA = (uint8_t)(page >> 8) ;
#endif
    // .........................................................
    // prepare transfer
#ifdef USE_DMA_NAND

	ConfigureDMAtoNAND(pageStruct, pageSize + reduntSize) ;
	// wait until end
	//while(LPC_GPDMACH1->CConfig & 1) ;
	if (KS_waitt(NANDSEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
#ifdef CBUG
		pdebugt(1,"NAND/w dma TIMEOUT") ;
#endif // CBUG
		return(-2) ;
	}
	if (LPC_GPDMA->IntErrStat & (1<<1)) {
		LPC_GPDMA->IntErrClr = (1<<1) ;
#ifdef CBUG
		pdebugt(1,"NAND/w dma ERROR") ;
#endif // CBUG
		return(-3) ;
	}

#else // USE_DMA_NAND

    // transfer without DMA
	SET_DATA
	{
		int i ;
		unsigned char *p = pageStruct ;
		for(i=0 ; i<(pageSize + reduntSize) ; i++) *PTRNAND_DATA = *p++;
	}

#endif // USE_DMA_NAND

#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_BLOCK_PROGRAM_2;
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_BLOCK_PROGRAM_2 ;
#endif

	WAIT_READY(700) ;
	i = nandStatus() ;
#ifdef CBUG
	if (i & 0x1) pdebugt(1,"NAND writePage ERROR") ;
#endif // CBUG

#ifdef MARKBADBLOCKS
	if (i & 0x01) nand_invalidateBlock( (addr ) ; // & ~(blockSize-1) ) ;
#endif

	return((i & 0x01) ? -4 : 0) ;
}
#endif // ifndef MTS_CODE

/******************************************************************************
 *
 * Description:
 *    Write 512 byte (1/4 page) of data to the NAND memory
 *
 * Params:
 *    address - full address (byte pointer)
 *    buffer  - 512 byte user buffer
 *
 * Returns:
 *  -4  write error from NAND chip
 *  -3  DMA error
 *  -2  timeout
 *  -1  parameter error
 *  0   write successful
 *
 *****************************************************************************/
int nand_write512(uint64_t addr, void * userbuffer)
{
int subpage ;
uint32_t specc ;
#ifdef MTS_CODE
uint32_t nblock ;

	nblock = addr / blockSize ;
	for(subpage=0;subpage<Nand_save.nand_nrbb;subpage++){
		if (NAND_badblocks[subpage]==nblock) return(-4) ;
	}
#endif

	// do nothing if never enabled
	if (!(LPC_SC->PCONP & CLKPWR_PCONP_PCEMC)) return(-1) ;
	
	// sanity checks
	if (addr >= (totSizeMB*1024*1024)) {
		return(-1) ;    // out of range
	}
	if (addr & 0x1ff) {
		return(-1) ;    // bad 512 byte alignment
	}
	subpage = (addr>>9) & 0x3 ; // supbage = 0..3

#ifdef CBUG_
	if (par71) pdebugt(1,"NAND req write512 0x%llx", addr) ;
#endif // CBUG
	// .........................................................
	// build ECC for correct 1/4 page

	nand_calculate_ecc(userbuffer,
						512,
						(unsigned char *)(&specc), NULL) ;

	// .........................................................
	// start writing from correct point

#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_BLOCK_PROGRAM_1 ;
	*PTRNAND_ALE = (uint8_t)(addr & 0x7ff) ;        // offset lo
	*PTRNAND_ALE = (uint8_t)((addr & 0x7ff)>>8) ;   //        hi
	*PTRNAND_ALE = (uint8_t)(addr>>11) ;            // page   lo
	*PTRNAND_ALE = (uint8_t)((addr>>11) >> 8) ;     //        hi
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_BLOCK_PROGRAM_1 ;
	SET_ALE
	*PTRNAND_DATA = (uint8_t)(addr & 0x7ff) ;        // offset lo
	*PTRNAND_DATA = (uint8_t)((addr & 0x7ff)>>8) ;   //        hi
	*PTRNAND_DATA = (uint8_t)(addr>>11) ;            // page   lo
	*PTRNAND_DATA = (uint8_t)((addr>>11) >> 8) ;     //        hi
#endif
    // .........................................................
    // prepare transfer
#ifdef USE_DMA_NAND

	ConfigureDMAtoNAND(userbuffer, 512) ;
	// wait until end
	//while(LPC_GPDMACH1->CConfig & 1) ;
	if (KS_waitt(NANDSEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
#ifdef CBUG
		pdebugt(1,"NAND/w dma TIMEOUT") ;
#endif // CBUG
		return(-2) ;
	}
	if (LPC_GPDMA->IntErrStat & (1<<1)) {
		LPC_GPDMA->IntErrClr = (1<<1) ;
#ifdef CBUG
		pdebugt(1,"NAND/w dma ERROR") ;
#endif // CBUG
		return(-3) ;
	}

#else // USE_DMA_NAND

	// transfer without DMA
	SET_DATA
	{
		int i ;
		unsigned char *p = userbuffer ;
		for(i=0 ; i<512 ; i++) *PTRNAND_DATA = *p++;
	}

#endif // USE_DMA_NAND

    // write proper ECC
#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_WRITE_RND ;
	*PTRNAND_ALE = (uint8_t)offsetof(struct _NAND_DATA, pageoob.ecc[subpage]) ;
	*PTRNAND_ALE = (uint8_t)((offsetof(struct _NAND_DATA, pageoob.ecc[subpage])) >> 8);
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_WRITE_RND ;
	SET_ALE
	*PTRNAND_DATA = (uint8_t)offsetof(struct _NAND_DATA, pageoob.ecc[subpage]) ;
	*PTRNAND_DATA = (uint8_t)((offsetof(struct _NAND_DATA, pageoob.ecc[subpage])) >> 8);
	SET_DATA
#endif

	*PTRNAND_DATA = *((uint8_t *)(&specc) + 0) ;
	*PTRNAND_DATA = *((uint8_t *)(&specc) + 1) ;
	*PTRNAND_DATA = *((uint8_t *)(&specc) + 2) ;
	*PTRNAND_DATA = 0 ;     /* ECC is 3 bytes only */

#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_BLOCK_PROGRAM_2;
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_BLOCK_PROGRAM_2 ;
#endif
	WAIT_READY(700) ;

	subpage = nandStatus() ;
#ifdef CBUG
	if (subpage & 0x1) pdebugt(1,"NAND write512 ERROR") ;
#endif // CBUG

// #ifdef PROVA_MARK // prova di falso blocco rovinato
// 	if (nblock==PROVA_MARK) subpage = 0x01 ;
// #endif

#ifdef MARKBADBLOCKS
	if (subpage & 0x01) nand_invalidateBlock( addr ) ; // & ~(blockSize-1) ) ;
#endif
#ifdef MTS_CODE
	if (subpage & 0x01) {
		if (Nand_save.nand_nrbb<NANDMAXBADBLOCK){
			NAND_badblocks[Nand_save.nand_nrbb++] = nblock ;
		}else if(NAND_badblocks[NANDMAXBADBLOCK-1]>0)
			NAND_badblocks[NANDMAXBADBLOCK-1] *= -1 ;
	}
#endif

	return((subpage & 0x01) ? -4 : 0) ;
}

/******************************************************************************
 *
 * Description:
 *    Erase a block
 *
 * Params:
 *    block - block number to erase
 *
 * Returns:
 *  -4  erase error from NAND chip
 *  -1  parameter error
 *  0   write successful
 *
 *****************************************************************************/
int nand_eraseBlock(uint32_t block)
{
uint32_t addr = 0;
int rv ;

	// do nothing if never enabled
	if (!(LPC_SC->PCONP & CLKPWR_PCONP_PCEMC)) return(-1) ;

#ifdef MTS_CODE
	for(rv=0;rv<Nand_save.nand_nrbb;rv++){
// #ifdef PROVA_MARK
// 		if ((NAND_badblocks[rv]==block) && (NAND_badblocks[rv]!=33)) {
// #else
		if (NAND_badblocks[rv]==block){
//#endif
#ifdef CBUG
			//if (par71)
				printf("no DEL BLOCK %ld AT 0x%lx\n", block, addr ) ;
#endif
			return(-4) ;
		}
	}
#endif

	if (block >= NAND_NUM_BLOCKS) {
		return(-1) ;
	}

	addr = block * (blockSize/pageSize);
#ifdef CBUG
	//if (par71) 
		printf("DELETE BLOCK %ld AT 0x%lx\n", block, addr ) ;
#endif

#ifndef NAND_ONEADDR
	*PTRNAND_CLE = K9FXX_BLOCK_ERASE_1;
	*PTRNAND_ALE = (uint8_t)(addr & 0x00FF);
	*PTRNAND_ALE = (uint8_t)((addr & 0xFF00) >> 8);
	*PTRNAND_CLE = K9FXX_BLOCK_ERASE_2;
#else
	SET_CLE
	*PTRNAND_DATA = K9FXX_BLOCK_ERASE_1 ;
	SET_ALE
	*PTRNAND_DATA = (uint8_t)(addr & 0x00FF);
	*PTRNAND_DATA = (uint8_t)((addr & 0xFF00) >> 8);
	SET_CLE
	*PTRNAND_DATA = K9FXX_BLOCK_ERASE_2 ;
#endif

	WAIT_READY(700) ;

	rv = ((nandStatus() & 0x01) ? -4 : 0) ;
#ifdef CBUG
	if (rv < 0) pdebugt(1,"NAND/e ERROR") ;
#endif // CBUG

#ifdef MARKBADBLOCKS
	if (rv==-4) nand_invalidateBlock(addr ) ; // & ~(blockSize-1) ) ;
#endif
#ifdef MTS_CODE
	if (rv == -4) {
		if (Nand_save.nand_nrbb<NANDMAXBADBLOCK){
			NAND_badblocks[Nand_save.nand_nrbb++] = block ;
		}else if(NAND_badblocks[NANDMAXBADBLOCK-1]>0)
			NAND_badblocks[NANDMAXBADBLOCK-1] *= -1 ;
	}
#endif

	return(rv) ;
}

// int nand_blockOOB(struct _NAND_OOB * oob_page)
// {
// 	return(NAND_PAGEISFREE) ;
// }


#endif // USE_NANDFLASH_ON_ARM

