//   ---------------------------------------------
//   | SDBdrive  SuperDbaseDriver                |
//   |          -Versione 99                     |
//   |                                           |
//   |                                           |
//   |             by Ferrà Art & Technology 1999 |
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"

// extern struct ADB_INFO *ADB_info;

//#include <flm_adb.h>

// Numero massimo di scroll contemporaneamente gestiti = 5
#define  SDBMAX 5

// Numero massimo di linee per scroll = 30
//#define	 MAX_Y 30
//static SINT iCount=0;

typedef struct{
	struct OBJ *ObjClient;
	BOOL Mode;      // 0=O_SCRDB/OW_SCRDB  1=O_SCROLL/OW_SCR
	SINT Hdb; 		 // Numero del ADB
	SINT IndexNum; // Numero del KeyPath
	SINT (*ExtFunz)(SINT cmd,
					CHAR *PtDati,
					LONG  dato,
					void  *str,
					SINT Hdb,
					SINT IndexNum);

	BOOL  ExtFilter;
	BOOL  AdbFilter;
	float FiltUP;
	float FiltDN;
	LONG  RecUP;
	LONG  RecDN;
	LONG  LastOffset;

	LONG  RecSize;

	SINT   WinScrHdl;
	struct WINSCR *WinScr;
	CHAR  *RecBuffer;
	SINT   Dove;
	SINT   End;
	BOOL   fCallLock; // Blocca le richieste al dbase
	struct WS_INFO ws;
	//BOOL fInProcess;

} SDBEXT;

static void  DB_GetLast(SDBEXT *Sdb);
static BOOL  DB_seek(LONG info,SDBEXT *Sdb);
static SINT  DB_perc(LONG info,SDBEXT *Sdb);
static SINT  DB_find(SINT cmd,LONG info,CHAR *str,SDBEXT *Sdb);
static BOOL  DB_Check(LONG info,CHAR *str,SDBEXT *Sdb);
static void  DB_buf (SDBEXT *Sdb);
static SINT  DB_load(SDBEXT *Sdb);
static SINT  DB_zone(SDBEXT *Sdb);

static HREC LGetRecord(SDBEXT *pSdb,SINT iRecord)
{
	HREC hRec;
	memoRead(ADB_info[pSdb->Hdb].AdbFilter->hdlRecords,iRecord*sizeof(HREC),&hRec,sizeof(HREC));
	return hRec;
}

// Costruttore di array
static void LArrayMake(SDBEXT *Sdb,SINT NumCam)
{
	CHAR Serv[40];
	LONG size=sizeof(struct WINSCR)*NumCam; // Calcola Grandezza Buffer

	if (NumCam<1) ehExit("Numcam ? [%d]",NumCam);
	sprintf(Serv,"*ListHrec:%02d",Sdb->Hdb);
	if (Sdb->WinScrHdl!=-1) memoFree(Sdb->WinScrHdl,Serv);
	Sdb->WinScrHdl=-1;
	
	Sdb->WinScrHdl=memoAlloc(M_HEAP,size,Serv);
	if (Sdb->WinScrHdl<0) ehExit("Sdb:x01");
	//win_infoarg("> %d (%d) %d",NumCam,size,Sdb->WinScrHdl);

	Sdb->WinScr=memoPtr(Sdb->WinScrHdl,NULL);
	memset(Sdb->WinScr,0,(SINT) size);
}

#ifdef _WIN32
static CHAR *CmdToString(SINT cmd)
{
switch (cmd)
 {
  case WS_OPEN:    return "WS_OPEN";
  case WS_CLOSE:   return "WS_CLOSE";
  case WS_LOAD:    return "WS_LOAD";
  case WS_CHANGE:  return "WS_CHANGE";
  case WS_HDB:     return "WS_HDB";
  case WS_IDX:     return "WS_IDX";
  case WS_EXTDSP:  return "WS_EXTDSP";
  case WS_EXTNOTIFY: return "WS_EXTNOTIFY";
  case WS_EXTREC:  return "WS_EXTREC";

  case WS_INF:	   return "WS_INF";
  case WS_BUF: return "WS_BUF";
  case WS_OFF: return "WS_OFF";
  case WS_SEL: return "WS_SEL";
  case WS_PTREC: return "WS_PTREC";
  case WS_ISEL: return "WS_ISEL";
  case WS_IPTREC: return "WS_IPTREC";
  case WS_FIND: return "WS_FIND";
  case WS_FINDKEY: return "WS_FINDKEY";
  case WS_REFON: return "WS_REFON";
  case WS_REFOFF: return "WS_REFOFF";
  case WS_DISPLAY: return "WS_DISPLAY";
  case WS_DBSEEK: return "WS_DBSEEK";
  case WS_DBPERC: return "WS_DBPERC";
  case WS_FIRST: return "WS_FIRST";
  case WS_LAST: return "WS_LAST";
  case WS_FILTER: return "WS_FILTER";
  case WS_SETFILTER: return "WS_SETFILTER";
  case WS_SETFLAG: return "WS_SETFLAG";
  case WS_REALGET: return "WS_REALGET";
  case WS_REALSET: return "WS_REALSET";
  case WS_ICONE: return "WS_ICONE";
  case WS_DRAG: return "WS_DRAG";
  case WS_DROP: return "WS_DROP";
  case WS_KEYPRESS: return "WS_KEYPRESS";
  case WS_ADD: return "WS_ADD";
  case WS_DEL: return "WS_DEL";
  case WS_UPDATE: return "WS_UPDATE";
  case WS_INSERT: return "WS_INSERT";
  case WS_COUNT: return "WS_COUNT";
  case WS_LINK : return "WS_LINK";
  case WS_DO: return "WS_DO";
  case WS_PROCESS: return "WS_PROCESS";
 
  case WS_DOLIST: return "WS_DOLIST";
  case WS_ADBFILTER: return "WS_ADBFILTER";
  case WS_FINDLAST: return "WS_FINDLAST";
  case WS_DESTROY: return "WS_DESTROY";
  case WS_LINEVIEW: return "WS_LINEVIEW";
  case WS_LINEEDIT: return "WS_LINEEDIT";
 }
 return "?";
}

#endif
//  +-----------------------------------------+
//	| SdbDriver                               |
//	|                                         |
//	|          by Ferrà Art & Technology 1999 |
//	+-----------------------------------------+
/*
static void LOGWrite(CHAR *lpFile,CHAR *Mess,...)
{	
	FILE *ch;
	va_list Ah;
	va_start(Ah,Mess);

	ch=fopen(lpFile,"a"); if (ch==NULL) ch=fopen(lpFile,"w");
	if (ch!=NULL)
	{
	 fprintf(ch,"%s %s | ",dateFor(dateToday(),"/"),hourNow("hms"));
	 if (Mess!=NULL) vfprintf(ch,Mess,Ah);
	 fprintf(ch,"\n");
	}
	fclose(ch);
	va_end(Ah);
}
*/
void *SdbDriver(struct OBJ * objCalled,SINT iEvent,LONG info,CHAR *str)
{
	static struct WINSCR rit;
	struct WINSCR *PtScr;
	static SDBEXT SDB[SDBMAX];
	static BOOL   CheckFirst=TRUE;
	SDBEXT *lpSdb; // Un giorno di lavoro ... cazzo
	LONG Hrec;
	EH_DISPEXT *psExt;

	SINT  b;
	SINT  ptClient;
	CHAR  Serv[80];

//	iCount++; 

	// -----------------------------------------------
	// Se è la prima volta azzero la struttura       |
	// -----------------------------------------------
	if (CheckFirst)
		{
			for (b=0;b<SDBMAX;b++)
			 {
				memset(SDB+b,0,sizeof(SDBEXT));
				SDB[b].WinScrHdl=-1;
			 }
		 CheckFirst=FALSE;
		}

	// ---------------------------------------
	// Trova il Client Object                |
	// ---------------------------------------
	ptClient=-1;
	for (b=0;b<SDBMAX;b++)
		{if (objCalled==SDB[b].ObjClient) {ptClient=b; break;}}

	// Se non c'è provo ad assegnarlo
	if (ptClient==-1)
	 {
		for (b=0;b<SDBMAX;b++)
		 {
			if (SDB[b].ObjClient==NULL)
				{SDB[b].ObjClient=objCalled;
				 ptClient=b;
				 SDB[b].ExtFilter=ADBNULL;
				 //SDB[b].fCallLock=TRUE;

				 //printf("[%s]=%d\n",objCalled->nome,b);
				 break;
				}
		 }
	 }
/*
	// Logging
	{
	  CHAR *lpStr="";
	  switch (cmd)
	  {
	    case WS_OPEN:   lpStr="OPEN"; break;
	    case WS_CLOSE: lpStr="CLOSE"; break;
	    case WS_BUF: lpStr="BUF"; break;
	    case WS_DRAG: lpStr="DRAG"; break;
	    case WS_DROP: lpStr="DROP"; break;
		case WS_HDB: lpStr="HDB"; break;
		case WS_IDX: lpStr="IDX"; break;
		case WS_EXTFUNZ: lpStr="FUNZ"; break;
		case WS_INF: lpStr="INF"; break;
		case WS_DISPLAY:lpStr="DISPLAY"; break;
		case WS_DESTROY:lpStr="KILL"; break;
 
		case WS_LINEVIEW:lpStr="LINEVIEW"; break;
		case WS_LINEEDIT:lpStr="LINEEDIT"; break;
		case WS_SETTITLE:lpStr="SETTITLE"; break;
		case WS_SETHRDW:lpStr="SETHRDW"; break;
		case WS_START:lpStr="START"; break;
		case WS_ADBFILTER:lpStr="ADBFILTER"; break;
		case WS_SEL:lpStr="SEL"; break;
	  }

	 LOGWrite("c:\\sdb.txt","%08x:%s[%d]) %d/%d \t\t%s",(LONG) objCalled,objCalled->nome,ptClient,cmd,iCount,lpStr); 
	}
*/

	// Se non ci riesco ... errore
	if (ptClient==-1) ehExit("SDB:Too client!");

	// Puntatore alla struttura SDBEXT
	lpSdb=&SDB[ptClient];
	
	//_d_("cmd=%s Mode=%d",CmdToString(cmd),lpSdb->Mode); ehSleep(500);

	//if ((ptClient!=0)||(cmd==WS_EXTFUNZ)) {
	//_d_("-------> %d [%d] %x",cmd,ptClient,(LONG) objCalled); ehSleep(500);
	if (iEvent==WS_INF) 
	{
		return &lpSdb->ws;
	}
	
	switch (iEvent) {

	// -----------------------------------
	// APERTURA DEL DRIVER               |
	// -----------------------------------

	case WS_OPEN:
			 if (info<3) {ehExit("Field ? SdbDriver");}
			 lpSdb->ws.numcam=info;// Assegna il numero di campi da visualizzare
#ifdef _WIN32
			 if (lpSdb->ExtFunz==NULL) ehExit("SDB:NULL1");
			 if (!lpSdb->fCallLock) (lpSdb->ExtFunz)(WS_OPEN,0,0,0,lpSdb->Hdb,lpSdb->IndexNum);
#endif

	// -----------------------------------
	// CARICA I DATI                     |
	// -----------------------------------

	case WS_LOAD :  // Ex Apri

			 // Congelo la gestione dell'oggetto
		     //win_info("ENTRO");
#ifdef _WIN32
			 lpSdb->ObjClient->bFreeze=TRUE;
#endif
			 lpSdb->Mode=0;// Modalità 1 gestore O_SCROLL
			 if ((lpSdb->ObjClient->tipo==OW_SCR)||(lpSdb->ObjClient->tipo==OW_SCRDB)) 
				   lpSdb->ObjClient->tipo=OW_SCRDB;  
				   else 
				   lpSdb->ObjClient->tipo=O_SCRDB;
           
			 // Se esite collegato un AdbFilter
			 if (lpSdb->AdbFilter)
			 {
			 //printf("LOAD COL FILTER");
		     //win_info("FILTRO");

			 // -----------------------------------------
			 // Controllo se ho un adb_filter connesso  |
			 // -----------------------------------------
			 if (ADB_info[lpSdb->Hdb].AdbFilter!=NULL)
				 {
					//LOGWrite("c:\\sdb.txt","a - %08x:%s)",(LONG) objCalled,objCalled->nome); 
					
					// Un Filtro Classico è attivo ?
					if (ADB_info[lpSdb->Hdb].AdbFilter->ExtProcess||ADB_info[lpSdb->Hdb].AdbFilter->NumComp)
						{ // SI
							//sonic(1000,1,100,10,100,1);
							adb_Filter(lpSdb->Hdb,WS_DOLIST,0,0,0);
							// Inserito nel 2001 perchè in qualche caso objCalled viene cambiato 
							// probabilente da un messaggio lasciato in sospeso
							objCalled=lpSdb->ObjClient; 
							
							//sonic(1000,1,100,10,100,1);
							lpSdb->Mode=1;// Modalità 1 gestore O_SCROLL
							if (lpSdb->ObjClient->tipo==OW_SCRDB)  lpSdb->ObjClient->tipo=OW_SCR; 
																   else 
																   lpSdb->ObjClient->tipo=O_SCROLL;
							
							//objCalled=lpSdb->ObjClient;
							// Comunico che ho cambiato l'indice
							SdbDriver(objCalled,WS_IDX,ADB_info[lpSdb->Hdb].AdbFilter->idxInUse,"");
						}
				 }
			 }

			 //else {printf("LOAD SENZA FILTER"); getch();}
			 if (!lpSdb->fCallLock&&DB_load(lpSdb)) ehExit("SdbDriver:No extern function");
#ifdef _WIN32
			 lpSdb->ObjClient->bFreeze=FALSE;
#endif
			 break;

	// -----------------------------------
	// WS_CLOSE IL DRIVER                  |
	// -----------------------------------
	case WS_CLOSE:

#ifdef _WIN32
			 lpSdb->ObjClient->bFreeze=TRUE;
			 if (!lpSdb->fCallLock) (lpSdb->ExtFunz)(WS_CLOSE,0,0,0,lpSdb->Hdb,lpSdb->IndexNum);
#endif
			 sprintf(Serv,"*ListHrec:%02d",lpSdb->Hdb);
			 if (lpSdb->WinScrHdl!=-1) memoFree(lpSdb->WinScrHdl,Serv);
			 lpSdb->WinScrHdl=-1;
			 lpSdb->WinScr=NULL;
			 lpSdb->RecBuffer=NULL;
			 break;

	// -----------------------------------
	// TERMINATA GESTIONE                |
	// -----------------------------------
	case WS_DESTROY: // Chiusura del Gestore

#ifdef _WIN32
			 (lpSdb->ExtFunz)(WS_DESTROY,0,0,0,lpSdb->Hdb,lpSdb->IndexNum);
#endif
			 if (lpSdb->WinScrHdl!=-1) ehExit("SDB:xx");
			 memset(lpSdb,0,sizeof(SDBEXT));
			 lpSdb->WinScrHdl=-1;
			 break;

	// -----------------------------------
	// RICHIESTA DEL BUFFER              |
	// -----------------------------------
	case WS_BUF : //			  			Richiesta buffer

			 lpSdb->Dove=0;
			 lpSdb->End=(SINT) lpSdb->ws.numcam;
			 DB_buf(lpSdb);
			 break;

	// -----------------------------------
	// RICHIESTA DI SPOSTAMENTO RELATIVO |
	// -----------------------------------
	case WS_DBSEEK : //			  		 Spostamento dell'offset relativo
			 if (lpSdb->Mode) ehExit("DBSEEK ?");
			 if (!lpSdb->fCallLock&&DB_seek(info,lpSdb)) DB_buf(lpSdb);
			 break;

	// -----------------------------------
	// RICERCA DI UN OFFSET ASSOLUTO     |
	// -----------------------------------
	case WS_DBPERC : //			  				 Ricerca di un offset assoluto
			 if (lpSdb->Mode) ehExit("DBPERC ?");
			 if (!lpSdb->fCallLock&&DB_perc(info,lpSdb)) DB_buf(lpSdb);
			 break;

	// -----------------------------------
	// PRESSIONE DI UN TASTO/MOUSE       |
	// -----------------------------------
	case WS_KEYPRESS :
			if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
			// Transfer del comando a sub-driver
//				(lpSdb->ExtFunz)(cmd,NULL,NULL,str,info,lpSdb->Hdb,lpSdb->IndexNum);
			if (!lpSdb->fCallLock)
			{
				 (lpSdb->ExtFunz)(iEvent,NULL,info,str,lpSdb->Hdb,lpSdb->IndexNum);
			}
			break;

	case WS_FINDKEY:
	case WS_REALSET : //			  						Ricerca la Chiave selezionata
	case WS_FIND : //			  						Ricerca la Chiave selezionata

	// --------------------------------------------------
	// CERCA L'ULTIMO (POSIZIONA LO SCROLL ALLA FINE    |
	// --------------------------------------------------
	case WS_FINDLAST:
			 if (!lpSdb->fCallLock) DB_find(iEvent,info,str,lpSdb);
			 break;
	
	// --------------------------------------------------
	// CONTROLLA SE ESISTE IL CODICE 
	// --------------------------------------------------
	case WS_CHECK:
			return (SINT *) DB_Check(info,str,lpSdb);

	// -----------------------------------
	// SETTA SELEZIONE RECORD            |
	// -----------------------------------
	case WS_SEL : //			  			Settaggio selez
			{ 
			 HREC Record;
			 if (lpSdb->ExtFunz==NULL) ehExit("SdbDriver:No ext/disp function");
			 if (!lpSdb->Mode)  // DB Mode
			 {
				lpSdb->ws.selez=info;
				Record=info;
			 }
			 else
			 {	
				 if (lpSdb->ws.selez==-1) break;
				 memoRead(ADB_info[lpSdb->Hdb].AdbFilter->hdlRecords,lpSdb->ws.selez*sizeof(HREC),&Record,sizeof(Record));
			 }
//			 (lpSdb->ExtFunz)(cmd,NULL,NULL,str,info,lpSdb->Hdb,lpSdb->IndexNum);
			 (lpSdb->ExtFunz)(iEvent,NULL,Record,str,lpSdb->Hdb,lpSdb->IndexNum);
			}
			 break;

	// -------------------------------------
	// SETTA L'OFFSET  (Solo Modo O_SCROLL |
	// -------------------------------------
	case WS_OFF : //			  			Settaggio offset
			if (!lpSdb->Mode) ehExit("SdbDriver:Usato da oggetto O_SCROLL");
			break;

	// -------------------------------------
	// RITORNA il Puntatore al Record      |
	// -------------------------------------
	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata

			if (lpSdb->fCallLock) break;

			rit.keypt=adb_DynRecBuf(lpSdb->Hdb);//lpSdb->RecBuffer;
			// Modo 0: Normale direct
			if (!lpSdb->Mode)
				{
				 rit.record=lpSdb->ws.selez;
				 //rit.keypt=NULL;
				 //for (b=0;b<lpSdb->ws.numcam;b++)// era (b=a+1
				 //{if (lpSdb->buf[b].record==lpSdb->ws.selez) {rit.keypt=lpSdb->buf[b].keypt;break;}
				 //}
				}
				else
			// Modo 1: Normale direct
				{
				  HREC Hrec;
					//if (lpSdb->ws.selez==-1) ehExit("E no ?");
				  if (lpSdb->ws.selez!=-1)
				  {
					  memoRead(ADB_info[lpSdb->Hdb].AdbFilter->hdlRecords,
												lpSdb->ws.selez*sizeof(HREC),
												&Hrec,
												sizeof(HREC));
					rit.record=Hrec;
				  }
				  else rit.record=ADBNULL;
				}

			 
			 if (rit.record!=ADBNULL) adb_get(lpSdb->Hdb,rit.record,lpSdb->IndexNum);
//			 iCount--;
			 return &rit;

	// -------------------------------------
	// RITORNA Selez ??????????            |
	// -------------------------------------
	case WS_REALGET:
			 ehExit("SDB:Uso RealGet ?");
			 //return (&lpSdb->ws.selez);

	// -------------------------------------
	// Refresh a ON                        |
	// -------------------------------------
	case WS_REFON : //			  			       Richiesta di refresh schermo
			 lpSdb->ws.refre=ON;
			 break;

	// -------------------------------------
	// Refresh a OFF                       |
	// -------------------------------------
	case WS_REFOFF : //			  					Schermo refreshato
			 lpSdb->ws.refre=OFF;
			 break;

	// -------------------------------------
	// Richiesta di Stampa dei Dati        |
	// -------------------------------------
	case WS_DISPLAY : //			  			Richiesta buffer
			 
			if (lpSdb->fCallLock) break;
			if (lpSdb->ExtFunz==NULL) ehExit("SdbDriver:No ext/disp function");
			psExt=(EH_DISPEXT *) str;

			if (psExt->ncam==-10) // Il Titolo
			 {
				(lpSdb->ExtFunz)(0,NULL,0,str,lpSdb->Hdb,lpSdb->IndexNum);
				break;
			 }

			 //printf("WDSP: %d|",psExt->ncam);
			 if (lpSdb->Mode) // Modo con List
				{
					//if ((psExt->ncam+lpSdb->ws.offset)<0) ehExit("poik");
				 // In caso di ricostruzione del filtro e contemporanemente refresh di un oggetto
				 // può capitare che Handle non sia valido
				 if (ADB_info[lpSdb->Hdb].AdbFilter->hdlRecords<0) break;
				 memoRead(ADB_info[lpSdb->Hdb].AdbFilter->hdlRecords,
							  (LONG) (psExt->ncam+lpSdb->ws.offset)*sizeof(LONG),
							  &Hrec,sizeof(LONG));
				}
				else // Modo Diretto in Db
				{Hrec=lpSdb->WinScr[psExt->ncam].record;}

					/*
				if (Hrec!=ADBNULL) // Se esiste il record
				{
				 adb_get(lpSdb->Hdb,Hrec,-1);
				 (lpSdb->ExtFunz)(0,
								  lpSdb->RecBuffer,
								  0,
								  str,
								  lpSdb->Hdb,
								  lpSdb->IndexNum);
								  */
				if (Hrec!=ADBNULL) // Se esiste il record
				{
				 adb_ErrorView(FALSE);
				 if (!adb_get(lpSdb->Hdb,Hrec,-1))
				 {
				 (lpSdb->ExtFunz)(0,
								  adb_DynRecBuf(lpSdb->Hdb),//lpSdb->RecBuffer,
								  0,
								  str,
								  lpSdb->Hdb,
								  lpSdb->IndexNum);
				 }
				 adb_ErrorView(TRUE);
				}

//				}
			 //sonic(1000,1,2,3,4,5); getch();
			 break;

	// -------------------------------------
	// COMUNICA LA FUNZIONE ESTERNA        |
	// -------------------------------------
	case WS_EXTNOTIFY: //	 era WS_EXTFUNZ

			 //if (lpSdb->fCallLock) break;
			 lpSdb->ExtFunz=(SINT (*)(SINT cmd,CHAR *PtDati,LONG  dato,
							          void  *str,SINT Hdb,SINT IndexNum)) str;
			 lpSdb->Mode=-1; // Modalità disattivata
												 /*
												 (SINT (*)(SINT cmd,
																 EH_DISPEXT *psExt,
																 struct WINSCR buf[],
																 CHAR *PtBuf,
																 LONG SizeRec,
																 SINT Hdb,
																 SINT IndexNum)) str;
																 */
			 break;

	// -------------------------------------
	// COMUNICA Handle del Dbase da Usare  |
	// -------------------------------------
	case WS_HDB : //	 era WS_HDB
			 lpSdb->Hdb=(SINT) info;
			 // Succede se SdbHDB è prima della dichiarazione della funzione
			 if (lpSdb->ExtFunz==NULL)
					ehExit("A) No Funzdisp");
					else
					(lpSdb->ExtFunz)(WS_HDB,NULL,0,str,lpSdb->Hdb,lpSdb->IndexNum);
			 break;

	// ---------------------------------------
	// COMUNICA L'indice attualmente in Uso  |
	// ---------------------------------------

	case WS_IDX : //	era WS_IDX
			 lpSdb->IndexNum=(SINT) info;
			 // Succede se SdbINX è prima della dichiarazione della funzione
			 if (lpSdb->ExtFunz==NULL)
					ehExit("B) No Funzdisp");
					else
					(lpSdb->ExtFunz)(WS_IDX,NULL,0,str,lpSdb->Hdb,lpSdb->IndexNum);

			 break;

	// -------------------------------------
	// SETTA IL MODO ADBFILTER             |
	// SE info==ON Prova a fare la Lista   |
	// -------------------------------------

	case WS_ADBFILTER : //			  			Richiesta buffer
			 if (lpSdb->ExtFunz==NULL) ehExit("SdbDriver01?");
			 // Info  = ON/OFF
			 lpSdb->AdbFilter=(BOOL) info;
			 (lpSdb->ExtFunz)(iEvent,0,info,str,lpSdb->Hdb,lpSdb->IndexNum);
//			 if ((lpSdb->ExtFunz)(cmd,0,info,str,lpSdb->Hdb,lpSdb->IndexNum))
			 break;
	// -------------------------------------
	// SETTA IL FILTRO ESTERNO             |
	// -------------------------------------
	case WS_SETFILTER : //			  			Richiesta buffer

			 // Info  = ON/OFF
			 lpSdb->ExtFilter=(BOOL) info;
			 (lpSdb->ExtFunz)(iEvent,0,info,str,lpSdb->Hdb,lpSdb->IndexNum);
			 break;

#ifdef _WIN32
	
	case WS_LINEVIEW: //	 
		// Creo nuova zona di memoria addatta a contenere la nuova dimensione di visione
		//_d_("Prima=%d Adesso=%d",lpSdb->ws.numcam,info); ehSleep(600);
        {
		 HREC LastTop=ADBNULL;
		 if (lpSdb->WinScrHdl!=-1) LastTop=lpSdb->WinScr[0].record;
		 //if (info<1) info=1; // Test
		 LArrayMake(lpSdb,info);
		 lpSdb->ws.numcam=info;
		 lpSdb->Dove=0;
		 lpSdb->End=(SINT) lpSdb->ws.numcam;
		 if ((!lpSdb->Mode)&&(LastTop!=ADBNULL)) adb_get(lpSdb->Hdb,LastTop,lpSdb->IndexNum);
		 DB_buf(lpSdb);
		}
		break;

	case WS_LINEEDIT: //	 
		lpSdb->ws.Enumcam=info;
		break;
#endif

	case WS_LOCK:
		lpSdb->fCallLock=info;
		break;

	case WS_EVENT:
		//lpSdb->fCallLock=info;
		(lpSdb->ExtFunz)(iEvent,(CHAR *) objCalled,info,str,lpSdb->Hdb,lpSdb->IndexNum);
		break;



	default:
//	case WS_SETFLAG : //			  			Richiesta buffer
			 if (lpSdb->ExtFunz==NULL) ehExit("SdbDriver02?");

			 (lpSdb->ExtFunz)(iEvent,0,info,str,lpSdb->Hdb,lpSdb->IndexNum);
			 break;

	 }

	PtScr=lpSdb->WinScr; if (!PtScr) PtScr=&rit;
//	iCount--;
	return PtScr;
}

// --------------------------------------------------
//  DB_GetLast                                      |
//  Setta il db nella posizione degli ultimi        |
//  record visualizzabili                           |
//                                                  |
//  Non ritorna nulla                               |
//                                                  |
// --------------------------------------------------

static void DB_GetLast(SDBEXT *Sdb)
{
	SINT a,test;
	LONG Hrec;

	// -----------------------------------------------
	//  Ricalcolo il puntatore partendo dal Basso    |
	//  e tornando indietro                          |
	// -----------------------------------------------

	//sonic(2000,1,2,3,4,8);
	//if (Sdb->ExtFilter) sonic(1000,1,2,3,4,8);
	/*
	if (adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_LAST,"",B_RECORD)) return;
	if (Sdb->ExtFilter)
		 {
			test=(Sdb->ExtFunz)(WS_FILTER,NULL,NULL,NULL,NULL,Sdb->Hdb,Sdb->IndexNum);
			if ((test==OFF)||(test==O_KEYSTS_DOWN_MOUSEOUT)) a=0;
		 }
		*/

	if (Sdb->RecDN)  {a=adb_get(Sdb->Hdb,Sdb->RecDN,Sdb->IndexNum);
					  if (a) Sdb->RecDN=ADBNULL;
						}
	if (!Sdb->RecDN) {a=adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_LAST,"",B_RECORD);}
	if (a) return;

#ifdef _WIN32
	for (a=0;a<(Sdb->ws.Enumcam-1);)
#else
	for (a=0;a<(Sdb->ws.numcam-1);)
#endif
			{
//			 rifo4:

			 // Controllo di essere arrivato in cima
			 if (Sdb->RecUP)
				{
				 adb_position(Sdb->Hdb,&Hrec);
				 //sonic(200,1,2,3,4,5);
				 if (Hrec==Sdb->RecUP) return;
				}

//			 if (adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD)) break;

			 // ------------------------------------------------------
			 //  Se c'è il filtro controllo la validità del record   |
			 // ------------------------------------------------------
			 if (Sdb->ExtFilter)
				 {
					test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
					//if (test==OFF) break; // Fine della ricerca
					//if ((test==OFF)||(test==O_KEYSTS_DOWN_MOUSEOUT)) goto rifo4; // Campo non valido ma continua
					if (test==ON) a++;
				 }
					else
				 {a++;test=ON;}

			 // Se il record ultimo non c'è me lo trovo
			 if (!Sdb->RecDN&&(test==ON)) {adb_position(Sdb->Hdb,&Sdb->RecDN);}
			 if (adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD)) break;
			}
}


// --------------------------------------------------
//  DB_FIND Ricerca un determinato record           |
//                                                  |
//  Ritorna : 0 - Nessun spostamento                |
//            1 - Mandare in stampa                 |
//                                                  |
// --------------------------------------------------
static SINT DB_find(SINT cmd,LONG info,CHAR *str,SDBEXT *Sdb)
{
	float percent,perc2;
	SINT test;
	HREC Hrec;
	struct WS_INFO *ws=&Sdb->ws;

	if (cmd==WS_FINDLAST)
		{
		if (Sdb->Mode)
		 {
			HREC Hrec;
			if (!ADB_info[Sdb->Hdb].AdbFilter->iRecordsReady) return 0;
			memoRead(ADB_info[Sdb->Hdb].AdbFilter->hdlRecords,
						  (ADB_info[Sdb->Hdb].AdbFilter->iRecordsReady-1)*sizeof(HREC),
						  &Hrec,sizeof(HREC));
			adb_get(Sdb->Hdb,Hrec,Sdb->IndexNum);
		 }
		 else
		 {   
			 if (Sdb->ExtFilter!=ADBNULL) {Sdb->RecDN=ADBNULL;DB_GetLast(Sdb);}
			                              else
										  {if (adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_LAST,"",B_RECORD)) return 0;}
		 }
		 goto after;
		}

	// Primo controllo
	if (((*str==0)||(ws->maxcam==0))&&(cmd==WS_FIND)) return 0;

	// ----------------------------------------------------------------
	// Chiedo alla funzione esterna quello che mi è stato chiesto     |
	// ----------------------------------------------------------------
	if (!Sdb->ExtFunz) ehExit("DB_find: Funzione esterna assente");
	test=(Sdb->ExtFunz)(cmd,0,info,str,Sdb->Hdb,Sdb->IndexNum);
	if (test==OFF) return 0; // Nessuna ricerca effettuata

	adb_position(Sdb->Hdb,&Hrec);
	adb_get(Sdb->Hdb,Hrec,Sdb->IndexNum);

	after:

	// ----------------------------------------------------------------
	// Se sono con una lista vado a cercare HREC                      |
	// ----------------------------------------------------------------
	if (Sdb->Mode)
	 {
		 BOOL Check=FALSE;
		 LONG l;
		 HREC Hrec,Hrec2;

         if (cmd!=WS_FINDLAST)
		 {
		  adb_position(Sdb->Hdb,&Hrec);
		  for (l=0;l<ADB_info[Sdb->Hdb].AdbFilter->iRecordsReady;l++)
			{
				memoRead(ADB_info[Sdb->Hdb].AdbFilter->hdlRecords,l*sizeof(HREC),&Hrec2,sizeof(HREC));
				if (Hrec==Hrec2) {Sdb->ws.selez=l;Check=TRUE;break;}
			}
		  if (!Check) {Sdb->ws.selez=-1;return 0;}

		 } else {Sdb->ws.selez=ADB_info[Sdb->Hdb].AdbFilter->iRecordsReady-1;}

#ifdef _WIN32
		 Sdb->ws.offset=Sdb->ws.selez-(Sdb->ws.Enumcam>>1);
		 if (Sdb->ws.offset<0) Sdb->ws.offset=0;
		 if (Sdb->ws.offset>(Sdb->ws.maxcam-Sdb->ws.Enumcam)) Sdb->ws.offset=Sdb->ws.maxcam-Sdb->ws.Enumcam;
		 if (Sdb->ws.offset<0) Sdb->ws.offset=0;
#else
		 Sdb->ws.offset=Sdb->ws.selez-(Sdb->ws.numcam>>1);
		 if (Sdb->ws.offset<0) Sdb->ws.offset=0;
		 if (Sdb->ws.offset>(Sdb->ws.maxcam-Sdb->ws.numcam)) Sdb->ws.offset=Sdb->ws.maxcam-Sdb->ws.numcam;
		 if (Sdb->ws.offset<0) Sdb->ws.offset=0;
#endif
		 return -1;
	 }
	 else
	 {
		 if (!adb_recnum(Sdb->Hdb)) return -1;
	 }
//	printf("OK2"); getch();

	// Controlla che sia buono
	//if (test)

	if (Sdb->ExtFilter)
	{
	 test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
	 //printf("test %d",test);
	 if (test!=ON) // Se non lo è setta la lista dall'inizio
					 {return 0;}
//						else
	}

	// Metto in ws->selez la selezione

	adb_position(Sdb->Hdb,&Hrec);
	ws->selez=Hrec;
	SdbDriver(Sdb->ObjClient,WS_SEL,Hrec,"");//Sdb->Hdb,Sdb->IndexNum);

	// Cerco la posizione in percentuale
	adb_findperc(Sdb->Hdb,Sdb->IndexNum,&percent);// Legge la posizione in percentuale

    //win_infoarg("%f %x [%s] ",percent,Hrec,adb_FldPtr(Sdb->Hdb,"CODICE"));
	perc2=percent*(ws->maxcam-1)/100;
//	_d_("[%d] - [%d]",ws->offset,(SINT) perc2);
	ws->offset=(LONG) perc2;
	ws->refre=ON;
//	Sdb->buf[0].record=NULL;
    if (Sdb->WinScrHdl==-1) ehExit("Hdl ?");
	Sdb->WinScr[0].record=ADBNULL;
	ws->dbult=-1; // <-- aggiunta
	ws->refre=ON;

	//efx2();
	//_d_("%4.2f",(double) percent);
	if (ws->offset>(ws->maxcam-1)) ws->offset=ws->maxcam-1;
	if (ws->offset<0) ws->offset=0;

    //win_infoarg("Offset %d",ws->offset);
	//	scr_adbExt1(WS_BUF,0,"");
	Sdb->Dove=0; Sdb->End=(SINT) ws->numcam;
	DB_buf(Sdb);

#ifdef _WIN32
	if (ws->maxcam<=ws->Enumcam)
			 {//scr_adbExt1(WS_DBSEEK,ws->numcam*-1,"");}
				if (DB_seek(ws->Enumcam*-1,Sdb)) DB_buf(Sdb);
			 }
			 else
			 {SINT a,righe;

				//scr_adbExt1(WS_DBSEEK,(ws->numcam/2)*-1,"");
				if (DB_seek((ws->Enumcam/2)*-1,Sdb)) DB_buf(Sdb);
				
				// Conto quante righe vuote ci sono
				righe=0;
				for (a=0;a<(SINT) ws->Enumcam;a++)
					 {//if (Sdb->buf[a].record==NULL) righe++;
						if (Sdb->WinScr[a].record==ADBNULL) righe++;
					 }
				
				// Ricalibra la fine dbase
				if (righe>0)
					{
					 //scr_adbExt1(WS_DBSEEK,-righe,"");
					 if (DB_seek(-righe,Sdb)) DB_buf(Sdb);
					 ws->offset=(ws->maxcam-ws->Enumcam);
					}
			 }
#else
	if (ws->maxcam<=ws->numcam)
			 {//scr_adbExt1(WS_DBSEEK,ws->numcam*-1,"");}
				if (DB_seek(ws->numcam*-1,Sdb)) DB_buf(Sdb);
			 }
			 else
			 {SINT a,righe;

				//scr_adbExt1(WS_DBSEEK,(ws->numcam/2)*-1,"");
				if (DB_seek((ws->numcam/2)*-1,Sdb)) DB_buf(Sdb);
				
				// Conto quante righe vuote ci sono
				righe=0;
				for (a=0;a<(SINT) ws->numcam;a++)
					 {//if (Sdb->buf[a].record==NULL) righe++;
						if (Sdb->WinScr[a].record==ADBNULL) righe++;
					 }
				
				// Ricalibra la fine dbase
				if (righe>0)
					{
					 //scr_adbExt1(WS_DBSEEK,-righe,"");
					 if (DB_seek(-righe,Sdb)) DB_buf(Sdb);
					 ws->offset=(ws->maxcam-ws->numcam);
					}
			 }
#endif			 
/*
	if ((ws->offset==(ws->maxcam-ws->numcam))&&(ws->numcam!=ws->Enumcam))
	{
		ws->offset++; 
		efx2();
		DB_GetLast(Sdb);
	}
*/
	return -1;
}

// --------------------------------------------------
//  DB_CHECK Ricerca se esiste un codice
//                                                  |
//  Ritorna : TRUE  Esiste
//            FALSE Non Esiste
//                                                  |
// --------------------------------------------------
static BOOL DB_Check(LONG info,CHAR *str,SDBEXT *Sdb)
{
	SINT test;
	HREC hRec;
	struct WS_INFO *ws=&Sdb->ws;

	// Primo controllo
	if (!ws->maxcam) return FALSE;

	// ----------------------------------------------------------------
	// Chiedo alla funzione esterna di effettuare la ricerca
	// ----------------------------------------------------------------
	test=(Sdb->ExtFunz)(WS_FIND,0,info,str,Sdb->Hdb,Sdb->IndexNum);
	if (test==OFF) return 0; // Nessuna ricerca effettuata

	adb_position(Sdb->Hdb,&hRec);
	// ----------------------------------------------------------------
	// Se sono con una lista vado a cercare HREC                      |
	// ----------------------------------------------------------------
	if (Sdb->Mode)
	 {
		 BOOL Check=FALSE;
		 LONG l;
		 HREC Hrec2;

		 for (l=0;l<ADB_info[Sdb->Hdb].AdbFilter->iRecordsReady;l++)
			{
				memoRead(ADB_info[Sdb->Hdb].AdbFilter->hdlRecords,l*sizeof(HREC),&Hrec2,sizeof(HREC));
				if (hRec==Hrec2) {Sdb->ws.selez=l;Check=TRUE;break;}
			}
		 if (!Check) return FALSE;
	 }
	 else
	 {
		 if (!adb_recnum(Sdb->Hdb)) return FALSE; 
	 }
	return TRUE;
}

// --------------------------------------------------
//  DB_ZONE Stabilisce il primo ed ultimo record    |
//          del dbase                               |
//                                                  |
//                                                  |
//  ritorno NOACTIVE zona globale senza filtro      |
//          OFF      Nessun record                  |
//          ON       Limiti caricati                |
//                                                  |
// --------------------------------------------------
static SINT DB_zone(SDBEXT *Sdb)
{
	SINT a;

	//printf("DBZONE:\n");
	//getch();
	//mouse_input();
	if (Sdb->Mode) ehExit("JFK");

	// -----------------------------------
	// A) Richiesta del Primo Record     |
	// -----------------------------------

	Sdb->RecUP=ADBNULL; Sdb->RecDN=ADBNULL;
	Sdb->FiltUP=0; Sdb->FiltDN=0;

	if (!Sdb->ExtFilter)
		 a=NOACTIVE;
		 else
		 a=(Sdb->ExtFunz)(WS_FIRST,0,0,0,Sdb->Hdb,Sdb->IndexNum);

	// ----------------------------------------------
	// B) Nessun filtro è attivo                    |
	//    Carico la struttura nel modo ottimale     |
	// ----------------------------------------------

	if (a==NOACTIVE) // Per eredità
		 {Sdb->ExtFilter=FALSE;
//			ws->maxcam=adb_recnum(Sdb->Hdb);
			// Posizioni delle percentuali
			Sdb->FiltUP=0; Sdb->FiltDN=100;
			if (!adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_FIRST,"",B_RECORD)) adb_position(Sdb->Hdb,&Sdb->RecUP);
			if (!adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_LAST,"",B_RECORD)) adb_position(Sdb->Hdb,&Sdb->RecDN);
			return NOACTIVE;
		 }

	// ----------------------------------------------
	// C) Ritorno ad OFF = 0record                  |
	// ----------------------------------------------

	if (a!=ON) return OFF;

	// -------------------------------------------------------------
	// D) A filtro Attivo controllo e registro la Prima Posizione  |
	// -------------------------------------------------------------

	Sdb->ExtFilter=TRUE;

	do {  // Loopizzo
	 a=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
	 if (a==OFF) return OFF; // Non ci sono record validi
	 if (a==ON) break;     // Trovato
	} while	(!adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD));

	// Registro la prima posizione
	adb_findperc(Sdb->Hdb,Sdb->IndexNum,&Sdb->FiltUP);
	adb_position(Sdb->Hdb,&Sdb->RecUP);

	// ------------------------------------------------
	// E) Ricerco l'ultima posizione                  |
	// ------------------------------------------------
	a=(Sdb->ExtFunz)(WS_LAST,0,0,0,Sdb->Hdb,Sdb->IndexNum);

	// Se risponde
	if (a)
		 {
			do {  // Loopizzo
			 a=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
			 // CONDIZIONE DA CONTROLLARE
			 if (a==OFF) return OFF; // Non ci sono record validi
			 if (a==ON) break;     // Trovato
			 } while (!adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD));

			// Registro l'ultima posizione
			adb_findperc(Sdb->Hdb,Sdb->IndexNum,&Sdb->FiltDN);
			adb_position(Sdb->Hdb,&Sdb->RecDN);
		 }

 return ON;
}

// --------------------------------------------------
//  DB_BUF  Carica il buffer con i record letti     |
//                                                  |
//  non ritorna nulla                               |
//                                                  |
// --------------------------------------------------
static void DB_buf(SDBEXT *Sdb)
{
	SINT a,b,test;
	LONG Hrec;
	struct WS_INFO *ws=&Sdb->ws;

	if (Sdb->Mode) // Se sono in modalità puntatori
	{
	 return;
	}

	//printf("DBBUF:");
	//getch();
	//mouse_input();

	if (ws->dbult==ADBNULL) // prima volta
		 {
			if (ws->maxcam==0) {a=0;goto canc;}
			//(Sdb->ExtFunz)(WS_FIRST,NULL,NULL,NULL,NULL,Sdb->Hdb,Sdb->IndexNum);
			if (!Sdb->RecUP) DB_zone(Sdb);
			ws->offset=0;
			if (!Sdb->RecUP) return; // Errore
			// Leggo il primo record
			adb_get(Sdb->Hdb,Sdb->RecUP,Sdb->IndexNum);
		}
		else
			if (ws->dbult==Sdb->WinScr[0].record) return; // il buffer è ok
//			if (ws->dbult==Sdb->buf[0].record) return; // il buffer è ok
        
		// -------------------------------------
		//  Carica i nuovi valori nella cache  !
		// -------------------------------------
		for (a=0;a<Sdb->End;a++)
		{
//			 memcpy(Sdb->buf[Sdb->Dove].keypt,Sdb->RecBuffer,(SINT) Sdb->RecSize);
//			 memmove(Sdb->buf[Sdb->Dove].keypt,Sdb->RecBuffer,(SINT) Sdb->RecSize);
			 // #?#
//			 adb_position(Sdb->Hdb,&Sdb->buf[Sdb->Dove].record);

			 adb_position(Sdb->Hdb,&Sdb->WinScr[Sdb->Dove].record);
			 Sdb->Dove++;

			 rifai:
			 // Fine corsa misurata in apertura
			 if (Sdb->RecDN)
					{adb_position(Sdb->Hdb,&Hrec);
					 if (Hrec==Sdb->RecDN) break;
					}
			 if (adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD)) break;
			 if (Sdb->ExtFilter)
				{
				 test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);

				 if (test==OFF) break; // Fine della ricerca
				 if (test==-2||test==2) goto rifai; // Campo non valido ma continua
				}
		}

		// Devo trovarmi l'ultima posizione valida
		if (!Sdb->RecDN)
		{//win_info("Controlla");
				adb_findperc(Sdb->Hdb,Sdb->IndexNum,&Sdb->FiltDN);
				adb_position(Sdb->Hdb,&Sdb->RecDN);
		}

			// Cancella il seguito
			canc:
			for (b=a;b<Sdb->End;b++)// era (b=a+1
			 {Sdb->WinScr[Sdb->Dove].record=ADBNULL;
				//Sdb->buf[Sdb->Dove].record=NULL;
				//strcpy(Sdb->buf[Sdb->Dove].keypt,"");
				Sdb->Dove++;
			 }

			ws->dbult=Sdb->WinScr[0].record;
}

// --------------------------------------------------
//  DB_PERC sposta la lettura del db ad una         |
//          determinata percentuale                 |
//                                                  |
//  Ritorna : 0 - Nessun spostamento                |
//            1 - Mandare in stampa                 |
//                                                  |
// --------------------------------------------------
static SINT DB_perc(LONG info,SDBEXT *Sdb)
{
	SINT a,test;
	float percent,perc2;
	SINT FineCorsa=OFF;
	LONG hrec,OriRec;
	struct WS_INFO *ws;

	ws=&Sdb->ws;

	perc2=(float) ws->maxcam-ws->numcam;
	perc2=perc2*100/ws->maxcam;

	if (info==-1) // Forzo la fine
		{percent=100;
		 ws->offset=(LONG) perc2;
		}
		else
		{
		 percent=(float) info*perc2/(ws->maxcam-ws->numcam);
		 ws->offset=info;

	// Controllo limiti con filtro
	if (Sdb->ExtFilter)
					{float Range;
					 Range=Sdb->FiltDN-Sdb->FiltUP;
					 percent=Sdb->FiltUP+((Range/100)*percent);
					}
		}

	// 	Il cursore è settato in testa
	if (percent<.1F) {ws->offset=0;goto goperc1;}

	adb_getperc(Sdb->Hdb,Sdb->IndexNum,percent);

	// -----------------------------------------------------------
	// Se il filtro è attivo controlla la validità del record    |
	// -----------------------------------------------------------
	if (Sdb->ExtFilter)
		{do
		 {
		 test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
		 if (test==ON) goto goperc1;

		 if ((Sdb->RecUP)||(test==OFF))
				{
				 adb_position(Sdb->Hdb,&hrec);
				 //sonic(200,1,2,3,4,5);
				 if (hrec==Sdb->RecUP) break;
				}

		 // Sei alla fine
		 if (Sdb->RecDN)
				{
				 adb_position(Sdb->Hdb,&hrec);
				 //sonic(200,1,2,3,4,5);
				 if (hrec==Sdb->RecDN)
					 {
						DB_GetLast(Sdb);
						return -1;
					 }
				}

		 } while (!adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD));
		 ws->offset=0;
		}

 goperc1:

 Sdb->Dove=0;
 Sdb->End=(SINT) ws->numcam;
 //Sdb->buf[0].record=NULL;
 Sdb->WinScr[0].record=ADBNULL;

 ws->refre=ON;

 // -----------------------------------------------------------
 //  Se l'offset è all'inizio stampa e basta                  |
 // -----------------------------------------------------------

 if (ws->offset<=0)
		{ws->offset=0;
		 //(Sdb->ExtFunz)(WS_FIRST,NULL,NULL,NULL,NULL,Sdb->Hdb,Sdb->IndexNum);

		 if (Sdb->RecUP) {a=adb_get(Sdb->Hdb,Sdb->RecUP,Sdb->IndexNum);
											if (a) Sdb->RecUP=0;
											}
		 return -1;
		}

 // -----------------------------------------------------------
 //  Controlla quanti ce ne sono                              |
 // -----------------------------------------------------------
 adb_position(Sdb->Hdb,&OriRec);
 for (a=1;a<(SINT) ws->numcam;a++)
 {
	erifa:

	// Sei alla fine
	if (Sdb->RecDN)
			{adb_position(Sdb->Hdb,&hrec);
			 //sonic(200,1,2,3,4,5);
			 if (hrec==Sdb->RecDN)
					 {FineCorsa=ON;
						break;
					 }
				}

	if (adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD))
		 {FineCorsa=ON;break;}

	if (Sdb->ExtFilter)
		{test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
		 if (test!=ON) goto erifa;
		}
 }

 if (FineCorsa)
		 {if (!Sdb->RecDN) {adb_position(Sdb->Hdb,&Sdb->RecDN);}
			ws->offset=ws->maxcam-ws->numcam;DB_GetLast(Sdb);
		 }
		 else
		 {adb_get(Sdb->Hdb,OriRec,Sdb->IndexNum);}

 return -1;
}


// --------------------------------------------------
//  DB_SEEK sposta la lettura del db in avanti o    |
//          indietro                                |
//                                                  |
//  info = Quanti record                            |
//                                                  |
//  Ritorna : 0 - Nessun spostamento                |
//            1 - Mandare in stampa                 |
//                                                  |
// --------------------------------------------------
static BOOL DB_seek(LONG info,SDBEXT *Sdb)
{
	SINT  SReale;
	SINT  FlagCORSA=OFF;
	LONG  PtrFind;
	LONG  b;
	SINT  test;
	HREC  Hrec;
	HREC  HrecApp;
//	TCHAR serv[80];

	if (Sdb->Mode) return 0;
//	_d_("* offset=%d maxcam=%d Enumcam=%d",Sdb->ws.offset,Sdb->ws.maxcam,Sdb->ws.Enumcam);

	// -------------------------------------------------------------
	//      RICERCA VERSO IL BASSO   =  SCROLL VERSO L'ALTO
	// -------------------------------------------------------------
	SReale=0;

	if (info>0) // Scroll verso il basso
	{
		FlagCORSA=OFF;

		// Leggo l'ultimo record
//				 PtrFind=Sdb->buf[(SINT) (Sdb->ws.numcam-1)].record;
		PtrFind=Sdb->WinScr[(SINT) (Sdb->ws.numcam-1)].record;
		// Se è a NULL pi— in gi— no si può andare
		if (PtrFind==ADBNULL) return 0;
		adb_get(Sdb->Hdb,PtrFind,Sdb->IndexNum);

		// ------------------------
		// Scrollo di info in gi— |
		// ------------------------
		for (b=0;b<info;b++)
		{
			rifai2:
			// Fine corsa misurata in apertura
			if (Sdb->RecDN)
			{adb_position(Sdb->Hdb,&Hrec);
			 if (Hrec==Sdb->RecDN) {FlagCORSA=ON; break;}
			}

			if (adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD))
				{FlagCORSA=ON;
				 break;// Fine corsa
				}

			// ----------------------------------------------------
			// Se c'è il filtro controllo la validità del record  |
			// ----------------------------------------------------

			if (Sdb->ExtFilter)
			{
				test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
				if (test==OFF) break; // Fine della ricerca

				if (test==-2||test==2) goto rifai2; // Campo non valido ma continua
			}

			SReale++; // Conta quanti ne trovo
			memmove(Sdb->WinScr,  // Destinazione
					Sdb->WinScr+1,// Sorgente
					(WORD) ((Sdb->ws.numcam-1)*sizeof(struct WINSCR))); // Numero byte
			adb_position(Sdb->Hdb,&HrecApp);
			Sdb->WinScr[(WORD) Sdb->ws.numcam-1].record=HrecApp;

			// Aggiorna l'ultimo movimento
			// #?#
			//adb_position(Sdb->Hdb,&Sdb->buf[(SINT) (Sdb->ws.numcam-1)].record);
		}

		// Aggiorna l'offset
		// Se c'è stato fine corsa ricalcolo l'offset
		// (offset può essere sbagliato per arrotondamento in Perc)
		Sdb->ws.offset+=SReale;
		//if (FlagCORSA) ws.offset=(ws.maxcam-ws.numcam);

#ifdef _WIN32
		//_d_("offset=%d maxcam=%d Enumcam=%d",Sdb->ws.offset,Sdb->ws.maxcam,Sdb->ws.Enumcam);
		if (FlagCORSA)
		{
		/*
			if (Sdb->ws.offset==(Sdb->ws.maxcam-Sdb->ws.Enumcam))  
		  {
			if (Sdb->WinScr[(WORD) Sdb->ws.numcam-1].record!=NULL) Sdb->ws.offset=(Sdb->ws.maxcam-Sdb->ws.numcam);
			efx2();

		  }
		  */
		  // Se il passo in avanti è 1 
		  if (info==1)
		  {
			 // Se l'offset è giusto non faccio niente
			 if (Sdb->ws.offset==(Sdb->ws.maxcam-Sdb->ws.Enumcam)) return 0;	    
		  }				

		  if ((SReale==0)&&(Sdb->ws.offset==(Sdb->ws.maxcam-Sdb->ws.numcam)))
			{
			  Sdb->ws.dbult=-1;
			  Sdb->ws.offset=(Sdb->ws.maxcam-Sdb->ws.Enumcam);
			  //win_infoarg("%d",Sdb->ws.offset);
			  Sdb->ws.refre=ON;
			  Sdb->Dove=0; Sdb->End=(SINT) Sdb->ws.Enumcam;
			  DB_GetLast(Sdb);
			  return -1;
			}
		// --------------------------------------------------------
		// Se l'offset> supera l'offset massimo                   |
		// Ricalcola l'offset e ristampa tutto                    |
		// --------------------------------------------------------
			if (Sdb->ws.offset!=(Sdb->ws.maxcam-Sdb->ws.Enumcam))
			{
				Sdb->ws.dbult=-1;
				Sdb->ws.offset=(Sdb->ws.maxcam-Sdb->ws.Enumcam);
				//win_infoarg("%d",Sdb->ws.offset);
				Sdb->ws.refre=ON;
				Sdb->Dove=0; Sdb->End=(SINT) Sdb->ws.Enumcam;
				DB_GetLast(Sdb);
				return -1;
			}
		}
#else
		if ( (FlagCORSA)&&
			 (info==1)  &&
			 (Sdb->ws.offset==(Sdb->ws.maxcam-Sdb->ws.numcam)) ) return 0;
		// --------------------------------------------------------
		// Se l'offset> supera l'offset massimo                   |
		// Ricalcola l'offset e ristampa tutto                    |
		// --------------------------------------------------------
		if ((FlagCORSA)||(Sdb->ws.offset>(Sdb->ws.maxcam-Sdb->ws.numcam)))
		{
			//  ws.koffset=ws.offset-(ws.maxcam-ws.numcam)-1;
//				sprintf(serv,"offset %ld  maxnum %ld numcam %ld ",
//								ws->offset,ws->maxcam,ws->numcam);
//				Adispfm(0,0,15,0,SET,"SMALL F",3,serv);
			Sdb->ws.offset=(Sdb->ws.maxcam-Sdb->ws.numcam);
			Sdb->ws.refre=ON;
			Sdb->Dove=0; Sdb->End=(SINT) Sdb->ws.numcam;
			DB_GetLast(Sdb);
			return -1;
		}
#endif
	}
	else
	// --------------------------------------
	//       SPOSTAMENTO VERSO L'ALTO       !
	// --------------------------------------
	{
//				 PtrFind=Sdb->buf[0].record;
				 PtrFind=Sdb->WinScr[0].record;
				 if (PtrFind==ADBNULL) return 0;// Nessun Record

				 adb_get(Sdb->Hdb,PtrFind,Sdb->IndexNum);
				 for (b=0;b<(info*-1);b++)
				 {
					rifai3:
					 // Fine corsa misurata in apertura
					 if (Sdb->RecUP)
						{adb_position(Sdb->Hdb,&Hrec);
						 if (Hrec==Sdb->RecUP) break;
						}
					if (adb_find(Sdb->Hdb,Sdb->IndexNum,
											 B_GET_PREVIOUS,"",B_RECORD)) break;// Fine corsa

					if (Sdb->ExtFilter)
					 {
					 test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
					 if (test==OFF) break; // Fine della ricerca

					 if (test==-2||test==2) goto rifai3; // Campo non valido ma continua
					 }

					 SReale++;
					 memmove(Sdb->WinScr+1,  // Destinazione
							 Sdb->WinScr,// Sorgente
							 (WORD) ((Sdb->ws.numcam-1)*sizeof(struct WINSCR))); // Numero byte

					 adb_position(Sdb->Hdb,&HrecApp);
					 Sdb->WinScr[0].record=HrecApp;

					 // Scroll di uno verso il basso
					 /*
					 for (a=(SINT) (Sdb->ws.numcam-1);a>0;a--)
							{
							 p=farPtr(Sdb->PtrMemo,(Sdb->RecSize*(a-1)));
							 p2=farPtr(p,Sdb->RecSize);
							 memcpy(p2,p,(SINT) Sdb->RecSize);
							 Sdb->buf[a].record=Sdb->buf[a-1].record;
							 };

					 // #?#
					 adb_position(Sdb->Hdb,&Sdb->buf[0].record);
					 */
				 }
				Sdb->ws.offset-=SReale;
				if ((SReale==0)&&(Sdb->ws.offset>0)) {Sdb->ws.offset=0;Sdb->ws.refre=ON;}
				if (Sdb->ws.offset<0) {Sdb->ws.koffset=Sdb->ws.offset*-1;Sdb->ws.offset=0;}
	}


	// PRE CONTROLLO PRIMA DEL LOAD
	if (SReale==0) return 0; // Nessun spostamento
   //if (SReale>(ws.numcam-1)) SReale=(SINT) (ws.numcam-1);
 
	if (info>0) Sdb->Dove=(SINT) Sdb->ws.numcam-SReale; else Sdb->Dove=0;
	Sdb->End=SReale;
	if (Sdb->Dove<0) Sdb->Dove=0;
	if (Sdb->End>Sdb->ws.numcam) Sdb->End=(SINT) Sdb->ws.numcam;
//				adb_get(Sdb->Hdb,Sdb->buf[Sdb->Dove].record,Sdb->IndexNum);
	adb_get(Sdb->Hdb,Sdb->WinScr[Sdb->Dove].record,Sdb->IndexNum);
    return -1;
}

// --------------------------------------------------
//  DB_LOAD Pre-Caricamento iniziale                |
//                                                  |
//  Ritorna  0 = Tutto OK                           |
//          -1 = Nn Extern                          |
//                                                  |
//                                                  |
//                                                  |
// --------------------------------------------------
//	ehExit("Scr_adbExt1:No disp function");

static SINT DB_load(SDBEXT *Sdb)
{
	SINT swclex;
	SINT a,test;
	//LONG size;
	LONG Hrec;
	SINT Count;
	struct WS_INFO *ws=&Sdb->ws;

	Sdb->RecBuffer=NULL;
	if (Sdb->ExtFunz==NULL) return -1;

	ws->maxcam=adb_recnum(Sdb->Hdb);
	ws->offset=ADBNULL;
	ws->selez=ADBNULL; // Nessun selezionato
	ws->koffset=-1;
	ws->kselez=-1;
	ws->dispext=ON;
	ws->dbult=ADBNULL;
	ws->refre=ON;

	Sdb->RecSize=adb_recsize(Sdb->Hdb);
	Sdb->RecBuffer=adb_DynRecBuf(Sdb->Hdb);
	ws->sizecam=Sdb->RecSize; // ???
	//size=sizeof(struct WINSCR)*ws->numcam; // Calcola Grandezza Buffer
	//printf("Campi %ld[%ld]",ws->numcam,size);

	// -------------------------------------------------------------
	// Se sono in modalità Lista di puntatori con Filtro           |
	// esco subito                                                 |
	// -------------------------------------------------------------
	if (Sdb->Mode)
		{
	     ws->offset=ADBNULL;
	     ws->selez=-1; // Nessun selezionato
		 Sdb->ExtFilter=OFF;
		 ws->maxcam=ADB_info[Sdb->Hdb].AdbFilter->iRecordsReady;
		 Sdb->LastOffset=-1;
		 return 0;
		}

	// -------------------------------------------------------------
	// Creo la lista di puntatatori                                |
	// -------------------------------------------------------------
	LArrayMake(Sdb,ws->numcam);

	// -------------------------------------------------------------
	// Trova i limiti della zona e conta i record se c'è il filtro |
	// -------------------------------------------------------------

	ws->maxcam=0;
	a=DB_zone(Sdb);

	// Copertura Globale
	if (a==NOACTIVE)
		 {Sdb->ExtFilter=OFF;
			ws->maxcam=adb_recnum(Sdb->Hdb);
			goto NU;
		 }

	mouse_graph(0,0,"CLEX");

	// Nessun record nei limiti
	if (a==OFF) goto NU;

	// -------------------------------------------------------------
	// Se tutti e due i limiti sono stabiliti chiedo se devo fare  |
	// il calcolo proporzionale                                    |
	// -------------------------------------------------------------

	if (Sdb->RecUP&&Sdb->RecDN)
		{float perc1,perc2;
		 LONG Rec;

		 a=(Sdb->ExtFunz)(WS_DO,0,0,0,Sdb->Hdb,Sdb->IndexNum);
		 if (a)
			{
			 adb_get(Sdb->Hdb,Sdb->RecUP,Sdb->IndexNum);
			 adb_findperc(Sdb->Hdb,Sdb->IndexNum,&perc1);
			 adb_get(Sdb->Hdb,Sdb->RecDN,Sdb->IndexNum);
			 adb_findperc(Sdb->Hdb,Sdb->IndexNum,&perc2);

			 // Proporzione:  (perc2-perc1):100=x:maxrec
			 Rec=(LONG) ((perc2-perc1)*(float) adb_recnum(Sdb->Hdb)/100);
			 ws->maxcam=Rec;
			 goto NU;
			}
		}

	// ---------------------------------
	// Conto i Record                  |
	// ---------------------------------

	adb_get(Sdb->Hdb,Sdb->RecUP,Sdb->IndexNum);
	swclex=0;
	Count=0;
	do
	 {
		test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);

		// Fine della ricerca per fine Dbase
		if (test==OFF) break;

		// Registro la numerazione dei record Buoni
		if (test==ON) ws->maxcam++;

		// Se ho un record di limite lo controllo
		if (Sdb->RecDN)
			{adb_position(Sdb->Hdb,&Hrec);
			 if (Hrec==Sdb->RecDN) break;
			}

		// GRAFICA: Mano che si muove
		if (Count++>30)
			 {swclex^=1;
				if (swclex) mouse_graph(0,0,"CLEX");
										else
										mouse_graph(0,0,"CLEX2");
				Count=0;
			 }

	 } while (!adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD));

	 // --------------------------------------------
	 // Se non avevo il record limite lo registro  |
	 // --------------------------------------------

	 if (!Sdb->RecDN)
			{adb_position(Sdb->Hdb,&Sdb->RecDN);
			 adb_findperc(Sdb->Hdb,Sdb->IndexNum,&Sdb->FiltDN);
			}

	 NU:
	 mouse_graph(0,0,"MS01");
	 return 0;
}

void SdbPreset(struct OBJ *pojStruct,
			   CHAR *ObjName,
			   SINT (*FunzDisp)(SINT cmd,CHAR *PtDati,
			   LONG  dato,void  *str,
			   SINT Hdb,SINT IndexNum),
			   SINT Hdb,
			   SINT Index,
			   SINT Filter,
			   void *dato)
{
	struct OBJ *poj;
	poj=ObjFindStruct(pojStruct,ObjName); if (!poj) ehExit("DriverObjFind [%s] ?",ObjName);

	//obj_setCall(poj,ObjName); // Setto il puntatore all'oggetto
	SdbDriver(poj,WS_EXTNOTIFY,0,(CHAR *) FunzDisp);   // Setta l'indice da usare
	SdbDriver(poj,WS_HDB,Hdb,(CHAR *) dato); // Setta il dbase Handle
	SdbDriver(poj,WS_IDX,Index,(CHAR *)dato);   // Setta l'indice da usare
	if (Filter)
	 {
		SdbDriver(poj,Filter,ON,(CHAR *)dato);   // Setta l'indice da usare
	 }
}

#ifdef _WIN32
SINT SdbDeco(SINT cmd,CHAR *lpDati,LONG dato,void *str,SINT Hdb,SINT IndexNum)
{
	CHAR *p;
	EH_DISPEXT *psExt;
	static SINT ofsdesc=-1;
	static SINT ofscod=-1;
	static SINT dila=32;
	SINT a;
	LONG Hrec;

	switch (cmd)
	{
	case 0:// chiamata dal driver
		if (ofsdesc==-1) ehExit("ScrollCaus");
		p=lpDati;
		psExt=(EH_DISPEXT *) str;

		dispfm_h(psExt->px+1,psExt->py,psExt->col1,psExt->col2,psExt->hdl,p+ofsdesc);
		//a=font_len(p+ofscod,psExt->hdl);
		a=psExt->px+psExt->lx-dila;
		line(a-3,psExt->py,a-3,psExt->py+psExt->ly-1,sys.arColorBGR[2],SET);

		dispfm_h(a,psExt->py,psExt->col1,psExt->col2,psExt->hdl,p+ofscod);
		break;

 case WS_HDB: // Cambio di indice

		ofscod =adb_FldOffset(Hdb,str);
		dila   =adb_FldSize(Hdb,str)*8;
		ofsdesc=adb_FldOffset(Hdb,"DESCRIZIONE");
		break;

 case WS_FINDKEY: // Ricerca del codice
	 //RecSize=1;
	 //strupr(PtrMemo);
	 if (adb_Afind(Hdb,"PERDESC",B_GET_GE,_strupr(str),B_RECORD)) {break;}
	 adb_position(Hdb,&Hrec);
	 adb_get(Hdb,Hrec,IndexNum);

 case WS_FIND: // Ricerca del codice

	 if (adb_find(Hdb,0,B_GET_GE,str,B_RECORD)) {break;}
	 adb_position(Hdb,&Hrec);
	 adb_get(Hdb,Hrec,IndexNum);
	 return ON;

 case WS_IDX: // Cambio di indice
		break;
/*
 case WS_FIRST: // Setta il dbase al primo record
	 if (adb_find(Hdb,IndexNum,B_GET_FIRST,"",B_RECORD)) break;
	 return NOACTIVE;

 case WS_FILTER: // Controllo sul record se è accettabile
	 return ON;
*/
 case WS_KEYPRESS :
	if (key_press(ESC)) {strcpy(str,"ESC:ESC"); break;}
	break;

 }
 return OFF;
}
#else
SINT SdbDeco(SINT cmd,CHAR *lpDati,LONG dato,void *str,SINT Hdb,SINT IndexNum)
{
	CHAR *p;
	EH_DISPEXT *psExt;
	static SINT ofsdesc=-1;
	static SINT ofscod=-1;
	static SINT dila=32;
	SINT a;
	LONG Hrec;

	switch (cmd)
	{
	case 0:// chiamata dal driver
		if (ofsdesc==-1) ehExit("ScrollCaus");
		p=lpDati;
		psExt=(EH_DISPEXT *) str;

		dispfm_h(psExt->px,
						 psExt->py,
						 psExt->col1,
						 psExt->col2,
						 SET,
						 psExt->hdl,p+ofsdesc);

		//a=font_len(p+ofscod,psExt->hdl);
		a=psExt->px+psExt->lx-dila;
		line(a-3,psExt->py,a-3,psExt->py+psExt->ly-1,2,SET);

		dispfm_h(a,psExt->py,psExt->col1,psExt->col2,psExt->hdl,p+ofscod);
		break;

 case WS_HDB: // Cambio di indice

		ofscod =adb_FldOffset(Hdb,str);
		dila   =adb_FldSize(Hdb,str)*8;
		ofsdesc=adb_FldOffset(Hdb,"DESCRIZIONE");
		break;

 case WS_FINDKEY: // Ricerca del codice
	 //RecSize=1;
	 //strupr(PtrMemo);
	 if (adb_Afind(Hdb,"PERDESC",B_GET_GE,strupr(str),B_RECORD)) {break;}
	 adb_position(Hdb,&Hrec);
	 adb_get(Hdb,Hrec,IndexNum);

 case WS_FIND: // Ricerca del codice

	 if (adb_find(Hdb,0,B_GET_GE,str,B_RECORD)) {break;}
	 adb_position(Hdb,&Hrec);
	 adb_get(Hdb,Hrec,IndexNum);
	 return ON;

 case WS_IDX: // Cambio di indice
		break;

 case WS_FIRST: // Setta il dbase al primo record
	 if (adb_find(Hdb,IndexNum,B_GET_FIRST,"",B_RECORD)) break;
	 return NOACTIVE;

 case WS_FILTER: // Controllo sul record se è accettabile
	 return ON;

 case WS_KEYPRESS :
	if (key_press(ESC)) {strcpy(str,"ESC:ESC"); break;}
	break;

 }
 return OFF;
}

#endif