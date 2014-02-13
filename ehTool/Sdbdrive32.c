//   ---------------------------------------------
//    SDBdrive32  SuperDbaseDriver              
//                -Versione 99                  
//                                              
//	  Richiesta Multithread a dbase	
//
//
//                    Ferrà Art & Technology 1999 
//								Ferrà srl	2007	
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/sdbdrive.h"

extern struct ADB_INFO *ADB_info;

//#include <flm_adb.h>

// Numero massimo di scroll contemporaneamente gestiti = 5

// Numero massimo di linee per scroll = 30
//#define	 MAX_Y 30

#define  SDBMAX 10

#define SDB_DIRECT_DB 0
#define SDB_THREAD_DATA 1
#define SDB_PROGRESS_LX 80
#define SDB_PROGRESS_LY 16 

typedef struct{

	CRITICAL_SECTION csCritical;

	struct OBJ *ObjClient;
	BOOL	iModeOperation;      // 0=Dati del Thread, 1=Dati filtrati originali 0=O_SCRDB/OW_SCRDB  1=O_SCROLL/OW_SCR
	
	HDB		hdbOriginal; 		 // Handle del db originale
	HDB		hdbThread;			 // Handle del clone in ricerca

	SINT	IndexNum; // Numero del KeyPath
	SINT	(*ExtFunz)(	SINT cmd,
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
	struct WS_INFO ws;

	// Gestione MultiThread
	HANDLE hThread;
	DWORD  dwThread;

	DWORD	dwRecords;		// Record presenti nel filtro di ricerca
	BOOL   bInProcess;		// Il Thread sta girando
	BOOL   bStopRequest;	// E' richiesto uno stop del thread
//	BOOL   fInStop;

	//
	// Posizione degli oggetti annessi
	//
	DWORD	dwParam;
	HWND   hProgress;
	//HWND   hButtonStop;

} SDBEXT_MT;

static void  DB_GetLast(SDBEXT_MT *Sdb);
static BOOL  DB_seek(LONG info,SDBEXT_MT *Sdb);
static SINT  DB_perc(LONG info,SDBEXT_MT *Sdb);
static SINT  DB_find(SINT cmd,LONG info,CHAR *str,SDBEXT_MT *Sdb);
static void  DB_buf (SDBEXT_MT *Sdb);
static SINT  DB_load(SDBEXT_MT *Sdb);
static SINT  DB_zone(SDBEXT_MT *Sdb);

// Costruttore di array
static void LArrayMake(SDBEXT_MT *Sdb,SINT NumCam)
{
	CHAR Serv[40];
	LONG size=sizeof(struct WINSCR)*NumCam; // Calcola Grandezza Buffer

	if (NumCam<1) ehExit("Numcam ? [%d]",NumCam);
	sprintf(Serv,"*ListHrec:%02d",Sdb->hdbOriginal);
	if (Sdb->WinScrHdl!=-1) memoFree(Sdb->WinScrHdl,Serv);
	Sdb->WinScrHdl=-1;
	
	Sdb->WinScrHdl=memoAlloc(M_HEAP,size,Serv);
	if (Sdb->WinScrHdl<0) ehExit("Sdb:x01");
	//win_infoarg("> %d (%d) %d",NumCam,size,Sdb->WinScrHdl);

	Sdb->WinScr=memoPtr(Sdb->WinScrHdl,NULL);
	memset(Sdb->WinScr,0,(SINT) size);
}

static SDBEXT_MT SDB[SDBMAX];



static HREC LGetRecord(SDBEXT_MT *pSdb,SINT iRecord)
{
	HREC hRec;
//	EnterCriticalSection(&pSdb->csCritical);
	if (pSdb->bInProcess) 
		hRec=ADB_info[pSdb->hdbThread].AdbFilter->arRecord[iRecord]; 
		else 
			memoRead(ADB_info[pSdb->hdbOriginal].AdbFilter->hdlRecords,iRecord*sizeof(HREC),&hRec,sizeof(HREC));
//	LeaveCriticalSection(&pSdb->csCritical);
	return hRec;
}

// --------------------------------------------
// Thread di elaborazione del Filtro
// --------------------------------------------

static void ProgressPos(SDBEXT_MT *lpSdb,POINT *pPoint)
{
	POINT pProgress;
	switch (lpSdb->dwParam)
	{
		case SDB_PRG_TL:
				pProgress.x=lpSdb->ObjClient->px;
				pProgress.y=lpSdb->ObjClient->py-11;
				break;

		case SDB_PRG_TR:
				pProgress.x=lpSdb->ObjClient->px+lpSdb->ObjClient->col1-60;
				pProgress.y=lpSdb->ObjClient->py-11;
				break;

		case SDB_PRG_BL:
				pProgress.x=lpSdb->ObjClient->px;
				pProgress.y=lpSdb->ObjClient->py+lpSdb->ObjClient->col2+1;
				break;

		case SDB_PRG_BR:
				pProgress.x=lpSdb->ObjClient->px+lpSdb->ObjClient->col1-60;
				pProgress.y=lpSdb->ObjClient->py+lpSdb->ObjClient->col2+1;
				break;

		default:
		case SDB_PRG_OFF: 
				break;
	
	}
	memcpy(pPoint,&pProgress,sizeof(POINT));
}

static void LProgressBarCreate(SDBEXT_MT *lpSdb)
{
	POINT pProgress;
	ProgressPos(lpSdb,&pProgress);
	if (lpSdb->dwParam!=SDB_PRG_OFF)
	{
#ifdef _DEBUG
		if (lpSdb->hProgress) dispx("hProgress carico"); 
#endif

		lpSdb->hProgress= CreateWindowEx(0, 
										 PROGRESS_CLASS, 
										 (LPSTR) NULL, 
										 WS_CHILD | WS_VISIBLE, 
										 pProgress.x,
										 pProgress.y,
										 SDB_PROGRESS_LX,
										 SDB_PROGRESS_LY, 
										 WindowNow(), 
										 (HMENU) 0, 
										 sys.EhWinInstance, NULL); 
		/* 
		lpSdb->hButtonStop=CreateWindow("Button","X",
										WS_CHILD|
										//BS_OWNERDRAW|
										WS_VISIBLE|
										//BS_FLAT|
										BS_PUSHLIKE|
										//BS_NOTIFY|
										BS_VCENTER,						        
										pProgress.x+40,
										pProgress.y,
										16,16,
										WindowNow(),
										(HMENU) obj_IDLock(lpSdb->ObjClient->nome),
										sys.EhWinInstance,
										NULL);
*/
		SendMessage(lpSdb->hProgress, PBM_SETRANGE, 0, MAKELPARAM(0,100)); 
		SendMessage(lpSdb->hProgress, PBM_SETSTEP, (WPARAM) 1, 0); 
	}
}

static void LProgressBarDestroy(SDBEXT_MT *lpSdb)
{
	DestroyWindow(lpSdb->hProgress); lpSdb->hProgress=NULL;
	// DestroyWindow(lpSdb->hButtonStop); lpSdb->hButtonStop=NULL;
//	obj_IDUnlock(lpSdb->ObjClient->nome);
}

static DWORD WINAPI dbaseMTSearch(LPVOID Dato)
{
	static SINT iCount=0;
	SDBEXT_MT *lpSdb;
	lpSdb=(SDBEXT_MT *) Dato;
	
	LProgressBarCreate(lpSdb);

	//EnterCriticalSection(&lpSdb->csCritical);
	lpSdb->dwRecords=0;
	lpSdb->bInProcess=TRUE;
	adb_Filter(lpSdb->hdbThread,WS_DEL,0,0,0);
	adb_Filter(lpSdb->hdbThread,WS_SETFLAG,0,"MT",(CHAR *) lpSdb->hProgress); // Setto il MultiThread mode

	//
	// Richiedo elaborazione a filtro esterno
	//
	if (!lpSdb->bStopRequest) adb_Filter(lpSdb->hdbThread,WS_DOLIST,0,0,0);
	//
	// Fine regolare se Stop Request
	//
	if (!lpSdb->bStopRequest)
	{
		//
		// Libero la memoria usata dal dbase reale (se c'è)
		//
		if (ADB_info[lpSdb->hdbOriginal].AdbFilter->hdlRecords>-1)
		{
			memoFree(ADB_info[lpSdb->hdbOriginal].AdbFilter->hdlRecords,"*hdbFltRecs");
			ADB_info[lpSdb->hdbOriginal].AdbFilter->hdlRecords=-1;
		}
		
		//
		// Copio i dati della nuova lista nel dbase originale
		//
		ADB_info[lpSdb->hdbOriginal].AdbFilter->hdlRecords=ADB_info[lpSdb->hdbThread].AdbFilter->hdlRecords;
		ADB_info[lpSdb->hdbOriginal].AdbFilter->iRecordsReady=ADB_info[lpSdb->hdbThread].AdbFilter->iRecordsReady;
		ADB_info[lpSdb->hdbOriginal].AdbFilter->iRecordsMax=ADB_info[lpSdb->hdbThread].AdbFilter->iRecordsMax;
		ADB_info[lpSdb->hdbOriginal].AdbFilter->idxInUse=ADB_info[lpSdb->hdbThread].AdbFilter->idxInUse;
		
		//
		// Scrivo nel dbase virtuale che la memoria non è stata usata
		// cosi l'adb_close() non la libera
		//
		ADB_info[lpSdb->hdbThread].AdbFilter->hdlRecords=-1;

		//
		// Aggiorno il ws->maxcam con i campi alla fine letti
		//
		lpSdb->dwRecords=ADB_info[lpSdb->hdbOriginal].AdbFilter->iRecordsReady;

		// Chiudo il dbase clone
		//adb_close(lpSdb->); lpSdb->hdbThread=-1;

		//
		// Comunico che ho cambiato l'indice
		//
		if (lpSdb->IndexNum!=ADB_info[lpSdb->hdbOriginal].AdbFilter->idxInUse)
		{
			SdbDriver32(lpSdb->ObjClient,WS_IDX,ADB_info[lpSdb->hdbOriginal].AdbFilter->idxInUse,"");
		}

		//
		// Aggiusto il range della barra	
		//
		//OBJ_RangeAdjust(lpSdb->ObjClient,lpSdb->ws.offset,lpSdb->ws.maxcam,lpSdb->ws.numcam);
		//lpSdb->dwRecords=lpSdb->ws.maxcam;
		
		//
		// Se ho una finestra collegata faccio un refresh dell'oggetto
		//
		//if (lpSdb->ObjClient->hWnd) InvalidateRect(lpSdb->ObjClient->hWnd,NULL,FALSE);
		if (lpSdb->ObjClient->hWnd) InvalidateRect(lpSdb->ObjClient->hWnd,NULL,FALSE);

	}
 
	lpSdb->bInProcess=FALSE;
//	LeaveCriticalSection(&lpSdb->csCritical);

	LProgressBarDestroy(lpSdb);
	
	iCount--;
	return 0;
}
//
// FT_ThreadRefresh()
// Funzione temporale che controlla le variazioni del trhead notificandole all'oggetto
// 
static void FT_ThreadRefresh(SINT cmd,void *dao)
{
	SINT b;
    static SINT Count=0;
	BOOL fNoBlank;
	SDBEXT_MT *lpSdb;
	LONG  lOldMaxRecords;

	//return;

	if (cmd==0)
	{
	 for (b=0;b<SDBMAX;b++)
	 {
		lpSdb=&SDB[b];
		if (lpSdb->ObjClient)
			{
				struct WS_INFO *ws;
//				EnterCriticalSection(&lpSdb->csCritical);
				ws=&lpSdb->ws;

				//obj_dataRefresh(SDB[b].ObjClient->nome,-2,-2);
				// Funziona solo OW_SCR,OW_SCRDB

				//
				// Il thread sta Girando
				//
				if (lpSdb->bInProcess)	
				{
					if (ws->maxcam!=ADB_info[lpSdb->hdbThread].AdbFilter->iRecordsReady)
					{
//						dispxEx(0,40,"IN PROGRESS: %d,%d",ws->maxcam,ADB_info[lpSdb->hdbThread].AdbFilter->iRecordsReady);
						ULONG ulRequestSize=ADB_info[lpSdb->hdbThread].ulRecBuffer;
						lOldMaxRecords=ws->maxcam;
						ws->maxcam=ADB_info[lpSdb->hdbThread].AdbFilter->iRecordsReady;
						adb_BufRealloc(lpSdb->hdbOriginal,ulRequestSize,FALSE,"SDBDRIVER32",FALSE);
					
						if (lpSdb->ObjClient->hWnd) // Ho una finestra collegata
						{
							 fNoBlank=ws->fNoBlankLine;
							 if (lOldMaxRecords>ws->numcam) ws->fNoBlankLine=TRUE;
							 if (ws->maxcam<ws->numcam) InvalidateRect(lpSdb->ObjClient->hWnd,NULL,FALSE);
							// UpdateWindow(SDB[b].ObjClient->hWnd);
							 ws->fNoBlankLine=fNoBlank;
							 if (lpSdb->IndexNum!=ADB_info[lpSdb->hdbThread].AdbFilter->idxInUse)
							 {
								 SdbDriver32(lpSdb->ObjClient,WS_IDX,ADB_info[lpSdb->hdbThread].AdbFilter->idxInUse,"");
							 }
							 OBJ_RangeAdjust(lpSdb->ObjClient,ws->offset,ws->maxcam,ws->numcam);

						}
					}
				}
				else
				{
					if (lpSdb->AdbFilter)
					{
						if (lpSdb->dwRecords!=lpSdb->ws.maxcam)
						{
							lpSdb->ws.maxcam=lpSdb->dwRecords;
							if (ws->maxcam<ws->numcam) InvalidateRect(lpSdb->ObjClient->hWnd,NULL,FALSE);
							OBJ_RangeAdjust(lpSdb->ObjClient,lpSdb->ws.offset,lpSdb->ws.maxcam,lpSdb->ws.numcam);
//							dispxEx(0,40,"STOP PROGRESS: %d,%d",ws->maxcam,lpSdb->dwRecords);
						}
					}
				}
	
				// Richiesta di Stop/Thread 
				//if (SDB[b].fInStop) {PostStopThread(ws,&SDB[b]);}
	//		LeaveCriticalSection(&SDB[b].csCritical);
			}
	 }
	 Count++; Count%=8;
	}
}

static BOOL fInitialize=TRUE;
static SDBEXT_MT *ObjToSdb(struct OBJ *objCalled)
{
	SINT b;
	SINT ptClient;

	if (fInitialize)
	{
	 for (b=0;b<SDBMAX;b++)
		{
			memset(SDB+b,0,sizeof(SDBEXT_MT));
			SDB[b].WinScrHdl=-1;
		}
	 // Attivo FTIME per il refresh degli oggetti
	 FTIME_on(FT_ThreadRefresh,1);
	 fInitialize=FALSE;
	}

	// ---------------------------------------
	// Trova il Client Object                |
	// ---------------------------------------
	ptClient=-1;
	for (b=0;b<SDBMAX;b++) {if (objCalled==SDB[b].ObjClient) {ptClient=b; break;}}

	// Se non c'è provo ad assegnarlo
	if (ptClient==-1)
	 {
		for (b=0;b<SDBMAX;b++)
		 {
			if (SDB[b].ObjClient==NULL)
				{SDB[b].ObjClient=objCalled;
				 ptClient=b;
				 SDB[b].ExtFilter=ADBNULL;
				 break;
				}
		 }
	 }

	// Se non ci riesco ... errore
	if (ptClient==-1) ehExit("SDB:Too client!");

	// Puntatore alla struttura SDBEXT
	return &SDB[ptClient];
}

//  +-----------------------------------------+
//	| SdbDriver32                             |
//	|                                         |
//	|          by Ferrà Art & Technology 1999 |
//	+-----------------------------------------+

void *SdbDriver32(struct OBJ *objCalled,SINT cmd,LONG info,CHAR *str)
{
	static struct WINSCR rit,*PtScr;
	LONG Hrec;
	struct WS_DISPEXT *DExt;
    SDBEXT_MT *lpSdb;
	CHAR  Serv[80];

	if ((objCalled->tipo!=OW_SCR)&&(objCalled->tipo!=OW_SCRDB)) return 0;
	
	// new 2007
	if (!objCalled->pOther) objCalled->pOther=ObjToSdb(objCalled);
	lpSdb=objCalled->pOther;

	switch (cmd) {

	case WS_INF: return &lpSdb->ws;

	// -----------------------------------
	// CREAZIONE 
	// -----------------------------------
	case WS_CREATE:
			ZeroFill(lpSdb->csCritical);
			InitializeCriticalSection(&lpSdb->csCritical);
			lpSdb->hProgress=NULL;
			lpSdb->bInProcess=FALSE;
			lpSdb->hdbThread=0;
			break;

	// -----------------------------------
	// DISTRUZIONE
	// -----------------------------------
	case WS_DESTROY: // Chiusura del Gestore

			 (lpSdb->ExtFunz)(WS_DESTROY,0,0,0,lpSdb->hdbOriginal,lpSdb->IndexNum);
			 DeleteCriticalSection(&lpSdb->csCritical);
			 if (lpSdb->WinScrHdl!=-1) ehExit("SDB:xx");
			 memset(lpSdb,0,sizeof(SDBEXT_MT));
			 lpSdb->WinScrHdl=-1;
			 lpSdb->ObjClient=NULL;
			 if (lpSdb->hdbThread>0) {adb_close(lpSdb->hdbThread); lpSdb->hdbThread=0;}
			 break;

	// -----------------------------------
	// SPOSTAMENTO E RIDIMENSIONAMENTO
	// -----------------------------------
	
	case WS_MOVE:
	case WS_SIZE:
			if (lpSdb->hProgress)
			{
				POINT pProgress;
				ProgressPos(lpSdb,&pProgress);
				MoveWindow(lpSdb->hProgress,pProgress.x,pProgress.y,SDB_PROGRESS_LX,SDB_PROGRESS_LY,FALSE);
			}
			break;

	case WS_OPEN:
			if (lpSdb->bInProcess) {win_infoarg("OPEN in process"); ehExit("");}
			if (info<3) {ehExit("Field ? SdbDriver32");}
			lpSdb->ws.numcam=info;// Assegna il numero di campi da visualizzare

			if (!lpSdb->ExtFunz) ehExit("SDB:NULL1");
			(lpSdb->ExtFunz)(WS_OPEN,0,0,0,lpSdb->hdbOriginal,lpSdb->IndexNum);

	// -------------------------------------------------------
	// RICHIESTA DI INIZIO DI RICERCA DATI
	// - Apro il nuovo dbase su cui farò la ricerca MT
	// - Lancio il thread di ricerca
	//
	// -------------------------------------------------------

	case WS_LOAD :  // Ex Apri

//			dispxEx(0,20,"OPEN:ENTRO");
//			EnterCriticalSection(&lpSdb->csCritical);
			if (lpSdb->bInProcess) {win_infoarg("LOAD in process"); ehExit("");}

			//
			// Congelo la gestione dell'oggetto
			//
			lpSdb->ObjClient->bFreeze=TRUE;
			lpSdb->iModeOperation=SDB_DIRECT_DB; // Modalità 1 gestore O_SCROLL

			 if ((lpSdb->ObjClient->tipo==OW_SCR)||
				 (lpSdb->ObjClient->tipo==OW_SCRDB)) 
				   lpSdb->ObjClient->tipo=OW_SCRDB;  
				   else 
				   lpSdb->ObjClient->tipo=O_SCRDB;
           
			 //
			 // Se esite collegato un AdbFilter
			 //
			 if (lpSdb->AdbFilter)
			 {
				 // -----------------------------------------
				 // Controllo se ho un adb_filter connesso  |
				 // -----------------------------------------
				 if (ADB_info[lpSdb->hdbOriginal].AdbFilter!=NULL)
					 {
						//
						// Un filtro è attivo ?
						 //
						 if (ADB_info[lpSdb->hdbOriginal].AdbFilter->NumComp)
							{ 
								//
								// 1) Se non l'ho già aperto, apro un nuovo dbase clone per la ricerca
								//	  Una volta una tantum fino alla fine
								//
								if (!lpSdb->hdbThread) lpSdb->hdbThread=adb_OpenClone(lpSdb->hdbOriginal);

								//efx2(); dispx("CLONO IL DBASE");ehSleep(1000); efx2();

								// 2) Configuro l'oggetto in base alla richiesta
								lpSdb->ws.offset=ADBNULL;
								lpSdb->ws.selez=-1; // Nessun selezionato
								lpSdb->ws.maxcam=0;
								lpSdb->ExtFilter=OFF;
								lpSdb->iModeOperation=SDB_THREAD_DATA;// Modalità 1 gestore O_SCROLL

								if (lpSdb->ObjClient->tipo==OW_SCRDB) 
									lpSdb->ObjClient->tipo=OW_SCR; 
									else 
									lpSdb->ObjClient->tipo=O_SCROLL;
								
								//
								// 4) Creo il Thread per l'elaborazione
								//
								lpSdb->bInProcess=TRUE;
								lpSdb->bStopRequest=FALSE;
								lpSdb->hThread = CreateThread(NULL, 
															  8000, 
															  dbaseMTSearch, 
															  (LPDWORD) lpSdb,
															  0, 
															  &lpSdb->dwThread);
								//ADB_info[lpSdb->hdbOriginal].AdbFilter->IndexInUse=-1;
								//ADB_info[lpSdb->hdbThread].AdbFilter->IndexInUse=-1; ????
								// Setto la priorità del Thread
								//SetThreadPriority(lpSdb->hThread,THREAD_PRIORITY_NORMAL);//THREAD_PRIORITY_LOWEST);
								SetThreadPriority(lpSdb->hThread,THREAD_PRIORITY_LOWEST);
							}
					 }
			 }
			 if (DB_load(lpSdb)) ehExit("SdbDriver32:No extern function");
			 lpSdb->ObjClient->bFreeze=FALSE;
			 //LeaveCriticalSection(&lpSdb->csCritical);
//			 dispxEx(0,20,"OPEN:PRONTO");
			 break;

	// -----------------------------------
	// WS_CLOSE IL DRIVER                  |
	// -----------------------------------
	case WS_CLOSE:

			SdbDriver32(objCalled,WS_PROCESS,STOP,NULL);
			EnterCriticalSection(&lpSdb->csCritical);
			
			if (lpSdb->WinScrHdl<0)
			{
				LeaveCriticalSection(&lpSdb->csCritical);
				break;
			}
			lpSdb->ObjClient->bFreeze=TRUE;
			(lpSdb->ExtFunz)(WS_CLOSE,0,0,0,lpSdb->hdbOriginal,lpSdb->IndexNum); // Notifico la chiusura
			sprintf(Serv,"*ListHrec:%02d",lpSdb->hdbOriginal);
			if (lpSdb->WinScrHdl!=-1) memoFree(lpSdb->WinScrHdl,Serv);
			lpSdb->WinScrHdl=-1;
			lpSdb->WinScr=NULL;
			lpSdb->RecBuffer=NULL;
			if (lpSdb->hdbThread>0) {adb_close(lpSdb->hdbThread); lpSdb->hdbThread=0;}
			LeaveCriticalSection(&lpSdb->csCritical);
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
			 if (lpSdb->iModeOperation!=SDB_DIRECT_DB) ehExit("DBSEEK ?");
			 if (DB_seek(info,lpSdb)) DB_buf(lpSdb);
			 break;

	// -----------------------------------
	// RICERCA DI UN OFFSET ASSOLUTO     |
	// -----------------------------------
	case WS_DBPERC : //			  				 Ricerca di un offset assoluto
			 if (lpSdb->iModeOperation!=SDB_DIRECT_DB) ehExit("DBPERC ?");
			 if (DB_perc(info,lpSdb)) DB_buf(lpSdb);
			 break;

	// -----------------------------------
	// PRESSIONE DI UN TASTO/MOUSE       |
	// -----------------------------------
	case WS_KEYPRESS :
				if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
				// Transfer del comando a sub-driver
//				(lpSdb->ExtFunz)(cmd,NULL,NULL,str,info,lpSdb->hdbOriginal,lpSdb->IndexNum);
				(lpSdb->ExtFunz)(cmd,NULL,
								 info,str,
								 lpSdb->hdbOriginal,
								 lpSdb->IndexNum);
				break;

	case WS_FINDKEY:
	case WS_REALSET : //			  						Ricerca la Chiave selezionata
	case WS_FIND : //			  						Ricerca la Chiave selezionata

	// --------------------------------------------------
	// CERCA L'ULTIMO (POSIZIONA LO SCROLL ALLA FINE    |
	// --------------------------------------------------
	case WS_FINDLAST:

			 DB_find(cmd,info,str,lpSdb);
			 break;
	// -----------------------------------
	// SETTA SELEZIONE RECORD            |
	// -----------------------------------
	case WS_SEL : //			  			Settaggio selez
			{ 
			 HREC Record;
			 if (lpSdb->ExtFunz==NULL) ehExit("SdbDriver32:No ext/disp function");
			 if (lpSdb->iModeOperation==SDB_DIRECT_DB)  // DB Mode
			 {
				 lpSdb->ws.selez=info;
			  Record=info;
			 }
			 else
			 { if (lpSdb->ws.selez==-1) break;
				Record=LGetRecord(lpSdb,lpSdb->ws.selez);
			 }
//			 (lpSdb->ExtFunz)(cmd,NULL,NULL,str,info,lpSdb->hdbOriginal,lpSdb->IndexNum);
			 (lpSdb->ExtFunz)(cmd,NULL,Record,str,lpSdb->hdbOriginal,lpSdb->IndexNum);
			}
			 break;


	// -------------------------------------
	// SETTA L'OFFSET  (Solo Modo O_SCROLL |
	// -------------------------------------
	case WS_OFF : //			  			Settaggio offset
			if (lpSdb->iModeOperation==SDB_DIRECT_DB) ehExit("SdbDriver32:Usato da oggetto O_SCROLL");
			break;

	// -------------------------------------
	// RITORNA il Puntatore al Record      |
	// -------------------------------------
	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata

			//SdbDriver32(WS_PROCESS,STOP,NULL); era attivo
	 
			rit.keypt=lpSdb->RecBuffer;
			 // Modo 0: Normale direct
			if (lpSdb->iModeOperation==SDB_DIRECT_DB)
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
				  HREC hRec;
				  //if (lpSdb->ws.selez==-1) ehExit("E no ?");
	
				  hRec=LGetRecord(lpSdb,lpSdb->ws.selez);
				  // Il Thread in azione
				  /*
				  if (lpSdb->fInProcess) 
					{Hrec=ADB_info[lpSdb->hdbThread].AdbFilter->arRecord[lpSdb->ws.selez];}
					else 
				  // Il Thread è fermo
					{memoRead(ADB_info[lpSdb->hdbOriginal].AdbFilter->hdlRecords,
						           lpSdb->ws.selez*sizeof(HREC),
								   &Hrec,sizeof(HREC));
					}
					*/

				  rit.record=hRec;
				}
			 
			 if (rit.record!=ADBNULL) 
				 adb_get(lpSdb->hdbOriginal,rit.record,lpSdb->IndexNum);
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
	case WS_REFOFF : //			  											 Schermo rifreshato
			 lpSdb->ws.refre=OFF;
			 break;

	//
	// Controllo del Thread
	//
	case WS_PROCESS:
	
			// Controllo se l'elaborazione è in corso
			if ((info==0)&&(*str=='?'))
			{
				BOOL bValore;
				bValore=lpSdb->bInProcess;
				if (bValore) return "!"; else return NULL;
			}

			if ((info==STOP)&&!str)
			 {
				BOOL bProcess;
				EnterCriticalSection(&lpSdb->csCritical);
				bProcess=lpSdb->bInProcess;
				LeaveCriticalSection(&lpSdb->csCritical);

				if (bProcess)
				{
					//
					// Chiedo di fermare il processo sia al thread che al fitro esterno
					//
					
					EnterCriticalSection(&lpSdb->csCritical);
					lpSdb->bStopRequest=TRUE;
					ADB_info[lpSdb->hdbThread].AdbFilter->fBreakExtern=TRUE;
					LeaveCriticalSection(&lpSdb->csCritical);
					
					//
					// Attendo che il thread sia fermo
					//
					while(TRUE) 
					{
					  //BOOL fCheck;
					  WindowsMessageDispatch();
					  
					  //EnterCriticalSection(&lpSdb->csCritical);
					  ADB_info[lpSdb->hdbThread].AdbFilter->fBreakExtern=TRUE;
					  lpSdb->bStopRequest=TRUE;

					  if (!lpSdb->bInProcess) break;
					  //LeaveCriticalSection(&lpSdb->csCritical);
					  
					  //if (fCheck) break;
					  //Sleep(200);
					}
				} 

				break;
			 }
			break;
			 
	// -------------------------------------
	// Richiesta di Stampa dei Dati        
	// -------------------------------------
	case WS_DISPLAY : //			  			Richiesta buffer
			if (lpSdb->ExtFunz==NULL) ehExit("SdbDriver32:No ext/disp function");
			 DExt=(struct WS_DISPEXT *) str;

			 //
			 // Il Titolo
			 //
			 if (DExt->ncam==-10) 
			 {
				(lpSdb->ExtFunz)(0,NULL,0,str,lpSdb->hdbOriginal,lpSdb->IndexNum);
				break;
			 }

			 //printf("WDSP: %d|",DExt->ncam);

			 // -----------------------------------------------
			 // Esiste una lista di filtro collegata
			 // -----------------------------------------------
			 if (lpSdb->iModeOperation) 
				{
					//if ((DExt->ncam+lpSdb->ws.offset)<0) ehExit("poik");
					LONG pt=DExt->ncam+lpSdb->ws.offset;
					
					//win_infoarg("Chiedo (Handle:%d) %d di (%d)",memo_hdl,pt,lpSdb->ws.maxcam); //ehSleep(200);
					Hrec=ADBNULL;
					
//					EnterCriticalSection(&lpSdb->csCritical);
					//
					// Il thread in azione
					// NOTA: L'importante che lpSdb->hdbThread non mi venga tolto mentre leggo il record
					if (lpSdb->bInProcess)
					{
						if (!ADB_info[lpSdb->hdbThread].AdbFilter) break;
						if (!ADB_info[lpSdb->hdbThread].AdbFilter->arRecord) break;
						if (pt<ADB_info[lpSdb->hdbThread].AdbFilter->iRecordsReady) Hrec=ADB_info[lpSdb->hdbThread].AdbFilter->arRecord[pt]; 
					}
					else 
					{
					// Il Thread è fermo
						if (ADB_info[lpSdb->hdbOriginal].AdbFilter->hdlRecords<0) break;
						memoRead(ADB_info[lpSdb->hdbOriginal].AdbFilter->hdlRecords,pt*sizeof(HREC),&Hrec,sizeof(HREC));
					}
//					LeaveCriticalSection(&lpSdb->csCritical); 
				}
				else // Modo Diretto in Db
				{   
					Hrec=lpSdb->WinScr[DExt->ncam].record;
				}

				if (Hrec) // Se esiste il record
				{
//				 dispx("LEGGO DB:%d ..",lpSdb->hdbOriginal);
					SINT iErr;
					adb_ErrorView(FALSE);
					iErr=adb_get(lpSdb->hdbOriginal,Hrec,-1);//-1);
					if (!iErr)
					 {
						 (lpSdb->ExtFunz)(0,
										  adb_DynRecBuf(lpSdb->hdbOriginal),//lpSdb->RecBuffer,
										  0,
										  str,
										  lpSdb->hdbOriginal,
										  lpSdb->IndexNum);
					 } else 
					 {
						dispx("Qui");
					 }
	//				 dispx("LETTO.");
					 adb_ErrorView(TRUE);
				}
			 //sonic(1000,1,2,3,4,5); getch();
			 break;
/*
	// -------------------------------------
	// COMUNICA LA FUNZIONE ESTERNA        |
	// -------------------------------------
	case WS_EXTFUNZ: //	 era WS_EXTFUNZ
			 lpSdb->ExtFunz=(SINT (*)(SINT cmd,CHAR *PtDati,LONG  dato,
							          void  *str,SINT Hdb,SINT IndexNum)) str;
			 lpSdb->iModeOperation=-1; // Modalità disattivata
			 break;

	// -------------------------------------
	// COMUNICA Handle del Dbase da Usare  |
	// -------------------------------------
	case WS_HDB : //	 era WS_HDB
			 lpSdb->Hdb=(SINT) info;
			 // Succede se SdbHDB Š prima della dichiarazione della funzione
			 if (lpSdb->ExtFunz==NULL)
					ehExit("No Funzdisp");
					else
					(lpSdb->ExtFunz)(WS_HDB,NULL,0,str,lpSdb->hdbOriginal,lpSdb->IndexNum);
			 break;
*/

	// ---------------------------------------
	// COMUNICA L'indice attualmente in Uso  |
	// ---------------------------------------

	case WS_IDX : //	era WS_IDX
			 lpSdb->IndexNum=(SINT) info;
			 // Succede se SdbINX Š prima della dichiarazione della funzione
			 if (lpSdb->ExtFunz==NULL)
					ehExit("No Funzdisp");
					else
					(lpSdb->ExtFunz)(WS_IDX,NULL,0,str,lpSdb->hdbOriginal,lpSdb->IndexNum);

			 break;

	// -------------------------------------
	// SETTA IL MODO ADBFILTER             |
	// SE info==ON Prova a fare la Lista   |
	// -------------------------------------

	case WS_ADBFILTER : //			  			Richiesta buffer
	
			EnterCriticalSection(&lpSdb->csCritical);
			//
			// Se ch'è un thread che usa il filter non si può fare
			//
			if (lpSdb->bInProcess) win_infoarg("Cambio del filtro in process");
			if (lpSdb->ExtFunz==NULL) ehExit("SdbDriver32 01?");
			// Info  = ON/OFF
			lpSdb->AdbFilter=(BOOL) info;
			(lpSdb->ExtFunz)(cmd,0,info,str,lpSdb->hdbOriginal,lpSdb->IndexNum);
			LeaveCriticalSection(&lpSdb->csCritical);
			break;

	// -------------------------------------
	// SETTA IL FILTRO ESTERNO             |
	// -------------------------------------
	case WS_SETFILTER : //			  			Richiesta buffer

			 // Info  = ON/OFF
			 lpSdb->ExtFilter=(BOOL) info;
			 (lpSdb->ExtFunz)(cmd,0,info,str,lpSdb->hdbOriginal,lpSdb->IndexNum);
			 break;

#ifdef _WIN32
	
	case WS_LINEVIEW: //	 
		// Creo nuova zona di memoria addatta a contenere la nuova dimensione di visione
		//dispx("Prima=%d Adesso=%d",lpSdb->ws.numcam,info); ehSleep(600);
        {
		 HREC LastTop=ADBNULL;
		 if (lpSdb->WinScrHdl!=-1) LastTop=lpSdb->WinScr[0].record;
		 LArrayMake(lpSdb,info);
		 lpSdb->ws.numcam=info;
		 lpSdb->Dove=0;
		 lpSdb->End=(SINT) lpSdb->ws.numcam;
		 if ((lpSdb->iModeOperation==SDB_DIRECT_DB)&&(LastTop!=ADBNULL)) adb_get(lpSdb->hdbOriginal,LastTop,lpSdb->IndexNum);
		 DB_buf(lpSdb);
		}
		break;

	case WS_LINEEDIT: //	 
		lpSdb->ws.Enumcam=info;
		break;
#endif

	default:
//	case WS_SETFLAG : //			  			Richiesta buffer
			 if (lpSdb->ExtFunz==NULL) ehExit("SdbDriver32 02? [%d] [%s]",cmd,objCalled->nome);

			 (lpSdb->ExtFunz)(cmd,0,info,str,lpSdb->hdbOriginal,lpSdb->IndexNum);
			 break;

	 }


	PtScr=lpSdb->WinScr; if (!PtScr) PtScr=&rit;
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

static void DB_GetLast(SDBEXT_MT *Sdb)
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
	if (adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_LAST,"",B_RECORD)) return;
	if (Sdb->ExtFilter)
		 {
			test=(Sdb->ExtFunz)(WS_FILTER,NULL,NULL,NULL,NULL,Sdb->hdbOriginal,Sdb->IndexNum);
			if ((test==OFF)||(test==O_KEYSTS_DOWN_MOUSEOUT)) a=0;
		 }
		*/

	if (Sdb->RecDN)  {a=adb_get(Sdb->hdbOriginal,Sdb->RecDN,Sdb->IndexNum);
					  if (a) Sdb->RecDN=ADBNULL;
						}
	if (!Sdb->RecDN) {a=adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_LAST,"",B_RECORD);}
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
				 adb_position(Sdb->hdbOriginal,&Hrec);
				 //sonic(200,1,2,3,4,5);
				 if (Hrec==Sdb->RecUP) return;
				}

//			 if (adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD)) break;

			 // ------------------------------------------------------
			 //  Se c'Š il filtro controllo la validità del record   |
			 // ------------------------------------------------------
			 if (Sdb->ExtFilter)
				 {
					test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
					//if (test==OFF) break; // Fine della ricerca
					//if ((test==OFF)||(test==O_KEYSTS_DOWN_MOUSEOUT)) goto rifo4; // Campo non valido ma continua
					if (test==ON) a++;
				 }
					else
				 {a++;test=ON;}

			 // Se il record ultimo non c'Š me lo trovo
			 if (!Sdb->RecDN&&(test==ON)) {adb_position(Sdb->hdbOriginal,&Sdb->RecDN);}
			 if (adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD)) break;
			}
}


// --------------------------------------------------
//  DB_FIND Ricerca un determinato record           |
//                                                  |
//  Ritorna : 0 - Nessun spostamento                |
//            1 - Mandare in stampa                 |
//                                                  |
// --------------------------------------------------
static SINT DB_find(SINT cmd,LONG info,CHAR *str,SDBEXT_MT *Sdb)
{
	float percent,perc2;
	SINT test;
	HREC hRec;
	struct WS_INFO *ws=&Sdb->ws;

	if (cmd==WS_FINDLAST)
		{
		if (Sdb->iModeOperation)
		 {
			if (!ADB_info[Sdb->hdbOriginal].AdbFilter->iRecordsReady) return 0;

			// Il thread in azione
			hRec=LGetRecord(Sdb,ADB_info[Sdb->hdbThread].AdbFilter->iRecordsReady-1);
			adb_get(Sdb->hdbOriginal,hRec,Sdb->IndexNum);
		 }
		 else
		 {   
			 if (Sdb->ExtFilter!=ADBNULL) {Sdb->RecDN=ADBNULL;DB_GetLast(Sdb);}
			                              else
										  {if (adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_LAST,"",B_RECORD)) return 0;}
		 }
		 goto after;
		}

	// Primo controllo
	if (((*str==0)||(ws->maxcam==0))&&(cmd==WS_FIND)) return 0;

	// ----------------------------------------------------------------
	// Chiedo alla funzione esterna quello che mi Š stato chiesto     |
	// ----------------------------------------------------------------
	test=(Sdb->ExtFunz)(cmd,0,info,str,Sdb->hdbOriginal,Sdb->IndexNum);
	if (test==OFF) return 0; // Nessuna ricerca effettuata

	adb_position(Sdb->hdbOriginal,&hRec);
	adb_get(Sdb->hdbOriginal,hRec,Sdb->IndexNum);

	after:

	// ----------------------------------------------------------------
	// Se sono con una lista vado a cercare HREC                      |
	// ----------------------------------------------------------------
	if (Sdb->iModeOperation==SDB_THREAD_DATA)
	 {
		 BOOL Check=FALSE;
		 LONG l;
		 HREC Hrec,Hrec2;

         if (cmd!=WS_FINDLAST)
		 {
		  adb_position(Sdb->hdbOriginal,&Hrec);
		  if (Sdb->bInProcess) // Thread in azione
		  {
			for (l=0;l<ADB_info[Sdb->hdbThread].AdbFilter->iRecordsReady;l++)
			{
				//memoRead(ADB_info[Sdb->hdbOriginal].AdbFilter->HdlRecord,l*sizeof(HREC),&Hrec2,sizeof(HREC));
				Hrec2=ADB_info[Sdb->hdbThread].AdbFilter->arRecord[l]; 
			    if (Hrec==Hrec2) {Sdb->ws.selez=l;Check=TRUE;break;}
			}
		  }
		  else // Thread Fermo
		  {
			for (l=0;l<ADB_info[Sdb->hdbOriginal].AdbFilter->iRecordsReady;l++)
			{
				memoRead(ADB_info[Sdb->hdbOriginal].AdbFilter->hdlRecords,l*sizeof(HREC),&Hrec2,sizeof(HREC));
				if (Hrec==Hrec2) {Sdb->ws.selez=l;Check=TRUE;break;}
			}
		  }
		  if (!Check) {Sdb->ws.selez=-1;return 0;}
		 } 
		 else 
		 {
//			 EnterCriticalSection(&Sdb->csCritical);
			 if (Sdb->bInProcess) Sdb->ws.selez=ADB_info[Sdb->hdbThread].AdbFilter->iRecordsReady-1;
								  else
								  Sdb->ws.selez=ADB_info[Sdb->hdbOriginal].AdbFilter->iRecordsReady-1;
//			 LeaveCriticalSection(&Sdb->csCritical);
		 }

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
		 if (!adb_recnum(Sdb->hdbOriginal)) return -1;
	 }
//	printf("OK2"); getch();


	// Controlla che sia buono
	//if (test)

	if (Sdb->ExtFilter)
	{
	 test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
	 //printf("test %d",test);
	 if (test!=ON) // Se non lo Š setta la lista dall'inizio
					 {return 0;}
//						else
	}

	// Metto in ws->selez la selezione

	adb_position(Sdb->hdbOriginal,&hRec); ws->selez=hRec;
	SdbDriver32(Sdb->ObjClient,WS_SEL,hRec,"");//Sdb->hdbOriginal,Sdb->IndexNum);

	// Cerco la posizione in percentuale
	adb_findperc(Sdb->hdbOriginal,Sdb->IndexNum,&percent);// Legge la posizione in percentuale
    //win_infoarg("%f %x [%s] ",percent,Hrec,adb_FldPtr(Sdb->hdbOriginal,"CODICE"));
	perc2=percent*(ws->maxcam-1)/100;
	ws->offset=(LONG) perc2;
	ws->refre=ON;
//	Sdb->buf[0].record=NULL;
    if (Sdb->WinScrHdl==-1) ehExit("Hdl ?");
	Sdb->WinScr[0].record=ADBNULL;
	ws->dbult=-1; // <-- aggiunta
	ws->refre=ON;

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
//  DB_ZONE Stabilisce il primo ed ultimo record    |
//          del dbase                               |
//                                                  |
//                                                  |
//  ritorno NOACTIVE zona globale senza filtro      |
//          OFF      Nessun record                  |
//          ON       Limiti caricati                |
//                                                  |
// --------------------------------------------------
static SINT DB_zone(SDBEXT_MT *Sdb)
{
	SINT a;

	//printf("DBZONE:\n");
	//getch();
	//mouse_input();
	if (Sdb->iModeOperation) ehExit("JFK");

	// -----------------------------------
	// A) Richiesta del Primo Record     |
	// -----------------------------------

	Sdb->RecUP=ADBNULL; Sdb->RecDN=ADBNULL;
	Sdb->FiltUP=0; Sdb->FiltDN=0;
	if (!Sdb->ExtFilter)
		 a=NOACTIVE;
		 else
		 a=(Sdb->ExtFunz)(WS_FIRST,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
	

	// ----------------------------------------------
	// B) Nessun filtro Š attivo                    |
	//    Carico la struttura nel modo ottimale     |
	// ----------------------------------------------

	if (a==NOACTIVE) // Per eredità
		 {Sdb->ExtFilter=FALSE;
//			ws->maxcam=adb_recnum(Sdb->hdbOriginal);
			// Posizioni delle percentuali
			Sdb->FiltUP=0; Sdb->FiltDN=100;
			if (!adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_FIRST,"",B_RECORD))
					adb_position(Sdb->hdbOriginal,&Sdb->RecUP);

			if (!adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_LAST,"",B_RECORD))
					adb_position(Sdb->hdbOriginal,&Sdb->RecDN);
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
	 a=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
	 if (a==OFF) return OFF; // Non ci sono record validi
	 if (a==ON) break;     // Trovato
	} while	(!adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD));

	// Registro la prima posizione
	adb_findperc(Sdb->hdbOriginal,Sdb->IndexNum,&Sdb->FiltUP);
	adb_position(Sdb->hdbOriginal,&Sdb->RecUP);

	// ------------------------------------------------
	// E) Ricerco l'ultima posizione                  |
	// ------------------------------------------------
	a=(Sdb->ExtFunz)(WS_LAST,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);

	// Se risponde
	if (a)
		 {
			do {  // Loopizzo
			 a=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
			 // CONDIZIONE DA CONTROLLARE
			 if (a==OFF) return OFF; // Non ci sono record validi
			 if (a==ON) break;     // Trovato
			 } while (!adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD));

			// Registro l'ultima posizione
			adb_findperc(Sdb->hdbOriginal,Sdb->IndexNum,&Sdb->FiltDN);
			adb_position(Sdb->hdbOriginal,&Sdb->RecDN);
		 }

 return ON;
}

// --------------------------------------------------
//  DB_BUF  Carica il buffer con i record letti     |
//                                                  |
//  non ritorna nulla                               |
//                                                  |
// --------------------------------------------------
static void DB_buf(SDBEXT_MT *Sdb)
{
	SINT a,b,test;
	LONG Hrec;
	struct WS_INFO *ws=&Sdb->ws;

	if (Sdb->iModeOperation) // Se sono in modalità puntatori
	{
	 return;
	}

	//printf("DBBUF:");
	//getch();
	//mouse_input();

	if (ws->dbult==ADBNULL) // prima volta
		 {
			if (ws->maxcam==0) {a=0;goto canc;}
			//(Sdb->ExtFunz)(WS_FIRST,NULL,NULL,NULL,NULL,Sdb->hdbOriginal,Sdb->IndexNum);
			if (!Sdb->RecUP) DB_zone(Sdb);
			ws->offset=0;
			if (!Sdb->RecUP) return; // Errore
			// Leggo il primo record
			adb_get(Sdb->hdbOriginal,Sdb->RecUP,Sdb->IndexNum);
		}
		else
			if (ws->dbult==Sdb->WinScr[0].record) return; // il buffer Š ok
//			if (ws->dbult==Sdb->buf[0].record) return; // il buffer Š ok
        
		// -------------------------------------
		//  Carica i nuovi valori nella cache  !
		// -------------------------------------
		for (a=0;a<Sdb->End;a++)
		{
//			 memcpy(Sdb->buf[Sdb->Dove].keypt,Sdb->RecBuffer,(SINT) Sdb->RecSize);
//			 memmove(Sdb->buf[Sdb->Dove].keypt,Sdb->RecBuffer,(SINT) Sdb->RecSize);
			 // #?#
//			 adb_position(Sdb->hdbOriginal,&Sdb->buf[Sdb->Dove].record);

			 adb_position(Sdb->hdbOriginal,&Sdb->WinScr[Sdb->Dove].record);
			 Sdb->Dove++;

			 rifai:
			 // Fine corsa misurata in apertura
			 if (Sdb->RecDN)
					{adb_position(Sdb->hdbOriginal,&Hrec);
					 if (Hrec==Sdb->RecDN) break;
					}
			 if (adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD)) break;
			 if (Sdb->ExtFilter)
				{
				 test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);

				 if (test==OFF) break; // Fine della ricerca
				 if (test==-2||test==2) goto rifai; // Campo non valido ma continua
				}
		}

		// Devo trovarmi l'ultima posizione valida
		if (!Sdb->RecDN)
		{//win_info("Controlla");
				adb_findperc(Sdb->hdbOriginal,Sdb->IndexNum,&Sdb->FiltDN);
				adb_position(Sdb->hdbOriginal,&Sdb->RecDN);
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
static SINT DB_perc(LONG info,SDBEXT_MT *Sdb)
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

	// 	Il cursore Š settato in testa
	if (percent<.1F) {ws->offset=0;goto goperc1;}

	adb_getperc(Sdb->hdbOriginal,Sdb->IndexNum,percent);

	// -----------------------------------------------------------
	// Se il filtro Š attivo controlla la validità del record    |
	// -----------------------------------------------------------
	if (Sdb->ExtFilter)
		{do
		 {
		 test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
		 if (test==ON) goto goperc1;

		 if ((Sdb->RecUP)||(test==OFF))
				{
				 adb_position(Sdb->hdbOriginal,&hrec);
				 //sonic(200,1,2,3,4,5);
				 if (hrec==Sdb->RecUP) break;
				}

		 // Sei alla fine
		 if (Sdb->RecDN)
				{
				 adb_position(Sdb->hdbOriginal,&hrec);
				 //sonic(200,1,2,3,4,5);
				 if (hrec==Sdb->RecDN)
					 {
						DB_GetLast(Sdb);
						return -1;
					 }
				}

		 } while (!adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD));
		 ws->offset=0;
		}

 goperc1:

 Sdb->Dove=0;
 Sdb->End=(SINT) ws->numcam;
 //Sdb->buf[0].record=NULL;
 Sdb->WinScr[0].record=ADBNULL;

 ws->refre=ON;

 // -----------------------------------------------------------
 //  Se l'offset Š all'inizio stampa e basta                  |
 // -----------------------------------------------------------

 if (ws->offset<=0)
		{ws->offset=0;
		 //(Sdb->ExtFunz)(WS_FIRST,NULL,NULL,NULL,NULL,Sdb->hdbOriginal,Sdb->IndexNum);

		 if (Sdb->RecUP) {a=adb_get(Sdb->hdbOriginal,Sdb->RecUP,Sdb->IndexNum);
											if (a) Sdb->RecUP=0;
											}
		 return -1;
		}

 // -----------------------------------------------------------
 //  Controlla quanti ce ne sono                              |
 // -----------------------------------------------------------
 adb_position(Sdb->hdbOriginal,&OriRec);
 for (a=1;a<(SINT) ws->numcam;a++)
 {
	erifa:

	// Sei alla fine
	if (Sdb->RecDN)
			{adb_position(Sdb->hdbOriginal,&hrec);
			 //sonic(200,1,2,3,4,5);
			 if (hrec==Sdb->RecDN)
					 {FineCorsa=ON;
						break;
					 }
				}

	if (adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD))
		 {FineCorsa=ON;break;}

	if (Sdb->ExtFilter)
		{test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
		 if (test!=ON) goto erifa;
		}
 }

 if (FineCorsa)
		 {if (!Sdb->RecDN) {adb_position(Sdb->hdbOriginal,&Sdb->RecDN);}
			ws->offset=ws->maxcam-ws->numcam;DB_GetLast(Sdb);
		 }
		 else
		 {adb_get(Sdb->hdbOriginal,OriRec,Sdb->IndexNum);}

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
static BOOL DB_seek(LONG info,SDBEXT_MT *Sdb)
{
	SINT  SReale;
	SINT  FlagCORSA=OFF;
	LONG  PtrFind;
	LONG  b;
	SINT  test;
	HREC  Hrec;
	HREC  HrecApp;
//	TCHAR serv[80];

	if (Sdb->iModeOperation) return 0;
//	dispx("* offset=%d maxcam=%d Enumcam=%d",Sdb->ws.offset,Sdb->ws.maxcam,Sdb->ws.Enumcam);

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
		// Se Š a NULL pi— in gi— no si pu• andare
		if (PtrFind==ADBNULL) return 0;
		adb_get(Sdb->hdbOriginal,PtrFind,Sdb->IndexNum);

		// ------------------------
		// Scrollo di info in gi— |
		// ------------------------
		for (b=0;b<info;b++)
		{
			rifai2:
			// Fine corsa misurata in apertura
			if (Sdb->RecDN)
			{adb_position(Sdb->hdbOriginal,&Hrec);
			 if (Hrec==Sdb->RecDN) {FlagCORSA=ON; break;}
			}

			if (adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD))
				{FlagCORSA=ON;
				 break;// Fine corsa
				}

			// ----------------------------------------------------
			// Se c'Š il filtro controllo la validità del record  |
			// ----------------------------------------------------

			if (Sdb->ExtFilter)
			{
				test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
				if (test==OFF) break; // Fine della ricerca

				if (test==-2||test==2) goto rifai2; // Campo non valido ma continua
			}

			SReale++; // Conta quanti ne trovo
			memmove(Sdb->WinScr,  // Destinazione
					Sdb->WinScr+1,// Sorgente
					(WORD) ((Sdb->ws.numcam-1)*sizeof(struct WINSCR))); // Numero byte
			adb_position(Sdb->hdbOriginal,&HrecApp);
			Sdb->WinScr[(WORD) Sdb->ws.numcam-1].record=HrecApp;

			// Aggiorna l'ultimo movimento
			// #?#
			//adb_position(Sdb->hdbOriginal,&Sdb->buf[(SINT) (Sdb->ws.numcam-1)].record);
		}

		// Aggiorna l'offset
		// Se c'Š stato fine corsa ricalcolo l'offset
		// (offset pu• essere sbagliato per arrotondamento in Perc)
		Sdb->ws.offset+=SReale;
		//if (FlagCORSA) ws.offset=(ws.maxcam-ws.numcam);

#ifdef _WIN32
		//dispx("offset=%d maxcam=%d Enumcam=%d",Sdb->ws.offset,Sdb->ws.maxcam,Sdb->ws.Enumcam);
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
//				Adispfm(0,0,15,0,ON,SET,"SMALL F",3,serv);
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

				 adb_get(Sdb->hdbOriginal,PtrFind,Sdb->IndexNum);
				 for (b=0;b<(info*-1);b++)
				 {
					rifai3:
					 // Fine corsa misurata in apertura
					 if (Sdb->RecUP)
						{adb_position(Sdb->hdbOriginal,&Hrec);
						 if (Hrec==Sdb->RecUP) break;
						}
					if (adb_find(Sdb->hdbOriginal,Sdb->IndexNum,
											 B_GET_PREVIOUS,"",B_RECORD)) break;// Fine corsa

					if (Sdb->ExtFilter)
					 {
					 test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
					 if (test==OFF) break; // Fine della ricerca

					 if (test==-2||test==2) goto rifai3; // Campo non valido ma continua
					 }

					 SReale++;
					 memmove(Sdb->WinScr+1,  // Destinazione
							 Sdb->WinScr,// Sorgente
							 (WORD) ((Sdb->ws.numcam-1)*sizeof(struct WINSCR))); // Numero byte

					 adb_position(Sdb->hdbOriginal,&HrecApp);
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
					 adb_position(Sdb->hdbOriginal,&Sdb->buf[0].record);
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
//				adb_get(Sdb->hdbOriginal,Sdb->buf[Sdb->Dove].record,Sdb->IndexNum);
	adb_get(Sdb->hdbOriginal,Sdb->WinScr[Sdb->Dove].record,Sdb->IndexNum);
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
//	PRG_END("Scr_adbExt1:No disp function");

static SINT DB_load(SDBEXT_MT *Sdb)
{
	SINT swclex;
	SINT a,test;
	//LONG size;
	LONG Hrec;
	SINT Count;
	struct WS_INFO *ws=&Sdb->ws;

	Sdb->RecBuffer=NULL;
	if (Sdb->ExtFunz==NULL) return -1;

	ws->maxcam=adb_recnum(Sdb->hdbOriginal);
	ws->offset=ADBNULL;
	ws->selez=ADBNULL; // Nessun selezionato
	ws->koffset=-1;
	ws->kselez=-1;
	ws->dispext=ON;
	ws->dbult=ADBNULL;
	ws->refre=ON;

	Sdb->RecSize=adb_recsize(Sdb->hdbOriginal);
	Sdb->RecBuffer=adb_DynRecBuf(Sdb->hdbOriginal);
	ws->sizecam=Sdb->RecSize; // ???
	//size=sizeof(struct WINSCR)*ws->numcam; // Calcola Grandezza Buffer
	//printf("Campi %ld[%ld]",ws->numcam,size);

	// -------------------------------------------------------------
	// Se sono in modalità Lista di puntatori con Filtro           |
	// esco subito                                                 |
	// -------------------------------------------------------------
	if (Sdb->iModeOperation)
		{
	     ws->offset=ADBNULL;
	     ws->selez=-1; // Nessun selezionato
		 Sdb->ExtFilter=OFF;
		 ws->maxcam=ADB_info[Sdb->hdbOriginal].AdbFilter->iRecordsReady;
		 Sdb->LastOffset=-1;
		 return 0;
		}

	// -------------------------------------------------------------
	// Creo la lista di puntatatori                                |
	// -------------------------------------------------------------
	LArrayMake(Sdb,ws->numcam);

	// -------------------------------------------------------------
	// Trova i limiti della zona e conta i record se c'Š il filtro |
	// -------------------------------------------------------------

	ws->maxcam=0;
	a=DB_zone(Sdb);

	// Copertura Globale
	if (a==NOACTIVE)
		 {Sdb->ExtFilter=OFF;
			ws->maxcam=adb_recnum(Sdb->hdbOriginal);
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

		 a=(Sdb->ExtFunz)(WS_DO,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);
		 if (a)
			{
			 adb_get(Sdb->hdbOriginal,Sdb->RecUP,Sdb->IndexNum);
			 adb_findperc(Sdb->hdbOriginal,Sdb->IndexNum,&perc1);
			 adb_get(Sdb->hdbOriginal,Sdb->RecDN,Sdb->IndexNum);
			 adb_findperc(Sdb->hdbOriginal,Sdb->IndexNum,&perc2);

			 // Proporzione:  (perc2-perc1):100=x:maxrec
			 Rec=(LONG) ((perc2-perc1)*(float) adb_recnum(Sdb->hdbOriginal)/100);
			 ws->maxcam=Rec;
			 goto NU;
			}
		}

	// ---------------------------------
	// Conto i Record                  |
	// ---------------------------------

	adb_get(Sdb->hdbOriginal,Sdb->RecUP,Sdb->IndexNum);
	swclex=0;
	Count=0;
	do
	 {
		test=(Sdb->ExtFunz)(WS_FILTER,0,0,0,Sdb->hdbOriginal,Sdb->IndexNum);

		// Fine della ricerca per fine Dbase
		if (test==OFF) break;

		// Registro la numerazione dei record Buoni
		if (test==ON) ws->maxcam++;

		// Se ho un record di limite lo controllo
		if (Sdb->RecDN)
			{adb_position(Sdb->hdbOriginal,&Hrec);
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

	 } while (!adb_find(Sdb->hdbOriginal,Sdb->IndexNum,B_GET_NEXT,"",B_RECORD));

	 // --------------------------------------------
	 // Se non avevo il record limite lo registro  |
	 // --------------------------------------------

	 if (!Sdb->RecDN)
			{adb_position(Sdb->hdbOriginal,&Sdb->RecDN);
			 adb_findperc(Sdb->hdbOriginal,Sdb->IndexNum,&Sdb->FiltDN);
			}

	 NU:
	 mouse_graph(0,0,"MS01");
	 return 0;
}

void SdbPreset32(struct OBJ *pojStruct,
			     CHAR *ObjName,
			     SINT (*FunzDisp)(SINT cmd,CHAR *PtDati,LONG  dato,void  *str,SINT Hdb,SINT IndexNum),
			     SINT Hdb,
			     SINT Index,
			     SINT Filter,
			     void *dato,
				 DWORD dwParam)
{
//	obj_setCall(poj,ObjName); // Setto il puntatore all'oggetto
	struct OBJ *poj;
	SDBEXT_MT *pSdb;
	poj=ObjFindStruct(pojStruct,ObjName); if (!poj) ehExit("DriverObjFind [%s] ?",ObjName);
	pSdb=ObjToSdb(poj);

//	SdbDriver32(poj,WS_EXTFUNZ,0,(CHAR *) FunzDisp);   // Setta l'indice da usare
	pSdb->ExtFunz=FunzDisp;
	pSdb->iModeOperation=-1; // Modalità disattivata
	pSdb->hdbOriginal=Hdb;
	pSdb->IndexNum=Index;
	(pSdb->ExtFunz)(WS_HDB,NULL,0,NULL,pSdb->hdbOriginal,pSdb->IndexNum);
	(pSdb->ExtFunz)(WS_IDX,NULL,0,NULL,pSdb->hdbOriginal,pSdb->IndexNum);

	if (Filter) SdbDriver32(poj,Filter,ON,(CHAR *)dato);  
	pSdb->dwParam=dwParam;
}

