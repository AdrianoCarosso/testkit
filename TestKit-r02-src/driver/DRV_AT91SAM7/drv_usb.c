// drv_usb.c - USB driver tasks
//
//   Copyright (c) 1997-2006.
//   T.E.S.T. srl
//

#include <stdio_console.h>

#include "rtxcapi.h"
#include "enable.h"

#include "cclock.h"
#include "csema.h"
#include "cqueue.h"

#include "extapi.h"

#include "assign.h"

//----------------------------------------------------------------------------
// only if we are well accepted
#ifdef USE_USB_ON_ARM

//----------------------------------------------------------------------------
// who we are

#define NULLSEMA ((SEMA)0)

//----------------------------------------------------------------------------
// functions in drv_clk.c

extern void AT91F_AIC_Configure(int irq_id, int priority, int src_type, FRAME *(*newHandler)(FRAME *frame)) ;

//----------------------------------------------------------------------------
// internal functions

void USB_SendData(char *pData, unsigned int length) ;
void USB_SendNextData(void) ;
void USB_SendStall(void) ;
void USB_SendZlp(void) ;
void USB_RxData(unsigned long bank) ;
void USB_RxSetup(void) ;

void usbtask(void) TASK_ATTRIBUTE ;

void USB_Receive(void) ;
int USB_Transmit(int first) ;

FRAME *usbdrv(FRAME * frame) ;

//----------------------------------------------------------------------------
// internal data

unsigned char currConfiguration ;       // alias of: IsConfigured
unsigned char currConnection ;

unsigned short usberr ;                 // error counter

unsigned char * currDataPtr ;           // data pointer
unsigned int  currDataLen ;             // data len

unsigned int  currRcvBank ;

#define AT91C_EP_OUT            1       // our Rx channel
#define AT91C_EP_OUT_SIZE       0x40
#define AT91C_EP_IN             2       // our Tx channel
#define AT91C_EP_IN_SIZE        0x40

const char devDescriptor[] = {
    /* Device descriptor */
    0x12,   // bLength
    0x01,   // bDescriptorType
    0x10,   // bcdUSBL
    0x01,   //
    0x02,   // bDeviceClass:    CDC class code
    0x00,   // bDeviceSubclass: CDC class sub code
    0x00,   // bDeviceProtocol: CDC Device protocol
    0x08,   // bMaxPacketSize0
    0xEB,   // idVendorL
    0x03,   //
    0x24,   // idProductL
    0x61,   //
    0x10,   // bcdDeviceL
    0x01,   //
    0x00,   // iManufacturer    // 0x01
    0x00,   // iProduct
    0x00,   // SerialNumber
    0x01    // bNumConfigs
} ;

const char cfgDescriptor[] = {
    /* ============== CONFIGURATION 1 =========== */
    /* Configuration 1 descriptor */
    0x09,   // CbLength
    0x02,   // CbDescriptorType
    0x43,   // CwTotalLength 2 EP + Control
    0x00,
    0x02,   // CbNumInterfaces
    0x01,   // CbConfigurationValue
    0x00,   // CiConfiguration
    0xC0,   // CbmAttributes 0xA0
    0x00,   // CMaxPower

    /* Communication Class Interface Descriptor Requirement */
    0x09, // bLength
    0x04, // bDescriptorType    Interface descriptor
    0x00, // bInterfaceNumber   Interface number 0
    0x00, // bAlternateSetting
    0x01, // bNumEndpoints
    0x02, // bInterfaceClass    2 = Communications Device Class
    0x02, // bInterfaceSubclass 2 = Abstract Control Model
    0x00, // bInterfaceProtocol 0 = No class specific protocol required
    0x00, // iInterface

    /* Header Functional Descriptor */
    0x05, // bFunction Length
    0x24, // bDescriptor type: CS_INTERFACE
    0x00, // bDescriptor subtype: Header Func Desc
    0x10, // bcdCDC:1.1
    0x01,

    /* ACM Functional Descriptor */
    0x04, // bFunctionLength
    0x24, // bDescriptor Type: CS_INTERFACE
    0x02, // bDescriptor Subtype: ACM Func Desc
    0x00, // bmCapabilities

    /* Union Functional Descriptor */
    0x05, // bFunctionLength
    0x24, // bDescriptorType: CS_INTERFACE
    0x06, // bDescriptor Subtype: Union Func Desc
    0x00, // bMasterInterface: Communication Class Interface
    0x01, // bSlaveInterface0: Data Class Interface

    /* Call Management Functional Descriptor */
    0x05, // bFunctionLength
    0x24, // bDescriptor Type: CS_INTERFACE
    0x01, // bDescriptor Subtype: Call Management Func Desc
    0x00, // bmCapabilities: D1 + D0
    0x01, // bDataInterface: Data Class Interface 1

    /* Endpoint 3 descriptor */
    0x07,   // bLength
    0x05,   // bDescriptorType
    0x83,   // bEndpointAddress, Endpoint 03 - IN
    0x03,   // bmAttributes      INT
    0x08,   // wMaxPacketSize
    0x00,
    0xFF,   // bInterval

    /* Data Class Interface Descriptor Requirement */
    0x09, // bLength
    0x04, // bDescriptorType
    0x01, // bInterfaceNumber
    0x00, // bAlternateSetting
    0x02, // bNumEndpoints
    0x0A, // bInterfaceClass    // 0a = CDC data
    0x00, // bInterfaceSubclass
    0x00, // bInterfaceProtocol
    0x00, // iInterface

    /* First alternate setting */
    /* Endpoint 1 descriptor */
    0x07,   // bLength
    0x05,   // bDescriptorType
    0x01,   // bEndpointAddress, Endpoint 01 - OUT
    0x02,   // bmAttributes      BULK
    AT91C_EP_OUT_SIZE,   // wMaxPacketSize
    0x00,
    0x00,   // bInterval

    /* Endpoint 2 descriptor */
    0x07,   // bLength
    0x05,   // bDescriptorType
    0x82,   // bEndpointAddress, Endpoint 02 - IN
    0x02,   // bmAttributes      BULK
    AT91C_EP_IN_SIZE,   // wMaxPacketSize
    0x00,
    0x00    // bInterval
} ;

/* USB standard request code */
#define STD_GET_STATUS_ZERO           0x0080
#define STD_GET_STATUS_INTERFACE      0x0081
#define STD_GET_STATUS_ENDPOINT       0x0082

#define STD_CLEAR_FEATURE_ZERO        0x0100
#define STD_CLEAR_FEATURE_INTERFACE   0x0101
#define STD_CLEAR_FEATURE_ENDPOINT    0x0102

#define STD_SET_FEATURE_ZERO          0x0300
#define STD_SET_FEATURE_INTERFACE     0x0301
#define STD_SET_FEATURE_ENDPOINT      0x0302

#define STD_SET_ADDRESS               0x0500
#define STD_GET_DESCRIPTOR            0x0680
#define STD_SET_DESCRIPTOR            0x0700
#define STD_GET_CONFIGURATION         0x0880
#define STD_SET_CONFIGURATION         0x0900
#define STD_GET_INTERFACE             0x0A81
#define STD_SET_INTERFACE             0x0B01
#define STD_SYNCH_FRAME               0x0C82

/* CDC Class Specific Request Code */
#define GET_LINE_CODING               0x21A1
#define SET_LINE_CODING               0x2021
#define SET_CONTROL_LINE_STATE        0x2221


typedef struct {
    unsigned int dwDTERRate;
    char bCharFormat;
    char bParityType;
    char bDataBits;
} __attribute__ ((packed)) AT91S_CDC_LINE_CODING, *AT91PS_CDC_LINE_CODING ;

const AT91S_CDC_LINE_CODING line_pars = {
    115200, // baudrate
    0,      // 1 Stop Bit
    0,      // None Parity
    8       // 8 Data bits
} ;


#define PRINT_BYTE(A) { while (!(AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_TXRDY)) ; \
                        AT91C_BASE_DBGU->DBGU_THR = (A) ; }

#define PRINT_OCTAL(A) { PRINT_BYTE('0'+(((A)>>6)&3)); PRINT_BYTE('0'+(((A)>>3)&7)); PRINT_BYTE('0'+(((A)>>0)&7)); }

// debug functions
//PRINT_BYTE('\r') ; PRINT_BYTE('\n') ;
//PRINT_BYTE('S') ; PRINT_BYTE('=') ; PRINT_OCTAL(isr>>8) ; PRINT_BYTE(' ') ;
//PRINT_BYTE('s') ; PRINT_BYTE('=') ; PRINT_OCTAL(isr) ; PRINT_BYTE(' ') ;
//PRINT_BYTE('\r') ; PRINT_BYTE('\n') ;

//----------------------------------------------------------------------------
// Interrupt routine for USB

FRAME *usbdrv(FRAME * frame)
{
    unsigned long isr = AT91C_BASE_UDP->UDP_ISR ;

// Interrupts enabled from 21/Aug/2006
//    ENABLE ;		// open interrupt window

    if (isr & AT91C_UDP_ENDBUSRES) {
        // reset interrupt source
//        AT91C_BASE_UDP->UDP_ICR = AT91C_UDP_ENDBUSRES ;

        // Enable the function
        AT91C_BASE_UDP->UDP_FADDR = AT91C_UDP_FEN ;

        // Configure endpoint 0
        AT91C_BASE_UDP->UDP_CSR[0] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL) ;
        
        // enable interrupt for endpoint 0
        AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT0 ;
    }
    
    // reset all interrupt sources
    AT91C_BASE_UDP->UDP_ICR = isr ;

    // purge handled signals
    isr &= (AT91C_BASE_UDP->UDP_IMR) ;
    
    // check for data
    if (isr & AT91C_UDP_EPINT0) {
        // disable interrupt for endpoint 0
        AT91C_BASE_UDP->UDP_IDR = AT91C_UDP_EPINT0 ;

        return(KS_ISRexit(frame, USBSEM0)) ;
    }
    if (isr & AT91C_UDP_EPINT1) {
        // disable interrupt for endpoint 1
        AT91C_BASE_UDP->UDP_IDR = AT91C_UDP_EPINT1 ;

        return(KS_ISRexit(frame, USBSEM1)) ;
    }
    if (isr & AT91C_UDP_EPINT2) {
        // disable interrupt for endpoint 2
        AT91C_BASE_UDP->UDP_IDR = AT91C_UDP_EPINT2 ;

        return(KS_ISRexit(frame, USBSEM2)) ;
    }

    return(KS_ISRexit(frame, NULLSEMA)) ;
}

//----------------------------------------------------------------------------
// USB_SendNextData

void USB_SendNextData(void)
{
    unsigned int cpt ;

    // Correct Tx Acknowledge
    AT91C_BASE_UDP->UDP_CSR[0] &= ~(AT91C_UDP_TXCOMP) ;

    // check for end of Tx
    if (!currDataLen) {
        return ;
    }

    cpt = MIN(currDataLen, 8) ;
    currDataLen -= cpt ;

    // write in FIFO
    while (cpt--)
        AT91C_BASE_UDP->UDP_FDR[0] = *currDataPtr++ ;

    // toss out
    AT91C_BASE_UDP->UDP_CSR[0] |= AT91C_UDP_TXPKTRDY ;
}

//----------------------------------------------------------------------------
// USB_RxData

void USB_RxData(unsigned long bank)
{
    int len ;

    len = (AT91C_BASE_UDP->UDP_CSR[0] >> 16) ;

    // pdebugt(1,SELFTASK, "USB_RxData len=%d", len) ;

    // Data acknowledge
    AT91C_BASE_UDP->UDP_CSR[0] &= ~bank ;
}

//----------------------------------------------------------------------------
// USB_SendData
// Send Data through the control endpoint

void USB_SendData(char *pData, unsigned int length)
{
    unsigned int cpt ;

    cpt = MIN(length, 8) ;
    length -= cpt ;
    
    // write in FIFO
    while (cpt--)
        AT91C_BASE_UDP->UDP_FDR[0] = *pData++ ;

    // start from here
    currDataLen = length ;
    currDataPtr = (unsigned char *)(pData) ;

    // toss out
    AT91C_BASE_UDP->UDP_CSR[0] |= AT91C_UDP_TXPKTRDY ;
}

//----------------------------------------------------------------------------
// USB_SendZlp
// Send zero length packet through the control endpoint

void USB_SendZlp(void)
{
    AT91C_BASE_UDP->UDP_CSR[0] |= AT91C_UDP_TXPKTRDY ;
    while ( !(AT91C_BASE_UDP->UDP_CSR[0] & AT91C_UDP_TXCOMP) ) ;
    AT91C_BASE_UDP->UDP_CSR[0] &= ~(AT91C_UDP_TXCOMP) ;
    while (AT91C_BASE_UDP->UDP_CSR[0] & AT91C_UDP_TXCOMP) ;
}

//----------------------------------------------------------------------------
// USB_SendStall
// Stall the control endpoint

void USB_SendStall(void)
{
    AT91C_BASE_UDP->UDP_CSR[0] |= AT91C_UDP_FORCESTALL ;
    while ( !(AT91C_BASE_UDP->UDP_CSR[0] & AT91C_UDP_ISOERROR) ) ;
    AT91C_BASE_UDP->UDP_CSR[0] &= ~(AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR) ;
    while (AT91C_BASE_UDP->UDP_CSR[0] & (AT91C_UDP_FORCESTALL | AT91C_UDP_ISOERROR)) ;
}

//----------------------------------------------------------------------------
// USB_RxSetup

void USB_RxSetup(void)
{
    unsigned char bmRequestType, bRequest ;
    unsigned short wValue, wIndex, wLength, wStatus ;

    // read Device Request data
    bmRequestType = AT91C_BASE_UDP->UDP_FDR[0];
    bRequest      = AT91C_BASE_UDP->UDP_FDR[0];
    wValue        = (AT91C_BASE_UDP->UDP_FDR[0] & 0xFF);
    wValue       |= (AT91C_BASE_UDP->UDP_FDR[0] << 8);
    wIndex        = (AT91C_BASE_UDP->UDP_FDR[0] & 0xFF);
    wIndex       |= (AT91C_BASE_UDP->UDP_FDR[0] << 8);
    wLength       = (AT91C_BASE_UDP->UDP_FDR[0] & 0xFF);
    wLength      |= (AT91C_BASE_UDP->UDP_FDR[0] << 8);

    //pdebugt(1,SELFTASK, "USB_RxSetup: bmRequestType=0x%x, bRequest=0x%x, wValue=0x%x, wIndex=0x%x, wLength=0x%x",
    //                  bmRequestType, bRequest, wValue, wIndex, wLength) ;

    // set direction according to Device Request
    if (bmRequestType & 0x80) {
        AT91C_BASE_UDP->UDP_CSR[0] |= AT91C_UDP_DIR;
    }
    // Device Request acknowledge
    AT91C_BASE_UDP->UDP_CSR[0] &= ~AT91C_UDP_RXSETUP;

    // Handle supported standard device request Cf Table 9-3 in USB specification Rev 1.1
    switch((bRequest << 8) | bmRequestType) {
    case STD_GET_DESCRIPTOR :
        if (wValue == 0x100)       // Return Device Descriptor
            USB_SendData((char *)devDescriptor, MIN(sizeof(devDescriptor), wLength)) ;
        else if (wValue == 0x200)  // Return Configuration Descriptor
            USB_SendData((char *)cfgDescriptor, MIN(sizeof(cfgDescriptor), wLength)) ;
        else
            USB_SendStall() ;
        break ;

    case STD_SET_ADDRESS :
        USB_SendZlp() ;
        AT91C_BASE_UDP->UDP_FADDR = (AT91C_UDP_FEN | wValue) ;
        AT91C_BASE_UDP->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_FADDEN : 0 ;
        break ;

    case STD_SET_CONFIGURATION:
        currConfiguration = wValue ;
        USB_SendZlp() ;
        AT91C_BASE_UDP->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_CONFG : AT91C_UDP_FADDEN ;
        AT91C_BASE_UDP->UDP_CSR[1] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT) : 0 ;
        AT91C_BASE_UDP->UDP_CSR[2] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN)  : 0 ;
        AT91C_BASE_UDP->UDP_CSR[3] = (wValue) ? (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_INT_IN)   : 0 ;
        // enable their interrupt
        AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT1 | AT91C_UDP_EPINT2 ;
        break ;

    case STD_GET_CONFIGURATION:
        USB_SendData((char *) &currConfiguration, sizeof(currConfiguration));
        break ;

    case STD_GET_STATUS_ZERO:
        wStatus = 0 ;
        USB_SendData((char *) &wStatus, sizeof(wStatus)) ;
        break ;

    case STD_GET_STATUS_INTERFACE:
        wStatus = 0 ;
        USB_SendData((char *) &wStatus, sizeof(wStatus)) ;
        break ;

    case STD_GET_STATUS_ENDPOINT:
        wStatus = 0 ;
        wIndex &= 0x0F ;
        if ((AT91C_BASE_UDP->UDP_GLBSTATE & AT91C_UDP_CONFG) && (wIndex <= 3)) {
            wStatus = (AT91C_BASE_UDP->UDP_CSR[wIndex] & AT91C_UDP_EPEDS) ? 0 : 1;
            USB_SendData((char *) &wStatus, sizeof(wStatus));
        } else if ((AT91C_BASE_UDP->UDP_GLBSTATE & AT91C_UDP_FADDEN) && (wIndex == 0)) {
            wStatus = (AT91C_BASE_UDP->UDP_CSR[wIndex] & AT91C_UDP_EPEDS) ? 0 : 1;
            USB_SendData((char *) &wStatus, sizeof(wStatus));
        } else {
            USB_SendStall() ;
        }
        break ;

    case STD_SET_FEATURE_ZERO:
        USB_SendStall() ;
        break ;

    case STD_SET_FEATURE_INTERFACE:
        USB_SendZlp() ;
        break ;

    case STD_SET_FEATURE_ENDPOINT:
        wIndex &= 0x0F ;
        if ((wValue == 0) && wIndex && (wIndex <= 3)) {
            AT91C_BASE_UDP->UDP_CSR[wIndex] = 0 ;
            USB_SendZlp() ;
        } else {
            USB_SendStall() ;
        }
        break ;

    case STD_CLEAR_FEATURE_ZERO:
        USB_SendStall() ;
        break ;

    case STD_CLEAR_FEATURE_INTERFACE:
        USB_SendZlp() ;
        break ;

    case STD_CLEAR_FEATURE_ENDPOINT:
        wIndex &= 0x0F ;
        if ((wValue == 0) && wIndex && (wIndex <= 3)) {
            if (wIndex == 1) {
                AT91C_BASE_UDP->UDP_CSR[1] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT) ;
            } else if (wIndex == 2) {
                AT91C_BASE_UDP->UDP_CSR[2] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN) ;
            } else if (wIndex == 3) {
                AT91C_BASE_UDP->UDP_CSR[3] = (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_INT_IN) ;
            }
            USB_SendZlp() ;
        } else {
            USB_SendStall() ;
        }
        break ;

    // handle CDC class requests
    case SET_LINE_CODING:
        while ( !(AT91C_BASE_UDP->UDP_CSR[0] & AT91C_UDP_RX_DATA_BK0) ) ;
        AT91C_BASE_UDP->UDP_CSR[0] &= ~(AT91C_UDP_RX_DATA_BK0) ;
        USB_SendZlp() ;
        break ;

    case GET_LINE_CODING:
        USB_SendData((char *) &line_pars, MIN(sizeof(line_pars), wLength)) ;
        break ;

    case SET_CONTROL_LINE_STATE:
        currConnection = wValue ;
        USB_SendZlp() ;
        break ;

    default:
        USB_SendStall() ;
        break ;
    }
}

//----------------------------------------------------------------------------
// USB_Receive

void USB_Receive(void)
{
    int packetSize ;
    unsigned char pData ;
    
    if ( AT91C_BASE_UDP->UDP_CSR[AT91C_EP_OUT] & currRcvBank ) {
        // get len
        packetSize = AT91C_BASE_UDP->UDP_CSR[AT91C_EP_OUT] >> 16 ;
        
        while(packetSize--) {
            pData = AT91C_BASE_UDP->UDP_FDR[AT91C_EP_OUT] ;
            if (KS_enqueuet(USBIQ, &pData, ((TICKS)500*CLKRATE/1000)) != RC_GOOD) {
                // one more error
                usberr++ ;
            }
        }
        
        // handled
        AT91C_BASE_UDP->UDP_CSR[AT91C_EP_OUT] &= ~(currRcvBank) ;
        // ready for next
        if (currRcvBank == AT91C_UDP_RX_DATA_BK0)
            currRcvBank = AT91C_UDP_RX_DATA_BK1 ;
        else
            currRcvBank = AT91C_UDP_RX_DATA_BK0 ;
    }
}

//----------------------------------------------------------------------------
// USB_Transmit

int USB_Transmit(int first)
{
    int i ;
    unsigned char pData ;

    if (AT91C_BASE_UDP->UDP_CSR[AT91C_EP_IN] & AT91C_UDP_TXCOMP) {
        // Correct Tx Acknowledge
        AT91C_BASE_UDP->UDP_CSR[AT91C_EP_IN] &= ~(AT91C_UDP_TXCOMP) ;
    }
    
    for(i=0 ; i<AT91C_EP_IN_SIZE ; i++) {
        if (KS_dequeue(USBOQ, &pData) == RC_GOOD) {
            if (currConfiguration) {    // only if configured
                AT91C_BASE_UDP->UDP_FDR[AT91C_EP_IN] = pData ;
            }
        } else {
            break ;     // no more data
        }
    }
    
    if ((i) && (currConfiguration)) {
        // packet transmission
        AT91C_BASE_UDP->UDP_CSR[AT91C_EP_IN] |= AT91C_UDP_TXPKTRDY ;
    }
    
    //pdebugt(1,SELFTASK, "USB_Transmit first=%d, len=%d", first, i) ;
    //pdebugt(1,SELFTASK, "ISR=0x%04x, IMR=0x%04x, CSR[2]=0x%08x", AT91C_BASE_UDP->UDP_ISR, AT91C_BASE_UDP->UDP_IMR, AT91C_BASE_UDP->UDP_CSR[2]) ;
    return(i) ;
}

//----------------------------------------------------------------------------
// USB task

void usbtask(void)
{
    // Item n.3 is modified according to queue wake-up or not
    SEMA semalist[5] = {USBSEM0, USBSEM2, USBSEM1, /*->*/USBQSEM/*<-*/, 0} ;
    SEMA cause ;
    
    if (current_clock != 48000000) {
        panic(9) ;
    }

    KS_delay(SELFTASK, ((TICKS)1000*CLKRATE/1000)) ;     // skip time

    // queue monitor sema
    KS_defqsema(USBOQ, USBQSEM, QNE) ;

    // Set the PLL USB Divider by 2 (PLLCLOCK=96MHz)
    AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1 ;

    // Specific Chip USB Initialisation
    // Enables the 48MHz USB clock UDPCK and System Peripheral USB Clock
    AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_UDP;
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_UDP);

    // reset all endpoints
    AT91C_BASE_UDP->UDP_RSTEP  = 0x3f ;

    // CDC Open by structure initialization
    currConfiguration = 0 ;
    currConnection    = 0 ;
    currDataLen       = 0 ;     // no Tx pending
    currRcvBank       = AT91C_UDP_RX_DATA_BK0 ;
    
    // Install interrupt handler
    AT91F_AIC_Configure(AT91C_ID_UDP, USB_INTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL/*AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE*/, usbdrv) ;

	// Main loop

    // pdebugt(1,SELFTASK, "Main loop") ;
    for( ; ; ) {
        // don't waste time
        cause = KS_waitm(semalist) ;
        // pdebugt(1,SELFTASK, "%d: ISR=0x%04x, IMR=0x%04x", cause, AT91C_BASE_UDP->UDP_ISR, AT91C_BASE_UDP->UDP_IMR) ;
        switch(cause) {
        case USBSEM0 :
            // pdebugt(1,SELFTASK, "CSR[0]=0x%08x", AT91C_BASE_UDP->UDP_CSR[0]) ;
            // check if Device Request received
            if (AT91C_BASE_UDP->UDP_CSR[0] & AT91C_UDP_RXSETUP) {
                USB_RxSetup() ;
            }

            // check if more data to Tx
            if (AT91C_BASE_UDP->UDP_CSR[0] & AT91C_UDP_TXCOMP) {
                USB_SendNextData() ;
            }

            // check if Data received on bank 0
            if (AT91C_BASE_UDP->UDP_CSR[0] & AT91C_UDP_RX_DATA_BK0) {
                USB_RxData(AT91C_UDP_RX_DATA_BK0) ;
            }

            // check if Data received on bank 1
            if (AT91C_BASE_UDP->UDP_CSR[0] & AT91C_UDP_RX_DATA_BK1) {
                USB_RxData(AT91C_UDP_RX_DATA_BK1) ;
            }
            
            // re-enable interrupt for endpoint 0
            AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT0 ;
            break ;

        case USBSEM1 :
            // pdebugt(1,SELFTASK, "CSR[1]=0x%08x", AT91C_BASE_UDP->UDP_CSR[1]) ;
            USB_Receive() ;
            // re-enable interrupt for endpoint 1
            AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT1 ;
            break ;


        case USBSEM2 :
            // pdebugt(1,SELFTASK, "CSR[2]=0x%08x", AT91C_BASE_UDP->UDP_CSR[2]) ;
            if (USB_Transmit(/*first = */ NO) == 0) {
                // end of transmission, re-enable queue
                semalist[3] = USBQSEM ;
            }
            // re-enable interrupt for endpoint 2
            AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT2 ;
            break ;

        case USBQSEM :
            AT91C_BASE_UDP->UDP_IDR = AT91C_UDP_EPINT2 ;
            // pdebugt(1,SELFTASK, "USBQ CSR[2]=0x%08x", AT91C_BASE_UDP->UDP_CSR[2]) ;
            USB_Transmit(/*first = */ YES) ;
            semalist[3] = 0 ;   // no more wakeup from queue
            // re-enable interrupt for endpoint 2
            AT91C_BASE_UDP->UDP_IER = AT91C_UDP_EPINT2 ;
            break ;
        }
    }
}
#endif // USE_USB_ON_ARM
// end of file - drv_usb.c

