//  -----------------------------------------------------------------------
//  | FLM_ADB2003  Gestione ADB AdvancedDataBase                             
//  |                                                                     
//  | Ciannomessolemani:                                                  
//  | G.Tassistro  
//  | L.Vadora      
//  | A.Dondi
//  |                                                                     
//  |                                 (C) Ferrà Art & Technology 1996-2003 
//  -----------------------------------------------------------------------
#include "/easyhand/inc/easyhand.h"
//#include <math.h>

#ifdef _PERVASIVE61
#pragma comment(lib, "/easyhand/lib/win32/wbtrv32.lib")
#pragma message("Includo /easyhand/lib/win32/wbtrv32.lib <------")
#else
#pragma comment(lib, "/easyhand/lib/win32/w3btrv7.lib")
#pragma message("Includo /easyhand/lib/win32/w3btrv7.lib <------")
#endif

#ifdef _ADBEMU
  // #include "/easyhand/inc/eh_odbc.h"
  extern EH_ODBC_SECTION sOdbcSection;
#endif

static void adb_BlobControl(HDB Hdb);


static ULONG ulPreAlloc=0; // Pre allocazione memoria in apertura
static BOOL fDynamicMemo=TRUE; // T/F se si vuole l'allocazione dinamica



#ifdef _ADBEMU
#define MAX_VIRTUAL_HREC 2000
static void adb_ODBCDoDictionary(INT Hdl);

static CHAR *adb_ODBCFldPtr(INT Hdb,CHAR *lpName);
EMUFIELD *adb_ODBCFldInfo(INT Hdb,CHAR *lpName);
static SQLRETURN adb_ODBCCloseCursor(INT Hdb);

//static CHAR *EmuFldTranslate(INT Hdl,CHAR *lpName);
static void EmuBindMaker(HDB Hdb,BOOL);

void EmuSelectBuilder(INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere);
static void EmuDeleteBuilder(INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere);
static void EmuUpdateBuilder(INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere);
static void EmuInsertBuilder(INT Hdb,CHAR *lpSQLCommand);
INT EmuTypeCount(CHAR *lpTypeObj,INT iStart,INT iEnd);

static BOOL EmuHrecFieldFind(HDB Hdb,CHAR *lpField);
static void adb_EmuClose(void);

static CHAR *EmuSQLFormat(HDB hdb,EMUBIND *lpEmuBind,void *lpAdbValore);

static void BT_OdbcToAdb(HDB Hdb);
static CHAR *LocalSpecialTranslate(CHAR *lpValore,CHAR *lpSpecialTranslate,BOOL bForSQL);

EMUSTRU sEmu; // Strutture per emulazione
#endif					   

// extern struct IPTINFO *IPT_info;
// extern INT IPT_ult; // Numero di input aperti

 void   BTRCLOSE(void);
 BTI_API BTRVCE(void);

BTI_API BTRV32(
           BTI_WORD      operation,
           BTI_VOID_PTR  posBlock,
           BTI_VOID_PTR  dataBuffer,
           BTI_ULONG_PTR dataLength,
           BTI_VOID_PTR  keyBuffer,
           BTI_SINT      keyNumber );

#ifdef _ADBEMU

CHAR *SQLGetLastError(SQLSMALLINT iType,SQLHSTMT hstmt,SQLRETURN rc1,SQLINTEGER *lpNativeError,SQLCHAR  *lpSqlState)
{
	static SQLCHAR Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLSMALLINT MsgLen;

	if ((rc1 == SQL_SUCCESS_WITH_INFO) || (rc1 == SQL_ERROR)) 
	{
		SQLGetDiagRec(iType, hstmt, 1, lpSqlState, lpNativeError, Msg, sizeof(Msg), &MsgLen);
	}
	return Msg;
}

BOOL SQLCheckError(SQLHSTMT hstmt,SQLRETURN rc1,CHAR *lpErrors)
{
	SQLCHAR  SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	SQLCHAR  szServ[500];
	SQLINTEGER NativeError;
	SQLSMALLINT i, MsgLen;
	SQLRETURN  rc2;

	if ((rc1 == SQL_SUCCESS_WITH_INFO) || (rc1 == SQL_ERROR)) 
	{
		
		// Get the status records.
		i = 1;
		while ((rc2 = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, i, SqlState, &NativeError,Msg, sizeof(Msg), &MsgLen)) != SQL_NO_DATA) 
		{
		 //sprintf(szServ,"Error rc1=%d da '%s'\nSQLState = %s\nNativeError=%d\n%s (LenMsg=%d)\n", rc1,WhoIs,SqlState,NativeError,Msg,MsgLen);
		 //strcat(lpError,szServ);
		 //i++;
			sprintf(szServ,"|%d|",NativeError);
			//win_infoarg("[%s]",szServ);
			if (strstr(lpErrors,szServ)) return TRUE;
			i++;
		}
	}
	return FALSE;
}

CHAR *SQLDisplay(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN rc1)
{
	SQLCHAR  SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
	//SQLCHAR  szServ[1500];
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
		 //DisplayError(SqlState,NativeError,Msg,MsgLen);
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

void SQLTRY(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult) 
{
   SQLRETURN rc = iResult; 
   sEmu.sqlLastError=iResult;
   if (rc != SQL_SUCCESS) 
      { 
		 if (rc != SQL_SUCCESS_WITH_INFO) 
		 {
          ehExit(SQLDisplay(iType,WhoIs,hstmt,rc));
		 }
      } 
}

static void LSQLExecute(CHAR *WhoIs,SQLHSTMT hstmt,CHAR *lpSQLCommand) 
{
   SQLRETURN rc; 
   rc=SQLExecDirect(hstmt,lpSQLCommand,SQL_NTS);
   sEmu.sqlLastError=rc;
   if (rc != SQL_SUCCESS) 
      { 
		 if (rc != SQL_SUCCESS_WITH_INFO) 
		 {
          ehExit("SQLExecute: %s\n%s\n%s",WhoIs,lpSQLCommand,SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
		 }
      } 
}

static SQLRETURN LSQLExecuteError(SQLHSTMT hstmt,CHAR *lpSQLCommand) 
{
   SQLRETURN rc; 
   rc=SQLExecDirect(hstmt,lpSQLCommand,SQL_NTS);
   sEmu.sqlLastError=rc;
   return rc;
}

void adb_SQLExecute(HDB hdb,CHAR *lpWhoIs,CHAR *lpSQLCommand) 
{
	LSQLExecute(lpWhoIs,ADB_info[hdb].hStmt,lpSQLCommand);
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

  switch (sEmu.sqlLastError)
  {
	case SQL_SUCCESS: lpR="Success"; break;
	case SQL_SUCCESS_WITH_INFO: lpR="Success+Info"; break;
	case SQL_ERROR: lpR="Errore"; break;
	case SQL_INVALID_HANDLE: lpR="InvalidHandle"; break;
  }
  
//  SQLError(sEmu.hEnv, sEmu.hConn, sEmu.hStmt, szState, &sdwNative, szMsg, sizeof(szMsg), &swMsgLen); 
  SQLError(sOdbcSection.hEnv, sOdbcSection.hConn, NULL, szState, &sdwNative, szMsg, sizeof(szMsg), &swMsgLen); 
  win_infoarg("%s\nsqlResult=%d [%s]\nSQLState = %s\nSQL message = %s\n", WhoIs,sEmu.sqlLastError, lpR,szState, szMsg); 
}
#endif

void adb_BufRealloc(HDB hdb,ULONG ulRequestSize,BOOL fCopy,CHAR *lpWho,BOOL fOpen)
{
	BYTE *lpNewBuffer=NULL;
	ULONG ulNewSize,ulOldSize;

	if (!fOpen&&!fDynamicMemo) ehExit("adb_BufRealloc(): Riallocazione Bloccata in running mode [%s]",lpWho);

	//win_infoarg("PRIMA %d",ADB_info[hdb].ulRecBuffer);
	// Riallocazione per tentativi
	ulNewSize=ulOldSize=ADB_info[hdb].ulRecBuffer;
	if (!ulRequestSize)
	{
		if (ADB_info[hdb].ulRecBuffer<0xF000) 
				ulNewSize=ADB_info[hdb].ulRecBuffer+1024;
				else
				{
				 ulNewSize=ADB_info[hdb].ulRecBuffer+(ADB_info[hdb].ulRecBuffer/10);
				 //_d_("[%d]     ",ulNewSize);
				}
		//if (ulNewSize>2000000) ehExit("Record Over Size");
		lpNewBuffer=ehAllocZero(ulNewSize); if (!lpNewBuffer) ehExit("adb_BufRealloc(1) - %d [%s]",ulNewSize,lpWho);
	}
	// Ho la dimensione
	else
	{
		if (ulRequestSize>ADB_info[hdb].ulRecBuffer) 
			{ulNewSize=ulRequestSize;
			 lpNewBuffer=ehAllocZero(ulRequestSize); if (!lpNewBuffer) ehExit("adb_BufRealloc(2) - %d [%s]",ulRequestSize,lpWho);
			} 
	}

	if (ulNewSize<ADB_info[hdb].ulRecBuffer&&lpNewBuffer) ehExit("Errore ulNewSize %d<%d",ulNewSize,ADB_info[hdb].ulRecBuffer);

	if (lpNewBuffer) 
	{
		memset(lpNewBuffer,0,ulNewSize);// Azzero il nuovo buffer
		if (ulOldSize>ulNewSize) ehError();
		if (fCopy) memcpy(lpNewBuffer,ADB_info[hdb].lpRecBuffer,ulOldSize);
		ehFree(ADB_info[hdb].lpRecBuffer);
		ADB_info[hdb].ulRecBuffer=ulNewSize;
		ADB_info[hdb].lpRecBuffer=lpNewBuffer;
	}
}

void adb_PreAlloc(ULONG ulDim)
{
	ulPreAlloc=ulDim;
}

void adb_DynMemo(BOOL fFlag)
{
	fDynamicMemo=fFlag;
}

static ULONG adb_BlobNewOfs(HDB Hdb,ULONG ulSize);
static void adb_BlobFldWrite(INT Hdb,CHAR *lpField,CHAR *lpBuffer);

#if !defined(_NODC)&&!defined(EH_CONSOLE)
#define HLINK 1
INT  ADB_HL=OFF; // Se il Link gerarchico è attivo
CHAR  HLtabella[MAXPATH];// Percorso e nome della Tabella Gerarchica
LONG  adb_HLink=0; // Numero di Link Gerarchici
INT  adb_HLinkHdl=-1; // Handle che contiene la tabella

static INT adb_HLfind(CHAR *FilePadre,LONG *pt);
static INT adb_HLfindFiglio (CHAR *FileFiglio,LONG *pt);
static void adb_HLload(FILE *pf1,struct ADB_LINK_LIST *link);
static void adb_HLsave(FILE *pf1,struct ADB_LINK_LIST *link,CHAR *mark);
static void adb_HLinsert(CHAR *file,struct ADB_LINK *link);
static void adb_HLinsertPro(CHAR *file,CHAR *client);
static INT adb_HLsonopadre(INT Hdb);
static CHAR *adb_HLaction(INT Hdb,CHAR *);
static void  adb_HLchiudi(void);
#endif

static INT adb_BlobOfsDyn(HDB Hdb); // Offset dell'inizio della zona dinamica

char *ConvertoPro (char*Proprietario);

#if !defined(_NODC)&&!defined(EH_CONSOLE)
static INT  adb_LockControl(INT Flag,INT Hdb,INT status,CHAR *tip,INT Escape);
static void win_btierr(CHAR *mess,CHAR *primo);
#endif

//   +-------------------------------------------
//   | BTI_err  Visualizza l'errore btrieve      |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
static BOOL fErrorView=TRUE;
static BOOL fTransActive=FALSE;

void adb_ErrorView(BOOL fError)
{
 fErrorView=fError;
}

static void BTI_err(HDB hdb,CHAR *opera,INT status)
{
	FILE *pf1;
	CHAR mess[300];
	CHAR buf[40];
	CHAR *lpFile=ADB_info[hdb].lpFileName;

	// Errori speciali
	if (status==3014)
	{
		sprintf(mess,"Pervasive:\nFile:%s\nin %s\n%s (status %d)",
			  lpFile,opera,"Netware:Versione installata antecedente 6.15",status);

		goto ViewMess;
	}
	if (status==3017)
	{
		sprintf(mess,"Pervasive:\nFile:%s\nin %s\n%s\n[ADB_info[Hdb].ulRecBuffer=%ld] (status %d)",
			  lpFile,opera,"Memoria insufficiente nel buffer Pervasive.SQL",
			  ADB_info[hdb].ulRecBuffer,
			  status);
		goto ViewMess;
	}

	if ((status<0)||(status>107)) 
	{
		win_infoarg("BTI_err: %s Status non gestito [%d]",opera,status); 
		return;
	}
	if (!fErrorView) return;

	if (fileCheck(ehPath("BTI.ERR"))) {

		pf1=fopen(ehPath("BTI.ERR"),"rb"); if (!pf1) ehError();
		fseek(pf1,(36*status),SEEK_SET);
		fread(buf,34,1,pf1);
		buf[3]=0; buf[33]=0;
		fclose(pf1);
		sprintf(mess,"Pervasive:\nFile:%s\nin %s\n%s (%s)",lpFile,opera,&buf[4],buf);
	}
	else
	{
		sprintf(mess,"Pervasive:\nFile:%s\nin %s\n%d",lpFile,opera,status);
	}
	ViewMess:

	win_err(mess);
}

// --------------------------------------
// Utiliti di copia adb_buffer          !
// --------------------------------------

void HookCopy(INT cmd,INT Hdb)
{
 static INT Size;
 static INT hdlMemory=-1;
 static CHAR *PtrRecord; // Puntatore al Record
 static CHAR *PtrCopy;
 switch (cmd)
 {
	 case WS_OPEN:

		 if (hdlMemory!=-1) memoFree(hdlMemory,"HC1");
		 hdlMemory=-1;
		 PtrRecord=adb_DynRecBuf(Hdb);
		 if (PtrRecord==NULL) ehExit("HC2");
		 break;

	case WS_CLOSE:
		 if (hdlMemory>-1) memoFree(hdlMemory,"HC4");
		 hdlMemory=-1;
		 break;

	case HC_BACKUP: // Copia nel buffer

		 Size=adb_GetDynSize(Hdb);
		 if (hdlMemory>-1) memoFree(hdlMemory,"HC4B");
		 hdlMemory=memoAlloc(M_HEAP,Size,"HookCopy"); 
		 if (hdlMemory==-1) ehExit("HC5 [%d]",Size);
		 PtrCopy=memoPtr(hdlMemory,NULL);
		 memcpy(PtrCopy,PtrRecord,Size);
		 break;

	case HC_RESTORE: // Ripristina l'hook

		 if (hdlMemory==-1) return;//ehExit("HC_RESTRE");
		 memcpy(PtrRecord,PtrCopy,Size);
		 break;

	case HC_RESET : // Reset della copia

		 if (hdlMemory==-1) return;//ehExit("HC7");
		 memset(PtrCopy,0,Size);
		 break;
 }
}

//  +-----------------------------------------
//	| ADB_START e ADB_END                     |
//	|                                         |
//	|  Inizializza e chiude la gestione       |
//	|  del Dbase                              |
//	|                                         |
//	|  Ritorna    : -1 Non cè suff. memoria   |
//	|                                         |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+
void adb_start(INT cmd)
{
	WORD a;
	CHAR serv[20],*pt=serv;

	// Ultima chiamata
	if (cmd!=WS_LAST) return;

#ifdef _ADBEMU
	ZeroFill(sEmu);  
#endif

	if (ADB_max==0) {ADB_hdl=-1; return;}
	ADB_hdl=memoAlloc(M_HEAP,(LONG) sizeof(struct ADB_INFO)*ADB_max,"*ADB");

	if (ADB_hdl<0) ehExit("Non cè memoria per ADB_SYSTEM");
	ADB_info=(struct ADB_INFO *) memoPtr(ADB_hdl,NULL);
	memset(ADB_info,0,sizeof(struct ADB_INFO)*ADB_max);
	ADB_ult=0;

	//		Azzera locazione
	for (a=0;a<ADB_max;a++) {memset(ADB_info+a,0,sizeof(struct ADB_INFO)); ADB_info[a].iHdlRam=-1;}


	// Controlla se si stà girando in rete
#ifdef HLINK
	// Se è attivo il Link
	if (ADB_HL) adb_HLapri();

	//  if (ini_find("adb_network",pt)) return;

	//	Cerca primo spazio
	pt=strstr(pt," ");
	if (pt!=NULL)
	{pt++;
	if (!strcmp(_strupr(pt),"ON"))
	{ADB_network=ON;
#if !defined(_NODC)&&!defined(EH_CONSOLE)
	if (fileCheck(ehPath("net"))) lic_load(ehPath("net"),1,RAM_AUTO);
#endif
	}
	} else ADB_network=OFF;
#endif
	fTransActive=FALSE;
}

void adb_end(INT iStep)
{
	 INT a;
	 if (ADB_hdl==-1) return;

	 //		Chiude tutti i db
	 for (a=0;a<ADB_max;a++) adb_close(a);

	#ifdef HLINK
	 if (ADB_HL) adb_HLchiudi();
	#endif

	 memoFree(ADB_hdl,"*ADB");
	 ADB_hdl=-1;

// Emulazione
#ifdef _ADBEMU
 if (sEmu.hdlFileEmu>0) 
	{adb_EmuClose();
	}
 if (sEmu.lpRows) {ehFree(sEmu.lpRows); sEmu.lpRows=NULL;}
 //if (sEmu.hConn) {SQLFreeHandle(SQL_HANDLE_DBC,sEmu.hConn); sEmu.hConn=0;}
 //if (sEmu.hEnv)  SQLFreeHandle(SQL_HANDLE_ENV,sEmu.hEnv);

#endif

}
//  +-----------------------------------------
//	| ADB_RESET Resetta il gestore BTRIEVE    |
//	|                                         |
//	|  - Chiude tutti i file aperti           |
//	|  - Rilascia i lock occupati dei record  |
//	|  - Chiude una transazione attiva        |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+
void adb_reset(void)
{
	BTRV32(B_RESET,0,0,0,0,0);
}
//   +-------------------------------------------
//   | ADB_RECSIZE Ritorna la grandezza record   |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna > 0 = Grandezza record           |
//   |          < 0 = Errore                     |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_recsize(INT Hdb)
{
	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("recsize",Hdb);

#ifdef _ADBEMU
	//return (ADB_info[Hdb].AdbHead->wRecSize+ADB_info[Hdb].ulRecAddBuffer);
	return ADB_info[Hdb].ulRecBuffer;
#else
	return ADB_info[Hdb].AdbHead->wRecSize;
#endif
}
//   +-------------------------------------------
//   | ADB_KEYMAXSIZE Ritorna la grandezza della |
//   |                chiave + grande            |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna > 0 = Grandezza record           |
//   |          < 0 = Errore                     |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_keymaxsize(INT Hdb)
{
	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("keymaxsize",Hdb);
	return ADB_info[Hdb].AdbHead->KeyMoreGreat;
}
//   +-------------------------------------------
//   | ADB_RECNUM  Ritorna il numero di chiavi   |
//   |             contenute nel db              |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna >=0 : Numero di chiavi           |
//   |          < 0 = Errore                     |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
LONG adb_recnum(INT Hdb)
{
	INT hdl;
	BTI_ULONG ulDataLen;
	CHAR *ADBbuf;
	LONG NRecord;

#pragma pack(1)
	typedef struct {
	 INT16 LenRec;
	 INT16 PageSize;
	 INT16 IndexNum;
	 LONG RecNum;
	 INT16 fileflag;
	 INT16 reserved;
	 INT16 PageNoUsed;
	} B_INFOREC ;
#pragma pack()

	B_INFOREC *databuf;
	BTI_SINT iStatus;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("recnum",Hdb);

#ifdef _ADBEMU
	// Numero di record
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		CHAR *lpQuery,*p;
		SQLUINTEGER lRowsTotal;
		if (ADB_info[Hdb].fQueryPending) {SQLCloseCursor(ADB_info[Hdb].hStmt); ADB_info[Hdb].fQueryPending=FALSE;}
		lpQuery=ehAlloc(EMU_SIZE_QUERY);
		//strcpy(lpBuffer,EmuGetInfo("D.adb_recnum",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint));

		strcpy(lpQuery,"SELECT COUNT(*)");
		p=EmuGetInfo("D.from",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
		if (!p) ehExit("adbrecnum(): From ? in emu");
		strcat(lpQuery," FROM ");
		strcat(lpQuery,p);
		p=EmuGetInfo("D.join",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
		if (p)
		{
			if (*p>32)
			{
				strcat(lpQuery," WHERE ");
				strcat(lpQuery,p);
			}
		}
		//win_infoarg("[%s]",lpBuffer);
		/*
		SQLTRY("SQLExecDirect",
				SQLExecDirect(ADB_info[Hdb].hStmt,
							  EmuGetInfo("D.adb_recnum",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint),
							  SQL_NTS));
							  */
//		adb_ODBCArgExecute(Hdb,"SELECT COUNT(*) FROM ACGEDATV2.ANCL200F"); // Comando
		/*
		if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO) 
		{
			ehExit("adb_recnum: SQL error");	
		}

		SQLTRY(SQL_HANDLE_STMT,"SET1",ADB_info[Hdb].hStmt,SQLSetStmtAttr(ADB_info[Hdb].hStmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET2",ADB_info[Hdb].hStmt,SQLSetStmtAttr(ADB_info[Hdb].hStmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET3",ADB_info[Hdb].hStmt,SQLSetStmtAttr(ADB_info[Hdb].hStmt, SQL_ATTR_ROW_STATUS_PTR, NULL, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET4",ADB_info[Hdb].hStmt,SQLSetStmtAttr(ADB_info[Hdb].hStmt, SQL_ATTR_ROWS_FETCHED_PTR, NULL, 0));
		adb_ODBCArgExecute(Hdb,lpBuffer); // Comando

		_d_("OK1"); Sleep(1000);
 		adb_ODBCNext(Hdb);
		_d_("OK2"); Sleep(1000);
		status=atoi(adb_ODBCFldPtr(Hdb,"00001"));
		SQLCloseCursor(ADB_info[Hdb].hStmt); ADB_info[Hdb].fQueryPending=FALSE;
		*/
		
		SQLBindCol(ADB_info[Hdb].hStmt, 1, SQL_INTEGER, &lRowsTotal, 0, 0);
		//SQLSetStmtAttr(lpSqd->hStmtClone, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0));
		SQLExecDirect(ADB_info[Hdb].hStmt,lpQuery,SQL_NTS);
		SQLFetch(ADB_info[Hdb].hStmt);
		SQLCloseCursor(ADB_info[Hdb].hStmt);
		
		ehFree(lpQuery);
		//win_infoarg("recnum = %d",status);
		return lRowsTotal;
	}
#endif
//	size=128+(1+ADB_info[Hdb].AdbHead->Index)*16+265; // Ci sto' largo
	ulDataLen=128+(1+ADB_info[Hdb].AdbHead->Index)*16+512; // Ci sto' largo
	hdl=memoAlloc(M_HEAP,ulDataLen,"adb_recnum");
	if (hdl<0) win_errgrave("ADB_renum: No memory");
	ADBbuf=memoPtr(hdl,NULL);
	databuf=(B_INFOREC *) ADBbuf;

	ADB_iLastStatus = iStatus = BTRV32(B_STAT,
				   ADB_info[Hdb].lpPosBlock,
				   databuf,// Puntatore ai dati
				   &ulDataLen, // Grandezza dei dati
				   ADBbuf,   // ???> in output
				   0); // ??? in input
	if (iStatus) {BTI_err(Hdb,"STAT:",iStatus);}

	NRecord=databuf->RecNum;
	memoFree(hdl,"adb_recnum");
	if (!iStatus) return NRecord;
	return iStatus;
}
//   +-------------------------------------------
//   | ADB_FINDPERC Ritorna la percentuale       |
//   |             approssimativa della posizione|
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |  Index: Key path usato                    |
//   |         -1 percentuale posizione fisica   |
//   |                                           |
//   |  Ritorna lo status BTRIEVE                |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_findperc(INT Hdb,INT IndexNum,float *Fperc)
{
	BTI_ULONG uiDataLen;
	INT iStatus;
	INT16 *perc;
	BTI_BYTE dataBuf[8];

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("findperc",Hdb);

	// --------------------------------------------------
	// Emulazione di position
	// NOTA:
	// Viene allocato un progressivo che riconduce a un WHERE 
	// da usare per UPDATE o DELETE
	//
#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		win_infoarg("FINDPERC: Non disponibile in emulazione");
		*Fperc=0;
		return 0;
	}
#endif
	uiDataLen=4;
	ADB_iLastStatus = iStatus = BTRV32(45,
				   ADB_info[Hdb].lpPosBlock,
				   &dataBuf, // Record dove andare
				   &uiDataLen,// Lunghezza buffer
				   ADB_info[Hdb].lpKeyBuffer,
				   (WORD) IndexNum);

	perc=(INT16 *) dataBuf;
//	win_infoarg("%d - %d",status,(INT) *perc);
//	sprintf(serv,"Len%d %d indexnum=%d datalen=%d %s %s",
								//*perc,*(perc+1),IndexNum,uiDataLen,dataBuf,ADB_info[Hdb].KeyBuffer);
	//Adispm(0,100,15,1,SET,serv);

	*Fperc=(float) (*perc)/100;

	if (iStatus) BTI_err(Hdb,"FINDPERC:",iStatus);
	return iStatus;
}
//   +-------------------------------------------
//   | ADB_GETPERC Legge un record tramite       |
//   |             percentuale (setta il DB pt)  |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |  Index: Key path usato                    |
//   |         -1 percentuale posizione fisica   |
//   |                                           |
//   |  Ritorna >=0 : Numero di chiavi           |
//   |          < 0 = Errore                     |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_getperc(INT Hdb,INT IndexNum,float fPerc)
{
	BTI_ULONG uiDataLen;
	INT status;
	INT *lpiPerc;
	INT iPerc;
	//CHAR serv[80];

	fPerc*=100; iPerc=(INT) fPerc;
	if (iPerc>10000) iPerc=10000;
	if (iPerc<0)     iPerc=0;

	if (((Hdb<0)||(Hdb>=ADB_max))||
			(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("getperc",Hdb);

	//uiDataLen=ADB_info[Hdb].AdbHead->wRecSize;
rifai:
	/*
	if (ADB_info[Hdb].fBlobPresent) uiDataLen=adb_BlobNewOfs(Hdb,0);
									else
									uiDataLen=ADB_info[Hdb].AdbHead->wRecSize;
*/
	uiDataLen=ADB_info[Hdb].ulRecBuffer;
	lpiPerc=(INT *) ADB_info[Hdb].lpRecBuffer;	*lpiPerc=iPerc;
	status = BTRV32(B_SEEK_PERCENT,
					ADB_info[Hdb].lpPosBlock,
					ADB_info[Hdb].lpRecBuffer, // Record dove andare
					&uiDataLen,// Lunghezza buffer
					ADB_info[Hdb].lpKeyBuffer,
					(WORD) IndexNum);

	// @@@
  	if (ADB_info[Hdb].fBlobPresent==TRUE&&status==22) 
		{
			adb_BufRealloc(Hdb,0,FALSE,"GETPERC",FALSE); 
			if (ADB_info[Hdb].ulRecBuffer>2000000)
				ehExit("adb_getperc %d - Hdb=%d iPerc=%d",ADB_info[Hdb].ulRecBuffer,Hdb,iPerc);
			goto rifai;

		}


	if (status) BTI_err(Hdb,"GETPERC:",status);
	return status;
}

//   +-------------------------------------------
//   | ADB_RECBUF Ritorna il puntatore al buffer |
//   |            per i record                   |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna NULL = Errore                    |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
//void *adb_recbuf(INT Hdb)
void *adb_DynRecBuf(INT Hdb)
{
	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("recbuf",Hdb);
	return ADB_info[Hdb].lpRecBuffer;
}

//   +-------------------------------------------
//   | ADB_KEYBUF Ritorna il puntatore al buffer |
//   |            per le chiave (KeyMoreGreat)   |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna NULL = Errore                    |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
void *adb_keybuf(INT Hdb)
{
	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("keybuf",Hdb);
	return ADB_info[Hdb].lpKeyBuffer;
}

//   +-------------------------------------------
//   | ADB_RecReset  Pulisce il record buffer    |
//   |                                           |
//   |  Hdb : Numero del adb handle              |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
void adb_recreset(INT Hdb)
{
	struct ADB_REC *Field;
	INT a;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("RecReset",Hdb);
	memset(ADB_info[Hdb].lpRecBuffer,0,ADB_info[Hdb].ulRecBuffer);//ADB_info[Hdb].AdbHead->wRecSize); // Pulisce il record

	Field=(struct ADB_REC *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		adb_FldWrite(Hdb,Field[a].desc,"",0);
	}
}

INT adb_GetDynSize(HDB hdb)
{
	BTI_ULONG uiDataLen;
	if (ADB_info[hdb].fBlobPresent) uiDataLen=adb_BlobNewOfs(hdb,0);
									else
									uiDataLen=ADB_info[hdb].AdbHead->wRecSize;
	return (INT) uiDataLen;
}

//   +-------------------------------------------
//   | ADB_INSERT Inserisce un nuovo record      |
//   |                                           |
//   |  Hdb : handle del file ADB                |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |            by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_insert(INT Hdb)
{
	BTI_ULONG uiDataLen;
	BTI_SINT status;

	if (((Hdb<0)||(Hdb>=ADB_max))|| (ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("insert",Hdb);
	// --------------------------------------------------
	// Emulazione di Insert
	//
#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		CHAR *lpSQLCommand;
		SQLHSTMT hstmt;
		CHAR *p,*p1,*p2;
		hstmt=ADB_info[Hdb].hStmt;

		p=EmuGetInfo("D.insert",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
		p=p?p:"0";

		if (*p!='1') ehExit("Insert non autorizzato su Hdb:%d [%s]",Hdb,ADB_info[Hdb].lpFileName);
			
		lpSQLCommand=ehAlloc(EMU_SIZE_QUERY);

		EmuInsertBuilder(Hdb,lpSQLCommand);

		//win_infoarg("Insert:SQL[%s]",lpSQLCommand);
		p1=lpSQLCommand;
		while (TRUE)
		{
			p2=strstr(p1,"\3"); if (p2) *p2=0;
			if (ADB_info[Hdb].fQueryPending) {SQLCloseCursor(hstmt); ADB_info[Hdb].fQueryPending=FALSE;}
		//	win_infoarg("Insert:SQL[%s]",p1);
#ifdef _DEBUG
			//dispx("INSERT: %s",p1);
			ehPrintd("%s" CRLF,p1);
#endif
			if (*p1) 
			{
				SQLRETURN rc;
				rc=LSQLExecuteError(ADB_info[Hdb].hStmt,p1);
				if (rc != SQL_SUCCESS) 
				{ 
					if (rc != SQL_SUCCESS_WITH_INFO) 
					{
						if (SQLCheckError(hstmt,rc,"|-803|")) {ehFree(lpSQLCommand); return 5;} // Chiave duplicata
						ehExit("adb_insert: %x-%d\n%s\n%s",rc,rc,lpSQLCommand,SQLDisplay(SQL_HANDLE_STMT,"adb_insert",hstmt,rc));
					}
				} 
			}
			if (p2) p1=p2+1; else break;
		}
		
		// win_infoarg("Insert:SQL[%s]",lpSQLCommand);
		//LSQLExecute("Insert",ADB_info[Hdb].hStmt,lpSQLCommand);
		//if (ADB_info[Hdb].fQueryPending) {SQLCloseCursor(hstmt); ADB_info	[Hdb].fQueryPending=FALSE;}
		ehFree(lpSQLCommand);
		return 0;
	}
#endif

	rifai:
	if (ADB_info[Hdb].fBlobPresent) uiDataLen=adb_BlobNewOfs(Hdb,0); else uiDataLen=ADB_info[Hdb].AdbHead->wRecSize;
	if (!uiDataLen) ehExit("adb_insert: !uiDataLen");

#ifdef _ADB32_BLOB_CTRL
	adb_BlobControl(Hdb);	
#endif

/*
	win_infoarg("Blob ? %s: Insert [Size %d : ulRecBuffer=%d]",
				ADB_info[Hdb].fBlobPresent?"SI":"NO",
				uiDataLen,
				ADB_info[Hdb].ulRecBuffer);
*/
	if (uiDataLen>60000) win_infoarg("Attenzione: uiDataLen =%d",uiDataLen);
	status = BTRV32(B_INSERT,
				   ADB_info[Hdb].lpPosBlock,
				   ADB_info[Hdb].lpRecBuffer,// Puntatore ai dati
				   &uiDataLen, // Grandezza dei dati
				   ADB_info[Hdb].lpKeyBuffer,   // ???> in output
				   0); // ??? in input

	if (status==5) goto AVANTI;

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {
#if defined(_NODC)||defined(EH_CONSOLE)
		goto rifai;
#else
		if (adb_LockControl(ON,Hdb,status,"Vietato l'inserimento.",OFF)) goto rifai;
#endif
	 }
	 else
	 {
		if (status) {BTI_err(Hdb,"INSERT:",status);}
	 }

	AVANTI:
#if !defined(_NODC)&&!defined(EH_CONSOLE)
	adb_LockControl(OFF,0,0,"",OFF);
#endif
	return status;
}
//	 FldPtr=adb_AFldInfo(Hook[a].pTableFieldName,&Fld);

//   +-------------------------------------------
//   | adbFieldPtr Ritorna il puntatore al campo  |
//   |                                           |
//   |  Nome : nome del campo da cercare         |
//   |  Field: pt a struttura ADB_REC            |
//   |                                           |
//   |  Ritorna Ptr campo nel record             |
//   |          NULL il campo non esiste         |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+

#ifdef _ADB32P
void * adb_FldPtr(INT Hdb,CHAR *lpField)
{
	BYTE *p;
	struct ADB_REC *Fld; 
	p=adb_FldInfo(Hdb,lpField,&Fld);
	if (!p) return NULL;
	if (Fld->tipo!=ADB_ALFA &&
		Fld->tipo!=ADB_DATA) 
	{
		#ifdef _DEBUG
			win_infoarg("Il campo %s hdb:%d non e' alfanumerico. (%d)",lpField,Hdb);
			_BREAK_POINT_
		#else
				ehExit("Il campo %s hdb:%d non e' alfanumerico. (%d)",lpField,Hdb,Fld->tipo);
		#endif
	}
	return p;
}
#endif

void * adbFieldPtr(INT Hdb,CHAR *lpField)
{
	void *p;
	struct ADB_REC *Fld; 

	p=adb_FldInfo(Hdb,lpField,&Fld);

// Cerco il Fantasma
#ifdef _ADBEMU
	if (p==NULL)
	{
		EMUBIND *lpEmuBind;
		lpEmuBind=AdbBindInfo(Hdb,lpField,TRUE); if (lpEmuBind) return (ADB_info[Hdb].lpRecBuffer+lpEmuBind->iGhostOffset);
	}
#endif

	if (p==NULL)
	{
		ehExit("adbFieldPtr: %s ? in %s",lpField,ADB_info[Hdb].lpFileName);
	}
	return p;
}

static double dDm[]={1,
					 10,
					 100,
					 1000,
					 10000,
					 100000,
					 1000000,
					 10000000,
					 100000000,
					 1000000000,
					 10000000000,
					 100000000000,
					 1000000000000,
					 10000000000000,
					 100000000000000};
																						 
CHAR * adb_MakeNumeric(double dValore,INT iSize,INT iDec,BOOL fView,CHAR * pszWho)
{
	static CHAR Cr1[80];
	CHAR Cr2[80];
	CHAR Cr3[80];
	INT iPrecision; 
	CHAR *lpPoint;
	double dpValore;
	double dSegno;

	if (iDec<0) iDec=0;
	if (dValore<0) {dpValore=-dValore;dSegno=-1;} else {dpValore=dValore; dSegno=1;}

	//win_infoarg("%f",((dpValore-floor(dpValore))*dDm[iDec]));
	iSize-=1; // 0
	iPrecision=iSize; if (iDec) iPrecision-=(iDec+1); // Tolgo i decimali + il punto
//	sprintf(Cr3,"%.8f",dValore);
	sprintf(Cr3,"%f",dValore); strcat(Cr3,"000000");
	lpPoint=strstr(Cr3,"."); if (!lpPoint) ehExit("?.?");
	*lpPoint=0; lpPoint++;
	if (iDec) {
				if (iPrecision>9) 
				{
				 sprintf(Cr2,"%%%d.0f.%%0%dld",iPrecision,iDec);
				 lpPoint[iDec]=0;
				 sprintf(Cr1,Cr2,atof(Cr3),atol(lpPoint));
				}
				else
				{
				 sprintf(Cr2,"%%%dld.%%0%dld",iPrecision,iDec);
				 lpPoint[iDec]=0;
				 sprintf(Cr1,Cr2,atol(Cr3),atol(lpPoint));
				}
				//win_infoarg("IDec:Cr1=%s/Cr3=%s",Cr1,Cr3);
				}
			  else
//			  {sprintf(Cr2,"%%%dld",iPrecision);
			  {if (iPrecision>9) 
					{
					 sprintf(Cr2,"%%%d.0f",iPrecision); 
					 sprintf(Cr1,Cr2,atof(Cr3));
					}
					else 
					{
					 sprintf(Cr2,"%%%dld",iPrecision);
					 sprintf(Cr1,Cr2,atol(Cr3));
					}
				//win_infoarg("NoDec:Precision=%d Cr1=%s/Cr3=%s",iPrecision,Cr1,Cr3);
			  }
/*
	if (iDec) {
				sprintf(Cr2,"%%%df.%%0%dld",iPrecision,iDec);
				lpPoint[iDec]=0;
				win_infoarg("cr3=%d",Cr3);
				sprintf(Cr1,Cr2,atof(Cr3),atol(lpPoint));
				}
			  else
			  {sprintf(Cr2,"%%%dld",iPrecision);
			   sprintf(Cr1,Cr2,atol(Cr3));
			  }
*/
	if ((INT) strlen(Cr1)!=iSize) 
	{  
		if (fView)
		{
			win_infoarg("adb_MakeNumeric([%s])\nCr1=[%s]\nCr2=[%s]\nCr3=[%s]\niSize=%d\nDec=%d\ndValore=%.7f",
						strEver(pszWho),
						Cr1,
						Cr2,
						Cr3,
						iSize,
						iDec,
						dValore);
		}   
		Cr1[iSize]=0;
		//return NULL; 
	}
	return Cr1;
}
// -------------------------------------------------------------------------------------------
// Gestione Cobol DECIMAL
// -------------------------------------------------------------------------------------------

BYTE *adb_MakeCobolDecimal(double dValore,INT iSize,INT iDec)
{
  static BYTE Digit[80];
  BYTE Cr1[80];
  BYTE bByte;
  CHAR Cr2[50];
  INT iSizeReal=(iSize<<1)-1;
  double dpValore;
  INT pb;

  if (dValore<0) dpValore=-dValore; else dpValore=dValore;
  // *  *
  // dd dn
  sprintf(Cr2,"%%0%d.0f",iSizeReal);
  sprintf(Cr1,Cr2,dpValore*dDm[iDec]);
 //win_infoarg("%f [%s][%s]",dpValore,Cr1,Cr2);
  if (dValore<0) strcat(Cr1,"="); else strcat(Cr1,"?");
  if ((INT) strlen(Cr1)!=(iSizeReal+1)) return NULL; 

  //win_infoarg("[%s]",Cr1);

  // ---------------------------------------------------
  // Compatto in digit a 4 bit
  //
  ZeroFill(Digit);
  for (pb=0;pb<(iSize<<1);pb++)
  {
	bByte=Cr1[pb]-'0';
	//win_infoarg("<%d>",(INT) bByte);
	if (pb%2) Digit[pb>>1]|=(bByte&0xF); else Digit[pb>>1]=bByte<<4; 
	//win_infoarg("Digit %d=[%d]",(INT) (pb>>1),(INT) (Digit[pb>>1]));
  }
 return Digit;
}

double adb_GetCobolDecimal(BYTE *lpStr,INT iSize,INT iControlDigit,INT iDec)
{
	INT a;
	BYTE bByte;
	double dSegno;
	INT iSizeVirtual;
	double Mul;
	double dValore=0;
	double dMaxValue;

	bByte=lpStr[iSize-1]&0xF;

//	win_infoarg("Test [%d] %d %d %d",(INT) bByte, (INT) lpStr[0],(INT) lpStr[1],(INT) lpStr[2]);
	if (bByte==0xD) dSegno=-1; else dSegno=1;

	iSizeVirtual=(iSize<<1)-1;
	Mul=1;
	dValore=0;
	for (a=(iSizeVirtual-1);a>-1;a--)
	{
		bByte=lpStr[a>>1];
		//win_infoarg("DECOD:a=%d pb=%d Byte=%d [Valore=%d]",a,a>>1,(INT) bByte,(INT) (a%2)?(bByte&0xF):(bByte>>4));
	    //if (a%2) bByte>>=4; else bByte&=4;
		bByte=(a%2)?(bByte&0xF):(bByte>>4);
		dValore+=(double) bByte*Mul; Mul*=10;
	}

	dValore/=dDm[iDec];
	dMaxValue=dDm[iControlDigit];//pow(10,(iControlDigit-iDec));
//	win_infoarg(">Prima %f (%d,%d,%d)",dValore,iSize,iControlDigit,iDec);
	if (fCobolLimitControl)
	{
		if (dValore>=dMaxValue) dValore=0; // Controllo del 9/2007 per valore errati nel dbase
	}
		//	win_infoarg("<Dopo %f:%f",dValore,dMaxValue);

	dValore*=dSegno;
	
	return dValore;
}


// -------------------------------------------------------------------------------------------
// Gestione Cobol NUMERIC
// -------------------------------------------------------------------------------------------
static CHAR *lpCNPositive="0ABCDEFGHI";
static CHAR *lpCNNegative="}JKLMNOPQR";

CHAR *adb_MakeCobolNumeric(double dValore,INT iSize,INT iDec)
{
 static CHAR Cr1[80];
 CHAR Cr2[50];
 
 double dpValore;
// dValore*=-1; // DA TOGLIERE ASSOLUTAMENTE
 if (dValore<0) dpValore=-dValore; else dpValore=dValore;
 sprintf(Cr2,"%%0%d.0f",iSize);
 sprintf(Cr1,Cr2,dpValore*dDm[iDec]);
 if ((INT) strlen(Cr1)!=iSize) return NULL; 
 
 // Codifico l'ultimo valore
 //win_infoarg("Cr1Prima[%s]",Cr1);
 if (dValore<0) Cr1[iSize-1]=lpCNNegative[Cr1[iSize-1]-'0'];
				else
				Cr1[iSize-1]=lpCNPositive[Cr1[iSize-1]-'0'];
 //win_infoarg("Cr1Dopo [%s]",Cr1);
 return Cr1;
}


double adb_GetCobolNumeric(BYTE *lpStr,INT Size,INT iControlDigit,INT iDec)
{
	CHAR *lpApp;
	CHAR *p;
	CHAR cFind;
	CHAR bCar;
	double dSegno;
	double dValore;
	double dMaxValue;

	if (!*lpStr) return 0;

	lpApp=ehAlloc(Size+1);
	memcpy(lpApp,lpStr,Size); lpApp[Size]=0;
	cFind=lpStr[Size-1];
	
	// --------------------------------------
	// Decodifico il carattere negativo
	p=strchr(lpCNNegative,cFind); 
	dSegno=1;
	if (p!=NULL)
	{
		if (cFind==*lpCNNegative) bCar='0'; else bCar=cFind-'J'+'1';
		lpApp[Size-1]=bCar;
		dSegno=-1;
	}

	// --------------------------------------
	// Decodifico il carattere positivo
	if (dSegno==1)
	{
	 p=strchr(lpCNPositive,cFind); 
	 if (p!=NULL)
	 {
		if (cFind==*lpCNPositive) bCar='0'; else bCar=cFind-'A'+'1';
		lpApp[Size-1]=bCar;
	 }
	}

	// --------------------------------------
	// Conversione in double
	dValore=atof(lpApp)/dDm[iDec]; 
	dMaxValue=dDm[iControlDigit];//pow(10,(iControlDigit-iDec));
	if (fCobolLimitControl)
	{
		if (dValore>=dMaxValue) dValore=0; // Controllo del 9/2007 per valore errati nel dbase
	}
	dValore*=dSegno;

	ehFree(lpApp);
	return dValore;
}

//   +-------------------------------------------
//   | ADB_FldNume  Ritorna il numero contenuto  |
//   |              in un campo                  |
//   |                                           |
//   |  Hdb  : Handle ADB                        |
//   |  Nome : nome del campo da cercare         |
//   |                                           |
//   |  Ritorna Valore del campo                 |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+

double adb_FldNume(INT Hdb,CHAR *NomeFld)
{
	struct ADB_REC *Fld;
	CHAR *FldPtr;
	INT16 *lpInt16;
	float *ptFloat;
	LONG  *lpLong;

	FldPtr=adb_FldInfo(Hdb,NomeFld,&Fld);

// Cerco il Fantasma
#ifdef _ADBEMU
	if (FldPtr==NULL)
	{
		EMUBIND *lpEmuBind;
		lpEmuBind=AdbBindInfo(Hdb,NomeFld,TRUE); if (lpEmuBind) return atof(ADB_info[Hdb].lpRecBuffer+lpEmuBind->iGhostOffset);
	}
#endif

	if (FldPtr==NULL)
		 {
			ehExit("fldNume: %s ?[%d]",NomeFld,Hdb);
		 }

	switch (Fld->tipo)
	 {
 		 case ADB_NUME : return atof(FldPtr);
		 case ADB_COBD : return adb_GetCobolDecimal(FldPtr,Fld->RealSize,Fld->size,Fld->tipo2);
		 case ADB_COBN : return adb_GetCobolNumeric(FldPtr,Fld->RealSize,Fld->size,Fld->tipo2);

		 case ADB_BOOL :
		 case ADB_INT  : lpInt16=(INT16 *) FldPtr;
						 return (double) *lpInt16;
#ifdef _WIN32
		 case ADB_INT32: return (double) * (INT *) FldPtr;
#endif

		 case ADB_FLOAT: ptFloat=(float *) FldPtr;
						 return (double) *ptFloat;

		 case ADB_AINC:  lpLong=(LONG *) FldPtr;
						 return (double) *lpLong;

		 case ADB_ALFA :
		 case ADB_BLOB :
		 case ADB_DATA :
		 default:
			ehExit("FldNume:Field Non numerico %s ?[%d] [%d]",NomeFld,Hdb,Fld->tipo);
		 }
 return 0;
}

//   +-------------------------------------------
//   | ADB_FldCopy Copia in un buffer un dato    |
//   |             convertito ADB                |
//   |                                           |
//   |  Buff : Buffer che riceverà il dato       |
//   |  Hdb  : Handle dbase interessato          |
//   |  Nome : Nome del campo da convertire      |
//   |  DatoAlfa : Dato da scrivere alfanumerico |
//   |  DatnNume : Dato da scrivere numerico     |
//   |                                           |
//   |  Ritorna NULLA                            |
//   |                                           |
//   |             by Ferrà Art & Technology 1997 |
//   +-------------------------------------------+
void adb_FldCopy(void *FldPtr,INT Hdb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume)
{
	struct ADB_REC *Fld;
	INT16  *lpInt16;
	CHAR   *ptr,*pp;
	float  *ptFloat;
	CHAR cr1[80],cr2[80];
	LONG   *lpLong;

	pp=adb_FldInfo(Hdb,NomeFld,&Fld);
	if (pp==NULL)
		{ehExit("FldCopy:Field ? [%s]",NomeFld);
		 //ehExit(serv);
		 }

	memset(FldPtr,0,Fld->RealSize); // Azzera il campo

	switch (Fld->tipo)
	 {
		 case ADB_ALFA:
		 case ADB_BLOB:
						if (strlen(DatoAlfa)>(WORD) Fld->size)
						{ehExit("Hdb:%d, FldCopy: > Field |%s| [%d]>[%d] \n[%s]",
								Hdb, NomeFld,strlen(DatoAlfa),(INT)Fld->size,
										 DatoAlfa);
											//win_errgrave(serv);
										 }
									 strcpy(FldPtr,DatoAlfa);
									 break;

		 case ADB_DATA : if (strlen(DatoAlfa)==0) {memset(FldPtr,0,9); break;}
//										 if (strlen(DatoAlfa)!=Fld->size) win_errgrave("FldCopy:Input != DataField");
						 if (strlen(DatoAlfa)!=8)
								{ehExit("FldCopy:Input != 8 \n[%d:%s|%s]",Hdb,NomeFld,DatoAlfa);
									//win_errgrave(serv);
								}
																					//win_errgrave("FldCopy:Input != 8");
						 strcpy(cr1,DatoAlfa);
						 memcpy(cr2,cr1+4,4);// Anno
						 memcpy(cr2+4,cr1+2,2);// Mese
						 memcpy(cr2+6,cr1,2);// Giorno
						 cr2[8]=0;
						 memcpy(FldPtr,cr2,9);
						 break;

		 case ADB_NUME : ptr=adb_MakeNumeric(DatoNume,Fld->RealSize,Fld->tipo2,TRUE,NomeFld);
						 if (ptr==NULL) ehExit("FldCopy:MakeNume!=Fld.Size [%s=%.3f/%d/%d] Hdb=%d",
												 NomeFld,DatoNume,Fld->RealSize,Fld->tipo2,Hdb);
						 strcpy(FldPtr,ptr); 
						 break;

		 case ADB_COBD : ptr=adb_MakeCobolDecimal(DatoNume,Fld->RealSize,Fld->tipo2);
						 if (ptr==NULL) ehExit("FldCopy:IptSize!=Fld.Size (CobolDecimal) [%s][%.2f]",NomeFld,DatoNume);
						 memcpy(FldPtr,ptr,Fld->RealSize); 
						 break;

		 case ADB_COBN : ptr=adb_MakeCobolNumeric(DatoNume,Fld->RealSize,Fld->tipo2);
						 if (ptr==NULL) ehExit("FldCopy:IptSize!=Fld.Size (CobolNumeric)");
						 memcpy(FldPtr,ptr,Fld->RealSize); 
						 break;
		 
		 case ADB_AINC:  // AutoIncremento
						 lpLong=(UINT *) FldPtr;
						 *lpLong=(UINT) DatoNume;
						 break;

		 case ADB_BOOL :
		 case ADB_INT  : lpInt16=(INT16 *) FldPtr;
						 *lpInt16=(INT16) DatoNume;
						 break;
#ifdef _WIN32
		 case ADB_INT32: * (UINT *) FldPtr=(INT) DatoNume;
		                 break;
#endif
		 case ADB_FLOAT: ptFloat=(float *) FldPtr;
						 *ptFloat=(float) DatoNume;
						 break;
		 }
}

//   +-------------------------------------------
//   | ADB_FldCopy Copia in un buffer un dato    |
//   |             convertito ADB                |
//   |                                           |
//   |  Buff : Buffer che riceverà il dato       |
//   |  Hdb  : Handle dbase interessato          |
//   |  Nome : Nome del campo da convertire      |
//   |  DatoAlfa : Dato da scrivere alfanumerico |
//   |  DatnNume : Dato da scrivere numerico     |
//   |                                           |
//   |  Ritorna FALSE = TUTTOK OK                |
//   |          TRUE  = ERRORE                   |
//   |             by Ferrà Art & Technology 1997 |
//   +-------------------------------------------+
BOOL adb_FldWriteCheck(INT Hdb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume,BOOL fView,INT *lpiError)
{
	struct ADB_REC *Fld;
	CHAR   *ptr,*pp;

	pp=adb_FldInfo(Hdb,NomeFld,&Fld);
	if (pp==NULL) 
	{
	 if (fView) win_infoarg("FldCopy:Field ? [%s]",NomeFld); 
	 if (lpiError) *lpiError=1;
	 return TRUE;
	}

	switch (Fld->tipo)
	 {
		 case ADB_BLOB:
						break;
		 case ADB_ALFA:
						if (strlen(DatoAlfa)>(WORD) Fld->size)
						{
							if (fView) win_infoarg("FldCopy: > Field\n%s",NomeFld);
							if (lpiError) *lpiError=2;
							return TRUE;
						}
						break;

		 case ADB_DATA : if (strlen(DatoAlfa)==0) break;
						 if (strlen(DatoAlfa)!=8)
						 {//ehExit("FldCopy:Input != 8 \n[%d:%s|%s]",Hdb,NomeFld,DatoAlfa);
							if (fView) win_infoarg("FldCopy:Input != 8 \n[%d:%s|%s]",Hdb,NomeFld,DatoAlfa);
							if (lpiError) *lpiError=3;
							return TRUE;
						 }
						 break;

		 case ADB_NUME : ptr=adb_MakeNumeric(DatoNume,Fld->RealSize,Fld->tipo2,FALSE,NomeFld);
						 if (ptr==NULL)
						 {
							if (fView) win_infoarg("FldCopy:MakeNume!=Fld.Size [%s=%.3f/%d/%d] Hdb=%d",NomeFld,DatoNume,Fld->RealSize,Fld->tipo2,Hdb);
							if (lpiError) *lpiError=4;
							return TRUE;
						 }
						 break;

		 case ADB_COBD : ptr=adb_MakeCobolDecimal(DatoNume,Fld->RealSize,Fld->tipo2);
						 //if (ptr==NULL) ehExit("FldCopy:IptSize!=Fld.Size (CobolDecimal)");
						 if (ptr==NULL)
						 {
							if (fView) win_infoarg("FldCopy:IptSize!=Fld.Size (CobolDecimal)");
							if (lpiError) *lpiError=5;
							return TRUE;
						 }
						 break;

		 case ADB_COBN : ptr=adb_MakeCobolNumeric(DatoNume,Fld->RealSize,Fld->tipo2);
						 //if (ptr==NULL) ehExit("FldCopy:IptSize!=Fld.Size (CobolNumeric)");
						 if (ptr==NULL)
						 {
							if (fView) win_infoarg("FldCopy:IptSize!=Fld.Size (CobolNumeric)");
							if (lpiError) *lpiError=6;
							return TRUE;
						 }
						 break;
		 
		 case ADB_AINC:  // AutoIncremento
						 break;

		 case ADB_BOOL :
		 case ADB_INT  : 
						 break;
#ifdef _WIN32
		 case ADB_INT32: 
		                 break;
#endif
		 case ADB_FLOAT: 
						 break;
		 }
	return FALSE;
}

//   +-------------------------------------------
//   | ADB_FldWrite Scrive (se si può) un campo  |
//   |              del record                   |
//   |                                           |
//   |  Nome : nome del campo da scriviere       |
//   |  Field: pt a struttura ADB_REC            |
//   |  DatoAlfa : Dato da scrivere alfanumerico |
//   |  DatnNume : Dato da scrivere numerico     |
//   |                                           |
//   |  Ritorna Ptr campo nel record             |
//   |          NULL il campo non esiste         |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
void adb_FldWrite(INT Hdb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume)
{
	struct ADB_REC *Fld;
	CHAR *FldPtr;

	FldPtr=adb_FldInfo(Hdb,NomeFld,&Fld);
	if (FldPtr==NULL) {
		ehAlert("FldWrite:Field ? [%s] in %s:%d",NomeFld,ADB_info[Hdb].lpFileName,Hdb);
		ehExit("FldWrite:Field ? [%s] in hdb %d",NomeFld,Hdb);
	
	}

	if (Fld->tipo==ADB_BLOB)
	{
		if (!DatoAlfa) return;
		adb_BlobFldWrite(Hdb,NomeFld,DatoAlfa);
		return;
	}
	adb_FldCopy(FldPtr,Hdb,NomeFld,DatoAlfa,DatoNume);
}

#ifdef _WIN32
BOOL adb_FldExist(INT Hdb,CHAR *NomeFld)
{
	struct ADB_REC *Fld;
	CHAR *FldPtr;
	FldPtr=adb_FldInfo(Hdb,NomeFld,&Fld);
	if (FldPtr==NULL) return FALSE; else return TRUE;
}

BOOL adb_FldImport(HDB hdbDest,HDB hdbSource,BOOL fRealloc,BOOL fCutSpace)
{
	INT a;
	CHAR *pt;
	struct ADB_HEAD *HeadDest;
	struct ADB_REC *RecordDest;
	struct ADB_REC *FldSource;
	CHAR *pdata;
	INT iError;

	HeadDest=ADB_info[hdbDest].AdbHead;
	RecordDest=(struct ADB_REC *) (HeadDest+1);

	// Richiesta Preventiva
	if (fRealloc) adb_BufRealloc(hdbDest,ADB_info[hdbSource].ulRecBuffer,FALSE,"IMPORT",FALSE);

	// 
	// Loop sul dizionario campi dell'Hdb di destinazione
	//
	
	for (a=0;a<HeadDest->Field;a++) 
	{
		pt=adb_FldInfo(hdbSource,RecordDest[a].desc,&FldSource);
		// ------------------------------------------------------
		// Nuovo Nome campo inesistente nel file sorgente
		//
		if (pt==NULL) 
		{
#ifdef _ADBEMU
			//
			// Campi Fantasma
			//
			EMUBIND *lpEmuBind;
			lpEmuBind=AdbBindInfo(hdbSource,RecordDest[a].desc,FALSE);
			if (!strcmp(RecordDest[a].desc,"ATTI")) win_infoarg("qui");
			if (lpEmuBind) 
			{
				BYTE *pValue=ADB_info[hdbSource].lpRecBuffer+lpEmuBind->iGhostOffset;
			//	win_infoarg("%s=%s",RecordDest[a].desc,pValue);
				 switch (RecordDest[a].tipo)
				 {
						case ADB_ALFA: // <--- Destinazione
						case ADB_BLOB: // <--- Destinazione
//						case ADB_DATA: // <--- Destinazione
							 adb_FldWrite(hdbDest,RecordDest[a].desc,pValue,0);
							 break;

						case ADB_INT:
						case ADB_INT32:
						case ADB_NUME:
						case ADB_COBN:
						case ADB_COBD:
						case ADB_AINC:
							 adb_FldWrite(hdbDest,RecordDest[a].desc,NULL,atof(pValue));
							break;
				 }
				/// return FALSE;
				//return (ADB_info[Hdb].lpRecBuffer+lpEmuBind->iGhostOffset);
			}
			else
#endif

			adb_FldReset(hdbDest,RecordDest[a].desc);
		}
		else
		{
		 switch (RecordDest[a].tipo)
		 {
			// da ALFA --> ALFA
			// da BLOB --> ALFA
			case ADB_ALFA: // <--- Destinazione
			
				// Tipi differenti
				if (FldSource->tipo!=RecordDest[a].tipo&&(FldSource->tipo!=ADB_BLOB)) return TRUE; // Campi differenti	
				
				// Se è' una stringa la ridimendiono se è il caso
				{
				  CHAR *lpSorg;
				  if (FldSource->tipo==ADB_BLOB) lpSorg=adb_BlobGetAlloc(hdbSource,RecordDest[a].desc);
												 else 
												 lpSorg=adbFieldPtr(hdbSource,RecordDest[a].desc);

			      // Taglio se è troppo lungo
				  if ((INT) strlen(lpSorg)>=adb_FldSize(hdbDest,RecordDest[a].desc))
				  {
					*(lpSorg+adb_FldSize(hdbDest,RecordDest[a].desc)-1)=0;
				  }

				 if (fCutSpace) strTrimRight(lpSorg);
				 if ((INT) strlen(lpSorg)>RecordDest[a].size) lpSorg[RecordDest[a].size]=0; // new 2005
				 adb_FldWrite(hdbDest,RecordDest[a].desc,lpSorg,0);
				 if (FldSource->tipo==ADB_BLOB) adb_BlobFree(lpSorg);
				}
				//win_infoarg("Ci passo [%s] %d->%d",RecordDest[a].desc,FldSource->tipo,RecordDest[a].tipo);
				break;
			
			// da BLOB --> BLOB
			// da ALFA --> BLOB
			case ADB_BLOB: // Tipo destinazione
				
				// Tipi differenti
				if (FldSource->tipo!=RecordDest[a].tipo&&(FldSource->tipo!=ADB_ALFA)) return TRUE; // Campi differenti	
				{
				  CHAR *lpSorg;

				  if (FldSource->tipo==ADB_BLOB) lpSorg=adb_BlobGetAlloc(hdbSource,RecordDest[a].desc);
												 else 
												 lpSorg=adbFieldPtr(hdbSource,RecordDest[a].desc);
				
				  if (fCutSpace) strTrimRight(lpSorg);
				  adb_BlobFldWrite(hdbDest,RecordDest[a].desc,lpSorg);
				  if (FldSource->tipo==ADB_BLOB) adb_BlobFree(lpSorg);
				}
				break;

			case ADB_DATA:
				// Tipi differenti
				if (FldSource->tipo!=RecordDest[a].tipo)
				{
					return TRUE; // Campi differenti
				}	
				pdata=adbFieldPtr(hdbDest,RecordDest[a].desc);
				memcpy(pdata,adbFieldPtr(hdbSource,RecordDest[a].desc),8);
				*(pdata+8)=0; // 0 Finale
				break;
				
			case ADB_BOOL:

				// Tipi differenti da ...
				if ((FldSource->tipo!=ADB_INT)&&
					(FldSource->tipo!=ADB_INT32)&&
					(FldSource->tipo!=ADB_BOOL)&&
					(FldSource->tipo!=ADB_NUME)&&
					(FldSource->tipo!=ADB_COBN)&&
					(FldSource->tipo!=ADB_COBD)&&
					(FldSource->tipo!=ADB_AINC))
					{
						return TRUE; // Campi differenti
					}	
				
					if (adb_FldInt(hdbSource,RecordDest[a].desc))
									adb_FldWrite(hdbDest,RecordDest[a].desc,NULL,1);
									else
									adb_FldWrite(hdbDest,RecordDest[a].desc,NULL,0);
					break;

			case ADB_INT:
			case ADB_INT32:
			case ADB_NUME:
			case ADB_COBN:
			case ADB_COBD:
			case ADB_AINC:

				// Tipi differenti
				if ((FldSource->tipo!=ADB_INT)&&
					(FldSource->tipo!=ADB_INT32)&&
					(FldSource->tipo!=ADB_BOOL)&&
					(FldSource->tipo!=ADB_NUME)&&
					(FldSource->tipo!=ADB_COBN)&&
					(FldSource->tipo!=ADB_COBD)&&
					(FldSource->tipo!=ADB_AINC))
					{
						return TRUE; // Campi differenti
					}	

				if (adb_FldWriteCheck(hdbDest,RecordDest[a].desc,NULL,adb_FldNume(hdbSource,RecordDest[a].desc),TRUE,&iError))
				{
#if !defined(_NODC)&&!defined(EH_CONSOLE)
					if (win_ask("Proseguo con l'importazione ?")==ESC) return TRUE;
#endif
				}
				else
				{
				  adb_FldWrite(hdbDest,RecordDest[a].desc,NULL,adb_FldNume(hdbSource,RecordDest[a].desc));
				}

				break;
				
			case ADB_FLOAT:
				// Tipi differenti
				if (FldSource->tipo!=RecordDest[a].tipo)
				{
					return TRUE; // Campi differenti
				}	
				memcpy(adbFieldPtr(hdbDest,RecordDest[a].desc),adbFieldPtr(hdbSource,RecordDest[a].desc),4);
				break;
		 } // Switch
		} // Else
	}// For
 return FALSE;
}
#endif

//   +-------------------------------------------
//   | ADB_FldOffset Ritorna l'offset di spiazza |
//   |               mento del campo nel record  |
//   |                                           |
//   |  Nome : nome del campo da cercare         |
//   |  Field: pt a struttura ADB_REC            |
//   |                                           |
//   |  Ritorna Offset campo nel record          |
//   |          PrgEnd il campo non esiste       |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_FldOffset(INT Hdb,CHAR *lpField)
{
	struct ADB_REC *Fld;
	void *lp;

	lp=adb_FldInfo(Hdb,lpField,&Fld);

// Cerco il Fantasma
#ifdef _ADBEMU
	if (lp==NULL)
	{
		EMUBIND *lpEmuBind;
		lpEmuBind=AdbBindInfo(Hdb,lpField,TRUE); if (lpEmuBind) return lpEmuBind->iGhostOffset;
	}
#endif

	if (lp==NULL) ehExit("FldOffset:Campo inesistente [hdb:%d] %s",Hdb,lpField);

	return Fld->pos;
}
//   +-------------------------------------------
//   | ADB_FldSize   Ritorna il size reale       |
//   |               del campo                   |
//   |                                           |
//   |  Nome : nome del campo da cercare         |
//   |  Field: pt a struttura ADB_REC            |
//   |                                           |
//   |  Ritorna Grandezza del campo nel record   |
//   |          NULL il campo non esiste         |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_FldSize(INT Hdb,CHAR *NomeFld)
{
	struct ADB_REC *Fld;
	if (adb_FldInfo(Hdb,NomeFld,&Fld)==NULL) ehExit("FldSize:Campo inesistente[%s]",NomeFld);
	return Fld->RealSize;
}

//   +-------------------------------------------
//   | ADB_FldReset  Pulisce un campo nome       |
//   |                                           |
//   |  Hdb : Numero del adb handle              |
//   |  NomeField : Nome del campo da cercare    |
//   |                                           |
//   |  Ritorna 0  = Tutto OK                    |
//   |          -1 = il campo non esiste         |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_FldReset(INT Hdb,CHAR *NomeFld)
{
	struct ADB_REC *Fld;
	CHAR *FldPtr;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("FldReset",Hdb);

	FldPtr=adb_FldInfo(Hdb,NomeFld,&Fld);
	if (FldPtr==NULL) return -1;
	//memset(FldPtr, 0, Fld->RealSize); // Pulisce il record
	adb_FldWrite(Hdb,NomeFld,"",0);
	return 0;
}

//   +-------------------------------------------
//   | ADB_FldInfo   Cerca un campo nome         |
//   |                                           |
//   |  Hdb : Numero del adb handle              |
//   |  NomeField : Nome del campo da cercare    |
//   |  Fldinfo: pt a un *ADB_REC                |
//   |           ritorna pt ad area info campo   |
//   |                                           |
//   |  Ritorna Ptr campo nel record             |
//   |          NULL il campo non esiste         |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
CHAR *adb_FldInfo(INT Hdb,CHAR *NomeFld,struct ADB_REC ** psFldInfo)
{
	struct ADB_REC *Field;
	INT a;

	if (((Hdb<0) || (Hdb>=ADB_max)) || (ADB_info[Hdb].iHdlRam==-1)) ehExit("FldInfo [%s][Hdb] %d",NomeFld,Hdb);

	Field=(struct ADB_REC *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		if (!strcmp(Field[a].desc,NomeFld))
		{
			if (psFldInfo) *psFldInfo=Field+a;
			return ADB_info[Hdb].lpRecBuffer+Field[a].pos;
		}
	}

	return NULL;
}

//   +-------------------------------------------------
//   | ADB_HOOKTOUCH  Controlla se dei campi descritti|
//   |                in HOOK qualcosa è cambiato     |
//   |                                                |
//   |  Hdb : handle del file ADB                    |
//   |  Hrec : Record                                 |
//   |  Hook : struttura HOOK *                       |
//   |  Ritorna NULL = Non è stato cambiato nulla     |
//   |          Puntatore al primo campo cambiato     |
//   |                                                |
//   |                  by Ferrà Art & Technology 2001 |
//   +-------------------------------------------------

//#if (!defined(_NODC)&&!defined(EH_CONSOLE))
#ifndef EH_CONSOLE
CHAR * adb_HookTouch(INT Hdb,HREC hRec,struct ADB_HOOK Hook[])
{
    INT a;
	INT iSize;
	CHAR *pRecClone;
	BOOL fTouch=FALSE;
//	INT Hdl;
	CHAR *FldPtr;
	CHAR cr1[150];
	struct ADB_REC *Fld;
	BYTE *pCampo,*pOldValue;
    ADB_STRUCTBLOB *pBlob;

	if (hRec==ADBNULL) return NULL;

	// Copio l'attuale record in un'area Clonata
	//Hdl=memoAlloc(RAM_AUTO,iSize,"Touch"); 
	
	//
	// Copio in memoria i dati
	//
	adb_get(Hdb,hRec,-1); 
	iSize=adb_GetDynSize(Hdb);//adb_recsize(Hdb); 
	if (!iSize) ehExit(__FUNCTION__ ": iSize ?");
//	win_infoarg("iSize=%d",iSize);
	pRecClone=ehAlloc(iSize+2);//memoLock(Hdl);
	memcpy(pRecClone,adb_DynRecBuf(Hdb),iSize);

	//
	// Scrivo il record con i nuovi campi
	//
	adb_HookWrite(Hdb,Hook);

	// Comparo i campi del record
	for (a=0;;a++)
	{
		if (!strlen(Hook[a].pTableFieldName)) break;
		if (Hook[a].pTableFieldName[0]=='#') continue; // Solo in lettura
		FldPtr=adb_FldInfo(Hdb,Hook[a].pTableFieldName,&Fld);

		if (FldPtr==NULL)
			{sprintf(cr1,"HookTouch:NomeField ? \n[%s] Hdb[%d:%s]",Hook[a].pTableFieldName,Hdb,fileName(ADB_info[Hdb].lpFileName));
			 win_errgrave(cr1);
			}

		switch (Fld->tipo)
		{
			case ADB_NUME: 
			case ADB_ALFA: 
							//win_infoarg("%s,%s,%d",FldPtr,pRecClone+Fld->pos);
							if (strcmp(FldPtr,pRecClone+Fld->pos)) fTouch=TRUE;
//							win_infoarg("%s -> [%s][%s]",Hook[a].pTableFieldName,FldPtr,lpRecClone+Fld->pos);
						    break;

			case ADB_BLOB: 
							pCampo=adb_BlobGetAlloc(Hdb,Hook[a].pTableFieldName);
							pBlob=(ADB_STRUCTBLOB *) (pRecClone+Fld->pos);
							
							pOldValue=ehAllocZero(pBlob->iLen+1); *pOldValue=0;
							if (pBlob->iLen) {memcpy(pOldValue,pRecClone+pBlob->iOfs,pBlob->iLen); pOldValue[pBlob->iLen]=0;}
							//strTrimRight(pOldValue);
 
							if (strcmp(pCampo,pOldValue)) 
							{
//								alert("[%s][%s]",pCampo,pOldValue);
								fTouch=TRUE;

							}
							ehFree(pOldValue);
							adb_BlobFree(pCampo);
							break; // Da Fare

			case ADB_BOOL:
			case ADB_INT: 
			case ADB_INT32: 
			case ADB_COBN:
			case ADB_COBD: if (memcmp(FldPtr,pRecClone+Fld->pos,Fld->RealSize)) fTouch=TRUE;
						   break;
		}
		if (fTouch) break;
	}
	
//	memoFree(Hdl,"Touch");
	ehFree(pRecClone);
	if (fTouch) return Hook[a].pTableFieldName; else return NULL;
	//return fTouch;
}
#endif

#if (!defined(_NODC)&&!defined(EH_CONSOLE))
//   +-------------------------------------------
//   | ADB_HOOKWRITE  Scrive il record traducendo|
//   |                lo dalla struttura HOOK    |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_HookWrite(INT Hdb,struct ADB_HOOK Hook[])
{
	INT a,b,pt;
	struct ADB_REC *Fld;
	INT ipt;
	INT tipo;
	INT16 *lpInt16;
	CHAR *FldPtr;
	CHAR cr1[150];
	CHAR cr2[20];
	CHAR *ptr;
	CHAR Grp[2];
	CHAR NomeObj[10];//
	INT Spiaz;
	struct IPT*arsIpt,*pin;
	LONG *lpLong;
	BYTE *pData;
	EH_OBJ * arsObj=objGetArray(-1);
	EH_IPTINFO * psIptInfo=iptGetInfo(-1);

//	struct OBJ_ALO *alo=sys.arObject[sys.dmiObject.Num].alo;
	struct OBJ *poj;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("HookWrite",Hdb);

	for (a=0;;a++)
	{
	 if (!strlen(Hook[a].pTableFieldName)) break;
	 if (Hook[a].pTableFieldName[0]=='#') continue; // Solo in lettura
	 FldPtr=adb_FldInfo(Hdb,Hook[a].pTableFieldName,&Fld);
	 if (FldPtr==NULL)
		{sprintf(cr1,"HookWrite:NomeField ? \n[%s] Hdb[%d:%s]",
				 Hook[a].pTableFieldName,Hdb,fileName(ADB_info[Hdb].lpFileName));
		 win_errgrave(cr1);
		 }

	if (Fld->tipo!=ADB_BLOB) memset(FldPtr, 0, Fld->RealSize); // Pulisce il record

	// ------------------------------------------------
	//		      CONTROLLO OPERAZIONI SPECIALI         :
	//		                                            :
	//	Per ora và solo sugli input.                  :
	//		                                            :
	// ------------------------------------------------
	strcpy(NomeObj,Hook[a].pObjName);
	Spiaz=0;

	if (NomeObj[0]=='#')
		{if (NomeObj[1]=='+') Spiaz=atoi(&NomeObj[2]);
		 strcpy(NomeObj,"");
		}

	// -------------------------
	//		 Dato in oggetto     :
	// -------------------------

	if (*NomeObj)
		 {
			tipo=obj_type(NomeObj);
			if (tipo<0) win_errgrave("HookWrite:ObjName in Hook errato");
			switch (Fld->tipo)
			{
			case ADB_ALFA:
			case ADB_BLOB:

				switch(tipo)
				{
				case O_RADIO :

							//b=obj_find(NomeObj);
							//poj=alo[b].poj; // Cerca puntatore all'oggetto
							poj=obj_ptr(NomeObj,NULL);
							*Grp=*poj->grp;// Trova il Grp dell'oggetto
							if (Grp[0]<'!') ehExit("HW:No Group Radio Object");

							// Cerca l'oggetto che è a ON
							/*
							pt=-1;
							for(b=0;b<sys.arObject[sys.dmiObject.Num].numobj;b++)
							{
								//poj=alo[b].poj;
								poj=&sys.arObject[sys.dmiObject.Num].arObj[b];
								if ((*poj->grp==Grp[0])&&(poj->status==ON)) {pt=b;break;}
							}
*/
							pt=-1;
							for(b=0;arsObj[b].tipo!=STOP;b++)
							{
								//poj=alo[b].poj;
								poj=arsObj+b;
								if ((*poj->grp==Grp[0])&&(poj->status==ON)) {pt=b;break;}
							}
							
							if (pt<0) ehExit("HW:No Radio Object ON");

							if (strlen(poj->nome)>(WORD) Fld->size)
													 win_errgrave("adb_HookWrite:Radio > Record");
							strcpy(FldPtr,poj->nome);
							//printf(">>%s ! %s %d" CRLF,FldPtr,poj->nome,strlen(poj->nome));
							break;


				case O_LIST :
				case O_LISTP:
				case OW_LIST:
				case OW_LISTP:

							ptr=obj_listcodget(NomeObj);
							if (ptr==NULL) ehExit("HW:ALFA/O_LIST ObjError");
							if (strlen(ptr)>(UINT) Fld->size)
													 {sprintf(cr1,"adb_HookWrite:O_LIST > Record [%s/%-20.20s]",NomeObj,ptr);
														win_errgrave(cr1);
													 };
							strcpy(FldPtr,ptr);
							break;

				case O_ZONAP:
						pData=ehAlloc(32002);
						obj_message(NomeObj,WS_REALGET,32000,pData);
#ifndef EH_CONSOLE
						if (strlen(pData)>1024) {win_infoarg("pDataWarning: %s - %d",NomeObj,strlen(pData)); pData[1024]=0;} // da vedere per controllo GIMBA
#endif
						adb_FldWrite(Hdb,Hook[a].pTableFieldName,pData,0);
						ehFree(pData);
						break;

				default : ehExit("HW:ALFA/ObjError");
				}
				break;

			case ADB_NUME:
			case ADB_COBN:
			case ADB_COBD:

				switch(tipo)
				{
					case O_IMARKA:
					case O_IMARKB:
					case O_MARK  : adb_FldWrite(Hdb,Hook[a].pTableFieldName,"",(double) obj_status(NomeObj));
								   break;

					case O_LIST  :
					case O_LISTP :
					case OW_LIST:
					case OW_LISTP:  adb_FldWrite(Hdb,Hook[a].pTableFieldName,"",(double) obj_listget(NomeObj));
									break;

					default : ehExit("HW:ObjError");
				}
				break;

			case ADB_BOOL:
			case ADB_INT : // Tipo intero
			case ADB_INT32:

				//if (Fld->tipo==ADB_INT32) else lpInt16=(INT16 *) FldPtr;

				switch(tipo)
				{
					case O_IMARKA:
					case O_IMARKB:
					case O_MARK  : 
									if (Fld->tipo==ADB_INT32)   * (UINT *) FldPtr  =obj_status(NomeObj);
																else
															    * (INT16 *) FldPtr =obj_status(NomeObj);
									break;

					case O_LIST  :
					case O_LISTP :
				    case OW_LIST :
				    case OW_LISTP:
									if (Fld->tipo==ADB_INT32)   * (UINT *) FldPtr =obj_listget(NomeObj);
																else
																*(INT16 *) FldPtr =obj_listget(NomeObj);
									break;

				default : ehExit("HW:ObjError");
				}
				break;

			default : ehExit("Fld->tipo invalido in HookWrite/obj");
		 }
		}
		 else

	// -------------------------
	//				Dato in input    :
	// -------------------------
		 {
//			 EH_IPTINFO * psIptInfo=I_iptGetInfo(-1);
			 
			 
			 ipt=Hook[a].iInput;
			

			if ((ipt<0)||(ipt>psIptInfo->numcampi-1))
					win_errgrave("adb_HookWrite:IptNum in Hook errato");

			switch (Fld->tipo)
			{
			 case ADB_ALFA :

							if ((strlen(ipt_read(ipt))+Spiaz)>(UINT) Fld->size) win_errgrave("adb_HookWrite:Input > Record");

							// ------------------------------
							//   CONTROLLO ANOMALIE         !
							// ------------------------------

							arsIpt=psIptInfo->arsIpt;
							pin=(arsIpt+ipt);

							// ANOMALIA MAIUSCOLE
							if (((Fld->tipo2==ON)&&(pin->num2!=1))||
									((Fld->tipo2==OFF)&&(pin->num2!=0)))
										{sprintf(cr1,"ANOMALIA \"MAIUSCOLE\":\ninput %d di tipo diverso dal campo adb %s\nControllare.",
												 ipt,Hook[a].pTableFieldName);
										 win_info(cr1);
										}

							// ANOMALIA LUNGHEZZA
							if ((Fld->RealSize-1)!=pin->len+Spiaz)
										{sprintf(cr1,"ANOMALIA LUNGHEZZA:\ninput %d di lunghezza diversa dal campo adb %s\n%d > %d\nControllare.",
										ipt,Hook[a].pTableFieldName,
										pin->len+Spiaz,(Fld->RealSize-1)
										);
										win_info(cr1);
										}

							strcpy(FldPtr+Spiaz,ipt_read(ipt));
							break;

			 case ADB_BLOB:
							adb_FldWrite(Hdb,Hook[a].pTableFieldName,ipt_read(ipt),0);
							break;

			 case ADB_DATA : //printf("%s-%d-%d",ipt_read(ipt),strlen(ipt_read(ipt)),Fld->size);
											 if ((strlen(ipt_read(ipt))+Spiaz)>(UINT) Fld->size)
													 win_errgrave("adb_HookWrite:Input > Data Record");

											 memset(cr1,0,9);
											 strcpy(cr1,ipt_read(ipt));
											 // Controllo
											 memcpy(cr2,cr1+4,4);// Anno
											 memcpy(cr2+4,cr1+2,2);// Mese
											 memcpy(cr2+6,cr1,2);// Giorno
											 memcpy(FldPtr+Spiaz,cr2,8);
											 break;

			 case ADB_NUME : adb_FldWrite(Hdb,Hook[a].pTableFieldName,"",atof(ipt_read(ipt)));
							 break;
			 case ADB_COBD:
							 adb_FldWrite(Hdb,Hook[a].pTableFieldName,"",atof(ipt_read(ipt)));
							 //ptr=adb_MakeCobolDecimal(atof(ipt_read(ipt)),Fld->RealSize,Fld->tipo2);
							 //if (ptr==NULL) win_errgrave("adb_HookWrite:IptSize!= Fld.Size (CD)");
							 //strcpy(FldPtr+Spiaz,ptr); 				 
							 break;
			 case ADB_COBN:
							 adb_FldWrite(Hdb,Hook[a].pTableFieldName,"",atof(ipt_read(ipt)));
							 //ptr=adb_MakeCobolNumeric(atof(ipt_read(ipt)),Fld->RealSize,Fld->tipo2);
							 //if (ptr==NULL) win_errgrave("adb_HookWrite:IptSize!= Fld.Size (CN)");
							 //strcpy(FldPtr+Spiaz,ptr); 				 
							 break;

			 case ADB_INT:   
			 case ADB_BOOL:
							 lpInt16=(INT16 *) FldPtr;
							 *lpInt16=atoi(ipt_read (ipt));
							 break;
#ifdef _WIN32
			 case ADB_INT32: * (UINT *) FldPtr =atoi(ipt_read (ipt));
							 break;
#endif

			 case ADB_AINC:  lpLong=(LONG *) FldPtr;
							 *lpLong=atol(ipt_read(ipt));
							 break;

			 default : ehExit("Fld->tipo invalido in HookWrite");
			}
		 }
	}
	return 0;
}

//   +-------------------------------------------
//   | ADB_HOOKGET    Legge  il record traducendo|
//   |                lo dalla struttura HOOK    |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_HookGet(INT Hdb,struct ADB_HOOK Hook[])
{
	INT a;
	struct ADB_REC *Fld;
	INT ipt;
	INT tipo;
	INT16 *lpInt16;
	CHAR *p;
	CHAR *FldPtr;
	CHAR cr1[150];
	CHAR cr2[50];
	double Dato;
	INT Spiaz;
	CHAR NomeObj[10];
	struct IPT*arsIpt,*pin;
	LONG *lpLong;
	BOOL fDato;
	CHAR *lpj;
	BYTE *pData;
	EH_IPTINFO * psIptInfo=iptGetInfo(-1);

	if (((Hdb<0)||(Hdb>=ADB_max))||
			(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("HookGet",Hdb);

	for (a=0;;a++)
	{
	 if (!strlen(Hook[a].pTableFieldName)) break;
	 p=Hook[a].pTableFieldName;
	 if (*p=='#') p++;// Solo in lettura incremento pt di uno
	 FldPtr=adb_FldInfo(Hdb,p,&Fld);
	 if (FldPtr==NULL)
		{sprintf(cr1,"HookGet:NomeField ? \n[%s] in Hdb[%d:%s]",
								 Hook[a].pTableFieldName,
								 Hdb,fileName(ADB_info[Hdb].lpFileName));
		 win_errgrave(cr1);
		}
	// ------------------------------------------------
	//		      CONTROLLO OPERAZIONI SPECIALI         :
	//		                                            :
	//	Per ora và solo sugli input.                  :
	//		                                            :
	// ------------------------------------------------
	strcpy(NomeObj,Hook[a].pObjName);
	Spiaz=0;

	if (NomeObj[0]=='#')
		{if (NomeObj[1]=='+') Spiaz=atoi(&NomeObj[2]);
		 strcpy(NomeObj,"");
		}

	// -------------------------
	//		 Dato in oggetto     :
	// -------------------------
	if (strlen(NomeObj)>0)
		 {
			tipo=obj_type(NomeObj);
			if (tipo<0) win_errgrave("HookGet:ObjName in Hook errato");
			switch (Fld->tipo)
			{
			case ADB_ALFA:

				switch(tipo)
				{
					case O_RADIO :
					case O_MARK :
								obj_on(FldPtr); // Mha ?
								break;

					case O_LIST  :
					case O_LISTP :
					case OW_LIST:
					case OW_LISTP:

								obj_listcodset(NomeObj,FldPtr);
								break;

					case O_ZONAP:
							obj_message(NomeObj,WS_REALSET,0,FldPtr); 
							break;

					default : ehExit("HW:ALFA/ObjError");
				}
				break;

			case ADB_BLOB:
				switch(tipo)
				{
					case O_ZONAP:
							pData=adb_BlobGetAlloc(Hdb,p); 
							obj_message(NomeObj,WS_REALSET,0,pData); 
							adb_BlobFree(pData);
							break;

					default : ehExit("HW:BLOB/ObjError");
				}
				break;


			case ADB_NUME:
			case ADB_COBD:
			case ADB_COBN:
				Dato=adb_FldNume(Hdb,p);
				switch(tipo)
				{
				 case O_IMARKA:
				 case O_IMARKB:
				 case O_MARK  : if (Dato) obj_on(NomeObj); else obj_off(NomeObj);
							    break;

				 case O_LIST  :
				 case O_LISTP :
				 case OW_LIST:
				 case OW_LISTP:

							   obj_listset(NomeObj,(INT) Dato);
							   break;

				 default : ehExit("HGN:ObjError");
				}
				break;

			case ADB_INT : // Tipo intero
			case ADB_INT32 : // Tipo intero 32bit
			case ADB_BOOL: // Tipo Flag

				if (Fld->tipo==ADB_INT32) fDato= * (UINT *) FldPtr;
									      else 
										  fDato= * (INT16 *) FldPtr;
				switch(tipo)
				{
				case O_IMARKA:
				case O_IMARKB:
				case O_MARK  : if (fDato) obj_on(NomeObj); else obj_off(NomeObj);
							   break;

				case O_LIST  :
				case O_LISTP :
				case OW_LIST:
				case OW_LISTP:
							  obj_listset(NomeObj,fDato);
							  break;

				default : ehExit("HG:ObjError");
				}
				break;

			default : ehExit("Fld->tipo invalido in HookGet/obj");
		 }
		}
		 else

	// -------------------------
	//				Dato in input    :
	// -------------------------

		 {ipt=Hook[a].iInput;
			if ((ipt<0)||(ipt>psIptInfo->numcampi-1))
					win_errgrave("adb_HookGet:IptNum in Hook errato");

			switch (Fld->tipo)
			{
			 case ADB_ALFA :
							ipt_write(ipt,FldPtr+Spiaz,0);

							 // ------------------------------
							 //   CONTROLLO ANOMALIE         !
							 // ------------------------------

							 arsIpt=psIptInfo->arsIpt;
							 pin=(arsIpt+ipt);

							 if (Fld->tipo==ADB_ALFA)
							 {
							 // ANOMALIA MAIUSCOLE
							 if (((Fld->tipo2==ON)&&(pin->num2!=1))||
								 ((Fld->tipo2==OFF)&&(pin->num2!=0)))
								{sprintf(cr1,
										"ANOMALIA \"MAIUSCOLE\":\ninput %d di tipo diverso dal campo adb %s\nControllare.",
										ipt,Hook[a].pTableFieldName);
										win_info(cr1);
										}

								 // ANOMALIA LUNGHEZZA
								 if ((Fld->RealSize-1)!=pin->len+Spiaz)
										{sprintf(cr1,
										 "ANOMALIA LUNGHEZZA:\ninput %d di lunghezza diversa dal campo adb %s\n%d > %d\nControllare.",
										 ipt,
										 Hook[a].pTableFieldName,
										 pin->len+Spiaz,Fld->RealSize-1);
										 win_info(cr1);
										}
							 }
							 break;

			 case ADB_BLOB:
							lpj=adb_BlobGetAlloc(Hdb,Hook[a].pTableFieldName);
							ipt_write(ipt,lpj,0);
							adb_BlobFree(lpj);
							break;

			 case ADB_DATA : memcpy(cr2,FldPtr+6+Spiaz,2);// Anno
							 memcpy(cr2+2,FldPtr+4+Spiaz,2);// Mese
							 memcpy(cr2+4,FldPtr+Spiaz,4);// Giorno
							 cr2[8]=0;
							 ipt_write(ipt,cr2,0);
							 break;

			 case ADB_NUME : ipt_write(ipt,"",atof(FldPtr+Spiaz));
							 break;
			 case ADB_COBD:  ipt_write(ipt,"",adb_GetCobolDecimal(FldPtr+Spiaz,Fld->RealSize,Fld->size,Fld->tipo2));
							 break;
			 case ADB_COBN:  ipt_write(ipt,"",adb_GetCobolNumeric(FldPtr+Spiaz,Fld->RealSize,Fld->size,Fld->tipo2));
							 break;

			 case ADB_INT : // Tipo intero
			 case ADB_BOOL: // Tipo Flag
							lpInt16=(INT16 *) FldPtr;
							ipt_write(ipt,"",*lpInt16);
							break;

			 case ADB_INT32:
			 case ADB_AINC: lpLong=(LONG *) FldPtr;
							ipt_write(ipt,"",*lpLong);
							break;

			 default : ehExit("Fld->tipo non valido in HookGet");

			}
		 ipt_vedisolo(ipt);

		 }
	}
	return 0;
}

//   +-------------------------------------------
//   | ADB_HOOKINSERT Inserisce un nuovo record  |
//   |                con struttura a uncino     |
//   |                                           |
//   | ADB_HOOKUPDATE Aggiorna un record         |
//   |                con struttura a uncino     |
//   |                                           |
//   |    -----------------------------------    |
//   |                                           |
//   | PAZZIA PURA al 16 Giugno:                 |
//   | All'incredibile annuncio della Ferrà Art& |
//   | Technology, la stampa specializzata       |
//   | mondiale ha commentato :                  |
//   |                                           |
//   |       PC_WEEK- (Minchia Vvvaii !!)        |
//   |       PCTODAY- (Hiiii ... non cci possso  |
//   |                 credere !!)               |
//   |                                           |
//   |    -----------------------------------    |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_HookInsert(INT Hdb,struct ADB_HOOK Hook[])
{
//	BTI_WORD uiDataLen;
//	BTI_SINT status;

	adb_HookWrite(Hdb,Hook);
/*
	uiDataLen=ADB_info[Hdb].AdbHead->RecSize;
	status = BTRV( B_INSERT,
								 ADB_info[Hdb].PosBlock,
								 ADB_info[Hdb].RecBuffer,// Puntatore ai dati
								 &uiDataLen, // Grandezza dei dati
								 ADB_info[Hdb].KeyBuffer,   // ???> in output
								 0); // ??? in input

	if (status==5) return status; // Chiave duplicata

	if (status) {BTI_err(Hdb,"HOOKINSERT:",status);}
	*/
	return adb_insert(Hdb);
}

INT adb_HookUpdate(INT Hdb,struct ADB_HOOK Hook[])
{
//	BTI_WORD uiDataLen;
//	BTI_SINT status;
	adb_HookWrite(Hdb,Hook);

	/*
	uiDataLen=ADB_info[Hdb].AdbHead->RecSize;
	status = BTRV( B_UPDATE,
								 ADB_info[Hdb].PosBlock,
								 ADB_info[Hdb].RecBuffer,// Puntatore ai dati
								 &uiDataLen, // Grandezza dei dati
								 ADB_info[Hdb].KeyBuffer,   // ???> in output
								 0); // ??? in input

	if (status==5) return status; // Chiave duplicata

	if (status) {BTI_err(Hdb,"HOOKUPDATE:",status);}
	*/
	return adb_update(Hdb);
}
#endif
//   +-------------------------------------------
//   | ADB_UPDATE Modifica il record corrente    |
//   |                                           |
//   |  Hdb : handle del file ADB                |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |         -1 = Errore in lettura vecchio rec|
//   |         -1 = Errore in lettura vecchio rec|
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_update(INT Hdb)
{
	BTI_ULONG uiDataLen;
	BTI_SINT status;
	INT iCount;

#ifdef HLINK
	LONG  Hrec;
	INT   HdlNuovo;
	CHAR *RecNuovo;
	INT   HdlVecchio;
	CHAR *RecVecchio;
	INT SizeRec;
	INT FlagSi=OFF;
#endif

	//CHAR *PtRecord;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("update",Hdb);

	// --------------------------------------------------
	// Emulazione di Update
	//

#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		CHAR *lpSQLCommand;
		SQLHSTMT hstmt;
		CHAR *p;
		CHAR *p1,*p2;
		hstmt=ADB_info[Hdb].hStmt;

		p=EmuGetInfo("D.update",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
		p=p?p:"0"; if (*p!='1') ehExit("Update non autorizzato su Hdb:%d [%s]",Hdb,p);
			
		lpSQLCommand=ehAlloc(EMU_SIZE_QUERY);
		if (!*ADB_info[Hdb].lpHrecLastWhere) ehExit("EMU:Update non valido");

		EmuUpdateBuilder(Hdb,lpSQLCommand,ADB_info[Hdb].lpHrecLastWhere);
		//win_infoarg("Update:SQL[%s]",lpSQLCommand);
		p1=lpSQLCommand;
		while (TRUE)
		{
			p2=strstr(p1,"\3"); if (p2) *p2=0;
			if (ADB_info[Hdb].fQueryPending) {SQLCloseCursor(hstmt); ADB_info[Hdb].fQueryPending=FALSE;}
			//win_infoarg("Update:SQL[%s]",p1);
			//SQLTRY("SQLExecDirect",ADB_info[Hdb].hStmt,SQLExecDirect(hstmt,lpSQLCommand,SQL_NTS));
			if (*p1) LSQLExecute("Update",ADB_info[Hdb].hStmt,p1);
			if (p2) p1=p2+1; else break;
		}
		//win_infoarg("Riattiva LSQLExecute()");

		ehFree(lpSQLCommand);
		return 0;
	}
#endif
	//PtRecord=ADB_info[Hdb].lpRecBuffer;

	//  --------------------------------------------------------------------
	//   SE ESISTE LA GESTIONE LINK CONTROLLO CHE NON ESISTANO CAMPI-CLONI !
	//  --------------------------------------------------------------------

#ifdef HLINK
	FlagSi=adb_HLsonopadre(Hdb);
	if (FlagSi)
	 {
		// Chiede la memoria per il backup
		SizeRec=adb_recsize(Hdb);
		HdlNuovo=memoAlloc(M_HEAP,SizeRec,"updateNEW"); if (HdlNuovo<0) ehExit("update NEW :MEMO ?");
		HdlVecchio=memoAlloc(M_HEAP,SizeRec,"updateOLD"); if (HdlVecchio<0) ehExit("update OLD :MEMO ?");

		RecNuovo=memoPtr(HdlNuovo,NULL);
		RecVecchio=memoPtr(HdlVecchio,NULL);

		// Backup del nuovo record
		memcpy(RecNuovo,ADB_info[Hdb].lpRecBuffer,SizeRec);

		// Backup del vecchio record
		adb_position(Hdb,&Hrec);
		if (adb_get(Hdb,Hrec,-1)) return -1;
		memcpy(RecVecchio,ADB_info[Hdb].lpRecBuffer,SizeRec);

		// Ripristina i dati da Updatare (se si può dire)
		memcpy(ADB_info[Hdb].lpRecBuffer,RecNuovo,SizeRec);
		memoFree(HdlNuovo,"updateNEW");
	 }
#endif
	iCount=0;
	rifai:
	if (ADB_info[Hdb].fBlobPresent) uiDataLen=adb_BlobNewOfs(Hdb,0); else uiDataLen=ADB_info[Hdb].AdbHead->wRecSize;
	if (!uiDataLen) ehExit("adb_update: !uiDataLen");

	//
	// BLOB_CTRL (Controllo i campi blob che non contengano dati oversize)
	//
#ifdef _ADB32_BLOB_CTRL
	adb_BlobControl(Hdb);	
#endif
	if (uiDataLen>60000) win_infoarg("Attenzione: uiDataLen =%d",uiDataLen);
	status = BTRV32(B_UPDATE,
				    ADB_info[Hdb].lpPosBlock,
				    ADB_info[Hdb].lpRecBuffer,// Puntatore ai dati
				    &uiDataLen, // Grandezza dei dati
				    ADB_info[Hdb].lpKeyBuffer,   // ???> in output
				    0); // ??? in input
	
	// Accesso negato al file
	if ((status==5)||(status==46)) return status;

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {
#if defined(_NODC)||defined(EH_CONSOLE)
		printf("Lock: %d",iCount);
		iCount++; if (iCount<30) goto rifai;
#else
		if (adb_LockControl(ON,Hdb,status,"Vietato l'update.",OFF)) goto rifai;
#endif
	 }
	 else
	 {
		if (status) {BTI_err(Hdb,"UPDATE:",status);}
	 }

#if !defined(_NODC)&&!defined(EH_CONSOLE)
	adb_LockControl(OFF,0,0,"",OFF);
#endif

	// -------------------------------------------------
	// Se i dati sono stati updeitati                  !
	// li modifica in tutti i campi-clone              !
	// -------------------------------------------------
#ifdef HLINK
	if (FlagSi)
	{
		adb_HLaction(Hdb,RecVecchio);
		memoFree(HdlVecchio,"updateOLD");
	}
#endif
	return status;
}
//   +-------------------------------------------
//   | ADB_DELETE Cancella il record corrente    |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |         -1 = Non cancellabile perchè usato|
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_delete(INT Hdb)
{
	BTI_ULONG uiDataLen;
	BTI_SINT status;

	if (((Hdb<0)||(Hdb>=ADB_max))||
			(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("delete",Hdb);
	// --------------------------------------------------
	// Emulazione di Update
	//
#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		CHAR *p1,*p2;
		CHAR *lpSQLCommand;
		SQLHSTMT hstmt;
		CHAR *p;
		hstmt=ADB_info[Hdb].hStmt;

		p=EmuGetInfo("D.delete",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
		p=p?p:"0";
 
		if (*p!='1') ehExit("delete non autorizzato su Hdb:%d",Hdb);
			
		lpSQLCommand=ehAlloc(EMU_SIZE_QUERY);
		if (!*ADB_info[Hdb].lpHrecLastWhere) ehExit("EMU:delete non valido");

		EmuDeleteBuilder(Hdb,lpSQLCommand,ADB_info[Hdb].lpHrecLastWhere);
		//win_infoarg("Insert:SQL[%s]",lpSQLCommand);
		p1=lpSQLCommand;
		while (TRUE)
		{
			p2=strstr(p1,"\3"); if (p2) *p2=0;
			if (ADB_info[Hdb].fQueryPending) {SQLCloseCursor(hstmt); ADB_info[Hdb].fQueryPending=FALSE;}
			//win_infoarg("Delete:SQL[%s]",p1);
			if (*p1) LSQLExecute("Delete",ADB_info[Hdb].hStmt,p1);
			if (p2) p1=p2+1; else break;
		}
		
//		if (ADB_info[Hdb].fQueryPending) {SQLCloseCursor(hstmt); ADB_info[Hdb].fQueryPending=FALSE;}
		// win_infoarg("Delete:%s",lpSQLCommand);
		//LSQLExecute("Delete",ADB_info[Hdb].hStmt,lpSQLCommand);
		ehFree(lpSQLCommand);
		return 0;
	}
#endif

#ifdef HLINK
	if (ADB_HL&&(adb_HLink>0))
	{
		CHAR *p;
		p=adb_HLaction(Hdb,NULL); // Testa l'esistenza di un figlio riconosciuto
		if (p!=NULL)
		{
			win_btierr("CANCELLAZIONE RECORD ANNULLATA:\nIl record e' collegato con altri archivi.",p);
			return -1;
		}
	}
#endif

	rifai:
//	uiDataLen=ADB_info[Hdb].AdbHead->RecSize;
	if (ADB_info[Hdb].fBlobPresent) uiDataLen=adb_BlobNewOfs(Hdb,0);
									 else
									 uiDataLen=ADB_info[Hdb].AdbHead->wRecSize;
	if (!uiDataLen) ehExit("adb_delete: !uiDataLen");
	status = BTRV32(B_DELETE,
				    ADB_info[Hdb].lpPosBlock,
				    0,
				    &uiDataLen, // Grandezza dei dati
				    0,
				    0);


	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {
#if defined(_NODC)||defined(EH_CONSOLE)
		goto rifai;
#else
		if (adb_LockControl(ON,Hdb,status,"Vietato la cancellazione.",OFF)) goto rifai;
#endif

	 }
	 else
	 {
	 if (status) {BTI_err(Hdb,"DELETE:",status);}
	 }

#if !defined(_NODC)&&!defined(EH_CONSOLE)
	adb_LockControl(OFF,0,0,"",OFF);
#endif
	return status;
}

void adb_deleteall(HDB hdbTable,CHAR *pszObjProgress) // 2010 - Incredibile !
{
	INT a,iMax,iCount;
	HREC hRec;
	_DMI dmiDel=DMIRESET;
			  
	iMax=adb_recnum(hdbTable); if (!iMax) return;

	// Carico i record i memoria
	DMIOpen(&dmiDel,RAM_AUTO,iMax,sizeof(HREC),"DEL");
#ifndef EH_CONSOLE
	if (pszObjProgress) obj_message(pszObjProgress,WS_SETFLAG,iMax,"MAX");
#endif
	iCount=0;
    if (!adb_find(hdbTable,0,B_GET_FIRST,NULL,B_RECORD))
	{
	 do
	 {
		adb_position(hdbTable,&hRec);
		DMIAppend(&dmiDel,&hRec);
	 } while (!adb_find(hdbTable,0,B_GET_NEXT,NULL,B_RECORD));
	}
	
#ifndef EH_CONSOLE
	if (pszObjProgress) obj_message(pszObjProgress,WS_SETFLAG,dmiDel.Num,"MAX");
#endif
	for (a=0;a<dmiDel.Num;a++)
	{
		DMIRead(&dmiDel,a,&hRec);
		adb_get(hdbTable,hRec,-1);
#ifndef EH_CONSOLE
		if (pszObjProgress) obj_message(pszObjProgress,WS_SETFLAG,a+1,"CNT");
#endif
		if (adb_delete(hdbTable)) win_infoarg("Errore in cancellazione");
	}
	DMIClose(&dmiDel,"DEL");
}


//  +-------------------------------------------------------+
//  | adb_deleteNoLink										|
//	| Cancella il record corrente senza controllare i Link	|
//  |														|
//  |  Hdb : handle del file ADB							|
//  |														|
//  |							   by Lion Informatica 2001 |
//  +-------------------------------------------------------+
void adb_deleteNoLink(INT Hdb)
{
	BTI_ULONG uiDataLen;
	BTI_SINT status;

	if (((Hdb<0) || (Hdb>=ADB_max)) || (ADB_info[Hdb].iHdlRam==-1))
		adb_errgrave("delete",Hdb);

	rifaiNoLink:
	if (ADB_info[Hdb].fBlobPresent) uiDataLen=adb_BlobNewOfs(Hdb,0);
									 else
									 uiDataLen=ADB_info[Hdb].AdbHead->wRecSize;
	if (!uiDataLen) ehExit("adb_deleteNoLink: !uiDataLen");

	status = BTRV32(B_DELETE,
				    ADB_info[Hdb].lpPosBlock,
				    0,
				    &uiDataLen, // Grandezza dei dati
				    0,
				    0);


	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	{
#if defined(_NODC)||defined(EH_CONSOLE)
		goto rifaiNoLink;
#else
		if (adb_LockControl(ON,Hdb,status,"Vietata la cancellazione.",OFF)) goto rifaiNoLink;
#endif
	}
	else
	{
		if (status) BTI_err(Hdb,"DELETE:",status);
	}
#if !defined(_NODC)&&!defined(EH_CONSOLE)
	adb_LockControl(OFF,0,0,"",OFF);
#endif
	return;
}

//   +-------------------------------------------
//   | adb_indexnum                              |
//   | Ritorna il numero di un indice "nome" 		 |
//   |                                           |
//   |  Hdb :  handle del file ADB							 |
//   |  idx  :  Nome dell'indice                 |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |         -1 = Non esiste                   |
//   |         -9 = Hdb non valido              |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+

INT adb_indexnum(INT Hdb,CHAR *IndexNome)
{
	INT a;
	struct ADB_INDEX *AdbIndex;
	INT IndexNum;

	if (((Hdb<0)||(Hdb>=ADB_max))||
			(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("indexnum",Hdb);

	AdbIndex=(struct ADB_INDEX *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD)+sizeof(struct ADB_REC)*ADB_info[Hdb].AdbHead->Field);

	IndexNum=-1;

	for (a=0;a<ADB_info[Hdb].AdbHead->Index;a++)
	{
	 if (!strcmp(AdbIndex[a].IndexName,IndexNome)) {IndexNum=a; break;}
	}

	if (IndexNum<0) return -1;
	return IndexNum;
}

//   +-------------------------------------------
//   | adb_GetDescIndex                          |
//   | Ritorna la descrizione dell'indice        |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
#ifdef _WIN32
CHAR *adb_GetDescIndex(HDB Hdb,INT IndexNum)
{
	struct ADB_INDEX *AdbIndex;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("adb_GetDescIndex",Hdb);

	AdbIndex=(struct ADB_INDEX *) ((BYTE *)ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD)+sizeof(struct ADB_REC)*ADB_info[Hdb].AdbHead->Field);

 	if (IndexNum>=ADB_info[Hdb].AdbHead->Index) ehExit("adb_GetDescIndex(); Indice errato %d",IndexNum);
	return AdbIndex[IndexNum].IndexDesc;
}
#endif

//   +-------------------------------------------
//   | ADB_POSITION  Ritorna la posizione fisica |
//   |               del record nel file btrieve |
//   |                                           |
//   |  Hdb :  Numero dell'indice               |
//   |  rec:  Pt al long che conterrà la posiz.  |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_position(INT Hdb,LONG *position)
{
	BTI_ULONG uiDataLen=4;
	INT status;
	LONG posizione;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("position",Hdb);

	// --------------------------------------------------
	// Emulazione di position
	// NOTA:
	// Viene allocato un progressivo che riconduce a un WHERE 
	// da usare per UPDATE o DELETE
	//
#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		INT a,iNew=-1;

		status=0;

		//win_infoarg("POSITION: Emulazione [%s]",ADB_info[Hdb].lpHrecWhere);
	
		*position=0;
		
		// A) Controllo se c'è già
		for (a=0;a<MAX_VIRTUAL_HREC;a++)
		{
			if (!ADB_info[Hdb].lppVirtualHrec[a]) 
			{
				if (iNew==-1) iNew=a;
				continue;
			}
			if (!strcmp(ADB_info[Hdb].lppVirtualHrec[a],ADB_info[Hdb].lpHrecLastWhere)) {*position=a+1; break;}
		}

		// Non c'è = assegno un nuovo spazio nella lista
		if (!*position)
		{
			if (iNew>=MAX_VIRTUAL_HREC) ehExit("INew overload");
			if (iNew<0) ehExit("INew <0 ?");
			ADB_info[Hdb].lppVirtualHrec[iNew]=ehAlloc(strlen(ADB_info[Hdb].lpHrecLastWhere)+1);
			if (!ADB_info[Hdb].lppVirtualHrec[iNew]) ehExit("EMUPOS: no memory");
			strcpy(ADB_info[Hdb].lppVirtualHrec[iNew],ADB_info[Hdb].lpHrecLastWhere);
			*position=iNew+1;
		}
		//win_infoarg("Position =[%d]",*position);
		return status;
	}
#endif

	rifai:
	status = BTRV32( B_GET_POSITION,
				   ADB_info[Hdb].lpPosBlock,
				   &posizione,
				   &uiDataLen,
				   0,
				   0);

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {
#if defined(_NODC)||defined(EH_CONSOLE)
		goto rifai;
#else
		if (adb_LockControl(ON,Hdb,status,"Vietato il position.",OFF)) goto rifai;
#endif
	 }
	 else
	 {
	 if (status) {BTI_err(Hdb,"POSIT:",status);}
	 }

#if !defined(_NODC)&&!defined(EH_CONSOLE)
	adb_LockControl(OFF,0,0,"",OFF);
#endif

	*position=posizione;

	//if (status) BTI_err(Hdb,"POSITION",status);
	return status;
}
//   +-------------------------------------------
//   | ADB_GET     Legge direttamente dal file   |
//   |             con la posizione fisica       |
//   |                                            |
//   |  Hdb :  Numero dell'indice               |
//   |  rec:  Pt al long che conterrà la posiz.  |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+

INT adb_get(INT Hdb,HREC position,INT IndexNum)
{
	BTI_ULONG uiDataLen;
	INT status;
	HREC *posizione;
	INT iTent;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("adbget",Hdb);

	// --------------------------------------------------
	// Emulazione di ricerca
	//
#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		CHAR *lpSQLCommand;
		SQLHSTMT hstmt;

		status=0;
/*
		win_infoarg("GETDIRECT: Emulazione %d [%s][Last=%s]",
					position,
					ADB_info[Hdb].lppVirtualHrec[position-1],
					ADB_info[Hdb].lpHrecLastWhere);
*/		
		hstmt=ADB_info[Hdb].hStmt;
		SQLTRY(SQL_HANDLE_STMT,"SET1",ADB_info[Hdb].hStmt,SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET2",ADB_info[Hdb].hStmt,SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET3",ADB_info[Hdb].hStmt,SQLSetStmtAttr(ADB_info[Hdb].hStmt, SQL_ATTR_ROW_STATUS_PTR, NULL, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET4",ADB_info[Hdb].hStmt,SQLSetStmtAttr(ADB_info[Hdb].hStmt, SQL_ATTR_ROWS_FETCHED_PTR, NULL, 0));

		// -----------------------------------------------------------------------
		// Siamo già sul posto
		// lpHrecWhere = Ultimo HrecWhere che ho fatto
		//
		if (position<1) ehExit("EMUGET: posizione errata (%d)",position);

		if (!ADB_info[Hdb].lppVirtualHrec[position-1]) ehExit("EMUGET:lppVirtualHrec[]=NULL");
		
//		win_infoarg("%s\n%s",ADB_info[Hdb].lppVirtualHrec[position-1],ADB_info[Hdb].lpHrecLastWhere);

		// Era messo li per ottimizzazione ma mi crea qualche problema nei costi ... (&#&1)
		//if (!strcmp(ADB_info[Hdb].lppVirtualHrec[position-1],ADB_info[Hdb].lpHrecLastWhere)) return 0;
		
		//EmuBindMaker(Hdb);
		lpSQLCommand=ehAlloc(EMU_SIZE_QUERY);
		EmuSelectBuilder(Hdb,lpSQLCommand,ADB_info[Hdb].lppVirtualHrec[position-1]);

		if (ADB_info[Hdb].fQueryPending) {SQLCloseCursor(hstmt); ADB_info[Hdb].fQueryPending=FALSE;}
		
		if (!ADB_info[Hdb].lppVirtualHrec[position-1]) ehExit("EMUGET1:lppVirtualHrec[]=NULL");

		EmuBindMaker(Hdb,FALSE);

		//win_infoarg("[%s]",ADB_info[Hdb].hStmt,lpSQLCommand);
		//SQLTRY("SQLExecDirect",ADB_info[Hdb].hStmt,SQLExecDirect(hstmt,lpSQLCommand,SQL_NTS));
		LSQLExecute("Get",ADB_info[Hdb].hStmt,lpSQLCommand);
		if (!ADB_info[Hdb].lppVirtualHrec[position-1]) ehExit("EMUGET2:lppVirtualHrec[]=NULL");
		if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO) {if (!sEmu.fNoErrorView) ODBCError("AE:Execute A");}
		if (!ADB_info[Hdb].lppVirtualHrec[position-1]) ehExit("EMUGET3:lppVirtualHrec[]=NULL");
		SQLTRY(SQL_HANDLE_STMT,"GET:SQLFetch",ADB_info[Hdb].hStmt,SQLFetch(hstmt));
		if (!ADB_info[Hdb].lppVirtualHrec[position-1]) ehExit("EMUGET4:lppVirtualHrec[]=NULL");

		BT_OdbcToAdb(Hdb); // Traduco
		if (!ADB_info[Hdb].lppVirtualHrec[position-1]) ehExit("EMUGET5:lppVirtualHrec[]=NULL");
		ehFree(lpSQLCommand);

		if (!ADB_info[Hdb].lppVirtualHrec[position-1]) ehExit("EMUGET6:lppVirtualHrec[]=NULL");
		adb_EmuLastWhere(Hdb,ADB_info[Hdb].lppVirtualHrec[position-1]);
		// Aggiorno l'ultimo HREC Virtuale
		//if (ADB_info[Hdb].lpHrecLastWhere) ehFree(ADB_info[Hdb].lpHrecLastWhere);
		//if (!ADB_info[Hdb].lppVirtualHrec[position-1]) ehExit("EMUGET3:lppVirtualHrec[]=NULL");

		//ADB_info[Hdb].lpHrecLastWhere=ehAlloc(strlen(ADB_info[Hdb].lppVirtualHrec[position-1])+1);
		//strcpy(ADB_info[Hdb].lpHrecLastWhere,ADB_info[Hdb].lppVirtualHrec[position-1]);

		SQLCloseCursor(hstmt);
		ADB_info[Hdb].fQueryPending=FALSE;
		return status;
	}
#endif

/*
INT adb_get(INT Hadb,HREC position,INT IndexNum)
{
	BTI_WORD dataLen;
	INT status;
	HREC *posizione;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("adbget",Hadb);

	posizione=(HREC *) ADB_info[Hadb].RecBuffer;
	*posizione=position;

	rifai:
	dataLen=ADB_info[Hadb].AdbHead->wRecSize;
	status = BTRV( B_GET_DIRECT,
				   ADB_info[Hadb].PosBlock,
				   ADB_info[Hadb].RecBuffer, // Record dove andare
				   &dataLen,// Lunghezza buffer
				   ADB_info[Hadb].KeyBuffer,
				   (WORD) IndexNum);

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	{ if (adb_LockControl(ON,Hadb,status,"Vietato la lettura.",OFF)) goto rifai;}
	 else
	 {
	 if (status) 
		{
		 BTI_err(ADB_info[Hadb].FileName,"GET:",status);
		}
	 }

//	AVANTI:
	adb_LockControl(OFF,0,0,"",OFF);

//	if (status) BTI_err(ADB_info[Hadb].FileName,"ADBGET:",status);
	return status;
}
*/

	posizione=(HREC *) ADB_info[Hdb].lpRecBuffer; *posizione=position;
	for (iTent=0;iTent<20;iTent++)
	{
		uiDataLen=ADB_info[Hdb].ulRecBuffer;
		status = BTRV32(B_GET_DIRECT,
						ADB_info[Hdb].lpPosBlock,
						ADB_info[Hdb].lpRecBuffer, // Record dove andare
						&uiDataLen,// Lunghezza buffer
						ADB_info[Hdb].lpKeyBuffer,
						(WORD) IndexNum);
		if (!status) break;
		// @@@
		if (ADB_info[Hdb].fBlobPresent==TRUE&&status==22) {adb_BufRealloc(Hdb,0,FALSE,"GET",FALSE); iTent=0; continue;}
		/*
		if (ADB_info[Hdb].fBlobPresent==TRUE&&status==43) 
			{
				adb_BufRealloc(Hdb,0,FALSE,"GET",FALSE); 
				continue;
			}
			*/

		if (!fErrorView&&status) return status;

		// --------------------------
		// Controllo dei LOCK       !
		// --------------------------
		if (status==81)
		{ 
	#if defined(_NODC)||defined(EH_CONSOLE)
			continue;
	#else
			if (adb_LockControl(ON,Hdb,status,"Vietato la lettura.",OFF)) continue;
	#endif
		}
		else
		{
		  if (status) 
		  {
			  BTI_err(Hdb,"GET:",status);
	#ifdef _DEBUG
			  {
				BYTE *p=NULL;
				*p=1;
			  }
	#endif
		  }
		}
		break;
	}
	

#if !defined(_NODC)&&!defined(EH_CONSOLE)
	adb_LockControl(OFF,0,0,"",OFF);
#endif

	return status;
}

//   +-------------------------------------------
//   | ADB_GETALLOC                              |
//   | Legge ed alloca il record in un nuovo     |
//   | Utile per letture in Multi-Thread         |
//   |                                           |
//   |  Hdb :  Numero dell'indice                |
//   |  rec:  Pt al long che conterrà la posiz.  |
//   |                                           |
//   |  Ritorna 0 = Il puntatore al nuovo record |
//   |          NULL Errore                      |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+

#ifdef _WIN32

BYTE *adb_GetAlloc32(INT Hdb,HREC position)
{
	BTI_ULONG uiDataLen;
	INT status;
	BYTE *lpPosBlock;
	BYTE *lpBuffer;
	HREC *posizione;

	// Creo copia del blocco del dbase
	lpPosBlock=ehAlloc(255);
	memcpy(lpPosBlock,ADB_info[Hdb].lpPosBlock,255);
	lpBuffer=ehAlloc(ADB_info[Hdb].ulRecBuffer);

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("adbget",Hdb);

	posizione=(HREC *) lpBuffer; *posizione=position;

	uiDataLen=ADB_info[Hdb].ulRecBuffer;
	status = BTRV32(B_GET_DIRECT,
				    lpPosBlock,
				    lpBuffer, // Record dove andare
				    &uiDataLen,// Lunghezza buffer
				    ADB_info[Hdb].lpKeyBuffer,
				    (WORD) -1);
	ehFree(lpPosBlock);

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status)
	{ 
	 ehFree(lpBuffer);
	 lpBuffer=NULL;
	 }

//	AVANTI:
//	adb_LockControl(OFF,0,0,"",OFF);

//	if (status) BTI_err(Hdb,"ADBGET:",status);
	return lpBuffer;
}
#endif

/*
void EmuHrecUpdate(INT Hdb,INT position)
{
	if (ADB_info[Hdb].lpHrecWhere) ehFree(ADB_info[Hdb].lpHrecWhere);
	ADB_info[Hdb].lpHrecWhere=ehAlloc(strlen(ADB_info[Hdb].lppVirtualHrec[position-1])+1);
	strcpy(ADB_info[Hdb].lpHrecWhere,ADB_info[Hdb].lppVirtualHrec[position-1]);
}
*/

#ifdef _ADBEMU
// ---------------------------------------------------------
// EmuHrecFieldFind()
// Ricerca se un determinato campo è nella dichiarazione di HREC
//
static BOOL EmuHrecFieldFind(HDB Hdb,CHAR *lpField)
{
	CHAR *p,*lpn;
	BOOL fRet=FALSE;
	p=EmuGetInfo("D.hrec",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
	if (!p) ehExit("ADB/EMU HREC ?");

	while (TRUE)
	{
		lpn=strstr(p,"+"); if (lpn) *lpn=0;
		//lpEmuBind=AdbBindInfo(Hdb,p);
		if (!strcmp(p,lpField)) fRet=TRUE;
		if (lpn) *lpn='+';
		if (fRet) break;
		if (lpn) p=lpn+1; else break;
	}
	return fRet;
}

// ---------------------------------------------------------
// EmuHrecMaker()
// Crea una stringa tipo "WHERE" contenente i campi segnalati
// come HREC nel .emu
// NOTA: lpBoolean permette di scegliere il simbolo tra i campi (Es. = o <>)
//
void EmuHrecMaker(HDB Hdb,CHAR *lpSQLCommand,CHAR *lpBoolean)
{
	CHAR *p,*lpn;
	BOOL fPass;
	EMUBIND *lpEmuBind;
	CHAR szServ[200];
	p=EmuGetInfo("D.hrec",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
	if (!p) ehExit("ADB/EMU HREC ?");

	strcpy(lpSQLCommand,"(");

	fPass=FALSE;
	while (TRUE)
	{
		lpn=strstr(p,"+"); if (lpn) *lpn=0;
		lpEmuBind=AdbBindInfo(Hdb,p,TRUE);
		if (!lpEmuBind) ehExit("EMU/ADBfind: Non trovato il campo [%s] in HREC virtuale",p);
		if (fPass) strcat(lpSQLCommand," AND ");
		sprintf(szServ,"%s%s%s",
				lpEmuBind->szSQLName,
				lpBoolean,
				EmuSQLFormat(Hdb,lpEmuBind,adbFieldPtr(Hdb,lpEmuBind->szAdbName)));
//				EmuSQLFormat(Hdb,lpEmuBind));

		strcat(lpSQLCommand,szServ);
		fPass=TRUE;
		if (lpn) {*lpn='+'; p=lpn+1;} else break;
	}

	strcat(lpSQLCommand,")");
}

void adb_EmuLastWhere(HDB Hdb,CHAR *lpSQLCommand)
{
	if (ADB_info[Hdb].lpHrecLastWhere) ehFree(ADB_info[Hdb].lpHrecLastWhere);
	ADB_info[Hdb].lpHrecLastWhere=ehAlloc(strlen(lpSQLCommand)+1);
	strcpy(ADB_info[Hdb].lpHrecLastWhere,lpSQLCommand);
}
#endif

//   +--------------------------------------------
//   | ADB_FIND  Cerca una chiave nell'indice    |
//   |                                           |
//   |  Hdb : Handle del file ADB                |
//   |  idx  : Numero dell'indice da usare       |
//   |  (idx  Nome dell'indice per Afind)        |
//   |  key  : Chiave da cercare                 |
//   |  modo : modo BTRIEVE di ricerca           |
//   |                                           |
//   |        B_GET_EQUAL se uguale              |
//   |        B_GET_GT    se è + grande          |
//   |        B_GET_GE    se è + grande o uguale |
//   |        B_GET_LT    se è minore            |
//   |        B_GET_LE    se + minore o uguale   |
//   |                                           |
//   |  flag: B_KEY o B_RECORD (valore ritorno)  |
//   |        Se si vuole solo chiave (veloce)   |
//   |        o tutto il record dati             |
//   |  rec:  Puntatore al record/chiave         |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_Afind(INT Hdb,CHAR *IndexNome,INT modo,void *keyfind,INT flag)
{
	INT IndexNum;

	IndexNum=adb_indexnum(Hdb,IndexNome);
	if (IndexNum<0) ehExit("adb_Afind():Nome indice non valido [%s]",IndexNome);

#ifdef _WIN32
	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) 
	{	win_infoarg("[%s][%s]",IndexNome,keyfind);}
#endif

	return adb_find(Hdb,IndexNum,modo,keyfind,flag);
}

INT adb_find(INT Hdb,INT IndexNum,INT modo,void *keyfind,INT flag)
{
	INT iLock;		

	BTI_ULONG uiDataLen;
	BTI_SINT status;
	INT iCommand;
	INT solochiave;
	static INT iCount=0;
	struct ADB_INDEX *AdbIndex;

	CHAR szServ[200];
	void *PtChiave;

	CHAR *tiporic[]= 
	{
	  "FIND_EQUAL:",
	  "FIND_NEXT:",
	  "FIND_PREVIOUS:",
	  "FIND_GT:",
	  "FIND_GE:",
	  "FIND_LT:",
	  "FIND_LE:",
	  "FIND_FIRST:",
	  "FIND_LAST:"
	};

#ifdef _WIN32
	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) 
	{
		win_infoarg("%d [%s]",Hdb,keyfind);
	}
#endif

	iLock=0;		
	while (TRUE)
	{
		if (modo<100) break;
		if (modo>400 && modo<499) {iLock=M_NOWAIT_LOCK; modo-=iLock; break;}	//	e' il piu' usato
		if (modo>300 && modo<399) {iLock=M_WAIT_LOCK; modo-=iLock; break;}
		if (modo>200 && modo<299) {iLock=S_NOWAIT_LOCK; modo-=iLock; break;}
		if (modo>100 && modo<199) {iLock=S_WAIT_LOCK; modo-=iLock; break;}
		adb_errgrave("Controllo modo",Hdb);
	}
	//if (iLock) win_infoarg("DAM  %d:%d",modo,iLock);

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("find",Hdb);
	if ((modo<B_GET_EQUAL)||(modo>B_GET_LAST))win_errgrave("Adb_find:Modo di ricerca non valido");
	AdbIndex=(struct ADB_INDEX *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD)+sizeof(struct ADB_REC)*ADB_info[Hdb].AdbHead->Field);

	// --------------------------------------------------
	// Emulazione di ricerca
	//
#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		INT a,iPti;
		BYTE *pkey;
		CHAR *lpEmuOperatorConvert[]={"=","?","?",">",">=","<","<=","?","?"};

		struct ADB_REC *Record;
		struct ADB_INDEX_SET *IndexSet;
		CHAR *lpSQLCommand;
		SQLHSTMT hstmt;
		EMUBIND *lpEmuBind;
		CHAR *lpWhere=NULL;

		SQLRETURN sqlReturn;

		hstmt=ADB_info[Hdb].hStmt;

		SQLTRY(SQL_HANDLE_STMT,"SET1",hstmt,SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET2",hstmt,SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET3",hstmt,SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, NULL, 0));
		SQLTRY(SQL_HANDLE_STMT,"SET4",hstmt,SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, NULL, 0));

		Record=(struct ADB_REC *) (ADB_info[Hdb].AdbHead+1);
		IndexSet=(struct ADB_INDEX_SET *) (AdbIndex+ADB_info[Hdb].AdbHead->Index);
		status=0;

		// --------------------------------------------------------------------
		// GOAL: Devo creare un comando SELECT che simula la richiesta di FIND
		// --------------------------------------------------------------------
		// A) Ricerco la descrizione di campi coinvolti nell'Indice (x ORDER BY)
		//
		iPti=-1;
		for (a=0;a<ADB_info[Hdb].AdbHead->IndexDesc;a++)
		{
			if (!strcmp(AdbIndex[IndexNum].IndexName,IndexSet[a].IndexName)) {iPti=a+1; break;}
			//win_infoarg("%d) [%s][%s]",a,IndexSet[a].IndexName,IndexSet[a].FieldName);
		}
		if (iPti<0) ehExit("EMU/adb_find: error iPTI");

		// --------------------------------------------------------------------
		// A2) Effettuo il Binding
		//

		// --------------------------------------------------------------------
		// B) Costruisco SELECT
		// NOTA: SELECT <campi> FROM <dbase> WHERE <condizione> ORDER BY <sort>
		lpSQLCommand=ehAlloc(EMU_SIZE_QUERY); *lpSQLCommand=0;

		// --------------------------------------------------------------------
		// C) Costruisco la Where (SE E' IL CASO)
		//
		switch (modo)
		{
			case B_GET_EQUAL:
			case B_GET_GE:
			case B_GET_GT:
			case B_GET_LE:
			case B_GET_LT:

				// Il Bind avviene per colonna ...
				lpWhere=ehAlloc(EMU_SIZE_QUERY);
				strcpy(lpWhere,"(");
				pkey=keyfind;
				for (a=iPti;a<(iPti+AdbIndex[IndexNum].KeySeg);a++)
				{
					if (a>iPti) strcat(lpWhere," AND ");
					lpEmuBind=AdbBindInfo(Hdb,IndexSet[a].FieldName,TRUE);
					if (!lpEmuBind) ehExit("EMU/ADBfind(2): Non trovato il campo [%s]",IndexSet[a].FieldName);
					strcat(lpWhere,lpEmuBind->szSQLName);
					strcat(lpWhere,lpEmuOperatorConvert[modo-B_GET_EQUAL]);
					strcat(lpWhere,EmuSQLFormat(Hdb,lpEmuBind,pkey)); 
					pkey+=lpEmuBind->lpAdbField->RealSize;
					// _d_("%s:%d       ",lpEmuBind->szSQLName,lpEmuBind->lpAdbField->RealSize); Sleep(1000);
				}
				strcat(lpWhere,") ");
				//win_infoarg("[%s]",lpWhere);
				break;


			// Cazzo ... mica da ridere
			case B_GET_NEXT:
				
				// a) Se sono Query/Pendente passo al successivo
				if (ADB_info[Hdb].fQueryPending&&ADB_info[Hdb].iLastDirection==0) 
				{
					sqlReturn=SQLFetch(hstmt);
					//SQL_SUCCESS, SQL_SUCCESS_WITH_INFO, SQL_NO_DATA, SQL_STILL_EXECUTING, SQL_ERROR, or SQL_INVALID_HANDLE
					if (sqlReturn==SQL_SUCCESS||
						sqlReturn==SQL_SUCCESS_WITH_INFO)  
					{

						BT_OdbcToAdb(Hdb); // Traduco

						// Genero il HREC/WHERE
						EmuHrecMaker(Hdb,lpSQLCommand,"=");
						adb_EmuLastWhere(Hdb,lpSQLCommand);

						//_d_("%d) [%s]",iCount++,adbFieldPtr(Hdb,"CODICE"));//adbFieldPtr(Hdb,"CODICE"));
						ehFree(lpSQLCommand);
						return 0;
					}
					else
					{
						ADB_info[Hdb].fQueryPending=FALSE;
						SQLCloseCursor(ADB_info[Hdb].hStmt);
						adb_EmuLastWhere(Hdb,"");
						ehFree(lpSQLCommand);
						return 1;
					}
				}
				else
				{
					// b) Se NON SONO Query/Pendente trovo il where in base al record corrente
					//  1. Leggere il record corrente

					if (ADB_info[Hdb].fQueryPending) {ADB_info[Hdb].fQueryPending=FALSE;SQLCloseCursor(ADB_info[Hdb].hStmt);}

					if (!ADB_info[Hdb].lpHrecLastWhere) {ehExit("NEXT senza puntatore precarico"); return 0;}
					EmuSelectBuilder(Hdb,lpSQLCommand,ADB_info[Hdb].lpHrecLastWhere);
	
					//win_infoarg("NEXT.GET:[%s]",lpSQLCommand);
					EmuBindMaker(Hdb,FALSE);

					//SQLTRY("NEXT:SQLExecDirect",ADB_info[Hdb].hStmt,SQLExecDirect(ADB_info[Hdb].hStmt,lpSQLCommand,SQL_NTS));
					LSQLExecute("NEXT:",ADB_info[Hdb].hStmt,lpSQLCommand);
					if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO) {if (!sEmu.fNoErrorView) ODBCError("AE:Execute B");}
					
					SQLTRY(SQL_HANDLE_STMT,"NEXT:SQLFetch",ADB_info[Hdb].hStmt,SQLFetch(ADB_info[Hdb].hStmt));
					BT_OdbcToAdb(Hdb); // Traduco

					// Aggiorno l'ultimo HREC Virtuale

					//  2. Formare la stringa di where
					lpWhere=ehAlloc(EMU_SIZE_QUERY);
					strcpy(lpWhere,"(");
					for (a=iPti;a<(iPti+AdbIndex[IndexNum].KeySeg);a++)
					{
						if (a>iPti) strcat(lpWhere," AND ");
						lpEmuBind=AdbBindInfo(Hdb,IndexSet[a].FieldName,TRUE);
						if (!lpEmuBind) ehExit("EMU/ADBfind: Non trovato il campo [%s]",IndexSet[a].FieldName);
						//strcat(lpSQLCommand,lpEmuBind->szSQLName);
						//if (modo==B_GET_LAST) strcat(lpSQLCommand," DESC");
						strcat(lpWhere,lpEmuBind->szSQLName);
						strcat(lpWhere,">=");
						strcat(lpWhere,EmuSQLFormat(Hdb,lpEmuBind,adbFieldPtr(Hdb,IndexSet[a].FieldName))); 
					}
					strcat(lpWhere," AND ");
					EmuHrecMaker(Hdb,lpSQLCommand,"<>");
					strcat(lpWhere,lpSQLCommand);
					strcat(lpWhere,") ");
					ADB_info[Hdb].fQueryPending=FALSE;
					SQLCloseCursor(ADB_info[Hdb].hStmt);
				}
				break;

			// Cazzo ... mica da ridere 2
			case B_GET_PREVIOUS:
				
				// a) Se sono Query/Pendente passo al successivo
				if (ADB_info[Hdb].fQueryPending&&ADB_info[Hdb].iLastDirection==1) 
				{
					sqlReturn=SQLFetch(hstmt);
					//SQL_SUCCESS, SQL_SUCCESS_WITH_INFO, SQL_NO_DATA, SQL_STILL_EXECUTING, SQL_ERROR, or SQL_INVALID_HANDLE
					if (sqlReturn==SQL_SUCCESS||
						sqlReturn==SQL_SUCCESS_WITH_INFO)  
					{

						BT_OdbcToAdb(Hdb); // Traduco

						// Genero il HREC/WHERE
						EmuHrecMaker(Hdb,lpSQLCommand,"=");
						adb_EmuLastWhere(Hdb,lpSQLCommand);

						//_d_("%d) [%s]",iCount++,adbFieldPtr(Hdb,"CODICE"));//adbFieldPtr(Hdb,"CODICE"));
						ehFree(lpSQLCommand);
						return 0;
					}
					else
					{
						ADB_info[Hdb].fQueryPending=FALSE;
						SQLCloseCursor(ADB_info[Hdb].hStmt);
						adb_EmuLastWhere(Hdb,"");
						ehFree(lpSQLCommand);
						return 1;
					}
				}
				else
				{
					// b) Se NON SONO Query/Pendente trovo il where in base al record corrente
					//  1. Leggere il record corrente

					if (ADB_info[Hdb].fQueryPending) {ADB_info[Hdb].fQueryPending=FALSE;SQLCloseCursor(ADB_info[Hdb].hStmt);}

					if (!ADB_info[Hdb].lpHrecLastWhere) {ehExit("NEXT senza puntatore precarico"); return 0;}
					EmuSelectBuilder(Hdb,lpSQLCommand,ADB_info[Hdb].lpHrecLastWhere);
	
					//win_infoarg("NEXT.GET:[%s]",lpSQLCommand);
					EmuBindMaker(Hdb,FALSE);
					//SQLTRY("NEXT:SQLExecDirect",ADB_info[Hdb].hStmt,SQLExecDirect(ADB_info[Hdb].hStmt,lpSQLCommand,SQL_NTS));
					LSQLExecute("PREVIOUS",ADB_info[Hdb].hStmt,lpSQLCommand);
					if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO) {if (!sEmu.fNoErrorView) ODBCError("AE:Execute C");}
					
					SQLTRY(SQL_HANDLE_STMT,"NEXT:SQLFetch",ADB_info[Hdb].hStmt,SQLFetch(ADB_info[Hdb].hStmt));
					BT_OdbcToAdb(Hdb); // Traduco

					// Aggiorno l'ultimo HREC Virtuale

					//  2. Formare la stringa di where
					lpWhere=ehAlloc(EMU_SIZE_QUERY);
					strcpy(lpWhere,"(");
					for (a=iPti;a<(iPti+AdbIndex[IndexNum].KeySeg);a++)
					{
						if (a>iPti) strcat(lpWhere," AND ");
						lpEmuBind=AdbBindInfo(Hdb,IndexSet[a].FieldName,TRUE);
						if (!lpEmuBind) ehExit("EMU/ADBfind: Non trovato il campo [%s]",IndexSet[a].FieldName);
						//strcat(lpSQLCommand,lpEmuBind->szSQLName);
						//if (modo==B_GET_LAST) strcat(lpSQLCommand," DESC");
						strcat(lpWhere,lpEmuBind->szSQLName);
						strcat(lpWhere,"<=");
						strcat(lpWhere,EmuSQLFormat(Hdb,lpEmuBind,adbFieldPtr(Hdb,IndexSet[a].FieldName))); 
					}
					strcat(lpWhere," AND ");
					EmuHrecMaker(Hdb,lpSQLCommand,"<>");
					strcat(lpWhere,lpSQLCommand);
					strcat(lpWhere,") ");
					ADB_info[Hdb].fQueryPending=FALSE;
					SQLCloseCursor(ADB_info[Hdb].hStmt);
				}
				break;

			case B_GET_FIRST:
				lpWhere=NULL;
				break;
			case B_GET_LAST:
				lpWhere=NULL;
				break;

			default:
				win_infoarg("adb_find(): non gestito %s",tiporic[modo-B_GET_EQUAL]);
				lpWhere=NULL;
				break;
		}

		// Crea lpSQLCommand
		EmuSelectBuilder(Hdb,lpSQLCommand,lpWhere);
		if (lpWhere) {ehFree(lpWhere); lpWhere=NULL;}
#ifdef _DEBUG
//		ehPrintd("ADBFIND: %s" CRLF,lpSQLCommand);
#endif
		strTrimRight(lpSQLCommand);
		// --------------------------------------------------------------------
		// D) ORDER BY Ordinati per
		//	  
		strcat(lpSQLCommand," ORDER BY ");
		for (a=iPti;a<(iPti+AdbIndex[IndexNum].KeySeg);a++)
		{
			if (a>iPti) strcat(lpSQLCommand,",");
			lpEmuBind=AdbBindInfo(Hdb,IndexSet[a].FieldName,TRUE);
			if (!lpEmuBind) ehExit("EMU/ADBfind: Non trovato il campo [%s]",IndexSet[a].FieldName);
			strcat(lpSQLCommand,lpEmuBind->szSQLName);
			if (modo==B_GET_LAST||
				modo==B_GET_PREVIOUS) 
				{
					strcat(lpSQLCommand," DESC"); 
					ADB_info[Hdb].iLastDirection=1;
				}
				else 
				{
					strcat(lpSQLCommand," ASC");
					ADB_info[Hdb].iLastDirection=0;
				}
		}

		// --------------------------------------------------------------------
		// E) ESEGUI LA QUERY
		//

		// win_infoarg("FIND.SQL:[%s]",lpSQLCommand);

		// DAFARE:Và chiuso solo se si tratta un inizio ricerca
		if (ADB_info[Hdb].fQueryPending) {SQLCloseCursor(ADB_info[Hdb].hStmt); ADB_info[Hdb].fQueryPending=FALSE;}

		EmuBindMaker(Hdb,FALSE);

		//SQLTRY("SQLExecDirect",ADB_info[Hdb].hStmt,SQLExecDirect(ADB_info[Hdb].hStmt,lpSQLCommand,SQL_NTS));
		LSQLExecute("FIND",ADB_info[Hdb].hStmt,lpSQLCommand);
		if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO) {if (!sEmu.fNoErrorView) ODBCError("AE:Execute D");}

		//SQL_NO_DATA
		sEmu.sqlLastError=SQLFetch(ADB_info[Hdb].hStmt);
		BT_OdbcToAdb(Hdb); // Traduco

		if (sEmu.sqlLastError==SQL_NO_DATA)
		{
			ADB_info[Hdb].fQueryPending=FALSE;
			SQLCloseCursor(ADB_info[Hdb].hStmt);
			ehFree(lpSQLCommand);
			return 1;	
		}

		if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO)
		{
			ODBCError("SQLFetch");
			ehExit("SQLFetch");
		}

		// --------------------------------------------------------------------
		// F) Creo l'HREC/WHERE virtuale
		//
		EmuHrecMaker(Hdb,lpSQLCommand,"=");
		adb_EmuLastWhere(Hdb,lpSQLCommand);
		ehFree(lpSQLCommand);

		ADB_info[Hdb].fQueryPending=TRUE;
		//win_infoarg("SQLFIND EXIT");

		return status;
	}
#endif

	// Ricerca per Step Fisici
	if (IndexNum==-1)
	{
		INT iStep=-1;
		switch (modo)
		{
			case B_GET_FIRST: iStep=33; break;
			case B_GET_LAST: iStep=34; break;
			case B_GET_NEXT: iStep=24; break;
			case B_GET_PREVIOUS: iStep=35;break;
		}
		if (iStep==-1) ehExit("Get Step: error(%d)",modo);

		iCommand=iStep;
		//win_infoarg("%d",iCommand);
	}
	else
	{

	// Copia la chiave nel buffer
	if ((modo!=B_GET_FIRST)&&
			(modo!=B_GET_LAST)&&
			(modo!=B_GET_NEXT)&&
			(modo!=B_GET_PREVIOUS))
			{
			 if (flag&B_NOSTR)
				 {PtChiave=keyfind;}
				 else
				 {if (strlen(keyfind)>sizeof(szServ)) ehExit("adb_find:serv memo");
					//memset(szServ,0,sizeof(serv)); 
					ZeroFill(szServ);
					strcpy(szServ,keyfind);
					PtChiave=szServ;
				 }

				 memcpy(ADB_info[Hdb].lpKeyBuffer,PtChiave,AdbIndex[IndexNum].KeySize);
			}

	 if (flag&B_KEY) solochiave=50; else solochiave=0;
     iCommand=modo+solochiave;
	}

	rifai:
	uiDataLen=ADB_info[Hdb].ulRecBuffer;
	ADB_iLastStatus = status = BTRV32((WORD) (iLock+iCommand),	//	by DAM    aggiunto iLock al comando
				    ADB_info[Hdb].lpPosBlock,//ADB_info[Hdb].lpPosBlock,
				    ADB_info[Hdb].lpRecBuffer,    // Puntatore ai dati
				    &uiDataLen, 										// Grandezza dei dati
				    ADB_info[Hdb].lpKeyBuffer,//ADB_info[Hdb].lpKeyBuffer,    // Ricerca e riceve chiave
				    (WORD) IndexNum); 									// Indice da usare


//	if (iLock && status) win_infoarg("DAM  %d:%d:%d",status,iLock,iCommand);

	//if (!status) return 0;
	// @@@
  	if (ADB_info[Hdb].fBlobPresent==TRUE&&status==22) 
		{
			adb_BufRealloc(Hdb,0,FALSE,"FIND",FALSE); 
			if (ADB_info[Hdb].ulRecBuffer>2000000)
				ehExit("Record Over Size %d - Hdb=%d",ADB_info[Hdb].ulRecBuffer,Hdb);

			switch (iCommand)
			{
				//	By DAM	(inizio)    aggiunto iLock al comando			
				case B_GET_NEXT: BTRV32((WORD) (iLock+B_GET_PREVIOUS),ADB_info[Hdb].lpPosBlock,ADB_info[Hdb].lpRecBuffer,&uiDataLen,ADB_info[Hdb].lpKeyBuffer,(WORD) IndexNum);  break;
				case B_GET_PREVIOUS: BTRV32((WORD) (iLock+B_GET_NEXT),ADB_info[Hdb].lpPosBlock,ADB_info[Hdb].lpRecBuffer,&uiDataLen,ADB_info[Hdb].lpKeyBuffer,(WORD) IndexNum);  break;
				// commentato by DAM case 24: BTRV32((WORD) (iLock+35),ADB_info[Hdb].lpPosBlock,ADB_info[Hdb].lpRecBuffer,&uiDataLen,ADB_info[Hdb].lpKeyBuffer,(WORD) IndexNum);  break;
				// commentato by DAM case 35: BTRV32((WORD) (iLock+24),ADB_info[Hdb].lpPosBlock,ADB_info[Hdb].lpRecBuffer,&uiDataLen,ADB_info[Hdb].lpKeyBuffer,(WORD) IndexNum);  break;
				//	By DAM	(fine)
			}
			goto rifai;
		}
	//win_infoarg("status %d",status);

	if ((status==9)|| // Fine del file
		 (status==4))   // Chiave infondata
			goto AVANTI;
			/*
			else
			BTI_err(Hdb,tiporic[modo-B_GET_EQUAL],status);
				*/

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------

	if (status==78 || status==81 || status==84 || status==85)		//	by DAM  (aggiunti status)
	{

//	By DAM (inizio)
#ifdef	_EXTERNAL_CONTROL
	return status;
#endif
//	By DAM (fine)


#if defined(_NODC)||defined(EH_CONSOLE)
		goto rifai;
#else
		if (adb_LockControl(ON,Hdb,status,"Vietato la ricerca.",OFF)) goto rifai;
#endif

	 }
	 else
	 {
		if (status) {BTI_err(Hdb,tiporic[modo-B_GET_EQUAL],status);}
	 }

	AVANTI:
#if !defined(_NODC)&&!defined(EH_CONSOLE)
	adb_LockControl(OFF,0,0,"",OFF);
#endif
	return status;
}

//   +-------------------------------------------
//   | ADB_OPEN  Apre un file AdvancedDataBase   |
//   |                                           |
//   |  file : Nome del file da aprire           |
//   |                                           |
//   |  modo   0 : Standard (pag 4.12)           |
//   |        -1 : Accelerato                    |
//   |             Disabilita DataRecovery       |
//   |        -2 . Solo in lettura               |
//   |        -4 : Gestione esclusiva            |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |          -1  problemi sul file ADB        |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_open(CHAR *file,CHAR *Client,INT modo,INT *Hdb)
{
 return adb_openEx(file,Client,modo,Hdb,ADB_BTI);
}

//
// adb_openEx()
//
INT adb_openEx(CHAR * file,CHAR * Client,INT modo,INT *lpHdb,INT iModeEmu)
{
	INT a,rt;
	FILE *pf1;
	CHAR ADBBuf[255];
	BTI_BYTE dataBuf[255];
	BTI_ULONG uiDataLen;
	BTI_SINT status=0;
	INT Hdb=-1,dbhdl=-1;
	CHAR serv[255];
	LONG SizeInfo;
	BYTE *lpb;
	struct ADB_HEAD AdbHead;
	DWORD dwSize;

#ifdef _ADBEMU
	INT iEmuStart,iEmuEnd;
 //	--------------------------------------------------------
 //  Controllo eventuale avvio di connessione con DB
 //	--------------------------------------------------------
	if (iModeEmu==ADB_EMU)
	{
		//BOOL sqlReturn;
		INT iPt;

		// Ricerco il nome del file (Tabella) nel profilo
		sprintf(serv,"T.File=%s",_strupr(fileName(file)));
		iPt=EmuFindInfo(serv,0,0);
		//win_infoarg("%s=%d",serv,iPt);
		if (iPt<0) ehExit("Il file %s non esiste nel .EMU",fileName(file));
		iPt++; iEmuStart=iPt; iEmuEnd=sEmu.iRows;
		for (a=iEmuStart;a<sEmu.iRows;a++)
		{
			if (*sEmu.lpRows[a]=='T') {iEmuEnd=a-1; break;}
		}
		// Controllo se devo stabilire la connessione (la prima volta)
		if (!sEmu.iODBCCountOpen)
		{

			if (!sOdbcSection.hConn) ehExit("Gestiore ODBC non aperto");
			/*

			// Vedi EMUFILE
			sqlReturn=adb_ODBCConnect();
			if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO) 
			{
				return -1;
			}
			*/
		}
		sEmu.iODBCCountOpen++;
	}
#endif

 //	-------------------------------------
 //		Cerca prima area adb disponibile  !
 // -------------------------------------
 
 if (ADB_hdl==-1) win_errgrave("adb_open(): ADBase manager non inizializzato");
 if (ADB_ult>=ADB_max) ehExit("adb_open(): ADBase full[Ult:%d][Max:%d] ",ADB_ult,ADB_max);

 for (a=0;a<=ADB_max;a++)
	{
	 if (ADB_info[a].iHdlRam==-1) {Hdb=a;break;}
	}
 if (Hdb<0) win_errgrave("adb_open(): Dbase manager inquinato");

 *lpHdb=-1;

  // -----------------------------------
  // calcola la lunghezza del file.ADB !
  // -----------------------------------
  strcpy(ADBBuf,file); strcat(ADBBuf,".ADB");
  if (!fileCheck(ADBBuf)) ehExit("%s non esiste",ADBBuf);
  SizeInfo=fileSize(ADBBuf);
  if ((SizeInfo<0)||(SizeInfo>9000)) {win_infoarg("Il record è > di 9000 %d",SizeInfo); return -1;}

  //-------------------------------------
  // APRE IL FILE ADB PER DETERMINARE   !
  // LA LUGHEZZA DEL RECORD E KEYBUFFER !
  // ------------------------------------
  pf1=fopen(ADBBuf,"rb"); if (!pf1) {rt=ON; status=-1; goto fine;}
  fread(&AdbHead,sizeof(AdbHead),1,pf1);
  if (strcmp(AdbHead.id,"Adb1.2 Eh3")) {rt=ON; status=-1; goto fine;}

	// ------------------------------------
	//	    Struttura della memoria ADB    !
	//	                                   !
	//	255 Nome del file                  !
	//	128 PosBlock                       !
	//	<-> Attributi del file             !
	//	    ADB_HEAD                       !
	//	    ADB_REC * i campi              !
	//	    ADB_INDEX * gli indici         !
	//	    ADB_INDEX_SET sorgente definiz !
	//	                                   !
	// ------------------------------------
	rt=OFF;

	ADB_info[Hdb].fBlobPresent=FALSE;
	if (AdbHead.wRecSize<1) ADB_info[Hdb].fBlobPresent=TRUE;

	sprintf(serv,"*DB%0d:%s",Hdb,fileName(file));
	strcpy(ADBBuf, file ); strcat(ADBBuf,".BTI");
	dwSize=(strlen(ADBBuf)+1)+128+SizeInfo;
	dbhdl=memoAlloc(RAM_AUTO,
					  //255+128+255+SizeInfo,
					  dwSize, // Nome del file + PosBlock + 255 per le chiavi
					  serv);
	//									 fileName(file));
	if (dbhdl<0)
		{sprintf(serv,"adb_open():Memoria insufficiente per aprire :\n%s",file);
		 win_errgrave(serv);
		}
//	lpb=memoPtr(dbhdl); memset(lpb,0,dwSize); // new 2001/10
	lpb=memoLock(dbhdl); memset(lpb,0,dwSize); // new 2001/10

	ADB_info[Hdb].iHdlRam=dbhdl;
	ADB_info[Hdb].lpFileName=lpb; // Size 255
	strcpy(ADB_info[Hdb].lpFileName, ADBBuf);

	ADB_info[Hdb].lpPosBlock=lpb+strlen(ADBBuf)+1; // Size 128
	ADB_info[Hdb].AdbHead=(struct ADB_HEAD *) (ADB_info[Hdb].lpPosBlock+128);
	ADB_info[Hdb].AdbFilter=NULL;
	ADB_info[Hdb].OpenCount=1;// Conteggio delle aperture
	ADB_info[Hdb].lpRecBuffer=NULL;
	ADB_info[Hdb].lpKeyBuffer=NULL;
#ifdef _ADBEMU
	ADB_info[Hdb].iEmulation=iModeEmu;
#endif
	fseek(pf1,0,SEEK_SET);
	if (!fread(ADB_info[Hdb].AdbHead,(WORD) SizeInfo,1,pf1)) 
	{	rt=ON;
		status=-2; 
		goto fine;
	}
	ADB_info[Hdb].AdbHead->wRecSize=AdbHead.wRecSize; // New2000

//	ADB_info[Hdb].lpRecBuffer=farPtr(memoPtr(dbhdl),255+128);  // Size RecSize
//	ADB_info[Hdb].lpKeyBuffer=farPtr(memoPtr(dbhdl),255+128+AdbHead.wRecSize); // Size 255

	rt=OFF;
	if (iModeEmu==ADB_BTI)
	{
	 rifai:
	 *dataBuf=0; if (Client) strcpy(dataBuf,Client);
	 uiDataLen=strlen(dataBuf) + 1;

#ifdef _DEBUG
//	 win_infoarg("BTI:%s",file);
#endif

	 ADB_iLastStatus = status = BTRV32(B_OPEN,
				     ADB_info[Hdb].lpPosBlock,// Blocco di riferimento del file
				     dataBuf, //Proprietario
				     &uiDataLen,// Lunghezza nome del proprietario
				     ADBBuf,  // Nome del file
				     (WORD) modo);  // vedi leggenda

	 if (!fErrorView&&status) {rt=status; goto fine;}

	 // --------------------------
	 // Controllo dei OWNER      !
	 // --------------------------
	 if (*dataBuf=='?'&&(status==51)) {rt=ON; goto fine;}
	 // --------------------------
	 // Controllo dei LOCK       !
	 // --------------------------
	 if ((status==46)||(status==88)||(status==81)||(status==85))
		{
#if defined(_NODC)||defined(EH_CONSOLE)
		 ehExit("Vietato l'accesso al file [%s]",file);
		 goto rifai;
#else
		 if (adb_LockControl(ON,Hdb,status,"Vietato l'accesso al file.",ON)) goto rifai;
		 rt=ON;
#endif
		}
		else
		{
		  if (status) {rt=ON;}
		}
#if !defined(_NODC)&&!defined(EH_CONSOLE)
	 adb_LockControl(OFF,0,0,"",OFF);
#endif
	}

	fine:
	fclose(pf1);

	if (rt)
		{//win_infoarg("OPEN [%d] [%s] status=%d",Hdb,ADBBuf,status);
	     //ehExit("");
         if (status>0) 
		 {
			 if (fErrorView) 
			 {
				BTI_err(Hdb,"OPEN:",status);
				sprintf(serv,"*DB%0d:%s",Hdb,fileName(file));
				if (dbhdl>-1) 
				{
					memoFree(dbhdl,serv);
					ADB_info[Hdb].iHdlRam=-1;
				}
			 }
		 }
		 if (status==20) ehExit("Il Pervasive SQL non e' installato");
		 return status;
		}
		else
		{
			// -----------------------------------------------------------
			// Alloco la memoria necessaria
			//
			ADB_info[Hdb].ulRecBuffer=ADB_info[Hdb].AdbHead->wRecSize;
			if (!ADB_info[Hdb].ulRecBuffer) ADB_info[Hdb].ulRecBuffer=adb_BlobOfsDyn(Hdb);
			ADB_info[Hdb].lpRecBuffer=ehAlloc(ADB_info[Hdb].ulRecBuffer);
			ADB_info[Hdb].ulKeyBuffer=255;
			ADB_info[Hdb].lpKeyBuffer=ehAlloc(ADB_info[Hdb].ulKeyBuffer);
		}

	ADB_ult++;
	*lpHdb=Hdb;

	//win_infoarg("[%d]",(INT) ulPreAlloc);
	if (ulPreAlloc&&ADB_info[Hdb].fBlobPresent) 
	{
		adb_BufRealloc(Hdb,ulPreAlloc,FALSE,"OPEN",TRUE);
	}

#ifdef _ADBEMU
	// --------------------------------------------------------------------
	// Alloca memoria per comandi (ritorna puntatore in hStmt)
	//
	// sqlReturn=SQLAllocStmt(EhOdbc.hConn, &EhOdbc.hStmt);
	//
	if (iModeEmu==ADB_EMU)
	{
		// Alloco spazio per lo stantment
		ADB_info[Hdb].hStmt=0;
		sEmu.sqlLastError=SQLAllocHandle(SQL_HANDLE_STMT, sOdbcSection.hConn, &ADB_info[Hdb].hStmt);
		if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO)
			ehExit("Errore in assegnazione Stantment");

		ADB_info[Hdb].iEmuStartPoint=iEmuStart;
		ADB_info[Hdb].iEmuEndPoint=iEmuEnd;
		if (sEmu.sqlLastError!=SQL_SUCCESS&&sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO) 
			{
			 ODBCError("SQLAllocHandle/Env");  
			 ehExit("Ambiente stantment per db [%s]:?",fileName(file));
			}

		ADB_info[Hdb].lppVirtualHrec=ehAlloc(sizeof(CHAR *)*MAX_VIRTUAL_HREC);
		memset(ADB_info[Hdb].lppVirtualHrec,0,sizeof(CHAR *)*MAX_VIRTUAL_HREC);
		EmuBindMaker(Hdb,TRUE);
		ADB_info[Hdb].AdbFilter=NULL;
	}
#endif
	return 0;
}

#ifdef _WIN32
//   +--------------------------------------------
//   | adb_Sopen Apre un file AdvancedDataBase   |
//   |                                           |
//   |  file : Nome del file da aprire           |
//   |                                           |
//   |  modo   0 : Standard (pag 4.12)           |
//   |        -1 : Accelerato                    |
//   |             Disabilita DataRecovery       |
//   |   r     -2 . Solo in lettura              |
//   |        -4 : Gestione esclusiva            |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |          -1  problemi sul file ADB        |
//   |                                           |
//   |                                           |
//   |                                           |
//   |                                           |
//   |                                           |
//   |             by Ferrà Art & Technology 2000 |
//   +-------------------------------------------+
INT adb_Sopen(CHAR *file,CHAR *Client,INT modo,INT *Hdb)
{
	INT iErr;
	adb_ErrorView(FALSE);
    iErr=adb_open(file,Client,modo,Hdb);
	adb_ErrorView(TRUE);
	return iErr;
}
#endif

//   +-------------------------------------------
//   | ADB_OpenClone Apre un dbase clone         |
//   |               con tutti i parametri di    |
//   |               un db già aperto            |
//   |                                           |
//   |                                           |
//   |  hdb  : Handle del dbase aperto           |
//   |                                           |
//   |  Ritorna Hdb del nuovo dbase              |
//   |          -1  problemi sul file ADB        |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
#ifdef _WIN32
HDB adb_OpenClone(HDB Hdb)
{
   HDB HdbNew;
   CHAR *p,File[MAXPATH];
   strcpy(File,ADB_info[Hdb].lpFileName);
   p=strReverseStr(File,"."); if (p) *p=0;
   if (adb_open(File,"",NORMAL|MEFS,&HdbNew)) ehExit("OpenClone !");
   adb_BufRealloc(Hdb,ADB_info[Hdb].ulRecBuffer,FALSE,"OpenClone",FALSE); 
 //  ulRecBuffer
   adb_Filter(Hdb,WS_CLONE,NULL,NULL,&HdbNew);
   return HdbNew;
}
#endif

// +-------------------------------------------
// | ADB_CLOSE Chiude un file AdvancedDataBase |
// |                                           |
// |  file : Nome del file da chiudere         |
// |                                           |
// |  Ritorna 0 = Tutto ok                     |
// |          > 0 Errore BTRIEVE               |
// |          < 0 File già chiuso              |
// |                                           |
// |  modo   0 : Standard                      |
// |        -1 : Accelerato                    |
// |             Disabilita DataRecovery       |
// |        -2 . Solo in lettura               |
// |        -3 : In verifica (solo DOS)        |
// |        -4 : Gestione esclusiva            |
// |                                           |
// |             by Ferrà Art & Technology 1996 |
// +-------------------------------------------+
INT adb_close(INT Hdb)
{
	BTI_SINT status=0;
	INT Hdl;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) return -9;

	if (fTransActive)
	{
		//win_infoarg("CLOSE:\nAttenzione è presente una transazione attiva\nLa transazione verra' annullata.");
		//adb_trans(Hdb,ABORT);
	}

#ifdef _ADBEMU
	// Numero di record
	if (ADB_info[Hdb].iEmulation==ADB_BTI)
	{
	 status = BTRV32(B_CLOSE,ADB_info[Hdb].lpPosBlock,0,0,0,0);
	 if (status) {BTI_err(Hdb,"CLOSE:",status);}
	}
#else
	 status = BTRV32(B_CLOSE,ADB_info[Hdb].lpPosBlock,0,0,0,0);
	 if (status) {BTI_err(Hdb,"CLOSE:",status);}
#endif

	if (ADB_info[Hdb].AdbFilter) {adb_Filter(Hdb,WS_CLOSE,NULL,NULL,NULL);}
	
#ifdef _ADBEMU

 //	--------------------------------------------------------
 //  Controllo eventuale avvio di connessione con DB
 //	--------------------------------------------------------
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
		INT j;
		EMUBIND *lpEmuBind;
		// Libero la memoria usata per il Bind
//		for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
		for (j=0;j<ADB_info[Hdb].iBindField;j++)
		{
			lpEmuBind=ADB_info[Hdb].lpEmuBind+j;
			if (lpEmuBind->fActive) 
			{
				ehFree(lpEmuBind->lpODBCField);
				if (lpEmuBind->lpSpecialTranslate) ehFree(lpEmuBind->lpSpecialTranslate);
			}
		}
		if (ADB_info[Hdb].hStmt) SQLCloseCursor(ADB_info[Hdb].hStmt);
		if (ADB_info[Hdb].hStmt) {SQLFreeHandle(SQL_HANDLE_STMT,ADB_info[Hdb].hStmt); ADB_info[Hdb].hStmt=0;}
		if (ADB_info[Hdb].hdlBind) {memoFree(ADB_info[Hdb].hdlBind,"bind"); ADB_info[Hdb].hdlBind=0;}
        if (ADB_info[Hdb].hdlDictionary) {memoFree(ADB_info[Hdb].hdlDictionary,"ODBC"); ADB_info[Hdb].hdlDictionary=0;}
		if (ADB_info[Hdb].lpHrecLastWhere) {ehFree(ADB_info[Hdb].lpHrecLastWhere); ADB_info[Hdb].lpHrecLastWhere=0;}
		if (ADB_info[Hdb].lppVirtualHrec)  
		{
			for (j=0;j<MAX_VIRTUAL_HREC;j++)
			{
				if (ADB_info[Hdb].lppVirtualHrec[j]) ehFree(ADB_info[Hdb].lppVirtualHrec[j]);
			}
			ehFree(ADB_info[Hdb].lppVirtualHrec); ADB_info[Hdb].lppVirtualHrec=0;
		}
		sEmu.iODBCCountOpen--;
		if (!sEmu.iODBCCountOpen) 
		{
			//adb_ODBCDisconnect();
		}
	}
#endif

	Hdl=ADB_info[Hdb].iHdlRam;
	memoFree(Hdl,sys.arMemoElement[Hdl].User);
    ADB_info[Hdb].lpFileName=NULL;
	ehFree(ADB_info[Hdb].lpRecBuffer);	ADB_info[Hdb].ulRecBuffer=0; 
	ehFree(ADB_info[Hdb].lpKeyBuffer);	ADB_info[Hdb].ulKeyBuffer=0; 
	memset(ADB_info+Hdb,0,sizeof(struct ADB_INFO));
	ADB_info[Hdb].iHdlRam=-1; // Libera cella
	ADB_ult--;

	return status;
}

INT adb_closeall(void)
{
 INT a;
 //		Chiude tutti i db
 for (a=0;a<ADB_max;a++) adb_close(a);
 return 0;
}


//   +-------------------------------------------
//   | ADB_Fpos  Cerca la posizione di un campo  |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_Fpos(CHAR *field,struct ADB_REC *RecInfo)
{
 INT a=0,pos=-1;

 for (;;a++)
 {
	if (RecInfo[a].tipo==STOP) break;
//	if (!strcmp(strupr(RecInfo[a].desc),strupr(field))) {pos=RecInfo[a].pos; break;}
	if (!strcmp(RecInfo[a].desc,field)) {pos=RecInfo[a].pos; break;}
 }
 return pos;
}
//   +-------------------------------------------
//   | ADB_Flen  Cerca la lunghezza di un campo  |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_Flen(CHAR *field,struct ADB_REC *RecInfo)
{
 INT a=0,pos=-1;

 for (;;a++)
 {
	if (RecInfo[a].tipo==STOP) break;
//	if (!strcmp(strupr(RecInfo[a].desc),strupr(field))) {pos=RecInfo[a].size; break;}
	if (!strcmp(RecInfo[a].desc,field)) {pos=RecInfo[a].size; break;}
 }
 return pos;
}
//   +-------------------------------------------
//   | ADB_Floc Cerca la posizione di un campo   |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_Floc(CHAR *field,struct ADB_REC *RecInfo)
{
 INT a=0,pos=-1;

 for (;;a++)
 {
	if (RecInfo[a].tipo==STOP) break;
//	if (!strcmp(strupr(RecInfo[a].desc),strupr(field))) {pos=a; break;}
	if (!strcmp(RecInfo[a].desc,field)) {pos=a; break;}
 }
 return pos;
}

//   +-------------------------------------------
//   | ADB_CREA  Crea un file AdvancedDataBase   |
//   |                                           |
//   |  file : Nome del file da creare           |
//   |  desc : Descrizione del contenuto max 30  |
//   |  RecInfo : Struttura ADB_REC              |
//   |            descrizione del record         |
//   |  Idx     : Struttura ADB_INDEX_SET        |
//   |            descrizione degli indici(max24)|
//   |  FlagFile: B_STANDARD = Default           |
//   |            Vedi pag 4-16                  |
//   |                                           |
//   |  Struttura di Link per Hierarchic Dbase   |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+

/*
INT adb_crea(CHAR *file,CHAR *desc,
			  struct ADB_REC *RecInfo,
			  struct ADB_INDEX_SET *idx,
			  INT FlagFile)
{ 
 INT a,b,rt;
 INT RecSize; 	// Grandezza record
 INT Field;   	// Numero dei campi
 INT Index;   	// Numero di indici collegati
 INT IndexDesc; // Numero descrizioni su indici collegati
 CHAR IndexUlt[ADB_MAX_IDXNAME];
 INT KeySpec; 	// Numero di specifiche di segmento
 LONG SizeMem;
 INT BTIhdl;
 INT PageSize;
 INT KeyMoreGreat;
 struct FILE_SPECS *fileSpecs;
 struct KEY_SPECS  *keySpecs;
 struct ADB_INDEX  AdbIndex[ADB_MAX_INDEX];
 struct ADB_HEAD  AdbHead;
 INT KS,flag;
 INT RealSize,FieldLoc;
 INT KeyDup=0;
 LONG Physical,MinKeyPage;
 LONG pp,pp2,pp3;
 FILE *pf1;
 const INT iMaxField=200;

 // Per btrieve       !

 BTI_BYTE ADBBuf[255];
 BTI_BYTE posBlock[128];
 BTI_WORD uiDataLen;
 BTI_SINT status;
 CHAR serv[100];

 // ------------------------------------------------------
 // Cerca dimensioni e controlla la struttura dei campi  !
 // ------------------------------------------------------
 RecSize=0; Field=0; rt=OFF;
 for (;;)
 {
	if (RecInfo[Field].tipo==STOP) break;
	//if (RecInfo[a].tipo==ADB_NOTE) FlagFile|=VAR_RECS; // Test
	switch(RecInfo[Field].tipo)
	{
	 case ADB_ALFA :
	 case ADB_NOTE :
					 RecSize+=RecInfo[Field].size+1; break;

	 case ADB_NUME : RecSize+=RecInfo[Field].size+1;
					 if (RecInfo[Field].tipo2) RecSize+=RecInfo[Field].tipo2+1;
					 break;
	 case ADB_COBD:  // Cobol Decimal New 2000
					 RecInfo[Field].size=((RecInfo[Field].size+RecInfo[Field].tipo2+1)>>1)+1;
					 //win_infoarg("COBD-Size=%d",RecInfo[Field].size);
					 RecSize+=RecInfo[Field].size;
					 break;

	 case ADB_COBN : // Cobol Numeric New 2000
					 RecInfo[Field].size=RecInfo[Field].size+RecInfo[Field].tipo2;
					 //win_infoarg("COBN-Size=%d",RecInfo[Field].size);
					 RecSize+=RecInfo[Field].size;
					 break;
	 

	 case ADB_DATA : RecSize+=8+1; RecInfo[Field].size=8; break;

	 case ADB_BOOL :
	 case ADB_INT  : RecSize+=2; RecInfo[Field].size=2; break;

	 case ADB_AINC: RecSize+=4; RecInfo[Field].size=4; break;

	 case ADB_FLOAT: RecSize+=4; RecInfo[Field].size=4; break;
	 default: win_errgrave("ADB_CREA:Probabile <tipo campo> in ADB_REC errato");
	}
	a=strlen(RecInfo[Field].desc);
	if ((a<0)||(a>(ADB_MAX_DESC-1))) {rt=ON;break;}
	if (Field++>iMaxField) break;
 }
 Field--;
 if (rt||(Field<1)||(Field++>iMaxField)||(RecSize<0))
 {
#ifdef _WIN32
	win_infoarg("ADB_CREA:Probabile ADB_REC errato Field=%d",Field);
#endif
	win_errgrave("ADB_CREA:Probabile ADB_REC errato");
 }

 //printf("File |%s|" CRLF,file);
 //printf("Numero campi : %d (%d)" CRLF,Field,RecSize);

 // -----------------------------------------
 //      Calcola le posizione dei campi     !
 // -----------------------------------------
 RecSize=0;
 for (a=0;a<Field;a++)
 {
	RecInfo[a].pos=RecSize;

	//printf("%d, Campo %s %d" CRLF,a,RecInfo[a].desc,RecInfo[a].pos);
	if (RecInfo[a].tipo==STOP) break;

	switch(RecInfo[a].tipo)
	{
	 // Aggiungo lo spazio per lo 0
	 case ADB_DATA :  
	 case ADB_ALFA : 
	 case ADB_NOTE :
					 RealSize=RecInfo[a].size+1; 
					 break;
	 // Calcolo la dimensione + 0 e decimale
	 case ADB_NUME : RealSize=RecInfo[a].size+1;
					 if (RecInfo[a].tipo2) {RealSize+=RecInfo[a].tipo2+1;}
					 break;

	 case ADB_INT  :
	 case ADB_BOOL : 
	 case ADB_FLOAT: 
	 case ADB_COBD :
	 case ADB_COBN :
	 case ADB_AINC :
					RealSize=RecInfo[a].size;
					break;
	}

	if (RealSize<=0) win_errgrave("ADB_CREA:Errore in lunghezza campo");
	RecInfo[a].RealSize=RealSize;

	// ------------------------------------------------------------------
	// Controllo che non esista un'altro campo con lo stesso nome

	for (b=0;b<a;b++)
	 {if (!strcmp(RecInfo[a].desc,RecInfo[b].desc))
				 {
				 sprintf((CHAR *) ADBBuf,"ADB_CREA:Due campi con lo stesso nome:\n%s",RecInfo[a].desc);
				 win_errgrave((CHAR *) ADBBuf);
				 }
	 }

	RecSize+=RealSize;
 }
 //printf("Grandezza record : %d" CRLF,RecSize);

 // -----------------------------------------
 //      Controlla gli Indici Collegati     !
 // -----------------------------------------

 Index=0; // Numero di indici collegati
 *IndexUlt=0;
 IndexDesc=0; // Numero di descrizioni degli indici

 rt=OFF;
 while(TRUE)
 {
	//printf("--%s %s" CRLF,idx[IndexDesc].IndexName,idx[IndexDesc].FieldName);
	if (!strcmp(idx[IndexDesc].IndexName,"FINE")) break;
	if (IndexDesc++>iMaxField) break;
	// Sempre lo stesso indice
	if (!strcmp(idx[IndexDesc-1].IndexName,IndexUlt))  {AdbIndex[Index-1].KeySeg++;continue;}

	a=strlen(idx[IndexDesc-1].IndexName);
	if ((a<0)||(a>(ADB_MAX_IDXNAME-1))) {rt=ON;break;}
	strcpy(IndexUlt,idx[IndexDesc-1].IndexName);

	// Controlla che non esista gia in indice
	for (a=0;a<Index;a++)
	{if (!strcmp(IndexUlt,AdbIndex[a].IndexName)) {rt=1;break;}
	 }
	if (rt) break;

	// Inserisce i dati nel nuovo indice
	strcpy(AdbIndex[Index].IndexName,IndexUlt); // Nome Index
	// Descrizione Index
	strcpy(AdbIndex[Index].IndexDesc,idx[IndexDesc-1].FieldName);
	AdbIndex[Index].KeySeg=0;
	Index++;
	if (Index>=ADB_MAX_INDEX) {rt=1;break;}
 }
 IndexDesc--;
 if (rt||(IndexDesc<1)||(IndexDesc++>iMaxField)) win_errgrave("ADB_CREA:Probabile ADB_INDEX_SET errato");

 // Controllo sul conteggio keysegment
 for (a=0;a<Index;a++)
 {if (AdbIndex[a].KeySeg<1) win_errgrave("ADB_CREA:Errore in ADB_INDEX_SET: KeySegment associati");
 }

 //printf(CRLF "Numero indici : %d (Descrizione %d)" CRLF,Index,IndexDesc);
 //for (a=0;a<Index;a++) printf("name : %s (Descrizione %s(%d))" CRLF,AdbIndex[a].IndexName,AdbIndex[a].IndexDesc,AdbIndex[a].KeySeg);

 KeySpec=IndexDesc-Index; // key segment necessari

 // -----------------------------------------
 // Richiesta Memoria per struttura BTRIEVE !
 // -----------------------------------------

 SizeMem=sizeof(struct FILE_SPECS)+(KeySpec*sizeof(struct KEY_SPECS))+265;

 //printf("Memoria necessaria per strutture Btrieve : %ld" CRLF,SizeMem);

 BTIhdl=memoAlloc(M_HEAP,SizeMem,"Btrieve1");

 fileSpecs=memoPtr(BTIhdl);
 keySpecs=(struct KEY_SPECS  *) farPtr((CHAR *) fileSpecs,sizeof(struct FILE_SPECS));

	// --------------------
	// Definizione chiavi !
	// --------------------
	KS=0;
	KeyMoreGreat=-1;
	KeyDup=0;

	for (a=0;a<Index;a++)
	{
	 flag=0;
		AdbIndex[a].KeySize=0;
	 for (b=0;b<IndexDesc;b++)
	 {
		if (strcmp(AdbIndex[a].IndexName,idx[b].IndexName)) continue;
		flag++;
		if (flag==1) continue;// Il primo è la descrizione
		FieldLoc=adb_Floc(idx[b].FieldName,RecInfo);
		if (FieldLoc<0)
			{sprintf(serv,"ADB_CREA:Campo in index non trovato\n[%s] ?",idx[b].FieldName);
			 win_errgrave(serv);
			 }
		keySpecs[KS].position = RecInfo[FieldLoc].pos+1; // Posizione
		keySpecs[KS].length = RecInfo[FieldLoc].RealSize; // Larghezza

		if (idx[b].flag==OFF) // Settaggio standard
				{keySpecs[KS].flags = EXTTYPE_KEY|MOD;}
				else
				{keySpecs[KS].flags = idx[b].bti_flag;
				}

		// Se il flag duplicabile è a ON
		if (idx[b].dup) keySpecs[KS].flags|=DUP;

		if (idx[b].bti_flag&DUP) KeyDup++; // Chiave Duplicabile

		// Prosegue ?
		if ((flag-1)<AdbIndex[a].KeySeg) keySpecs[KS].flags |= 0x10;

		switch (RecInfo[FieldLoc].tipo)
		{
		 case ADB_NOTE :
		 case ADB_NUME :
		 case ADB_DATA : keySpecs[KS].type = ZSTRING_TYPE; break;

		 case ADB_BOOL :
		 case ADB_INT  : keySpecs[KS].type = INTEGER_TYPE; break;

		 case ADB_FLOAT: keySpecs[KS].type = IEEE_TYPE; break;
		 case ADB_COBD : keySpecs[KS].type = DECIMAL_TYPE; break;
		 case ADB_COBN : keySpecs[KS].type = NUMERIC_TYPE; break;
		 case ADB_AINC : keySpecs[KS].type = AUTOINCREMENT_TYPE; break;
		 //case ADB_NOTE : keySpecs[KS].type = ZNOTE_TYPE; break;// Solo Pervasive.2000
		}

		keySpecs[KS].null = 0;
		AdbIndex[a].KeySize+=RecInfo[FieldLoc].RealSize;//Dimensioni chiave
		if (AdbIndex[a].KeySize>KeyMoreGreat) KeyMoreGreat=AdbIndex[a].KeySize;

		KS++;
	 }
	}

	if (KS!=KeySpec) win_errgrave("ADB_CREA:Errore in definizione indici");

 // -----------------------------------------
 //      Ottimizzazione del PagSize         !
 //      VEDI MANUALE 3.1                   !
 //                                         !
 // -----------------------------------------

 // Calcola grandezza fisica della pagina

 Physical=RecSize+2;// Per contatre il record 6.x
 Physical+=8*KeyDup;// Numero chiavi duplicabili
 if (FlagFile&VAR_RECS) Physical+=4;// Record di lunghezza variabile
 if (FlagFile&BLANK_TRUNC) Physical+=2;// Toglie gli spazi
 if (FlagFile&VATS_SUPPORT) Physical+=6;// Supportato dal VATS

 // Calcola grandezza minima della chiave
 MinKeyPage=KeyMoreGreat;
 if (KeyDup) MinKeyPage+=12; else MinKeyPage+=12;
 MinKeyPage*=8;
 MinKeyPage+=12;

 //printf("PhysicalPage %ld MinKeyPage %ld KeySegment %d" CRLF,Physical,MinKeyPage,KS);

 // Controllo dimensione per minimo key segment
 if ((KS<9)&&(MinKeyPage<512)) MinKeyPage=512;
 if ((KS<24)&&(MinKeyPage<1024)) MinKeyPage=1024;
 if ((KS<25)&&(MinKeyPage<1536)) MinKeyPage=1536;
 if ((KS<55)&&(MinKeyPage<2048)) MinKeyPage=2048;
 if ((KS<120)&&(MinKeyPage<4096)) MinKeyPage=4096;

 b=-1; pp=100000L;
 for (a=1;a<9;a++)
// for (a=1;a<18;a++)
 {
	pp2=(LONG) a*512;
	if (pp2<MinKeyPage) continue;
	pp2-=6; // Domensioni pagina
	pp3=pp2/Physical; pp3*=Physical;
	pp2-=pp3;
	//printf("pp2:%ld pp3:%ld pagesize=%d" CRLF,pp2,pp3,a*512);
	if ((pp2<pp)&&(pp3>0)&&(pp3>0)) {pp=pp2; b=a;}
 }

 if (b==-1) win_errgrave("ADB_CREA:Mega errore grave");
 PageSize=b*512;
 //printf("Page Size %d (512*%d) Phy%ld" CRLF,PageSize,b,Physical);

 // -----------------------------------------
 //        RIEMPE LA STRUTTURA BTRIEVE      !
 // -----------------------------------------

	fileSpecs[0].recLength = RecSize; // Len record
	fileSpecs[0].pageSize = PageSize;
	fileSpecs[0].indexCount = Index; // Numero indici
	fileSpecs[0].flags = FlagFile; // Vedi pag. 4-16
	fileSpecs[0].allocations = 0; // Pagine da allocare

	// -------------------------------------------------------
	//           CHIAMATA IN CREAZIONE DI BTRIEVE            !
	// -------------------------------------------------------
	//getch();
	strcpy((BTI_CHAR *) ADBBuf, file );
	strcat((BTI_CHAR *) ADBBuf,".BTI");
	uiDataLen=(INT ) SizeMem;

	status = BTRV( B_CREATE,
				   posBlock,
				   memoPtr(BTIhdl),// Puntatore alla strutture
				   &uiDataLen, // Lunghezza del dato
				   ADBBuf, // nome del File
				   0); // 0= nessun controllo sull'esitenza del file

	if (status) {BTI_err(file,"CREAZIONE",status); return status;}

	// -------------------------------------------------------
	//           CREAZIONE FILE ADB DI RIFERIMENTO           !
	// -------------------------------------------------------
	strcpy((BTI_CHAR *)ADBBuf,file);
	strcat((BTI_CHAR *)ADBBuf,".ADB");
	rt=OFF;
	if (f_open((BTI_CHAR *)ADBBuf,"wb+",&pf1)) rt=ON;

	//-------------------
	// Scrittura Header !
	//-------------------

	strcpy(AdbHead.id,"Adb1.2 Eh3");
	memset(AdbHead.DescFile, 0, 30); // Pulisce il record
	strcpy(AdbHead.DescFile,desc);
	AdbHead.eof=26;
	AdbHead.Field=Field;
	AdbHead.RecSize=RecSize;
	AdbHead.KeyMoreGreat=KeyMoreGreat;
	AdbHead.Index=Index;
	AdbHead.IndexDesc=IndexDesc;
	AdbHead.KeySeg=KS;
	if (f_put(pf1,NOSEEK,&AdbHead,sizeof(struct ADB_HEAD))) {rt=ON; goto fine;}

	//------------------------
	// Scrittura Dati Record !
	//------------------------
	for (a=0;a<Field;a++)
	{
	//strupr(RecInfo[a].desc);
	if (f_put(pf1,NOSEEK,&RecInfo[a],sizeof(struct ADB_REC))) {rt=ON; goto fine;}
	}

	//------------------------
	// Scrittura Dati Index  !
	//------------------------
	for (a=0;a<Index;a++)
	{
	if (f_put(pf1,NOSEEK,&AdbIndex[a],sizeof(struct ADB_INDEX))) {rt=ON; goto fine;}
	}

	//------------------------------------
	// Scrittura Dati Descrizione Index  !
	//------------------------------------
	for (a=0;a<IndexDesc;a++)
	{
	//strupr(idx[a].FieldName);
	if (f_put(pf1,NOSEEK,&idx[a],sizeof(struct ADB_INDEX_SET))) {rt=ON; goto fine;}
	}

	fclose(pf1);

	fine:
	if (rt) win_errgrave("ADB_CREA:Errore in creazione .ADB");

 memoFree(BTIhdl,"adb_crea");

 //------------------------------------
 // Aggiunta del 97 - Proprietario    !
 //------------------------------------

 return 0;
}
*/
//   +-------------------------------------------
//   | ADB_CREA  Crea un file AdvancedDataBase   |
//   |                                           |
//   |  file : Nome del file da creare           |
//   |  desc : Descrizione del contenuto max 30  |
//   |  RecInfo : Struttura ADB_REC              |
//   |            descrizione del record         |
//   |  Idx     : Struttura ADB_INDEX_SET        |
//   |            descrizione degli indici(max24)|
//   |  FlagFile: B_STANDARD = Default           |
//   |            Vedi pag 4-16                  |
//   |                                           |
//   |  Struttura di Link per Hierarchic Dbase   |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+

//int adb_crea(char *file,char *desc,struct ADB_REC *RecInfo,struct ADB_INDEX_SET *idx,int FlagFile)
INT adb_crea(CHAR * file,
			  CHAR *desc,
			  struct ADB_REC * RecInfo,
			  struct ADB_INDEX_SET * idx,
			  INT FlagFile,
			  BOOL fSoloAdb)
{ 
	INT a,b,rt;
	INT RecSize; 	// Grandezza record
	INT Field;   	// Numero dei campi
	INT Index;   	// Numero di indici collegati
	INT IndexDesc; // Numero descrizioni su indici collegati
	CHAR IndexUlt[ADB_MAX_IDXNAME];
	INT KeySpec; 	// Numero di specifiche di segmento
	LONG SizeMem;
	INT BTIhdl;
	INT PageSize;
	INT KeyMoreGreat;
	struct FILE_SPECS *fileSpecs;
	struct KEY_SPECS  *keySpecs;
	struct ADB_INDEX  AdbIndex[ADB_MAX_INDEX];
	struct ADB_HEAD  AdbHead;
	INT KS,flag;
	INT RealSize,FieldLoc;
	INT KeyDup=0;
	LONG Physical,MinKeyPage;
	LONG pp,pp2,pp3;
	FILE *pf1;
	const INT iMaxField=200;

	// Per btrieve       !

	BTI_BYTE ADBBuf[255];
	BTI_BYTE posBlock[128];
	BTI_ULONG uiDataLen;
	BTI_SINT status;
	CHAR serv[100];
	BOOL fVarCharPresent=FALSE;

	// ------------------------------------------------------
	// Cerca dimensioni e controlla la struttura dei campi  !
	// ------------------------------------------------------
	RecSize=0; Field=0; rt=OFF;
	for (;;)
	{
		if (RecInfo[Field].tipo==STOP) break;
		switch(RecInfo[Field].tipo)
		{
		 case ADB_ALFA : RecSize+=RecInfo[Field].size+1; 
						 break;
		 case ADB_BLOB : 
						 //FlagFile|=VAR_RECS;
						 fVarCharPresent=TRUE;
						 RecInfo[Field].size=8;
						 RecSize+=8;
						 break;

		 case ADB_NUME : RecSize+=RecInfo[Field].size+1;
						 if (RecInfo[Field].tipo2) RecSize+=RecInfo[Field].tipo2+1;
						 break;

		 case ADB_COBD:  // Cobol Decimal New 2000
						 RecInfo[Field].RealSize=((RecInfo[Field].size+RecInfo[Field].tipo2+1)>>1)+1;
						 //win_infoarg("COBD-Size=%d",RecInfo[Field].size);
						 RecSize+=RecInfo[Field].RealSize;
						 break;

		 case ADB_COBN : // Cobol Numeric New 2000
						 RecInfo[Field].RealSize=RecInfo[Field].size+RecInfo[Field].tipo2;
						 //win_infoarg("COBN-Size=%d",RecInfo[Field].size);
						 RecSize+=RecInfo[Field].RealSize;
						 break;
		 

		 case ADB_DATA : RecSize+=8+1; RecInfo[Field].size=8; break;

		 case ADB_BOOL :
		 case ADB_INT  : RecSize+=2; RecInfo[Field].size=2; break;


		 case ADB_INT32:
		 case ADB_AINC: RecSize+=4; RecInfo[Field].size=4; break;

		 case ADB_FLOAT: RecSize+=4; RecInfo[Field].size=4; break;
		 default: 
			 //win_errgrave("ADB_CREA:Probabile <tipo campo> in ADB_REC errato");
			 ehExit("ADB_CREA:Probabile <tipo campo> in ADB_REC errato = n.%d > %d",Field,RecInfo[Field].tipo);
		}
		a=strlen(RecInfo[Field].desc);
		if ((a<0)||(a>(ADB_MAX_DESC-1))) {rt=ON;break;}
		if (Field++>iMaxField) break;
	}

	Field--;
	if (rt||(Field<1)||(Field++>iMaxField)||(RecSize<0))
	{
		#ifdef _WIN32
		win_infoarg("ADB_CREA:Probabile ADB_REC errato Field=%d",Field);
		#endif
		win_errgrave("ADB_CREA:Probabile ADB_REC errato");
	}

 //printf("File |%s|" CRLF,file);
 //printf("Numero campi : %d (%d)" CRLF,Field,RecSize);

	// -----------------------------------------
	//      Calcola le posizione dei campi     !
	// -----------------------------------------
	RecSize=0;
	for (a=0;a<Field;a++)
	{
		RecInfo[a].pos=RecSize;

		//printf("%d, Campo %s %d" CRLF,a,RecInfo[a].desc,RecInfo[a].pos);
		if (RecInfo[a].tipo==STOP) break;

		switch(RecInfo[a].tipo)
		{
		 // Aggiungo lo spazio per lo 0
		 case ADB_DATA :  
		 case ADB_ALFA :
						 RealSize=RecInfo[a].size+1; 
						 break;

		 case ADB_BLOB: RealSize=8; 
						 RecInfo[a].size=8;
						 break;

		 // Calcolo la dimensione + 0 e decimale
		 case ADB_NUME : RealSize=RecInfo[a].size+1;
						 if (RecInfo[a].tipo2) {RealSize+=RecInfo[a].tipo2+1;}
						 break;

		 case ADB_INT  :
		 case ADB_BOOL : 
		 case ADB_FLOAT: 
		 case ADB_AINC :
		 case ADB_INT32:
						RealSize=RecInfo[a].size;
						break;

		 case ADB_COBD :
		 case ADB_COBN :
						RealSize=RecInfo[a].RealSize;
						break;
	}

	if (RealSize<=0) win_errgrave("ADB_CREA:Errore in lunghezza campo");
	RecInfo[a].RealSize=RealSize;

	// ------------------------------------------------------------------
	// Controllo che non esista un'altro campo con lo stesso nome

	for (b=0;b<a;b++)
	 {if (!strcmp(RecInfo[a].desc,RecInfo[b].desc))
				 {
				 sprintf((CHAR *) ADBBuf,"ADB_CREA:Due campi con lo stesso nome:\n%s",RecInfo[a].desc);
				 win_errgrave((CHAR *) ADBBuf);
				 }
	 }

	RecSize+=RealSize;
 }
 //printf("Grandezza record : %d" CRLF,RecSize);

	// -----------------------------------------
	//      Controlla gli Indici Collegati     !
	// -----------------------------------------

	Index=0; // Numero di indici collegati
	*IndexUlt=0;
	IndexDesc=0; // Numero di descrizioni degli indici

	rt=OFF;
	for (;;)
	{
		if (!strcmp(idx[IndexDesc].IndexName,"FINE")) break;
		if (IndexDesc++>iMaxField) break;
		// Sempre lo stesso indice
		if (!strcmp(idx[IndexDesc-1].IndexName,IndexUlt))
			{AdbIndex[Index-1].KeySeg++;
			 continue;}

		a=strlen(idx[IndexDesc-1].IndexName);
		if ((a<0)||(a>(ADB_MAX_IDXNAME-1))) {rt=ON;break;}
		strcpy(IndexUlt,idx[IndexDesc-1].IndexName);

		// Controlla che non esista gia in indice
		for (a=0;a<Index;a++)
		{if (!strcmp(IndexUlt,AdbIndex[a].IndexName)) {rt=1;break;}
		 }
		if (rt) break;

		// Inserisce i dati nel nuovo indice
		strcpy(AdbIndex[Index].IndexName,IndexUlt); // Nome Index
		// Descrizione Index
		strcpy(AdbIndex[Index].IndexDesc,idx[IndexDesc-1].FieldName);
		AdbIndex[Index].KeySeg=0;
		Index++;
		if (Index>=ADB_MAX_INDEX) {rt=1;break;}
	}
	IndexDesc--;
	if (rt||(IndexDesc<1)||(IndexDesc++>iMaxField)) win_errgrave("ADB_CREA:Probabile ADB_INDEX_SET errato");

	// Controllo sul conteggio keysegment
	for (a=0;a<Index;a++)
	{if (AdbIndex[a].KeySeg<1) win_errgrave("ADB_CREA:Errore in ADB_INDEX_SET: KeySegment associati");
	}

	//printf(CRLF "Numero indici : %d (Descrizione %d)" CRLF,Index,IndexDesc);
	//for (a=0;a<Index;a++) printf("name : %s (Descrizione %s(%d))" CRLF,AdbIndex[a].IndexName,AdbIndex[a].IndexDesc,AdbIndex[a].KeySeg);

	KeySpec=IndexDesc-Index; // key segment necessari

	// -----------------------------------------
	// Richiesta Memoria per struttura BTRIEVE !
	// -----------------------------------------

	SizeMem=sizeof(struct FILE_SPECS)+(KeySpec*sizeof(struct KEY_SPECS))+265;

	//printf("Memoria necessaria per strutture Btrieve : %ld" CRLF,SizeMem);

	BTIhdl=memoAlloc(M_HEAP,SizeMem,"Btrieve1");

	fileSpecs=memoPtr(BTIhdl,NULL);
	keySpecs=(struct KEY_SPECS  *) ((BYTE *) fileSpecs+sizeof(struct FILE_SPECS));


	// --------------------
	// Definizione chiavi !
	// --------------------
	KS=0;
	KeyMoreGreat=-1;
	KeyDup=0;

	for (a=0;a<Index;a++)
	{
	 flag=0;
	 AdbIndex[a].KeySize=0;
	 for (b=0;b<IndexDesc;b++)
	 {
		if (strcmp(AdbIndex[a].IndexName,idx[b].IndexName)) continue;
		flag++;
		if (flag==1) continue;// Il primo è la descrizione
		FieldLoc=adb_Floc(idx[b].FieldName,RecInfo);
		if (FieldLoc<0)
		{
			sprintf(serv,"ADB_CREA:Campo in index non trovato\n[%s] ?",idx[b].FieldName);
			win_errgrave(serv);
		}
		keySpecs[KS].position = RecInfo[FieldLoc].pos+1; // Posizione
		keySpecs[KS].length = RecInfo[FieldLoc].RealSize; // Larghezza
		
//		win_infoarg("%d-%d",b,IndexDesc);

		if (!idx[b].flag) // Settaggio standard
		//if (!idx[b].bti_flag)
				{keySpecs[KS].flags = EXTTYPE_KEY|MOD; }
				else
				{keySpecs[KS].flags = idx[b].flag;}//idx[b].bti_flag;}

		// Se il flag duplicabile è a ON 
		if (idx[b].dup) keySpecs[KS].flags|=DUP;
		if (idx[b].bti_flag&DUP) KeyDup++; // Chiave Duplicabile

		// Prosegue ?
		if ((flag-1)<AdbIndex[a].KeySeg) keySpecs[KS].flags |= 0x10;

		switch (RecInfo[FieldLoc].tipo)
		{
		 case ADB_ALFA :
		 case ADB_NUME :
		 case ADB_DATA :
		 case ADB_BLOB: 
						keySpecs[KS].type = ZSTRING_TYPE; break;

		 case ADB_BOOL :
		 case ADB_INT  : 
		 case ADB_INT32: keySpecs[KS].type = INTEGER_TYPE; break;

		 case ADB_FLOAT: keySpecs[KS].type = IEEE_TYPE; break;
		 case ADB_COBD : keySpecs[KS].type = DECIMAL_TYPE; break;
		 case ADB_COBN : keySpecs[KS].type = NUMERIC_TYPE; break;
		 case ADB_AINC : keySpecs[KS].type = AUTOINCREMENT_TYPE; 
						 keySpecs[KS].flags &=~MOD;
						 break;
		}

		keySpecs[KS].null = 0;
		AdbIndex[a].KeySize+=RecInfo[FieldLoc].RealSize;//Dimensioni chiave
		if (AdbIndex[a].KeySize>KeyMoreGreat) KeyMoreGreat=AdbIndex[a].KeySize;

		/*
		printf("idx:%d ks:%d %-7s flg:%4x type:%4d ps:%d (ln%d) fg%d KyS%d <_>%d" CRLF,
					 a,KS,RecInfo[FieldLoc].desc,
					 keySpecs[KS].flags,
					 keySpecs[KS].type,
					 keySpecs[KS].position,
					 keySpecs[KS].length,flag,AdbIndex[a].KeySeg,
					 AdbIndex[a].KeySize);
			*/
		KS++;
	 }
	}

	if (KS!=KeySpec) win_errgrave("ADB_CREA:Errore in definizione indici");

	// -----------------------------------------
	//      Ottimizzazione del PagSize         !
	//      VEDI MANUALE 3.1                   !
	//                                         !
	// -----------------------------------------

	// Calcola grandezza fisica della pagina

	Physical=RecSize+2;// Per contatre il record 6.x
	Physical+=8*KeyDup;// Numero chiavi duplicabili
	if (fVarCharPresent) FlagFile|=VAR_RECS;
	if (FlagFile&VAR_RECS) Physical+=4;// Record di lunghezza variabile
	if (FlagFile&BLANK_TRUNC) Physical+=2;// Toglie gli spazi
	if (FlagFile&VATS_SUPPORT) Physical+=6;// Supportato dal VATS

	// Calcola grandezza minima della chiave
	MinKeyPage=KeyMoreGreat;
	if (KeyDup) MinKeyPage+=12; else MinKeyPage+=12;
	MinKeyPage*=8;
	MinKeyPage+=12;


	// Controllo dimensione per minimo key segment

	if ((KS<9)&&(MinKeyPage<512)) MinKeyPage=512;
	if ((KS<24)&&(MinKeyPage<1024)) MinKeyPage=1024;
	if ((KS<25)&&(MinKeyPage<1536)) MinKeyPage=1536;
	if ((KS<55)&&(MinKeyPage<2048)) MinKeyPage=2048;
	if ((KS<120)&&(MinKeyPage<4096)) MinKeyPage=4096;

	b=-1; pp=100000L;

	for (a=1;a<9;a++)
	{
		pp2=(LONG) a*512;
		if (pp2<MinKeyPage) continue;
		pp2-=6; // Domensioni pagina
		pp3=pp2/Physical; pp3*=Physical;
		pp2-=pp3;
		//printf("pp2:%ld pp3:%ld pagesize=%d" CRLF,pp2,pp3,a*512);
		if ((pp2<pp)&&(pp3>0)&&(pp3>0)) {pp=pp2; b=a;}
	}

	if (b==-1) win_errgrave("ADB_CREA:Mega errore grave");
	PageSize=b*512;
	//printf("Page Size %d (512*%d) Phy%ld" CRLF,PageSize,b,Physical);

 // -----------------------------------------
 //        RIEMPE LA STRUTTURA BTRIEVE      !
 // -----------------------------------------

	fileSpecs[0].recLength = RecSize; // Len record
	fileSpecs[0].pageSize = PageSize;
	fileSpecs[0].indexCount = Index; // Numero indici
	fileSpecs[0].flags = FlagFile; // Vedi pag. 4-16
	fileSpecs[0].allocations = 0; // Pagine da allocare

	// -------------------------------------------------------
	//           CHIAMATA IN CREAZIONE DI BTRIEVE            !
	// -------------------------------------------------------
	//getch();
	strcpy((BTI_CHAR *)ADBBuf, file );
	strcat((BTI_CHAR *)ADBBuf,".BTI");
	uiDataLen=(INT ) SizeMem;
	if (fSoloAdb)
	{
		status=0;
	}
	else
	{
		status = BTRV32(B_CREATE,
					   posBlock,
					   memoPtr(BTIhdl,NULL),// Puntatore alla strutture
					   &uiDataLen, // Lunghezza del dato
					   ADBBuf, // nome del File
					   0); // 0= nessun controllo sull'esitenza del file
	}
	if (status) {BTI_err(0,"CREAZIONE",status); return status;}

	// -------------------------------------------------------
	//           CREAZIONE FILE ADB DI RIFERIMENTO           !
	// -------------------------------------------------------
	strcpy((BTI_CHAR *)ADBBuf,file);
	strcat((BTI_CHAR *)ADBBuf,".ADB");
	rt=OFF;
	pf1=fopen(ADBBuf,"wb+"); if (!pf1) rt=ON;

	//-------------------
	// Scrittura Header !
	//-------------------

	strcpy(AdbHead.id,"Adb1.2 Eh3");
	memset(AdbHead.DescFile, 0, 30); // Pulisce il record
	strcpy(AdbHead.DescFile,desc);
	AdbHead.eof=26;
	AdbHead.Field=Field;
	if (fVarCharPresent) RecSize=0; // Indefinibile
	AdbHead.wRecSize=RecSize;
	AdbHead.KeyMoreGreat=KeyMoreGreat;
	AdbHead.Index=Index;
	AdbHead.IndexDesc=IndexDesc;
	AdbHead.KeySeg=KS;
	if (!fwrite(&AdbHead,sizeof(struct ADB_HEAD),1,pf1)) {rt=ON; goto fine;}

	//------------------------
	// Scrittura Dati Record !
	//------------------------
	if (!fwrite(RecInfo,sizeof(struct ADB_REC),Field,pf1)) {rt=ON; goto fine;}
	/*
	for (a=0;a<Field;a++)
	{
		if (!fwrite(&RecInfo[a],sizeof(struct ADB_REC),1,pf1)) {rt=ON; goto fine;}
	}*/

	//------------------------
	// Scrittura Dati Index  !
	//------------------------
	if (!fwrite(AdbIndex,sizeof(struct ADB_INDEX),Index,pf1)) {rt=ON; goto fine;}
	/*
	for (a=0;a<Index;a++)
	{
		if (!fwrite(&AdbIndex[a],sizeof(struct ADB_INDEX),1,pf1)) {rt=ON; goto fine;}
	}
	*/

	//------------------------------------
	// Scrittura Dati Descrizione Index  !
	//------------------------------------
	if (!fwrite(idx,sizeof(struct ADB_INDEX_SET),IndexDesc,pf1)) {rt=ON; goto fine;}
	/*
	for (a=0;a<IndexDesc;a++)
	{
	//strupr(idx[a].FieldName);
		if (!fwrite(&idx[a],sizeof(struct ADB_INDEX_SET),1,pf1)) {rt=ON; goto fine;}
	}
	*/
	fclose(pf1);

	fine:
	if (rt) ehAlert("ADB_CREA:Errore in creazione .ADB");

	memoFree(BTIhdl,"adb_crea");

	//------------------------------------
	// Aggiunta del 97 - Proprietario    !
	//------------------------------------
	return rt;
}

//   +-------------------------------------------
//   | ADB_ERRGRAVE Errore in ADB                |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
void adb_errgrave(CHAR *chi,INT Hdb)
{
 ehExit("adb_errgrave():\nAdbHadle non valido in %s [%d]",chi,Hdb);
}

CHAR *adb_data(CHAR *ptd)
{
 static CHAR serv[14],*p=serv;

 if (!*ptd) {*serv=0;return serv;}
 memcpy(p,ptd+6,2); // Anno
 memcpy(p+2,ptd+4,2); // Anno
 memcpy(p+4,ptd,4); // Anno
 serv[8]=0;
 return serv;
}

CHAR *adb_GetDate(HDB Hdb,CHAR *Nome) {return adb_data(adbFieldPtr(Hdb,Nome));}

//   +-------------------------------------------
//   | ADB_NCCUPDATE Modifica il record corrente |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+
INT adb_NCCupdate(INT Hdb)
{
	BTI_ULONG uiDataLen;
	BTI_SINT  status;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("update",Hdb);

	if (ADB_info[Hdb].fBlobPresent) uiDataLen=adb_BlobNewOfs(Hdb,0);
									 else
									 uiDataLen=ADB_info[Hdb].AdbHead->wRecSize;
	status = BTRV32( B_UPDATE,
				    ADB_info[Hdb].lpPosBlock,
					ADB_info[Hdb].lpRecBuffer,// Puntatore ai dati
					&uiDataLen, // Grandezza dei dati
					ADB_info[Hdb].lpKeyBuffer,   // ???> in output
					-1); // ??? in input

	// Lock Errore
	if (status==81) return status;
	if (status) {BTI_err(Hdb,"NCCUPDATE:",status);}

	return status;
}

//   +-------------------------------------------
//   | ADB_TRANS  Gestione delle transizioni     |
//   |                                           |
//   |  Hdb : handle del file ADB               |
//   |  flag = ON    Inizio transizione          |
//   |         OFF   Fine transizione            |
//   |         ABORT Annulla transizione         |
//   |                                           |
//   |                                           |
//   |                                           |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferrà Art & Technology 1996 |
//   +-------------------------------------------+

INT adb_trans(INT Hdb,INT flag)
{
	BTI_SINT status;
	CHAR serv[15];

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("Trans",Hdb);

	switch (flag)
	{
	 case ON:

		strcpy(serv,"BEGIN_TRANS");
		status = BTRV32(1000+B_BEGIN_TRAN,
					    ADB_info[Hdb].lpPosBlock,
					    ADB_info[Hdb].lpRecBuffer,// Puntatore ai dati
					    NULL, // Grandezza dei dati
					    ADB_info[Hdb].lpKeyBuffer,   // ???> in output
					   0); // ??? in input
		fTransActive=TRUE;
		break;

	 case OFF:

		strcpy(serv,"END_TRANS");
		status = BTRV32(B_END_TRAN,
					    ADB_info[Hdb].lpPosBlock,
					    ADB_info[Hdb].lpRecBuffer,// Puntatore ai dati
					    NULL, // Grandezza dei dati
					    ADB_info[Hdb].lpKeyBuffer,   // ???> in output
					    0); // ??? in input
		fTransActive=FALSE;
		break;

	 case ABORT:

		strcpy(serv,"ABORT_TRANS");
		status = BTRV32( B_ABORT_TRAN,
					   ADB_info[Hdb].lpPosBlock,
					   ADB_info[Hdb].lpRecBuffer,// Puntatore ai dati
					   NULL, // Grandezza dei dati
					   ADB_info[Hdb].lpKeyBuffer,   // ???> in output
					  0); // ??? in input
		fTransActive=FALSE;
		break;

	 default :
			ehExit("Flag error in ADB_Tras");

	}

	if (status) {BTI_err(Hdb,serv,status);}

	return status;
}

//  +----------------------------------------------------------------
//	|                                                                |
//	|                     GESTORE LINK GERARCHICI                    |
//	|                                                                |
//	+----------------------------------------------------------------+

#ifdef HLINK

double HLcheck(struct ADB_LINK_LIST *link)
{
//	CHAR *p;
	BYTE *p;
	INT register b;
	double check=0;

#ifdef _WIN32
	#pragma pack(1)
	struct ADB_LINK_LISTCHECK {
		CHAR FilePadre[80]; // Nome del file padre
		CHAR FieldNamePadre[ADB_MAX_DESC]; // nome del campo collegato
		CHAR FileFiglio[80]; // Nome del file figlio ( si deve trovare nel percoro del padre
		CHAR FieldNameFiglio[ADB_MAX_DESC]; // nome del campo collegato
		CHAR IndexNameFiglio[ADB_MAX_IDXNAME]; // Key group di riferimento
		CHAR Opzione[30]; // Comando al linker : SCAN o +\"string\" o +n
		INT16 Clone;
		CHAR Proprietario[30];
	 };

    struct ADB_LINK_LISTCHECK Check;
	#pragma pack()

	
	
	
	
	
	
	//win_infoarg("%d",sizeof(Check));
	memset(&Check,0,sizeof(Check));

	strcpy(Check.FilePadre,link->FilePadre);
	strcpy(Check.FieldNamePadre,link->FieldNamePadre);
	strcpy(Check.FileFiglio,link->FileFiglio);
	strcpy(Check.FieldNameFiglio,link->FieldNameFiglio);
	strcpy(Check.IndexNameFiglio,link->IndexNameFiglio);
	strcpy(Check.Opzione,link->Opzione);
	strcpy(Check.Proprietario,link->Proprietario);
	Check.Clone=(INT16) link->Clone;

	p=(CHAR *) &Check;
	for (b=0;b<sizeof(Check);b++) {check+=(double) *p;p++;}
#else

	p=(CHAR *) link;
	for (b=0;b<sizeof(struct ADB_LINK_LIST);b++) {check+=(double) *p;p++;}
#endif

	return check;
}

//  +-----------------------------------------
//	| ADB_HLfile Definisce l'attivazione      |
//	|            e il file Tabella            |
//	|                                         |
//	|                                         |
//	| file Nome e percorso del file tabella   |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+
void adb_HLfile(CHAR *file)
{
 if (sys.iSystemStatus) ehExit("adb_HLfile:La funzione và chiamata prima del ehStart()");
 if ((strlen(file)+1)>MAXPATH) 
#ifdef _WIN32
 {win_info("Nome File HLTabella errato");}
#else
 {printf("Nome File HLTabella errato");}
#endif
 strcpy(HLtabella,file);
 ADB_HL=ON; // attivo il Link Gerarchico
}

// +-----------------------------------------
//	| ADB_HLfind Cerca nella tabella un file  |
//	|                                         |
//	| file Nome del file padre                |
//	|                                         |
//	| Ritorna ON se cè OFF se non c'è         |
//	|         in pt il puntatore al campo     |
//	|         o se non cè alla sua probabile  |
//	|         posizione nella lista (insert)  |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+
static INT adb_HLfind(CHAR *file,LONG *pt)
{
	LONG lp;
	struct ADB_LINK_LIST adbL;

	for (lp=0;lp<adb_HLink;lp++)
	{
		memoRead(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));

		if (!strcmp(_strupr(adbL.FilePadre),_strupr(file)))
		{
			*pt=lp;
			return ON;
		}

/**
		if (strcmp(strupr(adbL.FilePadre),strupr(file)))
		{
			*pt=lp;
			return OFF;
		}
**/
	}

	*pt=lp;

	return OFF;
}

// +-----------------------------------------------
//	| ADB_HLfindFiglio Cerca nella tabella un file  |
//	|                                               |
//	| file Nome del file figlio da cercare          |
//	|                                               |
//	| Ritorna ON se cè OFF se non c'è               |
//	|         in pt il puntatore al campo           |
//	|         o se non cè alla sua probabile        |
//	|         posizione nella lista (insert)        |
//	|                                               |
//	|                           by Luca Vadora 1997 |
//	+-----------------------------------------------+
static INT adb_HLfindFiglio (CHAR *file,LONG *pt)
{
	LONG lp;

	struct ADB_LINK_LIST adbL;

	for (lp = 0;lp < adb_HLink;lp++)
	{
		memoRead(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));

		if (!strcmp(_strupr(adbL.FileFiglio),_strupr(file)))
		{
			*pt = lp;
			return ON;
		}

/**
		if (strcmp(strupr(adbL.FilePadre),strupr(file)))
		{
			*pt=lp;
			return OFF;
		}
**/
	}

	*pt = lp;

	return OFF;
}

//  +-----------------------------------------
//	| ADB_HLdelete Cancella nella tabella     |
//	|              gerarchica i riferimenti   |
//	|              di link di un determinato  |
//	|              file                       |
//	|                                         |
//	|  file Nome del file                     |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+
static INT adb_HLdelete(CHAR *FilePadre)
{
 INT a;
 LONG lp,pt,ptv;
 LONG NrLink;
 struct ADB_LINK_LIST adbL;

 if (adb_HLink==0) return 0; // Non ci sono Link da cancellare
 if (adb_HLinkHdl==-1) ehExit("adb_Ldelete");
 //printf("DELETE:" CRLF);
 a=adb_HLfind(fileName(FilePadre),&pt);
 if (a==OFF) return 0;// il File Padre non esiste
 //printf("%s pt=%ld",FilePadre,pt);
 //getch();
 // ------------------------------
 // Azzera i Link del file padre !
 // e riorganizza l'archivio     !
 // ------------------------------

 NrLink=pt;
 ptv=pt;
 for (lp=pt;lp<adb_HLink;lp++)
 {
	memoRead(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));
	if (!strcmp(fileName(_strupr(FilePadre)),adbL.FilePadre)) continue;
	memoWrite(adb_HLinkHdl,ptv*sizeof(adbL),&adbL,sizeof(adbL));
	NrLink++; ptv++;
 }
 adb_HLink=NrLink;
 return 0;
}
//  +-----------------------------------------
//	| ADB_HLapri                              |
//	| (Hierarchic Link Table)                 |
//	|                                         |
//	| - Carica la tabella dei Link Gerarchici |
//	|   in memoria                            |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+

INT adb_HLapri(void)
{
 FILE *pf1;
 CHAR buf[200];
 LONG ptm;
 struct ADB_LINK_LIST adbL;
// INT a;
 double Check,CheckFile;

 if (ADB_HL==OFF) ehExit("Manca l'indicazione del FILE di tabella");
 // Chiude una precedente tabella se esiste
 adb_HLchiudi();

 //printf("WS_OPEN:" CRLF);

 // -----------------------
 // Apre il file          !
 // -----------------------

 adb_HLink=0;

 pf1=fopen(HLtabella,"r");

 if (pf1)
	{// Conta i link stabiliti
		 for (;;)
		 {
			if (!fgets(buf,sizeof(buf),pf1)) break;
			adb_HLink++;
		 }

		 fseek(pf1,0,SEEK_SET);
		 //f_get(pf1,0,buf,0); // resetta il puntatore all'inizio
		 // Se è maggiore di zero li carica
		 if (adb_HLink>0)
		 {
			adb_HLinkHdl=memoAlloc(RAM_AUTO,adb_HLink*sizeof(struct ADB_LINK_LIST),"*HLINK");
			if (adb_HLinkHdl<0) ehExit("No Memo per adb_link()");
			// Carica in memoria i dati e guarda se sono buoni
			CheckFile=0; Check=0;
			for (ptm=0;ptm<adb_HLink;ptm++)
			{
			 //if (f_gets(pf1,buf,sizeof(buf))==NULL) ehExit("Strano ?");
			 adb_HLload(pf1,&adbL);

			 if (!strcmp(adbL.FilePadre,".CHECK."))
					{CheckFile=atof(adbL.FileFiglio);
					 break;}

/**			printf("Apri >[%s][%s][%s][%s][%s][%s][%d][%s]" CRLF,
							adbL.FilePadre,
							adbL.FieldNamePadre,
							adbL.FileFiglio,
							adbL.FieldNameFiglio,
							adbL.IndexNameFiglio,
							adbL.Opzione,
							adbL.Clone,
							adbL.Proprietario);
			getch();
**/
			 memoWrite(adb_HLinkHdl,ptm*sizeof(adbL),&adbL,sizeof(adbL));
			 Check+=HLcheck(&adbL);

			}
//			printf ("Check apri = %f - %f\n",Check,CheckFile);
//			getch ();

//			if (Check!=CheckFile) ehExit("Checksum:Errore in ADB_HLload");
			if (Check!=CheckFile) win_info("Checksum:Errore in ADB_HLload");
			adb_HLink--;// Tolgo la riga del Checksum
		 }
	fclose(pf1);
	return OFF;
	}
	else
	{
	 if (!sys.iSystemStatus)
	 {
#ifdef _WIN32
	    win_infoarg("ATTENZIONE:\nManca la tabella dei link [%s]",HLtabella);
#else
		printf("ATTENZIONE:\nManca la tabella dei link [%s]",HLtabella);
		getch();
#endif
	 }
	}
 return ON;
}

static void adb_HLchiudi(void)
{
 //printf("WS_CLOSE:" CRLF);

 if (adb_HLinkHdl>-1) memoFree(adb_HLinkHdl,"*HLINK");
 adb_HLinkHdl=-1;
}

// +-----------------------------------------
//	| ADB_HLload  Legge da disco un link      |
//	|             gerarchico                  |
//	|                                         |
//	|  file      = Nome del file              |
//	|  path      = Path figli                 |
//	|              se non indicato nella stru |
//	|              ttura di link verrà assunto|
//	|              di default questo path     |
//	|  link      = Struttura di Link che      |
//	|              definisce i nuovi link     |
//	|              da inserire                |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+

static void adb_HLload(FILE *pf1,struct ADB_LINK_LIST *link)
{
	CHAR *p,*ptr[7];
	CHAR *p2;
 //	CHAR serv[200];
	CHAR buf[200];

	INT a;

	if (!fgets(buf,sizeof(buf),pf1)) ehExit("LLoad ?");

	p=buf;

	if (*p=='#')
	{
		strcpy(link->FilePadre,".CHECK.");
		p++;
		strcpy(link->FileFiglio,p);
		return;
	}

	if (*p!='>')
		ehExit("HLload:Errore in Mark");

	p++;
	ptr[0]=p;

 // Trova i 6 puntatori
	for (a=1;a<7;a++)
	{
		p = strstr(p,",");

		if (p==NULL)
			ehExit("HLload:Errore in HLinkTab");

		*p=0;
		ptr[a] = p+1;
		p++;
	}

// Toglie il CR
	p=ptr[6]+strlen(ptr[6]);
	p--;

	if ((*p==13)||(*p==10))
		*p=0;

 //	Pulisce la struttura
	memset(link,0,sizeof(struct ADB_LINK_LIST));

	// File Padre
	if (strlen(ptr[0])>sizeof(link->FilePadre))
	{
		ehExit("adb_LLoad:FilePadre > riga %s",buf);
		//ehExit(serv);
	}

	strcpy(link->FilePadre,ptr[0]);

	// Field Name Padre
	p2=ptr[1];

	if (*p2=='ð') // Tratasi di campo clone
	{
		link->Clone=ON;
		p2++;
	}
	else
		link->Clone=OFF;

	if (strlen(p2)	> sizeof (link->FieldNamePadre))
		 {ehExit("adb_LLoad:FieldNamePadre > riga %s",buf);
			//ehExit(serv);
			}

	strcpy(link->FieldNamePadre,p2);

	// File Figlio

	if (strlen(ptr[2])>sizeof(link->FileFiglio))
		 {ehExit("adb_LLoad:FileFiglio > riga %s",buf);
			//ehExit(serv);
			}
	strcpy(link->FileFiglio,ptr[2]);

	// Field Name Figlio
	if (strlen(ptr[3])>sizeof(link->FieldNameFiglio))
		 {ehExit("adb_LLoad:FieldNameFiglio > in %s",buf);
			//ehExit(serv);
			}
	strcpy(link->FieldNameFiglio,ptr[3]);

	// Index Name Figlio
	if (strlen(ptr[4])>sizeof(link->IndexNameFiglio))
		 {ehExit("adb_LLoad:IndexNameFiglio > in %s",buf);
			//ehExit(serv);
			}
	strcpy(link->IndexNameFiglio,ptr[4]);

	// Opzione
	if (strlen(ptr[5])>sizeof(link->Opzione))
		 {ehExit("adb_LLoad:Opzione > in %s",buf);
			//ehExit(serv);
			}
	strcpy(link->Opzione,ptr[5]);

	// Proprietario
	if (strlen(ptr[6])>sizeof(link->Proprietario))
		 {ehExit("adb_LLoad:Proprietario > in %s",buf);
			//ehExit(serv);
			}

	strcpy(link->Proprietario,ConvertoPro (ptr[6]));

	// Calcolo CheckDigit

	//return HLcheck(link);
}
// +-----------------------------------------
//	| ADB_HLsave  Salva su disco un link      |
//	|             gerarchico                  |
//	|                                         |
//	|  file      = Nome del file              |
//	|  path      = Path figli                 |
//	|              se non indicato nella stru |
//	|              ttura di link verrà assunto|
//	|              di default questo path     |
//	|  link      = Struttura di Link che      |
//	|              definisce i nuovi link     |
//	|              da inserire                |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+

static void adb_HLsave(FILE *pf1,struct ADB_LINK_LIST *link,CHAR *mark)
{
 //CHAR serv[256];
 INT Hdl;
 CHAR *Buf;

 Hdl=memoAlloc(M_HEAP,256,"HLS");
 Buf=memoPtr(Hdl,NULL);

 if (!strcmp(mark,"#"))
 fprintf(pf1,"%s%s\n",mark,link->FilePadre);
 else
 {
	if (link->Clone) sprintf(Buf,"ð%s",link->FieldNamePadre);
					 else
					 strcpy(Buf,link->FieldNamePadre);

    fprintf(pf1,"%s%s,%s,%s,%s,%s,%s,%s\n",mark,
			link->FilePadre,
			Buf,//link->FieldNamePadre,
			link->FileFiglio,
			link->FieldNameFiglio,
			link->IndexNameFiglio,
			link->Opzione,
			ConvertoPro(link->Proprietario));
 }

 memoFree(Hdl,"HLS");
}

// +-----------------------------------------
//	| ADB_HLInsert Inserisce nella tabella    |
//	|              gerarchica i riferimenti   |
//	|              di link di un determinato  |
//	|              file                       |
//	|                                         |
//	|  file      = Nome del file              |
//	|  path      = Path figli                 |
//	|              se non indicato nella stru |
//	|              ttura di link verrà assunto|
//	|              di default questo path     |
//	|  link      = Struttura di Link che      |
//	|              definisce i nuovi link     |
//	|              da inserire                |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+

static void adb_HLinsert(CHAR *FilePadre,struct ADB_LINK *link)
{
 INT a;
 LONG lp,pt;
 struct ADB_LINK_LIST adbL;
 FILE *pf1;
// char PathFigli[MAXPATH];
// TCHAR serv[200];
 CHAR *p,*p2;//,*nome1,*nome2;
 double Check;

 CHAR file[MAXPATH];

 // Crea il nome del file padre
 strcpy(file,fileName(FilePadre));
 p=strReverseStr(file,"."); if (p!=NULL) *p=0;

 //printf("INSERT:" CRLF);

	// File Figlio
	//strcpy(PathFigli,file);
	//nome1=fileName(PathFigli);
	//*nome1=0;

 //if (adb_HLinkHdl==-1) ehExit("adb_HLinsert");
 a=adb_HLfind(file,&pt);

 // printf("  -> file=%s a=%d pt=%ld" CRLF,file,a,pt); getch();
 // ----------------------------------------------------
 // Se ci sono già record su quel file li scorre fino  !
 // ad arrivare all'ultimo                             !
 // ----------------------------------------------------

 if (a)
	{
		for (pt++;pt<adb_HLink;pt++)
		{
		 memoRead(adb_HLinkHdl,pt*sizeof(adbL),&adbL,sizeof(adbL));
		 if (strcmp(adbL.FilePadre,file)>0) break;
		}
	}

 // -------------------------------------------
 //                                           !
 // Apre il file della tabella                !
 //                                           !
 // -------------------------------------------

 pf1=fopen(HLtabella,"w"); if (!pf1&&pt) ehExit("HTabLink:Errore grave - ricostruire la tabella");

 // ----------------------------------------
 // Riscrive i dati prima dell'inserimento !
 // ----------------------------------------
 Check=0;

 for (lp=0;lp<pt;lp++)
 {
	memoRead(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));
	Check+=HLcheck(&adbL);
	adb_HLsave(pf1,&adbL,">");
 }

 // --------------------------
 // Inserisce i nuovi Links  !
 // --------------------------
 if (link!=NULL)
 {
	for (a=0;;a++)
	{
	// Fine inserimento
	if (strlen(link[a].FileFiglio)<1) break;

	memset(&adbL,0,sizeof(struct ADB_LINK_LIST));
	// File Padre
	if (strlen(file)>sizeof(adbL.FilePadre))
		 {ehExit("adb_Linsert:FilePadre > in %s",file);
			//ehExit(serv);
			}
	strcpy(adbL.FilePadre,file);

	// Nome campo Padre
	p2=link[a].FieldNamePadre;
	if (*p2=='#') {adbL.Clone=ON; p2++;} else adbL.Clone=OFF;
	if (strlen(p2)>sizeof(adbL.FieldNamePadre))
		 {ehExit("adb_Linsert:FieldNamePadre > in %s",file);
			//ehExit(serv);
			}
	strcpy(adbL.FieldNamePadre,p2);

	if (strlen(link[a].FileFiglio)>sizeof(adbL.FileFiglio))
		 {ehExit("adb_Linsert:FileFiglio > in %s",file);
			//ehExit(serv);
			}
	strcpy(adbL.FileFiglio,link[a].FileFiglio);

	// Nomca campo Figlio
	if (strlen(link[a].FieldNameFiglio)>sizeof(adbL.FieldNameFiglio))
		 {ehExit("adb_Linsert:FieldNameFiglio > in %s",file);
			//ehExit(serv);
			}
	strcpy(adbL.FieldNameFiglio,link[a].FieldNameFiglio);

	// Index Name Figlio
	if (strlen(link[a].IndexNameFiglio)>sizeof(adbL.IndexNameFiglio))
		 {ehExit("adb_Linsert:IndexNameFiglio > in %s",file);
			//ehExit(serv);
			}
	strcpy(adbL.IndexNameFiglio,link[a].IndexNameFiglio);

	// Opzione
	if (strlen(link[a].Opzione)>sizeof(adbL.Opzione))
		 {ehExit("adb_Linsert:Opzione > in %s",file);
			//ehExit(serv);
			}
	strcpy(adbL.Opzione,link[a].Opzione);

	// Proprietario
	if (strlen(link[a].Proprietario) > sizeof (adbL.Proprietario))
	{
		ehExit("adb_Linsert:Proprietario > in %s",file);
		//ehExit(serv);
	}

//	printf ("%s\n",link[a].Proprietario);

	strcpy(adbL.Proprietario,link[a].Proprietario);

	// Aggiorna il file
	Check+=HLcheck(&adbL);
	adb_HLsave(pf1,&adbL,">");
	}
 }

 // ----------------------------------------
 // Riscrive i dati in coda                !
 // ----------------------------------------

 for (lp=pt;lp<adb_HLink;lp++)
 {
	memoRead(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));
	Check+=HLcheck(&adbL);
	adb_HLsave(pf1,&adbL,">");
 }

 // Scrittura del Check
 sprintf(adbL.FilePadre,"%f",Check);
 adb_HLsave(pf1,&adbL,"#");

 // Chiude un precedente tabelle se esiste
 fclose(pf1);
 adb_HLchiudi();
}

//  +--------------------------------------------
//	| ADB_HLInsertPro Inserisce nella tabella    |
//	|                 gerarchica i riferimenti   |
//	|                 di link di un determinato  |
//	|                 file                       |
//	|                                            |
//	|     link      = Struttura di Link che      |
//	|                 definisce i nuovi link     |
//	|                 da inserire                |
//	|                                            |
//	|                        by Luca Vadora 1997 |
//	+--------------------------------------------+

static void adb_HLinsertPro (CHAR *FileFiglio,CHAR *Client)
{
	INT a;
	LONG pt;
	struct ADB_LINK_LIST adbL;
	FILE *pf1;
	CHAR *p;//,*nome1,*nome2;
	double Check;

	CHAR file[MAXPATH];

// Crea il nome del file padre
	strcpy (file,fileName (FileFiglio));

	p=strReverseStr(file,".");

	if (p!=NULL)
		*p=0;

	a = adb_HLfindFiglio (file,&pt);

//	printf ("file=%s a=%d pt=%ld\n",file,a,pt);

 // ----------------------------------------------------
 // Se ci sono già record su quel file li scorre fino  !
 // ad arrivare all'ultimo                             !
 // ----------------------------------------------------

	if (a)
	{
		for (pt=pt;pt < adb_HLink;pt++)
		{
			memoRead (adb_HLinkHdl,pt * sizeof(adbL),&adbL,sizeof(adbL));

			if (strcmp (adbL.FileFiglio,file))
				continue;

			strcpy (adbL.Proprietario,Client);

			memoWrite (adb_HLinkHdl,pt * sizeof(adbL),&adbL,sizeof(adbL));
		}
	}
	else
		return;

 // -------------------------------------------
 //                                           !
 // Apre il file della tabella                !
 //                                           !
 // -------------------------------------------

	pf1 = fopen(HLtabella,"w");
	if (!pf1&&pt) ehExit("HTabLink:Errore grave - ricostruire la tabella");
	Check=0;

 // --------------------------
 // Inserisce i nuovi Links  !
 // --------------------------
	for (a = 0;a < adb_HLink;a++)
	{
		memoRead (adb_HLinkHdl,a * sizeof(adbL),&adbL,sizeof(adbL));

 // Aggiorna il file
		Check += HLcheck (&adbL);

		adb_HLsave (pf1,&adbL,">");
	}

 // Scrittura del Check
	sprintf (adbL.FilePadre,"%f",Check);

	adb_HLsave(pf1,&adbL,"#");

 // Chiude un precedente tabelle se esiste
	fclose(pf1);

	adb_HLchiudi();
}
//  +-----------------------------------------
//	| ADB_HLCrea   Ricerca e cancella se esis |
//	|              tono Link del file Padre   |
//	|              e inserisce quelli nuovi   |
//	|                                         |
//	|  file      = Nome del file              |
//	|              se non indicato nella stru |
//	|              ttura di link verrà assunto|
//	|              di default il path file    |
//	|  link      = Struttura di Link che      |
//	|              definisce i nuovi link     |
//	|              da inserire                |
//	|                                         |
//	| ATTENZIONE : 1) Il file tabella,i files |
//	|                 padri devono risiedere  |
//	|                 nella stessa directory  |
//	|                                         |
//	|              2) I files figli possono   |
//	|              risiedere in sub directory |
//	|              generate dalla directory   |
//	|              che contiene il file padri |
//	|              Ma è meglio se stanno      |
//	|              insieme ai padri           |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+

void adb_HLcrea(CHAR *FilePadre,struct ADB_LINK *link)
{
 adb_HLapri();
 adb_HLdelete(FilePadre); // Cancella i riferimenti gerarchici con il file
 adb_HLinsert(FilePadre,link); // Inserisce i nuovi riferimenti
}

#ifndef _WIN32
static void HLmonitor(CHAR *file)
{
	static INT a=0;
	INT b;
	clip_set(61,66,229,116,"HL");

	for (b=0;b<8;b++)
	{
	scroll(61,66,229,116,UP,2,2);
	//ehSleep(10);
	}

	if (a) ico_disp(63,100,"PAPERA"); else ico_disp(63,100,"PAPERB");
	a^=1;
	dispf(78,100,0,-1,SET,"SMALL F",3,file);
	clip_pop();
	//mouse_input();
	ehSleep(10);
}
#endif

 // -------------------------
 // Opzione di AGGIUNTA     !
 // Ritorna NULL se         !
 // l'opzione non è valida  !
 //                         !
 // -------------------------

CHAR *OPZ2(CHAR *Opz)
{
	CHAR *agg;
	CHAR *p;

	if (*Opz!='+')
		return NULL;

	p=strstr(Opz,"\"");

	if (p==NULL)
		return NULL;

	p++;

	agg=p;

	for (;*p!=0;p++)
	{
		if (*p=='\"')
			break;
	}

	if (*p==0)
		goto nob;

	*p=0;

	nob:
	return agg;
}

static void ScriviUP(INT Hdl,LONG pt,LONG dato)
{
 LONG dato2;
 dato2=dato;
 memoWrite(Hdl,pt*4,&dato2,4);
}

static LONG LeggiUP(INT Hdl,LONG pt)
{
 LONG dato2;
 memoRead(Hdl,pt*4,&dato2,4);
 return dato2;
}

void Presenta(CHAR *VecchioRec)
{
	if (VecchioRec==NULL) win_open(EHWP_SCREENCENTER,131,242,129,-1,7,OFF,"Attendere  ...");
						  else
						  win_open(EHWP_SCREENCENTER,131,242,129,-1,7,OFF,"Attendere upgrade ...");
 
	box3d(8,25,54,118,1);
	box3d(59,64,231,118,2);
	boxp(13,30,49,79,15,SET);
	dispf(16,26,0,-1,SET,"SANS SERIF F",5,"H");
	dispf(16,58,11,-1,SET,"SANS SERIF F",2,"link");
	ico_disp(14,88,"NET02");
}

//  +-----------------------------------------
//	| adb_HLaction                            |
//	| Funzione di azione e controllo          |
//	| su link gerarchici                      |
//	|                                         |
//	| se *Vecchiorec == NULL test di presenza |
//	|        link per controllo cancellazione |
//	|                                         |
//	| altrimenti                              |
//	|                                         |
//	| Vecchiorec = Puntatore al vecchio record|
//	|       prima del update (già avvenuto)   |
//	|       La funzione effettua l'update     |
//	|       dei campi-clone collegati         |
//	|       se il valore del campo è cambiato |
//	|       dal precedente                    |
//	|                                         |
//	| Hdb  = Handle del ADB                   |
//	|                                         |
//	| Ritorna NULL se nonc i sono collegamenti|
//	|         ptr = Si,Collegamenti attivi    |
//	|               nome Primo magazzino      |
//	|                                         |
//	|  NB : Il LINK non funziona sui campi    |
//	|       numerici (è da fare ma non ne ho  |
//	|       voglia per ora) 19/11/96          |
//	|                             Tassistro   |
//	|                                         |
//	|  NB2: Se aspettavo te il LINK non       |
//  |       funzionerebbe ancora adesso       |
//  |                       20/01/1998        |
//  |                             Vadora      |
//  |                                         |
//	|                          by Vadora 1998 |
//	+-----------------------------------------+
static CHAR *adb_HLaction(INT Hdb,CHAR *VecchioRec)
{
	INT NumeroUpdate;
	INT Cpt;
	INT test=OFF;
	INT a;
	INT HdlFiglio;
	INT HDF;
	INT Opz;
	INT Opz2;
	INT FlagFind;
	INT Update=OFF;
	INT SizeFiglio;
	INT SizePadre;
	INT SizeMemo;
//	INT SizeRec;
	INT MemoUP=-1;
	INT FlagPresenta;
//	INT HdlNuovo;
	INT CampoPadreHdl=-1;
	INT CampoPadreHdl2=-1;
	INT CampoPadreOldHdl=-1;
	INT CampoFiglioHdl=-1;
	INT CampoFiglioHdl2=-1;
	INT Hdl;

	LONG Hrec;
	LONG pt;
	LONG cnt;
	LONG RecNum;
	LONG PrimoFiglio=0;
	LONG FineFiglio=0;
	LONG NumFigli=1;

	CHAR PathPadre[MAXPATH];
	CHAR FilePadre[MAXPATH];
	CHAR FileFiglio[MAXPATH];
	CHAR FileConf[MAXPATH];

	CHAR *p;
	CHAR *agg;
	CHAR *agg2;

	INT16 *lpInt16;
	float *ptFloat;
	LONG  *lpLong;

	double dCampoPadre;
	double dCampoPadre2;
	double dCampoPadreOld;
	double dCampoFiglio;
	double dCampoFiglio2;

//	CHAR *RecNuovo;
	CHAR *CampoPadre;
	CHAR *CampoPadre2;
	CHAR *CampoPadreOld;
	CHAR *CampoFiglio;
	CHAR *CampoFiglio2;

	CHAR *Record;

	static CHAR DescFile[30];

	void *ptP;
	struct ADB_REC *FldPadre;
	void *ptP2;
	struct ADB_REC *FldPadre2;

	void *ptF;
	struct ADB_REC *FldFiglio;
	void *ptF2;
	struct ADB_REC *FldFiglio2;

	struct ADB_LINK_LIST adbL;
	struct ADB_LINK_LIST adbL2;

	if (adb_HLinkHdl<0) ehExit("adb_HLAction ?");

	// ------------------------------
	// Trova il PATH padre          !
	// ------------------------------
	strcpy(PathPadre,HLtabella);

	p=fileName (PathPadre);
	*p=0;

	// ------------------------------
	// Trova il nome del file Padre !
	// ------------------------------
	strcpy(FilePadre,fileName(ADB_info[Hdb].lpFileName));

	p=strReverseStr(FilePadre,".");

	if (p!=NULL)
		*p=0;

	a=adb_HLfind(FilePadre,&pt);

	if (a==OFF)
		return OFF;// il File Padre non esiste

	PrimoFiglio=pt;

	Update=OFF;

//
// Carico in CampoPadreOld il campo del vecchio Record
//
	if (VecchioRec!=NULL)
		Update=ON;

	// ---------------------------------------
	//  APRE AL FINESTRA                     !
	// ---------------------------------------
	FlagPresenta=OFF;

	test=OFF; // Azzera il flag

	// ----------------------------------------
	//  CONTO I FIGLIOLI                      !
	// ----------------------------------------
	NumFigli=0;

	for (;pt<adb_HLink;pt++)
	{
		memoRead(adb_HLinkHdl,pt*sizeof(adbL),&adbL,sizeof(adbL));

		if (strcmp(FilePadre,adbL.FilePadre))
			break;

		NumFigli++;
	}

	// ----------------------------------------
	//  LOOP DI CONTROLLO SUI FIGLI COLLEGATI !
	// ----------------------------------------
	FineFiglio=PrimoFiglio+NumFigli-1;

	for (pt=PrimoFiglio;pt<=FineFiglio;pt++)
	{
		memoRead(adb_HLinkHdl,pt*sizeof(adbL),&adbL,sizeof(adbL));

		// Se non ci sono più figli break
		if (strcmp(FilePadre,adbL.FilePadre))
			ehExit("FineFigli errato");

		// Se è un campo clone e sono in test delete lo salto
		if ((!Update) && (adbL.Clone))
			continue;

		// Se non è un campo clone e sono in update link lo salto
		if (Update && (!adbL.Clone))
			continue;

		Opz=-1;

		if (!strlen(adbL.Opzione))
			Opz=0;

		// -------------------------
		// Opzione di SCAN         !
		// -------------------------
		if (!strcmp(adbL.Opzione,"SCAN"))
			Opz=1;

		// -------------------------
		// Opzione di AGGIUNTA     !
		// -------------------------
		agg=OPZ2(adbL.Opzione);

		if (agg!=NULL)
			Opz=2;

		if (Opz==-1)
			ehExit("HLaction: Opzione ?");

		if (Update)
		{
			Hdl=memoAlloc(M_HEAP,adb_recsize(Hdb),"updateNEW");
			if (Hdl<0)
				ehExit("update NEW :MEMO ?");

			Record=memoPtr(Hdl,NULL);

			// Backup del nuovo record
			memcpy (Record,adb_DynRecBuf(Hdb),adb_recsize(Hdb));
			
			// Carico il Vecchio record
			memcpy (adb_DynRecBuf(Hdb),VecchioRec,adb_recsize(Hdb));

			ptP=adb_FldInfo(Hdb,adbL.FieldNamePadre,&FldPadre);
			if (ptP==NULL)
				ehExit("HLaction:CAMPO PADRE ?");

			switch (FldPadre->tipo)
			{
				case ADB_NUME : 
					dCampoPadreOld=atof(ptP);
					break;

				case ADB_COBD :
					dCampoPadreOld=adb_GetCobolDecimal(ptP,FldPadre->RealSize,FldPadre->size,FldPadre->tipo2);
					break;

				case ADB_COBN :
					dCampoPadreOld=adb_GetCobolNumeric(ptP,FldPadre->RealSize,FldPadre->size,FldPadre->tipo2);
					break;

				case ADB_BOOL :
				case ADB_INT  :
					lpInt16=(INT16 *) ptP;
					dCampoPadreOld=(double) *lpInt16;
					break;
#ifdef _WIN32
				case ADB_INT32:
					dCampoPadreOld=(double) * (INT *) ptP;
					break;
#endif
				case ADB_FLOAT:
					ptFloat=(float *) ptP;
					dCampoPadreOld=(double) *ptFloat;
					break;

				case ADB_AINC:
					lpLong=(LONG *) ptP;
					dCampoPadreOld=(double) *lpLong;
					break;

				case ADB_ALFA:
//			case ADB_BLOB:
				case ADB_DATA:
					if (CampoPadreOldHdl!=-1)
						memoFree (CampoPadreOldHdl,"CampoPadreOld");

					SizeMemo=adb_keymaxsize (Hdb);
					SizePadre=FldPadre->RealSize;

					if (SizeMemo <= SizePadre)
						SizeMemo=(SizePadre+1);

					CampoPadreOldHdl=memoAlloc(M_HEAP,SizeMemo,"CampoPadreOld");

					if (CampoPadreOldHdl<0)
						ehExit("HLaction:out of memory");

					CampoPadreOld=memoPtr(CampoPadreOldHdl,NULL);

					memset(CampoPadreOld,0,SizeMemo);

					if (Opz==2)
						strcpy (CampoPadreOld,agg);

					strcat (CampoPadreOld,ptP);

					break;
			}

			memcpy (adb_DynRecBuf(Hdb),Record,adb_recsize(Hdb));

			memoFree(Hdl,"updateNEW");
		}

		ptP=adb_FldInfo(Hdb,adbL.FieldNamePadre,&FldPadre);
		if (ptP==NULL)
			ehExit("HLaction:CAMPO PADRE ?");

		switch (FldPadre->tipo)
		{
			case ADB_NUME : 
				dCampoPadre=atof(ptP);
				break;

			case ADB_COBD :
				dCampoPadre=adb_GetCobolDecimal(ptP,FldPadre->RealSize,FldPadre->size,FldPadre->tipo2);
				break;

			case ADB_COBN :
				dCampoPadre=adb_GetCobolNumeric(ptP,FldPadre->RealSize,FldPadre->size,FldPadre->tipo2);
				break;

			case ADB_BOOL :
			case ADB_INT  :
				lpInt16=(INT16 *) ptP;
				dCampoPadre=(double) *lpInt16;
				break;
#ifdef _WIN32
			case ADB_INT32:
				dCampoPadre=(double) * (INT *) ptP;
				break;
#endif
			case ADB_FLOAT:
				ptFloat=(float *) ptP;
				dCampoPadre=(double) *ptFloat;
				break;

			case ADB_AINC:
				lpLong=(LONG *) ptP;
				dCampoPadre=(double) *lpLong;
				break;

			case ADB_ALFA:
//			case ADB_BLOB:
			case ADB_DATA:
				if (CampoPadreHdl!=-1)
					memoFree (CampoPadreHdl,"CampoPadre");

				SizeMemo=adb_keymaxsize (Hdb);
				SizePadre=FldPadre->RealSize;

				if (SizeMemo <= SizePadre)
					SizeMemo=(SizePadre+1);

				CampoPadreHdl=memoAlloc(M_HEAP,SizeMemo,"CampoPadre");

				if (CampoPadreHdl<0)
					ehExit("HLaction:out of memory");

				CampoPadre=memoPtr(CampoPadreHdl,NULL);

				memset(CampoPadre,0,SizeMemo);

				if (Opz==2)
					strcpy (CampoPadre,agg);

				strcat (CampoPadre,ptP);

				break;
		}

		// -----------------------------------------
		// Controllo se il campo e' stato cambiato !
		// -----------------------------------------
		if (Update)
		{
			switch (FldPadre->tipo)
			{
				case ADB_NUME : 
				case ADB_COBD :
				case ADB_COBN :
				case ADB_BOOL :
				case ADB_INT  :
#ifdef _WIN32
				case ADB_INT32:
#endif
				case ADB_FLOAT:
				case ADB_AINC:
					if (dCampoPadreOld==dCampoPadre)
						continue;

					break;

				case ADB_ALFA:
//				case ADB_BLOB:
				case ADB_DATA:
					if (!strcmp (CampoPadre,CampoPadreOld))
// Aggiungere Pulizia Memoria
						continue;

					break;
			}

			NumeroUpdate=0;
		}

		if (!FlagPresenta)
		{
			Presenta (VecchioRec);
			FlagPresenta=ON;
		}

		dispf(59,25,0,2,SET,"SMALL F",3,"Scansione Dbase-gerarchico");

		if (!(pt%2))
			bar_percf(59,38,172,pt,adb_HLink,"VGASYS",0);

		// ------------------------------------------------------
		//  Aggiunge il percorso Padre al file Figlio           !
		// ------------------------------------------------------
		sprintf(FileFiglio,"%s%s",_strupr(PathPadre),_strupr(adbL.FileFiglio));

		// ------------------------------------------------------
		// Ricerca sugli Handle aperti se esiste il file Figlio !
		// ------------------------------------------------------
		HdlFiglio=-1;

		for (a=0;a<ADB_max;a++)
		{
			if (ADB_info[a].iHdlRam==-1)
				continue;

			strcpy (FileConf,ADB_info[a].lpFileName);

			p=strReverseStr(FileConf,".");

			if (p!=NULL)
				*p=0;

			if (!strcmp(_strupr(FileConf),_strupr(FileFiglio)))
			{
				HDF=OFF;
				HdlFiglio=a;
				break;
			}
		}

		// Se non è aperto lo apre
		if (HdlFiglio==-1)
		{
			HDF=ON;

			a=adb_open(FileFiglio,adbL.Proprietario,NORMAL|MEFS,&HdlFiglio);

			if (a)
			{
				ehExit("HLaction= FileFiglio ? %s Btrieve =%d",FileFiglio,a);
				//ehExit(FilePadre);
			}
		}

		// -------------------------------------------------
		// INIZIO CONTROLLO CAMPO FIGLIO-PADRE             !
		// -------------------------------------------------
		strcpy (DescFile,ADB_info[HdlFiglio].AdbHead->DescFile);

		//DAM HLmonitor(DescFile);

		Opz=-1;

		if (!strlen(adbL.Opzione))
			Opz=0;

		// -------------------------
		// Opzione di SCAN         !
		// -------------------------
		if (!strcmp(adbL.Opzione,"SCAN"))
			Opz=1;

		// -------------------------
		// Opzione di AGGIUNTA     !
		// -------------------------
		agg=OPZ2(adbL.Opzione);

		if (agg!=NULL)
			Opz=2;

		if (Opz==-1)
			ehExit("HLaction: Opzione ?");

		// ----------------------------------------------
		// Trova Grandezza Campo Figlio e alloca memoria !
		// ----------------------------------------------
		ptF=adb_FldInfo(HdlFiglio,adbL.FieldNameFiglio,&FldFiglio);

		if (ptF==NULL)
			ehExit("HLaction:CAMPO FIGLIO ?");

		// ---------------------------------------
		// SE C'E' UN INDICE COLLEGATO LO USO    !
		// ---------------------------------------
		if (adbL.IndexNameFiglio[0])
		{
			switch (FldPadre->tipo)
			{
		 		case ADB_NUME : 
				case ADB_COBD :
				case ADB_COBN :
				case ADB_BOOL :
				case ADB_INT  :
#ifdef _WIN32
				case ADB_INT32:
#endif
				case ADB_FLOAT:
				case ADB_AINC:
					if (Update)
						a=adb_ASfind(HdlFiglio,adbL.IndexNameFiglio,B_GET_GE,1,dCampoPadreOld);
					else
						a=adb_ASfind(HdlFiglio,adbL.IndexNameFiglio,B_GET_GE,1,dCampoPadre);
					break;

				case ADB_ALFA:
//				case ADB_BLOB:
				case ADB_DATA:
					if (Update)
						a=adb_ASfind(HdlFiglio,adbL.IndexNameFiglio,B_GET_GE,1,CampoPadreOld);
					else
						a=adb_ASfind(HdlFiglio,adbL.IndexNameFiglio,B_GET_GE,1,CampoPadre);
					break;
			}

			if (a)
				continue;

			FlagFind=ON;
		}
		else
		{// ------------------------
		 // se no leggo dal primo  !
		 // ------------------------
			FlagFind=OFF;

			a=adb_find(HdlFiglio,0,B_GET_FIRST,"",B_RECORD);

			if (a) continue; // Non ci sono record: esce

		}

		dispf(59,25,0,2,SET,"SMALL F",3,"- Controllo Dbase collegato -                 ");

		RecNum=adb_recnum(HdlFiglio);

		if (RecNum<1)
			continue;

		cnt=0;
		NumeroUpdate=0;
		for (;;)
		{
			switch (FldFiglio->tipo)
			{
		 		case ADB_NUME : 
					dCampoFiglio=atof(ptF);
					break;

				case ADB_COBD :
					dCampoFiglio=adb_GetCobolDecimal(ptF,FldFiglio->RealSize,FldFiglio->size,FldFiglio->tipo2);
					break;

				case ADB_COBN :
					dCampoFiglio=adb_GetCobolNumeric(ptF,FldFiglio->RealSize,FldFiglio->size,FldFiglio->tipo2);
					break;

				case ADB_BOOL :
				case ADB_INT  :
					lpInt16=(INT16 *) ptF;
					dCampoFiglio=(double) *lpInt16;
					break;
#ifdef _WIN32
				case ADB_INT32:
					dCampoFiglio=(double) * (INT *) ptF;
					break;
#endif
				case ADB_FLOAT:
					ptFloat=(float *) ptF;
					dCampoFiglio=(double) *ptFloat;
					break;

				case ADB_AINC:
					lpLong=(LONG *) ptF;
					dCampoFiglio=(double) *lpLong;
					break;

				case ADB_ALFA:
//				case ADB_BLOB:
				case ADB_DATA:
					if (CampoFiglioHdl!=-1)
						memoFree (CampoFiglioHdl,"CampoFiglio");

					SizeMemo=adb_keymaxsize (Hdb);
					SizeFiglio=FldFiglio->RealSize;

					if (SizeMemo <= SizeFiglio)
						SizeMemo=(SizeFiglio+1);

					CampoFiglioHdl=memoAlloc(M_HEAP,SizeMemo,"Campofiglio");
					if (CampoFiglioHdl<0)
						ehExit("HLaction:out of memory");

					CampoFiglio=memoPtr(CampoFiglioHdl,NULL);

					memset(CampoFiglio,0,SizeMemo);

//					if (Opz==2)
//						strcpy (CampoFiglio,agg);

					strcpy (CampoFiglio,ptF);

					break;
			}

			cnt++;
			if (!(cnt%2))
				bar_percf(59,38,172,cnt,RecNum,"VGASYS",0);

			test=OFF;
			// -------------------------
			// Confronto con la chiave !
			// -------------------------
			switch (Opz)
			{
				// Confronto di tipo normale
				case 0:
				case 2:
					switch (FldPadre->tipo)
					{
				 		case ADB_NUME : 
						case ADB_COBD :
						case ADB_COBN :
						case ADB_BOOL :
						case ADB_INT  :
#ifdef _WIN32
						case ADB_INT32:
#endif
						case ADB_FLOAT:
						case ADB_AINC:
							if (!Update)
							{
								if (dCampoPadre==dCampoFiglio)
									test=ON;
							}
							else
							{
								if (dCampoPadreOld==dCampoFiglio)
									test=ON;
							}

							break;

						case ADB_ALFA:
//						case ADB_BLOB:
						case ADB_DATA:
							if (!Update)
							{
								if (!strcmp(CampoPadre,CampoFiglio))
									test=ON;
							}
							else
							{
								if (!strcmp(CampoPadreOld,CampoFiglio))
									test=ON;
							}

							break;
					}

					break;

				// Confronto di tipo SCAN
				case 1:
					for (a=0;a<(FldFiglio->RealSize%(SizePadre-1));a++)
					{
						if (!memcmp(CampoFiglio+(a*(SizePadre-1)),CampoPadre,strlen(CampoPadre)))
						{
							test=ON;
							break;
						}
					}

					break;
			}

			if (test)
			{
				if (!Update)
					break;

				// -------------------------------------------------------------
				//                                                             !
				//         sSetup.iModalita' UPDATE CAMPO VECCHIO CON CAMPO NUOVO      !
				//                                                             !
				// -------------------------------------------------------------
				// Se sono arrivato qui è perchè il campo clone "VECCHIO "padre è
				// uguale al campo clone "VECCHIO" figlio
				// Si verificherà ora se esistono altri campi di Link
				// Prioritari (es. CODICE) che confermano che il record è
				// sicuramente in riferimento all'altro
				// Se così non fosse il programma lo prende per buono
				for (Cpt=PrimoFiglio;Cpt<=FineFiglio;Cpt++)
				{
					if (Cpt==pt)
						continue; // Se sono me vai avanti

					memoRead(adb_HLinkHdl,Cpt*sizeof(adbL2),&adbL2,sizeof(adbL2));

					// ----------------------------------------------
					// Controlla se ci sono altri campi linkati     !
					// di tipo assoluto                             !
					// ----------------------------------------------
					if (!strcmp(adbL.FileFiglio,adbL2.FileFiglio))
					{
						if (adbL2.Clone)
							continue;

						// Trovato un riferimento assoluto
						// bisogna comparare i due
						// il campo figlio nuovo è gia
						// Al campo padre bisogna applicare l'opzione
						Opz2=-1;

						if (strlen(adbL2.Opzione)==0)
							Opz2=0;

						// -------------------------
						// Opzione di SCAN         !
						// -------------------------
						if (!strcmp(adbL2.Opzione,"SCAN"))
							ehExit("HLaction 1");

						// -------------------------
						// Opzione di AGGIUNTA     !
						// -------------------------
						agg2=OPZ2(adbL2.Opzione);

						if (agg2!=NULL)
							Opz2=2;

						if (Opz2==-1)
							ehExit("HLaction2: Opzione ? %s",adbL2.Opzione);

//						win_infoarg ("Inizio Padre2");

						ptP2=adb_FldInfo(Hdb,adbL2.FieldNamePadre,&FldPadre2);
						if (ptP2==NULL)
							ehExit("HLaction:CAMPO PADRE2 ?");

						switch (FldPadre2->tipo)
						{
							case ADB_NUME : 
								dCampoPadre2=atof(ptP2);
								break;

							case ADB_COBD :
								dCampoPadre2=adb_GetCobolDecimal(ptP2,FldPadre2->RealSize,FldPadre2->size,FldPadre2->tipo2);
								break;

							case ADB_COBN :
								dCampoPadre2=adb_GetCobolNumeric(ptP2,FldPadre2->RealSize,FldPadre2->size,FldPadre2->tipo2);
								break;

							case ADB_BOOL :
							case ADB_INT  :
								lpInt16=(INT16 *) ptP2;
								dCampoPadre2=(double) *lpInt16;
								break;
#ifdef _WIN32
							case ADB_INT32:
								dCampoPadre2=(double) * (INT *) ptP2;
								break;
#endif
							case ADB_FLOAT:
								ptFloat=(float *) ptP2;
								dCampoPadre2=(double) *ptFloat;
								break;

							case ADB_AINC:
								lpLong=(LONG *) ptP2;
								dCampoPadre2=(double) *lpLong;
								break;

							case ADB_ALFA:
//							case ADB_BLOB:
							case ADB_DATA:
								if (CampoPadreHdl2!=-1)
									memoFree (CampoPadreHdl2,"CampoPadre2");

								SizeMemo=adb_keymaxsize (Hdb);
								SizePadre=FldPadre2->RealSize;

								if (SizeMemo <= SizePadre)
									SizeMemo=(SizePadre+1);

								CampoPadreHdl2=memoAlloc(M_HEAP,SizeMemo,"CampoPadre2");

								if (CampoPadreHdl2<0)
									ehExit("HLaction:out of memory");

								CampoPadre2=memoPtr(CampoPadreHdl2,NULL);

								memset(CampoPadre2,0,SizeMemo);

								if (Opz2==2)
									strcpy (CampoPadre2,agg2);

								strcat (CampoPadre2,ptP2);

								break;
						}

//						win_infoarg ("Inizio Figlio2");

						// ----------------------------------------------
						// Trova Grandezza Campo Figlio e alloca memoria !
						// ----------------------------------------------
						ptF2=adb_FldInfo(HdlFiglio,adbL2.FieldNameFiglio,&FldFiglio2);

						if (ptF2==NULL)
							ehExit("HLaction:CAMPO FIGLIO2 ?");

						switch (FldFiglio2->tipo)
						{
		 					case ADB_NUME : 
								dCampoFiglio2=atof(ptF2);
								break;

							case ADB_COBD :
								dCampoFiglio2=adb_GetCobolDecimal(ptF2,FldFiglio2->RealSize,FldFiglio2->size,FldFiglio2->tipo2);
								break;

							case ADB_COBN :
								dCampoFiglio2=adb_GetCobolNumeric(ptF2,FldFiglio2->RealSize,FldFiglio2->size,FldFiglio2->tipo2);
								break;

							case ADB_BOOL :
							case ADB_INT  :
								lpInt16=(INT16 *) ptF2;
								dCampoFiglio2=(double) *lpInt16;
								break;
#ifdef _WIN32
							case ADB_INT32:
								dCampoFiglio2=(double) * (INT *) ptF2;
								break;
#endif
							case ADB_FLOAT:
								ptFloat=(float *) ptF2;
								dCampoFiglio2=(double) *ptFloat;
								break;

							case ADB_AINC:
								lpLong=(LONG *) ptF2;
								dCampoFiglio2=(double) *lpLong;
								break;

							case ADB_ALFA:
//							case ADB_BLOB:
							case ADB_DATA:
								if (CampoFiglioHdl2!=-1)
									memoFree (CampoFiglioHdl2,"CampoFiglio");

								SizeMemo=adb_keymaxsize (HdlFiglio);
								SizeFiglio=FldFiglio2->RealSize;

								if (SizeMemo <= SizeFiglio)
									SizeMemo=(SizeFiglio+1);

								CampoFiglioHdl2=memoAlloc(M_HEAP,SizeMemo,"Campofiglio2");
								if (CampoFiglioHdl2<0)
									ehExit("HLaction:out of memory");

								CampoFiglio2=memoPtr(CampoFiglioHdl2,NULL);

								memset(CampoFiglio2,0,SizeMemo);

//								if (Opz2==2)
//									strcpy (CampoFiglio2,agg2);

								strcpy (CampoFiglio2,ptF2);

								break;
						}

//						win_infoarg ("Inizio Confronto 2");

						// A questo punto dovrai avere in -campopadre2-
						// il campo da confrontare
						// Se sono diversi, questo record è da saltare
						switch (Opz2)
						{
							// Confronto di tipo normale
							case 0:
							case 2:
								switch (FldPadre2->tipo)
								{
							 		case ADB_NUME : 
									case ADB_COBD :
									case ADB_COBN :
									case ADB_BOOL :
									case ADB_INT  :
#ifdef _WIN32
									case ADB_INT32:
#endif
									case ADB_FLOAT:
									case ADB_AINC:
										if (dCampoPadre2!=dCampoFiglio2)
											test=OFF;

										break;

									case ADB_ALFA:
//									case ADB_BLOB:
									case ADB_DATA:
										if (strcmp(CampoPadre2,CampoFiglio2))
											test=OFF;

										break;
								}

								break;

							// Confronto di tipo SCAN
							case 1:
								for (a=0;a<(FldFiglio2->RealSize%(SizePadre-1));a++)
								{
									if (memcmp(CampoFiglio2+(a*(SizePadre-1)),CampoPadre2,strlen(CampoPadre2)))
									{
										test=OFF;
										break;
									}
								}

								break;
						}

						if (test==OFF)
							break;

						if (CampoPadreHdl2!=-1)
						{
							memoFree(CampoPadreHdl2,"HLaction");
							CampoPadreHdl2=-1;
						}

						if (CampoFiglioHdl2!=-1)
						{
							memoFree(CampoFiglioHdl2,"HLaction");
							CampoFiglioHdl2=-1;
						}
/*
						if (strcmp(CampoPadre2,adbFieldPtr(HdlFiglio,adbL2.FieldNameFiglio)))
						{
							test=OFF; // Passa subito ad un altro record
							break;
						}
*/
					} // Fine del loop di ricerca

					if (test==OFF)
						break;
				}
			}

			if (test)
			{
				if (MemoUP==-1)
				{
					MemoUP=memoAlloc (RAM_AUTO,adb_recnum (HdlFiglio)*4,"MemoUP");
					if (MemoUP<0)
						ehExit ("Non c'e' memoria per MemoUP");
				}

				adb_position (HdlFiglio,&Hrec);
				ScriviUP (MemoUP,NumeroUpdate++,Hrec);

				if (!(pt%10))
					bar_percf(59,38,172,NumeroUpdate,adb_recnum (HdlFiglio),"VGASYS",0);
			}

			if (FlagFind)
			{
				if (!test)
					break;

				a=adb_Afind(HdlFiglio,adbL.IndexNameFiglio,B_GET_NEXT,"",B_RECORD);
			}
			else
				a=adb_find(HdlFiglio,0,B_GET_NEXT,"",B_RECORD);

			if (a)
				break;
		}

		if (!Update)
		{
			if (test)
				break;
			else
				continue;
		}

		// -----------------------------------------
		// SE CI SONO UPDATE LI FACCIO FISICAMENTE !
		// -----------------------------------------
		if (NumeroUpdate)
		{
			dispf(59,25,0,2,SET,"SMALL F",3,"Update fisico");

			for (Cpt=0;Cpt<NumeroUpdate;Cpt++)
			{
				boxp(59,25,300,36,2,SET);

				bar_percf(59,38,172,Cpt+1,NumeroUpdate,"VGASYS",0);

				Hrec=LeggiUP(MemoUP,Cpt);

				adb_get(HdlFiglio,Hrec,-1);

				switch (FldFiglio->tipo)
				{
					case ADB_ALFA:
					case ADB_BLOB:
						adb_FldWrite (HdlFiglio,adbL.FieldNameFiglio,adbFieldPtr (Hdb,adbL.FieldNamePadre),0);
						break;

					case ADB_DATA:
						adb_FldWrite (HdlFiglio,adbL.FieldNameFiglio,adb_data (adbFieldPtr (Hdb,adbL.FieldNamePadre)),0);
						break;

					case ADB_NUME:
					case ADB_COBN:
					case ADB_COBD:
					case ADB_AINC:
						adb_FldWrite (HdlFiglio,adbL.FieldNameFiglio,"",adb_FldNume(Hdb,adbL.FieldNamePadre));
						break;
				}

				adb_update(HdlFiglio);
			}

			memoFree(MemoUP,"MemoUP");
			MemoUP=-1;
		}

		if (CampoPadreHdl!=-1)
		{
			memoFree(CampoPadreHdl,"HLaction");
			CampoPadreHdl=-1;
		}

		if (CampoPadreOldHdl!=-1)
		{
			memoFree(CampoPadreOldHdl,"HLaction");
			CampoPadreOldHdl=-1;
		}

		if (CampoFiglioHdl!=-1)
		{
			memoFree(CampoFiglioHdl,"HLaction");
			CampoFiglioHdl=-1;
		}
	}

	if (FlagPresenta)
	{
		ehSleep(100);
		win_close();
	}

	if (Update)
		test=OFF;

	if (test)
		return DescFile;
	else
		return NULL;
}

//  +-----------------------------------------
//	| HLcart   - Funzione FTIME per HLaction  |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+
static void HLcart2(INT cmd,void *ptr)
{
 INT x=24,y=45;
 static INT a='A';
 CHAR serv[20];

 if (cmd==1) {a=(INT) ptr; a='A';cmd=0;}

 if (cmd==0)
 {
	boxp(x,y,x+22,y+28,5,SET);
	sprintf(serv,"NETLG%c",a);
	ico_disp(x,y,serv);
	a++; if (a>'P') a='A';

	if (a%2) ico_disp(18,98,"NET01");
					 else
					 ico_disp(18,98,"NET02");
 }
}

static void win_btierr(CHAR *mess,CHAR *primo)
{
 CHAR *pt;
 INT hdl;
 hdl=memoAlloc(M_HEAP,2000L,"win_btierr");
 if (hdl<0) ehExit("winbtierr");
 pt=memoPtr(hdl,NULL);

 sprintf(pt,"%s\n \nPrimo riscontro in :\n%s",mess,primo);

 FTIME_on(HLcart2,2);
 win_mess(pt,8,0);
 FTIME_off(HLcart2);

 memoFree(hdl,"winbtierr");
}

// Controlla se esiste un collegamento da padre
static INT adb_HLsonopadre(INT Hdb)
{
	CHAR *p;
	CHAR FilePadre[MAXPATH];

	LONG pt;

	if (!ADB_HL) return OFF;
	if (adb_HLinkHdl==-1) ehExit("HL:1 ?");

	strcpy(FilePadre,fileName(ADB_info[Hdb].lpFileName));

	p = strReverseStr(FilePadre,".");

	if (p != NULL)
		*p=0;

	return adb_HLfind (FilePadre,&pt);
}

//  +-----------------------------------------
//	| ADB_LockControl                         |
//	| Controllo sulla chiusura di un record   |
//	|                                         |
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+
/*
INT  ADB_lock=1;// 0 Ritorna senza segnalare l'errore
								// 1 Riprova e segnala che stai riprovando
								// 2 Riprova senza segnalare che stai riprovando
								// 3 Chiede all'operatore se devere riprovare
	*/
static void HLcart3(INT cmd,void *ptr)
{
 INT x=20,y=36;
 static INT a='A';
 CHAR serv[20];

 if (cmd==1) {a=(INT) ptr; a='A';cmd=0;}

 if (cmd==0)
 {
	boxp(x,y,x+22,y+28,5,SET);
	sprintf(serv,"NETLG%c",a);
	ico_disp(x,y,serv);
	a++; if (a>'P') a='A';
 }
}

static void segnale(void)
{
 INT a;
 sonic(100,1,10,3,100,5);
 ehSleep(200);
 for (a=0;a<3;a++)
 {
 sonic(100,1,10,3,100,8);
 }
}


static INT adb_LockControl(INT Flag,INT Hdb,INT status,CHAR *tip,INT Escape)
{
 INT TempoAttesa=100;
 static INT Tentativi=0;
 static INT FuncStatus=OFF;
 INT a,ffg=OFF;
 CHAR serv[80];
 struct ADB_HEAD *head;

	struct OBJ obj[]={
// TipObj    Nome    Sts Lck   x   y CL1 CL2 TESTO
	{O_KEYDIM,"CR"    ,OFF, ON, 56,126, 66, 24,"Riprova"},
	{O_KEYDIM,"ESC"   ,OFF, ON,274,127, 66, 24,"Abbandona"},
	{STOP}
	};

 if ((status<0)||(status>107)) win_errgrave("BTI_err:Status non gestito");

 switch (Flag)
	{
	 case ON  : //FuncStatus=OFF;
							// Ritorna senza segnalare errore
							if (ADB_lock==0) return OFF;
							if (ADB_lock==2) {ehSleep(TempoAttesa);
											  //eventGet(NULL);
											  eventGet(NULL);
											  if (key_press(ESC)) return OFF;
											  return ON;
												}

							//strcpy (serv,"Record bloccato.");
							if (!FuncStatus)
							{
							 win_open(151,95,353,164,5,3,OFF,"Locking ");
							 dispf(67,35,0,-1,SET,"SMALL F",3,"sul file");
							 box3d(59,30,339,69,0);
							 head=ADB_info[Hdb].AdbHead;
//							 disp(109,33,0,-1,ADB_info[Hdb].lpFileName);
							 //sprintf(serv,"%s (%s)",head->DescFile,ADB_info[Hdb].lpFileName);
							 a=disp(109,33,0,-1,head->DescFile);
							 dispf(109+a+2,38,0,-1,SET,"SMALL F",2,fileName(ADB_info[Hdb].lpFileName));
							 dispf(67,52,0,-1,SET,"SMALL F",3,"status :");
							 sprintf(serv,"%d",status);
							 dispf(109,50,0,-1,SET,"VGAFIX",0,serv);
							 dispf(145,52,0,-1,SET,"SMALL F",3,"tentativo");
							 dispf(59,80,0,-1,SET,"8514SYS",0,tip);
							 dispf(60,78,14,-1,SET,"8514SYS",0,tip);
							 FTIME_on(HLcart3,2);
							}

							sprintf(serv,"%d",Tentativi++);
							dispf(190,51,0,5,SET,"VGAFIX",0,serv);

							if (ADB_lock==1)
							 {
								if (!FuncStatus)
								{
								 segnale();
								 FuncStatus=ON;
								 //   win_time2(mess,0|NOWIN);
								 ehSleep(TempoAttesa);
								 return ON;
								}
								else
								{if (!(Tentativi%29)) segnale();
										else
										{
										 ehSleep(TempoAttesa);
										}
								 //eventGet(NULL);
								 eventGet(NULL);
								 if (Tentativi==1000) return OFF;
								 if (key_press(ESC)&&Escape) return OFF;
								 return ON;
								}
							 }

							if (ADB_lock==3)
							 {
								//strcat(mess,"\nRiprovo ?");
								//a=win_ask(mess);
								obj_open(obj); obj_vedi();
								segnale();
								for (;;)
								{
								 eventGetWait(NULL);
								 if (obj_press("ESCOFF")||key_press(ESC)) {ffg=OFF; break;}
								 if (obj_press("CROFF")||key_press(CR)) {ffg=ON; break;}
								}
							 }

							win_close();
							FTIME_off(HLcart3);
							return ffg;

	 case OFF  : Tentativi=OFF;
				 if (FuncStatus)
				 {
					FTIME_off(HLcart3);
					win_close();
					FuncStatus=OFF;
				}
 }
 return OFF;
}
#endif

// +-----------------------------------------
// | adb_OwnerSet Setta il proprietario ed   |
// |              un encrypting del file     |
// |                                         |
// |  file      = Hdb del file              |
// |  client    = Nome del proprietario      |
// |  mode      = Accesso consentito         |
// |              0 = NO W/R  non encrypt    |
// |              1 = ONLY R  non encrypt    |
// |              2 = NO W/R  ENCRYPT        |
// |              3 = ONLY R  ENCRYPT        |
// |                                         |
// |                           by Ferrà 1996 |
// +-----------------------------------------+
INT adb_OwnerSet(INT Hdb,CHAR *Client,INT Mode)
{
	BTI_ULONG uiDataLen;
	INT status;
	INT Hdl;
	CHAR *File;
	//CHAR file[MAXPATH];

	Hdl=memoAlloc(M_HEAP,MAXPATH,"OSet");
	File=memoPtr(Hdl,NULL);

	if (((Hdb<0)||(Hdb>=ADB_max))||
			(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("ownerset",Hdb);

	uiDataLen=strlen(Client) + 1;
	status = BTRV32( 29,
				   ADB_info[Hdb].lpPosBlock,
				   Client, // Record dove andare
				   &uiDataLen,// Lunghezza buffer
				   Client,
				   (WORD) Mode);

	if (status) BTI_err(Hdb,"OWNERSET:",status);

#ifdef HLINK
	if (ADB_HL)
	{
		CHAR *p;
		memset (File,0,MAXPATH);

		p = strstr(ADB_info[Hdb].lpFileName,"\\");

		if (strlen (p))
		{
		 p++;
		 p = strstr (p,"\\");
		 p++;
		 strncpy (File,p,strlen (p) - 4);
		}
		else
 		 strncpy (File,ADB_info[Hdb].lpFileName,strlen (ADB_info[Hdb].lpFileName) - 4);
		
		// Aggiunge il nome del proprietario nei link
		if (adb_HLinkHdl == -1) adb_HLapri();
		adb_HLinsertPro (File,Client);
	}
#endif

	memoFree(Hdl,"OSet");
	return status;
}


INT adb_OwnerClear(INT Hdb)
{
	BTI_ULONG uiDataLen;
	INT status;

	INT Hdl;
	CHAR *File;
	//CHAR file[MAXPATH];

	if (((Hdb<0)||(Hdb>=ADB_max))||
			(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("ownerclear",Hdb);

	Hdl=memoAlloc(M_HEAP,MAXPATH,"OClr");
	File=memoPtr(Hdl,NULL);

	uiDataLen=0;
	status = BTRV32( 30,
					 ADB_info[Hdb].lpPosBlock,
					 "", // Record dove andare
					 &uiDataLen,// Lunghezza buffer
					 "",
					 0);

	if (status) BTI_err(Hdb,"OWNERCLEAR:",status);

	memset (File,0,MAXPATH);

#ifdef HLINK
	if (ADB_HL)
	{
	 CHAR *p;
	 p = strstr (ADB_info[Hdb].lpFileName,"\\");
	 if (strlen (p))
	 {
		p++;

		p = strstr (p,"\\");
		p++;

		strncpy (File,p,strlen (p) - 4);
	 }
	 else
		strncpy (File,ADB_info[Hdb].lpFileName,strlen (ADB_info[Hdb].lpFileName) - 4);

// Aggiunge il nome del proprietario nei link

		if (adb_HLinkHdl == -1) adb_HLapri();
		adb_HLinsertPro (File,"");
	}
#endif

	memoFree(Hdl,"OClr");
	return status;
}

CHAR *ConvertoPro (CHAR *Proprietario)
{
	INT a;
	INT Tabella[]={175,158,196,135,169,187,199,185,142,177,121,190,129,157,138,151,185,169,153,190,178,195,158,124,188,132,149,192,171,183};

	static CHAR Ritorno[31];

	for (a = 0;a < (INT) strlen (Proprietario);a++)
		Ritorno[a] = Proprietario[a] ^ Tabella[a];

	Ritorno[a] = '\0';

	return Ritorno;
}


// -------------------------------------------
// BLOB Function
// Gestione del BLOB (VARCHAR)
// Stringa a lunghezza variabile
//								Ferrà A&T 2000
// -------------------------------------------

// Ritorna la dimensione del campo Blob + 1 (x Zstring)
INT adb_BlobSize(HDB Hdb,CHAR *lpField) 
{
	ADB_STRUCTBLOB *lpBlob;
	struct ADB_REC *Fld; 
	lpBlob=(ADB_STRUCTBLOB *) adb_FldInfo(Hdb,lpField,&Fld);
	if (lpBlob==NULL) ehExit("adb_BlobSize: %s ? in %s",lpField,ADB_info[Hdb].lpFileName);
	if (Fld->tipo!=ADB_BLOB) ehExit("adb_BlobCopy: %s ? in %s (NOBLOB) ",lpField,ADB_info[Hdb].lpFileName);
	//if (!lpBlob->iOfs) return 0; else 
	return (lpBlob->iLen); 
}

// -------------------------------------------------------
// Copia il BLOB letto in una Ztring puntata da lpBuffer
// Ritorna FALSE=Tutto OK
//         TRUE =Buffer insufficiente
//
BOOL adb_BlobCopy(HDB Hdb,CHAR *lpField,CHAR *lpBuffer,INT iSizeBuf) 
{
	CHAR *lpSorg;
	INT iSizeSource;
	BOOL fRet=FALSE;
	struct ADB_REC *Fld; 

	ADB_STRUCTBLOB *lpBlob;
	lpBlob=(ADB_STRUCTBLOB *) adb_FldInfo(Hdb,lpField,&Fld);
	if (lpBlob==NULL) ehExit("adb_BlobCopy: %s ? in %s",lpField,ADB_info[Hdb].lpFileName);
	if (Fld->tipo!=ADB_BLOB) ehExit("adb_BlobCopy: %s ? in %s (NOBLOB) ",lpField,ADB_info[Hdb].lpFileName);

	lpSorg=adb_DynRecBuf(Hdb);	*lpBuffer=0; if (!lpBlob->iLen) return 0;
	iSizeSource=lpBlob->iLen; 
//	if (iSizeSource>=iSizeBuf) {iSizeSource=iSizeBuf-1; fRet=TRUE;}
	if (iSizeSource>iSizeBuf) {iSizeSource=iSizeBuf; fRet=TRUE;}
	//win_infoarg("[Ofs:%d Len:%d (Iss:%d)] (SizeRecSource %d)",lpBlob->iOfs,lpBlob->iLen,iSizeSource,ADB_info[Hdb].ulRecBuffer);
	if (iSizeSource) memcpy(lpBuffer,lpSorg+lpBlob->iOfs,iSizeSource);
	//*(lpBuffer+iSizeSource)=0; // Zero di fine stringa
	//win_infoarg("Copiato [%s]",lpBuffer);
	return fRet;
}


BOOL adb_isBlob(HDB Hdb,CHAR *lpField) 
{
	ADB_STRUCTBLOB *lpBlob;
	struct ADB_REC *Fld; 
	lpBlob=(ADB_STRUCTBLOB *) adb_FldInfo(Hdb,lpField,&Fld);
	if (lpBlob==NULL) ehExit("adb_isBlob: %s ? in %s",lpField,ADB_info[Hdb].lpFileName);
	if (Fld->tipo!=ADB_BLOB) return FALSE; else return TRUE;
}

BOOL adb_isField(HDB Hdb,CHAR *lpField) 
{
	struct ADB_REC *Fld; 
	BYTE *lp;
	lp=adb_FldInfo(Hdb,lpField,&Fld);
	if (!lp) return FALSE; 
	return TRUE;
}
// ------------------------------------------------
// Alloca e ritorna una stringa BLOB
// Ritorna il Puntatore alla stringa
// RICORDARSI di liberarlo con Free
//

CHAR *adb_BlobGetAlloc(HDB Hdb,CHAR *lpField) 
{
	INT iSize;
	CHAR *lpDest;
	iSize=adb_BlobSize(Hdb,lpField);  
	
	if (iSize>0xFFFF) // Controllo di file corrotto
	{
#if !defined(_NODC)&&!defined(EH_CONSOLE)
		dispx("[%s] corrotto",lpField);
#else
		printf("[%s] corrotto",lpField);
#endif
		iSize=4;
		lpDest=ehAlloc(iSize); 
		strcpy(lpDest,"?");
		return lpDest;
	}

	if (!iSize) 
	{iSize=2;
	 lpDest=ehAlloc(iSize); *lpDest=0;
	}
	else
	{
	 lpDest=ehAlloc(iSize+1); if (!lpDest) ehExit("adb_BlobGetMalloc() ? size=%d",iSize);
	 adb_BlobCopy(Hdb,lpField,lpDest,iSize);
	 lpDest[iSize]=0;
	 //win_infoarg("Copiato [%s]",lpDest);
	}
	return lpDest;
}

void adb_BlobFree(CHAR *lpField) {ehFree(lpField);}

// ------------------------------------------------
// Ritorna il primo offset per campi dinamici
// ------------------------------------------------
static INT adb_BlobOfsDyn(HDB Hdb)
{
	struct ADB_REC *Field;
	CHAR *lp;
	INT a;
	INT iOfsNew=0;

	Field=(struct ADB_REC *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD)) ;//farPtr((CHAR *) ADB_info[Hdb].AdbHead,sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		//lp=farPtr(ADB_info[Hdb].lpRecBuffer,Field[a].pos);
		lp=ADB_info[Hdb].lpRecBuffer+Field[a].pos;
		if ((Field[a].pos+Field[a].RealSize)>iOfsNew) iOfsNew=Field[a].pos+Field[a].RealSize;
	}
	return iOfsNew;
}

static ULONG adb_BlobNewOfs(HDB Hdb,ULONG ulSize)
{
	struct ADB_REC *Field;
	ADB_STRUCTBLOB *lpBlob;
	CHAR *lp;
	INT a;
	ULONG ulPos,ulOfsNew=0;

	Field=(struct ADB_REC *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));

	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		lp=ADB_info[Hdb].lpRecBuffer+Field[a].pos;
		ulPos=Field[a].pos+(ULONG) Field[a].RealSize;
		
		if (Field[a].tipo==ADB_BLOB)
		{
			lpBlob=(ADB_STRUCTBLOB *) lp;
			if (lpBlob->iOfs) ulPos=lpBlob->iOfs+lpBlob->iLen;
			//if ((lpBlob->iOfs+lpBlob->iLen)>ulOfsNew) ulOfsNew=lpBlob->iOfs+lpBlob->iLen; 
		}
	    if (ulPos>ulOfsNew) ulOfsNew=ulPos; 
	}

// Controllo che non si sovrascriva lo spazio destinato ai campi fantasma in emulazione
#ifdef _ADBEMU
	if (ADB_info[Hdb].iEmulation==ADB_EMU)
	{
	 if (ulOfsNew<(ADB_info[Hdb].iOfsRecAddBuffer+ADB_info[Hdb].ulRecAddBuffer-1)) 
			ulOfsNew=(ADB_info[Hdb].iOfsRecAddBuffer+ADB_info[Hdb].ulRecAddBuffer-1);
	}
#endif

	if (!ulSize) return ulOfsNew;
	if ((ulOfsNew+ulSize)>ADB_info[Hdb].ulRecBuffer) adb_BufRealloc(Hdb,ulOfsNew+ulSize,TRUE,"BLOBNEW",FALSE);

	return ulOfsNew;
}

static void adb_BlobReset(HDB Hdb,CHAR *lpField)
{
	ADB_STRUCTBLOB *lpBlobDel,*lpBlob;
	struct ADB_REC *Field;
	CHAR *lp;
	INT a;
	INT iOfsLast;
	INT iOfsDyn;
	CHAR *lpDest;
	lpDest=adb_DynRecBuf(Hdb);

	lpBlobDel=(ADB_STRUCTBLOB *) adbFieldPtr(Hdb,lpField);
	if (!lpBlobDel->iOfs) return; // Non è da azzerare

	// Cerca il primo posto libero in memoria
	iOfsLast=adb_BlobNewOfs(Hdb,0);  if (!iOfsLast) ehExit("adb_insert: !iOfsLast");

	// Cerco il primo byte degli campi dinamici (offset)
	iOfsDyn=adb_BlobOfsDyn(Hdb);

	if (iOfsLast==iOfsDyn) goto FINE;// return;
	if (iOfsLast==(lpBlobDel->iOfs+lpBlobDel->iLen)) goto FINE;//return; // Era l'ultimo Blob Dinamico
//	win_infoarg("Rst:iOfsLast=%d",iOfsLast);
	if ((lpBlobDel->iOfs<0)||(lpBlobDel->iLen<0)) goto FINE;// return;
	memmove(lpDest+lpBlobDel->iOfs,lpDest+lpBlobDel->iOfs+lpBlobDel->iLen,iOfsLast-(lpBlobDel->iOfs+lpBlobDel->iLen));
	
	Field=(struct ADB_REC *) ((CHAR *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));//farPtr((CHAR *) ADB_info[Hdb].AdbHead,sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		//lp=farPtr(ADB_info[Hdb].lpRecBuffer,Field[a].pos);
		lp=ADB_info[Hdb].lpRecBuffer+Field[a].pos;
		if (Field[a].tipo==ADB_BLOB)
		{
			lpBlob=(ADB_STRUCTBLOB *) lp;
			if (lpBlob->iOfs>lpBlobDel->iOfs)  
				{
					lpBlob->iOfs-=lpBlobDel->iLen;
					/*
					win_infoarg("[%s/%d] da %d - %d",
								 Field[a].desc,
								 iOfsDyn,
								 lpBlob->iOfs,
								 lpBlob->iLen);
								 */
				}
		}
	}
FINE:
	lpBlobDel->iLen=0;
	lpBlobDel->iOfs=0;
}
/*
static void adb_BlobFldWrite(INT Hdb,CHAR *lpField,CHAR *lpBuffer)
{
	ADB_STRUCTBLOB *lpBlob;
	CHAR *lpDest;
	lpBlob=(ADB_STRUCTBLOB *) adbFieldPtr(Hdb,lpField);

	// Se il Blob esisteva già lo svuoto
	//win_infoarg("BFW:%d) [%s=%s] %d/%d",Hdb,lpField,lpBuffer,lpBlob->iLen,lpBlob->iOfs);
	if (lpBlob->iLen&&lpBlob->iOfs) adb_BlobReset(Hdb,lpField);
	if (!*lpBuffer) lpBuffer=" ";
	lpBlob->iLen=strlen(lpBuffer); if (!lpBlob->iLen) {lpBlob->iOfs=0; return;} // Stringa vuota

	lpBlob->iOfs=adb_BlobNewOfs(Hdb,lpBlob->iLen);
//	win_infoarg("BFW:Assign[%s=%s] >%d (%d)",lpField,lpBuffer,lpBlob->iOfs,adb_BlobOfsDyn(Hdb));
	
	lpDest=adb_recbuf(Hdb);
	memcpy(lpDest+lpBlob->iOfs,lpBuffer,lpBlob->iLen);
}
*/
static void adb_BlobFldWrite(INT Hdb,CHAR *lpField,CHAR *lpBuffer)
{
	ADB_STRUCTBLOB *lpBlob;
	CHAR *lpDest;
	INT iNewOfs;
	lpBlob=(ADB_STRUCTBLOB *) adbFieldPtr(Hdb,lpField);

	// Se il Blob esisteva già lo svuoto
	if (lpBlob->iLen&&lpBlob->iOfs) adb_BlobReset(Hdb,lpField);
	lpBlob->iLen=strlen(lpBuffer);  if (!lpBlob->iLen) {lpBlob->iOfs=0; return;} // Stringa vuota

	iNewOfs=adb_BlobNewOfs(Hdb,lpBlob->iLen); if (!iNewOfs) ehExit("adb_BlobFldWrite: !iNewOfs");
	
	// Al ritorno da adb_BlobNewOfs il puntatore al record potrebbe essere cambiato
	lpBlob=(ADB_STRUCTBLOB *) adbFieldPtr(Hdb,lpField);
	lpBlob->iOfs=iNewOfs; 
	lpBlob->iLen=strlen(lpBuffer);
	lpDest=adb_DynRecBuf(Hdb);
	memcpy(lpDest+lpBlob->iOfs,lpBuffer,lpBlob->iLen);
}

void adb_BlobView(HDB Hdb)
{
	ADB_STRUCTBLOB *lpBlob;
	struct ADB_REC *Field;
	CHAR *lp;
	INT a;
	INT iOfsLast;
	INT iOfsDyn;

	iOfsLast=adb_BlobNewOfs(Hdb,0); if (!iOfsLast) ehExit("adb_BlobFldWrite: !iOfsLast");
	iOfsDyn=adb_BlobOfsDyn(Hdb);
	
	Field=(struct ADB_REC *) ((CHAR *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		lp=ADB_info[Hdb].lpRecBuffer+(ULONG) Field[a].pos;
		if (Field[a].tipo==ADB_BLOB)
		{
			lpBlob=(ADB_STRUCTBLOB *) lp;
			win_infoarg("Controllo\n[%s/%d] loc:%d len:%d next:%d",Field[a].desc,iOfsDyn,lpBlob->iOfs,lpBlob->iLen,lpBlob->iOfs+lpBlob->iLen);
		}
	}
}

// ----------------------------
// FINE BLOB FUNCTIONS
// ----------------------------

#define BTI_ERR        20					/* record manager not started */
#define BTR_INTRPT     0x7B                  /* Btrieve interrupt vector */
#define BTR_OFFSET     0x33             /* Btrieve offset within segment */
#define VARIABLE_ID    0x6176   /* id for variable length records - 'va' */
#define PROTECTED_ID   0x6370            /* id for protected call - 'pc' */
#define VERSION_OFFSET 0
#define BTRVID_CODE    257

typedef unsigned char  BOOLEAN;

	extern struct DRV_COM *DRV;
//	extern void interrupt (*DRV_monitor) (void);


//static int BTIW95=OFF;




/*************************************************************************
**
**  Copyright 1982-1995 Btrieve Technologies, Inc. All Rights Reserved
**
*************************************************************************/
/*************************************************************************
   BTRAPI.C
     This module implements the Btrieve Interface for C/C++ applications
     using MS Windows, Win32s, Windows NT, OS2, DOS, Extended DOS.  An NLM
     application does not need to compile and link this module.

     You must select a target platform switch.  See the prologue inside
     'btrapi.h' for a list of platform switches.

     IMPORTANT
     ---------
     Btrieve Technologies, Inc., invites you to modify this file
     if you find it necessary for your particular situation.  However,
     we cannot provide technical support for this module if you
     do modify it.

*************************************************************************/
/*
#if !defined(BTI_WIN) && !defined(BTI_OS2) && !defined(BTI_DOS) \
&& !defined(BTI_NLM) && !defined(BTI_DOS_32R) && !defined(BTI_DOS_32P) \
&& !defined(BTI_DOS_32B) && !defined(BTI_WIN_32) && !defined(BTI_OS2_32)
#error You must define one of the following: BTI_WIN, BTI_OS2, BTI_DOS, BTI_NLM, BTI_DOS_32R, BTI_DOS_32P, BTI_DOS_32B, BTI_WIN_32, BTI_OS2_32
#endif
*/

/*


#include <btrconst.h>
#include <btrapi.h>
#include <string.h>
*/

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
   RQSHELLINIT() Function for MS Windows
***************************************************************************/
//#if defined( BTI_WIN ) || defined( BTI_WIN_32 )
BTI_API RQSHELLINIT( BTI_CHAR_PTR option )
{
   return WBRQSHELLINIT( option );
}
//#endif

/****************************************************************************
   BTRV() Function for NT, Win32s, or 32-bit OS2
***************************************************************************/
//#if defined( BTI_WIN_32 ) || defined( BTI_OS2_32 )
//#if defined( BTI_OS2_32 )
//#define BTRCALL BTRCALL32
//#endif
BTI_API BTRV32(
           BTI_WORD      operation,
           BTI_VOID_PTR  posBlock,
           BTI_VOID_PTR  dataBuffer,
           BTI_ULONG_PTR dataLength,
           BTI_VOID_PTR  keyBuffer,
           BTI_SINT      keyNumber )
{
   BTI_BYTE keyLength = MAX_KEY_SIZE;
   BTI_CHAR ckeynum   = (BTI_CHAR) keyNumber;
   BTI_SINT status;
   status = BTRCALL(operation,posBlock,dataBuffer,dataLength,keyBuffer,keyLength,ckeynum );
   return status;
}


// Vecchia a 16bit
BTI_API BTRV(
           BTI_WORD     operation,
           BTI_VOID_PTR posBlock,
           BTI_VOID_PTR dataBuffer,
           BTI_WORD_PTR dataLength,
           BTI_VOID_PTR keyBuffer,
           BTI_SINT     keyNumber )
{
   BTI_BYTE keyLength  = MAX_KEY_SIZE;
   BTI_CHAR ckeynum    = (BTI_CHAR)keyNumber;
   BTI_ULONG dataLen32 = 0;
   BTI_SINT status;

   if ( dataLength != NULL )
      dataLen32 = *dataLength;

   status = BTRCALL (
              operation,
              posBlock,
              dataBuffer,
              &dataLen32,
              keyBuffer,
              keyLength,
              ckeynum );

   if ( dataLength != NULL )
      *dataLength = (BTI_WORD)dataLen32;

   return status;
}


/****************************************************************************
   BTRVID() Function for NT, Win32s or 32-bit OS2
***************************************************************************/
//#if defined( BTI_WIN_32 ) || defined( BTI_OS2_32 )
//#if defined( BTI_OS2_32 )
//#define BTRCALLID BTRCALLID32
//#endif
BTI_API BTRVID(
           BTI_WORD       operation,
           BTI_VOID_PTR   posBlock,
           BTI_VOID_PTR   dataBuffer,
           BTI_WORD_PTR   dataLength,
           BTI_VOID_PTR   keyBuffer,
           BTI_SINT       keyNumber,
           BTI_BUFFER_PTR clientID )
{
   BTI_BYTE keyLength  = MAX_KEY_SIZE;
   BTI_CHAR ckeynum    = (BTI_CHAR)keyNumber;
   BTI_ULONG dataLen32 = 0;
   BTI_SINT status;

   if ( dataLength != NULL )
      dataLen32 = *dataLength;

   status = BTRCALLID (
              operation,
              posBlock,
              dataBuffer,
              &dataLen32,
              keyBuffer,
              keyLength,
              ckeynum,
              clientID );

   if ( dataLength != NULL )
      *dataLength = (BTI_WORD)dataLen32;

   return status;
}
//#endif


#ifdef __cplusplus
}
#endif



#ifdef _ADBEMU
/*
void adb_ODBCDisconnect(void)
{
	SQLDisconnect(sEmu.hConn);
	if (sEmu.hEnv)  {SQLFreeHandle(SQL_HANDLE_ENV,sEmu.hEnv); sEmu.hEnv=0;}
	if (sEmu.hConn) {SQLFreeHandle(SQL_HANDLE_DBC,sEmu.hConn); sEmu.hConn=0;}
}
BOOL adb_ODBCConnect(void)
{
	SQLRETURN sRet;
#if !defined(_NODC)&&!defined(EH_CONSOLE)
	win_open(EHWP_SCREENCENTER,50,300,59,-1,3,ON,"Connessione a ...");
//	sys.fFontBold=TRUE;
	dispfm(10,27,1,-1,STYLE_BOLD,"#Arial",25,sEmu.szDNSServerName);
//	sys.fFontBold=FALSE;
#endif

	// Alloca Ambiente ODBC
	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &sEmu.hEnv);
	SQLTRY(SQL_HANDLE_ENV,"SQLSetEnvAttr",sEmu.hEnv,SQLSetEnvAttr(sEmu.hEnv, SQL_ATTR_ODBC_VERSION, (void*) SQL_OV_ODBC3, 0)); 
	SQLAllocHandle(SQL_HANDLE_DBC, sEmu.hEnv, &sEmu.hConn);

	// SQL_AM_CONNECTION 
	// SQL_AM_STATEMENT
	// SQL_AM_NONE

	// --------------------------------------------------------------------
	// Mi connetto al server
	//
	sEmu.sqlLastError=SQLConnect(sEmu.hConn,				// Handle della connessione
								 sEmu.szDNSServerName,		// Nome del server
								 SQL_NTS,						// Nome del file / Driver da usare
								 sEmu.szDNSUserName,SQL_NTS,  // UserName
								 sEmu.szDNSPassword,SQL_NTS); // Password

	if (sEmu.sqlLastError!=SQL_SUCCESS&&
		sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO) 
		{
		 if (!sEmu.fNoErrorView) ODBCError("Connect"); 

#if !defined(_NODC)&&!defined(EH_CONSOLE)
		 win_close();
#endif
	     return TRUE;
		}

#if !defined(_NODC)&&!defined(EH_CONSOLE)
	win_close();
#endif	

//  Leggo ASYNC
	sRet=SQLGetInfo(sEmu.hConn,SQL_ASYNC_MODE,(SQLPOINTER) &sEmu.uiAsync,sizeof(sEmu.uiAsync),0);
	sRet=SQLGetInfo(sEmu.hConn,SQL_SCROLL_OPTIONS,(SQLPOINTER) &sEmu.uiScroll,sizeof(sEmu.uiScroll),0);
	sRet=SQLGetInfo(sEmu.hConn,SQL_DYNAMIC_CURSOR_ATTRIBUTES1,(SQLPOINTER) &sEmu.uiDynamic,sizeof(sEmu.uiDynamic),0);
	//win_infoarg("%d:%d",sRet,iDato);
	
	return FALSE;
}
*/

SQLRETURN adb_ODBCArgExecute(INT Hdl,CHAR *Mess,...)
{
	va_list Ah;
	CHAR *lpBuf;
	lpBuf=ehAlloc(5000);

	va_start(Ah,Mess);
	vsprintf(lpBuf,Mess,Ah); // Messaggio finale

	// Preparo
	sEmu.sqlLastError=SQLPrepare(ADB_info[Hdl].hStmt, lpBuf, SQL_NTS);
	if ((sEmu.sqlLastError!=SQL_SUCCESS)&&(sEmu.sqlLastError!=SQL_SUCCESS_WITH_INFO))
	{
		if (!sEmu.fNoErrorView) ODBCError("AE:Prepare"); 
		goto FINE;
	}

	// Creo il dizionario
	adb_ODBCDoDictionary(Hdl);

	// Eseguo il comando
	sEmu.sqlLastError=SQLExecute(ADB_info[Hdl].hStmt);
	if (sEmu.sqlLastError!=SQL_SUCCESS) {if (!sEmu.fNoErrorView) ODBCError("AE:Execute E");}


/*
	EhOdbc.sqlLastError=SQLSetPos(EhOdbc.hStmt,1,SQL_POSITION,SQL_LOCK_NO_CHANGE);
	if (EhOdbc.sqlLastError!=SQL_SUCCESS) 
	{
		if (!EhOdbc.fNoErrorView) ODBCError("AE:SetPos");
	}
*/
FINE:
	ehFree(lpBuf);
	va_end(Ah);
	return sEmu.sqlLastError;
}

// -----------------------------------------------------------------
// ODBC DoDictionary
// -----------------------------------------------------------------
static void adb_ODBCDoDictionary(INT Hdl)
{
	 SQLCHAR szNameColumn[255];
	 SQLSMALLINT NameLenght;
	 SQLSMALLINT DataType;
	 SQLUINTEGER ColumnSize;
	 SQLSMALLINT DecimnaDigits;
	 SQLSMALLINT Nullable;
	 CHAR *lpType;
	 INT iCount;
	 INT iMemory;
	 CHAR *lpBind;
	 EMUFIELD *lpEhFld;
	 INT i,iTypeC;
     SDWORD sdwFNLen;
	 INT iTotMemory;
	 INT iAdbType;

	 // Conto i campi
     if (ADB_info[Hdl].hdlDictionary) {memoFree(ADB_info[Hdl].hdlDictionary,"ODBC"); ADB_info[Hdl].hdlDictionary=0;}

	 iCount=0; iMemory=0;
	 for (i=1;;i++)
	 {
	  if (SQLDescribeCol(ADB_info[Hdl].hStmt, // Handle del file
					     (SQLSMALLINT) i, // Numero della colonna
					     szNameColumn,sizeof(szNameColumn),
						 &NameLenght,&DataType,
				         &ColumnSize,&DecimnaDigits,
				         &Nullable)!=SQL_SUCCESS) break;
	  iCount++;
	  iMemory+=(ColumnSize+1);
	 }

	 //win_infoarg("Count = %d",iCount);
	 ADB_info[Hdl].iFieldNum=iCount;
	 ADB_info[Hdl].iFreeBindSize=iMemory;
	 if (!iCount)
	 {
		ADB_info[Hdl].lpEhFld=NULL;
		ADB_info[Hdl].iTotMemory=0;
		ADB_info[Hdl].lpFreeBind=NULL;
		return;
	 }
	 // Alloco memoria per descrizione Campi
	 iTotMemory=(sizeof(EMUFIELD)*iCount)+iMemory;
	 ADB_info[Hdl].hdlDictionary=memoAlloc(RAM_AUTO,iTotMemory,"EHODBC");
	 if (ADB_info[Hdl].hdlDictionary<0) ehExit("ODBC:Errore in DoDictionary()");
	 lpEhFld=memoLock(ADB_info[Hdl].hdlDictionary);
	 //lpEhFld=memoPtr(ADB_info[Hdl].hdlDictionary);
	 //memset(memoPtr(ADB_info[Hdl].hdlDictionary),0,iTotMemory);
	 memset(lpEhFld,0,iTotMemory);
	 ADB_info[Hdl].lpEhFld=lpEhFld;
	 ADB_info[Hdl].iTotMemory=iTotMemory;
	 lpBind=(CHAR *) lpEhFld;//memoPtr(ADB_info[Hdl].hdlDictionary); 
	 lpBind+=(sizeof(EMUFIELD)*iCount);
	 ADB_info[Hdl].lpFreeBind=lpBind;
	
	 // Alloco e binderizzo i campi
	 for (i=0;i<iCount;i++)
	 {
	  if (SQLDescribeCol(ADB_info[Hdl].hStmt, // Handle del file
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
	   default:
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
	 }

	 //win_infoarg("[%s][%s] <-- %d --->",szNameColumn,lpType,ColumnSize);

	 memset(&lpEhFld[i],0,sizeof(EMUFIELD));
	 strcpy(lpEhFld[i].szName,szNameColumn);
	 strcpy(lpEhFld[i].szType,lpType);
     lpEhFld[i].iType=DataType;
	 lpEhFld[i].iAdbType=iAdbType;
	 lpEhFld[i].iSize=ColumnSize;
	 lpEhFld[i].lpField=lpBind;

	 //win_infoarg("adb_ODBCDoDictionary %d) [%s][%d] = %s",i,lpEhFld[i].szName,lpEhFld[i].iType,lpBind);
	 if (SQLBindCol(ADB_info[Hdl].hStmt, 
					(SQLSMALLINT) (i+1), 
					SQL_C_CHAR, 
					(PTR) lpEhFld[i].lpField, 
					(lpEhFld[i].iSize+1), 
//					(lpEhFld[i].iSize), 
					&sdwFNLen)) 
					{ehExit("ODBC: BINDErrore in [%s]",lpEhFld[i].szName);}
	 lpBind+=(ColumnSize+1);
	}
}

BOOL adb_ODBCNext(INT Hdb)
{
  SQLRETURN rc=0;
//  _d_("OK1B"); Sleep(1000);
  memset(ADB_info[Hdb].lpFreeBind,0,ADB_info[Hdb].iFreeBindSize); // Pulisco lo spazio dei record

  rc=SQLFetch(ADB_info[Hdb].hStmt);
  //_d_("%d   ",ADB_info[Hdb].iFreeBindSize); Sleep(1000);
  if ((rc==SQL_SUCCESS)&&(rc==SQL_SUCCESS_WITH_INFO)) return FALSE; else return TRUE;
}

static CHAR *adb_ODBCFldPtr(INT Hdb,CHAR *lpName)
{
  INT i;
  for (i=0;i<ADB_info[Hdb].iFieldNum;i++)
  {
	  //win_infoarg("%d) %s - [%s]",i,EhOdbc.lpEhFld[i].szName,EhOdbc.lpEhFld[i].lpField);
	  if (!strcmp(lpName,ADB_info[Hdb].lpEhFld[i].szName)) return ADB_info[Hdb].lpEhFld[i].lpField;
  }
  /*
  win_infoarg("ODBCFldPtr:Campo [%s] non trovato",lpName);
  for (i=0;i<EhOdbc.Num;i++)
  {
	  win_infoarg("%d) %s - [%s]",i,EhOdbc.lpEhFld[i].szName,EhOdbc.lpEhFld[i].lpField);
  }
  */
  ehExit("ODBCFldPtr:Campo [%d:%s] non trovato",Hdb,lpName);
  return NULL;
}

// -------------------------------------------------------
// Ritorna le informazioni sul binding del campo
// -------------------------------------------------------
EMUBIND *AdbBindInfo(INT Hdb,CHAR *lpAdbName,BOOL bError)
{
	struct ADB_REC *Field;
	INT a;
	if (((Hdb<0) || (Hdb>=ADB_max)) || (ADB_info[Hdb].iHdlRam==-1)) ehExit("FldAdbToSQL [%s][Hdb] %d",lpAdbName,Hdb);

	Field=(struct ADB_REC *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].iBindField;a++)
	{
		if (!strcmp(ADB_info[Hdb].lpEmuBind[a].szAdbName,lpAdbName))
		{
			return ADB_info[Hdb].lpEmuBind+a;
		}
	}

	if (bError)
	{
		win_infoarg("Bind: non trovato Adb Name [%d][%s] | [%s] ?",Hdb,ADB_info[Hdb].lpFileName,lpAdbName);
/*
	for (a=0;a<ADB_info[Hdb].iBindField;a++)
	{
		win_infoarg("%d [%s]",a,ADB_info[Hdb].lpEmuBind[a].szAdbName);
	}
	*/
		ehExit("Bind: non trovato Adb Name [%d][%s] | [%s] ?",Hdb,ADB_info[Hdb].lpFileName,lpAdbName);
	}
	return NULL;
}

// ------------------------------------------------------------------------
// EmuFindInfo()
// Ritorna la posizione di un determinato oggetto
//
INT EmuFindInfo(CHAR *lpObj,INT iStart,INT iEnd)
{
	INT a,iLen;
	CHAR *lpSource;

	iLen=strlen(lpObj);
	
	if (!iEnd||(iEnd>sEmu.iRows-1) ) iEnd=sEmu.iRows-1;
	
	for (a=iStart;a<=iEnd;a++)
	{
		lpSource=sEmu.lpRows[a];
		if (*lpSource==']') lpSource++;
		if (!memcmp(lpSource,lpObj,iLen)) 
		{
			return a;
		}
	}
	return -1;
}

// ------------------------------------------------------------------------
// EmuTypeCount()
// Conta il tipo di oggetto
//
INT EmuTypeCount(CHAR *lpTypeObj,INT iStart,INT iEnd)
{
	INT a,iLen,iCount=0;
	CHAR *lpSource;

	iLen=strlen(lpTypeObj);
	if (!iEnd||(iEnd>sEmu.iRows-1) ) iEnd=sEmu.iRows-1;
	
	for (a=iStart;a<=iEnd;a++)
	{
		lpSource=sEmu.lpRows[a];
		if (*lpSource==']') lpSource++;
		if (!memcmp(lpSource,lpTypeObj,iLen)) 
		{
			iCount++;
		}
	}
	return iCount;
}

// ------------------------------------------------------------------------
// EmuTypeCount()
// Conta il tipo di oggetto
//
CHAR *EmuTypeGet(CHAR *lpTypeObj,INT iStart,INT iEnd,INT iNum)
{
	INT a,iLen,iCount=0;
	CHAR *lpSource;

	iLen=strlen(lpTypeObj);
	if (!iEnd||(iEnd>sEmu.iRows-1) ) iEnd=sEmu.iRows-1;
	
	for (a=iStart;a<=iEnd;a++)
	{
		lpSource=sEmu.lpRows[a];
		if (*lpSource==']') lpSource++;
		if (!memcmp(lpSource,lpTypeObj,iLen)) 
		{
			if (iNum==iCount) return lpSource;
			iCount++;
		}
	}
	return NULL;
}

// ------------------------------------------------------------------------
// EmuGetInfo()
// Ricerca le informazioni (la stringa) di un determinato oggetto
//
CHAR *EmuGetInfo(CHAR *lpObj,INT iStart,INT iEnd)
{
	CHAR szServ[80];
	BYTE *lp;
	INT iPt;
	sprintf(szServ,"%s=",lpObj);
	iPt=EmuFindInfo(szServ,iStart,iEnd);
	if (iPt!=-1) 
	{
		lp=sEmu.lpRows[iPt]+strlen(szServ);
		if (*sEmu.lpRows[iPt]==']') lp++;
		return lp;
	}
	return NULL;
}

//EmuGetInfo("C.Library",0,0);

BOOL EmuSetInfo(CHAR *lpObj,INT iStart,INT iEnd,CHAR *lpValore)
{
	CHAR szServ[80];
	//BYTE *lp;
	INT iPt,iMemo;
	sprintf(szServ,"%s=",lpObj);
	iPt=EmuFindInfo(szServ,iStart,iEnd);
	if (iPt!=-1) 
	{
//		win_infoarg("[%s]",sEmu.lpRows[iPt]);
		if (*sEmu.lpRows[iPt]==']') ehFree(sEmu.lpRows[iPt]);

		iMemo=strlen(szServ)+strlen(lpValore)+2;
		sEmu.lpRows[iPt]=ehAlloc(iMemo);
		sprintf(sEmu.lpRows[iPt],"]%s%s",szServ,lpValore);
		//win_infoarg("Nuovo [%s]",sEmu.lpRows[iPt]);
		return TRUE;
	} else win_infoarg("EmuSetInfo(=: %s non trovato",lpObj);
	return FALSE;
}

void adb_strReplace(CHAR *lpSource)
{
	CHAR szCampo[80];
	while(TRUE)
	{
		BYTE *lpStart,*lpEnd,*lpValore;
		lpStart=strstr(lpSource,"{"); if (!lpStart) break;
		lpEnd=strstr(lpStart+1,"}"); if (lpEnd==NULL) break;
		*lpEnd=0; 
		if (strlen(lpStart+1)>(sizeof(szCampo)-1)) {*lpEnd='}'; break;}
		strcpy(szCampo,"{"); strcat(szCampo,lpStart+1); strcat(szCampo,"}");
		*lpEnd='}';

		lpValore=NULL;
		if (!strcmp(szCampo,"{Library}")) lpValore=EmuGetInfo("C.Library",0,0);
		if (!lpValore) lpValore="-?-";
		strReplace(lpSource,szCampo,lpValore);
	}
}

// --------------------------------------------------------
// Alloca in memoria il file che contiene
// le indicazioni per l'emulazione
// --------------------------------------------------------

void adb_EmuFile(CHAR *lpFile)
{
	// INT hdlEmuInfo;
	CHAR seps[]   = "\n";
	CHAR *token;
//	BYTE *lp,*p;
	BYTE *lp;
	CHAR *lpEmu;
//	INT a;
	ZeroFill(sEmu);

	if (sEmu.hdlFileEmu) ehExit("Doppia chiamata di emulazione");
	// Carico il file in memoria
	sEmu.hdlFileEmu=fileLoad(lpFile,RAM_AUTO);
	lpEmu=memoLockEx(sEmu.hdlFileEmu,"EmuFile");
	
	// A) Alloco memoria per contenere i 1000 righe puntatori
	sEmu.lpRows=ehAlloc(sizeof(CHAR *)*3000);

	// B) conto le strighe 
	token = strtok( lpEmu, seps );
	while( token != NULL )
	{
      /* While there are tokens in "string" */
	  if (sEmu.iRows>=3000) ehExit("AdbEmu: memoria insufficiente.");
	  if (*token=='C'|| // Connect
		  *token=='T'|| // Table
		  *token=='D'|| // Definition
		  *token=='F')  // Fields
	  {
		for (lp=token;*lp;lp++) {if (*lp<32) {*lp=0; break;}}
		if (*token)
		{
			sEmu.lpRows[sEmu.iRows]=token; sEmu.iRows++;
		}
	  }
      token = strtok( NULL, seps );
    }

	if (!EmuGetInfo("C.Server",0,0)) ehExit("ADBEMU: Manca il nome del server");
	strcpy(sEmu.szDNSServerName,EmuGetInfo("C.Server",0,0));
	if (!EmuGetInfo("C.User",0,0)) ehExit("ADBEMU: Manca il nome user");
	strcpy(sEmu.szDNSUserName,EmuGetInfo("C.User",0,0));
	if (!EmuGetInfo("C.Password",0,0)) ehExit("ADBEMU: Manca la password");
	strcpy(sEmu.szDNSPassword,EmuGetInfo("C.Password",0,0));

	// Cambio i Tag ... {fluttuanti}

}

void adb_EmuRetag(void)
{
	INT a;
	CHAR *p;
	for (a=0;a<sEmu.iRows;a++)
	{
		p=strstr(sEmu.lpRows[a],"{");
		if (p)
		{
			CHAR *lpSource=ehAlloc(1024);
			strcpy(lpSource,sEmu.lpRows[a]);
			adb_strReplace(lpSource);
			sEmu.lpRows[a]=ehAlloc(strlen(lpSource)+2);
			sprintf(sEmu.lpRows[a],"]%s",lpSource);
			ehFree(lpSource);
		}
	}
}

static void adb_EmuClose(void)
{
	INT a;
	for (a=0;a<sEmu.iRows;a++)
	{
		if (*sEmu.lpRows[a]==']') ehFree(sEmu.lpRows[a]);
	}
	memoFree(sEmu.hdlFileEmu,"EMU"); sEmu.hdlFileEmu=0;
}

SQLRETURN adb_ODBCCloseCursor(INT Hdb)
{
    if (ADB_info[Hdb].hStmt) SQLCloseCursor(ADB_info[Hdb].hStmt);
	return sEmu.sqlLastError;
}

EMUFIELD *adb_ODBCFldInfo(INT Hdb,CHAR *lpName)
{
  INT i;
  for (i=0;i<ADB_info[Hdb].iFieldNum;i++)
  {
	  if (!strcmp(lpName,ADB_info[Hdb].lpEhFld[i].szName)) return &ADB_info[Hdb].lpEhFld[i];
  }
  return NULL;
}

// -------------------------------------------------------
// EmuBindMaker()
// Costruisce la struttura di Binding con il dbase
//
// Se !fCreate fa solo il binding
//
static void EmuBindMaker(HDB Hdb,BOOL fCreate)
{
	struct ADB_REC *Field;
	EMUBIND *lpEmuBind;
	CHAR *p,*p2,*p3,szServ[80];
	CHAR szDef[300];
	//INT a;
	INT j;
	INT iCol;
	INT iType;
	INT iSize;
//	INT iTotField;
	INT iTotGhost;
	INT idb;
	CHAR *lpCampo;
	INT iMemoCounter=0;
	
	Field=(struct ADB_REC *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));

	if (fCreate)
	{
		if (ADB_info[Hdb].hdlBind) {ehExit("Errore"); memoFree(ADB_info[Hdb].hdlBind,"bind"); ADB_info[Hdb].hdlBind=0;}

		ADB_info[Hdb].iBindField=EmuTypeCount("F.",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
		iTotGhost=EmuTypeCount("F.@",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);

		// Se sono presenti dei campi "fantasma" calcolo la memoria "aggiuntiva" che mi serve
		if (iTotGhost)
		{
			iMemoCounter=0;
			for (j=0;j<iTotGhost;j++)
			{
				// Leggo una dichiarazione di Binding per volta
				//win_infoarg("j=%d",j);
				lpCampo=EmuTypeGet("F.@",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint,j); 
				p=strstr(lpCampo,"|"); if (!p) ehExit("Err12"); else p++;
				p2=strstr(lpCampo,"="); if (!p2) ehExit("Err14: %d:%s",Hdb,lpCampo); else *p2=0;
				iMemoCounter+=atoi(p)+1; // +1 per lo 0 finale
				*p2='=';
			}	
			//win_infoarg("Totale %d",iMemoCounter);
		
			// Salvo offset originale e rialloco memoria ADB
			ADB_info[Hdb].iOfsRecAddBuffer=ADB_info[Hdb].ulRecBuffer;
			ADB_info[Hdb].ulRecAddBuffer=iMemoCounter;
			adb_BufRealloc(Hdb,ADB_info[Hdb].iOfsRecAddBuffer+ADB_info[Hdb].ulRecAddBuffer,TRUE,"EmuBindMaker",TRUE);
		}
		
		// Array che contiene le specifiche di binding
		// A) Conto quanti campi sono presenti nella dichiarazione
//		win_infoarg("TotField [%d]",iTotField);
		if (!ADB_info[Hdb].iBindField) ehExit("iTotField=0");
		ADB_info[Hdb].hdlBind=memoAlloc(RAM_AUTO,ADB_info[Hdb].iBindField*sizeof(EMUBIND),"BIND");
		ADB_info[Hdb].lpEmuBind=memoLock(ADB_info[Hdb].hdlBind);

		// Creo struttura di Binding
		iCol=1;

		// Loop sui campi "fisici" presenti nel dbase
		//for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)

		iMemoCounter=0;
		for (j=0;j<ADB_info[Hdb].iBindField;j++)
		{
			// Leggo una dichiarazione di Binding per volta
			//win_infoarg("j=%d",j);
			lpCampo=EmuTypeGet("F.",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint,j); 
			if (lpCampo==NULL) ehExit("Orrore !=!=!"); else lpCampo+=2;
			lpEmuBind=ADB_info[Hdb].lpEmuBind+j;
			memset(lpEmuBind,0,sizeof(EMUBIND));
			lpEmuBind->iCol=iCol;
//			lpEmuBind->fActive=TRUE;
			
			// ---------------------------------------------------------
			// Cerco il campo nel dbase fisico
			//
			p2=strstr(lpCampo,"="); if (p2) *p2=0; else ehExit("Manca uguale %s",p);
			for (idb=0;idb<ADB_info[Hdb].AdbHead->Field;idb++)
			{
				if (!strcmp(Field[idb].desc,lpCampo)) break;
			}
			*p2='='; p=p2+1;

			// ---------------------------------------------------------
			// Il Campo E' PRESENTE NEL FISICO
			//
			if (idb<ADB_info[Hdb].AdbHead->Field)
			{
				strcpy(lpEmuBind->szAdbName,Field[idb].desc);
			}
			else
			// ---------------------------------------------------------
			// Il Campo E' UN EXTRA
			//
			{
				if (*lpCampo!='@')
				{
					win_infoarg("In %s il campo %s è mancante.",ADB_info[Hdb].lpFileName,lpCampo);
				}
				else
				{
					p=strstr(lpCampo,"|"); if (!p) ehExit("Err12"); else *p=0;
					p2=strstr(p+1,"="); if (!p2) ehExit("Err18:[%s]",lpCampo); else *p2=0;
					lpCampo+=1;
					//win_infoarg("Extra [%s] [%s] [%s]",lpCampo,p+1,p2+1);
					strcpy(lpEmuBind->szAdbName,lpCampo);
					iSize=atoi(p+1)+1;
					lpEmuBind->fGhost=TRUE; // E' un Campo fantasma
					lpEmuBind->iGhostOffset=ADB_info[Hdb].iOfsRecAddBuffer+iMemoCounter;
					iMemoCounter+=iSize;
					*p='|'; *p2='='; p=p2+1;
				}
			}
			
			if (p) {if (!strcmp(p,"[*]")) p=NULL;}
			if (p)
			{
					// Campo bloccato in scrittura
					strcpy(szDef,p); p=szDef;
					if (*p=='!') {lpEmuBind->fWriteLock=TRUE; p++;}
					lpEmuBind->lpAdbField=Field+idb;
					lpEmuBind->fActive=TRUE;

					//strcpy(lpEmuBind->szAdbName,Field[a].desc);
					p2=strstr(p,"|"); if (p2) *p2=0;

					// Posizione e dimensione
					p3=strstr(p,"["); 
					if (p3)
					{
						CHAR *p4;
						*p3=0;
						p3++;
						p4=strstr(p3,"]"); if (!p4) ehExit("] ? in emu (%s)",szServ);
						*p4=0;
						p4=strstr(p3,","); if (!p4) ehExit(", ? in emu (%s)",szServ);
						*p4=0; p4++;
						//win_infoarg("[%s][%s]",p3,p4);
						lpEmuBind->iOffset=atoi(p3);
						lpEmuBind->iTrunk=atoi(p4);
					}

					strcpy(lpEmuBind->szSQLName,p);
					// Prima specifica: Quotato ?
					if (p2) 
					{	
						//*p2=0;
						if (*(p2+1)=='\"') lpEmuBind->fSQLQuote=TRUE;
						p=p2+1;
					}

					// Secondo campo di specifica
					p2=strstr(p,"|"); 
					if (p2) 
					{
						*p2=0;
						p=p2+1;
						p2=strstr(p,"|"); if (p2) *p2=0;
						lpEmuBind->lpSpecialTranslate=ehAlloc(strlen(p)+1);
						// win_infoarg("%s",p);
						// Creo il campo di traduzione
						strcpy(lpEmuBind->lpSpecialTranslate,p);
						p=p2+1;
					}

					// Se è un FISICO
					if (!lpEmuBind->fGhost)
					{
					 iType=-1;
					 iSize=Field[idb].RealSize;
					 switch (Field[idb].tipo)
					 {
						case ADB_ALFA:
						case ADB_DATA:
							iType=SQL_C_CHAR;
							break;

						case ADB_BLOB:
							iSize=15000; // Dimensione massima di un BLOB
							iType=SQL_C_CHAR;
							break;

						case ADB_NUME:
							iType=SQL_C_CHAR;
							break;

						case ADB_BOOL:
						case ADB_INT:  
							iSize=32; 
							break;

						case ADB_INT32:  
						case ADB_AINC:  
							iSize=32; 
							break;

						case ADB_COBD:  
						case ADB_COBN:  
							iType=SQL_C_DOUBLE;
							break;

						default: 
							ehExit("Bind ? %d",Field[idb].tipo); 
							break;
					 }
					}
			}

			// Se attivo creo memoria di bindingi ed incremento colonna
			if (lpEmuBind->fActive)
			{
				// Creo in memoria una area di bindingo per ogni campo, indipendente da il buffer SQL
				lpEmuBind->lpODBCField=ehAlloc(iSize); if (!lpEmuBind->lpODBCField) ehExit("BIND: Non memory");
				memset(lpEmuBind->lpODBCField,0,iSize);
				lpEmuBind->iODBCType=SQL_C_CHAR;//iType;
				lpEmuBind->iODBCSize=iSize;
				iCol++;
			}
		} // j
	} // fCreate
	

	// SQL BIND
//	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	for (j=0;j<ADB_info[Hdb].iBindField;j++)
	{
		lpEmuBind=ADB_info[Hdb].lpEmuBind+j;
		if (lpEmuBind->fActive)
		{
			// Collego il campo SQL al campo in memoria
			if (SQLBindCol(ADB_info[Hdb].hStmt,
						  (SQLSMALLINT) lpEmuBind->iCol, 
						  (SQLSMALLINT) lpEmuBind->iODBCType,
						  (PTR) lpEmuBind->lpODBCField,
						  (SQLINTEGER) lpEmuBind->iODBCSize,
						  (SQLINTEGER *) &lpEmuBind->dwInd)) {ehExit("ADB/EMI: BINDErrore in [%s]",lpEmuBind->szSQLName);}
		}
	}
}

// -------------------------------------------------------
// Bind Translate
//
// NON TOCCARE NIENTE STRONZO!
// Mi serve anche per altri scopi (vedi SQLDriver32)
// Traduce i dati ODBC nei dati ADB
//

void ODBCFieldToAdb(HDB Hdb, // Handle dbase di riferimento
					EMUBIND *lpEmuBind, // Struttura di bindering
					CHAR *lpValore, // Valore da inserire
					BOOL fTrunk, // Se deve troncare se il caso il valore
					CHAR *lpBufAlternative) // Buffer in ricezione alternativo; NULL = Originale
{
	struct ADB_REC *Fld;
	CHAR *lpNewValue=lpValore;
	BYTE *lpFldPtr;

	// if (!strcmp(lpEmuBind->szAdbName,"ATTI")) win_infoarg("qui %d [%s]",Hdb,lpValore);

	if (lpEmuBind->fGhost) 
	{
		INT iSize=strlen(lpValore);
		lpFldPtr=ADB_info[Hdb].lpRecBuffer+lpEmuBind->iGhostOffset;

		if (lpBufAlternative)
		{
			lpFldPtr+=(INT) lpBufAlternative;
			lpFldPtr-=(INT) adb_DynRecBuf(Hdb);
		}

		lpNewValue=lpValore;
		if (lpEmuBind->lpSpecialTranslate)
		{
			lpNewValue=LocalSpecialTranslate(lpValore,lpEmuBind->lpSpecialTranslate,FALSE);
		}

		if (iSize>(lpEmuBind->iODBCSize-1)) iSize=(lpEmuBind->iODBCSize-1);
		memcpy(lpFldPtr,lpNewValue,iSize);
		lpFldPtr[iSize]=0;
		return;
	}

	// --------------------------------------------------------------
	// Trovo il puntatore alla Destinazione del campo
	//
	lpFldPtr=adb_FldInfo(Hdb,lpEmuBind->lpAdbField->desc,&Fld);
	if (lpFldPtr==NULL) ehExit("ODBCFieldToAdb:Field ? %d:[%s]",Hdb,lpEmuBind->lpAdbField->desc);

	// Ho un Buffer alternativo
	if (lpBufAlternative)
	{
		lpFldPtr+=(INT) lpBufAlternative;
		lpFldPtr-=(INT) adb_DynRecBuf(Hdb);
	}

	if (Fld->tipo==ADB_BLOB)
	{
		//adb_BlobFldWrite(Hdb,NomeFld,DatoAlfa);
		ehExit("ODBCFieldToAdb:Blob da fare");
		return;
	}

	if (!lpValore&&(Fld->tipo==ADB_BLOB||Fld->tipo==ADB_DATA||Fld->tipo==ADB_ALFA))
		ehExit("ODBCFieldToAdb(): %d %s Null value",Hdb,lpEmuBind->lpAdbField->desc);
		

	// Pre controllo per Special Translate
	if (lpEmuBind->lpSpecialTranslate)
	{
		lpNewValue=LocalSpecialTranslate(lpValore,lpEmuBind->lpSpecialTranslate,FALSE);
	}


//	if (!strcmp(lpEmuBind->lpAdbField->desc,"ANALISI")) {_dx_(0,40,"%d) [ %s ]",lpEmuBind->lpAdbField->tipo,lpNewValue); Sleep(400);}

	switch (lpEmuBind->lpAdbField->tipo)
	{
 		case ADB_NUME : 
		case ADB_COBD : 
		case ADB_COBN : 
			//adb_FldWrite(Hdb,lpEmuBind->lpAdbField->desc,NULL,atof(lpNewValue));
			adb_FldCopy(lpFldPtr,Hdb,lpEmuBind->lpAdbField->desc,NULL,atof(lpNewValue));
			break;

		case ADB_BOOL :
		case ADB_INT  : 
		case ADB_INT32: 
		case ADB_FLOAT: 
		case ADB_AINC:  
			//adb_FldWrite(Hdb,lpEmuBind->lpAdbField->desc,NULL,atoi(lpNewValue));
			adb_FldCopy(lpFldPtr,Hdb,lpEmuBind->lpAdbField->desc,NULL,atoi(lpNewValue));
			break;

		case ADB_BLOB :
			//adb_BlobFldWrite(Hdb,lpEmuBind->lpAdbField->desc,lpNewValue+lpEmuBind->iOffset);
			//break;

		case ADB_ALFA :

			//_dx_(0,40,"Entro [%s:%d:%d]",lpEmuBind->lpAdbField->desc,lpEmuBind->iOffset,lpEmuBind->iTrunk); Sleep(400);

			if (fTrunk&&(INT) strlen(lpNewValue)>(lpEmuBind->lpAdbField->RealSize-1))
			{
				CHAR b;
				if (lpEmuBind->iTrunk) // Non è da troncare
				{
					CHAR *p,ch;
					//INT iLen;
					ch=0;
					//iLen=strlen(lpNewValue);
					//if (lpEmuBind->iOffset>=iLen&&iLen) ehExit("Oltre l'offset [%s] %d>%d",lpEmuBind->lpAdbField->desc,lpEmuBind->iOffset,iLen);
					p=lpNewValue+lpEmuBind->iOffset;
					//if (lpEmuBind->iOffset>0) _dx_(0,20,":%d:",lpEmuBind->iOffset);
					if ((INT) strlen(p)>lpEmuBind->iTrunk) {ch=p[lpEmuBind->iTrunk]; p[lpEmuBind->iTrunk]=0;}
					adb_FldCopy(lpFldPtr,Hdb,lpEmuBind->lpAdbField->desc,p,0);

					if (ch) p[lpEmuBind->iTrunk]=ch;
				}
				else
				{
					b=lpNewValue[lpEmuBind->lpAdbField->RealSize-1];  
					lpNewValue[lpEmuBind->lpAdbField->RealSize-1]=0;
					adb_FldCopy(lpFldPtr,Hdb,lpEmuBind->lpAdbField->desc,lpNewValue,0);
					lpNewValue[lpEmuBind->lpAdbField->RealSize-1]=b;
				}
			}
			else
			{
				if (lpEmuBind->iTrunk)
				{
					CHAR *p,ch;
					//INT iLen;
					ch=0;
					//iLen=strlen(lpNewValue);
					//if (lpEmuBind->iOffset>=iLen&&iLen) ehExit("Oltre l'offset [%s] %d>%d",lpEmuBind->lpAdbField->desc,lpEmuBind->iOffset,iLen);
					p=lpNewValue+lpEmuBind->iOffset;
					//if (lpEmuBind->iOffset>0) _dx_(0,20,":%d:",lpEmuBind->iOffset);
					if ((INT) strlen(p)>lpEmuBind->iTrunk) {ch=p[lpEmuBind->iTrunk]; p[lpEmuBind->iTrunk]=0;}
					//adb_FldWrite(Hdb,lpEmuBind->lpAdbField->desc,p,0);
					adb_FldCopy(lpFldPtr,Hdb,lpEmuBind->lpAdbField->desc,p,0);

					if (ch) p[lpEmuBind->iTrunk]=ch;
				}
				else
				{
					//adb_FldWrite(Hdb,lpEmuBind->lpAdbField->desc,lpNewValue,0);
					adb_FldCopy(lpFldPtr,Hdb,lpEmuBind->lpAdbField->desc,lpNewValue,0);
				}
			}
			break;

		case ADB_DATA :
			if (atoi(lpNewValue)==0) {strcpy(lpFldPtr,""); break;}
			// Rischioso
			if (strlen(lpNewValue)!=8&&strlen(lpNewValue)!=0) ehExit("ODBCToField %d Data !=8 %s(%d)",Hdb,lpNewValue,strlen(lpNewValue)); 
//			strcpy(adbFieldPtr(Hdb,lpEmuBind->lpAdbField->desc),lpNewValue);
			strcpy(lpFldPtr,lpNewValue);
			break;
			
		default: ehExit("??**??");
	}

	if (lpEmuBind->lpSpecialTranslate)
	{
		ehFree(lpNewValue);
	}

}

static void BT_OdbcToAdb(HDB Hdb)
{
	EMUBIND *lpEmuBind;
	INT a;

	adb_recreset(Hdb);
	for (a=0;a<ADB_info[Hdb].iBindField;a++)
	{
		lpEmuBind=ADB_info[Hdb].lpEmuBind+a;

		// E' un campo di ADB classico
		if (lpEmuBind->fActive) 
		{
			ODBCFieldToAdb(Hdb,lpEmuBind,lpEmuBind->lpODBCField,FALSE,NULL);
		}

	}
}
// ---------------------------------------------------------
// Costruttore di Select
//
void EmuSelectBuilder(INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere)
{
	INT a;
	BOOL fPass;
	CHAR *p;
	strcpy(lpSQLCommand,"SELECT ");

	// --------------------------------------------------------------------
	// C1) Se preselezionati, inserisco i campi con AS traendoli dal profilo
	//	  

	// --------------------------------------------------------------------
	// C2) Tutti i campi indicati nel profilo

	if (!ADB_info[Hdb].lpEmuBind) ehExit("ADB_info[Hdb].lpEmuBind=NULL ! %d",Hdb);
	fPass=FALSE;
	for (a=0;a<ADB_info[Hdb].iBindField;a++)
	{
		//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
		if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
		if (fPass) strcat(lpSQLCommand,",");
		strcat(lpSQLCommand,ADB_info[Hdb].lpEmuBind[a].szSQLName); 
		fPass=TRUE;
	}
	strcat(lpSQLCommand," ");

	// --------------------------------------------------------------------
	// D) Indico la libreria/dbase dove prendere i dati
	//	  
	if (!EmuGetInfo("D.from",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint)) ehExit("EMU/ADB: D.from ?");
	strcat(lpSQLCommand,"FROM ");
	strcat(lpSQLCommand,EmuGetInfo("D.from",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint));
	strcat(lpSQLCommand," ");

	// --------------------------------------------------------------------
	// D) Where (Condizione
	//	  
	p=EmuGetInfo("D.join",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
	if (p) 
	{
		if (*p>'!')
		{
			 strcat(lpSQLCommand,"WHERE ");
			 strcat(lpSQLCommand,
				   EmuGetInfo("D.join",
				   ADB_info[Hdb].iEmuStartPoint,
				   ADB_info[Hdb].iEmuEndPoint));
			 
			 if (lpWhere)
			 {
				strcat(lpSQLCommand," AND ");
				strcat(lpSQLCommand,lpWhere);
			 }
		}
		else
		{
			 p=NULL;
		}
	}
	
	if (!p)
	{
		if (lpWhere)
		{
			if (*lpWhere)
			{
				strcat(lpSQLCommand,"WHERE ");
				strcat(lpSQLCommand,lpWhere);
				strcat(lpSQLCommand," ");
			}
		}
	}

}

// ---------------------------------------------------------
// Costruttore di UPDATE Multiplo
//
static void MultiUpdateBuilder(CHAR *lpFrom,INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere)
{
	CHAR ch,*p1,*p2,*p3,*p4,*p5,*p6;
	CHAR szAlias[20];
	CHAR szServ[80];
	INT a;
	BOOL fPass;

	p1=lpFrom;
	*lpSQLCommand=0;
	while (TRUE)
	{
		p2=strstr(p1,","); if (p2) *p2=0;
		for (;*p1;p1++) {if (*p1>' ') break;} // Salto eventuali spazi
		p3=strstr(p1," "); if (p3) *p3=0; else ehExit("EMU: %s AS ?",p3);
		p3++; // E' il codice AS
		sprintf(szAlias,"%s.",p3);

		// --------------------------------------------------------------------
		// Pre controllo che ci siano campi da scrivere per la tabella in corso
		//
		fPass=FALSE;
		for (a=0;a<ADB_info[Hdb].iBindField;a++)
		{
			//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
			if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
			if (ADB_info[Hdb].lpEmuBind[a].fWriteLock) continue; // Campo bloccato

			// Controllo se è il campo della tabella
			if (memcmp(ADB_info[Hdb].lpEmuBind[a].szSQLName,szAlias,strlen(szAlias))) continue;

			fPass=TRUE;
			break;
		}

		if (fPass)
		{
			// --------------------------------------------------------------------
			// Intesto l'update
			//

			strcat(lpSQLCommand,"UPDATE ");
			strcat(lpSQLCommand,p1);
			strcat(lpSQLCommand," ");
			strcat(lpSQLCommand," SET ");

			// --------------------------------------------------------------------
			// Inserisco i Campi di selezione
			//
			fPass=FALSE;
			for (a=0;a<ADB_info[Hdb].iBindField;a++)
			{
				//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
				if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
				if (ADB_info[Hdb].lpEmuBind[a].fWriteLock) continue; // Campo bloccato

				// Controllo se è il campo della tabella
				if (memcmp(ADB_info[Hdb].lpEmuBind[a].szSQLName,szAlias,strlen(szAlias))) continue;

				// Se il codice da memorizzare è uno del where non lo includo nell'update
				if (EmuHrecFieldFind(Hdb,ADB_info[Hdb].lpEmuBind[a].szAdbName)) continue;

				if (fPass) strcat(lpSQLCommand," ,");

				strcat(lpSQLCommand,ADB_info[Hdb].lpEmuBind[a].szSQLName+strlen(szAlias));
				strcat(lpSQLCommand,"=");
				strcat(lpSQLCommand,EmuSQLFormat(Hdb,ADB_info[Hdb].lpEmuBind+a,adbFieldPtr(Hdb,ADB_info[Hdb].lpEmuBind[a].szAdbName))); // DA SISTEMARE PER I PLURI SEGMENTI
				

				fPass=TRUE;
			}
			strcat(lpSQLCommand," ");

			// -----------------------------------
			// Where

			if (fPass)
			{
			 strcat(lpSQLCommand," WHERE ");
			 sprintf(szServ,"D.updjoin_%s",p3);
			 p4=EmuGetInfo(szServ,ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
			 if (!p4) ehExit("%s ?",szServ);
			 if (!memcmp(p4,"*HREC",5))
				{	// Tolgo riferimenti .
					CHAR *lpMemo;
					lpMemo=ehAlloc(EMU_SIZE_QUERY);
					strcpy(lpMemo,lpWhere);
					if (p4[5]=='-') 
					{
						while (strReplace(lpMemo,p4+6,"")) {};
					}
					//win_infoarg("[%s][%s]",p4,lpMemo);
					strcat(lpSQLCommand,lpMemo);
					ehFree(lpMemo);
				}
				else
				{	// Creo il nuovo Join
					p5=strstr(p4,"="); if (!p5) ehExit("%s ? =",szServ);
					*p5=0; 
					strcat(lpSQLCommand,p4);
					strcat(lpSQLCommand,"=");
					*p5='='; p5++;
					p6=strstr(lpWhere,p5); if (!p6) ehExit("%s >> %s ?",szServ,p5);
					p5=strstr(p6,"=");  if (!p5) ehExit("%s >> %s ?",szServ,p5);
					p5++;
					// Ricerca di stringa
					if (*p5=='\'')
					{
						fPass=FALSE;
						for (p6=p5+1;*p6;p6++)
						{
							if (*p6=='\'') {p6++; fPass=TRUE; break;}
						}
						if (!fPass) ehExit("%s ???",szServ);
						ch=*p6; *p6=0;
						strcat(lpSQLCommand,p5);
						*p6=ch;
					}
					// Ricerca numero
					else
					{
						fPass=FALSE;
						for (p6=p5+1;*p6;p6++)
						{
							//
							if (!strrchr( "0123456789.", *p6)) {p6++; fPass=TRUE; break;}
						}
						if (!fPass) ehExit("%s ??? (number)",szServ);
						ch=*p6; *p6=0;
						strcat(lpSQLCommand,p5);
						*p6=ch;
					}
				
				}
				strcat(lpSQLCommand,"\3");
			}
		}

		*(p3-1)=' ';
		if (p2) {*p2=','; p1=p2+1;} else break;
	}
}

static void EmuUpdateBuilder(INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere)
{
	INT a;
	BOOL fPass;
	CHAR *lpFrom;
	// // UPDATE Articoli SET INBREVE='',CODWRITER=NULL,CATEGORIA=NULL WHERE (CODICE=23)

	strcpy(lpSQLCommand,"UPDATE ");

	// --------------------------------------------------------------------
	// D) Indico la libreria/dbase dove prendere i dati
	//	  
	lpFrom=EmuGetInfo("D.from",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint); if (!lpFrom) ehExit("EMU/ADB: D.from ?");
	if (strstr(lpFrom,",")) {MultiUpdateBuilder(lpFrom,Hdb,lpSQLCommand,lpWhere); return;}

	strcat(lpSQLCommand,EmuGetInfo("D.from",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint));
	strcat(lpSQLCommand," SET ");

	// --------------------------------------------------------------------
	// C2) Tutti i campi indicati nel profilo

	fPass=FALSE;
	for (a=0;a<ADB_info[Hdb].iBindField;a++)
	{
		//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
		if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
		if (ADB_info[Hdb].lpEmuBind[a].fWriteLock) continue; // Campo bloccato
		// Se il codice da memorizzare è uno del where non lo includo nell'update
		//if (EmuHrecFieldFind(Hdb,ADB_info[Hdb].lpEmuBind[a].szAdbName)) continue;

		if (fPass) strcat(lpSQLCommand,",");
		strcat(lpSQLCommand,ADB_info[Hdb].lpEmuBind[a].szSQLName);
		strcat(lpSQLCommand,"=");
		strcat(lpSQLCommand,EmuSQLFormat(Hdb,ADB_info[Hdb].lpEmuBind+a,adbFieldPtr(Hdb,ADB_info[Hdb].lpEmuBind[a].szAdbName))); // DA SISTEMARE PER I PLURI SEGMENTI
		fPass=TRUE;
	}
	strcat(lpSQLCommand," ");

	// --------------------------------------------------------------------
	// D) Where (Condizione
	//	  
	strcat(lpSQLCommand,"WHERE ");
	strcat(lpSQLCommand,lpWhere);
}

// ---------------------------------------------------------
// Costruttore di UPDATE
//
static void MultiInsertBuilder(CHAR *lpFrom,INT Hdb,CHAR *lpSQLCommand)
{
	CHAR *p1,*p2,*p3;
	CHAR szAlias[20];
	INT a;
	BOOL fPass;

	p1=lpFrom;
	*lpSQLCommand=0;
	while (TRUE)
	{
		p2=strstr(p1,","); if (p2) *p2=0;
		for (;*p1;p1++) {if (*p1>' ') break;} // Salto eventuali spazi
		p3=strstr(p1," "); if (p3) *p3=0; else ehExit("EMU: %s AS ?",p3);
		p3++; // E' il codice AS
		sprintf(szAlias,"%s.",p3);

		// --------------------------------------------------------------------
		// Pre controllo che ci siano campi da scrivere per la tabella in corso
		//
		fPass=FALSE;
		for (a=0;a<ADB_info[Hdb].iBindField;a++)
		{
//			if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
			if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
			if (ADB_info[Hdb].lpEmuBind[a].fWriteLock) continue; // Campo bloccato

			// Controllo se è il campo della tabella
			if (memcmp(ADB_info[Hdb].lpEmuBind[a].szSQLName,szAlias,strlen(szAlias))) continue;

			fPass=TRUE;
			break;
		}

		if (fPass)
		{
			// --------------------------------------------------------------------
			// Intesto l'INSERT
			//

			strcat(lpSQLCommand,"INSERT INTO ");
			strcat(lpSQLCommand,p1);
			strcat(lpSQLCommand," (");

			// --------------------------------------------------------------------
			// Inserisco i Campi di selezione
			//
			fPass=FALSE;
			for (a=0;a<ADB_info[Hdb].iBindField;a++)
			{
				//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
				if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
				if (ADB_info[Hdb].lpEmuBind[a].fWriteLock) continue; // Campo bloccato

				// Controllo se è il campo della tabella
				if (memcmp(ADB_info[Hdb].lpEmuBind[a].szSQLName,szAlias,strlen(szAlias))) continue;
				if (fPass) strcat(lpSQLCommand,",");
				strcat(lpSQLCommand,ADB_info[Hdb].lpEmuBind[a].szSQLName+strlen(szAlias));
				fPass=TRUE;
			}
			strcat(lpSQLCommand,") VALUES (");

			// --------------------------------------------------------------------
			// Inserisco i Campi di selezione
			//
			fPass=FALSE;
			for (a=0;a<ADB_info[Hdb].iBindField;a++)
			{
				//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
				if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
				if (ADB_info[Hdb].lpEmuBind[a].fWriteLock) continue; // Campo bloccato

				// Controllo se è il campo della tabella
				if (memcmp(ADB_info[Hdb].lpEmuBind[a].szSQLName,szAlias,strlen(szAlias))) continue;

				if (fPass) strcat(lpSQLCommand,",");
				strcat(lpSQLCommand,EmuSQLFormat(Hdb,ADB_info[Hdb].lpEmuBind+a,adbFieldPtr(Hdb,ADB_info[Hdb].lpEmuBind[a].szAdbName))); // DA SISTEMARE PER I PLURI SEGMENTI
				fPass=TRUE;
			}
			strcat(lpSQLCommand,")");
		}

		*(p3-1)=' ';
		if (p2) {*p2=','; p1=p2+1;} else break;
		strcat(lpSQLCommand,"\3");
	}
}


// ---------------------------------------------------------
// Costruttore di INSERT
//
static void EmuInsertBuilder(INT Hdb,CHAR *lpSQLCommand)
{
	INT a;
	BOOL fPass;
	CHAR *lpFrom;

	strcpy(lpSQLCommand,"INSERT INTO ");

	// --------------------------------------------------------------------
	// D) Indico la libreria/dbase dove prendere i dati
	//	  
	lpFrom=EmuGetInfo("D.from",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
	if (!lpFrom) ehExit("EMU/ADB: D.from ?");
	if (strstr(lpFrom,",")) {MultiInsertBuilder(lpFrom,Hdb,lpSQLCommand); return;}

	strcat(lpSQLCommand,lpFrom);
	strcat(lpSQLCommand," (");

	// --------------------------------------------------------------------
	// C1) Se preselezionati, inserisco i campi con AS traendoli dal profilo
	//	  

	// --------------------------------------------------------------------
	// C2) Tutti i campi indicati nel profilo

	fPass=FALSE;
	for (a=0;a<ADB_info[Hdb].iBindField;a++)
	{
		//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
		if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
		if (ADB_info[Hdb].lpEmuBind[a].fWriteLock) continue; // Campo bloccato

		if (fPass) strcat(lpSQLCommand,",");
		strcat(lpSQLCommand,ADB_info[Hdb].lpEmuBind[a].szSQLName);
//		strcat(lpSQLCommand,"=");
//		strcat(lpSQLCommand,EmuSQLFormat(Hdb,ADB_info[Hdb].lpEmuBind+a,adbFieldPtr(Hdb,ADB_info[Hdb].lpEmuBind[a].szAdbName))); // DA SISTEMARE PER I PLURI SEGMENTI
		fPass=TRUE;
	}
	strcat(lpSQLCommand,") VALUES (");
	
	fPass=FALSE;
	for (a=0;a<ADB_info[Hdb].iBindField;a++)
	{
		//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
		if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
		
		if (fPass) strcat(lpSQLCommand,",");
		strcat(lpSQLCommand,EmuSQLFormat(Hdb,ADB_info[Hdb].lpEmuBind+a,adbFieldPtr(Hdb,ADB_info[Hdb].lpEmuBind[a].szAdbName))); // DA SISTEMARE PER I PLURI SEGMENTI
		fPass=TRUE;
	}
	strcat(lpSQLCommand,")");
}


// ---------------------------------------------------------
// Costruttore di DELETE Multiplo
//
static void MultiDeleteBuilder(CHAR *lpFrom,INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere)
{
	CHAR ch,*p1,*p2,*p3,*p4,*p5,*p6;
	CHAR szAlias[20];
	CHAR szServ[80];
	INT a;
	BOOL fPass;

	p1=lpFrom;
	*lpSQLCommand=0;
	while (TRUE)
	{
		p2=strstr(p1,","); if (p2) *p2=0;
		for (;*p1;p1++) {if (*p1>' ') break;} // Salto eventuali spazi
		p3=strstr(p1," "); if (p3) *p3=0; else ehExit("EMU: %s AS ?",p3);
		p3++; // E' il codice AS
		sprintf(szAlias,"%s.",p3);

		// --------------------------------------------------------------------
		// Pre controllo che ci siano campi da scrivere per la tabella in corso
		//
		fPass=FALSE;
		for (a=0;a<ADB_info[Hdb].iBindField;a++)
		{
			//if (!ADB_info[Hdb].lpEmuBind[a].iCol) continue;
			if (!ADB_info[Hdb].lpEmuBind[a].fActive) continue;
			if (ADB_info[Hdb].lpEmuBind[a].fWriteLock) continue; // Campo bloccato

			// Controllo se è il campo della tabella
			if (memcmp(ADB_info[Hdb].lpEmuBind[a].szSQLName,szAlias,strlen(szAlias))) continue;

			fPass=TRUE;
			break;
		}

		if (fPass)
		{
			// --------------------------------------------------------------------
			// Intesto l'update
			//

			strcat(lpSQLCommand,"DELETE FROM ");
			strcat(lpSQLCommand,p1);
			strcat(lpSQLCommand," ");

			// -----------------------------------
			// Where
			strcat(lpSQLCommand," WHERE ");
			sprintf(szServ,"D.updjoin_%s",p3);
			p4=EmuGetInfo(szServ,ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
			if (!p4) ehExit("%s ?",szServ);
			if (!memcmp(p4,"*HREC",5))
				{	// Tolgo riferimenti .
					CHAR *lpMemo;
					lpMemo=ehAlloc(EMU_SIZE_QUERY);
					strcpy(lpMemo,lpWhere);
					if (p4[5]=='-') 
					{
						while (strReplace(lpMemo,p4+6,"")) {};
					}
					//win_infoarg("[%s][%s]",p4,lpMemo);
					strcat(lpSQLCommand,lpMemo);
					ehFree(lpMemo);
				}
				else
				{	// Creo il nuovo Join
					p5=strstr(p4,"="); if (!p5) ehExit("%s ? =",szServ);
					*p5=0; 
					strcat(lpSQLCommand,p4);
					strcat(lpSQLCommand,"=");
					*p5='='; p5++;
					p6=strstr(lpWhere,p5); if (!p6) ehExit("%s >> %s ?",szServ,p5);
					p5=strstr(p6,"=");  if (!p5) ehExit("%s >> %s ?",szServ,p5);
					p5++;
					// Ricerca di stringa
					if (*p5=='\'')
					{
						fPass=FALSE;
						for (p6=p5+1;*p6;p6++)
						{
							if (*p6=='\'') {p6++; fPass=TRUE; break;}
						}
						if (!fPass) ehExit("%s ???",szServ);
						ch=*p6; *p6=0;
						strcat(lpSQLCommand,p5);
						*p6=ch;
					}
					// Ricerca numero
					else
					{
						fPass=FALSE;
						for (p6=p5+1;*p6;p6++)
						{
							//
							if (!strrchr( "0123456789.", *p6)) {p6++; fPass=TRUE; break;}
						}
						if (!fPass) ehExit("%s ??? (number)",szServ);
						ch=*p6; *p6=0;
						strcat(lpSQLCommand,p5);
						*p6=ch;
					}
				
				}
		}

		*(p3-1)=' ';
		if (p2) {*p2=','; p1=p2+1;} else break;
		strcat(lpSQLCommand,"\3");
	}
}

// ---------------------------------------------------------
// Costruttore di DELETE
//
static void EmuDeleteBuilder(INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere)
{
//	INT a;
//	BOOL fPass;
	CHAR *p;
	CHAR *lpFrom;

	strcpy(lpSQLCommand,"DELETE FROM ");

	// --------------------------------------------------------------------
	// D) Indico la libreria/dbase dove prendere i dati
	//	  
	lpFrom=EmuGetInfo("D.from",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
	if (!lpFrom) ehExit("EMU/ADB: D.from ?");
	if (strstr(lpFrom,",")) {MultiDeleteBuilder(lpFrom,Hdb,lpSQLCommand,lpWhere); return;}

	strcat(lpSQLCommand,EmuGetInfo("D.from",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint));
	strcat(lpSQLCommand," ");


	// --------------------------------------------------------------------
	// D) Where (Condizione
	//	  
	p=EmuGetInfo("D.join",ADB_info[Hdb].iEmuStartPoint,ADB_info[Hdb].iEmuEndPoint);
	if (p) 
	{
		if (*p>'!')
		{
			 strcat(lpSQLCommand,"WHERE ");
			 strcat(lpSQLCommand,
				    EmuGetInfo("D.join",
				    ADB_info[Hdb].iEmuStartPoint,
				    ADB_info[Hdb].iEmuEndPoint));
			 
			 if (lpWhere)
			 {
				strcat(lpSQLCommand," AND ");
				strcat(lpSQLCommand,lpWhere);
			 }
		}
		else
		{
			 p=NULL;
		}
	}
	
	if (!p)
	{
		if (lpWhere)
		{
			strcat(lpSQLCommand,"WHERE ");
			strcat(lpSQLCommand,lpWhere);
		}
	}
}

// ---------------------------------------------------------
// Trasforma un adb valore in un SQL Format
//

static CHAR *EmuSQLFormat(HDB Hdb,EMUBIND *lpEmuBind,void *lpAdbValore)
{
	static CHAR szServ[300];
	CHAR *ptr;
	CHAR *pd,*ps;
	CHAR *lpNewValue=lpAdbValore;
	BOOL fQuote=FALSE;
	struct ADB_REC *Fld;
	Fld=lpEmuBind->lpAdbField;

	switch (lpEmuBind->lpAdbField->tipo)
	 {
		case ADB_NUME : ptr=adb_MakeNumeric(atof(lpAdbValore),Fld->RealSize,Fld->tipo2,TRUE,lpEmuBind->lpAdbField->desc);
						 if (ptr==NULL) ehExit("EmuSqulFormat:MakeNume!=Fld.Size [%s=%.3f/%d/%d]",
												 lpEmuBind->lpAdbField->desc,atof(lpAdbValore),
												 Fld->RealSize,Fld->tipo2);
						 for (;*ptr;ptr++) {if (*ptr>32) break;}
						 strcpy(szServ,ptr); 
						 break;

		 case ADB_COBD : ptr=adb_MakeNumeric(adb_GetCobolDecimal(lpAdbValore,lpEmuBind->lpAdbField->RealSize,Fld->size,lpEmuBind->lpAdbField->tipo2),Fld->RealSize,Fld->tipo2,TRUE,lpEmuBind->lpAdbField->desc);
						 if (ptr==NULL) ehExit("EmuSqulFormat:IptSize!=Fld.Size (CobolDecimal)");
						 for (;*ptr;ptr++) {if (*ptr>32) break;}
						 strcpy(szServ,ptr); 
						 break;

		 case ADB_COBN : ptr=adb_MakeNumeric(adb_GetCobolNumeric(lpAdbValore,lpEmuBind->lpAdbField->RealSize,Fld->size,lpEmuBind->lpAdbField->tipo2),Fld->RealSize,Fld->tipo2,TRUE,lpEmuBind->lpAdbField->desc);
						 if (ptr==NULL) ehExit("EmuSqulFormat:IptSize!=Fld.Size (CobolNumeric)");
						 for (;*ptr;ptr++) {if (*ptr>32) break;}
						 strcpy(szServ,ptr); 
						 break;

		 case ADB_BOOL :
		 case ADB_INT  :  sprintf(szServ,"%d",(INT) * (INT16 *) lpAdbValore);
						  break;

		 case ADB_INT32: //return (double) * (INT *) FldPtr;
		 case ADB_AINC:  
						 sprintf(szServ,"%d",* (INT *) lpAdbValore);
						 break;
		 case ADB_ALFA :
		 case ADB_BLOB :
		 case ADB_DATA :
				//sprintf(szServ,"\'%s\'",lpAdbValore);
				fQuote=TRUE;
				pd=szServ; *pd++='\'';
				for (ps=lpAdbValore;*ps;ps++)
				{
					if (*ps=='\'') *pd++='\'';
					*pd++=*ps;
				}
				*pd++='\''; *pd=0;
				break;
		 default:
			ehExit("EmuSQLFormat:Field Non numerico %s ?[%d] [%d]",lpEmuBind->szAdbName,Hdb,lpEmuBind->lpAdbField->tipo);
			break;
		 }	

	if (lpEmuBind->lpSpecialTranslate)
	{
		//win_infoarg("Prima %s=[%s] (%s)",lpEmuBind->lpAdbField->desc,lpEmuBind->lpSpecialTranslate,szServ);
		lpNewValue=LocalSpecialTranslate(szServ,lpEmuBind->lpSpecialTranslate,TRUE);
		if (lpEmuBind->fSQLQuote&&!fQuote)
			sprintf(szServ,"'%s'",lpNewValue);
			else
			strcpy(szServ,lpNewValue);

		
		//win_infoarg("Dopo %s=[%s] (%s)",lpEmuBind->lpAdbField->desc,lpEmuBind->lpSpecialTranslate,szServ);
		ehFree(lpNewValue);
	}


	return szServ;
}

#endif


// Traduce il valore seguente le istruzione contenute in Special Translate

static CHAR *LocalSpecialTranslate(CHAR *lpValore,CHAR *lpSpecialTranslate,BOOL bForSQL)
{
	CHAR *lpBuffer=ehAlloc(1024);
	CHAR *lpSpecial;
	CHAR *lpValue;
	CHAR *p1,*p2,*p3;
	INT x=0;

	lpSpecial=ehAlloc(strlen(lpSpecialTranslate)+1);
	strcpy(lpSpecial,lpSpecialTranslate);
	lpValue=ehAlloc(strlen(lpValore)+1);
	strcpy(lpValue,lpValore);
	strTrimRight(lpValue);

	*lpBuffer=0;
	// Trasformazione di un valore in un altro
	if (!memcmp(lpSpecial,"@S",2))
	{
		strcpy(lpBuffer,lpValue);
	}

	// Trasformazione di un valore in un altro
	if (!memcmp(lpSpecial,"@T:",3))
	{
		p1=lpSpecial+3;

		// Scrivo il default nel buffer
		p2=strstr(p1,","); if (p2) *p2=0;
		if (!bForSQL) strcpy(lpBuffer,p1); // Default SQL to ADB
					  else
					  *lpBuffer=0; // Default ADB to SQL
		
		if (p2) 
		{
			BOOL fStop=FALSE;
			p1=p2+1;
			while (TRUE)
			{
				p2=strstr(p1,","); if (p2) *p2=0;
				p3=strstr(p1,"="); 
				// Trovato il valore
				if (p3) 
				{
					*p3=0; p3++;
					//_dx_(0,20+x*20,"[%s]=[%s]",lpValue,p1); x++; //Sleep(200);
					//if (bForSQL) win_infoarg("%d [%s]:[p1:%s]-[p3:%s],[%s]",bForSQL,lpSpecialTranslate,p1,p3,lpValue);
					// Default SQL to ADB
					if (!bForSQL) 
					{
						if (!strcmp(lpValue,p1)) {strcpy(lpBuffer,p3); fStop=TRUE; break;}
					}
					// Default ADB to SQL
					else
					{
						if (!strcmp(lpValue,p3)) {strcpy(lpBuffer,p1); fStop=TRUE; break;}
					}
				}
				if (fStop) break;
				if (p2) p1=p2+1; else break;
			}
		}
	}

	//_dx_(0,20,"Special Translate: %s=%s      ",lpValore,lpBuffer); Sleep(400);
	ehFree(lpValue);
	ehFree(lpSpecial);
	return lpBuffer;
}


//	by DAM (inizio)			le definizioni sono in testa a Flmadb.h

INT adb_unlock(INT tipo,			//	Tipo operazione (SINGLE,MULTIPLE,ALL)
				INT Hdb,			//	handle del dBase
				LONG position,		//	solo per tipo=MULTIPLE altrimenti 0
				INT index)			//	solo per tipo=MULTIPLE altrimenti 0
{
	BTI_ULONG uiDataLen;
	BTI_SINT status;
	BTI_SINT KeyNumber;

	HREC *posizione;

	if (((Hdb<0)||(Hdb>=ADB_max))||(ADB_info[Hdb].iHdlRam==-1)) adb_errgrave("unlock",Hdb);

	if (tipo==ADB_UNLOCK_MULTIPLE)	//	devo fare prima la Get Direct
	{
		posizione=(HREC *) ADB_info[Hdb].lpRecBuffer; *posizione=position;
		uiDataLen=ADB_info[Hdb].ulRecBuffer;
		status = BTRV32(B_GET_DIRECT,
				    ADB_info[Hdb].lpPosBlock,
				    ADB_info[Hdb].lpRecBuffer,		
				    &uiDataLen,					
				    ADB_info[Hdb].lpKeyBuffer,
				    (WORD) index);
		if (status) adb_errgrave("get/d unlock",Hdb);
	}
	
	switch (tipo)
	{
		case ADB_UNLOCK_SINGLE:	{KeyNumber= 0; break;}
		case ADB_UNLOCK_MULTIPLE:	{KeyNumber=-1; break;}
		case ADB_UNLOCK_ALL:		{KeyNumber=-2; break;}
	}

	uiDataLen=4;

	status = BTRV32(B_UNLOCK,	
				    ADB_info[Hdb].lpPosBlock,			
				    &position,			
				    &uiDataLen,							
				    NULL,									
				    KeyNumber);							

	return status;
}
//	by DAM (fine)

BOOL adb_DmiDelete(_DMI *lpDmi,HDB Hdb)
{
	INT a;
	HREC hRec;
	for (a=0;a<lpDmi->Num;a++)
	{
		DMIRead(lpDmi,a,&hRec);
		adb_get(Hdb,hRec,0);
		if (adb_delete(Hdb)) return TRUE;
	}
	return FALSE;
}


BYTE *adb_InfoTable(HDB Hdb)
{
	struct ADB_REC *Field;
	INT a;
	BYTE *lpRet=ehAlloc(120000);
	CHAR szServ[200];

	if (((Hdb<0) || (Hdb>=ADB_max)) || (ADB_info[Hdb].iHdlRam==-1)) ehExit("adb_InfoTable [Hdb] %d",Hdb);

	*lpRet=0;
	Field=(struct ADB_REC *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		sprintf(szServ,"%d\t%-20.20s\t%d\t%d" CRLF,
						a,
						Field[a].desc,
						Field[a].tipo,
						Field[a].size);
		strcat(lpRet,szServ);
	}
	return lpRet;
}

static void adb_BlobControl(HDB Hdb)
{
	struct ADB_REC *arField;
	ADB_STRUCTBLOB *psBlob;
	INT  a;
	BYTE *lp;
	arField=(struct ADB_REC *) ((BYTE *) ADB_info[Hdb].AdbHead+sizeof(struct ADB_HEAD));

	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		lp=ADB_info[Hdb].lpRecBuffer+arField[a].pos;
//		ulPos=Field[a].pos+(ULONG) Field[a].RealSize;

		if (arField[a].tipo==ADB_BLOB)
		{
			psBlob=(ADB_STRUCTBLOB *) lp;
			//win_infoarg("%s (%d): hdb:%d ",arField[a].desc,psBlob->iLen,Hdb);
			if (psBlob->iLen>ADB_MaxBlobSize) ehExit("blob oversize: %s (%d (max: %d)): hdb:%d ",arField[a].desc,psBlob->iLen,ADB_MaxBlobSize,Hdb);
			if ((psBlob->iOfs+psBlob->iLen)>(INT) ADB_info[Hdb].ulRecBuffer) ehExit("Campo overrecord: %s (%d): hdb:%d (rec:%d)",arField[a].desc,psBlob->iLen,Hdb,ADB_info[Hdb].ulRecBuffer);
		}
	}
}

//
// adb_TypeToDataType()
//
EH_DATATYPE adb_TypeToDataType(EN_FLDTYPE enType) {
	
	EH_DATATYPE enRet=_UNKNOW;

	switch (enType) {

		case ADB_ALFA: enRet=_ALFA; break;
		case ADB_NUME: enRet=_NUMBER; break;
		case ADB_DATA: enRet=_DATE; break;
		case ADB_INT: enRet=_INTEGER; break;
		case ADB_FLOAT: enRet=_NUMBER; break;
		case ADB_BOOL: enRet=_BOOL; break;
		case ADB_COBD: enRet=_NUMBER; break;
		case ADB_COBN: enRet=_NUMBER; break;
		case ADB_AINC: enRet=_ID; break;
		case ADB_BLOB: enRet=_TEXT; break;
		case ADB_INT32: enRet=_INTEGER; break;

	}

	return enRet;

}