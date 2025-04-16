//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//


/*
 * Compile me with:
 *   gcc -o $(1) ($1).c $(pkg-config --cflags --libs gtk+-2.0 gmodule-2.0)
 */

//#include <glib.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
//for gdk version > 2.22
//#include <gdk/gdkgc.h>
//#include <gdk/gdk.h>

#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <dirent.h>

#include "MtsTestKit.h" 
#include "MtsGTK.h"

#include "video_def.h"
#include "logotest.h"

#include <sys/timeb.h>
#include <sys/stat.h>
#include <fcntl.h>

//Per Rete
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
//Per System
#include <stdlib.h>

#define NO_BLOCKING

#define QUOTE   34      // char: "

//#ifdef NO_BLOCKING
//short run_loop ;
//#endif

//########################################################################################################
//##############################Aggiunto installazione automatica#########################################
//##																									##
//##  To get FTDI device read link into '/dev/serial/by-id'												##
//##  To enable TestKit serial insert into  /etc/modules  ('sudo gedit /etc/modules')					##
//##  the follow line 'usbserial'																		##
//##  And add into '/etc/modprobe.d/options' the line 'options usbserial vendor=0x03eb product=0x6124'  ##
//##																								    ##
//########################################################################################################
//########################################################################################################


GtkBuilder *builder;

GtkStatusIcon *trayIcon = NULL;

char free_msg[MAX_STRING2] ;
GtkWidget  *aaa ;

int timestart=0;
int timescore=0;
int timewait=0;
int timetest=0;
struct timeb ltime;
int timewaitstart=0;
int timewaitstop=0;
int timeteststart=0;
int timetestold=0;

#define D_RADIUS 14 // 3
#define OFF_CENTER 5

struct _SCRN  Scrn ;
struct _TKGDATA Gdata ;
struct _AMPDATA Val_amp ;

struct _TKVALS ExtData ;
struct _MTSVALS MtsData ;
struct _SEQUENCE Gsequence ;


void choice_click( GtkWidget *buttonitem, gpointer  data ) ;
void PopupDestroy(GtkWidget * aa, gpointer  data ) ;
int RefreshUSB(void);
int get_my_IP(void);

#ifdef FR_WIN32
#include <wingdi.h>
void set_app_font (const char *fontname) ;
static void try_to_get_windows_font (void) ;
#endif

gboolean confirm_inputbox( GtkWidget * aa, gpointer  data )
{
const gchar *twidget;
	
	twidget = gtk_entry_get_text(GTK_ENTRY(Scrn.ib_val)) ;
	if (strlen(twidget))
		strcpy(Gdata.ib_data, twidget ) ;
	else
		strcpy(Gdata.ib_data, "##" ) ;
			   
	gtk_widget_hide(Scrn.form_inputbox) ;
	StatusIcon(FALSE);
	//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),FALSE) ;
	gtk_widget_set_sensitive(Scrn.main,TRUE) ;
	
#ifdef USE_MONITORING_
	printf("close_form Gdata.ib_data=>%s<\n", Gdata.ib_data) ;
#endif // USE_MONITORING

// 	Gdata.old_menu_choice = MNU_NOTHING ;
// 	Gdata.run_loop = MAIN_RUN ;
// 	old_diag=-1 ;
	ftime(&ltime);
	timewaitstop=ltime.time;
	timeteststart=timewaitstop;
	curtest=1;
	return(TRUE) ;
}

gboolean close_inputbox( GtkWidget * aa, gpointer  data )
{
#ifdef USE_MONITORING_
	printf("close_form\n") ;
#endif // USE_MONITORING

	gtk_widget_hide(Scrn.form_inputbox) ;
	StatusIcon(FALSE);
	//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),FALSE) ;
	gtk_widget_set_sensitive(Scrn.main,TRUE) ;
	
	strcpy(Gdata.ib_data, "#!" ) ;
#ifdef USE_MONITORING_
	printf("close_form Gdata.ib_data=>%s<\n",Gdata.ib_data) ;
#endif // USE_MONITORING
// 	Gdata.old_menu_choice = MNU_NOTHING ;
// 	Gdata.run_loop = MAIN_RUN ;
// 	old_diag=-1 ;
	ftime(&ltime);
	timewaitstop=ltime.time;
	timeteststart=timewaitstop;
	curtest=1;
	return(TRUE) ;
}

static gboolean key_press_inputbox( GtkWidget * aa, GdkEventKey *event, gpointer user_data) 
{

#ifdef USE_MONITORING_
	printf("!!INPUT BOX TASTO <%d>!!\n", event->keyval ) ;
#endif // USE_MONITORING
	if ((event->keyval==GDK_Return) || (event->keyval==GDK_KP_Enter) )
		confirm_inputbox(aa, user_data ) ;
	else if(event->keyval==GDK_Escape)
		close_inputbox(aa, user_data ) ;

	return(FALSE) ;

}



void init_Struct(void)
{
	CLEAR_MEM(&Scrn, sizeof(struct _SCRN) ) ;
	CLEAR_MEM(&Gdata, sizeof(struct _TKGDATA) ) ;
	CLEAR_MEM(&Val_amp, sizeof(struct _AMPDATA) ) ;
	CLEAR_MEM(&ExtData, sizeof(struct _TKVALS) ) ;
	CLEAR_MEM(&MtsData, sizeof(struct _MTSVALS) ) ;
	CLEAR_MEM(&Gsequence, sizeof(struct _SEQUENCE) ) ;
}

void get_widget( GtkWidget ** gg, char *name )
{
	*gg =  GTK_WIDGET(gtk_builder_get_object( builder, name ) );
#ifdef USE_MONITORING
	if (*gg==NULL) printf (free_msg, name );
#endif // USE_MONITORING
}

void populate_Scrn(void)
{
	int i ;
	char buff[50] ;

	strcpy(free_msg, "Widget '%s' not founded\n") ;

	get_widget(&Scrn.device_sel, "ListClassDevices") ;
	g_signal_connect (Scrn.device_sel, "delete_event", G_CALLBACK (close_selection), NULL);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.device_sel),TRUE) ;
	gtk_window_set_transient_for(GTK_WINDOW(Scrn.device_sel),GTK_WINDOW(Scrn.main)) ;

	Scrn.device_treeview = GTK_TREE_VIEW(gtk_builder_get_object( builder, "treeview" ) );
#ifdef USE_MONITORING
	if (Scrn.device_treeview==NULL) printf (free_msg, "treeview");
#endif // USE_MONITORING


	Scrn.device_listmodel = GTK_TREE_MODEL(gtk_builder_get_object( builder,"liststore1")) ;
#ifdef USE_MONITORING
	if (Scrn.device_listmodel==NULL) printf (free_msg, "liststore1");
#endif // USE_MONITORING


	get_widget(&Scrn.rbutProgram, "radiobutton1" );
	get_widget(&Scrn.rbutCollaudo, "radiobutton2" );
	get_widget(&Scrn.lbl_header, "LabelHeader" );
	get_widget(&Scrn.lbl_run, "LabelRun" );
	get_widget(&Scrn.bklbl_run, "BkLabelRun" );
	get_widget(&Scrn.progress_bar, "ProgressBar" );
	get_widget(&Scrn.lbl_amp, "LabelAmpere" );
	get_widget(&Scrn.lbl_volt, "LabelVolt" );
	get_widget(&Scrn.txt_amp, "TxtAmpere" );
	get_widget(&aaa, "bk_TxtAmpere" );
	//gtk_widget_modify_bg( aaa, GTK_STATE_NORMAL, &vb_color[VB_WHITE] ) ;
	get_widget(&Scrn.txt_volt, "TxtVolt" );
	get_widget(&aaa, "bk_TxtVolt" );
	//gtk_widget_modify_bg( aaa, GTK_STATE_NORMAL, &vb_color[VB_WHITE] ) ;

	for(i=0;i<8;i++){
		sprintf(buff,"FreeLabel%d", i ) ;
		get_widget(&Scrn.lbl_free[i], buff );
		
		sprintf(buff,"FreeText%d", i ) ;
		get_widget(&Scrn.txt_free[i], buff );

		sprintf(buff ,"bk_FreeText%d", i ) ;
		get_widget(&Scrn.bktxt_free[i], buff );
		//gtk_widget_modify_bg( Scrn.bktxt_free[i], GTK_STATE_NORMAL, &vb_color[VB_WHITE] ) ;
	}
	
	get_widget(&Scrn.boxtxt_mts, "textMTS" );
	get_widget(&Scrn.boxtxt_answer, "textAnswer" );
	get_widget(&Scrn.cmd_script, "bt_script" );
	g_signal_connect (Scrn.cmd_script, "clicked", G_CALLBACK (choice_click), NULL);
	get_widget(&Scrn.cmd_selection, "bt_selection" );
	g_signal_connect (Scrn.cmd_selection, "clicked", G_CALLBACK (choice_click), NULL);
	get_widget(&Scrn.lbl_extname, "LabelExtName" );
	get_widget(&aaa, "bk_LabelExtName" );
	gtk_widget_modify_bg( aaa, GTK_STATE_NORMAL, &vb_color[VB_WHITE] ) ;
	get_widget(&Scrn.lbl_protname, "LabelProtName" );
	get_widget(&aaa, "bk_LabelProtName" );
	gtk_widget_modify_bg( aaa, GTK_STATE_NORMAL, &vb_color[VB_WHITE] ) ;

	// Amperometer
	get_widget(&Scrn.amperometer, "drawingarea1" );

	for(i=0;i<22;i++){
		sprintf(buff,"stepcolor%d", i ) ;
		get_widget(&Scrn.clr_step[i], buff );

		sprintf(buff,"steptext%d", i ) ;
		get_widget(&Scrn.txt_step[i], buff );
	}
	Val_amp.value = 0 ;
	Val_amp.old_value = Val_amp.value ;
	
	get_widget(&aaa, "button1" ); // Class Selection OK
	g_signal_connect (aaa, "clicked", G_CALLBACK (set_selection), NULL);

	
	{
 		GdkPixbuf *abc ;
 
		get_widget(&aaa, "image1" );
		abc = gdk_pixbuf_new_from_inline (-1, logo, FALSE, NULL) ;
		gtk_image_set_from_pixbuf(GTK_IMAGE(aaa), abc );
	}


// 	MessageBox widget
	get_widget(&Scrn.form_inputbox, "InputBox" );
	g_signal_connect (Scrn.form_inputbox, "delete_event", G_CALLBACK (close_inputbox), NULL);
	g_signal_connect(Scrn.form_inputbox, "key-release-event", G_CALLBACK (key_press_inputbox), NULL);
	
	get_widget(&Scrn.ib_lbl, "ib_lbl" ) ;
	get_widget(&Scrn.ib_txt, "ib_txt" ) ;
	get_widget(&Scrn.ib_val, "ib_val" ) ;
	
	get_widget(&Scrn.ib_ok, "ib_ok" ) ;
	g_signal_connect (Scrn.ib_ok, "clicked", G_CALLBACK (confirm_inputbox), NULL);
	
	get_widget(&Scrn.ib_abort, "ib_abort" ) ;
	g_signal_connect (Scrn.ib_abort, "clicked", G_CALLBACK (close_inputbox), NULL);
}


// Amperometer
void upd_amp(void)
{
GdkDrawable *w = Scrn.amperometer->window;
GtkStyle *style = Scrn.amperometer->style;
GtkAllocation *size = &Scrn.amperometer->allocation ;
GdkGC *locgc = gdk_gc_new(Scrn.amperometer->window);
PangoLayout *pl ;
PangoRectangle rect;
int centerX, centerY, radius, posX, posY ; 
double pvalue ;
char buf[15], *p ;


//return ; // _FR_
	// Write data into mA box & V box
	sprintf(buf, "%4d", (int) Val_amp.value ) ;
	gtk_label_set_text( GTK_LABEL(Scrn.txt_amp), buf) ;
	if (Gdata.TKTYPE==0) pvalue = ExtData.analogs[9] ;
	if (Gdata.TKTYPE==1) pvalue = ExtData.analogs[13] ;
	sprintf(buf, "%.1lf", (pvalue * 3. / 1023. * 1056. / 56. + 0.1) ) ;
	while ((p=strchr(buf,','))!=NULL) *p='.' ;
	gtk_label_set_text( GTK_LABEL(Scrn.txt_volt), buf) ;
	

	sprintf(buf, "%4d mA", (int) Val_amp.value ) ;
	pl = gtk_widget_create_pango_layout(Scrn.amperometer, buf ) ;
	pango_layout_get_pixel_extents(pl, &rect, NULL);
	
	// CLEAR TEXT
//	gdk_draw_rectangle(w, style->white_gc, TRUE, 2,(size->height-rect.height-6),rect.width+8, rect.height+4);
	gdk_draw_rectangle(w, style->white_gc, TRUE, 2, size->height-20, 50, 20);

	// Write text
	gdk_draw_layout(w, style->black_gc, 2,(size->height-rect.height-6), pl ) ; // locgc
	
	// Write index
 	centerX = ( size->width / 2) - OFF_CENTER ;  //'#G# imposta la coordinata orizzontale del centro del quadrante
 	centerY =  size->height - 4 ; 	//'#G# imposta la coordinata verticale del centro del quadrante
	
	radius = ((centerX < centerY)? centerX : centerY) - D_RADIUS ; // 3 ; // - Screen.TwipsPerPixelX '#G# imposta il raggio del quadrante
	radius -= ((radius/14)*2.) ;
	
	// Clear old valueclose_inputbox
	pvalue = Val_amp.old_value/Val_amp.max_value ;
	if (pvalue > 1. ) pvalue = 1.05 ;
	pvalue *= M_PI * (140.0/180.);
	pvalue += (M_PI/9.) ;
	//printf("pvalue2  %.3lf\n", pvalue) ;
	posX = cos(pvalue) * (radius-1) ; //'#G# calcola coordinata orizzontale della punta della lancetta relativa al quadrante
	posY = sin(pvalue) * (radius-1) ; // '#G# calcola coordinata verticale della punta della lancetta relativa al quadrante

	//printf("H %d h %d\n", posX, posY) ;
	gdk_draw_line(w, style->white_gc, 
							centerX-2, centerY-2,
							(centerX-2) - posX,
							centerY - posY ) ;
	
	// Draw new valueclose_inputbox
	pvalue = Val_amp.value/Val_amp.max_value ;
	if (pvalue > 1. ) pvalue = 1.05 ;
//	printf("%d pvalue1  %.3lf\n", Val_amp.value, pvalue) ;
	pvalue *= M_PI * (140.0/180.);
	pvalue += (M_PI/9.) ;
	//printf("pvalue2  %.3lf\n", pvalue) ;
	posX = cos(pvalue) * (radius-1) ; //'#G# calcola coordinata orizzontale della punta della lancetta relativa al quadrante
	posY = sin(pvalue) * (radius-1) ; // '#G# calcola coordinata verticale della punta della lancetta relativa al quadrante

	//printf("H %d h %d\n", posX, posY) ;
	gdk_draw_line(w, style->black_gc, 
							centerX-2, centerY-2,
							(centerX-2) - posX,
							centerY - posY ) ;
	
	// Draw center
 	gdk_draw_arc(w, style->black_gc, TRUE,
 				 (gint) (centerX-4), (gint) (centerY-4),
 				 4, 4, (0*64), (360*64) ) ;

	g_object_unref(pl) ;
	g_object_unref(locgc) ;

	Val_amp.old_value = Val_amp.value ;
	
}



gint init_amp( GtkWidget *widget, gpointer   data )
{
int i, iter, pross, mom, sizew, sizeh ; 
int centerX, centerY, radius, nradius ;
double x, y ;

GdkDrawable *w = Scrn.amperometer->window;
GtkStyle *style = Scrn.amperometer->style;
GtkAllocation *size = &Scrn.amperometer->allocation ;
GdkGC *locgc = gdk_gc_new(Scrn.amperometer->window);
GdkColor lcolors[4]; // Colors for the hands

PangoLayout *pl ;
PangoRectangle rect;
char buf[10] ;

	

// return(0) ; // _FR_

	/* Store the colors */
    gdk_color_parse("Red", &lcolors[0]); // for minute's hand
    gdk_color_parse("Black", &lcolors[1]);    
    gdk_color_parse("DarkGreen", &lcolors[3]); // for hour's hand
    gdk_color_parse("RoyalBlue", &lcolors[2]); // for second's hand
	

	Val_amp.out_scale = FALSE ;
	Val_amp.max_value = 1200 ;  // prima 800

	
	sizew = size->width ;
	sizeh = size->height - 4 ;

	// background
	gdk_draw_rectangle(w, style->white_gc, TRUE, 
					   0,0, size->width, size->height);
	

 	centerX = (sizew / 2) - OFF_CENTER ;  // imposta la coordinata orizzontale del centro del quadrante
 	centerY = sizeh ; 	// imposta la coordinata verticale del centro del quadrante
	
	radius = ((centerX < centerY)? centerX : centerY) - D_RADIUS ; // imposta il raggio del quadrante
	nradius = radius / 14 ;  // '#G# imposta lunghezza delle tacche

// 	strcpy(Val_amp.title, "mA" );
// 	pl = gtk_widget_create_pango_layout(Scrn.amperometer, Val_amp.title ) ;
// 	pango_layout_get_pixel_extents(pl, &rect, NULL);
// 	gdk_draw_layout(w, style->black_gc, (centerX- rect.width/2 + 1), (centerY- rect.height / 2 + 1), pl ) ; // locgc
// 	g_object_unref(pl) ;


 	gdk_gc_set_line_attributes (locgc, 2, GDK_LINE_SOLID, 0, 0);

 	gdk_draw_arc(w, locgc, FALSE, // style->black_gc, FALSE, 
 				 (centerX-radius), (sizeh-radius),
 				 radius*2, radius*2, (20*64), (140*64) ) ;
				 
	iter = 1986 ;
	pross = 0 ;

    gdk_colormap_alloc_color (gdk_colormap_get_system(), &lcolors[0], FALSE, TRUE); // Red 
    gdk_colormap_alloc_color (gdk_colormap_get_system(), &lcolors[4], FALSE, TRUE); // Black

// _FR_ no text font
	i = pango_font_description_get_size(style->font_desc) ;
	if (i>10000){
// 		printf("Font size %d %s \n", i, pango_font_description_get_family(style->font_desc) );
		pango_font_description_set_size(style->font_desc, 8500 ) ;
		//pango_font_description_set_stretch(style->font_desc, PANGO_STRETCH_EXTRA_CONDENSED) ;
//#ifdef USE_MONITORING
//		printf("FONT size %d\n", pango_font_description_get_size(style->font_desc) );
//#endif // USE_MONITORING
		gtk_widget_modify_font(Scrn.amperometer, style->font_desc);
	}
	//pango_font_description_set_weight(style->font_desc, PANGO_WEIGHT_THIN);
	//gtk_widget_modify_font(Scrn.amperometer, style->font_desc);


	mom = 0 ;	
	for(i=0;i<=100;i++){ //  '#G# indico il numero di elementi
		iter = iter + 14 ;
		x = cos((iter / 10.) * M_PI / 180.) ;
		y = sin((iter / 10.) * M_PI / 180.) ;
		
		if (iter == (2000 + pross)) {
			// disegna tacche rosse
			//Amperometro.DrawWidth = CenterX / 1600
			gdk_gc_set_line_attributes (locgc, 2, GDK_LINE_SOLID, 0, 0);
			gdk_gc_set_foreground (locgc, lcolors+0); /* Red */
			//gdk_gc_set_rgb_bg_color(locgc, &red_color) ;
			gdk_draw_line(w, locgc, 
									(gint) (centerX + x * radius), 
									(gint) (centerY + y * radius),
									(gint) (centerX + x * (radius - (nradius * 2))),
									(gint) (centerY + y * (radius - (nradius * 2))) ) ;
			pross = pross + 140 ;
			
			// Text
			gdk_gc_set_foreground (locgc, &vb_color[VB_BLACK] ) ;
			
// _FR_ no text font
// TEXT
 			sprintf(buf, "%d", mom );
 			pl = gtk_widget_create_pango_layout(Scrn.amperometer, buf ) ;
// 			
 			pango_layout_get_pixel_extents(pl, &rect, NULL);
// Internal text
// 			x =(gint) (centerX + x * (radius - (nradius * 3))) - rect.width/2 + 1;
// 			y = (gint) (centerY + y * (radius - (nradius * 3))) - rect.height / 2 + 1;
// External text
 			x =(gint) (centerX + x * (radius + D_RADIUS -2 )) - rect.width/2 ;
 			y = (gint) (centerY + y * (radius + D_RADIUS -2 )) - rect.height / 2 ;
 			gdk_draw_layout(w, locgc, (gint) x, (gint) y, pl ) ;
 			g_object_unref(pl) ;
			//Call TextOut(Amperometro.hdc, 0, 0, Str(mom), Len(Str(mom)))
			mom = mom + (Val_amp.max_value / 10) ;
		}else{
			// disegna le divisioni piccole
			//gdk_gc_set_line_attributes (locgc, , GDK_LINE_SOLID, 0, 0);
			//gdk_gc_set_foreground (locgc, lcolors+0); // Black
			
			gdk_draw_line(w, style->black_gc, 
									(gint) (centerX + x * radius), 
									(gint) (centerY + y * radius),
									(gint) (centerX + x * (radius - nradius )),
									(gint) (centerY + y * (radius - nradius )) ) ;
		}
	}

	//pango_font_description_free (font_desc);
	g_object_unref(locgc) ;

	// Refresh also value
	upd_amp() ;

	return TRUE;

}

void init_Scrn(void)
{
int i ;
//GtkStateType act_state ;
//const GdkColor color = { 0, 45535, 45535, 45535 };


	sprintf(free_msg , "%s - Ver.%d.%02d.%02d", BANNER, VER, SUBVER, REVISION ) ;
	gtk_label_set_text( GTK_LABEL(Scrn.lbl_header), free_msg) ;
	
	// For texts buffer
	{
		GtkTextBuffer *buffer;
		GtkTextIter iter_m, iter_a;
		
		// boxtxt_mts
		buffer = gtk_text_view_get_buffer ((GTK_TEXT_VIEW (Scrn.boxtxt_mts)));
		gtk_text_buffer_get_end_iter (buffer, &iter_m);
		gtk_text_buffer_create_mark (buffer, "end_m", &iter_m, FALSE);

		// boxtxt_answer
		buffer = gtk_text_view_get_buffer ((GTK_TEXT_VIEW (Scrn.boxtxt_answer)));
		gtk_text_buffer_get_end_iter (buffer, &iter_a);
		gtk_text_buffer_create_mark (buffer, "end_a", &iter_a, FALSE);
	}
//	act_state = gtk_widget_get_state(Scrn.lbl_header) ;
//	printf("State is %d\n", act_state) ;
	
	//gtk_widget_modify_bg( Scrn.lbl_header, GTK_STATE_NORMAL, &green_color) ;
	//gtk_widget_modify_base( Scrn.lbl_run, GTK_STATE_NORMAL, &grey_color) ;

	//gtk_widget_modify_base( Scrn.txt_amp, GTK_STATE_NORMAL, &color) ;
	

	for(i=0;i<10;i++){
		gtk_widget_modify_bg( Scrn.clr_step[i], GTK_STATE_NORMAL, &vb_color[i]) ;
			
		//gtk_label_set_text(GTK_LABEL(Scrn.txt_step[i]), "Test ingressi digitali") ;
		gtk_label_set_text(GTK_LABEL(Scrn.txt_step[i]), "") ;
	}
	
	CreateListClassDevices(-1);
	
    g_signal_connect (Scrn.rbutProgram, "toggled", G_CALLBACK (class_program), NULL);
    g_signal_connect (Scrn.rbutCollaudo, "toggled", G_CALLBACK (class_taverniti), NULL);
	
	//gtk_widget_modify_bg( aaa, GTK_STATE_NORMAL, &green_color ) ; // Colore del bordo
	
	  // appel des fonctions de réalisation et de rafraichissement
	g_signal_connect(G_OBJECT(Scrn.amperometer), "expose_event", G_CALLBACK(init_amp), NULL);
	//g_signal_connect(G_OBJECT(Scrn.amperometer), "map-event", G_CALLBACK(init_amp), NULL);

// 	// Amperometer
	//? gtk_widget_modify_bg( Scrn.amperometer, GTK_STATE_NORMAL, &white_color) ;
// 	{
// 		GdkDrawable *w = Scrn.amperometer->window;
// 		GtkStyle *style = Scrn.amperometer->style;
// 	
// 
// 		// background
// 		gdk_draw_line(w, style->fg_gc[0], 
// 					  0,0, Scrn.amperometer->allocation.width,Scrn.amperometer->allocation.height);
// 	}
}

void close_application( GtkWidget *widget,
                        gpointer   data )
{
#ifdef NO_BLOCKING
	//if (Scrn.popup) PopupDestroy(Scrn.popup,NULL);
	//flock(1);
	Gdata.run_loop = MAIN_END ; 
#ifdef USE_MONITORING
	printf("request end\n") ;
#endif // USE_MONITORING
#else
	gtk_main_quit ();
#endif

}

void choice_click( GtkWidget *buttonitem, gpointer  data )
{
	const gchar * label ;

	label = gtk_button_get_label( GTK_BUTTON(buttonitem)) ;
	//label = gtk_widget_get_name(buttonitem) ;
	printf ("%s\n", label );

	// Initialize display
	Reset_Screen();
	
	if(Gdata.sequence_status){
		Stop_sequence() ;
		Gdata.sequence_status = SEQ_ENDED;
	}
	
	// Scelto "Connect" // Gdata.deviceClass
	if (!strcmp(label, "Avvia Script")) { // "bt_script")) //"Avvia Script"))
		Gdata.menu_choice = 3 ;
	}else if (!strcmp(label, "Cambia Selezione")) {// "bt_selection")) //"Cambia Selezione"))
		/*
		if(Gdata.sequence_status){
			Stop_sequence() ;
			Gdata.sequence_status = SEQ_ENDED;
		}
		*/
		Gdata.menu_choice = 2 ;
	}else{
		Gdata.menu_choice = -1 ;
	}
}


void PopupDestroy(GtkWidget * aa, gpointer  data )
{
		gtk_widget_destroy(Scrn.popup) ;
		//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),FALSE) ;
		gtk_widget_set_sensitive(Scrn.main,TRUE) ;
		StatusIcon(FALSE);
		//Gdata.menu_choice = 0 ;
		Scrn.popup = NULL  ;
}

void Popup(char *title, char* message, GtkMessageType type, GtkMessageType buttons, int deletable)
{
GtkDialogFlags dialog;
GtkMessageType button;
	
button=GTK_BUTTONS_NONE;
	
	//if (ScrPopup(n.popup) PopupDestroy(Scrn.popup,NULL);
		
	gtk_widget_set_sensitive(Scrn.main,FALSE) ;

	if(deletable){
		dialog=GTK_DIALOG_DESTROY_WITH_PARENT;
	}else{
		dialog=0;
	}

	Scrn.popup = gtk_message_dialog_new (GTK_WINDOW (Scrn.popup),
		    dialog, //GTK_DIALOG_FLAGS,                            
			type, // GTK_MESSAGE_INFO,
			buttons, // GTK_BUTTONS_CLOSE,
			"%s", message) ;
	
	if(buttons!=button){
		/* Destroy the dialog when the user responds to it (e.g. clicks a button) */
		g_signal_connect_swapped (Scrn.popup, "response",
								G_CALLBACK (PopupDestroy),
								Scrn.popup);
	}
	//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),TRUE) ;
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.popup),TRUE) ; //FALSE se false si vede la barra in basso
	gtk_window_set_title(GTK_WINDOW(Scrn.popup), title) ;
	gtk_window_set_deletable(GTK_WINDOW(Scrn.popup),deletable) ;
	gtk_window_set_transient_for(GTK_WINDOW(Scrn.popup),GTK_WINDOW(Scrn.main)) ;
	gtk_window_set_urgency_hint(GTK_WINDOW(Scrn.popup),TRUE);
	gtk_widget_grab_focus(Scrn.popup) ;
	gtk_widget_show(Scrn.popup) ;
	StatusIcon(TRUE);  //TRUE ICONA DI STATO LAMPEGGIA 
}



// InputBoxPopup(
void InputBox(int msgbox, char * caption, char *title, int nkey, char *lkey1, char *lkey2)
{
	gtk_widget_set_sensitive(Scrn.main,FALSE) ;
	gtk_window_set_title(GTK_WINDOW(Scrn.form_inputbox), title) ;
	gtk_entry_set_text(GTK_ENTRY(Scrn.ib_val), "" ) ;
	
	if (msgbox){
		gtk_label_set_text( GTK_LABEL(Scrn.ib_lbl), caption) ;
		gtk_widget_hide(Scrn.ib_val) ;
		gtk_widget_hide(Scrn.ib_txt) ;
		gtk_widget_show(Scrn.ib_lbl) ;
		// Focus
		gtk_widget_set_can_focus (Scrn.ib_ok,TRUE);
		gtk_widget_set_can_focus (Scrn.ib_val,FALSE);
		gtk_widget_set_can_focus (Scrn.ib_txt,FALSE);
		gtk_widget_set_can_focus (Scrn.ib_lbl,FALSE);
		gtk_widget_grab_focus(Scrn.ib_ok) ;
	}else{
		gtk_label_set_text( GTK_LABEL(Scrn.ib_txt), caption) ;
		gtk_widget_hide(Scrn.ib_lbl) ;
		gtk_widget_show(Scrn.ib_txt) ;
		gtk_widget_show(Scrn.ib_val) ;
		// Focus
		gtk_widget_set_can_focus (Scrn.ib_val,TRUE);
		gtk_widget_set_can_focus (Scrn.ib_ok,FALSE);
		gtk_widget_set_can_focus (Scrn.ib_txt,FALSE);
		gtk_widget_set_can_focus (Scrn.ib_lbl,FALSE);
		gtk_widget_grab_focus(Scrn.ib_val) ;
	}
	

	gtk_button_set_label(GTK_BUTTON(Scrn.ib_ok), ((lkey1!=NULL)? lkey1:"OK") ) ;
	gtk_button_set_label(GTK_BUTTON(Scrn.ib_abort), ((lkey2!=NULL)? lkey2:"Abort") ) ;
	
	if (nkey==2)
		gtk_widget_show(Scrn.ib_abort) ;
	else
		gtk_widget_hide(Scrn.ib_abort) ;
		
	//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),TRUE) ;
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.form_inputbox),TRUE) ;  //FALSE
	gtk_window_set_transient_for(GTK_WINDOW(Scrn.form_inputbox),GTK_WINDOW(Scrn.main)) ;
	gtk_window_set_urgency_hint(GTK_WINDOW(Scrn.form_inputbox),TRUE);
	gtk_widget_grab_focus(Scrn.form_inputbox) ;
	Gdata.ib_data[0] = '\0' ;
	
	CenterForm(GTK_WINDOW(Scrn.form_inputbox)) ;
	gtk_widget_show(Scrn.form_inputbox);
	StatusIcon(TRUE);
	ftime(&ltime);
	timewaitstart=ltime.time;
	timetestold=timetest;
	curtest=0;

}

// For text buffer
//void Add_txt_mts(GtkTextBuffer *buffer, char * new_buf)
void Add_txt_mts(GtkWidget *wd, char * new_buf)
{
gint i ;
GtkTextBuffer *buffer ;
GtkTextIter iter, iter1 ;
GtkTextMark *mark ;
  
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (wd)) ;
	gtk_text_buffer_get_iter_at_offset (buffer, &iter, -1);
	gtk_text_buffer_insert (buffer, &iter, new_buf, -1);
	mark = gtk_text_buffer_get_mark (buffer, "end_m");
	gtk_text_view_scroll_mark_onscreen( GTK_TEXT_VIEW (wd), mark);
	
	// Control max line nr
	i = gtk_text_buffer_get_line_count(buffer)  ;
	if (i>MAX_TEXTLINE){
		gtk_text_buffer_get_iter_at_line( buffer, &iter, 0 ) ;
		gtk_text_buffer_get_iter_at_line( buffer, &iter1, (i-MAX_TEXTLINE) ) ;
		gtk_text_buffer_delete( buffer, &iter, &iter1 ) ;
	}
}

void Clear_txt(GtkWidget *wd)
{
GtkTextBuffer *buffer ;
GtkTextMark *mark ;
GtkTextIter start, end;
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (wd)) ;
	gtk_text_buffer_get_start_iter (buffer,&start);
 	gtk_text_buffer_get_end_iter (buffer,&end);
	gtk_text_buffer_delete (buffer,&start,&end);
	gtk_text_buffer_create_mark (buffer, "end_m", &start, FALSE);
	mark = gtk_text_buffer_get_mark (buffer, "end_m");
	gtk_text_view_scroll_mark_onscreen( GTK_TEXT_VIEW (wd), mark);
}

// For text buffer
void Add_txt_answer(GtkWidget *wd, char * new_buf)
{
gint i ;
GtkTextBuffer *buffer ;
GtkTextIter iter, iter1 ;
GtkTextMark *mark ;
  
  
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (wd)) ;
	gtk_text_buffer_get_iter_at_offset (buffer, &iter, -1);
	gtk_text_buffer_insert (buffer, &iter, new_buf, -1);
	mark = gtk_text_buffer_get_mark (buffer, "end_a");
	gtk_text_view_scroll_mark_onscreen( GTK_TEXT_VIEW(wd), mark);

	// Control max line nr
	i = gtk_text_buffer_get_line_count(buffer)  ;
	if (i>MAX_TEXTLINE){
		gtk_text_buffer_get_iter_at_line( buffer, &iter, 0 ) ;
		gtk_text_buffer_get_iter_at_line( buffer, &iter1, (i-MAX_TEXTLINE) ) ;
		gtk_text_buffer_delete( buffer, &iter, &iter1 ) ;
	}
}

#ifdef FR_WIN32
static char appfontname[128] = "tahoma 8"; /* fallback value */
#else
static char appfontname[128] = "Sans 10";
#endif

void set_app_font (const char *fontname)
{
    GtkSettings *settings;

    if (fontname != NULL && *fontname == 0) return;

    settings = gtk_settings_get_default();

    if (fontname == NULL) {
	g_object_set(G_OBJECT(settings), "gtk-font-name", appfontname, NULL);
    } else {
	GtkWidget *w;
	PangoFontDescription *pfd;
	PangoContext *pc;
	PangoFont *pfont;

	w = gtk_label_new(NULL);
	pfd = pango_font_description_from_string(fontname);
	pc = gtk_widget_get_pango_context(w);
	pfont = pango_context_load_font(pc, pfd);

	if (pfont != NULL) {
	    strcpy(appfontname, fontname);
	    g_object_set(G_OBJECT(settings), "gtk-font-name", appfontname, NULL);
	}

	gtk_widget_destroy(w);
	pango_font_description_free(pfd);
    }
}

#ifdef FR_WIN32
char *default_windows_menu_fontspec (void)
{
    gchar *fontspec = NULL;
    NONCLIENTMETRICS ncm;

    memset(&ncm, 0, sizeof ncm);
    ncm.cbSize = sizeof ncm;

    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0)) {
	HDC screen = GetDC(0);
	double y_scale = 72.0 / GetDeviceCaps(screen, LOGPIXELSY);
	int point_size = (int) (ncm.lfMenuFont.lfHeight * y_scale);

	if (point_size < 0) point_size = -point_size;
	fontspec = g_strdup_printf("%s %d", ncm.lfMenuFont.lfFaceName,
				   point_size);
	ReleaseDC(0, screen);
    }

    return fontspec;
}

static void try_to_get_windows_font (void)
{
    gchar *fontspec = default_windows_menu_fontspec();

    if (fontspec != NULL) {
		int match = 0;
		PangoFontDescription *pfd;
		PangoFont *pfont;
		PangoContext *pc;
		GtkWidget *w;

#ifdef USE_MONITORING
		printf("Default font is %s\n", fontspec) ;
#endif // USE_MONITORING
		pfd = pango_font_description_from_string(fontspec);

		w = gtk_label_new(NULL);
		pc = gtk_widget_get_pango_context(w);
		pfont = pango_context_load_font(pc, pfd);
		match = (pfont != NULL);

		pango_font_description_free(pfd);
		g_object_unref(G_OBJECT(pc));
		gtk_widget_destroy(w);

		if (match) set_app_font(fontspec);
		g_free(fontspec);
    }
}

#endif /* FR_WIN32 */

int main(int argc, char *argv[])
{
     MTS_current_PORT=0;
        int i,len;
        char free_msg[2*MAX_STRING];
	Gdata.GTK_START = 0 ; // GTK not START
	Gdata.RISP255 = 0; // No Risp per EXSM 255
	Gdata.okcansend = 0; // No Risp per canconf
	curtest=0;
	// Clear all data struct
	init_Struct();
	// Parse argv
	if (argc>1){
		//char *sp[5] ;
		char *line, *cp ; // Max sub pars
		int nsp,c;

		Gdata.upass[0]='\0';
		
		for (i=0;i<argc;i++){
			if (argv[i][0]=='-'){
				// Init pointer
				//for(nsp=0;nsp<5;nsp++) sp[nsp] = NULL ;
				
				strcpy( free_msg, &argv[i][2] ) ;	// Skip '-?'
				line = free_msg ;
				for(nsp = 0;nsp < 5;){
					//sp[nsp] = line ;
					if (*line=='\0') break ;
					//sp[nsp++] = line ;
					if((cp = strchr(line,':')) == NULL) break ;
					*cp++ = '\0';
					line = cp;
				}
			
				switch(argv[i][1]){
					case 'h':	// Set working path
					case 'H':	// "-H<path>         define start path"
					strcpy(Gdata.lpath, &argv[i][2]) ;
					len=strlen(Gdata.lpath) ;
#ifdef FR_LINUX
					if (Gdata.lpath[len-1]!=47) 
					//sprintf(Gdata.lpath,"%s/",Gdata.lpath) ;  // 47='/' //
					strcat(Gdata.lpath,"/");
					
#endif
#ifdef FR_WIN32
					if (Gdata.lpath[len-1]!=92) sprintf(Gdata.lpath,"%s%c",Gdata.lpath,92) ; // 92='\' //
#endif
#ifdef USE_MONITORING						
					printf("\nCurrent dir <%s>\n", Gdata.lpath) ;
#endif // USE_MONITORING
					c=setinival("general","WorkSpace",Gdata.lpath);
#ifdef USE_MONITORING						
					printf("\nCurrent dir <%s>\n", Gdata.lpath) ;
#endif // USE_MONITORING
					if (c){
						Gdata.menu_choice = 1 ;	
						sprintf(free_msg,"Path '%s' non trovata\n", Gdata.lpath) ;
						printf("Path '%s' non trovata\n", Gdata.lpath);	
						Popup("INIZIALIZZAZIONE", free_msg, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,0) ;
						while (Gdata.menu_choice){
							while (gtk_events_pending ()){
									gtk_main_iteration ();
							}
						}
						return(0);
					}
					break ;

					case 'p':  //set Password User
					case 'P': // -P<password>
					strcpy(Gdata.upass, &argv[i][2]) ;
					//sprintf(Gdata.upass,"%s%c",Gdata.upass,'\0');
					#ifdef USE_MONITORING					
					printf("\nPassword User <%s>\n", Gdata.upass) ;
					#endif // USE_MONITORING
					break ;
				}
			}
		}
	}

#ifdef FR_LINUX
	char comm[3*MAX_STRING];
	sprintf(comm,"%sApplicativi/killMtsTestKit.sh %s",Gdata.lpath,Gdata.upass);
	printf("Kill All!,%s",comm);
	SLEEP(1000);
	system(comm);
#endif
		
	Gdata.pkt_offset = 0 ;
	// Get Local IP
	get_my_IP();
	// Get HostName
	gethostname(Gdata.hostname, sizeof Gdata.hostname);

	time_t t_old ;
	GError     *error = NULL;

	

	Gdata.run_loop = MAIN_RUN ;
	Gdata.sequence_status = SEQ_ENDED ;
	Gdata.menu_choice = 2;	
	Gdata.deviceClass[0]='\0' ;
	
	// Init GTK+
	gtk_disable_setlocale() ;
	gtk_init( &argc, &argv );

#ifdef FR_WIN32
	try_to_get_windows_font() ;
#endif
	// Create new GtkBuilder object 
	builder = gtk_builder_new();
	// Load UI from file. If error occurs, report it and quit application.
	//if( ! gtk_builder_add_from_file( builder, "mtstestkit.glade", &error ) )
	if( ! gtk_builder_add_from_string(builder, video_def, -1, &error ) ){
		g_warning( "%s", error->message );
		g_free( error );
		return( 1 );
	}

	// Get main window pointer from UI 
	strcpy(free_msg, "Widget '%s' not founded\n") ;
	get_widget(&Scrn.main, "window1" );

	
	// Connect signals 
	//gtk_builder_connect_signals( builder, NULL );
	g_signal_connect(Scrn.main, "destroy", G_CALLBACK (close_application), NULL);

	//Icona di Stato in alto
	StatusIcon(-1);
	
	//Se Applicazione socchiusa si riapre
	gtk_window_set_urgency_hint (GTK_WINDOW(Scrn.main),TRUE);
	gtk_window_set_auto_startup_notification (TRUE);

	

	// Get object reference
	populate_Scrn() ;

	// Initialize display
	init_Scrn() ;
	
	// Set taskbar //_GT
	gtk_widget_set_sensitive(Scrn.main,TRUE) ;
	//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),FALSE) ;		

	// Show window. All other widgets are automatically shown by GtkBuilder 
	gtk_widget_show(Scrn.main); 

	//for(i=0;i<100;i++) gtk_main_iteration ();

	//SLEEP(1000);
 
	//Destroy builder, since we don't need it anymore
    //g_object_unref( G_OBJECT( builder ) );

	Gdata.GTK_START = 1 ; // GTK is START
	
	i= Low_Init(0) ;
	if (i){
		if (i<2){
			Popup("INIZIALIZZAZIONE", "Attendere Ripristino porte USB", GTK_MESSAGE_INFO,GTK_BUTTONS_NONE,0) ;
			for(i=0;i<300;i++) gtk_main_iteration ();
			RefreshUSB();
			if (Gdata.refresh_USB) PopupDestroy(Scrn.popup,NULL);
			i=Low_Init(1);
			if (!i){
				if ((Scrn.popup)) {
					PopupDestroy(Scrn.popup,NULL);
				}
			}
		}
		if (i) Gdata.run_loop = MAIN_END ; //_FR_
	}
	if(!i){
		gtk_label_set_text( GTK_LABEL(Scrn.lbl_extname), Gdata.portdev[PORT_TK]) ;
                gtk_label_set_text( GTK_LABEL(Scrn.lbl_protname), Gdata.portdev[MTS_current_PORT]) ;
	}

#ifdef USE_MONITORING
	printf("init ok (%s),Gdata.run_loop=(%d),Gdata.menu_choice=(%d)\n", Gdata.lpath,Gdata.run_loop,Gdata.menu_choice) ;
#endif // USE_MONITORING
#ifdef NO_BLOCKING
	// Alternative use
	t_old = time(NULL) ;
	ftime(&ltime);
	timestart=ltime.time;
	int timescoresec=0;
	int timescoremin=0;
	int timescorehour=0;
	int timewaitsec=0;
	int timewaitmin=0;
	int timewaithour=0;
	int timetestsec=0;
	int timetestmin=0;
	int timetesthour=0;
	while (Gdata.run_loop==MAIN_RUN){
		ftime(&ltime);
		if (waithuman) timescore=ltime.time-timestart;
		timescorehour=(timescore/3600);
		timescoremin=(timescore/60)-(timescorehour*60);
		timescoresec=timescore-(timescoremin*60)-(timescorehour*3600);
		if ( curtest ) timetest=timetestold+(ltime.time-timeteststart);
		if (timewaitstop>timewaitstart) {
			timewait=timewait + (timewaitstop-timewaitstart);
			timewaitstart=timewaitstop;
		}
		timewaithour=(timewait/3600);
		timewaitmin=(timewait/60)-(timewaithour*60);
		timewaitsec=timewait-(timewaitmin*60)-(timewaithour*3600);
		timetesthour=(timetest/3600);
		timetestmin=(timetest/60)-(timetesthour*60);
		timetestsec=timetest-(timetestmin*60)-(timetesthour*3600);
		sprintf(free_msg , "%s - Ver.%d.%02d.%02d\t\tTIMETOTAL:%02d:%02d:%02d_TIMETEST:%02d:%02d:%02d_TIMEWAIT:%02d:%02d:%02d", BANNER, VER, SUBVER, REVISION,timescorehour,timescoremin,timescoresec,timetesthour,timetestmin,timetestsec,timewaithour,timewaitmin,timewaitsec) ;
		gtk_label_set_text( GTK_LABEL(Scrn.lbl_header), free_msg) ;
		// User input verify
		while ((gtk_events_pending ()) && (Gdata.run_loop==MAIN_RUN)){
			  gtk_main_iteration ();
		}
		
		 
		// If choosed a menu
		if (Gdata.menu_choice){
#ifdef USE_MONITORING
			printf ("choosed %d\n", Gdata.menu_choice);
#endif // USE_MONITORING
			
			switch(Gdata.menu_choice){
				case 1:
				//sprintf(free_msg, "Gdata.menu_choiceScelto menu %d", Gdata.menu_choice) ;
				//Popup(free_msg, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,0) ;
				Gdata.sequence_status = SEQ_WAITCOMMAND ;
				Gdata.menu_choice = 0 ;
				break ;
				
				case 2:
				gtk_widget_set_sensitive(Scrn.main,FALSE) ;
				//gtk_window_set_skip_taskbar_hint(GTK_WINDOW(Scrn.main),TRUE) ;
				gtk_widget_show( Scrn.device_sel );
				StatusIcon(TRUE);
				curtest=0;
				Gdata.menu_choice = 0 ;
				break ;

				case 3:
				StatusIcon(FALSE);
				if(!GetSelData()) Gdata.menu_choice=4 ;
				break;
					
				case 4: 	// If choosed a sequencea
				Start_sequence();
				curtest=1;
				waithuman=1;
				ftime(&ltime);
				timeteststart=ltime.time;
				Gdata.menu_choice = 0 ;
				break ;
					
				default:
				Gdata.menu_choice = 0 ;
				break ;
			}
			
		}
		
		// PORTs manage
		for(i=0;i<PORT_MAX;i++){
			if (Gdata.portopened[i]) {
				ports_tick(i) ;
			}
		}
		
		
		// If script  running
		switch(Gdata.sequence_status){
			case SEQ_WAITCOMMAND :		// Wait for command
				Get_sequence_command() ;
				break ;
			case SEQ_TOANSWER : 		// Wait for send answer, check if timeout expired
			case SEQ_USERANSWER:		// 
				Check_sequence_timeout() ;
				break ;
			default:
				break ;
		}
		
		
		SLEEP(10) ; // 0.01 sec
		if ( (Gdata.bar_msec) && (Gdata.bar_perc<1.0)) {
			struct timeb bar_newtime ;
			
			ftime(&bar_newtime);
			if (bar_newtime.millitm<Gdata.bar_oldmsec) bar_newtime.millitm+= 1000 ;
			
			if ((bar_newtime.millitm - Gdata.bar_oldmsec)>Gdata.bar_msec){
				if (Gdata.sequence_status!=SEQ_USERANSWER){
					Gdata.bar_perc += 0.01 ;
#ifdef USE_MONITORING_
	printf("\n!!Gdata.bar_perc=%f!!\n", Gdata.bar_perc) ;
#endif // USE_MONITORING
					if (Gdata.bar_perc>0.995) Gdata.bar_perc = 1.0 ;
					gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(Scrn.progress_bar), Gdata.bar_perc) ;
				}
				Gdata.bar_oldmsec = ((bar_newtime.millitm>1000) ? (bar_newtime.millitm-1000): bar_newtime.millitm) ;
			}
		}
		
		if (t_old!=time(NULL)){
			char lbuf[100] ;
			
			t_old=time(NULL) ;
			sprintf(lbuf,"Now: %lu (%d)\n", t_old, Gdata.run_loop ) ;
			// prova
			//Val_amp.value += 40 ;
			//if (Val_amp.value>(Val_amp.max_value*1.3)) Val_amp.value -= 753 ;
			//upd_amp() ;
#ifdef USE_MONITORING
			//printf("\nSequence status %d\n",Gdata.sequence_status) ;
			//printf("\n%s",lbuf) ; // _FR_
			//printf("\nF=%d\n", Gdata.run_loop ) ;
			//printf("\nB_tk=%d\n",gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW (Scrn.boxtxt_mts)))  ) ;
			//printf("\nB_mts=%d\n",gtk_text_buffer_get_line_count(gtk_text_view_get_buffer(GTK_TEXT_VIEW (Scrn.boxtxt_answer)))  ) ;
			//Add_txt_mts(Scrn.boxtxt_mts, lbuf) ;
#endif // USE_MONITORING
		}
	}
  // Alternative use
#else
	/* Start main loop */
	gtk_main ();
#endif

#ifdef USE_MONITORING
	printf("\nSequence status %d\n",Gdata.sequence_status) ;
#endif // USE_MONITORING
	// If sequence running -> stop 
	if(Gdata.sequence_status){
		curtest=0;
		Stop_sequence() ;
		Gdata.sequence_status = SEQ_ENDED;
	}
	
	/* Destroy builder, since we don't need it anymore */
    g_object_unref( G_OBJECT( builder ) );

	for(i=0;i<PORT_MAX;i++){
		if (Gdata.portopened[i]) {
#ifdef USE_MONITORING
			printf("\nclose com %d\n",i) ;
#endif // USE_MONITORING
			com_close(i) ;
#ifdef USE_MONITORING
			printf("\nclosed com %d\n",i) ;
#endif // USE_MONITORING
		}
	}

    return( 0 );
}

void ProgressBar(int type, int value)
{
struct timeb bar_newtime ;


	switch(type){
		case 0 :		// set value
		Gdata.bar_msec = 0  ; 
		if (value>100) value = 100 ;
		Gdata.bar_perc = ((float) value) / 100.0 ;
		gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(Scrn.progress_bar), Gdata.bar_perc ) ;
		break ;
		
		case 1:			// set total time
		ftime(&bar_newtime);
		Gdata.bar_oldmsec = bar_newtime.millitm ;
		Gdata.bar_perc = 0.0 ;
		gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR(Scrn.progress_bar), Gdata.bar_perc) ;
		Gdata.bar_msec = value ; 
		break ;
	}
		
}

void CenterForm(GtkWindow *form) 
{
int x, y, width, height ;

	gtk_window_get_position( GTK_WINDOW(Scrn.main), &x, &y ) ;
	gtk_window_get_size( GTK_WINDOW(Scrn.main), &width, &height ) ;
//printf("\nMAIN; x=%d, y=%d w=%d h=%d\n", x, y ,width, height ) ;
	x += (width/2) ;
	y += (height/2) ;
	gtk_window_get_size( form, &width, &height ) ;
	x -= (width/2) ;
	y -= (height/2) ;
	gtk_window_move( form, x, y ) ;

}

int setinival(char *bloc,char *par,char *con)
{
FILE *fini,*fnew ;
char cnfbuf[200],fileini[MAX_STRING],filenew[MAX_STRING],string[MAX_STRING],free_msg[MAX_STRING],st[0];
int valid_data,data_valid,i,l,lp,ls; // lc 

	if (!flock(0)) {
		lp=strlen(par);
		//lc=strlen(con);
		strcpy(fileini,Gdata.lpath);
		strcat(fileini,"MtsTestKit.ini");

		strcpy(filenew,Gdata.lpath);
		strcat(filenew,"MtsTestKit.new");

		fini = fopen(fileini, "r") ;         // open configuration file
		if (!fini){ 
		#ifdef USE_MONITORING
			printf("\nFile '%s' non trovato\n", fileini) ;
		#endif // USE_MONITORING
			flock(1);
			return(1);
		}

		fnew = fopen(filenew,"w") ;
		if (!fnew){
			#ifdef USE_MONITORING
			printf("\nFile '%s' non creato\n", filenew) ;
			#endif // USE_MONITORING
			fclose(fini);
			flock(1);
			return(1);
		}
		fclose(fnew);

		fnew = fopen(filenew, "a") ;		// create new file

		valid_data = 0 ;
		data_valid = 0 ;
		
		sprintf(free_msg, "[%s]", bloc ) ;
		i=strlen(free_msg);
		while(fgets(cnfbuf, 200, fini)) {// get lines
			l=strlen(cnfbuf);
			if (l){
				if (cnfbuf[0]=='['){					
					if (!valid_data) {
						if (!strncasecmp(cnfbuf,free_msg,i ) ) {
							valid_data = 1 ;
							data_valid = 1 ;
						}
					}else{
						valid_data = 0 ;
					}
					fwrite(&cnfbuf,l,1,fnew);
				}else{
					if ( valid_data ) {
						if ( !strncasecmp(cnfbuf,par,lp) ) {
							strcpy(string,par);
							strcat(string,"=");
							strcat(string,con);
							strcat(string,"\r\n");
							ls=strlen(string);
							fwrite(&string,ls,1,fnew);
							data_valid = 0 ;
						}else{
							if (!strncasecmp(cnfbuf,"\r\n",2)){
								if (data_valid){
									strcpy(string,par);
									strcat(string,"=");
									strcat(string,con);
									strcat(string,"\r\n");
									ls=strlen(string);
									fwrite(&string,ls,1,fnew);
									data_valid = 0 ;
								}	
							}
							fwrite(&cnfbuf,l,1,fnew);
						}					
					}else{
						fwrite(&cnfbuf,l,1,fnew);
					}					
				}
			}else{	
				st[0]='\0';	
				fwrite(&st,1,1,fnew);
			}
		}
		fclose(fini) ;
		fclose(fnew) ;

		unlink(fileini) ;
		SLEEP(2);
		rename(filenew,fileini); 
		SLEEP(2);
		flock(1);
	}
	return(0);	
}

char * getinival(char *stringd,char *bloc,char *par)
{
FILE *fini;
char cnfbuf[200], *p, *pp,fileini[MAX_STRING],free_msg[MAX_STRING];
int valid_data,i,l,lp;

	strcpy(stringd,"");

	p=NULL;

	if (!flock(0)) {
		lp=strlen(par);
		strcpy(fileini,Gdata.lpath);
		strcat(fileini,"MtsTestKit.ini");

		fini = fopen(fileini, "r") ;         // open configuration file
		if (!fini){ 
		#ifdef USE_MONITORING
			printf("\nFile '%s' non trovato\n", fileini) ;
		#endif // USE_MONITORING
			flock(1);
			return(p);
		}
		
		valid_data = 0 ;

		sprintf(free_msg, "[%s]", bloc ) ;
		i=strlen(free_msg);
		while(fgets(cnfbuf, 200, fini)) {// get lines
			if (p==NULL) {
				l=strlen(cnfbuf);
				if (l){
					if (cnfbuf[0]=='['){
						if (!valid_data) {
							if (!strncasecmp(cnfbuf,free_msg,i ) ) valid_data = 1 ;
						}else valid_data = 0 ;
					}else{
						if ( valid_data ) {
							if ( !strncasecmp(cnfbuf,par,lp) ) {
								p = strchr(cnfbuf, '=') ;
								if (p!=NULL){
									p++ ;
									while(*p == ' ' || *p == '\t') { 
										p++;
									}
									// Check for quoted token
									if(*p == QUOTE){
										p++;         // Suppress quote
										pp = strchr(p, QUOTE) ;
										if (pp) *pp = '\0' ;
									}
								}
								strcpy(stringd,p);
								valid_data = 0 ;
							}
						}
					}
				}
			}
		}	
		fclose(fini) ;
		flock(1);
	}
	return(stringd);
}

int RefreshUSB(void)
{
char *argv[3],applicativi[MAX_STRING],file[MAX_STRING];
int esito,i;
	
	Gdata.refresh_USB=0;
	strcpy(applicativi,Gdata.lpath);
	strcat(applicativi,"Applicativi/");
	
	strcpy(file,"ripristino_usb.sh");
	
	argv[0]=applicativi;
	argv[1]=file;
	i=2;
	printf("\nGdata.upass=<%s>\n",Gdata.upass) ;
	if (Gdata.upass[0]!='\0') {
		//printf("\nPassato da Gdata.upass\n") ;
		argv[2]=Gdata.upass;
		i=i+1;
	 }
	printf("\nStart_Command <%s%s %s>\n",argv[0],argv[1],argv[2]) ;
	if(Start_command(i,argv)!=0) {
		esito=1;	
	}else{
		esito=0;
	}
	Gdata.refresh_USB=1;
	return(esito);
}

int flock(int finito)
{
int r;
char filelock[MAX_STRING];
	
	// Cerca di lockare .ini.lck
	strcpy(filelock,Gdata.lpath);
	strcat(filelock,"MtsTestKit.lck");
	if (!finito) {
		Gdata.filock = tmpfile ();
		Gdata.filock = fopen(filelock,"wx");
		if (Gdata.filock!=NULL) {
			return(0);
		}else{
			srand(time(0)); /* n is random number in range of 0 - 1 */
			//sprintf(free_msg,"In attesa di accesso esclusivo al file MtsTestkit.ini",r);	
			//if (Gdata.GTK_START) Popup("ATTESA", free_msg, GTK_MESSAGE_INFO,GTK_BUTTONS_NONE,0) ;
			while (1) {
				Gdata.filock = fopen(filelock,"wx");
				if ( Gdata.filock!=NULL ) break;
				r=1000+(rand() % 100) ;  
				sprintf(free_msg,"In attesa di accesso esclusivo al file MtsTestkit.ini-Random:%d",r);
				printf("\n%s\n",free_msg);
				if (Gdata.GTK_START) Add_txt_mts(Scrn.boxtxt_mts, free_msg) ;
				SLEEP(r); // 0.001 sec
				if (Gdata.GTK_START) gtk_main_iteration ();
			}
			/*
			do{
				r=1000+(rand() % 100) ;  
				sprintf(free_msg,"In attesa di accesso esclusivo al file MtsTestkit.ini-Random:%d",r);
				printf("\n%s\n",free_msg);
				if (Gdata.GTK_START) Add_txt_mts(Scrn.boxtxt_mts, free_msg) ;
				SLEEP(r); // 0.001 sec
				if (Gdata.GTK_START) gtk_main_iteration ();
	 			Gdata.filock = fopen(filelock,"wx");
			}while(Gdata.filock==NULL);
			*/
			return(0);
		}
	}else{
		//if (Scrn.popup) {
		//	PopupDestroy(Scrn.popup,NULL);
		//}
		if (Gdata.filock!=NULL) {
			fclose(Gdata.filock) ;
			unlink(filelock) ;
		}
	}
	return(0);
}






 
int get_my_IP(void)
{
	//Returns the IP address of this host.
	//- if host have more than 1 IP, only 1 (the first) is returned.
	//- return is in network byte order
	//return: 1 if unsuccessful, the IP otherwise

	//have you ever seen a hostname longer than a screen (80cols)?
	
	

	char name[81]; // store my hostname
	struct hostent * hostent_ptr;
	//struct in_addr addr;
	int ret,i,end,valid_data;
	char iplocal[16];

	end=0;
	
	ret = gethostname (name, 80);
	
	if(ret == -1) return(1) ;
	
	hostent_ptr = gethostbyname(name);

	if (hostent_ptr == NULL) return(1);
	
	i=0;
	valid_data=1;
	/*  Non va sempre
	while (hostent_ptr->h_addr_list[i] != 0) {
		if (valid_data) {
			strcpy(iplocal,inet_ntoa(*((struct in_addr *)hostent_ptr->h_addr_list[i++])));
			if (!strncmp(iplocal, "127",3)) valid_data=0;
		}
    }*/
	strcpy(iplocal,inet_ntoa(*((struct in_addr *)hostent_ptr->h_addr_list[0])));
	//printf("\nIP Address :<%s>\n",iplocal);
	//printf("\nConfronto primi char con ip=<%d>",!strncmp(iplocal, "192",3));
	if (!strncmp(iplocal, "127",3)) {
		FILE *fip;
		char fileip[MAX_STRING2],cnfbuf[3000],*p;
		while(end==0){
			sprintf(fileip,"%s%siplist%d",Gdata.lpath,"Applicativi/",i++);
			//printf("\nfileip=<%s>",fileip);
			fip = fopen(fileip, "wx") ;
			if (fip){
				end=1;
				fclose(fip);
			}
		}	
		sprintf(cnfbuf,"ifconfig |grep inet: > %s",fileip); 
		system(cnfbuf);
		fip = fopen(fileip, "r") ;
		if (!fip){ 
			#ifdef USE_MONITORING
				printf("\nFile '%s' non trovato\n", fileip) ;
			#endif // USE_MONITORING
				return(1);
		}
		valid_data=1;
		while(fgets(cnfbuf, 200, fip)) { // get lines
			if (valid_data) {
				p = strchr(cnfbuf, ':') ;
				p++;
				strcpy(cnfbuf,p);
				p = strchr(cnfbuf, ' ') ;
				int lenmom= p-cnfbuf+1;
				//printf("\nSpazio Trovato a <%d>\n",lenmom);
				char mom[lenmom];
				strncpy(mom,cnfbuf,lenmom);
				mom[lenmom-1]='\0';
				mom[lenmom]='\0';
				strcpy(cnfbuf,mom);
				//printf("\n<%s>\n",cnfbuf);
				if (strncmp(cnfbuf, "127",3)) {
					strcpy(iplocal,cnfbuf);
					valid_data=0;
				}
			}
		}
		fclose(fip) ;
		unlink(fileip) ;
	}
	strcpy(Gdata.localIP,iplocal);
	//strcpy(Gdata.localIP,"192.168.100.240");
	printf("\nIP Address : <%s>\n",Gdata.localIP);
	//SLEEP(100000);
	return(0);
}

void StatusIcon(int blink)
{
char icona[MAX_STRING2];
	GError     *error = NULL;
	
	//GtkStatusIcon *trayIcon = NULL;
	//GtkStatusIcon *trayIcon = gtk_status_icon_new_from_file (icona);
	//trayIcon=gtk_status_icon_new_from_file (icona);
	
		
	switch(blink){
		//printf("\nBlink:<%d>\n",blink);
		case -1:
			sprintf(icona,"%sTestIco.png",Gdata.lpath);
			printf("\nStatusIcon=<%s>\n",icona);
			trayIcon = gtk_status_icon_new_from_file (icona);
			//gtk_status_icon_set_from_file(trayIcon,icona);
			gtk_status_icon_set_visible(trayIcon, TRUE);
			//Icona in taskbar e programma
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(icona, &error);
			gtk_window_set_icon(GTK_WINDOW(Scrn.main),pixbuf);
		break ;
		case 0:
			
			gtk_status_icon_set_blinking (trayIcon, FALSE);	
		break ;
		case 1:
			gtk_status_icon_set_blinking (trayIcon, TRUE);
		break ;

	}

}
void Reset_Screen(void)
{
	Clear_txt(Scrn.boxtxt_mts);
	Clear_txt(Scrn.boxtxt_answer);
	gtk_widget_modify_bg( Scrn.bklbl_run, GTK_STATE_NORMAL, &vb_color[VB_LGRAY]) ;
	gtk_widget_modify_fg( Scrn.lbl_run, GTK_STATE_NORMAL, &vb_color[VB_BLACK]) ;
	sprintf(free_msg , "%s%s", Gdata.deviceClass,  TEXT_BOX) ;
	gtk_label_set_text( GTK_LABEL(Scrn.lbl_run), free_msg) ;
	int i;
	for(i=0;i<8;i++){
		gtk_label_set_text(GTK_LABEL(Scrn.txt_free[i]), '\0') ;
		gtk_label_set_text(GTK_LABEL(Scrn.bktxt_free[i]), '\0') ;
		gtk_label_set_text(GTK_LABEL(Scrn.lbl_free[i]), '\0') ;
		gtk_widget_modify_bg( Scrn.bktxt_free[i], GTK_STATE_NORMAL, &vb_color[VB_LGRAY]) ;
		gtk_widget_modify_fg( Scrn.txt_free[i], GTK_STATE_NORMAL, &vb_color[VB_BLACK]) ;
	}
	for(i=0;i<22;i++){
		gtk_widget_modify_bg( Scrn.clr_step[i], GTK_STATE_NORMAL, &vb_color[VB_LGRAY]) ;
		gtk_label_set_text(GTK_LABEL(Scrn.txt_step[i]), '\0') ;
	}
	curtest=0;
	timewait=0;
	timescore=0;
	timetest=0;
	timetestold=0;
	ftime(&ltime);
	timestart=ltime.time;
	ProgressBar(0, 0) ;
}

