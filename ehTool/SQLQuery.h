//   ---------------------------------------------
//   | SQLQuery
//   ---------------------------------------------

typedef struct {

//	HDB Hdb; 		 // Handle ADB
	SINT iIndex; // Usato se order è NULL
	BOOL fHideError; // T/F se deve nascondere gli errori
	BOOL fSelectName; // T/F se deve rintracciare i campi con i nomi contenuti nella select

//	BYTE  *lpAdbCloneBuffer; // Puntatore al Buffer Clonato del ADB (Clonato per non influire con il Normale uso del buffer)
	BYTE  *lpAdbOriginalBuffer; // Necessario per il doppio click
	SINT  iAdbBufferSize;

	LONG   lRowSize; // Dimensione della riga
	LONG   lRowsBuffer; // Righe possibili nel buffer
	LONG   lRowsReady; // Righe lette
	SQLUINTEGER lRowsTotal;   // Numero totali di linee presenti con questa Query
	SQLUINTEGER lRowsCurrent; // 
	SINT   hDictionary; // Handle che contiene il dizionario del ResultSet
	SINT   iFieldNum; // Numero di campi
	EMUFIELD *lpEF;

	SQLRETURN sqlReturn; // Ultimo status
	SQLRETURN sqlLastError; // Ultimo errore
	CHAR *lpOrder;

	// SQL
//	CHAR *lpQueryWhereAdd; // Where aggiunta <CONDIZIONE>
	CHAR *lpQueryCount; // Comando SELECT COUNT(*) per contare i record
	SQLHSTMT hStmtClone; // Handle Stantment Clone
	CHAR *lpQueryRequest; // Query completa per l'estrazione (SELEXT <CAMPI> FORM <TABLE> WHERE <CONDIZIONE> ORDER BY <INDICE>)
	BOOL fCursorOpen;

//	CHAR *lpQueryActive;  // Puntatore alla Query Attiva al momento
	SINT iSQLOffset; // Offset di lettura delle righe
	SINT iQNum; // Numero di campi memorizzati
	SINT iQFields; // Array di trascodifica per i campi (serve per avere lo stesso nome richiesto con il comando SELECT)
	CHAR **lppQFields;

	BOOL fODBCScrollable;

	BOOL fNoRead; // T/F se è un comando se letture campi
	BOOL fAutoTrimRight; // T/F toglie automatica mente gli spazi a destra (new 2007)

	SQLINTEGER iNativeError;
	SQLCHAR  sqlState[6];
	SQLCHAR  sqlMsg[SQL_MAX_MESSAGE_LENGTH];

} FSQLQUERY;

void *SQLQuery(SINT cmd,FSQLQUERY *lpQuery,LONG info,void *ptr);
#define SQUERY_DIRECT -1
BOOL SQLQueryArg(CHAR *pMess,...); // new 2007
