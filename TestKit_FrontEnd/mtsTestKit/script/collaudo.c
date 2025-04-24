
/*
							MACCHINA di COLLAUDO per MTS
							
Rel. 1.00 - 28/05/09 - Adattata da taverniti per 1001 Rel 1.01
Rel. 1.01 - 29/05/09 - Gestione del salvataggio parametri su file
										 - Script suddiviso in funzioni di Test
Rel. 1.02 - 04/06/09 - Ottimizzata procedura di controllo Fonia
Rel. 1.03 - 08/06/09 - Test Fonia: aggiunta gestione di modem senza funzione Loop-Back
Rel. 1.04 - 08/06/09 - Aumentati tempi per Test OUT e Fonia con Loop-Back
Rel. 1.05 - 08/06/09 - Non si controlla più la freq di 1KHz (Tone decoder)
Rel. 1.06 - 09/06/09 - Inserito codice scheda servizi nel file di abbinamento
										 - Inizializzato lo stato dei relè (sia MTS che T-K) prima del test dei Relè
Rel. 1.07b1 - 09/06/09 - Ottimizzazione di Test inDig, outDig e inAnal
											 - Compressione e sintesi dei report di collaudo
Rel. 1.07b2 - 11/06/09 - Inseriti controlli su input di operatore (Codici prodotto e schede)
Rel. 1.07b3 - 12/06/09 - File di abbinamenti creati con cadenza giornaliera
Rel. 1.07b4 - 15/06/09 - Implementata la taratura del Reference Analogico
Rel. 1.08b0 - 16/06/09 - Iniziata la gestione con file di impostazioni
											 - gestita letture delle impostazioni
											 - modularizzato Test Input, Output
Rel. 1.08b1 - 18/06/09 - Iniziata la gestione comune degli MTS
Rel. 1.08b2 - 19/06/09 - Migliorata la presentazione di avanzamento
											 - modularizzato Test CAN, Accesso alle seriali
Rel. 1.08b3 - 27/10/09 - tolto il blocco del Test In Analogici
Rel. 1.09 - 27/10/09 - Aggiunto controllo alimentazione Console
										 - Gestione di Out OD su canali 0, 1, 2, 3, 4, 5, 6 e 7
										 - Impostato il 2102
										 - Aggiunto controllo di input con composizione MTS da ATI
										 - Aggiunto controllo di Famiglia MTS
Rel. 1.09b1 - 28/10/09 - Integrato errore di Alim COM secondaria come bloccante
Rel. 1.09b2 - 28/10/09 - Invertite le impostazioni di T-K su COM1 e COM2 per FTDI (param 24 e 28)
Rel. 1.09b3 - 28/10/09 - Aggiunta l'indicazione dei Failed durante il collaudo
Rel. 1.10   - 29/10/09 - Adeguato Output di 3008 da DOUT5 a DOUT2
										 - Aumento tolleranza Input Analogico a 4%
								RESTA DA AGGIUNGERE
										 - Leggere la versione e s/n di Test-Kit
										 - Gestire la barra di avanzamento
										 - gestire il Led Rosso
- ROVERA -
Rel  1.11 - 05/11/09 - Inizio lavoro 
					 - Inserito define CBUG (non definisco per eliminare scritte di DEBUG)
					 - Aggiunto LaserScanner
					 
Rel  1.15 - 24/11/09 - Loop di 20 sec in SK_Test_GSM in attesa di GMS reg e CSQ>10 

	======================================================================
Rel  2.00 -   /11/09	Tolti riferimenti interni ai modelli (usate solo setting da file)
						Unico programma per Collaudo & programmazione	
Rel  2.01 - 13/10/10	Corretto uso di CurrMax
Rel  2.02 - 20/01/10	Modificata ed utilizzata la ProgressBar ,
						Cancella configurazione CAN ( default QtaCan=0)
Rel  2.03 - 05/01/10	Aggiunti Ingressi Analogici in Tensione
						Test x 4004, MTS40, HC12
						Tensione di TK ridotta di 0.4 Volt
Rel  2.10 - 05/01/10	Versione portata a 2.10 in accordo con VB 2.1.0
Rel  2.11 - 08/01/10	Rimuove anche i TAB di fine riga dalle linee in x_setting.txt
Rel  2.12 - 09/01/10	Prima lettura AD non usata & Vext TK senza +/- 0.2 & Vext  HC12 -=1.2
Rel  2.14 - 16/02/10	Ripristinato laserscanner (Par.97 a 8) se no laser par.97 a 0
Rel  2.15 - 23/03/10	ARM7 - GSM csq timeout fino a 78 sec
Rel  2.16 - 19/05/10	Aggiunto CORTEX
Rel  2.18 - 01/06/10	In produzione imposta default TRACE (par.8) a 2
			03/06/10    Inserito M2020
Rel  2.19 - 04/06/10	Imposta LAN dopo OK utente
Rel  2.20 - 08/06/10	Modificato vbinterface
Rel  2.21 - 06/07/10	Baco sulla presenza del MTS02C 
Rel  2.22 - 13/07/10	Inseriti 2205C, 2206C (nella tabella famiglie)
Rel  2.23 - 04/08/10	Inserito 3108 (Cortex)
			07/09/10 	Inserito 2022 (Cortex - simile al 2102C)
Rel  2.24 - 09/08/10	Utilizzo di multiple schede servizi
			24/09/10	Test del LCD sul 2022 (e Jumper di lock), EnCnt diventa QtaCnt
Rel  2.25 - 01/10/10	Possibile lockare il baud x GSM in produzione (s/n=-1)
Rel  2.26 - 04/11/10	Se nuovo MTS par.8 a 2
Rel  2.27 - 10/11/10	Vext x 2102-EVR
Rel  2.28 - 11/11/10	MTS4004 (s/n=0 -> -1)
Rel  2.29 - 16/12/10	MTS4004 (s/n=0 -> -1)
Rel  2.30 - 14/02/11	Inserito 2020C/2120
Rel  2.31 - 01/03/11	Inserito lettore a barre x s/n schede. File abbinamenti: "<mts>_abbYY_<terzista>.csv" 
Rel  2.32 - 23/05/11	Inserito MTS3208
Rel  2.33 - 06/07/11	Diciture uscite per 3208
Rel  2.34 - 22/07/11	Inserito MTS2202 & EmLink & Ext_LED
Rel  2.35 - 28/07/11	Modificate uscite per 3208 (la 7 e 8 comandano la 2 e 3)

- TAVERNITI -
Rel  2.36 - 03/10/11	Rimossi pre taratura analogico,modificato tempo attesa risveglio mts
Rel  2.37 - 27/10/11	Invertiti PRODUZ e DEBUG in mgsbox
Rel  2.38 - 02/11/11	Migliorato check MODEM
Rel  2.39 - 03/11/11	progressbar programmazione portato a 40sec
Rel  2.41 - 04/11/11	Modificato collaudo Laser Scanner 

- ROVERA -
Rel  2.42 - 02/12/11	Sistemato cancellazione Terzista, aggiunto MB e SUB-MOD file log
- TAVERNITI
Rel  2.43 - 19/01/12	Inserito 2023 e 2305
Rel  2.44 - 20/01/12	Abilita il bk dei dati del MTS (Macchina Stati e parametri)
Rel  2.45 - 26/01/12	Modificato il collaudo per 2023
Rel  2.46 - 01/02/12	Aggiunto controllo S/N lunghezza di 9 caratteri
Rel  2.47 - 21/02/12	Rimosso Terzista Aggiunto hostname al suo posto
Rel  2.48 - 21/02/12	Aggiunto funzione WriteAbbin
Rel  2.49 - 01/03/12	Rimosso Richiesta S/N a prog
Rel  2.50 - 14/03/12	Risolto problemi di Buffer
Rel  2.51 - 23/03/12	Modificato Controllo Batteria
Rel  2.52 - 08/05/12	Aggiunto SM e Par selezionabili da ini
Rel  2.53 - 09/05/12	Ripristinato salvataggio parametri
Rel  2.54 - 25/06/12	Aggiunto salvataggio dati SIM (EnSIMdata)
Rel  2.55 - 25/06/12	Aggiunto random
Rel  2.56 - 26/06/12	Aggiunto while per incrementazione SN
Rel  2.57 - 05/07/12	Aggiunto file temp per abbinamenti
Rel  2.58 - 13/11/12	Aggiunto filecompare
Rel  2.59 - 14/11/12	Aggiunto controllo parametri e spostato assegnazione s/n
Rel  2.60 - 26/11/12	Modificato numero quantita cifre NUM TEL
Rel  2.61 - 10/01/13	Inserito 2106C(New NoCHARGE)
Rel  2.62 - 15/01/13	Modificato chiusura colllaudo parametri e macchina stati
Rel  2.63 - 22/01/13	Aggiunto attesa per blocco Jtag e collaudo DUALSIM
Rel	 2.64 - 24/01/13	Aggiunto attesa reboot macchina stati
Rel  2.65 - 26/02/13	Aggiunto parametro 75 per 2023 nuovi
Rel  2.66 - 17/05/13	Modificato riconoscimento dual-sim se VER 2.10 richiesta CDHW
Rel  2.67 - 21/05/13	Sistemato riconoscimento dual-sim se VER 2.10 richiesta CDHW
Rel	 2.68 - 23/07/13	Aggiunto gestione parametro 75 per tutti 2023 
Rel	 2.69 - 24/07/13	Aggiunto gestione parametro 75 per tutti 2022 2122 
Rel  3.00 - 06/09/13	Nuova Versione Testkit
Rel  3.01 - 15/10/13	Ridotti alcuni tempi e modificato test VConsole
Rel  3.02 - 22/10/13	Aggiunto invio Configurazione CAN
Rel  3.03 - 28/10/13 	Cambiato test Fonia
Rel  3.04 - 28/10/13 	Cambiato test Ingressi
Rel  3.05 - 29/10/13	Aggiunto Attesa Fix, Tolto Riconoscimento 1Khz,Rafforzato BFON su 3025
Rel  3.06 - 30/10/13	Aggiunto Funzione Ripeti a GPS E GSM
Rel  3.07 - 31/10/13	Modicato Funzione Ripeti a GPS E GSM e Tamper errore
Rel  3.08 - 04/11/13	Modificato Test ingressi
Rel  3.09 - 14/11/13 	Modificato Controllo Parametri
Rel  3.10 - 18/11/13	Aggiunto Possibilita Percentuale Analogico
Rel  3.11 - 27/11/13	Aggiunto Delay Invio configuarazione Can
Rel  3.12 - 28/11/13	Modificato ripeti gps e gsm
Rel  3.14 - 11/12/13	Swappate uscite 2-3 con 7-8 sul 3025 come su 3208
Rel	 3.15 - 18/12/13	Modificato EmergencyLink per 3025
Rel	 3.16 - 27/01/14    Aggiunto ripeti per EmergencyLink e CAN
Rel  3.17 - 28/01/14	Modificato check parametri
Rel  3.18 - 14/04/15	Aggiunto collaudo 2034
Rel  3.19 - 13/05/15	Modificato Test Ingressi
Rel  3.20 - 01/07/15	Aggiunto Possibilità disabilitazione Test Tamper
Rel  3.21 - 23/07/15	Aggiunto Pre-collaudo 3033
Rel  3.22 - 08/09/15	Aumentato Tempi Collaudo dual-sim
Rel  3.23 - 21/12/15	Aggiunto Collaudo 3035-3036
Rel  3.24 - 07/01/16	Aggiornato Collaudo 3035-3036
Rel  3.25 - 08/01/16	Aggiunto cansel
Rel  3.26 - 11/01/16	Modificato Collaudo 3035-3036
Rel  3.27 - 12/01/16  Modificato Collaudo RTC
Rel  3.28 - 21/03/16  Aggiunto Collaudo 4037
Rel  3.29 - 13/04/17	Aggiunto Collaudo 2039
Rel  3.30 - 24/05/17	Aggiunto Collaudo 2040
Rel  3.31 - 14/11/17	Aggiunto Collaudo 3036 v02
Rel  3.32 - 23/01/18	Modificato Collaudo HTL
Rel  3.33 - 24/01/18	Modificato Collaudo Counter
Rel  3.34 - 25/01/18	Dimmezzato Tempi CAN
Rel  3.35 - 20/02/18	Aumentato tempo com secondaria e introdotto diverse ripetizione test
Rel  3.36 - 23/05/18	Aggiornato gestione WSPACE
Rel  3.37 - 20/06/18	Aggiunto Collaudo 2046 + Gestione Porta USB MTS
Rel  3.38 - 10/07/18	Cambio Gestione Diag 12
Rel  3.39 - 27/09/18	Aggiunto Collaudo 2044
Rel  3.40 - 28/09/18	Aggiunto Collaudo CANLOGISTIC
Rel  3.41 - 10/01/19	Modificato controllo esecuzione Collaudo CANLOGISTIC
Rel  3.42 - 17/01/19	Modificato controllo esecuzione Collaudo CANLOGISTIC
Rel  3.43 - 21/01/19	Modificato collaudo CAN
Rel  3.44 - 22/01/19	Aggiunto collaudo 3048 
Rel  3.45 - 24/01/19	Riordinato dimensioni buffer e modificato collaudo Uscita 
Rel  3.46 - 05/02/19	Aggiunto collaudo 2047
Rel  3.47 - 19/02/19	Regolato buffer
Rel  3.48 - 04/03/19  Aggiunto Cancellazione Parametri
Rel  3.49 - 01/04/19  Aggiunto Cancellazione Parametri
Rel  3.50 - 12/04/19  Aggiunto/Modificato Invio Codice su Seriale 
Rel  3.51 - 15/04/19  Aggiunto Funziona caricamento Macchina Stati e Can 
Rel  3.52 - 16/04/19  Verificato e migliorato Funziona caricamento Macchina Stati e Can
Rel  3.53 - 23/04/19  Aggiunto Collaudo 2405 
Rel  3.54 - 20/05/19  Modificato controllo Blocco Jtag Diag3
Rel  3.55 - 19/06/19  Aggiunto controllo invio conf can e par e sm 
Rel  3.56	- 10/07/19	New version 2047 TTL SERIAL
Rel  3.57	- 03/09/19	Update GetIniInfo for SERIAL Programing
Rel  3.58	- 17/02/20	Update Param 105 for HTL
Rel  3.59 - 22/06/20	Aggiunto collaudo 2051
Rel  3.60 - 07/07/20	Aggiunto Debug Invio Configurazione CAN
Rel  3.61 - 22/07/20	Aggiunto Collaudo 2052
Rel  3.62 - 14/10/20  Aggiunto Gestione Parametri_Sim_Nome_Esterno
Rel	 3.63 - 16/12/20  Aggiunto paremetro 188 ha tablePIN
Rel	 3.64 - 02/07/21  Aggiunto Collaudo 2046_M4 Cortex LPC4078
Rel	 3.65 - 05/07/21  Modificato Collaudo Cambio Assetto Ribaltamento
Rel	 3.66 - 06/07/21  Aumentato Tempo CanBus
Rel	 3.67 - 13/07/21  Modificato indirizzo canBus in f004 e cambiato baudrate
Rel  3.68 - 15/07/21  Aggiunto parametro maschera Ingressi MaskDigInDn e MaskDigInUp
Rel  3.69 - 16/07/21  Aggiunto localmillisDelay e modificato timeoutCAN
Rel  3.70 - 19/07/21  Modificato Lettura Parametri File INI
Rel	 3.71 - 19/07/21  Aggiunto Collaudo 2044_M4 2047_M4 2051_M4 Cortex LPC4078
Rel	 3.72 - 21/09/21  Aggiunto Rimozione Caratteri non alfanumerici quando scrive i parametri.
Rel  3.73 - 16/11/21  Aggiunto Ribatezzamento Codice
- ROVERA -
Rel  3.75 - 17/05/23  Aggiunto Rimozione Caratteri non alfanumerici in lettura file ASCII
Rel  3.76 - 26/05/23  Aggiunta gestione (e divulgazione) valore flag di debug 
					 (chiave "leveldebug=" nel file di collaudo del tipo MTS)
Rel  3.77 - 03/03/25  Aggiunti 2054 e 3055
*/ 

//Delay in decimi di secondo 

#define VER 3
#define SUBVER 77
#define VERDATE "03/03/25"

// Funzioni di visualizzazione per "Collaudo.c"
// MsgWindow -> (definita in vbInterface.c)
//				visualizza su MtsTestKit finestra messaggi (trace) tramite sequence.c che chiama Add_txt_answer() in MtsTestKit.c
//				e stampa su terminal (directToConsole)

// non esiste funzione richiamabile da "Collaudo" per scrivere su MtsTestKit finestra dati MTS
//	ci scrive solo "mts_tk_man.c" ( chiamare funzione Add_txt_mts() in MtsTestKit.c )

// MsgFile	 -> scrive su file di log

//Variabili Globali RTC
int deltak=0;
int deltat=-500000; //impostato per dare errore;
///////////// definizioni variabili testkit //////////////
long long CHN;
long long AN_DIG_;
long long AN_CORR_;
//long long  A_MASSA_;
long long EN_CNT;
long long CAN_EN;
long long PON_ ;
long long TOD1;
long long TRL2_;
long long TOD5 ;
long long TOD4 ;
long long TK_MASKOUT;
//long long TIN_PUP;
long long PRES_;
//long long AL_OFF;
long long CPU_OFF;
long long TK_STARTMASK;
long long TK_STARTVAL;
long long SH_C;
long long PR_RD_;
long long OK_CNS_;
long long UNK_OK_;
long long EMLNK_IN2x;
long long EMLNK_IN3x;
//Analogici;
int VREF;
int IREF;
int TAN2;
int TAN3;
int TAN4;
int TAN5;
int TVEXT;
int PWR_C;
int FONIA;
//Servizio
int SETAN;
int TO_CURR;
int TO_VOLT;
//fine Analog In
///////////// definizioni variabili testkit //////////////
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>
#include <sys/timeb.h>

#ifdef FR_WIN32
#include <windows.h>
#include <sys/stat.h>
#include <winsock.h>

#include <sys\stat.h>
#define CONVERTPATH(_AA)

#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

//       #include <sys/types.h>
//       #include <unistd.h>


#define CONVERTPATH(_AA)	{ char *_p ;						\
			while ((_p = strchr(_AA,'\\'))!=NULL) *_p = '/'; 	\
			/*rem_duble_slash(_AA,_AA);*/						\
		}											




#include "vbInterface.h"
#include "testkitcomune.h"
#include "testkitold.h"
#include "testkitnew.h"

//#define DEBUG_P71 // Se valido P71=1 se remmato P71=0
#define DEBUG_P71
//#define DEBUG_FR
//#define DEBUG_1FR
#ifdef DEBUG_1FR
char Repeat=1 ;
#endif

#define LOCALTIME_R(_clock,_result)  (*(_result)=*localtime(_clock))

//#define CHECK_TESTTIME
#undef CHECK_TESTTIME

#ifdef CHECK_TESTTIME
unsigned int MSGBOXCALL(char * text, char * label, int ntasti, char * caption1, char * caption2, char * buf);
unsigned int INPUTBOXCALL(char * text, char * label, int ntasti, char * caption1, char * caption2, char * buf);
void LABEL_STEP(int _N, char * _T);
void COLOR_STEP(int _N, int _C);

#else
#define MSGBOXCALL(_A, _B, _C, _D, _E, _F) MsgBox(_A, _B, _C, _D, _E, _F)
#define INPUTBOXCALL(_A, _B, _C, _D, _E, _F) InputBox(_A, _B, _C, _D, _E, _F)
#define LABEL_STEP(_N, _T) 	OuputText((_N+100), _T, 0, 0, 0, 0 )
#define COLOR_STEP(_N, _C) 	OuputText((_N+100), "", _C, 0, 0, 0 )
#endif

//#define PrDeb 		1				// 1 = abilita attività di debug su VB  -   0 (o altro) disabilita

#define FREQ 	 1100				// Frequenza del Testkit (0609 era 1078)
#define TICK 	   95				// durata dell'unità di Delay in ms  (0609 era 100)
#define LEN_ARCA 	7				// Lunghezza dei codici ARCA
#define LEN_CDHW 	9				// Lunghezza dei CoDici HardWare
#define LEN_IDHW 	4				// Lunghezza degli Identificativi HardWare (nome MTS, n.Board e n.S.Servizi)
#define LEN_NUMHW 	5				// Lunghezza dei Serial Number (parte numerica)
#define LEN_NUM		9				// Lunghezza dei Serial Number
#define LEN_ICCID 	19				// Lunghezza ICCID
#define LEN_NUMTELMAX 	10			// Lunghezza NUMTEL MAX  
#define LEN_NUMTELMIN 	9			// Lunghezza NUMTEL MIN 

//--------------------------------------------------------------------------------------------------------------

//#define PATH_FW "..\\\\..\\\\"		// Percorso da path del programma al FW
//--------------------------------------------------------------------------------------------------------------
unsigned int TKTYPE; // Tipo di TestKit se 0 vecchio se 1 nuovo con SN maggiore di 901400100
char PathTK[NRMSIZE] ; 			// Path per gli Applicativi del TestKit (Wspace[]+Applicativi)
uint32_t leveldebug ;		// Added from 2.13 (25/05/23) same name as Gdata into MTScu

_InternalAnalogData DatoMTS;
_HwVers RevHW;

void WriteAbbin(char * buf);
void WriteSIMDATA(char * buf);
int FileCompare(char * file1, char * file2, unsigned int * same);

// Struttura con definiti i codici famiglia per ogni MTS
struct {
	char mts[MINSIZE] ;		// nome mts
	int  famcode ;			// codice famiglia mts
} MtsFam[100] ;

int QtaFam;					// Massimo n° di item = 100

// Struttura con i codici della tabella di ARCA
struct {
	short nr_aSV ;
	char cod[MINSIZE] ;		// Codice ARCA dell'MTS
	char mts[MINSIZE] ;		// nome mts
	char board[MINSIZE] ;   // codice della Motherboard
	char srvcard[5][MINSIZE] ; // codice della scheda servizi
} Arca[100] ;
int QtaArca = 0;

// Struttura con i codici della tabella PIN
struct {
	char snmts[MINSIZE] ;		// snMTS
	char extname[MINSIZE] ;		// ext mts
	char ICCID[MINSIZE] ;   // ICCID
	char PIN[MINSIZE] ; // PIN
	char centoottantacinquepar[MINSIZE*4]; //185 Parametro APN
} TablePIN[10000] ;
int QtaRow = 0;

// Struttura con i dati digitati di abbinamento: codice MTS, codice Motherboard (e s/n), codice scheda servizi (e s/n)
// Solo per produzione
struct {
	char codTEST[MINSIZE] ; 	// Codice ARCA dell'MTS
	char codMB[MINSIZE] ;		// codice della Motherboard completo di S/N
	char codSV[5][MINSIZE] ;	// codici delle schede servizi completi di S/N
	char codICCID[MINSIZE] ;	// codici ICCID SIM
	char numTEL[MINSIZE] ;	    // NUMERO TELEFONO SIM
} HwSN ;

// Struttura con le impostazioni del file mtsTestKit.ini della Classe Dispositivo scelto
struct {
	char NamTyp[MAXSIZE] ;		// workingPath
	char mName[MINSIZE] ;		// MtsName
	char FileImp[NRMSIZE] ;		// FileImpostazioni (di collaudo)
	char NamFile[MINSIZE] ; 	// prgFileRadix
	char NewNum[MINSIZE] ; 		// next S/N
	char NamFW[MAXSIZE] ;		// ProgramFile (non necessario)
	char NamSM[MAXSIZE] ;		// StateMachine
	char NamPar[MAXSIZE] ;		// Parameter File
	char NamCANConf[MAXSIZE]; 	// CanConfigure
	char Protocol[MAXSIZE];
} TkIni ;

// Struttura con definite le impostazioni del collaudo
struct {
// Stringhe
	char  Terzista[NRMSIZE] ;	// S Nome del costruttore (per inserirlo nel report di abbinamento)
	char  Cpu[NRMSIZE] ;		// S Identificativo della CPU (HC12,ARM7,ARM9,ATMEGA)
	char  CnetIP[NRMSIZE] ;		// S Local IP of MTS
// Interi
	int   BootTime ;			// I Tempo di attesa per riavvio dell'MTS in decimi di secondo
	int   SisCOM ;				// I Porta di servizio (console) ('1' = LU11; 'altro' = LU2)
	int   QtaOD ;				// I Numero di uscite OpenDrain
	int   QtaRL ;				// I Numero di uscite Rele'
	int   QtaIn ;				// I Numero di ingressi digitali
	int   MaskDigInDn ; // I Mask per Ingressi digitali con PullDown
	int   MaskDigInUp ; // I Mask per Ingressi digitali con PullUp
	int   QtaInA ;				// I Numero di ingressi Analogici in CORRENTE
	int   QtaInV ;				// I Numero di ingressi Analogici in TENSIONE
	int   QtaCAN ;				// I Numero di porte CAN
	int   CntToller ;			// I Delta in ms accettattati nella lettura del contatore ~1KHz in 2 sec
	int   SogliaF ;				// I Valore minimo di variazione di livello Fonia tra 'Fonia spenta' e 'Fonia di 1KHz'
	int   QtaLaserS ;			// I Abilita il test del Laser Scanner (max 9 stringhe)
	int   NetIP ;				// I Local IP of MTS
	int   QtaUSB ;				// I Nr. di porte USB
	int   QtaCnt ;				// I Abilita il test degli ingressi Counter
	int   GSMbaud ;				// I se>0 indica il baud x GSM in produzione
// Logici
	int   AnlTaratura ;			// L Permette di memorizzare il coefficiente di calibrazione del convertitore AD
	int   EnTrm ;				// L Abilita la COM della porta di sevizio
	int   EnPres ;				// L Nel collaudo si prevede la gestione della presenza
	int   EnRTC ;				// L Abilita il test del RTC
	int   EnDigInDn ;			// L Abilita il test degli Input Digitali con Pull-Down (o su TRIS tastierino)
	int   EnDigInUp ;			// L Abilita il test degli Input Digitali con Pull-Up (o su TRIS tastierino)
	int   EnDigOut ;			// L Abilita il test degli Output Digitali
	int	  EnTamper ;			// L Abilita il test del Tamper
	int   EnAnlIn ;				// L Abilita il test degli Input Analogici
	int   EnVext ;				// L Abilita il test della tensione esterna
	int   EnVbat ;				// L Abilita il test della tensione Batteria
//	int   EnCnt ;				// L Abilita il test degli ingressi Counter sostituito da QtaCnt
	int   EnGPS ;				// L Abilita il test del GPS
	int   EnGSM ;				// L Abilita il test del GSM
	int   EnFonia ;				// L Abilita il test della Fonia GSM
	int   EnCOM2 ;				// L Abilita il test della COM secondaria
	int   EnCANLOGISTIC;			// L Abilita il test del CANLOGISTIC
	int   EnVcns ;				// L Abilita il test della Tensione a Console
	int   EnRS485 ;				// L Abilita il test della porta 485
	int   EnHTL ;				// L Abilita il test della porta HTL
	int   EnCOMAUX ;		    // L Abilita il test della porta COM Ausiliare
	int   EnAccel ;				// L Abilita il test dell'Accelerometro
	int   EnLCD	  ; 			// L Abilita il test del display LCD (su 2022)
	int   EnExtLED ; 			// L Abilita il test del LED esterno (su 2202)
	int   EnEmeLink ; 			// L Abilita il test dell'emergency link (su 3208 e 2202)
	int   EnBKdata ; 			// L Abilita il bk dei dati del MTS (Macchina Stati e parametri)
	int   EnSendFW;				// L Abilita caricamento FW
	int	  ASKFW;					// L Abilita domanda tipo FW
	int   EnSMset ; 			// L Abilita caricamento Macchina Stati e parametri impostati nel ini
	int   EnCANConf ;           // L Abilita caricamento Configurazione CAN impostato nel file ini
	int   EnSIMdata ;           // L Abilita salvataggio dati SIM 
	int   EnChBat ;				// L Abilita Test Carica Batteria
	int   EnVibro ;				// L Abilita Test Vibrazione
	int	  EnNoAnt ;				// L Abilita Test Distacco Antenna GPS
// Virgola mobile
	float VextMin ;				// F Valore minimo di tensione accettata in ingresso
	float VextMax ;				// F Valore Massimo di tensione accettata in ingresso
	float CurrMin ;				// F Valore soglia di Minima Corrente in ingresso prevista
	float CurrMax ;				// F Valore soglia di Massima Corrente in ingresso prevista
	int   An_Perc ;             // Percentuale Analogico 
	int   CAN_Term_Perc;        // Percentuale convalida
	int EnPIN_Param;			// L Abilita gestione File PIN
	int EnFormat;					// L Abilita formattazione con SN -1
	int EnBaptismo;				// L Abilita Assegnazione SN
  char  StartProtocol[NRMSIZE] ;		// S Protocollo iniziale di comunicazione
} TestSet ;

// Variabile x LaserScanner
struct _LASER{
	char wait_mgs[MAXSIZE] ;
	char recv ;
} LS_barcode[10] ;

//// Dati ricevuti da MTS
struct {
	int mSerial ;
	int mSign ;					// Famiglia MTS fornita da MTS (da verificare con mTipo)
	int ERRTest ;				// Errori totali durante il collaudo
	int AlimFail ;				// Risultato del test (inizio e metà collaudo) della Vconsole
	int oldtime ;				// Data/ora dell'MTS prima di inviare SM di collaudo
	unsigned int TKcntStart ; 	// Old TK counter value
	unsigned int SMflags ;		// (Ronly)
	int ValoreCSQ ;
	char Imei[NRMSIZE] ; 
	char GsmModel[NRMSIZE] ;
	char GsmRev[NRMSIZE] ;	
	float VbatMTS ;	
	float SVer ;
	unsigned int HMainVer;
	unsigned int HSrvVer;
} MTSdata ;

// Variabili di collaudo con valori dipendenti dal tipo di MTS
struct {
	int nuovo ;					// Se s/n = -1
	int firstRun ;				// Se senza codice
	int steptest ;
	int presOFF ;				//
	int presON ;				// 
	int pres2OFF ;				//
	int pres2ON ;				//
	int mTipo ;					// Famiglia di MTS attesa (da tabella MtsFam[]) 
	int ArcaPos ; 				// Indice in dati Arca
	int PinPos ; 				// Indice in dati PIN
	int WTastiera ;				// MTS con tastiera (e non ingressi esterni)
	int MonoPiastra ;			// MTS senza scheda servizi
	int mCOM ;					// Unità logica con cui la 'COM1' del TK dialoga
	int tCOM ;					// Unità logica con cui la 'COM2' del TK dialoga
	time_t oldtime ;			// Data/ora del PC prima di inviare SM di collaudo

	float VextTK ;
	// Dipendenti dal tipo di batteria (se left(TkIni.mName,3) = "MTS" -> Ni-MH
	float VbatMIN ;				// Li=3.5 / Ni-MH=8.0
	float VbatMAX ;				// Li=4.3 / Ni-MH=10.5 
	char COM1[MINSIZE] ;		// Setting del TK per abilitare la 'COM1'
	char COM2[MINSIZE] ;		// Setting del TK per abilitare la 'COM2'
	char mFwVer[MINSIZE] ;		// Versione di FW più recente, presente in mVers[], per l'MTS in uso
} MtsTK ;

#define DELTA_AN_I   	0.5			// +/- DELTA_AN: variazione in mA ammessa nella lettura degli input analogici liberi
#define DELTA_AN_V   	2.8			// +/- DELTA_AN: variazione in V ammessa nella lettura degli input analogici liberi
#define DELTA_AN_VHC12  2.8			// +/- DELTA_AN: per HC12
//#define AN_DELTAPERC    4			// +/- DeltaAn: variazione in % ammessa nella lettura degli input analogici
#define MIN_CSQ		 	8			// Minimo valore di CSQ di accettazione
#define COM_OFF     	"10"		// Settaggio FTDI per disattivare sia COM1 che COM2

// Define per la funzione IDCMD_DIAG
#define DIAG_LU      0					// Tipo Stato LU
#define DIAG_AUDIO   1					// Tipo Audio-Loop
#define DIAG_EMLNK   7					// Tipo Emergency Link
#define DIAG_GSM	 4					// Tipo Dati GSM
#define DIAG_ENABLE  1					// Abilita (x AudioLoop)
#define DIAG_DISABLE 0					// Dissbilita (x AudioLoop)
#define DIAG_LU9     0					// Diagnost. LU9
#define DIAG_GSMERR  1					// Diagnost. Errori di GSM (CMS Error se 8xxx)
#define DIAG_LU5     2					// Diagnost. LU5
#define DIAG_LU13    3					// Diagnost. LU13
#define DIAG_LU14    4					// Diagnost. LU14
// #define QUERYANSWER 0

#define	NUMCHAR  "1234567890"			// Usato per verificare se stringa solo numerica

	
//*********************************************
// Variabili di "Macchina"
	int NamOut[18]; 					// Ordine di sequenza di Uscite O.D. e Rele'	
	int tbar_time ; 					// Tempo da inpostare per 
	
// inizio e fine collaudo
	time_t TimeDUT, TimeNOW;
//*********************************************

// Nomi delle path
	char WSpace[MAXSIZE] ; 			// Path di lavoro
	//char PathTK[MAXSIZE] ; 			// Path per gli Applicativi del TestKit (Wspace[]+Applicativi)

	char PathFwUp[MAXSIZE] ;		// Path per gli Applicativi di invio FW per la CPU dell'MTS (Wspace[]+Applicativi+'cpu')
	
	char mRoot[MAXSIZE] ;			// Path per i file dell'MTS in collaudo (Wspace[]+'workingPath')
	char mAppl[MAXSIZE] ; 			// Path per gli Applicativi dell'MTS in collaudo (mRoot[]+'Applicativi')
	char mVer[MAXSIZE];				// Path con i file di FW dell'MTS in collaudo (mRoot[]+'Versioni')

	
// Nomi dei file con path nella dir dell'MTS in oggetto
	//char LogFilecum[MAXSIZE] ;			// "LogCheck.txt"
	char LogCollaudo[MAXSIZE] ;		// "Log(<s/n>).txt"
	
// Buffer temporaneo di log (prima di creare nome del log file)
	char LogBuffer[256][MAXSIZE] ; 
	int QtaLog = 0;

// File di SM e parametri (con Path per gli Applicativi del TestKit)
	char SMachVoid[MAXSIZE] ; 		// SM vuota
	char SMachColl[MAXSIZE] ;		// SM per il collaudo
	char SMachTest[MAXSIZE] ; 		// SM da inviare a fine collaudo se MTS di produzione
	char SMachFile[MAXSIZE] ;		// SM presente prima del collaudo (per ripristinarla)
	char ParamFile[MAXSIZE] ;		// File con i parametri presenti prima del collaudo (per ripristinarli)
	char FWSel[MAXSIZE] ;				// FW scelto	
	char SMachSel[MAXSIZE] ;		// SM scelta
	char ParamSel[MAXSIZE] ;		// PM scelti
	char CANConfSel[MAXSIZE*2] ;		// CAN Conf scelta
	
// Stringhe scratch
char buflog[MAXSIZE*4];
char bufabb[MAXSIZE];
char bufSIM[MAXSIZE];
char bufmsg[MAXSIZE];
char bufwindow[MAXSIZE];
char bufresponse[MAXSIZE];
char nextip[MAXSIZE];
char Bmom[MAXSIZE];
char MyDebB[MAXSIZE];

//da togliere
char MexAll[MAXSIZE];
char EndMex[MAXSIZE];
char dDg[MAXSIZE];

char separ[MINSIZE];


// PARAMETRI MODIFICATI NEL COLLAUDO
int mtspars[] = { 65, 69, 70, 71, 76, 77, 79, 81, 96, 97, 228, 100 } ;
int nr_mtspars ;
// ----------------  Dichiarazioni  --------------------------------------------
#ifdef CBUG
void StampaDB(char * nome, char * testo); // Scrive su finestra DOS di Debug
void StampaDBn(char * nome, int dato);
#else
#define StampaDB(_A, _B)  
#define StampaDBn(_A, _B) 
#endif // #ifdef CBUG

void call_exit(int error, char * cc);
int  SK_GetIniInfo(void);
int  SK_TK_CheckVersion(void);
int  SK_TK_Init(void);
int  SK_TK_PreInit(void);
void GetDate(char *data);
void GetYear(char *data);
void LoadFamTab(void);
void LoadTabPINAssociative(void);
int  ReadMTSType(char *dutM);
int ReadMTSArca(char *dutM);
int ReadMTSPIN(int dutM);
void TesterMTS(void);

char * loc_fgets(char * a1, int nn, FILE * abc);
void RemoveComment(char *testo);
int  GetIntStr(char *Delim, char *sInput,  unsigned int order, char *sOut);
void UpperAlfaNum(char *testo);
void RLTrimm(char *testo);
void RLTrimmwithplace(char *testo);
void LoadProdTab(void);

char hostname[128];

int SK_ParamSet(void);
int SK_ParamSetCheck(void);
int SK_SM_StateSet(void);
int SK_SM_StateSetCheck(void);
int SK_CanConfSet(void);
void togliCR(char *msg);
void LoggaStampa(char *msg);

#ifdef SENDFW
int64_t SK_SendCode(void);
#else

float ReadAnalog(int channelId);

void LoggaStampaCR(char *msg);

int SK_PowerOn(void);
int SK_CheckId(void);
int SK_Set_MTS(void);
int SK_SaveParam(void);
int SK_RestoreParam(void);
/*
int SK_ParamSet(void);
int SK_ParamSetCheck(void);
int SK_SM_StateSet(void);
int SK_SM_StateSetCheck(void);
int SK_CanConfSet(void);
*/
int SK_Test_RTC(void);
int SK_Test_DigIn(void);
int SK_Test_DigOut(void);
int SK_Test_AnlIn(void);
int SK_Test_Vext(void);
int SK_Test_Vbat(void);
int SK_Test_Cnt(void);
int SK_Test_GPS(void);
int SK_Test_GSM(void);
int SK_Test_CAN(void);
int SK_Test_COM2(void);
int SK_Test_CANLOGISTIC(void);
int SK_Test_RS485(void);
int SK_Test_HTL(void);
int SK_Test_COMAUX(void);
int SK_Test_ChBat(void);
int SK_Test_Vibro(void);
int SK_Test_Accel(void);
int SK_Test_LCD(void);
int SK_EndChk(void);
int SK_TaraCurr(void);
int SK_TestLaserS(void);
int SK_TestLAN(void);
int SK_TestUSB(void);
int SK_TestEmeLink(void);
#endif

char SCRREL[MAXSIZE] ;

char* LastcharDel(char* name)
{
    int i = 0;
    while(name[i] != '\0')
    {
        i++;
         
    }
    name[i-1] = '\0';
    return name;
}

int countBits(unsigned char byte){
    int count = 0;
    for(int i = 0; i < 8; i++)
        count += (byte >> i) & 0x01; // Shift bit[i] to the first position, and mask off the remaining bits.
    return count;
}

void localmillisDelay(int timeout){
	struct timeb loc_time ;
	ftime(&loc_time) ;
	//long starttime=(loc_time.time*1000+loc_time.millitm);
	
	while (((loc_time.time*1000+loc_time.millitm)-timeout)<timeout){
		ftime(&loc_time) ;
	}
}


/* function prototypes */
int ascii (const unsigned char c);
 
int ascii_ext (const unsigned char c);
 
unsigned char* strip(unsigned char* str, const size_t n, int ext );
 
 
/* check a character 
   return 1 for true
          0 for false
*/ 
int ascii (const unsigned char c) 
{  
  unsigned char min = 32;   /* <space> */
  unsigned char max = 126;  /* ~ tilde */
 
  if ( c>=min && c<=max ) return 1;
 
  return 0;
} 
 
 
/* check if extended character 
   return 1 for true
          0 for false
*/ 
int ascii_ext (const unsigned char c) 
{  
  unsigned char min_ext = 128;   
  unsigned char max_ext = 255;
 
  if ( c>=min_ext && c<=max_ext )
       return 1;
 
  return 0;
} 
 
 
/* fill buffer with only ASCII valid characters
   then rewrite string from buffer
   limit to n < MAX chars
*/
 #define MAXBUF 256  /* limit */
unsigned char* strip( unsigned char* str, const size_t n, int ext) 
{ 
 
  unsigned char buffer[MAXBUF] = {'\0'};
 
  size_t i = 0;  // source index
  size_t j = 0;  // dest   index
 
  size_t max = (n<MAXBUF)? n : MAXBUF -1;  // limit size
 
  memset(buffer, 0, MAXBUF) ;		// FR 3.75 - 17/05/23: added
 
  while (i < max )
    {
      if ( (ext && ascii_ext(str[i]) ) ||  (ascii(str[i]) ) )    // check
	{
	  buffer[j++] = str[i]; // assign
	}      
      i++;
    }
 
  memset(str, '\0', max); // wipe string 
 
  i = 0;               // reset count
 
  while( i < j)
    {
      str[i] = buffer[i]; // copy back
      i++;
    }
 
  str[j] = '\0';  // terminate properly
 
  return str;  
}

enum {ASCII=0, EXT=1}; /* enumeration makes easier reading */

int main(void)
{
//	==================================================================================
// 								Versione dello Script di collaudo
//	==================================================================================
	sprintf (SCRREL , "Script %d.%02d - %s", VER, SUBVER, VERDATE ) ; 
//	==================================================================================
//	==================================================================================
	int i;
	int VrtSt=0;
	VrtSt=VrtSt;   // Per Compilatore Set but not used
	gethostname(hostname, sizeof hostname);
	strcpy(nextip,"next_");
	strcat(nextip,hostname);
		
	// Inizializza la ProgressBar a 0
	ProgressBar(BAR_PERC, 0) ;
	LogCollaudo[0] = '\0' ;
	
	nr_mtspars = sizeof(mtspars) / sizeof(int) ;

 	GetWorkSpace(WSpace);
	if (WSpace[strlen(WSpace)-1]=='/') LastcharDel(WSpace); 
 	// Path per gli Applicativi
	sprintf(PathTK, "%s\\Applicativi", WSpace);
	CONVERTPATH(PathTK);
	
#ifdef CHECK_TESTTIME
	strcpy(WSpace,"--------------------------------------------------") ;
	MsgFile(0, "tempi_collaudo.txt", WSpace) ;
#endif

	// MTS in test
	GetINIKeyVal("MtsName", TkIni.mName);
	togliCR(TkIni.mName);
// ==================== Verifica Versione TK =====================
	SK_TK_CheckVersion();
//	======================================================================
	/*//DEBUG
	sprintf(Bmom,"PASSO1 TKTYPE=%d\n", TKTYPE);
	PrintDB(Bmom);
	*///DEBUG
	//call_exit(YES, "FineTest");    ///GTGTGTGTGT/// Da Togliere
// ==================== Definizione delle USCITE del TK =====================
// Porte di T-K (Test-Kit):
//	======================================================================
	
	if (TKTYPE==0){
		//     uscite: 	a23b09b01b03a30a31b08b05d05b29d07d06c00c01c02c03d00d01d02d03b02c07   
		T_SetMapDigOut("023109101103030031108105305129307306200201202203300301302303102207");
	}else{
		//     uscite: 	a23b09b01b03b06b07b08b05d00d01d02d03d04d05d06d07e00e01e02e03e04e05e06e07e08e09e10e11e12e13e14e15b02b04b13a12a22
		T_SetMapDigOut("023109101103106107108105300301302303304305306307400401402403404405406407408409410411412413414415102104113012022");  
		//   PUPDFL:	  L 00L 01L 02L 03L 04L 05L 06L 07L 08L 09L 10L 11L 12L 13L 14L 15
		T_SetMapPUPDFLIn("900901902903904905906907908909910911912913914915"); 
	}
	/*//DEBUG
	sprintf(Bmom,"PASSO2 TKTYPE=%d\n", TKTYPE);
	PrintDB(Bmom);
	*///DEBUG
	MtsTK.steptest = 0 ;				// Inizializza contatore dei test eseguiti
	i = 0 ;	
	VrtSt = 0 ;

	SK_GetIniInfo();					// Carica i riferimenti dal file TestKit.INI
	
	//_FR - Rel 3.76 - 26/05/23
	SetLevelDebug(leveldebug) ;
	
#ifdef SENDFW
	// Inizializza scritte degli step
	LABEL_STEP(i++, "Connessione MTS") ;
	if ( TestSet.EnSendFW == YES ) LABEL_STEP(i++, "Invio codice") ;
	if ( TestSet.EnCANConf == YES) LABEL_STEP(i++, "Configurazione CAN") ;
	if ( TestSet.EnSMset == YES) {
		LABEL_STEP(i++, "Parametri") ;
		LABEL_STEP(i++, "Macchina Stati") ;
	}

	LABEL_STEP(i++, "Verifica versione") ;	
	tbar_time = 100 ; // 10 sec
#else
	// Inizializza scritte degli step
	LABEL_STEP(i++, "Controllo MTS") ;
	LABEL_STEP(i++, "Preparazione MTS") ;
	LABEL_STEP(i++, "-- INIZIO COLLAUDO --") ;
	tbar_time = 400 ; // 30 sec
#endif
	
#ifdef SENDFW
	int k,r;
	char start_number[4];
	strncpy(start_number,TkIni.mName,4);
	SK_TK_PreInit();
	T_Led(MSK_PRES, FRQ_PRES);				// Il LED lampeggia per indicare "Presenza abilitata"
	sprintf(bufwindow,"\r---->   VERSIONE di PROGRAMMAZIONE %s: %s   <----\r", TkIni.mName, SCRREL);
	MsgWindow(bufwindow);
	if (!strncmp(TestSet.StartProtocol,"WAY",3)) SetProtocolComunication(0);	// Setta Protocollo WAY
	if (!strncmp(TestSet.StartProtocol,"TEST",4)) SetProtocolComunication(1);	// Setta Protocollo TEST
	Delay(10);
	T_Output (PON_, 0);
	Delay(10);
	T_Output (PON_, 0);
	Delay(10);
	T_Output (PON_, 0);
	Delay(10);
	T_Output (PRES_, 0);											// Si accende l'MTS ... attivando la Presenza ...
	Delay(10);
	T_Output (PRES_, 0);											// Si accende l'MTS ... attivando la Presenza ...
	Delay(10);
	T_Output (PRES_, 0);											// Si accende l'MTS ... attivando la Presenza ...
	Delay(10);
	if (TestSet.EnFormat){
		sprintf(bufwindow,"\rCANCELLAZIONE SERIAL NUMBER\r");
		MsgWindow(bufwindow);
		if (T_SetFTDI(MtsTK.COM1)){							// si abilita la COM1 del T-K
			sprintf(Bmom, "Errore durante aperturta COM MTS");
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
		}
		M_SetSourceId(MtsTK.mCOM);						// si imposta la COM di protocollo primario
		Delay(5);
		for (k=0;k<150;k++){
			r = M_GetSerNum() ;
			if (r!=0) break;
		}	
		M_Diag(250,0,dDg);                   	//Sblocco Scrittura S/N
		Delay(5);
		for (i=1; i<=4; i++) {
			M_SetPar(255,"-1");				   		//Scrivo S/N -1
			Delay(10);
			k = M_GetSerNum();								// si rilegge il Serial-Number
			if (k==-1) break;
		}
		if (k!=-1) call_exit(YES, "Errore cancellazione Serial NUMBER");
		sprintf(Bmom, "s/n Assegnato = %d",k);
		OuputText(1,Bmom,0,0,1,7);				// ... scrivo il Serial Number nella Output Text
		//Cancello Parametri
		M_SetPar(255,"N.D.");  //Cancello Tutti i Parametri
		//Cancello Macchina Stati
		//Cancello CanBus
	}
  int64_t Status;	
	Status=SK_SendCode() ;
	if (Status == -1){
		sprintf(bufwindow, "Invio codice E R R A T O ---> MTS tipo %d, s.n. %d (Ver.%5.2f) CheckSum(0x%lX)", MTSdata.mSign, MTSdata.mSerial, MTSdata.SVer,Status);
		COLOR_STEP(MtsTK.steptest, C_RED ) ;
	}
	MsgWindow(" ");
	MsgWindow(bufwindow);
	MsgFile(0, LogCollaudo, bufwindow);
	if (Status == -1) call_exit(NO, bufwindow) ;

	if (TestSet.EnBaptismo){
		char LogCollaudoNew[MAXSIZE] ;
		unsigned int NextNum ;
		for (i=1; i<=4; i++) {
				GetINIKeyVal(nextip, TkIni.NewNum);
				togliCR(TkIni.NewNum);
				StampaDB("NewNum", TkIni.NewNum);
				M_SetPar(255, TkIni.NewNum);					// ... si inizializza il Serial Number    
				Delay(10);
				k = M_GetSerNum();								// si rilegge il Serial-Number
				if (k!=-1) break;
		}
		sprintf(Bmom, "s/n Assegnato = %d",k);
		OuputText(1,Bmom,0,0,1,7);				// ... scrivo il Serial Number nella Output Text
		if(k==-1){
			call_exit(YES, "Errore assegnazione S/N Serial NUMBER");
		}else{
				sprintf(LogCollaudoNew, "%s\\Logs\\Log%s.txt", mRoot,TkIni.NewNum);
				CONVERTPATH(LogCollaudoNew);
				PrintDB(LogCollaudoNew);
				NextNum = atoi(TkIni.NewNum) + 1;
				sprintf(TkIni.NewNum,"%d",NextNum);
				PrintDB(TkIni.NewNum);
				SetINIKeyVal(nextip, TkIni.NewNum);
				srand(time(0)); /* n is random number in range of 0 - 1 */
				char NumSet[MAXSIZE];
				unsigned int NumSetn;
				int r;
				while (1) {
					GetINIKeyVal(nextip, NumSet);
					togliCR(NumSet);
			   		NumSetn = atoi(NumSet);
			   		if ( NumSetn==NextNum ) break;
					PrintDB("Attesa incrementazione numero\n") ;
					sprintf(TkIni.NewNum,"%d",NextNum);
					PrintDB(TkIni.NewNum);
					SetINIKeyVal(nextip, TkIni.NewNum);
					sprintf(NumSet,"0");
					r=100+(rand() % 100) ; 
					Delay(r);
				}
				rename(LogCollaudo,LogCollaudoNew);					// si rinomina il file di log del collaudo
				sprintf(LogCollaudo,"%s",LogCollaudoNew);			// ed il nome che lo identifica
				MTSdata.mSerial = k ;
		}
	}

	sprintf(bufwindow, "Aggiornamento completato ---> MTS tipo %d, s.n. %d (Ver.%5.2f) CheckSum(0x%lX)", MTSdata.mSign, MTSdata.mSerial, MTSdata.SVer,Status);
	COLOR_STEP(MtsTK.steptest, C_GREEN ) ;
	MsgWindow(" ");
	MsgWindow(bufwindow);
	MsgFile(0, LogCollaudo, bufwindow);
	call_exit(NO, bufwindow) ;
#else
	SK_TK_Init();													// Inizializza il T-K perchè sia invisibile all'inserimento dei connettori  


	if ( (TestSet.EnCANLOGISTIC) && (
				(!strcmp(TkIni.mName, "2046")) ||
				(!strcmp(TkIni.mName, "2044	")) ||
				(!strcmp(TkIni.mName, "2046_M4")) ||		
				(!strcmp(TkIni.mName, "2044_M4"))
	) ) TestSet.EnCANLOGISTIC=YES;

	// Controlla che non si siano impostati Ingressi sia in Corrente che in Tensione
	if (TestSet.EnAnlIn){
		if ((TestSet.QtaInV>0) && (TestSet.QtaInA>0) && (strcmp(TkIni.mName, "3025")) ){
			MSGBOXCALL ("Impostati Ingressi Analogici sia in Tensione che in Corrente!",0,2,"Continua","Ferma",bufresponse);	
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore File Conf");
			}			
			TestSet.EnAnlIn = NO ;
		}
	}		
	
	// Continua dopo aver letto le impostazioni
	if (TestSet.EnRTC) 		LABEL_STEP(i++, "Real Time Clock" ) ;
	
	if (TestSet.EnVext)		LABEL_STEP(i++,"Alimentazione") ;
	if (TestSet.EnVbat)		LABEL_STEP(i++,"Batteria") ;
	if ( ((TestSet.EnDigInDn) || (TestSet.EnDigInUp))>0){
		if (MtsTK.WTastiera){
			if ((TestSet.EnDigInDn) && (TestSet.EnDigInUp)) LABEL_STEP(i++, "Pulsanti + Switch" ) ;
		}else{
			LABEL_STEP(i++,"Ingressi digitali") ;
		}
	}
	if (TestSet.EnDigOut)	LABEL_STEP(i++,"Uscite digitali") ;
	if (TestSet.EnAnlIn)	LABEL_STEP(i++,"Ingressi analogici") ;
	if (TestSet.EnHTL)	LABEL_STEP(i++,"Porta HTL") ;
// Counter e LED esterno
	if (TestSet.QtaCnt){
		if (TestSet.EnExtLED)
			LABEL_STEP(i++,"Counter e LED esterno") ;
		else
			LABEL_STEP(i++,"Counter") ;
	}else if (TestSet.EnExtLED)
		LABEL_STEP(i++,"LED esterno") ;
//CAN
	if ( (TestSet.QtaCAN) || (TestSet.EnCANConf) ) LABEL_STEP(i++,"CAN") ;
//COM2 e Vconsole
	if (TestSet.EnCOM2){
		LABEL_STEP(i++,"COM2 e Vconsole") ;
		TestSet.EnVcns = 1 ;
	}else if (TestSet.EnVcns){
		LABEL_STEP(i++,"Alimentazione Console") ;
	}
	if (TestSet.EnCANLOGISTIC) LABEL_STEP(i++,"CANLOGISTIC") ;
	if (TestSet.EnRS485)	LABEL_STEP(i++,"Porta RS485") ;
	if (TestSet.EnCOMAUX)	LABEL_STEP(i++,"Porta COMAUX") ;
	if (TestSet.NetIP)		LABEL_STEP(i++,"Rete TCP/IP") ;
	if (TestSet.QtaUSB)		LABEL_STEP(i++,"Porte USB") ;
	if (TestSet.EnChBat)	LABEL_STEP(i++,"Carica batteria") ;
	if (TestSet.EnEmeLink)	LABEL_STEP(i++,"Emergency link") ;
	if (TestSet.EnVibro)	LABEL_STEP(i++,"Sensore vibrazione") ;
	if (TestSet.EnAccel)	LABEL_STEP(i++,"Sensore accelerometrico") ;
	if (TestSet.EnLCD)		LABEL_STEP(i++,"Display LCD") ;
	if (TestSet.QtaLaserS)	LABEL_STEP(i++,"Laser Scanner") ;
	if (TestSet.EnGPS)		LABEL_STEP(i++,"Verifiche GPS") ;
	if (TestSet.EnGSM){
		LABEL_STEP(i++,"Verifiche GSM") ;
		if (TestSet.EnFonia)	LABEL_STEP(i++,"Collaudo Fonia") ;
	}
	LABEL_STEP(i++, "Impostazioni di fine collaudo") ;
	
// ==================== Definizione delle Porte =====================
// Porte di T-K (Test-Kit):
//	======================================================================

	if (TestSet.QtaRL==0){				// Se nessun relé		|    D15 variazioni su D07
		if (TKTYPE==0){
			//   ingressi: C04 C05 C06 C07 - D04 D05 D06 D07 - B12 A24 A06 C03 - D15 C14
			T_SetMapDigIn("204205206207304305306307112024006203315214");
		}else{
			//   ingressi T-K:  C00C03C04C05C06C07C01C02C08C09C10C11C12C13C14C15B12A24A06B10 			
			//   ingressi: C00 C01 C02 C03 - C04 C05 C06 C07 - C08 C09 C10 C11 - C12 C13 C14 C15 - B12 A24 A06 B10 C07 C02 CO8
			T_SetMapDigIn ("200201202203204205206207208209210211212213214215112024006110207202208");
		}
	}else{
		if (TKTYPE==0){
			//   ingressi: C04 C07 D04 D05 - D06 D07 C05 C06 - B12 A24 A06 C03 - D15 C14
			T_SetMapDigIn("204207304305306307205206112024006203315214");
		}else{
			//   ingressi T-K:  C00C03C04C05C06C07C01C02C08C09C10C11C12C13C14C15B12A24A06B10 			
			//   ingressi: C00 C03 C04 C05 - C06 C07 C01 C02 - C08 C09 C10 C11 - C12 C13 C14 C15 - B12 A24 A06 B10 C07 C02 C08
			T_SetMapDigIn ("200203204205206207201202208209210211212213214215112024006110207202208");

		}
	}
//	======================================================================
// Porte di MTS
//	======================================================================

	if (TestSet.QtaRL==0){										// Se nessun relé
//     uscite:      b0 b1 b2 b3 b4 b5 b6 b7
		M_SetMapDigOut("000001002003004005006007");
	}else{
		//if (!strcmp(TkIni.mName,"3008")) {
		if (TestSet.QtaRL==1){									// Se 1 relé
//     uscite:      b0 b3 b2 b4 b5 b6 b7 b1
			M_SetMapDigOut("000003002004005006007001");
		}else { // TestSet.QtaRL>1 (2)
//     uscite:      b0 b3 b4 b5 b6 b7 b1 b2
			M_SetMapDigOut("000003004005006007001002");
		}
	}
	
//  ingressi :  b0 b1 b2 b3 - b4 b5 b6 b7 - b8 b9b10b11 - b12b13b14b15   -- in pratica non c'è mappatura !!!!!
	M_SetMapDigIn("000001002003004005006007008009010011012013014015");

	MTSdata.AlimFail = 0;
	SK_PowerOn();							// Accensione dell'MTS
	MTSdata.ERRTest = 0;					// Si inizializza la variabile che conterà il numero di "Avaria" incontrate

		SK_CheckId();							// Controllo di Identità MTS e Presenza
	
		SK_Set_MTS();							// Si settano nell'MTS i parametri e/o la Macch.Stati per il collaudo
// Imposta valori per Vbat
	if (strncmp(TkIni.mName, "MTS", 3)) {
		MtsTK.VbatMAX =  4.3 ;
		MtsTK.VbatMIN =  3.5 ;
	}else{
		MtsTK.VbatMAX = 10.8 ;
		MtsTK.VbatMIN =  8.0 ;
	}
	
	MtsTK.steptest++ ;		// Salta voce "Inizio Collaudo"
	
	MTSdata.ERRTest += SK_Test_RTC();		// Si testa il Real Time Clock (superlfluo per ARM7)
	T_Output(PON_, 0);						// Rimetto comunque l'Alimentazione
	Delay(20);
	MTSdata.ERRTest += SK_Test_Vext();			// Si testa la lettura dell'MTS della Tensione Esterna
	MTSdata.ERRTest += SK_Test_Vbat();			// Si testa la lettura della Batteria dell'MTS
	//MTSdata.ERRTest += SK_Test_DigIn();		// Si testano gli Ingressi dell'MTS
	if ( SK_Test_DigIn() ) {
		MSGBOXCALL("ATTENZIONE INGRESSI IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_DigIn();			// Si testano gli Ingressi dell'MTS
		}
	}
	//MTSdata.ERRTest += SK_Test_DigOut();		// Si testano le Uscite dell'MTS
	if ( SK_Test_DigOut() ) {
		MSGBOXCALL("ATTENZIONE USCITE IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_DigOut();		// Si testano le Uscite dell'MTS
		}
	}
	
//	Preparazione x Test di Vibrazione
	M_Diag(2, 0, dDg); 					// Legge lo stato del sensore di vibrazione e ...
	VrtSt = atoi(dDg);					// ... fa pulizia del buffer per test vibrazione
	M_SetPar(70, "10240");			// soglia più dura
		
	if ( !strcmp(TkIni.mName, "3025") ) {
		int momQtaInV=TestSet.QtaInV;
		TestSet.QtaInV=0;
		MTSdata.ERRTest += SK_Test_AnlIn();			// Si testano gli Ingressi Analogici Corrente dell'MTS 3025
		TestSet.QtaInV=momQtaInV;
		TestSet.QtaInA=0;
		MtsTK.steptest--;
		MTSdata.ERRTest += SK_Test_AnlIn();			// Si testano gli Ingressi Analogici Tensione dell'MTS 3025
	}else{
		//MTSdata.ERRTest += SK_Test_AnlIn();			// Si testano gli Ingressi Analogici dell'MTS
		if ( SK_Test_AnlIn() ) {
			MSGBOXCALL("ATTENZIONE ANALOGICI IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				MTSdata.ERRTest +=1;
			}else{
				MtsTK.steptest--;
				COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
				MTSdata.ERRTest += SK_Test_AnlIn();			// Si testano gli Ingressi Analogici dell'MTS
			}
		}
	}

	if ( SK_Test_HTL() ) {
		MSGBOXCALL("ATTENZIONE HTL IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_HTL();		// Si testa l'interfaccia HTL (per 3025)
		}
	}
#ifdef DEBUG_1FR
	Repeat=1 ; while(Repeat)
#endif
	//MTSdata.ERRTest += SK_Test_Cnt();			// Si testano gli ingressi Counter dell'MTS
	if ( SK_Test_Cnt() ) {
		MSGBOXCALL("ATTENZIONE COUTER IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_Cnt();			// Si testano gli ingressi Counter dell'MTS
		}
	}
	//MTSdata.ERRTest += SK_Test_CAN();			// Si testa il CAN dell'MTS
	if ( SK_Test_CAN() ) {
		MSGBOXCALL("ATTENZIONE CAN IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_CAN();			// Si testa il CAN dell'MTS
		}
	}
	//MTSdata.ERRTest += SK_Test_COM2();			// Si testa la seconda seriale dell'MTS (e/o Vcns)
	if ( SK_Test_COM2() ) {
		MSGBOXCALL("ATTENZIONE COM SECONDARIA IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_COM2();			// Si testa la seconda seriale dell'MTS (e/o Vcns)
		}
	}
	if ( SK_Test_CANLOGISTIC() ) {
		MSGBOXCALL("ATTENZIONE CANLOGISTIC IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_CANLOGISTIC();			// Si testa CANLOGISTIC
		}
	}
#ifdef DEBUG_1FR
	Repeat=1 ; while(Repeat)
#endif
	MTSdata.ERRTest += SK_Test_RS485();			// Si testa l'interfaccia RS485 (per MTS40C e 4004)

#ifdef DEBUG_1FR
	Repeat=1 ; while(Repeat)
#endif

#ifdef DEBUG_1FR
	Repeat=1 ; while(Repeat)
#endif
	MTSdata.ERRTest += SK_Test_COMAUX();		// Si testa l'interfaccia COM Ausiliaria (per 3025)
#ifdef DEBUG_1FR
	Repeat=1 ; while(Repeat)
#endif
	MTSdata.ERRTest += SK_TestLAN();			// Si testa l'interfaccia TCP/IP

#ifdef DEBUG_1FR
	Repeat=1 ; while(Repeat)
#endif
	MTSdata.ERRTest += SK_TestUSB();			// Si testano i connettore USB

	MTSdata.ERRTest += SK_Test_ChBat();			// Si testa il Carica Batteria dell'MTS
	
	// MTSdata.ERRTest += SK_TestEmeLink();		// Si testa l'emergency link

	if ( SK_TestEmeLink() ) {
		MSGBOXCALL("ATTENZIONE Emergecy Link IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_TestEmeLink();		// Si testa l'emergency link
		}
	}
	
	//MTSdata.ERRTest += SK_Test_Vibro();			// Si testa il sensore di Vibrazione dell'MTS
	if ( SK_Test_Vibro() ) {
		MSGBOXCALL("ATTENZIONE TEST VIBRAZIONE IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_Vibro();			// Si testa il sensore di Vibrazione dell'MTS
		}
	}
	//MTSdata.ERRTest += SK_Test_Accel();			// Si testa il sensore Accelerometro dell'MTS
	if ( SK_Test_Accel() ) {
		MSGBOXCALL("ATTENZIONE TEST RIBALTAMENTO IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_Accel();			// Si testa il sensore Accelerometro dell'MTS
		}
	}
	MTSdata.ERRTest += SK_Test_LCD();			// Si testa il sensore Accelerometro dell'MTS
	MTSdata.ERRTest += SK_TestLaserS();			// Si testa il Laser Scanner dell'MTS
	if ( SK_Test_GPS() ) {
		MSGBOXCALL("ATTENZIONE GPS IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			MTSdata.ERRTest += SK_Test_GPS();			// Si testa il GPS dell'MTS
		}
	}
	
	if ( SK_Test_GSM() ) {
		MSGBOXCALL("ATTENZIONE GSM IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			MTSdata.ERRTest +=1;
		}else{
			MtsTK.steptest--;
			COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			if (TestSet.EnFonia==YES) {
				MtsTK.steptest--; 
				COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
			}
			MTSdata.ERRTest += SK_Test_GSM();			// Si testa il GSM dell'MTS
		}
	}
	COLOR_STEP(MtsTK.steptest, C_YELLOW ) ;
	SK_EndChk();									// conclusione del collaudo
	COLOR_STEP(MtsTK.steptest, C_GREEN ) ;
	
	//call_exit(NO, "MTestEnd");

#endif // #ifndef SENDFW

	return 0 ;
}

void call_exit(int error, char * cc)
{
	if (error) COLOR_STEP(MtsTK.steptest, C_RED) ;
#ifdef DEBUG_1FR
	Repeat = 0 ;
#else
	// Re-Inizializza la ProgressBar a 0
	ProgressBar(BAR_PERC, 0) ;
	SK_TK_PreInit();	
	PrgExit(cc) ;
#endif
}

int SK_GetIniInfo(void)
{
#ifndef SENDFW
	int i ;
#endif
	int qta;
	char NewRiga[MAXSIZE];
	char Chiave[NRMSIZE], Valore[NRMSIZE] ;
	FILE *fimpo ;

	GetINIKeyVal("workingPath", TkIni.NamTyp);		// Legge da dir di lavoro
	togliCR(TkIni.NamTyp);
	
	// unità in produzione con tastiera, non hanno presenza e hanno le 3 uscite in ordine diverso da altri MTS
	MtsTK.WTastiera = (TkIni.mName[0]=='1') ; // Famiglia "1" ha la tastiera
	
			
	// Prepara le path con la dir dell'MTS
	sprintf(mRoot, "%s\\%s", WSpace, TkIni.NamTyp);
	CONVERTPATH(mRoot);
	sprintf(mAppl, "%s\\Applicativi", mRoot);
	CONVERTPATH(mAppl);
	sprintf(SMachVoid, "%s\\void.smk", PathTK);
	CONVERTPATH(SMachVoid);
	
	if (MtsTK.WTastiera) {
		// Configura porta 1 CMOS tra T-K e MTS
		sprintf(MtsTK.COM1, "%s", "28");
				
		// Configura porta 2 CMOS tra T-K e MTS
		sprintf(MtsTK.COM2, "%s", "24");
		
		// Macch.Stati per il collaudo di tastiera
		sprintf(SMachColl, "%s\\TastControl.smk", PathTK);
		CONVERTPATH(SMachColl);
		
		// Macch.Stati di MTS Testato da inviare alla fine del collaudo
		sprintf(SMachTest, "%s\\TastTested.smk", PathTK);
		CONVERTPATH(SMachTest);
	} else {
		if (TkIni.mName[0]=='2'){ // Famiglia '2' ha 1 seriale
			if ( 
						( !strcmp(TkIni.mName, "2034")) || 
						( !strcmp(TkIni.mName, "2039")) || 
						( !strcmp(TkIni.mName, "2040")) || 
						( !strcmp(TkIni.mName, "2046")) || 
						( !strcmp(TkIni.mName, "2044")) || 
						( !strcmp(TkIni.mName, "2047")) || 
						( !strcmp(TkIni.mName, "2045")) || 
						( !strcmp(TkIni.mName, "2405")) || 
						( !strcmp(TkIni.mName, "2051")) || 
						( !strcmp(TkIni.mName, "2052")) || 
						(!strcmp(TkIni.mName, "2046_M4")) || 
						(!strcmp(TkIni.mName, "2044_M4")) 
			) {
				// Configura porta 1 RS232 tra T-K e MTS
				sprintf(MtsTK.COM1, "%s", "130");
			
				// Configura porta 2 RS232 tra T-K e MTS
				sprintf(MtsTK.COM2, "%s", "194");	
			}else{
				if (!strcmp(TkIni.mName, "2046USB")) { //Pre Versione con USB
					// Configura porta 1 RS232 tra T-K e MTS
					sprintf(MtsTK.COM1, "%s","USB");
			
					// Configura porta 2 RS232 tra T-K e MTS
					sprintf(MtsTK.COM2, "%s", "28");
				}else{
					if ( 
							( !strcmp(TkIni.mName, "2047TTL")) || 
							( !strcmp(TkIni.mName, "2051TTL")) ||
							( !strcmp(TkIni.mName, "2047_M4TTL")) || 
							( !strcmp(TkIni.mName, "2051_M4TTL"))
					  ) {
						if ( !strcmp(TkIni.mName, "2047TTL") ) {
							StampaDB("Change mName 2047TTL to 2047", TkIni.mName);
							sprintf(TkIni.mName,"%s","2047");
							StampaDB("Change mName 2047TTL to 2047", TkIni.mName);
						}
						if ( !strcmp(TkIni.mName, "2051TTL") ) {
							StampaDB("Change mName 2051TTL to 2051", TkIni.mName);
							sprintf(TkIni.mName,"%s","2051");
							StampaDB("Change mName 2051TTL to 2051", TkIni.mName);
						}
						if ( !strcmp(TkIni.mName, "2047_M4TTL") ) {
							StampaDB("Change mName 2047_M4TTL to 2047_M4", TkIni.mName);
							sprintf(TkIni.mName,"%s","2047_M4");
							StampaDB("Change mName 2047_M4TTL to 2047_M4", TkIni.mName);
						}
						if ( !strcmp(TkIni.mName, "2051_M4TTL") ) {
							StampaDB("Change mName 2051_M4TTL to 2051_M4", TkIni.mName);
							sprintf(TkIni.mName,"%s","2051_M4");
							StampaDB("Change mName 2051_M4TTL to 2051_M4", TkIni.mName);
						}
						// Configura porta 1 RS232 tra T-K e MTS
						sprintf(MtsTK.COM1, "%s", "28");
					}else{
						// Configura porta 2 RS232 tra T-K e MTS
						sprintf(MtsTK.COM2, "%s", "130");
					}
				}
			}
		}else {
			if (!strcmp(TkIni.mName,"MTS02")){
				// Configura porta 1 RS232 tra T-K e MTS
				sprintf(MtsTK.COM1, "%s", "28");
			}else{
				// Configura porta 1 RS232 tra T-K e MTS
				sprintf(MtsTK.COM1, "%s", "130");
			}
			// Configura porta 2 RS232 tra T-K e MTS
			sprintf(MtsTK.COM2, "%s", "194");
		}
		// Macch.Stati per il collaudo (sarà una 'void')
		sprintf(SMachColl, "%s\\MTS-Control.smk", PathTK);
		CONVERTPATH(SMachColl);
		
		// Macch.Stati di MTS Testato da inviare alla fine del collaudo
		sprintf(SMachTest, "%s\\MTS-Tested.smk", PathTK);
		CONVERTPATH(SMachTest);
	}
	
	StampaDB("WorkSpace", WSpace);						// Su finestra DOS di Debug 
	StampaDB("workingPath", mRoot);						// Su finestra DOS di Debug 
	
	GetINIKeyVal("prgFileRadix", TkIni.NamFile);
	togliCR(TkIni.NamFile);
	StampaDB("prgFileRadix", TkIni.NamFile);					// Su finestra DOS di Debug 
	
	sprintf(mVer, "%s\\Versioni", mRoot);                  //Versione 
	CONVERTPATH(mVer);
  	
  	// Chiede numero di versione + recente e dimensione file
#ifdef SENDFW
  	GetBestFileNum(mVer, TkIni.NamFile, MtsTK.mFwVer) ;	
  	
	StampaDB("BestFileNum", MtsTK.mFwVer);							// Su finestra DOS di Debug 
#endif ///#ifdef SENDFW
#ifndef SENDFW	
	GetINIKeyVal(nextip, TkIni.NewNum);
	togliCR(TkIni.NewNum);
	//Aggiunto controllo su SN lughezza di 9 caratteri
	if ((strlen(TkIni.NewNum) != LEN_NUM) && (strcmp(TestSet.Cpu,"HC12")) && (strcmp(TkIni.mName,"MTS40A-B")) && (strcmp(TkIni.mName,"MTS40C")) ){
			sprintf(bufmsg, "Attenzione! Nel File ini per %s impostato Serial Number diverso da 9 caratteri ", TkIni.mName);
			call_exit(YES, bufmsg);
		}
	StampaDB("NewNum", TkIni.NewNum);							// Su finestra DOS di Debug 
//#endif // ifndef SENDFW
//#ifndef SENDFW
	
// 	IMPOSTAZIONI per il collaudo
//--------------------------------------------------------------------------------------------------------
//	fissaggio delle impostazioni dei default

	TestSet.Cpu[0] ='\0';
	
	TestSet.BootTime = 60 ;
	TestSet.SisCOM = 1 ;
	TestSet.QtaOD = 3 ;
	TestSet.QtaRL = 0 ;
	TestSet.QtaIn = 3 ;
	TestSet.MaskDigInDn = 0;
	TestSet.MaskDigInUp = 0;
	TestSet.QtaInA = 1 ;
	TestSet.QtaInV = 0 ;
	TestSet.QtaCAN = 0 ;
	TestSet.CntToller = 350 ;
	TestSet.SogliaF = 90 ;
	TestSet.QtaLaserS = 0 ;
	TestSet.NetIP = 0 ;
	TestSet.QtaUSB = 0 ;
	TestSet.QtaCnt = 1 ;
	TestSet.GSMbaud = 0 ; // No lock GSM baud

	TestSet.AnlTaratura = NO ;
	TestSet.EnTrm = YES ;
	TestSet.EnPres = YES ;
	TestSet.EnRTC = NO ;
	TestSet.EnDigInDn = YES ;
	TestSet.EnDigInUp = YES ; 
	TestSet.EnDigOut = YES ;
	TestSet.EnTamper = YES;
	TestSet.EnAnlIn = YES ;
	TestSet.EnVext = YES ;
	TestSet.EnVbat = YES ;
//	TestSet.EnCnt = YES ;
	TestSet.EnGPS = YES ;
	TestSet.EnGSM = YES ;
	TestSet.EnFonia = YES ;
	TestSet.EnCOM2 = NO ; // Cambiato default da 2.00
	TestSet.EnCANLOGISTIC = NO ;
	TestSet.EnVcns = NO ;
	TestSet.EnRS485 = NO ;
	TestSet.EnHTL = NO ;
	TestSet.EnCOMAUX = NO ;
	TestSet.EnChBat = YES ;
	TestSet.EnVibro = YES ;
	TestSet.EnAccel = YES ;
	TestSet.EnLCD = NO ;
	TestSet.EnExtLED = NO ;
	TestSet.EnEmeLink = NO ;
	TestSet.EnBKdata = NO ;
#endif ///#ifdef SENDFW
	
	TestSet.EnSendFW = YES;
	TestSet.ASKFW = NO;
	TestSet.EnSMset = NO ;
	TestSet.EnCANConf = NO;
	TestSet.EnPIN_Param= NO;
	TestSet.EnFormat= NO;
	TestSet.EnBaptismo= NO;
	TestSet.StartProtocol[0] ='\0';
	leveldebug = 0 ;

#ifndef SENDFW	
	TestSet.EnSIMdata = NO ;
	TestSet.EnNoAnt = YES;
		
	TestSet.VextMin = 12.0 ;
	TestSet.VextMax = 15.0 ;
	TestSet.CurrMin = 100.0 ;
	TestSet.CurrMax = 500.0 ;
	TestSet.An_Perc = 4 ;
	TestSet.CAN_Term_Perc = 5 ;
	
//--------------------------------------------------------------------------------------------------------
#endif //#ifndef SENDFW	
	GetINIKeyVal("FileImpostazioni", TkIni.FileImp);
	togliCR(TkIni.FileImp);
	sprintf(NewRiga, "%s\\Script\\%s", mRoot, TkIni.FileImp);		// File con le impostazioni di collaudo
	CONVERTPATH(NewRiga) ;
	
	fimpo = fopen(NewRiga, "r");				// Apertura del file di impostazioni
	if (fimpo==NULL) {							// Se il file non esiste
		QtaLog++;
#ifndef SENDFW	
		sprintf(LogBuffer[QtaLog], "\r------ Collaudo eseguito con impostazioni di DEFAULT -------");
#else
		sprintf(LogBuffer[QtaLog], "\r------ Programmazione eseguita con impostazioni di DEFAULT -------");
#endif //#ifndef SENDFW	
		MsgWindow(LogBuffer[QtaLog]);
	} else {								   // si leggono le chiavi per il settaggio del collaudo
		// il file è già aperto !!!!!!
		QtaLog++;
#ifndef SENDFW	
		sprintf(LogBuffer[QtaLog], "\r------ Collaudo eseguito con impostazioni di %s ----", NewRiga);
#else
		sprintf(LogBuffer[QtaLog], "\r------ Programmazione eseguita con impostazioni di %s ----", NewRiga);
#endif //#ifndef SENDFW	
		MsgWindow(LogBuffer[QtaLog]);
#ifndef SENDFW
		QtaLog++; 
		sprintf(LogBuffer[QtaLog], "\rTerzista:<%s>", hostname); //TestSet.Terzista);
		MsgWindow(LogBuffer[QtaLog]);
#endif //#ifndef SENDFW	
		while (!feof(fimpo)) {								// fino alla fine del file
			NewRiga[0] = '\0' ;
			loc_fgets(NewRiga, MAXSIZE-1, fimpo);			// Legge una nuova riga (Massimo di 511 caratteri)
			togliCR(NewRiga);
			Valore[0] = '\0' ;
			if (NewRiga[0]){
				if (isalpha(NewRiga[0])) {						// se inizia con una lettera può essere una chiave
					// Rimuove, se esiste, il commento ';'
					RemoveComment(NewRiga) ;
					GetIntStr("=", NewRiga, 1, Chiave);
					qta = GetIntStr("=", NewRiga, 2, Valore);
					if (qta!=0) {
						sprintf(bufwindow,"Chiave %s senza valore valido!", Chiave);
						MsgWindow(bufwindow);
					} else {
						togliCR(Valore);
						UpperAlfaNum(Valore);			// si converte tutto in maiuscolo
						RLTrimm(Valore);				// e si tolgono eventuali caratteri non alfanumerici prima e dopo
						RLTrimmwithplace(Valore); // e si tolgono eventuali caratteri non alfanumerici prima e dopo
#ifdef SENDFW
						if ( (!strcmp(Chiave,"EnSendFW")) || (!strcmp(Chiave,"EnSMset")) || (!strcmp(Chiave,"EnCANConf")) || (!strcmp(Chiave,"EnFormat")) || (!strcmp(Chiave,"EnBaptismo")) || (!strcmp(Chiave,"StartProtocol"))  )	{
#endif //#ifdef SENDFW
						QtaLog++;
						// si registrano in buffer le impostazioni incontrate
						sprintf(LogBuffer[QtaLog], "<%s>---> Impostato: <%s>=<%s>", NewRiga, Chiave, Valore);
//#ifndef SENDFW
						MsgWindow(LogBuffer[QtaLog]);
//#endif // #ifdef CBUG
#ifdef SENDFW
						}
#endif //#ifdef SENDFW	
	
	//				decodifica della chiave !!!!!!!!! 
						if (!strcmp(Chiave,"CPU"))					strcpy(TestSet.Cpu,	Valore);
#ifndef SENDFW							 
						else if (!strcmp(Chiave,"BootTime"))		TestSet.BootTime = atoi(Valore);
						else if (!strcmp(Chiave,"SisCOM"))			TestSet.SisCOM = atoi(Valore);
						else if (!strcmp(Chiave,"QtaOD"))			  TestSet.QtaOD = atoi(Valore);
						else if (!strcmp(Chiave,"QtaRL"))			  TestSet.QtaRL = atoi(Valore);
						else if (!strcmp(Chiave,"QtaIn"))			  TestSet.QtaIn = atoi(Valore);
						else if (!strcmp(Chiave,"MaskDigInDn"))	TestSet.MaskDigInDn = atoi(Valore);
						else if (!strcmp(Chiave,"MaskDigInUp"))	TestSet.MaskDigInUp = atoi(Valore);
						else if (!strcmp(Chiave,"QtaInA"))			TestSet.QtaInA = atoi(Valore);
						else if (!strcmp(Chiave,"QtaInV"))			TestSet.QtaInV = atoi(Valore);
						else if (!strcmp(Chiave,"QtaCAN"))			TestSet.QtaCAN = atoi(Valore);
						else if (!strcmp(Chiave,"CntToller"))		TestSet.CntToller = atoi(Valore);
						else if (!strcmp(Chiave,"SogliaF"))			TestSet.SogliaF = atoi(Valore);
						else if (!strcmp(Chiave,"QtaLaserS"))		TestSet.QtaLaserS = atoi(Valore);
						else if (!strcmp(Chiave,"QtaCnt"))			TestSet.QtaCnt = atoi(Valore);
						else if (!strcmp(Chiave,"GSMbaud"))			TestSet.GSMbaud = atoi(Valore);
						else if (!strcmp(Chiave,"NetIP")){
							strcpy(TestSet.CnetIP, Valore) ;
							TestSet.NetIP = (int) inet_addr(Valore);
						}
						else if (!strcmp(Chiave,"QtaUSB"))			TestSet.QtaUSB = atoi(Valore);


						else if (!strcmp(Chiave,"AnlTaratura")) 	TestSet.AnlTaratura = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnTrm"))			TestSet.EnTrm = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnPres"))			TestSet.EnPres = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnRTC"))			TestSet.EnRTC = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnDigInDn"))		TestSet.EnDigInDn = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnDigInUp"))		TestSet.EnDigInUp = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnDigOut"))		TestSet.EnDigOut = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnTamper"))		TestSet.EnTamper = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnAnlIn"))			TestSet.EnAnlIn = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnVext"))			TestSet.EnVext = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnVbat"))			TestSet.EnVbat = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnCnt")) {		
																	if ( TestSet.QtaCnt <= 1) {
																		TestSet.QtaCnt = (!strncmp(Valore,"SI",2)) ;
																	}
						}
						else if (!strcmp(Chiave,"EnGPS"))			TestSet.EnGPS = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnGSM"))			TestSet.EnGSM = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnFonia"))			TestSet.EnFonia = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnCOM2"))			TestSet.EnCOM2 = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnCANLOGISTIC")) TestSet.EnCANLOGISTIC = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnVcns"))			TestSet.EnVcns = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnRS485"))			TestSet.EnRS485 = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnHTL"))			TestSet.EnHTL = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnCOMAUX"))		TestSet.EnCOMAUX = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnChBat"))			TestSet.EnChBat = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnVibro"))			TestSet.EnVibro = (!strncmp(Valore,"SI",2)) ; 
						else if (!strcmp(Chiave,"EnAccel"))			TestSet.EnAccel = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnLCD"))			TestSet.EnLCD = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnExtLED"))		TestSet.EnExtLED = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnEmeLink"))		TestSet.EnEmeLink = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnBKdata"))		TestSet.EnBKdata = (!strncmp(Valore,"SI",2)) ;
#endif //#ifndef SENDFW	
						else if (!strcmp(Chiave,"EnSendFW"))		TestSet.EnSendFW = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"ASKFW"))				TestSet.ASKFW = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnSMset"))			TestSet.EnSMset = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnCANConf"))		TestSet.EnCANConf = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnPIN_Param")) TestSet.EnPIN_Param = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnFormat")) 		TestSet.EnFormat = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnBaptismo")) 		TestSet.EnBaptismo = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"StartProtocol"))	strcpy(TestSet.StartProtocol,	Valore);
						else if (!strcmp(Chiave,"LevelDebug"))	leveldebug = strtol(Valore,NULL,0);
#ifndef SENDFW
						else if (!strcmp(Chiave,"EnSIMdata"))		TestSet.EnSIMdata = (!strncmp(Valore,"SI",2)) ;
						else if (!strcmp(Chiave,"EnNoAnt"))			TestSet.EnNoAnt = (!strncmp(Valore,"SI",2)) ;
						
						else if (!strcmp(Chiave,"VextMin"))			TestSet.VextMin = atof(Valore);
						else if (!strcmp(Chiave,"VextMax"))			TestSet.VextMax = atof(Valore);
						else if (!strcmp(Chiave,"CurrMin"))			TestSet.CurrMin = atof(Valore);
						else if (!strcmp(Chiave,"CurrMax"))			TestSet.CurrMax = atof(Valore);
						else if (!strcmp(Chiave,"An_Perc"))			TestSet.An_Perc = atoi(Valore);

						else if (!strcmp(Chiave,"LaserS1")) 		strcpy(LS_barcode[0].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS2"))			strcpy(LS_barcode[1].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS3"))			strcpy(LS_barcode[2].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS4"))			strcpy(LS_barcode[3].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS5"))			strcpy(LS_barcode[4].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS6"))			strcpy(LS_barcode[5].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS7"))			strcpy(LS_barcode[6].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS8"))			strcpy(LS_barcode[7].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS9"))			strcpy(LS_barcode[8].wait_mgs,	Valore); 
						else if (!strcmp(Chiave,"LaserS10"))		strcpy(LS_barcode[9].wait_mgs,	Valore); 
#endif//#ifndef SENDFW							
					}
				}
			}
		}
		//StampaDB("File letto", "chiuso");				// Su finestra DOS di Debug 
		fclose(fimpo);
	}
#ifndef SENDFW
	LoadProdTab();							        // carica in una tabella il file di associazioni ARCA
	MtsTK.ArcaPos = ReadMTSArca(TkIni.mName);		// cerca la Famiglia dell'MTS in test
	
#endif // #ifndef SENDFW
	// Prepara la path con la dir dei programmi per tipi di CPU (ARM7, ARM9, ATMEGA, HC12)
	sprintf(PathFwUp, "%s\\Applicativi\\%s", WSpace, TestSet.Cpu );
	CONVERTPATH(PathFwUp);

	LoadFamTab();							// carica in una tabella delle famiglie MTS riconosciuti dal T-K (Signature)
	MtsTK.mTipo = ReadMTSType(TkIni.mName);		// cerca la Famiglia dell'MTS in test

#ifdef SENDFW
	if ( (!MtsTK.WTastiera) && (TkIni.mName[0]!='2') ){
		MtsTK.mCOM = 11;							// predispone il T-K a comunicare coll'MTS con LU11
		MtsTK.tCOM = 2;
	} else {
		MtsTK.mCOM = 2;							// predispone il T-K a comunicare coll'MTS con LU2
		MtsTK.tCOM = 11;
		if (
					( !strcmp(TkIni.mName, "2034")) || 
					( !strcmp(TkIni.mName, "2039")) || 
					( !strcmp(TkIni.mName, "2040")) || 
					( !strcmp(TkIni.mName, "2046")) || 
					( !strcmp(TkIni.mName, "2044")) || 
					( !strcmp(TkIni.mName, "2047")) || 
					( !strcmp(TkIni.mName, "2045")) || 
					( !strcmp(TkIni.mName, "2405")) || 
					( !strcmp(TkIni.mName, "2051")) || 
					( !strcmp(TkIni.mName, "2052")) || 
					( !strcmp(TkIni.mName, "2046_M4")) || 
					( !strcmp(TkIni.mName, "2044_M4")) || 
					( !strcmp(TkIni.mName, "2047_M4")) ||
					( !strcmp(TkIni.mName, "2051_M4"))
			) {
					MtsTK.mCOM = 11;							// predispone il T-K a comunicare coll'MTS con LU11
					MtsTK.tCOM = 2;
		}
	}
	return 0 ; // No more needed 
#endif // ifdef SENDFW
	// Usa ModelliARCA per definizioni: MtsTK.MonoPiastra
	if (MtsTK.ArcaPos){ 
		if (QtaArca){	// Se dati Arca
			MtsTK.MonoPiastra = ((Arca[MtsTK.ArcaPos-1].nr_aSV>0)? 0:1 ) ;
		}else{
			if ((TkIni.mName[0]=='4') || (TkIni.mName[0]=='3')) MtsTK.MonoPiastra = 0 ;
		}		
	}	
	
#ifndef SENDFW
// stampa per controllare la memorizzazione della tabella !!!!!!!!!
#ifdef CBUG_
	for (i=1; i<=QtaArca; i++) {
		sprintf(bufwindow,"%s - %s - %s - %d - %s - %s - %s - %s - %s ", Arca[i].cod, Arca[i].mts,  Arca[i].board, Arca[i].nr_aSV,
					Arca[i].srvcard[0], Arca[i].srvcard[1], Arca[i].srvcard[2], Arca[i].srvcard[3], Arca[i].srvcard[4]);
		MsgWindow(bufwindow);
	}
#endif
	if (TestSet.SisCOM == 1) {
		MtsTK.mCOM = 11 ;							// predispone il T-K a comunicare coll'MTS con LU11
		MtsTK.tCOM = 2 ;
	} else {
		MtsTK.mCOM = 2 ;							// predispone il T-K a comunicare coll'MTS con LU2
		MtsTK.tCOM = 11 ;
	}
	if (TkIni.mName[0]=='4'){ // Famiglia '4'	unità con Uscite O.D.(6) e Rele'(2)
//	if (!strcmp(TkIni.mName,"4004") ||			// unità con Uscite O.D.(6) e Rele'(2)				
//			!strcmp(TkIni.mName,"4104")) {
		i = 0;
		if ( (!strcmp(TkIni.mName, "4037")) ) {
			NamOut[i] = 1; i++;
			NamOut[i] = 2; i++;
			NamOut[i] = 3; i++;
			NamOut[i] = 4; i++;		
			NamOut[i] = 5; i++;								
			NamOut[i] = 6; i++;
			NamOut[i] = 7; i++;
			NamOut[i] = 8; i++;
		}else{
											// {1, 4, 5, 6, 7, 8, 2, 3}; 
												// Ordine di sequenza di Uscite O.D.(6) e Rele'(2)
			NamOut[i++] = 1 ;
			NamOut[i++] = 4 ;
			NamOut[i++] = 5 ;
			NamOut[i++] = 6 ;
			NamOut[i++] = 7 ;
			NamOut[i++] = 8 ;
			NamOut[i++] = 2 ;
			NamOut[i++] = 3 ;
		}
	}
	// 'HC12' come famiglia '3'
	if ( ((TkIni.mName[0]=='3') || (!strcmp(TestSet.Cpu,"HC12"))) && ( strcmp(TkIni.mName, "3025") && strcmp(TkIni.mName, "3035") && strcmp(TkIni.mName, "3036"))  ) { // Famiglia '3' unità con Uscite O.D.(3) e Rele'(1)
	//if (!strcmp(TkIni.mName,"3008")) {			// unità con Uscite O.D.(3) e Rele'(1)				
		i = 0;								// {1, 4, 5, 2};
				 							// Ordine di sequenza di Uscite O.D.(3) e Rele'(1)
		if (TestSet.QtaOD>=3){
			NamOut[i++] = 1 ;
			NamOut[i++] = 4 ;
			NamOut[i++] = 5 ;
		}
		if (TestSet.QtaRL>0){
			NamOut[i++] = 2 ;
			if (TestSet.QtaRL==2){		// MTS20-15
				NamOut[i++] = 3 ;
			}
		}
		if (TestSet.QtaOD>3) NamOut[i++] = 6 ;					// le altre sono fittizie
		if (TestSet.QtaOD>4) NamOut[i++] = 7 ;
		if (TestSet.QtaOD>5) NamOut[i++] = 8 ;
	}
	if ( (TkIni.mName[0]=='2') || (TkIni.mName[0]=='1') ){ // FamigliE '2'ED '1'
	//if ( (!strcmp(TkIni.mName,"2102")) || (!strcmp(TkIni.mName,"2102")) || MtsTK.WTastiera) {					
		i = 0;								// {1, 2, 3, 4};
				 							// Ordine di sequenza di Uscite Dirette(3) e Vout (1) 1016 e 2102
		NamOut[i] = 1; i++;
		NamOut[i] = 2; i++;
		NamOut[i] = 3; i++;
		//NamOut[i] = 4*(!strcmp(TkIni.mName,"1016")); 
		NamOut[i] = 4; i++;		// solo per 1016
		NamOut[i] = 0; i++;								// le altre sono fittizie
		NamOut[i] = 0; i++;
		NamOut[i] = 0; i++;
		NamOut[i] = 0; i++;
	}
	if ( (!strcmp(TkIni.mName, "3025"))  ) {
		i = 0;
		NamOut[i] = 1; i++;
		NamOut[i] = 7; i++;
		NamOut[i] = 8; i++;
		NamOut[i] = 4; i++;		
		NamOut[i] = 5; i++;								
		NamOut[i] = 6; i++;
		NamOut[i] = 2; i++;
		NamOut[i] = 3; i++;
	}
	if ( (!strcmp(TkIni.mName, "3035")) || (!strcmp(TkIni.mName, "3036")) ) {
		i = 0;
		NamOut[i] = 1; i++;
		NamOut[i] = 2; i++;
		NamOut[i] = 3; i++;
		NamOut[i] = 4; i++;		
		NamOut[i] = 5; i++;								
		NamOut[i] = 6; i++;
		NamOut[i] = 7; i++;
		NamOut[i] = 8; i++;
	}
	if ( (!strcmp(TkIni.mName, "3048"))  ) {
		i = 0;
		NamOut[i] = 0; i++;
		NamOut[i] = 1; i++;
		NamOut[i] = 2; i++;
		NamOut[i] = 3; i++;		
		NamOut[i] = 4; i++;								
		NamOut[i] = 5; i++;
		NamOut[i] = 6; i++;
		NamOut[i] = 7; i++;
		NamOut[i] = 8; i++;
		NamOut[i] = 9; i++;
		NamOut[i] = 10; i++;
		NamOut[i] = 11; i++;		
		NamOut[i] = 12; i++;								
		NamOut[i] = 13; i++;
		NamOut[i] = 14; i++;
		NamOut[i] = 15; i++;
	}

// Già presente in alto
//	LoadProdTab();							// carica in una tabella il file di associazioni ARCA
//	LoadFamTab();						// carica in una tabella delle famiglie MTS riconosciuti dal T-K (Signature)
//	MtsTK.mTipo = ReadMTSType(TkIni.mName);				// cerca la Famiglia dell'MTS in test
// stampa per controllare la memorizzazione della tabella !!!!!!!!!
#ifdef CBUG_
	for (i=1; i<=QtaArca; i++) {
		sprintf(bufwindow,"%s - %s - %s - %s", BArca[i], BMTS[i], BBrd[i], BSrv[i]);
		MsgWindow(bufwindow);
	}
#endif
#endif // ifndef SENDFW
	return 0;
}

int SK_TK_CheckVersion(void)
{
	char tkver[255] ;
	int tksn;
	char aaa[255];
	int tktypeh;
	TKTYPE=0;
	int TESTVER;
	TESTVER=0;

	tksn=901400101;
	
	if (TESTVER) {
		tksn=0;
		T_GetVer(tkver) ;
	
		char *p;
		p = strtok(tkver, ";");

		if(p)
		//{
		//sprintf(bufwindow,"\r---->   printf: %s   <----\r", p);
		//MsgWindow(bufwindow);
		//}
		p = strtok(NULL, ";");

		//if(p)
		//	 sprintf(bufwindow,"\r---->   printf2: %s   <----\r", p);
		//	 MsgWindow(bufwindow);
	
		tksn=atoi(p);
		//sprintf(bufwindow,"\r---->   TestKitSN: %d   <----\r", tksn);
		//MsgWindow(bufwindow);
	}
	
	T_GetType(aaa);
	tktypeh=atoi(aaa);
	//sprintf(bufwindow,"\r---->   TestKitTESTloopaaa: %d   <----\r", tktypeh);
	//MsgWindow(bufwindow);
	if ( (tksn>901400100) && (tktypeh==1) ) TKTYPE=1;
	sprintf(bufwindow,"\r---->   TestKitTYPE: %d   <----\r", TKTYPE);
	MsgWindow(bufwindow);

	//////////////////////////////////////////////////////////
	///////////// definizioni variabili testkit //////////////
	//////////////////////////////////////////////////////////
	if (TKTYPE==0){
		CHN=OLDCHN;
		AN_DIG_=OLDAN_DIG_;
		AN_CORR_=OLDAN_CORR_;
		//A_MASSA_=OLDA_MASSA_;
		EN_CNT=OLDEN_CNT;
		CAN_EN=OLDCAN_EN;
		PON_=OLDPON_;
		TOD1=OLDTOD1;
		TRL2_=OLDTRL2_;
		TOD5=OLDTOD5;
		TOD4=OLDTOD4;
		TK_MASKOUT=OLDTK_MASKOUT;
		//TIN_PUP=OLDTIN_PUP;
		PRES_=OLDPRES_;
		//AL_OFF=OLDAL_OFF;
		CPU_OFF=OLDCPU_OFF;
		TK_STARTMASK=OLDTK_STARTMASK;
		TK_STARTVAL=OLDTK_STARTVAL;
		SH_C=OLDSH_C;
		PR_RD_=OLDPR_RD_;
		OK_CNS_=OLDOK_CNS_;
		UNK_OK_=OLDUNK_OK_;
		EMLNK_IN2x=OLDEMLNK_IN2x;
		EMLNK_IN3x=OLDEMLNK_IN3x;
		VREF=OLDVREF;
		IREF=OLDIREF;
		TAN2=OLDTAN2;
		TAN3=OLDTAN3;
		TAN4=OLDTAN4;
		TAN5=OLDTAN5;
		TVEXT=OLDTVEXT;
		PWR_C=OLDPWR_C;
		FONIA=OLDFONIA;
		SETAN=OLDSETAN;
		TO_CURR=OLDTO_CURR;				
		TO_VOLT=OLDTO_VOLT;	
	}else{
		CHN=NEWCHN;
		AN_DIG_=NEWAN1ORAN2;
		AN_CORR_=NEWAN_CORR_;
		//A_MASSA_=NEWA_MASSA_;
		EN_CNT=NEWEN_CNT_;
		CAN_EN=NEWCAN_EN_;
		PON_=NEWPON_;
		TOD1=NEWTOD1;
		TRL2_=NEWTRL2_;
		TOD5=NEWTOD3;
		TOD4=NEWTOD2;
		TK_MASKOUT=NEWTODALL ;
		//TIN_PUP=NEWTIN_PUP;
		PRES_=NEWPRES;
		//AL_OFF=NEWAL_OFF;
		CPU_OFF=NEWCPU_OFF;
		TK_STARTMASK=NEWTK_STARTMASK;
		TK_STARTVAL=NEWTK_STARTVAL;
		SH_C=NEWSH_C;
		PR_RD_=NEWPR_RD_;
		OK_CNS_=NEWOK_CNS_;
		UNK_OK_=NEWUNK_OK_;
		EMLNK_IN2x=NEWEMLNK_IN2x;
		EMLNK_IN3x=NEWEMLNK_IN3x;
		VREF=NEWVREF;
		IREF=NEWIREF;
		TAN2=NEWTAN2;
		TAN3=NEWTAN3;
		TAN4=NEWTAN4;
		TAN5=NEWTAN5;
		TVEXT=NEWTVEXT;
		PWR_C=NEWPWR_C;
		FONIA=NEWFONIA;
		SETAN=NEWSETAN;
		TO_CURR=NEWTO_CURR;				
		TO_VOLT=NEWTO_VOLT;
	}
	/*//DEBUG
	int pippo;
	pippo=(NEWCHN |NEWAN1ORAN2|NEWAN_CORR_|NEWEN_CNT_| NEWCAN_EN_ | NEWPON_);
	sprintf(Bmom,"PASSOMASK:%d (0x%x) NEWCAN_EN_:0x%x",pippo,pippo,NEWCAN_EN_);
	PrintDB(Bmom);
	//////////////////////////////////////////////////////////
	sprintf(Bmom,"PASSOinmezzo TKTYPE=%d__TVEXT:%d__TK_STARTMASK:0x%llx___TK_STARTVAL:0x%llx\n", TKTYPE,TVEXT,TK_STARTMASK,TK_STARTVAL);
	PrintDB(Bmom);
	*///DEBUG
	return 0;
}

int  SK_TK_PreInit(void)
{
	// Il T-K è impostato perchè all'avvio non fornisca tensione all'MTS, ed inoltre 
// tutti gli script di collaudo prevedono che alla fine la tensione venga tolta.
//
//	Si presuppone, inoltre che, l'MTS sia già stato "battezzato".
//
//	======================================================================
//	CONTROLLARE che il T-K sia STATO CONNESSO al PC !!!!!!!!
//	======================================================================
//
// -D-	MSGBOXCALL ("Assicurarsi che l'MTS NON sia connesso dal Test-Kit!");
//	Valore	    Mask	--------	--------
//	AL_OFF		AL_OFF	0x200000	0x200000	Spegne l'alimentatore switching del T-K (già previsto all'accensione del T-K nel firmware)
//	PRES_		PRES_	0x100000	0x100000	Toglie Presenza all'MTS
//	PON_		PON_	0x000080	0x000080	Toglie Alimentazione all'MTS
//	0x18		0x08	0x000018	0x000008	Imposta tutte le uscite del T-K come "flottanti" per gli INPUT dell'MTS
//	EN_CNT	0			0x000020	0x000000	Output-CNT di T-K fermati
//	0xF00		0x200	0x000F00	0x000200	Output di T-K disattivi
//	CHN			0x01	0x000007	0x000001	Abilito 7° canale analogico [= ~(6)] per non mandare tensione analog. all'MTS
//	---------------------------------
//								0x300FBF	0x300289
//
	SetProtocolMode(KPROTOCOLOMODE_QUERYANSWER);	// Imposta la modalità aggiornata degli analogici
	
	T_Output(TK_STARTMASK, TK_STARTVAL);		// Esegue le operazioni di reset del T-K descritte sopra
	if (TKTYPE==1) T_SetPull(0xFFFFFFFF, 0xFFFFFFFF);
	//Per prova NEWTRL2_ | NEWRELECAN 
	//
	//		T_Output( NEWRELECAN , 0);
	//		Delay(10);
	//		T_Output(NEWRELECAN, NEWRELECAN);
	//		Delay(10);
	//		T_Output(NEWRELECAN, 0);
	//		Delay(10);
	//		T_Output(NEWRELECAN, NEWRELECAN);
	//		Delay(10);
	//		T_Output(NEWRELECAN, 0);
	//		Delay(10);
	//		T_Output(NEWRELECAN, NEWRELECAN);
	//		Delay(10);
	//		T_Output(NEWRELECAN, 0);
	//		call_exit(YES, "FineTestnew");    ///GTGTGTGTGT/// Da Togliere
	//	}
	//Fine Per Prova
//	T_Output(0x300FBF, 0x300289);		// Esegue le operazioni di reset del T-K descritte sopra
//	T_Output (0x300FBF, 0x200289);		// Esegue le operazioni di reset del T-K descritte sopra ma METTE la presenza
// ----------------------------------------------------------------------------------------
//				Controllo LED ROSSO di Test-Kit     -->  T_Led(mask,period)
// ----------------------------------------------------------------------------------------
// - mask: pattern di 32 bit che verrà inviato a Led Rosso (bit a '1' = Led acceso, a '0' Led spento)
// - period: tempo di durata bit del pattern in ms (deve essere >10 e viene letto come decine di ms)
	T_SetFTDI(COM_OFF);	 				// Vengono disattivate le seriali
	T_Led(MSK_OFF, FRQ_OFF);			// Led Spento
	return 0;
}

int SK_TK_Init(void)
{
	char tkver[255] ;
	
	// avvia la ProgressBar con il tempo desiderato
	ProgressBar(BAR_TIME, tbar_time) ; // 100s x sendcode, 200s x collaudo
	SK_TK_PreInit();
	sprintf(bufwindow,"\r---->   VERSIONE di COLLAUDO %s: %s   <----\r", TkIni.mName, SCRREL);
	MsgWindow(bufwindow);
	
	// Legge sn e ver del TK
	T_GetVer(tkver) ;
	T_GetVer(tkver) ;
	T_GetVer(tkver) ;
	//if(tkver[0]=='\0') call_exit(YES,"ATTENZIONE!\rTESTKIT NON COMUNICA!!");
	sprintf(bufwindow,"\r---->   TestKit: %s   <----\r", tkver);
	MsgWindow(bufwindow);
	//MsgFile(1, LogCollaudo, bufwindow);

	QtaLog++; 
	strcpy(LogBuffer[QtaLog], bufwindow ) ;
	
	//retryOnError(2);					// imposta a 2 tentativi di richieste vs MTS
	retryOnError(4);					// imposta a 4 tentativi di richieste vs MTS
	return 0;
}

void LoadFamTab(void)
{
// inizializza la tabella delle Famiglie di MTS
	int i = 1 ;
	
	strcpy(MtsFam[i].mts,"1009"); 		MtsFam[i++].famcode = 10;		// AVR
	strcpy(MtsFam[i].mts,"2002"); 		MtsFam[i++].famcode = 2;		// AVR
	strcpy(MtsFam[i].mts,"2003"); 		MtsFam[i++].famcode = 2;		// AVR
	strcpy(MtsFam[i].mts,"2005"); 		MtsFam[i++].famcode = 2;		// AVR
	strcpy(MtsFam[i].mts,"2006");		MtsFam[i++].famcode = 2;		// AVR
	strcpy(MtsFam[i].mts,"2105"); 		MtsFam[i++].famcode = 2;		// AVR
                                    	
	strcpy(MtsFam[i].mts,"1001"); 		MtsFam[i++].famcode = 10;		// ARM7
	strcpy(MtsFam[i].mts,"1016"); 		MtsFam[i++].famcode = 10;		// ARM7
	strcpy(MtsFam[i].mts,"1109"); 		MtsFam[i++].famcode = 11;		// ARM7
	strcpy(MtsFam[i].mts,"2015"); 		MtsFam[i++].famcode = 201;		// ARM7
	strcpy(MtsFam[i].mts,"2020"); 		MtsFam[i++].famcode = 201;		// ARM7
	strcpy(MtsFam[i].mts,"2102"); 		MtsFam[i++].famcode = 210;		// ARM7
	strcpy(MtsFam[i].mts,"2106"); 		MtsFam[i++].famcode = 210;		// ARM7
	strcpy(MtsFam[i].mts,"2205"); 		MtsFam[i++].famcode = 210;		// ARM7
	strcpy(MtsFam[i].mts,"3008");		MtsFam[i++].famcode = 30;		// ARM7

	strcpy(MtsFam[i].mts,"MTS40A-B");	MtsFam[i++].famcode = 40;		// ARM9
	strcpy(MtsFam[i].mts,"MTS40C");		MtsFam[i++].famcode = 40;		// ARM9
	strcpy(MtsFam[i].mts,"4004");		MtsFam[i++].famcode = 40;		// ARM9
	strcpy(MtsFam[i].mts,"4010");		MtsFam[i++].famcode = 40;		// ARM9
	strcpy(MtsFam[i].mts,"4011");		MtsFam[i++].famcode = 40;		// ARM9
	strcpy(MtsFam[i].mts,"4104");		MtsFam[i++].famcode = 41;		// ARM9
	                                	            
	strcpy(MtsFam[i].mts,"MTS02");		MtsFam[i++].famcode = 2;		// HC12
	strcpy(MtsFam[i].mts,"MTS03");		MtsFam[i++].famcode = 3;		// HC12
	strcpy(MtsFam[i].mts,"MTS05");		MtsFam[i++].famcode = 5;		// HC12
	strcpy(MtsFam[i].mts,"MTS08");		MtsFam[i++].famcode = 8;		// HC12
	strcpy(MtsFam[i].mts,"MTS15");		MtsFam[i++].famcode = 15;		// HC12
	strcpy(MtsFam[i].mts,"MTS20");		MtsFam[i++].famcode = 20;		// HC12
	strcpy(MtsFam[i].mts,"MTS21");		MtsFam[i++].famcode = 21;		// HC12
	
	strcpy(MtsFam[i].mts,"2102C"); 		MtsFam[i++].famcode = 210;		// CORTEX
	strcpy(MtsFam[i].mts,"2205C"); 		MtsFam[i++].famcode = 210;		// CORTEX
	strcpy(MtsFam[i].mts,"2206C"); 		MtsFam[i++].famcode = 210;		// CORTEX

	strcpy(MtsFam[i].mts,"3108");		MtsFam[i++].famcode = 31;		// CORTEX
	strcpy(MtsFam[i].mts,"2022");		MtsFam[i++].famcode = 202;		// CORTEX
	strcpy(MtsFam[i].mts,"2122");		MtsFam[i++].famcode = 202;		// CORTEX
	
	strcpy(MtsFam[i].mts,"2020C");		MtsFam[i++].famcode = 212;		// CORTEX
	strcpy(MtsFam[i].mts,"2120");		MtsFam[i++].famcode = 212;		// CORTEX

	strcpy(MtsFam[i].mts,"3208");		MtsFam[i++].famcode = 32;		// CORTEX
	strcpy(MtsFam[i].mts,"2202");		MtsFam[i++].famcode = 22;		// CORTEX
	strcpy(MtsFam[i].mts,"2023");		MtsFam[i++].famcode = 203;		// CORTEX
	strcpy(MtsFam[i].mts,"2305");		MtsFam[i++].famcode = 23;		// CORTEX
	strcpy(MtsFam[i].mts,"2106C");		MtsFam[i++].famcode = 23;		// CORTEX
	strcpy(MtsFam[i].mts,"3025");		MtsFam[i++].famcode = 35;		// CORTEX
	strcpy(MtsFam[i].mts,"2034");		MtsFam[i++].famcode = 204;		// CORTEX
	strcpy(MtsFam[i].mts,"3033");		MtsFam[i++].famcode = 28;		// CORTEX
	strcpy(MtsFam[i].mts,"3035");		MtsFam[i++].famcode = 33;		// CORTEX
	strcpy(MtsFam[i].mts,"3036");		MtsFam[i++].famcode = 36;		// CORTEX
	strcpy(MtsFam[i].mts,"4037");		MtsFam[i++].famcode = 41;		// CORTEX A8
	strcpy(MtsFam[i].mts,"2039");		MtsFam[i++].famcode = 209;		// CORTEX
	strcpy(MtsFam[i].mts,"2040");		MtsFam[i++].famcode = 240;		// CORTEX
	strcpy(MtsFam[i].mts,"2046");		MtsFam[i++].famcode = 26;		// CORTEX
	strcpy(MtsFam[i].mts,"2044");		MtsFam[i++].famcode = 26;		// CORTEX
	strcpy(MtsFam[i].mts,"3048");		MtsFam[i++].famcode = 48;		// CORTEX
	strcpy(MtsFam[i].mts,"2047");		MtsFam[i++].famcode = 47;		// CORTEX
	strcpy(MtsFam[i].mts,"2045");		MtsFam[i++].famcode = 45;		// CORTEX  //47
	strcpy(MtsFam[i].mts,"2405");		MtsFam[i++].famcode = 47;		// CORTEX
	strcpy(MtsFam[i].mts,"2051");		MtsFam[i++].famcode = 51;		// CORTEX
	strcpy(MtsFam[i].mts,"2052");		MtsFam[i++].famcode = 52;		// CORTEX
	strcpy(MtsFam[i].mts,"2046_M4");		MtsFam[i++].famcode = 46;		// CORTEX 
	strcpy(MtsFam[i].mts,"2044_M4");		MtsFam[i++].famcode = 46;		// CORTEX
	strcpy(MtsFam[i].mts,"2047_M4");		MtsFam[i++].famcode = 147;		// CORTEX
	strcpy(MtsFam[i].mts,"2051_M4");		MtsFam[i++].famcode = 151;		// CORTEX

	strcpy(MtsFam[i].mts,"2054");		MtsFam[i++].famcode = 54;		// CORTEX	// Added 03/03/25
	strcpy(MtsFam[i].mts,"3055");		MtsFam[i++].famcode = 55;		// CORTEX	// Added 03/03/25

//	QtaFam = i-1;
	QtaFam = i ;
}

int ReadMTSType(char *dutM)
{
	int i;
	int trovato = 0;
	
	//sprintf(bufwindow,"Cerco famiglia %s", dutM);
	//MsgWindow(bufwindow);

	for (i=1; i<=QtaFam; i++) {
		if (strcmp(MtsFam[i].mts, dutM) == 0){
			trovato = MtsFam[i].famcode ;
			break;
		}
		//sprintf(bufwindow,">%s< >%s<", MtsFam[i].mts, dutM);
		//MsgWindow(bufwindow);
		
	}
	if (trovato<1){ // i <= QtaFam) {
//		trovato = MtsFam[i].famcode ;
//	} else {
		sprintf(bufwindow,">>>>>>> ATTENZIONE: per l'MTS %s non è definita la Famiglia!!", dutM);
		MsgWindow(bufwindow);
	}
	return trovato;
}


int ReadMTSArca(char *dutM)
{
	int i;
	
	for (i=1; i<=QtaArca; i++) {
		if (strncmp(Arca[i].mts, dutM, 4) == 0) return(i+1) ;
	}
	sprintf(bufwindow,">>>>>>> ATTENZIONE: per l'MTS %s manca definizione in Arca!!", dutM);
	MsgWindow(bufwindow);

	return 0;
}

int ReadMTSPIN(int dutM)
{
	int i;
	char buf[10];
	snprintf(buf, sizeof buf, "%d", dutM);
	
	for (i=1; i<=QtaRow; i++) {
		if (strncmp(TablePIN[i].snmts, buf, 9) == 0) return(i+1) ;
	}
	sprintf(bufwindow,">>>>>>> ATTENZIONE: per l'MTS %s manca parametri in file Ass_PIN!!", buf);
	MsgWindow(bufwindow);

	return 0;
}

float ReadMTSbatt(void){
int k ;
float vbatt ;

#ifndef SENDFW
	// Se in produzione (e richiesto) controlla se GSM comunica
	//if ((TestSet.GSMbaud) && (MTSdata.mSerial==-1)){
	if (TestSet.GSMbaud){
		M_Diag(TestSet.GSMbaud, 0, dDg); 					// Legge lo stato del sensore di vibrazione e ...			
		k = atoi(dDg) ;
		if (!k){ // Retry
			M_Diag(TestSet.GSMbaud, 0, dDg); 					// Legge lo stato del sensore di vibrazione e ...			
			k = atoi(dDg) ;
		}
		if (!k){	// No mts connected
			MSGBOXCALL("", "Nessun MTS connesso: STOP",1, "OK", 0, dDg);
			call_exit(NO, "NO_MTS") ;
		}
		k -= (5*65536) ;
		sprintf(MyDebB,"--> Uptime %d (%d)", k, TestSet.GSMbaud ) ;
		LoggaStampa(MyDebB);
	}
#endif // #ifndef SENDFW
	
	k = M_Analog(A_VBAT);
	// Diversificare a seconda dell'MTS !  _FR_ 
	switch (k &0xf){
		case 1 :
		vbatt = (float)k/64.0/1023.0*5.0; break ;				// Valore della tensione di batteria in V
		case 2:
		vbatt = 0.1 + (float)k/64.0/1023.0*5.0*3.553 ; break ;
		default:
		vbatt = 0.6 + (float)k/64.0/1023.0*5.0*3.56 ; break ;//  (anl / 65536#) * 5# * 3.56, "0.0") & " V"
	}
	return (vbatt) ;
	
}

void TesterMTS(void){
	int k;
	char lstrg[MINSIZE];
	float lVMTS, lCMTS ;
	lCMTS=0;
	lCMTS=lCMTS; // Per Compilatore Set but not used
#ifndef SENDFW
	float lVBatMTS ;
#endif // ifndef SENDFW
	
// Misura di consumi e tensioni di MTS	
	k = T_Analog(TVEXT);
	lVMTS = (float)k*3.0/1023.0*1056.0/56.0 ; // -0.2;	// Valore della tensione esterna in V
	k = T_Analog(PWR_C);
	lCMTS = (float)k*3000.0/1023.0/3.0;					// Valore della corrente assorbita da MTS in mA
	
#ifdef SENDFW
	sprintf (lstrg,"%5.2f", lVMTS);
	OuputText(8, lstrg, 2, "Volt", 0, 0);
	sprintf (lstrg,"%5.1f", lCMTS);
	OuputText(9, lstrg, 3, "mA", 0, 0);
	sprintf (MyDebB,"Tensione fornita all'MTS: %5.2f Volt", lVMTS);
	StampaDB (MyDebB,"");
	sprintf (MyDebB,"Corrente assorbita dall'MTS: %5.0f mA", lCMTS);
	StampaDB (MyDebB,"");
	return ;
#else // ifdef SENDFW

	if (lVMTS < TestSet.VextMin) {
		MSGBOXCALL("ATTENZIONE !!! Tensione fornita all'MTS TROPPO BASSA",0,2,0,0,bufresponse);
	}else{
	    if (lVMTS > TestSet.VextMax) {
		    MSGBOXCALL("ATTENZIONE !!! Tensione fornita all'MTS TROPPO ALTA",0,2,0,0,bufresponse);
		    T_Output(PON_, PON_);
	    }    
		/*
		k = T_Analog(PWR_C);
	    lCMTS = (float)k*3000.0/1023.0/3.0;					// Valore della corrente assorbita da MTS in mA

	    lVBatMTS = ReadMTSbatt() ;
	    
	    if (lCMTS < TestSet.CurrMin) {
	        
		    MSGBOXCALL("ATTENZIONE !!!\rAssorbimento TROPPO BASSO\r- VERIFICARE VBat -",0,2,0,0,bufresponse);
		    if (lVBatMTS > 4.10) 
		    MSGBOXCALL("ATTENZIONE BATTERIA CARICA\r--- INSERIRE UNA BATTERIA SCARICA E PREMERE O.K.---",0,2,0,0,bufresponse);
		    
	    }else{
	        if (lCMTS > TestSet.CurrMax){
		        sprintf(MyDebB,"ATTENZIONE !!!\rAssorbimento TROPPO ALTO (%5.2f mA)\rAlimentazione INTERROTTA!!", lCMTS);
		        T_Output(PON_, PON_);
		        MSGBOXCALL(MyDebB,0,2,0,0,bufresponse);
	        }
	    }
	    
	    // sprintf(lstrg,"%5.2f", lVMTS);
	    //sprintf(lstrg,"%5.1f", lCMTS);
	    //sprintf(MyDebB,"Tensione   fornita  all'MTS: %5.2f Volt", lVMTS);
	    //MsgWindow(MyDebB);
	    //sprintf(MyDebB,"Corrente assorbita dall'MTS: %5.1f mA", lCMTS);
	    //MsgWindow(MyDebB);
		*/
		lVBatMTS = ReadMTSbatt() ;
		sprintf(lstrg,"%5.2f", lVBatMTS);
	    OuputText(2,lstrg,0,"Vbat",0,0);
	    
	    
	}  
#endif // #ifndef SENDFW
}


void GetDate(char *data) {
	struct tm mytm ;
	time_t mytime ;
	
	mytime = time(NULL) ;
	LOCALTIME_R(&mytime, &mytm);					// time attuale
	sprintf(data, "%02d%02d%02d", mytm.tm_year%100, mytm.tm_mon+1, mytm.tm_mday);
}

void GetYear(char *data) {
	struct tm mytm ;
	time_t mytime ;
	
	mytime = time(NULL) ;
	LOCALTIME_R(&mytime, &mytm);					// time attuale
	sprintf(data, "%02d", mytm.tm_year%100);
}


#ifdef CBUG
void StampaDB(char * nome, char * testo)				// Su finestra DOS di Debug 
{
	char mystringa[MAXSIZE];
	
//	if (PrDeb==1) {
		//	si unisce nome a testo e si aggiunge CR al fondo
		sprintf(mystringa, "%s = %s", nome, testo);	
		MsgWindow(mystringa);
		//PrintDB(mystringa);
//	}
}

void StampaDBn(char * nome, int dato)					// Su finestra DOS di Debug 
{
	char mystringa[MAXSIZE];
//	if (PrDeb==1) {
		//	si unisce nome a dato e si aggiunge CR al fondo
		sprintf(mystringa, "%s: 0x%x (%d)", nome, dato, dato);		
		MsgWindow(mystringa);
		//PrintDB(mystringa);
//	}
}
#endif // #ifdef CBUG

#ifdef SENDFW
int Cortex_Reset(void)
{
	if ( 
	    (!strcmp(TkIni.mName, "2102")) 
	   )
		return 1;
	else
		return 0;
}

int64_t SK_SendCode()
{
	int JTAG_SEND=0; //JTAG_SEND 1 send fw to JTAG
	int tutOK, k, nuovo,r,kk,Errore_INVIO,swvermts;
	char checksum[64];
	int64_t check_checksum;
	long fsize ;
	double tsend ;
	char pgrPar[NRMSIZE] ;
	//FILE *fcode ;
	struct stat dstat ;

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	retryOnError(2);					// imposta a 2 tentativi di richieste vs MTS
	
	if (T_SetFTDI(MtsTK.COM1)){							// si abilita la COM1 del T-K
			sprintf(Bmom, "Errore durante aperturta COM MTS");
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
	}
	M_SetSourceId(MtsTK.mCOM);						// si imposta la COM di protocollo primario
	Delay(5);	
	nuovo=0;
	for (kk=0;kk<1;kk++){
		MTSdata.mSerial = M_GetSerNum();
		if (TKERROR==0) break ;
		Delay(1);		
	}
	MTSdata.mSerial = M_GetSerNum();				// si legge il Serial-Number
	if (TKERROR!=0) nuovo=1;
	if (nuovo==1) {
		// assegna un nome temporaneno al file di log: s/n MB)
		sprintf(LogCollaudo, "%s\\Logs\\Log(-1).txt", mRoot);
		CONVERTPATH(LogCollaudo);
	}else{
		sprintf(LogCollaudo, "%s\\Logs\\Log%d.txt", mRoot, MTSdata.mSerial);
		CONVERTPATH(LogCollaudo);
	}
	if (nuovo==1) {
		MTSdata.mSerial = -1 ;
		swvermts = 0;
		MTSdata.mSign = ReadMTSType(TkIni.mName) ;
		sprintf(bufwindow,"        Connessione stabilita con unità NUOVA");
		MsgWindow(bufwindow);
	}
	else {
		swvermts=M_GetSwVers();													// si legge la Versione Software
		if (TKERROR!=0) {
			swvermts = 0;
		}
		MTSdata.mSign = M_GetFamily();
		if (TKERROR!=0) {
			MTSdata.mSign = 0;
		}
		MTSdata.SVer = (float)swvermts/1000;
		sprintf(bufwindow,"        Connessione stabilita con MTS tipo %d, s.n. %d (Ver.%5.2f)", MTSdata.mSign, MTSdata.mSerial, MTSdata.SVer);
		MsgWindow(bufwindow);
		if (TestSet.EnPIN_Param == YES) {
			LoadTabPINAssociative();
			MtsTK.PinPos = ReadMTSPIN(MTSdata.mSerial);
			if(MtsTK.PinPos>0){
				if (TablePIN[MtsTK.PinPos-1].ICCID[0]!='\0'){
					//Spengo Modem
					M_Action(32,4,"") ; 	// Spengo Modem
					//Controllo ICCID
					int valido = 0; 
					sprintf(bufmsg, "Inserire ICCID x %s",TablePIN[MtsTK.PinPos-1].extname); 
					while (valido == 0) {
						// richiesta codice ICCID (es. 8939010001362772051)
						INPUTBOXCALL(bufmsg, "ICCID",2,"Continua","Ferma", HwSN.codICCID) ;		// richiesta codice ICCID
						if (strcmp(HwSN.codICCID,"#!")==0) {
							//COLOR_STEP(MtsTK.steptest, C_RED) ;
							call_exit(YES, "C_NO_ICCID");
						}
						sprintf(bufmsg , "ERRORE:\r Reinserire ICCID.");
						UpperAlfaNum(HwSN.codICCID);					// si rende Maiuscolo e solo AlfaNumerico (oltre '_')
						RLTrimm(HwSN.codICCID);						// toglie i caratteri non alfanumerici prima e dopo !
						if (strlen(HwSN.codICCID) == LEN_ICCID) {		// a questo punto DEVE essere di lunghezza == di'LEN_ICCID'
							StampaDB("ICCID inserito",HwSN.codICCID);		// Su finestra DOS di Debug 
							StampaDBn("valido",valido);			// Su finestra DOS di Debug 
							strcpy(Bmom, HwSN.codICCID);							// Bmom (cioè ICCID) deve avere ...
							if (strspn(Bmom, NUMCHAR) == strlen(HwSN.codICCID)){		// ... solo cifre  ...
									// si controlla che il codice Arca corrisponda al tipo di MTS selezionato per il test
									if (strncmp(TablePIN[MtsTK.PinPos-1].ICCID, HwSN.codICCID,LEN_ICCID) != 0) {
										sprintf(bufmsg, "ATTENZIONE:\r la SIM inserita non corrisponde nel DB!");
										MSGBOXCALL(bufmsg,0,1,"ATTENZIONE",0,bufresponse);
										call_exit(YES, "C_NO_ICCID");
									}else{
										valido++;
									}
								StampaDB("codICCID",HwSN.codICCID);			// Su finestra DOS di Debug 
							}
						}
					}
					sprintf(MyDebB, "codICCID inserito:<%s>", HwSN.codICCID);
					MsgFile(0, LogCollaudo, MyDebB);
					MsgWindow (MyDebB);
				}

				sprintf(bufmsg, "ATTENZIONE:\r Etichettare MTS con NOME %s",TablePIN[MtsTK.PinPos-1].extname);
				MSGBOXCALL(bufmsg,0,1,"ATTENZIONE",0,bufresponse);	
			}
		}
	}
	if (TestSet.EnSendFW == YES) {
		sprintf (buflog,"\n-----------------  Invio Codice  ------------ %s --", SCRREL);
		MsgFile(1, LogCollaudo, buflog);
		MsgFile(0, LogCollaudo, bufwindow); 
		T_Led(MSK_OFF, FRQ_OFF);													// Si spegne il LED per indicare "NO test"
		if (MTSdata.mSign != ReadMTSType(TkIni.mName)){
			sprintf(bufmsg,"MTS diverso da %s", TkIni.mName) ;
			MSGBOXCALL(bufmsg, "INVIO CODICE",2, "Invio codice", "STOP", bufresponse);
			if (strcmp(bufresponse,"#!")==0) 	call_exit(YES, "NO_Prog");
		}
		sprintf(MyDebB, "Check FW VER:%d[%d]", (atoi(MtsTK.mFwVer)*10),swvermts);
		MsgWindow (MyDebB);
		if ((atoi(MtsTK.mFwVer)*10)==swvermts){
				TestSet.EnSendFW = NO;
				sprintf(MyDebB, "Firmware is Update");
				MsgFile(0, LogCollaudo, MyDebB);
				MsgWindow (MyDebB);
		}
	}
	COLOR_STEP(MtsTK.steptest++, C_GREEN ) ;

//	======================================================================
//	Invio del Codice
//	======================================================================


	
	
	T_Led(MSK_FW, FRQ_FW) ;									// Lampeggia il LED + in fretta per indicare "Programmazione in corso"
	if (TestSet.EnSendFW == YES) {
		COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
		if (nuovo==0) TesterMTS();
		if (TestSet.ASKFW == YES) {
			MSGBOXCALL("Versione di Produzione o Debug ", "Tipo di CODICE",2, "PRODUZ", "DEBUG", bufresponse);
			if (strcmp(bufresponse,"##")==0) {
				sprintf(Bmom, "%s", "PROD");
			}else{
				sprintf(Bmom, "%s", "CBUG");
			}														//   "3008\\Versioni\\", 
		}else{
			sprintf(Bmom, "%s", "PROD");
		}
		// Legge la dimensione del file

		char extfilename[10] = "hex";
		
		if(JTAG_SEND==1) {
			strcpy(extfilename,"bin");
		}
		
		sprintf(pgrPar, "%s/%s%s.%s.%s", mVer, TkIni.NamFile, MtsTK.mFwVer, Bmom,extfilename);			
		CONVERTPATH(pgrPar) ;
		StampaDB("FW Send", pgrPar) ;
		stat(pgrPar, &dstat ) ;
		fsize = dstat.st_size ;
		
		// Avvia la progress bar
		tsend = 0.003175 * ((double)(fsize)) ;
		k = (tsend +15) ;
		ProgressBar(BAR_TIME, k) ;

		if(JTAG_SEND==1) {
			// ! Path relativa !           //Prima PATH_FW
			sprintf(pgrPar, "%s%s%s%s%s %s", WSpace, TkIni.NamTyp, "\\\\Versioni\\\\", TkIni.NamFile, MtsTK.mFwVer, Bmom);
			CONVERTPATH(pgrPar) ;
			StampaDB("Sorgente:", pgrPar) ;
		}
		sprintf(buflog,"Inizio invio di %s%s %s  . . .", TkIni.NamFile, MtsTK.mFwVer, Bmom);
		MsgFile(0, LogCollaudo, buflog);
		//INVIO CODICE
		if(JTAG_SEND==1) {
			sprintf(Bmom, "sendver%sTK%d.bat", TestSet.Cpu,TKTYPE ) ;
			if (TKTYPE==1) T_SetFTDI("160");                // Per settare NewTK su JTAG
			tutOK = DoProgram(PathFwUp, Bmom, pgrPar);		// si invia il codice all'MTS
		}else{
			//GetINIKeyVal("FWFile", TkIni.NamFW);
			//togliCR(TkIni.NamFW);
			sprintf(FWSel, "%s", pgrPar);
			CONVERTPATH(FWSel);
			StampaDB("FW_FILE:",FWSel);	
			//fcode = fopen(FWSel, "r");	
			M_PutFWFile(FWSel,checksum);		// si invia il codice all'MTS via Seriale
			sprintf(Bmom, "Receive CheckSum_String 0x%s",checksum);
			PrintDB(Bmom);
			check_checksum=stringascii_to_hex(checksum);
			if (check_checksum != 0) {
				//TKERROR=0; 
				tutOK=1;
				sprintf(Bmom, "Receive CheckSum_OK 0x%lx",check_checksum);
				PrintDB(Bmom);
			};
		}

		//INVIO CODICE
		//tutOK = 1;
		if ( (TKERROR != 0) || ( tutOK != 1)  ) {
			StampaDB("Invio - TK-Error", "NO good");
			MsgFile(0, LogCollaudo, "   . . . Errore invio Codice");
			k=0;
			MTSdata.mSign = ReadMTSType(TkIni.mName) ;
			Errore_INVIO=1;
		}else{
			ProgressBar(BAR_TIME, 400) ; // 40 sec
			COLOR_STEP(MtsTK.steptest++, C_GREEN ) ;
			MsgFile(0, LogCollaudo, "   . . . Codice inviato");
			Errore_INVIO=0;
			TesterMTS();	
			if(JTAG_SEND==1) {
				if (T_SetFTDI(MtsTK.COM1)){							// si abilita la COM1 del T-K
					sprintf(Bmom, "Errore durante aperturta COM MTS");
					PrintDB(Bmom);
					call_exit(YES, Bmom);
				}
				
			}
			GetINIKeyVal("Protocol", TkIni.Protocol);
			togliCR(TkIni.Protocol);
			UpperAlfaNum(TkIni.Protocol);			// si converte tutto in maiuscolo
			RLTrimm(TkIni.Protocol);				// e si tolgono eventuali caratteri non alfanumerici prima e dopo
			RLTrimmwithplace(TkIni.Protocol); // e si tolgono eventuali caratteri non alfanumerici prima e dopo
			sprintf(Bmom, "INI PROTOCOL:[%s]",TkIni.Protocol);
			PrintDB(Bmom);
			if (!strncmp(TkIni.Protocol,"TEST",4)) 
				SetProtocolComunication(1);	// Setta Protocollo TEST
			else
				SetProtocolComunication(0);	// Setta Protocollo WAY
			Delay(10);

			M_SetSourceId(MtsTK.mCOM);									// si imposta la COM di protocollo primario

			for (kk=0;kk<15;kk++){	
				r = M_GetSerNum() ;
				if (r==MTSdata.mSerial) break ;
			}
			
			// Aggiunta per CORTEX: tolgo 3.3v alla CPU
			if (Cortex_Reset()){
				T_Output (CPU_OFF, CPU_OFF);
				Delay(20);
				T_Output (CPU_OFF, 0);
				StampaDB("Reset 3.3v CPU", "per 2 sec");
				MsgFile(0, LogCollaudo, "Reset 3.3v CPU per 2 sec");
				T_Output(PON_, PON_);										// Si toglie presenza all'MTS 
				for (kk=0;kk<15;kk++){	
				r = M_GetSerNum() ;
				if (r!=MTSdata.mSerial) break ;
				}
				T_Output(PON_, 0);											// Si rida' presenza all'MTS 
				Delay(15);
				for (kk=0;kk<15;kk++){	
					r = M_GetSerNum() ;
					if (r==MTSdata.mSerial) break ;
				}
			}
			// Aggiunta per CORTEX: tolgo 3.3v alla CPU
		}
	}else{
		Errore_INVIO=0;
		TKERROR=0;
	}
	//Invio CAN-BUS
	if (TestSet.EnCANConf == 1) {
		COLOR_STEP(MtsTK.steptest, C_YELLOW ) ;
		SK_CanConfSet();
		COLOR_STEP(MtsTK.steptest++, C_GREEN ) ;
	}

	
	if (TestSet.EnSMset == YES) {
		COLOR_STEP(MtsTK.steptest, C_YELLOW ) ;
		//Invio Parametri
		SK_ParamSet();
		COLOR_STEP(MtsTK.steptest, C_RED ) ;
		Delay(20);
		SK_ParamSetCheck();
		COLOR_STEP(MtsTK.steptest++, C_GREEN ) ;

		if (TestSet.EnPIN_Param == YES) {
				//Setto Parametro PIN MTS
				if (MtsTK.PinPos!=0) {
					if (TablePIN[MtsTK.PinPos-1].PIN[0]!='\0'){
						M_SetPar(169,TablePIN[MtsTK.PinPos-1].PIN);
					}
					if (TablePIN[MtsTK.PinPos-1].centoottantacinquepar[0]!='\0'){
						M_SetPar(185,TablePIN[MtsTK.PinPos-1].centoottantacinquepar);
					}
				}
		}

		//Invio Macchina-Stati
		COLOR_STEP(MtsTK.steptest, C_YELLOW ) ;
		SK_SM_StateSet();
		Delay(20);
		if (T_SetFTDI(MtsTK.COM1)){							// si abilita la COM1 del T-K
			sprintf(Bmom, "Errore durante aperturta COM MTS");
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
		}
		Delay(10);
		if (T_SetFTDI(MtsTK.COM1)){							// si abilita la COM1 del T-K
			sprintf(Bmom, "Errore durante aperturta COM MTS");
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
		}
		/*
		if(SK_SM_StateSetCheck())	{ 
			Errore_INVIO=1;
			COLOR_STEP(MtsTK.steptest++, C_RED ) ;
		}else{
		*/
			COLOR_STEP(MtsTK.steptest++, C_GREEN ) ;		
		//}
	}
	
	COLOR_STEP(MtsTK.steptest, C_YELLOW ) ;	
	for (kk=0;kk<15;kk++){	
			r = M_GetSerNum() ;
			if (r==MTSdata.mSerial) break ;
	}

	if(r!=MTSdata.mSerial) Errore_INVIO=1;

	MTSdata.mSerial = M_GetSerNum();							// si rilegge il Serial-Number
	if (TKERROR == 0) {
		tutOK = 0;												// s/n letto correttamente
		k=M_GetSwVers();										// si rilegge la Versione Software
		MTSdata.mSign = M_GetFamily();
		MTSdata.SVer = (float)k/1000;
		if (TestSet.EnSendFW == YES) {
			M_Diag(3, 0, dDg); //Leggo CheckSum
			int64_t diag3;
			diag3=stringascii_to_hex(dDg);
			sprintf(Bmom, "Diag 3 CheckSum 0x%lX",(diag3>>8));
			PrintDB(Bmom);
			if ( check_checksum != (diag3>>8) ) Errore_INVIO =1;
			sprintf(MyDebB, "Check FW VER:%d[%d]", (atoi(MtsTK.mFwVer)*10),k);
			MsgWindow (MyDebB);
			if ((atoi(MtsTK.mFwVer)*10)==k) Errore_INVIO =0;
		}
	}else{
		MTSdata.mSerial = 0 ;
		k=0;
		MTSdata.mSign = ReadMTSType(TkIni.mName) ;
	}

	if (TestSet.EnPIN_Param == YES) {
			//Si Legge CSQ per Verifica PIN
			if (MtsTK.PinPos!=0){
				if (TablePIN[MtsTK.PinPos-1].PIN[0]!='\0'){
					DatoMTS = M_ReadIntAD();
					Delay(10);
					MTSdata.ValoreCSQ = DatoMTS.csq ;	
#ifdef CBUG
					sprintf(MyDebB,"CSQ = %d", MTSdata.ValoreCSQ ) ;
					MsgWindow(MyDebB);
#endif // #ifdef CBUG
		
				if ( MTSdata.ValoreCSQ == 99 ){	
					sprintf(bufmsg, "ATTENZIONE:\r Modem Non Registrato!");
					MSGBOXCALL(bufmsg,0,1,"ATTENZIONE",0,bufresponse);
					call_exit(YES, "NO_PIN");
				}
			}
		}
	}

	ProgressBar(BAR_PERC, 100) ;
	
	if (Errore_INVIO == 0){
		return check_checksum;
		sprintf(bufwindow, "Aggiornamento completato ---> MTS tipo %d, s.n. %d (Ver.%5.2f) CheckSum(0x%lX)", MTSdata.mSign, MTSdata.mSerial, MTSdata.SVer,check_checksum);
		COLOR_STEP(MtsTK.steptest, C_GREEN ) ;
	}else{
		sprintf(bufwindow, "Invio codice E R R A T O ---> MTS tipo %d, s.n. %d (Ver.%5.2f)", MTSdata.mSign, MTSdata.mSerial, MTSdata.SVer);
		COLOR_STEP(MtsTK.steptest, C_RED ) ;
	}
	MsgWindow(" ");
	MsgWindow(bufwindow);
	MsgFile(0, LogCollaudo, bufwindow);
	
	//MSGBOXCALL(bufwindow, "Esito Invio",1, "OK", 0, bufresponse);
	return(0);
}


#else

float ReadAnalog(int channelId) 
{
	float Valore = 0;
	
	if (channelId == 5) {							// il 6° analogico esterno è letto in 8° posizione !!!!!!!!!
		channelId = 7 ;
	}
	Valore = M_Analog(channelId+1)/64.0 ;
	
	return Valore;									// ritorna il valore analogico da 0 a 1023 !!!!!!!
}

#endif // ifndef SENDFW
void LoggaStampa(char *msg)
{
	char LocalStr[MAXSIZE];
	
	if (strlen(LogCollaudo)) MsgFile(0, LogCollaudo, msg);
	sprintf(LocalStr,"%s", msg);
	MsgWindow(LocalStr);
}

void togliCR(char *msg)
{
	int i;
	
	for(i=0 ; msg[i] ; i++){ 
		if (msg[i] < ' ') {							// se è un carattere di controllo ...
			msg[i] = '\0';							// ... lo si sostituisce con terminatore di stringa
			break;
		}
	}
}
#ifndef SENDFW

//	======================================================================
//	INIZIO del Collaudo
//	======================================================================
//		- Accensione di MTS (Alimentazione e Presenza)
//		- Analisi funzionalità della Presenza
//		- Impostazione di COM di protocollo

int SK_PowerOn(void)
{
	
	MSGBOXCALL("Connettere l'MTS al Test-Kit con SIM inserita!", "Avvio del Collaudo",2, "Avvia", "Ferma", bufresponse);
	if (strcmp(bufresponse,"#!")==0) {
		call_exit(YES, "C_MTS_Stop");
	}
	
	
	ProgressBar(BAR_TIME, TestSet.BootTime+tbar_time) ; // 100s x sendcode, 200s x collaudo

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	if (!strcmp(TkIni.mName,"MTS02")) T_Output(PRES_, 0);		// Si accende l'MTS02 ... attivando la Presenza ...

	T_Led(MSK_TEST, FRQ_TEST);					// pulsa il LED per indicare "test iniziato"

	if (T_SetFTDI(MtsTK.COM1)){							// si inizia ad abilitare la COM1 del T-K
			sprintf(Bmom, "Errore durante aperturta COM MTS");
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
	}
	T_Output(PON_, 0);							// Si da' tensione all'MTS
	Delay(3);
	M_SetSourceId(MtsTK.mCOM);					// si imposta la COM di protocollo primario
	//???????????
	//MTSdata.mSerial = M_GetSerNum();
	int k,r ;
	//for (k=0;k<20;k++){	
	//	r = M_GetSerNum() ;
	//	if (r!=MTSdata.mSerial) break ;
	//}
	// Delay(40); //20 Prima per ATTENDERE SPEGNIMENTO MTS	// ... ed attendo che si rispenga 							
	 								
	
// si verifica che la Presenza non sia "bloccata" attiva ("0") all'interno dell'MTS o del cablaggio
// quando la Presenza non è asserita dal T-K, si deve leggere PR_RD_ a "1"
	
	// Per le unità che hanno una 'presenza' fissa in HW o NON leggibile
	if  ((MtsTK.MonoPiastra) || (!TestSet.EnPres) )
		MtsTK.presOFF = 1;						
	else 
		MtsTK.presOFF = ((T_Input() & PR_RD_) == PR_RD_); 	// si considera "presOFF" un booleano
		
	T_Output(PRES_, 0);								// Si accende l'MTS ... attivando la Presenza ...
	Delay(15);
	for (k=0;k<150;k++){							// ... ed attendo che finisca il bootstrap
		T_Output(PRES_, 0);								// Si accende l'MTS ... attivando la Presenza ...
		Delay(1);
		T_Output(PON_, 0);							// Si da' tensione all'MTS
		Delay(1);
		r = M_GetSerNum() ;
		Delay(1);
		if (r != '\0') break ;
	}
	//Delay(TestSet.BootTime-15);						// ... ed attendo che finisca il bootstrap
	
// Primo test:
//	Controllo della Presenza tra conn. I/O e conn. Seriale dell'MTS
// 	con MTS ACCESO si deve leggere PR_RD_ a "0"
	if  ((MtsTK.MonoPiastra) || (!TestSet.EnPres) )
		MtsTK.presON=1;				// per le unità che hanno una 'presenza' hardware da testare perchè fissa
	else
		MtsTK.presON = !(T_Input() & PR_RD_);	// si considera "presON" un booleano
	//Test Presenza Secondo Connettore:
	MtsTK.pres2OFF = 1;
	MtsTK.pres2ON = 1;
	if (!strcmp(TkIni.mName, "3048")) {
		//Disabilito uscita 1 TestKit
		T_Output(TOD1,0);
		//Leggo Presenza prima di settare
		Delay(10);
		DatoMTS = M_ReadIntAD();
		Delay(10);
		StampaDBn("Ingressi extin (0x00)", DatoMTS.extin);
		MtsTK.pres2OFF=(!((DatoMTS.extin & 0x20) == 0x20)); // si considera "pres2OFF" un booleano
		//Setto uscita 1 del TeskKit per asserrire massa a Presenza
		//Abilito uscita 1 TestKit
		T_Output(TOD1,TOD1);
		Delay(10);
		DatoMTS = M_ReadIntAD();
		Delay(10);
		StampaDBn("Ingressi extin (0x00)", DatoMTS.extin);
		MtsTK.pres2ON= ((DatoMTS.extin & 0x20) == 0x20); // si considera "pres2ON" un booleano
		//Disabilito uscita 1 TestKit
		T_Output(TOD1,0);
		////////////////////////////////////////////////
		/////////FORZATO A UNO MANCA GESTIONE IN MTS3048
		MtsTK.pres2OFF = 1;
		MtsTK.pres2ON = 1;
		////////////////////////////////////////////////
	}
	/*
	if ( (!strcmp(TkIni.mName, "3035")) || (!strcmp(TkIni.mName, "3036")) ) {
		//Disabilito uscita 4 TestKit
		T_Output(TOD4,0);
		//Leggo Presenza prima di settare
		Delay(10);
		DatoMTS = M_ReadIntAD();
		Delay(10);
		StampaDBn("Ingressi extin (0x00)", DatoMTS.extin);
		MtsTK.pres2OFF=(!((DatoMTS.extin & 0x20) == 0x20)); // si considera "pres2OFF" un booleano
		//Setto uscita 4 del TeskKit per asserrire massa a Presenza
		//Abilito uscita 4 TestKit
		T_Output(TOD4,TOD4);
		Delay(10);
		DatoMTS = M_ReadIntAD();
		Delay(10);
		StampaDBn("Ingressi extin (0x00)", DatoMTS.extin);
		MtsTK.pres2ON= ((DatoMTS.extin & 0x20) == 0x20); // si considera "pres2ON" un booleano
		//Disabilito uscita 4 TestKit
		T_Output(TOD4,0);
	}	*/
	return 0;
}


//
//	======================================================================
//			Si controlla identità dell'MTS
//	======================================================================
//
int SK_CheckId(void)
{
	int  i, k, valido, Tens_R, Cur_R ;
	int pProd, nr_ss ;
	char MexShow[NRMSIZE];
	char mstr1[MINSIZE], mstr2[MINSIZE];
	//char pgrPar[NRMSIZE] ;

	//M_SetSourceId(MtsTK.mCOM);				// si imposta la COM di protocollo primario
	//Delay(15);  
	
	// Req uptime
	M_Diag(2, 5, dDg); 							// Legge lo stato del sensore di vibrazione e ...	
	k = atoi(dDg) ;
	k -= (5*65536) ;
	sprintf(MyDebB,"--> Uptime %d", k ) ;
	LoggaStampa(MyDebB);
	
	TesterMTS(); 
//												si assume che l'MTS abbia già bootstrap-pato
	Tens_R= T_Analog(TVEXT);					// lettura della V esterna ( = k*3/1023*1056/56+0.1). 170 = 9.5Volt
	Cur_R = T_Analog(PWR_C);					// in presenza di tensione esterna si misura la corrente ...
	if ((Cur_R<5) && (Tens_R>170)) {			// ... che deve essere >= di 5mA
		MSGBOXCALL("l'MTS NON si è acceso!", "Basso Consumo",2, "Continua", "Ferma", bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "C_MTS_Stop");
		}
	}
	MTSdata.mSerial = M_GetSerNum();					// si chiede il Serial-Number
	MtsTK.firstRun = 0 ;
//												si controlla che l'MTS sia in grado di rispondere
	if (TKERROR != 0) {							// se non risponde potrebbe essere NON programmato !!!
		MtsTK.firstRun = 1 ;
		sprintf(bufmsg, "%s NON Programmato\r(o MAL Programmato)!", TkIni.mName);
		//MSGBOXCALL(bufmsg, "NESSUNA Connessione",2, "Programma", "Ferma", bufresponse);
		MSGBOXCALL(bufmsg, "NESSUNA Connessione",2, "Continua", "Ferma", bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "C_MTS_NO_Prog");
		}else{ 									// si chiedono i codici di produzione delle Boards
			//Prima lanciava prog
			MTSdata.mSerial =  M_GetSerNum();							// ... si legge il Serial-Number
			
		}
	}else{	// _FR_ Verifica la versione nell'MTS: se precedente richiede se aggiornarlo
		// 
	}
	
	sprintf (Bmom, "s/n %d: Preparazione al COLLAUDO", MTSdata.mSerial );
	OuputText(1,Bmom,2,0,1,7);
	if (!MTSdata.mSerial) MTSdata.mSerial = -1 ;
	MtsTK.nuovo = (MTSdata.mSerial==-1); 										
	// se l'MTS è nuovo (sn=-1) -> "nuovo" == 'Vero'//
	if (MtsTK.nuovo) {												
	// si chiedono i codici di produzione delle Boards
	// *******************************************************************************

//		controllo di inserimento corretto di Codice ARCA
		valido = 0; 
		sprintf(MexShow, "Inserire Codice T.E.S.T."); 
		while (valido == 0) {
			// richiesta codice T.E.S.T. (es. per i 3008= 100184B)
			INPUTBOXCALL(MexShow, "Codice T.E.S.T.", 2,"Continua","Ferma", HwSN.codTEST);	
			if (strcmp(HwSN.codTEST,"#!")==0) {
				call_exit(YES, "C_NO_Codici");
			}
			sprintf(MexShow, "ERRORE:\r Reinserire Codice T.E.S.T.");
			if (strlen(HwSN.codTEST) >= LEN_ARCA) {
				UpperAlfaNum(HwSN.codTEST);							// si rende Maiuscolo e solo AlfaNumerico (oltre '_')
				RLTrimm(HwSN.codTEST);								// toglie i caratteri non alfanumerici prima e dopo !
				if (strlen(HwSN.codTEST) == LEN_ARCA) {				// a questo punto DEVE essere di lunghezza 'LEN_ARCA'
					if (strspn(HwSN.codTEST, NUMCHAR) == LEN_ARCA-1){	// se sono cifre tutti tranne l'ultimo carattere ...
						if (isalpha(HwSN.codTEST[LEN_ARCA-1])) {		// ... e se l'ultimo carattere è una lettera 
							StampaDB("codTEST inserito",HwSN.codTEST);	// Su finestra DOS di Debug 
// ricerca del codice ARCA nell tabella Prodotti	
							for (i=1; i<=QtaArca; i++) { 
								if (strcmp(HwSN.codTEST, Arca[i].cod) ==0) break;
							}
							StampaDBn("Item riconosciuto", i);	// Su finestra DOS di Debug 
							if (i <= QtaArca) {
// si controlla che il codice Arca corrisponda al tipo di MTS selezionato per il test
								if (strncmp(Arca[i].mts, TkIni.mName,4) != 0) {
									sprintf(bufmsg, "ATTENZIONE:\r %s NON è un MTS tipo %s!", Arca[i].mts, TkIni.mName);
									MSGBOXCALL(bufmsg,0,1,"Altro Codice",0,bufresponse);
								}else{
									pProd =i;			// pProd è l'indice del prodotto nella tabella Prodotti
									valido++;
								}
							}
						}
					}
				}
			}
		}
		valido = 0; 
		sprintf(MexShow, "Inserire s/n MotherBoard: %s.....", Arca[pProd].board); 
		while (valido == 0) {
			INPUTBOXCALL(MexShow, "S/N Mother Board",2,"Continua","Ferma", HwSN.codMB) ;		// richiesta codice mother board
			if (strcmp(HwSN.codMB,"#!")==0) {
				COLOR_STEP(MtsTK.steptest, C_RED) ;
				call_exit(YES, "C_NO_Codici");
			}
			sprintf(MexShow , "ERRORE:\r Reinserire s/n MotherBoard: %s.....", Arca[pProd].board);
			UpperAlfaNum(HwSN.codMB);					// si rende Maiuscolo e solo AlfaNumerico (oltre '_')
			RLTrimm(HwSN.codMB);						// toglie i caratteri non alfanumerici prima e dopo !
			if (strlen(HwSN.codMB) <= LEN_NUMHW) {		// a questo punto DEVE essere di lunghezza <= di'LEN_NUMHW'
				StampaDB("codMB inserito",HwSN.codMB);		// Su finestra DOS di Debug 
				StampaDBn("valido",valido);			// Su finestra DOS di Debug 
				strcpy(Bmom, HwSN.codMB);							// Bmom (cioè codMB) deve avere ...
				if (strspn(Bmom, NUMCHAR) == strlen(HwSN.codMB)){		// ... solo cifre  ...
					sprintf(mstr1, "%s", ""); 
					sprintf(mstr2, "0");
					for (i=1; i<=(5-strlen(HwSN.codMB)); i++) {		// si aggiungono gli "0" mancanti davanti al s/n ...
						strncat(mstr1, mstr2, 1);
					}
					strncat(mstr1, Bmom, strlen(HwSN.codMB));
					sprintf(HwSN.codMB, "%s%s", Arca[pProd].board, mstr1);	// per ottenere il codice completo
					valido++;
					StampaDB("codMB",HwSN.codMB);			// Su finestra DOS di Debug 
				}
			}else if (strlen(HwSN.codMB) == LEN_CDHW){
				if (!strncmp(HwSN.codMB, Arca[pProd].board, LEN_IDHW)){
					StampaDB("codMB inserito",HwSN.codMB);		// Su finestra DOS di Debug 
					StampaDBn("valido",valido);			// Su finestra DOS di Debug 
					valido++;
					StampaDB("codMB",HwSN.codMB);			// Su finestra DOS di Debug 				
				}
			}
		}
		QtaLog++; 
		sprintf(LogBuffer[QtaLog], "\rcodMB inserito:<%s>", HwSN.codMB);
		MsgWindow(LogBuffer[QtaLog]);

		
		for(nr_ss=0;nr_ss<5;nr_ss++) HwSN.codSV[nr_ss][0]='\0' ;	// Inizializzo
		
// Se si sta collaudando un MTS mono-piastra (1001, 1016, 1009, 1109, 2003)
		if (MtsTK.MonoPiastra) {
			//sprintf(HwSN.codSV, "%s", "");								// NON c'è la scheda di servizio
			if (strcmp(Arca[pProd].srvcard[0], "NA") != 0) {
				sprintf(bufwindow,"Errore in >> ModelliARCA.txt << -> %s NON puo' usare SUBMODULI (%s)",
																					 Arca[pProd].cod, Arca[pProd].srvcard[0]);
				MsgWindow(bufwindow);
			}
		} else {
			for(nr_ss=0;nr_ss<Arca[pProd].nr_aSV;nr_ss++){
				if (strcmp(Arca[pProd].srvcard[nr_ss], "NA") == 0) {
					sprintf(bufwindow,"Errore in >> ModelliARCA.txt << -> Per %s DEVE esserci %d SUBMODULO/I", 
																						Arca[pProd].cod, Arca[pProd].nr_aSV);
					MsgWindow(bufwindow);
				} else {
					valido = 0; 
					sprintf(MexShow, "Inserire s/n Scheda Servizi: %s.....", Arca[pProd].srvcard[nr_ss]); 
					while (valido == 0) {
						// richiesta codice scheda servizi
						INPUTBOXCALL(MexShow, "S/N Scheda Servizi", 2,"Continua","Ferma", HwSN.codSV[nr_ss]); 		
						if (strcmp(HwSN.codSV[nr_ss],"#!")==0) {
							COLOR_STEP(MtsTK.steptest, C_RED) ;
							call_exit(YES, "C_NO_Codici");
						}
						sprintf(MexShow, "ERRORE:\r Reinserire s/n Scheda Servizi: %s.....", Arca[pProd].srvcard[nr_ss]);
						UpperAlfaNum(HwSN.codSV[nr_ss]);					// si rende Maiuscolo e solo AlfaNumerico (oltre '_')
						RLTrimm(HwSN.codSV[nr_ss]);						// toglie i caratteri non alfanumerici prima e dopo !
						if (strlen(HwSN.codSV[nr_ss]) <= LEN_NUMHW) {		// a questo punto DEVE essere di lunghezza 'LEN_NUMHW'
							strcpy(Bmom, HwSN.codSV[nr_ss]);				// Bmom (cioè HwSN.codSV) deve avere ...      
							if (strspn(Bmom, NUMCHAR) == strlen(HwSN.codSV[nr_ss])){			// ... solo cifre  ...                    
								sprintf(mstr1, "%s", "");
								sprintf(mstr2, "0");
								// si aggiungono gli "0" mancanti davanti al s/n ...
								for (i=1; i<=(5-strlen(HwSN.codSV[nr_ss])); i++) {
									strncat(mstr1, mstr2, 1);
								}
								strncat(mstr1, Bmom, strlen(HwSN.codSV[nr_ss]));
								sprintf(HwSN.codSV[nr_ss], "%s%s", Arca[pProd].srvcard[nr_ss], mstr1);	// per ottenere il codice completo
								valido++;
								StampaDB("codSV",HwSN.codSV[nr_ss]);			// Su finestra DOS di Debug 
							}
						}else if (strlen(HwSN.codSV[nr_ss]) == LEN_CDHW){
							if (!strncmp(HwSN.codSV[nr_ss], Arca[pProd].srvcard[nr_ss], LEN_IDHW)){
								valido++;
								StampaDB("codSV",HwSN.codSV[nr_ss]);			// Su finestra DOS di Debug 
							}
						}
					}
					QtaLog++; 
					sprintf(LogBuffer[QtaLog], "\rcodSV%d inserito:<%s>", nr_ss, HwSN.codSV[nr_ss]);
					MsgWindow(LogBuffer[QtaLog]);
				} 
			}
		} 

// *******************************************************************************
//	sprintf(HwSN.codTEST,"100184B");
//	sprintf(HwSN.codMB,"M30300001");
//	sprintf(HwSN.codSV,"S00400011");
// *******************************************************************************

		// assegna un nome temporaneno al file di log: s/n MB)
		sprintf(LogCollaudo, "%s\\Logs\\Log(%s).txt", mRoot, HwSN.codMB);
		CONVERTPATH(LogCollaudo);
	}else{
		sprintf(LogCollaudo, "%s\\Logs\\Log%d.txt", mRoot, MTSdata.mSerial);
		CONVERTPATH(LogCollaudo);
	}
	StampaDB("File di Log", LogCollaudo);				// Su finestra DOS di Debug 
	for (i=1; i<=QtaLog; i++) {							// copia il buffer nel file di log di collaudo definito
		MsgFile(1, LogCollaudo, LogBuffer[i]);
	}
	k = M_GetSwVers();										// si legge la Versione Software
	MTSdata.SVer = (float)k/1000;
	MTSdata.mSign = M_GetFamily();									// la Famiglia dell'MTS è la sua 'Signature' e ...
	//															deve corrispondere al tipo previsto dal test (MtsTK.mTipo)
	sprintf(buflog,"\r----------------  Inizio Collaudo  ------------ %s --", SCRREL);
	MsgFile(1, LogCollaudo, buflog);
	if ((MTSdata.mSign == MtsTK.mTipo) && (MtsTK.presON) && (MtsTK.presOFF)) {
		sprintf(buflog,"Collaudo dell'MTS tipo %s, s.n. %d (Ver.%5.2f)", TkIni.mName, MTSdata.mSerial, MTSdata.SVer);
		LoggaStampa(buflog);
	}
	if ((MTSdata.mSign == MtsTK.mTipo) && (!(MtsTK.presON)) && (MtsTK.presOFF)) {
		sprintf(buflog,"Collaudo dell'MTS tipo %s, s.n. %d (Ver.%5.2f). Presenza Seriale letta non a <0>", TkIni.mName,  MTSdata.mSerial, MTSdata.SVer);
		LoggaStampa(buflog);
	}
	if ((MTSdata.mSign == MtsTK.mTipo) && (!(MtsTK.presOFF))) {
		sprintf(buflog,"Collaudo dell'MTS tipo %s, s.n. %d (Ver.%5.2f). Presenza Seriale BLOCCATA a <0>: verificare cablaggio o MTS", TkIni.mName,  MTSdata.mSerial, MTSdata.SVer);
		LoggaStampa(buflog);
	}
	if ((MTSdata.mSign != MtsTK.mTipo) && (MTSdata.mSign != 0) && (MtsTK.presON) && (MtsTK.presOFF)) {
		sprintf(buflog,"ATTENZIONE: l'unità collegata NON è un %s, ma è: %d - %d (Ver.%5.2f)", TkIni.mName,  MTSdata.mSign, MTSdata.mSerial, MTSdata.SVer);
		LoggaStampa(buflog);
			// Prima di finire, si toglie Presenza ed Alimentazione (xchè appena messe)
		T_Output(PRES_, PRES_);
		T_Output(PON_, PON_);		
		COLOR_STEP(MtsTK.steptest, C_RED) ;
		OuputText(1,"ATTENZIONE: Unità diversa",1,0,1,3);
		call_exit(YES, "MTest_KO");
	}
	if ((MTSdata.mSign != MtsTK.mTipo) && (MTSdata.mSign != 0) && (!(MtsTK.presON)) && (MtsTK.presOFF)) {
		sprintf(buflog,"ATTENZIONE: l'unità collegata NON è un %s, ma è: %d - %d (Ver.%5.2f). Presenza Seriale letta non a <0>", TkIni.mName,  MTSdata.mSign, MTSdata.mSerial, MTSdata.SVer);
		LoggaStampa(buflog);
			// Prima di finire, si toglie Presenza ed Alimentazione (xchè appena messe)
		T_Output(PRES_, PRES_);
		T_Output(PON_, PON_);		
		OuputText(1,"ATTENZIONE: Unità diversa",1,0,1,3);
		call_exit(YES, "MTest_KO");
	}
	if ((MTSdata.mSign != MtsTK.mTipo) && (MTSdata.mSign != 0) && (!(MtsTK.presOFF))) {
		sprintf(buflog,"ATTENZIONE: l'unità collegata NON è un %s, ma è: %d - %d (Ver.%5.2f). Presenza Seriale BLOCCATA a <0>: verificare cablaggio o MTS", TkIni.mName,  MTSdata.mSign, MTSdata.mSerial, MTSdata.SVer);
		LoggaStampa(buflog);
			// Prima di finire, si toglie Presenza ed Alimentazione (xchè appena messe)
		T_Output(PRES_, PRES_);
		T_Output(PON_, PON_);		
		OuputText(1,"ATTENZIONE: Unità diversa",1,0,1,3);
		call_exit(YES, "MTest_KO");
	}
	if ((MTSdata.mSign == 0) && (MtsTK.presON) && (MtsTK.presOFF)) {
		sprintf(buflog,"ATTENZIONE: l'unità collegata NON comunica o è spenta, ma la Presenza (I/O e Seriale) è O.K.");
		LoggaStampa(buflog);
			// Prima di finire, si toglie Presenza ed Alimentazione (xchè appena messe)
		T_Output(PRES_, PRES_); 
		T_Output(PON_, PON_);		
		OuputText(1,"ATTENZIONE: l'Unità NON comunica",1,0,1,3);
		call_exit(YES, "MTest_KO");
	}
	if ((MTSdata.mSign == 0) && (!(MtsTK.presON)) && (MtsTK.presOFF)) {
		sprintf (buflog,"ATTENZIONE: l'unità collegata NON comunica o è spenta, Presenza Seriale letta non a <0>: verificare cablaggio o MTS");
		LoggaStampa (buflog);
			// Prima di finire, si toglie Presenza ed Alimentazione (xchè appena messe)
		T_Output(PRES_, PRES_);
		T_Output(PON_, PON_);		
		OuputText(1,"ATTENZIONE: l'Unità NON sembra collegata",1,0,1,3);
		call_exit(YES, "MTest_KO");
	}
	if ((MTSdata.mSign == 0) && (!(MtsTK.presOFF))) {
		sprintf (buflog,"ATTENZIONE: l'unità collegata NON comunica o è spenta. Presenza Seriale BLOCCATA a <0>: verificare cablaggio o MTS");
		LoggaStampa(buflog);
			// Prima di finire, si toglie Presenza ed Alimentazione (xchè appena messe)
		T_Output(PRES_, PRES_);
		T_Output(PON_, PON_);		
		COLOR_STEP(MtsTK.steptest, C_RED) ;
		OuputText(1,"ATTENZIONE: Cablaggio verso l'Unità con problemi",1,0,1,3);
		call_exit(YES, "MTest_KO");
	}
	if ((!(MtsTK.pres2ON)) && (MtsTK.pres2OFF)) {
		sprintf (buflog,"ATTENZIONE: presenza Secondo Connettore non a <0>: verificare cablaggio o MTS");
		LoggaStampa (buflog);
			// Prima di finire, si toglie Presenza ed Alimentazione (xchè appena messe)
		T_Output(PRES_, PRES_);
		T_Output(PON_, PON_);		
		OuputText(1,"ATTENZIONE: l'Unità NON sembra collegata",1,0,1,3);
		call_exit(YES, "MTest_KO");
	}
	if (!(MtsTK.pres2OFF)){
		sprintf (buflog,"ATTENZIONE: presenza Secondo Connettore BLOCCATA a <0>: verificare cablaggio o MTS");
		LoggaStampa(buflog);
			// Prima di finire, si toglie Presenza ed Alimentazione (xchè appena messe)
		T_Output(PRES_, PRES_);
		T_Output(PON_, PON_);		
		COLOR_STEP(MtsTK.steptest, C_RED) ;
		OuputText(1,"ATTENZIONE: Cablaggio verso l'Unità con problemi",1,0,1,3);
		call_exit(YES, "MTest_KO");
	}
	COLOR_STEP(MtsTK.steptest++, C_GREEN) ;
	
	return 0;
}

int SK_Set_MTS(void)
{
	int sendpar; 
	sendpar=0;
	sendpar=sendpar; // Per Compilatore Set but not used
	int sendsm ;
	sendsm=0;
	sendsm=sendsm; // Per Compilatore Set but not used
	FILE *fmacs ;
	
	// "Preparazione MTS"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	ProgressBar(BAR_TIME, 800 ) ; // 1 minuto
	if (MtsTK.firstRun) {												// se NON appena inviato FW per 1.a volta
		sendpar = 1 ;
		sendsm = 1 ;
		M_SetPar(8, "2");					// si imposta Trace a 2
	}else{
		if (!MtsTK.nuovo) {											 // se NON è un MTS nuovo (cioè s/n <> da "-1")
			M_SetPar(104,"N.D.");     //si cancella parametro 104
			M_DelPar(104);            //si cancella parametro 104
			Delay(15); 
			TesterMTS();
			if (TestSet.EnBKdata == 1){	//	Salvataggio parametri e Macchina a Stati
				sendpar = SK_SaveParam();
				sprintf(SMachFile, "%s\\Logs\\%d.smk", mRoot, MTSdata.mSerial);
				CONVERTPATH(SMachFile) ;
				fmacs = fopen(SMachFile, "r");
				if (fmacs==NULL) {								// Se non esiste già un file di Macchina a Stati
					M_GetSmFile(SMachFile);						// Salva la Macchina a Stati nel file corrispondente
					MsgFile(0, LogCollaudo, "Salvata la Macchina a Stati");
					sendsm = 1 ;
#ifdef CBUG
					sprintf(bufwindow, "Salvata la Macchina a Stati: O.K.");
					MsgWindow(bufwindow);
#endif // #ifdef CBUG
				}else{
					fclose(fmacs);
					sendsm = 0 ;
				}
			}
		}
	}
//		M_SetPar(77, "1");			// si imposta lo stato dell'alimentazione di COM1
//		M_SetPar(79, "22588");				// = 0x583C Abilita gli analogici (interni,esterni) e counter
//		M_SetPar(96, "1028");				// Imposto a 4 (0x404) il volume Fonia (sia Mic che Spk)
//		M_SetPar(71, "0");
//		M_SetPar(69, "0");					// Imposta tutti gli ingressi con Pull-DOWN ed accensione con Fronte di Salita
//		if (TestSet.EnCOM2) M_SetPar(76, "2");
//			
////		Si esegue un reboot (tramite invio di S.M.) per rendere operativi tutti i parametri
//		Delay(5); 
//		TesterMTS();
//		sprintf(Bmom, "(%d) ATTENDERE: invio Macchina-Stati per collaudo", MTSdata.mSerial );
//		OuputText(1,Bmom,0,0,0,0);
//		M_PutSmFile(SMachColl);				// Carica la Macchina a Stati per il Collaudo (sarà void o p.e. x la tastiera)
//	}else{
	//if (sendpar){
		M_SetPar(77, "1");					// si imposta lo stato dell'alimentazione di COM1 in "ON"
		M_SetPar(79, "22588");				// = 0x583C Abilita gli analogici (interni,esterni) e counter
		M_SetPar(96, "1028");				// Imposto a 4 (0x404) il volume Fonia (sia Mic che Spk)
#ifdef DEBUG_P71
		M_SetPar(71, "128");
#else
		M_SetPar(71, "0"); //0
#endif
		M_SetPar(69, "0");					// Imposta tutti gli ingressi con Pull-DOWN ed accensione con Fronte di Salita
		if ( (!strcmp(TkIni.mName, "3025")) || (!strcmp(TkIni.mName, "3048")) ) M_SetPar(62, "0");
		if ( (!strcmp(TkIni.mName, "3025")) || (!strcmp(TkIni.mName, "3035")) || (!strcmp(TkIni.mName, "3036"))) {
			//Aggiungere controllo flash 4037
			char bmhex[512];
			char llb[20] ;
			M_Diag(25,0,dDg);
			sprintf(Bmom,"Numeri Blocchi Danneggiati della FLASH:%s",dDg);
			LoggaStampa(Bmom);
			text_to_hex(dDg,bmhex);
			sprintf(llb, " (%hd)",*((unsigned short *)(&bmhex[2]))) ;
			if ( ((atoi(llb)) && 0x80) ){
				sprintf(bufmsg,"ATTENZIONE\nFLASH DANNEGGIATA\nIMPOSSIBILE PROCEDERE!") ;
				call_exit(YES,bufmsg);
			}
		}
		if (TestSet.QtaLaserS)
			M_SetPar(97, "8");
		else
			M_SetPar(97, "0");
		if (TestSet.EnCOM2){
			M_SetPar(76, "10");
			if ( (!strcmp(TkIni.mName, "3025")) || (!strcmp(TkIni.mName, "3035")) || (!strcmp(TkIni.mName, "3036")) || (!strcmp(TkIni.mName, "4037"))) {
				M_SetPar(100, "11");
			}else{
				M_SetPar(100, "N.D.");
			}
		}
		if (TestSet.EnCOMAUX) M_SetPar(101, "11");
	//}
	if (MTSdata.mSerial==-1)  M_SetPar(8, "2");					// si imposta Trace a 2
	M_SetPar(81,"2") ;
	if (TestSet.EnHTL == YES) M_SetPar(105,"256");     //si setta parametro 105 a 0x100 (per bloccare l'HTL come VDO a 10400)	

	//if ((MtsTK.nuovo) && (!(strcmp(TkIni.mName, "2023")))) M_SetPar(75, "0");     // si imposta parametro 75 a zero per 2023 per inex3
	if(!(strcmp(TkIni.mName, "2023"))) M_SetPar(75, "0");     // si imposta parametro 75 a zero per 2023 per inex3
	if(!(strcmp(TkIni.mName, "2022"))) M_SetPar(75, "0");     // si imposta parametro 75 a zero per 2022 per inex3
	if(!(strcmp(TkIni.mName, "2122"))) M_SetPar(75, "0");     // si imposta parametro 75 a zero per 2122 per inex3
	Delay(15);
	TesterMTS();
	
// FRFR
	// Se in produzione (e richiesto) controlla se GSM comunica
	if ((TestSet.GSMbaud) && (MTSdata.mSerial==-1)){
		int k ;
		// Verifico lo stato del GSM
		M_Diag(DIAG_LU, DIAG_LU9, dDg); 
		k = atoi(dDg) ;
		if (k & 0xf0) { // GSM non comunica!!
			sprintf(buflog,"GSM a %d: non comunica (0x%x)", TestSet.GSMbaud, k ) ;
			LoggaStampa(buflog);
			sprintf(bufmsg,"ATTENZIONE\nGSM NON FUNZIONANTE (%d baud)", TestSet.GSMbaud) ;
			MSGBOXCALL(bufmsg ,0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore GSM on");
			}
		}else{
			sprintf(MyDebB,"--> GSM at %d: 0x%x", TestSet.GSMbaud, k ) ;
			LoggaStampa(MyDebB);
		}
		
	}
	// Qui  prima parte di collaudo RTC
	int ki,k0,k1,t0,t1;
	if (TestSet.EnRTC == YES) {
		if (MTSdata.mSign == 40){
			for(ki=0;ki<3;ki++){
				M_Action(0,0,"io");
				Delay(150) ;
				M_Diag(201,0,dDg) ;
				k0 = atoi(dDg) ;
				if (k0) break ;
			}
			deltak=k0;
			deltat=k0;
		}else{	
			M_Action(13,0,0);								// spegne il GPS x eseguire test del RTC
			t0 = M_GetTime();								// mi aspetto un numero intero di secondi !!!!!!
			k0 = T_Cnt(2)/100;								// valore dei "tick" in secondi
			#ifdef CBUG
				sprintf(MyDebB,"1' lettura = %d (%d)\r", t0, k0);
				MsgFile(0, LogCollaudo, MyDebB);
				MsgWindow(MyDebB);
			#endif // #ifdef CBUG
		}
	}
	
	sprintf(Bmom, "(%d) ATTENDERE: invio Macchina-Stati per collaudo", MTSdata.mSerial );
	OuputText(1,Bmom,0,0,0,0);
	M_PutSmFile(SMachColl);					// Carica la Macchina a Stati per il Collaudo (sarà void o p.e. x la tastiera)
	sprintf(Bmom, "(%d) ATTENDERE: riavvio MTS per inizio collaudo", MTSdata.mSerial );
	OuputText(1,Bmom,0,0,0,0);
	int k,r ;
	for (k=0;k<150;k++){	
		r = M_GetSerNum() ;
		if (r!=MTSdata.mSerial) break ;
	}
	Delay(15);
	for (k=0;k<150;k++){	
		r = M_GetSerNum() ;
		if (r==MTSdata.mSerial) break ;
	}
	if ( !(MTSdata.mSign == 40) && (TestSet.EnRTC == YES) ) {
		t1 = M_GetTime();								// mi aspetto un numero intero di secondi !!!!!!
		k1 = T_Cnt(2)/100;								// valore dei "tick" in secondi
		#ifdef CBUG
			sprintf(MyDebB,"2' lettura = %d (%d)\r", t1, k1);
			MsgFile(0, LogCollaudo, MyDebB);
			MsgWindow(MyDebB);
		#endif // #ifdef CBUG
		deltak = k1 - k0;									// tempo trascorso (misurato in sec. di tick-T-K)
		deltat = t1 - t0;									// tempo trascorso (misurato in sec. di MTS)
	}
	COLOR_STEP(MtsTK.steptest++, C_GREEN ) ;
	
	return 0;
}

int SK_SaveParam(void)
{
	int i ;
	FILE *fpar ;
	char M_Par[NRMSIZE] ;
	
	sprintf(ParamFile, "%s\\Logs\\%s_%d.par", mRoot, hostname ,MTSdata.mSerial);
	CONVERTPATH(ParamFile) ;
	fpar = fopen(ParamFile, "r");
	if (fpar != NULL) {											// Se il file di Parametri esiste già ...
		fclose(fpar);
		return 0 ;
	}else{														// altrimenti ...
		fpar = fopen(ParamFile, "w");							// ... lo si crea
		
		for (i=0; i<nr_mtspars ; i++) {
			M_GetPar(mtspars[i], M_Par);
			fprintf(fpar, "%s\r\n", M_Par) ;
		}
// NON NECESSARIO
		fclose(fpar);
		MsgFile(0, LogCollaudo, "Parametri Salvati");
	}
	return 1;
}
#endif // ifndef SENDFW
int SK_ParamSet(void)
{
	M_SetPar(255,"N.D.");  //Cancello Tutti i Parametri
	FILE *fpar ;
	char M_Par[MAXSIZE] ;
	char Valore[NRMSIZE] ;
	char Parametro[NRMSIZE] ;
	
	GetINIKeyVal("ParamFile", TkIni.NamPar);
	togliCR(TkIni.NamPar);
	sprintf(ParamSel, "%s\\%s",mRoot, TkIni.NamPar);
	CONVERTPATH(ParamSel);
	StampaDB("Parametri:",ParamSel);
	fpar = fopen(ParamSel, "r");
	if (fpar == NULL) {			// Se il file di Parametri non esiste ...
		fclose(fpar);
		sprintf(Bmom, "Errore durante la lettura del file: %s\n",ParamSel);
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
	}else{													// si leggono parametri
		while (!feof(fpar)) { //fino alla fine del file
			loc_fgets(M_Par, 256, fpar);
			if (strlen(M_Par)==0) break ;
			togliCR(M_Par);
			GetIntStr("=", M_Par, 1, Parametro);
			GetIntStr("=", M_Par, 2, Valore); 
			RLTrimmwithplace(Valore);						// toglie i caratteri non alfanumerici prima e dopo !
// Aggiunto per debug			
		sprintf(Bmom, "Invio par %d a <%s>\n",atoi(Parametro), Valore);
	  	PrintDB(Bmom);
// Aggiunto per debug			
			M_SetPar(atoi(Parametro), Valore);
		}
		fclose(fpar);
	}                                              
	return 0;
}
#ifndef SENDFW
int SK_RestoreParam(void)
{
	int i ;
	FILE *fpar ;
	char M_Par[NRMSIZE] ;
	sprintf(ParamFile, "%s\\Logs\\%d.par", mRoot, MTSdata.mSerial);
	CONVERTPATH(ParamFile) ;
	fpar = fopen(ParamFile, "r");	
	if (fpar == NULL) {			// Se il file di Parametri non esiste ...
		fclose(fpar);
	}else{													// si leggono parametri
		for (i=0; i<nr_mtspars ; i++) {
			loc_fgets(M_Par, 256, fpar);
			togliCR(M_Par);
			M_SetPar(mtspars[i], M_Par);
		}
		fclose(fpar);
	}                                              
	return 0;
}
#endif // ifndef SENDFW

int SK_ParamSetCheck(void)
{
	int y; // ,i ;
	FILE *fpar ;
	char M_Par[MAXSIZE] ;
	char Valore[NRMSIZE] ;
	char *p;
	char Parametro[NRMSIZE] ;
	char momErrorepar[MAXSIZE] ;
	char Errorepar[MAXSIZE] ;
	char Valoresave[NRMSIZE] ;
	int diverso;
	//char *goodstring;
	//char *M_Par_ASCII;

	//i=1;
	Errorepar[0] = '\0';
	
	GetINIKeyVal("ParamFile", TkIni.NamPar);
	togliCR(TkIni.NamPar);
	sprintf(ParamSel, "%s\\%s",mRoot, TkIni.NamPar);
	CONVERTPATH(ParamSel);
	StampaDB("Controllo Parametri Caricati.","");
	fpar = fopen(ParamSel, "r");
	if (fpar == NULL) {			// Se il file di Parametri non esiste ...
		fclose(fpar);
		sprintf(Bmom, "Errore durante la lettura del file: %s\n",ParamSel);
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
	}else{
		while (!feof(fpar)) { // si leggono parametri fino alla fine del file
			loc_fgets(M_Par, 256, fpar);
			if (strlen(M_Par)==0) break ;
			diverso=0;
			togliCR(M_Par);
			GetIntStr("=", M_Par, 1, Parametro);
			//RLTrimm(Parametro);						// toglie i caratteri non alfanumerici prima e dopo !
			GetIntStr("=", M_Par, 2, Valore);
			RLTrimmwithplace(Valore);						// toglie i caratteri non alfanumerici prima e dopo !
			sprintf(Bmom, "Parametro %d [%s]\n",atoi(Parametro),Valore);
			MsgWindow(Bmom);

			// No needed
//			p = (char *) strip( (unsigned char*)Valore , sizeof(Valore), EXT); /* remove non-extended and non-ascii */
//
			p = strchr (Valore, '(') ;
			if ( p != NULL) {
				p++ ;
				y=strlen(p);
				y--;
				p[y]='\0';
			}else{
				y = strlen(Valore) ;
				p = Valore ;
				p[y] = '\0' ;
		    }
		

			sprintf(Bmom, "Parametro POST %d [%s]\n",atoi(Parametro),p);
			MsgWindow(Bmom);

			strcpy(Valoresave,"N.D.");
			M_GetPar(atoi(Parametro), Valoresave);
			togliCR(Valoresave);
			//goodstring = strip(Valoresave , sizeof(Valoresave), EXT); /* remove non-extended and non-ascii */

			if (strncmp(p,Valoresave,strlen(Valoresave))) diverso++; // se i valori sono diversi
			sprintf(Bmom, "Diversopost[%s]: <%s> %d\n",Parametro, Valoresave, diverso);
			MsgWindow(Bmom);
			if ( diverso > 0 ) {
				sprintf(Bmom, "0x%s",p);	// sprintf(Bmom, "0x%s",p);
				if ( !(strncasecmp(Bmom,Valoresave,strlen(Valoresave))) ) diverso=0;
			}
			//sprintf(Bmom, "Diversopost2[%s]: %d\n",Parametro,diverso);
			//MsgWindow(Bmom);
			if ( diverso > 0 ) {
				sprintf(momErrorepar, "Parametro %s letto [%s] differente da quello impostato [%s]\n",Parametro,Valoresave,p);
				strcat(Errorepar, momErrorepar ) ;
				//if (i==1) {
				//	sprintf(Errorepar,"%s\r",momErrorepar);
				//	i++;
				//}else{
				//	sprintf(Errorepar,"%s%s\r",Errorepar,momErrorepar);
				//}
		  }
		}
		fclose(fpar);
	}
	if (strlen(Errorepar)) { // se c'e errore
		PrintDB(Errorepar);
	  	call_exit(YES, Errorepar);
	}
	sprintf(Bmom, "Parametri Caricati: %s",ParamSel);
	MsgFile(0, LogCollaudo, Bmom);
	MsgWindow (Bmom);
	sprintf(Bmom, "Controllo Parametri Caricati Esito: Positivo");
	MsgFile(0, LogCollaudo, Bmom);
	MsgWindow (Bmom);
	StampaDB("Controllo Parametri Caricati Esito:","Positivo");
	return 0;
}

int SK_SM_StateSet(void)
{
	FILE *fmacs ;
	GetINIKeyVal("StateMachine", TkIni.NamSM);
	togliCR(TkIni.NamSM);
	sprintf(SMachSel, "%s\\%s", mRoot,TkIni.NamSM);
	CONVERTPATH(SMachSel);
	StampaDB("Macchina Stati:",SMachSel);	
	fmacs = fopen(SMachSel, "r");	
	if (fmacs == NULL) {			// Se il file di Macchina Stati non esiste ...
		fclose(fmacs);
		sprintf(Bmom, "Errore durante la lettura del file: %s\n",SMachSel);
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
	}else{ // si invia macchina stati
		fclose(fmacs);									// si chiude il file prima di passarlo all'atra funzione				
		sprintf(Bmom, "(%d_E:%d) ATTENDERE: caricamento Macchina-Stati: %s ", MTSdata.mSerial, MTSdata.ERRTest,TkIni.NamSM);
		OuputText(1,Bmom,0,0,0,0);
		M_PutSmFile(SMachSel);
	} 
	int k,r ;
	for (k=0;k<150;k++){	
		r = M_GetSerNum() ;
		if (r!=MTSdata.mSerial) break ;
	}
	Delay(15);
	for (k=0;k<150;k++){	
		r = M_GetSerNum() ;
		if (r==MTSdata.mSerial) break ;
	}                                            
	return 0;
}

int SK_SM_StateSetCheck(void)
{
	int error=0;
	sprintf(SMachFile, "%s\\Logs\\%s_%d.smk", mRoot,hostname ,MTSdata.mSerial);
	CONVERTPATH(SMachFile) ;
	M_GetSmFile(SMachFile);
	sprintf(Bmom, "Richiesta macchina stati per confronto: %s",SMachFile);
	MsgFile(0, LogCollaudo, Bmom);
	MsgWindow (Bmom);
	unsigned int bufcomp;
	bufcomp=0;
	// Compare File
	//Delay(300); //messo per cambiare file a mano
	FileCompare(SMachSel,SMachFile,&bufcomp);
	sprintf(Bmom, "Bufcomp: %d",bufcomp);
	PrintDB(Bmom);
	if(!bufcomp) {
		SK_SM_StateSet();
		// Delay(400); Prima per ATTENDERE RISVEGLIO MTS
		M_GetSmFile(SMachFile);
		sprintf(Bmom, "Richiesta macchina stati per confronto: %s",SMachFile);
		MsgFile(0, LogCollaudo, Bmom);
		MsgWindow (Bmom);
		unsigned int bufcomp;
		bufcomp=0;
		// Compare File
		//Delay(300); //messo per cambiare file a mano
		FileCompare(SMachSel,SMachFile,&bufcomp);
		sprintf(Bmom, "Bufcomp: %d",bufcomp);
		PrintDB(Bmom);
		if(!bufcomp) {
				call_exit(YES, "File diversi");
				error=1;
		}
	}			
	sprintf(Bmom, "Macchina-Stati Caricata: %s",SMachSel);
	MsgFile(0, LogCollaudo, Bmom);
	MsgWindow (Bmom);
	return error;
}

int SK_CanConfSet(void)
{
	sprintf(buflog, "Inizio Configurazione CAN");
	LoggaStampa(buflog);
	int error=0;
	GetINIKeyVal("CanConf", TkIni.NamCANConf);
	togliCR(TkIni.NamCANConf);
	sprintf(buflog, "Configurazione CAN_INI:%s",TkIni.NamCANConf);
	LoggaStampa(buflog);
	sprintf(CANConfSel, "%s\\%s", mRoot,TkIni.NamCANConf);
	sprintf(buflog, "Configurazione CAN:%s",CANConfSel);
	LoggaStampa(buflog);
	CONVERTPATH(CANConfSel);
	sprintf(buflog, "Configurazione CAN:%s",CANConfSel);
	LoggaStampa(buflog);
	Delay(50); //100		
	int cantype;
	if (MTSdata.mSign==40) 
		cantype=0;
	else
		cantype=1;
	if ( M_SENDCANCONF(cantype,CANConfSel) ) {
			sprintf(bufmsg, "Attenzione: errore nel caricare file configurazione CAN");
			LoggaStampa(bufmsg);
			MSGBOXCALL(bufmsg,0,2,"Continua","Ferma",bufresponse); 
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore Conf CAN");
			}
			error=1;
	}else{
		sprintf(buflog, "--> Configurazione CAN O.K.");
		LoggaStampa(buflog);
	}
	Delay(25); //50		
	return error;
}
#ifndef SENDFW

//	======================================================================
//					Test Orologio     (Test superfluo per gli MTS con ARM7)
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_RTC(void)
{
	int Failed = 0;
	if (TestSet.EnRTC == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;

	sprintf(Bmom, "(%d_E:%d) TEST Real-Time-CLOCK", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	// I delta sono calcolati prima del caricamento della macchina stati
	if ((deltat >= (deltak - 2)) && (deltat <= (deltak + 2))) {
		sprintf(MyDebB,"RTC O.K.(%d/%d)", deltat, deltak);
		LoggaStampa(MyDebB);
	}else{						// la differenza è dovuta ad anomalie con il RTC nei 15sec. in cui l'MTS è spento
		sprintf(MyDebB, "RTC in AVARIA (incremento di %d in 15 sec)", (deltak - deltat));
		LoggaStampa(MyDebB);
		MSGBOXCALL("RTC IN AVARIA",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore RTC");
		}
		Failed++;
	}     
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed;
}

//	======================================================================
//					Test INPUT (3 Pulsanti + SWitch) per 1xxx
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_DigIn(void)
{
	int Failed = 0;
	int i,k, MaskIn, finito ; //,j;
	unsigned int ktest;
	char M_Par[NRMSIZE] ;
	unsigned short in_Letti, inr, lett1, lett2;
	unsigned short confl, inOK_, matc;//, cnf ;
	
	T_Output(EN_CNT, 0);						// Disattivo il timer di T-K di 1 KHz
	if (TKTYPE==0) 
		T_Output(OLDTIN_PUP, 0);				// Tutti gli ingressi di Board-T-K sono configurati con Pull-Down
	else
		T_SetPull( 0xFFFFFFFF , 0);
	
	MTSdata.TKcntStart = T_Cnt(1) ; 				// Leggo il CNT1 del TK (nel caso ci sia il  LED esterno)
	if ((TestSet.EnDigInDn == NO) && (TestSet.EnDigInUp == NO)) {
		COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
		return Failed;
	}
	// Se si sta collaudando un MTS con tastiera (1001, 1016, 1009, 1109)
	if (MtsTK.WTastiera) {	// esci senza far nulla se il test non è richiesto  
		COLOR_STEP(MtsTK.steptest, C_YELLOW ) ;

		M_GetPar(65, M_Par);
		sprintf(M_Par,"%d",(atoi(M_Par) & 0xdf));	// Attiva (se già non lo è) il led Verde
		M_SetPar(65, M_Par);	
		k = 2;										// Numero di volte che si può controllare la tastiera
		sprintf(MexAll,"TEST Pulsanti + SWitch");
		i = k+1;
		while (k > 0){
			ProgressBar(BAR_PERC, 0 ) ;		//Inizializza
			Failed = 0;
			M_Output(0x20, 0x20);							// Si resettano i 4 Status flag del 1xxx tramite l'uscita "7" (ottava)
			Delay(15);										// .. ed attendo che sia passato almeno un cilclo di S.M.
			M_Output(0x20, 0x00);							// Si libera la'uscita "7" (ottava)
			sprintf(MyDebB,"%x",M_InStatus());
			StampaDB("Status",MyDebB);					// Su finestra DOS di Debug 
			sprintf(MyDebB,"%d° TEST 3+1 Pulsanti",(i-k));
			MsgFile(0, LogCollaudo, MyDebB);
			Delay(3);
			lett1 = (0x0F & M_InStatus());					// lett1 sarà "0" se i tasti non sono premuti
			if (lett1!=0){
				sprintf(MyDebB,"PULSANTE/I INATTESO/I: (0x0%x)", lett1);
				LoggaStampa(MyDebB);						// mappa dei pulsanti rilevata
				Failed++;
			}
//		else 		MsgWindow ("RESETTATI I Flags STATUS");
			sprintf(Bmom, "(%d_E:%d) %s", MTSdata.mSerial, MTSdata.ERRTest, MexAll);
			OuputText(1,Bmom,0,0,0,0);
			OuputText(6,"",4,"RED",1,1);
			OuputText(7,"",4,"BLUE",1,6);
			OuputText(8,"",4,"GREEN",1,2);
			OuputText(9,"",4,"BACK",1,5);
			MsgWindow("TEST 3+1 Pulsanti\r");
			MSGBOXCALL("Azionare i 3 tasti e lo switch interno entro 20 secondi", 0, 1, 0, 0, bufresponse);
			ProgressBar(BAR_TIME, 200 ) ; // 20s
			finito = 0;
			TimeDUT = time(NULL);
#ifdef CBUG
			sprintf(MyDebB,"Tempo iniziale: %d\r", (int)TimeDUT);
			MsgWindow(MyDebB);
#endif // #ifdef CBUG
			lett2 = 0;
			while (finito==0) {
				lett1 = (0x0F & M_InStatus());				// lett1 sarà "0000" per i tasti che non sono stati premuti
				ktest = lett1 ^ lett2;						// con lo XOR solo i bit cambiati (dal ciclo precedente) sono posti a "1"
				if (ktest != 0) {
					if ((ktest & 0x01)!=0) {				// tasto ROSSO pigiato !!
						OuputText(6,"OK",1,"RED",0,0);
					}
					if ((ktest & 0x02)!=0) {				// tasto BLU pigiato !!
						OuputText(7,"OK",6,"BLUE",0,0);
					}
					if ((ktest & 0x04)!=0) {				// tasto VERDE pigiato !!
						OuputText(8,"OK",2,"GREEN",0,0);
					}
					if ((ktest & 0x08)!=0) {				// tasto Contenitore pigiato !!
						OuputText(9,"OK",5,"BACK",0,0);
					}
					lett2 = lett1;
				}
				TimeNOW = time(NULL);
					// c'è il time-out di 15 secondi per il test dei pulsanti
				if ((lett1 == 0x0F ) | (TimeNOW-TimeDUT>20)) finito = 1;	
			}
			ProgressBar(BAR_PERC, 100 ) ;		//Inizializza
#ifdef CBUG
			sprintf(MyDebB,"Durata: %d sec.\r", (int)(TimeNOW-TimeDUT));
			MsgWindow(MyDebB);
#endif // #ifdef CBUG

			sprintf(MyDebB,"%x  -  lett2 = %x", lett1, lett2);
			StampaDB("Tasti-lett1", MyDebB);			// Su finestra DOS di Debug 

			if (lett1 == 0x0F) {
				LoggaStampa("Test dei Pulsanti + Switch O.K.");
				Delay(30);
#ifdef CBUG
				sprintf(MyDebB,"Ingressi letti (0x00): %x\r", lett2);
				MsgWindow(MyDebB);
#endif // #ifdef CBUG
			}else{
				if (!(lett1 & 0x01)) {						// tasto ROSSO NON pigiato !!
					sprintf(MyDebB,"Pulsante ROSSO in avaria");
					LoggaStampa(MyDebB);
				}
				lett1 >>= 1 ;
				if (!(lett1 & 0x01)) {						// tasto BLU NON pigiato !!
					sprintf(MyDebB,"Pulsante BLU in avaria");
					LoggaStampa(MyDebB);
				}
				lett1 >>= 1 ;
				if (!(lett1 & 0x01)) {						// tasto VERDE NON pigiato !!
					sprintf(MyDebB,"Pulsante VERDE in avaria");
					LoggaStampa(MyDebB);
				}
				lett1 >>= 1 ;
				if (!(lett1 & 0x01)) {						// switch Contenitore NON pigiato !!
					sprintf(MyDebB,"Switch Contenitore in avaria");
					LoggaStampa(MyDebB);
				}
				Failed++;
			}
			k--;
			if (Failed==0){
				k=0;								// se il test è andato bene si continua
			}else{
				if (k>0) {
					sprintf (MexAll, "Anomalia TEST Pulsanti + SWitch ... RIPETERE");       			
				}
			}
			OuputText(6,"",0,"",0,0);
			OuputText(7,"",0,"",0,0);
			OuputText(8,"",0,"",0,0);
			OuputText(9,"",0,"",0,0);
		}
	} else {											// se sono altri MTS (senza tastiera)
		// "Ingressi digitali"
		COLOR_STEP(MtsTK.steptest, C_YELLOW ) ;

		sprintf(Bmom, "(%d_E:%d) TEST INGRESSI", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgWindow(" ");
		
		if ((TestSet.QtaIn>8) && (TKTYPE==0)) TestSet.QtaIn=8;			// non si possono gestire + di 8 Input Digitali
		if ((TestSet.QtaIn>16) && (TKTYPE==1)) TestSet.QtaIn=16;		// non si possono gestire + di 16 Input Digitali
		i = 60 * TestSet.QtaIn ;
		if (!strcmp(TestSet.Cpu,"HC12")){ // aggiungo 25 sec 
			i += 250 ;
		}
		
		ProgressBar(BAR_TIME, i) ; // 35 sec

		MaskIn = 0;
		for (i=1; i<=TestSet.QtaIn; i++) {				// creo la maschera per filtrare gli ingressi da testare
			MaskIn <<= 1;
			MaskIn++;
		}

		if (TestSet.MaskDigInDn>0) {
			MaskIn=TestSet.MaskDigInDn;
			TestSet.QtaIn=countBits(MaskIn);
		}
//
//	======================================================================
//					Test INPUT con PULL_DOWN
//	======================================================================
//
		int r;
		if (TestSet.EnDigInDn) {						// se il test con il pull-Down è abilitato
			SetProtocolMode(KPROTOCOLOMODE_QUERYANSWER);
			for (k=0;k<150;k++){	
				r = M_GetSerNum() ;
				if (r==MTSdata.mSerial) break ;         // Attesa per il risveglio 
			}
			//Delay(30);							    // Attesa per il risveglio 
			M_GetPar(69, M_Par);						// Se risponde è SVEGLIO !!!!!!!!!!
			StampaDB("Parametro 69 prima", M_Par);		// Su finestra DOS di Debug
			if( (!strcmp(TkIni.mName, "3025") || (!strcmp(TkIni.mName, "3048")) ) && (TKTYPE==1) ) {
				M_GetPar(62, M_Par);
				StampaDB("Parametro 62 prima", M_Par);
			}
			// Già inserito in SK_Set_MTS
			//M_SetPar(69, "0");					    // Imposta tutti gli ingressi con Pull-DOWN ed accensione con Fronte di Salita
			M_SetPar(70, "255");				        // Imposta Wake-UP da tutti gli ingressi
			  
			MsgWindow(" ");
			// -> Tutte le uscite del T-K sono state poste come "flottanti" all'avvio del collaudo
			LoggaStampa("TEST INPUT con Pull-DOWN");	
			Delay(10);										// .. ed attendo che sia tutto a posto
			lett1 = MaskIn & (M_Input() & 0xFF);			// lett1 sarà '0' per ogni bit di maschera
			if (TKTYPE==0){
				StampaDBn("Ingressi lett1 (0x00)", lett1);			// Su finestra DOS di Debug
				T_Output(SET_IN, IN_TOVEXT);						// Fa cambiare tutti gli ingressi dell'MTS che vanno da 0PD("0" con Pull-Down) a VEXT
			}else{
				if (TestSet.QtaIn>8){
					DatoMTS = M_ReadIntAD();
					Delay(10);
					StampaDBn("Ingressi extin (0x00)", DatoMTS.extin);
					lett1=( lett1 | (DatoMTS.extin<<8)) ;
				}
				StampaDBn("Ingressi lett1 (0x0000)", lett1);	   // Su finestra DOS di Debug 
				T_Output(NEWALLTOVFG , NEWALLTOVFG );			   // Fa cambiare tutti gli ingressi dell'MTS che vanno da 0PD("0" con Pull-Down) a VEXT
				T_Output(NEWFLOAT_ , NEWFLOAT_ );				   // Toglie Flottante
			}
			inr = lett1;
			i = 0;										// si cercano quali ingressi sono rimasti a "1"
			sprintf(MexAll, "%s", "");
			sprintf(separ, "%s", "");
			while (i < TestSet.QtaIn) {
					if (inr & 0x01) {					//	si aggiunge il j-esimo ingresso a quelli che non van bene
						sprintf(&MexAll[strlen(MexAll)], "%sINEX%d", separ, i);
						sprintf(separ,", ");
						StampaDBn("Ingresso testato", i);	// Su finestra DOS di Debug 
						StampaDB("MexAll", MexAll);		// Su finestra DOS di Debug
						sprintf(MyDebB,"INEX%d K.O. (no Pull_Down)", i);	//	.. segnala anonalia
						LoggaStampa(MyDebB);
						Failed++;
					}
					inr >>= 1 ; i++;
			}
			Delay(15);											// .. ed attendo che sia tutto a posto (0608 era 0.5 sec)
			lett2 = MaskIn & (M_Input() & 0xFF);				// lett2 sarà '1' per ogni bit di maschera
			if (TKTYPE==0){
				StampaDBn("Ingressi lett2 (0xFF)", lett2);		// Su finestra DOS di Debug 
				T_Output(SET_IN, IN_TOFLOAT);					// Fa cambiare tutti gli ingressi dell'MTS che vanno da VEXT a 0PD("0" con Pull-Down)
			}else{
				if (TestSet.QtaIn>8){
					DatoMTS = M_ReadIntAD();
					Delay(10);
					StampaDBn("Ingressi extin (0xFF)", DatoMTS.extin);
					lett2=( lett2 | (DatoMTS.extin<<8)) ;
				}
				StampaDBn("Ingressi lett2 (0xFFFF)", lett2);	// Su finestra DOS di Debug 
				T_Output(NEWFLOAT_ , 0);					    // Fa cambiare tutti gli ingressi dell'MTS che vanno da VEXT a 0PD("0" con Pull-Down)
			}
			Delay(50);										// .. ed attendo che sia tutto a posto (0608 era 0.5 sec)
			in_Letti = MaskIn & (M_Input() & 0xFF);					// in_Letti sarà '0' per ogni bit di maschera
			if (TKTYPE==0){
				StampaDBn("Ingressi in_letti (0x00)", in_Letti);	// Su finestra DOS di Debug 
			}else{
				if (TestSet.QtaIn>8){
					DatoMTS = M_ReadIntAD();
					Delay(10);
					DatoMTS = M_ReadIntAD();
					Delay(10);
					StampaDBn("Ingressi extin (0x00)", DatoMTS.extin);
					in_Letti=( in_Letti | (DatoMTS.extin<<8));
				}
				StampaDBn("Ingressi in_letti (0x0000)", in_Letti);	// Su finestra DOS di Debug 
			}
			unsigned int test;
			test=(~(lett1) & lett2 & ~(in_Letti));
			StampaDBn("Ingressi test (0x0000)", test);		// Su finestra DOS di Debug
			StampaDBn("Ingressi MASK (0x0000)", MaskIn);	// Su finestra DOS di Debug

			if ((~(lett1) & lett2 & ~(in_Letti)) == MaskIn) {
				sprintf(MyDebB,"--> INEX0..INEX%d O.K. (con Pull-Down)", TestSet.QtaIn-1);
				LoggaStampa(MyDebB);
				if (TKTYPE==0){
					T_Output(SET_IN, IN_TOVEXT);
					T_Output(CHN, 7); Delay(5);
					T_Output(AN_DIG_, 0);
				}
			} else {	
				/*
				if (TKTYPE==0){
					T_Output(SET_IN, IN_TOVEXT);					   // Fa cambiare tutti gli ingressi dell'MTS che vanno da 0PD("0" con Pull-Down) a VEXT
				}else{
					T_Output(NEWALLTOVFG , NEWALLTOVFG );			   // Fa cambiare tutti gli ingressi dell'MTS che vanno da 0PD("0" con Pull-Down) a VEXT
					T_Output(NEWFLOAT_ , NEWFLOAT_ );				   // Toglie Flottante
				}
				Delay(20);									// .. ed attendo che sia tutto a posto (0608 era 0.5 sec)
				*/
				//in_Letti = MaskIn & (M_Input() & 0xFF);
				in_Letti=test;
				if (TKTYPE==0){
					StampaDBn("Ingressi in_letti (0x00)", in_Letti);	// Su finestra DOS di Debug 
				}else{
					if (TestSet.QtaIn>8){
						DatoMTS = M_ReadIntAD();
						Delay(10);
						DatoMTS = M_ReadIntAD();
						Delay(10);
						StampaDBn("Ingressi extin (0x00)", DatoMTS.extin);
						in_Letti=( in_Letti | (DatoMTS.extin<<8)) ;
					}
					StampaDBn("Ingressi in_letti (0x0000)", in_Letti); // Su finestra DOS di Debug
					test=(in_Letti & MaskIn);
					StampaDBn("Ingressi test (0x0000)", test);		// Su finestra DOS di Debug
		
				}
				if ( test != MaskIn ) {					// se NON sono tutti a "0"  ... cerco x individuare quali non vanno bene
					inr = in_Letti;
					i = 0;										// si cercano quali ingressi sono rimasti a "0"
					sprintf(MexAll, "%s", "");
					sprintf(separ, "%s", "");
					while (i < TestSet.QtaIn) {
							if ((inr & 0x01)!=1) {					//	si aggiunge il j-esimo ingresso a quelli che non van bene
								sprintf(&MexAll[strlen(MexAll)], "%sINEX%d", separ, i);
								sprintf(separ,", ");
								StampaDBn("Ingresso testato", i);	// Su finestra DOS di Debug 
								StampaDB("MexAll", MexAll);		// Su finestra DOS di Debug
								sprintf(MyDebB,"INEX%d K.O. (con Pull_DOWN)", i);	//	.. segnala anonalia
								LoggaStampa(MyDebB);
								Failed++;
							}
							inr >>= 1 ; i++;
					}
					sprintf(MyDebB,"Ingressi Bloccati a <0>: %s\r", MexAll);	// si mostrano tutti gli ingressi individuati
					MsgWindow(MyDebB);
					MSGBOXCALL(MyDebB,0,2,"Continua","Ferma",bufresponse);			// (0609) aggiunto per permettere uscita
					if (strcmp(bufresponse,"#!")==0) {
						call_exit(YES, "Errore INEX-PD");
					}	
				}else{
					sprintf(MyDebB,"--> INEX0..INEX%d O.K. (con Pull-Down)", TestSet.QtaIn-1);
					LoggaStampa(MyDebB);
				}
				/////////////////////////////////////////////////////////////////////////////////////
				///////Vecchio TEST INGRESSI SE SI VUOLE USARE SISTEMARE PER NUOVO T-K///////////////
				/////////////////////////////////////////////////////////////////////////////////////
				/*	
				T_Output(AN_DIG_, 0);		// abilita il pilotaggio dei canali del T-K
				i = 0; matc = 0x01;							// posiziona il "puntatore" di canale sul 1° ingresso
				while (i < TestSet.QtaIn) { 						// test per tutti gli ingressi dell'MTS ...
					if (TKTYPE==0) T_Output(CHN, i);				// pilota l'uscita corrispondente del T-K
					Delay(15);
						// si controlla quanto legge l'MTS (deve essere a 0, per PDN, solo quello puntato)
					in_Letti = MaskIn & M_Input();			
					StampaDBn("Ingressi letti", in_Letti);	// Su finestra DOS di Debug 
					inOK_ = MaskIn & (in_Letti & matc);				// inOK_ è "0" se i-esimo ingresso è OK
					confl = MaskIn & (in_Letti | matc);				// confl ha "0" per ogni ingresso in conflitto
					if ((inOK_ == 0) && (confl == MaskIn)) {	// se l'ingresso testato è a "0" e ..
																	//	.. se gli altri ingressi sono tutti a "1" (VEXT)
						sprintf(MyDebB,"INEX%d OK (posto a <0> dall'esterno)", i);	
						LoggaStampa(MyDebB);
					}else{
						if (inOK_ != 0) {							// se l'ingresso testato NON è a "0"
					  		sprintf(MyDebB,"INEX%d in Avaria: letto a <1> (posto a <0> dall'esterno)\r", i);
							Failed++;
						}
						if (confl != MaskIn) {						// ed inoltre si segnalano quali altri ingressi NON sono a "1"
							cnf = (~(confl) & MaskIn);
							j = 0;									// si individuano quale sono gli ingressi in conflitto tra loro
							sprintf(MexAll, "%s", "");
							sprintf(separ, "%s", "");
							while (cnf != 0) {													
								if (cnf & 0x01) {
									sprintf(&MexAll[strlen(MexAll)], "%sINEX%d", separ, j);
									sprintf(separ,", ");
								}
								cnf >>= 1 ; j++;
							}
							sprintf(MyDebB,"Conflitto tra INEX%d e %s", i, MexAll);
							Failed++;
						}
						LoggaStampa(MyDebB);
						MSGBOXCALL(MyDebB,0,2,"Continua","Ferma",bufresponse);			// (0609) aggiunto per permettere uscita
						if (strcmp(bufresponse,"#!")==0) {
							call_exit(YES, "Errore INEX-PD");
						}
						sprintf(MyDebB,"INEX%d K.O. (con Pull_Down)", i);	//	.. segnala anonalia
						LoggaStampa(MyDebB);
					}
					i++; matc <<= 1;									//	si passa all'ingresso successivo
				}
				*/
				/////////////////////////////////////////////////////////////////////////////////////
				///////Vecchio TEST INGRESSI SE SI VUOLE USARE SISTEMARE PER NUOVO T-K///////////////
				/////////////////////////////////////////////////////////////////////////////////////
			}
		}
//	======================================================================
//					Test INPUT con PULL_UP
//	======================================================================
		if (TestSet.EnDigInUp) {							// se il test con il pull-Up è abilitato
			if (TestSet.MaskDigInUp>0) {
				MaskIn=TestSet.MaskDigInUp;
				TestSet.QtaIn=countBits(MaskIn);
			}
			SetProtocolMode(KPROTOCOLOMODE_QUERYANSWER);			
			if (TKTYPE==0){
				T_Output(SET_IN, IN_TOFLOAT);				// pone tutte le uscite del T-K come "flottanti"
				T_Output(CHN, 0);							// posiziona il "puntatore" di canale sul 1° ingresso MTS (INEX0)
			}else{
				T_Output(NEWALLTOVFG , NEWALLTOVFG );	    // Fa cambiare tutti gli ingressi dell'MTS che vanno da 0PD("0" con Pull-Down) a VEXT
				T_Output(NEWFLOAT_ , 0);		            // Mette Flottante
			}
			Delay (25);	
			M_SetPar(69, "255");						// Imposta tutti gli ingressi MTS con Pull-PU ed accensione con Fronte di Salita
			if ( ( (!strcmp(TkIni.mName, "3025")) || (!strcmp(TkIni.mName, "3048")) ) && (TKTYPE==1) ) {
				Delay (25);
				M_SetPar(62, "255");
			}
			Delay (25);									// .. ed attendo che sia tutto a posto
			MsgWindow(" ");
			LoggaStampa("TEST INPUT con Pull-UP");
			Delay (25);
			// Se HC12 devo riavviare l'MTS
			if (!strcmp(TestSet.Cpu,"HC12")){
				T_Output(PRES_, PRES_);					// Tolgo presenza
				LoggaStampa("HC12: necessario reboot dell'MTS per attivare Pull-UP");
				// Delay(200); 							// come era prima 
				// Attendo spegnimento
				int k,r ;
				for (k=0;k<200;k++){	
				r = M_GetSerNum() ;
				if (r!=MTSdata.mSerial) break ;
				}
				
				T_Output(PRES_, 0);						// Si accende l'MTS ... attivando la Presenza ...
				if (!strcmp(TkIni.mName,"MTS02")){
					T_Output(PON_, PON_);
					Delay(10);
					T_Output(PON_, 0);
				}
				
				//Delay(100) ; 							// Come era Prima Attende accensione HC12
				//Attende accensione HC12
				Delay(15);
				for (k=0;k<100;k++){	
					r = M_GetSerNum() ;
				if (r==MTSdata.mSerial) break ;
				}
			}
			M_GetPar(69, M_Par);						// Se risponde è SVEGLIO !!!!!!!!!!
			StampaDB("Parametro 69 prima", M_Par);		// Su finestra DOS di Debug
			if ( ( (!strcmp(TkIni.mName, "3025")) || (!strcmp(TkIni.mName, "3048")) )  && (TKTYPE==1) ) {
				Delay(10);
				M_GetPar(62, M_Par);
				StampaDB("Parametro 62 prima", M_Par);
			}
			if (TKTYPE==0){
				//T_Output(AN_DIG_, 0);						// Accende l'MTS con INEX0 che va da 1PU("1" con Pull-Up) a "0"
				Delay(20);									// ... ed attendo che sia tutto a posto
				//	 Con i seguenti comandi si blocca a "0" un canale per volta (si leggerà su MTS solo un ingresso a "0" per volta)
				i = 0; 
				matc = 0x01;								// posiziona il "puntatore" di canale sul 1° ingresso
				while (i < TestSet.QtaIn) { 				// test per tutti gli ingressi ..
					T_Output(CHN, i);
					Delay(15);
					in_Letti = MaskIn & (M_Input() & 0xFF);
					StampaDBn("Ingressi letti", in_Letti);	// Su finestra DOS di Debug 
					inOK_ = MaskIn & (in_Letti & matc);		// inOK_ è "0" se i-esimo ingresso è OK                   
					confl = MaskIn & (in_Letti | matc);		// confl ha "0" per ogni ingresso in conflitto     
					if ((inOK_ == 0) && (confl == MaskIn)) {// se l'ingresso testato è a "0" e ..
						sprintf(MyDebB,"INEX%d OK (posto a <0> dall'esterno)", i);	//	.. se gli altri ingressi sono tutti a "1" (1PU)
						StampaDB(MyDebB,"");				// Su finestra DOS di Debug 
					} else {
						if (inOK_ != 0) {					// se l'ingresso testato NON è a "0"
							sprintf(MyDebB,"INEX%d in Avaria: letto a <1> (posto a <0> dall'esterno)\r", i);
							Failed++;
						}
						/*
						if (confl != MaskIn) {						// ed inoltre si segnalano quali altri ingressi NON sono a "1"
							cnf = (~(confl) & MaskIn);
							j = 0;									// si individuano quale sono gli ingressi in conflitto tra loro
							sprintf(MexAll, "%s", ""); 
							sprintf(separ, "%s", "");
							while (cnf != 0) {													
								if (cnf & 0x01) {
									sprintf(&MexAll[strlen(MexAll)], "%sINEX%d", separ, j);
									sprintf(separ,", ");
								}
								cnf >>= 1 ; j++;
							}
							sprintf(MyDebB,"Conflitto tra INEX%d e %s", i, MexAll);
							Failed++;
						}
						*/
						LoggaStampa(MyDebB);
						MSGBOXCALL(MyDebB,0,2,"Continua","Ferma",bufresponse);		// (0609) aggiunto (è come Test Pull_Down) per permettere uscita
						if (strcmp(bufresponse,"#!")==0) {
							call_exit(YES, "Errore INEX-PU");
						}
						sprintf(MyDebB,"INEX%d K.O. (con Pull_Up)", i);	//	.. segnala anonalia
						LoggaStampa(MyDebB);
					}
					i++; matc <<= 1;										//	si passa all'ingresso successivo
				}
				if (Failed == 0) {											//	se non ci sono stati errori ..
					sprintf(MyDebB,"--> INEX0..INEX%d O.K. (con Pull-Up)", TestSet.QtaIn-1);	
					LoggaStampa(MyDebB);
				}
			}else{
				// -> Tutte le uscite del T-K sono state poste come "flottanti"
				M_GetPar(69, M_Par);						// Se risponde è SVEGLIO !!!!!!!!!!
				StampaDB("Parametro 69 adesso:", M_Par);		// Su finestra DOS di Debug
				if ( ( (!strcmp(TkIni.mName, "3025")) || (!strcmp(TkIni.mName, "3048")) )  && (TKTYPE==1) ) {
					Delay(10);
					M_GetPar(62, M_Par);
					StampaDB("Parametro 62 adesso:", M_Par);
				}
				Delay (25);
				lett1 = MaskIn & (M_Input() & 0xFF);			// lett1 sarà '1' per ogni bit di maschera
				if (TestSet.QtaIn>8){
					DatoMTS = M_ReadIntAD();
					Delay(10);
					DatoMTS = M_ReadIntAD();
					Delay(10);
					StampaDBn("Ingressi extin (0xFF)", DatoMTS.extin);
					lett1=( lett1 | (DatoMTS.extin<<8)) ;
				}
				StampaDBn("Ingressi lett1 (0xFFFF)", lett1);		// Su finestra DOS di Debug 
				inr = lett1;
				i = 0;										// si cercano quali ingressi sono rimasti a "0"
				sprintf(MexAll, "%s", "");
				sprintf(separ, "%s", "");
				while (i < TestSet.QtaIn) {
						if ((inr & 0x01)==0) {					//	si aggiunge il j-esimo ingresso a quelli che non van bene
							sprintf(&MexAll[strlen(MexAll)], "%sINEX%d", separ, i);
							sprintf(separ,", ");
							StampaDBn("Ingresso testato", i);	// Su finestra DOS di Debug 
							StampaDB("MexAll", MexAll);		// Su finestra DOS di Debug
							sprintf(MyDebB,"INEX%d K.O. (no Pull_UP)", i);	//	.. segnala anonalia
							LoggaStampa(MyDebB);
							Failed++;
						}
						inr >>= 1 ; i++;
				}
				T_Output(NEWALLTOVFG , 0 );			   				// Fa cambiare tutti gli ingressi dell'MTS che vanno da 1PU("1" con Pull-Up) a GND
				T_Output(NEWFLOAT_ , NEWFLOAT_ );					// Toglie Flottante
				Delay(15);											// .. ed attendo che sia tutto a posto (0608 era 0.5 sec)
				lett2 = MaskIn & (M_Input() & 0xFF);				// lett2 sarà '0' per ogni bit di maschera
				if (TestSet.QtaIn>8){
					DatoMTS = M_ReadIntAD();
					Delay(10);
					StampaDBn("Ingressi extin (0x00)", DatoMTS.extin);
					lett2=( lett2 | (DatoMTS.extin<<8)) ;
				}
				StampaDBn("Ingressi lett2 (0x0000)", lett2);		// Su finestra DOS di Debug 
				T_Output(NEWFLOAT_ , 0);							// Fa cambiare tutti gli ingressi dell'MTS che vanno da GND a 1PU("1" con Pull-Up)
				T_Output(NEWALLTOVFG , NEWALLTOVFG );				// Per Flottante
				Delay(50);											// .. ed attendo che sia tutto a posto (0608 era 0.5 sec)
				in_Letti = MaskIn & (M_Input() & 0xFF);				// in_Letti sarà '1' per ogni bit di maschera
				if (TestSet.QtaIn>8){
					DatoMTS = M_ReadIntAD();
					Delay(10);
					DatoMTS = M_ReadIntAD();
					Delay(10);
					StampaDBn("Ingressi extin (0xFF)", DatoMTS.extin);
					in_Letti=( in_Letti | (DatoMTS.extin<<8)) ;
				}
				StampaDBn("Ingressi in_letti (0xFFFF)", in_Letti);	// Su finestra DOS di Debug 
				StampaDBn("Ingressi lett1 (0xFFFF)", lett1);		// Su finestra DOS di Debug
				StampaDBn("Ingressi lett2 (0xFFFF)", ~(lett2) );		// Su finestra DOS di Debug 
				unsigned int test;
				test=(lett1 & ~(lett2) & in_Letti);
				StampaDBn("Ingressi test (0x0000)", test);			// Su finestra DOS di Debug
				StampaDBn("Ingressi MASK (0x0000)", MaskIn);		// Su finestra DOS di Debug
				if (test == MaskIn) {
					sprintf(MyDebB,"--> INEX0..INEX%d O.K. (con Pull-Up)", TestSet.QtaIn-1);
					LoggaStampa(MyDebB);
				} else {
					/*
					T_Output(NEWALLTOVFG , 0 );			   			// Fa cambiare tutti gli ingressi dell'MTS che vanno da 1PU("1" con Pull-Up) a GND
					T_Output(NEWFLOAT_ , NEWFLOAT_ );				// Toglie Flottante
					Delay(20);										// .. ed attendo che sia tutto a posto (0608 era 0.5 sec)
					*/
					//in_Letti = MaskIn & (M_Input() & 0xFF);
					/*
					if (TestSet.QtaIn>8){
						DatoMTS = M_ReadIntAD();
						Delay(10);
						DatoMTS = M_ReadIntAD();
						Delay(10);
						StampaDBn("Ingressi extin (0xFF)", DatoMTS.extin);
						in_Letti=( in_Letti | (DatoMTS.extin<<8)) ;
					}*/
					in_Letti=test;
					StampaDBn("Ingressi in_letti (0xFFFF)", in_Letti);		// Su finestra DOS di Debug 
					if ( (in_Letti & MaskIn) != MaskIn ) {							// se NON sono tutti a "1"  ... cerco x individuare quali non vanno bene
						inr = in_Letti;
						i = 0;												// si cercano quali ingressi sono rimasti a "0"
						sprintf(MexAll, "%s", "");
						sprintf(separ, "%s", "");
						while (i < TestSet.QtaIn) {
								if ((inr & 0x01)==0) {					//	si aggiunge il j-esimo ingresso a quelli che non van bene
									sprintf(&MexAll[strlen(MexAll)], "%sINEX%d", separ, i);
									sprintf(separ,", ");
									StampaDBn("Ingresso testato", i);	// Su finestra DOS di Debug 
									StampaDB("MexAll", MexAll);		// Su finestra DOS di Debug
									sprintf(MyDebB,"INEX%d K.O. (con Pull_UP)", i);	//	.. segnala anonalia
									LoggaStampa(MyDebB);
									Failed++;
								}
								inr >>= 1 ; i++;
						}
						sprintf(MyDebB,"Ingressi Bloccati a <0>: %s\r", MexAll);	// si mostrano tutti gli ingressi individuati
						MsgWindow(MyDebB);
						MSGBOXCALL(MyDebB,0,2,"Continua","Ferma",bufresponse);		// (0609) aggiunto per permettere uscita
						if (strcmp(bufresponse,"#!")==0) {
							call_exit(YES, "Errore INEX-PU");
						}
					
					}else{
						sprintf(MyDebB,"--> INEX0..INEX%d O.K. (con Pull-Up)", TestSet.QtaIn-1);
						LoggaStampa(MyDebB);
					}
				}
			}
		}
	}
	if ( ( 
			(!strcmp(TkIni.mName, "3025")) || 
			(!strcmp(TkIni.mName, "2034")) || 
			(!strcmp(TkIni.mName, "2039")) || 
			(!strcmp(TkIni.mName, "2040")) || 
			(!strcmp(TkIni.mName, "3035")) || 
			(!strcmp(TkIni.mName, "3036")) || 
			(!strcmp(TkIni.mName, "4037")) ||
  		(!strcmp(TkIni.mName, "2046")) ||
			(!strcmp(TkIni.mName, "2044")) ||
			(!strcmp(TkIni.mName, "2046_M4")) ||
			(!strcmp(TkIni.mName, "2044_M4"))
		 ) && TestSet.EnTamper  ) 
	{
		int ritenta;
		for (ritenta=0;ritenta<2; ritenta ++){	
			SetProtocolMode(KPROTOCOLOMODE_QUERYANSWER);
			int Tamper,OkTamper;
			OuputText(9,"",4,"ALARM",1,5);
			MsgWindow("TEST TAMPER\r");
			Delay(20);
			if ( (!strcmp(TkIni.mName, "3025")) || (!strcmp(TkIni.mName, "3035")) || (!strcmp(TkIni.mName, "3036")) || (!strcmp(TkIni.mName, "4037")) ) {
				DatoMTS = M_ReadIntAD();
				Delay(10);
				Tamper= ( DatoMTS.extin & 0x10 );
			}else{
				Tamper= ( M_Input() & 0x40 );
			}
			StampaDBn("Ingresso Tamper >=", Tamper);
			OkTamper=0;
			int tentativi=0;
			int notcurstatus=Tamper ;
			MSGBOXCALL("Premere e tener Premuto lo switch interno", 0, 1, 0, 0, bufresponse);
			ProgressBar(BAR_TIME, 200 ) ; // 20s
			tentativi=0;
			while (OkTamper==0) { //0
				if ( (!strcmp(TkIni.mName, "3025")) || (!strcmp(TkIni.mName, "3035")) || (!strcmp(TkIni.mName, "3036")) || (!strcmp(TkIni.mName, "4037")) ) {
					DatoMTS = M_ReadIntAD();
					Delay(10);
					if ((DatoMTS.extin & 0x10)!=notcurstatus) OkTamper=1;
				}else{
					if ((M_Input() & 0x40)!=notcurstatus) OkTamper=1;
				}
				Delay(20);
				tentativi ++;
				if (tentativi>3) Tamper=-1;
			}
			if (OkTamper==1){
				OuputText(9,"OK",5,"ALARM",0,0);
				sprintf(MyDebB,"Test Tamper OK");
				LoggaStampa(MyDebB);
				MsgWindow(MyDebB);
				//MSGBOXCALL("Rilasciare lo switch", 0, 1, 0, 0, bufresponse);
				ProgressBar(BAR_TIME, 200 ) ; // 20s
				ritenta=2; // per uscire dal ciclo for
			}else{
				//Failed++;
				sprintf(MyDebB,"Tamper Bloccato");
				LoggaStampa(MyDebB);
				MsgWindow(MyDebB);
				if (!ritenta) {
					MSGBOXCALL("ATTENZIONE TAMPER IN AVARIA" ,0,2,"Ripeti","Continua",bufresponse);
					if (strcmp(bufresponse,"##")==0) {
						Failed++;
						ritenta=2; // per uscire dal ciclo for
					}
				}else{
					Failed++;
					MSGBOXCALL(MyDebB,0,2,"Continua","Ferma",bufresponse);			// (0609) aggiunto per permettere uscita
					if (strcmp(bufresponse,"#!")==0) {
						call_exit(YES, "Errore Tamper");
					}
				}
			}
		ritenta++;
		}
	}
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed;
}

//	======================================================================
//					Test OUTPUT  per 1xxx
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_DigOut(void)
{
	int Failed = 0;
	int i, k, Lett, MaskOut, MaskRl ; //,j;
	int ExCh, ExTmp, ToSt;
	unsigned short out_Letti ;
	unsigned short confl, outOK_, matc;//, cnf ;
	
	if (TestSet.EnDigOut == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	// "Uscite digitali"
	COLOR_STEP(MtsTK.steptest, C_YELLOW ) ;

	// Se si sta collaudando un MTS con tastiera (1001, 1016, 1009, 1109)
	if (MtsTK.WTastiera) {
		sprintf(Bmom, "(%d_E:%d) TEST LED e Buzzer", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		M_Output(ALLOUT, NOOUT);										// Disattiva tutte le uscite dell'MTS
		MsgFile(0, LogCollaudo, "TEST LED e Buzzer");
		MsgWindow("TEST LED e Buzzer\r");
		MSGBOXCALL("Nel test dei Pulsanti\rsi è sentito il Buzzer?" ,"BUZZER" ,2 ,"Si" ,"No" , bufresponse);
		if (strcmp(bufresponse,"#!")==0) {													// il buzzer Non si è sentito
			Failed++;
			sprintf(MyDebB,"Buzzer in Avaria\r");
			MSGBOXCALL("Buzzer in Avaria\r",0,2,0,0,bufresponse);
			MsgWindow(MyDebB);
			MsgFile(0, LogCollaudo, MyDebB);
		}
		M_Output(LED_R, LED_R);										// Si accende il LED Rosso
		MSGBOXCALL("Il led ROSSO si è acceso?" ,"Led ROSSO" ,2 ,"Si" ,"No" , bufresponse);
		if (strcmp(bufresponse,"#!")==0) {													// il Led Rosso Non si è acceso
			Failed++;
			sprintf(MyDebB,"Led ROSSO in Avaria\r");
			MSGBOXCALL("Led ROSSO in Avaria\r",0,2,0,0,bufresponse);
			MsgWindow(MyDebB);
			MsgFile(0, LogCollaudo, MyDebB);
		}
		M_Output(LED_R, 0);												// Si spegne il LED Rosso
		M_Output(LED_G, LED_G);										// Si accende il LED Verde
		MSGBOXCALL("Il led VERDE è acceso?" ,"Led VERDE" ,2 ,"Si" ,"No" , bufresponse);
		if (strcmp(bufresponse,"#!")==0) {													// il Led Rosso Non si è acceso
			Failed++;
			sprintf(MyDebB,"Led VERDE in Avaria\r");
			MSGBOXCALL("Led VERDE in Avaria\r",0,2,0,0,bufresponse);
			MsgWindow(MyDebB);
			MsgFile(0, LogCollaudo, MyDebB);
		}
		if (Failed==0) MsgFile(0, LogCollaudo, "---> LED e BUZZER O.K.");
		M_Output(ALLOUT, NOOUT);									// Disattiva tutte le uscite dell'MTS
	} else {														// se sono altri MTS (senza tastiera)

		if ((TestSet.QtaOD>6) && (TKTYPE==0)) TestSet.QtaOD=6;				// non si possono gestire + di 6 Output Digitali
		if ((TestSet.QtaOD>16) && (TKTYPE==1)) TestSet.QtaOD=16;				// non si possono gestire + di 16 Output Digitali
		if (TestSet.QtaRL>2) TestSet.QtaRL=2;				// non si possono gestire + di 2 Relè
		
		i = TestSet.QtaOD * 50 + TestSet.QtaRL * 130 + 50 ;
		ProgressBar(BAR_TIME, i) ;


		MaskOut = 0;
		for (i=1; i<=TestSet.QtaOD; i++) {									// creo la maschera per filtrare le uscite da testare
			MaskOut <<= 1;
			MaskOut++;
		}

//	======================================================================
//	TEST delle uscite Open Drain dell'MTS
//	======================================================================
		sprintf(Bmom, "(%d_E:%d) TEST USCITE", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgWindow(" ");
		LoggaStampa("TEST degli OUTPUT");
		if (!strcmp(TkIni.mName, "3048")) {
			M_Output(0xFFFF, 0x0000 );								// Disattiva tutte le uscite dell'MTS 
		}else{ 
			M_Output(ALLOUT, NOOUT);								// Disattiva tutte le uscite dell'MTS (0609)
		}
		if (TKTYPE==0) 
			T_Output(OLDTIN_PUP, OLDTIN_PUP);					// tutti gli ingressi di Board-T-K sono configurati con Pull-UP (0609)
		else
			T_SetPull(0xFFFFFFFF, 0xFFFFFFFF);
		Delay(3);													// attesa di 0.3 sec. (14/09/11 era 2 sec.) (0609 era 0 sec.)
		i = 0; matc = 0x01;											// A partire dalla 1ª Uscita ...
		sprintf(MexAll, "%s", ""); sprintf (separ, "%s", "");  // Si prepara stringa per messaggio errore
		while (i < TestSet.QtaOD) {							// ... per tutte le Uscite MTS in Open Drain
			if ( (!strcmp(TkIni.mName, "MTS05")) && (i==2)) matc = 1<<5 ;
			M_Output(matc, matc);									// Attiva l'i-esima uscita di MTS
			Delay(3);															// attesa di 0.3 sec. (14/09/11 era 2 sec.) (0608 era 0.5 sec.)
			out_Letti = T_Input();								// si leggono gli INPUT di T-K
			// Old (1-4-5-6-7-8)
			//if ( (!strcmp(TkIni.mName, "3208")) && (i>0))
			//	out_Letti >>= 2 ;
			// New (1-7-8-4-5-6)
			if ( (!strcmp(TkIni.mName, "3025")))
			  out_Letti = ((out_Letti & 0xff39) | ((out_Letti & 0xc0)>>5) | ((out_Letti & 0x6)<<5) );
			if (!strcmp(TkIni.mName, "3208"))
				out_Letti = ((out_Letti & 0x39) | ((out_Letti & 0xc0)>>5) );
			outOK_ = MaskOut & (out_Letti & matc);					// outOK_ è "0" se i-esima uscita MTS è OK
			confl = MaskOut & (out_Letti | matc);					// confl (8bit) ha "0" per ogni uscita MTS in conflitto
			if ((outOK_ == 0) && (confl == MaskOut)) {				// se l'uscita testata è a "0" e ..
				sprintf(MyDebB,"OUTEX%d OK", NamOut[i]);			//	.. se le altre uscite sono tutte a "1"
				LoggaStampa(MyDebB);
				//MSGBOXCALL("Controllare UScita", 0, 1, 0, 0, bufresponse);
			}else{
				Failed++;
				if (outOK_ != 0) {									// se l'uscita testata NON è a "0"
					sprintf(MyDebB,"OUTEX%d in Avaria: letta DISATTIVA)", NamOut[i]);
				}else{
					sprintf(MyDebB,"OUTEX%d 0x%x 0x%x", NamOut[i],confl,MaskOut);
					sprintf(&MexAll[strlen(MexAll)], "%sOUTEX%d(%d)", separ, NamOut[i], i);
					sprintf(separ,", ");
				}
				/*
				if (confl != MaskOut) {								// ed inoltre si segnalano quali altre uscite risultano Attive
					StampaDBn("confl", confl);					    // Su finestra DOS di Debug 
					cnf = (~(confl) & MaskOut);
					j = 0;											// si individuano quali sono le uscite in conflitto tra loro
					sprintf(MexAll, "%s", ""); sprintf (separ, "%s", "");
					while (cnf != 0) {
						if (cnf & 0x01) {
							sprintf(&MexAll[strlen(MexAll)], "%sOUTEX%d(%d)", separ, NamOut[j], j);
							sprintf(separ,", ");
						}
						cnf >>= 1 ; j++;
					}
					sprintf(MyDebB,"Conflitto tra OUTEX%d e %s", NamOut[i], MexAll);
				}
				*/
				LoggaStampa(MyDebB);
				sprintf(MyDebB,"OUTEX%d K.O.", NamOut[i]);	//	.. segnala anonalia
				LoggaStampa(MyDebB);
			}
			M_Output(matc, 0);										// Terminato il test, disattiva l'i-esima uscita di MTS ...
			i++; matc <<= 1;										// ... e si passa all'uscita successiva
		}
		M_Output(ALLOUT, NOOUT);									// Disattiva tutte le uscite dell'MTS
		//OUT ERROR
		if ( strlen(MexAll)>0) {
			sprintf(&MexAll[strlen(MexAll)], " K.0.");
			MSGBOXCALL(MexAll,0,2,"Continua","Ferma",bufresponse);	// (0609) aggiunto (è come Test Pull_Down) per permettere uscita
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore INEX");
			}
		}		
		
	//	======================================================================
	//	TEST dell'uscita RELE' dell'MTS
	//	======================================================================

#ifdef CHECK_TESTTIME
		MsgFile(0, "tempi_collaudo.txt", "Test Relé") ;
#endif		
		
		MaskOut += REL1*(TestSet.QtaRL*2-1);						// aggiorno la Maschera delle uscite aggiungendo anche i Rele'
		M_Output((REL1+REL2), 0);									// Disattiva i 2 Rele' dell'MTS
		if (TKTYPE==0) T_Output(OLDTIN_PUP, OLDTIN_PUP);					// tutti gli ingressi di Board-T-K sono configurati con Pull-UP
		ExCh = TOD1;												// prima uscita del T-K da eccitare per i test dei Relè
		for (i=1; i<=TestSet.QtaRL; i++) {
			T_Output(TK_MASKOUT, TRL2_);							// Setta tutte le uscite del T-K a riposo e libera CN2 di T-K (LNO2 di T-K è fisso a '0')
			MaskRl = REL1*i;
//																		Rele': test del contatto Normal Close
			k = 0;													// x registrare anomalie di Rele'
			Delay(25);												// Attesa del settaggio
			Lett = (T_Input() & MaskOut);
			StampaDBn("Lett", Lett);					// Su finestra DOS di Debug 
			StampaDBn("MaskRl", MaskRl);				// Su finestra DOS di Debug 
			StampaDBn("MaskOut", MaskOut);				// Su finestra DOS di Debug 
			if (Lett != MaskOut) {									// Testa uno qualsiasi di LNCi di MTS (1° e 2° Rele') NON è a "1"
				if ((Lett | ~MaskRl) != MaskOut) {					// Testa se LNCi di i-esimo Relè di MTS NON è a "1"
					k++;
					sprintf(buflog, "Relay-LNC%d in Avaria (Contatto NC bloccato a <0>)", i+1);
					LoggaStampa(buflog);
				}
			}
			T_Output(ExCh, ExCh);									// imposta LNCi di MTS a "0"
			ExTmp = ExCh;
			ExCh <<= 1;												// si punta alla prossima eccitazione
			Delay(25);
			if (((T_Input() & MaskOut) | ~MaskRl) == MaskOut) {		// Testa se LNCi di MTS NON è a "0"
				k = k + 2;
				sprintf(buflog, "Relay-CN-LNC%d in Avaria (Contatto CN NON corretto)", i+1);
				LoggaStampa(buflog);
			}
//	 																Rele': test del contatto Normal Open
			M_Output(MaskRl, MaskRl);								// Attiva rele' dell'MTS per testare LNOi del Rele'
			T_Output(ExTmp, 0);										// Libero LNCi di MTS 
			Delay(25);										
			Lett = (T_Input() & MaskOut);
			StampaDBn("Lett", Lett);								// Su finestra DOS di Debug 
			if (Lett != MaskOut) {									// Testa uno qualsiasi di LNOi di MTS (1° e 2° Rele') NON è a "1"
				if ((Lett | ~MaskRl) != MaskOut) {					// Testa se LNOi di i-esimo Relè di MTS NON è a "1"
					k = k + 4;
					sprintf(buflog, "Relay-LNO%d in Avaria (Contatto NO bloccato a <0>)", i+1);
					LoggaStampa(buflog);
				}
			}
			ToSt = ExCh *(i-1);										// la 1° volta ToSt è 0,  poi la 2° è ExCh !!!!! 
			T_Output(ExCh, ToSt);									// imposta LNOi di MTS a "0"
			ExCh <<= 1;												// si prepara la prossima eccitazione
			Delay(25);
  	
			if (((T_Input() & MaskOut) | ~MaskRl) == MaskOut) {		// Testa se LNOi di MTS NON è a "0"
				k = k + 8;
				sprintf(buflog, "Relay-CN-LNO%d in Avaria (Contatto CN NON corretto)", i+1);
				LoggaStampa(buflog);
			}
			M_Output(ALLOUT, NOOUT);								// Disattiva tutte le uscite dell'MTS
			T_Output(TK_MASKOUT, TRL2_);							// Disattiva tutte le uscite del T-K
			if (k == 0){
		 		LoggaStampa("Relay O.K.");
			}else{
				Failed++;
				sprintf(MyDebB,"ATTENZIONE: %d° Rele' con problema (0x0%x)", i, k);
				LoggaStampa(MyDebB);
				sprintf(MyDebB, "%d° RELE' IN AVARIA", i);
		 		MSGBOXCALL(MyDebB,0,2,"Continua","Ferma",bufresponse);
		 		if (strcmp(bufresponse,"#!")==0) {
					call_exit(YES, "Errore Relay-N0");
				}
			}
		}
	}
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
	
	return Failed;
}

int SK_TaraCurr(void)
{
//	____________________________________________________________________________________________
//	Per come è realizzata la struttura HW del T-K OLD il canale Analog. 'n' è indirizzato con ~('n')
//	============================================================================================
	int i, Failed,chref;
	Failed = 0;
	float an_rd, AnLevel ;

	OuputText(1,"TARATURA Corrente ANALOGICA 19mA",0,0,0,0);
	chref=6;
	if (TKTYPE==0) chref=~(chref);
	T_Output(CHN, chref);				// posiziona il "puntatore di canale" sul 7ª uscita analogica T-K
										// che è connessa al 1° input di Board-T-K (VREF)
	T_Output(SETAN, TO_VOLT);			// abilita le uscite analogiche del T-K in Tensione
										// (e tutte le uscite dig. del T-K come "flottanti")
	T_Output(AN_DIG_, AN_DIG_);			// Abilita le uscite analogiche
	Delay(25);							// Attesa di 2.5 sec perchè si stabilizzi l'uscita An
	MsgWindow("TARARE su TP2-TP1 a 2.85 V\rRuotando TR1 in senso antiorario\r");
	while (Failed == 0) {
		i=60;														// 60 sec.  
		while (i>0) {												// sessione di 1 minuto per tarare TR1
			an_rd = (float)T_Analog(VREF)*3.0/1023.0; 					// Tensione tarata
			AnLevel = an_rd/0.15;									// AnLevel = val. in mA ottenuti
			sprintf (MyDebB,"%5.2f", an_rd);
			sprintf (Bmom,"%5.2f", AnLevel);
			OuputText(8,MyDebB,3,"Volt",1,9);
			OuputText(9,Bmom,0,"mA",0,0);
			sprintf(bufwindow,"Taratura: %5.2f Volt (%5.2f mA)", an_rd, AnLevel);
			MsgWindow(bufwindow);
			Delay(9);
		}
		MSGBOXCALL("TARATURA Eseguita?", "Taratura 19mA",2, "Continua", "Finito", bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(NO, "EndTar");
		}
	}
	return Failed;
}


//	======================================================================
//					Test INGRESSI ANALOGICI 4-20 mA
//	======================================================================
float GetAnlRef(void)
{
	int tmpc ;  // i
	unsigned int ch_ref ;
	float TkRefer, retval, an_level ;
	
	ch_ref = ( (TestSet.QtaInA>0)? IREF:VREF) ;
	
	/* //Ordini di Sgueglia rimuovere PreLettura
	// Si eseguono le letture del reference
	TkRefer= 0;
	i=0;										// Si eseguono <Media> misure del riferimemento PRIMA la lettura eseguite con l'MTS
	while (i<2) {							// si misura il riferimento a 19mA generando un accumulo che deternina la media finale
		tmpc = T_Analog(ch_ref);					// valore ADC della misura di tensione su 150 Ohm (del T-K)
		sprintf(MyDebB, "PRELettura %d REF: %d", (i+1), tmpc);
		StampaDB ("", MyDebB);
		Delay(10);
		i++;
	}
	i = 0 ;
	while (i<2) {							// si misura il riferimento a 19mA generando un accumulo che deternina la media finale
		tmpc = T_Analog(ch_ref);					// valore ADC della misura di tensione su 150 Ohm (del T-K)
		TkRefer += tmpc;
		sprintf(MyDebB, "Lettura %d REF: %d", (i+1), tmpc);
		StampaDB ("", MyDebB);
//		TkRefer += T_Analog (IREF);				// ... in <Media> secondi
		Delay(10);
		i++;
	}
	*/ //Ordini di Sgueglia rimuovere PreLettura
	//AGGIUNTO AL POSTO di PRELETTURA
		TkRefer= 0;
		tmpc = T_Analog(ch_ref);					// valore ADC della misura di tensione su 150 Ohm (del T-K)
		TkRefer += tmpc;
		sprintf(MyDebB, "Lettura %d REF: %d", 1, tmpc);
		StampaDB ("", MyDebB);
		//TkRefer += T_Analog (IREF);
	//AGGIUNTO AL POSTO di PRELETTURA
	if (TestSet.QtaInA>0){
		retval = ((TkRefer)*3000.0/1023.0)/150.0 ;		// Valore in mA della corrente imposta
		an_level = (retval-4.0)/16.0*1023.0;				// AnLevel = val. [0..1023 --> 4..20mA] x 19mA (tarati con trimmer sul T-K)
		sprintf (MyDebB,"Reference Corrente (media): %5.0f (%5.2f mA)", an_level, retval);
	}else{	// In tensione
		retval = ((TkRefer)*3.0/1023.0) ;		// Valore in Volt
		an_level = (TkRefer) ;
		sprintf (MyDebB,"Reference Tensione (media): %5.0f (%5.2f V)", an_level, retval);
	}
	LoggaStampa (MyDebB);
	MsgWindow(" ");
	
	return(retval) ;
}

/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/

int SK_Test_AnlIn(void)
{
	int Failed = 0;
	int i,ch; //,j
	int tmpc ; 
	unsigned int nrIngressi, chref, out_type ; 			// Settati a seconda se in corrente od in tensione
	char Dato[MINSIZE];
	int SiConf, Media, AnCoef, OldCoeff;
	float TkRefer, MsRefer, VadC;				// conterranno i valori medi [0..1023] di corrente (fissa) misurati da Test-kit e Mts
	float Cur_R, Cur_R1, an_rd ;	
	// unsigned char confl ;
	//float mAd_mean ;
	
	// AnLevel conterrà il valore di riferimento x 19mA (analogici) con interfaccia 4-20mA
	float diffcur_R, diffcur_R_per ; // , AnLevel ; // , difftemp_R, difftemp_R_per ;		
	if (!(strcmp(TkIni.mName, "2023"))){
			// PB29 a 0
		T_Output(TRL2_, 0); 					//	Attivare rele TK!
	}
	
	if (TestSet.EnAnlIn == NO) return Failed;	//	esci senza far nulla se il test non è richiesto  
	
	if (MtsTK.WTastiera) {
		sprintf(buflog, "TEST ANALOGICI non fattibile per %s", TkIni.mName);
		LoggaStampa(buflog);
		MtsTK.steptest++ ;
		return Failed;							//	esci se NON è uno dei modelli qui testati !!!
	}

	// "Ingressi analogici"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;

	if (TKTYPE==0) {
		if (TestSet.QtaInA>6) TestSet.QtaInA = 6 ;			// non si possono gestire + di 6 Analogici
		if (TestSet.QtaInV>6) TestSet.QtaInV = 6 ;			// non si possono gestire + di 6 Analogici
	}else{
		if (TestSet.QtaInA>14) TestSet.QtaInA = 14 ;		// non si possono gestire + di 6 Analogici
		if (TestSet.QtaInV>14) TestSet.QtaInV = 14 ;		// non si possono gestire + di 6 Analogici
	}
	chref=0;
	out_type=0;
	if (TestSet.QtaInA>0){
		nrIngressi = TestSet.QtaInA ;
		chref = 7 ;
		out_type = TO_CURR ;
		i = (nrIngressi+1) * 160 ;
	}else{
		nrIngressi = TestSet.QtaInV ;
		chref = 6 ;
		out_type = TO_VOLT ;
		i = (nrIngressi+1) * 160 ;
	}
	ProgressBar(BAR_TIME, i ) ; // 96 sec
	sprintf(Bmom, "%d", chref );
	StampaDB ("CHREF:",Bmom);

//	____________________________________________________________________________________________
//	Per come è realizzata la struttura HW del T-K OLD il canale Analog. 'n' è indirizzato con ~('n')
//	============================================================================================
	SiConf = 0;
	TkRefer= 0;
	MsRefer= 0;
	Media = 2;									//  <----- Numero di letture per misurare la Media di valore analogico
	sprintf(Bmom, "(%d_E:%d) TEST ANALOGICI", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	if (out_type == TO_CURR)
		LoggaStampa("TEST degli INPUT Analogici (4-20 mA)");
	else
		LoggaStampa("TEST degli INPUT Analogici (0-5 V)");
	if (TKTYPE==0) chref=~(chref);
	sprintf(Bmom, "%lld", CHN );
	StampaDB ("CHN:",Bmom);
	sprintf(Bmom, "%d", chref );
	StampaDB ("CHREF:",Bmom);
	T_Output(CHN, chref);		// posiziona il "puntatore di canale" sul 8ª/7ª uscita analogica T-K
								// che è connessa al 2°/1° input di Board-T-K (IREF/VREF)
	if (TKTYPE==0){								
		T_Output(SETAN, out_type);		// abilita le uscite analogiche del T-K in Corrente/Tensione (e tutte le uscite dig. del T-K come "flottanti")

		T_Output(AN_DIG_, AN_DIG_);		// Abilita le uscite analogiche
	}else{
		T_Output(EN_CNT, 0);				  // Disattivo il timer di T-K di 1 KHz
		T_Output(NEWALLTOVFG , NEWALLTOVFG ); // uscite TK Flottanti
		T_Output(NEWFLOAT_ , 0);		      // uscite TK Flottanti
		
		T_Output(SETAN, out_type);		// abilita le uscite analogiche del T-K in Corrente/Tensione (e tutte le uscite dig. del T-K come "flottanti")
	}
	Delay(25);						// Attesa di 2.5 sec perchè si stabilizzi l'uscita An
	
//// Si eseguono 6 letture scratch di stabilizzazione
//	i=0;							// Si eseguono <Media> misure del riferimemento PRIMA la lettura eseguite con l'MTS
//	while (i<2) {					// si misura il riferimento a 19mA/2.9V generando un accumulo che deternina la media finale
//		//tmpc = T_Analog(IREF);	// valore ADC della misura di tensione su 150 Ohm (del T-K)
//		tmpc = GetAnlRef() ;
//		sprintf(MyDebB, "PreLettura %d REF: %d", (i+1), tmpc);
//		StampaDB("", MyDebB);							// Su finestra DOS di Debug 
//		Delay(10);
//		i++;
//	}
// Riferimento in corrente 19.0 mA
// Riferimento in tensione 
//	-------------------------------------------------------------------------------------------
	i=0;
	if ( (!strcmp(TkIni.mName, "3025")) && out_type == TO_VOLT ){
		i=3;
		nrIngressi=nrIngressi+i;
	}
	while (i < nrIngressi) {					// il 3008 ha 6 ingressi analogici (da 0 a 5)
		if ((i>5) && (TKTYPE==1)) T_Output(NEWAN1ORAN2, NEWAN1ORAN2);
		T_Output(CHN, chref);				    // posiziona il "puntatore di canale" sul 8ª uscita analogica T-K
		Delay(30);								// si rinfrescano i valori analogici entro 3 sec (0610 era 25)
		Cur_R = GetAnlRef() ; // _FR_
		if ((i>5) && (TKTYPE==1)) {
			T_Output(NEWAN1ORAN2, 0);
			ch=i-6;
		}else{
			ch=i;
		}
		if (TKTYPE==0) ch=~(ch);						// per la struttura HW del T-Kold il canale Analog. 'n' è indirizzato con ~('n')
		T_Output(CHN, ch);					    
		SetProtocolMode(KPROTOCOLOMODE_QUERYANSWER);	// Imposta la modalità di analogici aggiornati
		Delay(10);										// si rinfrescano i valori analogici entro 3 sec (0610 era 25)
		if (out_type == TO_CURR)
			sprintf(MyDebB,"Corrente su CHN-%d", i);
		else
			sprintf(MyDebB,"Tensione su CHN-%d", i);
		StampaDB(MyDebB,"");	
		// Nuovo da 1.16: 3 letture e media
		/*
		j=0;										// Si eseguono <Media> misure del riferimemento PRIMA la lettura eseguite con l'MTS
		mAd_mean = 0.0 ;
		while (j<3) {							// si misura il riferimento a 19mA generando un accumulo che deternina la media finale
			if (j==2) SetProtocolMode(KPROTOCOLOMODE_BUFFERED);	// Imposta la modalità di analogici bufferizzati
			an_rd = ReadAnalog(i);
			if (j) mAd_mean += an_rd ;
			sprintf(MyDebB, "Lettura %d REF:  %4.0f", (j+1), an_rd);
			StampaDB ("", MyDebB);
			Delay(5); //prima 10
			j++;
		}
		an_rd = (mAd_mean /2.0) ; // Prima lettura non usata
		// end nuovo tolto sgueglia
		*/
		// Su finestra DOS di Debug 
		SetProtocolMode(KPROTOCOLOMODE_BUFFERED);	// Imposta la modalità di analogici bufferizzati
		if ( (!strcmp(TkIni.mName, "3025")) && out_type == TO_VOLT ){
			SetProtocolMode(KPROTOCOLOMODE_QUERYANSWER);
			DatoMTS = M_ReadIntAD();
			Delay(10);
			SetProtocolMode(KPROTOCOLOMODE_BUFFERED);
			Delay(10);
			an_rd=0;
			if (i==3) an_rd = DatoMTS.adv1;
			if (i==4) an_rd = DatoMTS.adv2;
			an_rd= an_rd * 1023.0 / 500.0 ;
		}else{
			an_rd = ReadAnalog(i);						// misura di corrente dell'MTS nel range 4..20 mA
		}
		sprintf(MyDebB,"AN:%4.0f",an_rd);
		MsgWindow (MyDebB);
		MsRefer += an_rd;
		if (out_type == TO_CURR){
			Cur_R1 = (an_rd*16.0/1023.0)+4.0;					// Valore in mA della corrente letta dall'MTS
			sprintf(Bmom,"Analogico %d: %4.0f (%5.2f mA)", i, an_rd, Cur_R1);		
			diffcur_R = fabs(Cur_R - Cur_R1);
			diffcur_R_per = (diffcur_R/16.0)*100.0;
			sprintf(MyDebB,"%s  [Diff %5.2f mA (%1.2f %%)]", Bmom, diffcur_R, diffcur_R_per);		
		}else{
			Cur_R1 = an_rd * 5.0 /1023.0;					// Valore in V della tensione letta dall'MTS
			sprintf(Bmom,"Analogico %d: %4.0f (%5.2f V)", i, an_rd, Cur_R1);		
			diffcur_R = fabs(Cur_R - Cur_R1);
			diffcur_R_per = (diffcur_R/5.0)*100.0;
			sprintf(MyDebB,"%s  [Diff %5.2f V (%1.2f %%)]", Bmom, diffcur_R, diffcur_R_per);		
		}
		
		//if (diffcur_R_per > AN_DELTAPERC) {
		if (diffcur_R_per > TestSet.An_Perc) {
			//sprintf(MyDebB,"Avaria ANIN%d letto con Valore: %5.2f mA (%5.2f mA)\r", i, Cur_R1, Cur_R);
			LoggaStampa(MyDebB);
			MSGBOXCALL (MyDebB,0,2,"Continua","Ferma",bufresponse);	// (0609) aggiunto (è come Test Pull_Down) per permettere uscita
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore ANIN");
			}
			sprintf(MyDebB,"ANIN%d K.O. x lettura", i);	//	.. segnala anonalia
			Failed++;
		}
		LoggaStampa(MyDebB);
		/*
		confl = 0;
		sprintf(MexAll, "%s", ""); 
		sprintf(separ, "%s", "");
		j=0;
		if (strcmp(TestSet.Cpu,"HC12")){
			while (j < nrIngressi) {
				if (j != i) {
	//				Delay(10);		
					an_rd = ReadAnalog(j);
					if (out_type == TO_CURR){
						Cur_R1 = (an_rd*16.0/1023.0)+4.0;				
	#ifdef CBUG_
						sprintf(MyDebB,"Controllo conflitto con Analogico %d : %5.0f (%5.2f mA)", j, an_rd, Cur_R1);		
						StampaDB(MyDebB,"");						// Su finestra DOS di Debug 
	#endif // #ifdef CBUG
						diffcur_R = fabs(4 - Cur_R1);
	#ifdef CBUG_
						sprintf(MyDebB,"Differenza conflitto %5.2f",diffcur_R);		
						StampaDB(MyDebB,"");						// Su finestra DOS di Debug 
	#endif // #ifdef CBUG
						if (diffcur_R > DELTA_AN_I) {
							confl ++; 
							sprintf(&MexAll[strlen(MexAll)], "%sANIN%d(%5.2f)", separ, j, Cur_R1);
							sprintf(separ,", ");
							Failed++;
						}
					}else{
						Cur_R1 = an_rd * 5.0 /1023.0;
	#ifdef CBUG_
						sprintf(MyDebB,"Controllo conflitto con Analogico %d : %5.0f (%5.2f V)", j, an_rd, Cur_R1);		
						StampaDB(MyDebB,"");						// Su finestra DOS di Debug 
	#endif // #ifdef CBUG
						diffcur_R = fabs(Cur_R1);
	#ifdef CBUG_
						sprintf(MyDebB,"Differenza conflitto %5.2f",diffcur_R);		
						StampaDB(MyDebB,"");						// Su finestra DOS di Debug 
	#endif // #ifdef CBUG
						if ( (diffcur_R > DELTA_AN_V) && (strcmp(TestSet.Cpu,"HC12")) ){
							confl ++; 
							sprintf(&MexAll[strlen(MexAll)], "%sANIN%d(%5.2f)", separ, j, Cur_R1);
							sprintf(separ,", ");
							Failed++;
						}
						if ( (diffcur_R > DELTA_AN_VHC12) && (!strcmp(TestSet.Cpu,"HC12")) ){
							confl ++; 
							sprintf(&MexAll[strlen(MexAll)], "%sANIN%d(%5.2f)", separ, j, Cur_R1);
							sprintf(separ,", ");
							Failed++;
						}
					}
				}
				j++;
			}										// chiude il while del 'j'
		} // Not HC12
		if (confl > 0) {
			sprintf(MyDebB,"Conflitto tra ANIN%d e %s", i, MexAll);
			LoggaStampa(MyDebB);
			sprintf(MyDebB,"ANIN%d K.O. x conflitto", i);	//	.. segnala anonalia
			LoggaStampa(MyDebB);
			SiConf++;
		}
		if ((confl != 0) || (Failed != 0)) {			//.. se su i-esimo ANIN ci sono stati errori ..
			MSGBOXCALL (MyDebB,0,2,"Continua","Ferma",bufresponse);	// (0609) aggiunto (è come Test Pull_Down) per permettere uscita
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore ANIN");
			}
		}
		*/
		i++;
	}													// chiude il while del 'i'
	
	if ((SiConf == 0) && (Failed == 0)) {				//.. se non ci sono stati errori per tutti gli ingressi
		if (TestSet.QtaInA == 1)
			sprintf(MyDebB,"--> ANIN0 O.K.");
		else
			sprintf(MyDebB,"--> ANIN0..ANIN%d O.K.", TestSet.QtaInA-1);
		if (!(strcmp(TkIni.mName, "2023"))){
			// PB29 a 1
			T_Output(TRL2_,TRL2_); //Disattiva rele TK!
	   	}
		
		LoggaStampa(MyDebB);
// -----------------------------------------------------------------------------------
// Elaborazione per il calcolo del coefficiente di taratura del Conv AD 
		if (TestSet.AnlTaratura){
			if (TKTYPE==1) T_Output(NEWAN1ORAN2, NEWAN1ORAN2);
			chref=7;
			if (TKTYPE==0) chref=~(chref);
			T_Output(CHN, chref);							// posiziona il "puntatore di canale" sul 8ª uscita analogica T-K
			Delay(25);
			i=0; 											// Si eseguono altre <Media> misure del riferimemento DOPO la lettura eseguite con l'MTS
			while (i<Media) {								// si misura il riferimento a 19mA generando un accumulo che deternina la media finale
				tmpc = T_Analog(IREF);
				TkRefer += tmpc;
				sprintf(MyDebB, "Lettura %d REF: %d", (i+1+Media), tmpc);
				StampaDB("", MyDebB);						// Su finestra DOS di Debug 
	//			TkRefer += T_Analog (IREF);					// ... in <Media> secondi
				Delay(10);
				i++;
			}
			TkRefer /= (((float)(Media))*2.0);							// valore ADC mediato della misura di tensione su 150 Ohm (del T-K)
			M_GetPar(127, Dato);							// il parametro 127 è il coefficiente già memorizzato!!
			if (!strcmp(Dato,"N.D.")) sprintf(Dato,"0");			// se il coefficiente non è ancora stato settato è come se fosse '0'
			OldCoeff = atoi(Dato);
			sprintf(MyDebB,"Coefficiente Analog.  precedente: %s", Dato);
			LoggaStampa(MyDebB);
	//				StampaDBn("Vecchio coeff", OldCoeff);				// Su finestra DOS di Debug 
			MsRefer /= TestSet.QtaInA;								// calcola la media dei valori di misura della corrente fissa eseguiti dall'MTS
			StampaDBn("Valore MTS (solo media)", (int)MsRefer);			// Su finestra DOS di Debug 
			VadC = ((MsRefer*16.0/1023.0+4.0)*15.0*1023.0/300.0); // si depura il valore mediato degli ingressi Analogici ...
			StampaDBn("Valore MTS (intermedio)", (int)VadC);		// Su finestra DOS di Debug 
			MsRefer = VadC / (1.0+OldCoeff/100000.0);		// ... dal coefficiente già memorizzato   
			AnCoef = ((TkRefer/MsRefer-1.0)*100000.0);		// calcola il nuovo coefficiente di taratura
			sprintf(Dato, "%d", AnCoef);
			sprintf(MyDebB,"Calcolo Coefficiente Anal. nuovo: %s", Dato);
			LoggaStampa(MyDebB);
//		if (TestSet.AnlTaratura) {
			i=2;										// numero di retry!
			while (i>0) {
				M_Diag(127, AnCoef, MyDebB); 			// memorizzo il coefficiente nell'MTS
				if (strcmp(Dato, MyDebB)==0){
					i = -1;
				}else{
					Delay(10);
					i--;
				}
			}
			if (i==0) {									// non è riuscito a memorizzare il coefficiente !!!!
				sprintf(MyDebB,"ATTENZIONE: Nuovo Coefficiente Anal. (%s) NON memorizzato!", Dato);
				LoggaStampa(MyDebB);
				Failed++;
			}else{
				sprintf(MyDebB,"Coefficiente Anal. aggiornato : nuovo valore -> %s", Dato);
				LoggaStampa(MyDebB);
			}
		}
	}
	SetProtocolMode(KPROTOCOLOMODE_QUERYANSWER);		// Imposta la modalità di analogici aggiornati
	if (TKTYPE==0) T_Output(AN_DIG_, 0);

	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed;
}

//	======================================================================
//					Test VEXT
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_Vext(void)
{
	int k, Failed = 0;
	//int Vdiodo = 0;
	float VextMTS, dpercOK ;
	float diffVext_R, diffVext_R_per ;
	
	if (TestSet.EnVext == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	//dpercOK = AN_DELTAPERC ;
	dpercOK = TestSet.An_Perc;
	// "Alimentazione"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	ProgressBar(BAR_PERC, 0 ) ;
	
	sprintf(Bmom, "(%d_E:%d) TEST Lettura di Vext", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	LoggaStampa("TEST Lettura Vext");
	k=T_Analog(TVEXT);
	MtsTK.VextTK=(float)k*3/1023.0*1056.0/56.0 ; // -0.2;					// Valore (in V) di Vext sentito da T-K
	k=M_Analog(A_VEXT);
	if (k==0) {
		Delay(40);
		M_Analog(A_VEXT);
	}
	/*//DEBUG
	int pippo2;
	pippo2=k &0xf;
	sprintf(Bmom,"PASSOdaTESTALIM k=%d(0x%x)_SELECT:%d\n", k,k,pippo2);
	PrintDB(Bmom);
	*///DEBUG
	switch (k &0xf){
		case 8:
			VextMTS =(((float) (k>>6)) / 1024.0) * 100.0 ; break ;
		case 4:
			VextMTS =(k>>8) ; break ; // (8 bit) -> only  MTS01
		case 2:
			VextMTS =(((float) (k>>6)) * 55.0 / 1024.0) + 0.74 ; break ;// as MTS40 (10 bit)
		case 1:
			VextMTS =(((float) (k>>8)) * 5.0 * 5.83 * 2.044 / 255.0) ;
			VextMTS -= 1.1 ;
			break ;// as HC12 extended range (8 bit)
		default:
			VextMTS =(((float) (k>>8)) * 5.0 * 5.83 / 255.0) ; 
			VextMTS -= 1.1 ;
			break ;// as HC12 WITHOUT extended range (8 bit)
	}
	if (!(strncmp(TkIni.mName, "MTS",3))) dpercOK *= 3.0 ;
	/*//DEBUG
	sprintf(Bmom,"PASSOdaTESTALIM VextMTS=%5.2f\n", VextMTS);
	PrintDB(Bmom);
	*///DEBUG
	
////	VextMTS= (float)k/64/1024*3*18.85+0.44;	
//	if (strcmp(TkIni.mName, "4004") == 0 ||
//		strcmp(TkIni.mName, "4104") == 0 ||
//		strcmp(TkIni.mName, "3008") == 0 ) Vdiodo = 0.2;
//	VextMTS= (float)k/64/1024*55 + (float)Vdiodo;		// Valore (in V) di Vext sentito dall'MTS
	diffVext_R = fabs(VextMTS - MtsTK.VextTK);
	diffVext_R_per = fabs((VextMTS / MtsTK.VextTK)-1.0)*100.0;
	
	if (TestSet.EnCOM2)
		ProgressBar(BAR_PERC, 50 ) ;
	else
		ProgressBar(BAR_PERC, 100 ) ;

	if (diffVext_R_per > dpercOK) {
		sprintf(MyDebB,"ATTENZIONE!!! Vext letto con Valore: %5.2f Volt (%5.2f Volt,0x%x)" , VextMTS, MtsTK.VextTK, k);
		LoggaStampa(MyDebB);
		MSGBOXCALL("VEXT IN AVARIA",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore VEXT");
		}
		Failed++;
	}else{
		sprintf(MyDebB,"--> VExt O.K. %4.2f Volt (%4.2f Volt)  [Diff. %4.2f V (%1.2f %%)]", 
														VextMTS, MtsTK.VextTK, diffVext_R, diffVext_R_per);
		LoggaStampa(MyDebB);
	}

	// Esegue anche il test della Vcns
	if (TestSet.EnVcns){
		//MSGBOXCALL("Controllare VCNS", 0, 1, 0, 0, bufresponse);
		ProgressBar(BAR_PERC, 100 ) ;
		int MOK_CNS_;
		MOK_CNS_=(T_Input() & OK_CNS_);
		if (TKTYPE==1) MOK_CNS_^=OK_CNS_;
		if (MOK_CNS_==OK_CNS_) {				// se MOK_CNS_ è "1" allora NON c'è l'alimentazione in uscita
			LoggaStampa("ATTENZIONE: Alimentazione per console in Avaria");
			MSGBOXCALL("ATTENZIONE: Alimentazione per console in Avaria",0,2,"Continua","Ferma",bufresponse);	
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore AlCOM2");
			}
			MTSdata.AlimFail++;
			Failed++;
		}else{
			if  (MTSdata.AlimFail == 0){
				LoggaStampa("--> Alimentazione per COM Secondaria O.K.");
			}else{
				LoggaStampa("ATTENZIONE2: Alimentazione per console in Avaria");
				MSGBOXCALL("ATTENZIONE: Alimentazione per console in Avaria",0,2,"Continua","Ferma",bufresponse);	
				if (strcmp(bufresponse,"#!")==0) {
					call_exit(YES, "Errore AlCOM2");
				}
				Failed++;
			}
		}
		M_Action(32,3,"") ; 	// Disattiva Vcns
		M_SetPar(77, "0");		// si imposta lo stato dell'alimentazione di COM1 in "OFF" (HC12)
		if (!strcmp(TestSet.Cpu,"HC12")) M_SetPar(228,"16000") ;
	}
	
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed;
}

//	======================================================================
//					Test VBatt
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_Vbat(void)
{
	int Failed = 0;
	int Failed1 = 0;
	
	if (TestSet.EnVbat == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	// "Batteria"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	ProgressBar(BAR_PERC, 0 ) ;

	sprintf(Bmom, "(%d_E:%d) TEST Lettura di Vbat", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	LoggaStampa("TEST Lettura Vbat");

	MTSdata.VbatMTS = ReadMTSbatt() ;					// valore Vbat sentinto dall'MTS
// 								 /64/1024*3*1.482
	ProgressBar(BAR_PERC, 100 ) ;
	if (MTSdata.VbatMTS > MtsTK.VbatMAX) {
		sprintf(MyDebB,"ATTENZIONE!!!Vbat %5.2f sopra il limite(%.2fV)" , MTSdata.VbatMTS, MtsTK.VbatMAX);
		LoggaStampa(MyDebB);
		MSGBOXCALL("Batteria IN AVARIA",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			if (TestSet.EnVbat == NO) {
				call_exit(NO, "Errore VEXT");
			}else{
				call_exit(YES, "Errore VEXT");
			}
		}
		if (TestSet.EnVbat == NO) {
				Failed1=1;
		}else{
				Failed++;
		}
	}else if (MTSdata.VbatMTS < MtsTK.VbatMIN) {
		sprintf(MyDebB,"ATTENZIONE!!!Vbat %5.2f sotto il limite(%.2fV)" , MTSdata.VbatMTS, MtsTK.VbatMIN);
		LoggaStampa(MyDebB);
		MSGBOXCALL("Vbat IN AVARIA",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			if (TestSet.EnVbat == NO) {
				call_exit(NO, "Errore VEXT");
			}else{
				call_exit(YES, "Errore VEXT");
			}
		}
		if (TestSet.EnVbat == NO) {
				Failed1=1;
		}else{
				Failed++;
		}
	}
	if ((Failed == 0) && (Failed1 == 0) ) {
		sprintf(MyDebB,"--> Vbat O.K. - Sentito da MTS: %4.2f Volt", MTSdata.VbatMTS);
		LoggaStampa(MyDebB);
	}
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed;
}

//	======================================================================
//					Test Counter
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_Cnt(void)
{
	int k, k0, k1, Failed = 0;
	int at1, cntr ;
	int Efreq, cnt_id ;
	
	if ( (TestSet.QtaCnt == 0) && (TestSet.EnExtLED == 0) )
		return Failed;			// esci senza far nulla se il test non è richiesto  
	
	// "Counter"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	k = 20 ; // 2 sec se solo 'EnExtLED' (forfettario perchè lo esegue subito)
	if (TestSet.QtaCnt) k = 100 ;
	
	ProgressBar(BAR_TIME, k ) ; // 10s

	if (TestSet.EnExtLED){
		unsigned int act_cnt ;
		
		sprintf(Bmom, "(%d_E:%d) TEST LED ESTERNO", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgWindow(" ");
		LoggaStampa("TEST del LED ESTERNO");
		
		T_Output (EN_CNT,0);				// Disattivo il timer di T-K di 1 KHz
		act_cnt = T_Cnt(1) ;
		if (act_cnt == MTSdata.TKcntStart){ // Errore
			sprintf(MyDebB,"ATTENZIONE:  LED ESTERNO: fisso a %u", act_cnt);
			Failed++ ;
		}else
			sprintf(MyDebB,"LED ESTERNO ok: old %u new %u", MTSdata.TKcntStart, act_cnt);
		LoggaStampa(MyDebB);
	}
		
	if (TestSet.QtaCnt != 0){
	
		sprintf(Bmom, "(%d_E:%d) TEST COUNTER(s)", MTSdata.mSerial, MTSdata.ERRTest);

		//sprintf(MyDebB,"Quantità CNT:%d",TestSet.QtaCnt);
		//MsgWindow(MyDebB);
		for(cnt_id=1;cnt_id<=TestSet.QtaCnt;cnt_id++){
			//sprintf(MyDebB,"cnt_id:%d",cnt_id);
			//MsgWindow(MyDebB);
			if (cnt_id==1){
				if (!strcmp(TestSet.Cpu,"HC12"))	
					cnt_id = 0 ;
				else
					cnt_id = 1 ;
			}
			//	gapTm = atoi(CntToller); 				// Delta che tiene conto di un tempo costante legato all'elaborazione!!
			T_Output(EN_CNT, 0);						// Disattivo il timer di T-K di 1 KHz
			// Aggiunto ver.2.24
			if (TKTYPE==0){
				T_Output(SET_IN, IN_TOFLOAT);			// pone tutte le uscite del T-K come "flottanti"
			}else{
				T_Output(NEWFLOAT_ , 0);		        // Mette Flottante
				T_Output(NEWALLTOVFG , NEWALLTOVFG );	// Per Flottante
			}
			OuputText(1,Bmom,0,0,0,0);
			//	MODIFICATO tick DA 121 a 100
			MsgWindow(" ");
			LoggaStampa("TEST del Counter");
			k = M_Cnt(cnt_id);							// lettura iniziale del Counter dell'MTS
			sprintf(MyDebB,"Counter(%d) sec prima: %d|\r", cnt_id, k);
			StampaDB(MyDebB,"");						// Su finestra DOS di Debug 
			Delay(50);									// Attesa di un secondo: il contatore NON deve aumentare
			k0 = M_Cnt(cnt_id);							// lettura del Counter dell'MTS dopo 1 sec.
			if (k0>k+TestSet.CntToller){ // Un impulso è accettabile ?
				sprintf(MyDebB,"ATTENZIONE: Counter(%d): incrementato SENZA segnale (da %d a %d)", cnt_id, k,k0);
				LoggaStampa(MyDebB);
				if (strcmp(TkIni.mName, "3036")) Failed++; // se non è 3036
			}
			at1 = 3;									// sec della finestra di controllo del Counter
			Efreq = FREQ * TICK / 100;
			T_Output(EN_CNT, EN_CNT);					// Attivo il timer di T-K di 1 KHz
			Delay((at1*9));								// Attesa di 'at1' secondi: il contatore deve aumentare di 1000 !!
			T_Output(EN_CNT, 0);						// Disattivo il timer di T-K di 1 KHz
			Delay(30);									// Attendo 3 sec.
			k1 = M_Cnt(cnt_id);							// Leggo il contatore
			sprintf(MyDebB,"Counter(%d) con Gen. abilitato: %d|\r", cnt_id, k0);
			StampaDB(MyDebB,"");						// Su finestra DOS di Debug 
			sprintf(MyDebB,"Counter(%d) dopo %dsec: %d|\r", cnt_id, at1, k1);
			StampaDB(MyDebB,"");						// Su finestra DOS di Debug 
			cntr = k1 - k0 ;							// delta contatore
			// Su 4004 conta solo i fronti di salita
			if (!(strcmp(TkIni.mName, "4004"))){
				cntr *= 1.4 ;
			}else if (!(strncmp(TkIni.mName, "MTS",3))){
				cntr *= 1.4 ;
			}
			if (!(strncmp(TkIni.mName, "MTS",3))){
				if(cntr>0){
					sprintf(MyDebB,"--> CounterHC12(%d) è O.K. (letto=%d atteso=%d)", cnt_id, cntr, cntr);
					LoggaStampa(MyDebB);	
				}else{
					sprintf(MyDebB,"ATTENZIONE: CounterHC12(%d) in AVARIA: impulsi %d su %d (delta %d)", cnt_id, cntr, Efreq*at1, cntr-Efreq*at1);
					MSGBOXCALL("COUNTER IN AVARIA" ,0,2,"Continua","Ferma",bufresponse);
					if (strcmp(bufresponse,"#!")==0) {
						call_exit(YES, "Errore COUNTER");
					}
					LoggaStampa(MyDebB);
					Failed++;	
				}
			}else if ((cntr > (at1 * Efreq - TestSet.CntToller)) && (cntr < (at1 * Efreq + TestSet.CntToller))) {
				sprintf(MyDebB,"--> Counter(%d) è O.K. (letto=%d atteso=%d)", cnt_id, cntr, at1*Efreq);
				LoggaStampa(MyDebB);
				#ifdef DEBUG_1FR
				Repeat = 0 ;
				#endif		
			}else{
				sprintf(MyDebB,"ATTENZIONE: Counter(%d) in AVARIA: impulsi %d su %d (delta %d)", cnt_id, cntr, Efreq*at1, cntr-Efreq*at1);
				MSGBOXCALL("COUNTER IN AVARIA" ,0,2,"Continua","Ferma",bufresponse);
				if (strcmp(bufresponse,"#!")==0) {
					call_exit(YES, "Errore COUNTER");
				}
				LoggaStampa(MyDebB);
				Failed++;
			}
			//	k1 = M_Cnt(1);									// Leggo il contatore
			//	sprintf(MyDebB,"Counter(1) %d\r", k1);
			//	MsgWindow(MyDebB);
			if (!strcmp(TestSet.Cpu,"HC12"))	break ;
		}
	}
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	return Failed ;
}

//	======================================================================
//					Test GPS
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int GPS_Extern(void)
{
	if ( 
	    (TkIni.mName[0]=='3') || 
	    (TkIni.mName[0]=='4') || 
	    (!(strncmp(TkIni.mName, "MTS",3))) || 
	    (!(strcmp(TkIni.mName,"2202"))) || 
			(!(strcmp(TkIni.mName,"2034"))) || 
			(!(strcmp(TkIni.mName,"2039"))) ||
			(!(strcmp(TkIni.mName,"2040"))) ||
	    (!(strcmp(TkIni.mName,"2015"))) || 
	    (!(strcmp(TkIni.mName,"2020"))) || 
	    (!(strcmp(TkIni.mName,"2003"))) || 
	    (!(strcmp(TkIni.mName,"2102"))) 
	   )
		return 1;
	else
		return 0;
	
}

int SK_Test_GPS(void)
{
	int Failed = 0;
	int StFIX ;
	
	if (TestSet.EnGPS == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	// "Verifiche GPS"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;

	ProgressBar(BAR_TIME, 100 ) ;
	
	sprintf(Bmom, "(%d_E:%d) TEST GPS", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
//	
	Delay(15); 
	TesterMTS();
	MsgWindow(" ");
	LoggaStampa ("TEST del GPS");
	MTSdata.SMflags = M_InVirt();
	sprintf (MyDebB,"0x%x", MTSdata.SMflags);
	StampaDB("Virtuali",MyDebB);								// Su finestra DOS di Debug 
	if (!MTSdata.SMflags){
		MTSdata.SMflags = M_InVirt();
		sprintf(MyDebB,"0x%x", MTSdata.SMflags);
		StampaDB("Virtuali",MyDebB);							// Su finestra DOS di Debug 
	}		
	if ((MTSdata.SMflags & GPS_ON) != GPS_ON) {					// se il GPS non è alimentato (GPS_ON => '0')		
		LoggaStampa("ATTENZIONE: GPS NON Alimentato");
		MSGBOXCALL("GPS NON Alimentato" ,0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore ALIMENTAZIONE GPS");
		}
		Failed++;
	}else{
		MsgWindow("GPS Alimentato");
		MTSdata.SMflags = M_InVirt();
		StFIX = (MTSdata.SMflags & LST_FIX);
		int attesa=0;
		while( (StFIX =! (0x04 | 0x00)) && (attesa<15) ) {
			MTSdata.SMflags = M_InVirt();
			StFIX = (MTSdata.SMflags & LST_FIX);
			sprintf (MyDebB,"0x%x", (MTSdata.SMflags & GPS_ANT_));
			StampaDB("bit GPS_ANT_",MyDebB);	// Su finestra  DOS di Debug
			sprintf(MyDebB,"Attesa fix GPS = 0x%X",StFIX);
			OuputText(1,MyDebB,0,0,1,7);
			PrintDB(MyDebB);			
			attesa++;
			Delay(1);
		}
		if ((MTSdata.SMflags & GPS_ANT_) == GPS_ANT_) {				// se l'Antenna GPS è in Avaria ((GPS_ANT_ => '1')
			LoggaStampa("ATTENZIONE: Antenna GPS in Avaria");
			MSGBOXCALL("ANTENNA GPS IN AVARIA",0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore ANTENNA GPS");
			}
			Failed++;
		}else{
			LoggaStampa("Antenna GPS O.K.");
			StFIX = (MTSdata.SMflags & LST_FIX);
			sprintf(MyDebB,"0x%x", StFIX);
			StampaDB("Status FIX",MyDebB);							// Su finestra DOS di Debug 
			switch (StFIX){
				case 0x04 :
					MsgWindow("--> GPS O.K.: Fixing O.K. (meno di 10s)");
					MsgFile(0, LogCollaudo, "GPS O.K.: Fixing O.K. (meno di 10s)");
					break;
				case 0x00 :
					MsgWindow("--> GPS O.K.: Fixing entro 15 min");
					MsgFile(0, LogCollaudo,"GPS O.K.: Fixing entro 15 min");
					break;
				case 0x08 :
					LoggaStampa("ATTENZIONE: GPS non fixa");
					MSGBOXCALL("GPS NON FIXA",0,2,"Continua","Ferma",bufresponse);
					if (strcmp(bufresponse,"#!")==0) {
						call_exit(YES, "Errore GPS");
					}
					Failed++;
					break;
				default :											// se il GPS è in Avaria ((LST_FIX => '11')
					LoggaStampa("ATTENZIONE: GPS in Avaria");
					MSGBOXCALL("GPS IN AVARIA",0,2,"Continua","Ferma",bufresponse);
					if (strcmp(bufresponse,"#!")==0) {
						call_exit(YES, "Errore GPS");
					}
					Failed++;
					break;
			}
		}
		if ( (TestSet.EnNoAnt) && GPS_Extern() ){
			MSGBOXCALL("Scollegare l'Antenna GPS e premere OK" ,0,1,0,0,bufresponse);
			int tentativi=0;
			while ( ((MTSdata.SMflags & GPS_ANT_) == 0) && (tentativi<30) ) {
				MTSdata.SMflags = M_InVirt();
				sprintf (MyDebB,"0x%02X(%d)", (MTSdata.SMflags & GPS_ANT_),tentativi);
				StampaDB("bit GPS_ANT_",MyDebB);
				tentativi++;
				Delay(1);	
			}
			if ((MTSdata.SMflags & GPS_ANT_) == GPS_ANT_) {				// se l'Antenna GPS è in Avaria ((GPS_ANT_ => '1')
				LoggaStampa("SENSORE Antenna GPS O.K.");
			}else{
				LoggaStampa("ATTENZIONE: Sensore Antenna GPS in Avaria");
				MSGBOXCALL("SENSORE ANTENNA GPS IN AVARIA",0,2,"Continua","Ferma",bufresponse);
				if (strcmp(bufresponse,"#!")==0) {
					call_exit(YES, "Errore ANTENNA GPS");
				}
				Failed++;
			}
		}
	}

	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;

	if (Failed>0) 
		return 1;
	else
		return 0;
}

//	======================================================================
//					Test GSM
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/

int SK_Test_GSM(void)
{
	int	FailedF, Failed = 0;
	int	i, j, Fonia, sGSM ; // Ring,
	char *nStart, *nEnd ;
	char stcsq[2];
	
	float AnLevel, an_rd, Vers;
	unsigned char cnf, inOK_;
	
	if (TestSet.EnGSM == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	// "Verifiche GSM"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	sprintf(Bmom, "(%d_E:%d) TEST GSM", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	LoggaStampa("TEST del GSM");
//	MTSdata.SMflags = M_InVirt();
//	sGSM = (0x01 & M_Diag(DIAG_LU,DIAG_LU9));					// leggo lo stato del GSM --> 0 se è registato!!
	M_Diag(DIAG_LU, DIAG_LU9, dDg); 
	sGSM = (0x01 & atoi(dDg));
#ifdef CBUG
	sprintf(MyDebB,"sGSM = %x", sGSM);
	MsgWindow(MyDebB);
#endif
	Delay(10);
	int attesaregpre;
	int modemregpre;
	modemregpre=0;	
	attesaregpre=0;
	
	while (modemregpre == 0) {						//Controllo che sia registrato
		M_Diag(DIAG_LU, DIAG_LU9, dDg);
		if (atoi(dDg)==0x00) modemregpre=1;
		sprintf(MyDebB,"Attesa registazione modem sGSM = %x", atoi(dDg));
		OuputText(1,MyDebB,0,0,1,7);
		PrintDB(MyDebB);
		attesaregpre++;
		Delay(20);
		if (attesaregpre >= 60) { 					//attesa di 120 secondi max 1200/20=60 
			modemregpre=1; 
		}
	}

	if (strcmp(TestSet.Cpu,"HC12")){ // non è un HC12
		ProgressBar(BAR_TIME, 780 ) ; // 78 s
		M_Diag(DIAG_GSM, 0, dDg); 
		StampaDB("da Diag", dDg);							// Su finestra DOS di Debug 
		if (GetIntStr("$", dDg, 1, MTSdata.Imei)!=0) 		{
			MsgWindow("Errore sul 1° token");
			M_Diag(DIAG_GSM, 0, dDg); 
			StampaDB("da Diag", dDg);							// Su finestra DOS di Debug 
		}
		if (GetIntStr("$", dDg, 2, MTSdata.GsmModel)!=0) 	{
			MsgWindow("Errore sul 2° token");
			M_Diag(DIAG_GSM, 0, dDg); 
			StampaDB("da Diag", dDg);							// Su finestra DOS di Debug 
		}
		if (GetIntStr("$", dDg, 3, MTSdata.GsmRev)!=0) 		{
			MsgWindow("Errore sul 3° token");
			M_Diag(DIAG_GSM, 0, dDg); 
			StampaDB("da Diag", dDg);							// Su finestra DOS di Debug 
		}
	
		if (!strcmp(MTSdata.GsmModel,"MC55")) {
			nStart = MTSdata.GsmRev + 9;
			Vers = atof(nStart);					// valore numerico della Release Firmware del Modem
		}else if (!strcmp(MTSdata.GsmModel,"MC55i")) {
			nStart = MTSdata.GsmRev + 9;
			Vers = atof(nStart) + 4.0 ;				// valore numerico della Release Firmware del Modem
		}else{
			Vers = 0.0 ;							// valore numerico della Release Firmware del Modem
		}
		StampaDBn ("Vers", (int)Vers);			// Su finestra DOS di Debug 
#ifdef CBUG
		sprintf(MyDebB,"Ver = %5.2f", Vers);
		MsgWindow(MyDebB);
#endif // #ifdef CBUG

		sprintf(MyDebB,"%s %s", MTSdata.GsmModel, MTSdata.GsmRev ) ;
		StampaDB("Modem", MyDebB);						// Su finestra DOS di Debug 	
		
		for (i=0;i<10;i++){ // 10*3s
			int attesareg;
			int modemreg;
			modemreg=0;	
			attesareg=0;
			while (modemreg == 0) {						//Controllo che sia registrato
				M_Diag(DIAG_LU, DIAG_LU9, dDg);
				if (atoi(dDg)==0x00) modemreg=1;
				sprintf(MyDebB,"Attesa registazione modem sGSM = %x", atoi(dDg));
				OuputText(1,MyDebB,0,0,1,7);
				PrintDB(MyDebB);
				attesareg++;
				Delay(20);
				if (attesareg >= 60) modemreg=1; //attesa di 120 secondi max 1200/20=60 
			}
			M_Diag(DIAG_LU, DIAG_LU9, dDg); 
			sGSM = (0x01 & atoi(dDg));
#ifdef CBUG
			sprintf(MyDebB,"sGSM = %x", sGSM);
			MsgWindow(MyDebB);
#endif
			Delay(10);
	
			DatoMTS = M_ReadIntAD();
			Delay(10);
			MTSdata.ValoreCSQ = DatoMTS.csq ;
		
#ifdef CBUG
			sprintf(MyDebB,"Virtuale GSM: %x.  CSQ = %d", sGSM, MTSdata.ValoreCSQ ) ;
			MsgWindow(MyDebB);
#endif // #ifdef CBUG
	
			if ( MTSdata.ValoreCSQ > MIN_CSQ ){	// OK
				break ;
			}
			Delay(10) ;
		}
	}else{		// HC12
		Vers = 0.0 ;							// valore numerico della Release Firmware del Modem
		ProgressBar(BAR_TIME, 550 ) ; 
	
		MTSdata.GsmModel[0]='\0';
		MTSdata.ValoreCSQ=0;
		for (i=0;i<10;i++){
			// Vede se il GSM è registrato
			MTSdata.SMflags = M_InVirt();
			if (sGSM!=0){
				if (MTSdata.SMflags & GSM_REG) 
					sGSM = 0 ;
				else
					sGSM = 1 ;
			}
			// Deve alzare il virtuale 0 per avere i dati del GSM
			M_SetStatus(1,1) ;
			Delay(10);
			// Get answer
			M_GetDirect(dDg) ;
			// Cerca modello modem
			nStart = strstr(dDg, "Tel=") ;
			if (nStart){
				nStart += 4 ;
				// divide modello da revision
				nEnd = strstr(nStart, "Gps=") ;
				if (nEnd) nEnd[0]='\0' ;
				strcpy(MTSdata.GsmModel, nStart) ;
				StampaDB("GSM ", nStart ) ;
			}
			nStart = strstr(dDg, "Ctel=") ;			
			if (nStart){
				nStart += 5 ;
				sprintf(stcsq,"%d",atoi(nStart));
				StampaDB("CSQ ", stcsq ) ;
				MTSdata.ValoreCSQ = atoi(nStart) ;
			}
			Delay(10);
			//if ((MTSdata.GsmModel!='\0')&&( MTSdata.ValoreCSQ > MIN_CSQ )){	// OK
			if ((MTSdata.GsmModel!=NULL)&&( MTSdata.ValoreCSQ > MIN_CSQ )){	// OK
				break ;
			}
			Delay(10);
		}
		
		// NOW GET IMEI
		M_GetIMEI(MTSdata.Imei) ;
		// attendi 24 secondi affinchè l'MTS ritorni in modalità normale
		Delay(400);
	}

	if (sGSM == 0) {
		if (MTSdata.ValoreCSQ >= MIN_CSQ) {								// MIN_CSQ = Soglia di CSQ di accettazione 
			MsgWindow(" ");
			sprintf(MyDebB,"--> MODEM O.K. - %s-%s-IMEI: %s - Registrato con CSQ di %d (min. %d)", 
												MTSdata.GsmModel, MTSdata.GsmRev, MTSdata.Imei, MTSdata.ValoreCSQ, MIN_CSQ);
			LoggaStampa(MyDebB);
		}else{
			sprintf(MyDebB,"ATTENZIONE: GSM (%s-%s-IMEI:%s) Registrato con CSQ basso (%d)", 
													MTSdata.GsmModel, MTSdata.GsmRev, MTSdata.Imei, MTSdata.ValoreCSQ);
			LoggaStampa(MyDebB);
			Failed++;
		}
	}else{
		sprintf(MyDebB,"ATTENZIONE: GSM (%s-%s-IMEI:%s) NON Registrato", MTSdata.GsmModel, MTSdata.GsmRev, MTSdata.Imei);
		LoggaStampa(MyDebB);
		Failed++;
	}

//	======================================================================
//					Dual-SIM
//	======================================================================
#define WAITTEST 30
	unsigned int statosim,modsim,numsim,modemoff,modemreg,numsimnew;
	if (!strcmp(TestSet.Cpu,"CORTEX")){  
		int k;
		k=M_GetSwVers();							// si legge la Versione Software
		if ( ( k == 2100) && !(strncmp(TkIni.mName, "2202", 4)) ) {
			RevHW = M_GetHwVers();
			Delay(10);
			MTSdata.HMainVer = RevHW.HwMain;
			MTSdata.HSrvVer = RevHW.HwSrv;
			if ( MTSdata.HMainVer  > 4 ){          // Dual-Sim CodHwMain = 5
				M_Diag(2,7,dDg); 
			}else{
				sprintf(dDg,"0");
			}
		}else{
			M_Diag(2,7,dDg); 							// verifica se dual sim
			sprintf(bufwindow,"Diag dual-sim:<%s>",dDg);
			PrintDB(bufwindow);
		}
		if ( (atoi(dDg)/65536) == 7 ) { 						//allora dual-sim
			int okspostata=0;
			int pretry=0;
			while (okspostata == 0) {
				ProgressBar(BAR_TIME, 780 ) ; 				// 78 s
				statosim=atoi(dDg)%65536;						
				modsim=statosim & 0xF0;
				numsim=statosim & 0x0F;						//Mi segno quale sim uso
				sprintf(bufwindow,"Modalità Dual Sim:<0x%x>\rSim in uso:<%d>",modsim,numsim);
				LoggaStampa(bufwindow);
				PrintDB(bufwindow);
				M_Action(32,4,0);							//Spengo il modem
				Delay(20);
				modemoff=0;
				int try=0;
				while (modemoff == 0) {
					M_Diag(DIAG_LU, DIAG_LU9, dDg);
					if (atoi(dDg)==0xFF) modemoff=1;		//Controllo che sia spento
					if ( (!strcmp(MTSdata.GsmModel,"LISA-U200")) && (atoi(dDg)==0x01) ) modemoff=1;
					sprintf(MyDebB,"Attesa spegnimento modem diag = %x", atoi(dDg));
					OuputText(1,MyDebB,0,0,1,7);			
					PrintDB(MyDebB);
					if (try>WAITTEST){
						MSGBOXCALL("PROBLEMI DEREGISTRAZIONE MODEM" ,0,1,0,0,bufresponse);
						call_exit(YES, "DEREGISTRAZIONE MODEM");
					}
					try++;
					Delay(20);
				}
				MSGBOXCALL("Spostare la sim nell'altra posizione!",0,1,0,0,bufresponse); //Avviso di Spostare la Sim
				Delay(100);
				if (!strcmp(MTSdata.GsmModel,"LISA-U200")) {
					if (numsim==0) 
						M_Action(31,10,0); //SIM2 SIM_UP
					else 
						M_Action(32,10,0); //SIM1 SIM_DOWN
				}
				Delay(100);
				M_Diag(2,7,dDg);							//Controllo che sim selezionata
				Delay(20);
				statosim=atoi(dDg)%65536;
				numsimnew=statosim & 0x0F;
				sprintf(bufwindow,"Sim selezionata:<%d>",numsimnew);
				LoggaStampa(bufwindow);
				Delay(20);
				M_Action(31,4,0);						    //Accendo il modem
				int attesareg;
				modemreg=0;	
				attesareg=0;
				Delay(100);
				while (modemreg == 0) {						//Controllo che sia registrato
					M_Diag(DIAG_LU, DIAG_LU9, dDg);
					if (atoi(dDg)==0x00) modemreg=1;
					sprintf(MyDebB,"Attesa registazione modem sGSM = %x", atoi(dDg));
					OuputText(1,MyDebB,0,0,1,7);
					PrintDB(MyDebB);
					attesareg++;
					Delay(20);
					if (attesareg >= 60) { 					//attesa di 120 secondi max 1200/20=60 
						modemreg=1; 
						numsim=255;							//imposto numsim a 0xff
					}
				}
				M_Diag(2,7,dDg);							//Controllo che sim diversa da prima
				statosim=atoi(dDg)%65536;
				modsim=statosim & 0xF0;
				numsimnew=statosim & 0x0F;
				sprintf(bufwindow,"Modalità Dual Sim:<0x%x>\rSim in uso:<%d>",modsim,numsimnew);
				if (numsim==numsimnew) {
					MSGBOXCALL("Sim non Spostata!\rAttendere reset procedura!",0,1,0,0,bufresponse); //Non Avvenuto	
					Delay(30);
				}else{
					okspostata=1;
					Delay(40);
				}
				if (pretry>WAITTEST){
					MSGBOXCALL("PROBLEMI DUAL SIM" ,0,1,0,0,bufresponse);
					call_exit(YES, "PROBLEMI DUAL SIM");
				}
				pretry++;
			}
			for (i=0;i<10;i++){ // 10*3s
				M_Diag(DIAG_LU, DIAG_LU9, dDg); 
				sGSM = (0x01 & atoi(dDg));
	#ifdef CBUG
				sprintf(MyDebB,"sGSM = %x", sGSM);
				MsgWindow(MyDebB);
	#endif
				Delay(20);
	
				DatoMTS = M_ReadIntAD();
				Delay(20);
				MTSdata.ValoreCSQ = DatoMTS.csq ;
		
	#ifdef CBUG
				sprintf(MyDebB,"Virtuale GSM: %x.  CSQ SIM2 = %d", sGSM, MTSdata.ValoreCSQ ) ;
				MsgWindow(MyDebB);
	#endif // #ifdef CBUG
	
				if ( MTSdata.ValoreCSQ > MIN_CSQ ){	// OK
					break ;
				}
				Delay(20) ;
			}
			if (sGSM == 0) {
				if (MTSdata.ValoreCSQ >= MIN_CSQ) {								// MIN_CSQ = Soglia di CSQ di accettazione 
					MsgWindow(" ");
					sprintf(MyDebB,"--> MODEM O.K. - %s-%s-IMEI: %s - Registrato su SIM2 con CSQ di %d (min. %d)", 
														MTSdata.GsmModel, MTSdata.GsmRev, MTSdata.Imei, MTSdata.ValoreCSQ, MIN_CSQ);
					LoggaStampa(MyDebB);
				}else{
					sprintf(MyDebB,"ATTENZIONE: GSM (%s-%s-IMEI:%s) Registrato su SIM2 con CSQ basso (%d)", 
															MTSdata.GsmModel, MTSdata.GsmRev, MTSdata.Imei, MTSdata.ValoreCSQ);
					LoggaStampa(MyDebB);
					Failed++;
				}
			}else{
				sprintf(MyDebB,"ATTENZIONE: GSM (%s-%s-IMEI:%s) NON Registrato su SIM2", MTSdata.GsmModel, MTSdata.GsmRev, MTSdata.Imei);
				LoggaStampa(MyDebB);
				Failed++;
			}
		}
	}
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
//	======================================================================
//					Test Fonia
//	======================================================================

	
	

	if ( (TestSet.EnFonia == YES) && (Failed == 0) ) {	
	
		FailedF = 0 ;

		// "Collaudo fonia"
		COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
		sprintf(MyDebB,"--> MODELLO:%s", MTSdata.GsmModel);
		LoggaStampa (MyDebB);
	
		if ( (Vers>4.0) || (!strcmp(MTSdata.GsmModel,"LISA-U200")) ) {																// solo per le versioni + recenti (>4.00) di MC55 esiste la funzione di Loop-Back
			ProgressBar(BAR_TIME, 190 ) ; // 19 s
		}else{
			ProgressBar(BAR_PERC, 0 ) ;
		}
	
		sprintf(Bmom, "(%d_E:%d) TEST FONIA", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		int tenta=0;
		if ( (Vers>4.0) || (!strcmp(MTSdata.GsmModel,"LISA-U200")) ) {																// solo per le versioni + recenti (>4.00) di MC55 esiste la funzione di Loop-Back
			while (tenta<2) {
				FailedF = 0;
				LoggaStampa("Test Fonia con Loop-Back interno");
				i = 3 ;
				AnLevel = 0 ; 
				while (i>0) {									// si misura il livello Analog. in OFF della Fonia facendo ... 
					AnLevel = AnLevel + T_Analog(FONIA);		// ... la media matematica di 3 letture eseguite in 3 secondi 
					Delay(9);
					i--;
				}
				AnLevel = AnLevel / 3;										// AnLevel = valore del livello Analog. in OFF della Fonia
		#ifdef CBUG
				sprintf(MyDebB,"Fonia spenta: Vcresta = %5.0f", AnLevel);
				LoggaStampa(MyDebB);
		#endif // #ifdef CBUG
				if (!strcmp(MTSdata.GsmModel,"LISA-U200")) {
					M_Actionwithresp(255,6,"AT+UI2CO=1,0,0,0x10,0\r",bufresponse);									//Apro la Porta I2c per parlare con MAX9860
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					M_Actionwithresp(255,6,"AT+UI2CW=\"00000000109E20240000000060600000008A\",18\r",bufresponse); 	//Setto al Max MIC e SPEAKER del MODEM
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					M_Actionwithresp(255,6,"AT+UI2CC\r",bufresponse);												//Chiudo la Porta I2c per parlare con MAX9860
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					M_Actionwithresp(255,6,"AT+UMGC=5,,32767\r",bufresponse);										//Setto il MIC DIG al MAX
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					M_Actionwithresp(255,6,"AT+USGC=5,,,32767,32767\r",bufresponse);									//Setto il SPEAKER DIG al MAX
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					M_Actionwithresp(255,6,"AT+UPAR=2,0,0\r",bufresponse);											//Attivo Audio LOOP
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					M_Actionwithresp(255,6,"AT+UGPIOC=21,0,1\r",bufresponse);										//Accendo Ampli MTS
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					M_Actionwithresp(255,6,"AT+UGPIOC=21,0,1\r",bufresponse);										//Accendo Ampli MTS
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					M_Actionwithresp(255,6,"AT+UGPIOC=21,0,1\r",bufresponse);										//Accendo Ampli MTS
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
							FailedF++;
					}
					sprintf(dDg,"0");
					Delay (200);	
				}else{
					M_Diag(DIAG_AUDIO, DIAG_ENABLE, dDg); 
				}
				if (~(atoi(dDg))) {												// Abilita Audio Loop: OK se ritorna 0
		//		if (~(M_Diag(DIAG_AUDIO,DIAG_ENABLE))) {						// Abilita Audio Loop: OK se ritorna 0
					Delay (70);													// attesa di 7 sec. (0608 erano 4)
					i = 3; an_rd = 0.0 ; inOK_ = 0;
					while (i>0) {												// si misura il livello Analog. corrente della Fonia facendo ... 
						an_rd = an_rd + T_Analog(FONIA);			// ... la media matematica di 3 letture eseguite in 3 secondi
						if ((UNK_OK_ & T_Input()) == 0) inOK_++;	// e per ogni volta si controlla che la fonia sia giusta in frequenza (1KHz)
						Delay(18);															// attesa di 1.8 sec. (0608 erano 0.9)
						i--;
					}
					if ( inOK_ < 3 ) inOK_ = 3;									// non si controlla la frequenza (si impone il riconoscimento)!!!!!!!!1 (0608)
					an_rd = an_rd / 3.0 ;						// an_rd = valore del livello Analog. corrente della Fonia
																// inOK_ indica quante volte 1KHz è stato riconosciuto (matching freq)
					sprintf(MyDebB,"GSM: Vcresta Fonia %5.1f  - 1KHz visto %d volte", an_rd, inOK_);
					MsgWindow(MyDebB);
					cnf = an_rd - AnLevel;
					if (inOK_ >= 3) {
						if (cnf > TestSet.SogliaF) {
							sprintf(MyDebB,"--> GSM: Connessione Fonia O.K. (livello = %d)", cnf);
							LoggaStampa(MyDebB);
							tenta=250;
						}else{
							if (tenta>0){
								sprintf(MyDebB,"GSM: ATTENZIONE Fonia debole in ampiezza (livello = %d)", cnf);
								LoggaStampa(MyDebB);
								FailedF++;
							}
						}
					}else{
						if (tenta>0){
							if (inOK_ == 0) {
								sprintf(MyDebB,"GSM: ATTENZIONE Fonia in Avaria (livello = %d)", cnf);
								LoggaStampa(MyDebB);
								MSGBOXCALL("ATTENZIONE FONIA IN AVARIA" ,0,2,"Continua","Ferma",bufresponse);
								if (strcmp(bufresponse,"#!")==0) {
									call_exit(YES, "Errore FONIA");
								}
								FailedF++;
							}else{
								sprintf(MyDebB,"GSM: ATTENZIONE Fonia fuori Banda (livello = %d)", cnf);
								LoggaStampa(MyDebB);
								FailedF++;
							}
						}
					}
				}
				if (!strcmp(MTSdata.GsmModel,"LISA-U200")) {
					M_Actionwithresp(255,6,"AT+UGPIOC=21,0,0\r",bufresponse);								//Spengo Ampli MTS
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
					}
					M_Actionwithresp(255,6,"AT+UEXTDCONF=0,1\r",bufresponse);								//Risetto default MAX9860
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
					}
					M_Actionwithresp(255,6,"AT+UMGC=5,,8192\r",bufresponse);									//Setto il MIC DIG al default
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
					}
					M_Actionwithresp(255,6,"AT+USGC=5,,,8192,16384\r",bufresponse);							//Setto il SPEAKER DIG al default
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
					}
					M_Actionwithresp(255,6,"AT+USAR=2\r",bufresponse);										//Disativo Audio LOOP
					if (strcmp(bufresponse,"OK")!=0) {
							MSGBOXCALL("PROBLEMI COMANDI AT" ,0,1,0,0,bufresponse);
							//call_exit(YES, "PROBLEMI COMANDI AT");
					}
				}else{
					M_Diag(DIAG_AUDIO, DIAG_DISABLE, dDg); 								// Disabilita Audio Loop
				}
			tenta++;
			}
		}else{																		// se il modem non prevede il Loop-Back
			LoggaStampa("Test Fonia senza Loop-Back");
			MSGBOXCALL("Connettere all'MTS il cablaggio con Microfono  ed Altoparlante!", "Test Fonia (NO loop-back)",2, "Continua", "Salta", bufresponse);
		
			if (strcmp(bufresponse,"##")==0) {									// fare il controllo con l'operatore
				//M_SetPar(81, "2"); 													// Abilita AUTO-ANSWARE
				MSGBOXCALL("Effettuare una chiamata   vocale e controllarne la qualità", "Test Fonia (NO loop-back)",
																											1, 0, 0, bufresponse);
	/*			Ring = (RING_F & M_InVirt());
				j = 0;
				while ((Ring == 0) && (j < 20)){							// attendo il RING per massimo 20 sec.
					Ring = (RING_F & M_InVirt());							// il Ring è arrivato ?
					Delay(10);
					j++;
				}
				if (Ring == 0) {														// Chiamata non pervenuta in 20 sec.
					sprintf (MyDebB,"ATTENZIONE: GSM: Chiamata non pervenuta (No Ring)");
					LoggaStampa (MyDebB);
					FailedF++;
				}else{																			// Se invece è arrivata ...
	*/
	//				OuputText(6,"OK",4,"Ring",1,1);
				
					ProgressBar(BAR_TIME, 300 ) ; // 30 s
				
					Fonia =(FONIA_N & M_InVirt());
					j = 0;
					while ((Fonia == 0) && (j < 30)){					// attendo il Fonia-in-corso per massimo 30 sec.
						Fonia =(FONIA_N & M_InVirt());
						Delay(10);
						j++;
					}
					if (Fonia == 0) {													// Fonia non attivata entro 10 sec.
						sprintf(buflog,"GSM: ATTENZIONE Test Fonia non Attivata in 30s");
						sprintf(MyDebB,"%s\r", buflog);
						MsgWindow(MyDebB);
						MsgFile(0, LogCollaudo, buflog);
						FailedF++;
					}else{
						OuputText(9,"In corso",4,"Fonia",1,2);	// Segnala la connessione presente
						sprintf(MyDebB,"--> GSM: Connessione Fonia O.K. (con operatore)");
						LoggaStampa(MyDebB);
						Delay(30);
						MSGBOXCALL("Il test di Fonia è O.K.   oppure è Fallito ", "Test Fonia (NO loop-back)",2, "O.K.", "Fallito", bufresponse);
						if (strcmp(bufresponse,"##")==0) {
							sprintf(MyDebB,"--> GSM: Qualità Fonia O.K. (con operatore)");
						}else{
							sprintf(MyDebB,"GSM: ATTENZIONE Test Fonia con operatore FALLITO");
							FailedF++;
						}
						LoggaStampa(MyDebB);
					}
					OuputText(9,"",0,"",0,0);
	//			}
			}else{
				sprintf(MyDebB,"--> GSM: Test Fonia NON eseguito (con operatore)");
				//FailedF++;
				LoggaStampa(MyDebB);
			}
		}
	}
	if ( (!strcmp(MTSdata.GsmModel,"LISA-U200")) && (numsimnew==1) && (Failed==0) && (FailedF==0) ) {
			M_Action(32,4,0);							//Spengo il modem
			modemoff=0;
			int try=0;
			while (modemoff == 0) {
				M_Diag(DIAG_LU, DIAG_LU9, dDg);
				if (atoi(dDg)==0xFF) modemoff=1;		//Controllo che sia spento
				if ( (!strcmp(MTSdata.GsmModel,"LISA-U200")) && (atoi(dDg)==0x01) ) modemoff=1;
				sprintf(MyDebB,"Attesa spegnimento modem diag = %x", atoi(dDg));
				OuputText(1,MyDebB,0,0,1,7);			
				PrintDB(MyDebB);
				if (try>WAITTEST){
					MSGBOXCALL("PROBLEMI DEREGISTRAZIONE MODEM" ,0,1,0,0,bufresponse);
					call_exit(YES, "DEREGISTRAZIONE MODEM");
				}
				try++;
				Delay(10);
			}
			M_Action(32,10,0); //SIM1 SIM_DOWN
	}
	if (TestSet.EnFonia == NO){ // esci senza far nulla se il test non è richiesto  	
		if (Failed>0) 
			return 1;
		else
			return 0;		
	}
	
	COLOR_STEP(MtsTK.steptest++, ( (FailedF)? C_RED: C_GREEN)) ;

	if ((Failed+FailedF)>0) 
		return 1;
	else
		return 0;
}

//	======================================================================
//					Test CAN	
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_CAN(void)
{
	int Failed = 0;
	int FailedS, FailedF, FailCAN[3],cansel;
	int i, k;
	struct timeb loc_time ;
	int test_CAN = -1;				
	long starttime = 0;
	long starttime2 = 0;
	long timeout= 0;
	/*
	if (TestSet.EnCANConf == 1) {
			GetINIKeyVal("CanConf", TkIni.NamCANConf);
			togliCR(TkIni.NamCANConf);
			sprintf(CANConfSel, "%s\\%s", mRoot,TkIni.NamCANConf);
			CONVERTPATH(CANConfSel);
			sprintf(buflog, "Configurazione CAN:%s",CANConfSel);
			LoggaStampa(buflog);	
	}*/
	if (TestSet.QtaCAN != 0) {

		// "CAN"
		COLOR_STEP(MtsTK.steptest, C_YELLOW) ;

		if (TestSet.QtaCAN>2) TestSet.QtaCAN=2;	 // non si possono gestire + di 2 porte CAN

		i = TestSet.QtaCAN * 500  ;
		ProgressBar(BAR_TIME, i ) ;
	
		#ifdef DEBUG_FR
		sprintf(bufmsg, "Stop per start CAN");
		MSGBOXCALL(bufmsg,0,2,"Continua","Ferma",bufresponse); 
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Stop CAN");
		}
		#endif // DEBUG_FR		

		//	NumCAN = atoi(QtaCAN); 	// imposta la quantità degli ingressi da testare
		FailCAN[1]=0; 
		FailCAN[2]=0;
		sprintf(Bmom, "(%d_E:%d) TEST CANBUS", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgWindow(" ");
		for(i=1; i<=TestSet.QtaCAN; i++) {
			//cansel=i+1;
			//if(cansel>TestSet.QtaCAN) cansel=i-1;
			cansel=i;
			FailedS = 0 ; 
			FailedF = 0 ;
			sprintf(buflog, "TEST della porta CAN%d di %d", cansel, TestSet.QtaCAN);
			LoggaStampa(buflog);
			int CAN_LOW=0;
			if (CAN_LOW){
				//RESET TKCAN
				T_SetCanBaudrate(cansel-1, 0);					// imposta CAN di T-K: baudrate = 0 kbit/sec
				Delay(80);//80
				M_CanClear( ((MTSdata.mSign==40)? cansel+1: cansel-1), 50001); // Vecchio o nuovo tipo di CAN
				M_CanAdd(cansel-1, 20, 0x1c030, 0x800fffff);
				M_checkcanstart(cansel-1);
				
				//MSGBOXCALL("Controllare Configurazione Can LOW", 0, 1, 0, 0, bufresponse);
				Delay(10);  //20
				//extension commands
				T_SetCanBaudrate(cansel-1, 50000);					// imposta CAN di T-K: baudrate = 50000 kbit/sec
				T_SetCanMailbox(cansel-1, 1, 0xFFFFFFFF, 0x1c030, 'E', 0);
				Delay(15);  //30
				M_CanClearBuffer(cansel-1);
				MsgWindow("In corso invio di dati all'MTS (Low Baudrate) ...");
				for (k=0; k<8; k++) { // 8
					Delay(5); //5
					T_EmitCanFrame(cansel-1, 1, "aa5566");		// " Frame aa5566
				}
				#ifdef DEBUG_FR
				sprintf(bufmsg, "Stop per CAN%d",cansel);
				MSGBOXCALL(bufmsg,0,2,"Continua","Ferma",bufresponse); 
				if (strcmp(bufrespone,"#!")==0) {
					call_exit(YES, "Stop CAN");
				}
				#endif // DEBUG_FR		
				k = M_CanCheck(cansel-1, 0x1c030, "aa5566");
				if (k != 0) FailedS++;								// la prima risposta deve essere a '0'
				Delay(10); //20
				k = M_CanCheck(cansel-1, 0x1c030, "aa5567");
				if (k != 1) FailedS++;								// la seconda risposta deve essere a '1'
				if (FailedS == 0) {										// si valuta il test a bassa velocità (Slow)
					LoggaStampa("--> Low Baud-Rate O.K.");
				}else{
					sprintf(buflog, "ATTENZIONE: CAN%d (Low Baudrate) NON riceve",cansel);
					LoggaStampa(buflog);
				}
			}
				
			if ((strncmp(TkIni.mName, "MTS", 3)) || (cansel<2)){
				if (CAN_LOW) Delay(40);//80
				//RESET TKCAN
				T_SetCanBaudrate(cansel-1, 0);					// imposta CAN di T-K: baudrate = 0 kbit/sec
				//Delay(80);//80
				localmillisDelay(1000); //wait 1sec
				if(cansel>1){
					M_CanClear(((MTSdata.mSign==40)? cansel+1: cansel-1), 100001);		// Cancella Conf precedente e predispopne baudrate = 100000 kbit/sec
				}else{
					M_CanClear(((MTSdata.mSign==40)? cansel+1: cansel-1), 250001);		// Cancella Conf precedente e predispopne baudrate = 250000 kbit/sec
				}
				M_CanAdd(cansel-1, 20, 0x00F00400, 0x8fffffff);		// imposta canale 'cansel' di monitoraggio
				M_checkcanstart(cansel-1);											// inizia a monitorare
				//Delay(40);//80
				localmillisDelay(500); //wait 0.5sec
				//MSGBOXCALL("Controllare Configurazione Can HIGH", 0, 1, 0, 0, bufresponse);
				if(cansel>1){
					T_SetCanBaudrate(cansel-1, 100000);				// imposta CAN di T-K: baudrate = 100000 kbit/sec
				}else{
					T_SetCanBaudrate(cansel-1, 250000);				// imposta CAN di T-K: baudrate = 250000 kbit/sec
				}
				T_SetCanMailbox(cansel-1, 1, 0xFFFFFFFF, 0x00F00400, 'E', 0); // " : in TX, Indirizzo 1, Esteso
				M_CanClearBuffer(cansel-1);
				MsgWindow("In corso invio di dati all'MTS (High Baudrate)[Bazooka] ...");
				test_CAN=-1;				
				ftime(&loc_time) ;
				starttime=(loc_time.time*1000+loc_time.millitm);
				starttime2=starttime;
				timeout=starttime;

				while (((loc_time.time*1000+loc_time.millitm)-timeout)<60000){ //per un minuto
				//for (k=0;k<150;k++){
					ftime(&loc_time) ;
					if(((loc_time.time*1000+loc_time.millitm)-starttime)>1000){
						sprintf(MyDebB,"time[%ld]starttime{%ld}TestKit Invio Trasmissione to CAN%d from address 0x00F00400 send data: AA BB DD EE FF 55 AA 55\r", (loc_time.time*1000+loc_time.millitm),starttime,cansel);
						MsgWindow(MyDebB);
						T_EmitCanFrame(cansel-1, 1, "AABBDDEEFF55AA55");
						starttime=(loc_time.time*1000+loc_time.millitm);
					}
				  if(((loc_time.time*1000+loc_time.millitm)-starttime2)>5000){
						sprintf(MyDebB,"time[%ld]starttime2{%ld}Check CAN%d Ricezione MTS\r", (loc_time.time*1000+loc_time.millitm),starttime2,cansel);
						test_CAN=M_CanCheck(cansel-1, 0x00F00400, "AABBDDEEFF55AA55");
						starttime2=(loc_time.time*1000+loc_time.millitm);
						if (test_CAN==0) break ;         // CAN OK
					}
				}

				if (test_CAN != 0) {																		// la risposta deve essere a '0'
					sprintf(buflog, "ATTENZIONE: CAN%d (High Baudrate) NON riceve",cansel);
					LoggaStampa(buflog);
					FailedF++;
				}
			}
			
			//MSGBOXCALL("Controllare Configurazione Can HIGH", 0, 1, 0, 0, bufresponse);
			// Cancella configurazione CAN
			M_CanClear(((MTSdata.mSign==40)? cansel+1: cansel-1), 250000);	// Cancella Conf precedente
			M_checkcanstart(cansel-1);									// (Per 4004 necessario per eseguire la cancellazione della conf.)
		
			FailCAN[cansel] = (FailedS) || (FailedF);
			if (FailCAN[cansel]) {
				sprintf(bufmsg, "Attenzione: CAN%d non riceve",cansel);
				MSGBOXCALL(bufmsg,0,2,"Continua","Ferma",bufresponse); 
				if (strcmp(bufresponse,"#!")==0) {
					call_exit(YES, "Errore CAN");
				}
			}else {
				sprintf(buflog, "--> CAN%d port O.K.",cansel);
				LoggaStampa(buflog);
			}
		}
		Failed= FailCAN[1] + FailCAN[2];
	}
	if ( ( (TestSet.EnCANConf == 1) || (TestSet.QtaCAN != 0) ) && Failed == 0) {
		sprintf(buflog, "Invio Configurazione CAN");
		LoggaStampa(buflog);
		if (SK_CanConfSet()) Failed++;
		sprintf(buflog, "Invio Configurazione CAN %s",(Failed)? "Fallito": "Riuscito");
		LoggaStampa(buflog);
	}
	// TEST Resistenza Terminale da 120 Ohm
	// TEST DISABILITATO MODIFICATO CAVO
	/*
	if ( (TestSet.QtaCAN != 0) && ( (!strcmp(TkIni.mName, "3035")) || (!strcmp(TkIni.mName, "3036")) ) )
	{
		//Disabilito uscita 5 TestKit
		T_Output(TOD5,0);
		// Setto i can in off comando 
		M_Action(32,14,0); 
		// Leggo riferimento analogico out 7 del TestKit su in AN0 del TestKit
		sprintf(Bmom, "(%d_E:%d) TEST RESISTENZA TERMINALE 120ohm", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgWindow(" ");
		unsigned int chref=0;
		if (TKTYPE==0) chref=~(chref);
		T_Output(CHN, chref);		// posiziona il "puntatore di canale" sul 8ª/7ª uscita analogica T-K
									// che è connessa al 2°/1° input di Board-T-K (IREF/VREF)
		if (TKTYPE==0){								
			T_Output(SETAN, TO_VOLT);		// abilita le uscite analogiche del T-K in Corrente/Tensione (e tutte le uscite dig. del T-K come "flottanti")

			T_Output(AN_DIG_, AN_DIG_);		// Abilita le uscite analogiche
		}else{
			T_Output(EN_CNT, 0);				  // Disattivo il timer di T-K di 1 KHz
			T_Output(NEWALLTOVFG , NEWALLTOVFG ); // uscite TK Flottanti
			T_Output(NEWFLOAT_ , 0);		      // uscite TK Flottanti
			T_Output(SETAN, TO_VOLT);		// abilita le uscite analogiche del T-K in Corrente/Tensione (e tutte le uscite dig. del T-K come "flottanti")
		}
		Delay(25); // Attesa di 2.5 sec perchè si stabilizzi l'uscita An
		float TkRefer= 0;
		int tmpc = T_Analog(0);					// valore ADC della misura di tensione su 150 Ohm (del T-K)
		TkRefer += tmpc;
		sprintf(MyDebB, "Lettura %d REF: %d", 1, tmpc);
		StampaDB ("", MyDebB);
		float Cur_R = ((TkRefer)*3000.0/1023.0)/150.0 ;		// Valore in mA della corrente imposta
		sprintf (MyDebB,"Reference Corrente (media): %5.2f mA", Cur_R);
		// Muovo il rele su NO 
		T_Output(TRL2_, 0); 					//	Attivare rele TK! 
		Delay(5);
		// setto in canale analogico out  4 del TestKit
		unsigned int ch=4;
		if (TKTYPE==0) ch=~(ch);						// per la struttura HW del T-Kold il canale Analog. 'n' è indirizzato con ~('n')
		T_Output(CHN, ch);					    
		SetProtocolMode(KPROTOCOLOMODE_QUERYANSWER);	// Imposta la modalità di analogici aggiornati
		Delay(10);										// si rinfrescano i valori analogici entro 3 sec (0610 era 25)
		SetProtocolMode(KPROTOCOLOMODE_BUFFERED);
		// Attivo uscita 5 TestKit
		T_Output(TOD5,TOD5);
		Delay(10);
		// Leggo analogico out 4 del TestKit su in AN2 del TestKit
		float an_rd= 0;
		tmpc = T_Analog(2);					// valore ADC della misura di tensione su 120 Ohm (del T-K)
		an_rd += tmpc;
		sprintf(MyDebB, "Lettura TK-AN2: %d", tmpc);
		StampaDB ("", MyDebB);
		float Cur_R1 = ((an_rd)*3000.0/1023.0)/120.0 ;		// Valore in mA della corrente imposta
		sprintf (MyDebB,"Corrente su 120ohm: %5.2f mA", Cur_R1);
		MsgWindow (MyDebB);	
		float diffcur_R = fabs(Cur_R - Cur_R1);
		float diffcur_R_per = (diffcur_R/16.0)*100.0;
		sprintf(MyDebB,"Differenza [Diff %5.2f mA (%1.2f %%)]", diffcur_R, diffcur_R_per);		
		// Confronto AN0/150ohm vs AN2/120ohm se tolleranza +-5% OK
	
		//Risetto uscita su AN0 di riferimento
		ch=0;
		if (TKTYPE==0) ch=~(ch);						// per la struttura HW del T-Kold il canale Analog. 'n' è indirizzato con ~('n')
		T_Output(CHN, ch);
		//Disabilito uscita 5 TestKit
		T_Output(TOD5,0);
		//Disabilita Rele
		T_Output(TRL2_,TRL2_); //Disattiva rele TK!

		if (diffcur_R_per > TestSet.CAN_Term_Perc) {
			sprintf(MyDebB,"Avaria corrente su 120ohm errore maggiore del %d %% letto con Valore: %5.2f mA (%5.2f mA)\r",TestSet.CAN_Term_Perc,Cur_R1, Cur_R);
			LoggaStampa(MyDebB);
			MSGBOXCALL (MyDebB,0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore CAN TERM");
			}
			sprintf(MyDebB,"CAN TERM 120ohm KO");	//	.. segnala anonalia
			Failed++;
		}else{
			sprintf(MyDebB,"Corrente su 120ohm OK letto con Valore: %5.2f mA (%5.2f mA)\r",Cur_R1, Cur_R);
			LoggaStampa(MyDebB);
			sprintf(MyDebB,"CAN TERM 120ohm OK");
		}
		LoggaStampa(MyDebB);
		// Setto i can in on comando
		Delay(20);
		M_Action(33,14,0); 
	}*/
	//RESET TKCAN
	T_SetCanBaudrate(cansel-1, 0);					// imposta CAN di T-K: baudrate = 0 kbit/sec
	//Delay(80);//80
	localmillisDelay(1000); //wait 1sec
	if ((TestSet.EnCANConf == 1) || (TestSet.QtaCAN != 0)) COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
	return Failed;
}

unsigned int M_checkcanstart(unsigned int ch){
	unsigned int k;
	for (k=0;k<15;k++){	
		M_CanStart(ch,bufresponse);
		if (strcmp(bufresponse,"OKCANCONF")==0) break;  // Attesa per settaggio CAN
		Delay(10);		   
	}
	return(0) ;
}

//	======================================================================
//					TEST HTL	
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_HTL(void)
{
	
#define HTL_LOOP 5
	
	int i,x, Failed = 0;
	
	if (TestSet.EnHTL == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	sprintf(Bmom, "(%d_E:%d) TEST HTL", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	LoggaStampa("TEST porta HTL");
	ProgressBar(BAR_TIME, 200 ) ;
	
	T_Output(NEWHTLORTTL | TRL2_ , NEWHTLORTTL | TRL2_ );   // Seriale in HTTL e Rele su NC sul TK
	T_SetComPort(3, 10400,'N', 8, 1, '0') ; // 9600,N,8,1 RSHTL
	char Bmomhexcheck[MAXSIZE];
	char bufresponsehex[MAXSIZE];
	char bufmommom[MAXSIZE];
	int nCheck;
	sprintf(Bmomhexcheck,"45573434384347"); //454454434f Prima	
	//TEST SERIALE + TEST CTS E RTS
	for (i=0;i<HTL_LOOP;i++){
		///FACCIO INVIARE AL TK
		for (x=0;x<HTL_LOOP;x++){			
			T_ComSendHex(3,"554454434f00801d0505711e7d7f0010c0");
			Delay(5);
			T_ComSendHex(3,"c00000be57500164575001b414ffff5001");
			Delay(5);
			T_ComSendHex(3,"11595632585a5830413645423639363339");
			Delay(5);
			T_ComSendHex(3,"390e014557343438434720202020202000009f");
			Delay(5);
		}
		/// Controllo che ha ricevuto MTS
		bufresponsehex[0]='\0';
		bufresponse[0]='\0';
		Delay(40);
		M_Diag(12,0,bufresponsehex);
		hex_to_text(bufresponsehex,bufresponse);
		nCheck = (strstr( bufresponsehex , Bmomhexcheck )!=NULL)?1:0;
		sprintf(bufmommom,"\r %d Ricevuto da MTS:{%s Hex:[%s] Check:[%s] strstr:[%d]}\r",i,bufresponse,bufresponsehex,Bmomhexcheck,nCheck);
		MsgWindow(bufmommom);
		if (nCheck){
			sprintf(buflog,"HTL O.K. (%d)", i) ;
			LoggaStampa(buflog);
			break ;
		}
	}

	if (i>=HTL_LOOP){
		LoggaStampa("HTL in AVARIA");
		MSGBOXCALL("HTL IN AVARIA",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore HTL");
		}
		Failed++;
	}

	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;

	return Failed;
}
//	======================================================================
//					TEST COMAUX		
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_COMAUX(void)
{
#define COMAUX_LOOP 8
	int i , Failed = 0, Passato = 0;
	
	if (TestSet.EnCOMAUX == NO) return Failed;			// esci senza far nulla se il test non è richiesto  
	if (TKTYPE==0) {
		MSGBOXCALL("TEST COMAUX NON DISPONIBILE",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore COMAUX");
		}
		Failed++;
	}else{
		COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
		sprintf(Bmom, "(%d_E:%d) TEST COMAUX", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgWindow(" ");
		LoggaStampa("TEST porta COMAUX");
		ProgressBar(BAR_TIME, 200 ) ;
		T_SetPull(NEWPMAC10 ,NEWPDWC10);  //// in10 del TK con PullDoWN
		if ((T_Input() & 0x400) == 0x400){  // Leggo in10 del TK se non è zero errore
			LoggaStampa("ATTENZIONE: Alimentazione per COMAUX presente (ma non attivata)");
			MSGBOXCALL("ATTENZIONE: Alimentazione per COMAUX in Avaria",0,2,"Continua","Ferma",bufresponse);	
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore ALIM COMAUX");
			}
			Failed++;			
		}else{
			//Devo Attivare alimentazione su MTS
			M_Action(31,11,0);  //Alimentazione ON SU COMAUX
			Delay(10);
			if ((T_Input() & 0x400) == 0x400) {  // Leggo in10 del TK se 1 OK
				//Devo Disattivare alimentazione su MTS
				M_Action(32,11,0);  //Alimentazione OFF SU COMAUX
				if ((T_Input() & 0x400) == 0) {
					LoggaStampa("Alimentazione per COMAUX presente è O.K.");
				}else{
					LoggaStampa("ATTENZIONE: Alimentazione per COMAUX presente (ma non attivata)");
					MSGBOXCALL("ATTENZIONE: Alimentazione per COMAUX in Avaria",0,2,"Continua","Ferma",bufresponse);	
					if (strcmp(bufresponse,"#!")==0) {
						call_exit(YES, "Errore ALIM COMAUX");
					}
					Failed++;
				}
			}else{
				LoggaStampa("ATTENZIONE: Alimentazione per COMAUX assente (ma attivata)");
				MSGBOXCALL("ATTENZIONE: Alimentazione per COMAUX in Avaria",0,2,"Continua","Ferma",bufresponse);	
				if (strcmp(bufresponse,"#!")==0) {
					call_exit(YES, "Errore ALIM COMAUX");
				}
				Failed++;
			}
		}
		T_SetPull(NEWPMAC09 ,NEWPUPC09);  //// in9 del TK con PullUP
		if ((T_Input() & 0x200) == 0){  // Leggo in9 del TK se è zero errore
			LoggaStampa("ATTENZIONE: Reset per COMAUX  bloccato a 0");
			MSGBOXCALL("ATTENZIONE: Reset per COMAUX  bloccato a 0",0,2,"Continua","Ferma",bufresponse);	
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore Reset COMAUX");
			}
			Failed++;			
		}else{
			//Devo Attivare reset su MTS
			M_Action(31,12,0);  //RESET ON SU COMAUX
			Delay(10);
			if ((T_Input() & 0x200) == 0) {  // Leggo in9 del TK se 0 OK
				//Devo Disattivare reset su MTS
				M_Action(32,12,0);  //RESET OFF SU COMAUX
				Delay(10);
				sprintf(Bmom,"COMAUXMTS a TK il SN è:%d", MTSdata.mSerial );
				if ((T_Input() & 0x200) == 0x200) {
					LoggaStampa("Reset per COMAUX O.K.");
				}else{
					LoggaStampa("ATTENZIONE: Reset per COMAUX  bloccato a 1");
					MSGBOXCALL("ATTENZIONE: Reset per COMAUX  bloccato a 1",0,2,"Continua","Ferma",bufresponse);	
					if (strcmp(bufresponse,"#!")==0) {
						call_exit(YES, "Errore Reset COMAUX");
					}
					Failed++;		
				}
			}else{
				LoggaStampa("ATTENZIONE: Reset per COMAUX  non attivato");
				MSGBOXCALL("ATTENZIONE: Reset per COMAUX  non attivato",0,2,"Continua","Ferma",bufresponse);	
				if (strcmp(bufresponse,"#!")==0) {
					call_exit(YES, "Errore Reset COMAUX");
				}
				Failed++;
			}
		}
		
		T_Output(NEWHTLORTTL | TRL2_ ,0);   // Seriale in TTL e Rele su NOpen sul TK
		T_SetComPort(3, 9600,'N', 8, 1,'0') ; // 9600,N,8,1 RSHTL
		
		//TEST SERIALE + TEST CTS E RTS
		for (i=0;i<COMAUX_LOOP;i++){
			///FACCIO INVIARE ALL'MTS
			sprintf(Bmom,"COMAUXMTS a TK il SN è:%d", MTSdata.mSerial );
			M_DirectTOLU(12,Bmom);
			Delay(40) ; // wait 2 sec
			if (!(T_CheckRec(3, Bmom))){
			
				for (i=0;i<COMAUX_LOOP;i++){
					///FACCIO INVIARE AL TK
					sprintf(Bmom,"COMAUXTK A MTS il SN è:%d", MTSdata.mSerial );
					char Bmomcrlf[MAXSIZE];
					sprintf(Bmomcrlf,"%s\n\r",Bmom);
					T_ComSend(3,Bmomcrlf);
					/// Controllo che ha ricevuto MTS
					M_GetDirect(bufresponse);
					char bufresponsehex[MAXSIZE];
					char Bmomhex[MAXSIZE];
					text_to_hex(bufresponse,bufresponsehex);
					text_to_hex(Bmom,Bmomhex);
					MsgWindow("\rRicevuto da MTS:{");
					MsgWindow (bufresponse);
					//MsgWindow (bufresponsehex);
					MsgWindow("}\r");
					if (strcmp(bufresponsehex,Bmomhex)==0){
						sprintf(buflog,"COMAUX O.K. (%d)", i) ;
						LoggaStampa(buflog);
						Passato = 1 ;
						break ;
					}
				}
				if (Passato) break ;
			}
		}
		M_SetPar(101, "N.D.");	
		
		if (i>=COMAUX_LOOP){
			LoggaStampa("COMAUX in AVARIA");
			MSGBOXCALL("COMAUX IN AVARIA",0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore COMAUX");
			}
			Failed++;
		}
	
		T_Output(TRL2_ ,TRL2_);   // Rele su NC sul TK
	}
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;

	return Failed;
}

//	======================================================================
//					TEST RS485			
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
#define RS485_LOOP 8
int SK_Test_RS485(void)
{
	int i, Failed = 0, Passato = 0;
	if (TestSet.EnRS485 == NO) return Failed;			// esci senza far nulla se il test non è richiesto  
	if ( 
	     (strcmp(TkIni.mName, "4004"))    && 
	     (strcmp(TkIni.mName,"MTS40A-B")) && 
	     (strcmp(TkIni.mName,"MTS40C"))   && 
	     (strcmp(TkIni.mName,"3025")) 
	    )    return Failed;			// esci senza far nulla se il test non è richiesto  
	
	sprintf(Bmom, "(%d_E:%d) TEST RS485", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	LoggaStampa("TEST porta RS485");
	ProgressBar(BAR_TIME, 200 ) ;

	if ( 
	    !(strcmp(TkIni.mName, "4004"))    || 
	    !(strcmp(TkIni.mName,"MTS40A-B")) || 
	    !(strcmp(TkIni.mName,"MTS40C"))
	   ) {
			T_SetComPort(0, 9600,'N', 8, 2, 'R') ; // 9600,N,8,2 Rs485
	 	 }else{
			T_SetComPort(0, 9600,'N', 8, 1, 'R') ; // 9600,N,8,1 Rs485
		 }
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
//	Seguiranno le operazioni da eseguire per il collaudo della porta RS485 40 4004
	if ( 
	    !(strcmp(TkIni.mName, "4004"))    || 
	    !(strcmp(TkIni.mName,"MTS40A-B")) || 
	    !(strcmp(TkIni.mName,"MTS40C"))
	   ) {
		for (i=0;i<RS485_LOOP;i++){
			M_Action(100,0,"/usr/mts40d/rs485test ") ;
			Delay(40) ; // wait 2 sec
			if (!(T_CheckRec(0, "355\r" ))){  // intero è "030355\r" // Prima "3335350D" // intero è "3033303335350D"
				sprintf(buflog,"RS485 O.K. (%d)", i) ;
				LoggaStampa(buflog);
				i = 0 ;
				break ;
			}
		}	
    }else if ( !(strcmp(TkIni.mName,"3025")) ) {
		M_SetPar(76, "42"); 							// Commuta su 485 bit 5 a 1 par 76
		
		///TEST
		for (i=0;i<RS485_LOOP;i++){
			///FACCIO INVIARE ALL'MTS
			sprintf(Bmom,"485MTS a TK il SN è:%d", MTSdata.mSerial );
			M_DirectTOLU(2,Bmom);
			Delay(40) ; // wait 2 sec
			if (!(T_CheckRec(0, Bmom))){
				
				for (i=0;i<RS485_LOOP;i++){
					///FACCIO INVIARE AL TK
					sprintf(Bmom,"485TK A MTS il SN è:%d", MTSdata.mSerial );
					char Bmomcrlf[MAXSIZE];
					sprintf(Bmomcrlf,"%s\n\r",Bmom);
					T_ComSend(0,Bmomcrlf);
					/// Controllo che ha ricevuto MTS
					M_GetDirect(bufresponse);
					char bufresponsehex[MAXSIZE];
					char Bmomhex[MAXSIZE];
					text_to_hex(bufresponse,bufresponsehex);
					text_to_hex(Bmom,Bmomhex);
					MsgWindow("\rRicevuto da MTS:{");
					MsgWindow (bufresponse);
					//MsgWindow (bufresponsehex);
					MsgWindow("}\r");
					if (strcmp(bufresponsehex,Bmomhex)==0){
						sprintf(buflog,"RS485 O.K. (%d)", i) ;
						LoggaStampa(buflog);
						Passato = 1 ;
						break ;
					}
				}
				if (Passato) break ;
			}
		}
		M_SetPar(76, "0"); 							// Commuta su RS232 bit 5 a 0 par 76
		M_SetPar(100, "N.D.");
		//i=200;	
	}
	if (i>=RS485_LOOP){
		LoggaStampa("RS485 in AVARIA");
		MSGBOXCALL("RS485 IN AVARIA",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore RS485");
		}
		Failed++;
	}
	
#ifdef DEBUG_1FR
		Repeat = 0 ;
#endif

	
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;

	return Failed;
}

//	======================================================================
//					TEST COM2 -MgetsernumCom2
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_COM2(void)
{
	int Failed = 0;
	int i, j ; // , Tens_R, Cur_R;
	
	// esci senza far nulla se il test non è richiesto  
	if ((TestSet.EnCOM2 == NO) && (TestSet.EnVcns==NO)) return Failed;			

	// "Seconda RS232"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	ProgressBar(BAR_TIME, 4 ) ;

	if (TestSet.EnCOM2){
		sprintf(Bmom, "(%d_E:%d) TEST COM Secondaria", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgWindow(" ");
		LoggaStampa("TEST COM Secondaria");
		T_SetFTDI(MtsTK.COM2);										// si commuta T-K su COM secondaria
		if (T_SetFTDI(MtsTK.COM2)){							// si commuta T-K su COM secondaria
			sprintf(Bmom, "Errore durante aperturta COM2 MTS");
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
		}	
		M_SetSourceId(MtsTK.tCOM);								// si imposta la COM di protocollo primario
		Delay(40);																// prima era 20
	
		for (j=0;j<4;j++){
			Delay(20);	//aggiunto  delay controllo
			i = M_GetSerNum() ;
			if (i==MTSdata.mSerial) break ;
		}
	
		if (i!=MTSdata.mSerial) {
			LoggaStampa("ATTENZIONE: COM Secondaria in Avaria");
			MSGBOXCALL("ATTENZIONE: COM Secondaria in Avaria",0,2,"Continua","Ferma",bufresponse);	
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore COM2");
			}
			Failed++;
		}else{
			LoggaStampa("--> COM Secondaria O.K.");
		}
		M_SetSourceId(MtsTK.mCOM);															// si re-imposta la COM di protocollo primario
		T_SetFTDI(MtsTK.COM1);																	// si ri-commuta T-K su COM primaria
		if (T_SetFTDI(MtsTK.COM1)){															// si ri-commuta T-K su COM primaria
			sprintf(Bmom, "Errore durante aperturta COM MTS");
	  	PrintDB(Bmom);
	  	call_exit(YES, Bmom);
		}	
		M_SetPar(76, "0");													// Imposta il protocollo BCS su COM1 (dopo il reboot)
		TestSet.EnVcns=1;
	}
	
//
//	Test dell'alimentazione fornita al terminale seriale (param 77 = 1)
//	
	sprintf(Bmom, "EnVcns:%d_EnVext:%d TEST VConsole", TestSet.EnVcns, TestSet.EnVext);
	MsgWindow(Bmom);
	if ((TestSet.EnVcns) && (TestSet.EnVext) ){
		
		int MOK_CNS_;
		
		sprintf(Bmom, "(%d_E:%d) TEST VConsole", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgWindow(" ");
		LoggaStampa("TEST VConsole");

		while (T_Analog(TVEXT)<100){ // <5V
			MSGBOXCALL("ATTENZIONE: Fornire alimentazione all'MTS!",0,2,"Continua","Ferma",bufresponse);	
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore TK_VEXT");
			}
		}
	

	// Aggiunto verifica Vcns ( tolgo e controllo) Action: ACT_PHERIFOFF, PHE_12VCNS
		MOK_CNS_= (T_Input() & OK_CNS_);
		if ( TKTYPE==1 ) MOK_CNS_^=OK_CNS_;
		if ( MOK_CNS_== 0) {
			LoggaStampa("ATTENZIONE1: Alimentazione per console presente (ma non attivata)");
			MSGBOXCALL("ATTENZIONE: Alimentazione per console in Avaria",0,2,"Continua","Ferma",bufresponse);	
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore Vcns");
			}
			Failed++;
		}
		if (MOK_CNS_) LoggaStampa("TEST Assenza VConsole O.K.");
	}

	
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
	
	return Failed;
}



//	======================================================================
//					TEST CANLOGISTIC -
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_CANLOGISTIC(void)
{
	int Failed = 0;
	int i;
	
	// esci senza far nulla se il test non è richiesto
	  
	if ( TestSet.EnCANLOGISTIC == NO ) return Failed;

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	ProgressBar(BAR_TIME, 4 ) ;
	i=0;
	M_Diag(2,30, dDg); 									// Legge lo stato del sensore di vibrazione
	i = (atoi(dDg) & 15);
	sprintf(Bmom, "DIAG 2 30:%s {%d}", dDg, i);
	MsgWindow(Bmom);
	sprintf(Bmom, "(%d_E:%d) TEST CANLOGISTIC", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow (" ");
	LoggaStampa("TEST CANLOGISTIC");

	if (i<3) {
		LoggaStampa("ATTENZIONE: CANLOGISTIC in Avaria");
		MSGBOXCALL("ATTENZIONE: CANLOGISTIC in Avaria",0,2,"Continua","Ferma",bufresponse);	
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore CANLOGISTIC");
		}
		Failed++;
	}else{
		LoggaStampa("--> CAN LOGISTIC O.K.");
	}

	
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
	
	return Failed;
}

//	======================================================================
//					Test CARICA BATTERIA
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_ChBat(void)
{
	int Failed = 0;
	int i, k;
	int VrtSt ;
	
	if (TestSet.EnChBat == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	// "Carica batteria"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;

	ProgressBar(BAR_TIME, 200 ) ; // 20s
	i = 0 ;
	k = 0 ;
	sprintf(Bmom, "(%d_E:%d) TEST CARICA BATTERIA", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	while (i<5) {
		Delay(20); 																				// (0609) era 30
		float batt;
		if (
				(!strcmp(TkIni.mName, "3025")) ||
				(!strcmp(TkIni.mName, "2034")) || 
				(!strcmp(TkIni.mName, "2039")) || 
				(!strcmp(TkIni.mName, "2040")) || 
				(!strcmp(TkIni.mName, "3035")) || 
				(!strcmp(TkIni.mName, "3036")) || 
				(!strcmp(TkIni.mName, "4037")) ||
				(!strcmp(TkIni.mName, "2046")) ||
				(!strcmp(TkIni.mName, "2044")) ||
				(!strcmp(TkIni.mName, "3048")) ||
				(!strcmp(TkIni.mName, "2047")) ||
				(!strcmp(TkIni.mName, "2045")) ||
				(!strcmp(TkIni.mName, "2405")) ||
				(!strcmp(TkIni.mName, "2051")) ||
				(!strcmp(TkIni.mName, "2052")) ||
				(!strcmp(TkIni.mName, "2046_M4")) || 				 
				(!strcmp(TkIni.mName, "2044_M4")) ||
				(!strcmp(TkIni.mName, "2047_M4")) ||
				(!strcmp(TkIni.mName, "2051_M4"))
			 )
			batt=4.20;
		else
			batt=4.10;
		if (MTSdata.VbatMTS > batt) {
			//MSGBOXCALL ("ATTENZIONE!\rBATTERIA GIA' CARICA\r--- INSERIRE UNA BATTERIA SCARICA E PREMERE OK ---",0,1,0,0,bufresponse);
			//MSGBOXCALL("ATTENZIONE!\rBATTERIA GIA' CARICA\r--- INSERIRE UNA BATTERIA SCARICA E PREMERE OK ---",0,2,0,0,bufresponse);
			MSGBOXCALL("ATTENZIONE: BATTERIA CARICA",0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				break ;
				call_exit(YES, "Errore CARICA BATTERIA");
			//if (strcmp(bufresponse,"#!")==0) {
			//	break ;
			}

		}
		MsgWindow("ATTESA Carica Batteria");
		VrtSt = M_Input();
		Delay(20); 																				// (0609) era 30
		if ((VrtSt & M_CHRGING) == M_CHRGING) {
			sprintf(MyDebB,"CARICA in corso");
			MsgWindow(MyDebB);
			k++;
			break;
			//Delay(20); 																			// (0609) era 30
		}
		i++;
		if (i==2 && k==0) {
			MSGBOXCALL("ATTENZIONE!\rBATTERIA GIA' CARICA\r--- INSERIRE UNA BATTERIA SCARICA E PREMERE OK ---",0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore CARICA BATTERIA");
			}
			//i=0;	
		}
	}
	if (k==0) {
		LoggaStampa( "ATTENZIONE: Errore CARICA BATTERIA");
		MSGBOXCALL("ATTENZIONE: Errore CARICA",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore CARICA BATTERIA");
		}
		Failed++;
	}else{
		LoggaStampa("--> CARICA Batteria O.K.");
	}
	
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed;
}

//	======================================================================
//					Test Vibrazione
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_Vibro(void)
{
	int svibra ;
	int Failed = 0;
	
	if (TestSet.EnVibro == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	// "Sensore vibrazione"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;

	ProgressBar(BAR_PERC, 0 ) ;
	
//	VrtSt = M_Diag (2,0);
	M_Diag(2, 0, dDg); 									// Legge lo stato del sensore di vibrazione
	//svibra = atoi(dDg);
	sprintf(Bmom, "(%d_E:%d) TEST VIBRAZIONE", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow (" ");
	/*
	if (svibra!=0){
		MSGBOXCALL("ATTENZIONE VIBRAZIONE NON ATTESA",0,2,0,0,bufresponse);
		MsgWindow("ATTENZIONE VIBRAZIONE NON ATTESA");
	}
	*/
	MSGBOXCALL("Effettuare una vibrazione e poi cliccare su OK",0,2,0,0,bufresponse);
//	VrtSt = M_Diag (2,0);
	M_Diag(2, 0, dDg); 									// Legge lo stato del sensore di vibrazione
	svibra = atoi(dDg);
	if (svibra!=0) {
		MsgWindow("VIBRAZIONE O.K.");
		MsgFile(0, LogCollaudo, "--> VIBRAZIONE O.K.");
	}else{
		MSGBOXCALL("VIBRAZIONE NON AVVENUTA - RIPROVARE",0,2,0,0,bufresponse);
		MsgWindow("VIBRAZIONE NON AVVENUTA - RIPROVARE");
		MSGBOXCALL("Effettuare nuovamente una vibrazione e poi cliccare su OK",0,2,0,0,bufresponse);
//		VrtSt = M_Diag (2,0);
		M_Diag(2, 0, dDg); 								// Legge lo stato del sensore di vibrazione
		svibra = atoi(dDg);
		if (svibra!=0) {
			MsgWindow("VIBRAZIONE O.K.");
			MsgFile(0, LogCollaudo, "--> VIBRAZIONE O.K.");
		}else{
			MSGBOXCALL("VIBRAZIONE NUOVAMENTE NON AVVENUTA - ERRORE",0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore VIBRAZIONE");
			}
			MsgWindow("VIBRAZIONE NUOVAMENTE NON AVVENUTA - ERRORE");
			MsgFile(0, LogCollaudo, "--> VIBRAZIONE IN AVARIA");
			Failed++;
		}
	}
	ProgressBar(BAR_PERC, 100 ) ;
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed;
}

//	======================================================================
//					Test Ribaltamento
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_Accel(void)
{
	int Failed = 0;
	int VrtSt ;
	
	if (TestSet.EnAccel == NO) return Failed;	// esci senza far nulla se il test non è richiesto  

	// "Sensore accelerometrico"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	ProgressBar(BAR_TIME, 450) ; // 45s
	
	sprintf(Bmom, "(%d_E:%d) TEST RIBALTAMENTO ...ATTENDERE", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	Delay (100);               // Prima 150
//	VrtSt = M_Diag (2,1);
	M_Diag(2, 1, dDg); 							// Legge lo stato del sensore accelerometrico
	VrtSt = atoi(dDg);
	sprintf(Bmom, "Ribaltamento (%d)", VrtSt);
	MsgWindow(Bmom);
	/*
	if (VrtSt>=65536) {
	//if (VrtSt==65537) {
		MSGBOXCALL("ATTENZIONE CAMBIO ASSETTO NON ATTESO",0,2,0,0,bufresponse);
		MsgWindow("ATTENZIONE CAMBIO ASSETTO NON ATTESO");
	}
	*/
	MSGBOXCALL("Ruotare l'MTS di 90°.... poi cliccare su OK",0,2,0,0,bufresponse);
	sprintf(Bmom, "(%d_E:%d) MANTENERE QUESTA POSIZIONE", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	Delay (100);          // Prima 150
//	VrtSt = M_Diag (2,1);
	M_Diag(2, 1, dDg); 							// Legge lo stato del sensore accelerometrico
	VrtSt = atoi(dDg);
	sprintf(Bmom, "Ribaltamento (%d)", VrtSt);
	MsgWindow(Bmom);
	if (VrtSt>=65536) {
	//if (VrtSt==65537) {
		MsgWindow("RIBALTAMENTO O.K.");
		sprintf(Bmom, "(%d_E:%d) RIBALTAMENTO O.K.  ->  RIPOSIZIONARE MTS", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		MsgFile(0, LogCollaudo, "--> RIBALTAMENTO O.K.");
	}else{
		ProgressBar(BAR_TIME, 250) ; // 45s
		MSGBOXCALL("RIBALTAMENTO NON AVVENUTO - RIPROVARE",0,2,0,0,bufresponse);
		MsgWindow("RIBALTAMENTO NON AVVENUTO - RIPROVARE");
		MSGBOXCALL("Ruotare l'MTS di 90°.... poi cliccare su OK",0,2,0,0,bufresponse);
		sprintf(Bmom, "(%d_E:%d) MANTENERE QUESTA POSIZIONE", MTSdata.mSerial, MTSdata.ERRTest);
		OuputText(1,Bmom,0,0,0,0);
		Delay(100);    // Prima 150
//		VrtSt = M_Diag (2,1);
		M_Diag(2, 1, dDg); 						// Legge lo stato del sensore accelerometrico
		VrtSt = atoi(dDg);
		sprintf(Bmom, "Ribaltamento (%d)", VrtSt);
		MsgWindow(Bmom);
		if (VrtSt>=65536) {
		//if (VrtSt==65537) {
			MsgWindow("RIBALTAMENTO O.K.");
			sprintf(Bmom, "(%d_E:%d) RIBALTAMENTO O.K.  ->  RIPOSIZIONARE MTS", MTSdata.mSerial, MTSdata.ERRTest);
			OuputText(1,Bmom,0,0,0,0);
			MsgFile(0, LogCollaudo, "--> RIBALTAMENTO O.K.");
		}else{
			MSGBOXCALL("RIBALTAMENTO NUOVAMENTE NON AVVENUTO - ERRORE",0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore RIBALTAMENTO");
			}
			MsgWindow("RIBALTAMENTO NUOVAMENTE NON AVVENUTO - ERRORE");
			MsgFile(0, LogCollaudo, "--> RIBALTAMENTO IN AVARIA");
			Failed++;
		}
	}

	if ( !strcmp(TkIni.mName, "3025") ) {
	///Test Giroscopio per 3025
			int okgyro;
			sprintf(dDg,"0");
			M_Diag(2,9,dDg) ;
			okgyro = atoi(dDg) ;
			if (okgyro){
				MsgWindow("GIROSCOPIO O.K.");
				sprintf(Bmom, "(%d_E:%d) GIROSCOPIO O.K.", MTSdata.mSerial, MTSdata.ERRTest);
				OuputText(1,Bmom,0,0,0,0);
				MsgFile(0, LogCollaudo, "--> GIROSCOPIO O.K.");
			}else{
				Failed++;
				MsgWindow("GIROSCOPIO IN AVARIA - ERRORE");
				MsgFile(0, LogCollaudo, "--> GIROSCOPIO IN AVARIA");
				MSGBOXCALL("GIROSCOPIO IN AVARIA - ERRORE",0,2,"Continua","Ferma",bufresponse);
				if (strcmp(bufresponse,"#!")==0) {
					call_exit(YES, "Errore GIROSCOPIO");
				}
			}
	}
	
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed;
}

//	======================================================================
//					Test Laser Scanner
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_TestLaserS(void)
{
	int Failed = 0;
	int i, j, nloop,nr_read ;
	int k ;
	
	if (TestSet.QtaLaserS == 0){
#ifdef USE_DEBUG
		sprintf(bufwindow,"QtaLaserS=%s (%d)", QtaLaserS, strcmp(QtaLaserS,"0") ) ;
		MsgWindow(bufwindow) ;
#endif		
		return Failed;			// esci senza far nulla se il test non è richiesto  
	}

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;

	nr_read = nloop = 0 ;
	
	
	// VB: Setta free label rosse in attesa delle letture
	for (i=TestSet.QtaLaserS; i ; i--){
		sprintf(Bmom, "Cod.%d", i) ;
		OuputText(9-(TestSet.QtaLaserS-i),"",4,Bmom,1,1);
	}
	
	ProgressBar(BAR_PERC, 0 ) ;

	// VB: Cancella ultimo IDCMD_DIRECT ricevuto
	M_GetDirect(dDg) ;
	
	sprintf(Bmom, "(%d_E:%d) TEST LASER SCANNER", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	
	// VB: Visualizza messaggio di attesa scanner (se beep lungo leggere codice di COM setting)
	MSGBOXCALL("Prepararsi con i codici a barre da leggere\rCliccare su OK quando pronti\rIniziare leggendo l'Attivazione",0,2,0,0,bufresponse);
	if (strcmp(bufresponse,"#!")==0) {
		COLOR_STEP(MtsTK.steptest++, C_RED) ;	
		return 1 ; // Error
	}
	
	nloop = 0 ;
	nr_read= 0;
	for(;;){	// Loop di lettura			
		nloop++ ;
		// VB: Attivazione LaserScanner
		M_Action( 100, 127, 0 ) ; 		// Accende Laser e richiede invio DIRECT come pkt
		
		ProgressBar(BAR_TIME, 250 ) ;
		
		// VB: Legge Direct
		//for (i=0;i<2;i++){ // 5
			Delay(5);		// 25
			dDg[0]='\0' ;
			M_GetDirect(dDg) ;
			
			if(strlen(dDg)>0){							 // If a read
				//sprintf (bufwindow,"Recv: %s", dDg);
				//MsgWindow (bufwindow);
				UpperAlfaNum(dDg) ;
				for (j=0;j<TestSet.QtaLaserS;j++){
					k = strcmp(dDg, LS_barcode[j].wait_mgs) ;
					
					if (!k ) {	// if a waited
						if (! LS_barcode[j].recv){					// if not already
							LS_barcode[j].recv = 1 ;
							nr_read++ ;
							// Done message to user
							sprintf(Bmom, "Cod.%d", j+1) ;
							OuputText(9-TestSet.QtaLaserS+j+1,"OK",2,Bmom,0,0);
							if (nr_read==TestSet.QtaLaserS){
								COLOR_STEP(MtsTK.steptest++, C_GREEN) ;	
								return 0 ;
							}
						}else{
						sprintf(bufwindow,"Check%d: %d %d %s", j, k, LS_barcode[j].recv, LS_barcode[j].wait_mgs );
						MsgWindow(bufwindow);
						}
					}
				}
				Delay(5); // 25
			}
		//} // for (i=0;i<2;i++)
		if (nloop>5){
			if (!nr_read)
				strcpy(bufmsg, "NESSUN CODICE RICEVUTO!\r") ;
			else
				strcpy(bufmsg, "Alcuni codici NON ricevuti!\r") ;
				
			strcat(bufmsg,"Prepararsi con i codici a barre da leggere\rCliccare su OK quando pronti") ;
			MSGBOXCALL(bufmsg,0,2,0,0,bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				COLOR_STEP(MtsTK.steptest++, C_RED) ;	
				return 1 ; // Error
			}else{
				nloop=0;
			}
		}
		if (nr_read>=TestSet.QtaLaserS) break;
	} // for(;;)			
	COLOR_STEP(MtsTK.steptest++, C_RED) ;	
	
	return 1;
}

//	======================================================================
//					TEST LAN 
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_TestLAN(void)
{
	int Failed = 0;
	int i, j ; // , Tens_R, Cur_R;
	
	if (TestSet.NetIP == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;


	sprintf(Bmom, "(%d_E:%d) TEST Rete TCP/IP", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	LoggaStampa("TEST Rete TCP/IP");

	ProgressBar(BAR_TIME, 100 ) ;
	if (!strcmp(TkIni.mName, "3048")) {
		//set ip on mts;
	}else{
		sprintf(Bmom,"exe ifconfig eth0 %s", TestSet.CnetIP ) ;
		M_Action(0,0, Bmom) ;
		Delay(40);
	}
	
// VB: Visualizza messaggio di attesa LAN
	MSGBOXCALL("Connettere il cavo di rete all'MTS\rCliccare su OK quando eseguito",0,2,0,0,bufresponse);
	if (strcmp(bufresponse,"#!")==0) {
		COLOR_STEP(MtsTK.steptest++, C_RED) ;	
		return 1 ; // Error
	}
	
	M_SetSourceId(TestSet.NetIP);						// si imposta la LAN come porta di comunicazione
	
	for (j=0;j<4;j++){	
		i = M_GetSerNum() ;
		if (i==MTSdata.mSerial) break ;
	}
	

	M_SetSourceId(MtsTK.mCOM);												// si re-imposta la COM di protocollo primario
	
	if (i!=MTSdata.mSerial){
		MSGBOXCALL("ATTENZIONE: test LAN",0,2,"Continua","Ferma",bufresponse);	
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore LAN");
		}
		Failed++ ;
		LoggaStampa("Rete TCP/IP in avaria - ERRORE");
	}else{
		LoggaStampa("TEST Rete TCP/IP O.K.");
#ifdef DEBUG_1FR
		Repeat = 0 ;
#endif
	}
	M_SetSourceId(MtsTK.mCOM);						// si re-imposta la COM di protocollo primario
	
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
	
	return Failed;
}


//	======================================================================
//					TEST porte USB 
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_TestUSB(void)
{
	int Failed = 0;
	int nrUSB, i, j, nrTest ; // , Tens_R, Cur_R;
	
	if (TestSet.QtaUSB == NO) return Failed;			// esci senza far nulla se il test non è richiesto  

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;

	i = 30 * TestSet.QtaUSB ;
	ProgressBar(BAR_TIME, i ) ;

	sprintf(Bmom, "(%d_E:%d) TEST porte USB", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	LoggaStampa("TEST porte USB");

#ifdef DEBUG_1FR
	MSGBOXCALL("INIZIO test delle porte USB",0,2,"Continua","Ferma",bufresponse);	
	if (strcmp(bufresponse,"#!")==0) {
		call_exit(YES, "Errore porte USB");
	}
#endif

	M_Action(100,0,"dmesg -c") ;		// Clear dmesg	
	Delay(10) ; // wait 1 sec
	nrUSB = 0 ;

	
	nrTest = 0 ;
	while (nrUSB<TestSet.QtaUSB){
		if (!nrUSB){
			if (TestSet.QtaUSB==1)
				strcpy(bufmsg, "Inserire la chiavetta nella porta USB" ) ;
			else
				strcpy(bufmsg, "Inserire la chiavetta in una porta USB" ) ;
		}else if (TestSet.QtaUSB==(nrUSB+1))
			strcpy(bufmsg, "Inserire la chiavetta nell'altra porta USB" ) ;
		else
			strcpy(bufmsg, "Inserire la chiavetta in un'altra porta USB" ) ;

		MSGBOXCALL(bufmsg,0,2,"Continua","Ferma",bufresponse);	
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore porte USB");
		}
		M_Action(0,0,"exe dmesg -c") ;		// get Clear dmesg	
		Delay(20) ; // wait 2 sec
		
		M_Diag(200,0,dDg) ;
		i = atoi(dDg) ;
		
		nrUSB = 0 ;
		for (j=0;j<8;j++){
			if (i & (1<<j)) nrUSB++;
		}
		if (nrUSB<TestSet.QtaUSB){
			if ((++nrTest)==5){
				MSGBOXCALL("Test porte USB non completato",0,2,"Ripeti","Salta",bufresponse);	
				if (strcmp(bufresponse,"#!")==0) {
					Failed++ ;
					break ;
				}			
			}
		}
	}
	
	if (Failed){
		LoggaStampa("Porte USB in avaria - ERRORE");
	}else{
		LoggaStampa("TEST porte USB O.K.");
#ifdef DEBUG_1FR
		Repeat = 0 ;
#endif
	}
		
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
	
	return Failed;
}

//	======================================================================
//					Test Display LCD
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_Test_LCD(void)
{
	int i, Failed = 0;
	int VrtSt ;
	char MexShow[NRMSIZE];

	if (TestSet.EnLCD == NO) return Failed;	// esci senza far nulla se il test non è richiesto  

	// "Display LCD"
	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	ProgressBar(BAR_TIME, 450) ; // 45s
	
	sprintf(Bmom, "(%d_E:%d) TEST Display LCD ...ATTENDERE", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	
	// Verifica che il Jumper non sia inserito (a 1 se non presente)
	VrtSt = M_Input() & 0x20 ;
	while (!(VrtSt)){
		MSGBOXCALL("Togliere il Jumper vicino al Display,\re attendere 5 secondi.\rEseguito?" ,"Display LCD" ,2 ,"Si" ,"Ferma" , bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			sprintf(MyDebB,"Jumper Display presente, in Avaria\r");
			MSGBOXCALL("Jumper Display presente, in Avaria\r",0,2,0,0,bufresponse);
			MsgWindow(MyDebB);
			MsgFile(0, LogCollaudo, MyDebB);
			COLOR_STEP(MtsTK.steptest++, C_RED ) ;	
			call_exit(YES, "C_NO_JumperLCD");
		}
		VrtSt = M_Input() & 0x20 ;
	}
//	Delay (150);
//	VrtSt = M_Diag (2,1);
	M_Diag(128, 0, dDg); 							// Imposta tutto acceso su LCD

// Verifica tutti accesi i segmenti e le 10 icone
	if (strcmp(TkIni.mName,"2022")){
		MSGBOXCALL("Sono perfetti i caratteri che scorrono?" ,"Display LCD" ,2 ,"Si" ,"No" , bufresponse);
	}else{
		MSGBOXCALL("Sono accesi tutti i segmenti e le 10 icone?" ,"Display LCD" ,2 ,"Si" ,"No" , bufresponse);
	}
	if (strcmp(bufresponse,"#!")==0) {
		Failed++;
		sprintf(MyDebB,"Display LCD in Avaria\r");
		MSGBOXCALL("Display LCD in Avaria\r",0,2,0,0,bufresponse);
		MsgWindow(MyDebB);
		MsgFile(0, LogCollaudo, MyDebB);
		COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
		return Failed;
	}

	// Crea un numero random
	srand ( time(NULL) );
	VrtSt = (rand() *10) + 6 ;
	if (VrtSt<0) VrtSt=VrtSt*-1;
	M_Diag(128, VrtSt, dDg); 
	strcpy(	MexShow, "Digitare il numero visualizzato sul LCD" ) ;
	
	for(i=0;i<3;i++){
		sprintf(bufmsg , "Il numero visualizzato sul LCD é %d ?", VrtSt) ;
		MSGBOXCALL(bufmsg,"Display LCD" ,2 ,"Si" ,"No" , bufresponse);
		//INPUTBOXCALL("Digitare il numero visualizzato sul LCD", "Valore",2,"OK","Ferma", dDg) ;		// richiesta codice mother board
		if (strcmp(bufresponse,"#!")==0) {
			Failed++;
			sprintf(MyDebB,"Display LCD in Avaria\r");
			MSGBOXCALL("Display LCD in Avaria\r",0,2,0,0,bufresponse);
			MsgWindow(MyDebB);
			MsgFile(0, LogCollaudo, MyDebB);
			COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
			call_exit(YES, "C_NO_DisplayLCD");
		}else
			break ;
		// if (atoi(dDg)==VrtSt){
			// i = 0 ;
			// break ;
			// //COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;		
			// //return Failed;
		// }
		
		strcpy(MexShow, "ERRORE:\r Reinserire il numero visualizzato sul LCD" );
		
	}
	
	if (i) Failed++ ;
	
	// Test del Jumper
	VrtSt = 0x20 ;
	while (VrtSt){
		MSGBOXCALL("Inserire il Jumper vicino al Display.\rEseguito?" ,"Display LCD" ,2 ,"Si" ,"Ferma" , bufresponse);
		if (strcmp(bufresponse,"#!")==0) {	
			sprintf(MyDebB,"Jumper Display assente, in Avaria\r");
			MSGBOXCALL("Jumper Display assente, in Avaria\r",0,2,0,0,bufresponse);
			MsgWindow(MyDebB);
			MsgFile(0, LogCollaudo, MyDebB);
			COLOR_STEP(MtsTK.steptest++, C_RED ) ;	
			call_exit(YES, "C_EVER_JumperLCD");
		}
		VrtSt = M_Input() & 0x20 ;
	}
	
	while (!(VrtSt)){
		MSGBOXCALL("Togliere il Jumper vicino al Display,\re attendere 5 secondi.\rEseguito?" ,"Display LCD" ,2 ,"Si" ,"Ferma" , bufresponse);
		if (strcmp(bufresponse,"#!")==0) {	
			sprintf(MyDebB,"Jumper Display presente, in Avaria\r");
			MSGBOXCALL("Jumper Display presente, in Avaria\r",0,2,0,0,bufresponse);
			MsgWindow(MyDebB);
			MsgFile(0, LogCollaudo, MyDebB);
			COLOR_STEP(MtsTK.steptest++, C_RED ) ;	
			call_exit(YES, "C_NO_JumperLCD");
		}
		VrtSt = M_Input() & 0x20 ;
	}
	
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	return Failed;
}

//	======================================================================
//					Test Emergency Link
//	======================================================================
/* la funzione torna, in uscita, un valore :
					-  0 (zero) non ci sono stati errori
					-  # (numero) numero di errori incontrati
*/
int SK_TestEmeLink(void)
{
	int k, k0, k1, Failed = 0;
	int cntr ;
	int in_emlink;
	if (TestSet.EnEmeLink == 0)
		return Failed;			// esci senza far nulla se il test non è richiesto  

	COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	ProgressBar(BAR_TIME, 100 ) ; // 10 sec

	
	sprintf(Bmom, "(%d_E:%d) TEST EMERGENCY LINK", MTSdata.mSerial, MTSdata.ERRTest);
	OuputText(1,Bmom,0,0,0,0);
	MsgWindow(" ");
	LoggaStampa("TEST di EMERGENCY LINK");

	if ( (!(strcmp(TkIni.mName, "3025"))) && (TKTYPE==0) ){
		MSGBOXCALL("TEST di EMERGENCY LINK NON DISPONIBILE",0,2,"Continua","Ferma",bufresponse);
		if (strcmp(bufresponse,"#!")==0) {
			call_exit(YES, "Errore EMERGENCY LINK");
		}
		Failed++;		
	}else{
		// Imposta:
			// PB29 a 1
		if ( strcmp(TkIni.mName, "3025") ) T_Output(TRL2_, TRL2_);
			// PD03 a 1
		if (TKTYPE==0){
			if (TkIni.mName[0]=='3')
				T_Output(0x04000, 0x04000); 
			else
				T_Output(0x80000, 0x80000); 
		}else{
				T_SetPull((NEWPMAC02 | NEWPMAC07 | NEWPMAC08) , (NEWPDWC02 | NEWPDWC07 | NEWPDWC08));  //// Metto in2,in7 e in8 prima in pull-down prima di metterli flottanti perchè così da flottanti partono da 0 
				T_SetPull((NEWPMAC02 | NEWPMAC07 | NEWPMAC08) , (NEWFLOC02 | NEWFLOC07 | NEWFLOC08));  //// in2,in7 e in8 flottanti
		}

		if (TkIni.mName[0]=='3') in_emlink=EMLNK_IN3x; 					// Leggo C14(OLD) C02(NEW)
		if (TkIni.mName[0]=='2') in_emlink=EMLNK_IN2x; 					// Leggo D15(OLD) C07(NEW)
		if (!(strcmp(TkIni.mName, "3025")) || !(strcmp(TkIni.mName, "3035")) || !(strcmp(TkIni.mName, "3036")) || (!strcmp(TkIni.mName, "4037"))) in_emlink=NEWEMLNK_IN3025;	// Leggo		  C08(NEW)
	
		if ( (T_Input() & in_emlink)==0 ){
			sprintf(MyDebB,"EMERGENCY LINK: Manca Pull-up nel MTS");
			Failed++ ;	
		}else{
			cntr = 0 ;
			// Avvia il TEST (DIAG=7, val=255)
			M_Diag(DIAG_EMLNK, 255, dDg); 
			// Salva valore di avvio del diag
			k0 = atoi(dDg) ;
			// Per 2 volte :
			for(k=0;k<3;k++){
				// Attende 1 sec
				Delay(10) ;
				if (!(strcmp(TkIni.mName, "3025")) || !(strcmp(TkIni.mName, "3035")) || !(strcmp(TkIni.mName, "3036")) || (!strcmp(TkIni.mName, "4037"))) {
					if ( (T_Input() & in_emlink ) == 0 )  cntr++ ;
				}else{
					if (T_Input() & in_emlink ) cntr++ ;
				}	
			}
			if (!cntr) {
				sprintf(MyDebB,"EMERGENCY LINK: Nessuna variazione rilevata dal TestKit");
				Failed++ ;	 
			}else
				sprintf(MyDebB,"EMERGENCY LINK: TestKit letto segnali (%d) ...", cntr);
			LoggaStampa(MyDebB);

			//Ferma Emmisione
			M_Diag(DIAG_EMLNK, 0, dDg);
			// Imposta:
			// PB29 a 0
			if ( strcmp(TkIni.mName, "3025") ) T_Output(TRL2_, 0);
			//Avvia Ricezione
			M_Diag(DIAG_EMLNK, 1, dDg); 
			// Attende 1 sec
			Delay(10) ;
			// Per 4 volte :
			for(k=0;k<4;k++){
				// Imposta D05 a 1
				T_Output(TOD1, TOD1) ; // ATTENZIONE: STESSA DEL RELE 1
				// Attende 0.20 sec
				Delay(2) ; //prima 2
				// Imposta D05 a 0
				T_Output(TOD1, 0) ;
				// Attende 0.20 sec
				Delay(2) ; // Prima 2
			}
			// Richiede valore del TEST su MTS (DIAG=7, val=2)
			//Delay(20) ;
			M_Diag(DIAG_EMLNK, 2, dDg);
			//Ferma Emmisione
			M_Diag(DIAG_EMLNK, 0, dDg);
			// Legge valore di fine
			k1 = atoi(dDg) ;
			// Lo confronta con valore di avvio: se uguale -> Errore
			if (k1==k0){
				sprintf(MyDebB,"EMERGENCY LINK: Nessuna variazione rilevata dal MTS (%d)", k1);
				Failed++ ;	 
			}else
				sprintf(MyDebB,"EMERGENCY LINK: MTS ha rilevato segnali (%d,%d)", k0, k1);

			LoggaStampa(MyDebB);
		}
	
		if (Failed){
			MSGBOXCALL("EMERGENCY LINK IN AVARIA" ,0,2,"Continua","Ferma",bufresponse);
			if (strcmp(bufresponse,"#!")==0) {
				call_exit(YES, "Errore EMERGENCY LINK");
			}
			LoggaStampa(MyDebB);
		}
	}
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;	
	
	return Failed ;
}
//	======================================================================

int SK_EndChk(void)
{
	int i, k , valido,Failed=0 ;
	unsigned int NextNum ;
	FILE *fmacs ;
	char LogCollaudoNew[MAXSIZE] ;
	char MexShow[NRMSIZE];

	// "Impostazioni di fine collaudo"
	//COLOR_STEP(MtsTK.steptest, C_YELLOW) ;
	
	ProgressBar(BAR_TIME, 400 ) ; // 40 s
	
 // "EndMex" prepara il messaggio finale se ci sono stati problemi
	sprintf(EndMex, "MTS %s, s/n %d (Ver.%5.2f): in AVARIA (Er:%d)", TkIni.mName, MTSdata.mSerial, MTSdata.SVer, MTSdata.ERRTest);
	//PrintDB(EndMex);
	if ( MTSdata.ERRTest == 0 ) {
		//Spostato prima macchina stati e parametri
		if (!(strcmp(TkIni.mName, "2023")))  M_SetPar(75,"N.D.");     //si cancella parametro 75
		if (!(strcmp(TkIni.mName, "2023")))  M_DelPar(75);     		  //si cancella parametro 75
		if (!(strcmp(TkIni.mName, "2022")))  M_SetPar(75,"N.D.");     //si cancella parametro 75
		if (!(strcmp(TkIni.mName, "2022")))  M_DelPar(75);            //si cancella parametro 75
		if (!(strcmp(TkIni.mName, "2122")))  M_SetPar(75,"N.D.");     //si cancella parametro 75
		if (!(strcmp(TkIni.mName, "2122")))  M_DelPar(75);            //si cancella parametro 75
		M_SetPar(104,"N.D.");     //si cancella parametro 104
		M_DelPar(104);            //si cancella parametro 104
		if (TestSet.EnHTL == YES) M_SetPar(105,"N.D.");     //si setta parametro 105 a 0x100 (per bloccare l'HTL come VDO a 10400)	
		if (TestSet.EnSMset == YES) {
			//M_SetPar(255,"N.D.");  //Cancello Tutti i Parametri
			SK_ParamSet();
			SK_ParamSetCheck();
			GetINIKeyVal("StateMachine", TkIni.NamSM);
			togliCR(TkIni.NamSM);
			sprintf(SMachSel, "%s\\%s", mRoot,TkIni.NamSM);
			CONVERTPATH(SMachSel);
			StampaDB("Macchina Stati:",SMachSel);	
			fmacs = fopen(SMachSel, "r");
		}else{
			if ( MtsTK.nuovo ) {
				M_SetPar(8,  "2");
				M_SetPar(65, "12"); 
				M_SetPar(69, "0");							// Ripristino dei parametri iniziali
				M_SetPar(70, "1"); 	
				M_SetPar(77, "0");	
				M_SetPar(79, "22588");
				M_SetPar(185, "ibox.tim.it"); 
				M_SetPar(186, "88.32.160.199");
				fmacs= fopen(SMachTest, "r");
			}else{
				fmacs=NULL;
				if (TestSet.EnBKdata == 1) {
					SK_RestoreParam();																// Ripristino dei parametri iniziali
					remove(ParamFile);
					fmacs = fopen(SMachFile, "r");
				}else{
						M_SetPar(65, "12"); 
						M_SetPar(69, "0");							// Ripristino dei parametri iniziali
						M_SetPar(70, "1"); 	
						M_SetPar(77, "0");	
						M_SetPar(79, "22588");
						M_SetPar(185, "ibox.tim.it"); 
						M_SetPar(186, "88.32.160.199");
				}
				M_SetPar(8, "2");
			}
		}
		if (fmacs!=NULL) {// Se è stato salvato il file di Macchina a Stati
			fclose(fmacs);
			if ( MtsTK.nuovo && (TestSet.EnSMset != YES) ) {
				sprintf(Bmom, "(%d_E:%d) ATTENDERE: invio Macchina-Stati di Fine-Collaudo", MTSdata.mSerial, MTSdata.ERRTest);
				OuputText(1,Bmom,0,0,0,0);			
				M_PutSmFile(SMachTest);																// Invia la Macchina a Stati di MTS Testato(fine collaudo)
				int k,r ;
				for (k=0;k<150;k++){	
					r = M_GetSerNum() ;
					if (r!=MTSdata.mSerial) break ;
				}
				Delay(15);
				for (k=0;k<150;k++){	
					r = M_GetSerNum() ;
					if (r==MTSdata.mSerial) break ;
				}
				// Delay(400); Prima per ATTENDERE RISVEGLIO MTS
				MsgFile(0, LogCollaudo, "Inviata Macchina-Stati di Fine-Collaudo");
			}
			if (TestSet.EnBKdata == 1) {
				sprintf(Bmom, "(%d_E:%d) ATTENDERE: ripristino Macchina-Stati iniziale", MTSdata.mSerial, MTSdata.ERRTest);
				OuputText(1,Bmom,0,0,0,0);
				M_PutSmFile(SMachFile);															// Ripristino della macchina a stati iniziale
				int k,r ;
				for (k=0;k<150;k++){	
					r = M_GetSerNum() ;
					if (r!=MTSdata.mSerial) break ;
				}
				Delay(15);
				for (k=0;k<150;k++){	
					r = M_GetSerNum() ;
					if (r==MTSdata.mSerial) break ;
				}
				// Delay(400); Prima per ATTENDERE RISVEGLIO MTS
				MsgFile(0, LogCollaudo, "Macchina-Stati Ripristinata");
				MsgWindow ("Macchina-Stati Ripristinata");
				Delay(10);
				remove(SMachFile);
			}
			if (TestSet.EnSMset == YES) {
				SK_SM_StateSet();
				if(SK_SM_StateSetCheck()) Failed++;
				//sprintf(Bmom, "(%d_E:%d) ATTENDERE: caricamento Macchina-Stati: %s ", MTSdata.mSerial, MTSdata.ERRTest,TkIni.NamSM);
				//OuputText(1,Bmom,0,0,0,0);
				//M_PutSmFile(SMachSel);
	
				/*
				int k,r ;
				for (k=0;k<150;k++){	
					r = M_GetSerNum() ;
					if (r!=MTSdata.mSerial) break ;
				}
				Delay(15);
				for (k=0;k<150;k++){	
					r = M_GetSerNum() ;
					if (r==MTSdata.mSerial) break ;
				}
				*/
				// Delay(400); Prima per ATTENDERE RISVEGLIO MTS
				/*
				sprintf(SMachFile, "%s\\Logs\\%s_%d.smk", mRoot,hostname ,MTSdata.mSerial);
				CONVERTPATH(SMachFile) ;
				M_GetSmFile(SMachFile);
				sprintf(Bmom, "Richiesta macchina stati per confronto: %s",SMachFile);
				MsgFile(0, LogCollaudo, Bmom);
				MsgWindow (Bmom);
				unsigned int bufcomp;
				bufcomp=0;
				// Compare File
				//Delay(300); //messo per cambiare file a mano
				FileCompare(SMachSel,SMachFile,&bufcomp);
				sprintf(Bmom, "Bufcomp: %d",bufcomp);
				PrintDB(Bmom);
				if(!bufcomp) {
					M_PutSmFile(SMachSel);
					int k,r ;
					for (k=0;k<150;k++){	
						r = M_GetSerNum() ;
						if (r!=MTSdata.mSerial) break ;
					}
					Delay(15);
					for (k=0;k<150;k++){	
						r = M_GetSerNum() ;
						if (r==MTSdata.mSerial) break ;
					}
					// Delay(400); Prima per ATTENDERE RISVEGLIO MTS
					M_GetSmFile(SMachFile);
					sprintf(Bmom, "Richiesta macchina stati per confronto: %s",SMachFile);
					MsgFile(0, LogCollaudo, Bmom);
					MsgWindow (Bmom);
					unsigned int bufcomp;
					bufcomp=0;
					// Compare File
					//Delay(300); //messo per cambiare file a mano
					FileCompare(SMachSel,SMachFile,&bufcomp);
					sprintf(Bmom, "Bufcomp: %d",bufcomp);
					PrintDB(Bmom);
					if(!bufcomp) {
							call_exit(YES, "File diversi");
							Failed++;
					}
				}			
				sprintf(Bmom, "Macchina-Stati Caricata: %s",SMachSel);
				MsgFile(0, LogCollaudo, Bmom);
				MsgWindow (Bmom);
				*/
			}
		}else{
			sprintf(Bmom, "(%d_E:%d) ATTENDERE: cancellazione Macchina-Stati", MTSdata.mSerial, MTSdata.ERRTest);
			OuputText(1,Bmom,0,0,0,0);
			M_PutSmFile(SMachVoid);															// Invia la Macchina a Stati vuota (x cancellarla)
			int k,r ;
			for (k=0;k<150;k++){	
				r = M_GetSerNum() ;
				if (r!=MTSdata.mSerial) break ;
			}
			Delay(15);
			for (k=0;k<150;k++){	
				r = M_GetSerNum() ;
				if (r==MTSdata.mSerial) break ;
			}
			// Delay(400); Prima per ATTENDERE RISVEGLIO MTS
			MsgFile(0, LogCollaudo, "Cancellata Macchina-Stati di Collaudo");
			MsgWindow ("Cancellata Macchina-Stati di Collaudo");
		}
	
		if (TKERROR == 0){
			MsgWindow ("Invio Macchina-Stati Esito:Positivo");
			StampaDB("Invio Macchina-Stati Esito","Positivo"); // Su finestra DOS di Debug 
		}else{
			MsgWindow ( "Invio Macchina-Stati Esito:Negativo");
			StampaDB("Invio Macchina-Stati Esito","Negativo");
		}
		//Spostato prima macchina stati e parametri
		if ( MtsTK.nuovo ) {																// Collaudo superato e quindi ...
			sprintf(Bmom, "(%d_E:%d): s/n Assegnato = %s", MTSdata.mSerial, MTSdata.ERRTest, TkIni.NewNum);
			OuputText(1,Bmom,0,0,1,7);							// ... scrivo il Serial Number nella Output Text 
			for (i=1; i<=4; i++) {
				GetINIKeyVal(nextip, TkIni.NewNum);
				togliCR(TkIni.NewNum);
				StampaDB("NewNum", TkIni.NewNum);
				M_SetPar(255, TkIni.NewNum);					// ... si inizializza il Serial Number    
				Delay(10);
				k = M_GetSerNum();								// si rilegge il Serial-Number
				if (k==-10) {									// attesa blocco JTAG
					sprintf(bufwindow,"Attesa blocco JTAG");
					LoggaStampa(bufwindow);
					PrintDB(bufwindow);
					int nn,kk,rr ;
					char diag3;
					nn = 0 ;
					for (kk=0;kk<150;kk++){	
						rr = M_GetSerNum() ;
						if (rr!=-10) break ;
					}
					Delay(15);
					for (kk=0;kk<150;kk++){	
						rr = M_GetSerNum() ;
						if (rr==atoi(TkIni.NewNum)) break ;
					}
					k = M_GetSerNum();
					M_Diag(3,0,dDg); 							// verifica Blocco diag 3 se a 1 bloccato
					diag3=(char) atoi(&dDg[4]);
					if ((diag3 & 0xFF)!=0){
						sprintf(Bmom,"Eseguito!");
					}else{
						M_Diag(250,0,dDg);                   	//Sblocco Scrittura S/N
						M_SetPar(255,"-1");				   		//Scrivo S/N -1
						k = M_GetSerNum();
						sprintf(Bmom, "(%d_E:%d): s/n Assegnato = %d", MTSdata.mSerial, MTSdata.ERRTest, k);
						OuputText(1,Bmom,0,0,1,7);				// ... scrivo il Serial Number nella Output Text
						sprintf(Bmom,"Fallito! [%x]",diag3);
					}
					sprintf(bufwindow,"Blocco JTAG %s",Bmom);
					LoggaStampa(bufwindow);
					PrintDB(bufwindow);
					if (nn==0){
						k = M_GetSerNum();
						if (k!=-1){
							while (k!=-1) {
								sprintf(MyDebB,"Attesa cambio S/N a -1");
								PrintDB(MyDebB);
								M_Diag(250,0,dDg);                  //Sblocco Scrittura S/N
								M_SetPar(255,"-1");				   //Scrivo S/N -1
								Delay(20);
								k = M_GetSerNum();
							}
						}
						call_exit(YES, bufwindow);
						Failed++;
					}
					if (k==-1) break;					
				}
				if (k!=-1) break;
			}
			if (atoi(TkIni.NewNum)!=k){
				MSGBOXCALL("Errore Serial Number",0,2,0,0,bufresponse);
			}else{
				sprintf(buflog,"%s %s %s %s %s %s %s %5.2f %5.2f %d %s\r", 
							TkIni.NewNum, HwSN.codMB, HwSN.codSV[0], HwSN.codSV[1], HwSN.codSV[2], HwSN.codSV[3], HwSN.codSV[4],
							MtsTK.VextTK, MTSdata.VbatMTS, MTSdata.ValoreCSQ, MTSdata.Imei);
				MsgFile(0,LogCollaudo, buflog);
				sprintf(bufabb,"%s;%s;%s;%s;%s;%s;%s;%s;;%s", TkIni.NewNum, HwSN.codTEST, HwSN.codMB, 
								HwSN.codSV[0], HwSN.codSV[1], HwSN.codSV[2], HwSN.codSV[3], HwSN.codSV[4], hostname); //TestSet.Terzista);
				WriteAbbin(bufabb);
				PrintDB(bufabb);
				if (TestSet.EnSIMdata == 1) {
					valido = 0; 
					sprintf(MexShow, "Inserire ICCID"); 
					while (valido == 0) {
						// richiesta codice ICCID (es. 8939010001362772051)
						INPUTBOXCALL(MexShow, "ICCID",2,"Continua","Ferma", HwSN.codICCID) ;		// richiesta codice ICCID
						if (strcmp(HwSN.codICCID,"#!")==0) {
							//COLOR_STEP(MtsTK.steptest, C_RED) ;
							call_exit(YES, "C_NO_ICCID");
							Failed++;
						}
						sprintf(MexShow , "ERRORE:\r Reinserire ICCID.");
						UpperAlfaNum(HwSN.codICCID);					// si rende Maiuscolo e solo AlfaNumerico (oltre '_')
						RLTrimm(HwSN.codICCID);						// toglie i caratteri non alfanumerici prima e dopo !
						if (strlen(HwSN.codICCID) == LEN_ICCID) {		// a questo punto DEVE essere di lunghezza == di'LEN_ICCID'
							StampaDB("ICCID inserito",HwSN.codICCID);		// Su finestra DOS di Debug 
							StampaDBn("valido",valido);			// Su finestra DOS di Debug 
							strcpy(Bmom, HwSN.codICCID);							// Bmom (cioè ICCID) deve avere ...
							if (strspn(Bmom, NUMCHAR) == strlen(HwSN.codICCID)){		// ... solo cifre  ...
								valido++;
								StampaDB("codICCID",HwSN.codICCID);			// Su finestra DOS di Debug 
							}
						}
					}
					sprintf(MyDebB, "codICCID inserito:<%s>", HwSN.codICCID);
					MsgFile(0, LogCollaudo, MyDebB);
					MsgWindow (MyDebB);

					valido = 0; 
					sprintf(MexShow, "Inserire Numero Telefono"); 
					while (valido == 0) {
						// richiesta codice numTEL
						INPUTBOXCALL(MexShow, "Numero Telefono",2,"Continua","Ferma", HwSN.numTEL) ;		// richiesta codice mother board
						if (strcmp(HwSN.numTEL,"#!")==0) {
							//COLOR_STEP(MtsTK.steptest, C_RED) ;
							call_exit(YES, "C_NO_numTEL");
							Failed++;
						}
						sprintf(MexShow , "ERRORE:\r Reinserire Numero Telefono.");
						UpperAlfaNum(HwSN.numTEL);					// si rende Maiuscolo e solo AlfaNumerico (oltre '_')
						RLTrimm(HwSN.numTEL);						// toglie i caratteri non alfanumerici prima e dopo !
						if (strlen(HwSN.numTEL) == LEN_NUMTELMAX) {		// a questo punto DEVE essere di lunghezza == di'LEN_NUMTELMAX'
							StampaDB("Numero Telefono inserito",HwSN.numTEL);		// Su finestra DOS di Debug 
							StampaDBn("valido",valido);			// Su finestra DOS di Debug 
							strcpy(Bmom, HwSN.numTEL);							// Bmom (cioè numTEL) deve avere ...
							if (strspn(Bmom, NUMCHAR) == strlen(HwSN.numTEL)){		// ... solo cifre  ...
								valido++;
								StampaDB("numTEL",HwSN.numTEL);			// Su finestra DOS di Debug 
							}
						}
						if (strlen(HwSN.numTEL) == LEN_NUMTELMIN) {		// a questo punto DEVE essere di lunghezza == di'LEN_NUMTELMAX'
							StampaDB("Numero Telefono inserito",HwSN.numTEL);		// Su finestra DOS di Debug 
							StampaDBn("valido",valido);			// Su finestra DOS di Debug 
							strcpy(Bmom, HwSN.numTEL);							// Bmom (cioè numTEL) deve avere ...
							if (strspn(Bmom, NUMCHAR) == strlen(HwSN.numTEL)){		// ... solo cifre  ...
								valido++;
								StampaDB("numTEL",HwSN.numTEL);			// Su finestra DOS di Debug 
							}
						}
					}
					sprintf(MyDebB, "numTEL inserito:<%s>", HwSN.numTEL);
					MsgFile(0, LogCollaudo, MyDebB);
					MsgWindow (MyDebB);

					sprintf(bufSIM,"%s;%s;%s", TkIni.NewNum, HwSN.codICCID, HwSN.numTEL);
					WriteSIMDATA (bufSIM);
				}
				sprintf(LogCollaudoNew, "%s\\Logs\\Log%s.txt", mRoot,TkIni.NewNum);
				CONVERTPATH(LogCollaudoNew);
				PrintDB(LogCollaudoNew);
				NextNum = atoi(TkIni.NewNum) + 1;
				sprintf(TkIni.NewNum,"%d",NextNum);
				PrintDB(TkIni.NewNum);
				SetINIKeyVal(nextip, TkIni.NewNum);
				srand(time(0)); /* n is random number in range of 0 - 1 */
				char NumSet[MAXSIZE];
				unsigned int NumSetn;
				int r;
				while (1) {
					GetINIKeyVal(nextip, NumSet);
					togliCR(NumSet);
			   		NumSetn = atoi(NumSet);
			   		if ( NumSetn==NextNum ) break;
					PrintDB("Attesa incrementazione numero\n") ;
					sprintf(TkIni.NewNum,"%d",NextNum);
					PrintDB(TkIni.NewNum);
					SetINIKeyVal(nextip, TkIni.NewNum);
					sprintf(NumSet,"0");
					r=100+(rand() % 100) ; 
					Delay(r);
				}
				rename(LogCollaudo,LogCollaudoNew);					// si rinomina il file di log del collaudo
				sprintf(LogCollaudo,"%s",LogCollaudoNew);			// ed il nome che lo identifica
				MTSdata.mSerial = k ;
			}
		}else { //se non nuovo
			M_Diag(3,0,dDg); 							// verifica Blocco 5 byte di diag 3 se a 1 bloccato
			char diag3;
			diag3=(char) atoi(&dDg[4]);
			if ((diag3 & 0xFF)!=0){
				sprintf(Bmom,"Bloccato!");
			}else{
				sprintf(Bmom,"Non Bloccato!\rEseguire l'aggiornamento FW per abilitare blocco JTAG! [%x]",diag3);
			}
			sprintf(bufwindow,"JTAG %s",Bmom);
			MsgWindow(bufwindow);
			MsgFile(0, LogCollaudo, bufwindow);
			PrintDB(bufwindow);
			if (diag3==0){
				MSGBOXCALL(bufwindow,0,1,0,0,bufresponse);
				//call_exit(NO, bufwindow);
			}
			sprintf(Bmom,"%d", MTSdata.mSerial);
			OuputText(1, Bmom, 2, 0, 1, 7);								// Scrivo il Serial Number nella Output Text 
			sprintf(buflog,"%d %5.2f %5.2f %d %s\r", MTSdata.mSerial, MtsTK.VextTK, MTSdata.VbatMTS, MTSdata.ValoreCSQ, MTSdata.Imei);
			MsgFile(0,LogCollaudo, buflog);
		}
//												 "EndMex" modifico il messaggio finale xchè non ci sono stati problemi
		sprintf(EndMex, "MTS %s, s/n %d (Ver.%5.2f): Collaudo O.K.", TkIni.mName, MTSdata.mSerial, MTSdata.SVer);
		
	// Qui c'era blocco macchina stati e parametri
	}else{
		Failed=1;
	}
	OuputText(1,EndMex,0,0,0,0);
	MsgFile(0, LogCollaudo, EndMex);
	COLOR_STEP(MtsTK.steptest++, ( (Failed)? C_RED: C_GREEN)) ;
	call_exit(NO, "FINE COLLAUDO:\rTogliere la SIM e\rscollegare i cavi");
	////MSGBOXCALL("FINE COLLAUDO:\rTogliere la SIM e\rscollegare i cavi",0,1,0,0,bufresponse);
	return 0;
}


#endif // SENDFW

void RLTrimm(char *testo)
{
	char *pnome, stemp[MAXSIZE];
	unsigned char c;
	int lungh;

//				StampaDB("RLTrimm---> Stringa da elaborare", testo); // Su finestra DOS di Debug 
	
	if (strlen(testo) > 0) {
		strcpy(stemp, testo);
		pnome = stemp;
		c = pnome[0];
		while (!isalnum(c)) {
//				StampaDB ("pnome",pnome);					// Su finestra DOS di Debug 
			pnome++;							// toglie i caratteri NON Alfanumerici prima
			c = pnome[0];
		}
		lungh = strlen(pnome);
		c = pnome[lungh-1];
		while ( ((!isalnum(c)) && (c!='!') && (c!='?')) || (c==182) ) { // ot tab char
//				StampaDB ("pnome",pnome);		// Su finestra DOS di Debug 
			pnome[lungh-1]='\0';
			lungh--;							// toglie i caratteri NON Alfanumerici dopo
			c=pnome[lungh-1];
		}
		strcpy(testo, pnome);
	}
//				StampaDB("RLTrimm---> Stringa elaborata1", pnome); // Su finestra DOS di Debug 
//				StampaDB("RLTrimm---> Stringa elaborata", testo);  // Su finestra DOS di Debug 
}

void RLTrimmwithplace(char *testo)
{
	char *pnome, stemp[MAXSIZE];
	unsigned char c;
	int lungh;

//				StampaDB("RLTrimm---> Stringa da elaborare", testo); // Su finestra DOS di Debug 
	
	if (strlen(testo) > 0) {
		strcpy(stemp, testo);
		pnome = stemp;
		c = pnome[0];
		// FR 231218
//		while ( ((!isalnum(c)) && (c!='!') && (c!='?') && (c!='+') ) || (c==182) ) { // ot tab char
		while ( ((!isalnum(c)) && (c!='!') && (c!='?') && (c!='+') && (c!='"' ) )|| (c==182) ) {
//				StampaDB ("pnome",pnome);					// Su finestra DOS di Debug 
			pnome++;							// toglie i caratteri NON Alfanumerici prima
			c = pnome[0];
		}
		lungh = strlen(pnome);
		c = pnome[lungh-1];
		// FR 231218
//		while ( ((!isalnum(c)) && (c!='!') && (c!='?') && (c!='+') ) || (c==182) ) { // ot tab char
		while ( ((!isalnum(c)) && (c!='!') && (c!='?') && (c!='+') && (c!='"' ) ) || (c==182) ) {
//				StampaDB ("pnome",pnome);		// Su finestra DOS di Debug 
			//pnome[lungh-1]='\0';
			lungh--;							// toglie i caratteri NON Alfanumerici dopo
			c=pnome[lungh-1];
		}
		pnome[lungh+1]='\0';
		strcpy(testo, pnome);
	}
//				StampaDB("RLTrimm---> Stringa elaborata1", pnome); // Su finestra DOS di Debug 
//				StampaDB("RLTrimm---> Stringa elaborata", testo);  // Su finestra DOS di Debug 
}

//OLD void UpperAlfaNum(unsigned char *testo)
void UpperAlfaNum(char *testo)
{
	int i;
	char c;

//				StampaDB("UpperAlfaNum---> Stringa da elaborare", testo); // Su finestra DOS di Debug 

	for(i=0 ; testo[i] ; i++){ 
		c = toupper(testo[i]);					// si converte in Maiuscolo i'esimo carattere
		if (isalnum(c)){
			testo[i] = c;			// e se è Alfanumerico lo si memorizza
		}else if ( (testo[i]<=32) && (testo[i])){
			testo[i] = 182 ; // '_';					// ... altrimenti lo si sostituisce con ' ¶' 
		}
	}

//				StampaDB("UpperAlfaNum---> Stringa elaborata", testo); // Su finestra DOS di Debug 

}


/*
int GetIntStr(char *Delim, char *sInput,  unsigned int order, char *sOut){
	int i=0;
	int OverLen=0;
	char cpIn[MAXSIZE], *pStringa;

// si cercano i token separati con >Delim<
// order indica  quale token tornare indietro
	sprintf(cpIn,"%s%s%s", Delim, sInput, Delim);
	//			StampaDB("---> cpIn", cpIn); // Su finestra DOS di Debug 
	pStringa = strtok(cpIn,Delim);

	while ((i<order) && (pStringa!=NULL)) { 
		sprintf(sOut,"%s",pStringa);
		//		StampaDB("---> Stringa da elaborare", pStringa); // Su finestra DOS di Debug 		
		pStringa = strtok(NULL, Delim);
		i++;
		// Se è già arrivata la fine della stringa, ma non si è raggiunto il token richiesto 
		if ((pStringa==NULL) && (i<order)) {			
			OverLen = i;						// indica quanti token sono stati individuati
		}
	}
	return OverLen;
}
*/

int GetIntStr(char *Delim, char *sInput,  unsigned int order, char *sOut){
	int i ;
	int OverLen=0;
	char cpIn[512], *pStringa;

    memset(cpIn, 0, 512) ;		// FR 3.75 - 17/05/23: added
	i = strlen(sInput) ;
	strncpy(cpIn,sInput, ((i>511)? 511:i));	// FR 3.75 - 17/05/23: modified
    char *inputstring=cpIn;
    
	i = 0 ;
    while( (i<order) && (0 != (pStringa = strsep(&inputstring, Delim))) ) {
        sprintf(sOut,"%s",pStringa);
        if (0 == *pStringa) {
            sOut[0] = '\0';
        }
        i++;
    }
    if (i> order) OverLen=i;
    
	return OverLen;
}

void LoadProdTab(void)
{
	int qta, nr_ss ;
	char NewRiga[MAXSIZE], Valore[NRMSIZE] ;
	char FileARCA[NRMSIZE] ; 		// path + nome del file ARCA
	FILE *farca ;
	
	sprintf(FileARCA, "%s\\ModelliARCA.txt", PathTK);		// File con le associazioni di cod. ARCA e cod.Schede
	CONVERTPATH(FileARCA) ;
	farca = fopen(FileARCA, "r");				// Apertura del file di associazioni ARCA
//	StampaDBn("errore", errno);					// Su finestra DOS di Debug 
	if (farca==NULL) {							// Se il file non esiste
		QtaLog++;
		sprintf(LogBuffer[QtaLog], "\r\n------ Collaudo eseguito con impostazioni di default -------");
		MsgWindow(LogBuffer[QtaLog]);
	} else {																													// si leggono i campi che descrivono i codici ARCA
// il file è già aperto !!!!!!
		while (!feof(farca)) {					// fino alla fine del file
// =================================================================
// costruzione della tabella Prodotti
			QtaArca++;
			loc_fgets(NewRiga, 256, farca);
			if (strlen(NewRiga)==0) break ;
#ifdef CBUG_
			sprintf(bufwindow,"read:<%s>", NewRiga);
			MsgWindow(bufwindow);
#endif
			// PRODOTTO: CODICE ARCA 
			qta = GetIntStr(";", NewRiga, 1, Valore);  
			if (qta!=0) {
				sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> cod.ARCA", QtaArca);
				MsgWindow(bufwindow);
			} else {
				UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				if (strlen(Valore) != LEN_ARCA) {
					sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> cod.ARCA", QtaArca);
					MsgWindow(bufwindow);
				}
				strcpy(Arca[QtaArca].cod, Valore);	// memorizza il codice ARCA
			}
			// MODELLO MTS
			qta = GetIntStr(";", NewRiga, 2, Valore);
			if (qta!=0) {
				sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> nome MTS", QtaArca);
				MsgWindow(bufwindow);
			} else {
				UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				if (strlen(Valore) != LEN_IDHW) {
					sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> nome MTS", QtaArca);
					MsgWindow(bufwindow);
				}
				strcpy(Arca[QtaArca].mts, Valore);		// memorizza il nome di MTS
			}
			// PRODOTTO: NR. di schede servizi
			qta = GetIntStr(";", NewRiga, 3, Valore);
			if (qta!=0) {
				sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> Nr.schedeSRV <%s>%d", QtaArca,NewRiga,qta);
				MsgWindow(bufwindow);
			} else {
				UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				// if (strlen(Valore) != LEN_IDHW) {
					// sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> Motherboard", QtaArca);
					// MsgWindow(bufwindow);
				// }
				Arca[QtaArca].nr_aSV = atoi(Valore) ;
			}
			
			// PRODOTTO: TIPO BOARD
			qta = GetIntStr(";", NewRiga, 4, Valore);
			if (qta!=0) {
				sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> Motherboard <%s>%d", QtaArca,NewRiga,qta);
				MsgWindow(bufwindow);
			} else {
				UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				if (strlen(Valore) != LEN_IDHW) {
					sprintf(bufwindow,"Errore LEN in >> ModelliARCA.txt << riga %d -> Motherboard <%s>%d", QtaArca,NewRiga,qta);
					MsgWindow(bufwindow);
				}
				strcpy(Arca[QtaArca].board, Valore);		// memorizza la Motherboard
			}
			// PRODOTTO: SCHEDE SERVIZI
			for (nr_ss=0;nr_ss<5; nr_ss++)
				strcpy(Arca[QtaArca].srvcard[nr_ss], "NA");		// inizializza  S.Servizi
				
			for (nr_ss=0;nr_ss<Arca[QtaArca].nr_aSV; nr_ss++){
				qta = GetIntStr(";", NewRiga, 5+nr_ss, Valore);
				if (qta!=0) {
					sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> S.Servizi", QtaArca);
					MsgWindow(bufwindow);
				} else {
					UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
					RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
					if ((strlen(Valore) != LEN_IDHW) && (strcmp(Valore,"NA")!=0)) {
						sprintf(bufwindow,"Errore in >> ModelliARCA.txt << riga %d -> S.Servizi", QtaArca);
						MsgWindow(bufwindow);
					}
					strcpy(Arca[QtaArca].srvcard[nr_ss], Valore);		// memorizza la S.Servizi
				}
			}
		}
// =================================================================
		fclose(farca);								// Chiusura del file di associazioni ARCA
	}
}

void LoadTabPINAssociative(void)
{
	int qta ; // , nr_ss ;
	char NewRiga[MAXSIZE], Valore[NRMSIZE] ;
	char FilePIN[NRMSIZE] ; 		// path + nome del file PIN
	FILE *fpin ;
	sprintf(FilePIN, "%s\\Ass_PIN.txt", mRoot);		// File con le associazioni
	CONVERTPATH(FilePIN) ;
	fpin = fopen(FilePIN, "r");				// Apertura del file di associazioni ARCA
//	StampaDBn("errore", errno);					// Su finestra DOS di Debug 
	if (fpin==NULL) {							// Se il file non esiste
		QtaLog++;
		sprintf(LogBuffer[QtaLog], "\r\n------ Impossibile aprire Ass_PIN.txt -------");
		MsgWindow(LogBuffer[QtaLog]);
	} else {																													// si leggono i campi che descrivono i codici ARCA
// il file è già aperto !!!!!!
		while (!feof(fpin)) {					// fino alla fine del file
// =================================================================
// costruzione della tabella Prodotti
			QtaRow++;
			loc_fgets(NewRiga, 256, fpin);
			if (strlen(NewRiga)==0) break ;
#ifdef CBUG_
			sprintf(bufwindow,"read:<%s>", NewRiga);
			MsgWindow(bufwindow);
#endif
			// SN MTS 
			qta = GetIntStr(";", NewRiga, 1, Valore);  
			if (qta!=0) {
				sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> SN MTS", QtaRow);
				MsgWindow(bufwindow);
			} else {
				UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				if (strlen(Valore) != LEN_NUM) {
					sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> SN MTS", QtaRow);
					MsgWindow(bufwindow);
				}
				strcpy(TablePIN[QtaRow].snmts, Valore);	// memorizza il SN MTS
			}
			// EXT_NAME
			qta = GetIntStr(";", NewRiga, 2, Valore);
			if (qta!=0) {
				sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> Nome Esterno MTS", QtaRow);
				MsgWindow(bufwindow);
			} else {
				UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				if (strlen(Valore) < 0) {
					sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> Nome Esterno MTS", QtaRow);
					MsgWindow(bufwindow);
				}
				strcpy(TablePIN[QtaRow].extname, Valore);		// memorizza il nome Esterno MTS
			}
			// ICCID
			qta = GetIntStr(";", NewRiga, 3, Valore);
			if (qta!=0) {
				TablePIN[QtaRow].ICCID[0] = '\0';
				//sprintf(TablePIN[QtaRow].ICCID, '\0');
				//sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> ICCID", QtaRow);
				//MsgWindow(bufwindow);
			} else {
				UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				if (strlen(Valore)>0){
					if (strlen(Valore) != LEN_ICCID) {
						sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> ICCID len[%ld]{%s}", QtaRow,strlen(Valore),Valore);
						MsgWindow(bufwindow);
					}
					strcpy(TablePIN[QtaRow].ICCID, Valore);		// memorizza ICCID
				}else{
					TablePIN[QtaRow].ICCID[0] = '\0';
					//sprintf(TablePIN[QtaRow].ICCID, '\0');
				}
			}
			// PIN
			qta = GetIntStr(";", NewRiga, 4, Valore);
			if (qta!=0) {
				TablePIN[QtaRow].PIN[0] = '\0';
				//sprintf(TablePIN[QtaRow].PIN, '\0');
				//sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> PIN", QtaRow);
				//MsgWindow(bufwindow);
			} else {
				UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				if (strlen(Valore)>0){
					if (strlen(Valore) != LEN_IDHW) {
						sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> PIN len[%ld]{%s}", QtaRow,strlen(Valore),Valore);
						MsgWindow(bufwindow);
					}
					strcpy(TablePIN[QtaRow].PIN, Valore);		// memorizza PIN
				}else{
					TablePIN[QtaRow].PIN[0] = '\0' ;
					//sprintf(TablePIN[QtaRow].PIN, '\0');
				}
			}
			// Parametro 185
			qta = GetIntStr(";", NewRiga, 5, Valore);
			if (qta!=0) {
				TablePIN[QtaRow].centoottantacinquepar[0] = '\0';
				//sprintf(TablePIN[QtaRow].centoottantacinquepar, '\0');
				//sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> 185", QtaRow);
				//MsgWindow(bufwindow);
			} else {
				//UpperAlfaNum(Valore);				// si converte tutto in maiuscolo
				RLTrimm(Valore);					// e si tolgono eventuali carateri non alfanumerici prima e dopo
				/*
				if (strlen(Valore) != LEN_IDHW) {
					sprintf(bufwindow,"Errore in >> Ass_PIN.txt << riga %d -> 185", QtaRow);
					MsgWindow(bufwindow);
				}*/
				strcpy(TablePIN[QtaRow].centoottantacinquepar, Valore);		// memorizza PIN
			}
			
		}
// =================================================================
		fclose(fpin);								// Chiusura del file di associazioni PIN
	}
}


char * loc_fgets(char * a1, int nn, FILE * abc)
{
char *aa ;
int ii, i ;
	
	aa = fgets(a1, nn, abc); 	
	
	if (aa==NULL) a1[0]='\0';
		
	ii = strlen(a1) ;
// added FR 3.75 - 17/05/23
	for(i=0;i<ii;i++){
		if ((a1[i]<32) || (a1[i]>126)) {
			a1[i] = '\0' ;
			break ;
		}
	}
	
	if (a1[ii-1]==0xa){
		a1[ii-1]='\0' ;
	}
	
	return(aa) ;
}



#ifdef CHECK_TESTTIME
void LABEL_STEP(int _N, char * _T){
char tmpstr[MAXSIZE] ;
	
	sprintf(tmpstr,"nr.%d, %s", _N, _T) ;
	MsgFile(0, "tempi_collaudo.txt", tmpstr) ;
 	OuputText((_N+100), _T, 0, 0, 0, 0 ) ;

}

void COLOR_STEP(int _N, int _C){
char tmpstr[MAXSIZE] ;

	sprintf(tmpstr,"nr.%d, ", _N) ;
	switch(_C){
		case C_RED :
		strcat(tmpstr, "Rosso" ) ; break ;
		case C_GREEN:
		strcat(tmpstr, "Verde") ; break ;
		case C_YELLOW:
		strcat(tmpstr, "Giallo") ; break ;
		case C_WHITE:
		strcat(tmpstr, "Bianco" ) ; break ;
		case C_BLACK:
		strcat(tmpstr, "Nero" ) ; break ;
		case C_BLUE:
		strcat(tmpstr, "Blu") ; break ;
		case C_CYAN:
		strcat(tmpstr, "Cyan" ) ; break ;
		case C_MAGENTA:
		strcat(tmpstr, "Magenta") ; break ;
		case C_GREY:
		strcat(tmpstr, "Grigio" ) ; break ;
		case C_DEFAULT:
		strcat(tmpstr, "Default" ) ; break ;
		default:
		strcat(tmpstr, "No code" ) ; break ;		
	}
	MsgFile(0, "tempi_collaudo.txt", tmpstr) ;
		
	OuputText((_N+100), "", _C, 0, 0, 0 ) ;
}

unsigned int MSGBOXCALL(char * text, char * label, int ntasti, char * caption1, char * caption2, char * buf)
{
unsigned int ii ;
char tmpstr[MAXSIZE] ;
int i;

	strcpy( tmpstr, "MSGBOXwait: " ) ;
	strcat( tmpstr, text ) ;
	MsgFile(0, "tempi_collaudo.txt", tmpstr ) ;
	for(i=0;i<MAXSIZE;i++) buf[i]='\0';
	ii = MsgBox(text, label, ntasti, caption1, caption2, buf) ;
	MsgFile(0, "tempi_collaudo.txt", "MSGBOXend") ;
	
	return(ii) ;
}

unsigned int INPUTBOXCALL(char * text, char * label, int ntasti, char * caption1, char * caption2, char * buf)
{
unsigned int ii ;
char tmpstr[MAXSIZE] ;

	strcpy( tmpstr, "INPUTBOXwait: " ) ;
	strcat( tmpstr, text ) ;
	MsgFile(0, "tempi_collaudo.txt", tmpstr ) ;
	ii = InputBox(text, label, ntasti, caption1, caption2, buf) ;
	MsgFile(0, "tempi_collaudo.txt", "INPUTBOXend") ;
	
	return(ii) ;
	
}

#endif // #ifdef CHECK_TESTTIME

void RemoveComment(char *testo) // Rimuove, se esiste, il commento ';'
{
	char *pcomment, *pcmarks ;
	
	pcomment = strrchr(testo, ';') ;
	if (pcomment) {
		pcmarks = strrchr(pcomment, '"') ;
		if (!pcmarks){
			pcomment[0] = '\0' ;
		}
	}
}

void WriteAbbin(char * buf)
{
	char AbbinFile[MAXSIZE] ;		//  new: "<mts>_abbYY_<terzista>.csv" old "<mts>_abb<date>.csv"
	char AbbinFileblk[MAXSIZE] ;
	char anno[4];
	int r;
	//char data[20];
	FILE *fabbn ;
	FILE *fabbnblk;
	fabbnblk = tmpfile ();
	srand(time(0)); /* n is random number in range of 0 - 1 */
	
	//	GetDate(data);
	GetYear(anno);  
	sprintf(AbbinFile, "%s\\Logs\\%s_abb20%s.txt", mRoot, TkIni.mName, anno); //hostname //TestSet.Terzista);	// File di abbinamenti
	CONVERTPATH(AbbinFile);
	PrintDB(AbbinFile);
	sprintf(AbbinFileblk, "%s\\Logs\\%s_abb20%s.blk", mRoot, TkIni.mName, anno);
	CONVERTPATH(AbbinFileblk);
	PrintDB(AbbinFileblk);
	while (1) {
   		fabbnblk = fopen(AbbinFileblk, "wx");
   		if ( fabbnblk!=NULL ) break;
		PrintDB("Attesa permessi di scrittura File Abbinamenti\n") ;
		r=100+(rand() % 100) ; 
		Delay(r);
	}
	/*
	do{
		fabbnblk = fopen(AbbinFileblk, "wx");
		PrintDB("Attesa permessi di scrittura\n");
	}while (fabbnblk==NULL);
	*/
	fabbn = fopen(AbbinFile, "r");
		if (fabbn==NULL) {							// Se non esiste già il file degli abbinamenti
			fabbn = fopen(AbbinFile, "a");
			// Crea e salva intestazione nel file degli abbinamenti
			fprintf(fabbn, "SN_MTS;CODTEST;SN_MOTH;SN_SUB1;SN_SUB2;SN_SUB3;SN_SUB4;SN_SUB5;LOTTO;FORNITORE\r\n");
		}
	fclose(fabbn);
	fabbn = fopen(AbbinFile, "a+");
	if ( AbbinFile==NULL ) {	
		PrintDB("Impossibile Scrivere file Abbinamenti\n");
		call_exit(YES, "Impossibile Scrivere file Abbinamenti");
	}
	fprintf(fabbn, "%s\r\n", buf) ;	
	fclose(fabbn);
	fclose(fabbnblk);
	unlink(AbbinFileblk) ;
}

void WriteSIMDATA(char * buf)
{
	char SIMDataFile[MAXSIZE] ;		//  new: "<mts>_abbYY_<terzista>.csv" old "<mts>_abb<date>.csv"
	char SIMDataFileblk[MAXSIZE] ;
	char anno[4];
	int r;
	//char data[20];
	FILE *fSIMData ;
	FILE *fSIMDatablk;
	fSIMDatablk = tmpfile ();
	srand(time(0)); /* n is random number in range of 0 - 1 */
	
	//	GetDate(data);
	GetYear(anno);  
	sprintf(SIMDataFile, "%s\\Logs\\%s_SIMDATA20%s.txt", mRoot, TkIni.mName, anno); //hostname //TestSet.Terzista);	// File di abbinamenti
	CONVERTPATH(SIMDataFile);
	PrintDB(SIMDataFile);
	sprintf(SIMDataFileblk, "%s\\Logs\\%s_SIMDATA20%s.blk", mRoot, TkIni.mName, anno);
	CONVERTPATH(SIMDataFileblk);
	PrintDB(SIMDataFileblk);
	while (1) {
   		fSIMDatablk = fopen(SIMDataFileblk, "wx");
   		if ( fSIMDatablk!=NULL ) break;
		PrintDB("Attesa permessi di scrittura File SIM\n") ;
		r=100+(rand() % 100) ; 
		Delay(r);
	}
	/*do{
		fSIMDatablk = fopen(SIMDataFileblk, "wx");
		PrintDB("Attesa permessi di scrittura\n") ;
	}while (fSIMDatablk==NULL);
	*/
	PrintDB("Scrittura File SIM\n");
	fSIMData = fopen(SIMDataFile, "r");
	if ( fSIMData==NULL ) {							// Se non esiste già il file SIM
		fSIMData = fopen(SIMDataFile, "w");
		// Crea e salva intestazione nel file SIM
		fprintf(fSIMData, "SN_MTS;ICCID;N_TELEFONO\r\n");
		PrintDB("Creato file SIM\n");
	}
	fclose(fSIMData);
	PrintDB("Scrittura File SIM2\n");
	fSIMData = fopen(SIMDataFile, "a+");
	if ( fSIMData==NULL ) {	
		PrintDB("Impossibile Scrivere file SIM\n");
		call_exit(YES, "Impossibile Scrivere file SIM");
	}
	fprintf(fSIMData, "%s\r\n", buf) ;	
	fclose(fSIMData);
	fclose(fSIMDatablk);
	unlink(SIMDataFileblk) ;
	PrintDB("Cancellazione Blocco File SIM\n");
}

#include <stdio.h>
#include <stdlib.h>

int FileCompare(char * file1, char * file2, unsigned int * same) 
{
  FILE *fp1, *fp2;
  char ch1, ch2;
  unsigned long l;

 /* open first file */
  if((fp1 = fopen(file1, "rb"))==NULL) {
	sprintf(Bmom, "Impossibile Aprire: %s\n",file1);
	PrintDB(Bmom);
	call_exit(YES, Bmom);
  }

  /* open second file */
  if((fp2 = fopen(file2, "rb"))==NULL) {
   	sprintf(Bmom, "Impossibile Aprire: %s\n",file2);
	PrintDB(Bmom);
	call_exit(YES, Bmom);
  }

  l = 0;
  *same = 1;
  /* compare the files */
  while(!feof(fp1)) {
    ch1 = fgetc(fp1);
    if(ferror(fp1)) {
      sprintf(Bmom, "Errore durante la lettura del file: %s\n",file1);
	  PrintDB(Bmom);
	  call_exit(YES, Bmom);
    }
    ch2 = fgetc(fp2);
    if(ferror(fp2)) {
      sprintf(Bmom, "Errore durante la lettura del file: %s\n",file2);
	  PrintDB(Bmom);
	  call_exit(YES, Bmom);
    }
    if(ch1 != ch2) {
	  sprintf(Bmom, "I file differiscono al numero di byte %lu", l);
	  PrintDB(Bmom);
      *same = 0;
	  //call_exit(YES, Bmom);
      break;
    }
    l++;
  }
  if(same)
	  PrintDB("I file sono uguali\n");

  if(fclose( fp1 ) == EOF) {
	sprintf(Bmom, "Errore durante la chiusura del file: %s\n",file1);
	PrintDB(Bmom);
	call_exit(YES, Bmom);
  }

  if(fclose( fp2 ) == EOF) {
    sprintf(Bmom, "Errore durante la chiusura del file: %s\n",file2);
	PrintDB(Bmom);
	call_exit(YES, Bmom);
  }

  return 0;
}





