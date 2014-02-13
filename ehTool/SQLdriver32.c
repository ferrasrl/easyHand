//   ---------------------------------------------
//   | SQLDriver32
//   | Che Dio me la mandi di nuovo buona ...
//   | Halleluia,  Halleluia ....
//   |            
//   |            
//   |                                           
//   |             by Ferrà Art & Technology 2003
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"


#ifdef _ADBEMU
  #include "/easyhand/inc/eh_odbc.h"
   extern EH_ODBC_SECTION sOdbcSection;
#endif

#ifdef _DEBUG
#define SQD_DEBUG 1
#endif

extern struct ADB_INFO *ADB_info;
extern EMUSTRU sEmu;
//#include <flm_adb.h>

// Numero massimo di scroll contemporaneamente gestiti = 5

// Numero massimo di linee per scroll = 30
//#define	 MAX_Y 30

#define  SQDMAX 10
#define ESQL_MAXEVENTLOAD 3
#define ESQL_STOP  0
#define ESQL_EXECUTE 1
#define ESQL_FETCH 2

#define WB_CLONE 0
#define WB_ORIGINAL 1
typedef struct{
	struct OBJ *ObjClient;
	HDB Hdb; 		 // Handle ADB
	SINT iIndex; // Numero del KeyPath (per compatibilità)
	SINT (*SubProcNotify)(SINT cmd,CHAR *PtDati,LONG  dato,void  *str,SINT Hdb,SINT IndexNum);
	struct WS_INFO ws;
//	BYTE  *lpAdbCloneBuffer; // Puntatore al Buffer Clonato del ADB (Clonato per non influire con il Normale uso del buffer)
	BYTE  *lpCloneBuffer; // Puntatore al Buffer Clonato del ADB (Clonato per non influire con il Normale uso del buffer)
	BYTE  *lpAdbOriginalBuffer; // Necessario per il doppio click
	SINT  iAdbBufferSize;

	// ResultSet
	//SINT   hResultSet; // Handle
	//BYTE   *lpResultSet;
//	LONG   lRowRequest;		// Righe richieste del nuovo buffer
	LONG   lRowsBuffer;		// Righe possibili nel buffer
	LONG   lRowsReady; // Righe lette
	SQLUINTEGER lRowsTotal; // Numero totali di linee presenti con questa Query
	SQLUINTEGER lRowsCurrent; // 
	SINT   hDictionary; // Handle che contiene il dizionario del ResultSet
	SINT   iFieldNum; // Numero di campi
	BOOL   fChanged; // T/F se qualcosa è cambiato
	EMUFIELD *lpEF;

	// Gestione MultiThread
	HANDLE hEventSql[ESQL_MAXEVENTLOAD];
	HANDLE hThread;
	HDB    HdbFind;
	DWORD  dwThread;
	BOOL   fInProcess; // Il di ricerca è in azione
	BOOL   fInStop;
	BOOL   fBreak; // Chiedo l'interruzione della ricerca
	BOOL   fDataReady; // T/F quando i dati sono pronti

	// SQL
	SQLHSTMT hStmtClone; // Handle Stantment Clone
	CHAR *lpQueryRequest; // Query completa per l'estrazione (SELEXT <CAMPI> FORM <TABLE> WHERE <CONDIZIONE> ORDER BY <INDICE>)
	BOOL fQueryToExecute; // T/F se la query è da eseguire

	CHAR *lpQueryActive;  // Puntatore alla Query Attiva al momento
	CHAR *lpQueryWhereAdd; // Where aggiunta <CONDIZIONE>
	CHAR *lpQueryCount; // Comando SELECT COUNT(*) per contare i record
	BOOL fCursorOpen; // T/F se aperto
	SINT iSQLOffset; 
	SINT iHdlFields;
	CHAR **lppFields;

	CRITICAL_SECTION csSqdStruct; // Accesso ad area generale
	CRITICAL_SECTION csSqdQuery; // Accesso ad area generale
	BOOL fFindLast;

} SQDEXT;

static SQDEXT SQD[SQDMAX];
static SINT  DB_find(SINT cmd,LONG info,CHAR *str,SQDEXT *Sdb);
static SINT SQD_Load(SQDEXT *Sdb);
static void SQD_Fetch(SQDEXT *lpSqd);
static void SQD_Execute(SQDEXT *lpSqd);
static void LQueryAssign(SQDEXT *lpSqd);
static SINT LocalBox(HDC hDC,SINT x1,SINT y1,SINT x2,SINT y2,LONG colPen,LONG colBg);

static void LOGWrite(CHAR *Mess,...)
{	
	FILE *ch;
	CHAR *lpFile="c:\\Eh3\\SQLDriver32.log";
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
	 fprintf(ch,"\r\n");
	}
	fclose(ch);

	va_end(Ah);
}


void LocalSqlTry(CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult) 
{
   SQLRETURN rc = iResult; 
   sEmu.sqlLastError=iResult;
   if (rc != SQL_SUCCESS) 
      { 
		 if (rc != SQL_SUCCESS_WITH_INFO) 
		 {
			// ehExit(SQLDisplay(WhoIs,hstmt,rc));
			 ehPrint("! SqlError: %s" CRLF,SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
#ifdef SQD_DEBUG
			 dispx(SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc)); 
#endif
			 LOGWrite(SQLDisplay(SQL_HANDLE_STMT,WhoIs,hstmt,rc));
			 //efx2(); efx2(); efx2(); 
			 //Sleep(2000);
		 }
      } 
}

static void LQueryAssign(SQDEXT *lpSqd)
{
	CHAR *lp;
	
	EnterCriticalSection(&lpSqd->csSqdStruct);
	
	EnterCriticalSection(&lpSqd->csSqdQuery);
	if (lpSqd->lpQueryActive) free(lpSqd->lpQueryActive);
	lpSqd->lpQueryActive=malloc(strlen(lpSqd->lpQueryRequest)+1);
	strcpy(lpSqd->lpQueryActive,lpSqd->lpQueryRequest);
	//_d_("[%s]     ",lpSqd->lpQueryActive); Sleep(2000);
	LeaveCriticalSection(&lpSqd->csSqdQuery);

	if (lpSqd->lpQueryCount) free(lpSqd->lpQueryCount);
	lpSqd->lpQueryCount=malloc(EMU_SIZE_QUERY);
	lp=strstr(lpSqd->lpQueryActive,"FROM");
	if (lp)
	{
		sprintf(lpSqd->lpQueryCount,"SELECT COUNT(*) %s",lp);
		lp=strstr(lpSqd->lpQueryCount,"ORDER"); if (lp) *lp=0;
		//win_infoarg("[%s]",lpSqd->lpQueryCount);
	}
	else
	{
		//*lpSqd->lpQueryCount;
		LeaveCriticalSection(&lpSqd->csSqdStruct);
		ehExit("Query count ?");
	}
	LeaveCriticalSection(&lpSqd->csSqdStruct);

	//win_infoarg("%s\n\n%s",lpSqd->lpQueryActive,lpSqd->lpQueryCount);
}

// Costruttore di array da tenere in memoria
/*
static void LArrayMake(SQDEXT *Sdb,SINT NumCam)
{
	CHAR Serv[80];
	if (NumCam<1) ehExit("Numcam ? [%d]",NumCam);
	sprintf(Serv,"*SQLResultSet:%02d",Sdb->Hdb);
	if (Sdb->hResultSet!=-1) memoFree(Sdb->hResultSet,Serv);
	Sdb->hResultSet=-1;
	
	Sdb->lMaxRows=NumCam;
	Sdb->lRowSize=adb_recsize(Sdb->Hdb)*NumCam;
	Sdb->hResultSet=memoAlloc(RAM_AUTO,Sdb->lRowSize,Serv);
	if (Sdb->hResultSet<0) ehExit("Sqd:x01");
	Sdb->lpResultSet=memoLock(Sdb->hResultSet);
	memset(Sdb->lpResultSet,0,(SINT) Sdb->lRowSize);
}
*/

// -----------------------------------------------------------------
// ODBC DoDictionary
// -----------------------------------------------------------------
static void ThreadResultSetFree(SQDEXT *lpSqd)
{
	SINT i;
	if (lpSqd->hDictionary<1) return;

	// Libero risorse precedentemente impegnate
	for (i=0;i<lpSqd->iFieldNum;i++)
	{
		free(lpSqd->lpEF[i].lpField);
	}
	memoFree(lpSqd->hDictionary,"ODBC"); 
	lpSqd->hDictionary=0;
}

static void ThreadResultSetAlloc(SQDEXT *lpSqd)
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
	 SINT iTotMemory;
	 SINT iAdbType;

	 // Conto i campi
	 ThreadResultSetFree(lpSqd);

	 iCount=0; iRowMemory=0;
	 for (i=1;;i++)
	 {
	  if (SQLDescribeCol(lpSqd->hStmtClone, // Handle del file
					     (SQLSMALLINT) i, // Numero della colonna
					     szNameColumn,sizeof(szNameColumn),
						 &NameLenght,&DataType,
				         &ColumnSize,&DecimnaDigits,
				         &Nullable)!=SQL_SUCCESS) break;
	  iCount++;
	 }

	 // Alloco memoria per descrizione Campi
	 lpSqd->iFieldNum=iCount;
	 //ADB_info[Hdl].iFreeBindSize=iRowMemory;
	 iTotMemory=sizeof(EMUFIELD)*iCount;
	 lpSqd->hDictionary=memoAlloc(M_HEAP,iTotMemory,"SQD:Dictionary");
	 if (lpSqd->hDictionary<0) ehExit("ODBC:Errore in LResultSetMaker()");
	 lpSqd->lpEF=memo_heap(lpSqd->hDictionary); 
	 memset(lpSqd->lpEF,0,iTotMemory);

	 // Alloco e binderizzo i campi
	 for (i=0;i<lpSqd->iFieldNum;i++)
	 {
	  if (SQLDescribeCol(lpSqd->hStmtClone, // Handle del file
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
			case SQL_DECIMAL:  lpType="DECIMAL";  iTypeC=SQL_C_CHAR; break;
			case SQL_NUMERIC:  lpType="NUMERIC";  iTypeC=SQL_C_CHAR; break;
			case SQL_SMALLINT: lpType="SMALLINT"; iTypeC=SQL_C_CHAR; break;
			case SQL_INTEGER:  lpType="INTEGER";  iTypeC=SQL_C_CHAR; break;
			case SQL_REAL:	   lpType="REAL";     iTypeC=SQL_C_CHAR; break;
			case SQL_FLOAT:	   lpType="FLOAT";    iTypeC=SQL_C_CHAR; break;
			case SQL_DOUBLE:   lpType="DOUBLE";   iTypeC=SQL_C_CHAR; break;
			case SQL_BIT:	   lpType="BIT";      iTypeC=SQL_C_CHAR; break;
	  }

	  ColumnSize++; // Aumento di uno la grandezza della colonna
	  //win_infoarg("[%s][%s] <-- %d --->",szNameColumn,lpType,ColumnSize);
	  memset(lpSqd->lpEF+i,0,sizeof(EMUFIELD));
//	  strcpy(lpSqd->lpEF[i].szName,szNameColumn);
	  strcpy(lpSqd->lpEF[i].szName,lpSqd->lppFields[i]);
	  
	  //_dx_(0,30+i*16,"[%s][%s]   ",szNameColumn,lpSqd->lpEF[i].szName); 
	  strcpy(lpSqd->lpEF[i].szType,lpType);
      lpSqd->lpEF[i].iType=DataType;
	  lpSqd->lpEF[i].iSize=ColumnSize;
	  lpSqd->lpEF[i].lpField=malloc(ColumnSize*lpSqd->lRowsBuffer);

	  //_d_("%d) [%s][%d]",i,lpSqd->lpEF[i].szName,lpSqd->lpEF[i].iSize); Sleep(1500);
	  if (SQLBindCol(lpSqd->hStmtClone, 
					(SQLSMALLINT) (i+1), 
					(SQLSMALLINT) iTypeC, 
					(PTR) lpSqd->lpEF[i].lpField, 
					lpSqd->lpEF[i].iSize, 0)) 
					{ehExit("ODBC: BINDErrore in [%s]",lpSqd->lpEF[i].szName);}
	 }
}

static CHAR *LRSColumn(SQDEXT *lpSqd,SINT iRow,CHAR *lpField)
{
	SINT i;
	if (iRow<0||iRow>=lpSqd->lRowsReady) return NULL;
	for (i=0;i<lpSqd->iFieldNum;i++)
	{
		if (!strcmp(lpSqd->lpEF[i].szName,lpField)) return lpSqd->lpEF[i].lpField+(lpSqd->lpEF[i].iSize*iRow);
	}
	return NULL;
}



static SINT LocalBox(HDC hDC,SINT x1,SINT y1,SINT x2,SINT y2,LONG colPen,LONG colBg)
{
	HPEN hpen, hpenOld;
	POINT Point;
	HBRUSH hbr,hbrOld;
//	SINT Rop;

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


static void LWDisp(SQDEXT *lpSqd,CHAR *lpMessage)
{
	HDC hdc;
	RECT rWin;
	SIZE sWin;
	HFONT hFont,hFontOld=NULL;
	RECT rText;
	SIZE sText;

	hdc=GetDC(lpSqd->ObjClient->hWnd);
	GetWindowRect(lpSqd->ObjClient->hWnd,&rWin);
	sWin.cx=rWin.right-rWin.left+1;
	sWin.cy=rWin.bottom-rWin.top+1;
	
	sText.cx=180; sText.cy=50; // Larghezza ed altezza del rettangolo
	
	rText.left=(sWin.cx-sText.cx)/2; rText.right=rText.left+sText.cx+1;
	rText.top=(sWin.cy-sText.cy)/2; rText.bottom=rText.top+sText.cy+1;

	if (*lpMessage)
	{
		SetBkMode(hdc,OPAQUE);
		SetBkColor(hdc,RGB(255,255,255));
		SetTextColor(hdc,RGB(120,120,120));
		hFont=GetStockObject(SYSTEM_FONT);
		//Mmp = SetMapMode(hDC, MM_TEXT);
		hFontOld = SelectObject(hdc, hFont);
	//TextOut(hdc, 10, 10, lpMessage,strlen(lpMessage));
		LocalBox(hdc,rText.left,rText.top,rText.right,rText.bottom,RGB(120,120,120),RGB(255,255,255));
		rText.left+=2; rText.top+=2;
		rText.right-=2; rText.bottom-=2;
		DrawText(hdc,
			 lpMessage,
			 strlen(lpMessage),
			 &rText,    // pointer to struct with formatting dimensions
			 DT_CENTER|DT_VCENTER// text-drawing flags
			);
	}
	else
	{
		LocalBox(hdc,rText.left,rText.top,rText.right,rText.bottom,sys.ColorWindowBack,sys.ColorWindowBack);
	}

	//SetMapMode(hDC, Mmp);
	if (hFontOld) SelectObject(hdc, hFontOld);

	ReleaseDC(lpSqd->ObjClient->hWnd,hdc);
}

// --------------------------------------------
// Thread di elaborazione del Filtro
// --------------------------------------------
static DWORD WINAPI SQLExecuteThread(LPVOID Dato)
{
	DWORD dwWait;
	SQDEXT *lpSqd;
	SQLRETURN sqlReturn;
	lpSqd=(SQDEXT *) Dato;

	// ----------------------------------------------------------
	// Segnalo l'attivazione dell'elaborazione
	// ----------------------------------------------------------

	// ATTENZIONE:
	// Bisognerebbe sincronizzare l'accesso alla struttura Sqd
	lpSqd->fInProcess=FALSE;
	lpSqd->fInStop=FALSE;

	while (TRUE)
	{
	 lpSqd->fInProcess=FALSE;
	 lpSqd->fBreak=FALSE;
//	 _dx_(0,20,"SQLExecuteThread in attesa: %d (%d)",ESQL_MAXEVENTLOAD);
	 dwWait=WaitForMultipleObjects(ESQL_MAXEVENTLOAD,lpSqd->hEventSql,FALSE,INFINITE);
//	 _dx_(0,40,"SQLExecuteThread.dwWait:%d ",dwWait);

	 // ---------------------------------------------------------
	 // Chiusura del Thread
	 //
	 if (dwWait==WAIT_OBJECT_0+ESQL_STOP) break;

	 // --------------------------------------------------------------------------------------------------------
	 // 
	 // Richiesta di Esecuzione del comando SQL di query
	 // 
	 // --------------------------------------------------------------------------------------------------------
	 if (dwWait==WAIT_OBJECT_0+ESQL_EXECUTE) 
	 {
		ResetEvent(lpSqd->hEventSql[ESQL_EXECUTE]); 
		if (lpSqd->fBreak) continue;

		// -------------------------------------------------------------
		// Cerco quante righe saranno in totale
		//
		LQueryAssign(lpSqd);

		EnterCriticalSection(&lpSqd->csSqdStruct);
		lpSqd->fInProcess=TRUE;
		lpSqd->ws.offset=0;
		lpSqd->ws.maxcam=0;
		lpSqd->lRowsReady=0;
		lpSqd->lRowsTotal=0;
		lpSqd->fDataReady=FALSE;
		
		// Chiudo l'uso del cursore se è aperto
		if (lpSqd->fCursorOpen) {SQLCloseCursor(lpSqd->hStmtClone); lpSqd->fCursorOpen=FALSE;}
		LWDisp(lpSqd,"Conteggio records\nattendere ...");

#ifdef SQD_DEBUG
		dispx("Execute:conto i record                            ");
#endif
		if (!strstr(lpSqd->lpQueryCount,"1=0"))
		{
			SQLBindCol(lpSqd->hStmtClone, 1, SQL_INTEGER, &lpSqd->lRowsTotal, 0, 0);
			LocalSqlTry("ATTR3",lpSqd->hStmtClone,SQLSetStmtAttr(lpSqd->hStmtClone, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0));
			LocalSqlTry("EXEC_COUNTER",lpSqd->hStmtClone,SQLExecDirect(lpSqd->hStmtClone,lpSqd->lpQueryCount,SQL_NTS));
			LocalSqlTry("FETCH_COUNTER",lpSqd->hStmtClone,SQLFetch(lpSqd->hStmtClone));
			SQLCloseCursor(lpSqd->hStmtClone);
		}
		else
		{
			lpSqd->lRowsTotal=0;
		}

#ifdef SQD_DEBUG
		dispx("Execute:Record presenti %d        ",lpSqd->lRowsTotal); //Sleep(2000);
#endif
		LeaveCriticalSection(&lpSqd->csSqdStruct);
		LWDisp(lpSqd,"");

		// Non ho di record mi fermo
		if (!lpSqd->lRowsTotal) 
			{//InvalidateRect(lpSqd->ObjClient->hWnd,NULL,FALSE); 
		     lpSqd->fInProcess=FALSE; 
			 // Segnalo che il numero di record è pronto
			 EnterCriticalSection(&lpSqd->csSqdStruct);
			 lpSqd->fChanged++;
			 LeaveCriticalSection(&lpSqd->csSqdStruct);
			 continue;
			}
		
		// Richiesto uno stop della ricerca
		if (lpSqd->fBreak) continue;

		// Se ho un buffer
		if (lpSqd->lRowsBuffer)
		{
			LWDisp(lpSqd,"In attesa della\nrisposta dal server...");
			EnterCriticalSection(&lpSqd->csSqdStruct);

#ifdef SQD_DEBUG
			dispx("Thread:Execute ... [%d]",lpSqd->lRowsBuffer); //Sleep(1000);
#endif

			lpSqd->lRowsCurrent=0;
			if (lpSqd->fCursorOpen) {SQLCloseCursor(lpSqd->hStmtClone); lpSqd->fCursorOpen=FALSE;}
			LocalSqlTry("ATTR3",lpSqd->hStmtClone,SQLSetStmtAttr(lpSqd->hStmtClone, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) lpSqd->lRowsBuffer, 0));
			LocalSqlTry("ATTR5",lpSqd->hStmtClone,SQLSetStmtAttr(lpSqd->hStmtClone, SQL_ATTR_ROWS_FETCHED_PTR, &lpSqd->lRowsCurrent, 0));
			LocalSqlTry("SQLEXEC",lpSqd->hStmtClone,SQLExecDirect(lpSqd->hStmtClone,lpSqd->lpQueryActive,SQL_NTS));
			ThreadResultSetAlloc(lpSqd);

#ifdef SQD_DEBUG
			dispx("Thread:Execute OK  (Buf:%d,Current:%d)      ",lpSqd->lRowsBuffer,lpSqd->lRowsCurrent);
#endif

			//lpSqd->fChanged++;
			LeaveCriticalSection(&lpSqd->csSqdStruct);
			lpSqd->fCursorOpen=TRUE;
			
			// Richiesto uno stop della ricerca
			if (lpSqd->fBreak) continue;

			SQD_Fetch(lpSqd); // Richiedo una fetch
			// Chiedo di rinfrescare la finestra
			//lpSqd->fInStop=TRUE; // Avviso che il thread è finito
			lpSqd->fInProcess=FALSE;
		}
	}

	 // A) Il Thread deve allocare la query in memoria in modo da rendere disponibili 
	 //    Le informazioni per lo scroll
	
	 // --------------------------------------------------------------------------------------------------------
	 // 
	 // Richiesta di Esecuzione della Fetch
	 // 
	 // --------------------------------------------------------------------------------------------------------
	 if (dwWait==WAIT_OBJECT_0+ESQL_FETCH) 
	 {
		SINT iOffset;
		ResetEvent(lpSqd->hEventSql[ESQL_FETCH]); 
		
		// Richiesto uno stop della ricerca
		if (lpSqd->fBreak) continue;

		EnterCriticalSection(&lpSqd->csSqdStruct);
		lpSqd->fDataReady=FALSE;
		//InvalidateRect(lpSqd->ObjClient->hWnd,NULL,FALSE);
		lpSqd->lRowsCurrent=0;
		if (!lpSqd->lRowsReady&&lpSqd->fFindLast&&((SINT) lpSqd->lRowsTotal>=(SINT) lpSqd->ws.Enumcam))
		{
			lpSqd->ws.offset=lpSqd->lRowsTotal-lpSqd->ws.Enumcam;
			//efx1(); _d_("CI SONO"); Sleep(2000);
			//lpSqd->fFindLast=FALSE;
		}
		iOffset=lpSqd->ws.offset;
		if (lpSqd->ws.offset<0) lpSqd->ws.offset=0;
		//_dx_(0,20,"Offset %d        ",lpSqd->ws.offset); //Sleep(2000);
		LeaveCriticalSection(&lpSqd->csSqdStruct);

#ifdef SQD_DEBUG
		dispx("                                         Thread:Prima di Fetch (offset %d)                 ",lpSqd->iSQLOffset);
#endif
		//LWDisp(lpSqd,"In attesa della\nrisposta dal server (Fetch) ...");
		if (iOffset<1) sqlReturn=SQLFetchScroll(lpSqd->hStmtClone,SQL_FETCH_FIRST,0);
					   else
					   sqlReturn=SQLFetchScroll(lpSqd->hStmtClone,SQL_FETCH_ABSOLUTE,iOffset+1);

		EnterCriticalSection(&lpSqd->csSqdStruct);
		lpSqd->iSQLOffset=iOffset;
		lpSqd->lRowsReady=lpSqd->lRowsCurrent;
		lpSqd->fDataReady=TRUE;
		InvalidateRect(lpSqd->ObjClient->hWnd,NULL,FALSE); 
		lpSqd->fChanged++; 
		LeaveCriticalSection(&lpSqd->csSqdStruct);
		//LWDisp(lpSqd,"");

#ifdef SQD_DEBUG
		dispx("                                         Thread:Dopo di Fetch [%d][%d]                       ",sqlReturn,lpSqd->lRowsCurrent);
#endif

		lpSqd->fCursorOpen=TRUE;
		lpSqd->fInProcess=FALSE;
		
		// Chiedo di rinfrescare la finestra
		lpSqd->fInStop=TRUE; // Avviso che il thread è finito
	 }
	}
	
	return 0;
}

// Chiedi di aggiornare eseguire un Fetch aggiornato
void SQD_Fetch(SQDEXT *lpSqd)
{
	lpSqd->fBreak=FALSE;
	SetEvent(lpSqd->hEventSql[ESQL_FETCH]); // Chiedo l'esecuzione del comando
}

// Chiedi di aggiornare eseguire un Fetch aggiornato
void SQD_Execute(SQDEXT *lpSqd)
{
	lpSqd->fBreak=FALSE;
	SetEvent(lpSqd->hEventSql[ESQL_EXECUTE]); // Chiedo l'esecuzione del comando
}

// ----------------------------------------------
// PostStopThread
// Sistemazione/Traslazione variabili,handle,ecc...
// dopo la fine dle thread di processo
// ----------------------------------------------
static void PostStopThread(struct WS_INFO *ws,SQDEXT *Sdb)
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
		SQLDriver32(WS_IDX,ADB_info[Sdb->Hdb].AdbFilter->IndexInUse,"");
	}
*/
}

// FTIME
static void FT_SQLThreadRefresh(SINT cmd,void *dao)
{
	SINT b;
    static SINT Count=0;
    static SINT iCount=0;
	BOOL fNoBlank;

	if (cmd==0)
	{
	 for (b=0;b<SQDMAX;b++)
	 {
		if (SQD[b].ObjClient!=NULL)
			{
				struct WS_INFO *ws=&SQD[b].ws;	

				// Se servono più righe chiedo di ricaricarle
				if (SQD[b].ws.numcam!=SQD[b].lRowsBuffer) 
				{
					SQD[b].lRowsBuffer=SQD[b].ws.numcam;
					//_dx_(0,80,"%d) %d:%d   ",iCount++,ws->numcam,SQD[b].lRowsBuffer);
					SQD_Execute(SQD+b);
					//LeaveCriticalSection(&csSql);
					continue;
				}

				// Cambio di Query: Richiedo Execute
				if (SQD[b].fQueryToExecute)
				{
					SQD[b].fQueryToExecute=FALSE;
					SQD_Execute(SQD+b);
					continue;
				}
				
				// Se il thread stà girando
				//if (SQD[b].fInProcess)	
				{
//				 if (ws->maxcam!=(SINT) SQD[b].lRowsTotal) // Ho record diversi ?
				 if (SQD[b].fChanged) // Qualcosa è cambiato
				 {
					LONG OldMax=ws->maxcam; 

					if (SQD[b].ObjClient->hWnd) 
					{
					//	if (!Count)
					//	{
						//efx2();
						 ws->maxcam=SQD[b].lRowsTotal;
						 fNoBlank=ws->fNoBlankLine;
						 if (OldMax>ws->numcam) ws->fNoBlankLine=TRUE;
						 InvalidateRect(SQD[b].ObjClient->hWnd,NULL,FALSE);
						 UpdateWindow(SQD[b].ObjClient->hWnd);
						 ws->fNoBlankLine=fNoBlank;
						 
						 OBJ_RangeAdjust(SQD[b].ObjClient,ws->offset,ws->maxcam,ws->numcam);
						 if (SQD[b].ObjClient->yTitle)
						 {
							 RECT rTitle;
							 rTitle.left=SQD[b].ObjClient->px+2;
							 rTitle.top=SQD[b].ObjClient->py+2;
							 rTitle.bottom=rTitle.top+SQD[b].ObjClient->yTitle-1;
							 rTitle.right=rTitle.left+SQD[b].ObjClient->col1-1;
							 InvalidateRect(WindowNow(),&rTitle,FALSE);
						 }

					//	}
					   // Reset flag
					   EnterCriticalSection(&SQD[b].csSqdStruct);
					   SQD[b].fChanged--;
					   LeaveCriticalSection(&SQD[b].csSqdStruct);
					}
				 }
				} // Thread

				// Controllo se l'offset è cambiato
				if (!Count) 
				{
					if (SQD[b].iSQLOffset!=ws->offset&&ws->offset>-1) //{SQLReload(SQD+b);}
					{
						//_dx_(0,0,"-- %d,%d --   ",SQD[b].iSQLOffset,ws->offset); efx2(); Sleep(1000);
						SQD_Fetch(SQD+b);
						SQD[b].iSQLOffset=ws->offset; // ## DANGER ##
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

// ---------------------------------------------------
// Copia una riga nel buffer del ADB locale
//
BOOL LResultSetToAdb(SINT iWhere,SQDEXT *lpSqd,SINT iRow,BOOL fTrunk)
{
	SINT a;
	EMUBIND *lpEmuBind;
	BYTE *lpValore;
	BYTE *lpDest;

	if (iWhere==WB_CLONE) lpDest=lpSqd->lpCloneBuffer; else lpDest=lpSqd->lpAdbOriginalBuffer;
	if (lpDest==NULL) return TRUE;
	
	EnterCriticalSection(&lpSqd->csSqdStruct);
	memset(lpDest,0,lpSqd->iAdbBufferSize);
	//for (a=0;a<ADB_info[lpSqd->Hdb].AdbHead->Field;a++)
	for (a=0;a<ADB_info[lpSqd->Hdb].iBindField;a++)
	{
		lpEmuBind=ADB_info[lpSqd->Hdb].lpEmuBind+a;
		if (lpEmuBind->fActive) 
		{
			// Copio nel record ADB il campo ODBC
			lpValore=LRSColumn(lpSqd,iRow,lpEmuBind->szSQLName);
			//if (lpValore) _dx_(0,40+a*20,lpValore); 
			if (lpValore)
			{
				ODBCFieldToAdb(lpSqd->Hdb,lpEmuBind,lpValore,fTrunk,lpDest); // Tronco l'informazione
			}
			else
			{
				dispxEx(0,40+a*20,"Non trovato [%s] ? ",lpEmuBind->szSQLName); efx2();
			}
		}
	}
	LeaveCriticalSection(&lpSqd->csSqdStruct);
	return FALSE;
}

//  +-----------------------------------------+
//	| SQLDriver32 - MainFunction
//	+-----------------------------------------+
void *SQLDriver32(struct OBJ *objCalled,SINT cmd,LONG info,CHAR *str)
{
	static struct WINSCR rit,*PtScr;
	static BOOL   CheckFirst=TRUE;
	struct WS_DISPEXT *DExt;
    SQDEXT *lpSqd;
	SINT   a,b;
	SINT   ptClient;
	SQLRETURN sqlReturn;
	BYTE *lpSQLCommand;
	SINT iPointer;
	// -----------------------------------------------
	// Se è la prima volta azzero la struttura       |
	// -----------------------------------------------
	if (CheckFirst)
		{
		 for (b=0;b<SQDMAX;b++)
			{
				memset(SQD+b,0,sizeof(SQDEXT));
				SQD[b].hDictionary=-1;
			}
		 CheckFirst=FALSE;
		 // Attivo FTIME per il refresh degli oggetti
		 FTIME_on(FT_SQLThreadRefresh,1);
		}

	if ((objCalled->tipo!=OW_SCR)&&(objCalled->tipo!=OW_SCRDB)) return 0;

	// ---------------------------------------
	// Trova il Client Object                |
	// ---------------------------------------
	ptClient=-1;
	for (b=0;b<SQDMAX;b++)
		{if (objCalled==SQD[b].ObjClient) {ptClient=b; break;}}

	// Se non c'è provo ad assegnarlo
	if (ptClient==-1)
	 {
		for (b=0;b<SQDMAX;b++)
		 {
			if (SQD[b].ObjClient==NULL)
				{SQD[b].ObjClient=objCalled;
				 ptClient=b;
				 //SQD[b].ExtFilter=ADBNULL;
				 break;
				}
		 }
	 }

	// Se non ci riesco ... errore
	if (ptClient==-1) ehExit("SQLDriver:Too client!");

	// Puntatore alla struttura SQDEXT
	lpSqd=SQD+ptClient;
	
	//_d_("cmd=%s Mode=%d",CmdToString(cmd),lpSqd->Mode); pausa(500);

	//if ((ptClient!=0)||(cmd==WS_EXTFUNZ)) {
	//_d_("-------> %d [%d] %x",cmd,ptClient,(LONG) objCalled); pausa(500);
	if (cmd==WS_INF) 
	{
		return &lpSqd->ws;
	}
	
	switch (cmd) 
	{
	// -----------------------------------
	// APERTURA DEL DRIVER               |
	// -----------------------------------

	case WS_OPEN:
#ifdef SQD_DEBUG
		dispx("OPEN  ");
#endif
		if (info<3) {ehExit("Field ? SdbDriver32");}
		lpSqd->ws.numcam=info;// Assegna il numero di campi da visualizzare
		if (lpSqd->SubProcNotify==NULL) ehExit("SDB:NULL1");

		// Notifico alla funzione esterna l'apertura
		(lpSqd->SubProcNotify)(WS_OPEN,0,0,0,lpSqd->Hdb,lpSqd->iIndex);

	case WS_LOAD :  // Ex Apri

		lpSqd->ObjClient->bFreeze=TRUE; // Blocco la gestione dell'oggetto

		lpSqd->ws.selez=-1; // Nessun selezionato
		lpSqd->ws.maxcam=0;
		lpSqd->ObjClient->tipo=OW_SCR;
		lpSqd->lRowsTotal=0;
		lpSqd->fChanged=0;

		// Alla prima chiamata creo thread e "finestrame" necessario 
		if (!lpSqd->hThread)
		{

//			CHAR szCursor[80];
			InitializeCriticalSection(&lpSqd->csSqdStruct); 
			InitializeCriticalSection(&lpSqd->csSqdQuery); 
			
			// ---------------------------------------------------------------------
			// Alloco lo stantment clone ( Si libererà con WS_DESTROY)
			//
			sqlReturn=SQLAllocHandle(SQL_HANDLE_STMT, sOdbcSection.hConn, &lpSqd->hStmtClone);
			if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO) ehExit("SQLDriver32:hStmt Clone impossible %d",sqlReturn);

			SQLTRY(SQL_HANDLE_STMT,"ATTR1",lpSqd->hStmtClone,SQLSetStmtAttr(lpSqd->hStmtClone, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_READ_ONLY, 0));

			// Chiedo il cursore scrollabile
			sqlReturn=SQLSetStmtAttr(lpSqd->hStmtClone, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0);
			if (sqlReturn==SQL_ERROR)  
			// Altro metodo
			{
				sqlReturn=SQLSetStmtAttr( lpSqd->hStmtClone,SQL_ATTR_CURSOR_TYPE,  (SQLPOINTER) SQL_CURSOR_STATIC, 0);
				if (sqlReturn==SQL_ERROR) win_infoarg("errore in assegnazione cursore");
				sqlReturn=SQLSetStmtAttr( lpSqd->hStmtClone, SQL_ATTR_USE_BOOKMARKS, (SQLPOINTER) SQL_UB_VARIABLE, 0);
				if (sqlReturn==SQL_ERROR) win_infoarg("SQL_ATTR_USE_BOOKMARKS");
			}

			//sprintf(szCursor,"SQD%d",ptClient);
			//SQLTRY(SQL_HANDLE_STMT,"SQD->",lpSqd->hStmtClone,SQLSetCursorName(lpSqd->hStmtClone, szCursor, SQL_NTS));
			//SQLTRY("ASYNC",lpSqd->hStmtClone,SQLSetStmtAttr(lpSqd->hStmtClone, SQL_ATTR_ASYNC_ENABLE, (SQLPOINTER) SQL_ASYNC_ENABLE_ON , 0));

			for (a=0;a<ESQL_MAXEVENTLOAD;a++) lpSqd->hEventSql[a]=CreateEvent(NULL,TRUE,FALSE,NULL); 

			// 4) Creo il Thread (SQLExecuteThread) per l'elaborazione delle query
			lpSqd->hThread = CreateThread(NULL, 
										  0, 
										  SQLExecuteThread, 
										  (LPDWORD) lpSqd,
										  0, 
										  &lpSqd->dwThread);
			SetThreadPriority(lpSqd->hThread,THREAD_PRIORITY_NORMAL);
		}
		lpSqd->ObjClient->bFreeze=FALSE;
		if (SQD_Load(lpSqd)) ehExit("SdbDriver32:No extern function");
		break;


	// -----------------------------------
	// Richiesta di refresh
	// -----------------------------------
	case WS_RELOAD:
		SQD_Execute(lpSqd);
		return NULL; // Non serve stampare


	// -----------------------------------
	// WS_CLOSE IL DRIVER                  |
	// -----------------------------------
	case WS_CLOSE:
			SQLDriver32(objCalled,WS_PROCESS,STOP,NULL);
			lpSqd->ObjClient->bFreeze=TRUE;
			(lpSqd->SubProcNotify)(WS_CLOSE,0,0,0,lpSqd->Hdb,lpSqd->iIndex);
			break;

	// -----------------------------------
	// CHIUSURA DEFINITIVA DEL GESTORE (chiamato in obj_close());
	// -----------------------------------
	case WS_DESTROY: 

			 // Notifico la chiusura alla funzione esterna
			 (lpSqd->SubProcNotify)(WS_DESTROY,0,0,0,lpSqd->Hdb,lpSqd->iIndex);

			 // Fermo il Thread
			 SetEvent(lpSqd->hEventSql[ESQL_STOP]); // Segnalo che siamo in chiusura
			 if (WaitForSingleObject(lpSqd->hThread,5000)) 
			 {
				//_dx_(0,20,"Entro qui");
				//SQLFetchScroll(lpSqd->hStmtClone,SQL_FETCH_FIRST,0);
				//SQLFreeHandle(SQL_HANDLE_STMT,lpSqd->hStmtClone); lpSqd->hStmtClone=0;
				//_dx_(0,40,"Terminate %d",TerminateThread(lpSqd->hThread,0));
			 }
			 
			 // Libero il Cursore e la memoria usata per il dizionario
			 ThreadResultSetFree(lpSqd);
			 if (lpSqd->iHdlFields) memoFree(lpSqd->iHdlFields,"Fields");
			 lpSqd->iHdlFields=0;
				
			 // Libero lo spazio per lo stantment aggiunto (Clone)
			 if (lpSqd->hStmtClone) {SQLFreeHandle(SQL_HANDLE_STMT,lpSqd->hStmtClone); lpSqd->hStmtClone=0;}

			 // Libero la memoria usata per la Query
			 if (lpSqd->lpQueryRequest) {free(lpSqd->lpQueryRequest); lpSqd->lpQueryRequest=NULL;}
			 if (lpSqd->lpQueryActive) {free(lpSqd->lpQueryActive); lpSqd->lpQueryActive=NULL;}

			 // Libero la memoria usata per la Count
			 if (lpSqd->lpQueryCount) {free(lpSqd->lpQueryCount); lpSqd->lpQueryCount=NULL;}

			 // Libero la memoria usata per la WhereAdd
			 if (lpSqd->lpQueryWhereAdd) {free(lpSqd->lpQueryWhereAdd); lpSqd->lpQueryWhereAdd=NULL;}

			 // Libero gli Handle degli eventi
			 for (a=0;a<ESQL_MAXEVENTLOAD;a++) CloseHandle(lpSqd->hEventSql[a]);
			 lpSqd->lppFields=NULL;

			 if (lpSqd->lpCloneBuffer) ehFree(lpSqd->lpCloneBuffer);
			 lpSqd->lpCloneBuffer=NULL;

			 // 1.2.3. Liberi tutti !!
			 DeleteCriticalSection(&lpSqd->csSqdStruct); 
			 DeleteCriticalSection(&lpSqd->csSqdQuery); 
			 
			 // Azzero la struttura di riferimento
			 memset(SQD+ptClient,0,sizeof(SQDEXT));
			 break;

	// -----------------------------------
	// PRESSIONE DI UN TASTO/MOUSE       |
	// -----------------------------------
	case WS_KEYPRESS :
			if (key_press2(KEY_F9)) {strcpy(str,"ESC:F9"); break;}
			// Transfer del comando a sub-driver
			(lpSqd->SubProcNotify)(cmd,NULL,info,str,lpSqd->Hdb,lpSqd->iIndex); 
			break;

	// --------------------------------------------------
	// CERCA L'ULTIMO (POSIZIONA LO SCROLL ALLA FINE    |
	// --------------------------------------------------
	case WS_FINDLAST:
//			DB_find(cmd,info,str,lpSqd);
			break;

	// -----------------------------------
	// SETTA SELEZIONE RECORD            |
	// -----------------------------------
	case WS_SEL : //			  			Settaggio selez
			if (lpSqd->SubProcNotify==NULL) ehExit("SdbDriver32:No ext/disp function");
			if (lpSqd->ws.selez==-1) break;
			break;

	// -------------------------------------
	// SETTA L'OFFSET  (Solo Modo O_SCROLL |
	// -------------------------------------
	case WS_OFF : 
			break;

	// -------------------------------------
	// RITORNA il Puntatore al Record      |
	// -------------------------------------
	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata

		    if ((lpSqd->ws.selez-lpSqd->ws.offset)<0) return NULL;
			
			lpSQLCommand=ehAlloc(EMU_SIZE_QUERY);
			LResultSetToAdb(WB_ORIGINAL,lpSqd,lpSqd->ws.selez-lpSqd->ws.offset,FALSE);
			EmuHrecMaker(lpSqd->Hdb,lpSQLCommand,"=");
			adb_EmuLastWhere(lpSqd->Hdb,lpSQLCommand);
			adb_position(lpSqd->Hdb,&rit.record);
			ehFree(lpSQLCommand);
			return &rit;

	// -------------------------------------
	// RITORNA Selez ??????????            |
	// -------------------------------------
	case WS_REALGET:
			 //ehExit("SDB:Uso RealGet ?");
			 //return (&lpSqd->ws.selez);

			 //iPointer=lpSqd->ws.offset+DExt->ncam;
			 //LResultSetToAdb(WB_CLONE,lpSqd,iPointer-lpSqd->iSQLOffset,TRUE);
			 break;

	// -------------------------------------
	// Refresh a ON                        |
	// -------------------------------------
	case WS_REFON : lpSqd->ws.refre=ON; break;
	case WS_REFOFF : lpSqd->ws.refre=OFF; break;

	case WS_PROCESS:
	
			// Controllo se l'elaborazione è in corso
			if ((info==0)&&(*str=='?'))
			{
			  if (lpSqd->fInProcess) return "!"; else return NULL;
			}

			if ((info==STOP)&&(str==NULL))
			 {
				SQLCancel(lpSqd->hStmtClone); // Cancello il processo nello stantment
				EnterCriticalSection(&lpSqd->csSqdStruct);
				lpSqd->fBreak=TRUE;
				LeaveCriticalSection(&lpSqd->csSqdStruct);
				while (TRUE)
				{
					if (!lpSqd->fInProcess) break;
					Sleep(50);
				}
			}
			break;

	
	// ----------------------------------------------------
	// Chiedo di cambiare il la Where di ricerca
	//
	case WS_SETFILTER:
		if (lpSqd->lpQueryWhereAdd) free(lpSqd->lpQueryWhereAdd);
		lpSqd->lpQueryWhereAdd=NULL;
		if (str)
		{
			lpSqd->lpQueryWhereAdd=malloc(strlen(str)+1);
			strcpy(lpSqd->lpQueryWhereAdd,str);
		}
		lpSqd->fFindLast=info; // Dico che deve andare alla fine
		SQD_Load(lpSqd);
		break;

	// -------------------------------------
	// Richiesta di Stampa dei Dati        |
	// -------------------------------------
	case WS_DISPLAY : //			  			Richiesta buffer
			 if (lpSqd->SubProcNotify==NULL) ehExit("SdbDriver32:No ext/disp function");
			 DExt=(struct WS_DISPEXT *) str;
			
			 // Richiesta di stampa del titolo
			 if (DExt->ncam==-10) 
			 {
				(lpSqd->SubProcNotify)(0,NULL,0,str,lpSqd->Hdb,lpSqd->iIndex); 
				break;
			 }

			 iPointer=lpSqd->ws.offset+DExt->ncam;
			 // Non ho ancora le linee da visualizzare
			 if (DExt->ncam>=lpSqd->lRowsReady||
				 //!lpSqd->fDataReady||
				 (iPointer<lpSqd->iSQLOffset)||
				 ((iPointer-lpSqd->iSQLOffset)>=lpSqd->lRowsBuffer)
				 ) 
			 {
				if (!DExt->bFocus) 
				{
					Tboxp(DExt->px,DExt->py,DExt->px+DExt->lx-1,DExt->py+DExt->ly-2,sys.Color3DLight,SET);
				}
				break;
			 }

			 //_dx_(0,20,"DISPLAY: %d (%d)  (%d)         ",DExt->ncam,lpSqd->ws.offset+DExt->ncam,lpSqd->lRowsReady); Sleep(400);
			 // --------------------------------------------------------------------------
			 // Per emulazione con il passato, devo riempire il record dell'ADB
			 // B) Lo scroll deve poter accedere alle informazioni nel modo "Classico" adb_FldPtr
			 //    e nel modo nuovo (ODBC) per permettere Join e quindi limitare le richieste
			 //
			 if (!LResultSetToAdb(WB_CLONE,lpSqd,iPointer-lpSqd->iSQLOffset,TRUE))
			 {
				(lpSqd->SubProcNotify)(0,
									  lpSqd->lpCloneBuffer,
									  0,
									  str, // struct WS_DISPEXT *)
									  lpSqd->Hdb,
									  lpSqd->iIndex);
			 }
			 break;

	// --------------------------------------------------------------------------
	// COMUNICA LA FUNZIONE ESTERNA  (SubCallback)
	// --------------------------------------------------------------------------
	case WS_EXTFUNZ: 
			 lpSqd->SubProcNotify=(SINT (*)(SINT cmd,CHAR *PtDati,LONG  dato,void  *str,SINT Hdb,SINT IndexNum)) str;
			 break;

	// --------------------------------------------------------------------------
	// COMUNICA Handle del Dbase da Usare  
	// --------------------------------------------------------------------------
	case WS_HDB : //	 era WS_HDB
			 lpSqd->Hdb=(SINT) info;
			 
			 // ---------------------------------------------------------------------
			 // Succede se SdbHDB è prima della dichiarazione della funzione
			 //
			 if (lpSqd->SubProcNotify==NULL) ehExit("No Funzdisp"); else (lpSqd->SubProcNotify)(WS_HDB,NULL,0,str,lpSqd->Hdb,lpSqd->iIndex);

			 break;

	// --------------------------------------------------------------------------
	// COMUNICA L'indice attualmente in Uso  
	// --------------------------------------------------------------------------

	case WS_IDX : //	era WS_IDX
			 lpSqd->iIndex=(SINT) info;
			 // Ho un where Iniziale
			 if (str) 
			 {
				if (lpSqd->lpQueryWhereAdd) free(lpSqd->lpQueryWhereAdd);
				lpSqd->lpQueryWhereAdd=malloc(strlen(str)+1);
				strcpy(lpSqd->lpQueryWhereAdd,str);
				//LQueryAssign(lpSqd);
			 }

			 // Succede se SdbINX è prima della dichiarazione della funzione
			 if (lpSqd->SubProcNotify==NULL)
					ehExit("No Funzdisp");
					else
					(lpSqd->SubProcNotify)(WS_IDX,NULL,0,str,lpSqd->Hdb,lpSqd->iIndex);

			 break;


	// Comunico le linee da visualizzare
	case WS_LINEVIEW: //	 
		// Creo nuova zona di memoria addatta a contenere la nuova dimensione di visione
		//_d_("Prima=%d Adesso=%d",lpSqd->ws.numcam,info); pausa(600);
		//_d_("Prima=%d Adesso=%d",lpSqd->ws.numcam,info); Sleep(500);
		lpSqd->ws.numcam=info;
		//LArrayMake(lpSqd,info);
        /*
		{
		 HREC LastTop=ADBNULL;
		 if (lpSqd->WinScrHdl!=-1) LastTop=lpSqd->WinScr[0].record;
		 
		 lpSqd->Dove=0;
		 lpSqd->End=(SINT) lpSqd->ws.numcam;
		 if ((!lpSqd->Mode)&&(LastTop!=ADBNULL)) adb_get(lpSqd->Hdb,LastTop,lpSqd->IndexNum);
		}
		*/
		break;

	case WS_LINEEDIT: //	 
		lpSqd->ws.Enumcam=info;
		break;

	default:
//	case WS_SETFLAG : //			  			Richiesta buffer
		if (lpSqd->SubProcNotify==NULL) ehExit("SQLDriver32 02? [%d] [%s]",cmd,objCalled->nome);
		(lpSqd->SubProcNotify)(cmd,0,info,str,lpSqd->Hdb,lpSqd->iIndex);
		break;
	 }

	//PtScr=lpSqd->WinScr; 
	PtScr=NULL;
	if (!PtScr) PtScr=&rit;
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
/*
static void DB_GetLast(SQDEXT *Sdb)
{
	SINT a,test;
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
					test=(Sdb->SubProcNotify)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
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
static SINT DB_find(SINT cmd,LONG info,CHAR *str,SQDEXT *Sdb)
{
	/*
	float percent,perc2;
	SINT test;
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
	test=(Sdb->SubProcNotify)(cmd,0,info,str,Sdb->Hdb,Sdb->IndexNum);
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
	 test=(Sdb->SubProcNotify)(WS_FILTER,0,0,0,Sdb->Hdb,Sdb->IndexNum);
	 //printf("test %d",test);
	 if (test!=ON) // Se non lo è setta la lista dall'inizio
					 {return 0;}
//						else
	}

	// Metto in ws->selez la selezione

	adb_position(Sdb->Hdb,&Hrec);
	ws->selez=Hrec;
	SQLDriver32(WS_SEL,Hrec,"");//Sdb->Hdb,Sdb->IndexNum);

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
	Sdb->Dove=0; Sdb->End=(SINT) ws->numcam;

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
 */
 return -1;
}

// --------------------------------------------------
//  SQL LOAD Pre-Caricamento iniziale               |
//                                                  |
//  Ritorna  0 = Tutto OK                           |
//          -1 = Nn Extern                          |
//                                                  |
//                                                  |
//                                                  |
// --------------------------------------------------
static SINT SQD_Load(SQDEXT *lpSqd)
{
	SINT a,iPti;
	EMUBIND *lpEmuBind;
	CHAR *lpSQLQuery;
	struct ADB_INDEX *lpAdbIndex;
	struct ADB_INDEX_SET *IndexSet;
	CHAR *p,*p2;
	CHAR *lpBuffer;
	SINT hdl;
	
	lpSqd->lpAdbOriginalBuffer=adb_DynRecBuf(lpSqd->Hdb);
	lpSqd->iAdbBufferSize=adb_recsize(lpSqd->Hdb);
	if (lpSqd->iAdbBufferSize<1) lpSqd->iAdbBufferSize=64000; // Presenza di campi BLOB/VARCHAR
	if (lpSqd->lpCloneBuffer) ehFree(lpSqd->lpCloneBuffer);
	lpSqd->lpCloneBuffer=ehAlloc(lpSqd->iAdbBufferSize); 
	memset(lpSqd->lpCloneBuffer,0,lpSqd->iAdbBufferSize);

	//lpSqd->ws.offset=0;
	lpSqd->ws.selez=-1;
	
	// Non ho il comando SQL ? Lo creo secondo l'indice
	//if (!Sdb->lpQueryExtract)
	lpAdbIndex=(struct ADB_INDEX *) ((BYTE *) ADB_info[lpSqd->Hdb].AdbHead+sizeof(struct ADB_HEAD)+sizeof(struct ADB_REC)*ADB_info[lpSqd->Hdb].AdbHead->Field);
	IndexSet=(struct ADB_INDEX_SET *) (lpAdbIndex+ADB_info[lpSqd->Hdb].AdbHead->Index);

	// Entro in sezione critica
	//EnterCriticalSection(&lpSqd->csSqdStruct);
	//if (lpSqd->lpQueryExtract) free(lpSqd->lpQueryExtract);

	// Alloco e costruisco il Select
	lpSQLQuery=malloc(EMU_SIZE_QUERY);  
	*lpSQLQuery=0;
	EmuSelectBuilder(lpSqd->Hdb,lpSQLQuery,lpSqd->lpQueryWhereAdd); // Where aggiuntivo

	// Creo un array con le colonne richieste
	p=strstr(lpSQLQuery," "); if (!p) ehExit("SQL_load 1 ?");
	lpBuffer=ehAlloc(strlen(p)+1);
	strcpy(lpBuffer,p);
	p=strstr(lpBuffer," FROM"); if (p) *p=0;
	p=lpBuffer;
	ARMaker(WS_OPEN,NULL);
	while (TRUE)
	{
		p2=strstr(p,","); if (p2) *p2=0;
		for (;*p==' ';p++);
		strTrimRight(p);
		ARMaker(WS_ADD,p);
		if (p2) p=p2+1; else break;
	}
	hdl=ARMaker(WS_CLOSE,"SQD:Fields");
	ehFree(lpBuffer);

	// --------------------------------------------------------------------
	// Ordinati per
	// --------------------------------------------------------------------

	if (strstr(lpSQLQuery,"ORDER")==NULL)
	{
		iPti=-1;
		for (a=0;a<ADB_info[lpSqd->Hdb].AdbHead->IndexDesc;a++)
		{
			if (!strcmp(lpAdbIndex[lpSqd->iIndex].IndexName,IndexSet[a].IndexName)) {iPti=a+1; break;}
		}
		if (iPti<0) ehExit("EMU/adb_find: error iPTI");

		strcat(lpSQLQuery," ORDER BY ");
		for (a=iPti;a<(iPti+lpAdbIndex[lpSqd->iIndex].KeySeg);a++)
		{
			if (a>iPti) strcat(lpSQLQuery,",");
			lpEmuBind=AdbBindInfo(lpSqd->Hdb,IndexSet[a].FieldName,TRUE);
			if (!lpEmuBind) ehExit("EMU/ADBfind: Non trovato il campo [%s]",IndexSet[a].FieldName);
			strcat(lpSQLQuery,lpEmuBind->szSQLName);
			if (IndexSet[a].flag&DESC_KEY)
				strcat(lpSQLQuery," DESC");
				else
				strcat(lpSQLQuery," ASC"); 
		}
	}

	// Se è diversa libero
	EnterCriticalSection(&lpSqd->csSqdQuery);
	if (lpSqd->iHdlFields) memoFree(lpSqd->iHdlFields,"Fields");
	if (lpSqd->lpQueryRequest) free(lpSqd->lpQueryRequest);
	// win_infoarg("[%s]",lpSQLQuery);

	lpSqd->lpQueryRequest=lpSQLQuery;
	lpSqd->fQueryToExecute=TRUE;
	lpSqd->iHdlFields=hdl;
	lpSqd->lppFields=memo_heap(hdl);
	LeaveCriticalSection(&lpSqd->csSqdQuery);

//	_d_("SQL_LOAD:OUT       ");
	return 0;
}

void SQLDriverPreset(struct OBJ *pojStruct, // Struttura Oggetti
					 CHAR *ObjName, // Nome oggetto
					 SINT (*SubCallBack)(SINT cmd,CHAR *PtDati,LONG  dato,void  *str,SINT Hdb,SINT IndexNum), // Sotto procedura di gestione
					 HDB Hdb,
					 SINT iIndex,
					 CHAR *lpWhereIniziale
					 ) // dbase su cui effettuare le ricerche
{
	// Setto il puntatore all'oggetto per chiamate
	struct OBJ *poj;
	poj=ObjFindStruct(pojStruct,ObjName); if (!poj) ehExit("SQLDriverPreset [%s] ?",ObjName);
	SQLDriver32(poj,WS_EXTFUNZ,0,(CHAR *) SubCallBack);   // Setta La funzione di callback
	SQLDriver32(poj,WS_HDB,Hdb,(CHAR *) Hdb );// Setta l'handle del database da interrogare
	SQLDriver32(poj,WS_IDX,iIndex,(CHAR *) lpWhereIniziale);   // Setta l'indice da usare

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



void SQLWhereChange(CHAR *lpObjName,BOOL fFindLast,CHAR *Mess,...)
{
	va_list Ah;
	CHAR *lpBuf;
	lpBuf=malloc(64000);

	if (!Mess)
		obj_message(lpObjName,WS_SETFILTER,fFindLast,NULL);
		else
		{
			va_start(Ah,Mess);
			vsprintf(lpBuf,Mess,Ah); // Messaggio finale

			// A) Cerco l'oggetto
			obj_message(lpObjName,WS_SETFILTER,fFindLast,lpBuf);

			va_end(Ah);
		}
	free(lpBuf);
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

void SQLWhereChange(CHAR *lpObjName,BOOL fFindLast,CHAR *Mess,...)
{
	va_list Ah;
	CHAR *lpBuf;
	lpBuf=malloc(64000);

	if (!Mess)
		obj_message(lpObjName,WS_SETFILTER,fFindLast,NULL);
		else
		{
			va_start(Ah,Mess);
			vsprintf(lpBuf,Mess,Ah); // Messaggio finale

			// A) Cerco l'oggetto
			obj_message(lpObjName,WS_SETFILTER,fFindLast,lpBuf);

			va_end(Ah);
		}
	free(lpBuf);
}

