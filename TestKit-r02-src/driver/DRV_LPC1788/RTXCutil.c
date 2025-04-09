// RTXCutil.c for LPC1788
//
//   Copyright (c) 1997-2013.
//   T.E.S.T. srl
//
#include <stdint.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

#include <nand_ecc.h>

extern void __kernel_stack__end ;
unsigned long Global_STACK_FILLER = 0x0ad50ad5 ;

// -----------------------------------------------------------------------------
// internal functions prototype

void CLOCKinit(unsigned long desired_clock) ;
void xPortPendSVHandler( void ) __attribute__ (( naked )) ;
void vPortSVCHandler( void ) __attribute__ (( naked )) ;

//----------------------------------------------------------------------------

#undef USE_DEBUG

#if defined(CBUG) && defined(USE_DEBUG)
//----------------------------------------------------------------------------
// Debug only
#define USE_MARK_DEBUG(A,B)   *((int *)(0x20004000) + (A)) = B
#define USE_MARK_INCREMENT(A) (*((int *)(0x20004000) + (A)))++
#undef USE_FAKE_ERASE
#define PUT_OCT(A) { PUT_CHAR('0' + (((A)>>6) & 7)) ; PUT_CHAR('0' + (((A)>>3) & 7)) ; PUT_CHAR('0' + ((A) & 7)) ; }
#define PUT_CHAR(A) {while(!((UART0->LSR) & 0x20));UART0->THR=(A);}

#else // defined(CBUG) && defined(USE_DEBUG)
//----------------------------------------------------------------------------
// Production only
#define USE_MARK_DEBUG(A,B)   {}
#define USE_MARK_INCREMENT(A) {}
#undef USE_FAKE_ERASE

#endif // defined(CBUG) && defined(USE_DEBUG)

#define IAP_LOCATION 0x1FFF1FF1
typedef void (*IAP)(unsigned long *command, unsigned long *result) ;
static const IAP iap_entry = (IAP) IAP_LOCATION ;

//----------------------------------------------------------------------------
// IAP: device id

unsigned long IAP_partid(void)
{
    unsigned long command[5];
    unsigned long result[5];
    command[0] = 54 ;
    iap_entry(command, result) ;

    return(result[1]) ;
}

#define PLLFEED_FEED1   0xaa
#define PLLFEED_FEED2   0x55

//----------------------------------------------------------------------------
// CLOCKinit
// This function performs very low level HW PLL initialization
//
// Handled clock:
//      12 MHz  - No use of PLL0 / PLL1
//      15 MHz  - 240/2=120 MHz PLL with div by 8
//      20 MHz  - 240/2=120 MHz PLL with div by 6
//      30 MHz  - 240/2=120 MHz PLL with div by 4
//      40 MHz  - 240/2=120 MHz PLL with div by 3
//      60 MHz  - 240/2=120 MHz PLL with div by 2
//     120 MHz  - 240/2=120 MHz PLL with div by 1

unsigned long current_clock NOINIT_ATTRIBUTE ;

void CLOCKinit(unsigned long desired_clock)
{
    int divider ;

    // Enable GPIO and RTC power, only by now
    LPC_SC->PCONP = CLKPWR_PCONP_PCGPIO | CLKPWR_PCONP_PCRTC ;

    // Enable main OSC.
    LPC_SC->SCS |= 0x20 ;
    while( !( LPC_SC->SCS & 0x40 ) ) ;

    // select main OSC, (xxMHz xtal), as the PLL clock source.
    LPC_SC->CLKSRCSEL = 0x1;

    if (desired_clock <= 12000000) {    // NO pll

        // Set clock divider.
        LPC_SC->CCLKSEL = 1 ;               // div by 1 --> cpuclk 12 MHz

        // Configure flash accelerator
        LPC_SC->FLASHCFG = 0x003a;          // 1 CPU clocks (< 20 MHz)

        current_clock = 12000000 ;

    } else {                            // enable pll
        LPC_SC->FLASHCFG = (0x6 << 12) | 0x3a ; // CPU clocks for 120MHz

        LPC_SC->PLL0CFG = 0x00009 ;         // 12MHz --> M=10, P=1 --> Fcco=240MHz
        LPC_SC->PLL0CON = 1 ;               // enable PLL
        LPC_SC->PLL0FEED = PLLFEED_FEED1 ;
        LPC_SC->PLL0FEED = PLLFEED_FEED2 ;

        // Set clock divider.
        divider = 120000000 / desired_clock ;
        if (divider < 1) divider = 1 ;
        if (divider > 8) divider = 8 ;
        current_clock = 120000000 / divider ;
        LPC_SC->CCLKSEL = 0x100 | divider ;

        // Configure flash accelerator
        divider = ((current_clock - 1) / 20000000) ;
        LPC_SC->FLASHCFG = (divider << 12) | 0x3a ; // CPU clocks

        // Check lock bit status
        while( ( ( LPC_SC->PLL0STAT & ( 1 << 10 ) ) == 0 ) ) ;

#if defined(USE_USB_ON_ARM) || defined(TFTP_FILE_NAME)
        // Configure the clock for the USB.

        LPC_SC->PLL1CFG = 0x00007 ;         // 12MHz --> M=8, P=1 --> Fcco=192MHz
        LPC_SC->PLL1CON = 1 ;               // enable PLL
        LPC_SC->PLL1FEED = PLLFEED_FEED1 ;
        LPC_SC->PLL1FEED = PLLFEED_FEED2 ;

        LPC_SC->USBCLKSEL = (2<<8) | 2 ;    // USB clock from PLL1 / 2

        while( ( ( LPC_SC->PLL1STAT & ( 1 << 10 ) ) == 0 ) );
#endif // defined(USE_USB_ON_ARM) || defined(TFTP_FILE_NAME)
    }

    // Power boost option
//    LPC_SC->PBOOST = (current_clock >= 100000000) 3 : 0 ;

    // Set peripheral clock divider.
    LPC_SC->PCLKSEL = PERIPHERAL_DIVIDER ;  // PERIPHERAL_CLOCK = current_clock / PERIPHERAL_DIVIDER
    LPC_SC->EMCCLKSEL = (EMC_DIVIDER - 1) & 1  ;    // EMC_CLOCK = current_clock / EMC_DIVIDER ;
}

//----------------------------------------------------------------------------

void xPortPendSVHandler( void )
{
// This is a naked function.
// Cortex M3 has automatically saved:
//     top ->   xPSR
//              Return address
//              LR (R14)
//              R12
//              R3
//              R2
//              R1
//              R0
// Saved by hand:
//              R11
//              R10
//              R9
//              R8
//              R7
//              R6
//              R5
//  FRAME* -->  R4

    __asm volatile (
        "       cpsid   i                       \n" // disable interrupts
        "       mrs     r0, msp                 \n" // Process Stack Pointer: banked R13
        "       stmdb   r0!, {r4-r11}           \n" // Save the remaining registers, r0 is FRAME *

        "       ldr     r1, =__kernel_stack__end\n"
        "       msr     msp, r1                 \n" // use msp

        "       mov     r1, #0                  \n" // no sema

        "       stmdb	sp!, {lr}               \n"
        "       cpsie   i                       \n" // enable interrupts
        "       bl      KS_ISRexit              \n"
        "       cpsid   i                       \n" // disable interrupts
        "       ldmia   sp!, {lr}               \n"

        "       ldmia   r0!, {r4-r11}           \n" // Pop the registers that are not automatically saved on exception entry.
        "                                       \n"
        "       msr     msp, r0                 \n"
        "       cpsie   i                       \n" // enable interrupts
        "       bx      r14                     \n"
        ) ;
}

//----------------------------------------------------------------------------

void vPortSVCHandler( void )
{
// This is a naked function.
// Cortex M3 has automatically saved:
//     top ->   xPSR
//              Return address
//              LR (R14)
//              R12
//              R3
//              R2
//              R1
//              R0
// Saved by hand:
//              R11
//              R10
//              R9
//              R8
//              R7
//              R6
//              R5
//  FRAME* -->  R4

    __asm volatile (
        "       cpsid   i                       \n" // disable interrupts
        "       mrs     r0, msp                 \n" // Process Stack Pointer: banked R13
        "       stmdb   r0!, {r4-r11}           \n" // Save the remaining registers, r0 is FRAME *

        "       ldr     r1, =__kernel_stack__end\n"
        "       msr     msp, r1                 \n" // use msp

        "       stmdb	sp!, {lr}               \n"
        "       cpsie   i                       \n" // enable interrupts
        "       bl      rtxc                    \n"
        "       cpsid   i                       \n" // disable interrupts
        "       ldmia   sp!, {lr}               \n"

        "       ldmia   r0!, {r4-r11}           \n" // Pop the registers that are not automatically saved on exception entry.
        "                                       \n"
        "       msr     msp, r0                 \n"
        "       cpsie   i                       \n" // enable interrupts
        "       bx      r14                     \n"
        ) ;
}

//----------------------------------------------------------------------------
// Code copy

void CodeUpgrade(void) __attribute__ ((noreturn)) ;
void CodeUpgradeMid(void (*ProgramCopyRam)(struct _ECC_TABLE * pecct), unsigned long *pulDest, struct _ECC_TABLE * pecct) __attribute__ ((noreturn)) ;
#if defined(USE_NANDFLASH_ON_ARM)
void ProgramCopy(struct _ECC_TABLE * pecct) __attribute__ ((section(".code_copy_entry"), noreturn)) ;
#else
void ProgramCopy(void) __attribute__ ((section(".code_copy"), noreturn)) ;
#endif
extern unsigned long _vStackTop;
extern unsigned long __code_copy_start, __data_copy_start, __code_copy_end ;

void CodeUpgrade(void)
{
    // This function MUST be executed with interrupt disabled,
    // no other actions possible
    DISABLE ;
    __disable_irq();

    __asm("     ldr     r0, =_vStackTop                 \n"
          "     msr     msp, r0                         \n"     // new stack top
          "     sub     r0, r0, #1024                   \n"     // leave room for stack (ProgramCopy uses 512 bytes)

          "     ldr     r1, =__code_copy_start          \n"
          "     ldr     r2, =__code_copy_end            \n"
          "     sub     r1, r2, r1                      \n"     // r1 = data+code len
          "     sub     r0, r0, r1                      \n"     // r0 = start

          "     ldr     r1, =__code_copy_start          \n"
          "     ldr     r2, =__data_copy_start          \n"
          "     sub     r3, r2, r1                      \n"     // r3 = code len

          "     add     r2, r0, r3                      \n"     // r2 = code begin + code len = data begin
          "     mov     r1, r0                          \n"     // r1 = code begin = pulDest
          "     orr     r0, #1                          \n"     // ProgramCopyRam

          "     bl      CodeUpgradeMid                  \n"
          ) ;

    for(;;) ;
}

void CodeUpgradeMid(void (*ProgramCopyRam)(struct _ECC_TABLE * pecct), unsigned long *pulDest, struct _ECC_TABLE * pecct)
{
    register unsigned long *pulSrc = (unsigned long *)(&__code_copy_start) ;

    USE_MARK_DEBUG(0, 0x00000000) ;

    while(pulSrc < (unsigned long *)(&__code_copy_end)) {
        *pulDest++ = *pulSrc++ ;
    }

    USE_MARK_DEBUG(0, (int)ProgramCopyRam) ;
    ProgramCopyRam(pecct) ;

    for(;;) ;
}

//----------------------------------------------------------------------------
// NAND ProgramCopy: NO RETURN
// Copy program from xxx to Internal Flash

int nand_raw_read512(uint64_t addr, void * userbuffer, struct _ECC_TABLE * pecct)
                     __attribute__ ((section(".code_copy"))) ;

void nand_calculate_ecc(const unsigned char *buf,
                        unsigned int eccsize,
                        unsigned char *code,
                        const struct _ECC_TABLE *pecctable)
                        __attribute__ ((section(".code_copy"))) ;

int nand_correct_data(unsigned char *buf,
                      unsigned char *read_ecc, unsigned char *calc_ecc,
                      unsigned int eccsize,
                      const struct _ECC_TABLE *pecctable)
                      __attribute__ ((section(".code_copy"))) ;


#if defined(USE_NANDFLASH_ON_ARM)

#define MYNOPBEGIN   {volatile int _i ; for(_i=0 ; _i<2 ; _i++) __NOP();}
#define MYNOPEND     {volatile int _i ; for(_i=0 ; _i<3 ; _i++) __NOP();}

#define SET_CLE  {MYNOPBEGIN; LPC_GPIO4->CLR = 0x1 ; LPC_GPIO4->SET = 0x2; MYNOPEND;}
#define SET_ALE  {MYNOPBEGIN; LPC_GPIO4->CLR = 0x2 ; LPC_GPIO4->SET = 0x1; MYNOPEND;}
#define SET_DATA {MYNOPBEGIN; LPC_GPIO4->CLR = 0x3 ;                       MYNOPEND;}

// Wait fixed time
#define WAIT_READY(A) {  volatile int i ; for(i=0 ; i<((A)*500) ; i++) __NOP() ; }

//#define CHECK_UPGRADE
//----------------------------------------------------------------------------
// RAM ProgramCopy: NO RETURN
void ProgramCopy(struct _ECC_TABLE * pecct)
{
    int i, pg, rv ;
    unsigned long command[5];
    unsigned long result[5];
//    unsigned char nandbuf[512] ;
    unsigned char *nandbuf = (unsigned char *)(0x20000000) ;
    unsigned long checksum ;
    uint64_t bbegin ;

    extern const uint32_t my_prot_flag ;
//    uint32_t loc_prot_flag ;

    // This function MUST be executed from RAM, with interrupt disabled,
    // no other actions possible

//    loc_prot_flag = my_prot_flag ;  // make local copy

#ifdef MTS_CODE
#if (CODEFLASH_STOP != 0x7ffff) // sanity check
#error "Flash must be 2 x 512k"
#endif
#else
#if (CODEFLASH_STOP != 0xfffff) // sanity check
#error "Flash must be 2 x 512k"
#endif
#endif

    USE_MARK_DEBUG(1, pecct->bitsperbyte[15]) ; // must be 4

#if !defined(USE_FAKE_ERASE)
    // First step: prepare sectors for erase
    command[0] = 50 ;
#ifdef CHECK_UPGRADE
    command[1] = 22 ;    // start sector
#else
    command[1] = 0 ;    // start sector
#endif
    command[2] = 29 ;   // stop sector: 512k
    iap_entry(command, result) ;
    if (result[0]) {    // some error
        for(;;);        // what to do?
    }

    // Second step: erase
    command[0] = 52 ;
#ifdef CHECK_UPGRADE
    command[1] = 22 ;    // start sector
#else
    command[1] = 0 ;    // start sector
#endif
    command[2] = 29 ;   // stop sector: 512k
    command[3] = current_clock / 1000 ; // CPU clock in kHz
    iap_entry(command, result) ;
    if (result[0]) {    // some error
        for(;;);        // what to do?
    }
#endif //  USE_FAKE_ERASE

    USE_MARK_DEBUG(4, 0) ;  // reset ECC counter
    USE_MARK_DEBUG(6, 0) ;  // reset ECC err
    USE_MARK_DEBUG(7, 0) ;  // reset ECC empty
    USE_MARK_DEBUG(5, 0) ;  // reset Error flag
    USE_MARK_DEBUG(3, 0) ;  // reset termination flag

    // Third step: write flash, 1024 pages of 512 bytes
#ifdef CHECK_UPGRADE
    for(pg=0 ; pg<10 ; pg++) {
#else
    for(pg=0 ; pg<1024 ; pg++) {
#endif

        USE_MARK_DEBUG(2, pg) ;

        // read page from NAND flash
        bbegin = CODEFLASH_START + (pg * 512) ;     // bytes per page
        rv = nand_raw_read512(bbegin, nandbuf, pecct) ;

        // check if empty page
        if (rv == 5) continue ;
        
        // use second page, in case of errors
        if (rv < 0) {
#ifdef MTS_CODE
            nand_raw_read512(bbegin + (((CODEFLASH_STOP*2+1) + CODEFLASH_START + 1)>>1), nandbuf, pecct) ;
#else
            nand_raw_read512(bbegin + ((CODEFLASH_STOP + CODEFLASH_START + 1)>>1), nandbuf, pecct) ;
#endif
        }

        // code modification part -1-
        // build vector checksum
        if (pg == 0) {                  // page with vector table
            checksum = 0 ;
            for(i=0 ; i<7 ; i++) {      // check vectors
                checksum += *((unsigned long *)(&nandbuf[i * 4])) ;
            }
            // checksum just built
            *((unsigned long *)(&nandbuf[7 * 4])) = 0 - checksum ;
        }

        // code modification part -2- _BM_
        // set 0x2fc CRP
        if (pg == 1) {                  // page with CRP
//             if (loc_prot_flag != 0xad5) {
//                 *((uint32_t *)(&nandbuf[0xfc])) = 0x43218765 ; // CRP3
//             }
            
            if (*((uint32_t *)(&nandbuf[0xfc]))==0xF1F1F1F1) {
                *((uint32_t *)(&nandbuf[0xfc])) = 0xF3F4F5F6 ; // NO CRP
			}else if (my_prot_flag != 0xad5){
#ifdef NO_JTAG // code protection
                *((uint32_t *)(&nandbuf[0xfc])) = 0x43218765 ; // CRP3
            }else{
                *((uint32_t *)(&nandbuf[0xfc])) = 0x43218765 ; // CRP3
			}
#else
                *((uint32_t *)(&nandbuf[0xfc])) = 0xF3F4F5F6 ; // NO CRP
            }else{
                *((uint32_t *)(&nandbuf[0xfc])) = 0xF3F4F5F6 ; // NO CRP
			}
#endif
        }

#if defined(USE_FAKE_ERASE)
        continue ;      // skip copy
#endif // USE_FAKE_ERASE

        // write page buffer (always from base address)
        // prepare sectors for write
        command[0] = 50 ;
#ifdef CHECK_UPGRADE
	    command[1] = 22 ;    // start sector
#else
	    command[1] = 0 ;    // start sector
#endif
        command[2] = 29 ;   // stop sector: 512k
        iap_entry(command, result) ;
        if (result[0]) {    // some error
            for(;;);        // what to do?
        }

        // copy RAM to flash
        command[0] = 51 ;
#ifdef CHECK_UPGRADE
        command[1] = (256*1024) + (pg * 512) ;                     // destination
#else
        command[1] = pg * 512 ;                     // destination
#endif
        command[2] = (unsigned long)(nandbuf) ;     // source
        command[3] = 512 ;                          // size
        command[4] = current_clock / 1000 ;         // CPU clock in KHz
        iap_entry(command, result) ;
        if (result[0]) {    // some error
            for(;;);        // what to do?
        }
    }

    USE_MARK_DEBUG(3, 0x99999999) ;
    WAIT_READY(10) ;

#ifndef CHECK_UPGRADE
#ifndef USE_WATCHDOG
    // if not, enabled, enable it
    // Watchdog has a 500kHz RC oscillator, set at 16 seconds
    LPC_WDT->TC = 0xff ;            // once WDEN is set, the WDT will start after feeding
    LPC_WDT->MOD = 0x03 ;           // enable reset at watchdog timeout WITH TC PROTECT

    LPC_WDT->FEED = 0xAA ;          // Feeding sequence
    LPC_WDT->FEED = 0x55 ;
#endif // USE_WATCHDOG

    // Make software reset
    LPC_WDT->FEED=0xAA ;
    LPC_WDT->FEED=0x00 ;            // any value other than 55 will reset
#endif

    for( ; ; ) ;                    // just to be sure
}

/******************************************************************************
 *
 * Description:
 *    RAW Read 512 Byte (1/4 page) from the NAND memory without interrupts
 *    Useful in software upgrade
 *
 * Params:
 *    address - full address (byte pointer)
 *    buffer  - 512 byte user buffer
 *
 * Returns:
 *  -4  unrecoverable ECC error
 *  0   read successful
 *  1   1 bit error recovered by ECC
 *  2   2 bit errors recovered by ECC
 *  3   3 bit errors recovered by ECC
 *  4   4 bit errors recovered by ECC
 *  5   probably empty page - ECC at 0xFF's
 *
 *
 *****************************************************************************/

int nand_raw_read512(uint64_t addr, void * userbuffer, struct _ECC_TABLE * pecct)
{
    int rv ;        // return value
    int subpage, i ;
    unsigned char *p ;
    uint32_t specc ;
    uint32_t myecc ;
    int rc ;

    // no sanity checks, code flash is already empty
    subpage = (addr>>9) & 0x3 ; // supbage = 0..3

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
    // wait internal NAND activity
    WAIT_READY(25) ;

    // .........................................................
    // make transfer, without DMA
#ifdef NAND_ONEADDR
	SET_DATA
#endif
    p = userbuffer ;
    for(i=0 ; i<512 ; i++)
        *p++ = *PTRNAND_DATA ;

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
	SET_DATA
#endif

    *((uint8_t *)(&specc) + 0) = *PTRNAND_DATA ;
    *((uint8_t *)(&specc) + 1) = *PTRNAND_DATA ;
    *((uint8_t *)(&specc) + 2) = *PTRNAND_DATA ;
    *((uint8_t *)(&specc) + 3) = *PTRNAND_DATA ;  /* even if ECC is 3 byte only */

    USE_MARK_INCREMENT(4) ;

    // check if subpage is empty - based on ECC values at 0xFF's
    if ( specc == 0xffffffff ) {
        USE_MARK_INCREMENT(7) ;
        return(5) ; // probably empty page
    }

    // check ECC
    rv = 0 ;    // defualt no error detected

    // evaluate ECC of read data
    nand_calculate_ecc(userbuffer, 512, (unsigned char *)(&myecc), pecct) ;
    // check correctness - in case of 1 bit error, correct it in buffer
    rc = nand_correct_data(userbuffer,
                            (unsigned char *)(&specc),      /* from chip */
                            (unsigned char *)(&myecc),      /* evaluated now */
                            512, pecct) ;

    if (rc < 0) {
        USE_MARK_INCREMENT(6) ;
        return(-4) ;    // unrecoverable ECC error
    }

    if (rc > 0) rv++ ;  // one more recovered

    return(rv) ;
}

// This file contains an ECC algorithm that detects and corrects 1 bit
// errors in a 256/512 byte block of data.

const struct _ECC_TABLE ecctable __attribute__ ((section(".data_copy"))) = {
/*
 * invparity is a 256 byte table that contains the odd parity
 * for each byte. So if the number of bits in a byte is even,
 * the array element is 1, and when the number of bits is odd
 * the array eleemnt is 0.
 */
/*static const char invparity[256] __attribute__ ((section(".data_copy"))) = { */
.invparity = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
    },

/*
 * bitsperbyte contains the number of bits per byte
 * this is only used for testing and repairing parity
 * (a precalculated value slightly improves performance)
 */
/*static const char bitsperbyte[256] __attribute__ ((section(".data_copy"))) = { */
.bitsperbyte = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
    },

/*
 * addressbits is a lookup table to filter out the bits from the xor-ed
 * ecc data that identify the faulty location.
 * this is only used for repairing parity
 * see the comments in nand_correct_data for more details
 */
/*static const char addressbits[256] __attribute__ ((section(".data_copy"))) = { */
.addressbits = {
    0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
    0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
    0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
    0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
    0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
    0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
    0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
    0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
    0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
    0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01,
    0x02, 0x02, 0x03, 0x03, 0x02, 0x02, 0x03, 0x03,
    0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
    0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
    0x04, 0x04, 0x05, 0x05, 0x04, 0x04, 0x05, 0x05,
    0x06, 0x06, 0x07, 0x07, 0x06, 0x06, 0x07, 0x07,
    0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
    0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
    0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
    0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
    0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
    0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
    0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
    0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
    0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
    0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
    0x08, 0x08, 0x09, 0x09, 0x08, 0x08, 0x09, 0x09,
    0x0a, 0x0a, 0x0b, 0x0b, 0x0a, 0x0a, 0x0b, 0x0b,
    0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
    0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f,
    0x0c, 0x0c, 0x0d, 0x0d, 0x0c, 0x0c, 0x0d, 0x0d,
    0x0e, 0x0e, 0x0f, 0x0f, 0x0e, 0x0e, 0x0f, 0x0f
    }
} ;

/**
 * nand_calculate_ecc - [NAND Interface] Calculate 3-byte ECC for 256/512-byte
 *           block
 * @buf:    input buffer with raw data
 * @eccsize:    data bytes per ecc step (256 or 512)
 * @xcode:      output buffer with ECC
 * @pecctable:  table
 */
void nand_calculate_ecc(const unsigned char *buf, unsigned int eccsize, unsigned char *code, const struct _ECC_TABLE *pecctable)
{
    int i;
    const uint32_t *bp = (uint32_t *)buf;
    /* 256 or 512 bytes/ecc  */
    const uint32_t eccsize_mult = eccsize >> 8;
    uint32_t cur;       /* current value in buffer */
    /* rp0..rp15..rp17 are the various accumulated parities (per byte) */
    uint32_t rp0, rp1, rp2, rp3, rp4, rp5, rp6, rp7;
    uint32_t rp8, rp9, rp10, rp11, rp12, rp13, rp14, rp15, rp16;
    uint32_t rp17 = 0 ; /* uninitialized_var(rp17); to make compiler happy */
    uint32_t par;       /* the cumulative parity for all data */
    uint32_t tmppar;    /* the cumulative parity for this iteration;
                   for rp12, rp14 and rp16 at the end of the
                   loop */

    // default location when called from code in Flash memory
    if (!pecctable) pecctable = &ecctable ;

    par = 0;
    rp4 = 0;
    rp6 = 0;
    rp8 = 0;
    rp10 = 0;
    rp12 = 0;
    rp14 = 0;
    rp16 = 0;

    /*
     * The loop is unrolled a number of times;
     * This avoids if statements to decide on which rp value to update
     * Also we process the data by longwords.
     * Note: passing unaligned data might give a performance penalty.
     * It is assumed that the buffers are aligned.
     * tmppar is the cumulative sum of this iteration.
     * needed for calculating rp12, rp14, rp16 and par
     * also used as a performance improvement for rp6, rp8 and rp10
     */
    for (i = 0; i < eccsize_mult << 2; i++) {
        cur = *bp++;
        tmppar = cur;
        rp4 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp6 ^= tmppar;
        cur = *bp++;
        tmppar ^= cur;
        rp4 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp8 ^= tmppar;

        cur = *bp++;
        tmppar ^= cur;
        rp4 ^= cur;
        rp6 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp6 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp4 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp10 ^= tmppar;

        cur = *bp++;
        tmppar ^= cur;
        rp4 ^= cur;
        rp6 ^= cur;
        rp8 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp6 ^= cur;
        rp8 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp4 ^= cur;
        rp8 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp8 ^= cur;

        cur = *bp++;
        tmppar ^= cur;
        rp4 ^= cur;
        rp6 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp6 ^= cur;
        cur = *bp++;
        tmppar ^= cur;
        rp4 ^= cur;
        cur = *bp++;
        tmppar ^= cur;

        par ^= tmppar;
        if ((i & 0x1) == 0)
            rp12 ^= tmppar;
        if ((i & 0x2) == 0)
            rp14 ^= tmppar;
        if (eccsize_mult == 2 && (i & 0x4) == 0)
            rp16 ^= tmppar;
    }

    /*
     * handle the fact that we use longword operations
     * we'll bring rp4..rp14..rp16 back to single byte entities by
     * shifting and xoring first fold the upper and lower 16 bits,
     * then the upper and lower 8 bits.
     */
    rp4 ^= (rp4 >> 16);
    rp4 ^= (rp4 >> 8);
    rp4 &= 0xff;
    rp6 ^= (rp6 >> 16);
    rp6 ^= (rp6 >> 8);
    rp6 &= 0xff;
    rp8 ^= (rp8 >> 16);
    rp8 ^= (rp8 >> 8);
    rp8 &= 0xff;
    rp10 ^= (rp10 >> 16);
    rp10 ^= (rp10 >> 8);
    rp10 &= 0xff;
    rp12 ^= (rp12 >> 16);
    rp12 ^= (rp12 >> 8);
    rp12 &= 0xff;
    rp14 ^= (rp14 >> 16);
    rp14 ^= (rp14 >> 8);
    rp14 &= 0xff;
    if (eccsize_mult == 2) {
        rp16 ^= (rp16 >> 16);
        rp16 ^= (rp16 >> 8);
        rp16 &= 0xff;
    }

    /*
     * we also need to calculate the row parity for rp0..rp3
     * This is present in par, because par is now
     * rp3 rp3 rp2 rp2 in little endian and
     * rp2 rp2 rp3 rp3 in big endian
     * as well as
     * rp1 rp0 rp1 rp0 in little endian and
     * rp0 rp1 rp0 rp1 in big endian
     * First calculate rp2 and rp3
     */
#ifdef __BIG_ENDIAN
    rp2 = (par >> 16);
    rp2 ^= (rp2 >> 8);
    rp2 &= 0xff;
    rp3 = par & 0xffff;
    rp3 ^= (rp3 >> 8);
    rp3 &= 0xff;
#else
    rp3 = (par >> 16);
    rp3 ^= (rp3 >> 8);
    rp3 &= 0xff;
    rp2 = par & 0xffff;
    rp2 ^= (rp2 >> 8);
    rp2 &= 0xff;
#endif

    /* reduce par to 16 bits then calculate rp1 and rp0 */
    par ^= (par >> 16);
#ifdef __BIG_ENDIAN
    rp0 = (par >> 8) & 0xff;
    rp1 = (par & 0xff);
#else
    rp1 = (par >> 8) & 0xff;
    rp0 = (par & 0xff);
#endif

    /* finally reduce par to 8 bits */
    par ^= (par >> 8);
    par &= 0xff;

    /*
     * and calculate rp5..rp15..rp17
     * note that par = rp4 ^ rp5 and due to the commutative property
     * of the ^ operator we can say:
     * rp5 = (par ^ rp4);
     * The & 0xff seems superfluous, but benchmarking learned that
     * leaving it out gives slightly worse results. No idea why, probably
     * it has to do with the way the pipeline in pentium is organized.
     */
    rp5 = (par ^ rp4) & 0xff;
    rp7 = (par ^ rp6) & 0xff;
    rp9 = (par ^ rp8) & 0xff;
    rp11 = (par ^ rp10) & 0xff;
    rp13 = (par ^ rp12) & 0xff;
    rp15 = (par ^ rp14) & 0xff;
    if (eccsize_mult == 2)
        rp17 = (par ^ rp16) & 0xff;

    /*
     * Finally calculate the ecc bits.
     * Again here it might seem that there are performance optimisations
     * possible, but benchmarks showed that on the system this is developed
     * the code below is the fastest
     */
#ifdef CONFIG_MTD_NAND_ECC_SMC
    code[0] =
        (pecctable->invparity[rp7] << 7) |
        (pecctable->invparity[rp6] << 6) |
        (pecctable->invparity[rp5] << 5) |
        (pecctable->invparity[rp4] << 4) |
        (pecctable->invparity[rp3] << 3) |
        (pecctable->invparity[rp2] << 2) |
        (pecctable->invparity[rp1] << 1) |
        (pecctable->invparity[rp0]);
    code[1] =
        (pecctable->invparity[rp15] << 7) |
        (pecctable->invparity[rp14] << 6) |
        (pecctable->invparity[rp13] << 5) |
        (pecctable->invparity[rp12] << 4) |
        (pecctable->invparity[rp11] << 3) |
        (pecctable->invparity[rp10] << 2) |
        (pecctable->invparity[rp9] << 1)  |
        (pecctable->invparity[rp8]);
#else
    code[1] =
        (pecctable->invparity[rp7] << 7) |
        (pecctable->invparity[rp6] << 6) |
        (pecctable->invparity[rp5] << 5) |
        (pecctable->invparity[rp4] << 4) |
        (pecctable->invparity[rp3] << 3) |
        (pecctable->invparity[rp2] << 2) |
        (pecctable->invparity[rp1] << 1) |
        (pecctable->invparity[rp0]);
    code[0] =
        (pecctable->invparity[rp15] << 7) |
        (pecctable->invparity[rp14] << 6) |
        (pecctable->invparity[rp13] << 5) |
        (pecctable->invparity[rp12] << 4) |
        (pecctable->invparity[rp11] << 3) |
        (pecctable->invparity[rp10] << 2) |
        (pecctable->invparity[rp9] << 1)  |
        (pecctable->invparity[rp8]);
#endif
    if (eccsize_mult == 1)
        code[2] =
            (pecctable->invparity[par & 0xf0] << 7) |
            (pecctable->invparity[par & 0x0f] << 6) |
            (pecctable->invparity[par & 0xcc] << 5) |
            (pecctable->invparity[par & 0x33] << 4) |
            (pecctable->invparity[par & 0xaa] << 3) |
            (pecctable->invparity[par & 0x55] << 2) |
            3;
    else
        code[2] =
            (pecctable->invparity[par & 0xf0] << 7) |
            (pecctable->invparity[par & 0x0f] << 6) |
            (pecctable->invparity[par & 0xcc] << 5) |
            (pecctable->invparity[par & 0x33] << 4) |
            (pecctable->invparity[par & 0xaa] << 3) |
            (pecctable->invparity[par & 0x55] << 2) |
            (pecctable->invparity[rp17] << 1) |
            (pecctable->invparity[rp16] << 0);
}

/**
 * nand_correct_data - [NAND Interface] Detect and correct bit error(s)
 * @buf:    raw data read from the chip
 * @read_ecc:   ECC from the chip
 * @calc_ecc:   the ECC calculated from raw data
 * @eccsize:    data bytes per ecc step (256 or 512)
 * @pecctable:  table
 *
 * Detect and correct a 1 bit error for eccsize byte block
 */
int nand_correct_data(unsigned char *buf,
            unsigned char *read_ecc, unsigned char *calc_ecc,
            unsigned int eccsize,
            const struct _ECC_TABLE *pecctable)
{
    unsigned char b0, b1, b2, bit_addr;
    unsigned int byte_addr;
    /* 256 or 512 bytes/ecc  */
    const uint32_t eccsize_mult = eccsize >> 8;

    // default location when called from code in Flash memory
    if (!pecctable) pecctable = &ecctable ;

    /*
     * b0 to b2 indicate which bit is faulty (if any)
     * we might need the xor result  more than once,
     * so keep them in a local var
    */
#ifdef CONFIG_MTD_NAND_ECC_SMC
    b0 = read_ecc[0] ^ calc_ecc[0];
    b1 = read_ecc[1] ^ calc_ecc[1];
#else
    b0 = read_ecc[1] ^ calc_ecc[1];
    b1 = read_ecc[0] ^ calc_ecc[0];
#endif
    b2 = read_ecc[2] ^ calc_ecc[2];

    /* check if there are any bitfaults */

    /* repeated if statements are slightly more efficient than switch ... */
    /* ordered in order of likelihood */

    if ((b0 | b1 | b2) == 0)
        return(0) ; /* no error */

    if ((((b0 ^ (b0 >> 1)) & 0x55) == 0x55) &&
        (((b1 ^ (b1 >> 1)) & 0x55) == 0x55) &&
        ((eccsize_mult == 1 && ((b2 ^ (b2 >> 1)) & 0x54) == 0x54) ||
         (eccsize_mult == 2 && ((b2 ^ (b2 >> 1)) & 0x55) == 0x55))) {
        /* single bit error */
        /*
         * rp17/rp15/13/11/9/7/5/3/1 indicate which byte is the faulty
         * byte, cp 5/3/1 indicate the faulty bit.
         * A lookup table (called addressbits) is used to filter
         * the bits from the byte they are in.
         * A marginal optimisation is possible by having three
         * different lookup tables.
         * One as we have now (for b0), one for b2
         * (that would avoid the >> 1), and one for b1 (with all values
         * << 4). However it was felt that introducing two more tables
         * hardly justify the gain.
         *
         * The b2 shift is there to get rid of the lowest two bits.
         * We could also do addressbits[b2] >> 1 but for the
         * performance it does not make any difference
         */
        if (eccsize_mult == 1)
            byte_addr = (pecctable->addressbits[b1] << 4) + pecctable->addressbits[b0];
        else
            byte_addr = (pecctable->addressbits[b2 & 0x3] << 8) +
                    (pecctable->addressbits[b1] << 4) + pecctable->addressbits[b0];
        bit_addr = pecctable->addressbits[b2 >> 2];
        /* flip the bit */
        buf[byte_addr] ^= (1 << bit_addr);
        return(1) ;

    }
    /* count nr of bits; use table lookup, faster than calculating it */
    if ((pecctable->bitsperbyte[b0] + pecctable->bitsperbyte[b1] + pecctable->bitsperbyte[b2]) == 1)
        return(1) ; /* error in ecc data; no action needed */

//  printf("ECC uncorrectable error\n");
    return(-1) ;
}

#elif defined(USE_SPI_ON_ARM)

void ProgramCopy(void)
{

    int pg, i, bbegin, flag_towrite ;
    unsigned char buftx[8]  ;
    unsigned char flashbuf[256] ;
    int len ;
    unsigned char *p ;
    unsigned long command[5];
    unsigned long result[5];
    unsigned long checksum ;

    extern unsigned char flashcode ;
    extern const uint32_t my_prot_flag ;

    // This function MUST be executed with interrupt disabled,
    // no other actions possible

#if (CODEFLASH_STOP != 0x3ffff) // sanity check
#error "Flash must be 256k"
#endif

    // First step: prepare sectors for erase
    command[0] = 50 ;
    command[1] = 0 ;    // start sector
    command[2] = 21 ;   // stop sector: 256K
    iap_entry(command, result) ;
    if (result[0]) {    // some error
        for(;;);        // what to do?
    }

    // Second step: erase
    command[0] = 52 ;
    command[1] = 0 ;    // start sector
    command[2] = 21 ;   // stop sector: 256K
    command[3] = current_clock / 1000 ; // CPU clock in kHz
    iap_entry(command, result) ;
    if (result[0]) {    // some error
        for(;;);        // what to do?
    }

    // Third step: write flash, 1024 pages of 256 bytes
    for(pg=0 ; pg<1024 ; pg++) {

        // CS --\__
        LPC_GPIO0->CLR = 0x00010000 ;       // CS at '0'

        // read page from SPI flash
        bbegin = CODEFLASH_START + (pg * sizeof(flashbuf)) ;    // bytes per page

        len = 0 ;   // default
        if (flashcode == FLASHCODE_ATMEL_AT45DB161D) {

            buftx[0] = 0xd2 ;               // page read
            buftx[1] = bbegin >> 15 ;
            buftx[2] = ((bbegin >> 7) & 0xfc) | ((bbegin >> 8) & 1) ;
            buftx[3] = bbegin ;
            len = 8 ;

        } else if (flashcode == FLASHCODE_NUMONYX_M45PE16) {

            buftx[0] = 0x03 ;               // read
            buftx[1] = bbegin >> 16 ;
            buftx[2] = bbegin >> 8 ;
            buftx[3] = bbegin ;
            len = 4 ;
        }

        p = &buftx[0] ;

        // step -1-
        while(len--) {
            //printf("-1- SSP0->SR=0x%08lx\n", SSP0->SR) ;
            // wait for tx
            while(!(LPC_SSP0->SR & 0x01)) ;
            //printf("-2- SSP0->SR=0x%08lx\n", SSP0->SR) ;

            // tx
            LPC_SSP0->DR = *p ;

            //printf("-3- SSP0->SR=0x%08lx\n", SSP0->SR) ;
            // wait for rx
            while(!(LPC_SSP0->SR & 0x04)) ;
            //printf("-4- SSP0->SR=0x%08lx\n", SSP0->SR) ;

            // rx
            *p++ = LPC_SSP0->DR ;
        }

        p = &flashbuf[0] ;
        len = sizeof(flashbuf) ;

        // step -2-
        while(len--) {
            // wait for tx
            while(!(LPC_SSP0->SR & 0x01)) ;

            // tx
            LPC_SSP0->DR = *p ;

            // wait for rx
            while(!(LPC_SSP0->SR & 0x04)) ;

            // rx
            *p++ = LPC_SSP0->DR ;
        }

        flag_towrite = NO ;    // default
        for(i=0 ; i<sizeof(flashbuf) ; i++) {
            if (flashbuf[i] != 0xff) {  // really to write ?
                flag_towrite = YES ;
                break ;
            }
        }

        // CS __/--
        LPC_GPIO0->SET = 0x00010000 ;       // CS at '1'

        // code modification part -1-
        // build vector checksum
        if (pg == 0) {                  // page with vector table
            checksum = 0 ;
            for(i=0 ; i<7 ; i++) {      // check vectors
                checksum += *((unsigned long *)(&flashbuf[i * 4])) ;
            }
            // checksum just built
            *((unsigned long *)(&flashbuf[7 * 4])) = 0 - checksum ;
        }

        // code modification part -2- _BM_
        // set 0x2fc CRP
        if (pg == 2) {                  // page with vector table
            if (my_prot_flag != 0xad5) {
                *((uint32_t *)(&flashbuf[0xfc])) = 0x43218765 ; // CRP3
            }
        }

        // write page buffer (always from base address)
        if (flag_towrite) {
            // prepare sectors for write
            command[0] = 50 ;
            command[1] = 0 ;    // start sector
            command[2] = 21 ;   // stop sector: 256K
            iap_entry(command, result) ;
            if (result[0]) {    // some error
                for(;;);        // what to do?
            }

            // copy RAM to flash
            command[0] = 51 ;
            command[1] = pg * sizeof(flashbuf) ;        // destination
            command[2] = (unsigned long)(flashbuf) ;    // source
            command[3] = sizeof(flashbuf) ;             // size
            command[4] = current_clock / 1000 ;         // CPU clock in KHz
            iap_entry(command, result) ;
            if (result[0]) {    // some error
                for(;;);        // what to do?
            }
        }
    }

#ifndef USE_WATCHDOG
    // if not, enabled, enable it
    // Watchdog has a 500kHz RC oscillator, set at 16 seconds
    LPC_WDT->TC = 0xff ;            // once WDEN is set, the WDT will start after feeding
    LPC_WDT->MOD = 0x03 ;           // enable reset at watchdog timeout WITH TC PROTECT

    LPC_WDT->FEED = 0xAA ;          // Feeding sequence
    LPC_WDT->FEED = 0x55 ;
#endif // USE_WATCHDOG

    // Make software reset
    LPC_WDT->FEED=0xAA ;
    LPC_WDT->FEED=0x00 ;            // any value other than 55 will reset

    for( ; ; ) ;                    // just to be sure
}

#else // no USE_SPI_ON_ARM, no USE_NANDFLASH_ON_ARM, use RAM
//----------------------------------------------------------------------------
// RAM ProgramCopy: NO RETURN

void ProgramCopy(void)
{
    int pg, i, bbegin, flag_towrite ;
    unsigned long *uptr ;
    unsigned long command[5];
    unsigned long result[5];
    unsigned char flashbuf[256] ;

    // This function MUST be executed with interrupt disabled,
    // no other actions possible
    DISABLE ;
    __disable_irq();

    // First step: prepare sectors for erase
    command[0] = 50 ;
    command[1] = 0 ;    // start sector
    command[2] = 21 ;   // stop sector: 256K
    iap_entry(command, result) ;
    if (result[0]) {    // some error
        for(;;);        // what to do?
    }

    // Second step: erase
    command[0] = 52 ;
    command[1] = 0 ;    // start sector
    command[2] = 21 ;   // stop sector: 256K
    iap_entry(command, result) ;
    if (result[0]) {    // some error
        for(;;);        // what to do?
    }

    // Third step: prepare sectors for write
    command[0] = 50 ;
    command[1] = 0 ;    // start sector
    command[2] = 21 ;   // stop sector: 256K
    iap_entry(command, result) ;
    if (result[0]) {    // some error
        for(;;);        // what to do?
    }

    // Fourth step: write flash, 1024 pages of 256 bytes
    for(pg=0 ; pg<1024 ; pg++) {
        bbegin = pg * sizeof(flashbuf) ;
        uptr = &ramcode[bbegin >> 2] ;

        flag_towrite = NO ;    // default
        for(i=0 ; i<(sizeof(flashbuf) >> 2) ; i++) {
            if (uptr[i] != 0xffffffff) {  // really to write ?
                flag_towrite = YES ;
                break ;
            }
        }

        // write page buffer (always from base address)
        if (flag_towrite) {
            // copy RAM to flash
            command[0] = 51 ;
            command[1] = pg * sizeof(flashbuf) ;        // destination
            command[2] = (unsigned long)(uptr) ;        // source
            command[3] = sizeof(flashbuf) ;             // size
            command[4] = current_clock / 1000 ;         // CPU clock in KHz
            iap_entry(command, result) ;
            if (result[0]) {    // some error
                for(;;);        // what to do?
            }
        }
    }

    // Make software reset
    LPC_WDT->FEED=0xAA ;
    LPC_WDT->FEED=0x00 ;          // any value other than 55 will reset

    for( ; ; ) ;        // just to be sure
}
#endif // USE_SPI_ON_ARM
// end of file - RTXCutil.c

