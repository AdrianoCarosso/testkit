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
#include "assign.h"

#include <extapi.h>

// -----------------------------------------------------------------------
// local data

int GlobTimer ;         // spontaneous data dump
int GlobCurrTimer ;

#ifdef USE_CAN_ON_ARM
int CanTimer[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES] ;         // spontaneous data dump
int CanCurrTimer[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES] ;
int CanLenMsg[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES] ;
//char CanMsg[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES][8] ;
unsigned char CanMsg[CAN_TOT_CHANNELS][CAN_TOT_MAILBOXES][8] ;
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

void tk0extender(void) {
    static char usbdata[128] ;          // buffer of data from USB
    int usbidx ;                        // index of buffer
    unsigned long lpmget ;
    static const SEMA semalist[] = {LU0TSEM, TK0ISEM, LU0QSEM,
#ifdef USE_CAN_ON_ARM
                                    CAN0SEM, CAN1SEM,
#endif // USE_CAN_ON_ARM
                                    PORT0SEM,
#if !defined(USE_TASK0_SERIAL) || !defined(USE_REAL_BOARD)
                                    PORT1SEM,
#endif // !defined(USE_TASK0_SERIAL) || !defined(USE_REAL_BOARD)
                                    PORT2SEM,
#if !defined(USE_TASK0_SERIAL) || !defined(USE_EVALUATION_BOARD)
                                    PORT3SEM,
#endif // !defined(USE_TASK0_SERIAL) || !defined(USE_EVALUATION_BOARD)
                                    0} ;
    static CLKBLK clkblk ;              // our timer pointer
    SEMA cause ;                        // wake up reason
    char lastch, c ;
#ifdef USE_CAN_ON_ARM
    int i, j ;
    unsigned long canstat ;
#endif // USE_CAN_ON_ARM

    // Peripheral Clock Enable Register for PIO: already done in extapi.c EKS_init()

    //printf("Start: 0x%x\n", AT91C_BASE_RSTC->RSTC_RSR) ;

    //KS_signal(CBUGSEMA) ;        // wake up debugger now

    // settling time
    KS_delay(SELFTASK, ((TICKS)200*CLKRATE/1000)) ;     // skip time
    Set_LedBlinker(0, 0x0f0f0f0f, 50) ;

    dio_write(PORT_TW1, 0x80, 0x80) ;
    dio_write(PORT_PIOB, 0x24, 0x00) ;
    dio_write(PORT_PIOB, PIOB_MTS3V_OFF, 0x00) ;
    dio_write(PORT_TW1, 0x80, 0x80) ;

    // Init semaphore and timer

    KS_defqsema(LU0Q, LU0QSEM, QNE) ;           // wake up on no empty
    KS_defqsema(TK0IPORT, TK0ISEM, QNE) ;       // wake up on no empty

    while(KS_dequeue(COM0IQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(COM0IQ, PORT0SEM, QNE) ;        // wake up on no empty

#if !defined(USE_TASK0_SERIAL) || !defined(USE_REAL_BOARD)
    while(KS_dequeue(COM1IQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(COM1IQ, PORT1SEM, QNE) ;        // wake up on no empty
#endif // !defined(USE_TASK0_SERIAL) || !defined(USE_REAL_BOARD)


    while(KS_dequeue(COM2IQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(COM2IQ, PORT2SEM, QNE) ;        // wake up on no empty

#if !defined(USE_TASK0_SERIAL) || !defined(USE_EVALUATION_BOARD)
    while(KS_dequeue(COM3IQ, &c)==RC_GOOD) ;    // flush
    KS_defqsema(COM3IQ, PORT3SEM, QNE) ;        // wake up on no empty
#endif // !defined(USE_TASK0_SERIAL) || !defined(USE_EVALUATION_BOARD)

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

            break ;

        case PORT0SEM :                // serial port -0-
            printf("C0=") ;
            while(KS_dequeue(COM0IQ, &c)==RC_GOOD) {
                printf("%02X", (c & 0xff)) ;
            }
            printf("\n") ;
            break ;

#if !defined(USE_TASK0_SERIAL) || !defined(USE_REAL_BOARD)
        case PORT1SEM :                // serial port -1-
            printf("C1=") ;
            while(KS_dequeue(COM1IQ, &c)==RC_GOOD) {
                printf("%02X", (c & 0xff)) ;
            }
            printf("\n") ;
            break ;
#endif // USE_TASK0_SERIAL

        case PORT2SEM :                // serial port -2-
            printf("C2=") ;
            while(KS_dequeue(COM2IQ, &c)==RC_GOOD) {
                printf("%02X", (c & 0xff)) ;
            }
            printf("\n") ;
            break ;

#if !defined(USE_TASK0_SERIAL) || !defined(USE_EVALUATION_BOARD)
        case PORT3SEM :                // serial port -3-
            printf("C3=") ;
            while(KS_dequeue(COM3IQ, &c)==RC_GOOD) {
                printf("%02X", (c & 0xff)) ;
            }
            printf("\n") ;
            break ;
#endif // USE_TASK0_SERIAL

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

    while( (isxdigit(*cp)) &&
           ((value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < BASE) &&
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
void HandleCanRead(int chn, int mbx)
{
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

  switch(usbbuf[0]) {
    #if defined(USE_PARAMETERS_ON_TWI) || defined(USE_PARAMETERS_ON_FLASH) || defined(USE_PARAMETERS_ON_EEPROM)
    //USE_PARAMETERS_ON_FLASH
    case 'V' :  // version
        psetup = EKS_GetSetup() ;
        printf("V=%d.%02d;%ld\n", SOFTREL, SUBSREL, psetup->sernum) ;
        break ;
    #endif

	case 'H':
	case 'h':
		printf(" Available commands are:\r\n") ;
		printf(" Ax <BaudRate>                                 Ser CANx baud rate\r\n") ;
		printf(" Bx <BaudRate>,<Par>,<Bits>,<Stop>,<Handshake> Set COMx options\r\n") ;
		printf(" Cx <HexData>                                  Send to COMx port binary values from <HexData> string\r\n") ;
		printf(" D                                             Dump <Digital>;<Analog>;<Counter>\r\n") ;
		printf(" Exy <HexData>                                 Emit CANx MAILy\r\n") ;
		printf(" F                                             Check CAN flags\r\n") ;
		printf(" H                                             This help\r\n") ;
		printf(" L <HexMask>,<period_ms>                       Set Red Led Mask and period in [ms]\r\n") ;
		printf(" Mxy <Mask>,<Addr>,<Extended>,<Period>         Set CANx MAILy (Mask = ffffffff means TX)\r\n") ;
		printf(" S                                             Software upgrade record (srec)\r\n") ;
		printf(" T <Interval>                                  Set spontaneus Dump command every <Interval> msec (0=never)\r\n") ;
		printf(" U A[V|C]x/I[V|C]x/V/Px                        User fast commands:\r\n") ;
		printf("                                                   Ax: Anx (1-6 if x=0 all to 0)\r\n") ;
		printf("                                                   Ix: Inx (1-8 if x=0 all to 1)\r\n") ;
		printf("                                                   Vx Vext 1 or 0\r\n") ;
		printf("                                                   Px Presence 1 or 0\r\n") ;
		printf(" V                                             Version\r\n") ;
		printf(" X <Port>,<HexMask>,<HexVal>                   Set digital output <HexMask> of <Port> to <HexVal>\r\n") ;		
		printf("\r\n Every command is composed by a <CR> terminated string\r\n") ;
		break ;
		
    case 'v' :  // version
        psetup = EKS_GetSetup() ;
        printf("Version %d.%02d, S/N %ld\n", SOFTREL, SUBSREL, psetup->sernum) ;
        break ;

    case 'D' :  // dump
        // digital I/O
        printf("D=%08lx,%08lx,%08lx,%08lx;", dio_read(PORT_PIOA), dio_read(PORT_PIOB),
                                             dio_read(PORT_TW1), dio_read(PORT_TW2)) ;
        // read analog inputs from ADC -0- and -1-
        for(i=0 ; i<19 ; i++) {
            if (i) printf(",") ;
            printf("%x", ADC_read(i)) ;
        }
        printf(";") ;
        // read counters
        printf("%x,%x,%x\n", dio_counter(CNT_PPS), dio_counter(CNT_ODOMETER), rtctick) ;
        break ;

    case 'd' :  // dump
        // digital I/O
        printf("PIOA=0x%08lx, PIOB=0x%08lx, TW1=0x%08lx, TW2=0x%08lx\n", dio_read(PORT_PIOA), dio_read(PORT_PIOB),
                                                                         dio_read(PORT_TW1), dio_read(PORT_TW2)) ;
        // read analog inputs from ADC -0- and -1-
        for(i=0 ; i<19 ; i++) {
            if (i) printf(", ") ;
            printf("A%d=%d", i, ADC_read(i)) ;
        }
        printf("\n") ;
        // read counters
        printf("Cpps=%d, Codom=%d, Tick=%d\n", dio_counter(CNT_PPS), dio_counter(CNT_ODOMETER), rtctick) ;
        break ;

    case 'C' :  // serial
    case 'c' :  // serial
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
    case 't' :  // timer
        i=1 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;
        GlobTimer = atoi(&usbbuf[i]) ;  // base 10 in verbose mode
        GlobCurrTimer = GlobTimer ;
        break ;

    case 'X' :  // set output
    case 'x' :  // set output
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
        while(isxdigit(usbbuf[i])) i++ ;
        while((usbbuf[i] == ' ') || (usbbuf[i] == ',')) i++ ;

        // get val
        outval = GetHexValue(&usbbuf[i], 99) ;
        
        // printf("Mask=0x%08x, Val=0x%08x\n", outmask, outval) ;

        dio_write(r, outmask, outval) ;
//        // set at 0
//        AT91C_BASE_PIOB->PIO_CODR = outmask & (~outval) ;       // set at 0
//        // set at 1
//        AT91C_BASE_PIOB->PIO_SODR = outmask & outval ;          // set at 1
        break ;

    case 'B' :  // set baud
    case 'b' :  // set baud
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
    case 'a' :  // set CAN baud
        i=2 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get baudrate
        baudrate = atoi(&usbbuf[i]) ;

        // set new parameter
        i = usbbuf[1] - '0' ;   // com number
        if ((i>=0) && (i<=CAN_TOT_CHANNELS)) {
            dio_write(1, PIOB_CAN_OFF, PIOB_CAN_OFF) ;
            CAN_speed(/* chn = */ i, baudrate, NO /*YES*/) ; // YES if no TX
            //printf("A %d - %d\n", i, baudrate) ;
        }
        break ;

    case 'F' :  // check CAN flags
        chn = GetHexValue(&usbbuf[1], 1) & 1 ;
        printf("F%d=%x\n", chn, CanStatus[chn]) ;
        break ;
    case 'f' :  // check CAN flags
        chn = GetHexValue(&usbbuf[1], 1) & 1 ;
        printf("CanStatus[%d] %x\n", chn, CanStatus[chn]) ;
        break ;

    case 'M' :  // set CAN MAILBOX
    case 'm' :  // set CAN MAILBOX
        chn = GetHexValue(&usbbuf[1], 1) & 1 ;
        mbx = GetHexValue(&usbbuf[2], 1) ;
        
        i=3 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get mask
        outmask = GetHexValue(&usbbuf[i], 99) ;

        // next par
        while(isxdigit(usbbuf[i])) i++ ;
        while((usbbuf[i] == ' ') || (usbbuf[i] == ',')) i++ ;

        // get val
        outval = GetHexValue(&usbbuf[i], 99) ;

        // next par
        while(isxdigit(usbbuf[i])) i++ ;
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
    case 'e' :  // emit CAN MAILBOX
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
    case 'l' :  // set Red Led
        i=1 ;   // start from here
        while((usbbuf[i] == ' ') || (usbbuf[i] == '=')) i++ ;

        // get mask
        outmask = GetHexValue(&usbbuf[i], 99) & 0xff ;

        // next par
        while(isxdigit(usbbuf[i])) i++ ;
        while((usbbuf[i] == ' ') || (usbbuf[i] == ',')) i++ ;

        // get period val
        outval = atoi(&usbbuf[i]) ;
        
        Set_LedBlinker(1, outmask, outval) ;
        break ;
        
// Added by FR
	case 'U':        
		i = 2 ;
		while(usbbuf[i]){	// Scan all command
			switch (usbbuf[i]){
				case 'A':	// Analog channel
				case 'I':	// Digital channel out (In for MTS)
					if (usbbuf[i]=='A'){
						dio_write(1, 0x8, 0x8) ; // First set Analog (PB3 to 1)
						chn = 0 ;
					}else{
						dio_write(1, 0x8, 0x0) ; // First set Digital (PB3 to 0)
						chn = 1 ;
					}
					i++ ;
					if (usbbuf[i]=='V'){  // AD as voltage, I/O at Vext (A30)
						dio_write(0, 0x40000000, 0x40000000) ; 
					}else{
						dio_write(0, 0x40000000, 0x0) ;
					}
					i++ ;
					r = atoi(&usbbuf[i]) ;
					if (chn) r = 9-r ;
					i++ ;
					if (r & 0x1){ // If odd
						dio_write(0, 0x800000, 0x800000) ;
					}else{
						dio_write(0, 0x800000, 0x0) ;
					}
					switch(r){
						case 1:
						case 2:
							dio_write(1, 0x202, 0x202) ;
							break ;
						case 3:
						case 4:
							dio_write(1, 0x202, 0x2) ;
							break ;
						case 5:
						case 6:
							dio_write(1, 0x202, 0x200) ;
							break ;
						case 7:
						case 8:
						case 0:
							dio_write(1, 0x202, 0x0) ;
							break ;

					}
					break ;
				case 'P':	// presence
					i++ ;
					if (usbbuf[i]=='0')
						dio_write(1, 0x4, 0x4) ; // pioB 2 to 1
					else
						dio_write(1, 0x4, 0x0) ; // pioB 2 to 0
					break ;
				case 'V':	// Vext
					i++ ;
					if (usbbuf[i]=='0')
						dio_write(1, 0x20, 0x20) ; // pioB 5 to 1
					else
						dio_write(1, 0x20, 0x0) ; // pioB 5 to 0
					break ;
			}
			i++ ;
		}
		break ;
    }
}

