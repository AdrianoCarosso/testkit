// drv_dio.c - digital I/O driver tasks

//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

//
// This module is provided as a digital I/O port driver.
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"
#include "cres.h"

#include "extapi.h"

#include "assign.h"

#ifdef MTS_CODE_
#define BUG_ACC_CHIP
#endif

#ifdef BUG_ACC_CHIP
#define ACCMASK_ON  (HW_Bo0_SRV_ON | HW_Bo6_ACC_BIT0 | HW_Bo7_ACC_BIT1)
#define ACCMASK_OFF  (~(HW_Bo0_SRV_ON | HW_Bo6_ACC_BIT0 | HW_Bo7_ACC_BIT1))
#endif

#ifdef USE_MAX7324_TW1  // I/O n. 1
short tw1in_change = 0 ;
#endif

#ifdef USE_MAX7324_TW2  // I/O n. 2
short tw2in_change = 0 ;
#endif

//----------------------------------------------------------------------------
// internal functions

void diostart(void) ;
void diostop(void) ;
unsigned long dio_read(int port) ;
void dio_write(int port, int pmask, int pval) ;
void dio_mask(int port, int pmask) ;

//----------------------------------------------------------------------------
// DIO initializer

void diostart(void)
{
    // ------------------------------------------------------------
    // Enable clock for I/O

    // Peripheral Clock Enable Register
    PMC->PMC_PCER0 = (1 << ID_PIOA) ;

    // ------------------------------------------------------------
    // PORT A

    PIOA->PIO_SODR = PIOA_MASK ;     // set at 1
    PIOA->PIO_PER = PIOA_MASK ;      // Set in PIO mode
    PIOA->PIO_OER = PIOA_MASK ;      // Configure in Output
    // disable pull-up for all outputs (no open drain)
    PIOA->PIO_PUDR = PIOA_PMASK ;
    // disable pull-up for these inputs
    PIOA->PIO_PUDR = PIOA_INPMASK ;
}

//----------------------------------------------------------------------------
// DIO terminator

void diostop(void)
{
}

//----------------------------------------------------------------------------
// Read digital input port
// port values:
//      0       internal PORT A, 32 bit
//      1       internal PORT B, 32 bit
//      2       external optional MAX7324 TW1, 8 input data bit + 8 input flag bit + 8 output data bit
//      3       external optional MAX7324 TW2, 8 input data bit + 8 input flag bit + 8 output data bit

unsigned long dio_read(int port)
{
    unsigned long retval = 0 ;

    switch(port) {
    case 0:     // PORT A
        retval = PIOA->PIO_PDSR ;
        break ;
    }
    return(retval) ;
}

//----------------------------------------------------------------------------
// Write digital input port
// port values:
//      0       internal PORT A, 32 bit
//      1       internal PORT B, 32 bit
//      2       external optional MAX7324 TW1, 8 output data bit
//      3       external optional MAX7324 TW2, 8 output data bit

void dio_write(int port, int pmask, int pval)
{
    switch(port) {
    case PORT_PIOA:             // PORT A
        PIOA->PIO_SODR = pmask & pval  ;        // set
        PIOA->PIO_CODR = pmask & (~pval) ;      // clear
        break ;
    }
}

//----------------------------------------------------------------------------
// Write digital input port mask
// port values:
//      0       internal PORT A, 32 bit NOT USED
//      1       internal PORT B, 32 bit NOT USED
//      2       external optional MAX7324 TW1, 8 output data bit
//      3       external optional MAX7324 TW2, 8 output data bit

void dio_mask(int port, int pmask)
{
    switch(port) {
    case 0:             // PORT A
        break ;
    }
}

//----------------------------------------------------------------------------
// Read counter value
// port values:
//      0       connected to GPS 1 PPS
//      1       connected to odometer

unsigned short dio_counter(int port)
{
    return(0) ;
}

// end of file - drv_dio.c

