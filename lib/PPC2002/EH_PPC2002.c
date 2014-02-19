// -------------------------------------------
//  EH_PPC2002
//  EasyHand per Pocket PC 2002
//                                          
//                                          
//  Creato da: G.Tassistro  by Ferrเ A&T 2003 
// -------------------------------------------

#include "\ehtool\include\ehsw_i.h"

#define LIMITEMIN 5000
#define DEFRAM 512
extern BOOL MemoLiberaControl;

// --------------------------
// Funzioni                 !
// Temporanee               !
// --------------------------

 extern void (*FunzForEnd)(void); // driver da chiamare con PRG_end

// -----------------------------------------------------------------------------------------
// FLM_VID
// -----------------------------------------------------------------------------------------
 #define NOGOOD 2
 
 static CHAR *P_field(CHAR *buf,SINT NumCampo);
 static LONG P_valore(CHAR *p);
 static SINT P_flag(CHAR *p);
 static void P_errore(SINT ct,CHAR *riga);
 static void P_skip_area(SINT ct,FILE *ch);
 static SINT P_copia(CHAR *dest,CHAR *sorg,UINT Len);
 static void LoadEH3(void);
 #define EH3BUF 120
 static SINT EH3_hdl=-1;
 static SINT EH3_line=0;

// -------------------------------------------
// ณ PRG_start     Starting del programma    ณ
// ณ             - Sistema le variabili e    ณ
// ณ             - chiama i programmi necess ณ
// ณ               sari per la partenza.     ณ
// ณ			 - Controlla i driver 		 ณ
// ณ			   installati			     ณ
// ณ										 ณ
// ณ										 ณ
// ณ Ritorna OFF - Prima partenza   		 ณ
// ณ         ON  - Arrivo da un altro prog	 ณ
// ณ										 ณ
// ณ                           by Ferr… 1995 ณ
// -------------------------------------------
SINT PRG_start(void (*FunzForStart)(SINT),void (*Funz2)(void))
{
	SINT	max_icone;
//	LONG	Valore;
//	FILE	*ch;
//	CHAR	Campo1[20];
//	CHAR	buf[250];
	SINT	Ristart;
//	CHAR	*p1,*p2;

//	LONG	pos_font;
	SINT	led=OFF;
	SINT	win_ora=OFF;
	SINT	show=ON;
//	WORD	Flag;
//	SINT    a,ct;
	SINT a;
	HDC hdc;
	
    VStackFull=FALSE;
	PtJobber="PRG_start";
	if (EHPower) PRG_end(TEXT("PRG_start(): Invocato due volte"));
	*dir_bak=0;

	memset(&sys,0,sizeof(sys));
	
	sys.DirectDCActive=FALSE;
	sys.DirectDC=0;
	sys.ms_x=-1;
    sys.ms_y=-1;

	XMS_uso=ON;
	sys.memomaxi=10;
	sys.MEMO_diskult=-1;
	sys.OemTranslate=TRUE;
	sys.WinKeySize=100;
	sys.txc_hWnd=NULL;
    
	sys.WinWriteFocus=-1;
	sys.WDblClick=TRUE;
	sys.fFontBold=FALSE;  
    sys.fFontItalic=FALSE;
    sys.fFontUnderline=FALSE;
    sys.iFontRotation=0;
    sys.fMouseCapture=FALSE;
	sys.wMenuHeightMin=0;
	sys.wMenuLeftMin=0;
	sys.fTitleLock=FALSE;
	sys.fSoundEfx=FALSE;
	
	memo_conv=100000L;
	MEMO_print=OFF;
	WIN_max=5;
	FONT_max=10;
	FONT_hdl=0;
	FONT_nfi=0;
	max_icone=30;
	OBJ_rep1=300; OBJ_rep2=20;
	MGZ_max=0;
	HMZ_max=0;
	SaveScreen=60;
	COM_max=0;

	hdc=GetDC(NULL);
	for (a=0;a<16;a++) ColorPal[a]=GetNearestColor(hdc,ColorPal[a]);

	sys.video_x=GetDeviceCaps(hdc,HORZRES);
	sys.video_y=GetDeviceCaps(hdc,VERTRES);
	sys.colori=GetDeviceCaps(hdc,BITSPIXEL);
	sys.DeskTop_x=GetSystemMetrics(SM_CXFULLSCREEN);
	sys.DeskTop_y=GetSystemMetrics(SM_CYFULLSCREEN);
//	sys.WinBorder.x=GetSystemMetrics(SM_CXFRAME);
//	sys.WinBorder.y=GetSystemMetrics(SM_CYFRAME);
//	sys.yWinTitle  =GetSystemMetrics(SM_CYSIZE);
//	sys.yWinTop    =sys.yWinTitle+sys.WinBorder.y;
//	sys.yWinMenu   =GetSystemMetrics(SM_CYMENUSIZE);

	ReleaseDC(NULL,hdc);

/*
	EHPath("#START#");

    if (f_open(EHPath("EH3.INI"),"r",&ch)) 
		{win_infoarg("ERRORE: Manca il file di configurazione EH3.INI [%s][%d]",EHPath("EH3.INI"),GetLastError()); 
	     exit(2);
		}
	
	ct=0;
	for (;;)
	{ct++;
	 if (fgets(buf,sizeof(buf),ch)==NULL) break;

	 p1=P_field(buf,0);
	 if (p1==NULL) continue; // Campo vuoto o non valido
	 if (strlen(p1)>=sizeof(Campo1))
		{win_infoarg("ERRORE: Campo troppo lungo in EH3.INI\n%s",p1); exit(3);}
	 strcpy(Campo1,p1);

	 p2=P_field(buf,1);
	 Valore=P_valore(p2);// Ritorna -1 se non valido
	 Flag=P_flag(p2); // Ritorna NOGOOD se no valido

	 // ----------------------------
	 //		Gestione della memoria !
	 // ----------------------------

	 if (!strcmp(Campo1,"max_memo"))
					{if ((Valore<1)||(Valore>300)) P_errore(ct,buf);
					 sys.memomaxi=(WORD) Valore;
					 continue;}

	 if (!strcmp(Campo1,"memo_conv"))
					{if ((Valore<1)||(Valore>500)) P_errore(ct,buf);
					 memo_conv=(LONG) Valore*1000;
					 continue;}

	 if (!strcmp(Campo1,"memo_path"))
					{if (P_copia(MEMO_path,p2,MAXPATH)) P_errore(ct,buf);
					 strupr(MEMO_path);
					 continue;}

	 if (!strcmp(Campo1,"memo_vedi"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
					 WIN_memo=Flag; continue;}

	 if (!strcmp(Campo1,"memo_print"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
					 MEMO_print=Flag; continue;}

	 if (!strcmp(Campo1,"xms"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
					 XMS_uso=Flag; continue;}

	 if (!strcmp(Campo1,"mouse_vedi"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
					 mouse_disp=Flag; continue;}

	 if (!strcmp(Campo1,"soundefx"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
	                 sys.fSoundEfx=(Flag) ? TRUE : FALSE; 
					 continue;}

	 // ----------------------------
	 //		Gestione della windows !
	 // ----------------------------

	 if (!strcmp(Campo1,"max_windows"))
					{if ((Valore<1)||(Valore>300)) P_errore(ct,buf);
					 WIN_max=(WORD) Valore;
					 continue;}

	 if (!strcmp(Campo1,"obj_vedi"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
					 obj_vedib=Flag; continue;}

	 if (!strcmp(Campo1,"mgz_max"))
					{if ((Valore<1)||(Valore>100)) P_errore(ct,buf);
					 MGZ_max=(WORD) Valore; continue;}

	 if (!strcmp(Campo1,"hmz_max"))
					{if ((Valore<1)||(Valore>100)) P_errore(ct,buf);
					 HMZ_max=(WORD) Valore; continue;}

	 // --------------------------------------------------------
	 //			Numero di Porte seriali apribili conteporaneamente !
	 // --------------------------------------------------------

	 if (!strcmp(Campo1,"com_max"))
					{if ((Valore<1)||(Valore>16)) P_errore(ct,buf);
					 COM_max=(WORD) Valore;
					 continue;}

	 //------------------------
	 //			Gestione dei font !
	 // -----------------------

	 if (!strcmp(Campo1,"max_font"))
					{if ((Valore<1)||(Valore>2000)) P_errore(ct,buf);
					 FONT_max=(WORD) Valore;
					 continue;}

	 if (!strcmp(Campo1,"font_sys"))
					{if (P_copia(FONT_name,p2,30)) P_errore(ct,buf);
					 strupr(FONT_name);
					 continue;}

	 if (!strcmp(Campo1,"font_nfi"))
					{if ((Valore<0)||(Valore>20)) P_errore(ct,buf);
					 FONT_nfi=(WORD) Valore;
					 continue;}

	 //------------------------------------------
	 //			Typerating Ripetizione dell'oggetto !
	 //------------------------------------------

	 if (!strcmp(Campo1,"obj_winrep1"))
					{if ((Valore<1)||(Valore>5000)) P_errore(ct,buf);
					 OBJ_rep1=(WORD) Valore;
					 continue;}

	 if (!strcmp(Campo1,"obj_winrep2"))
					{if ((Valore<1)||(Valore>5000)) P_errore(ct,buf);
					 OBJ_rep2=(WORD) Valore;
					 continue;}

	 // ------------------------------------------
	 //		Carica i font                        !
	 // ------------------------------------------

	 if (!strcmp(Campo1,"font_load"))
			{pos_font=ftell(ch); P_skip_area(ct,ch); continue;}

	 if (!strcmp(Campo1,"max_icone"))
					{if ((Valore<30)||(Valore>2000)) P_errore(ct,buf);
					 max_icone=(WORD) Valore;
					 continue;}

	 // ------------------------------------------
	 //		Funzioni a tempo !
	 // ------------------------------------------

	 if (!strcmp(Campo1,"ledblink"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
					 led=Flag; continue;}

	 if (!strcmp(Campo1,"ehshow"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
					 show=Flag; continue;}

	 }
	f_close(ch);
*/

	// ------------------------------------------
	// ------------------------------------------
	// INIZIO PROGRAMMAZIONE AMBIENTE
	// ------------------------------------------
	// ------------------------------------------

	Ristart=OFF;

	if (FunzForStart!=NULL) (*FunzForStart)(WS_PROCESS);

	//  Apro il gestore della memoria 
	a=memo_start();
	if (a) {PRG_end(TEXT("Errore MEMO_START hd"));}

	// da Fare
	//if (com_start())  PRG_end("No memory:COM manager");
	//lpt_start();

	// Guarda funzioni aggiuntive di preprogrammazione
	FunzForEnd=NULL;
	if (FunzForStart!=NULL) (*FunzForStart)(WS_LAST);
	FunzForEnd=Funz2;

	// ณ 			 BACKUP DIRECTORY DI LANCIO       ณ
//	_getcwd(dir_bak,MAXDIR);

	EHPower=ON; // Sistema attivato
	//LoadEH3();

	CoInitializeEx(NULL,COINIT_MULTITHREADED);
//    InitCommonControls();
 return Ristart;
}
 
// -------------------------------------------
// | PRG_end       Fine del programma        |
// -------------------------------------------
void PRG_end(TCHAR *Mess,...)
{
	va_list Ah;
	va_start(Ah,Mess);
	if (FunzForEnd!=NULL) (*FunzForEnd)(); // Chiusura Drivers Esterni
	
	CoUninitialize();
	 // Libera il gestore della memoria
	 //if (memo_end()==-2) win_info("Attenzione:LMO in sospeso\n\7");
	 memo_end();
//	 if (*dir_bak>0) chdir(dir_bak);

	 if (Mess==NULL) // Usato per le DLL
	 {
		fcloseall();
		va_end(Ah);
		return;
	 }

	if (!*Mess)
	{
		fcloseall();
		va_end(Ah);
		exit(0);
		}
		else
		{
			TCHAR *lpBuf;
			lpBuf=malloc(2000);
			//printf("<BR>\nEasyHand Internet Application:<BR>\nError in procedure:<BR>\n");
			vswprintf(lpBuf,Mess,Ah);
			MessageBox(NULL, lpBuf, TEXT("EH Errore in procedura"), MB_ICONEXCLAMATION | MB_OK);
			fcloseall();
			va_end(Ah);
#ifdef _PRGENDKEY
		getch();
#endif

#ifdef _DEBUG
//			getch();
#endif
			exit(1);
		}
}

/*
// ------------------------------------------------------------
//  Controlla e sistema la stringa del file di configurazione !
//																														!
//  - Se la stringa  nulla (Commento non contiene dati)      !
//    La funzione ritorna NULL                                !
//                                                            !
//  - Viene cercato l'inizio della stringa                    !
//  - Viene marcata la fine della stringa                     !
//                                                            !
// ------------------------------------------------------------

static CHAR *P_field(CHAR *buf,SINT NumCampo)
{
	static CHAR Campo[MAXPATH];
	WORD Cnt,NCampo;
	CHAR z;
	CHAR *p;

	//		Tutto in minuscolo
	strlwr(buf); p=buf;

	// --------------------------------
	//		Testa se il record  valido !
	// --------------------------------

	for (;(!((*p==0)||(*p>' ')));p++);
	if ((*p==';')||(*p=='\n')||(*p=='\r')||(*p==0)) return NULL;// Stringa vuota

	for (NCampo=0;;NCampo++)
	{
	 // Trova
	 for (Cnt=0;;Cnt++)
	 {z=*(p+Cnt);
		if (z==0) break; // Fine stringa
		if ((z=='=')||(z==' ')||(z=='\0')||(z=='\n')||(z=='\r')||(z==',')) break;
	 }
	 if ((Cnt==0)||(Cnt>=sizeof(Campo))) return NULL;

	 if (NCampo==NumCampo)
				{memcpy(Campo,p,Cnt);
				 Campo[Cnt]=0;
				 return Campo;
				 }

	 p+=Cnt;

	 for (;(!((*p==0)||(*p>' ')));p++);
	 if ((*p==';')||(*p=='\n')||(*p=='\r')||(*p==0)) break;// Fine
	}
	return NULL;
}
// ----------------------------------------------
//	Copia una stringa in un'altra con controllo !
// ----------------------------------------------
static SINT P_copia(CHAR *dest,CHAR *sorg,UINT Len)
{
 if (sorg==NULL) return -1;// No buono
 if (strlen(sorg)>=Len) return -1; // Non ci st…
 strcpy(dest,sorg);
 return 0;
}
// ---------------------------------------
//	Converte il campo in valore numerico !
// ---------------------------------------
static LONG P_valore(CHAR *p)
{
 if (p==NULL) return -1;// Non valido
 return atol(p);
}

// ---------------------------------------
//	Converte il campo in Flag ON/OFF     !
// ---------------------------------------
SINT P_flag(CHAR *p)
{
 if (p==NULL) return NOGOOD;// Non valido
 if (!strcmp(p,"on")) return ON;
 if (!strcmp(p,"off")) return OFF;
 return NOGOOD;
}
// ---------------------------------------
//	Gestione degli errori sul file       !
// ---------------------------------------

void P_errore(SINT ct,CHAR *riga)
{
 win_info("Errore nella configurazione in EH3.INI:\n\7");
 win_infoarg("%d: %s\n",ct,riga);
 exit(2);
}

void P_skip_area(SINT ct,FILE *ch)
{
 SINT a;

 for (;;)
 {
 a=fgetc(ch);
 if (a==EOF) P_errore(ct,"File interrotto o Inquinato");
 if (a=='}') return;
 }
}


CHAR *EHPath(CHAR *Str)
{
 static CHAR EHP[MAXPATH]="";
 static CHAR EHPR[MAXPATH]="";

 if (*Str=='@')
 {
   strcpy(EHP,Str+1);
   if (EHP[strlen(EHP)-1]!='\\') strcat(EHP,"\\");
   return NULL;
 }

 if (!strcmp(Str,"#START#")&&!*EHP)
	{
	 if (getenv("EHPATH")==NULL) strcpy(EHP,"C:/Eh3/"); else strcpy(EHP,getenv("EHPATH"));
	 if (EHP[strlen(EHP)-1]!='/') strcat(EHP,"/");
	 return NULL;
	}

 strcpy(EHPR,EHP);
 strcat(EHPR,Str);
 return EHPR;
}
// Ricerca un argomento in EH3.INI
// Deve poter girare senza Easyhand
static void LoadEH3(void)
{
 CHAR buf[EH3BUF];
 UINT a;
 SINT L;
 FILE *ch;

 ch=fopen(EHPath("eh3.ini"),"r");
 if (ch==NULL)
	{sprintf(buf,"LoadEH3(): %s",sys_errlist[_doserrno]);
	 PRG_end(buf);
	}

 fseek(ch,0,SEEK_SET);
 EH3_line=0;

 // Conta le linee
 for (;;)
 {
	if (fgets(buf,EH3BUF,ch)==NULL) break;
	EH3_line++;
 }

 EH3_hdl=memo_chiedi(RAM_AUTO,(LONG) EH3_line*EH3BUF,"EH3.INI");
 if (EH3_hdl<0) PRG_end("Non c' memo per EH3");

 fseek(ch,0,SEEK_SET);

 // Carica le linee in memoria
 for (L=0;L<EH3_line;L++)
 {
	if (fgets(buf,EH3BUF,ch)==NULL) break;
	///		Tolgo CR O LF
	for (a=0;a<strlen(buf);a++) if ((buf[a]=='\n')||(buf[a]=='\r')) buf[a]=0;

	memo_scrivivar(EH3_hdl,(LONG) L*EH3BUF,buf,EH3BUF);
 }
 fclose(ch);
}

SINT ini_find(CHAR *cerca,CHAR *serv)
{
 CHAR buf[EH3BUF];
 UINT a;
 SINT rt;
 FILE *ch;

 rt=-1;
 if (EHPower) // Se il sistema  attivo EH3.ini  in memoria
 {
	for (a=0;a<(UINT)EH3_line;a++)
	{
	 memo_leggivar(EH3_hdl,(LONG) a*EH3BUF,buf,EH3BUF);
	 // Se lo trova
	 if (memcmp(cerca,buf,strlen(cerca))==0) {rt=0;strcpy(serv,buf);break;}
	}
 }
 else  // Vecchio sistema per eredit…
 {
 EHPath("#START#");
 ch=fopen(EHPath("eh3.ini"),"r");
 if (ch==NULL)
	{sprintf(buf,"ini_find(): %s",sys_errlist[_doserrno]);
	 PRG_end(buf);
	}
 for (;;)
 {
	if (fgets(buf,sizeof(buf),ch)==NULL) break;

	 ///		Tolgo CR O LF
	 for (a=0;a<strlen(buf);a++) if ((buf[a]=='\n')||(buf[a]=='\r')) buf[a]=0;

	 // Se lo trova
	 if (memcmp(cerca,buf,strlen(cerca))==0)
			{rt=0;strcpy(serv,buf);break;}
 }

 fclose(ch);
 }

 return rt;
}
void pausa(LONG ps)
{
	LONG Stt,Ct;

	Stt=GetTickCount(); Ct=Stt; Stt+=ps;
	for(;Stt>Ct;) Ct=GetTickCount();
}
*/

// -----------------------------------------------------------------------------------------
// FLM_RAM
// -----------------------------------------------------------------------------------------

//                           :
//		FUNZIONI DI SERVIZIO :
//                           :

// ------------------------------------------------------------
//  Controlla e sistema la stringa del file di configurazione !
//																														!
//  - Se la stringa  nulla (Commento non contiene dati)      !
//    La funzione ritorna NULL                                !
void win_memoerr(TCHAR *buf)
{
  MessageBox(NULL,buf,TEXT("EH:Gestore memoria"),MB_ICONHAND|MB_OK);
}

//-------------------------------------------------------------
//			      Calcola in nuovo puntatore far              -
//-------------------------------------------------------------
void *farPtr(void *p,LONG l)
{
	BYTE *ptr;
	ptr=p; ptr+=l;
	return ptr;
}
// ีออออออออออออออออออออออออออออออออออออออออออธ
// ณFAR_    Ricerca con metodo binario in una ณ
// ณBSEARCH zona far                          ณ
// ณ                                          ณ
// ณ cerca = Stringa cerca                    ณ
// ณ dbase = Puntatore all'area che contiene  ณ
// ณ         le stringhe in ordine alfabetico ณ
// ณ pt		 = Se la trova  il puntatore     ณ
// ณ         alla stringa                     ณ
// ณ size  = Grandezza del record             ณ
// ณ offset= Spiazzamento del campo           ณ
// ณ nkey  = Numero chiavi contenute          ณ
// ณ                                          ณ
// ณ Ritorna 0 = Chiave trovata               ณ
// ณ        -1 = Chiave non trovata           ณ
// ณ                                          ณ
// ิออออออออออออออออออออออออออออออออออออออออออพ

SINT far_bsearch(CHAR *cerca,
				 CHAR *dbase,
				 WORD *pos,
				 SINT size,
				 SINT offset,
				 WORD nkey)
{
	WORD ct=0;
	SINT psx,pdx,pt,ce;
	CHAR *trovo;

	if (nkey<1) {*pos=0;return -1;} // Archivio vuoto
	ce=0;
	psx=0; pdx=nkey-1;
	for (;;)
	{
		pt=psx+((pdx-psx)>>1);// Zona centrale
		trovo=farPtr(dbase,((LONG) (size)*pt)+offset);
		if (strcmp(trovo,cerca)==0) {ce=1;break;} // Stringa uguale

		if (strcmp(trovo,cerca)>0)
			 {//		Stringa piu grande
				if (psx==pt) {break;} // Fine archivio
				pdx=pt-1; //if (pdx<psx) {pt=psx+1;break;}
			 }
			 else
			 {//		Stringa pi— piccola
				if (pdx==pt) {pt++;break;} // Fine archivio
				psx=pt+1; //if (psx>pdx) {pt=pdx;break;}
			 }
	 if (ct++>nkey) {PRG_end(TEXT("Errore in far_bsearch"));}
//	if (psx>nico) {pt=nico;break;} // +grande dell'ultimo
	}
	//avanti:
	*pos=pt;
	if (ce==0) return -1; else return 0;
}


// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_START Crea la struttura per la     ณ
// ณ            gestione della memoria       ณ
// ณ                                         ณ
// ณ memomaxi = Numero di memorie massime    ณ
// ณ            gestibili dal sistema        ณ
// ณ                                         ณ
// ณ Ritorna ID della memoria se =>0		 ณ
// ณ oppure :        						 ณ
// ณ                 						 ณ
// ณ -1 = Non c memoria bassa sufficiente   ณ
// ณ                 						 ณ
// ณ                 						 ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ

HLOCAL handMemo=NULL;

SINT memo_start(void)
{
 SINT a;

 // Richiede l'handle dell'oggetto
 handMemo=LocalAlloc(LPTR, sizeof(struct RAM)*sys.memomaxi); 
 if (handMemo==NULL) {return -1;}

 if ((sys.memolist=(struct RAM *) LocalLock(handMemo))==NULL) {LocalFree(handMemo);return -1;}
 
 //		Resetta
 sys.memocont=0;
 for (a=0;a<sys.memomaxi;a++) (sys.memolist+a)->iTipo=RAM_HDLFREE;
 //sys.MEMO_inizio=coreleft ();
 sys.MEMO_inizio=0;//GetFreeSpace(0);
 sys.MEMO_rimasta=sys.MEMO_inizio;
 //win_info("memo_start()");
 return 0;
}


// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_END    Libera tutta la memoria     ณ
// ณ             richiesta                   ณ
// ณ                                         ณ
// ณ  ritorna  = -1 Non c' il segmento      ณ
// ณ              0 TUTTO OK                 ณ
// ณ                                         ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ
SINT memo_end(void)
{
	SINT a;
	if (sys.memocont==0) return -1;

	MemoLiberaControl=OFF;
	for (a=0;a<sys.memomaxi;a++) {memo_libera(a,sys.memolist[a].User);}

	if (handMemo!=NULL) {LocalUnlock(handMemo); LocalFree(handMemo);}
	return 0;
}
//if (handMemo!=NULL) {LocalUnlock(handMemo); LocalFree(handMemo);}



// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_USATA  Ritorna la memoria bassa    ณ
// ณ             usata                       ณ
// ณ                                         ณ
// ณ  ritorna  = -1 Non c' il segmento      ณ
// ณ              0 TUTTO OK                 ณ
// ณ                                         ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ

LONG memo_usata(void)
{
 LONG totale=0;
 SINT a;

 for (a=0;a<sys.memomaxi;a++)
	 {
		if (sys.memolist[a].iTipo==RAM_BASSA) totale+=sys.memolist[a].dwSize;
		if (sys.memolist[a].iTipo==RAM_HEAP) totale+=sys.memolist[a].dwSize;
	 }
 return totale;
}


// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_CHIEDI Chiede memoria al sistema   ณ
// ณ                                         ณ
// ณ tipo = Tipo di memoria richiesta        ณ
// ณ                                         ณ
// ณ        RAM_HEAP = Area di memoria conv  ณ
// ณ                   fissa non spostabile  ณ
// ณ                                         ณ
// ณ        RAM_AUTO = Area di memoria       ณ
// ณ                   temporanea accedere   ณ
// ณ                   a questa area solo conณ
// ณ                   le funzioni memo_...  ณ
// ณ                                         ณ
// ณ sizebyte: Memoria richiesta          	 ณ
// ณ usr     : Nome del richiedente/uso      ณ
// ณ                 						 ณ
// ณ Ritorna ID della memoria se =>0		 ณ
// ณ oppure :        						 ณ
// ณ                 						 ณ
// ณ -1 = Non c memoria bassa sufficiente   ณ
// ณ                 						 ณ
// ณ                 		  Ferr… (c) 1996 ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ

SINT memo_chiedi(SINT tipo,LONG sizebyte,CHAR *usr)
{
 SINT hdl,hdl2;
 HGLOBAL hglb;
 void *lpvBuffer;

 //	---------------------------------------------
 //		Controlla i contatori del gestore memorie !
 // ---------------------------------------------

 hdl=-1;
 if (sys.memocont>=sys.memomaxi) PRG_end(TEXT("Gestore memorie esaurito [max %d]"),sys.memomaxi);

 //		Cerca primo memo handle disponibile
 for (hdl2=0;hdl2<sys.memomaxi;hdl2++) {if (sys.memolist[hdl2].iTipo==RAM_HDLFREE) {hdl=hdl2;break;}}
 if (hdl==-1) goto fine;// Non c una area disponibile

 // Aggiunta per ridurre la frammentazione
 if (sizebyte<DEFRAM) sizebyte=DEFRAM;

 switch (tipo)
 {
 //	------------------------------------------------------
 //		RAM_HEAP : Richiesta memoria di tipo HEAP/protetto !
 // ------------------------------------------------------

		case RAM_HEAP :

		//win_infoarg("HEAP User [%s], [%ld] ",usr,sizebyte);
		if ((sizebyte+LIMITEMIN)>sys.MEMO_rimasta) {hdl=-1;break;}

		//		Controlla se c abbastanza memoria

		//hglb=GlobalAlloc(GPTR, sizebyte); // Richiede l'handle dell'oggetto
		//if (hglb==NULL) {hdl=-1;break;}

		//lpvBuffer=GlobalLock(hglb); // Trova il puntatore all'oggetto
		lpvBuffer=malloc(sizebyte);
		if (lpvBuffer==NULL) {//GlobalFree(hglb);
		                      win_info(TEXT("malloc=NULL"));
							  hdl=-1;
							  break;
							 }
        //lpvBuffer=malloc(sizebyte);
		//if (lpvBuffer==NULL) {hdl=-1;break;}
		//	Archivia la richiesta
		sys.memocont++;
		sys.memolist[hdl].iTipo  =tipo;
		sys.memolist[hdl].lpvMemo=(void *) lpvBuffer;
		sys.memolist[hdl].dwSize =sizebyte;//GlobalSize(hglb);// Grandezza reale
		sys.memolist[hdl].hGlobal=0;//hglb;
		sys.MEMO_rimasta-=sizebyte;
		break;


 //	------------------------------------------------
 //		RAM_ALTA : Richiesta memoria di tipo alto  !
 // ----------------------------------------------

		case RAM_BASSA:
		case RAM_ALTA :
		case RAM_XMS  :
		case RAM_DISK :

		// Alloca memoria movibile
		hglb = GlobalAlloc(GMEM_MOVEABLE, sizebyte); // Richiede l'handle dell'oggetto
		if (hglb==NULL) {hdl=-1;break;}

		sys.memocont++;
		sys.memolist[hdl].iTipo=RAM_BASSA;
		sys.memolist[hdl].lpvMemo=0;
		sys.memolist[hdl].dwSize=GlobalSize(hglb);// Grandezza reale
		sys.memolist[hdl].hGlobal=hglb;
		break;

//	 no_memory:
	 hdl=-1;
	 break;

 } // Bracket dello switch

 //sys.memolist[hdl].user[MEMOUSER-1]=0;// End mark// Copia il nome dell'user
 if (hdl!=-1) 
	{memcpy(sys.memolist[hdl].User,usr,MEMOUSER-1);
	 sys.memolist[hdl].User[MEMOUSER-1]=0;// End mark
	}

 fine:

 return hdl;
}
// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_LIBERA Libera memoria al sistema   ณ
// ณ                                         ณ
// ณ  hdl		   = Handle assegnato        ณ
// ณ                                         ณ
// ณ  ritorna  = 0 Tutto OK                  ณ
// ณ           = -1 Handle errato            ณ
// ณ           = -2 Handle libero            ณ
// ณ           = -3 Area LMO inquinata       ณ
// ณ           = -4 Errore XMS               ณ
// ณ           = -5 Violata protezione       ณ
// ณ                                         ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ
SINT memo_libera(SINT hdl,CHAR *chisei)
{
 SINT flag=0;

 //		Controlli principali
 if ((hdl<0)||(hdl>=sys.memomaxi)) {flag=-1;goto fine;}

 // Memoria di tipo protetto da password
 if (sys.memolist[hdl].User[0]=='*')
		{
		 if (strcmp(sys.memolist[hdl].User,chisei))
				{
				 win_infoarg(TEXT("memo_libera(): Tentativo di accesso a\n[%s] da parte di [%s]"),
							  sys.memolist[hdl].User,chisei);
				 return -5;
				}
		}

 switch (sys.memolist[hdl].iTipo)
 {
 //	---------------------------------------
 //		RAM_HEAP  : Libera memoria bassa  !
 // ---------------------------------------

		case RAM_HEAP :
		//if (!GlobalUnlock(sys.memolist[hdl].hGlobal))  win_memoerr("GlobalUnlock ?");
		//if (GlobalFree(sys.memolist[hdl].hGlobal)!=NULL) win_memoerr("memo_libera():Errore in Globalfree 1"); 
        //if (sys.memolist[hdl].lpvMemo==NULL) PRG_end("lpmemo=NULL");
		//if (hdl==47) win_infoarg("[%s] [%s]",chisei,sys.memolist[hdl].User);
		if (sys.memolist[hdl].lpvMemo==NULL) {win_info(TEXT("memo_libera: Heap2")); break;}
		free(sys.memolist[hdl].lpvMemo); 
		sys.memolist[hdl].lpvMemo=NULL;
        //free(sys.memolist[hdl].lpvMemo);
		sys.MEMO_rimasta+=sys.memolist[hdl].dwSize;
		sys.memolist[hdl].lpvMemo=NULL;
		break;

 //	---------------------------------------------
 //		RAM_AUTO : Libera memoria di tipo alto  !
 // ---------------------------------------------
		case RAM_GLOBALHEAP :
			Wmemo_unlock(hdl);

		case RAM_AUTO :
		case RAM_BASSA :
		case RAM_XMS :
		case RAM_DISK :

		if (GlobalFree(sys.memolist[hdl].hGlobal)!=NULL) win_memoerr(TEXT("memo_libera():Errore in Globalfree"));
		break;

 //	-------------------------------------------
 //		RAM_HDLFREE : memoria libera				    !
 // -------------------------------------------

		case RAM_HDLFREE :

		if (MemoLiberaControl)
		 {
			TCHAR serv[80];
			wsprintf(serv,TEXT("HDLFREE: Hdl=%d %s"),hdl,chisei);
			win_memoerr(serv);
		 }
		flag=-2;
		break;

		default :

		flag=-3;
		break;

 } // Bracket dello switch

 if (!flag) {sys.memolist[hdl].iTipo=RAM_HDLFREE; sys.memocont--;}

 fine:

 if ((flag)&&(MemoLiberaControl))
		 {TCHAR serv[50];
		  wsprintf(serv,TEXT("MemoLibera:\n[Flag %d] Hdl=%d\n|%s|"),flag,hdl,chisei);
		  win_err(serv);
		 }

 return flag;
}

// ีอออออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_SCRIVI Copia una porzione di convenz ณ
// ณ             in una memoria handle         ณ
// ณ                                           ณ
// ณ input :  memo_hdl = Codice memoria        ณ
// ณ          dest		 = (long) destinazione ณ
// ณ          sgm      = Segmento sorgente     ณ
// ณ          off      = offset sorgente       ณ
// ณ          numbyte	 = (long) byte da copy ณ
// ณ                                           ณ
// ณ   ritorna  = -1 Non c'e il memo_hdl       ณ
// ณ            = -2 Destinazione o size ERR   ณ
// ณ            = 0  TUTTO OK                  ณ
// ณ                                           ณ
// ิอออออออออออออออออออออออออออออออออออออออออออพ

SINT memo_scrivivar(SINT memo_hdl,LONG dest,
					void far *Source,
					LONG numbyte)
{
 CHAR * Destinazione;
 
 //	---------------------------------------------
 //		Controlla i contatori del gestore memorie !
 // ---------------------------------------------

 if ((memo_hdl<0)||(numbyte<1)) return -1; // Controllo contatore interno

 //		Controlla che sia nei limiti
 if ((DWORD) (dest+numbyte)>sys.memolist[memo_hdl].dwSize)
	{win_memoerr(TEXT("memo_scrivivar:Size o Dest errato"));
	 return -2;
	}

 switch (sys.memolist[memo_hdl].iTipo)
 {

 //	----------------------------------------
 //		RAM_HEAP : Legge da memoria protetta !
 //		RAM_BASSA: Legge da memoria bassa    !
 // ----------------------------------------

		case RAM_GLOBALHEAP:
		case RAM_HEAP : // Per memoria giเ precedentemento Lockata

		  Destinazione=sys.memolist[memo_hdl].lpvMemo;
		  Destinazione+=dest;
		  memcpy(Destinazione, Source, numbyte);
		  break;

		case RAM_AUTO: // Memoria da Lockare
		case RAM_BASSA:
		case RAM_XMS:
		case RAM_DISK:

		  Destinazione=GlobalLock(sys.memolist[memo_hdl].hGlobal);
		  if (Destinazione==NULL) win_memoerr(TEXT("memo_scrivivar:Errore in GlobaLock"));

		  Destinazione+=dest;
		  memcpy(Destinazione, Source, numbyte);
		  GlobalUnlock(sys.memolist[memo_hdl].hGlobal);
		  break;

	}
	return 0;
 }



// ีอออออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_LEGGI Copia una porzione di memoria  ณ
// ณ            allocata in un segmento basso  ณ
// ณ                                           ณ
// ณ input :  memo_hdl = Codice memoria        ณ
// ณ          sorg	   = (long) sorgente       ณ
// ณ          sgm      = Segmento destinazione ณ
// ณ          off      = offset destinazione   ณ
// ณ          numbyte  = (long) byte da leggereณ
// ณ                                           ณ
// ณ   ritorna  = -1 Non c'e il memo_hdl       ณ
// ณ            = 0  TUTTO OK                  ณ
// ณ                                           ณ
// ิอออออออออออออออออออออออออออออออออออออออออออพ
SINT memo_leggivar(SINT memo_hdl,
				   LONG sorg,
				   void far *Destinazione,
				   LONG numbyte)
{
  TCHAR buf[80];
  CHAR * Source;
  SINT FlagOut=0;

 //	-----------------------------------------------
 //		Controlla i contatori del gestore memorie !
 // -----------------------------------------------

 if ((memo_hdl<0)||(numbyte<1)) {FlagOut=-1; goto FINE;} // Controllo contatore interno

 //		Controlla che sia nei limiti
 if ((DWORD) (sorg+numbyte)>sys.memolist[memo_hdl].dwSize) {FlagOut=-2; goto FINE;}

 switch (sys.memolist[memo_hdl].iTipo)
 {
 //	----------------------------------------
 //		RAM_HEAP : Legge da memoria protetta !
 //		RAM_BASSA: Legge da memoria bassa    !
 // ----------------------------------------

		case RAM_GLOBALHEAP:
		case RAM_HEAP : // Per memoria giเ precedentemento Lockata

		  Source=sys.memolist[memo_hdl].lpvMemo;
		  Source+=sorg;
		  memcpy(Destinazione, Source, numbyte);
		break;

		case RAM_AUTO: // Memoria da Lockare
		case RAM_BASSA:
		case RAM_XMS:
		case RAM_DISK:

		  Source=GlobalLock(sys.memolist[memo_hdl].hGlobal);
		  if (Source==NULL) win_memoerr(TEXT("memo_leggivar:Errore in GlobaLock"));

		  Source+=sorg;
		  memcpy(Destinazione, Source, numbyte);
		  GlobalUnlock(sys.memolist[memo_hdl].hGlobal);

		break;
	 }

 FINE:

 if (FlagOut)
	 {wsprintf(buf,TEXT("memo_leggivar(): Hdl=%d Flag=%d"),memo_hdl,FlagOut);
		win_memoerr(buf);
	 }

 return FlagOut;
}




// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_INFO Informazioni su in handle	 ณ
// ณ                                         ณ
// ณ hdl   = Handle interessato              ณ
// ณ *tipo = Conterra il tipo:RAM_HEAP ecc.. ณ
// ณ *sgm  = Conterr… se c il segmento      ณ
// ณ                                         ณ
// ณ Ritorna 0 se l'handle e valido   		 ณ
// ณ oppure :        						 ณ
// ณ                 						 ณ
// ณ -1 = Handle free (Non valido)           ณ
// ณ -2 = Handle errato                      ณ
// ณ                 						 ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ
SINT memo_info(SINT hdl,SINT *tipo,WORD *sgm)
{
 if ((hdl<0)||(hdl>(sys.memomaxi-1))) return -2;
 if (sys.memolist[hdl].iTipo==RAM_HDLFREE) return -1;
 *tipo=sys.memolist[hdl].iTipo;
 if (sgm) *sgm=0;//sys.memolist[hdl].sgm;
 return 0;
}

// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_TIPO Ritorna il tipo di memoria 	 ณ
// ณ           Assegnato al handle           ณ
// ณ                                         ณ
// ณ hdl   = Handle interessato              ณ
// ณ                                         ณ
// ณ Ritorna 0 se l'handle e valido   		 ณ
// ณ oppure :        						 ณ
// ณ                 						 ณ
// ณ -1 = Handle free (Non valido)           ณ
// ณ -2 = Handle errato                      ณ
// ณ    									 ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ

SINT memo_tipo(SINT hdl)
{
 SINT a;

 if ((hdl<0)||(hdl>(sys.memomaxi-1))) a=-2;
		else a=sys.memolist[hdl].iTipo;

 return a;
}
// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_HEAP Ritorna il puntatore alla  	 ณ
// ณ           HEAP                          ณ
// ณ                                         ณ
// ณ void * Far se in modello large          ณ
// ณ        near se in modello medium        ณ
// ณ                                         ณ
// ณ Ritorna NULL se l'handle non  valido   ณ
// ณ        								 ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ

void * memo_heap(SINT hdl)
{
 //CHAR buf[30];
 BOOL fError=FALSE;

 if (hdl<0) fError=TRUE;
 if (!fError) {if (sys.memolist[hdl].iTipo!=RAM_HEAP) fError=TRUE;}
 if (fError)
	{
	 //sprintf(buf,"memo_heap ?: [%d][%s]",hdl,sys.memolist[hdl].User);
	 //win_errgrave(buf);
	 win_infoarg(TEXT("memo_heap ?: [%d]"),hdl);//,(hdl>-1) ? sys.memolist[hdl].User : TEXT("?"));
	 PRG_end(TEXT("memo_heap ?: [%d]"),hdl);//,(hdl>-1) ? sys.memolist[hdl].User : TEXT("?"));
	 return NULL;
	}
 
 return sys.memolist[hdl].lpvMemo;
}
// Solo Windows
SINT memo_clone(SINT Hdl)
{
	SINT HdlNew;
	HdlNew=memo_chiedi(sys.memolist[Hdl].iTipo,sys.memolist[Hdl].dwSize,sys.memolist[Hdl].User);
	if (HdlNew<0) return HdlNew;
	memo_copyall(Hdl,HdlNew);
	return HdlNew;
}

// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ MEMO_COPYALL Copia un intera memoria 	 ณ
// ณ              in un altra memoria        ณ
// ณ                                         ณ
// ณ hdl1  = Handle sorgente                 ณ
// ณ hdl2  = Handle destinazione             ณ
// ณ                                         ณ
// ณ Ritorna 0 se l'handle e valido   		 ณ
// ณ oppure :        						 ณ
// ณ                 						 ณ
// ณ -1 = Handle free (Non valido)           ณ
// ณ -2 = Handle errato                      ณ
// ณ -3 = Dimensioni memoria differenti      ณ
// ณ -4 = Stesso handle                      ณ
// ณ -5 = Non gestito                        ณ
// ณ                 						 ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ

SINT memo_copyall(SINT hdl1,SINT hdl2)
{
 SINT a;
 SINT tipo1,tipo2;
 DWORD size;
 SINT FlagSorg=OFF;
 SINT FlagDest=OFF;
 CHAR *Sorg=NULL;
 CHAR *Dest=NULL;

 //		Pre-controlli
 if (hdl1==hdl2) {a=-4;goto FINE;}
 if ((hdl1<0)||(hdl1>(sys.memomaxi-1))) {a=-2;goto FINE;}
 if ((hdl2<0)||(hdl2>(sys.memomaxi-1))) {a=-2;goto FINE;}
 tipo1=sys.memolist[hdl1].iTipo; tipo2=sys.memolist[hdl2].iTipo;
 if ((tipo1==RAM_HDLFREE)||(tipo2==RAM_HDLFREE)) {a=-1;goto FINE;}

 //	Dimenzioni adeguate
 size=sys.memolist[hdl1].dwSize;
 if (size!=sys.memolist[hdl2].dwSize) {a=-3;goto FINE;}

 // Controllo se sono Heap

  if ((sys.memolist[hdl1].iTipo!=RAM_HEAP)&&(sys.memolist[hdl1].iTipo!=RAM_GLOBALHEAP))
		{Sorg=GlobalLock(sys.memolist[hdl1].hGlobal);
		 if (Sorg==NULL) {win_memoerr(TEXT("copyall:Errore in LOCK hld1"));
						  a=-10;goto FINE;
						}
		 FlagSorg=ON;
		 } else Sorg=sys.memolist[hdl1].lpvMemo;

 if ((sys.memolist[hdl2].iTipo!=RAM_HEAP)&&(sys.memolist[hdl2].iTipo!=RAM_GLOBALHEAP))
		{Dest=GlobalLock(sys.memolist[hdl2].hGlobal);
		 if (Dest==NULL) {win_memoerr(TEXT("copyall:Errore in LOCK hld2"));
											GlobalUnlock(sys.memolist[hdl1].hGlobal);
											a=-11;goto FINE;
											}
		 FlagDest=ON;
		 } else Dest=sys.memolist[hdl2].lpvMemo;

		memcpy(Dest, Sorg, sys.memolist[hdl1].dwSize);

 if (FlagSorg) GlobalUnlock(sys.memolist[hdl1].hGlobal);
 if (FlagDest) GlobalUnlock(sys.memolist[hdl2].hGlobal);

 FINE:
 return a;
}


// Solo Windows
void *Wmemo_lock(SINT memo_hdl)
{
  if (sys.fInternalError) return NULL;
  if (sys.memolist[memo_hdl].iTipo!=RAM_BASSA) 
  {
	 sys.fInternalError=TRUE;
	 PRG_end(TEXT("Wmemo_lock(%d)"),memo_hdl);
  }
  sys.memolist[memo_hdl].iTipo=RAM_GLOBALHEAP; 
  sys.memolist[memo_hdl].lpvMemo=GlobalLock(sys.memolist[memo_hdl].hGlobal);

  return sys.memolist[memo_hdl].lpvMemo;
}

void Wmemo_unlock(SINT memo_hdl)
{
  if (sys.fInternalError) return;
 if (memo_hdl<0)
  {
	sys.fInternalError=TRUE;
	PRG_end(TEXT("Wmemo_unlock %d"),memo_hdl);
  }

  if (sys.memolist[memo_hdl].iTipo!=RAM_GLOBALHEAP) 
  {
	  sys.fInternalError=TRUE;
	  PRG_end(TEXT("Wmemo_unlock %d (%d-%s)"),
	          sys.memolist[memo_hdl].iTipo,
			  memo_hdl,
			  sys.memolist[memo_hdl].User);
  }

  sys.memolist[memo_hdl].iTipo=RAM_BASSA; 
  sys.memolist[memo_hdl].lpvMemo=NULL;
  GlobalUnlock(sys.memolist[memo_hdl].hGlobal);

}

void CtrlFree(void *ptr)
{
 SINT a;
 for (a=0;a<sys.memomaxi;a++) 
 {
   if ((sys.memolist[a].iTipo==RAM_HEAP)&&(sys.memolist[a].lpvMemo==ptr))
   {
     win_infoarg(TEXT("CtrlFree error %d [%x]"),a,sys.memolist[a].lpvMemo);
   }
 }
 free(ptr);
}

void * EhAlloc(SINT iMemo) {return GlobalAlloc(GPTR,iMemo);}
void * EhReAlloc(void *ptr,SINT iOldMemo,SINT iNewMemo) 
	{
		CHAR *p;
		p=EhAlloc(iNewMemo);
		if (!p) win_infoarg(L"EhReAlloc error:[%d:%d]",GetLastError(),iNewMemo);
		memcpy(p,ptr,iOldMemo);
		EhFree(ptr);
		return p;
	}

void EhFree(void *ptr) {GlobalFree(ptr);}

// ---------------------------------------------------------------------------------------------------------------------
// FLM_WIN Gestione delle windows             
//                                            
//																			Creato da: G.Tassistro  by Ferr… SD '995/6 |
// ---------------------------------------------------------------------------------------------------------------------
void win_info(TCHAR info[])
{
	win_infoarg(info);
}

void win_errgrave(TCHAR info[])
{
	win_infoarg(TEXT("ERRORE GRAVE:\n%s"),info);
	//PRG_end(info);
}

void win_err(TCHAR info[])
{
	win_infoarg(TEXT("ERRORE:\n%s"),info);
}

void win_infoarg(TCHAR *Mess,...)
{
	va_list Ah;
	TCHAR *lpBuf;
	lpBuf=malloc(2000);

	va_start(Ah,Mess);
	//vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	vswprintf(lpBuf,Mess,Ah); // Messaggio finale
	//win_info(lpBuf);
	MessageBox(WindowNow(), lpBuf, TEXT("Info"), MB_ICONEXCLAMATION | MB_OK);

	free(lpBuf);
	va_end(Ah);
}


// ---------------------------------------------------------------------------------------------------------------------
// ณ FLM_FILE  Ferr… Library Module : FILE    
// ณ           Gestione dei FILE              
// ณ                                          
// ณ																									by Ferr… 1999 
// ---------------------------------------------------------------------------------------------------------------------

// --------------------------------------------
//	ณ F_OPEN Apre un file e controlla gli     ณ
//	ณ        errori                           ณ
//	ณ                                         ณ
//	ณ nome = Puntatore al nome del file       ณ
//	ณ                                         ณ
//	ณ tipo = Tipologia di apertura            ณ
//	ณ        vedi fread()                     ณ
//	ณ                                         ณ
//	ณ **ch = Puntatore ad un FILE *ch         ณ
//	ณ                                         ณ
//	ณ Ritorna : 0 = Tutto OK                  ณ
//	ณ           Segnala errore e ritorna      ณ
//	ณ           IGNORA                        ณ
//	ณ           RIPROVA                       ณ
//	ณ           ABORT e DOSERR                ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1996 ณ
// --------------------------------------------

SINT f_open(TCHAR *nome,TCHAR *tipo,FILE **ch)
{
	 FILE *kk;
	 SINT a;
	 static CHAR serv[250];

	 a=0;
	 for (;;)
		{kk=_tfopen(nome,tipo); if (kk!=NULL) {a=0;*ch=kk;break;}
		 sprintf(serv,"Apertura file; %s,%s\n",nome,tipo);
		 a=os_error(serv); if (a!=RIPROVA) break;
		}

	 if (!a) FILE_aperti++;
	 return a;
}

// --------------------------------------------
//	ณ F_CLOSE Chiude un file e controlla gli  ณ
//	ณ         errori                          ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1996 ณ
// --------------------------------------------
SINT f_close(FILE *ch)
{
	 SINT a;

	 a=0;
	 for (;;)
		{a=fclose(ch); if (a==0) break;
		 a=os_error("Chiusura file; \n"); if (a!=RIPROVA) break;
		}

	 if (!a) FILE_aperti--;
	 return a;
}
// --------------------------------------------
//	ณ F_GET  Legge da disco una porzione di   ณ
//	ณ        file                             ณ
//	ณ                                         ณ
//	ณ ch = FILE *ch                           ณ
//	ณ location : Puntatore nel file in byte   ณ
//	ณ            NOSEEK non esegue il         ณ
//	ณ            puntamento                   ณ
//	ณ                                         ณ
//	ณ *sorg    : Puntatore alla destinazione  ณ
//	ณ            memoria (near)               ณ
//	ณ                                         ณ
//	ณ dim      : Dimensioni da trasferire     ณ
//	ณ            (WORD)                       ณ
//	ณ                                         ณ
//	ณ Ritorna : 0 = Tutto OK                  ณ
//	ณ           Segnala errore e ritorna      ณ
//	ณ           IGNORA                        ณ
//	ณ           RIPROVA                       ณ
//	ณ           ABORT e DOSERR                ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1996 ณ
// --------------------------------------------
SINT f_get(FILE *ch,LONG location,void *dest,UINT dim)
{
	 SINT a;

	 if (location!=NOSEEK)
	 {for (;;)
		{if (fseek(ch,location,SEEK_SET)==0) break; // OK Riuscito
		 a=os_error("Lettura record/seek;\n"); if (a!=RIPROVA) return a;
		}
	 }
	 if (dim<1) return 0; // No read

	 for (;;)
		{if (fread(dest,dim,1,ch)==1) {a=0;break;}
	     //if (GetLastError()==0) {a=0;break;} // Operazione terminata con successo (EOF)
		 a=os_error("Lettura record;\n"); if (a!=RIPROVA) break;
		}

	 return a;
}

// --------------------------------------------
//	ณ F_GETS  Legge da disco una porzione di  ณ
//	ณ         file tipo fgets                 ณ
//	ณ                                         ณ
//	ณ ch = FILE *ch                           ณ
//	ณ location : Puntatore nel file in byte   ณ
//	ณ            NOSEEK non esegue il         ณ
//	ณ            puntamento                   ณ
//	ณ                                         ณ
//	ณ *sorg    : Puntatore alla destinazione  ณ
//	ณ            memoria (near)               ณ
//	ณ                                         ณ
//	ณ dim      : Dimensioni da trasferire     ณ
//	ณ            (WORD)                   ณ
//	ณ                                         ณ
//	ณ Ritorna : 0 = Tutto OK                  ณ
//	ณ           Segnala errore e ritorna      ณ
//	ณ           IGNORA                        ณ
//	ณ           RIPROVA                       ณ
//	ณ           ABORT e DOSERR                ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1996 ณ
// --------------------------------------------
SINT f_gets(FILE *ch,CHAR *buf,UINT dim)
{
	 SINT a;

	 if (dim<1) return 0; // No read

	 for (;;)
		{if (fgets(buf,dim,ch)!=NULL) {a=0;break;}
		 a=os_error("Lettura f_gets;\n"); if (a!=RIPROVA) break;
		}

	 return a;
}

// --------------------------------------------
//	ณ F_PUT  Scrive da disco una porzione di  ณ
//	ณ        file                             ณ
//	ณ                                         ณ
//	ณ ch = FILE *ch                           ณ
//	ณ location : Puntatore nel file in byte   ณ
//	ณ            NOSEEK non esegue il         ณ
//	ณ            puntamento                   ณ
//	ณ                                         ณ
//	ณ *sorg    : Puntatore al sorgente in     ณ
//	ณ            memoria (near)               ณ
//	ณ                                         ณ
//	ณ dim      : Dimensioni da trasferire     ณ
//	ณ            (WORD)                   ณ
//	ณ                                         ณ
//	ณ Ritorna : 0 = Tutto OK                  ณ
//	ณ           Segnala errore e ritorna      ณ
//	ณ           IGNORA                        ณ
//	ณ           RIPROVA                       ณ
//	ณ           ABORT e DOSERR                ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1996 ณ
// --------------------------------------------
SINT f_put(FILE *ch,LONG location,void *sorg,UINT dim)
{
	 SINT a;

	 if (location!=NOSEEK)
	 {for (;;)
		{if (fseek(ch,location,SEEK_SET)==0) break; // OK Riuscito
		 a=os_error("Scrittura record/seek;\n"); if (a!=RIPROVA) return a;
		}
	 }

	 if (dim<1) return 0; // No write

	 a=0;
	 for (;;)
		{if (fwrite(sorg,dim,1,ch)==1) {a=0;break;}
		 a=os_error("Scrittura record;\n"); if (a!=RIPROVA) break;
		}
	 return a;
}

// --------------------------------------------
//	ณ F_EXIST Controlla l'esistenza di un fileณ
//	ณ                                         ณ
//	ณ nome = Puntatore al nome del file       ณ
//	ณ                                         ณ
//	ณ Ritorna : 0 = Non c                    ณ
//	ณ           1 = Esistente                 ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1996 ณ
// --------------------------------------------
SINT f_exist(TCHAR *lpNome)
{
	 //SINT a;
	 //static CHAR serv[250];
	 HANDLE hFile;
//	 wchar_t wcServ[250];

//	 CharToUnicode(wcServ,lpNome);

	 hFile=CreateFile(lpNome,
					  GENERIC_READ, 
					  FILE_SHARE_READ,
					  NULL,
					  OPEN_EXISTING,
					  FILE_ATTRIBUTE_NORMAL,
					  NULL);
	 if (hFile==INVALID_HANDLE_VALUE) return 0;
	 CloseHandle(hFile);
	 return 1;

/*
	 a=0;
	 for (;;)
		{
		 os_errset(OFF);
		 a=access(nome,0); // Guardo se esiste
		 if (a==0) {a=1; os_errset(POP);break;} // Esiste
         //win_infoarg("%d",GetLastError());
		 if (GetLastError()==3) {a=0; os_errset(POP);break;} // No Esiste
		 
		 a=os_error(serv);
		 os_errset(POP);
		 if (a==RIPROVA) continue;

		 if (DE_coden==2) {a=0;break;} // File inesistente
		 //sprintf(serv,"%x",DE_coden); disp(100,100,15,1,serv);
		 if (DE_flag!=OFF)
		 {
			sprintf(serv,"Accesso al file: %s\n",nome);
			os_errvedi(serv);
			if (a!=RIPROVA) break;
		 }
		}
	 return a;
*/
}

// --------------------------------------------
//	ณ F_GETDRIVE Legge il drive corrente      ณ
//	ณ            percorso completo            ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1999 ณ
// --------------------------------------------
/*
SINT f_getdrive(void)
{
 return _getdrive();
}
 */

// --------------------------------------------
//	ณ F_GETDIR Legge dal drive specifico il   ณ
//	ณ          percorso completo              ณ
//	ณ                                         ณ
//	ณ ch = FILE *ch                           ณ
//	ณ location : Puntatore nel file in byte   ณ
//	ณ            NOSEEK non esegue il         ณ
//	ณ            puntamento                   ณ
//	ณ                                         ณ
//	ณ *sorg    : Puntatore alla destinazione  ณ
//	ณ            memoria (near)               ณ
//	ณ                                         ณ
//	ณ dim      : Dimensioni da trasferire     ณ
//	ณ            (WORD)                       ณ
//	ณ                                         ณ
//	ณ Ritorna : 0 = Tutto OK                  ณ
//	ณ           Segnala errore e ritorna      ณ
//	ณ           IGNORA                        ณ
//	ณ           RIPROVA                       ณ
//	ณ           ABORT e DOSERR                ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1996 ณ
// --------------------------------------------
/*
SINT f_getdir(SINT drive,CHAR *dir)
{
	CHAR dirtr[_MAX_PATH ];
	SINT a;

	for (;;)
	{if (_getdcwd( drive, dir, _MAX_PATH )!=NULL)
			{if (strlen(dir)==0) sprintf(dirtr,"%c:\\",drive+64);
			 a=0;break;
			}
		 a=os_error("Lettura Path corrente;\n"); if (a!=RIPROVA) break;
	}

	 return a;
	return 0;
}
*/
// --------------------------------------------
//	ณ F_FINDFIRST Ricerca il primo file vali  ณ
//	ณ F_FINDNEXT  do di un perscorso          ณ
//	ณ                                         ณ
//	ณ input    : vedi findfirst()&next pg.428 ณ
//	ณ                                         ณ
//	ณ Ritorna : 0 = Tutto OK                  ณ
//	ณ           Segnala errore e ritorna      ณ
//	ณ           IGNORA                        ณ
//	ณ           RIPROVA                       ณ
//	ณ           ABORT e DOSERR                ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1996 ณ
// --------------------------------------------
/*
SINT f_findFirst(CHAR *fname,FFBLK *ptr,SINT attrib)
{
  SINT a;
  //,attrib ?
  for (;;)
	 { ptr->Handle=_findfirst(fname,&ptr->ffile);
	   if (ptr->Handle!=-1) {a=0;break;}
	   a=os_error("Lettura prima directory;\n"); if (a!=RIPROVA) break;
	 }

  ptr->AttribSel=attrib;
  
  if (!a)
	 {
	  // Controllo se l'attributo ่ quello giusto
	  for (;!(ptr->ffile.attrib&ptr->AttribSel);)
	  {
		if (f_findNext(ptr)) {a=-1; break;}
	  }
      if (!a)
	  {
		ptr->ff_name=ptr->ffile.name;
		ptr->ff_attrib=ptr->ffile.attrib;
		//strcpy(ptr->ff_date,ctime(ptr->ffile.time_write));
		strcpy(ptr->ff_date,"ggmmaaaa");
	  }
	 }
  return a;
}

SINT f_findNext(FFBLK *ptr)
{
	 SINT a;
	 for (;;)
		{if (_findnext(ptr->Handle,&ptr->ffile)==0) {a=0;break;}
		 a=os_error("Lettura sequenziale directory;\n"); if (a!=RIPROVA) break;
		}

	 if (!a)
		 {
	       for (;!(ptr->ffile.attrib&ptr->AttribSel);)
		   {
		     if (f_findNext(ptr)) {a=-1; break;}
		   }
          
		   if (!a)
		  {
			ptr->ff_name=ptr->ffile.name;
			ptr->ff_attrib=ptr->ffile.attrib;
			strcpy(ptr->ff_date,"ggmmaaaa");
		  }
		 }
	 return a;
}

void f_findClose(FFBLK *ptr)
{
  _findclose(ptr->Handle);
  ptr->Handle=0;
}
*/

// --------------------------------------------
// ณ FILE_LEN Ritorna le dimensione di       ณ
// ณ          file                           ณ
// ณ                                         ณ
// ณ File : Nome del file che contiene       ณ
// ณ        il Font                          ณ
// ณ                                         ณ
// ณ Ritorna :                            	 ณ
// ณ                 						 ณ
// ณ -1 = Il file  inesistente              ณ
// ณ                 						 ณ
// --------------------------------------------

LONG file_len(TCHAR *file)
{
 FILE *fp;
 LONG len;
// SINT a;

 fp=_tfopen(file,TEXT("rb")); if (!fp) return -1;

 fseek(fp,0,SEEK_END);
 len=ftell(fp);
 fclose(fp);

 return len;
}

// --------------------------------------------
// ณ FILE_LOAD Carica un file in memoria     ณ
// ณ                                         ณ
// ณ File : Nome del file che va caricato    ณ
// ณ tipo : RAM_BASSA,RAM_HEAP,RAM_ALTA      ณ
// ณ                                         ณ
// ณ Ritorna handle della memoria assegnata	 ณ
// ณ oppure :        						 ณ
// ณ                 						 ณ
// ณ -1 = Il file  inesistente              ณ
// ณ -2 = Non c memoria disponibile FILE    ณ
// ณ -3 = Non c memoria per fare lo swap    ณ
// ณ -5x= File inquinato (x errore f-get)    ณ
// ณ -6x= Errore in memo_(operazione)        ณ
// ณ      x errore memo_scrivivar  			 ณ
// ณ                 						 ณ
// --------------------------------------------
SINT file_load(TCHAR *file,SINT tipo)
{
 FILE *fp;
 LONG lenfile;
 SINT hdl;
 CHAR memo[256];
 BYTE *lpb;

 lenfile=file_len(file); if (lenfile<0) return (SINT) lenfile; // Il file non esiste

 sprintf(memo,"F:%s",file_name(file));
 //																						!
 //		CHIEDE MEMORIA AL SISTEMA PER IL FILE		!
 //																						!
 hdl=memo_chiedi(tipo,lenfile+1,memo);
 if (hdl==-1) return -2; // Non c memoria disponibile

	//dwSize=file_len(lpFileName);
	//iRealSize=dwSize+sizeof(S_TXB)+1;
	//hdl=memo_chiedi(RAM_AUTO,iRealSize,file_name(lpFileName));
	//if (hdl<0) {f_close(ch); return NULL;}
 fp=_tfopen(file,TEXT("rb"));
 if (!fp) {memo_libera(hdl,"file_load4"); return -5;}

 if (tipo==RAM_AUTO)
 {
	lpb=Wmemo_lock(hdl);
 }
 else lpb=memo_heap(hdl);

 fread(lpb,lenfile,1,fp);
 lpb[lenfile]=0;

 if (tipo==RAM_AUTO)
 {
	Wmemo_unlock(hdl);
 }
/*
 // -----------------------------
 // Lettura diretta in RAM_HEAP !
 // -----------------------------

 if (tipo==RAM_HEAP)
		{
			//		Apre il file e inizia il trasferimento
		 if (f_open(file,"rb",&fp))
			{memo_libera(hdl,"file_load1"); f_close(fp);return -5;}

			ptf=memo_heap(hdl);
			block=0xf000;// Blocco di trasferimento

		 for (;;)
		 {
			block2=lenfile; if (block2>block) block2=(LONG) block;
			if (block2<=0) break;
			//printf ("%s %ld\n\r",file,block2);

			if (f_get(fp,NOSEEK,ptf,(WORD) block2))
			{memo_libera(hdl,"file_load2"); f_close(fp); return -5;};

			ptf=farPtr(ptf,block2);
			lenfile-=block2;
		 }
		}
	 else
	 {
 //																						!
 //		     IMPORTA IL FILE IN MEMORIA         !
 //		            CON SWAPPING                !
 //																						!

 //			Cerca e Chiede blocco per il trasporto
 block=0xF000;
 for (a=0;a<8;a++)
 {
	serv=memo_chiedi(RAM_HEAP,(LONG) block,"FileLOAD");
	if (serv>-1) break;
	block/=2;
 }
	if (serv==-1) {memo_libera(hdl,"file_load3");return -3;}
	ptf=memo_heap(serv);

 //		Apre il file e inizia il trasferimento
 if (f_open(file,"rb",&fp))
		{memo_libera(hdl,"file_load4"); memo_libera(serv,"file_load5"); f_close(fp);return -5;}

 //		Trasferimento del file con swapping
 ptrmemo=0;
 for (;;) {
	block2=lenfile; if (block2>block) block2=block;
	if (block2<=0) break;
	//printf ("(S) %s pos %ld %ld ",file,ptrmemo,block2);
	err=f_get(fp,NOSEEK,ptf,(WORD) block2);
	if (err)
			{memo_libera(hdl,"file_load6");
			 memo_libera(serv,"file_load7");
			 f_close(fp);
			 return (err-50);};

	err=memo_scrivivar(hdl,ptrmemo,ptf,(LONG) block2);
	if (err)
			{//printf("Hdl=%d",hdl);
			 memo_libera(hdl,"file_load8");
			 memo_libera(serv,"file_load9");
			 f_close(fp);
			 return (err-60);};

	ptrmemo+=block2; lenfile-=block2;
	}

 memo_libera(serv,"file_load10");
}
*/
 f_close(fp);

 return hdl;
}


// --------------------------------------------
// ณ FILE_NAME  Ritorna solo il nome del fileณ
// ณ            (sensa il percorso)          ณ
// ณ                                         ณ
// ณ File : Nome del file che contiene       ณ
// ณ        il Font                          ณ
// ณ                                         ณ
// ณ Ritorna :                            	 ณ
// ณ                 						 ณ
// ณ -1 = Il file  inesistente              ณ
// ณ                 						 ณ
// --------------------------------------------
TCHAR *file_name(TCHAR *file)
{
 SINT a;
 TCHAR *p=file; // era Huge
 p+=_tcslen(file)-1;
 for (a=_tcslen(file);a>1;a--,p--) {if ((*p=='\\')||(*p==':')) {p++;break;}}
 return p;
}
/*

// --------------------------------------------
// ณ os_drvtype Ritorna se esiste un drive  ณ
// ณ             e se  locale o remoto      ณ
// ณ                                         ณ
// ณ                                         ณ
// ณ  Drive : 1=A,2=B ecc...                 ณ
// ณ                                         ณ
// ณ  Ritorno -1 Non c'                     ณ
// ณ           0 Floppy                      ณ
// ณ           1 Hard disk                   ณ
// ณ           2 CD-ROM                      ณ
// ณ           3 Network                     ณ
// ณ                                         ณ
// ณ                           by Ferr… 1996 ณ
// --------------------------------------------
SINT os_drvtype(SINT iDrive)
{
 CHAR RootP[30];
 WORD wReturn;

 sprintf(RootP,"%c:\\",iDrive-1+'A');
 wReturn = GetDriveType(RootP);

		switch (wReturn) {
				case 0:   // Indeterminabile
				case 1:   // Root ???
				case DRIVE_REMOVABLE:
						return -1;
				case DRIVE_FIXED:
						//strcat(szMsg, "fixed");
						return 1;
				//		break;
				case DRIVE_CDROM:
						//strcat(szMsg, "fixed");
						return 2;
				case DRIVE_REMOTE:
						//strcat(szMsg, "remote (network)");
						return 3;
			//			break;
		}
		//TextOut(hdc, 10, 15 * iDrive, szMsg, strlen(szMsg));
 return -1;
}

// --------------------------------------------
// ณ os_VolumeInfo                           ณ
// ณ Ritorna informazioni sul Volume         ณ
// ณ                                         ณ
// --------------------------------------------

BOOL f_volumeinfo(CHAR *lpRootPathName,VOLINFO *VolInfo)
{
   return GetVolumeInformation(lpRootPathName,
						       // Nome del volume
						       VolInfo->Name, sizeof(VolInfo->Name)-1,
						       // Serial-number del Volume
						       &VolInfo->SerialNumber,
						       // Massima grandezza del Percorso
						       &VolInfo->MaxPath,
						       // System Flags
						       &VolInfo->SystemFlags,
						       // Nome del volume
						       VolInfo->OSName, sizeof(VolInfo->OSName)-1); 
}
// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ os_ERRCODE Ritorna codici di errore     ณ
// ณ             dos verificati              ณ
// ณ                                         ณ
// ณ Puntatore a quattro variabili intere    ณ
// ณ                                         ณ
// ณ Ritorna :                            	 ณ
// ณ                 						 ณ
// ณ code = Codice errore verificato         ณ
// ณ clas = Classe di appartenenza           ณ
// ณ act  = Azione suggerita				 ณ
// ณ locus= In relazione a ...               ณ
// ณ                                         ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ
void os_errcode(SINT *code,SINT *clas,SINT *act,SINT *locus)
{
 *code=0;
 win_info("os_error: da fare");
}
*/

// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ os_ERROR Scrive nell'area rapport       ณ
// ณ           la descrizione degli errori   ณ
// ณ                                         ณ
// ณ                              by Ferr… 96ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ
SINT os_error(CHAR *ms1)
{
	return 0;
}

void os_errvedi(CHAR *ms)
{
 LPVOID lpMsgBuf;
 TCHAR *lpMess; 

 FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		       NULL,
               DE_coden,
               MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
               (LPTSTR) &lpMsgBuf,
			   0,
			   NULL);// Display the string.
 
 if (ms==NULL) ms="<NULL>";
 lpMess=malloc(strlen(lpMsgBuf)+strlen(ms)+100);
 wsprintf(lpMess,TEXT("%s\n%s"),ms,lpMsgBuf);
 MessageBox(NULL,lpMess,L"Errore di Sistema",MB_OK|MB_ICONINFORMATION);
 LocalFree(lpMsgBuf);
 CtrlFree(lpMess);
}
// -------------------------------------------
// ณ os_ERRSET Setta il funzionamento della  !
// ณ            funzione di controllo errori ณ
// ณ            DOS                          ณ
// ณ                                         ณ
// ณ tipo = ON Su errore mostra l'errore     ณ
// ณ           e esce dal programma          ณ
// ณ        ONVEDI Su errore mostra l'errore ณ
// ณ               e ritorna al programma    ณ
// ณ        OFF Mostra sollo errori gravi    ณ
// ณ            SINT 24h per chiedere azione ณ
// ณ            Poi ritorna al programma     ณ
// ณ        POP Ripristina tipo precedente   ณ
// ณ                                         ณ
// ณ                                         ณ
// ณ                              by Ferr… 96ณ
// -------------------------------------------
void os_errset(SINT tipo)
{
	static SINT err[10];
	static SINT num=-1;

	switch (tipo)
	{
	 case ON:
	 case ONVEDI:
	 case OFF:
				num++; if ((num<0)||(num>9)) PRG_end(TEXT("Overflow in os_errset()"));
				err[num]=DE_flag;
				DE_flag=tipo;
				break;

	 case POP:
				if (num<0) PRG_end(TEXT("POP Overflow in os_errset()"));
				DE_flag=err[num];
				num--;
				break;

	 default : PRG_end(TEXT("Tipo errato in os_errset()"));
	}
}

void os_ErrorShow(CHAR *ms,SINT iError)
{
 LPVOID lpMsgBuf=NULL;
 LPVOID lpMess; 

 FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		       NULL,
               iError,
               MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
               (LPTSTR) &lpMsgBuf,
			   0,
			   NULL);// Display the string.
 
 if (ms==NULL) ms="<NULL>";
 if (!lpMsgBuf) return;
 lpMess=malloc(strlen(lpMsgBuf)+strlen(ms)+100);
 wsprintf(lpMess,TEXT("%s\n%s"),ms,lpMsgBuf);
 MessageBox(NULL,lpMess,TEXT("Errore di Sistema"),MB_OK|MB_ICONINFORMATION);
 LocalFree(lpMsgBuf);
 CtrlFree(lpMess);
}

TCHAR *file_path(TCHAR *file)
{
 static TCHAR Path[MAXPATH];
 TCHAR *p;

 _tcscpy(Path,file);
 p=file_name(Path); if (p) *p=0; 
 return Path;
}

#ifdef _UNICODE
wchar_t *rwcsstr(wchar_t *String,wchar_t *Find)
{
	SINT iFind=wcslen(Find);
	wchar_t *p=String+wcslen(String)-iFind;
    SINT a;	

	for (a=0;a<(SINT) (wcslen(String)-iFind);a++)
	{
	  if (!memcmp(p,Find,iFind)) return p; else p--;
	}
	return NULL;
}
#endif

TCHAR *rstrstr(TCHAR *String,TCHAR *Find)
{
	SINT iFind=_tcslen(Find);
	TCHAR *p=String+_tcslen(String)-iFind;
    SINT a;	
	for (a=0;a<(SINT) (_tcslen(String)-iFind);a++)
	{
	  if (!memcmp(p,Find,iFind)) return p; else p--;
	}
	return NULL;
}
void NoSpaceCue(BYTE *lpStr)
{
 SINT a,len;
 if (!*lpStr) return;
 len=strlen(lpStr); lpStr+=len-1;
 for (a=0;a<len;a++,lpStr--) {if ((*lpStr<'!')||(*lpStr>'z')) {*lpStr=0; continue;} else break;}
}

SINT xtoi(CHAR *str)
{
 SINT a,v,c;
 SINT ris;

 if (*str==0) return 0;

 ris=0; c=0;
 for (a=strlen(str)-1;a>-1;a--)
 {
	v=(SINT) *(str+a);
	if ((v>='0')&&(v<='9')) v-='0';
	if ((v>='A')&&(v<='F')) {v-='A';v+=10;}
	if ((v>='a')&&(v<='f')) {v-='a';v+=10;}
	v*=(1<<c);
	ris+=v; c+=4;
 }
 return ris;
}


//---------------------------------------------------
//ณ MASK - Formatizza i numeri per la stampa       ณ
//ณ                                                ณ
//ณ *num  = Stringa da Stampare                    ณ
//ณ numcif= Lunghezza in cifre (prima della virgolaณ
//ณ dec   = Numero decimali                        ณ
//ณ sep   = Flag separatori (0=No,1=Si)            ณ
//ณ segno = Flag segno (OFF=Solo -,ON=+&-)         ณ
//ณ                                                ณ
//ณ                                  by Ferr… 1996 ณ
//---------------------------------------------------
TCHAR *nummask(TCHAR *num,SINT numcif,SINT dec,SINT sep,SINT segno)
{
	CHAR lensx,lendx,sg=0;
	SINT  ptsx,ptdx,ptnum,a,pt,cnt;
	SINT  len,lnum;
	static TCHAR srv[40];
	TCHAR *sr=srv;

	srv[0]=0;

	//	PRE-controlli
	if ((lnum=_tcslen(num))==0) goto errore;
	if ((numcif<1)||(numcif>18)) goto errore;
	if ((dec<0)||(dec>10)) goto errore;

	//	Calcola lunghezza stringa di output
	len=(dec>0) ? numcif+1+dec : numcif;
	if (sep) len+= ((numcif-1)/3);
	//if ((segno)||(num[0]=='-'))
	len++;

	//	Pulisce la stringa per il ritorno
	for (a=0;a<len;a++) {srv[a]=' ';}
	srv[len]=0;

	//	C il segno : sg va a 1 o 2
	ptsx=0;
	if (num[ptsx]=='+') ptsx++;
	if (num[ptsx]==' ') ptsx++;
	if (num[ptsx]=='-') {sg='-'; ptsx++;}
	if ((segno)&&!sg) {if (num[0]=='0') sg=' '; else sg='+';}

	// 	Ricerco se ci sono decimali nel numero passato
	lensx=lnum-ptsx; lendx=0;
	for (a=ptsx;num[a]!=0;a++) {
			if ((num[a]=='.')||(num[a]==',')) {lensx=a-ptsx; ptdx=a+1; lendx=lnum-a-1; break;}
	}

	// 	La stampa prevede decimali ? li copia per l'output
	if (dec>0) {
		 pt=len-dec-1; ptnum=pt-1; srv[pt++]=',';
		 for (a=0;a<dec;a++) {srv[pt++]=((a+1)>lendx) ? '0' : num[ptdx++];}
	 }
	 else ptnum=len-1;

	//  Stampa numero con separatori
	cnt=0; ptsx+=lensx-1;
	for (a=0;a<lensx;a++) {
			if ((sep)&&(cnt++==3)) {srv[ptnum--]='.';cnt=1;}// divide le migliaia
			if ((ptsx<0)||(ptnum<0)) goto errore;
			srv[ptnum--]=num[ptsx--];
	}

	if (sg) {if (ptnum>=0) srv[ptnum]=sg; else goto errore;}

fine:

	sr=srv; return sr;

errore:
	srv[0]='?'; goto fine;
}


static TCHAR Num[80];

// Ritorna una stringa con separazioni ,/.
TCHAR *Snummask(double Numer,SINT numcif,SINT dec,SINT sep,SINT segno)
{
 _stprintf(Num,TEXT("%lf"),Numer);
 return nummask(Num,numcif,dec,sep,segno);
}

double Ddecfix(double Numero,SINT dec)
 {
	TCHAR *p;
	_stprintf(Num,TEXT("%lf"),Numero);
	if (_tcslen(Num)>(sizeof(Num)-1)) PRG_end(TEXT("Ddecfix"));
	p=_tcsstr(Num,TEXT("."));
	p+=dec+1; *p=0;
	return atof(Num);
 }

void CharToUnicode(wchar_t *lpDest,CHAR *lpSource)
{
	MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,
						lpSource,strlen(lpSource)+1,
						lpDest,(strlen(lpSource)+1)*2);
}	

void UnicodeToChar(CHAR *lpDest,wchar_t *lpwcSource)
{
	SINT iLen;
	iLen=wcslen(lpwcSource);
	WideCharToMultiByte(CP_ACP,
						WC_COMPOSITECHECK,
						lpwcSource,iLen,
						lpDest,wcslen(lpwcSource),
						NULL,NULL);
	lpDest[iLen]=0;
}	

HWND WindowNow(void)
{
	return GetFocus();
}

void dispx(TCHAR *Mess,...)
{	
	va_list Ah;
	TCHAR *lpBuf;
	HDC hDC;
	HGDIOBJ hFont,hFontOld;
	if (!Mess) return;

	lpBuf=malloc(2000);
	va_start(Ah,Mess);
	vswprintf(lpBuf,Mess,Ah); // Messaggio finale

	hDC=GetDC(NULL);
	hFont=GetStockObject(SYSTEM_FONT);

	hFontOld = SelectObject(hDC, hFont);
	SetBkMode(hDC,OPAQUE);
	SetBkColor(hDC,RGB(0,0,0));
	SetTextColor(hDC,RGB(255,255,255));
	ExtTextOut(hDC, 0, 0, 0,NULL, lpBuf,wcslen(lpBuf),NULL);
	SelectObject(hDC, hFontOld);
    ReleaseDC(NULL,hDC);
	va_end(Ah);
	free(lpBuf);
}

void dispxEx(SINT x,SINT y,TCHAR *Mess,...)
{	
	va_list Ah;
	TCHAR *lpBuf;
	HDC hDC;
	HGDIOBJ hFont,hFontOld;
	if (!Mess) return;

	lpBuf=malloc(2000);
	va_start(Ah,Mess);
	vswprintf(lpBuf,Mess,Ah); // Messaggio finale

	hDC=GetDC(NULL);
	hFont=GetStockObject(SYSTEM_FONT);

	hFontOld = SelectObject(hDC, hFont);
	SetBkMode(hDC,OPAQUE);
	SetBkColor(hDC,RGB(0,0,0));
	SetTextColor(hDC,RGB(255,255,255));
	ExtTextOut(hDC, x, y, 0,NULL, lpBuf,wcslen(lpBuf),NULL);
	SelectObject(hDC, hFontOld);
    ReleaseDC(NULL,hDC);
	va_end(Ah);
	free(lpBuf);
}

// Toglie dei caratteri da una stringa (max 250) new 2004
TCHAR *ChrOmit(TCHAR *lpSource,TCHAR *lpChar)
{
	static TCHAR szServ[250];
	TCHAR *p=szServ;

	for (;*lpSource;lpSource++)
	{
		if (strchr(lpChar,*lpSource)) continue;
		*p++=*lpSource;
	}
	*p=0;
	return szServ;
}

// Tiene solo dei caratteri da una stringa (max 250) new 2004
CHAR *ChrKeep(CHAR *lpSource,CHAR *lpChar)
{
	static CHAR szServ[250];
	CHAR *p=szServ;

	for (;*lpSource;lpSource++)
	{
		if (!strchr(lpChar,*lpSource)) continue;
		*p++=*lpSource;
	}
	*p=0;
	return szServ;
}

// Toglie gli spazi davanti quelli dietro e quelli oltre i due
CHAR *ChrSpaceCut(CHAR *lpSource)
{
	static CHAR szServ[250];
	SINT a;
	BYTE *p=szServ;

	// Spazi davanti
	for (;*lpSource&&*lpSource==' ';lpSource++);

	// Spazi dietro
	NoSpaceCue(lpSource);
	a=0;
	for (;*lpSource;lpSource++)
	{
		if (*lpSource==' ') a++; else a=0;
		if (a>1) continue;
		//if (!strchr(lpChar,*lpSource)) continue;
		*p++=*lpSource;
	}
	*p=0;
	return szServ;
}
