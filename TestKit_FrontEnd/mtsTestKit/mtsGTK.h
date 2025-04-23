//
//
//   Copyright (c) 1997-2010.
//   T.E.S.T. srl
//

#include <gdk/gdkkeysyms.h>

#define MAX_TEXTLINE 500		// Max line nr into 'boxtxt_mts' and 'boxtxt_answer'

#define VB_LGRAY	0
#define VB_RED  	1
#define VB_GREEN	2
#define VB_YELLOW	3
#define VB_WHITE	4
#define VB_BLACK	5
#define VB_BLUE		6
#define VB_CYAN		7
#define VB_MAGENTA	8
#define VB_GRAY		9


static const GdkColor vb_color[] = {{0, 0xeded, 0xecec, 0xebeb},	// VB_LGRAY
									{0, 0xffff, 0x0000, 0x0000},	// VB_RED
									{0, 0x4582, 0xe019, 0x2d0b},	// VB_GREEN
									{0, 0xffff, 0xffff, 0x0000},	// VB_YELLOW
									{0, 0xffff, 0xffff, 0xffff},	// VB_WHITE
									{0, 0x0000, 0x0000, 0x0000},	// VB_BLACK
									{0, 0x0000, 0x0000, 0xffff},	// VB_BLUE
									{0, 0x0000, 0xffff, 0xffff},	// VB_CYAN
									{0, 0xffff, 0x0000, 0xffff},	// VB_MAGENTA
									{0, 0x8080, 0x8080, 0x8080}};	// VB_GRAY


//static const GdkColor grey_color =  {0, 0xeded, 0xecec, 0xebeb} ;
//static const GdkColor green_color =  {0, 0x4582, 0xe019, 0x2d0b} ;
//static const GdkColor red_color =  {0, 0xffff, 0x0, 0x0} ;
//static const GdkColor white_color =  {0, 0xffff, 0xffff, 0xffff} ;
//static const GdkColor black_color =  {0, 0x0, 0x0, 0x0} ;

// STRUCTURE for Display 
struct _SCRN
{
	// ListClassDevices
	GtkWidget		*device_sel ;
	GtkTreeView		*device_treeview ;
	GtkTreeModel	*device_listmodel ;
	GtkWidget		*rbutProgram ;
	GtkWidget		*rbutCollaudo ;
	
	GtkWidget		*popup ;
	
	// Main window
	GtkWidget		*main ;
	GtkWidget		*lbl_header ;	// Label header
	GtkWidget		*lbl_run ; 		// Label work status
	GtkWidget		*bklbl_run ; 		// Label work status
	int				lrun_flashcol ;
	GtkWidget		*progress_bar; 	// Progress Bar
	GtkWidget		*lbl_amp ;		// Label of current 
	GtkWidget		*lbl_volt ; 		// label of voltage 
	GtkWidget		*lbl_free[8] ;	// 8 free label
	GtkWidget		*txt_amp ;
	GtkWidget		*txt_volt ;
	GtkWidget		*txt_free[8] ;
	GtkWidget		*bktxt_free[8] ;
	int 			txt_flashcol[8] ;
	
	GtkWidget		*boxtxt_mts ;
	GtkWidget		*boxtxt_answer ;

	GtkWidget		*cmd_script ;
	GtkWidget		*cmd_selection ;

	GtkWidget		*lbl_extname ;
	//GtkWidget		*lbl_protcom ;
	GtkWidget		*lbl_protname ;

	//GtkWidget		*cont_ampere ; 		// Amperometer container
	GtkWidget		*amperometer ; // NOT USED NOW

	GtkWidget		*clr_step[22] ;
	GtkWidget		*txt_step[22] ;

// Input box
	GtkWidget		*form_inputbox ;
	GtkWidget		*ib_lbl ;
	GtkWidget		*ib_txt ;
	GtkWidget		*ib_val ;
	GtkWidget		*ib_ok ;
	GtkWidget		*ib_abort ;

} ;
extern struct _SCRN  Scrn ;

extern GtkBuilder *builder;

// from MtsTestKit.c
extern void Add_txt_mts(GtkWidget *wd, char * new_buf) ;
extern void Add_txt_answer(GtkWidget *wd, char * new_buf) ;
extern void Clear_txt(GtkWidget *wd);
extern void Popup(char *title, char* message, GtkMessageType type, GtkMessageType buttons,int deletable) ;
extern void upd_amp(void) ;

// from ListClassDevices.c
extern void CreateListClassDevices(int type) ;
extern void class_program( GtkWidget * aa, gpointer  data ) ;
extern void class_taverniti( GtkWidget * aa, gpointer  data ) ;


// from tkmanage.c
extern void ports_tick(int port) ;

// from sequence.c
extern void Get_script_command(void) ;

