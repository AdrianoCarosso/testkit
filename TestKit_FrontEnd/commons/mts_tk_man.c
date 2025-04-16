//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//
// Aggiornato da rovera

#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <dirent.h>

#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

//#include <termios.h>
#include <fcntl.h>
//#include <sys/ioctl.h>
 
#ifdef SW_MTSCU
#include "MTScu.h"
#endif

#ifdef MTSTESTKIT
#include "MtsTestKit.h"
extern int ActCsec(void);
#endif

#include "MtsGTK.h"
#include "../commons/protocol.h"

#ifdef SW_MTSCU
extern void UpdateScreen(void) ;
FILE *sm_hex, *sm_bin, *sm_smk ;
#endif

static char buf_rx[PORT_MAX][SIZE_BUFFS] ;
static int rxnum[PORT_MAX] = {0,0}  ;


//static unsigned short LocTrans ;

// for MTS protocol
//static char LastString[MAX_STRING] ;
//static char OldString[MAX_STRING] ;
static char LastTemp[SIZE_BUFFS] ;
static int nrtemp = 0 ;
static int RxCan[2] ;
//static short Istext ;

    //#ifdef SW_MTSCU
#define MEMBASE 	0xE0000         //  Offset for HC12 code
#define MAXCODESIZE (256L*1024L)    // 0x40000
static unsigned char MemMirror[MAXCODESIZE] ;
//#endif
// #define RXMAXLEN 5000
// static char RxBuffer[RXMAXLEN] ;
// static int  RxBufLen = 0 ;

//static char LastChars[10] = {' ', ' ';
//static char tmp[

#define RX_IDLE		0
#define RX_WSTX		1
#define RX_RUN		2
#define RX_DDLE		3
#define RX_WCRCL	4
#define RX_WCRCH	5

static int RXstatus = RX_IDLE ;

#define STX			2           //#X# packet begin
#define ETX			3           //#X# packet end
#define EOT			4           //#X# transmission end
#define ENQ			5           //#X# enquiry
#define ACK			6           //#X# packet acknowledge
#define DLE			16          //#X# data link escape
#define NAK			21          //#X# packet not acknowledged



#define SMCANLEN	 8
#define SMLEN  		46
#define SMMAXPARLEN	64			// maximum parameter len

#pragma pack (1)

struct _SMACHINE {
unsigned long schedule ;                // Period in seconds between rule checks
unsigned short valtime ;                // Period in seconds rule must be verified before validation

// Checking rules

unsigned short phisin ;                 // Phisical inputs, 16 bit directly from the field
unsigned short phisout ;                // Phisical outputs, 16 bit directly to the field
unsigned long intflag ;                 // Internal flags, 32 bit generated directly by LU5, see after
unsigned long stsflag ;                 // Status flags, 32 bit handled in R/W

unsigned short msk_phisin ;             // Validity mask for phisical inputs
unsigned short msk_phisout ;            // Validity mask for phisical outputs
unsigned long msk_intflag ;             // Validity mask for internal flags
unsigned long msk_stsflag ;             // Validity mask for status flags

		// Related low level actions

unsigned short setphisout ;             // Phisical outputs, 16 bit directly to the field
unsigned long setstsflag ;              // Status flags, 32 bit handled in R/W

unsigned short msk_setphisout ;         // Validity mask for phisical outputs
unsigned long msk_setstsflag ;          // Validity mask for status flags

		// Related high level actions

unsigned short actcode ;                // Action code, see after
unsigned short actval ;                 // Action value, context dependent
}  ;

#pragma pack (4)



struct _SMACHINE *sm_item, sendsm_line ;

char sm_actpar[DEFAULTLEN] ;
int actparlen ;
unsigned long canflag, canmask ; 

unsigned short GetCRC_CCITT(unsigned char *pbytData, int intNumByte, unsigned short crc) ;
static void decode_mts_char(unsigned char c) ;

static void MTS_TransByCom(void) ;

int  DoCrcCheck(int lenbuf, unsigned char * buf, char crcl, char crch);
void MTS_AddPacket(int lu, int pkt, int field, char *data, int ldata ) ;
void MTS_SendTransaction(void) ;

void ParseInBuffer(struct _MTSVALS *pmc) ;
static void ParseCommandBuffer(struct _MTSVALS *pmc, int start) ;
static void ParseGpsBuffer(struct _MTSVALS *pmc, int start) ;
static void ParseRepBuffer(struct _MTSVALS *pmc, int start) ;
static void ParseDefaultBuffer(struct _MTSVALS *pmc, int start) ;
static void ParseCanBuffer(struct _MTSVALS *pmc, int start) ;


#ifdef SW_MTSCU
static void ParseBridge(struct _MTSVALS *pmc) ;

// Decode SM line
static void SmParseItem(char * csm) 
{
int i, j ;
int status ;
long v, vm, vt ;
char *p ;

	status = 0 ;
	
	while( (p=strchr(csm, 10))!=NULL){ // remove end LF
		*p = '\0' ;
	}
	
	while( (p=strchr(csm, 13))!=NULL){ // remove end LF
		*p = '\0' ;
	}
	
	while( (p=strchr(csm, '|'))!=NULL){ // ' but used as macro into MTS
		*p = 13 ;
	}
	
	while( (p=strchr(csm, 253))!=NULL){ // ' Alt 253 old "|") but used as macro into MTS
		*p = '|' ;
	}

	strcat(csm," ") ;
	i = strlen(csm) ;

	sm_actpar[0]='\0' ;
	actparlen = 0 ;
	
	for(j=0;j<i;j++){
		if (csm[j]=='@'){
			if (!status){
				sscanf(&csm[j+1],"%02ld", &v ) ;		// minute
				if (csm[j+3]=='*')						// hour
					v += 0xff00 ;
				else{
					sscanf(&csm[j+3],"%02ld", &vm ) ;
					v += (vm*0x100) ;
				}
					
				if (csm[j+5]=='*')						// day
					v += 0xff0000 ;
				else{
					sscanf(&csm[j+5],"%02ld", &vm ) ;
					v += (vm*0x10000) ;
				}
					
				if (csm[j+7]=='*')						// month
					v += 0x7f0000 ;
				else{
					sscanf(&csm[j+7],"%02ld", &vm ) ;
					v += (vm*0x1000000) ;
				}
				v|= 0x80000000 ;
				status++ ;
				j += 8 ;
			}
		}else if ( (csm[j]>='0') && (csm[j]<='9') ){
			
			if ((status==0) || (status==2) || (status==16) || (status==18) ){
				// ' wait for schedule, valtime, actcode, actval
				v = csm[j] - '0' ;
				status++ ;
			}else if ((status==1) || (status==3) || (status==17) || (status==19) ){
				// ' schedule, valtime, actcode, actval
				v *= 10 ;
				v += (csm[j]- '0') ;
			}else if ((status==4) || (status==6) || (status==8) || (status==10) || (status==12) || (status==14) ){
				vt = csm[j] - '0' ;
				if (vt>1) vt=1 ;
				v = vt ;
				vm = 1 ; 
				status++ ;
			}else if ((status==5) || (status==7) || (status==9) || (status==11) || (status==13) || (status==15) ){
				vt = csm[j] - '0' ;
				if (vt>1) vt=1 ;
				v *= 2 ;
				v += vt ;
				
				vm *= 2 ;
				vm += 1 ;
			}
		}else if (csm[j]=='.'){
			if ((status==4) || (status==6) || (status==8) || (status==10) || (status==12) || (status==14) ){
				v = 0 ;
				vm = 0 ;
				status++ ;
			}else if ((status==5) || (status==7) || (status==9) || (status==11) || (status==13) || (status==15) ){
				if (v < 0x40000000)
					v *= 2 ;
				else
					v = (v - 0x40000000) * 2 + 0x80000000 ;

				if (vm < 0x40000000) 
					vm *= 2 ;
				else
					vm = (vm - 0x40000000) * 2 + 0x80000000 ;
			}
		}else if ( (csm[j]>=0) && (csm[j]<=32)){
			switch(status){
				case 1: 		// schedule
				sendsm_line.schedule = v ;
				status = 2 ;
				break ;

				case 3:			// valtime
				sendsm_line.valtime = (v & 0xffff) ;
				status = 4 ;
				break ;

				case 5:			// phisin & msk_phisin
				sendsm_line.phisin = (v & 0xffff)  ;
				sendsm_line.msk_phisin = (vm & 0xffff) ;
				status = 6 ;
				break ;

				case 7:			// phisout & msk_phisout
				sendsm_line.phisout = (v & 0xffff)  ;
				sendsm_line.msk_phisout = (vm & 0xffff) ;
				status = 8 ;
				break ;

				case 9:			// intflag & msk_intflag
				sendsm_line.intflag = v   ;
				sendsm_line.msk_intflag = vm ;
				status = 10 ;
				break ;

				case 11:		// stsflag & msk_stsflag
				sendsm_line.stsflag = v   ;
				sendsm_line.msk_stsflag = vm  ;
				status = 12 ;
				break ;

				case 13:		// setphissendsm_lineout & msk_setphisout
				sendsm_line.setphisout = (v & 0xffff)  ;
				sendsm_line.msk_setphisout = (vm & 0xffff) ;
				status = 14 ;
				break ;

				case 15:		// setstsflag & msk_setstsflag
				sendsm_line.setstsflag = v  ;
				sendsm_line.msk_setstsflag = vm  ;
				status = 16 ;
				break ;

				case 17:		// actcode
				sendsm_line.actcode = (v & 0xffff)  ;
				status = 18 ;
				break ;

				case 19:		// actval
				sendsm_line.actval = (v & 0xffff)  ;
				while((csm[j]==' ') || (csm[j]=='\t')) j++ ;
#ifdef USE_MONITORING
				printf("\nACTPAR %d,%d <%s>\n", j,i, &csm[j]) ;
#endif
				if ((j+1)<i){
					actparlen = 0 ;
					csm[i]='\0';
					while(csm[j]){
						if (csm[j] != (const gchar) 0xC2)
							sm_actpar[actparlen++] = csm[j] ;

						j++ ;
					}
					sm_actpar[actparlen++] = '\0' ;
					//strcpy(sm_actpar, &csm[j]) ;
					//actparlen = strlen(sm_actpar) ;
				}
				status = 20 ;
				break ;
			}
		}
	}
// printf("\nDecoded <%s> (%d)\n", csm, status ) ;
// printf("\ninto %ld %d <%d %d %ld %ld> <%d %d %ld %ld> <%d %ld> <%d %ld> <%d %d>\n", 
// sendsm_line.schedule,sendsm_line.valtime,  
// sendsm_line.phisin,sendsm_line.phisout,sendsm_line.intflag,sendsm_line.stsflag,
// sendsm_line.msk_phisin,sendsm_line.msk_phisout,sendsm_line.msk_intflag,sendsm_line.msk_stsflag,
// sendsm_line.setphisout,sendsm_line.setstsflag, sendsm_line.msk_setphisout,sendsm_line.msk_setstsflag,
// sendsm_line.actcode, sendsm_line.actval ) ;

}
// #ifdef SW_MTSCU
static char * WriteMask(unsigned long val, unsigned long mask, int nbit, char *retval) {
int i, j  ;
    
    j = 0 ;
    retval[0] = '\0' ;
    for(i = nbit - 1; i>=0 ; i--){
        if (mask & (1 << i)) { 	      // enabled
            if (val & (1<<i))      		// value
                retval[j] = '1' ;
            else
                retval[j] = '0' ;
		}else
			retval[j] = '.' ;
		
		j++ ;
	}
	retval[j] = '\0' ;

	
	return(retval) ;

}

// #ifdef SW_MTSCU
void write_smline(FILE * outf, struct _SMACHINE  *locsm)
{
int j ;
char comment[DEF_STRING], c ;

	if (locsm->schedule >= 0){
		fprintf(outf, "%10ld", locsm->schedule ) ;
	}else{
		locsm->schedule = (locsm->schedule & 0x7FFFFFFF) ;
		fprintf(outf, "  @") ;
		for(j=0;j<25;j+=8){
			c = ((locsm->schedule>>j) & 0x3F) ;
			if (c>=63)
				fprintf(outf, "**") ;
			else
				fprintf(outf, "%2d", c ) ;
		}
	}
	
	fprintf(outf, "%8d  ", locsm->valtime ) ;
	//	Print #1, Tab(15); sm_item.valtime; Tab(22);
	
	fprintf(outf, "%s ", WriteMask(locsm->phisin, locsm->msk_phisin, 16, comment) ) ;
	fprintf(outf, "%s ", WriteMask(locsm->phisout, locsm->msk_phisout, 16, comment) ) ;
	
	fprintf(outf, "%s ", WriteMask(locsm->intflag, locsm->msk_intflag, 32, comment) );
	fprintf(outf, "%s ", WriteMask(locsm->stsflag, locsm->msk_stsflag, 32, comment) );
	
	fprintf(outf, "%s ", WriteMask(locsm->setphisout, locsm->msk_setphisout, 16, comment) );
	fprintf(outf, "%s ", WriteMask(locsm->setstsflag, locsm->msk_setstsflag, 32, comment) );
	
	// actpar
	//fprintf(l_fd," <%d %d> ", pmc->RxBuffer[fp + 1] , sizeof(struct _SMACHINE) ) ;
	
	fprintf(outf, "%6d %6d  %s\n",  locsm->actcode,  locsm->actval , sm_actpar ) ;

}

// #ifdef SW_MTSCU
void  decompile_sm(char *fname)
{
int i, sm_type, f_type, f_len, flag_write ;
FILE *f_in, *f_out ;
char aa[SIZE_BUFFS], *pp ;

	sm_type = 0 ;
	i = strlen(fname) ;
	if (!(strcmp(&fname[i-3], "smh"))) sm_type = 1 ;
	else if (!(strcmp(&fname[i-3], "smb"))) sm_type = 2 ;
	else if (!(strcmp(&fname[i-3], "smk"))) sm_type = 3 ;
	
	if (!sm_type){
		Popup("MTScu", "File extension unknow", GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
		return ;
	}
	
	f_in = fopen( fname, "r") ;
	if (f_in==NULL) return ;

	strcpy(aa, fname) ;
	i = strlen(aa) ;
	aa[i-3] = '\0' ;
	strcat( aa, "sm" ) ;
	f_out = fopen(aa, "w") ;
	
	while (!feof(f_in)){
		flag_write = 0 ;
		switch(sm_type){
			case 1:		// Hexadecimal ASCII
			if (fgets(aa, (SIZE_BUFFS-1), f_in)==NULL){
				fclose(f_out) ;
				fclose(f_in) ;
				return ;
			}
			if (aa[1]==2){
				sscanf(&aa[2],"%02x", &f_len ) ;
				f_len -= 4 ;
				if (f_len<sizeof(struct _SMACHINE)){ // actpar
					actparlen = f_len ;
					for(i=0;i<actparlen;i++){
						sscanf(&aa[10+(i*2)],"%02hhX", &sm_actpar[i]) ;
					}
					if (fgets(aa, (SIZE_BUFFS-1), f_in)==NULL){
						fclose(f_out) ;
						fclose(f_in) ;
						return ;
					}
					sscanf(&aa[2],"%02x", &f_len ) ;
					f_len -= 4 ;
					if (f_len<sizeof(struct _SMACHINE)){
						fclose(f_out) ;
						fclose(f_in) ;
						return ;
					}
					pp = (char*) & sendsm_line ;
					for(i=0;i<f_len;i++,pp++){
						sscanf(&aa[10+(i*2)],"%02hhX", pp ) ;
					}
				}
				
				flag_write = 1; 
			}
			break ;

			case 2:		// Binary Flash
			if (fread( aa, 1, 1, f_in)>0){
				f_type = aa[0] ; // SMver
				if (!(fread( (char*) &sendsm_line, 1, sizeof(struct _SMACHINE), f_in))){
					fclose(f_out) ;
					fclose(f_in) ;
					return ;
				}
				if (!(fread( aa, 1, 1, f_in))){
					fclose(f_out) ;
					fclose(f_in) ;
					return ;
				}
				actparlen = aa[0] ;
				
				if (f_type== 0xfd){ // Read also can fields
					if (!(fread( aa, 1, 4, f_in))){
						fclose(f_out) ;
						fclose(f_in) ;
						return ;
					}
					canflag =  *((short *)(&aa[0])) ;
					canmask =  *((short *)(&aa[2])) ;
				}
				if (actparlen){
					if (!(fread( sm_actpar, 1, actparlen, f_in))){
						fclose(f_out) ;
						fclose(f_in) ;
						return ;
					}
				}
				
				flag_write = 1 ;
			}
			break ;

			case 3:		// Binary protocol
			canmask = 0 ;
			if (fread( aa, 1, 2, f_in)>0){
				f_type = aa[0] ;
				f_len  = aa[1] ;
				
				fread( aa, 1, f_len, f_in) ;
				if (f_type==IDGPS_SMCAN){
					memcpy( &canmask, &aa[0], 4)  ;
					memcpy( &canflag, &aa[4], 4)  ;
					fprintf(f_out, "can %lu|%lu\n",  canmask,canflag ) ;
				}else{
					memcpy( &sendsm_line, &aa[0], sizeof(struct _SMACHINE) ) ;
				}
				flag_write = 1 ;
			}
			break ;
		}
		// Write textual SM
		if (flag_write) write_smline(f_out, &sendsm_line) ;
		
	}
	fprintf(f_out,"# End") ;
	fclose(f_out) ;
	fclose(f_in) ;
	
}
#endif
// #ifdef SW_MTSCU
// Send next SM packtes
void step_sm(char compile)
{
int nr_to_send, nr_send ;
char dummy[DEF_STRING] ;
char aa[SIZE_BUFFS] ;
int  f_type, f_len ;
#ifdef SW_MTSCU	
int i, j;	
short lshort, mychk ;
unsigned char smver ;
char comment[DEF_STRING], *pp ;
int32_t laddr ;
	
	if (compile){
		nr_to_send = 1000 ;
		fwrite("S003FFFFFE\n", 1, 11, sm_hex) ;
		smver = 0xfe ;
		laddr = 0x8000 ;
	}else
#endif
		nr_to_send = 4 ;
#ifdef USE_MONITORING
	printf("step_sm\n" ) ;
#endif
	
	// Add next 4 SM lines packet
	for (nr_send=0;nr_send<nr_to_send;){
		if (feof(Gdata.down_sm)){
			if (!compile){
				dummy[0] = 0xaa ;
				MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMSTART, dummy, 1) ;
			}
			fclose(Gdata.down_sm) ;
			Gdata.down_sm = NULL ;
			break ;
		}
#ifdef SW_MTSCU
//		if(Istext){
			if (fgets(aa, (SIZE_BUFFS-1), Gdata.down_sm)!=NULL){
				i = 0 ;
				while((aa[i]==' ') || (aa[i]=='\t')) i++ ;
				if ((aa[i]=='@') || ((aa[i]>='0') && (aa[i]<='9')) || (aa[i]=='C') || (aa[i]=='c')){
					if ((aa[i]=='C') || (aa[i]=='c')){
						while ((aa[i]!=' ') && (aa[i]!='\0')) i++ ;
						canmask = atoll(&aa[i]) ;
						while ((aa[i]!=' ') && (aa[i]!='\0')) i++ ;
						canflag = atoll(&aa[i]) ;
						smver = 0xfd ;
					}else{
						Gdata.item_sended++ ;
						nr_send++ ;
						SmParseItem(&aa[i]) ;		// Decode SM line
						if (!compile){
							if (canmask){ // Add CAN flag data
								memcpy(&dummy[0], &canmask, 4)  ;
								memcpy(&dummy[4], &canflag, 4)  ;
								MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMCAN, dummy, 8) ;
							}
							j = sizeof(struct _SMACHINE) ;
							memcpy(&dummy[0], &sendsm_line , j) ;
						
							if (actparlen){
								memcpy(&dummy[j], sm_actpar , actparlen) ;
								j += actparlen ;
							}
							MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMADD, dummy, j) ;
					
						}else{
						
						// sm_bin
							fwrite(&smver, 1, 1, sm_bin) ;
							fwrite(&sendsm_line, 1, sizeof(struct _SMACHINE), sm_bin) ;
							aa[0] = actparlen ;
							fwrite(&aa[0], 1, 1, sm_bin) ;
							if (smver == 0xfd){ // also can data (for RESEC)
								lshort = canflag ;
								fwrite(&lshort, 1, 2, sm_bin) ;
								lshort = canmask ;
								fwrite(&smver, 1, 2, sm_bin) ;
							}
							fwrite(&sm_actpar, 1, actparlen, sm_bin) ;
						
						// HEX (for AVR & 2003)  [1+3+1+46)
							// Par data
							if (actparlen){
								sprintf(aa, "%02hX%06lX", (1+3+actparlen), (laddr+0x40) ) ;
							
								for(j=0;j<actparlen;j++){
									sprintf(comment, "%02hX", (unsigned char) sm_actpar[j] ) ;
									strcat(aa, comment) ;
								}

								i = strlen(aa) ;
								mychk = 0 ;
								for(j=0; j<i; j+=2 ) {
									sscanf(&aa[j], "%02hX", &lshort) ;
									mychk += lshort ;
								}
								fwrite("S2", 1, 2, sm_hex ) ;
								fwrite(aa, 1, i, sm_hex ) ;
								sprintf(comment, "%02hX\n", (unsigned short) ((~lshort) & 0xff) ) ;
								fwrite(comment,1,3, sm_hex ) ;
							}

							j = 1 + 3 + 1 + sizeof(struct _SMACHINE)  ;
							if (smver == 0xfd) j += 4 ;
						
							sprintf(aa, "%02hX%06lX%02hX", j, laddr, smver ) ;
						
						
							pp = (char *) &sendsm_line ;
							for(j=0;j<sizeof(struct _SMACHINE);j++){
								sprintf(comment, "%02hX", (unsigned char) pp[j] ) ;
								strcat(aa, comment) ;
							}
							if (smver == 0xfd){
								lshort = canflag ;
								sprintf(comment, "%04hX", (unsigned short) lshort ) ;
								strcat(aa, comment) ;
								lshort = canmask ;
								sprintf(comment, "%04hX", (unsigned short) lshort ) ;
								strcat(aa, comment) ;
							}
						
							i = strlen(aa) ;
							mychk = 0 ;
							for(j=0; j<i; j+=2 ) {
								sscanf(&aa[j], "%02hX", &lshort) ;
								mychk += lshort ;
							}
							fwrite("S2", 1, 2, sm_hex ) ;
							fwrite(aa, 1, i, sm_hex ) ;
							sprintf(comment, "%02hX\n", (unsigned short) ((~lshort) & 0xff) ) ;
							fwrite(comment,1,3, sm_hex ) ;

							laddr = laddr + 0x80 ;


						// smk
							if (canmask){ // Add CAN flag data
								dummy[0] = IDGPS_SMCAN ;
								dummy[1] = 8 ;
								memcpy(&dummy[0+2], &canmask, 4)  ;
								memcpy(&dummy[4+2], &canflag, 4)  ;
								fwrite(dummy,1,10, sm_smk ) ;
							}
						
							j = sizeof(struct _SMACHINE) ;
							memcpy(&dummy[0+2], &sendsm_line , j) ;
						
							if (actparlen){
								memcpy(&dummy[j+2], sm_actpar , actparlen) ;
								j += actparlen ;
							}
							dummy[0] = IDGPS_SMADD ;
							dummy[1] = j ;
							fwrite(dummy,1,j+2, sm_smk ) ;
						}
					
					}
				
				}
			}
#else
////		}else{
//			fread(&aa[0], 1, 2, Gdata.down_sm);
//			i = aa[0] ;
//			j = aa[1] ;
//			fread(&dummy[0], 1, j, Gdata.down_sm);
//			MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, i, dummy, j) ;
////		}
			canmask = 0 ;
			if (fread( aa, 1, 2,  Gdata.down_sm)>0){
				f_type = aa[0] ;
				f_len  = aa[1] ;
				
				fread( aa, 1, f_len,  Gdata.down_sm) ;
				if (f_type==IDGPS_SMCAN){
#ifdef USE_MONITORING
					printf("can %lu|%lu\n",  canmask,canflag ) ;
#endif
					MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMCAN, aa, 8) ;
				}else{
					Gdata.item_sended++ ;
					nr_send++ ;
#ifdef USE_MONITORING
					printf("SM line %d\n",  f_len ) ;
#endif
					MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMADD, aa, f_len) ;
				}
			}
#endif
	}
	
	if (!compile){
		// Add Ack packet
		Gdata.confirmidx += ACK_ADDPKT ;
		memcpy(&dummy[0], &Gdata.confirmidx , 4) ;
		// Not use SrcDst because is a echo pkt
		MTS_AddPacket(((LU0CPU<<4)|Gdata.LU_src), PKT_GPSIO, IDGPS_ECHO, dummy, 4) ;
#ifdef SW_MTSCU
		sprintf(comment, "SM: sent %d of %d",  Gdata.item_sended, Gdata.total_item) ;
		StatusBar(1, comment) ;
#endif
		
		// Send data
		MTS_SendTransaction() ;
#ifdef MTSTESTKIT
		Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
#endif
	}
}

int start_fw(char *fname)
{
    int ll ;
//    int family_code, family_mts ;
    char ss[DEF_STRING], *p ;
    
    #ifdef USE_MONITORING
    printf("FW upgrade1 <%s>\n", fname) ;
    #endif
    // Open file
    Gdata.down_fw = fopen( fname, "r") ;
    if (Gdata.down_fw==NULL) return(1) ;
    
    // Now check MTS type (if needed)
    
    //if (!(Gdata.use64bit)){
        // Decode 'Gdata.systype' ("0", "1", "2", "3", "A", "W" )
        ll = strlen(fname) ;
        Gdata.systype = toupper(fname[ll-10]) ;  //_3.CBUG.hex
																								//_3.PROD.hex
    //}else{
    //    Gdata.systype = ' ';
    //}
    
    // Set total size of data
    fseek(Gdata.down_fw, 0L, SEEK_END) ;
    Gdata.total_item = ftell(Gdata.down_fw) ;
    
    //if (Gdata.systype==' '){ // MTS40-4004
    //    Gdata.total_item /= FW_SIZE_ITEM ;
    //}
    Gdata.item_sended = 0 ;
    fseek(Gdata.down_fw, 0L, SEEK_SET) ;
    
    //#ifdef FR_LINUX
    p = strrchr(fname, '/') ;
    //#endif
    //#ifdef FR_WIN32
    //p = strrchr(fname, '\\') ;
    //#endif
    if (p==NULL){
        #ifdef USE_MONITORING
        printf("FW upgrade dir not founded\n") ;
        #endif 
        p = fname ;
    }else
        p++ ;
    
    //strcpy(Gdata.upg_file, p) ;
//     strcpy(ss, p) ;
//     #ifdef USE_MONITORING
//     printf("FW upgrade: <%s> <%c>\n", Gdata.upg_file, Gdata.systype) ;
//     #endif
//     // Q2686 FW jam
//     if ( (Gdata.systype=='2') || (Gdata.systype=='3') || (Gdata.systype=='W')){ 
//         ss[5]='\0';
//         family_code = atoi(&ss[1]) ;
//         
//         
//         family_mts = Gdata.mts_sn/100000L ;
//         if (family_mts!=family_code){
//             switch(family_mts){
//                 case 2019:
//                 case 2106: family_mts = 2305 ; break ;
//                 case 2205: family_mts = 2102 ; break ;
//                 
//                 case 1016: family_mts = 1001 ; break ;
//                 
//                 case 2020: family_mts = 2015 ; break ;
//                 case 2122: family_mts = 2022 ; break ;
//             }
//         }
//         #ifdef USE_MONITORING
//         printf("FW upgrade2 <%c,%c> <%d,%d>\n", Gdata.systype, fname[ll-5], family_code, family_mts) ;
//         #endif
//         // If unknow MTS connected request start confirm
//         // or if SN not match request  start confirm
//         /*
//         if (family_code != family_mts){
//             Gdata.old_step = Gdata.new_step ;
//             sprintf(ss, "file per MTS %d, connesso MTS %d.\nContinuare ?" , family_code, family_mts) ;
//             Popup("MTScu",ss, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO) ;
//             // Wait user
//             while(Gdata.new_step == Gdata.old_step)
//                 MainLoop() ;
//             
//             if ((++Gdata.old_step) != Gdata.new_step){ // Answer NO
//                 fclose(Gdata.down_fw) ;
//                 Gdata.down_fw = NULL ;
//                 return(1) ;
//             }
//         }
//         */
//     }
    
    // If by RS232 will be change baud rate
    //if (Gdata.LU_src != LU8GPRS){
//         if ((Gdata.systype!='0') && (Gdata.systype!='1')){
//             #ifdef USE_MONITORING
//             printf("FW upgrade3 <%c>\n", Gdata.systype) ;
//             #endif
            ss[0] = 0 ; 
            ss[1] = 6 ; // 57600
            MTS_AddPacket( SrcDst(Gdata.LU_src), PKT_COMMAND, IDCMD_SET, ss, 2) ;
            MTS_SendTransaction() ;
            SLEEP(1000) ;			// 1 sec
            com_baud(MTS_current_PORT, baud_select(57600) ) ;
//             #ifdef USE_MONITORING
//             printf("FW upgrade4 <%c>\n", Gdata.systype) ;
//             #endif
        //}else{
//             Gdata.old_step = Gdata.new_step ;
//             Popup("MTScu","Invio codice ad HC12:\nVELOCE -> non bisogna interrompere l'operazione pena il blocco dell'MTS,\n\nLENTA: è possibile interrompere l'operazione.\n\nUso l'aggiornamento VELOCE ?",
//                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO) ;
//                   // Wait user
//                   while(Gdata.new_step == Gdata.old_step)
//                       MainLoop() ;
//                   
//                   if ((++Gdata.old_step) == Gdata.new_step){ // Answer YES
//                       return(1) ;
//                   }
//         }
    //}
    
    // Create 'Gdata.confirmidx'
    Gdata.confirmidx = random() ;								// this session
    Gdata.confirmidx &= ACK_RNDMASK ;							// highest byte for nr pkt
    if (!Gdata.confirmidx) Gdata.confirmidx = 3 ;				// never 0
    
    // Prepare for retry data
    //if (Gdata.LU_src != LU8GPRS)
        Gdata.tout_next = FIRST_COM_TIMEOUT ;
    //else
    //    Gdata.tout_next = FIRST_LAN_TIMEOUT ;
    Gdata.down_fw_tout = Gdata.tout_next * 2 ;
    
//     {
//         time_t now ;
//         struct timeb abc ;
//         char aa[DEF_STRING] ;
//         
//         now = time(NULL) ;
//         ftime(&abc) ;
//         now -=  (abc.timezone * 60L) ;
//         TimeDB(&now, aa) ;
//         sprintf(ss,"Start upgrade at %s\n", aa ) ;
//         Add_txt_mts(Scrn.boxtxt_mts, ss ) ;
//     }
    
    step_fw(0) ;
    
    return(0) ;
}

// #ifdef SW_MTSCU
// Send first SM packet
int start_sm(char *fname, char compile)
{
int f_type, f_len ;
#ifdef SW_MTSCU
int i; 
#endif
char aa[SIZE_BUFFS];//, bb[SIZE_BUFFS] ;
char dummy[5] ;

#ifdef USE_MONITORING
	printf("start_sm\n" ) ;
#endif
	
//Istext = 1 ;
//if (fname[strlen(fname) - 1]=='k') Istext=0 ; 
//printf("\n\nistext=%d fname<%s>\n", Istext,fname );
#ifdef SW_MTSCU
	Gdata.smver = 254 ;
#endif	
	// Open file
	Gdata.down_sm = fopen( fname, "r") ;
	if (Gdata.down_sm==NULL) return(1) ;

	// Count total line
	Gdata.total_item = 0 ;
	Gdata.item_sended = 0 ;
#ifdef SW_MTSCU
	i = 0 ;
#endif
	while (!feof(Gdata.down_sm)){
#ifdef SW_MTSCU	
			if (fgets(aa, (SIZE_BUFFS-1), Gdata.down_sm)!=NULL){
				i = 0 ;
				while((aa[i]==' ') ||(aa[i]=='\t')) i++ ;
				if ((aa[i]=='@') || ((aa[i]>='0') && (aa[i]<='9'))) Gdata.total_item++ ;
				if ((aa[i]=='C') || (aa[i]=='c')) Gdata.smver = 253 ;
			}
#else

//			i += 2 ;
//			if (fread(aa,1,2,Gdata.down_sm)>0){
//				i+=aa[1];
//				Gdata.total_item++ ;
//				fseek(Gdata.down_sm, i, SEEK_SET );
//			}
//		}
		while(fread( aa, 1, 2, Gdata.down_sm)>0){
			f_type = aa[0] ;
			f_len  = aa[1] ;
			
			fread( aa, 1, f_len, Gdata.down_sm) ;
			if (f_type!=IDGPS_SMCAN) Gdata.total_item++ ;
		}
#endif
	}

	// Close file
	fclose(Gdata.down_sm) ;
	
	// Reopen file
	Gdata.down_sm = fopen( fname, "r") ;
//#endif	
	if (!compile){
		// Create clear SM packet
		dummy[0] = 0xaa ;
		dummy[1] = 0x55 ;
		MTS_AddPacket( SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMCLEAR, dummy, 2) ;

		// Create 'Gdata.confirmidx'
		Gdata.confirmidx = random() ;								// this session
		Gdata.confirmidx &= ACK_RNDMASK ;							// highest byte for nr pkt
		if (!Gdata.confirmidx) Gdata.confirmidx = 3 ;				// never 0
#ifdef SW_MTSCU
	}else{
		strcpy(aa, fname) ;
		i = strlen(aa) ;
		aa[i-2] = '\0' ;
		sprintf(bb, "%ssmh", aa ) ;
		sm_hex = fopen(bb, "w") ;
		
		sprintf(bb, "%ssmb", aa ) ;
		sm_bin = fopen(bb, "w") ;
		
		sprintf(bb, "%ssmk", aa ) ;
		sm_smk = fopen(bb, "w") ;
#endif
	}
	
	step_sm(compile) ; // Add 4 SM lines , ack packet and send data
#ifdef SW_MTSCU
	if (compile){
		fclose(sm_hex) ;
		fclose(sm_bin) ;
		fclose(sm_smk) ;
	}
#endif
return(0);
}

// End SM download

#ifdef SW_MTSCU
// Start Target download
// Send Target packet
void step_tgt(void)
{
int  i, j, k ;
int tlat, tlong, family ;
unsigned int tdlat, tdlong ;
char aa[SIZE_BUFFS], dummy[20] ;

	tlat = tlong = tdlat = tdlong = family = 0 ;
	
	for(j=0;j<30;){
		if (feof(Gdata.down_tgt)){
			fclose(Gdata.down_tgt) ;
			Gdata.down_tgt = NULL ;
			break ;
		}
		if (fgets(aa, (SIZE_BUFFS-1), Gdata.down_tgt)!=NULL){
#ifdef USE_MONITORING
			printf("\nTarget len %d\n", strlen(aa)) ;
#endif
			i = 0 ;
			while((aa[i]==' ') || (aa[i]=='\t')) i++ ;
			if ((aa[i]>='0') && (aa[i]<='9')){
				family = 0 ;
				sscanf(aa, "%d,%d,%u,%u,%d", &tlat, &tlong, &tdlat, &tdlong, &family ) ;
#ifdef USE_MONITORING
				printf("\n\nTarget: %d,%d,%u,%u,%d\n", tlat, tlong, tdlat, tdlong, family ) ;
#endif
				memcpy(&dummy[0], &tlat ,4) ;
				memcpy(&dummy[4], &tlong ,4) ;
				memcpy(&dummy[8], &tdlat ,4) ;
				memcpy(&dummy[12], &tdlong ,4) ;
				switch(family){
					case 1: k = IDGPS_TGFAM1 ; break ;
					case 2: k = IDGPS_TGFAM2 ; break ;
					default: k = IDGPS_TGADD ; break ;
				}
				MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, k, dummy, 16) ;
				Gdata.item_sended++ ;
				j++ ;
			}
		}
	}

	Gdata.confirmidx += ACK_ADDPKT ;
	memcpy(&dummy[0], &Gdata.confirmidx , 4) ;
	MTS_AddPacket(((LU0CPU<<4)|Gdata.LU_src), PKT_GPSIO, IDGPS_ECHO, dummy, 4) ;

#ifdef SW_MTSCU
	sprintf(aa, "Target: sent %d of %d",  Gdata.item_sended, Gdata.total_item) ;
	StatusBar(1, aa) ;
#endif
	// Send data
	MTS_SendTransaction() ;

}

// #ifdef SW_MTSCU
void start_tgt(char *fname)
{
int i ;
char aa[SIZE_BUFFS] ;

	// Open file
	Gdata.down_tgt = fopen( fname, "r") ;
	if (Gdata.down_tgt==NULL) return ;
	
	// Count total line
	Gdata.total_item = 0 ;
	Gdata.item_sended = 0 ;
	while (!feof(Gdata.down_tgt)){
		if (fgets(aa, (SIZE_BUFFS-1), Gdata.down_tgt)!=NULL){
			i = 0 ;
			while((aa[i]==' ') ||(aa[i]=='\t')) i++ ;
			if ((aa[i]>='0') && (aa[i]<='9')) Gdata.total_item++ ;
		}

	}
	// Close file
	fclose(Gdata.down_tgt) ;
	
	// Reopen file
	Gdata.down_tgt = fopen( fname, "r") ;
	
	// Create 'Gdata.confirmidx'
	Gdata.confirmidx = random() ;								// this session
	Gdata.confirmidx &= ACK_RNDMASK ;							// highest byte for nr pkt
	if (!Gdata.confirmidx) Gdata.confirmidx = 3 ;				// never 0
	
	step_tgt() ; // Add target packets
	
}


// #ifdef SW_MTSCU
void fw_HC12(void)		// Raw send, cannot stop download
{
int i, j, ok=0 ; 
int baseaddr, flen ;
char c, aa[DEF_STRING], answer[DEF_STRING] , *pp ;
short sh ;
float s_perc ;
time_t t1  ;
unsigned char bcrcl, bcrch ;

	i = 0 ;
	t1 = time(NULL) ;
	Gdata.t_fwstart = t1 ;

	// 1.st: Connect (ComConnect(Gdata.systype)
	StatusBar( 1, "Connect ...") ;
        com_baud(MTS_current_PORT, baud_select(9600) ) ;	// flush
	strcpy(aa,"\rP8") ;
        com_write(MTS_current_PORT, 3, aa ) ; 
	j = 0 ;
	while ((time(NULL)-t1)<5){
#ifdef SW_MTSCU
		// Get user events
		while (gtk_events_pending ()){
				gtk_main_iteration ();
		}
#endif
		if (com_inlen(MTS_current_PORT)>0){
			com_read_char(MTS_current_PORT, &c) ;
			if ((c=='\n') || (c=='\r')){
				j=0 ;
			}else{
				answer[j++] = c ;
			}
			answer[j]='\0' ;
		}else{
			SLEEP(100) ;
		}
		
		if (j){
#ifdef USE_MONITORING_
	printf("\n\n1.st step RX:%s\n", answer) ;
#endif // USE_MONITORING
			if (!(strncmp(answer, "RTXCbug>", 8))) {
				ok = 1 ;
				break ;
			}
		}
	}
#ifdef USE_MONITORING
	printf("\n\n1.st step %d\n", ok) ;
#endif // USE_MONITORING
	if (!ok){
		Add_txt_mts(Scrn.boxtxt_mts, "L'MTS non risponde: ABORT\n") ;
		com_baud(MTS_current_PORT, baud_select(Gdata.baudrate) ) ;
		StatusBar( 1, "") ;
		return ;
	}


	// 2.nd: Identify (comidentify)
	ok = 0 ;
	StatusBar( 1, "Identify ...") ;
	
	com_baud(MTS_current_PORT, baud_select(9600) ) ;	// flush
	strcpy(aa,"\rl\r") ;
	com_write(MTS_current_PORT, 3, aa ) ; 
	SLEEP(100) ;
	
	com_baud(MTS_current_PORT, baud_select(9600) ) ;	// flush
	strcpy(aa,"S?\r") ;
	com_write(MTS_current_PORT,3, aa ) ; 
	t1 = time(NULL) ;

	c = 'x';
	while ((time(NULL)-t1)<5){
#ifdef SW_MTSCU
		// Get user events
		while (gtk_events_pending ()){
				gtk_main_iteration ();
		}
#endif
		if (com_inlen(MTS_current_PORT)>0){
			com_read_char(MTS_current_PORT, &c) ;
#ifdef USE_MONITORING
	printf("\n\n2.nd step RX 0x%x (%c)\n", c, c) ;
#endif // USE_MONITORING
			if (c==Gdata.systype){
				ok = 1 ;
				break ;
			}
		}else{
			SLEEP(10) ;
		}
	}
	
#ifdef USE_MONITORING
	printf("\n\n2.nd step %d\n", ok) ;
#endif // USE_MONITORING
		// if bad SysType (Gdata.systype!=)
	if (!ok){
		sprintf(answer, "Sistema su MTS '%c', file di tipo '%c': ABORT\n", c, Gdata.systype ) ;
		Add_txt_mts(Scrn.boxtxt_mts, answer) ;
		strcpy(aa,"S9\r") ;
		com_write(MTS_current_PORT, 3, aa ) ; 
		SLEEP(10) ;
		com_baud(MTS_current_PORT, baud_select(Gdata.baudrate) ) ;
		StatusBar( 1, "") ;
		return ;
	}
	
	// 3.th: Erase (ComErase)
	ok = 0 ;
	StatusBar( 1, "Erase ...") ;
	if (Gdata.systype=='0'){
		strcpy(aa,"SZ") ;
		com_write(MTS_current_PORT, 2, aa ) ; 
		SLEEP(10) ;
		com_baud(MTS_current_PORT, baud_select(38400) ) ;	// flush
	}else{		// systype = '1'
		strcpy(aa,"SF") ;
		com_write(MTS_current_PORT, 2, aa ) ; 
		SLEEP(10) ;
		com_baud(MTS_current_PORT, baud_select(115200) ) ;	// flush
		SLEEP(10) ;
		strcpy(aa,"SX") ;
		com_write(MTS_current_PORT, 2, aa ) ; 
	}
	c = 'x';
	t1 = time(NULL) ;
	while ((time(NULL)-t1)<10){
#ifdef SW_MTSCU
		// Get user events
		while (gtk_events_pending ()){
				gtk_main_iteration ();
		}
#endif
		if (com_inlen(MTS_current_PORT)>0){
			com_read_char(MTS_current_PORT, &c) ;
			if (c=='>'){
				ok = 1 ;
				break ;
			}
		}else{
			SLEEP(10) ;
		}
	}
#ifdef USE_MONITORING
	printf("\n\n3.nd step %d\n", ok) ;
#endif // USE_MONITORING
		// if bad SysType (Gdata.systype!=)
	if (!ok){
		sprintf(answer, "L'MTS non risponde a %d baud: ABORT\n", ((Gdata.systype=='0')? 38400:115200) ) ;
		Add_txt_mts(Scrn.boxtxt_mts, answer) ;
		com_baud(MTS_current_PORT, baud_select(Gdata.baudrate) ) ;
		StatusBar( 1, "") ;
		return ;
	}

	// 4.th: Loading (ComDownload)
	j = 0 ;
	memset(MemMirror, 0xff, MAXCODESIZE) ;
	while  (!(feof(Gdata.down_fw))){
		ok = 0 ;
		j++ ;
		Gdata.item_sended = ftell(Gdata.down_fw) ;
#ifdef SW_MTSCU
		if ((j & 0xf) == 0xf){
			s_perc = ((float)((Gdata.item_sended*100)/Gdata.total_item)) ;
			
			sprintf(answer, "FW: sended %.1f %%...",  s_perc ) ;
			StatusBar(1, answer) ;
			s_perc /= 100.0 ;
			StatusBar(2, &s_perc) ;
		}
#endif
		if (fgets(aa, (DEF_STRING-1), Gdata.down_fw)!=NULL){
			pp = strchr(aa, '\n') ;
			if (pp!=NULL) *pp = '\0' ;
			pp = strchr(aa, '\r') ;
			if (pp!=NULL) *pp = '\0' ;
			if (!strncmp(aa, "S2", 2)){
				
				com_write(MTS_current_PORT, strlen(aa), aa ) ;
				//bsent = bsent + Len(a$)
				//flen = Val("&h" & Mid$(a$, 3, 2)) - 4
				//baseaddr = Val("&h" & Mid$(a$, 5, 6)) - MemBase
				sscanf(&aa[2],"%02x%06x", &flen, &baseaddr ) ;
				baseaddr -= MEMBASE ;				// For HC12
				flen -= 4 ;
				
				for(i=0;i<flen; (i++,baseaddr++)){ // store code data
					sscanf(&aa[10+(i*2)],"%02hhx", &MemMirror[baseaddr] ) ;
				}
				
				t1 = time(NULL) ;
#ifdef USE_MONITORING_
	printf("\n\n%d Sended <%s>\n", (int) t1, aa) ;
#endif // USE_MONITORING
				c = 'x';
				while ((time(NULL)-t1)<5){
#ifdef SW_MTSCU
					// Get user events
					while (gtk_events_pending ()){
							gtk_main_iteration ();
					}
#endif
					if (com_inlen(MTS_current_PORT)>0){
						com_read_char(MTS_current_PORT, &c) ;
#ifdef USE_MONITORING_
	printf("\n\n%d Recv <0x%02x>\n", (int) (time(NULL)-t1), c) ;
#endif // USE_MONITORING
						if (c=='*'){
							ok = 1 ;
							break ;
						}
					}else{
						SLEEP(10) ;
					}
				}
				if (!ok){
					sprintf(answer, "L'MTS si é bloccato a %d baud: ABORT\n", ((Gdata.systype=='0')? 38400:115200) ) ;
					Add_txt_mts(Scrn.boxtxt_mts, answer) ;
					com_baud(MTS_current_PORT, baud_select(Gdata.baudrate) ) ;
					StatusBar( 1, "") ;
					return ;
				}
			}
		}
	}

	// CRC evaluation
	bcrcl = 0xFF ;
	bcrch = 0xFF ;
	for (baseaddr=0;baseaddr<MAXCODESIZE;baseaddr+=2){
		bcrcl ^= MemMirror[baseaddr] ;
		bcrch ^= MemMirror[baseaddr + 1] ;
	}
    // Right$("00" & Hex$(&HFF And (Not (&H6 + &HF + &HFF + &HCC + CInt(crcl) + CInt(crch)) )), 2)
	sh = 0xff & (~(0x6+0xf+0xff+0xcc+bcrcl+bcrch)) ;
    sprintf(aa,"S2060FFFCC%02x%02x%02x",  bcrcl, bcrch, sh ) ;
	com_write(MTS_current_PORT, strlen(aa), aa ) ;
    
#ifdef SW_MTSCU
	s_perc = 100.0 ;
	sprintf(answer, "FW: sended %.1f %%...",  s_perc ) ;
	StatusBar(1, answer) ;
	s_perc /= 100.0 ;
	StatusBar(2, &s_perc) ;
#endif
        
// #ifdef USE_MONITORING
// 	printf("\n\nUpgrade time: %lu\n", (time(NULL)-Gdata.t_fwstart) ) ; 
// #endif

	// 5.th: Reboot (ComReboot)
	SLEEP(200) ;
	strcpy(aa,"S9\r") ;
	com_write(MTS_current_PORT, 3, aa ) ; 
	SLEEP(10) ;
	fclose(Gdata.down_fw) ;
	Gdata.down_fw = NULL ;
	com_baud(MTS_current_PORT, baud_select(Gdata.baudrate) ) ;

	s_perc = 0.0 ;
	StatusBar(2, &s_perc) ;
	
}

#endif
// #ifdef SW_MTSCU
// Send next FW packtes
void step_fw(int retry)
{
int i ;
int pkt_step, nr_send ; // , last_pkt ;
int baseaddr, flen ;
char lu_dest, fdata ;
char aa[DEF_STRING], dummy[SIZE_BUFFS],ss[1] ;
//char comment[DEF_STRING] ;
unsigned char bcrcl, bcrch ;

#ifdef USE_MONITORING
	printf("\n\nStep FW: %d\n", retry) ;
#endif
	if (Gdata.systype=='A'){
		pkt_step = 15 ;
		lu_dest = LU13CAN ;
	}else if (Gdata.systype==' '){
		lu_dest = LU0CPU ;
		pkt_step = 5 ;
	}else{
		lu_dest = LU0CPU ;
		if (Gdata.LU_src != LU8GPRS)
			pkt_step = 25 ;
		else{
			//if (Gdata.useSmall)
			//	pkt_step = 5 ;
			//else
				pkt_step = 20 ;
		}
	}
	
	//last_pkt = 0 ;
	baseaddr = flen = 0 ;
	
	// If a retry reset file pointer
// 	if (Gdata.systype==' '){
// 		if (retry){
// 			fseek( Gdata.down_fw, (Gdata.item_old*FW_SIZE_ITEM), SEEK_SET ) ;
// 			Gdata.item_sended = Gdata.item_old ;
// 		}else
// 			Gdata.item_old = Gdata.item_sended ;
// 	}else{
		if (retry)
			fseek( Gdata.down_fw, Gdata.item_old, SEEK_SET ) ;
		else
			Gdata.item_old = ftell(Gdata.down_fw) ;
	//}
	
	for (nr_send=0;nr_send<pkt_step;){
		if (feof(Gdata.down_fw)){
			fclose(Gdata.down_fw) ;
			Gdata.down_fw = NULL ;
			if (Gdata.LU_src != LU8GPRS){
				//SLEEP(200) ;
				ss[0] = 0 ; 
        MTS_AddPacket( SrcDst(Gdata.LU_src), PKT_COMMAND, IDCMD_RESET, ss, 1) ;
        MTS_SendTransaction() ;
        SLEEP(1000) ;			// 1 sec
				com_baud(MTS_current_PORT, baud_select(9600) ) ;
			}
			if (!nr_send){
#ifdef SW_MTSCU
				float ff=0.0 ;
				StatusBar(1, "FW: sended 100 %") ;
				StatusBar(2, &ff) ;
#endif
// #ifdef USE_MONITORING
// 				printf("\n\nUpgrade time: %lu\n", (time(NULL)-Gdata.t_fwstart) ) ; 
// #endif
				return ; 
			}
			break ;
		}
		
// 		if (Gdata.systype==' '){ // MTS40-4004
// 			if (ftell(Gdata.down_fw)==0L){ // start
// 				Gdata.t_ack = time(NULL) ;
// 				dummy[0] = 0xfd ;
// 				// Timeout
// 				if (Gdata.LU_src != LU8GPRS){
// 					dummy[1] = 0x0a ;
// 					dummy[2] = 0x00 ;
// 				}else{
// 					dummy[1] = 0xff ;
// 					dummy[2] = 0x07 ;
// 				}
// 				MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_DCLR, dummy, 3) ;
// 				nr_send++ ;
// #ifdef USE_MONITORING
// 				printf("\n\nStart upg\n") ;
// #endif
// 			}
// 			
// 			dummy[0] = 0xff ;
// 			dummy[1] = (++Gdata.item_sended) & 0xff ;
// 			dummy[2] = (Gdata.item_sended>>8) & 0xff ;
// 			
// 			flen = fread(&dummy[3], 1, FW_SIZE_ITEM, Gdata.down_fw ) ;
// 			nr_send++ ;
// 			MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_DATA, dummy, (3+flen)) ;
// 			
// #ifdef USE_MONITORING
// 			printf("\n\nAdding %d\n", Gdata.item_sended) ;
// #endif
// 			if (flen<FW_SIZE_ITEM){ // Last packet
// 				if (Gdata.delayed){
// 					MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_UPGDELAY, dummy, 0) ;
// 				}
// 				// File name
// 				strcpy(dummy, Gdata.upg_file) ;
// 				flen = strlen(Gdata.upg_file) ;
// 				MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_PLUGNAME, dummy, flen) ;
// 				
// 				Gdata.item_sended = Gdata.total_item ;
// 				dummy[0] = 0xfd ;
// 				MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_PROG, dummy, 1) ;
// 				break ;
// 			}
// 			
// 		}else{		// By HEX
			if (fgets(aa, (DEF_STRING-1), Gdata.down_fw)!=NULL){
				Gdata.item_sended = ftell(Gdata.down_fw) ;
				if (aa[1]=='0'){
// 					Gdata.t_ack = time(NULL) ;
// #ifdef USE_MONITORING
// 					printf("\n\nGet Tstart %lu\n", Gdata.t_ack ) ;
// #endif
					memset(MemMirror, 0xff, MAXCODESIZE) ;
					if (Gdata.systype == '3')  // CORTEX
						dummy[0] = 0xBA ;
					else
						dummy[0] = 255 ;
					MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_DCLR, dummy, 1) ;
					nr_send++ ;

				}else if ((aa[1]=='1')||(aa[1]=='2')){
					if (aa[1]=='1'){
						sscanf(&aa[2],"%02x%04x", &flen, &baseaddr ) ;
						fdata = 8 ;
						flen -= 3 ;
						nr_send++ ;
					}else if (aa[1]=='2'){
						sscanf(&aa[2],"%02x%06x", &flen, &baseaddr ) ;
						fdata = 10 ;
						flen -= 4 ;
						nr_send++ ;
					}
					memcpy(dummy, &baseaddr, 4 ) ;
					if (baseaddr>MEMBASE) baseaddr -= MEMBASE ;				// For HC12
					for(i=0;i<flen; (i++,baseaddr++)){ // store code data
						sscanf(&aa[fdata+(i*2)],"%02hhx", &dummy[4+i] ) ;
						MemMirror[baseaddr] = dummy[4+i] ;
					}
					MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_DATA, dummy, (4+flen)) ;
				}else{ // Calculate CRC
					//last_pkt = 1 ;
					bcrcl = 0 ;
					bcrch = 0 ;
					for (baseaddr=0;baseaddr<MAXCODESIZE;baseaddr+=2){
						bcrcl ^= MemMirror[baseaddr] ;
						bcrch ^= MemMirror[baseaddr + 1] ;
					}
					if (Gdata.systype=='3') { // Cortex
						dummy[0] = bcrch ;
						dummy[1] = bcrcl ;
					}else{
						dummy[0] = bcrcl ;
						dummy[1] = bcrch ;
					}
#ifdef USE_MONITORING
					printf("\n\nFW: crcH 0x%x crcl 0x%x (delay:%d)\n", bcrch, bcrcl, Gdata.delayed) ;
#endif
                                        Gdata.bcrch=bcrch;
                                        Gdata.bcrcl=bcrcl;
					if (Gdata.delayed){
						MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_UPGDELAY, dummy, 0) ;
					}
					MTS_AddPacket( SrcDst(lu_dest), PKT_COMMAND, IDCMD_PROG, dummy, 2) ;
					Gdata.item_sended = Gdata.total_item ;
					break ;
				}
			}	// if (fgets(aa, (DEF_STRING-1), Gdata.down_fw)!=NULL){
		//}	// if (Gdata.systype==' ')
		
	}

	// Add Ack packet
	if (!retry) 	Gdata.confirmidx += ACK_ADDPKT ;
	memcpy(&dummy[0], &Gdata.confirmidx , 4) ;
// PROVATO A UNIFICARE	
// 	if (Gdata.LU_src != LU8GPRS){
// 		if ( (Gdata.systype==' ') || (Gdata.systype=='1') || (Gdata.systype=='2') ){ // MTS40-4004
// 			MTS_AddPacket(((LU0CPU<<4)|Gdata.LU_src), PKT_GPSIO, IDGPS_EXESM, dummy, 4) ;
// 		}else{
// 			MTS_AddPacket(((Gdata.LU_src<<4)|Gdata.LU_src), PKT_COMMAND, IDCMD_DIRECT, dummy, 4) ;
// 		}
// 	}else
// 		MTS_AddPacket(((LU0CPU<<4)|Gdata.LU_src), PKT_COMMAND, IDCMD_DIRECT, dummy, 4) ;
		MTS_AddPacket(((LU0CPU<<4)|Gdata.LU_src), PKT_GPSIO, IDGPS_ECHO, dummy, 4) ;

#ifdef USE_MONITORING
		printf("\n\nFW: %d (%d of %d)\n", (Gdata.confirmidx>>20), Gdata.item_sended, Gdata.total_item) ;
#endif
#ifdef SW_MTSCU
		{
			float s_perc = ((float)((Gdata.item_sended*1000)/Gdata.total_item)/10.0) ;
			
			sprintf(comment, "FW: sended %.1f %%...",  s_perc ) ;
			StatusBar(1, comment) ;
			s_perc /= 100.0 ;
			StatusBar(2, &s_perc) ;
		}
#endif
	
	// Send data
	MTS_SendTransaction() ;
}
// #ifdef SW_MTSCU
int sendoldcanconf(char *fname)
{
int i, ludest, nr_chnl, llbuf ;
FILE *ff ;
char aa[DEF_STRING], locbuf[DEF_STRING], dummy[DEF_STRING] ;
char chnldata[SIZE_BUFFS], *pp ;
char sm_csend ;
int32_t ch_addr ;
int  CANTiming, TimeSend ;
char CanMode, FromLU ;

	for(ludest=13; ludest<15;ludest++){
		ff = fopen( fname, "r") ;
		if (ff==NULL) return 1 ;

		sprintf(locbuf,"[LU%d]", ludest) ;
		llbuf = strlen(locbuf) ;
		
		sm_csend = 0 ;
		while (fgets(aa, (DEF_STRING-1), ff)!=NULL){
			pp = strchr(aa, '\n') ;
			if (pp!=NULL) *pp = '\0' ;
			pp = strchr(aa, '\r') ;
			if (pp!=NULL) *pp = '\0' ;
			
			for(i=0;i<strlen(aa);i++) aa[i] = toupper((unsigned char)aa[i]) ;

			if (!sm_csend){
				if (!(strncmp(aa, locbuf, llbuf))){ // Get LU
					sm_csend = 1 ;
					nr_chnl = 1 ;
					sprintf(locbuf,"ADDRESS%d=", nr_chnl) ;
					llbuf = strlen(locbuf) ;
				}
			}else{
				if ((aa[0]=='[') || (aa[0]=='\0')) {// next channel send channel data
					break ;
				}else{
					strcat(chnldata, aa) ; // Append data to conf buffer
					strcat(chnldata, ";") ;
				}
			}
		}
		fclose(ff) ;
		
		if (strlen(chnldata)){
			if ((pp=strstr(chnldata, "BUSTIMING="))==NULL){
				sprintf(locbuf,"No baud rate defined for LU%dCAN\n", ludest) ;
				Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
				break ;
			}
			pp += 10 ;
			CANTiming = atoi(pp) ;
			
			if ((pp=strstr(chnldata, "MODE="))==NULL){
				sprintf(locbuf,"No CAN mode defined for LU%dCAN\n", ludest) ;
				Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
				break ;
			}
			pp += 5 ;
			CanMode = atoi(pp) ;

			if ((pp=strstr(chnldata, "CONNECTION="))==NULL){
				sprintf(locbuf,"No destination data defined for LU%dCAN\n", ludest) ;
				Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
				break ;
			}
			pp += 11 ;
			FromLU = atoi(pp) ;

			if ((pp=strstr(chnldata, "SENDEVERY="))==NULL){
				sprintf(locbuf,"No send data timing defined for LU%dCAN\n", ludest) ;
				Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
				break ;
			}
			pp += 10 ;
			TimeSend = atoi(pp) ;

			if ((pp=strstr(chnldata, "USETRANS="))!=NULL){
				pp += 9 ;
				i = atoi(pp) ;
				if (i) CanMode |= 0x10 ;
			}

			for (nr_chnl=0;nr_chnl<32;nr_chnl++){
				sprintf(locbuf,"ADDRESS%d=", (nr_chnl+1)) ;
				llbuf = strlen(locbuf) ;
				if ((pp=strstr(chnldata, locbuf))==NULL) break ;
				pp += llbuf ;
				ch_addr = strtol(pp, NULL, 16) ;
				//10, 11, 12 address
				dummy[10+(4 *nr_chnl)] = (ch_addr>>16) & 0xff ;
				dummy[11+(4 *nr_chnl)] = (ch_addr>>8) & 0xff ;
				dummy[12+(4 *nr_chnl)] = ch_addr & 0xff ;

				sprintf(locbuf,"FILTER%d=", (nr_chnl+1)) ;
				llbuf = strlen(locbuf) ;			
				// 9 maskch_addr
				dummy[9+(4 *nr_chnl)] = 0xff ;
				if ((pp=strstr(chnldata, locbuf))!=NULL){
					pp += llbuf ;
					ch_addr = strtol(pp, NULL, 16) ;
					dummy[9+(4 *nr_chnl)] = ch_addr & 0xff ;
				}
			}
			
			dummy[0] = 0x03 ;		// Send CAN Conf
			dummy[1] = 0x01 ;		// sja_IER Receive interrupt enable
			switch(CANTiming){
			case 1000:
				dummy[2] = 0x00 ;
				dummy[3] = 0x14 ;
				break ;
			case 800:
				dummy[2] = 0x00 ;
				dummy[3] = 0x16 ;
				break ;
			case 500:
				dummy[2] = 0x00 ;
				dummy[3] = 0x1C ;
				break ;
			case 250:
				dummy[2] = 0x01 ;
				dummy[3] = 0x1C ;
				break ;
			case 125:
				dummy[2] = 0x03 ;
				dummy[3] = 0x1C ;
				break ;
			case 100:
				dummy[2] = 0x04 ;
				dummy[3] = 0x1C ;
				break ;
			case 50:
				dummy[2] = 0x09 ;
				dummy[3] = 0x1C ;
				break ;
			case 20:
				dummy[2] = 0x18 ;
				dummy[3] = 0x1C ;
				break ;
			default :
				sprintf(locbuf,"Bad baud rate (%d) defined for LU%dCAN\nUsed 10k\n", CANTiming, ludest) ;
				Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
			case 10:
				dummy[2] = 0x31 ;
				dummy[3] = 0x1C ;
				break ;
			}
		    dummy[4]= 0xDA;			// OCR

			dummy[5] = (ludest << 4) | FromLU  ;		// BSC parameter(pk_sd)
			dummy[6] = 0x1B ;		//FREE

		}
		
		if (nr_chnl){
			dummy[7] = CanMode ;
		}else{
			// old FS
			sprintf(locbuf,"exe rm mts_lu%d.conf", ludest) ;
			llbuf = strlen(locbuf) ;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, locbuf, llbuf ) ;
			sprintf(locbuf,"exe rm Lu%d*.conf", ludest) ;
			llbuf = strlen(locbuf) ;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, locbuf, llbuf ) ;
			
			// new FS
			sprintf(locbuf,"exe rm /mnt/work/log/can/mts_lu%d.conf", ludest) ;
			llbuf = strlen(locbuf) ;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, locbuf, llbuf ) ;
			sprintf(locbuf,"exe rm /mnt/work/log/can/Lu%d*.conf", ludest) ;
			llbuf = strlen(locbuf) ;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, locbuf, llbuf ) ;
			MTS_SendTransaction() ;
		}
		
		dummy[8] = nr_chnl ;
		
		// used as lenpkt
		ch_addr = (12 + (4 * nr_chnl)) ;
		dummy[ch_addr - 2] = 0xF1 ;
		dummy[ch_addr - 1] = 0x00 ;
		dummy[ch_addr] = 0x00 ;
		if (ludest==13)
			MTS_AddPacket(SrcDst(ludest), PKT_BINARY, IDBIN_CCANBUS, dummy, ch_addr ) ;  
		else
			MTS_AddPacket(SrcDst(ludest), PKT_BINARY, IDBIN_BCANBUS, dummy, ch_addr ) ;  

    	if (nr_chnl){	// SEND TIMING CONF
			// prepare packet
			dummy[0] = 0x02 ;					// Send CAN Conf
			dummy[1] = 0x01 ;					// 1 data for each transmission
			dummy[3] = (TimeSend >>8) & 0xff ;	// Each xx dsec
			dummy[2] = TimeSend & 0xff ;		// Each xx dsec
		
			dummy[4] = 0 ;						// FREE
		
			// to LU13 or LU14
			if (ludest==13)
				MTS_AddPacket(SrcDst(ludest), PKT_BINARY, IDBIN_CCANBUS, dummy, 5 ) ;  
			else
				MTS_AddPacket(SrcDst(ludest), PKT_BINARY, IDBIN_BCANBUS, dummy, 5 ) ;  
			
		}
		MTS_SendTransaction() ;
		
	}
	return 0;
}

// #ifdef SW_MTSCU
#define SM_CANC_END 0x10
int sendnewcanconf(char *fname)
{
int32_t canbaud ;
int i, nr_chnl, ltag[256] ;
char sm_csend, sm_next, lu_dest, flag_reset, can_sended ;
FILE *ff ;
char aa[DEF_STRING], *pp ;
char chnldata[SIZE_BUFFS] ;
char locbuf[DEF_STRING], dummy[DEF_STRING] ;
struct W_CAN_LINE can_line ;
float aaff ;
	
	sm_csend = flag_reset = can_sended = 0 ;  // global status
	nr_chnl = 0 ;
	
	for(i=0;i<255;i++) ltag[i] = 0 ;
	
#ifdef USE_MONITORING
		printf("\n\nSize CANCONF %ld\n", sizeof(struct W_CAN_LINE)) ;
#endif
		
	while (sm_csend!=SM_CANC_END){
		// Open file
		ff = fopen( fname, "r") ;
		if (ff==NULL) return 1;
		sm_next = 0 ;		// next status into channel
		
		CLEAR_MEM(&can_line, sizeof(struct W_CAN_LINE)) ;
		// default value for conf (diff from 0)
		can_line.ludest = 8 ;
		can_line.addr_mask = 0xffffffff ;
		can_line.k_mult = 1 ;
		can_line.bit_len = 63 ;
		chnldata[0] = ';' ;
		chnldata[1] = '\0' ;
		
		while (fgets(aa, (DEF_STRING-1), ff)!=NULL){
			pp = strchr(aa, '\n') ;
			if (pp!=NULL) *pp = '\0' ;
			pp = strchr(aa, '\r') ;
			if (pp!=NULL) *pp = '\0' ;
			
			if (!strlen(aa)) continue ;	// blank line
			if (aa[0]==';') continue ;	// comment line
			
			if (Gdata.localefloat == LOCALE_POINT){
				while ((pp = strchr(aa, CHR_COMMA))!=NULL) pp[0]=CHR_POINT ;
			}else{
				while ((pp = strchr(aa, CHR_POINT))!=NULL) pp[0]=CHR_COMMA ;
			}
			
			for(i=0;i<strlen(aa);i++) aa[i] = toupper((unsigned char)aa[i]) ;

#ifdef USE_MONITORING_
		printf("\n\nline %s (%d %d %d) <%s>\n", aa, sm_csend, sm_next, nr_chnl, chnldata) ;
#endif

			if (!sm_csend){ // configure baud
				if (!sm_next){
					if (!(strncmp(aa, "[NEWLU", 6))){ // Get LU
						sm_next = 1 ;
						lu_dest = atoi(&aa[6]) ;
						if ((lu_dest!=13) && (lu_dest!=14)) sm_next = 0 ;
					}
				}else if ((aa[0]=='[') || (aa[0]=='\0')){
					//sm_next = 0 ;
					break ;
				}else{
					strcat(chnldata, aa) ; // Append data to conf buffer
					strcat(chnldata, ";") ;
				}
				
			}else{			// configure channels
				if (!sm_next){
					sprintf(locbuf, "[NEWLU%d_DATA%d]", lu_dest, (nr_chnl+1) ) ;
					if (!(strcmp(aa, locbuf))){
						nr_chnl++ ;
						sm_next = 1 ;
					}
				}else if ((aa[0]=='[') || (aa[0]=='\0')) {// next channel send channel data
					break ;
				}else{
					strcat(chnldata, aa) ; // Append data to conf buffer
					strcat(chnldata, ";") ;
				}
				
			}
		}
		
		fclose(ff) ;
#ifdef USE_MONITORING
		printf("\n\nFIRST STEP (%d %d %d) <%s>\n\n", sm_csend, sm_next, nr_chnl, chnldata) ;
#endif

		if (sm_next){
			if (!sm_csend){
				if ((pp=strstr(chnldata, ";BAUD="))!=NULL){
					pp += 6 ;
					canbaud = atol(pp) ;
					dummy[0] = 0xaa ;
					dummy[1] = 0x55 ;
					
					dummy[2] = canbaud & 0xff ;
					dummy[3] = (canbaud>>8) & 0xff ;
					dummy[4] = (canbaud>>16) & 0xff ;
					dummy[5] = (canbaud>>24) & 0xff ;
					MTS_AddPacket(SrcDst(lu_dest), PKT_BINARY, IDBIN_CCCLEAR, dummy, 6) ;
					sm_csend++ ;
#ifdef USE_MONITORING
		printf("\n\nCAN %d (baud %d)\n", lu_dest, canbaud) ;
#endif
					//sm_next = 0 ;
					//break ;
				}
				if ((pp=strstr(chnldata, ";RESET="))!=NULL){
					pp += 7 ;
					flag_reset = atoi(pp) ;
				}
				
			}else{
#ifdef USE_MONITORING
				printf("\n\nCAN %d %d (%s)\n", lu_dest, nr_chnl, chnldata) ;
#endif
				// Decode conf buffer
			// TAG
				if ((pp=strstr(chnldata, ";TAG="))==NULL){
					sprintf(locbuf,"No Tag defined for channel %d\n", nr_chnl) ;
					Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
					break ;
				}
				pp+= 5 ;
				can_line.tag = atoi(pp) ;
				
				if (!can_line.tag){
					sprintf(locbuf,"Cannt use Tag 0\n") ;
					Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
					break ;
				}
				
				for(i=1;i<nr_chnl;i++){ // verify if Tag duplicated
					if (ltag[i]== can_line.tag){
						sprintf(locbuf,"Channel %d: same Tag of channel %d\n", nr_chnl, i) ;
						Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
						break ;
					}
				}
				ltag[nr_chnl] = can_line.tag ;
			// TIPO
				if ((pp=strstr(chnldata, ";TIPO="))==NULL){
					sprintf(locbuf,"No Type defined for channel %d\n", nr_chnl) ;
					Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
					break ;
				}
				pp+= 6 ;
				can_line.type = atoi(pp) ;
				if( (can_line.type<1) || (can_line.type>0x8f) ){
					sprintf(locbuf,"Type unknow (%d) for channel %d\n", can_line.type, nr_chnl) ;
					Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
					break ;
				}

			// INDIRIZZO
				if ((pp=strstr(chnldata, ";INDIRIZZO="))==NULL){
					sprintf(locbuf,"No address defined for channel %d\n", nr_chnl) ;
					Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
					break ;
				}
				pp+= 11 ;
				can_line.address = (int32_t) strtoll(pp, NULL, 16) ;
#ifdef USE_MONITORING
				printf("\n\nADDRESS [0x%x]\n", can_line.address) ;
#endif
				
			// MASKIND
				if ((pp=strstr(chnldata, ";MASKIND="))!=NULL){
					pp+= 9 ;
					can_line.addr_mask = strtol(pp, NULL, 16) ;
				}

			// CANSTANDARD
				if ((pp=strstr(chnldata, ";CANSTANDARD="))!=NULL){
					pp+= 13 ;
					i = atoi(pp) ;
					if (!i) can_line.addr_mask |= 0x80000000 ;
				}

			// INVIOOGNI
				if ((pp=strstr(chnldata, ";INVIOOGNI="))!=NULL){
					pp+= 11 ;
					can_line.send_time = atoi(pp) ;
				}

			// LUDEST
				if ((pp=strstr(chnldata, ";LUDEST="))!=NULL){
					pp+= 8 ;
					can_line.ludest = atoi(pp) ;
				}

			// INVERTIBYTE
				if ((pp=strstr(chnldata, ";INVERTIBYTE="))!=NULL){
					pp+= 13 ;
					i = atoi(pp) ;
					if (i>0) can_line.flags |=  WCAN_REVERSE ;
				}

			// TRANSAZIONATO
				if ((pp=strstr(chnldata, ";TRANSAZIONATO="))!=NULL){
					pp+= 15 ;
					i = atoi(pp) ;
					if (i>0) can_line.flags |=  WCAN_TRANS ;
				}

			// ESTERNOSOGLIE
				if ((pp=strstr(chnldata, ";ESTERNOSOGLIE="))!=NULL){
					pp+= 15 ;
					i = atoi(pp) ;
					if (i>0) can_line.flags |=  WCAN_OUTSIDE ;
				}

			// CHECKERRORE
				if ((pp=strstr(chnldata, ";CHECKERRORE="))!=NULL){
					pp+= 13 ;
					i = atoi(pp) ;
					if (i>0) can_line.flags |=  WCAN_BITERR ;
				}

			// COEFF
				if ((pp=strstr(chnldata, ";COEFF="))!=NULL){
					pp+= 7 ;
					can_line.k_mult= atof(pp) ;
				}

			// OFFSET
				if (can_line.type == WCAN_PHIST){
					if ((pp=strstr(chnldata, ";LETTURAOGNI="))!=NULL){
						pp+= 13 ;
						can_line.k_off = atol(pp) ;
					}
// 				}else if (can_line.type == WCAN_TIME){
// 					if ((pp=strstr(chnldata, "DIVTEMPO="))!=NULL){
// 						pp+= 7 ;
// 						can_line.k_off = atol(pp) ;
// 					}
				}else if (can_line.type == WCAN_MTIME){
					if ((pp=strstr(chnldata, ";SOGLIASUPERIORE="))!=NULL){
						pp+= 17 ;
						can_line.k_off = atol(pp) ;
					}
					if ((pp=strstr(chnldata, ";SOGLIAINFERIORE="))!=NULL){
						pp+= 17 ;
						can_line.k_mult= atof(pp) ;
					}
				}else{
					if ((pp=strstr(chnldata, ";OFFSET="))!=NULL){
						pp+= 8 ;
						can_line.k_off = atol(pp) ;
					}
				}

			// CANBIT
				if ( (can_line.type == WCAN_TIME)||(can_line.type == WCAN_TIMEALL) ||(can_line.type == WCAN_MTIME) ||(can_line.type == WCAN_DAYTIME)){
					if ((pp=strstr(chnldata, ";CANBIT="))!=NULL){
						pp+= 8 ;
						can_line.SMbit = atoi(pp) ;
						if (can_line.SMbit>16){
							sprintf(locbuf,"Bad CANBIT %d(Max 16)\n", can_line.SMbit) ;
							Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
							break ;
						}
					}
				}
				
			// SECANBIT
				if (!can_line.SMbit){
					if ((pp=strstr(chnldata, ";SECANBIT="))!=NULL){
						pp+= 10 ;
						can_line.SMbit = atoi(pp) ;
						if (can_line.SMbit>32){
							sprintf(locbuf,"Bad SECANBIT %d(Max 32)\n", can_line.SMbit) ;
							Add_txt_mts(Scrn.boxtxt_mts, locbuf ) ;
							break ;
						}
						can_line.SMbit |= 0x80 ;
					}
				}

				if ((can_line.type >= WCAN_MTIME) && (can_line.type<WCAN_IST4SLOT)){
					can_line.macro.can_bit.bitmask = can_line.macro.can_bit.bitvals = 0L ;
					if (can_line.type != WCAN_MTIME) can_line.SMbit = 0 ;
					if ((pp=strstr(chnldata, ";BITVAL="))!=NULL){
						pp+= 8 ;
						can_line.macro.can_bit.bitvals = strtoul(pp, NULL, 0) ;
					}
					if ((pp=strstr(chnldata, ";BITMASK="))!=NULL){
						pp+= 9 ;
						can_line.macro.can_bit.bitmask = strtoul(pp, NULL, 0) ;
					}
				}

				if (can_line.type != WCAN_POLLING){ 
				// PRIMOBITMASK
					if ((pp=strstr(chnldata, ";PRIMOBITMASK="))!=NULL){
						pp+= 14 ;
						can_line.fst_bit= atoi(pp) ;
					}
				// LUNGHEZZAMASK
					if ((pp=strstr(chnldata, ";LUNGHEZZAMASK="))!=NULL){
						pp+= 15 ;
						can_line.bit_len= atoi(pp) ;
					}
				}
				
				if ((can_line.type == WCAN_TIMEALL) || (can_line.type == WCAN_TIME) 
					||(can_line.type == WCAN_DAYTIME)
					|| (can_line.type == WCAN_COUNT)){
				// SOGLIAINFERIORE
					if ((pp=strstr(chnldata, ";SOGLIAINFERIORE="))!=NULL){
						pp+= 17 ;
						aaff = atof(pp) ;
// 						if (can_line.type == WCAN_TIME)
// 							can_line.macro.can_time.thr_low = aaff ; 
// 						else
							can_line.macro.can_time.thr_low = ((aaff / can_line.k_mult) - can_line.k_off) ;
					}
				// SOGLIASUPERIORE
					if ((pp=strstr(chnldata, ";SOGLIASUPERIORE="))!=NULL){
						pp+= 17 ;
						aaff = atof(pp) ;
// 						if (can_line.type == WCAN_TIME)
// 							can_line.macro.can_time.thr_low = aaff ; 
// 						else
							can_line.macro.can_time.thr_high = ((aaff / can_line.k_mult) - can_line.k_off) ;
					}
#ifdef USE_MONITORING
				if ((can_line.type == WCAN_TIMEALL) || (can_line.type == WCAN_TIME) || (can_line.type == WCAN_DAYTIME))
					printf("\n\nWCAN_TIME 0x%x 0x%x %d\n", can_line.macro.can_time.thr_low, 
						   can_line.macro.can_time.thr_high, can_line.bit_len) ;
#endif

				}else if (can_line.type == WCAN_SEND){
					if ((pp=strstr(chnldata, ";BITMASK="))!=NULL){
						pp+= 9 ;
						can_line.macro.can_time.thr_low = strtol(pp, NULL, 16) ;
					}
					if ((pp=strstr(chnldata, ";ERRMASK="))!=NULL){
						pp+= 9 ;
						can_line.macro.can_time.thr_high = strtol(pp, NULL, 16) ;
					}
					
					if ( (can_line.macro.can_time.thr_low & 0xFF000000) ||
							(can_line.macro.can_time.thr_high & 0xFF000000) )
						can_line.bit_len = 32 ;
					else if ( (can_line.macro.can_time.thr_low & 0xFF0000) ||
								(can_line.macro.can_time.thr_high & 0xFF0000) )
						can_line.bit_len = 24 ;
					else if ((can_line.macro.can_time.thr_low & 0xFF00) ||
								(can_line.macro.can_time.thr_high & 0xFF00) )
						can_line.bit_len = 16 ;
					else
						can_line.bit_len = 8 ;
					
#ifdef USE_MONITORING
				printf("\n\nWCAN_SEND 0x%x 0x%x %d\n", can_line.macro.can_time.thr_low, can_line.macro.can_time.thr_high, can_line.bit_len) ;
#endif
					can_line.send_time = 0 ;
				}

				if (can_line.type == WCAN_PSUM){
					if ((pp=strstr(chnldata, ";PRIMOBITMASK2="))!=NULL){
						pp+= 15 ;
						can_line.macro.can_sum.fst_bit1 = atoi(pp) ;
					}
					if ((pp=strstr(chnldata, ";LUNGHEZZAMASK2="))!=NULL){
						pp+= 16 ;
						can_line.macro.can_sum.bit_len1 = atoi(pp) ;
					}
					if ((pp=strstr(chnldata, ";OFFSET2="))!=NULL){
						pp+= 9 ;
						can_line.macro.can_sum.k_off1 = atoi(pp) ;
					}
				}
				i = 36 ;
				// Old version (now send always 36 bytes)
// 				i = 28 ;
// 				switch(can_line.type){
// 					case WCAN_DAYTIME:
// 					case WCAN_TIMEALL:
// 					case WCAN_TIME:
// 					case WCAN_COUNT:
// 					case WCAN_SEND:
// 						i = 36 ;
// 						break ;
// 					case WCAN_PSUM:
// 						i = 34 ;
// 						break ;
// 				}
				memcpy(dummy, &can_line, i) ;
				MTS_AddPacket(SrcDst(lu_dest), PKT_BINARY, IDBIN_CCADD, dummy, i) ;

#ifdef USE_MONITORING
				if (can_line.type == WCAN_MTIME)
					printf("\n\nWCAN_MTIME %d %d %g 0x%x 0x%x\n", can_line.SMbit, can_line.k_off, can_line.k_mult,
						   		can_line.macro.can_bit.bitvals, can_line.macro.can_bit.bitmask ) ;
				printf("\n\nPrepared CAN %d %d (%d) [0x%x]\n", lu_dest, nr_chnl, i,can_line.address) ;
				if (can_line.type == WCAN_DAYTIME)
					printf("\n\nWCAN_DAYTIME 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", dummy[28], dummy[29], dummy[30], dummy[31], dummy[32], dummy[33], dummy[34], dummy[35] ) ;
#endif
			}
			if(aa[0]!='[') sm_csend = SM_CANC_END ;
		}else
			sm_csend = SM_CANC_END ;
		
		if (Gdata.txbuflen>1000){
			MTS_SendTransaction() ;
			SLEEP(2500) ;
			can_sended = 1 ;
		}
	}
	
	if ((Gdata.txpacketnum) || (can_sended)){
		i = 0 ;
		dummy[0] = 0xaa ;
		dummy[1] = 0x55 ;
		if (flag_reset){
			i = 2 ;
#ifdef USE_MONITORING
			printf("\n\nSend RESET cmd\n") ;
#endif
		}
#ifdef USE_MONITORING
		else
			printf("\n\nNOSEND RESET cmd\n") ;
#endif
			
		MTS_AddPacket(SrcDst(lu_dest), PKT_BINARY, IDBIN_CCSTART, dummy, i) ;
		MTS_SendTransaction() ;
	}
	return 0;
}

#ifdef SW_MTSCU
void send_pars(char *fname)
{
int p1, p2, plen ;
char aa[SIZE_BUFFS], bb[SIZE_BUFFS] ;
char dummy[DEF_STRING], *pp ;
FILE *ff ;

	Gdata.smver = 254 ;
	
	// Open file
	ff = fopen( fname, "r") ;
	if (ff==NULL) return ;
	
#ifdef USE_MONITORING
	printf("\n\nFilePar <%s>\n", fname) ;
#endif
	while (fgets(aa, (DEF_STRING-1), ff)!=NULL){
		pp = strchr(aa, '\n') ;
		if (pp!=NULL) *pp = '\0' ;
		pp = strchr(aa, '\r') ;
		if (pp!=NULL) *pp = '\0' ;
		if ((aa[0]=='c') || (aa[0]=='C') ){
#ifdef USE_MONITORING
			printf("\n\nClear PARS\n") ;
#endif
			//StatusBar(1, "Clear Pars") ;
			Add_txt_mts(Scrn.boxtxt_mts,"Clear Pars\n") ;
			dummy[0] = 255 ;
			dummy[1] = 0xAA ;
			dummy[2] = 0x55 ;
			MTS_AddPacket( SrcDst(LU0CPU), PKT_COMMAND, IDCMD_RESET, dummy, 3) ;
			MTS_SendTransaction() ;
			SLEEP(1000) ;
			
		}else if ((aa[0]>='0') && (aa[0]<='9')){
			p1 = atoi(aa) ;
			dummy[0] = p1 & 0xff ;
			dummy[1] = (p1>>8) & 0xff ;
			pp = strchr(aa, '=') ;
			plen = 0 ;
			if (pp!=NULL){
				pp++ ;
				if (p1< 128) {				// short
					p2 = atoi(pp) ;
					dummy[2] = p2 & 0xff ;
					dummy[3] = (p2>>8) & 0xff ;
					plen = 2 ;
#ifdef USE_MONITORING
					printf("\nSend par <%s> [%d %d](%d, %d<%d)\n", aa, p1, p2, plen, Gdata.txbuflen, SIZE_BUFFS) ;
#endif
				}else if (p1< 224) {		// string
					plen = strlen(pp) ;
#ifdef USE_MONITORING
					printf("\nSend par <%s> [%d %s](%d, %d<%d)\n", aa, p1, pp, plen, Gdata.txbuflen, SIZE_BUFFS) ;
#endif
					memcpy(&dummy[2], pp, plen) ;
				}else{						// long
					p2 = atoi(pp) ;
					dummy[2] = p2 & 0xff ;
					dummy[3] = (p2>>8) & 0xff ;
					dummy[4] = (p2>>16) & 0xff ;
					dummy[5] = (p2>>24) & 0xff ;
					plen = 4 ;
#ifdef USE_MONITORING
					printf("\nSend par <%s> [%d %d](%d, %d<%d)\n", aa, p1, p2, plen, Gdata.txbuflen, SIZE_BUFFS) ;
#endif
				}
			}
			if (plen){
				sprintf(bb,"Send Par %s\n", aa ) ;
				//StatusBar(1, bb) ;
				Add_txt_mts(Scrn.boxtxt_mts, bb) ;

				MTS_AddPacket( SrcDst(LU0CPU), PKT_COMMAND, IDCMD_SET, dummy, plen+2) ;
			}
		}
	}
	fclose(ff) ;
	MTS_SendTransaction() ;
	
}	
// End Send pars
#endif // #ifdef SW_MTSCU

void ports_tick(int port)
{
int i,  oldrxnum ; //, ret_val ;

	//if ( (!Gdata.portopened) && (Gdata.LU_src!=LU8GPRS)) return ;
#ifdef SW_MTSCU
	if (!Gdata.LU_src) return ;
#endif
// #ifdef MTSTESTKIT
// 	if (!Gdata.portopened[port]) return ;
// #endif

	// Check if data
	if ((i=com_inlen(port))>0){
		i = com_read(port, (SIZE_BUFFS-rxnum[port]), &buf_rx[port][rxnum[port]] ) ;
		//buf_rx[port][rxnum[port]] = '\0' ;
		if (i>0){
			oldrxnum = rxnum[port] ;
			rxnum[port] += i ;
			
			switch(port){
#ifdef MTSTESTKIT
			int j, lencmd ;
			char cmd[MAX_STRING], buf[MAX_STRING], *p, *p1 ;
			//int32_t old_count2 ;
				
				case PORT_TK:
				p = NULL ;
				for (i=oldrxnum; i<rxnum[port];i++){
					if (buf_rx[port][i] == 10){	 // Search LF ( end message)
						p = &buf_rx[port][i] ;
						i++ ;
						//printf("\nLF at %d\n", i) ;
						break ;
					}
				}
				//p = strchr(buf_rx[port], 10) ; // Search LF ( end message)
				if (p!=NULL){
					// Move message into cmd and skip other into buf_rx
					p[0] = '\0' ;
					strcpy(cmd, buf_rx[port]) ;
					
					p++ ;
					//printf("\ni=%d, rxnum[port]=%d(%s)\n", i,rxnum[port], cmd) ;
					if ( i< rxnum[port] ){
						memcpy(buf, p, (rxnum[port]-i)) ;
						memcpy(buf_rx[port], buf, (rxnum[port]-i)) ;
						rxnum[port] -= i ;
					}else{
						rxnum[port] = 0 ;
						//buf_rx[port][rxnum[port]] = '\0' ;
					}
					
					//printf("\nRX:%s\n", cmd) ;
					// Now check message
					lencmd = strlen(cmd) ;
					switch(cmd[0]){
						case 'C':			// Data from TK_COM0: (Cx <HexData>)
						// C0=4142430d0a
						if (cmd[2]=='='){
							j = 0 ;
							switch(cmd[1]){  //0
								case '3': j++;
								case '2': j++;
								case '1': j++;
								case '0': break ;
								default: j=-1 ; break;
							}
							if(j>-1){
								i = ((ExtData.ComsRxNr[j]+(lencmd-3))<MAX_STRING)? 
														(lencmd-3): (MAX_STRING-ExtData.ComsRxNr[j]) ;
								memcpy(&(ExtData.ComsRxBuffer[j][ExtData.ComsRxNr[j]]), &cmd[3], i ) ;
								ExtData.ComsRxNr[j] += i ;
							}
						}
						break ;
						
						case 'D':			// Dump <Digital>;<Analog>;<Counter>
						//printf("\n%s\n",cmd) ;
		// D<CR> --> D=fc8dffdf,33002fdb,0080000d,000f00fe;2e5,1,0,0,0,0,0,0,5,f7,0,14,0,17d,1a1,0,1f6,1fa,248;0,686e,942
						//if (cmd[1]=='A'){ per non eseguire
						if (cmd[1]=='='){
							// Digitals
							p1 = &cmd[2] ;
							p = NULL ;
							ExtData.portA = strtoll(p1, &p, 16) ;
							p1 = p+1 ;
							ExtData.portB = strtoll(p1, &p, 16) ;
							p1 = p+1 ;
							ExtData.portC = strtoll(p1, &p, 16) ;
							p1 = p+1 ;
							ExtData.portD = strtoll(p1, &p, 16) ;
							if (Gdata.TKTYPE==1) {
								p1 = p+1 ;
								ExtData.portE = strtoll(p1, &p, 16) ;	
							}
							p++ ;
							p1 = p ;
#ifdef USE_MONITORING_
	if (!ExtData.digitalsUpdated) printf("\n------ UPDATE TK DIGITAL -----------\n") ;
#endif
							ExtData.digitalsUpdated = 1 ;
							// Analogs
							i=0 ;
							while(p[0]!=';'){ 
								ExtData.analogs[i++] = strtoll(p1, &p, 16) ;
								//printf("\n%s\n", p) ;
								p1 = p+1 ;
							}
							ExtData.analogsUpdated = 1 ;
							// Counters
							i = 0 ;
							//old_count2 = ExtData.counters[2] ;
							while(p1[0]){ 
								ExtData.counters[i++] = strtoll(p1, &p, 16) ;
								p1 = p+1 ;
							}
							// Update amperometer
							Val_amp.value = ExtData.analogs[12] ;
							Val_amp.value *= (3000. / 1023. / 3.) ;
							upd_amp() ;
							//printf("\ndelta_c %ld\n", (ExtData.counters[2]-old_count2) ) ;
	/*						// To verify
							printf("\nd=%08lx,%08lx,%08lx,%08lx;%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x;%lx,%lx,%lx\n",
												ExtData.portA,
												ExtData.portB,
												ExtData.portC,
												ExtData.portD,
												ExtData.analogs[0],
												ExtData.analogs[1],
												ExtData.analogs[2],
												ExtData.analogs[3],
												ExtData.analogs[4],
												ExtData.analogs[5],
												ExtData.analogs[6],
												ExtData.analogs[7],
												ExtData.analogs[8],
												ExtData.analogs[9],
												ExtData.analogs[10],
												ExtData.analogs[11],
												ExtData.analogs[12],
												ExtData.analogs[13],
												ExtData.analogs[14],
												ExtData.analogs[15],
												ExtData.analogs[16],
												ExtData.analogs[17],
												ExtData.analogs[18],
												ExtData.counters[0],
												ExtData.counters[1],
												ExtData.counters[2]) ;
					//Add_txt_mts(Scrn.boxtxt_mts, free_msg) ;*/
						}
						break ;
						
						case 'V':			// Version info
						// V<CR> --> V=1.00
						if (cmd[1]=='='){
							i = ( (lencmd-2)<DEF_STRING)? (lencmd-2) : (DEF_STRING-1) ;
							memcpy(ExtData.VerSn, &cmd[2], i ) ;
						}
						break ;
					}

					// To verify
#ifdef USE_MONITORING
					if (cmd[0]!='D'){
						cmd[lencmd] = '\n' ;
						cmd[lencmd+1] = '\0' ;
						printf("\n%s\n",cmd) ;
						Add_txt_answer(Scrn.boxtxt_answer, cmd) ;
					}
#endif
				}
				break ;
#endif // PORT_TK
				case PORT_MTSUSB:
				case PORT_MTS:
	// 			// To verify
	// 			buf_rx[port][rxnum[port]] = '\0' ;
	// 			Add_txt_mts(Scrn.boxtxt_mts, buf_rx[port] ) ;
	// 			rxnum[port] = 0 ;
	//			tmp[0] = '\0' ; 
					for (i=0; i<rxnum[port];i++)
					decode_mts_char(buf_rx[port][i]) ;
				rxnum[port] = 0 ;
				//if (strlen(tmp)) Add_txt_mts(Scrn.boxtxt_mts, tmp) ;
				break ;

				case PORT_UDP:
				rxnum[port] = 0 ;
				ParseInBuffer( &MtsData ) ;
				break ;
			}
		}
#ifdef SW_MTSCU
		else if (i<0){		// Data from Bridge
			if (port==PORT_UDP) ParseBridge( &MtsData ) ;
		}
#endif 
	}else if (port==MTS_current_PORT){
		if (nrtemp){
			//char momenta[MAX_STRING];
			LastTemp[nrtemp] = '\0' ;
			//sprintf(momenta,"PASSATODIQUI:%s\n",LastTemp);
			//Add_txt_mts(Scrn.boxtxt_mts, momenta) ;
			strcpy(MtsData.Dir_data, LastTemp) ;
			Add_txt_mts(Scrn.boxtxt_mts, LastTemp) ;
			CLEAR_MEM(&LastTemp, MAX_STRING ) ;
			nrtemp = 0 ;
		}
	}
}


void decode_mts_char(unsigned char c)
{
int i ;
static char crcl,  crch ;
char *p, *imeiStart, *imeiEnd ;
char locbuf[DEF_STRING] ;


//printf("\nDecoding 0x%02hhx\n", c ) ;
//printf("\n%hhu\n", c ) ;
	//lentmp = 0 ;
	switch(RXstatus){
		case RX_IDLE:
		if (c == DLE){
			RXstatus = RX_WSTX ;
		}else{
			switch(c){
				case  0 :
				case 13 :
				case 10 :
				//If (strlen(LastString) > 0)  strcpy(OldString, LastString) ;
				//strcpy(LastString, LastTemp) ;
				if (nrtemp){
					LastTemp[nrtemp++] = '\n' ;
					LastTemp[nrtemp] = '\0' ;
					//printf("\nGdata.RISP255(3)=%d\n", Gdata.RISP255) ;
					if (Gdata.RISP255==0) Add_txt_mts(Scrn.boxtxt_mts, LastTemp) ;
					strcpy(MtsData.Dir_data, LastTemp) ;
					CLEAR_MEM(&LastTemp, MAX_STRING ) ;
					nrtemp = 0 ;
				}
				break ;
				
				case  8 : // delete last char!
				if (nrtemp)	LastTemp[--nrtemp] ='\0' ;
				break ;
				
				//case 10:		break ;
				
				default:
				//printf("\nRXCHAR 0x%02x\n", c) ;
				// If LF or a ASCII char
				//if ( (c==10) || ((c>=32) && (c<=127)) ){
				if (  ((c>=32) && (c<=127)) ){
					LastTemp[nrtemp++] = c ;
					p = LastTemp ;
					while( (imeiStart = strchr(p,'I'))!=NULL){
						if (!strncmp(imeiStart,"IMEI=", 5)){ // If founded
							imeiStart += 5 ;
							break ;
						}
						p++ ;
					}
					if (imeiStart!=NULL){ // Search end
						p = imeiStart ;
						while( (imeiEnd = strchr(p,'I'))!=NULL){
							if (!strncmp(imeiEnd,"Tel=", 4)){ // If founded
								break ;
							}
							p++ ;
						}
						if (imeiEnd!=NULL){ // Get IMEI
							i = 0 ;
							for(p=imeiStart;p<imeiEnd;p++){
								MtsData.Imei[i++] = p[0] ;
								if (i>=50) break ;
							}
							MtsData.Imei[i] = '\0' ;
						}
					}
				}else{
					if (  ((c>=127) && (c<=255)) ){
						LastTemp[nrtemp++] = c ;
					}else{
						sprintf(locbuf, "$%02X", c) ;
						for(i=0;i<strlen(locbuf);i++) LastTemp[nrtemp++] = locbuf[i] ;
					}					
					
				}			
				break ;
			}
		}
		break ;
		
		case RX_WSTX :
		if (c == STX){
			MtsData.RxBufLen = 0 ;
			crcl = crch = 0 ;
			RXstatus = RX_RUN ;
		}else{
			printf("DLE> %d\n",c);
			if (c==ACK ) MtsData.setok=c;
			RXstatus = RX_IDLE ;
		}
		break ;

		case RX_RUN :
		if (c == DLE){
			RXstatus = RX_DDLE ;
		}else{
			MtsData.RxBuffer[MtsData.RxBufLen++] = c ;
			if (MtsData.RxBufLen >= RXMAXLEN) MtsData.RxBufLen-- ;
		}
		break ;
		
		case RX_DDLE :
		if (c == DLE){
			MtsData.RxBuffer[MtsData.RxBufLen++] = c ;
			if (MtsData.RxBufLen >= RXMAXLEN) MtsData.RxBufLen-- ;
			RXstatus = RX_RUN ;
		}else if (c == ETX){
			RXstatus = RX_WCRCL ;
		}else{
			RXstatus = RX_IDLE ;
		}
		break ;
		
		case RX_WCRCL :
		crcl = c ;
		RXstatus = RX_WCRCH ;
		break ;
		
		case RX_WCRCH :
		crch = c ;
		if (DoCrcCheck(MtsData.RxBufLen, MtsData.RxBuffer, crcl, crch) ){
			//skipIncrememt = False      '#X#default
			ParseInBuffer( &MtsData ) ;
// 			if (!Gdata.skipIncrememt){				//'#X#skip increment on async packet rx
// 				switch(Gdata.lastSlaveCommandId){
// 					case KSLAVECMD_NONE:
// 					case KSLAVECMD_DELAY:
// 					case KSLAVECMD_INPUTBOX:
// 					case KSLAVECMD_MSGBOX:
// 					break ;
// 						
// 					default:
// 					if ((Gdata.lastSlaveCommandId & 0x1) == 0)
// 						Gdata.lastSlaveCommandId++ ;     // passa a stato di generazione risposta
// 					break ;
// 				}
// 			}
		}else{
#ifdef USE_MONITORING
			printf("\nMTS: Crc Error\n") ;
#endif
		}
		RXstatus = RX_IDLE ;
		break ;

		default:
		RXstatus = RX_IDLE ;
		break ;
		
	}
}

//*****************************************************************************
// OUTPUT FUNCTION
int SrcDst(int lu)
{
	return( (Gdata.LU_src<<4) | lu) ;
}

void MTS_AddPacket(int lu, int pkt_type, int field, char *data, int ldata )
{
int i ;

	// Packet
// 	if (!(lu & 0xf0))
// 		Gdata.trans_buff[5+Gdata.txbuflen+0] = (Gdata.LU_src<<4) | lu ;	// Src_dst
// 	else
	Gdata.trans_buff[5+Gdata.txbuflen+0] = lu ;	// Src_dst
		
	Gdata.trans_buff[5+Gdata.txbuflen+1] = pkt_type + Gdata.pkt_offset ;					// Packet type
	Gdata.trans_buff[5+Gdata.txbuflen+2] = (7+ldata) & 0xff ;         // packet length (low byte)
	Gdata.trans_buff[5+Gdata.txbuflen+3] = ((7+ldata)>>8) & 0xff ;	// packet length (high byte)
	Gdata.trans_buff[5+Gdata.txbuflen+4] = 1 ;						// 1 field
	Gdata.trans_buff[5+Gdata.txbuflen+5] = field ;					// field type
	Gdata.trans_buff[5+Gdata.txbuflen+6] = ldata ;					// field length
	if (ldata){
		for(i=0;i<ldata;i++){
			Gdata.trans_buff[5+Gdata.txbuflen+7+i] = data[i] ;
		}

		//memcpy( &Gdata.trans_buff[5+Gdata.txbuflen+7], data, ldata ) ;
	}
	// Update transaction header data
	Gdata.txbuflen += (7+ldata) ;
	Gdata.txpacketnum++ ;
	
}

void MTS_SendTransaction(void)
{

	Gdata.txbuflen += 5 ;
	
	// Build transaction and send it
	Gdata.trans_buff[0] = Gdata.transnum & 0xff ;		// transaction number low
	Gdata.trans_buff[1] = (Gdata.transnum>>8) & 0xff ;	// transaction number high
	Gdata.trans_buff[2] = Gdata.txbuflen & 0xff ;		// transaction length low byte
	Gdata.trans_buff[3] = (Gdata.txbuflen>>8) & 0xff ;	// transaction length high byte
	Gdata.trans_buff[4] = Gdata.txpacketnum ;			// packets number
	
	
	// Check if to send by RS232 or GPRS
	if (Gdata.LU_src==LU8GPRS){
		MTS_TransByGprs() ;
	}else{
		MTS_TransByCom() ;
	}
	Gdata.txpacketnum = 0 ;
	Gdata.txbuflen = 0 ;
	Gdata.transnum = 0 ;
}

void MTS_TransByCom(void)
{
int i, j ;
unsigned short crc ;
char out_buf[SIZE_BUFFS] ;

	crc = GetCRC_CCITT( (unsigned char*) Gdata.trans_buff, Gdata.txbuflen, 0) ;				// crc

	out_buf[0] = DLE ; 		// DLE
	out_buf[1] = STX ; 		// STX

	j = 2 ;
	for(i=0; i<Gdata.txbuflen;i++){
		out_buf[j++] = Gdata.trans_buff[i] ;
		if (Gdata.trans_buff[i]==DLE)
			out_buf[j++] = Gdata.trans_buff[i] ;
//printf("\n[%x]",(out_buf[j-1] & 0xff) ) ;

	}
//printf("\n") ;
    
	out_buf[j++] = DLE ; 		// DLE
	out_buf[j++] = ETX ; 		// ETX
	    
	out_buf[j++] = crc & 0xff ; 		// CRC low
	out_buf[j++] = (crc>>8) & 0xff ; 	// CRC high

	com_write(MTS_current_PORT, j, out_buf) ;
#ifdef USE_MONITORING
	printf("\nSended Trans by COM (%d->%d, %d)\n", j, Gdata.txbuflen, Gdata.txpacketnum) ;
#endif
	
	
}

//*****************************************************************************
// INPUT DECODE FUNCTION

// *********************************************************************
//
//      The following table contains the crc-16 calculation for each of
//      the possible 256 characters.  The polinomial used is 0x1021,
//      often called the crc-ccitt. (2^16 + 2^12 + 2^5 + 1)
//

static const unsigned int uintCRC16Table[256] = {
		0x78f0,0xf1e1,0x6ad3,0xe3c2,0x5cb6,0xd5a7,0x4e95,0xc784,
		0x307c,0xb96d,0x225f,0xab4e,0x143a,0x9d2b,0x0619,0x8f08,
		0xf9e0,0x70f1,0xebc3,0x62d2,0xdda6,0x54b7,0xcf85,0x4694,
		0xb16c,0x387d,0xa34f,0x2a5e,0x952a,0x1c3b,0x8709,0x0e18,
		0x7ad1,0xf3c0,0x68f2,0xe1e3,0x5e97,0xd786,0x4cb4,0xc5a5,
		0x325d,0xbb4c,0x207e,0xa96f,0x161b,0x9f0a,0x0438,0x8d29,
		0xfbc1,0x72d0,0xe9e2,0x60f3,0xdf87,0x5696,0xcda4,0x44b5,
		0xb34d,0x3a5c,0xa16e,0x287f,0x970b,0x1e1a,0x8528,0x0c39,
		0x7cb2,0xf5a3,0x6e91,0xe780,0x58f4,0xd1e5,0x4ad7,0xc3c6,
		0x343e,0xbd2f,0x261d,0xaf0c,0x1078,0x9969,0x025b,0x8b4a,
		0xfda2,0x74b3,0xef81,0x6690,0xd9e4,0x50f5,0xcbc7,0x42d6,
		0xb52e,0x3c3f,0xa70d,0x2e1c,0x9168,0x1879,0x834b,0x0a5a,
		0x7e93,0xf782,0x6cb0,0xe5a1,0x5ad5,0xd3c4,0x48f6,0xc1e7,
		0x361f,0xbf0e,0x243c,0xad2d,0x1259,0x9b48,0x007a,0x896b,
		0xff83,0x7692,0xeda0,0x64b1,0xdbc5,0x52d4,0xc9e6,0x40f7,
		0xb70f,0x3e1e,0xa52c,0x2c3d,0x9349,0x1a58,0x816a,0x087b,
		0x7074,0xf965,0x6257,0xeb46,0x5432,0xdd23,0x4611,0xcf00,
		0x38f8,0xb1e9,0x2adb,0xa3ca,0x1cbe,0x95af,0x0e9d,0x878c,
		0xf164,0x7875,0xe347,0x6a56,0xd522,0x5c33,0xc701,0x4e10,
		0xb9e8,0x30f9,0xabcb,0x22da,0x9dae,0x14bf,0x8f8d,0x069c,
		0x7255,0xfb44,0x6076,0xe967,0x5613,0xdf02,0x4430,0xcd21,
		0x3ad9,0xb3c8,0x28fa,0xa1eb,0x1e9f,0x978e,0x0cbc,0x85ad,
		0xf345,0x7a54,0xe166,0x6877,0xd703,0x5e12,0xc520,0x4c31,
		0xbbc9,0x32d8,0xa9ea,0x20fb,0x9f8f,0x169e,0x8dac,0x04bd,
		0x7436,0xfd27,0x6615,0xef04,0x5070,0xd961,0x4253,0xcb42,
		0x3cba,0xb5ab,0x2e99,0xa788,0x18fc,0x91ed,0x0adf,0x83ce,
		0xf526,0x7c37,0xe705,0x6e14,0xd160,0x5871,0xc343,0x4a52,
		0xbdaa,0x34bb,0xaf89,0x2698,0x99ec,0x10fd,0x8bcf,0x02de,
		0x7617,0xff06,0x6434,0xed25,0x5251,0xdb40,0x4072,0xc963,
		0x3e9b,0xb78a,0x2cb8,0xa5a9,0x1add,0x93cc,0x08fe,0x81ef,
		0xf707,0x7e16,0xe524,0x6c35,0xd341,0x5a50,0xc162,0x4873,
		0xbf8b,0x369a,0xada8,0x24b9,0x9bcd,0x12dc,0x89ee,0x00ff
} ;

unsigned short GetCRC_CCITT(unsigned char *pbytData, int intNumByte, unsigned short crc)
{
    unsigned short uintCRC ;
    int i ;

	uintCRC = crc ;
	for(i = 0 ; i < intNumByte ; i++) {
		uintCRC = (uintCRC << 8) ^ *(uintCRC16Table + (((uintCRC >> 8) & 0xff)
				^ *pbytData++));
	}

	return(uintCRC) ;
}


int  DoCrcCheck(int lenbuf, unsigned char * buf, char crcl, char crch)
{
unsigned short crc ;

	crc = GetCRC_CCITT( (unsigned char*) buf,lenbuf, 0) ;
#ifdef USE_MONITORING_
	printf("\nCRC of %d:  %x,%x %x,%x\n", lenbuf, (crcl & 0xff)  , (crc & 0xff), (crch & 0xff) , ((crc>>8) & 0xff) ) ;
#endif
	if ( ((crcl & 0xff) == (crc & 0xff)) && ((crch & 0xff) == ((crc>>8) & 0xff)) ) {
		return(-1) ;
	} else {
#ifdef USE_MONITORING
	printf("\nBAD CRC of %d:  %x,%x %x,%x\n", lenbuf, (crcl & 0xff)  , (crc & 0xff), (crch & 0xff) , ((crc>>8) & 0xff) ) ;
#endif
		return(0) ;
	}
}

#ifdef SW_MTSCU
static void ParseBridge(struct _MTSVALS *pmc)
{
int i, j ;
char *p, ss[DEF_STRING] ;

#ifdef USE_MONITORING
	printf("\nrcv Bridge %s\n", MtsData.Rxbridge ) ;
#endif
	if (!(strncmp(MtsData.Rxbridge, "SN", 2))){
		p = (char*) &MtsData.Rxbridge[3] ;
		i = atoi(p) ;
		if (i==Gdata.mts_sn){
			p = strchr(p, '\0') ;
			p++ ;
#ifdef USE_MONITORING
	printf("\nrcv Bridge1 %s\n", p ) ;
#endif
			if (strncmp(p, "IP:0.0.0.0", 10)){
				strcpy(Gdata.mts_ip, &p[3] ) ;
				sprintf(ss, "<<IP of %d CHANGED (%s)>>\n", Gdata.mts_sn, Gdata.mts_ip ) ; 
				UpdateTitle() ;
			}else{
				for(j=2;j<5;j++){
					p = strchr(p, '\0') ;
					if (p==NULL) break ;
					p++ ;
					if ((p-MtsData.Rxbridge)>=MtsData.RxBridgeLen){
						p = NULL ;
						break ;
					}
#ifdef USE_MONITORING
					printf("\nrcv Bridge%d %s (%d,%d)\n", j, p ,p-MtsData.Rxbridge ,MtsData.RxBridgeLen ) ;
#endif
				}
				if (p){
					sprintf(ss, "<<NO IP for %d, Last data: %s>>\n", Gdata.mts_sn, p) ;
				}else
					sprintf(ss, "<<NO IP for %d>>\n", Gdata.mts_sn) ;
			}
			Add_txt_mts(Scrn.boxtxt_mts, ss ) ;
			if (Gdata.bridge_search){
				Gdata.bridge_search |= 0x8000 ; // Next
// 				Gdata.bridge_search++ ;
// 				if (Gdata.bridge_search<NrSrvWAY){
// 					sprintf(ss,"Request SRV %s: \n", SrvWAY[Gdata.bridge_search]);
// 					Add_txt_mts(Scrn.boxtxt_mts, ss ) ;
// 					sprintf(Gdata.trans_buff, "RS,%d,%s,%s,%d", Gdata.mts_sn, SrvWAY[Gdata.bridge_search], Gdata.pc_ip, Gdata.pc_socket ) ;
// 					Gdata.txbuflen = strlen(Gdata.trans_buff) ;
// 					SendToBridge() ;
// 				}else
// 					Gdata.bridge_search=-1 ;
			}
		}
	}else{
		for(i=0;i<MtsData.RxBridgeLen;i++){
			if (!(MtsData.Rxbridge[i])) MtsData.Rxbridge[i] = ' ' ;
		}
		MtsData.Rxbridge[MtsData.RxBridgeLen] = '\n' ;
		MtsData.Rxbridge[MtsData.RxBridgeLen+1] = '\0' ;
		Add_txt_mts(Scrn.boxtxt_mts, MtsData.Rxbridge) ;
	}
	MtsData.RxBridgeLen = 0;
}
#endif // #ifdef SW_MTSCU

void ParseInBuffer( struct _MTSVALS *pmc )
{
//unsigned short LocTrans ;
//char tmpbuf[DEFAULTLEN] ;
int i ;
int np ;            // number of packets
int pp ;            // packet pointer

	//LocTrans = pmc->RxBuffer[0] + (pmc->RxBuffer[1] * 256) ;    // transaction
	pmc->curr_trans = pmc->RxBuffer[0] + (pmc->RxBuffer[1] * 256) ;    // transaction
//	if ((!LocTrans) || (LocTrans == pmc->MtsTransaction)) {
//		(pmc->MtsTransaction)++ ;               // accepted
		np = pmc->RxBuffer[4] ;                 // num of packets
		pp = 5 ;

#ifdef USE_MONITORING
	printf("\nRECV TRANSACTION %d lenght %d nr_packets %d\n", pmc->curr_trans, pmc->RxBufLen, np ) ;
#endif
		//PrepareHistoryDb(pmc) ;
		pmc->RxBuffer[pp + 1] -= Gdata.pkt_offset ;
		for(i=0 ; i<np ; i++) {                 // scan all packets
			switch(pmc->RxBuffer[pp + 1]) {
			case PKT_COMMAND :
				ParseCommandBuffer(pmc, pp) ;
				break ;

			case PKT_GPSIO :
				ParseGpsBuffer(pmc, pp) ;
				break ;

			case PKT_REPORT :
				ParseRepBuffer(pmc, pp) ;
				break ;

			case PKT_BINARY :
				ParseCanBuffer(pmc, pp) ;
				break ;
				
			default :
#ifdef USE_MONITORING
	printf("\nRECV Other PKT %d\n", pmc->RxBuffer[pp + 1] ) ;
#endif
				ParseDefaultBuffer(pmc, pp) ;
				break ;
			}
			pp += (pmc->RxBuffer[pp + 2] + (pmc->RxBuffer[pp + 3] * 256)) ;

// 			if (DataReady) {       // some data
// 				UpdateScreen() ; // for MTScu
// 				UpdateDb(pmc, mdm, pmc->MTSname) ;
// 			}
		}

//#ifdef SW_MTSCU
	if (pmc->DataReady & MTS_CONFIRM_DATA){
		pmc->DataReady &= ~MTS_CONFIRM_DATA ;
		if (Gdata.confirmidx==pmc->confirmidx){
			if (Gdata.down_sm!=NULL)
				step_sm(0);
#ifdef SW_MTSCU
			else if (Gdata.down_tgt!=NULL)
				step_tgt() ;
#endif
			else if (Gdata.down_fw!=NULL){
				if (Gdata.LU_src == LU8GPRS){
					if ((strncmp(Gdata.mts_ip, "192.168.", 8))) {
						if ( (Gdata.confirmidx & (ACK_ADDPKT *15) ) == (ACK_ADDPKT *15) ){ // Every 15 pkt
							SLEEP(5000) ;
						}else{
							SLEEP(500) ;
						}
					}
				}else{
					if ( (Gdata.confirmidx & (ACK_ADDPKT *15) ) == (ACK_ADDPKT *15) ){ // Every 15 pkt
						SLEEP(1200) ;
					}
				}
#ifdef SW_MTSCU
				
				//SLEEP(100) ;			// .5 sec
				// At first ACK calculate delta time
				if (Gdata.t_ack>600L){
					Gdata.t_fwstart = Gdata.t_ack ;
					Gdata.t_ack = time(NULL) - Gdata.t_fwstart ;
					if (Gdata.t_ack==0L) Gdata.t_ack = 1L ;
					Gdata.t_ack *= 2L ;
#ifdef USE_MONITORING
printf("\nT_ack: %lu\n", Gdata.t_ack ) ;
#endif
				}
				if (Gdata.t_ack<60L){
					Gdata.tout_next = Gdata.t_ack ;
//printf("\nCalc Tout: %d\n", Gdata.tout_next ) ;
				}else{
					// Prepare for retry data
					if (Gdata.LU_src != LU8GPRS)
						Gdata.tout_next = FIRST_COM_TIMEOUT ;
					else
						Gdata.tout_next = FIRST_LAN_TIMEOUT ;
//printf("\nDef Tout: %d\n", Gdata.tout_next ) ;
				}
#endif
				Gdata.down_fw_tout = Gdata.tout_next ;
				step_fw(0); // Not retry
			}
#ifdef SW_MTSCU
		}
	}else if (Gdata.down_fw!=NULL){
		if (Gdata.tout_next>Gdata.t_ack){ // retry
			Gdata.down_fw_tout = 2 ;
#endif
		}
	}
//#endif	

#ifdef SW_MTSCU
	// For transaction
	if (pmc->curr_trans){
		switch(Gdata.auto_trans){
			case TRANS_RUNS:
				if (pmc->curr_trans > pmc->rx_trans) pmc->rx_trans = pmc->curr_trans ;
				
				if ( (pmc->curr_trans-1) == pmc->rxok_trans){
					pmc->rxok_trans = pmc->curr_trans ;
				}else{ // Request retry
					char dummy[100] ;
					np = 0 ;
					for(i=(pmc->rxok_trans+1);i<=pmc->curr_trans;i++,np++){
						dummy[0] = i & 0xff ;
						dummy[1] = (i>>8) & 0xff ;
						MTS_AddPacket(SrcDst(Gdata.LU_src), PKT_COMMAND, IDCMD_RETRY, dummy, 2) ;
						sprintf(dummy, "Req retry %d\n", i ) ;
						Add_txt_mts(Scrn.boxtxt_mts, dummy) ;
						if (np>5) break ;
					}
					MTS_SendTransaction() ;
				}
				break ;
			case TRANS_ARMED:
				Gdata.auto_trans = TRANS_RUNS ;
				Gdata.trans_timer = 3 ;
				break ;
			default:
				break ;
		}
	}
#endif // #ifdef SW_MTSCU

//		InsertHistoryDb(pmc, mdm, pmc->MTSname) ;
//	}
}

// *********************************************************************
// *********************************************************************

int unpack_string(char *ins, int inlen, char *outs, char crlf)
{
	int i ;
	int slen ;          // output string len

	for(slen=0, i=0 ; i<inlen ; i++) {
		if ( (crlf) && ( (ins[i]=='\n') || (ins[i]=='\r')) ){
			outs[slen++] = ins[i] ;     // printable
		} else if ( (ins[i] & 0x80) ||
			 (ins[i] < ' ')  ||
			 (ins[i] == 92)  ||
			 (ins[i] == 36)  ||
			 (ins[i] == 39) ) {  // escape char needed ?
			sprintf(&outs[slen], "$%03d", ins[i] & 0xff) ;
			slen += 4 ;
		} else if (ins[i] == '$') {
			outs[slen++] = ins[i] ;     // double it
			outs[slen++] = ins[i] ;
		} else {
			outs[slen++] = ins[i] ;     // printable
		}
	}
	outs[slen] = '\0' ;                 // terminator
	
	return(slen) ;
}

// *********************************************************************

static void ParseCommandBuffer(struct _MTSVALS *pmc, int pp)
{
int i, fldlen, j ;
int nf ;    // number of fields
int fp ;    // field pointer
#ifdef SW_MTSCU
int  k ;
FILE * l_fd ;
char free_msg[DEF_STRING2] ;
#endif
char ldir_data[DATALEN],ss[1] ;
char aux[MAX_STRING];
//	char parname[250], parval[250] ;
//	struct _TRACE_DATA dtrace ;
//	short trace_record, end_trace ;
//	time_t timenow ;

	fp = pp + 5 ;                       // start
	nf = pmc->RxBuffer[pp + 4] ;        // number of fields
// 	dtrace.nr_record = 0 ;		// Added for trace 4.10
// 	end_trace = 0 ;				// Added for trace 4.10
// 	dtrace.nr_rcv = 0 ;
#ifdef USE_MONITORING
	if (Gdata.leveldebug & 0x1) printf("\nRECV PKT_COMMAND: %d fields (1.st %hd)\n", nf, pmc->RxBuffer[fp] ) ;
#endif
#ifdef SW_MTSCU
	if (!nf) Add_txt_mts(Scrn.boxtxt_mts, "No data\n") ;
#endif

	for(i=0 ; i<nf ; i++) {
#ifdef USE_MONITORING
	printf("\nPKT_COMMAND: %d field (1.st %hd)\n", (i+1), pmc->RxBuffer[fp] ) ;
#endif
		switch(pmc->RxBuffer[fp]) {
			case IDCMD_DIAG:		
			pmc->DataReady |= MTS_DATA_DIAG ;
			// Bug on MTS
			if ((pmc->RxBuffer[fp + 2]==2) && (pmc->RxBuffer[fp + 1]==3) ){
				if(pmc->RxBuffer[fp + 3]==5)
					pmc->RxBuffer[fp + 1] = 6 ;
				else
					pmc->RxBuffer[fp + 1] = 4 ;
			}
#ifdef USE_MONITORING
			printf("\nRECV IDCMD_DIAG: %d fields, tot len %d,  (field %hd)\n",nf, 
		   								(pmc->RxBuffer[pp + 2] + (pmc->RxBuffer[pp + 3] * 256)), 
		   								pmc->RxBuffer[fp+1] ) ;
			if ( (nf==1) && (pmc->RxBuffer[fp+1]<(pmc->RxBuffer[pp + 2]-7)) && (pmc->RxBuffer[fp + 2]==4) )
				pmc->RxBuffer[fp+1]=(pmc->RxBuffer[pp + 2]-7) ;
#endif
			pmc->diaglen = pmc->RxBuffer[fp + 1] ;
			       
			memcpy(pmc->diagdata, &(pmc->RxBuffer[fp + 2]), pmc->RxBuffer[fp + 1]) ;
			break;
				
			case IDCMD_DEBUG: //_FR_
#ifdef USE_MONITORING
			if (Gdata.leveldebug & 0x1) printf("\nRECV PKT_COMMAND: IDCMD_DEBUG\n" ) ;
#endif
#ifdef USE_MONITORING
			printf("\nDEBUG: <%s>\n",  &pmc->RxBuffer[fp + 2]) ;
#endif
			memcpy(aux,&pmc->RxBuffer[fp + 2],sizeof(aux));
				
			if ( strstr(aux,"usb 1-1:") > 0 ) MtsData.usb++;
			if ( strstr(aux,"usb 1-2:") > 0 ) MtsData.usb++;
			if ( strstr(aux,"USB Mass Storage device found") > 0 ) MtsData.usb++ ;

			fldlen = pmc->RxBuffer[fp + 1] ;
#ifdef SW_MTSCU
			free_msg[0] = '\0' ;
			k = 0 ;
			for(j=0;j<fldlen;j++){
				free_msg[k++] = pmc->RxBuffer[fp + 2 + j] ;
				if (pmc->RxBuffer[fp + 2 + j] == 10 ){  // '\n'
					free_msg[k++] = '\0' ;
					Add_txt_mts(Scrn.boxtxt_mts, free_msg) ;
					free_msg[0] = '\0' ;
					k = 0 ;
				}
				//else if ( (pmc->RxBuffer[fp + 2 + j]<32) || (pmc->RxBuffer[fp + 2 + j]>0x7e))
				//	printf("\nRC 0x%x %c\n", pmc->RxBuffer[fp + 2 + j], pmc->RxBuffer[fp + 2 + j] ) ;
			}
			//pmc->RxBuffer[fp + 1 + fldlen] = '\n' ;
			//pmc->RxBuffer[fp + 2 + fldlen] = '\0' ;
			if (k){
				free_msg[k] = '\0' ;
				Add_txt_mts(Scrn.boxtxt_mts, free_msg) ;
			}
				//Add_txt_mts(Scrn.boxtxt_mts, (char*) &pmc->RxBuffer[fp + 2]) ; // ltty_buf[0] = <LF>
#endif
			break ;

			case IDCMD_DATA: // Abort upgrade (from 4.23)
#ifdef USE_MONITORING
			if (Gdata.leveldebug & 0x1) printf("\nRECV PKT_COMMAND: IDCMD_DATA\n" ) ;
#endif

 			if (pmc->RxBuffer[fp + 1]==2){
				j = *((short *)(&(pmc->RxBuffer[fp + 2]))) ;
				if (!j){
					fclose(Gdata.down_fw) ;
					Gdata.down_fw = NULL ;
					if (Gdata.LU_src != LU8GPRS){
						ss[0] = 0 ; 
				    MTS_AddPacket( SrcDst(Gdata.LU_src), PKT_COMMAND, IDCMD_RESET, ss, 1) ;
				    MTS_SendTransaction() ;
				    SLEEP(1000) ;			// 1 sec
						com_baud(MTS_current_PORT, baud_select(9600) ) ;
						//com_baud(MTS_current_PORT, baud_select(Gdata.baudrate) ) ;
						Gdata.bcrch=0;
						Gdata.bcrcl=0;
					}
					//StatusBar(1, "FW: Abort by MTS") ;
				}
 			}
			break ;
			
			case IDCMD_PING: // added ver 4.08 (only by GPRS)
#ifdef USE_MONITORING
			if (Gdata.leveldebug & 0x1) printf("\nRECV PKT_COMMAND: IDCMD_PING\n" ) ;
#endif
// 			strcpy(comment,"IDCMD_PING") ;
// 			// invia: pmc->RxBuffer
// 			if (pmc->ModemSM!=MDM_UDPDATA){
// 				pmc->TxBufLen = (short) pmc->RxBuffer[pp + 1] ;
// 				pmc->TxPacketNum = nf ;
// 				memcpy(&pmc->TxBuffer[0], &pmc->RxBuffer[pp], pmc->TxBufLen ) ;
// 				pmc->ModemSM = MDM_UDPPING ;
// 				strcpy(pmc->TrType,"Answer to Ping") ;
// 			}
// 			sprintf(parval,"RecvPing");
// 			ActivityLog(parval, mdm) ;
			break ;

			case IDCMD_SET :
#ifdef USE_MONITORING
			if (Gdata.leveldebug & 0x1) printf("\nRECV PKT_COMMAND: IDCMD_SET\n" ) ;
#endif
			//strcpy(comment,"IDCMD_SET") ;
// ver 4.01 start IDCMD_RETRY
			pmc->parnum = *((unsigned short *)(&(pmc->RxBuffer[fp + 2]))) ;
			pmc->DataReady |= MTS_DATA_PAR ;
#ifdef USE_MONITORING
			printf(" Recv %d \n", pmc->parnum ) ;
#endif
			if ( ((pmc->RxBuffer[pp + 0])>>4) == LU13CAN ) { // from CAN
				if (pmc->RxBuffer[fp + 1]>2) {        // if something
					memcpy(pmc->parval, &(pmc->RxBuffer[fp + 4]), pmc->RxBuffer[fp + 1] - 2) ;
					pmc->parval[pmc->RxBuffer[fp + 1] - 2] = '\0' ;
					printf("\nRecv %d\n", pmc->parnum ) ;
#ifdef SW_MTSCU
					sprintf(free_msg,"%d=%s\n", pmc->parnum, pmc->parval ) ;
#endif
					//Added ver 4.08 (get CAN FW version)
					if (pmc->RxBuffer[fp + 2]==PAR_SWAUXVER){
						memset(pmc->Data_swAUXver, 0, sizeof(pmc->Data_swAUXver)) ; // Added into 4.09
						memcpy(pmc->Data_swAUXver, &pmc->RxBuffer[fp + 4 + 6], pmc->RxBuffer[fp + 1]-8) ;
						pmc->DataReady |= MTS_DATA_SWAUXVER ;
					printf("\nRecv %d <%s>\n", pmc->parnum, pmc->Data_swAUXver ) ;
#ifdef SW_MTSCU
						sprintf(free_msg,"%d=%s\n", PAR_SWAUXVER, pmc->Data_swAUXver ) ;
#endif
					}
				}
			}else{
				if (pmc->parnum < 128) {              // short
					sprintf(pmc->parval, "0x%x", *((unsigned short *)(&(pmc->RxBuffer[fp + 4]))) ) ;
					pmc->hpval = *((unsigned short *)(&(pmc->RxBuffer[fp + 4]))) ;
#ifdef SW_MTSCU
					sprintf(free_msg,"%d=%hu (%s)\n", pmc->parnum, (unsigned short) pmc->hpval, pmc->parval ) ;
#endif
				} else if (pmc->parnum < 224) {       // char
					memcpy(pmc->parval, &(pmc->RxBuffer[fp + 4]), pmc->RxBuffer[fp + 1] - 2) ;
					pmc->parval[pmc->RxBuffer[fp + 1] - 2] = '\0' ;
#ifdef SW_MTSCU
					sprintf(free_msg,"%d=%s\n", pmc->parnum, pmc->parval ) ;
#endif
				} else {                                        // long
					sprintf(pmc->parval, "0x%x", *((unsigned int *)(&(pmc->RxBuffer[fp + 4]))) ) ;
					pmc->ipval = *((int*)(&(pmc->RxBuffer[fp + 4]))) ;
#ifdef SW_MTSCU
					sprintf(free_msg,"%d=%d (%s)\n", pmc->parnum, pmc->ipval, pmc->parval ) ;
#endif
				}
				// New: NO PARAM
				if (pmc->parnum == 255) {
					//if (ConvertParam(pmc->RxBuffer[fp + 4], parname))
					//	sprintf(parname, "Par.%d", *((unsigned short *)(&(pmc->RxBuffer[fp + 4]))) ) ;
					pmc->parnum =  *((unsigned short *)(&(pmc->RxBuffer[fp + 4])))  ;
					pmc->parnum |= 0x8000 ; // NO data
					pmc->parval[0]='\0';
#ifdef SW_MTSCU
					sprintf(free_msg,"%d= N.D.\n", (pmc->parnum & 0xff) ) ;
#endif
				}
#ifdef SW_MTSCU
				//Case 64, 65, 67 To 71, 73, 76 To 79, 81 To 83, 90 To 98
				if ( (pmc->parnum>63) && (pmc->parnum<72) && (pmc->parnum!=66) )
					strcat(pmc->parldisp, free_msg) ; // ltty_buf[0] = <LF>
				else if (pmc->parnum==73)
					strcat(pmc->parldisp, free_msg) ; // ltty_buf[0] = <LF>
				else if ( (pmc->parnum>75) && (pmc->parnum<84) && (pmc->parnum!=80) )
					strcat(pmc->parldisp, free_msg) ; // ltty_buf[0] = <LF>
				else if ((pmc->parnum>89) && (pmc->parnum<99) )
					strcat(pmc->parldisp, free_msg) ; // ltty_buf[0] = <LF>
				//Case 128 To 167, 170 To 172, 180 To 186
				else if ((pmc->parnum>127) && (pmc->parnum<168) )
					strcat(pmc->parldisp, free_msg) ; // ltty_buf[0] = <LF>
				else if ((pmc->parnum>169) && (pmc->parnum<173) )
					strcat(pmc->parldisp, free_msg) ; // ltty_buf[0] = <LF>
				else if ((pmc->parnum>79) && (pmc->parnum<187) )
					strcat(pmc->parldisp, free_msg) ; // ltty_buf[0] = <LF>
					
#endif // #ifdef SW_MTSCU
			}
#ifdef SW_MTSCU
			strcat(pmc->pardisp, free_msg) ; // ltty_buf[0] = <LF>
#endif
			break ;

			case IDCMD_VOL :
			//strcpy(comment,"IDCMD_VOL") ;
			//if (!(pmc->DataReady & DATA_VOLUME)) {
				sprintf(pmc->Data_volspk, "%d", pmc->RxBuffer[fp + 2]) ;
				sprintf(pmc->Data_volmic, "%d", pmc->RxBuffer[fp + 3]) ;
				if (nf == 4) {
					nf = 1 ;    // bug recovery
				}
				pmc->DataReady |= MTS_DATA_VOLUME ;
			//}
			break ;

			case IDCMD_RETRY :
#ifdef SW_MTSCU
			//char free_msg[DEF_STRING] ;
			j = *((short *)(&(pmc->RxBuffer[fp + 2]))) ;	// Trans to resend
#ifdef USE_MONITORING
			printf("\nRECV retry %d\n", j ) ;
#endif
			k = 100 + (j & 0x7F) ;
			memset( free_msg, 'A', k) ;
			sprintf(&free_msg[k], ";%05d\n", j) ;
			k = strlen(free_msg) ;
			MTS_AddPacket(SrcDst(Gdata.trans_lu), PKT_COMMAND, IDCMD_DIRECT, free_msg, k) ;
			Gdata.transnum = j ;
			MTS_SendTransaction() ;
#endif // SW_MTSCU
			break ;
		
			case IDCMD_DIRECT :
#ifdef USE_MONITORING
			if (Gdata.leveldebug & 0x1) printf("\nRECV PKT_COMMAND: IDCMD_DIRECT (0x%x)\n", pmc->RxBuffer[pp + 0] ) ;
#endif
			//strcpy(comment,"IDCMD_DIRECT") ;
			// Is a confirm ?
 				if (  ( ( ((pmc->RxBuffer[pp + 0])>>4) == LU0CPU ) /*// by GPRS */ || (((pmc->RxBuffer[pp + 0])>>4) == (pmc->RxBuffer[pp + 0] & 0x0f) )) && (Gdata.confirmidx)){    // by COMs

			 
//			if  ( ((pmc->RxBuffer[pp + 0])>>4) == LU0CPU ) {
				//if (!(pmc->DataReady & CONFIRM_DATA)) {
					memcpy(&pmc->confirmidx, &pmc->RxBuffer[fp + 2], 4) ;
					pmc->DataReady |= MTS_CONFIRM_DATA ;
				//}
#ifdef USE_MONITORING
				printf("\nRECV MTS_CONFIRM_DATA \n" ) ;
#endif
			} else {
				// direct
				
				//if (!(pmc->DataReady & DEFAULT_DATA)) {         // resource free
				//	strcpy(pmc->Def_src, src) ;                 // Source
					sprintf(pmc->Def_lu, "%d", (pmc->RxBuffer[pp + 0])>>4 ) ;
					sprintf(pmc->Def_pkt, "%d", pmc->RxBuffer[pp + 1]) ;
					sprintf(pmc->Def_fld, "%d", pmc->RxBuffer[fp]) ;

#if ((DATALEN/4)<0xff)
					fldlen = MIN(pmc->RxBuffer[fp + 1], DATALEN/4) ;
#else
					fldlen = pmc->RxBuffer[fp + 1] ;
#endif					
					fldlen = unpack_string((char*)&(pmc->RxBuffer[fp + 2]), fldlen, ldir_data, 1) ;
					if (fldlen) {
#ifdef SW_MTSCU
						if (strlen(pmc->Dir_data))
							strcat(pmc->Dir_data, ldir_data) ;
						else
#endif
							strcpy(pmc->Dir_data, ldir_data) ;
					}
#ifdef USE_MONITORING
					printf("\nRECV IDCMD_DIRECT (%s)\n",  pmc->Dir_data) ;
#endif
//////////////      fldlen = MIN(pmc->RxBuffer[fp + 1], DATALEN-1) ;
//////////////      memcpy(pmc->Def_data, &(pmc->RxBuffer[fp + 2]), fldlen) ;
//////////////      pmc->Def_data[fldlen] = '\0' ;

					pmc->DataReady |= MTS_DIRECT_DATA ;
				//}

			}
			break ;

			case IDCMD_BOOT :       // boot equivalent to alarm code 0
			//strcpy(comment,"IDCMD_BOOT") ;
			//if (!(pmc->DataReady & (DATA_ALARM | ALARM_REPLY))) { // resource free
				if (pmc->RxBuffer[fp+1]) {
					TimeDB((time_t *)(&(pmc->RxBuffer[fp+2])),pmc->Data_almtime) ;
				} else {
					strcpy(pmc->Data_almtime, "1980") ;
				}
				strcpy(pmc->Data_almnum, "0") ;         // Special code
				//strcpy(pmc->Data_almsrc, src) ;         // Source

				pmc->DataReady |= MTS_DATA_ALARM ;
			//}
			break ;

#ifdef SW_MTSCU
			case IDCMD_SYNQ:
			Add_txt_mts(Scrn.boxtxt_mts, "Confermato set data/ora\n") ;
			break ;
#endif
			
			//DEFAULTLEN=100IDCMD_RETRY
			case IDCMD_TRACE:	 	// get a trace record
#ifdef SW_MTSCU
			l_fd = fopen("target_recvIDCMD_RETRY.txt", "a") ;
			if (pmc->RxBuffer[fp+1] == 2) {
				fprintf(l_fd,"#END") ;
				sprintf(free_msg, "Trace: Recv ALL (%d)", Gdata.trace_nr) ;
				StatusBar(1, free_msg ) ;
				fclose(l_fd) ;
				break ;
			}
			Gdata.trace_nr++ ;
			// RECORD NR, trace_ver, task_id, type, event_date, fix_date, lat, long, speed, sat, CSQ, LENDATA
			fprintf(l_fd,"%d\t%hd\t%hd\t%d\t%lu\t%lu\t%.6f\t%.6f\t%d\t%ud\t%ud\t%ud\t",
									*((short *)(&(pmc->RxBuffer[fp+2]))), pmc->RxBuffer[fp+3], pmc->RxBuffer[fp+4],
									*((short *)(&(pmc->RxBuffer[fp+5]))), *((unsigned long *)(&(pmc->RxBuffer[fp+7]))), 
									*((unsigned long *)(&(pmc->RxBuffer[fp+11]))), 
									(double)(*((int *)(&(pmc->RxBuffer[fp+15])))) * 90.0 / ((double)(0x40000000)), // lat 
									(double)(*((int *)(&(pmc->RxBuffer[fp+19])))) * 90.0 / ((double)(0x40000000)), // long
									*((short *)(&(pmc->RxBuffer[fp+23]))),pmc->RxBuffer[fp+24], pmc->RxBuffer[fp+25],
									pmc->RxBuffer[fp+26] ) ;
			if (pmc->RxBuffer[fp+26] != (pmc->RxBuffer[fp+1]-27) ){
				fprintf(l_fd,"Bad other data\n") ;
			}else{
				for(j=27;j<pmc->RxBuffer[fp+1];j++) fprintf(l_fd,"%02x", pmc->RxBuffer[fp+j] ) ;
			}
			fclose(l_fd) ;
#endif
			break ;
			
			case IDCMD_ACK  :       // ignore data (if all fields zero)
			//strcpy(comment,"IDCMD_ACK") ;
			for (j=0; j<6;j++){
				if (pmc->RxBuffer[fp + 2 + i]>0) break ;
			}
			//if (j==6) break ;
			break ;

			default :
#ifdef USE_MONITORING
			if (Gdata.leveldebug & 0x1) printf("\nRECV PKT_COMMAND: IDCMD_OTHER (1.st %hd)\n", pmc->RxBuffer[fp] ) ;
#endif
			//sprintf(comment,"IDCMD_OTHER! (%hhd)", pmc->RxBuffer[fp]) ;
			//if (!(pmc->DataReady & DEFAULT_DATA)) {             // resource free
			//	strcpy(pmc->Def_src, src) ;                     // Source
				sprintf(pmc->Def_lu, "%d", (pmc->RxBuffer[pp + 0])>>4 ) ;
				sprintf(pmc->Def_pkt, "%d", pmc->RxBuffer[pp + 1]) ;
				sprintf(pmc->Def_fld, "%d", pmc->RxBuffer[fp]) ;

#if ((DATALEN/4)<0xff)
				fldlen = MIN(pmc->RxBuffer[fp + 1], DATALEN/4) ;
#else				
				fldlen = pmc->RxBuffer[fp + 1] ;
#endif
				unpack_string((char*)&(pmc->RxBuffer[fp + 2]), fldlen, pmc->Def_data, 0) ;

///////////     fldlen = MIN(pmc->RxBuffer[fp + 1], DATALEN-1) ;
///////////     memcpy(pmc->Def_data, &(pmc->RxBuffer[fp + 2]), fldlen) ;
///////////     pmc->Def_data[fldlen] = '\0' ;

				pmc->DataReady |= MTS_DEFAULT_DATA ;
			//}
			break ;
		}
		fp += ((pmc->RxBuffer[fp + 1]) + 2) ;

//		if (pmc->ModemMOD!=MOD_SOCKET) ActivityLog(comment, mdm) ;

	}
	
// 	if (dtrace.nr_record){ // if received trace data
// 	}
}

static void ParseGpsBuffer(struct _MTSVALS *pmc, int pp)
{
int i, j ;
int nf ;            // number of fields
int fp ;            // field pointer
double speedn, speede ;
unsigned short anl ;
char comment[DEF_STRING] ;

#ifdef SW_MTSCU		// Used as end history block
FILE * l_fd ;
char last_hist = 0 ;
unsigned char *p ;
#endif
// ver 4.01 startunsigned char *canbuf=NULL ;
	//float can_ad ;
// ver 4.01 end
//char pgline[4*DEFAULTLEN] ;

	pmc->Data_src = pmc->RxBuffer[pp]>>4 ;
	
	nf = pmc->RxBuffer[pp + 4] ;        // number of fields
#ifdef USE_MONITORING
	printf("\nRECV PKT_GPSIO: %d fields\n", nf) ;
#endif
	fp = pp + 5 ;                       // start
	
	for(i=0 ; i<nf ; i++) {
		
		j = 0 ;
	
		switch(pmc->RxBuffer[fp]) {
		case IDGPS_ECHO: // Used as FW upgrade ACK by serial (MTS40-4004)
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_ECHO\n" ) ;
#endif
			strcpy(comment,"IDGPS_ECHO") ;
			if  ( ((pmc->RxBuffer[pp + 0])>>4) == LU0CPU ) {
				memcpy(&pmc->confirmidx, &pmc->RxBuffer[fp + 2], 4) ;
				pmc->DataReady |= MTS_CONFIRM_DATA ;
#ifdef USE_MONITORING
				printf("\nMTS40: RECV MTS_CONFIRM_DATA \n" ) ;
#endif
			}
		break ;
		
		case IDGPS_FULLFIX :            // IDGPS_fix
		case IDGPS_ABSFIX :
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_FULLFIX\n" ) ;
#endif
			//sprintf(comment,"IDGPS_FIX") ;

			//if (!(pmc->DataReady & DATA_FIX)) {    // resource free
				TimeDB((time_t *)(&(pmc->RxBuffer[fp+2])),pmc->Data_fixtime) ;
				//strcpy(pmc->Hist_fixtime, pmc->Data_fixtime) ;
#ifdef SW_MTSCU		// Used as end history block
			if ( !(*(time_t *)(&(pmc->RxBuffer[fp+2]))) ){
				if ( (Gdata.logs_type & DATA_LOGALLHIST) && (pmc->Data_src!=LU5GPS) ){
					Gdata.logs_type &= ~DATA_LOGALLHIST ;
					sprintf(comment, "History: recv %d ALL", (Gdata.histrecv) ) ;
					//Add_txt_mts(Scrn.boxtxt_mts, comment) ;
					StatusBar(1, comment) ;
					last_hist = 1 ;
				}
			}
#endif
				sprintf(comment,"IDGPS_FIX") ;
				
				sprintf(pmc->Data_flat,"%.6f",(double)(*((int *)(&(pmc->RxBuffer[fp+10])))) * 90.0 / ((double)(0x40000000))) ;
				//strcpy(pmc->Hist_flat, pmc->Data_flat) ;

				sprintf(pmc->Data_flon,"%.6f",(double)(*((int *)(&(pmc->RxBuffer[fp+14])))) * 90.0 / ((double)(0x40000000))) ;
				//strcpy(pmc->Hist_flon, pmc->Data_flon) ;

				speedn = ((double)(*((short *)(&(pmc->RxBuffer[fp+6]))))) * 0.072 ;
				speede = ((double)(*((short *)(&(pmc->RxBuffer[fp+8]))))) * 0.072 ;

				sprintf(pmc->Data_fspn,"%.1f",speedn) ;
				//strcpy(pmc->Hist_fspn, pmc->Data_fspn) ;
				sprintf(pmc->Data_fspe,"%.1f",speede) ;
				//strcpy(pmc->Hist_fspe, pmc->Data_fspe) ;
				sprintf(pmc->Data_fspd,"%.1f",sqrt(speedn*speedn+speede*speede)) ;
				//strcpy(pmc->Hist_fspd, pmc->Data_fspd) ;

				//strcpy(pmc->Data_fixsrc, src) ;         // Source
				//strcpy(pmc->Hist_src, src) ;            // Source

				//strcpy(pmc->Hist_log, "D") ;            // default from direct (BM 19/4/2004)
				
				if (pmc->RxBuffer[fp] == IDGPS_FULLFIX) {
					sprintf(comment,"IDGPS_FULLFIX") ;
					sprintf(pmc->Data_falt,"%d",*((short *)(&(pmc->RxBuffer[fp+18])))) ;
					sprintf(pmc->Data_fnsat,"%d",pmc->RxBuffer[fp+20]) ;
#ifdef SW_MTSCU
					if (pmc->RxBuffer[fp+20]) {         // satellites ?
						if (pmc->RxBuffer[fp+21] & 4) {
							strcpy(pmc->Data_fixtype,"2d") ;
						} else {
							strcpy(pmc->Data_fixtype,"3d") ;
						}
					} else {
						pmc->Data_fixtype[0] = '\0' ;
					}
#endif // #ifdef SW_MTSCU
#ifdef MTSTESTKIT
					sprintf(pmc->Data_fixtype,"%d",pmc->RxBuffer[fp+21]) ;
#endif

				} else {
					strcpy(pmc->Data_falt, "0") ;
					strcpy(pmc->Data_fnsat, "0") ;
					pmc->Data_fixtype[0] = '\0' ;

				}
				// check Rovera mail of 11/11/2003
// 				if ((pmc->RxBuffer[pp] & 0xf0)==0x10){  // from 2.35 history from LU1
// 					strcpy(pmc->Hist_log, "B") ;        // from buffer
// 				}
				
				pmc->DataReady |= MTS_DATA_FIX  ;
			//}
			break ;

		case IDGPS_IO :                 // IDGPS_IO
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_IO\n" ) ;
#endif
			strcpy(comment,"IDGPS_IO") ;

			//if (!(pmc->DataReady & (DATA_IO | HIST_IO))) {      // resource free
				TimeDB((time_t *)(&(pmc->RxBuffer[fp+2])),pmc->Data_iotime) ;
				pmc->mts_iotime = *((int32_t *)(&(pmc->RxBuffer[fp+2]))) ;
#ifdef USE_MONITORING
				printf("\ntime MTS:%d\n", *((int32_t *)(&(pmc->RxBuffer[fp+2])))) ;
#endif
#ifdef SW_MTSCU					
				//strcpy(pmc->Hist_iotime, pmc->Data_iotime) ;
				sprintf(pmc->Data_ioinp,"%04x",*((unsigned short *)(&(pmc->RxBuffer[fp+6]))) ) ;
				//strcpy(pmc->Hist_ioinp, pmc->Data_ioinp) ;
				sprintf(pmc->Data_ioout,"%04x",*((unsigned short *)(&(pmc->RxBuffer[fp+8]))) ) ;
				//strcpy(pmc->Hist_ioout, pmc->Data_ioout) ;
				//strcpy(pmc->Data_iosrc, src) ;          // Source
#endif
#ifdef MTSTESTKIT
				sprintf(pmc->Data_ioinp,"%d",*((unsigned short *)(&(pmc->RxBuffer[fp+6])))) ;
				sprintf(pmc->Data_ioout,"%d",*((unsigned short *)(&(pmc->RxBuffer[fp+8])))) ;
#endif
				if (pmc->RxBuffer[fp+ 1] > 10) {
					sprintf(pmc->Data_cnt1, "%i", *((int32_t *)(&(pmc->RxBuffer[fp+11]))) ) ;  //prima long "%lu"
					sprintf(pmc->Data_cnt2, "%i", *((int32_t *)(&(pmc->RxBuffer[fp+15]))) ) ;  //prima long "%lu"
				}else{
					pmc->Data_cnt1[0] = '\0' ;
					pmc->Data_cnt2[0] = '\0' ;
				}

				pmc->DataReady |= MTS_DATA_IO  ;
			//}
			break ;

		case IDGPS_ALARM :              // IDGPS_ALARM
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_ALARM\n" ) ;
#endif
			strcpy(comment,"IDGPS_ALARM") ;
			//if (!(pmc->DataReady & (DATA_ALARM | ALARM_REPLY))) { // resource free
				TimeDB((time_t *)(&(pmc->RxBuffer[fp+4])), pmc->Data_almtime) ;
				sprintf(pmc->Data_almnum,"%d",*((short *)(&(pmc->RxBuffer[fp+2])))) ;
				//strcpy(pmc->Data_almsrc, src) ;         // Source
				pmc->DataReady |= MTS_DATA_ALARM ; //  | ALARM_REPLY) ;
			//}
			break ;

		case IDGPS_AD :					// IDGPS_AD
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_AD\n" ) ;
#endif
			strcpy(comment,"IDGPS_AD") ;

			//if (!(pmc->DataReady & (DATA_AD | HIST_AD))) {      // resource free
				for(j=0 ; j<8 ; j++) {
					anl = *(unsigned short *)(&(pmc->RxBuffer[fp+6 + 2*j])) ;
#ifdef SW_MTSCU				
					switch(j) {
					case 5 :    // Vin
						if ((anl & 0xf) == 1) {   // extended Vin range from 1.48
							sprintf(pmc->Data_ioad[j],"%.1f",(((double)(anl)) / 65536.0) * 5.0 * 5.83 * 2.044) ;
						} else if ((anl & 0xf) == 2) {    // extended Vin range from MTS100
							sprintf(pmc->Data_ioad[j],"%.1f",((((double)(anl)) / 65536.0) * 5.0 * 11.0) + 0.74) ;
						} else if ((anl & 0xf) == 8) {    // extended Vin range from MTS2102-EVR
							sprintf(pmc->Data_ioad[j],"%.1f",((((double)(anl>>6)) / 1023.0) * 100.0 )) ;
						} else {
							sprintf(pmc->Data_ioad[j],"%.1f",(((double)(anl)) / 65536.0) * 5.0 * 5.83) ;
						}
						strcat(pmc->Data_ioad[j], " V") ;
						break ;

					case 6 :    // Vb
						if ((anl & 0xf) == 1) {   // From MTS02E
							sprintf(pmc->Data_ioad[j],"%.2f",(((double)(anl)) / 65536.0) * 5.0) ;
						} else if ((anl & 0xf) == 2) {    // extended Vin range from MTS40
							sprintf(pmc->Data_ioad[j],"%.2f",((((double)(anl)) / 65536.0) * 5.0 * 3.553) + 0.1) ;
						} else {
							sprintf(pmc->Data_ioad[j],"%.2f",((((double)(anl)) / 65536.0) * 5.0 * 3.56) + 0.6) ;
						}
						strcat(pmc->Data_ioad[j], " V") ;
						break ;

					default :
						sprintf(pmc->Data_ioad[j],"%d",(anl>>8)) ;
						break ;
					}
#endif
#ifdef MTSTESTKIT
					sprintf(pmc->Data_ioad[j],"%d",anl) ;
#endif
					//strcpy(pmc->Hist_ioad[j], pmc->Data_ioad[j]) ;
				}
				sprintf(pmc->Data_iokm,"%.3f",(double)(*((int *)(&(pmc->RxBuffer[fp+22])))) / 1000.0) ;
				//strcpy(pmc->Hist_iokm, pmc->Data_iokm) ;
				
				if (pmc->RxBuffer[fp+1]>26 ){ // check len
					sprintf(pmc->Data_ioHinX,"%.3f",(double)(*((int *)(&(pmc->RxBuffer[fp+26]))))/60.0) ;
					//strcpy(pmc->Hist_ioHinX, pmc->Data_ioHinX) ;				
				}else{
					strcpy(pmc->Data_ioHinX, "") ;
				}
				//strcpy(pmc->Hist_ioHinX, pmc->Data_ioHinX) ;				
				
				// check Rovera mail of 11/11/2003
				//strcpy(pmc->Hist_src, src) ;  // Source
// 				if (pmc->Hist_log[0]=='\0'){
// 					// Vcc ext=0  and Vbatt = 0  into history data from ver 1.51
// 					if ( (!(pmc->RxBuffer[fp+6+11])) && (!(pmc->RxBuffer[fp+6+13])) ){
// 						strcpy(pmc->Hist_log, "B") ;        // from buffer
// 					} else {
// 						strcpy(pmc->Hist_log, "D") ;        // from direct
// 					}
// 					if ((pmc->RxBuffer[pp] & 0xf0)==0x10){  // from 2.35 history from LU1
// 						strcpy(pmc->Hist_log, "B") ;        // from buffer
// 					}
// 				}
				
				pmc->DataReady |= MTS_DATA_AD ;
			//}ParseRepBuffer
			break ;

		case IDGPS_SMREAD :             // IDGPS_SMREAD
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_SMREAD\n" ) ;
#endif
		strcpy(comment,"IDGPS_SMREAD") ;
#ifdef SW_MTSCU		// Used as end history block
			if (Gdata.logs_type & DATA_LOGALLHIST){ // REquest next step
				if (!((++Gdata.histnext) % 10)) SLEEP(2000) ;
				
				comment[0] = 0xff ;
				comment[1] = 0xff ;
				comment[2] = 0xff ;
				comment[3] = 0xff ;

				comment[4] = 20 ; // nr records
				comment[5] = 2 ;  // Also internal A/D
				MTS_AddPacket(SrcDst(LU5GPS), PKT_COMMAND, IDCMD_HISTORY, comment, 6) ;
				MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMREAD, NULL, 0) ;
				
				MTS_SendTransaction() ;
				sprintf(comment,"IDGPS_SMREAD (end hist step)") ;
			}else{

				strcpy(comment,"IDGPS_SMREAD") ;
				
			//if (!(pmc->DataReady & (DATA_SMREAD | HIST_SMREAD))) {      // resource free
				sprintf(pmc->Data_ioflags,"%08x",*((unsigned int *)(&(pmc->RxBuffer[fp+2])))) ;
				//strcpy(pmc->Hist_ioflags, pmc->Data_ioflags) ;
				sprintf(pmc->Data_iostat,"%08x",*((unsigned int *)(&(pmc->RxBuffer[fp+6])))) ;
				//strcpy(pmc->Hist_iostat, pmc->Data_iostat) ;
// ver 4.03 start
#endif
#ifdef MTSTESTKIT
				sprintf(pmc->Data_ioflags,"%u",*((uint32_t *)(&(pmc->RxBuffer[fp+2])))) ;
				sprintf(pmc->Data_iostat,"%u",*((uint32_t *)(&(pmc->RxBuffer[fp+6])))) ;
#endif
				if (pmc->RxBuffer[fp + 1]>8){
					sprintf(pmc->canflags,"%08u",*((uint32_t *)(&(pmc->RxBuffer[fp+10])))) ;
				}else
					pmc->canflags[0] = '\0' ;
// ver 4.03 end	

				pmc->DataReady |= MTS_DATA_SMREAD ; // | HIST_SMREAD) ;

#ifdef SW_MTSCU		// Used as end history block
			}
#endif
			break ;

		// ver 4.01 start
		case IDGPS_SMCAN : // get status machine
			strcpy(comment,"IDGPS_SMCAN") ;
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_SMCAN\n" ) ;
#endif
			canmask = *((unsigned long *)(&(pmc->RxBuffer[fp+2])))  ;
			canflag = *((unsigned long *)(&(pmc->RxBuffer[fp+6])))  ;
#ifdef SW_MTSCU
			l_fd = fopen("upload.sm", "a") ;
			fprintf(l_fd, "can %lu|%lu\n",  canmask, canflag ) ;
			fclose(l_fd) ;
#endif
#ifdef MTSTESTKIT
if (Gdata.up_sm!=NULL) {
	if (pmc->RxBuffer[fp + 1]) {
	fwrite(&pmc->RxBuffer[fp],1,pmc->RxBuffer[fp+1]+2,Gdata.up_sm);
	}else{
	fclose(Gdata.up_sm);
	Gdata.up_sm=NULL;
	}
}
#endif
			//canbuf = &(pmc->RxBuffer[fp + 2]) ;
			break ;
		// ver 4.01 end
		
		case IDGPS_SMDUMP :
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_SMDUMP\n" ) ;
#endif
			if (pmc->RxBuffer[fp + 1]==2) {
#ifdef SW_MTSCU
				sprintf(comment, "SM Nr.line=%u", *((unsigned short *)(&(pmc->RxBuffer[fp+2]))) ) ;
				//Add_txt_mts(Scrn.boxtxt_mts, comment) ;
				StatusBar(1, comment) ;
#endif
			}
			strcpy(comment,"IDGPS_SMDUMP") ;
			break ;
			
		case IDGPS_SMADD :              // get status machine
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_SMADD\n" ) ;
#endif
#ifdef SW_MTSCU
			l_fd = fopen("upload.sm", "a") ;
 			if (pmc->RxBuffer[fp + 1]) {        // if something
				
				sprintf(comment, "SM: recv %d ..", MtsData.nrrevc++ ) ;
				StatusBar(1, comment ) ;
				
				sm_item =  ((struct _SMACHINE  *)(&(pmc->RxBuffer[fp + 2]))) ;
				
				actparlen = pmc->RxBuffer[fp + 1] - sizeof(struct _SMACHINE) ;
				if (actparlen>0){
					memcpy(sm_actpar , &pmc->RxBuffer[fp + 2+sizeof(struct _SMACHINE)], actparlen)  ;
					sm_actpar[actparlen] = '\0' ;
					while ((p= (unsigned char*) strchr(sm_actpar, '|'))!=NULL) *p = 178 ; //'²'
					while ((p= (unsigned char*) strchr(sm_actpar, 13))!=NULL) *p='|' ;
				}else
					sm_actpar[0]='\0' ;

				write_smline(l_fd, sm_item) ;
				
/*				if (sm_item->schedule >= 0){
					fprintf(l_fd, "%10ld", sm_item->schedule ) ;
				}else{
					sm_item->schedule = (sm_item->schedule & 0x7FFFFFFF) ;
					fprintf(l_fd, "  @") ;
					for(j=0;j<25;j+=8){
						c = ((sm_item->schedule>>j) & 0x3F) ;
						if (c>=63)
							fprintf(l_fd, "**") ;
						else
							fprintf(l_fd, "%2d", c ) ;
					}
				}
				
				fprintf(l_fd, "%8d  ", sm_item->valtime ) ;
				//	Print #1, Tab(15); sm_item.valtime; Tab(22);
				
				fprintf(l_fd, "%s ", WriteMask(sm_item->phisin, sm_item->msk_phisin, 16, comment) ) ;
				fprintf(l_fd, "%s ", WriteMask(sm_item->phisout, sm_item->msk_phisout, 16, comment) ) ;
				
				fprintf(l_fd, "%s ", WriteMask(sm_item->intflag, sm_item->msk_intflag, 32, comment) );
				fprintf(l_fd, "%s ", WriteMask(sm_item->stsflag, sm_item->msk_stsflag, 32, comment) );
				
				fprintf(l_fd, "%s ", WriteMask(sm_item->setphisout, sm_item->msk_setphisout, 16, comment) );
				fprintf(l_fd, "%s ", WriteMask(sm_item->setstsflag, sm_item->msk_setstsflag, 32, comment) );
				
				// actpar
				//fprintf(l_fd," <%d %d> ", pmc->RxBuffer[fp + 1] , sizeof(struct _SMACHINE) ) ;
				actparlen = pmc->RxBuffer[fp + 1] - sizeof(struct _SMACHINE) ;
				if (actparlen>0){
					memcpy(sm_actpar , &pmc->RxBuffer[fp + 2+sizeof(struct _SMACHINE)], actparlen)  ;
					sm_actpar[actparlen] = '\0' ;
					while ((p= (unsigned char*) strchr(sm_actpar, '|'))!=NULL) *p = 178 ; //'²'
					while ((p= (unsigned char*) strchr(sm_actpar, 13))!=NULL) *p='|' ;
				}else
					sm_actpar[0]='\0' ;
				
				//fprintf(l_fd, "%6d %6d  %s\n",  sm_item->actcode,  sm_item->actval , sm_actpar ) ;*/
// 				 
// ver 4.01 modified
// 				PrintProgramLine((char*)canbuf, (char*)&(pmc->RxBuffer[fp + 2]), pmc->RxBuffer[fp + 1], pgline) ;
// 				AddProgramDB(pmc->dBconidx,
// 						ParseRepBuffer	 pmc->MTSname[0] ? pmc->MTSname : pmc->MTSbytel,
// 							 pgline) ;
 			} else {                            // if end of SM
				sprintf(comment, "SM: recv ALL (%d)", MtsData.nrrevc ) ;
				StatusBar(1, comment ) ;
				fprintf(l_fd, "# End") ;
 			}
			fclose(l_fd) ;
#endif
#ifdef MTSTESTKIT
if (Gdata.up_sm!=NULL) {
	if (pmc->RxBuffer[fp + 1]) {
	fwrite(&pmc->RxBuffer[fp],1,pmc->RxBuffer[fp+1]+2,Gdata.up_sm);
	}else{
	fclose(Gdata.up_sm);
	Gdata.up_sm=NULL;
	}
}
#endif
// 			canbuf = NULL ;
			strcpy(comment,"IDGPS_SMADD") ;
			break ;

		case IDGPS_TGDUMP :
#ifdef USE_MONITORING
	printf("\nRECV PKT_GPSIO: IDGPS_TGDUMP\n" ) ;
#endif	
			if (pmc->RxBuffer[fp + 1]==2) {
#ifdef SW_MTSCU
				sprintf(comment, "Target Nr=%u", *((unsigned short *)(&(pmc->RxBuffer[fp+2]))) ) ;
				//Add_txt_mts(Scrn.boxtxt_mts, comment) ;
				StatusBar(1, comment) ;
#endif
			}
			strcpy(comment,"IDGPS_TGDUMP") ;
			break ;

		case IDGPS_TGFAM2: 			// GPS_IO packet: add target FAMILY2 to list
#ifdef USE_MONITORING
			printf("\nRECV PKT_GPSIO: IDGPS_TGFAM2\n" ) ;
#endif
			strcpy(comment,"IDGPS_TGFAM2") ;
			j++ ;
		case IDGPS_TGFAM1: 			// GPS_IO packet: add target FAMILY1 to list
#ifdef USE_MONITORING
			printf("\nRECV PKT_GPSIO: IDGPS_TGFAM1\n" ) ;
#endif
			strcpy(comment,"IDGPS_TGFAM1") ;
			j++ ;
		case IDGPS_TGADD :              // get target list
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_TGADD\n" ) ;
#endif
			strcpy(comment,"IDGPS_TGADD") ;
#ifdef SW_MTSCU
			{
				FILE * outf ;
				
				if ((outf = fopen("target_recv.txt", "a")) == NULL)  break ;
				
				if (pmc->RxBuffer[fp + 1]!=0) {        // if something

					sprintf(comment, "Tg: recv %d ..", MtsData.nrrevc++ ) ;
					StatusBar(1, comment ) ;
				
					if (j){
						fprintf(outf, "%lu,%lu,%lu,%lu,%d\n",
							*((unsigned long *)(&(pmc->RxBuffer[fp+2]))),
							*((unsigned long *)(&(pmc->RxBuffer[fp+6]))),
							*((unsigned long *)(&(pmc->RxBuffer[fp+10]))),
							*((unsigned long *)(&(pmc->RxBuffer[fp+14]))),
							j ) ;
					}else{
						fprintf(outf, "%lu,%lu,%lu,%lu\n",
							*((unsigned long *)(&(pmc->RxBuffer[fp+2]))),
							*((unsigned long *)(&(pmc->RxBuffer[fp+6]))),
							*((unsigned long *)(&(pmc->RxBuffer[fp+10]))),
							*((unsigned long *)(&(pmc->RxBuffer[fp+14]))) ) ;
					}
	// 				PrintTargetLine((char*)&(pmc->RxBuffer[fp + 2]), pmc->RxBuffer[fp + 1], pgline) ;
	// 				AddTargetDB(pmc->dBconidx,
	// 							pmc->MTSname[0] ? pmc->MTSname : pmc->MTSbytel,
	// 							pgline) ;
				} else {                            // if end of TG
					sprintf(comment, "Tg: recv ALL (%d)", MtsData.nrrevc ) ;
					StatusBar(1, comment ) ;
					//fprintf(outf, "END" ) ;
				}
				fclose(outf) ;
			}
#endif
			break ;
// ver 4.01 added
		case IDGPS_ADCAN :                 // IDGPS_ADCAN
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_ADCAN\n" ) ;
#endif
			strcpy(comment,"IDGPS_ADCAN") ;

			//if (!(pmc->DataReady & DATA_ADCAN )) {      // resource free
				for(j=0 ; j<4 ; j++) {
					//can_ad = *(float *)(&(pmc->RxBuffer[fp+ 2 + 4*j])) ;
					sprintf(pmc->Data_canad[j],"%g",*(float *)(&(pmc->RxBuffer[fp+ 2 + 4*j]))) ;
				}
				pmc->DataReady |= MTS_DATA_ADCAN ; // added ver 4.03
			//}
// ver 4.01 end
			break ;
			
		case IDGPS_INTAD:
#ifdef USE_MONITORING
	printf("\nRECV PKT_GPSIO: IDGPS_INTAD\n" ) ;
#endif
			strcpy(comment,"IDGPS_INTAD") ;
			{
				int ii, idfield, nbyte ;
				double anld ;
				
				ii = 1 ; // Char index
				pmc->Data_nrintad = j = 0 ;  // AD index 
				
				while (pmc->RxBuffer[fp+1] >= ii){
				//printf("\nINTAD len=%d (%d)\n", pmc->RxBuffer[fp+1], ii ) ;
					if ( !(*((unsigned short *)(&(pmc->RxBuffer[fp+ii+1]))) & 0x1fff) ){  // Iotime
						TimeDB((time_t *)(&(pmc->RxBuffer[fp+2+2])),pmc->Data_iotime) ;
						nbyte = 6 ;
						//printf("\nINTAD %s\n" ,pmc->Data_iotime  ) ;
					}else{
						idfield = *((unsigned short *)(&(pmc->RxBuffer[fp+1+ii]))) ;
						
						//printf("\nAD %d len=%d\n", (idfield & 0x1fff), (idfield>>14) ) ;

						switch( (idfield>>14) ){
							case 0 : // 1 byte
							anld = pmc->RxBuffer[fp+1+ii+2] ;
							nbyte = 2 + 1 ;
							break ;

							case 1 : // 2 byte
							if (idfield & 0x2000)  // Signed 
								anld = (double) (*((short *)(&(pmc->RxBuffer[fp+1+ii+2])))) ;
							else
								anld = (double) (*((unsigned short *)(&(pmc->RxBuffer[fp+1+ii+2])))) ;
							nbyte = 2 + 2 ;
							break ;

							case 2 : // 4 byte
							if (idfield & 0x2000)  // Signed 
								anld = (double) (*((long *)(&(pmc->RxBuffer[fp+1+ii+2])))) ;
							else
								anld = (double) (*((unsigned long *)(&(pmc->RxBuffer[fp+1+ii+2])))) ;
							nbyte = 2 + 4 ;
							break ;

							case 3 : // 8 byte
							if (idfield & 0x2000)  // Signed 
								anld = (double)(*((long long *)(&(pmc->RxBuffer[fp+1+ii+2])))) ;
							else
								anld = (double)(*((unsigned long long *)(&(pmc->RxBuffer[fp+1+ii+2])))) ;
							nbyte = 2 + 8 ;
							break ;
						}
						
						idfield &= 0x1fff ;
						switch (idfield){
#ifdef SW_MTSCU
							case 1 :// Vext "42.54 V"
							j = 0 ;			// Always first intAD
							sprintf(pmc->Data_intad[j], "%.2f V", (anld/100.) ) ;
							strcpy(pmc->Name_intad[j], "Vext" ) ;
							j++ ;
							break ;
							
							case 2 :	// Vbatt "12.544 V"
							sprintf(pmc->Data_intad[j], "%.3f V", (anld/1000.) ) ;
							strcpy(pmc->Name_intad[j], "Vbat" ) ;
							j++ ;
							break ;
							
							case 3 :	//'CSQ ..
							sprintf(pmc->Data_intad[j], "%u", (unsigned int) anld ) ;
							strcpy(pmc->Name_intad[j], "CSQ" ) ;
							j++ ;
							break ;
							
							case 4 : // Temp "102.1 °C"
							sprintf(pmc->Data_intad[j], "%.1f °C", (anld/10.) ) ;
							strcpy(pmc->Name_intad[j], "Temp" ) ;
							j++ ;
							break ;
								
							case 5 :
							case 6 :
							case 7 : // Acc "-6.123 g"
							sprintf(pmc->Data_intad[j], "%.3f g", (anld/1000.) ) ;
							sprintf(pmc->Name_intad[j], "Acc%c", (83+idfield) ) ;
							j++ ;
							break ;
							
							case 8 :
							case 9 :
							case 10: // Cnt 1-3
							sprintf(pmc->Data_intad[j], "%lu", (unsigned long) anld ) ;
							sprintf(pmc->Name_intad[j], "Cnt%d", (idfield-7) ) ;
							j++ ;
							break ;
							
							case 11: // GPS sat
							sprintf(pmc->Data_intad[j], "%u", (unsigned int) anld ) ;
							strcpy(pmc->Name_intad[j], "Sat" ) ;
							j++ ;
							break ;
							
							case 12: // GPS altitude
							sprintf(pmc->Data_intad[j], "%ld m", (long) anld ) ;
							strcpy(pmc->Name_intad[j], "Alt" ) ;
							j++ ;
							break ;

							case 256:
							case 257:
							case 258: // AD mean "20.00 mA"
							sprintf(pmc->Data_intad[j], "%.2f mA", (anld/100.) ) ;
							sprintf(pmc->Name_intad[j], "mAD%d", (idfield-255) ) ;
							j++ ;
							break ;

							case 259:
							case 260:
							case 261: // "2022 HourCounter
							sprintf(pmc->Data_intad[j], "%.2f h", (anld/100.) ) ;
							sprintf(pmc->Name_intad[j], "h%d", (idfield-258) ) ;
							j++ ;
							break ;

							case 262:
							case 263:
							case 264: // "2022 Round/10000 Counter
							sprintf(pmc->Data_intad[j], "%.0f", anld ) ;
							sprintf(pmc->Name_intad[j], "c%d", (idfield-261) ) ;
							j++ ;
							break ;
#endif
#ifdef MTSTESTKIT								
							default :
							sprintf(pmc->Data_intad[j], "%u", (unsigned int) anld ) ;
							pmc->Id_intad[j]=idfield;
							j++ ;
							break ;
#endif
						}
					}
					ii += nbyte ;
				}
			}
			pmc->Data_nrintad = j ;
			pmc->DataReady |= MTS_DATA_INTAD ;
			break ;
		
		case IDGPS_AUXAD:
#ifdef USE_MONITORING
	printf("\nRECV PKT_GPSIO: IDGPS_AUXAD\n" ) ;
#endif			
				{
				int k=0 ;

				strcpy(comment,"IDGPS_AUXAD") ;
				for(j=0;j<MAX_AUXAD;j++){
					if (pmc->RxBuffer[fp+1] >= (j + 1) * 3){
						k++ ;
						sprintf(pmc->Data_auxad[j], "%u", 
									*((unsigned short *)(&(pmc->RxBuffer[fp+2+(3*j)+1] )))) ;
						switch(pmc->RxBuffer[fp+2+(3*j)]){
							case 64:
							strcpy(pmc->Name_auxad[j], "HwMAIN") ;
							break ;

							case 65:
							strcpy(pmc->Name_auxad[j], "HwSRV") ;
							break ;

							case 66:
							strcpy(pmc->Name_auxad[j], "I-GPS") ;
							break ;

							case 67:
							strcpy(pmc->Name_auxad[j], "Gyro") ;
							break ;
							
							case 68:
							strcpy(pmc->Name_auxad[j], "I-CNS") ;
							break ;
						}
					}else{
						pmc->Data_auxad[j][0] = '\0' ;
						pmc->Name_auxad[j][0] = '\0' ;
					}
				}
				pmc->Data_nrauxad = k ;
			}
			printf("\nAUXAD %d\n" , pmc->Data_nrauxad ) ;
			pmc->DataReady |= MTS_DATA_AUXAD ;
			break ;

		default : // other data
#ifdef USE_MONITORING
		printf("\nRECV PKT_GPSIO: IDGPS_OTHER\n" ) ;
#endif	
		sprintf(comment,"IDGPS_OTHER! (%hhd)", pmc->RxBuffer[fp]) ;
			//if (!(pmc->DataReady & DEFAULT_DATA)) {             // resource free
			{
				int fldlen ;        // field len
				
				//strcpy(pmc->Def_src, src) ;                     // Source
				sprintf(pmc->Def_lu, "%d", (pmc->RxBuffer[pp + 0])>>4 ) ;
				sprintf(pmc->Def_pkt, "%d", pmc->RxBuffer[pp + 1]) ;
				sprintf(pmc->Def_fld, "%d", pmc->RxBuffer[fp]) ;

#if ((DATALEN/4)<0xff)
				fldlen = MIN(pmc->RxBuffer[fp + 1], DATALEN/4) ;
#else				
				fldlen = pmc->RxBuffer[fp + 1] ;
#endif
				unpack_string((char*)&(pmc->RxBuffer[fp + 2]), fldlen, pmc->Def_data, 0) ;

///////////     fldlen = MIN(pmc->RxBuffer[fp + 1], DATALEN-1) ;
///////////     memcpy(pmc->Def_data, &(pmc->RxBuffer[fp + 2]), fldlen) ;
///////////     pmc->Def_data[fldlen] = '\0' ;

				pmc->DataReady |= MTS_DEFAULT_DATA ;
			}
			break ;
			
		}
		fp += ((pmc->RxBuffer[fp + 1]) + 2) ;
#ifdef USE_MONITORING
	printf("\nField %s\n", comment ) ;
#endif

	}


#ifdef SW_MTSCU
	// If a history data
	if ( (Gdata.logs_type & DATA_LOGHISTORY) && (MtsData.DataReady & MTS_DATA_FIX) && (pmc->Data_src!=5)){
#ifdef USE_MONITORING
		printf("\nHist %d", Gdata.histrecv) ;
#endif
		UpdateScreen() ;
		MtsData.DataReady = 0 ;
	}
#endif

#ifdef SW_MTSCU
	if (last_hist) MtsData.DataReady = 0 ;
#endif 

}

static void ParseRepBuffer(struct _MTSVALS *pmc, int pp)
{

	int i ;
	int nf ;    // number of fields
	int fp ;    // field pointer
	char comment[DEFAULTLEN] ;
	char t_asc[DEFAULTLEN];

	nf = pmc->RxBuffer[pp + 4] ;        // number of fields
#ifdef USE_MONITORING
	printf("\nRECV PKT_REPORT: %d fields\n", nf ) ;
#endif
	fp = pp + 5 ;                       // start
	for(i=0 ; i<nf ; i++) {
		switch(pmc->RxBuffer[fp]) {
		case IDREP_IP:
#ifdef USE_MONITORING
	printf("\nRECV PKT_REPORT: IDREP_IP \n") ;
#endif
			//if (*((int *)(&(pmc->RxBuffer[fp + 2])))!=0){
			sprintf(pmc->Data_IP,"%hhu.%hhu.%hhu.%hhu", 
											pmc->RxBuffer[fp + 2],
											pmc->RxBuffer[fp + 3],
											pmc->RxBuffer[fp + 4],
											pmc->RxBuffer[fp + 5] ) ;
			pmc->Data_IPsckt = UDPSOCKET ;

			if (pmc->RxBuffer[fp + 1] > 10){
				sprintf(pmc->Data_IP2,"%hhu.%hhu.%hhu.%hhu", 
												pmc->RxBuffer[fp + 10],
												pmc->RxBuffer[fp + 11],
												pmc->RxBuffer[fp + 12],
												pmc->RxBuffer[fp + 13] ) ;
				pmc->Data_PcSckt2 = pmc->RxBuffer[fp + 16] * 256 + pmc->RxBuffer[fp + 17] ;
			
// 			If (ConnType = CONN_GPRS) Then
// 				If (Val(SysSerNum) = Mts_sn) Then
// 					If (Left(MtsIP, 4) <> "192.") Then
// 						If (RXbuffer(fp + 1) > 10) And (RXbuffer(fp + 10) > 0) Then
// 							MtsIP = SysIP2
// 						Else
// 							MtsIP = SysIP1
// 						End If
// 					End If
// 				End If
// 			End If
			//'If (Len(MtsIP) = 0) Then
//			If (Left(MtsIP, 4) <> "192.") Then MtsIP = SysIP1
			//' endif
			//If (pmc->RxBuffer[fp + 1] > (fp + 5)) Then
				pmc->Data_IPsckt = pmc->RxBuffer[fp + 6] * 256 + pmc->RxBuffer[fp + 7] ;
				pmc->Data_PcSckt = pmc->RxBuffer[fp + 8] * 256 + pmc->RxBuffer[fp + 9] ;
				//If (MtsPort = 0) Then MtsPort = SysMtsSckt
				//If (PcPort = 0) Then PcPort = SysPcSckt1
#ifdef SW_MTSCU
				if (!Gdata.NoChkIP){
					strcpy(Gdata.mts_ip, pmc->Data_IP ) ;
					Gdata.mts_socket = pmc->Data_IPsckt ;
				}
#endif // #ifdef SW_MTSCU

			}else{
				pmc->Data_IP2[0] = '\0' ;
				pmc->Data_IPsckt = UDPSOCKET ;
				pmc->Data_PcSckt = UDPSOCKET ;
			}
			
			pmc->DataReady |= MTS_DATA_IP ;
			break ;

		case IDREP_STATS :
#ifdef USE_MONITORING
	printf("\nRECV PKT_REPORT: IDREP_STATS \n") ;
#endif
			strcpy(comment,"IDREP_STATS") ;
//			if (!(pmc->DataReady & DATA_CPU)) {         // resource free
				//printf("\nSTO PER SCRIVERE SN\n");
				sprintf(pmc->Data_cpu, "%d", *((int *)(&(pmc->RxBuffer[fp + 6])))) ;
				//printf("\nHO SCRITTO SN\n");
				//printf("\nSN=%s\n",pmc->Data_cpu);
#ifdef SW_MTSCU
				Gdata.mts_sn = atol(pmc->Data_cpu) ;
// #ifdef SW_MTSCU
				//pmc->DataReady |= MTS_DATA_CPU ;
//			}
//			if (!(pmc->DataReady & DATA_SWVER)) {       // resource free
				sprintf(pmc->Data_swver, "%d.%02x", pmc->RxBuffer[fp + 10], pmc->RxBuffer[fp + 11]) ;
#endif
#ifdef MTSTESTKIT
				char ver2nd[8];
				int ver2;
				sprintf(ver2nd,"%x",pmc->RxBuffer[fp + 11]);
				ver2=atoi(ver2nd)*10;
				sprintf(pmc->Data_swver, "%d", ((pmc->RxBuffer[fp + 10]*1000)+ver2) ) ;
				//printf("Software ver=|%s|",pmc->Data_swver);
				//printf("\nHO SCRITTO SWVER\n");
#endif
				//pmc->DataReady |= MTS_DATA_SWVER ;
//			}
//			if (!(pmc->DataReady & DATA_REPORT)) {      // resource free
//				short i_loc ;
				//time_t lltime ;
					
//				lltime = *( (unsigned int*) (&(pmc->RxBuffer[fp+2]))) ;
//				printf("\nSTO PER SCRIVERE TimeDB %ld\n", lltime);
				
				sprintf(pmc->DRep_time, "%s", TimeDB((time_t *)(&(pmc->RxBuffer[fp+2])),t_asc)) ;
//				sprintf(pmc->DRep_time, "%s", TimeDB(&lltime,t_asc)) ;
				
				//printf("\nHO SCRITTO TimeDB\n");
				sprintf(pmc->DRep_avgtel, "%d", pmc->RxBuffer[fp + 13]) ;
				sprintf(pmc->DRep_pwrmain, "%d", *((short *)(&(pmc->RxBuffer[fp + 14])))) ;
				sprintf(pmc->DRep_pwrbatt, "%d", *((short *)(&(pmc->RxBuffer[fp + 16])))) ;
				sprintf(pmc->DRep_numpwr, "%d", *((short *)(&(pmc->RxBuffer[fp + 18])))) ;
				sprintf(pmc->DRep_numfix, "%d", *((int *)(&(pmc->RxBuffer[fp + 20])))) ;
				sprintf(pmc->DRep_numterm, "%d", *((short *)(&(pmc->RxBuffer[fp + 24])))) ;
				sprintf(pmc->DRep_teloper,  "%d", *((int *)(&(pmc->RxBuffer[fp + 26])))) ;
				sprintf(pmc->DRep_csqtel, "%d", pmc->RxBuffer[fp + 30]) ;
				//printf("\nHO SCRITTO FINO A CSQTEL\n");
				
				// for MTS with only a modem type
// 				i_loc = DecodeMtsType(LocMTSname) ;
// 				if (i_loc!=-1){
// 					strcpy(pmc->DRep_mtstype, Mts_default[i_loc].mtstype ) ;
// 				}else{

					switch(pmc->RxBuffer[fp + 31]){
						case 32:
						pmc->RxBuffer[fp + 31]=40;
						break ;
						case 40:
						pmc->RxBuffer[fp + 31]=32;
						break ;
					}
#ifdef SW_MTSCU
					if ((pmc->RxBuffer[fp + 1]) >= 30) {     // from 1.32
						sprintf(pmc->DRep_mtstype, "MTS%02d", pmc->RxBuffer[fp + 31]) ;
					} else {
						sprintf(pmc->DRep_mtstype, "MTS%d", 10) ;
					}
#endif 
#ifdef MTSTESTKIT
					sprintf(pmc->DRep_mtstype, "%d", pmc->RxBuffer[fp + 31]) ;
					//printf("\nHO SCRITTO FINO A MTSTYPE\n");
#endif
//				}
				//sprintf(pmc->DRep_src, "%s", src) ;
				pmc->DataReady |= MTS_DATA_REPORT ;
//			}

// 			if (pmc->DataReady & HIST_REPORT) {         // resource free
// 				InsertHistoryDb(pmc, mdm, LocMTSname) ;
// 			}

// 			if (!(pmc->DataReady & HIST_REPORT)) {      // resource free
// 				strcpy(pmc->HRep_time,    pmc->DRep_time) ;
// 				strcpy(pmc->HRep_avgtel,  pmc->DRep_avgtel) ;
// 				strcpy(pmc->HRep_pwrmain, pmc->DRep_pwrmain) ;
// 				strcpy(pmc->HRep_pwrbatt, pmc->DRep_pwrbatt) ;
// 				strcpy(pmc->HRep_numpwr,  pmc->DRep_numpwr) ;
// 				strcpy(pmc->HRep_numfix,  pmc->DRep_numfix) ;
// 				strcpy(pmc->HRep_numterm, pmc->DRep_numterm) ;
// 				strcpy(pmc->HRep_teloper, pmc->DRep_teloper) ;
// 				strcpy(pmc->HRep_csqtel,  pmc->DRep_csqtel) ;
// 				strcpy(pmc->HRep_mtstype, pmc->DRep_mtstype) ;
// 				pmc->DataReady |= HIST_REPORT ;
// 			}
			//printf("\nFINITO IDREP_STATS \n");
			break ;
			
		default :
#ifdef USE_MONITORING
	printf("\nRECV PKT_REPORT: IDREP_OTHER \n") ;
#endif
			printf(comment,"IDREP_OTHER! (%hhd)", pmc->RxBuffer[fp]) ;
			break ;
		}
		fp += ((pmc->RxBuffer[fp + 1]) + 2) ;

		//if (pmc->ModemMOD!=MOD_SOCKET) ActivityLog(comment, mdm) ;

	}
}

static void ParseCanBuffer(struct _MTSVALS *pmc, int pp)
{
int i, j ;
int nf ;            // number of fields
int fp ;            // field pointer
//long lpar;
int lpar;
char comment[DEF_STRING], cdata[MAX_STRING], lusrc ;
unsigned char c ; // *p
FILE * l_fd ;
//time_t now ;

	pmc->Data_src = pmc->RxBuffer[pp]>>4 ;
//	now = time(NULL) ;
	
	nf = pmc->RxBuffer[pp + 4] ;        // number of fields
#ifdef USE_MONITORING
	printf("\nRECV PKT_BINARY: %d fields\n", nf ) ;
#endif
	fp = pp + 5 ;                       // start
	lusrc = ((pmc->RxBuffer[pp + 0])>>4) ;
	
	for(i=0 ; i<nf ; i++) {
		
		j = 0 ;
		switch(pmc->RxBuffer[fp]) {
			// NEW CANBUS
#ifdef SW_MTSCU
			case IDBIN_CMSET:
			sprintf(cdata, "From LU%d: Tag=%d val=%lld\n", lusrc, *((unsigned int *)(&(pmc->RxBuffer[fp+2]))),
							*((long long *)(&(pmc->RxBuffer[fp+6])))) ;
			Add_txt_mts(Scrn.boxtxt_mts, cdata ) ;
			break ;
#endif
			case IDBIN_CDATA:
#ifdef SW_MTSCU
			sprintf(comment,"Candata%d.txt", lusrc) ;
			l_fd = fopen(comment, "a") ;
			if (ftell(l_fd)==0){ // New file
				fprintf(l_fd, "Date\tTag\tType\tAddress\tValue\tByte0\tByte1\tByte2\tByte2\tByte3\tByte5\tByte6\tByte7\n") ;
			}
			
			// Get tag
			sprintf(cdata, "From LU%d: Tag:%d ", lusrc, *((unsigned int *)(&(pmc->RxBuffer[fp+2]))) ) ;
			fprintf(l_fd, "%s\t%u", TimeDB(&now,comment), *((unsigned int *)(&(pmc->RxBuffer[fp+2]))) ) ;
			//Get address
			j = *((unsigned int *)(&(pmc->RxBuffer[fp+6]))) ;
			if (j == 0xFFFFFFFF){
				strcat(cdata, " CanTime     DATA:") ;
				fprintf(l_fd, "\t%s", "CanTime" ) ;
			}else{
				// Can type
				// Extended or standard
				strcat(cdata, ((j & 0x80000000)? " Ext":" Std") ) ;
				fprintf(l_fd, "\t%s", ((j & 0x80000000)? "Ext":"Std") ) ;
				// Mishmach
				if (j & 0x40000000){
					strcat(cdata, " BadAddr" ) ;
					fprintf(l_fd, " %s", "Bad" ) ;
				}

				// Only address
				j &= 0x1FFFFFFF ;
				sprintf(comment, " Addr:0x%x DATA:", j ) ;
				strcat(cdata, comment) ;
				fprintf(l_fd, "\t0x%x", j ) ;
			}
			//Data
			if ((pmc->RxBuffer[fp+1] - 8) == 4){
				sprintf(comment, "0x%x ", *((unsigned int *)(&(pmc->RxBuffer[fp+10]))) ) ;
				fprintf(l_fd, "\t0x%x",  *((unsigned int *)(&(pmc->RxBuffer[fp+10]))) ) ;
			}else if ((pmc->RxBuffer[fp+1] - 8) == 8){
				sprintf(comment, "0x%llx ", *((long long *)(&(pmc->RxBuffer[fp+10]))) ) ;
				fprintf(l_fd, "\t0x%llx",  *((long long *)(&(pmc->RxBuffer[fp+10]))) ) ;
			}
			strcat(cdata, comment) ;
			
			for ( j=0; j<(pmc->RxBuffer[fp+1] - 8) ;j++) {
				sprintf(comment, "0x%x ", pmc->RxBuffer[fp+10+j] ) ;
				strcat(cdata, comment) ;
				fprintf(l_fd, "\t0x%x",   pmc->RxBuffer[fp+10+j] ) ;
			}
			fprintf(l_fd,"\n") ;
			fclose( l_fd) ;

			Add_txt_mts(Scrn.boxtxt_mts, cdata ) ;
#endif
#ifdef MTSTESTKIT
			switch(lusrc){
				case LU13CAN:
				MtsData.lastCan13Rx.tag = *((unsigned long *)(&(pmc->RxBuffer[fp+2])));  // Get tag
				MtsData.lastCan13Rx.Address = *((unsigned long *)(&(pmc->RxBuffer[fp+6]))); // Get Address
				//Data
				for ( j=0;j<(pmc->RxBuffer[fp+1] - 8);j++) {
					sprintf(comment, "%02hhX",(pmc->RxBuffer[fp+10+j])) ;
					if(!j){ 
						strcpy(cdata,comment);
					}else{
						strcat(cdata, comment) ;
					}	
				}
				strcpy(MtsData.lastCan13Rx.data,cdata);
				break;
				case LU14CAN:
				MtsData.lastCan14Rx.tag =  *((unsigned long *)(&(pmc->RxBuffer[fp+2])));  // Get tag
				MtsData.lastCan14Rx.Address = *((unsigned long *)(&(pmc->RxBuffer[fp+6]))); // Get Address
				//Data
				for ( j=0;j<(pmc->RxBuffer[fp+1] - 8);j++) {
					sprintf(comment, "%02hhX",(pmc->RxBuffer[fp+10+j])) ;
					if(!j){ 
						strcpy(cdata,comment);
					}else{
						strcat(cdata, comment) ;
					}
				}
				strcpy(MtsData.lastCan14Rx.data,cdata);
				break;	
			}				
#endif
			strcpy(comment,"IDBIN_CDATA") ;
			break ;

			case IDBIN_CCCLEAR: // Start Recv CAN Conf
			sprintf(comment,"CanConf%d.mts", lusrc) ;
			l_fd = fopen(comment, "w") ;
			fprintf(l_fd, "[NEWLU%d]\n", lusrc) ;
			fprintf(l_fd, "baud=%d\n", *((unsigned int *)(&(pmc->RxBuffer[fp+4]))) ) ;
			fclose(l_fd) ;
			RxCan[lusrc-13] = 1 ;
			break ;

			case IDBIN_CCADD: // Next Recv CAN Conf line
			sprintf(comment,"CanConf%d.mts", lusrc) ;
			l_fd = fopen(comment, "a") ;
			fprintf(l_fd, "\n[NEWLU%d_DATA%d]\n", lusrc, RxCan[lusrc-13]++ ) ;
#ifdef SW_MTSCU
			sprintf(cdata, "CAN%d: Recv %d", lusrc, (RxCan[lusrc-13]-1) ) ;
			StatusBar(1, cdata ) ;
#endif
			// Tag
			fprintf(l_fd, "Tag=%u\n",  *((unsigned int *)(&(pmc->RxBuffer[fp+14]))) ) ;
			// Tipo
			fprintf(l_fd, "Tipo=%hd\n",  pmc->RxBuffer[fp+2] ) ;
			// CanBit
			if (pmc->RxBuffer[fp+3]>0){
				if (pmc->RxBuffer[fp+3] & 0x80){
					fprintf(l_fd, "SeCanBit=%hd\n",  ( pmc->RxBuffer[fp+3] & 0x7F) ) ;
				}else{
					fprintf(l_fd, "CanBit=%hd\n",  pmc->RxBuffer[fp+3] ) ;
				}
			}
			// InvioOgni
			if (pmc->RxBuffer[fp+2]!=WCAN_SEND){
				fprintf(l_fd, "InvioOgni=%hu\n",  *((unsigned short *)(&(pmc->RxBuffer[fp+4]))) ) ;
			}
			// Address
			fprintf(l_fd, "Indirizzo=%08lX\n",  *((unsigned long *)(&(pmc->RxBuffer[fp+6]))) ) ;
			// MaskInd
			fprintf(l_fd, "MaskInd=%08lX\n",  (*((unsigned long *)(&(pmc->RxBuffer[fp+10]))) & 0x7FFFFFFF) ) ;
			// CanStandard
			fprintf(l_fd, "CanStandard=%d\n",  (*((unsigned long *)(&(pmc->RxBuffer[fp+10]))) & 0x80000000)? 0:1 ) ;
			// InvertiByte, Transazionato, EsternoSoglie
			if ( pmc->RxBuffer[fp + 18] & WCAN_REVERSE)
				fprintf(l_fd, "InvertiByte=1\n") ;
			if ( pmc->RxBuffer[fp + 18] & WCAN_TRANS)
				fprintf(l_fd, "Transazionato=1\n") ;
			if ((pmc->RxBuffer[fp+2]==WCAN_TIME)||(pmc->RxBuffer[fp+2]==WCAN_TIMEALL)){
				if ( pmc->RxBuffer[fp + 18] & WCAN_OUTSIDE)
					fprintf(l_fd, "EsternoSoglie=1\n") ;
			}
			// Check Errore
			if ( pmc->RxBuffer[fp + 18] & WCAN_BITERR)
				fprintf(l_fd, "CheckErrore=1\n") ;
			// LuDest
			fprintf(l_fd, "LuDest=%hd\n",  pmc->RxBuffer[fp+19] ) ;
			// PrimoBitMask
			if (pmc->RxBuffer[fp+2]!=WCAN_POLLING){
				fprintf(l_fd, "PrimoBitMask=%hd\n",  pmc->RxBuffer[fp+20] ) ;
				if (pmc->RxBuffer[fp+2]!=WCAN_SEND){
					fprintf(l_fd, "LunghezzaMask=%hd\n",  pmc->RxBuffer[fp+21] ) ;
				}
				
				if (pmc->RxBuffer[fp+2]==WCAN_PHIST){
					fprintf(l_fd, "LetturaOgni=%lu\n",  *((unsigned long *)(&(pmc->RxBuffer[fp+26]))) ) ;
				}else if (pmc->RxBuffer[fp+2]==WCAN_SEND){
					fprintf(l_fd, "BitMask=0x%lx\n",  *((unsigned long *)(&(pmc->RxBuffer[fp+30]))) ) ;
					fprintf(l_fd, "ErrMask=0x%lx\n",  *((unsigned long *)(&(pmc->RxBuffer[fp+34]))) ) ;
				}else{
					float f_coeff, f_val ;
					long c_offset, c_val ;
					
					// Coeff
					f_coeff =  *((float *)(&(pmc->RxBuffer[fp+22]))) ;
					fprintf(l_fd, "Coeff=%g\n", f_coeff ) ;
					// Offset
					c_offset = *((unsigned long *)(&(pmc->RxBuffer[fp+26]))) ;
					if (c_offset!=0){
						// Coeff tempo
// 						if (pmc->RxBuffer[fp+2]==WCAN_TIME)
// 							fprintf(l_fd, "DivTempo=%ld\n",  *((long *)(&(pmc->RxBuffer[fp+26]))) ) ;
// 						else
							fprintf(l_fd, "Offset=%ld\n",  *((long *)(&(pmc->RxBuffer[fp+26]))) ) ;
					}
					if (pmc->RxBuffer[fp+2]==WCAN_PSUM){
						// PrimoBitMask2
						fprintf(l_fd, "PrimoBitMask2=%hd\n",  pmc->RxBuffer[fp+34] ) ;
						fprintf(l_fd, "LunghezzaMask2=%hd\n", pmc->RxBuffer[fp+35] ) ;
						if (*((unsigned long *)(&(pmc->RxBuffer[fp+30])))!=0)
							fprintf(l_fd, "Offset2=%ld\n",  *((long *)(&(pmc->RxBuffer[fp+30]))) ) ;

					}else if ((pmc->RxBuffer[fp+2]==WCAN_TIMEALL) || (pmc->RxBuffer[fp+2]==WCAN_TIME) || (pmc->RxBuffer[fp+2]==WCAN_COUNT) ){
						// SogliaInferiore
						c_val = *((long *)(&(pmc->RxBuffer[fp+30]))) ;
// 						if (pmc->RxBuffer[fp+2]==WCAN_TIME)
// 							f_val = (float)(c_val);
// 						else
							f_val = ((float)(c_val+c_offset))* f_coeff ;
						fprintf(l_fd, "SogliaInferiore=%g\n",  f_val ) ;
						// SogliaSuperiore
						c_val = *((long *)(&(pmc->RxBuffer[fp+34]))) ;
// 						if (pmc->RxBuffer[fp+2]==WCAN_TIME)
// 							f_val = (float)(c_val);
// 						else
							f_val = ((float)(c_val+c_offset))* f_coeff ;
						fprintf(l_fd, "SogliaSuperiore=%g\n", f_val ) ; 
					}
				}
			}
			
			fclose(l_fd) ;
			break ;

			case IDBIN_CCSTART: // End Recv CAN Conf
			if (RxCan[lusrc-13]){
#ifdef SW_MTSCU
				sprintf(cdata, "CAN%d: Recv ALL(%d)", lusrc, (RxCan[lusrc-13]-1) ) ;
				StatusBar(1, cdata ) ;
#endif
				sprintf(comment,"CanConf%d.mts", lusrc) ;
				l_fd = fopen(comment, "a") ;
				fprintf(l_fd, "\n[END%d]\n", lusrc  ) ;
				fprintf(l_fd, "rec_all=YES\n"  ) ;
				fclose(l_fd) ;
			}else{
#ifdef SW_MTSCU
				sprintf(cdata, "CAN%d: Sended OK\n", lusrc ) ;
				Add_txt_mts(Scrn.boxtxt_mts, cdata ) ;
#endif
				Gdata.okcansend=255;
			}
			RxCan[lusrc-13] = 0 ; 
			break ;

			// OLD CANBUS
			case IDBIN_CCANBUS:
			case IDBIN_BCANBUS:
#ifdef MTSTESTKIT
//		        switch(*((unsigned int *)(&(pmc->RxBuffer[fp+2])))){
		        switch(pmc->RxBuffer[fp+2]){
		      	        case 1:    // data
		                    j = 1;
						break ;
		                case 2:    // NO_CANBUS
						break ;
		                case 4:    // ANSWER TO SEND DATA
						break ;
		                default:
 						//if ( *((unsigned int *)(&(pmc->RxBuffer[fp+1])))> 6) { 										
						if ( pmc->RxBuffer[fp+1]> 6) {
		                        switch(lusrc){
		                            case LU13CAN:
		                                //Get tag
		                                MtsData.lastCan13Rx.tag = 1 ;
		                                //Get address
		                                lpar =  *((unsigned int *)(&(pmc->RxBuffer[fp+2]))) ;  //prima long
										lpar = lpar & 0xFFFFFF ;
		                                if (lpar < 0xA00000){
		                                    MtsData.lastCan13Rx.Address = lpar ;										
											for ( j=0; j<8;j++) {
												sprintf(comment, "%02hhX",(pmc->RxBuffer[fp+7+j])) ;
												if (!j){
													strcpy(cdata, comment) ;
												}else{
													strcat(cdata, comment) ;
												}
											}
											strcpy(MtsData.lastCan13Rx.data,cdata);
										}
									break ;
									case LU14CAN:
										//Get tag
										MtsData.lastCan14Rx.tag = 1 ;
										//Get address
										lpar =  *((unsigned int*)(&(pmc->RxBuffer[fp+2]))) ;
										lpar = lpar & 0xFFFFFF ;
										if (lpar < 0xA00000){
											MtsData.lastCan14Rx.Address = lpar ;
											for ( j=0; j<8;j++) {
												sprintf(comment, "%02hhX",(pmc->RxBuffer[fp+7+j])) ;
												if (!j){
													strcpy(cdata, comment) ;
												}else{
													strcat(cdata, comment) ;
												}
											}
											strcpy(MtsData.lastCan14Rx.data,cdata);
										}
									break ;
								}
							}
						break;
				}
#endif
			break ;

			default:
			c = pmc->RxBuffer[fp] ;
			if ( ((c>0) && (c<27)) || (c>30) ){
				
			}
			break ;
		}
		fp += ((pmc->RxBuffer[fp + 1]) + 2) ;
	}
}

// Others
static void ParseDefaultBuffer(struct _MTSVALS *pmc, int pp)
{
}

