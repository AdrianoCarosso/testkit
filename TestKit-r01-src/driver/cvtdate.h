// cvtdate.h

//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

#include "typedef.h"

struct time_tm
{
   int tm_yr ;
   int tm_mon ;
   int tm_day ;
   int tm_wday ;
   int tm_sec ;
   int tm_min ;
   int tm_hr ;
// int tm_isdst ;
} ;

time_t date2systime(struct time_tm *) ;
void   systime2date(time_t, struct time_tm *) ;

#if defined(USE_RTC_TWI_DS1337) || defined(USE_RTC_AT91SAM7) || defined(USE_RTC_LPC17XX) || defined(USE_RTC_AT91SAM3) 
extern void RTC_WriteTime_t(time_t t) ;
extern time_t RTC_ReadTime_t(void) ;
extern void RTC_WriteAlarm_t(time_t t) ;
extern int  RTC_ReadStatus(void) ;
#endif // defined(USE_RTC_TWI_DS1337) || defined(USE_RTC_AT91SAM7) || defined(USE_RTC_LPC17XX) || defined(USE_RTC_AT91SAM3) 

// end of file - cvtdate.h

