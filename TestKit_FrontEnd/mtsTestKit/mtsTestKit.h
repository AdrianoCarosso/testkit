//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

// 0.10   06/07/10		Grafica Ok, inizio inserimento codice
// 2.03   23/07/15		Aggiunto gestione protocolli
// 2.04   24/02/16		Aggiunto controllo porte
// 2.05   20/06/18		Aggiunto porta USB MTS 
// 2.06   10/07/18		Gestito DiaG12
// 2.07   04/03/19    Aggiunto cancellazione parametri 
// 2.08   10/04/19    Aggiunto attesa canstart 
// 2.09   15/04/19    Aggiunto invio codice da seriale
// 2.10   03/09/19    Modificato tempi richiesta parametri
// 2.11   07/07/20    Aggiunti gestione TAG CAN
// 2.12   16/11/21    Aggiunto doSetProtocolComunication per cambio protocollo
// 2.13   25/05/23    Aggiunta variabile globale 'leveldebug' per regolare output di debug
// 2.13.1 18/12/23    Non toglie nei parametri il carattere '"'

// prevent re-include
#ifndef _MTSTESTKIT_H
#define _MTSTESTKIT_H

#define VER 2
#define SUBVER 14
#define REVISION 00

#define BANNER    "MTS TestKit"
#define TEXT_BOX  " - Diagnostic box"

#define LOW_INIT_NEW
#ifdef LOW_INIT_NEW
#define TK_PORTNAME 	"usb-03eb_6124-if00"
//#define MTS_PORTNAME	"usb-T.E.S.T._TestKit_901400106-if00-port0"
 //"usb-1a86_USB2.0-Serial-if00-port0"
#define MTS_USB_PORTPREFIX "usb-1a86"
#else
#define USB_DIR         "/dev/serial/by-id"
//#define TK_PORTNAME     "usb-03eb_6124"
#define TK_PORTNAME   "usb-03eb_6124-if00"
#define TK_PORTNAME2    "03eb_6124"  // "pci-03eb_6124-if01-port0"
#define MTS_PORTPREFIX  "usb-FTDI_DLP2232M_"
#define MTS_PORTPREFIXNEW "usb-T.E.S.T._TestKit_901400106"
#define MTS_PORTPREFIXNEW2 "T.E.S.T._TestKit_901400106"
#define MTS_PORTSUFFIX  "-if01-port0"
 //"usb-1a86_USB2.0-Serial-if00-port0"
#define MTS_USB_PORTPREFIX "usb-1a86"
#endif

#define CLEAR_MEM(_A, _B)   bzero(_A, _B)

typedef unsigned long ulong ;
extern int curtest;
extern int waithuman;
extern int MTS_current_PORT;

#define SLEEP(_A) usleep(_A*1000)


#define USE_MONITORING		// !!  Option for debug output !!
#define SEQUENCE_DEBUG		// !!  Option for debug output of sequence !!

#define TK_UPDATE		"T 1000\r"

// Message !
#define BADPORT_TK		"Porta TestKit non trovata\n"
#define BADPORT_MTS		"Porta MTS non trovata\n"
#define ERRPORT_TK		"Errore %d all'apertura della porta TK"
#define ERRPORT_MTS		"Errore %d all'apertura della porta MTS"
// Message !



enum{
PORT_TK,
PORT_MTS,
PORT_MTSUSB,
PORT_UDP,
PORT_MAX
};

#define MIN_STRING		  64
#define DEF_STRING 		 256
#define DEFAULTLEN       100     // default string length
#define MAX_STRING 		1024
#define MAX_STRING2		2048

#define SIZE_BUFFS		2048


// STRUCTURE for sequence
struct _SEQUENCE{
    pid_t 		pid;
    int	 		cgi_output[2];
    int	 		cgi_input[2];
	pthread_t	thr_p ;
	int 		BufferedMode ;
	int			NewCommand ;
	char		Command[MAX_STRING] ;
	int 		secStartTime ;			// for delay(s)
	int 		milliStartTime ;		// for delay(s)
	int 		TimeoutAnswer ;			// UNIT CSEC
#define TIMEOUT_TK  500 	// csec
#define TIMEOUT_MTS 500		// csec
#define TIMEOUT_CMD  50		// csec
	int 		ShellExitCode ;
};

extern struct _SEQUENCE Gsequence ;


// STRUCTURE for global data
struct _TKGDATA{
// Added from 2.13 (25/05/23) same name as MTScu
	uint32_t leveldebug ;
	char localefloat;
#define LOCALE_POINT 0
#define LOCALE_COMMA 1
#define CHR_COMMA ','
#define CHR_POINT '.'
	int GTK_START ;   				// GTK is Start
	//char lpath[MAX_STRING] ;		// Path 
	char upass[MAX_STRING] ;		// UserPass
	char deviceClass[MAX_STRING] ;	// Device selection

	int LU_src ;
	char portname[PORT_MAX][MAX_STRING] ; 		// TestKit port file name (with path)
	char portdev[PORT_MAX][MAX_STRING] ; 		// TestKit port file name (with path)
	int  portopened[PORT_MAX];
	//char MTSport[MAX_STRING] ; 		// MTS (by TK) port file name (with path)

	char pc_ip[MIN_STRING];
	int  pc_socket ;
	int  mts_sn ;
	char mts_ip[MIN_STRING];
	int  mts_socket ;
	
	int menu_choice ;
	int run_loop ;			// Status: 0=exit, 1=running
#define MAIN_END		0	// Exit
#define MAIN_RUN		1	// Script ended
	int sequence_status ;	// 0=ended , >0 running (1= wait command, 2=answering)
#define SEQ_ENDED		0	// Ended
#define SEQ_WAITCOMMAND	1	// Wait a command from sequence
#define SEQ_TOANSWER	2	// Wait data to send at sequence
#define SEQ_USERANSWER	3	// Wait input from user

	// Script command
	int lastSlaveCommandId ; 	// Command coded
	
	short lastSlaveSubCommandId ;
	char skipIncrememt ;					// 0 or 1
	
	char Terzista[MAX_STRING] ;
	char workingPath[MAX_STRING] ;
	char MtsName[MAX_STRING] ;
	char FileImpostazioni[MAX_STRING] ;
	char prgFileRadix[MAX_STRING] ;
	char ProgramFile[MAX_STRING] ;
	char Protocol[MAX_STRING];
	char pkt_offset ; // val of protocol
	char sn_next[MIN_STRING] ;
	char mominival[MAX_STRING] ;

	char usb_dir[MAX_STRING] ;
	char tk_portname[MAX_STRING] ;
	char mts_portname[MAX_STRING] ;

	int   bar_msec ;
	float bar_perc ;
	unsigned short bar_oldmsec ;
	
	char ib_data[MAX_STRING] ;
	FILE *up_sm; 
		
	int transnum ;
	int txbuflen ;
	int txpacketnum ;
	char trans_buff[SIZE_BUFFS] ; 	// Transaction buffer

	// When sending SM or FW the file descriptor is open up to end download
	FILE *down_sm;				// the value of 'MtsData.confirmidx' is acknoledge waited
	int   confirmidx ;
	int	  total_item;
	int  item_sended;
	// CAN
	int   CanChannel ;
	
	//File Lock
	FILE *filock;

	//Refresh_USB
	int refresh_USB;

	//Local IP
	char localIP[MAX_STRING];

	//Host name
	char hostname[128];

	//se richiesto IP HC12
	char MtsIMEIcheck[2];

	char rxok[2];

	unsigned short TKTYPE;  // Tipo di TestKit se 0 vecchio se 1 nuovo con SN maggiore di 901400100

	unsigned short RISP255;
	
	unsigned short okcansend;

	int rDiag;		// Request Diag
	
	FILE *down_fw ;				// the value of 'MtsData.confirmidx' is acknoledge waited
	short down_fw_tout ;
	short tout_next ;
	char systype ; 					// for FW upgrade
	int   item_old ; 			// file pointer of previous packet
	short delayed ;
	unsigned char bcrcl;
	unsigned char bcrch ;
#define FIRST_COM_TIMEOUT 5
	
} ;
extern struct _TKGDATA Gdata ;

// lastSlaveCommandId CODE
enum{		// _T_ = TestKit, _M_ = MTS , _U_ = User
WSCMD_NONE = 0,				// NONE
WSCMD_T_GETVER,				// from TestKit
WSCMD_T_GETTYPE,
WSCMD_T_GETDIGITAL,
WSCMD_M_DIAG0,				// from MTS
WSCMD_M_DIAG1,
WSCMD_M_DIAG2,
WSCMD_M_DIAG3,
WSCMD_M_DIAG4,
WSCMD_M_DIAG7,
WSCMD_M_DIAG9,
WSCMD_M_DIAG12,
WSCMD_M_DIAG25,
WSCMD_M_DIAG127,
WSCMD_M_DIAG250,
WSCMD_M_DIAGUNKNOWN,
WSCMD_M_CNT,
WSCMD_M_ANALOG,
WSCMD_M_FAMILY,
//GT//
WSCMD_WAIT_TKCOM,
WSCMD_M_FIX,
WSCMD_M_GETINTERNAL_AD,
WSCMD_M_GETDUPSMFLAGS,
WSCMD_M_GETDUPSMSTATUS,
WSCMD_M_INPUT,
WSCMD_M_GETTIME,
WSCMD_M_GETSWVERS,
WSCMD_M_GETHWVERS,
WSCMD_M_GETSERNUM,
WSCMD_M_GETPAR,
WSCMD_M_SETPAR,
WSCMD_M_PUTSMFILE,
WSCMD_M_GETSMFILE,
WSCMD_M_GETDIRECT,	
WSCMD_AT_OK,
WSCMD_AT_IMEI,
WSCMD_M_EXESM255,
WSCMD_M_CANSTART,
//GT//
WSCMD_U_INBOX,
WSCMD_U_MSGBOX,
WSCMD_U_TIMER,
WSCMD_M_PUTFWFILE,
};

// Structure with TestKit data

#define MAX_EXTENDER_ANALOGS	32
#define MAX_EXTENDER_COUNTERS 	10
#define MAX_TKCOMS 4
struct _TKVALS{
	char    VerSn[DEF_STRING];
    int		digitalsUpdated ;
    ulong	portA ;
    ulong	portB ; 
    ulong	portC ;
    ulong	portD ;
	ulong   portE ;
    int		analogsUpdated ;
    int		analogs[MAX_EXTENDER_ANALOGS] ; 	// 'NB: sovradimensionato per eventuali evoluzioni future
    long	counters[MAX_EXTENDER_COUNTERS] ;	// 'NB: sovradimensionato per eventuali evoluzioni future
    char	ComsRxBuffer[MAX_TKCOMS][MAX_STRING] ;
    int		ComsRxNr[MAX_TKCOMS] ;
	int 	portselect;	
	int 	linput;
};

extern struct _TKVALS ExtData ;

struct CAN_RXDATA {
long  	tag ; 
long    Address ; 
char    data[16] ;
};

#define DATALEN        5000
#define DATEFIELD		32
// Structure with MTS data
struct _MTSVALS{
#define RXMAXLEN 5000
unsigned char RxBuffer[RXMAXLEN] ;	// OK
int  RxBufLen ;						// OK

char Rxbridge[RXMAXLEN] ;			// OK
int  RxBridgeLen ;					// OK

#define MTS_DATA_NONE          0x000   // OK   // No data
#define MTS_DATA_INTAD         0x001   // OK   // New AD fields
#define MTS_DATA_AUXAD         0x002   // OK   // Internal AD data
#define MTS_DATA_PAR           0x004   // OK   // A par received
#define MTS_DATA_FIX           0x008   // OK   // Fix
#define MTS_DATA_IO            0x010   // OK   // I/O
#define MTS_DATA_ALARM         0x020   // OK   // Alarm
#define MTS_DATA_AD            0x040   // OK   // A/D
#define MTS_DATA_SMREAD        0x080   // OK   // SM
#define MTS_DATA_VOLUME        0x100           // Volume
#define MTS_DATA_REPORT        0x200   // OK   // Report
#define MTS_DATA_DIAG          0x400   // OK   // Diag data
#define MTS_DIRECT_DATA        0x800   // OK   // Hist I/O
//#define MTS_HIST_AD           0x1000           // Hist A/D
//#define MTS_HIST_REPORT       0x2000           // Hist Report
//#define MTS_HIST_SMREAD       0x4000           // Hist SM
#define MTS_DEFAULT_DATA      0x8000           // Default data
#define MTS_CONFIRM_DATA     0x10000           // Confirmation
#define MTS_ALARM_REPLY      0x20000           // Alarm reply
#define MTS_DATA_IP          0x40000   //OK    // IP address
#define MTS_DATA_ADCAN       0x80000           // A/D from CAN
#define MTS_DATA_SWAUXVER   0x100000           // CAN aux fw Ver
#define MTS_DATA_REQPOLL    0x200000           // Request a polling
//Define for SM / target / Can conf / Upgrade protocol
#define ACK_RNDMASK 0x000fffff
#define ACK_ADDPKT	0x00100000
#define ACK_SHPKT	20
#define ACK_LASTPKT	0xfff00000
int DataReady ;                            // bitmap with available data

// DATA_REPORT	-> MTS_DATA_REPORT
char Data_cpu[16] ;     // cpu serial number	// OK
char Data_swver[8] ;    // software version		// OK
char DRep_time[32] ;				// OK
char DRep_avgtel[16] ;				// OK
char DRep_pwrmain[16] ;				// OK
char DRep_pwrbatt[16] ;				// OK
char DRep_numpwr[16] ;				// OK
char DRep_numfix[16] ;				// OK
char DRep_numterm[16] ;				// OK
char DRep_teloper[16] ;				// OK
char DRep_csqtel[16] ;				// OK
char DRep_mtstype[16] ;				// OK

// DATA_AUX_SWVER
char Data_swAUXver[8] ;    // software version
// DATA_MODEM
char Data_modem[12] ;   // modem type

// DATA_FIX		-> MTS_DATA_FIX
char Data_fixtime[DATEFIELD] ;		// OK
char Data_fnsat[8] ;				// OK
char Data_fixtype[8] ;				// OK
char Data_flat[16] ;				// OK
char Data_flon[16] ;				// OK
char Data_falt[16] ;				// OK
char Data_fspd[16] ;				// OK
char Data_fspn[16] ;				// OK
char Data_fspe[16] ;				// OK
//char Data_fixsrc[4] ;
// DATA_IO		-> MTS_DATA_IO
char Data_src ; 
int32_t mts_iotime ;
char Data_iotime[DATEFIELD] ;		// OK
char Data_ioinp[8] ;				// OK
char Data_ioout[8] ;				// OK
char Data_iosrc[4] ;				// OK
char Data_cnt1[12] ;				// OK
char Data_cnt2[12] ;				// OK
// DATA_ALARM	-> MTS_DATA_ALARM
char Data_almtime[DATEFIELD] ;		// OK
char Data_almnum[8] ;				// OK
//char Data_almsrc[4] ;
// DATA_AD		-> MTS_DATA_AD
char Data_ioad[8][8] ;				// OK
char Data_iokm[16] ;				// OK
char Data_ioHinX[16] ;				// OK
// IDGPS_INTAD	-> MTS_DATA_INTAD
#define MAX_INTAD 50
short Data_nrintad ; 				// OK
char Data_intad[MAX_INTAD] [16];	// OK
short Id_intad[MAX_INTAD];
//char Name_intad[MAX_INTAD][8] ;		// OK
// IDGPS_AUXAD	-> MTS_DATA_AUXAD
#define MAX_AUXAD 11
short Data_nrauxad ; 				// OK
char Data_auxad[MAX_INTAD][16] ;	// OK
char Name_auxad[MAX_INTAD][8] ;		// OK

// DATA_SMREAD	-> MTS_DATA_SMREAD
char Data_ioflags[16] ;				// OK
char Data_iostat[16] ;				// OK
// DATA_VOLUME	-> MTS_DATA_VOLUME
char Data_volmic[16] ;				// OK  come parametri!
char Data_volspk[16] ;				// OK

//// HIST_FIX
//char Hist_fixtime[32] ;
//char Hist_flat[16] ;
//char Hist_flon[16] ;
//char Hist_falt[16] ;
//char Hist_fspd[16] ;
//char Hist_fspn[16] ;
//char Hist_fspe[16] ;
//char Hist_src[4] ;
//char Hist_log[4] ;
//// HIST_IO
//char Hist_iotime[32] ;
//char Hist_ioinp[8] ;
//char Hist_ioout[8] ;
//// HIST_AD
//char Hist_ioad[8][8] ;
//char Hist_iokm[16] ;
//char Hist_ioHinX[16] ;
//// HIST_REPORT
//char HRep_time[32] ;
//char HRep_avgtel[16] ;
//char HRep_pwrmain[16] ;
//char HRep_pwrbatt[16] ;
//char HRep_numpwr[16] ;
//char HRep_numfix[16] ;
//char HRep_numterm[16] ;
//char HRep_teloper[16] ;
//char HRep_csqtel[16] ;
//char HRep_mtstype[16] ;
//// HIST_SMREAD
//char Hist_ioflags[16] ;
//char Hist_iostat[16] ;
// DEFAULT_DATA
char Def_src[4] ;
char Def_lu[8] ;
char Def_pkt[8] ;
char Def_fld[8] ;
char Def_data[DATALEN] ;
char Dir_data[DATALEN] ;


// CONFIRM DATA
int confirmidx ;

#define IPLENGTH 20  // max IP string length
// DATA IP ADDRESS	-> MTS_DATA_IP
char Data_IP[IPLENGTH] ;		// OK
int Data_IPsckt ;				// OK
int Data_PcSckt ;				// OK
char Data_IP2[IPLENGTH] ;		// OK
int Data_PcSckt2 ;				// OK

// CAN_AD modified ver 4.08
char Data_canad[4][30] ; // old [4][12]
// ver 4.03 added 
char canflags[16] ;
// CAN
struct CAN_RXDATA lastCan13Rx ;
struct CAN_RXDATA lastCan14Rx ;

// DIAG DATA
int  diaglen ;
unsigned char diagdata[MAX_STRING] ; 	//  data from diag

// For upgrade FWpmc->RxBuffer[fp + 1]-2
//char newdataupgrade ; // Flag for activate refresh of 'mtsupgrading' and 'nextFWtout'
//int mtsupgrading ;
//time_t nextFWtout ;
#define UPG_NOTOUT 0
char Imei[50] ;
int  parnum ;
unsigned short hpval ;
int ipval ;
char parval[250] ;
// to display into TTY
char pardisp[SIZE_BUFFS] ;
char parldisp[SIZE_BUFFS] ;
int nrrevc ;
int usb ;
int rtc ;
// Transaction data
short curr_trans ;
short rx_trans ; 		// last transaction received
short rxok_trans ; 		// last transaction in sequence received
char setok; //check if ACK
};

extern struct _MTSVALS MtsData ;

// Logic units
#define LU0CPU   0
#define LU1APP   1
#define LU2TERM  2
#define LU5GPS   5
#define LU8GPRS  8
#define LU9GSM   9
#define LU11PC  11
#define LU13CAN 13
#define LU14CAN 14

#define PAR_SWAUXVER 	223




// STRUCTURE for manage amperometer
struct _AMPDATA{
	double  out_scale ; 	// IS out of scale
	int     max_value ; 	// End scale
	char    title[10] ;		// Label
	double  value ;			// Current value
	double  old_value ;		// Old value
};

extern struct _AMPDATA Val_amp ;

extern char free_msg[MAX_STRING2] ;

// from MtsTestKit.c
extern void StatusIcon(int blink);
extern void ProgressBar(int type, int value) ;
extern void InputBox(int msgbox, char * caption, char *title, int nkey, char *lkey1, char *lkey2) ;
extern void CenterForm(GtkWindow *form) ;
extern int setinival(char *bloc,char *par,char *con) ;
extern char * getinival(char *stringd,char *bloc,char *par) ;
extern int flock(int finito);
extern void Reset_Screen(void);

// from forXXXXX.c
extern int Low_Init(int repeat) ; 
extern int baud_select(int bval) ;
extern int  com_open(int port, int brate, int flowcontrol) ;
extern void com_write(int port, int len, char * msg) ;
extern int  com_inlen(int port) ;
extern int  com_read(int port, int maxlen, char * msg) ;
extern int  com_read_char(int port, char * buf) ;
extern void com_close(int port) ;

extern int  run_sequence(void) ;
extern void Stop_sequence(void) ;
extern int  Check_sequence(void) ;
extern void Send_sequence_answer(char *answer) ;
extern int Start_command(int argc, char *argv[]) ;
extern int com_baud(int port, int brate)  ;

extern char* TimeDB(const time_t* loctime, char *aa ) ;
extern char* rem_duble_slash(char *name, char *mom) ;


// from mts_tk_man.c
extern unsigned short GetCRC_CCITT(unsigned char *pbytData, int intNumByte, unsigned short crc) ;
extern void MTS_AddPacket(int lu, int pkt, int field, char *data, int ldata ) ;
extern int SrcDst(int lu) ;
extern void MTS_SendTransaction(void) ;
extern int start_sm(char *fname, char compile) ;
extern int start_fw(char *fname) ;
extern void step_fw(int retry) ;
extern int sendoldcanconf(char *fname);
extern int sendnewcanconf(char *fname);


// for GPRS
extern int CheckIPvalid(char *l_ip) ;
extern void MTS_TransByGprs(void) ;

// from ListClassDevices
extern gboolean close_selection( GtkWidget * aa, gpointer  data ) ;
extern void set_selection( GtkWidget * aa, gpointer  data) ;
extern int GetSelData(void) ;

// from sequence
extern int  Start_sequence(void) ;
extern void Get_sequence_command(void) ;
extern void Check_sequence_timeout(void) ;
#endif
