//   ---------------------------------------------
//	 | txbFile.h
//	 | Gestione dei dbase in formato TXT
//   | (da un'illuminazione Aprile 2003)
//   |
//	 |								by Ferrà 2003
//   ---------------------------------------------

#define TBXVER 0x1010
#define TXB_FIND_FIRST 1 // Dammi il primo
#define TXB_FIND_NEXT 2// Dammi il successivo

// Source Filter di riferimento
typedef enum {
	TXA_REAL,
	TXA_FILTER,
	TXA_SORT
} EN_TXB_ARRAY;


typedef struct {
	CHAR * lpName; // Nome del campo
	EH_DATATYPE enType;
	INT iLen; // Lunghezza campo o numero cifre numero
	INT iInfo; // Decimali / Case insensible ... etc...
} TXBFLD;

typedef struct {

	INT		iVer;				// Versione
	INT		hdl;				// hdl EH Memory
	UTF8	utfNomeFile[200];	// Nome del file

#ifdef __windows__
	HANDLE	hFile;
#endif
	BOOL	bFileSource;		// T/F se il sorgente è su file (non in memoria)
	INT64	uiFileSize;
	DWORD	dwFisMax;			// Numero massimo di posizioni fisiche allocabili
	UINT64	*ariFis;			// Posizione fisica su disco
	UINT64	uiDataOffset;

	CHAR *	pszSource;		// Sorgente dati
	CHAR *	pszDataStart;	// Primo record	(se sono in modalità in memoria bFileSource=FALSE)

	INT	iRealLines;		// Linee presenti totali
	CHAR **	arRealLine;		// Puntatori alle linee Reali (Ordine fisico)
	
	INT	iFilterLines;		// Linee presenti dopo un Filtro
	INT *	arFilterLine;    // Puntatori alle linee Filtrata (NULL nessun filtro)

	INT	iSortLines;		// Line presenti nell'array del sort
	INT *	arSortLine;      // Puntatori alle linee Ordinate (NULL nessun ordinamento)

	// Usato nel filtro di ricerca per notificare l'avanzamento
	INT	iFilterMax;
	INT	iFilterPos;


	//BYTE **lpFieldName;		// Puntatore al nome dei campi
	INT			iFieldCount;		// Campi presenti
	TXBFLD *	lpFieldInfo;		// Puntatore al nome dei campi
	CHAR **		lpFieldPtr;		// Puntatore al valore dei campi
	
	INT	iMaxBuffer;		// Dimensione del buffer "rows"
	CHAR *	lpBuffer;			// Buffer "Rows"
	INT	iVirtualCursor;		// Linea corrente virtuale (da 1 a iLineCount) nel Buffer
	INT	iRealCursor;			// Linea corrente Reale (con il sort potrebbe essere differente)

	FILE *	ch;
	CHAR	szColSep[8];
	INT	iColSep; // Dimensioni col sep
	CHAR	szRowSep[8];
	INT	iRowSep; // Dimensioni Row sep
	void *	pvPointMax;

	EN_CHARTYPE	iCharType; // 0=Ascii, 1=Iso(ansi), 2=uf8
	EN_CHARTYPE iCharDecoding; // 0=no, 1=Ansi
	BOOL		bSilentError;
	BOOL		bFieldUnknownError;	// T/F se va in errore con campi sconosciuti

} S_TXB;

typedef S_TXB * PS_TXB;

PS_TXB	txbOpen(UTF8 * utfFileName);
PS_TXB  txbOpenMemo(UTF8 * utfTxbName,CHAR *pszSource,DWORD dwSize,BOOL bUseSource); // 
PS_TXB	txbOpenOnFile(UTF8 * utfFileName);

#ifdef EH_INTERNET
PS_TXB	txbOpenWeb(CHAR * pszUrl,CHAR * pszMethod,S_WEB_ERR * psWebError);
#endif

BOOL	txbClose(PS_TXB htx);
PS_TXB	CSVOpen(UTF8* utfFileName, BOOL bEncode); // new 2009 / 2010
PS_TXB	txtOpen(UTF8 * utfFileName,CHAR * pszFileHeaderEncode, BOOL bUtf8ToAnsi); // new 2014

TXBFLD * txbFldInfo(PS_TXB htx,CHAR * pszName);
BOOL	txbFind(PS_TXB htx,EN_TXB_ARRAY enArray,INT iMode,void *lpb);
CHAR *	txbFldPtr(PS_TXB htx,CHAR * pszName);
INT	txbFldIdx(PS_TXB htx,CHAR * pszName); // new 2007

#define txbFieldPtr(a,b) txbFldPtr(a,b)
INT	txbFldInt(PS_TXB htx,CHAR * pszName);
double	txbFldNume(PS_TXB htx,CHAR * pszName);
BOOL	txbVirtualGet(PS_TXB htx,EN_TXB_ARRAY enArray,INT iRow);
EH_AR	txbVirtualGetAlloc(PS_TXB htx,EN_TXB_ARRAY enArray,INT iLine,INT * piRealCursor); // New 2007
INT		txbGetCursor(PS_TXB htx,EN_TXB_ARRAY enArray,INT iRow);

BOOL	txbRealGet(PS_TXB htx,INT iRow);

BOOL	txbSort(PS_TXB htx,EN_TXB_ARRAY enArray,CHAR * lpField, BOOL fDescendent,BOOL fCaseNotSensible); // iMode=0 Usa Elenco originale, 1=Usa Elenco Filtrato 
BOOL	txbFilter(PS_TXB htx,EN_TXB_ARRAY enArraySource,BOOL (* funcProc)(PS_TXB htx,BOOL *));
BOOL	txbFindRecord(PS_TXB htx,EN_TXB_ARRAY enArray,BOOL (* funcProc)(PS_TXB ,void *),void *lpdata);

void	txbAdbExport(INT hdb,
					INT iIndex,
					UTF8 * lpFileExport,
					CHAR *lpCol,
					CHAR *lpRow,
					CHAR *lpFieldsInclude, // Includere questi campi separati da pipe = NULL TUTTI
					CHAR *lpFieldsExclude, // Non includere questi campi separati da pipe = NULL non controllo
					CHAR *lpAddFields,
					BOOL (*funcNotify)(EH_SRVPARAMS));

#if (defined(_ADB32)||defined(_ADB32P))

	// Usato per funzione di notifica esterna
	typedef struct {
		
		HDB		hdb;
		PS_TXB	txb;
		CHAR *	pszName;	// Nome del campo
		
		BOOL	bReady;
		CHAR *	pszValue;	
		BOOL	bExclude;
	
	} S_TXB_EXT;

BOOL	txbFldImport(HDB hdbDest,
					 PS_TXB txbSource,
					 BOOL fCutSpace,
					 BOOL bNotResetUnknow,
					 CHAR * pszBlackList,
					 CHAR * pszWhiteList,
					 void *	(*funcNotify)(EH_SRVPARAMS));
#endif

PS_TXB	txbCreate(UTF8 * utfFileName,
			      CHAR *lpCol, // Carattere separatore colonne
			      CHAR *lpRow, // Carattere separatore righe
				  CHAR *lpEnconding, // NULL= default (ASCII)
			      TXBFLD *lpFld);// Descrizione dei campi

void	txbArgWrite(PS_TXB lph, // Handle Txb
					CHAR *lpCel, // Carattere di separazione campi
					CHAR *Mess,
				 ...);

BOOL	txbArrayAlloc(PS_TXB htx,EN_TXB_ARRAY enArrayMode,BOOL fCloneReal); // new 2007
void	txbArrayCopy(PS_TXB htx,EN_TXB_ARRAY enSource,EN_TXB_ARRAY enDest);

