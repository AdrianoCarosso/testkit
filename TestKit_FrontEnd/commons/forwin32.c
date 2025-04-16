//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//



#include <gtk/gtk.h>
#include <errno.h>

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <memory.h>
#include <conio.h>

#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>


#include <objbase.h>

#include <unistd.h>
// for NET
// link with Ws2_32.lib
//#pragma comment(lib,"Ws2_32.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>   // Needed for _wtoi
// for NET

#ifdef SW_MTSCU
#include "MTScu.h"
#endif

#ifdef MTSTESTKIT
#include "MtsTestKit.h"
#endif

#include "MtsGTK.h"

#include "../commons/protocol.h"
#define MAX_UDPTX       1400    // Max UDP packet len in TX


#ifdef MTSTESTKIT
int GetSerialPorts() ;
#endif

struct _COMSDATA{
	COMMTIMEOUTS	OrigCommTimeOuts ;
	DCB				Origdcb ;
 	HANDLE 			fd ;
	SOCKET			sd ;
} D_ports[PORT_MAX] ;

SHELLEXECUTEINFO ShellExInfo ;

static int GetGprsData(int mlen, char *lbuf) ;

#ifdef MTSTESTKIT
int Low_Init(int repeat)
{
//int i ;
int ret_val ;
char msg[100] ;
//Dim oWMI As Object, Subitem As Object, Items As Object

	// Get current directory path
	//getcwd(Gdata.lpath, (MAX_STRING-1));

// 	for(i=0;i<PORT_MAX;i++){
// 		Gdata.portname[i][0]='\0' ;
// 		Gdata.portopened[i] = 0 ;
// 	}	
// 	// Init TK structure
// 	FillMemory(&ExtData, sizeof(struct _TKVALS),0) ;
// 	FillMemory(&MtsData, sizeof(struct _MTSVALS),0) ;
	
	// Check if TestKit and MTS serial are present
	// Search  for 'TK_PORTNAME' and 'MTS_PORTNAME' and store com number into Gdata.portname[
	GetSerialPorts() ;
	
			

	free_msg[0] = '\0' ;
	ret_val = 0  ;
	if (!strlen(Gdata.portname[PORT_TK])){
		ret_val = 2 ;
		strcpy(free_msg, BADPORT_TK ) ;		// "Porta TestKit non trovata\n"
	}
	if (!strlen(Gdata.portname[PORT_MTS])){
		ret_val = 2 ;
		strcpy(msg, BADPORT_MTS) ;			// "Porta MTS non trovata\n"
		strcat(free_msg, msg) ;
	}
	
	if (ret_val){		// ERROR
		Gdata.menu_choice = 1 ;
		Popup("INIZIALIZZAZIONE",free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK) ;
		while (Gdata.menu_choice){
			while (gtk_events_pending ()){
				gtk_main_iteration ();
			}
		}
		//Gdata.run_loop = MAIN_END ; // _FR_
		return( ret_val );
	}
	
	// OPEN PORTs
	ret_val= com_open(PORT_TK, CBR_115200, 0 ) ;
	if (ret_val){		// ERROR
#ifdef USE_MONITORING
		printf("Error opening TK port (%d,%s)\n", ret_val, strerror(ret_val)) ;
#endif // USE_MONITORING
		Gdata.menu_choice = 1 ;
		sprintf(free_msg, ERRPORT_TK , ret_val ) ;
		Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK) ;
		while (Gdata.menu_choice){
			while (gtk_events_pending ()){
				gtk_main_iteration ();
			}
		}
		// Gdata.run_loop = MAIN_END ; 
		return( ret_val );
	}
	// Send update rate data
	strcpy(free_msg, TK_UPDATE) ;
	//i = strlen(free_msg) ;
	com_write(PORT_TK, -1, free_msg) ; 
	
	ret_val = com_open(PORT_MTS, CBR_9600, 0 ) ;
	if (ret_val){		// ERROR
#ifdef USE_MONITORING
		printf("Error opening MTS port (%d)\n", ret_val) ;
#endif // USE_MONITORING
		Gdata.menu_choice = 1 ;
		sprintf(free_msg, ERRPORT_MTS , ret_val ) ;
		Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK) ;
		while (Gdata.menu_choice){
			while (gtk_events_pending ()){
				gtk_main_iteration ();
			}
		}
		// Gdata.run_loop = MAIN_END ;
		return( ret_val );
	}
	
	
    return( 0 );
}
#endif // MTSTESTKIT

// *********************************************************************
// Function for COMs
// *********************************************************************
int baud_select(int bval)
{
int retb ;
	
	switch(bval){
		case 1200 :
			retb = CBR_1200 ;
			break ;
		case 2400 :
			retb = CBR_2400 ;
			break ;
		case 4800 :
			retb = CBR_4800 ;
			break ;
		case 19200 :
			retb = CBR_19200 ;
			break ;
		case 38400 :
			retb = CBR_38400 ;
			break ;
		case 57600 :
			retb = CBR_57600 ;
			break ;
		case 115200 :
			retb = CBR_115200 ;
			break ;
		default:
			retb = CBR_9600 ;
			break ;
	}
	return(retb) ;
}

// Change baud rate
int com_baud(int port, int brate) 
{
DCB dcb ;

	GetCommState(D_ports[port].fd, & dcb ) ;
    dcb.BaudRate = brate ;
    
    // purge any information in the buffer
    PurgeComm(D_ports[port].fd, PURGE_TXABORT | PURGE_RXABORT |
                        PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	SetCommState(D_ports[port].fd, &dcb ) ;

	return(0) ;
}


int com_open(int port, int brate, int flowcontrol)
{
int i ;
COMMTIMEOUTS CommTimeOuts ;
DCB dcb ;
WSADATA wsaData = {0};
struct sockaddr_in my_name ;
char lbuf[MAX_STRING] ;
unsigned long nb ;

	if (port==PORT_UDP){
		// Initialize Winsock
		i = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (i != 0) {
#ifdef USE_MONITORING
			printf("Port %s NOT opened\n", 
#ifdef SW_MTSCU
				   Gdata.portname ) ;
#endif
#ifdef MTSTESTKIT
				   Gdata.portname[port] ) ;
#endif
#endif // USE_MONITORINGcom_open
			return(1);
		}

		D_ports[port].sd = socket(AF_INET, SOCK_DGRAM, 0);
		if (D_ports[port].sd == INVALID_SOCKET) {
#ifdef USE_MONITORING
			printf("TCP/IP Port NOT opened\n") ;
#endif // USE_MONITORING
			return(1) ;
		}


// NOT NEEDED
// 		strcpy(freq.ifr_name, "eth0");
// 		// get IP address of device requested ( only for display it)
// 		GetAddressByName()
// 		
// 		ioctl(D_ports[port].sd, SIOCGIFADDR, &freq) ;
// 
// 		sprintf(Gdata.local_ip, "%d.%d.%d.%d",
// 				(short)(((unsigned char *)freq.ifr_addr.sa_data)[2]),
// 				(short)(((unsigned char *)freq.ifr_addr.sa_data)[3]),
// 				(short)(((unsigned char *)freq.ifr_addr.sa_data)[4]),
// 				(short)(((unsigned char *)freq.ifr_addr.sa_data)[5])) ;
// 
// #ifdef USE_MONITORING
// 			printf("Local IP is %s\n", Gdata.local_ip ) ;
// #endif // USE_MONITORING

		my_name.sin_family = AF_INET ;
		my_name.sin_addr.s_addr = INADDR_ANY ; // inet_addr(ttybuf) ; // INADDR_ANY ;
		my_name.sin_port = htons(Gdata.pc_socket) ;

		if (bind(D_ports[port].sd, (struct sockaddr*)&my_name, sizeof(my_name))  == SOCKET_ERROR) {
			closesocket(D_ports[port].sd) ;         		// error, no longer useful
			D_ports[port].sd = INVALID_SOCKET ;     // free it
		}
		if (D_ports[port].sd == INVALID_SOCKET) {
#ifdef USE_MONITORING
			printf("TCP/IP Port NOT opened\n") ;
#endif // USE_MONITORING
			return(errno) ;        // some error
		}
		
		// Set non blocking mode
		nb = 1 ;
		ioctlsocket(D_ports[port].sd, FIONBIO, &nb) ; 




	}else{
#ifdef SW_MTSCU
		sprintf(lbuf, "%s%s", DEV_PREFIX,  Gdata.portname ) ;
#endif
#ifdef MTSTESTKIT
		strcpy(lbuf, Gdata.portname[port] ) ;
#endif

		D_ports[port].fd = CreateFile(lbuf, GENERIC_READ | GENERIC_WRITE,
							0,                    // exclusive access
							NULL,                 // no security attrs
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL) ;

		if (D_ports[port].fd < 0) {
#ifdef USE_MONITORING
			printf("Port %s NOT opened\n", 
#ifdef SW_MTSCU
				   Gdata.portname ) ;
#endif
#ifdef MTSTESTKIT
				   Gdata.portname[port] ) ;
#endif
#endif // USE_MONITORING
			return(errno) ;                // some error
		}
#ifdef USE_MONITORING
		printf("Port %s opened\n", 
#ifdef SW_MTSCU
				   Gdata.portname ) ;
#endif
#ifdef MTSTESTKIT
				   Gdata.portname[port] ) ;
#endif
#endif // USE_MONITORING


		// setup device buffers
		SetupComm(D_ports[port].fd, 4096, 4096 ) ;

		// purge any information in the buffer
		PurgeComm(D_ports[port].fd, PURGE_TXABORT | PURGE_RXABORT |
							PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

		// set up for overlapped I/O
		GetCommTimeouts(D_ports[port].fd, &D_ports[port].OrigCommTimeOuts ) ;

		CommTimeOuts.ReadIntervalTimeout = MAXDWORD ;
		CommTimeOuts.ReadTotalTimeoutMultiplier = MAXDWORD ;
		CommTimeOuts.ReadTotalTimeoutConstant = 200 ;
		CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
		CommTimeOuts.WriteTotalTimeoutConstant = 10000 ;
		SetCommTimeouts(D_ports[port].fd, &CommTimeOuts) ;

		// parameters
		D_ports[port].Origdcb.DCBlength = sizeof( DCB ) ;
		GetCommState(D_ports[port].fd, &D_ports[port].Origdcb ) ;

		FillMemory(&dcb, sizeof( DCB ), 0) ;
		dcb.DCBlength = sizeof( DCB ) ;
	//  dcb.BaudRate = CBR_115200 ;
		dcb.BaudRate = brate ;
		dcb.ByteSize = 8 ;
		dcb.Parity = NOPARITY ;
		dcb.StopBits = ONESTOPBIT ;
		dcb.fBinary = TRUE ;
		dcb.fParity = FALSE ;
		dcb.fOutxCtsFlow = FALSE ;
		dcb.fOutxDsrFlow = FALSE ;
		dcb.fDtrControl = DTR_CONTROL_DISABLE  ;
		dcb.fDsrSensitivity = FALSE ;
		dcb.fOutX = FALSE ;
		dcb.fInX = FALSE ;
		dcb.fNull = FALSE ;
	//    dcb.fRtsControl = RTS_CONTROL_ENABLE ;
		dcb.fRtsControl = RTS_CONTROL_DISABLE ; 
		dcb.fAbortOnError = FALSE ;
		SetCommState(D_ports[port].fd, &dcb ) ;
#ifdef SW_MTSCU
		Gdata.portopened = 1 ;		// Only for RS232 port
#endif
#ifdef MTSTESTKIT
		Gdata.portopened[port] = 1 ;		// Only for RS232 port
#endif
	}
	//D_ports[port].isopen = 1 ;

	return(0) ;
}


// *********************************************************************

void com_close(int port) 
{
    if (port==PORT_UDP){
		closesocket(D_ports[port].sd) ;         		// error, no longer useful
		D_ports[port].sd = INVALID_SOCKET ;     // free it
		WSACleanup();
	}else{
		PurgeComm(D_ports[port].fd, PURGE_TXABORT | PURGE_RXABORT |
							PURGE_TXCLEAR | PURGE_RXCLEAR ) ;
							
		SetCommState(D_ports[port].fd, &D_ports[port].Origdcb ) ;
		SetCommTimeouts(D_ports[port].fd, &D_ports[port].OrigCommTimeOuts) ;
		CloseHandle(D_ports[port].fd) ;
	}
#ifdef SW_MTSCU
		Gdata.portopened = 0 ;		// Only for RS232 port
#endif
#ifdef MTSTESTKIT
		Gdata.portopened[port] = 0 ;		// Only for RS232 port
#endif

}

// *********************************************************************

void com_write(int port, int len, char * msg)
{
unsigned long ll ;
int sl ;

	if (len==-1) sl=strlen(msg) ;
	else sl = len ;

    if (port==PORT_UDP){
		send(D_ports[port].sd, msg, sl, 0 ) ;
	}else{
		WriteFile(D_ports[port].fd, msg, sl, &ll, (OVERLAPPED *)(0)) ;
	}
	
}

// *********************************************************************

int  com_read(int port, int maxlen, char * buf)
{
//char c ;
int i,  nrx, totrx ;
unsigned long ll ;
COMSTAT pStat ;

	nrx = 0 ;
	totrx = 0 ;

    if (port==PORT_UDP){
		i = ioctlsocket(D_ports[port].sd, FIONREAD, &ll) ;
		if ( (ll>0) && (!i) ){
			totrx = GetGprsData(ll, buf) ;
			return(totrx) ;
		}
	}else{

		i = ClearCommError( D_ports[port].fd, NULL, &pStat);
		
		if ((pStat.cbInQue) && (i!=0)){
			nrx = ((totrx+pStat.cbInQue<maxlen)? pStat.cbInQue:(maxlen-totrx)) ;
			ReadFile(D_ports[port].fd, &buf[totrx], nrx , &ll, (OVERLAPPED *)(0)) ; // WIN32

			totrx += ll ;
			
			// check if other char from input buffer
			i = ClearCommError( D_ports[port].fd, NULL, &pStat);
			pStat.cbInQue = maxlen ;
		}
	}
	
	return(totrx) ;
}

// *********************************************************************

int  com_inlen(int port) 
{
COMSTAT pStat ;
int i ;
unsigned long ll ;
 
    if (port==PORT_UDP){
		i = ioctlsocket(D_ports[port].sd, FIONREAD, &ll) ;
		if ( (ll>0) && (!i) ){
			return(ll) ;
		}
	}else{
		if (ClearCommError( D_ports[port].fd, NULL, &pStat)!=0){
			return(pStat.cbInQue) ;	
		}
#ifdef USE_MONITORING
		else
			printf ("com_inlen: port %d ERROR %s\n", port, strerror( ((int) GetLastError()) ) ) ;
#endif // USE_MONITORING
	}
	return(0) ;
}


// *********************************************************************

int  com_read_char(int port, char * buf)
{
int i ;
unsigned long ll ;
COMSTAT pStat ;

	if (port==PORT_UDP) return(0) ;
	
	// check if other char from input buffer
	i = ClearCommError( D_ports[port].fd, NULL, &pStat);
	
	if ((pStat.cbInQue) && (i!=0)){
		ReadFile(D_ports[port].fd, buf, 1 , &ll, (OVERLAPPED *)(0)) ; // WIN32
		//read(D_ports[port].fd, buf, 1) ;
		return(1) ;
	}
	
	return(0) ;
	
}

#ifdef MTSTESTKIT
// Search  for 'TK_PORTNAME' and 'MTS_PORTNAME' and store com number into Gdata.portname[
int GetSerialPorts(void) 
{
HDEVINFO hDevInfo;
SP_DEVINFO_DATA DeviceInfoData;
DWORD i;
int found, lerr ;

	// Create a HDEVINFO with all present devices.
	hDevInfo = SetupDiGetClassDevs(NULL,
		0, // Enumerator
		0,
		DIGCF_PRESENT | DIGCF_ALLCLASSES );
	
	if (hDevInfo == INVALID_HANDLE_VALUE){
		// Insert error handling here.
		lerr = (int) GetLastError() ;
#ifdef USE_MONITORING
		printf("Cannot get class device info %d\n", lerr ) ;
#endif // USE_MONITORING
		return 1;
	}
       

	// Enumerate through all devices in Set.
       
	DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (i=0;SetupDiEnumDeviceInfo(hDevInfo,i,&DeviceInfoData);i++){
		DWORD DataT ;
		LPTSTR buffer = NULL, p = NULL ;
		DWORD buffersize = 0;
           
		//
		// Call function with null to begin with, 
		// then use the returned buffer size (doubled)
		// to Alloc the buffer. Keep calling until
		// success or an unknown failure.
		//
		//  Double the returned buffersize to correct
		//  for underlying legacy CM functions that 
		//  return an incorrect buffersize value on 
		//  DBCS/MBCS systems.
		// 
		while (!SetupDiGetDeviceRegistryProperty(hDevInfo, &DeviceInfoData,
               SPDRP_FRIENDLYNAME, &DataT, (PBYTE)buffer, buffersize, &buffersize)){
               
			if (!buffersize) break ;
			lerr = (int) GetLastError() ;
			if (lerr == ERROR_INSUFFICIENT_BUFFER){
				// Change the buffer size.
				if (buffer) LocalFree(buffer);
				// Double the size to avoid problems on 
				// W2k MBCS systems per KB 888609. 
				buffer = LocalAlloc(LPTR,buffersize * 2);
			}else{
				// Insert error handling here.
#ifdef USE_MONITORING
				printf("Cannot get device info %d\n",lerr) ;
#endif // USE_MONITORING
				break;
			}
		}
           
		if (buffersize){
			found=0 ;
			if (!strncmp(buffer,TK_PORTNAME, strlen(TK_PORTNAME) )){
				//printf(" FOUND:           ->") ;
				found = 1 ;
				p = strchr(buffer,')') ;
				*p = '\0';
				p = strchr(buffer,'(') ;
				*p = '\0';
				p++ ;

				strcpy(Gdata.portname[PORT_TK], p) ;
#ifdef USE_MONITORING
				printf("Founded TK port (%s %s)\n", buffer, Gdata.portname[PORT_TK]) ;
				//printf("Result:[%s,%s]\n",buffer, p);
#endif // USE_MONITORING
			}
					
			if (!strncmp(buffer,MTS_PORTNAME, strlen(MTS_PORTNAME) )){
				//printf(" FOUND:           ->") ;
				found = 1 ;
				p = strchr(buffer,')') ;
				*p = '\0';
				p = strchr(buffer,'(') ;
				*p = '\0';
				p++ ;

				strcpy(Gdata.portname[PORT_MTS], p) ;
#ifdef USE_MONITORING
				printf("Founded TK port (%s %s)\n", buffer, Gdata.portname[PORT_MTS]) ;
				//printf("Result:[%s,%s]\n",buffer, p);
#endif // USE_MONITORING
			}
			if (buffer) LocalFree(buffer);
		}
			
	}
       
	lerr = (int) GetLastError() ;
	if ( lerr!=NO_ERROR && lerr!=ERROR_NO_MORE_ITEMS )
	{
		// Insert error handling here.
#ifdef USE_MONITORING
		printf("Error %d\n",lerr) ;
#endif // USE_MONITORING
		return 1;
	}
       
	//  Cleanup
	SetupDiDestroyDeviceInfoList(hDevInfo);
       
	return 0;
}
	
#endif // MTSTESTKIT
// *********************************************************************
// End Function for COMs
// *********************************************************************
// Function for GPRS
// *********************************************************************

int CheckIPvalid(char *l_ip)
{
unsigned long addr;

	addr = inet_addr(l_ip) ;
	if (addr==INADDR_NONE) { // ERROR
		return(1) ;
	}
	return(0) ;
}


// *********************************************************************
static int GetGprsData(int mlen, char *lbuf)
{
unsigned int pp, nrecv, plen ;
int addrlen ;
unsigned char * abc ;
//struct in_addr inadd ;
struct sockaddr_in cli_name ;
struct EHEADER_TYPE_GPRS * ehp ;
struct EH_TYPE_GPRS2 * ep2 ;
char recv_ip[DEF_STRING] ;

	addrlen = sizeof(cli_name);
	nrecv = recvfrom(D_ports[PORT_UDP].sd, lbuf, mlen, 0, (struct sockaddr*) &cli_name, &addrlen) ;
	if (!nrecv) return(0) ;
	
	recv_ip[0]='\0' ;
#ifdef USE_MONITORING
	printf("recv UDP from %s:%d len(%d)\n", inet_ntoa(cli_name.sin_addr), htons(cli_name.sin_port), mlen); 
#endif
#ifdef SW_MTSCU
	if (Gdata.bridge_ip[0]){ // if the bridge defined
		// If data from BRIDGE and ASCII data
		struct in_addr br_ip ; 
		
		//inet_aton( Gdata.bridge_ip, &br_ip);
		br_ip.s_addr = inet_addr(Gdata.bridge_ip) ;
		if ((cli_name.sin_addr.s_addr==br_ip.s_addr) && (lbuf[2]!=0)){
			// Copy data to BRIDGE buffer
			MtsData.RxBridgeLen = nrecv ;
			memcpy(MtsData.Rxbridge, lbuf, MtsData.RxBridgeLen ) ;
			return((MtsData.RxBridgeLen*-1)) ;
		}
	}
#endif // #ifdef SW_MTSCU

	// extract extended header
	ehp = (struct EHEADER_TYPE_GPRS *)(lbuf) ;
	pp = sizeof(struct EHEADER_TYPE_GPRS) ;

	// Check packet validity
	if ( (SWAP(ehp->hlen) < 20) ||					// header total len
		 (ehp->id != 0) ||							// header identifier
		 (ehp->length != 16) ||						// total length of this identifier
		 (ehp->dest_id != 0) ) {			// my socket

		printf( "EHP refused: IP=%s, hlen=%d, id=%d, len=%d, src=%ld, dst=%ld(me=0)\n",
						inet_ntoa(cli_name.sin_addr),
						ehp->hlen, ehp->id, ehp->length,
						ehp->source_id, ehp->dest_id ) ;
		return(0) ;                        // return with error
	}
	
// extract extended 2 header (if any)

	plen = ehp->hlen - 20 ;
	if (plen >= sizeof(struct EH_TYPE_GPRS2)) {

		ep2 = (struct EH_TYPE_GPRS2 *)(ehp+1) ;

		if ( (ep2->id != 2) ||              // header identifier
			 (ep2->length != 6) ) {         // total length of this identifier
			printf("EHP2 refused: IP=%s, id=%d, len=%d\n",
							inet_ntoa(cli_name.sin_addr),
							ep2->id, ep2->length) ;
			return(0) ;                    // return with error
		}

		//inadd.s_addr = htonl(ep2->ip) ;
		abc = (unsigned char *) &ep2->ip ;
		//pmc->Data_IPsckt = htons((ep2->socket))   ;
		if (ep2->ip)
			sprintf(recv_ip, "%u.%u.%u.%u", abc[3], abc[2], abc[1], abc[0] ) ;
		
		//pmc->DataReady |= DATA_IP ;
		printf("EHP2 OK: IP=%s, id=%d, len=%d (%s,%u.%u.%u.%u)\n",
							inet_ntoa(cli_name.sin_addr),
							ep2->id, ep2->length, recv_ip, 
							abc[3], abc[2], abc[1], abc[0] ) ;
	}
	pp += plen ;

#ifdef SW_MTSCU
	//Verify sender id
	if (ehp->source_id != Gdata.mts_sn){
		printf("Data refused: IP=%s, id=%lu (waited %d)\n",
						inet_ntoa(cli_name.sin_addr),
						ehp->source_id, Gdata.mts_sn ) ;
		return(0) ;
	}
	
	// Update MTS ip
	printf("Recv from %d (%s,%s)\n", Gdata.mts_sn, inet_ntoa(cli_name.sin_addr), recv_ip ) ;
		   
	printf("%s,%s\n", Gdata.mts_ip, recv_ip ) ;
	if (strncmp(Gdata.mts_ip, "192.", 4) ){
		if (strlen(recv_ip))
			strcpy(Gdata.mts_ip, recv_ip) ;
		else
			strcpy(Gdata.mts_ip, inet_ntoa(cli_name.sin_addr)) ;
		UpdateTitle() ;
	}
	printf("%s,%s\n", Gdata.mts_ip, recv_ip ) ;
		//pmc->Data_IPsckt = htons((ep2->socket))   ;
#endif // #ifdef SW_MTSCU

	// Copy data
	MtsData.RxBufLen = nrecv-pp ;
	memcpy(MtsData.RxBuffer, &lbuf[pp], MtsData.RxBufLen ) ;
	return(MtsData.RxBufLen) ;
}

// *********************************************************************
void MTS_TransByGprs(void)
{
	struct EHEADER_TYPE_GPRS ehp ;
	struct EH_TYPE_GPRS2 ep2 ;
	struct sockaddr_in rem_addr ;
	unsigned char crc ; 
	unsigned char udpbuf[MAX_UDPTX] ;   // UDP buffer

	register int i, j ; // , k ;         // general purpose
	//char tmpbuf[100] ;

	CLEAR_MEM((char*) &rem_addr, sizeof(rem_addr)) ;

//	// moved just before 'sendto'
//	rem_addr.sin_family = AF_INET ;
//	// MTS IP and socket 
//	rem_addr.sin_addr.s_addr = inet_addr(pmc->ModemDial);
//	rem_addr.sin_port = htons(pmc->DialRetry);

	j = 0 ;
//	if (pmc->ModemSM != MDM_UDPBRIDGE){
	

	crc = GetCRC_CCITT( (unsigned char*) Gdata.trans_buff, Gdata.txbuflen, 0) ;				// crc	
	
		//if (!pmc->DialRetry) pmc->DialRetry = UDPDEFSCKT ;
		
		j = 0 ;
		// prepare header EHEADER_TYPE_GPRS
		ehp.hlen = 28 ;
		ehp.id = 0 ;
		ehp.length = 16 ;
		ehp.source_num = 0 ;
		ehp.source_id = ntohl(inet_addr(Gdata.pc_ip)) ;				// my IP
		ehp.dest_id = Gdata.mts_sn ;               					// MTS name or Serial nr.
		ehp.dest_ip = ntohl(inet_addr(Gdata.mts_ip)) ;				// MTS IP
		
		// copy into udp buffer
		memcpy(&udpbuf[j], (char*)&ehp, sizeof(struct EHEADER_TYPE_GPRS));
		j = sizeof(struct EHEADER_TYPE_GPRS) ;
		
		//sprintf(tmpbuf, "UDP TX %s TrNum: %d to %s (0x%lx)", pmc->TrType, pmc->SrvTransaction, pmc->MTSname, ehp.dest_ip) ;
		//ActivityLog(tmpbuf, mdm) ;
	
		// prepare header EH_TYPE_GPRS2
		ep2.id = 2 ;
		ep2.length = 6 ;
		ep2.ip = ntohl(inet_addr(Gdata.pc_ip)) ;   // my IP
		ep2.socket = Gdata.pc_socket ;
		
		// copy into udp buffer
		memcpy(&udpbuf[j], (char*)&ep2, sizeof(struct EH_TYPE_GPRS2));
		j += sizeof(struct EH_TYPE_GPRS2) ;
		
		// check if message < MAX_UDPTX
		if ((Gdata.txbuflen+j+2)>MAX_UDPTX){
//			sprintf(tmpbuf, "UDP TX TOO LONG to %s at %s", pmc->MTSname, pmc->ModemDial) ;
//			ActivityLog(tmpbuf, mdm) ;
			return ;        
		}   
//	}
	
	// copy data to send into udp buffer    
	memcpy(&udpbuf[j], Gdata.trans_buff, Gdata.txbuflen ) ;
	j += Gdata.txbuflen ;
	

//	for (k=0;k<3;k++){
		// moved just before 'sendto'
		rem_addr.sin_family = AF_INET ;

		// MTS IP and socket 
// 		if (pmc->MTSlastsms[0]){ // Send data to Bridge
// 			rem_addr.sin_addr.s_addr = inet_addr(pmc->MTSlastsms);
// 			rem_addr.sin_port = htons(comdata.sck_bridge);
// 			sprintf(tmpbuf, "SENDING BY %s,%d", pmc->MTSlastsms,comdata.sck_bridge ) ; 
// 			ActivityLog(tmpbuf, mdm) ;
// 		}else{
			rem_addr.sin_addr.s_addr = inet_addr(Gdata.mts_ip);		// MTS IP
			rem_addr.sin_port = htons(Gdata.mts_socket);			// MTS socket
//		}
printf("Sending UDP (%s,%d)\n",Gdata.mts_ip, Gdata.mts_socket) ;

		if ((i = sendto( D_ports[PORT_UDP].sd, (char*) udpbuf, j, 0, (struct sockaddr *) &rem_addr, 
							sizeof(struct sockaddr_in))) != j ){
			printf("ERROR UDP TX (%s,%d)\n", Gdata.mts_ip, Gdata.mts_socket ) ;
//			printf("ERROR UDP TX (%d,%d) (%s,%d) %s TrNum: %d %d", Gdata.mts_ip, Gdata.mts_socket ) ;
//															pmc->TrType, pmc->SrvTransaction, pmc->ModemSM ) ;			
//			ActivityLog(tmpbuf, mdm) ;
		}else{
// 			if (pmc->ModemSM != MDM_UDPBRIDGE){
// 				sprintf(tmpbuf, "UDP TX (%s,%d) %s TrNum: %d", pmc->ModemDial, pmc->DialRetry, pmc->TrType, pmc->SrvTransaction) ;
// 			}else{
//				udpbuf[j]='\0';
				printf("Sended UDP (%s,%d)\n",Gdata.mts_ip, Gdata.mts_socket) ;
// 			}
// 			ActivityLog(tmpbuf, mdm) ;
// 			pmc->MTSlastsms[0] = '\0' ;	
			return;
		}
//	}
}

// *********************************************************************
#ifdef SW_MTSCU
void SendToBridge(void)
{
register int i ;
struct sockaddr_in rem_addr ;

	CLEAR_MEM((char*) &rem_addr, sizeof(rem_addr)) ;
	rem_addr.sin_family = AF_INET ;
	rem_addr.sin_addr.s_addr = inet_addr(Gdata.bridge_ip);
	rem_addr.sin_port = htons(Gdata.bridge_socket);

	if ((i = sendto( D_ports[PORT_UDP].sd, (char*) Gdata.trans_buff, Gdata.txbuflen, 0, (struct sockaddr *) &rem_addr, 
							sizeof(struct sockaddr_in))) != Gdata.txbuflen ){
		printf("ERROR BRIDGE TX (%s,%d)\n", Gdata.bridge_ip, Gdata.bridge_socket ) ;
	}else{
		printf("Sended BRIDGE (%s,%d)\n",Gdata.bridge_ip, Gdata.bridge_socket) ;
	}
	Gdata.txpacketnum = 0 ;
	Gdata.txbuflen = 0 ;

}
#endif // #ifdef SW_MTSCU

// *********************************************************************
// End Function for GPRS
// *********************************************************************
// Function for SEQUENCE
// *********************************************************************

#ifdef MTSTESTKIT
FILE *list_cmd ; // for debug

// Check if sequence is running
int Check_sequence(void)
{
DWORD dwRead ;

//	dwRead = WaitForSingleObject( pi.hProcess, 1000 ) ; // *INFINITE*
	dwRead = WaitForSingleObject( Gsequence.pi.hProcess, 0) ;
	switch( dwRead ){
		case WAIT_TIMEOUT: // Running
		return 0 ;
		break ;

		default :// Not running
#ifdef USE_MONITORING
			printf("SEQUENCE terminated\n") ;
#endif	// USE_MONITORING
			Gsequence.pi.hProcess = NULL ;
		break ;
	}

	Gdata.sequence_status = SEQ_ENDED ;
	fclose(list_cmd) ;
	printf("CLOSED list_cmd\n") ;
	return 1 ;
}


LONG ThreadProc(LPVOID pParam)
{
char cnfbuf[MAX_STRING] ;
unsigned long dwRead ;
int i ;

#ifdef USE_MONITORING
	sprintf(cnfbuf, "Started ThreadProc (%d)\n", Gdata.sequence_status) ;
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), cnfbuf, strlen(cnfbuf), &dwRead, NULL ) ;
	//	printf("Started ThreadProc (%d)\n", Gdata.sequence_status) ;
#endif
	
//	CSequenceCtrl *pCtrl = (CSequenceCtrl*)pParam;
	while (Gdata.sequence_status != SEQ_ENDED) {
		//if (Gdata.sequence_status == SEQ_WAITCOMMAND){
#ifdef USE_MONITORING_
			printf("Wait Command\n") ;
#endif
			dwRead = 1 ;
			if (!ReadFile( Gsequence.hOutputRead, cnfbuf, sizeof(cnfbuf), &dwRead, NULL)  || !(dwRead)) {
				if (GetLastError() == ERROR_BROKEN_PIPE){
					Gdata.sequence_status = SEQ_ENDED ;
					fclose(list_cmd) ;

#ifdef USE_MONITORING
					printf("SEQUENCE terminated (abort)\n") ;
#endif
	// 				nBytesRead = strlen(STR_CHILD_EXITED);
	// 				strcpy(Gsequence.Command, STR_CHILD_EXITED);
	// 				Gsequence.NewCommand = 1 ;
				}
#ifdef USE_MONITORING
				else
					printf("Thread error %d\n", (int) GetLastError() ); // Something bad happened.
#endif // USE_MONITORING
				return -1;	// Pipe was closed (abnormal exit).
			}

			cnfbuf[dwRead]='\0' ;
			CLEAR_MEM(&Gsequence.Command, sizeof(Gsequence.Command)) ;
			strncpy(Gsequence.Command, cnfbuf, dwRead);
#ifdef USE_MONITORING
			//printf("recv %lu\n", dwRead) ;
			printf("Recv:<%s>\n", Gsequence.Command) ;
			i = fprintf(list_cmd, "%s\r\n",Gsequence.Command) ;
			//printf("Wroted %d bytes\n", i ) ;
#endif // USE_MONITORING
			Gsequence.NewCommand = 1 ;
		//}
	}
	return(0) ;
}

// Start sequence
int run_sequence(void)
{
char name[MAX_STRING] ;
SECURITY_ATTRIBUTES sa ;
STARTUPINFO siStartInfo ;
HANDLE hOutputReadTmp ;
HANDLE hInputWriteTmp ;
DWORD dwID;

	sprintf(name, "%s\\%s", Gdata.lpath, Gdata.ProgramFile ) ;

#ifdef USE_MONITORING
	printf("Starting %s\n", name );
#endif // USE_MONITORING
	// Set the bInheritHandle flag so pipe handles are inherited.

	// Set up the security attributes struct.
	ZeroMemory(&sa, sizeof(sa)) ;
	sa.nLength= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	

	// Create a pipe for the child process's STDOUT.
	if (!CreatePipe(&hOutputReadTmp,&Gsequence.hOutputWrite,&sa,0)){
#ifdef USE_MONITORING
		printf("Stdout pipe creation failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1 ;
	}
	
// 	// Ensure that the read handle to the child process's pipe for STDOUT is not inherited.
// 	SetHandleInformation(Gsequence.hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

	// Create a duplicate of the output write handle for the std error
	// write handle. This is necessary in case the child application
	// closes one of its std output handles.
	if (!DuplicateHandle(GetCurrentProcess(), Gsequence.hOutputWrite,
						GetCurrentProcess(), &Gsequence.hErrorWrite,0,
						TRUE,DUPLICATE_SAME_ACCESS)){
#ifdef USE_MONITORING
		printf("DuplicateHandle OUT failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1 ;
	}
	
	// Create a pipe for the child process's STDIN.
	if (!CreatePipe(&Gsequence.hInputRead,&hInputWriteTmp,&sa,0)) {
#ifdef USE_MONITORING
		printf("Stdin pipe creation failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1;
	}


	// Create new output read handle and the input write handles. Set
	// the Properties to FALSE. Otherwise, the child inherits the
	// properties and, as a result, non-closeable handles to the pipes
	// are created.
	if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,
						GetCurrentProcess(),
						&Gsequence.hOutputRead, // Address of new handle.
						0,FALSE, // Make it uninheritable.
						DUPLICATE_SAME_ACCESS)){
#ifdef USE_MONITORING
		printf("DuplicateHandle READ failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1;
	}
	  
	if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,
						GetCurrentProcess(),
						&Gsequence.hInputWrite, // Address of new handle.
						0,FALSE, // Make it uninheritable.
						DUPLICATE_SAME_ACCESS)){
#ifdef USE_MONITORING
		printf("DuplicateHandle WRITE failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1;
	}


	// Close inheritable copies of the handles you do not want to be
	// inherited.
	if (!CloseHandle(hOutputReadTmp)){
#ifdef USE_MONITORING
		printf("CloseHandle Rtmp failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1;
	}
		
	if (!CloseHandle(hInputWriteTmp)){
#ifdef USE_MONITORING
		printf("CloseHandle Wtmp failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1;
	}

	// Get std input handle so you can close it and force the ReadFile to
	// fail when you want the input thread to exit.
	if ( (Gsequence.hStdIn = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE ){
#ifdef USE_MONITORING
		printf("GetHandle failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1;
	}


// 	// Ensure that the write handle to the child process's pipe for STDIN is not inherited.
// 	SetHandleInformation( Gsequence.hChildStdinWr, HANDLE_FLAG_INHERIT, 0);

	// Set up members of the STARTUPINFO structure.
	ZeroMemory( &siStartInfo, sizeof(siStartInfo) );
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = Gsequence.hErrorWrite ;
	siStartInfo.hStdOutput = Gsequence.hOutputWrite ;
	siStartInfo.hStdInput = Gsequence.hInputRead ;
	siStartInfo.dwFlags = STARTF_USESTDHANDLES;

	// Set up members of the PROCESS_INFORMATION structure.
	ZeroMemory( &Gsequence.pi, sizeof(Gsequence.pi) );


	// Start the child process.
	if ( !CreateProcess(NULL,   // No module name (use command line)
		name,           		// Command line + args
		NULL,           		// Process handle not inheritable
		NULL,           		// Thread handle not inheritable
		TRUE,           		// handles are inherited
#ifdef SEQUENCE_DEBUG
		CREATE_NEW_CONSOLE, 	// With a console
#else
		CREATE_NO_WINDOW,   	// Without a console
#endif
		NULL,           		// Use parent's environment block
		NULL,           		// Use parent's starting directory
		&siStartInfo,   		// Pointer to STARTUPINFO structure
		&Gsequence.pi )     	// Pointer to PROCESS_INFORMATION structure
		){
#ifdef USE_MONITORING
		printf( "Sequence start failed (%ld)\n", GetLastError() );
#endif // USE_MONITORING
		return 1;
	}


	CloseHandle(Gsequence.hInputRead) ;
	CloseHandle(Gsequence.hOutputWrite) ;
	CloseHandle(Gsequence.hErrorWrite) ;

	CloseHandle( Gsequence.pi.hThread );

	printf( "Sequence %s  started\n", name) ;
	// Is running
	Gdata.sequence_status = SEQ_WAITCOMMAND ;

	// Now start thread for get sequence command
	list_cmd = fopen("list_command.txt", "a") ;         // configuration file
	if (!list_cmd) printf("error create list_command.txt file\n") ;
	
// 	list_cmd = CreateFile("list_command",
//                        GENERIC_WRITE,          // open for writing
//                        0,                      // do not share
//                        NULL,                   // default security
//                        CREATE_NEW,             // create new file only
//                        FILE_ATTRIBUTE_NORMAL,  // normal file
//                        NULL);                  // no attr. template

	// Start a thread for waiting input
	Gsequence.threadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) ThreadProc,
											(LPVOID) NULL, 0, &dwID);
				
#ifdef USE_MONITORING
	//if(Gsequence.threadHandle==NULL)
		printf( "Sequence thread start is (%p)\n", Gsequence.threadHandle );
#endif // USE_MONITORING

	return(0) ;
}

// Terminate sequence
void Stop_sequence(void)
{
DWORD dwRead ;

	TerminateProcess( Gsequence.pi.hProcess, 0 ) ; // No error code

	// Wait until child process exits.
	dwRead = WaitForSingleObject( Gsequence.pi.hProcess, 1000  ) ; // *INFINITE*
    
	// Close the write end of the pipe before reading from the
	// read end of the pipe.
	if (!CloseHandle(Gsequence.hOutputWrite)) {
#ifdef USE_MONITORING
		printf("Closing handle failed1n\n");
#endif // USE_MONITORING
	}
    
	CloseHandle(Gsequence.hOutputRead) ;
	CloseHandle(Gsequence.hInputWrite) ;

	// Close process and thread handles.
	CloseHandle( Gsequence.pi.hProcess );

    printf("Sequence terminated OK\n") ;
	
}

void Send_sequence_answer(char *answer)
{
unsigned long dwRead ;

#ifdef USE_MONITORING
	printf("Send:%s", answer) ;
#endif

	if (!WriteFile(Gsequence.hInputWrite, answer, strlen(answer), &dwRead, NULL)) {
		if (GetLastError() == ERROR_NO_DATA) {
			Gdata.sequence_status = SEQ_ENDED ;
			return;	// Pipe was closed (normal exit path).
		} else{
			printf("GSEQUENCE: Error sending answer\n");
		}
	}
}


//int ExeProg(char * path, char * command, char * args )
// Start command
// 0 = Path, 1 = program, 2.. = args
int Start_command(int argc, char *argv[])
{
int esito ;
char pars[SIZE_BUFFS] ;

    CLEAR_MEM(&ShellExInfo, sizeof(ShellExInfo) ) ;
	
	strcpy(pars, argv[2] ) ;
	for(esito=3;esito<argc;esito++) {
		strcat(pars, " ") ;
		strcat(pars, argv[esito]) ;
	}

	ShellExInfo.cbSize = sizeof(ShellExInfo) ;
    ShellExInfo.fMask = SEE_MASK_NOCLOSEPROCESS ; // L'handle del task avviato viene chiuso da 'MtsTestKit'
    ShellExInfo.hwnd = NULL ; // Me.hwnd
    ShellExInfo.lpVerb = "open" ;
    ShellExInfo.lpFile = argv[1] ;
    ShellExInfo.lpParameters = pars ;
	
    ShellExInfo.lpDirectory = argv[0]  ;

// 0=vbHide, 1=vbNormalFocus ,2=vbMinimizedFocus, 3=vbMaximizedFocus, 4=vbNormalNoFocus, 5=, 6=vbMinimizedNoFocus 
#ifdef SEQUENCE_DEBUG
    ShellExInfo.nShow = 2 ; // vbMinimizedFocus  
#else
    ShellExInfo.nShow = 0 ; // vbHide
#endif

    ShellExecuteEx(&ShellExInfo) ;

    if (ShellExInfo.hProcess){
		do{
			//'#X#Repaint anything that got covered up,
			//'#X#respond to internal events and form resizings
			SLEEP(500) ;
		}while (WaitForSingleObject(ShellExInfo.hProcess, 50) == WAIT_TIMEOUT) ;
        if ( (int) ShellExInfo.hInstApp <=  32){
            esito = ((int) ShellExInfo.hInstApp) * (-1) ;
		}else{
            if (GetExitCodeProcess(ShellExInfo.hProcess, (LPDWORD)&esito) == 0)
                esito = (int) ShellExInfo.hInstApp - 32 ;

		}
        CloseHandle(ShellExInfo.hProcess) ;
	}else
		return(1) ;

	return(0) ;
}
#endif  // MTSTESTKIT

// *********************************************************************
// End Function for SEQUENCE
// *********************************************************************

// *********************************************************************

char* TimeDB(const time_t* loctime, char *aa )
{
// int ii ;
// char *cc ;
// 
// 	cc = ctime( loctime );
// 	strcpy(aa, cc) ;
// 	//ctime_s( aa, (DATEFIELD-1), loctime );
// 	ii = strlen(aa);
// 	cc[ii-1]='\0';
// 	return(	cc ) ;
struct tm *act;

	act = gmtime(loctime) ;

	sprintf(aa, "%02d/%02d/%04d %02d:%02d:%02d", 
				act->tm_mday, act->tm_mon+1, act->tm_year+1900,
				act->tm_hour, act->tm_min, act->tm_sec ) ;
	
	return(aa) ;

}

