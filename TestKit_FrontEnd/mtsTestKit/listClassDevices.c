//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

#include <gtk/gtk.h>
#include <unistd.h>
#include <string.h>

#include "mtsTestKit.h"
#include "mtsGTK.h"


#define QUOTE   34      // char: "

// Open MtsTestKit.ini and get all [DISPOSITIVI]
// Type: -1 retrieve old value, 0 =Program, 1 = Collaudo
void CreateListClassDevices(int type) {
  GtkTreeIter iter;
  GtkTreeSelection *selection = gtk_tree_view_get_selection(Scrn.device_treeview) ;
  GtkTreeModel *fstore=gtk_tree_view_get_model(GTK_TREE_VIEW(Scrn.device_treeview));

  FILE *fin ;
  char cnfbuf[200],fileini[MAX_STRING],lastsel[MAX_STRING],lastsel2[MAX_STRING]; // , *p ;
  int i, cnt , valid_data, ctype,lastsellen,lastsellen2;

		if (!flock(0)) {
			ctype = type ;
			strcpy(fileini, MYNAME".ini");
			fin = fopen(fileini, "r") ;         // configuration file
			if (!fin){                          // really exists ?
				printf("File '%s' non trovato\n", fileini) ;
				flock(1);
				return ;
			  }
	
			valid_data = 0 ;
			cnt = 0 ;
			gtk_list_store_clear(GTK_LIST_STORE (Scrn.device_listmodel) ) ;
	
			while(fgets(cnfbuf, 200, fin)) {        // get lines
				if (strlen(cnfbuf)){
					if (cnfbuf[0]=='['){
						if (!strncasecmp(cnfbuf,"[GENERAL]", 9 ) ) {
							valid_data = 2;
						}else{
							if (!strncasecmp(cnfbuf,"[DISPOSITIVI]", 13 ) ) {
								valid_data = 1 ;
								if (type==-1) type = 1 ;
							  } else { valid_data = 0;	}
						}
					}else{
						if (valid_data) {
							if (cnfbuf[0]!=';') {
								if (cnfbuf[0]!=10) { 
									if (cnfbuf[0]!=13) {
										for(i=strlen(cnfbuf);i;i--){
											if (cnfbuf[i]<32)  cnfbuf[i] = '\0' ;
											else break ;
										}
										if (strlen(cnfbuf)){
											switch(type){
											case -1: // Get old selection
												strcpy(lastsel,"LastSelection_");
												//strcat(lastsel,Gdata.localIP);
												strcat(lastsel,Gdata.hostname);
												strcat(lastsel,"=");
												lastsellen=strlen(lastsel);
												if (!strncasecmp(cnfbuf,lastsel,lastsellen)){
													strcpy(Gdata.deviceClass, &cnfbuf[lastsellen]) ;
													printf("\nDeviceClass=<%s>\n",Gdata.deviceClass);
													valid_data = 1 ;
													i = strlen(Gdata.deviceClass) ;
													type = (strcasecmp(&Gdata.deviceClass[i-7], "Program"))? 1:0 ;
													printf("LS2: old sel <%d, %d, %s>\n", i, type, Gdata.deviceClass) ;
												}
				  
												if (type) { // Change default button
													gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Scrn.rbutCollaudo), TRUE) ;
												  }
                        else{
													gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Scrn.rbutProgram),TRUE) ;
												  }
												// Terzista
												if (!strncasecmp(cnfbuf,"Terzista=",9)){ strcpy(Gdata.Terzista, &cnfbuf[9]); }
												// usb_dir
												if (!strncasecmp(cnfbuf,"usb_dir=",8)) { strcpy(Gdata.usb_dir, &cnfbuf[8]); }
												// tk_portname
												if (!strncasecmp(cnfbuf,"tk_portname=",12)) { strcpy(Gdata.tk_portname, &cnfbuf[12]); }
												// usb_dir
												if (!strncasecmp(cnfbuf,"mts_portname=",13)) { strcpy(Gdata.mts_portname, &cnfbuf[13]); }

											break ;
											//case  0: // Program
											//case  1: // Collaudo
											default :
												strcpy(lastsel2,"LastSelection");
												lastsellen2=strlen(lastsel2);
												if (!strncasecmp(cnfbuf,lastsel2,lastsellen2)) break;
												if (valid_data==1) {
													i = strlen(cnfbuf);
													i = (strcasecmp(&cnfbuf[i-7], "Program"))? 1:0 ;
													if(type==i){
														//printf("LS1: <%s>\n", cnfbuf) ;
														gtk_list_store_append (GTK_LIST_STORE (Scrn.device_listmodel), &iter);
														gtk_list_store_set (GTK_LIST_STORE (Scrn.device_listmodel), &iter, 0, cnfbuf, -1 ) ;
														if (!strcmp(cnfbuf,Gdata.deviceClass)){
															gtk_tree_selection_select_iter(selection, &iter ) ;
															GtkTreePath *treepath = gtk_tree_model_get_path (fstore,&iter);
															gtk_tree_view_set_cursor (GTK_TREE_VIEW(Scrn.device_treeview), treepath,NULL, 0);
															gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW(Scrn.device_treeview),treepath, 0, FALSE, 0, 0);
															gtk_tree_path_free(treepath);
															//Focus
															gtk_widget_grab_focus(Scrn.ib_ok) ;
															//gtk_tree_view_scroll_to_cell(Scrn.device_treeview,path,NULL,0,0.0,0.0);
														}
														cnt++ ;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			fclose(fin) ;
			flock(1);
	
		// 	gtk_list_store_append (GTK_LIST_STORE (Scrn.device_listmodel), &iter);
		// 	foo.product = g_strdup("4004 Collaudo" ) ; // cnfbuf);
		// 	gtk_list_store_set (GTK_LIST_STORE (Scrn.device_listmodel), &iter, 0, foo.product, -1 ) ;
		// 	gtk_list_store_append (GTK_LIST_STORE (Scrn.device_listmodel), &iter);
		// 	foo.product = g_strdup("3008 Collaudo" ) ; // cnfbuf);
		// 	gtk_list_store_set (GTK_LIST_STORE (Scrn.device_listmodel), &iter, 0, foo.product, -1 ) ;
			if (ctype<0) Gdata.deviceClass[0]='\0' ;
		}
}

int GetSelData(void) {
  FILE *fin ;
  char cnfbuf[200], *p, *pp, fileini[MAX_STRING],nextip[MAX_STRING];
  int i, valid_data, parn ;

  strcpy(nextip,"next_");
  strcat(nextip,Gdata.hostname);
		
if (!flock(0)) {
		strcpy(fileini,MYNAME".ini");
		fin = fopen(fileini, "r") ;         // configuration file
		if (!fin){                          // really exists ?
			printf("File '%s' non trovato\n", fileini) ;
			flock(1);
			return(1);
		}

	
		valid_data = 0 ;
	
		sprintf(free_msg, "[%s]", Gdata.deviceClass ) ;
		i = strlen(free_msg) ;
		while(fgets(cnfbuf, 200, fin)) {        // get lines
			if (strlen(cnfbuf)){
				if (cnfbuf[0]=='['){
					if (!valid_data){
						if (!strncasecmp(cnfbuf,free_msg, i ) ) valid_data = 1 ;
					}else valid_data = 0 ;
				}else{
					if ((valid_data) && (cnfbuf[0]!=';') && (cnfbuf[0]!=10)&& (cnfbuf[0]!=13)){
						for(i=strlen(cnfbuf);i;i--){
							if (cnfbuf[i]<32)  cnfbuf[i] = '\0' ;
							else break ;
						}
						parn = 0 ;
						if (strlen(cnfbuf)){
							if (!strncasecmp(cnfbuf,"workingPath",11)) {
								parn = 1 ;
							}else if (!strncasecmp(cnfbuf,"MtsName",7)) {
								parn = 2 ;
							}else if (!strncasecmp(cnfbuf,"FileImpostazioni",16)) {
								parn = 3 ;
							}else if (!strncasecmp(cnfbuf,"prgFileRadix",12)) {
								parn = 4 ;
							}else if (!strncasecmp(cnfbuf,"ProgramFile",11)) {
								parn = 5 ;
							}else if (!strncasecmp(cnfbuf,nextip,strlen(nextip))) {
								parn = 6 ;
							}else if (!strncasecmp(cnfbuf,"Protocol",8)) {
								parn = 7 ;
							}
						
							if (parn){
								// Search '='
								p = strchr(cnfbuf, '=') ;
								if (p!=NULL){
									p++ ;
									while(*p == ' ' || *p == '\t') p++;
									// Check for quoted token
									if(*p == QUOTE){
										p++;         // Suppress quote
										pp = strchr(p, QUOTE) ;
										if (pp) *pp = '\0' ;
									}
									switch(parn){
										case 1:
										strcpy(Gdata.workingPath, p) ;
										break ;
									
										case 2:
										strcpy(Gdata.MtsName, p) ;
										break ;
									
										case 3:
										strcpy(Gdata.FileImpostazioni, p) ;
										break ;
									
										case 4:
										strcpy(Gdata.prgFileRadix, p) ;
										break ;
									
										case 5:
										strcpy(Gdata.ProgramFile, p) ;
										break ;
									
										case 6:
										strcpy(Gdata.sn_next, p) ;
										break ;

										case 7:
										strcpy(Gdata.Protocol, p) ;
										if (strcasecmp(Gdata.Protocol,"test")==0) 
											Gdata.pkt_offset = 23 ;
										else 
											Gdata.pkt_offset = 0 ;
										break ;
									}
								}
							}
						}
					}
				}
			}
		}
		fclose(fin) ;
		flock(1);

		// Debug info
		printf("SELECTED [%s]\n", Gdata.deviceClass ) ;
		printf("workingPath=%s\n",Gdata.workingPath) ;
		printf("MtsName=%s\n",Gdata.MtsName) ;
		printf("FileImpostazioni=%s\n",Gdata.FileImpostazioni) ;
		printf("prgFileRadix=%s\n",Gdata.prgFileRadix) ;
		printf("ProgramFile=%s\n",Gdata.ProgramFile) ;
	}
	return(0);
}


void class_program( GtkWidget * button, gpointer  data ){
//GtkWidget *pInfo;
//GtkWidget *pWindow;
//GSList *pList;
const gchar *sLabel;

	sLabel = gtk_button_get_label(GTK_BUTTON(button));	
	printf ("changed class: <%s>\n", sLabel);
	CreateListClassDevices(0) ;
}

void class_taverniti( GtkWidget * button, gpointer  data ){
  const gchar *sLabel;

	sLabel = gtk_button_get_label(GTK_BUTTON(button));
	printf ("changed class: <%s>\n", sLabel);
	CreateListClassDevices(1) ;
}

gboolean close_selection( GtkWidget * aa, gpointer  data ) {
	printf("close_selection\n") ;
	gtk_widget_hide(Scrn.device_sel) ;
	//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),FALSE) ;
	gtk_widget_set_sensitive(Scrn.main,TRUE) ;
		
	// If start end program
	if (strlen(Gdata.deviceClass)==0){
		Gdata.run_loop = MAIN_END ;
		printf("End prog\n") ;
	}
	StatusIcon(FALSE);
	return(TRUE) ;
}

void set_selection( GtkWidget * aa, gpointer  data ) {
GtkTreeIter iter;
GValue selection_item = {0,};
const char *str_sel ;
char lastsel[MAX_STRING];
GtkTreeSelection *selection = gtk_tree_view_get_selection(Scrn.device_treeview) ;
	
	if (gtk_tree_selection_get_selected (selection, NULL, &iter)){
		gtk_tree_model_get_value(Scrn.device_listmodel, &iter, 0, &selection_item) ;
		
		str_sel=g_value_get_string(&selection_item);			
		if (strncasecmp(Gdata.deviceClass,str_sel,strlen(str_sel))) {
			strcpy(Gdata.deviceClass, str_sel) ;
			strcpy(lastsel,"LastSelection_");
			//strcat(lastsel,Gdata.localIP);
			strcat(lastsel,Gdata.hostname);
			printf("\n%s=%s\n",lastsel,Gdata.deviceClass);
			setinival("general",lastsel,Gdata.deviceClass);
			//printf("\nScritto deviceClass\n");
		}
		// Get data from 'MtsTestKit.ini'
		//GetSelData() ;
// 	}else{
// 		Gdata.deviceClass[0]='\0' ;
	}
	sprintf(free_msg , "%s%s", Gdata.deviceClass,  TEXT_BOX) ;
	gtk_label_set_text( GTK_LABEL(Scrn.lbl_run), free_msg) ;
	printf ("closed selection: <%s>\n", Gdata.deviceClass);

	gtk_widget_hide(Scrn.device_sel) ;
	//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),FALSE) ;
	gtk_widget_set_sensitive(Scrn.main,TRUE) ;
	//close_selection(aa, data) ;

	// Disable button
	//_FR_
	//gtk_widget_set_sensitive(Scrn.cmd_script, FALSE) ;
	//gtk_widget_set_sensitive(Scrn.cmd_selection, FALSE) ;
	Gdata.menu_choice = 3 ;
}

