// drv_clk.c - model CLK driver task

//
//   Copyright (c) 1997-2011.
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

#if (defined(USECOM0_RTSCTS) || defined(USECOM2_RTSCTS) || defined(USECOM3_RTSCTS) )
#include "cqueue.h"     // COMnIQ,   COMnOQ
#ifdef USECOM0_RTSCTS
extern unsigned char COM0_Handshake ;
#endif // USECOM0_RTSCTS
#ifdef USECOM2_RTSCTS
extern unsigned char COM2_Handshake ;
#endif // USECOM2_RTSCTS
#ifdef USECOM3_RTSCTS
extern unsigned char COM3_Handshake ;
#endif // USECOM3_RTSCTS
#endif
// -----------------------------------------------------------------------------
// internal functions prototype

void clkstart(int lowpowermode) ;
void clkstop(void) ;
void LowLevelInit(void) ;

// -----------------------------------------------------------------------------
// local variables

#if defined(USE_LED_BLINKER)
unsigned long led_mask[USE_LED_BLINKER] ;
unsigned long led_curmask[USE_LED_BLINKER] ;
unsigned short led_prescaler[USE_LED_BLINKER] ;
unsigned short led_counter[USE_LED_BLINKER] ;
unsigned short led_bitnum[USE_LED_BLINKER] ;
#endif // USE_LED_BLINKER

static volatile time_t rtctime ;	 // running seconds maintained by clock driver
static int ratecnt ;		         // clkrate counter (0 -> clkrate-1)
// #ifndef  USE_ADC_MUX_ON_ARM
// unsigned short ExtAD0[16] ;
// unsigned short ADVExt[16] ;
// short  ADp ;
// #endif

// -----------------------------------------------------------------------------
// functions related to system timer
// //----------------------------------------------------------------------------
// // Dummy interrupt routine for Watchdog
// 
// void WDT_IRQHandler(void)
// {
// //    LPC_RTC->ILR = 2 ;              // clear interrupt flag
// #ifdef MTS
// 	SETWUPREG(2) ;
// #else
//     LPC_RTC->GPREG1 = (2) ;
// #endif
// }


//----------------------------------------------------------------------------
// Dummy interrupt routine for RTC

void RTC_IRQHandler(void)
{
    LPC_RTC->ILR = 2 ;              // clear interrupt flag
#ifdef MTS
	SETWUPREG(1<<9) ;
#else
    LPC_RTC->GPREG1 = (1<<9) ;
#endif
}

//----------------------------------------------------------------------------
// Tick Interrupt routine
void xPortSysTickHandler( void )
{

    if (!(--ratecnt)) {
        rtctime++ ;             // update second counter
        ratecnt = CLKRATE ;     // reset rate counter
    }

	if (LPC_SC->PCONP & CLKPWR_PCONP_PCADC) {
#ifdef USE_ADC_MUX_ON_ARM
        extern volatile unsigned short muxed_adc[8] ;
        static unsigned char muxed_idx = 0 ;
        muxed_adc[muxed_idx++ & 7] = (unsigned short)(ADC->ADDR0) ;
        if (muxed_idx & 1) { LPC_GPIO1->SET = (1<<26) ; } else { LPC_GPIO1->CLR = (1<<26) ; }
        if (muxed_idx & 2) { LPC_GPIO1->SET = (1<<28) ; } else { LPC_GPIO1->CLR = (1<<28) ; }
        if (muxed_idx & 4) { LPC_GPIO1->SET = (1<<29) ; } else { LPC_GPIO1->CLR = (1<<29) ; }
#endif // USE_ADC_MUX_ON_ARM

// #ifndef  USE_ADC_MUX_ON_ARM
// 	// If modem off or not sending data read external AD
// 	//if (ADC->ADCR){
// 	//if (sd_mode==SD_RUN){
// 		if (!HwRevBRD){
// 			if ( !(LPC_GPIO1->PIN & (1<<10)) || (LPC_GPIO1->PIN & (1<<29)) ){
// 				ADVExt[ADp] = ((LPC_ADC->DR[2]>>4) & 0xfff) ;
// 				ExtAD0[ADp++] = ((LPC_ADC->DR[0]>>4) & 0xfff) ;
// 				if (ADp>15) ADp=0 ;
// 			}
// 		}
// #endif
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

#if defined(USE_LED_BLINKER)
    {
        register int i ;

        for(i=0 ; i<USE_LED_BLINKER ; i++) {
            // handle led blinking
            if (!(--led_counter[i])) {  // time to change ?
                led_counter[i] = led_prescaler[i] ;
                if (led_curmask[i] & 1) {
                    // turn led on
                    USE_LED_ON(i) ;
                } else {
                    // turn led off
                    USE_LED_OFF(i) ;
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


#ifdef USECOM0_RTSCTS
	// if HandShake and port opened
  if ((COM0_Handshake) && (LPC_SC->PCONP & CLKPWR_PCONP_PCUART0) ){
    //static char flg_cant = 0 ;
    //static char flg_hnd = 0 ;
    static char flg_hcom0 = 0 ;

	if (flg_hcom0 & 0x1) {
		if(qheader[COM0IQ].curndx< qheader[COM0IQ].depth){
			KS_ISRsignal(COM0ISEM) ;
			flg_hcom0 &= 0x2 ;
		}
	}else if (COM0_CHECKRTS){
		flg_hcom0 |= 0x1;
	}

//     //if (SignalStopToTX == 1) {
// 	if (COM0_CHECKRTS){
//       //flg_hnd = 1 ;
// 	  flg_hcom0 |= 0x1;
//     } else {
//       //if (flg_hnd) {
//       if (flg_hcom0 & 0x1) {
//         KS_ISRsignal(COM0ISEM) ;
//       }
//       //flg_hnd = 0 ;
// 	  flg_hcom0 &= 0x2 ;
//     }

    //if (! WeCanTransmit) {
	if (COM0_CHECKCTS){
        //flg_cant = 1 ;
	  	flg_hcom0 |= 0x2;
    } else {
      //if (flg_cant) {
      if (flg_hcom0 & 0x2) {
        KS_ISRsignal(COM0OSEM) ;
      }
      //flg_cant = 0 ;
	  flg_hcom0 &= 0x1 ;
    }
}
#endif // #ifdef USECOM0_RTSCTS

#ifdef USECOM2_RTSCTS
	// if HandShake and port opened
  if ((COM2_Handshake) && (LPC_SC->PCONP & CLKPWR_PCONP_PCUART2) ){
    //static char flg_cant = 0 ;
    //static char flg_hnd = 0 ;
    static char flg_hcom2 = 0 ;


	if (flg_hcom2 & 0x1) {
		if(qheader[COM2IQ].curndx< qheader[COM2IQ].depth){
			KS_ISRsignal(COM2ISEM) ;
			flg_hcom2 &= 0x2 ;
		}
    }else if (COM2_CHECKRTS){
	  flg_hcom2 |= 0x1;
    }

// 	//if (SignalStopToTX == 1) {
// 	if (COM2_CHECKRTS){
//       //flg_hnd = 1 ;
// 	  flg_hcom2 |= 0x1;
//     } else {
//       //if (flg_hnd) {
//       if (flg_hcom2 & 0x1) {
//         KS_ISRsignal(COM2ISEM) ;
//       }
//       //flg_hnd = 0 ;
// 	  flg_hcom2 &= 0x2 ;
//     }

    //if (! WeCanTransmit) {
	if (COM2_CHECKCTS){
        //flg_cant = 1 ;
	  	flg_hcom2 |= 0x2;
    } else {
      //if (flg_cant) {
      if (flg_hcom2 & 0x2) {
        KS_ISRsignal(COM2OSEM) ;
      }
      //flg_cant = 0 ;
	  flg_hcom2 &= 0x1 ;
    }
}
#endif // #ifdef USECOM2_RTSCTS

#ifdef USECOM3_RTSCTS
	// if HandShake and port opened
  if ((COM3_Handshake) && (LPC_SC->PCONP & CLKPWR_PCONP_PCUART3) ){
    //static char flg_cant = 0 ;
    //static char flg_hnd = 0 ;
    static char flg_hcom3 = 0 ;


	if (flg_hcom3 & 0x1) {
		if(qheader[COM3IQ].curndx< qheader[COM3IQ].depth){
			KS_ISRsignal(COM3ISEM) ;
			flg_hcom3 &= 0x2 ;
		}
    }else if (COM3_CHECKRTS){
	  flg_hcom3 |= 0x1;
    }

// 	//if (SignalStopToTX == 1) {
// 	if (COM3_CHECKRTS){
//       //flg_hnd = 1 ;
// 	  flg_hcom3 |= 0x1;
//     } else {
//       //if (flg_hnd) {
//       if (flg_hcom3 & 0x1) {
//         KS_ISRsignal(COM3ISEM) ;
//       }
//       //flg_hnd = 0 ;
// 	  flg_hcom3 &= 0x2 ;
//     }

    //if (! WeCanTransmit) {
	if (COM3_CHECKCTS){
        //flg_cant = 1 ;
	  	flg_hcom3 |= 0x2;
    } else {
      //if (flg_cant) {
      if (flg_hcom3 & 0x2) {
        KS_ISRsignal(COM3OSEM) ;
      }
      //flg_cant = 0 ;
	  flg_hcom3 &= 0x1 ;
    }
}
#endif // #ifdef USECOM3_RTSCTS

    // Set a PendSV to request a context switch.
    ASK_CONTEXTSWITCH ;         // set PendSV
    return ;
}

//----------------------------------------------------------------------------
// LowLevelInit
// This function performs very low level HW initialization

void LowLevelInit(void)
{
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
    struct time_tm tm ;

    // init prescaler
    ratecnt = CLKRATE ;

// #ifndef  USE_ADC_MUX_ON_ARM
// 	// Init ADp
// 	ADp = 0 ;
// #endif

	// enable it (just to be sure)
    LPC_SC->PCONP |= CLKPWR_PCONP_PCRTC ;

    // If RTC is stopped, clear STOP bit.
    if ( (LPC_RTC->CCR & 1) == 0) { // if disabled
        LPC_RTC->CCR = 0x13 ;       // reset
    }

    LPC_RTC->AMR = 0 ;
    LPC_RTC->CIIR = 0 ;

    LPC_RTC->CCR = 0x011 ;          // un-reset

    // get RTC
    LPC_RTC->CCR &= (~1) ;          // stop
    tm.tm_sec = LPC_RTC->SEC ;
    tm.tm_min = LPC_RTC->MIN ;
    tm.tm_hr = LPC_RTC->HOUR ;
    tm.tm_wday = LPC_RTC->DOW ;
    tm.tm_day = LPC_RTC->DOM ;
    tm.tm_mon = LPC_RTC->MONTH ;
    tm.tm_yr = LPC_RTC->YEAR ;
    LPC_RTC->CCR |= 1 ;             // start

//    do {
//        ctime0 = RTC->CTIME0 ;
//        ctime1 = RTC->CTIME1 ;
//    } while((ctime0 != RTC->CTIME0) || (ctime1 != RTC->CTIME1)) ;
//    tm.tm_yr = (ctime1 >> 16) & 0xfff ;
//    tm.tm_mon = (ctime1 >> 8) & 0xf ;
//    tm.tm_day = (ctime1) & 0x1f ;
//    tm.tm_wday = 0 ;
//    tm.tm_sec = (ctime0) & 0x3f ;
//    tm.tm_min = (ctime0 >> 8) & 0x3f ;
//    tm.tm_hr = (ctime0 >> 16) & 0x1f ;

    rtctime = date2systime(&tm) ;

    // Configure SysTick to interrupt at the requested rate.
    SysTick->LOAD = ((current_clock / CLKRATE) - 1) & SYSTICK_MAXCOUNT ;        // Set reload register
    //SysTick->VAL  = 0 ;                                                       // Load the SysTick Counter Value
    SysTick->CTRL = (1<<SYSTICK_CLKSOURCE) | (1<<SYSTICK_ENABLE) | (1<<SYSTICK_TICKINT) ; // Enable SysTick IRQ and SysTick Timer

    // enable dummy RTC interrupt
    NVIC_EnableIRQ(RTC_IRQn) ;
    NVIC_SetPriority(RTC_IRQn, KERNEL_LEVEL) ;

// #ifdef USE_WATCHDOG
//     // enable dummy WDT interrupt
//     NVIC_EnableIRQ(WDT_IRQn) ;
//     NVIC_SetPriority(WDT_IRQn, KERNEL_LEVEL) ;
// #endif

#if defined(USE_LED_BLINKER)
    // no longer leds need to be initialized, dio.c must do the job
#endif // USE_LED_BLINKER

}

// -----------------------------------------------------------------------------
// tick wait function

void tickwait(int udelay)
{
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
// tick measure function

#ifdef CBUG
int random(void)
{
    static unsigned long r = 0 ;

    if (r==0) r = SysTick->VAL ;

    r = ((((r ^ SysTick->VAL) * 214013 + 2531011) >> 16) & 0xffff) ;
    return(r) ;
}
#endif // CBUG

// -----------------------------------------------------------------------------
// terminate function

void clkstop(void)
{
    SysTick->CTRL = 0 ;

#if defined(USE_LED_BLINKER)
    {
        int i ;
        for(i=0 ; i<USE_LED_BLINKER ; i++) {
            // turn led off
            USE_LED_OFF(i) ;
        }
    }
#endif // USE_LED_BLINKER
}

#if defined(USE_LED_BLINKER)
// -----------------------------------------------------------------------------
// Led Blinker

void Set_LedBlinker(int led, unsigned long mask, int period)
{
    led &= (USE_LED_BLINKER - 1) ;  // only power of 2 num of leds

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
//+++++++++++++++++++++TODO+++++++++++++++++++++++++
void timer8_beep_on(int freq)
{
}
void timer8_beep_off(void)
{
}
#endif // USE_BEEPER_TIMER8

// end of file - drv_clk.c
