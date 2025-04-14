/* csema.h - RTXC Semaphore include file */

#include "extapi.h"

#define NAMMAX 8

enum SEMAPHORELIST {
FIRSTSEMA = 0,  // dummy, never used

CBUGSEMA,       // RTXCBug entry

COM0ISEM,       // COM 0 input
COM0OSEM,       // COM 0 output
COM0QSEM,       // COM 0 queue
COM1ISEM,       // COM 1 input
COM1OSEM,       // COM 1 output
COM1QSEM,       // COM 1 queue
COM2ISEM,       // COM 2 input
COM2OSEM,       // COM 2 output
COM2QSEM,       // COM 2 queue
COM3ISEM,       // COM 3 input
COM3OSEM,       // COM 3 output
COM3QSEM,       // COM 3 queue

#ifdef USE_COM4_ON_ARM
COM4ISEM,       // COM 4 input
COM4OSEM,       // COM 4 output
COM4QSEM,       // COM 4 queue
#endif // USE_COM4_ON_ARM

SPISEM,         // SPI end of activity

TWI0SEM,        // TWI 0 end of activity
#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
TWI1SEM,        // TWI 1 end of activity
#endif // USE_TWI1_AUXILIARY
#ifdef USE_TWI2_ON_ARM
TWI2SEM,
#endif //  USE_TWI2_ON_ARM
#ifdef USE_TWI0_SLAVE
TWI0SLSM,
#endif // USE_TWI0_SLAVE
#ifdef USE_TWI1_SLAVE
TWI1SLSM,
#endif // USE_TWI1_SLAVE
#ifdef USE_TWI2_SLAVE
TWI2SLSM,
#endif // USE_TWI2_SLAVE

#ifdef USE_ADC_FAST_ON_ARM
ADCSEM,         // ADC buffer
#endif // USE_ADC_FAST_ON_ARM

CAN0SEM,        // CAN 0 mailbox
CAN1SEM,        // CAN 1 mailbox

#ifdef USE_AT91SAM7A3
USBSEM0,        // USB activity Endpoint 0
USBSEM1,        // USB activity Endpoint 1
USBSEM2,        // USB activity Endpoint 2
#endif // USE_AT91SAM7A3
USBQSEM,        // USB queue

#if defined(USE_DMA_NAND)
NANDSEM,        // NAND DMA transfer
#endif // defined(USE_DMA_NAND)

#if defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)
EFCSEM,         // EFC queue
#endif // defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)

PORT0SEM,       // Peripheral 0 input
PORT1SEM,       // Peripheral 1 input
PORT2SEM,       // Peripheral 2 input
PORT3SEM,       // Peripheral 3 input

#ifdef USE_COM4_ON_ARM
PORT4SEM,       // Peripheral 4 input
#endif // USE_COM4_ON_ARM

PORTUSEM,       // Peripheral U input

LU0TSEM,        // LU 0 Cyclic timer
LU1TSEM,        // LU 1 Cyclic timer
LU2TSEM,        // LU 2 Cyclic timer
LU3TSEM,        // LU 3 Cyclic timer
LU4TSEM,        // LU 4 Cyclic timer

LU0QSEM,        // LU 0 Queue
LU1QSEM,        // LU 1 Queue
LU2QSEM,        // LU 2 Queue
LU3QSEM,        // LU 3 Queue
LU4QSEM,        // LU 4 Queue

MAXSEMAS        // evaluate total number
} ;

#define NSEMAS (MAXSEMAS-1)

extern const SEMA nsemas ;

extern const int siglistsize ;

extern SEMA siglist[] ;

extern SSTATE semat[1+NSEMAS] ;

#ifdef CBUG
extern const char semakname[1+NSEMAS][NAMMAX+1] ;
#endif // CBUG

