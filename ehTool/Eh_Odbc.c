//   +-------------------------------------------+
//   | ODBC
//   | Interfaccia al dbase ODBC
//   |             
//   |							Ferrà srl 04/2008
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/eh_odbc.h"
#define SQL_BUFFER_QUERY_DEFAULT 8192

//
// Macro senza support MultiThread
//
#ifndef EH_ODBC_MT
EH_ODBC_SECTION sOdbcSection={NULL,0};
#else
#define MAX_MTSECTION 64
EH_ODBC_SECTION arMysSection[MAX_MTSECTION];
static CRITICAL_SECTION csODBCSection;
#endif

//
// odbc_start()
//
void odbc_start(SINT cmd)
{
	if (cmd!=WS_LAST) return;

#ifndef EH_ODBC_MT
	odbc_CreateSection(&sOdbcSection);
#else
	ZeroFill(arMysSection);
	InitializeCriticalSection(&csODBCSection);
//	ODBC_thread_init();
#endif
}

//
// odbc_end()
//
void odbc_end(void)
{
#ifndef EH_ODBC_MT
	odbc_DestroySection(&sOdbcSection);
#else
	odbc_MTCloseAllSection();
	DeleteCriticalSection(&csODBCSection);
	ODBC_library_end();
#endif
}	


//
// odbc_CreateSection()
//
void odbc_CreateSection(EH_ODBC_SECTION *pOdbcSection)
{
	SQLRETURN sRet;
	memset(pOdbcSection,0,sizeof(EH_ODBC_SECTION));
	pOdbcSection->lpQuery=EhAlloc(SQL_BUFFER_QUERY_DEFAULT);
	pOdbcSection->fDebug=FALSE;
	pOdbcSection->dwQueryCounter=0;
	//pOdbcSection->iRowsLimit=25;

	// Alloca Ambiente ODBC
	sRet=SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &pOdbcSection->hEnv);
	sRet=SQLSetEnvAttr(pOdbcSection->hEnv, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0);
	sRet=SQLAllocHandle(SQL_HANDLE_DBC, pOdbcSection->hEnv, &pOdbcSection->hConn);
}

//
// odbc_DestroySection()
//
void odbc_DestroySection(EH_ODBC_SECTION *pOdbcSection)
{
	// QueryResultFree(pOdbcSection);

	ehFree(pOdbcSection->lpQuery);
	EhFreeNN(pOdbcSection->pSqlHost);
	EhFreeNN(pOdbcSection->pSqlUser);
	EhFreeNN(pOdbcSection->pSqlPass);
	EhFreeNN(pOdbcSection->pSqlSchema);
	EhFreeNN(pOdbcSection->pLastQuery);

//	if (sEmu.lpRows) {ehFree(sEmu.lpRows); sEmu.lpRows=NULL;}
	if (pOdbcSection->hConn)	SQLFreeHandle(SQL_HANDLE_DBC,pOdbcSection->hConn); 
	if (pOdbcSection->hEnv)		SQLFreeHandle(SQL_HANDLE_ENV,pOdbcSection->hEnv);
	if (pOdbcSection->hStmt)	SQLFreeHandle(SQL_HANDLE_ENV,pOdbcSection->hStmt);
	memset(pOdbcSection,0,sizeof(EH_ODBC_SECTION));
}

static void OdbcWindow(CHAR *Testo)
{
	SINT x;
	SINT size;
	SINT cBack=RGB(120,120,140);
	CHAR *lpFont="#Arial";
	SINT iSize=24;
	RECT rcBox,rcGradient;
	SINT yHeight;
	if (!Testo) {win_close(); return;}

	size=font_lenf(Testo,lpFont,iSize,STYLE_BOLD);
	x=(sys.video_x-(size+20))/2;
	yHeight=35;
	win_openEx(x,10,"",size+20,yHeight,cBack,0,0,WS_POPUP|WS_BORDER|WS_VISIBLE,TRUE,NULL);

	rcBox.left=0; rcBox.top=0;	
	rcBox.right=size+20; rcBox.bottom=46;
	memcpy(&rcGradient,&rcBox,sizeof(RECT));
	rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;
	// BoxGradient(&rcBox,&rcGradient,0,AlphaRGB(255,0,2,80));//AlphaColor(255,RGB(80,0,0)));

//	dispfm(10,8,0,-1,STYLE_BOLD,lpFont,iSize,Testo);
	dispfm(10,6,15,-1,STYLE_BOLD,lpFont,iSize,Testo);

	// Gradiente Bianco
	rcBox.left=1; rcBox.top=1;	rcBox.right=size+20-2; rcBox.bottom=yHeight/2;
	memcpy(&rcGradient,&rcBox,sizeof(RECT));
	rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;
	BoxGradient(&rcBox,&rcGradient,0,AlphaRGB(40,255,255,255));//AlphaColor(255,RGB(80,0,0)));
}

//
// odbc_Connect()
//
void *odbc_Connect(DYN_SECTION_FUNC CHAR *pSQLHostName,
				   CHAR *pSQLUser,
				   CHAR *pSQLPassword,
				   CHAR *pDbaseSchema,
				   ULONG client_flag,
				   BOOL bShowWindow)
{
	DYN_SECTION_GLOBALPTR
	SQLRETURN sqlReturn;

#if !defined(_NODC)&&!defined(EH_CONSOLE)
	if (bShowWindow)
	{
		CHAR szServ[800];
		sprintf(szServ,"Connessione SQL a %s ...",pSQLHostName);
		OdbcWindow(szServ);
	}
#endif

	//MYSQL *mys;
	pOdbcSection->pSqlHost=StringAlloc(pSQLHostName);
	pOdbcSection->pSqlUser=StringAlloc(pSQLUser);
	pOdbcSection->pSqlPass=StringAlloc(pSQLPassword);
	pOdbcSection->pSqlSchema=StringAlloc(pDbaseSchema);
	pOdbcSection->iSqlClientFlag=client_flag;

	// --------------------------------------------------------------------
	// Mi connetto al server
	//

	pOdbcSection->sqlLastError=SQLConnect(	pOdbcSection->hConn,				// Handle della connessione
											pOdbcSection->pSqlHost,		// Nome del server
											SQL_NTS,						// Nome del file / Driver da usare
											pOdbcSection->pSqlUser,SQL_NTS,  // UserName
											pOdbcSection->pSqlPass,SQL_NTS); // Password

	if (pOdbcSection->sqlLastError!=SQL_SUCCESS&&
		pOdbcSection->sqlLastError!=SQL_SUCCESS_WITH_INFO) 
		{
			 // if (!pOdbcSection->bNoErrorView) odbc_error("Connect"); 

#if !defined(_NODC)&&!defined(EH_CONSOLE)
			if (bShowWindow)
			{
				OdbcWindow(NULL);
			}
#endif
			return NULL;
		}

	// win_infoarg("%d",pOdbcSection->sqlLastError);
#if !defined(_NODC)&&!defined(EH_CONSOLE)
	if (bShowWindow) OdbcWindow(NULL);
#endif	

	// Creo l'handle di stantment
	sqlReturn=SQLAllocHandle(SQL_HANDLE_STMT, pOdbcSection->hConn, &pOdbcSection->hStmt);
	// sqlReturn=SQLSetStmtAttr(pOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 10, 0); // Dimensione delle righe nel buffer

//  Leggo ASYNC
	sqlReturn=SQLGetInfo(pOdbcSection->hConn,SQL_ASYNC_MODE,(SQLPOINTER) &pOdbcSection->uiAsync,sizeof(pOdbcSection->uiAsync),0);
	sqlReturn=SQLGetInfo(pOdbcSection->hConn,SQL_SCROLL_OPTIONS,(SQLPOINTER) &pOdbcSection->uiScroll,sizeof(pOdbcSection->uiScroll),0);
	//sqlReturn=SQLGetInfo(pOdbcSection->hConn,SQL_DYNAMIC_CURSOR_ATTRIBUTES1,(SQLPOINTER) &pOdbcSection->uiDynamic,sizeof(pOdbcSection->uiDynamic),0);

	return pOdbcSection->hConn;
}


//
// odbc_error()
//
/*
void odbc_error(DYN_SECTION_FUN) 
{s
	DYN_SECTION_GLOBALPTR
	ehExit("%s\n%s",pOdbcSection->lpQuery,ODBC_error(pOdbcSection->mySql));
}
*/

// 
// odbc_error()
//
void odbc_error(DYN_SECTION_FUNC CHAR *pWho) 
{
	DYN_SECTION_GLOBALPTR
	
	CHAR *lpR="";
	SQLCHAR     szBuffer[SQL_MAX_MESSAGE_LENGTH + 1];
	SQLCHAR     sqlstate[SQL_SQLSTATE_SIZE + 1];
	SQLINTEGER  sqlcode;
	SQLSMALLINT length;

	switch (pOdbcSection->sqlLastError)
	{
	case SQL_SUCCESS: lpR="Success"; break;
	case SQL_SUCCESS_WITH_INFO: lpR="Success+Info"; break;
	case SQL_ERROR: lpR="Errore"; break;
	case SQL_INVALID_HANDLE: lpR="InvalidHandle"; break;
	}
  
	//EhConWrite("Query:%s" CRLF,pOdbcSection->pLastQuery);
	while ( SQLError(pOdbcSection->hEnv, 
					 pOdbcSection->hConn, 
					 pOdbcSection->hStmt, 
					 sqlstate, 
					 &sqlcode, 
					 szBuffer,
                     SQL_MAX_MESSAGE_LENGTH + 1, 
					 &length) == SQL_SUCCESS )
    {
        EhConWrite("\n **** ERROR *****\n");
        EhConWrite("         SQLSTATE: %s\n", sqlstate);
        EhConWrite("Native Error Code: %ld\n", sqlcode);
        EhConWrite("%s \n", szBuffer);
		win_infoarg("%s",szBuffer);
    };
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

static void OdbcTry(CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult) 
{
	DYN_SECTION_GLOBALPTR
   SQLRETURN rc = iResult; 
   pOdbcSection->sqlLastError=iResult;
   if (rc != SQL_SUCCESS) 
      { 
		 if (rc != SQL_SUCCESS_WITH_INFO) 
		 {
			// ehExit(SQLDisplay(WhoIs,hstmt,rc));
#ifdef _DEBUG
			 dispx(SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc)); 
#endif
//			 LOGWrite(SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
			 //efx2(); efx2(); efx2(); 
			 //Sleep(2000);
		 }
      } 

}

/*
const char *odbc_GetError(DYN_SECTION_FUN) 
{ 
	DYN_SECTION_GLOBALPTR 
	return odbc_error(NULL);//pOdbcSection->mySql);
}
*/

//
// Conta i record secondo la query
//
SINT odbc_count(DYN_SECTION_FUNC CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	EH_ODBC_RS pRes;
	SINT iRecNum=-1;
	va_list Ah;
	CHAR *p,*lpQueryMess=EhAlloc(1024);
	CHAR *lpQueryCommand=EhAlloc(SQL_BUFFER_QUERY_DEFAULT);

	sprintf(lpQueryMess,"SELECT COUNT(*) FROM %s",Mess);
	va_start(Ah,Mess);
	vsprintf(lpQueryCommand,lpQueryMess,Ah); // Messaggio finale
	va_end(Ah);

	pOdbcSection->dwQueryCounter++;

    odbc_queryarg(FALSE,"%s",lpQueryCommand);
	pRes=odbc_store_result(1); 
	if (pRes)
	{
		odbc_fetch_row(pRes);
		p=odbc_fldptr(pRes,"1"); if (p) iRecNum=atoi(p);
		odbc_free_result(pRes); // Libero le risorse
	}
	else
	{
		odbc_error(__FUNCTION__);
		iRecNum=-1;
	}

/*
	if (pOdbcSection->fCursorOpen) {SQLCloseCursor(pOdbcSection->hStmt); pOdbcSection->fCursorOpen=FALSE;} // Apro il cursore
	SQLBindCol(pOdbcSection->hStmt, 1, SQL_INTEGER, &iRecNum, 0, 0); // Bind dell'informazione
	SQLSetStmtAttr(pOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0); // Numero di righe
	
	strcpy(pOdbcSection->lpQuery,lpQueryCommand);
	pOdbcSection->sqlLastError=SQLExecDirect(pOdbcSection->hStmt,lpQueryCommand,SQL_NTS); // Query
	if (pOdbcSection->sqlLastError<0) 
	{
		odbc_error(__FUNCTION__);
		iRecNum=-1;
	}
	else
	{
		SQLFetch(pOdbcSection->hStmt); // Leggo il Fatch
	}
	SQLCloseCursor(pOdbcSection->hStmt); // CHiudo il cursore
*/	

    ehFree(lpQueryCommand);
	ehFree(lpQueryMess); 
	return iRecNum;
}

//
// odbc_sum()
// Somma il valore di un campo
//
SINT odbc_sum(DYN_SECTION_FUNC CHAR *lpField,CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	/*
	SINT iRecNum=-1;
	va_list Ah;
	CHAR *lpQueryMess=EhAlloc(1024);
	CHAR *lpQueryCommand=EhAlloc(SQL_BUFFER_QUERY_DEFAULT);

	sprintf(lpQueryMess,"SELECT SUM(%s) FROM %s",lpField,Mess);
	va_start(Ah,Mess);
	vsprintf(lpQueryCommand,lpQueryMess,Ah); // Messaggio finale
	va_end(Ah);

	pOdbcSection->dwQueryCounter++;

	if (pOdbcSection->fCursorOpen) {SQLCloseCursor(pOdbcSection->hStmt); pOdbcSection->fCursorOpen=FALSE;} // Apro il cursore
	SQLBindCol(pOdbcSection->hStmt, 1, SQL_INTEGER, &iRecNum, 0, 0); // Bind dell'informazione
	SQLSetStmtAttr(pOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0); // Numero di righe
	
	pOdbcSection->sqlLastError=SQLExecDirect(pOdbcSection->hStmt,lpQueryCommand,SQL_NTS); // Query
	if (pOdbcSection->sqlLastError<0) 
	{
		odbc_error(__FUNCTION__);
		iRecNum=-1;
	}
	else
	{
		SQLFetch(pOdbcSection->hStmt); // Leggo il Fatch
	}
	SQLCloseCursor(pOdbcSection->hStmt); // CHiudo il cursore

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
int odbc_query(DYN_SECTION_FUNC BOOL fNoErrorStop,CHAR *pQuery)
{
	DYN_SECTION_GLOBALPTR
	int err=0;
	BYTE *pRealQuery;
	pOdbcSection->dwQueryCounter++;

	if (strstr(pQuery,"{LIB}"))
	{
		pRealQuery=EhAlloc(strlen(pQuery)+1024);
		strcpy(pRealQuery,pQuery);
		while (StrReplace(pRealQuery,"{LIB}",pOdbcSection->pSqlSchema));
	}
	else
	{
		pRealQuery=StringAlloc(pQuery);
	}

#ifdef EH_CONSOLE
	if (pOdbcSection->fDebug) {printf("%s" CRLF,pRealQuery);}
#else
	if (pOdbcSection->fDebug) {fprintf(stderr,"%s" CRLF,pRealQuery);}
#endif

	EhFreeNN(pOdbcSection->pLastQuery);
	pOdbcSection->pLastQuery=StringAlloc(pRealQuery);
	pOdbcSection->sqlLastError=SQLExecDirect(	pOdbcSection->hStmt,
												pRealQuery,
												SQL_NTS); // Query

	ehFree(pRealQuery);
	if (pOdbcSection->sqlLastError<0) 
	{
		//win_infoarg("qui: %d",	pOdbcSection->sqlLastError);
		odbc_error(__FUNCTION__);
		err=TRUE;
	}
	return err;
}


//
// odbc_queryarg()
// ritorna TRUE se c'è errore
//
int odbc_queryarg(DYN_SECTION_FUNC BOOL fNoErrorStop,CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(pOdbcSection->lpQuery,Mess,Ah); // Messaggio finale
	va_end(Ah);

	return odbc_query(DYN_SECTIONC fNoErrorStop,pOdbcSection->lpQuery);
}


//
// odbc_affected_rows()
//
int odbc_affected_rows(DYN_SECTION_FUN) 
{
	DYN_SECTION_GLOBALPTR
	SQLINTEGER sqlInteger;
	SQLRowCount(pOdbcSection->hStmt,&sqlInteger);
	return sqlInteger;
}


int odbc_queryargEx(DYN_SECTION_FUNC BOOL fNoErrorStop,CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	int err;
	CHAR *lpBuf=EhAlloc(SQL_BUFFER_QUERY_DEFAULT);

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	va_end(Ah);

	if (pOdbcSection->fDebug) {printf("%s",lpBuf); printf(CRLF);}
	err=odbc_query(DYN_SECTIONC fNoErrorStop,lpBuf);
	ehFree(lpBuf);
	return err;
}

int odbc_queryargExBig(DYN_SECTION_FUNC BOOL fNoErrorStop,DWORD dwSizeMemory,CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	int err;
	CHAR *lpBuf=EhAlloc(dwSizeMemory);

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	va_end(Ah);

	if (pOdbcSection->fDebug) {printf("%s",lpBuf); printf(CRLF);}
	err=odbc_query(DYN_SECTIONC fNoErrorStop,lpBuf);
	ehFree(lpBuf);
	return err;
}


//
// odbc_store_result()
//
EH_ODBC_RS odbc_store_result(DYN_SECTION_FUNC SINT iLimit)
{
	DYN_SECTION_GLOBALPTR
	EH_ODBC_RS pRes;

	// Trovo dizionario e creo buffer
	if (pOdbcSection->sqlLastError!=SQL_SUCCESS&&
		pOdbcSection->sqlLastError!=SQL_SUCCESS_WITH_INFO) 	{return NULL;} // Precedente errore in query

	pRes=odbc_QueryResultCreate(pOdbcSection->hStmt,iLimit); 
	SQLAllocHandle(SQL_HANDLE_STMT, pOdbcSection->hConn, &pOdbcSection->hStmt);
	return pRes;
}


//
// odbc_fetch_row()
// Avanzo di una riga
//
BOOL odbc_fetch_row(DYN_SECTION_FUNC EH_ODBC_RS pRes) 
{
	DYN_SECTION_GLOBALPTR
	SQLRETURN sqlReturn;
	
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
		pRes->iCurrentRow++; if ((pRes->iCurrentRow-pRes->iOffset)<pRes->iRowsReady) return TRUE;
		pRes->iOffset+=pRes->iRowsReady;
	}

	sqlReturn=SQLFetch(pRes->hStmt);
	if (sqlReturn==SQL_NO_DATA_FOUND) return FALSE;
	if (sqlReturn==SQL_SUCCESS||sqlReturn==SQL_SUCCESS_WITH_INFO) return TRUE;
	//ehExit(__FUNCTION__": error %d",sqlReturn);

	return TRUE;
}

//
// odbc_fetch_first()
//
BOOL odbc_fetch_first(DYN_SECTION_FUNC EH_ODBC_RS pRes) 
{
	DYN_SECTION_GLOBALPTR
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
void odbc_free_result(DYN_SECTION_FUNC EH_ODBC_RS pRes) 
{
	DYN_SECTION_GLOBALPTR

	odbc_QueryResultDestroy(pRes);
	if (pRes->hStmt) SQLFreeHandle(SQL_HANDLE_ENV,pRes->hStmt);	
	ehFree(pRes);
}

// odbc_f MySqlField
// Operazione su i campi di una query
// odbc_ffind() ricerca
int odbc_fldfind(DYN_SECTION_FUNC EH_ODBC_RS pRes,CHAR *lpField,BOOL bNoError) 

{
	DYN_SECTION_GLOBALPTR
	SINT i;
	for (i=0; i<pRes->iFieldNum; i++)
	{
		if (!_stricmp(pRes->arDict[i].szName,lpField)) return i;
	}
	if (atoi(lpField)>0) return atoi(lpField)-1; // Numero di colonna
	if (!bNoError) ehExit("campo inesistente [%s]",lpField); 
	return -1;
}

// myf_ptr() Puntatore a

//
// odbc_fptr()
//
char *odbc_fldptr(DYN_SECTION_FUNC EH_ODBC_RS pRes, CHAR *lpField) 
{
	DYN_SECTION_GLOBALPTR
	SINT iField=odbc_fldfind(DYN_SECTIONC pRes,lpField,FALSE); //if (i<0) return NULL;
	SINT iRow=pRes->iCurrentRow-pRes->iOffset;
	if (iRow<0||iRow>pRes->iRowsLimit)
	{
		EhConWrite("odbc_fldptr() error : %d,%d" CRLF,iRow,pRes->iRowsLimit);
		return NULL;
	}
	iRow*=pRes->arDict[iField].iSize;
	return pRes->arDict[iField].lpBuffer+iRow; // DA VEDERE
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

#ifdef _ODBC_MT

//
// odbc_MTSectionId()
//
SINT odbc_MTSectionId(DWORD idThread,BOOL bCritical)
{
	SINT a,idx=-1;
	if (bCritical) EnterCriticalSection(&csODBCSection);
	for (a=0;a<MAX_MTSECTION;a++)
	{
		if (arMysSection[a].idThread==idThread) {idx=a; break;}
	}
	if (bCritical) LeaveCriticalSection(&csODBCSection);
	return idx;
}


//
// odbc_MTOpenSection()
//
EH_ODBC_SECTION *odbc_MTOpenSection(DWORD idThread)
{
	SINT idx;
	EnterCriticalSection(&csODBCSection);

		idx=odbc_MTSectionId(0,FALSE); // Trovo una sezione libera
		if (idx<0) {LeaveCriticalSection(&csODBCSection); ehExit("odbc_MTOpenSection(): full");}

	//	EhLogWrite("Open: %d = %d",idThread,idx);
		odbc_CreateSection(&arMysSection[idx]);
		arMysSection[idx].idThread=idThread;

	LeaveCriticalSection(&csODBCSection);

	return &arMysSection[idx];
}

//
// odbc_MTGetSection()
//
EH_ODBC_SECTION *odbc_MTGetSection(DWORD idThread)
{
	SINT idx=odbc_MTSectionId(idThread,TRUE);
	if (idx<0) 
	{
		EhLogWrite("odbc_MTGetSection(): #%d ?",idThread); return NULL;
	}
	return &arMysSection[idx];
}

//
// odbc_MTCloseSection()
// Ritorna true se la sezione non esiste
//
BOOL odbc_MTCloseSection(DWORD idThread)
{
	SINT idx=odbc_MTSectionId(idThread,TRUE);
	if (idx<0) 
	{
		EhLogWrite("odbc_MTCloseSection(): #%d ?",idThread);
		return TRUE;
	}
	EnterCriticalSection(&csODBCSection);
	odbc_DestroySection(&arMysSection[idx]);
	LeaveCriticalSection(&csODBCSection);
	return FALSE;
}

//
// odbc_MTCloseAllSection()
//
void odbc_MTCloseAllSection(void)
{
	SINT a;
//	EhLogWrite("odbc_MTCloseAllSection");
	EnterCriticalSection(&csODBCSection);
	for (a=0;a<MAX_MTSECTION;a++)
	{
		if (arMysSection[a].idThread) 
		{	EhLogWrite("MTSectionFree: #%d (Query servite:%d)",arMysSection[a].idThread,arMysSection[a].dwQueryCounter);
			odbc_DestroySection(&arMysSection[a]);
		}
	}
	LeaveCriticalSection(&csODBCSection);
}

//
// odbc_MTGetThreads()
//
CHAR *odbc_MTGetThreads(void)
{
	SINT a;
	CHAR *lpStatus=EhAlloc(32000); *lpStatus=0;
	EnterCriticalSection(&csODBCSection);
	for (a=0;a<MAX_MTSECTION;a++)
	{
		if (arMysSection[a].idThread) 
		{	
			if (*lpStatus) strcat(lpStatus,",");
			sprintf(lpStatus+strlen(lpStatus),"%d",arMysSection[a].idThread);
		}
	}
	LeaveCriticalSection(&csODBCSection);
	return lpStatus;
}

#endif

//
//	QueryResultAllocation()
//
EH_ODBC_RS  odbc_QueryResultCreate(SQLHSTMT hStmt,SINT iLimit)
{
	 SQLCHAR szNameColumn[255];
	 SQLSMALLINT NameLenght;
	 SQLSMALLINT DataType;
	 SQLUINTEGER ColumnSize;
	 SQLSMALLINT DecimnaDigits;
	 SQLSMALLINT Nullable;
	 CHAR *lpType;
	 SINT iCount;
	 SINT iRowMemory;
	 SINT i,iTypeC;
	 SINT iAdbType;

	 EH_ODBC_RS pRes=NULL;

	 //
	 // Libera le risorse precedentemente impegnate
	 //
	 // odbc_QueryResultDestroy(pRes);

	 // Sposto l'handle di stantment nella Result ed alloco un nuovo stantment pubblico
	 pRes=EhAllocZero(sizeof(EH_ODBC_RESULTSET));  // Mi rimane allocata <-----------
	 pRes->hStmt=hStmt;
	 pRes->iRowsLimit=iLimit;

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
				         &ColumnSize,&DecimnaDigits,
				         &Nullable)!=SQL_SUCCESS) break;
	  iCount++;
	 }

	 //
	 // Alloco memoria per descrizione Campi
	 //
	 pRes->iFieldNum=iCount;
	 pRes->arDict=EhAllocZero(sizeof(EH_SQL_FIELD)*iCount);

	 // Alloco e binderizzo i campi
	 for (i=0;i<pRes->iFieldNum;i++)
	 {
	  if (SQLDescribeCol(pRes->hStmt, // Handle del file
					     (SQLSMALLINT) (i+1), // Numero della colonna
					     szNameColumn,
						 (SQLSMALLINT) sizeof(szNameColumn),
						 &NameLenght,
						 &DataType,
				         &ColumnSize,
						 &DecimnaDigits,
				         &Nullable)!=SQL_SUCCESS) break;
	
	  lpType="<?>"; iTypeC=SQL_C_CHAR; iAdbType=ADB_ALFA;
	  switch (DataType)
	  {
			case SQL_CHAR:     lpType="ALFAF";    iTypeC=SQL_C_CHAR; break;
			case SQL_VARCHAR:  lpType="ALFA";     iTypeC=SQL_C_CHAR; break;
			case SQL_DECIMAL:  lpType="DECIMAL";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
			case SQL_NUMERIC:  lpType="NUMERIC";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
			case SQL_SMALLINT: lpType="SMALLINT"; iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
			case SQL_INTEGER:  lpType="INTEGER";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
			case SQL_REAL:	   lpType="REAL";     iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
			case SQL_FLOAT:	   lpType="FLOAT";    iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
			case SQL_DOUBLE:   lpType="DOUBLE";   iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
			case SQL_BIT:	   lpType="BIT";      iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	  }

	  ColumnSize++; // Aumento di uno la grandezza della colonna

	  strcpy(pRes->arDict[i].szName,szNameColumn);
	  strcpy(pRes->arDict[i].szType,lpType);
      pRes->arDict[i].iType=DataType;
	  pRes->arDict[i].iAdbType=iAdbType;
	  pRes->arDict[i].iSize=ColumnSize;
	  pRes->arDict[i].lpBuffer=EhAllocZero(ColumnSize*pRes->iRowsLimit);
	  // fprintf(stderr,"%d) pRes->arDict[i].lpBuffer=%x" CRLF,i,pRes->arDict[i].lpBuffer);

	  if (SQLBindCol(pRes->hStmt, 
					(SQLSMALLINT) (i+1), // Colonna
					(SQLSMALLINT) iTypeC, 
					(PTR) pRes->arDict[i].lpBuffer, // Buffer
					pRes->arDict[i].iSize, 0)) // Dimensioni
					{ehExit("ODBC: BINDErrore in [%s]",pRes->arDict[i].szName);}
	 }

	//EhConWrite("odbc_QueryResultCreate" CRLF);
	return pRes;
}


void odbc_QueryResultDestroy(EH_ODBC_RS pRes)
{
	SINT i;
	
	if (!pRes) return;
	if (!pRes->arDict) return;

	// Libero risorse precedentemente impegnate
	for (i=0;i<pRes->iFieldNum;i++)
	{
		EhFreePtr(&pRes->arDict[i].lpBuffer);  
	}
	EhFreePtr(&pRes->arDict);
	//EhConWrite("odbc_QueryResultDestroy" CRLF);
}



/*
	sRet=SQLSetStmtAttr(pOdbcSection->hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) pOdbcSection->iRowsSize, 0); // Dimensione delle righe nel buffer
	sRet=SQLSetStmtAttr(pOdbcSection->hStmt, SQL_ATTR_ROWS_FETCHED_PTR, &pOdbcSection->iRows, 0); // Righe realmente lette
*/

//
//  odbc_QueryLastError()
//
CHAR *odbc_QueryLastError(DYN_SECTION_FUNC CHAR *lpQuery)
{
	DYN_SECTION_GLOBALPTR
	static CHAR szError[8000];
	strcpy(szError,"?");
	//sprintf(szError,"Error:%d - %s" CRLF "%s",ODBC_errno(pOdbcSection->mySql),ODBC_error(pOdbcSection->mySql),lpQuery);
	return szError;
}

//
// odbc_querytoarray()
// Ritorna il valore di un campo solo da una query (es DESCRIZIONE di un codice)
//
// deve essere liberato con ARDestroy()
EH_AR odbc_querytoarray(DYN_SECTION_FUNC CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	EH_ODBC_RS pRes;
	EH_AR arRet=NULL;
	SINT i;
	CHAR szServ[10];

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(pOdbcSection->lpQuery,Mess,Ah); // Messaggio finale
	va_end(Ah);

	if (odbc_query(DYN_SECTIONC TRUE,pOdbcSection->lpQuery)) return NULL;

	pRes=odbc_store_result(1); // Richiedo il risultato
	if (pRes)
	{
		while (odbc_fetch_row(pRes)) 
		{
			arRet=EhAlloc(sizeof(CHAR *)*(pRes->iFieldNum+1));
			for (i=0;i<pRes->iFieldNum;i++)
			{
				sprintf(szServ,"%d",(i+1)); arRet[i]=StringAlloc(odbc_fldptr(pRes,szServ)); // Poco elegante ma funzionale ...
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
EH_ODBC_RS odbc_queryrow(DYN_SECTION_FUNC CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	EH_ODBC_RS pRes;
	//SINT i;

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(pOdbcSection->lpQuery,Mess,Ah); // Messaggio finale
	va_end(Ah);

	if (odbc_query(DYN_SECTIONC TRUE,pOdbcSection->lpQuery)) return NULL;
	pRes=odbc_store_result(1); // Richiedo il risultato
	if (pRes)
	{
		if (odbc_fetch_row(pRes)) return pRes;

		// Libero le risorse
		odbc_free_result(pRes);
	}
	return NULL;
}

//
// odbc_hookget()
// ritorna TRUE se c'è errore o il record non c'è
//
int odbc_hookget(DYN_SECTION_FUNC EH_TABLEHOOK *arHook,CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	EH_ODBC_RS pRes;
	SINT a;
	BYTE *pFieldName,*pFieldValue;
	SINT iSpiaz;
	SINT idxField;
	CHAR szObjName[20];
	SINT iValue;
	BOOL bError=TRUE;

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(pOdbcSection->lpQuery,Mess,Ah); // Messaggio finale
	va_end(Ah);

	if (odbc_query(DYN_SECTIONC TRUE,pOdbcSection->lpQuery)) return TRUE;

	pRes=odbc_store_result(1); // Richiedo il risultato
	if (pRes)
	{
		while (odbc_fetch_row(pRes)) 
		{
			bError=FALSE;
			for (a=0;arHook[a].pTableFieldName;a++)
			{
				pFieldName=arHook[a].pTableFieldName;
				if (*pFieldName=='#') pFieldName++;// Solo in lettura incremento pt di uno
				pFieldValue=odbc_fldptr(pRes,pFieldName);
				if (pFieldValue==NULL)
				{
					win_infoarg(__FUNCTION__":NomeField ? \n[%s] in Query[%s]",arHook[a].pTableFieldName,pOdbcSection->lpQuery);
					continue;
				}

				idxField=odbc_fldfind(pRes,pFieldName,FALSE);


				if (arHook[a].iType==BIND)
				{
					strcpy(arHook[a].pObjName,odbc_fldptr(pRes,pFieldName));
					continue;
				}

				strcpy(szObjName,arHook[a].pObjName);
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
					SINT iObjTipo=obj_type(szObjName);
					if (iObjTipo<0) win_errgrave(__FUNCTION__":ObjName in Hook errato");
					switch (arHook[a].iType)
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

						case NUME:

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
					SINT iIptNumber=arHook[a].iInput;
					BYTE *pData;
					if ((iIptNumber<0)||(iIptNumber>IPT_info[IPT_ult].numcampi-1)) win_errgrave(__FUNCTION__":IptNum in Hook errato");

					switch (arHook[a].iType)
					{
						 case NUME : 
								ipt_writevedi(iIptNumber,NULL,atof(pFieldValue+iSpiaz));
								break;
						 
						 case DATA:
								pData=ChrTrim(pFieldValue+iSpiaz);
								// win_infoarg("[%s]",pData);
								if (*pData) ipt_writevedi(iIptNumber,daAnno(pData),0); else ipt_writevedi(iIptNumber,"",0);
								//mask=IPT_info[IPT_ult].mask;
								break;

						 default:
						 case ALFA :
								ipt_writevedi(iIptNumber,pFieldValue+iSpiaz,0);
								//mask=IPT_info[IPT_ult].mask;
								break;
					}
				}
			}
		} 
		odbc_free_result(pRes); // Libero le risorse
	}

	return bError;
}

//
// odbc_hookinsert()
// ritorna TRUE se c'è errore
//
int odbc_hookinsert(DYN_SECTION_FUNC EH_TABLEHOOK *arHook,CHAR *pQuery,...)
{
	DYN_SECTION_GLOBALPTR
	CHAR *pSqlCommand=EhAlloc(64000);
	CHAR *pFieldList=EhAlloc(8000);
	CHAR *pValueList=EhAlloc(64000);
	CHAR *pValue=EhAlloc(64000);

	SINT a;
	BYTE *p,*pFieldName;
	BOOL bError=TRUE;

	va_list Ah;
	va_start(Ah,pQuery);
	vsprintf(pSqlCommand,pQuery,Ah); // Messaggio finale
	va_end(Ah);

	// 
	// Cerco il Campo FIELDS
	//
	*pFieldList=0; *pValueList=0;
	for (a=0;arHook[a].pTableFieldName;a++)
	{
		pFieldName=arHook[a].pTableFieldName; if (*pFieldName=='#') continue;
		if (arHook[a].iType==BIND) continue;

		// FIELDS
		if (*pFieldList) strcat(pFieldList,",");
		strcat(pFieldList,arHook[a].pTableFieldName);
		
		//
		// OGGETTI
		//
		if (*arHook[a].pObjName)
		{
			SINT iObjTipo=obj_type(arHook[a].pObjName);
			BYTE *pData;
//			struct OBJ * pObj=obj_GetInfo(arHook[a].pObjName));

			switch (arHook[a].iType)
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

							p=obj_listcodget(arHook[a].pObjName); if (p==NULL) ehExit("HW:ALFA/O_LIST ObjError");
							p=StrEncode(p,SE_SQL,NULL);
							sprintf(pValue,"'%s'",p);
							ehFree(p);
							break;

						case O_ZONAP:
							pData=EhAlloc(32000);
							obj_message(arHook[a].pObjName,WS_REALGET,32000,pData);
							p=StrEncode(pData,SE_SQL,NULL);
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
				case NUME:

					switch(iObjTipo)
					{
						case O_IMARKA:
						case O_IMARKB:
						case O_MARK:
							sprintf(pValue,"%d",obj_status(arHook[a].pObjName));
							break;

						case O_LIST:
						case O_LISTP:
						case OW_LIST:
						case OW_LISTP:  
							sprintf(pValue,"%d",obj_listget(arHook[a].pObjName));
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
			SINT iIptNumber=arHook[a].iInput;
			struct IPT *psIpt;
			if ((iIptNumber<0)||(iIptNumber>IPT_info[IPT_ult].numcampi-1)) win_errgrave(__FUNCTION__":IptNum in Hook errato");
			psIpt=ipt_GetInfo(iIptNumber);
			switch (arHook[a].iType)
			{
				case NUME:
						if (psIpt->num1)
								sprintf(pValue,"%.3f",atof(ipt_read(iIptNumber))); // Bisognerebe vedere il numero di decimali
								else
								sprintf(pValue,"%d",atoi(ipt_read(iIptNumber)));
						break;
	
				default:

				case ALFA:
						p=StrEncode(ipt_read(iIptNumber),SE_SQL,NULL);
						sprintf(pValue,"'%s'",p);
						ehFree(p);
						break;

				case DATA: // Da vedere
						p=StringAlloc(ipt_read(iIptNumber));
						sprintf(pValue,"'%s'",daGiorno(p));
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

	StrReplace(pSqlCommand,"[FIELDS]",pFieldList);
	StrReplace(pSqlCommand,"[VALUES]",pValueList);

//	win_infoarg(pSqlCommand);
	bError=odbc_query(FALSE,pSqlCommand);

	ehFree(pSqlCommand);
	ehFree(pFieldList);
	ehFree(pValueList);
	ehFree(pValue);

	return bError;
}
//
// odbc_hookupdate()
// ritorna TRUE se c'è errore
//
int odbc_hookupdate(DYN_SECTION_FUNC EH_TABLEHOOK *arHook,CHAR *pQuery,...)
{
	DYN_SECTION_GLOBALPTR
	CHAR *pSqlCommand=EhAlloc(64000);
	CHAR *pFieldValueList=EhAlloc(8000);
	CHAR *pValue=EhAlloc(64000);

	SINT a;
	BYTE *p,*pFieldName;
	BOOL bError=TRUE;

	va_list Ah;
	va_start(Ah,pQuery);
	vsprintf(pSqlCommand,pQuery,Ah); // Messaggio finale
	va_end(Ah);

	// 
	// Cerco il Campo FIELDS
	//
	*pFieldValueList=0; 
	for (a=0;arHook[a].pTableFieldName;a++)
	{
		pFieldName=arHook[a].pTableFieldName; if (*pFieldName=='#') continue;
		if (arHook[a].iType==BIND) continue;
		
		// FIELDS
		if (*pFieldValueList) strcat(pFieldValueList,",");
		strcat(pFieldValueList,arHook[a].pTableFieldName); strcat(pFieldValueList,"=");
		
		//
		// OGGETTI
		//
		if (*arHook[a].pObjName)
		{
			SINT iObjTipo=obj_type(arHook[a].pObjName);
			BYTE *pData;
//			struct OBJ * pObj=obj_GetInfo(arHook[a].pObjName));

			switch (arHook[a].iType)
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

							p=obj_listcodget(arHook[a].pObjName); if (p==NULL) ehExit("HW:ALFA/O_LIST ObjError");
							p=StrEncode(p,SE_SQL,NULL);
							sprintf(pValue,"'%s'",p);
							ehFree(p);
							break;

						case O_ZONAP:
							pData=EhAlloc(32000);
							obj_message(arHook[a].pObjName,WS_REALGET,32000,pData);
							p=StrEncode(pData,SE_SQL,NULL);
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
				case NUME:

					switch(iObjTipo)
					{
						case O_IMARKA:
						case O_IMARKB:
						case O_MARK:
							sprintf(pValue,"%d",obj_status(arHook[a].pObjName));
							break;

						case O_LIST:
						case O_LISTP:
						case OW_LIST:
						case OW_LISTP:  
							sprintf(pValue,"%d",obj_listget(arHook[a].pObjName));
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
			SINT iIptNumber=arHook[a].iInput;
			struct IPT *psIpt;
			if ((iIptNumber<0)||(iIptNumber>IPT_info[IPT_ult].numcampi-1)) win_errgrave(__FUNCTION__":IptNum in Hook errato");
			psIpt=ipt_GetInfo(iIptNumber);
			switch (arHook[a].iType)
			{
				case NUME:
						if (psIpt->num1)
								sprintf(pValue,"%.3f",atof(ipt_read(iIptNumber))); // Bisognerebe vedere il numero di decimali
								else
								sprintf(pValue,"%d",atoi(ipt_read(iIptNumber)));
						break;
	
				default:

				case ALFA:
						p=StrEncode(ipt_read(iIptNumber),SE_SQL,NULL);
						sprintf(pValue,"'%s'",p);
						ehFree(p);
						break;

				case DATA: // Da vedere
						//win_infoarg("Da vedere");
						sprintf(pValue,"'%s'",daGiorno(ipt_read(iIptNumber)));
						break;
			}
		}

		//
		// Aggiungo il valore
		// 
		strcat(pFieldValueList,pValue);

	}

	StrReplace(pSqlCommand,"[FIELDS]",pFieldValueList);
//	win_infoarg(pSqlCommand);
	bError=odbc_query(FALSE,pSqlCommand);

	ehFree(pSqlCommand);
	ehFree(pFieldValueList);
	ehFree(pValue);

	return bError;
}
//
// odbc_query()
//
/*
int odbc_query(DYN_SECTION_FUNC BOOL fNoErrorStop)
{
	DYN_SECTION_GLOBALPTR

	int a=0,err=0;
	pOdbcSection->dwQueryCounter++;

	// Se ho il cursore aperto lo chiudo
//	if (pOdbcSection->fCursorOpen) {SQLCloseCursor(pOdbcSection->hStmt); pOdbcSection->fCursorOpen=FALSE;} // Apro il cursore

	// SQLSetStmtAttr(lpSqd->hStmtClone, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) lpSqd->lRowsBuffer, 0));
	pOdbcSection->sqlLastError=SQLExecDirect(	pOdbcSection->hStmt,
												pOdbcSection->lpQuery,
												SQL_NTS); // Query
	if (pOdbcSection->sqlLastError<0) 
	{
		odbc_error(__FUNCTION__);
		err=TRUE;
	}
	else
	{
		QueryResultAllocation(pOdbcSection); // Alloca in memoria il risultato
	}
	return err;
}
*/

/*

	EH_ODBC_RS pRes
    odbc_queryarg(FALSE,"SELECT * FROM ANTA200F");
	pRes=odbc_store_result(20); // Richiedo il risultato
	if (pRes)
	{
		while (odbc_fetch_row(pRes)) 
		{
			dispx("%d:%d [%d:%d]) XDTAB=[%s]",
						pRes->iOffset,pRes->iCurrentRow,
						pRes->iMaxRows,pRes->iRowsReady,
						odbc_fldptr(pRes,"XDTAB"));
		} 
		odbc_free_result(pRes); // Libero le risorse
	}
*/

