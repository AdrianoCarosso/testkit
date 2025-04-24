/*
	versione 14: 
		-modiciche per passaggio da parser a mingw
	versione 15: 
		-implementazione comandi per CANBUS
	versione 16: 
		-Granello rinomina funzioni
	....		
	....		
	....		
	versione 20: 
		-gestione IMEI
		-inserimento nell'SVN repository con nome: vbInterface.c
	
	versione 21 giuseppe:
		-aggiunto inputbox
		-aggiunto GetWorkSpace
	versione 22 giuseppe:
	  -aggiunto OuputText
	  -modificato MsgWindows aggiungendo ritorno a capo di default (\n)
	versione 23 giuseppe:
		-aggiunto ProgressBar
		-modificato msgfile aggiunto data

			 RICHIESTA DI GRANELLO (per Renato):
		-aggiungere M_SaveSMach(NomeFile)
				Funzione che legge la Macchina a Stati dell'MTS e 
				la salva in <NomeFile> (che viene fornito completo di path)
		-aggiungere M_RestoreSMach(NomeFile)
				Funzione che ripristina la Macchina a Stati dell'MTS e 
				leggendola da <NomeFile> (che viene fornito completo di path)

	versione 24 06/11/09 Rovera:
		Aggiunto comando "M_DelPar"
	AGGIUNTI FILES AL REPOSTITORY SVN. L'HISTORY E' PERTANTO CONSULTABILE MEDIANTE SVN.
	aggiunto comando M_GetDirect
	.....
	.....
	.....
	.....
	.....
	.....
	.....
	.....

	versione 25 25/11/09 Rovera: Tutte le risposte da VB hanno "A:" come inizio

	versione 26 08/06/10 Rovera: TKERROR valorizzato con risposta errore da VB
	
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "vbInterface.h"
#include "testkit.h"
#include <ctype.h>

#undef DBG_MAP
//#define DBG_MAP

static char debugBf[2048];

unsigned int TKERROR;		//assume valori != 0 se errore durante comando

// Write a message direct to the console (will not be redirected).

static void directToConsole(char out[]) { // _FR_
#ifdef CBUG
	FILE* fp;
	//CHAR     szInput[1024];


	// Open the console. By doing this, you can send output directly to
	// the console that will not be redirected.
	fp = NULL ;
	
	fp = fopen("/dev/tty", "w");
	if (!fp) {
		printf("Error opening child console\n");
		fflush(NULL);
	} else {

		fprintf(fp, "%s", out);
		/*
		fprintf(fp,"This data is being printed directly to the\n");
		fprintf(fp,"console and will not be redirected.\n\n");
		fprintf(fp,"Since the standard input and output have been\n");
		fprintf(fp,"redirected data sent to and from those handles\n");
		fprintf(fp,"will be redirected.\n\n");
		fprintf(fp,"To send data to the std input of this process.\n");
		fprintf(fp,"Click on the console window of the parent process\n");
		fprintf(fp,"(redirect), and enter data from it's console\n\n");
		fprintf(fp,"To exit this process send the string 'exit' to\n");
		fprintf(fp,"it's standard input\n");
		 */
		fflush(fp);
		fclose(fp);
	}
#endif	
}




#define TRIGGER_STRING "***>"

/* 	gestione errori: tendenzialmente i comandi rispondono sempre con una stringa che, 
		in caso di errore assume il valore:
			"+++ERROR=dddd "
			dove dddd rappresenta il codice  di errore verificatosi.
		Altrimenti la stringa trasporta il direttamente il valore da testituire.
 */
#define ERROR_STRING_START "+++ERROR="
static char readBf[1024], locbuf[1024] ;
static int maxRetryOnError = 0;

static int getanswer(char * usrbuf)
{
    //int i;
	char *p;
	char mybf[8];

	usrbuf[0]='\0';
	
	fflush(stdout);
	fflush(stdin);
	
//	if (scanf("%s", readBf)==EOF) PrgExit("Chiusura") ;
	scanf("%s", readBf) ;

	sprintf(debugBf, " >>> Pre-answer=%s\n", readBf);
	directToConsole(debugBf);
        p = strstr(readBf, ERROR_STRING_START);
        sprintf(debugBf, " >>> MyPointer=%s\n", p);
        if (leveldebug & 0x1) // Added from 2.13 (25/05/23) same name as Gdata into MTScu
        	directToConsole(debugBf);
        if ( p != NULL) {
		//error from VB command: decode error code
		memset(mybf, 0, sizeof (mybf)); //clear buffer
		strncpy(mybf, &p[strlen(ERROR_STRING_START)], 4); //copia 4 cifre codice errore in mybf
                sprintf(debugBf, " >>> MyBuffer=%s\n", mybf);
                directToConsole(debugBf);
		TKERROR = atoi(mybf);
                
		//errore non puo' essere 0!!!!
		if (!TKERROR) TKERROR=1 ;
//			return 1;
//		else
		return TKERROR ;
	}

	memcpy(usrbuf, &readBf[2], strlen(readBf)-1) ; // also '\0'
	return 0;
	
}

void retryOnError(int retryNum) {
	maxRetryOnError = retryNum;
}

#define FALSE 0
#define TRUE 1

struct singlebit {
	int port;
	unsigned long long bitmask;
};

unsigned int readedData[8]; //bufferizzazione dati letti

unsigned int outData[8]; //bufferizzazione dati per output (dati)
unsigned int outMask[8]; //bufferizzazione dati per output (mask)

//mappatura OUTPUT DIGITALI EXTENDER
struct singlebit extenderOutBitMap[64] = {
	{0, 0x1 << 0},
	{0, 0x1 << 1},
	{0, 0x1 << 2},
	{0, 0x1 << 3},
	{0, 0x1 << 4},
	{0, 0x1 << 5},
	{0, 0x1 << 6},
	{0, 0x1 << 7},
	{0, 0x1 << 8},
	{0, 0x1 << 9},
	{0, 0x1 << 10},
	{0, 0x1 << 11},
	{0, 0x1 << 12},
	{0, 0x1 << 13},
	{0, 0x1 << 14},
	{0, 0x1 << 15},
	{0, 0x1 << 16},
	{0, 0x1 << 17},
	{0, 0x1 << 18},
	{0, 0x1 << 19},
	{0, 0x1 << 20},
	{0, 0x1 << 21},
	{0, 0x1 << 22},
	{0, 0x1 << 23},
	{0, 0x1 << 24},
	{0, 0x1 << 25},
	{0, 0x1 << 26},
	{0, 0x1 << 27},
	{0, 0x1 << 28},
	{0, 0x1 << 29},
	{0, 0x1 << 30},
	{0, 0x1 << 31},
	{0, 0x1L << 32},
	{0, 0x1L << 33},
	{0, 0x1L << 34},
	{0, 0x1L << 35},
	{0, 0x1L << 36},
	{0, 0x1L << 37},
	{0, 0x1L << 38},
	{0, 0x1L << 39},
	{0, 0x1L << 40},
	{0, 0x1L << 41},
	{0, 0x1L << 42},
	{0, 0x1L << 43},
	{0, 0x1L << 44},
	{0, 0x1L << 45},
	{0, 0x1L << 46},
	{0, 0x1L << 47},
	{0, 0x1L << 48},
	{0, 0x1L << 49},
	{0, 0x1L << 50},
	{0, 0x1L << 51},
	{0, 0x1L << 52},
	{0, 0x1L << 53},
	{0, 0x1L << 54},
	{0, 0x1L << 55},
	{0, 0x1L << 56},
	{0, 0x1L << 57},
	{0, 0x1L << 58},
	{0, 0x1L << 59},
	{0, 0x1L << 60},
	{0, 0x1L << 61},
	{0, 0x1L << 62},
	{0, 0x1L << 63}
};

struct singlebit extenderInPUPDFLBitMap[64] = {
	{0, 0x1 << 0},
	{0, 0x1 << 1},
	{0, 0x1 << 2},
	{0, 0x1 << 3},
	{0, 0x1 << 4},
	{0, 0x1 << 5},
	{0, 0x1 << 6},
	{0, 0x1 << 7},
	{0, 0x1 << 8},
	{0, 0x1 << 9},
	{0, 0x1 << 10},
	{0, 0x1 << 11},
	{0, 0x1 << 12},
	{0, 0x1 << 13},
	{0, 0x1 << 14},
	{0, 0x1 << 15},
	{0, 0x1 << 16},
	{0, 0x1 << 17},
	{0, 0x1 << 18},
	{0, 0x1 << 19},
	{0, 0x1 << 20},
	{0, 0x1 << 21},
	{0, 0x1 << 22},
	{0, 0x1 << 23},
	{0, 0x1 << 24},
	{0, 0x1 << 25},
	{0, 0x1 << 26},
	{0, 0x1 << 27},
	{0, 0x1 << 28},
	{0, 0x1 << 29},
	{0, 0x1 << 30},
	{0, 0x1 << 31},
	{0, 0x1L << 32},
	{0, 0x1L << 33},
	{0, 0x1L << 34},
	{0, 0x1L << 35},
	{0, 0x1L << 36},
	{0, 0x1L << 37},
	{0, 0x1L << 38},
	{0, 0x1L << 39},
	{0, 0x1L << 40},
	{0, 0x1L << 41},
	{0, 0x1L << 42},
	{0, 0x1L << 43},
	{0, 0x1L << 44},
	{0, 0x1L << 45},
	{0, 0x1L << 46},
	{0, 0x1L << 47},
	{0, 0x1L << 48},
	{0, 0x1L << 49},
	{0, 0x1L << 50},
	{0, 0x1L << 51},
	{0, 0x1L << 52},
	{0, 0x1L << 53},
	{0, 0x1L << 54},
	{0, 0x1L << 55},
	{0, 0x1L << 56},
	{0, 0x1L << 57},
	{0, 0x1L << 58},
	{0, 0x1L << 59},
	{0, 0x1L << 60},
	{0, 0x1L << 61},
	{0, 0x1L << 62},
	{0, 0x1L << 63}
};

//mappatura OUTPUT DIGITALI DISPOSITIVO SOTTO TEST
struct singlebit digitalOutBitMap[64] = {
	{0, 0x1 << 0},
	{0, 0x1 << 1},
	{0, 0x1 << 2},
	{0, 0x1 << 3},
	{0, 0x1 << 4},
	{0, 0x1 << 5},
	{0, 0x1 << 6},
	{0, 0x1 << 7},
	{0, 0x1 << 8},
	{0, 0x1 << 9},
	{0, 0x1 << 10},
	{0, 0x1 << 11},
	{0, 0x1 << 12},
	{0, 0x1 << 13},
	{0, 0x1 << 14},
	{0, 0x1 << 15},
	{0, 0x1 << 16},
	{0, 0x1 << 17},
	{0, 0x1 << 18},
	{0, 0x1 << 19},
	{0, 0x1 << 20},
	{0, 0x1 << 21},
	{0, 0x1 << 22},
	{0, 0x1 << 23},
	{0, 0x1 << 24},
	{0, 0x1 << 25},
	{0, 0x1 << 26},
	{0, 0x1 << 27},
	{0, 0x1 << 28},
	{0, 0x1 << 29},
	{0, 0x1 << 30},
	{0, 0x1 << 31},
	{0, 0x1L << 32},
	{0, 0x1L << 33},
	{0, 0x1L << 34},
	{0, 0x1L << 35},
	{0, 0x1L << 36},
	{0, 0x1L << 37},
	{0, 0x1L << 38},
	{0, 0x1L << 39},
	{0, 0x1L << 40},
	{0, 0x1L << 41},
	{0, 0x1L << 42},
	{0, 0x1L << 43},
	{0, 0x1L << 44},
	{0, 0x1L << 45},
	{0, 0x1L << 46},
	{0, 0x1L << 47},
	{0, 0x1L << 48},
	{0, 0x1L << 49},
	{0, 0x1L << 50},
	{0, 0x1L << 51},
	{0, 0x1L << 52},
	{0, 0x1L << 53},
	{0, 0x1L << 54},
	{0, 0x1L << 55},
	{0, 0x1L << 56},
	{0, 0x1L << 57},
	{0, 0x1L << 58},
	{0, 0x1L << 59},
	{0, 0x1L << 60},
	{0, 0x1L << 61},
	{0, 0x1L << 62},
	{0, 0x1L << 63}
};

//mappatura INPUT DIGITALI DISPOSITIVO SOTTO TEST
struct singlebit digitalInBitMap[64] = {
	{0, 0x1 << 0},
	{0, 0x1 << 1},
	{0, 0x1 << 2},
	{0, 0x1 << 3},
	{0, 0x1 << 4},
	{0, 0x1 << 5},
	{0, 0x1 << 6},
	{0, 0x1 << 7},
	{0, 0x1 << 8},
	{0, 0x1 << 9},
	{0, 0x1 << 10},
	{0, 0x1 << 11},
	{0, 0x1 << 12},
	{0, 0x1 << 13},
	{0, 0x1 << 14},
	{0, 0x1 << 15},
	{0, 0x1 << 16},
	{0, 0x1 << 17},
	{0, 0x1 << 18},
	{0, 0x1 << 19},
	{0, 0x1 << 20},
	{0, 0x1 << 21},
	{0, 0x1 << 22},
	{0, 0x1 << 23},
	{0, 0x1 << 24},
	{0, 0x1 << 25},
	{0, 0x1 << 26},
	{0, 0x1 << 27},
	{0, 0x1 << 28},
	{0, 0x1 << 29},
	{0, 0x1 << 30},
	{0, 0x1 << 31},
	{0, 0x1L << 32},
	{0, 0x1L << 33},
	{0, 0x1L << 34},
	{0, 0x1L << 35},
	{0, 0x1L << 36},
	{0, 0x1L << 37},
	{0, 0x1L << 38},
	{0, 0x1L << 39},
	{0, 0x1L << 40},
	{0, 0x1L << 41},
	{0, 0x1L << 42},
	{0, 0x1L << 43},
	{0, 0x1L << 44},
	{0, 0x1L << 45},
	{0, 0x1L << 46},
	{0, 0x1L << 47},
	{0, 0x1L << 48},
	{0, 0x1L << 49},
	{0, 0x1L << 50},
	{0, 0x1L << 51},
	{0, 0x1L << 52},
	{0, 0x1L << 53},
	{0, 0x1L << 54},
	{0, 0x1L << 55},
	{0, 0x1L << 56},
	{0, 0x1L << 57},
	{0, 0x1L << 58},
	{0, 0x1L << 59},
	{0, 0x1L << 60},
	{0, 0x1L << 61},
	{0, 0x1L << 62},
	{0, 0x1L << 63}
};

//mappatura INPUT DIGITALI EXTENDER
struct singlebit extenderInBitMap[64] = {
	{0, 0x1 << 0},
	{0, 0x1 << 1},
	{0, 0x1 << 2},
	{0, 0x1 << 3},
	{0, 0x1 << 4},
	{0, 0x1 << 5},
	{0, 0x1 << 6},
	{0, 0x1 << 7},
	{0, 0x1 << 8},
	{0, 0x1 << 9},
	{0, 0x1 << 10},
	{0, 0x1 << 11},
	{0, 0x1 << 12},
	{0, 0x1 << 13},
	{0, 0x1 << 14},
	{0, 0x1 << 15},
	{0, 0x1 << 16},
	{0, 0x1 << 17},
	{0, 0x1 << 18},
	{0, 0x1 << 19},
	{0, 0x1 << 20},
	{0, 0x1 << 21},
	{0, 0x1 << 22},
	{0, 0x1 << 23},
	{0, 0x1 << 24},
	{0, 0x1 << 25},
	{0, 0x1 << 26},
	{0, 0x1 << 27},
	{0, 0x1 << 28},
	{0, 0x1 << 29},
	{0, 0x1 << 30},
	{0, 0x1 << 31},
		{0, 0x1L << 32},
	{0, 0x1L << 33},
	{0, 0x1L << 34},
	{0, 0x1L << 35},
	{0, 0x1L << 36},
	{0, 0x1L << 37},
	{0, 0x1L << 38},
	{0, 0x1L << 39},
	{0, 0x1L << 40},
	{0, 0x1L << 41},
	{0, 0x1L << 42},
	{0, 0x1L << 43},
	{0, 0x1L << 44},
	{0, 0x1L << 45},
	{0, 0x1L << 46},
	{0, 0x1L << 47},
	{0, 0x1L << 48},
	{0, 0x1L << 49},
	{0, 0x1L << 50},
	{0, 0x1L << 51},
	{0, 0x1L << 52},
	{0, 0x1L << 53},
	{0, 0x1L << 54},
	{0, 0x1L << 55},
	{0, 0x1L << 56},
	{0, 0x1L << 57},
	{0, 0x1L << 58},
	{0, 0x1L << 59},
	{0, 0x1L << 60},
	{0, 0x1L << 61},
	{0, 0x1L << 62},
	{0, 0x1L << 63}
};


//mappatura INPUT DIGITALI VIRTUALI
struct singlebit virtualInBitMap[64] = {
	{0, 0x1 << 0},
	{0, 0x1 << 1},
	{0, 0x1 << 2},
	{0, 0x1 << 3},
	{0, 0x1 << 4},
	{0, 0x1 << 5},
	{0, 0x1 << 6},
	{0, 0x1 << 7},
	{0, 0x1 << 8},
	{0, 0x1 << 9},
	{0, 0x1 << 10},
	{0, 0x1 << 11},
	{0, 0x1 << 12},
	{0, 0x1 << 13},
	{0, 0x1 << 14},
	{0, 0x1 << 15},
	{0, 0x1 << 16},
	{0, 0x1 << 17},
	{0, 0x1 << 18},
	{0, 0x1 << 19},
	{0, 0x1 << 20},
	{0, 0x1 << 21},
	{0, 0x1 << 22},
	{0, 0x1 << 23},
	{0, 0x1 << 24},
	{0, 0x1 << 25},
	{0, 0x1 << 26},
	{0, 0x1 << 27},
	{0, 0x1 << 28},
	{0, 0x1 << 29},
	{0, 0x1 << 30},
	{0, 0x1 << 31},
	{0, 0x1L << 32},
	{0, 0x1L << 33},
	{0, 0x1L << 34},
	{0, 0x1L << 35},
	{0, 0x1L << 36},
	{0, 0x1L << 37},
	{0, 0x1L << 38},
	{0, 0x1L << 39},
	{0, 0x1L << 40},
	{0, 0x1L << 41},
	{0, 0x1L << 42},
	{0, 0x1L << 43},
	{0, 0x1L << 44},
	{0, 0x1L << 45},
	{0, 0x1L << 46},
	{0, 0x1L << 47},
	{0, 0x1L << 48},
	{0, 0x1L << 49},
	{0, 0x1L << 50},
	{0, 0x1L << 51},
	{0, 0x1L << 52},
	{0, 0x1L << 53},
	{0, 0x1L << 54},
	{0, 0x1L << 55},
	{0, 0x1L << 56},
	{0, 0x1L << 57},
	{0, 0x1L << 58},
	{0, 0x1L << 59},
	{0, 0x1L << 60},
	{0, 0x1L << 61},
	{0, 0x1L << 62},
	{0, 0x1L << 63}
};

static unsigned int doOutCmd(struct singlebit bitArr[], unsigned long long m, unsigned long long v) 
{
	//unsigned int anwer;
	int d=0; //for DEBUG
	//d=1;  //for DEBUG
	unsigned long long im, iv; //input values
	//unsigned int om,ov;		//output (calculated) values
	
	unsigned int bitcnt;
	struct singlebit thisbit;

	im = iv = 0;
	//om=ov=0;

	outData[0] = outData[1] = outData[2] = outData[3] = outData[4] = outData[5] = outData[6] = outData[7] = 0;
	outMask[0] = outMask[1] = outMask[2] = outMask[3] = outMask[4] = outMask[5] = outMask[6] = outMask[7] = 0;

	//im=strtoul(pm, NULL, 0);	//read mask
	//iv=strtoul(pv, NULL, 0);	//read value
	im = m; //read mask (arg 1)
	iv = v; //read value (arg 2)
	if (d){
		sprintf(debugBf, "\nRead Mask:%llx\n",im);
		directToConsole(debugBf);
		sprintf(debugBf, "\nRead Value:%llx\n",iv);
		directToConsole(debugBf);
	}    
	for (bitcnt = 0; bitcnt < 64; bitcnt++, iv = iv >> 1, im = im >> 1) {
		if (d){
			sprintf(debugBf, "\nRead bit:%llx\n",im);
			directToConsole(debugBf);
		}
		if (im & 1) {
			//bit da impostare
			thisbit = bitArr[bitcnt]; //info mappatura bit corrente
			if (d){
				sprintf(debugBf, "\n bitcnt:%d\n",bitcnt);
				directToConsole(debugBf);
				sprintf(debugBf, "\n Port:%x\n",thisbit.port);
				directToConsole(debugBf);
				sprintf(debugBf, "\n Mask:%llx\n",thisbit.bitmask);
				directToConsole(debugBf);
				sprintf(debugBf, "\nRead valuebit:%llx\n",iv);
				directToConsole(debugBf);
			}
			outMask[thisbit.port] |= thisbit.bitmask;
			if (iv & 1)
				outData[thisbit.port] |= thisbit.bitmask;
		}
	}
	return 0;
}



static unsigned int doInCmd(struct singlebit bitArr[]) 
{
	unsigned int remappedIn;

	unsigned long long  iv, im; //input values, mask

	unsigned int bitcnt;
	struct singlebit thisbit;

	remappedIn = 0;
	im = 1; //mask bit
	for (bitcnt = 0; bitcnt < 64; bitcnt++, im = im << 1) {
		thisbit = bitArr[bitcnt]; //info mappatura bit corrente
		iv = readedData[thisbit.port] & thisbit.bitmask; //rileva valore effettivo letto da port fisico
		if (iv != 0) {
			//bit da settare in posizione richiesta
			remappedIn |= im;
		}
	}

	return remappedIn;
}

#ifdef DBG_MAP
static void displayMap(struct singlebit bitarr[], char * outbf)
{
	unsigned int bitcnt;
	struct singlebit thisbit;

	sprintf(outbf, "--DISPLAYMAP--\n");
	for (bitcnt = 0; bitcnt < 64; bitcnt++) {
		//bit corrente
		thisbit = bitarr[bitcnt]; //info mappatura bit corrente
		sprintf(&outbf[strlen(outbf)], "bit: %d: PORT=%d MASK=%llx\n", bitcnt, thisbit.port, thisbit.bitmask);
	}
	sprintf(&outbf[strlen(outbf)], "-----\n");
}
#endif // #ifdef DBG_MAP


static unsigned int set64bitmap(struct singlebit bitarr[], char * terne)
{
	unsigned int l, pos, cntbit, posbit, i, m;
	char * pt;
	char bf[3];
	int d=0; //DEBUG
	
	l = strlen((char *) terne);
	if ((l % 3) != 0)
		//check solo terne!
		return FALSE;
	
	if (d) {
		sprintf(debugBf,"\nset64bitmap|%s\n",terne);
		directToConsole(debugBf);
	}
	pt = (char *) terne;
	for (pos = 0, cntbit = 0; pos < l; pos += 3, cntbit++) {
		//set port number
		if (d) {
			sprintf(debugBf,"\ncntbit|%d\n",cntbit);
			directToConsole(debugBf);
		}
		switch (pt[pos]) {
			case '0':
				bitarr[cntbit].port = 0;
				break;
			case '1':
				bitarr[cntbit].port = 1;
				break;
			case '2':
				bitarr[cntbit].port = 2;
				break;
			case '3':
				bitarr[cntbit].port = 3;
				break;
			case '4':
				bitarr[cntbit].port = 4;
				break;
			case '9':
				bitarr[cntbit].port = 0;
				break;
		}
		if (d) {
			sprintf(debugBf,"\nport|%d\n",bitarr[cntbit].port);
			directToConsole(debugBf);
		}
		//calcola mask del bit specificato
		bf[0] = pt[pos + 1]; //decine numero bit
		bf[1] = pt[pos + 2]; //unita' nummero bit
		bf[2] = 0; //tappo

		posbit = atoi(bf);
		if (d) {
			sprintf(debugBf,"\nposbit|%d\n",posbit);
			directToConsole(debugBf);
		}
		for (m = 1, i = 0; i < posbit; i++)
			m = m << 1;
		if (d) {
			sprintf(debugBf,"\nbitmask|0x%X\n",m);
			directToConsole(debugBf);
		}
		bitarr[cntbit].bitmask = m; //set bit mask
	}

	return TRUE;

}

//set map Extender output
//NB: questo comando non riguarda il lato VisualBasic (gestito in loco)
unsigned int T_SetMapDigOut(char * mappa) {
	int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "T_SetMapDigOut (%s) GESTITO LOCALMENTE IN PARSER!\n", mappa);
	directToConsole(debugBf);
	//

	answer = set64bitmap(extenderOutBitMap, (char *) mappa);


#ifdef DBG_MAP
	//trace
	displayMap(extenderOutBitMap, debugBf);
	directToConsole(debugBf);
	//
#endif

	return answer;
}

//set map Extender output
//NB: questo comando non riguarda il lato VisualBasic (gestito in loco)
unsigned int T_SetMapPUPDFLIn(char * mappa) {
	int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "T_SetMapPUPDFLIn (%s) GESTITO LOCALMENTE IN PARSER!\n", mappa);
	directToConsole(debugBf);
	//

	answer = set64bitmap(extenderInPUPDFLBitMap, (char *) mappa);


#ifdef DBG_MAP
	//trace
	displayMap(extenderInPUPDFLBitMap, debugBf);
	directToConsole(debugBf);
	//
#endif

	return answer;
}

//set map MTS40 output
//NB: questo comando non riguarda il lato VisualBasic (gestito in loco)
unsigned int M_SetMapDigOut(char * mappa) {
	int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "M_SetMapDigOut (%s) GESTIONE LOCALE IN PARSER!\n", mappa);
	directToConsole(debugBf);
	//

	answer = set64bitmap(digitalOutBitMap, mappa);

#ifdef DBG_MAP
	//trace
	displayMap(digitalOutBitMap, debugBf);
	directToConsole(debugBf);
	//
#endif
	return answer;
}


//set map MTS40 inputs
//NB: questo comando non riguarda il lato VisualBasic (gestito in loco)
unsigned int M_SetMapDigIn(char * mappa) {
	int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "M_SetMapDigIn (%s) GESTIONE LOCALE IN PARSER!\n", mappa);
	directToConsole(debugBf);
	//

	answer = set64bitmap(digitalInBitMap, mappa);

#ifdef DBG_MAP
	//trace
	displayMap(digitalInBitMap, debugBf);
	directToConsole(debugBf);
	//
#endif

	return answer;
}

//NB: questo comando non riguarda il lato VisualBasic (gestito in loco)
unsigned int T_SetMapDigIn(char * mappa) {
	int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "T_SetMapDigIn (%s) GESTIONE LOCALE IN PARSER!\n", mappa);
	directToConsole(debugBf);
	//

	answer = set64bitmap(extenderInBitMap, mappa);

#ifdef DBG_MAP
	//trace
	displayMap(extenderInBitMap, debugBf);
	directToConsole(debugBf);
	//
#endif

	return answer;
}

//NB: questo comando non riguarda il lato VisualBasic (gestito in loco)
unsigned int M_SetMapVirtualIn(char * mappa) {
	int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "M_SetMapVirtualIn (%s) GESTIONE LOCALE IN PARSER!\n", mappa);
	directToConsole(debugBf);
	//

	answer = set64bitmap(virtualInBitMap, mappa);

#ifdef DBG_MAP
	//trace
	displayMap(virtualInBitMap, debugBf);
	directToConsole(debugBf);
	//
#endif

	return answer;
}
unsigned int Delay(int dsec) { 
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sDelay|%d", TRIGGER_STRING, dsec);
	directToConsole(debugBf);
	//


	printf("%sDELAY|%d", TRIGGER_STRING, dsec);
	//fflush(stdout);
	//scanf("%d", &answer);
	getanswer(locbuf);
	sscanf(locbuf, "%d", &answer);
	
	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}


//int readAndTestError(char * bf) {
//	char mybf[8];
//	char * p;
//	int i;
//
//	i = 0;
//	memset(readBf, 0, sizeof (readBf)); //clear buffer
//	scanf("%s", readBf); //read answer as string
//
//	//trace
//	sprintf(debugBf, " >>> readBf=%s\n", readBf);
//	directToConsole(debugBf);
//
//	if ((p = strstr(readBf, ERROR_STRING_START)) != NULL) {
//		//error from VB command: decode error code
//		memset(mybf, 0, sizeof (mybf)); //clear buffer
//		strncpy(mybf, &p[strlen(ERROR_STRING_START)], 4); //copia 4 cifre codice errore in mybf
//		i = atoi(mybf);
//		//errore non puo' essere 0!!!!
//		if (!i)
//			return 1;
//		else
//			return i;
//	} else
//		return 0;
//}

/*
	 Default MsgBox("Ciao",0,2,0,0,buf)
	 PARAMs:
		 Text= testo da scrivere
		 Label= intestazione della finestra se 0--> default= "Msgbox"
		 ntasti= numero tasti se 1 un tasto se 2 due tasti --> default 2
		 caption1= scritta tasto 1 --> default = "OK"
		 caption2= scritta tasto 2 --> default = "Annulla"
		 buf= dove mettere la risposta
		 se premuto tasto1 restituisce ## 
		 se premuto tasto2 restituisce #!   

	RETURNs:
		Da RV a Giuseppe: prima restituiva un numerico, adesso...non so....COMPLETA TU PER FAVORE

 */
//*********************************************************************************************************

unsigned int MsgBox(char * text, char * label, int ntasti, char * caption1, char * caption2, char * buf) {
	unsigned int answer = 0;
	
	//trace
	if (label == 0)
		label = "0";
	if (caption1 == 0)
		caption1 = "0";
	if (caption2 == 0)
		caption2 = "0";
	sprintf(debugBf, "%s%s|%s|%s|%d|%s|%s|", TRIGGER_STRING, "MsgBox", text, label, ntasti, caption1, caption2);
	directToConsole(debugBf);
	//
	printf("%s%s|%s|%s|%d|%s|%s|", TRIGGER_STRING, "MsgBox", text, label, ntasti, caption1, caption2);
	answer = getanswer(buf) ;
	
//	fflush(stdout);
//	fflush(stdin);					// Ciro - Brignolo
//	scanf("%s", buf);
//	answer = 0;

	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

/*
	aggiunge nella finestra di log (main form di VB) il messaggio passato.
	PARAMs:
		msg: messaggio da visulizzare
	
	RETURNs:
		0
 */

unsigned int PrintDB(char * buf) {
	unsigned int answer = 0;
	sprintf(debugBf, " >>> DEBUG=%s\n", buf);
	directToConsole(debugBf);
	//
	return answer;
}


unsigned int MsgWindow(char * msg) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sMsgWindow|%s\n", TRIGGER_STRING, msg);
	directToConsole(debugBf);
	//

	printf("%sMSGWINDOW|%s\n", TRIGGER_STRING, msg);
//	fflush(stdout);
//	//fflush(NULL);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

/*
append current message at the specified file name
	
es MsgFile(0,"Nomefile","Ciao")
  
PARAMs:
  data= 0 --> inserisce data se 1 --> non inserisce data
  fileName = NomeFile
  mess= messaggio
  	
RETURNs:
1 se errore
0 se ok
  	
 */
unsigned int MsgFile(int data, char * fileName, char * mess) {
	unsigned int answer = 0;
	//trace
	sprintf(debugBf, "%s%s|%d|%s|%s", TRIGGER_STRING, "MsgFile", data, fileName, mess);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%s|%s", TRIGGER_STRING, "MsgFile", data, fileName, mess);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

/*
  mode:
		#define KPROTOCOLOMODE_QUERYANSWER 	0
	  #define KPROTOCOLOMODE_BUFFERED 		1
 */
unsigned int SetProtocolMode(int mode) {

	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sSetProtocolMode", TRIGGER_STRING);
	directToConsole(debugBf);
	//

	//rileva input digitali del Dispositivo
	printf("%sSetProtocolMode|%d", TRIGGER_STRING, mode);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}


unsigned int SetProtocolComunication(int mode) {

	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sSetProtocolComunication>%d", TRIGGER_STRING,mode);
	directToConsole(debugBf);
	//

	//rileva input digitali del Dispositivo
	printf("%sSetProtocolComunication|%d", TRIGGER_STRING, mode);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}



unsigned int PrgExit(char * cause) {
	//unsigned int ii;
	unsigned int answer = 0;
#ifdef CBUG
	char aa[1000] ;
		
	MsgBox(cause,"EXIT",1,0,0,aa) ;
#endif
	
	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sexit", TRIGGER_STRING);
	directToConsole(debugBf);
	//

	printf("%sexit", TRIGGER_STRING);

	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	
	fflush(stdout);
	//scanf("%d", &ii);
	//fflush(stdout);
	exit(EXIT_SUCCESS);
	return 0;
}

unsigned int GetINIKeyVal(char * key, char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%s|", TRIGGER_STRING, "GetINIKeyVal", key);
	directToConsole(debugBf);
	//

	printf("%s%s|%s|", TRIGGER_STRING, "GetINIKeyVal", key);
	answer = getanswer(buf) ;

	//trace
	//sprintf(debugBf," >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int SetINIKeyVal(char * key, char * val) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%s|%s|", TRIGGER_STRING, "SetINIKeyVal", key, val);
	directToConsole(debugBf);
	//

	printf("%s%s|%s|%s|", TRIGGER_STRING, "SetINIKeyVal", key, val);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

	//_FR - Rel 3.76 - 26/05/23
unsigned int SetLevelDebug(uint32_t leveldbg){
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%u|", TRIGGER_STRING, "SetLevelDebug", leveldbg);
	directToConsole(debugBf);
	//

	printf("%s%s|%u|", TRIGGER_STRING, "SetLevelDebug", leveldbg);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int GetBestFileNum(char * path, char * basenome, char * buf) {
	unsigned int answer = 0 ;

	TKERROR = 0; //default: no error

	//rem_duble_slash(path,path);

	//trace
	sprintf(debugBf, "%s%s|%s|%s|", TRIGGER_STRING, "GetBestFileNum", path, basenome);
	directToConsole(debugBf);
	//

	printf("%s%s|%s|%s|", TRIGGER_STRING, "GetBestFileNum", path, basenome);
	//fflush(stdout);
	//QUI RICEVO STRINGA IN RISPOSTA!!! scanf("%d", &answer);
	//scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%s", buf) ;
	//trace
	//sprintf(debugBf," >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

/*
PARAMs:
 Default InputBox("Ciao",0,2,0,0,buf)
 Text= testo da scrivere
 Label= intestazione della finestra se 0 --> default= "InputBox"
 ntasti= numero tasti se 1 un tasto se 2 due tasti --> default 2
 caption1= scritta tasto 1 --> default = "OK"
 caption2= scritta tasto 2 --> default = "Annulla"
 buf= dove mettere la risposta
 se stringa nulla ed premuto tasto1 restituisce ##
 se premuto tasto2 restituisce #!
	
RETURNs:
		
	-la stringa immmessa dall'utente viene copiata in * buf
	-return value: 0
			
 */
unsigned int InputBox(char * text, char * label, int ntasti, char * caption1, char * caption2, char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	if (label == 0)
		label = "0";
	if (caption1 == 0)
		caption1 = "0";
	if (caption2 == 0)
		caption2 = "0";
	sprintf(debugBf, "%s%s|%s|%s|%d|%s|%s|", TRIGGER_STRING, "InputBox", text, label, ntasti, caption1, caption2);
	directToConsole(debugBf);
	//

	printf("%s%s|%s|%s|%d|%s|%s|", TRIGGER_STRING, "InputBox", text, label, ntasti, caption1, caption2);

	getanswer(buf) ;

//	fflush(stdout);
//	//QUI RICEVO STRINGA IN RISPOSTA!!! scanf("%d", &answer);
//	//scanf("%d", &answer);
//	fflush(stdin);					// Ciro - Brignolo
//	gets(buf);
	//scanf("%s", buf);


	answer = 0;

	//trace
	//sprintf(debugBf," >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}


unsigned int GetWorkSpace(char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s", TRIGGER_STRING, "GetWorkSpace");
	directToConsole(debugBf);
	//

	//fflush(stdout);
	printf("%s%s", TRIGGER_STRING, "GetWorkSpace");
	getanswer(buf) ;

//	fflush(stdout);
//	scanf("%s", buf);
	answer = 0;


	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int GetTerzista(char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s", TRIGGER_STRING, "Terzista");
	directToConsole(debugBf);
	//

	printf("%s%s", TRIGGER_STRING, "Terzista");
	getanswer(buf) ;

//	fflush(stdout);
//	scanf("%s", buf);
	answer = 0;


	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int OuputText(int q, char * text, int cbase, char * label, int lamp, int clamp) {
	// Default LabelRun OuputText(1,"Test Ingressi",0,0,0,0)
	// q= numero di ouput da scrivere
	//      1 LabelRun
	//      2 free0
	//		3 free1
	//		4 free2
	//      5 free3
	//		6 free4
	//		7 free5
	//		8 free6
	//		9 free7
	// ROVERA aggiunti per gli STEP del taverniti da 100 in avanti
	// text  = quello da scrivere nella casella 
	// 		 STEP: il testo dello step, se nullo non lo cambia
	// cbase = colore di base --> default 0 bianco per labelrun e grigio chiaro(&H8000000F) per free
	//		 STEP: colore dell'indicatore del'esito dello STEP
	// label = intestazione della casella se 0 --> default= "Free"
	// 		 STEP: ignorato
	// lamp  = se 0 non lampeggia la casella di testo se 1 lampeggia la casella di testo --> default 0
	// 		 STEP: ignorato
	// clamp = colore per lampeggio --> default = 0 "nessun colore"
	// 		 STEP: ignorato
	//
	// tabella colori di base:
	// 1 = red
	// 2 = green
	// 3 = yellow
	// 4 = white
	// 5 = black
	// 6 = blue
	// 7 = cyan
	// 8 = magenta
	// 9 = grey (&H808080)

	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	if (label == 0)
		label = "0";
	sprintf(debugBf, "%s%s|%d|%s|%d|%s|%d|%d|", TRIGGER_STRING, "OuputText", q, text, cbase, label, lamp, clamp);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%s|%d|%s|%d|%d|", TRIGGER_STRING, "OuputText", q, text, cbase, label, lamp, clamp);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);

	return answer;
}

unsigned int ProgressBar(int tipo, int param) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%d", TRIGGER_STRING, "ProgressBar", tipo, param);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d", TRIGGER_STRING, "ProgressBar",  tipo, param);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);
	answer = 0;


	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}


//	==================================================================================
// Funzioni vs TestKit
//	==================================================================================
unsigned int T_Output(unsigned long long m, unsigned long long v) {
	if ( (m==NEWPRES) && (TKTYPE==1) ) v ^= m ;  //Presenza nel NewTk Negata
	if ( (m==NEWEN_CNT_) && (TKTYPE==1) ) v ^= m ;  //EN_CNT nel NewTk Negata
	//if ( (m==NEWCAN_EN_) && (TKTYPE==1) ) v ^= m ;  //CAN_EN nel NewTk Negata

	//pretrace
	sprintf(debugBf, "%s%s|%llx|%llx\n", TRIGGER_STRING, "T_Outputpreord",m,v);
	directToConsole(debugBf);
	//
	
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	doOutCmd(extenderOutBitMap, m, v);

	//trace
	sprintf(debugBf, "%s%s|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x|%x", TRIGGER_STRING, "T_Output", outMask[0], outData[0], outMask[1], outData[1], outMask[2], outData[2], outMask[3], outData[3],outMask[4], outData[4], outMask[5], outData[5], outMask[6], outData[6], outMask[7], outData[7]);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d", TRIGGER_STRING, "T_OUTPUT", outMask[0], outData[0], outMask[1], outData[1], outMask[2], outData[2], outMask[3], outData[3],outMask[4], outData[4], outMask[5], outData[5], outMask[6], outData[6], outMask[7], outData[7]);
//	fflush(stdout);
//	scanf("%d", &answer);

	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int T_SetPull(unsigned long long m, unsigned long long v) {
	
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//pretrace
	sprintf(debugBf, "%s%s|%llx|%llx\n", TRIGGER_STRING, "T_SetPullpreord",m,v);
	directToConsole(debugBf);
	//

	doOutCmd(extenderInPUPDFLBitMap, m, v);

	//trace
	sprintf(debugBf, "%s%s|%x|%x|%x|%x|%x|%x|%x|%x", TRIGGER_STRING, "T_SetPull", outMask[0], outData[0], outMask[1], outData[1], outMask[2], outData[2], outMask[3], outData[3]);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d|%d|%d|%d|%d|%d|%d", TRIGGER_STRING, "T_SetPull", outMask[0], outData[0], outMask[1], outData[1], outMask[2], outData[2], outMask[3], outData[3]);
//	fflush(stdout);
//	scanf("%d", &answer);

	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

_ExtenderData T_ReadAll() {
	unsigned int answer = 0;
	_ExtenderData ed;
	int i;

	TKERROR = 0; //default: no error
	//unsigned int answer;

	//trace
	sprintf(debugBf, "%sT_ReadAll", TRIGGER_STRING);
	directToConsole(debugBf);
	//

	//forza capture dati da extender
	printf("%sinputExtRefresh", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	directToConsole("T_ReadAll -> \t inputExtRefresh DONE\n");

	//rileva single parti: pioA
	printf("%sinputExtA", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[0]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[0]);

	//rileva single parti: pioB
	printf("%sinputExtB", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[1]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[1]);

	//rileva single parti: pioC
	printf("%sinputExtC", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[2]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[2]);

	//rileva single parti: pioD
	printf("%sinputExtD", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[3]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[3]);

	if (TKTYPE==1){
	//rileva single parti: pioE
	printf("%sinputExtE", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[3]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[4]);
		directToConsole("T_ReadAll -> \t inputExtA,B,C,D,E DONE");
	}else{
		directToConsole("T_ReadAll -> \t inputExtA,B,C,D DONE");
	}
	
	//copy in rec	
	for (i = 0; i < 8; i++)
		ed.digitals[i] = readedData[i];

	ed.digital64 = doInCmd(extenderInBitMap);

	for (i = 0; i < KUSED_EXT_ANALOGS; i++) {
		//rileva  anag0 o anag1
		printf("%sT_ANALOG|%d", TRIGGER_STRING, i);		
//		fflush(stdout);
//		scanf("%d", &ed.analogs[i]);
		getanswer(locbuf) ;
		sscanf(locbuf, "%d", &ed.analogs[i]);
	}

	directToConsole("T_ReadAll -> \t T_ANALOG 0-18 DONE");

	for (i = 0; i < KUSED_EXT_COUNTERS; i++) {
		//rileva  counters
		printf("%sT_CNT|%d", TRIGGER_STRING, i);
//		fflush(stdout);
//		scanf("%d", &ed.counters[i]);
		getanswer(locbuf) ;
		sscanf(locbuf, "%d", &ed.counters[i]);
	}

	directToConsole("T_ReadAll -> \t T_COUNTER 0-1 DONE");

	if (TKTYPE==0){ 
		//trace
		sprintf(debugBf, "\n\tpioA=%x\tpioB=%x\tpioC=%x\tpioD=%x\t",
			readedData[0],
			readedData[1],
			readedData[2],
			readedData[3]
			);
		directToConsole(debugBf);
		//
	}else{
			//trace
		sprintf(debugBf, "\n\tpioA=%x\tpioB=%x\tpioC=%x\tpioD=%x\tpioE=%x\t",
			readedData[0],
			readedData[1],
			readedData[2],
			readedData[3],
		    readedData[4]
			);
		directToConsole(debugBf);
		//
	}

	//answer=doInCmd(extenderInBitMap);

	return ed; //return ExtenderData record
}

unsigned int T_Input() {

	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sT_Input", TRIGGER_STRING);
	directToConsole(debugBf);
	//

	//forza refresh dati da extender
	printf("%sinputExtRefresh", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf);
	sscanf(locbuf, "%d", &answer);


	//rileva single parti: pioA
	printf("%sinputExtA", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[0]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[0]);

	//rileva single parti: pioB
	printf("%sinputExtB", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[1]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[1]);

	//rileva single parti: pioC
	printf("%sinputExtC", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[2]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[2]);

	//rileva single parti: pioD
	printf("%sinputExtD", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &readedData[3]);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &readedData[3]);

	if (TKTYPE==0){ 
		//trace
		sprintf(debugBf, "\n\tpioA=%x\tpioB=%x\tpioC=%x\tpioD=%x\t",
			readedData[0],
			readedData[1],
			readedData[2],
			readedData[3]
			);
		directToConsole(debugBf);
		//
	}else{
		//rileva single parti: pioD
		printf("%sinputExtE", TRIGGER_STRING);
//		fflush(stdout);
//		scanf("%d", &readedData[3]);
		getanswer(locbuf) ;
		sscanf(locbuf, "%d", &readedData[4]);
		
			//trace
		sprintf(debugBf, "\n\tpioA=%x\tpioB=%x\tpioC=%x\tpioD=%x\tpioE=%x\t",
			readedData[0],
			readedData[1],
			readedData[2],
			readedData[3],
		    readedData[4]
			);
		directToConsole(debugBf);
		//
	}

	answer = doInCmd(extenderInBitMap);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int T_GetVer(char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|", TRIGGER_STRING, "T_GetVer");
	directToConsole(debugBf);
	//

	printf("%s%s|", TRIGGER_STRING, "T_GetVer");
	//fflush(stdout);
	//QUI RICEVO STRINGA IN RISPOSTA!!! scanf("%d", &answer);
	//scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%s", buf);
	answer = 0;

	//trace
	//sprintf(debugBf," >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int T_GetType(char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|", TRIGGER_STRING, "T_GetType");
	directToConsole(debugBf);
	//

	printf("%s%s|", TRIGGER_STRING, "T_GetType");
	//fflush(stdout);
	//QUI RICEVO STRINGA IN RISPOSTA!!! scanf("%d", &answer);
	//scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%s", buf);
	answer = 0;

	//trace
	//sprintf(debugBf," >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int T_SetFTDI(char * buf) {
	/*//DEBUG
	char Bmom[512];
	sprintf(Bmom,"PASSO T_SetFTDI TKTYPE=%d\n", TKTYPE);
	PrintDB(Bmom);
	*///DEBUG
	
	if ( (TKTYPE==0) && strcmp(buf,"USB") ){ 
		DoProgram(PathTK, "sendparam.bat", buf);
		return 0;
	}else{
		unsigned int answer = 0;

		TKERROR = 0; //default: no error

		//trace
		sprintf(debugBf, "%s%s|%s", TRIGGER_STRING, "T_SetFTDI", buf);
		directToConsole(debugBf);

		printf("%s%s|%s", TRIGGER_STRING, "T_SetFTDI", buf);
		if (getanswer(locbuf))
                answer=1; // ERRORE
                else
		sscanf(locbuf, "%d", &answer);

		//trace
		sprintf(debugBf, " >>> answer=%x\n", answer);
		directToConsole(debugBf);
		//

		return answer;
	}
}

//return Extender Analogs
// channelId 0-18

unsigned int T_Analog(unsigned int channelId) {

	TKERROR = 0; //default: no error

	unsigned int answer = 0;
	//unsigned int channelId=*((unsigned int *) n[0]);			//read channelId (arg 1)

	//trace
	sprintf(debugBf, "%sinputExtRefresh", TRIGGER_STRING);
	directToConsole(debugBf);
	//

	//forza refresh dati da extender
	printf("%sinputExtRefresh", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, "%sT_Analog|%d", TRIGGER_STRING, channelId);
	directToConsole(debugBf);
	//
	//rileva single parti: anag0 o anag1
	printf("%sT_ANALOG|%d", TRIGGER_STRING, channelId);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

//read counter value from extender

unsigned int T_Cnt(unsigned int counterId) {

	TKERROR = 0; //default: no error

	unsigned int answer = 0;
	//	unsigned int counterId=*((unsigned int *) n[0]);			//read counterId (arg 1)

	//trace
	sprintf(debugBf, "%sinputExtRefresh", TRIGGER_STRING);
	directToConsole(debugBf);
	//

	//forza refresh dati da extender
	printf("%sinputExtRefresh", TRIGGER_STRING);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, "%sT_Cnt|%d", TRIGGER_STRING, counterId);
	directToConsole(debugBf);
	//

	//rileva single parti: 
	printf("%sT_CNT|%d", TRIGGER_STRING, counterId);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

/*
set specified serial port options (<Baud Rate>,<Parity>,<bits>,<stop bits>,<handshake>) ad clear RX buffer
		params:
			portNum:  0,1,2,3
			
		nota: portnum=1
 */
unsigned int T_SetComPort(unsigned int portNum, unsigned int baudrate, char parity, unsigned int bits, unsigned int stop, char handshake) {

	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%d|%c|%d|%d|%c", TRIGGER_STRING, "T_SetComPort", portNum, baudrate, parity, bits, stop, handshake);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d|%c|%d|%d|%c", TRIGGER_STRING, "T_SetComPort", portNum, baudrate, parity, bits, stop, handshake);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

//send the binary values from hexData to the specified serial port

unsigned int T_ComSend(unsigned int port, char * Data) {
	unsigned int answer = 0;
	char HexData[MAXSIZE];
	
    text_to_hex(Data,HexData);
	
	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%s", TRIGGER_STRING, "T_ComSend", port, HexData);
	directToConsole(debugBf);

	printf("%s%s|%d|%s", TRIGGER_STRING, "T_ComSend", port, HexData);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int T_ComSendHex(unsigned int port, char * HexData) {
	unsigned int answer = 0;
	
	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%s", TRIGGER_STRING, "T_ComSend", port, HexData);
	directToConsole(debugBf);

	printf("%s%s|%d|%s", TRIGGER_STRING, "T_ComSend", port, HexData);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

//compare the binary values from hexData with the received data. Return 0 if OK.

unsigned int T_CheckRec(unsigned int port, char * Data) {
	unsigned int answer = 1;
	char HexData[MAXSIZE];
	char DataCOM[MAXSIZE];
	int laux,i;
	
    text_to_hex(Data,HexData);
	
	TKERROR = 0; //default: no error
	for (i=0;i<3;i++){		
		sprintf(debugBf, "%s%s|%d|%s", TRIGGER_STRING, "T_CheckRec", port, HexData);
		directToConsole(debugBf);
	
		printf("%s%s|%d|%s", TRIGGER_STRING, "T_CheckRec", port, HexData);
		getanswer(locbuf) ;
		sscanf(locbuf, "%s", DataCOM);

		//trace
		sprintf(debugBf, " >>> DataCOM=%s\n", DataCOM);
		directToConsole(debugBf);
		//

		
		sprintf(debugBf, "\nstrcmp=<%d>\n",strcmp(DataCOM,HexData));
		directToConsole(debugBf);
		laux=strlen(DataCOM);
		sprintf(debugBf, "\nlaux=<%d>\n",laux);
		directToConsole(debugBf);
		if (strcmp(DataCOM,HexData)==0) {
			answer=0; //ok
		}else{
			char momaux[MAXSIZE];
			if (laux>3){
				char*from=DataCOM;
	  			char *to = (char*) malloc(4);
				strncpy(to, from+2, 5);
				strcpy(momaux,to);
			}else
				strcpy(momaux,DataCOM);
			sprintf(debugBf, "\nmomaux=<%s>\n",momaux);
			directToConsole(debugBf);
			sprintf(debugBf, "\nstrstr=<%s>\n",strstr(HexData,momaux));
			directToConsole(debugBf);
			if ( strstr(HexData,momaux) > 0 ) {
				answer=0; //ok
			}else{
				answer=1; //ko
			}
		}
		//trace
		sprintf(debugBf, " >>> answer=%d\n", answer);
		directToConsole(debugBf);
		if (answer==0) break ;
	}
	return answer;
}

// -----------------------------------------------------------
//				Controllo LED ROSSO di Test-Kit
// -----------------------------------------------------------
// - mask: pattern di 32 bit che verrà inviato a Led Rosso (bit a '1' = Led acceso, a '0' Led spento)
// - period: tempo di durata bit del pattern in ms (deve essere >10 e viene letto come decine di ms)
unsigned int T_Led(unsigned int mask, unsigned int period) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%x|%d", TRIGGER_STRING, "T_Led", mask, period);
	directToConsole(debugBf);
	//

	printf("%s%s|%x|%d", TRIGGER_STRING, "T_Led", mask, period);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int T_SetCanBaudrate(unsigned int canNum, unsigned int baudrate) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%d", TRIGGER_STRING, "T_SetCanBaudrate", canNum, baudrate);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d", TRIGGER_STRING, "T_SetCanBaudrate", canNum, baudrate);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int T_SetCanMailbox(unsigned int canNum, unsigned int mailboxnum, unsigned int canMask, unsigned int canAddr, char extended, unsigned int txperiod) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%x|%x|%x|%x|%c|%x", TRIGGER_STRING, "T_SetCanMailbox", canNum, mailboxnum, canMask, canAddr, extended, txperiod);
	directToConsole(debugBf);
	//

	printf("%s%s|%x|%x|%x|%x|%c|%x", TRIGGER_STRING, "T_SetCanMailbox", canNum, mailboxnum, canMask, canAddr, extended, txperiod);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int T_EmitCanFrame(unsigned int canNum, unsigned int mailboxnum, char * databyte) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%x|%x|%s", TRIGGER_STRING, "T_EmitCanFrame", canNum, mailboxnum, databyte);
	directToConsole(debugBf);
	//

	printf("%s%s|%x|%x|%s", TRIGGER_STRING, "T_EmitCanFrame", canNum, mailboxnum, databyte);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}


//	==================================================================================
// Funzioni vs MTS
//	==================================================================================

unsigned int M_DirectTOLU (unsigned int LU, char * Data) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%s", TRIGGER_STRING, "M_DirectTOLU", LU, Data);
	directToConsole(debugBf);

	printf("%s%s|%d|%s", TRIGGER_STRING, "M_DirectTOLU", LU, Data);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%d\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int M_Output(unsigned int m, unsigned int v) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	doOutCmd(digitalOutBitMap, m, v);

	//trace
	sprintf(debugBf, "%s%s|%x|%x", TRIGGER_STRING, "M_Output", outMask[0], outData[0]);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d", TRIGGER_STRING, "M_OUTPUT", outMask[0], outData[0]);
//	fflush(stdout);
//	scanf("%d", &answer);

	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

/*
	rileva input digitali del DUT.



 */

unsigned int M_Input() {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	
	do {
	    answer = 0;
	    TKERROR = 0; //default: no error
	    //trace
	    sprintf(debugBf, "%sM_Input", TRIGGER_STRING);
	    directToConsole(debugBf);
	    //

	    //rileva input digitali del Dispositivo
	    printf("%sM_INPUT", TRIGGER_STRING);
	    //fflush(stdout);

//	    if ((TKERROR = readAndTestError(readBf)) != 0) {
//			//trace
//			//sprintf(debugBf," >>> ERROR code=%x\n", TKERROR);
//			//directToConsole(debugBf);
//	    } else {
			// sscanf(readBf, "%d", &readedData[0]);
		if ( !(getanswer(locbuf))) {
			//scanf("%d", &readedData[0]);
			sscanf(locbuf, "%d", &readedData[0]);
			readedData[1] = 0;
			readedData[2] = 0;
			readedData[3] = 0;

			answer = doInCmd(digitalInBitMap);

			//trace
			sprintf(debugBf," >>> answer=%x\n", answer);
			directToConsole(debugBf);
			//
			return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	
	return 0;
}

unsigned int M_InStatus() {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	
	do {
	    answer = 0;
		TKERROR=0;	//default: no error
	
		//trace
		sprintf(debugBf,"%sM_InStatus", TRIGGER_STRING);
		directToConsole(debugBf);
	
		//rileva input di Stato del Dispositivo
		printf("%sM_INSTATUS", TRIGGER_STRING);
	
		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%d", &readedData[0]);
			readedData[1]=0;
			readedData[2]=0;
			readedData[3]=0;
			answer=readedData[0];
			return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	
	return 0;
}

unsigned int M_SetStatus(unsigned long statusval, unsigned long statusmask) {
	unsigned int answer = 0;
	
	TKERROR=0;	//default: no error

	//trace
	sprintf(debugBf,"%sM_SetStatus at 0x%lx of 0x%lx", TRIGGER_STRING, statusval, statusmask );
	directToConsole(debugBf);

	// imposta flag di Stato del Dispositivo
	printf("%sM_SETSTATUS|%lu|%lu", TRIGGER_STRING, statusval, statusmask );
	
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}


_InternalAnalogData M_ReadIntAD() {
    _InternalAnalogData dt;
    unsigned int locRetryCounter=0;
    
    do {
		TKERROR = 0; //default: no error
	
		//trace
		sprintf(debugBf, "%sM_ReadIntAD", TRIGGER_STRING);
		directToConsole(debugBf);
		//
	
		//forza refresh dati da extender
		printf("%sM_ReadIntAD", TRIGGER_STRING);
		//fflush(stdout);
		//scanf("%d", &answer);
	
		//if ((TKERROR = readAndTestError(readBf)) == 0) {
		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%d&%d&%d&%d&%d&%d&%d&%d&%d&%d&", &dt.Vext, &dt.Vbatt, &dt.csq, &dt.temp, &dt.x, &dt.y, &dt.z, &dt.extin, &dt.adv1, &dt.adv2);
			//trace
			sprintf(debugBf, " >>> answer=%s\n", locbuf);
			directToConsole(debugBf);
			//
			return dt;
		}
		++locRetryCounter;
    } while ((TKERROR) && (locRetryCounter < maxRetryOnError));

	//trace
	sprintf(debugBf, " >>> answer=%s\n", locbuf);
	directToConsole(debugBf);
	//
    return dt;
}

_HwVers M_GetHwVers() {
    _HwVers hwv;
    unsigned int locRetryCounter=0;
    
    do {
		TKERROR = 0; //default: no error
	
		//trace
		sprintf(debugBf, "%sM_GetHwVers", TRIGGER_STRING);
		directToConsole(debugBf);
		//
	
		//forza refresh dati da extender
		printf("%sM_GetHwVers", TRIGGER_STRING);
		//fflush(stdout);
		//scanf("%d", &answer);
	
		//if ((TKERROR = readAndTestError(readBf)) == 0) {
		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%d&%d&", &hwv.HwMain, &hwv.HwSrv);
			return hwv;
		}
		++locRetryCounter;
    } while ((TKERROR) && (locRetryCounter < maxRetryOnError));

    return hwv;
}


unsigned int M_Analog(int channelId) {

	unsigned int answer = 0;
	//unsigned int channelId=*((unsigned int *) n[0]);			//read channelId (arg 1)

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sM_Analog|%d", TRIGGER_STRING, channelId);
	directToConsole(debugBf);
	//

	printf("%sM_ANALOG|%d", TRIGGER_STRING, channelId);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%d\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int M_InVirt() {
	unsigned int answer;
	unsigned int locRetryCounter=0;

	do {
	    TKERROR = 0; //default: no error
	    answer = 0;

	    //trace
	    sprintf(debugBf, "%sM_InVirt", TRIGGER_STRING);
	    directToConsole(debugBf);
	    //

	    //rileva input virtuali del Dispositivo
	    printf("%sM_INVIRT", TRIGGER_STRING);
	    //fflush(stdout);

//	    if ((TKERROR = readAndTestError(readBf)) == 0) {
//		sscanf(readBf, "%d", &readedData[0]);
		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%d", &readedData[0]);
			readedData[1] = 0;
			readedData[2] = 0;
			readedData[3] = 0;
	
			answer = doInCmd(virtualInBitMap);
	
			//trace
			//sprintf(debugBf," >>> answer=%x\n", answer);
			//directToConsole(debugBf);
			//
			return answer;
	    }

	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;
}



//imposta il SOURCEID di protocollo usato dall'mtstestkit (mtstestkit usa di default=11)

unsigned int M_SetSourceId(int id) {
    unsigned int answer;
    int arg=0;
	arg=arg;   // Per Compilatore Set but not used
    unsigned int locRetryCounter=0;

   do {
	answer = 0;
	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sM_SETSOURCEID|%d", TRIGGER_STRING, id);
	directToConsole(debugBf);
	//

	printf("%sM_SETSOURCEID|%d", TRIGGER_STRING, id);
	//fflush(stdout);

	//if ((TKERROR = readAndTestError(readBf)) == 0) {
	if ( !(getanswer(locbuf))) {
	    arg = sscanf(locbuf, "%d", &answer);
	    return answer;
	}
	++locRetryCounter;
    } while ((TKERROR) && (locRetryCounter < maxRetryOnError));
    return 0;
}


unsigned int M_GetSerNum() {
	unsigned int answer;
	int arg=0;
	arg=arg;   // Per Compilatore Set but not used

	unsigned int locRetryCounter=0;
	do {
	    answer = 0;
	    TKERROR = 0; //default: no error

	    //trace
	    sprintf(debugBf, "%sM_GetSerNum", TRIGGER_STRING);
	    directToConsole(debugBf);
	    //

	    printf("%sM_GETSERNUM", TRIGGER_STRING);
	    //fflush(stdout);

	    //if ((TKERROR = readAndTestError(readBf)) == 0) {
		if ( !(getanswer(locbuf))) {
			arg = sscanf(locbuf, "%d", &answer);
			return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;
}

unsigned int M_GetSwVers() {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	do {
	    answer = 0;
	    TKERROR = 0; //default: no error

	    //trace
	    sprintf(debugBf, "%sM_GetSwVers", TRIGGER_STRING);
	    directToConsole(debugBf);
	    //

	    printf("%sM_GETSWVERS", TRIGGER_STRING);
	    //fflush(stdout);

	    //if ((TKERROR = readAndTestError(readBf)) == 0) {
		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%d", &answer);
			return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;
}


//riporta il valore del campo mts_type relativo la IDREP_STATS

unsigned int M_GetFamily() {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	do {
	    answer = 0;
	    TKERROR = 0; //default: no error

	    //trace
	    sprintf(debugBf, "%sM_GetFamily", TRIGGER_STRING);
	    directToConsole(debugBf);
	    //

	    printf("%sM_GETFAMILY", TRIGGER_STRING);
	    //fflush(stdout);

	    //if ((TKERROR = readAndTestError(readBf)) == 0) {
		if ( !(getanswer(locbuf))) {
		    sscanf(locbuf, "%d", &answer);
		    return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;
}

unsigned int M_GetTime() {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	do {
	    answer = 0;
	    TKERROR = 0; //default: no error

	    //trace
	    sprintf(debugBf, "%sM_GetTime", TRIGGER_STRING);
	    directToConsole(debugBf);
	    //

	    printf("%sM_GETTIME", TRIGGER_STRING);
	    //fflush(stdout);

	    //if ((TKERROR = readAndTestError(readBf)) == 0) {
		if ( !(getanswer(locbuf))) {
		    sscanf(locbuf, "%d", &answer);
		    return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;
}

unsigned int M_GetGpsFlags() {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	do {
	    answer = 0;

	    TKERROR = 0; //default: no error

	    //trace
	    sprintf(debugBf, "%sM_GetGpsFlags", TRIGGER_STRING);
	    directToConsole(debugBf);
	    //

	    printf("%sM_GETGPSFLAGS", TRIGGER_STRING);
	    //fflush(stdout);

	    if ( (TKERROR=getanswer(locbuf)) != 0) {
		    sscanf(locbuf, "%d", &answer);
			return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;
}


unsigned int M_Diag(unsigned int type, unsigned int val, char * risp) {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	do {
	    answer = 0;
	    TKERROR = 0; //default: no error
	    //trace
	    sprintf(debugBf, "%s%s|%d|%d", TRIGGER_STRING, "M_Diag", type, val);
	    directToConsole(debugBf);
	    //

	    printf("%s%s|%d|%d", TRIGGER_STRING, "M_DIAG", type, val);
	    //fflush(stdout);

	    //QUI RICEVO STRINGA IN RISPOSTA!!!
	    //if ((TKERROR = readAndTestError(readBf)) == 0) {
		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%s", risp);
			answer = 0;
			replacespace(risp);			
			//trace
			//sprintf(debugBf," >>> answer=%x\n", answer);
			sprintf(debugBf, " >>> answer=%s\n", risp);
			directToConsole(debugBf);
			//
			return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;

}



unsigned int M_GetPar(unsigned int num, char * val) {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	do {
	    answer = 0;

	    TKERROR = 0; //default: no error

	    //trace
	    sprintf(debugBf, "%s%s|%d", TRIGGER_STRING, "M_GetPar", num);
	    directToConsole(debugBf);
	    //

	    printf("%s%s|%d", TRIGGER_STRING, "M_GetPar", num);
	    //fflush(stdout);

	    //QUI RICEVO STRINGA IN RISPOSTA!!! scanf("%d", &answer);
//	    if ((TKERROR = readAndTestError(readBf)) == 0) {
//					sscanf(readBf, "%s", val);
		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%s", val);
			
			answer = 0;
			
			//trace
			//sprintf(debugBf," >>> answer=%x\n", answer);
			sprintf(debugBf, " >>> answer=%s\n", val);
			directToConsole(debugBf);
			//
			return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;

}


unsigned int M_SetPar(unsigned int num, char * val) {
	unsigned int answer;
	unsigned int locRetryCounter=0;
	do {
	    answer = 0;

	    TKERROR = 0; //default: no error

	    //trace
	    sprintf(debugBf, "%s%s|%d|%s", TRIGGER_STRING, "M_SetPar", num, val);
	    directToConsole(debugBf);
	    //

	    printf("%s%s|%d|%s", TRIGGER_STRING, "M_SetPar", num, val);
	    //fflush(stdout);


		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%d",&answer);
			
			//trace
			sprintf(debugBf, " >>> answer=%d\n", answer);
			directToConsole(debugBf);
			//
			return answer;
	   }
	   ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
	return 0;

}
/*
unsigned int M_SetPar(unsigned int num, char * val) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%s", TRIGGER_STRING, "M_SetPar", num, val);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%s", TRIGGER_STRING, "M_SetPar", num, val);
	//fflush(stdout);
	
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;

}
*/
unsigned int M_DelPar(unsigned int num) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d", TRIGGER_STRING, "M_DelPar", num);
	directToConsole(debugBf);
	//

	printf("%s%s|%d", TRIGGER_STRING, "M_DelPar", num);
//	fflush(stdout);
//	scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;

}

unsigned int M_Action(unsigned int actcode, unsigned int actval, char * actpar) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%d|%s", TRIGGER_STRING, "M_Action", actcode, actval, actpar);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d|%s", TRIGGER_STRING, "M_EXESM", actcode, actval, actpar);
	getanswer(locbuf) ;
	if (actval==255){
		sscanf(locbuf, "%d", &answer);
	}else{
		sscanf(locbuf, "%d", &answer);
	}
	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;

}

unsigned int M_Actionwithresp(unsigned int actcode, unsigned int actval, char * actpar,char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%d|%s", TRIGGER_STRING, "M_Actionwithresp", actcode, actval, actpar);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d|%s", TRIGGER_STRING, "M_EXESM", actcode, actval, actpar);
	getanswer(locbuf) ;
	sscanf(locbuf, "%s", buf);
	answer = 0;

	//trace
	//sprintf(debugBf," >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;

}

unsigned int DoProgram(char * path, char * program, char * argos) {
	unsigned int answer = 0;
	
	TKERROR = 0; //default: no error
	
	//rem_duble_slash(path,path);
	
	//trace
	sprintf(debugBf, "%s%s|%s|%s|%s|", TRIGGER_STRING, "DoProgram", path, program, argos);
	directToConsole(debugBf);
	//

	printf("%s%s|%s|%s|%s", TRIGGER_STRING, "M_PROGRAM", path, program, argos);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

//ch=0 >> CANB
//ch=1 >> CANC

unsigned int M_CanClear(unsigned int ch, unsigned int baudrate) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sM_CanClear|%d|%d", TRIGGER_STRING, ch, baudrate);
	directToConsole(debugBf);
	//

	printf("%sM_CANCLEAR|%d|%d", TRIGGER_STRING, ch, baudrate);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//
	return answer;
}

//ch=0 >> CANB
//ch=1 >> CANC

unsigned int M_CanAdd(unsigned int ch, unsigned int txperiod, unsigned int canAddr, unsigned int canMask) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%d|%d|%d", TRIGGER_STRING, "M_CanAdd", ch, txperiod, canAddr, canMask);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d|%d|%d", TRIGGER_STRING, "M_CANADD", ch, txperiod, canAddr, canMask);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;

}

//ch=0 >> CANB
//ch=1 >> CANC

unsigned int M_CanStart(unsigned int ch,char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d", TRIGGER_STRING, "M_CanStart", ch);
	directToConsole(debugBf);
	//

	printf("%s%s|%d", TRIGGER_STRING, "M_CANSTART", ch);
	getanswer(locbuf) ;
	sscanf(locbuf, "%s", buf);
	answer = 0;

	//trace
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

/* M_Cnt counterId=1 >>> I counter
	 M_Cnt counterId=2 >>> II counter
 */
unsigned int M_Cnt(unsigned int counterId) {

	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%sM_Cnt|%d", TRIGGER_STRING, counterId);
	directToConsole(debugBf);
	//

	printf("%sM_Cnt|%d", TRIGGER_STRING, counterId);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%d\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int M_CanClearBuffer(unsigned int ch) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d", TRIGGER_STRING, "M_CanClearBuffer", ch);
	directToConsole(debugBf);
	//

	printf("%s%s|%d", TRIGGER_STRING, "M_CanClearBuffer", ch);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int M_CanCheck(unsigned int ch, unsigned int canAddr, char * databyte) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%d|%s", TRIGGER_STRING, "M_CanCheck", ch, canAddr, databyte);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d|%s", TRIGGER_STRING, "M_CanCheck", ch, canAddr, databyte);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int M_SetTime(unsigned int currenttime, unsigned int newtime) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%d", TRIGGER_STRING, "M_SetTime", currenttime, newtime);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%d", TRIGGER_STRING, "M_SetTime", currenttime, newtime);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);
	//

	return answer;

}

unsigned int M_GetIMEI(char * buf) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|", TRIGGER_STRING, "M_GetIMEI");
	directToConsole(debugBf);
	//

	printf("%s%s|", TRIGGER_STRING, "M_GetIMEI");
	//fflush(stdout);
	//QUI RICEVO STRINGA IN RISPOSTA!!! scanf("%d", &answer);
	//scanf("%d", &answer);
	getanswer(locbuf) ;
	sscanf(locbuf, "%s", buf);
	answer = 0;

	//trace
	//sprintf(debugBf," >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

unsigned int M_GetDirect(char * buf) {
	
	unsigned int answer = 0;
	
	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|", TRIGGER_STRING, "M_GetDirect");
	directToConsole(debugBf);
	//
	//buf[0] = '\0' ;
	printf("%s%s|", TRIGGER_STRING, "M_GetDirect");

	answer = getanswer(buf) ;

	//trace
	sprintf(debugBf, " >>> postpreanswer=%s\n", buf);
	directToConsole(debugBf);
	
	replacespace(buf);

	//trace
	//sprintf(debugBf," >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", buf);
	directToConsole(debugBf);
	//

	return answer;
}

//upload SM file

unsigned int M_GetSmFile(char * filename) {
	unsigned int answer = 0;

	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%s|", TRIGGER_STRING, "M_GetSmFile", filename);
	directToConsole(debugBf);
	//

	printf("%s%s|%s|", TRIGGER_STRING, "M_GetSmFile", filename);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);

	return answer;
}

//download SM file

unsigned int M_PutSmFile(char * filename) {
	unsigned int answer = 0;


	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%s|", TRIGGER_STRING, "M_PutSmFile", filename);
	directToConsole(debugBf);
	//

	printf("%s%s|%s|", TRIGGER_STRING, "M_PutSmFile", filename);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);

	return answer;
}

//download FW file

unsigned int M_PutFWFile(char * filename,char *risp) {
	unsigned int answer = 0;
	unsigned int locRetryCounter=0;
	do {
	    answer = 0;
	    TKERROR = 0; //default: no error
	    //trace
	    sprintf(debugBf, "%s%s|%s|", TRIGGER_STRING, "M_PutFWFile", filename);
	    directToConsole(debugBf);
	    //

	    printf("%s%s|%s|", TRIGGER_STRING, "M_PutFWFile", filename);
	    //fflush(stdout);

	    //QUI RICEVO STRINGA IN RISPOSTA!!!
	    //if ((TKERROR = readAndTestError(readBf)) == 0) {
		if ( !(getanswer(locbuf))) {
			sscanf(locbuf, "%s", risp);
			answer = 0;
			replacespace(risp);			
			//trace
			//sprintf(debugBf," >>> answer=%x\n", answer);
			sprintf(debugBf, " >>> answer=%s\n", risp);
			directToConsole(debugBf);
			//
			return answer;
	    }
	    ++locRetryCounter;
	} while ((TKERROR) && (locRetryCounter < maxRetryOnError));
/*
	


	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%s|", TRIGGER_STRING, "M_PutFWFile", filename);
	directToConsole(debugBf);
	//

	printf("%s%s|%s|", TRIGGER_STRING, "M_PutFWFile", filename);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	//sprintf(debugBf, " >>> answer=%x\n", answer);
	sprintf(debugBf, " >>> answer=%s\n", risp);
	directToConsole(debugBf);
*/
	return answer;
}



unsigned int M_SENDCANCONF(int type,char * filename) {
	unsigned int answer = 0;


	TKERROR = 0; //default: no error

	//trace
	sprintf(debugBf, "%s%s|%d|%s|", TRIGGER_STRING, "M_SENDCANCONF", type,filename);
	directToConsole(debugBf);
	//

	printf("%s%s|%d|%s|", TRIGGER_STRING, "M_SENDCANCONF", type,filename);
	getanswer(locbuf) ;
	sscanf(locbuf, "%d", &answer);

	//trace
	sprintf(debugBf, " >>> answer=%x\n", answer);
	directToConsole(debugBf);

	return answer;
}

char* rem_duble_slash(char *stringo,char *stringd)
{
int i,h,lenini;
char new[strlen(stringo)];
	
	lenini=strlen(stringo);

	for(i=0;i<(strlen(stringo));i++) new[i]='\0';
	h=0;
	new[0]=stringo[0];
	for(i=1;i<(strlen(stringo));i++) {
		if ( stringo[i] == '/') {
		 if ( stringo[i-1] == '/') h=h+1;
		}
		new[i-h]=stringo[i];
	}
	if ((lenini-i) !=0) {
		for(i=i;i<(lenini-i);i++) new[i]='\0';
	}
	strcpy(stringd,new);
	return(stringd) ;	
}

int hex_to_int(char c)
{

	if(c >=97) c=c-32;
	int first = c / 16 - 3;
	int second = c % 16;
	int result = first*10 + second;
	if(result > 9) result--;
	return result;

}

int hex_to_ascii(char c, char d)
{
        int high = hex_to_int(c) * 16;
        int low = hex_to_int(d);
        return high+low;
}


void text_to_hex(char *is_ascii_string, char *os_hex_string)
{
   int len,i;
   len=strlen(is_ascii_string);
	
   for(i = 0; i<len; i++){
		int pr;
		pr=is_ascii_string[i];
		if (pr<0) {
			pr=is_ascii_string[i];
			pr=pr & 0xFF;
			sprintf(os_hex_string+i*2, "%02X", pr);
		}else{
    		sprintf(os_hex_string+i*2, "%02X", is_ascii_string[i]);
		}
    }
}

void string_to_hex(char *buffer)
{
	 char hexbuffer[strlen(buffer)*2];
	 text_to_hex(buffer,hexbuffer);
	 strcpy(buffer,hexbuffer);
}

void hex_to_text(char *is_hex_string, char *os_ascii_string)
{
	int i;
	int length = strlen(is_hex_string);
    char buf = 0;
    for(i = 0; i < length; i++){
            if(i % 2 != 0){
					sprintf(os_ascii_string+(i/2), "%c", hex_to_ascii(buf, is_hex_string[i]));
            }else{
                    buf = is_hex_string[i];
            }
    }
}

void replacespace(char *msg)
{
	while ((msg=strchr(msg, 128))!=NULL){
		*msg = 32 ;
		msg++ ;
	};	
}

unsigned char atoh (unsigned char data)
{ if (data > '9') 
    { data += 9;
    }
    return (data &= 0x0F);
}

int64_t stringascii_to_hex(char *is_ascii_string)
{
   int len,i,num;
	 int64_t hex;
	 hex=0;
   len=strlen(is_ascii_string);
	 if (len>8) len=8;
	 //len=1;
   for(i = 0; i<len; i++){
		num=atoh(is_ascii_string[i]);
		hex=(hex << 4) + num;
   }
	 return hex;
}

