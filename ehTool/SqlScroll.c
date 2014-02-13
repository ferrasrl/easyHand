//   
//   sqlScroll
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
#define ESQL_COUNT 1
#define ESQL_FETCH 2

#define WB_CLONE 0
#define WB_ORIGINAL 1


//
// Adattatore di chiamata alla funzione EH_NOTIFYPARAMS
//
#define _adaptor(obj,mess,info,psExt,rsSet) obj, (BOOL *) psExt,EXT_CALL,NULL,mess,info,(LONG) rsSet 

typedef struct {

	struct OBJ *ObjClient;
	LRESULT (*funcNotify)(EH_NOTIFYPARAMS);//struct OBJ *ObjClient,INT cmd,LONG  dato,void *str,EH_ODBC_RS rsSet);
	struct WS_INFO ws;

	EN_SQL_PLATFORM		enPlatform; // 0=non definita
	void *	rsSet;		// ResultSet 
	DWORD	dwTotalRows; // Numero totali di linee presenti con questa Query

	//
	// ResultSet (copia in memoria della fetch raccolta dalla chiamata
	//
	void **  arResultSet;
	INT		iResultSize;	// Grandezza di spazio disponibile per memorizzare le righe
	INT		iResultRows;	// Righe effettivamente presenti
	INT		iResultFields;	// Campi presenti per riga
	INT		iCurrent;		// Riga corrente (gestione interna)


	LONG    lRowsRequest;	// Righe richieste del nuovo buffer
//	LONG	lRowsBuffer;	// Righe possibili nel buffer
//	LONG	lRowsReady; // Righe lette dopo il Fetch
//	DWORD	dwRowsReady;	// Righe lette e pronte in memoria dopo la query
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
	BOOL	fBreak; // Chiedo l'interruzione della ricerca
	BOOL	fDataReady; // T/F quando i dati sono pronti


	BOOL	fQueryToExecute; // T/F se la query è da eseguire

	CHAR *	pszQuerySelect;	// query (select) richiesta dall'utente
	CHAR *	pszQueryActive;	// query attualmente selezionata
	CHAR *	pszQueryCount;	// Comando SELECT COUNT(*) per contare i record
	CHAR *	pszQueryLast;	// Ultima query richiesta al SQL
	CHAR *  pszLastErrorText;// Ultimo errore sql testuale
//	BOOL	fCursorOpen;	// T/F se aperto
	
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
static void _reqExecute(S_SCR_INFO *psSI,INT iLine);
static void _reqFetch(S_SCR_INFO *psSI);
static void _message(S_SCR_INFO *psSI,BOOL bClear,CHAR *lpMessage);
static CHAR * _keyCodeBuilder(S_SCR_INFO *psSI,INT iRow);
static void _sqlSetCurrentRow(S_SCR_INFO *psSI,INT iRow);
static DWORD WINAPI _sqlExecuteThread(LPVOID Dato);

static void _queryFetchLoad(S_SCR_INFO *psSI);
static void _resultSetCreate(S_SCR_INFO * psSI,INT iRows,INT iFields);
static void _resultSetDestroy(S_SCR_INFO * psSI);


//
// sqlScroll() - Scroll SQL 
//
void *	sqlScroll(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,CHAR *str)
{
	static struct WINSCR rit,*PtScr;
	EH_DISPEXT *psExt;
	
    S_SCR_INFO * psSI,sSqlInfo;
	INT   a;//,b;

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
		//
		// Creazione oggetto
		//
		case WS_CREATE: //break;

			_(sSqlInfo);
			objCalled->pOther=lstPush(_local.lstScr,&sSqlInfo);
			psSI=objCalled->pOther;
			psSI->ObjClient=objCalled;
			psSI->ObjClient->bFreeze=true; // Blocco la gestione dell'oggetto
			psSI->bDrawLineDiv=true;

#ifdef EH_SQL_SQLITE
			psSI->enPlatform=_SQL_SQLITE;
#endif
#ifdef EH_SQL_MYSQL
			psSI->enPlatform=_SQL_MYSQL;
#endif

			//
			// Alla prima chiamata creo thread e "finestrame" necessario 
			// 

			//
			// Creo il thread necessario alle richieste
			//
			//
			memset(&psSI->csSqdStruct,0,sizeof(CRITICAL_SECTION));
			memset(&psSI->csSqdQuery,0,sizeof(CRITICAL_SECTION));
			InitializeCriticalSection(&psSI->csSqdStruct); 
			InitializeCriticalSection(&psSI->csSqdQuery); 

			for (a=0;a<ESQL_MAXEVENTLOAD;a++) psSI->arhEvent[a]=CreateEvent(NULL,true,FALSE,NULL); 
			psSI->hThread = CreateThread(NULL, 
										  0, 
										  _sqlExecuteThread, 
										  (LPDWORD) psSI,
										  0, 
										  &psSI->dwThread);
			psSI->bAutoRowSelect=true; // <-- 2010 - Inserito auto select in partenza
			SetThreadPriority(psSI->hThread,THREAD_PRIORITY_NORMAL);
			break;

		//
		// Distruzione oggetto
		//
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

			_resultSetDestroy(psSI);	
			//
			 // Libero la memoria usata per la Query
			//
			ehFreePtrs(4,&psSI->pszQuerySelect,&psSI->pszQueryActive,&psSI->pszQueryCount,&psSI->pszQueryLast);

			//
			// Libero la memoria usata per la WhereAdd
			//
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
			 
			 //
			 // Cancello pOther da tutto (compreso l'oggetto
			 //
			 memset(psSI,0,sizeof(S_SCR_INFO));
			{
				EH_LST_I * psi;
				psi=lstSearch(_local.lstScr,objCalled->pOther);
				lstRemoveI(_local.lstScr,psi);
			}
			 ehFreePtr(&objCalled->pOther);
			 break;


		//
		// Apro
		//

		case WS_OPEN:

		//
		//	Inizializzazione della finestra
		//	
//			if (info<3) {ehExit("Field ? " __FUNCTION__);}
			psSI->ws.numcam=info;// Assegna il numero di campi da visualizzare
			psSI->ws.selez=-1; 
			psSI->ws.maxcam=0;
			psSI->ObjClient->tipo=OW_SCR;
			psSI->dwTotalRows=0;	// Righe totali della query
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
				_reqExecute(psSI,__LINE__); 
				else
				_reqFetch(psSI); 
				
			return NULL; // Non serve stampare

		// -----------------------------------
		// WS_CLOSE IL DRIVER                  |
		// -----------------------------------
		case WS_CLOSE:
				sqlScroll(objCalled,WS_PROCESS,STOP,NULL); // Chiedo di fermare il processo in corso
				psSI->ObjClient->bFreeze=true;
				if (psSI->funcNotify) (psSI->funcNotify)(_adaptor(objCalled,WS_CLOSE,0,NULL,NULL));
				break;

		case WS_KEYPRESS:
				if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
				if (psSI->funcNotify) (psSI->funcNotify)(_adaptor(objCalled,cmd,info,str,NULL)); 
				break;

		case WS_EVENT:

				if (!psSI->rsSet) break;
//				psSI->rsSet->iCurrentRow=info;
//				psSI->rsSet->iOffset=psSI->iSQLOffset;
				if (psSI->funcNotify) 
				{
					BYTE *ptr;
					_sqlSetCurrentRow(psSI,info);
					ptr=(BYTE *) (psSI->funcNotify)(_adaptor(objCalled,cmd,info,str,psSI->rsSet));
					return ptr;
				}
				printf("qui");
				break;

		case WS_SEL: 
				if (!psSI->funcNotify) break;//ehExit(SdbDriver32 ":No ext/disp function");
				// if (!psSI->rsSet) break;
				// printf("%d",psSI->rsSet->iCurrentRow);
				//if (psSI->ws.selez==-1) break;
				break;

		case WS_PTREC : //			  			Restituisce pt alla chiave selezionata
		case WS_GET_SELECTED:

			_(rit);
			rit.record=-1;
			if (strEmpty(psSI->pszKeyCode)) {efx2(); return NULL;}

			//
			// Selezione in scroll
			//
			if (psSI->rsSet&&psSI->ws.selez>-1)
			{
				//psSI->rsSet->iCurrentRow=psSI->ws.selez;
				if (strEmpty(psSI->pszKeyCode)) ehExit("%s:%d manca assegnazione pszKeyCode",__FILE__,__LINE__);
//				_sqlSetCurrentRow(psSI,psSI->ws.selez);
				if (psSI->pCodeReturn) ehFree(psSI->pCodeReturn);
				psSI->pCodeReturn=_keyCodeBuilder(psSI,psSI->ws.selez);

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
printf ("[-- %s --]"CRLF,rit.keypt);
//			if (cmd==WS_GET_SELECTED) return retCreate(_ALFA,rit.record,rit.keypt);
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
	 		strAssign(&psSI->pszQuerySelect,str);
			psSI->ws.selez=-1;
			psSI->ws.maxcam=0;
//			psSI->dwRowsReady=0;
			_resultSetDestroy(psSI);
			psSI->dwTotalRows=0;
			LeaveCriticalSection(&psSI->csSqdStruct);
			if (info)
			{
				InvalidateRect(objCalled->hWnd,NULL,TRUE);
				_message(psSI,TRUE,"Attendere\nRichiesta al server ...");
				OsEventLoop(5);
			}
			psSI->fQueryToExecute=true;
			// _reqExecute(psSI,__LINE__); // Me ne arriva uno dopo
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

				if (!psSI->rsSet) 
				{
					if (psSI->bSqlError)
					{
						Tboxp(psExt->px,psExt->py,psExt->px+psExt->lx-1,psExt->py+psExt->ly-1,sys.Color3DShadow,SET);
						if (psSI->pszLastErrorText)
						{
							EH_AR arError;
							INT iRow;
							arError=ARCreate(psSI->pszLastErrorText,"\n",&iRow);
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

				
				_sqlSetCurrentRow(psSI,info);
//				psSI->rsSet->iCurrentRow=info;
//				psSI->rsSet->iOffset=psSI->iSQLOffset;

				// Non ho ancora le linee da visualizzare
				if (psExt->ncam>=psSI->iResultRows||
					(psSI->iCurrent<psSI->iSQLOffset)||
					((psSI->iCurrent-psSI->iSQLOffset)>=psSI->iResultRows)) 
				{
					boxBrush(psExt->px,psExt->py,psExt->rClientArea.right,psExt->rClientArea.bottom,HS_VERTICAL,sys.Color3DLight,ColorLum(sys.Color3DHighLight,-10));
					Tline(psExt->px,psExt->rClientArea.bottom,psExt->rClientArea.right,psExt->rClientArea.bottom,sys.Color3DShadow,SET);
					break;
				}

				//
				// Stampo la riga
				//
				psSI->funcNotify(	_adaptor(objCalled,WS_DISPLAY,info,psExt, psSI->rsSet));

				if (psSI->bDrawLineDiv) line(psExt->px,psExt->rClientArea.bottom,psExt->rClientArea.right,psExt->rClientArea.bottom,sys.colScrollDiv,SET);
				if (psExt->bFocus) 
				{
					boxFocus(psExt->px,psExt->py,psExt->px+psExt->lx-1,psExt->py+psExt->ly-2);
				}
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



//
// _queryBuilder() Creo la query e la query count
//
static void _queryBuilder(S_SCR_INFO *psSI) {

	CHAR * psz;

	EnterCriticalSection(&psSI->csSqdStruct);
	
	//
	// pszQueryActive
	//
	strAssign(&psSI->pszQueryActive,psSI->pszQuerySelect); 

	//
	// pszQueryCount
	//
	ehFreePtr(&psSI->pszQueryCount);
	psSI->pszQueryCount=ehAlloc(strlen(psSI->pszQueryActive)+1024);
	psz=strstr(psSI->pszQueryActive,"FROM");
	if (psz)
	{
		sprintf(psSI->pszQueryCount,"SELECT COUNT(*) %s",psz);
		psz=strstr(psSI->pszQueryCount,"ORDER"); if (psz) *psz=0;
	}
	else
	{
		printf(__FUNCTION__ "Query count error ? [%s]" CRLF,psSI->pszQueryActive);
		ehFreePtr(&psSI->pszQueryActive);
		ehFreePtr(&psSI->pszQueryCount);
	}
	LeaveCriticalSection(&psSI->csSqdStruct);

}

//
// _queryCounter() 
// Conta le righe massime della query richiesta
//
static void _queryCounter(S_SCR_INFO *psSI) {

	//
	// b. Conto le righe disponibili con la query
	//
	
	psSI->ws.offset=0;
	psSI->ws.maxcam=0;
//	psSI->dwRowsReady=0;
	_resultSetDestroy(psSI);
	psSI->dwTotalRows=0;
	psSI->fDataReady=FALSE;
	psSI->fInProcess=true;

	switch (psSI->enPlatform) {
	
	//
	// I. SQLITE
	//

#ifdef EH_SQL_SQLITE
		case _SQL_SQLITE:
			{
				BOOL bGroup=FALSE;
				EH_SQLITE_RS rsSet;

printf (CRLF"Inizio"CRLF);

				if (strstr(psSI->pszQueryCount,"GROUP"))
					bGroup=TRUE;

				sqlite_query(psSI->pszQueryCount);
				rsSet=sqlite_store();
				psSI->dwTotalRows=0;
				while (sqlite_fetch(rsSet))
				{
					if (bGroup)
						psSI->dwTotalRows++;
					else
						psSI->dwTotalRows=atoi(strEver(sqlite_ptr(rsSet,"1")));
				}
				sqlite_free(rsSet);
printf (CRLF"Righe Totali%d"CRLF,psSI->dwTotalRows);
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


	//
	// **************** SCONOSCIUTO O TECNOLOGIA NON GESTITA *********************
	//

		default:
				ehError();
				break;
	
	}

#ifdef OS_DEBUG
	printf("%d records." CRLF,psSI->dwTotalRows);
#endif
	psSI->fInProcess=false;
}

void _resultSetCreate(S_SCR_INFO * psSI,INT iRows,INT iFields) {
	psSI->iResultRows=0;
	psSI->iResultSize=iRows;
	psSI->iResultFields=iFields;
	psSI->arResultSet=ehAllocZero(sizeof(void *)*psSI->iResultSize); // Memoria per resultSet
}

//
// _resultSetDestroy()
//
void _resultSetDestroy(S_SCR_INFO * psSI) {
	
	INT iRow,iFld;

	if (!psSI->arResultSet) return;

	for (iRow=0;iRow<psSI->iResultSize;iRow++) {
		EH_AR ar=psSI->arResultSet[iRow];
		if (!ar) continue;
		for (iFld=0;iFld<psSI->iResultFields;iFld++) {
			ehFreeNN(ar[iFld]);
		}
		ehFreePtr(&psSI->arResultSet[iRow]);
	}
	ehFreePtr(psSI->arResultSet);
	psSI->arResultSet=NULL;
	psSI->iResultSize=0;
	psSI->iResultFields=0;

}

//
// _queryFetchLoad() 
// Leggo le righe della sezione interessata
// E le alloco in memoria
//
static void _queryFetchLoad(S_SCR_INFO * psSI) {

	CHAR * pszQuery;
	INT iRet;

	if (psSI->lRowsRequest&&psSI->pszQueryActive)
	{
		EnterCriticalSection(&psSI->csSqdStruct);
		psSI->fInProcess=true;
		psSI->lRowsCurrent=0;

		pszQuery=ehAlloc(strlen(psSI->pszQueryActive)+8000);
		strcpy(pszQuery,psSI->pszQueryActive);

		//
		// Richiedo i dati del blocco e li memorizzo in memoria
		//
		_message(psSI,TRUE,"Richiesta dati ..."); 
		switch (psSI->enPlatform) {

	#ifdef EH_SQL_SQLITE

			case _SQL_SQLITE:
				if (psSI->iSQLOffset<1) {
					strAppend(pszQuery," LIMIT %d",psSI->lRowsRequest);
				}
				else{
					strAppend(pszQuery," LIMIT %d OFFSET %d",psSI->lRowsRequest,psSI->iSQLOffset);
				}
				strAssign(&psSI->pszQueryLast,pszQuery);
	#ifdef OS_DEBUG
				printf(" - Query: %s ..." ,psSI->pszQueryLast); // < ---#############
	#endif
				iRet=sqlite_query(pszQuery);
				if (!iRet) 
				{
					EH_SQLITE_RS rsSet;
					rsSet=sqlite_store();

					_resultSetDestroy(psSI);
					_resultSetCreate(psSI,psSI->lRowsRequest,rsSet->iFields);

					while (true) {

						rsSet->psqSection->iLastError=sqlite3_step(rsSet->psStmt); 
						if (rsSet->psqSection->iLastError==SQLITE_DONE) break;

						// Carico in memoria la riga
						if (rsSet->psqSection->iLastError==SQLITE_ROW) {
							INT a; 
							EH_AR ar=ehAllocZero(sizeof(CHAR *)*psSI->iResultFields);
							psSI->arResultSet[psSI->iResultRows]=ar;
							for (a=0;a<psSI->iResultFields;a++) {
								  strAssign(&ar[a],(BYTE *) sqlite3_column_text(rsSet->psStmt, a));
							  }
							psSI->iResultRows++;		
						}

					}
					psSI->rsSet=rsSet;
				} else psSI->iResultRows=0;
				break;
	#endif

	#ifdef EH_SQL_MYSQL

			case _SQL_MYSQL:
				if (psSI->iSQLOffset<1) {
					strAppend(pszQuery," LIMIT %d",psSI->lRowsRequest);
				}
				else{
					strAppend(pszQuery," LIMIT %d OFFSET %d",psSI->lRowsRequest,psSI->iSQLOffset);
				}
				strAssign(psSI->pszLastQuery,&pszQuery);
				iRet=mys_query(pszQuery);
				if (!iRet) 
				{
					EH_SQLITE_RS rsSet;
					rsSet=sqlite_store();
					psSI->dwRowsReady=rsSet->iRows;
					psSI->rsSet=rsSet;
				} else psSI->dwRowsReady=0;
				break;
	#endif

			default:
				ehError();

		}
		ehFree(pszQuery);

		//
		// Tutto ok
		//
		if (!iRet) {

	#ifdef EH_MEMO_DEBUG
			memDebugUpdate(psSI->rsSet,__FILE__,__LINE__);	// Interno: cambio posizione di allocazione per memo debug
	#endif
			psSI->bSqlError=false;
		//
		// Gestione errore
		//
		} else {

			printf("da Vedere");
			psSI->bSqlError=true;
//			strAssign(&psSI->pszLastErrorText,psSI->pszQueryActive);
//			psSI->dwTotalRows=strCount(psSI->pszLastErrorText,"\n");
#ifdef OS_DEBUG
			printf("%s" CRLF,psSI->pszLastErrorText);
#endif
		}
		psSI->fInProcess=false;
//
//	   EnterCriticalSection(&psInfo->csSqdStruct);
	   psSI->fChanged++;
	   LeaveCriticalSection(&psSI->csSqdStruct);//		LeaveCriticalSection(&psSI->csSqdStruct);

	}
	InvalidateRect(psSI->ObjClient->hWnd,NULL,false);
				
}
			//	_fetch(psSI); // Richiedo una fetch


// --------------------------------------------
// _sqlExecuteThread() 
// Elaborazione della query
// --------------------------------------------
static DWORD WINAPI _sqlExecuteThread(LPVOID Dato)
{
	DWORD dwWait;
	S_SCR_INFO *	psSI;
	psSI=(S_SCR_INFO *) Dato;
	psSI->fInProcess=FALSE;

	while (TRUE)
	{
		psSI->fInProcess=FALSE; psSI->fBreak=FALSE;
		dwWait=WaitForMultipleObjects(ESQL_MAXEVENTLOAD,psSI->arhEvent,false,INFINITE);

		//
		// Chiusura del Thread
		//
		if (dwWait==WAIT_OBJECT_0+ESQL_STOP) break;

		// 
		// COUNT - Conteggio delle righe (verifica implicita della query)
		// 
		if (dwWait==WAIT_OBJECT_0+ESQL_COUNT) 
		{
			ResetEvent(psSI->arhEvent[ESQL_COUNT]); if (psSI->fBreak) continue;
			_queryBuilder(psSI); if (psSI->fBreak) continue;	// Costruisco le query
			_queryCounter(psSI); if (psSI->fBreak) continue;	// Conto rows

			//
			// Se, nessun record disponibile, finisco li ...
			//
			if (!psSI->dwTotalRows) 
			{
				psSI->fInProcess=FALSE; 
				EnterCriticalSection(&psSI->csSqdStruct);
				psSI->fChanged++;
				LeaveCriticalSection(&psSI->csSqdStruct);
				_message(psSI,FALSE,NULL);
				continue;
			}
			if (psSI->fBreak) continue;

			_queryFetchLoad(psSI);

			psSI->fInProcess=false;

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
			ResetEvent(psSI->arhEvent[ESQL_FETCH]); 
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
			_queryFetchLoad(psSI);

			EnterCriticalSection(&psSI->csSqdStruct);
			psSI->iSQLOffset=iOffset;
			//psSI->lRowsReady=psSI->lRowsCurrent;
			psSI->fDataReady=true;
			if (psSI->ws.offset==psSI->iSQLOffset) {
				psSI->fChanged++; 
			}
			psSI->fInProcess=false;
			psSI->fChanged++;
			LeaveCriticalSection(&psSI->csSqdStruct);
#ifdef OS_DEBUG
			printf("= [%6d] ready!" CRLF,psSI->lRowsCurrent);
#endif
		}

	}
	return 0;
}

// Chiedi di aggiornare eseguire un Fetch aggiornato
static void _reqFetch(S_SCR_INFO *psSI)
{
	psSI->fBreak=FALSE;
	SetEvent(psSI->arhEvent[ESQL_FETCH]); // Chiedo l'esecuzione del comando
}

// Chiedi di aggiornare eseguire un Fetch aggiornato
static void _reqExecute(S_SCR_INFO *psSI,INT iLine)
{
	psSI->fBreak=FALSE;
	SetEvent(psSI->arhEvent[ESQL_COUNT]); // Chiedo l'esecuzione del comando
}

//
// _sql_ptr()
//
static CHAR * _sql_ptr(S_SCR_INFO * psSI,CHAR * pszName) {

	switch (psSI->enPlatform) {
	
		case _SQL_SQLITE:
			return sqlite_ptr(psSI->rsSet,pszName);
			break;

#ifdef EH_SQL_MYSQL
		case _SQL_MYSQL:
			return mys_ptr(psSI->rsSet,pszName);
			break;
#endif

		default:
			ehError(); break;
	}
	return NULL;
}

//
// Crea il KeyCode per riconoscere il record
//
static CHAR *  _keyCodeBuilder(S_SCR_INFO * psSI,INT iRow)
{
	CHAR * pRet=NULL;
	if (psSI->iKeyCodePart<1) ehError();
	_sqlSetCurrentRow(psSI,iRow);

	if (psSI->iKeyCodePart==1) 
	{
		strAssign(&pRet,_sql_ptr(psSI,psSI->pszKeyCode));
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
			BYTE *pVal=_sql_ptr(psSI,psSI->arKeyCodeFld[a]);
			if (a) strcat(pBuf,EH_SQL_FLD_SEPARATOR);
			strcat(pBuf,pVal);
		}
		strAssign(&pRet,pBuf);
		ehFree(pBuf);
	}
	return pRet;
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
					_reqExecute(psInfo,__LINE__);
					psInfo->fQueryToExecute=FALSE;
					continue;
				}

				//
				// Cambio di Query: Richiedo Execute
				//
				if (psInfo->fQueryToExecute)
				{
					psInfo->fQueryToExecute=FALSE;
					_reqExecute(psInfo,__LINE__);
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
						 ws->maxcam=psInfo->dwTotalRows;
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
						_reqFetch(psInfo);
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



//
// _sqlSetCurrentRow() 
//  Setta la riga attualmente in lettura nel ResultSet
//
static void _sqlSetCurrentRow(S_SCR_INFO *psSI,INT iRow) {
	
	EH_SQLITE_RS rsSL;

	psSI->iCurrent=iRow;
	switch (psSI->enPlatform) {
	
		case _SQL_SQLITE:
			rsSL=psSI->rsSet;
			rsSL->iCurrent=iRow;
			rsSL->arRow=psSI->arResultSet[(iRow-psSI->iSQLOffset)%psSI->iResultRows];
			// rsSL->iOffset=psSI->iSQLOffset;
			break;

		default:
			ehError();
			break;

	}

}

