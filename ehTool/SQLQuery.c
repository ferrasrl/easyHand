//   ---------------------------------------------
//   | SQLQuery
//   | Esegue ed alloca una query di richiesta
//   |
//   | Che Dio me la mandi di nuovo buona ...
//   | Halleluia,  Halleluia ....
//   |            
//   |            
//   |                                           
//   |             by Ferrà Art & Technology 2003
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
// 
#include <time.h>

#ifdef _ADBEMU
  #include "/easyhand/inc/eh_odbc.h"
   extern EH_ODBC_SECTION sOdbcSection;
#endif

static SINT LocalARMaker(SINT cmd,void *ptr);
/*
#ifdef _DEBUG
#define _SQDEBUG 1
#endif
*/
// extern struct ADB_INFO *ADB_info;
// extern EMUSTRU sEmu;

//
// SQLQuery(WS_OPEN,Hdb,"")
// 
static void LOGWrite(CHAR *Mess,...);
static void LocalSqlTry(FSQLQUERY *lpQuery,CHAR *WhoIs,SQLRETURN iResult,CHAR *lpOther);
static void SQResultSetAlloc(FSQLQUERY *lpQuery);
static void SQResultSetFree(FSQLQUERY *lpQuery);
static CHAR *LocalRSColumnCheck(FSQLQUERY *lpQuery,SINT iRow,CHAR *lpField);
static CHAR *LocalRSColumn(FSQLQUERY *lpQuery,SINT iRow,CHAR *lpField,SINT *lpiIdx);

void SQLGetInformation(SQLSMALLINT iType,FSQLQUERY *lpQuery)
{
//	SQLCHAR  szServ[1500];
	SQLSMALLINT i, MsgLen;
	SQLRETURN  rc2;
	CHAR *lpError=NULL;

	if ((lpQuery->sqlReturn == SQL_SUCCESS_WITH_INFO) || (lpQuery->sqlReturn == SQL_ERROR)) 
	{
		// Get the status records.
		i = 1;
		lpError=ehAlloc(16000); *lpError=0;
		while ((rc2 = SQLGetDiagRec(iType, lpQuery->hStmtClone, i, lpQuery->sqlState, &lpQuery->iNativeError,lpQuery->sqlMsg, sizeof(lpQuery->sqlMsg), &MsgLen)) != SQL_NO_DATA) 
		{ 
		 //DisplayError(SqlState,NativeError,Msg,MsgLen);
		 //sprintf(szServ,"Error rc1=%d da '%s'\nSQLState = %s\nNativeError=%d\n%s (LenMsg=%d)\n", rc1,WhoIs,SqlState,NativeError,Msg,MsgLen);
		 //strcat(lpError,szServ);
		 i++;
		}
	}
	//return lpError;
}


void *SQLQuery(SINT cmd,FSQLQUERY *lpQuery,LONG info,void *ptr)
{
	SINT a,iPti;
	EMUBIND *lpEmuBind;
	CHAR *lpSQLQuery;
	struct ADB_INDEX *lpAdbIndex;
	struct ADB_INDEX_SET *IndexSet;
	CHAR *p,*p2,*lp;
	CHAR *lpBuffer;
	SINT iOffset;
	SINT iCol;
	SQLRETURN sqlReturn;
	UDWORD fSupportedOpt;
	CHAR szServ[80];

	switch (cmd)
	{
		// ------------------------------------------------------------------
		// Apro una richiesta di query
		//
		case WS_OPEN: 

			// --------------------------------------------------------------------
			// Alloco la memoria necessaria
			// --------------------------------------------------------------------
			lpQuery->Hdb=(HDB) info;

			// ---------------------------------------------------------------------
			// Alloco lo stantment clone ( Si libererà con WS_DESTROY)
			//
			sqlReturn=SQLAllocHandle(SQL_HANDLE_STMT, sOdbcSection.hConn, &lpQuery->hStmtClone);
			if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO) ehExit("SQ:hStmt Clone impossible %d",sqlReturn);

			SQLTRY(SQL_HANDLE_STMT,"ATTR1",lpQuery->hStmtClone,SQLSetStmtAttr(lpQuery->hStmtClone, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_READ_ONLY, 0));
			sprintf(szServ,"QRS%d-%d",clock(),rand()*1000);
			SQLTRY(SQL_HANDLE_STMT,"SQD->",lpQuery->hStmtClone,SQLSetCursorName(lpQuery->hStmtClone, szServ, SQL_NTS));
//			SQLTRY(SQL_HANDLE_STMT,"ATTR2",lpQuery->hStmtClone,SQLSetStmtAttr(lpQuery->hStmtClone, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0));
			// Chiedo il cursore scrollabile
			
			// Controllo gli attributi del cursore
			sqlReturn=SQLGetStmtAttr(lpQuery->hStmtClone, SQL_ATTR_CURSOR_TYPE, &fSupportedOpt, 4, 0);
			if (sqlReturn!=SQL_SUCCESS&&sqlReturn!=SQL_SUCCESS_WITH_INFO)  {win_infoarg("errore");}

			if (fSupportedOpt&SQL_SO_FORWARD_ONLY) win_infoarg("Cursor:SQL_SO_FORWARD_ONLY");
			if (fSupportedOpt&SQL_SO_STATIC) win_infoarg("Cursor:SQL_SO_STATIC");
			if (fSupportedOpt&SQL_SO_KEYSET_DRIVEN) win_infoarg("Cursor:SQL_SO_KEYSET_DRIVEN");
			if (fSupportedOpt&SQL_SO_DYNAMIC) win_infoarg("Cursor:SQL_SO_DYNAMIC");
			if (fSupportedOpt&SQL_SO_MIXED) win_infoarg("Cursor:SQL_SO_MIXED");

			fSupportedOpt=0;

			// Chiedo il cursore scrollabile
			lpQuery->fODBCScrollable=TRUE;
			sqlReturn=SQLSetStmtAttr(lpQuery->hStmtClone, SQL_ATTR_CURSOR_SCROLLABLE, (SQLPOINTER) SQL_SCROLLABLE , 0);
			if (sqlReturn==SQL_ERROR)  {lpQuery->fODBCScrollable=FALSE;}

			// Altro metodo
			if (!lpQuery->fODBCScrollable)
			{
				sqlReturn=SQLSetStmtAttr( lpQuery->hStmtClone,SQL_ATTR_CURSOR_TYPE,  (SQLPOINTER) SQL_CURSOR_STATIC, 0);
				if (sqlReturn==SQL_ERROR) win_infoarg("errore in assegnazione cursore");
				sqlReturn=SQLSetStmtAttr( lpQuery->hStmtClone, SQL_ATTR_USE_BOOKMARKS, (SQLPOINTER) SQL_UB_VARIABLE, 0);
				if (sqlReturn==SQL_ERROR) win_infoarg("SQL_ATTR_USE_BOOKMARKS");
				lpQuery->fODBCScrollable=TRUE;
			}

			// --------------------------------------------------------------------
			// Buffer e indirizzi del ADB dataBase
			// --------------------------------------------------------------------
			if (lpQuery->Hdb!=SQUERY_DIRECT) // Query su HDB esistente
			{
				lpQuery->lpAdbOriginalBuffer=adb_DynRecBuf(lpQuery->Hdb);
				lpQuery->iAdbBufferSize=adb_recsize(lpQuery->Hdb);

				// Non ho il comando SQL ? Lo creo secondo l'indice
				//if (!Sdb->lpQueryExtract)
				lpAdbIndex=(struct ADB_INDEX *) ((BYTE *) ADB_info[lpQuery->Hdb].AdbHead+sizeof(struct ADB_HEAD)+sizeof(struct ADB_REC)*ADB_info[lpQuery->Hdb].AdbHead->Field);
				IndexSet=(struct ADB_INDEX_SET *) (lpAdbIndex+ADB_info[lpQuery->Hdb].AdbHead->Index);

				// Alloco e costruisco il SELECT
				lpSQLQuery=malloc(EMU_SIZE_QUERY);  *lpSQLQuery=0;
				if (ptr) 
					{if (* (BYTE *) ptr) 
								EmuSelectBuilder(lpQuery->Hdb,lpSQLQuery,ptr); // Where aggiuntivo
								else
								EmuSelectBuilder(lpQuery->Hdb,lpSQLQuery,NULL); // Where aggiuntivo
					}
					else
					{
						EmuSelectBuilder(lpQuery->Hdb,lpSQLQuery,NULL); // Where aggiuntivo
					}
#ifdef _SQDEBUG
				//	win_infoarg(lpSQLQuery);
#endif
			}
			else // Query LIBERO
			{
				lpQuery->lpAdbOriginalBuffer=NULL;
				lpQuery->iAdbBufferSize=0;

				// Alloco e costruisco il SELECT
				lpSQLQuery=malloc(EMU_SIZE_QUERY);  
				*lpSQLQuery=0;
				strcpy(lpSQLQuery,ptr);
			}

			// --------------------------------------------------------------------
			// Estraggo i Campi coinvolti
			// --------------------------------------------------------------------

			if (lpQuery->fSelectName)
			{
				p=strstr(lpSQLQuery," "); if (!p) ehExit("SQL_load 1 ?");
				lpBuffer=malloc(strlen(p)+1); 
				strcpy(lpBuffer,p);
				p=strstr(lpBuffer," FROM"); if (p) *p=0;
				p=lpBuffer;
				LocalARMaker(WS_OPEN,NULL);
				lpQuery->iQNum=0;
				while (TRUE)
				{
					p2=strstr(p,","); if (p2) *p2=0;
					for (;*p==' ';p++); strTrimRight(p);
					LocalARMaker(WS_ADD,p);
					lpQuery->iQNum++;
					if (p2) p=p2+1; else break;
				}
				lpQuery->iQFields=LocalARMaker(WS_CLOSE,"SQ:Fields");
				lpQuery->lppQFields=memo_heap(lpQuery->iQFields);
				free(lpBuffer);
			}

			// --------------------------------------------------------------------
			// Ordinati per
			// --------------------------------------------------------------------

			if (lpQuery->Hdb!=SQUERY_DIRECT) // Query su HDB esistente
			{
				iPti=-1;
				for (a=0;a<ADB_info[lpQuery->Hdb].AdbHead->IndexDesc;a++)
				{
					if (!strcmp(lpAdbIndex[lpQuery->iIndex].IndexName,IndexSet[a].IndexName)) {iPti=a+1; break;}
				}
				if (iPti<0) ehExit("EMU/adb_find: error iPTI");
		
				strcat(lpSQLQuery," ORDER BY ");
				for (a=iPti;a<(iPti+lpAdbIndex[lpQuery->iIndex].KeySeg);a++)
				{
					if (a>iPti) strcat(lpSQLQuery,",");

					lpEmuBind=AdbBindInfo(lpQuery->Hdb,IndexSet[a].FieldName,TRUE);
					if (!lpEmuBind) ehExit("EMU/ADBfind: Non trovato il campo [%s]",IndexSet[a].FieldName);
					strcat(lpSQLQuery,lpEmuBind->szSQLName);
					strcat(lpSQLQuery," ASC");
				}
			}

			// --------------------------------------------------------------------
			// Preparo le stringhe per la richiesta
			// --------------------------------------------------------------------
			if (lpQuery->lpQueryRequest) free(lpQuery->lpQueryRequest);
			lpQuery->lpQueryRequest=lpSQLQuery;

			if (lpQuery->lpQueryCount) free(lpQuery->lpQueryCount);
			lpQuery->lpQueryCount=NULL;

			if (lpQuery->Hdb!=SQUERY_DIRECT) // Query su HDB esistente
			{
				lpQuery->lpQueryCount=malloc(EMU_SIZE_QUERY);
				lp=strstr(lpQuery->lpQueryRequest,"FROM");
				if (lp)
				{
					sprintf(lpQuery->lpQueryCount,"SELECT COUNT(*) %s",lp);
					lp=strstr(lpQuery->lpQueryCount,"ORDER"); if (lp) *lp=0;
				}
				else
				{
					//*lpQuery->lpQueryCount;
					ehExit("Query count ?");
				}
			}
			
			// -------------------------------------------------------------
			// Cerco quante righe saranno in totale
			//
			lpQuery->lRowsReady=0;
			lpQuery->lRowsTotal=0;
		
			// Chiudo l'uso del cursore se è aperto
			if (lpQuery->fCursorOpen) {SQLCloseCursor(lpQuery->hStmtClone); lpQuery->fCursorOpen=FALSE;}

			if (lpQuery->Hdb!=SQUERY_DIRECT&&lpQuery->lpQueryCount) 
			{
#ifdef _SQDEBUG
				_dx_(0,30,"SQ:Execute:conto i record                            ");
#endif
				SQLBindCol(lpQuery->hStmtClone, 1, SQL_INTEGER, &lpQuery->lRowsTotal, 0, 0);
				LocalSqlTry(lpQuery,"SQ:ATTR3",SQLSetStmtAttr(lpQuery->hStmtClone, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 1, 0),NULL);
				LocalSqlTry(lpQuery,"SQ:EXEC_COUNTER",SQLExecDirect(lpQuery->hStmtClone,lpQuery->lpQueryCount,SQL_NTS),lpQuery->lpQueryCount);
				LocalSqlTry(lpQuery,"SQ:FETCH_COUNTER",SQLFetch(lpQuery->hStmtClone),NULL);

#ifdef _SQDEBUG
				_dx_(0,30,"SQ:Execute:Record presenti %d        ",lpQuery->lRowsTotal); //Sleep(2000);
#endif
				SQLCloseCursor(lpQuery->hStmtClone);

				//win_infoarg("[%s][%d]",lpQuery->lpQueryCount,lpQuery->lRowsTotal);
				// Non ho di record mi fermo
				if (!lpQuery->lRowsTotal) break;
				if (!lpQuery->lRowsBuffer) lpQuery->lRowsBuffer=lpQuery->lRowsTotal;
			}
#ifdef _SQDEBUG
			_dx_(0,0,"SQ:Execute ... [%d]",lpQuery->lRowsBuffer); //Sleep(1000);
#endif
			if (!lpQuery->lRowsBuffer) 
			{
				ehExit("SQLQuery:Non stabilito il numero di righe da caricare");
				break; // Numero totale di righe in memoria che deve caricare
			}
			lpQuery->lRowsCurrent=0;
			
			if (lpQuery->fCursorOpen) {SQLCloseCursor(lpQuery->hStmtClone); lpQuery->fCursorOpen=FALSE;}
			LocalSqlTry(lpQuery,"SQ:ATTR3",SQLSetStmtAttr(lpQuery->hStmtClone, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) lpQuery->lRowsBuffer, 0),NULL);
			LocalSqlTry(lpQuery,"SQ:ATTR5",SQLSetStmtAttr(lpQuery->hStmtClone, SQL_ATTR_ROWS_FETCHED_PTR, &lpQuery->lRowsCurrent, 0),NULL);
			
			// Eseguo il comando ***********************************************
			lpQuery->iNativeError=0; *lpQuery->sqlState=0; *lpQuery->sqlMsg=0;
			LocalSqlTry(lpQuery,"SQ:SQLEXEC",SQLExecDirect(lpQuery->hStmtClone,lpQuery->lpQueryRequest,SQL_NTS),lpQuery->lpQueryRequest);
			
			if (!lpQuery->fNoRead) 
				SQResultSetAlloc(lpQuery); // Eseguo l'allocazione della memoria lpEf[] ed il bind dei campi
				else
				{
					//win_infoarg("%d,%d",lpQuery->sqlReturn,SQL_SUCCESS_WITH_INFO);
					if (lpQuery->sqlReturn==SQL_SUCCESS_WITH_INFO)
					{
						SQLGetInformation(SQL_HANDLE_STMT,lpQuery);
					//	win_infoarg("%d [%s][%s]",lpQuery->iNativeError,lpQuery->sqlState,lpQuery->sqlMsg);
					}
					//win_infoarg(SQLDisplay(SQL_HANDLE_STMT,"IO",lpQuery->hStmtClone,lpQuery->sqlReturn)); 
				}
#ifdef _SQDEBUG
			_dx_(0,0,"SQ:Execute OK  (Buf:%d,Current:%d)      ",lpQuery->lRowsBuffer,lpQuery->lRowsCurrent);
#endif
			lpQuery->fCursorOpen=TRUE;
			// Richiedo una Fetch
			return lpQuery;

		// Esegue la Fetch
		case WS_PROCESS: 

			lpQuery->lRowsCurrent=0;
			iOffset=info;//lpQuery->ws.offset;
			if (!iOffset) 
				{
					sqlReturn=SQLFetch(lpQuery->hStmtClone); iOffset=0;
				}
				else
				{
					if (iOffset==(lpQuery->iSQLOffset+lpQuery->lRowsReady))
						{sqlReturn=SQLFetch(lpQuery->hStmtClone);}
						else
						{sqlReturn=SQLFetchScroll(lpQuery->hStmtClone,SQL_FETCH_ABSOLUTE,iOffset+1);}
				}
/*
			if (iOffset<1) sqlReturn=SQLFetchScroll(lpQuery->hStmtClone,SQL_FETCH_FIRST,0);
						   else
						   sqlReturn=SQLFetchScroll(lpQuery->hStmtClone,SQL_FETCH_ABSOLUTE,iOffset+1);
*/
			lpQuery->iSQLOffset=iOffset;
			lpQuery->lRowsReady=lpQuery->lRowsCurrent;
/*
			//lpQuery->lpEF[i].iSize
			for (a=0;a<lpQuery->iFieldNum;a++)
			{
				if (!strcmp(ptr,lpQuery->lpEF[a].szName))
				{
						return lpQuery->lpEF[a].lpField+(info*lpQuery->lpEF[a].iSize);		
				}
			}
*/
#ifdef _SQDEBUG
			_dx_(0,20,"SQ:Dopo di Fetch [%d][%d]                       ",sqlReturn,lpQuery->lRowsCurrent);
#endif
			lpQuery->fCursorOpen=TRUE;
			return lpQuery;
	
		// ------------------------------------------------------------------
		// Chiudo una richiesta di query
		//
		case WS_CLOSE:

			 // Libero lo spazio del dizionario
			 SQResultSetFree(lpQuery);

			 // Libero lo spazio per lo stantment aggiunto (Clone)
			 if (lpQuery->fCursorOpen) {SQLCloseCursor(lpQuery->hStmtClone); lpQuery->fCursorOpen=FALSE;}
			 if (lpQuery->hStmtClone) {SQLFreeHandle(SQL_HANDLE_STMT,lpQuery->hStmtClone); lpQuery->hStmtClone=0;}

			 // Libero la memoria usata per la richiesta
			 if (lpQuery->lpQueryRequest) {free(lpQuery->lpQueryRequest); lpQuery->lpQueryRequest=NULL;}
			 // Libero la memoria usata per la Count
			 if (lpQuery->lpQueryCount) {free(lpQuery->lpQueryCount); lpQuery->lpQueryCount=NULL;}

			 if (lpQuery->fSelectName)
			 {
				if (lpQuery->iQFields) memoFree(lpQuery->iQFields,"QFields");
				lpQuery->iQFields=0;
				lpQuery->lppQFields=NULL;
			 }

			 // Azzero la struttura di riferimento
			 //memset(lpQuery,0,sizeof(FSQLQUERY));
			 break;
	
		// ------------------------------------------------------------------
		// Leggo una linea dal buffer (copio in ADB Buffer)
		//
		case WS_REALGET:

			//if (iWhere==WB_CLONE) lpDest=lpQuery->lpAdbCloneBuffer; else 
			if (lpQuery->Hdb<0) ehExit("Query ?? 2");
			lpQuery->lpAdbOriginalBuffer=adb_DynRecBuf(lpQuery->Hdb);
			memset(lpQuery->lpAdbOriginalBuffer,0,lpQuery->iAdbBufferSize);
			if (info<0||info>=lpQuery->lRowsReady) return NULL;
//			for (a=0;a<ADB_info[lpQuery->Hdb].AdbHead->Field;a++)

			for (a=0;a<ADB_info[lpQuery->Hdb].iBindField;a++)
			{
				lpEmuBind=ADB_info[lpQuery->Hdb].lpEmuBind+a;
				if (lpEmuBind->fActive)
				{
					SINT idx;
					// Copio nel record ADB il campo ODBC
					//
					lp=LocalRSColumn(lpQuery,info,lpEmuBind->szSQLName,&idx);
					if (lp)
					{/*
						if (!strcmp(lpEmuBind->szSQLName,"D1.LOTTO")&&
							strlen(lp)>10)
							win_infoarg("[%s=%s]",lpEmuBind->szSQLName,lp); // OK
*/ 
						//ODBCFieldToAdb(lpQuery->Hdb,lpEmuBind,lp,FALSE,NULL); // Tronco l'informazione
						ODBCFieldToAdb(lpQuery->Hdb,
									   lpEmuBind,
									   lp,
									   FALSE,
									   NULL); // Tronco l'informazione
					}
					else
					{
						win_infoarg("%d) |%s| %d?",info,lpEmuBind->szSQLName,strlen(lpEmuBind->szSQLName));
						//LocalRSColumnCheck(lpQuery,info,lpEmuBind->szSQLName);
					}
				}
			}
			return lpQuery;

		// ------------------------------------------------------------------
		// Leggo una campo diretto nel bind
		//
		case WS_BUF:

			if (info<0||info>=lpQuery->lRowsReady) return NULL;

			// --------------------------------------------------
			// Ricerca per nome
			if (* (CHAR *) ptr>='A' && * (CHAR *) ptr<='Z')
			{
				for (a=0;a<lpQuery->iFieldNum;a++)
				{
					if (!strcmp(ptr,lpQuery->lpEF[a].szName))
					{
						if (lpQuery->fAutoTrimRight) strTrimRight(lpQuery->lpEF[a].lpField+(info*lpQuery->lpEF[a].iSize));
						return lpQuery->lpEF[a].lpField+(info*lpQuery->lpEF[a].iSize);		
					}
				}
				return NULL;
			}

			// --------------------------------------------------
			// Ricerca per colonna
			iCol=atoi(ptr);
			if (iCol<1||iCol>lpQuery->iFieldNum) return NULL;
			iCol--;
			return lpQuery->lpEF[iCol].lpField+(info*lpQuery->lpEF[iCol].iSize);

	}
	return NULL;
}

static void LocalSqlTry(FSQLQUERY *lpQuery,CHAR *WhoIs,SQLRETURN iResult,CHAR *lpOther) 
{
   SQLRETURN rc = iResult; 
   lpQuery->sqlReturn=iResult;
   lpQuery->sqlLastError=0;
	
   if (!lpQuery->hStmtClone) ehExit("SQ:LocalTry hstmt = NULL");
   if (rc != SQL_SUCCESS) 
      { 
		 if (rc != SQL_SUCCESS_WITH_INFO) 
		 {
			lpQuery->sqlLastError=iResult;
			ehPrint("Query:%s\nError:%s",lpOther,SQLDisplay(SQL_HANDLE_STMT,WhoIs,lpQuery->hStmtClone,rc));
			 if (!lpQuery->fHideError)
			 {
				win_infoarg(SQLDisplay(SQL_HANDLE_STMT,WhoIs,lpQuery->hStmtClone,rc)); 
			 }
#ifdef _SQDEBUG
			 _d_(SQLDisplay(SQL_HANDLE_STMT,WhoIs,lpQuery->hStmtClone,rc)); 
#endif
			 // Scrivo nel log l'errore
			 LOGWrite(SQLDisplay(SQL_HANDLE_STMT,WhoIs,lpQuery->hStmtClone,rc));
			 if (lpOther) LOGWrite(lpOther);
		 }
      } 
}


static CHAR *LocalRSColumn(FSQLQUERY *lpQuery,SINT iRow,CHAR *lpField,SINT *lpiIdx)
{
	SINT i;
	if (iRow<0||iRow>=lpQuery->lRowsReady) return NULL;
	for (i=0;i<lpQuery->iFieldNum;i++)
	{
		if (!strcmp(lpQuery->lpEF[i].szName,lpField)) 
		{
			*lpiIdx=i;
			return lpQuery->lpEF[i].lpField+(lpQuery->lpEF[i].iSize*iRow);
		}
	}
	return NULL;
}
static CHAR *LocalRSColumnCheck(FSQLQUERY *lpQuery,SINT iRow,CHAR *lpField)
{
	SINT i;
	if (iRow<0||iRow>=lpQuery->lRowsReady) return NULL;
	for (i=0;i<lpQuery->iFieldNum;i++)
	{
		//if (!strcmp(lpQuery->lpEF[i].szName,lpField)) return lpQuery->lpEF[i].lpField+(lpQuery->lpEF[i].iSize*iRow);
		win_infoarg("[%s][%s]",lpQuery->lpEF[i].szName,lpField);
	}
	return NULL;
}

static void SQResultSetAlloc(FSQLQUERY *lpQuery)
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
	 SQResultSetFree(lpQuery);

	 iCount=0; iRowMemory=0;
	 for (i=1;;i++)
	 {
	  if (SQLDescribeCol(lpQuery->hStmtClone, // Handle del file
					     (SQLSMALLINT) i, // Numero della colonna
					     szNameColumn,sizeof(szNameColumn),
						 &NameLenght,&DataType,
				         &ColumnSize,&DecimnaDigits,
				         &Nullable)!=SQL_SUCCESS) break;
	  iCount++;
	 }

	 // Alloco memoria per descrizione Campi
	 lpQuery->iFieldNum=iCount;
	 //ADB_info[Hdl].iFreeBindSize=iRowMemory;
	 iTotMemory=sizeof(EMUFIELD)*iCount;
	 lpQuery->hDictionary=memoAlloc(M_HEAP,iTotMemory,"SQ:Dictionary");
	 if (lpQuery->hDictionary<0) ehExit("ODBC:Errore in LResultSetMaker()");
	 lpQuery->lpEF=memo_heap(lpQuery->hDictionary); 
	 memset(lpQuery->lpEF,0,iTotMemory);

	 // Alloco e binderizzo i campi
	 for (i=0;i<lpQuery->iFieldNum;i++)
	 {
	  if (SQLDescribeCol(lpQuery->hStmtClone, // Handle del file
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
	  memset(lpQuery->lpEF+i,0,sizeof(EMUFIELD));
	  if (lpQuery->fSelectName)
			 strcpy(lpQuery->lpEF[i].szName,lpQuery->lppQFields[i]);
			 else
			 strcpy(lpQuery->lpEF[i].szName,szNameColumn);
	  strcpy(lpQuery->lpEF[i].szType,lpType);
      lpQuery->lpEF[i].iType=DataType;
	  lpQuery->lpEF[i].iSize=ColumnSize;
	  lpQuery->lpEF[i].lpField=GlobalAlloc(GPTR,ColumnSize*lpQuery->lRowsBuffer);

	  //win_infoarg("%d) [%s]%s[T:%d][S:%d] ",i,lpQuery->lpEF[i].szName,lpType,lpQuery->lpEF[i].iType,lpQuery->lpEF[i].iSize);
	  if (SQLBindCol(lpQuery->hStmtClone, 
					(SQLSMALLINT) (i+1), 
					(SQLSMALLINT) iTypeC, 
					(PTR) lpQuery->lpEF[i].lpField, 
					lpQuery->lpEF[i].iSize, 0)) 
					{ehExit("ODBC: BINDErrore in [%s]",lpQuery->lpEF[i].szName);}
	 }
}

static void SQResultSetFree(FSQLQUERY *lpQuery)
{
	SINT i;
	if (lpQuery->hDictionary<1) return;

	// Libero risorse precedentemente impegnate
	for (i=0;i<lpQuery->iFieldNum;i++)
	{
		GlobalFree(lpQuery->lpEF[i].lpField);
	}
	memoFree(lpQuery->hDictionary,"ODBC"); 
	lpQuery->hDictionary=0;
}


static void LOGWrite(CHAR *Mess,...)
{	
	FILE *ch;
	CHAR *lpFile="c:\\Eh3\\SQLQuery.log";
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

static SINT LocalARMaker(SINT cmd,void *ptr)
{
	#define LNSIZE sizeof(LONG)
	static LONG RecordSize;
	static UINT ListRec=0;
	static UINT ListPtr=0;
	static UINT ListCount=0;
	static LONG *lpOffsets=NULL;
	static CHAR *lpRecords=NULL;
	SINT Len;
	SINT hdl;
	SINT a;

	CHAR *lpDest;
	void **lpPTDest;

	switch (cmd)
	{
	 case WS_OPEN:
				if ((lpOffsets!=NULL)||(lpRecords!=NULL)) ehExit("AR0");
				if (ptr) RecordSize=* (SINT *) ptr; else RecordSize=0;
				lpOffsets=NULL;
				ListPtr=0;
				ListRec=0;
				ListCount=0;
				break;

	 case WS_ADD:

				if (ptr!=NULL)
				{
				 if (!RecordSize) Len=strlen(ptr)+1; else Len=(UINT) RecordSize;
				 if (Len<1) ehExit("AR1");
                }
				if (ptr!=NULL)
				{
				// Archiviato il nuovo record
				if (!ListRec) {lpRecords=malloc(Len);
							   lpDest=lpRecords;
								}
							   else
							   {lpRecords=realloc(lpRecords,(UINT) (ListRec+Len));
							    lpDest=lpRecords+ListRec;
								}
				memcpy(lpDest,ptr,Len);
				} else {lpDest=NULL;Len=0;}

				if (!ListPtr) {lpOffsets=malloc(LNSIZE);} else {lpOffsets=realloc(lpOffsets,LNSIZE*(ListCount+1));}
				if (ptr==NULL) lpOffsets[ListCount]=-1; else lpOffsets[ListCount]=ListRec;
				ListRec+=Len;
				ListPtr+=sizeof(void *);
				ListCount++;
				break;

	 case WS_CLOSE:
				
				LocalARMaker(WS_ADD,NULL);
				
				hdl=memoAlloc(M_HEAP,ListPtr+ListRec,ptr);
				
				// Sposto nella nuova memoria i puntatori
				lpDest=farPtr(memo_heap(hdl),ListPtr);
				memcpy(lpDest,lpRecords,ListRec);
			    
				//win_infoarg("%d %d %s",ListPtr,ListRec,lpDest);
                // Rialloco i Puntatori a Puntatori
				lpPTDest=(void **) memo_heap(hdl);
				for (a=0;a<(SINT) ListCount;a++)
				{
				 if (lpOffsets[a]==-1) lpPTDest[a]=NULL; else lpPTDest[a]=farPtr(lpDest,lpOffsets[a]);
				}
				free(lpOffsets);
				free(lpRecords);

				lpOffsets=NULL;
				lpRecords=NULL;
				ListRec=0;
				ListPtr=0;
				return hdl;
	}
 return 0;
}



/*
	// Esempio di comando diretto
	FSQLQUERY sQuery;
	CHAR *lpSQLCommand,*lpLibrary;

	lpSQLCommand=malloc(EMU_SIZE_QUERY);
	lpLibrary=EmuGetInfo("C.Library",0,0); if (!lpLibrary) lpLibrary="-?-";

    strcpy(lpSQLCommand,"DELETE FROM {LIB}.PGMG31");
	while (TagExchange(lpSQLCommand,"{LIB}",lpLibrary));

	ZeroFill(sQuery);
	sQuery.iIndex=0;
	sQuery.lRowsBuffer=10;
	SQLQuery(WS_OPEN,&sQuery,0,lpSQLCommand);
	SQLQuery(WS_CLOSE,&sQuery,0,NULL);
*/

//
// SQLQueryArg()
// New 2007
//

BOOL SQLQueryArg(CHAR *pMess,...)
{
	BOOL fRet=FALSE;
	FSQLQUERY sQuery;
	CHAR *lpSQLCommand;
	va_list Ah;
	va_start(Ah,pMess);
	
	lpSQLCommand=ehAlloc(EMU_SIZE_QUERY);
	vsprintf(lpSQLCommand,pMess,Ah); // Messaggio finale
	va_end(Ah);

	ZeroFill(sQuery);
	sQuery.lRowsBuffer=1;
	sQuery.fNoRead=TRUE;
	SQLQuery(WS_OPEN,&sQuery,SQUERY_DIRECT,lpSQLCommand);
	if (sQuery.sqlLastError&&sQuery.sqlLastError!=SQL_NO_DATA) {win_infoarg("SQLQueryArg(): %s",lpSQLCommand); fRet=TRUE;}
	SQLQuery(WS_CLOSE,&sQuery,0,NULL);
	ehFree(lpSQLCommand);
	return fRet;
}
