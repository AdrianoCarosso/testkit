//
//   Copyright (c) 1997-2007.
//   T.E.S.T. srl
//
#include <stdio_console.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <rtxcapi.h>        // RTXC Application Program Interface
#include <enable.h>

#include <cclock.h>
#include <csema.h>
#include <cqueue.h>
#include <cpart.h>
#include <cres.h>

#include <extapi.h>

#include "_AT91SAM7A3.h"

#include "assign.h"

#define USE_VERBOSE
#define USE_REAL_INPUTS
#define USE_REAL_OUTPUTS

// -----------------------------------------------------------------------
// local data

int GlobTimer ;         // spontaneous data dump
int GlobCurrTimer ;

#ifdef USE_CAN_ON_ARM
int CanTimer[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES] ;         // spontaneous data dump
int CanCurrTimer[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES] ;
int CanLenMsg[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES] ;
char CanMsg[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES][8] ;
#endif // USE_CAN_ON_ARM

// -----------------------------------------------------------------------
// local functions

void tk0extender(void) TASK_ATTRIBUTE ;
void ParseBuffer(const char *usbbuf) ;
int GetHexValue(const char *cp, int len) ;
void HandleSrecord(const char *linebuff) ;

#ifdef USE_CAN_ON_ARM
void HandleCanRead(int chn, int mbx) ;
#endif // USE_CAN_ON_ARM

// -----------------------------------------------------------------------
// Configure hardware I/O

#define TIMER_INTERVAL  100                     // in millisec
#define TMINTVL ((TICKS)(TIMER_INTERVAL*CLKRATE/1000))        // Wake up every xxx msec

#ifdef USE_PERFORMANCE
#define PERFORMANCE_PRESCALER   5000
int perf_presca ;
#endif // USE_PERFORMANCE

// -----------------------------------------------------------------------
// from uartdrv.c
extern void uartstart(int num, unsigned long baud_rate, int mode) ;

//----------------------------------------------------------------------------

void tk0extender(void){
    static char usbdata[128] ;          // buffer of data from USB
    int usbidx ;                        // index of buffer
    unsigned long lpmget ;
    static const SEMA semalist[] = {LU0TSEM, TK0ISEM, LU0QSEM,
#ifdef USE_CAN_ON_ARM
                                    CAN0SEM, CAN1SEM,
#endif // USE_CAN_ON_ARM
                                    PORT0SEM,
                                    PORT1SEM,
#if !defined(USE_TASK0_SERIAL)
                                    PORT2SEM,
#else
									PORTUSEM,
#endif // !defined(USE_TASK0_SERIAL)
                                    PORT3SEM,
                                    0} ;
    static CLKBLK clkblk ;              // our timer pointer
    SEMA cause ;                        // wake up reason
    char lastch, c ;
#ifdef USE_CAN_ON_ARM
    int i, j ;
    unsigned long canstat ;
#endif // USE_CAN_ON_ARM
#if defined(USE_TASK0_SERIAL)
    static char mond[128] ;          // buffer of data from USB
    int monidx ;                        // index of buffer
    char lastm ;
	
    lastm = monidx = 0  ;
#endif
    // settling time

    KS_delay(SELFTASK, ((TICKS)200*CLKRATE/1000)) ;     // skip time
    Set_LedBlinker(0, 0x08080808, 50) ;

#ifdef PORT_TW1_CNF
	dioconfTW() ;
#endif

	// Default
    dio_write(PORT_PIOA, PIOA_TTL, 0) ;

    dio_write(PORT_PIOB, ( PIOB_PON | PIOB_MTSPRES ), 0) ;		// No presence & Vext to MTS
    dio_write(PORT_PIOB, PIOB_MTS3V_OFF, 0) ;					// No 3V to MTS
    dio_write(PORT_PIOB, ( PIOB_VOLTAMP | PIOB_FLOAT ), 0) ;	// Output as mA, dig. output float
    
    #if 0
    dio_write(PORT_TW2, 0xe37f, 0) ;
    dio_write(PORT_TW2, 0x1c80, 0x1c80) ;
    #else
    dio_write(PORT_TW2, 0xFF00, (0xA1<<8) );      // ok 
    //dio_write(PORT_TW2, 0xFF00, (0xA0<<8) );    // no !
    //dio_write(PORT_TW2, 0xFF00, (0x20<<8) );    // no !
    /*
    ftdi layout_init 0x0a08 0x0ffb
    #0x0a08 = 0b0001001 00001000 -- data
    #0x0ffb = 0b0001111 11111011 -- direction
    
    ftdi layout_init 0x0218 0x0ffb
    
    # DLP2232M              pin Signal    Data    Direction       Notes
    # ADBUS0                TCK           0       1 (out)
    # ADBUS1                TDI           0       1 (out)
    # ADBUS2                TDO           0       0 (in)
    # ADBUS3                TMS           1       1 (out)         JTAG IEEE std recommendation

    # ADBUS4                nSRST         0       1 (out)         Reset CPU
    # ADBUS5                ENKIT*        0       1 (out)         Enable TestKit                            (active low)
    # ADBUS6                CPU           1       1 (out)         Choice CPU0/1 (with MOD*=0)
    # ADBUS7                MOD*          0       1 (out)         Talk to modem (with CPU=0)        (active low)

    # ACBUS0                MDMLCK        0       1 (out)         Lock Modem reset
    # ACBUS1                ENJ           0       1 (out)         Enable JTAG (otherwise rs232)
    # ACBUS2                COM1*         0       1 (out)         Connect to COM0                               (active low)
    # ACBUS3                COMDB         0       1 (out)         Connect to COM1
    */
    //dio_write(PORT_TW2, 0x0FFB, 0x0A08 );
    //dio_write(PORT_TW2, 0xFF00, (0xA0<<8) );
    //dio_write(PORT_TW2, 0xFF00, (0xB0<<8) );
    //dio_write(PORT_TW2, 0xFF00, (0xA8<<8) );  // talk to modem
    //dio_write(PORT_TW2, 0xFF00, (0xA4<<8) ); //talk to CPU 1
    // CPU GD32F407
    #endif

    dio_write(PORT_TW3, 0xffff, 0xffff) ;

    // Init semaphore and timer

    KS_defqsema(LU0Q, LU0QSEM, QNE) ;           // wake up on no empty
    KS_defqsema(TK0IPORT, TK0ISEM, QNE) ;       // wake up on no empty

    while(KS_dequeue(COM0IQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(COM0IQ, PORT0SEM, QNE) ;        // wake up on no empty

#if !defined(USE_TASK0_SERIAL) 
    while(KS_dequeue(COM2IQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(COM2IQ, PORT2SEM, QNE) ;        // wake up on no empty
#else
    while(KS_dequeue(USBIQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(USBIQ, PORTUSEM, QNE) ;        // wake up on no empty
#endif // !defined(USE_TASK0_SERIAL) 


    while(KS_dequeue(COM1IQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(COM1IQ, PORT1SEM, QNE) ;        // wake up on no empty

    while(KS_dequeue(COM3IQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(COM3IQ, PORT3SEM, QNE) ;        // wake up on no empty

    //pclkblk = KS_alloc_timer() ;                // Get one system timer
    KS_start_timer(&clkblk, TMINTVL, TMINTVL, LU0TSEM) ;

    // Main loop

    //printf("MTS extender\n") ;
    //KS_delay(SELFTASK, ((TICKS)200*CLKRATE/1000)) ;     // skip time

    GlobTimer = 0 ;     // no spontaneous data at startup
#ifdef USE_PERFORMANCE
    perf_presca = PERFORMANCE_PRESCALER * 2 ;
#endif // USE_PERFORMANCE
    lastch = 0 ;
    usbidx = 0 ;        // no data in usb buffer
    for( ; ; ) {
        cause = KS_waitm(semalist) ;        // don't waste time
        switch(cause) {         // I know who waked me up
        case LU0QSEM :                // data from other L.U.s
            KS_dequeue(LU0Q, &lpmget) ;                // get data from other task
            if (!lpmget) {                        // System is requesting shutdown
                KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;     // skip time
                // turn off
                EKS_AgreeShutdown() ;                // I agree
                KS_terminate(SELFTASK) ;        // terminate this task
            } else {                                // data handling
            }
            break ;

        case TK0ISEM :                // data from serial input (monitor)
            KS_dequeue(TK0IPORT, &c) ;
            //KS_enqueue(COM1OQ, &c) ;
            //KS_enqueue(TK0OPORT, &c) ;
            usbdata[usbidx] = c ;
            if (usbidx < (sizeof(usbdata) - 1))
                usbidx++ ;
            if ((c == 13) || (c == 10)) {
                usbdata[usbidx] = '\0' ;
                ParseBuffer(usbdata) ;
                usbidx = 0 ;
            }
            if ((lastch == 'P') && (c == '8')) {
#ifdef USE_PERFORMANCE_EVALUATION
                { unsigned long cpperf ;
                  KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;     // skip time
                  perf_last = perf_counter ;
                  KS_delay(SELFTASK, ((TICKS)10000*CLKRATE/1000)) ;   // skip time
                  cpperf = perf_counter - perf_last ;
                  printf("Performance=%ld\n", cpperf) ;
                  KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;     // skip time
                }
#endif // USE_PERFORMANCE_EVALUATION
                KS_signal(CBUGSEMA) ;                // wake up debugger
                lastch = 0 ;                        // reset status machine
                c = 0 ;
                usbidx = 0 ;
            } else {
                lastch = c ;                        // keep status machine alive
            }
            break ;

        case LU0TSEM :                // cyclic timer
            // refresh WatchDog, see RTXCsys.c and cclock.h for details
            wdt_reset() ;       // keep the dog quiet

#ifdef USE_PERFORMANCE
            perf_presca -= TIMER_INTERVAL ;
            if (perf_presca <= 0) {
                perf_presca = PERFORMANCE_PRESCALER ; // reload
                // performance print
                printf("CPU=%ld %%\n", 100 - ( (((perf_counter - perf_last) * 100 / LOOPS_PER_SEC) + 2) / 5)) ;
                perf_last = perf_counter ;
            }
#endif // USE_PERFORMANCE

            if (GlobTimer) {    // if timer is enabled
                GlobCurrTimer -= TIMER_INTERVAL ;
                if (GlobCurrTimer <= 0) {
                    GlobCurrTimer = GlobTimer ; // reload
                    ParseBuffer("D\n") ;
                }
            }

#ifdef USE_CAN_ON_ARM
            for(i=0 ; i<CAN_TOT_CHANNELS ; i++) {       // scan all mailboxes
                for(j=0 ; j<CAN_TOT_MAILBOXES ; j++) {
                    if (CanTimer[i][j]) {               // if timer is enabled
                        CanCurrTimer[i][j] -= TIMER_INTERVAL ;
                        if (CanCurrTimer[i][j] <= 0) {
                            CanCurrTimer[i][j] = CanTimer[i][j] ; // reload
                            if (!CanLenMsg[i][j])
                            	HandleCanRead(i, j) ;
							else if (CanLenMsg[i][j]>0)
								CAN_write(i, j, CanMsg[i][j], CanLenMsg[i][j]) ;
                        }
                    }
                }
            }
#endif // USE_CAN_ON_ARM

//#ifdef USE_REAL_LEDS
//            // let led to blink
//            if (AT91C_BASE_PIOA->PIO_PDSR & LED_MASK) {
//                // set at 0 (turn led on)
//                AT91C_BASE_PIOA->PIO_CODR = LED_MASK ;
//            } else {
//                // set at 1 (turn led off)
//                AT91C_BASE_PIOA->PIO_SODR = LED_MASK ;
//            }
//#endif // USE_REAL_LEDS
            break ;

        case PORT0SEM :                // serial port -0-
            printf("C0=") ;
            while(KS_dequeue(COM0IQ, &c)==RC_GOOD) {
                printf("%02X", (c & 0xff)) ;
            }
            printf("\n") ;
            break ;

#if !defined(USE_TASK0_SERIAL)
        case PORT2SEM :                // serial port -1-
            printf("C2=") ;
            while(KS_dequeue(COM2IQ, &c)==RC_GOOD) {
                printf("%02X", (c & 0xff)) ;
            }
            printf("\n") ;
            break ;
#else
        case PORTUSEM :                // serial port -1-
            printf("CU=") ;
            while(KS_dequeue(USBIQ, &c)==RC_GOOD) {
                printf("%02X", (c & 0xff)) ;
//             }
//             printf("\n") ;
//             break ;
//             KS_dequeue(USBIQ, &c) ;
// 			printf("%02X\n", (c & 0xff)) ;
            mond[monidx] = c ;
			KS_enqueuew(USBOQ, &c) ;
            if (monidx < (sizeof(mond) - 1))
                monidx++ ;
            if ((c == 13) || (c == 10)) {
                mond[monidx] = '\0' ;
                ParseBuffer(mond) ;
                monidx = 0 ;
            }
            if ((lastm == 'P') && (c == '8')) {
#ifdef USE_PERFORMANCE_EVALUATION
                { unsigned long cpperf ;
                  KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;     // skip time
                  perf_last = perf_counter ;
                  KS_delay(SELFTASK, ((TICKS)10000*CLKRATE/1000)) ;   // skip time
                  cpperf = perf_counter - perf_last ;
                  printf("Performance=%ld\n", cpperf) ;
                  KS_delay(SELFTASK, ((TICKS)500*CLKRATE/1000)) ;     // skip time
                }
#endif // USE_PERFORMANCE_EVALUATION
                KS_signal(CBUGSEMA) ;                // wake up debugger
                lastm = 0 ;                        // reset status machine
                c = 0 ;
                monidx = 0 ;
            } else {
                lastm = c ;                        // keep status machine alive
            }
			}
            break ;
#endif // USE_TASK0_SERIAL

//         case PORT2SEM :                // serial port -2-
//             printf("C2=") ;
//             while(KS_dequeue(COM2IQ, &c)==RC_GOOD) {
//                 printf("%02X", (c & 0xff)) ;
//             }
//             printf("\n") ;
//             break ;

        case PORT3SEM :                // serial port -3-
            printf("C3=") ;
            while(KS_dequeue(COM3IQ, &c)==RC_GOOD) {
                printf("%02X", (c & 0xff)) ;
            }
            printf("\n") ;
            break ;

#ifdef USE_CAN_ON_ARM
        case CAN0SEM :                 // CAN -0-
            canstat = CAN_status(0) ;
            for(i=0 ; i<CAN_TOT_MAILBOXES ; i++) {
                if (canstat & (1<<i)) {
                    HandleCanRead(0, i) ;
                }
            }
            break ;

        case CAN1SEM :                 // CAN -1-
            canstat = CAN_status(1) ;
            for(i=0 ; i<CAN_TOT_MAILBOXES ; i++) {
                if (canstat & (1<<i)) {
                    HandleCanRead(1, i) ;
                }
            }
            break ;
#endif // USE_CAN_ON_ARM
        }
    }
}

// -----------------------------------------------------------------------
// GetHexValue

#define BASE    16

int GetHexValue(const char *cp, int len){
    int result = 0 ;    // default
    int value ;

    while( (isxdigit((unsigned char)*cp)) &&
           ((value = isdigit((unsigned char)*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < BASE) &&
           (len--) ) {
        result = (result * BASE) + value ;
        cp++ ;
    }
    return(result) ;
}

// -----------------------------------------------------------------------
// HandleSrecord

void HandleSrecord(const char *linebuff){
    unsigned char binbuff[40] ;
    int size, addr, i ;

    if (linebuff[0] != 'S') return ;

    if (linebuff[1] == '0') {   // init
        EKS_RamFill() ;
    	printf("RamClear\r\n") ;
        return ;
    }

    if (linebuff[1] == '9') {   // end
#ifndef CBUG
        EKS_AskShutdown(SD_CODEUPGRADE) ;
        //printf("Ram: %08lx %08lx %08lx %08lx\n",
        //        ramcode[0], ramcode[1], ramcode[2], ramcode[3]) ;
#endif // CBUG
        return ;
    }

    if (linebuff[1] != '1') return ;

    // get size
    size = GetHexValue(&linebuff[2], 2) ;
    size -= 3 ;

    // get address
    addr = GetHexValue(&linebuff[4], 4) ;

    //printf("A=0x%x, S=%d\r\n", addr, size) ;

    if ((size<=0) || (size>=sizeof(binbuff))) return ;
    for(i=0 ; i<size ; i++) {
        binbuff[i] = (unsigned char)(GetHexValue(&linebuff[8 + (i*2)], 2)) ;
    }
    EKS_RamWrite(addr, binbuff, size) ;
}

// -----------------------------------------------------------------------
// HandleCanRead

#ifdef USE_CAN_ON_ARM
void HandleCanRead(int chn, int mbx){
//    unsigned long canstat ;
    unsigned long can_address ;
    unsigned char can_data[8] ;
    int can_len ;
    int i ;
    
    can_len = CAN_read(chn, mbx, &can_address, can_data) ;
    
    printf("N%x%x=", chn, mbx) ;
    if (can_len >= 0) {
        printf("%08lx", can_address) ;
        for(i=0 ; i<can_len ; i++) {
             printf("%02x", can_data[i]) ;
        }
    }
    printf("\n") ;
}
#endif // USE_CAN_ON_ARM

// -----------------------------------------------------------------------
// ParseBuffer

void ParseBuffer(const char *usbbuf){
    int i, r ;
    unsigned char c ;
    int outmask, outval ;
    int mode, baudrate ;
#ifdef USE_CAN_ON_ARM
    int timerate ;
    int chn, mbx ;
    unsigned char canbuf[16] ;
    int canptr ;
#endif // USE_CAN_ON_ARM
    extern volatile TICKS rtctick ;
    struct MYSETUP * psetup ;
	double vbr, vb1 ;
	
    switch(usbbuf[0]) {
    case 'V' :  // version
        psetup = EKS_GetSetup() ;
        printf("V=%d.%02d;%ld\n", SOFTREL, SUBSREL, psetup->sernum) ;
        break ;

#ifdef USE_VERBOSE
	case 'H':
	case 'h':
		printf(" Available commands are:\r\n") ;
		printf(" Ax <BaudRate>\r\n") ;
		printf("           Ser CANx baud rate\r\n") ;
		printf(" Bx <BaudRate>,<Par>,<Bits>,<Stop>,<Handshake>\r\n") ;
		printf("           Set COMx options\r\n") ;
		printf(" Cx <HexData>\r\n") ;
		printf("           Send to COMx port binary values from <HexData> string\r\n") ;
		printf(" D\r\n") ;
		printf("           Dump <Digital>;<Analog>;<Counter>\r\n") ;
		printf(" Exy <HexData>\r\n") ;
		printf("           Emit CANx MAILy\r\n") ;
		printf(" F\r\n") ;
		printf("           Check CAN flags\r\n") ;
		printf(" H\r\n") ;
		printf("           This help\r\n") ;
		printf(" L <HexMask>,<period_ms>\r\n") ;
		printf("           Set Red Led Mask and period in [ms]\r\n") ;
		printf(" Mxy <Mask>,<Addr>,<Extended>,<Period>\r\n") ;
		printf("           Set CANx MAILy (Mask = ffffffff means TX)\r\n") ;
		printf(" p<par>\r\n") ;
		printf("           Set FTDI ports (same par of -p into old openocd)\r\n") ;
		printf(" S\r\n") ;
		printf("           Software upgrade record (srec)\r\n") ;
		printf(" T <Interval>\r\n") ;
		printf("           Set spontaneus Dump command every <Interval> msec (0=never)\r\n") ;
		printf(" U A[V|C]x/I[V|C]x/V/Px\r\n") ;
		printf("           User fast commands:\r\n") ;
		printf("                Ax: Anx (1-14 if x=0 all to 0)\r\n") ;
		printf("                Ix: Inx (1-16 if x=0 all to 1)\r\n") ;
		printf("                Vx Vext 1 or 0\r\n") ;
		printf("                Px Presence 1 or 0\r\n") ;
		printf(" V\r\n") ;
		printf("           Version\r\n") ;
		printf(" X <Port>,<HexMask>,<HexVal>\r\n") ;
		printf("           Set digital output <HexMask> of <Port> to <HexVal>\r\n") ;
		printf("           if Port>9 change I/Oexpander Pup/Pdn \r\n") ;
		printf("\r\n Every command is composed by a <CR> terminated string\r\n") ;
		break ;
		
    case 'v' :  // version
        psetup = EKS_GetSetup() ;
        printf("Version %d.%02d, S/N %ld\n", SOFTREL, SUBSREL, psetup->sernum) ;
        break ;
#endif // USE_VERBOSE

    case 'D' :  // dump
        // digital I/O
        printf("D=%08lx,%08lx,%08lx,%08lx,%08lx;", dio_read(PORT_PIOA), dio_read(PORT_PIOB),
                                             dio_read(PORT_TW1), dio_read(PORT_TW2), dio_read(PORT_TW3) ) ;
        // read analog inputs from ADC -0- and -1-
        for(i=0 ; i<19 ; i++) {
            if (i) printf(",") ;
            printf("%x", ADC_read(i)) ;
        }
        printf(";") ;
        // read counters
        printf("%x,%x,%x\n", dio_counter(CNT_PPS), dio_counter(CNT_ODOMETER), rtctick) ;
        break ;

#ifdef USE_VERBOSE
    case 'd' :  // dump
        // digital I/O
        printf("PIOA=0x%08lx, PIOB=0x%08lx\n" ,dio_read(PORT_PIOA), dio_read(PORT_PIOB) ) ;
        printf("TW1=0x%08lx, TW2=0x%08lx, TW3=0x%08lx\n", dio_read(PORT_TW1), dio_read(PORT_TW2), dio_read(PORT_TW3) ) ;
        // read analog inputs from ADC -0- and -1-
        for(i=0 ; i<19 ; i++) {
            if (i){
				if ((i==6) || (i==12))
					printf("\n") ;
				else
					printf(", ") ;
			}
            if (i==13){
				vbr = (double) ADC_read(i) ;
				vb1 = (vbr/1024.0) * 3.0 * 18.85 + 0.04 ;
				r = vb1 ;
				vb1 *= 100.0 ;
				mode= vb1 ;
				printf("Vext=%d.%02d", r, (mode -(r*100)) ) ;
			}else if (i==14) {
				vbr = (double) ADC_read(i) ;
				vb1 = (vbr/1024.0) * 3.0 * 1.482 ;
				r = vb1 ;
				vb1 *= 100.0 ;
				mode= vb1 ;
				printf("Vbat=%d.%02d", r, (mode -(r*100)) ) ;
			}else
            	printf("A%d=%d", i, ADC_read(i)) ;
        }
        printf("\n") ;
        // read counters
        printf("Cpps=%d, Codom=%d, Tick=%d\n", dio_counter(CNT_PPS), dio_counter(CNT_ODOMETER), rtctick) ;
        break ;
#endif // USE_VERBOSE

    case 'C' :  // serial
#ifdef USE_VERBOSE
    case 'c' :  // serial
#endif // USE_VERBOSE
        i=2 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;
        while((usbbuf[i]) && (usbbuf[i+1])) {
            c = (unsigned char)(GetHexValue(&usbbuf[i], 2)) ;
            i += 2 ;
            switch(usbbuf[1]) {
            case '0': KS_enqueuew(COM0OQ, &c) ; break ;
            case '1': KS_enqueuew(COM1OQ, &c) ; break ;
            case '2': KS_enqueuew(COM2OQ, &c) ; break ;
            case '3': KS_enqueuew(COM3OQ, &c) ; break ;
            }
        }
        break ;

    case 'S' :  // software upgrade
        HandleSrecord(usbbuf) ;
        //c = usbbuf[1] ;
        //KS_enqueuew(COM3OQ, &c) ;
        break ;

    case 'T' :  // timer
#ifdef USE_VERBOSE
    case 't' :  // timer
#endif // USE_VERBOSE
        i=1 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;
        GlobTimer = atoi(&usbbuf[i]) ;  // base 10 in verbose mode
        GlobCurrTimer = GlobTimer ;
        break ;

    case 'X' :  // set output
#ifdef USE_VERBOSE
    case 'x' :  // set output
#endif // USE_VERBOSE
        i=1 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get register
        r = atoi(&usbbuf[i]) ;
		
        // next par
        while((usbbuf[i]) && (usbbuf[i] != ',')) i++ ;
        if (usbbuf[i]) i++ ;

        // get mask
        outmask = GetHexValue(&usbbuf[i], 99) ;

        // next par
        while(isxdigit((unsigned char)usbbuf[i])) i++ ;
        while((usbbuf[i] == ' ') || (usbbuf[i] == ',')) i++ ;

        // get val
        outval = GetHexValue(&usbbuf[i], 99) ;
        
        //printf("port %d Mask=0x%08x, Val=0x%08x\n", r, outmask, outval) ;

#ifdef USE_REAL_OUTPUTS
        dio_write(r, outmask, outval) ;
//        // set at 0
//        AT91C_BASE_PIOB->PIO_CODR = outmask & (~outval) ;       // set at 0
//        // set at 1
//        AT91C_BASE_PIOB->PIO_SODR = outmask & outval ;          // set at 1
#endif // USE_REAL_OUTPUTS
        break ;

    case 'B' :  // set baud
#ifdef USE_VERBOSE
    case 'b' :  // set baud
#endif // USE_VERBOSE
        i=2 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get baudrate
        baudrate = atoi(&usbbuf[i]) ;

        // next par
        while((usbbuf[i]) && (usbbuf[i] != ',')) i++ ;
        if (usbbuf[i]) i++ ;

        // get parity
        switch(toupper(usbbuf[i])) {
        default :
        case 'N' : mode = AT91C_US_PAR_NONE ; break ;
        case 'P' : mode = AT91C_US_PAR_EVEN ; break ;
        case 'O' : mode = AT91C_US_PAR_ODD ;  break ;
        }

        // next par
        while((usbbuf[i]) && (usbbuf[i] != ',')) i++ ;
        if (usbbuf[i]) i++ ;

        // get bit number
        switch(usbbuf[i]) {
        default :
        case '8' : mode |= AT91C_US_CHRL_8_BITS ; break ;
        case '7' : mode |= AT91C_US_CHRL_7_BITS ; break ;
        case '6' : mode |= AT91C_US_CHRL_6_BITS ; break ;
        }

        // next par
        while((usbbuf[i]) && (usbbuf[i] != ',')) i++ ;
        if (usbbuf[i]) i++ ;

        // get stop bit number
        switch(usbbuf[i]) {
        default :
        case '1' : mode |= AT91C_US_NBSTOP_1_BIT ; break ;
        case '2' : mode |= AT91C_US_NBSTOP_2_BIT ; break ;
        }

        // next par
        while((usbbuf[i]) && (usbbuf[i] != ',')) i++ ;
        if (usbbuf[i]) i++ ;

        // get mode (normal, rs485,handshake )
        if (toupper(usbbuf[i]) == 'R') mode |= AT91C_US_USMODE_RS485 ;
        if (toupper(usbbuf[i]) == 'H') mode |= AT91C_US_USMODE_HWHSH ;

        // set new parameter
        i = usbbuf[1] - '0' ;   // com number
        if ((i>=0) && (i<=3)) {
            uartstart(/* com = */ i, baudrate, mode) ;
        }
        break ;
        
#ifdef USE_CAN_ON_ARM
    case 'A' :  // set CAN baud
#ifdef USE_VERBOSE
    case 'a' :  // set CAN baud
#endif // USE_VERBOSE
        i=2 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get baudrate
        baudrate = atoi(&usbbuf[i]) ;

        // set new parameter
        i = usbbuf[1] - '0' ;   // com number
        if ((i>=0) && (i<=CAN_TOT_CHANNELS)) {
            dio_write(1, PIOB_CAN_OFF, 0) ;
            CAN_speed(/* chn = */ i, baudrate, NO /*YES*/) ; // YES if no TX
            //printf("A %d - %d\n", i, baudrate) ;
        }
        break ;

    case 'F' :  // check CAN flags
        chn = GetHexValue(&usbbuf[1], 1) & 1 ;
        printf("F%d=%x\n", chn, CanStatus[chn]) ;
        break ;
#ifdef USE_VERBOSE
    case 'f' :  // check CAN flags
        chn = GetHexValue(&usbbuf[1], 1) & 1 ;
        printf("CanStatus[%d] %x\n", chn, CanStatus[chn]) ;
        break ;
#endif // USE_VERBOSE

    case 'M' :  // set CAN MAILBOX
#ifdef USE_VERBOSE
    case 'm' :  // set CAN MAILBOX
#endif // USE_VERBOSE
        chn = GetHexValue(&usbbuf[1], 1) & 1 ;
        mbx = GetHexValue(&usbbuf[2], 1) ;
        
        i=3 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get mask
        outmask = GetHexValue(&usbbuf[i], 99) ;

        // next par
        while(isxdigit((unsigned char)usbbuf[i])) i++ ;
        while((usbbuf[i] == ' ') || (usbbuf[i] == ',')) i++ ;

        // get val
        outval = GetHexValue(&usbbuf[i], 99) ;

        // next par
        while(isxdigit((unsigned char)usbbuf[i])) i++ ;
        while((usbbuf[i] == ' ') || (usbbuf[i] == ',')) i++ ;

        mode = (toupper(usbbuf[i]) == 'E') ? CAN_FLAG_EXTENDEDADDR : 0 ;

        // next par
        if (usbbuf[i]) i++ ;
        while((usbbuf[i] == ' ') || (usbbuf[i] == ',')) i++ ;

        // get timerate
        timerate = atoi(&usbbuf[i]) ;
		CanLenMsg[chn][mbx] = 0 ;
        if (outmask == 0xffffffff) {    // use as Tx
            //timerate = 0 ;      //  used from 1.18
            mode |= CAN_FLAG_TRANSMIT ;
			CanLenMsg[chn][mbx] = -1 ;
        //printf("M %d,%d %x %x %x\n", chn, mbx, outval, outmask, mode) ;
        } else {                        // use as Rx
            if (!timerate) mode |= CAN_FLAG_INTERRUPT ;
        }
        
        CAN_configure(chn, mbx, outval, outmask, mode) ;

        CanTimer[chn][mbx] = timerate ;
        CanCurrTimer[chn][mbx] = timerate ;
        break ;

    case 'E' :  // emit CAN MAILBOX
#ifdef USE_VERBOSE
    case 'e' :  // emit CAN MAILBOX
#endif // USE_VERBOSE
        chn = GetHexValue(&usbbuf[1], 1) & 1 ;
        mbx = GetHexValue(&usbbuf[2], 1) ;

        i=3 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get data
        canptr = 0 ;
        while((usbbuf[i]) && (usbbuf[i+1])) {
            c = (unsigned char)(GetHexValue(&usbbuf[i], 2)) ;
            i += 2 ;
            canbuf[canptr++] = c ;
            canptr &= (sizeof(canbuf) - 1)  ;
        }

        if (canptr){
			mode=CAN_write(chn, mbx, canbuf, canptr) ;
			CanLenMsg[chn][mbx] = canptr ;
			memcpy(CanMsg[chn][mbx], canbuf, canptr) ;
		}else{ // from 1.18: if timer save data to send
			CanLenMsg[chn][mbx] = -1 ;
		}
        /*
        printf("E %d,%d", chn, mbx) ;
        for(i=0;i<canptr;i++) printf(" %02x", canbuf[i]);
        printf(" (%d)\n", mode);
        printf("Dump registers\n\r");
        printf("CAN_MR: 0x%X\n\r", AT91C_BASE_CAN0->CAN_MR);
        printf("CAN_IMR: 0x%X\n\r", AT91C_BASE_CAN0->CAN_IMR);
        printf("CAN_SR: 0x%X\n\r", AT91C_BASE_CAN0->CAN_SR);
        printf("CAN_BR: 0x%X\n\r", AT91C_BASE_CAN0->CAN_BR);
        printf("CAN_ECR: 0x%X\n\r", AT91C_BASE_CAN0->CAN_ECR);
        printf("CAN_MB_MMR: 0x%X\n\r", AT91C_BASE_CAN0_MB1->CAN_MB_MMR);
        printf("CAN_MB_MAM: 0x%X\n\r", AT91C_BASE_CAN0_MB1->CAN_MB_MAM);
        printf("CAN_MB_MID: 0x%X\n\r", AT91C_BASE_CAN0_MB1->CAN_MB_MID);
        printf("CAN_MB_MSR: 0x%X\n\r", AT91C_BASE_CAN0_MB1->CAN_MB_MSR);
        printf("CAN_MB_MDL: 0x%X\n\r", AT91C_BASE_CAN0_MB1->CAN_MB_MDL);
        printf("CAN_MB_MDH: 0x%X\n\r", AT91C_BASE_CAN0_MB1->CAN_MB_MDH);
        */
        break ;
#endif // USE_CAN_ON_ARM

    case 'L' :
#ifdef USE_VERBOSE
    case 'l' :  // set Red Led
#endif // USE_VERBOSE
        i=1 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get mask
        outmask = GetHexValue(&usbbuf[i], 99) & 0xff ;

        // next par
        while(isxdigit((unsigned char)usbbuf[i])) i++ ;
        while((usbbuf[i] == ' ') || (usbbuf[i] == ',')) i++ ;

        // get period val
        outval = atoi(&usbbuf[i]) ;
        
        Set_LedBlinker(1, outmask, outval) ;
        break ;
        
// Added by FR
//	case 'P':
	case 'p':
		r = atoi(&usbbuf[1]) ;
		switch(r){
			case 160: // 0xA0 (special case -> write A1)
				dio_write(PORT_TW2, 0xFF00, (0xA1<<8) ) ; break ;
			case  56: // 0x38
				dio_write(PORT_TW2, 0xFF00, (0x38<<8) ) ; break ;
			case  24: // 0x18
				dio_write(PORT_TW2, 0xFF00, (0x18<<8) ) ; break ;
			case  10: // 0x0A
				dio_write(PORT_TW2, 0xFF00, (0x0A<<8) ) ; break ;
			case   2: // 0x02
				dio_write(PORT_TW2, 0xFF00, (0x02<<8) ) ; break ;
			case  60: // 0x3C
				dio_write(PORT_TW2, 0xFF00, (0x3C<<8) ) ; break ;
			case  28: // 0x1C
				dio_write(PORT_TW2, 0xFF00, (0x1C<<8) ) ; break ;
			case  14: // 0x0E
				dio_write(PORT_TW2, 0xFF00, (0x0E<<8) ) ; break ;
			case 130: // 0x82
				dio_write(PORT_TW2, 0xFF00, (0x82<<8) ) ; break ;
			case 194: // 0xC2
				dio_write(PORT_TW2, 0xFF00, (0xC2<<8) ) ; break ;
			default:break ;
		}
		break ;
		
	case 'U':        
		i = 2 ;
		while(usbbuf[i]){	// Scan all command
#ifdef USE_REAL_OUTPUTS
			switch (usbbuf[i]){
				case 'A':	// Analog channel
					printf("Set Analog") ;
					
						// Prepare digital
					// All digital output to Vext
					dio_write(PORT_TW3, 0xffff, 0xffff) ;
					// All digital floating
					dio_write(PORT_PIOB, PIOB_FLOAT, 0 ) ;
					// Disable counter
					dio_write(PORT_PIOB, PIOB_NOCOUNT, PIOB_NOCOUNT ) ;
					
					i++ ;
					if (usbbuf[i]=='V'){  // AD as voltage, I/O at Vext (PB6)
						printf(" Voltage ") ;
						dio_write(PORT_PIOB, PIOB_VOLTAMP, PIOB_VOLTAMP) ; 
					}else{
						printf(" Current ") ;
						dio_write(PORT_PIOB, PIOB_VOLTAMP, 0) ;
					}
					r = atoi(&usbbuf[i+1]) ;
					
					printf("Channel %d\n", r) ;
					if (r<7){
						dio_write(PORT_PIOB, PIOB_AN1AN2, PIOB_AN1AN2) ; 	// PB3
					}else{
						dio_write(PORT_PIOB, PIOB_AN1AN2, 0) ;
						r -= 6 ;
					}
					if (r & 0x1){ // If odd
						dio_write(PORT_PIOA, PIOA_CH0ANL, 0) ; // (PA23) (CH0)
					}else{
						dio_write(PORT_PIOA, PIOA_CH0ANL, PIOA_CH0ANL) ;
						
					}
					switch(r){
						case 1:
						case 2:
							dio_write(PORT_PIOA, (PIOB_CH1ANL | PIOB_CH2ANL), 0) ;// (PB1,PB9)
							break ;
						case 3:
						case 4:
							dio_write(PORT_PIOA, (PIOB_CH1ANL | PIOB_CH2ANL), PIOB_CH1ANL) ;// (PB1,PB9)
							break ;
						case 5:
						case 6:
							dio_write(PORT_PIOA, (PIOB_CH1ANL | PIOB_CH2ANL), PIOB_CH2ANL) ;// (PB1,PB9)
							break ;
						case 0:
						case 7:
						case 8:
							dio_write(PORT_PIOA, (PIOB_CH1ANL | PIOB_CH2ANL), (PIOB_CH1ANL | PIOB_CH2ANL)) ;// (PB1,PB9)
							break ;

					}
					break ;
				case 'I':	// Digital channel out (In for MTS)
					i++ ;
					// ship 'V' or 'A'
					i++ ;
					
					r = atoi(&usbbuf[i]) ;
					while(usbbuf[i] != ' ' )  i++ ;       // search next blank
					while(usbbuf[i] == ' ' )  i++ ;       // skip blank
					outval = atoi(&usbbuf[i]) ;
					if (outval<0){
						// All output set to 0 (out 1) become floating
						// All output set to 1 (out 0) no change
						dio_write(PORT_PIOB, PIOB_FLOAT, 0) ; 
					}else{
						dio_write(PORT_PIOB, PIOB_FLOAT, PIOB_FLOAT) ; 
						if (outval)
							dio_write(PORT_TW3, (1<<r), 0 ) ;
						else
							dio_write(PORT_TW3, (1<<r), (1<<r) ) ;
					}
					
					break ;
				case 'P':	// presence
					i++ ;
					if (usbbuf[i]=='1')
						dio_write(PORT_PIOB, PIOB_MTSPRES, PIOB_MTSPRES) ;	// PB2 to 1
					else
						dio_write(PORT_PIOB, PIOB_MTSPRES, 0) ; 			// PB2 to 0
					break ;
				case 'V':	// Vext
					i++ ;
					if (usbbuf[i]=='0')
						dio_write(PORT_PIOB, PIOB_PON, PIOB_PON) ;	// PB5 to 1
					else
						dio_write(PORT_PIOB, PIOB_PON, 0) ;			// PB5 to 0
					break ;
			}
#endif // #ifdef USE_REAL_OUTPUTS
			i++ ;
		}
		break ;
    }
}

