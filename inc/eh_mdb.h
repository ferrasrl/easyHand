//   +-------------------------------------------+
//   | eh_mdb.h
//   | Interfaccia al dbase Access MDB
//   |             
//   |							  Ferrà srl 2013
//   +-------------------------------------------+

#ifndef MDB_DEFINE
	#define MDB_DEFINE 1

	#ifdef __cplusplus
	extern "C" {
	#endif

		typedef struct {

			//void *	oDatabase;	// Sessione DB	
			void *	pConn;		// Connessione	 (interfaccia)
			void *	pvConn;		// Connessione	 (interfaccia)
			void *	pRst;		// Record Set (interfaccia)

			BOOL	bError;
			CHAR *	pszLastError;
			DWORD	dwQueryCounter;
			BOOL	fDebug;
			CHAR *	pszLastQuery;

		} EH_MDB_SECTION;

		typedef struct {

			EH_MDB_SECTION *	psSection;
			void *	pRst;		// Record Set (interfaccia)
			EH_AR	arCells;	// array Variant

			INT					iFieldNum;
			CHAR *				pszQuery;	// 2010 > Query che ha dato il risultato
	/*
			//		EH_ODBC_FIELD *		arDict;

			BOOL				bCursorOpen; // T/F se il cursore è aperto
			BOOL				bDataReady;	// T/F se i dati sono pronti
	*/
			INT					iRowsLimit;	// Numero di righe lette per query
			INT					iOffset;
			INT					iMaxRows;		// Righe totali da leggere nella query
			INT					iRowsReady;	// Righe effettivamente lette
			INT					iCurrentRow;	// Puntatore alla linea di result da leggere
			BOOL				bCloseProtect;

		} EH_MDB_RESULTSET;

		typedef EH_MDB_RESULTSET * EH_MDB_RS;

		EH_MDB_SECTION *	mdbConnect(	CHAR *	pszFileMdb,
										CHAR *	pszConnectionString,
										CHAR *	pSQLUser,
										CHAR *	pSQLPassword);
		int mdbQuery(EH_MDB_SECTION * psSection,CHAR * pszQuery);
		int mdbQueryf(EH_MDB_SECTION * psSection,CHAR *Mess,...);
		
		EH_MDB_RS	mdbStore(EH_MDB_SECTION * psSection);
		BOOL		mdbFetch(EH_MDB_RS psRs);
		CHAR *		mdbFldPtr(EH_MDB_RS psRs,CHAR * pszName);
		INT			mdbFldInt(EH_MDB_RS psRs,CHAR * pszName);
		double		mdbFldNum(EH_MDB_RS psRs,CHAR * pszName);
		BOOL		mdbFree(EH_MDB_RS psRs);

	int mdbCheck(CHAR * pszFileAccess);


	#ifdef __cplusplus
	}
	#endif

#endif 