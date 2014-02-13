//
// Soap.h 
// Funzioni di supporto per il protocollo SOAP
//
/*
#ifndef EH_WEBTOOL
#include "/easyhand/ehtool/webtool.h"
#endif
*/
#ifndef EH_XMLUTIL
#include "/easyhand/inc/xmlParser.h"
#endif


#define SOAP_USER_AGENT "FerraSoap/1.0"

typedef enum 
{
	SE_OK=0,
	SE_SINTAX_ERROR,	// Error di sintassi nella funzione
	SE_SINTAX_PARAM_ERROR,// Errore di sintassi nella dichiarazione dei parametri
	SE_MISSING_PARAMETERS, // Parametri mancanti
	SE_XML_ERROR // Parametri mancanti
} SoapError;

typedef enum 
{
	STE_UNKNOW=0,
	STE_SIMPLE,		// Es decimal, string
	STE_COMPLEX,	// Dato strutturato (solo sequenza)
//	STE_ARRAY,		// Array di dati
	STE_ELEMENT,	// Insieme di dati complessi
	STE_REF,		// REF: Referement > indicazione di struttura non definita (da definire all'arrivo dei dati)
	STE_DEFINE		// 2010 - Macro simile al C

} EleType;

typedef enum 
{
	CTE_EMPTY=0,
	CTE_SIMPLE,		// Es decimal, string
	CTE_COMPLEX,	// Dato strutturato (solo sequenza)
	CTE_ARRAY,		// Array di dati
//	CTE_ELEMENT,	// Insieme di dati complessi
} CntType;


typedef enum 
{
	STA_NOT_ARRAY=0,
	STA_ARRAY,		
	STA_ARRAY_ELEMENT,	
} EleArray;

typedef enum 
{
	SE_UNDEFINED=0,
	SE_VAR_TO_XML,	// Il valore della variabile viene trasformato in XML
	SE_XML_TO_VAR,  // Il valore letto nell'xml viene traformato in un valore variabile
	SE_VAR_TO_STR,	// Il valore della variabile viene trasformato un valore stringa

} XEDType;

// Descrizione di un elemento di un dato complesso (Struttura)
typedef struct {
	BOOL	bAlloc;		// T/F se l'elemento è da rilasciare

	XNS_PTR	xnsName;		// Nome dell'elemento
	XNS_PTR	xnsType;		// Attributo: type		Tipo dell'elemento
	XNS_PTR	xnsArrayType;	// Attributo: arrayType

	//	CHAR *	pszName;	// Nome dell'elemento
	// Attributo tipo (in caso di arrayType = NS:Tipo dell' elemento dell'array)
//	CHAR *	pszNSType;		   // NameSpace (fatto) da associare in caso di encoding
//	CHAR *	pszTypeOnly;	   // Tipo XML (senza name space) ---> è psxType->pszType

	CHAR *	pszOriginalType;   // Tipo letto originale XML
//	CHAR *	pszNSTypeAddress;  // Indirizzo URI del name space del tipo

	EleType		iType;	
	EleArray	iArrayType;

//	BOOL	bShowArrayType;		// T/F se devo mostrare l'array type
	BOOL	bComplexContent;	// T/F se ha un complexContent
//	void *  psComplex;	// Assume valori diversi a seconda del tipo, Descrizione dell'elemento complesso (struttura); NULL= DataType standard singolo
	SINT	iElements;	// Numero degli elementi (figli) che compongo l'elemento
	void	*arsElement; // sovrebbe essere EHS_ELEMENT * (ma non posso farlo)

	CHAR *	pMinOccurs; // Minimo (non considerato)
	CHAR *	pMaxOccurs;	// Massimo (>1 o "unbounded") = Array
	//CHAR *	pNillable;
	BOOL	bNillable;
	// CHAR *  pRef; // Ref > Refers to the name of another element ,This attribute cannot be used if the parent element is the schema element. (http://www.w3schools.com/schema/el_element.asp)
} EHS_ELEMENT;

/*
typedef struct {
	SINT	iElements;
	EHS_ELEMENT	*arElement; // array con definizione degli elementi
} EHS_COMPLEX;
*/

typedef struct {
	CntType iCntType;		
	EleType	iElementType; // Copia del tipo dell'elemento originale
	SINT	iLength;	// Lunghezza (numero di elementi se array o array di tipo complesso)
	CHAR	szLength[10];	// Mi serve per il ritorno (della lunghezza) se richiesta
	void *	pValue;		// NOTA: 
						// Se datosingolo ptr a stringa							iLength=Lunghezza dato
						// se array puntatore a array di stringhe				iLength=Numero di elementi
						// se complesso, puntatore ad array di elementi array	iLength=Numero di elementi
} EHS_CONTAINER;

typedef struct {
	EHS_ELEMENT sElement;			// Descrizione del primo elemento o elementi
	EHS_CONTAINER sContainer;		// Contenitore dei dati della variabile
} EHS_VAR;

typedef struct {
	EHS_ELEMENT *psElement;			// Descrizione del primo elemento o elementi
	EHS_CONTAINER *psContainer;		// Contenitore dei dati della variabile
} EHS_VARPTR;


typedef enum 
{
	SVE_OK=0,
	SVE_VAR_UNKNOW,
	SVE_NO_MEMBER,
	SVE_MEMBER_UNKNOW,
	SVE_INDEX_NEEDED,
	SVE_NO_ARRAY,
	SVE_INDEX_ERROR,
	SVE_INDEX_OVERSIZE,
	SVE_SINTAX_ERROR

} SoapVarError;

// http://www.ibm.com/developerworks/webservices/library/ws-whichwsdl/
typedef enum 
{
	BODY_USE_ENCODED=1000,
	BODY_USE_LITERAL

} EN_BODYUSE;

typedef struct {
	SoapVarError iErrorCode;	// Errore in interpretazione
	EleArray	iArrayType;		// Ritorna tipo array
	CHAR *pszErrorDesc;	// Descrizione dell'errore

	EHS_ELEMENT *psVirtualElement; // Elemento virtuale in ritorno
	EHS_CONTAINER *psContainer;
	CHAR *pStringValue; // Valore di ritorno (sempre stringa)

} EHS_ELEMENT_INFO;

typedef struct {
	BOOL	bIndent;
	BOOL	bCRLF;
	BOOL	bNameSpace;
} EHS_XMLSOAP_REMOVE;

typedef struct {
	BOOL		bReady;

	BYTE	*	pszWSDLSource;
	BYTE	*	pszLastOperation;
	BYTE	*	pszLastOperationTime;

	CHAR *		pUseUriAddress; // [in] UriAddress alternativo del servizio
	EN_BODYUSE	enInputBodyUse;			
	EN_BODYUSE	enInputBodyUseForce; // [in] Chiede di forzare l'input body use Es. VARIABLE_TO_XML_LITERAL (per velocizzare le comunicazioni)
	BOOL		bVarNullLikeBlank;	// T/F se deve ritornare le variabili NULL come "" 

	// Dati caricati dalle chiamate solo [Out]
	SoapError	enSoapErrorCode;
	XMLDOC		xmlWSDL;
	XMLELEMENT *pxmlDefinition;
	BYTE *		pTargetNamespace;
	XMLARRAY	arSchema;	
	XMLARRAY	arMessage;	
	XMLARRAY	arPortType;
	XMLARRAY	arBinding;
	XMLARRAY	arService;
	XMLARRAY	arOperation;
	_DMI		dmiVar;
	EHS_VAR	*	arVar;
	_DMI		dmiNamespace;
	_DMI		dmiCookie;

	// Internal
	SINT		iParamGet; // Contatore del parametro letto

	// Dopo una chiamata
	SINT		iWebLastError; // 0=Tutto ok
	CHAR	*	pszLastHeaderResponse;	// Ultimo Header
	CHAR	*	pszLastDataResponse;	// Ultimo LastDataResponse

	// Uso interno
//	CHAR	*	pszBuffer;			// Buffer di costruzione XML
	EH_AR		arXML; // new 2010
	CHAR		szOperationStyle[30];	// rpc/document
	CHAR		szInputBodyUse[30];		// Tipo di uso del body
	EN_BODYUSE	enInputBodyUseOriginal;	// .. LITERAL > ... ENCODE

	SINT		iInputHeaderUse;		// 0=Non ho Header, .. LITERAL > ... ENCODE
	CHAR		szInputHeaderUse[30];	// Tipo di uso del Header

	BOOL		bNSRenaming;		// T/F se è attivo il renaming degli NS
	// BOOL	bResponseDebug;		// T/F se si vuole di debug del response decoding

	CHAR	*	pszLogFolder;		// Se indicata verranno creati una serie di log con tutte le comunicazioni
	DWORD		dwComProgress;		// Progressivo di comunicazioni (incrementato ad ogni comunicazione sia in request che response)
	BOOL		bUtf8Mode;	// T/F se si è in UTF8Mode

	// new 2010 ------------------------------------------------------------------------------------------------
	BYTE	*	pszPreHeader;	// Usato per autotentificazioni (dati da inserire nell'header di richiesta)
	BYTE	*	pszSoapHeader;	// Soluzione "temporanea" per inviare dati un Header Soap

	SINT		iDeep;			// new 2010
	FILE	*	chOutput;
	DWORD		dwDisplayParam;		// Usato con WS_DISPLAY/WS_PRINT

	EHS_XMLSOAP_REMOVE sXmlRemove;	// Settaggi sulla generazione del'XML in richiesta
	SINT		iTimeOutSec;		// Secondi di attesia prima del Timeout 0 = No timeout (default)
	CHAR	*	pszSoapEnvName;
	CHAR		szNsName[30];

	BOOL		bResultVarError;	// T/F il client non è riuscito a costruire la variabile per prendere il result (casi particolari dinamici Es digibusiness)

} EH_SOAP;

typedef struct {
	CHAR *pUriAddress;		// Indirizzo da chiamare
	CHAR *pSoapAction;		// Valore da inserire nell'header di SOAPAction
	CHAR *pSoapEnvelope;	// Busta soap con la richiesta
	CHAR *pDescription;		// Descrizione dell'operazione (serve solo per informazione)
	CHAR *pXmlEncode;		// Default utf-8

	CHAR *pszMessageHeaderInput;  // Nome del messaggio di Header input
	CHAR *pszMessageHeaderOutput; // Nome del messaggio di Header Output

	CHAR *pszMessageInput;	// Nomde del messaggio di input
	CHAR *pszMessageOutput;	// Nome del messaggio di output
} EH_SOAP_REQUEST;

/*
typedef struct {
//	SoapVar iType;
	CHAR *	pType;	// Tipo standard (string,boolean, ecc...)
	CHAR *	pszName;
	CHAR *	pszValue;
	BOOL	bArray;		// L'elemento è un array
	EH_SOAP_COMPLEX *psComplex; // Se carico, la variabile è un dato complesso strutturato
} EH_SOAP_VAR;
*/

typedef struct {
	CHAR	*pszName;
	CHAR	*pszLocation;
	SINT	idx; // Indice in XML Tag che contiene xmlxs
	DWORD	dwCount; // Quante volte è usata
} EHS_NS;

enum soap_process
{
	SOAP_ARRAY_OPERATION=0x100,
	SOAP_REQUEST_PREPARE,	// Prepara l'xml per la richiesta
	SOAP_REQUEST_SEND,		// Spedisce l'xml
	SOAP_REQUEST			// Prepara l'xml e spedisce per la richiesta

//	SOAP_VAR_ADD,			// Aggiunge un variabile all'ambiente
//	SOAP_VAR_ASSIGN,		// Assegna un valore ad una variabile di ambiente
};


void *SoapClient(EH_SOAP *sSoap,SINT cmd,SINT info,void *ptr);
void *SoapRequest(EH_SOAP *psSoap,CHAR *pStr,...);
BOOL soap_WSDL_GetAndSave(CHAR *pUriAddress,CHAR *pLocalFileSave);

//EH_SOAP_VAR *soap_var_get(EH_SOAP *psSoap,CHAR *pName);
//EH_SOAP_VAR *soap_var_free(EH_SOAP_VAR *psSoapVar);

typedef enum {
	SEARCH_NS_NAME,
	SEARCH_NS_URI
} EN_NS_SEARCH;

SINT soap_ns_add(EH_SOAP *psSoap,CHAR *pName,CHAR *pLocation,SINT idx);
EHS_NS *soap_ns_getalloc(EH_SOAP *psSoap,CHAR *pName,EN_NS_SEARCH iWhere);
EHS_NS *soap_ns_free(EHS_NS *psSoapNS,BOOL bFree);

CHAR *soap_get_param(EH_SOAP *psSoap,CHAR *pDefault);

//void soap_cookie_free(EH_SOAP_COOKIE *psSoapCookie);
//void soap_cookie_add(EH_SOAP *psSoap,CHAR *pSetCookie);
//void soap_cookie_clean(EH_SOAP *psSoap);

void soap_ns_clean(EH_SOAP *psSoap);
void soap_ns_reset(EH_SOAP *psSoap);
void *SoapVariable(EH_SOAP *psSoap,SINT cmd,void *ptr);
void *SoapVarArg(EH_SOAP *psSoap,CHAR *Mess,...);

typedef struct {
	SINT	id;
	CHAR *	pName; // Es. string
	BOOL	bNumber; // T/F se è un numvero
	CHAR *  pszDerivated; // nome da cui è derivato NULL=PRIMITIVO
	BOOL	bLocked; // Bloccato perchè non ancora implementato
//	void * (*funcEncode)(EH_SOAP *psSoap,XEDType cmd,CHAR *pTagName,CHAR *pTagType,CHAR *pTagValue,CHAR *pNS);
	void * (*funcEncode)(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pszValue);
} EHS_SIMPLEDATATYPE;


//#define VARIABLE_TO_XML_LITERAL 1000
// #define VARIABLE_TO_XML_ENCODED 1001

#define SOAP_FLOAT_FORMAT "%.8g"
#define DBL_DIG         15                      /* # of decimal digits of precision */
#define DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */
#define DBL_MANT_DIG    53                      /* # of bits in mantissa */
#define DBL_MAX         1.7976931348623158e+308 /* max value */
#define DBL_MAX_10_EXP  308                     /* max decimal exponent */
#define DBL_MAX_EXP     1024                    /* max binary exponent */
#define DBL_MIN         2.2250738585072014e-308 /* min positive value */
#define DBL_MIN_10_EXP  (-307)                  /* min decimal exponent */
#define DBL_MIN_EXP     (-1021)                 /* min binary exponent */
#define _DBL_RADIX      2                       /* exponent radix */
#define _DBL_ROUNDS     1                       /* addition rounding: near */

#define FLT_DIG         6                       /* # of decimal digits of precision */
#define FLT_EPSILON     1.192092896e-07F        /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define FLT_GUARD       0
#define FLT_MANT_DIG    24                      /* # of bits in mantissa */
#define FLT_MAX         3.402823466e+38F        /* max value */
#define FLT_MAX_10_EXP  38                      /* max decimal exponent */
#define FLT_MAX_EXP     128                     /* max binary exponent */
#define FLT_MIN         1.175494351e-38F        /* min positive value */
#define FLT_MIN_10_EXP  (-37)                   /* min decimal exponent */
#define FLT_MIN_EXP     (-125)                  /* min binary exponent */
#define FLT_NORMALIZE   0
#define FLT_RADIX       2                       /* exponent radix */
#define FLT_ROUNDS      1                       /* addition rounding: near */

#define LDBL_DIG        DBL_DIG                 /* # of decimal digits of precision */
#define LDBL_EPSILON    DBL_EPSILON             /* smallest such that 1.0+LDBL_EPSILON != 1.0 */
#define LDBL_MANT_DIG   DBL_MANT_DIG            /* # of bits in mantissa */
#define LDBL_MAX        DBL_MAX                 /* max value */
#define LDBL_MAX_10_EXP DBL_MAX_10_EXP          /* max decimal exponent */
#define LDBL_MAX_EXP    DBL_MAX_EXP             /* max binary exponent */
#define LDBL_MIN        DBL_MIN                 /* min positive value */
#define LDBL_MIN_10_EXP DBL_MIN_10_EXP          /* min decimal exponent */
#define LDBL_MIN_EXP    DBL_MIN_EXP             /* min binary exponent */
#define _LDBL_RADIX     DBL_RADIX               /* exponent radix */
#define _LDBL_ROUNDS    DBL_ROUNDS              /* addition rounding: near */
