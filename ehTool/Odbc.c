//   +-------------------------------------------+
//   | ODBC
//   | Interfaccia al dbase ODBC
//   |             
//   |							Ferrà srl 04/2008
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"

#ifndef EH_ODBC
#include "/easyhand/inc/eh_odbc.h"
#endif

#define SQL_BUFFER_QUERY_DEFAULT 8192

//
// Macro senza support MultiThread
//
static struct {
	
	BOOL	bReady;
	HANDLE  mtxSection;
	EH_LST	lstSection;
	EH_ODBC_SECTION * psSectionMain;

} _p={false};
#define _mutex_section_open_ WaitForSingleObject(_p.mtxSection,INFINITE);
#define _mutex_section_close_ {if (!ReleaseMutex(_p.mtxSection)) {ehPrintf("_memoMultiThread mutex error"); exit(128);}}


EH_LST odbcGetDiagnostic(SQLRETURN erErr,HANDLE hStmt,SQLSMALLINT iType,BOOL bShow);
// static HANDLE _mtxOdbc=NULL;
/*
#ifdef EH_ODBC_MT
 EH_ODBC_SECTION sOdbcSection={0,NULL,0};
#else
 static CRITICAL_SECTION csODBCSection;
#endif
*/
//
// _memoMultiThread() - Gestisce la sicronia tra thread nell'accesso in memoria
//
/*
static void _mutex(EN_MESSAGE enMess) {

    switch (enMess) {

        //
        // Creazione Gestione Multithread
        //
        case WS_CREATE:
			_mtxOdbc=CreateMutex(NULL,FALSE,NULL);
            break;

        //
        // Libera Risorse Gestione Multithread
        //
        case WS_DESTROY:
			CloseHandle(_mtxOdbc); _mtxOdbc=NULL;
            break;

        case WS_OPEN:
			WaitForSingleObject(_mtxOdbc,INFINITE);  // no time-out interval
            break;

        case WS_CLOSE:
			if (!ReleaseMutex(_mtxOdbc)) {ehPrintf("_memoMultiThread mutex error"); exit(128);}
            break;

        default:
            ehError();
    }

}
*/



//
// odbc_start()
//
void odbc_start(INT cmd)
{
	if (cmd!=WS_LAST) return;

	//_mutex(WS_CREATE);
	_(_p);
	// Creo gestione sezioni
	_p.mtxSection=CreateMutex(NULL,FALSE,NULL);
	_p.lstSection=lstCreate(sizeof(EH_ODBC_SECTION));
	_p.psSectionMain=odbcSectionCreate(0); // Sezione di default single thread
	//_p.psSectionMain=lstGet(_p.lstSection,0);
	/*
#ifndef EH_ODBC_MT
	odbc_CreateSection(&sOdbcSection);
#else
	_(arsOdbcSection);
	InitializeCriticalSection(&csODBCSection);
//	ODBC_thread_init();
#endif
*/
}
 
//
// odbc_end()
//
void odbc_end(INT iStep) {

	if (iStep!=WS_LAST) return;

	odbcSectionCloseAll();
//	CloseHandle(_p.mtxSection); _p.mtxSection=NULL;
	_p.lstSection=lstDestroy(_p.lstSection);

//	odbc_DestroySection(&sOdbcSection);
//	odbc_MTCloseAllSection();
//	DeleteCriticalSection(&csODBCSection);
// 	ODBC_library_end();
}	
/*

//
// odbc_CreateSection()
//
void odbc_CreateSection(EH_ODBC_SECTION *psOdbcSection)
{
	memset(psOdbcSection,0,sizeof(EH_ODBC_SECTION));
	psOdbcSection->lpQuery=ehAlloc(SQL_BUFFER_QUERY_DEFAULT);
	psOdbcSection->fDebug=FALSE;
	psOdbcSection->dwQueryCounter=0;

}

//
// odbc_DestroySection()
//
void odbc_DestroySection(EH_ODBC_SECTION *psOdbcSection)
{
	// QueryResultFree(psOdbcSection);
	if (psOdbcSection->hConn) 
	{	
		odbc_Disconnect(psOdbcSection);
	}
	ehFreePtr(&psOdbcSection->lpQuery);
	ehFreePtr(&psOdbcSection->pLastQuery);
	memset(psOdbcSection,0,sizeof(EH_ODBC_SECTION));
}
*/
#ifndef EH_CONSOLE
static void _OdbcWindow(CHAR *Testo)
{
	INT x;
	INT size;
	INT cBack=RGB(120,120,140);
	CHAR *lpFont="#Arial";
	INT iSize=24;
	RECT rcBox,rcGradient;
	INT yHeight;
	if (!Testo) {win_close(); return;}

	size=font_lenf(Testo,lpFont,iSize,STYLE_BOLD);
	x=(sys.video_x-(size+20))/2;
	yHeight=35;
	win_openEx(x,10,"",size+20,yHeight,cBack,0,0,WS_POPUP|WS_BORDER|WS_VISIBLE,TRUE,NULL);
	win_SizeLimits(AUTOMATIC,AUTOMATIC,-1,-1);
	// win_SizeLimits(AUTOMATIC,AUTOMATIC,-1,-1);

	rcBox.left=0; rcBox.top=0;	
	rcBox.right=size+20; rcBox.bottom=46;
	memcpy(&rcGradient,&rcBox,sizeof(RECT));
	rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;

//	dispfm(10,8,0,-1,STYLE_BOLD,lpFont,iSize,Testo);
	dispfm(10,6,15,-1,STYLE_BOLD,lpFont,iSize,Testo);

	// Gradiente Bianco
	rcBox.left=1; rcBox.top=1;	rcBox.right=size+20-2; rcBox.bottom=yHeight/2;
	memcpy(&rcGradient,&rcBox,sizeof(RECT));
	rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;
	BoxGradient(&rcBox,&rcGradient,0,AlphaRGB(40,255,255,255));//AlphaColor(255,RGB(80,0,0)));
}
#endif

//
// _odbcConnection()
//
static BOOL _odbcConnection(EH_ODBC_SECTION * psOdbcSection, HANDLE hConn, BOOL bShowWindow) {

	SQLRETURN sqlReturn;
	
	//
	// Nuova richiesta di connesione
	//
	if (!hConn) {

		// Alloca Ambiente ODBC
		psOdbcSection->sqlLastError=0;
		sqlReturn=SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &psOdbcSection->hEnv); 
		if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO) {
			osError(true,osGetError(),"_odbcConnection");
		}

		sqlReturn=SQLSetEnvAttr(psOdbcSection->hEnv, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0);
		if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO) ehError();

		// Alloco Connessione
		sqlReturn=SQLAllocHandle(SQL_HANDLE_DBC, psOdbcSection->hEnv, &psOdbcSection->hConn); 
		if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO) ehError();

		psOdbcSection->sqlLastError=SQLConnect(	psOdbcSection->hConn,				// Handle della connessione
												psOdbcSection->pSqlHost,		// Nome del server
												SQL_NTS,						// Nome del file / Driver da usare
												psOdbcSection->pSqlUser,SQL_NTS,  // UserName
												psOdbcSection->pSqlPass,SQL_NTS); // Password

		if (psOdbcSection->sqlLastError!=SQL_SUCCESS&&
			psOdbcSection->sqlLastError!=SQL_SUCCESS_WITH_INFO) 
			{
				 // if (!psOdbcSection->bNoErrorView) 
				odbcError(psOdbcSection, "Connect",0); 

	#if !defined(_NODC)&&!defined(EH_CONSOLE)
				if (bShowWindow)
				{
					_OdbcWindow(NULL);
				}
	#endif
				return true;
			}

		// win_infoarg("%d",psOdbcSection->sqlLastError);
	#if !defined(_NODC)&&!defined(EH_CONSOLE)
		if (bShowWindow) _OdbcWindow(NULL);
	#endif	

		//
		// Alloco Stantment
		//
		sqlReturn=SQLAllocHandle(SQL_HANDLE_STMT, psOdbcSection->hConn, &psOdbcSection->hStmt);
		// sqlReturn=SQLSetStmtAttr(psOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 10, 0); // Dimensione delle righe nel buffer
		if (psOdbcSection->sqlLastError!=SQL_SUCCESS&&psOdbcSection->sqlLastError!=SQL_SUCCESS_WITH_INFO) ehError();

	//  Leggo ASYNC
		sqlReturn=SQLGetInfo(psOdbcSection->hConn,SQL_ASYNC_MODE,(SQLPOINTER) &psOdbcSection->uiAsync,sizeof(psOdbcSection->uiAsync),0);
		sqlReturn=SQLGetInfo(psOdbcSection->hConn,SQL_SCROLL_OPTIONS,(SQLPOINTER) &psOdbcSection->uiScroll,sizeof(psOdbcSection->uiScroll),0);
		// printf("* SQL_SCROLL_OPTIONS: %d" CRLF,psOdbcSection->uiScroll);
	} 
	//
	// Clonazione 
	//
	else {

		//
		// Creo uno stantment sulla connessione in corso
		//
		psOdbcSection->hConn=hConn;
		sqlReturn=SQLAllocHandle(SQL_HANDLE_STMT, hConn, &psOdbcSection->hStmt);
		if (sqlReturn!=SQL_SUCCESS&&
			sqlReturn!=SQL_SUCCESS_WITH_INFO) {
				ehExit("OdbcScroll:hStmt Clone impossible %d",sqlReturn);
				psOdbcSection->hStmt=NULL;
		}

/*
		// Bho ?
		SQLSetStmtAttr(psOdbc->hStmtScroll, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_READ_ONLY, 0);

				sqlReturn=SQLSetStmtAttr(psOdbc->hStmtScroll, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0);
				if (sqlReturn==SQL_ERROR)  
				// Altro metodo
				{
					sqlReturn=SQLSetStmtAttr( psOdbc->hStmtScroll,SQL_ATTR_CURSOR_TYPE,  (SQLPOINTER) SQL_CURSOR_STATIC, 0);
					if (sqlReturn==SQL_ERROR) win_infoarg("errore in assegnazione cursore");
					sqlReturn=SQLSetStmtAttr( psOdbc->hStmtScroll, SQL_ATTR_USE_BOOKMARKS, (SQLPOINTER) SQL_UB_VARIABLE, 0);
					if (sqlReturn==SQL_ERROR) win_infoarg("SQL_ATTR_USE_BOOKMARKS");
				}
*/

	


	}

	return FALSE;
}

//
// odbc_Connect()
//
void * odbcConnect(EH_ODBC_SECTION * psOdbcSection,
				   CHAR * pSQLHostName,
				   CHAR *pSQLUser,
				   CHAR *pSQLPassword,
				   CHAR *pDbaseSchema,
				   ULONG client_flag,
				   BOOL bShowWindow)
{
	
	
#if !defined(_NODC)&&!defined(EH_CONSOLE)
	if (bShowWindow)
	{
		CHAR szServ[800];
		sprintf(szServ,"Connessione SQL a %s ...",pSQLHostName);
		_OdbcWindow(szServ);
	}
#endif

	psOdbcSection->pSqlHost=strDup(pSQLHostName);
	psOdbcSection->pSqlUser=strDup(pSQLUser);
	psOdbcSection->pSqlPass=strDup(pSQLPassword);
	psOdbcSection->pSqlSchema=strDup(pDbaseSchema);
	psOdbcSection->iSqlClientFlag=client_flag;

	// 
	// Mi connetto al server
	//
	if (_odbcConnection(psOdbcSection,NULL,bShowWindow)) 
		return NULL;

	return psOdbcSection->hConn;
}

//
// odbcDisconnect()
//
void odbcDisconnect(EH_ODBC_SECTION * psOdbcSection) {

	if (!psOdbcSection) return;
	if (!psOdbcSection->hConn) return;
	ehFreePtr(&psOdbcSection->pSqlHost);
	ehFreePtr(&psOdbcSection->pSqlUser);
	ehFreePtr(&psOdbcSection->pSqlPass);
	ehFreePtr(&psOdbcSection->pSqlSchema);
	if (psOdbcSection->hStmt)	{SQLFreeHandle(SQL_HANDLE_STMT,psOdbcSection->hStmt); psOdbcSection->hStmt=NULL;}
	if (psOdbcSection->hConn)	{SQLDisconnect(psOdbcSection->hConn); SQLFreeHandle(SQL_HANDLE_DBC,psOdbcSection->hConn);  psOdbcSection->hConn=NULL;}
	if (psOdbcSection->hEnv)	{SQLFreeHandle(SQL_HANDLE_ENV,psOdbcSection->hEnv); psOdbcSection->hEnv=NULL;}

}


//
// odbc_error()
//
/*
void odbc_error(EH_ODBC_SECTION * psOdbcSection) 
{s
	
	ehExit("%s\n%s",psOdbcSection->lpQuery,ODBC_error(psOdbcSection->mySql));
}
*/

// 
// odbc_error()
//
// 0 = Visualizza l'errore e scrive la console
// 1 = Visualizza solo l'errore
// 2 = Ritorna la tringa con l'errore
//
CHAR * odbcError(EH_ODBC_SECTION * psOdbcSection,CHAR *pWho,INT iMode) 
{
	
	CHAR *lpR="";
	SQLCHAR     szBuffer[SQL_MAX_MESSAGE_LENGTH + 1];
	SQLCHAR     sqlstate[SQL_SQLSTATE_SIZE + 1];
	SQLINTEGER  sqlcode;
	SQLSMALLINT length;
	CHAR *pReturn=NULL;

	switch (psOdbcSection->sqlLastError)
	{
		case SQL_SUCCESS: lpR="Success"; break;
		case SQL_SUCCESS_WITH_INFO: lpR="Success+Info"; break;
		case SQL_ERROR: lpR="Errore"; break;
		case SQL_INVALID_HANDLE: lpR="InvalidHandle"; break;
	}
  
	if (iMode==2) pReturn=ehAllocZero(128000);

	ehPrintd("Query: %s" CRLF,psOdbcSection->pLastQuery);
	while ( SQLError(psOdbcSection->hEnv, 
					 psOdbcSection->hConn, 
					 psOdbcSection->hStmt, 
					 sqlstate, 
					 &sqlcode, 
					 szBuffer,
                     SQL_MAX_MESSAGE_LENGTH + 1, 
					 &length) == SQL_SUCCESS )
    {
		if (!iMode) {
			ehPrintd("\n **** ERROR *****\n");
			ehPrintd("         SQLSTATE: %s\n", sqlstate);
			ehPrintd("Native Error Code: %ld\n", sqlcode);
			ehPrintd("%s \n", szBuffer);
		}

		else if (iMode<2)
		{
			win_infoarg("%s",szBuffer);
			ehLogWrite("SQLERROR:%s",szBuffer);
		}
		
		else if (iMode==2) 
		{
			if (*pReturn) strcat(pReturn,CRLF);
			strcat(pReturn,szBuffer);
		}
    };
	return pReturn;
}

/*
int print_error (SQLHENV    henv,
                    SQLHDBC    hdbc,
                    SQLHSTMT   hstmt)
{
SQLCHAR     buffer[SQL_MAX_MESSAGE_LENGTH + 1];
SQLCHAR     sqlstate[SQL_SQLSTATE_SIZE + 1];
SQLINTEGER  sqlcode;
SQLSMALLINT length;
 
 
    while ( SQLError(henv, hdbc, hstmt, sqlstate, &sqlcode, buffer,
                     SQL_MAX_MESSAGE_LENGTH + 1, &length) == SQL_SUCCESS )
    {
        printf("\n **** ERROR *****\n");
        printf("         SQLSTATE: %s\n", sqlstate);
        printf("Native Error Code: %ld\n", sqlcode);
        printf("%s \n", buffer);
    };
    return (0);
 
}
*/

/*
void OdbcTry(CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult) 
{
	
	SQLRETURN rc = iResult; 
	psOdbcSection->sqlLastError=iResult;
	if (rc != SQL_SUCCESS) 
      { 
		 if (rc != SQL_SUCCESS_WITH_INFO) 
		 {
			// ehExit(SQLDisplay(WhoIs,hstmt,rc));
#if (defined(_DEBUG)&&!defined(EH_CONSOLE))
			 dispx(OdbcDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc)); 
#endif
//			 LOGWrite(SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
			 //efx2(); efx2(); efx2(); 
			 //Sleep(2000);
		 }
      } 

}
*/
/*
const char *odbc_GetError(EH_ODBC_SECTION * psOdbcSection) 
{ 
	 
	return odbc_error(NULL);//psOdbcSection->mySql);
}
*/

//
// Conta i record secondo la query
//
INT odbcCount(EH_ODBC_SECTION * psOdbcSection,CHAR *Mess,...)
{
	
	EH_ODBC_RS pRes;
	INT iRecNum=-1;
	va_list Ah;
	CHAR *p,*lpQueryMess=ehAlloc(1024);
	CHAR *lpQueryCommand=ehAlloc(SQL_BUFFER_QUERY_DEFAULT);

	sprintf(lpQueryMess,"SELECT COUNT(*) FROM %s",Mess);
	va_start(Ah,Mess);
	vsprintf(lpQueryCommand,lpQueryMess,Ah); // Messaggio finale
	va_end(Ah);

	psOdbcSection->dwQueryCounter++;

/*
	//if (lpSqd->fCursorOpen) {SQLCloseCursor(lpSqd->hStmtScroll); lpSqd->fCursorOpen=FALSE;}
	iCount=0;
	SQLBindCol(psOdbcSection->hStmt, 1, SQL_INTEGER, &iCount, 0, 0);
	SQLSetStmtAttr(psOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0);
	SQLExecDirect(psOdbcSection->hStmt,lpQueryCommand,SQL_NTS);
	SQLFetch(psOdbcSection->hStmt);
	SQLCloseCursor(psOdbcSection->hStmt);
*/

    odbcQuery(psOdbcSection,lpQueryCommand);
//#ifdef EH_ODBC_MT
//	pRes=odbc_store_result(psOdbcSection,3); // Richiedo il risultato
//#else
	pRes=odbcStore(psOdbcSection,3); // Richiedo il risultato
//#endif
	if (pRes)
	{
		odbc_fetch_row(pRes);
		p=odbc_fldptr(pRes,"1"); if (p) iRecNum=atoi(p);
		odbc_free_result(pRes); // Libero le risorse
	}
	else
	{
		odbcError(psOdbcSection,__FUNCTION__,0);
		iRecNum=-1;
	}

/*
	if (psOdbcSection->fCursorOpen) {SQLCloseCursor(psOdbcSection->hStmt); psOdbcSection->fCursorOpen=FALSE;} // Apro il cursore
	SQLBindCol(psOdbcSection->hStmt, 1, SQL_INTEGER, &iRecNum, 0, 0); // Bind dell'informazione
	SQLSetStmtAttr(psOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0); // Numero di righe
	
	strcpy(psOdbcSection->lpQuery,lpQueryCommand);
	psOdbcSection->sqlLastError=SQLExecDirect(psOdbcSection->hStmt,lpQueryCommand,SQL_NTS); // Query
	if (psOdbcSection->sqlLastError<0) 
	{
		odbc_error(__FUNCTION__);
		iRecNum=-1;
	}
	else
	{
		SQLFetch(psOdbcSection->hStmt); // Leggo il Fatch
	}
	SQLCloseCursor(psOdbcSection->hStmt); // CHiudo il cursore
*/	

    ehFree(lpQueryCommand);
	ehFree(lpQueryMess); 
	return iRecNum;
}

//
// odbc_sum()
// Somma il valore di un campo
//
INT odbcSum(EH_ODBC_SECTION * psOdbcSection,CHAR *lpField,CHAR *Mess,...)
{
	
	/*
	INT iRecNum=-1;
	va_list Ah;
	CHAR *lpQueryMess=ehAlloc(1024);
	CHAR *lpQueryCommand=ehAlloc(SQL_BUFFER_QUERY_DEFAULT);

	sprintf(lpQueryMess,"SELECT SUM(%s) FROM %s",lpField,Mess);
	va_start(Ah,Mess);
	vsprintf(lpQueryCommand,lpQueryMess,Ah); // Messaggio finale
	va_end(Ah);

	psOdbcSection->dwQueryCounter++;

	if (psOdbcSection->fCursorOpen) {SQLCloseCursor(psOdbcSection->hStmt); psOdbcSection->fCursorOpen=FALSE;} // Apro il cursore
	SQLBindCol(psOdbcSection->hStmt, 1, SQL_INTEGER, &iRecNum, 0, 0); // Bind dell'informazione
	SQLSetStmtAttr(psOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0); // Numero di righe
	
	psOdbcSection->sqlLastError=SQLExecDirect(psOdbcSection->hStmt,lpQueryCommand,SQL_NTS); // Query
	if (psOdbcSection->sqlLastError<0) 
	{
		odbc_error(__FUNCTION__);
		iRecNum=-1;
	}
	else
	{
		SQLFetch(psOdbcSection->hStmt); // Leggo il Fatch
	}
	SQLCloseCursor(psOdbcSection->hStmt); // CHiudo il cursore

	ehFree(lpQueryCommand);
	ehFree(lpQueryMess); 
	return iRecNum;
	*/
	return 0;
}


//
// odbc_query()
// ritorna TRUE se c'è errore
//
int odbcQuery(EH_ODBC_SECTION * psOdbcSection,CHAR *pQuery)
{
	
	int err=0;
	BYTE *pRealQuery;
	BOOL bAlloc=FALSE;
	BOOL bNoErrorStop=FALSE;

//	_mutex(WS_OPEN);

	psOdbcSection->dwQueryCounter++;
	if (strEmpty(pQuery)) ehError();

	if (*pQuery=='?') {bNoErrorStop=true; pQuery++;}
	if ((strstr(pQuery,"{LIB}")||strstr(pQuery,"[LIB]"))&&
			psOdbcSection->pSqlSchema) // modifica per Crea importazione ehBook
	{
		pRealQuery=ehAlloc(strlen(pQuery)+1024); 
		strcpy(pRealQuery,pQuery);
		while (strReplace(pRealQuery,"{LIB}",psOdbcSection->pSqlSchema));
		while (strReplace(pRealQuery,"[LIB]",psOdbcSection->pSqlSchema));
		bAlloc=TRUE;
	}
	else
	{
		pRealQuery=pQuery;//strDup(pQuery);
	}

#ifdef EH_CONSOLE
	if (psOdbcSection->fDebug) {printf("%s" CRLF,pRealQuery);}
#else
	if (psOdbcSection->fDebug) {fprintf(stderr,"%s" CRLF,pRealQuery);}
#endif

	strAssign(&psOdbcSection->pLastQuery,pRealQuery);
	psOdbcSection->sqlLastError=SQLExecDirect(	psOdbcSection->hStmt,
												pRealQuery,
												SQL_NTS); // Query

	// SQL_ERROR= -1
	if (psOdbcSection->sqlLastError<0)  {

		//win_infoarg("qui: %d",	psOdbcSection->sqlLastError);
		if (!bNoErrorStop) {
			ehPrintd("%s" CRLF,pRealQuery);
			ehLogWrite("SQL query; %s",pRealQuery);
			odbcError(psOdbcSection,__FUNCTION__,1);
			ehExit("odbc_query() - Several error");
		}
		err=TRUE;
	}
	if (bAlloc) ehFree(pRealQuery);




//	_mutex(WS_CLOSE);

	return err;
}


//
// odbc_queryarg()
// ritorna TRUE se c'è errore
//
int odbcQueryf(EH_ODBC_SECTION * psOdbcSection,CHAR *Mess,...)
{
	
	INT iTest;
	va_list Ah;
	iTest=(INT) Mess;
	va_start(Ah,Mess);

	if (iTest==0||iTest==1) {
		BYTE *pStr=va_arg(Ah,BYTE *);
		ehExit("odbc_queryarg() invocato con TRUE/FALSE: query:[%s]",pStr);
	}
	
	vsprintf(psOdbcSection->lpQuery,Mess,Ah); // Messaggio finale
	va_end(Ah);

	if (strlen(psOdbcSection->lpQuery)>SQL_BUFFER_QUERY_DEFAULT) ehExit("odbc_queryarg() too big : %s",psOdbcSection->lpQuery);
	return odbcQuery(psOdbcSection,psOdbcSection->lpQuery);
}


//
// odbc_affected_rows()
//
int odbcAffectedRows(EH_ODBC_SECTION * psOdbcSection) 
{
	
	SQLINTEGER sqlInteger;
	SQLRowCount(psOdbcSection->hStmt,&sqlInteger);
	return sqlInteger;
}
/*
int odbc_queryargEx(EH_ODBC_SECTION * psOdbcSection,BOOL fNoErrorStop,CHAR *Mess,...)
{
	
	int err;
	CHAR *lpBuf=ehAlloc(SQL_BUFFER_QUERY_DEFAULT);

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	va_end(Ah);

	if (psOdbcSection->fDebug) {printf("%s",lpBuf); printf(CRLF);}
	err=odbc_query(DYN_ODBC_SECTIONC fNoErrorStop,lpBuf);
	ehFree(lpBuf);
	return err;
}
*/
int odbcQueryBig(EH_ODBC_SECTION * psOdbcSection,DWORD dwSizeMemory,CHAR *Mess,...)
{
	
	int err;
	CHAR *lpBuf=ehAlloc(dwSizeMemory);

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	va_end(Ah);

	//if (psOdbcSection->fDebug) {printf("%s",lpBuf); printf(CRLF);}
	err=odbcQuery(psOdbcSection,lpBuf);
	ehFree(lpBuf);
	return err;
}


//
// odbc_store_result()
//
#ifdef EH_MEMO_DEBUG 
EH_ODBC_RS _odbcStore(EH_ODBC_SECTION * psOdbcSection,INT iLimit,CHAR * pProg, INT iLine)
#else
EH_ODBC_RS odbcStore(EH_ODBC_SECTION * psOdbcSection,INT iLimit)
#endif
{
	EH_ODBC_RS pRes;
//	SQLRETURN sqlReturn;

	// Trovo dizionario e creo buffer
	if (psOdbcSection->sqlLastError!=SQL_SUCCESS&&
		psOdbcSection->sqlLastError!=SQL_SUCCESS_WITH_INFO) {return NULL;} // Precedente errore in query

	pRes=odbcQueryResultCreate(psOdbcSection,iLimit); 
	pRes->psOdbcSection=psOdbcSection;
#ifdef EH_MEMO_DEBUG 
		memDebugUpdate(pRes,pProg,iLine);
#endif
	SQLAllocHandle(SQL_HANDLE_STMT, psOdbcSection->hConn, &psOdbcSection->hStmt);
		
/*
	// Indicare il tipo di cursore > Sospesa per ora
	sqlReturn=SQLSetStmtAttr(psOdbcSection->hStmt, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0);
	if (sqlReturn==SQL_ERROR)  
	{
		sqlReturn=SQLSetStmtAttr( psOdbcSection->hStmt,SQL_ATTR_CURSOR_TYPE,  (SQLPOINTER) SQL_CURSOR_STATIC, 0);
		if (sqlReturn==SQL_ERROR) win_infoarg("errore in assegnazione cursore");
		sqlReturn=SQLSetStmtAttr( psOdbcSection->hStmt, SQL_ATTR_USE_BOOKMARKS, (SQLPOINTER) SQL_UB_VARIABLE, 0);
		if (sqlReturn==SQL_ERROR) win_infoarg("SQL_ATTR_USE_BOOKMARKS");
	}
*/

	if (pRes) pRes->pszQuery=strDup(psOdbcSection->pLastQuery);
	return pRes;
}


//
// odbc_fetch_row()
// Avanzo di una riga
//
BOOL odbc_fetch_row( EH_ODBC_RS pRes) 
{
// 	
	SQLRETURN sqlReturn;
	if (!pRes) return FALSE;
 	if (!pRes->bDataReady) // Prima volta
	{
		sqlReturn=SQLSetStmtAttr(pRes->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) pRes->iRowsLimit, 0); // Dimensione delle righe nel buffer
		sqlReturn=SQLSetStmtAttr(pRes->hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &pRes->iRowsReady, 0); // Righe realmente lette
		sqlReturn=SQLSetStmtAttr(pRes->hStmt, SQL_ATTR_MAX_ROWS, &pRes->iMaxRows, 0); // Righe realmente lette
		
		pRes->iOffset=0;
		pRes->iCurrentRow=0;
		pRes->bDataReady=TRUE; 
	}
	else
	{
		pRes->iCurrentRow++; 
		if ((pRes->iCurrentRow-pRes->iOffset)<pRes->iRowsReady) return true;
		pRes->iOffset+=pRes->iRowsReady;
	}

	pRes->psOdbcSection->sqlLastError=sqlReturn=SQLFetch(pRes->hStmt);
	if (sqlReturn==SQL_ERROR) {

		odbcGetDiagnostic(sqlReturn,pRes->hStmt,SQL_HANDLE_STMT,true);//,"x",pRes->hStmt,sqlReturn);
//		ehError();
		//odbcError(pRes->psOdbcSection, "Fetch",0); 

	}

	// sqlReturn=SQLFetchScroll(pRes->hStmt,SQL_FETCH_NEXT,0);
	if (sqlReturn==SQL_NO_DATA_FOUND) return false;
	if (sqlReturn==SQL_SUCCESS||sqlReturn==SQL_SUCCESS_WITH_INFO) return true;
	return TRUE;
}

//
// odbc_setcursor() (Beta - da testare su grossi volumi)
//
BOOL odbc_setcursor(EH_ODBC_RS pRes,DWORD dwCursor) {
	pRes->iCurrentRow=dwCursor-1;
	pRes->iOffset=0;
	return FALSE;
}

//
// odbc_fetch_first()
//
BOOL odbc_fetch_first(EH_ODBC_RS pRes) 
{
// 	
	SQLRETURN sqlReturn;
	
	if (!pRes->bDataReady) // Prima volta
	{
		sqlReturn=SQLSetStmtAttr(pRes->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) pRes->iRowsLimit, 0); // Dimensione delle righe nel buffer
		sqlReturn=SQLSetStmtAttr(pRes->hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &pRes->iRowsReady, 0); // Righe realmente lette
		sqlReturn=SQLSetStmtAttr(pRes->hStmt, SQL_ATTR_MAX_ROWS, &pRes->iMaxRows, 0); // Righe realmente lette
	
		pRes->bDataReady=TRUE; 
	}
	pRes->iOffset=0;
	pRes->iCurrentRow=-1;
	//pRes->bDataReady=FALSE;
	return FALSE;
}


//
// odbc_free_result()
//
void odbc_free_result(EH_ODBC_RS pRes) 
{
// 	
	if (!pRes) return;
	if (pRes->bCloseProtect)
	{
		printf("Close Protect");
	}
	if (pRes) odbc_QueryResultDestroy(pRes);
	if (pRes->hStmt) {
		SQLFreeHandle(SQL_HANDLE_ENV,pRes->hStmt); 
		SQLCloseCursor(pRes->hStmt);	
		pRes->hStmt=NULL;
	}	
	ehFreePtr(&pRes->pszQuery);
	ehFreeNN(pRes);
}

// odbc_f MySqlField
// Operazione su i campi di una query
// odbc_ffind() ricerca
int odbc_fldfind(EH_ODBC_RS pRes,CHAR *lpField,BOOL bNoError) 
{
// 	
	INT i;
	for (i=0; i<pRes->iFieldNum; i++)
	{
		if (!strCaseCmp(pRes->arDict[i].szName,lpField)) return i;
	}
	if (atoi(lpField)>0) return atoi(lpField)-1; // Numero di colonna
	if (!bNoError) ehExit("campo inesistente [%s]",lpField); 
	return -1;
}

//
// odbc_fldptr()
//
char * odbc_fldptr(EH_ODBC_RS pRes, CHAR *lpField) 
{
// 	
	CHAR *pRet;
	INT iField=odbc_fldfind(pRes,lpField,FALSE); //if (i<0) return NULL;
	INT iRow=pRes->iCurrentRow-pRes->iOffset;
	if (iRow<0||iRow>pRes->iRowsLimit)
	{
		ehPrintd("odbc_fldptr() error: [%s] %d,%d" CRLF,lpField,iRow,pRes->iRowsLimit);
		return NULL;
	}
	iRow*=pRes->arDict[iField].iSize;
	pRet=pRes->arDict[iField].arBuffer+iRow; // DA VEDERE
	if (pRet&&pRes->psOdbcSection->bRightTrim) strTrimRight(pRet); // modificato 2015
	return pRet;

}

//
// odbc_fldlen()
//
SIZE_T odbc_fldlen(EH_ODBC_RS pRes, CHAR *lpField) 
{
	//
	CHAR *pRet;
	INT iField=odbc_fldfind(pRes,lpField,FALSE); //if (i<0) return NULL;
	INT iRow=pRes->iCurrentRow-pRes->iOffset;
	if (iRow<0||iRow>pRes->iRowsLimit)
	{
		ehPrintd("odbc_fldptr() error: [%s] %d,%d" CRLF,lpField,iRow,pRes->iRowsLimit);
		return -1;
	}
	if (pRes->arDict[iField].iType==SQL_LONGVARBINARY) return pRes->arDict[iField].arsLen[iRow];
	iRow*=pRes->arDict[iField].iSize;
	pRet=pRes->arDict[iField].arBuffer+iRow; // DA VEDERE
	if (pRet&&pRes->psOdbcSection->bRightTrim) strTrimRight(pRet);
	return strlen(pRet);

}

//
// odbc_fldinfo()
//
EH_ODBC_FIELD * odbc_fldinfo(EH_ODBC_RS pRes, CHAR *lpField,BOOL bNoError)
{
//	
	INT iField=odbc_fldfind(pRes,lpField,bNoError); //if (i<0) return NULL;
	INT iRow=pRes->iCurrentRow-pRes->iOffset;
	if (iField<0) return NULL;
	if (iRow<0||iRow>pRes->iRowsLimit)
	{
		ehPrintd("odbc_fldptr() error : %d,%d" CRLF,iRow,pRes->iRowsLimit);
		return NULL;
	}
	return &pRes->arDict[iField];
//	return pRes->arDict[iField].lpBuffer+iRow; // DA VEDERE
}

/*
char * odbc_GetDate(int hdb,CHAR *lpField) 
{
	CHAR *ptd;
	static CHAR szServ[20];
	ptd=odbc_fptr(lpField); *szServ=0;
	if (!*ptd) return szServ;
	memcpy(szServ,ptd+6,2); // Anno
	memcpy(szServ+2,ptd+4,2); // Anno
	memcpy(szServ+4,ptd,4); // Anno
	szServ[8]=0;
	return szServ;
}
*/

//
//	Gestione sezioni multiThread
//

// #ifdef EH_ODBC_MT

//
// odbcSectionGet() - EX odbc_MTSectionId()
//
EH_ODBC_SECTION * odbcSectionGet(DWORD idThread)// ,BOOL bCritical)
{
	EH_ODBC_SECTION * psSection=NULL;
	//if (bCritical) EnterCriticalSection(&csODBCSection);
	if (!_p.lstSection) return NULL;

	_mutex_section_open_
	for (lstLoop(_p.lstSection,psSection)) {
		if (psSection->idThread==idThread) 
		{
			_mutex_section_close_
			return psSection;
		}
	}
	_mutex_section_close_
	return NULL;
// 		if (arsOdbcSection[a].idThread==idThread) {idx=a; break;}
//	}
	//if (bCritical) LeaveCriticalSection(&csODBCSection);
//	_mutex_section_close_
//	return idx;
}


//
// odbcSectionCreate() - odbc_MTOpenSection()
//
EH_ODBC_SECTION * odbcSectionCreate(DWORD idThread)
{
	EH_ODBC_SECTION * psSection,*psRet;
	psSection=odbcSectionGet(idThread); if (psSection) ehExit("odbcSectionCreate(): #%d creato due volte",idThread);
	
	psSection=ehAllocZero(sizeof(EH_ODBC_SECTION));
	psSection->idThread=idThread;
	psSection->lpQuery=ehAlloc(SQL_BUFFER_QUERY_DEFAULT); * psSection->lpQuery=0;
	psSection->fDebug=false;
	psSection->dwQueryCounter=0;

	_mutex_section_open_
	psRet=lstPush(_p.lstSection,psSection);
	ehFree(psSection);
	_mutex_section_close_

	return psRet;
}

//
// odbcSectionClone() - odbc_MTOpenSection()
//
EH_ODBC_SECTION * odbcSectionStmtClone(EH_ODBC_SECTION * psSection,INT idThread)
{
	EH_ODBC_SECTION * psRet;
//	SQLRETURN sqlReturn;

// 	psSection=odbcSectionGet(idThread); if (psSection) ehExit("odbcSectionCreate(): #%d creato due volte",idThread);
	
	psRet=odbcSectionCreate(idThread);
	
	psRet->pSqlHost=strDup(psSection->pSqlHost);
	psRet->pSqlUser=strDup(psSection->pSqlUser);
	psRet->pSqlPass=strDup(psSection->pSqlPass);
	psRet->pSqlSchema=strDup(psSection->pSqlSchema);
	psRet->iSqlClientFlag=psSection->iSqlClientFlag;
	psRet->uiAsync=psSection->uiAsync;
	psRet->uiScroll=psSection->uiScroll;
	psRet->uiDynamic=psSection->uiDynamic;
	psRet->hEnv=psSection->hEnv;
	psRet->bClone=true;
	if (_odbcConnection(psRet,psSection->hConn,false)) return NULL;
	
	return psRet;
}

/*
//
// odbc_MTGetSection()
//
EH_ODBC_SECTION *odbc_MTGetSection(DWORD idThread)
{
	INT idx=odbc_MTSectionId(idThread,TRUE);
	if (idx<0) 
	{
		ehLogWrite("odbc_MTGetSection(): #%d ?",idThread); return NULL;
	}
	return &arsOdbcSection[idx];
}
*/
//
// odbc_MTCloseSection()
// Ritorna true se la sezione non esiste
//
BOOL odbcSectionDestroy(DWORD idThread)
{
	EH_ODBC_SECTION * psSection;
	EH_LST_I * psItem;
	psSection=odbcSectionGet(idThread); if (!psSection) ehError();

	_mutex_section_open_
	if (!psSection->bClone) 
	{
		// if (psSection->hConn) 
		odbcDisconnect(psSection);
	}
	else {

//		if (psSection->pRes) {odbc_QueryResultDestroy(psSection->pRes); ehFree(psSection->pRes);}
		//psOdbc->pRes=NULL;
		if (psSection->hStmt) {
			SQLFreeHandle(SQL_HANDLE_STMT,psSection->hStmt); 
			psSection->hStmt=0;
		}

		ehFreePtrs(4,&psSection->pSqlHost,&psSection->pSqlUser,&psSection->pSqlPass,&psSection->pSqlSchema);

	}
	ehFreePtr(&psSection->lpQuery);
	ehFreePtr(&psSection->pLastQuery);

	// Elimino dalla lista

	psItem=lstSearch(_p.lstSection,psSection);
	if (!psItem) ehError();
	lstRemoveI(_p.lstSection,psItem);

	_mutex_section_close_

	return FALSE;
}

//
// odbcSectionCloseAll() - odbc_MTCloseAllSection()
//
void odbcSectionCloseAll(void)
{
	EH_ODBC_SECTION * psSection;
	for (;_p.lstSection->iLength;) {
	_mutex_section_open_
		psSection=lstGet(_p.lstSection,_p.lstSection->iLength-1);
	_mutex_section_close_
		odbcSectionDestroy(psSection->idThread);
	}
}

//
// odbcSectionGetThreads() - odbc_MTGetThreads()
//
CHAR * odbcSectionGetThreads(void)
{
	EH_LST lstId=lstNew();
	EH_ODBC_SECTION * psSection;
	CHAR * pszRet;
	//CHAR *lpStatus=ehAlloc(32000); *lpStatus=0;
//	EnterCriticalSection(&csODBCSection);
	_mutex_section_open_
	for (lstLoop(_p.lstSection,psSection))
	{
		lstPushf(lstId,"%d",psSection->idThread);
	}
	_mutex_section_close_
	pszRet=lstToString(lstId,",","","");
	lstDestroy(lstId);
//	LeaveCriticalSection(&csODBCSection);
	return pszRet;
}

//
// odbcSectionMain()
//
EH_ODBC_SECTION * odbcSectionMain(void) {

	if (!_p.psSectionMain) ehError();
	return _p.psSectionMain;

}

//
//	QueryResultAllocation()
//
EH_ODBC_RS  odbcQueryResultCreate(EH_ODBC_SECTION * psOdbcSection,INT iLimit)
{
	 SQLCHAR szNameColumn[255];
	 SQLSMALLINT NameLenght;
	 SQLSMALLINT DataType;
	 SQLUINTEGER uiColumnSize;
	 SQLSMALLINT DecimnaDigits;
	 SQLSMALLINT Nullable;
	 CHAR *lpType;
	 INT iCount;
	 INT iRowMemory;
	 INT i,iTypeC;
	 INT iAdbType;

	 EH_ODBC_RS pRes=NULL;

	 //
	 // Libera le risorse precedentemente impegnate
	 //
	 // odbc_QueryResultDestroy(pRes);

	 // Sposto l'handle di stantment nella Result ed alloco un nuovo stantment pubblico
	 pRes=ehAllocZero(sizeof(EH_ODBC_RESULTSET));  // Mi rimane allocata <-----------
	 pRes->hStmt=psOdbcSection->hStmt;
	 pRes->iRowsLimit=iLimit;
	 pRes->psOdbcSection=psOdbcSection;

	 //
	 // Conta i campi presenti nella query
	 //
	 iCount=0; iRowMemory=0;
	 for (i=1;;i++)
	 {
		  if (SQLDescribeCol(pRes->hStmt, // Handle del file
							 (SQLSMALLINT) i, // Numero della colonna
							 szNameColumn,sizeof(szNameColumn),
							 &NameLenght,&DataType,
							 &uiColumnSize,
							 &DecimnaDigits,
							 &Nullable)!=SQL_SUCCESS) break;
		  iCount++;
	 }

	 //
	 // Alloco memoria per descrizione Campi
	 //
	 pRes->iFieldNum=iCount;
	 pRes->arDict=ehAllocZero(sizeof(EH_ODBC_FIELD)*iCount);

	 // Alloco e binderizzo i campi
	 for (i=0;i<pRes->iFieldNum;i++)
	 {
		if (SQLDescribeCol(pRes->hStmt, // Handle del file
						 (SQLSMALLINT) (i+1), // Numero della colonna
						 szNameColumn,
						 (SQLSMALLINT) sizeof(szNameColumn),
						 &NameLenght,
						 &DataType,
						 &uiColumnSize,
						 &DecimnaDigits,
						 &Nullable)!=SQL_SUCCESS) break;
	
		lpType="<?>"; iTypeC=SQL_C_CHAR; iAdbType=_ALFA;
		switch (DataType)
		{
			case SQL_CHAR:     lpType="ALFAF";    iTypeC=SQL_C_CHAR; break;

			case -9: // odbc 5.1
			case SQL_VARCHAR:  
				lpType="ALFA";     iTypeC=SQL_C_CHAR; 
				break;

			case SQL_DECIMAL:  lpType="DECIMAL";  iTypeC=SQL_C_CHAR; iAdbType=_NUMBER; break;
			case SQL_NUMERIC:  lpType="NUMERIC";  iTypeC=SQL_C_CHAR; iAdbType=_NUMBER; break;
			
			case SQL_BIGINT: // Mha
			case SQL_SMALLINT: lpType="SMALLINT"; iTypeC=SQL_C_CHAR; iAdbType=_INTEGER; 
				break;

			case SQL_TINYINT:
			case SQL_INTEGER:  
				lpType="INTEGER";  
				iTypeC=SQL_C_CHAR; 
				iAdbType=_INTEGER; 
				break;

			case SQL_REAL:	   lpType="REAL";     iTypeC=SQL_C_CHAR; iAdbType=_NUMBER; break;
			case SQL_FLOAT:	   lpType="FLOAT";    iTypeC=SQL_C_CHAR; iAdbType=_NUMBER; break;
			case SQL_DOUBLE:   lpType="DOUBLE";   iTypeC=SQL_C_CHAR; iAdbType=_NUMBER; break;
			case SQL_BIT:	   lpType="BIT";      iTypeC=SQL_C_CHAR; iAdbType=_BOOL; break;
/*
#define SQL_LONGVARCHAR                         (-1)
#define SQL_BINARY                              (-2)
#define SQL_VARBINARY                           (-3)
#define SQL_LONGVARBINARY                       (-4)
#define SQL_BIGINT                              (-5)
#define SQL_TINYINT                             (-6)
#define SQL_BIT                                 (-7)
*/

			case SQL_LONGVARBINARY: 
				lpType="BYNARY"; 
				iTypeC=SQL_C_BINARY; 
				uiColumnSize=65000;
				iAdbType=_BINARY; 
				break;
			
			case -10: // odbc 5.1
			case SQL_LONGVARCHAR: 
			case 93: // Tempo
				lpType="TEXT"; 
				iTypeC=SQL_C_CHAR; 
				iAdbType=_TEXT; 
				uiColumnSize=65000;
				break;

			default:
				//printf("qui");
				ehError();
				break;
		}

		uiColumnSize++; // Aumento di uno la grandezza della colonna
		if (uiColumnSize>64000) uiColumnSize=64000;
		strcpy(pRes->arDict[i].szName,szNameColumn);
		strcpy(pRes->arDict[i].szType,lpType);
		pRes->arDict[i].iType=DataType;
		pRes->arDict[i].iAdbType=iAdbType;
		pRes->arDict[i].iSize=uiColumnSize;
		pRes->arDict[i].arBuffer=ehAllocZero(uiColumnSize*pRes->iRowsLimit);
		if (iTypeC==SQL_C_BINARY||Nullable) // Version ODBC 5.x
			pRes->arDict[i].arsLen=ehAllocZero(sizeof(SQLLEN)*pRes->iRowsLimit);
			else 
			pRes->arDict[i].arsLen=NULL;
		if (!pRes->arDict[i].arsLen&&Nullable) ehError();
		if (!pRes->arDict[i].arBuffer) ehError();

	  // fprintf(stderr,"%d) pRes->arDict[i].lpBuffer=%x" CRLF,i,pRes->arDict[i].lpBuffer);

		if (SQLBindCol(pRes->hStmt, 
						(SQLSMALLINT) (i+1), // Colonna
						(SQLSMALLINT) iTypeC, 
						(PTR) pRes->arDict[i].arBuffer, // Buffer
						pRes->arDict[i].iSize, 
						pRes->arDict[i].arsLen)) // Dimensioni
						{ehExit("ODBC: BINDErrore in [%s]",pRes->arDict[i].szName);}
	 }

	//ehPrintd("odbc_QueryResultCreate" CRLF);
	return pRes;
}

//
// odbc_QueryResultDestroy()
//
void odbc_QueryResultDestroy(EH_ODBC_RS pRes)
{
	INT i;
	
	if (!pRes) return;
	if (!pRes->arDict) return;

	// Libero risorse precedentemente impegnate
	for (i=0;i<pRes->iFieldNum;i++)
	{
		ehFreePtrs(2,&pRes->arDict[i].arBuffer,&pRes->arDict[i].arsLen);  
	}
	ehFreePtr(&pRes->arDict);
	//ehFree(pRes);
	//ehPrintd("odbc_QueryResultDestroy" CRLF);
}



/*
	sRet=SQLSetStmtAttr(psOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) psOdbcSection->iRowsSize, 0); // Dimensione delle righe nel buffer
	sRet=SQLSetStmtAttr(psOdbcSection->hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &psOdbcSection->iRows, 0); // Righe realmente lette
*/

//
//  odbc_QueryLastError()
//
CHAR * odbc_QueryLastError(EH_ODBC_SECTION * psOdbcSection,CHAR *lpQuery)
{
	
	static CHAR szError[8000];
	strcpy(szError,"?");
	//sprintf(szError,"Error:%d - %s" CRLF "%s",ODBC_errno(psOdbcSection->mySql),ODBC_error(psOdbcSection->mySql),lpQuery);
	return szError;
}

//
// odbc_querytoarray()
// Ritorna il valore di un campo solo da una query (es DESCRIZIONE di un codice)
//
// deve essere liberato con ARDestroy()
EH_AR odbcQueryToArray(EH_ODBC_SECTION * psOdbcSection,CHAR *Mess,...)
{
	
	EH_ODBC_RS pRes;
	EH_AR arRet=NULL;
	INT i;
	CHAR szServ[10];

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(psOdbcSection->lpQuery,Mess,Ah); // Messaggio finale
	va_end(Ah);

	strIns(psOdbcSection->lpQuery,"?"); // Richiedo controllo
	if (odbcQuery(psOdbcSection,psOdbcSection->lpQuery)) return NULL;

//#ifdef EH_ODBC_MT
//	pRes=odbc_store_result(psOdbcSection,3); // Richiedo il risultato
//#else
	pRes=odbcStore(psOdbcSection,3); // Richiedo il risultato
//#endif
	if (pRes)
	{
		while (odbc_fetch_row(pRes)) 
		{
			arRet=ehAlloc(sizeof(CHAR *)*(pRes->iFieldNum+1));
			for (i=0;i<pRes->iFieldNum;i++)
			{
				sprintf(szServ,"%d",(i+1)); arRet[i]=strDup(odbc_fldptr(pRes,szServ)); // Poco elegante ma funzionale ...
			}
			arRet[i]=NULL;
			break;
		}
		odbc_free_result(pRes); // Libero le risorse
	}
	return arRet;
}

//
// odbc_queryrow()
// Ritorna il valore di un campo solo da una query (es DESCRIZIONE di un codice)
//
// deve essere liberato con odbc_free_result(pRes)
EH_ODBC_RS odbcRow(EH_ODBC_SECTION * psOdbcSection,CHAR *Mess,...) {

	
	EH_ODBC_RS pRes;
	BYTE *pLimit;

	va_list Ah;

	if (!psOdbcSection->enPlatform) ehExit("odbc_queryrow(): definire la piattaforma");

	va_start(Ah,Mess);
	vsprintf(psOdbcSection->lpQuery,Mess,Ah); // Messaggio finale
	va_end(Ah);

	if (*psOdbcSection->lpQuery!='?') strIns(psOdbcSection->lpQuery,"?");
	strTrim(psOdbcSection->lpQuery);	strcat(psOdbcSection->lpQuery," "); 
	pLimit=odbcLimit(psOdbcSection,1,1); if (!pLimit) ehError();
	strcat(psOdbcSection->lpQuery,pLimit);
	if (strlen(psOdbcSection->lpQuery)>=SQL_BUFFER_QUERY_DEFAULT) ehError();

	if (odbcQuery(psOdbcSection,psOdbcSection->lpQuery)) 
	{
		CHAR * pszError=odbcError(psOdbcSection,__FUNCTION__,2);
		printf("\7%s",pszError);
		ehFree(pszError);

		return NULL;
	}

//#ifdef EH_ODBC_MT
	//pRes=odbc_store_result(psOdbcSection,2); // Richiedo il risultato
//#else
	pRes=odbcStore(psOdbcSection,2); // Richiedo il risultato
//#endif
	if (pRes)
	{
		if (odbc_fetch_row(pRes)) return pRes;

		// Libero le risorse
		odbc_free_result(pRes);
	}
	return NULL;
}

#ifndef EH_CONSOLE
//
// odbc_hookget()
// ritorna TRUE se c'è errore o il record non c'è
//
BOOL odbcHookGet(EH_ODBC_SECTION * psOdbcSection,EH_TABLEHOOK *arsHook,CHAR *Mess,...)
{
	
	EH_ODBC_RS pRes;
	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(psOdbcSection->lpQuery,Mess,Ah); // Messaggio finale
	va_end(Ah);

	strIns(psOdbcSection->lpQuery,"?"); // Richiedo controllo
	pRes=odbcRow(psOdbcSection,psOdbcSection->lpQuery);
	if (!pRes) return TRUE;

	odbc_hookgetRs(arsHook,pRes);
	odbc_free_result(pRes); // Libero le risorse
	return FALSE;
}

//
// odbc_hookreset()
//
int odbcHookReset(EH_TABLEHOOK *arsHook) {
	return odbcHookGetRs(arsHook,NULL);
}

int odbcHookGetRs(EH_TABLEHOOK *arsHook,EH_ODBC_RS pRes)
{
	

	INT a;
	BYTE *pFieldName,*pFieldValue;
	INT iSpiaz;
//	INT idxField;
	CHAR szObjName[20];
	INT iValue;
	BOOL bError=TRUE;

	bError=FALSE;
	for (a=0;arsHook[a].pTableFieldName;a++)
	{
		pFieldName=arsHook[a].pTableFieldName;
		if (*pFieldName=='#') pFieldName++;// Solo in lettura incremento pt di uno
		if (pRes) pFieldValue=odbc_fldptr(pRes,pFieldName); else pFieldValue="";
		if (!pFieldValue) {
			alert(__FUNCTION__":NomeField ? \n[%s] in Query[%s]",arsHook[a].pTableFieldName,pRes->psOdbcSection->lpQuery);
			continue;
		}

	//	idxField=odbc_fldfind(pRes,pFieldName,FALSE);

		//
		// Binding diretto > Scrittura della stringa puntata da pObjName
		// 
		if (arsHook[a].iType&TH_BIND) {
			strcpy(arsHook[a].pObjName,pFieldValue);
			continue;
		}
		//
		// TH_FUNC > Notifico a funzione esterna
		//
		else if (arsHook[a].iType&TH_FUNC) {
			EH_TABLEHOOKNOTIFY sTHN;
			sTHN.pszFieldName=pFieldName;
			sTHN.pszFieldValue=pFieldValue;
			sTHN.psTableHookField=&arsHook[a];
			arsHook[a].funcExt(WS_REALSET,0,&sTHN);
			continue;
		}

		strcpy(szObjName,arsHook[a].pObjName);
		iSpiaz=0;
		if (*szObjName=='#') // Non ricordo !!!!
		{
			if (szObjName[1]=='+') iSpiaz=atoi(&szObjName[2]);
			strcpy(szObjName,"");
		}

		//
		// Hookad oggetto
		//
		if (*szObjName)
		 {
			INT iObjTipo=obj_type(szObjName);
			if (iObjTipo<0) win_errgrave(__FUNCTION__":ObjName in Hook errato");
			switch (arsHook[a].iType&TH_MASK)
			{
				case ALFA:
					switch(iObjTipo)
					{
						case O_RADIO:
						case O_MARK:
								obj_on(pFieldValue); // Mha ?
								break;

						case O_LIST:
						case O_LISTP:
						case OW_LIST:
						case OW_LISTP:

								obj_listcodset(szObjName,pFieldValue);
								break;

						case O_ZONAP:
								obj_message(szObjName,WS_REALSET,0,pFieldValue); 
								break;

						default : ehExit("HW:ALFA/ObjError");
					}
					break;

				case _NUMBER:
				case _INTEGER:
				case _BOOL:

					iValue=atoi(pFieldValue);
					switch(iObjTipo)
					{
						 case O_IMARKA:
						 case O_IMARKB:
						 case O_MARK: 
							
							 if (iValue) obj_on(szObjName); else obj_off(szObjName);
							 break;

						 case O_LIST:
						 case O_LISTP:
						 case OW_LIST:
						 case OW_LISTP:
							   obj_listset(szObjName,iValue);
							   break;

						 default : ehExit("HGN:ObjError");
					}
					break;
			}
		}
		else
		//
		// Campo di input
		//
		{
			INT iIptNumber=arsHook[a].iInput;
			BYTE *pData;
			EH_IPTINFO * psIptInfo=iptGetInfo(-1);
			if ((iIptNumber<0)||(iIptNumber>psIptInfo->numcampi-1)) win_errgrave(__FUNCTION__":IptNum in Hook errato");

			switch (arsHook[a].iType&TH_MASK)
			{
				case _NUMBER : 
				case _BOOL: 
				case _INTEGER:
						ipt_writevedi(iIptNumber,NULL,atof(pFieldValue+iSpiaz));
						break;
				 
				 case DATA:
						pData=strTrim(pFieldValue+iSpiaz);
						// win_infoarg("[%s]",pData);
						if (*pData) ipt_writevedi(iIptNumber,dateYtoD(pData),0); else ipt_writevedi(iIptNumber,"",0);
						//mask=psIptInfo->mask;
						break;

				 default:
				 case ALFA :
						ipt_writevedi(iIptNumber,pFieldValue+iSpiaz,0);
						//mask=psIptInfo->mask;
						break;
			}
		}
	}
	return FALSE;//bError;
}

//
// odbc_hookinsert()
// ritorna TRUE se c'è errore
//
BOOL odbcHookInsert(EH_ODBC_SECTION * psOdbcSection,EH_TABLEHOOK *arsHook,CHAR *pQuery,...)
{
	
	CHAR *pszQuery=ehAlloc(64000);
	CHAR *pFieldList=ehAlloc(8000);
	CHAR *pValueList=ehAlloc(64000);
	CHAR *pValue=ehAlloc(64000);

	INT a;
	BYTE *p,*pFieldName;
	BOOL bError=TRUE;

	va_list Ah;
	va_start(Ah,pQuery);
	vsprintf(pszQuery,pQuery,Ah); // Messaggio finale
	va_end(Ah);

	// 
	// Cerco il Campo FIELDS
	//
	*pFieldList=0; *pValueList=0;
	for (a=0;arsHook[a].pTableFieldName;a++)
	{
		pFieldName=arsHook[a].pTableFieldName; 
		if (*pFieldName=='#') continue;
		if (arsHook[a].iType&TH_BIND) continue; // Mha ... non va mica bene !!!!

		// FIELDS
		if (*pFieldList) strcat(pFieldList,",");
		strcat(pFieldList,arsHook[a].pTableFieldName);

		//
		// Richiedo a funzione esterna
		//
		if (arsHook[a].iType&TH_FUNC) {

			EH_TABLEHOOKNOTIFY sTHN;
			sTHN.pszFieldName=pFieldName;
			*pValue=0;
			sTHN.pszFieldValue=pValue;
			sTHN.psTableHookField=&arsHook[a];
			arsHook[a].funcExt(WS_REALGET,0,&sTHN);
			switch (arsHook[a].iType&TH_MASK)
			{
				case _ALFA:
				case _TEXT:
					p=strEncode(pValue,SE_SQL,NULL);
					sprintf(pValue,"'%s'",p);
					ehFree(p);
					break;
			}
		}
		//
		// OGGETTI
		//
		else if (*arsHook[a].pObjName)
		{
			INT iObjTipo=obj_type(arsHook[a].pObjName);
			BYTE *pData;
//			struct OBJ * pObj=obj_GetInfo(arsHook[a].pObjName));

			switch (arsHook[a].iType&TH_MASK)
			{
				//
				// Valore ALFANUMERICO
				//
				case _ALFA:

					switch(iObjTipo)
					{
						case O_RADIO:
						case O_LIST:
						case O_LISTP:
						case OW_LIST:
						case OW_LISTP:

							p=obj_listcodget(arsHook[a].pObjName); if (p==NULL) ehExit("HW:ALFA/O_LIST ObjError");
							p=strEncode(p,SE_SQL,NULL);
							sprintf(pValue,"'%s'",p);
							ehFree(p);
							break;

						case O_ZONAP:
							pData=ehAlloc(32000);
							obj_message(arsHook[a].pObjName,WS_REALGET,32000,pData);
							p=strEncode(pData,SE_SQL,NULL);
							sprintf(pValue,"'%s'",p);
							ehFree(p);
							ehFree(pData);
							break;

						default: 
							ehExit("%s:%s %d\n[%s] Tipo %d non gestito in modo ALFA",__FILE__,__FUNCTION__,__LINE__,pFieldName,iObjTipo);
					}
					break;

				//
				// Valore NUMERICO
				//
				case _NUMBER:
				case _INTEGER:
				case _BOOL:

					switch(iObjTipo)
					{
						case O_IMARKA:
						case O_IMARKB:
						case O_MARK:
							sprintf(pValue,"%d",obj_status(arsHook[a].pObjName));
							break;

						case O_LIST:
						case O_LISTP:
						case OW_LIST:
						case OW_LISTP:  
							sprintf(pValue,"%d",obj_listget(arsHook[a].pObjName));
							break;

						default : ehExit("%s:%s %d\n[%s] Tipo %d non gestito in modo NUME",__FILE__,__FUNCTION__,__LINE__,pFieldName,iObjTipo);
					}
					break;
			}
		}
		//
		// Campi di INPUT
		//
		else
		{
			INT iIptNumber=arsHook[a].iInput;
			struct IPT *psIpt;
			EH_IPTINFO * psIptInfo=iptGetInfo(-1);

			if ((iIptNumber<0)||(iIptNumber>psIptInfo->numcampi-1)) win_errgrave(__FUNCTION__":IptNum in Hook errato");
			psIpt=ipt_GetInfo(iIptNumber);
			switch (arsHook[a].iType&TH_MASK)
			{
				case _NUMBER:
						if (psIpt->num1)
								sprintf(pValue,"%.3f",atof(ipt_read(iIptNumber))); // Bisognerebe vedere il numero di decimali
								else
								sprintf(pValue,"%d",atoi(ipt_read(iIptNumber)));
						break;

				case _INTEGER:
						sprintf(pValue,"%d",atoi(ipt_read(iIptNumber)));
						break;

				case _BOOL:
						sprintf(pValue,"%d",atoi(ipt_read(iIptNumber))?TRUE:FALSE);
						break;

				default:
				case ALFA:
						p=strEncode(ipt_read(iIptNumber),SE_SQL,NULL);
						sprintf(pValue,"'%s'",p);
						ehFree(p);
						break;

				case DATA: // Da vedere
						p=strDup(ipt_read(iIptNumber));
						sprintf(pValue,"'%s'",dateDtoY(p));
						ehFree(p);
						//win_infoarg("Da vedere [%s]",p);
						break;
			}
		}

		//
		// Aggiungo il valore
		// 
		if (*pValueList) strcat(pValueList,",");
		strcat(pValueList,pValue);

	}

	strReplace(pszQuery,"[FIELDS]",pFieldList);
	strReplace(pszQuery,"[VALUES]",pValueList);

//	win_infoarg(pszQuery);
	bError=odbcQuery(psOdbcSection,pszQuery);

	ehFree(pszQuery);
	ehFree(pFieldList);
	ehFree(pValueList);
	ehFree(pValue);

	return bError;
}
//
// odbc_hookupdate()
// ritorna TRUE se c'è errore
//
int odbcHookUpdate(EH_ODBC_SECTION * psOdbcSection,EH_TABLEHOOK *arsHook,CHAR *pQuery,...)
{
	
	CHAR *pszQuery=ehAlloc(64000);
	CHAR *pFieldValueList=ehAlloc(8000);
	CHAR *pValue=ehAlloc(64000);

	INT a;
	BYTE *p,*pFieldName;
	BOOL bError=TRUE;

	va_list Ah;
	va_start(Ah,pQuery);
	vsprintf(pszQuery,pQuery,Ah); // Messaggio finale
	va_end(Ah);

	// 
	// Cerco il Campo FIELDS
	//
	*pFieldValueList=0; 
	for (a=0;arsHook[a].pTableFieldName;a++)
	{
		pFieldName=arsHook[a].pTableFieldName; if (*pFieldName=='#') continue;
		if (arsHook[a].iType&TH_BIND) continue; // Neanche qui non va bene
		
		// FIELDS
		if (*pFieldValueList) strcat(pFieldValueList,",");
		strcat(pFieldValueList,arsHook[a].pTableFieldName); strcat(pFieldValueList,"=");

		//
		// Richiedo a funzione esterna
		//
		if (arsHook[a].iType&TH_FUNC) {

			EH_TABLEHOOKNOTIFY sTHN;
			sTHN.pszFieldName=pFieldName;
			*pValue=0;
			sTHN.pszFieldValue=pValue;
			sTHN.psTableHookField=&arsHook[a];
			arsHook[a].funcExt(WS_REALGET,0,&sTHN);
			switch (arsHook[a].iType&TH_MASK)
			{
				case _ALFA:
				case _TEXT:
					p=strEncode(pValue,SE_SQL,NULL);
					sprintf(pValue,"'%s'",p);
					ehFree(p);
					break;
			}
		}
		//
		// OGGETTI
		//
		else if (*arsHook[a].pObjName)
		{
			INT iObjTipo=obj_type(arsHook[a].pObjName);
			BYTE *pData;
//			struct OBJ * pObj=obj_GetInfo(arsHook[a].pObjName));

			switch (arsHook[a].iType&TH_MASK)
			{
				//
				// Valore ALFANUMERICO
				//
				case ALFA:

					switch(iObjTipo)
					{
						case O_RADIO:
						case O_LIST:
						case O_LISTP:
						case OW_LIST:
						case OW_LISTP:

							p=obj_listcodget(arsHook[a].pObjName); if (p==NULL) ehExit("HW:ALFA/O_LIST ObjError");
							p=strEncode(p,SE_SQL,NULL);
							sprintf(pValue,"'%s'",p);
							ehFree(p);
							break;

						case O_ZONAP:
							pData=ehAlloc(32000);
							obj_message(arsHook[a].pObjName,WS_REALGET,32000,pData);
							p=strEncode(pData,SE_SQL,NULL);
							sprintf(pValue,"'%s'",p);
							ehFree(p);
							ehFree(pData);
							break;

						default: 
							ehExit("%s:%s %d\n[%s] Tipo %d non gestito in modo ALFA",__FILE__,__FUNCTION__,__LINE__,pFieldName,iObjTipo);
					}
					break;

				//
				// Valore NUMERICO
				//
				case _NUMBER:
				case _INTEGER:
				case _BOOL:

					switch(iObjTipo)
					{
						case O_IMARKA:
						case O_IMARKB:
						case O_MARK:
							sprintf(pValue,"%d",obj_status(arsHook[a].pObjName));
							break;

						case O_LIST:
						case O_LISTP:
						case OW_LIST:
						case OW_LISTP:  
							sprintf(pValue,"%d",obj_listget(arsHook[a].pObjName));
							break;

						default : ehExit("%s:%s %d\n[%s] Tipo %d non gestito in modo NUME",__FILE__,__FUNCTION__,__LINE__,pFieldName,iObjTipo);
					}
					break;
			}
		}
		//
		// Campi di INPUT
		//
		else
		{
			INT iIptNumber=arsHook[a].iInput;
			struct IPT *psIpt;
			EH_IPTINFO * psIptInfo=iptGetInfo(-1);

			if ((iIptNumber<0)||(iIptNumber>psIptInfo->numcampi-1)) win_errgrave(__FUNCTION__":IptNum in Hook errato");
			psIpt=ipt_GetInfo(iIptNumber);
			switch (arsHook[a].iType&TH_MASK)
			{
				case _NUMBER:
						if (psIpt->num1)
								sprintf(pValue,"%.3f",atof(ipt_read(iIptNumber))); // Bisognerebe vedere il numero di decimali
								else
								sprintf(pValue,"%d",atoi(ipt_read(iIptNumber)));
						break;

				case _INTEGER:
						sprintf(pValue,"%d",atoi(ipt_read(iIptNumber)));
//						strcpy(pValue,"7"); // togli subito !!
						break;

				case _BOOL:
						sprintf(pValue,"%d",atoi(ipt_read(iIptNumber))?TRUE:FALSE);
//						strcpy(pValue,"7"); // togli subito !!
						break;

				default:

				case ALFA:
						p=strEncode(ipt_read(iIptNumber),SE_SQL,NULL);
						sprintf(pValue,"'%s'",p);
						ehFree(p);
						break;

				case DATA: // Da vedere
						//win_infoarg("Da vedere");
						sprintf(pValue,"'%s'",dateDtoY(ipt_read(iIptNumber)));
						break;
			}
		}

		//
		// Aggiungo il valore
		// 
		strcat(pFieldValueList,pValue);

	}

	strReplace(pszQuery,"[FIELDS]",pFieldValueList);
//	win_infoarg(pszQuery);
	bError=odbcQuery(psOdbcSection,pszQuery);

	ehFree(pszQuery);
	ehFree(pFieldValueList);
	ehFree(pValue);

	return bError;
}

#endif

//
// odbcDisplay
//
CHAR * odbcDisplay(	SQLSMALLINT iType,
					CHAR *WhoIs,
					SQLHSTMT hstmt,
					SQLRETURN rc1)
{
	SQLCHAR  SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	CHAR *lpServ=ehAlloc(26000);
	SQLINTEGER NativeError;
	SQLSMALLINT i, MsgLen;
	SQLRETURN  rc2;
	CHAR *lpError=NULL;

	if ((rc1 == SQL_SUCCESS_WITH_INFO) || (rc1 == SQL_ERROR)) 
	{
		// Get the status records.
		i = 1;
		lpError=ehAlloc(16000); *lpError=0;
		while ((rc2 = SQLGetDiagRec(iType, hstmt, i, SqlState, &NativeError,Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) 
		{
			// DisplayError(SqlState,NativeError,Msg,MsgLen);
			  *lpServ=0;
			 sprintf(lpServ,"Error rc1=%d da '%s'\nSQLState = %s\nNativeError=%d\n%s (LenMsg=%d)\n", rc1,WhoIs,
							"SqlState",//SqlState,
							NativeError,Msg,MsgLen);
			 strcat(lpError,lpServ);
			 i++;
		}
	}
	ehFree(lpServ);
	return lpError;
}


//
// odbcGetDiagnostic
//
EH_LST odbcGetDiagnostic(SQLRETURN  sqlErr,HANDLE hStmt,SQLSMALLINT iType,BOOL bShow) {

	SQLCHAR  SqlState[20], szMsg[SQL_MAX_MESSAGE_LENGTH];
	SQLINTEGER iNativeError;
	SQLSMALLINT i, iMsgLen;
	SQLRETURN  rc2;
	CHAR *lpError=NULL;
	EH_LST lst=lstNew();

	if ((sqlErr == SQL_SUCCESS_WITH_INFO) || (sqlErr == SQL_ERROR)) 
	{
		// Get the status records.
		i = 1;
		
		while (true) {
			rc2 = SQLGetDiagRec(iType, hStmt, i, SqlState, &iNativeError,szMsg, sizeof(szMsg), &iMsgLen);
//			if (rc2==SQL_ERROR) break;
//			if (rc2==SQL_INVALID_HANDLE) break;
			if (rc2==SQL_NO_DATA) break;

			// DisplayError(SqlState,NativeError,Msg,MsgLen);
			lstPushf(lst,"%d:%s",iNativeError,szMsg,iMsgLen);
			i++;
		}
	}
	if (bShow) {lstPrint(lst); lst=lstDestroy(lst);}
	return lst;
}

void odbcSetParam(EH_ODBC_SECTION * psOdbcSection,EN_ODBC_PARAM enParam,DWORD dwParam) // 2010
{
	
	switch (enParam)
	{
		case OP_SET_PLATFORM:
			psOdbcSection->enPlatform=dwParam;
			break;

		case OP_RIGHT_TRIM:
			psOdbcSection->bRightTrim=dwParam;
			break;

		default:
			ehError();
	}
}

// http://en.wikipedia.org/wiki/Select_%28SQL%29

//
// odbcLimit()
//
CHAR * odbcLimit(EH_ODBC_SECTION * psOdbcSection,DWORD dwOffset,DWORD dwLimit)
{
	
	static CHAR szQuery[256];
	if (dwLimit<1||dwOffset<1) ehError();
	switch (psOdbcSection->enPlatform)
	{
		case ODBC_PLATFORM_DB2:
			if (dwLimit==1&&dwOffset==1) return " FETCH FIRST 1 ROW ONLY";
			if (dwOffset=1) 
			{
				sprintf(szQuery," FETCH FIRST %d ROW ONLY",dwLimit);
				break;
			}
			return NULL; // Non si può fare

		case ODBC_PLATFORM_MYSQL:
		   if (dwLimit==1&&dwOffset==1) return " LIMIT 0,1";
		   if (dwOffset=1) 
		   {
			sprintf(szQuery," LIMIT 0,%d",dwLimit);
			break;
		   }
		   return NULL; // Non si può fare

		default: ehError();
	}
	return szQuery;
}

/*
SELECT TEMP.* FROM (
    SELECT ROW_NUMBER() OVER () AS NUMTEMP, TABELLA.* FROM TABELLA
) AS TEMP WHERE NUMTEMP BETWEEN 0 AND 10
*/


// --------------------------------------------------------------------------------
// odbc_sourcelist
// Crea una lista con ARMAKER delle Sorgenti di dati presenti nel sistema
// Filtra i dati cercando la parola passata con *lpDriver nel driver collegato
// alla sorgente 
// --------------------------------------------------------------------------------
EH_AR odbcSourceList(EH_ODBC_SECTION * psOdbcSection,CHAR *lpDriver)
{
	//
	SQLHENV hEnv;
	SQLCHAR szServerName[180]="";
	SQLCHAR szServerDesc[180]="";
	SQLSMALLINT iServerName;
	SQLSMALLINT iServerDesc;
	SQLRETURN sqlReturn;
	EH_AR ar=ARNew();

#ifndef EH_CONSOLE
//	mouse_graph(0,0,"W:WAIT");
	MouseCursorWait();
#endif
	// -------------------------------
	// Elenco server disponibili
	//
	sqlReturn=SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
	SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0); 
 //LSQLTRY("SQLAllocHandleConn", SQLAllocHandle(SQL_HANDLE_DBC, EhOdbc.hEnv, &EhOdbc.hConn)); 

	sqlReturn=SQLDataSources(hEnv,
							 SQL_FETCH_FIRST,
							 szServerName,sizeof(szServerName)-1,&iServerName,
							 szServerDesc,sizeof(szServerDesc)-1,&iServerDesc);
	do 
	{
//		win_infoarg("[%s]",szServerDesc);
		if (!strCaseStr(szServerDesc,lpDriver)) continue;
		ARAdd(&ar,szServerName);
	} while (SQLDataSources(hEnv,SQL_FETCH_NEXT,
							szServerName,
							sizeof(szServerName)-1,&iServerName,
							szServerDesc,
							sizeof(szServerDesc)-1,&iServerDesc)==SQL_SUCCESS);
	SQLFreeHandle(SQL_HANDLE_ENV,hEnv); 
#ifndef EH_CONSOLE
	MouseCursorDefault();
#endif
	ARSort(ar,0);
	//win_close();
	return ar;
}

void odbc_showDict(EH_ODBC_RS pRes) {
	
	INT i;

	for (i=0; i<pRes->iFieldNum; i++) {

		printf("- %s " CRLF,pRes->arDict[i].szName);

		//if (!strCaseCmp(pRes->arDict[i].szName,lpField)) return i;
	}

}

// --------------------------------------------------------------------------------
// ODBCTableList
// Crea una lista delle tabelle di un dbase
// --------------------------------------------------------------------------------
EH_AR odbcTableList(EH_ODBC_SECTION * psOdbcSection,CHAR * pszSchema,CHAR *lpName,CHAR *lpType)
{
	
	EH_AR ar=NULL;
	EH_ODBC_RS pRes;

	if (!psOdbcSection->hStmt) return NULL;

#ifndef EH_CONSOLE
	MouseCursorWait();
#endif

	if (strEmpty(pszSchema)) pszSchema=NULL;
	if (strEmpty(lpName)) lpName=NULL;
	ar=ARNew();
	printf("- [%s][%s]" CRLF,pszSchema,lpName);
	psOdbcSection->sqlLastError=SQLTables(psOdbcSection->hStmt, 
								  NULL, 0, // Catalogo
								  //"",SQL_NTS,
								  pszSchema, !strEmpty(pszSchema)?SQL_NTS:0, //Schema
								  lpName, !strEmpty(lpName)?SQL_NTS:0, // Nome della tabella
								  lpType, !strEmpty(lpType)?SQL_NTS:0); // Tipo della tabella

	if (psOdbcSection->sqlLastError!=SQL_SUCCESS) 
	{
		//if (!EhOdbc.fNoErrorView) odbc_error("ODBCTableList()::SQLTables()"); 
		return ar;
	}
	pRes=odbcQueryResultCreate(psOdbcSection,300);
	// odbc_showDict(pRes);
	while (odbc_fetch_row(pRes))
	{
		if (!strcmp(odbc_fldptr(pRes,"TABLE_TYPE"),lpType))
			{	
				if (!strEmpty(odbc_fldptr(pRes,"TABLE_SCHEM"))) 
					ARAddarg(&ar,"%s.%s",odbc_fldptr(pRes,"TABLE_SCHEM"),odbc_fldptr(pRes,"TABLE_NAME"));
					else
					ARAddarg(&ar,"%s",odbc_fldptr(pRes,"TABLE_NAME"));

			}
	}
	odbc_free_result(pRes);
	
#ifndef EH_CONSOLE
	MouseCursorDefault(); 
#endif
	return ar;
} 
 


// --------------------------------------------------------------------------------
// ODBCTableList
// Crea una lista delle tabelle di un dbase
// --------------------------------------------------------------------------------
CHAR * odbcTableInfo(EH_ODBC_SECTION * psOdbcSection,CHAR * lpLibrary, CHAR * lpName, CHAR *lpType)
{
	
	EH_AR ar;

	ar=odbcTableList(psOdbcSection,lpLibrary,lpName,lpType);
	ARPrint(ar);
	ARDestroy(ar);
	return NULL;
/*
	EH_ODBC_RS rsSet;
	if (!sOdbcSection.hStmt) return NULL;

	sOdbcSection.sqlLastError=SQLTables(sOdbcSection.hStmt, 
								  NULL, 0, // Catalogo
								  //"",SQL_NTS,
								  lpLibrary, lpLibrary?SQL_NTS:0, //Schema
								  lpName, (lpName?SQL_NTS:0), // Nome della tabella
								  lpType, lpType?SQL_NTS:0); // Tipo della tabella

	if (sOdbcSection.sqlLastError!=SQL_SUCCESS) 
	{//if (!EhOdbc.fNoErrorView) ODBCError("ODBCTableList()::SQLTables()"); 
		return NULL;
	}

	//odbc_queryarg(
	// ODBCDoDictionary(); // Creo il dizionario del risultato
	rsSet=odbc_store_result(100);
	if (rsSet)
	{
		while (odbc_fetch_row(rsSet))
		{  
			INT a;

			for (a=0;a<rsSet->iFieldNum;a++) {
				printf("%s=%s" CRLF,rsSet->arDict[a].szName,odbc_fldptr(rsSet,rsSet->arDict[a].szName));
			}
		} 
		odbc_free_result(rsSet);
	}

	return NULL;
	*/
}
