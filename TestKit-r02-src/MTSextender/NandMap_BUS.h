// Will be defined "USE_NANDFLASH_ON_ARM"

#define USE_EEPROM_ON_LPC1788
#define EEPROM_START 0x0000
#define EEPROM_END	 0x0fc0 // only 63 pages
// ----------------------------- EEPROM map ------------------------------------
// Area         Size     	From      		To
// Virtual RAM   1k			0x0000		  0x03ff
// Parameters	 3k 		0x0400		  0x0fff

// VirtualRAM on EEPROM
#define USE_VIRTUALRAM_ON_ARM
#define USE_VIRTUALRAM_ON_EEPROM
#define VIRTUALRAM_START	0x000000L
#define VIRTUALRAM_STOP		0x0003ffL
#define VIRTUALRAM_SSIZE	512 // (2 copy) alternate write

// Parameters
// defined USE_PARAMETERS_ON_EEPROM
// At par section start locate S/N (4 byte)
#define EE_SN_SETUP			0x400L
#define EE_NAND_MAP			0x404L		// How wroted last LOG,TRACE,TRANS record & ADDR temp buf

#define PARAM_START			EE_SN_SETUP + EEPROM_PAGE_SIZE
#define PARAM_STOP			0x0fffL
//#define PARAM_SSIZE			512
// use 3008 byte

struct _NAND_SAVE{		// max size is 64 - 4 = 60 (TRANS, TRACE, HIST)
	uint32_t baddr[3] ;		// Where saved data
	uint32_t addr[3] ;		// First address free to store data
} ; // size is 24 byte

extern struct _NAND_SAVE Nand_save ;

extern char NAND_lasterr ;
// ----------------------------- NAND Flash map ------------------------------------
// External SPI Flash memory
//
// Area         Size     	  From      		To
// ----------------------------------------------------
// FW upgrade     1024k		0x0000000		0x00FFFFF
// SM              256k		0x0100000		0x013FFFF
// Target          256k		0x0140000		0x017FFFF
// CanConf         256k		0x0180000		0x01BFFFF

// Transaction    2688k		0x01C0000		0x045FFFF		// old 384k ( 7 times)
// History(log)   4096k		0x0460000		0x085FFFF		// old 512k ( 8 times)
// Trace          1920k		0x0860000		0x0A3FFFF		// old  96k (20 times)

// Crono upload   3072k		0x0A40000		0x0D3FFFF		// old 256k (12 times)
// Print file     1280k		0x0D40000		0x0E7FFFF

#define FLASH_PAGESIZE   512

#define NAND_BLOCK_USED   512

#define SIZE_BUFFER_NAND	FLASH_PAGESIZE		// Same of a sector
#define SIZE_SECTOR_NAND	FLASH_PAGESIZE

// DO NOT REMOVE !!
#define LOG_8192  // NEW from 2.00 

// Code upgrade: temporal area for new code download (2x512k = 1M)
#define CODEFLASH_START		0x0000000L
#define CODEFLASH_STOP  	0x007FFFFL
#define FLASH_OFFMIRROR		0x0080000L
// Mirror from  0x0080000 & 0x00FFFFFL

// S.M. [rec size 128 bytes]
#define SMFLASH_START		0x0100000L		// (46+1+64=111 -> 17 scratch) 128 byte/cad
#define SMFLASH_STOP		0x011FFFFL
#define SMFLASH_UPSTART		0x0120000L		// (46+1+64=111 -> 17 scratch) 128 byte/cad
#define SMFLASH_UPSTOP		0x013FFFFL
// SM record length
//#define SMFLASH_MEMLEN	((SMFLASH_STOP-SMFLASH_START+1)/256)	// 128 byte (46+1+64=111 -> 17 scratch)
#define SMFLASH_MEMLEN		128	// 128 byte (46+1+64=111 -> 17 scratch)

// TARGET [rec size 32 bytes] (16 rec x sector)
#define TGFLASH_START		0x0140000L		// target record -> 17 + 15 free
#define TGFLASH_STOP		0x017FFFFL

// Canconf [rec size 64 bytes]
#define CANCONF_START		0x0180000L
#define CANCONF_STOP		0x01BFFFFL
#define CANCONF_SIZE		(CANCONF_STOP-CANCONF_START+1)/2 // Use & upg = 2

#define CAN1CONF_START		0x0180000L		// 8k CAN1Conf-128 lines
#define CAN1CONF_STOP	   (CAN1CONF_START + CANxCONF_SIZE -1)
#define CANxCONF_SIZE	   (CANCONF_SIZE / LU_FORCAN)
#define CAN2CONF_START	   (CAN1CONF_STOP + 1 )// 8k CAN2Conf-128 lines
#define CAN2CONF_STOP	   (CAN2CONF_START + CANxCONF_SIZE -1)   // 0x1FBFFFL
#define CAN1CONF_UPSTART   (CAN2CONF_STOP+1) // 0x01C0000L		// 8k CAN1Upgrade-128 lines
#define CAN1CONF_UPSTOP	   (CAN1CONF_UPSTART + CANxCONF_SIZE -1) // 0x1FDFFFL
#define CAN2CONF_UPSTART   (CAN1CONF_UPSTOP + 1 )	// 8k CAN2Upgrade-128 lines
#define CAN2CONF_UPSTOP	   (CAN2CONF_UPSTART + CANxCONF_SIZE -1) // 0x1FFFFFL

// used up to 

// Transactions [rec size free]
//#define USE_TRANSACTIONS_ON_ARM
#define FLASH_START     	0x01C0000L
#define FLASH_STOP      	0x045FFFFL
#define FLASH_TSSIZE    	(8*512)
#define FLASH_TOTSIZE		(FLASH_STOP-FLASH_START+1)
#define FLASH_SVALID		0x0100000L	// only 8 block valid of 21


// log items  [rec size 64 bytes]
#define LOGFLASH_START		0x0460000L		// 64 byte -> 8192 item from 1.64
//#define LOGFLASH_STOP		0x085FFFFL
#define LOGFLASH_STOP		0x049FFFFL	// 0x085FFFFL
#define LOGFLASH_PAGE		512
#define LOGFLASH_SITEM		sizeof(struct _LOGITEM) // 64 -> 128 on NAND
#define LOGFLASH_SVALID		0x0200000L	// only 16 block valid of 32
#define LOGFLASH_ITEMSVALID (LOGFLASH_SVALID/LOGFLASH_SITEM)

#ifdef USE_TRACE
// Internal log (trace)  [rec size 64 bytes]
#define TRACEFL_START		0x0860000L		// 64 byte -> (96k / 64 ) = 1536 item
#define TRACEFL_STOP		0x0A3FFFFL
#define TRACEFL_PAGE		512
#define TRACEFL_SITEM		64
#define TRACEFL_TOTITEM    ((TRACEFL_STOP+1L-TRACEFL_START)/TRACEFL_SITEM)
#define TRACEFL_SVALID      0x00C0000L	// only 6 block valid of 15 
#define TRACEFL_ITEMSVALID  (TRACEFL_SVALID/TRACEFL_SITEM)
#endif // #ifdef USE_TRACE

// 3208 used for crono download
#ifdef ENABLE_CRONO
// [rec size free]
//// CRONO		512k		0x180000	0x1fffff
#define CRONOFLASH_START	0x0A40000L
#define CRONOFLASH_STOP 	0x0D3FFFFL
#endif
// OTHER free
// Free(&Crash)	512k		0x180000	0x1fffff


// Function to start & stop NAND flash
extern void nandinit(void) ;
extern void nanddisable(void);
extern void SyncNAND(unsigned long bbegin) ;

// NAND define
#define NAND_ONEADDR

#define PTRNAND_DATA  ((volatile unsigned char *)0x90000000)
#ifndef NAND_ONEADDR
#define PTRNAND_CLE   ((volatile unsigned char *)0x90100000)
#define PTRNAND_ALE   ((volatile unsigned char *)0x90080000)
#endif
#define USE_DMA_NAND

// // Code upgrade: temporal area for new code download (2x512k = 1M)
// #define CODEFLASH_START 0x00000
// #define CODEFLASH_STOP  0xfffff

// NAND commands
#define K9FXX_READ_1            0x00
#define K9FXX_READ_2            0x30

#define K9FXX_READ_RND_1        0x05
#define K9FXX_READ_RND_2        0xe0

#define K9FXX_WRITE_RND         0x85

#define K9FXX_SET_ADDR_A        0x00
#define K9FXX_SET_ADDR_B        0x01
#define K9FXX_SET_ADDR_C        0x50
#define K9FXX_READ_ID           0x90
#define K9FXX_RESET             0xff
#define K9FXX_BLOCK_PROGRAM_1   0x80
#define K9FXX_BLOCK_PROGRAM_2   0x10
#define K9FXX_BLOCK_ERASE_1     0x60
#define K9FXX_BLOCK_ERASE_2     0xd0
#define K9FXX_READ_STATUS       0x70
#define K9FXX_BUSY              (1 << 6)
#define K9FXX_OK                (1 << 0)

