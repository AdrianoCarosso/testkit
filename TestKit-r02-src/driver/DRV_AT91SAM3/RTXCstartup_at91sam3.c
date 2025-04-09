//*****************************************************************************
// RTXC startup for AT91SAM3
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//
//
//*****************************************************************************
#include <assign.h>
#include <cclock.h>

#define WEAK __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((weak, alias (#f)))

//*****************************************************************************
//
// Forward declaration of the default handlers.
//
//*****************************************************************************
void Reset_Handler(void) __attribute__ ((noreturn)) ;
void ResetISR(void) ALIAS(Reset_Handler);
static void NMI_Handler(void);
static void HardFault_Handler(void);
static void MemManage_Handler(void);
static void BusFault_Handler(void);
static void UsageFault_Handler(void);
static void DebugMon_Handler(void);

//*****************************************************************************
//
// Forward declaration of the specific IRQ handlers. These are aliased
// to the IntDefaultHandler, which is a 'forever' loop. When the application
// defines a handler (with the same name), this will automatically take
// precedence over these weak definitions
//
//*****************************************************************************
void SUPC_IrqHandler(void) ALIAS(IntDefaultHandler);    // 0  Supply Controller
void RSTC_IrqHandler(void) ALIAS(IntDefaultHandler);    // 1  Reset Controller
void RTC_IrqHandler(void) ALIAS(IntDefaultHandler);     // 2  Real Time Clock
void RTT_IrqHandler(void) ALIAS(IntDefaultHandler);     // 3  Real Time Timer
void WDT_IrqHandler(void) ALIAS(IntDefaultHandler);     // 4  Watchdog Timer
void PMC_IrqHandler(void) ALIAS(IntDefaultHandler);     // 5  PMC
void EEFC_IrqHandler(void) ALIAS(IntDefaultHandler);    // 6  EEFC
void IrqHandler7NotUsed(void) ALIAS(IntDefaultHandler); // 7  Reserved
void UART0_IrqHandler(void) ALIAS(IntDefaultHandler);   // 8  UART0
void UART1_IrqHandler(void) ALIAS(IntDefaultHandler);   // 9  UART1
void SMC_IrqHandler(void) ALIAS(IntDefaultHandler);     // 10 SMC
void PIOA_IrqHandler(void) ALIAS(IntDefaultHandler);    // 11 Parallel IO Controller A
void PIOB_IrqHandler(void) ALIAS(IntDefaultHandler);    // 12 Parallel IO Controller B
void PIOC_IrqHandler(void) ALIAS(IntDefaultHandler);    // 13 Parallel IO Controller C
void USART0_IrqHandler(void) ALIAS(IntDefaultHandler);  // 14 USART 0
void USART1_IrqHandler(void) ALIAS(IntDefaultHandler);  // 15 USART 1
void IrqHandler16NotUsed(void) ALIAS(IntDefaultHandler);// 16 Reserved
void IrqHandler17NotUsed(void) ALIAS(IntDefaultHandler);// 17 Reserved
void MCI_IrqHandler(void) ALIAS(IntDefaultHandler);     // 18 MCI
void TWI0_IrqHandler(void) ALIAS(IntDefaultHandler);    // 19 TWI 0
void TWI1_IrqHandler(void) ALIAS(IntDefaultHandler);    // 20 TWI 1
void SPI_IrqHandler(void) ALIAS(IntDefaultHandler);     // 21 SPI
void SSC_IrqHandler(void) ALIAS(IntDefaultHandler);     // 22 SSC
void TC0_IrqHandler(void) ALIAS(IntDefaultHandler);     // 23 Timer Counter 0
void TC1_IrqHandler(void) ALIAS(IntDefaultHandler);     // 24 Timer Counter 1
void TC2_IrqHandler(void) ALIAS(IntDefaultHandler);     // 25 Timer Counter 2
void TC3_IrqHandler(void) ALIAS(IntDefaultHandler);     // 26 Timer Counter 3
void TC4_IrqHandler(void) ALIAS(IntDefaultHandler);     // 27 Timer Counter 4
void TC5_IrqHandler(void) ALIAS(IntDefaultHandler);     // 28 Timer Counter 5
void ADC_IrqHandler(void) ALIAS(IntDefaultHandler);     // 29 ADC controller
void DAC_IrqHandler(void) ALIAS(IntDefaultHandler);     // 30 DAC controller
void PWM_IrqHandler(void) ALIAS(IntDefaultHandler);     // 31 PWM
void CRCCU_IrqHandler(void) ALIAS(IntDefaultHandler);   // 32 CRC Calculation Unit
void ACC_IrqHandler(void) ALIAS(IntDefaultHandler);     // 33 Analog Comparator
void USBD_IrqHandler(void) ALIAS(IntDefaultHandler);    // 34 USB Device Port
void IrqHandler35NotUsed(void) ALIAS(IntDefaultHandler);// 35 not used


extern void xPortSysTickHandler(void) ;
extern void xPortPendSVHandler(void) ;
extern void vPortSVCHandler(void) ;


//*****************************************************************************
//
// The entry point for the C++ library startup
//
//*****************************************************************************
extern WEAK void __libc_init_array(void);

//*****************************************************************************
//
// The entry point for the application.
// __main() is the entry point for redlib based applications
// main() is the entry point for newlib based applications
//
//*****************************************************************************
extern WEAK void __main(void);
extern WEAK void main(void);
extern void maintask(void);

//*****************************************************************************
//
// External declaration for the pointer to the stack top from the Linker Script
//
//*****************************************************************************
extern void _vStackTop;
//*****************************************************************************
//
// The vector table.
// This relies on the linker script to place at correct location in memory.
//
//*****************************************************************************
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) =
{
    // Core Level - CM3
    (void *)&_vStackTop,                // The initial stack pointer
    Reset_Handler,                      // The reset handler
    NMI_Handler,                        // The NMI handler
    HardFault_Handler,                  // The hard fault handler
    MemManage_Handler,                  // The MPU fault handler
    BusFault_Handler,                   // The bus fault handler
    UsageFault_Handler,                 // The usage fault handler
    0,                                  // Reserved
    0,                                  // Reserved
    0,                                  // Reserved
    0,                                  // Reserved
    vPortSVCHandler,                    // SVCall handler
    DebugMon_Handler,                   // Debug monitor handler
    0,                                  // Reserved
    xPortPendSVHandler,                 // The PendSV handler
    xPortSysTickHandler,                // The SysTick handler

    // Chip Level - AT91SAM3
    SUPC_IrqHandler,    // 0  Supply Controller
    RSTC_IrqHandler,    // 1  Reset Controller
    RTC_IrqHandler,     // 2  Real Time Clock
    RTT_IrqHandler,     // 3  Real Time Timer
    WDT_IrqHandler,     // 4  Watchdog Timer
    PMC_IrqHandler,     // 5  PMC
    EEFC_IrqHandler,    // 6  EEFC
    IrqHandler7NotUsed, // 7  Reserved
    UART0_IrqHandler,   // 8  UART0
    UART1_IrqHandler,   // 9  UART1
    SMC_IrqHandler,     // 10 SMC
    PIOA_IrqHandler,    // 11 Parallel IO Controller A
    PIOB_IrqHandler,    // 12 Parallel IO Controller B
    PIOC_IrqHandler,    // 13 Parallel IO Controller C
    USART0_IrqHandler,  // 14 USART 0
    USART1_IrqHandler,  // 15 USART 1
    IrqHandler16NotUsed,// 16 Reserved
    IrqHandler17NotUsed,// 17 Reserved
    MCI_IrqHandler,     // 18 MCI
    TWI0_IrqHandler,    // 19 TWI 0
    TWI1_IrqHandler,    // 20 TWI 1
    SPI_IrqHandler,     // 21 SPI
    SSC_IrqHandler,     // 22 SSC
    TC0_IrqHandler,     // 23 Timer Counter 0
    TC1_IrqHandler,     // 24 Timer Counter 1
    TC2_IrqHandler,     // 25 Timer Counter 2
    TC3_IrqHandler,     // 26 Timer Counter 3
    TC4_IrqHandler,     // 27 Timer Counter 4
    TC5_IrqHandler,     // 28 Timer Counter 5
    ADC_IrqHandler,     // 29 ADC controller
    DAC_IrqHandler,     // 30 DAC controller
    PWM_IrqHandler,     // 31 PWM
    CRCCU_IrqHandler,   // 32 CRC Calculation Unit
    ACC_IrqHandler,     // 33 Analog Comparator
    USBD_IrqHandler,    // 34 USB Device Port
    IrqHandler35NotUsed // 35 not used
};

//*****************************************************************************
//
// The following are constructs created by the linker, indicating where the
// the "data" and "bss" segments reside in memory.  The initializers for the
// for the "data" segment resides immediately following the "text" segment.
//
//*****************************************************************************

extern unsigned long _etext ;
extern unsigned long _data ;
extern unsigned long _edata ;
extern unsigned long _bss ;
extern unsigned long _ebss ;
extern unsigned long _isr_vectors ;

//*****************************************************************************
// Reset entry point for your code.
// Sets up a simple runtime environment and initializes the C/C++
// library.
//
//*****************************************************************************
void Reset_Handler(void)
{
    register unsigned long *pulSrc, *pulDest;
    extern const unsigned long Global_STACK_FILLER ;
    extern const unsigned long __kernel_stack_start ;
    extern unsigned long kernellastrunstack ;

    //
    // Copy the data segment initializers from flash to SRAM.
    //
    pulSrc = &_etext;
    for(pulDest = &_data ; pulDest < &_edata ; ) {
        *pulDest++ = *pulSrc++;
    }

    //
    // Zero fill the bss segment.  This is done with inline assembly since this
    // will clear the value of pulDest if it is not kept in a register.
    //
    __asm("    ldr     r0, =_bss\n"
          "    ldr     r1, =_ebss\n"
          "    mov     r2, #0\n"
          "    .thumb_func\n"
          "zero_loop:\n"
          "        cmp     r0, r1\n"
          "        it      lt\n"
          "        strlt   r2, [r0], #4\n"
          "        blt     zero_loop"
    );


#ifdef USE_WATCHDOG
    // Enable Watchdog (write once register)
    // Max time: 16 sec., Reset all
    WDT->WDT_MR = (0xfff << 0) | (0xfff<<16) | WDT_MR_WDRSTEN | WDT_MR_WDDBGHLT ;
#else // USE_WATCHDOG
    // Disable Watchdog (write once register)
    WDT->WDT_MR = WDT_MR_WDDIS ;
#endif // USE_WATCHDOG

    //
    // set user stack pointer, msp/psp if priv/unpriv mode desired
    // NOT REALLY USED
    __asm("    ldr      r0, =__kernel_stack__end        \n"
          "    msr      psp, r0                         \n"
    );

    //
    // set CCR in order NOT to align stack to 8 byte
    //
//    __asm("    ldr      r0, =#0xe000ed14                \n"
//          "    ldr      r1, =#0                         \n"
//          "    str      r1, [r0]                        \n"
//    );

    // Enable NRST reset
    RSTC->RSTC_MR = (RSTC_MR_KEY & (0xa5<<24)) | RSTC_MR_URSTEN ;

    //
    // Init stack for Kernel and main
    //
    kernellastrunstack = __kernel_stack_start ;
    for(pulDest = (unsigned long *)(&__kernel_stack_start) ; pulDest < (unsigned long *)(&_vStackTop) ; ) {
        *pulDest++ = Global_STACK_FILLER ;
    }

    // Set the vector table base address
    pulSrc = (unsigned long *)&_isr_vectors ;
    SCB->VTOR = ((unsigned long)(pulSrc)) ;

    //
    // Call C++ library initilisation, if present
    //
    if (__libc_init_array)
        __libc_init_array() ;

    //
    // Call the application's entry point.
    // __main() is the entry point for redlib based applications (which calls main())
    // main() is the entry point for newlib based applications
    //
    if (__main)
        __main() ;
    else if (main)
        main() ;
    else
        maintask() ;

    //
    // main() shouldn't return, but if it does, we'll just enter an infinite loop
    //
    while (1) {
        ;
    }
}

//*****************************************************************************
//
// This is the code that gets called when the processor receives a NMI.  This
// simply enters an infinite loop, preserving the system state for examination
// by a debugger.
//
//*****************************************************************************
static void NMI_Handler(void)
{
    while(1)
    {
    }
}

static void HardFault_Handler(void)
{
    while(1)
    {
    }
}

static void MemManage_Handler(void)
{
    while(1)
    {
    }
}

static void BusFault_Handler(void)
{
    while(1)
    {
    }
}

static void UsageFault_Handler(void)
{
    while(1)
    {
    }
}

static void DebugMon_Handler(void)
{
    while(1)
    {
    }
}

//*****************************************************************************
//
// Processor ends up here if an unexpected interrupt occurs or a handler
// is not present in the application code.
//
//*****************************************************************************
static void IntDefaultHandler(void)
{
    //
    // Go into an infinite loop.
    //
    while(1)
    {
    }
}
