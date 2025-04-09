// drv_twi.c - TWI driver tasks
//
//   Copyright (c) 1997-2012.
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

#define TWI0_SPEED  50000           // Hz

// Fast TW1 for accelerometer
// sanity check
#ifdef USE_TW1_FAST_ACCELEROMETER
#define TWI1_SPEED  100000          // Hz

#ifndef USE_TWI1_AUXILIARY
#error "USE_TW1_FAST_ACCELEROMETER requires USE_TWI1_AUXILIARY"
#endif // USE_TWI1_AUXILIARY

#define ACC_SAMPLE_FREQ     400     // Hz

unsigned char * tw1fastbuffer ;     // buffer pointer
int tw1fastbufsize ;                // buffer size

unsigned char * tw1fastOUTptr ;
int tw1fastnum ;                    // elements in buffer
#else // USE_TW1_FAST_ACCELEROMETER

#define TWI1_SPEED  50000           // Hz

#endif // USE_TW1_FAST_ACCELEROMETER

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
#define PUT_HEX(B) {PUT_CHAR('_');PUT_CHAR((((B)>>4)&0xf)+'0');PUT_CHAR(((B)&0xf)+'0');PUT_CHAR('_');}
#define PUT_DEBUG(B) {PUT_CHAR('_');PUT_CHAR(B);PUT_CHAR('_');}
#define PUT_CHAR(A) {while(!((UART2->LSR) & 0x20));UART2->THR=(A);}
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

//----------------------------------------------------------------------------
// local data

volatile unsigned char twi0addr ;               // TWI slave address + RW
volatile unsigned char twi0suba ;               // TWI slave subaddress
volatile unsigned char twi0askrestart ;         // TWI restart request
volatile unsigned char * twi0bufptr ;           // TWI buffer pointer
volatile int twi0buflen ;                       // TWI buffer len

unsigned short twi0err = 0 ;

#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
volatile unsigned char twi1addr ;               // TWI slave address + RW
volatile unsigned char twi1suba ;               // TWI slave subaddress
volatile unsigned char twi1askrestart ;         // TWI restart request
volatile unsigned char * twi1bufptr ;           // TWI buffer pointer
volatile int twi1buflen ;                       // TWI buffer len

unsigned short twi1err = 0 ;
#endif // defined(USE_TWI1_AUXILIARY)

//----------------------------------------------------------------------------
// internal functions

void twistart(void) ;
void twistop(void) ;

#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
int TWI1_send(int dev, unsigned char *buf, int len) ;
int TWI1_txrx(int dev, int sub, unsigned char *buf, int len) ;
#endif

#ifndef USE_TWI_ONLYAUX
int TWI_send(int dev, unsigned char *buf, int len) ;
int TWI_txrx(int dev, int sub, unsigned char *buf, int len) ;
#endif
//----------------------------------------------------------------------------
// Interrupt routine for TWI
#ifndef USE_TWI_ONLYAUX
void I2C0_IRQHandler(void)
{
    uint8_t StatValue ;
    I2C_TypeDef *I2Cx = I2C0 ;

    // this handler deals with master read and master write only
    StatValue = I2Cx->I2STAT ;

    switch(StatValue) {
    case 0x08 :                 // A Start condition is issued.
        PUT_DEBUG('1');
        // write SlaveAddress with R=1/W=0 bit
        I2Cx->I2DAT = twi0addr ;
        I2Cx->I2CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    case 0x10 :                 // A repeated started is issued
        PUT_DEBUG('2');
        // write SlaveAddress with R=1 bit: surely a Read
        I2Cx->I2DAT = twi0addr | RD_BIT ;
        I2Cx->I2CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    //case 0x20 :                 // Master TX: SLAVEADDR+W sent, noACK
    case 0x18 :                 // Master TX: SLAVEADDR+W sent, ACK
        PUT_DEBUG('3');
    case 0x28 :                 // Master TX: DATA sent
        PUT_DEBUG('4');
        if (twi0askrestart) {
            PUT_DEBUG('a');
            I2Cx->I2CONSET = I2CONSET_STA ;     // Set Repeated-start flag
        } else if (twi0suba) {
            PUT_DEBUG('b');
            I2Cx->I2DAT = twi0suba ;
            twi0suba = 0 ;                       // handled
            twi0askrestart = 1 ;                 // set restart flag
        } else if (twi0buflen) {
            PUT_DEBUG('c');
            I2Cx->I2DAT = *twi0bufptr++ ;
            twi0buflen-- ;
        } else {
            PUT_DEBUG('d');
            I2Cx->I2CONSET = I2CONSET_STO ;     // Set Stop flag
            KS_ISRsignal(TWI0SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->I2CONCLR = I2CONCLR_SIC ;
        break ;

    //case 0x48 :                 // Master RX: SLAVEADDR+R sent, noACK
    case 0x40 :                 // Master RX: SLAVEADDR+R sent, ACK
        PUT_DEBUG('5');
        I2Cx->I2CONSET = I2CONSET_AA ;          // assert ACK after data is received
        I2Cx->I2CONCLR = I2CONCLR_SIC ;
        break;

    case 0x58 :                 // Master RX: DATA received + NACK
        PUT_DEBUG('6');
    case 0x50 :                 // Master RX: DATA received + ACK
        PUT_DEBUG('7');
        if (twi0buflen) {
            PUT_DEBUG('a');
            *twi0bufptr++ = I2Cx->I2DAT ;
            twi0buflen-- ;
            if (twi0buflen != 0) {
                PUT_DEBUG('b');
                I2Cx->I2CONSET = I2CONSET_AA ;  // assert ACK after data is received
                I2Cx->I2CONCLR = I2CONCLR_SIC ;
            } else {
                PUT_DEBUG('c');
                I2Cx->I2CONCLR = I2CONCLR_AAC | I2CONCLR_SIC ;
            }
        } else {
            PUT_DEBUG('d');
            I2Cx->I2CONSET = I2CONSET_STO ;     // Set Stop flag
            I2Cx->I2CONCLR = I2CONCLR_SIC ;
            KS_ISRsignal(TWI0SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->I2CONCLR = I2CONCLR_SIC ;
        break ;

    default:
        PUT_HEX(StatValue);
        I2Cx->I2CONCLR = I2CONCLR_SIC ;
        break;
    }
}
#endif // #ifndef USE_TWI_ONLYAUX

#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
//----------------------------------------------------------------------------
// Interrupt routine for TWI1

void I2C1_IRQHandler(void)
{
    uint8_t StatValue ;
    I2C_TypeDef *I2Cx = I2C1 ;

    // this handler deals with master read and master write only
    StatValue = I2Cx->I2STAT ;

    switch(StatValue) {
    case 0x08 :                 // A Start condition is issued.
        PUT_DEBUG('1');
        // write SlaveAddress with R=1/W=0 bit
        I2Cx->I2DAT = twi1addr ;
        I2Cx->I2CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    case 0x10 :                 // A repeated started is issued
        PUT_DEBUG('2');
        // write SlaveAddress with R=1 bit: surely a Read
        I2Cx->I2DAT = twi1addr | RD_BIT ;
        I2Cx->I2CONCLR = (I2CONCLR_SIC | I2CONCLR_STAC) ;
        break ;

    //case 0x20 :                 //USE_TWI_SRV Master TX: SLAVEADDR+W sent, noACK
    case 0x18 :                 // Master TX: SLAVEADDR+W sent, ACK
        PUT_DEBUG('3');
    case 0x28 :                 // Master TX: DATA sent
        PUT_DEBUG('4');
        if (twi1askrestart) {
            PUT_DEBUG('a');
            I2Cx->I2CONSET = I2CONSET_STA ;     // Set Repeated-start flag
        } else if (twi1suba) {
            PUT_DEBUG('b');
            I2Cx->I2DAT = twi1suba ;
            twi1suba = 0 ;                       // handled
            twi1askrestart = 1 ;                 // set restart flag
        } else if (twi1buflen) {
            PUT_DEBUG('c');
            I2Cx->I2DAT = *twi1bufptr++ ;
            twi1buflen-- ;
        } else {
            PUT_DEBUG('d');
            I2Cx->I2CONSET = I2CONSET_STO ;     // Set Stop flag
            KS_ISRsignal(TWI1SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
        }
        I2Cx->I2CONCLR = I2CONCLR_SIC ;
        break ;

    //case 0x48 :                 // Master RX: SLAVEADDR+R sent, noACK
    case 0x40 :                 // Master RX: SLAVEADDR+R sent, ACK
        PUT_DEBUG('5');
        I2Cx->I2CONSET = I2CONSET_AA ;          // assert ACK after data is received
        I2Cx->I2CONCLR = I2CONCLR_SIC ;
        break;

    case 0x58 :                 // Master RX: DATA received + NACK
        PUT_DEBUG('6');
    case 0x50 :                 // Master RX: DATA received + ACK
        PUT_DEBUG('7');
        if (twi1buflen) {
            PUT_DEBUG('a');
            *twi1bufptr++ = I2Cx->I2DAT ;
            twi1buflen-- ;
            if (twi1buflen != 0) {
                PUT_DEBUG('b');
                I2Cx->I2CONSET = I2CONSET_AA ;  // assert ACK after data is received
                I2Cx->I2CONCLR = I2CONCLR_SIC ;
            } else {
                PUT_DEBUG('c');
                I2Cx->I2CONCLR = I2CONCLR_AAC | I2CONCLR_SIC ;
            }
        } else {
            PUT_DEBUG('d');
            I2Cx->I2CONSET = I2CONSET_STO ;     // Set Stop flag
            I2Cx->I2CONCLR = I2CONCLR_SIC ;
#ifdef USE_TW1_FAST_ACCELEROMETER
            if (tw1fastbuffer) {    // fast mode enabled
                tw1fastnum++ ;      // another tripleaxis
                if (twi1bufptr >= &tw1fastbuffer[tw1fastbufsize])
                    twi1bufptr = tw1fastbuffer ;    // re-init producer ptr
            } else {
                if (tw1fastbufsize) {
                    tw1fastbufsize = 0 ;        // from now, use TWI1SEM
                } else {
                    KS_ISRsignal(TWI1SEM) ;
                    ASK_CONTEXTSWITCH ;         // set PendSV
                }
            }
#else // USE_TW1_FAST_ACCELEROMETER
            KS_ISRsignal(TWI1SEM) ;
            ASK_CONTEXTSWITCH ;                 // set PendSV
#endif // USE_TW1_FAST_ACCELEROMETER
        }
        I2Cx->I2CONCLR = I2CONCLR_SIC ;
        break ;

    default:
        PUT_HEX(StatValue);
        I2Cx->I2CONCLR = I2CONCLR_SIC ;
        break;
    }
}
#endif // (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )

#ifdef USE_TW1_FAST_ACCELEROMETER
//----------------------------------------------------------------------------
// Interrupt routine for Timer3

#if defined(USE_TWI_ACCELEROMETER_MMA8451)
#define FAST_ACC_DEV    0x38    // this device address
#define FAST_ACC_SUBA   1       // subaddress
#else //  defined(USE_TWI_ACCELEROMETER_MMA8451)
#error "Unknown accelerometer"
#endif //  defined(USE_TWI_ACCELEROMETER_MMA8451)

void TIMER3_IRQHandler(void)
{
    int ir = TIM3->IR ;         // don't bother us

#ifdef CBUG
    if (twi1buflen) {           // test if everything done in time by TWI1

        twi1err++ ;
        //KS_ISRsignal(CBUGSEMA) ;
        //ASK_CONTEXTSWITCH ;                 // set PendSV

        // disable use timer 3 interrupt
        //TIM3->TCR = 2 ;     // reset

        TIM3->IR  = ir ;        // reset interrrupt
        return ;
    }
#endif // CBUG
    I2C1->I2CONCLR = I2CONCLR_I2ENC ;   // disable

    twi1buflen = 6 ;            // how many samples
    twi1askrestart = 0 ;        // clear restart flag
    twi1suba = FAST_ACC_SUBA ;                  // subaddress
    twi1addr = FAST_ACC_DEV & (~RD_BIT) ;       // SLA+W
    // twi1bufptr set by enable

    // Enable
    I2C1->I2CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    I2C1->I2CONSET = I2CONSET_STA ; // Set Start flag

    TIM3->IR  = ir ;                // reset interrrupt
}
#endif // USE_TW1_FAST_ACCELEROMETER

//----------------------------------------------------------------------------
// TWI initializer

void twistart(void)
{
#ifndef USE_TWI_ONLYAUX
    SC->PCONP |= PCONP_PCI2C0 ;         // enable it

    // P0.27 = SDA
    // P0.28 = SCL
    PINCON->PINSEL1 |= 0x01400000 ;
    PINCON->PINSEL1 &= ~0x02800000 ;

    // default div by 4
    SC->PCLKSEL0 &= ~0x0000c000 ;       // bit15,14: 00 - PCLK = CCLK / 4

    // configure: Clear flags
    I2C0->I2CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;

    // configure: set clock
    I2C0->I2SCLL = current_clock/4/TWI0_SPEED / 2 ;
    I2C0->I2SCLH = current_clock/4/TWI0_SPEED / 2 ;

    NVIC_EnableIRQ(I2C0_IRQn) ;
    NVIC_SetPriority(I2C0_IRQn, TWI_INTERRUPT_LEVEL) ;
#endif // #ifndef USE_TWI_ONLYAUX

#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
    SC->PCONP |= PCONP_PCI2C1 ;         // enable it

#ifdef USE_TWI1_ON_P00      // useful for LPC1758 cpu
    // P0.0 = SDA
    // P0.1 = SCL
    PINCON->PINSEL0 |= 0x0000000F ;
#else // USE_TWI1_ON_P00    // normal LPC1768 cpu
    // P0.19 = SDA
    // P0.20 = SCL
    PINCON->PINSEL1 |= 0x000003C0 ;
#endif // USE_TWI1_ON_P00

    // default div by 4
// #ifdef M3108
// 	SC->PCLKSEL1 &= ~0x000000c0 ;       // bit7,6: 00 - PCLK = CCLK / 4
// 	SC->PCLKSEL1 |= 0x000000c0 ;       // bit7,6: 11 - PCLK = CCLK / 8
// #else
	SC->PCLKSEL1 &= ~0x000000c0 ;       // bit7,6: 00 - PCLK = CCLK / 4
//#endif

    // conficure: Clear flags
    I2C1->I2CONCLR = I2CONCLR_AAC | I2CONCLR_SIC | I2CONCLR_STAC | I2CONCLR_I2ENC ;

    // configure: set clock
    I2C1->I2SCLL = current_clock/4/TWI1_SPEED / 2 ;
    I2C1->I2SCLH = current_clock/4/TWI1_SPEED / 2 ;

    NVIC_EnableIRQ(I2C1_IRQn) ;
    NVIC_SetPriority(I2C1_IRQn, TWI_INTERRUPT_LEVEL) ;
#endif // (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )

#ifdef USE_TW1_FAST_ACCELEROMETER
    // in case of fast accelerometer, use timer 3 interrupt
    SC->PCONP |= PCONP_PCTIM3 ;
    TIM3->TCR = 2 ;     // reset

    tw1fastbufsize = 0 ;    // use TWI1SEM
    tw1fastbuffer = NULL ;
    tw1fastnum = 0 ;

    // enable interrupt from timer 3
    NVIC_EnableIRQ(TIMER3_IRQn) ;
    NVIC_SetPriority(TIMER3_IRQn, TWI_INTERRUPT_LEVEL) ;
#endif // USE_TW1_FAST_ACCELEROMETER
}

//----------------------------------------------------------------------------
// TWI terminator

void twistop(void)
{
#ifndef USE_TWI_ONLYAUX
    I2C0->I2CONCLR = I2CONCLR_I2ENC ;   // disable

    NVIC_DisableIRQ(I2C0_IRQn) ;        // disable interrupt

    SC->PCONP &= (~PCONP_PCI2C0) ;      // disable it

    GPIO0->FIODIR |= 0x18000000 ;       // all outputs
    GPIO0->FIOSET  = 0x18000000 ;       // all outputs at '1'

    PINCON->PINSEL1 &= ~0x03c00000 ;    // GPIO again
#endif // #ifndef USE_TWI_ONLYAUX

#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
    I2C1->I2CONCLR = I2CONCLR_I2ENC ;   // disable

    NVIC_DisableIRQ(I2C1_IRQn) ;        // disable interrupt

    SC->PCONP &= (~PCONP_PCI2C1) ;      // disable it

#ifdef USE_TWI1_ON_P00      // useful for LPC1758 cpu
    GPIO0->FIODIR |= 0x00000003 ;       // all outputs
    GPIO0->FIOSET  = 0x00000003 ;       // all outputs at '1'
#else // USE_TWI1_ON_P00    // normal LPC1768 cpu
    GPIO0->FIODIR |= 0x00180000 ;       // all outputs
    GPIO0->FIOSET  = 0x00180000 ;       // all outputs at '1'
#endif // USE_TWI1_ON_P00

    PINCON->PINSEL1 &= ~0x000003C0 ;    // GPIO again
#endif // (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )

#ifdef USE_TW1_FAST_ACCELEROMETER
    // disable interrupt from timer 3
    NVIC_DisableIRQ(TIMER3_IRQn) ;

    // disable use timer 3 interrupt
    TIM3->TCR = 2 ;     // reset
    SC->PCONP &= ~PCONP_PCTIM3 ;
#endif // USE_TW1_FAST_ACCELEROMETER
}

//----------------------------------------------------------------------------
// TWI TX with interrupt
#ifndef USE_TWI_ONLYAUX
int TWI_send(int dev, unsigned char *buf, int len)
{
    twi0askrestart = 0 ;         // clear restart flag
    twi0suba = 0 ;
    twi0addr = dev & (~RD_BIT) ; // SLA+W
    twi0bufptr = buf ;           // TWI buffer pointer
    twi0buflen = len ;           // TWI buffer len


    // Enable
    I2C0->I2CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    I2C0->I2CONSET = I2CONSET_STA ; // Set Start flag

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

    I2C0->I2CONCLR = I2CONCLR_I2ENC ;

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
    I2C0->I2CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    I2C0->I2CONSET = I2CONSET_STA ; // Set Start flag

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
			I2C0->I2CONSET = I2CONSET_I2EN ;
			// Issue a start condition
			I2C0->I2CONSET = I2CONSET_STA ; // Set Start flag
//         twistop() ;
//         tickwait(WAIT_USEC) ;       // waste usec
//         twistart() ;    // reinit
//         twi0err++ ;
		KS_waitt(TWI0SEM, ((TICKS)1000/CLKTICK)) ;
		I2C0->I2CONCLR = I2CONCLR_I2ENC ;
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

    I2C0->I2CONCLR = I2CONCLR_I2ENC ;

#ifdef CBUG_
        if (dev==0x30) pdebugt(1,"TWI_txrx0 %x OK", dev) ;
#endif // CBUG
    return(OK) ;
}
#endif // #ifndef USE_TWI_ONLYAUX

#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
//----------------------------------------------------------------------------
// TWI1 TX with interrupt

int TWI1_send(int dev, unsigned char *buf, int len)
{
#ifdef USE_TW1_FAST_ACCELEROMETER
    if (tw1fastbuffer) return(ERROR) ;  // fast mode enabled
#endif // USE_TW1_FAST_ACCELEROMETER

    twi1askrestart = 0 ;            // clear restart flag
    twi1suba = 0 ;
    twi1addr = dev & (~RD_BIT) ;    // SLA+W
    twi1bufptr = buf ;              // TWI buffer pointer
    twi1buflen = len ;              // TWI buffer len
#ifdef USE_TW1_FAST_ACCELEROMETER
    tw1fastbufsize = 0 ;            // use TWI1SEM
#endif // USE_TW1_FAST_ACCELEROMETER

    // Enable
    I2C1->I2CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    I2C1->I2CONSET = I2CONSET_STA ; // Set Start flag

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

    I2C1->I2CONCLR = I2CONCLR_I2ENC ;

    return(OK) ;
}

//----------------------------------------------------------------------------
// TWI RX with interrupt

int TWI1_txrx(int dev, int sub, unsigned char *buf, int len)
{
#ifdef USE_TW1_FAST_ACCELEROMETER
    if (tw1fastbuffer) return(ERROR) ;  // fast mode enabled
#endif // USE_TW1_FAST_ACCELEROMETER
    twi1askrestart = 0 ;         // clear restart flag
    twi1suba = sub ;             // must be 0 in order to bypass
    if (sub) {
        twi1addr = dev & (~RD_BIT) ;     // SLA+W
    } else {
        twi1addr = dev | (RD_BIT) ;      // SLA+R
    }
    twi1bufptr = buf ;          // TWI buffer pointer
    twi1buflen = len ;          // TWI buffer len
#ifdef USE_TW1_FAST_ACCELEROMETER
    tw1fastbufsize = 0 ;        // use TWI1SEM
#endif // USE_TW1_FAST_ACCELEROMETER

    // Enable
    I2C1->I2CONSET = I2CONSET_I2EN ;
    // Issue a start condition
    I2C1->I2CONSET = I2CONSET_STA ; // Set Start flag

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

    I2C1->I2CONCLR = I2CONCLR_I2ENC ;

    return(OK) ;
}

#endif // (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )

#ifdef USE_TW1_FAST_ACCELEROMETER
//----------------------------------------------------------------------------
// TWI RX with interrupt

void TWI1_setfast(unsigned char *buffer, int bufsize)
{
    if (buffer) {
        if (bufsize % 6) return ;       // must be multiple of tipleaxis size

        tw1fastbuffer = buffer ;
        tw1fastbufsize = bufsize ;
        twi1bufptr = tw1fastbuffer ;    // TWI buffer pointer

        tw1fastOUTptr = tw1fastbuffer ; // init consumer ptr
        tw1fastnum = 0 ;

        // in case of fast accelerometer, use timer 3 interrupt
        TIM3->TCR = 2 ;     // reset
        TIM3->CTCR = 0 ;    // count at PCLK
        TIM3->MR0 = current_clock/4/ACC_SAMPLE_FREQ ;   // desired frequency
        TIM3->MCR = 3 ;     // reset TC on match with MR0 + INTERRUPT
        TIM3->IR  = 0xff;   // reset all interrrupts
        TIM3->TCR = 1 ;     // enable

    } else {    // disable

        tw1fastbuffer = NULL ;

        // disable use timer 3 interrupt
        TIM3->TCR = 2 ;     // reset
    }
}


int TWI1_get3axisfast(short *ax, short *ay, short *az)
{
    if (!tw1fastnum) return(0) ;    // no data
    //if (!tw1fastnum) return(1) ;    // no data

    *ax = (tw1fastOUTptr[0]<<8) + tw1fastOUTptr[1] ;
    *ay = (tw1fastOUTptr[2]<<8) + tw1fastOUTptr[3] ;
    *az = (tw1fastOUTptr[4]<<8) + tw1fastOUTptr[5] ;
    tw1fastOUTptr += 6 ;
    if (tw1fastOUTptr >= &tw1fastbuffer[tw1fastbufsize])
        tw1fastOUTptr = tw1fastbuffer ;   // re-init consumer ptr

    __disable_irq() ;   // critical region
    tw1fastnum-- ;
    __enable_irq() ;

    return((tw1fastnum+1)) ;     // we have data
//    return(0) ;     // we have data
}
#endif // USE_TW1_FAST_ACCELEROMETER

#endif // USE_TWI_ON_ARM
// end of file - drv_twi.c

