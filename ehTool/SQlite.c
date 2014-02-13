//   +-------------------------------------------+
//   | SQlite
//   | Interfaccia al dbase SQlite
//   |             
//   |							  Ferrà srl 2008
//   +-------------------------------------------+
//
//    http://www.sqlite.org/

#include "/easyhand/inc/easyhand.h"
#include <time.h>
#define SQL_BUFFER_QUERY_DEFAULT 8192

EH_SQLITE_SECTION sqSection={NULL};

//
// Crea_SQLite_open()
//
BOOL sqlite_open(CHAR * lpFileName,int iFlags)
{
	int rc;

	//
	// Il nome del dbase viene creato prendendo il nome del FXD e nella stessa cartella
	//
	_(sqSection);
	if (!iFlags)
		rc = sqlite3_open(lpFileName, &sqSection.dbSqlLite);
		else
		rc = sqlite3_open_v2(lpFileName, &sqSection.dbSqlLite,iFlags,NULL);
	if (rc)
	{
		win_infoarg("Non posso aprire (%s): %s\n", lpFileName, sqlite3_errmsg(sqSection.dbSqlLite));
		sqlite3_close(sqSection.dbSqlLite);
		return TRUE;
	}

//	sqlite3_create_function(sqSection.dbSqlLite, 'like', 2,SQLITE_UTF8, nil, Like_Ansi_Callback, nil, nil);
	sqSection.pszQueryBuffer=ehAllocZero(SQL_BUFFER_QUERY_DEFAULT);
	sqSection.dwQueryTimeout=15; // Secondi un cui tenta di fare la query
	return FALSE;
}

//
// Crea_SQLite_close()
//
BOOL sqlite_close(void) {

	if (!sqSection.dbSqlLite) return TRUE;
	sqlite3_close(sqSection.dbSqlLite);
	sqSection.dbSqlLite=NULL;
	ehFreePtr(&sqSection.pszQueryBuffer);
	return FALSE;
}


//
// sqlite_count() - Ritorna il numero di items
//
INT sqlite_count(CHAR *Mess,...)
{
	INT iRecNum=-1;
	va_list Ah;
	CHAR *lpQueryMess=ehAlloc(1024);
	CHAR *lpQueryCommand=ehAlloc(SQL_BUFFER_QUERY_DEFAULT);
	EH_SQLITE_RS rsSet;

	sprintf(lpQueryMess,"?SELECT COUNT(*) FROM %s",Mess);
	va_start(Ah,Mess);
	vsprintf(lpQueryCommand,lpQueryMess,Ah); // Messaggio finale
	va_end(Ah);

	if (sqlite_query(lpQueryCommand)) 
	{
		 ehFree(lpQueryCommand);
		 ehFree(lpQueryMess); 
		 return iRecNum;
	}

	rsSet=sqlite_store(); // Richiedo il risultato
	while (sqlite_fetch(rsSet))
	{
		iRecNum=sqlite_int(rsSet,"1");	break;
	}
	sqlite_free(rsSet); // Libero le risorse

    ehFree(lpQueryCommand);
	ehFree(lpQueryMess); 
	return iRecNum;
}

static int _callback(void *NotUsed, int argc, char **argv, char **azColName)
{
  int i;
  for(i=0; i<argc; i++)
  {
	 win_infoarg("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  return 0;
}

//
// sqlite_query()
// return 0=Tutto ok
//#define SQLITE_RETRY 5
INT sqlite_query(CHAR * pQuery)
{
	BOOL	bRetry=true;
	BOOL	bError=false;
	char *	zErrMsg = NULL;
	BOOL	bNoErrorStop=FALSE;
	EH_SQLITE_SECTION *psqSection=&sqSection;
	BYTE *	pQuery8;
	INT		a;
	DWORD	tStart;

	if (!sqSection.dbSqlLite) 
		ehError();
	if (*pQuery=='?') {bNoErrorStop=TRUE; pQuery++;}
	pQuery8=strEncode(pQuery,SE_UTF8,NULL);
	tStart=clock();
	for (a=0;;a++) {

		if (!strCaseBegin(pQuery,"SELECT ")) {

			CHAR *pzsQuery=ehAlloc(strlen(pQuery8)+100);
			strcpy(pzsQuery,pQuery8);
			psqSection->iLastError=sqlite3_prepare_v2(
									  sqSection.dbSqlLite,            
									  pzsQuery,  // Dovrebbe essere UTF-8
									  -1,             
									  &sqSection.psStantment,  
									  NULL //const char **pzTail     
									  );
			ehFree(pzsQuery);
		}
		else {
			psqSection->iLastError=sqlite3_exec(sqSection.dbSqlLite, pQuery8, _callback, 0, &zErrMsg);
		}

		//
		// Controllo degli errori
		//

		switch (psqSection->iLastError) {
		
			case SQLITE_OK: 
			case SQLITE_ROW: // Ho le righe
				bError=false; 
				bRetry=false;
				break;

			case SQLITE_IOERR: // 10
				bError=TRUE;
				break;

			case SQLITE_LOCKED:
			case SQLITE_BUSY:
//				printf("\7 > %s" CRLF,sqlite_QueryLastError(pQuery));
#ifdef _DEBUG
				printf(" > sqlite_query (%d): Attendere ..." CRLF,psqSection->iLastError);
#endif
				bRetry=true;
				break;

			default:
				bError=true; 
				bRetry=false;
				break;
		
		}

		if (!bRetry) break; 

		// Raggiunto Time out
		if (((clock()-tStart)/CLOCKS_PER_SEC)>psqSection->dwQueryTimeout) break;
//		ehSleep(500);
	}
	ehFree(pQuery8);

	if (bError) //psqSection->iLastError!=SQLITE_OK ) 
	{
		if (!bNoErrorStop) 
		{
	#ifdef _DEBUG
			printf("[%s]",sqlite_QueryLastError(pQuery));
	#endif
			ehExit(CRLF "mys_query(): Sql error: %s",sqlite_QueryLastError(pQuery));
		}
	}

	return psqSection->iLastError;
}

//
// sqlite_queryarg()
//
int sqlite_queryarg(CHAR *pszMess,...) {

	EH_SQLITE_SECTION *psqSection=&sqSection;
	va_list Ah;
	va_start(Ah,pszMess);
	if (!psqSection->dbSqlLite) ehError();

	vsprintf(psqSection->pszQueryBuffer,pszMess,Ah); // Messaggio finale
	va_end(Ah);
	if (strlen(psqSection->pszQueryBuffer)>SQL_BUFFER_QUERY_DEFAULT) ehExit(__FUNCTION__ "() too big : %s max(%d b)",psqSection->pszQueryBuffer,SQL_BUFFER_QUERY_DEFAULT);
	return sqlite_query(psqSection->pszQueryBuffer);

}

//
// sqlite_queryargBig()
//
INT sqlite_queryargBig(DWORD dwSize, CHAR *pszMess,...) {

	EH_SQLITE_SECTION *psqSection=&sqSection;
	CHAR * pszBuffer=ehAlloc(dwSize);
	INT iRet;
	va_list Ah;
	va_start(Ah,pszMess);
	if (!psqSection->dbSqlLite) ehError();

	vsprintf(pszBuffer,pszMess,Ah); // Messaggio finale
	va_end(Ah);
	iRet=sqlite_query(pszBuffer);
	ehFree(pszBuffer);
	return iRet;

}

//
// sqlite_queryarg()
//
EH_SQLITE_RS sqlite_queryrow(CHAR *pszMess,...) {

	EH_SQLITE_RS rsSet;
	EH_SQLITE_SECTION * psqSection=&sqSection;

	va_list Ah;

	if (!psqSection->dbSqlLite) ehError();
	va_start(Ah,pszMess);
	vsprintf(psqSection->pszQueryBuffer,pszMess,Ah); // Messaggio finale
	va_end(Ah);
	if (strlen(psqSection->pszQueryBuffer)>SQL_BUFFER_QUERY_DEFAULT) ehExit("sqlite_queryrow() too big : %s",psqSection->pszQueryBuffer);
	strcat(psqSection->pszQueryBuffer," LIMIT 1");

	if (sqlite_query(psqSection->pszQueryBuffer)) 
		return NULL;

	rsSet=sqlite_store(); // Richiedo il risultato
	if (rsSet) {
		while (sqlite_fetch(rsSet)) {
			return rsSet;
		}
	}
	sqlite_free(rsSet);
	return NULL;
}


//
// sqlite_queryFieldBuilder()
//
static void sqlite_queryFieldBuilder(EH_SQLITE_RS rsSet) {

	INT i;
	CHAR *p;
	if (rsSet->arFields) ehError();
	rsSet->iFields=sqlite3_column_count(rsSet->psStmt);
	rsSet->arFields=ehAllocZero(sizeof(EH_SQLITE_FIELD)*rsSet->iFields);

	for (i=0;i<rsSet->iFields;i++) {

//		BYTE *pName=(BYTE *) sqlite3_column_name(rsSet->psStmt, i);
		BYTE *pType=(BYTE *) sqlite3_column_decltype(rsSet->psStmt, i);
		strcpy(rsSet->arFields[i].szName,(BYTE *) sqlite3_column_name(rsSet->psStmt, i));
		if (pType) {
			strcpy(rsSet->arFields[i].szType,pType);
			p=strExtract(pType,"(",")",FALSE,FALSE);
			if (p) {rsSet->arFields[i].iSize=atoi(p); ehFree(p);}

			if (strstr(pType,"INTEGER"))
			{
				rsSet->arFields[i].iType=_INTEGER;
			}
			else if (strstr(pType,"TEXT"))
			{
				rsSet->arFields[i].iType=_ALFA;
				if (!rsSet->arFields[i].iSize) rsSet->arFields[i].iType=_TEXT;

			}

			else if (strstr(pType,"BLOB"))
			{
				rsSet->arFields[i].iType=_TEXT;
			}

			else if (strstr(pType,"REAL"))
			{
				rsSet->arFields[i].iType=_NUMBER;
			}
		}
	}
}


//
// mys_store_result()
//
EH_SQLITE_RS sqlite_store(void)
{
	EH_SQLITE_SECTION *psqSection=&sqSection;
	EH_SQLITE_RS rsSet=NULL;

	if (psqSection->psStantment) 
	{
		rsSet=ehAllocZero(sizeof(EH_SQLITE_RESULTSET));
		rsSet->psqSection=psqSection;
		rsSet->psStmt=psqSection->psStantment;
		sqlite_queryFieldBuilder(rsSet);
		rsSet->iRows=0; // Estratto nella prima riga
		if (rsSet->iFields) 
		{
			rsSet->arRow=ehAllocZero(sizeof(BYTE *)*rsSet->iFields);
		}
	}
	return rsSet;
}

//
// Crea_SQLite_query()
//
BOOL sqlite_fetch(EH_SQLITE_RS rsSet)
{
	INT a;
	BOOL bBreak=FALSE;
	DWORD tStart;

	tStart=clock();
	while (true) {

		rsSet->psqSection->iLastError=sqlite3_step(rsSet->psStmt); 
		if (rsSet->psqSection->iLastError==SQLITE_DONE) return FALSE;

		switch (rsSet->psqSection->iLastError)
		{
			case SQLITE_DONE:	return FALSE;
			case SQLITE_BUSY:	break;
			case SQLITE_ERROR:  return FALSE;

			case SQLITE_ROW:
				for (a=0;a<rsSet->iFields;a++) {
					  strAssign(&rsSet->arRow[a],(BYTE *) sqlite3_column_text(rsSet->psStmt, a));
				  }
				rsSet->iCurrent++;		
				return TRUE;

			default:
				break;
		}

		// Raggiunto Time out
		if (((clock()-tStart)/CLOCKS_PER_SEC)>rsSet->psqSection->dwQueryTimeout) break;
	//	ehSleep(500);
#ifdef _DEBUG
		printf("> fetch: retry ... " CRLF);
#endif

	}
	return FALSE; // Timeout
}

void sqlite_free(EH_SQLITE_RS rsSet)
{
	INT a;
	if (!rsSet) return;

	for (a=0;a<rsSet->iFields;a++) {
		ehFreeNN(rsSet->arRow[a]); //psqSection->arRow[a]=NULL;
	}
	ehFreePtr(&rsSet->arFields);
	sqlite3_finalize(rsSet->psStmt); 
	ehFreeNN(rsSet->arRow); //psqSection->arRow=NULL;
	memset(rsSet,0,sizeof(EH_SQLITE_RS));
	ehFree(rsSet);
}

BYTE * sqlite_QueryLastError(CHAR *pQuery)
{
	EH_SQLITE_SECTION *psqSection=&sqSection;
	static CHAR szError[2048];
	sprintf(szError,"Error:%d - %s | ",
		sqlite3_errcode(psqSection->dbSqlLite),
		(BYTE *) sqlite3_errmsg(psqSection->dbSqlLite));
	if (pQuery) strCpy(strNext(szError),pQuery,1024);
	return szError;

//	return (BYTE *) sqlite3_errmsg(psqSection->dbSqlLite);
}

INT sqlite_fldfind(EH_SQLITE_RS rsSet,CHAR * pColumnName) {

	INT i,idx;
	idx=-1;
	for (i=0;i<rsSet->iFields;i++) {
		if (!_stricmp(rsSet->arFields[i].szName,pColumnName)) {
			idx=i; break;	
		}
	}
	return idx;
}

//
// sqlite_ptr()
//
BYTE * sqlite_ptr(EH_SQLITE_RS rsSet,CHAR * pColumnName) {

	INT idx;

	//
	// Ricerco per nome
	//
	idx=sqlite_fldfind(rsSet,pColumnName);

	if (idx<0) idx=atoi(pColumnName)-1; 
	if (idx<0) ehExit("sqlite: %s field not found",pColumnName);
	if (idx>=rsSet->iFields) ehError();
	return rsSet->arRow[idx];

}


INT		sqlite_int(EH_SQLITE_RS rsSet,CHAR * pColumnName) {
	BYTE *ptr;
	ptr=sqlite_ptr(rsSet,pColumnName); if (!ptr) ehError();
	return atoi(ptr);
}

DOUBLE	sqlite_num(EH_SQLITE_RS rsSet,CHAR * pColumnName) {
	BYTE *ptr;
	ptr=sqlite_ptr(rsSet,pColumnName); if (!ptr) ehError();
	return atof(ptr);
}

//
// sqlite_max() - Ritorna il numero di items
//
DOUBLE sqlite_max(CHAR *Campo,CHAR *Tabella)
{
	double dMaxNum=-1;
	CHAR *lpQueryCommand=ehAlloc(SQL_BUFFER_QUERY_DEFAULT);
	EH_SQLITE_SECTION *psqSection=&sqSection;

	sprintf(lpQueryCommand,"SELECT MAX(%s) FROM %s",Campo,Tabella);
	psqSection->iLastError=sqlite3_prepare_v2(
							  sqSection.dbSqlLite,            
							  lpQueryCommand,  // Dovrebbe essere UTF-8
							  -1,             
							  &sqSection.psStantment,  
							  NULL //const char **pzTail     
							  );

	if (sqlite3_step (sqSection.psStantment)==SQLITE_ROW)
		dMaxNum=sqlite3_column_double (sqSection.psStantment,0);
	sqlite3_finalize (sqSection.psStantment);
	return dMaxNum;
}

/*
int QuerySample(void)
{
  sqlite3 *dbHandle;
  sqlite3_stmt *psStm;
  char *zErrMsg = 0;
  int rc;
  INT iCount=0;
  BYTE *pQuery;
  INT a,iCol;

  printf("Query sample");
  // Apro
  rc = sqlite3_open("c:\\Gimba2008\\archivi\\G2008.sqlite", &dbHandle);
  if( rc ){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(dbHandle));
    sqlite3_close(dbHandle);
    exit(1);
  }

// Create the object using sqlite3_prepare_v2() or a related function. 
  //
  // Preparo la richiesta
  //
	rc=sqlite3_prepare_v2(
						  dbHandle,            
						  "SELECT * FROM variabili",      
						  -1,             
						  &psStm,  
						  NULL//const char **pzTail      
						  );
	// LIMIT 5 OFFSET 3

  if( rc!=SQLITE_OK ){
    fprintf(stderr, "Can't sqlite3_prepare: %s\n", sqlite3_errmsg(dbHandle));
    sqlite3_close(dbHandle);
    exit(1);
  }

  iCol=sqlite3_column_count(psStm);
  printf("Colonne presenti: %d" CRLF,iCol);
  for (a=0;a<iCol;a++)
  {
	  printf("Colonna %d - %s - Tipo: %s" CRLF,
			a,
			sqlite3_column_name(psStm, a),
			sqlite3_column_decltype(psStm, a));        
  }

  //SQLITE_INTEGER  
  while ( TRUE ) 
  {
	BOOL bBreak=FALSE;
	rc=sqlite3_step(psStm); if (rc==SQLITE_DONE) break;
	printf("rc=%d" CRLF,rc);
	_getch();

	switch (rc)
	{
		case SQLITE_BUSY:
			continue;

		case SQLITE_ROW:
			  for (a=0;a<iCol;a++)
			  {
				printf("%d| %s=[%s] Len=%d, Tipo=%d" CRLF,
						a,
						sqlite3_column_name(psStm, a),
						sqlite3_column_text(psStm, a),
						sqlite3_column_bytes(psStm, a),
						sqlite3_column_type(psStm, a));
			  }
			break;

		default:
		case SQLITE_ERROR:
			fprintf(stderr, "Can't sqlite3_prepare: %s\n", sqlite3_errmsg(dbHandle));
			bBreak=TRUE;
			break;
	}
	if (bBreak) break;

	//iCount++;printf("%d) qui %d [%s]" CRLF,iCount,rc,sqlite3_column_name(psStm, 0));
	//sqlite3_reset(psStm);
  }

  sqlite3_finalize(psStm); 

  // Chiudo
  sqlite3_close(dbHandle);
  return 0;
}
*/
 