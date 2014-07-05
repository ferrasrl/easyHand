//   ---------------------------------------------
//   | FLM_FHD.H Ferrà Libary Module             |
//   |           Ferrà Hierarchic Dbase          |
//   |                                           |
//   |             by Ferr… Art & Tecnology 2000 |
//   ---------------------------------------------

#ifndef ADB_MAX_DESC
#define  ADB_MAX_DESC 30
#endif

#define FXD_SERVER 

typedef struct {
	SINT bOpen;
} FXD_FOL_INFO;

typedef struct {
	CHAR *	pszComments;
	CHAR *	pszQuery;
} FXD_QRY_INFO;

typedef enum {

	FHD_BTRIEVE,
	FHD_SQL_IBMDB2=1,
	FHD_SQL_ORACLE=2,
	FHD_SQL_MICROSOFT=3,
	FHD_SQL_MYSQL=4,
	FHD_SQL_SQLITE=5,

	FHD_DEFAULT=1000

} EN_SQL_PLATFORM;


typedef struct {

	EN_SQL_PLATFORM enCode;
	CHAR *			pszName;
	BOOL			bSql;

} EN_SQL_PLATFORM_INFO;



typedef enum {

	FHD_TABLE=1,
	FHD_FOLDER,
	FHD_FIELD,
	FHD_INDEX,
	FHD_VIEW,
	FHD_QUERY

} EN_FXD_TYPE;

typedef struct {

	EN_SQL_PLATFORM enSqlPlatform; // Piattaforma usata per il dbase SQL
	CHAR *	pszSqlAlternateName; // Nome della tabella SQL
	CHAR *	pszTableComments;
	CHAR *	pszLocalFile;		// Usato con il btrieve (nome del file locale bti)
	CHAR *	pszPresetAutoinc;	// Usato per determinare l'inzio dell'autoinc

} FXD_TAB_INFO;

typedef struct {
	CHAR *lpDesc; // Descrizione dell indice
	BOOL fDup; // Se la chiava è Duplicabile
	BOOL fCase; // Se è Case sensible (?)
	// BOOL fSort; // 0=ASC 1=DESC (cambiato nel 2010)
	CHAR *pType; // "" non dichiarato = ASC, DESC, FULLTEXT, SPATIAL
	CHAR *lpCompos; // Elenco dei campi coinvolti separati da +
//	CHAR *lpOther; // Usi Futuri
} FXD_IDX_INFO;

typedef struct {
	EN_FLDTYPE iType;	// ALFA,NOTE
	SINT iSize; // Grandezza in byte
	SINT iDecimal; // Numero decimali 
	BOOL bCaseInsensible; // Solo per le lettere
	CHAR *lpNote;
	BYTE *lpCharSet;
	SINT iNullMode;	// 0=Not nullable (default) 1=Null 2=Default full (0 oppure '') 
} FXD_FLD_INFO;

typedef struct {
	EN_FXD_TYPE iType;
	SINT idAutocode;
	SINT idParent; // Id del parente (idAutocode)
	//CHAR szOldCode[MAXPATH]; // Vecchio sistema di codificazione
	CHAR szName[ADB_MAX_DESC*2];
	FXD_FOL_INFO *psFolderInfo;
	FXD_TAB_INFO *psTableInfo;
	FXD_FLD_INFO *psFieldInfo;
	FXD_IDX_INFO *psIndexInfo;
	FXD_QRY_INFO *psQueryInfo;

	BOOL bRemove; // Usato in fase di rimozione
} FXD_ELEMENT;

typedef struct {
	SINT iType;
	SINT iFrom;
	SINT iTo;
} FHDPROCESS;


#define FHD_NO_FREE   1024
#define FHD_MODE_FHD   50
#define FHD_MODE_ADB   51
#define FHD_MODE_FXD   52

#define FHD_OPEN_NEW   60
#define FHD_OPEN_RESET 61

#define FHD_GET_NAME   -100
#define FHD_GET_FILE   -101
#define FHD_GET_LAST   -102
#define FHD_GET_DSN    -103
#define FHD_GET_USER   -104
#define FHD_GET_PASS   -105
#define FHD_GET_LIB    -106
#define FHD_GET_PLATFORM -107
#define FHD_GET_VERSION -108
#define FHD_GET_BTRIEVE -109 // per compatibilità con il passato
#define FHD_GET_SERVER  -110

#define FHD_SET_NAME   -200
#define FHD_SET_FILE   -201 
#define FHD_SET_DSN    -203
#define FHD_SET_USER   -204
#define FHD_SET_PASS   -205
#define FHD_SET_LIB    -206
#define FHD_SET_PLATFORM   -207
#define FHD_SET_VERSION   -208
#define FHD_SET_BTRIEVE   -209
#define FHD_SET_SERVER   -210

#define FHD_FIND_FIELD -0x10
#define FHD_FIND_TABLE -0x20
#define FHD_FIND_TABLE_FIELD -0x30

#define FHD_PROC_MOVE  0


SINT	FhdReader(SINT cmd,SINT info,void *str);
SINT	fxdServer(EN_MESSAGE cmd,LONG info,void *str);
SINT	FHD_EnumType(CHAR *lpType);

//void FHD_SetIndexCode(CHAR *lpCode,CHAR *lpDesc,BOOL fDup,BOOL fCase,BOOL fSort,CHAR *lpCompos);
//void FHD_GetIndexCode(CHAR *lpCode,CHAR **lpDesc,BOOL *fDup,BOOL *fCase,BOOL *fSort,CHAR **lpCompos);
//void FHD_SetFieldCode(CHAR *lpCode,SINT iTipo,SINT iSize,SINT iStyle,CHAR *lpNote);
//void FHD_GetFieldCode(CHAR *lpCode,SINT *iTipo,SINT *iSize,SINT *iStyle,CHAR **lpNote);
/*
void FHD_SetIndexCode(CHAR *lpCode,FXD_IDX_INFO *lpf);
void FHD_GetIndexCode(CHAR *lpCode,FXD_IDX_INFO *lpf);
void FHD_GetTableCode(CHAR *lpCode,CHAR **lpLocalFile,SINT *iFql,CHAR **lpSQLTableName);
void FHD_SetTableCode(CHAR *lpCode,CHAR *lpLocalFile,SINT iFql,CHAR *lpSQLTableName);

void FHD_SetFieldInfo(CHAR *lpCode,FXD_FLD_INFO *lpsFldInfo);
void FHD_GetFieldInfo(CHAR *lpCode,FXD_FLD_INFO *lpsFldInfo);
*/
void FHD_DDFReport(CHAR *lpDbPath);
void FHD_DDFViewer(void);
void FHD_DDFMaker(void);

typedef struct {

	SINT	iVersion;
	SINT	iDefaultPlatform;	// Piattaforma del dbase Es. FHD_SQL_IBMDB2 0=Non definita
	CHAR	szFHDName[80];
	CHAR	szFileSource[500];
	SINT	iFileSourceType;
	CHAR	szDSN[80];
	CHAR	szServer[80];
	CHAR	szUser[80];
	CHAR	szPass[80];
	CHAR	szLibrary[80];
	CHAR	szVersion[80];
	BOOL	bBtrieveMigration;	// New 2011 (T/F se deve essere gestita in contemporanea a SQL un accesso Btrieve)

} FHD_PROPERTY;

void			fxdPropertyBuilder(FXD_ELEMENT *pElement,SINT iProperty,BOOL bAllocInfo);
void			FhdElementFree(FXD_ELEMENT *pFhdElement);
FXD_ELEMENT *	FhdElementClone(FXD_ELEMENT *pSource);
CHAR *			fxdGetTableName(SINT idxTable,BOOL bNotLibrary,CHAR *pszName);


#ifdef EH_MAIN
EN_SQL_PLATFORM_INFO arsFxdPlatform[]={
		{FHD_DEFAULT		,"Default"	,	false},
		{FHD_BTRIEVE		,"Pervasive Btrieve",	false},
		{FHD_SQL_IBMDB2		,"IBM db2/400",	true},
		{FHD_SQL_ORACLE		,"Oracle", true},
		{FHD_SQL_MICROSOFT	,"Microsoft SQL Server", true},
		{FHD_SQL_MYSQL		,"mySql (Oracle)", true},
		{FHD_SQL_SQLITE		,"sqlLite", true},
		{0,NULL,0}
};
#else
extern EN_SQL_PLATFORM_INFO arsFxdPlatform[];
#endif

void * lstFxdMainPlatform(struct OBJ *psObj,EN_MESSAGE enMess,LONG lParam,void *pVoid);
void * lstFxdPlatform(struct OBJ *psObj,EN_MESSAGE enMess,LONG lParam,void *pVoid);
CHAR * fxdPlatformDesc(INT iPlatform);
EN_SQL_PLATFORM fxdPlatformReal(EN_SQL_PLATFORM enPlatform);
EN_SQL_PLATFORM_INFO * fxdPlatformInfo(INT iPlatform);