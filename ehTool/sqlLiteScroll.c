//   
//   sqlLiteScroll
//   Funzioni di appoggio per Zona Actve            
//
//   							by Ferrà srl 2008
//   

#include "/easyhand/inc/easyhand.h"
#ifdef _DEBUG
 #define OS_DEBUG 1
#endif

typedef enum {
	_SQL_UNKNOW,
	_SQL_SQLITE,
	_SQL_MYSQL
} EN_SQL_PLATFORM;


#define EH_SQL_FLD_SEPARATOR "\1"
// extern EH_ODBC_SECTION sOdbcSection;

// Numero massimo di scroll contemporaneamente gestiti = 5
// Numero massimo di linee per scroll = 30
// #define	 MAX_Y 30

#define SQDMAX 10
#define ESQL_MAXEVENTLOAD 3
#define ESQL_STOP  0
#define ESQL_EXECUTE 1
#define ESQL_FETCH 2

#define WB_CLONE 0
#define WB_ORIGINAL 1


// Adattatore di chiamata alkla funzione
#define _adaptor(obj,mess,info,psExt,rsSet) obj, (BOOL *) psExt,EXT_CALL,NULL,mess,info,(LONG) rsSet 

typedef struct {

	struct OBJ *ObjClient;
	LRESULT (*funcNotify)(EH_NOTIFYPARAMS);//struct OBJ *ObjClient,INT cmd,LONG  dato,void *str,EH_ODBC_RS rsSet);
	struct WS_INFO ws;

	EN_SQL_PLATFORM		enPlatform; // 0=non definita
	void *	rsSet;		// ResultSet 

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
	HANDLE	arhEvent[ESQL_MAXEVENTLOAD];
	HANDLE	hThread;
	DWORD	dwThread;
	BOOL	fInProcess; // Il di ricerca è in azione
	BOOL	fInStop;
	BOOL	fBreak; // Chiedo l'interruzione della ricerca
	BOOL	fDataReady; // T/F quando i dati sono pronti


	BOOL	fQueryToExecute; // T/F se la query è da eseguire

	CHAR	*pQuerySelect;	// La query di ricerca con il select
	CHAR	*pQueryActive;	// Puntatore alla Query Attiva al momento
	CHAR *	pszQueryCount;	// Comando SELECT COUNT(*) per contare i record
	CHAR	*pLastErrorText;// Ultimo errore sql testuale
	BOOL	fCursorOpen;	// T/F se aperto
	
	CHAR	*pszKeyCode;	// Campi che compongono la chiave univoca
	EH_AR	arKeyCodeFld;
	INT	iKeyCodePart;

	CHAR *	pCodeFocused;	// Valore del codice in focus
	CHAR *	pCodeSelected;	// Valore del codice in selected
	CHAR *	pCodeReturn;	// Valore del codice in code return (doppio click)
	INT		iSQLOffset; 
	BOOL	bAutoRowSelect;	// T/F autoselect (default TRUE)
	BOOL	bDrawLineDiv;	// T/F se devo disegnare la linea di separazione (default TRUE)

	CRITICAL_SECTION csSqdStruct; // Accesso ad area generale
	CRITICAL_SECTION csSqdQuery; // Accesso ad area generale

} S_SCR_INFO;

static struct {

	BOOL	bReset;
	EH_LST	lstScr;

} _local={true,NULL};

/*
static BOOL   _bFirstTime=TRUE;
static S_SCR_INFO arSqlInfo[SQDMAX];
*/

/*
static INT  DB_find(INT cmd,LONG info,CHAR *str,S_SCR_INFO *Sdb);
static BOOL _LSqlTry(CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult,BYTE *pQuery);
static INT _LBox(HDC hDC,INT x1,INT y1,INT x2,INT y2,LONG colPen,LONG colBg);
*/
static void _SqlThreadRefresh(INT cmd,void *dao);
static void _queryExecute(S_SCR_INFO *psSI,INT iLine);
static void _fetch(S_SCR_INFO *psSI);
static void _message(S_SCR_INFO *psSI,BOOL bClear,CHAR *lpMessage);
static CHAR * _keyCodeBuilder(S_SCR_INFO *psSI,INT iRow);
static DWORD WINAPI _sqlExecuteThread(LPVOID Dato);

//
// Driver Odbc
//
void *	sqlLiteScroll(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,CHAR *str)
{
	static struct WINSCR rit,*PtScr;
	EH_DISPEXT *psExt;
	
    S_SCR_INFO * psSI,sSqlInfo;
	INT   a;//,b;
//	INT   ptClient;
//	SQLRETURN sqlReturn;

	//
	// Inizializzazione
	//
	if (_local.bReset)
	{
		_local.lstScr=lstCreate(sizeof(S_SCR_INFO));
		 FTIME_on(_SqlThreadRefresh,1);
		_local.bReset=false;
	}

	// Oggetto buono ?
	if ((objCalled->tipo!=OW_SCR)&&(objCalled->tipo!=OW_SCRDB)) return 0;

	psSI=objCalled->pOther;
	if (cmd==WS_INF) return &psSI->ws;

	switch (cmd) 
	{
	// -----------------------------------
	// APERTURA DEL DRIVER               |
	// -----------------------------------

		case WS_CREATE: //break;

#ifdef OS_DEBUG
			printf("OdbcScroll: Create" CRLF);
#endif

			_(sSqlInfo);
			objCalled->pOther=lstPush(_local.lstScr,&sSqlInfo);
			psSI=objCalled->pOther;
			psSI->ObjClient=objCalled;
			psSI->ObjClient->bFreeze=TRUE; // Blocco la gestione dell'oggetto
			psSI->bDrawLineDiv=TRUE;
#ifdef EH_SQL_SQLITE
			psSI->enPlatform=_SQL_SQLITE;
#endif
#ifdef EH_SQL_MYSQL
			psSI->enPlatform=_SQL_MYSQL;
#endif

			//
			// Alla prima chiamata creo thread e "finestrame" necessario 
			// 

			// Tecnologia sqlLite
			//		- La connessione deve essere inizializzata
			//
			if (!psSI->hThread)
			{
				memset(&psSI->csSqdStruct,0,sizeof(CRITICAL_SECTION));
				memset(&psSI->csSqdQuery,0,sizeof(CRITICAL_SECTION));
				InitializeCriticalSection(&psSI->csSqdStruct); 
				InitializeCriticalSection(&psSI->csSqdQuery); 
/*
			
				// 
				// Alloco lo stantment clone ( Si libererà con WS_DESTROY)
				//
				sqlReturn=SQLAllocHandle(SQL_HANDLE_STMT, sOdbcSection.hConn, &psSI->hStmtScroll);
				if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO) ehExit("OdbcScroll:hStmt Clone impossible %d",sqlReturn);

				// Bho ?
				SQLSetStmtAttr(psSI->hStmtScroll, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_READ_ONLY, 0);

				sqlReturn=SQLSetStmtAttr(psSI->hStmtScroll, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0);
				if (sqlReturn==SQL_ERROR)  
				// Altro metodo
				{
					sqlReturn=SQLSetStmtAttr( psSI->hStmtScroll,SQL_ATTR_CURSOR_TYPE,  (SQLPOINTER) SQL_CURSOR_STATIC, 0);
					if (sqlReturn==SQL_ERROR) win_infoarg("errore in assegnazione cursore");
					sqlReturn=SQLSetStmtAttr( psSI->hStmtScroll, SQL_ATTR_USE_BOOKMARKS, (SQLPOINTER) SQL_UB_VARIABLE, 0);
					if (sqlReturn==SQL_ERROR) win_infoarg("SQL_ATTR_USE_BOOKMARKS");
				}

				//sprintf(szCursor,"SQD%d",ptClient);
				//SQLTRY(SQL_HANDLE_STMT,"SQD->",psSI->hStmtScroll,SQLSetCursorName(psSI->hStmtScroll, szCursor, SQL_NTS));
				//SQLTRY("ASYNC",psSI->hStmtScroll,SQLSetStmtAttr(psSI->hStmtScroll, SQL_ATTR_ASYNC_ENABLE, (SQLPOINTER) SQL_ASYNC_ENABLE_ON , 0));


				//
				// 4) Creo il Thread (SQLExecuteThread) per l'elaborazione delle query
				//
				*/
				for (a=0;a<ESQL_MAXEVENTLOAD;a++) psSI->arhEvent[a]=CreateEvent(NULL,true,FALSE,NULL); 
				psSI->hThread = CreateThread(NULL, 
											  0, 
											  _sqlExecuteThread, 
											  (LPDWORD) psSI,
											  0, 
											  &psSI->dwThread);
				psSI->bAutoRowSelect=TRUE; // <-- 2010 - Inserito auto select in partenza
				SetThreadPriority(psSI->hThread,THREAD_PRIORITY_NORMAL);
			}
			break;

		case WS_OPEN:

		//
		//	Inizializzazione della finestra
		//	
			if (info<3) {ehExit("Field ? " __FUNCTION__);}
			psSI->ws.numcam=info;// Assegna il numero di campi da visualizzare
			psSI->ws.selez=-1; 
			psSI->ws.maxcam=0;
			psSI->ObjClient->tipo=OW_SCR;
			psSI->lRowsTotal=0;
			psSI->fChanged=0;
			psSI->ObjClient->bFreeze=FALSE;

			if (psSI->funcNotify) psSI->funcNotify(_adaptor(objCalled,WS_OPEN,0,NULL,NULL));//objCalled,NULL,0,NULL,WS_OPEN,NULL,NULL);
			psSI->ws.bExtSelection=TRUE; // Gestione esterna della selezione
			psSI->ws.fNoBlankLine=TRUE;
			

		case WS_LOAD:  
			break;

	// -----------------------------------
	// Richiesta di refresh
	// -----------------------------------
	case WS_RELOAD: 
			if (info)
				_queryExecute(psSI,__LINE__); 
				else
				_fetch(psSI); 
				
			return NULL; // Non serve stampare

	// -----------------------------------
	// WS_CLOSE IL DRIVER                  |
	// -----------------------------------
	case WS_CLOSE:
			sqlLiteScroll(objCalled,WS_PROCESS,STOP,NULL); // Chiedo di fermare il processo in corso
			psSI->ObjClient->bFreeze=true;
			if (psSI->funcNotify) (psSI->funcNotify)(_adaptor(objCalled,WS_CLOSE,0,NULL,NULL));
			break;

	// -----------------------------------
	// CHIUSURA DEFINITIVA DEL GESTORE (chiamato in obj_close());
	// -----------------------------------
	case WS_DESTROY: 

			//
			// Notifico la chiusura alla funzione esterna
			//
			if (psSI->funcNotify) (psSI->funcNotify)(_adaptor(objCalled,WS_DESTROY,0,NULL,NULL));

			// Fermo il Thread
			SetEvent(psSI->arhEvent[ESQL_STOP]); // Segnalo che siamo in chiusura
			if (WaitForSingleObject(psSI->hThread,5000)) 
			 {
				//_dx_(0,20,"Entro qui");
				//SQLFetchScroll(psSI->hStmtScroll,SQL_FETCH_FIRST,0);
				//SQLFreeHandle(SQL_HANDLE_STMT,psSI->hStmtScroll); psSI->hStmtScroll=0;
				//_dx_(0,40,"Terminate %d",TerminateThread(psSI->hThread,0));
			 }

			/*
			 if (psSI->hStmtScroll) {SQLFreeHandle(SQL_HANDLE_STMT,psSI->hStmtScroll); psSI->hStmtScroll=0;}
			 if (psSI->rsSet) {odbc_QueryResultDestroy(psSI->rsSet); ehFree(psSI->rsSet);}
			 psSI->rsSet=NULL;

			 // Libero la memoria usata per la Query
			 ehFreePtr(&psSI->pQueryActive);
			 ehFreePtr(&psSI->pszQueryCount);
			 ehFreePtr(&psSI->pLastErrorText);

			 // Libero la memoria usata per la WhereAdd
			 ehFreePtr(&psSI->pQuerySelect);

			 ehFreePtr(&psSI->pszKeyCode);
			 ARDestroy(psSI->arKeyCodeFld);
			 ehFreePtr(&psSI->pCodeFocused);
			 ehFreePtr(&psSI->pCodeSelected);
			 ehFreePtr(&psSI->pCodeReturn);

			 // Libero gli Handle degli eventi
			 for (a=0;a<ESQL_MAXEVENTLOAD;a++) CloseHandle(psSI->arhEvent[a]);

			 // 1.2.3. Liberi tutti !!
			 DeleteCriticalSection(&psSI->csSqdStruct); 
			 DeleteCriticalSection(&psSI->csSqdQuery); 
			 // Azzero la struttura di riferimento
			 memset(psSI,0,sizeof(S_SCR_INFO));
			 */
			{
				EH_LST_I * psi;
				psi=lstSearch(_local.lstScr,objCalled->pOther);
				lstRemoveI(_local.lstScr,psi);
			}
			 ehFreePtr(&objCalled->pOther);
			 break;

	// -----------------------------------
	// PRESSIONE DI UN TASTO/MOUSE       |
	// -----------------------------------
	case WS_KEYPRESS:
			if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
			if (psSI->funcNotify) (psSI->funcNotify)(_adaptor(objCalled,cmd,info,str,NULL)); 
			break;

	// -----------------------------------
	// SETTA SELEZIONE RECORD            |
	// -----------------------------------
	case WS_SEL: 
			if (!psSI->funcNotify) break;//ehExit(SdbDriver32 ":No ext/disp function");
			// if (!psSI->rsSet) break;
			// printf("%d",psSI->rsSet->iCurrentRow);
			//if (psSI->ws.selez==-1) break;
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

			_(rit);
			rit.record=-1;

			if (strEmpty(psSI->pszKeyCode)) {efx2(); return NULL;}
			/*
			//
			// Selezione in scroll
			//
			if (psSI->rsSet&&psSI->ws.selez>-1)
			{
				psSI->rsSet->iCurrentRow=psSI->ws.selez;
				if (strEmpty(psSI->pszKeyCode)) ehExit("%s:%d manca assegnazione pszKeyCode",__FILE__,__LINE__);
				_keyCodeBuilder(&psSI->pCodeReturn,psSI);

				rit.record=psSI->ws.selez;
				rit.keypt=psSI->pCodeReturn;
				if (psSI->bAutoRowSelect)
				{
					strAssign(&psSI->pCodeSelected,psSI->pCodeReturn);
					InvalidateRect(objCalled->hWnd,NULL,FALSE);
				}
			}
			//
			// Pre-selezione (non entrao in scroll
			//
			else if (!strEmpty(psSI->pCodeSelected)) {

				rit.record=psSI->ws.selez;
				rit.keypt=psSI->pCodeSelected;
			}
			if (cmd==WS_GET_SELECTED) return retCreate(_ALFA,rit.record,rit.keypt);
			*/
			return &rit;

	// -------------------------------------
	// RITORNA Selez ??????????            |
	// -------------------------------------
	case WS_REALGET:
			 break;

	// -------------------------------------
	// Refresh a ON                        |
	// -------------------------------------
	case WS_REFON : psSI->ws.refre=ON; break;
	case WS_REFOFF : psSI->ws.refre=OFF; break;

	case WS_PROCESS:
		if (info==STOP&&!str)
		 {
			// SQLCancel(psSI->hStmtScroll); // Cancello il processo nello stantment
			 printf("> cancellare processo in corso");
			EnterCriticalSection(&psSI->csSqdStruct);
			psSI->fBreak=true;
			LeaveCriticalSection(&psSI->csSqdStruct);
			while (TRUE) {if (!psSI->fInProcess) break; else Sleep(50);}
			//win_infoarg("SQL stop");
		}
		// Controllo se l'elaborazione è in corso
		if ((info==0)&&(*str=='?'))
		{
		  if (psSI->fInProcess) return "!"; else return NULL;
		}
		break;

	
	// 
	// Chiedo di cambiare il la Where di ricerca
	//
	case WS_SETFILTER:

		EnterCriticalSection(&psSI->csSqdStruct);
		ehFreePtr(&psSI->pQuerySelect); 
	 	psSI->pQuerySelect=strDup(str);
		psSI->ws.selez=-1;
		psSI->ws.maxcam=0;
		psSI->lRowsReady=0;
		psSI->lRowsTotal=0;
		LeaveCriticalSection(&psSI->csSqdStruct);
		if (info)
		{
			InvalidateRect(objCalled->hWnd,NULL,TRUE);
			_message(psSI,TRUE,"Attendere\nRichiesta al server ...");
			OsEventLoop(5);
		}
		psSI->fQueryToExecute=true;
		// _queryExecute(psSI,__LINE__); // Me ne arriva uno dopo
		break;

	case WS_SETFLAG:
		objCalled->pOtherEx=str; // Assegno un puntatore esterno
		if (psSI->funcNotify) (psSI->funcNotify)(_adaptor(objCalled,WS_SETFLAG,info,NULL,NULL)); 
		break;

	// -------------------------------------
	// Richiesta di Stampa dei Dati        |
	// -------------------------------------
	case WS_DISPLAY : //			  			Richiesta buffer
			 
			psExt=(EH_DISPEXT *) str;

			if (!psSI->funcNotify) {
				Tboxp(	psExt->rClientArea.left,
						psExt->rClientArea.top,
						psExt->rClientArea.right,
						psExt->rClientArea.bottom-1,
						RGB(255,128,0),SET);
//						arError=ARCreate(psSI->pLastErrorText,"\n",&iRow);
				dispf(psExt->px+1,psExt->py+1,RGB(255,255,255),-1,STYLE_NORMAL,"SMALL F",3,"-Not func for Display-");
				break;
			}

			//
			// Richiesta di stampa del titolo
			//
			if (psExt->ncam==-10) 
			{
				psSI->funcNotify(_adaptor(objCalled,WS_DISPLAY,0,psExt,NULL));//psSI->Hdb,psSI->iIndex); 
				break;
			}
/*
			if (!psSI->rsSet) 
			{
				if (psSI->bSqlError)
				{
					Tboxp(psExt->px,psExt->py,psExt->px+psExt->lx-1,psExt->py+psExt->ly-1,sys.Color3DShadow,SET);
					if (psSI->pLastErrorText)
					{
						EH_AR arError;
						INT iRow;
						arError=ARCreate(psSI->pLastErrorText,"\n",&iRow);
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

			psSI->rsSet->iCurrentRow=info;
			psSI->rsSet->iOffset=psSI->iSQLOffset;

			// Non ho ancora le linee da visualizzare
			if (psExt->ncam>=psSI->lRowsReady||
				(psSI->rsSet->iCurrentRow<psSI->iSQLOffset)||
				((psSI->rsSet->iCurrentRow-psSI->iSQLOffset)>=psSI->rsSet->iRowsLimit)) 
			{
				boxBrush(psExt->px,psExt->py,psExt->rClientArea.right,psExt->rClientArea.bottom,HS_VERTICAL,sys.Color3DLight,ColorLum(sys.Color3DHighLight,-10));
				Tline(psExt->px,psExt->rClientArea.bottom,psExt->rClientArea.right,psExt->rClientArea.bottom,sys.Color3DShadow,SET);
				break;
			}

			// RIchiedo stampa della riga
			psSI->funcNotify(	_adaptor(objCalled,WS_DISPLAY,info,psExt, psSI->rsSet));

			if (psSI->bDrawLineDiv) line(psExt->px,psExt->rClientArea.bottom,psExt->rClientArea.right,psExt->rClientArea.bottom,sys.colScrollDiv,SET);
			if (psExt->bFocus) 
			{
				boxFocus(psExt->px,psExt->py,psExt->px+psExt->lx-1,psExt->py+psExt->ly-2);
			}
			*/
			break;

	// --------------------------------------------------------------------------
	// SET della funzione esterna
	// --------------------------------------------------------------------------
	case WS_EXTNOTIFY: 
			 psSI->funcNotify=(LRESULT (*)(EH_NOTIFYPARAMS)) str;//(INT (*)(struct OBJ *,INT ,LONG  ,void  *str,EH_ODBC_RS )) str;
			 psSI->funcNotify(_adaptor(objCalled,WS_CREATE,0,NULL,NULL));
			 break;

	case WS_LINEVIEW: //	 
		psSI->ws.numcam=info;
		break;

	case WS_LINEEDIT: //	 
		psSI->ws.Enumcam=info;
		break;

	case WS_SIZE: break;
	case WS_MOVE: break;

	case WS_SETTITLE: break;
	case WS_CODENAME: 
		
		strAssign(&psSI->pszKeyCode,str); strTrim(psSI->pszKeyCode); 
		ARDestroy(psSI->arKeyCodeFld);
		psSI->arKeyCodeFld=ARCreate(psSI->pszKeyCode,"+",&psSI->iKeyCodePart);
		break;

	//
	// Riga in FOCUS
	//
		/*
	case WS_SET_ROWFOCUS: 
			if (!str) // Dal gestore oggetti
			{
				 if (info<0) {strAssign(&psSI->pCodeFocused,NULL); break;}
				 if (psSI->pCodeName&&psSI->rsSet)
				 {
					psSI->rsSet->iCurrentRow=info;
					psSI->rsSet->iOffset=psSI->ws.offset;

					// psSI->rsSet->iCurrentRow riga della query

					if (psSI->rsSet->iCurrentRow>=psSI->iSQLOffset&&
						psSI->rsSet->iCurrentRow<(psSI->iSQLOffset+psSI->lRowsReady))
					{
						strAssign(&psSI->pCodeFocused,sql_ptr(psSI->rsSet,psSI->pCodeName));
						strTrim(psSI->pCodeFocused);
					}
					else
					{
						strAssign(&psSI->pCodeFocused,NULL); // Non ho la selezione richiesta in memoria
					}
					dispxEx(0,80,"%d,%d = %s          ",
							psSI->rsSet->iCurrentRow,
							psSI->iSQLOffset+psSI->lRowsReady,
							psSI->pCodeFocused);
				 }
				return NULL;
			}
			else // Esterno
			{
				strAssign(&psSI->pCodeFocused,str);
				InvalidateRect(objCalled->hWnd,NULL,FALSE);
			}
			break;

	case WS_GET_ROWFOCUS: 
			 if (psSI->pCodeName&&psSI->pCodeFocused&&psSI->rsSet)
			 {
				CHAR *p;
				if (info<0) return NULL;
				psSI->rsSet->iCurrentRow=info;
				psSI->rsSet->iOffset=psSI->ws.offset;

				p=sql_ptr(psSI->rsSet,psSI->pCodeName); strTrimRight(p);
				if (!strcmp(p,psSI->pCodeFocused)) break;
			 }
			 return NULL;
*/

	//
	// Riga in SELECTED
	//
	case WS_SET_ROWSELECTED: 
			printf("qui");
		/*
			 if (!str) // Dal gestore oggetti
			 {
				 if (info<0) {strAssign(&psSI->pCodeSelected,NULL); InvalidateRect(objCalled->hWnd,NULL,FALSE); break;}
				 if (psSI->pszKeyCode&&psSI->rsSet)
				 {
					psSI->rsSet->iCurrentRow=info;
					psSI->rsSet->iOffset=psSI->ws.offset;
					_keyCodeBuilder(&psSI->pCodeSelected,psSI); //if (!strEmpty(p)) break;
				 }
				InvalidateRect(objCalled->hWnd,NULL,FALSE);
				return NULL;
			}
			else // Esterno
			{
				strAssign(&psSI->pCodeSelected,str);
				InvalidateRect(objCalled->hWnd,NULL,FALSE);
			}
			*/
			break;

	case WS_GET_ROWSELECTED: 

			//
			// Controllo di selezione
			// 
			if (psSI->pszKeyCode&&!strEmpty(psSI->pCodeSelected)&&psSI->rsSet)
			 {
				 CHAR * p;
				if (info<0) return NULL;

				//psSI->rsSet->iCurrent=info;
				p=_keyCodeBuilder(psSI,info); // Può ritornare Null in p se sto chiedendo una riga che non c'è (in fase di ingrandimento)
				if (!strCmp(p,psSI->pCodeSelected)) {ehFreeNN(p); break;}
				ehFreeNN(p); 
			 }
			 return NULL;

	case WS_EVENT:
		/*
			if (!psSI->rsSet) break;
			psSI->rsSet->iCurrentRow=info;
			psSI->rsSet->iOffset=psSI->iSQLOffset;
			if (psSI->funcNotify) 
			{
				BYTE *ptr=(BYTE *) (psSI->funcNotify)(_adaptor(objCalled,cmd,info,str,psSI->rsSet));
				return ptr;
			}
			*/
			printf("qui");
			break;

	case WS_BUF:
			break;

	default:
/*
		if (psSI->funcNotify) 
			(psSI->funcNotify)(_adaptor(objCalled,cmd,info,str,psSI->rsSet));
			else
			printf(__FUNCTION__ "Event ? [%d][%s]" CRLF,cmd,objCalled->nome); 
			*/
			break;
	}

	//PtScr=psSI->WinScr; 
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
/*
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
//			 printf("%s" CRLF,SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc)); 
			 printf(CRLF "*** ERROR ***" CRLF);
			 printf("Q: [%s]" CRLF,pQuery);
			 printf("%s" CRLF,OdbcDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
#endif
//			 SqlLogWrite("Q: [%s]",pQuery);
//			 SqlLogWrite("%s",OdbcDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
		 }
		 return TRUE;
      } 
   else return FALSE;
}
*/

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


static void _message(S_SCR_INFO *psSI,BOOL bClear,CHAR *lpMessage)
{
	HDC hdc;
	RECT rWin;
	SIZE sWin;
	HFONT hFont,hFontOld=NULL;
	RECT rText;
	SIZE sText;

	hdc=GetDC(psSI->ObjClient->hWnd);
	GetWindowRect(psSI->ObjClient->hWnd,&rWin);
	sWin.cx=rWin.right-rWin.left+1;
	sWin.cy=rWin.bottom-rWin.top+1;
	
	sText.cx=180; sText.cy=50; // Larghezza ed altezza del rettangolo
	
	rText.left=(sWin.cx-sText.cx)/2; rText.right=rText.left+sText.cx+1;
	rText.top=(sWin.cy-sText.cy)/2; rText.bottom=rText.top+sText.cy+1;

	if (bClear) {

		RECT rArea;
		_(rArea);
		rArea.right=sWin.cx-1; rArea.bottom=sWin.cy-1;
		dcBoxBrush(hdc,&rArea,HS_BDIAGONAL,sys.Color3DShadow,sys.Color3DLight);
		/*
		RECT rArea;
//		HBRUSH backBrush=CreateSolidBrush(sys.Color3DShadow);
		HBRUSH diagBrush=CreateHatchBrush(HS_BDIAGONAL,sys.Color3DLight);
		SetBkColor(hdc,sys.Color3DShadow);
		_(rArea);
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

		_(rcText);
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

	ReleaseDC(psSI->ObjClient->hWnd,hdc);
}

// --------------------------------------------
// _sqlExecuteThread() 
// Elaborazione della query
// --------------------------------------------
static DWORD WINAPI _sqlExecuteThread(LPVOID Dato)
{
	DWORD dwWait;
	S_SCR_INFO *	psSI;

	psSI=(S_SCR_INFO *) Dato;

	// ATTENZIONE:
	// Bisognerebbe sincronizzare l'accesso alla struttura Sqd
	psSI->fInProcess=FALSE;
	psSI->fInStop=FALSE;

	while (TRUE)
	{
		psSI->fInProcess=FALSE; psSI->fBreak=FALSE;
		dwWait=WaitForMultipleObjects(ESQL_MAXEVENTLOAD,psSI->arhEvent,false,INFINITE);

		// Chiusura del Thread
		if (dwWait==WAIT_OBJECT_0+ESQL_STOP) break;

		// --------------------------------------------------------------------------------------------------------
		// 
		// Richiesta di Esecuzione del comando SQL di query
		// 
		// --------------------------------------------------------------------------------------------------------
		if (dwWait==WAIT_OBJECT_0+ESQL_EXECUTE) 
		{
			CHAR * psz;
			ResetEvent(psSI->arhEvent[ESQL_EXECUTE]); if (psSI->fBreak) continue;

			//
			// a. Creo la query e la query count
			//
			EnterCriticalSection(&psSI->csSqdStruct);
			strAssign(&psSI->pQueryActive,psSI->pQuerySelect);

			ehFreePtr(&psSI->pszQueryCount);
			psSI->pszQueryCount=ehAlloc(strlen(psSI->pQueryActive)+1024);
			psz=strstr(psSI->pQueryActive,"FROM");
			if (psz)
			{
				sprintf(psSI->pszQueryCount,"SELECT COUNT(*) %s",psz);
				psz=strstr(psSI->pszQueryCount,"ORDER"); if (psz) *psz=0;
			}
			else
			{
				printf(__FUNCTION__ "Query count error ? [%s]" CRLF,psSI->pQueryActive);
				ehFreePtr(&psSI->pQueryActive);
				ehFreePtr(&psSI->pszQueryCount);
			}
			LeaveCriticalSection(&psSI->csSqdStruct);

			//
			// b. Conto le righe disponibili con la query
			//
			psSI->fInProcess=TRUE;
			psSI->ws.offset=0;
			psSI->ws.maxcam=0;
			psSI->lRowsReady=0;
			psSI->lRowsTotal=0;
			psSI->fDataReady=FALSE;

			switch (psSI->enPlatform) {
			
				//
				// I. SQLITE
				//

#ifdef EH_SQL_SQLITE
				case _SQL_SQLITE:
					{
						EH_SQLITE_RS rsSet;
						sqlite_query(psSI->pszQueryCount);
						rsSet=sqlite_store();
						psSI->lRowsTotal=0;
						while (sqlite_fetch(rsSet)) {
							psSI->lRowsTotal=atoi(strEver(sqlite_ptr(rsSet,"1")));
						}
						sqlite_free(rsSet);

					}
					break;
#endif

				//
				// II. MySql
				//
#ifdef EH_SQL_MYSQL
				case _SQL_MYSQL: // DA FARE
					ehError();
					break;
#endif
				default:
						ehError();
						break;
			
			}

	#ifdef OS_DEBUG
			printf("%d records." CRLF,psSI->lRowsTotal);
	#endif

			//
			// Se, nessun record disponibile, finisco
			//
			if (!psSI->lRowsTotal) 
			{
				psSI->fInProcess=FALSE; 
				EnterCriticalSection(&psSI->csSqdStruct);
				psSI->fChanged++;
				LeaveCriticalSection(&psSI->csSqdStruct);
				_message(psSI,FALSE,NULL);
				continue;
			}

			if (psSI->fBreak) continue;
			
			//
			// c. Leggo le righe
			//

			// Se ho un buffer
			if (psSI->lRowsRequest)
			{
				EnterCriticalSection(&psSI->csSqdStruct);
				psSI->lRowsCurrent=0;

				//
				// Query effettiva
				//
				_message(psSI,TRUE,"Richiesta dati ..."); 
	#ifdef OS_DEBUG
				printf(" - Query ..." ,psSI->pQueryActive); // < ---#############
	#endif

				switch (psSI->enPlatform) {
				
					//
					// I. SQLITE
					//

	#ifdef EH_SQL_SQLITE
					case _SQL_SQLITE:
							sqlite_query(psSI->pQueryActive);
							psSI->rsSet=sqlite_store();
							break;
	#endif

					//
					// II. MySql
					//
	#ifdef EH_SQL_MYSQL
					case _SQL_MYSQL: // DA FARE
						ehError();
						break;
	#endif
					default:
							ehError();
							break;
				
				}

				//
				// Query riuscita ...
				//
				if (psSI->rsSet)
				{
	#ifdef EH_MEMO_DEBUG
					memDebugUpdate(psSI->rsSet,__FILE__,__LINE__);	
	#endif
/*
	#ifdef OS_DEBUG
					printf("iRowsLimit (buffer): %6d ... " CRLF,psSI->rsSet->iRowsLimit); 
	#endif
	*/
					psSI->bSqlError=FALSE;
				}
				//
				// Query in ERRORE !
				//
				else
				{
					psSI->bSqlError=TRUE;
					psSI->pLastErrorText=strDup(psSI->pQueryActive);
					strCat(&psSI->pLastErrorText,"\n");
//					strCat(&psSI->pLastErrorText,OdbcDisplay(SQL_HANDLE_STMT,"Query",psSI->hStmtScroll,sqlReturn));
					psSI->lRowsTotal=strCount(psSI->pLastErrorText,"\n");
	#ifdef OS_DEBUG
					printf("%s" CRLF,psSI->pLastErrorText);
	#endif
				}

				psSI->fCursorOpen=true;
				LeaveCriticalSection(&psSI->csSqdStruct);
				
				// Richiesto uno stop della ricerca
				if (psSI->fBreak) continue;

			//	_fetch(psSI); // Richiedo una fetch
				psSI->fInProcess=FALSE;
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

			/*
								sqlGlobalBrowser.iSQLOffset=sqlGlobalBrowser.iNewoffset;
					if (iRealOffset<1) 
					{
						pQuery=ehAlloc(strlen(sqlGlobalBrowser.lpQueryActive)+100);
						sprintf(pQuery,"%s LIMIT %d ",sqlGlobalBrowser.lpQueryActive,iRealLimit);
					}
					else
					{
						pQuery=ehAlloc(strlen(sqlGlobalBrowser.lpQueryActive)+100);
						sprintf(pQuery,"%s LIMIT %d OFFSET %d",sqlGlobalBrowser.lpQueryActive,iRealLimit,iRealOffset);
					}
*/
			/*
			INT iOffset;
			ResetEvent(psSI->arhEvent[ESQL_FETCH]); 

			// Richiesto uno stop della ricerca
			if (psSI->fBreak) continue;

			EnterCriticalSection(&psSI->csSqdStruct);
			psSI->fDataReady=FALSE;
			psSI->lRowsCurrent=0;
			iOffset=psSI->ws.offset;
			if (psSI->ws.offset<0) psSI->ws.offset=0;
			LeaveCriticalSection(&psSI->csSqdStruct);

#ifdef OS_DEBUG
			printf(" - Fetch ... (%5d) " ,psSI->iSQLOffset); 
#endif
			if (iOffset<1) sqlReturn=SQLFetchScroll(psSI->hStmtScroll,SQL_FETCH_FIRST,0); // <-- FETCH #############
						   else
						   sqlReturn=SQLFetchScroll(psSI->hStmtScroll,SQL_FETCH_ABSOLUTE,iOffset+1);
			if (sqlReturn!=SQL_SUCCESS&&
				sqlReturn!=SQL_SUCCESS_WITH_INFO) 
			{
				printf(" - Fetch error ... (%5d) !" CRLF,sqlReturn); 
			}
			EnterCriticalSection(&psSI->csSqdStruct);
			psSI->iSQLOffset=iOffset;
			psSI->lRowsReady=psSI->lRowsCurrent;
			psSI->fDataReady=TRUE;
			if (psSI->ws.offset==psSI->iSQLOffset) 
			{
				// InvalidateRect(psSI->ObjClient->hWnd,NULL,FALSE); 
				psSI->fChanged++; 
			}
			psSI->fCursorOpen=TRUE;
			psSI->fInProcess=FALSE;
			psSI->fInStop=TRUE; // Avviso che il thread è finito
			LeaveCriticalSection(&psSI->csSqdStruct);
			#ifdef OS_DEBUG
			printf("= [%6d] ready!" CRLF,psSI->lRowsCurrent);
			#endif
			*/
		}

	}
	return 0;
}

// Chiedi di aggiornare eseguire un Fetch aggiornato
static void _fetch(S_SCR_INFO *psSI)
{
	psSI->fBreak=FALSE;
	SetEvent(psSI->arhEvent[ESQL_FETCH]); // Chiedo l'esecuzione del comando
}

// Chiedi di aggiornare eseguire un Fetch aggiornato
static void _queryExecute(S_SCR_INFO *psSI,INT iLine)
{
	psSI->fBreak=FALSE;
	SetEvent(psSI->arhEvent[ESQL_EXECUTE]); // Chiedo l'esecuzione del comando
}

//
// Crea il KeyCode per riconoscere il record
//
static CHAR *  _keyCodeBuilder(S_SCR_INFO * psSI,INT iRow)
{
	CHAR * pRet=NULL;
	if (psSI->iKeyCodePart<1) ehError();

	if (psSI->iKeyCodePart==1) 
	{
		strAssign(&pRet,sql_ptr(psSI->rsSet,psSI->pszKeyCode));
	}
	else
	{
		BYTE *pBuf=NULL;
		INT a,iSize=0;
		for (a=0;psSI->arKeyCodeFld[a];a++) {iSize+=strlen(sql_ptr(psSI->rsSet,psSI->arKeyCodeFld[a]))+1;}
		iSize+=10;
		pBuf=ehAlloc(iSize); *pBuf=0;
		for (a=0;psSI->arKeyCodeFld[a];a++) 
		{
			BYTE *pVal=sql_ptr(psSI->rsSet,psSI->arKeyCodeFld[a]);
			if (a) strcat(pBuf,EH_SQL_FLD_SEPARATOR);
			strcat(pBuf,pVal);
		}
		strAssign(&pRet,pBuf);
		ehFree(pBuf);
	}
	return pRet;
}


// ----------------------------------------------
// PostStopThread
// Sistemazione/Traslazione variabili,handle,ecc...
// dopo la fine dle thread di processo
// ----------------------------------------------
static void PostStopThread(struct WS_INFO *ws,S_SCR_INFO *Sdb)
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
static void _SqlThreadRefresh(INT cmd,void *dao)
{
    static INT Count=0;
    static INT iCount=0;
	BOOL fNoBlank;
	S_SCR_INFO * psInfo;

	if (cmd==0)
	{
	 //for (b=0;b<SQDMAX;b++)
		for (lstLoop(_local.lstScr,psInfo)) {
	 
				struct WS_INFO *ws;
				ws=&psInfo->ws;	

				//
				// Se servono più righe chiedo di ricaricarle
				//
				if (psInfo->ws.numcam!=psInfo->lRowsRequest) 
				{
					psInfo->lRowsRequest=psInfo->ws.numcam;
					_queryExecute(psInfo,__LINE__);
					psInfo->fQueryToExecute=FALSE;
					continue;
				}

				//
				// Cambio di Query: Richiedo Execute
				//
				if (psInfo->fQueryToExecute)
				{
					psInfo->fQueryToExecute=FALSE;
					_queryExecute(psInfo,__LINE__);
					continue;
				}
				
				// Se il thread stà girando
				//if (SQD[b].fInProcess)	
				{
//				 if (ws->maxcam!=(INT) SQD[b].lRowsTotal) // Ho record diversi ?
				 if (psInfo->fChanged) // Qualcosa è cambiato
				 {
					LONG OldMax=ws->maxcam; 

					if (psInfo->ObjClient->hWnd) 
					{
						 //ehPrintd("Refresh ..." CRLF);
						 ws->maxcam=psInfo->lRowsTotal;
						 fNoBlank=ws->fNoBlankLine;
						 if (OldMax>ws->numcam) ws->fNoBlankLine=TRUE;
						 InvalidateRect(psInfo->ObjClient->hWnd,NULL,FALSE);
						 UpdateWindow(psInfo->ObjClient->hWnd);
						 ws->fNoBlankLine=fNoBlank;
						 
						 OBJ_RangeAdjust(psInfo->ObjClient,ws->offset,ws->maxcam,ws->numcam);
						 if (psInfo->ObjClient->yTitle)
						 {
							 RECT rTitle;
							 rTitle.left=psInfo->ObjClient->px+2;
							 rTitle.top=psInfo->ObjClient->py+2;
							 rTitle.bottom=rTitle.top+psInfo->ObjClient->yTitle-1;
							 rTitle.right=rTitle.left+psInfo->ObjClient->col1-1;
							 InvalidateRect(WindowNow(),&rTitle,FALSE);
						 }

					   // Reset flag
					   EnterCriticalSection(&psInfo->csSqdStruct);
					   psInfo->fChanged--;
					   LeaveCriticalSection(&psInfo->csSqdStruct);
					}
				 }
				} // Thread

				// Controllo se l'offset è cambiato
				if (!Count) 
				{
					if (psInfo->iSQLOffset!=ws->offset&&ws->offset>-1) //{SQLReload(SQD+b);}
					{
						_fetch(psInfo);
						psInfo->iSQLOffset=ws->offset; // ## DANGER ##
					}
					//LeaveCriticalSection(&csSql);
				}
	
				// Richiesta di Stop/Thread 
				//if (SQD[b].fInStop) {PostStopThread(ws,&SQD[b]);} // ??
//			}
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
static void DB_GetLast(S_SCR_INFO *Sdb)
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
static INT DB_find(INT cmd,LONG info,CHAR *str,S_SCR_INFO *Sdb)
{
	/*
	float percent,perc2;
	INT test;
	HREC Hrec;
	struct WS_INFO *ws=&Sdb->ws;

	if (cmd==WS_FINDLAST)
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
	if (((*str==0)||(ws->maxcam==0))&&(cmd==WS_FIND)) return 0;

	// ----------------------------------------------------------------
	// Chiedo alla funzione esterna quello che mi è stato chiesto     |
	// ----------------------------------------------------------------
	test=(Sdb->funcNotify)(cmd,0,info,str,Sdb->Hdb,Sdb->IndexNum);
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

