//   +-------------------------------------------+
//   | SQlite
//   | Interfaccia al dbase mySql
//   |             
//   |							  Ferrà srl 2006
//   +-------------------------------------------+

#include "/easyhand/ehtoolx/sqlite/sqlite3.h"

typedef struct {
	
	sqlite3 *		dbSqlLite; // Struttura principale creata in connessione
	CHAR *			lpQuery;
	sqlite3_stmt *	psStantment; // Struttura del result ad un'azione di query

	BOOL			fDebug; // Modalità Debug
	DWORD			dwQueryCounter;
	DWORD			dwQueryTimeout; // Default 15 sec

//	BOOL			bReconnect; // T/F se deve provare a riconnetersi in caso di errore 2006
//	INT				iReconnectTry; // Numero dei tentativi da fare prima di dare errore
//	void (*funcExtNotify)(void *,SINT iTry,SINT iError);

	SINT			iLastError;
	BYTE *			pszQueryBuffer;

} EH_SQLITE_SECTION;

typedef struct {
  CHAR szName[64];	// Nome del campo
  EH_DATATYPE iType;
  CHAR szType[10];
  SINT iSize;
} EH_SQLITE_FIELD;

typedef struct {

	EH_SQLITE_SECTION * psqSection;
	sqlite3_stmt *psStmt;
	BYTE **arRow;
	SINT	iFields;
	EH_SQLITE_FIELD *arFields; 
	SINT	iRows;
	SINT	iCurrent;
	CHAR *	pszQuery;

} EH_SQLITE_RESULTSET;

typedef EH_SQLITE_RESULTSET * EH_SQLITE_RS;

BOOL			sqlite_open(CHAR * lpFileName,INT iFlags);
BOOL			sqlite_close(void);

SINT			sqlite_query(CHAR * pQuery);
SINT			sqlite_queryarg(CHAR *Mess,...);
EH_SQLITE_RS	sqlite_store(void);

EH_SQLITE_RS	sqlite_queryrow(CHAR *Mess,...);

BOOL			sqlite_fetch(EH_SQLITE_RS);
void			sqlite_free(EH_SQLITE_RS);

SINT			sqlite_count(CHAR *Mess,...);
BYTE *			sqlite_QueryLastError(CHAR *pQuery);

INT				sqlite_fldfind(EH_SQLITE_RS rsSet,CHAR * pColumnName);
BYTE *			sqlite_ptr(EH_SQLITE_RS,CHAR * pColumnName);
INT				sqlite_int(EH_SQLITE_RS,CHAR * pColumnName);
DOUBLE			sqlite_num(EH_SQLITE_RS,CHAR * pColumnName);

CHAR  *			sqlite_HookTouch(BYTE *pTable,BYTE *pQuery,struct ADB_HOOK *pHook); 
SINT			sqlite_HookWrite(BYTE *pTable,struct ADB_HOOK *pHook);
SINT			sqlite_HookGet(BYTE *pTable,struct ADB_HOOK *pHook);
SINT			sqlite_HookInsert(BYTE *pTable,struct ADB_HOOK *pHook);
SINT			sqlite_HookUpdate(BYTE *pTable,struct ADB_HOOK *pHook);


extern EH_SQLITE_SECTION sqSection;