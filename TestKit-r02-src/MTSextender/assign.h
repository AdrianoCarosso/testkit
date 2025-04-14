//*****************************************************************************
//
//   MTS extender
//   Copyright (c) 1997-2008.
//   T.E.S.T. srl
//
// -----------------------------------------------------------------------
//   Hall of fame
//
//   1.00 16/Feb/2007 Start of work
//   1.01  8/Aug/2008 Modified X and D commands
//   1.02 16/Sep/2008 PB11 no longer pull-up
//                    PB9 output
//                    PA30 - PA31 output (no COM3)
//   1.03 28/Oct/2008 Can Tx
//   1.04 31/Oct/2008 Red LED user handling
//   1.05  3/Nov/2008 Fixed X command, digital output set
//   1.06  3/Dec/2008 Added parameters/SN on flash
//                    TWI-0 init bit 7 at 1
//                    Tick readout
//   1.07  3/Feb/2009 PIOB mas 0x24 at 0 at boot
//   1.08 17/Jul/2009 Added 'f' flag for CanStatus
//   1.09 29/Oct/2009 _FR_ Added 'U' command (util command)
//   1.10 04/Nov/2009 _FR_ Bug on 'U Ax' command (added help command)
//   1.11 16/Dec/2009 _FR_ Removed 1.015 AD coeff
//   1.12 17/Dec/2009 _FR_ For rs485 test COM0 at 9600
//   1.14 04/Feb/2010 _FR_ For rs485 test enabled COMx mode RS485
//   1.15 08/Feb/2010 _FR_ Modified ADC_MR for AD into drv_adc.c
//   1.16 20/Gen/2011 _FR_ PioB13 at start set to 0 (enable 3.3V to MTS CPU)
//
//   1.18 08/Lug/2011 _FR_ Can timer for emit data
//

// Available commands are:
// Ax <BaudRate>        							Ser CANx baud rate
// Bx <BaudRate>,<Par>,<Bits>,<Stop>,<Handshake>    Set COMx options
// Cx <HexData>         							Send to COMx port binary values from <HexData> string
// D                    							Dump <Digital>;<Analog>;<Counter>
// Exy <HexData>                                	Emit CANx MAILy
// F												Check CAN flags
// H 												This help
// L <HexMask>,<period_ms>      					Set Red Led Mask and period in [ms]
// Mxy <Mask>,<Addr>,<Extended>,<Period>        	Set CANx MAILy (Mask = ffffffff means TX)
// S 												Software upgrade
// T <Interval>         							Set spontaneus Dump command every <Interval> msec (0=never)
// U A[V|C]x/I[V|C]x/V/Px										Fast commands:
//													Ax: Anx (1-6 if x=0 all to 0)
//													Ix: Inx (1-8 if x=0 all to 1)
//													Vx Vext 1 or 0
//													Px Presence 1 or 0
// V                    							Version
// X <Port>,<HexMask>,<HexVal> 						Set digital output <HexMask> of <Port> to <HexVal>
//
// Every command is composed by a <CR> terminated string

// Examples:
// V<CR> --> V=1.00
// D<CR> --> D=ffdfffdf,1b003ffd,ffdfafdf,1b001cfd;3ff,3ff,0,0,263,34c,3cb,3ff,3ff,3ff,3ff,3f7;0,0
// C3=4142430d0a<CR> --> send ABC<CR><LF> to COM3
// T 1000<CR> --> Spontaneous D command every second
// X 0,1,1<CR> --> Set register 0 (PORT A), first digital output at one
// B3=9600,N,8,1,H<CR> --> Set COM3 to 9600 baud, no parity, 1 stop, Hardware Handshake
//
// Example of CAN rx
// A0=50000<CR> --> Set CAN0 at 50k
// M02=1FF00,1C030,E,1000 --> Return CAN0 MAIL2 (Extended) every 1000 ms
//
// Spontaneous uploaded strings may be:
// D=<Digital>;<Analog>;<Counter><CR> --> in case of T command
// Cx=<HexData> --> when data is received from COMx
//
// Example of CAN tx
// A0=50000
// M01=ffffffff,1C030,E,0
// E01=aa5566
//
// New software may be downloaded directly in S-record format

// ****************************************************************************

// prevent re-include
#ifndef _ASSIGN_H
#define _ASSIGN_H

// ****************************************************************************
// who we are

#define SOFTREL         1       // current release.subrel
#define SUBSREL        19

#include "_AT91SAM7A3.h"

#define USE_REAL_BOARD
#define TOOLKIT_V2
// -----------------------------------------------------------------------
// Performance options

#undef USE_PERFORMANCE

#if defined(CBUG) && defined(USE_PERFORMANCE) // not for production
#undef USE_PERFORMANCE_EVALUATION
#endif // CBUG

// ----------------------------------------------------------------------------
// LOW POWER options
#undef USE_LOW_POWER

// ----------------------------------------------------------------------------
// LED options
#define USE_LED_BLINKER

// ----------------------------------------------------------------------------
// SPI options
#define USE_SPI_ON_ARM
#define USE_PARAMETERS_ON_FLASH
#define USE_SERIALFLASH_ON_ARM
#define USE_FLASH_DATA

// ----------------------------------------------------------------------------
// USB options
// This definition is also used in csema.c/h and cqueue.c/h and cclock.c/h
#define USE_USB_ON_ARM

// ----------------------------------------------------------------------------
// TWI options
// This definition is also used in csema.c/h

#define USE_TWI_ON_ARM
#undef  USE_PARAMETERS_ON_TWI

#undef  USE_ATMEL_TWI           // I/O with Atmel

#define USE_RTC_AT91SAM7        // RTC chip used
#undef USE_RTC_TWI_DS1337       // RTC chip NOT used
//#define DS1337_ADDR     0x68
#define PORT_TW1        2       // external digital I/O PORT -TW1-
#define PORT_TW2        3       // external digital I/O PORT -TW2-

#ifdef TOOLKIT_V2
#define USE_PCAL9555A_TW1         // I/O n. 1
#define PCAL9555A_ADDR 0x20

#define USE_PCAL9555A_TW2         // I/O n. 2
#define PCAL9555A_ADDR2 0x21

#define PCAL9555A_ADDR3 0x22
#define PORT_TW3        4       // external digital I/O PORT -TW2-

#else
#define USE_MAX7324_TW1         // I/O n. 1
#define MAX7324_R1_ADDR 0x68
#define MAX7324_W1_ADDR 0x58

#define USE_MAX7324_TW2         // I/O n. 2
#define MAX7324_R2_ADDR 0x69
#define MAX7324_W2_ADDR 0x59

#endif


#define CNT_PPS         0
#define CNT_ODOMETER    1

// ----------------------------------------------------------------------------
// ADC options
// This definition is also used in csema.c/h

#undef USE_ADC_FAST_ON_ARM
#define USE_ADC_MUX_ON_ARM

// ----------------------------------------------------------------------------
// CAN options
// This definition is also used in csema.c/h
#define USE_CAN_ON_ARM
#ifdef USE_CAN_ON_ARM
#define USE_CAN_TRANSMIT_ON_ARM
#endif // USE_CAN_ON_ARM

// ----------------------------------------------------------------------------
// CLOCK options

// Master Clock
#define EXTERNAL_CLOCK          16000000   // Exetrnal oscillator MAINCK

// ----------------------------------------------------------------------------
// EXTAPI options
#define USE_PKTMEMBUF

// ----------------------------------------------------------------------------
//	I/O port definition
//	let's change port assignment whenever we want

extern unsigned short com0err ; // error from drivers
extern unsigned short com1err ;
extern unsigned short com2err ;
extern unsigned short com3err ;
#ifdef USE_USB_ON_ARM
extern unsigned short usberr ;
#endif // USE_USB_ON_ARM

// deifne in order to use serial line, undef for USB
#undef USE_TASK0_SERIAL

#ifdef USE_TASK0_SERIAL

#define TK0IPORT COM1IQ 	// Task 0 Input port
#define TK0OPORT COM1OQ 	// Task 0 Output port (monitor)
#define TK0ISEM  PORT1SEM	// Task 0 input port semaphore
#define TK0PERR  com1err	// Task 0 input port error flag

#else  // USE_TASK0_SERIAL

#define TK0IPORT USBIQ 	        // Task 0 Input port
#define TK0OPORT USBOQ 	        // Task 0 Output port (monitor)
#define TK0ISEM  PORTUSEM	// Task 0 input port semaphore
#define TK0PERR  usberr	        // Task 0 input port error flag

#endif // USE_TASK0_SERIAL

// Monitor port assignment
#ifdef USE_TASK0_SERIAL

#ifdef USE_REAL_BOARD
#define MONITOPORT COM1OQ
#define MONITIPORT COM1IQ
#endif // USE_REAL_BOARD

#define MONIUNBLOCK { KS_unblock(UARTDRV, UARTDRV) ; }

#else  // USE_TASK0_SERIAL
#define MONITOPORT USBOQ
#define MONITIPORT USBIQ
#define MONIUNBLOCK { KS_unblock(USBTASK, USBTASK) ; }

//#define MONITOPORT COM1OQ
//#define MONITIPORT COM1IQ
//#define MONIUNBLOCK { KS_unblock(UARTDRV, UARTDRV) ; }

#endif // USE_TASK0_SERIAL

// modem
#define MODEMOQ	 COM1OQ
#define MODEMIQ	 COM1IQ

// -----------------------------------------------------------------------
// Task definitions

#define NAMMAX 8

enum TASKLIST {
FIRSTTASK = 0,  // dummy, never used

#ifdef CBUG
RTXCBUG,	// RTXCbug
#endif // CBUG

UARTDRV,        // COMs task RTX

#ifdef USE_USB_ON_ARM
USBTASK,        // USB task
#endif // USE_USB_ON_ARM
#ifdef USE_ADC_FAST_ON_ARM
ADCTASK,        // ADC task
#endif // USE_ADC_FAST_ON_ARM

TK0EXTENDER,    // Task 0 - extender

MAXTASKS        // evaluate total number
} ;

#define NTASKS  (MAXTASKS-1)

// -----------------------------------------------------------------------

#define LU_TOT		1	// Number of LUs into system

// -----------------------------------------------------------------------
// Configure hardware I/O

#define PORT_PIOA       0       // internal digital I/O PORT -A-
#define PORT_PIOB       1       // internal digital I/O PORT -B-

#ifdef REAL_GYRO
#define PIO_GYRO	AT91C_PIO_PB20
#else
#define PIO_GYRO 	0          // no present
#endif                         

#ifdef USE_REAL_BOARD
#define LED1            0x00000010
#define LED2            0x00000010
#define LED3            0x00000010
#define LED4            0x00000010
#define LED_MASK        (LED1 | LED2 | LED3 | LED4)
#define BPIOA_LEDR      18
#define PIOA_LEDR       (1<<BPIOA_LEDR) // red led

#define PIOA_LED        (1<<4)          // Led
#define PIOA_STAYON     (1<<18)         // set to zero in order to turn off
#define PIOA_MUX0       (1<<19)         // MUX address 0
#define PIOA_MUX1       (1<<20)         // MUX address 1
#define PIOA_MUX2       (1<<21)         // MUX address 2
#define PIOA_GPSDIFF    (1<<23)         // change GPS input port

#define PIOA_MASK       (PIOA_LED | PIOA_LEDR | PIOA_STAYON | PIOA_MUX0 | PIOA_MUX1 | PIOA_MUX2 | PIOA_GPSDIFF | \
                        (1<<30) | (1<<31) /* COM3 not used */ \
                        )
#define PIOA_PMASK		PIOA_MASK
// Input without pull-up
#define PIOA_INPMASK (AT91C_PIO_PA2  | AT91C_PIO_PA6  | AT91C_PIO_PA7  | \
                      AT91C_PIO_PA9  | AT91C_PIO_PA25 |					 \
                      AT91C_PIO_PA26 | AT91C_PIO_PA28 | AT91C_PIO_PA30 )

#define PIOB_SRV_ON             (1<<0)          // enable RS232 drivers
#define PIOB_GSM_SOFT_ON        (1<<1)          // GSM soft on
#define PIOB_AUDIO_OFF          (1<<2)          // GSM Audio off
#define PIOB_GSM_RST_OFF        (1<<3)          // GSM Reset off
#define PIOB_CONSPWR_OFF        (1<<4)          // Console power off
#define PIOB_GPS_OFF            (1<<5)          // GPS power off
#define PIOB_ACC_BIT0           (1<<6)          // Accelerometer range selection bit 0
#define PIOB_ACC_BIT1           (1<<7)          // Accelerometer range selection bit 1
#define PIOB_CAN_OFF            (1<<8)          // CAN driver disable
#define PIOB_MTS3V_OFF      (1<<13)         	// 3.3V of MTS with CORTEX 
#define PIOB_RELE_OFF           (1<<29)         // RELE' disable

#define PIOB_MASK       (PIOB_SRV_ON | PIOB_GSM_SOFT_ON | PIOB_AUDIO_OFF | PIOB_GSM_RST_OFF | \
                         PIOB_CONSPWR_OFF | PIOB_GPS_OFF | PIOB_ACC_BIT0 | PIOB_ACC_BIT1 |    \
                         PIOB_CAN_OFF | PIOB_MTS3V_OFF | PIOB_RELE_OFF | \
                         (1<<9) /* only for extender */ \
                         )
#define PIOB_PMASK		PIOB_MASK

// Input without pull-up	// (AT91C_PIO_PB11)?
#define PIOB_INPMASK	( AT91C_PIO_PB14 | AT91C_PIO_PB15 | AT91C_PIO_PB16 | \
                          AT91C_PIO_PB17 | AT91C_PIO_PB18 | AT91C_PIO_PB19 | PIO_GYRO | \
                          AT91C_PIO_PB21 | AT91C_PIO_PB22 | AT91C_PIO_PB23 | AT91C_PIO_PB25 | \
                          AT91C_PIO_PB26 | AT91C_PIO_PB28 )
#endif // USE_REAL_BOARD

#ifdef USE_SPI_ON_ARM
// ----------------------------- Flash map -------------------------------
// External SPI Flash memory
//
// Area         Size      From      To
// -----------------------------------
// Upgrade      256k    000000  03ffff
// Transaction  256k    040000  07ffff
// Parameters   128k    080000  09ffff
// Log           96k
// Crash         64K
// S.M.          64k
// Target        64k
// Free

// Upgrade
#define CODEFLASH_BEGIN 0x00000
#define CODEFLASH_END   0x3ffff

// Transactions
#define USE_TRANSACTIONS_ON_ARM
#define FLASH_BEGIN     0x40000
#define FLASH_END       0x7ffff
#define FLASH_SSIZE     512

// Parameters
#define PARAM_BEGIN     0x80000
#define PARAM_END       0x9ffff
#define PARAM_SSIZE     512

#define CODEFLASH_START CODEFLASH_BEGIN
#define CODEFLASH_STOP  CODEFLASH_END

#define FLASH_START     FLASH_BEGIN
#define FLASH_STOP      FLASH_END
#define FLASH_TSSIZE    (8*512)        // _BM_ 28/8/2008
#define FLASH_PAGESIZE  FLASH_SSIZE
#define FLASH_TOTSIZE	(FLASH_STOP-FLASH_START+1)


#define PARAM_START PARAM_BEGIN
#define PARAM_STOP  PARAM_END

#endif // USE_SPI_ON_ARM

// Internal Flash Code
#define RUNCODE_START 	((unsigned long)(AT91C_IFLASH))	  	// Internal FLASH base address
#define RUNCODE_SIZE    AT91C_IFLASH_SIZE	// Internal FLASH size in byte (256 Kbytes)

#define AD_ADC0_MASK  0xff


// -----------------------------------------------------------------------
#ifdef USE_CAN_ON_ARM
// From drv_can.c
extern unsigned char CanStatus[2] ;	// Diag of CAN
#define CAN_NODATA		0x01	// No data from CAN (x sec of timeout)
#define CAN_MOBFULL		0x08	// Insufficient slots (NB_MOB)
#define CAN_BADRATE		0x20	// panic(11) (cannot configure)
#define CAN_OVFRATE		0x40	// BRP > 127) panic(12)
#define CAN_OFF 		0xff	// Baud rate = 0
#endif // USE_CAN_ON_ARM

#endif // _ASSIGN_H

