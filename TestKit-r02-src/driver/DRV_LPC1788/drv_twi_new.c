// drv_twi.c - TWI driver tasks

//
//   Copyright (c) 1997-2011.
//   T.E.S.T. srl
//

//
// This module is provided as a TWI port driver.
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
#ifdef USE_TWI_ON_ARM

#define NULLSEMA ((SEMA)0)

#define WAIT_USEC       150

#define RD_BIT          0x01

#define I2CONSET_I2EN   0x00000040      // I2C Control Set Register
#define I2CONSET_AA     0x00000004
#define I2CONSET_SI     0x00000008
#define I2CONSET_STO    0x00000010
#define I2CONSET_STA    0x00000020

#define I2CONCLR_AAC    0x00000004      // I2C Control clear Register
#define I2CONCLR_SIC    0x00000008
#define I2CONCLR_STAC   0x00000020
#define I2CONCLR_I2ENC  0x00000040

#ifdef CBUG

#if defined(MTS_CODE)
extern unsigned short par71 ;
#endif

#undef TWI_REAL_DEBUG

#ifdef TWI_REAL_DEBUG
//#define PUT_HEX(B) {PUT_CHAR('_');PUT_CHAR((((B)>>4)&0xf)+'0');PUT_CHAR(((B)&0xf)+'0');PUT_CHAR('_');}
#define PUT_HEX(B)      {}
#define PUT_DEBUG(B) {PUT_CHAR('_');PUT_CHAR(B);PUT_CHAR('_');}
#define PUT_CHAR(A) {while(!((LPC_UART0->LSR) & 0x20));LPC_UART0->THR=(A);}
#else // TWI_REAL_DEBUG
#define PUT_HEX(B)      {}
#define PUT_DEBUG(B)    {}
#define PUT_CHAR(A)     {}
#endif // TWI_REAL_DEBUG
#else // CBUG
#define PUT_HEX(B)      {}
#define PUT_DEBUG(B)    {}
#define PUT_CHAR(A)     {}

#endif // CBUG

//#define PUT_DEBUG2(B)    {PUT_CHAR2('_');PUT_CHAR2(B);PUT_CHAR2('_');}
//#define PUT_CHAR2(A) {while(!((LPC_UART0->LSR) & 0x20));LPC_UART0->THR=(A);}
#define PUT_DEBUG2(B)    {}
#define PUT_CHAR2(A) {}

//----------------------------------------------------------------------------
// local data

volatile unsigned char twi0addr ;               // TWI slave address + RW
volatile unsigned short twi0suba ;              // TWI slave subaddress (keep also flag if byte=0)
volatile unsigned char twi0askrestart ;         // TWI restart request
volatile unsigned char * twi0bufptr ;           // TWI buffer pointer
volatile int twi0buflen ;                       // TWI buffer len

unsigned short twi0err = 0 ;

#ifdef USE_TWI1_ON_ARM
volatile unsigned char twi1addr ;               // TWI slave address + RW
volatile unsigned short twi1suba ;              // TWI slave subaddress (keep also flag if byte=0)
volatile unsigned char twi1askrestart ;         // TWI restart request
volatile unsigned char * twi1bufptr ;           // TWI buffer pointer
volatile int twi1buflen ;                       // TWI buffer len

unsigned short twi1err = 0 ;
#endif // USE_TWI1_ON_ARM

#ifdef USE_TWI2_ON_ARM
volatile unsigned char twi2addr ;               // TWI slave address + RW
volatile unsigned short twi2suba ;              // TWI slave subaddress (keep also flag if byte=0)
volatile unsigned char twi2askrestart ;         // TWI restart request
volatile unsigned char * twi2bufptr ;           // TWI buffer pointer
volatile int twi2buflen ;                       // TWI buffer len
volatile unsigned char * twi2rxptr ;           // TWI buffer pointer
volatile int twi2rxflen ;                       // TWI buffer len

unsigned short twi2err = 0 ;
#endif // USE_TWI2_ON_ARM

#if defined(USE_TWI0_SLAVE) || defined(USE_TWI1_SLAVE) || defined(USE_TWI2_SLAVE)
struct _SLAVEBUF {
uint8_t * sbuffer ;
int sbufcnt ;
volatile int * spointer ;
} ;
#endif // defined(USE_TWI0_SLAVE) || defined(USE_TWI1_SLAVE) || defined(USE_TWI2_SLAVE)
#if defined(USE_TWI0_SLAVE)
volatile struct _SLAVEBUF slave0 ;
#endif // defined(USE_TWI0_SLAVE)
#if defined(USE_TWI1_SLAVE)
volatile struct _SLAVEBUF slave1 ;
#endif // defined(USE_TWI1_SLAVE)
#if defined(USE_TWI2_SLAVE)
volatile struct _SLAVEBUF slave2 ;
#endif // defined(USE_TWI2_SLAVE)

//----------------------------------------------------------------------------
// internal functions

void twistart(void) ;
void twistop(void) ;

int TWI_send(int dev, unsigned char *buf, int len) ;
int TWI_txrx(int dev, int sub, unsigned char *buf, int len) ;

#ifdef USE_TWI1_ON_ARM
int TWI1_send(int dev, unsigned char *buf, int len) ;
int TWI1_txrx(int dev, int sub, unsigned char *buf, int len) ;
#endif // USE_TWI1_ON_ARM

#ifdef USE_TWI2_ON_ARM
int TWI2_send(int dev, unsigned char *buf, int len) ;
int TWI2_txrx(int dev, int sub, unsigned char *buf, int len) ;
int TWI2_sendrx(int dev, unsigned char *buf, int len, unsigned char *bufrx, int lenrx) ;
#endif // USE_TWI2_ON_ARM

//----------------------------------------------------------------------------
// Interrupt routine for TWI

void I2C0_IRQHandler(void)
{
    uint8_t StatValue ;
    LPC_I2C_TypeDef *I2Cx = LPC_I2C0 ;

    // this handler deals with master read and master write only
    StatValue = I2Cx->STAT ;

    switch(StatValue) {
    case 0x08 :                 // A Start condition is issued.
        PUT_DEBUG('1');
        // write SlaveAddress with R=1/W=0 bit
        I2Cx->DAT = twi0addr ;
        I2Cx->CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    case 0x10 :                 // A repeated started is issued
        PUT_DEBUG('2');
        // write SlaveAddress with R=1 bit: surely a Read
        I2Cx->DAT = twi0addr | RD_BIT ;
        I2Cx->CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    //case 0x20 :                 // Master TX: SLAVEADDR+W sent, noACK
    case 0x18 :                 // Master TX: SLAVEADDR+W sent, ACK
        PUT_DEBUG('3');
    case 0x28 :                 // Master TX: DATA sent
        PUT_DEBUG('4');
        if (twi0askrestart) {
            PUT_DEBUG('a');
            I2Cx->CONSET = I2CONSET_STA ;     // Set Repeated-start flag
        } else if (twi0suba) {
            PUT_DEBUG('b');
            I2Cx->DAT = twi0suba ;
            twi0suba = 0 ;                       // handled
            twi0askrestart = 1 ;                 // set restart flag
        } else if (twi0buflen) {
            PUT_DEBUG('c');
            I2Cx->DAT = *twi0bufptr++ ;
            twi0buflen-- ;
        } else {
            PUT_DEBUG('d');
            I2Cx->CONSET = I2CONSET_STO ;     // Set Stop flag
            KS_ISRsignal(TWI0SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;

    //case 0x48 :                 // Master RX: SLAVEADDR+R sent, noACK
    case 0x40 :                 // Master RX: SLAVEADDR+R sent, ACK
        PUT_DEBUG('5');
        I2Cx->CONSET = I2CONSET_AA ;          // assert ACK after data is received
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break;

    case 0x58 :                 // Master RX: DATA received + NACK
        PUT_DEBUG('6');
    case 0x50 :                 // Master RX: DATA received + ACK
        PUT_DEBUG('7');
        if (twi0buflen) {
            PUT_DEBUG('a');
            *twi0bufptr++ = I2Cx->DAT ;
            twi0buflen-- ;
            if (twi0buflen != 0) {
                PUT_DEBUG('b');
                I2Cx->CONSET = I2CONSET_AA ;  // assert ACK after data is received
                I2Cx->CONCLR = I2CONCLR_SIC ;
            } else {
                PUT_DEBUG('c');
                I2Cx->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC ;
            }
        } else {
            PUT_DEBUG('d');
            I2Cx->CONSET = I2CONSET_STO ;     // Set Stop flag
            I2Cx->CONCLR = I2CONCLR_SIC ;
            KS_ISRsignal(TWI0SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;
        
#ifdef USE_TWI0_SLAVE
    case 0x60 :                 // Slave RX: Address received    
        I2Cx->CONSET = I2CONSET_AA ;
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;
    case 0x80 :                 // Slave RX: Data received  
        {
            volatile struct _SLAVEBUF *ps = &slave0 ;
            *(ps->sbuffer) = I2Cx->DAT ;
            if (ps->sbufcnt) {      // enough room ?
                (ps->sbuffer)++ ;   // pointer to next
                (ps->sbufcnt)-- ;
                (*(ps->spointer))++ ;
            }
        }
        I2Cx->CONSET = I2CONSET_AA ;
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;
    case 0xA0 :                 // Slave RX: end of packet
        I2Cx->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;
        KS_ISRsignal(TWI0SLSM) ;
        ASK_CONTEXTSWITCH ;     // set PendSV
        break ;
#endif // USE_TWI0_SLAVE

    default:
        PUT_HEX(StatValue);
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break;
    }
}

#ifdef USE_TWI1_ON_ARM
//----------------------------------------------------------------------------
// Interrupt routine for TWI1 for display

void I2C1_IRQHandler(void)
{
    uint8_t StatValue ;
    LPC_I2C_TypeDef *I2Cx = LPC_I2C1 ;

    // this handler deals with master read and master write only
    StatValue = I2Cx->STAT ;

    switch(StatValue) {
    case 0x08 :                 // A Start condition is issued.
        PUT_DEBUG('1');
        // write SlaveAddress with R=1/W=0 bit
        I2Cx->DAT = twi1addr ;
        I2Cx->CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    case 0x10 :                 // A repeated started is issued
        PUT_DEBUG('2');
        // write SlaveAddress with R=1 bit: surely a Read
        I2Cx->DAT = twi1addr | RD_BIT ;
        I2Cx->CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    //case 0x20 :                 //USE_TWI_SRV Master TX: SLAVEADDR+W sent, noACK
    case 0x18 :                 // Master TX: SLAVEADDR+W sent, ACK
        PUT_DEBUG('3');
    case 0x28 :                 // Master TX: DATA sent
        PUT_DEBUG('4');
        if (twi1askrestart) {
            PUT_DEBUG('a');
            I2Cx->CONSET = I2CONSET_STA ;     // Set Repeated-start flag
        } else if (twi1suba) {
            PUT_DEBUG('b');
            I2Cx->DAT = twi1suba ;
            twi1suba = 0 ;                       // handled
            twi1askrestart = 1 ;                 // set restart flag
        } else if (twi1buflen) {
            PUT_DEBUG('c');
            I2Cx->DAT = *twi1bufptr++ ;
            twi1buflen-- ;
        } else {
            PUT_DEBUG('d');
            I2Cx->CONSET = I2CONSET_STO ;     // Set Stop flag
            KS_ISRsignal(TWI1SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;

    //case 0x48 :                 // Master RX: SLAVEADDR+R sent, noACK
    case 0x40 :                 // Master RX: SLAVEADDR+R sent, ACK
        PUT_DEBUG('5');
        I2Cx->CONSET = I2CONSET_AA ;          // assert ACK after data is received
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break;

    case 0x58 :                 // Master RX: DATA received + NACK
        PUT_DEBUG('6');
    case 0x50 :                 // Master RX: DATA received + ACK
        PUT_DEBUG('7');
        if (twi1buflen) {
            PUT_DEBUG('a');
            *twi1bufptr++ = I2Cx->DAT ;
            twi1buflen-- ;
            if (twi1buflen != 0) {
                PUT_DEBUG('b');
                I2Cx->CONSET = I2CONSET_AA ;  // assert ACK after data is received
                I2Cx->CONCLR = I2CONCLR_SIC ;
            } else {
                PUT_DEBUG('c');
                I2Cx->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC ;
            }
        } else {
            PUT_DEBUG('d');
            I2Cx->CONSET = I2CONSET_STO ;     // Set Stop flag
            I2Cx->CONCLR = I2CONCLR_SIC ;
            KS_ISRsignal(TWI1SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;

#ifdef USE_TWI1_SLAVE
    case 0x60 :                 // Slave RX: Address received    
        I2Cx->CONSET = I2CONSET_AA ;
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;
    case 0x80 :                 // Slave RX: Data received    
        {
            volatile struct _SLAVEBUF *ps = &slave1 ;
            *(ps->sbuffer) = I2Cx->DAT ;
            if (ps->sbufcnt) {      // enough room ?
                (ps->sbuffer)++ ;   // pointer to next
                (ps->sbufcnt)-- ;
                (*(ps->spointer))++ ;
            }
        }
        I2Cx->CONSET = I2CONSET_AA ;
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;
    case 0xA0 :                 // Slave RX: end of packet
        I2Cx->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;
        KS_ISRsignal(TWI1SLSM) ;
        ASK_CONTEXTSWITCH ;     // set PendSV
        break ;
#endif // USE_TWI1_SLAVE
        
    default:
        PUT_HEX(StatValue);
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break;
    }
}
#endif // USE_TWI1_ON_ARM

#ifdef USE_TWI2_ON_ARM
//----------------------------------------------------------------------------
// Interrupt routine for TWI1 for display

void I2C2_IRQHandler(void)
{
    uint8_t StatValue ;
    LPC_I2C_TypeDef *I2Cx = LPC_I2C2 ;

    // this handler deals with master read and master write only
    StatValue = I2Cx->STAT ;

    switch(StatValue) {
    case 0x08 :                 // A Start condition is issued.
        PUT_DEBUG2('1');
        // write SlaveAddress with R=1/W=0 bit
        I2Cx->DAT = twi2addr ;
        I2Cx->CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    case 0x10 :                 // A repeated started is issued
        PUT_DEBUG2('2');
        // write SlaveAddress with R=1 bit: surely a Read
        I2Cx->DAT = twi2addr | RD_BIT ;
        I2Cx->CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    //case 0x20 :                 //USE_TWI_SRV Master TX: SLAVEADDR+W sent, noACK
    case 0x18 :                 // Master TX: SLAVEADDR+W sent, ACK
        PUT_DEBUG2('3');
    case 0x28 :                 // Master TX: DATA sent
        PUT_DEBUG2('4');
        if (twi2askrestart) {
            PUT_DEBUG2('a');
            I2Cx->CONSET = I2CONSET_STA ;     // Set Repeated-start flag
        } else if (twi2suba) {
            PUT_DEBUG2('b');
            I2Cx->DAT = twi2suba ;
            twi2suba = 0 ;                       // handled
            twi2askrestart = 1 ;                 // set restart flag
        } else if (twi2buflen) {
            PUT_DEBUG2('c');
            I2Cx->DAT = *twi2bufptr++ ;
            twi2buflen-- ;
        } else if (twi2rxflen){
            PUT_DEBUG2('e');
            I2Cx->CONSET = I2CONSET_STA ;     // Set Repeated-start flag
            twi2buflen = twi2rxflen ;
			twi2bufptr = twi2rxptr ;
			//twi2rxflen = 0 ;
        } else {
            PUT_DEBUG2('d');
            I2Cx->CONSET = I2CONSET_STO ;     // Set Stop flag
            KS_ISRsignal(TWI2SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;

    //case 0x48 :                 // Master RX: SLAVEADDR+R sent, noACK
    case 0x40 :                 // Master RX: SLAVEADDR+R sent, ACK
        PUT_DEBUG2('5');
        I2Cx->CONSET = I2CONSET_AA ;          // assert ACK after data is received
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break;

    case 0x58 :                 // Master RX: DATA received + NACK
        PUT_DEBUG2('6');
    case 0x50 :                 // Master RX: DATA received + ACK
        PUT_DEBUG2('7');
#ifdef GPS_I2C
        if (twi2buflen>1) {	// Modify 11/11/13
#else
        if (twi2buflen) {	// Modify 11/11/13
#endif
            PUT_DEBUG2('a');
            *twi2bufptr++ = I2Cx->DAT ;
#ifdef PROVA_GPSNEW
            *twi2bufptr = I2Cx->DAT ;
            if ((I2Cx->DAT==0xa0) || (twi2rxflen==0)){
				twi2rxflen = 0 ;
				twi2bufptr++;
				twi2buflen-- ;
			}
#else
			twi2buflen-- ;
#endif
#ifdef GPS_I2C
            if (twi2buflen>1) {	// Modify 11/11/13
#else
            if (twi2buflen != 0) {
#endif
                PUT_DEBUG2('b');
                I2Cx->CONSET = I2CONSET_AA ;  // assert ACK after data is received
                I2Cx->CONCLR = I2CONCLR_SIC ;
            } else {
                PUT_DEBUG2('c');
                I2Cx->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC ;
            }
        } else {
#ifdef GPS_I2C
			*twi2bufptr++ = I2Cx->DAT ;	// Modify 11/11/13
			twi2buflen-- ;				 // Modify 11/11/13
#endif
            PUT_DEBUG2('d');
            I2Cx->CONSET = I2CONSET_STO ;     // Set Stop flag
            I2Cx->CONCLR = I2CONCLR_SIC ;
            KS_ISRsignal(TWI2SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;

#ifdef USE_TWI2_SLAVE
    case 0x60 :                 // Slave RX: Address received    
        I2Cx->CONSET = I2CONSET_AA ;
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;
    case 0x80 :                 // Slave RX: Data received    
        {
            volatile struct _SLAVEBUF *ps = &slave2 ;
            *(ps->sbuffer) = I2Cx->DAT ;
            if (ps->sbufcnt) {      // enough room ?
                (ps->sbuffer)++ ;   // pointer to next
                (ps->sbufcnt)-- ;
                (*(ps->spointer))++ ;
            }
        }
        I2Cx->CONSET = I2CONSET_AA ;
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break ;
    case 0xA0 :                 // Slave RX: end of packet
//        I2Cx->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;
        I2Cx->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC  ;
        KS_ISRsignal(TWI2SLSM) ;
        ASK_CONTEXTSWITCH ;     // set PendSV
        break ;
#endif // USE_TWI0_SLAVE
        
    default:
        PUT_HEX(StatValue);
        I2Cx->CONCLR = I2CONCLR_SIC ;
        break;
    }
}
#endif // USE_TWI2_ON_ARM

//----------------------------------------------------------------------------
// TWI initializer

void twistart(void)
{
    LPC_SC->PCONP |= (CLKPWR_PCONP_PCI2C0  // enable it
#ifdef USE_TWI1_ON_ARM
                    | CLKPWR_PCONP_PCI2C1
#endif // USE_TWI1_ON_ARM
#ifdef USE_TWI2_ON_ARM
                    | CLKPWR_PCONP_PCI2C2
#endif // USE_TWI2_ON_ARM
                     ) ;

    // pins are already configured by dio.c

    // configure: Clear flags
    LPC_I2C0->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;

    // configure: set clock
    LPC_I2C0->SCLL = PERIPHERAL_CLOCK/50000 / 2 ;
    LPC_I2C0->SCLH = PERIPHERAL_CLOCK/50000 / 2 ;

    NVIC_EnableIRQ(I2C0_IRQn) ;
    NVIC_SetPriority(I2C0_IRQn, TWI_INTERRUPT_LEVEL) ;

#ifdef USE_TWI1_ON_ARM
    // conficure: Clear flags
    LPC_I2C1->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;

    // configure: set clock
    LPC_I2C1->SCLL = PERIPHERAL_CLOCK/50000 / 2 ;
    LPC_I2C1->SCLH = PERIPHERAL_CLOCK/50000 / 2 ;

    NVIC_EnableIRQ(I2C1_IRQn) ;
    NVIC_SetPriority(I2C1_IRQn, TWI_INTERRUPT_LEVEL) ;
#endif // USE_TWI1_ON_ARM

#ifdef USE_TWI2_ON_ARM
    // conficure: Clear flags
    LPC_I2C2->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;
	twi2rxflen = 0 ;
    // configure: set clock
#ifdef GPS_I2C
//     LPC_I2C2->SCLL = PERIPHERAL_CLOCK/50000  ;	// Prova
//     LPC_I2C2->SCLH = PERIPHERAL_CLOCK/50000  ;
    LPC_I2C2->SCLL = PERIPHERAL_CLOCK/50000 / 4 ;
    LPC_I2C2->SCLH = PERIPHERAL_CLOCK/50000 / 4 ;
#else
    LPC_I2C2->SCLL = PERIPHERAL_CLOCK/50000 / 2 ;
    LPC_I2C2->SCLH = PERIPHERAL_CLOCK/50000 / 2 ;
#endif

    NVIC_EnableIRQ(I2C2_IRQn) ;
    NVIC_SetPriority(I2C2_IRQn, TWI_INTERRUPT_LEVEL) ;
#endif // USE_TWI1_ON_ARM
}

//----------------------------------------------------------------------------
// TWI terminator

void twistop(void)
{
    LPC_I2C0->CONCLR = I2CONCLR_I2ENC ;   // disable
    NVIC_DisableIRQ(I2C0_IRQn) ;            // disable interrupt

#ifdef USE_TWI1_ON_ARM
    LPC_I2C1->CONCLR = I2CONCLR_I2ENC ;   // disable
    NVIC_DisableIRQ(I2C1_IRQn) ;            // disable interrupt
#endif // USE_TWI1_ON_ARM

#ifdef USE_TWI2_ON_ARM
    LPC_I2C2->CONCLR = I2CONCLR_I2ENC ;   // disable
    NVIC_DisableIRQ(I2C2_IRQn) ;            // disable interrupt
#endif // USE_TWI2_ON_ARM

    LPC_SC->PCONP &= ~(CLKPWR_PCONP_PCI2C0  // disable it
#ifdef USE_TWI1_ON_ARM
                     | CLKPWR_PCONP_PCI2C1
#endif // USE_TWI1_ON_ARM
#ifdef USE_TWI1_ON_ARM
                     | CLKPWR_PCONP_PCI2C2
#endif // USE_TWI1_ON_ARM
                      ) ;

    // pins will be un-configured by dio.c
}

//----------------------------------------------------------------------------
// TWI TX with interrupt

int TWI_send(int dev, unsigned char *buf, int len)
{
    twi0askrestart = 0 ;         // clear restart flag
    twi0suba = 0 ;
    twi0addr = dev & (~RD_BIT) ; // SLA+W
    twi0bufptr = buf ;           // TWI buffer pointer
    twi0buflen = len ;           // TWI buffer len

    // Enable
    LPC_I2C0->CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    LPC_I2C0->CONSET = I2CONSET_STA ; // Set Start flag

    // wait end
    if (KS_waitt(TWI0SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
        twi0err++ ;
#ifdef CBUG
        pdebugt(1,"TWI0-%02x TX error %d, len=%d/%d", dev, twi0err, twi0buflen, len) ;
#endif // CBUG
        twistop() ;
        tickwait(WAIT_USEC) ;       // waste usec
        twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;       // waste usec

    LPC_I2C0->CONCLR = I2CONCLR_I2ENC ;

#ifdef CBUG_
    if (dev==0x30) pdebugt(1,"TWI_send0 %x OK", dev) ;
#endif // CBUG

    return(OK) ;
}

//----------------------------------------------------------------------------
// TWI RX with interrupt

int TWI_txrx(int dev, int sub, unsigned char *buf, int len)
{
    twi0askrestart = 0 ;         // clear restart flag
    twi0suba = sub ;             // must be 0 in order to bypass
    if (sub) {
        twi0addr = dev & (~RD_BIT) ;     // SLA+W
    } else {
        twi0addr = dev | (RD_BIT) ;      // SLA+R
    }
    twi0bufptr = buf ;           // TWI buffer pointer
    twi0buflen = len ;           // TWI buffer len

    // Enable
    LPC_I2C0->CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    LPC_I2C0->CONSET = I2CONSET_STA ; // Set Start flag

#ifdef CBUG_ // prova per incastrare sensore acc (non ci riesco)

    if ((twi0err<10) && (par71)){
        tickwait(WAIT_USEC) ;       // waste usec
        twi0askrestart = 0 ;         // clear restart flag
        twi0suba = sub ;             // must be 0 in order to bypass
        if (sub) {
            twi0addr = dev & (~RD_BIT) ;     // SLA+W
        } else {
            twi0addr = dev | (RD_BIT) ;      // SLA+R
        }
        twi0bufptr = buf ;           // TWI buffer pointer
        twi0buflen = len ;           // TWI buffer len

        // Enable
        LPC_I2C0->CONSET = I2CONSET_I2EN ;
        // Issue a start condition
        LPC_I2C0->CONSET = I2CONSET_STA ; // Set Start flag
//      twistop() ;
//      tickwait(WAIT_USEC) ;       // waste usec
//      twistart() ;    // reinit
//      twi0err++ ;
        KS_waitt(TWI0SEM, ((TICKS)1000/CLKTICK)) ;
        LPC_I2C0->I2CONCLR = I2CONCLR_I2ENC ;
//		KS_signal(CBUGSEMA) ;       // wake up debugger now
	}
#endif
    // wait end
    if (KS_waitt(TWI0SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
        twi0err++ ;
#ifdef CBUG
        pdebugt(1,"TWI0-%02x RX error %d, len=%d/%d", dev, twi0err, twi0buflen, len) ;
#endif // CBUG
        twistop() ;
        tickwait(WAIT_USEC) ;       // waste usec
        twistart() ;    // reinit
        return(ERROR) ;
    }
    tickwait(WAIT_USEC) ;       // waste usec

    LPC_I2C0->CONCLR = I2CONCLR_I2ENC ;

#ifdef CBUG_
    if (dev==0x30) pdebugt(1,"TWI_txrx0 %x OK", dev) ;
#endif // CBUG
    return(OK) ;
}

#ifdef USE_TWI1_ON_ARM
//----------------------------------------------------------------------------
// TWI1 TX with interrupt

int TWI1_send(int dev, unsigned char *buf, int len)
{
    twi1askrestart = 0 ;         // clear restart flag
    twi1suba = 0 ;
    twi1addr = dev & (~RD_BIT) ; // SLA+W
    twi1bufptr = buf ;           // TWI buffer pointer
    twi1buflen = len ;           // TWI buffer len

    // Enable
    LPC_I2C1->CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    LPC_I2C1->CONSET = I2CONSET_STA ; // Set Start flag

    // wait end
    if (KS_waitt(TWI1SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
        twi1err++ ;
#ifdef CBUG
        pdebugt(1,"TWI1-%02x TX error %d, len=%d/%d", dev, twi1err, twi1buflen, len) ;
#endif // CBUG
        twistop() ;
        tickwait(WAIT_USEC) ;       // waste usec
        twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;       // waste usec

    LPC_I2C1->CONCLR = I2CONCLR_I2ENC ;

    return(OK) ;
}

//----------------------------------------------------------------------------
// TWI RX with interrupt

int TWI1_txrx(int dev, int sub, unsigned char *buf, int len)
{
    twi1askrestart = 0 ;         // clear restart flag
    twi1suba = sub ;             // must be 0 in order to bypass
    if (sub) {
        twi1addr = dev & (~RD_BIT) ;     // SLA+W
    } else {
        twi1addr = dev | (RD_BIT) ;      // SLA+R
    }
    twi1bufptr = buf ;           // TWI buffer pointer
    twi1buflen = len ;           // TWI buffer len

    // Enable
    LPC_I2C1->CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    LPC_I2C1->CONSET = I2CONSET_STA ; // Set Start flag

    // wait end
    if (KS_waitt(TWI1SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
        twi1err++ ;
#ifdef CBUG
        pdebugt(1,"TWI1-%02x RX error %d, len=%d/%d", dev, twi1err, twi1buflen, len) ;
#endif // CBUG
        twistop() ;
        tickwait(WAIT_USEC) ;       // waste usec
        twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;       // waste usec

    LPC_I2C1->CONCLR = I2CONCLR_I2ENC ;

    return(OK) ;
}


#endif // USE_TWI1_ON_ARM

#ifdef USE_TWI2_ON_ARM
//----------------------------------------------------------------------------
// TWI2 TX with interrupt

int TWI2_send(int dev, unsigned char *buf, int len)
{
    twi2askrestart = 0 ;         // clear restart flag
    twi2suba = 0 ;
    twi2addr = dev & (~RD_BIT) ; // SLA+W
    twi2bufptr = buf ;           // TWI buffer pointer
    twi2buflen = len ;           // TWI buffer len

    // Enable
    LPC_I2C2->CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    LPC_I2C2->CONSET = I2CONSET_STA ; // Set Start flag

    // wait end
    if (KS_waitt(TWI2SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
        twi2err++ ;
#ifdef CBUG
        pdebugt(1,"TWI2-%02x TX error %d, len=%d/%d", dev, twi2err, twi2buflen, len) ;
#endif // CBUG
        twistop() ;
        tickwait(WAIT_USEC) ;       // waste usec
        twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;       // waste usec

    LPC_I2C2->CONCLR = I2CONCLR_I2ENC ;

    return(OK) ;
}

//----------------------------------------------------------------------------
// TWI RX with interrupt

int TWI2_txrx(int dev, int sub, unsigned char *buf, int len)
{
    twi2askrestart = 0 ;         // clear restart flag
    twi2suba = sub ;             // must be 0 in order to bypass
    if (sub) {
        twi2addr = dev & (~RD_BIT) ;     // SLA+W
    } else {
        twi2addr = dev | (RD_BIT) ;      // SLA+R
    }
    twi2bufptr = buf ;           // TWI buffer pointer
    twi2buflen = len ;           // TWI buffer len

    // Enable
    LPC_I2C2->CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    LPC_I2C2->CONSET = I2CONSET_STA ; // Set Start flag

    // wait end
    if (KS_waitt(TWI2SEM, ((TICKS)1000/CLKTICK)) == RC_TIMEOUT) {
        twi2err++ ;
#ifdef CBUG
        pdebugt(1,"TWI2-%02x RX error %d, len=%d/%d", dev, twi2err, twi2buflen, len) ;
#endif // CBUG
        twistop() ;
        tickwait(WAIT_USEC) ;       // waste usec
        twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;       // waste usec

    LPC_I2C2->CONCLR = I2CONCLR_I2ENC ;

    return(OK) ;
}
#if defined(USE_TWI0_SLAVE) || defined(USE_TWI1_SLAVE) || defined(USE_TWI2_SLAVE) 
//----------------------------------------------------------------------------
// TWI RX slave configure
void TWI_SlaveRx(int port, int slaveaddr, uint8_t *buf, volatile int *len)
{
    LPC_I2C_TypeDef *I2Cx ;
    volatile struct _SLAVEBUF *ps ;
    
    switch(port) {
#if defined(USE_TWI0_SLAVE)
    case 0 : 
        I2Cx = LPC_I2C0 ;
        ps = &slave0 ;
        break ;
#endif // defined(USE_TWI0_SLAVE)
#if defined(USE_TWI1_SLAVE)
    case 1 : 
        I2Cx = LPC_I2C1 ;
        ps = &slave1 ;
        break ;
#endif // defined(USE_TWI1_SLAVE)
#if defined(USE_TWI2_SLAVE)
    case 2 : 
        I2Cx = LPC_I2C2 ;
        ps = &slave2 ;
        break ;
#endif // defined(USE_TWI2_SLAVE)
    default :    
#ifdef CBUG
        pdebugt(1, "TWI port %d not configured", port) ;
#endif // CBUG
        return ;
    }
    
    // simply stop TWI
    if (!buf || !len) {
        I2Cx->CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;
		I2Cx->ADR0 = 0 ;
		I2Cx->MASK0 = 0 ;
        return ;
    }
    
    // enable salve mode
    ps->sbuffer = buf ;
    ps->sbufcnt = *len ;        // max buf len
    ps->spointer = len ;        // address of len
    *len = 0 ;                  // len init
    
    // set slave address
    I2Cx->ADR0 = slaveaddr & 0xfe ;     // lower bit is R/W
    I2Cx->MASK0 = 0xfe ;
    
    // ready to accept address 
    // Enable
    I2Cx->CONSET = I2CONSET_I2EN ;
    // Enable AA
    I2Cx->CONSET = I2CONSET_AA ;
    I2Cx->CONCLR = I2CONCLR_SIC | I2CONCLR_STAC ;        
}
#endif // defined(USE_TWI0_SLAVE) || defined(USE_TWI1_SLAVE) || defined(USE_TWI2_SLAVE) 

//----------------------------------------------------------------------------
// TWI2 TX with interrupt

int TWI2_sendrx(int dev, unsigned char *buf, int len, unsigned char *bufrx, int lenrx)
{
    twi2askrestart = 0 ;         // clear restart flag
    twi2suba = 0 ;
    twi2addr = dev & (~RD_BIT) ; // SLA+W
    twi2bufptr = buf ;           // TWI buffer pointer
    twi2buflen = len ;           // TWI buffer len
	twi2rxptr = bufrx ;
	twi2rxflen = lenrx ;
	
    // Enable
    LPC_I2C2->CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    LPC_I2C2->CONSET = I2CONSET_STA ; // Set Start flag

    // wait end
    if (KS_waitt(TWI2SEM, ((TICKS)500/CLKTICK)) == RC_TIMEOUT) {
        twi2err++ ;
#ifdef CBUG
        pdebugt(1,"TWI2-%02x TX error %d, len=%d/%d", dev, twi2err, twi2buflen, len) ;
#endif // CBUG
        twistop() ;
        tickwait(WAIT_USEC) ;       // waste usec
        twistart() ;    // reinit
        return(ERROR) ;
    }

    tickwait(WAIT_USEC) ;       // waste usec

    LPC_I2C2->CONCLR = I2CONCLR_I2ENC ;

    return(OK) ;
}


#endif // USE_TWI2_ON_ARM

#endif // USE_TWI_ON_ARM
// end of file - drv_twi.c
