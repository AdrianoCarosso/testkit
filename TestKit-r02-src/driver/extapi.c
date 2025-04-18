//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//
//      History:
//      October 1997    Purchase
//       10/Jul/2006    Reworked for ARM

// ******************************************************** Related definitions

#include <stdio_console.h>

#include <stdlib.h>     // "C" standard include files
#include <string.h>

#include <rtxcapi.h>    // RTXC Application Program Interface
#include <enable.h>

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"
#include "cpart.h"
#include "cres.h"

#include "cvtdate.h"
#include "extapi.h"

#define  USE_FLASHMONITORING     // debug only
//#undef  USE_FLASHMONITORING     // debug only
#undef  USE_TWIMONITORING       // debug only

#ifdef USE_TRANSACTIONS_ON_ARM
#define USECIRCULARBUFFER
unsigned long trans_addr = 0L ;
#endif // USE_TRANSACTIONS_ON_ARM

#ifdef CBUG /* { */
extern char cbugflag ;
extern unsigned short par71 ;
#endif /* } */

// *********************************************************************
// Sanity check

#if (!defined(USE_TWI_ON_ARM)) && defined(USE_PARAMETERS_ON_TWI)
#error "Bad definition on TWI"
#endif

#if (!defined(USE_SPI_ON_ARM)) && defined(USE_PARAMETERS_ON_FLASH)
#error "Bad definition on FLASH"
#endif

#if (!defined(USE_EEPROM_ON_ARM)) && defined(USE_PARAMETERS_ON_EEPROM)
#error "Bad definition on EEPROM"
#endif

#ifdef USE_VIRTUALRAM_ON_ARM
#if (!defined(USE_SPI_ON_ARM))  && defined(USE_VIRTUALRAM_ON_FLASH)
#error "Bad definition on VRAM"
#endif

#if (!defined(USE_EEPROM_ON_ARM)) && defined(USE_VIRTUALRAM_ON_EEPROM)
#error "Bad definition on VRAM"
#endif
#endif // USE_VIRTUALRAM_ON_ARM

// *********************************************************************
// from RTXCstartup.S

extern void CodeUpgrade(void) ;

// *********************************************************************
// local buffer
#if defined(CBUG) || defined(USE_TWI_ON_ARM) || defined(USE_SPI_ON_ARM)
// mutex guarantee by EXTAPI resource
#ifdef GEMHH
unsigned char extapibuf[512] __attribute__ ((aligned (2))) ;    // used also as word
#else
unsigned char extapibuf[256] __attribute__ ((aligned (2))) ;    // used also as word
#endif
// EXTAPI resource locks TWI and PDEBUGactivities
#endif // defined(CBUG) || defined(USE_TWI_ON_ARM) || defined(USE_SPI_ON_ARM)

// *********************************************************************
// external functions

// global allocation space
static size_t msize ;   // size of memory block of PCKBUF

static unsigned char NumOfTasks ;       // num of tasks involved in shutdown
static unsigned char sd_mode ;          // shutdown mode

#if defined(USE_PARAMETERS_ON_TWI) || defined(USE_PARAMETERS_ON_FLASH) || defined(USE_PARAMETERS_ON_EEPROM)
static struct MYSETUP setup ;
#endif // defined(USE_PARAMETERS_ON_TWI) || defined(USE_PARAMETERS_ON_FLASH)

#if defined(USE_TWI_ON_ARM) && defined(USE_ATMEL_TWI)
struct _IOSTATUS iostatus ;     // common I/O proc status
#endif // defined(USE_TWI_ON_ARM) && defined(USE_ATMEL_TWI)

#if defined(USE_PKTMEMBUF) || defined(CBUG)
static struct time_tm tm ;      // real time operations
#endif // defined(USE_PKTMEMBUF) || defined(CBUG)

// *********************************************************************
#if ( defined(USE_SPI_ON_ARM) && defined(USE_SERIALFLASH_ON_ARM) )
void SPI_FlashErase(unsigned long bbegin, unsigned long bend) ;
void SPI_FlashRead(unsigned char *dst, unsigned long bbegin, int flen) ;
void SPI_FlashWrite(unsigned long bbegin, unsigned char *src, unsigned char *dst, int flen) ;
#endif
// *********************************************************************

#if defined(USE_LOW_POWER) && defined (USE_JUST_FOR_DEBUG)

#define USE_FLASHCODE
#undef USE_RAMCODE

void GoStandBy(void)
{
#ifdef USE_FLASHCODE
    AT91C_BASE_PMC->PMC_SCDR = AT91C_PMC_PCK ;      // idle mode
#endif // USE_FLASHCODE

#ifdef USE_RAMCODE
    static unsigned short localcode[6] = {
        0x4b01, 0x2201, 0x605a, 0x4770, 0xfc00, 0xffff
//        0x2201, 0x4b01, 0x4770, 0x605a, 0xffff, 0xfc00
    } ;
/*
<GoStandBy>:
        4b01    ldr	r3, [pc, #4]	(Konst)
        2201   	mov	r2, #1
        605a    str	r2, [r3, #4]
        4770    bx	lr
Konst:
        fc00    second half of BL instruction 0xfc00
        ffff    second half of BL instruction 0xffff
*/
    ((void (*)(void))(localcode))() ;
#endif // USE_RAMCODE
}
#endif // USE_LOW_POWER && USE_JUST_FOR_DEBUG

#if defined(USE_TWI_ON_ARM) && defined(USE_ATMEL_TWI)
// *********************************************************************
// Input from subprocessor

unsigned long EKS_ReadInput(void)
{
    int i ;
    unsigned char locbuf[1] ;
#if defined(CBUG) && defined(USE_TWIMONITORING)
    unsigned long long lasttick ;
#endif // CBUG USE_TWIMONITORING

#if defined(CBUG) && defined(USE_TWIMONITORING)
    lasttick = tickmeasure(0LL) ;
#endif // CBUG USE_TWIMONITORING

    KS_lockw(TWIPORT) ;         // we trust with

    iostatus.cmd = 0 ;          // default no data

    for(i=0 ; i<3 ; i++) {      // max retry number
        // default for I/O sub-processor is 'TWI_PROTOCOL_CMD_STATUS'
        if (TWI_receive(TWI_DEVICE_IOPROC, (unsigned char *)(&iostatus), sizeof(iostatus)) != OK)
            break ;     // break if error

        if (iostatus.cmd == TWI_PROTOCOL_CMD_STATUS)
            break ;     // break if done

        // try to change state
        locbuf[0] = TWI_PROTOCOL_CMD_STATUS ;
        if (TWI_send(TWI_DEVICE_IOPROC, locbuf, 1) != OK)
            break ;     // break if error
    }

    KS_unlock(TWIPORT) ;        // free resource

#if defined(CBUG) && defined(USE_TWIMONITORING)
    lasttick = tickmeasure(lasttick) ;
#endif // CBUG USE_TWIMONITORING

#if defined(CBUG) && defined(USE_TWIMONITORING)
{ int i ; unsigned char *p = (unsigned char *)(&iostatus) ;
  pdebugt(1,"TWI time: %llu", lasttick) ;
  if (iostatus.cmd == TWI_PROTOCOL_CMD_STATUS) {
    for(i=0 ; i<sizeof(iostatus) ; i+=8) {
      pdebugt(1,"%02x %02x %02x %02x %02x %02x %02x %02x",
                      p[i+0], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5], p[i+6], p[i+7]) ;
    }
  } else {
    pdebugt(1,"TWI error") ;
  }
}
#endif // CBUG USE_TWIMONITORING

    return((iostatus.cmd == TWI_PROTOCOL_CMD_STATUS) ? iostatus.pd_value : 0) ;
}
#endif // defined(USE_TWI_ON_ARM) && defined(USE_ATMEL_TWI)

// *********************************************************************
// Parameter section - COMMON

#if defined(USE_PARAMETERS_ON_TWI) || defined(USE_PARAMETERS_ON_FLASH) || defined(USE_PARAMETERS_ON_EEPROM)
int EKS_ParReadShort(unsigned char num, unsigned short * val)
{
        struct PKTMEMBUF *ph ;      // data chain

        if ((ph = EKS_ParRead(num))) {        // does it exist ?
                *val = *((short *)(((unsigned char *)(ph))+(ph->offs))) ;
                EKS_PktBufRelease(ph) ;         // return memory
                return(OK) ;
        } else {
                *val = 0 ;              // default value
                return(ERROR) ;
        }
}

int EKS_ParReadLong(unsigned char num, long *val)
{
        struct PKTMEMBUF *ph ;      // data chain

        if ((ph = EKS_ParRead(num))) {        // does it exist ?
                memcpy(val, (((unsigned char *)(ph)) + (ph->offs)), sizeof(*val)) ;
                EKS_PktBufRelease(ph) ;         // return memory
                return(OK) ;
        } else {
                *val = 0 ;              // default value
                return(ERROR) ;
        }
}

int EKS_ParWriteShort(unsigned char num, short val)
{
        struct PKTMEMBUF *ph ;      // data chain

        if ((ph = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL)
                return(ERROR) ;         // return with error
        ph->next = MNULL ;          // no others
        ph->offs = sizeof(struct PKTMEMBUF) ;       // data start
        ph->mtot = sizeof(short) ;  // data len

        *((short *)(((unsigned char *)(ph))+(ph->offs))) = val ;
        return(EKS_ParWrite(num, &ph)) ;
}

int EKS_ParWriteLong(unsigned char num, long val)
{
        struct PKTMEMBUF *ph ;      // data chain

        if ((ph = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL)
                return(ERROR) ;         // return with error
        ph->next = MNULL ;          // no others
        ph->offs = sizeof(struct PKTMEMBUF) ;       // data start
        ph->mtot = sizeof(long) ;   // data len

        memcpy((((unsigned char *)(ph)) + (sizeof(struct PKTMEMBUF))), &val, sizeof(val)) ;
        return(EKS_ParWrite(num, &ph)) ;
}
#endif // defined(USE_PARAMETERS_ON_FLASH) || defined(USE_PARAMETERS_ON_TWI)

#ifdef USE_PARAMETERS_ON_TWI
// *********************************************************************
// Parameter section - USE_PARAMETERS_ON_TWI

struct MYSETUP * EKS_GetSetup(void)		// USE_PARAMETERS_ON_TWI
{
    unsigned char locbuf[2 + sizeof(setup.sernum)] ;

    KS_lockw(TWIPORT) ;         // we trust with

    locbuf[0] = TWI_PROTOCOL_CMD_EE_READ_PAR ;
    locbuf[1] = 255 ;           // it means sernum
    if (TWI_send(TWI_DEVICE_IOPROC, locbuf, 2) != OK) {
        setup.sernum = 0 ;
    } else {
        TWI_receive(TWI_DEVICE_IOPROC, locbuf, 1+sizeof(setup.sernum)) ;
        memcpy(&setup.sernum, &locbuf[1], sizeof(setup.sernum)) ;
    }

    KS_unlock(TWIPORT) ;        // free resource

    return(&setup) ;            // end of memory
}

void EKS_NewSetup(struct MYSETUP * ms)		// USE_PARAMETERS_ON_TWI
{
    unsigned char locbuf[3 + sizeof(ms->sernum)] ;

    KS_lockw(TWIPORT) ;         // we trust with

    locbuf[0] = TWI_PROTOCOL_CMD_EE_WRITE ;
    locbuf[1] = 255 ;          // it means sernum
    locbuf[2] = sizeof(ms->sernum) ;
    memcpy(&locbuf[3], &(ms->sernum), sizeof(ms->sernum)) ;
    TWI_send(TWI_DEVICE_IOPROC, locbuf, 3+sizeof(ms->sernum)) ;

    KS_unlock(TWIPORT) ;        // free resource
}

int EKS_ParSize(void)		// USE_PARAMETERS_ON_TWI
{
    EKS_ReadInput() ;           // just to receive a fresh copy of iostatus
    return(iostatus.eepromused) ;
}

struct PKTMEMBUF * EKS_ParRead(unsigned char num)		// USE_PARAMETERS_ON_TWI
{
    int i, len ;
    struct PKTMEMBUF *pmfirst, *pmactual ;      // data chain
    unsigned char locbuf[4] ;

    pmfirst=pmactual=MNULL ;    // initialize pointers for data chain

    KS_lockw(TWIPORT) ;         // we trust with

    locbuf[0] = TWI_PROTOCOL_CMD_EE_READ_PAR ;
    locbuf[1] = num ;

    if (TWI_send(TWI_DEVICE_IOPROC, locbuf, 2) != OK) {
        KS_unlock(TWIPORT) ;    // free resource
        return(MNULL) ;         // end of memory
    }

    for(i=3 ; i>=0 ; i--) {     // max retry
        // first read: get par len
        if (TWI_receive(TWI_DEVICE_IOPROC, locbuf, 3) != OK) {
            i = -3 ;            // error
            break ;
        }

        // check if found
        if (locbuf[0] == TWI_PROTOCOL_CMD_EE_READ_DATA)
            break ;
    }
    KS_unlock(TWIPORT) ;        // free resource

    // take parameter len
    len = locbuf[2] ;

    // sanity check
    if ((i < 0) || (len <= 0) || (len > 254)) {
        return(MNULL) ;         // end of memory
    }

    KS_lockw(EXTAPI) ;          // we trust with
    KS_lockw(TWIPORT) ;         // we trust with

    // next read, read whole par
    if (TWI_receive(TWI_DEVICE_IOPROC, extapibuf, len+3) != OK) {
        KS_unlock(TWIPORT) ;    // free resource
        KS_unlock(EXTAPI) ;     // free resource
        return(MNULL) ;         // end of memory
    }

    KS_unlock(TWIPORT) ;        // free resource

    for(i=0 ; i<len ; i++) {    // create a new chain
        // remember: if EKS_PktBufAdd has an error, it panics and never returns
        EKS_PktBufAdd(extapibuf[i+3], &pmfirst, &pmactual) ;
    }

    KS_unlock(EXTAPI) ; // free resource

    return(pmfirst) ;   // done
}

int EKS_ParWrite(unsigned char num, struct PKTMEMBUF ** ppst)		// USE_PARAMETERS_ON_TWI
{
    int i, len, reallen ;
    unsigned char locvar ;

    len = EKS_PktBufLength(*ppst) ;
    reallen = MIN(250, len) ;

    KS_lockw(EXTAPI) ;          // we trust with

    extapibuf[0] = TWI_PROTOCOL_CMD_EE_WRITE ;
    extapibuf[1] = num ;
    extapibuf[2] = reallen ;

    for(i=0 ; i<len ; i++) {    // get from buffer
        if (EKS_PktBufPullup(ppst, &locvar, 1) == 1) {
            if (i < reallen) {  // if inside bounds
                extapibuf[i+3] = locvar ;
            }
        }
    }

    KS_lockw(TWIPORT) ;         // we trust with

    i = TWI_send(TWI_DEVICE_IOPROC, extapibuf, 3+reallen) ;

    KS_unlock(TWIPORT) ;        // free resource

    KS_unlock(EXTAPI) ;         // free resource

    return(i) ;
}

void EKS_ParClear(void)		// USE_PARAMETERS_ON_TWI
{
    unsigned char locbuf[1] ;

    KS_lockw(TWIPORT) ;         // we trust with

    locbuf[0] = TWI_PROTOCOL_CMD_EE_ERASE ;
    TWI_send(TWI_DEVICE_IOPROC, locbuf, 1) ;

    KS_unlock(TWIPORT) ;        // free resource
}
#endif // USE_PARAMETERS_ON_TWI

#ifdef USE_PARAMETERS_ON_FLASH
// *********************************************************************
// Parameter section - USE_PARAMETERS_ON_FLASH

struct MYSETUP * EKS_GetSetup(void)		// USE_PARAMETERS_ON_FLASH
{
//     KS_lockw(SPIPORT) ;         // we trust with
//     SPI_FlashRead((unsigned char *)(&setup), PARAM_START + (255 * PARAM_SSIZE), sizeof(setup)) ;
//     KS_unlock(SPIPORT) ;        // free resource

    EKS_LCK_FlashRead( PARAM_START + (255 * PARAM_SSIZE), (unsigned char *)(&setup), sizeof(setup)) ;
    return(&setup) ;            // end of memory
}

void EKS_NewSetup(struct MYSETUP * ms)		// USE_PARAMETERS_ON_FLASH
{
    struct PKTMEMBUF *pmnew ;

    // alloc temporary
    if ((pmnew = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
        panic(0) ;
    }

    KS_lockw(SPIPORT) ;         // we trust with

    SPI_FlashErase(PARAM_START + (255 * PARAM_SSIZE), PARAM_START + (255 * PARAM_SSIZE) + sizeof(setup) - 1) ;

    SPI_FlashWrite(PARAM_START + (255 * PARAM_SSIZE), (unsigned char *)(ms), (unsigned char *)(pmnew), MIN(sizeof(setup), msize)) ;

    KS_unlock(SPIPORT) ;        // free resource

    // release temporary
    KS_free(PKTBUF, pmnew) ;
}

int EKS_ParSize(void)		// USE_PARAMETERS_ON_FLASH
{
    return(PARAM_STOP - PARAM_START + 1) ;
}

struct PKTMEMBUF * EKS_ParRead(unsigned char num)		// USE_PARAMETERS_ON_FLASH
{
    struct PKTMEMBUF *pmfirst, *pmactual, *pmnew ;      // data chain
    unsigned long faddr, fok ;
    unsigned char len, plen, lok ;

    pmfirst = pmactual = MNULL ;        // initialize pointers for data chain
    faddr = PARAM_START + (num * PARAM_SSIZE) ;
    fok = 0 ;
    lok = 0 ;
    
    KS_lockw(SPIPORT) ;                 // we trust with
    
    for( ; ; ) {                        // scan whole parameter segment
        SPI_FlashRead(&len, faddr, 1) ;
        if (len == 0xff) {
            if (fok != 0) {             // really existant parameter
                while(lok) {                // build buffer chain
                    plen = MIN(lok, msize - sizeof(struct PKTMEMBUF)) ;
                    if ((pmnew = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
                        panic(1) ;
                    }
                    pmnew->next = MNULL ;                   // no others by now
                    pmnew->offs = sizeof(struct PKTMEMBUF) ;// data start
                    pmnew->mtot = plen ;
                    SPI_FlashRead(((unsigned char *)(pmnew)) + sizeof(struct PKTMEMBUF), fok, plen) ;
                    if (pmfirst == MNULL) {
                        pmfirst = pmnew ;
                    }
                    if (pmactual) {
                        pmactual->next = pmnew ;
                    }
                    pmactual = pmnew ;

                    fok += plen ;
                    lok -= plen ;
                }
            }
            break ;             // stop in any case
        }
        
        fok = faddr + 1 ;       // best found
        lok = len ;
        faddr += (len + 1) ;    // try with next
    }
    
    KS_unlock(SPIPORT) ;        // free resource
    return(pmfirst) ;           // done
}

int EKS_ParWrite(unsigned char num, struct PKTMEMBUF ** ppst)		// USE_PARAMETERS_ON_FLASH
{
    struct PKTMEMBUF *pmf, *pmnew, *pm ;
    unsigned long faddr ;
    int reallen, i ;
    unsigned char len ;

    pmf = *ppst ;
    reallen = MIN(250, EKS_PktBufLength(pmf)) ;

    faddr = PARAM_START + (num * PARAM_SSIZE) ;

    KS_lockw(SPIPORT) ;                 // we trust with

    // search first free slot
    for( ; ; ) {                        // scan whole parameter segment
        SPI_FlashRead(&len, faddr, 1) ;
        if (len == 0xff) {
            break ;                     // first free
        } else {                        // try next
            faddr += (len + 1) ;
        }
    }
    
    // check if room enough
    if ((faddr + reallen + 2) >= (PARAM_START + ((num + 1) * PARAM_SSIZE))) {
        // erase parameter
        SPI_FlashErase(PARAM_START + (num * PARAM_SSIZE), PARAM_START + ((num + 1) * PARAM_SSIZE) - 1) ;
        faddr = PARAM_START + (num * PARAM_SSIZE) ;     // restart
    }

    // alloc temporary buffer
    if ((pmnew = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
        panic(3) ;
    }

    // write len
    len = reallen ;
    SPI_FlashWrite(faddr, &len, (unsigned char *)(pmnew), 1) ;
    faddr++ ;

    // write param
    while(reallen) {
        i = MIN(reallen, pmf->mtot) ;

        SPI_FlashWrite(faddr, ((unsigned char *)(pmf))+(pmf->offs), (unsigned char *)(pmnew), i) ;

        faddr += ((unsigned long)(i)) ;

        pmf->mtot -= i ;
        if (!(pmf->mtot)) {             // free this packet
            pm = pmf->next ;            // new chain entry point
            KS_free(PKTBUF, pmf) ;      // no longer used
            pmf = pm ;
        }

        reallen -= i ;
    }

    // release temporary
    KS_free(PKTBUF, pmnew) ;

    // in case something still present in buffer (it was > 250)
    EKS_PktBufRelease(pmf) ;

    // free resource
    KS_unlock(SPIPORT) ;
    return(len) ;
}

void EKS_ParClear(void)		// USE_PARAMETERS_ON_FLASH
{
    EKS_GetSetup() ;            // I know it uses "setup" static

    KS_lockw(SPIPORT) ;         // we trust with
    SPI_FlashErase(PARAM_START, PARAM_START + (255 * PARAM_SSIZE) - 1) ;
    KS_unlock(SPIPORT) ;        // free resource
    
    EKS_NewSetup(&setup) ;      // restore
}
#endif // USE_PARAMETERS_ON_FLASH


#ifdef USE_PARAMETERS_ON_EEPROM
// *********************************************************************
// Parameter section - USE_PARAMETERS_ON_EEPROM

struct MYSETUP * EKS_GetSetup(void)		// USE_PARAMETERS_ON_EEPROM
{
    KS_lockw(EEPROMPORT) ;         // we trust with
    EEPROM_Read(EEPROM_PAGE_OFFSET(EE_SN_SETUP), (EE_SN_SETUP/EEPROM_PAGE_SIZE), (void*) (&setup), MODE_32_BIT, 1 ) ;
//    EEPROM_Read(16, 0, (void*) (&setup), MODE_32_BIT, 1 ) ;
    //SPI_FlashRead((unsigned char *)(&setup), (PARAM_START - sizeof(setup)), sizeof(setup)) ;
    KS_unlock(EEPROMPORT) ;        // free resource

	if (!setup.sernum){
		setup.sernum= 0xffffffff ;
		EKS_NewSetup(&setup) ;
	}
    return(&setup) ;            // end of memory
}

void EKS_NewSetup(struct MYSETUP * ms)		// USE_PARAMETERS_ON_EEPROM
{
    KS_lockw(EEPROMPORT) ;         // we trust with

    EEPROM_Erase((EE_SN_SETUP/EEPROM_PAGE_SIZE)) ;

    EEPROM_Write(EEPROM_PAGE_OFFSET(EE_SN_SETUP), (EE_SN_SETUP/EEPROM_PAGE_SIZE), (void*) (ms), MODE_32_BIT, 1 ) ;

    KS_unlock(EEPROMPORT) ;        // free resource
}

int EKS_ParSize(void)		// USE_PARAMETERS_ON_EEPROM
{
    return(PARAM_STOP - PARAM_START + 1) ;
}

struct PKTMEMBUF * EKS_ParRead(unsigned char num)		// USE_PARAMETERS_ON_EEPROM
{
int i ;
unsigned char buf[2] ;
struct PKTMEMBUF *pmfirst, *pmactual, *pmnew ;      // data chain
unsigned char len,plen , odd ;

	pmfirst=pmactual=MNULL ;    // initialize pointers for data chain
	odd = NO ;
	
	KS_lockw(EEPROMPORT) ;         // we trust with

	// Read & write multiple of 2
	for(i=PARAM_START ; i<PARAM_STOP ; ) {
		EEPROM_Read(EEPROM_PAGE_OFFSET(i), (i/EEPROM_PAGE_SIZE), (void*) (buf), MODE_16_BIT, 1 ) ;
		i +=2 ;
		if (!buf[0]){ // EEprom empty
			KS_unlock(EEPROMPORT) ;        // free resource
			return(MNULL) ;             // end of memory
		}
		if (buf[0] == num) {        // we got it
			len = buf[1] ;
			if (len & 0x1){
				len++ ; // will by multiple of 2
				odd = YES ;
			}
			while(len) {      // create a new chain
				plen = MIN(len, msize - sizeof(struct PKTMEMBUF)) ;
				//if (plen & 0x1) plen++ ;
				if ((pmnew = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
					KS_unlock(EEPROMPORT) ;// free resource
					panic(1) ;
				}
				pmnew->next = MNULL ;                   // no others by now
				pmnew->offs = sizeof(struct PKTMEMBUF) ;// data start
				if ((odd) && (len==plen))
					pmnew->mtot = plen-1 ;
				else
					pmnew->mtot = plen ;
				EEPROM_Read(EEPROM_PAGE_OFFSET(i), (i/EEPROM_PAGE_SIZE), 
							((void *)(pmnew)) + sizeof(struct PKTMEMBUF), MODE_16_BIT, (plen>>1) ) ;
				if (pmfirst == MNULL) {
					pmfirst = pmnew ;
				}
				if (pmactual) {
					pmactual->next = pmnew ;
				}
				pmactual = pmnew ;

				i += plen ;
				len -= plen ;
			}
			KS_unlock(EEPROMPORT) ;// free resource
			return(pmfirst) ;   // done
//		} else if ((buf[0] != 0xff) && (buf[0]) ){ // free memory
		} else if (buf[0] != 0xff){ // free memory
			i += buf[1] ;   // pointer to next parameter
			if (buf[1] & 0x1) i++ ;
		}
	}

	KS_unlock(EEPROMPORT) ;        // free resource
	return(MNULL) ;             // end of memory
}

int EKS_ParWrite(unsigned char num, struct PKTMEMBUF ** ppst)		// USE_PARAMETERS_ON_EEPROM
{
int i ;
unsigned char buf[2] ;
//unsigned char * p ;
unsigned char locvar ;
size_t len, reallen, plen ;
int flag ;
struct PKTMEMBUF * pmnext ;
// #ifdef SYS1
// 	unsigned short eecrc ;
// #endif

	len = EKS_PktBufLength(*ppst) ;
	reallen = MIN(250, len) ;

	KS_lockw(EEPROMPORT) ;         // we trust with

	flag = ERROR ;              // default no valid data
	locvar = 0 ;
	for(i=PARAM_START ; i<PARAM_STOP ; ) {
		EEPROM_Read(EEPROM_PAGE_OFFSET(i), (i/EEPROM_PAGE_SIZE), (void*) (buf), MODE_16_BIT, 1 ) ;
		i+= 2 ;
		if (!buf[0]){ // EEprom empty
			break ;
		}
		if (buf[0] == num) {        // we got it
			i -= 2 ;
			flag = OK ;             // something has been erased
			locvar = buf[1] + 2 ;   // erase everything
			if (buf[1] & 0x1) locvar++ ;
			// Delete old
			buf[0] = 0xff ;
			buf[1] = 0xff ;
			while((locvar) && (i<PARAM_STOP)) {
				EEPROM_Write(EEPROM_PAGE_OFFSET(i), (i/EEPROM_PAGE_SIZE), (void*) (buf), MODE_16_BIT, 1 ) ;
				i+= 2 ;
				locvar-= 2 ;
			}
//		} else if ((buf[0] != 0xff) && (buf[0]) ){ // free memory
		} else if (buf[0] != 0xff){ // free memory
			i += buf[1] ;   // pointer to next parameter
			if (buf[1] & 0x1) i++ ;
		}
	}
	

// 	while((eeprom_last > EEPROM_BEGIN) && (*(eeprom_last-1) == 0xff)) {
// 		eeprom_last-- ;
// 	}

	if (*ppst) {        // only if valid argument
	// Read & write multiple of 2
		for(i=PARAM_START ; i<PARAM_STOP ; ) {
			EEPROM_Read(EEPROM_PAGE_OFFSET(i), (i/EEPROM_PAGE_SIZE), (void*) (buf), MODE_16_BIT, 1 ) ;
			i+= 2 ;

			if (((buf[0] == 0xff) || (!buf[0])) && (locvar==0)) {  // start of free area
				locvar = 2 ;
			} else if ((buf[0] == 0xff) || (!buf[0])) {            // free area goes on
				locvar += 2 ;
				if (locvar >= (reallen+2)) {    // hey, what we need !
					i -= locvar ;            // return to begin
					// Write header
					buf[0] = num ;                // this par
					buf[1] = reallen ;            // this len
					EEPROM_Write(EEPROM_PAGE_OFFSET(i), (i/EEPROM_PAGE_SIZE), 
								(void*) (buf), MODE_16_BIT, 1 ) ;
					i += 2 ;
					// If odd
					if (reallen & 0x1){ // add a char
						buf[0] = 0xff ;
						pmnext = *ppst ;
						while (pmnext->next) pmnext = pmnext->next ;
						EKS_PktBufAdd(buf[0], ppst, &pmnext ) ;
						reallen++;
					}
					while(len>0) {
						if (reallen) {      // if inside bounds
							plen = MIN(reallen, msize - sizeof(struct PKTMEMBUF)) ;
							EEPROM_Write(EEPROM_PAGE_OFFSET(i), (i/EEPROM_PAGE_SIZE), 
									((void *)(*ppst)) + sizeof(struct PKTMEMBUF), MODE_16_BIT, (plen>>1) ) ;
							pmnext = *ppst ;
							pmnext = pmnext->next ;
							KS_free(PKTBUF, *ppst) ;
							*ppst = pmnext ;
							reallen -= plen ;
							i += plen ;
							if (plen>len)
								len=0 ;
							else
								len -= plen ;
						}
					}
// 					if (eeprom_last < p)
// 						eeprom_last = p ;       // new last pointer

					flag = OK ;                 // done
					break ;
				}
				//i += 2 ;
			} else {                            // free area too small
				locvar = 0 ;
				i += buf[1] ;       // pointer to next parameter
				if (buf[1] & 0x1) i++ ;
			}
		}
	}

// #ifdef SYS1
// 	if (flag == OK)
// 		ParDoCrc() ;                    // Crc rebuild
// #endif
	KS_unlock(EEPROMPORT) ;                // free resource
	EKS_PktBufRelease(*ppst) ;          // return memory
	return(flag) ;                      // result
}

void EKS_ParClear(void)		// USE_PARAMETERS_ON_EEPROM
{
int i ;

	KS_lockw(EEPROMPORT) ;         // we trust with

	for(i=PARAM_START ; i<(PARAM_STOP+1-EEPROM_PAGE_SIZE) ; i+=EEPROM_PAGE_SIZE) {
		EEPROM_Erase((i/EEPROM_PAGE_SIZE)) ;
	}
	
	KS_unlock(EEPROMPORT) ;                // free resource
}

#endif // USE_PARAMETERS_ON_EEPROM

#if defined(USE_PKTMEMBUF) || defined(CBUG)
struct PKTMEMBUF * CheckPointer(short tt, struct PKTMEMBUF * mptr)
{
short cnt ;
unsigned long pmdup ;
extern TCB * hipritsk ;
extern TCB rtxtcb[] ;
#ifdef MTS_CODE
char ac ;
extern unsigned char rtxtid[] ;
#endif // MTS_CODE

#if defined(USE_LPC17XX) || defined(USE_LPC1788)
    // WARNING RAM base addr 0x10000000
    return(mptr) ; // +++++++++++++++++++++++++++++++++++++++ _BM_ _FR_
#endif // defined(USE_LPC17XX) || defined(USE_LPC1788)

#if defined(USE_AT91SAM3S4)
    // WARNING RAM base addr 0x20000000
    return(mptr) ; // +++++++++++++++++++++++++++++++++++++++ _BM_ _FR_
#endif // defined(USE_AT91SAM3S4)

// FR-CHECK
	pmdup = (unsigned long ) mptr ;
	//if (pmdup & Q_MASKATTR){
	if (pmdup > 0x300000){
	    for(cnt=1 ; cnt<(ntasks+1) ; cnt++){
	    	if (hipritsk == &rtxtcb[cnt])
	    		break ;
	    }
#ifdef MTS_CODE
	    ac = rtxtid[cnt] ;
	    printf("-ERR%d %d %lx-\n", tt, ac, pmdup) ;
	    KS_delay(SELFTASK, ((TICKS)(150)/CLKTICK)) ;
#endif // MTS_CODE
	    pmdup &= Q_MASKADDR ;
	}
// FR-CHECK
	return(((struct PKTMEMBUF *)(pmdup))) ;
}

// Return PKTMEMBUF chain to system
void EKS_PktBufRelease(struct PKTMEMBUF * mptr)
{
        struct PKTMEMBUF * pmnext ;
        while(mptr) {       // loop until somethig to release
                mptr = CheckPointer(1,mptr) ;
                pmnext = mptr->next ;
                KS_free(PKTBUF, mptr) ;
                mptr = pmnext ;
        }
}

// Concatenate PKTMEMBUF 2 chains
void EKS_PktBufCat(struct PKTMEMBUF * mpf, struct PKTMEMBUF * mps)
{
        if (!mpf)           // not a chain
                return ;

       mpf = CheckPointer(2,mpf) ;
       mps = CheckPointer(3,mps) ;

       while(mpf->next)    // search end of first chain
                mpf = mpf->next ;

        mpf->next = mps ;   // concatenate
}

// Duplicate PKTMEMBUF chain
struct PKTMEMBUF * EKS_PktBufDuplicate(struct PKTMEMBUF * mptr)
{
        struct PKTMEMBUF * pmdup, * pmfirst, * pmlast ;

        pmfirst = pmlast = MNULL ;

       	mptr = CheckPointer(4,mptr) ;

        while(mptr) {       // loop until somethig to duplicate
                if ((pmdup = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
                        EKS_PktBufRelease(pmfirst) ;
                        return(MNULL) ;             // return with error
                }
                if (!pmfirst)
                        pmfirst = pmdup ;

                pmdup->offs = mptr->offs ;      // copy structure
                pmdup->mtot = mptr->mtot ;
                pmdup->next = MNULL ;
                memcpy(((unsigned char *)(pmdup))+(pmdup->offs),
                           ((unsigned char *)(mptr))+(mptr->offs),
                           mptr->mtot) ;

                if (pmlast)
                        pmlast->next = pmdup ;      // maintain chain alive

                pmlast = pmdup ;
                mptr = mptr->next ;
        }

        return(pmfirst) ;
}

// Count PKTMEMBUF chain byte number
size_t EKS_PktBufLength(struct PKTMEMBUF * mptr)
{
        size_t cnt ;

        cnt = 0 ;
        while(mptr) {       // loop until end of chain
                mptr = CheckPointer(5,mptr) ;
                cnt += (size_t)(mptr->mtot) ;
                mptr = mptr->next ;
        }

        return(cnt) ;
}

// Add one more byte to PKTMEMBUF chain. If necessary, add one more element
int EKS_PktBufAdd(unsigned char cts, struct PKTMEMBUF ** ppmfirst, struct PKTMEMBUF ** ppmactual)
{
        struct PKTMEMBUF * ph ;

        ph = *ppmactual ;                   // keep actual up-to-date

        if ((ph == MNULL) || ((ph->offs + ph->mtot) == msize)) {    // time to allocate new buffer
                if ((*ppmactual = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
                        EKS_PktBufRelease(*ppmfirst) ;      // Release memory if error
                        *ppmfirst = MNULL ;
                        *ppmactual = MNULL ;
                        panic(4) ;              // nothing to do: NO RETURN
                        return(ERROR) ;         // return with error
                }

                if (*ppmfirst == MNULL)
                        *ppmfirst = *ppmactual ;    // This is the first allocation

                if (ph)                         // only if last points to something
                        ph->next = *ppmactual ;     // new link of last buffer

                ph = *ppmactual ;               // initialize actual
                ph->next = MNULL ;              // no others by now
                ph->offs = sizeof(struct PKTMEMBUF) ;   // data start
                ph->mtot = 0 ;                          // no data by now
        }

        *(((unsigned char *)(ph))+(ph->offs)+((ph->mtot)++)) = cts ;

        return(OK) ;                // Return without errors
}

// Add one more byte to PKTMEMBUF chain. If necessary, add one more element
int EKS_PktBufAddString(unsigned char *pcts, unsigned char slen, struct PKTMEMBUF ** ppmfirst, struct PKTMEMBUF ** ppmactual)
{
        if (!slen) slen = (unsigned char)(strlen((char *)(pcts))) ;
        while(slen--) {
                // remember: if EKS_PktBufAdd has an error, it panics and never returns
                EKS_PktBufAdd(*pcts++, ppmfirst, ppmactual) ;
        }
        return(OK) ;                // Return without errors
}

// Copy and delete "cnt" bytes from beginning of PKTMEMBUF. Return number of
// bytes actually pulled off
size_t EKS_PktBufPullup(struct PKTMEMBUF ** ppmf, unsigned char * buf, size_t cnt)
{
        struct PKTMEMBUF *pm ;
        size_t n, tot ;

        tot = 0 ;

       	*ppmf = CheckPointer(6,*ppmf) ;

        while((*ppmf != MNULL) && (cnt != 0)) {
                pm = *ppmf ;
                n = MIN(cnt, pm->mtot) ;
                if (buf != NULL){
                        memcpy(buf,                                 // user destination
                                   ((unsigned char *)(pm))+(pm->offs),  // packet start
                                   n) ;                                 // bytes #
                        buf += n ;
                }
                tot += n ;
                cnt -= n ;
                pm->offs += n ;
                pm->mtot -= n ;
                if (pm->mtot == 0) {
                        *ppmf = pm->next ;          // new chain entry point
                        KS_free(PKTBUF, pm) ;       // no longer used
                }
        }
        return(tot) ;
}

// Copy but NOT delete "cnt" bytes from beginning of PKTMEMBUF. Return number of
// bytes actually copied
size_t EKS_PktBufCopy(struct PKTMEMBUF * pmf, unsigned char * buf, size_t cnt)
{
        size_t n, tot ;

        tot = 0 ;

        while((pmf != MNULL) && (cnt != 0)) {
                n = MIN(cnt, pmf->mtot) ;
                if (buf != NULL){
                        memcpy(buf,                                 // user destination
                                   ((unsigned char *)(pmf))+(pmf->offs),  // packet start
                                   n) ;                                 // bytes #
                        buf += n ;
                }
                tot += n ;
                cnt -= n ;
                if (pmf->mtot == n) {
                        pmf = pmf->next ;           // new chain entry point
                }
        }
        return(tot) ;
}

// Compare but NOT delete "cnt" bytes from beginning of PKTMEMBUF. Return YES in
// case of match
int EKS_PktBufComp(struct PKTMEMBUF * pmf, const char * buf, size_t cnt)
{
        size_t n ;

        while(cnt) {
                if (pmf == MNULL)
                        return(NO) ;        // mismatch

                n = MIN(cnt, pmf->mtot) ;
                if (memcmp(buf,                                 // what to compare
                                   ((unsigned char *)(pmf))+(pmf->offs),  // packet start
                                   n))                                  // bytes #
                        return(NO) ;
                buf += n ;
                cnt -= n ;
                if (pmf->mtot == n) {
                        pmf = pmf->next ;           // new chain entry point
                }
        }
        return(YES) ;
}

// Compare but NOT delete 2 PKTMEMBUF. Return YES if equal
int EKS_Pkt2BufComp(struct PKTMEMBUF * pmf1, struct PKTMEMBUF * pmf2)
{
        size_t n, cnt, n1, n2 ;

        cnt=EKS_PktBufLength(pmf1) ;
        if (cnt!= EKS_PktBufLength(pmf2) ) return(NO) ;
        n1 = n2 = 0 ;

        while(cnt) {
                if ( (pmf1 == MNULL) || (pmf2 == MNULL) )
                        return(NO) ;        // mismatch

                n = MIN((pmf2->mtot-n2), (pmf1->mtot-n1)) ;
                if (memcmp(((unsigned char *)(pmf1))+(pmf1->offs)+n1,
                           ((unsigned char *)(pmf2))+(pmf2->offs)+n2,
                                                                   n))              // bytes #
                        return(NO) ;

                n1 = (pmf1->mtot)>(n+n1)?  ((pmf1->mtot)-(n+n1)) : 0 ;
                n2 = (pmf2->mtot)>(n+n2)?  ((pmf2->mtot)-(n+n2)) : 0 ;

                cnt -= n ;
                if (pmf1->mtot == n)  pmf1 = pmf1->next ;           // new chain entry point
                if (pmf2->mtot == n)  pmf2 = pmf2->next ;           // new chain entry point
        }

        return(YES) ;
}
#endif // defined(USE_PKTMEMBUF) || defined(CBUG)

#ifdef USE_TRANSACTIONS_ON_ARM
// <- Added from 1.55 _FR_ 13/01/11
// Check if to store (or not) chain in Flash
int EKS_PktBufCheckStore(struct PKTMEMBUF * pmf, short signature, short id, unsigned char p_seqok, unsigned char p_trlast)
{
unsigned short par_chk ;

	// get num of last received in sequece
	if (EKS_ParReadShort(p_seqok, &par_chk)) par_chk = 0 ;
	// if already received in sequece no store
	if (id<par_chk){
		EKS_PktBufRelease(pmf) ;    // Release memory if error
		return(ERROR) ;
	}

	// get num of last absolute received
	if (EKS_ParReadShort(p_trlast, &par_chk)) par_chk = 0 ;
	// if duplicate of last no store
	if (id==par_chk){
		EKS_PktBufRelease(pmf) ;    // Release memory if error
		return(ERROR) ;
	}

	// will be check if already received
	if (id<par_chk){
#ifdef TRANS_FASTCHECK
		if (EKS_PktBufCheck(signature, id, NULL)){	// already received
#else // TRANS_FASTCHECK
		if (EKS_PktBufCheck(signature, id)){	// already received
#endif // TRANS_FASTCHECK
			EKS_PktBufRelease(pmf) ;    // Release memory if error
			return(ERROR) ;
		}
	}

	// Will be store
	return( EKS_PktBufStore(pmf, signature, id)) ;
}

#ifdef TRANS_PACK
extern void NAND_FlashRead(unsigned char *dst, unsigned long bbegin, int flen) ;
extern void NAND_FlashErase(unsigned long bbegin, unsigned long bend) ;
extern void NAND_FlashWrite(unsigned long bbegin, unsigned char *src, int flen) ;

short EKS_transprepack(short * ntr )
{
size_t i ;
short lid, lsign[3], lntr[3] ;
unsigned long rfaddr, wfaddr, tot_read ;
struct PKTMEMBUF *ph ;      // outgoing list
struct FFILE head ;
unsigned char * lp ;
//char lbuf[64] ;

#ifdef USE_NANDFLASH_ON_ARM
uint32_t bsize ;

	bsize = nand_getBlockSize() ;
	KS_lockw(NANDPORT) ;        // we trust with
	NAND_FlashErase(TRPACK_START, TRPACK_STOP) ;
	//trans_addr = FLASH_START ;
#else
	KS_lockw(SPIPORT) ;         // we trust with
	SPI_FlashErase(TRPACK_START, TRPACK_STOP) ;
#endif

#ifdef USECIRCULARBUFFER
// trans_addr is next to write
// #ifdef USE_NANDFLASH_ON_ARM
// 	if (Nand_save.addr[BNAND_TRANS]==0L) Nand_save.addr[BNAND_TRANS] = FLASH_START ; 
// 	rfaddr = Nand_save.addr[BNAND_TRANS] + nand_addrinbuf[BNAND_TRANS] ;
// #else
	rfaddr = trans_addr ;
//#endif
#ifdef USE_PDEBUG
	//if (par71 & 0x1)
			pdebugt(1,"EKS_transprepack start0 0x%lx", rfaddr) ;
#endif // #ifdef USE_PDEBUG
// Added from 2.21
	if (rfaddr==0L){
		for(rfaddr=FLASH_START ; ; ) {       // find tail
			if (rfaddr >= (FLASH_STOP-sizeof(head))) {
#ifdef USE_NANDFLASH_ON_ARM
				KS_unlock(NANDPORT) ;        // we trust with
#else
				KS_unlock(SPIPORT) ;         // we trust with
#endif
				return(0) ;
			}

#ifdef USE_PDEBUG
	//if (par71 & 0x1)
			pdebugt(1,"EKS_transprepack startAA 0x%lx", rfaddr) ;
#endif // #ifdef USE_PDEBUG

			// memcpy((unsigned char *)(&head), faddr, sizeof(head)) ;
#ifdef USE_NANDFLASH_ON_ARM
			NAND_FlashRead((unsigned char *)(&head), rfaddr, sizeof(head)) ;
#else
			SPI_FlashRead((unsigned char *)(&head), rfaddr, sizeof(head)) ;
#endif

			if (head.fsign == -1) {         // first free cell
				break ;
			}

#ifdef USE_PDEBUG
	//if (par71 & 0x1)
			pdebugt(1,"EKS_transprepack startAB 0x%x", head.fsign) ;
#endif // #ifdef USE_PDEBUG

			// try next
			rfaddr += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
		}
	}
// Added from 2.4x
	
	// search older transaction
	tot_read = 0L ;				// at start=0, at end=FLASH_TOTSIZE
	do{
		rfaddr = (rfaddr & (~(FLASH_TSSIZE-1))) + FLASH_TSSIZE ;
		if (rfaddr >= FLASH_STOP)         // handle roll over
			rfaddr = FLASH_START ;
#ifdef USE_PDEBUG
	//if (par71 & 0x1)
			pdebugt(1,"EKS_transprepack check 0x%lx", rfaddr) ;
#endif // #ifdef USE_PDEBUG
		if (tot_read>FLASH_TOTSIZE){
#ifdef USE_NANDFLASH_ON_ARM
			KS_unlock(NANDPORT) ;        // we trust with
#else
			KS_unlock(SPIPORT) ;         // we trust with
#endif
			return(0) ;
		}
#ifdef USE_NANDFLASH_ON_ARM
		NAND_FlashRead((unsigned char *)(&head), rfaddr, sizeof(head)) ;
		if (head.fsign == -1){ // move to next block
			rfaddr = (rfaddr / bsize) ;
			rfaddr++ ;
			rfaddr *= bsize ;
			rfaddr -= FLASH_TSSIZE ;
			tot_read += bsize ;
		}
#else
		SPI_FlashRead((unsigned char *)(&head), rfaddr, sizeof(head)) ;
		tot_read += FLASH_TSSIZE ;
#endif
	}while(head.fsign == -1) ;
#else
	rfaddr = FLASH_START ;
#endif

#ifdef USE_PDEBUG
	//if (par71 & 0x1)
			pdebugt(1,"EKS_transprepack start2 0x%lx", rfaddr) ;
#endif // #ifdef USE_PDEBUG

	tot_read = 0L ;				// at start=0, at end=FLASH_TOTSIZE
	wfaddr = TRPACK_START ;
	lsign[0] = SMS_OUT ;
	lsign[1] = DIR11OUT ;
	lsign[2] = DIR2OUT ;
	
	lntr[0] = lntr[1] = lntr[2] = 0 ;
	
	ph = (struct PKTMEMBUF *)(KS_alloc(PKTBUF)) ;
	
	while(tot_read<FLASH_TOTSIZE){
#ifdef USE_NANDFLASH_ON_ARM
		NAND_FlashRead((unsigned char *)(&head), rfaddr, sizeof(head)) ;
#else
		SPI_FlashRead((unsigned char *)(&head), rfaddr, sizeof(head)) ;
#endif
#ifdef USE_PDEBUG
		//if (par71 & 0x1)
			pdebugt(1,"Check ToPack(0x%lx) 0x%04x: %d", rfaddr, head.fsign, head.fid) ;
#endif // #ifdef USE_PDEBUG
		if (head.fsign == -2) {               // filler
			rfaddr += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
			// -> Added from 1.55 _FR_ 13/01/11
			tot_read += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
			
// 			tot_read += (FLASH_TSSIZE - (rfaddr & (FLASH_TSSIZE-1)) ) ;
// 			rfaddr = (rfaddr & (~(FLASH_TSSIZE-1))) + FLASH_TSSIZE ;
			continue ;                    // try with this page
		}
		if (head.fsign == -1){                // end of page
#ifdef USE_PDEBUG
		//if (par71 & 0x1) 
			pdebugt(1,"End Check ToPack(0x%lx) 0x%x: %d", rfaddr, head.fsign, head.fid) ;
#endif // #ifdef USE_PDEBUG
			break ;
		}
		
		rfaddr += ((unsigned long)(sizeof(head))) ;
		tot_read += ((unsigned long)(sizeof(head))) ;
		
		for(lid=0;lid<3;lid++){	// lid: 0=SMS_OUT, 1=DIR11OUT, 2=DIR2OUT
			if (ntr[lid]>-1){
				if ((head.fsign == lsign[lid]) && (head.fid > ntr[lid]) ){ // to store
					lntr[lid]++;
#ifdef USE_PDEBUG
					//if (par71 & 0x1) 
						pdebugt(1,"ToPack 0x%x: %d->%d", head.fsign, head.fid, lntr[lid]) ;
#endif // #ifdef USE_PDEBUG
					head.fid = lntr[lid] ;
#ifdef USE_SERIALFLASH_ON_ARM
					if ((wfaddr+sizeof(head)+head.flen) >= TRPACK_STOP){         // handle roll over ->error
						KS_free(PKTBUF, ph) ;
						SPI_FlashErase(TRPACK_START, TRPACK_STOP) ;
						KS_unlock(SPIPORT) ;         // we trust with
						return(0) ;
					}
					KS_lockw(EXTAPI) ;              // we trust with
					SPI_FlashWrite(wfaddr, (unsigned char *)(&head), extapibuf, sizeof(head)) ;
					KS_unlock(EXTAPI) ;             // free resource
#else
					NAND_FlashWrite(wfaddr, (unsigned char *)(&head), sizeof(head)) ;
#endif
					wfaddr += sizeof(head) ;
					
					// Get data & change trans nr into 
					i = head.flen ;             // bytes to read
					
					while(i){
						
						ph->mtot = MIN(i, msize - sizeof(struct PKTMEMBUF)) ;
						
#ifdef USE_NANDFLASH_ON_ARM
						NAND_FlashRead(((unsigned char *)(ph))+(ph->offs), rfaddr, ph->mtot) ;
#else
						SPI_FlashRead(((unsigned char *)(ph))+(ph->offs), rfaddr, ph->mtot) ;
#endif
						
						if (i==head.flen){ // First read -> change trans nr into data
							lp = ((unsigned char *)(ph))+(ph->offs) ;
							lp[0] = (lntr[lid] & 0xff) ;
							lp[1] = (lntr[lid] & 0xff00)>>8 ;
						}
						rfaddr += ph->mtot ;
						i -= ph->mtot ;
						
#ifdef USE_NANDFLASH_ON_ARM
						NAND_FlashWrite(wfaddr, ((unsigned char *)(ph))+(ph->offs), ph->mtot) ;
#else
						KS_lockw(EXTAPI) ;              // we trust with
						SPI_FlashWrite(wfaddr, ((unsigned char *)(ph))+(ph->offs), extapibuf, ph->mtot) ;
						KS_unlock(EXTAPI) ;             // free resource
#endif
						wfaddr += ph->mtot ;

					}
				}else
					rfaddr += head.flen ;
			}
		}
		tot_read += ((unsigned long)(head.flen)) ;
		
	}
#ifdef USE_NANDFLASH_ON_ARM
	KS_unlock(NANDPORT) ;        // we trust with
#else
	KS_unlock(SPIPORT) ;         // we trust with
#endif
	KS_free(PKTBUF, ph) ;
	
	return(1) ; // OK
}

short EKS_transpack(short * ntr)
{
size_t i ;
short lid, lsign[3] ;
unsigned long rfaddr ;
struct PKTMEMBUF *phf, *pha, *ph ;      // outgoing list
struct FFILE head ;
RESOURCE pksema ;

#ifdef USE_NANDFLASH_ON_ARM
	pksema = NANDPORT ;
#else
	pksema = SPIPORT ;
#endif
	
	EKS_FlashClear() ;
	rfaddr = TRPACK_START ;
	trans_addr = FLASH_START ;
	lsign[0] = SMS_OUT ;
	lsign[1] = DIR11OUT ;
	lsign[2] = DIR2OUT ;
	
	ntr[0] = ntr[1] = ntr[2] = 0 ;
	
	ph = (struct PKTMEMBUF *)(KS_alloc(PKTBUF)) ;

#ifdef CBUG_
	KS_signal(CBUGSEMA) ;       // wake up debugger now
#endif

	while(rfaddr<TRPACK_STOP){
		KS_lockw(pksema) ;        // we trust with
#ifdef USE_NANDFLASH_ON_ARM
		//KS_lockw(NANDPORT) ;        // we trust with
		NAND_FlashRead((unsigned char *)(&head), rfaddr, sizeof(head)) ;
#else
		//KS_lockw(SPIPORT) ;         // we trust with
		SPI_FlashRead((unsigned char *)(&head), rfaddr, sizeof(head)) ;
#endif
		if (head.fsign == -1){              // end of page
			KS_unlock(pksema) ;         // we trust with
			break ;
		}
		rfaddr += ((unsigned long)(sizeof(head))) ;
		
// Fill with 'filler ' if needed

		for(lid=0;lid<3;lid++){	// lid: 0=SMS_OUT, 1=DIR11OUT, 2=DIR2OUT
			if (head.fsign == lsign[lid]){
				ntr[lid]++;
				head.fid = ntr[lid] ;
#ifdef USE_PDEBUG
					//if (par71 & 0x1) 
						pdebugt(1,"Pack 0x%x: %d->%d", head.fsign, head.fid, ntr[lid]) ;
#endif // #ifdef USE_PDEBUG
			}
		}
		// Use EKS_PktBufStore for correct store
		// Get data & change trans nr into 
		i = head.flen ;             // bytes to read
		
		ph = phf = pha = MNULL ;             // initialize pointers for data
		while(i){
			if ((pha = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
				EKS_PktBufRelease(phf) ;      // Release memory if error
				phf = pha = MNULL ;
				panic(5) ;              // nothing to do: NO RETURN
				KS_unlock(pksema) ;    // free resource
				return(0) ;         // return with error
			}

			if (phf == MNULL) phf = pha ;    // This is the first allocation

			// only if last points to something
			if (ph) ph->next = pha ;   // new link of last buffer

			ph = pha ;                 // initialize actual
			ph->next = MNULL ;              // no others by now
			ph->offs = sizeof(struct PKTMEMBUF) ;   // data start

			ph->mtot = MIN(i, msize - sizeof(struct PKTMEMBUF)) ;

#ifdef USE_NANDFLASH_ON_ARM
			NAND_FlashRead(((unsigned char *)(ph))+(ph->offs), rfaddr, ph->mtot) ;
#else
			SPI_FlashRead(((unsigned char *)(ph))+(ph->offs), rfaddr, ph->mtot) ;
#endif

			rfaddr += ph->mtot ;
			i -= ph->mtot ;
		}
		KS_unlock(pksema) ;         // we trust with
		EKS_PktBufStore(phf, head.fsign, head.fid ) ;
		
	}
	
#ifdef CBUG_
	KS_signal(CBUGSEMA) ;       // wake up debugger now
#endif

	return(YES) ;
}
#endif

#endif	// USE_TRANSACTIONS_ON_ARM

#if ( defined(USE_SPI_ON_ARM) && defined(USE_SERIALFLASH_ON_ARM) )

void EKS_LCK_FlashRead(unsigned long bbegin, unsigned char *dst, int flen)
{
        KS_lockw(SPIPORT) ;         // we trust with
        SPI_FlashRead(dst,bbegin, flen) ;
        KS_unlock(SPIPORT) ;        // free resource
}

void EKS_LCK_FlashWrite(unsigned long bbegin, unsigned char *src, int flen)
{
        KS_lockw(SPIPORT) ;         // we trust with
        KS_lockw(EXTAPI) ;              // we trust with
        SPI_FlashWrite(bbegin, src, extapibuf, flen) ;
        KS_unlock(EXTAPI) ;             // free resource
        KS_unlock(SPIPORT) ;        // free resource
}

void EKS_LCK_FlashErase(unsigned long bbegin, unsigned long bend)
{
        KS_lockw(SPIPORT) ;         // we trust with
        SPI_FlashErase(bbegin, bend) ;
        KS_unlock(SPIPORT) ;        // free resource
}

#ifdef USE_TRANSACTIONS_ON_ARM

// Clear data Flash
void EKS_FlashClear(void)
{
	KS_lockw(SPIPORT) ;         // we trust with
	SPI_FlashErase(FLASH_START, FLASH_STOP) ;
	trans_addr = 0L ;
	KS_unlock(SPIPORT) ;        // free resource
}

// Store a chain in Flash
int EKS_PktBufStore(struct PKTMEMBUF * pmf, short signature, short id)
{
struct PKTMEMBUF *pm ;
struct FFILE head ;
size_t i ;
unsigned long faddr ;

	if (pmf==MNULL)  return(ERROR) ;

#ifdef CBUG
	pdebugt(1,"Req TRadd: 0x%lx (0x%lx) %lu", faddr, trans_addr, (EKS_PktBufLength(pmf)+sizeof(head)) ) ;
#endif

	KS_lockw(SPIPORT) ;         // we trust with

	faddr = trans_addr ;
	
	if (trans_addr==0L){
		for(faddr=FLASH_START ; ; ) {       // find tail
			if (faddr >= (FLASH_STOP-sizeof(head))) {
				KS_unlock(SPIPORT) ;        // free resource
				EKS_PktBufRelease(pmf) ;    // Release memory if error
				return(ERROR) ;
			}

			// memcpy((unsigned char *)(&head), faddr, sizeof(head)) ;
			SPI_FlashRead((unsigned char *)(&head), faddr, sizeof(head)) ;

			if (head.fsign == -1) {         // first free cell
				break ;
			}

			if ((head.fsign == signature) && (head.fid == id)) {    // dup. error
				KS_unlock(SPIPORT) ;        // free resource
				EKS_PktBufRelease(pmf) ;    // Release memory if error
				return(ERROR) ;
			}

			// try next
			faddr += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
		}
	}
	
// check for page fault (From 1.70)

    i = EKS_PktBufLength(pmf) ;         // user len
#ifdef TRANS_FASTCHECK // Warning: to check,  bad result
// Added from 2.12 (no need to store data of SMS_IN transaction)
	if (signature==SMS_IN) i = 0 ; // = MIN( 2, i ) ;
#endif

#ifdef USECIRCULARBUFFER
	if ( ((faddr+i+(3*sizeof(head))) & (~(FLASH_TSSIZE-1))) != (faddr & (~(FLASH_TSSIZE-1))) ) {

		head.fsign = -2 ;               // filler
		head.fid = 1 ;
		head.flen = (size_t)(FLASH_TSSIZE - (faddr & (FLASH_TSSIZE-1)) - sizeof(head)) ;

                //BootProgramPage(faddr, (unsigned char *)(&head), sizeof(head)) ;
#ifdef CBUG
		if (sizeof(head) > sizeof(extapibuf)) { // sanity check
			*((char *)(0x80000000)) = 2 ;       // surely an error
		}
#endif // CBUG
		KS_lockw(EXTAPI) ;              // we trust with
		SPI_FlashWrite(faddr, (unsigned char *)(&head), extapibuf, sizeof(head)) ;
		KS_unlock(EXTAPI) ;             // free resource

			faddr = (faddr & (~(FLASH_TSSIZE-1))) + FLASH_TSSIZE ;
			if (faddr >= FLASH_STOP)         // handle roll over
					faddr = FLASH_START ;

			// BootProgramPage(faddr, NULL, FLASH_TSSIZE) ;
			SPI_FlashErase(faddr, faddr + FLASH_TSSIZE - 1) ;
	}
#endif  // USECIRCULARBUFFER

// write data// <- Added from 1.55 _FR_ 13/01/11

	head.fsign = signature ;            // from user
	head.fid = id ;
	head.flen = i ;                     // user len

        //BootProgramPage(faddr, (unsigned char *)(&head), sizeof(head)) ;
#ifdef CBUG
	if (sizeof(head) > sizeof(extapibuf)) { // sanity check
		*((char *)(0x80000000)) = 0 ;       // surely an error
	}
#endif // CBUG
	KS_lockw(EXTAPI) ;              // we trust with
	SPI_FlashWrite(faddr, (unsigned char *)(&head), extapibuf, sizeof(head)) ;
	KS_unlock(EXTAPI) ;             // free resource

	faddr += ((unsigned long)(sizeof(head))) ;

	i = head.flen ;             // bytes to write

	while(head.flen) {
		i = MIN(head.flen, pmf->mtot) ;

			//BootProgramPage(faddr, ((unsigned char *)(pmf))+(pmf->offs), i) ;
#ifdef CBUG// <- Added from 1.55 _FR_ 13/01/11
		if (sizeof(*pmf) > sizeof(extapibuf)) { // sanity check
			*((char *)(0x80000000)) = 1 ;       // surely an error
		}
#endif // CBUG
		KS_lockw(EXTAPI) ;              // we trust with
		SPI_FlashWrite(faddr, ((unsigned char *)(pmf))+(pmf->offs), extapibuf, i) ;
		KS_unlock(EXTAPI) ;             // free resource

		faddr += ((unsigned long)(i)) ;

		pmf->mtot -= i ;
		if (!(pmf->mtot)) {     // free this packet
			pm = pmf->next ;            // new chain entry point
			KS_free(PKTBUF, pmf) ;      // no longer used
			pmf = pm ;
		}

		head.flen -= i ;
	}
#ifdef CBUG
	pdebugt(1,"TRadd: 0x%lx (0x%lx) %lu", faddr, trans_addr, (i+sizeof(head)) ) ;
#endif
	trans_addr = faddr ;

	KS_unlock(SPIPORT) ;        // free resource

#ifdef TRANS_FASTCHECK
// Added from 2.12 (no need to store all data of SMS_IN transaction only 2 byte)
	if (signature==SMS_IN) EKS_PktBufRelease(pmf) ;
#endif
	if (pmf != MNULL) {         // if something still present in buffer
			EKS_PktBufRelease(pmf) ;// Release memory if error
// Removed from 2.12 (no need to store all data of SMS_IN transaction only 2 byte)
#ifndef TRANS_FASTCHECK
			return(ERROR) ;         // free from spurious data
#endif
	}

	return(OK) ;
}

// Retrive a chain from Flash
struct PKTMEMBUF * EKS_PktBufRetrive(short signature, short id, unsigned long *st_addr)
{
struct PKTMEMBUF *pmfirst, *pmactual ;      // outgoing list
struct FFILE head ;
size_t i ;
unsigned long faddr, tot_read ;
struct PKTMEMBUF *ph ;


	KS_lockw(SPIPORT) ;         // we trust with

// -> Added from 1.55 _FR_ 13/01/11
	tot_read = 0L ;				// at start=0, at end=FLASH_TOTSIZE
	faddr = 0L ;
	if (st_addr!=NULL){
		faddr = *st_addr ;
#if defined(USE_FLASHMONITORING) && defined(CBUG)
		pdebugt(1,"FLS: request retry 0x%hx %d (0x%lx)", signature, id, faddr ) ;
#endif // defined(USE_FLASHMONITORING) && defined(CBUG)
	}
	if ((faddr<FLASH_START) || (faddr>FLASH_STOP))
		faddr=FLASH_START ;

	while(tot_read<FLASH_TOTSIZE){
		// -> Added from 1.55 _FR_ 13/01/11
		if (faddr >= FLASH_STOP){         // handle roll over
			faddr = FLASH_START ;
#if defined(USE_FLASHMONITORING) && defined(CBUG)
			pdebugt(1,"EKS_PktBufRetrive: ADDR start") ;
#endif // defined(USE_FLASHMONITORING) && defined(CBUG)
		}
		// <- Added from 1.55 _FR_ 13/01/11
// <- Added from 1.55 _FR_ 13/01/11

// -> Removed from 1.55 _FR_ 13/01/11
// 	for(faddr=FLASH_START ; ; ) {       // find
// 		if (faddr >= (FLASH_STOP-sizeof(head))) {
// #if defined(USE_FLASHMONITORING) && defined(CBUG)
// 			pdebugt(1,"FLS: end") ;
// #endif // defined(USE_FLASHMONITORING) && defined(CBUG)
// 			KS_unlock(SPIPORT) ;          // free resource
// 			return(MNULL) ;
// 		}
// <- Removed from 1.55 _FR_ 13/01/11

		// memcpy_EP((unsigned char *)(&head), faddr, sizeof(head)) ;
		SPI_FlashRead((unsigned char *)(&head), faddr, sizeof(head)) ;

#ifdef USECIRCULARBUFFER
		if (head.fsign == -1) {               // end of page
			// -> Added from 1.55 _FR_ 13/01/11
			tot_read += (FLASH_TSSIZE - (faddr & (FLASH_TSSIZE-1)) ) ;
			// <- Added from 1.55 _FR_ 13/01/11
			
			faddr = (faddr & (~(FLASH_TSSIZE-1))) + FLASH_TSSIZE ;
			continue ;                    // try with this page
		}
#else
		if (head.fsign == -1) {               // end of buffer
			KS_unlock(SPIPORT) ;          // free resource
			return(MNULL) ;
		}
#endif  // USECIRCULARBUFFER

		if ((signature == -1) && (head.fid > id))       // first is ok
			break ;

		if ((head.fsign == signature) && (head.fid == id))
			break ;

// #if defined(USE_FLASHMONITORING) && defined(CBUG)
// 		pdebugt(1,"FLS: check retry 0x%hx!=0x%hx %d!=%d (0x%lx)", head.fsign, signature, head.fid, id, faddr ) ;
// #endif // defined(USE_FLASHMONITORING) && defined(CBUG)
		// try next
		faddr += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
		// -> Added from 1.55 _FR_ 13/01/11
		tot_read += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
	}

	// -> Added from 1.55 _FR_ 13/01/11
	if (tot_read>=FLASH_TOTSIZE){
#if defined(USE_FLASHMONITORING) && defined(CBUG)
			pdebugt(1,"FLS: end") ;
#endif // defined(USE_FLASHMONITORING) && defined(CBUG)
			KS_unlock(SPIPORT) ;          // free resource
			*st_addr = 0L ;
			return(MNULL) ;
	}
	if (st_addr!=NULL) *st_addr = faddr ;
	
#if defined(USE_FLASHMONITORING) && defined(CBUG)
	pdebugt(1,"FLS: sign=0x%x, addr=0x%lx, id=0x%x, len=%lu (0x%lx)",
									head.fsign,
									faddr,
									head.fid,
									head.flen,
									*st_addr) ;
#endif // defined(USE_FLASHMONITORING) && defined(CBUG)
	// <- Added from 1.55 _FR_ 13/01/11

	faddr += ((unsigned long)(sizeof(head))) ;

	i = head.flen ;             // bytes to read

	ph=pmfirst=pmactual=MNULL ;             // initialize pointers for data
	while(i) {
		if ((pmactual = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
			EKS_PktBufRelease(pmfirst) ;      // Release memory if error
			pmfirst = MNULL ;
			pmactual = MNULL ;
			panic(5) ;              // nothing to do: NO RETURN
			KS_unlock(SPIPORT) ;    // free resource
			return(MNULL) ;         // return with error
		}

		if (pmfirst == MNULL) {
			pmfirst = pmactual ;    // This is the first allocation
		}

		if (ph) {                       // only if last points to something
			ph->next = pmactual ;   // new link of last buffer
		}

		ph = pmactual ;                 // initialize actual
		ph->next = MNULL ;              // no others by now
		ph->offs = sizeof(struct PKTMEMBUF) ;   // data start

		ph->mtot = MIN(i, msize - sizeof(struct PKTMEMBUF)) ;

		SPI_FlashRead(((unsigned char *)(ph))+(ph->offs), faddr, ph->mtot) ;

		faddr += ph->mtot ;
		i -= ph->mtot ;
	}

	KS_unlock(SPIPORT) ;        // free resource
	return(pmfirst) ;
}

// Modified from 1.55 _FR_ 13/01/11 
// Check starting from last sector writed
// Check for a chain from Flash
#ifdef TRANS_FASTCHECK
int EKS_PktBufCheck(short signature, short id, short * id_retry)
{
short j, nf ; // nf = nr found
struct FFILE head ;
unsigned long faddr, tot_read ;

	KS_lockw(SPIPORT) ;         // we trust with

//  Added from 1.55 _FR_ 13/01/11
	tot_read = 0L ;				// at start=0, at end=FLASH_TOTSIZE
	faddr = (trans_addr & (~(FLASH_TSSIZE-1)))  ;

	nf = 0 ;
	if (id_retry!=NULL){
		for(j=0;j<5;j++){
			if (!id_retry[j]) nf++ ;
		}
	}
	while(tot_read<FLASH_TOTSIZE){
// -> Removed from 1.55 _FR_ 13/01/11
//	for(faddr=FLASH_START ; ; ) {       // find
// 		if (faddr >= (FLASH_STOP-sizeof(head))) {
// 			KS_unlock(SPIPORT) ;           // free resource
// 			return(NO) ;
// 		}
// <- Removed from 1.55 _FR_ 13/01/11

		//memcpy_EP((unsigned char *)(&head), faddr, sizeof(head)) ;
		SPI_FlashRead((unsigned char *)(&head), faddr, sizeof(head)) ;

#ifdef USECIRCULARBUFFER
		//if (head.fsign == -1) {                 // end of page
		if ((head.fsign == -1) || (head.fsign == -2)){                 // end of page or filler
//  -> Added from 1.55 _FR_ 13/01/11
			tot_read += (FLASH_TSSIZE - (faddr & (FLASH_TSSIZE-1)) ) ;
 			faddr = (faddr & (~(FLASH_TSSIZE-1))) - FLASH_TSSIZE ;
			if (faddr<FLASH_START) faddr = (FLASH_STOP & (~(FLASH_TSSIZE-1))) ;
//  <- Added from 1.55 _FR_ 13/01/11

// -> Removed from 1.55 _FR_ 13/01/11
// 			faddr = (faddr & (~(FLASH_TSSIZE-1))) + FLASH_TSSIZE ;
			continue ;                          // try with this page
		}
#else
		if (head.fsign == -1) {                 // end of buffer
			KS_unlock(SPIPORT) ;            // free resource
			return(NO) ;
		}
#endif  // USECIRCULARBUFFER

		if (head.fsign == signature) {
			if (id_retry!=NULL){
				for(j=0;j<5;j++){
					if (id_retry[j]==id){
						id_retry[j] = 0 ;
						nf++ ;
					}
				}
				if (nf==5){
					KS_unlock(SPIPORT) ;        // free resource
					return(YES) ;
				}
			}else{
				if (head.fid == id){
// -> Added from 1.55 _FR_ 13/01/11
					KS_unlock(SPIPORT) ;        // free resource
					return(YES) ;
//  <- Added from 1.55 _FR_ 13/01/11
				}
			}
// -> Removed from 1.55 _FR_ 13/01/11
//			break ;
		}
		
		// try next
		faddr += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
		// -> Added from 1.55 _FR_ 13/01/11
		tot_read += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
	}

	KS_unlock(SPIPORT) ;        // free resource
// -> Added from 1.55 _FR_ 13/01/11
	return(NO) ;
// -> Removed from 1.55 _FR_ 13/01/11
//	return(YES) ;
}

#else // TRANS_FASTCHECK

int EKS_PktBufCheck(short signature, short id)
{
struct FFILE head ;
unsigned long faddr, tot_read ;

	KS_lockw(SPIPORT) ;         // we trust with

//  Added from 1.55 _FR_ 13/01/11
	tot_read = 0L ;				// at start=0, at end=FLASH_TOTSIZE
	faddr = (trans_addr & (~(FLASH_TSSIZE-1)))  ;
	
	while(tot_read<FLASH_TOTSIZE){
// -> Removed from 1.55 _FR_ 13/01/11
//	for(faddr=FLASH_START ; ; ) {       // find
// 		if (faddr >= (FLASH_STOP-sizeof(head))) {
// 			KS_unlock(SPIPORT) ;           // free resource
// 			return(NO) ;
// 		}
// <- Removed from 1.55 _FR_ 13/01/11

		//memcpy_EP((unsigned char *)(&head), faddr, sizeof(head)) ;
		SPI_FlashRead((unsigned char *)(&head), faddr, sizeof(head)) ;

#ifdef USECIRCULARBUFFER
//		if (head.fsign == -1) {                 // end of page
		if ((head.fsign == -1) || (head.fsign == -2)){                 // end of page or filler
//  -> Added from 1.55 _FR_ 13/01/11
			tot_read += (FLASH_TSSIZE - (faddr & (FLASH_TSSIZE-1)) ) ;
 			faddr = (faddr & (~(FLASH_TSSIZE-1))) - FLASH_TSSIZE ;
			if (faddr<FLASH_START) faddr = (FLASH_STOP & (~(FLASH_TSSIZE-1))) ;
//  <- Added from 1.55 _FR_ 13/01/11

// -> Removed from 1.55 _FR_ 13/01/11
// 			faddr = (faddr & (~(FLASH_TSSIZE-1))) + FLASH_TSSIZE ;
			continue ;                          // try with this page
		}
#else
		if (head.fsign == -1) {                 // end of buffer
			KS_unlock(SPIPORT) ;            // free resource
			return(NO) ;
		}
#endif  // USECIRCULARBUFFER

		if ((head.fsign == signature) && (head.fid == id)){
// -> Added from 1.55 _FR_ 13/01/11
			KS_unlock(SPIPORT) ;        // free resource
			return(YES) ;
//  <- Added from 1.55 _FR_ 13/01/11

// -> Removed from 1.55 _FR_ 13/01/11
//			break ;
		}
		
		// try next
		faddr += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
		// -> Added from 1.55 _FR_ 13/01/11
		tot_read += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
	}

	KS_unlock(SPIPORT) ;        // free resource
// -> Added from 1.55 _FR_ 13/01/11
	return(NO) ;
// -> Removed from 1.55 _FR_ 13/01/11
//	return(YES) ;
}
#endif // TRANS_FASTCHECK

// Get total flash memory size
unsigned long EKS_FlashTotal(void)
{
	return(FLASH_STOP - FLASH_START + 1) ;
}

// Check free flash memory size
unsigned long EKS_FlashFree(void)
{
        struct FFILE head ;
        unsigned long total ;
        unsigned long faddr ;

        total = 0L ;        // init

        KS_lockw(SPIPORT) ;         // we trust with

        for(faddr=FLASH_START ; faddr < (FLASH_STOP-sizeof(head)) ; ) {      // find

                //memcpy_EP((unsigned char *)(&head), faddr, sizeof(head)) ;
                SPI_FlashRead((unsigned char *)(&head), faddr, sizeof(head)) ;

#ifdef USECIRCULARBUFFER
                if (head.fsign == -1) {                 // end of page
                        total += (FLASH_TSSIZE - (faddr & (FLASH_TSSIZE-1))) ;
                        faddr = (faddr & (~(FLASH_TSSIZE-1))) + FLASH_TSSIZE ;
                        continue ;                          // try with this page
                }
#else
                if (head.fsign == -1) {                 // end of buffer
                        total = FLASH_STOP - faddr ;
                        break ;
                }
#endif  // USECIRCULARBUFFER

                // try next
                faddr += (((unsigned long)(sizeof(head))) + ((unsigned long)(head.flen))) ;
        }

        KS_unlock(SPIPORT) ;        // free resource
        return(total) ;
}
#endif // USE_TRANSACTIONS_ON_ARM


#endif // ( defined(USE_SPI_ON_ARM) && defined(USE_SERIALFLASH_ON_ARM) )

// Check Flash checksum
unsigned short EKS_FlashCheckSum(void) {
        unsigned long faddr ;
        unsigned short chk ;

        chk = 0 ;
		faddr = 0L ;

		for(faddr=RUNCODE_START ; faddr < (RUNCODE_START+RUNCODE_SIZE) ; faddr+=2) {
                chk ^= *((unsigned short *) (faddr)) ;
        }

        return(chk) ;
}

#if defined(USE_PKTMEMBUF) || defined(CBUG)
struct PKTMEMBUF * EKS_PktBufMyTime(time_t giventime, char type){
struct PKTMEMBUF * ppm ;
char * p ;
char mtot ; // , c_crc ;

        switch (type){
                case 1 : mtot = 14 ; break ;    // EKS_PktBufComprexTime
                //case 2 : mtot = 17 ; break ;    // EKS_PktBufAccTime
                case 2 : mtot = 23 ; break ;    // BENEFON
                default: mtot = 19 ; break ;    // EKS_PktBufFromTime
        }

        if (msize < mtot)                     // buffer capacity
                return(MNULL) ;                 // return with error

        if ((ppm = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL)
                return(MNULL) ;                 // return with error

        ppm->next = MNULL ;                 // no others
        ppm->offs = sizeof(struct PKTMEMBUF) ;      // data start
        ppm->mtot = mtot ;                    // future data

        p = (char *)(ppm)+ppm->offs ;

        if (!giventime)
                giventime = KS_inqtime() ;      // get system time

        KS_lockw(ONLYONE) ;                 // we trust with

        systime2date(giventime, &tm) ;      // get time

        switch (type){
                case 1 :
                        sprintf(p, "%04d%02d%02d",
                                           (unsigned int)(tm.tm_yr),
                                           (unsigned int)(tm.tm_mon),
                                           (unsigned int)(tm.tm_day)) ;

                        p += 8 ;                            // point to end

                        sprintf(p, "%02d%02d%02d",
                                           (unsigned int)(tm.tm_hr),
                                           (unsigned int)(tm.tm_min),
                                           (unsigned int)(tm.tm_sec)) ;
                        break ;

                case 2 :
                        sprintf(p, "0,0\021%02d.%02d.%04d\021%02d:%02d:%02d",
                                           (unsigned int)(tm.tm_day),
                                           (unsigned int)(tm.tm_mon),
                                           (unsigned int)(tm.tm_yr),
                                           (unsigned int)(tm.tm_hr),
                                           (unsigned int)(tm.tm_min),
                                           (unsigned int)(tm.tm_sec)) ;
//                         c_crc = 0x6A ;
//                         c_crc ^= tm.tm_sec ;
//                         c_crc++;
//                         c_crc ^= tm.tm_min ;
//                         c_crc++;
//                         c_crc ^= tm.tm_hr ;
//                         c_crc++;
//                         c_crc ^= tm.tm_day ;
//                         c_crc++;
//                         sprintf(p, "0A0800%02x%02x%02x%02x%02x\n",
//                                            (unsigned int)(tm.tm_sec),
//                                            (unsigned int)(tm.tm_min),
//                                            (unsigned int)(tm.tm_hr),
//                                            (unsigned int)(tm.tm_day),
//                                            (unsigned int) c_crc ) ;

                        break ;

                default:
                        sprintf(p, "%02d/%02d/%04d",
                                           (unsigned int)(tm.tm_day),
                                           (unsigned int)(tm.tm_mon),
                                           (unsigned int)(tm.tm_yr)) ;

                        p += 10 ;                           // point to end

                        *p++ = ' ' ;                        // separator

                        sprintf(p, "%02d:%02d:%02d",
                                           (unsigned int)(tm.tm_hr),
                                           (unsigned int)(tm.tm_min),
                                           (unsigned int)(tm.tm_sec)) ;
                        break ;
        }
        KS_unlock(ONLYONE) ;        // we trust with

        return(ppm) ;
}

struct PKTMEMBUF * EKS_PktBufFromTime(time_t giventime)
{
        return(EKS_PktBufMyTime(giventime, 0)) ;
}

struct PKTMEMBUF * EKS_PktBufComprexTime(void)
{
        return(EKS_PktBufMyTime( 0L, 1)) ;
}

struct PKTMEMBUF * EKS_PktBufAccTime(time_t giventime)
{
        return(EKS_PktBufMyTime(giventime, 2)) ;
}

void EKS_GetHHMM(unsigned char * storetime)
{
        KS_lockw(ONLYONE) ;         // we trust with

        systime2date(KS_inqtime(), &tm) ;   // get system time

        sprintf((char *)(storetime), "%02d%02d",
                                           (unsigned int)(tm.tm_hr),
                                           (unsigned int)(tm.tm_min)) ;

        KS_unlock(ONLYONE) ;        // we trust with
}
#endif // defined(USE_PKTMEMBUF) || defined(CBUG)

//----------------------------------------------------------------------------
// Enquiry shutdown type
unsigned char EKS_EnqShutdown(void)
{
        return(sd_mode) ;           // latched shutdown mode
}

//----------------------------------------------------------------------------
// Task requires shutdown
void EKS_AskShutdown(unsigned char mode)
{
    unsigned char i ;
    unsigned long lppm ;

    // Moved  31/03/09 when no other task running
    // adcstop() ; // stop ADC system, it handles also FAST_ADC
	
	// If already requested code upgrade ignore next request
	if (sd_mode!=SD_CODEUPGRADE)
    	sd_mode = mode ;                        // latch shutdown mode

    lppm = 0 ;                              // void pointer means shutdown
    for(i=0 ; i<NumOfTasks ; i++) {
        KS_enqueuew(LU0Q+i, &lppm) ;        // send to destination
    }
}

//----------------------------------------------------------------------------
// Task agrees shutdown
void EKS_AgreeShutdown(void) {
long i ;
//time_t time_wd ;

#ifdef USE_LOW_POWER
    extern volatile unsigned char runall ;
#endif // USE_LOW_POWER

    NumOfTasks-- ;      // One more task agrees

#ifdef CBUG
	pdebugt(1,"NumOfTasks: %d(%d)", (int)NumOfTasks, (int) sd_mode) ;
#endif

    if (!NumOfTasks) {          // all tasks agree
#ifdef USE_PDEBUG_
	KS_signal(CBUGSEMA) ;       // wake up debugger now		
#endif		

    	adcstop() ; // stop ADC system, it handles also FAST_ADC

#if defined(USE_NANDFLASH_ON_ARM)
		//LPC_GPIO2->SET = (1<<HW_2o12_LEDG); // Off
		// If enabled
		if (sd_mode!=SD_CODEUPGRADE){
			if (nand_getSizeMB()){
#ifdef CBUG
				printf("Nand writetemp\n") ;
#endif
				nanddisable() ;
			}
#ifdef CBUG
			else
				printf("Nand disabled\n") ;
#endif
		}else{
			extern void NandWriteTemp(void) ;
			
			NandWriteTemp() ;
		}
// 		for( i=0;i<2000; i++) 	tickwait(1000) ;
#endif
		//KS_delay(SELFTASK, ((TICKS)100/CLKTICK)) ;
        // Modified 31/03/09 wait adcstop
#ifndef USE_LPC1788
    	KS_delay(SELFTASK, ((TICKS)700/CLKTICK)) ;
#endif
    	
        switch(sd_mode) {       // select what to do
        case SD_CODEUPGRADE :   // code upgrade and reboot
        	//AT91C_BASE_SYS->SYS_GPBR1 |= FLAG_REBOOT ;
            CodeUpgrade() ;     // NO RETURN
            break ;

        case SD_LATEBOOT :      // system reboot after time
        	//AT91C_BASE_SYS->SYS_GPBR1 |= FLAG_REBOOT ;
            DISABLE ;           // no more interrupts
//            time_wd = RTC_ReadTime_t() ;
            //for( ; ; ) ;        // Watch dog will reset after 16 sec
            // break ;
//#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
            for( i=0;i<2000; i++) 	tickwait(1000) ;

            //for( ; ; ) {				
            	//for(i=0;i<200000;i++) ;
				//if (RTC_ReadTime_t()> (time_wd+2L) ) break ;
            //}
//#endif
        case SD_REBOOT :        // system reboot
#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
            //AT91C_BASE_SYS->SYS_GPBR1 |= FLAG_REBOOT ;
            // Make software reset
            AT91C_BASE_RSTC->RSTC_RCR = (AT91C_RSTC_KEY & (0xa5<<24)) |
                                         AT91C_RSTC_PROCRST | // (RSTC) Processor Reset
                                         AT91C_RSTC_PERRST  | // (RSTC) Peripheral Reset
                                         AT91C_RSTC_EXTRST ;  // (RSTC) External Reset
#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#if defined(USE_LPC17XX)
            WDT->WDFEED=0xAA ;
            WDT->WDFEED=0x00 ;  // any value other than 55 will reset
#endif // defined(USE_LPC17XX)
#if defined(USE_LPC1788)
            LPC_WDT->FEED=0xAA ;
            LPC_WDT->FEED=0x00 ;  // any value other than 55 will reset
#endif // defined(USE_LPC1788)
#if defined(USE_AT91SAM3S4)
            // Make software reset
            RSTC->RSTC_CR = (RSTC_CR_KEY & (0xa5<<24)) |
                                 RSTC_CR_PROCRST | // (RSTC) Processor Reset
                                 RSTC_CR_PERRST  | // (RSTC) Peripheral Reset
                                 RSTC_CR_EXTRST ;  // (RSTC) External Reset
#endif // defined(USE_AT91SAM3S4)
            DISABLE ;       // no more interrupts
            for( ; ; ) ;    // just t be sure
            break ;

            // section to optimize and tune up
        case SD_STDBY           :   // stdby mode
        case SD_TURNOFF         :   // turn off
        case SD_UNVOLTSLEEP     :	// low batt without Vext
        case SD_NOPRESENCE      :   // no presence
        case SD_STDBYCH         :   // stdby mode with battery charge
        case SD_TURNOFFCH       :   // shutdown with charge
// New         	
        case SD_SLEEPRING       :	// sleep with GSM int standby
        case SD_STDBYRING       :	// standby with GSM int standby
        case SD_STDBYRINGCH     :	// standby mode with battery charge end GSM int standby
        case SD_SLEEPRINGCH     :	// turn off mode with battery charge end GSM int standby

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ DEBUG

            // Shutdown
#if defined(USE_AT91SAM7A3)
            AT91C_BASE_SHDWC->SHDWC_SHCR = 0xA5000001 ;      // shutdown
            DISABLE ;       // no more interrupts
            for( ; ; ) ;    // just t be sure
#endif // defined(USE_AT91SAM7A3)
#ifndef USE_LOW_POWER
            break ;
#endif // ifndef USE_LOW_POWER

#ifdef USE_LOW_POWER
        case SD_LOWPOWER :
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ DEBUG
#if defined(USE_AT91SAM7A3)
#error
            // disable serials
            dio_write(PORT_PIOB, PIOB_SRV_ON, 0) ;

            // Charge off
            dio_write(PORT_TW2, (1<<0), (1<<0)) ;
            // Ext alim off
            dio_write(PORT_TW1, (1<<3), (1<<3)) ;
#endif // defined(USE_AT91SAM7A3)

            runall = NO ;
            break ;
#endif // USE_LOW_POWER
        }
    }
}

// *********************************************************************
// code upgrade functions. temporal storage is SPI flash or internal RAM

//#ifdef USE_SPI_ON_ARM
#if ( defined(USE_SPI_ON_ARM) && defined(USE_SERIALFLASH_ON_ARM) )
int EKS_RamFill(void)
{
        KS_lockw(SPIPORT) ;         // we trust with
        SPI_FlashErase(CODEFLASH_START, CODEFLASH_STOP) ;
        KS_unlock(SPIPORT) ;        // free resource

        return(OK) ;                // end
}

int EKS_RamWrite(unsigned long pstart, unsigned char *buf, unsigned long flen)
{
int sz ;

	if ( (pstart+flen) > (CODEFLASH_STOP - CODEFLASH_START + 1) )
			return(ERROR) ;     // not enough memory

	while(flen) {
		sz = MIN(flen, sizeof(extapibuf)) ;

		KS_lockw(SPIPORT) ;         // we trust with
		KS_lockw(EXTAPI) ;          // we trust with
		SPI_FlashWrite(CODEFLASH_START + pstart, buf, extapibuf, sz) ;
		KS_unlock(EXTAPI) ;         // free resource
		KS_unlock(SPIPORT) ;        // free resource

		flen -= sz ;
		pstart += sz ;
		buf += sz ;
	}

	return(OK) ;                // end
}

int EKS_RamRead(unsigned long pstart, unsigned char *buf, unsigned long flen)
{
        if ( (pstart+flen) > (CODEFLASH_STOP-CODEFLASH_START))
                return(ERROR) ;         // not such a memory

        KS_lockw(SPIPORT) ;         // we trust with
        SPI_FlashRead(buf, CODEFLASH_START + pstart, flen) ;
        KS_unlock(SPIPORT) ;        // free resource

        return(OK) ;                // end
}

unsigned short EKS_RamCheckSum(void)
{
    unsigned short chk ;
    int i, locp, flen, plen ;

    flen = CODEFLASH_STOP - CODEFLASH_START + 1 ;
    locp = CODEFLASH_START ;
    chk = 0 ;
    while(flen) {
        // this turn len
        plen = MIN(sizeof(extapibuf), flen) ;

        KS_lockw(SPIPORT) ;         // we trust with
        KS_lockw(EXTAPI) ;          // we trust with

        SPI_FlashRead(extapibuf, locp, plen) ;

        for(i=0 ; i<plen ; i+=2) {
            chk ^= *((unsigned short *)(&extapibuf[i])) ;
        }

        flen -= plen ;
        locp += plen ;

        KS_unlock(EXTAPI) ;         // free resource
        KS_unlock(SPIPORT) ;        // free resource
    }

    return(chk) ;
}
#endif // ( defined(USE_SPI_ON_ARM) && defined(USE_SERIALFLASH_ON_ARM) )

// *********************************************************************
// Copy Flash code from Ram
// No longer available, use EKS_AskShutdown(SD_CODEUPGRADE)

#ifdef NOT_IMPLEMENTED
void EKS_FlashCodeFromRam(void)
{
        KS_lockw(SPIPORT) ;         // critical region
        //BootProgramCopy((unsigned long)(PROGRAMFLASH_START), (unsigned long)(CODEFLASH_START), (unsigned long)(PROGRAMFLASH_LEN)) ;
        KS_unlock(SPIPORT) ;        // free resource (never executed)
}
#endif // NOT_IMPLEMENTED


#ifndef CODEFLASH_START
// *********************************************************************

unsigned long ramcode[RAMCODESIZE] ;

int EKS_RamFill(void)
{
    memset(ramcode, -1, sizeof(ramcode)) ;
    return(0) ;
}

int EKS_RamWrite(unsigned long pstart, unsigned char *buf, unsigned long flen)
{
    if ((flen) && ((pstart + flen) <= sizeof(ramcode))) {
        memcpy(((unsigned char *)(ramcode)) + pstart, buf, flen) ;
    }
    return(0) ;
}

#endif // ndef CODEFLASH_START


// *********************************************************************
// Debug function

#define USE_TASKNAME

//void pdebugt(int debug_level, const char *format, ...)
//{
//  va_list ap;
//  va_start(ap, format);
// 
//  pdebugt(1,format, ap); //similar to printf
//  
//  va_end(ap);
//}

void pdebugt(int debug_level, const char *args, ...)
{
#ifdef CBUG
#ifdef USE_TASKNAME
    int task ;
    extern const char taskkname[][NAMMAX+1] ;
    extern TCB * hipritsk ;
    extern TCB rtxtcb[] ;
#endif // USE_TASKNAME

    struct PKTMEMBUF *pm ;
    int i ;
    va_list ap ;

#ifdef USE_TASKNAME
    // discover who I'm
    for(i=1 ; i<(ntasks+1) ; i++)
        if (hipritsk == &rtxtcb[i])
            break ;
    task = i ;
#endif // USE_TASKNAME

    // get actual time
    pm = EKS_PktBufFromTime(0L) ;

    // _FR_ change 'EXTAPI' into 'PCONE'
    KS_lockw(PCONE) ;
    KS_lockw(EXTAPI) ;

#ifdef USE_TASKNAME
    for(i=0 ; taskkname[task][i] ; i++) {
        putchar(taskkname[task][i]) ;
    }
    i = 9 - strlen(taskkname[task]) ;
    while((i--) > 0) {
        putchar(' ') ;
    }
#endif // USE_TASKNAME

    for(i=0 ; i<(pm->mtot) ; i++) {
        putchar(*((unsigned char *)(pm)+(pm->offs)+i)) ;
    }
//    printf((unsigned char *)(pm)+(pm->offs)) ;
    putchar(' ') ;

    EKS_PktBufRelease(pm) ;     // free memory

    va_start(ap, args);
    vsnprintf((char *)(extapibuf), sizeof(extapibuf), args, ap) ;
    va_end(ap);

    for(i=0 ; (extapibuf[i]) && (i<sizeof(extapibuf)) ; i++)
        putchar(extapibuf[i]) ;

    putchar('\n') ;

    // _FR_ change 'EXTAPI' into 'PCONE'
    KS_unlock(EXTAPI) ;
    KS_unlock(PCONE) ;
#endif // CBUG
}

void panic(int step)
{
#ifndef CBUG
    printf("PANIC %d", step) ;	// FR-CHECK
    KS_delay(SELFTASK, ((TICKS)(500)/CLKTICK)) ;
#endif // CBUG

#ifdef USE_TRACE
	if (par_trace) AppendTrace(TRACE_VER, TR_PANIC, "h", step ) ; // "PANIC %d"
#endif // #ifdef USE_TRACE

#ifdef CBUG
    pdebugt(1,"PANIC %d", step) ;
    KS_delay(SELFTASK, ((TICKS)(500)/CLKTICK)) ;
	KS_signal(CBUGSEMA) ;       // wake up debugger now    
#endif // CBUG

    DISABLE ;
    for( ; ; ) ;        // watchdog will restart
}

// *********************************************************************
// Virtual RAM functions

#if defined(USE_VIRTUALRAM_ON_FLASH)

static unsigned long vramptr ;

struct _VRAM_HEADER {
unsigned short dsize ;          // data real size
unsigned char pchk ;            // positive checksum
unsigned char nchk ;            // negated checksum
} __attribute__ ((packed)) ;

void EKS_VirtualRAM_Read(void * ptr, int len)		// USE_SPI_ON_ARM
{
    unsigned long faddr ;
    unsigned char * p ;
    struct _VRAM_HEADER vram_header ;
    unsigned char chk ;
    int retry ;
    
    KS_lockw(SPIPORT) ; // we trust with

    for(retry=0 ; retry<2 ; retry++) {  // If 2015 can be blocked when writing data
#if defined(MTS_CODE) && defined(CBUG)
        if (par71) pdebugt(1,"Start VRAM: 0x%08lx", vramptr) ;
#endif // defined(MTS_CODE) && defined(CBUG)

        // if never used, find it
        if ((vramptr < VIRTUALRAM_START) || (vramptr > VIRTUALRAM_STOP)) {
            for(faddr=VIRTUALRAM_START ; faddr<VIRTUALRAM_STOP ; faddr+=VIRTUALRAM_SSIZE ) {       // find tail
                SPI_FlashRead((unsigned char *)(&vram_header), faddr, sizeof(vram_header)) ;
	
                if (vram_header.dsize == 0xffff)
                    continue ;                      // continue vram scan
	
                if (vram_header.dsize <= len) {     // found (changed FR 15/10/08) old == len
                    len = vram_header.dsize ;		// (added FR 15/10/08)
                    vramptr = faddr ;			
#if defined(MTS_CODE) && defined(CBUG)
                    if (par71) pdebugt(1,"VRAM: 0x%08lx", vramptr) ;
#endif // defined(MTS_CODE) && defined(CBUG)
                    break ;
                }

#if defined(MTS_CODE_) && defined(CBUG)
                if (par71) pdebugt(1,"BAD VRAM: 0x%08lx, %d, 0x%x, 0x%x", vramptr, len, vram_header.pchk, vram_header.nchk ) ;
#endif // defined(MTS_CODE_) && defined(CBUG)
                // if here we are, something went wrong, erase sector
                SPI_FlashErase(faddr, faddr + VIRTUALRAM_SSIZE - 1) ;
            }
        }
	
        if (vramptr) {              // some data?
            // read this sector
            SPI_FlashRead(ptr, vramptr + sizeof(vram_header), len) ;
            // checksum
            chk = 0 ;
            p = (unsigned char *)(ptr) ;
            for(faddr=0 ; faddr<len ; faddr++) {
                chk ^= (*p++) ;
            }
            if ((chk != vram_header.pchk) || ((chk ^ 0xff) != vram_header.nchk)) {
#if defined(CBUG)
                pdebugt(1,"VRAM wrong checksum: 0x%08lx, 0x%x, 0x%x, 0x%x (%d)", vramptr, chk, vram_header.pchk, vram_header.nchk, len) ;
#endif // defined(CBUG)
                SPI_FlashErase(vramptr, vramptr + VIRTUALRAM_SSIZE - 1) ;
                memset(ptr, 0 , len) ;      // reset to known values
                if (!retry) vramptr = 0L ;
            } else {
                break ;
            }
        } else {
#if defined(CBUG)
            pdebugt(1,"VRAM: use defaults") ;
#endif // defined(CBUG)
            // use defaults
            memset(ptr, 0 , len) ;          // reset to known values
            vramptr = VIRTUALRAM_START ;
        }
    }
	
    KS_unlock(SPIPORT) ;
}

void EKS_VirtualRAM_Write(void * ptr, int len)		 // USE_SPI_ON_ARM
{
    unsigned long faddr, newaddr ;
    unsigned char * p ;
    struct _VRAM_HEADER vram_header ;
    struct PKTMEMBUF *pmd, *pmdummy;
    int ll ;
    unsigned char chk ;

    if ((vramptr < VIRTUALRAM_START) || (vramptr > VIRTUALRAM_STOP))
        return ;        // perform read before

    // alloc temporary
    if ((pmd = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL) {
        panic(0) ;
    }
    pmdummy = pmd ;
    
    KS_lockw(SPIPORT) ; // we trust with

    // Calculate address
    faddr = vramptr + VIRTUALRAM_SSIZE ;
    if (faddr > VIRTUALRAM_STOP)
        faddr = VIRTUALRAM_START ;
    newaddr = faddr ;
    
    // before check if Flash is erased
    // read this sector
    SPI_FlashRead((unsigned char *)(&vram_header), faddr, sizeof(vram_header)) ;
	// Verify data
	if (vram_header.dsize != 0xffff){	// If with bad data erase it
        SPI_FlashErase(faddr, faddr + VIRTUALRAM_SSIZE - 1) ;
	}

    // evaluate checksum and size
    vram_header.dsize = len ;
    chk = 0 ;
    p = (unsigned char *)(ptr) ;
    for(ll=0 ; ll<len ; ll++) {
        chk ^= (*p++) ;
    }
    vram_header.pchk = chk ;
    vram_header.nchk = (~chk) ;
#ifdef CBUG_
    pdebugt(1,"VRAM checksum: 0x%08lx, 0x%x, 0x%x (%d,%ld)", vramptr, vram_header.pchk, vram_header.nchk, len, msize) ;
#endif // CBUG

    
    // Now write data
    // header
    SPI_FlashWrite(faddr, (unsigned char *)(&vram_header), (unsigned char *)(pmdummy), sizeof(vram_header)) ;
    // struct
    faddr += sizeof(vram_header) ;
    while(len) {
        ll = MIN(len, msize) ;
        SPI_FlashWrite(faddr, ptr, (unsigned char *)(pmdummy), ll) ;
        ptr += ll ;
        len -= ll ;
        faddr += ll ;
    }

    // erase previous
    SPI_FlashErase(vramptr, vramptr + VIRTUALRAM_SSIZE - 1) ;

    // new pointer
    vramptr = newaddr ;
    
    KS_unlock(SPIPORT) ;

    // release temporary
    KS_free(PKTBUF, pmd) ;

#ifdef CBUG_
    pdebugt(1,"VRAM write at 0x%08lx", newaddr) ;
#endif // CBUG
}
#endif // defined(USE_VIRTUALRAM_ON_FLASH)


#if defined(USE_VIRTUALRAM_ON_EEPROM)

static unsigned long vramptr  ;// = VIRTUALRAM_STOP+EEPROM_PAGE_SIZE ;

struct _VRAM_HEADER {
unsigned short dsize ;          // data real size
unsigned char pchk ;            // positive checksum
unsigned char nchk ;            // negated checksum
} __attribute__ ((packed)) ;

// Search data ( 2 data write space, if wroted 1 data struct clean  other 512 byte space for next write)
void EKS_VirtualRAM_Read(void * ptr, int len)		// USE_VIRTUALRAM_ON_EEPROM
{
unsigned long faddr ;
unsigned char * p ;
struct _VRAM_HEADER vram_header ;
unsigned char chk ;
int i; // retry ;
    
	KS_lockw(EEPROMPORT) ; // we trust with
	faddr = vramptr ;
	
//	for(retry=0 ; retry<2 ; retry++) {  // If blocked when writing data
#if defined(MTS_CODE) && defined(CBUG)
		if (par71) pdebugt(1,"Start VRAM: 0x%08lx", vramptr) ;
#endif // defined(MTS_CODE) && defined(CBUG)

		// if never used, find it
		if ((vramptr < VIRTUALRAM_START) || (vramptr > VIRTUALRAM_STOP)) {
#if defined(MTS_CODE_) && defined(CBUG)
					if (par71) pdebugt(1,"check1 VRAM 0x%08lx %d<=%d", faddr,vram_header.dsize,len) ;
#endif // defined(MTS_CODE) && defined(CBUG)
			for(faddr=VIRTUALRAM_START ; faddr<VIRTUALRAM_STOP ; faddr+=VIRTUALRAM_SSIZE ) {       // find tail
						// offset in page, page
				EEPROM_Read(EEPROM_PAGE_OFFSET(faddr), (faddr/EEPROM_PAGE_SIZE), (void*) (&vram_header), MODE_32_BIT, 1 ) ;

#if defined(MTS_CODE) && defined(CBUG)
					if (par71) pdebugt(1,"check VRAM 0x%08lx %d<=%d", faddr,vram_header.dsize,len) ;
#endif // defined(MTS_CODE) && defined(CBUG)
				if ((vram_header.dsize == 0xffff) || (vram_header.dsize == 0x0000))
					continue ;                      // continue vram scan

				if (vram_header.dsize <= len) {     // found (changed FR 15/10/08) old == len
					len = vram_header.dsize ;		// (added FR 15/10/08)
					vramptr = faddr ;
					faddr += 4 ;
#if defined(MTS_CODE) && defined(CBUG)
					if (par71) pdebugt(1,"VRAM: 0x%08lx", vramptr) ;
#endif // defined(MTS_CODE) && defined(CBUG)

					// read this sector
					EEPROM_Read(EEPROM_PAGE_OFFSET(faddr), (faddr/EEPROM_PAGE_SIZE), ptr, MODE_32_BIT, ((VIRTUALRAM_SSIZE-4)/4) ) ;
					// checksum
					chk = 0 ;
					p = (unsigned char *)(ptr) ;
					for(faddr=0 ; faddr<len ; faddr++) {
#ifdef CBUG_
						printf("\n%ld %02x %02x\n",faddr, *p, chk) ;
#endif
						chk ^= (*p++) ;
					}
					if ((chk == vram_header.pchk) && ((chk ^ 0xff) == vram_header.nchk)) {
#if defined(CBUG)
							pdebugt(1,"VRAM OK checksum: 0x%08lx, 0x%x, 0x%x, 0x%x (%d,%d)", vramptr, chk, vram_header.pchk, 
													vram_header.nchk, vram_header.dsize,len) ;
#endif // defined(CBUG)
						KS_unlock(EEPROMPORT) ;
						return ;
						//break ;
				//if (!retry) vramptr = 0L ;
// 			} else {
// 				break ;
					}
				}

#if defined(MTS_CODE_) && defined(CBUG)
				if (par71) pdebugt(1,"BAD VRAM: 0x%08lx, %d, 0x%x, 0x%x", vramptr, len, vram_header.pchk, vram_header.nchk ) ;
#endif // defined(MTS_CODE_) && defined(CBUG)
				// if here we are, something went wrong, erase sector
				for(i=0;i<VIRTUALRAM_SSIZE;i+=EEPROM_PAGE_SIZE) EEPROM_Erase( ((faddr+i)/EEPROM_PAGE_SIZE) ) ;
				//SPI_FlashErase(faddr, faddr + VIRTUALRAM_SSIZE - 1) ;
			}
		}else{
			EEPROM_Read(EEPROM_PAGE_OFFSET(faddr), (faddr/EEPROM_PAGE_SIZE), (void*) (&vram_header), MODE_32_BIT, 1 ) ;
			faddr += 4 ;
		}

		if (vramptr!=(VIRTUALRAM_STOP+EEPROM_PAGE_SIZE)) {              // some data?
			// read this sector
			EEPROM_Read(EEPROM_PAGE_OFFSET(faddr), (faddr/EEPROM_PAGE_SIZE), ptr, MODE_32_BIT, ((VIRTUALRAM_SSIZE-4)/4) ) ;
			// checksum
			chk = 0 ;
			p = (unsigned char *)(ptr) ;
			for(faddr=0 ; faddr<len ; faddr++) {
#ifdef CBUG_
		printf("\n%ld %02x %02x\n",faddr, *p, chk) ;
#endif
				chk ^= (*p++) ;
			}
			if ((chk != vram_header.pchk) || ((chk ^ 0xff) != vram_header.nchk)) {
#if defined(CBUG)
				pdebugt(1,"VRAM wrong checksum: 0x%08lx, 0x%x, 0x%x, 0x%x (%d,%d)", vramptr, chk, vram_header.pchk, 
												vram_header.nchk, vram_header.dsize,len) ;
#endif // defined(CBUG)
				for(i=0;i<VIRTUALRAM_SSIZE;i+=EEPROM_PAGE_SIZE)
					EEPROM_Erase( ((vramptr+i)/EEPROM_PAGE_SIZE) ) ;
				memset(ptr, 0 , len) ;      // reset to known values
				//if (!retry) vramptr = 0L ;
// 			} else {
// 				break ;
			}
		} else {
#if defined(CBUG)
			pdebugt(1,"VRAM: use defaults") ;
#endif // defined(CBUG)
			// use defaults
			memset(ptr, 0 , len) ;          // reset to known values
			vramptr = VIRTUALRAM_START ;
		}
//	}

	KS_unlock(EEPROMPORT) ;
}

void EKS_VirtualRAM_Write(void * ptr, int len)		// USE_VIRTUALRAM_ON_EEPROM
{
unsigned long faddr, newaddr ;
unsigned char * p ;
struct _VRAM_HEADER vram_header ;
struct PKTMEMBUF *pmd ; // , *pmdummy;
int ll ;
unsigned char chk ;

#ifdef CBUG_
			pdebugt(1,"VRAM write1 0x%08lx", vramptr) ;
#endif // defined(CBUG)

	if ((vramptr < VIRTUALRAM_START) || (vramptr > VIRTUALRAM_STOP))
		return ;        // perform read before

#ifdef CBUG_
			pdebugt(1,"VRAM write2 0x%08lx", vramptr) ;
#endif // defined(CBUG)
	// alloc temporary
	if ((pmd = ((struct PKTMEMBUF *)(KS_alloc(PKTBUF)))) == MNULL)  panic(0) ;
//	pmdummy = pmd ;

	KS_lockw(EEPROMPORT) ; // we trust with

	// Calculate address
	faddr = vramptr + VIRTUALRAM_SSIZE ;
	if (faddr > VIRTUALRAM_STOP)
		faddr = VIRTUALRAM_START ;
	newaddr = faddr ;

	// before check if Flash is erased
	// read this sector
			// offset in page, page
	EEPROM_Read(EEPROM_PAGE_OFFSET(faddr), (faddr/EEPROM_PAGE_SIZE), (void*) (&vram_header), MODE_32_BIT, 1 ) ;
	// Verify data
	if (vram_header.dsize){	// If with bad data erase it
		for(ll=0;ll<VIRTUALRAM_SSIZE;ll+=EEPROM_PAGE_SIZE) EEPROM_Erase( ((faddr+ll)/EEPROM_PAGE_SIZE) ) ;
	}

	// evaluate checksum and size
	vram_header.dsize = len ;
	chk = 0 ;
	p = (unsigned char *)(ptr) ;
	for(ll=0 ; ll<len ; ll++) {
#ifdef CBUG_
		printf("\n%d %02x %02x\n",ll, *p, chk) ;
#endif
		chk ^= (*p++) ;
	}
	vram_header.pchk = chk ;
	vram_header.nchk = (~chk) ;
#ifdef CBUG_
	pdebugt(1,"VRAM checksum: 0x%08lx, 0x%x, 0x%x (%d,%ld)", faddr, vram_header.pchk, vram_header.nchk, len, msize) ;
#endif // CBUG

    
	// Now write data
	// header
	EEPROM_Write(EEPROM_PAGE_OFFSET(faddr), (faddr/EEPROM_PAGE_SIZE), (void*) (&vram_header), MODE_32_BIT, 1 ) ;
	// struct
	faddr += sizeof(vram_header) ;
	EEPROM_Write(EEPROM_PAGE_OFFSET(faddr), (faddr/EEPROM_PAGE_SIZE), (void*) (ptr), MODE_32_BIT, ((VIRTUALRAM_SSIZE-4)/4) ) ;

	// erase previous
	for(ll=0;ll<VIRTUALRAM_SSIZE;ll+=EEPROM_PAGE_SIZE) EEPROM_Erase( ((vramptr+ll)/EEPROM_PAGE_SIZE) ) ;

	// new pointer
	vramptr = newaddr ;

	KS_unlock(EEPROMPORT) ;

	// release temporary
	KS_free(PKTBUF, pmd) ;

#ifdef CBUG_
	pdebugt(1,"VRAM wroted at 0x%08lx", newaddr) ;
#endif // CBUG
}
#endif // defined(USE_VIRTUALRAM_ON_EEPROM)

// *********************************************************************

void EKS_init(unsigned char numtasks)
{
	msize = KS_inqmap(PKTBUF) ; // constant

	NumOfTasks = numtasks ;     // set number of LU
#if defined(USE_VIRTUALRAM_ON_FLASH)
	vramptr = 0L ;
#endif    
#if defined(USE_VIRTUALRAM_ON_EEPROM)
	vramptr = VIRTUALRAM_STOP+EEPROM_PAGE_SIZE ;
#endif
}
