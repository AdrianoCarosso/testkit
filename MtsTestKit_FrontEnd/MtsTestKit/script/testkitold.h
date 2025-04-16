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


