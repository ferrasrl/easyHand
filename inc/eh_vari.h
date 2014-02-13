// --------------------------------------------------------
//               DICHIARAZIONE EXTERN PER PRJ             !
// --------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

	extern EH_SYSTEM sys; // Area di sistema
	extern CHAR *PtJobber;

// ----------------------------------------------------
//	    VARIABILI PER LA GESTIONE DEGLI OGGETTI       !
// ----------------------------------------------------

	extern LONG OBJ_key; // Chiave collegata
	extern CHAR *OBJ_keyname;// Pt alla descrizione della chiave

// ----------------------------------------------------
//	        GESTIONE DELL'INPUT DA TASTIERA           !
// ----------------------------------------------------

	//extern INT IPT_fhdl,IPT_nfi,IPT_alt;
//	extern struct IPTINFO * IPT_info;
//	extern INT IPT_infohdl;
//	extern INT IPT_ult; 
//	extern INT IPT_max;
//	extern INT IPT_ins;

// ----------------------------------------------------
//	 VARIABILI PER LA GESTIONE DELLE FINESTRE         !
// ----------------------------------------------------

	//		WINMAX definito in flm_win.h

	extern struct WIN *WIN_info;
	extern INT WIN_infohdl;
	extern INT WIN_ult; // Numero di window aperte
	extern INT WIN_max;
	extern INT zooming;
	extern INT WIN_fz;
	extern INT zm_x1,zm_x2,zm_y1,zm_y2;
	extern INT relwx,relwy;

 /*
 extern INT EHPower; // Flag di inizializzazione di sistema

 //		Dos_harderror
 extern CHAR DHE_code[];
 extern DWORD DE_coden;
 extern INT  DHE_err;
 extern INT  DHE_drive;

 extern INT  DE_flag;
 extern INT  DE_last; // Ultimo errore ritornato da dos_error()

 extern INT SaveScreen;

 // ------------------------------------------
//	VARIABILI PER LA GESTIONE DELLA MEMORIA !
// ------------------------------------------

//	extern INT XMS_uso;
	extern LONG MEMO_inizio;
	extern INT MEMO_print;
//	extern LONG memo_conv; // Riserva di memoria convenzionale
	extern CHAR MEMO_path[]; // Percorso per lo swapping
	extern INT WIN_memo;

#ifdef __DOS__
	extern struct CLIP clip_stack[];
	extern INT	clip_num,clip_vedi;
#endif

// ---------------------------------------------------
//	  VARIABILI PER LA GESTIONE DEI FONT caratteri   !
// ---------------------------------------------------

	extern CHAR code_sys[];
	extern CHAR code_symbol[];
	extern CHAR *codepage;

#ifdef __DOS__
	extern struct FONT *FONT_info;
	extern INT  FONT_syshdl;
#endif

#ifndef EH_CONSOLE
// ----------------------------------------------------
//	 VARIABILI PER LA GESTIONE DEL MOUSE E DELL'INPUT !
// ----------------------------------------------------


	// Gestione Mouse Graphic Zone (settaggio automatico aspetto mouse)
	extern struct MGZ *sys.arsMgz;
	extern INT sys.hdlMgz;
	extern INT sys.iMgzNum; // Numero di MGZ
	extern INT MGZ_max;
	extern INT MGZ_zone;
//	extern CHAR MS_icob[];
//	extern INT MS_axb,MS_ayb;
#ifndef WIN32
	extern INT prs1,prs2; // Servono per double click e ritorno tasti
#endif
	extern INT HMZ_max;
	extern INT sys.iHmzNum;
	extern INT sys.hdlHmz;
	extern struct HMZ *sys.arsHmz;

	extern INT mouse_disp;
	extern LONG dbclkvel;  //	Velocita DoubleClick

//	extern CHAR keybuf[];		  // tasto premuto
//	extern CHAR *by_key;



#ifdef _WIN32 // Modalità windows
// -----------------------------------------------
//	VARIABILI GLOBALI PER LA GESTIONE IN WINDOWS !
// -----------------------------------------------

extern COLORREF  ColorPal[];
 
// --------------------------------------------
// Variabili gestione tastiera sotto windows  !
// --------------------------------------------
extern  INT CUR_Width,CUR_Height,CUR_Size;

// --------------------------------------------
// Gestione CLONAZIONE schermo                !
// --------------------------------------------

// Metodo:
// - Copiare sezione schermo di base
// - Aggiornare gli input
// - Aggiornare gli oggetti

extern INT sys.CLO_flag;
extern INT CLO_bck;
extern RECT sys.CLO_space;
extern struct VGAMEMO CLO_memo;
#else
 extern INT  DE_coden;
 extern INT  DE_classn;
 extern INT  DE_azionen;
 extern INT  DE_locusn;

 //		Dos_error rilevato
 extern CHAR *lpDE_code;
 extern CHAR *lpDE_class;
 extern CHAR *lpDE_azione;
 extern CHAR *lpDE_locus;
#endif
98
#endif

 // ----------------------------------------------------
//	     VARIABILI PER LA GESTIONE DELE PORTE SERIALI !
// ----------------------------------------------------
#if defined(EH_COM) || defined(EH_COMPLETED)
	extern struct COMINFO *COM_info;
	extern INT sys.iComMax; // Numero di COM gestite
	extern INT COM_aperte; // Numero di COM aperte
	extern INT COM_hdl; // Handle della gestione seriale

	extern CHAR COM_port8259; // Backup 8259
	extern CHAR *COM_baud[];
#endif

 extern 	INT ErrCtrl[];
 extern 	INT ErrCtrlNum;
*/

#ifdef __cplusplus
}
#endif
