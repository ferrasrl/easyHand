// -------------------------------------------
// ณ EHConsole                               ณ
// ณ EasyHand leggero per l'uso in consolle  ณ
// ณ                                         ณ
// ณ                                         ณ
// ณCreato da: G.Tassistro  by Ferr… SD '999 ณ
// -------------------------------------------

#include "\ehtool\include\ehsw_i.h"
#include <winnls.h>
#include <time.h>
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
 static SINT uACP=0;

#define MAX_FUNC_EXIT 20
 static void *funcExit[MAX_FUNC_EXIT]={0,0,0};

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
	LONG	Valore;
	FILE	*ch;
	CHAR	Campo1[20];
	CHAR	buf[250];
	SINT	Ristart;
	CHAR	*p1,*p2;

	LONG	pos_font;
	SINT	led=OFF;
	SINT	win_ora=OFF;
	SINT	show=ON;
	WORD	Flag;
	SINT    a,ct;

	PtJobber="PRG_start";
	if (EHPower) 
	{
		printf("PRG_start(): Invocato due volte");
		return FALSE;
	}
	*dir_bak=0;

	//printf("QUI"); return FALSE;
	//memset(&sys,0,sizeof(sys));

	ZeroFill(sys);
	
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

	EHPath("#START#");
	ZeroFill(funcExit);

    if ((ch=fopen(EHPath("EH3.INI"),"r"))==NULL) 
		{win_infoarg("EH3.INI [%s][%d]",EHPath("EH3.INI"),GetLastError()); 
	     exit(2);
		}
	
	ct=0;
	for (;ch!=NULL;)
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
					 _strupr(MEMO_path);
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
					 _strupr(FONT_name);
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

	 if (!strcmp(Campo1,"ehshow"))
					{if (Flag==NOGOOD) P_errore(ct,buf);
					 show=Flag; continue;}

	 }
	if (ch) fclose(ch);

	// ------------------------------------------
	// ------------------------------------------
	// INIZIO PROGRAMMAZIONE AMBIENTE
	// ------------------------------------------
	// ------------------------------------------

	Ristart=OFF;

	if (FunzForStart!=NULL) (*FunzForStart)(WS_PROCESS);

	//  Apro il gestore della memoria 
	a=memo_start();
	if (a) 
	{
		printf("Errore MEMO_START %hd");
		return FALSE;
	}

	// da Fare
	//if (com_start())  PRG_end("No memory:COM manager");

#ifdef _EH_PRN_MANAGER
	lpt_start();
#endif

	// Guarda funzioni aggiuntive di preprogrammazione
	FunzForEnd=NULL;
	if (FunzForStart!=NULL) (*FunzForStart)(WS_LAST);
	FunzForEnd=Funz2;

	// ณ 			 BACKUP DIRECTORY DI LANCIO       ณ
	_getcwd(dir_bak,MAXDIR);

	EHPower=ON; // EasyHand Pronto
	LoadEH3();

	uACP = GetACP();

//    InitCommonControls();
 return Ristart;
}
 
// -------------------------------------------
// | PRG_end       Fine del programma        |
// -------------------------------------------

void EM_AddFunc(void (*Func)(BOOL))
{
	SINT a;
	for (a=0;a<MAX_FUNC_EXIT;a++)
	{
		if (!funcExit[a]) {funcExit[a]=Func; return;}
	}
	PRG_end("EM_AddFunc(): overload");
}

void EM_DelFunc(void (*Func)(BOOL))
{
	SINT a;
	for (a=0;a<MAX_FUNC_EXIT;a++)
	{
		if (funcExit[a]==Func) {funcExit[a]=NULL; return;}
	}
	PRG_end("EM_DelFunc(): notfound ");
}

void EM_FuncCall(BOOL fExitOnError)
{
	SINT a;
	void (*Func)(BOOL);
	for (a=0;a<MAX_FUNC_EXIT;a++)
	{
		if (funcExit[a]) 
		{
			Func=funcExit[a]; (*Func)(fExitOnError);
		}
	}
	ZeroFill(funcExit);
}

void PRG_end(CHAR *Mess,...)
{
	BOOL fNoExitProcess=FALSE;
	va_list Ah;
	va_start(Ah,Mess);

	if (*Mess=='>') {fNoExitProcess=TRUE; *Mess=0;}

	if (*Mess)
	{
		CHAR *Buffer=EhAlloc(2000);

#ifdef _HTMLHEADER
		printf("Content-Type: text/html\n\n"); fflush(stdout);
#endif
		sys_rapport(Mess,Ah);

		printf("<br />"CRLF"EasyHand Internet Application:<BR>"CRLF"Errore in procedure:<BR>"CRLF);
		vsprintf(Buffer,Mess,Ah); // Messaggio finale
		
		printf("[%s]\n\r",Buffer);
		if (stdout) fflush(stdout);
		if (sys.chLog) EhLogWrite("%s",Buffer);
		EhFree(Buffer);
	}

	if (FunzForEnd!=NULL) (*FunzForEnd)(); // Chiusura Drivers Esterni
	EM_FuncCall(*Mess?TRUE:FALSE);

	// Libera il gestore della memoria
	//if (memo_end()==-2) win_info("Attenzione:LMO in sospeso\n\7");
	memo_end();

#ifdef _EH_PRN_MANAGER
	 lpt_end();
#endif
	 
//	 if (*dir_bak>0) chdir(dir_bak);
	 if (sys.chLog) {fclose(sys.chLog); sys.chLog=NULL;}
	  
	 _fcloseall();

	 if (Mess==NULL) // Usato per le DLL
	 {
		va_end(Ah);
		return;
	 }

	if (!*Mess)
	{
		va_end(Ah);
		if (fNoExitProcess) return;

#ifdef _EXITWIN
		if (stdout) fflush(stdout);
		ExitProcess(0); 
#else
		if (stdout) fflush(stdout);
		exit(0); 

#endif
	}
	else
	{
		va_end(Ah);
		if (fNoExitProcess) return;

#ifdef _PRGENDKEY
		_getch();
#endif

#ifdef _DEBUG
			getch(); 
#endif

#ifdef _EXITWIN
		if (stdout) fflush(stdout);
		ExitProcess(1); 

#else
		if (stdout) fflush(stdout);
		exit(1);
#endif
	}
}


// -----------------------------------------------------
// Cancella i file di una directory o percorso
// esempio file_delete("c:\*.*");
//
SINT file_delete(TCHAR *lpPath,
				 TCHAR *lpDate) // NULL=Tutto 
							   // aaaammgg cancella i file fino <= data indicata (compresa)
							   // Quindi i pi๙ vecchi
{
	DRVMEMOINFO FDInfo=DMIRESET;
	struct _finddata_t ffbk;
	TCHAR szPercorso[600];
	TCHAR szDest[600];
	CHAR szDate[600];
	SINT a;
	LONG lFind;
	struct tm *gtm;
	SINT iCount;

	// ----------------------------------------------------
	// Cancello tutti i file della directory temporanea
    // ----------------------------------------------------
#ifdef UNICODE
	wcscpy(szPercorso,file_path(lpPath)); 
#else
	strcpy(szPercorso,file_path(lpPath)); 
#endif
	AddBs(szPercorso);
	DMIOpen(&FDInfo,RAM_AUTO,1000,sizeof(szDest),"FD");
	lFind=_findfirst(lpPath, &ffbk);
	if (lFind!=-1)
	{
		do
		{
			if (ffbk.attrib&_A_SUBDIR) continue;
	
			if (lpDate) 
			{
				gtm=gmtime(&ffbk.time_write);
				sprintf(szDate,"%04d%02d%02d",
						gtm->tm_year+1900,
						gtm->tm_mon+1,
						gtm->tm_mday);
				if (strcmp(szDate,lpDate)>0) continue; // Da tenere
			}
	
#ifdef UNICODE
			wprintf(szDest,TEXT("%s%s"),szPercorso,ffbk.name);
#else
			sprintf(szDest,"%s%s",szPercorso,ffbk.name);
#endif
			//printf("[%s] %s" CRLF,szDest,szDate);
			DMIAppendDyn(&FDInfo,szDest);
		} while(_findnext(lFind, &ffbk )==0);
	}		 
	_findclose( lFind );
	iCount=FDInfo.Num;
	for (a=0;a<FDInfo.Num;a++)
	{
		DMIRead(&FDInfo,a,szDest);
#ifdef UNICODE
		_wremove(szDest);
#else
		remove(szDest);
#endif
	}
	DMIClose(&FDInfo,"FD");
	return iCount;
}


/*
// -----------------------------------------------------
// Cancella i file di una directory o percorso
//
//
void file_delete(CHAR *lpPath,
				 CHAR *lpDate) // NULL=Tutto 
							   // aaaammgg cancella i file fino <= data indicata (compresa)
							   // Quindi i pi๙ vecchi
{
	DRVMEMOINFO FDInfo=DMIRESET;
	FFBLK Fblk;
	CHAR szPercorso[MAXPATH];
	CHAR szDest[MAXPATH];
	CHAR szDate[200];
	SINT a;

	// ----------------------------------------------------
	// Cancello tutti i file della directory temporanea
    // ----------------------------------------------------
	strcpy(szPercorso,file_path(lpPath)); AddBs(szPercorso);
	os_errset(OFF);
	DMIOpen(&FDInfo,RAM_AUTO,1000,sizeof(Fblk),"FD");

	lFind=_findfirst(szFileScan, &ffbk);
	if (lFind!=-1)
	{
//	if (!_findfirst(lpPath,&Fblk,FA_ARCH)) {
		do
		{
			if (Fblk.ff_attrib==_A_SUBDIR) continue;
			sprintf(szDest,"%s%s",szPercorso,Fblk.ff_name);
			if (lpDate) {if (strcmp(Fblk.ff_date,lpDate)>0) continue;}
			DMIAppendDyn(&FDInfo,szDest);
		} while (!f_findNext(&Fblk));
	}
	f_findClose(&Fblk);
	os_errset(POP);

	for (a=0;a<FDInfo.Num;a++)
	{
		DMIRead(&FDInfo,a,szDest);
		remove(szDest);
	}
	DMIClose(&FDInfo,"FD");
}
*/

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
	_strlwr(buf); p=buf;

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
	{//sprintf(buf,"LoadEH3(): %s",sys_errlist[_doserrno]);
	 //PRG_end(buf);
	 return;
	}

 fseek(ch,0,SEEK_SET);
 EH3_line=0;

 // Conta le linee
 while (TRUE)
 {
	if (fgets(buf,EH3BUF,ch)==NULL) break;
	EH3_line++;
 }

 EH3_hdl=memo_chiedi(RAM_AUTO,(LONG) EH3_line*EH3BUF,"EH3.INI");
 if (EH3_hdl<0) {fclose(ch); PRG_end("Non c'่ memo per EH3");}

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
 //if (EH3_hdl<0) return rt;

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

 while (TRUE)
 {
	if (fgets(buf,sizeof(buf),ch)==NULL) break;

	 ///		Tolgo CR O LF
	 //for (a=0;a<strlen(buf);a++) if ((buf[a]=='\n')||(buf[a]=='\r')) buf[a]=0;
	 ChrNullCue(buf);

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



void win_memoerr(CHAR *buf)
{
#ifdef _NODC
  EhLogWrite("Errore memoria\n%s",buf);
#else
  MessageBox(NULL,buf,TEXT("EH:Gestore memoria"),MB_ICONHAND|MB_OK);
#endif

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
	 if (ct++>nkey) {PRG_end("Errore in far_bsearch");}
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
 sys.MEMO_inizio=GetFreeSpace(0);
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

/*
SINT memo_end(void)
{
	SINT a;
	if (sys.memocont==0) return -1;

	MemoLiberaControl=OFF;
	for (a=0;a<sys.memomaxi;a++)
			{memo_libera(a,sys.memolist[a].User);}

	if (sys.memolist!=NULL) farfree(sys.memolist); // Libero memoria usata
	return 0;
}
*/


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
 if (sys.memocont>=sys.memomaxi) PRG_end("Gestore memorie esaurito");

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
			lpvBuffer=EhAlloc(sizebyte);
			if (lpvBuffer==NULL) {//GlobalFree(hglb);
		                      win_info("malloc=NULL");
							  hdl=-1;
							  break;
							 }
			//lpvBuffer=EhAlloc(sizebyte);
			//if (lpvBuffer==NULL) {hdl=-1;break;}
			//	Archivia la richiesta
			sys.memocont++;
			sys.memolist[hdl].iTipo  =tipo;
			sys.memolist[hdl].lpvMemo=(void *) lpvBuffer;
			sys.memolist[hdl].dwSize =sizebyte;//GlobalSize(hglb);// Grandezza reale
			sys.memolist[hdl].hGlobal=0;//hglb;
			sys.MEMO_rimasta-=sizebyte;
			break;

		/*
		if ((sizebyte+LIMITEMIN)>sys.MEMO_rimasta) {hdl=-1;break;}

		//		Controlla se c abbastanza memoria
		ptf=farEhAlloc(sizebyte);
		if (ptf==NULL) {hdl=-1;break;}

		//	Archivia la richiesta
		sys.memocont++;
		sys.memolist[hdl].iTipo=RAM_HEAP;
		sys.memolist[hdl].sgm=FP_SEG(ptf);
		sys.memolist[hdl].ofs=FP_OFF(ptf);
		sys.memolist[hdl].ptmemo=ptf;
		sys.memolist[hdl].dwSize=sizebyte;
		sys.memolist[hdl].hGlobal=0;
		sys.MEMO_rimasta-=sizebyte;
		break;
*/

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
				 win_infoarg("memo_libera(): Tentativo di accesso a\n[%s] da parte di [%s]",
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
		if (sys.memolist[hdl].lpvMemo==NULL) {win_info("memo_libera: Heap2"); break;}
		EhFree(sys.memolist[hdl].lpvMemo); 
		sys.memolist[hdl].lpvMemo=NULL;
        //EhFree(sys.memolist[hdl].lpvMemo);
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

		if (GlobalFree(sys.memolist[hdl].hGlobal)!=NULL) win_memoerr("memo_libera():Errore in Globalfree");
		break;

 //	-------------------------------------------
 //		RAM_HDLFREE : memoria libera				    !
 // -------------------------------------------

		case RAM_HDLFREE :

		if (MemoLiberaControl)
		 {
			CHAR serv[80];
			sprintf(serv,"HDLFREE: Hdl=%d %s",hdl,chisei);
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
		 {CHAR serv[50];
		  sprintf(serv,"MemoLibera:\n[Flag %d] Hdl=%d\n|%s|",flag,hdl,chisei);
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
	{win_memoerr("memo_scrivivar:Size o Dest errato");
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
		  if (Destinazione==NULL) win_memoerr("memo_scrivivar:Errore in GlobaLock");

		  Destinazione+=dest;
		  memcpy(Destinazione, Source, numbyte);
		  GlobalUnlock(sys.memolist[memo_hdl].hGlobal);
		  break;

	}
	return 0;
 }

 /*
SINT  memo_scrivivar(SINT memo_hdl,
					 LONG dest,
                     void far *pt,
                     LONG numbyte)
{
 return memo_scrivi(memo_hdl,dest,FP_SEG(pt),FP_OFF(pt),numbyte);
}

 SINT memo_scrivi(SINT memo_hdl,
								 LONG dest,
								 WORD sgm,
								 WORD off,
								 LONG numbyte)
{
	struct D_MEMO dmem;

	dmem.hdl=memo_hdl;
	dmem.dest=dest;
	dmem.sgm=sgm;
	dmem.off=off;
	dmem.numbyte=numbyte;

	DRV->command=MEMO_SCRIVI;
	DRV->in=&dmem;
	(*DRV_monitor)();

 if ((DRV->risposta)||(dest<0))
	 {CHAR serv[100];
		sprintf(serv,"MWrite:%d \n(DOS %d-%s) \nMemo: %s\nofs=%ld size=%ld",
						 DRV->risposta,DE_coden,DE_code,
//						 sys.memolist[memo_hdl].user,
						 sys.memolist[memo_hdl].User,
						 dest,numbyte);
		win_errgrave(serv);
	 }

 return DRV->risposta;
}
*/


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
  CHAR buf[80];
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
		  if (Source==NULL) win_memoerr("memo_leggivar:Errore in GlobaLock");

		  Source+=sorg;
		  memcpy(Destinazione, Source, numbyte);
		  GlobalUnlock(sys.memolist[memo_hdl].hGlobal);

		break;
	 }

 FINE:

 if (FlagOut)
	 {sprintf(buf,"memo_leggivar(): Hdl=%d Flag=%d",memo_hdl,FlagOut);
		win_memoerr(buf);
	 }

 return FlagOut;
}



/*
SINT	far memo_leggivar(SINT memo_hdl,
											LONG sorg,
											void far *pt,
											LONG numbyte)
{
 return memo_leggi(memo_hdl,sorg,FP_SEG(pt),FP_OFF(pt),numbyte);
}

SINT	far memo_leggi(SINT memo_hdl,
									 LONG sorg,
									 WORD sgm,
									 WORD off,
									 LONG numbyte)
{
	static struct D_MEMO mem;

	mem.hdl=memo_hdl;
	mem.dest=sorg;
	mem.sgm=sgm;
	mem.off=off;
	mem.numbyte=numbyte;

//	if (numbyte==302) {printf("[%d]",memo_hdl);sonic(1000,1,2,3,4,5);}
	DRV->command=MEMO_LEGGI;
	DRV->in=&mem;
	(*DRV_monitor)();

	if ((DRV->risposta)||(sorg<0))
	 {//CHAR serv[100];
		PRG_end("MRead:%d \n(DOS %d-%s) \nUMemo:[%d] %s\nofs=%ld size=%ld",
						 DRV->risposta,DE_coden,DE_code,
//						 memo_hdl,sys.memolist[memo_hdl].user,
						 memo_hdl,sys.memolist[memo_hdl].User,
						 sorg,numbyte);
		//win_errgrave(serv);
	 }


 return DRV->risposta;
}
*/




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
/*
SINT memo_info(SINT hdl,SINT *tipo,WORD *sgm)
{
 if ((hdl<0)||(hdl>(sys.memomaxi-1))) return -2;
 if (sys.memolist[hdl].iTipo==RAM_HDLFREE) return -1;
 *tipo=sys.memolist[hdl].iTipo;
 *sgm=sys.memolist[hdl].sgm;
 return 0;
}
*/
SINT memo_info(SINT hdl,SINT *tipo,WORD *sgm)
{
 if ((hdl<0)||(hdl>(sys.memomaxi-1))) return -2;
 if (sys.memolist[hdl].iTipo==RAM_HDLFREE) return -1;
 *tipo=sys.memolist[hdl].iTipo;
 if (sgm) *sgm=0;//sys.memolist[hdl].sgm;
 return 0;
}

// -------------------------------------------
// | MEMO_TIPO Ritorna il tipo di memoria 	 |
// |           Assegnato al handle           |
// |                                         |
// | hdl   = Handle interessato              |
// |                                         |
// | Ritorna 0 se l'handle e valido   		 |
// | oppure :        						 |
// |                 						 |
// | -1 = Handle free (Non valido)           |
// | -2 = Handle errato                      |
// |    									 |
// -------------------------------------------

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
/*
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
	 win_infoarg("memo_heap ?: [%d][%s]",hdl,(hdl>-1) ? sys.memolist[hdl].User : "?");
	 PRG_end("memo_heap ?: [%d][%s]",hdl,(hdl>-1) ? sys.memolist[hdl].User : "?");
	 return NULL;
	}
 
 return sys.memolist[hdl].lpvMemo;
}
*/
void * memo_heap(SINT hdl)
{
 BOOL fError=FALSE;
 SINT iTipo;
 
 if (sys.fInternalError) return NULL;

 if (hdl<0) fError=TRUE;
 iTipo=sys.memolist[hdl].iTipo;
 if (!fError) 
 {
	 if ((iTipo!=RAM_HEAP)&&(iTipo!=RAM_GLOBALHEAP)) fError=TRUE;
 }
 if (fError)
	{
	 //sprintf(buf,"memo_heap ?: [%d][%s]",hdl,sys.memolist[hdl].User);
	 //win_errgrave(buf);
	 sys.fInternalError=TRUE;
	 win_infoarg("memo_heap ?: [%d][%s]",hdl,(hdl>-1) ? sys.memolist[hdl].User : "?");
	 PRG_end("memo_heap ?: [%d][%s]",hdl,(hdl>-1) ? sys.memolist[hdl].User : "?");
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
CHAR *memo_name(SINT hdl)
{
 if (hdl<0) return NULL;
 return sys.memolist[hdl].User;
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

SINT memo_copyall(SINT hdlSource,SINT hdl2)
{
 SINT a;
 SINT tipo1,tipo2;
 DWORD size;
 SINT FlagSorg=OFF;
 SINT FlagDest=OFF;
 CHAR *Sorg=NULL;
 CHAR *Dest=NULL;

 if (sys.fInternalError) return 0;
 //		Pre-controlli
 if (hdlSource==hdl2) {a=-4;goto FINE;}
 if ((hdlSource<0)||(hdlSource>(sys.memomaxi-1))) {a=-2;goto FINE;}
 if ((hdl2<0)||(hdl2>(sys.memomaxi-1))) {a=-2;goto FINE;}
 tipo1=sys.memolist[hdlSource].iTipo; tipo2=sys.memolist[hdl2].iTipo;
 if ((tipo1==RAM_HDLFREE)||(tipo2==RAM_HDLFREE)) {a=-1;goto FINE;}

 //	Dimenzioni adeguate
 size=sys.memolist[hdlSource].dwSize;
// if (size!=sys.memolist[hdl2].dwSize) {a=-3;goto FINE;}
 if (size>sys.memolist[hdl2].dwSize) {a=-3;goto FINE;}

 // Controllo se sono Heap

 if ((sys.memolist[hdlSource].iTipo!=RAM_HEAP)&&(sys.memolist[hdlSource].iTipo!=RAM_GLOBALHEAP))
		{Sorg=GlobalLock(sys.memolist[hdlSource].hGlobal);
		 if (Sorg==NULL) {win_memoerr("copyall:Errore in LOCK hld1");
						  a=-10;goto FINE;
						}
		 FlagSorg=ON;
		 } else Sorg=sys.memolist[hdlSource].lpvMemo;

 if ((sys.memolist[hdl2].iTipo!=RAM_HEAP)&&(sys.memolist[hdl2].iTipo!=RAM_GLOBALHEAP))
		{Dest=GlobalLock(sys.memolist[hdl2].hGlobal);
		 if (Dest==NULL) {win_memoerr("copyall:Errore in LOCK hld2");
											GlobalUnlock(sys.memolist[hdlSource].hGlobal);
											a=-11;goto FINE;
											}
		 FlagDest=ON;
		 } else Dest=sys.memolist[hdl2].lpvMemo;

		//memcpy(Dest, Sorg, sys.memolist[hdlSource].dwSize);
		Ehmemcpy(Dest, Sorg, sys.memolist[hdlSource].dwSize);


 if (FlagSorg) GlobalUnlock(sys.memolist[hdlSource].hGlobal);
 if (FlagDest) GlobalUnlock(sys.memolist[hdl2].hGlobal);

 FINE:
 return a;
}

static CHAR *LGetMemoType(SINT iType)
{
	switch (iType)
	{
	 case RAM_HDLFREE:  return "RAM_HDLFREE";
	 case RAM_HEAP: return "RAM_HEAP";
	 case RAM_GLOBALHEAP: return "RAM_GLOBALHEAP";
	 case RAM_BASSA: return "RAM_BASSA";
	 //case RAM_ALTA: return "RAM_ALTA";
	 case RAM_AUTO: return "RAM_AUTO";
	 case RAM_XMS: return "RAM_XMS";
	 case RAM_DISK: return "RAM_DISK";
	}
	return "?";
}

/*
// Solo Windows
void *Wmemo_lock(SINT memo_hdl)
{
  if (sys.fInternalError) return NULL;
  if (sys.memolist[memo_hdl].iTipo!=RAM_BASSA) 
  {
	 sys.fInternalError=TRUE;
	 PRG_end("Wmemo_lock(%d)",memo_hdl);
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
	PRG_end("Wmemo_unlock %d",memo_hdl);
  }

  if (sys.memolist[memo_hdl].iTipo!=RAM_GLOBALHEAP) 
  {
	  sys.fInternalError=TRUE;
	  PRG_end("Wmemo_unlock %d (%d-%s)",
	          sys.memolist[memo_hdl].iTipo,
			  memo_hdl,
			  sys.memolist[memo_hdl].User);
  }

  sys.memolist[memo_hdl].iTipo=RAM_BASSA; 
  sys.memolist[memo_hdl].lpvMemo=NULL;
  GlobalUnlock(sys.memolist[memo_hdl].hGlobal);

}
*/
void * EhReAlloc(void *ptr,SINT iOldMemo,SINT iNewMemo) 
	{
		CHAR *p;
		p=EhAlloc(iNewMemo);
		if (!p) win_infoarg("EhReAlloc error:[%d:%d]",GetLastError(),iNewMemo);
		memcpy(p,ptr,iOldMemo);
		EhFree(ptr);
		return p;
	}

void * EhAlloc(SINT iMemo) {return GlobalAlloc(GPTR,iMemo);}
void EhFree(void *ptr) {GlobalFree(ptr);}
void EhFreePtr(void **ptr) {if (*ptr) {EhFree(*ptr); *ptr=NULL;}}
void *Wmemo_lock(SINT memo_hdl)
{
	return Wmemo_lockEx(memo_hdl,"?");
}

void *Wmemo_lockEx(SINT memo_hdl,CHAR *lpWho)
{
  if (sys.fInternalError) return NULL;
  if (sys.memolist[memo_hdl].iTipo!=RAM_BASSA) 
  {
	 sys.fInternalError=TRUE;
	 PRG_end("Wmemo_lock hdl=%d (%s); (User:%s); Who:%s",
			  memo_hdl,
			  LGetMemoType(sys.memolist[memo_hdl].iTipo),
	          //sys.memolist[memo_hdl].iTipo,
			  sys.memolist[memo_hdl].User,
			  lpWho);
  }
#ifdef EHMULTI_THREAD
	EnterCriticalSection(&csMemory);
#endif

  sys.memolist[memo_hdl].iTipo=RAM_GLOBALHEAP; 
  sys.memolist[memo_hdl].lpvMemo=GlobalLock(sys.memolist[memo_hdl].hGlobal);

#ifdef EHMULTI_THREAD
	LeaveCriticalSection(&csMemory);
#endif
  return sys.memolist[memo_hdl].lpvMemo;
}

void Wmemo_unlock(SINT memo_hdl)
{
	Wmemo_unlockEx(memo_hdl,"?");
}
void Wmemo_unlockEx(SINT memo_hdl,CHAR *lpWho)
{
 if (sys.fInternalError) return;
 if (memo_hdl<0)
  {
	sys.fInternalError=TRUE;
	PRG_end("Wmemo_unlock %d [%s]",memo_hdl,lpWho);
  }
#ifdef EHMULTI_THREAD
	EnterCriticalSection(&csMemory);
#endif

  if (sys.memolist[memo_hdl].iTipo!=RAM_GLOBALHEAP) 
  {
	  sys.fInternalError=TRUE;
	  PRG_end("Wmemo_unlock [%s (%d)] (%d-%s) [%s]",
			  LGetMemoType(sys.memolist[memo_hdl].iTipo),
	          sys.memolist[memo_hdl].iTipo,
			  memo_hdl,
			  sys.memolist[memo_hdl].User,
			  lpWho);
  }

  sys.memolist[memo_hdl].iTipo=RAM_BASSA; 
  sys.memolist[memo_hdl].lpvMemo=NULL;
  GlobalUnlock(sys.memolist[memo_hdl].hGlobal);

#ifdef EHMULTI_THREAD
	LeaveCriticalSection(&csMemory);
#endif

}

void CtrlFree(void *ptr)
{
 SINT a;
 for (a=0;a<sys.memomaxi;a++) 
 {
   if ((sys.memolist[a].iTipo==RAM_HEAP)&&(sys.memolist[a].lpvMemo==ptr))
   {
     win_infoarg("CtrlFree error %d [%x]",a,sys.memolist[a].lpvMemo);
   }
 }
 EhFree(ptr);
}

// ---------------------------------------------------------------------------------------------------------------------
// FLM_WIN Gestione delle windows             
//                                            
//																			Creato da: G.Tassistro  by Ferr… SD '995/6 |
// ---------------------------------------------------------------------------------------------------------------------


void win_info(CHAR info[])
{
	win_infoarg(info);
}

void win_errgrave(CHAR info[])
{
	win_infoarg("ERRORE GRAVE:\n%s",info);
	PRG_end(info);
}

void win_err(CHAR info[])
{
	win_infoarg("ERRORE:\n%s",info);
}

#ifdef _NODC
void win_infoarg(CHAR *Mess,...)
{
	va_list Ah;
	//CHAR Buffer[2000];
	CHAR *lpBuf;
	lpBuf=EhAlloc(2000);

	va_start(Ah,Mess);
	//vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	vprintf(Mess,Ah); // Messaggio finale
	printf("\n");
	EhFree(lpBuf);
	va_end(Ah);
}
#else
void win_infoarg(CHAR *Mess,...)
{
	va_list Ah;
	//CHAR Buffer[2000];
	CHAR *lpBuf;
	lpBuf=EhAlloc(64000);

	if (Mess) 
	{
	 va_start(Ah,Mess);
	 vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	 MessageBox(NULL,lpBuf,"Info",MB_OK);
	 va_end(Ah);
	}
	EhFree(lpBuf);
}

#endif

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
SINT f_open(CHAR *nome,CHAR *tipo,FILE **ch)
{
	 FILE *kk;
	 SINT a;
	 static CHAR serv[250];

	 a=0; 
	 for (;;)
		{kk=fopen(nome,tipo); if (kk!=NULL) {a=0;*ch=kk;break;}
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
	 if (!ch) return 0;
	 a=fclose(ch);
	 /*
	 for (;;)
		{a=fclose(ch); if (a==0) break;
		 a=os_error("Chiusura file; \n"); if (a!=RIPROVA) break;
		}
*/
	 //if (!a) 
	 FILE_aperti--;
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
SINT f_exist(CHAR *nome)
{
	 SINT a;
	 static CHAR serv[250];

	 a=0;
	 for (;;)
		{
		 os_errset(OFF);
		 a=_access(nome,0); // Guardo se esiste
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
}

SINT f_existw(WCHAR *nome)
{
	 SINT a;
	 static CHAR serv[250];

	 a=0;
	 for (;;)
		{
		 os_errset(OFF);
		 a=_waccess(nome,0); // Guardo se esiste
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
}

// --------------------------------------------
//	ณ F_GETDRIVE Legge il drive corrente      ณ
//	ณ            percorso completo            ณ
//	ณ                                         ณ
//	ณ                           by Ferr… 1999 ณ
// --------------------------------------------
SINT f_getdrive(void)
{
 return _getdrive();
}

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

LONG file_len(CHAR *file)
{
 FILE *fp;
 LONG len;
 SINT a;

 a=f_open(file,"rb",&fp); if (a) return a;

 fseek(fp,0,SEEK_END);
 len=ftell(fp);
 f_close(fp);

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
SINT file_load(CHAR *file,SINT tipo)
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
 hdl=memo_chiedi(tipo,lenfile+1,file_name(file));
 if (hdl==-1) return -2; // Non c memoria disponibile

	//dwSize=file_len(lpFileName);
	//iRealSize=dwSize+sizeof(S_TXB)+1;
	//hdl=memo_chiedi(RAM_AUTO,iRealSize,file_name(lpFileName));
	//if (hdl<0) {f_close(ch); return NULL;}
 if (f_open(file,"rb",&fp))
		{memo_libera(hdl,"file_load4"); return -5;}

 if (tipo==RAM_AUTO)
 {
	lpb=Wmemo_lock(hdl);
 }
 else lpb=memo_heap(hdl);

 memset(lpb,0,lenfile+1);
 fread(lpb,lenfile,1,fp);
 //lpb[lenfile]=0;

 if (tipo==RAM_AUTO)
 {
	Wmemo_unlock(hdl);
 }
 f_close(fp);

 return hdl;
}


/*
SINT file_load(CHAR *file,SINT tipo)
{
 FILE *fp;
 LONG lenfile;
 CHAR *ptf;
 SINT a,err,hdl;
 SINT serv;
 WORD block;
 LONG block2; // Trasferimento a 32000 byte per volta
 LONG ptrmemo;
 CHAR memo[256];

 lenfile=file_len(file); if (lenfile<0) return (SINT) lenfile; // Il file non esiste

 sprintf(memo,"F:%s",file_name(file));

 //																						!
 //		CHIEDE MEMORIA AL SISTEMA PER IL FILE		!
 //																						!

 hdl=memo_chiedi(tipo,lenfile,file_name(file));
 if (hdl==-1) return -2; // Non c memoria disponibile

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
 f_close(fp);
 return hdl;
}
*/

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
CHAR *file_name(CHAR *file)
{
 SINT a;
 CHAR *p=file; // era Huge
 p+=strlen(file)-1;
 for (a=strlen(file);a>1;a--,p--) {if ((*p=='\\')||(*p==':')) {p++;break;}}
 return p;
}
SINT TempFileName(CHAR *lpFolder, // Cartella dove crearlo
				  CHAR *lpPrefix,
				  CHAR *lpFileName) // Buffer
{
	BOOL fFree=FALSE;
	SINT iErr;
	if (!lpFolder) {lpFolder=EhAlloc(1024); GetTempPath(1024,lpFolder); fFree=TRUE;}
	iErr=GetTempFileName(lpFolder,lpPrefix,0,lpFileName);
	if (fFree) EhFree(lpFolder);
	return iErr;
}

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

// ีอออออออออออออออออออออออออออออออออออออออออธ
// ณ os_ERROR Scrive nell'area rapport       ณ
// ณ           la descrizione degli errori   ณ
// ณ                                         ณ
// ณ                              by Ferr… 96ณ
// ิอออออออออออออออออออออออออออออออออออออออออพ
SINT os_error(CHAR *ms1)
{
 SINT code;//,clas,act,locus;
 static CHAR mess[250];
 SINT ritdat;

#ifndef _WIN32
 //		Gestione INT 24
 CHAR *hard_err[]= {
						/*00*/"Protezione da scrittura nell'unit… %c:",
						/*01*/"Unita %c: sconosciuta",
						/*02*/"L'Unit… %c: non  pronta",
						/*03*/"Unknown command",
						/*04*/"Data error (bad CRC)",
						/*05*/"Bad request structure lenght",
						/*06*/"Seek error nel l'unit… %c:",
						/*07*/"Unknow media type in %c:",
						/*08*/"Sector not found in %c:"
						/*09*/"Carta finita",
						/*0A*/"Write fault",
						/*0B*/"Read fault",
						/*0C*/"Fallimento generale verificato sull'unit… %c:",
						/*0D*/"Violazione di sharing nell'unit… %c:",
						/*0E*/"Violazione di lock nell'unit… %c:"
						/*0F*/"Cambio di disco non autorizzato nell'unit… %c:",
						/*10*/"FCB non disponibile sull'unit… %c:",
						/*11*/"Sharing buffer overflow"};

 CHAR *claspt[]= {"Tutto ok",
						/*01*/"Fuori dalle risorse",
						/*02*/"Situazione temporanea",
						/*03*/"Autorizzazione",
						/*04*/"Interno",
						/*05*/"Hardware malfuzionante",
						/*06*/"Sistema DOS malfuzionante",
						/*07*/"Errore nell'applicazione",
						/*08*/"Inesistenti",
						/*09*/"Formato errato",
						/*0A*/"Chiuso (Locked)",
						/*10*/"Media error",
						/*11*/"Esiste gi…",
						/*12*/"Sconosciuto"};

 CHAR *actpt[]= {"Tutto ok",
						/*01*/"Riprova",
						/*02*/"Riprova ritardata",
						/*03*/"Ritorna al sistema",
						/*04*/"Annulla dopo aver cancellato",
						/*05*/"Annullamento immediato",
						/*06*/"Ignora",
						/*07*/"Riprova dopo l'intervento dell'user"};

 CHAR *locpt[]= {"Tutto ok",
						/*01*/"tipo Sconosciuto o non appropriato",
						/*02*/"Blocco di una device",
						/*03*/"Problema di rete",
						/*04*/"Problema sulla porta seriale",
						/*05*/"Problema sulla memoria"};

 struct I24H {
	WORD w_r:1;
	WORD diskarea:2;
	WORD fail:1;
	WORD retry:1;
	WORD ignore:1;
	WORD nousato:1;
	WORD ioerr:1;
	} status;

 CHAR *ptst=(CHAR *) &status;
#endif
 //os_errcode(&code,&clas,&act,&locus);

 code=GetLastError();
 DE_coden=code;

 // Controllo prima dell'attivazione della libreria
 if (EHPower==OFF) {DE_last=DOSERR;return DOSERR;}
 if (DE_flag)
	 {
	  os_errvedi(ms1);
	 }
	 ritdat=DOSERR;
//	}

 if ((DE_flag==ON)&&(ritdat!=RIPROVA)) PRG_end(mess); // Finisce se il flag  a ON

 DE_last=ritdat;
 return ritdat;
}

void os_errvedi(CHAR *ms)
{
 LPVOID lpMsgBuf;
 LPVOID lpMess; 

 FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		       NULL,
               DE_coden,
               MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
               (LPTSTR) &lpMsgBuf,
			   0,
			   NULL);// Display the string.
 
 if (ms==NULL) ms="<NULL>";
 lpMess=EhAlloc(strlen(lpMsgBuf)+strlen(ms)+100);
#ifdef _NODC
 EhLogWrite("%s\n%s",ms,lpMsgBuf);
#else
 sprintf(lpMess,"%s\n%s",ms,lpMsgBuf);
 MessageBox(NULL,lpMess,"Errore di Sistema",MB_OK|MB_ICONINFORMATION);
#endif
 LocalFree(lpMsgBuf);
 CtrlFree(lpMess);
}

TCHAR *os_ErrorToString(SINT iError)
{
 LPVOID lpMsgBuf;
 TCHAR *lp;
 FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		       NULL,
               iError,
               MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), // Default language
               (LPTSTR) &lpMsgBuf,
			   0,
			   NULL);// Display the string.
 
 lp=StringAlloc(lpMsgBuf);  LocalFree(lpMsgBuf);
 return lp;
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
				num++; if ((num<0)||(num>9)) PRG_end("Overflow in os_errset()");
				err[num]=DE_flag;
				DE_flag=tipo;
				break;

	 case POP:
				if (num<0) PRG_end("POP Overflow in os_errset()");
				DE_flag=err[num];
				num--;
				break;

	 default : PRG_end("Tipo errato in os_errset()");
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
 lpMess=EhAlloc(strlen(lpMsgBuf)+strlen(ms)+100);
#ifdef _NODC
 EhLogWrite("%d:%s\n%s",iError,ms,lpMsgBuf);
 printf("%d:%s\n%s",iError,ms,lpMsgBuf); fflush(stdout);
#else
 sprintf(lpMess,"%s\n%s",ms,lpMsgBuf);
 MessageBox(NULL,lpMess,"Errore di Sistema",MB_OK|MB_ICONINFORMATION);
#endif
 LocalFree(lpMsgBuf);
 CtrlFree(lpMess);
}


CHAR *file_path(CHAR *file)
{
 static CHAR Path[MAXPATH];
 CHAR *p;

 strcpy(Path,file);
 p=file_name(Path); if (p) *p=0; 
 return Path;
}

TCHAR *rstrstr(TCHAR *lpString,TCHAR *lpFind)
{
	SINT iFind=strlen(lpFind);
	TCHAR *p=lpString+strlen(lpString)-iFind;
    SINT a;	
	for (a=0;a<(SINT) (strlen(lpString)-iFind);a++)
	{
	  if (!memcmp(p,lpFind,iFind)) return p; else p--;
	}
	return NULL;
}

CHAR *stristr(CHAR *String,CHAR *lpFind)
{
	SINT iFind=strlen(lpFind);
	CHAR *p=String;
	CHAR *pe=p+strlen(String)-iFind;
	for (;p<pe;p++)
	{
	  if (!_memicmp(p,lpFind,iFind)) return p; 
	}
	return NULL;
}

WCHAR *wcsistr(WCHAR *String,WCHAR *pwFind)
{
	SINT iFind=wcslen(pwFind);
	WCHAR *p=String;
	WCHAR *pe=p+wcslen(String)-iFind;
	for (;p<pe;p++)
	{
	  if (!_memicmp(p,pwFind,iFind<<1)) return p; 
	}
	return NULL;
}

void NoSpaceCue(BYTE *lpStr)
{
 SINT a,len;
 if (!*lpStr) return;
 len=strlen(lpStr); lpStr+=len-1;
// for (a=0;a<len;a++,lpStr--) {if ((*lpStr<'!')||(*lpStr>'z')) {*lpStr=0; continue;} else break;}
 for (a=0;a<len;a++,lpStr--) {if (*lpStr==' ') {*lpStr=0; continue;} else break;}
}

void ChrNullCue(TCHAR *lpStr)
{
 SINT a,len;
 if (!*lpStr) return;
 len=strlen(lpStr); lpStr+=len-1;
 for (a=0;a<len;a++,lpStr--) {if (*lpStr<' '&&*lpStr>0) {*lpStr=0; continue;} else break;}
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

//ีออออออออออออออออออออออออออออออออออออออออออออออออธ
//ณ MASK - Formatizza i numeri per la stampa       ณ
//ณ                                                ณ
//ณ *num  = Stringa da Stampare                    ณ
//ณ numcif= Lunghezza in cifre (prima della virgolaณ
//ณ dec   = Numero decimali                        ณ
//ณ sep   = Flag separatori (0=No,1=Si)            ณ
//ณ segno = Flag segno (OFF=Solo -,ON=+&-)         ณ
//ณ                                                ณ
//ณ                                  by Ferr… 1996 ณ
//ิออออออออออออออออออออออออออออออออออออออออออออออออพ
CHAR *nummask(CHAR *num,SINT numcif,SINT dec,SINT sep,SINT segno)
{
	CHAR lensx,lendx,sg=0;
	SINT  ptsx,ptdx,ptnum,a,pt,cnt;
	SINT  len,lnum;
	static CHAR srv[40];
	CHAR *sr=srv;

	srv[0]=0;

	//	PRE-controlli
	if ((lnum=strlen(num))==0) goto errore;
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


 static CHAR Num[80];


// Ritorna una stringa con separazioni ,/.
CHAR *Snummask(double Numer,SINT numcif,SINT dec,SINT sep,SINT segno)
 {
 sprintf(Num,"%lf",Numer);
 return nummask(Num,numcif,dec,sep,segno);
 }

double Ddecfix(double Numero,SINT dec)
 {
	CHAR *p;
	sprintf(Num,"%lf",Numero);
	if (strlen(Num)>(sizeof(Num)-1)) PRG_end("Ddecfix");
	p=strstr(Num,".");
	p+=dec+1; *p=0;
	return atof(Num);
 }

// Attenzione SOURCE deve avere spazio per contenere il nuovo valore
BOOL StrReplace(CHAR *lpSource,CHAR *lpTagFind,CHAR *lpNewValue)
{
 CHAR *p;
 SINT lo,ln,le;
 
 p=strstr(lpSource,lpTagFind); if (!p) return 0;
 lo=strlen(lpTagFind);
 ln=strlen(lpNewValue);
 le=strlen(p+lo)+1;
 memmove(p+ln,p+lo,le);
 memcpy(p,lpNewValue,ln);
 return 1;
}

BOOL StrIReplace(CHAR *lpSource,CHAR *lpTagFind,CHAR *lpNewValue)
{
 CHAR *p;
 SINT lo,ln,le;
 
 p=stristr(lpSource,lpTagFind); if (!p) return 0;
 lo=strlen(lpTagFind);
 ln=strlen(lpNewValue);
 le=strlen(p+lo)+1;
 memmove(p+ln,p+lo,le);
 memcpy(p,lpNewValue,ln);
 return 1;
}

BOOL StrReplaceW(WCHAR *pwcSource,WCHAR *pwcFindString,WCHAR *pwcReplaceString)
{
 wchar_t *pw;
 SINT lo,ln,le;
 
 pw=wcsstr(pwcSource,pwcFindString); if (!pw) return 0;
 lo=wcslen(pwcFindString);	// Lunghezza della find string
 ln=wcslen(pwcReplaceString); // Lunghezza della replace string
 le=wcslen(pw+lo)*2; 
 memmove(pw+ln,pw+lo,le+2);
 memcpy(pw,pwcReplaceString,ln*2);
 return 1;
}

// Toglie dei caratteri da una stringa (max 250) new 2004
CHAR *ChrOmit(CHAR *lpSource,CHAR *lpChar)
{
	static CHAR szServ[1550];
	CHAR *p=szServ;

	if (strlen(lpSource)>(sizeof(szServ)-1)) PRG_end("ChrOmit(): %d > %d",strlen(lpSource),(sizeof(szServ)-1));
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
	static CHAR szServ[1550];
	CHAR *p=szServ;
	if (strlen(lpSource)>(sizeof(szServ)-1)) PRG_end("ChrKeep(): %d > %d",strlen(lpSource),(sizeof(szServ)-1));

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
	static CHAR szServ[550];
	SINT a;
	BYTE *p=szServ;
	if (strlen(lpSource)>(sizeof(szServ)-1)) PRG_end("ChrSpaceCut(): %d > %d",strlen(lpSource),(sizeof(szServ)-1));

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

CHAR *ChrTrim(CHAR *lpSource)
{
	static CHAR szServ[550];
	BYTE *p=szServ;
	if (strlen(lpSource)>(sizeof(szServ)-1)) PRG_end("ChrTrim(): %d > %d",strlen(lpSource),(sizeof(szServ)-1));

	// Spazi davanti
	for (;*lpSource&&*lpSource<'!';lpSource++);

	if (strlen(lpSource)>(sizeof(szServ)-1)) PRG_end("ChrTrim(): %d > %d",strlen(lpSource),(sizeof(szServ)-1));
	// Spazi dietro
	strcpy(szServ,lpSource);
	NoSpaceCue(szServ);
	return szServ;
}

CHAR *ChrToAscii(CHAR *lpSource)
{
	static CHAR szBuffer[1024];
	strcpy(szBuffer,lpSource);
	while (StrReplace(szBuffer,"๙","u"));
	while (StrReplace(szBuffer,"์","i"));
	while (StrReplace(szBuffer,"เ","a"));
	while (StrReplace(szBuffer,"่","e"));
	while (StrReplace(szBuffer,"้","e"));
	while (StrReplace(szBuffer,"๒","o"));
	return szBuffer;
}

LONG ColorGray(SINT Perc)
{
 SINT c;
 // x:255=100-Perc:100
 c=((100-(Perc%101))*255); if (c!=0) c/=100;
 return RGB((BYTE) c, (BYTE) c,(BYTE) c);
}


// --------------------------------------------------------------
// Gestione Stampante
// --------------------------------------------------------------
#ifdef _EH_PRN_MANAGER
void lpt_start(void)
{
 SINT a;
 CHAR printer[30];
 //CHAR tipoprint[60];
 CHAR *tipoprint,*coda;
/*
 //LPT_info[0].attiva=ON;
 strcpy(LPT_info[0].szDirectName,"");
 strcpy(LPT_info[0].szDirectDesc,"");
 LPT_info[0].fDirectRemote=OFF;
 LPT_info[0].fDirectClose=OFF;
 LPT_info[0].iMode=EHPM_EMULATION;

 for (a=0;a<LPT_MAX;a++)
 {
	// Trovo il modello di stampante usato e se  installata
	sprintf(printer,"lpt%d=",a+1);
	tipoprint=RicercaINI(ON,printer);
	LPT_info[a].orientamento=LPT_PORTRAIT;
	if (tipoprint==NULL) tipoprint="";
	
	if (*tipoprint<'!')
	{
	 //  -------------------------
	 //     PORTA NON COLLEGATA  !
	 //  -------------------------
			LPT_info[a].iDirectActive=OFF;
			strcpy(LPT_info[a].szDirectName,"");
			strcpy(LPT_info[a].szDirectDesc,"");
			LPT_info[a].fDirectRemote=OFF;
			LPT_info[a].fDirectClose=OFF;
		 }
		 else
		 {
		 //  -------------------------
		 //       PORTA COLLEGATA    !
		 //  -------------------------

			LPT_info[a].iDirectActive=ON;
			LPT_info[a].iDirectType=LPT_EPSON;

			strcpy(LPT_info[a].szDirectName,tipoprint);
			sprintf(printer,"%s_NAME",tipoprint);
			strcpy(LPT_info[a].szDirectDesc,RicercaINI(OFF,printer));

			// Close alla fine
			sprintf(printer,"lpt%d_close",a+1);
			coda=RicercaINI(ON,printer);
			if (coda!=NULL) LPT_info[a].fDirectClose=ON; else LPT_info[a].fDirectClose=OFF;

			// Stampante remota
			sprintf(printer,"lpt%d_coda",a+1);
			coda=RicercaINI(ON,printer);
			if (coda!=NULL) LPT_info[a].fDirectRemote=TRUE; else LPT_info[a].fDirectRemote=FALSE;

			// Tipo di stampante
			sprintf(printer,"lpt%d_type=EPSON",a+1);
			if (RicercaINI(ON,printer)) LPT_info[a].iDirectType=LPT_EPSON;
			sprintf(printer,"lpt%d_type=EPSON24",a+1);
			if (RicercaINI(ON,printer)) LPT_info[a].iDirectType=LPT_EPSON24;
			sprintf(printer,"lpt%d_type=HP",a+1);
			if (RicercaINI(ON,printer)) LPT_info[a].iDirectType=LPT_HP;
		 }

	//LPT_info[a].attiva=ON; // Bisognerebbe controllare
	LPT_info[a].capture=OFF;  // Flag di capture
	strcpy(LPT_info[a].file,"");
	LPT_info[a].ch=NULL;  // Flag di capture
	LPT_info[a].len=0L;  // Flag di capture

	if (LPT_info[a].iDirectActive)
	{
	 LPT_info[a].iDirectOrient=LPT_PORTRAIT;  // Flag di capture
	 LPT_info[a].iDirectMaxline_V=lpt_maxline(a,"MAXLINE");
	 LPT_info[a].iDirectMaxline_O=lpt_maxline(a,"MAXLINEO");
	}
 }
 */
    // Inizializzazione struttura finestra di dialogo
	memset(&sys.pd,0,sizeof(sys.pd));
	sys.pd.lStructSize         = sizeof (PRINTDLG) ;
    sys.pd.hwndOwner           = NULL ;
    sys.pd.hDevMode            = NULL ;
    sys.pd.hDevNames           = NULL ;
    sys.pd.hDC                 = NULL ;
    sys.pd.Flags               = PD_ALLPAGES | PD_COLLATE | PD_RETURNDC ;
    sys.pd.nFromPage           = 0 ;
    sys.pd.nToPage             = 0 ;
    sys.pd.nMinPage            = 0 ;
    sys.pd.nMaxPage            = 0 ;
    sys.pd.nCopies             = 1 ;
    sys.pd.hInstance           = NULL ;
    sys.pd.lCustData           = 0L ;
    sys.pd.lpfnPrintHook       = NULL ;
    sys.pd.lpfnSetupHook       = NULL ;
    sys.pd.lpPrintTemplateName = NULL ;
    sys.pd.lpSetupTemplateName = NULL ;
    sys.pd.hPrintTemplate      = NULL ;
    sys.pd.hSetupTemplate      = NULL ;

	memset(&sys.ps,0,sizeof(sys.ps));
	sys.ps.lStructSize         = sizeof(PAGESETUPDLG);
	sys.ps.Flags               = PSD_MARGINS|PSD_MINMARGINS|PSD_INHUNDREDTHSOFMILLIMETERS;
	sys.ps.hInstance		   = NULL;//sys.EhWinInstance;
	//sys.ps.ptPaperSize		   = PSD_INHUNDREDTHSOFMILLIMETERS;

}

void lpt_end(void)
{
 SINT a;
 /*
 for (a=0;a<LPT_MAX;a++)
 {
	if (LPT_info[a].capture) f_close(LPT_info[a].ch);
	LPT_info[a].iDirectActive=OFF; // Bisognerebbe controllare
	strcpy(LPT_info[a].file,"");
	LPT_info[a].ch=NULL;
 }
 */
 if (sys.pd.hDC!=NULL) DeleteDC(sys.pd.hDC);
 sys.pd.hDC=NULL;
}

static void lpt_misure(void)
{
    //if (LPT_info[0].xChar) LPT_info[0].iCharsPerLine = GetDeviceCaps (sys.pd.hDC, HORZRES) / LPT_info[0].xChar ;
    //if (LPT_info[0].yChar) LPT_info[0].iLinesPerPage = GetDeviceCaps (sys.pd.hDC, VERTRES) / LPT_info[0].yChar ;
}

static void lpt_PDEReset(void)
{
}

BOOL InizializePrinter(void)
{
	//if (lpt_CreateDC()) return -1;
	DEVMODE *lpDevMode;
	DEVNAMES *lpDevNames; 
	CHAR *lpNames;
	LPCTSTR lpDriver="WINSPOOL";

	lpDevMode=GlobalLock(sys.pd.hDevMode);
	lpNames=(CHAR *) lpDevNames=GlobalLock(sys.pd.hDevNames);
	if (sys.pd.hDC!=NULL) DeleteDC(sys.pd.hDC);
	sys.pd.hDC=NULL;
	sys.pd.hDC=CreateDC(lpDriver,lpNames+lpDevNames->wDeviceOffset,NULL,lpDevMode);
//	printf("[%s]",lpNames+lpDevNames->wDeviceOffset); getch();
	if (sys.pd.hDC==NULL) PRG_end("InitPrinter():Create DC error\n[%s]",lpDevMode->dmDeviceName);
    GlobalUnlock(sys.pd.hDevNames);
    GlobalUnlock(sys.pd.hDevMode);

	if (!sys.pd.hDC) return -1;

	lpt_PDEReset();
	lpt_misure();
	return 0;
}
/*
static BOOL lpt_CreateDC(void)
{
	DEVMODE *lpDevMode;
	DEVNAMES *lpDevNames;
	HANDLE hPrinter;
	LPCTSTR lpDriver="WINSPOOL";
	DEVMODE devBackup;
	BOOL fError=FALSE;
	CHAR *lpNames;

	lpDevMode=GlobalLock(sys.pd.hDevMode);
	lpNames=(CHAR *) lpDevNames=GlobalLock(sys.pd.hDevNames);

	//printf("[%s]",lpDevMode->dmDeviceName); getch();
	if (OpenPrinter(lpDevMode->dmDeviceName,&hPrinter,NULL))
	{
	  memcpy(&devBackup,lpDevMode,sizeof(DEVMODE));
	  DocumentProperties(NULL,//WIN_info[sys.WinInputFocus].hWnd,
		                 hPrinter,
						 (CHAR *) lpDevNames,
						 lpDevMode,
						 lpDevMode,
						 DM_OUT_BUFFER);
	  if (sys.pd.hDC!=NULL) DeleteDC(sys.pd.hDC);
	  memcpy(lpDevMode,&devBackup,sizeof(DEVMODE)); // Non so perch่ funziona
	  sys.pd.hDC=CreateDC(lpDriver,lpNames+lpDevNames->wDeviceOffset,NULL,lpDevMode);
	  if (sys.pd.hDC==NULL) PRG_end("lpt_CreateDC():Create DC error\n[%s]",lpNames+lpDevNames->wDeviceOffset);
	  
	  ClosePrinter(hPrinter);
	} else {fError=TRUE; PRG_end("Error in lpt_CreateDC([%s])",lpDevMode->dmDeviceName);}
  
  GlobalUnlock(sys.pd.hDevNames);
  GlobalUnlock(sys.pd.hDevMode);
  return fError;
}
*/
static SINT lpt_askPageSystem(void)
{
	sys.ps.lStructSize=sizeof(PAGESETUPDLG);
	sys.ps.hwndOwner =NULL;
	sys.ps.hDevMode  =sys.pd.hDevMode;
    sys.ps.hDevNames =sys.pd.hDevNames;

//	ipt_noedit();
	    
	if (!PageSetupDlg(&sys.ps)) return -1;
	sys.pd.hDevMode  =sys.ps.hDevMode;
	sys.pd.hDevNames =sys.ps.hDevNames;
	
	//if (lpt_CreateDC()) return -1;
	InizializePrinter();
	//lpt_PDEReset();
	lpt_misure();
	return 0;
}

static SINT lpt_askSystem(void);
static SINT lpt_askEh(void);
static SINT lpt_askPageSystem(void);

SINT lpt_ask(SINT Mode)
{
	switch (Mode)
	{
	 case EHP_DEFAULT:
//	 case EHPM_EMULATION : return lpt_askSystem(); 
//	 case EHPM_DIRECT    : return lpt_askEh();
	 case EHP_ASKPAGE    : return lpt_askPageSystem(); 
	}
	return 0;
}

BOOL lpt_GetDefault(void)
{
	SINT Flags;
	BOOL rt;

	if (sys.pd.hDevMode!=NULL) return TRUE;
	
	sys.pd.hwndOwner = NULL;//WIN_info[sys.WinInputFocus].hWnd;
	Flags=sys.pd.Flags;
    sys.pd.Flags|=PD_RETURNDEFAULT;
    rt=PrintDlg(&sys.pd);
	sys.pd.Flags=Flags;
	if (!rt) return FALSE;
	return TRUE;
}

void lpt_orient(SINT lpt,SINT flag)
{
	DEVMODE *lpDevMode;
	
	// Se la stampante non ่ ancora setta la enumero
	if (!lpt_GetDefault()) return;

	lpDevMode=GlobalLock(sys.pd.hDevMode);
    if (lpDevMode==NULL) PRG_end("lpt_orient:eseguire prima lpt_ask()");
	
	switch (flag)
	{
		 case LPT_PORTRAIT:  lpDevMode->dmOrientation=DMORIENT_PORTRAIT;
							 break;

		 case LPT_LANDSCAPE: lpDevMode->dmOrientation=DMORIENT_LANDSCAPE;
							 break;
		 default: PRG_end("orient ?");
	}

	GlobalUnlock(sys.pd.hDevMode);
	//lpt_CreateDC();
	InizializePrinter();
	lpt_misure();
}
#endif

void EhLogOpen(CHAR *lpFileLog,void (*funz)(CHAR *ptr))
{
	sys.chLog=fopen(lpFileLog,"ab"); 
	if (!sys.chLog) sys.chLog=fopen(lpFileLog,"wb");
	if (!sys.chLog) PRG_end("EhLogOpen(): apertura %d LOG %s",GetLastError(),lpFileLog);
}

void EhLogWrite(CHAR *Mess,...)
{	
	va_list Ah;
	va_start(Ah,Mess);
	if (sys.chLog!=NULL)
	{
	 fprintf(sys.chLog,"%s %s | ",data_sep(data_oggi(),"/"),ora_oggi("hms"));
	 if (Mess!=NULL) vfprintf(sys.chLog,Mess,Ah);
	 fprintf(sys.chLog,"\r\n");
	 fflush(sys.chLog);
	}
	va_end(Ah);
}

/*
void ProcessRun(CHAR *lpProgram,
    CHAR *lpCommandLine,
    CHAR *lpEnvironment, // Separare con /n CICCIO=PIPPO/nA=2/n
    BOOL fHide)
{
 STARTUPINFO StartupInfo;
 PROCESS_INFORMATION ProcessInformation;
 BOOL bCheck;
 BYTE *p;
 
 if (lpEnvironment) {for (p=lpEnvironment;*p;p++) {if (*p=='\n') *p=0;}}
 
 ZeroFill(StartupInfo);
 
 // Nascondo la finestra
 if (fHide)
 {
  StartupInfo.cb=sizeof(StartupInfo);
  StartupInfo.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
 }
 
 ZeroFill(ProcessInformation);
 
 bCheck=CreateProcess(lpProgram, // // pointer to name of executable module
      lpCommandLine,  // pointer to command line string
      NULL,//LPSECURITY_ATTRIBUTES lpProcessAttributes,  // process security attributes
        NULL,// LPSECURITY_ATTRIBUTES lpThreadAttributes,   // thread security attributes
      FALSE,//BOOL bInheritHandles,  // handle inheritance flag
      0,//DWORD dwCreationFlags, // creation flags
      lpEnvironment,  // pointer to new environment block
      NULL,//LPCTSTR lpCurrentDirectory,   // pointer to current directory name
      &StartupInfo,  // pointer to STARTUPINFO
      &ProcessInformation  // pointer to PROCESS_INFORMATION
      );
 
 WaitForSingleObject(ProcessInformation.hProcess, INFINITE);  // time-out interval, in milliseconds
 CloseHandle(ProcessInformation.hProcess);
}

void Ehmemcpy(void* dst, const void* src, size_t len) {memcpy(dst, src, len);}

*/
void Ehmemcpy(void* dst, const void* src, size_t len) {memcpy(dst, src, len);}

BOOL ProcessRun(CHAR *lpProgram,
				CHAR *lpCommandLine,
				CHAR *lpEnvironment, // Separare con /n CICCIO=PIPPO/nA=2/n
				BOOL fHide,
				SINT *lpiErr)
{
 STARTUPINFO StartupInfo;
 PROCESS_INFORMATION ProcessInformation;
 BOOL bCheck;
 BYTE *p;
 
 if (lpEnvironment) 
 {
	 while (StrReplace(lpEnvironment,CRLF,"\n"));
	 for (p=lpEnvironment;*p;p++) {if (*p=='\n') *p=0;}
	 *(p+1)=0;
}
 
 ZeroFill(StartupInfo);
 
 // Nascondo la finestra
 if (fHide)
 {
  StartupInfo.cb=sizeof(StartupInfo);
  StartupInfo.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
 }
 
 ZeroFill(ProcessInformation);
 
 bCheck=CreateProcess(lpProgram, // // pointer to name of executable module
      lpCommandLine,  // pointer to command line string
      NULL,//LPSECURITY_ATTRIBUTES lpProcessAttributes,  // process security attributes
      NULL,// LPSECURITY_ATTRIBUTES lpThreadAttributes,   // thread security attributes
      FALSE,//BOOL bInheritHandles,  // handle inheritance flag
      0,//DWORD dwCreationFlags, // creation flags
      lpEnvironment,  // pointer to new environment block
      NULL,//LPCTSTR lpCurrentDirectory,   // pointer to current directory name
      &StartupInfo,  // pointer to STARTUPINFO
      &ProcessInformation  // pointer to PROCESS_INFORMATION
      );
 
 if (bCheck) 
 {
  WaitForSingleObject(ProcessInformation.hProcess, INFINITE);  // time-out interval, in milliseconds
  CloseHandle(ProcessInformation.hProcess);
 }
 else
 {
	if (lpiErr) *lpiErr=GetLastError();
 }
 return bCheck;
}

CHAR *ProcessRunAlloc(CHAR *lpProgram,
					  CHAR *lpCommandLine,
					  CHAR *lpEnvironment, // Separare con /n CICCIO=PIPPO/nA=2/n
					  BOOL fHide,
					  SINT *lpiErr,
					  BOOL fEnvironmentParent) // T/F Aggiungo l'Enviroment del Task Padre
{
 STARTUPINFO StartupInfo;
 PROCESS_INFORMATION ProcessInformation;
 BOOL bCheck;
 BYTE *p;
 HANDLE hdlFile;
 BYTE *lpEnv=NULL;
 SECURITY_ATTRIBUTES sa ={sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
 CHAR *lpFile=EhAlloc(1024);
 
 TempFileName(NULL,"PRCS",lpFile);
 if (lpiErr) *lpiErr=0;

 if (fEnvironmentParent||lpEnvironment)  {lpEnv=EhAlloc(0x10000); *lpEnv=0;} // Alloco Environment
 if (fEnvironmentParent)
 {
  BYTE *pBlock=GetEnvironmentStrings();
  for (p=pBlock;*p;) 
	{
		strcat(lpEnv,p); strcat(lpEnv,"\n");
		p+=strlen(p)+1;
	}
	FreeEnvironmentStrings(pBlock);
 }
 if (lpEnvironment) strcat(lpEnv,lpEnvironment);

 if (lpEnv) 
 {
	 while (StrReplace(lpEnv,CRLF,"\n"));
	 for (p=lpEnv;*p;p++) {if (*p=='\n') *p=0;}
	 *(p+1)=0;
 }

 ZeroFill(StartupInfo);
 hdlFile= CreateFile(lpFile,GENERIC_WRITE,0,&sa,OPEN_ALWAYS,FALSE,(HANDLE)NULL);
 if (hdlFile==(HANDLE) INVALID_HANDLE_VALUE) PRG_end("Errore %d - %s",GetLastError(),lpFile);

 StartupInfo.cb=sizeof(StartupInfo);
 StartupInfo.dwFlags=STARTF_USESTDHANDLES;//|STARTF_USESHOWWINDOW;
 if (fHide)
 {
	StartupInfo.dwFlags|=STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = SW_MINIMIZE;
 }

 //StartupInfo.hStdInput=hdlFile;
 StartupInfo.hStdError=hdlFile;
 StartupInfo.hStdOutput=hdlFile;

 ZeroFill(ProcessInformation);

 // printf("Run: %s" CRLF,lpProgram);
 bCheck=CreateProcess(lpProgram, // // pointer to name of executable module
      lpCommandLine,  // pointer to command line string
      NULL,//LPSECURITY_ATTRIBUTES lpProcessAttributes,  // process security attributes
      NULL,// LPSECURITY_ATTRIBUTES lpThreadAttributes,   // thread security attributes
      TRUE,//BOOL bInheritHandles,  // handle inheritance flag
      0,//DWORD dwCreationFlags, // creation flags
      lpEnv,  // pointer to new environment block
      NULL,//LPCTSTR lpCurrentDirectory,   // pointer to current directory name
      &StartupInfo,  // pointer to STARTUPINFO
      &ProcessInformation  // pointer to PROCESS_INFORMATION
      );

 if (bCheck) 
 {
    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);  // time-out interval, in milliseconds
    CloseHandle(ProcessInformation.hProcess);
 }
 else
 {
	 if (lpiErr) *lpiErr=GetLastError();
 }
 CloseHandle(hdlFile);
 if (lpEnv) {EhFree(lpEnv); lpEnv=NULL;}

 p=FileToString(lpFile); remove(lpFile);
 EhFree(lpFile);
 return p;
}


void sys_rapport(CHAR *Mess,va_list arg)
{
	SINT a,ccm;
	LONG totale,totmem[129];
	LONG totocc;
	FILE *ch;
	CHAR file[50];

	//		Variabili Ico_Report
	struct ICONE *ptico;

	CHAR *desc_memo[]={"(Heap)     ",
					   "(Global)   ",
					   "(Heap/Lock)",
					   "3          ",
					   "4          "};

	//sonic(5000,1,1,1,1,10);
    if (Mess[0]=='#') strcpy(file,Mess+1); else strcpy(file,EHPath("RAPPORTO.INF"));

	if ((ch=fopen(file,"w"))==NULL) return;
//	if ((ch=fopen("PRN","wb"))==NULL) return;

	fprintf(ch,"STATUS DELLA SUITE (32bit)\n");
	fprintf(ch,"------------------------------------\n");
	fprintf(ch,"Errore programma  : ");
	vfprintf(ch,Mess,arg);
	fprintf(ch,"\n",Mess,arg);
/*
	if (DE_coden!=0)
	{fprintf(ch,"\n\rDos error         : %s [%2x]\n\r",DE_code,DE_coden);
	 fprintf(ch,"Classe di appart. : %s [%2x]\n\r",DE_class,DE_classn);
	 fprintf(ch,"Sugg.             : %s [%2x]\n\r",DE_azione,DE_azionen);
	 fprintf(ch,"Relativo a        : %s [%2x]\n\r",DE_locus,DE_locusn);
	 if (DE_coden==0x53) // INT 24h
	 {
	 fprintf(ch,"\n\rDos Hard error    : %s [%4x/%4x]\n\r",DHE_code,DHE_err,DHE_drive);
	 }
	}
	*/
	fprintf(ch,"Jobber Program    : %s\n",PtJobber);
	fprintf(ch,"\n.STATUS DELLA MEMORIA\n");

	for (a=0;a<9;a++) totmem[a]=0; // Azzero il contatore di memoria

	fprintf(ch,"Stato della L.M.O. : occupate %hd celle (Max %hd)\n",sys.memocont,sys.memomaxi);

	totmem[RAM_HEAP]=sys.memomaxi*sizeof(struct RAM);
	fprintf(ch,"Heap memory L.M.O. : %u \n\n",totmem[RAM_HEAP]);

	if (sys.memocont==0) fprintf(ch,"Nessuna cella occupata.\n");

	ccm=0;
	for (a=0;a<sys.memomaxi;a++)
	 {
	if (sys.memolist[a].iTipo==RAM_HDLFREE) continue;

	//fsg=sys.memolist[a].lpvMemo;
	//fof=sys.memolist[a].lpvMemo;

	switch (sys.memolist[a].iTipo)
	{

		case RAM_HEAP:
		ccm++;
		fprintf(ch,
					"%3d - %-16s %s : %7ldb / WHld : %4x / PT32 [%12ld]\n",
					a,
					sys.memolist[a].User,
					desc_memo[sys.memolist[a].iTipo],
					sys.memolist[a].dwSize,
					sys.memolist[a].hGlobal,
					(LONG) sys.memolist[a].lpvMemo);
		break;

		case RAM_GLOBALHEAP:
		ccm++;
		fprintf(ch,
					"%3d - %-16s %s : %7ldb / WHld : %4x / PT32 [%12ld]\n",
					a,
					sys.memolist[a].User,
					desc_memo[2],
					sys.memolist[a].dwSize,
					sys.memolist[a].hGlobal,
					(LONG) sys.memolist[a].lpvMemo);
		break;

		case RAM_BASSA:
		ccm++;
		fprintf(ch,
					"%3d - %-16s %s : %7ldb / WHld : %4x\n",
					a,
					sys.memolist[a].User,
					desc_memo[sys.memolist[a].iTipo],
					sys.memolist[a].dwSize,
					sys.memolist[a].hGlobal);
	  break;

	 case RAM_XMS:
	 case RAM_DISK:

	 fprintf(ch,"Errore <<\n");
	 break;

	}

	totmem[sys.memolist[a].iTipo]+=sys.memolist[a].dwSize;
 }

 if (ccm!=sys.memocont) fprintf(ch,"Discordanza tra sys.memocont e memorie realmente occupate\n");

 //	fprintf(ch,"\nSize Stack Memory  : %d [_stklen %d:SS=%4x]\n\n",SIZESTACK,_stklen,_SS);

 //		Totali memorie occupate
 fprintf(ch,"\n");
 totale=0;
 for (a=0;a<5;a++)
 {
	fprintf(ch,"Totale %s : %10ldb\n",desc_memo[a],totmem[a]);
	totale+=totmem[a];
	}
	fprintf(ch,"Memoria totale occupata : %9ldb\n\n",totale);

	totocc=memo_usata();//totmem[RAM_HEAP]+totmem[RAM_BASSA];
	fprintf(ch,"Convez. Iniziale       : %10ldb\n",sys.MEMO_inizio);
	fprintf(ch,"Convezionale usata     : %10ldb\n",totocc);
	fprintf(ch,"Libera matematica      : %10ldb\n",sys.MEMO_inizio-totocc);
	//fprintf(ch,"Libera (Coreleft)      : %10ldb\n",coreleft());
	fprintf(ch,"sys.MEMO_rimasta       : %10ldb\n",sys.MEMO_rimasta);
	//fprintf(ch,"Differenza sfuggita    : %10ldb\n",coreleft()-(sys.MEMO_inizio-totocc));

// lpt_pag();

 fprintf(ch,"\n");

 // ณ         Situazione CLIP_STACK             ณ
/*
 fprintf(ch,".STATUS CLIP_STACK\n");
 fprintf(ch,"Clip_stack : %hd [%hd]\n",clip_num,MAXCLIP);

 fprintf(ch,"CLIP_SET Attivo: (%3d,%3d,%3d,%3d)\n",
						 sys.clp_x1,sys.clp_y1,sys.clp_x2,sys.clp_y2);

 for (a=0;a<clip_num;a++)
 {

	fprintf(ch,"Stack clip-zone %3d <%-10s> ",
					a,
					clip_stack[a].user);


	if (clip_stack[a].x1==-1)
		fprintf(ch,"OUTSIDE:Stampa disattivata");
		else
		fprintf(ch,"(%3d,%3d,%3d,%3d)\n",
					clip_stack[a].x1,
					clip_stack[a].y1,
					clip_stack[a].x2,
					clip_stack[a].y2);

 }
 fprintf(ch,"\n");
*/
 // ณ         Situazione WINDOWS             ณ

 fprintf(ch,".STATUS WINDOWS\n");
 fprintf(ch,"windows : %hd [%hd]\n",WIN_ult+1,WIN_max);

 for (a=0;a<WIN_ult+1;a++)
 {
    int b;
	fprintf(ch,"%3d <%s> pos=%3d,%3d (lx=%3d,ly=%3d)\n",
			a,
			WIN_info[a].titolo,
			WIN_info[a].x,
			WIN_info[a].y,
			WIN_info[a].Lx,
			WIN_info[a].Ly);
    
	fprintf(ch,"   .Emulation : %s\n",WIN_info[a].DosEmulation ? "Si" : "No");
    fprintf(ch,"   .SubPaint  : %s\n",(WIN_info[a].SubPaint!=NULL) ? "Si" : "No");
   
	// Elenco del ClipZone della finestra
    fprintf(ch,"   Clip Numero=%d\n",WIN_info[a].ClipNum);
	
	for (b=0;b<WIN_info[a].ClipNum;b++)
	{
      fprintf(ch,"  %d) %-20.20s (%d,%d)-(%d,%d)\n",
		      WIN_info[a].Clip[b].user,
		      WIN_info[a].Clip[b].x1,
			  WIN_info[a].Clip[b].y1,
		      WIN_info[a].Clip[b].x2,
			  WIN_info[a].Clip[b].y2);
	}

 }
 fprintf(ch,"\n");

 // ณ         Situazione OGGETTI             ณ

 fprintf(ch,".STATUS OGGETTI\n");
 fprintf(ch,"Aree -object- : %hd [%hd]\n",OBJ_ult+1,OBJ_max);

 for (a=0;a<OBJ_ult+1;a++)
 {
	fprintf(ch,"%3d N.Obj:%3d (ALO_Hdl %3d)\n",
					a,
					OBJ_info[a].numobj,OBJ_info[a].alo_hdl);

 }
 fprintf(ch,"\n");

 // ณ         Situazione KEYINPUT            ณ

 fprintf(ch,".STATUS KEYINPUT\n");
 fprintf(ch,"Aree -input- : %hd [%hd]\n",IPT_ult+1,IPT_max);

 for (a=0;a<IPT_ult+1;a++)
 {
	fprintf(ch,"%3d N.campi:%3d %3d>%3d (BUF_Hdl %3d / OBJPASS_hdl %3d)\n",
					a,IPT_info[a].numcampi,
					IPT_info[a].campo,IPT_info[a].procampo,
					IPT_info[a].buf_hdl,
					IPT_info[a].objpass_hdl);
 }
 fprintf(ch,"\n");

 // ณ         Situazione MGZ            ณ

 if (MGZ_hdl!=-1)
 {
	fprintf(ch,".STATUS MGZ (Mouse Graphic Zone) \n");
	fprintf(ch,"Aree abilitate : %hd [%hd]\n",MGZ_ult+1,MGZ_max);

	for (a=0;a<MGZ_ult+1;a++)
	{
	fprintf(ch,"%3d %-10s (%3d,%3d,%3d,%3d) grp=%3d\n",
					a,
					MGZ_info[a].nome,
					MGZ_info[a].x, MGZ_info[a].y,
					MGZ_info[a].x2, MGZ_info[a].y2,
					MGZ_info[a].grp);
	}
 }
 else fprintf(ch,".MGZ non attivo \n");

 fprintf(ch,"\n");

 // ณ         Situazione HMZ            ณ

 if (HMZ_hdl!=-1)
 {
	fprintf(ch,".STATUS HMZ (Help Mouse Zone) \n");
	fprintf(ch,"Aree abilitate : %hd [%hd]\n",HMZ_ult+1,HMZ_max);

	for (a=0;a<HMZ_ult+1;a++)
	{
	    if (HMZ_info[a].win==-1)
			fprintf(ch,"%3d Vuota\n",a);
			else
			fprintf(ch,"%3d %-10s (%3d,%3d,%3d,%3d) win=%3d \n",
					a,
					HMZ_info[a].help,
					HMZ_info[a].x, HMZ_info[a].y,
					HMZ_info[a].x2, HMZ_info[a].y2,
					HMZ_info[a].win);
	}
 }
 else fprintf(ch,".HMZ non attivo \n");

 fprintf(ch,"\n");


 // ณ         Situazione FTIME          ณ


 fprintf(ch,"\n");

 // ณ         Situazione Font           ณ
/*
 fprintf(ch,".STATUS Font \n");
 fprintf(ch,"Font caricati: %hd [%hd] \n\n",FONT_car,FONT_max);

 for (a=0;a<FONT_car;a++) // Loop per il font di caratteri
 {
	fprintf(ch,"%-30s nfi=%hd hdl=%hd \n",
							FONT_info[a].font,FONT_info[a].num_font,FONT_info[a].hdl);
			 if (memo_leggivar(hdl,0,&chr_head,sizeof(chr_head))) return -3;
			 *maxnfi=chr_head.num_font;
			 *hdl_memo=hdl;
 }
 fprintf(ch,"\n");
*/

 // ณ         Situazione File           ณ

 fprintf(ch,".STATUS Files\n");
 fprintf(ch,"Files aperti: %hd \n\n",FILE_aperti);

 // ณ         Situazione Dbase          ณ
 /*
 if (DB_max>0)
 {
	fprintf(ch,".STATUS DBase \n\r");
	fprintf(ch,"Dbase aperti: %hd [%hd]\n\r",DB_ult+1,DB_max);

	for (a=0;a<DB_max;a++)
	{
		if (DB_info[a].ramhdl!=-1)
		{
		fprintf(ch,"%3d %-14s ramHdl=%3d \n\r",
					a,
					DB_info[a].nome,
					DB_info[a].ramhdl);
		}
	}
 fprintf(ch,"\n\r");
 }
   */

 // ณ         Situazione Porte seriali  ณ
/*
	fprintf(ch,".STATUS Porte seriali \n\r");
	fprintf(ch,"COM aperte: %hd su [%hd]\n\r",COM_aperte,COM_max);

	for (a=0;a<COM_max;a++)
	{
	 fprintf(ch,"COM%c:",a+'1');

	 if (COM_info[a].ind==-1)
			{fprintf(ch,"DISATTIVATA in EH3.INI\n\r"); continue;}

	 if (COM_info[a].power) fprintf(ch," IN USO ");
													else
													fprintf(ch,"        ");

	 fprintf(ch,"Ind:%3x Irq%2d Hdlbuf=%3d \n\r",
					 COM_info[a].ind,
					 COM_info[a].irq,
					 COM_info[a].hdlbuf);
	}

 fprintf(ch,"\n\r");
	*/
 // ณ         Mostra le icone in memoria        ณ

  // ณ         Mostra le icone in memoria        ณ

 fprintf(ch,".STATUS DELLE ICONE\n");

 fprintf(ch,"Numero icone : %d [%d]\n",sys.ICO_num,sys.ICO_max);
 ptico=sys.icone;

 for (a=0;a<sys.ICO_num;a++)
 {
	//strcpy((CHAR *) &s1,ptico->nome);
	fprintf(ch,"%3d) %-9s hdl=%3d offset %6ld size=%6ld grp:%d",
							a+1,ptico->nome,ptico->hdl,ptico->offset,
							ptico->size,ptico->grp);

//	if (*ptico->lic>0) fprintf(ch," LIC %s",ptico->lic);
	 if (ptico->Lic)
	 {
		fprintf(ch," %s",sys.memolist[ptico->hdl].User);
	 }
/*
	ico_head=(struct ICO_HEAD *) ptico->pti;

	fprintf(ch," (%3dx%3d) \n",
					ico_head->dimx,
					ico_head->dimy);
					//ico_head->byte, Byte per riga
					//ico_head->ofs_mask,
					//ico_head->ofs_icone);
	*/
	fprintf(ch,"\n");

 ptico++;
 }
fprintf(ch,"\n");


 fclose(ch);
}

BOOL ClipboardPut(CHAR *lpMess,...)
{
	va_list Ah;
	HANDLE hgm;
	LONG lSize;
	BYTE *lpt;
	BYTE *lpBuffer=NULL;

	if (!lpMess) return TRUE;

	lpBuffer=EhAlloc(8192);
	va_start(Ah,lpMess);
	vsprintf(lpBuffer,lpMess,Ah); // Messaggio finale

	lSize=strlen(lpBuffer);

	hgm=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,lSize+1);
	lpt=GlobalLock(hgm); memset(lpt,0,lSize+1);
	memcpy(lpt,lpBuffer,lSize);
	GlobalUnlock(hgm);
	if (OpenClipboard(NULL))
	{
		EmptyClipboard();
		SetClipboardData(CF_TEXT,hgm);
		CloseClipboard();
	}

	EhFree(lpBuffer);
	va_end(Ah);
	return FALSE;
}


// NOTA
// Lavorando in modalitเ UTF-16 ci possono essere catatteri che prendono due coppie a 16bit
// Quindi in caso di allocazione Unicode prevedere 4 byte per carattere
//
void CharToUnicode(wchar_t *lpDest,CHAR *lpSource)
{
	MultiByteToWideChar(uACP,
						MB_PRECOMPOSED,//MB_COMPOSITE,//MB_PRECOMPOSED,
						lpSource,-1,//strlen(lpSource)+1,
						lpDest,(strlen(lpSource)+1)*2);
//	mbstowcs(lpDest,lpSource,(strlen(lpSource)+1)*4);
}	
void UnicodeToChar(CHAR *lpDest,wchar_t *lpwcSource)
{
	SINT iLen;
	iLen=wcslen(lpwcSource);
	WideCharToMultiByte(uACP,
						WC_COMPOSITECHECK,
						lpwcSource,-1,//iLen+1,
						lpDest,iLen+1,
						NULL,NULL);
	lpDest[iLen]=0;
}	
CHAR *StringAlloc(CHAR *lpString)
{
	CHAR *lp;
	lp=EhAlloc(strlen(lpString)+1);
	strcpy(lp,lpString);
	return lp;
}

WCHAR *StringAllocW(WCHAR *lpString)
{
	WCHAR *lp;
	SINT iSize=wcslen(lpString)+1; 
	lp=EhAlloc(iSize*4); memset(lp,0,iSize*4);
	memcpy(lp,lpString,iSize<<1);	//wcscpy(lp,lpString);
	return lp;
}

WCHAR *StringAllocToUnicode(CHAR *lpString)
{
	WCHAR *lp;
	int iBufSize;
	//int iCodePage=850;
	iBufSize=MultiByteToWideChar(uACP,
								 MB_PRECOMPOSED,//MB_PRECOMPOSED,
								 lpString,-1,//strlen(lpSource)+1,
								 0,0);
	lp=EhAlloc(iBufSize*2);
	MultiByteToWideChar(uACP,
						MB_PRECOMPOSED,//MB_PRECOMPOSED,
						lpString,-1,//strlen(lpSource)+1,
						lp,iBufSize);
	return lp;
}

CHAR *StringAllocToChar(WCHAR *wcString)
{
	CHAR *lp;
	lp=EhAlloc(wcslen(wcString)+1);
	UnicodeToChar(lp,wcString);
	return lp;
}

CHAR *FileToString(CHAR *lpFile)
{
 HANDLE hFile;
 DWORD dwLen,dwHigh,dwReaded;
 CHAR *lpString;

 hFile = CreateFile((LPCTSTR) lpFile,
				   GENERIC_READ,
				   FILE_SHARE_READ,
				   NULL,
			       OPEN_EXISTING,
				   0,
				  (HANDLE)NULL);
 
 if (hFile==(HANDLE) INVALID_HANDLE_VALUE) return NULL;
 dwLen=GetFileSize(hFile,&dwHigh);
 lpString=EhAlloc(dwLen+1);
 ReadFile(hFile,lpString,dwLen,&dwReaded,NULL);
 lpString[dwReaded]=0;
 CloseHandle(hFile);
 return lpString;
}

// Se errore ritorna TRUE
BOOL StringToFile(CHAR *lpFile,CHAR *lpString)
{
 HANDLE hFile;
 DWORD dwWrited;
 hFile = CreateFile((LPCTSTR) lpFile,
				   GENERIC_WRITE,
				   0,
				   NULL,
			       CREATE_ALWAYS,
				   0,
				   (HANDLE)NULL);
 
 if (hFile==(HANDLE) INVALID_HANDLE_VALUE) return TRUE;
 WriteFile(hFile,lpString,strlen(lpString),&dwWrited,NULL);
 CloseHandle(hFile);
 return FALSE;
}