//   ---------------------------------------------
//   | xmlParser  Utilità per il controllo         |
//   |				di documenti XML                 |
//   |                                           |
//   |             by Ferr… Art & Tecnology 2001 |
//   |             Giugno 2001
//   |
//   |
//   | DOM = Documento
//   |  Composto da Element (Elementi)
//   |  Ogni elemento ha degli attributi dinamici
//   |  Ogni elemento puà avere un valore dinamico
//   |  Ogni elemento può avere figli
//   |
//   ---------------------------------------------

#ifndef EH_XMLUTIL
#define EH_XMLUTIL
#endif

typedef enum {

	IS_A_END,
	IS_A_ERROR, 
	IS_A_TAG=3,
	IS_A_VALUE,
	IS_A_CDATA,
	IS_A_COMMENT,
	IS_A_ENTITY,
	IS_A_DOCTYPE

} EN_XMLTAG_IS;

#define CDATA_START "<![CDATA[" 
#define CDATA_STOP "]]>" 
enum open {XMLOPEN_FILE=0,XMLOPEN_PTR};
typedef struct {

	struct xmlDoc * psDoc;
	CHAR *	pNamespace;
	CHAR *	pName;
	CHAR *	pNameOriginal; // Nome dell'elemento originale (compreso di name space)
	SINT	iParent;
	SINT	iLevel; // Livello di profondità nell'albero dell'elemento
	LONG	lParam; // Parametro di riferimento per utente
	LONG	lFlags; // Flag libero per l'utente
	EH_AR	arAttrib; // nome\1Valore
	SINT	idx;	  // indice in DmiElement
	BYTE *	lpSourceStart; // Inizio nel sorgente originale (ex lpSourcePointer)
	BYTE *	lpSourceEnd; // Fine nel sorgente originale

	CHAR *	lpValue;	// Value Es. <tag>[valore]</tag> (solo se semplice)
	BYTE *	pszValueStart; // Inizio del value nel sorgente originale (ex lpSourcePointer)
	BYTE *	pszValueEnd; // Vine del value sorgente originale

	SINT	iSourceLine;
	BOOL	bAlloc;	// T/F se elemento è allocato e va liberato

} XMLELEMENT;

typedef XMLELEMENT * XML_PTR;
typedef XMLELEMENT ** XMLARRAY;

struct xmlDoc {

	CHAR	szFile[255]; // Nome dell'elemento
	SINT	hdlDOM;
	CHAR *	lpDOM;
	_DMI	dmiElement;
	BOOL	bUseNamespace; // T/F se devo usare i namespace

	XML_PTR arElement;
	INT	*	arId;	// Indice degli ID

};
typedef struct xmlDoc XMLDOC;


void *	xmlParser(XMLDOC *lpXml,SINT cmd,SINT info,void *ptr);
SINT	xmlIdBuilder(XMLDOC *lpXml); // 2010
XMLELEMENT * xmlIdSearch(XMLDOC *lpXml,CHAR *psIdValue); // 2010


// new 2012

#define xmlOpen(xml,mode,file) xmlParser(xml,WS_OPEN,mode,file)
#define xmlClose(xml) xmlParser(xml,WS_CLOSE,0,NULL)
#define xmlGetParent(a,b) xmlParser(a,WS_FIND,xmlGetIdx(b),"^") // Trova il padre
#define xmlGetChild(a,b,c) xmlParser(xmlDoc(a,b),WS_FIND,xmlGetIdx(b),c) // Trova il primo figlio (Ex xmlGetChild
#define xmlGetFirstChild(a,b) xmlParser(a,WS_FIND,xmlGetIdx(b),".") // Trova il primo figlio (Ex xmlGetChild
#define xmlNextChild(a,b) xmlParser(a,WS_FIND,xmlGetIdx(b),">") // Trova il prossimo figlio
#define xmlArrayAlloc(a,b,c) xmlParser(a,WS_PROCESS,xmlGetIdx(b),c) // Crea un array
XMLDOC * xmlDoc(XMLDOC *, XMLELEMENT *);

void *	xmlArrayFree(XMLARRAY ar);
INT		xmlArrayLen(XMLARRAY);
INT		xmlGetIdx(XMLELEMENT *);

CHAR *	xmlGetAttrib(XMLELEMENT *lpXml,CHAR *lpName);
CHAR *	xmlGetValue(XMLDOC *lpXml,SINT idx,CHAR *pNodeElement);
CHAR *	xmlGetAttribStr(XMLELEMENT *lpXml);

#ifndef EH_MEMO_DEBUG
	CHAR *	xmlGetAttribAlloc(XMLELEMENT *lpXml,CHAR *lpName,CHAR *pDefault);
#else
	CHAR *	_xmlGetAttribAllocEx(XMLELEMENT *lpXml,CHAR *lpName,CHAR *pDefault,CHAR *pszProgram,SINT iLine);
	#define xmlGetAttribAlloc(a,b,c) _xmlGetAttribAllocEx(a,b,c,__FILE__,__LINE__)
#endif

void *	xmlElementFree(XMLELEMENT * psXmlOriginal);
XMLELEMENT * xmlElementWithAttribute(XMLARRAY arXml,CHAR *pAttributeName,CHAR *pAttributeValue,BOOL bElementAlloc);
CHAR *	xmlNameSpaceLocation(XMLDOC *lpXml,CHAR *pNamespace,SINT idx);

void xmlTagFileWrite(FILE *pf1,BYTE *pTag,BYTE *pValue,BYTE *pLineStart,BYTE *pLineEnd);


typedef struct {
	CHAR szNameSpace[200];
	CHAR szName[200];
} XMLNAMESPACE;

XMLNAMESPACE *	xmlNSExtract(CHAR *pszInputName,XMLNAMESPACE *psNamespace);
XMLELEMENT *	xmlElementClone(XMLELEMENT * psXmlOriginal); 
CHAR *			xmlElementToString(XMLDOC* xmlDoc,XMLELEMENT *psXmlElement,BOOL bOnlyValue);
// new 2010
typedef struct {
	CHAR *pszSpace;		// Name Space
	CHAR *pszElement;	// Name Element
} S_XNS;
typedef S_XNS * XNS_PTR;

XNS_PTR xnsCreate(CHAR *pszTypeElement);
XNS_PTR xnsDestroy(XNS_PTR xns);
CHAR *	xnsString(XNS_PTR xns);

#ifdef EH_MEMO_DEBUG
	XNS_PTR _xnsDup(XNS_PTR xns,CHAR *p,SINT l);
	#define xnsDup(a) _xnsDup(a,__FILE__,__LINE__)
	XNS_PTR _xnsCreate(CHAR *pszInputName,CHAR *pFile,SINT iLine);
	#define xnsCreate(a) _xnsCreate(a,__FILE__,__LINE__)
#else
	XNS_PTR xnsDup(XNS_PTR xns);
	XNS_PTR xnsCreate(CHAR *pszInputName);
#endif

BYTE * xmlQGet(CHAR * pszXml,CHAR * pszElement, ...);