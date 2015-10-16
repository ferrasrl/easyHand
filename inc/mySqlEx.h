// ------------------------------------------------
//  mySqlEx
//	Funzioni Estese mySql 
//  
//                            by Ferrà srl 2012/2013
// ------------------------------------------------


typedef enum {

	TBL_UNKNOW,
	TBL_BASE,
	TBL_VIEW

} EN_TBLTYPE;


typedef struct {

	CHAR		szName[100]; // nome del campo
	EN_TBLTYPE	enTableType;

} S_TBL_INFO;

typedef struct {

	CHAR		szName[50]; // nome del campo
	CHAR		szTypeText[300]; // nome del campo
	EN_FLDTYPE	enFldType;	// ALFA,NOTE
	INT			iSize; // Grandezza in byte
	INT			iDecimal; // Numero decimali 
	BOOL		bCaseInsensible; // Solo per le lettere
	
	EN_CHARTYPE enEncoding;
	//INT iNullMode;	// 0=Not nullable (default) 1=Null 2=Default full (0 oppure '') 
	BOOL		bIsNullable;
	CHAR		szDefault[80];
	CHAR		szFieldBefore[80];
	
	BOOL		bTouch:1;	// Riservato

} S_FLD_INFO;


typedef struct {

	CHAR		szName[50]; // nome del INDICE
	BOOL		bUnique;
	INT			iSeq;
	CHAR		szField[20];	// Nome Campo
	CHAR		szCollation[20];
	//BOOL		bTouch:1;	// Riservato

} S_IDX_INFO;


typedef struct {
	
	CHAR *		pszName;
	BOOL		bNotExist;	// Non esiste nella destinazione
	BOOL		bInsert;	// T/F se siamo in insert
	CHAR *		pszSep;		// Separatore tra campi

	CHAR *		pszValue;
	EN_FLDTYPE	enFldType;
	EH_LST		lstQuery;
	

	BOOL		bError;		// Da valorizzare se il record non va memorizzato
	
	
	// Parametri liberi
	void	*	pVoid;

} S_FLD_EXT;

S_TBL_INFO * mysGetSchema(DYN_SECTION_FUNC CHAR * pszSchema,_DMI * pdmiSchema);
S_FLD_INFO * mysGetTableInfo(DYN_SECTION_FUNC CHAR * pszTable,_DMI * pdmiField,_DMI * pdmiIndex);
BOOL		mysTableExport(DYN_SECTION_FUNC CHAR * pszTable,UTF8 * utfFileDest,CHAR * pszWhere,BOOL bShowProgress, DWORD * pdwRecords,CHAR * pszFieldsExclude,BOOL (*funcExt)(EH_SRVPARAMS));
BOOL		mysTableImport(DYN_SECTION_FUNC UTF8 * pszFileSource,CHAR * pszTableDest,BOOL bNotUpdate, BOOL bShowProgress,EH_LST	lstErrors,CHAR * pszFieldsExclude,BOOL (*funcExt)(EH_SRVPARAMS));
//CHAR *	mysFieldQuery(CHAR * pszTable,EN_MESSAGE enMess,S_FLD_INFO * psFld);
INT			mysFieldSearch(_DMI * pdmiFld,CHAR * pszName);
INT			mysTableSearch(_DMI * pdmiTbl,CHAR * pszName);
CHAR *		mysFieldModify(EN_MESSAGE enMess,CHAR * pszTable,S_FLD_INFO * psFld);

BOOL		mysTableSync(S_MYSQL_SECTION * psecSource,CHAR * pszTableSource,
					 S_MYSQL_SECTION * psecDest,CHAR * pszTableDest,
					 CHAR *		pszCode,
					 CHAR *		pszWhere,
					 BOOL		bUpdate,
					 BOOL		bAutoRemove,
					 INT		iBlockRecord,
					 BOOL		bShowProgress,
					 BOOL		bOperatorAsk); 
BOOL		mysTableUtf8Repair(DYN_SECTION_FUNC CHAR * pszTable,BOOL bShowProgress,DWORD * pdwUpdate,DWORD * pdwErrors);

void	 mysSchemaSync(	CHAR *	pszSchemaSource,
						CHAR *	pszSchemaTarget,
						CHAR *	pszSchemaImport,
						BOOL	bOperatorAsk);