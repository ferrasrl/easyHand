//ÕÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¸
//³ eh_adb.h - Header per la gestione dei  ³
//³             file adb/Btrieve            ³
//³                                         ³
//³          by Ferr… Art & Technology 1996 ³
//ÔÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¾

#define SEFS    -32
#define MEFS    -64
typedef int	   HDB;

#define ADBNULL 0

//	by DAM (inizio)
#define ADB_UNLOCK_SINGLE		1 
#define ADB_UNLOCK_MULTIPLE	2 
#define ADB_UNLOCK_ALL			3

INT adb_unlock(INT tipo,INT Hdb,LONG position,INT index);
//	by DAM (fine)

#ifdef _WIN32
 #define BTI_WIN_32 1
 #include "/easyhand/inc/btrapi.h"
 #include "/easyhand/inc/btrconst.h"
 #define NORMAL B_NORMAL 
#else
 #define BTI_DOS 1
 #include <btrapi.h>
 #include <btrconst.h>
 #define NORMAL B_NORMAL
#endif

#ifdef _ADBEMU
#include <process.h>
#include <sql.h>
#include <sqlext.h>

#define EMU_SIZE_QUERY 8192 // Dimensione massima di una richiesta di Query/Stantment
#define EMU_SIZE_QUERY_BIG 0x10000 // Dimensione massima di una richiesta di Query/Stantment
typedef struct {
	INT hdlFileEmu; // Handle del file Emu
	INT iRows; // Numero di righe nel file Emu
	CHAR **lpRows; // Puntatore alle righe
	INT iODBCCountOpen; // Contatore delle connessioni aperte ad ODBC

	CHAR szDNSServerName[80];
	CHAR szDNSUserName[80];
	CHAR szDNSPassword[80];
	
	/*
	SQLHENV hEnv; // Ambiente
	SQLHDBC hConn;
	// Informazioni sul dbase
	SQLUINTEGER uiAsync;  // Info sulla possibilità di eseguire comandi Asincroni
	SQLUINTEGER uiScroll; // Info sulla possibilità di scroll
	SQLUINTEGER uiDynamic; // Caratteristiche del cursore dinamico
	*/
	SQLRETURN sqlLastError; 
	BOOL fNoErrorView; // Non visualizzare gli errori

} EMUSTRU;

void EmuSelectBuilder(INT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere);

	typedef struct 
	{
	  CHAR szName[64];
	  INT iType;
	  INT iAdbType;
	  CHAR szType[10];
	  INT iSize;
	  CHAR *lpField;

	} EMUFIELD;

	typedef struct 
	{
	  INT iCol; // Colonna nella query
	  CHAR szAdbName[64];
	  CHAR szSQLName[64];
	  INT iAdbType;
	  BOOL fActive; // Se il Binding è attivo (Es [*] non attivo)
	  DWORD dwInd; // ???
	  BOOL fSQLQuote; // Se il valore è da quotare in SQL
	  struct ADB_REC *lpAdbField; // Puntatore alla descrizione Adb del campo
	  
	  void *lpODBCField; // Puntatore al buffer che conterrà il campo
	  INT iODBCType;
	  INT iODBCSize;

	  BOOL fWriteLock; // T/F se il campo è bloccato in scrittura
	  CHAR *lpSpecialTranslate; // Puntatore al una stringa con specifiche per traduzione del campo
	  INT iOffset; // Offset del campo
	  INT iTrunk; // iTrunk dimensione del campo (0=Tutto)
	  BOOL fGhost; // T/F se è un campo FANTASMA e quindi non collegato ad un campo FISICO (serve per le query con altre tabelle o viste logiche)
	  INT iGhostOffset; // Posizione del campo Ghost nel Buffer ADB

	} EMUBIND;


//EMUBIND *AdbBindInfo(INT Hdl,CHAR *lpName);
EMUBIND *AdbBindInfo(INT Hdb,CHAR *lpAdbName,BOOL bError);
void SQLTRY(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult);
CHAR *SQLDisplay(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN rc1);
//void ODBCFieldToAdb(HDB Hdb,EMUBIND *lpEmuBind,CHAR *lpValore,BOOL);
void ODBCFieldToAdb(HDB Hdb, // Handle dbase di riferimento
					EMUBIND *lpEmuBind, // Struttura di bindering
					CHAR *lpValore, // Valore da inserire
					BOOL fTrunk, // Se deve troncare se il caso il valore
					CHAR *lpBufAlternative); // Buffer in ricezione alternativo; NULL = Originale

void EmuHrecMaker(HDB Hdb,CHAR *lpSQLCommand,CHAR *lpBoolean);
void adb_EmuLastWhere(HDB Hdb,CHAR *lpSQLCommand);

INT EmuFindInfo(CHAR *lpObj,INT iStart,INT iEnd);
CHAR *EmuGetInfo(CHAR *lpObj,INT iStart,INT iEnd);
BOOL EmuSetInfo(CHAR *lpObj,INT iStart,INT iEnd,CHAR *lpNewValue);


BOOL adb_ODBCConnect(void);
void adb_ODBCDisconnect(void);
SQLRETURN adb_ODBCArgExecute(INT Hdl,CHAR *Mess,...);
void adb_TagExchange(CHAR *lpSource);

#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
	// Prova di archiviazione dati
	#define ADB_ALFA  0
	#define ADB_NUME  1
	#define ADB_DATA  2
	#define ADB_INT   3 // Intero a 16bit
	#define ADB_FLOAT 4
	#define ADB_BOOL  5 // Valore vero o falso
	#define ADB_COBD  6 // Cobol Decimal  New 2000
	#define ADB_COBN  7 // Cobol Numeric  New 2000
	#define ADB_AINC  8 // AutoIncrement  New 2000
	#define ADB_BLOB  9 // Note da 5 a 32K
	#define ADB_INT32 10 // Intero a 32 bit

	#define HREC LONG
*/
 #include "/easyhand/inc/adbConst.h" // Costanti

	#define B_STANDARD BALANCED_KEYS
	#define B_KEY 1
	#define B_RECORD 0
	#define B_NOSTR  2
	#define ADB_MAX_IDXNAME 20
	#define ADB_MAX_DESC 30
	#define ADB_MAX_INDEX 24

	typedef struct {
		INT iLen;
		INT iOfs;
	} ADB_STRUCTBLOB;

//	#define ADB_KEYSEG_MAX 20
struct FILE_SPECS
{
	 BTI_SINT recLength;
	 BTI_SINT pageSize;
	 BTI_SINT indexCount;
	 BTI_CHAR reserved[ 4 ];
	 BTI_SINT flags;
	 BTI_BYTE dupPointers;
	 BTI_BYTE notUsed;
	 BTI_SINT allocations;
};

struct KEY_SPECS
{
	 BTI_SINT position;
	 BTI_SINT length;
	 BTI_SINT flags;
	 BTI_CHAR reserved[ 4 ];
	 BTI_CHAR type;
	 BTI_CHAR null;
	 BTI_CHAR notUsed[ 2 ];
	 BTI_BYTE manualKeyNumber;
	 BTI_BYTE acsNumber;
};

//-----------------------------------
// Struttura di definizione record  !
//-----------------------------------
#pragma pack(1)
struct ADB_REC {
		INT16 tipo;// Tipo del campo
		CHAR desc[ADB_MAX_DESC]; // Descrizione del campo
		INT16 size; // Lunghezza del campo
		INT16 tipo2; // ON=tutto in maiuscolo,i decimali per i numeri
		INT16 pos; // Posizione nel record (calcolata in adb_crea
		INT16 RealSize;
	 };
//-----------------------------------
// Struttura di definizione indici  !
//-----------------------------------

struct ADB_INDEX_SET {
		CHAR IndexName[ADB_MAX_IDXNAME]; // Key group di riferimento (descrittivo)
		CHAR FieldName[ADB_MAX_DESC]; // nome del campo collegato
		// flag Tipo chiave : OFF- Preset standard ON : settaggio Btrieve
		//										vedi pag. 4-19
		INT16 flag;
		INT16 dup; // ON- Se la chiave Š duplicabile, OFF non lo Š
		INT16 bti_flag;// Flag Btrieve (tipo di campo e ordinamento)
		INT16 keynum;  // Key num in Btreive (automatico)
	 };

struct ADB_INDEX {
		CHAR IndexName[ADB_MAX_IDXNAME]; // Key group di riferimento
		CHAR IndexDesc[ADB_MAX_DESC];
		INT16 KeySeg;
		INT16 KeySize;// Grandezza della chiave di ricerca
	 };

struct ADB_HEAD
{
	CHAR id[11];
	CHAR DescFile[30];
	CHAR eof;
	INT16 Field; // Numero dei campi
	WORD  wRecSize; // Grandezza del record
	INT16 KeyMoreGreat; // Chiave pi— grande presente degli indici
	INT16 Index; // Numero degli indici collegati
	INT16 IndexDesc; // Numero delle descrizione degli indici
	INT16 KeySeg; // Numero dei segmeti collegati
};

struct ADB_HEAD32
{
	CHAR id[11];
	CHAR DescFile[30];
	CHAR eof;
	INT16 Field; // Numero dei campi
	WORD  wRecSize; // Grandezza del record
	INT16 KeyMoreGreat; // Chiave pi— grande presente degli indici
	INT16 Index; // Numero degli indici collegati
	INT16 IndexDesc; // Numero delle descrizione degli indici
	INT16 KeySeg; // Numero dei segmeti collegati
	ULONG ulRecSizeDyn;
};
#pragma pack()


// --------------------------------
// News 99                        |
// --------------------------------

#define ADBF_ELEMMAX 80
typedef struct {

	INT  iType;        // 0=Elemento di comparazione,1=Priorità indice
	CHAR *lpFldName;  // Puntatore al nome del campo
//	CHAR *lpFld;      // Puntatore al campo nel record
	INT  iFldType;     // Tipo di campo (NUME,ALFA)
	BOOL  bMultiComp;   // Multi comparazione TRUE/FALSE
	INT  iCompType;    // Tipo di Comparazione (1=,2<,3> ec..) vedi il .c
	INT  iGrp;  // Gruppo di appartenenza
	double fValore;
	CHAR  szValore[ADBF_ELEMMAX]; // Saparabili da pipe (|)
	
	// new 2007
	BYTE **arFields;
	BYTE **arStringSearch; // Preparato internamente
	BYTE *pBuffer;
} ADBF_ELEM;

#ifdef _WIN32
typedef struct {
	INT  iMode; // 0=Normale,1=Multithread	
	INT  hdlFilter;
	INT  NumFilter;
	INT  MaxFilter;
	INT  NumComp;

	INT  hdlRecords;		// Memoria che contiene l'elenco dei records
	INT  iRecordsReady;	// Record letti
	HREC  *arRecord;		// array dei puntatori al record
	INT  iRecordsMax;

	INT  idxInUse;		// Indice usato per fare la lista
	BOOL  idxFloat;

	INT  GrpNum;
	INT  FirstIndex;
	void  * (*ExtProcess)(INT cmd,LONG info,void *); //  Processo esterno di crezione db
	ADBF_ELEM *Element;

	BOOL  fBreakExtern;
	HWND  hwndProgress; // Progress bar in caso di modalità di MultiThread
} ADB_FILTER;

#else
typedef struct {
	INT  HdlFilter;
	INT  NumFilter;
	INT  MaxFilter;
	INT  NumComp;

	INT  HdlRecord;
	LONG  NumRecord;
	LONG  MaxRecord;
	INT  IndexInUse;  // Indice usato per fare la lista
	BOOL  IndexFloat;

	INT  GrpNum;
	INT  FirstIndex;
	void  * (*ExtProcess)(INT cmd,LONG info,void *); //  Processo esterno di crezione db
	ADBF_ELEM *Element;
} ADB_FILTER;
#endif
// --------------------------------


struct ADB_LINK {
		CHAR FieldNamePadre[ADB_MAX_DESC]; // nome del campo collegato
		CHAR FileFiglio[MAXPATH]; // Nome del file ( si deve trovare nel percoro del padre
		CHAR FieldNameFiglio[ADB_MAX_DESC]; // nome del campo collegato
		CHAR IndexNameFiglio[ADB_MAX_IDXNAME]; // Key group di riferimento
		CHAR Opzione[30]; // Comando al linker : SCAN o +\"string\" o +n
		CHAR Proprietario[30];
	 };

struct ADB_LINK_LIST {
		CHAR FilePadre[MAXPATH]; // Nome del file padre
		CHAR FieldNamePadre[ADB_MAX_DESC]; // nome del campo collegato
		CHAR FileFiglio[MAXPATH]; // Nome del file figlio ( si deve trovare nel percoro del padre
		CHAR FieldNameFiglio[ADB_MAX_DESC]; // nome del campo collegato
		CHAR IndexNameFiglio[ADB_MAX_IDXNAME]; // Key group di riferimento
		CHAR Opzione[30]; // Comando al linker : SCAN o +\"string\" o +n
		INT Clone;
		CHAR Proprietario[30];
	 };

struct ADB_DISP {
		CHAR NomeFld[30];
		INT  px;
		INT  col;
		INT  flag; // SX DX
		INT  offset; // Calcolato da driver
	};


#if (defined(_ADB32)||defined(_ADB32P))

struct ADB_INFO
{
	INT  iHdlRam;
	CHAR *lpFileName; // Nome del file
	BYTE *lpPosBlock; // Area dati per BTRIEVE
	BYTE *lpRecBuffer;// Buffer per i record
	ULONG ulRecBuffer;// Dimensione del buffer

	BYTE *lpKeyBuffer;   // Buffer per le chiavi
	ULONG ulKeyBuffer;

	struct ADB_HEAD *AdbHead;
	ADB_FILTER *AdbFilter; // Puntatore al filtro
	CHAR  OpenCount;
	BOOL fBlobPresent; // Se sono presenti o NO campi Blob Variabili

#ifdef _ADBEMU
	INT	iEmuStartPoint; // Puntatore alla prima riga nel file di emulazione
	INT	iEmuEndPoint;	// Puntatore alla ultima riga nel file di emulazione
	INT	iEmulation;		// T/F Emulazione : Apertura reale SQL
	SQLHSTMT hStmt;			// Handle per i comandi
	INT	iFieldNum;		// Campi letti da SQL
	CHAR	*lpHrecLastWhere; // Posizione nel record ultimo (Testuale per WHERE)
	BOOL	fQueryPending; // T/F se ho una ricerca pendente
	INT	iLastDirection;	// 0=ASC 1=DESC

	// Dizionario "Classico"
	INT	hdlDictionary; // hdl del dizionario
	EMUFIELD *lpEhFld;   // Puntatore ai campi
	INT	iTotMemory;    // Totale di memoria letta
	CHAR	*lpFreeBind;	// Spazio usato per richieste "non conformi" (Es Recnum) (Gestito da ODBC_DoDic..)
	INT	iFreeBindSize;	// Dimensione dello spazio

	// Binding "Statico"
	INT	hdlBind; // hdl del dizionario
	INT	iBindField; // Numero campi nell'array lpEmuBind[]
	EMUBIND *lpEmuBind;
	ULONG	ulRecAddBuffer; // Memoria in più per i campi aggiunti "Fantasma" aggiunti in Binding
	INT	iOfsRecAddBuffer; // Inizio del buffer buono per la scrittura campi fantasma

	// Hrec Virtuale
	CHAR	**lppVirtualHrec;

	// 
#endif

};
void   adb_PreAlloc(ULONG ulDim);

#else
struct ADB_INFO
{
	INT  HdlRam;
	CHAR *FileName; // Nome del file
	CHAR *PosBlock; // Area dati per BTRIEVE
	void *RecBuffer;// Buffer per i record
	void *KeyBuffer;   // Buffer per le chiavi
	struct ADB_HEAD *AdbHead;
	ADB_FILTER *AdbFilter; // Puntatore al filtro
	CHAR  OpenCount;
	BOOL  fBlobPresent; // Se sono presenti o NO campi Blob Variabili
};
#endif

// ---------------------------------------
// Funzione collegate alla gestione ADB  !
// ---------------------------------------

//void   BTI_err(CHAR *file,CHAR *opera,INT  status);

void   adb_errgrave(CHAR *chi,INT  Hadb);
void   adb_start(INT);
void   adb_end(INT);
void   adb_reset(void);
INT   adb_findperc(INT  Hadb,INT  IndexNum,float *Fperc);
INT   adb_getperc(INT  Hadb,INT  IndexNum,float Fperc);
long   adb_recnum(INT  Hadb);

INT   adb_crea(CHAR *file,CHAR *desc,struct ADB_REC *record,struct ADB_INDEX_SET *index,INT FlagFile,BOOL fSoloAdb);
INT   adb_indexnum(INT  Hadb,CHAR *IndexNome);
INT   adb_recsize(INT  Hadb);
INT   adb_GetDynSize(HDB hdb);

void   adb_recreset(INT  Hadb);	// azzera record
INT   adb_keymaxsize(INT  Hadb);
#if (defined(_ADB32)||defined(_ADB32P))
void * adb_DynRecBuf(INT  Hadb); // Ex recbuf
#else
void * adb_recbuf(INT  Hadb); // Ex recbuf
#define adb_DynRecBuf(a) adb_recbuf(a)
#endif
void   adb_DynMemo(BOOL fFlag); // T/F se si vuole la riallocazione dinamica della memoria

void * adb_keybuf(INT  Hadb);
void adb_BufRealloc(HDB hdb,ULONG ulRequestSize,BOOL fCopy,CHAR *lpWho,BOOL);


INT   adb_open(CHAR *file,CHAR *Client,INT  modo,INT  *Hadb);
#ifdef _WIN32
#define ADB_BTI 0
#define ADB_EMU 1
INT   adb_openEx(CHAR *file,CHAR *Client,INT  modo,INT  *Hadb,INT iMode);
#endif
INT   adb_close(INT  Hadb);
INT   adb_closeall(void);

INT	adb_insert(INT  Hadb);
INT	adb_NCCupdate(INT  Hadb);
INT	adb_delete(INT  Hadb);
void	adb_deleteNoLink(INT  Hadb);
void	adb_deleteall(HDB hdbTable,CHAR *pszObjProgress); // 2010 - Incredibile !

INT   adb_Afind(INT  Hadb,CHAR *IndexNome,INT  modo,void *keyfind,INT  flag);
INT   adb_find(INT  Hadb,INT  IndexNum,INT  modo,void *keyfind,INT  flag);

INT   adb_trans(INT  Hadb,INT  flag);

INT   adb_position(INT  Hadb,long *posizione);
INT   adb_get(INT  Hadb,long position,INT  IndexNum);

CHAR *adb_FldInfo(INT  Hadb,CHAR *NomeFld,struct ADB_REC **FldInfo);

double adb_FldNume(INT  Hadb,CHAR *NomeFld);		// x numero
#define adb_FldInt(h,c) (INT)adb_FldNume(h,c)		// x num 32

void * adbFieldPtr(INT Hdb,CHAR *lpField);
#ifdef _ADB32P
void * adb_FldPtr(INT  Hadb,CHAR *NomeFld);		// x stringhe
#else
#define adb_FldPtr(a,b) adbFieldPtr(a,b)
#endif

void   adb_FldCopy(void *FldPtr,INT  Hadb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume);
void   adb_FldWrite(INT  Hadb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume);
BOOL   adb_FldWriteCheck(INT Hadb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume,BOOL fView,INT *lpiError); // New 2002/10
INT   adb_FldReset(INT  Hadb,CHAR *NomeFld);
INT   adb_FldOffset(INT  Hadb,CHAR *NomeFld);
INT   adb_FldSize(INT  Hadb,CHAR *NomeFld);
CHAR *adb_data(CHAR *ptd);
CHAR *adb_GetDate(HDB Hdb,CHAR *Nome);

CHAR   *adb_HookTouch(INT Hadb,HREC hRec,struct ADB_HOOK *Hook); // new 2001
INT   adb_HookWrite(INT  Hadb,struct ADB_HOOK *Hook);
INT   adb_HookGet(INT  Hadb,struct ADB_HOOK *Hook);
INT   adb_HookInsert(INT  Hadb,struct ADB_HOOK *Hook);
INT   adb_HookUpdate(INT  Hadb,struct ADB_HOOK *Hook);
INT   adb_update(INT  Hadb);

//		 Gestione del proprietario
INT   adb_OwnerSet(INT  Hadb,CHAR *Client,INT  Mode);
INT   adb_OwnerClear(INT  Hadb);
void   adb_ErrorView(BOOL fError); // New99


//		Gestione DB Gerachico
void   adb_HLfile(CHAR *file);
//INT   adb_HLapri(void);
INT   adb_HLapri(void);
void   adb_HLcrea(CHAR *FilePadre,struct ADB_LINK *link);

#define HC_BACKUP  100
#define HC_RESTORE 101
#define HC_RESET   102
void   HookCopy(INT  cmd,INT  Hdb);

// Sono in \EHTOOL\ADBSFIND.C (Special find)
INT   adb_Sfind(INT Hdb,INT Idx,INT modo,INT NumPt,...);
INT   adb_ASfind(INT Hdb,CHAR *NomeIndex,INT modo,INT NumPt,...);
INT   adb_HdlFind(CHAR *File,BOOL Flag);

BOOL   adb_OpenFind(CHAR *File,
					CHAR *User,
					INT FlagOpen,
					INT *Hdb,
					BOOL *DaChiudere);
INT adb_Sopen(CHAR *file,CHAR *Client,INT modo,INT *Hadb);

void adb_CloseFind(INT Hdb);
void adb_CloseAllFind(void);
LONG adb_Filter(HDB hdb,INT iCmd,CHAR *pCampo,CHAR *pAzione,void *pValore);


// New 2000
CHAR *adb_MakeNumeric(double dValore,INT iSize,INT iDec,BOOL,CHAR * pszWho);

CHAR *adb_MakeCobolNumeric(double dValore,INT iSize,INT iDec);
double adb_GetCobolNumeric(BYTE *lpStr,INT Size,INT iControlDigit,INT iDec);

BYTE *adb_MakeCobolDecimal(double dValore,INT iSize,INT iDec);
double adb_GetCobolDecimal(BYTE *lpStr,INT iSize,INT iControlDigit,INT iDec);


// Gestione del Tipo BLOB
BOOL adb_isBlob(HDB Hdb,CHAR *lpField); // new 9/2007
BOOL adb_isField(HDB Hdb,CHAR *lpField); // new 9/2007

INT adb_BlobSize(HDB Hdb,CHAR *Name); // Ritorna la dimensione del campo Blob
BOOL adb_BlobCopy(HDB Hdb,CHAR *Name,CHAR *lpBuffer,INT Size); // Copia il BLOB letto in un Buffer
CHAR *adb_BlobGetAlloc(HDB Hdb,CHAR *lpField);
void adb_BlobFree(CHAR *lpField);


#ifdef _WIN32
BYTE *adb_GetAlloc32(INT Hadb,HREC position);
HDB adb_OpenClone(HDB Hdb);
CHAR *adb_GetDescIndex(HDB Hadb,INT IndexNum);
BOOL adb_FldExist(INT Hadb,CHAR *NomeFld);
//BOOL adb_FldImport(HDB hdbDest,HDB hdbSource,BOOL fRealloc);
BOOL adb_FldImport(HDB hdbDest,HDB hdbSource,BOOL fRealloc,BOOL fCutSpace);
BYTE *adb_InfoTable(HDB hdb);
EH_DATATYPE adb_TypeToDataType(EN_FLDTYPE enType) ; // 2011
// 

#pragma pack (1)

	// ---------------------------------------
    // FILE.DDF structure
	//
	typedef struct {
		INT16 siFileId; // Enumeratore di file
		CHAR  szTableName[20];
		CHAR  szTableLocation[64];
		BYTE  bFileFlags;
		BYTE  Reserved[10];
	} DDF_FILE;

	// ---------------------------------------
    // FIELD.DDF structure
	//
	typedef struct {
		INT16 siFieldId; // Enumeratore di campo
		INT16 siFileId;  // Riferimento al file
		CHAR  szFieldName[20];
		BYTE  bDataTypeCode;
		INT16 siFieldOffset;
		INT16 siFieldSize;
		BYTE  bDecimalDelimiter;
		INT16 siCaseFlags;
	} DDF_FIELD;

	// ---------------------------------------
    // INDEX.DDF structure
	//
	typedef struct {
		INT16 siFileId; 
		INT16 siFieldId; 
		INT16 siIndexNumber; 
		INT16 siSegmentPart; 
		INT16 siBtrieveFlags;
	} DDF_INDEX;

#pragma pack ()

#endif

#ifdef __cplusplus
}
#endif

#ifdef _ADBEMU
void adb_EmuFile(CHAR *lpFile);
void adb_EmuRetag(void);
void adb_SQLExecute(HDB hdb,CHAR *lpWhoIs,CHAR *lpSQLCommand);
SQLRETURN adb_ODBCArgExecute(INT Hdl,CHAR *Mess,...);
BOOL adb_ODBCNext(INT Hdb);
CHAR *SQLGetLastError(SQLSMALLINT iType,SQLHSTMT hstmt,SQLRETURN rc1,SQLINTEGER *lpNativeError,SQLCHAR  *lpSqlState);
#endif
BOOL adb_DmiDelete(DRVMEMOINFO *lpDmi,HDB Hdb);



// 
#ifdef EH_MAIN
	struct ADB_INFO * ADB_info=NULL;
	INT		ADB_hdl=-1;
	INT		ADB_ult=0;
	INT		ADB_max=16; // Numeri di Db apribili contemporanemante
	INT		ADB_network=OFF; // ON/OFF se si sta usando un NetWork
	INT		ADB_lock=1;// 0 Ritorna senza segnalare l'errore
									// 1 Riprova e segnala che stai riprovando
									// 2 Riprova senza segnalare che stai riprovando
									// 3 Chiede all'operatore se devere riprovare
	INT		ADB_iLastStatus=0; // new 8/2007
	BOOL	fCobolLimitControl=FALSE;
	INT		ADB_MaxBlobSize=2048;
#else

	extern struct ADB_INFO * ADB_info;
	extern INT		ADB_hdl;
	extern INT		ADB_ult;
	extern INT		ADB_max; // Numeri di Db apribili contemporanemante
	extern INT		ADB_network; // ON/OFF se si sta usando un NetWork
	extern INT		ADB_lock;// 0 Ritorna senza segnalare l'errore
									// 1 Riprova e segnala che stai riprovando
									// 2 Riprova senza segnalare che stai riprovando
									// 3 Chiede all'operatore se devere riprovare
	extern INT		ADB_iLastStatus; // new 8/2007
	extern BOOL		fCobolLimitControl;
	extern INT		ADB_MaxBlobSize;

#endif