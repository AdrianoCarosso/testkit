// RTXCutil.c
//
//   Copyright (c) 1997-2007.
//   T.E.S.T. srl
//

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

// -----------------------------------------------------------------------------
// internal functions prototype

void AT91F_CLOCKinit(unsigned long desired_clock) ;

//----------------------------------------------------------------------------

#undef USE_DEBUG

//----------------------------------------------------------------------------
// Debug only

#ifdef USE_DEBUG
#define PUT_OCT(A) { PUT_CHAR('0' + (((A)>>6) & 7)) ; PUT_CHAR('0' + (((A)>>3) & 7)) ; PUT_CHAR('0' + ((A) & 7)) ; }

#define PUT_CHAR(A) { \
    while (!(AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_TXRDY)) ; \
        AT91C_BASE_DBGU->DBGU_THR = (A) & 0xff ; \
    }
#endif // USE_DEBUG

//----------------------------------------------------------------------------
// AT91F_CLOCKinit
// This function performs very low level HW PLL initialization

// this function is also called by RTXCstartup.S BEFORE BSS CLEARING
unsigned long current_clock NOINIT_ATTRIBUTE ;

void AT91F_CLOCKinit(unsigned long desired_clock)
{
    register int use_div ;
    register int use_mul ;
    unsigned long tmpreg ;

    if (!desired_clock) {       // zero means use default
        desired_clock = EXTERNAL_CLOCK ;
    } else {
        // SLOW clock
        // CSS first: use slow clock with old PRES
        AT91C_BASE_PMC->PMC_MCKR = ((AT91C_BASE_PMC->PMC_MCKR) & AT91C_PMC_PRES) | AT91C_PMC_CSS_SLOW_CLK ;
        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;
        // PRES after
        AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_CSS_SLOW_CLK | AT91C_PMC_PRES_CLK ;
        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;

        // just to be sure, correct values settlede hereafter
        AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&( (((desired_clock*3)/2000000)+1) <<16))
                                | AT91C_MC_FWS_2FWS     // wait states
                                | AT91C_MC_NEBP ;       // no erase before prog
    }
    
    // Set MCK at desired_clock
    // 1 Enabling the Main Oscillator:
    if (desired_clock > 32768) {        // we need external clock
        if (!((AT91C_BASE_PMC->PMC_MOR) & AT91C_CKGR_MOSCEN)) {
            // main oscillator is disabled, turn it on
            // SCK = 1/32768 = 30.51 uSecond
            // Start up time = 8 * 6 / SCK = 56 * 30.51 = 1,46484375 ms
            AT91C_BASE_PMC->PMC_MOR = ( (AT91C_CKGR_OSCOUNT) & (0x06 <<8)) | AT91C_CKGR_MOSCEN ;
            // Wait the startup time
            while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MOSCS)) ;
        }
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
        // - NOUSB Mul 11+1 - Fout: 8 MHz * 12 = 96 MHz
        // - USB   Mul 14+1 - Fout: 8 MHz * 15 = 120 MHz
        // Field out NOT USED = 0 (PLL range 80 - 160 MHz)
        // PLLCOUNT pll startup time estimate at : 0.865 ms
        // PLLCOUNT 28 = 0.000865 /(1/32768)
        AT91C_BASE_PMC->PMC_PLLR = ((AT91C_CKGR_DIV & 2) |
                            (AT91C_CKGR_PLLCOUNT & (60/*28*/<<8)) |
                            (AT91C_CKGR_MUL & ((use_mul - 1)<<16))) ;

        // examples:
        // CLOCK   CLOCK/2   use_mul   PLL  use_div  MAIN  USB
        //   16       8         15     120      2      60       .
        //   16       8         14     112      2      56       .
        //   16       8         13     104      2      52       .
        //   16       8         12      96      2      48   ok  .
        //   16       8         10      80      2      40       .
        //   16       8         15     120      4      30       .
        //   16       8         12      96      4      24   ok  .
        //   16       8         10      80      4      20       .

        // Wait the startup time
        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_LOCK)) ;
        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;
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
        AT91C_BASE_PMC->PMC_MCKR = ((AT91C_BASE_PMC->PMC_MCKR) & AT91C_PMC_CSS) |
                                   ((use_div == 2) ? AT91C_PMC_PRES_CLK_2 : AT91C_PMC_PRES_CLK_4) ;
        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;
        // CSS after
        AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_CSS_PLL_CLK |
                                   ((use_div == 2) ? AT91C_PMC_PRES_CLK_2 : AT91C_PMC_PRES_CLK_4) ;

        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;

        current_clock = ((EXTERNAL_CLOCK/2) * use_mul) / use_div ;

    } else if (desired_clock > 32768) {
        // EXTERNAL clock: CSS first
        // CSS first: use master clock with old PRES
        AT91C_BASE_PMC->PMC_MCKR = ((AT91C_BASE_PMC->PMC_MCKR) & AT91C_PMC_PRES) | AT91C_PMC_CSS_MAIN_CLK ;
        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;
        
        // PRES after
        tmpreg = AT91C_PMC_CSS_MAIN_CLK |
                        ((use_div==1) ? AT91C_PMC_PRES_CLK :
                                ((use_div==2) ? AT91C_PMC_PRES_CLK_2 :
                                                        AT91C_PMC_PRES_CLK_4)) ;
        if (tmpreg != AT91C_BASE_PMC->PMC_MCKR) {
            AT91C_BASE_PMC->PMC_MCKR = tmpreg ;
            while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;
        }
        
        // disable PLL
        AT91C_BASE_PMC->PMC_PLLR = 0x3f00 ;     // value at reset

        current_clock = EXTERNAL_CLOCK / use_div ;

    } else {
        // SLOW clock
        // CSS first: use slow clock with old PRES
        AT91C_BASE_PMC->PMC_MCKR = ((AT91C_BASE_PMC->PMC_MCKR) & AT91C_PMC_PRES) | AT91C_PMC_CSS_SLOW_CLK ;
        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;
        // PRES after
        AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_CSS_SLOW_CLK |
                                   ((use_div==1) ? AT91C_PMC_PRES_CLK :
                                        ((use_div==2) ? AT91C_PMC_PRES_CLK_2 :
                                                            AT91C_PMC_PRES_CLK_4)) ;
        while(!(AT91C_BASE_PMC->PMC_SR & AT91C_PMC_MCKRDY)) ;

        // disable PLL
        AT91C_BASE_PMC->PMC_PLLR = 0x3f00 ;     // value at reset
        // disable main oscillator
        AT91C_BASE_PMC->PMC_MOR = 0 ;

        current_clock = 32768 / use_div ;
    }

    // Set Flash Waite state
    // examples:
    // MCK = 48000000 means 72 Cycle for 1.5 usecond
    // MCK = 60000000 means 90 Cycle for 1.5 usecond
    // 1 Wait State necessary to work > 30MHz
    AT91C_BASE_MC->MC_FMR = ((AT91C_MC_FMCN)&( (((current_clock*3)/2000000)+1) <<16))
                            | ( (current_clock>=60000000) ? AT91C_MC_FWS_2FWS : // wait states
                                        ((current_clock>=30000000) ? AT91C_MC_FWS_1FWS :
                                                                              AT91C_MC_FWS_0FWS) )
                            | AT91C_MC_NEBP ;           // no erase before prog

    // done
}

//----------------------------------------------------------------------------
// SPI ProgramCopy: NO RETURN
// Copy program from SPI Flash to Internal Flash
// only if we are well accepted
#ifdef USE_SPI_ON_ARM

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#define AT91C_MC_EOP    AT91C_MC_FRDY
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

void ProgramCopy(void)
{
#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI0 ;
#endif // USE_REAL_BOARD

#ifdef USE_EVALUATION_BOARD
    AT91PS_SPI const pSPI = AT91C_BASE_SPI1 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    AT91PS_SPI const pSPI = AT91C_BASE_SPI ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

    int pg, i, bbegin, flag_towrite ;
    unsigned char buftx[8]  ;
    unsigned char flashbuf[AT91C_IFLASH_PAGE_SIZE] ;
    int len ;
    extern unsigned char flashcode ;

    // This function MUST be executed with interrupt disabled,
    // no other actions possible

    // First step: erase all flash
    // MC_FMR has correct value for Flash Write (set in drv_clk.c)
    AT91C_BASE_MC->MC_FCR = (AT91C_MC_KEY & (0x5a<<24)) | AT91C_MC_FCMD_ERASE_ALL ;

    // Wait for end of flash erase
    while(!(AT91C_BASE_MC->MC_FSR & AT91C_MC_EOP))
        ;

    // write flash: 1024 pages of 256 bytes
    for(pg=0 ; pg<AT91C_IFLASH_NB_OF_PAGES ; pg++) {

#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
        // NPCS0 --\__
        AT91C_BASE_PIOA->PIO_CODR = AT91C_PA11_SPI0_NPCS0 ;
#endif // USE_REAL_BOARD
#ifdef USE_EVALUATION_BOARD
        // NPCS3 --\__
        AT91C_BASE_PIOA->PIO_CODR = AT91C_PA7_SPI1_NPCS3 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        // NPCS0 --\__
        AT91C_BASE_PIOA->PIO_CODR = AT91C_PA11_NPCS0 ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

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

        pSPI->SPI_TPR = (unsigned long)(buftx) ;        // TX buffer 1 ptr
        pSPI->SPI_TCR = len ;                           // TX buffer 1 len
        pSPI->SPI_TNPR = (unsigned long)(flashbuf) ;    // TX buffer 2 ptr
        pSPI->SPI_TNCR = AT91C_IFLASH_PAGE_SIZE ;       // TX buffer 2 len

        pSPI->SPI_RPR = (unsigned long)(buftx) ;        // RX buffer 1 ptr
        pSPI->SPI_RCR = len ;                           // RX buffer 1 len
        pSPI->SPI_RNPR = (unsigned long)(flashbuf) ;    // RX buffer 2 ptr
        pSPI->SPI_RNCR = AT91C_IFLASH_PAGE_SIZE ;       // RX buffer 2 len

        // enable DMA
        pSPI->SPI_PTCR = AT91C_PDC_TXTEN | AT91C_PDC_RXTEN ;

        // wait for end of buffer
        while (!(pSPI->SPI_SR & AT91C_SPI_RXBUFF))
            ;

        // Disable DMA
        pSPI->SPI_PTCR = AT91C_PDC_TXTDIS | AT91C_PDC_RXTDIS ;

        // wait end of transfer
        while (!(pSPI->SPI_SR & AT91C_SPI_TDRE))
            ;

        flag_towrite = NO ;    // default
        for(i=0 ; i<AT91C_IFLASH_PAGE_SIZE ; i++) {
            if (flashbuf[i] != 0xff) {  // really to write ?
                flag_towrite = YES ;
                break ;
            }
        }

#if defined(USE_AT91SAM7A3)
#ifdef USE_REAL_BOARD
        // NPCS0 __/--
        AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_SPI0_NPCS0 ;
#endif // USE_REAL_BOARD
#ifdef USE_EVALUATION_BOARD
        // NPCS3 __/--
        AT91C_BASE_PIOA->PIO_SODR = AT91C_PA7_SPI1_NPCS3 ;
#endif // USE_EVALUATION_BOARD
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        // NPCS0 __/--
        AT91C_BASE_PIOA->PIO_SODR = AT91C_PA11_NPCS0 ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

        // write page buffer (always from base address)
        if (flag_towrite) {
            for(i=0 ; i<AT91C_IFLASH_PAGE_SIZE ; i+=4) {
                *((unsigned long *)(AT91C_IFLASH + i)) = *((unsigned long *)(flashbuf + i)) ;
            }

            // write desired page number
            AT91C_BASE_MC->MC_FCR = (AT91C_MC_KEY & (0x5a<<24))
                                  | (AT91C_MC_PAGEN & (pg<<8)) ;

            // write desired page number + write page command
            AT91C_BASE_MC->MC_FCR = (AT91C_MC_KEY & (0x5a<<24))
                                  | (AT91C_MC_PAGEN & (pg<<8))
                                  |  AT91C_MC_FCMD_START_PROG ;

            // Wait for end of flash page program
            while(!(AT91C_BASE_MC->MC_FSR & AT91C_MC_EOP))
                ;
        }
    }

    // Make software reset
    AT91C_BASE_RSTC->RSTC_RCR = (AT91C_RSTC_KEY & (0xa5<<24)) |
                                 AT91C_RSTC_PROCRST | // (RSTC) Processor Reset
                                 AT91C_RSTC_PERRST  | // (RSTC) Peripheral Reset
                                 AT91C_RSTC_EXTRST ;  // (RSTC) External Reset
                                 
    for( ; ; ) ;        // just to be sure
}
void ProgramCopyEnd(void)
{
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

    // First step: erase all flash
    // MC_FMR has correct value for Flash Write (set in drv_clk.c)
    AT91C_BASE_MC->MC_FCR = (AT91C_MC_KEY & (0x5a<<24)) | AT91C_MC_FCMD_ERASE_ALL ;

#if defined(USE_AT91SAM7A3)
    // Wait for end of flash erase
    while(!(AT91C_BASE_MC->MC_FSR & AT91C_MC_EOP))
        ;
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    // Wait for end of flash erase
    while(!(AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY))
        ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

    // write flash: 1024 pages of 256 bytes
    for(pg=0 ; pg<AT91C_IFLASH_NB_OF_PAGES ; pg++) {
        bbegin = pg * AT91C_IFLASH_PAGE_SIZE ;
        uptr = &ramcode[bbegin >> 2] ;
        
        flag_towrite = NO ;    // default
        for(i=0 ; i<(AT91C_IFLASH_PAGE_SIZE >> 2) ; i++) {
            if (uptr[i] != 0xffffffff) {  // really to write ?
                flag_towrite = YES ;
                break ;
            }
        }

        // write page buffer (always from base address)
        if (flag_towrite) {
            for(i=0 ; i<AT91C_IFLASH_PAGE_SIZE ; i+=4) {
                *((unsigned long *)(AT91C_IFLASH + i)) = uptr[i/4] ;
            }

            // write desired page number
            AT91C_BASE_MC->MC_FCR = (AT91C_MC_KEY & (0x5a<<24))
                                  | (AT91C_MC_PAGEN & (pg<<8)) ;

            // write desired page number + write page command
            AT91C_BASE_MC->MC_FCR = (AT91C_MC_KEY & (0x5a<<24))
                                  | (AT91C_MC_PAGEN & (pg<<8))
                                  |  AT91C_MC_FCMD_START_PROG ;

#if defined(USE_AT91SAM7A3)
            // Wait for end of flash page program
            while(!(AT91C_BASE_MC->MC_FSR & AT91C_MC_EOP))
                ;
#endif // defined(USE_AT91SAM7A3)
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
            // Wait for end of flash page program
            while(!(AT91C_BASE_MC->MC_FSR & AT91C_MC_FRDY))
                ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
        }
    }

    // Make software reset
    AT91C_BASE_RSTC->RSTC_RCR = (AT91C_RSTC_KEY & (0xa5<<24)) |
                                 AT91C_RSTC_PROCRST | // (RSTC) Processor Reset
                                 AT91C_RSTC_PERRST  | // (RSTC) Peripheral Reset
                                 AT91C_RSTC_EXTRST ;  // (RSTC) External Reset

    for( ; ; ) ;        // just to be sure
}
void ProgramCopyEnd(void)
{
}
#endif // USE_SPI_ON_ARM
// end of file - RTXCProgramCopy.c

