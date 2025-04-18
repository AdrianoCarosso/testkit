// drv_clk.c - model CLK driver task

//
//   Copyright (c) 1997-2010.
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

// -----------------------------------------------------------------------------
// internal functions prototype

void clkstart(int lowpowermode) ;
void clkstop(void) ;
void LowLevelInit(void) ;

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
// functions related to real time timer
void RTT_IrqHandler(void)
{
    volatile int dummy ;

    dummy = RTT->RTT_SR ;
    RTT->RTT_MR &= (~RTT_MR_ALMIEN) ;
}

// -----------------------------------------------------------------------------
// functions related to system timer

//----------------------------------------------------------------------------
// Tick Interrupt routine
void xPortSysTickHandler(void){

    if (!(--ratecnt)) {
        rtctime++ ;             // update second counter
        ratecnt = CLKRATE ;     // reset rate counter
    }

#ifdef CBUG
    if (cbugflag) {             // no clock ticks while RTXCbug active
        wdt_reset() ;           // keep the dog quiet
        // Set a PendSV to request a context switch.
        ASK_CONTEXTSWITCH ;     // set PendSV
        return ;
    }
#endif // CBUG

    KS_ISRtick() ;

#ifdef USE_LED_BLINKER
    {
        register int i ;
        
        for(i=0 ; i<MAX_NUMOFLEDS ; i++) {
            // handle led blinking
            if (!(--led_counter[i])) {  // time to change ?
                led_counter[i] = led_prescaler[i] ;
                if (led_curmask[i] & 1) {
                    // set at 0 (turn led on)
                    PIOA->PIO_CODR = i ? PIOA_LEDR : PIOA_LED ;
                } else {
                    // set at 1 (turn led off)
                    PIOA->PIO_SODR = i ? PIOA_LEDR : PIOA_LED ;
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

    // Set a PendSV to request a context switch.
    ASK_CONTEXTSWITCH ;         // set PendSV
    return ;
}

//----------------------------------------------------------------------------
// LowLevelInit
// This function performs very low level HW initialization

void LowLevelInit(void){
    DISABLE ;
    //__disable_irq();

    // setup priority levels
    NVIC_SetPriority(SVCall_IRQn, KERNEL_LEVEL) ;       // min priority
    NVIC_SetPriority(PendSV_IRQn, KERNEL_LEVEL) ;       // min priority
    NVIC_SetPriority(SysTick_IRQn, KERNEL_LEVEL) ;      // min priority
}

// -----------------------------------------------------------------------------
// init function

void clkstart(int lowpowermode)
{
    ratecnt = CLKRATE ;     // init prescaler

    rtctime = GPBR->SYS_GPBR0 + RTT->RTT_VR ;   // init timer

    // Configure SysTick to interrupt at the requested rate.
    SysTick->LOAD = ((current_clock / CLKRATE) - 1) & SYSTICK_MAXCOUNT ;        // Set reload register
    //SysTick->VAL  = 0 ;                                                       // Load the SysTick Counter Value
    SysTick->CTRL = (1<<SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE) | (1<<SYSTICK_TICKINT) ; // Enable SysTick IRQ and SysTick Timer

    NVIC_EnableIRQ(RTT_IRQn) ;
    NVIC_SetPriority(RTT_IRQn, KERNEL_LEVEL) ;
}

// -----------------------------------------------------------------------------
// tick wait function

void tickwait(int udelay){
    long tstart, tnow, tdelay ;

    tdelay = (udelay * (current_clock / 1000000)) ;

    // current count
    tstart = (SysTick->LOAD - SysTick->VAL) ;

    do {
        tnow = (SysTick->LOAD - SysTick->VAL) - tstart ;
        if (tnow < 0) { // add total count
            tnow += (SysTick->LOAD) ;
        }
    } while(tdelay > tnow) ;
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
        utck += ((SysTick->LOAD - SysTick->VAL) / (current_clock / 1000000)) ;
    } while(tlatch != rtctick) ;        // still same ?
    
    return(utck - lasttick) ;
}
#endif // CBUG

// -----------------------------------------------------------------------------
// random function

#ifdef CBUG
int random(void)
{
    static unsigned long last ;

    // in any case from periodic timer
    last ^= SysTick->VAL ;
    return(last) ;
}
#endif // CBUG

// -----------------------------------------------------------------------------
// terminate function

void clkstop(void){
    // stop system tick
    SysTick->CTRL = 0 ;

    // leave system timer interrupt enable in order to handle wakeup

#ifdef USE_LED_BLINKER
    // set at 1 (turn led off)
    PIOA->PIO_SODR = PIOA_LED | PIOA_LEDR ;
#endif // USE_LED_BLINKER
}

#ifdef USE_LED_BLINKER
// -----------------------------------------------------------------------------
// Led Blinker

void Set_LedBlinker(int led, unsigned long mask, int period) {
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
time_t KS_inqtime(void) {
    return(rtctime) ;
}
void KS_deftime(time_t t) {
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

