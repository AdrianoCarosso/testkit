//   Copyright (c) 1997-2004.
//   T.E.S.T. srl
//  ------------------------------------------------------------------------
//				MTS family
//  ------------------------------------------------------------------------
#include <stdint.h>

#ifdef FR_WIN32
#pragma pack(push,1)
#else
#pragma pack(1)
#endif

struct PACKET_BEGIN {
unsigned char pk_sd :8 __attribute__ ((packed));			// Hi nibble=source; Low-nibble=destination
unsigned char pk_type :8 __attribute__ ((packed));			// packet type: valid values 0-31 only
uint16_t pk_len :16 __attribute__ ((packed));			// total length of packet in bytes
unsigned char pk_tot :8 __attribute__ ((packed));			// number of PACKET_FIELD structures they follow
} ;

struct PACKET_FIELD {
unsigned char pf_id :8 __attribute__ ((packed));			// This data field identifier
unsigned char pf_len :8 __attribute__ ((packed));			// total length of data field it follows
} ;

struct TRANSACTION_HEADER {
uint16_t th_transaction :16 __attribute__ ((packed));	// transaction number
uint16_t th_length :16 __attribute__ ((packed));		// total length of transaction in bytes
unsigned char th_tot :8 __attribute__ ((packed));			// number of packets belonging to this transaction
} ;

struct EHEADER_TYPE_0 {			// This header is designed for SMS based links
uint16_t hsequence :16 __attribute__ ((packed));		// (value & 0x1fff) | (0 << 13)
unsigned char cur_tot :8 __attribute__ ((packed));			// Hi-nibble=total frames; Low-nibble=this frame number
unsigned char length :8 __attribute__ ((packed));			// total length of this frame (CRC included)
} ;								// (length-sizeof(struct EHEADER_TYPE_0)-2) data bytes follow

struct EHEADER_TYPE_1 {			// This header is designed for HAYES modem based links
uint16_t hsequence :16 __attribute__ ((packed));		// (value & 0x1fff) | (1 << 13)
unsigned char cur_tot :8 __attribute__ ((packed));			// Hi-nibble=total frames; Low-nibble=this frame number
uint16_t length :16 __attribute__ ((packed));			// total length of this frame (CRC included)
uint32_t source :32 __attribute__ ((packed));			// source signature
} ;								// (length-sizeof(struct EHEADER_TYPE_1)-2) data bytes follow

struct EHEADER_TYPE_2 {			// This header is designed for RADIO based links
uint16_t hsequence :16 __attribute__ ((packed));		// (value & 0x1fff) | (2 << 13)
unsigned char cur_tot :8 __attribute__ ((packed));			// Hi-nibble=total frames; Low-nibble=this frame number
uint16_t length :16 __attribute__ ((packed));			// total length of this frame (CRC included)
uint32_t source :32 __attribute__ ((packed));			// source signature
uint32_t destination :32 __attribute__ ((packed));		// destination signature
} ;								// (length-sizeof(struct EHEADER_TYPE_2)-2) data bytes follow

// GPRS
#define UDPSOCKET 5100

struct EHEADER_TYPE_GPRS {		// This header is designed for GPRS based links
uint16_t hlen :16 __attribute__ ((packed));			// header total len
unsigned char id :8 __attribute__ ((packed));				// header identifier (0)
unsigned char length :8 __attribute__ ((packed));			// total length of this identifier
uint32_t source_num :32 __attribute__ ((packed));		// source signature number
uint32_t source_id :32 __attribute__ ((packed));		// source signature id
uint32_t dest_id :32 __attribute__ ((packed));			// destination signature id
uint32_t dest_ip :32 __attribute__ ((packed));			// destination ip address
} __attribute__ ((packed));

struct EH_TYPE_GPRS1 {			// This header is designed for GPRS based links
unsigned char id :8 __attribute__ ((packed));				// header identifier (1)  + 0x80 crypted
unsigned char length :8 __attribute__ ((packed));			// total length of this identifier
uint32_t source_ip :32 __attribute__ ((packed)) ;		// destination ip address
} __attribute__ ((packed));

struct EH_TYPE_GPRS2 {			// This header is designed for GPRS based links
unsigned char id :8 __attribute__ ((packed));				// header identifier (2)
unsigned char length :8 __attribute__ ((packed));			// total length of this identifier
uint32_t ip :32 __attribute__ ((packed));				// destination ip address
uint16_t socket :16 __attribute__ ((packed));			// destination socket
} __attribute__ ((packed));

#define PKT_COMMAND		0	// Packet id: send this command
#define PKT_GPSIO		1	// Packet id: GPS and/or I/O data
#define PKT_USER_A		2	// Packet id: User type -A- from mobile to base
#define PKT_USER_B		3	// Packet id: User type -B- from base to mobile
#define PKT_REPORT		4	// Packet id: Generic report
// New (from MTS40)
#define PKT_ASCIIBIN	8	// Packet id: trasporto dati ASCII e BINARI
#define PKT_ASCII		9	// Packet id: trasporto dati ASCII
#define PKT_BINARY	   10	// Packet id: trasporto dati BINARI


#define IDCMD_DIRECT	0	// Command packet: direct raw data
#define IDCMD_STATUS	1	// Command packet: status report
#define IDCMD_SET		2	// Command packet: set parameter to value
#define IDCMD_RESET		3	// Command packet: clear workspace
#define IDCMD_RESTART	4	// Command packet: re-send form last IDCMD_RESET
#define IDCMD_RETRY		5	// Command packet: re-send transaction
#define IDCMD_ACK		6	// Command packet: Last transaction acknowledged
#define IDCMD_BOOT		7	// Command packet: Boot info
#define IDCMD_PIN		8	// Command packet: PIN code of SIM card
#define IDCMD_DIAL		9	// Command packet: Dial number
#define IDCMD_HANG		10	// Command packet: hang up telephone
#define IDCMD_VOL		11	// Command packet: change telephone volume
#define IDCMD_MIC		12	// Command packet: change mic volume
//#define IDCMD_DTMF	  12	   // Command packet: send tone during call
#define IDCMD_SYNQ		13	// Command packet: Synch request (int32_t)
#define IDCMD_SYNA		14	// Command packet: Synch answer  (int32_t)
#define IDCMD_SYNQA		15	// Command packet: Synch request (Ascii)
#define IDCMD_SYNAA		16	// Command packet: Synch answer  (Ascii)
#define IDCMD_DCLR		18	// Command packet: Clear Ram
#define IDCMD_DATA		19	// Command packet: Load  Ram
#define IDCMD_PROG		20	// Command packet: Update Flash from Ram
#define IDCMD_HISTORY	21	// Command packet: Get Historical data
#define IDCMD_HISTCLR	22	// Command packet: Clear historical data

#define IDCMD_CHLD		23	// Command packet: Call hold and MultiParty
#define IDCMD_GPRSIP	24	// Command packet: Request local IP
#define IDCMD_SETBIT	25	// Command packet: set bits par to 1
#define IDCMD_UNSETBIT	26	// Command packet: set bits par to 0
#define IDCMD_BINDIRECT	27	// Command packet: direct BIN raw data

#define IDCMD_PING		30	// Command packet: private ping to central
#define IDCMD_PLUGNAME	31  // Command packet: get plug-in name (or upgrade file name)
#define IDCMD_UPGDELAY	32  // Command packet: execute upgrade at first shutdown

#define IDCMD_LOCCMD   252	// Command packet: local command to LU
#define IDCMD_TRACE	   253  // Command packet: trace upload (and control)
#define IDCMD_DIAG	   254	// Command packet: return diag of lu9 and lu5
#define IDCMD_DEBUG	   255	// Command packet: debug ascii string 



struct CMD_BOOT {
uint32_t act_time :32 __attribute__ ((packed));		// actual time in sec. from Jan. 1, 1970 at 0 a.m.
uint32_t off_time :32 __attribute__ ((packed));		// turn off time in sec. from Jan. 1, 1970 at 0 a.m.
uint32_t sernum :32 __attribute__ ((packed));			// device serial number
unsigned char sw_rel :8 __attribute__ ((packed));			// software release (hexadecimal)
unsigned char sw_subrel :8 __attribute__ ((packed));		// software sub-release (hexadecimal)
unsigned char reset_type :8 __attribute__ ((packed));		// 0 = Normal
								// 1 = Normal with Power On Reset
								// 2-255 = Abnormal off
} ;

#define IDGPS_FULLFIX	0	// GPS_IO packet: full format fix
#define IDGPS_ABSFIX	1	// GPS_IO packet: absolute fix
#define IDGPS_RELFIX	2	// GPS_IO packet: relative fix
#define IDGPS_IO		3	// GPS_IO packet: status of digital inputs
#define IDGPS_MSKIO		4	// GPS_IO packet: set digital outputs
#define IDGPS_ALARM		5	// GPS_IO packet: alarm report
#define IDGPS_AD		6	// GPS_IO packet: status of analog inputs
#define IDGPS_TRACK		7	// GPS_IO packet: tracking parameters
#define IDGPS_STATION	8	// GPS_IO packet: stationing report

#define IDGPS_ALMQUIT	9	// GPS_IO packet: quit alarm

#define IDGPS_SMCLEAR	10	// GPS_IO packet: clear status machine
#define IDGPS_SMADD		11	// GPS_IO packet: add rule to status machine
#define IDGPS_SMSTART	12	// GPS_IO packet: start status machine
#define IDGPS_SMDUMP	13	// GPS_IO packet: reply current status machine
#define IDGPS_SMREAD	14	// GPS_IO packet: reply flags and status
#define IDGPS_SMWRITE	15	// GPS_IO packet: change status

#define IDGPS_SETKM		16	// GPS_IO packet: set new Km counter

#define IDGPS_TGCLEAR	17	// GPS_IO packet: clear target list
#define IDGPS_TGADD		18	// GPS_IO packet: add target to list
#define IDGPS_TGDUMP	19	// GPS_IO packet: reply current target list

#define IDGPS_AFIXCELL	20	// GPS_IO packet: absolute fix - WITH CELLS DATA
#define IDGPS_REBOOT	21	// GPS_IO packet: from 2.65
//#define IDGPS_DABSFIX	20	// GPS_IO packet: absolute fix - WITH DGPS DATA
//#define IDGPS_MDABSFIX	21	// GPS_IO packet: absolute fix - WITH MEAN DGPS DATA

#define IDGPS_SETIN0T	22	// GPS_IO packet: set new time_In0
#define IDGPS_SYNC		23	// GPS_IO packet: Command syncro (centr->MTS)(int32_t)
#define IDGPS_NMEA		24	// GPS_IO packet: NMEA data

#define IDGPS_SM64CLEAR	25	// GPS_IO packet: clear status machine
#define IDGPS_SM64ADD	26	// GPS_IO packet: add rule to status machine
#define IDGPS_SM64START	27	// GPS_IO packet: start status machine
#define IDGPS_SM64DUMP	28	// GPS_IO packet: reply current status machine
#define IDGPS_SM64READ	29	// GPS_IO packet: reply flags and status
#define IDGPS_SM64WRITE	30	// GPS_IO packet: change status

#define IDGPS_AFCELLS	31	// GPS_IO packet: absolute fix - WITH CELLS DATA (NEW)

#define IDGPS_TGFAM1 	32 // GPS_IO packet: add target FAMILY1 to list
#define IDGPS_TGFAM2 	33 // GPS_IO packet: add target FAMILY2 to list

#define IDGPS_SMCAN		40	// GPS_IO packet: Can SM append
#define IDGPS_ADCAN		41	// GPS_IO packet: Can A/D data
#define IDGPS_FLAGCAN	42	// GPS_IO packet: flags of Can
#define IDGPS_INTAD 	43	// GPS_IO packet: internal A/D	(Vext, Vbatt, CSQ, Temp, ACC x-y-z)
#define IDGPS_AUXAD 	44	// GPS_IO packet: auxiliary A/D (Gyro, HwRevSRV, ..)

#define IDGPS_ECHO	   127  // SPECIAL: for MTScu echo packet
#define IDGPS_EXESM	   255	// GPS_IO packet: execute received SM command

struct GPS_FULLFIX {
uint32_t ftime :32 __attribute__ ((packed));			// time of fix in sec. from Jan. 1, 1970 at 0 a.m.
int16_t speednorth :16 __attribute__ ((packed));				// speednorthKmh = speednorth * 0.072
int16_t speedeast :16 __attribute__ ((packed));				// speedeastKmh  = speedeast  * 0.072
int32_t ilat :32 __attribute__ ((packed));						// lat=(double)(ilat)/(double)(0x40000000L)*90.0
int32_t ilon :32 __attribute__ ((packed));						// lon=(double)(ilon)/(double)(0x40000000L)*90.0
uint16_t alt :16 __attribute__ ((packed));			// altitude in meters
uint16_t flags :16 __attribute__ ((packed));			// Fix details: NumSat, 2d/3d, Diff,Filtered, etc.
} ;

struct GPS_ABSFIX {
uint32_t ftime :32 __attribute__ ((packed));			// time of fix in sec. from Jan. 1, 1970 at 0 a.m.
int16_t speednorth :16 __attribute__ ((packed));				// speednorthKmh = speednorth * 0.072
int16_t speedeast :16 __attribute__ ((packed));				// speedeastKmh  = speedeast  * 0.072
int32_t ilat :32 __attribute__ ((packed));						// lat=(double)(ilat))/(double)(0x40000000L)*90.0
int32_t ilon :32 __attribute__ ((packed));						// lon=(double)(ilon))/(double)(0x40000000L)*90.0
} ;

struct GPS_RELFIX {
uint16_t rtime :16 __attribute__ ((packed));			// delta time of fix in sec.
int16_t speednorth :16 __attribute__ ((packed));				// speednorthKmh = speednorth * 0.072
int16_t speedeast :16 __attribute__ ((packed));				// speedeastKmh  = speedeast  * 0.072
int16_t rilat :16 __attribute__ ((packed));					// delta latitude
int16_t rilon :16 __attribute__ ((packed));					// delta longitude
} ;

// CHANGE FROM 2.42 ADDED char flag ;
struct GPS_IO {
uint32_t ftime :32 __attribute__ ((packed));			// time of fix in sec. from Jan. 1, 1970 at 0 a.m.
uint16_t dgin :16 __attribute__ ((packed));			// digital input
uint16_t dgout :16 __attribute__ ((packed));			// digital output
unsigned char flags :8 __attribute__ ((packed));			// same of high word of dgin
uint32_t cnt1 :32 __attribute__ ((packed));		// raw counter 1
uint32_t cnt2 :32 __attribute__ ((packed));		// raw counter 2
};

struct GPS_MSKIO {
uint16_t dgset :16 __attribute__ ((packed));			// new digital output status
uint16_t mask :16 __attribute__ ((packed));			// bit really involved
} ;

struct GPS_ALARM {
uint16_t code :16 __attribute__ ((packed));			// alarm code
uint32_t ftime :32 __attribute__ ((packed));			// time of fix in sec. from Jan. 1, 1970 at 0 a.m.
} ;

struct GPS_AD {
uint32_t ftime :32 __attribute__ ((packed));			// time of fix in sec. from Jan. 1, 1970 at 0 a.m.
int16_t adval[8] __attribute__ ((packed));				// Analog inputs
uint32_t km :32 __attribute__ ((packed));				// space counter, in meters
uint32_t time_In0 :32 __attribute__ ((packed));		// minutes with In0 active
} ;

struct GPS_TRACK {
uint32_t stime :32 __attribute__ ((packed));			// start time (0=now)
uint32_t etime :32 __attribute__ ((packed));			// end time (0=never, -1=stop)
uint16_t delta :16 __attribute__ ((packed));			// period in sec.
} ;

#define IDREP_STATS	2		// REPORT packet: Statistics
#define IDREP_IP	3		// REPORT packet: local IP address

struct REP_STATS_HDR {
uint32_t act_time :32 __attribute__ ((packed));		// actual time in sec. from Jan. 1, 1970 at 0 a.m.
uint32_t sernum :32 __attribute__ ((packed));			// device serial number
unsigned char sw_rel :8 __attribute__ ((packed));			// software release (hexadecimal)
unsigned char sw_subrel :8 __attribute__ ((packed));		// software sub-release (hexadecimal)
unsigned char genflags :8 __attribute__ ((packed));		// Flags:
								// bit0: 0=normal, 1=reset stats after packet
} ;

struct REP_STATS {
unsigned char avg_tel :8 __attribute__ ((packed));			// average telephone field
uint16_t opt_main :16 __attribute__ ((packed));		// minutes with main power
uint16_t opt_bat :16 __attribute__ ((packed));		// minutes with battery
uint16_t num_pwr :16 __attribute__ ((packed));		// num of main/battery switches
uint32_t num_fix :32 __attribute__ ((packed));			// num of GPS fix
uint16_t num_term :16 __attribute__ ((packed));		// num of Terminal insertions
uint32_t tel_oper :32 __attribute__ ((packed));		// telephone operator
unsigned char tel_field :8 __attribute__ ((packed));		// telephone field now
unsigned char mts_model :8 __attribute__ ((packed));		// MTS model
} ;



// For CAN
// not used
#define IDBIN_ACCELER  1	// accelerometro seriale

#define IDBIN_CDATA    9    // Can Data
#define IDBIN_CCCLEAR 10    // Can Conf Clear
#define IDBIN_CCADD   11    // Can Conf Add
#define IDBIN_CCSTART 12    // Can Conf Restart
#define IDBIN_CCDUMP  13    // Can Conf Read
#define IDBIN_CMSET   14    // Set a can macro value

#define IDBIN_CCANBUS	27	// da LU13, dati canbus
// not more used
#define IDBIN_BCANBUS	28	// da LU14, Canbus tipo B - LOW SPEED



//#define SWAP(A) (A)
#define SWAP(A) ((uint16_t) ((A & 0x00ff)<<8) | ((A & 0xff00)>>8) )

#define TSWAP(A) ( ((A & 0x00ff)<<8) | ((A & 0xff00)>>8) )
#define LSWAP(A) ( (uint32_t) (TSWAP((A & 0xffff))<<16 | TSWAP(((A & 0xffff0000)>>16))) )

//extern int32_t LSWAP(int32_t val) ;
//#define SWAP(A)  _asm("\texg a,b\n",A)   // Intel strikes again

// Can configuration

// per WAY
struct W_CAN_CONFTIME{
uint32_t thr_low  :32 __attribute__ ((packed)); // soglia minima (valore come da can)
uint32_t thr_high :32 __attribute__ ((packed)); // soglia minima (valore come da can)
} ;	// size is 8

struct W_CAN_MULTISMBIT{
unsigned long bitmask  :32 __attribute__ ((packed)); // mask   su SmCanBits
unsigned long bitvals  :32 __attribute__ ((packed));// valore di SmCanBits
}  ;	// size is 8

struct W_CAN_CONFSUM{
int32_t k_off1 :32 __attribute__ ((packed));
unsigned char fst_bit1 ;
unsigned char bit_len1 ;
} ;	// size is 6

union W_CAN_TYPE{
	//struct _CAN_CONFACC  can_acc ;
	struct W_CAN_CONFTIME can_time ;
	struct W_CAN_CONFSUM  can_sum ;
	struct W_CAN_MULTISMBIT can_bit ;
} __attribute__ ((packed)) ;	// size is max(8,6) 

struct W_CAN_LINE{
unsigned char	type ;			// 0-Polling, 1-Accum, 2-Time, 3-Sum, 4-SM can bit (as Time) ..
								// bit 8 at 1 tipo RESEC 
unsigned char	SMbit ;			// SM Bit to change
uint16_t  send_time :16 __attribute__ ((packed));     // 0 = no send
uint32_t	address :32 __attribute__ ((packed));
uint32_t	addr_mask :32 __attribute__ ((packed));     // 0x8XXXXXXX -> Standard
uint32_t	tag :32 __attribute__ ((packed));
unsigned char   flags ;		
							// 1 bit per transazionato
							// 1 bit per invertire il byte
							// 1 bit per conta quando interno o esterno
							// 1 bit per controllo errore
unsigned char   ludest ;			// 0x0 -> no_dest
unsigned char   fst_bit ;
unsigned char   bit_len ;
float           k_mult ;
int32_t         k_off :32 __attribute__ ((packed));
union W_CAN_TYPE macro ;
}  __attribute__ ((packed)) ; // size is 28+ union(8) -> 36
// Way Type can
#define WCAN_POLLING 1
#define WCAN_ACCUM   2
#define WCAN_TIME    3
#define WCAN_PSUM    4
#define WCAN_PHIST   5
#define WCAN_COUNT   6
// New from 1.22
#define WCAN_SEND    7
// New from 1.56
#define WCAN_TIMEALL 8
// New from 1.60
#define WCAN_CRONODR 9
// New from 1.61
#define WCAN_DRIVERID 10	// Macro 16 byte (Driver_id)

//#ifdef CONF_T
#define WCAN_DAYDRIVE 11	// Daily hours driving 
#define WCAN_DAYTIME  12	// Macro 8 byte (daily reset)
#define WCAN_HTL	  32	// From HTL (will be defined WCAN_DAYDRIVE and a POLLING to 0x00FEE
//#endif

// New with smbit mask & value
// This types will be at end of conf
#define WCAN_MTIME	 33	// Time if condition on smbits ON
#define WCAN_MDELTA	 34	// Delta value when condition on smbits is ON


// New from 2.10 (driving style)
#define WCAN_IST4SLOT 64	// HIstogram 4 slots ( 8 byte) + 4 header (bit 7 at 1)
#define WCAN_IST6SLOT 65	// HIstogram 6 slots (12 byte) + 4 header (bit 7 at 1)
#define WCAN_IST8SLOT 66	// HIstogram 8 slots (16 byte) + 4 header (bit 7 at 1)

// Way Can Flags bit
#define WCAN_TRANS   0x1
#define WCAN_REVERSE 0x2
#define WCAN_OUTSIDE 0x4
#define WCAN_BITERR  0x8

#ifdef FR_WIN32
#pragma pack(pop)
#endif
