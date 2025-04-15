// drv_clk.c - model CLK driver task

//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//

#include "rtxcapi.h"
#include "enable.h"

#include "csema.h"
#include "cclock.h"

#include "assign.h"

#include "cvtdate.h"

#ifdef CBUG
extern char cbugflag;
#endif // CBUG

//----------------------------------------------------------------------------
// Sanity check

#if !defined(USE_TIMER_PIT) && !defined(USE_TIMER_PWM)
#error "One timer at least"
#endif
#if defined(USE_TIMER_PIT) && defined(USE_TIMER_PWM)
#error "Too many timer"
#endif

#define PUTOUT(A) { while (!(AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_TXRDY)) ; AT91C_BASE_DBGU->DBGU_THR = (A) & 0x1ff ; }

//----------------------------------------------------------------------------
// The following functions must be write in ARM mode. Function called directly
// by exception vector

extern void AT91F_Spurious_handler(void) ;
extern void AT91F_Default_IRQ_handler(void) ;
extern void AT91F_Default_FIQ_handler(void) ;

// -----------------------------------------------------------------------------
// internal functions prototype

void clkstart(int lowpowermode) ;
void clkstop(void) ;
void LowLevelInit(void) ;
void AT91F_AIC_Configure(int irq_id, int priority, int src_type, FRAME *(*newHandler)(FRAME *frame)) ;

// -----------------------------------------------------------------------------
// local variables

#ifdef USE_LED_BLINKER
#define MAX_NUMOFLEDS   2       // MUST be power of 2
unsigned long led_mask[MAX_NUMOFLEDS] ;
unsigned long led_curmask[MAX_NUMOFLEDS] ;
unsigned short led_prescaler[MAX_NUMOFLEDS] ;
unsigned short led_counter[MAX_NUMOFLEDS] ;
unsigned short led_bitnum[MAX_NUMOFLEDS] ;
#endif // USE_LED_BLINKER

static volatile time_t rtctime ;	 // running seconds maintained by clock driver
static int ratecnt ;		         // clkrate counter (0 -> clkrate-1)

// -----------------------------------------------------------------------------
// functions related to system timer

// Interrupt routine
FRAME *clkc(FRAME * frame)
{
    register volatile unsigned long status ;
#ifdef USE_TIMER_PIT
    register volatile unsigned long statreset ;
    extern void com3drvpit(void) ;      // same vector
#if defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)
    extern void efcdrvpit(void) ;       // same vector
#endif // defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)
#endif // USE_TIMER_PIT

// Interrupts enabled from 21/Aug/2006
//    ENABLE ;                  // open interrupt window

    // Interrupt Acknowledge
#ifdef USE_TIMER_PWM
    status = AT91C_BASE_PWMC->PWMC_ISR ;
    {
#endif // USE_TIMER_PWM
#ifdef USE_TIMER_PIT
    status = AT91C_BASE_PITC->PITC_PISR ;
    if (status) {
        statreset = AT91C_BASE_PITC->PITC_PIVR ;   // reset interrupt
#endif // USE_TIMER_PIT

        if (!(--ratecnt)) {
            rtctime++ ;             // update second counter
            ratecnt = CLKRATE ;     // reset rate counter
            // adjust RC offset
            //AT91C_BASE_SYS->SYS_GPBR0 = rtctime - AT91C_BASE_RTTC->RTTC_RTVR ;
        }

#ifdef USE_TIMER_PIT
    }
    com3drvpit() ;              // same vector
#if defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)
    efcdrvpit() ;               // same vector
#endif // defined(USE_EFC_ON_ARM) && defined(USE_AT91SAM7S512)
#endif // USE_TIMER_PIT
#ifdef USE_TIMER_PWM
    }
#endif // USE_TIMER_PWM

#ifdef CBUG
    if (cbugflag) {             // no clock ticks while RTXCbug active
        wdt_reset() ;           // keep the dog quiet
        //return(frame);
        return(KS_ISRexit(frame, (SEMA)0)) ;
    }
#endif // CBUG

#ifdef USE_TIMER_PIT
    if (status) {
        KS_ISRtick() ;
    }
#endif // USE_TIMER_PIT

#ifdef USE_TIMER_PWM
    KS_ISRtick() ;
#endif // USE_TIMER_PWM

#ifdef USE_LED_BLINKER
    {
        register int i ;
        
        for(i=0 ; i<MAX_NUMOFLEDS ; i++) {
            // handle led blinking
            if (!(--led_counter[i])) {  // time to change ?
                led_counter[i] = led_prescaler[i] ;
                if (led_curmask[i] & 1) {
                    // set at 0 (turn led on)
                    AT91C_BASE_PIOA->PIO_CODR = i ? PIOA_LEDR : PIOA_LED ;
                } else {
                    // set at 1 (turn led off)
                    AT91C_BASE_PIOA->PIO_SODR = i ? PIOA_LEDR : PIOA_LED ;
                }
                led_curmask[i] >>= 1 ;     // another bit
                if (!(--led_bitnum[i])) {  // end of bits
                    led_bitnum[i] = sizeof(led_mask[0]) * 8  ;
                    led_curmask[i] = led_mask[i] ;
                }
            }
        }
    }
#endif // USE_LED_BLINKER

    return(KS_ISRexit(frame, (SEMA)0)) ;
}

//----------------------------------------------------------------------------
// Slow clock interrupt routine.
// only timer PIT is used

FRAME *slowclkc(FRAME * frame)
{
    register volatile unsigned long status ;

    // Interrupt Acknowledge
    status = AT91C_BASE_PITC->PITC_PIVR ;

#undef USE_MYLED
#ifdef USE_MYLED
    // let led to blink
    if (AT91C_BASE_PIOA->PIO_PDSR & PIOA_LED) {
        // set at 0 (turn led on)
        AT91C_BASE_PIOA->PIO_CODR = PIOA_LED ;
    } else {
        // set at 1 (turn led off)
        AT91C_BASE_PIOA->PIO_SODR = PIOA_LED ;
    }
#endif // USE_MYLED

    return(frame);
}

//----------------------------------------------------------------------------
// AT91F_AIC_Configure

void AT91F_AIC_Configure(int irq_id,     // interrupt number to initialize
	                 int priority,   // priority to give to the interrupt
	                 int src_type,   // activation and sense of activation
	                 FRAME *(*newHandler) (FRAME *frame) ) // address of the interrupt handler
{
    unsigned int mask ;

    mask = 1 << irq_id ;

    // Disable the interrupt on the interrupt controller
    AT91C_BASE_AIC->AIC_IDCR = mask ;
    // Save the interrupt handler routine pointer and the interrupt priority
    AT91C_BASE_AIC->AIC_SVR[irq_id] = (unsigned int) newHandler ;
    // Store the Source Mode Register
    AT91C_BASE_AIC->AIC_SMR[irq_id] = src_type | priority  ;
    //* Clear the interrupt on the interrupt controller
    AT91C_BASE_AIC->AIC_ICCR = mask ;

    // Enable the interrupt on the interrupt controller
    AT91C_BASE_AIC->AIC_IECR = mask ;
}

//----------------------------------------------------------------------------
// LowLevelInit
// This function performs very low level HW initialization

void LowLevelInit(void)
{
    int i ;

    // Set up the default interrupt handler vectors
    AT91C_BASE_AIC->AIC_SVR[0] = (int) AT91F_Default_FIQ_handler ;
    for (i=1;i < 31; i++) {
	    AT91C_BASE_AIC->AIC_SVR[i] = (int) AT91F_Default_IRQ_handler ;
    }
    AT91C_BASE_AIC->AIC_SPU  = (int) AT91F_Spurious_handler ;

    // enable external reset
    AT91C_BASE_RSTC->RSTC_RMR = (int)((AT91C_RSTC_KEY & (0xa5<<24)) |
                                      (AT91C_RSTC_URSTEN)) ;

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    {
        extern unsigned long noinit_GPBR0 ;
        // check if this reset is from a poweron
        if ( ((AT91C_BASE_RSTC->RSTC_RSR) & AT91C_RSTC_RSTTYP) == AT91C_RSTC_RSTTYP_POWERUP ) {
            noinit_GPBR0 = 0 ;
        }
    }
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
}

// -----------------------------------------------------------------------------
// init function

void clkstart(int lowpowermode)
{
   ratecnt = CLKRATE ;  // init prescaler

   rtctime = 0 ;        // just to have a default
#if defined(USE_AT91SAM7A3)
   rtctime = AT91C_BASE_SYS->SYS_GPBR0 + AT91C_BASE_RTTC->RTTC_RTVR ;   // init timer
//   if (!lowpowermode) AT91C_BASE_RTTC->RTTC_RTAR = 0xFFFFFFFF ;		// _FR_ 28/01/10
#endif  // defined(USE_AT91SAM7A3)

#if defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
    {
        extern unsigned long noinit_GPBR0 ;
        rtctime = noinit_GPBR0 + AT91C_BASE_RTTC->RTTC_RTVR ;           // init timer
   		if (!lowpowermode) AT91C_BASE_RTTC->RTTC_RTAR = 0xFFFFFFFF ;	// _FR_ 28/01/10
    }
#endif // defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

    // enable 'tick' timer
    // NOUSB: 60MHz / 16 = 3.75MHz / 100 = 37500 --> integer divide for 100 Hz
    //  USB:  48MHz / 16 = 3MHz / 100 = 30000 --> integer divide for 100 Hz

#ifdef USE_TIMER_PWM
    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_PWMC) ;

    // clock: current_clock / 16
    // Left Aligned
    AT91C_BASE_PWMC_CH0->PWMC_CMR = 4 ;

    // end of count
    AT91C_BASE_PWMC_CH0->PWMC_CPRDR = current_clock / 16 / CLKRATE ;

    // middle count (just to keep it quiet, use half the value
    AT91C_BASE_PWMC_CH0->PWMC_CDTYR = current_clock / 16 / CLKRATE / 2 ;

    // install interrupt vector
    AT91F_AIC_Configure(AT91C_ID_PWMC, PWM_INTERRUPT_PRIORITY, AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE, clkc) ;

    // enable interrupt
    AT91C_BASE_PWMC->PWMC_IER = 1 ;     // our channel
    
    // enable channel
    AT91C_BASE_PWMC->PWMC_ENA = 1 ;     // our channel

#ifdef CBUG
    // if random function desired, enable this counter for RND extraction
    AT91C_BASE_PITC->PITC_PIMR |= AT91C_PITC_PITEN ;
#endif // CBUG
#endif // USE_TIMER_PWM

#ifdef USE_TIMER_PIT
    if (lowpowermode) {
        // Period Interval Mode Register, 1 Hz
        AT91C_BASE_PITC->PITC_PIMR = ((current_clock / 16 / 1) - 1 ) ;
        // install interrupt vector
        AT91F_AIC_Configure(AT91C_ID_SYS, PIT_INTERRUPT_PRIORITY, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, slowclkc) ;
    } else {
        // Period Interval Mode Register
        AT91C_BASE_PITC->PITC_PIMR = ((current_clock / 16 / CLKRATE) - 1 ) ;
        // install interrupt vector
        AT91F_AIC_Configure(AT91C_ID_SYS, PIT_INTERRUPT_PRIORITY, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, clkc) ;
    }
    // enable channel and interrupt
    AT91C_BASE_PITC->PITC_PIMR |= (AT91C_PITC_PITEN | AT91C_PITC_PITIEN) ;
#endif // USE_TIMER_PIT
}

// -----------------------------------------------------------------------------
// tick wait function

void tickwait(int udelay)
{
    long tstart, tnow, tdelay ;

    tdelay = (udelay * (current_clock / 16)) / 1000000 ;

    // current count
#ifdef USE_TIMER_PWM
    tstart = AT91C_BASE_PWMC_CH0->PWMC_CCNTR ;

    do {
        tnow = (AT91C_BASE_PWMC_CH0->PWMC_CCNTR) - tstart ;
        if (tnow < 0) { // add total count
            tnow += (current_clock / 16 / CLKRATE) ;
        }
        tdelay -= tnow ;
    } while(tdelay > 0) ;
#endif // USE_TIMER_PWM

#ifdef USE_TIMER_PIT
    tstart = (AT91C_BASE_PITC->PITC_PIIR) & AT91C_PITC_CPIV ;

    do {
        tnow = ((AT91C_BASE_PITC->PITC_PIIR) & AT91C_PITC_CPIV) - tstart ;
        if (tnow < 0) { // add total count
            tnow += (current_clock / 16 / CLKRATE) ;
        }
        tdelay -= tnow ;
    } while(tdelay > 0) ;
#endif // USE_TIMER_PIT
}

// -----------------------------------------------------------------------------
// tick measure function

#ifdef CBUG
unsigned long long tickmeasure(unsigned long long lasttick)
{
    extern volatile TICKS rtctick ;
    register unsigned long long utck ;
    TICKS tlatch ;
    
    if (cbugflag) {	// no clock ticks while RTXCbug active
        return(0) ;
    }
    
    // make measure avoiding 'rtctick' increment instant
    do {
        tlatch = rtctick ;              // tick now
        utck = (tlatch * 1000000LL) / CLKRATE ;
#ifdef USE_TIMER_PWM
        utck += (((AT91C_BASE_PWMC_CH0->PWMC_CCNTR) * 16000000LL) / current_clock) ;
#endif // USE_TIMER_PWM
#ifdef USE_TIMER_PIT
        utck += ((((AT91C_BASE_PITC->PITC_PIIR) & AT91C_PITC_CPIV) * 16000000LL) / current_clock) ;
#endif // USE_TIMER_PIT
    } while(tlatch != rtctick) ;        // still same ?
    
    return(utck - lasttick) ;
}
#endif // CBUG

// -----------------------------------------------------------------------------
// tick measure function

#ifdef CBUG
int random(void)
{
    static unsigned long last ;

    // in any case from periodic timer
    last ^= AT91C_BASE_PITC->PITC_PIIR ;
    return(last) ;
}
#endif // CBUG

// -----------------------------------------------------------------------------
// terminate function

void clkstop(void)
{
#ifdef USE_TIMER_PWM
    // disable interrupt
    AT91C_BASE_PWMC->PWMC_IDR = 1 ;     // our channel

    // disable channel
    AT91C_BASE_PWMC->PWMC_DIS = 1 ;     // our channel

    // Peripheral Clock Disable Register
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_PWMC) ;

#ifdef CBUG
    // if random function desired, disable this counter for RND extraction
    AT91C_BASE_PITC->PITC_PIMR &= (~AT91C_PITC_PITEN) ;
#endif // CBUG
#endif // USE_TIMER_PWM

#ifdef USE_TIMER_PIT
    // disable channel and interrupt
    AT91C_BASE_PITC->PITC_PIMR &= (~((AT91C_PITC_PITEN | AT91C_PITC_PITIEN))) ;
#endif // USE_TIMER_PIT

#ifdef USE_LED_BLINKER
    // set at 1 (turn led off)
    AT91C_BASE_PIOA->PIO_SODR = PIOA_LED ;
#endif // USE_LED_BLINKER
}

#ifdef USE_LED_BLINKER
// -----------------------------------------------------------------------------
// Led Blinker

void Set_LedBlinker(int led, unsigned long mask, int period)
{
    led &= (MAX_NUMOFLEDS-1) ;          // only power of 2 num of leds

    DISABLE ;   // critical region
    led_mask[led] = mask ;
    led_curmask[led] = mask ;
    led_prescaler[led] = period / CLKTICK ;
    led_counter[led] = 1 ;
    led_bitnum[led] = 1 ;
    ENABLE ;    // end of critical region
}
#endif // USE_LED_BLINKER

// -----------------------------------------------------------------------------
// tick timer functions
time_t KS_inqtime(void)
{
    return(rtctime) ;
}
void KS_deftime(time_t t)
{
    rtctime = t ;               // system timer
    RTC_WriteTime_t(t) ;        // store in RTC
}

// -----------------------------------------------------------------------------
// Beeper with timer 8

#ifdef USE_BEEPER_TIMER8
#define TIMER8_MASK     (1 << 29)
void timer8_beep_on(int freq)
{
    // Peripheral Clock Enable Register
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_TC8) ;

    AT91C_BASE_PIOB->PIO_PPUDR = TIMER8_MASK ;  // no pull up

    // PIO B: Peripheral A select register
    AT91C_BASE_PIOB->PIO_ASR = TIMER8_MASK ;
    // PIO B: disable register
    AT91C_BASE_PIOB->PIO_PDR = TIMER8_MASK ;

    // Timer 8 (2 of C)
    // mode: WAVE (WSEL=10), input is TIMER_CLOCK5: current_clock/1024
    AT91C_BASE_TCB2->TCB_TC2.TC_CMR = AT91C_TC_CLKS_TIMER_DIV5_CLOCK |
                                      AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_WAVE |
                                      AT91C_TC_BCPB_SET |   // RB sets TIOB
                                      AT91C_TC_BCPC_CLEAR | // RC clears TIOB
                                      AT91C_TC_EEVT_XC0 ;   // Just to avoid TIOB as input

    // Timer 8 reference registers (A and B something in between)
    AT91C_BASE_TCB2->TCB_TC2.TC_RA = current_clock / 1024 / freq / 3 ;
    AT91C_BASE_TCB2->TCB_TC2.TC_RB = current_clock / 1024 / freq / 2 ;
    // NOUSB: 60MHz / 1024 / 5859 = 10 Hz
    // USB:   48MHz / 1024 / 4687 = 10 Hz
    //         1MHz / 1024 /   98 = 10 Hz
    AT91C_BASE_TCB2->TCB_TC2.TC_RC = current_clock / 1024 / freq ;

    // Timer 8 control: enable
    AT91C_BASE_TCB2->TCB_TC2.TC_CCR = AT91C_TC_CLKEN ;

    // Timer 8 control: start
    AT91C_BASE_TCB2->TCB_TC2.TC_CCR = AT91C_TC_SWTRG ;

}
void timer8_beep_off(void)
{
    // Disable timer
    AT91C_BASE_TCB2->TCB_TC2.TC_CCR = AT91C_TC_CLKDIS ;

    // Peripheral Clock Disable Register
    AT91C_BASE_PMC->PMC_PCDR = (1 << AT91C_ID_TC8) ;

    // Set digital output at 1
    AT91C_BASE_PIOB->PIO_SODR = TIMER8_MASK ;   // set at 1
    AT91C_BASE_PIOB->PIO_PER = TIMER8_MASK ;    // Set in PIO mode
    AT91C_BASE_PIOB->PIO_OER = TIMER8_MASK ;    // Configure in Output

}
#endif // USE_BEEPER_TIMER8

// end of file - drv_clk.c

