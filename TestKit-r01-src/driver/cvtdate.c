// cvtdate.c - date/time functions

//
//   Copyright (c) 1997-2004.
//   T.E.S.T. srl
//

#include <stdio_console.h>
#include <stdlib.h>	// "C" standard include files
#include "assign.h"

#include "typedef.h"
#include "rtxstruc.h"
#include "extapi.h"

#include "rtxcapi.h"
#include "enable.h"
#include "cres.h"

#include "cvtdate.h"

#define SECS_PER_MIN  (60L)
#define MINS_PER_HOUR (60L)
#define SECS_PER_HOUR (MINS_PER_HOUR * SECS_PER_MIN)
#define HOURS_PER_DAY (24L)
#define SECS_PER_DAY  (HOURS_PER_DAY * SECS_PER_HOUR)
#define DAYS_PER_YEAR (365L)

#define NIB2BIN(A)	((((A)>>4)*10)+((A)&0x0f))
#define BIN2NIB(A)	((((A)/10)<<4)+((A)%10))

// -----------------------------------------------------------------------------
//
//
//   Function to compute a time_t value when given the calendar time in
//   a	time structure.  The pointer to the time structure is passed as
//   the argument to the function.
//
//

time_t date2systime(struct time_tm *p)
{
	long j;

	j = (long)(p->tm_yr - 1901) * DAYS_PER_YEAR + (p->tm_yr - 1901) / 4 +
		((p->tm_mon * 30L) + (p->tm_mon * 145L) / 256) + p->tm_day - 30;

	if (p->tm_mon >= 3) /* february */
	{
		j--;
		if (p->tm_yr & 0x0003)	j--;
	}
	j -= 25203L; /* adjust for base date of 1-Jan-1970 */

// if (p->tm_isdst)
//    p->tm_hr--;

	j = ((((long)j * HOURS_PER_DAY) + (long)p->tm_hr) * 60L +
		(long)p->tm_min) * 60L + (long)p->tm_sec;

	return(j);
}

// -----------------------------------------------------------------------------
//
//
//   Function  to compute  the members	of the	time structure	given a
//   time_t.  The function fills in the members of the structure except
//   for  the isdst  flag indicating  if the  current time  is Daylight
//   Savings Time  (DST) which	must be  supplied by  the caller.   The
//   calendar is accurate until Monday, 18-Jan-2038 at 3:14:07.
//
//

void systime2date(time_t n, struct time_tm *p)
{
	time_t systime;

// if (p->tm_isdst)
//    n += SECS_PER_HOUR;

	systime = n / SECS_PER_DAY;	/* divide time by 86400 to get day number */
	n -= systime * SECS_PER_DAY; /* leaves the time-of-day in number of seconds */

	p->tm_hr = (int)(n / SECS_PER_HOUR);

	n -= (long)p->tm_hr * SECS_PER_HOUR;
	p->tm_min = (int)(n / MINS_PER_HOUR);

	p->tm_sec = (int)(n - (long)p->tm_min * MINS_PER_HOUR);

	systime += 25203L; /* adjust for base date of 1-Jan-1970 */
	n = (systime * 1000L) / 365251L; /* 365.251 days/year - the hard way */

	p->tm_yr = 1901 + (int)n;
	if ((n = systime - n * 36525L / 100L + 30) >= 90)
	{
		if ((p->tm_yr & 0x0003) != 0)
			n = n + 2;
		else if (n != 90)
			n++;
	}

	p->tm_mon = (int)(n * 100L / 3057L); /* 30.57 days/mo - the hard way */

	p->tm_day = (int)(n - (p->tm_mon * 3057L) / 100L);

	if ((p->tm_wday = (int)(systime - ((systime / 7) * 7))) == 0)
		p->tm_wday = 7;

	if ((p->tm_wday = p->tm_wday + 2) > 7)
		p->tm_wday -= 7;
}

// *****************************************************************************
// RTC chip used

#ifdef USE_RTC_TWI_DS1337

// #define BCD2BIN(A) ( (((A)&0x70)>>4)*10 + ((A)&0x0f) )
static int BCD2BIN(unsigned char a)
{
    return( ((a & 0x70)>>4)*10 + (a & 0x0f) ) ;
}

// #define BIN2BCD(A) ( (((A)/10)<<4) + ((A)%10) )
static unsigned char BIN2BCD(int a)
{
    return( ((a / 10)<<4) + (a % 10) ) ;
}

// -----------------------------------------------------------------------------
// RTC_WriteTime_t (USE_RTC_TWI_DS1337)

void RTC_WriteTime_t(time_t t)
{
    unsigned char buf[8] ;
    struct time_tm p ;

    systime2date(t, &p) ;

    buf[0] = 0 ;        // initial write pointer
    buf[1] = BIN2BCD(p.tm_sec) ;
    buf[2] = BIN2BCD(p.tm_min) ;
    buf[3] = BIN2BCD(p.tm_hr) ;
    buf[4] = BIN2BCD(p.tm_wday) ;
    buf[5] = BIN2BCD(p.tm_day) ;
    buf[6] = BIN2BCD(p.tm_mon) ;
    buf[7] = BIN2BCD((p.tm_yr) - 2000) ;

    KS_lockw(TWIPORT) ;         // we trust with

    TWI_send(DS1337_ADDR, buf, 8) ;

    KS_unlock(TWIPORT) ;        // free resource

}

// -----------------------------------------------------------------------------
// RTC_ReadTime_t (USE_RTC_TWI_DS1337)

time_t RTC_ReadTime_t(void)
{
    unsigned char buf[16] ;
    struct time_tm p ;

    KS_lockw(TWIPORT) ;         // we trust with

    // set pointer
    buf[0] = 0 ;
    TWI_send(DS1337_ADDR, buf, 1) ;

    // read data
    TWI_receive(DS1337_ADDR, buf, 16) ;

    KS_unlock(TWIPORT) ;        // free resource

    // decode time
    p.tm_yr = BCD2BIN(buf[6]) + 2000 ;
    p.tm_mon = BCD2BIN(buf[5]) ;
    p.tm_day = BCD2BIN(buf[4]) ;
    p.tm_wday = BCD2BIN(buf[3]) ;
    p.tm_sec = BCD2BIN(buf[0]) ;
    p.tm_min = BCD2BIN(buf[1]) ;
    p.tm_hr = BCD2BIN(buf[2]) ;

    return(date2systime(&p)) ;
}

// -----------------------------------------------------------------------------
// RTC_WriteAlarm_t (USE_RTC_TWI_DS1337)

void RTC_WriteAlarm_t(time_t t)
{
    unsigned char buf[5] ;
    struct time_tm p ;

    systime2date(t, &p) ;

    // Set A1Mx = 0 in order to Alarm when:
    //          day, hours, minutes, and seconds match

    KS_lockw(TWIPORT) ;         // we trust with

    buf[0] = 7 ;        // initial write pointer
    buf[1] = BIN2BCD(p.tm_sec) ;       // A1M1 = 0
    buf[2] = BIN2BCD(p.tm_min) ;       // A1M2 = 0
    buf[3] = BIN2BCD(p.tm_hr) ;        // A1M3 = 0
    buf[4] = BIN2BCD(p.tm_day) ;       // A1M4 = 0

    TWI_send(DS1337_ADDR, buf, 5) ;

    buf[0] = 0x0e ;     // initial write pointer
    buf[1] = 5 ;        // Enable interrupt 1

    TWI_send(DS1337_ADDR, buf, 2) ;

    KS_unlock(TWIPORT) ;        // free resource
}
// -----------------------------------------------------------------------------
// RTC_ReadStatus (USE_RTC_TWI_DS1337)
//

int RTC_ReadStatus(void)
{
    unsigned char buf[3] ;
    int retval ;

    KS_lockw(TWIPORT) ;         // we trust with

    // set pointer
    buf[0] = 0xe ;
    TWI_send(DS1337_ADDR, buf, 1) ;

    // read data
    TWI_receive(DS1337_ADDR, buf, 2) ;

    // prepare result
    retval = (buf[0]) |         // Control Register (0xe)
             (buf[1] << 8) ;    // Status register (oxf)

    if (1) {                    // time to clear ?
        // set pointer
        buf[0] = 0xe ;
        buf[1] = 0 ;            // Clear Control Register (0xe)
        buf[2] = 0 ;            // Clear Status Register (0xf)

        TWI_send(DS1337_ADDR, buf, 3) ;
    }

    KS_unlock(TWIPORT) ;        // free resource

    return(retval) ;
}
#endif // USE_RTC_TWI_DS1337

// *****************************************************************************
// RTC chip used

#ifdef USE_RTC_AT91SAM7

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
unsigned long noinit_GPBR0 NOINIT_ATTRIBUTE ;
#define BASETIMER_STATIC_UINT32         (noinit_GPBR0)
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_AT91SAM7A3)
#define BASETIMER_STATIC_UINT32         (AT91C_BASE_SYS->SYS_GPBR0)
#endif // defined(USE_AT91SAM7A3)

// -----------------------------------------------------------------------------
// RTC_WriteTime_t (USE_RTC_AT91SAM7)

void RTC_WriteTime_t(time_t t)
{
    unsigned long kk ;

// simple
//    BASETIMER_STATIC_UINT32 = t - AT91C_BASE_RTTC->RTTC_RTVR ;

    // get old coefficient
    kk = (AT91C_BASE_RTTC->RTTC_RTMR & 0xffff) ;

    // check if this is the first calibration after battery power up
    if ((kk == 0x8000) || (BASETIMER_STATIC_UINT32 == 0)) {
        kk = ( (EXTERNAL_CLOCK * 16) / (AT91C_BASE_CKGR->CKGR_MCFR & 0xffff) ) ;

    // check how old last calibration was
    } else if ( (t - BASETIMER_STATIC_UINT32) > 3000 ) {

        // new calibration
        kk = ((AT91C_BASE_RTTC->RTTC_RTVR) * kk) /
             (t - BASETIMER_STATIC_UINT32) ;

        // sanity check
        if ((kk < 20000) || (kk > 44000)) {
            kk = ( (EXTERNAL_CLOCK * 16) / (AT91C_BASE_CKGR->CKGR_MCFR & 0xffff) ) ;
        }
    // no calibration change
    } else {

    }

    // sanity check
    if ((kk < 20000) || (kk > 44000)) kk = 32768 ;

    DISABLE ;
    AT91C_BASE_RTTC->RTTC_RTMR = AT91C_RTTC_RTTRST | ( kk & 0xffff ) ;
    BASETIMER_STATIC_UINT32 = t ;
    ENABLE ;
}

// -----------------------------------------------------------------------------
// RTC_ReadTime_t (USE_RTC_AT91SAM7)

time_t RTC_ReadTime_t(void)
{
    time_t t ;

    DISABLE ;   // critical region
    t = BASETIMER_STATIC_UINT32 + AT91C_BASE_RTTC->RTTC_RTVR ;
    ENABLE ;    // end of critical region

    return(t) ;
}

// -----------------------------------------------------------------------------
// RTC_WriteAlarm_t (USE_RTC_AT91SAM7)

void RTC_WriteAlarm_t(time_t t)
{
    volatile int dummy ;

    AT91C_BASE_RTTC->RTTC_RTAR = t - BASETIMER_STATIC_UINT32 ;

    dummy = AT91C_BASE_RTTC->RTTC_RTSR ;
#if defined(USE_AT91SAM7A3)
    AT91C_BASE_SHDWC->SHDWC_SHMR |= AT91C_SHDWC_RTTWKEN ;       // enable
#endif // defined(USE_AT91SAM7A3)
}

// -----------------------------------------------------------------------------
// RTC_ReadStatus (USE_RTC_AT91SAM7)
//

int RTC_ReadStatus(void)
{
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    return(AT91C_BASE_RTTC->RTTC_RTSR) ;
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_AT91SAM7A3)
    return(AT91C_BASE_SHDWC->SHDWC_SHSR) ;
#endif // defined(USE_AT91SAM7A3)
}
#endif // USE_RTC_AT91SAM7

// *****************************************************************************
// RTC chip used

#ifdef USE_RTC_LPC17XX
#ifndef RTC // useful for LPC1788
#define RTC LPC_RTC
#endif

// -----------------------------------------------------------------------------
// RTC_WriteTime_t (USE_RTC_LPC17XX)

void RTC_WriteTime_t(time_t t)
{
    struct time_tm tm ;

    systime2date(t, &tm) ;

    RTC->CCR &= (~1) ;          // stop
    RTC->SEC = tm.tm_sec ;
    RTC->MIN = tm.tm_min ;
    RTC->HOUR = tm.tm_hr ;
    RTC->DOW = tm.tm_wday ;
    RTC->DOM = tm.tm_day ;
    RTC->MONTH = tm.tm_mon ;
    RTC->YEAR = tm.tm_yr ;
    RTC->CCR |= 1 ;             // start
}

// -----------------------------------------------------------------------------
// RTC_ReadTime_t (USE_RTC_LPC17XX)

time_t RTC_ReadTime_t(void)
{
    struct time_tm tm ;

    RTC->CCR &= (~1) ;          // stop
    tm.tm_sec = RTC->SEC ;
    tm.tm_min = RTC->MIN ;
    tm.tm_hr = RTC->HOUR ;
    tm.tm_wday = RTC->DOW ;
    tm.tm_day = RTC->DOM ;
    tm.tm_mon = RTC->MONTH ;
    tm.tm_yr = RTC->YEAR ;
    RTC->CCR |= 1 ;             // start

#ifdef NODEF
    printf("%02d/%02d/%04d ",
                (unsigned int)(tm.tm_day),
                (unsigned int)(tm.tm_mon),
                (unsigned int)(tm.tm_yr)) ;

    printf("%02d:%02d:%02d\n",
                (unsigned int)(tm.tm_hr),
                (unsigned int)(tm.tm_min),
                (unsigned int)(tm.tm_sec)) ;
#endif // NODEF
//    tm.tm_yr = (ctime1 >> 16) & 0xfff ;
//    tm.tm_mon = (ctime1 >> 8) & 0xf ;
//    tm.tm_day = (ctime1) & 0x1f ;
//    tm.tm_wday = 0 ;
//    tm.tm_sec = (ctime0) & 0x3f ;
//    tm.tm_min = (ctime0 >> 8) & 0x3f ;
//    tm.tm_hr = (ctime0 >> 16) & 0x1f ;

    return(date2systime(&tm)) ;
}

// -----------------------------------------------------------------------------
// RTC_WriteAlarm_t (USE_RTC_LPC17XX)

void RTC_WriteAlarm_t(time_t t)
{
    struct time_tm tm ;

    systime2date(t, &tm) ;
    RTC->ALSEC = tm.tm_sec ;
    RTC->ALMIN = tm.tm_min ;
    RTC->ALHOUR = tm.tm_hr ;
    RTC->ALDOW = tm.tm_wday ;
    RTC->ALDOM = tm.tm_day ;
    RTC->ALMON = tm.tm_mon ;
    RTC->ALYEAR = tm.tm_yr ;

    RTC->ILR = 3 ;
    RTC->AMR = 0x30 ;   // don't check DOY DOW
}

// -----------------------------------------------------------------------------
// RTC_ReadStatus (USE_RTC_LPC17XX)
//

int RTC_ReadStatus(void)
{
    return(0) ;
}
#endif // USE_RTC_LPC17XX

// *****************************************************************************
// RTC chip used

#ifdef USE_RTC_AT91SAM3

#define BASETIMER_STATIC_UINT32         (GPBR->SYS_GPBR0)

// -----------------------------------------------------------------------------
// RTC_WriteTime_t (USE_RTC_AT91SAM3)

void RTC_WriteTime_t(time_t t)
{
    unsigned long kk ;

// simple
//    BASETIMER_STATIC_UINT32 = t - AT91C_BASE_RTTC->RTTC_RTVR ;

    // get old coefficient
    kk = (RTT->RTT_MR & 0xffff) ;

    // check if this is the first calibration after battery power up
    if ((kk == 0x8000) || (BASETIMER_STATIC_UINT32 == 0)) {
        kk = ( (EXTERNAL_CLOCK * 16) / (PMC->CKGR_MCFR & 0xffff) ) ;

    // check how old last calibration was
    } else if ( (t - BASETIMER_STATIC_UINT32) > 3000 ) {

        // new calibration
        kk = ((RTT->RTT_VR) * kk) /
             (t - BASETIMER_STATIC_UINT32) ;

        // sanity check
        if ((kk < 20000) || (kk > 44000)) {
            kk = ( (EXTERNAL_CLOCK * 16) / (PMC->CKGR_MCFR & 0xffff) ) ;
        }
    // no calibration change
    } else {

    }

    // sanity check
    if ((kk < 20000) || (kk > 44000)) kk = 32768 ;

    DISABLE ;
    RTT->RTT_MR = RTT_MR_RTTRST | ( kk & 0xffff ) ;
    BASETIMER_STATIC_UINT32 = t ;
    ENABLE ;
}

// -----------------------------------------------------------------------------
// RTC_ReadTime_t (USE_RTC_AT91SAM3)

time_t RTC_ReadTime_t(void)
{
    time_t t ;

    DISABLE ;   // critical region
    t = BASETIMER_STATIC_UINT32 + RTT->RTT_VR ;
    ENABLE ;    // end of critical region

    return(t) ;
}

// -----------------------------------------------------------------------------
// RTC_WriteAlarm_t (USE_RTC_AT91SAM3)

void RTC_WriteAlarm_t(time_t t)
{
    volatile int dummy ;

    RTT->RTT_AR = t - BASETIMER_STATIC_UINT32 ;

    dummy = RTT->RTT_SR ;
    RTT->RTT_MR |= RTT_MR_ALMIEN ;  // enable interrupt to wake up
}

// -----------------------------------------------------------------------------
// RTC_ReadStatus (USE_RTC_AT91SAM3)
//

int RTC_ReadStatus(void)
{
    return(RTT->RTT_SR) ;
}
#endif // USE_RTC_AT91SAM3
// end of file - cvtdate.c

