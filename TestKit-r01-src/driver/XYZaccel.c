//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//
// This example module calculates the average of X, Y and Z

// used only for debug printf
#include <stdio_console.h>

#ifdef CBUG
#undef USE_MONITORING  // just for debug
#endif // CBUG

//----------------------------------------------------------------------------
// local storage

unsigned long avgX, avgY, avgZ ;

//----------------------------------------------------------------------------
// local function declaration

void XYZ_accel_init(void) ;
void XYZ_accel_end(void) ;
void XYZ_accel_step(unsigned short *ptr, int len) ;

//----------------------------------------------------------------------------
// XYZ_accel_init()
// called only once at boot time, before any XYZ_accel_step()

void XYZ_accel_init(void)
{
#ifdef USE_MONITORING  // just for debug
    printf("XYZ_accel_init()\n") ;
#endif // USE_MONITORING  -- just for debug

    // prepare variables for average calculation
    // 512 is the mid range value
    avgX = avgY = avgZ = 512 ;
}

//----------------------------------------------------------------------------
// XYZ_accel_end()
// called only once at shutdown time, no more XYZ_accel_step() after.

void XYZ_accel_end(void)
{
#ifdef USE_MONITORING  // just for debug
    printf("XYZ_accel_end()\n") ;
#endif // USE_MONITORING  -- just for debug
}

//----------------------------------------------------------------------------
// XYZ_accel_step()
// called every time an ADC sampled data chunk has been acquired
// parameters:
//      ptr     ADC buffer pointer to first useful X
//      len     how many samples (i.e. 3 means XYZ*3 --> 18 bytes)
// ADC buffer is organized as a sequence of X, Y, Z values of 10 bit stored in
// a short (double byte). We have 6 bytes every XYZ sample.

void XYZ_accel_step(unsigned short *ptr, int len)
{
    register int i ;
    
#ifdef USE_MONITORING  // just for debug
    printf("XYZ_accel_step(0x%x, %d)\n", (int)(ptr), len) ;
#endif // USE_MONITORING  -- just for debug

    // scan all new values
    for(i=0 ; i<len ; i++) {
        avgX += ptr[(i*3) + 0] ;        // X axis
        avgY += ptr[(i*3) + 1] ;        // Y axis
        avgZ += ptr[(i*3) + 2] ;        // Z axis
    }

    // evaluate average
    // keep in mind last value of avgXYZ, use len+1
    avgX /= (len+1) ;                   // X axis
    avgY /= (len+1) ;                   // Y axis
    avgZ /= (len+1) ;                   // Z axis
}

