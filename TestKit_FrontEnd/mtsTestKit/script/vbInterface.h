// Global definition
#ifndef _VBINTERFACE_H
#define _VBINTERFACE_H


#define KMAX_EXT_ANALOGS 32
#define KMAX_EXT_COUNTERS 10
#define KUSED_EXT_ANALOGS 19
#define KUSED_EXT_COUNTERS 2
typedef struct {
		unsigned int digital64;							//valore rimappato
		unsigned int digitals[8];						//valori originali da Extender
		unsigned int analogs[KMAX_EXT_ANALOGS];			//analogici da Extender
		unsigned int counters[KMAX_EXT_COUNTERS];		//analogici da Extender
} _ExtenderData ;
	
_ExtenderData T_ReadAll();


typedef struct {
		int dummy;
		int Vext;
		int Vbatt;
		int csq;
		int temp;
		int x;
		int y;
		int z;
		int extin;
		int adv1;
		int adv2;
} _InternalAnalogData ;
	
_InternalAnalogData M_ReadIntAD();

typedef struct {
		int HwMain;
		int HwSrv;
} _HwVers ;
	
_HwVers M_GetHwVers();



#define KPROTOCOLOMODE_QUERYANSWER 	0
#define KPROTOCOLOMODE_BUFFERED 	1
unsigned int SetProtocolMode(int mode);
unsigned int SetProtocolComunication(int mode);



extern unsigned int TKERROR;		//assume valori != 0 se errore durante comando

// -----------------------------------------------------------------------
// FUNCTIONs PROTOTYPE
// -----------------------------------------------------------------------


// ---------------- MTSTestKit program functions
unsigned int MsgWindow(char * );										// Display message into lower textbox
																		// Display message box to user
unsigned int MsgBox(char * text, char * label ,int ntasti, char * caption1, char * caption2, char * buf);
																		// enquire user for a data
unsigned int InputBox(char * text, char * label ,int ntasti, char * caption1, char * caption2, char * buf);
unsigned int MsgFile(int data ,char * fileName, char * mess);			// Write message into a file
unsigned int Delay(int) ;					// Execute a (ds) delay time and return answer when elapsed
unsigned int GetINIKeyVal(char * key, char * buf); 						// Get a key from INI file
unsigned int SetINIKeyVal(char * key, char * val);						// Set a key into INI file
unsigned int SetLevelDebug(uint32_t leveldbg);					//_FR - Rel 3.76 - 26/05/23
unsigned int GetBestFileNum(char * path, char * basenome, char * buf);	// Get higher code version file
unsigned int GetWorkSpace(char * buf);									// Get path of data files
unsigned int GetTerzista(char * buf) ;
									// Set a free label (beside mA and Vext) text and color (Q is nr. of label)
unsigned int OuputText(int q, char * text ,int cbase, char * label, int lamp, int clamp);
#define C_DEFAULT   0
#define C_RED 		1
#define C_GREEN 	2
#define C_YELLOW	3
#define C_WHITE		4
#define C_BLACK		5
#define C_BLUE		6
#define C_CYAN		7
#define C_MAGENTA	8
#define C_GREY		9	

unsigned int ProgressBar(int tipo, int param);					// Set type (if 0 param is percent(0-100), 
																// if 1 param is total time in dsec) 
#define BAR_PERC 0
#define BAR_TIME 1											
void retryOnError(int retryNum);								// Set nr of retry if a error
unsigned int PrgExit(char * );									// End script execution

// // Define com port parameter 
// unsigned int SetComPort(unsigned int baudrate, char parity, unsigned int bits, unsigned int stop, char handshake);


// ---------------- MTS functions
unsigned int DoProgram(char * path, char * program, char * argos); 		// Execute program file
unsigned int M_GetSerNum();												// Get MTS serial number
unsigned int M_GetSwVers();												// Get MTS SW ver
unsigned int M_GetFamily();												// Get MTS family
unsigned int M_GetTime();												// Get MTS internal date/time
unsigned int M_SetTime(unsigned int currenttime, unsigned int newtime);	// SET MTS internal date/time
unsigned int M_GetIMEI(char * buf);										// Get GSM IMEI string from MTS
unsigned int M_GetDirect(char * buf);									// Get last IDCMD_DIRECT from MTS
unsigned int M_Input();													// Get MTS digital input 
unsigned int M_Output(unsigned int m, unsigned int v);					// SET MTS digital output
unsigned int M_InStatus();												// Get MTS SM status (R/W)
unsigned int M_SetStatus();												// SET MTS SM status (R/W)
unsigned int M_InVirt();												// Get MTS SM flags (Ronly)
unsigned int M_Analog(int channelId);									// Get MTS Analog channel value
unsigned int M_SetSourceId(int id);										// SET comunication LU (11 or 2)
unsigned int M_DirectTOLU (unsigned int LU, char * Data);
//unsigned int M_GetSerNumCOM2();		// disabled
unsigned int M_GetGpsFlags();											// Get gps fix flag
unsigned int M_Diag(unsigned int type, unsigned int val,char * risp);	// Get a MTS diag value
unsigned int M_Action(unsigned int actcode, unsigned int actval, char * actpar); // Execute a SM command
unsigned int M_Actionwithresp(unsigned int actcode, unsigned int actval, char * actpar,char * buf);

unsigned int M_Cnt(unsigned int counterId);								// Get a MTS counter

unsigned int M_GetPar(unsigned int num, char * val);					// Get a MTS parameter
unsigned int M_SetPar(unsigned int num, char * val);					// SET a MTS parameter
unsigned int M_DelPar(unsigned int num);								// Remove a MTS parameter

unsigned int M_GetSmFile(char * filename);								// Upload SM file
unsigned int M_PutSmFile(char * filename);								// Download SM file
unsigned int M_SENDCANCONF(int type,char * filename);							// Download CAN CONF

unsigned int M_PutFWFile(char * filename,char *risp);								// Download FW file


//can MTS
unsigned int M_CanClear(unsigned int ch, unsigned int baudrate);		
unsigned int M_CanAdd(unsigned int ch, unsigned int txperiod, unsigned int canAddr, unsigned int canMask);
unsigned int M_CanStart(unsigned int ch, char * buf);
unsigned int M_checkcanstart(unsigned int ch);

unsigned int M_CanClearBuffer(unsigned int ch); 
unsigned int M_CanCheck(unsigned int ch, unsigned int canAddr, char * databyte); 

//unsigned int M_GetCSQ();	//sostituita dalla M_ReadIntAD()
//unsigned int M_GetSerNumCOM2();
//unsigned int M_Diag(unsigned int type, unsigned int val);
//seriali MTS
//unsigned int M_ComSend (unsigned int LU_Num, unsigned char * databyte, unsigned int datalen);
//unsigned int M_ComFlush (unsigned int LU_Num);
//unsigned int M_CheckRec (unsigned int LU_Num, unsigned char * databyte, unsigned int datalen); 



// ---------------- TestKit functions
unsigned int T_Input(void);												// Get TestKit digital input
unsigned int T_GetVer(char * buf);										// Get serial & version from TestKit
unsigned int T_GetType(char * buf);									    // Get Type Testkit
unsigned int T_SetFTDI(char * buf);										// Set Param -p to FTDI

unsigned int T_Analog(unsigned int channelId);							// Get TestKit Analog channel value
unsigned int T_Cnt(unsigned int counterId);								// Get TestKit counter value
unsigned int T_Output(unsigned long long m, unsigned long long v);		// Set TestKit digital output
unsigned int T_SetPull(unsigned long long m, unsigned long long v);					// Set TestKit digital in pull
unsigned int T_Led(unsigned int mask, unsigned int period);				// Set TestKit green led blinker

//can Extension
unsigned int T_SetCanBaudrate(unsigned int canNum, unsigned int baudrate);
unsigned int T_SetCanMailbox(unsigned int canNum, unsigned int mailboxnum, unsigned int canMask, unsigned int canAddr, char extended, unsigned int txperiod);
unsigned int T_EmitCanFrame(unsigned int canNum, unsigned int mailboxnum,  char * databyte);

//Seriali extender
unsigned int T_SetComPort(unsigned int portNum, unsigned int baudrate, char parity, unsigned int bits, unsigned int stop, char handshake);
unsigned int T_ComSend(unsigned int port, char * Data);
unsigned int T_ComSendHex(unsigned int port, char * HexData);
unsigned int T_CheckRec(unsigned int portNum, char * Data); 


// unsigned int T_ComPortOption(unsigned int port, char * opz);
// unsigned int T_ComPortSend(unsigned int port, char * values);

unsigned int PrintDB(char * buf);

// ---------------- internal VBINTERFACE functions
unsigned int M_SetMapDigOut(char * );		// Set map to decode MTS digital output
unsigned int M_SetMapDigIn(char * );		// Set map to decode MTS digital input
unsigned int M_SetMapVirtualIn(char * );	// Set map to decode MTS digital virtual input

unsigned int T_SetMapDigOut(char * );		// Set map to decode TestKit digital output
unsigned int T_SetMapPUPDFLIn(char * );		// Set map to decode TestKit Pull UP Down Float
unsigned int T_SetMapDigIn(char * );		// Set map to decode TestKit digital input
char* rem_duble_slash(char *stringo,char *stringd) ; //rimove doppio slash

void text_to_hex(char *is_ascii_string, char *os_hex_string);
void string_to_hex(char *buffer);
void hex_to_text(char *is_hex_string, char *os_ascii_string);
int hex_to_ascii(char c, char d);
int hex_to_int(char c);
void replacespace(char *msg);
int64_t stringascii_to_hex(char *is_ascii_string);

#endif