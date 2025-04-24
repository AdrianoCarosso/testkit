//   Copyright (c) 2009-  T.E.S.T. srl
//  ------------------------------------------------------------------------
//	TESTKIT definition -	MTS system
//  ------------------------------------------------------------------------
#ifndef _TESTKITCOMUNE_H
#define _TESTKITCOMUNE_H

#define YES 1
#define NO  0

//
//	======================================================================
// Porte di MTS:
//	======================================================================
//       uscite:      b0 b3 b4 b5 b6 b7 b1 b2
//	M_SetMapDigOut ("000003004005006007001002");
#define MOD1    	0x01
#define MOD4    	0x02
#define MOD5    	0x04
#define MOD6    	0x08
#define MOD7    	0x10
#define MOD8    	0x20
#define REL1    	0x40							
#define REL2    	0x80		
#define LED_R   	0x01					// per famiglia 10xx
#define LED_G   	0x40					//				"
#define BUZZ    	0x80					//				"
//         ingressi:   b0 b1 b2 b3 b4 b5 b6 b7 b8 b9b10b11b12b13b14b15
//		M_SetMapDigIn ("000001002003004005006007008009010011012013014015");
#define IN0     	0x01							
#define IN1     	0x02
#define IN2     	0x04
#define IN3     	0x08
#define IN4     	0x10
#define IN5     	0x20
#define IN6     	0x40
#define IN7     	0x80
#define SYNC    	0x0100
#define RING    	0x0200
#define D_RK    	0x0400
#define M_RBOOT 	0x0800
#define M_GSM_ON 	0x1000
#define M_MOVED   	0x2000
#define M_CHRGING 	0x4000
#define VEXT_OK    	0x8000
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv 	[da definire meglio !!!!!]  vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//    flag: in ordine dal 1° (F00) all'ultimo (F31).  Ho assunto che i FLAG sono la "Porta 5"
// da MTSdata.SMflags
//	M_SetMapVirtualIn ("000001002003004005006007008009010011012013014015016017018019020021022023024025026027028029030031");
#define GPS_ANT_  0x0001				// x GPS
#define GPS_ON    0x0002
#define LST_FIX   0x000C
#define RING_F    0x0020				// x Modem
#define FONIA_N   0x0040
#define GSM_REG   0x1000
#define GPRS_CN   0x2000
#define GPRS2_CN  0x0010				// x Famiglie MC55
#define DATI_ST_  0x4000
#define DATI_OK_  0x8000
#define G_ROAM    0x02000000
#define BRSGIO    0x0080				// x Posiione
#define FRM_MOV   0x0100
#define VELOCITA  0x0600
#define Var_TRACK 0x04000000
#define SPAZ_PER  0x78000000		// Per famiglia HC12
#define FRST_CYC  0x80000000		// x Stato
#define EXT_PWR_  0x0800
#define TRM_PR    0x01000000
#define BAT_LEV   0x18000000		// x Famiglie Litio
#define ACC1_S    0x20000000		// x Famiglie Accelerometro
#define ACC2_S    0x40000000		// x Famiglie Accelerometro
#define TAST_PR   0x00800000		// x Tastiera
#define TASTO     0x000F0000
#define COD_TST   0x00700000

//  Analog IN:		ordinale di un vettore
#define AN_0    1									
#define AN_1    2
#define AN_2    3
#define AN_3    4
#define AN_4    5
#define A_VEXT  6
#define A_VBAT  7									
#define AN_5    8
// #define Temp    9
//	#define CNSC    xx
//	#define ANTC    xx
//	#define SV_V    xx
//	#define CR_V    xx
//

// Per LED ROSSO TESTKIT
#define MSK_OFF 	0x0
#define MSK_PRES	0x000000FF
#define MSK_FW    	0x0000000F
#define MSK_TEST	0x0000FFFF

#define FRQ_OFF		10
#define FRQ_PRES    20
#define FRQ_FW      10
#define FRQ_TEST    30

//	Definizioni di costanti di servizio
//#define AllIn      0xFF
#define NOOUT      0x00
#define ALLOUT     0xFF

#define MAXSIZE   512			// Dimensione massima di stringa
#define NRMSIZE   128				// Dimensione normale di stringa
#define MINSIZE    32				// Dimensione minima di stringa
//
extern unsigned int TKTYPE; // Tipo di TestKit se 0 vecchio se 1 nuovo con SN maggiore di 901400100
extern uint32_t leveldebug ;		// Added from 2.13 (25/05/23) same name as Gdata into MTScu
extern char PathTK[NRMSIZE] ; 			// Path per gli app del TestKit (Wspace[]+app)
#endif
