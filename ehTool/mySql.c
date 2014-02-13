//   +-------------------------------------------+
//   | mySql
//   | Interfaccia al dbase mySql
//   |
//   |							  Ferrà srl 2006
//   +-------------------------------------------+

//
// 9.10 del 23/11/2006 ... ma vviieni (è dalle 6 che ci sono dietro ca..o)
//

#include "/easyhand/inc/easyhand.h"
/*
#ifndef EH_SQL_MYSQL
#include "/easyhand/inc/mySql.h"
#endif
*/

#if defined(_MYSQL)&&defined(__windows__)
#pragma comment(lib, "/easyhand/EhToolx/MySQL Server 5.0/lib/opt/libmysql.lib")
#endif

#define MYS_FILE 1

#include "/easyhand/ehtoolx/MySQL Server 5.0/include/errmsg.h"
#define SQL_BUFFER_QUERY_DEFAULT 8192

//
// Macro senza support MultiThread
//
#ifndef _MYSQL_MT
S_MYSQL_SECTION sMYS={NULL,0};
#else
struct {
	S_MYSQL_SECTION arMysSection[MAX_MTSECTION];
	BOOL			bLogThread;
//static CRITICAL_SECTION csMysSection;
	HANDLE			mtxMemo;
} _sLocal;
#endif

void mys_debug(DYN_SECTION_FUNC BOOL bEnable) {DYN_SECTION_GLOBALPTR mySection->bDebug=bEnable;}

//
// mys_start()
//
void mys_start(INT iStep)
{
	if (iStep!=WS_LAST) return;
	mysql_library_init(0,NULL,NULL);

#ifndef _MYSQL_MT
	mys_CreateSection(&sMYS);
#else
	ZeroFill(_sLocal.arMysSection);
	_sLocal.bLogThread=false; // 
	_sLocal.mtxMemo=CreateMutex(NULL,FALSE,NULL);
//	InitializeCriticalSection(&csMysSection);
//	mysql_thread_init();
#endif
}

//
// mys_end()
//
void mys_end(INT iStep)
{
	if (iStep!=WS_LAST) return; 
#ifndef _MYSQL_MT
	mys_Deconnect();//(&sMYS);
//	mysql_library_end();
#else
	mys_MTCloseAllSection();
	CloseHandle(_sLocal.mtxMemo); _sLocal.mtxMemo=NULL;
//	DeleteCriticalSection(&csMysSection);
	
#endif
	mysql_library_end();

}

#ifdef _MYSQL_MT
void mys_logThread(BOOL bFlag) {
	_sLocal.bLogThread=bFlag;
}
#endif

//
// mys_Connect()
//
MYSQL * mys_Connect(DYN_SECTION_FUNC CHAR *pSQLServer,
					CHAR *pSQLUser,
					CHAR *pSQLPassword,
					CHAR *pDbaseSchema,
					ULONG client_flag,
					CHAR * pszCharSet)
{
	DYN_SECTION_GLOBALPTR

	//MYSQL *mys;
	mySection->pSqlHost=strDup(pSQLServer);
	mySection->pSqlUser=strDup(pSQLUser);
	mySection->pSqlPass=strDup(pSQLPassword);
	mySection->pSqlSchema=strDup(pDbaseSchema);
	mySection->iSqlClientFlag=client_flag;
	mySection->psRS=NULL;

	if (!mySection->mySql) ehError();
	if (!mysql_real_connect(mySection->mySql,//mySection->mySql,
							pSQLServer,
							pSQLUser,
							pSQLPassword,
							pDbaseSchema,
							0,
							NULL,
							client_flag)) return NULL;

	if (pszCharSet) {
		if (mysql_set_character_set(mySection->mySql, pszCharSet)) ehError();
	}
	return mySection->mySql;
}

//
// mys_Deconnect()
//
BOOL mys_Deconnect(DYN_SECTION_FUN)
{
	DYN_SECTION_GLOBALPTR
	mys_DestroySection(mySection);
	return FALSE;
}

MYSQL *mys_Reconnect(DYN_SECTION_FUNC INT iTent,INT iErr)
{

	DYN_SECTION_GLOBALPTR

	MYSQL *mys;
	if (mySection->funcExtNotify) (*mySection->funcExtNotify)(&mySection,iTent,iErr);
	mys=mysql_real_connect(mySection->mySql,
							mySection->pSqlHost,
							mySection->pSqlUser,
							mySection->pSqlPass,
							mySection->pSqlSchema,
							0,
							NULL,
							mySection->iSqlClientFlag);
	return mys;
}

//
// mys_CreateSection()
//
void mys_CreateSection(S_MYSQL_SECTION *pmySection)
{
	memset(pmySection,0,sizeof(S_MYSQL_SECTION));
	if (!(pmySection->mySql=mysql_init(pmySection->mySql))) ehExit("mysql_init() failed");
	pmySection->pszQueryBuffer=ehAllocZero(SQL_BUFFER_QUERY_DEFAULT);
	pmySection->bDebug=FALSE;
	pmySection->dwQueryCounter=0;
}

//
// mys_DestroySection()
//
void mys_DestroySection(S_MYSQL_SECTION *mySection)
{
	ehFreePtr(&mySection->pszQueryBuffer);	// Buffer standard
	ehFreePtr(&mySection->pszQueryLast);	// Copia dell'ultima query
	if (!mySection->pSqlHost) return;
	mysql_close(mySection->mySql); // Libero le risorse
	ehFreePtr(&mySection->pSqlHost);
	ehFreePtr(&mySection->pSqlUser);
	ehFreePtr(&mySection->pSqlPass);
	ehFreePtr(&mySection->pSqlSchema);
	memset(mySection,0,sizeof(S_MYSQL_SECTION));
}

const char *mys_GetError(DYN_SECTION_FUN)
{ 
	DYN_SECTION_GLOBALPTR
	return mysql_error(mySection->mySql);
}

//
// Conta i record secondo la query
//
INT mys_count(DYN_SECTION_FUNC CHAR * pszFormat,...)
{
	DYN_SECTION_GLOBALPTR

	INT iRecNum=-1;
	MYSQL_RES *result;
	MYSQL_ROW row;
	INT		iLen;
	va_list Ah;
	INT		iMaxBuffer=SQL_BUFFER_QUERY_DEFAULT;
	CHAR *	pszQuery=ehAlloc(SQL_BUFFER_QUERY_DEFAULT);

	va_start(Ah,pszFormat);
	//vsprintf(lpQueryCommand,lpQueryMess,Ah); // Messaggio finale
	iLen=vsnprintf(pszQuery,SQL_BUFFER_QUERY_DEFAULT-1,pszFormat,Ah); // Messaggio finale
	if (iLen<0) ehError();
	va_end(Ah);
	strIns(pszQuery,"SELECT COUNT(*) FROM ");

	mySection->dwQueryCounter++;
	if (mysql_query(mySection->mySql, pszQuery))
	{
		ehExit(CRLF "mys_count(): Sql error: %s",mys_QueryLastError(DYN_SECTIONC pszQuery));
		ehFree(pszQuery); // Errore
		/*
		return iRecNum;
		*/
	}

	result = mysql_store_result(mySection->mySql); // Richiedo il risultato
	iRecNum=0;
	while ((row = mysql_fetch_row (result)) != NULL)
	{
		if (row[0]) iRecNum=atoi(row[0]);
		break;
	}
	mysql_free_result(result); // Libero le risorse
    ehFree(pszQuery);
	return iRecNum;
}

//
// mys_sum()
// Somma il valore di un campo
//
INT mys_sum(DYN_SECTION_FUNC CHAR *lpField,CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	INT iRecNum=-1;
	MYSQL_RES *result;
	MYSQL_ROW row;
	va_list Ah;
	CHAR *lpQueryMess=ehAlloc(1024);
	CHAR *lpQueryCommand=ehAlloc(SQL_BUFFER_QUERY_DEFAULT);

	sprintf(lpQueryMess,"SELECT SUM(%s) FROM %s",lpField,Mess);
	va_start(Ah,Mess);
	vsprintf(lpQueryCommand,lpQueryMess,Ah); // Messaggio finale
	va_end(Ah);

	mySection->dwQueryCounter++;
	if (mysql_query(mySection->mySql, lpQueryCommand))
	{
		 ehFree(lpQueryCommand);
		 ehFree(lpQueryMess);
		 return iRecNum;
	}
	result = mysql_store_result(mySection->mySql); // Richiedo il risultato
	while ((row = mysql_fetch_row (result)) != NULL)
	{
		if (!row[0])
			iRecNum=0;
			else
			iRecNum=atoi(row[0]);
		break;
	}
	mysql_free_result(result); // Libero le risorse
    ehFree(lpQueryCommand);
	ehFree(lpQueryMess);
	return iRecNum;
}


//
//  mys_QueryLastError()
//
CHAR * mys_QueryLastError(DYN_SECTION_FUNC CHAR *lpQuery)
{
	DYN_SECTION_GLOBALPTR
	static CHAR szError[2048];
	sprintf(szError,"Error:%d - %s | ",mysql_errno(mySection->mySql),mysql_error(mySection->mySql));
	strCpy(strNext(szError),lpQuery,1024);
	return szError;
}
//
// mys_query() -> (Ritorna False se tutto ok)
//
int mys_query(DYN_SECTION_FUNC CHAR *pQuery)
{
	DYN_SECTION_GLOBALPTR
	int a,err;
	BOOL bNoErrorStop=FALSE;
	CHAR * pRealQuery,*pszQuery;
	mySection->dwQueryCounter++;

	if (strEmpty(pQuery)) ehError();

#ifdef _CONSOLE
	if (mySection->bDebug) {ehPrintf("%s" CRLF,pQuery);}
#else
	if (mySection->bDebug) {ehPrint("%s" CRLF,pQuery);}
#endif
	pRealQuery=pQuery;
	if (mySection->pSqlSchema) {

		if (strstr(pQuery,"{LIB}")) // modifica per Crea importazione ehBook
		{
			pRealQuery=ehAlloc(strlen(pQuery)+1024); 
			strcpy(pRealQuery,pQuery);
			while (strReplace(pRealQuery,"{LIB}",mySection->pSqlSchema));
		}
	}
	pszQuery=pRealQuery;
	if (*pszQuery=='?') {bNoErrorStop=TRUE; pszQuery++;}
	strAssign(&mySection->pszQueryLast,pszQuery);
	if (!mySection->bReconnect)
		{
			err=mysql_query(mySection->mySql, pszQuery);
		}
		else
		{
			for (a=0;a<mySection->iReconnectTry;a++) // Promo iReconnectTry volte prima di chiuedere
			{
				err=mysql_query(mySection->mySql, mySection->pszQueryLast); if (!err) break;
				if (mysql_errno(mySection->mySql)==2006||mysql_errno(mySection->mySql)==2013) mys_Reconnect(DYN_SECTIONC a,err); else break;
			}
		}
/*
#ifdef _DEBUG
	if (err) {
		ehPrintf("ERROR: [%s]" CRLF,mys_QueryLastError(DYN_SECTIONC pQuery));
	}
#endif
	*/
	if (err&&!bNoErrorStop) {

		ehExit(CRLF "mys_query(): Sql error: %s",mys_QueryLastError(DYN_SECTIONC pQuery));

	}
	if (pRealQuery!=pQuery) 
		ehFree(pRealQuery);

	return err;
}

//
// mys_queryarg()
//
int mys_queryarg(DYN_SECTION_FUNC CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	INT64 iTest;
	va_list Ah;
	iTest=(INT64) Mess;
	va_start(Ah,Mess);

	if (iTest==0||iTest==1) {
		BYTE *pStr=va_arg(Ah,BYTE *);
		ehExit("mys_queryarg() invocato con TRUE/FALSE: query:[%s]",pStr);
	}

	vsprintf(mySection->pszQueryBuffer,Mess,Ah); // Messaggio finale
	va_end(Ah);
	if (strlen(mySection->pszQueryBuffer)>=SQL_BUFFER_QUERY_DEFAULT) ehExit("mys_queryarg() too big : %s max(%d b)",mySection->pszQueryBuffer,SQL_BUFFER_QUERY_DEFAULT);
	return mys_query(DYN_SECTIONC mySection->pszQueryBuffer);
}

//
// mys_queryrow()
// Legge una sola riga (query+fetch)
//
EH_MYSQL_RS mys_queryrow(DYN_SECTION_FUNC CHAR *Mess,...)
{
	DYN_SECTION_GLOBALPTR
	EH_MYSQL_RS rsSet;

	va_list Ah;
	va_start(Ah,Mess);
	vsprintf(mySection->pszQueryBuffer,Mess,Ah); // Messaggio finale
	va_end(Ah);
	if (strlen(mySection->pszQueryBuffer)>SQL_BUFFER_QUERY_DEFAULT) ehExit("mys_queryrow() too big : %s",mySection->pszQueryBuffer);
	strcat(mySection->pszQueryBuffer," LIMIT 1");

	if (mys_query(DYN_SECTIONC mySection->pszQueryBuffer)) return NULL;

#ifndef _MYSQL_MT
	rsSet=mys_store_result(); // Richiedo il risultato
#else
	rsSet=mys_store_result(DYN_SECTION); // Richiedo il risultato
#endif
	if (rsSet)
	{
		while (mys_fetch_row(rsSet))
		{
			return rsSet;
		}
	}
	return NULL;
}

int mys_queryargBig(DYN_SECTION_FUNC DWORD dwSizeMemory,CHAR *pszFormat,...)
{
//	DYN_SECTION_GLOBALPTR
	int iRet;
	CHAR *lpBuf=ehAlloc(dwSizeMemory);
	va_list Ah;

	va_start(Ah,pszFormat);
	//vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	iRet=vsnprintf(lpBuf,dwSizeMemory-1,pszFormat,Ah); // Messaggio finale
	if (iRet<0) {
		ehPrintf("%s:overflow [%s] [size:%d]" CRLF, __FUNCTION__, pszFormat, (INT) dwSizeMemory);
		ehFree(lpBuf);
		// Provo a vedere il risultato
		dwSizeMemory=1000000;
		iRet=vsnprintf(lpBuf,dwSizeMemory-1,pszFormat,Ah); // Messaggio finale
		if (iRet) ehPrintf("%s" CRLF,lpBuf);
		ehError();
	}
	va_end(Ah);

	iRet=mys_query(DYN_SECTIONC lpBuf);
	ehFree(lpBuf);
	return iRet;
}

/*
{
	va_list Ah;
	CHAR *pBuffer=NULL;
	DWORD dwSize=2048;
	INT iRet;

	va_start(Ah,pString);
	while (TRUE)
	{
		pBuffer=ehAlloc(dwSize);
		iRet=vsnprintf(pBuffer,dwSize-1,pString,Ah); // Messaggio finale
		if (iRet>-1) break;
		dwSize+=2048;
		if (dwSize>1000000) ehError();
		ehFree(pBuffer);
	}

	ARAdd(pAr,pBuffer);
	ehFree(pBuffer);
	va_end(Ah);
}
*/






// mys_f MySqlField
// Operazione su i campi di una query
// mys_ffind() ricerca
//int mys_ffind(DYN_SECTION_FUNC CHAR *lpField,BOOL bNoError)
int mys_fldfind(EH_MYSQL_RS rsSet,CHAR *lpField,BOOL bNoError)
{
//	DYN_SECTION_GLOBALPTR
	int i;
    for (i=0; i<rsSet->iFields; i++)
	{
		if (!strcmp(lpField,rsSet->arFields[i].name)) return i;
	}
	if (atoi(lpField)>0) return atoi(lpField)-1; // New
	if (!bNoError) 
		ehExit("mys_fldfind(): [%s] non trovato.\nQuery:\n%s",lpField,rsSet->psSection->pszQueryLast);
	return -1;
}


//
// mys_find()
//
	/*
int I_mys_find(MYSQL_RES *myRes,CHAR *lpField,BOOL bNoError)
{
	DYN_SECTION_GLOBALPTR
	int i;
	MYSQL_FIELD *field;
	INT iNum=mysql_num_fields(myRes);

	for (i=0; i<iNum; i++)
	{
		field=mysql_fetch_field_direct(myRes, i);
		if (!strcmp(field->name,lpField)) return i;
	}
	if (atoi(lpField)>0) return atoi(lpField)-1; // New
	ehExit("mys_find(): Campo inesistente [%s]",lpField);
	return -1;
}
	*/

//INT mys_fldfind(EH_MYSQL_RS psRS,CHAR *lpField) {return I_mys_find(psRS->myRes,lpField);}


// myf_ptr() Puntatore a

//
// mys_fldptr()
//
CHAR * mys_fldptrEx(EH_MYSQL_RS psRS,CHAR * lpField,CHAR * pDefault)
{
	INT i=mys_fldfind(psRS,lpField,FALSE); if (i<0) return pDefault;
	if (!psRS->myRow) ehError(); // Controllare sei hai fatto la fetch..
	if (!psRS->myRow[i]) return pDefault;
	return psRS->myRow[i];
}

//
// mys_fldlen()
//
SIZE_T	mys_fldlen(EH_MYSQL_RS rsSet,CHAR * lpField) {
	//SIZE_T iLen=0;
	long * arLen;
	
	INT i=mys_fldfind(rsSet,lpField,FALSE); if (i<0) ehExit("mys_fldlen(): %s ?",lpField);
	arLen = mysql_fetch_lengths(rsSet->myRes);
//	iLen=rsSet->arFields[i].max_length;
	return arLen[i];
}

#ifdef EH_MEMO_DEBUG
EH_MYSQL_RS mys_store_result_dbg(DYN_SECTION_FUNC CHAR *pFile,INT iLine)
#else
EH_MYSQL_RS mys_store_result(DYN_SECTION_FUN)
#endif
{
	DYN_SECTION_GLOBALPTR

#ifdef EH_MEMO_DEBUG
	EH_MYSQL_RS psRS=_ehAlloc(sizeof(EH_MYSQL_RESULTSET),TRUE,pFile,iLine);
#else
	EH_MYSQL_RS psRS=ehAllocZero(sizeof(EH_MYSQL_RESULTSET));
#endif

	BOOL bClean=FALSE;
	psRS->psSection=mySection; // new 2010
	psRS->myRes=mysql_store_result(mySection->mySql); // Richiedo il risultato
	if (psRS->myRes)
	{
		psRS->iFields=mysql_num_fields(psRS->myRes);
		psRS->iRows=(int) mysql_num_rows(psRS->myRes);
		if (!psRS->iRows)
			{mysql_free_result(psRS->myRes); bClean=TRUE;}
			else
			{
				psRS->arFields = mysql_fetch_fields(psRS->myRes); // new 2009
			}
	}
	else
	{
		bClean=TRUE;//ehFree(psRS); return NULL;
	}
	if (bClean) ehFreePtr(&psRS);
	return psRS;
}

//
// mys_fetch_row()
//
BOOL mys_fetch_row(EH_MYSQL_RS psRS)
{
	if (!psRS) return FALSE;
	psRS->myRow=mysql_fetch_row(psRS->myRes);
	psRS->iCurrent++;
	if (psRS->myRow) return TRUE; else return FALSE;
}
//
// mys_free_result()
//

void * mys_free_result(EH_MYSQL_RS psRS)
{
	if (!psRS) return NULL;
	mysql_free_result(psRS->myRes); memset(psRS,0,sizeof(EH_MYSQL_RESULTSET));
	ehFree(psRS);
	return NULL;
}

//
// mys_lastid()
//
DWORD mys_lastid(DYN_SECTION_FUN) {

	DYN_SECTION_GLOBALPTR
	return (INT) mysql_insert_id(mySection->mySql); // Evento sospeso da chiudere (se si registra l'utente)

}


#ifndef _MYSQL_MT
//// mys -> adb funzioni simili in sostituzione per tenere doppia compatibilità
int    mys_FldFind(int hdb,CHAR *lpField) {return mys_fldfind(sMYS.psRS,lpField,FALSE);}
char * mys_FldPtr(int hdb,CHAR *lpField)  {return mys_fldptr(sMYS.psRS,lpField);}
int    mys_FldInt(int hdb,CHAR *lpField)  {return mys_fldint(sMYS.psRS,lpField);}
double mys_FldNume(int hdb,CHAR *lpField) {return mys_fldnum(sMYS.psRS,lpField);}
void  *mys_BlobGetAlloc(int hdb,CHAR *lpField) {return strDup(mys_fldptr(sMYS.psRS,lpField));}
void   mys_BlobFree(void *lPointer) {ehFree(lPointer);}
char * mys_GetDate(int hdb,CHAR *lpField)
{
	CHAR *ptd;
	static CHAR szServ[20];
	ptd=mys_fldptr(sMYS.psRS,lpField); *szServ=0;
	if (!*ptd) return szServ;
	memcpy(szServ,ptd+6,2); // Anno
	memcpy(szServ+2,ptd+4,2); // Anno
	memcpy(szServ+4,ptd,4); // Anno
	szServ[8]=0;
	return szServ;
}
#endif


//
//	Gestione sezioni multiThread
//

#ifdef _MYSQL_MT

static void _mutexEnter(void) {
	WaitForSingleObject(_sLocal.mtxMemo,INFINITE); // _mutexEnter();
}
static void _mutexLeave(void) {
	if (!ReleaseMutex(_sLocal.mtxMemo)) {printf("_memoMultiThread mutex error"); exit(128);}
}

//
// mys_MTSectionId()
//
INT mys_MTSectionId(DWORD idThread,BOOL bCritical)
{
	INT a,idx=-1;
	if (bCritical) _mutexEnter();
	for (a=0;a<MAX_MTSECTION;a++)
	{
		if (_sLocal.arMysSection[a].idThread==idThread) {idx=a; break;}
	}
	if (bCritical) _mutexLeave();//_mutexLeave();
	return idx;
}


//
// mys_MTOpenSection()
//
S_MYSQL_SECTION * mys_MTOpenSection(DWORD idThread)
{
	INT idx;
	_mutexEnter();

	idx=mys_MTSectionId(0,FALSE); // Trovo una sezione libera
	if (idx<0) {_mutexLeave(); ehExit("mys_MTOpenSection(): full");}

	if (!idThread) idThread=GetCurrentThreadId();
	mys_CreateSection(&_sLocal.arMysSection[idx]);
	_sLocal.arMysSection[idx].idThread=idThread;

	_mutexLeave();//	_mutexLeave();

	return &_sLocal.arMysSection[idx];
}

//
// mys_MTGetSection()
//
S_MYSQL_SECTION *mys_MTGetSection(DWORD idThread)
{
	INT idx=mys_MTSectionId(idThread,TRUE);
	if (idx<0)
	{
		if (_sLocal.bLogThread) ehLogWrite("mys_MTGetSection(): #%d ?",idThread); 
		return NULL;
	}
	return &_sLocal.arMysSection[idx];
}

//
// mys_MTCloseSection()
// Ritorna true se la sezione non esiste
//
BOOL mys_MTCloseSection(DWORD idThread)
{
	INT idx=mys_MTSectionId(idThread,TRUE);
	if (idx<0)
	{
		if (_sLocal.bLogThread) ehLogWrite("mys_MTCloseSection(): #%d ?",idThread);
		return true;
	}
	_mutexEnter();
	mys_DestroySection(&_sLocal.arMysSection[idx]);
	_mutexLeave();
	return FALSE;
}

//
// mys_MTCloseAllSection()
//
void mys_MTCloseAllSection(void)
{
	INT a;
	_mutexEnter();
	for (a=0;a<MAX_MTSECTION;a++)
	{
		if (_sLocal.arMysSection[a].idThread)
		{	
			if (_sLocal.bLogThread) 
				ehLogWrite("MTSectionFree: Thread #%d (Query servite:%d)",
							_sLocal.arMysSection[a].idThread,
							_sLocal.arMysSection[a].dwQueryCounter);
			mys_DestroySection(&_sLocal.arMysSection[a]);
			_sLocal.arMysSection[a].idThread=0;
		}
	}
	_mutexLeave();
}

//
// mys_MTGetThreads()
//
CHAR *mys_MTGetThreads(void)
{
	INT a;
	CHAR *lpStatus=ehAlloc(32000); *lpStatus=0;
	_mutexEnter();
	for (a=0;a<MAX_MTSECTION;a++)
	{
		if (_sLocal.arMysSection[a].idThread)
		{
			if (*lpStatus) strcat(lpStatus,",");
			sprintf(lpStatus+strlen(lpStatus),"%d",_sLocal.arMysSection[a].idThread);
		}
	}
	_mutexLeave();
	return lpStatus;
}

 
#endif

//
// mysToJson()
//
EH_JSON * mysToJson(EH_MYSQL_RS rsSet) {

	EH_LST lstFld=lstNew();
	EH_JSON * psJson;
	CHAR * psz;
	int i;
    for (i=0; i<rsSet->iFields; i++)
	{
//		if (!strcmp(lpField,rsSet->arFields[i].name)) return i;
		psz=strEncode(strEver(rsSet->myRow[i]),SE_CFORMAT,NULL);
		lstPushf(lstFld,"\"%s\":\"%s\"",rsSet->arFields[i].name,psz);
		ehFree(psz);
	}

	psz=lstToString(lstFld,",","{","}");
	psJson=jsonCreate(psz);
	ehFree(psz);
	lstDestroy(lstFld);
	return psJson;
}

//
// mysTimeZoneField()
//
// Es. mysql: date_format(convert_tz(TS_REV,<@CHAR 64><@CHAR 64>session.time_zone,'@@szTimeZone'),@@szDtFormat)
//
CHAR * mysTimeZoneField(CHAR * pszBuffer,
						INT iSizeBuffer,
						CHAR * pszFieldSource,
						CHAR * pszNameField, 
						CHAR * pszTimeZone, 
						CHAR * pszDateFormat) {

	if (!strCmp(pszDateFormat,"DT")) pszDateFormat="%Y%m%d_%H%i%s";
	if (!strEmpty(pszDateFormat)) {

		snprintf(	pszBuffer, iSizeBuffer,
					"date_format(convert_tz(%s,@@global.time_zone,'%s'),'%s') AS %s",
					pszFieldSource,
					pszTimeZone,
					pszDateFormat,
					pszNameField?pszNameField:pszFieldSource);
	}
	else {
		snprintf(	pszBuffer, iSizeBuffer,
					"convert_tz(%s,@@global.time_zone,'%s') AS %s",
					pszFieldSource,
					pszTimeZone,
					pszNameField?pszNameField:pszFieldSource);
	}
	return 	pszBuffer;

}


/*

	//	Istruzioni per ricodifica mySql

	a) Aggiungere EH_MYSQL_RS psRS;
	b) mys_store_result()									diventa		psRS=mys_store_result()
	c) while (mys_fetch_row(psRS))	diventa		while (mys_fetch_row(psRS))
	d) mys_fint("campo")	diventa mys_fldint(psRS,"campo")
	e) mys_fptr("campo")	diventa mys_fldptr(psRS,"campo")
	f) sql_free();	diventa sql_free(psRS);
	g) myRow,myRes diventa psRS
	h) mys_fldptr(psRS diventa mys_fldptr(psRS

	EH_MYSQL_RS *pRes
    odbc_queryarg(FALSE,"SELECT * FROM ANTA200F");
	pRes=mys_store_result(20); // Richiedo il risultato
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


	mys_queryarg("SELECT IDCODE,CODINT FROM fasce WHERE (IDCOMP=%d AND CODINT IN (%s))",ID_MOBY,lstCode);
	myRS=mysql_store_result(sMYS.mySql);
	while (mys_fetch_row(myRS))
	{
		CHAR *pIdCode=mys_geptr(myRS,"IDCODE");
		CHAR *pCodInt=mys_getp(myRS,"CODINT");
		sprintf(szServ,"%s|%s",pIdCode,pCodInt);
	}
	sql_free(myRS);
	ehFree(lstCode);
*/
//
// mys_error()
//
/*
void mys_error(DYN_SECTION_FUN)
{s
	DYN_SECTION_GLOBALPTR
	ehExit("%s\n%s",mySection->lpQuery,mysql_error(mySection->mySql));
}
*/
//
//
//
/*
void process_result_set(MYSQL_RES *result)
{
	MYSQL_ROW row;
	unsigned int i;
	while ((row = mysql_fetch_row (result)) != NULL)
		{
			for (i = 0; i < mysql_num_fields (result); i++)
			{
				if (i > 0) fputc('\t', stdout);
				printf ("%s", row[i] != NULL ? row[i] : "NULL");
			}
			fputc ('\n', stdout);
		}
}
*/
