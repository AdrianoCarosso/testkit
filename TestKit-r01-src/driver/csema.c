/* csema.c - RTXC Semaphore definitions - ARM */

#include "typedef.h"
#include "rtxstruc.h"

#include "extapi.h"

#include "csema.h"

#define SIGLISTSIZE 32 /* sema list size */

const SEMA nsemas = NSEMAS;

const int siglistsize = SIGLISTSIZE;

SEMA siglist[SIGLISTSIZE];/* sema list */

SSTATE semat[1+NSEMAS];

#ifdef CBUG
const char semakname[1+NSEMAS][NAMMAX+1] =
{
   " ",
   "CBUGSEMA",

   "COM0ISEM",
   "COM0OSEM",
   "COM0QSEM",
   "COM1ISEM",
   "COM1OSEM",
   "COM1QSEM",
   "COM2ISEM",
   "COM2OSEM",
   "COM2QSEM",
   "COM3ISEM",
   "COM3OSEM",
   "COM3QSEM",

#ifdef USE_COM4_ON_ARM
   "COM4ISEM",
   "COM4OSEM",
   "COM4QSEM",
#endif // USE_COM4_ON_ARM

#ifdef USE_SPI_ON_ARM
   "SPISEM",
#endif // USE_SPI_ON_ARM

#ifdef USE_TWI_ON_ARM
   "TWI0SEM",
#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
	"TWI1SEM",
#endif // (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )
#ifdef USE_TWI2_ON_ARM
	"TWI2SEM",
#endif //  USE_TWI2_ON_ARM
#ifdef USE_TWI0_SLAVE
   "TWI0SLSM",
#endif // USE_TWI0_SLAVE
#ifdef USE_TWI1_SLAVE
   "TWI1SLSM",
#endif // USE_TWI1_SLAVE
#ifdef USE_TWI2_SLAVE
   "TWI2SLSM",
#endif // USE_TWI2_SLAVE
#endif // USE_TWI_ON_ARM

#ifdef USE_ADC_FAST_ON_ARM
   "ADCSEM",
#endif // USE_ADC_FAST_ON_ARM

#ifdef USE_CAN_ON_ARM
   "CAN0SEM",
   "CAN1SEM",
#endif // USE_CAN_ON_ARM

#ifdef USE_USB_ON_ARM
#ifdef USE_LPC1788
   "USBTIMER",          // USB periodic task call
#endif // USE_LPC1788
#ifdef USE_AT91SAM7A3
   "USBSEM0",
   "USBSEM1",
   "USBSEM2",
#endif // USE_AT91SAM7A3
   "USBQSEM",
#endif // USE_USB_ON_ARM

#if defined(USE_DMA_NAND)
   "NANDSEM",
#endif // defined(USE_DMA_NAND)

#if defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)
   "EFCSEM",
#endif // defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)

   "PORT0SEM",
   "PORT1SEM",
   "PORT2SEM",
   "PORT3SEM",

#ifdef USE_COM4_ON_ARM
   "PORT4SEM",
#endif // USE_COM4_ON_ARM

#ifdef USE_USB_ON_ARM
   "PORTUSEM",
#endif // USE_USB_ON_ARM

   "LU0TSEM",
   "LU1TSEM",
   "LU2TSEM",
   "LU3TSEM",
   "LU4TSEM",

   "LU0QSEM",
   "LU1QSEM",
   "LU2QSEM",
   "LU3QSEM",
   "LU4QSEM",
};
#endif // CBUG

