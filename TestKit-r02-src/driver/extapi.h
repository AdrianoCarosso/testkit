//
// Copyright (c) 1997-2006.
// T.E.S.T. srl
// ------------------------------------------------------------------------
// Extended Operating System functions by T.E.S.T. srl

// prevent re-include
#ifndef _EXTAPI_H
#define _EXTAPI_H

// -----------------------------------------------------------------------
// TASK options
#define TASK_ATTRIBUTE __attribute__ ((noreturn))

// NOINIT options
#define NOINIT_ATTRIBUTE __attribute__ ((section(".noinit")))

// -----------------------------------------------------------------------
// Performance options

// Performances with different Optimizations:
// -Os --> 10 sec = ???
#define LOOPS_PER_SEC_Os 0

#if defined(USE_AT91SAM3S4)
#define LOOPS_PER_SEC_O2        (current_clock/12)
#endif // defined(USE_AT91SAM3S4)

#if defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)
// -O2 --> 10 sec =  8793245 @  8 MHz - 0 Wait
// -O2 --> 10 sec = 26620910 @ 24 MHz - 0 Wait
// -O2 --> 10 sec = 29946751 @ 30 MHz - 1 Wait
// -O2 --> 10 sec = 55946751 @ 56 MHz - 1 Wait
#ifdef USE_LOW_POWER
// #define LOOPS_PER_SEC_O2 (current_clock/((current_clock>=30000000)?18:17))
#define LOOPS_PER_SEC_O2 (current_clock/((current_clock>=30000000)?10:9)) // _FR_ Perché diverso se USE_LOW_POWER ?
#else // USE_LOW_POWER
#define LOOPS_PER_SEC_O2 (current_clock/((current_clock>=30000000)?10:9))
#endif // USE_LOW_POWER

#endif //defined(USE_AT91SAM7A3) || defined(USE_AT91SAM7S256) || defined(USE_AT91SAM7S512)

#define LOOPS_PER_SEC LOOPS_PER_SEC_O2

#ifdef USE_PERFORMANCE
extern volatile unsigned long perf_counter ;
extern volatile unsigned long perf_last ;
#endif // USE_PERFORMANCE

// ---------------------------------------------------------------------
// CLOCK handler

extern void AT91F_CLOCKinit(unsigned long desired_clock) ;
extern unsigned long current_clock ;

// ---------------------------------------------------------------------
// I/O handler

extern void diostart(void) ;
extern void diostop(void) ;
extern unsigned long dio_read(int port) ;
extern void dio_write(int port, int pmask, int pval) ;
extern void dio_mask(int port, int pmask) ;
extern unsigned short dio_counter(int port) ;
extern void dio_beeper(int freq) ;


// ---------------------------------------------------------------------
// Generic FLASH handler
#ifdef USE_FLASH_DATA
extern void				EKS_FlashClear(void) ;
extern unsigned short	EKS_FlashCheckSum(void) ;

extern void EKS_LCK_FlashRead(unsigned long bbegin, unsigned char *dst, int flen) ;
extern void EKS_LCK_FlashWrite(unsigned long bbegin, unsigned char *src, int flen) ;
extern void EKS_LCK_FlashErase(unsigned long bbegin, unsigned long bend) ;
#endif

// ---------------------------------------------------------------------
// SPI handler

//#ifdef USE_SPI_ON_ARM
#ifdef USE_FLASH_DATA
// #if ( defined(USE_SPI_ON_ARM) && defined(USE_SERIALFLASH_ON_ARM) )
// extern void SPI_FlashErase(unsigned long bbegin, unsigned long bend) ;
// extern void SPI_FlashRead(unsigned char *dst, unsigned long bbegin, int flen) ;
// extern void SPI_FlashWrite(unsigned long bbegin, unsigned char *src, unsigned char *dst, int flen) ;
// #endif // USE_SERIALFLASH_ON_ARM

extern unsigned long	  EKS_FlashFree(void) ;
extern unsigned long	  EKS_FlashTotal(void) ;

#endif // USE_FLASH_DATA

// ---------------------------------------------------------------------
// general functions redirector, avoid REENTRANT stuff

// from stdio_xxx.c
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base) ;
long simple_strtol(const char *cp,char **endp,unsigned int base) ;
unsigned long long simple_strtoull(const char *cp,char **endp,unsigned int base) ;
long long simple_strtoll(const char *cp,char **endp,unsigned int base) ;

#define atoi(A) simple_strtol(A,(char **)(0),10)
#define atol(A) simple_strtol(A,(char **)(0),10)
#define atoll(A) simple_strtoll(A,(char **)(0),10)
#define atof(A) strtod(A,0)
#define atod(A) strtod(A,0)

#define strtoul(A,B,C) simple_strtoul(A,B,C)
#define strtol(A,B,C) simple_strtol(A,B,C)
#define strtoull(A,B,C) simple_strtoull(A,B,C)
#define strtoll(A,B,C) simple_strtoll(A,B,C)

// ---------------------------------------------------------------------
// general purpose definitions

#define YES	1	// useful for logical operation
#define NO	0

#define TRUE	1
#define FALSE	0

#define ERROR	1
#define OK		0

#define FOREVER 1

#define ON  1
#define OFF 0

#define TRACE_DISABLE 2

#define NULLCHAR	((unsigned char *)(0))
#ifndef NULL
#define NULL	 ((void *)0)
#endif

#define MIN(A,B) (((A)<(B))?(A):(B))
#define MAX(A,B) (((A)>(B))?(A):(B))

struct PKTMEMBUF {		// header of each memory block
struct PKTMEMBUF *next ;	// pointer to next, last is NULL
unsigned char offs ;		// offset of first byte inside this buffer
unsigned char mtot ;		// data len inside this buffer
} __attribute__ ((packed)) ;	// data follows here after

#define MNULL	((struct PKTMEMBUF *)(0))

#define SELFTASK ((TASK)0)

// init only
extern void   EKS_init(unsigned char numtasks) ;

// ---------------------------------------------------------------------
// Transaction handler
#ifdef USE_TRANSACTIONS_ON_ARM
//
// Flash transaction identificators
//
#define SMS_IN	 (short) 0xa001		// from outerworld
#define SMS_OUT  (short) 0xa002		// our creatures
#define BSC_OUT  (short) 0xa005		// our creatures   (only LU2)
#define BSC_IN	 (short) 0xa006		// from outerworld (only LU2)
// New from 1.53
#define DIR11OUT (short) 0xa007		// to PDA from LU11
#define DIR2OUT  (short) 0xa008		// to PDA from LU2


extern int		  EKS_PktBufStore(struct PKTMEMBUF * pmf, short signature, short id) ;
extern int		  EKS_PktBufCheckStore(struct PKTMEMBUF * pmf, short signature, short id, unsigned char p_seqok, unsigned char p_trlast) ;
extern struct PKTMEMBUF * EKS_PktBufRetrive(short signature, short id, unsigned long *st_addr) ;
#ifdef TRANS_FASTCHECK
extern int		EKS_PktBufCheck(short signature, short id, short *id_retry) ;
#else
extern int 		EKS_PktBufCheck(short signature, short id);
#endif
#endif

#if defined(USE_PKTMEMBUF) || defined(CBUG)

// Queue attributes
#define Q_MASKADDR	0x30ffffffL	// No attributes
#define Q_MASKATTR	0xcf000000L	// No attributes
#define Q_DEFAULT	0x00000000L	// No attributes
#define Q_NOTRAN	0x01000000L	// No transaction mode
#define Q_ALSODEST	0x02000000L	// Also destionation data

#define Q_FAST  	0x04000000L	// Send answer without timeout

// memory chain management
extern void   EKS_PktBufRelease(struct PKTMEMBUF * mptr) ;
extern void   EKS_PktBufCat(struct PKTMEMBUF * mpf, struct PKTMEMBUF * mps) ;
extern struct PKTMEMBUF * EKS_PktBufDuplicate(struct PKTMEMBUF * mptr) ;
extern size_t EKS_PktBufLength(struct PKTMEMBUF * mptr) ;
extern int    EKS_PktBufAdd(unsigned char cts, struct PKTMEMBUF ** ppmfirst, struct PKTMEMBUF ** ppmactual) ;
extern int    EKS_PktBufAddString(unsigned char *pcts, unsigned char slen, struct PKTMEMBUF ** ppmfirst, struct PKTMEMBUF ** ppmactual) ;
extern size_t EKS_PktBufCopy(struct PKTMEMBUF * pmf, unsigned char * buf, size_t cnt) ;
extern int    EKS_PktBufComp(struct PKTMEMBUF * pmf, const char * buf, size_t cnt) ;
extern int    EKS_Pkt2BufComp(struct PKTMEMBUF * pmf1, struct PKTMEMBUF * pmf2) ;
extern size_t EKS_PktBufPullup(struct PKTMEMBUF ** ppmf, unsigned char * buf, size_t cnt) ;
extern struct PKTMEMBUF * EKS_PktBufComprexTime(void) ;
extern struct PKTMEMBUF * EKS_PktBufFromTime(time_t giventime) ;
extern struct PKTMEMBUF * EKS_PktBufAccTime(time_t giventime) ;
extern void   EKS_GetHHMM(unsigned char * storetime) ;
extern struct PKTMEMBUF * EKS_PktBufMyTime(time_t giventime, char type) ;
#endif // defined(USE_PKTMEMBUF) || defined(CBUG)

#if (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV))
#define USE_TWI1_ON_ARM
extern int TWI1_send(int dev, unsigned char *buf, int len) ;
extern int TWI1_txrx(int dev, int sub, unsigned char *buf, int len) ;
#endif // (defined(USE_TWI1_AUXILIARY) || defined(USE_TWI_SRV) )

#ifdef USE_TWI2_ON_ARM
extern int TWI2_send(int dev, unsigned char *buf, int len) ;
extern int TWI2_sendrx(int dev, unsigned char *buf, int len,unsigned char *bufrx, int lenrx) ;
extern int TWI2_txrx(int dev, int sub, unsigned char *buf, int len) ;
#endif // USE_TWI2_ON_ARM

#if defined(USE_TWI0_SLAVE) || defined(USE_TWI1_SLAVE) || defined(USE_TWI2_SLAVE) 
extern void TWI_SlaveRx(int port, int slaveaddr, uint8_t *buf, volatile int *len) ;
#endif


#ifdef USE_TW1_FAST_ACCELEROMETER
extern void TWI1_setfast(unsigned char *buffer, int bufsize) ;
extern int TWI1_get3axisfast(short *ax, short *ay, short *az) ;
#define ACC_setfast TWI1_setfast
#define ACC_getfast TWI1_get3axisfast
#define USE_FAST_ACC
#endif // USE_TW1_FAST_ACCELEROMETER

#ifdef USE_SPI_FAST_ACCELEROMETER
extern void SPI_setfast(unsigned char *buffer, int bufsize) ;
extern int SPI_get3axisfast(short *ax, short *ay, short *az) ;
#define ACC_setfast SPI_setfast
#define ACC_getfast SPI_get3axisfast
#endif // USE_SPI_FAST_ACCELEROMETER


#if defined(USE_PARAMETERS_ON_TWI) || defined(USE_PARAMETERS_ON_FLASH) || defined(USE_PARAMETERS_ON_EEPROM)
#ifdef USE_TWI_ONLYAUX
#define TWI_send(_A, _B, _C) 	 TWI1_send(_A, _B, _C)
#define TWI_txrx(_A, _B, _C, _D) TWI1_txrx(_A, _B, _C, _D)
#else
extern int TWI_send(int dev, unsigned char *buf, int len) ;
extern int TWI_txrx(int dev, int sub, unsigned char *buf, int len) ;
#endif
extern int TWI_receive(int dev, unsigned char *buf, int len) ;

struct MYSETUP {
unsigned long sernum ;		// serial number
//unsigned char free[12] ;
//unsigned short chksum ; 	// checksum
} __attribute__ ((packed)) ;

// Setup handling
extern struct PKTMEMBUF * EKS_ParRead(unsigned char num) ;
extern int		  EKS_ParReadShort(unsigned char num, unsigned short *val) ;
extern int		  EKS_ParReadLong(unsigned char num, long *val) ;
extern int		  EKS_ParWrite(unsigned char num, struct PKTMEMBUF ** ppst) ;
extern int		  EKS_ParWriteShort(unsigned char num, short val) ;
extern int		  EKS_ParWriteLong(unsigned char num, long val) ;
extern void		  EKS_ParClear(void) ;
extern int		  EKS_ParSize(void) ;
extern struct MYSETUP *   EKS_GetSetup(void) ;
extern void		  EKS_NewSetup(struct MYSETUP *) ;
#endif // defined(USE_PARAMETERS_ON_FLASH) || defined(USE_PARAMETERS_ON_TWI)

// Input from subprocessor
#if defined(USE_TWI_ON_ARM) && defined(USE_ATMEL_TWI)
extern unsigned long      EKS_ReadInput(void) ;
#endif // defined(USE_TWI_ON_ARM) && defined(USE_ATMEL_TWI)


// Ram UPGRADE file
extern int            EKS_RamFill(void) ;
extern int            EKS_RamWrite(unsigned long pstart, unsigned char *buf, unsigned long flen) ;
extern int            EKS_RamRead(unsigned long pstart, unsigned char *buf, unsigned long flen) ;
extern unsigned short EKS_RamCheckSum(void) ;


#ifdef USE_TRANSACTIONS_ON_ARM
// Flash file
struct FFILE {
short fsign ;	// type signature
short fid ;	// id of type
size_t flen ;	// data len
}  __attribute__ ((packed)) ;
#endif

#ifdef TRANS_PACK
extern short EKS_transprepack(short * ntr ) ; // save compact trans on copy (input last trans ACK)
extern short EKS_transpack(short * ntr) ;	  // clear trans space and move copy on trans space (output last trans to ACK)
#endif

#ifdef NOT_IMPLEMENTED
extern void	          EKS_FlashCodeFromRam(void) ;
#endif // NOT_IMPLEMENTED

// temporary
// FW Upg data not into flash
#ifndef CODEFLASH_START

//#define RAMCODESIZE     (24*1024/4)

// Ram file with internal RAM
#ifdef CBUG
#define RAMCODESIZE     (1*1024/4)
#else // CBUG
#define RAMCODESIZE     (24*1024/4)
#endif // CBUG
extern unsigned long ramcode[RAMCODESIZE] ;
extern int            EKS_RamFill(void) ;
extern int            EKS_RamWrite(unsigned long pstart, unsigned char *buf, unsigned long flen) ;
#endif // ndef CODEFLASH_START

// Shutdown management
#define SD_RUN				0
#define SD_LATEBOOT			1		// system reboot after delay
#define SD_TURNOFF			2		// complete shutdown
#define SD_STDBY			3		// stdby mode
#define SD_CODEUPGRADE  	4       // code upgrade and reboot
#define SD_SLEEPRING		5		//* Also MTS4004
//#define SYS_SYSRESTART	6		// MTS4004 // Restart all (Linux)
#define SD_STDBYRING		7		//* Also MTS4004
#define SD_STDBYRINGCH		8		//* Also MTS4004
#define SD_STDBYCH      	9       // stdby mode with battery charge
#define SD_UNVOLTSLEEP		10		//	added from 3.20 for disable Move
#define SD_NOPRESENCE   	11      // no presence
#define SD_TURNOFFCH		12		//* shutdown with charge
#define SD_REBOOT			13		// system reboot
#ifdef USE_LOW_POWER
#define SD_LOWPOWER     	14       // LOW power with CAN enabled
#endif // USE_LOW_POWER
#define SD_SLEEPRINGCH		15		//* Also MTS4004

//#ifdef USE_TRACE
// Definition for shutdown extended code
#define SHUTCODE_RECVSM 	0
#define SHUTCODE_ODOTAR 	1
#define SHUTCODE_FWUPGB 	2
#define SHUTCODE_PRES   	3
#define SHUTCODE_BYSM   	4
#define SHUTCODE_NOVEXT 	5
#define SHUTCODE_NOVBAT 	6
#define SHUTCODE_MANUAL 	7
#define SHUTCODE_FWUPG  	8
#define SHUTCODE_WDOG   	9
#define SHUTCODE_GSMOFF    10
#define SHUTCODE_PIDUNK    11
#define SHUTCODE_NOREG     12
#define SHUTCODE_SSTATS	   13
#define SHUTCODE_CHANGEIN  14
#define SHUTCODE_OUTOFMEM  15

//#endif


extern void EKS_AskShutdown(unsigned char mode) ;
extern unsigned char EKS_EnqShutdown(void) ;
extern void EKS_AgreeShutdown(void) ;

// Debug printf
#ifdef CBUG
#define USE_PDEBUG
extern unsigned short par71 ;
#endif

extern void RequestShutdown(unsigned char sh_type, unsigned char sh_reason) ;

//extern void pdebugt(1,const char *args, ...) __attribute__((format(printf, 1, 2))) ;
extern void pdebugt(int debug_level, const char *args, ...) __attribute__((format(printf, 2, 3))) ;

//#define pdebugt(_A, args) pdebugt(1,args)
//#define pdebugt(_A, _B, ...) pdebugt(1, _B, ##__VA_ARGS__ )

/* printf only if debugging is at the desired level */
//extern void pdebugt(int dlevel, const char *args, ...) __attribute__((format(printf, 2, 3)));  /* 2=format 3=params */

// panic function: no return
extern void panic(int step) ;

//#if defined(USE_SPI_ON_ARM) && defined(USE_VIRTUALRAM_ON_ARM)
#ifdef USE_VIRTUALRAM_ON_ARM
extern void EKS_VirtualRAM_Read(void * ptr, int len) ;
extern void EKS_VirtualRAM_Write(void * ptr, int len) ;
#endif // defined(USE_SPI_ON_ARM) && defined(USE_VIRTUALRAM_ON_ARM)

// Hardware management

// *********************************************************************
// from drv_adc.c
extern void adcstart(void) ;
extern void adcstop(void) ;
extern void ADC_readready(int chn) ;
extern int ADC_read(int chn) ;

#if defined(USE_TWI_ON_ARM) && defined(USE_ATMEL_TWI)
// ***********************************************************************
// I/O sub-system status structure

struct _IOSTATUS {
    unsigned char cmd ;             // command returned

    unsigned char free ;            // aligner

    unsigned char pb_value ;        // port B value
    unsigned char pc_value ;        // port C value
    unsigned char pd_value ;        // port D value

    unsigned char pb_wakeup ;       // port B value at wakeup
    unsigned char pc_wakeup ;       // port C value at wakeup
    unsigned char pd_wakeup ;       // port D value at wakeup

    unsigned short inadc6 ;         // Analog input ADC6
    unsigned short inadc7 ;         // Analog input ADC7
    unsigned short valim ;          // Main voltage in mV

    unsigned char ee_pullup ;       // pullup mask (from eeprom)
    unsigned char ee_enabled ;      // enabled (from eeprom)
    unsigned char ee_polarity ;     // polarity (from eeprom)
    unsigned char ee_lastbits ;     // threshold and move enable (from eeprom)

    unsigned char sw_version ;      // software version
    unsigned char sw_subver ;       // software sub version
    unsigned short stacksize ;      // stack size
    unsigned short stackused ;      // stack used

    unsigned short eepromsize ;     // eeprom size
    unsigned short eepromused ;     // eeprom used
} __attribute__ ((packed)) ;

extern struct _IOSTATUS iostatus ;

// I/O sub-system device address on TWI bus
#define TWI_DEVICE_IOPROC       1       // I/O processor address

// ***********************************************************************
// TWI I/O sub-system protocol

/*
RX frame format (after our address Rx + W bit):
   1 byte       DATALEN byte(s)
+----------+ .................... +
|   CMD    |        DATA          |
+----------+ .................... +

TX frame format (after our address Rx + R bit):
   1 byte       DATALEN byte(s)
+----------+ .................... +
|   CMD    |        DATA          |
+----------+ .................... +

*/

#define TWI_PROTOCOL_CMD_STATUS         1       // status request
// data in:     <ignored>
// data out:    struct _MYSTATUS

#define TWI_PROTOCOL_CMD_EE_WRITE       2       // write eeprom param
// data in:     byte 0: ParNum, byte 1: ParLen, byte 2..N: ParVal
// data out:    struct _MYSTATUS

#define TWI_PROTOCOL_CMD_EE_READ_PAR    3       // read eeprom param num
// Note: must preceed TWI_PROTOCOL_CMD_READ_DATA
// data in:     byte 0: ParNum
// data out:    struct _MYSTATUS

#define TWI_PROTOCOL_CMD_EE_READ_DATA   4       // read eeprom choosed num
// Note: must follow TWI_PROTOCOL_CMD_READ_PAR
// data in:     <ignored>
// data out:   byte 0: ParNum, byte 1: ParLen, byte 2..N: ParVal

#define TWI_PROTOCOL_CMD_EE_ERASE       5       // erase eeprom
// data in:     <ignored>
// data out:    struct _MYSTATUS

#define TWI_PROTOCOL_CMD_RAM_WRITE      10      // write backed-up ram
// data in:     new RAM values, up to ((int)(backup_ram) - (int)(maincpu_ram))
// data out:    struct _MYSTATUS

#define TWI_PROTOCOL_CMD_RAM_READ       11      // read backed-up ram
// data in:     <ignored>
// data out:    RAM values, up to ((int)(backup_ram) - (int)(maincpu_ram))

#define TWI_PROTOCOL_CMD_OFF            20      // main CPU turn off
// data in:     <ignored>
// data out:    struct _MYSTATUS

#define TWI_PROTOCOL_CMD_CHARGE_ON      21      // enable battery charger
// data in:     <ignored>
// data out:    struct _MYSTATUS

#define TWI_PROTOCOL_CMD_CHARGE_OFF     22      // disable battery charger
// data in:     <ignored>
// data out:    struct _MYSTATUS

#endif // defined(USE_TWI_ON_ARM) && defined(USE_ATMEL_TWI)

#ifdef USE_CAN_ON_ARM
// ***********************************************************************
// CAN bus sub-system

#define CAN_FLAG_INTERRUPT      1
#define CAN_FLAG_EXTENDEDADDR   2
#ifdef USE_CAN_TRANSMIT_ON_ARM
#define CAN_FLAG_TRANSMIT       4
#endif // USE_CAN_TRANSMIT_ON_ARM
#define CAN_FLAG_BUFDATA		8	// New from 29/11/13
#define CAN_TOT_MAILBOXES       16
#define CAN_TOT_CHANNELS        2
void CAN_speed(int cannum, unsigned long bitrate, int listenonly) ;
void CAN_configure(int cannum, int mailbox, unsigned long addr, unsigned long mask, int flags) ;
int CAN_read(int cannum, int mailbox, unsigned long *addr, unsigned char *buffer) ;
#ifdef USE_CAN_TRANSMIT_ON_ARM
int CAN_write(int cannum, int mailbox, unsigned char *buffer, int msglen) ;
#endif // USE_CAN_TRANSMIT_ON_ARM
unsigned long CAN_status(int cannum) ;

extern void canstart(int cannum) ;
extern void canstop(void) ;
#endif // USE_CAN_ON_ARM

// ***********************************************************************
// from uartdrv.c
extern void uartstart(int num, unsigned long baud_rate, int mode) ;
extern void uartstop(void) ;
#ifdef CLOSE_SINGLECOM
extern void uartNstop(char ncoms) ;
#endif
extern unsigned char COMS_disable ; 

// *********************************************************************
// from drv_twi.c
#ifdef USE_TWI_ON_ARM
extern void twistart(void) ;
extern void twistop(void) ;
extern int TWI_send(int dev, unsigned char *buf, int len) ;
extern int TWI_receive(int dev, unsigned char *buf, int len) ;
#endif // USE_TWI_ON_ARM

// *********************************************************************
// from drv_i2s.c
#ifdef USE_I2S
extern void i2sstart(void) ;
extern void i2sstop(void) ;
extern void i2swave(int samplingfreq, signed short * buffer, int samples) ;
#endif // USE_I2S

// *********************************************************************
// from drv_dac.c
#ifdef USE_DAC
extern void dacstart(void) ;
extern void dacstop(void) ;
extern void DACwave(int samplingfreq, signed long * buffer, int samples
#ifdef OSTEO
, int dhz,  signed long * buffer1
#endif
) ;
#endif // USE_DAC

// *********************************************************************
// from drv_spi.c
#ifdef USE_SPI_ON_ARM
extern void spistart(void) ;
extern void spistop(void) ;
extern void SPI_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
              		 unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;
#ifdef USE_DOUBLE_SPI
extern void SPI1_rtx2(unsigned char *buf1tx, unsigned char *buf1rx, int len1,
               		  unsigned char *buf2tx, unsigned char *buf2rx, int len2) ;
#endif // USE_DOUBLE_SPI
#endif // USE_SPI_ON_ARM

// *********************************************************************
// from drv_dio.c
extern void diostart(void) ;    // initialize I/O
extern void diostop(void) ;     // terminate I/O
#ifdef PORT_TW1_CNF
extern short dioconfTW(void) ; 
#endif
#if defined(USE_LPC17XX) || defined(USE_LPC1788)
// *********************************************************************
// IAP interface
unsigned long IAP_partid(void) ;
#endif // defined(USE_LPC17XX) || defined(USE_LPC1788)

// *********************************************************************
// SPI flash

#define FLASHCODE_UNKNOWN           0
#define FLASHCODE_ATMEL_AT45DB161D  1
#define FLASHCODE_NUMONYX_M45PE16   2
#define FLASHCODE_NAND				5

// from flashapi.c
extern unsigned char flashcode ;

// ***********************************************************************
#ifdef USE_EEPROM_ON_LPC1788

#define EEPROM_PAGE_OFFSET(n)           (n & 0x3F)
#define EEPROM_PAGE_ADRESS(n)           ((n & 0x3F) << 6)

#define EEPROM_PAGE_SIZE                64      /* EEPROM bytes per page */
#define EEPROM_PAGE_NUM                 63      /*  EEPROM pages */

#define MODE_8_BIT      8
#define MODE_16_BIT    16
#define MODE_32_BIT    32

extern void eepromstart(void) ;
extern void eepromstop(void) ;
// Read & Write: nr. bytes =  mode*count
extern int EEPROM_Write(uint16_t page_offset, uint16_t page_nr, void *data, int mode, uint32_t count) ;
extern void EEPROM_Read(uint16_t page_offset, uint16_t page_nr, void *data, int mode, uint32_t count) ;
extern void EEPROM_Erase(uint16_t page_nr) ;
#endif
// *********************************************************************
// from drv_nand.c
#ifdef USE_NANDFLASH_ON_ARM

#define PTRNAND_CLE   ((volatile unsigned char *)0x90100000)
#define PTRNAND_ALE   ((volatile unsigned char *)0x90080000)
#define PTRNAND_DATA  ((volatile unsigned char *)0x90000000)
#define USE_DMA_NAND

#define NAND_PAGEISFREE  5
#define NAND_PAGEISBAD  -1

struct _NAND_PAGE {         // structure of 2048 data byte
unsigned char data[2048] ;
} ;

struct _NAND_OOB {          // structure of 64 byte OOB
unsigned char badblock ;
unsigned char free1[3] ;
uint32_t      ecc[4] ;
uint32_t      oldblk ;
unsigned char free2[40] ;
} ;

struct _NAND_DATA {         // structure of 2048 data byte + 64 byte OOB = 2112 byte
struct _NAND_PAGE page ;
struct _NAND_OOB  pageoob ;
} ;

extern int nandstart(void) ;
extern void nandstop(void) ;
extern uint32_t nand_getSizeMB(void) ;
extern uint32_t nand_getPageSize(void) ;
extern uint32_t nand_getRedundantSize(void) ;
extern uint32_t nand_getBlockSize(void) ;
extern uint32_t nand_getBlocks(void) ;
extern int nand_isBlockValid(uint32_t block) ;
extern int nand_readPage(uint32_t page, struct _NAND_DATA * pageStruct /*uint8_t* pageBuf*/) ;
extern int nand_writePage(uint32_t page, struct _NAND_DATA * pageStruct /*uint8_t* pageBuf*/) ;
extern int nand_eraseBlock(uint32_t block) ;

extern int nand_read512(uint64_t addr, void * userbuffer) ;
extern int nand_write512(uint64_t addr, void * userbuffer) ;

extern int nand_blockOOB(struct _NAND_OOB * oob_page) ;

#define NANDMAXBADBLOCK 32
extern short NAND_badblocks[NANDMAXBADBLOCK] ;
#endif // USE_NANDFLASH_ON_ARM


#ifdef USE_USB_ON_ARM
extern int usbstatus(void) ;
#endif
// ***********************************************************************

#ifdef USE_FAST_AD_T0
extern unsigned char mADusemax ;
#define USE_MEAN  0  
#define USE_MIN   1
#define USE_MAX   2 

#define AD_CLEAR 0x80
#endif

#endif // _EXTAPI_H

