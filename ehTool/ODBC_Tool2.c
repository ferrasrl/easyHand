//   ----------------------------------------------------------------
//   | ODBC_Tool.c
//   | Tool per gestione dbase attraverso ODBC 
//   | ATTENZIONE: Si può aprire solo un file ODBC per volta
//   |                                           
//   |                                           
//   |                            by Ferrà Art & Tecnology 1993-2000
//   ----------------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ODBC_Tool.h"
#include <stdarg.h>

// Variabili Private per ODBC
static EHODBC EhOdbc;
static void ODBCError(CHAR *WhoIs);
static void LSQLTRY(CHAR *WhoIs,SINT iResult) 
{
   SINT rc = iResult; 
   if (rc != SQL_SUCCESS) 
      { 
         char szState[6]; 
         char szMsg[255]; 
         SDWORD sdwNative; 
         SWORD swMsgLen; 
         SQLError(EhOdbc.hEnv, EhOdbc.hConn, EhOdbc.hStmt, szState, &sdwNative, szMsg, sizeof(szMsg), &swMsgLen); 
		 if (rc != SQL_SUCCESS_WITH_INFO ) 
		 {
          PRG_end("Error %d performing %s\nSQLState = %s\nSQL message = %s\n", rc, WhoIs, szState, szMsg); 
		 }
      } 
}

EHODBC *ODBCInfo(void) {return &EhOdbc;}

// Status 
// FALSE = Errori non visibili
// TRUE  = Errori visibili
void ODBCViewError(BOOL Status)  {EhOdbc.fNoErrorView=Status^1;}
SQLRETURN ODBCGetLastError(void) {return EhOdbc.sqlLastError;}
/*
CHAR *SQLDisplay(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN rc1)
{
	SQLCHAR  SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLCHAR  szServ[500];
	SQLINTEGER NativeError;
	SQLSMALLINT i, MsgLen;
	SQLRETURN  rc2;
	CHAR *lpError=NULL;

	if ((rc1 == SQL_SUCCESS_WITH_INFO) || (rc1 == SQL_ERROR)) 
	{
		
		// Get the status records.
		i = 1;
		lpError=malloc(16000); *lpError=0;
		while ((rc2 = SQLGetDiagRec(iType, hstmt, i, SqlState, &NativeError,Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) 
		{
		//DisplayError(SqlState,NativeError,Msg,MsgLen);
		sprintf(szServ,"Error %d da '%s'\nSQLState = %s\nNative:%d\n%s (%d)\n", rc1,WhoIs,SqlState,NativeError,Msg,MsgLen);
		strcat(lpError,szServ);
		i++;
		}
	}
	return lpError;
}
*/
/*
void SQLTRY(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult) 
{
   SQLRETURN rc = iResult; 
   if (rc != SQL_SUCCESS) 
      { 
		 if (rc != SQL_SUCCESS_WITH_INFO) 
		 {
          PRG_end(SQLDisplay(iType,WhoIs,hstmt,rc));
		 }
      } 
}
*/
//  +-----------------------------------------
//	| START END                     |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+

void ODBCStartAdb(SINT cmd)
{
 adb_start(cmd);
 ODBCStart(cmd);
}

void ODBCEndAdb(void)
{
 adb_end();
 ODBCEnd();
}

// -------------------------------------------------
// ODBCConnect()
//
BOOL ODBCConnect(CHAR *lpServerName,CHAR *lpUserName,CHAR *lpPassword)
{
	win_open(EHWP_SCREENCENTER,50,300,59,-1,3,ON,"Connessione a ...");
	dispfm(10,27,1,-1,STYLE_BOLD,"#Arial",25,lpServerName);
	
	// --------------------------------------------------------------------
	// Mi connetto al server
	//
	EhOdbc.sqlLastError=SQLConnect(EhOdbc.hConn,    // Handle della connessione
								   lpServerName,    // Nome del server
								   SQL_NTS, // Nome del file / Driver da usare
								   lpUserName,SQL_NTS, // UserName
								   lpPassword,SQL_NTS); // Password
	if (EhOdbc.sqlLastError!=SQL_SUCCESS&&EhOdbc.sqlLastError!=SQL_SUCCESS_WITH_INFO) 
		{if (!EhOdbc.fNoErrorView) ODBCError("Connect"); 
		 win_close();
	     return TRUE;
		}

	// --------------------------------------------------------------------
	// Alloca memoria per comandi (ritorna puntatore in hStmt)
	//
	// sqlReturn=SQLAllocStmt(EhOdbc.hConn, &EhOdbc.hStmt);
	//
	EhOdbc.sqlLastError=SQLAllocHandle(SQL_HANDLE_STMT, EhOdbc.hConn, &EhOdbc.hStmt);
	if (EhOdbc.sqlLastError!=SQL_SUCCESS) {ODBCError("SQLAllocHandle/Env");  PRG_end("Ambiente stantment:?");}

	SQLSetStmtAttr(EhOdbc.hStmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_READ_ONLY, 0);
	SQLSetStmtAttr(EhOdbc.hStmt, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0);
	SQLSetCursorName(EhOdbc.hStmt, "SQL_CURSOR", SQL_NTS);

	SQLTRY(SQL_HANDLE_STMT,"SET1",EhOdbc.hStmt,SQLSetStmtAttr(EhOdbc.hStmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
	SQLTRY(SQL_HANDLE_STMT,"SET2",EhOdbc.hStmt,SQLSetStmtAttr(EhOdbc.hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0));
	SQLTRY(SQL_HANDLE_STMT,"SET3",EhOdbc.hStmt,SQLSetStmtAttr(EhOdbc.hStmt, SQL_ATTR_ROW_STATUS_PTR, NULL, 0));
	SQLTRY(SQL_HANDLE_STMT,"SET4",EhOdbc.hStmt,SQLSetStmtAttr(EhOdbc.hStmt, SQL_ATTR_ROWS_FETCHED_PTR, NULL, 0));

	win_close();
	return FALSE;
}

// -------------------------------------------------
// ODBCFileOpen()
// Apre un file ODBC
// Ricerca in base all'estensione il driver da utilizzare
// 
BOOL ODBCFileOpen(CHAR *lpFileName)
{
	CHAR *lpBuf;
	SQLCHAR szConnStr[255];
    SWORD cbConnStr;
	SINT a,Hdl;
	CHAR **Driver;
	CHAR *lpExt;
	SINT nDriver=-1;
	
	win_open(EHWP_SCREENCENTER,50,300,59,-1,3,ON,"Connessione a ...");
	dispfm(10,27,1,-1,STYLE_BOLD,"#Arial",25,file_name(lpFileName));
	
	lpExt=rstrstr(lpFileName,"."); 
	if (lpExt==NULL) 
	{
		win_infoarg("Il file non contiene estensione [%s]",lpFileName);
		win_close(); return TRUE;
	}

	Hdl=ODBCDriverList("");
	Driver=memo_heap(Hdl);

	nDriver=-1;
	for (a=0;;a++)
	{
		if (Driver[a]==NULL) break;
		if (strstr(Driver[a],lpExt)) {nDriver=a; break;}
	}

	if (nDriver==-1)
	{
		memo_libera(Hdl,"?");
		win_infoarg("Driver sconosciuto per [%s]",lpExt);
		win_close();
	    return TRUE;
	}
/*
	sqlReturn=SQLConnect(EhOdbc.hConn,    // Handle della connessione
						 lpFileName,    // Handle della Windows
						 SQL_NTS, // Nome del file / Driver da usare
						 "",SQL_NTS, // UserName
						 "",SQL_NTS); // Password

	if (sqlReturn!=SQL_SUCCESS) 
		{ODBCError("Connect",sqlReturn); 
		 win_close();
	     return TRUE;
		}
iCheck=ODBCConnect("DBQ=C:\\Convert\\pa.xls;DRIVER={Microsoft Excel Driver (*.xls)}");
 */
	lpBuf=malloc(1024);
	sprintf(lpBuf,"DBQ=%s;DRIVER={%s}",lpFileName,Driver[nDriver]);
	EhOdbc.sqlLastError=SQLDriverConnect(EhOdbc.hConn,    // Handle della connessione
			                  NULL,    // Handle della Windows
							  lpBuf, // Nome del file / Driver da usare
							  (SQLSMALLINT) strlen(lpBuf), // Lunghezza
							  szConnStr, // Connection string di ritorno
							  (SQLSMALLINT) sizeof(szConnStr), 
							  &cbConnStr, 
							  SQL_DRIVER_NOPROMPT);
	free(lpBuf);

	if (EhOdbc.sqlLastError!=SQL_SUCCESS) 
		{
		 memo_libera(Hdl,"?");
		 if (!EhOdbc.fNoErrorView) ODBCError("ODBCFileOpen"); 
		 win_close();
	     return TRUE;
		}

	// --------------------------------------------------------------------
	// Alloca memoria per comandi (ritorna puntatore in hStmt)
	//
	//sqlReturn=SQLAllocStmt(EhOdbc.hConn, &EhOdbc.hStmt);
	EhOdbc.sqlLastError=SQLAllocHandle(SQL_HANDLE_STMT, EhOdbc.hConn, &EhOdbc.hStmt);
	if (EhOdbc.sqlLastError!=SQL_SUCCESS) {ODBCError("SQLAllocHandle/Env");  PRG_end("Ambiente stantment:?");}
	memo_libera(Hdl,"?");

	win_close();
	return FALSE;

/*
 iCheck=ODBCConnect("DBQ=C:\\Convert\\pa.xls;DRIVER={Microsoft Excel Driver (*.xls)}");
   // Alloca Ambiente ODBC
	SQLTRY("SQLAllocEnv", SQLAllocEnv(&EhOdbc->hEnv),EhOdbc);

    // Alloca Memoria per la connessione
	SQLTRY("SQLAllocConnect", SQLAllocConnect(EhOdbc->hEnv, &EhOdbc->hDBC),EhOdbc);

    // Carica il driver 
	SQLTRY("SQLDriverConnect", 
	        SQDriverConnect(EhOdbc->hDBC,    // Handle della connessione
			                 NULL,    // Handle della Windows
							 File, // Nome del file / Driver da usare
							 strlen(File), // Lunghezza
							 szConnStr, // Connection string di ritorno
							 sizeof(szConnStr), 
							 &cbConnStr, 
							 SQL_DRIVER_NOPROMPT),
							 EhOdbc);

	// Alloca memoria per i comandi (ritorna puntatore in hStmt)
	SQLTRY("SQLAllocStmt", SQLAllocStmt(EhOdbc->hDBC, &EhOdbc->hStmt),EhOdbc);
	return NULL;
}
*/
}

void ODBCFreeDictionary(void)
{
	SINT i;
    if (EhOdbc.hdlDictionary>0)   
		{
//		 lpEhFld[i].lpField
		 for (i=0;i<EhOdbc.iFieldNum;i++)
		 {
			free(EhOdbc.lpEhFld[i].lpField);
		 }
		 memo_libera(EhOdbc.hdlDictionary,"ODBC"); 
	     EhOdbc.hdlDictionary=0;
		 EhOdbc.iFieldNum=0;
		 EhOdbc.lpEhFld=NULL;
		}
}



// -----------------------------------------------------------------
// ODBC Close
// -----------------------------------------------------------------
void ODBCClose(void)
{
	ODBCFreeDictionary();
	// Libera la memoria usata per i comandi
    if (EhOdbc.hStmt) SQLCloseCursor(EhOdbc.hStmt);
	if (EhOdbc.hStmt) {SQLFreeHandle(SQL_HANDLE_STMT,EhOdbc.hStmt); EhOdbc.hStmt=0;}
	if (EhOdbc.hConn) SQLDisconnect(EhOdbc.hConn);
}

void ODBCStart(SINT cmd)
{
 // Ultima chiamata
 if (cmd!=WS_LAST) return;
 ZeroFill(EhOdbc);
 // Alloca Ambiente ODBC
 LSQLTRY("SQLAllocHandleEnv",  SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &EhOdbc.hEnv));
// SQLTRY("SQLSetEnvAttr",      SQLSetEnvAttr(EhOdbc.hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0)); 
 LSQLTRY("SQLSetEnvAttr",      SQLSetEnvAttr(EhOdbc.hEnv, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0)); 
 /* Set login timeout to 5 seconds. */
 //SQLTRY("SQLAllocHandleConn", SQLSetConnectAttr(EhOdbc.hConn, (void*) SQL_LOGIN_TIMEOUT, 5, 0));
 LSQLTRY("SQLAllocHandleConn", SQLAllocHandle(SQL_HANDLE_DBC, EhOdbc.hEnv, &EhOdbc.hConn)); 
 EhOdbc.lpSQLCommand=GlobalAlloc(GPTR,5000);
}

void ODBCEnd(void)
{
	ODBCClose();
    if (EhOdbc.hConn) {SQLFreeHandle(SQL_HANDLE_DBC,EhOdbc.hConn); EhOdbc.hConn=0;}
	if (EhOdbc.hEnv)  {SQLFreeHandle(SQL_HANDLE_ENV,EhOdbc.hEnv); EhOdbc.hEnv=0;}
	if (EhOdbc.lpSQLCommand) {GlobalFree(EhOdbc.lpSQLCommand); EhOdbc.lpSQLCommand=NULL;}
}


// -------------------------------------------------
// ODBCError()
//
static void ODBCError(CHAR *WhoIs)
{
  char szState[6]; 
  char szMsg[255]; 
  SDWORD sdwNative; 
  SWORD swMsgLen; 
  CHAR *lpR="";

  switch (EhOdbc.sqlLastError)
  {
	case SQL_SUCCESS: lpR="Success"; break;
	case SQL_SUCCESS_WITH_INFO: lpR="Success+Info"; break;
	case SQL_ERROR: lpR="Errore"; break;
	case SQL_INVALID_HANDLE: lpR="InvalidHandle"; break;
  }
  
  SQLError(EhOdbc.hEnv, EhOdbc.hConn, EhOdbc.hStmt, szState, &sdwNative, szMsg, sizeof(szMsg), &swMsgLen); 
  win_infoarg("%s\nsqlResult=%d [%s]\nSQLState = %s\nSQL message = %s\n", WhoIs,EhOdbc.sqlLastError, lpR,szState, szMsg); 
}

// -----------------------------------------------------------------
// ODBC Open 
// -----------------------------------------------------------------
/*
void *ODBCConnectDriver(CHAR *File)
{
	SQLCHAR szConnStr[255];
    SQLSMALLINT cbConnStr;


    // Carica il driver 
	SQLTRY("SQLDriverConnect", 
	        SQLDriverConnect(EhOdbc.hConn,    // Handle della connessione
			                 NULL,    // Handle della Windows
							 File, // Nome del file / Driver da usare
							 (SQLSMALLINT) strlen(File), // Lunghezza
							 szConnStr, // Connection string di ritorno
							 sizeof(szConnStr), 
							 &cbConnStr, 
							 SQL_DRIVER_NOPROMPT));

	// Alloca memoria per i comandi (ritorna puntatore in hStmt)
	SQLTRY("SQLAllocStmt", SQLAllocStmt(EhOdbc.hConn, &EhOdbc.hStmt));

	return NULL;
}
*/
/*

         retcode = SQLConnect(hdbc, (SQLCHAR*) "Sales", SQL_NTS,
                (SQLCHAR*) "JohnS", SQL_NTS,
                (SQLCHAR*) "Sesame", SQL_NTS);

*/

// -----------------------------------------------------------------
// ODBC DoDictionary
// -----------------------------------------------------------------
void ODBCDoDictionary(void)
{
	 SQLCHAR szNameColumn[255];
	 SQLSMALLINT NameLenght;
	 SQLSMALLINT DataType;
	 SQLUINTEGER ColumnSize;
	 SQLSMALLINT DecimnaDigits;
	 SQLSMALLINT Nullable;
	 CHAR *lpType;
	 SINT iCount;
	 //SINT iMemory;
//	 CHAR *lpBind;
	 EHODBCFLD *lpEhFld;
	 SINT i,iTypeC;
     SDWORD sdwFNLen;
	 SINT iTotMemory;
	 SINT iAdbType;
	 SINT iAdd=1;

	 // Conto i campi
	 iCount=0; //iMemory=0;
	 for (i=1;;i++)
	 {
	  if (SQLDescribeCol(EhOdbc.hStmt, // Handle del file
					     (SQLSMALLINT) i, // Numero della colonna
					     szNameColumn,
						 sizeof(szNameColumn),
						 &NameLenght,
						 &DataType,
				         &ColumnSize,&DecimnaDigits,
				         &Nullable)!=SQL_SUCCESS) break;
	  iCount++;
	 }

	 // Alloco memoria per descrizione Campi
	 iTotMemory=(sizeof(EHODBCFLD)*iCount);
	 ODBCFreeDictionary();

	 EhOdbc.hdlDictionary=memo_chiedi(RAM_AUTO,iTotMemory,"EHODBC");
	 if (EhOdbc.hdlDictionary<0) PRG_end("ODBC:Errore in DoDictionary()");
	 lpEhFld=Wmemo_lock(EhOdbc.hdlDictionary);
	
	 memset(lpEhFld,0,iTotMemory);
	 EhOdbc.iFieldNum=iCount;
	 EhOdbc.lpEhFld=lpEhFld;
	 EhOdbc.iTotMemory=iTotMemory;
	
	 // Alloco e binderizzo i campi
	 for (i=0;i<iCount;i++)
	 {
	  if (SQLDescribeCol(EhOdbc.hStmt, // Handle del file
					    (SQLSMALLINT) (i+1), // Numero della colonna
					    szNameColumn,
						(SQLSMALLINT) sizeof(szNameColumn),
						&NameLenght,
						&DataType,
				        &ColumnSize,
						&DecimnaDigits,
				        &Nullable)!=SQL_SUCCESS) break;
	
	 lpType="<?>"; iTypeC=SQL_C_CHAR; iAdbType=ADB_ALFA;
//	 win_infoarg("DD: %s %d %d",szNameColumn,DataType,ColumnSize);
	 switch (DataType)
	 {
	   case SQL_CHAR:     lpType="ALFAF";    iTypeC=SQL_C_CHAR; break;
	   case SQL_VARCHAR:  lpType="ALFA";     iTypeC=SQL_C_CHAR; break;
	   case SQL_DECIMAL:  lpType="DECIMAL";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_NUMERIC:  lpType="NUMERIC";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_SMALLINT: lpType="SMALLINT"; iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_INTEGER:  lpType="INTEGER";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_REAL:	  lpType="REAL";     iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_FLOAT:	  lpType="FLOAT";    iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_DOUBLE:	  lpType="DOUBLE";   iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_BIT:	  lpType="BIT";      iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_TINYINT:  lpType="TINY";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   case SQL_UNKNOWN_TYPE: lpType="UNKNOW";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;

	   case SQL_LONGVARCHAR: lpType="BLOBALFA";    iTypeC=SQL_C_CHAR; break;
	   case SQL_BINARY: lpType="BLOBBIN";    iTypeC=SQL_C_CHAR; break;
	   case SQL_BIGINT: lpType="BIGINT"; iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
		   /*
#define SQL_VARBINARY                           (-3)
#define SQL_LONGVARBINARY                       (-4)
#define SQL_BIGINT                              (-5)
#define SQL_TINYINT                             (-6)
		   */
	   //case -6 : lpType="16BIT";  iTypeC=SQL_C_CHAR; iAdbType=ADB_NUME; break;
	   default: win_infoarg("DataType:%x (%d) ?",(SINT) DataType,DataType); iTypeC=SQL_C_CHAR; break;
	 }

	 //win_infoarg("[%s][%s] <-- %d --->",szNameColumn,lpType,ColumnSize);

	 memset(lpEhFld+i,0,sizeof(EHODBCFLD));
	 strcpy(lpEhFld[i].szName,szNameColumn);
	 strcpy(lpEhFld[i].szType,lpType);
     lpEhFld[i].iType=DataType;//(DataType<0)?255+DataType:DataType;
	 lpEhFld[i].iAdbType=iAdbType;
	 if (!ColumnSize) ColumnSize=1024; // Dimensione colonna indeterminabile
	 lpEhFld[i].iSize=ColumnSize+1; 
	 //if (ColumnSize<1) PRG_end("ColumnSize %d ?",ColumnSize);
	 lpEhFld[i].lpField=malloc(ColumnSize+1);//lpBind;
	 //win_infoarg("%s %d",szNameColumn,ColumnSize);
	 if (!lpEhFld[i].lpField) PRG_end("lpField %s ?",szNameColumn);

	 if (SQLBindCol(EhOdbc.hStmt, 
					(SQLSMALLINT) (i+1), 
					SQL_C_CHAR, 
					(PTR) lpEhFld[i].lpField, 
					lpEhFld[i].iSize,//(ColumnSize+1), 
					&sdwFNLen)) {PRG_end("ODBC: BINDErrore in [%s]",lpEhFld[i].szName);}
	}
}

void ODBCExecute(void)
{
	LSQLTRY("SQLExecute", SQLExecute(EhOdbc.hStmt));
}

// -----------------------------------------------------------------
// ODBC Select
// -----------------------------------------------------------------
/*
void ODBCSelect(CHAR *lpSelect)
{
	SQLTRY("SQLPrepare", SQLPrepare(EhOdbc.hStmt, lpSelect, strlen(lpSelect)));
}
*/

SQLRETURN ODBCArgPrepare(CHAR *Mess,...)
{
	va_list Ah;

	va_start(Ah,Mess);
	vsprintf(EhOdbc.lpSQLCommand,Mess,Ah); // Messaggio finale

	// Preparo
	EhOdbc.sqlLastError=SQLPrepare(EhOdbc.hStmt, EhOdbc.lpSQLCommand, SQL_NTS);
	if (EhOdbc.sqlLastError!=SQL_SUCCESS) 
	{if (!EhOdbc.fNoErrorView) ODBCError("ArgSqlPrepare:"); 
	 goto FINE;
	}

	// Creo il dizionario
	ODBCDoDictionary(); // Creo il dizionario del risultato

FINE:
//	free(lpBuf);
	va_end(Ah);
	return EhOdbc.sqlLastError;
}

SQLRETURN ODBCArgExecute(CHAR *Mess,...)
{
	va_list Ah;

	va_start(Ah,Mess);
	vsprintf(EhOdbc.lpSQLCommand,Mess,Ah); // Messaggio finale

//	win_infoarg("[%s]",EhOdbc.lpSQLCommand);

	// Preparo
	EhOdbc.sqlLastError=SQLPrepare(EhOdbc.hStmt, EhOdbc.lpSQLCommand, SQL_NTS);
	if ((EhOdbc.sqlLastError!=SQL_SUCCESS)&&(EhOdbc.sqlLastError!=SQL_SUCCESS_WITH_INFO))
	{if (!EhOdbc.fNoErrorView) ODBCError("AE:Prepare"); 
	 goto FINE;
	}

	// Creo il dizionario
	ODBCDoDictionary(); // Creo il dizionario del risultato

	// Eseguo il comando
	EhOdbc.sqlLastError=SQLExecute(EhOdbc.hStmt);
	if (EhOdbc.sqlLastError!=SQL_SUCCESS) {if (!EhOdbc.fNoErrorView) ODBCError("AE:Execute");}

/*
	EhOdbc.sqlLastError=SQLSetPos(EhOdbc.hStmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
	if (EhOdbc.sqlLastError!=SQL_SUCCESS) 
	{
		if (!EhOdbc.fNoErrorView) ODBCError("AE:SetPos");
	}
*/

FINE:
	va_end(Ah);
	return EhOdbc.sqlLastError;
}

SQLRETURN ODBCCloseCursor(void)
{
    if (EhOdbc.hStmt) SQLCloseCursor(EhOdbc.hStmt);
	/*
	if (EhOdbc.hStmt) {SQLFreeHandle(SQL_HANDLE_STMT,EhOdbc.hStmt); EhOdbc.hStmt=0;}

	EhOdbc.sqlLastError=SQLAllocHandle(SQL_HANDLE_STMT, EhOdbc.hConn, &EhOdbc.hStmt);
	if (EhOdbc.sqlLastError!=SQL_SUCCESS)  {ODBCError("SQLAllocHandle/Env");  PRG_end("Ambiente stantment:?");}
*/
//	return EhOdbc.sqlLastError;
	/*
	EhOdbc.sqlLastError=SQLCloseCursor(EhOdbc.hStmt);
	return EhOdbc.sqlLastError;
}

SQLRETURN ODBCStop(void)
{
	SQLCloseCursor(EhOdbc.hStmt);
	*/
	//EhOdbc.sqlLastError=SQLFreeStmt(EhOdbc.hStmt,SQL_CLOSE);
	return EhOdbc.sqlLastError;
}

BOOL ODBCNext(void)
{
  RETCODE rc;
  //memset(EhOdbc.lpBind,0,EhOdbc.iSizeBind); // Pulisco lo spazio dei record

  rc=SQLFetch(EhOdbc.hStmt);
  if ((rc==SQL_SUCCESS)||(rc==SQL_SUCCESS_WITH_INFO)) return FALSE; else return TRUE;
}

/*
while ((rc = SQLFetch(hstmt)) != SQL_NO_DATA) {
 */
CHAR *ODBCFldPtr(CHAR *lpName)
{
  SINT i;
  for (i=0;i<EhOdbc.iFieldNum;i++)
  {
	  //win_infoarg("%d) %s - [%s]",i,EhOdbc.lpEhFld[i].szName,EhOdbc.lpEhFld[i].lpField);
	  if (!_stricmp(lpName,EhOdbc.lpEhFld[i].szName)) return EhOdbc.lpEhFld[i].lpField;
  }
  /*
  win_infoarg("ODBCFldPtr:Campo [%s] non trovato",lpName);
  for (i=0;i<EhOdbc.iFieldNum;i++)
  {
	  win_infoarg("%d) %s - [%s]",i,EhOdbc.lpEhFld[i].szName,EhOdbc.lpEhFld[i].lpField);
  }
  PRG_end("ODBCFldPtr:Campo [%s] non trovato",lpName);
  */
  return NULL;
}

CHAR *ODBCColPtr(SINT iCol)
{
	if (iCol>=EhOdbc.iFieldNum) PRG_end("Error 1");
	return EhOdbc.lpEhFld[iCol].lpField;
}

void ODBCShowDictionary(void)
{
 SINT i;
 for (i=0;i<EhOdbc.iFieldNum;i++)
  {
	  win_infoarg("%d) %s  Tipo:%s size:%d",i,EhOdbc.lpEhFld[i].szName,EhOdbc.lpEhFld[i].szType,EhOdbc.lpEhFld[i].iSize);
  }
}

EHODBCFLD *ODBCFldInfo(CHAR *lpName)
{
  SINT i;
  for (i=0;i<EhOdbc.iFieldNum;i++)
  {
	  //win_infoarg("%d) %s - [%s]",i,EhOdbc.lpEhFld[i].szName,EhOdbc.lpEhFld[i].lpField);
	  if (!strcmp(lpName,EhOdbc.lpEhFld[i].szName)) return &EhOdbc.lpEhFld[i];
  }
  //PRG_end("ODBCFldInfo:Campo [%s] non trovato",lpName);
  return NULL;
}

void ODBCSaveDictionary(CHAR *lpFileName)
{
 SINT i;
 FILE *ch;
 
 if ((ch=fopen(lpFileName,"w"))==NULL) return;
 for (i=0;i<EhOdbc.iFieldNum;i++)
  {
	 fprintf(ch,"%3d) %-10.10s  %-10.10s  %5d\n",i,
			 EhOdbc.lpEhFld[i].szName,
			 EhOdbc.lpEhFld[i].szType,
			 EhOdbc.lpEhFld[i].iSize); 
  }
 fclose(ch);
}

/*
void *ODBCDriverList(void)
{
	SQLCHAR szDriver[180];
	SQLCHAR szDriverAttr[180];
	SQLCHAR szServerName[180];
	SQLCHAR szServerDesc[180];
	SQLSMALLINT iDriveLen;
	SQLSMALLINT iDriveAttrLen;
	SQLSMALLINT iServerName;
	SQLSMALLINT iServerDesc;
	SQLRETURN sqlReturn;
	memset(EhOdbc,0,sizeof(EHODBC));

    // Alloca Ambiente ODBC
	SQLTRY("SQLAllocEnv", SQLAllocEnv(&EhOdbc.hEnv),EhOdbc);

	SQLTRY("SQLDrivers", SQLDrivers(EhOdbc.hEnv, 
									SQL_FETCH_FIRST,
									szDriver,
									sizeof(szDriver)-1,
									&iDriveLen,
									szDriverAttr,
									sizeof(szDriverAttr)-1,
									&iDriveAttrLen),
									EhOdbc);

	win_infoarg("Driver:[%s][%s]",szDriver,szDriverAttr);
	 
	// -------------------------------
	// Elenco server disponibili
	//
	
	sqlReturn=SQLDataSources(EhOdbc.hEnv,SQL_FETCH_FIRST,
							szServerName,sizeof(szServerName)-1,&iServerName,
							szServerDesc,sizeof(szServerDesc)-1,&iServerDesc);

	while (TRUE)
	{

		if (sqlReturn!=SQL_SUCCESS) break;
		
		win_infoarg("Server:[%s][%s]",szServerName,szServerDesc);
		sqlReturn=SQLDataSources(EhOdbc.hEnv,SQL_FETCH_NEXT,
							szServerName,sizeof(szServerName)-1,&iServerName,
							szServerDesc,sizeof(szServerDesc)-1,&iServerDesc);

	}

*/
/*
    // Alloca Memoria per la connessione
	SQLTRY("SQLAllocConnect", SQLAllocConnect(EhOdbc.hEnv, &EhOdbc.hDBC),EhOdbc);

    // Carica il driver 
	SQLTRY("SQLDriverConnect", 
	        SQLDriverConnect(EhOdbc.hDBC,    // Handle della connessione
			                 NULL,    // Handle della Windows
							 File, // Nome del file / Driver da usare
							 strlen(File), // Lunghezza
							 szConnStr, // Connection string di ritorno
							 sizeof(szConnStr), 
							 &cbConnStr, 
							 SQL_DRIVER_NOPROMPT),
							 EhOdbc);

	// Alloca memoria per i comandi (ritorna puntatore in hStmt)
	SQLTRY("SQLAllocStmt", SQLAllocStmt(EhOdbc.hDBC, &EhOdbc.hStmt),EhOdbc);
	*/
/*
	return NULL;
}
*/

// --------------------------------------------------------------------------------
// ODBCSourceList
// Crea una lista con ARMAKER delle Sorgenti di dati presenti nel sistema
// Filtra i dati cercando la parola passata con *lpDriver nel driver collegato
// alla sorgente 
// --------------------------------------------------------------------------------
SINT ODBCSourceList(CHAR *lpDriver)
{
	SQLCHAR szServerName[180];
	SQLCHAR szServerDesc[180];
	SQLSMALLINT iServerName;
	SQLSMALLINT iServerDesc;
	SQLRETURN sqlReturn;
//	CHAR Buffer[500];

	// -------------------------------
	// Elenco server disponibili
	//
	
	ARMaker(WS_OPEN,NULL);
	win_open(EHWP_SCREENCENTER,50,232,59,-1,3,ON,"Lettura Data Source Name");

	sqlReturn=SQLDataSources(EhOdbc.hEnv,SQL_FETCH_FIRST,
							szServerName,sizeof(szServerName)-1,&iServerName,
							szServerDesc,sizeof(szServerDesc)-1,&iServerDesc);
	do 
	{
//		win_infoarg("[%s]",szServerDesc);
		if (!strstr(szServerDesc,lpDriver)) continue;
		ARMaker(WS_ADD,szServerName);
	} while (SQLDataSources(EhOdbc.hEnv,SQL_FETCH_NEXT,
							szServerName,
							sizeof(szServerName)-1,&iServerName,
							szServerDesc,
							sizeof(szServerDesc)-1,&iServerDesc)==SQL_SUCCESS);
	win_close();
	return ARMaker(WS_CLOSE,"ODBCSource");
}


// --------------------------------------------------------------------------------
// ODBCSourceList
// Crea una lista con ARMAKER delle Sorgenti di dati presenti nel sistema
// Filtra i dati cercando la parola passata con *lpDriver nel driver collegato
// alla sorgente 
// --------------------------------------------------------------------------------
SINT ODBCDriverList(CHAR *lpDriver)
{
	SQLCHAR szServerName[180];
	SQLCHAR szServerDesc[180];
	SQLSMALLINT iServerName;
	SQLSMALLINT iServerDesc;
	SQLRETURN sqlReturn;
//	CHAR Buffer[500];

	// -------------------------------
	// Elenco server disponibili
	//
	
	ARMaker(WS_OPEN,NULL);
	win_open(EHWP_SCREENCENTER,50,232,59,-1,3,ON,"Lettura Data Source Name");

	sqlReturn=SQLDataSources(EhOdbc.hEnv,SQL_FETCH_FIRST,
							szServerName,sizeof(szServerName)-1,&iServerName,
							szServerDesc,sizeof(szServerDesc)-1,&iServerDesc);
	do 
	{
		if (!strstr(szServerDesc,lpDriver)) continue;
//		sprintf(Buffer,"%s - %s",szServerName,szServerDesc);
		ARMaker(WS_ADD,szServerDesc);
	} while (SQLDataSources(EhOdbc.hEnv,SQL_FETCH_NEXT,
							szServerName,
							sizeof(szServerName)-1,&iServerName,
							szServerDesc,
							sizeof(szServerDesc)-1,&iServerDesc)==SQL_SUCCESS);

	win_close();
	return ARMaker(WS_CLOSE,"ODBCSource");
}

// --------------------------------------------------------------------------------
// ODBCTableList
// Crea una lista delle tabelle di un dbase
// --------------------------------------------------------------------------------
SINT ODBCTableList(CHAR *lpLibrary,CHAR *lpName,CHAR *lpType)
{
	ARMaker(WS_OPEN,NULL);
	if (!EhOdbc.hStmt) goto FINE;
    // Carica il driver 
	//win_open(EHWP_SCREENCENTER,50,232,59,-1,3,ON,"Lettura delle Tabelle");
	mouse_graph(0,0,"CLEX");

	ODBCCloseCursor();
	if (lpLibrary) 
	{
		if (!*lpLibrary) lpLibrary=NULL;
	}
	// Chiedo l'elenco delle tabelle
//	EhOdbc.sqlLastError=SQLTables(EhOdbc.hStmt, lpLibrary, SQL_NTS, NULL, 0, "%", SQL_NTS, NULL, 0);
	EhOdbc.sqlLastError=SQLTables(EhOdbc.hStmt, 
								  NULL, 0, // Catalogo
								  //"",SQL_NTS,
								  lpLibrary, lpLibrary?SQL_NTS:0, //Schema
								  lpName, (lpName?SQL_NTS:0), // Nome della tabella
								  lpType, lpType?SQL_NTS:0); // Tipo della tabella

	if (EhOdbc.sqlLastError!=SQL_SUCCESS) 
	{if (!EhOdbc.fNoErrorView) ODBCError("ODBCTableList()::SQLTables()"); 
	 goto FINE;
	}

	ODBCDoDictionary(); // Creo il dizionario del risultato
	while (!ODBCNext())
	{  
		//dispx("[%s] %s     <        ",ODBCFldPtr("TABLE_TYPE"),ODBCFldPtr("TABLE_NAME")); mouse_input();
		if (!strcmp(ODBCFldPtr("TABLE_TYPE"),lpType))
			//!strcmp(ODBCFldPtr("TABLE_TYPE"),"SYSTEM TABLE"))
			{	
				ARMaker(WS_ADD,ODBCFldPtr("TABLE_NAME"));
			}
	} 

	ODBCCloseCursor();
FINE:
	//win_close();
	MouseCursorDefault();
	return ARMaker(WS_CLOSE,"ODBCSource");
}


EHODBC *ODBCGetInfo(void)
{
	return &EhOdbc;
}