/* cclock.h - RTXC Clock include file */

#include "rtxcopts.h"

#ifdef HAS_ALLOC_TIMER /* { */
#define NTMRS 6         /* number of timers */
#endif /* } HAS_ALLOC_TIMER */

#define CLKRATE 100     /* in Hertz */
#define CLKTICK 10      /* in millisec */

#ifdef HAS_ALLOC_TIMER /* { */
extern const int ntmrs ;
#endif /* } HAS_ALLOC_TIMER */

//extern const int clktick ;
//extern const int clkrate ;

//----------------------------------------------------------------------------
// Timer to use

#undef  USE_TIMER_PWM           // internal PWM as 'tick'
#define USE_TIMER_PIT           // internal PIT as 'tick'

// compatibility
extern time_t KS_inqtime(void) ;
extern void KS_deftime(time_t t) ;

void CLOCKinit(unsigned long desired_clock) ;

//----------------------------------------------------------------------------
// Watchdog control

#define USE_WATCHDOG    // define or undef
//#undef USE_WATCHDOG    // define or undef

#ifdef USE_WATCHDOG

#if defined(USE_LPC1788)
#define wdt_reset()     {LPC_WDT->FEED=0xAA; LPC_WDT->FEED=0x55;}
#endif  // defined(USE_LPC17XX)

#if defined(USE_LPC17XX)
#define wdt_reset()     {WDT->WDFEED=0xAA; WDT->WDFEED=0x55;}
#endif  // defined(USE_LPC17XX)

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
#define wdt_reset()     AT91C_BASE_WDTC->WDTC_WDCR = (0xa5<<24) | AT91C_WDTC_WDRSTT
#endif  // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_AT91SAM3S4)
#define wdt_reset()     WDT->WDT_CR = (0xa5<<24) | WDT_CR_WDRSTT
#endif  // defined(USE_AT91SAM3S4)

#else // USE_WATCHDOG

#define wdt_reset()  {}   /* do nothing */

#endif // USE_WATCHDOG

//----------------------------------------------------------------------------
// waste precise usec time

extern void tickwait(int udelay) ;
#ifdef CBUG
extern unsigned long long tickmeasure(unsigned long long lasttick) ;
extern long int random(void) ;
#endif // CBUG

// -----------------------------------------------------------------------------
// Led Blinker
//extern void Set_LedBlinker(int led, unsigned long mask, int period) ;
void Set_LedBlinker(int led, unsigned long mask, int period) ;

#ifdef USE_BEEPER_TIMER8
// -----------------------------------------------------------------------------
// Beeper
void timer8_beep_on(int freq) ;
void timer8_beep_off(void) ;
#endif // USE_BEEPER_TIMER8

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
//----------------------------------------------------------------------------
// AT91xx
// Interrupt priority level - Range: 7 - 0 (7=Hi, 0=Lo)

#define USART_3_INTERRUPT_LEVEL         7       // USART 3 (DBGU - no rx dma)
#define USART_0_INTERRUPT_LEVEL         6       // USART 0
#define USART_1_INTERRUPT_LEVEL         5       // USART 1
#define USART_2_INTERRUPT_LEVEL         5       // USART 2
#define TWI_INTERRUPT_LEVEL             4       // TWI (no DMA)
#define SPI_INTERRUPT_LEVEL             3       // SPI
#define CAN_INTERRUPT_LEVEL             3       // CAN (both 0 and 1)
#define ADC1_INTERRUPT_LEVEL            2       // ADC 1
#define USB_INTERRUPT_LEVEL             1       // USB
#define TC0_INTERRUPT_LEVEL             1       // Timer 0 (for ADC MUX)
#define PIT_INTERRUPT_PRIORITY          0       // System tick priority (PIT option)
#define PWM_INTERRUPT_PRIORITY          0       // System tick priority (PWM option)

#endif // defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#if defined(USE_LPC17XX) || defined(USE_LPC1788)
//----------------------------------------------------------------------------
// LPC17xx - LPC1788
// Interrupt priority level - Range: 31 - 0 (31=Lo, 0=Hi)

#define configPRIO_BITS                 5       // 32 priority levels

#define USART_3_INTERRUPT_LEVEL         5       // USART 3 (Max priority)
#ifdef CBUG
#define USART_0_INTERRUPT_LEVEL         6       // USART 0 // DEBUG pkt Minor of CAN
#else
#define USART_0_INTERRUPT_LEVEL         5       // USART 0
#endif
#define USART_1_INTERRUPT_LEVEL         5       // USART 1
#define USART_2_INTERRUPT_LEVEL         5       // USART 2
#ifdef USE_COM4_ON_ARM
#define USART_4_INTERRUPT_LEVEL         5       // USART 4
#endif //  USE_COM4_ON_ARM
#ifdef CRONO_REMOTE
#define CAN_INTERRUPT_LEVEL             4       // CAN (both 0 and 1)
#else
#define CAN_INTERRUPT_LEVEL             6       // CAN (both 0 and 1)
#endif
#define TWI_INTERRUPT_LEVEL             7       // TWI (no DMA)
#define SPI_INTERRUPT_LEVEL             8       // SPI
#define ADC1_INTERRUPT_LEVEL            9       // ADC 1
#define USB_INTERRUPT_LEVEL             10      // USB
#define TC0_INTERRUPT_LEVEL             11      // Timer 0 (for ADC MUX)
#define GPIO_INTERRUPT_LEVEL            12      // GPIO (moved etc)
#define DMA_INTERRUPT_LEVEL             16      // DMA (low priority)
#define KERNEL_LEVEL                    31      // KERNEL (Min priority)

// The lowest priority: SVC, PEND, TICK
#define configKERNEL_INTERRUPT_PRIORITY 	( KERNEL_LEVEL << (8 - configPRIO_BITS) )
// Priority 5 is maximum priority level
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( 5 << (8 - configPRIO_BITS) )

// Set a PendSV to request a context switch.
#define ASK_CONTEXTSWITCH                       ((SCB->ICSR) = 0x10000000)
#endif // defined(USE_LPC17XX) || defined(USE_LPC1788)

#if defined(USE_AT91SAM3S4)
//----------------------------------------------------------------------------
// AT91SAM3S4
// Interrupt priority level - Range: 15 - 0 (15=Lo, 0=Hi)

#define configPRIO_BITS                 4       // 16 priority levels

#define USART_3_INTERRUPT_LEVEL         5       // USART 3 (Max priority)
#define USART_0_INTERRUPT_LEVEL         5       // USART 0
#define USART_1_INTERRUPT_LEVEL         5       // USART 1
#define USART_2_INTERRUPT_LEVEL         5       // USART 2
#define CAN_INTERRUPT_LEVEL             6       // CAN (both 0 and 1)
#define TWI_INTERRUPT_LEVEL             7       // TWI (no DMA)
#define SPI_INTERRUPT_LEVEL             8       // SPI
#define ADC1_INTERRUPT_LEVEL            9       // ADC 1
#define USB_INTERRUPT_LEVEL             10      // USB
#define TC0_INTERRUPT_LEVEL             11      // Timer 0 (for ADC MUX)
#define GPIO_INTERRUPT_LEVEL            12      // GPIO (moved etc)
#define KERNEL_LEVEL                    15      // KERNEL (Min priority)

// The lowest priority: SVC, PEND, TICK
#define configKERNEL_INTERRUPT_PRIORITY 	( KERNEL_LEVEL << (8 - configPRIO_BITS) )
// Priority 5 is maximum priority level
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( 5 << (8 - configPRIO_BITS) )

// Set a PendSV to request a context switch.
#define ASK_CONTEXTSWITCH                       ((SCB->ICSR) = 0x10000000)
#endif // defined(USE_AT91SAM3S4)
