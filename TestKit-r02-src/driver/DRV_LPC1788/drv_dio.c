// drv_dio.c - digital I/O driver tasks

//
//   Copyright (c) 1997-2011.
//   T.E.S.T. srl
//

//
// This module is provided as a digital I/O port driver.
//

#define POPULATE_PORTARRAY  // real array population

#ifdef CBUG
#define PORT_TW_DEBUG
#endif

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"
#include "cres.h"

#include "extapi.h"

#include "assign.h"

#include "portat.h"

// sanity check
#if defined(USE_MAX7324_TW1) && defined(USE_PCAL9555A_TW1)
#error "Only USE_MAX7324_TW1 or USE_PCAL9555A_TW1"
#endif

#ifdef USE_PCAL9555A_TW1
extern int TWI_txrx(int dev, int sub, unsigned char *buf, int len) ;
//extern char dbg_io ;
#endif

#ifdef USE_MAX7324_TW1 // I/O n. 1
short tw1in_change = 0 ;
#endif

#ifdef USE_MAX7324_TW2  // I/O n. 2
short tw2in_change = 0 ;
#endif

#ifdef USE_TWI_SRV
short tw1in1_change = 0 ;
#endif

#ifdef USE_PCAL9555A_TW2
unsigned char badout2 ;
unsigned short old_out2 ;
#endif
#ifdef USE_PCAL9555A_TW3
unsigned char badout3 ;
unsigned short old_out3 ;
#endif
// USE_MAX7324_TW1			I/O expander n.1 (on TW0)
// USE_MAX7324_TW2			I/O expander n.2 (on TW0)

// USE_TWI1_AUXILIARY		Acc, other service 	 (on TW1)
// USE_TWI_SRV				I/O on Service Board (on TW1)
//----------------------------------------------------------------------------
// global variebles: MOVED
volatile int moved_flag ;
volatile int acc_flag ;
static unsigned long loc_int ;

//----------------------------------------------------------------------------
// internal functions

void diostart(void) ;
void diostop(void) ;
unsigned short dio_counter(int port) ;
unsigned long dio_read(int port) ;
void dio_write(int port, int pmask, int pval) ;
void dio_beeper(int freq) ;

//----------------------------------------------------------------------------
// Interrupt routine for GPIO - same as Ext Int 3

void GPIO_IRQHandler(void)
{

    // useful after a wakeup
    if (CHKWUPREG == 0xffffffL){
        LPC_RTC->GPREG2 = (LPC_GPIOINT->IO0IntStatF | LPC_GPIOINT->IO0IntStatR ) ;
        LPC_RTC->GPREG3 = (LPC_GPIOINT->IO2IntStatF | LPC_GPIOINT->IO2IntStatR ) ;
        SETWUPREG(1L) ;
		
	} else {
		loc_int = (LPC_GPIOINT->IO0IntStatF | LPC_GPIOINT->IO0IntStatR ) ;
		//if (loc_int & (1<<21)) moved_flag++ ;
		if (loc_int & ((1<<9)|(1<<8))) acc_flag++ ;
#ifdef GEMHH
		if (loc_int & (1<<21)) moved_flag++ ;
#else
		if (LPC_GPIOINT->IO2IntStatF & 0xf) moved_flag++ ;
#endif
	}

	// Clear Interrupts
    LPC_GPIOINT->IO0IntClr = -1 ;
    LPC_GPIOINT->IO2IntClr = -1 ;

}

//----------------------------------------------------------------------------
// Port Set

void diosetpin(const uint32_t pinportset)
{
    volatile uint32_t * const iocon_base = ((uint32_t *)(LPC_IOCON_BASE)) ;
    volatile uint32_t * const iodir_base = ((uint32_t *)(LPC_GPIO0_BASE)) ;
    volatile uint32_t * const ioset_base = ((uint32_t *)(LPC_GPIO0_BASE + 0x18)) ;
    volatile uint32_t * const ioclr_base = ((uint32_t *)(LPC_GPIO0_BASE + 0x1c)) ;
    int port, pin ;

    // identify port and pin
    port = (pinportset & PIN_MASK_PORT)>>PIN_POSITION_PORT ;
    pin  = (pinportset & PIN_MASK_NUM)>>PIN_POSITION_NUM ;

    // set desired function
    iocon_base[ 32*port + pin ] = pinportset & 0x1ffff ;

    // set direction
    if (pinportset & (SET_PIN_INPUT<<PIN_POSITION_SET)) {
        // set pin as input
        iodir_base[port * 0x8] &= ~(1<<pin) ;
    } else {
        // prepare future output level
        if (pinportset & (SET_PIN_OUT_1<<PIN_POSITION_SET)) {
            ioset_base[port * 0x8] = (1<<pin) ;
        } else {
            ioclr_base[port * 0x8] = (1<<pin) ;
        }
        // set pin as output
        iodir_base[port * 0x8] |= (1<<pin) ;
    }
}

void diosetports(const uint32_t * PortAt)
{
int i ;
int pin, pf ;

    // set port pins according to array populated in assign_xxx.h
    for(i=0 ; PortAt[i] ; i++) {
        diosetpin(PortAt[i]) ;
    }
    
	// set all port pins not declared as Output at 0
	// PIO0: 0-31
	for(pin=0;pin<31;pin++){
		pf = 0 ;
		for(i=0 ; PortAt[i] ; i++) {
			if ( (((PortAt[i] & PIN_MASK_PORT)>>PIN_POSITION_PORT) == PORT_PIO0 ) &&
				 (((PortAt[i] & PIN_MASK_NUM)>>PIN_POSITION_NUM) == pin ) ){
				pf = 1 ;
				break ;
			}
		}
		if (!pf) diosetpin( SET_PORT_VAL(PORT_PIO0, pin, SET_PIN_OUT_0, SET_PORT_PN | 0) ) ;
	}
	// PIO1: 0-1,4,8-10,14-31
	for(pin=0;pin<31;pin++){
		switch(pin){
			case 2:
			case 3:
			case 5:
			case 6:
			case 7:
			case 11:
			case 12:
			case 13:
				break ;
			default:
			pf = 0 ;
			for(i=0 ; PortAt[i] ; i++) {
				if ( (((PortAt[i] & PIN_MASK_PORT)>>PIN_POSITION_PORT) == PORT_PIO1 ) &&
					(((PortAt[i] & PIN_MASK_NUM)>>PIN_POSITION_NUM) == pin ) ){
					pf = 1 ;
					break ;
				}
			}
			if (!pf) diosetpin( SET_PORT_VAL(PORT_PIO1, pin, SET_PIN_OUT_0, SET_PORT_PN | 0) ) ;
			break ;
		}
	}
	// PIO2: 0-13
	for(pin=0;pin<13;pin++){
		pf = 0 ;
		for(i=0 ; PortAt[i] ; i++) {
			if ( (((PortAt[i] & PIN_MASK_PORT)>>PIN_POSITION_PORT) == PORT_PIO2 ) &&
				 (((PortAt[i] & PIN_MASK_NUM)>>PIN_POSITION_NUM) == pin ) ){
				pf = 1 ;
				break ;
			}
		}
		if (!pf) diosetpin( SET_PORT_VAL(PORT_PIO2, pin, SET_PIN_OUT_0, SET_PORT_PN | 0) ) ;
	}
	// PIO3: 0-7,23-26
	for(pin=0;pin<27;pin++){
		if ((pin<8) || (pin>22)){
			pf = 0 ;
			for(i=0 ; PortAt[i] ; i++) {
				if ( (((PortAt[i] & PIN_MASK_PORT)>>PIN_POSITION_PORT) == PORT_PIO3 ) &&
					(((PortAt[i] & PIN_MASK_NUM)>>PIN_POSITION_NUM) == pin ) ){
					pf = 1 ;
					break ;
				}
			}
			if (!pf) diosetpin( SET_PORT_VAL(PORT_PIO3, pin, SET_PIN_OUT_0, SET_PORT_PN | 0) ) ;
		}
	}
	// PIO4: 0-15,24,25,28-31
	for(pin=0;pin<32;pin++){
		if ((pin<16) || (pin==24) || (pin==25) || (pin>27)){
			pf = 0 ;
			for(i=0 ; PortAt[i] ; i++) {
				if ( (((PortAt[i] & PIN_MASK_PORT)>>PIN_POSITION_PORT) == PORT_PIO4 ) &&
					(((PortAt[i] & PIN_MASK_NUM)>>PIN_POSITION_NUM) == pin ) ){
					pf = 1 ;
					break ;
				}
			}
			if (!pf) diosetpin( SET_PORT_VAL(PORT_PIO4, pin, SET_PIN_OUT_0, SET_PORT_PN | 0) ) ;
		}
	}
	// PIO5: 0-4
	for(pin=0;pin<5;pin++){
		pf = 0 ;
		for(i=0 ; PortAt[i] ; i++) {
			if ( (((PortAt[i] & PIN_MASK_PORT)>>PIN_POSITION_PORT) == PORT_PIO5 ) &&
				 (((PortAt[i] & PIN_MASK_NUM)>>PIN_POSITION_NUM) == pin ) ){
				pf = 1 ;
				break ;
			}
		}
		if (!pf) diosetpin( SET_PORT_VAL(PORT_PIO5, pin, SET_PIN_OUT_0, SET_PORT_PN | 0) ) ;
	}
	
}

//----------------------------------------------------------------------------
// DIO initializer

void diostart(void)
{

	diosetports(PortAtBoot) ;

	// enable interrupts
	NVIC_EnableIRQ(GPIO_IRQn) ;
	NVIC_SetPriority(GPIO_IRQn, GPIO_INTERRUPT_LEVEL) ;

	// enable interrupts (also for wakeup)
	//GPIOINT->IO0IntEnF = (1<<21) ;      // enable interrupt from Falling P0.21 MOVED

	// ------------------------
	// enable timers
	LPC_SC->PCONP |= ( 0
#if defined(USE_CNT_T1CAP0)
						| CLKPWR_PCONP_PCTIM1
#endif // defined(USE_CNT_T1CAP0)

#if (defined(USE_FREQ_T2MAT2) || defined(USE_CNT_T2CAP0) )
						| CLKPWR_PCONP_PCTIM2
#endif // (defined(USE_FREQ_T2MAT2) || defined(USE_CNT_T2CAP0) )

#if defined(USE_CNT_T3CAP0)
						| CLKPWR_PCONP_PCTIM3
#endif // defined(USE_CNT_T3CAP0)

#if defined(USE_PWM0A)
						| CLKPWR_PCONP_PCMCPWM
#endif // defined(USE_PWM0A)
				) ;

#if defined(USE_CNT_T1CAP0)
	LPC_TIM1->TCR = 2 ;     // reset
	LPC_TIM1->CTCR = 1 ;    // count on rising edge of cap0
	LPC_TIM1->TCR = 1 ;     // enable
//    LPC_TIM1->TCR = 2 ;     // reset
//    LPC_TIM1->CTCR = 5 ;    // count on rising edge of cap1
//    LPC_TIM1->TCR = 1 ;     // enable
#endif // defined(USE_CNT_T1CAP0)

#if defined(USE_CNT_T2CAP0)
	LPC_TIM2->TCR = 2 ;     // reset
	LPC_TIM2->CTCR = 1 ;    // count on rising edge of cap0
	LPC_TIM2->TCR = 1 ;     // enable
#endif // defined(USE_CNT_T2CAP0)

#if defined(USE_CNT_T3CAP0)
	LPC_TIM3->TCR = 2 ;     // reset
	LPC_TIM3->CTCR = 1 ;    // count on rising edge of cap0
	LPC_TIM3->TCR = 1 ;     // enable
//    LPC_TIM3->TCR = 2 ;     // reset
//    LPC_TIM3->CTCR = 5 ;    // count on rising edge of cap1
//    LPC_TIM3->TCR = 1 ;     // enable
#endif // defined(USE_CNT_T3CAP0)

#ifdef USE_PCAL9555A_TW2
	badout2 = 0 ;
#endif
#ifdef USE_PCAL9555A_TW3
	badout3 = 0 ;
#endif
}

//----------------------------------------------------------------------------
// DIO terminator

void diostop(void)
{
    // do nothing if code upgrade
    if (EKS_EnqShutdown() == SD_CODEUPGRADE) return ;

    // ------------------------
    // disable timers
#if defined(USE_CNT_T1CAP0)
    LPC_TIM1->TCR = 0 ;     // disable
#endif // defined(USE_CNT_T1CAP0)
#if (defined(USE_FREQ_T2MAT2) || defined(USE_CNT_T2CAP0) )
    LPC_TIM2->TCR = 0 ;     // disable
#endif
#if defined(USE_CNT_T3CAP0)
    LPC_TIM3->TCR = 0 ;     // disable
#endif // defined(USE_CNT_T3CAP0)

    LPC_SC->PCONP &= ~( 0
#if defined(USE_CNT_T1CAP0)
                        | CLKPWR_PCONP_PCTIM1
#endif // defined(USE_CNT_T1CAP0)

#if (defined(USE_FREQ_T2MAT2) || defined(USE_CNT_T2CAP0) )
                        | CLKPWR_PCONP_PCTIM2
#endif // (defined(USE_FREQ_T2MAT2) || defined(USE_CNT_T2CAP0) )

#if defined(USE_CNT_T3CAP0)
                        | CLKPWR_PCONP_PCTIM3
#endif // defined(USE_CNT_T3CAP0)

#if defined(USE_PWM0A)
                        | CLKPWR_PCONP_PCMCPWM
#endif // defined(USE_PWM0A)
                  ) ;

   diosetports(PortAtOff) ;

    // interrupts are not disabled, they are useful for wakeup
}

//----------------------------------------------------------------------------
#ifdef PORT_TW1_CNF
short dioconfTW(void)
{
unsigned char lbuf[5] ;
unsigned short retval ;
//unsigned short *ls ;
//unsigned long *ll ;
// Output port configuration register (4Fh) is good as default

//	ls = (unsigned short *) &lbuf[1] ;
//	ll = (unsigned long *) &lbuf[1] ;
	retval = 0 ;
	// Read conf
// // Configure I/O expander (if needed)
	KS_lockw(TWIPORT) ;         // we trust with
	
#ifdef PORT_TW1_CNF
	lbuf[0] = 0x46 ;		// Set pullup
	//*ll = PORT_TW1_PUP ;
	lbuf[1] = (PORT_TW1_PUP & 0xff) ;
	lbuf[2] = ((PORT_TW1_PUP & 0xff00)>>8) ;
	TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
	lbuf[0] = 0x48 ;		// Set pullup
	lbuf[1] = ((PORT_TW1_PUP & 0xff0000)>>16) ;
	lbuf[2] = ((PORT_TW1_PUP & 0xff000000)>>24) ;
	TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
	
	lbuf[0] = 0xff ;
	lbuf[1] = 0xff ;
	TWI_txrx(PCAL9555A_ADDR, 0x6, lbuf, 2) ; // get DIRECTION
#ifdef PORT_TW_DEBUG
		printf("TW1:%02x%02x (%04x)\n", lbuf[1], lbuf[0], PORT_TW1_CNF ) ;
#endif
	// If not same direction change
	if ( ((PORT_TW1_CNF & 0xff) != lbuf[0]) || ( ((PORT_TW1_CNF & 0xff00)>>8) != lbuf[1]) ){
		lbuf[0] = 0x06 ;
		lbuf[1] = (PORT_TW1_CNF & 0xff) ;
		lbuf[2] = ((PORT_TW1_CNF>>8) & 0xff) ;
		//*ls = PORT_TW1_CNF ;
		TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
		retval = 1 ;
	}
	// Disable latch
	lbuf[0] = 0x44 ;
	lbuf[1] = 0x00 ;
	lbuf[2] = 0x00 ;
//	*ls = 0 ;
	TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
#endif

#ifdef PORT_TW2_CNF
	lbuf[0] = 0x46 ;		// Set pullup
	//*ll = PORT_TW2_PUP ;
	lbuf[1] = (PORT_TW2_PUP & 0xff) ;
	lbuf[2] = ((PORT_TW2_PUP & 0xff00)>>8) ;
	TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
	lbuf[0] = 0x48 ;		// Set pullup
	lbuf[1] = ((PORT_TW2_PUP & 0xff0000)>>16) ;
	lbuf[2] = ((PORT_TW2_PUP & 0xff000000)>>24) ;
	TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
	
	lbuf[0] = 0x00 ;
	lbuf[1] = 0xff ;
	TWI_txrx(PCAL9555A_ADDR2, 0x6, lbuf, 2) ;
#ifdef PORT_TW_DEBUG
		printf("TW2:%02x%02x (%04x)\n", lbuf[1], lbuf[0], PORT_TW2_CNF) ;
#endif
	if ( ((PORT_TW2_CNF & 0xff) != lbuf[0]) || ( ((PORT_TW2_CNF & 0xff00)>>8) != lbuf[1]) ){
#ifdef PORT_TW2_OUT // Before set output value
		lbuf[0] = 0x02 ;
		//*ls = PORT_TW2_OUT ;
		lbuf[1] = (PORT_TW2_OUT & 0xff) ;
		lbuf[2] = ((PORT_TW2_OUT>>8) & 0xff) ;
		TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
#endif
		lbuf[0] = 0x06 ;
		//*ls = PORT_TW2_CNF ;
		lbuf[1] = (PORT_TW2_CNF & 0xff) ;
		lbuf[2] = ((PORT_TW2_CNF>>8) & 0xff) ;
		TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
		retval = 1 ;
	}
	// Disable latch
	lbuf[0] = 0x44 ;
	lbuf[1] = 0x00 ;
	lbuf[2] = 0x00 ;
	//*ls = 0 ;
	TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
#endif

#ifdef PORT_TW3_CNF
	lbuf[0] = 0x46 ;		// Set pullup
	//*ll = PORT_TW3_PUP ;
	lbuf[1] = (PORT_TW3_PUP & 0xff) ;
	lbuf[2] = ((PORT_TW3_PUP & 0xff00)>>8) ;
	TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
	lbuf[0] = 0x48 ;		// Set pullup
	lbuf[1] = ((PORT_TW3_PUP & 0xff0000)>>16) ;
	lbuf[2] = ((PORT_TW3_PUP & 0xff000000)>>24) ;
	TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
	
	
	lbuf[0] = 0xc3 ;
	lbuf[1] = 0x7f ;
	TWI_txrx(PCAL9555A_ADDR3, 0x6, lbuf, 2) ;
#ifdef PORT_TW_DEBUG
		printf("TW3:%02x%02x (%04x)\n", lbuf[1], lbuf[0], PORT_TW3_CNF) ;
#endif
	if ( ((PORT_TW3_CNF & 0xff) != lbuf[0]) || ( ((PORT_TW3_CNF & 0xff00)>>8) != lbuf[1]) ){
#ifdef PORT_TW3_OUT // Before set output value
		lbuf[0] = 0x02 ;
		//*ls = PORT_TW3_OUT ;
		lbuf[1] = (PORT_TW3_OUT & 0xff) ;
		lbuf[2] = ((PORT_TW3_OUT>>8) & 0xff) ;
		TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
#endif
		lbuf[0] = 0x06 ;
		//*ls = PORT_TW3_CNF ;
		lbuf[1] = (PORT_TW3_CNF & 0xff) ;
		lbuf[2] = ((PORT_TW3_CNF>>8) & 0xff) ;
		TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
		retval = 1 ;
	}
	// Disable latch (not for vibra)
	lbuf[0] = 0x44 ;
	//*ls = 0xffff ; // 0x44 ;
	lbuf[1] = 0xff ;
	lbuf[2] = 0xff ;
	TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
	// Enable interrupt
	lbuf[0] = 0x4A ;
	//*ls = 0xfdb8 ; // 0x44 ;
	lbuf[1] = 0xb8 ;
	lbuf[2] = 0xfd ;
	TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
#endif

	KS_unlock(TWIPORT) ;        // we trust with
	
	return(retval) ;
}
#endif // #ifdef PORT_TW1_CNF
//----------------------------------------------------------------------------
// Read digital input port
// port values:
//      0 - 4   internal PORTs, 32 bit

unsigned long dio_read(int port)
{
unsigned long retval = 0 ;
#ifdef USE_PCAL9555A_TW1
unsigned char *lcc ;
	
	lcc = (unsigned char *)(&retval) ;
#endif

    switch(port) {
    case PORT_PIO0 : retval = LPC_GPIO0->PIN ; break ;
    case PORT_PIO1 : retval = LPC_GPIO1->PIN ; break ;
    case PORT_PIO2 : retval = LPC_GPIO2->PIN ; break ;
    case PORT_PIO3 : retval = LPC_GPIO3->PIN ; break ;
    case PORT_PIO4 : retval = LPC_GPIO4->PIN ; break ;
    case PORT_PIO5 : retval = LPC_GPIO5->PIN ; break ;
#ifdef USE_PCAL9555A_TW1
    case PORT_TW1 : // TWI-1
        {
            KS_lockw(TWIPORT) ;         // we trust with
			TWI_txrx(PCAL9555A_ADDR,  0x4C, &lcc[2], 2) ;
            TWI_txrx(PCAL9555A_ADDR, 0x100, &lcc[0], 2) ; // ((unsigned char *)(&retval)), 2) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_PCAL9555A_TW1
#ifdef USE_MAX7324_TW1
    case PORT_TW1 : // TWI-1
        {
            KS_lockw(TWIPORT) ;         // we trust with
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&retval)), 2) ;
            TWI_txrx(MAX7324_R1_ADDR, 0, ((unsigned char *)(&retval)), 2) ;
            //TWI_receive(MAX7324_W1_ADDR, ((unsigned char *)(&retval))+2, 1) ;
            TWI_txrx(MAX7324_W1_ADDR, 0, ((unsigned char *)(&retval))+2, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with

            retval |= (tw1in_change & 0xff00) ;
            tw1in_change = 0 ;
#ifdef USE_DEBUG____
            {
                extern char cbugflag ;
                if (retval & 0xff00) {
                    if (!cbugflag) pdebugt(1,"----FLAG TW1 0x%lx", retval) ;
                }
            }
#endif // #ifdef USE_DEBUG____
        }
        break ;
#endif // USE_MAX7324_TW1

#ifdef USE_PCAL9555A_TW2
    case PORT_TW2 : // TWI-1
        {
            KS_lockw(TWIPORT) ;         // we trust with
            //TWI_txrx(PCAL9555A_ADDR2, 0x100, ((unsigned char *)(&retval)), 2) ;
			TWI_txrx(PCAL9555A_ADDR2,  0x4C, &lcc[2], 2) ;
            TWI_txrx(PCAL9555A_ADDR2, 0x100, &lcc[0], 2) ; // ((unsigned char *)(&retval)), 2) ;
            KS_unlock(TWIPORT) ;        // we trust with
            old_out2 = lcc[1] ;
            old_out2 <<= 8 ;
            old_out2 += lcc[0]  ;
        }
        break ;
#endif // USE_PCAL9555A_TW2
#ifdef USE_MAX7324_TW2
    case PORT_TW2 : // TWI-2
        {
            KS_lockw(TWIPORT) ;         // we trust with
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&retval)), 2) ;
            TWI_txrx(MAX7324_R2_ADDR, 0, ((unsigned char *)(&retval)), 2) ;
            //TWI_receive(MAX7324_W1_ADDR, ((unsigned char *)(&retval))+2, 1) ;
            TWI_txrx(MAX7324_W2_ADDR, 0, ((unsigned char *)(&retval))+2, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with

            retval |= (tw2in_change & 0xff00) ;
            tw1in_change = 0 ;
#ifdef USE_DEBUG____
            {
                extern char cbugflag ;
                if (retval & 0xff00) {
                    if (!cbugflag) pdebugt(1,"----FLAG TW2 0x%lx", retval) ;
                }
            }
#endif // #ifdef USE_DEBUG____
        }
        break ;
#endif // USE_MAX7324_TW2
#ifdef USE_PCAL9555A_TW3
    case PORT_TW3 : // TWI-1
        {
            KS_lockw(TWIPORT) ;         // we trust with
            //TWI_txrx(PCAL9555A_ADDR3, 0x100, ((unsigned char *)(&retval)), 2) ;
			TWI_txrx(PCAL9555A_ADDR3,  0x4C, &lcc[2], 2) ;
            TWI_txrx(PCAL9555A_ADDR3, 0x100, &lcc[0], 2) ; // ((unsigned char *)(&retval)), 2) ;
            KS_unlock(TWIPORT) ;        // we trust with
            old_out3 = lcc[1] ;
            old_out3 <<= 8 ;
            old_out3 += lcc[0]  ;
        }
        break ;
#endif // USE_PCAL9555A_TW3
}
    return(retval) ;
}

//----------------------------------------------------------------------------
// Write digital output port
// port values:
//      0 - 4   internal PORTs, 32 bit

void dio_write(int port, int pmask, int pval)
{

	switch(port) {
	case PORT_PIO0 : LPC_GPIO0->SET = pmask & pval ; LPC_GPIO0->CLR = pmask & (~pval) ; break ;
	case PORT_PIO1 : LPC_GPIO1->SET = pmask & pval ; LPC_GPIO1->CLR = pmask & (~pval) ; break ;
	case PORT_PIO2 : LPC_GPIO2->SET = pmask & pval ; LPC_GPIO2->CLR = pmask & (~pval) ; break ;
	case PORT_PIO3 : LPC_GPIO3->SET = pmask & pval ; LPC_GPIO3->CLR = pmask & (~pval) ; break ;
	case PORT_PIO4 : LPC_GPIO4->SET = pmask & pval ; LPC_GPIO4->CLR = pmask & (~pval) ; break ;
	case PORT_PIO5 : LPC_GPIO5->SET = pmask & pval ; LPC_GPIO5->CLR = pmask & (~pval) ; break ;
#ifdef USE_PCAL9555A_TW1
	case PORT_TW1 : // TWI-1
		{
			unsigned short retval ;
			//unsigned short * ls ;
			unsigned char lbuf[3] ;

			//ls = (unsigned short *) &lbuf[1] ;
			KS_lockw(TWIPORT) ;         // we trust with
			TWI_txrx(PCAL9555A_ADDR, 0x02, ((unsigned char *)(&retval)), 2) ; // read 02 at place of 0x100 (0x0)
			retval &= (~pmask) ;
			retval |= pval ;
			lbuf[0] = 0x02 ;
			lbuf[1] = (retval & 0xff) ;
			lbuf[2] = ((retval>>8) & 0xff) ;
			//*ls = retval ;
			TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
			KS_unlock(TWIPORT) ;        // we trust with
		}
		break ;
#endif // USE_PCAL9555A_TW1
#ifdef USE_MAX7324_TW1  // I/O n. 1
	case PORT_TW1 : // TWI-1
		{
			unsigned char c ;
			KS_lockw(TWIPORT) ;         // we trust with
			// old value
			//TWI_receive(MAX7324_W1_ADDR, &c, 1) ;
			TWI_txrx(MAX7324_W1_ADDR, 0, &c, 1) ;
			// change value
			c |= (pmask & pval) ;
			c &= ((~pmask) | pval) ;
			// new value
			TWI_send(MAX7324_W1_ADDR, &c, 1) ;
			KS_unlock(TWIPORT) ;        // we trust with
		}
		break ;
#endif // USE_MAX7324_TW1

#ifdef USE_PCAL9555A_TW2
	case PORT_TW2 : // TWI-1
		{
			unsigned short retval ;
			//unsigned short * ls ;
			unsigned char lbuf[3] ;

			//ls = (unsigned short *) &lbuf[1] ;
			KS_lockw(TWIPORT) ;         // we trust with
			TWI_txrx(PCAL9555A_ADDR2, 0x02, ((unsigned char *)(&retval)), 2) ;
#ifdef USE_TRACE
			if ((old_out2 & 0x7e80) != (retval & 0x7e80)){
				badout2++;
				if (par_trace) AppendTrace(TRACE_VER, TR_BADOUT, "chh", 2, old_out2, retval) ;	//BAD_OUT$0c%d:0x$1h%04x,0x$2h%04x|
#ifdef USE_PDEBUG
				pdebugt(1, "BAD_OUT2:0x%04x,0x%04x", old_out2, retval) ;
#endif
			}
#endif // #ifdef USE_TRACE
			retval = old_out2 ;
			retval &= (~pmask) ;
			retval |= pval ;
			lbuf[0] = 0x02 ;
			lbuf[1] = (retval & 0xff) ;
			lbuf[2] = ((retval>>8) & 0xff) ;
			//*ls = retval ;
			TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
			KS_unlock(TWIPORT) ;        // we trust with
			old_out2 =  retval ;
		}
		break ;
#endif // USE_PCAL9555A_TW2
#ifdef USE_MAX7324_TW2  // I/O n. 2
    case PORT_TW2 : // TWI-1
        {
            unsigned char c ;
            KS_lockw(TWIPORT) ;         // we trust with
            // old value
            //TWI_receive(MAX7324_W1_ADDR, &c, 1) ;
            TWI_txrx(MAX7324_W2_ADDR, 0, &c, 1) ;
            // change value
            c |= (pmask & pval) ;
            c &= ((~pmask) | pval) ;
            // new value
            TWI_send(MAX7324_W2_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_MAX7324_TW2
#ifdef USE_PCAL9555A_TW3
		case PORT_TW3 : // TWI-1
			{
			unsigned short retval ;
			//unsigned short * ls ;
			unsigned char lbuf[3] ;
			//unsigned char lcbuf[3] ;

			//ls = (unsigned short *) &lbuf[1] ;
			KS_lockw(TWIPORT) ;         // we trust with
			TWI_txrx(PCAL9555A_ADDR3, 0x02, ((unsigned char *)(&retval)), 2) ;
			//printf("TW3 diff: %04x,%04x\n", retval, old_out3 ) ;
#ifdef USE_TRACE
			if ((old_out3 & 0x7e80) != (retval & 0x7e80)){
				badout3++;
				if (par_trace) AppendTrace(TRACE_VER, TR_BADOUT, "chh", 3, old_out3, retval) ;	//BAD_OUT$0c%d:0x$1h%04x,0x$2h%04x|
#ifdef USE_PDEBUG
				pdebugt(1, "BAD_OUT3:0x%04x,0x%04x", old_out3, retval) ;
#endif
			}
#endif // #ifdef USE_TRACE
			retval = old_out3 ;
			retval &= (~pmask) ;
			retval |= pval ;
			lbuf[0] = 0x02 ;
			lbuf[1] = (retval & 0xff) ;
			lbuf[2] = ((retval>>8) & 0xff) ;
			//*ls = retval ;
			//printf("TW3 w: %04x,%04x (%04x,%04x)\n", old_io3, retval, pmask, pval ) ; // (lbuf[2]& 0x7e), (lcbuf[2]& 0x7e) ) ;
			//KS_delay(SELFTASK, ((TICKS)(2)/CLKTICK)) ; // PROVA
			TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
// 			if (dbg_io){
// 				KS_delay(SELFTASK, ((TICKS)(2)/CLKTICK)) ;
// 				TWI_txrx(PCAL9555A_ADDR3, 0x100, lcbuf, 2) ; // ((unsigned char *)(&retval)), 2) ;
// 			}
			KS_unlock(TWIPORT) ;        // we trust with
// 			if (dbg_io){
// 				if ((lcbuf[1] & 0x7e) != (lbuf[2] & 0x7e)){
// 					printf("TW3 w: %04x,%04x (%04x,%04x)\n", old_io3, retval, pmask, pval ) ;
// 					printf("TW3 d: %02x%02x,%02x%02x\n", lbuf[2], lbuf[1], lcbuf[1], lcbuf[0] ) ;
// 					TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
// 				}
// 			}
			old_out3 =  retval ;
		}
		break ;
#endif // USE_PCAL9555A_TW3
    }
}

//----------------------------------------------------------------------------
// Write digital input port mask

void dio_mask(int port, int pmask)
{
#ifdef USE_MAX7324_TW1  // I/O n. 1
unsigned char c = (unsigned char)(pmask) ;
#endif
#ifdef USE_PCAL9555A_TW1
unsigned char lbuf[3] ;
//unsigned short * ls ;

	//ls = (unsigned short *) &lbuf[1] ;
	lbuf[0] = 0x4a ;
	//*ls = (0xffff & (~pmask)) ;
	lbuf[1] = (0xff & (~pmask)) ;
	lbuf[2] = ((0xff00 & (~pmask))>>8) ;
#endif

    switch(port) {
#ifdef USE_PCAL9555A_TW1
    case PORT_TW1 : // TWI-1
        {
            KS_lockw(TWIPORT) ;         // we trust with
            TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
			// If enabled use latch
			if (pmask){
				lbuf[0] = 0x44 ;
				//*ls = 0xffff ; // pmask ;
				lbuf[1] = 0xff ;
				lbuf[2] = 0xff ;
// #ifdef FR_BUG
// 				printf("Latch1: %02x 0x%02x%02x\n", lbuf[0], lbuf[2], lbuf[1] ) ;
// #endif
				TWI_send(PCAL9555A_ADDR, lbuf, 3) ;
			}
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_PCAL9555A_TW1
#ifdef USE_MAX7324_TW1  // I/O n. 1
    case PORT_TW1 : // TWI-1
        {
            KS_lockw(TWIPORT) ;         // we trust with
            // before get input modification
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&tw1in_change)), 2) ;
            TWI_txrx(MAX7324_R1_ADDR, 0, ((unsigned char *)(&tw1in_change)), 2) ;
            // new value
            TWI_send(MAX7324_R1_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_MAX7324_TW1

#ifdef USE_PCAL9555A_TW2
    case PORT_TW2 : // TWI-2
        {
            KS_lockw(TWIPORT) ;         // we trust with
            TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
			// If enabled use latch
			if (pmask){
				lbuf[0] = 0x44 ;
				//*ls = 0xff ; // pmask ;
				lbuf[1] = 0xff ;
				lbuf[2] = 0x00 ;
// #ifdef FR_BUG
// 				printf("Latch2: %02x 0x%02x%02x\n", lbuf[0], lbuf[2], lbuf[1] ) ;
// #endif
				TWI_send(PCAL9555A_ADDR2, lbuf, 3) ;
			}
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_PCAL9555A_TW2
#ifdef USE_MAX7324_TW2  // I/O n. 2
    case PORT_TW2 : // TWI-1
        {
            KS_lockw(TWIPORT) ;         // we trust with
            // before get input modification
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&tw1in_change)), 2) ;
            TWI_txrx(MAX7324_R2_ADDR, 0, ((unsigned char *)(&tw2in_change)), 2) ;
            // new value
            TWI_send(MAX7324_R2_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_MAX7324_TW2

#ifdef USE_PCAL9555A_TW3
    case PORT_TW3 : // TWI-3
        {
            KS_lockw(TWIPORT) ;         // we trust with
            TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
			// If enabled use latch
			if (pmask){
				lbuf[0] = 0x44 ;
				//*ls = pmask ;
				lbuf[1] = (pmask & 0xff) ;
				lbuf[2] = ((pmask>>8) & 0xff) ;
// #ifdef FR_BUG
// 				printf("Latch3: %02x 0x%02x%02x\n", lbuf[0], lbuf[2], lbuf[1] ) ;
// #endif
				TWI_send(PCAL9555A_ADDR3, lbuf, 3) ;
// #ifdef FR_BUG
// 				*ls = pmask ;
// 				printf("(Latch3: %02x 0x%02x%02x)\n", lbuf[0], lbuf[2], lbuf[1] ) ;
// #endif
			}
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_PCAL9555A_TW3

#ifdef USE_TWI_SRV  // I/O n. 2
    case PORT_TWS : // USE_TWI1_AUXILIARY
        {
            KS_lockw(TWIPORT) ;         // we trust with
            // before get input modification
            //TWI_receive(MAX7324_R1_ADDR, ((unsigned char *)(&tw1in_change)), 2) ;
            TWI1_txrx(MAX7324_R1_ADDR, 0, ((unsigned char *)(&tw1in1_change)), 2) ;
            // new value
            TWI1_send(MAX7324_R1_ADDR, &c, 1) ;
            KS_unlock(TWIPORT) ;        // we trust with
        }
        break ;
#endif // USE_TWI_SRV
	}
}

//----------------------------------------------------------------------------
// Read counter value

unsigned short dio_counter(int port)
{
    switch(port) {
#if defined(USE_CNT_T1CAP0)
    case 0:
        return(LPC_TIM1->TC) ;
#endif // defined(USE_CNT_T1CAP0)

#if defined(USE_CNT_T2CAP0)
    case 1:
        return(LPC_TIM2->TC) ;
#endif // defined(USE_CNT_T2CAP0)

#if defined(USE_CNT_T3CAP0)
    case 2:
        return(LPC_TIM3->TC) ;
#endif // defined(USE_CNT_T3CAP0)

    default :
        break ;
    }
    return(0) ;
}

#if defined(USE_FREQ_T2MAT2)
//----------------------------------------------------------------------------
void dio_beeper(int freq)
{
    if (freq) {     // enable
        diosetpin(SET_PORT_VAL(PORT_PIO5, 0, SET_PIN_OUT_0, SET_PORT_PN | 3)) ;

        LPC_TIM2->TCR = 2 ;     // reset
        LPC_TIM2->CTCR = 0 ;    // count at PCLK
        LPC_TIM2->MR2 = PERIPHERAL_CLOCK/freq/2 ;               // desired frequency
        LPC_TIM2->MCR = 128 ;   // reset TC on match with MR2
        LPC_TIM2->EMR = 0x300 ; // toggle pin on match
        LPC_TIM2->TCR = 1 ;     // enable

    } else {        // disable

        LPC_TIM2->TCR = 2 ;     // reset
        diosetpin(SET_PORT_VAL(PORT_PIO5, 0, SET_PIN_OUT_0, SET_PORT_PN | 0)) ;
    }
}
#endif // defined(USE_FREQ_T2MAT2)

#if defined(USE_PWM0A)
//----------------------------------------------------------------------------
void dio_pwm0a(int freq, int duty)
{
    if (freq) {     // enable
        diosetpin(SET_PORT_VAL(PORT_PIO1,19, SET_PIN_OUT_0, SET_PORT_PN | 4)) ;

        LPC_MCPWM->CON_CLR = 1 ;    // stop
        LPC_MCPWM->TC0 = 0 ;                    // restart
        LPC_MCPWM->LIM0 = current_clock/PERIPHERAL_DIVIDER/freq ;  // max count (frequency)
        LPC_MCPWM->MAT0 = (duty*(current_clock/PERIPHERAL_DIVIDER/freq))/100 ; // duty cycle
        LPC_MCPWM->CON_SET = 1 ;    // start

    } else {        // disable

        LPC_MCPWM->CON_CLR = 1 ;    // stop
        diosetpin(SET_PORT_VAL(PORT_PIO1,19, SET_PIN_OUT_0, SET_PORT_PN | 0)) ;
    }
}
#endif // defined(USE_PWM0A)
