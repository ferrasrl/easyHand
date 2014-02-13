//ีอออออออออออออออออออออออออออออออออออออออออธ
//ณ FLM_ADB.H - Header per la gestione dei  ณ
//ณ             file adb/Btrieve            ณ
//ณ                                         ณ
//ณ          by Ferr… Art & Technology 1996 ณ
//ิอออออออออออออออออออออออออออออออออออออออออพ

#define SEFS    -32
#define MEFS    -64
typedef int	   HDB;

#define ADBNULL 0


//	by DAM (inizio)
#define SINGLE		1 
#define MULTIPLE	2 
#define ALL			3

SINT adb_unlock(SINT tipo,SINT Hdb,LONG position,SINT index);
//	by DAM (fine)



#ifdef _WIN32
 #define BTI_WIN_32 1
 #include "\ehtool\include\btrapi.h"
 #include "\ehtool\include\btrconst.h"
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
typedef struct {
	SINT hdlFileEmu; // Handle del file Emu
	SINT iRows; // Numero di righe nel file Emu
	CHAR **lpRows; // Puntatore alle righe
	SINT iODBCCountOpen; // Contatore delle connessioni aperte ad ODBC

	CHAR szDNSServerName[80];
	CHAR szDNSUserName[80];
	CHAR szDNSPassword[80];
	
	SQLHENV hEnv; // Ambiente
	SQLHDBC hConn;
	SQLRETURN sqlLastError; 
	BOOL fNoErrorView; // Non visualizzare gli errori

	// Informazioni sul dbase
	SQLUINTEGER uiAsync;  // Info sulla possibilitเ di eseguire comandi Asincroni
	SQLUINTEGER uiScroll; // Info sulla possibilitเ di scroll
	SQLUINTEGER uiDynamic; // Caratteristiche del cursore dinamico

} EMUSTRU;

void EmuSelectBuilder(SINT Hdb,CHAR *lpSQLCommand,CHAR *lpWhere);

	typedef struct 
	{
	  CHAR szName[64];
	  SINT iType;
	  SINT iAdbType;
	  CHAR szType[10];
	  SINT iSize;
	  CHAR *lpField;
	} EMUFIELD;

	typedef struct 
	{
	  SINT iCol; // Colonna nella query
	  CHAR szAdbName[64];
	  CHAR szSQLName[64];
	  SINT iAdbType;
	  BOOL fActive; // Se il Binding ่ attivo (Es [*] non attivo)
	  DWORD dwInd; // ???
	  BOOL fSQLQuote; // Se il valore ่ da quotare in SQL
	  struct ADB_REC *lpAdbField; // Puntatore alla descrizione Adb del campo
	  
	  void *lpODBCField; // Puntatore al buffer che conterrเ il campo
	  SINT iODBCType;
	  SINT iODBCSize;

	  BOOL fWriteLock; // T/F se il campo ่ bloccato in scrittura
	  CHAR *lpSpecialTranslate; // Puntatore al una stringa con specifiche per traduzione del campo
	  SINT iOffset; // Offset del campo
	  SINT iTrunk; // iTrunk dimensione del campo (0=Tutto)
	  BOOL fGhost; // T/F se ่ un campo FANTASMA e quindi non collegato ad un campo FISICO (serve per le query con altre tabelle o viste logiche)
	  SINT iGhostOffset; // Posizione del campo Ghost nel Buffer ADB

	} EMUBIND;


EMUBIND *AdbBindInfo(SINT Hdl,CHAR *lpName);
void SQLTRY(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN iResult);
CHAR *SQLDisplay(SQLSMALLINT iType,CHAR *WhoIs,SQLHSTMT hstmt,SQLRETURN rc1);
//void ODBCFieldToAdb(HDB Hdb,EMUBIND *lpEmuBind,CHAR *lpValore,BOOL);
void ODBCFieldToAdb(HDB Hdb, // Handle dbase di riferimento
					EMUBIND *lpEmuBind, // Struttura di bindering
					CHAR *lpValore, // Valore da inserire
					BOOL fTrunk, // Se deve troncare se il caso il valore
					CHAR *lpBufAlternative); // Buffer in ricezione alternativo; NULL = Originale

void EmuHrecMaker(HDB Hdb,CHAR *lpSQLCommand,CHAR *lpBoolean);
void EmuLastWhere(HDB Hdb,CHAR *lpSQLCommand);

SINT EmuFindInfo(CHAR *lpObj,SINT iStart,SINT iEnd);
CHAR *EmuGetInfo(CHAR *lpObj,SINT iStart,SINT iEnd);
BOOL EmuSetInfo(CHAR *lpObj,SINT iStart,SINT iEnd,CHAR *lpNewValue);


BOOL adb_ODBCConnect(void);
void adb_ODBCDisconnect(void);
SQLRETURN adb_ODBCArgExecute(SINT Hdl,CHAR *Mess,...);
void adb_TagExchange(CHAR *lpSource);

#endif

#ifdef __cplusplus
extern "C" {
#endif

	// Prova di archiviazione dati
	#define ADB_ALFA  0
	#define ADB_NUME  1
	#define ADB_DATA  2
	#define ADB_INT   3 // Intero a 16bit
	#define ADB_FLOAT 4
	#define ADB_FLAG  5
	#define ADB_COBD  6 // Cobol Decimal  New 2000
	#define ADB_COBN  7 // Cobol Numeric  New 2000
	#define ADB_AINC  8 // AutoIncrement  New 2000
	#define ADB_BLOB  9 // Note da 5 a 32K
	#define ADB_INT32 10 // Intero a 32 bit

	#define B_STANDARD BALANCED_KEYS

	#define ADB_MAX_IDXNAME 20
	#define ADB_MAX_DESC 30
	#define ADB_MAX_INDEX 24

	#define B_KEY 1
	#define B_RECORD 0
	#define B_NOSTR  2

	#define HREC LONG

	typedef struct {
		SINT iLen;
		SINT iOfs;
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
		INT16 dup; // ON- Se la chiave  duplicabile, OFF non lo 
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

	SINT  Type;        // 0=Elemento di comparazione,1=Priorit… indice
	CHAR *lpFldName;  // Puntatore al nome del campo
//	CHAR *lpFld;      // Puntatore al campo nel record
	SINT  FldType;     // Tipo di campo (NUME,ALFA)
	BOOL  MultiComp;   // Multi comparazione TRUE/FALSE
	SINT  CompType;    // Tipo di Comparazione (1=,2<,3> ec..) vedi il .c
	SINT  Grp;  // Gruppo di appartenenza
	double fValore;
	CHAR Valore[ADBF_ELEMMAX]; // Saparabili da pipe (|)
} ADBF_ELEM;

#ifdef _WIN32
typedef struct {
	SINT  HdlFilter;
	SINT  NumFilter;
	SINT  MaxFilter;
	SINT  NumComp;

	SINT  HdlRecord;
	HREC  *lpList;
	BOOL  fBreakExtern;
	LONG  NumRecord;
	LONG  MaxRecord;
	SINT  IndexInUse;  // Indice usato per fare la lista
	BOOL  IndexFloat;

	SINT  GrpNum;
	SINT  FirstIndex;
	void  * (*ExtProcess)(SINT cmd,LONG info,void *); //  Processo esterno di crezione db
	ADBF_ELEM *Element;
} ADB_FILTER;
#else
typedef struct {
	SINT  HdlFilter;
	SINT  NumFilter;
	SINT  MaxFilter;
	SINT  NumComp;

	SINT  HdlRecord;
	LONG  NumRecord;
	LONG  MaxRecord;
	SINT  IndexInUse;  // Indice usato per fare la lista
	BOOL  IndexFloat;

	SINT  GrpNum;
	SINT  FirstIndex;
	void  * (*ExtProcess)(SINT cmd,LONG info,void *); //  Processo esterno di crezione db
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
		SINT Clone;
		CHAR Proprietario[30];
	 };

struct ADB_DISP {
		CHAR NomeFld[30];
		SINT  px;
		SINT  col;
		SINT  flag; // SX DX
		SINT  offset; // Calcolato da driver
	};


#ifdef _ADB32

struct ADB_INFO
{
	SINT  iHdlRam;
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
	SINT	iEmuStartPoint; // Puntatore alla prima riga nel file di emulazione
	SINT	iEmuEndPoint;	// Puntatore alla ultima riga nel file di emulazione
	SINT	iEmulation;		// T/F Emulazione : Apertura reale SQL
	SQLHSTMT hStmt;			// Handle per i comandi
	SINT	iFieldNum;		// Campi letti da SQL
	CHAR	*lpHrecLastWhere; // Posizione nel record ultimo (Testuale per WHERE)
	BOOL	fQueryPending; // T/F se ho una ricerca pendente
	SINT	iLastDirection;	// 0=ASC 1=DESC

	// Dizionario "Classico"
	SINT	hdlDictionary; // hdl del dizionario
	EMUFIELD *lpEhFld;   // Puntatore ai campi
	SINT	iTotMemory;    // Totale di memoria letta
	CHAR	*lpFreeBind;	// Spazio usato per richieste "non conformi" (Es Recnum) (Gestito da ODBC_DoDic..)
	SINT	iFreeBindSize;	// Dimensione dello spazio

	// Binding "Statico"
	SINT	hdlBind; // hdl del dizionario
	SINT	iBindField; // Numero campi nell'array lpEmuBind[]
	EMUBIND *lpEmuBind;
	ULONG	ulRecAddBuffer; // Memoria in pi๙ per i campi aggiunti "Fantasma" aggiunti in Binding
	SINT	iOfsRecAddBuffer; // Inizio del buffer buono per la scrittura campi fantasma

	// Hrec Virtuale
	CHAR	**lppVirtualHrec;

	// 
#endif

};
void   adb_PreAlloc(ULONG ulDim);

#else
struct ADB_INFO
{
	SINT  HdlRam;
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

void   BTI_err(CHAR *file,CHAR *opera,SINT  status);

void   adb_errgrave(CHAR *chi,SINT  Hadb);
void   adb_start(SINT);
void   adb_end(void);
void   adb_reset(void);
SINT   adb_findperc(SINT  Hadb,SINT  IndexNum,float *Fperc);
SINT   adb_getperc(SINT  Hadb,SINT  IndexNum,float Fperc);
long   adb_recnum(SINT  Hadb);

SINT   adb_crea(CHAR *file,CHAR *desc,struct ADB_REC *record,struct ADB_INDEX_SET *index,SINT  FlagFile);
SINT   adb_indexnum(SINT  Hadb,CHAR *IndexNome);
SINT   adb_recsize(SINT  Hadb);
SINT   adb_GetDynSize(HDB hdb);

void   adb_recreset(SINT  Hadb);	// azzera record
SINT   adb_keymaxsize(SINT  Hadb);
#ifdef _ADB32
void * adb_DynRecBuf(SINT  Hadb); // Ex recbuf
#else
void * adb_recbuf(SINT  Hadb); // Ex recbuf
#define adb_DynRecBuf(a) adb_recbuf(a)
#endif
void   adb_DynMemo(BOOL fFlag); // T/F se si vuole la riallocazione dinamica della memoria

void * adb_keybuf(SINT  Hadb);
void   adb_BufRealloc(HDB hdb,ULONG ulNewSize);

SINT   adb_open(CHAR *file,CHAR *Client,SINT  modo,SINT  *Hadb);
#ifdef _WIN32
#define ADB_BTI 0
#define ADB_EMU 1
SINT   adb_openEx(CHAR *file,CHAR *Client,SINT  modo,SINT  *Hadb,SINT iMode);
#endif
SINT   adb_close(SINT  Hadb);
SINT   adb_closeall(void);

SINT   adb_insert(SINT  Hadb);
SINT   adb_NCCupdate(SINT  Hadb);
SINT   adb_delete(SINT  Hadb);
void   adb_deleteNoLink(SINT  Hadb);

SINT   adb_Afind(SINT  Hadb,CHAR *IndexNome,SINT  modo,void *keyfind,SINT  flag);
SINT   adb_find(SINT  Hadb,SINT  IndexNum,SINT  modo,void *keyfind,SINT  flag);

SINT   adb_trans(SINT  Hadb,SINT  flag);

SINT   adb_position(SINT  Hadb,long *posizione);
SINT   adb_get(SINT  Hadb,long position,SINT  IndexNum);

CHAR *adb_FldInfo(SINT  Hadb,CHAR *NomeFld,struct ADB_REC **FldInfo);

double adb_FldNume(SINT  Hadb,CHAR *NomeFld);		// x numero
#define adb_FldInt(h,c) (SINT)adb_FldNume(h,c)		// x num 32
void * adb_FldPtr(SINT  Hadb,CHAR *NomeFld);		// x stringhe

void   adb_FldCopy(void *FldPtr,SINT  Hadb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume);
void   adb_FldWrite(SINT  Hadb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume);
BOOL   adb_FldWriteCheck(SINT Hadb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume,BOOL fView,SINT *lpiError); // New 2002/10
SINT   adb_FldReset(SINT  Hadb,CHAR *NomeFld);
SINT   adb_FldOffset(SINT  Hadb,CHAR *NomeFld);
SINT   adb_FldSize(SINT  Hadb,CHAR *NomeFld);
CHAR *adb_data(CHAR *ptd);
CHAR *adb_GetDate(HDB Hdb,CHAR *Nome);

CHAR   *adb_HookTouch(SINT Hadb,HREC hRec,struct ADB_HOOK *Hook); // new 2001
SINT   adb_HookWrite(SINT  Hadb,struct ADB_HOOK *Hook);
SINT   adb_HookGet(SINT  Hadb,struct ADB_HOOK *Hook);
SINT   adb_HookInsert(SINT  Hadb,struct ADB_HOOK *Hook);
SINT   adb_HookUpdate(SINT  Hadb,struct ADB_HOOK *Hook);
SINT   adb_update(SINT  Hadb);

//		 Gestione del proprietario
SINT   adb_OwnerSet(SINT  Hadb,CHAR *Client,SINT  Mode);
SINT   adb_OwnerClear(SINT  Hadb);
void   adb_ErrorView(BOOL fError); // New99


//		Gestione DB Gerachico
void   adb_HLfile(CHAR *file);
//SINT   adb_HLapri(void);
SINT   adb_HLapri(void);
void   adb_HLcrea(CHAR *FilePadre,struct ADB_LINK *link);

#define HC_BACKUP  100
#define HC_RESTORE 101
#define HC_RESET   102
void   HookCopy(SINT  cmd,SINT  Hdb);

// Sono in \EHTOOL\ADBSFIND.C (Special find)
SINT   adb_Sfind(SINT Hdb,SINT Idx,SINT modo,SINT NumPt,...);
SINT   adb_ASfind(SINT Hdb,CHAR *NomeIndex,SINT modo,SINT NumPt,...);
SINT   adb_HdlFind(CHAR *File,BOOL Flag);

BOOL   adb_OpenFind(CHAR *File,
					CHAR *User,
					SINT FlagOpen,
					SINT *Hdb,
					BOOL *DaChiudere);
SINT adb_Sopen(CHAR *file,CHAR *Client,SINT modo,SINT *Hadb);

void adb_CloseFind(SINT Hdb);
void adb_CloseAllFind(void);
LONG adb_Filter(SINT Hdb,SINT cmd,CHAR *Campo,CHAR *Azione,void *Valore);


// New 2000
CHAR *adb_MakeNumeric(double dValore,SINT iSize,SINT iDec,BOOL);

CHAR *adb_MakeCobolNumeric(double dValore,SINT iSize,SINT iDec);
double adb_GetCobolNumeric(BYTE *lpStr,SINT Size,SINT iDec);

BYTE *adb_MakeCobolDecimal(double dValore,SINT iSize,SINT iDec);
double adb_GetCobolDecimal(BYTE *lpStr,SINT iSize,SINT iDec);

// Gestione del Tipo BLOB
SINT adb_BlobSize(HDB Hdb,CHAR *Name); // Ritorna la dimensione del campo Blob
BOOL adb_BlobCopy(HDB Hdb,CHAR *Name,CHAR *lpBuffer,SINT Size); // Copia il BLOB letto in un Buffer
CHAR *adb_BlobGetMalloc(HDB Hdb,CHAR *lpField);

#ifdef _WIN32
BYTE *adb_GetAlloc32(SINT Hadb,HREC position);
HDB adb_OpenClone(HDB Hdb);
CHAR *adb_GetDescIndex(HDB Hadb,SINT IndexNum);
BOOL adb_FldExist(SINT Hadb,CHAR *NomeFld);
BOOL adb_FldImport(HDB hdbDest,HDB hdbSource,BOOL fRealloc);
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
SQLRETURN adb_ODBCArgExecute(SINT Hdl,CHAR *Mess,...);
BOOL adb_ODBCNext(SINT Hdb);
CHAR *SQLGetLastError(SQLSMALLINT iType,SQLHSTMT hstmt,SQLRETURN rc1,SQLINTEGER *lpNativeError,SQLCHAR  *lpSqlState);
#endif
