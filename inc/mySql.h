//   +-------------------------------------------+
//   | mySql
//   | Interfaccia al dbase mySql
//   |
//   |							  Ferrà srl 2006
//   +-------------------------------------------+

#ifdef _MYSQL_MT
#define _MYSQL
#endif

#ifdef _MYSQL
#include "/easyhand/ehtoolx/MySQL Server 5.0/include/mysql.h"

#define MAX_MTSECTION 64
typedef struct {

	MYSQL *mySql;//=NULL; // Struttura principale creata in connessione
	void * psRS; // Result Globale da usare in caso di compatibilità con Pervasive

	CHAR *	pszQueryBuffer;
	CHAR *	pszQueryLast;

	BOOL	bDebug; // Modalità Debug
	DWORD	dwQueryCounter;

	BOOL bReconnect; // T/F se deve provare a riconnetersi in caso di errore 2006
	INT iReconnectTry; // Numero dei tentativi da fare prima di dare errore
	void (*funcExtNotify)(void *,INT iTry,INT iError);

	DWORD idThread;
	CHAR * pSqlHost;
	CHAR * pSqlUser;
	CHAR * pSqlPass;
	CHAR * pSqlSchema;
	INT iSqlClientFlag;

	DWORD dwCalls;	// Numero di volte che è stato chiamato il thread

} S_MYSQL_SECTION; // Sezione per le interrogazioni

typedef struct {
	S_MYSQL_SECTION * psSection;
	MYSQL_RES *myRes;
	MYSQL_ROW myRow;
	INT iFields;
	MYSQL_FIELD *arFields; // new 2009
	INT iRows;
	INT iCurrent;
} EH_MYSQL_RESULTSET;

//#define EH_MYSQL_RS EH_MYSQL_RESULTSET *
typedef EH_MYSQL_RESULTSET *EH_MYSQL_RS;

//
// Macro SENZA supporto MultiThread
//
#ifndef _MYSQL_MT

#define DYN_SECTION_FUN
#define DYN_SECTION_FUNC
#define DYN_SECTION_GLOBALPTR S_MYSQL_SECTION * mySection=&sMYS;
#define DYN_SECTION
#define DYN_SECTIONC

//
// Macro con supporto MultiThread
//
#else

#define DYN_SECTION_FUN S_MYSQL_SECTION *mySection
#define DYN_SECTION_FUNC S_MYSQL_SECTION *mySection,
#define DYN_SECTION_GLOBALPTR
#define DYN_SECTION mySection
#define DYN_SECTIONC mySection,

#endif

void mys_start(INT iStep);
void mys_end(INT iStep);
void mys_debug(DYN_SECTION_FUNC BOOL bEnable);


//CHAR *mys_QueryLastError(CHAR *lpQuery);

#ifdef _MYSQL_MT
//
// Gestione section MultiThrad
//
INT	mys_MTSectionId(DWORD idThread,BOOL bCritical);
S_MYSQL_SECTION *mys_MTOpenSection(DWORD idThread);
S_MYSQL_SECTION *mys_MTGetSection(DWORD idThread);
BOOL	mys_MTCloseSection(DWORD idThread);
void	mys_MTCloseAllSection(void);
CHAR *	mys_MTGetThreads(void);
void	mys_logThread(BOOL bFlag);

#endif

//
// Funzioni semplificate senza il supporto del MultiThread
//
void mys_CreateSection(S_MYSQL_SECTION *mySection);
void mys_DestroySection(S_MYSQL_SECTION *mySection);

MYSQL * mys_Connect(DYN_SECTION_FUNC CHAR *pSQLServer,
					CHAR *pSQLUser,
					CHAR *pSQLPassword,
					CHAR *pDbaseSchema,
					ULONG client_flag,
					CHAR * pszCharSet);
MYSQL *	mys_Reconnect(DYN_SECTION_FUNC INT iTent,INT iErr);
BOOL	mys_Deconnect(DYN_SECTION_FUN);

const char *mys_GetError(DYN_SECTION_FUN);

void		mys_error(DYN_SECTION_FUN);
INT			mys_count(DYN_SECTION_FUNC CHAR *Mess,...); // era mys_recnum
INT			mys_sum(DYN_SECTION_FUNC CHAR *lpField,CHAR *Mess,...); // News 8/2007
int			mys_query(DYN_SECTION_FUNC CHAR *lpQuery); // Effettuo un query
int			mys_queryarg(DYN_SECTION_FUNC CHAR *Mess,...);
int			mys_queryargBig(DYN_SECTION_FUNC DWORD dwSizeMemory,CHAR *Mess,...);


//
// RESULT (Core)
//
// Operazione sui campi
//
//int			I_mys_find(MYSQL_RES *myRes,CHAR *lpField);


#ifdef EH_MEMO_DEBUG
	EH_MYSQL_RS mys_store_result_dbg(DYN_SECTION_FUNC CHAR *,INT); // Richiesta del ResultSet
	#ifndef _MYSQL_MT
		#define	mys_store_result() mys_store_result_dbg(__FILE__,__LINE__)
	#else
		#define	mys_store_result(a) mys_store_result_dbg(a,__FILE__,__LINE__)
	#endif

	EH_MYSQL_RS mys_queryrow_dbg(DYN_SECTION_FUNC CHAR * pszProg,INT iLine, CHAR * pszMess, ...); // Richiesta del ResultSet
	#ifndef _MYSQL_MT
		#define	mys_queryrow(...) mys_queryrow_dbg(__FILE__,__LINE__, __VA_ARGS__)
	#else
		#define	mys_queryrow(a,...) mys_queryrow_dbg(a,__FILE__,__LINE__, __VA_ARGS__)
	#endif

#else
	EH_MYSQL_RS mys_queryrow(DYN_SECTION_FUNC CHAR *Mess,...); // Metodo rapido ad una riga
	EH_MYSQL_RS mys_store_result(DYN_SECTION_FUN); // Richiesta del ResultSet
#endif

BOOL		mys_fetch_row(EH_MYSQL_RS psRS); // Loop sulle righe
void *		mys_free_result(EH_MYSQL_RS psRS); // Libero le risorse del result

int			mys_fldfind(EH_MYSQL_RS psRS,CHAR * lpField,BOOL bNoError);
CHAR *		mys_fldptrEx(EH_MYSQL_RS pRes,CHAR * lpField,CHAR * pDefault,BOOL bNoError);
SIZE_T		mys_fldlen(EH_MYSQL_RS pRes,CHAR * lpField);
DWORD		mys_lastid(DYN_SECTION_FUN);
DWORD		mys_affectedRow(DYN_SECTION_FUN);
#define		mys_fldptr(pRes,fieldName) mys_fldptrEx(pRes,fieldName,NULL,false)
#define		mys_fldint(pRes,fieldName) atoi(mys_fldptrEx(pRes,fieldName,"0",false))
#define		mys_fldnum(pRes,fieldName) atof(mys_fldptrEx(pRes,fieldName,"0",false))


//char   *mys_fptr(DYN_SECTION_FUNC CHAR *lpField);
//int     mys_fint(DYN_SECTION_FUNC CHAR *lpField);
//double  mys_fnum(DYN_SECTION_FUNC CHAR *lpField);
//int		mys_ffind(DYN_SECTION_FUNC CHAR *lpField,BOOL bNoError);

//
//	Richiesta di campo su ResultSet Globale
//
/*
#define mys_fptr(field) mys_fldptr(sMYS.psRS,field)
#define mys_fint(field) mys_fldint(sMYS.psRS,field)
#define mys_fnum(field) mys_fldnum(sMYS.psRS,field)

//
// Leggo i dati in emulazione/compatibilità HDB
//
int		mys_FldFind(int hdb,CHAR *lpField);
int		mys_FldInt(int hdb,CHAR *lpField);
double	mys_FldNume(int hdb,CHAR *lpField);
char	*mys_FldPtr(int hdb,CHAR *lpField);
void	ar_FldToAdb(int idb,BOOL fAfterFree);
void	*mys_BlobGetAlloc(int hdb,CHAR *lpField);
void	mys_BlobFree(void *lpPointer);
char	*mys_GetDate(int hdb,CHAR *lpField);
*/


CHAR   *	mys_QueryLastError(DYN_SECTION_FUNC CHAR *lpQuery);
EH_JSON *	mysToJson(EH_MYSQL_RS rsSet);
CHAR *		mysTimeZoneField(CHAR * pszBuffer,INT iSizeBuffer,CHAR * pszFieldSource,CHAR * pszNameField, CHAR * pszUserTimeZone, CHAR * pszDateFormat);

//#endif



// Utilità
#ifndef MYS_FILE
	#ifndef _MYSQL_MT
	extern S_MYSQL_SECTION sMYS;
	#endif
#endif

//
// Macro per lettura campi in query ex(
//
//#define mys_GetInt(myRow,myRes,fieldName) atoi(myRow[mys_find(myRes,fieldName)])
//#define mys_GetPtr(myRow,myRes,fieldName) myRow[mys_find(myRes,fieldName)]

#else

// Per compatibilità con il passato e/o doppia gestione
#define mys_FldInt adb_FldInt
#define mys_FldNume adb_FldNume
#define mys_FldPtr adb_FldPtr
#define mys_BlobGetAlloc adb_BlobGetAlloc
#define mys_BlobFree adb_BlobFree
#define mys_GetDate adb_GetDate
#endif



