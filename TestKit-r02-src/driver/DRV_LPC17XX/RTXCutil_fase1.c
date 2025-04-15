// RTXCutil.c for LPC17xx
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

extern void __kernel_stack__end ;
unsigned long Global_STACK_FILLER = 0x0ad50ad5 ;

// -----------------------------------------------------------------------------
// internal functions prototype

void CLOCKinit(unsigned long desired_clock) ;
void xPortPendSVHandler( void ) __attribute__ (( naked )) ;
void vPortSVCHandler( void ) __attribute__ (( naked )) ;

//----------------------------------------------------------------------------

#undef USE_DEBUG

//----------------------------------------------------------------------------
// Debug only

#ifdef USE_DEBUG
#define PUT_OCT(A) { PUT_CHAR('0' + (((A)>>6) & 7)) ; PUT_CHAR('0' + (((A)>>3) & 7)) ; PUT_CHAR('0' + ((A) & 7)) ; }
#define PUT_CHAR(A) {while(!((UART0->LSR) & 0x20));UART0->THR=(A);}
#endif // USE_DEBUG

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

//----------------------------------------------------------------------------
// CLOCKinit
// This function performs very low level HW PLL initialization
//
// Handled clock:
//      16 MHz  - No use of PLL0 / PLL1
//      25 MHz  - 400 MHz PLL with div by 16
//      50 MHz  - 400 MHz PLL with div by 8
//      80 MHz  - 400 MHz PLL with div by 5
//     100 MHz  - 400 MHz PLL with div by 4

unsigned long current_clock NOINIT_ATTRIBUTE ;

void CLOCKinit(unsigned long desired_clock)
{
    int divider ;
    
    // Enable GPIO power, only
    SC->PCONP = PCONP_PCGPIO ;

    // PCLKSELx MUST be configure before PLL0 setup - see NXP Errata
    // SSPI0, UART0-3 CAN need PCLK = CCLK
    // I2S needs PCLK = CCLK/2
    // All others: PCLK = CCLK / 4

    SC->PCLKSEL0 = 0x54000140 ;         // it was 0x00000140 without CAN
#if defined(G100)
    SC->PCLKSEL1 = 0x00850400 ;
#else
	SC->PCLKSEL1 = 0x00050400 ;         // bit11,10: 01 -> SSPI0 @ PCLK == CCLK
#endif

    // Disable TPIU.
    PINCON->PINSEL10 = 0 ;

    // first step, if PLL enabled, disable it
    if ( SC->PLL0STAT & ( 1 << 25 ) ) {
        // Enable PLL, disconnected
        SC->PLL0CON = 1 ;
        SC->PLL0FEED = PLLFEED_FEED1 ;
        SC->PLL0FEED = PLLFEED_FEED2 ;
    }

    // Disable PLL, disconnected.
    SC->PLL0CON = 0 ;
    SC->PLL0FEED = PLLFEED_FEED1 ;
    SC->PLL0FEED = PLLFEED_FEED2 ;

    // Enable main OSC.
    SC->SCS |= 0x20 ;
    while( !( SC->SCS & 0x40 ) ) ;

    // select main OSC, (xxMHz xtal), as the PLL clock source.
    SC->CLKSRCSEL = 0x1;

    if (desired_clock <= 16000000) {    // NO pll

        // Enable main OSC.
        SC->SCS |= 0x20;
        while( !( SC->SCS & 0x40 ) ) ;

        // select main OSC, (xxMHz xtal), as the clock source.
        SC->CLKSRCSEL = 0x1 ;

        // Set clock divider.
        SC->CCLKCFG = 0x00 ;            // div by 1 --> cpuclk 16 MHz

        // Configure flash accelerator
        SC->FLASHCFG = 0x003a;          // 1 CPU clocks (< 20 MHz)

        current_clock = 16000000 ;

    } else {                            // enable pll

        // SC->PLL0CFG = 0x20031;       // 12MHz --> N=3, M=50 --> 400MHz
        SC->PLL0CFG = 0x30031 ;         // 16MHz --> N=4, M=50 --> 400MHz
        SC->PLL0FEED = PLLFEED_FEED1 ;
        SC->PLL0FEED = PLLFEED_FEED2 ;

        // Enable PLL, disconnected.
        SC->PLL0CON = 1 ;
        SC->PLL0FEED = PLLFEED_FEED1 ;
        SC->PLL0FEED = PLLFEED_FEED2 ;

        divider = 400000000 / desired_clock ;
        if (divider < 4) divider = 4 ;
        current_clock = 400000000 / divider ;

        // Set clock divider.
        SC->CCLKCFG = (divider -1) ;    // 400MHz --> div by X --> cpuclk

        // Configure flash accelerator
        divider = ((current_clock - 1) / 20000000) ;
        SC->FLASHCFG = (divider << 12) | 0x3a ; // CPU clocks

        // Check lock bit status
        while( ( ( SC->PLL0STAT & ( 1 << 26 ) ) == 0 ) ) ;

        // Enable and connect.
        SC->PLL0CON = 3 ;
        SC->PLL0FEED = PLLFEED_FEED1 ;
        SC->PLL0FEED = PLLFEED_FEED2 ;
        while( ( ( SC->PLL0STAT & ( 1 << 25 ) ) == 0 ) ) ;

#ifdef USE_USB_ON_ARM
        // Configure the clock for the USB.

        if( SC->PLL1STAT & ( 1 << 9 ) ) {
            // Enable PLL, disconnected.
            SC->PLL1CON = 1;
            SC->PLL1FEED = PLLFEED_FEED1;
            SC->PLL1FEED = PLLFEED_FEED2;
        }

        // Disable PLL, disconnected.
        SC->PLL1CON = 0;
        SC->PLL1FEED = PLLFEED_FEED1;
        SC->PLL1FEED = PLLFEED_FEED2;

        // SC->PLL1CFG = 0x23;                 // 12MHz --> M=3 P=1 --> 12 * 3 / 1 = 48
        SC->PLL1CFG = 0x46;                    // 16MHz --> M=6 P=2 --> 16 * 6 / 2 = 48
        SC->PLL1FEED = PLLFEED_FEED1;
        SC->PLL1FEED = PLLFEED_FEED2;

        // Enable PLL, disconnected.
        SC->PLL1CON = 1;
        SC->PLL1FEED = PLLFEED_FEED1;
        SC->PLL1FEED = PLLFEED_FEED2;
        while( ( ( SC->PLL1STAT & ( 1 << 10 ) ) == 0 ) );

        // Enable and connect.
        SC->PLL1CON = 3;
        SC->PLL1FEED = PLLFEED_FEED1;
        SC->PLL1FEED = PLLFEED_FEED2;
        while( ( ( SC->PLL1STAT & ( 1 << 9 ) ) == 0 ) );
#endif // USE_USB_ON_ARM
    }
    
#if defined(USE_WATCHDOG)
    WDT->WDTC = (16*(current_clock/4/4) | 0xff) ;       // once WDEN is set, the WDT will start after feeding
    // from this moment, use internal clock
    WDT->WDCLKSEL = 0x80000001 ;        // lock internal CPU oscillator
#endif // defined(USE_WATCHDOG)
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
void CodeUpgradeMid(void (*ProgramCopyRam)(void), unsigned long *pulDest) __attribute__ ((noreturn)) ;
void ProgramCopy(void) __attribute__ ((section(".code_copy"), noreturn)) ;
extern void _vStackTop;
extern void __code_copy_start, __code_copy_end ;

void CodeUpgrade(void)
{
    // This function MUST be executed with interrupt disabled,
    // no other actions possible
    DISABLE ;
    __disable_irq();

    __asm("     ldr     r0, =_vStackTop                 \n"
          "     ldr     r1, =__code_copy_start          \n"
          "     ldr     r2, =__code_copy_end            \n"
          "     sub     r1, r2, r1                      \n"     // r1 = len
          "     sub     r0, r0, r1                      \n"     // r0 = start
          "     mov     r2, r0                          \n"
          "     sub     r2, r2, #8                      \n"     // prepare stack
          "     msr     msp, r2                         \n"
          "     mov     r1, r0                          \n"     // r1 = pulDest
          "     orr     r0, #1                          \n"     // ProgramCopyRam
          "     bl      CodeUpgradeMid                  \n"
          ) ;

    for(;;) ;
}

void CodeUpgradeMid(void (*ProgramCopyRam)(void), unsigned long *pulDest)
{
    register unsigned long *pulSrc ;

    pulSrc = (unsigned long *)(&__code_copy_start) ;
    //pulDest = (unsigned long *)(ProgramCopyRam) ;
    //len >>= 2 ;
    //while(len--) {
    while(pulSrc < (unsigned long *)(&__code_copy_end)) {
        *pulDest++ = *pulSrc++ ;
    }

    ProgramCopyRam() ;
    
    for(;;) ;
}

//----------------------------------------------------------------------------
// SPI ProgramCopy: NO RETURN
// Copy program from SPI Flash to Internal Flash
// only if we are well accepted
#ifdef USE_SPI_ON_ARM

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

    // This function MUST be executed with interrupt disabled,
    // no other actions possible

#if (CODEFLASH_STOP != 0x3ffff) // sanity check
#error "Flash must be 256K"
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
    iap_entry(command, result) ;
    if (result[0]) {    // some error
        for(;;);        // what to do?
    }

    // Third step: write flash, 1024 pages of 256 bytes
    for(pg=0 ; pg<1024 ; pg++) {

        // CS --\__
        GPIO0->FIOCLR = 0x00010000 ;    // CS at '0'

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
            while(!(SSP0->SR & 0x01)) ;
            //printf("-2- SSP0->SR=0x%08lx\n", SSP0->SR) ;

            // tx
            SSP0->DR = *p ;

            //printf("-3- SSP0->SR=0x%08lx\n", SSP0->SR) ;
            // wait for rx
            while(!(SSP0->SR & 0x04)) ;
            //printf("-4- SSP0->SR=0x%08lx\n", SSP0->SR) ;

            // rx
            *p++ = SSP0->DR ;
        }

        p = &flashbuf[0] ;
        len = sizeof(flashbuf) ;

        // step -2-
        while(len--) {
            // wait for tx
            while(!(SSP0->SR & 0x01)) ;

            // tx
            SSP0->DR = *p ;

            // wait for rx
            while(!(SSP0->SR & 0x04)) ;

            // rx
            *p++ = SSP0->DR ;
        }

        flag_towrite = NO ;    // default
        for(i=0 ; i<sizeof(flashbuf) ; i++) {
            if (flashbuf[i] != 0xff) {  // really to write ?
                flag_towrite = YES ;
                break ;
            }
        }

        // CS __/--
        GPIO0->FIOSET = 0x00010000 ;    // CS at '1'

        // build vector checksum
        if (pg == 0) {                  // page with vector table
            checksum = 0 ;
            for(i=0 ; i<7 ; i++) {      // check vectors
                checksum += *((unsigned long *)(&flashbuf[i * 4])) ;
            }
            // checksum just built
            *((unsigned long *)(&flashbuf[7 * 4])) = 0 - checksum ;
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
    WDT->WDTC = 0xff ;          // once WDEN is set, the WDT will start after feeding
    WDT->WDMOD = 0x03 ;         // enable reset at watchdog timeout
    WDT->WDFEED=0xAA ;
    WDT->WDFEED=0x55 ;          // enable watchdog
#endif // USE_WATCHDOG

    // Make software reset
    WDT->WDFEED=0xAA ;
    WDT->WDFEED=0x00 ;          // any value other than 55 will reset

    for( ; ; ) ;                // just to be sure
}

#else // USE_SPI_ON_ARM
//----------------------------------------------------------------------------
// RAM ProgramCopy: NO RETURN

void ProgramCopy(void)
{
    int pg, i, bbegin, flag_towrite ;
    unsigned long *uptr ;
    
    // This function MUST be executed with interrupt disabled,
    // no other actions possible
    DISABLE ;
    __disable_irq();

#if (CODEFLASH_STOP != 0x3ffff) // sanity check
#error "Flash must be 256K"
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
    WDT->WDFEED=0xAA ;
    WDT->WDFEED=0x00 ;          // any value other than 55 will reset

    for( ; ; ) ;        // just to be sure
}
#endif // USE_SPI_ON_ARM
// end of file - RTXCutil.c

