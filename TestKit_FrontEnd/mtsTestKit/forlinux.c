//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

/*
 * Compile me with:
 *   gcc -o $(1) ($1).c $(pkg-config --cflags --libs gtk+-2.0 gmodule-2.0)
 */
//#define __DEFAULT_SOURCE
#define __USE_XOPEN
#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <dirent.h>

#include <stdlib.h>
#include <errno.h>

#include <termios.h>
#include <fcntl.h>

#include <pthread.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/if.h>
#include <string.h>

#ifdef SW_MTSCU
#include "MTScu.h"
#endif

#include "mtsTestKit.h"
#define  SEQUENCE_DEBUG
#include "mtsGTK.h"

#include "protocol.h"

#define FORNINUX_DBG_MIN
#ifdef FORNINUX_DBG_MIN
  #define DBG_MIN(fmt...) do {printf("%s: ", __FUNCTION__); printf(fmt); printf("\n"); } while(0)
#else
  #define DBG_MIN(fmt...) do { } while(0)
#endif
//#define FORNINUX_DBG_MAX
#ifdef FORNINUX_DBG_MAX
  #define DBG_MAX(fmt...) do {printf("%s: ", __FUNCTION__); printf(fmt); printf("\n"); } while(0)
#else
  #define DBG_MAX(fmt...) do { } while(0)
#endif



#define MAX_UDPTX       1400    // Max UDP packet len in TX

struct _COMSDATA{
 	struct termios oldtio ;			// port settings, before
 	struct termios newtio ;			// port settings, after
 	int fd ;
	int isopen ;
} D_ports[PORT_MAX] ;

static int GetGprsData(int mlen, char *lbuf) ;

char* rem_duble_slash(char *stringo,char *stringd) ;

extern char *strcasestr(const char *haystack, const char *needle);

// from sequence
#ifdef LOW_INIT_NEW
int Low_Init(int repeat) {
	int ret_val;
	DIR *ldir ; 
	struct dirent  *afile ;
	char msg[100] ;

	// Get current directory path
	//getcwd(Gdata.lpath, (MAX_STRING-1));

	
	// Check if TestKit and MTS serial are present
	ldir = opendir(Gdata.usb_dir);
	
	if (ldir!=NULL){
		while ( (afile=readdir(ldir))!=NULL ) {
		  if (afile->d_name[0]=='.') { continue; }
			printf("\nafile->d_name %s\n", afile->d_name) ;


			// search for TK_PORT
			if (!strncmp( afile->d_name, Gdata.tk_portname, strlen(Gdata.tk_portname)) ){
				strcpy(Gdata.portname[PORT_TK], afile->d_name);
				printf("TK port 1 found (%s)\n", Gdata.portname[PORT_TK]) ;
			  }
      else {
        // search for MTS_PORT
        if (!strncmp( afile->d_name, Gdata.mts_portname, strlen(Gdata.mts_portname)) ){
          strcpy(Gdata.portname[PORT_MTS], afile->d_name);
          Gdata.TKTYPE=1;
          printf("MTS port found (%s), TK TYPE found (%d)\n", Gdata.portname[PORT_MTS], Gdata.TKTYPE) ;
          MTS_current_PORT=PORT_MTS;
          } //else { printf("no MTS_PORTPREFIX %s\n", Gdata.mts_portname); }
        }

      #if 0
			// search for MTS_PORT new
	    int i;
			if (!strncmp( afile->d_name, MTS_PORTPREFIXNEW, strlen(MTS_PORTPREFIXNEW)) ){
				i = strlen(afile->d_name) - strlen(MTS_PORTSUFFIX) ;
				printf("\nSearch new MTS port (%s)\n", &(afile->d_name[i])) ;
				if (i>0){
					if (!strncmp( &(afile->d_name[i]), MTS_PORTSUFFIX, strlen(MTS_PORTSUFFIX)) ){
						strcpy(Gdata.portname[PORT_MTS], afile->d_name) ;
						Gdata.TKTYPE=1;
						printf("\nMTS port found (%s)\n", Gdata.portname[PORT_MTS]) ;
						printf("\nTK TYPE found (%d)\n", Gdata.TKTYPE) ;
					}
				}
			} else { printf("no MTS_PORTPREFIXNEW %s\n", MTS_PORTPREFIXNEW); }
			
			if (strcasestr( afile->d_name, MTS_PORTPREFIXNEW2) != NULL ){
				i = strlen(afile->d_name) - strlen(MTS_PORTSUFFIX) ;
				printf("\nSearch new2 MTS port (%s)\n", &(afile->d_name[i])) ;
				if (i>0){
					if (!strncmp( &(afile->d_name[i]), MTS_PORTSUFFIX, strlen(MTS_PORTSUFFIX)) ){
						strcpy(Gdata.portname[PORT_MTS], afile->d_name) ;
						Gdata.TKTYPE=1;
						printf("\nMTS port (%s)\n", Gdata.portname[PORT_MTS]) ;
            MTS_current_PORT=PORT_MTS;
						printf("\nTK TYPE (%d)\n", Gdata.TKTYPE) ;
					}
				}
			} else { printf("no MTS_PORTPREFIXNEW2 %s\n", MTS_PORTPREFIXNEW2); }
      #endif

			if (!strncmp( afile->d_name, Gdata.usb_dir, strlen(Gdata.usb_dir)) ){
				strcpy(Gdata.portname[PORT_MTSUSB], afile->d_name) ;
				printf("MTSUSB port (%s), TK TYPE (%d)\n", Gdata.portname[PORT_MTSUSB], Gdata.TKTYPE) ;
			  }	

		  }   //end while
		closedir(ldir) ;
	  }
  else{
		printf("\nError opening %s\n", Gdata.usb_dir) ;
	  }

	free_msg[0] = '\0' ;
	ret_val = 0  ;
	if (!strlen(Gdata.portname[PORT_TK])){
		ret_val = 1 ;
		strcpy(free_msg, BADPORT_TK ) ;
		printf("\nPorta TestKit non trovata\n");
	}
	if (!strlen(Gdata.portname[PORT_MTS])){
		ret_val = 1 ;
		strcpy(msg, BADPORT_MTS) ;
		strcat(free_msg, msg) ;
		printf("\nPorta MTS non trovata\n");
	}
	
	if (ret_val){		// ERROR
		printf("\nErrore su com ret_val<'%d'> e repeat<'%d'>\n", ret_val,repeat);	
		if(repeat){
			Gdata.menu_choice = 1 ;
			Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
			while (Gdata.menu_choice){
				while (gtk_events_pending ()){
					gtk_main_iteration ();
				}
			}
			//Gdata.run_loop = MAIN_END ; // _FR_
		}
		return( ret_val+repeat );
	}
	
	// OPEN PORTs
	ret_val= com_open(PORT_TK, B115200, 0 ) ;
	if (ret_val){		// ERROR
		printf("\nError opening TK port (%d,%s)\n", ret_val, strerror(ret_val)) ;
		Gdata.menu_choice = 1 ;
		sprintf(free_msg, ERRPORT_TK , ret_val ) ;
		Popup("INIZIALIZZAZIONE",free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
		while (Gdata.menu_choice){
			while (gtk_events_pending ()){
				gtk_main_iteration ();
			}
		}
		//Gdata.run_loop = MAIN_END ;
		return( ret_val );
	}
	// Send update rate data
	strcpy(free_msg, TK_UPDATE) ;
	//i = strlen(free_msg) ;
	com_write(PORT_TK, -1, free_msg) ; 
	
        ret_val = com_open(MTS_current_PORT, B9600, 0 ) ;
	if (ret_val){		// ERROR
		printf("\nError opening MTS port (%d)\n", ret_val) ;
		Gdata.menu_choice = 1 ;
		sprintf(free_msg, ERRPORT_MTS , ret_val ) ;
		Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
		while (Gdata.menu_choice){
			while (gtk_events_pending ()) { gtk_main_iteration (); }
		  }
		//Gdata.run_loop = MAIN_END ;
		return( ret_val );
	}
	if (strlen(Gdata.portname[PORT_MTSUSB])){
    ret_val = com_open(PORT_MTSUSB, B9600, 0 ) ;
    if (ret_val){		// ERROR
      printf("\nError opening MTS USB port (%d)\n", ret_val) ;
      Gdata.menu_choice = 1 ;
      sprintf(free_msg, ERRPORT_MTS , ret_val ) ;
      Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
      while (Gdata.menu_choice){
          while (gtk_events_pending ()){
              gtk_main_iteration ();
          }
        }
      //Gdata.run_loop = MAIN_END ;
      return( ret_val );
      }
    }
	
	
    return( 0 );
}

#else

int Low_Init(int repeat) {
int i, ret_val ;
DIR *ldir ; 
struct dirent  *afile ;
char msg[100] ;

	// Get current directory path
	//getcwd(Gdata.lpath, (MAX_STRING-1));

	
	// Check if TestKit and MTS serial are present
	ldir = opendir(USB_DIR);
	
	if (ldir!=NULL){
		while (( afile=readdir(ldir))!=NULL){
			printf("\nNew file %s\n", afile->d_name) ;
			// search for TK_PORT
			if (!strncmp( afile->d_name, TK_PORTNAME, strlen(TK_PORTNAME)) ){
				strcpy(Gdata.portname[PORT_TK], afile->d_name) ;
				printf("\nTK port found(%s)\n", Gdata.portname[PORT_TK]) ;
			}
			if (strcasestr( afile->d_name, TK_PORTNAME2) != NULL ){
				strcpy(Gdata.portname[PORT_TK], afile->d_name) ;
				printf("\nFounded TK port (%s)\n", Gdata.portname[PORT_TK]) ;
			}
			
			// search for MTS_PORT old
			if (!strncmp( afile->d_name, MTS_PORTPREFIX, strlen(MTS_PORTPREFIX)) ){
				i = strlen(afile->d_name) - strlen(MTS_PORTSUFFIX) ;
				//printf("\nSearch MTS port (%s)\n", &(afile->d_name[i])) ;
				if (i>0){
					if (!strncmp( &(afile->d_name[i]), MTS_PORTSUFFIX, strlen(MTS_PORTSUFFIX)) ){
						strcpy(Gdata.portname[PORT_MTS], afile->d_name) ;
						Gdata.TKTYPE=0;
						printf("\nMTS port found(%s)\n", Gdata.portname[PORT_MTS]) ;
						printf("\nTK TYPE found(%d)\n", Gdata.TKTYPE) ;
					}
				}
			}
			// search for MTS_PORT new
			if (!strncmp( afile->d_name, MTS_PORTPREFIXNEW, strlen(MTS_PORTPREFIXNEW)) ){
				i = strlen(afile->d_name) - strlen(MTS_PORTSUFFIX) ;
				//printf("\nSearch MTS port (%s)\n", &(afile->d_name[i])) ;
				if (i>0){
					if (!strncmp( &(afile->d_name[i]), MTS_PORTSUFFIX, strlen(MTS_PORTSUFFIX)) ){
						strcpy(Gdata.portname[PORT_MTS], afile->d_name) ;
						Gdata.TKTYPE=1;
						printf("\nFounded MTS port (%s)\n", Gdata.portname[PORT_MTS]) ;
						printf("\nFounded TK TYPE (%d)\n", Gdata.TKTYPE) ;
					}
				}
			}
			if (strcasestr( afile->d_name, MTS_PORTPREFIXNEW2) != NULL ){
				i = strlen(afile->d_name) - strlen(MTS_PORTSUFFIX) ;
				//printf("\nSearch MTS port (%s)\n", &(afile->d_name[i])) ;
				if (i>0){
					if (!strncmp( &(afile->d_name[i]), MTS_PORTSUFFIX, strlen(MTS_PORTSUFFIX)) ){
						strcpy(Gdata.portname[PORT_MTS], afile->d_name) ;
						Gdata.TKTYPE=1;
						printf("\nFounded MTS port (%s)\n", Gdata.portname[PORT_MTS]) ;
                                                MTS_current_PORT=PORT_MTS;
						printf("\nFounded TK TYPE (%d)\n", Gdata.TKTYPE) ;
					}
				}
			}
			if (!strncmp( afile->d_name, MTS_USB_PORTPREFIX, strlen(MTS_USB_PORTPREFIX)) ){
				strcpy(Gdata.portname[PORT_MTSUSB], afile->d_name) ;
				printf("\nFounded MTSUSB port (%s)\n", Gdata.portname[PORT_MTSUSB]) ;
				printf("\nFounded TK TYPE (%d)\n", Gdata.TKTYPE) ;
			}	
		}
		closedir(ldir) ;
	}else{
		printf("\nError opening %s\n", USB_DIR) ;
	}

	free_msg[0] = '\0' ;
	ret_val = 0  ;
	if (!strlen(Gdata.portname[PORT_TK])){
		ret_val = 1 ;
		strcpy(free_msg, BADPORT_TK ) ; // "Porta TestKit non trovata\n"
	}
	if (!strlen(Gdata.portname[PORT_MTS])){
		ret_val = 1 ;
		strcpy(msg, BADPORT_MTS) ;			// "Porta MTS non trovata\n"
		strcat(free_msg, msg) ;
	}
	
	if (ret_val){		// ERROR
		//printf("\nErrore su com ret_val<'%d'> e repeat<'%d'>\n", ret_val,repeat);	
		if(repeat){
			Gdata.menu_choice = 1 ;
			Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
			while (Gdata.menu_choice){
				while (gtk_events_pending ()){
					gtk_main_iteration ();
				}
			}
			//Gdata.run_loop = MAIN_END ; // _FR_
		}
		return( ret_val+repeat );
	}
	
	// OPEN PORTs
	ret_val= com_open(PORT_TK, B115200, 0 ) ;
	if (ret_val){		// ERROR
		printf("\nError opening TK port (%d,%s)\n", ret_val, strerror(ret_val)) ;
		Gdata.menu_choice = 1 ;
		sprintf(free_msg, ERRPORT_TK , ret_val ) ;
		Popup("INIZIALIZZAZIONE",free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
		while (Gdata.menu_choice){
			while (gtk_events_pending ()){
				gtk_main_iteration ();
			}
		}
		//Gdata.run_loop = MAIN_END ;
		return( ret_val );
	}
	// Send update rate data
	strcpy(free_msg, TK_UPDATE) ;
	//i = strlen(free_msg) ;
	com_write(PORT_TK, -1, free_msg) ; 
	
        ret_val = com_open(MTS_current_PORT, B9600, 0 ) ;
	if (ret_val){		// ERROR
		printf("\nError opening MTS port (%d)\n", ret_val) ;
		Gdata.menu_choice = 1 ;
		sprintf(free_msg, ERRPORT_MTS , ret_val ) ;
		Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
		while (Gdata.menu_choice){
			while (gtk_events_pending ()){
				gtk_main_iteration ();
			}
		}
		//Gdata.run_loop = MAIN_END ;
		return( ret_val );
	}
	if (strlen(Gdata.portname[PORT_MTSUSB])){
            ret_val = com_open(PORT_MTSUSB, B9600, 0 ) ;
            if (ret_val){		// ERROR
                printf("\nError opening MTS USB port (%d)\n", ret_val) ;
                Gdata.menu_choice = 1 ;
                sprintf(free_msg, ERRPORT_MTS , ret_val ) ;
                Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
                while (Gdata.menu_choice){
                    while (gtk_events_pending ()){
                        gtk_main_iteration ();
                    }
                }
                //Gdata.run_loop = MAIN_END ;
                return( ret_val );
            }
        }
	
	
    return( 0 );
}

#endif

// *********************************************************************
// Function for COMs
// *********************************************************************

int baud_select(int bval) {
  int retb ;
	
	switch(bval){
		case 1200 :
			retb = B1200 ;
			break ;
		case 2400 :
			retb = B2400 ;
			break ;
		case 4800 :
			retb = B4800 ;
			break ;
		case 19200 :
			retb = B19200 ;
			break ;
		case 38400 :
			retb = B38400 ;
			break ;
		case 57600 :
			retb = B57600 ;
			break ;
		case 115200 :
			retb = B115200 ;
			break ;
		default:
			retb = B9600 ;
			break ;
	}
	return(retb) ;
}

// Change baud rate
int com_baud(int port, int brate) {
  struct termios * pnewtio =  &(D_ports[port].newtio) ;

	//cfsetspeed(pnewtio, brate) ; 
	pnewtio->c_cflag = brate | CS8 | CLOCAL | CREAD ;
	
	tcflush(D_ports[port].fd , TCIFLUSH) ;
	tcsetattr(D_ports[port].fd , TCSANOW, pnewtio) ;
	
	return(0) ;
}

int com_open(int port, int brate, int flowcontrol) {
  int i ;
  char *p, msg[100] ;
    
  struct termios * pnewtio =  &(D_ports[port].newtio) ;
  struct termios * poldtio  = &(D_ports[port].oldtio) ;
  char lbuf[2*MAX_STRING] ;

  struct sockaddr_in my_name ;
  struct ifreq freq ;

// Open device for reading and writing and not as controlling tty
// because we don't want to get killed if linenoise sends CTRL-C.
	
	if (port==PORT_UDP){		
		D_ports[port].fd = socket(AF_INET, SOCK_DGRAM, 0) ;
		if (D_ports[port].fd < 0) {
			DBG_MIN("Socket Port NOT opened\n") ;
			return(errno) ;                // some error
		}
	
		strcpy(freq.ifr_name, "eth0");
		// get IP address of device requested ( only for display it)
		ioctl(D_ports[port].fd, SIOCGIFADDR, &freq) ;

#ifdef SW_MTSCU
		sprintf(Gdata.local_ip, "%d.%d.%d.%d",
				(short)(((unsigned char *)freq.ifr_addr.sa_data)[2]),
				(short)(((unsigned char *)freq.ifr_addr.sa_data)[3]),
				(short)(((unsigned char *)freq.ifr_addr.sa_data)[4]),
				(short)(((unsigned char *)freq.ifr_addr.sa_data)[5])) ;

			printf("\nLocal IP is %s\n", Gdata.local_ip ) ;
#endif

		my_name.sin_family = AF_INET ;
		my_name.sin_addr.s_addr = INADDR_ANY ; // inet_addr(ttybuf) ; // INADDR_ANY ;
		my_name.sin_port = htons(Gdata.pc_socket) ;

		if (bind(D_ports[port].fd, (struct sockaddr*)&my_name, sizeof(my_name)) < 0) {
			close(D_ports[port].fd) ;         // error, no longer useful
			D_ports[port].fd = -1 ;           // free it
		}
		if (D_ports[port].fd < 0) {
			printf("\nTCP/IP Port NOT opened\n") ;
			return(errno) ;        // some error
		}
		fcntl(D_ports[port].fd, F_SETFL, O_NONBLOCK) ;
		
	}
else  { // Com port
#ifdef SW_MTSCU
		sprintf(lbuf, "%s%s", DEV_PREFIX,  Gdata.portname ) ;
#endif
		sprintf(lbuf, "%s/%s", Gdata.usb_dir, Gdata.portname[port] ) ;
    i = readlink(lbuf, msg, 99 ) ;
    msg[i]='\0' ;
    p = strrchr(msg, '/') ;
    if (p) {
      sprintf(Gdata.portdev[port], "/dev%s", p) ;
    }else{
      sprintf(Gdata.portdev[port], "/dev%s", msg) ;
    }
		
		D_ports[port].fd = open(lbuf, O_RDWR | O_NOCTTY ) ;
		if (D_ports[port].fd < 0) {
			DBG_MIN("Port %s NOT opened\n", lbuf) ;
			return(errno) ;                // some error
		}
		DBG_MAX("Port %s opened (%d)", lbuf, port) ;

		tcgetattr(D_ports[port].fd, poldtio ) ;        // save current port settings
		bzero(pnewtio, sizeof(struct termios)) ;

// BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
// CRTSCTS : output hardware flow control (only used if the cable has
//             all necessary lines. See sect. 7 of Serial-HOWTO)
// CS8           : 8n1 (8bit,no parity,1 stopbit)
// CSTOPB  : 2 stop bit
// CLOCAL  : local connection, no modem contol
// CREAD   : enable receiving characters
// HUPCL   : lower modem control lines after last process closes
//             the device (hang up).

		if (flowcontrol) {
			pnewtio->c_cflag = brate | CRTSCTS | CS8 | HUPCL | CLOCAL | CREAD ;
		} else {
			pnewtio->c_cflag = brate | CS8 | CLOCAL | CREAD ;
		}
//pdebug("New tio %x", pnewtio->c_cflag) ;
// IGNBRK  : ignore breaks
// IGNPAR  : ignore bytes with parity errors
// otherwise make device raw (no other input processing)

		pnewtio->c_iflag = IGNPAR | IGNBRK ;

// Raw output.
		pnewtio->c_oflag = 0 ;

// Set input mode (non-canonical, no echo,...)
		pnewtio->c_lflag = 0;

// initialize all control characters
// default values can be found in /usr/include/termios.h, and are given
// in the comments, but we don't need them here

		pnewtio->c_cc[VINTR]    = 0 ;        // Ctrl-c
		pnewtio->c_cc[VQUIT]    = 0 ;        // Ctrl-bkslash
		pnewtio->c_cc[VERASE]   = 0 ;        // del
		pnewtio->c_cc[VKILL]    = 0 ;        // @
		pnewtio->c_cc[VEOF]     = 0 ;        // Ctrl-d
		pnewtio->c_cc[VSWTC]    = 0 ;        // '0'
		pnewtio->c_cc[VSTART]   = 0 ;        // Ctrl-q
		pnewtio->c_cc[VSTOP]    = 0 ;        // Ctrl-s
		pnewtio->c_cc[VSUSP]    = 0 ;        // Ctrl-z
		pnewtio->c_cc[VEOL]     = 0 ;        // '0'
		pnewtio->c_cc[VREPRINT] = 0 ;        // Ctrl-r
		pnewtio->c_cc[VDISCARD] = 0 ;        // Ctrl-u
		pnewtio->c_cc[VWERASE]  = 0 ;        // Ctrl-w
		pnewtio->c_cc[VLNEXT]   = 0 ;        // Ctrl-v
		pnewtio->c_cc[VEOL2]    = 0 ;        // '0'

/// 	pnewtio->c_cc[VTIME]    = 1 ;        // inter-character timer unused 0.1 sec
		pnewtio->c_cc[VTIME]    = 0 ;        // no timeout (use select)
		pnewtio->c_cc[VMIN]     = 1 ;        // blocking read until x chars received

//  now clean the modem line and activate the settings for the port

		tcflush(D_ports[port].fd , TCIFLUSH) ;
		tcsetattr(D_ports[port].fd , TCSANOW, pnewtio) ;
#ifdef SW_MTSCU
		Gdata.portopened = 1 ;		// Only for RS232 port
#endif
	}
	Gdata.portopened[port] = 1 ;	
	
	D_ports[port].isopen = 1 ;

	SLEEP(10) ;                // settling time
	
	return(0) ;
}

// *********************************************************************

void com_close(int port) {

	if (D_ports[port].isopen){
		if (port!=PORT_UDP){
			tcflush(D_ports[port].fd, TCIOFLUSH) ;                // buffer flush
			tcsetattr(D_ports[port].fd, TCSANOW, &(D_ports[port].oldtio) ) ;

			tcflush(D_ports[port].fd, TCIOFLUSH) ;                // buffer flush
		}
		close(D_ports[port].fd) ;
	}
	D_ports[port].isopen = 0 ;
#ifdef SW_MTSCU
	Gdata.portopened = 0 ;		// Only for RS232 port
#endif
	Gdata.portopened[port] = 0 ;
	DBG_MAX("ComPortclosed") ;
}

// *********************************************************************

void com_write(int port, int len, char * msg) {
int sl ;

	if (len==-1) sl=strlen(msg) ;
	else sl = len ;
	{
		int aa = 0;
		while(aa<sl){
			printf("[%d]",(msg[aa++] & 0xff) ) ;
		}
		printf("\n") ;
	}
	write(D_ports[port].fd, msg, sl) ;
}

// *********************************************************************

int  com_read(int port, int maxlen, char * buf) {
//char c ;
int i, bytes,  nrx, totrx ;

	nrx = 0 ;
	totrx = 0 ;
	// check if other char from input buffer
	i = ioctl(D_ports[port].fd , FIONREAD, &bytes) ;
	if ((bytes) && (i>-1)){
		//do{
			nrx = ((totrx+bytes<maxlen)? bytes:(maxlen-totrx)) ;
		  DBG_MAX("\ncom_read: COM%d recv %d(%d) bytes\n", port, bytes,nrx) ;
			if (port==PORT_UDP){
				totrx = GetGprsData(nrx, buf) ;
				return(totrx) ;
			}else{
				i = read(D_ports[port].fd, &buf[totrx], nrx) ;
			}
			totrx += i ;
			
			// check if other char from input buffer
			i = ioctl(D_ports[port].fd , FIONREAD, &bytes) ;
		//}while ((bytes) && (i>-1) && (totrx<maxlen)) ;
	}
	
	return(totrx) ;
}

// *********************************************************************
int  com_inlen(int port) {
  int bytes ;

	if (ioctl(D_ports[port].fd , FIONREAD, &bytes)>-1){
		if (bytes) { DBG_MAX("COM%d recv %d bytes\n", port, bytes); }
		return(bytes) ;
	  }
	return(0) ;
}

// *********************************************************************
int  com_read_char(int port, char * buf) {
  int i, bytes ;

	// check if other char from input buffer
	i = ioctl(D_ports[port].fd , FIONREAD, &bytes) ;
	if ((bytes) && (i>-1)){
		read(D_ports[port].fd, buf, 1) ;
		return(1) ;
	}
	
	return(0) ;
	
}

// *********************************************************************
// End Function for COMs
// *********************************************************************
// Function for GPRS
// *********************************************************************
int CheckIPvalid(char *l_ip)
{
struct in_addr addr;

	if (inet_aton(l_ip, &addr) == 0) { // ERROR
		return(1) ;
	}
	return(0) ;
}


// *********************************************************************
static int GetGprsData(int mlen, char *lbuf)
{
unsigned int pp, nrecv, addrlen, plen ;
unsigned char * abc ;
//struct in_addr inadd ;
struct sockaddr_in cli_name ;
struct EHEADER_TYPE_GPRS * ehp ;
struct EH_TYPE_GPRS2 * ep2 ;
char recv_ip[DEF_STRING] ;

	addrlen = sizeof(cli_name);
	nrecv = recvfrom(D_ports[PORT_UDP].fd, lbuf, mlen, 0, (struct sockaddr*) &cli_name, &addrlen) ;
	if (!nrecv) return(0) ;
	
	recv_ip[0]='\0' ;
	printf("\nrecv UDP from %s:%d len(%d)\n", inet_ntoa(cli_name.sin_addr), htons(cli_name.sin_port), mlen); 

#ifdef SW_MTSCU
	if (Gdata.bridge_ip[0]){ // if the bridge defined
		// If data from BRIDGE and ASCII data
		struct in_addr br_ip ; 
		
		inet_aton( Gdata.bridge_ip, &br_ip);
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

		printf( "\nEHP refused: IP=%s, hlen=%d, id=%d, len=%d, src=%d, dst=%d(me=0)\n",
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
			printf("\nEHP2 refused: IP=%s, id=%d, len=%d\n",
							inet_ntoa(cli_name.sin_addr),
							ep2->id, ep2->length) ;
			return(0) ;                    // return with error
		}

		//inadd.s_addr = htonl(ep2->ip) ;
		//abc = (unsigned char *) &ep2->ip ;
		abc = (unsigned char *) &ep2 ;
		abc+=2;
		//pmc->Data_IPsckt = htons((ep2->socket))   ;
		if (ep2->ip)
			sprintf(recv_ip, "%u.%u.%u.%u", abc[3], abc[2], abc[1], abc[0] ) ;
		
		//pmc->DataReady |= DATA_IP ;
		printf("\nEHP2 OK: IP=%s, id=%d, len=%d (%s,%u.%u.%u.%u)\n",
							inet_ntoa(cli_name.sin_addr),
							ep2->id, ep2->length, recv_ip, 
							abc[3], abc[2], abc[1], abc[0] ) ;
	}
	pp += plen ;

#ifdef SW_MTSCU
	//Verify sender id
	if (ehp->source_id != Gdata.mts_sn){
		printf("\nData refused: IP=%s, id=%lu (waited %d)\n",
						inet_ntoa(cli_name.sin_addr),
						ehp->source_id, Gdata.mts_sn ) ;
		return(0) ;
	}
	
	// Update MTS ip
	printf("\nRecv from %d (%s,%s)\n", Gdata.mts_sn, inet_ntoa(cli_name.sin_addr), recv_ip ) ;
		   
	printf("\n%s,%s\n", Gdata.mts_ip, recv_ip ) ;
	if (strncmp(Gdata.mts_ip, "192.", 4) ){
		if (strlen(recv_ip))
			strcpy(Gdata.mts_ip, recv_ip) ;
		else
			strcpy(Gdata.mts_ip, inet_ntoa(cli_name.sin_addr)) ;
		UpdateTitle() ;
	}
	printf("\n%s,%s\n", Gdata.mts_ip, recv_ip ) ;
		//pmc->Data_IPsckt = htons((ep2->socket))   ;
#endif // #ifdef SW_MTSCU

	// Copy data
	MtsData.RxBufLen = nrecv-pp ;
	memcpy(MtsData.RxBuffer, &lbuf[pp], MtsData.RxBufLen ) ;
	return(MtsData.RxBufLen) ;
}

// *********************************************************************
#ifdef SW_MTSCU
void SendToBridge(void)
{
register int i ;
struct sockaddr_in rem_addr ;

	bzero((char*) &rem_addr, sizeof(rem_addr)) ;
	rem_addr.sin_family = AF_INET ;
	rem_addr.sin_addr.s_addr = inet_addr(Gdata.bridge_ip);
	rem_addr.sin_port = htons(Gdata.bridge_socket);

	if ((i = sendto( D_ports[PORT_UDP].fd, (char*) Gdata.trans_buff, Gdata.txbuflen, 0, (struct sockaddr *) &rem_addr, 
							sizeof(struct sockaddr_in))) != Gdata.txbuflen ){
		printf("\nERROR BRIDGE TX (%s,%d)\n", Gdata.bridge_ip, Gdata.bridge_socket ) ;
	}else{
		printf("\nSended BRIDGE (%s,%d)\n",Gdata.bridge_ip, Gdata.bridge_socket) ;
	}
	Gdata.txpacketnum = 0 ;
	Gdata.txbuflen = 0 ;

}
#endif // #ifdef SW_MTSCU

// *********************************************************************
void MTS_TransByGprs(void)
{
	struct EHEADER_TYPE_GPRS ehp ;
	struct EH_TYPE_GPRS2 ep2 ;
	struct sockaddr_in rem_addr ;
//	unsigned char crc ; 
	unsigned char udpbuf[MAX_UDPTX] ;   // UDP buffer

	register int i, j ; // , k ;         // general purpose
	//char tmpbuf[100] ;

	bzero((char*) &rem_addr, sizeof(rem_addr)) ;

//	// moved just before 'sendto'
//	rem_addr.sin_family = AF_INET ;
//	// MTS IP and socket 
//	rem_addr.sin_addr.s_addr = inet_addr(pmc->ModemDial);
//	rem_addr.sin_port = htons(pmc->DialRetry);

	j = 0 ;
//	if (pmc->ModemSM != MDM_UDPBRIDGE){
	

	//crc = 
	GetCRC_CCITT( (unsigned char*) Gdata.trans_buff, Gdata.txbuflen, 0) ;				// crc	
	
		//if (!pmc->DialRetry) pmc->DialRetry = UDPDEFSCKT ;
		
		j = 0 ;
		// prepare header EHEADER_TYPE_GPRS
		ehp.hlen = 28 ;
		ehp.id = 0 ;
		ehp.length = 16 ;
		ehp.source_num = 0 ;
		ehp.source_id = ntohl(inet_addr(Gdata.pc_ip)) ; // my IP
		ehp.dest_id = Gdata.mts_sn ;               					// MTS name or Serial nr.
		ehp.dest_ip = ntohl(inet_addr(Gdata.mts_ip)) ;				// MTS IP
	
		printf("\nEHP %lu,%lu:\n<\nHlen=%d;\nid=%d;\nlength=%d;\nsource_num=%u;\nsource_id=%u;\ndest_id=%u;\ndest_ip=%u\n>\n",sizeof(struct EHEADER_TYPE_GPRS),sizeof(struct EH_TYPE_GPRS2),ehp.hlen,ehp.id,ehp.length,ehp.source_num,ehp.source_id,ehp.dest_id,ehp.dest_ip) ;       

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
printf("\nSending UDP (%s,%d)\n",Gdata.mts_ip, Gdata.mts_socket) ;

		if ((i = sendto( D_ports[PORT_UDP].fd, (char*) udpbuf, j, 0, (struct sockaddr *) &rem_addr, 
							sizeof(struct sockaddr_in))) != j ){
			printf("\nERROR UDP TX (%s,%d)\n", Gdata.mts_ip, Gdata.mts_socket ) ;
//			printf("\nERROR UDP TX (%d,%d) (%s,%d) %s TrNum: %d %d", Gdata.mts_ip, Gdata.mts_socket ) ;
//															pmc->TrType, pmc->SrvTransaction, pmc->ModemSM ) ;			
//			ActivityLog(tmpbuf, mdm) ;
		}else{
// 			if (pmc->ModemSM != MDM_UDPBRIDGE){
// 				sprintf(tmpbuf, "UDP TX (%s,%d) %s TrNum: %d", pmc->ModemDial, pmc->DialRetry, pmc->TrType, pmc->SrvTransaction) ;
// 			}else{
//				udpbuf[j]='\0';
				printf("\nSended UDP (%s,%d)\n",Gdata.mts_ip, Gdata.mts_socket) ;
// 			}
// 			ActivityLog(tmpbuf, mdm) ;
// 			pmc->MTSlastsms[0] = '\0' ;	
			return;
		}
//	}
}


// *********************************************************************
// End Function for GPRS
// *********************************************************************
// Function for SEQUENCE
// *********************************************************************

// Check if sequence is running
int Check_sequence(void) {
  int status, retcode ;

	retcode = waitpid(Gsequence.pid, &status, WNOHANG) ;
	if (retcode == Gsequence.pid) {
		if (WIFEXITED(status))
			if( ( WIFEXITED(status) == 0 ) )
				DBG_MAX("Sequence terminated OK") ;
			else
				DBG_MIN("SEQUENCE terminated 0x%X", WEXITSTATUS(status)) ;
		else
			DBG_MIN("SEQUENCE terminated (abort1)") ;
		Gsequence.pid = -1 ;
		kill(Gsequence.thr_p, SIGTERM) ;
	}else
		return(0) ;

	Gdata.sequence_status = SEQ_ENDED ;
/*	fclose(list_cmd) ;
	printf("\nCLOSED list_cmd\n") ;
#endif
*/		
	return 1 ;
}


static void * ThreadProc(void * arg) {
  //char c ;
  int i ;
  //int ll, i ;
  char cnfbuf[MAX_STRING] ;
  unsigned long dwRead ;

  //	ll = 0 ;
	while (Gdata.sequence_status != SEQ_ENDED) {
		//printf("\nWait Command\n") ;
		i = read(Gsequence.cgi_output[0], cnfbuf, MAX_STRING) ;
		//printf("\nThread SEQUENCE: %d\n",i) ;
		switch(i){
			case -1: // Error
 			Gdata.sequence_status = SEQ_ENDED ;
//			fclose(list_cmd) ;
//			printf("\nCLOSED list_cmd\n") ;
			printf("\nSEQUENCE terminated (abort2)\n");

			pthread_exit(NULL) ;        // terminate thread
			return(NULL) ;
			break ;
			
// 			case 0:
// 			printf("\nSEQUENCE no data\n") ;
// #endif
// 			if (ll){
// 				Gsequence.Command[ll++] = '\0' ;
// 				Gsequence.NewCommand = 1 ;
// 				ll = 0 ;
// 			}
// 			SLEEP(10) ;
// 			break ;
			
//			case 1:
			default:
			dwRead =  i ;
			cnfbuf[dwRead]='\0' ;
			CLEAR_MEM(&Gsequence.Command, sizeof(Gsequence.Command)) ;
			strncpy(Gsequence.Command, cnfbuf, dwRead);
			Gsequence.NewCommand = 1 ;
			//printf("\nRecv:<%s>\n", Gsequence.Command) ;

//			i = fprintf(list_cmd, "%s\r\n",Gsequence.Command) ;
			//printf("Wroted %d bytes\n", i ) ;

// 			printf("SEQUENCE char <%x,%c>\n", c, c) ;
// 			Gsequence.Command[ll++] = c ;
// 			if (c=='\0') {
// 				ll = 0 ;
// 				Gsequence.NewCommand = 1 ;
// 			}
		}
		g_usleep(100000) ;
		DBG_MAX("Ssat %d Command", i) ;
		
	}
	DBG_MIN("SEQUENCE thread end") ;
	pthread_exit(NULL) ;        // terminate thread
	return(NULL) ;
}

// Start sequence
int run_sequence(void) {
  char name[2*MAX_STRING+2];
  int retcode;
	
	retcode = strlen(Gdata.ProgramFile) ; 
	sprintf(name, "./app/%s", Gdata.ProgramFile ) ;		
	DBG_MIN("Starting %s", name );

	if (pipe(Gsequence.cgi_output) < 0) {
		DBG_MIN("Stdout pipe creation failed (%d)", errno );
		return 1;
	}
	
	if (pipe(Gsequence.cgi_input) < 0) {
		DBG_MIN("Stdin pipe creation failed (%d)", errno );
		return 1;
	  }

	if ( (Gsequence.pid = fork()) < 0 ) {
		DBG_MIN("Sequence start failed (%d)", errno );
		return 1;
	  }
	if (Gsequence.pid == 0) { // child: CGI script
		dup2(Gsequence.cgi_output[1], 1);
		dup2(Gsequence.cgi_input[0], 0);

#ifdef SEQUENCE_DEBUG // fnctl(oldfd, F DUPFD, newfd)
		if (execlp("/bin/sh", "sh", "-c", name, (char *) 0)){
#else
		if (execl(name, name, NULL)){ // Starting sequence
#endif
			DBG_MIN("AA Sequence exe failed (%d)", errno );
		}
		return(0);
		
	} else {    // parent 
		close(Gsequence.cgi_output[1]);
		close(Gsequence.cgi_input[0]);
	}


	DBG_MIN("Sequence %s started", name) ;
	// Is running
	Gdata.sequence_status = SEQ_WAITCOMMAND ;

	// Now start thread for get sequence command
/*	list_cmd = fopen("list_command.txt", "a") ;         // configuration file
	if (!list_cmd)
		printf("\nerror create list_command.txt file\n") ;
*/
	// create sequence IN/OUT thread
	retcode = pthread_create(&Gsequence.thr_p, NULL, ThreadProc, NULL) ;
	if (retcode != 0) {
		DBG_MIN("UTIL: thread error (%d)", retcode) ;
		return(1) ;
	}

	return(0) ;
}

// Terminate sequence
void Stop_sequence(void) {
int status ;

	DBG_MIN("Closing sequence") ;
	if  (Gsequence.pid != (pid_t)(0)) {        // close needed
		if (kill(Gsequence.pid, SIGTERM)) {      // or SIGINT
			DBG_MIN("Kill error") ;
		} else {
			SLEEP(1000) ;
		}
	}
	close(Gsequence.cgi_output[0]) ;
	close(Gsequence.cgi_input[1]) ;
	waitpid(Gsequence.pid, &status, 0) ;

  DBG_MIN("Sequence terminated OK") ;
}

void Send_sequence_answer(char *answer) {
	if (Check_sequence()) return;
	
	if (write(Gsequence.cgi_input[1], answer, strlen(answer))<0){ // Answer to sequence
		Gdata.sequence_status = SEQ_ENDED ;
		return;	// Pipe was closed (normal exit path).
	}
}


// Start command
// 0 = Path, 1 = program, 2.. = args
int Start_command(int argc, char *argv[]) {
char name[SIZE_BUFFS];
int retcode;

	retcode = strlen(argv[1]) ; 
	sprintf(name, "%s//%s", argv[0], argv[1] ) ;
	
	for(retcode=2;retcode<argc;retcode++) {
		strcat(name, " ") ;
		strcat(name, argv[retcode]) ;
	  }
	
	rem_duble_slash(name,name);
	
	DBG_MIN("Starting %s", name );

	retcode = 0 ;
	if (system(name)==-1){
		retcode = errno ;
    DBG_MIN("AA Sequence exe failed (%d)", errno );
  	}
	return(retcode);
}

// *********************************************************************
// End Function for SEQUENCE
// *********************************************************************

// *********************************************************************
//static char lstr_time[100] ;

char* TimeDB(const time_t* loctime, char *aa )
{
// int ii ;
// char *cc ;
// 	
// 	cc = ctime_r(loctime, aa) ;
// 	ii = strlen(cc);
// 	cc[ii-1]='\0';
// 	return(	cc ) ;
struct tm *act;
time_t lltime ;

	lltime = *( (unsigned int*) (&(loctime))) ;
	

	act = gmtime(&lltime) ;
	sprintf(aa, "%02d/%02d/%04d %02d:%02d:%02d", 
				act->tm_mday, act->tm_mon+1, act->tm_year+1900,
				act->tm_hour, act->tm_min, act->tm_sec ) ;
	return(aa) ;
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
	//printf("\nnew=<%s>\n",new);
	//printf("\nlenini=<%d> e i=<%d>",lenini,i);
	if ((lenini-i) !=0) {
		for(i=i;i<(lenini-i);i++) new[i]='\0';
	}
	strcpy(stringd,new);
	//printf("\nstringd=<%s>\n",stringd);
	//sleep(1000000);
	return(stringd) ;	
}


