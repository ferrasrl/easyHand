// --------------------------------------------------------
//³ EH_VAR.H - Dichiarazioni variabili e   ³
//³             strutture globali di sistema³
//³                                         ³
//³          by Ferr… Art & Technology 1996 ³
// --------------------------------------------------------

	EH_SYSTEM sys={0}; // Area di sistema
	CHAR 	*PtJobber; // Programma al lavoro
	void * (*FunzForEnd)(void); // driver da chiamare con PRG_end

// --------------------------------------------------------
//				         	   GESTIONE STACK                     !
// --------------------------------------------------------
// char far *stack;
// unsigned int stack_ss=0;
// unsigned int stack_sp=0;
// unsigned int old_ss;
// unsigned int old_sp;
//#define VIDEO 0xA000

	//	Plastocene!
	// Se non si è in modalità windows
	// creo uno stack alternativo
#ifdef __dos__
	extern WORD _stklen=SIZESTACK;
    void interrupt (*DRV_monitor) (void);
    struct CLIP clip_stack[MAXCLIP];
    INT	  clip_num=0,clip_vedi=OFF;
	CHAR  DHE_code[60];
	INT  DHE_err;
	INT  DHE_drive;
	INT  DE_flag=ON;
	INT  DE_last=0; // Ultimo errore ritornato da dos_error()
#endif

	CHAR pek_car;


// ------------------------------------------
//	VARIABILI PER LA GESTIONE DELLA MEMORIA !
// ------------------------------------------

 INT  XMS_uso;
 LONG MEMO_inizio=0;
 INT  MEMO_print=OFF;
 LONG memo_conv; // Riserva di memoria convenzionale
 CHAR MEMO_path[MAXPATH];
 INT  WIN_memo=OFF;
 INT  MemoLiberaControl=ON;


/*
// ---------------------------------------------------
//	  VARIABILI PER LA GESTIONE DEI FONT caratteri   !
// ---------------------------------------------------
#pragma warning( disable : 4305 )

	CHAR code_sys[256]=
	{ 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, //0-9
  	  63, 63, 63, 63, 63,132, 63, 63, 63, 63, //10-19
	 150,135,135, 63, 63, 63, 63, 63, 63, 63, //20-29
   	  63, 63,  0,  1,  2,  3,  4,  5,  6,  7, //30-39
 	   8,  9, 10, 11, 12, 13, 14, 15, 16, 17, //40-49
 	  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, //50-59
	  28, 29, 30, 31, 32, 33, 34, 35, 36, 37, //60-69
	  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, //70-79
	  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, //80-89
	  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, //90-99
	  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, //100-109
	  78, 79, 80, 81, 82, 83, 84, 85, 86, 87, //110-119
	  88, 89, 90, 91, 92, 93, 94, 95,167,220, //120-129
	 201,194,196,192,197,199,202,203,200,207, //130-139
	 206,204,164,165,169,198,166,212,214,210, //140-149
	 219,217,223,182,188,130,131,133, 63, 63, //150-159
	 193,205,211,218,209,177,138,154,159, 63, //160-169
	 140,157,156,129,139,155, 63, 63, 63, 63, //170-179
	  63, 63, 63, 63, 63, 63, 63, 63, 63, 63, //180-189
	  63, 63, 63, 63, 63, 63, 63, 63, 63, 63, //190-199
	  63, 63, 63, 63, 63, 63, 63, 63, 63, 63, //200-209
	  63, 63, 63, 63, 63, 63, 63, 63, 63, 63, //210-219
	  63, 63, 63, 63, 63, 63, 63,150, 63, 63, //220-229
	 149, 63, 63, 63, 63, 63, 63, 63, 63, 63, //230-239
	  63,145, 63, 63, 63, 63,215, 63,144, 63, //240-249
	 151, 63, 63,146, 63, 63                  //250-255
	 };

	CHAR code_symbol[256]=
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //0-9
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //10-19
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //20-29
	   0,  0,  0,  1,  2,  3,  4,  5,  6,  7, //30-39
	   8,  9, 10, 11, 12, 13, 14, 15, 16, 17, //40-49
	  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, //50-59
	  28, 29, 30, 31, 32, 33, 34, 35, 36, 37, //60-69
	  38, 39, 40, 41, 42, 43, 44, 45, 46, 47, //70-79
	  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, //80-89
	  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, //90-99
	  68, 69, 70, 71, 72, 73, 74, 75, 76, 77, //100-109
	  78, 79, 80, 81, 82, 83, 84, 85, 86, 87, //110-119
	  88, 89, 90, 91, 92, 93, 94, 95, 96, 97, //120-129
	  98, 99,100,101,102,103,104,105,106,107, //130-139
	 108,109,110,111,112,113,114,115,116,117, //140-149
	 118,119,120,121,122,123,124,125,126,127, //150-159
	 128,129,130,131,132,133,134,135,136,137, //160-169
	 138,139,140,141,142,143,144,145,146,147, //170-179
	 148,149,150,151,152,153,154,155,156,157, //180-189
	 158,159,160,161,162,163,164,165,166,167, //190-199
	 168,169,170,171,172,173,174,175,176,177, //200-209
	 178,179,180,181,182,183,184,185,186,187, //210-219
	 188,189,190,191,192,193,194,195,196,197, //220-229
	 198,199,200,201,202,203,204,205,206,207, //230-239
	 208,209,210,211,212,213,214,215,216,217, //240-249
	 218,219,220,221,222,223                  //250-255
	 };

#pragma warning( default : 4305 )

	CHAR *codepage=code_sys;
*/
	//CHAR FONT_name[31]="VGASYS";
	//INT  FONT_max=0,FONT_car=0;

#ifndef __windows__
	struct FONT *FONT_info=NULL;
	INT  FONT_syshdl=-1;
#endif

//	INT  FONT_hdl=-1; // Font-Handle default di sistema
//	INT  FONT_nfi=-1; // Dimensione carattere di default
//	INT  FONT_alt=-1;
//	INT  FONT_fix=OFF; // Flag di font a larghezza fissa (W98)

	//INT  FONT_keyhdl=-1; // Font-Handle default di sistema
	//INT  FONT_keynfi=-1; // Dimensione carattere di default
	//INT  FONT_keyalt=-1;


#ifndef EH_CONSOLE

// ----------------------------------------------------
//	        GESTIONE DELL'INPUT DA TASTIERA           !
// ----------------------------------------------------

	//INT IPT_fhdl,IPT_nfi,IPT_alt;
//	struct IPTINFO *IPT_info=NULL;
//	INT IPT_infohdl=-1;
//	INT IPT_ult=-1; // Numero di window aperte
//	INT IPT_max;
//	INT IPT_ins=ON;

// ----------------------------------------------------
//	 VARIABILI PER LA GESTIONE DELLE FINESTRE         !
// ----------------------------------------------------

	//		WINMAX definito in flm_win.h

	struct WIN * WIN_info=NULL;
	INT WIN_infohdl=-1;
	INT WIN_ult=-1; // Numero di window aperte
	INT WIN_max;
	INT zooming=OFF;
	INT WIN_fz=ON;
	INT zm_x1,zm_x2,zm_y1,zm_y2;
	INT relwx=0,relwy=0;

// ----------------------------------------------------
//	    VARIABILI PER LA GESTIONE DEGLI OGGETTI       !
// ----------------------------------------------------

	LONG  OBJ_key=-1; // Chiave collegata
	CHAR *OBJ_keyname;// Pt alla descrizione della chiave
#endif

// ----------------------------------------------------
//	     VARIABILI PER LA GESTIONE DEI FILE           !
// ----------------------------------------------------

//	CHAR dir_bak[600];
//	INT  FILE_aperti=0;


//DWORD DE_coden;

// ----------------------------------------------------
//	     VARIABILI PER LA GESTIONE DELLE STAMPE       !
// ----------------------------------------------------
//#if defined(LPT_MAX)
//struct LPT_INFO LPT_info[LPT_MAX];
//#endif // Se sono definite le stampanti


//	INT ErrCtrl[10];
//	INT ErrCtrlNum=-1;
