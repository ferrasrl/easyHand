//   
//   OdbcScroll
//   Funzioni di appoggio per Zona Actve            
//
//   							by Ferrà srl 2008
//   

#include "/easyhand/inc/easyhand.h"
#ifndef EH_SQL_ODBC
#include "/easyhand/inc/eh_odbc.h"
#endif
#include "/easyhand/inc/OdbcScroll.h"

#ifdef _DEBUG
// #define OS_DEBUG 1
#endif

#ifdef EH_ODBC

// extern EH_ODBC_SECTION sOdbcSection;
// Numero massimo di scroll contemporaneamente gestiti = 5
// Numero massimo di linee per scroll = 30
// #define	 MAX_Y 30

// #define SQDMAX 10
#define ESQL_MAXEVENTLOAD 3
#define ESQL_STOP  0
#define ESQL_EXECUTE 1
#define ESQL_FETCH 2

#define WB_CLONE 0
#define WB_ORIGINAL 1


// Adattatore di chiamata alkla funzione
// EH_OBJ * pObj,BOOL * pbReturn, EN_EXT_NOTIFY enMode, HWND hWnd, UINT message, LONG wParam, LONG lParam
#define _adaptor(obj,mess,info,psExt,pRes) obj, (BOOL *) psExt,EXT_CALL,NULL,mess,info,(LONG) pRes 

typedef struct {

	struct OBJ *	ObjClient;
	LRESULT (*funcNotify)(EH_NOTIFYPARAMS);//struct OBJ *ObjClient,INT enMess,LONG  dato,void *str,EH_ODBC_RS pRes);
	struct WS_INFO ws;

	INT		iPlatform; // 0=non definita

	LONG    lRowsRequest;	// Righe richieste del nuovo buffer
//	LONG	lRowsBuffer;	// Righe possibili nel buffer
	LONG	lRowsReady; // Righe lette dopo il Fetch
	LONG	lRowsTotal; // Numero totali di linee presenti con questa Query
	LONG	lRowsCurrent; // 
	BOOL	fChanged; // T/F se qualcosa è cambiato
	BOOL	bSqlError;

	//
	// Gestione MultiThread
	//
	HANDLE	hEventSql[ESQL_MAXEVENTLOAD];
	HANDLE	hThread;
//	HDB		HdbFind;
	DWORD	dwThread;
	BOOL	fInProcess; // Il di ricerca è in azione
	BOOL	fInStop;
	BOOL	fBreak; // Chiedo l'interruzione della ricerca
	BOOL	fDataReady; // T/F quando i dati sono pronti

	// SQL
	EH_ODBC_SECTION * psOdbcSection;
	//SQLHSTMT hStmtScroll; // Handle Stantment Clone
	EH_ODBC_RS	rsSet;
	
	BOOL	fQueryToExecute; // T/F se la query è da eseguire

//	CHAR *	pszSchema;		// Schema/Library di default {LIB}
	CHAR *	pQuerySelect;	// La query di ricerca con il select
	CHAR *	pQueryActive;	// Puntatore alla Query Attiva al momento
	CHAR *	pQueryCount;	// Comando SELECT COUNT(*) per contare i record
	CHAR *	pLastErrorText;// Ultimo errore sql testuale
	BOOL	fCursorOpen;	// T/F se aperto
	
	CHAR *	pszKeyCode;	// Campi che compongono la chiave univoca
	EH_AR	arKeyCodeFld;
	INT		iKeyCodePart;

	CHAR *	pCodeFocused;	// Valore del codice in focus
	CHAR *	pCodeSelected;	// Valore del codice in selected
	CHAR *	pCodeReturn;	// Valore del codice in code return (doppio click)
	INT		iSQLOffset; 
	BOOL	bAutoRowSelect;	// T/F autoselect (default TRUE)
	BOOL	bDrawLineDiv;	// T/F se devo disegnare la linea di separazione (default TRUE)
//	INT iHdlFields;
//s	CHAR **lppFields;

	CRITICAL_SECTION csSqdStruct; // Accesso ad area generale
	CRITICAL_SECTION csSqdQuery; // Accesso ad area generale

} S_SQLINFO;

static struct {

	BOOL bReady;
	EH_LST lstOdbc;

} _p={false};


//static BOOL   bFirstTime=TRUE;
//static S_SQLINFO arSqlInfo[SQDMAX];
// static INT  DB_find(INT enMess,LONG info,CHAR *str,S_SQLINFO *Sdb);
static void _LFetch(S_SQLINFO *psOdbc);
static void _LQueryExecute(S_SQLINFO *psOdbc,INT iLine);
static void _FTSqlThreadRefresh(INT enMess,void *dao);
static DWORD WINAPI _OdbcExecuteThread(LPVOID Dato);
static BOOL _LSqlTry(CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult,BYTE *pQuery);
static void _LKeyCodeBuilder(CHAR **pStr,S_SQLINFO *psOdbc);
static void _LMessage(S_SQLINFO *psOdbc,BOOL bClear,CHAR *lpMessage);
static INT _LBox(HDC hDC,INT x1,INT y1,INT x2,INT y2,LONG colPen,LONG colBg);

static void _odbcScrollFreeResource(BOOL bStep) {
	if (!_p.bReady) return;
	_p.lstOdbc=lstDestroy(_p.lstOdbc);
	FTIME_off(_FTSqlThreadRefresh);
	_p.bReady=false;
}
//
// Driver Odbc
//
void * OdbcScroll(struct OBJ * psObjCaller,EN_MESSAGE enMess,LONG info,CHAR *str)
{
	static struct WINSCR rit,*PtScr;
	EH_DISPEXT *psExt;
	S_SQLINFO sSqlInfo;
    S_SQLINFO * psOdbc=NULL;
	INT   a;
	EH_LST_I * psLi;

/*
#ifndef EH_ODBC_MT
	EH_ODBC_SECTION * _psOdbcSection=&sOdbcSection;
#else
	EH_ODBC_SECTION * _psOdbcSection=arsOdbcSection;
#endif
*/


	// Inizializzazione
	if (!_p.bReady) {

		_(_p);
		_p.lstOdbc=lstCreate(sizeof(S_SQLINFO));
		_p.bReady=true;

		// DEVO AGGIUNGERE UNA FUNZIONE DI CHIUSURA IN USCITA !!!!!
		ehAddExit(_odbcScrollFreeResource);

		FTIME_on(_FTSqlThreadRefresh,1);
	}


	// Oggetto buono ?
	if ((psObjCaller->tipo!=OW_SCR)&&(psObjCaller->tipo!=OW_SCRDB)) return 0;

	// ---------------------------------------
	// Trova il Client Object                |
	// ---------------------------------------
//	if (!psObjCaller->pOther) {

/*
		//ptClient=-1;
		// for (b=0;b<SQDMAX;b++) {if (psObjCaller==psOdbc->ObjClient) {ptClient=b; break;}}
		//
		// Se non c'è provo ad assegnarlo
		//
		if (ptClient==-1) {

			for (b=0;b<SQDMAX;b++) {

				if (!psOdbc->ObjClient)
					{psOdbc->ObjClient=psObjCaller;
					 ptClient=b;
					 break;
					}

			}
		 }

		//
		// Se non ci riesco ... errore
		//
		if (ptClient==-1) ehExit("SQLDriver:Too client!");
		psObjCaller->pOther=psOdbc=&arSqlInfo[ptClient];
		*/
	
	psOdbc=psObjCaller->pOther;

	if (enMess==WS_INF) return &psOdbc->ws;

	switch (enMess) 
	{
	// -----------------------------------
	// APERTURA DEL DRIVER               |
	// -----------------------------------

		case WS_CREATE: //break;

			_(sSqlInfo);
			psOdbc=psObjCaller->pOther=lstPush(_p.lstOdbc,&sSqlInfo);
			psOdbc->ObjClient=psObjCaller;

#ifdef OS_DEBUG
			fprintf(stderr,"OdbcScroll: Create" CRLF);
#endif
			psObjCaller->bFreeze=true; // Blocco la gestione dell'oggetto
			psOdbc->bDrawLineDiv=true;

			//
			// Alla prima chiamata creo thread e "finestrame" necessario 
			// 

			// Tecnologia ODBC
			//		- La connessione deve essere inizializzata
			//
			if (!psOdbc->hThread)
			{
				memset(&psOdbc->csSqdStruct,0,sizeof(CRITICAL_SECTION));
				memset(&psOdbc->csSqdQuery,0,sizeof(CRITICAL_SECTION));
				InitializeCriticalSection(&psOdbc->csSqdStruct); 
				InitializeCriticalSection(&psOdbc->csSqdQuery); 
/*			
				// 
				// Alloco lo stantment clone ( Si libererà con WS_DESTROY)
				//
				// NOTA: Pericoloso, cambiare id Thread con numerazione differente
				//
				psOdbc->psOdbcSection=odbcSectionStmtClone(_psOdbcSection,1000+_p.lstOdbc->iLength); 

				//
				// Cose che mi servono per lo scroll
				//

				// Creo 
				//sqlReturn=SQLAllocHandle(SQL_HANDLE_STMT, _psOdbcSection->hConn, &psOdbc->psOdbcSection->hStmt);
				//if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO) ehExit("OdbcScroll:hStmt Clone impossible %d",sqlReturn);
				
				// Dico che sono in sola lettura
				SQLSetStmtAttr(psOdbc->psOdbcSection->hStmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_READ_ONLY, 0);
				// Dico che sullo stantment
				sqlReturn=SQLSetStmtAttr(psOdbc->psOdbcSection->hStmt, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0);
				if (sqlReturn==SQL_ERROR)  
				// Altro metodo
				{
					sqlReturn=SQLSetStmtAttr( psOdbc->psOdbcSection->hStmt,SQL_ATTR_CURSOR_TYPE,  (SQLPOINTER) SQL_CURSOR_STATIC, 0);
					if (sqlReturn==SQL_ERROR) win_infoarg("errore in assegnazione cursore");
					sqlReturn=SQLSetStmtAttr( psOdbc->psOdbcSection->hStmt, SQL_ATTR_USE_BOOKMARKS, (SQLPOINTER) SQL_UB_VARIABLE, 0);
					if (sqlReturn==SQL_ERROR) win_infoarg("SQL_ATTR_USE_BOOKMARKS");
				}
*/
				for (a=0;a<ESQL_MAXEVENTLOAD;a++) psOdbc->hEventSql[a]=CreateEvent(NULL,TRUE,FALSE,NULL); 

				//
				// 4) Creo il Thread (SQLExecuteThread) per l'elaborazione delle query
				//
				psOdbc->hThread = CreateThread(NULL, 
											  0, 
											  _OdbcExecuteThread, 
											  (LPDWORD) psOdbc,
											  0, 
											  &psOdbc->dwThread);
				psOdbc->bAutoRowSelect=TRUE; // <-- 2010 - Inserito auto select in partenza
				SetThreadPriority(psOdbc->hThread,THREAD_PRIORITY_NORMAL);
			}
			break;

		case WS_OPEN:

		//
		//	Inizializzazione della finestra
		//	
			if (info<3) {ehExit("Field ? " __FUNCTION__);}
			psOdbc->ws.numcam=info;// Assegna il numero di campi da visualizzare
			psOdbc->ws.selez=-1; 
			psOdbc->ws.maxcam=0;
			psOdbc->ObjClient->tipo=OW_SCR;
			psOdbc->lRowsTotal=0;
			psOdbc->fChanged=0;
			psOdbc->ObjClient->bFreeze=FALSE;

			if (psOdbc->funcNotify) psOdbc->funcNotify(_adaptor(psObjCaller,WS_OPEN,0,NULL,NULL));//psObjCaller,NULL,0,NULL,WS_OPEN,NULL,NULL);
			psOdbc->ws.bExtSelection=TRUE; // Gestione esterna della selezione
			psOdbc->ws.fNoBlankLine=TRUE;
			

		case WS_LOAD:  
			break;

	// -----------------------------------
	// Richiesta di refresh
	// -----------------------------------
	case WS_RELOAD: 
			if (info)
				_LQueryExecute(psOdbc,__LINE__); 
				else
				_LFetch(psOdbc); 
				
			return NULL; // Non serve stampare

	// -----------------------------------
	// WS_CLOSE IL DRIVER                  |
	// -----------------------------------
	case WS_CLOSE:
			OdbcScroll(psObjCaller,WS_PROCESS,STOP,NULL); // Chiedo di fermare il processo in corso
			psOdbc->ObjClient->bFreeze=TRUE;
			if (psOdbc->funcNotify) (psOdbc->funcNotify)(_adaptor(psObjCaller,WS_CLOSE,0,NULL,NULL));
			break;

	// -----------------------------------
	// CHIUSURA DEFINITIVA DEL GESTORE (chiamato in obj_close());
	// -----------------------------------
	case WS_DESTROY: 

			//
			// Notifico la chiusura alla funzione esterna
			//
			if (psOdbc->funcNotify) (psOdbc->funcNotify)(_adaptor(psObjCaller,WS_DESTROY,0,NULL,NULL));

			// Fermo il Thread
			SetEvent(psOdbc->hEventSql[ESQL_STOP]); // Segnalo che siamo in chiusura
			if (WaitForSingleObject(psOdbc->hThread,5000)) 
			{
				TerminateThread(psOdbc->hThread,0);
			}

			// Libero il Cursore e la memoria usata per il dizionario
			//ThreadResultSetFree(psOdbc);

			// Libero lo spazio per lo stantment aggiunto (Clone)
			/*
			odbc_free_result(psOdbc->rsSet); psOdbc->rsSet=NULL;
 			odbcSectionDestroy(psOdbc->psOdbcSection->idThread);
			psOdbc->psOdbcSection=NULL;
			*/
			//psOdbc->rsSet=NULL;

			// Libero la memoria usata per la Query
			ehFreePtr(&psOdbc->pQueryActive);
			ehFreePtr(&psOdbc->pQueryCount);
			ehFreePtr(&psOdbc->pLastErrorText);

			// Libero la memoria usata per la WhereAdd
			ehFreePtr(&psOdbc->pQuerySelect);
//			ehFreePtr(&psOdbc->pszSchema);

			ehFreePtr(&psOdbc->pszKeyCode);
			ARDestroy(psOdbc->arKeyCodeFld);
			ehFreePtr(&psOdbc->pCodeFocused);
			ehFreePtr(&psOdbc->pCodeSelected);
			ehFreePtr(&psOdbc->pCodeReturn);

			// Libero gli Handle degli eventi
			for (a=0;a<ESQL_MAXEVENTLOAD;a++) CloseHandle(psOdbc->hEventSql[a]);

			// 1.2.3. Liberi tutti !!
			DeleteCriticalSection(&psOdbc->csSqdStruct); 
			DeleteCriticalSection(&psOdbc->csSqdQuery); 

			// Azzero la struttura di riferimento
			//memset(psOdbc,0,sizeof(S_SQLINFO));

			//psOdbc=psObjCaller->pOther=lstPush(_p.lstOdbc,&sSqlInfo);
			psLi=lstSearch(_p.lstOdbc,psOdbc); if (!psLi) ehError();
			lstRemoveI(_p.lstOdbc,psLi);
			break;

	// -----------------------------------
	// PRESSIONE DI UN TASTO/MOUSE       |
	// -----------------------------------
	case WS_KEYPRESS:
			if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
			if (psOdbc->funcNotify) (psOdbc->funcNotify)(_adaptor(psObjCaller,enMess,info,str,NULL)); 
			break;

	// -----------------------------------
	// SETTA SELEZIONE RECORD            |
	// -----------------------------------
	case WS_SEL: 
			if (!psOdbc->funcNotify) break;//ehExit(SdbDriver32 ":No ext/disp function");
			// if (!psOdbc->rsSet) break;
			// fprintf(stderr,"%d",psOdbc->rsSet->iCurrentRow);
			//if (psOdbc->ws.selez==-1) break;
			break;

	// -------------------------------------
	// SETTA L'OFFSET  (Solo Modo O_SCROLL |
	// -------------------------------------
	case WS_OFF: break;

	// -------------------------------------
	// 
	// -------------------------------------
	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata
	case WS_GET_SELECTED:

			ZeroFill(rit);
			rit.record=-1;

			if (strEmpty(psOdbc->pszKeyCode)) {efx2(); return NULL;}
			//
			// Selezione in scroll
			//
			if (psOdbc->rsSet&&psOdbc->ws.selez>-1)
			{
				psOdbc->rsSet->iCurrentRow=psOdbc->ws.selez;
				if (strEmpty(psOdbc->pszKeyCode)) ehExit("%s:%d manca assegnazione pszKeyCode",__FILE__,__LINE__);
				_LKeyCodeBuilder(&psOdbc->pCodeReturn,psOdbc);

				rit.record=psOdbc->ws.selez;
				rit.keypt=psOdbc->pCodeReturn;
				if (psOdbc->bAutoRowSelect)
				{
					strAssign(&psOdbc->pCodeSelected,psOdbc->pCodeReturn);
					InvalidateRect(psObjCaller->hWnd,NULL,FALSE);
				}
			}
			//
			// Pre-selezione (non entrao in scroll
			//
			else if (!strEmpty(psOdbc->pCodeSelected)) {

				rit.record=psOdbc->ws.selez;
				rit.keypt=psOdbc->pCodeSelected;
			}
			if (enMess==WS_GET_SELECTED) return valCreate(_ALFA,rit.record,rit.keypt);
			return &rit;

	// -------------------------------------
	// RITORNA Selez ??????????            |
	// -------------------------------------
	case WS_REALGET:
			 //ehExit("SDB:Uso RealGet ?");
			 //return (&psOdbc->ws.selez);

			 //iPointer=psOdbc->ws.offset+psExt->ncam;
			 //LResultSetToAdb(WB_CLONE,psOdbc,iPointer-psOdbc->iSQLOffset,TRUE);
			 break;

	// -------------------------------------
	// Refresh a ON                        |
	// -------------------------------------
	case WS_REFON : psOdbc->ws.refre=ON; break;
	case WS_REFOFF : psOdbc->ws.refre=OFF; break;

	case WS_PROCESS:
		if (info==STOP&&!str)
		 {
			SQLCancel(psOdbc->psOdbcSection->hStmt); // Cancello il processo nello stantment
			EnterCriticalSection(&psOdbc->csSqdStruct);
			psOdbc->fBreak=TRUE;
			LeaveCriticalSection(&psOdbc->csSqdStruct);
			while (true) {if (!psOdbc->fInProcess) break; else Sleep(50);}
			//win_infoarg("SQL stop");
		}
		// Controllo se l'elaborazione è in corso
		if ((info==0)&&(*str=='?'))
		{
		  if (psOdbc->fInProcess) return "!"; else return NULL;
		}
		break;

	
	// 
	// Chiedo di cambiare il la Where di ricerca
	//
	case WS_SETFILTER:

		EnterCriticalSection(&psOdbc->csSqdStruct);
		ehFreePtr(&psOdbc->pQuerySelect); 
	 	psOdbc->pQuerySelect=strDup(str);
		psOdbc->ws.selez=-1;
		psOdbc->ws.maxcam=0;
		psOdbc->lRowsReady=0;
		psOdbc->lRowsTotal=0;
		LeaveCriticalSection(&psOdbc->csSqdStruct);
		if (info)
		{
			InvalidateRect(psObjCaller->hWnd,NULL,TRUE);
			_LMessage(psOdbc,TRUE,"Attendere\nRichiesta al server ...");
			OsEventLoop(5);
		}
		psOdbc->fQueryToExecute=TRUE;
		// _LQueryExecute(psOdbc,__LINE__); // Me ne arriva uno dopo
		break;

	case WS_SETFLAG:
		psObjCaller->pOtherEx=str; // Assegno un puntatore esterno
		if (psOdbc->funcNotify) (psOdbc->funcNotify)(_adaptor(psObjCaller,WS_SETFLAG,info,NULL,NULL)); 
		break;

	// -------------------------------------
	// Richiesta di Stampa dei Dati        |
	// -------------------------------------
	case WS_DISPLAY : //			  			Richiesta buffer
			 
			psExt=(EH_DISPEXT *) str;

			if (!psOdbc->funcNotify) {
				Tboxp(	psExt->rClientArea.left,
						psExt->rClientArea.top,
						psExt->rClientArea.right,
						psExt->rClientArea.bottom-1,
						RGB(255,128,0),SET);
//						arError=ARCreate(psOdbc->pLastErrorText,"\n",&iRow);
				dispf(psExt->px+1,psExt->py+1,RGB(255,255,255),-1,STYLE_NORMAL,"SMALL F",3,"-Not func for Display-");
				break;
			}

			//
			// Richiesta di stampa del titolo
			//
			if (psExt->ncam==-10) 
			{
				psOdbc->funcNotify(_adaptor(psObjCaller,WS_DISPLAY,0,psExt,NULL));//psOdbc->Hdb,psOdbc->iIndex); 
				break;
			}

			if (!psOdbc->rsSet) 
			{
				if (psOdbc->bSqlError)
				{
					Tboxp(psExt->px,psExt->py,psExt->px+psExt->lx-1,psExt->py+psExt->ly-1,sys.Color3DShadow,SET);
					if (psOdbc->pLastErrorText)
					{
						EH_AR arError;
						INT iRow;
						arError=ARCreate(psOdbc->pLastErrorText,"\n",&iRow);
						if (psExt->ncam<iRow) dispf(psExt->px+1,psExt->py+1,RGB(255,255,255),-1,STYLE_NORMAL,"SMALL F",3,arError[psExt->ncam]);
						ARDestroy(arError);
					}
				}
				else
				Tboxp(psExt->px,psExt->py,psExt->px+psExt->lx-1,psExt->py+psExt->ly-1,sys.Color3DLight,SET);
				break;
			 }

			 psExt->col1=GetSysColor(COLOR_MENUTEXT); 
			 psExt->col2=GetSysColor(COLOR_WINDOW);

			 if (psExt->bFocus) // selezione con il mouse
				{
					psExt->col1=GetSysColor(COLOR_HIGHLIGHTTEXT); 
					psExt->col2=ColorLum(GetSysColor(COLOR_HIGHLIGHT),30);
				} 

			 if (psExt->bSelected) // Selezionato fisso
				{
					psExt->col1=GetSysColor(COLOR_HIGHLIGHTTEXT); 
					psExt->col2=GetSysColor(COLOR_HIGHLIGHT); 
				} 

			Tboxp(psExt->px,psExt->py,psExt->px+psExt->lx-1,psExt->py+psExt->ly-2,psExt->col2,SET); //Sleep(100);

			psOdbc->rsSet->iCurrentRow=info;
			psOdbc->rsSet->iOffset=psOdbc->iSQLOffset;

			// Non ho ancora le linee da visualizzare
			if (psExt->ncam>=psOdbc->lRowsReady||
				(psOdbc->rsSet->iCurrentRow<psOdbc->iSQLOffset)||
				((psOdbc->rsSet->iCurrentRow-psOdbc->iSQLOffset)>=psOdbc->rsSet->iRowsLimit)) 
			{
				boxBrush(psExt->px,psExt->py,psExt->rClientArea.right,psExt->rClientArea.bottom,HS_VERTICAL,sys.Color3DLight,ColorLum(sys.Color3DHighLight,-10));
				Tline(psExt->px,psExt->rClientArea.bottom,psExt->rClientArea.right,psExt->rClientArea.bottom,sys.Color3DShadow,SET);
				break;
			}

			// RIchiedo stampa della riga
			psOdbc->funcNotify(	_adaptor(psObjCaller,WS_DISPLAY,info,psExt, psOdbc->rsSet));

			if (psOdbc->bDrawLineDiv) line(psExt->px,psExt->rClientArea.bottom,psExt->rClientArea.right,psExt->rClientArea.bottom,sys.colScrollDiv,SET);
			if (psExt->bFocus) {
				boxFocus(psExt->px,psExt->py,psExt->px+psExt->lx-1,psExt->py+psExt->ly-2);
			}
			break;

	// --------------------------------------------------------------------------
	// SET della funzione esterna
	// --------------------------------------------------------------------------
	case WS_EXTNOTIFY: 
			 psOdbc->funcNotify=(LRESULT (*)(EH_NOTIFYPARAMS)) str;//(INT (*)(struct OBJ *,INT ,LONG  ,void  *str,EH_ODBC_RS )) str;
			 psOdbc->funcNotify(_adaptor(psObjCaller,WS_CREATE,0,NULL,NULL));
			 break;

	case WS_LINEVIEW: //	 
		psOdbc->ws.numcam=info;
		break;

	case WS_LINEEDIT: //	 
		psOdbc->ws.Enumcam=info;
		break;

	case WS_SIZE: break;
	case WS_MOVE: break;

	case WS_SETTITLE: break;
	case WS_CODENAME: 
		
		strAssign(&psOdbc->pszKeyCode,str); strTrim(psOdbc->pszKeyCode); 
		ARDestroy(psOdbc->arKeyCodeFld);
		psOdbc->arKeyCodeFld=ARCreate(psOdbc->pszKeyCode,"+",&psOdbc->iKeyCodePart);
		break;

	//
	// Riga in FOCUS
	//
		/*
	case WS_SET_ROWFOCUS: 
			if (!str) // Dal gestore oggetti
			{
				 if (info<0) {strAssign(&psOdbc->pCodeFocused,NULL); break;}
				 if (psOdbc->pCodeName&&psOdbc->rsSet)
				 {
					psOdbc->rsSet->iCurrentRow=info;
					psOdbc->rsSet->iOffset=psOdbc->ws.offset;

					// psOdbc->rsSet->iCurrentRow riga della query

					if (psOdbc->rsSet->iCurrentRow>=psOdbc->iSQLOffset&&
						psOdbc->rsSet->iCurrentRow<(psOdbc->iSQLOffset+psOdbc->lRowsReady))
					{
						strAssign(&psOdbc->pCodeFocused,sql_ptr(psOdbc->rsSet,psOdbc->pCodeName));
						strTrim(psOdbc->pCodeFocused);
					}
					else
					{
						strAssign(&psOdbc->pCodeFocused,NULL); // Non ho la selezione richiesta in memoria
					}
					dispxEx(0,80,"%d,%d = %s          ",
							psOdbc->rsSet->iCurrentRow,
							psOdbc->iSQLOffset+psOdbc->lRowsReady,
							psOdbc->pCodeFocused);
				 }
				return NULL;
			}
			else // Esterno
			{
				strAssign(&psOdbc->pCodeFocused,str);
				InvalidateRect(psObjCaller->hWnd,NULL,FALSE);
			}
			break;

	case WS_GET_ROWFOCUS: 
			 if (psOdbc->pCodeName&&psOdbc->pCodeFocused&&psOdbc->rsSet)
			 {
				CHAR *p;
				if (info<0) return NULL;
				psOdbc->rsSet->iCurrentRow=info;
				psOdbc->rsSet->iOffset=psOdbc->ws.offset;

				p=sql_ptr(psOdbc->rsSet,psOdbc->pCodeName); strTrimRight(p);
				if (!strcmp(p,psOdbc->pCodeFocused)) break;
			 }
			 return NULL;
*/

	//
	// Riga in SELECTED
	//
	case WS_SET_ROWSELECTED: 
			 if (!str) // Dal gestore oggetti
			 {
				 if (info<0) {strAssign(&psOdbc->pCodeSelected,NULL); InvalidateRect(psObjCaller->hWnd,NULL,FALSE); break;}
				 if (psOdbc->pszKeyCode&&psOdbc->rsSet)
				 {
					psOdbc->rsSet->iCurrentRow=info;
					psOdbc->rsSet->iOffset=psOdbc->ws.offset;
					_LKeyCodeBuilder(&psOdbc->pCodeSelected,psOdbc); //if (!strEmpty(p)) break;
				 }
				InvalidateRect(psObjCaller->hWnd,NULL,FALSE);
				return NULL;
			}
			else // Esterno
			{
				strAssign(&psOdbc->pCodeSelected,str);
				InvalidateRect(psObjCaller->hWnd,NULL,FALSE);
			}
			break;

	case WS_GET_ROWSELECTED: 

			//
			// Controllo di selezione
			// 
			 if (psOdbc->pszKeyCode&&!strEmpty(psOdbc->pCodeSelected)&&psOdbc->rsSet)
			 {
				CHAR *p=NULL;
				if (info<0) return NULL;

				psOdbc->rsSet->iCurrentRow=info;
				psOdbc->rsSet->iOffset=psOdbc->ws.offset;
				_LKeyCodeBuilder(&p,psOdbc); // Può ritornare Null in p se sto chiedendo una riga che non c'è (in fase di ingrandimento)
				if (!strCmp(p,psOdbc->pCodeSelected)) {ehFreeNN(p); break;}
				ehFreeNN(p); 
			 }
			 return NULL;

	case WS_EVENT:
			if (!psOdbc->rsSet) break;
			psOdbc->rsSet->iCurrentRow=info;
			psOdbc->rsSet->iOffset=psOdbc->iSQLOffset;
			if (psOdbc->funcNotify) 
			{
				BYTE *ptr=(BYTE *) (psOdbc->funcNotify)(_adaptor(psObjCaller,enMess,info,str,psOdbc->rsSet));
				return ptr;
			}
			break;

	case WS_BUF:
			break;

	default:

		if (psOdbc->funcNotify) 
			(psOdbc->funcNotify)(_adaptor(psObjCaller,enMess,info,str,psOdbc->rsSet));
			else
			fprintf(stderr,__FUNCTION__ "Event ? [%d][%s]" CRLF,enMess,psObjCaller->nome); 
		break;
	}

	//PtScr=psOdbc->WinScr; 
	PtScr=NULL;
	if (!PtScr) PtScr=&rit;
	return PtScr;
}


static void SqlLogWrite(CHAR *Mess,...)
{	
	FILE *ch;
	CHAR *lpFile="c:\\Eh3\\SqlScroll.log";
	CHAR szTime[80];

	va_list Ah;
	va_start(Ah,Mess);
	sprintf(szTime,"%s %s | ",dateFor(dateToday(),"/"),hourNow("hms"));

	// Azzeramento
    ch=fopen(lpFile,"ab"); if (ch==NULL) ch=fopen(lpFile,"wb");
	if (ch!=NULL)
	{
		fprintf(ch,szTime);
		if (Mess!=NULL) vfprintf(ch,Mess,Ah);
		fprintf(ch,CRLF);
		fclose(ch);
	}
	
	va_end(Ah);
}

//
// _LSqlTry() : Ritorna True se ci sono errori
//

static BOOL _LSqlTry(CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult,BYTE *pQuery) 
{
   SQLRETURN rc = iResult; 
//   sEmu.sqlLastError=iResult;
   if (rc != SQL_SUCCESS) 
      { 
		 if (rc != SQL_SUCCESS_WITH_INFO) 
		 {
			// ehExit(SQLDisplay(WhoIs,hstmt,rc));

#ifdef _DEBUG
//			 fprintf(stderr,"%s" CRLF,SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc)); 
			 fprintf(stderr,CRLF "*** ERROR ***" CRLF);
			 fprintf(stderr,"Q: [%s]" CRLF,pQuery);
			 fprintf(stderr,"%s" CRLF,odbcDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
#endif
//			 SqlLogWrite("Q: [%s]",pQuery);
//			 SqlLogWrite("%s",OdbcDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
		 }
		 return TRUE;
      } 
   else return FALSE;
}


static INT _LBox(HDC hDC,INT x1,INT y1,INT x2,INT y2,LONG colPen,LONG colBg) {
	HPEN hpen, hpenOld;
	POINT Point;
	HBRUSH hbr,hbrOld;

	hpen=CreatePen(PS_SOLID, 0, colPen); 
	hpenOld = SelectObject(hDC, hpen);
	hbr = CreateSolidBrush(colBg);        // Creo un nuovo pennello
	hbrOld= SelectObject(hDC, hbr);

	Rectangle(hDC, x1, y1, x2, y2);

	MoveToEx(hDC,x1,y1,&Point); 
	LineTo(hDC,x2,y1);
	LineTo(hDC,x2,y2); LineTo(hDC,x1,y2);
	LineTo(hDC,x1,y1);

	SelectObject(hDC, hbrOld);
	DeleteObject(hbr);
	SelectObject(hDC, hpenOld);
	DeleteObject(hpen); // Cancella la penna usata
	return 0;
}


static void _LMessage(S_SQLINFO *psOdbc,BOOL bClear,CHAR *lpMessage)
{
	HDC hdc;
	RECT rWin;
	SIZE sWin;
	HFONT hFont,hFontOld=NULL;
	RECT rText;
	SIZE sText;

	hdc=GetDC(psOdbc->ObjClient->hWnd);
	GetWindowRect(psOdbc->ObjClient->hWnd,&rWin);
	sWin.cx=rWin.right-rWin.left+1;
	sWin.cy=rWin.bottom-rWin.top+1;
	
	sText.cx=180; sText.cy=50; // Larghezza ed altezza del rettangolo
	
	rText.left=(sWin.cx-sText.cx)/2; rText.right=rText.left+sText.cx+1;
	rText.top=(sWin.cy-sText.cy)/2; rText.bottom=rText.top+sText.cy+1;

	if (bClear) {

		RECT rArea;
		ZeroFill(rArea);
		rArea.right=sWin.cx-1; rArea.bottom=sWin.cy-1;
		dcBoxBrush(hdc,&rArea,HS_BDIAGONAL,sys.Color3DShadow,sys.Color3DLight);
		/*
		RECT rArea;
//		HBRUSH backBrush=CreateSolidBrush(sys.Color3DShadow);
		HBRUSH diagBrush=CreateHatchBrush(HS_BDIAGONAL,sys.Color3DLight);
		SetBkColor(hdc,sys.Color3DShadow);
		ZeroFill(rArea);
		rArea.right=sWin.cx-1; rArea.bottom=sWin.cy-1;
//		FillRect(hdc, &rArea, backBrush);
		FillRect(hdc, &rArea, diagBrush);
//		DeleteObject(backBrush);
		DeleteObject(diagBrush);
		*/
	}

	if (!strEmpty(lpMessage)) {

		RECT rcText;
		SIZE sCalc;
		SetBkMode(hdc,OPAQUE);
		SetBkColor(hdc,RGB(255,255,255));
		SetTextColor(hdc,RGB(120,120,120));

		hFont=GetStockObject(SYSTEM_FONT);
		hFontOld = SelectObject(hdc, hFont);
		dcRectRound(hdc,&rText,ColorLum(sys.Color3DShadow,-20),RGB(255,255,255),24,24,3);

		ZeroFill(rcText);
		DrawText(hdc,
				 lpMessage,strlen(lpMessage),
				 &rcText,    // pointer to struct with formatting dimensions
				 DT_CALCRECT// text-drawing flags
				);

		sizeCalc(&sCalc,&rcText);
		
		//rText.left
		rText.left+=2; 
		rText.top+=(sText.cy-sCalc.cy)>>1;
		//rText.right
		//rText.right-=2; rText.bottom-=2;

		DrawText(hdc,
				 lpMessage,
				 strlen(lpMessage),
				 &rText,    // pointer to struct with formatting dimensions
				 DT_CENTER|DT_VCENTER// text-drawing flags
				);
	}
	else
	{
		_LBox(hdc,rText.left,rText.top,rText.right,rText.bottom,sys.ColorWindowBack,sys.ColorWindowBack);
		//_LBox(hdc, &rText, sys.ColorWindowBack,1,sys.ColorWindowBack);
	}

	//SetMapMode(hDC, Mmp);
	if (hFontOld) SelectObject(hdc, hFontOld);

	ReleaseDC(psOdbc->ObjClient->hWnd,hdc);
}

// --------------------------------------------
// Thread di elaborazione del Filtro
// --------------------------------------------
static DWORD WINAPI _OdbcExecuteThread(LPVOID Dato)
{
	DWORD dwWait;
	S_SQLINFO * psOdbc;
	SQLRETURN sqlReturn;
	BOOL bError;
	INT idThread=GetCurrentThreadId();
	EH_ODBC_SECTION * _psOdbcSection=odbcSectionMain();

	psOdbc=(S_SQLINFO *) Dato;

	// 
	// Inizializzo la nuova sezione
	//
	psOdbc->psOdbcSection=odbcSectionStmtClone(_psOdbcSection,idThread); 

	//
	// Cose che mi servono per lo scroll
	//

	// Creo 
	//sqlReturn=SQLAllocHandle(SQL_HANDLE_STMT, _psOdbcSection->hConn, &psOdbc->psOdbcSection->hStmt);
	//if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO) ehExit("OdbcScroll:hStmt Clone impossible %d",sqlReturn);
	
	// Dico che sono in sola lettura
	SQLSetStmtAttr(psOdbc->psOdbcSection->hStmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_READ_ONLY, 0);
	// Dico che sullo stantment
	sqlReturn=SQLSetStmtAttr(psOdbc->psOdbcSection->hStmt, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0);
	if (sqlReturn==SQL_ERROR)  
	// Altro metodo
	{
		sqlReturn=SQLSetStmtAttr( psOdbc->psOdbcSection->hStmt,SQL_ATTR_CURSOR_TYPE,  (SQLPOINTER) SQL_CURSOR_STATIC, 0);
		if (sqlReturn==SQL_ERROR) {ehLogWrite("errore in assegnazione cursore"); return 1;}
		sqlReturn=SQLSetStmtAttr( psOdbc->psOdbcSection->hStmt, SQL_ATTR_USE_BOOKMARKS, (SQLPOINTER) SQL_UB_VARIABLE, 0);
		if (sqlReturn==SQL_ERROR) {ehLogWrite("SQL_ATTR_USE_BOOKMARKS"); return 2;}
	}


	// ATTENZIONE:
	// Bisognerebbe sincronizzare l'accesso alla struttura Sqd
	psOdbc->fInProcess=FALSE;
	psOdbc->fInStop=FALSE;

	while (TRUE)
	{
		psOdbc->fInProcess=FALSE; psOdbc->fBreak=FALSE;
		dwWait=WaitForMultipleObjects(ESQL_MAXEVENTLOAD,psOdbc->hEventSql,FALSE,INFINITE);

		// Chiusura del Thread
		if (dwWait==WAIT_OBJECT_0+ESQL_STOP) break;

	 // --------------------------------------------------------------------------------------------------------
	 // 
	 // Richiesta di Esecuzione del comando SQL di query
	 // 
	 // --------------------------------------------------------------------------------------------------------
	 if (dwWait==WAIT_OBJECT_0+ESQL_EXECUTE) 
	 {
		ResetEvent(psOdbc->hEventSql[ESQL_EXECUTE]); if (psOdbc->fBreak) continue;

		//
		// A) Trasformo la query Select nelle due Active
		//
		EnterCriticalSection(&psOdbc->csSqdStruct);
		if (!psOdbc->pQuerySelect)
		{
			ehFreePtr(&psOdbc->pQueryActive);
			ehFreePtr(&psOdbc->pQueryCount);
		}
		else
		{
			BYTE *lp;
			EnterCriticalSection(&psOdbc->csSqdQuery);
			ehFreePtr(&psOdbc->pQueryActive);

			if (strstr(psOdbc->pQuerySelect,"{LIB}"))
			{
				INT iSize=strlen(psOdbc->pQuerySelect);
				psOdbc->pQueryActive=ehAlloc(iSize+1024); 
				if (!psOdbc->pQueryActive)
				{
					fprintf(stderr,"iSize:%d" CRLF,iSize);
					LeaveCriticalSection(&psOdbc->csSqdQuery);
					LeaveCriticalSection(&psOdbc->csSqdStruct);
					break;
				}
				//if (!psOdbc->pQueryActive) ehExit("qui");
				strcpy(psOdbc->pQueryActive,psOdbc->pQuerySelect);
				while (strReplace(psOdbc->pQueryActive,"{LIB}",psOdbc->psOdbcSection->pSqlSchema));
			}
			else
			{
				psOdbc->pQueryActive=strDup(psOdbc->pQuerySelect);
			}

			LeaveCriticalSection(&psOdbc->csSqdQuery);

		//
		// B) Trovo la query per contare
		//

			ehFreePtr(&psOdbc->pQueryCount);
			psOdbc->pQueryCount=ehAlloc(strlen(psOdbc->pQueryActive)+1024);
			lp=strstr(psOdbc->pQueryActive,"FROM");
			if (lp)
			{
				sprintf(psOdbc->pQueryCount,"SELECT COUNT(*) %s",lp);
				lp=strstr(psOdbc->pQueryCount,"ORDER"); if (lp) *lp=0;
			}
			else
			{
				fprintf(stderr,__FUNCTION__ "Query count error ? [%s]" CRLF,psOdbc->pQueryActive);
				ehFreePtr(&psOdbc->pQueryActive);
				ehFreePtr(&psOdbc->pQueryCount);
			}
		}
		LeaveCriticalSection(&psOdbc->csSqdStruct);

#ifdef OS_DEBUG
//		fprintf(stderr,"Query count: %s" CRLF,psOdbc->pQueryCount);
		fprintf(stderr," - Conto ... " ); // < ---#############
#endif

		psOdbc->fInProcess=TRUE;
		psOdbc->ws.offset=0;
		psOdbc->ws.maxcam=0;
		psOdbc->lRowsReady=0;
		psOdbc->lRowsTotal=0;
		psOdbc->fDataReady=FALSE;
		

		//
		// C) Query Count(*) Conto le righe possibili
		//

		bError=FALSE;
		_LMessage(psOdbc,TRUE,"Richiesta al dbase server\nAttendere ..."); 
		// Chiudo l'uso del cursore se è aperto
		if (psOdbc->fCursorOpen) {SQLCloseCursor(psOdbc->psOdbcSection->hStmt); psOdbc->fCursorOpen=FALSE;}
		if (psOdbc->pQueryCount)
		{
			SQLBindCol(psOdbc->psOdbcSection->hStmt, 1, SQL_INTEGER, &psOdbc->lRowsTotal, 0, 0);
			bError=_LSqlTry("ATTR3",psOdbc->psOdbcSection->hStmt,SQLSetStmtAttr(psOdbc->psOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0),NULL);
			if (!bError) bError=_LSqlTry("EXEC_COUNTER",psOdbc->psOdbcSection->hStmt,SQLExecDirect(psOdbc->psOdbcSection->hStmt,psOdbc->pQueryCount,SQL_NTS),psOdbc->pQueryCount);
			if (!bError) bError=_LSqlTry("FETCH_COUNTER",psOdbc->psOdbcSection->hStmt,SQLFetch(psOdbc->psOdbcSection->hStmt),psOdbc->pQueryCount);
			SQLCloseCursor(psOdbc->psOdbcSection->hStmt);
		}
		else
		{
			psOdbc->lRowsTotal=0;
		}

		if (bError) psOdbc->lRowsTotal=0;

#ifdef OS_DEBUG
		fprintf(stderr,"%d records." CRLF,psOdbc->lRowsTotal);
#endif

	//	_LMessage(psOdbc,TRUE,"Fatto."); 
		//
		// D) Nessun record disponibile !
		//
		if (!psOdbc->lRowsTotal) 
		{
			psOdbc->fInProcess=FALSE; 
			EnterCriticalSection(&psOdbc->csSqdStruct);
			psOdbc->fChanged++;
			LeaveCriticalSection(&psOdbc->csSqdStruct);
			_LMessage(psOdbc,FALSE,NULL);
			continue;
		}
		if (psOdbc->fBreak) continue;
		
		//
		// D) LEGGO LE RIGHE
		//

		// Se ho un buffer
		if (psOdbc->lRowsRequest)
		{

			EnterCriticalSection(&psOdbc->csSqdStruct);
			psOdbc->lRowsCurrent=0;
			if (psOdbc->fCursorOpen) {
				SQLCloseCursor(psOdbc->psOdbcSection->hStmt); 
				psOdbc->fCursorOpen=FALSE;
			}

			//
			// Query effettiva
			//

			_LMessage(psOdbc,TRUE,"Richiesta dati ..."); 
#ifdef OS_DEBUG
			fprintf(stderr," - Query ..." ,psOdbc->pQueryActive); // < ---#############
//			fprintf(stderr,"%s" CRLF,psOdbc->pQueryActive);
#endif
			_LSqlTry("ATTR5",psOdbc->psOdbcSection->hStmt,SQLSetStmtAttr(psOdbc->psOdbcSection->hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &psOdbc->lRowsCurrent, 0),NULL);
			sqlReturn=SQLExecDirect(psOdbc->psOdbcSection->hStmt, psOdbc->pQueryActive, SQL_NTS);

			//
			// Query riuscita ...
			//
			if (sqlReturn==SQL_SUCCESS||sqlReturn==SQL_SUCCESS_WITH_INFO)
			{
				if (psOdbc->rsSet) {odbc_QueryResultDestroy(psOdbc->rsSet); ehFree(psOdbc->rsSet);}
				psOdbc->rsSet=odbcQueryResultCreate(psOdbc->psOdbcSection,psOdbc->lRowsRequest);

#ifdef EH_MEMO_DEBUG
				memDebugUpdate(psOdbc->rsSet,__FILE__,__LINE__);	
#endif
				_LSqlTry("ATTR3",psOdbc->psOdbcSection->hStmt,SQLSetStmtAttr(psOdbc->psOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) psOdbc->rsSet->iRowsLimit, 0),NULL);
#ifdef OS_DEBUG
				fprintf(stderr,"iRowsLimit (buffer): %6d ... " CRLF,psOdbc->rsSet->iRowsLimit); 
				// fprintf(stderr," OK." CRLF);//  Lette %d / %d." CRLF,psOdbc->rsSet->iRowsLimit,psOdbc->lRowsCurrent);
#endif
				psOdbc->bSqlError=FALSE;
			}
			//
			// Query in ERRORE !
			//
			else
			{
				_LSqlTry("ATTR3",psOdbc->psOdbcSection->hStmt,SQLSetStmtAttr(psOdbc->psOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 0, 0),NULL);
				psOdbc->bSqlError=TRUE;
				psOdbc->pLastErrorText=strDup(psOdbc->pQueryActive);
				strCat(&psOdbc->pLastErrorText,"\n");
				strCat(&psOdbc->pLastErrorText,odbcDisplay(SQL_HANDLE_STMT,"Query",psOdbc->psOdbcSection->hStmt,sqlReturn));
				psOdbc->lRowsTotal=strCount(psOdbc->pLastErrorText,"\n");
#ifdef OS_DEBUG
				//psOdbc->lRowsCurrent=psOdbc->lRowsBuffer;
			//	fprintf(stderr,"Q: [%s]" CRLF,psOdbc->pQueryActive);
//				fprintf(stderr," Error !" CRLF,psOdbc->rsSet->iRowsLimit,psOdbc->lRowsCurrent);
				fprintf(stderr,"%s" CRLF,psOdbc->pLastErrorText);
#endif
			}

			//psOdbc->fChanged++;
			psOdbc->fCursorOpen=TRUE;
			LeaveCriticalSection(&psOdbc->csSqdStruct);
			
			// Richiesto uno stop della ricerca
			if (psOdbc->fBreak) continue;

			_LFetch(psOdbc); // Richiedo una fetch
			psOdbc->fInProcess=FALSE;
		}

	}

	 // A) Il Thread deve allocare la query in memoria in modo da rendere disponibili 
	 //    Le informazioni per lo scroll
	
	 // --------------------------------------------------------------------------------------------------------
	 // 
	 // FETCH : Richiesta di Esecuzione della Fetch
	 // 
	 // --------------------------------------------------------------------------------------------------------
	 if (dwWait==WAIT_OBJECT_0+ESQL_FETCH) 
		{
			INT iOffset;
			ResetEvent(psOdbc->hEventSql[ESQL_FETCH]); 

			// Richiesto uno stop della ricerca
			if (psOdbc->fBreak) continue;

			EnterCriticalSection(&psOdbc->csSqdStruct);
			psOdbc->fDataReady=FALSE;
			psOdbc->lRowsCurrent=0;
			iOffset=psOdbc->ws.offset;
			if (psOdbc->ws.offset<0) psOdbc->ws.offset=0;
			LeaveCriticalSection(&psOdbc->csSqdStruct);

#ifdef OS_DEBUG
			fprintf(stderr," - Fetch ... (%5d) " ,psOdbc->iSQLOffset); 
#endif
			if (iOffset<1) sqlReturn=SQLFetchScroll(psOdbc->psOdbcSection->hStmt,SQL_FETCH_FIRST,0); // <-- FETCH #############
						   else
						   sqlReturn=SQLFetchScroll(psOdbc->psOdbcSection->hStmt,SQL_FETCH_ABSOLUTE,iOffset+1);
			if (sqlReturn!=SQL_SUCCESS&&
				sqlReturn!=SQL_SUCCESS_WITH_INFO) 
			{
				fprintf(stderr," - Fetch error ... (%5d) !" CRLF,sqlReturn); 
			}
			EnterCriticalSection(&psOdbc->csSqdStruct);
			psOdbc->iSQLOffset=iOffset;
			psOdbc->lRowsReady=psOdbc->lRowsCurrent;
			psOdbc->fDataReady=TRUE;
			if (psOdbc->ws.offset==psOdbc->iSQLOffset) 
			{
				// InvalidateRect(psOdbc->ObjClient->hWnd,NULL,FALSE); 
				psOdbc->fChanged++; 
			}
			psOdbc->fCursorOpen=TRUE;
			psOdbc->fInProcess=FALSE;
			psOdbc->fInStop=TRUE; // Avviso che il thread è finito
			LeaveCriticalSection(&psOdbc->csSqdStruct);
			#ifdef OS_DEBUG
			fprintf(stderr,"= [%6d] ready!" CRLF,psOdbc->lRowsCurrent);
			#endif
		}
	}

	// Libero risorse
	odbc_free_result(psOdbc->rsSet); psOdbc->rsSet=NULL;
	psOdbc->psOdbcSection=NULL;
	odbcSectionDestroy(idThread);

//	odbcSectionDestroy(psOdbc->psOdbcSection);
	
	return 0;
}

// Chiedi di aggiornare eseguire un Fetch aggiornato
static void _LFetch(S_SQLINFO *psOdbc)
{
	psOdbc->fBreak=FALSE;
	SetEvent(psOdbc->hEventSql[ESQL_FETCH]); // Chiedo l'esecuzione del comando
}

// Chiedi di aggiornare eseguire un Fetch aggiornato
static void _LQueryExecute(S_SQLINFO *psOdbc,INT iLine)
{
//	if (!psOdbc->pQuerySelect) return;
	psOdbc->fBreak=FALSE;
	SetEvent(psOdbc->hEventSql[ESQL_EXECUTE]); // Chiedo l'esecuzione del comando
}

//
// Crea il KeyCode per riconoscere il record
//
static void _LKeyCodeBuilder(CHAR **pStr,S_SQLINFO *psOdbc)
{
	if (psOdbc->iKeyCodePart<1) ehError();

	ehFreePtr(pStr);
	if (psOdbc->iKeyCodePart==1) 
	{
		strAssign(pStr,sql_ptr(psOdbc->rsSet,psOdbc->pszKeyCode));
	}
	else
	{
		BYTE *pBuf=NULL;
		INT a,iSize=0;
		for (a=0;psOdbc->arKeyCodeFld[a];a++) {iSize+=strlen(sql_ptr(psOdbc->rsSet,psOdbc->arKeyCodeFld[a]))+1;}
		iSize+=10;
		pBuf=ehAlloc(iSize); *pBuf=0;
		for (a=0;psOdbc->arKeyCodeFld[a];a++) 
		{
			BYTE *pVal=sql_ptr(psOdbc->rsSet,psOdbc->arKeyCodeFld[a]);
			if (a) strcat(pBuf,EH_ODBC_FLD_SEPARATOR);
			strcat(pBuf,pVal);
		}
		strAssign(pStr,pBuf);
		ehFree(pBuf);
	}
}


// ----------------------------------------------
// PostStopThread
// Sistemazione/Traslazione variabili,handle,ecc...
// dopo la fine dle thread di processo
// ----------------------------------------------
static void PostStopThread(struct WS_INFO *ws,S_SQLINFO *Sdb)
{
	// Libero la memoria corrente della lista del dbase
	Sdb->fInStop=FALSE;
/*
	// Libero la memoria usata dal dbase reale (se c'è)
	if (ADB_info[Sdb->Hdb].AdbFilter->HdlRecord!=-1)
		{
			memoFree(ADB_info[Sdb->Hdb].AdbFilter->HdlRecord,"*HDBRec32");
		}

	// Copio i dati della nuova lista nel dbase
	ADB_info[Sdb->Hdb].AdbFilter->NumRecord=ADB_info[Sdb->HdbFind].AdbFilter->NumRecord;
	ADB_info[Sdb->Hdb].AdbFilter->MaxRecord=ADB_info[Sdb->HdbFind].AdbFilter->MaxRecord;
	ADB_info[Sdb->Hdb].AdbFilter->HdlRecord=ADB_info[Sdb->HdbFind].AdbFilter->HdlRecord;
	ADB_info[Sdb->Hdb].AdbFilter->IndexInUse=ADB_info[Sdb->HdbFind].AdbFilter->IndexInUse;
	
	// Scrivo nel dbase virtuale che la memoria non è stata usata
	// cosi l'adb_close() non la libera
	ADB_info[Sdb->HdbFind].AdbFilter->HdlRecord=-1;

	// Aggiorno il ws->maxcam con i campi alla fine letti
	ws->maxcam=ADB_info[Sdb->Hdb].AdbFilter->NumRecord;
*/

	// CAMBIO MODO: Torno al modo non in thread-process
	Sdb->fInProcess=FALSE;
	
	// Aggiusto il range della barra	
	OBJ_RangeAdjust(Sdb->ObjClient,ws->offset,ws->maxcam,ws->numcam);
	
	// Se ho una finestra collegata faccio un refresh dell'oggetto
	if (Sdb->ObjClient->hWnd) 
	{
		InvalidateRect(Sdb->ObjClient->hWnd,NULL,FALSE);
		UpdateWindow(Sdb->ObjClient->hWnd);
	}
		
	// Distruggo la progress bar alla base della finestra
	//DestroyWindow(Sdb->hProgress); Sdb->hProgress=NULL;
	//DestroyWindow(Sdb->hButtonStop); Sdb->hButtonStop=NULL;
	obj_IDUnlock(Sdb->ObjClient->nome);

	// Chiudo l'handle del thread
	//if (Sdb->hThread) {CloseHandle(Sdb->hThread); Sdb->hThread=NULL;}
	
	// Chiudo il dbase clone
	//adb_close(Sdb->HdbFind);
	
	// Comunico che ho cambiato l'indice
	/*
	if (Sdb->iIndex!=ADB_info[Sdb->Hdb].AdbFilter->IndexInUse)
	{
		OdbcScroll(WS_IDX,ADB_info[Sdb->Hdb].AdbFilter->IndexInUse,"");
	}
*/
}

// FTIME
static void _FTSqlThreadRefresh(INT enMess,void *dao)
{
//	INT b;
    static INT Count=0;
    static INT iCount=0;
	BOOL fNoBlank;
	S_SQLINFO * psOdbc;

	if (enMess==0)
	{
	 //for (b=0;b<SQDMAX;b++)
	for (lstLoop(_p.lstOdbc,psOdbc))
	 {
		if (psOdbc->ObjClient!=NULL)
			{
				struct WS_INFO * ws=&psOdbc->ws;	

				//
				// Se servono più righe chiedo di ricaricarle
				//
				if (psOdbc->ws.numcam!=psOdbc->lRowsRequest) 
				{
					psOdbc->lRowsRequest=psOdbc->ws.numcam;
					_LQueryExecute(psOdbc,__LINE__);
					psOdbc->fQueryToExecute=FALSE;
					continue;
				}

				//
				// Cambio di Query: Richiedo Execute
				//
				if (psOdbc->fQueryToExecute)
				{
					psOdbc->fQueryToExecute=FALSE;
					_LQueryExecute(psOdbc,__LINE__);
					continue;
				}
				
				// Se il thread stà girando
				//if (SQD[b].fInProcess)	
				{
//				 if (ws->maxcam!=(INT) SQD[b].lRowsTotal) // Ho record diversi ?
				 if (psOdbc->fChanged) // Qualcosa è cambiato
				 {
					LONG OldMax=ws->maxcam; 

					if (psOdbc->ObjClient->hWnd) 
					{
						 //ehPrintd("Refresh ..." CRLF);
						 ws->maxcam=psOdbc->lRowsTotal;
						 fNoBlank=ws->fNoBlankLine;
						 if (OldMax>ws->numcam) ws->fNoBlankLine=TRUE;
						 InvalidateRect(psOdbc->ObjClient->hWnd,NULL,FALSE);
						 UpdateWindow(psOdbc->ObjClient->hWnd);
						 ws->fNoBlankLine=fNoBlank;
						 
						 OBJ_RangeAdjust(psOdbc->ObjClient,ws->offset,ws->maxcam,ws->numcam);
						 if (psOdbc->ObjClient->yTitle)
						 {
							 RECT rTitle;
							 rTitle.left=psOdbc->ObjClient->px+2;
							 rTitle.top=psOdbc->ObjClient->py+2;
							 rTitle.bottom=rTitle.top+psOdbc->ObjClient->yTitle-1;
							 rTitle.right=rTitle.left+psOdbc->ObjClient->col1-1;
							 InvalidateRect(WindowNow(),&rTitle,FALSE);
						 }

					   // Reset flag
					   EnterCriticalSection(&psOdbc->csSqdStruct);
					   psOdbc->fChanged--;
					   LeaveCriticalSection(&psOdbc->csSqdStruct);
					}
				 }
				} // Thread

				// Controllo se l'offset è cambiato
				if (!Count) 
				{
					if (psOdbc->iSQLOffset!=ws->offset&&ws->offset>-1) //{SQLReload(SQD+b);}
					{
						_LFetch(psOdbc);
						psOdbc->iSQLOffset=ws->offset; // ## DANGER ##
					}
					//LeaveCriticalSection(&csSql);
				}
	
				// Richiesta di Stop/Thread 
				//if (SQD[b].fInStop) {PostStopThread(ws,&SQD[b]);} // ??
			}
	 }
	 Count++; Count%=4;
	}
}

// --------------------------------------------------
//  DB_GetLast                                      |
//  Setta il db nella posizione degli ultimi        |
//  record visualizzabili                           |
//                                                  |
//  Non ritorna nulla                               |
//                                                  |
// --------------------------------------------------
/*
static void DB_GetLast(S_SQLINFO *Sdb)
{
	INT a,test;
	LONG Hrec;

	// -----------------------------------------------
	//  Ricalcolo il puntatore partendo dal Basso    |
	//  e tornando indietro                          |
	// -----------------------------------------------

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
					test=(Sdb->funcNotify)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
					//if (test==OFF) break; // Fine della ricerca
					//if ((test==OFF)||(test==SEMIOFF)) goto rifo4; // Campo non valido ma continua
					if (test==ON) a++;
				 }
					else
				 {a++;test=ON;}

			 // Se il record ultimo non c'è me lo trovo
			 if (!Sdb->RecDN&&(test==ON)) {adb_position(Sdb->Hdb,&Sdb->RecDN);}
			 if (adb_find(Sdb->Hdb,Sdb->IndexNum,B_GET_PREVIOUS,"",B_RECORD)) break;
			}
}

*/
// --------------------------------------------------
//  DB_FIND Ricerca un determinato record           |
//                                                  |
//  Ritorna : 0 - Nessun spostamento                |
//            1 - Mandare in stampa                 |
//                                                  |
// --------------------------------------------------
static INT DB_find(INT enMess,LONG info,CHAR *str,S_SQLINFO *Sdb)
{
	/*
	float percent,perc2;
	INT test;
	HREC Hrec;
	struct WS_INFO *ws=&Sdb->ws;

	if (enMess==WS_FINDLAST)
		{
			HREC Hrec;
			if (!ADB_info[Sdb->Hdb].AdbFilter->NumRecord) return 0;
			if (Sdb->fInProcess) Hrec=ADB_info[Sdb->HdbFind].AdbFilter->lpList[ADB_info[Sdb->HdbFind].AdbFilter->NumRecord-1]; 
								 else 
								 memoRead(ADB_info[Sdb->Hdb].AdbFilter->HdlRecord,(ADB_info[Sdb->Hdb].AdbFilter->NumRecord-1)*sizeof(HREC),&Hrec,sizeof(HREC));
			adb_get(Sdb->Hdb,Hrec,Sdb->IndexNum);
		    goto after;
		}

	// Primo controllo
	if (((*str==0)||(ws->maxcam==0))&&(enMess==WS_FIND)) return 0;

	// ----------------------------------------------------------------
	// Chiedo alla funzione esterna quello che mi è stato chiesto     |
	// ----------------------------------------------------------------
	test=(Sdb->funcNotify)(enMess,0,info,str,Sdb->Hdb,Sdb->IndexNum);
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

         if (enMess!=WS_FINDLAST)
		 {
		  adb_position(Sdb->Hdb,&Hrec);
		  if (Sdb->fInProcess) // Thread in azione
		  {
		   for (l=0;l<ADB_info[Sdb->HdbFind].AdbFilter->NumRecord;l++)
			{
				//memoRead(ADB_info[Sdb->Hdb].AdbFilter->HdlRecord,l*sizeof(HREC),&Hrec2,sizeof(HREC));
				Hrec2=ADB_info[Sdb->HdbFind].AdbFilter->lpList[l]; 
			    if (Hrec==Hrec2) {Sdb->ws.selez=l;Check=TRUE;break;}
			}
		  }
		  else // Thread Fermo
		  {
		   for (l=0;l<ADB_info[Sdb->Hdb].AdbFilter->NumRecord;l++)
			{
				memoRead(ADB_info[Sdb->Hdb].AdbFilter->HdlRecord,l*sizeof(HREC),&Hrec2,sizeof(HREC));
				if (Hrec==Hrec2) {Sdb->ws.selez=l;Check=TRUE;break;}
			}
		  }
		  if (!Check) {Sdb->ws.selez=-1;return 0;}
		 } 
		 else 
		 {
			 if (Sdb->fInProcess) Sdb->ws.selez=ADB_info[Sdb->HdbFind].AdbFilter->NumRecord-1;
								  else
								  Sdb->ws.selez=ADB_info[Sdb->Hdb].AdbFilter->NumRecord-1;
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
		 if (!adb_recnum(Sdb->Hdb)) return -1;
	 }
//	printf("OK2"); getch();


	// Controlla che sia buono
	//if (test)

	if (Sdb->ExtFilter)
	{
	 test=(Sdb->funcNotify)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
	 //printf("test %d",test);
	 if (test!=ON) // Se non lo è setta la lista dall'inizio
					 {return 0;}
//						else
	}

	// Metto in ws->selez la selezione

	adb_position(Sdb->Hdb,&Hrec);
	ws->selez=Hrec;
	OdbcScroll(WS_SEL,Hrec,"");//Sdb->Hdb,Sdb->IndexNum);

	// Cerco la posizione in percentuale
	adb_findperc(Sdb->Hdb,Sdb->IndexNum,&percent);// Legge la posizione in percentuale
    //win_infoarg("%f %x [%s] ",percent,Hrec,adb_FldPtr(Sdb->Hdb,"CODICE"));
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
	Sdb->Dove=0; Sdb->End=(INT) ws->numcam;

#ifdef _WIN32
	if (ws->maxcam<=ws->Enumcam)
			 {//scr_adbExt1(WS_DBSEEK,ws->numcam*-1,"");}
				if (DB_seek(ws->Enumcam*-1,Sdb)) DB_buf(Sdb);
			 }
			 else
			 {INT a,righe;

				//scr_adbExt1(WS_DBSEEK,(ws->numcam/2)*-1,"");
				if (DB_seek((ws->Enumcam/2)*-1,Sdb)) DB_buf(Sdb);
				
				// Conto quante righe vuote ci sono
				righe=0;
				for (a=0;a<(INT) ws->Enumcam;a++)
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
			 {INT a,righe;

				//scr_adbExt1(WS_DBSEEK,(ws->numcam/2)*-1,"");
				if (DB_seek((ws->numcam/2)*-1,Sdb)) DB_buf(Sdb);
				
				// Conto quante righe vuote ci sono
				righe=0;
				for (a=0;a<(INT) ws->numcam;a++)
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
 */
 return -1;
}


/*
#define ROWS 20
#define STATUS_LEN 6
#define OPENDATE_LEN 11
#define DONE -1

SQLHENV         henv;
SQLHDBC         hdbc;
SQLHSTMT         hstmt1, hstmt2;
SQLRETURN         retcode;
SQLCHAR         szStatus[ROWS][STATUS_LEN], szOpenDate[ROWS][OPENDATE_LEN];
SQLCHAR         szNewStatus[STATUS_LEN], szNewOpenDate[OPENDATE_LEN];
SQLSMALLINT      sOrderID[ROWS], sNewOrderID[ROWS];
SQLINTEGER      cbStatus[ROWS], cbOrderID[ROWS], cbOpenDate[ROWS];
SQLUINTEGER      FetchOrientation, crow, FetchOffset, irowUpdt;
SQLUSMALLINT      RowStatusArray[ROWS];

SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, SQL_OV_ODBC3,0);
SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

// Specify that the ODBC Cursor Library is always used, then connect. 

SQLSetConnectAttr(hdbc, SQL_ATTR_ODBC_CURSORS, SQL_CUR_USE_ODBC);
SQLConnect(hdbc, "SalesOrder", SQL_NTS,
       "JohnS", SQL_NTS,
       "Sesame", SQL_NTS);

if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

   // Allocate a statement handle for the result set and a statement 
   // handle for positioned update statements.         

   SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt1);
   SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt2);

   // Specify an updatable static cursor with 20 rows of data. Set   
   // the cursor name, execute the SELECT statement, and bind the   
   // rowset buffers to result set columns in column-wise fashion.   
   SQLSetStmtAttr(hstmt1, SQL_ATTR_CONCURRENCY, SQL_CONCUR_VALUES, 0);
   SQLSetStmtAttr(hstmt1, SQL_ATTR_CURSOR_TYPE, SQL_CURSOR_STATIC, 0);
   SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, ROWS, 0);
   SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_STATUS_PTR, RowStatusArray, 0);
   SQLSetStmtAttr(hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &crow, 0);
   SQLSetCursorName(hstmt1, "ORDERCURSOR", SQL_NTS);
   SQLExecDirect(hstmt1,
         "SELECT ORDERID, OPENDATE, STATUS FROM ORDERS ",
        SQL_NTS);
   SQLBindCol(hstmt1, 1, SQL_C_SSHORT, sOrderID, 0, cbName);
   SQLBindCol(hstmt1, 2, SQL_C_CHAR, szOpenDate, OPENDATE_LEN, cbOpenDate);
   SQLBindCol(hstmt1, 3, SQL_C_CHAR, szStatus, STATUS_LEN, cbStatus);

   // Fetch the first block of data and display it. Prompt the user  
   // for new data values. If the user supplies new values, update 
   // the rowset buffers, bind them to the parameters in the update  
   // statement, and execute a positioned update on another hstmt. 
   // Prompt the user for how to scroll. Fetch and redisplay data as 
   // needed.                    

   FetchOrientation = SQL_FETCH_FIRST;
   FetchOffset = 0;
   do {

      SQLFetchScroll(hstmt1, FetchOrientation, FetchOffset);
      DisplayRows(sOrderID, szOpenDate, szStatus, RowStatusArray);

      if (PromptUpdate(&irowUpdt,&sNewOrderID,szNewOpenDate,szNewStatus)==TRUE){
         sOrderID[irowUpdt] = sNewOrderID;
         cbOrderID[irowUpdt] = 0;
         strcpy(szOpenDate[irowUpdt], szNewOpenData);
         cbOpenDate[irowUpdt] = SQL_NTS;
         strcpy(szStatus[irowUpdt], szNewStatus);
         cbStatus[irowUpdt] = SQL_NTS;
         SQLBindParameter(hstmt2, 1, SQL_PARAM_INPUT,
                          SQL_C_SSHORT, SQL_INTEGER, 0, 0,
                          &sOrderID[irowUpdt], 0, &cbOrderID[irowUpdt]);
         SQLBindParameter(hstmt2, 2, SQL_PARAM_INPUT,
                          SQL_C_CHAR, SQL_TYPE_DATE, OPENDATE_LEN, 0,
                          szOpenDate[irowUpdt], OPENDATE_LEN, &cbOpenDate[irowUpdt]);
         SQLBindParameter(hstmt2, 3, SQL_PARAM_INPUT,
                          SQL_C_CHAR, SQL_CHAR, STATUS_LEN, 0,
                          szStatus[irowUpdt], STATUS_LEN, &cbStatus[irowUpdt]);
         SQLExecDirect(hstmt2,
               "UPDATE EMPLOYEE SET ORDERID = ?, OPENDATE = ?, STATUS = ?"
               "WHERE CURRENT OF EMPCURSOR",
               SQL_NTS);
      }

   while (PromptScroll(&FetchOrientation, &FetchOffset) != DONE)
}

*/

/*
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[IC0EXPO]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[IC0EXPO]
GO

CREATE TABLE [dbo].[IC0EXPO] (
	[EXP_CLI] [varchar] (4) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_DMI] [datetime] NOT NULL ,
	[EXP_HMI] [datetime] NOT NULL ,
	[EXP_TIP] [char] (1) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_SPO] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_DOC] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_CON] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_MOT] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_MOD] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_MOP] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_007] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_008] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_009] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_010] [varchar] (7) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_FLG] [char] (1) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[EXP_CDI] [varchar] (8) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO
*/

#else
#error Easyhand error. E' richiesto EH_ODBC
#endif