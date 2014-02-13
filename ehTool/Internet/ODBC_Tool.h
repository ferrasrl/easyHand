//   ---------------------------------------------
//   ³ ODBC_TOOL.h
//   ³ Tools per gestione odbc             
//
//   ³          by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#include <process.h>
#include <sql.h>
#include <sqlext.h>

	typedef struct 
	{
	  CHAR szName[64];
	  SINT iType;
	  CHAR szType[10];
	  SINT iSize;
	  CHAR *lpField;
	} EHODBCFLD;

	typedef struct 
	{
	  SQLHENV  hEnv;
	  SQLHDBC  hConn;
	  SQLHSTMT hStmt;
	  SINT     iFieldNum;
	  SINT     hdlDictionary;
	  EHODBCFLD *lpEhFld;
	} EHODBC;

BOOL ODBCConnect(CHAR *lpServerName);
void *ODBCConnectDriver(CHAR *lpServerDriver);

void  ODBCClose(void);
//void  ODBCSelect(CHAR *lpSelect);
void  ODBCDoDictionary(void);
void ODBCShowDictionary(void);
BOOL  ODBCNext(void);
CHAR *ODBCFldPtr(CHAR *lpName);
//void *ODBCDriverList(void);
SINT ODBCSourceList(CHAR *lpDriver);
SINT ODBCDriverList(CHAR *lpDriver);
SINT ODBCTableList(void);


void ODBCStart(SINT cmd);
void ODBCEnd(void);
void ODBCStartAdb(SINT cmd);
void ODBCEndAdb(void);
SQLRETURN ODBCArgExecute(CHAR *Mess,...);
SQLRETURN ODBCArgPrepare(CHAR *Mess,...);
void  ODBCExecute(void);
EHODBC *ODBCInfo(void);
BOOL ODBCFileOpen(CHAR *lpFileName);
