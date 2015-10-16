//-----------------------------------------------------------------------
//	FormulaProcess  Trasforma la formula in un numero                   |
//											Creato da G.Tassistro       |
//											Ferrà Art & Tecnology 1999  |
//  Ritorna TRUE = Formula errata                                       |
//          FALSE= Formula OK Valore messo in "Valore"                  |
//-----------------------------------------------------------------------


#define SCRIPT_FUNC_EXT void * (*funcExt)(struct _script * psScript,EN_MESSAGE enMess,CHAR * pszToken,void * pVoid)

// Tipi di elementi possibili in whatIsThis
typedef enum {
	
	E_VARUNKNOW=-1,
	E_ERROR=0,
	E_NUMDEC=1,
	E_NUMHEX=2,
	E_NUMBIN=3,
	E_TEXT=10,
	E_TEXT_SUM=11,	// Somma di stringhe probabile
	E_FORMULANUM=20,
	E_VARNUM=30,
	E_VARTEXT=31,
	E_INTFUNC=33,
	VARSIZE=32

} EN_TAG_TYPE;

// Translate Group
#define TG_GLOBAL 0x1000 
#define TG_SELECTION 0x2000
#define TG_ACTION 0x4000

// Translate Element
typedef enum {

	TE_FUNCTION=0x01,
	TE_CONST=0x02,
	TE_VARIABLE=0x04,
	TE_STANTMENT=0x08

} EN_TAGTYPE;

typedef struct 
{
	EN_TAGTYPE		enTypes;
	INT				iCode;
	CHAR *			pszName;

} S_SCRIPT_TAG; // TRANGLANG


typedef enum {

	SER_OK,
	SER_SINTAX,		// Sintax Errore
	SER_VAR_UNKNOW,	// Variabile sconosciuta
	SER_EXTERN,		// Errore verificato in funzione di notifica esterna
	SER_VAR_EXT_UNKNOW,	// Variabile sconosciuta
	SER_OBJ_UNKNOW

} EN_SER;

typedef struct {

	// Cosa mi determina la fine della porzione di codice processato
	BOOL	fBracketEnd; // T/F bracket end
	BYTE *	pszStart;
	BYTE *	pszEnd;

} S_SCPOS;



 struct _script {

	S_UNIVAL *	psRet; // Return dello script

	// Valore e funzione esterna
	void *		pVoid;	
	SCRIPT_FUNC_EXT;

	// Errori
	EN_SER		enError;	// Errore di ritorno
	EH_LST		lstError;
	//CHAR		szError[400];
	//EH_LST		lstError;
	INT			iErrorExtra;	// Informazioni aggiuntiva numeriche per errori esterni

	// Gestione variabili
	_DMI		dmiVar;
	S_SCRIPT_TAG *	arsTag;

	// Altro
	BOOL		bVerbose;		// T/F se si vuole output di debug
	EH_LST		lstPos;
	EH_LST		lstNote;
	BOOL		bTrace; // Visualizza le righe
	INT			iRow;	// Riga in analisi
	INT			iRowNext;	// Riga in analisi
	CHAR		szToken[1024];	// Ultimo token
	

};
typedef struct _script S_SCRIPT;

typedef struct {
	
	CHAR		szNome[32];
	//double dValore;
	S_UNIVAL *	psRet;

} S_VAR;

S_SCRIPT *		scriptCreate(S_SCRIPT * psSource);
S_UNIVAL *		scriptExecute(S_SCRIPT * psScript,CHAR * pszTextScript);
S_SCRIPT *		scriptDestroy(S_SCRIPT * psScript);

//S_UNIVAL *  scriptGetValue(S_SCRIPT * psScript,CHAR * pszTextScript);
S_SCRIPT_TAG * scriptTag(S_SCRIPT_TAG * psArrayTag,EN_TAGTYPE enGroup,CHAR * pszToken);
S_UNIVAL *		scriptGetValue(S_SCRIPT * psScript,CHAR * pszFormula);
//EH_AR		scriptFuncArg(BYTE *pString,BYTE *pCharStart,BYTE *pCharStop,SINT *piError,SINT *piArgNum); // new 2009

typedef struct {
	
	EN_TAG_TYPE enType;
	CHAR * pszValue;
} S_ARG_INFO;

EH_LST		scriptFuncArg(BYTE *pString,BYTE *pCharStart,BYTE *pCharStop,INT *piError, CHAR ** ppEnd); // 2013
EH_LST		scriptFuncDestroy(EH_LST lst);
S_UNIVAL *	scriptFuncGet(S_SCRIPT * psScript,EH_LST lstParams,INT idx,EH_DATATYPE enDaTy);

CHAR *		scriptExtract(CHAR *lpSource,CHAR *lpTesta,CHAR *lpCoda); // Isola
CHAR *		scriptParExtract(CHAR *lpSource); // Ex IsolaPar
CHAR *		scriptNext(CHAR *lpStart,CHAR cCosa); // FindNext
void *		scriptError(S_SCRIPT * psScript,EN_SER enErr,CHAR * pszFormat,...);
void		scriptShowErrors(S_SCRIPT * psScript);
