// drv_usb.c - USB driver tasks
//
//   Copyright (c) 1997-2014.
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

#include "USB/USB.h"

unsigned char usb_enable ;	// 0=req.OFF, 1=req.ON
#ifdef CBUG

#undef USB_REAL_DEBUG

#ifdef USB_REAL_DEBUG
#define PUT_HEX(B) {PUT_CHAR('_');PUT_CHAR((((B)>>4)&0xf)+'0');PUT_CHAR(((B)&0xf)+'0');PUT_CHAR('_');}
#define PUT_DEBUG(B) {PUT_CHAR('_');PUT_CHAR(B);PUT_CHAR('_');}
#define PUT_CHAR(A) {while(!((UART2->LSR) & 0x20));UART2->THR=(A);}
#else // USB_REAL_DEBUG
#define PUT_HEX(B)      {}
#define PUT_DEBUG(B)    {}
#define PUT_CHAR(A)     {}
#endif // USB_REAL_DEBUG
#else // CBUG
#define PUT_HEX(B)      {}
#define PUT_DEBUG(B)    {}
#define PUT_CHAR(A)     {}

#endif // CBUG

//----------------------------------------------------------------------------
// who we are

#define USB_PORT_NUMBER     0

#define NULLSEMA ((SEMA)0)

/** @defgroup Virtual_Serial_Device_Descriptor Class descriptors
 * @ingroup USB_Virtual_Serial_Device_18xx43xx USB_Virtual_Serial_Device_17xx40xx USB_Virtual_Serial_Device_11Uxx
 * @{
 */

/** Endpoint number of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPNUM         1

/** Endpoint number of the CDC device-to-host data IN endpoint. */
#define CDC_TX_EPNUM                   2

/** Endpoint number of the CDC host-to-device data OUT endpoint. */
#if defined(__LPC175X_6X__) || defined(__LPC177X_8X__) || defined(__LPC407X_8X__)
    #define CDC_RX_EPNUM               5    
#else
    #define CDC_RX_EPNUM               3
#endif

/** Size in bytes of the CDC device-to-host notification IN endpoint. */
#define CDC_NOTIFICATION_EPSIZE        8

/** Size in bytes of the CDC data IN and OUT endpoints. */
#define CDC_TXRX_EPSIZE                16

/** @brief  Type define for the device configuration descriptor structure. This must be defined in the
 *          application code, as the configuration descriptor contains several sub-descriptors which
 *          vary between devices, and which describe the device's usage to the host.
 */
typedef struct {
    USB_Descriptor_Configuration_Header_t    Config;
    USB_Descriptor_Interface_t               CDC_CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t    CDC_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t       CDC_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t     CDC_Functional_Union;
    USB_Descriptor_Endpoint_t                CDC_NotificationEndpoint;
    USB_Descriptor_Interface_t               CDC_DCI_Interface;
    USB_Descriptor_Endpoint_t                CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t                CDC_DataInEndpoint;
    unsigned char                            CDC_Termination;
} USB_Descriptor_Configuration_t;


/** Device descriptor structure. This descriptor, located in FLASH memory, describes the overall
 *  device characteristics, including the supported USB version, control endpoint size and the
 *  number of device configurations. The descriptor is read out by the USB host when the enumeration
 *  process begins.
 */
USB_Descriptor_Device_t DeviceDescriptor = {
    .Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

    .USBSpecification       = VERSION_BCD(01.10),
    .Class                  = CDC_CSCP_CDCClass,
    .SubClass               = CDC_CSCP_NoSpecificSubclass,
    .Protocol               = CDC_CSCP_NoSpecificProtocol,

    .Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

    .VendorID               = 0x1fc9,   /* NXP */
    .ProductID              = 0x2047,
    .ReleaseNumber          = VERSION_BCD(00.01),

    .ManufacturerStrIndex   = 0x01,
    .ProductStrIndex        = 0x02,
    .SerialNumStrIndex      = USE_INTERNAL_SERIAL,

    .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS  /* config 1 descriptor has 1 excess byte, ignoring _BM_ */
};

/** Configuration descriptor structure. This descriptor, located in FLASH memory, describes the usage
 *  of the device in one of its supported configurations, including information about any device interfaces
 *  and endpoints. The descriptor is read out by the USB host during the enumeration process when selecting
 *  a configuration so that the host may correctly communicate with the USB device.
 */
USB_Descriptor_Configuration_t ConfigurationDescriptor = {
    .Config = {
        .Header                 = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

        .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t) - 1,       // termination byte not included in size
        .TotalInterfaces        = 2,

        .ConfigurationNumber    = 1,
        .ConfigurationStrIndex  = NO_DESCRIPTOR,

        .ConfigAttributes       = (USB_CONFIG_ATTR_BUSPOWERED | USB_CONFIG_ATTR_SELFPOWERED),

        .MaxPowerConsumption    = USB_CONFIG_POWER_MA(500)
    },

    .CDC_CCI_Interface = {
        .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

        .InterfaceNumber        = 0,
        .AlternateSetting       = 0,

        .TotalEndpoints         = 1,

        .Class                  = CDC_CSCP_CDCClass,
        .SubClass               = CDC_CSCP_ACMSubclass,
        .Protocol               = CDC_CSCP_ATCommandProtocol,

        .InterfaceStrIndex      = NO_DESCRIPTOR
    },

    .CDC_Functional_Header = {
        .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t), .Type = DTYPE_CSInterface},
        .Subtype                = CDC_DSUBTYPE_CSInterface_Header,

        .CDCSpecification       = VERSION_BCD(01.10),
    },

    .CDC_Functional_ACM = {
        .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t), .Type = DTYPE_CSInterface},
        .Subtype                = CDC_DSUBTYPE_CSInterface_ACM,

        .Capabilities           = 0x06,
    },

    .CDC_Functional_Union = {
        .Header                 = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t), .Type = DTYPE_CSInterface},
        .Subtype                = CDC_DSUBTYPE_CSInterface_Union,

        .MasterInterfaceNumber  = 0,
        .SlaveInterfaceNumber   = 1,
    },

    .CDC_NotificationEndpoint = {
        .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

        //          .EndpointAddress        = (ENDPOINT_DESCRIPTOR_DIR_IN | CDC_NOTIFICATION_EPNUM),
        .EndpointAddress        = (ENDPOINT_DIR_IN | CDC_NOTIFICATION_EPNUM),
        .Attributes             = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_NOTIFICATION_EPSIZE,
        .PollingIntervalMS      = 0xFF
    },

    .CDC_DCI_Interface = {
        .Header                 = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

        .InterfaceNumber        = 1,
        .AlternateSetting       = 0,

        .TotalEndpoints         = 2,

        .Class                  = CDC_CSCP_CDCDataClass,
        .SubClass               = CDC_CSCP_NoDataSubclass,
        .Protocol               = CDC_CSCP_NoDataProtocol,

        .InterfaceStrIndex      = NO_DESCRIPTOR
    },

    .CDC_DataOutEndpoint = {
        .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

        //          .EndpointAddress        = (ENDPOINT_DESCRIPTOR_DIR_OUT | CDC_RX_EPNUM),
        .EndpointAddress        = (ENDPOINT_DIR_OUT | CDC_RX_EPNUM),
        .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_TXRX_EPSIZE,
        .PollingIntervalMS      = 0x01
    },

    .CDC_DataInEndpoint = {
        .Header                 = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

        //          .EndpointAddress        = (ENDPOINT_DESCRIPTOR_DIR_IN | CDC_TX_EPNUM),
        .EndpointAddress        = (ENDPOINT_DIR_IN | CDC_TX_EPNUM),
        .Attributes             = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
        .EndpointSize           = CDC_TXRX_EPSIZE,
        .PollingIntervalMS      = 0x01
    },
    .CDC_Termination = 0x00
};

/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
uint8_t LanguageString[] = {
    USB_STRING_LEN(1),
    DTYPE_String,
    WBVAL(LANGUAGE_ID_ENG),
};
USB_Descriptor_String_t *LanguageStringPtr = (USB_Descriptor_String_t *) LanguageString;

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
uint8_t ManufacturerString[] = {
    USB_STRING_LEN(3),
    DTYPE_String,
    WBVAL('N'),
    WBVAL('X'),
    WBVAL('P'),
};
USB_Descriptor_String_t *ManufacturerStringPtr = (USB_Descriptor_String_t *) ManufacturerString;

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
uint8_t ProductString[] = {
    USB_STRING_LEN(16),
    DTYPE_String,
    WBVAL('T'),
    WBVAL('E'),
    WBVAL('S'),
    WBVAL('T'),
    WBVAL(' '),
    WBVAL('G'),
    WBVAL('e'),
    WBVAL('m'),
    WBVAL('i'),
    WBVAL('n'),
    WBVAL('u'),
    WBVAL('s'),
    WBVAL(' '),
    WBVAL('U'),
    WBVAL('S'),
    WBVAL('B'),
};
USB_Descriptor_String_t *ProductStringPtr = (USB_Descriptor_String_t *) ProductString;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/** This function is called by the library when in device mode, and must be overridden (see library "USB Descriptors"
 *  documentation) by the application code so that the address and size of a requested descriptor can be given
 *  to the USB library. When the device receives a Get Descriptor request on the control endpoint, this function
 *  is called so that the descriptor details can be passed back and the appropriate descriptor sent back to the
 *  USB host.
 */
uint16_t CALLBACK_USB_GetDescriptor(uint8_t corenum,
                                    const uint16_t wValue,
                                    const uint8_t wIndex,
                                    const void * *const DescriptorAddress)
{
    const uint8_t  DescriptorType   = (wValue >> 8);
    const uint8_t  DescriptorNumber = (wValue & 0xFF);

    const void *Address = NULL;
    uint16_t    Size    = NO_DESCRIPTOR;

    switch (DescriptorType) {
    case DTYPE_Device:
        Address = &DeviceDescriptor;
        Size    = sizeof(USB_Descriptor_Device_t);
        break;

    case DTYPE_Configuration:
        Address = &ConfigurationDescriptor;
        Size    = sizeof(USB_Descriptor_Configuration_t);
        break;

    case DTYPE_String:
        switch (DescriptorNumber) {
        case 0x00:
            Address = LanguageStringPtr;
            Size    = pgm_read_byte(&LanguageStringPtr->Header.Size);
            break;

        case 0x01:
            Address = ManufacturerStringPtr;
            Size    = pgm_read_byte(&ManufacturerStringPtr->Header.Size);
            break;

        case 0x02:
            Address = ProductStringPtr;
            Size    = pgm_read_byte(&ProductStringPtr->Header.Size);
            break;
        }

        break;
    }

    *DescriptorAddress = Address;
    return Size;
}

//----------------------------------------------------------------------------
// internal functions

void usbtask(void) TASK_ATTRIBUTE ;

//----------------------------------------------------------------------------
// internal data

/** LPCUSBlib CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
    .Config = {
        .ControlInterfaceNumber         = 0,

        .DataINEndpointNumber           = CDC_TX_EPNUM,
        .DataINEndpointSize             = CDC_TXRX_EPSIZE,
        .DataINEndpointDoubleBank       = false,

        .DataOUTEndpointNumber          = CDC_RX_EPNUM,
        .DataOUTEndpointSize            = CDC_TXRX_EPSIZE,
        .DataOUTEndpointDoubleBank      = false,

        .NotificationEndpointNumber     = CDC_NOTIFICATION_EPNUM,
        .NotificationEndpointSize       = CDC_NOTIFICATION_EPSIZE,
        .NotificationEndpointDoubleBank = false,
        .PortNumber             = USB_PORT_NUMBER,
    },
};

unsigned short usberr ;                 // error counter

///----------------------------------------------------------------------------
/// Interrupt routine for USB

// moving stuff to approriate places ?
extern void DcdIrqHandler(uint8_t DeviceID);

void USB_IRQHandler(void)
{
    if (USB_CurrentMode[USB_PORT_NUMBER] == USB_MODE_Host) {
#if defined(USB_CAN_BE_HOST)
        HcdIrqHandler(USB_PORT_NUMBER);
#endif
    }

    if (USB_CurrentMode[USB_PORT_NUMBER] == USB_MODE_Device) {
#if defined(USB_CAN_BE_DEVICE)
        DcdIrqHandler(USB_PORT_NUMBER);
#endif
    }
}

///----------------------------------------------------------------------------
/// HAL

void HAL_USBInit(uint8_t corenum)
{
    LPC_SC->PCONP |= CLKPWR_PCONP_PCUSB ;           // USB PCLK -> enable USB Per.

#if defined(USB_CAN_BE_DEVICE)
    LPC_USB->USBClkCtrl = 0x1a ;                    // Dev, PortSel, AHB clock enable 
    while ((LPC_USB->USBClkSt & 0x1a) != 0x1a) ;

    LPC_USB->StCtrl = 0x3 ;                         // Port 2 is host

    LPC_USB->USBClkCtrl = 0x12 ;                    // Dev, AHB clock enable 
    while ((LPC_USB->USBClkSt & 0x12) != 0x12) ;
    
//    LPC_USB->OTGClkCtrl = 0x0000001a ;  // enable Dev clock, OTG clock and AHB clock 
//    while((LPC_USB->OTGClkSt & 0x0000001a) != 0x0000001a) ;
    
    HAL_Reset(USB_PORT_NUMBER);
#endif  
}

void HAL_USBDeInit(uint8_t corenum, uint8_t mode)
{
    NVIC_DisableIRQ(USB_IRQn) ;                     // disable USB interrupt 
    LPC_SC->PCONP &= (~CLKPWR_PCONP_PCUSB) ;        // disable USB Per.      
}

void HAL_EnableUSBInterrupt(uint8_t corenum)
{
    NVIC_EnableIRQ(USB_IRQn) ;                      // enable USB interrupt 
    NVIC_SetPriority(USB_IRQn, USB_INTERRUPT_LEVEL) ;
}

void HAL_DisableUSBInterrupt(uint8_t corenum)
{
    NVIC_DisableIRQ(USB_IRQn) ;                     // enable USB interrupt 
}

void HAL_USBConnect(uint8_t corenum, uint32_t con)
{
    if (USB_CurrentMode[corenum] == USB_MODE_Device) {
        // printf("HAL_USBConnect %ld\n", con) ; 
#if defined(USB_CAN_BE_DEVICE)
        HAL17XX_USBConnect(con);
#endif
    }
}

///----------------------------------------------------------------------------

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
    // printf("EVENT_USB_Device_Connect\n") ;
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    // printf("EVENT_USB_Device_Disconnect\n") ;
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    /*bool rc ;*/

    /*rc =*/ CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface) ;

    //  LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
    // printf("EVENT_USB_Device_ConfigurationChanged: %d\n", rc) ;
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
    
    // printf("EVENT_USB_Device_ControlRequest\n") ;
}

void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
    /*TODO: add LineEncoding processing here
     * this is just a simple statement, only Baud rate is set */
    
    // Serial_Init(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS, false);
    // printf("EVENT_CDC_Device_LineEncodingChanged bps:%lu, ep:%d\n", 
    //        CDCInterfaceInfo->State.LineEncoding.BaudRateBPS,
    //        CDCInterfaceInfo->Config.DataOUTEndpointNumber
    //      ) ;
}

///----------------------------------------------------------------------------
/// USB task

#define TMINT_USBTASK ((TICKS)20*CLKRATE/1000)      // Wake up every 20 msec
//#define TMINT_USBTASK ((TICKS)10*CLKRATE/1000)      // Wake up every 10 msec // FR 11/03/14

int usbstate ;

void usbtask(void)
{
SEMA semalist[3] = {USBTIMER, USBQSEM, 0} ;
SEMA cause ;
static CLKBLK clkblk ;      // our timer pointer
uint8_t rtx_byte ;
int wt ;
int nbytes ; // , ds = - 1 ;
int usb_on = NO ;

#ifdef CBUG_
int nwaitt, nwaitq ;
#endif

#ifdef CBUG_    
    KS_delay(SELFTASK, ((TICKS)1000*CLKRATE/1000)) ;    // skip time
#endif // CBUG    
	
	usbstate = -1 ;
	
//     // init
//     USB_Init(USB_PORT_NUMBER, USB_MODE_Device);
	if (usb_enable){
		USB_Init(USB_PORT_NUMBER, USB_MODE_Device);
		usb_on = YES ;
	}
	
    // queue monitor sema
    KS_defqsema(USBOQ, USBQSEM, QNE) ;
       
	KS_start_timer(&clkblk, TMINT_USBTASK, TMINT_USBTASK, USBTIMER) ;
  
#ifdef CBUG_
	nwaitt = nwaitq = 0 ;
#endif
	// Main loop
#ifdef CBUG    
    pdebugt(1, "USB Main loop") ;
#endif // CBUG    
	for( ; ; ) {
		// don't waste time
		cause = KS_waitm(semalist) ;
		switch(cause) {
		case USBQSEM :  // something to send
			nbytes = 0 ;
// 			while(KS_dequeue(USBOQ, &rtx_byte) == RC_GOOD) { 
// 				// Added usberr by FR 11/03/14
// 				if (CDC_Device_SendData(&VirtualSerial_CDC_Interface, (char *)(&rtx_byte), 1) )
// 					usberr++ ;
// 				else
// 					nbytes++ ;
// 			}
			wt = 0 ;
			//nwait = 0 ;
			for(;;){
				if (KS_dequeue(USBOQ, &rtx_byte) == RC_GOOD){ 
					wt = 0 ;
					// Added usberr by FR 11/03/14
					if (usb_on){
						if (CDC_Device_SendData(&VirtualSerial_CDC_Interface, (char *)(&rtx_byte), 1) )
							usberr++ ;
						else
							nbytes++ ;
					}
				}else{
					if (wt) break ;
					KS_delay(SELFTASK, 1) ;
					wt++ ;
				}
#ifdef CBUG_
				if ((++nwaitq)>500) { nwaitq = 0 ; break ; }
				if ((nwaitq & 0x1f)==0x1f) 
					pdebugt(1,"USBQ WAIT %d %d (%d)", wt, nbytes, nwaitq) ;
					//printf("%lu USB: recv %d bytes\n", KS_inqtime(), nbytes) ;
#endif // CBUG    

			}
			
			if (usb_on) {if (CDC_Device_Flush(&VirtualSerial_CDC_Interface)) usberr++ ;}
#ifdef CBUG_
			//printf("%lu USB: sent %d bytes\n", KS_inqtime(), nbytes) ;
			pdebugt(1,"USB: sent %d bytes", nbytes) ;
#endif // CBUG    
			break ;
                
		case USBTIMER :
			if (usb_on){
				if (!usb_enable){
					// Disable
					USB_Disable(USB_PORT_NUMBER, USB_MODE_Device);
					usb_on = NO ;
					usbstate = - 1 ;
					break ;
				}
			}else{
				if (usb_enable){
					// init
					USB_Init(USB_PORT_NUMBER, USB_MODE_Device);
					usb_on = YES ;
				}else
					break ;
			}
			wt = nbytes = 0 ;
			//nwait = 0 ;
			do{
				if (nbytes==512) break ; // USB buffer full
				
				// check status change
				int dsnow = USB_DeviceState[USB_PORT_NUMBER] ;
				if (usbstate != dsnow) {
					usbstate = dsnow ;
					if (usbstate == DEVICE_STATE_Configured) {
						// TODO
						// set output pin according to "Connected to PC" status - max power 500mA
					}
#ifdef CBUG    
					pdebugt(1, "USB: status %d", usbstate) ;
#endif // CBUG    
				}
				CDC_Device_USBTask(&VirtualSerial_CDC_Interface) ;
				USB_USBTask(USB_PORT_NUMBER, USB_MODE_Device) ;
				
				if (nbytes!=wt){
					KS_delay(SELFTASK, 1) ;
#ifdef CBUG_
					if ((++nwaitt)>500) { nwaitt = 0 ; break ; }
					if ((nwaitt & 0x1f)==0x1f) 
						pdebugt(1,"USBT WAIT %d %d (%d)", wt, nbytes, nwaitt) ;
						//printf("%lu USB: recv %d bytes\n", KS_inqtime(), nbytes) ;
#endif // CBUG    
				}
				nbytes = wt ;
				if (nbytes<0) break ;
			}while ((wt = CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface)) != nbytes) ;
			//wt = 0 ;
			if (nbytes>0){
#ifdef CBUG_ 
				if (nbytes) 
					pdebugt(1,"USB: recv %d bytes", nbytes) ;
					//printf("%lu USB: recv %d bytes\n", KS_inqtime(), nbytes) ;
#endif // CBUG    
				while(nbytes > 0) {
					rtx_byte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface) ;
					nbytes-- ;  // one less
#ifdef CBUG    
					// pdebugt(1, "USB: received 0x%02x - %c", rtx_byte, (rtx_byte>=' ') ? rtx_byte : '_') ;
#endif // CBUG    
					if (KS_enqueue(USBIQ, &rtx_byte) != RC_GOOD) {
						// one more error
						usberr++ ;
					}
					// Added by FR 11/03/14
					//if (!nbytes) nbytes = CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface) ;
					// Added by FR 11/03/14
				} 
				// check status change
				int dsnow = USB_DeviceState[USB_PORT_NUMBER] ;
				if (usbstate != dsnow) {
					usbstate = dsnow ;
					if (usbstate == DEVICE_STATE_Configured) {
						// TODO
						// set output pin according to "Connected to PC" status - max power 500mA
					}
#ifdef CBUG    
					pdebugt(1, "USB: status %d", usbstate) ;
#endif // CBUG    
				}
				CDC_Device_USBTask(&VirtualSerial_CDC_Interface) ;
				USB_USBTask(USB_PORT_NUMBER, USB_MODE_Device) ;
			}
#ifdef CBUG_  
			if (wt) pdebugt(1,"--->USB: TOT bytes %d", wt) ;
					//printf("%lu USB: recv %d bytes\n", KS_inqtime(), nbytes) ;
#endif // CBUG    
			// TODO
			// check USB presence pin - if lost, remove "Connected to PC" status
			break ;
		}
	}
}

int usbstatus(void)
{
	return((usbstate==DEVICE_STATE_Configured)? 1:0) ;
}

#endif // USE_USB_ON_ARM
// end of file - drv_usb.c
