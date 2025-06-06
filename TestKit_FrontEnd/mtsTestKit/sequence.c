//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//
#include <gtk/gtk.h>

#include <unistd.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>

#include <sys/timeb.h>

#include "cmdpslave.h"

#include "mtsTestKit.h" 
#include "mtsGTK.h"
#include "protocol.h"

//for eth
#include <arpa/inet.h>
#include <netinet/in.h>


// Data for sequence comms
#define STR_CHILD_EXITED "***>child_terminated|0"
#define ERROR_STRING_ANSWER "+++ERROR="
#define START_ANSWER  "A:"

// Return code for function
#define CODE_ANSWER_OK     0
#define CODE_WAIT_ANSWER  -1  // Wait data from TK or MTS
#define CODE_WAIT_USER    -2  // Wait input from user
// Other value are error 

#define ERR_ANSWER_TIMEOUT 1

char NextAnswer[MAX_STRING] ;
char dummy[DEF_STRING] ;

// Possibili risposte
// strcpy(NextAnswer,"0") ; default
// strcpy(NextAnswer, risposta stringa)
// sprintf(NextAnswer, "   ", risposta numerica)
// return(0) ; -> risposta subito

// 	Gsequence.TimeoutAnswer = 0;
// 	Gdata.lastSlaveCommandId = WSCMD_U_MSGBOX ;
// 	return(CODE_WAIT_USER);

// 
// 	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_TK ;
// 	Gdata.lastSlaveCommandId = WSCMD_T_GETVER ;
// 	return(CODE_WAIT_ANSWER);

int hex_to_int(char c){

	if(c >=97)
		c=c-32;
		
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

void text_to_hex(int len,char *is_ascii_string, char *os_hex_string)
{
   int i;
	 if (len==0) len=strlen(is_ascii_string);
	
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


int  donull(int argc, char *argv[]) ;
int  doDELAY(int argc, char *argv[]) ;
int  doexit(int argc, char *argv[]) ;
int  doGetBestFileNum(int argc, char *argv[]) ;
int  doGetINIKeyVal(int argc, char *argv[]) ;
int  doGetWorkSpace(int argc, char *argv[]) ;
int  doInputBox(int argc, char *argv[]) ;
int  doinputExtRefresh(int argc, char *argv[]) ;
int  doinputExtA(int argc, char *argv[]) ;    
int  doinputExtB(int argc, char *argv[]) ;
int  doinputExtC(int argc, char *argv[]) ;   
int  doinputExtD(int argc, char *argv[]) ;
int  doinputExtE(int argc, char *argv[]) ; 
int  doMsgBox(int argc, char *argv[]) ;   
int  doMsgFile(int argc, char *argv[]) ;
int  doMSGWINDOW(int argc, char *argv[]) ;
int  doM_ANALOG(int argc, char *argv[]) ;       
int  doM_CANADD(int argc, char *argv[]) ;       
int  doM_CANCLEAR(int argc, char *argv[]) ;     
int  doM_CANSTART(int argc, char *argv[]) ;     
int  doM_CanCheck(int argc, char *argv[]) ;     
int  doM_CanClearBuffer(int argc, char *argv[]) ;
int  doM_Cnt(int argc, char *argv[]) ;          
int  doM_DelPar(int argc, char *argv[]) ;     
int  doM_DIAG(int argc, char *argv[]) ;         
int  doM_EXESM(int argc, char *argv[]) ;        
int  doM_GetDirect(int argc, char *argv[]) ;    
int  doM_GETFAMILY(int argc, char *argv[]) ;    
int  doM_GETGPSFLAGS(int argc, char *argv[]) ;  
int  doM_GetIMEI(int argc, char *argv[]) ;      
int  doM_GetPar(int argc, char *argv[]) ;       
int  doM_GETSERNUM(int argc, char *argv[]) ;
int  doM_GetSmFile(int argc, char *argv[]) ;
int  doM_SENDCANCONF(int argc, char *argv[]);
int  doM_SETSOURCEID(int argc, char *argv[]) ;
int  doM_GETSWVERS(int argc, char *argv[]) ;
int  doM_GETHWVERS(int argc, char *argv[]) ;
int  doM_DirectTOLU(int argc, char *argv[]) ;
int  doM_GETTIME(int argc, char *argv[]) ;      
int  doM_INPUT(int argc) ;        
int  doM_INSTATUS(int argc, char *argv[]) ;     
int  doM_INVIRT(int argc, char *argv[]) ;       
int  doM_OUTPUT(int argc, char *argv[]) ;       
int  doM_PROGRAM(int argc, char *argv[]) ;      
int  doM_PutSmFile(int argc, char *argv[]) ;
int  doM_PutFWFile(int argc, char *argv[]) ;     
int  doM_ReadIntAD(int argc, char *argv[]) ;    
int  doM_SetPar(int argc, char *argv[]) ;       
int  doM_SETSTATUS(int argc, char *argv[]) ;    
int  doM_SetTime(int argc, char *argv[]) ;      
int  doOuputText(int argc, char *argv[]) ;    
int  doProgressBar(int argc, char *argv[]) ;    
int  doSetINIKeyVal(int argc, char *argv[]) ;   
int  doSetLevelDebug(int argc, char *argv[]) ; //_FR - Rel 3.76 - 26/05/23
int  doSetProtocolMode(int argc, char *argv[]) ;
int  doSetProtocolComunication(int argc, char *argv[]) ;
int  doT_ANALOG(int argc, char *argv[]) ;       
int  doT_CheckRec(int argc, char *argv[]) ;
int  doT_CNT(int argc, char *argv[]) ;          
int  doT_ComSend(int argc, char *argv[]) ;
int  doT_SetFTDI(int argc, char *argv[]) ;  
int  doT_EmitCanFrame(int argc, char *argv[]) ; 
int  doT_GetVer(int argc, char *argv[]) ; 
int  doT_GetType(int argc, char *argv[]) ; 
int  doT_Led(int argc, char *argv[]) ;    
int  doT_OUTPUT(int argc, char *argv[]) ;
int  doT_SetPull(int argc, char *argv[]);
int  doT_SetComPort(int argc, char *argv[]) ;
int  doT_SetCanBaudrate(int argc, char *argv[]) ;
int  doT_SetCanMailbox(int argc, char *argv[]) ;
int  doTerzista(int argc, char *argv[]) ;


struct CMDSLAVE maincmds[] = {
	{"DELAY"           , doDELAY, },
	{"exit"            , doexit, },
	{"GetBestFileNum"  , doGetBestFileNum, },
	{"GetINIKeyVal"    , doGetINIKeyVal, },
	{"GetWorkSpace"    , doGetWorkSpace, },
	{"InputBox"        , doInputBox, },
	{"inputExtRefresh" , doinputExtRefresh, },
	{"inputExtA"       , doinputExtA, },    
	{"inputExtB"       , doinputExtB, },
	{"inputExtC"       , doinputExtC, },   
	{"inputExtD"       , doinputExtD, },
	{"inputExtE"       , doinputExtE, },
	{"MsgBox"          , doMsgBox, },   
	{"MsgFile"         , doMsgFile, },
	{"MSGWINDOW"       , doMSGWINDOW, },
	
	{"M_ANALOG"        , doM_ANALOG, },       
	{"M_CANADD"        , doM_CANADD, },       
	{"M_CANCLEAR"      , doM_CANCLEAR, },     
	{"M_CANSTART"      , doM_CANSTART, },     
	{"M_CanCheck"      , doM_CanCheck, },     
	{"M_CanClearBuffer", doM_CanClearBuffer, },
	{"M_Cnt"           , doM_Cnt, },          
	{"M_DelPar"        , doM_DelPar, },     
	{"M_DIAG"          , doM_DIAG, },         
	{"M_EXESM"         , doM_EXESM, },        
	{"M_GetDirect"     , doM_GetDirect, },    
	{"M_GETFAMILY"     , doM_GETFAMILY, },    
	{"M_GETGPSFLAGS"   , doM_GETGPSFLAGS, },  
	{"M_GetIMEI"       , doM_GetIMEI, },      
	{"M_GetPar"        , doM_GetPar, },       
	{"M_GETSERNUM"     , doM_GETSERNUM, },
	{"M_GetSmFile"     , doM_GetSmFile, },  
	{"M_SETSOURCEID"   , doM_SETSOURCEID, },
	{"M_SENDCANCONF"   , doM_SENDCANCONF, },
	{"M_GETSWVERS"     , doM_GETSWVERS, },
	{"M_GETHWVERS"     , doM_GETHWVERS, },
	{"M_DirectTOLU"    , doM_DirectTOLU, },
	{"M_GETTIME"       , doM_GETTIME, },      

	{"M_INPUT"         , doM_INPUT, },        
	{"M_INSTATUS"      , doM_INSTATUS, },     
	{"M_INVIRT"        , doM_INVIRT, },       
	{"M_OUTPUT"        , doM_OUTPUT, },       
	{"M_PROGRAM"       , doM_PROGRAM, },      
	{"M_PutSmFile"     , doM_PutSmFile, },
	{"M_PutFWFile"     , doM_PutFWFile, },    
	{"M_ReadIntAD"     , doM_ReadIntAD, },    
	{"M_SetPar"        , doM_SetPar, },       
	{"M_SETSTATUS"     , doM_SETSTATUS, },    
	{"M_SetTime"       , doM_SetTime, },      
	
	{"OuputText"       , doOuputText, },    
	{"ProgressBar"     , doProgressBar, },    
	{"SetINIKeyVal"    , doSetINIKeyVal, },   
	{"SetLevelDebug"   , doSetLevelDebug, },   
	{"SetProtocolMode" , doSetProtocolMode, },
	{"SetProtocolComunication" , doSetProtocolComunication, },
	{"T_ANALOG"        , doT_ANALOG, },       
	{"T_CheckRec"      , doT_CheckRec, },     
	{"T_CNT"           , doT_CNT, },          
	{"T_ComSend"       , doT_ComSend, },
	{"T_SetFTDI"       , doT_SetFTDI, },  
	{"T_EmitCanFrame"  , doT_EmitCanFrame, }, 
	{"T_GetVer"        , doT_GetVer, }, 
	{"T_GetType"       , doT_GetType, },
	{"T_Led"           , doT_Led, },    
	{"T_OUTPUT"        , doT_OUTPUT, },
	{"T_SetPull"       , doT_SetPull, },
	{"T_SetComPort"    , doT_SetComPort, },
	{"T_SetCanBaudrate", doT_SetCanBaudrate, },
	{"T_SetCanMailbox" , doT_SetCanMailbox, },
	{"Terzista"        , doTerzista, },
	{""				   , donull,},
	{NULLCHAR		   , donull}
} ;

static int mtskick_lu = 11 ;

struct locCanConfline {
int		Address ;
int		Mask ; 
};	

// STRUCTURE for CAN OLD
struct _LocCanConf {
int  	newcan ; 	// Booleano if newcan
char    busTiming1[1] ; 
char    busTiming2[1] ;
int		CanChannel;
int		TimeSend;
int		tot_addr;
struct locCanConfline  channel[5] ;
};

struct _LocCanConf locCanConf;

//**************************

int  Start_sequence(void)
{
int i ;
struct timeb loc_time ;

	// Initialize screen
	for(i=0;i<22;i++){
		gtk_widget_modify_bg( Scrn.clr_step[i], GTK_STATE_NORMAL, &vb_color[VB_LGRAY]) ;
		gtk_label_set_text(GTK_LABEL(Scrn.txt_step[i]), "") ;
	}
	// Initialize start time
	ftime(&loc_time) ;
	Gsequence.secStartTime = loc_time.time ;
	Gsequence.milliStartTime = loc_time.millitm ;
	Gdata.lastSlaveCommandId = WSCMD_NONE ;
	
	return(run_sequence()) ;
}

int ActCsec(void)
{
struct timeb ltime ;
int retval ;

	ftime(&ltime);
	retval = ((ltime.time - Gsequence.secStartTime) * 100 ) ;
	retval += ((ltime.millitm - Gsequence.milliStartTime)/ 10 ) ;
	
	return(retval) ;
}

static void Send_sequence_error(int n_err)
{
char cnfbuf[DEF_STRING] ;

	sprintf(cnfbuf, "%s%s%04d\n", START_ANSWER, ERROR_STRING_ANSWER, n_err ) ;
	
#ifdef USE_MONITORING_
			Add_txt_mts(Scrn.boxtxt_mts, cnfbuf) ;
#endif // USE_MONITORING
	Send_sequence_answer(cnfbuf) ;
	Gdata.sequence_status = SEQ_WAITCOMMAND ; 

}

static void Send_sequence_message(void)
{
char cnfbuf[2*MAX_STRING], *p ;

	sprintf(cnfbuf, "%s%s\n", START_ANSWER, NextAnswer ) ;
	
	// replace ' ' with char '128'
	p = cnfbuf ;
	while ((p=strchr(p, ' '))!=NULL){
		*p = 128 ;
		p++ ;
	};
	
#ifdef USE_MONITORING_
			Add_txt_mts(Scrn.boxtxt_mts, cnfbuf) ;
#endif // USE_MONITORING
	Send_sequence_answer(cnfbuf) ;
	Gdata.sequence_status = SEQ_WAITCOMMAND ; 
	
}

void Get_sequence_command(void)
{
int i ;
char recv_cmd[MAX_STRING] ;

	if (!Check_sequence()){
		if (Gsequence.NewCommand){

#ifdef USE_MONITORING_
			//printf("\nrecv %d\n", dwRead) ;
			//printf("\ncc:%s\n", Gsequence.Command) ;
			sprintf( recv_cmd, "%s\n", Gsequence.Command) ;
			Add_txt_mts(Scrn.boxtxt_mts, recv_cmd) ;
#endif // USE_MONITORING

			if (!strncmp(Gsequence.Command, "***>", 4)) {
				strcpy(recv_cmd, &Gsequence.Command[4]) ;
				i = strlen(recv_cmd) ;
				if (recv_cmd[i-1]!='|'){
					recv_cmd[i]='|';
					recv_cmd[i+1]='\0';
				}
				strcpy(NextAnswer,"0") ;			// Default
				Gsequence.TimeoutAnswer = 0 ;
				Gdata.lastSlaveCommandId = WSCMD_NONE ;
				i=cmdpslave(maincmds,recv_cmd);
				Gsequence.NewCommand = 0 ;
				Gsequence.Command[0] = '\0' ;
				if (i){
					if (i==CODE_WAIT_ANSWER){ // Wait answer
						Gdata.sequence_status = SEQ_TOANSWER ;
					}else if (i==CODE_WAIT_USER){ // Wait user answer
						Gdata.sequence_status = SEQ_USERANSWER ;
					}else
						Send_sequence_error(i) ;
				}else
					Send_sequence_message() ;
			}else{
				Gsequence.NewCommand = 0 ;
				Gsequence.Command[0] = '\0' ;
			}
		}
	/*
	}else{
		printf("\nCHECK SEQUENCE ERROR\n") ;
	*/
	}

}

void Check_sequence_timeout(void)
{
unsigned short hi ;
int act_csec ;
int curr_csec;
unsigned long ll ;

	if (!Check_sequence()){
		NextAnswer[0]='\0' ;
		switch(Gdata.lastSlaveCommandId){
			case WSCMD_WAIT_TKCOM:
				printf("\nWAIT_TKCOM_ExtData.ComsRxNr=%d\n", ExtData.ComsRxNr[ExtData.portselect]) ;
				if (ExtData.ComsRxNr[ExtData.portselect]>=ExtData.linput){
					int i;
					memcpy(NextAnswer,&ExtData.ComsRxBuffer[ExtData.portselect],ExtData.linput);
					ExtData.ComsRxNr[ExtData.portselect]=0;
					for(i=0;i<MAX_STRING;i++) ExtData.ComsRxBuffer[ExtData.portselect][i]='\0' ;
				}
				break ;
			case WSCMD_T_GETVER:
				if (ExtData.VerSn[0]) strcpy(NextAnswer, ExtData.VerSn) ;
				break ;
			case WSCMD_T_GETDIGITAL:
				if (ExtData.digitalsUpdated) strcpy(NextAnswer, "1") ;
				break ;

			case WSCMD_M_DIAG0:
				if (MtsData.diagdata[0] == 0){
					if (Gdata.lastSlaveSubCommandId==2){
						hi = *((unsigned short *)(&MtsData.diagdata[2])) ;
					}else
						hi = MtsData.diagdata[Gdata.lastSlaveSubCommandId] ;
					sprintf(NextAnswer, "%hd", hi) ;
				}
				break ;
			case WSCMD_M_DIAG1:
				if (MtsData.diagdata[0] == 1){
					hi = MtsData.diagdata[1] ;
					sprintf(NextAnswer, "%hd", hi) ;
				}
				break ;
			case WSCMD_M_DIAG2:
				if (MtsData.diagdata[0] == 2){
					ll = MtsData.diagdata[1] ;
					hi = *((unsigned short *)(&MtsData.diagdata[2])) ;
					ll *= 65536 ;
					ll += hi ;
					for(hi=0;hi<3;hi++) printf("\nDIAG=<%lu>\n",ll);
					sprintf(NextAnswer, "%lu", ll) ;
				}
				break ;
			case WSCMD_M_DIAG3:
				if (MtsData.diagdata[0] == 3){
					if ( MtsData.diaglen<6 ) MtsData.diagdata[5] = 0;
					sprintf(NextAnswer, "%02hx%02hx%02hx", MtsData.diagdata[3],MtsData.diagdata[4],MtsData.diagdata[5]) ;
				}
				break ;
			case WSCMD_M_DIAG4:
				if (MtsData.diagdata[0] == 4){
					int i;
					for(i=1;i<MtsData.diaglen;i++){
						if (MtsData.diagdata[i]<0) MtsData.diagdata[i]='_';
						if (MtsData.diagdata[i]==32) MtsData.diagdata[i]='_';
						if (MtsData.diagdata[i]==167) MtsData.diagdata[i]='$';
					}
					memcpy(NextAnswer, &MtsData.diagdata[1], MtsData.diaglen-1) ;
					NextAnswer[MtsData.diaglen] = '\0' ;
				}
				break ;
			case WSCMD_M_DIAG7:
				if (MtsData.diagdata[0] == 7){
					hi = MtsData.diagdata[1] ;
					sprintf(NextAnswer, "%hd", hi) ;
				}
				break ;
			case WSCMD_M_DIAG9:
				if (MtsData.diagdata[0] == 9){
					hi = MtsData.diagdata[1] ;
					sprintf(NextAnswer, "%hd", hi) ;
				}
				break ;
			case WSCMD_M_DIAG12:
				if (MtsData.diagdata[0] == 12){
					memcpy(NextAnswer, &MtsData.diagdata[1], MtsData.diaglen) ;
					NextAnswer[MtsData.diaglen] = '\0' ;
					char texthex[5000];
					text_to_hex(MtsData.diaglen,NextAnswer,texthex);
					printf("\nDIAG%d(%d):%s\n",Gdata.rDiag,MtsData.diaglen,texthex);
					strcpy(NextAnswer,texthex);
					printf("\nDIAG12:%s\n",NextAnswer);
				}
				break ;
			case WSCMD_M_DIAG25:
				if (MtsData.diagdata[0] == 25){
					int i;
					char llb[20] ;
					sprintf(NextAnswer, "%d", MtsData.diagdata[1]) ;
					for(i=0;i<(MtsData.diagdata[1] & 0x7f);i++){
						sprintf(llb, " (%hd)",
						        *((unsigned short *)(&MtsData.diagdata[(i*2)+2]))) ;
						strcat(NextAnswer, llb) ;
					}
					printf("\nDIAG25:%s\n",NextAnswer);
				}
				break ;
			case WSCMD_M_DIAG127:
				if (MtsData.diagdata[0] == 127){
					hi = *((unsigned short *)(&MtsData.diagdata[1])) ;
					sprintf(NextAnswer, "%hd", hi) ;
				}
				break ;
			case WSCMD_M_DIAG250:
				if (MtsData.diagdata[0] == 250){
					hi = MtsData.diagdata[1] ;
					sprintf(NextAnswer, "%hd", hi) ;
				}
				break ;
			case WSCMD_M_DIAGUNKNOWN:
				if (MtsData.diagdata[0] == Gdata.rDiag){
					memcpy(NextAnswer, &MtsData.diagdata[1], MtsData.diaglen) ;
					NextAnswer[MtsData.diaglen] = '\0' ;
					printf("\nDIAG%d(%d):%s\n",Gdata.rDiag,MtsData.diaglen,NextAnswer);
					char texthex[5000];
					text_to_hex(MtsData.diaglen,NextAnswer,texthex);
					printf("\nDIAG%d(%d):%s\n",Gdata.rDiag,MtsData.diaglen,texthex);
				}
				break ;
			case WSCMD_M_CNT:
				switch(Gdata.lastSlaveSubCommandId){
					case 0:
						if (MtsData.Data_iokm[0]) strcpy(NextAnswer, MtsData.Data_iokm) ;
						break ;
					case 1:
						if (MtsData.Data_cnt1[0]) strcpy(NextAnswer, MtsData.Data_cnt1) ;
						break ;
					case 2:
						if (MtsData.Data_cnt2[0]) strcpy(NextAnswer, MtsData.Data_cnt2) ;
						break ;
				}
				break ;
			case WSCMD_M_ANALOG:
				if (MtsData.Data_ioad[Gdata.lastSlaveSubCommandId][0])
					strcpy(NextAnswer, MtsData.Data_ioad[Gdata.lastSlaveSubCommandId]) ;
				break ;
			case WSCMD_M_FAMILY:
				if (MtsData.DRep_mtstype[0]) strcpy(NextAnswer, MtsData.DRep_mtstype) ;
				break ;
			//_GT//
			case WSCMD_M_CANSTART:
        if (Gdata.okcansend==255){
            Gdata.okcansend=0;
            strcpy(NextAnswer,"OKCANCONF") ;    
        }
				break ;
			case WSCMD_M_EXESM255:
				//printf("\nGdata.RISP255(4)=%d\n", Gdata.RISP255) ;
				if (MtsData.Dir_data[0]) {
					strcpy(NextAnswer,MtsData.Dir_data);
					MtsData.Dir_data[0]='\0' ;
					Gdata.RISP255=0;
					//printf("\nGdata.RISP255(5)=%d\n", Gdata.RISP255) ;
				}
				break ;
			case WSCMD_M_PUTSMFILE:
			 	if (Gdata.down_sm==NULL) strcpy(NextAnswer,"1") ;
				break ;
			case WSCMD_M_PUTFWFILE:
				curr_csec = ActCsec() ;
				#ifdef USE_MONITORING
				if (!(curr_csec % 100)) printf("\nDown_fw_tout %d[MAX:1000] %d\n", curr_csec,Gdata.down_fw_tout ) ;
				#endif // USE_MONITORING
				if (Gdata.down_fw==NULL){
						com_baud(MTS_current_PORT, baud_select(9600) );
						SLEEP(500) ;
						sprintf(NextAnswer,"%02hx%02hx",Gdata.bcrcl,Gdata.bcrch);
						//strcpy(NextAnswer,"1") ;
				}else{
					if (!(curr_csec % 100)){
						if (Gdata.down_fw_tout==0){
					#ifdef USE_MONITORING
							printf("\nDown_fw_tout_END %d\n", Gdata.down_fw_tout ) ;
					#endif // USE_MONITORING						
							step_fw(1);
					}
						else
							Gdata.down_fw_tout--;
					}
				}
				break ;
			case WSCMD_M_GETSMFILE:
			 	if (Gdata.up_sm==NULL) strcpy(NextAnswer,"1") ;
				break ;
			case WSCMD_M_FIX:
				if (MtsData.Data_fixtype[0]) strcpy(NextAnswer, MtsData.Data_fixtype) ;
				break ;
			case WSCMD_M_GETINTERNAL_AD:
				if (MtsData.Data_intad[0][0]) {
					int loc_i;					
					int mom_i;
					for(loc_i=0;loc_i<7;loc_i++){
						//printf("\nSTO PER RISPONDERE CON Data_intad[%d]=%s\n",loc_i,MtsData.Data_intad[loc_i]);
						strcat(NextAnswer, MtsData.Data_intad[loc_i]) ;
						strcat(NextAnswer,"&");
					}
					//salto fino a id 14 fino a 16
					for(loc_i=0;loc_i<3;loc_i++){
						for(mom_i=0;mom_i<=MAX_INTAD;mom_i++){
							if ( MtsData.Id_intad[mom_i]==(14+loc_i) ) {
								//printf("\nSTO PER RISPONDERE CON Data_intad[%d]=%s\n",(14+loc_i),MtsData.Data_intad[mom_i]);
								strcat(NextAnswer, MtsData.Data_intad[mom_i]) ;
								strcat(NextAnswer,"&");
							}
						}
					}
				//printf("\nSTO PER RISPONDERE CON Stringa=%s\n",NextAnswer);
				}
				break ;
			case WSCMD_M_GETHWVERS:
				if (MtsData.Data_auxad[0][0]) {
					strcat(NextAnswer, MtsData.Data_auxad[0]) ;
					strcat(NextAnswer,"&");
					strcat(NextAnswer, MtsData.Data_auxad[1]) ;
				}
				break ;
			case WSCMD_M_GETDUPSMFLAGS:	
				if (MtsData.Data_ioflags[0]) strcpy(NextAnswer, MtsData.Data_ioflags) ;
				break ;
			case WSCMD_M_GETDUPSMSTATUS:	
				if (MtsData.Data_iostat[0]) strcpy(NextAnswer, MtsData.Data_iostat) ;
				break ;
			case WSCMD_M_INPUT:
				if (MtsData.Data_iotime[0]) strcpy(NextAnswer,MtsData.Data_ioinp) ;
				break ;
			case WSCMD_M_GETTIME:
				if (MtsData.Data_iotime[0]) sprintf(NextAnswer, "%d", MtsData.mts_iotime) ;
				break ;
			case WSCMD_M_GETSWVERS:
				if (MtsData.Data_swver[0]) strcpy(NextAnswer,MtsData.Data_swver) ;
				break;
			case WSCMD_M_GETSERNUM:
				//printf("\nSTO PER RISPONDERE SN\n");
				if (MtsData.Data_cpu[0]) {
					//printf("\nSTO PER RISPONDERE CON SN=%s\n",MtsData.Data_cpu);
					//SLEEP(1000000);
					strcpy(NextAnswer, MtsData.Data_cpu) ;
				}
				break;
			case WSCMD_M_SETPAR:
				if (MtsData.setok==6) { // if ACK;	
					strcpy(NextAnswer,"OKSETPAR"); //ok
					MtsData.setok='\0';
				}
				break;
			case WSCMD_M_GETPAR:
				if (strlen(MtsData.parval)>=1) {
					strcpy(NextAnswer, MtsData.parval) ;
					printf("\nM_GETPAR_risposta=<%s> NextAnswer=<%s>\n",MtsData.parval,NextAnswer);
					MtsData.parval[0]='\0';
				}
				break;
			case WSCMD_M_GETDIRECT:
				if (MtsData.Dir_data[0]) {
					printf("\nM_GETDIRECT_risposta=<%s>\n",MtsData.Dir_data);
					strcpy(NextAnswer,MtsData.Dir_data);
					CLEAR_MEM(&MtsData.Dir_data, MAX_STRING ) ;
				}
				break;
			case WSCMD_AT_OK:
				if (!strncasecmp(MtsData.Dir_data,"OK",2)) {
					com_write(MTS_current_PORT, 8, "AT+CGSN\r" ) ;
					Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
					Gdata.lastSlaveCommandId = WSCMD_AT_IMEI ;
					MtsData.Dir_data[0]='\0';
				}
				break;
			case WSCMD_AT_IMEI:
				if (strlen(MtsData.Dir_data)>=15) {
					strcpy(MtsData.Imei,MtsData.Dir_data) ;
					strcpy(NextAnswer, MtsData.Imei) ;
					MtsData.Dir_data[0]='\0' ;
				}
				break;	
			//GT//
			case WSCMD_U_INBOX:
			case WSCMD_U_MSGBOX:
				if (Gdata.ib_data[0]) {
				strcpy(NextAnswer, Gdata.ib_data) ;
				//printf("\nrisposta=<%s>\n",Gdata.ib_data);
				}
				break ;
		}
		if (NextAnswer[0]){
			Send_sequence_message() ;
			Gdata.sequence_status = SEQ_WAITCOMMAND ; 
		}else if (Gsequence.TimeoutAnswer){
			act_csec = ActCsec() ;
			
#ifdef USE_MONITORING_
	if (!(act_csec % 10)) printf("\nTimeout %d %d\n", act_csec, Gsequence.TimeoutAnswer ) ;
#endif // USE_MONITORING
			// Timeout expired
			if ( act_csec > Gsequence.TimeoutAnswer ){
#ifdef USE_MONITORING
	printf("\nFncTimeout %d %d\n", act_csec, Gsequence.TimeoutAnswer ) ;
#endif // USE_MONITORING
				if (Gdata.down_sm!=NULL) { 
					fclose(Gdata.down_sm) ;	
					Gdata.down_sm=NULL;
				}
				if (Gdata.down_fw!=NULL) { 
					fclose(Gdata.down_fw) ;	
					Gdata.down_fw=NULL;
				}
				if (Gdata.up_sm!=NULL) { 
					fclose(Gdata.up_sm) ;	
					Gdata.up_sm=NULL;
					}
				if (Gdata.lastSlaveCommandId==WSCMD_U_TIMER){
					strcpy(NextAnswer,"0") ;
					Send_sequence_message() ;
				}else
					Send_sequence_error(ERR_ANSWER_TIMEOUT) ;
			}
		}
	}
}

// *********************************************************************
// *********************************************************************

int donull(int aragdatagc, char *argv[])
{
	printf("\nSEQUENCE: Unknow command %s\n", argv[0] ) ;
	return(CODE_WAIT_USER) ;
}

// *********************************************************************

int  doDELAY(int argc, char *argv[])
{
int lwait ;
	
	lwait = 0 ;
	if (argc>1)
		lwait = atoi(argv[1]) ;

	if (!lwait) lwait = TIMEOUT_TK ;
	//Parametro in decimi di secondo, invece il timeout in centesimi
	Gsequence.TimeoutAnswer = ActCsec() + lwait*10 ;

	Gdata.lastSlaveCommandId = WSCMD_U_TIMER ;
	return(CODE_WAIT_ANSWER);
}

// *********************************************************************

int  doexit(int argc, char *argv[]) // _GT_OK
{
	printf("\nGO_TO_EXIT\n");
	curtest=0;
	waithuman=0;
	com_write(PORT_TK, -1, "T 0\r") ;
	return(0);
}

// *********************************************************************
// argv[1]= dir, argv[2]=prefix file name
int  doGetBestFileNum(int argc, char *argv[])
{
int lastmax, i, iref ;
char strLastmax[10] ;
DIR *ldir ; 
struct dirent *afile ;
char *p = NULL ;


	if (argc<3) return(1) ;

	iref = strlen(argv[2]) ;
	
	lastmax = 0 ;
	strcpy(strLastmax, "0") ;

	//rem_duble_slash(argv[1], argv[1]) ;
	
	ldir = opendir(argv[1]) ;
	if (ldir!=NULL){
		while (( afile=readdir(ldir))!=NULL){
#ifdef USE_MONITORING
			printf("\nDir %s\n", afile->d_name) ;
#endif // USE_MONITORING
			
			if (afile->d_name[0]!='.'){		// not "." & ".."
				if (!strncasecmp(afile->d_name, argv[2], iref-1)){
#ifdef USE_MONITORING
				printf("\nFounded %s\n", afile->d_name) ;
#endif // USE_MONITORING
					p = strchr(afile->d_name, '.') ;
					if (p!=NULL){
						i = atoi(&afile->d_name[iref]) ;
						if (i>=lastmax){
							lastmax = i ;
							i = iref ;
							while(afile->d_name[i]!='.'){
								strLastmax[i-iref] = afile->d_name[i] ;
								i++ ;
							}
							strLastmax[i-iref] = '\0'  ;
						}
					}
				}
			}
		}
		closedir(ldir) ;
	}else{
#ifdef USE_MONITORING
		printf("\nError opening dir %s\n", argv[1]) ;
#endif // USE_MONITORING
		return(1) ;
	}
#ifdef USE_MONITORING
	printf("\nBest is  %s\n", strLastmax) ;
#endif // USE_MONITORING
	
	strcpy(NextAnswer,strLastmax) ;
	return(0);
}

// *********************************************************************

int  doGetINIKeyVal(int argc, char *argv[]) {
	if (argc<2) return(1);
	
	char nextip[MAX_STRING];
	strcpy(nextip,"next_");
	strcat(nextip,Gdata.hostname);
	
	NextAnswer[0] = '\0' ;
	
	if (!strcasecmp(argv[1], "workingPath")) {
		strcpy(NextAnswer, Gdata.workingPath) ;
		
	}else if (!strcasecmp(argv[1], "MtsName")) {
		strcpy(NextAnswer, Gdata.MtsName) ;
		
	}else if (!strcasecmp(argv[1], "FileImpostazioni")) {
		strcpy(NextAnswer, Gdata.FileImpostazioni) ;
		
	}else if (!strcasecmp(argv[1], "prgFileRadix")) {
		strcpy(NextAnswer, Gdata.prgFileRadix) ;
		
	}else if (!strcasecmp(argv[1], "ProgramFile")) {
		strcpy(NextAnswer, Gdata.ProgramFile) ;
		
	}else if (!strcasecmp(argv[1], "Protocol")) {
		strcpy(NextAnswer, Gdata.Protocol) ;
		
//	}else if (!strcasecmp(argv[1], "SetDebug")) {	// Added from 2.13 (25/05/23)
//		Gdata.leveldebug = strtol(NextAnswer,NULL, 0) ;	// Can read decimal or "0xValue" (a uint32_t)
		
	}else if (!strcasecmp(argv[1], nextip)) {
		getinival(Gdata.sn_next,Gdata.deviceClass,nextip) ;	
		strcpy(NextAnswer, Gdata.sn_next) ;
	}else{
		getinival(Gdata.mominival,Gdata.deviceClass,argv[1]) ;	
		strcpy(NextAnswer, Gdata.mominival) ;
	}

	return(0);
}

// *********************************************************************

int  doGetWorkSpace(int argc, char *argv[]) {
	//strcpy(NextAnswer, Gdata.lpath) ;
	//printf("\n%s\n", argv[0]) ;
	return(0);
}

// *********************************************************************

int  doInputBox(int argc, char *argv[]){
  int nkey ;
  char lcap[MIN_STRING], lkey1[MIN_STRING], lkey2[MIN_STRING] ;
  char ltitle[DEF_STRING] ;

	if (argc<6) return(1);
	
	strcpy(lcap, argv[1] ) ;
	CLEAR_MEM(ltitle, DEF_STRING-1) ;
	strncpy(ltitle, ((argv[2][0]=='0')? "InputBox":argv[2]), DEF_STRING-1 ) ;
	nkey = atoi(argv[3]) ;
	CLEAR_MEM(lkey1, MIN_STRING-1) ;
	strncpy(lkey1, ((argv[4][0]=='0')? "OK":argv[4]), MIN_STRING-1 ) ;
	CLEAR_MEM(lkey2, MIN_STRING-1) ;
	strncpy(lkey2, ((argv[5][0]=='0')? "Annulla":argv[5]), MIN_STRING-1 ) ;
	
	//InputBox(char * caption, char *title, int nkey, char *lkey1, char *lkey2) 
	InputBox(0, lcap, ltitle, nkey, lkey1, lkey2 ) ;
	Gsequence.TimeoutAnswer = 0;
	Gdata.lastSlaveCommandId = WSCMD_U_INBOX ;
	return(CODE_WAIT_USER);
}

// *********************************************************************

int  doinputExtRefresh(int argc, char *argv[]) {
	ExtData.digitalsUpdated = 0 ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_TK ;
	Gdata.lastSlaveCommandId = WSCMD_T_GETDIGITAL ;
	return(CODE_WAIT_ANSWER);
}

// *********************************************************************

int  doinputExtA(int argc, char *argv[]){
	sprintf(NextAnswer, "%lu" , ExtData.portA ) ;
	return(0) ;
}

// *********************************************************************

int  doinputExtB(int argc, char *argv[])
{
	sprintf(NextAnswer, "%lu" , ExtData.portB ) ;
	return(0) ;
}

// *********************************************************************

int  doinputExtC(int argc, char *argv[])
{
	sprintf(NextAnswer, "%lu" , ExtData.portC ) ;
	return(0) ;
}

// *********************************************************************

int  doinputExtD(int argc, char *argv[])
{
	sprintf(NextAnswer, "%lu" , ExtData.portD ) ;
	return(0) ;
}

int  doinputExtE(int argc, char *argv[])
{
	sprintf(NextAnswer, "%lu" , ExtData.portE ) ;
	return(0) ;
}

// *********************************************************************

int  doMsgBox(int argc, char *argv[]){
  int nkey ;
  char lcap[MAX_STRING], lkey1[MIN_STRING], lkey2[MIN_STRING] ;
  char ltitle[DEF_STRING] ;

	printf("\nMsgBox:argv=<%s %s %s %s %s>\n",argv[1],argv[2],argv[3],argv[4],argv[5]);
  SLEEP(10);

	if (argc<6) return(1) ;
	
	strcpy(lcap, argv[1] ) ;
	CLEAR_MEM(ltitle, DEF_STRING-1) ;
	SLEEP(10);
	strncpy(ltitle, ((argv[2][0]=='0')? "MessageBox":argv[2]), DEF_STRING-1 ) ;
	nkey = atoi(argv[3]) ;
	CLEAR_MEM(lkey1, MIN_STRING-1) ;
	SLEEP(10);
	strncpy(lkey1, ((argv[4][0]=='0')? "OK":argv[4]), MIN_STRING-1 ) ;
	CLEAR_MEM(lkey2, MIN_STRING-1) ;
	SLEEP(10);
	strncpy(lkey2, ((argv[5][0]=='0')? "Annulla":argv[5]), MIN_STRING-1 ) ;
	
	//InputBox(char * caption, char *title, int nkey, char *lkey1, char *lkey2)
	InputBox(1, lcap, ltitle, nkey, lkey1, lkey2 ) ; // as msgbox
	Gsequence.TimeoutAnswer = 0;
	Gdata.lastSlaveCommandId = WSCMD_U_MSGBOX ;
	return(CODE_WAIT_USER);

}

// *********************************************************************

int  doMsgFile(int argc, char *argv[]){
FILE *msg_file ; 
struct timeval  tv;
struct timezone tz;
struct tm      *act;

	if (argc<3) return(1) ;

	gettimeofday(&tv, &tz);
	act = localtime(&tv.tv_sec);
	
	if ((msg_file = fopen(argv[2], "a"))==NULL) return(1) ;


	if (!strcmp(argv[1], "0") ){
		fprintf(msg_file, "%02d/%02d/%04d %02d:%02d:%02d(.%02ld) | %s\r\n", 
					act->tm_mday, act->tm_mon+1, act->tm_year+1900,
					act->tm_hour, act->tm_min, act->tm_sec, (tv.tv_usec/10000),
					argv[3]) ;
    	//Print #nFileNum, Now & Format(Timer - Int(Timer), "(.00) | ") & subarg(2)
	}else{
		fprintf(msg_file, "%s\r\n",argv[3]) ;
    	//Print #nFileNum, subarg(2)
	}

	fclose(msg_file) ;
	//strcpy(NextAnswer,"0") ;
	
return(0);
}

// *********************************************************************

int  doMSGWINDOW(int argc, char *argv[])
{
//int i, ii ;
//int ii ;
char hexstring[MAX_STRING];
char p[MAX_STRING];
//char *p ;

	if (argc<2) return(1);
	/*	
	p = argv[1] ;
	for(ii=0;ii<strlen(p);ii++){
		if (p[ii]<0) p[ii]='_' ;
		if (p[ii]==167) p[ii]='_';
	}
	Add_txt_answer(Scrn.boxtxt_answer, p) ;
	*/
	text_to_hex (0,argv[1],hexstring);
	hex_to_text (hexstring,p);
	Add_txt_answer(Scrn.boxtxt_answer, p) ;
	return(0);
}

// *********************************************************************

int  doM_ANALOG(int argc, char *argv[])
{

	if (argc<2) return(1);
	
	Gdata.lastSlaveCommandId = WSCMD_M_ANALOG ;
	Gdata.lastSlaveSubCommandId = atoi(argv[1]) - 1 ;
	MtsData.Data_ioad[Gdata.lastSlaveSubCommandId][0]='\0';
	dummy[0] = 'F' ;
	MTS_AddPacket(SrcDst(LU5GPS), PKT_COMMAND, IDCMD_STATUS, dummy, 1) ;
	MTS_SendTransaction() ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************



int  doM_SENDCANCONF(int argc, char *argv[]) {
	int type;
	int err;
	
	if (argc<3) return(1);
	type=atoi(argv[1]) ;

	if (type)
		err=sendnewcanconf(argv[2]) ;
	else
		err=sendoldcanconf(argv[2]) ;
	
	//invia al device la configurazione Can
	if (err) {
		char ss[DEF_STRING];
		//InputBox(char * caption, char *title, int nkey, char *lkey1, char *lkey2) 
		sprintf(ss, "file di CanCONF %s inesistente", argv[1]) ;
		InputBox(1, ss, "Attenzione!", 1, "OK",NULL) ; // as msgbox
		return(1);
	}else{
		return(0) ;
	}
}
// *********************************************************************
int  doM_CANADD(int argc, char *argv[])//_GT_//OK_CAN
{
int canAddr, canMask;
int txperiod, i;

	if (argc<5) return(1);

	txperiod = atoi(argv[2]) ;
    canAddr = atoi(argv[3]) ;
    canMask = atoi(argv[4]) ;

	//printf("\nCanAddr <%d>-<%x> - CanMask <%d>-<%x>\n",canAddr,canAddr,canMask,canMask) ;
	
	// configure CAN acquisistion
	//printf("\nLocalCanConf=<%d>\n",locCanConf.newcan);
    if (locCanConf.newcan) {
		i = (atoi(argv[1])) ;
        if (!i) {
            Gdata.CanChannel = LU13CAN ;
		}else{
            Gdata.CanChannel = LU14CAN ;
		}
		
    dummy[0] = 1  ;             // type
    dummy[1] = 0  ;            // not used
    dummy[2] = (txperiod & 0xff) ;  //send_time
		dummy[3] = (txperiod >>8) & 0xff ; 
    dummy[4] = (canAddr & 0xff) ; //address
    dummy[5] = (canAddr >>8) & 0xff;
		dummy[6] = (canAddr >>16) & 0xff;
    dummy[7] = (canAddr >>24) & 0xff; 
    dummy[8] = (canMask & 0xff) ; //mask
    dummy[9] = (canMask >>8) & 0xff;
		dummy[10] = (canMask >>16) & 0xff;
    dummy[11] = (canMask >>24) & 0xff;
		dummy[12] = 1 ; //tag
    dummy[13] = 0 ;
		dummy[14] = 0 ;
		dummy[15] = 0 ;
		dummy[16] = 0  ; //flags=0 (no transazionato)
		dummy[17] = 11 ; //ludest

		MTS_AddPacket(SrcDst(Gdata.CanChannel), PKT_BINARY, IDBIN_CCADD, dummy, 18) ;
		MTS_SendTransaction() ;
	}else{
        locCanConf.TimeSend = txperiod ;
        locCanConf.channel[locCanConf.tot_addr].Address = canAddr ;
        locCanConf.channel[locCanConf.tot_addr].Mask = canMask ;
        locCanConf.tot_addr++ ; 
		//printf("\nLocalCantot_addr=<%d>\n",locCanConf.tot_addr);
	}
	strcpy(NextAnswer,"0");
return(0);
}

// *********************************************************************

int  doM_CANCLEAR(int argc, char *argv[])//_GT_//OK_CAN
{
int CanBaud ;
int i;

	//printf("\nM_CANCLEAR=%s\n", argv[2] ) ;
	
	if (argc<3) return(1);
	//init CAN subsystem
	
	CanBaud = atoi(argv[2]) ;
	//printf("\nCanBaud=<%d>,<%x>\n",CanBaud,CanBaud) ;
	i=atoi(argv[1]) ;
	switch(i){
		case 0:
			Gdata.CanChannel = LU13CAN ;
			locCanConf.newcan = 1 ; //True 
		break ;
		case 1:
			Gdata.CanChannel = LU14CAN ;
            locCanConf.newcan = 1 ; //True
		break ;
		case 2: // Old can
			locCanConf.CanChannel = LU13CAN ;
            locCanConf.newcan = 0 ; //False
		break ;
		case 3: // Old can
			locCanConf.CanChannel = LU14CAN ;
            locCanConf.newcan = 0 ; //False
		break ;			
	}
	if (locCanConf.newcan) {
		dummy[0] = 0xAA ;
		dummy[1] = 0x55 ;
		dummy[2] = (CanBaud & 0xff) ;
        dummy[3] = (CanBaud >>8) & 0xff;
		dummy[4] = (CanBaud >>16) & 0xff;
        dummy[5] = (CanBaud >>24) & 0xff;
		MTS_AddPacket(SrcDst(Gdata.CanChannel), PKT_BINARY, IDBIN_CCCLEAR, dummy, 6) ;
		MTS_SendTransaction() ;
	}else{
		CanBaud=CanBaud -1 ;
		switch(CanBaud){
			case 1000000:
                locCanConf.busTiming1[0] = 0x00 ;
                locCanConf.busTiming2[0] = 0x14 ;
			break ;	
            case 800000:
                locCanConf.busTiming1[0] = 0x00 ;
                locCanConf.busTiming2[0] = 0x16 ;
			break ;	
            case 500000:
                locCanConf.busTiming1[0] = 0x00 ;
                locCanConf.busTiming2[0] = 0x1C ;
			break ;	
            case 250000:
                locCanConf.busTiming1[0] = 0x01 ;
                locCanConf.busTiming2[0] = 0x1C ;
			break ;	
            case 125000:
                locCanConf.busTiming1[0] = 0x03 ;
                locCanConf.busTiming2[0] = 0x1C ;
			break ;	
            case 100000:
                locCanConf.busTiming1[0] = 0x04 ;
                locCanConf.busTiming2[0] = 0x1C ;
			break ;	
            case 50000:
                locCanConf.busTiming1[0] = 0x09 ;
                locCanConf.busTiming2[0] = 0x1C ;
			break ;	
            case 20000:
                locCanConf.busTiming1[0] = 0x18 ;
                locCanConf.busTiming2[0] = 0x1C ;
			break ;	
            case 10000:
                locCanConf.busTiming1[0] = 0x31 ;
                locCanConf.busTiming2[0] = 0x1C ;
			break ;
		}
		locCanConf.tot_addr = 0 ;
	}
		strcpy(NextAnswer,"0");
return(0);
}

// *********************************************************************

int  doM_CANSTART(int argc, char *argv[])//_GT_//OK_CAN
{
//int lenpkt;
int i,llbuf2 ; // ,l;
char locbuf2[DEF_STRING];	
	
	if (argc<2) return(1) ;

	//start CAN acquisistion
    
    if (locCanConf.newcan) {
		i=atoi(argv[1]);
        if (!i) {
            Gdata.CanChannel = LU13CAN ;
		}else{
            Gdata.CanChannel = LU14CAN ;
		}
		MTS_AddPacket(SrcDst(Gdata.CanChannel), PKT_BINARY, IDBIN_CCSTART, dummy, 0) ;
		MTS_SendTransaction() ;
	}else{
        if (!locCanConf.tot_addr) {
			// old FS
			sprintf(locbuf2,"exe rm mts_lu%i.conf", locCanConf.CanChannel) ;
			llbuf2 = strlen(locbuf2)+1;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, locbuf2, llbuf2 ) ;
			sprintf(locbuf2,"exe rm Lu%i*.conf", locCanConf.CanChannel) ;
			llbuf2 = strlen(locbuf2)+1 ;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, locbuf2, llbuf2 ) ;
			
			// new FS
			sprintf(locbuf2,"exe rm /mnt/work/log/can/mts_lu%i.conf", locCanConf.CanChannel) ;
			llbuf2 = strlen(locbuf2)+1 ;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, locbuf2, llbuf2 ) ;
			sprintf(locbuf2,"exe rm /mnt/work/log/can/Lu%i*.conf", locCanConf.CanChannel) ;
			llbuf2 = strlen(locbuf2)+1 ;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, locbuf2, llbuf2 ) ;
			MTS_SendTransaction() ;
		}else{
            //lenpkt = (12 + (4 * locCanConf.tot_addr)) ;
            
            // send can conf (with old protocol)
            dummy[0] = 0x03 ;                      // Send CAN Conf
            dummy[1] = 0x01 ;                      // sja_IER Receive interrupt enable
            dummy[2] = locCanConf.busTiming1[0] ;     // Bus timing 0 & 1
            dummy[3] = locCanConf.busTiming2[0] ;     // Bus timing 0 & 1
            dummy[4] = 0xDA ;                      // OCR
            // from LU13 to LU11
            dummy[5] = locCanConf.CanChannel * 16 + mtskick_lu ;   // 1. BSC parameter(pk_sd)
            dummy[6] = 0x1B ;                      // FREE
            dummy[7] = 6 ;        				 //	extended
            dummy[8] = locCanConf.tot_addr ;       // Only 1 address for check
			/*	
			for(i=0;i<locCanConf.tot_addr-1;i++) {
				printf("\nlocCanConf.channel[%d].Address=<%d>\n",i,locCanConf.channel[i].Address);
                // filter
                dummy[9 + (4 * i)] = (locCanConf.channel[i].Mask & 0xff) ;
                // Address
                dummy[12 + (4 * i)] = ((locCanConf.channel[i].Address & 0xFF0000) >> 16) & 0xff ;
                dummy[11 + (4 * i)] = (locCanConf.channel[i].Address >> 8) & 0xff ;
                dummy[10 + (4 * i)] = locCanConf.channel[i].Address & 0xff ;
			}
            dummy[lenpkt - 2] = 0xF1 ;
            dummy[lenpkt - 1] = 0x00 ;
            dummy[lenpkt] = 0x00 ;
			*/
			dummy[9] = (locCanConf.channel[0].Mask & 0xff) ;
                // Address
            dummy[12] = ((locCanConf.channel[0].Address & 0xFF0000) >> 16) & 0xff ;
            dummy[11] = (locCanConf.channel[0].Address >> 8) & 0xff ;
            dummy[10] = locCanConf.channel[0].Address & 0xff ;

			dummy[13] = 0xF1 ;
            dummy[14] = 0x00 ;
			dummy[15] = 0x00 ;
			
            	// send IDBIN_CCANBUS or IDBIN_BCANBUS
			MTS_AddPacket(SrcDst(locCanConf.CanChannel), PKT_BINARY, 14 + locCanConf.CanChannel, dummy, 16) ;
			MTS_SendTransaction() ; 
        		// SEND TIMING CONF
            	//prepare packet
            dummy[0] = 0x02 ;                      // Send CAN Conf
            dummy[1] = 0x01 ;                      // 1 data for each transmission
            dummy[3] = (locCanConf.TimeSend >> 8) & 0xff ;    // Each xx dsec
            dummy[2] = (locCanConf.TimeSend & 0xff) ;         // Each xx dsec
        
            dummy[4] = 0  ;                      //FREE
        
            // to LU13 or LU14
            // send IDBIN_CCANBUS or IDBIN_BCANBUS
			MTS_AddPacket(SrcDst(locCanConf.CanChannel), PKT_BINARY, 14 + locCanConf.CanChannel, dummy, 5) ;
			MTS_SendTransaction() ;
			
		}
	}
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS*3 ;
	Gdata.lastSlaveCommandId = WSCMD_M_CANSTART ;
	return(CODE_WAIT_ANSWER) ;
	/*	
		strcpy(NextAnswer,"0") ;
		return(0);
	*/
}

// *********************************************************************

int  doM_CanCheck(int argc, char *argv[]) //_GT_//OK_CAN
{
int Address,Address_Chech;
char Data[MAX_STRING];
int i,len ;

	for(len=0;len<MAX_STRING;len++) Data[len]='\0';
	
	//len=strlen(argv[3]);
	
	//printf("\nCAncheck strlen(argv[3])=%d(%s)\n",len,argv[3]);

	if (argc<4) return(1);
	Address_Chech = atoi(argv[2]);
// check CAN acquisistion
	i=atoi(argv[1]);
	//printf("\nMCanCheck i=%d\n",i);
	len=strlen(argv[3]);
    if (!(i&0x1)) {
		 //CanChannel = LU13CANBUS
	printf("\nMtsdata.lastCan13Rx.data: %s\n",MtsData.lastCan13Rx.data) ;
	//printf("\nLunghezza parametro 3= %d , %s",len,argv[3]);
		strncpy(Data,MtsData.lastCan13Rx.data,len);
		Address = MtsData.lastCan13Rx.Address & 0xFFFFFFF;
	}else{
		//CanChannel = LU14CANBUS
	printf("\nMtsdata.lastCan14Rx.data: %s\n",MtsData.lastCan14Rx.data) ;
		strncpy(Data,MtsData.lastCan14Rx.data,len);
		Address = MtsData.lastCan14Rx.Address & 0xFFFFFFF;
	}
	//Data[len]='\0';
	//printf("\nMtsData.Address: %ld TK.Address: %ld\n",Address,Address_Chech) ;
	//printf("\nMtsData.Data: <%s><%d> TK.Data: <%s><%d>\n",Data,strlen(Data),argv[3],strlen(argv[3])) ;
	
	if ( (Address == Address_Chech) && (!strcasecmp(Data,argv[3])) ) {
		strcpy(NextAnswer,"0"); //ok
	}else{
		strcpy(NextAnswer,"1"); //ko
	}
	printf("\nAddress=<%d><%x> Address_Chech=<%d><%x> Data=<%s>argv[3]=<%s>\n",Address,Address,Address_Chech,Address_Chech,Data,argv[3]) ;
return(0) ;
}

// *********************************************************************

int  doM_CanClearBuffer(int argc, char *argv[])//_GT_//OK_CAN
{
int i,l;
	if (argc<2) return(1);
	
	//cclear CAN rx buffer
    i=atoi(argv[1]);
   	if (!i) { 
		//CanChannel = LU13CAN
		MtsData.lastCan13Rx.Address = 0 ;
		for(l=0;l<17;l++) MtsData.lastCan13Rx.data[l] = '\0' ;
		MtsData.lastCan13Rx.tag = 0 ;
	}else{
		//CanChannel = LU14CAN
		MtsData.lastCan14Rx.Address = 0 ;
		for(l=0;l<17;l++) MtsData.lastCan14Rx.data[l] = '\0' ;
		MtsData.lastCan14Rx.tag = 0 ;
	}
	strcpy(NextAnswer,"0") ;
	return(0);
}

// *********************************************************************

int  doM_Cnt(int argc, char *argv[])
{

	if (argc<2) return(1);
	
	Gdata.lastSlaveSubCommandId = atoi(argv[1]) ;
	if (Gsequence.BufferedMode){
		switch(Gdata.lastSlaveSubCommandId){
			case 0:
				strcpy(NextAnswer, MtsData.Data_iokm) ;
				break ;
			case 1:
				strcpy(NextAnswer, MtsData.Data_cnt1) ;
				break ;
			case 2:
				strcpy(NextAnswer, MtsData.Data_cnt2) ;
				break ;
		}
		return(0) ;
	}else{
		MtsData.Data_iokm[0]='\0' ;
		MtsData.Data_cnt1[0]='\0' ;
		MtsData.Data_cnt2[0]='\0' ;
		Gdata.lastSlaveCommandId = WSCMD_M_CNT ;
		dummy[0] = 'F' ;
		MTS_AddPacket(SrcDst(LU5GPS), PKT_COMMAND, IDCMD_STATUS, dummy, 1) ;
		MTS_SendTransaction() ;
		Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
		return(CODE_WAIT_ANSWER) ;
	}
}

// *********************************************************************

int  doM_DelPar(int argc, char *argv[])
{
short ih ;

	if (argc<2) return(1);
	
	ih = atoi(argv[1]) ;
	dummy[0] = ih & 0xff ;
	dummy[1] = (ih>>8) & 0xff ;
	
	MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_RESET, dummy, 2) ;
	MTS_SendTransaction() ;

	Gdata.lastSlaveCommandId = WSCMD_U_TIMER ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_CMD ;
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************

int  doM_DIAG(int argc, char *argv[])
{
int i ;
int retval, len ;

	if (argc<3) return(1) ;
	for(i=0;i<MAX_STRING;i++) MtsData.diagdata[i]='\0' ;
	retval = CODE_WAIT_ANSWER ;

	Gdata.rDiag = atoi(argv[1]) ;
	dummy[0] = Gdata.rDiag & 0xff ;
	len = 1 ;
	switch(Gdata.rDiag){
		case 0 :
			MtsData.diagdata[0] = 100 ;
			Gdata.lastSlaveSubCommandId = atoi(argv[2])+1 ;
			if (Gdata.lastSlaveSubCommandId>2) Gdata.lastSlaveSubCommandId++ ;
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG0 ;
			break ;
		case 1:
			len = 2 ;
			dummy[1] = atoi(argv[2]) & 0xff ;
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG1 ;
			break ;
		case 2:
			len = 2 ;
			dummy[1] = atoi(argv[2]) & 0xff ;
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG2 ;
			break ;
		case 3 :
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG3 ;
			break ;
		case 4 :
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG4 ;
			break ;
		case 7 :
			len = 2 ;
			dummy[1] = atoi(argv[2]) & 0xff ;
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG7 ;
			break ;
		case 9 :
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG9 ;
			break ;
		case 12 :
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG12 ;
			break ;
		case 25 :
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG25 ;
			break ;
		case 127:
			len = 3 ;
			dummy[1] = atoi(argv[2]) & 0xff ;
			dummy[2] = (atoi(argv[2])>>8 & 0xff) ;
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG127 ;
			break ;
		case 128: // Display a number on LCD or execute test if 0
			if (atol(argv[2])>0){
				len = strlen(argv[2]) ;
				memcpy(&dummy[1], argv[2], len ) ;
				len++ ;
			}
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DIAG, dummy, len) ;
			MTS_SendTransaction() ;
			return(0) ;
			break ;
		case 200 : // Test USB
			sprintf(NextAnswer, "%d", MtsData.usb ) ;
			return(0) ;
			break ;
		case 201: // RTC
			sprintf(NextAnswer, "%d", MtsData.rtc ) ;
			return(0) ;
			break ;
		case 250 : // Sblocco S/N
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG250 ;
			break ;
		case 9600:		// Lock GSM baud
		case 19200:
		case 38400:
		case 57600:
		case 115200:
			dummy[0] = 0  ;
			dummy[1] = Gdata.rDiag & 0xff ;
			dummy[2] = (Gdata.rDiag>>8) & 0xff ;
			dummy[3] = (Gdata.rDiag>>16) & 0xff ;
			dummy[4] = (Gdata.rDiag>>24) & 0xff ;
			MTS_AddPacket(0x99, PKT_COMMAND, IDCMD_SET, dummy, 5) ;
			dummy[0] = 2  ;
			dummy[1] = 5  ;
			len = 2 ;
			Gdata.lastSlaveCommandId = WSCMD_M_DIAG2 ;
			break ;
		default :
			Gdata.lastSlaveCommandId = WSCMD_M_DIAGUNKNOWN ;
			break ;
	}
	
	if (retval == CODE_WAIT_ANSWER){
		MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DIAG, dummy, len) ;
		MTS_SendTransaction() ;
 		Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
		return(retval) ;
	}
	
	return(retval);
}

// *********************************************************************

int  doM_EXESM(int argc, char *argv[])//_GT_//OK
{
int actcode,actval,i ;

	i = 0 ;

	if (argc<3) return(1) ;
		actcode = atoi(argv[1]) ;
		actval = atoi(argv[2]) ;

	if (argc>3) i = strlen(argv[3])+1 ;

	if (actcode) {
		//store actcode
		dummy[0] = (actcode & 0xff) ; 
		dummy[1] = (actcode >>8) & 0xff ;
		// store actval
		dummy[2] = (actval & 0xff) ;
		dummy[3] = (actval >> 8) & 0xff ;		
		// store actpar (if exist)
		if (i>0) memcpy(&dummy[4], argv[3],i) ;			
		MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_EXESM, dummy, i+4) ;	              
	}else{
		memcpy(&dummy[0],argv[3],i) ;
		dummy[i]='\0' ;
		MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_DEBUG, dummy, i+1) ;
	}
	if ( actcode==255 ) {
		//printf("\nGdata.RISP255(1)=%d\n", Gdata.RISP255) ; 
		Gdata.RISP255=1;
		//printf("\nGdata.RISP255(2)=%d\n", Gdata.RISP255) ; 
		MtsData.Dir_data[0]='\0' ;
	}
	MTS_SendTransaction() ;
	if ( actcode==255 ) {
		Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
		Gdata.lastSlaveCommandId = WSCMD_M_EXESM255 ;
		return(CODE_WAIT_ANSWER) ;
	}else{
		strcpy(NextAnswer,"0") ;
		return(0);
	}
}

// *********************************************************************

int  doM_GetDirect(int argc, char *argv[])//_GT_//OK
{	
	printf("\nM_GETDIRECT\n");
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	Gdata.lastSlaveCommandId = WSCMD_M_GETDIRECT ;
	return(CODE_WAIT_ANSWER) ;
	
}

// *********************************************************************

int  doM_GETFAMILY(int argc, char *argv[])
{
	/*
	if (Gsequence.BufferedMode){
		strcpy(NextAnswer, MtsData.DRep_mtstype) ;
		return(0) ;
	}
	*/

	MtsData.DRep_mtstype[0]='\0' ;
	MTS_AddPacket(SrcDst(LU1APP), PKT_COMMAND, IDCMD_STATUS, dummy, 0) ;
	MTS_SendTransaction() ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	Gdata.lastSlaveCommandId = WSCMD_M_FAMILY ;
	return(CODE_WAIT_ANSWER) ;

}

// *********************************************************************

int  doM_GETGPSFLAGS(int argc, char *argv[])//_Gt_//OK
{
	if (Gsequence.BufferedMode){
		strcpy(NextAnswer, MtsData.Data_fixtype) ;
		return(0) ;
	}

	MtsData.Data_fixtype[0]='\0' ;
	dummy[0]='F' ;
	MTS_AddPacket(SrcDst(LU5GPS), PKT_COMMAND, IDCMD_STATUS, dummy, 1) ;
	MTS_SendTransaction() ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	Gdata.lastSlaveCommandId = WSCMD_M_FIX ;
	return(CODE_WAIT_ANSWER) ;

}

// *********************************************************************
int  doM_GetIMEI(int argc, char *argv[])//_GT_//OK
{ 

	com_baud(MTS_current_PORT, baud_select(9600) ) ; // flush
	SLEEP(500) ;
        com_write(MTS_current_PORT, 5,"ATE0\r") ; 

	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	Gdata.lastSlaveCommandId = WSCMD_AT_OK ;
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************

int  doM_GetPar(int argc, char *argv[])//_GT_//OK
{
int ih;
	
	if (argc<2) return(1);
	
	ih = atoi(argv[1]) ;
	dummy[0] = ih & 0xff ;
	dummy[1] = (ih>>8) & 0xff ;
	
	MtsData.parval[0] = '\0' ;
	
	MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_SET, dummy, 2) ;
	MTS_SendTransaction() ;
	
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS*3 ;
	Gdata.lastSlaveCommandId = WSCMD_M_GETPAR ;
	
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************

int  doM_GETSERNUM(int argc, char *argv[])//_GT_//OK
{		
	MtsData.Data_cpu[0] = '\0' ;
	MTS_AddPacket(SrcDst(LU1APP), PKT_COMMAND, IDCMD_STATUS, dummy, 0) ;
	MTS_SendTransaction() ;
	Gdata.lastSlaveCommandId = WSCMD_M_GETSERNUM ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	return(CODE_WAIT_ANSWER) ;
}
// *********************************************************************

int  doM_GetSmFile(int argc, char *argv[])//_GT_//OK
{
#ifdef USE_MONITORING
	printf("doM_GetSmFile %d <%s>\n", argc, argv[1] ) ;
#endif
	if (argc<2) return(1);
	/* ottiene dal device la macchina a stati in binario e salva nel file speificato.
       Se il file esiste gia' viene sovrascritto */
	// Open file
	Gdata.up_sm = fopen(argv[1], "w") ; // file sm
	if (Gdata.up_sm==NULL) {
		printf("Non riesco a scrivere file macchina stati" ) ;
		return(1) ; // really exists ?  
	}
	//manda richiesta macchina a stati (completa). Arrivera' risposta con IDGPS_SMADD
	MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMDUMP, dummy, 0) ;
	MTS_SendTransaction() ;
	Gdata.lastSlaveCommandId = WSCMD_M_GETSMFILE ;
	//Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************

int  doM_SETSOURCEID(int argc, char *argv[])
{
unsigned long  il ;
int ret_val ;

	if (argc<2) return(1);
	il = atol(argv[1]) ;
	if ((il !=11) && (il !=2)) com_close(PORT_UDP);
	
	if ((il==11) || (il==2)){
		Gdata.LU_src = il ;
	}else{
		in_addr_t address ;
		address=il ;
		struct in_addr address_struct ;
   		address_struct.s_addr = address ;
		strcpy(Gdata.mts_ip,inet_ntoa(address_struct));
		printf("\nGdata.mts_ip=<%s>\n",Gdata.mts_ip);
		strcpy(Gdata.pc_ip,Gdata.localIP);
		printf("\nGdata.pc_ip=<%s>\n",Gdata.pc_ip);
		Gdata.mts_sn = atol(MtsData.Data_cpu) ;
		Gdata.pc_socket = 5100 ;
		Gdata.mts_socket = 5100 ;
		Gdata.LU_src = 8 ;
		ret_val = com_open(PORT_UDP, 0, 0 ) ;
		if (ret_val) {		// ERROR
#ifdef USE_MONITORING
		printf("\nError opening UDP port (%d)\n", ret_val) ;
#endif // USE_MONITORING
		return(1);
		}
	}
	
	return(0) ;
}

// *********************************************************************

int  doM_GETSWVERS(int argc, char *argv[]) //_GT_//OK
{	
	if (argc<1) return(1);
	
	MtsData.Data_swver[0]= '\0' ;
	
	MTS_AddPacket(SrcDst(LU1APP), PKT_COMMAND, IDCMD_STATUS, dummy, 0) ;
	MTS_SendTransaction() ;
	Gdata.lastSlaveCommandId = WSCMD_M_GETSWVERS ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************

int  doM_GETTIME(int argc, char *argv[])//_GT_//OK
{
	if (argc<1) return(1);

	MtsData.Data_iotime[0]= '\0' ;
	dummy[0] = 'F' ;
	MTS_AddPacket(SrcDst(LU5GPS), PKT_COMMAND, IDCMD_STATUS, dummy, 1) ;
	MTS_SendTransaction() ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	Gdata.lastSlaveCommandId = WSCMD_M_GETTIME ;
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************

int  doM_INPUT(int argc)//_GT_//OK
{
	if (argc<1) return(1);
	
	//lettura IO
		
	dummy[0] = 'F' ;

	MtsData.Data_iotime[0]= '\0';
	
	MTS_AddPacket(SrcDst(LU5GPS), PKT_COMMAND, IDCMD_STATUS, dummy, 1) ;
	MTS_SendTransaction() ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	Gdata.lastSlaveCommandId = WSCMD_M_INPUT ;
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************

int  doM_INSTATUS(int argc, char *argv[])//_GT_//OK
{
int i ;

	if (argc<1) return(1) ;

	//GET STATUS (gsm reg, gps, gprs,...)
	if (Gsequence.BufferedMode){
		strcpy(NextAnswer, MtsData.Data_ioflags) ;
		return(0) ;
	}else{
		for(i=0;i<16;i++) MtsData.Data_iostat[i]='\0' ;
		MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMREAD, dummy, 0) ;
		MTS_SendTransaction() ;
		Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
		Gdata.lastSlaveCommandId = WSCMD_M_GETDUPSMSTATUS ;
		return(CODE_WAIT_ANSWER) ;
	}
}

// *********************************************************************

int  doM_INVIRT(int argc, char *argv[])//_GT_//OK
{
	if (argc<1) return(1) ;

	//FLAG SM (gsm reg, gps, gprs,...)
	/*
	if (Gsequence.BufferedMode){
		strcpy(NextAnswer, MtsData.Data_ioflags) ;
		return(0) ;
	}else{
	*/
		MtsData.Data_ioflags[0]='\0' ;
		MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMREAD, dummy, 0) ;
		MTS_SendTransaction() ;
		Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
		Gdata.lastSlaveCommandId = WSCMD_M_GETDUPSMFLAGS ;
		return(CODE_WAIT_ANSWER) ;
	/*}*/
}

// *********************************************************************

int  doM_OUTPUT(int argc, char *argv[])//_GT_//OK
{
long lmask ;
int lval;
	if (argc<3) return(1);
	
    lmask = atol(argv[1]) ;
    lval = atoi(argv[2]);
	
	dummy[0] = (lval & 0xff) ; 
	dummy[1] = (lval >>8) & 0xff ;
	dummy[2] = (lmask & 0xff) ; 
	dummy[3] = (lmask >>8) & 0xff ;
	MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_MSKIO, dummy, 4) ;
	MTS_SendTransaction() ;
	//Delay of 5 sec to allow implementation of control
	//Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
	Gsequence.TimeoutAnswer = 0.5 + TIMEOUT_MTS ;
	Gdata.lastSlaveCommandId = WSCMD_U_TIMER ;
	return(CODE_WAIT_ANSWER);
}

// *********************************************************************
// Start a shell and a thread waiting for end process
// 1 = Path, 2 = program, 3.. = args
int  doM_PROGRAM(int argc, char *argv[]) {
int i;//,c;
//char free_buf[MAX_STRING],free_buf2[MAX_STRING];

	if (argc<3) return(1) ;

	//rem_duble_slash(argv[1], argv[1]);
	
	i=(argc-1);
	//printf("\nPrima di upass i=<%d>\n",i);
 	if (Gdata.upass[0]!='\0') {
		i=0;
		//printf("\n In upass i=<%d>\n",i);
		//while (argv[i]!='\0') { ??? // =FR
		while (argv[i]!=0) {
			//printf("\n Argmontento[%d]=%s \n",i,*&argv[i]);
			i++;
		}
		i=i-1;
		//printf("\n Dopo while i=<%d>\n",i);
		strcat(argv[i],Gdata.upass);
		//printf("\n Argmontento[%d]=%s \n",i,*&argv[i]);
		argv[i+1]='\0';
		//printf("\n Argmontento[%d]=%s \n",i+1,*&argv[i+1]);
	}
	/*c=1;
	while (argv[c]!='\0') {
		printf("\n Argmontento[%d]=%s \n",c,*&argv[c]) ;
		
		if (c==1) {
			sprintf(free_buf2,"%s",*&argv[c]);
			printf("\nFree_buf2=<%s>",free_buf2);
			strcpy(free_buf,free_buf2);
		}else{
			sprintf(free_buf2," %s",*&argv[c]);
			printf("\nFree_buf2=<%s>",free_buf2);
			strcat(free_buf,free_buf2);
		}
		c++;
	}
	//strcat(free_buf,'\0');
	printf("\n free_buf=<%s> \n",free_buf) ;
	c=strlen(free_buf);
	printf("\nLunghezza di c=<%d>",c);
	//c++;
	//free_buf[c]='\0';
	c=0;
	//while (argv[c]!='\0') argv[c]='\0';
	while (argv[c]!='\0') {
			printf("\n Argmontento[%d]=%s \n",c,*&argv[c]);
			strcpy(argv[c],"");
			printf("\n Argmontento[%d]=%s \n",c,*&argv[c]);
			c++;
	}
	strcpy(argv[0],free_buf);
	printf("\n Argmontento[0]=%s \n",*&argv[0]);*/
	//SLEEP(1000000);
	
	 
	//SLEEP(1000000) ;
	
	
	i=Start_command( (i),&argv[1]) ;
	
	
// 
// 	Gsequence.TimeoutAnswer = TIMEOUT_TK ;
// 	Gdata.lastSlaveCommandId = KSLAVECMD_EXESHELL ;

//da sostituire con fork
	com_baud(PORT_TK, baud_select(300) ) ;	// flush
	
	return(i); // No send answer 
}

// *********************************************************************

int  doM_PutSmFile(int argc, char *argv[])//_GT_//OK
{

#ifdef USE_MONITORING
	printf("doM_PutSmFile %d <%s>\n", argc, argv[1] ) ;
#endif
	
	if (argc<2) return(1) ;

	//invia al device la macchina a stati in binario e prendendola dal file specificato
	if (start_sm(argv[1],0)) {
		char ss[DEF_STRING];
		//InputBox(char * caption, char *title, int nkey, char *lkey1, char *lkey2) 
		sprintf(ss, "file di SM %s inesistente", argv[1]) ;
		InputBox(1, ss, "Attenzione!", 1, "OK",NULL) ; // as msgbox
		return(0);
	}else{
		Gdata.lastSlaveCommandId = WSCMD_M_PUTSMFILE ;
		return(CODE_WAIT_ANSWER) ;
	}
}

// *********************************************************************

int  doM_PutFWFile(int argc, char *argv[])//_GT_
{

#ifdef USE_MONITORING
	printf("doM_PutFWFile %d <%s>\n", argc, argv[1] ) ;
#endif
	
	if (argc<2) return(1) ;

	//invia al device il firmware prendendolo dal file specificato
	if (start_fw(argv[1])) {
		char ss[DEF_STRING];
		sprintf(ss, "file del FW %s inesistente", argv[1]) ;
		InputBox(1, ss, "Attenzione!", 1, "OK",NULL) ; // as msgbox
		return(0);
	}else{
		Gdata.delayed=0; //Se uno non ribustrabba MTS dopo invio codice
		Gdata.bcrch=0;
		Gdata.bcrcl=0;
		//Gdata.systype=3;
		Gsequence.TimeoutAnswer = TIMEOUT_MTS*150 ;
		Gdata.lastSlaveCommandId = WSCMD_M_PUTFWFILE ;
		return(CODE_WAIT_ANSWER) ;
	}
}


// *********************************************************************

int  doM_ReadIntAD(int argc, char *argv[])//_GT_//OK
{
	if (argc<1) return(1) ;
	
	//tornerà insieme valori con Vext, Vbat, CSQ, Temperatura interna, Acc assi x,y,z
	if (Gsequence.BufferedMode){
		int loc_i;
		int mom_i;
		for(loc_i=0;loc_i<7;loc_i++){
			//printf("\nSTO PER RISPONDERE CON Data_intad[%d]=%s\n",loc_i,MtsData.Data_intad[loc_i]);
			strcat(NextAnswer, MtsData.Data_intad[loc_i]) ;
			strcat(NextAnswer,"&");
		}
		//salto fino a id 14 fino a 16
		for(loc_i=0;loc_i<3;loc_i++){
			for(mom_i=0;mom_i<=MAX_INTAD;mom_i++){
				if ( MtsData.Id_intad[mom_i]==(14+loc_i) ) {
					//printf("\nSTO PER RISPONDERE CON Data_intad[%d]=%s\n",(14+loc_i),MtsData.Data_intad[mom_i]);
					strcat(NextAnswer, MtsData.Data_intad[mom_i]) ;
					strcat(NextAnswer,"&");
				}
			}
		}
		printf("\nBuffer Stringa=%s\n",NextAnswer);
		return(0) ;
	}else{
		MtsData.Data_intad[0][0] = '\0' ;
		MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_INTAD, dummy, 0) ;
		MTS_SendTransaction() ;
		Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
		Gdata.lastSlaveCommandId = WSCMD_M_GETINTERNAL_AD ;
		return(CODE_WAIT_ANSWER) ;
	}
}

// *********************************************************************

int  doM_GETHWVERS(int argc, char *argv[])//_GT_//OK
{
	if (argc<1) return(1) ;
	
	//tornerà insieme valori cod HW MB, cod HW SW
		MtsData.Data_auxad[0][0] = '\0' ;
		MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_AUXAD, dummy, 0) ;
		MTS_SendTransaction() ;
		Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;
		Gdata.lastSlaveCommandId = WSCMD_M_GETHWVERS ;
		return(CODE_WAIT_ANSWER) ;
}

int  doM_DirectTOLU(int argc, char *argv[])//_GT_//OK
{
	int LU, len ;
	
	
	if (argc<1) return(1) ;
	
	LU=atoi(argv[1]);
	len = strlen(argv[2]) ;
	strcpy(dummy,argv[2]);
	if ( dummy[len] != 13 ) {
		len=len+1;
		dummy[len]=13;	
	}
	
	MTS_AddPacket(SrcDst(LU), PKT_COMMAND, IDCMD_DIRECT, dummy, len) ;
	MTS_SendTransaction() ;
	
	return(0) ;
}

// *********************************************************************

int  doM_SetPar(int argc, char *argv[])//_GT_//OK
{
int nr_par,ival,lpar,len;
long lval;
	MtsData.setok='\0';
	if (argc<2) return(1);
	
	//modifica i flag virtuali di SM
    nr_par = atoi(argv[1]) ;

	dummy[0] = (nr_par & 0xff) ; 
	dummy[1] = (nr_par >>8) & 0xff ;

	if ( (!strncmp(argv[2],"N.D.",4)) || strlen(argv[2])==0) {
		if (nr_par==255){
			dummy[1]=0xaa;
			dummy[2]=0x55;
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_RESET, dummy, 3) ;
		}else{
			MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_RESET, dummy, 2) ;
		}		
		MTS_SendTransaction() ;
	}else{
		if ((nr_par>0) & (nr_par<=127)){
			ival = atoi(argv[2]) ;
			lpar = 2 ;
			dummy[2] = (ival & 0xff) ;
      dummy[3] = (ival >>8) & 0xff;
		}else if ((nr_par>=224) & (nr_par<=255)){
			lval = atol(argv[2]) ;
			lpar = 4 ;
			dummy[2] = (lval & 0xff) ;
      dummy[3] = (lval >>8) & 0xff;
			dummy[4] = (lval >>16) & 0xff;
      dummy[5] = (lval >>24) & 0xff;	
		}else if ((nr_par>=128) & (nr_par<=223)){
				len = strlen(argv[2]) ;
				memcpy(&dummy[2], argv[2], len ) ;
				lpar = len;
				printf("\nSend String Par %d <%s>\n",nr_par,argv[2]);
		}else{
			return(1);
		}
		MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_SET, dummy, 2 + lpar) ;
		MTS_SendTransaction() ;
	}
	//Delay of 5 sec to allow implementation of control
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS*3 ;

	Gdata.lastSlaveCommandId = WSCMD_M_SETPAR ;
	return(CODE_WAIT_ANSWER);
}

// *********************************************************************

int  doM_SETSTATUS(int argc, char *argv[])//_GT_//OK
{
int32_t lval,auxl;
	if (argc<3) return(1);
	
	//modifica i flag virtuali di SM
    lval = atoi(argv[1]) ;
    auxl = atoi(argv[2]);
	
	dummy[0] = (lval & 0xff) ; 
	dummy[1] = (lval >>8) & 0xff ;
	dummy[2] = (lval >>16) & 0xff ;
	dummy[3] = (lval >>24) & 0xff ;
	//Maschera flag
	dummy[4] = (auxl & 0xff) ; 
	dummy[5] = (auxl >>8) & 0xff ;
	dummy[6] = (auxl >>16) & 0xff ;
	dummy[7] = (auxl >>24) & 0xff ;
	MTS_AddPacket(SrcDst(LU5GPS), PKT_GPSIO, IDGPS_SMWRITE, dummy, 8) ;
	MTS_SendTransaction() ;
	//Delay of 5 sec to allow implementation of control
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;

	Gdata.lastSlaveCommandId = WSCMD_U_TIMER ;
	return(CODE_WAIT_ANSWER);
}

// *********************************************************************

int  doM_SetTime(int argc, char *argv[])//_GT_//OK
{
long oldtimeL,newtimeL;
	if (argc<3) return(1);
	
	//imposta orologio (attenzione: necessario passare anche la vecchia ora!)
    oldtimeL = atol(argv[1]) ;
    newtimeL = atol(argv[2]);
	
	dummy[0] = (oldtimeL & 0xff) ; 
	dummy[1] = (oldtimeL >>8) & 0xff ;
	dummy[2] = (oldtimeL >>16) & 0xff ;
	dummy[3] = (oldtimeL >>24) & 0xff ;
	dummy[4] = (newtimeL & 0xff) ; 
	dummy[5] = (newtimeL >>8) & 0xff ;
	dummy[6] = (newtimeL >>16) & 0xff ;
	dummy[7] = (newtimeL >>24) & 0xff ;
	MTS_AddPacket(SrcDst(LU0CPU), PKT_COMMAND, IDCMD_SYNA, dummy, 8) ;
	MTS_SendTransaction() ;
	//Delay of 5 sec to allow implementation of control
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_MTS ;

	Gdata.lastSlaveCommandId = WSCMD_U_TIMER ;
	return(CODE_WAIT_ANSWER);
}

// *********************************************************************
// 0="OuputText", 1=text identifier, 2=TEXT, 3=BackColor, 4=TextLabel, 5=Flashed, 6=ColorFlash

int  doOuputText(int argc, char *argv[])
{
int textId, i ;
int lForeColor, lBackColor ;
int flash, fColor ;

	if (argc<7) return(1) ;

	textId = atoi(argv[1]) ;
	
	if (!textId) return(2) ;
	
	lBackColor = atoi(argv[3]) ; // textcolor 0-
	switch(lBackColor){
		case VB_BLACK:
		case VB_BLUE:
		case VB_GRAY:
			lForeColor = VB_WHITE ;
			break ;
		default:
			lForeColor = VB_BLACK ;
	}
	
	flash = atoi(argv[5]) ; // Flash color
	fColor = atoi(argv[6]) ; // Flash color
	
	if (textId==1){ // lbl_run
		// 'lbl_run'
		gtk_widget_modify_bg( Scrn.bklbl_run, GTK_STATE_NORMAL, &vb_color[lBackColor]) ;
		gtk_widget_modify_fg( Scrn.lbl_run, GTK_STATE_NORMAL, &vb_color[lForeColor]) ;
		gtk_label_set_text(GTK_LABEL(Scrn.lbl_run), argv[2]) ;
		if (flash)
			Scrn.lrun_flashcol = fColor ;
		else
			Scrn.lrun_flashcol = -1 ;
	}else if (textId<10){ // lbl_free[8]
		i = textId-2 ;
		gtk_widget_modify_bg( Scrn.bktxt_free[i], GTK_STATE_NORMAL, &vb_color[lBackColor]) ;
		gtk_widget_modify_fg( Scrn.txt_free[i], GTK_STATE_NORMAL, &vb_color[lForeColor]) ;
		gtk_label_set_text(GTK_LABEL(Scrn.txt_free[i]), argv[2]) ;
		gtk_label_set_text(GTK_LABEL(Scrn.lbl_free[i]), argv[4]) ;
		if (flash)
			Scrn.txt_flashcol[i] = fColor ;
		else
			Scrn.txt_flashcol[i] = -1 ;
		// _FR_ TIMER for Flash
	}else if ((textId>=100) && (textId<=122)){
		i = textId-100 ;
		// lForeColor = color of 'clr_step'
		// argv[2] = text to display into 'txt_step'
		gtk_widget_modify_bg( Scrn.clr_step[i], GTK_STATE_NORMAL, &vb_color[lBackColor]) ;
		
		if ( strlen(argv[2]) )	gtk_label_set_text(GTK_LABEL(Scrn.txt_step[i]), argv[2]) ;
	}


	return(0);
}

// *********************************************************************

int  doProgressBar(int argc, char *argv[])
{
int retval, type, lpar ;

	retval = 1 ;

 	if (argc>2){
		type = atoi(argv[1]) ;
		lpar = atoi(argv[2]) ;
		switch(type){
			case 0:		// value
			if ( (lpar>=0) && (lpar<=100)) {
				ProgressBar(0, lpar ) ;
				retval = 0 ;
			}
			break ;
			
			case 1:		// time step msec
			if (lpar>0){
				ProgressBar(1, lpar ) ;
				retval = 0 ;
			}
			break ;
		}
 	}
	
return(retval);
}

// *********************************************************************

int  doSetINIKeyVal(int argc, char *argv[])//_GT_//OK
{
	
	if (argc<3) return(1) ;
	setinival(Gdata.deviceClass,argv[1],argv[2]) ;
	return(0) ;
}

// *********************************************************************
// 
int  doSetLevelDebug(int argc, char *argv[])//_FR - Rel 3.76 - 26/05/23
{
	
	if (argc<2) return(1) ;
	Gdata.leveldebug = atoi(argv[1]) ;
	return(0) ;
}

// *********************************************************************

int  doSetProtocolMode(int argc, char *argv[])
{
int i ;

	if (argc<2) return(1) ;
	
	com_write(PORT_TK, -1, TK_UPDATE) ;		 //always start spontaneous periodic IO update from Extender

    i = atoi(argv[1]) ;
    if(i == 1){
        Gsequence.BufferedMode = 1 ;
    }else{
        Gsequence.BufferedMode = 0 ;
	}
	strcpy(NextAnswer,"1") ;
	
return(0);
}
// *********************************************************************

int  doSetProtocolComunication(int argc, char *argv[])
{
int i ;

	if (argc<2) return(1) ;

    i = atoi(argv[1]) ;
	switch(i){
		case 0:		// way protocol
			Gdata.pkt_offset = 0 ;	
		break ;
		case 1:		// test protocol
			Gdata.pkt_offset = 23 ;
		break ;
	}
	strcpy(NextAnswer,"1") ;
	
return(0);
}
// *********************************************************************

int  doT_ANALOG(int argc, char *argv[])
{
int i ;

	if (argc<2) return(1);
	
	i = atoi(argv[1]) ;
	if (i>=MAX_EXTENDER_ANALOGS) i = MAX_EXTENDER_ANALOGS-1 ;
	sprintf(NextAnswer, "%d", ExtData.analogs[i] ) ;
	return(0) ;
}

// *********************************************************************

int  doT_CheckRec(int argc, char *argv[])//_GT_//OK
{

	if (argc<3) return(1);
	
	ExtData.portselect = atoi(argv[1]) ;
	ExtData.linput=strlen(argv[2]);
	Gdata.lastSlaveCommandId = WSCMD_WAIT_TKCOM ;
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_TK ;
	return(CODE_WAIT_ANSWER) ;
}

// *********************************************************************

int  doT_CNT(int argc, char *argv[])//_GT_//OK
{
int i ;

	if (argc<2) return(1);
	
    i = atoi(argv[1]) ;
	if (i>=MAX_EXTENDER_COUNTERS) i = MAX_EXTENDER_COUNTERS-1 ;
	sprintf(NextAnswer, "%ld", ExtData.counters[i] ) ;
	return(0) ;
}

// *********************************************************************

int  doT_SetFTDI(int argc, char *argv[])//_GT_//OK
{
	if (argc<2) return(1);

        if (!strcmp(argv[1], "USB")) {
            if (Gdata.portopened[PORT_MTSUSB]) {
                gtk_label_set_text( GTK_LABEL(Scrn.lbl_protname), Gdata.portdev[PORT_MTSUSB]) ;
                MTS_current_PORT=PORT_MTSUSB;
            }else{
                return(1);
            }
        }else{
            sprintf(NextAnswer, "p%s\r", argv[1]) ;
            com_write(PORT_TK, -1, NextAnswer) ;
        }

	return(0);
}

// *********************************************************************

int  doT_ComSend(int argc, char *argv[])//_GT_//OK
{
	if (argc<3) return(1);

	sprintf(NextAnswer, "C%d=%s\r", atoi(argv[1]),argv[2]) ;
	com_write(PORT_TK, -1, NextAnswer) ;

	return(0);
}

// *********************************************************************

int  doT_EmitCanFrame(int argc, char *argv[])//_GT_//OK_CAN
{
	if (argc<4) return(1);
	//E01=aa5566
	sprintf(NextAnswer, "E%d%d=%s\r", atoi(argv[1]),atoi(argv[2]),argv[3]) ;
	com_write(PORT_TK, -1, NextAnswer) ;
return(0);
}

// *********************************************************************

int  doT_GetVer(int argc, char *argv[])
{
	strcpy(ExtData.VerSn,"0");
	ExtData.VerSn[0] = '\0' ;
	strcpy(NextAnswer, "V\r" ) ;
	com_write(PORT_TK, -1, NextAnswer) ;
	
	Gsequence.TimeoutAnswer = ActCsec() + TIMEOUT_TK + 100;

	Gdata.lastSlaveCommandId = WSCMD_T_GETVER ;
	return(CODE_WAIT_ANSWER);
}

int  doT_GetType(int argc, char *argv[])
{
	sprintf(NextAnswer, "%d", Gdata.TKTYPE) ;
	return(0);
}

// *********************************************************************

int  doT_Led(int argc, char *argv[])
{
	if (argc<3)  return(1);
	
	strcpy(NextAnswer, "L ") ;
	strcat(NextAnswer, argv[1]) ;
	strcat(NextAnswer, ",") ;
	strcat(NextAnswer, argv[2]) ;
	strcat(NextAnswer, "\r") ;
	
	com_write(PORT_TK, -1, NextAnswer) ;

	strcpy(NextAnswer, "0") ;
	return(0) ;
}

// *********************************************************************

int  doT_OUTPUT(int argc, char *argv[])
{
int i ;
long t_out[16] ;

	if (argc<17) return(1) ;
	
	//lastSlaveCommand = KSLAVECMD_SETEXTIO_OUTPUT
	//extData.digitalsUpdated = False
	//lastSlaveSubCommand = cmdId	
	for(i=0;i<16;i++) t_out[i] = atol(argv[i+1]) ;
	
	sprintf(NextAnswer, "X 0,%lx,%lx\r", t_out[0], t_out[1] ) ;
	com_write(PORT_TK, -1, NextAnswer) ;
	
	sprintf(NextAnswer, "X 1,%lx,%lx\r", t_out[2], t_out[3] ) ;
	com_write(PORT_TK, -1, NextAnswer) ;
	
	sprintf(NextAnswer, "X 2,%lx,%lx\r", t_out[4], t_out[5] ) ;
	com_write(PORT_TK, -1, NextAnswer) ;
	
	sprintf(NextAnswer, "X 3,%lx,%lx\r", t_out[6], t_out[7] ) ;
	com_write(PORT_TK, -1, NextAnswer) ;

	sprintf(NextAnswer, "X 4,%lx,%lx\r", t_out[8], t_out[9] ) ;
	com_write(PORT_TK, -1, NextAnswer) ;
	
	//lastSlaveCommand = KSLAVECMD_NONE

	strcpy(NextAnswer,"0") ;

	return(0);
}

int  doT_SetPull(int argc, char *argv[])
{
int i ;
long t_out[8] ;

	if (argc<9) return(1) ;
	
	//lastSlaveCommand = KSLAVECMD_SETEXTIO_OUTPUT
	//extData.digitalsUpdated = False
	//lastSlaveSubCommand = cmdId	
	for(i=0;i<8;i++) t_out[i] = atol(argv[i+1]) ;
	
	sprintf(NextAnswer, "X 10,%lx,%lx\r", t_out[0], t_out[1] ) ;
	printf("\nDEBUG:%s\n",NextAnswer);
	com_write(PORT_TK, -1, NextAnswer) ;
	
	//sprintf(NextAnswer, "X 11,%lx,%lx\r", t_out[2], t_out[3] ) ;
	//com_write(PORT_TK, -1, NextAnswer) ;
	
	//sprintf(NextAnswer, "X 12,%lx,%lx\r", t_out[4], t_out[5] ) ;
	//com_write(PORT_TK, -1, NextAnswer) ;
	
	//lastSlaveCommand = KSLAVECMD_NONE

	strcpy(NextAnswer,"0") ;

	return(0);
}

// *********************************************************************

int  doT_SetComPort(int argc, char *argv[])//_GT_//OK
{
int i;
	//"B0=9600,n,8,1,N"
	if (argc<7) return(1);
	
	sprintf(NextAnswer, "B%d=%d,%s,%d,%d,%s\r", atoi(argv[1]),atoi(argv[2]),argv[3],atoi(argv[4]),atoi(argv[5]),argv[6]) ;
	com_write(PORT_TK, -1, NextAnswer) ;
	for(i=0;i<MAX_STRING;i++) ExtData.ComsRxBuffer[atoi(argv[1])][i]='\0' ;
	ExtData.ComsRxNr[atoi(argv[1])]=0;
	return(0);
}

// *********************************************************************

int  doT_SetCanBaudrate(int argc, char *argv[])//_GT_//OK_CAN
{
	if (argc<3) return(1);

	if (!atoi(argv[1])){
		sprintf(NextAnswer, "A0=%d\r", atoi(argv[2])) ;
	}else{
		sprintf(NextAnswer, "A1=%d\r", atoi(argv[2])) ;
	}	
	com_write(PORT_TK, -1, NextAnswer) ;
	return(0);
}

// *********************************************************************

int  doT_SetCanMailbox(int argc, char *argv[])//_GT_//OK_CAN
{
int aa ,bb ;
	
	if (argc<7) return(1);

	aa=atoi(argv[1]) ;
	bb=atoi(argv[2]) ;
	//M01 0xFFFFFFFF,0x1c030,'E',0	
	sprintf(NextAnswer, "M%d%d=%s,%s,%s,%s\r", aa,bb,argv[3],argv[4],argv[5],argv[6]) ;
	com_write(PORT_TK, -1, NextAnswer) ;
	return(0);
}

// *********************************************************************

int  doTerzista(int argc, char *argv[])
{
	strcpy(NextAnswer, Gdata.Terzista) ;
	return(0);
}
