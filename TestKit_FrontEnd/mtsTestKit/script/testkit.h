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


//   Copyright (c) 2009-  T.E.S.T. srl
//  ------------------------------------------------------------------------
//	TESTKIT definition -	TESTKITNEW
//  ------------------------------------------------------------------------
// ==================== Definizione delle Porte =====================
// Porte di T-K (Test-Kit):
//	======================================================================
//       uscite: 	a23b09b01b03b06b07b08b05d00d01d02d03d04d05d06d07e00e01e02e03e04e05e06e07e08e09e10e11e12e13e14e15b02b04b13a12a22
//  T_SetMapDigOut("023109101103106107108105300301302303304305306307400401402403404405406407408409410411412413414415102104113012022");  
//a23 b09 b01 b03 b06 b07 b08 b05
//023 109 101 103 106 107 108 105 
//d00 d01 d02 d03 d04 d05 d06 d07 
//300 301 302 303 304 305 306 307
//e00 e01 e02 e03 e04 e05 e06 e07
//400 401 402 403 404 405 406 407
//e08 e09 e10 e11 e12 e13 e14 e15
//408 409 410 411 412 413 414 415
//b02 b04 b13 a12 a22
//102 104 113 012 022
#define NEWCHN      	0x0000000007	// B01-B09-A23 : scelta canale analogico
#define NEWAN1ORAN2  	0x0000000008	// B03 Scelta tra primo gruppo An. o secondo (1=An1 0= An2 )
#define NEWAN_CORR_ 	0x0000000010	// B06 Analogici in corrente o Tensione (1=Tens 0=Corr) 
#define NEWEN_CNT_   	0x0000000020	// B07 Abilita counter (0=Abil 1=Disabil)
#define NEWCAN_EN_   	0x0000000040	// B08 Abilita CAN (0=Abil 1=Disabil)
#define NEWPON_     	0x0000000080	// B05 Da Alimentazione all'MTS (0=fornisce 1=non fornisce)
#define NEWTOD1     	0x0000000100	// D00 Uscita 1 del TK open-drain
#define NEWTOD2     	0x0000000200	// D01 Uscita 2 del TK open-drain
#define NEWTOD3     	0x0000000400	// D02 Uscita 3 del TK open-drain
#define NEWTOD4     	0x0000000800	// D03 Uscita 4 del TK open-drain
#define NEWTOD5     	0x0000001000	// D04 Uscita 5 del TK open-drain
#define NEWTOD6     	0x0000002000	// D05 Uscita 6 del TK open-drain
#define NEWTOD7     	0x0000004000	// D06 Uscita 7 del TK open-drain
#define NEWTRL2_    	0x0000008000	// D07 l'uscita Rele' è attivata con "0"
#define NEWTODALL       0X000000FF00	// Tutte Uscite Open-drain TK
#define NEWTOVFG1    	0x0000010000	// E00 Uscita 1 del TK VEXT o FLOAT o GND
#define NEWTOVFG2    	0x0000020000	// E01 Uscita 2 del TK VEXT o FLOAT o GND
#define NEWTOVFG3    	0x0000040000	// E02 Uscita 3 del TK VEXT o FLOAT o GND
#define NEWTOVFG4    	0x0000080000	// E03 Uscita 4 del TK VEXT o FLOAT o GND
#define NEWTOVFG5    	0x0000100000	// E04 Uscita 5 del TK VEXT o FLOAT o GND
#define NEWTOVFG6    	0x0000200000	// E05 Uscita 6 del TK VEXT o FLOAT o GND
#define NEWTOVFG7    	0x0000400000	// E06 Uscita 7 del TK VEXT o FLOAT o GND
#define NEWTOVFG8    	0x0000800000	// E07 Uscita 8 del TK VEXT o FLOAT o GND
#define NEWTOVFG9    	0x0001000000	// E08 Uscita 9 del TK VEXT o FLOAT o GND
#define NEWTOVFG10    	0x0002000000	// E09 Uscita 10 del TK VEXT o FLOAT o GND
#define NEWTOVFG11   	0x0004000000	// E10 Uscita 11 del TK VEXT o FLOAT o GND
#define NEWTOVFG12    	0x0008000000	// E11 Uscita 12 del TK VEXT o FLOAT o GND
#define NEWTOVFG13   	0x0010000000	// E12 Uscita 13 del TK VEXT o FLOAT o GND
#define NEWTOVFG14    	0x0020000000	// E13 Uscita 14 del TK VEXT o FLOAT o GND
#define NEWTOVFG15    	0x0040000000	// E14 Uscita 15 del TK VEXT o FLOAT o GND
#define NEWTOVFG16    	0x0080000000	// E15 Uscita 16 del TK VEXT o FLOAT o GND
#define NEWALLTOVFG     0x00FFFF0000	// Tutte le Uscite TK VEXT o FLOAT o GND
#define NEWPRES    		0x0100000000	// B02 Mette Presenza all'MTS (attivo 1)
#define NEWFLOAT_  	    0x0200000000	// B04 Mette Flotante le uscite che sono a VEXT (attivo a 0) [per quelle che sono GND per essere flottanti metterle a VEXT]
#define NEWCPU_OFF		0x0400000000	// B13 Spegne alimentatore 3.3v dell'MTS (attivo 1) [per 1 sec]
#define NEWRELECAN_		0x0800000000	// A12 Commuta l'alimentazione MTS verso DB9 CAN (0 su DB9 1 su DB37)
#define NEWHTLORTTL     0X1000000000	// A22 Commuta tra HTL e TTL* con 1 HTL con 0 TTL

#define NEWTK_STARTMASK ( NEWPRES | NEWPON_ | NEWFLOAT_| NEWALLTOVFG | NEWTODALL | NEWRELECAN_ | NEWHTLORTTL | NEWEN_CNT_ | NEWCAN_EN_  | NEWCPU_OFF | NEWAN_CORR_ | NEWAN1ORAN2 | NEWCHN )
#define NEWTK_STARTVAL  (     0   | NEWPON_ |      0   | NEWALLTOVFG | NEWTRL2_  | NEWRELECAN_ |     0       | NEWEN_CNT_ | NEWCAN_EN_  | NEWCPU_OFF | NEWAN_CORR_ | NEWAN1ORAN2 | NEWCHN )
																	//NEWTRL2_	 //NEWRELECAN
//#define NEWTK_STARTVAL2  (     0   | NEWPON_ |     0   | NEWALLTOVFG |    0      |NEWRELECAN |          0| NEWEN_CNT_ | NEWCAN_EN_  | NEWCPU_OFF | NEWAN_CORR_ | NEWAN1ORAN2 | NEWCHN )
// Definizione Pull o Float ingressi 
//			PUPDFL:	  L00L01L02L03L04L05L06L07L08L09L10L11L12L13L14L15
//  T_SetMapPUPDFLIn("900901902903904905906907908909910911912913914915");  

//#define NEWPUPC00       0xFFFFFFFF	// C00 Setta PullUp
//Mask Define
#define NEWPMAC00		0x00010001	// L00(C00) Setta PullUp
#define NEWPMAC01		0x00020002	// L01(C01) Setta PullUp
#define NEWPMAC02		0x00040004	// L02(C02) Setta PullUp
#define NEWPMAC03		0x00080008	// L03(C03) Setta PullUp
#define NEWPMAC04		0x00100010	// L04(C04) Setta PullUp
#define NEWPMAC05		0x00200020	// L05(C05) Setta PullUp
#define NEWPMAC06		0x00400040	// L06(C06) Setta PullUp
#define NEWPMAC07		0x00800080	// L07(C07) Setta PullUp
#define NEWPMAC08		0x01000100	// L08(C08) Setta PullUp
#define NEWPMAC09		0x02000200	// L01(C01) Setta PullUp
#define NEWPMAC10		0x04000400	// L02(C02) Setta PullUp
#define NEWPMAC11		0x08000800	// L03(C03) Setta PullUp
#define NEWPMAC12		0x10001000	// L04(C04) Setta PullUp
#define NEWPMAC13		0x20002000	// L05(C05) Setta PullUp
#define NEWPMAC14		0x40004000	// L06(C06) Setta PullUp
#define NEWPMAC15		0x80008000	// L07(C07) Setta PullUp


//Pull define
#define NEWPUPC00		0x00010001	// L00(C00) Setta PullUp
#define NEWPDWC00		0x00000001	// L00(C00) Setta PullDown
#define NEWFLOC00		0x00010000	// L00(C00) Setta Flottante
#define NEWPUPC01		0x00020002	// L01(C01) Setta PullUp
#define NEWPDWC01		0x00000002	// L01(C01) Setta PullDown
#define NEWFLOC01		0x00020000	// L01(C01) Setta Flottante
#define NEWPUPC02		0x00040004	// L02(C02) Setta PullUp
#define NEWPDWC02		0x00000004	// L02(C02) Setta PullDown
#define NEWFLOC02		0x00040000	// L02(C02) Setta Flottante
#define NEWPUPC03		0x00080008	// L03(C03) Setta PullUp
#define NEWPDWC03		0x00000008	// L03(C03) Setta PullDown
#define NEWFLOC03		0x00080000	// L03(C03) Setta Flottante
#define NEWPUPC04		0x00100010	// L04(C04) Setta PullUp
#define NEWPDWC04		0x00000010	// L04(C04) Setta PullDown
#define NEWFLOC04		0x00100000	// L04(C04) Setta Flottante
#define NEWPUPC05		0x00200020	// L05(C05) Setta PullUp
#define NEWPDWC05		0x00000020	// L05(C05) Setta PullDown
#define NEWFLOC05		0x00200000	// L05(C05) Setta Flottante
#define NEWPUPC06		0x00400040	// L06(C06) Setta PullUp
#define NEWPDWC06		0x00000040	// L06(C06) Setta PullDown
#define NEWFLOC06		0x00400000	// L06(C06) Setta Flottante
#define NEWPUPC07		0x00800080	// L07(C07) Setta PullUp
#define NEWPDWC07		0x00000080	// L07(C07) Setta PullDown
#define NEWFLOC07		0x00800000	// L07(C07) Setta Flottante
#define NEWPUPC08		0x01000100	// L08(C08) Setta PullUp
#define NEWPDWC08		0x00000100	// L08(C08) Setta PullDown
#define NEWFLOC08		0x01000000	// L08(C08) Setta Flottante
#define NEWPUPC09		0x02000200	// L09(C09) Setta PullUp
#define NEWPDWC09		0x00000200	// L09(C09) Setta PullDown
#define NEWFLOC09		0x02000000	// L09(C09) Setta Flottante
#define NEWPUPC10		0x04000400	// L10(C10) Setta PullUp
#define NEWPDWC10		0x00000400	// L10(C10) Setta PullDown
#define NEWFLOC10		0x04000000	// L10(C10) Setta Flottante
#define NEWPUPC11		0x08000800	// L11(C11) Setta PullUp
#define NEWPDWC11		0x00000800	// L11(C11) Setta PullDown
#define NEWFLOC11		0x08000000	// L11(C11) Setta Flottante
#define NEWPUPC12		0x10001000	// L12(C12) Setta PullUp
#define NEWPDWC12		0x00001000	// L12(C12) Setta PullDown
#define NEWFLOC12		0x10000000	// L12(C12) Setta Flottante
#define NEWPUPC13		0x20002000	// L13(C13) Setta PullUp
#define NEWPDWC13		0x00002000	// L13(C13) Setta PullDown
#define NEWFLOC13		0x20000000	// L13(C13) Setta Flottante
#define NEWPUPC14		0x40004000	// L14(C14) Setta PullUp
#define NEWPDWC14		0x00004000	// L14(C14) Setta PullDown
#define NEWFLOC14		0x40000000	// L14(C14) Setta Flottante
#define NEWPUPC15		0x80008000	// L15(C15) Setta PullUp
#define NEWPDWC15		0x00008000	// L15(C15) Setta PullDown
#define NEWFLOC15		0x80000000	// L15(C15) Setta Flottante



//   ingressi T-K:  C00C03C04C05C06C07C01C02C08C09C10C11C12C13C14C15B12A24A06B10C07C02C08 
//	 T_SetMapDigIn ("200203204205206207201202208209210211212213214215112024006110207202208");
//   ingressi: C00 C03 C04 C05 - C06 C07 C01 C02 - C08 C09 C10 C11 - C12 C13 C14 C15 - B12 A24 A06 B10 C07 C02 C08( NO per 2102 )
//   ingressi: C00 C01 C02 C03 - C04 C05 C06 C07 - C08 C09 C10 C11 - C12 C13 C14 C15 - B12 A24 A06 B10 C07 C02 C08( 2102 e 2205 )
				  //  0x00FFFF		// Primi 16 ingressi
#define NEWSH_C       	0x010000		// B12 Se corrente verso MTS oltre 1 A
#define NEWPR_RD_     	0x020000		// A24 Presenza (se su MTS c'è) da DB15
#define NEWOK_CNS_    	0x040000		// A06 Alim. su console (OK=1 NO=0)
#define NEWUNK_OK_    	0x080000		// B10 Test frequenza fonia (one kilohertz)
#define NEWEMLNK_IN2x 	0x100000		// C07	Emergency link Iput per famiglia 2x
#define NEWEMLNK_IN3x 	0x200000		// C02	Emergency link Iput per famiglia 3x
#define NEWEMLNK_IN3025	0x400000		// C08	Emergency link Iput per 3025

//  Analog IN:		ordinale di un vettore
#define NEWVREF    0			//	Su Motherboard di T-K il 1° Ingresso Analog. è impostato in tensione
#define NEWIREF    1			//	Su Motherboard di T-K il 2° Ingresso Analog. è impostato in corrente 
#define NEWTAN2    2			// Ingresso Analogico esterno
#define NEWTAN3    3			// Ingresso Analogico esterno
#define NEWTAN4    4			// Ingresso Analogico esterno
#define NEWTAN5    5			// Ingresso Analogico esterno
#define NEWTAN6    6			// Ingresso Analogico esterno
#define NEWHWTKVER 7			// Versione HW Motherboard T-K
#define NEWTAN7    8			// Ingresso Analogico esterno
#define NEWTAN8    9			// Ingresso Analogico esterno
#define NEWTAN9    10			// Ingresso Analogico esterno
#define NEWTAN10   11			// Ingresso Analogico esterno
#define NEWPWR_C   12 
#define NEWTVEXT   13			// Tensione esterna fornita all'MTS
#define NEWTBATCH  14			// Tensione carica/scarica Batteria
#define NEWFONIA   15			// Livello Fonia

//	Definizioni di costanti di servizio
#define NEWSETAN	0x10
#define NEWTO_CURR	0x00				
#define NEWTO_VOLT	0x10
//   Copyright (c) 2009-  T.E.S.T. srl
//  ------------------------------------------------------------------------
//	TESTKIT definition -	TESTKITOLD
//  ------------------------------------------------------------------------
// ==================== Definizione delle Porte =====================
// Porte di T-K (Test-Kit):
//	======================================================================
//       uscite: 	 a23b09b01b03a30a31b08b05d05b29d07d06c00c01c02c03d00d01d02d03b02c07b13
//  T_SetMapDigOut ("023109101103030031108105305129307306200201202203300301302303102207113");
////T_SetMapDigOut ("023109101103030031108105305129306307200201202203300301302303102207113");
#define OLDCHN      	0x07			// A23-B09-B01 : scelta canale
#define OLDAN_DIG_  	0x08			// B03 Scelta An. o Dig. (0=Dig, 1=An )
#define OLDAN_CORR_ 	0x10			// A30 Analogici in corrente (0 Corrente 1 Tensione)
#define OLDA_MASSA_ 	0x10			// A30 Dig. a Vext
#define OLDEN_CNT   	0x20			// A31 Abilita counter
#define OLDCAN_EN   	0x40			// B08 Abilita CAN
#define OLDPON_     	0x80			// B05 Toglie Alimentazione all'MTS (attivo 1)
#define OLDTOD1     	0x100			// D05 Uscita 1 del TK
#define OLDTRL2_    	0x200			// B29 l'uscita Rele' è attivata con "0"
#define OLDTOD5     	0x400			// D07 Uscita 5 del TK
#define OLDTOD4     	0x800			// D06 Uscita 4 del TK
#define OLDTK_MASKOUT  	0xF00			// Tutte le uscite del TK
#define OLDTIN_PUP  	0xFF000			// C00-C01-C02-C03-D00-D01-D02-D03 : Pull-Up del TK 
#define OLDPRES_    	0x100000		// B02 Toglie Presenza all'MTS (attivo 1)
#define OLDAL_OFF   	0x200000		// C07 Spegne alimentatore switching del TK (attivo 1)
#define OLDCPU_OFF		0x400000		// B13 Spegne alimentatore 3.3v dell'MTS (attivo 1) [per 1 sec]

#define OLDTK_STARTMASK ( OLDAL_OFF | OLDPRES_ | OLDTK_MASKOUT | OLDPON_ | OLDEN_CNT | OLDA_MASSA_ | OLDAN_DIG_ | OLDCHN )
#define OLDTK_STARTVAL  ( OLDAL_OFF | OLDPRES_ | OLDTRL2_      | OLDPON_ |         0 |           0 |          0 |   1 )

//   ingressi T-K:  C04C07D04D05D06D07C05C06B12A24A06C03D15C14 
//	T_SetMapDigIn ("204207304305306307205206112024006203315214");
//   ingressi: C04 C07 D04 D05 - D06 D07 C05 C06 - B12 A24 A06 C03 - D15 C14 ( NO per 2102 )
//   ingressi: C04 C05 C06 C07 - D04 D05 D06 D07 - B12 A24 A06 C03 - D15 C14 ( 2102 e 2205 )
					//0x00FF            // Primi 8 ingressi
#define OLDSH_C       0x0100			// B12 Se corrente verso MTS oltre 1 A
#define OLDPR_RD_     0x0200			// A24 Presenza (se su MTS c'è) da DB15
#define OLDOK_CNS_    0x0400			// A06 Alim. su console
#define OLDUNK_OK_    0x0800			// C03 Test frequenza fonia (one kilohertz)
#define OLDEMLNK_IN2x 0x1000			// D15	Emergency link Iput per famiglia 2x
#define OLDEMLNK_IN3x 0x2000			// C14	Emergency link Iput per famiglia 3x
/*									   / 2102
#define OLDTIN0    0x01			// c04      
#define OLDTIN3    0x02            // d05 / c07
#define OLDTIN4    0x04            // d06 / d04
#define OLDTIN5    0x08            // d07 / d05
#define OLDTIN6    0x10            // c05 / d06
#define OLDTIN7    0x20            // c06 / d07
#define OLDTIN1    0x40            // c07 / c05
#define OLDTIN2    0x80            // d04 / c06
*/
//  Analog IN:		ordinale di un vettore
#define OLDVREF    0			//	Su Motherboard di T-K il 1° Ingresso Analog. è impostato in tensione e con 
							//	il cablaggio di taverniti è connesso all'Uscita Analogica 6 (la 7ª)
#define OLDIREF    1			//	Su Motherboard di T-K il 2° Ingresso Analog. è impostato in corrente e con
	                 		//	il cablaggio di taverniti è connesso all'Uscita Analogica 7 (la 8ª)        
#define OLDTAN2    2
#define OLDTAN3    3
#define OLDTAN4    4
#define OLDTAN5    5
/*
#define OLDGYRO    6			//	Sempre a "0"
#define OLDOMux    7			//	Uscita del MUX Analogico
#define OLDTbat    8			//	Sempre a "0"  (non c'è  batteria)
*/
#define OLDTVEXT   9			//	Tensione esterna fornita all'MTS
/*
#define OLDPDwn    10			//	Fisso a "0" con Pull-Down
#define OLDCNS_C   11			//	Sempre a "0"	(non c'è Vext e collegamento con DB15)
*/
#define OLDPWR_C   12   
//#define OLDTTEMP   13
#define OLDFONIA   14
//#define OLDCR_V    15		//	Versione HW di Motherboard
//#define OLDTAccX   16        
//#define OLDTAccY   17        
//#define OLDTAccZ   18

//	Definizioni di costanti di servizio
#define SET_IN		0x18
#define IN_TOFLOAT	0x08				
#define IN_TOVEXT	0x18
#define OLDSETAN	0x18
#define OLDTO_CURR	0x08				
#define OLDTO_VOLT	0x18	


#endif
