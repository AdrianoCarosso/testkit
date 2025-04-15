/* cres.c - RTXC Resource definitions - ARM */

#include "typedef.h"
#include "rtxstruc.h"
#include "extapi.h"
#include "cres.h"

const RESOURCE nres = NRES;

RHEADER rheader[1+NRES];

#ifdef CBUG
const char reskname[1+NRES][NAMMAX+1] =
{
   " ",
   "EXTAPI",
   "SPIPORT",
   "TWIPORT",
   "ONLYONE",
   "GPSONE",
#if defined(USE_NANDFLASH_ON_ARM)
   "NANDPORT",
#endif // defined(USE_NANDFLASH_ON_ARM)
#if defined(USE_EEPROM_ON_ARM)
   "EEPRPORT",
#endif // defined(USE_EEPROM_ON_ARM)
   "PCONE"
} ;
#endif // CBUG

