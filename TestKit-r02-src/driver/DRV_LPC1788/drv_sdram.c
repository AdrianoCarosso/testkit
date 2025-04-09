// drv_sdram.c - SDRAM driver tasks
//
//   Copyright (c) 1997-2011.
//   T.E.S.T. srl
//

//
// This module is provided as a SDRAM driver.
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

#ifdef USE_SDRAM

//#define SDRAM_TEST_LEN  (SDRAM_SIZE/0x40000)    // deep test
#define SDRAM_TEST_LEN  (8)                     // light test

#ifndef CBUG
// #define USE_SDRAM_DEBUG
#endif // CBUG

//----------------------------------------------------------------------------
// Local variables

static volatile uint32_t ringosccount[2] = {0,0};

//----------------------------------------------------------------------------
// Local Functions

int sdram_test(void)
{
    volatile uint32_t * volatile wr_ptr;
    volatile uint16_t * volatile short_wr_ptr;
    uint32_t data;
    int i, j ;

    wr_ptr = (uint32_t *)(SDRAM_BASE) ;
    short_wr_ptr = (uint16_t *)(SDRAM_BASE);
    /* Clear content before 16 bit access test */
//  for (i = 0; i < SDRAM_SIZE/4; i++) {
//	    *wr_ptr++ = 0;
//  }

    // 16 bit write
    for(i = 0; i < SDRAM_TEST_LEN ; i++) {
        for (j = 0; j < 256 ; j++) {
            *short_wr_ptr++ = (i + j);
            *short_wr_ptr++ = (i + j) + 1;
        }
    }

    // Verifying
    wr_ptr = (uint32_t *)SDRAM_BASE;
    for (i = 0; i < SDRAM_TEST_LEN ; i++) {
        for (j = 0; j < 256 ; j++) {
            data = *wr_ptr;
            if (data != (((((i + j) + 1) & 0xFFFF) << 16) | ((i + j) & 0xFFFF))) {
#ifdef USE_SDRAM_DEBUG
                printf("i=%d, j=%d, data=%lx, val=%x\n", i, j, data, (((((i + j) + 1) & 0xFFFF) << 16) | ((i + j) & 0xFFFF)) ) ;
#endif // USE_SDRAM_DEBUG
                return(0) ;
            }
            wr_ptr++;
        }
    }
    return(1) ;
}

static uint32_t find_cmddly(void)
{
    int cmddlystart, cmddlyend, cmddly ;
    uint32_t dwtemp ;
    int ppass = 0, pass = 0 ;

    cmddly = 0 ;
    cmddlystart = cmddlyend = 0xFF;

    while (cmddly < 32) {
#ifdef USE_SDRAM_DEBUG
        printf("------ cmddly=%d\n", cmddly) ;
#endif // USE_SDRAM_DEBUG
        dwtemp = LPC_SC->EMCDLYCTL & ~0x1F;
        LPC_SC->EMCDLYCTL = dwtemp | cmddly;

        if (sdram_test() == 0x1) {
            /* Test passed */
            if (cmddlystart == 0xFF) {
                cmddlystart = cmddly;
            }
            ppass = 1 ;
        } else {
            /* Test failed */
            if (ppass == 1) {
                cmddlyend = cmddly;
                pass = 1 ;
                ppass = 0 ;
            }
        }

        /* Try next value */
        cmddly++;
    }

    /* If the test passed, the we can use the average of the min and max values to get an optimal DQSIN delay */
    if (pass == 1) {
        cmddly = (cmddlystart + cmddlyend) / 2;
    } else if (ppass == 1) {
        cmddly = (cmddlystart + 0x1F) / 2;
    } else {
        /* A working value couldn't be found, just pick something safe so the system doesn't become unstable */
        cmddly = 0x10;
    }
#ifdef USE_SDRAM_DEBUG
    printf("------ using cmddly=%d\n", cmddly) ;
#endif // USE_SDRAM_DEBUG

    dwtemp = LPC_SC->EMCDLYCTL & ~0x1F;
    LPC_SC->EMCDLYCTL = dwtemp | cmddly;

    return(pass | ppass) ;
}

static uint32_t find_fbclkdly(void)
{
    int fbclkdly, fbclkdlystart, fbclkdlyend ;
    uint32_t dwtemp ;
    int ppass = 0, pass = 0 ;

    fbclkdly = 0 ;
    fbclkdlystart = fbclkdlyend = 0xFF ;

    while(fbclkdly < 32) {
        dwtemp = LPC_SC->EMCDLYCTL & ~0x1F00;
        LPC_SC->EMCDLYCTL = dwtemp | (fbclkdly << 8);

        if (sdram_test() == 1) {
            /* Test passed */
            if (fbclkdlystart == 0xFF) {
                fbclkdlystart = fbclkdly;
            }
            ppass = 1 ;
        } else {
            /* Test failed */
            if (ppass == 1) {
                fbclkdlyend = fbclkdly;
                pass = 1 ;
                ppass = 0 ;
            }
        }

        /* Try next value */
        fbclkdly++;
    }

    /* If the test passed, the we can use the average of the min and max values to get an optimal DQSIN delay */
    if (pass == 0x1) {
        fbclkdly = (fbclkdlystart + fbclkdlyend) / 2;
    } else if (ppass == 0x1) {
        fbclkdly = (fbclkdlystart + 0x1F) / 2;
    } else {
        /* A working value couldn't be found, just pick something safe so the system doesn't become unstable */
        fbclkdly = 0x10;
    }

#ifdef USE_SDRAM_DEBUG
    printf("------ using fbclkdly=%d\n", fbclkdly) ;
#endif // USE_SDRAM_DEBUG

    dwtemp = LPC_SC->EMCDLYCTL & ~0x1F00;
    LPC_SC->EMCDLYCTL = dwtemp | (fbclkdly << 8);

    return(pass | ppass) ;
}

static uint32_t calibration( void )
{
    uint32_t dwtemp ;
    int i, cnt = 0;

    for (i = 0; i < 10; i++) {
        dwtemp = LPC_SC->EMCCAL & ~0x4000;
        LPC_SC->EMCCAL = dwtemp | 0x4000;

        dwtemp = LPC_SC->EMCCAL;
        while ((dwtemp & 0x8000) == 0x0000) {
            dwtemp = LPC_SC->EMCCAL;
        }
        cnt += (dwtemp & 0xFF);
    }
#ifdef USE_SDRAM_DEBUG
    printf("------ using cnt=%d\n", cnt / 10) ;
#endif // USE_SDRAM_DEBUG
    return(cnt / 10) ;
}

//----------------------------------------------------------------------------
// Public Functions

void adjust_timing( void )
{
    uint32_t dwtemp, cmddly, fbclkdly;

    /* Current value */
    ringosccount[1] = calibration();

    dwtemp   = LPC_SC->EMCDLYCTL;
    cmddly   = ((dwtemp & 0x1F) * ringosccount[0] / ringosccount[1]) & 0x1F;
    fbclkdly = ((dwtemp & 0x1F00) * ringosccount[0] / ringosccount[1]) & 0x1F00;
    LPC_SC->EMCDLYCTL = (dwtemp & ~0x1F1F) | fbclkdly | cmddly;
}

//----------------------------------------------------------------------------
// sdramstart()

int sdramstart (void)
{
    volatile int i ;
    volatile uint32_t dwtemp ;
    //volatile uint16_t wtemp ;

    LPC_SC->PCONP |= CLKPWR_PCONP_PCEMC ;

    //LPC_SC->CLKOUTCFG  = 0x110 ;    // enable clockout at cpuclock/2

    LPC_SC->EMCDLYCTL  = 0x00001010 ;
    LPC_EMC->Control   = 0x00000001 ;
    LPC_EMC->Config    = 0x00000000 ;

    //  pinConfig(); //Full 32-bit Data bus, 24-bit Address
    // Configure memory layout, but MUST DISABLE BUFFERs during configuration
    // _BM_ original comment ???: 256MB, 8Mx32, 4 banks, row=12, column=9
    LPC_EMC->DynamicConfig0 = 0x00004480 ; // 128 Mb (8Mx16), 4 banks, row length = 12, column length = 9

    // Configure memory layout, but MUST DISABLE BUFFERs during configuration
    // row-bank-column: 8Mx32 = 32MByte = 256Mbit -> 4 banks, row length = 13, column length = 8
    //LPC_EMC->DynamicConfig0 = 0x00004700 ;

    // Configure timing for ISSI IS4x32800D SDRAM

#if (SDRAM_SPEED==SDRAM_SPEED_48)
    //Timing for 48MHz Bus
    LPC_EMC->DynamicRasCas0    = 0x00000201; /* 1 RAS, 2 CAS latency */
    LPC_EMC->DynamicReadConfig = 0x00000001; /* Command delayed strategy, using EMCCLKDELAY */
    LPC_EMC->DynamicRP         = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
    LPC_EMC->DynamicRAS        = 0x00000002; /* ( n + 1 ) -> 3 clock cycles */
    LPC_EMC->DynamicSREX       = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicAPR        = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicDAL        = 0x00000002; /* ( n ) -> 2 clock cycles */
    LPC_EMC->DynamicWR         = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicRC         = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicRFC        = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicXSR        = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicRRD        = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
    LPC_EMC->DynamicMRD        = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
#elif (SDRAM_SPEED==SDRAM_SPEED_50)
    //Timing for 50MHz Bus (with 100MHz M3 Core)
    LPC_EMC->DynamicRasCas0    = 0x00000201; /* 1 RAS, 2 CAS latency */
    LPC_EMC->DynamicReadConfig = 0x00000001; /* Command delayed strategy, using EMCCLKDELAY */
    LPC_EMC->DynamicRP         = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
    LPC_EMC->DynamicRAS        = 0x00000002; /* ( n + 1 ) -> 3 clock cycles */
    LPC_EMC->DynamicSREX       = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicAPR        = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicDAL        = 0x00000002; /* ( n ) -> 2 clock cycles */
    LPC_EMC->DynamicWR         = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicRC         = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicRFC        = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicXSR        = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicRRD        = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
    LPC_EMC->DynamicMRD        = 0x00000000; /* ( n + 1 ) -> 1 clock cycles */
#elif (SDRAM_SPEED==SDRAM_SPEED_60)
    //Timing for 60 MHz Bus (same as 72MHz)
    LPC_EMC->DynamicRasCas0    = 0x00000202; /* 2 RAS, 2 CAS latency */
    LPC_EMC->DynamicReadConfig = 0x00000001; /* Command delayed strategy, using EMCCLKDELAY */
    LPC_EMC->DynamicRP         = 0 ;//0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicRAS        = 2 ;//0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicSREX       = 2 ;//0x00000005; /* ( n + 1 ) -> 6 clock cycles */
    LPC_EMC->DynamicAPR        = 1 ;//0x00000002; /* ( n + 1 ) -> 3 clock cycles */
    LPC_EMC->DynamicDAL        = 2 ;//0x00000003; /* ( n ) -> 3 clock cycles */
    LPC_EMC->DynamicWR         = 1 ;//0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicRC         = 3 ;//0x00000004; /* ( n + 1 ) -> 5 clock cycles */
    LPC_EMC->DynamicRFC        = 3 ;//0x00000004; /* ( n + 1 ) -> 5 clock cycles */
    LPC_EMC->DynamicXSR        = 3 ;//0x00000005; /* ( n + 1 ) -> 6 clock cycles */
    LPC_EMC->DynamicRRD        = 0 ;//0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicMRD        = 0 ;//0x00000001; /* ( n + 1 ) -> 2 clock cycles */
#elif (SDRAM_SPEED==SDRAM_SPEED_72)
    //Timing for 72 MHz Bus
    LPC_EMC->DynamicRasCas0    = 0x00000202; /* 2 RAS, 2 CAS latency */
    LPC_EMC->DynamicReadConfig = 0x00000001; /* Command delayed strategy, using EMCCLKDELAY */
    LPC_EMC->DynamicRP         = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicRAS        = 0x00000003; /* ( n + 1 ) -> 4 clock cycles */
    LPC_EMC->DynamicSREX       = 0x00000005; /* ( n + 1 ) -> 6 clock cycles */
    LPC_EMC->DynamicAPR        = 0x00000002; /* ( n + 1 ) -> 3 clock cycles */
    LPC_EMC->DynamicDAL        = 0x00000003; /* ( n ) -> 3 clock cycles */
    LPC_EMC->DynamicWR         = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicRC         = 0x00000004; /* ( n + 1 ) -> 5 clock cycles */
    LPC_EMC->DynamicRFC        = 0x00000004; /* ( n + 1 ) -> 5 clock cycles */
    LPC_EMC->DynamicXSR        = 0x00000005; /* ( n + 1 ) -> 6 clock cycles */
    LPC_EMC->DynamicRRD        = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
    LPC_EMC->DynamicMRD        = 0x00000001; /* ( n + 1 ) -> 2 clock cycles */
#else
	#error UNSUPPORTED SDRAM FREQ
#endif

    for(i=0 ; i<100 ; i++) tickwait(1000) ; // wait 100ms
    LPC_EMC->DynamicControl = 0x00000183 ;  // Issue NOP command
    for(i=0 ; i<200 ; i++) tickwait(1000) ; // wait 200ms
    LPC_EMC->DynamicControl = 0x00000103 ;  // Issue PALL command
    LPC_EMC->DynamicRefresh = 0x00000002 ;  // ( n * 16 ) -> 32 clock cycles
    for(i = 0; i < 0x80; i++) ;             // wait 128 AHB clock cycles


#if (SDRAM_SPEED==SDRAM_SPEED_48)
    //Timing for 48MHz Bus
    LPC_EMC->DynamicRefresh    = 0x0000002E; /* ( n * 16 ) -> 736 clock cycles -> 15.330uS at 48MHz <= 15.625uS ( 64ms / 4096 row ) */
#elif (SDRAM_SPEED==SDRAM_SPEED_50)
    //Timing for 50MHz Bus
    LPC_EMC->DynamicRefresh    = 0x0000003A; /* ( n * 16 ) -> 768 clock cycles -> 15.360uS at 50MHz <= 15.625uS ( 64ms / 4096 row ) */
#elif (SDRAM_SPEED==SDRAM_SPEED_60)
    //Timing for 60MHz Bus
    LPC_EMC->DynamicRefresh    = 0x0000003A; /* ( n * 16 ) -> 928 clock cycles -> 15.466uS at 60MHz <= 15.625uS ( 64ms / 4096 row ) */
#elif (SDRAM_SPEED==SDRAM_SPEED_72)
    //Timing for 72MHz Bus
    LPC_EMC->DynamicRefresh    = 0x00000046; /* ( n * 16 ) -> 1120 clock cycles -> 15.556uS at 72MHz <= 15.625uS ( 64ms / 4096 row ) */
#else
	#error UNSUPPORTED SDRAM FREQ
#endif

    LPC_EMC->DynamicControl    = 0x00000083; /* Issue MODE command */
    //Timing for 48/60/72MHZ Bus
    dwtemp = *((volatile uint32_t *)(SDRAM_BASE | (0x22<<(2+2+9)))); /* 4 burst, 2 CAS latency */
    LPC_EMC->DynamicControl    = 0x00000000; /* Issue NORMAL command */

    //[re]enable buffers
    LPC_EMC->DynamicConfig0    = 0x00084480; /* 256MB, 8Mx32, 4 banks, row=12, column=9 */
    //LPC_EMC->DynamicConfig0    = 0x00084700 ;

    /* Nominal value */
    ringosccount[0] = calibration();

    if (find_cmddly() == 0x0) {
#ifdef USE_SDRAM_DEBUG
        printf("find_cmddly fatal error\n") ;
#endif // USE_SDRAM_DEBUG
        return(FALSE) ;
    }

    if (find_fbclkdly() == 0x0) {
#ifdef USE_SDRAM_DEBUG
        printf("find_fbclkdly fatal error\n") ;
#endif // USE_SDRAM_DEBUG
        return(FALSE) ;
    }

    adjust_timing() ;

    return(TRUE) ;
}

//----------------------------------------------------------------------------
// sdramstop()

void sdramstop(void)
{
    LPC_EMC->Control   = 0 ;    // disable
    LPC_SC->PCONP &= ~CLKPWR_PCONP_PCEMC ;
}
#endif // USE_SDRAM
