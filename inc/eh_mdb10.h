//   +-------------------------------------------+
//   | eh_mdb10.h
//   | Interfaccia al dbase Access MDB
//   |             
//   |							  Ferrà srl 2013
//   +-------------------------------------------+

#ifndef MDB_DEFINE
	#define MDB_DEFINE 1
	#ifdef __cplusplus

	#define CATCHERROR(ptr,a)	catch(_com_error &e)\
								{\
									ErrorHandler(e,lstErr);\
									ptr=NULL;\
									return a;\
								}

	#define CATCHERRGET			catch(_com_error &e)\
								{\
									ErrorHandler(e,lstErr);\
									lstPushf(lstErr,"**For Field Name:%s",FieldName);\
									return 0;\
								}

	#import "c:\Program Files\Common Files\System\ADO\msado15.dll" \
				  rename("EOF", "EndOfFile")

	typedef ADODB::_RecordsetPtr	RecPtr;
	typedef ADODB::_ConnectionPtr	CnnPtr; 

	class Database;
	class Table;


	class Database
	{
	public:
		CnnPtr m_Cnn;
		//char m_ErrStr[500];
		EH_LST lstErr;
		Database();
		~Database();
		bool	Open(char* UserName, char* Pwd,char* CnnStr);
		bool	OpenTbl(int Mode, char* CmdStr, Table& Tbl);
		bool	Execute(char* CmdStr);
		bool	Execute(char* CmdStr, Table * Tbl);
		CHAR *	GetErrorErrStr(void);
	};

	class Table{
	public:
		RecPtr m_Rec;
		EH_LST lstErr;
		Table();
		void GetErrorErrStr(char* ErrStr);
		int ISEOF();
		HRESULT MoveNext();
		HRESULT MovePrevious();
		HRESULT MoveFirst();
		HRESULT MoveLast();
		int AddNew();
		int Update();
		int Add(char* FieldName, char* FieldValue);
		int Add(char* FieldName,int FieldValue);
		int Add(char* FieldName,float FieldValue);
		int Add(char* FieldName,double FieldValue);
		int Add(char* FieldName,long FieldValue);
		bool Get(char* FieldName, char* FieldValue);
		bool Get(char* FieldName,int& FieldValue);
		bool Get(char* FieldName,float& FieldValue);
		bool Get(char* FieldName,double& FieldValue);
		bool Get(char* FieldName,double& FieldValue,int Scale);
		bool Get(char* FieldName,long& FieldValue);
	};
	#endif


	#ifdef __cplusplus
	extern "C" {
	#endif

		typedef struct {

			void *	oDatabase;	// Sessione DB	
			void *	oRec;		// Record Set dopo execute

			BOOL	bError;
			CHAR *	pszLastError;
			DWORD	dwQueryCounter;
			BOOL	fDebug;
			CHAR *	pszLastQuery;

		} EH_MDB_SECTION;

		typedef struct {

			EH_MDB_SECTION *	psSection;
			void *	oRec;
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