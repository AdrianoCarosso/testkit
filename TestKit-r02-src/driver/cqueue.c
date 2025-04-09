/* cqueue.c - RTXC Queue definitions */

#include "typedef.h"
#include "rtxstruc.h"

#include "extapi.h"             // must be before cqueue.h

#include "cqueue.h"

#ifdef GEMHH
#define Q_COM0IQ_WD 1           // COM0IQ   - COM 0 input (at 115200)
#define Q_COM0IQ_DP 64

#define Q_COM0OQ_WD 1           // COM0OQ   - COM 0 output
#define Q_COM0OQ_DP 16

#define Q_COM1IQ_WD 1           // COM1IQ   - COM 1 input
#define Q_COM1IQ_DP 64

#define Q_COM1OQ_WD 1           // COM1OQ   - COM 1 output
#define Q_COM1OQ_DP 16

#define Q_COM2IQ_WD 1           // COM2IQ   - COM 2 input
#define Q_COM2IQ_DP 1024

#define Q_COM2OQ_WD 1           // COM2OQ   - COM 2 output
#define Q_COM2OQ_DP 128

#define Q_COM3IQ_WD 1           // COM3IQ   - COM 3 input
#define Q_COM3IQ_DP 64

#define Q_COM3OQ_WD 1           // COM3OQ   - COM 3 output
#define Q_COM3OQ_DP 16

#ifdef USE_COM4_ON_ARM
#define Q_COM4IQ_WD 1           // COM4IQ   - COM 4 input
#define Q_COM4IQ_DP 64

#define Q_COM4OQ_WD 1           // COM4OQ   - COM 4 output
#define Q_COM4OQ_DP 16
#endif // USE_COM4_ON_ARM

#ifdef USE_USB_ON_ARM
#define Q_USBIQ_WD 1            // USBIQ    - USB input
#define Q_USBIQ_DP 3072		// 2304 = 10243*2+256

#define Q_USBOQ_WD 1            // USBOQ    - USB output
#define Q_USBOQ_DP 128
#endif // USE_USB_ON_ARM

#else // GEMHH **************************************

#define Q_COM0IQ_WD 1           // COM0IQ   - COM 0 input (at 115200)
#if defined(USE_LPC1788)
#define Q_COM0IQ_DP 1024
#else
#define Q_COM0IQ_DP 512
#endif

#define Q_COM0OQ_WD 1           // COM0OQ   - COM 0 output
#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#define Q_COM0OQ_DP 128
#else
#define Q_COM0OQ_DP 64
#endif

#define Q_COM1IQ_WD 1           // COM1IQ   - COM 1 input
#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512) || defined(USE_AT91SAM3S4) || defined(USE_LPC17XX) //|| defined(USE_LPC1788) 
#define Q_COM1IQ_DP 1152		//128
#elif defined(USE_LPC1788)
#define Q_COM1IQ_DP 2304		//128
#else
#define Q_COM1IQ_DP 256			//128
#endif

#define Q_COM1OQ_WD 1           // COM1OQ   - COM 1 output
#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#define Q_COM1OQ_DP 64
#else
#define Q_COM1OQ_DP 128
#endif

#define Q_COM2IQ_WD 1           // COM2IQ   - COM 2 input
#if defined(USE_LPC1788)
#define Q_COM2IQ_DP 1024
#else
#define Q_COM2IQ_DP 256			//128
#endif

#define Q_COM2OQ_WD 1           // COM2OQ   - COM 2 output
#define Q_COM2OQ_DP 64			// 16

#define Q_COM3IQ_WD 1           // COM3IQ   - COM 3 input
#define Q_COM3IQ_DP 256

#define Q_COM3OQ_WD 1           // COM3OQ   - COM 3 output
#define Q_COM3OQ_DP 16

#ifdef USE_COM4_ON_ARM
#define Q_COM4IQ_WD 1           // COM4IQ   - COM 4 input
#define Q_COM4IQ_DP 256

#define Q_COM4OQ_WD 1           // COM4OQ   - COM 4 output
#define Q_COM4OQ_DP 16
#endif // USE_COM4_ON_ARM

#ifdef USE_USB_ON_ARM
#define Q_USBIQ_WD 1            // USBIQ    - USB input
#define Q_USBIQ_DP 96

#define Q_USBOQ_WD 1            // USBOQ    - USB output
#define Q_USBOQ_DP 96
#endif // USE_USB_ON_ARM

#endif // GEMHH

#define Q_LU0Q_WD 4             // LU0Q     - LU 0 Queue
#define Q_LU0Q_DP 16

#define Q_LU1Q_WD 4             // LU1Q     - LU 1 Queue
#define Q_LU1Q_DP 16

#define Q_LU2Q_WD 4             // LU2Q     - LU 2 Queue
#define Q_LU2Q_DP 16

#define Q_LU3Q_WD 4             // LU3Q     - LU 3 Queue
#define Q_LU3Q_DP 16

#define Q_LU4Q_WD 4             // LU4Q     - LU 4 Queue
#define Q_LU4Q_DP 16

const QUEUE nqueues = NQUEUES;

static char q_COM0IQ_[Q_COM0IQ_DP][Q_COM0IQ_WD] ;
static char q_COM0OQ_[Q_COM0OQ_DP][Q_COM0OQ_WD] ;
static char q_COM1IQ_[Q_COM1IQ_DP][Q_COM1IQ_WD] ;
static char q_COM1OQ_[Q_COM1OQ_DP][Q_COM1OQ_WD] ;
static char q_COM2IQ_[Q_COM2IQ_DP][Q_COM2IQ_WD] ;
static char q_COM2OQ_[Q_COM2OQ_DP][Q_COM2OQ_WD] ;
static char q_COM3IQ_[Q_COM3IQ_DP][Q_COM3IQ_WD] ;
static char q_COM3OQ_[Q_COM3OQ_DP][Q_COM3OQ_WD] ;
#ifdef USE_COM4_ON_ARM
static char q_COM4IQ_[Q_COM4IQ_DP][Q_COM4IQ_WD] ;
static char q_COM4OQ_[Q_COM4OQ_DP][Q_COM4OQ_WD] ;
#endif // USE_COM4_ON_ARM
#ifdef USE_USB_ON_ARM
static char q_USBIQ_[Q_USBIQ_DP][Q_USBIQ_WD] ;
static char q_USBOQ_[Q_USBOQ_DP][Q_USBOQ_WD] ;
#endif // USE_USB_ON_ARM
static char q_LU0Q_[Q_LU0Q_DP][Q_LU0Q_WD] ;
static char q_LU1Q_[Q_LU1Q_DP][Q_LU1Q_WD] ;
static char q_LU2Q_[Q_LU2Q_DP][Q_LU2Q_WD] ;
static char q_LU3Q_[Q_LU3Q_DP][Q_LU3Q_WD] ;
static char q_LU4Q_[Q_LU4Q_DP][Q_LU4Q_WD] ;

QHEADER qheader[1+NQUEUES];

const QKHEADER qkkheader[1+NQUEUES] =
{
   { (char *)0, (size_t)0, 0 }, /* not used */
   { &q_COM0IQ_[0][0], (size_t)Q_COM0IQ_WD, Q_COM0IQ_DP },
   { &q_COM0OQ_[0][0], (size_t)Q_COM0OQ_WD, Q_COM0OQ_DP },
   { &q_COM1IQ_[0][0], (size_t)Q_COM1IQ_WD, Q_COM1IQ_DP },
   { &q_COM1OQ_[0][0], (size_t)Q_COM1OQ_WD, Q_COM1OQ_DP },
   { &q_COM2IQ_[0][0], (size_t)Q_COM2IQ_WD, Q_COM2IQ_DP },
   { &q_COM2OQ_[0][0], (size_t)Q_COM2OQ_WD, Q_COM2OQ_DP },
   { &q_COM3IQ_[0][0], (size_t)Q_COM3IQ_WD, Q_COM3IQ_DP },
   { &q_COM3OQ_[0][0], (size_t)Q_COM3OQ_WD, Q_COM3OQ_DP },
#ifdef USE_COM4_ON_ARM
   { &q_COM4IQ_[0][0], (size_t)Q_COM4IQ_WD, Q_COM4IQ_DP },
   { &q_COM4OQ_[0][0], (size_t)Q_COM4OQ_WD, Q_COM4OQ_DP },
#endif // USE_COM4_ON_ARM
#ifdef USE_USB_ON_ARM
   { &q_USBIQ_[0][0],  (size_t)Q_USBIQ_WD,  Q_USBIQ_DP  },
   { &q_USBOQ_[0][0],  (size_t)Q_USBOQ_WD,  Q_USBOQ_DP  },
#endif // USE_USB_ON_ARM
   { &q_LU0Q_[0][0],   (size_t)Q_LU0Q_WD,   Q_LU0Q_DP   },
   { &q_LU1Q_[0][0],   (size_t)Q_LU1Q_WD,   Q_LU1Q_DP   },
   { &q_LU2Q_[0][0],   (size_t)Q_LU2Q_WD,   Q_LU2Q_DP   },
   { &q_LU3Q_[0][0],   (size_t)Q_LU3Q_WD,   Q_LU3Q_DP   },
   { &q_LU4Q_[0][0],   (size_t)Q_LU4Q_WD,   Q_LU4Q_DP   },
} ;

#ifdef CBUG
const char queuekname[1+NQUEUES][NAMMAX+1] =
{
   " ",
   "COM0IQ",
   "COM0OQ",
   "COM1IQ",
   "COM1OQ",
   "COM2IQ",
   "COM2OQ",
   "COM3IQ",
   "COM3OQ",
#ifdef USE_COM4_ON_ARM
   "COM4IQ",
   "COM4OQ",
#endif // USE_COM4_ON_ARM
#ifdef USE_USB_ON_ARM
   "USBIQ",
   "USBOQ",
#endif // USE_USB_ON_ARM
   "LU0Q",
   "LU1Q",
   "LU2Q",
   "LU3Q",
   "LU4Q",
};
#endif // CBUG

