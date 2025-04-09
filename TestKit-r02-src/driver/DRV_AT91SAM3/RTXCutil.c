// RTXCutil.c for AT91SAM3S4
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
#define PUT_CHAR(A) {while (!(USART0->US_CSR & US_CSR_TXRDY)); USART0->US_THR=(A);}
#endif // USE_DEBUG

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
    register int use_div ;
    register int use_mul ;
    unsigned long tmpreg ;

    if (!desired_clock) {       // zero means use default
        desired_clock = EXTERNAL_CLOCK ;
    }

    // disable USB PCKn
    PMC->PMC_SCDR = PMC_SCDR_UDP | PMC_SCDR_PCK0 | PMC_SCDR_PCK1 | PMC_SCDR_PCK2 ;

    // SLOW clock
    // CSS first: use slow clock with old PRES
    PMC->PMC_MCKR = PMC_MCKR_CSS_SLOW_CLK | ((PMC->PMC_MCKR) & PMC_MCKR_PRES)  ;
    while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;
    // PRES after
    PMC->PMC_MCKR = PMC_MCKR_CSS_SLOW_CLK | PMC_MCKR_PRES_CLK  ;
    while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;

    // just to be sure, correct values settled hereafter
    EFC->EEFC_FMR = 2 << 8 ;    // set 2+1 = 3 Wait States
    
    // Set MCK at desired_clock
    // 1 Enabling the Main Oscillator:
    if (desired_clock > 32768) {        // we need external clock
        // Initialize main oscillator
//        if (!((PMC->CKGR_MOR) & CKGR_MOR_MOSCXTEN)) {
            // main oscillator is disabled, turn it on
            // SCK = 1/32768 = 30.51 uSecond
            // Start up time = 8 * 6 / SCK = 56 * 30.51 = 1,46484375 ms
            // use 8 just to be sure
            PMC->CKGR_MOR = (0x37 << 16) | CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCRCEN | (CKGR_MOR_MOSCXTST & (0x08<<8)) ;
            // Wait the startup time
            while(!(PMC->PMC_SR & PMC_SR_MOSCXTS)) ;

            // Switch to Xtal oscillator
            PMC->CKGR_MOR = (0x37 << 16) | CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCRCEN | (CKGR_MOR_MOSCXTST & (0x08<<8)) | CKGR_MOR_MOSCSEL ;
            while (!(PMC->PMC_SR & PMC_SR_MOSCSELS)) ;
//        }
    }

    // 2 Checking the Main Oscillator Frequency (Optional)
// NOT RELIABLE, 32KHz int clock spreads from 22KHz to 42KHz
//    // Wait the main clock ready time
//    while(!(AT91C_BASE_PMC->PMC_MCFR & AT91C_CKGR_MAINRDY)) ;
//    // 16MHz ext, 32KHz int     --> 8000
//    // 18.432MHz ext, 32KHz int --> 9216
//    // threshold = (8000 + 9216) / 2 = 8608
//    debug_mainclock = (unsigned short)(AT91C_BASE_PMC->PMC_MCFR) ;
//    if (debug_mainclock < 8608) {
//        EXTERNAL_CLOCK is 16MHz
//    } else {
//        EXTERNAL_CLOCK is 18.432MHz
//    }

    // 3 Setting PLL and divider:
    if (desired_clock > EXTERNAL_CLOCK) {       // we need PLL
        use_div = (desired_clock >= 40000000) ? 2 : 4 ;
        use_mul = (desired_clock * use_div) / (EXTERNAL_CLOCK/2) ;
    
        // PLL is disabled, turn it on
        // - div by 2 - Fin: EXTERNAL_CLOCK / 2
        // - pll will be divided by 2
        // Field out NOT USED = 0 (PLL range 80 - 160 MHz)
        // PLLCOUNT pll startup time estimate at : 0.865 ms
        // PLLCOUNT 28 = 0.000865 /(1/32768)
        PMC->CKGR_PLLAR = ( CKGR_PLLAR_STUCKTO1 |
                           (CKGR_PLLAR_DIVA & 2) |
                           (CKGR_PLLAR_PLLACOUNT & (60/*28*/<<8)) |
                           (CKGR_PLLAR_MULA & ((use_mul - 1)<<16)) ) ;

        // examples:
        // CLOCK   CLOCK/2   use_mul   PLL  use_div  MAIN
        //   16       8         16     128      2      64
        //   16       8         15     120      2      60
        //   16       8         14     112      2      56
        //   16       8         13     104      2      52
        //   16       8         12      96      2      48
        //   16       8         10      80      2      40
        //   16       8         15     120      4      30
        //   16       8         12      96      4      24
        //   16       8         10      80      4      20

        // Wait the startup time
        while(!(PMC->PMC_SR & PMC_SR_LOCKA)) ;
        while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;
    } else if (desired_clock > 32768) {
        use_div = EXTERNAL_CLOCK / desired_clock ;
        if (use_div == 3) use_div = 4 ; // not available
        if (use_div > 4) use_div = 4 ;  // not available
        use_mul = 1 ;
    } else {
        use_div = 32768 / desired_clock ;
        if (use_div == 3) use_div = 4 ; // not available
        if (use_div > 4) use_div = 4 ;  // not available
        use_mul = 1 ;
    }

    // 4. Selection of Master Clock and Processor Clock
    if (desired_clock > EXTERNAL_CLOCK) {
        // PLL: select PRES first
        // select the PLL clock divided by 2
        PMC->PMC_MCKR = ((PMC->PMC_MCKR) & PMC_MCKR_CSS) |
                                   ((use_div == 2) ? PMC_MCKR_PRES_CLK_2 : PMC_MCKR_PRES_CLK_4) ;
        while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;
        // CSS after
        PMC->PMC_MCKR = PMC_MCKR_CSS_PLLA_CLK |
                                   ((use_div == 2) ? PMC_MCKR_PRES_CLK_2 : PMC_MCKR_PRES_CLK_4) ;
        while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;

        current_clock = ((EXTERNAL_CLOCK/2) * use_mul) / use_div ;

    } else if (desired_clock > 32768) {
        // EXTERNAL clock: CSS first
        // CSS first: use master clock with old PRES
        PMC->PMC_MCKR = PMC_MCKR_CSS_MAIN_CLK | ((PMC->PMC_MCKR) & PMC_MCKR_PRES)  ;
        while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;
        
        // PRES after
        tmpreg = PMC_MCKR_CSS_MAIN_CLK |
                        ((use_div==1) ? PMC_MCKR_PRES_CLK :
                                ((use_div==2) ? PMC_MCKR_PRES_CLK_2 :
                                                        PMC_MCKR_PRES_CLK_4)) ;
        if (tmpreg != PMC->PMC_MCKR) {
            PMC->PMC_MCKR = tmpreg ;
            while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;
        }
        
        // disable PLL
        PMC->CKGR_PLLAR = 0x3f00 ;      // value at reset

        current_clock = EXTERNAL_CLOCK / use_div ;

    } else {
        // SLOW clock
        // CSS first: use slow clock with old PRES
        PMC->PMC_MCKR = PMC_MCKR_CSS_SLOW_CLK | ((PMC->PMC_MCKR) & PMC_MCKR_PRES)  ;
        while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;
        // PRES after
        PMC->PMC_MCKR = PMC_MCKR_CSS_SLOW_CLK | 
                        ((use_div==1) ? PMC_MCKR_PRES_CLK :
                                ((use_div==2) ? PMC_MCKR_PRES_CLK_2 :
                                                        PMC_MCKR_PRES_CLK_4)) ;
        while(!(PMC->PMC_SR & PMC_SR_MCKRDY)) ;

        // disable PLL
        PMC->CKGR_PLLAR = 0x3f00 ;      // value at reset
        // disable main oscillator
        PMC->CKGR_MOR = (0x37 << 16) ;

        current_clock = 32768 / use_div ;
    }

    // Set Flash Waite state
    // <25 MHz: 1 WS
    // <50 MHz: 2 WS
    // <64 MHz: 3 WS
    if (current_clock < 25000000) {
        EFC->EEFC_FMR = 0 << 8 ;    // set 0+1 = 1 Wait States
    } else if (current_clock < 50000000) {
        EFC->EEFC_FMR = 1 << 8 ;    // set 1+1 = 2 Wait States
    } else {
        EFC->EEFC_FMR = 2 << 8 ;    // set 2+1 = 3 Wait States
    }

    // done
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

// EFC commands
#define EFC_FCMD_GETD    0x00
#define EFC_FCMD_WP      0x01
#define EFC_FCMD_WPL     0x02
#define EFC_FCMD_EWP     0x03
#define EFC_FCMD_EWPL    0x04
#define EFC_FCMD_EA      0x05
#define EFC_FCMD_SLB     0x08
#define EFC_FCMD_CLB     0x09
#define EFC_FCMD_GLB     0x0A
#define EFC_FCMD_SFB     0x0B
#define EFC_FCMD_CFB     0x0C
#define EFC_FCMD_GFB     0x0D
#define EFC_FCMD_STUI    0x0E
#define EFC_FCMD_SPUI    0x0F

void ProgramCopy(void)
{
    int pg, i, bbegin, flag_towrite ;
    unsigned char buftx[8]  ;
    unsigned char flashbuf[AT91C_IFLASH_PAGE_SIZE] ;
    int len ;
    extern unsigned char flashcode ;

    // This function MUST be executed with interrupt disabled,
    // no other actions possible

    // First step: erase all flash
    // MC_FMR has correct value for Flash Write
    EFC->EEFC_FCR = (EEFC_FCR_FKEY & (0x5a<<24)) | EFC_FCMD_EA ;

    // Wait for end of flash erase
    while(!(EFC->EEFC_FSR & EEFC_FSR_FRDY))
        ;

    // write flash: 1024 pages of 256 bytes
    for(pg=0 ; pg<AT91C_IFLASH_NOF_PAGES ; pg++) {

        // NPCS0 --\__
        PIOA->PIO_CODR = PIO_PA11A_NPCS0 ;

        // read page from SPI flash
        bbegin = CODEFLASH_START + (pg * AT91C_IFLASH_PAGE_SIZE) ;

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

        SPI->SPI_TPR = (unsigned long)(buftx) ;     // TX buffer 1 ptr
        SPI->SPI_TCR = len ;                        // TX buffer 1 len
        SPI->SPI_TNPR = (unsigned long)(flashbuf) ; // TX buffer 2 ptr
        SPI->SPI_TNCR = AT91C_IFLASH_PAGE_SIZE ;    // TX buffer 2 len

        SPI->SPI_RPR = (unsigned long)(buftx) ;     // RX buffer 1 ptr
        SPI->SPI_RCR = len ;                        // RX buffer 1 len
        SPI->SPI_RNPR = (unsigned long)(flashbuf) ; // RX buffer 2 ptr
        SPI->SPI_RNCR = AT91C_IFLASH_PAGE_SIZE ;    // RX buffer 2 len

        // enable DMA
        SPI->SPI_PTCR = SPI_PTCR_TXTEN | SPI_PTCR_RXTEN ;

        // wait for end of buffer
        while (!(SPI->SPI_SR & SPI_SR_RXBUFF))
            ;

        // Disable DMA
        SPI->SPI_PTCR = SPI_PTCR_TXTDIS | SPI_PTCR_RXTDIS ;

        // wait end of transfer
        while (!(SPI->SPI_SR & SPI_SR_TDRE))
            ;

        flag_towrite = NO ;    // default
        for(i=0 ; i<AT91C_IFLASH_PAGE_SIZE ; i++) {
            if (flashbuf[i] != 0xff) {  // really to write ?
                flag_towrite = YES ;
                break ;
            }
        }

        // NPCS0 __/--
        PIOA->PIO_SODR = PIO_PA11A_NPCS0 ;

        // write page buffer (always from base address)
        if (flag_towrite) {
            for(i=0 ; i<AT91C_IFLASH_PAGE_SIZE ; i+=4) {
                *((unsigned long *)(AT91C_IFLASH + i)) = *((unsigned long *)(flashbuf + i)) ;
            }

            // write desired page number
//            EFC->EEFC_FCR = (EEFC_FCR_FKEY & (0x5a<<24))
//                                  | (EEFC_FCR_FARG & (pg<<8)) ;

            // write desired page number + write page command
            EFC->EEFC_FCR = (EEFC_FCR_FKEY & (0x5a<<24))
                                  | (EEFC_FCR_FARG & (pg<<8))
                                  |  EFC_FCMD_WP ;

            // Wait for end of flash page program
            while(!(EFC->EEFC_FSR & EEFC_FSR_FRDY))
                ;
        }
    }

    // Make software reset
    RSTC->RSTC_CR = (RSTC_CR_KEY & (0xa5<<24)) |
                                 RSTC_CR_PROCRST | // (RSTC) Processor Reset
                                 RSTC_CR_PERRST  | // (RSTC) Peripheral Reset
                                 RSTC_CR_EXTRST ;  // (RSTC) External Reset
                                 
    for( ; ; ) ;        // just to be sure
}

#else // USE_SPI_ON_ARM
//----------------------------------------------------------------------------
// RAM ProgramCopy: NO RETURN

void ProgramCopy(void)
{
#error "TODO"
    for( ; ; ) ;        // just to be sure
}
#endif // USE_SPI_ON_ARM
// end of file - RTXCutil.c

