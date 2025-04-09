/* cres.h - RTXC Resource include file */

#define NAMMAX 8

enum RESOURCELIST {
FIRSTRES = 0,   // dummy, never used

EXTAPI,         // Mutual exclusion
SPIPORT,        // Mutual exclusion
TWIPORT,        // Mutual exclusion
ONLYONE,        // Generic MutExc
GPSONE,         // GPS Fix MutExc
#if defined(USE_NANDFLASH_ON_ARM)
NANDPORT,       // NAND MutExc
#endif // defined(USE_NANDFLASH_ON_ARM)
#if defined(USE_EEPROM_ON_ARM)
EEPROMPORT,
#endif // defined(USE_EEPROM_ON_ARM)
PCONE,          // PC output mutual exc

MAXRESOURCE     // evaluate total number
} ;

#define NRES (MAXRESOURCE-1)

extern const RESOURCE nres ;

extern RHEADER rheader[1+NRES] ;

#ifdef CBUG
extern const char reskname[1+NRES][NAMMAX+1] ;
#endif // CBUG

