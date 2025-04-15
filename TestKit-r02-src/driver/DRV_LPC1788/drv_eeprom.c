// drv_eeprom.c - eeprom use

//
//   Copyright (c) 1997-2011.
//   T.E.S.T. srl
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

//----------------------------------------------------------------------------
// only if we are well accepted

#ifdef USE_EEPROM_ON_LPC1788


//#define USE_EEPROM_DEBUG

#ifdef CBUG /* { */
extern char cbugflag ;
extern unsigned short par71 ;
#endif /* } */

// Macro defines for EEPROM address register

// Moved into 'assign.h'
// #define EEPROM_PAGE_OFFSET(n)           (n & 0x3F)
// #define EEPROM_PAGE_ADRESS(n)           ((n & 0x3F) << 6)
// 
// #define EEPROM_PAGE_SIZE                64      /* EEPROM byes per page */
// #define EEPROM_PAGE_NUM                 63      /*  EEPROM pages */
// 
// #define MODE_8_BIT      8
// #define MODE_16_BIT    16
// #define MODE_32_BIT    32

#define EEPROM_CMD_8_BIT_READ           (0)     /* EEPROM 8-bit read command */
#define EEPROM_CMD_16_BIT_READ          (1)     /* EEPROM 16-bit read command */
#define EEPROM_CMD_32_BIT_READ          (2)     /* EEPROM 32-bit read command */
#define EEPROM_CMD_8_BIT_WRITE          (3)     /* EEPROM 8-bit write command */
#define EEPROM_CMD_16_BIT_WRITE         (4)     /* EEPROM 16-bit write command */
#define EEPROM_CMD_32_BIT_WRITE         (5)     /* EEPROM 32-bit write command */
#define EEPROM_CMD_ERASE_PRG_PAGE       (6)     /* EEPROM erase command */

#define EEPROM_CMD_RDPREFETCH           (1 << 3)/* EEPROM read pre-fetch enable */

// Macro defines for EEPROM write data register

#define EEPROM_WDATA_8_BIT(n)           (n & 0x000000FF)
#define EEPROM_WDATA_16_BIT(n)          (n & 0x0000FFFF)
#define EEPROM_WDATA_32_BIT(n)          (n & 0xFFFFFFFF)

// Macro defines for EEPROM read data register

#define EEPROM_RDATA_8_BIT(n)           (n & 0x000000FF)
#define EEPROM_RDATA_16_BIT(n)          (n & 0x0000FFFF)
#define EEPROM_RDATA_32_BIT(n)          (n & 0xFFFFFFFF)

// Macro defines for EEPROM power down register

#define EEPROM_PWRDWN                   (1 << 0)
#define EEPROM_ENDOF_RW                 (26)
#define EEPROM_ENDOF_PROG               (28)

//----------------------------------------------------------------------------
// EEPROM initializer

void eepromstart(void)
{
    uint32_t val ;

    // Disable EEPROM power down mode
    LPC_EEPROM->PWRDWN = 0x0 ;
//    LPC_EEPROM->PWRDWN = 0x1 ;

    // Set EEPROM clock to the required 375KHz rate
    LPC_EEPROM->CLKDIV = (current_clock / 375000) - 1 ;

    // Setup EEPROM wait states to 15, 55, 35 ns
    val  =  ((((current_clock / 1000000) * 15) / 1000) + 1) ;
    val |= (((((current_clock / 1000000) * 55) / 1000) + 1) << 8) ;
    val |= (((((current_clock / 1000000) * 35) / 1000) + 1) << 16) ;
    LPC_EEPROM->WSTATE = val ;
	
//     // Disable EEPROM power down mode
//     LPC_EEPROM->PWRDWN = 0x0 ;

}

//----------------------------------------------------------------------------
// EEPROM terminator

void eepromstop(void)
{
    // Enable EEPROM power down mode
    LPC_EEPROM->PWRDWN = 0x1 ;
}

//----------------------------------------------------------------------------
// EEPROM Write data specific address

int EEPROM_Write(uint16_t page_offset, uint16_t page_nr, void *data,
                 int mode,  /* 8, 16 or 32 */
                 uint32_t count)
{
    uint32_t i;
    uint8_t *tmp8 = (uint8_t *) data;
    uint16_t *tmp16 = (uint16_t *) data;
    uint32_t *tmp32 = (uint32_t *) data;

#ifdef USE_EEPROM_DEBUG
	if (cbugflag)
	printf("EEPROM write %ld at 0x%x (%d mode %d)\n", count, EEPROM_PAGE_ADRESS(page_nr) | EEPROM_PAGE_OFFSET(page_offset),
		   			page_nr, mode) ;
#endif

    LPC_EEPROM->INT_CLR_STATUS = ((1 << EEPROM_ENDOF_RW) | (1 << EEPROM_ENDOF_PROG));

    /* check page_offset */
    if (mode == MODE_16_BIT) {
        if ((page_offset & 0x01) != 0) {
            return(ERROR) ;
        }
    }
    else if (mode == MODE_32_BIT) {
        if ((page_offset & 0x03) != 0) {
            return(ERROR) ;
        }
    }
    LPC_EEPROM->ADDR = EEPROM_PAGE_OFFSET(page_offset);
    for (i = 0; i < count; i++) {
        /* update data to page register */
        if (mode == MODE_8_BIT) {
            LPC_EEPROM->CMD = EEPROM_CMD_8_BIT_WRITE;
            LPC_EEPROM->WDATA = *tmp8;
            tmp8++;
            page_offset += 1;
        }
        else if (mode == MODE_16_BIT) {
            LPC_EEPROM->CMD = EEPROM_CMD_16_BIT_WRITE;
            LPC_EEPROM->WDATA = *tmp16;
#ifdef USE_EEPROM_DEBUG
	if (cbugflag)
	printf("EEPROM writing%ld at 0x%04x\n ", i, *tmp16 ) ;
#endif
            tmp16++;
            page_offset += 2;
        }
        else {
            LPC_EEPROM->CMD = EEPROM_CMD_32_BIT_WRITE;
            LPC_EEPROM->WDATA = *tmp32;
            tmp32++;
            page_offset += 4;
        }
        while (!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_RW) & 0x01)) {}
        LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_RW);
        if ((page_offset >= EEPROM_PAGE_SIZE) | (i == count - 1)) {
            /* update to EEPROM memory */
            LPC_EEPROM->INT_CLR_STATUS = (0x1 << EEPROM_ENDOF_PROG);
            LPC_EEPROM->ADDR = EEPROM_PAGE_ADRESS(page_nr);
            LPC_EEPROM->CMD = EEPROM_CMD_ERASE_PRG_PAGE;
            while (!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_PROG) & 0x01)) {}
            LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_PROG);
        }
        if (page_offset >= EEPROM_PAGE_SIZE) {
            page_offset = 0;
            page_nr += 1;
            LPC_EEPROM->ADDR = 0;
            if (page_nr > EEPROM_PAGE_NUM - 1) {
                page_nr = 0;
            }
        }
    }
    return(OK) ;
}

//----------------------------------------------------------------------------
// EEPROM Read data at specific address

void EEPROM_Read(uint16_t page_offset, uint16_t page_nr,void *data,
                 int mode,  /* 8, 16 or 32 */
                 uint32_t count)
{
    uint32_t i;
    uint8_t *tmp8 = (uint8_t *) data;
    uint16_t *tmp16 = (uint16_t *) data;
    uint32_t *tmp32 = (uint32_t *) data;

    LPC_EEPROM->INT_CLR_STATUS = ((1 << EEPROM_ENDOF_RW) | (1 << EEPROM_ENDOF_PROG));
    LPC_EEPROM->ADDR = EEPROM_PAGE_ADRESS(page_nr) | EEPROM_PAGE_OFFSET(page_offset);
#ifdef USE_EEPROM_DEBUG_
	if (cbugflag)
	printf("EEPROM read %ld at 0x%x (%d mode %d) ", count, EEPROM_PAGE_ADRESS(page_nr) | EEPROM_PAGE_OFFSET(page_offset),
		   			page_nr, mode) ;
#endif
	if (mode == MODE_8_BIT) {
        LPC_EEPROM->CMD = EEPROM_CMD_8_BIT_READ | EEPROM_CMD_RDPREFETCH;
    }
    else if (mode == MODE_16_BIT) {
        LPC_EEPROM->CMD = EEPROM_CMD_16_BIT_READ | EEPROM_CMD_RDPREFETCH;
        /* check page_offset */
        if ((page_offset & 0x01) != 0) {
            return ;
        }
    }
    else {
        LPC_EEPROM->CMD = EEPROM_CMD_32_BIT_READ | EEPROM_CMD_RDPREFETCH;
        /* page_offsetUSE_EEPROM_DEBUG must be a multiple of 0x04 */
        if ((page_offset & 0x03) != 0) {
            return ;
        }
    }

    /* read and store data in buffer */
    for (i = 0; i < count; i++) {

        if (mode == MODE_8_BIT) {
            *tmp8 = (uint8_t) (LPC_EEPROM->RDATA);
#ifdef USE_EEPROM_DEBUG_
	if (cbugflag) printf("val8:%02x\n", *tmp8 ) ;
#endif
            tmp8++;
            page_offset += 1;
        }
        else if (mode == MODE_16_BIT) {
            *tmp16 =  (uint16_t) (LPC_EEPROM->RDATA);
#ifdef USE_EEPROM_DEBUG_
	if (cbugflag) printf("val16:%04x\n", *tmp16 ) ;
#endif
            tmp16++;
            page_offset += 2;
        }
        else {
            *tmp32 = (uint32_t) (LPC_EEPROM->RDATA);
#ifdef USE_EEPROM_DEBUG_
	if (cbugflag) printf("val32:%08lx\n", *tmp32 ) ;
#endif
            tmp32++;
            page_offset += 4;
        }
        while (!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_RW) & 0x01)) {}
        LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_RW);
        if ((page_offset >= EEPROM_PAGE_SIZE) && (i < count - 1)) {
            page_offset = 0;
            page_nr++;
            LPC_EEPROM->ADDR = EEPROM_PAGE_ADRESS(page_nr) | EEPROM_PAGE_OFFSET(page_offset);
            if (mode == MODE_8_BIT) {
                LPC_EEPROM->CMD = EEPROM_CMD_8_BIT_READ | EEPROM_CMD_RDPREFETCH;
            }
            else if (mode == MODE_16_BIT) {
                LPC_EEPROM->CMD = EEPROM_CMD_16_BIT_READ | EEPROM_CMD_RDPREFETCH;
            }
            else {
                LPC_EEPROM->CMD = EEPROM_CMD_32_BIT_READ | EEPROM_CMD_RDPREFETCH;
            }
        }
    }
}

//----------------------------------------------------------------------------
// EEPROM Erase a page at the specific address

void EEPROM_Erase(uint16_t page_nr)
{
    uint32_t i;
    uint32_t count = EEPROM_PAGE_SIZE / 4 ;

    LPC_EEPROM->INT_CLR_STATUS = ((1 << EEPROM_ENDOF_RW) | (1 << EEPROM_ENDOF_PROG));

#ifdef USE_EEPROM_DEBUG
	printf("EEPROM Erase page %d\n", page_nr ) ;
#endif
    /* clear page register */
    LPC_EEPROM->ADDR = EEPROM_PAGE_OFFSET(0);
    LPC_EEPROM->CMD = EEPROM_CMD_32_BIT_WRITE;
    for (i = 0; i < count; i++) {
#ifdef USE_EEPROM_DEBUG_
	if (cbugflag) printf("EEPROM Erase page %d (count=%ld)\n", page_nr, i ) ;
#endif
        LPC_EEPROM->WDATA = 0;
        while (!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_RW) & 0x01)) {}
        LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_RW);
    }

    LPC_EEPROM->INT_CLR_STATUS = (0x1 << EEPROM_ENDOF_PROG);
    LPC_EEPROM->ADDR = EEPROM_PAGE_ADRESS(page_nr);
    LPC_EEPROM->CMD = EEPROM_CMD_ERASE_PRG_PAGE;
    while (!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_PROG) & 0x01)) {}
    LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_PROG);
}

#endif // USE_EEPROM_ON_LPC1788
