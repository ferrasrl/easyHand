//   ---------------------------------------------
//   | xmlDoc
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

#ifndef EH_XMLDOC
#define EH_XMLDOC
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
enum open {XMLOPEN_FILE=0,XMLOPEN_PTR};

#define STR_CDATA_START "<![CDATA["
#define STR_CDATA_END "]]>"
//#define CDATA_START "<![CDATA[" 
//#define CDATA_STOP "]]>" 
struct xmlElement {


	CHAR *	pName;
	CHAR *	pNameOriginal; // Nome dell'elemento originale (compreso di name space)
	CHAR *	pNamespace;

	struct	xmlElement * pxParent;
	struct	xmlElement * pxFirstChild;
	struct	xmlElement * pxNext;

	INT		iLevel; // Livello di profondità nell'albero dell'elemento
	EH_LST	lstAttrib; // nome\1Valore

	BOOL	bCdata;		// Encoding CDATA
	CHAR *	pszValue;	// Value Es. <tag>[valore]</tag> (solo se semplice)

	struct	xmlDocument * psDoc;	// Serve per risalire dall'elemento al document (se necessario)

	// Mha ? non so se tenerli
	BYTE *	pszSourceStart; // Inizio nel sorgente originale (ex lpSourcePointer)
	BYTE *	pszSourceEnd; // Fine nel sorgente originale
	BYTE *	pszValueStart; // Inizio del value nel sorgente originale (ex lpSourcePointer)
	BYTE *	pszValueEnd; // Vine del value sorgente originale
	INT		iSourceLine;	// Line adel sorgente


};

typedef struct xmlElement XMLE;

typedef XMLE * XMLPTR;
typedef XMLE * PXML;
typedef XMLE ** XMLARRAY;

struct xmlDocument {

	CHAR	szFile[500]; // Nome dell'elemento
	CHAR *	pszFile;
	BOOL	bUseNamespace; // T/F se devo usare i namespace

	PXML	pxRoot;		

//	PXML	arxElement;
	INT	*	arId;	// Indice degli ID
	
	INT		iCurrentLine;

};
typedef struct xmlDocument XMLD;

typedef struct {
	CHAR *	pszName;
	CHAR *	pszValue;
} XML_ATTRIB;

XMLD *	xmlOpen(CHAR * pszData,INT iMode,BOOL bUseNameSpace);
BOOL	xmlClose(XMLD *,BOOL bSave);
PXML	xmlGet(XMLD * pxDoc, CHAR * pszFormat,...);
DWORD	xmlLength(XMLD * pxDoc, CHAR * pszFormat);
PXML	xmlAdd(XMLD * pxDoc, BOOL bAddEver, PXML pxParent, CHAR * pszNew,...);
BOOL	xmlRemove(XMLD * pxDoc, PXML pxElement,BOOL bOnlyChildren);
void	xmlPrint(XMLD * pxDoc,PXML pxElement);
EH_LST	xmlToLst(XMLPTR pxStart,EH_LST lst);

CHAR *	xmlGetAttrib(PXML px,CHAR *lpName);
BOOL	xmlSetAttrib(PXML pxml,CHAR * pszName,CHAR *pszValue,...);

XMLARRAY xmlArrayAlloc(XMLD * pxDoc,PXML pxEle,BOOL bChildren);
XMLARRAY xmlArrayFree(XMLARRAY ar);

/*
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
*/

//CHAR *	xmlGetValue(XMLDOC *lpXml,SINT idx,CHAR *pNodeElement);
CHAR *	xmlGetAttribStr(PXML px);

#ifndef EH_MEMO_DEBUG
	CHAR *	xmlGetAttribAlloc(PXML px,CHAR *lpName,CHAR *pDefault);
#else
	CHAR *	_xmlGetAttribAllocEx(PXML px,CHAR *lpName,CHAR *pDefault,CHAR *pszProgram,SINT iLine);
	#define xmlGetAttribAlloc(a,b,c) _xmlGetAttribAllocEx(a,b,c,__FILE__,__LINE__)
#endif

void *	xmlElementFree(PXML px);
PXML	xmlElementWithAttribute(XMLARRAY arXml,CHAR *pAttributeName,CHAR *pAttributeValue,BOOL bElementAlloc);
CHAR *	xmlNameSpaceLocation(XMLD * psDoc,CHAR *pNamespace,SINT idx);

void xmlTagFileWrite(FILE *pf1,BYTE *pTag,BYTE *pValue,BYTE *pLineStart,BYTE *pLineEnd);


typedef struct {
	CHAR szNameSpace[200];
	CHAR szName[200];
} XMLNAMESPACE;

XMLNAMESPACE *	xmlNSExtract(CHAR *pszInputName,XMLNAMESPACE *psNamespace);
PXML			xmlElementClone(PXML pxElement); 
CHAR *			xmlElementToString(XMLD * psDox,PXML pxElement,BOOL bOnlyValue);
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

CHAR * xmlQGet(CHAR * pszXml,CHAR * pszElement, ...);