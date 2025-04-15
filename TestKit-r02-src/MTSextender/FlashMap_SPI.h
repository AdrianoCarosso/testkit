// Will be defined "USE_SPI_ON_ARM"
// ----------------------------- Flash map ------------------------------------
// External SPI Flash memory
//
// Area         Size     	From      		To
// ---------------------------------------------
// Upgrade      256k		0x000000	0x03ffff
// Parameters   128k		0x040000	0x05ffff
// VirtualRAM   256k		0x060000	0x09ffff
// Transaction  384k		0x0a0000	0x0fffff
// First Mega

// S.M.          64k		0x100000	0x10ffff	(Main and uploading) (256+256 item)
// Target        64k		0x110000	0x11ffff						 (3855 item)
// Canconf       32k		0x120000	0x127fff
// Crono(oldLog)256k		0x128000	0x167fff	(4096 item) from 1.64 for CRONO_DOWNLOAD

// IntLog        96k		0x168000	0x17ffff						 (1536 item)
// Log(oldFree) 512k		0x180000	0x1fffff				from 1.64 LOG_HISTORY (8192)  (record 64 byte)


#define LOG_8192  // NEW from 2.00

#ifndef LOG_8192
#undef ENABLE_CRONO
#undef TFTP_PDA
#endif

// 3208 used for crono download
#ifdef ENABLE_CRONO
//// CRONO		512k		0x180000	0x1fffff
#ifdef LOG_8192
#define CRONOFLASH_START	0x128000L		// from 1.64 	262143 bytes (256 k)
#define CRONOFLASH_STOP 	0x167FFFL
#else // LOG_8192
#define CRONOFLASH_START	0x180000L
#define CRONOFLASH_STOP 	0x1fffffL
#endif // LOG_8192
#endif
// OTHER free
// Free(&Crash)	512k		0x180000	0x1fffff

// Code upgrade: temporal area for new code download (max size=262,143)
#define CODEFLASH_START		0x000000L
#define CODEFLASH_STOP		0x03ffffL

// Parameters
#define PARAM_START			0x040000L
#define PARAM_STOP			0x05ffffL
#define PARAM_SSIZE			512

// VirtualRAM
#define USE_VIRTUALRAM_ON_ARM
#define USE_VIRTUALRAM_ON_FLASH
#define VIRTUALRAM_START	0x060000L
#define VIRTUALRAM_STOP		0x09ffffL
#define VIRTUALRAM_SSIZE	512

// Transactions
//#define USE_TRANSACTIONS_ON_ARM
#define FLASH_START     	0x0a0000L
#define FLASH_STOP      	0x0fffffL
#define FLASH_TSSIZE    	(8*512)        // _BM_ 28/8/2008 --> 60 macro sector
#define FLASH_TOTSIZE		(FLASH_STOP-FLASH_START+1)

#define FLASH_PAGESIZE		512

// S.M.
#define SMFLASH_START		0x100000L		// (46+1+64=111 -> 17 scratch) 128 byte/cad
#define SMFLASH_STOP		0x107FFFL
#define SMFLASH_UPSTART		0x108000L		// (46+1+64=111 -> 17 scratch) 128 byte/cad
#define SMFLASH_UPSTOP		0x10FFFFL
// SM record length
#define SMFLASH_MEMLEN	((SMFLASH_STOP-SMFLASH_START+1)/256)	// 128 byte (46+1+64=111 -> 17 scratch)

// NEW: TARGET
#define TGFLASH_START		0x110000L
#define TGFLASH_STOP		0x11FFFFL

// Canconf
#define CANxCONF_SIZE	   (0x004000L / LU_FORCAN)
#define CAN1CONF_START		0x120000L		// 8k CAN1Conf-128 lines
#define CAN1CONF_STOP	   (CAN1CONF_START + CANxCONF_SIZE -1)
#define CAN2CONF_START	   (CAN1CONF_STOP + 1 )// 8k CAN2Conf-128 lines
#define CAN2CONF_STOP	   (CAN2CONF_START + CANxCONF_SIZE -1)   // 0x1FBFFFL
#define CAN1CONF_UPSTART    0x124000L		// 8k CAN1Upgrade-128 lines
#define CAN1CONF_UPSTOP	   (CAN1CONF_UPSTART + CANxCONF_SIZE -1) // 0x1FDFFFL
#define CAN2CONF_UPSTART   (CAN1CONF_UPSTOP + 1 )	// 8k CAN2Upgrade-128 lines
#define CAN2CONF_UPSTOP	   (CAN2CONF_UPSTART + CANxCONF_SIZE -1) // 0x1FFFFFL


// log items
#ifdef LOG_8192
#define LOGFLASH_START		0x180000L		// 64 byte -> 8192 item from 1.64
#define LOGFLASH_STOP		0x1fffffL
#else // LOG_8192
#define LOGFLASH_START		0x128000L		// 64 byte -> 4096 item
#define LOGFLASH_STOP		0x167FFFL
#endif // LOG_8192
#define LOGFLASH_PAGE		512
#define LOGFLASH_SITEM		64

#ifdef USE_TRACE
// Internal log
#define TRACEFL_START		0x168000L		// 64 byte -> (96k / 64 ) = 1536 item
#define TRACEFL_STOP		0x17FFFFL
#define TRACEFL_PAGE        512
#define TRACEFL_SITEM		64
#define TRACEFL_TOTITEM    ((TRACEFL_STOP+1L-TRACEFL_START)/TRACEFL_SITEM)
#endif // #ifdef USE_TRACE




