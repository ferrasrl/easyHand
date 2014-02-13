//   +-------------------------------------------+
//   | Eh_ODBC
//   | Interfaccia al dbase ODBC
//   |             
//   |							  Ferrà srl 2006
//   +-------------------------------------------+

#ifdef EH_ODBC_MT
#define EH_ODBC
#endif

#ifdef EH_ODBC

//
// Macro SENZA supporto MultiThread
//
/*
#ifndef EH_ODBC_MT

#define DYN_ODBC_SECTION_FUN
#define DYN_ODBC_SECTION_FUNC
#define DYN_ODBC_SECTION_GLOBALPTR EH_ODBC_SECTION * pOdbcSection=&sOdbcSection;
#define DYN_ODBC_SECTION 
#define DYN_ODBC_SECTIONC 
#define DYN_ODBC_SECTION_PTRC 
//
// Macro con supporto MultiThread
//
#else

#define DYN_ODBC_SECTION_FUN EH_ODBC_SECTION *pOdbcSection
#define DYN_ODBC_SECTION_FUNC EH_ODBC_SECTION *pOdbcSection,
#define DYN_ODBC_SECTION_GLOBALPTR
#define DYN_ODBC_SECTION sOdbcSection
#define DYN_ODBC_SECTIONC sOdbcSection,
#define DYN_ODBC_SECTION_PTRC pOdbcSection,

#endif
*/

#include <Odbcinst.h>
#include <Sql.h>
#include <Sqlext.h>
#include <Sqltypes.h>
#include <Sqlucode.h>
#include <Msdasql.h>
#include <Msdadc.h>

typedef enum {
	ODBC_PLATFORM_DB2=1,
	ODBC_PLATFORM_MYSQL
} EH_ODBC_PLATFORM;

typedef struct {
  CHAR szName[64];	// Nome del campo
  INT iType;		// Tipo ODBC
  EH_DATATYPE iAdbType; // Tipo Easyhand
  CHAR szType[10];
  INT iSize;
  CHAR * arBuffer;
  SQLLEN * arsLen;

} EH_ODBC_FIELD;

typedef enum {
	OP_UNKNOW,
	OP_SET_PLATFORM, // Indica quale piattaforma stiamo usando
	OP_RIGHT_TRIM	 // Indica che voglio trimmare gli spazi in coda dei campi
} EN_ODBC_PARAM;

#define EH_ODBC_FLD_SEPARATOR "\1"

//
// Struttura di Section
//
typedef struct {

	DWORD	idThread;		// Thread che ha in uso la connessione
	EH_ODBC_PLATFORM enPlatform;

	BOOL		bClone;	// SI: è una clonazione quindi non libero connessione 
	SQLHENV		hEnv;	// Ambiente
	SQLHDBC		hConn;	// Connessione

	SQLRETURN	sqlLastError; 
	SQLHSTMT	hStmt; // Stantment

	// Informazioni sul dbase
	SQLUINTEGER uiAsync;  // Info sulla possibilità di eseguire comandi Asincroni
	SQLUINTEGER uiScroll; // Info sulla possibilità di scroll
	SQLUINTEGER uiDynamic; // Caratteristiche del cursore dinamico

	BOOL fDebug; // Modalità Debug
	DWORD dwQueryCounter;

	CHAR *	lpQuery;		// Stringa di query (Spazio in memoria)
	CHAR *	pLastQuery;	// Puntatore all'ultima query fatta (per gli errori)

	BOOL	bNoErrorView;	// T/F se NON Voglio vedere gli errori
	void	(*funcExtNotify)(void *,INT iTry,INT iError);

	BYTE *	pSqlHost;
	BYTE *	pSqlUser;
	BYTE *	pSqlPass;
	BYTE *	pSqlSchema;
	INT		iSqlClientFlag;

//	CHAR	*pQueryOneRow;	//
	BOOL	bRightTrim;

} EH_ODBC_SECTION; // Sezione per le interrogazioni


//
// Struttura di Query / Results
//
typedef struct {

	EH_ODBC_SECTION *	psOdbcSection;
	SQLHSTMT			hStmt; // Stantment
	INT					iFieldNum;
	CHAR *				pszQuery;	// 2010 > Query che ha dato il risultato
	EH_ODBC_FIELD *		arDict;

	BOOL				bCursorOpen; // T/F se il cursore è aperto
	BOOL				bDataReady;	// T/F se i dati sono pronti
//	EH_AR row; // Array con la riga

	INT					iRowsLimit;	// Numero di righe lette per query
	INT					iOffset;
	INT					iMaxRows;		// Righe totali da leggere nella query
	INT					iRowsReady;	// Righe effettivamente lette
	INT					iCurrentRow;	// Puntatore alla linea di result da leggere
	BOOL				bCloseProtect;

} EH_ODBC_RESULTSET;

typedef EH_ODBC_RESULTSET * EH_ODBC_RS;

void odbc_start(INT iStep);
void odbc_end(INT iStep);

//#ifdef EH_ODBC_MT

//
// Gestione section 
//
EH_ODBC_SECTION *	odbcSectionGet(DWORD idThread);//,BOOL bCritical); // odbc_MTSectionId
EH_ODBC_SECTION *	odbcSectionCreate(DWORD idThread);		// odbc_MTOpenSection
BOOL				odbcSectionDestroy(DWORD idThread);		//	odbc_MTOpenSection 
//BOOL				odbc_MTCloseSection(DWORD idThread);	// odbc_MTCloseSection
void				odbcSectionCloseAll(void);			// odbc_MTCloseAllSection 
CHAR *				odbcSectionGetThreads(void);				// odbc_MTGetThreads
EH_ODBC_SECTION *	odbcSectionMain(void);
EH_ODBC_SECTION *	odbcSectionStmtClone(EH_ODBC_SECTION * psSection,INT idThread);
//#endif

//
// Funzioni semplificate senza il supporto del MultiThread
//
void				odbc_CreateSection(EH_ODBC_SECTION * pOdbcSection);
void				odbc_DestroySection(EH_ODBC_SECTION * mySection);


//
// MULTI-THREAD Function
//
void *				odbcConnect(EH_ODBC_SECTION * ps,
								 CHAR *pSQLHostName,
								 CHAR * pSQLUser,
								 CHAR * pSQLPassword,
								 CHAR * pDbaseSchema,
								 ULONG	client_flag,
								 BOOL	bShowWindow);

void				odbcDisconnect(EH_ODBC_SECTION * ps);
const char *		odbcGetError(EH_ODBC_SECTION * ps);
CHAR *				odbcError(EH_ODBC_SECTION * ps,CHAR *pWho,INT iMode);
INT					odbcCount(EH_ODBC_SECTION * ps,CHAR *Mess,...);
INT					odbcSum(EH_ODBC_SECTION * ps,CHAR *lpField,CHAR *Mess,...); // News 8/2007
INT					odbcQuery(EH_ODBC_SECTION * ps,CHAR *pQuery);
INT					odbcQueryf(EH_ODBC_SECTION * ps,CHAR *Mess,...);
INT					odbcQueryBig(EH_ODBC_SECTION * ps,DWORD dwSizeMemory,CHAR *Mess,...);

EH_AR				odbcQueryToArray(EH_ODBC_SECTION * ps,CHAR *Mess,...); // Ritorna un array con la prima riga della query
EH_ODBC_RS			odbcRow(EH_ODBC_SECTION * ps,CHAR *Mess,...);
int					odbcAffectedRows(EH_ODBC_SECTION * ps); // Ritorna il numero di righe modificate dopo un UPDATE,INSERT o DELETE
void				odbcSetParam(EH_ODBC_SECTION * ps,EN_ODBC_PARAM enParam,DWORD dwParam); // 2010
CHAR *				odbcLimit(EH_ODBC_SECTION * ps,DWORD dwOffset,DWORD dwLimit); // 2010
CHAR *				odbcTableInfo(EH_ODBC_SECTION * ps,CHAR * lpLibrary, CHAR * lpName, CHAR *lpType); // 2011
EH_AR				odbcSourceList(EH_ODBC_SECTION * ps,CHAR *lpDriver); // new 2009
EH_AR				odbcTableList(EH_ODBC_SECTION * ps,CHAR *lpLibrary,CHAR *lpName,CHAR *lpType); // new 2009
int					odbcAffectedRows(EH_ODBC_SECTION * psOdbcSection);

EH_ODBC_RS			odbcQueryResultCreate(EH_ODBC_SECTION * psOdbcSection,INT iLimit);

#ifdef EH_MEMO_DEBUG 
	EH_ODBC_RS _odbcStore(EH_ODBC_SECTION * ps,INT iLimit,CHAR * pProg, INT iLine);
	#define odbcStore(ps,iLimit) _odbcStore(ps,iLimit,__FILE__,__LINE__)
#else
	EH_ODBC_RS	odbcStore(EH_ODBC_SECTION * ps,INT iLimit);
#endif

#ifndef EH_CONSOLE
	BOOL	odbcHookGet(EH_ODBC_SECTION * psOdbcSection,EH_TABLEHOOK *arsHook,CHAR *Mess,...);
	int		odbcHookReset(EH_TABLEHOOK *arsHook);
	int		odbcHookGetRs(EH_TABLEHOOK *arsHook,EH_ODBC_RS pRes);
	BOOL	odbcHookInsert(EH_ODBC_SECTION * psOdbcSection,EH_TABLEHOOK *arsHook,CHAR *pQuery,...);
	int		odbcHookUpdate(EH_ODBC_SECTION * psOdbcSection,EH_TABLEHOOK *arsHook,CHAR *pQuery,...);

#endif

//
// SINGLE-THREAD Function
//

#define		odbc_Connect(pSQLHostName,pSQLUser,pSQLPassword,pDbaseSchema,client_flag,bShowWindow) odbcConnect(odbcSectionMain(),pSQLHostName,pSQLUser,pSQLPassword,pDbaseSchema,client_flag,bShowWindow)
#define		odbc_Disconnect() odbcDisconnect(odbcSectionMain())
#define		odbc_query(sz)	odbcQuery(odbcSectionMain(),sz)
#define		odbc_queryarg(sz,...) odbcQueryf(odbcSectionMain(),sz,__VA_ARGS__)
#define		odbc_queryargBig(dwSizeMemory,sz,...) odbcQueryBig(odbcSectionMain(),dwSizeMemory,sz,__VA_ARGS__);
#define		odbc_querytoarray(sz,...) odbcQueryToArray(odbcSectionMain(),sz,__VA_ARGS__)
#define		odbc_queryrow(sz,...) odbcRow(odbcSectionMain(),sz,__VA_ARGS__)
#define 	odbc_count(msg,...) odbcCount(odbcSectionMain(),msg,__VA_ARGS__)
#define 	odbc_sum(msg,...) odbcSum(odbcSectionMain(),msg,__VA_ARGS__)
#define		odbc_affected_rows() odbcAffectedRows(odbcSectionMain())
#define		odbc_setparam(sz,...) odbcSetParam(odbcSectionMain(),sz,__VA_ARGS__)
#define		odbc_limit(dwOffset,dwLimit) odbcLimit(odbcSectionMain(),dwOffset,dwLimit)

#define		odbc_tableinfo(lpLibrary,lpName,lpType) odbcTableInfo(odbcSectionMain(),lpLibrary,lpName,lpType)
#define		odbc_sourcelist(lpDriver) odbcSourceList(odbcSectionMain(),lpDriver)
#define		odbc_tablelist(lpLibrary,lpName,lpType) odbcTableList(odbcSectionMain(),lpLibrary,lpName,lpType)
#define		odbc_store_result(iLimit) odbcStore(odbcSectionMain(),iLimit)
#define		odbc_store() odbcStore(odbcSectionMain(),100)
	/*
const char *		odbc_GetError(void);
CHAR *				odbc_error(CHAR *pWho,INT iMode);
INT					odbc_count(CHAR *Mess,...);
INT					odbc_sum(CHAR *lpField,CHAR *Mess,...); // News 8/2007
INT					odbc_query(CHAR *pQuery);
INT					odbc_queryarg(CHAR *Mess,...);
INT					odbc_queryargBig(DWORD dwSizeMemory,CHAR *Mess,...);

EH_AR				odbc_querytoarray(CHAR *Mess,...); // Ritorna un array con la prima riga della query
EH_ODBC_RS			odbc_queryrow(CHAR *Mess,...);
int					odbc_affected_rows(); // Ritorna il numero di righe modificate dopo un UPDATE,INSERT o DELETE
void				odbc_setparam(EN_ODBC_PARAM enParam,DWORD dwParam); // 2010
CHAR *				odbc_limit(DWORD dwOffset,DWORD dwLimit); // 2010
CHAR *				odbc_tableinfo(CHAR * lpLibrary, CHAR * lpName, CHAR *lpType); // 2011
EH_AR				odbc_sourcelist(CHAR *lpDriver); // new 2009
EH_AR				odbc_tablelist(CHAR *lpLibrary,CHAR *lpName,CHAR *lpType); // new 2009
*/


//
// HOOK functions
//
#ifndef EH_CONSOLE
#define		odbc_hookget(arsHook,Mess,...) odbcHookGet(odbcSectionMain(),arsHook,Mess,__VA_ARGS__)
#define		odbc_hookinsert(arsHook,pQuery,...) odbcHookInsert(odbcSectionMain(),arsHook,pQuery,__VA_ARGS__)
#define		odbc_hookupdate(arsHook,pQuery,...) odbcHookUpdate(odbcSectionMain(),arsHook,pQuery,__VA_ARGS__)
#define		odbc_hookgetRs(a,b)	odbcHookGetRs(a,b)
#define		odbc_hookreset(a) odbcHookReset(a)
#endif


//
// RS Functions (SINGLE E MULTI THREAD)
//
BOOL	odbc_fetch_row(EH_ODBC_RS pRes); // Giro sulle righe
#define odbc_fetch(a) odbc_fetch_row(a)
BOOL	odbc_fetch_first(EH_ODBC_RS pRes); // Ricomincio dalla prima
void	odbc_free_result(EH_ODBC_RS pRes); // Libero le risorse del result
#define odbc_free(a) odbc_free_result(a)
#define odbc_row(sz,...) odbcRow(odbcSectionMain(),sz,__VA_ARGS__)
BOOL	odbc_setcursor(EH_ODBC_RS pRes,DWORD dwCursor); // Riparte da capo (2010)

int		odbc_fldfind(EH_ODBC_RS pRes,CHAR *lpField,BOOL bNoError);
CHAR *	odbc_fldptr(EH_ODBC_RS pRes,CHAR *lpField);
#define odbc_fldint(pRes,fieldName) atoi(odbc_fldptr(pRes,fieldName))
#define odbc_fldnum(pRes,fieldName) atof(odbc_fldptr(pRes,fieldName))
SIZE_T	odbc_fldlen(EH_ODBC_RS pRes, CHAR *lpField); // 2011 - per dati binari
EH_ODBC_FIELD * odbc_fldinfo(EH_ODBC_RS pRes, CHAR *lpField,BOOL bNoError);

//int		odbc_queryEx(DYN_ODBC_SECTION_FUNC BOOL fNoErrorStop,CHAR *lpQuery);
//int		odbc_queryargEx(DYN_ODBC_SECTION_FUNC BOOL fNoErrorStop,CHAR *Mess,...);
// void	OdbcTry(CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult);

//
// Query functions ?
//
void		odbc_QueryResultDestroy(EH_ODBC_RS pRes);
CHAR *		odbcDisplay(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN rc1);


// Query Globale semplificata
/*
BOOL	odbc_store_result(DYN_ODBC_SECTION_FUN);  // Richiedo il risultato
BOOL	odbc_fetch_row(DYN_ODBC_SECTION_FUN); // Giro sulle righe
void	odbc_free_result(DYN_ODBC_SECTION_FUN); // Libero le risorse del result
char   *odbc_fptr(DYN_ODBC_SECTION_FUNC CHAR *lpField);
int     odbc_fint(DYN_ODBC_SECTION_FUNC CHAR *lpField);
double  odbc_fnum(DYN_ODBC_SECTION_FUNC CHAR *lpField);
int		odbc_ffind(DYN_ODBC_SECTION_FUNC CHAR *lpField,BOOL bNoError);
CHAR   *odbc_QueryLastError(DYN_ODBC_SECTION_FUNC CHAR *lpQuery);
*/

//
// Macro per lettura campi in query ex(
//

#else

// Per compatibilità con il passato e/o doppia gestione
#define odbc_FldInt adb_FldInt
#define odbc_FldNume adb_FldNume
#define odbc_FldPtr adb_FldPtr
#define odbc_BlobGetAlloc adb_BlobGetAlloc
#define odbc_BlobFree adb_BlobFree
#define odbc_GetDate adb_GetDate
#endif

/*
#ifndef EH_ODBC_MT
	
	#ifdef EH_MAIN
		EH_ODBC_SECTION sOdbcSection={0,NULL,0};
	#else
		extern EH_ODBC_SECTION * arsOdbcSection;
	#endif

#else
	#define MAX_MTSECTION 64
	#ifdef EH_MAIN
		EH_ODBC_SECTION arsOdbcSection[MAX_MTSECTION];
	#else
		extern EH_ODBC_SECTION * arsOdbcSection;
	#endif
#endif
	*/