//
// ehPdf
//

#include "/easyhand/inc/powerDoc.h"

typedef enum {
	PDFT_UNKNOW,
	PDFT_NO_OBJ,
	PDFT_OBJ_UNKNOW,
	PDFT_TEXT,
	PDFT_DEFLATED
} EN_PDFTYPE;

typedef struct {

	EN_PDFTYPE enType;
	BYTE *	pbPart;
	CHAR *	pszDecode;
	SIZE_T	tSize;

} S_PDFPART;


typedef enum {

	POT_UNKNOW,
	POT_BOOL,
	POT_NUMERIC,
	POT_STRING,
	POT_HEX,
	POT_NAME,	// Name Object
	POT_ARRAY,
	POT_DICT,	// Dictionary Object
	POT_STREAM,
	POT_OBJ		// Oggetto indiretto (xRef)

} EN_POT; // Pdf Object Type

struct pdfObj {

	void *			psPdf;		// Puntatore alla struttura pdf principale
	CHAR *			pszName;
	
	EN_POT			enType;		// Tipo di valore
	BYTE *			pbValue;	// Valore estratto (da liberare alla fine)
	INT				iSize;
	BYTE *			pbSourceStart;	// Inizio del valore nel sorgente	

	INT				idRef;		// Valore di riferimento xRef
	void *			psXref;	
//	BOOL	bExplode;

	struct pdfObj * psNext;		// Prossimo stesso ramo
	struct pdfObj * psChild;	// Figlio

	INT				idx;		// Indice dell'array
	INT				iLength;
	struct pdfObj * psArray;	// Array


};
typedef struct pdfObj S_PDF_OBJ;

typedef struct {

	BYTE *		pbSource;
	BYTE *		pbEndFile;

	_DMI		dmiPart;
	S_PDFPART * arsPart;
	
	EH_LST		lstRef;	// xRef
	BOOL		xRefReady;
	S_PDF_OBJ *	psTrailer;	// Puntatore al primo oggetto "trailer"

	INT			iPages;		// Numero di pagine del pdf
	INT			iPageFocus;
	S_PDF_OBJ *	psRoot;		// Puntatore alla Root del documento
	
	EH_LST		lstPageGo;			// Contiene gli elementi S_PDF_GO
	EH_LST		lstContentsObj;		// Contiene gli elementi S_PDF_GO

} S_PDF;

#define PS_PDF S_PDF *

typedef struct {
	
	INT		id;
	DWORD	iOffset;
	INT		iGeneration;
	CHAR	cType;
	BYTE *	pb;

	BYTE *	pbStart;
	BYTE *	pbEnd;
	DWORD	dwSize;

	S_PDF_OBJ * psObj;

} S_PDF_XREF;

typedef struct {
	
	CHAR		szOriginal[10];
	CHAR *		pszOperation;
	EH_LST		lstParams;	 // S_PDF_OBJ

} S_PDF_GO; // Graphic Operator

typedef struct {
	
	EH_LST		lstGo;
	RECTD		rcdRect;	// Dimensioni della pagina

	S_PDF_OBJ * psPage;		// Oggetto del contenuto
	S_PDF_OBJ * psFonts;	// Font (i figli sono i fonts usati nel documento)

	
} S_PDF_PAGE;

S_PDF *		pdfLoad(UTF8 * pszFileName);
S_PDF *		pdfLoadEx(UTF8 * pszFileName);
BOOL		pdfSave(S_PDF * psPdf,UTF8 * pszFileName);
BOOL		pdfFree(S_PDF * psPdf);
BOOL		pdfStrReplace(S_PDF * psPdf,UTF8 * pszStrSearch,UTF8 * pszStrReplace);
BOOL		pdfGetPage(S_PDF * psPdf,INT iPage,S_PDF_PAGE * psPageInfo);

S_PDF_OBJ *	pdfObjGet(S_PDF_OBJ * psObj,CHAR * pszKey);
CHAR *		pdfObjGetValue(S_PDF_OBJ * psObj,CHAR * pszKey,CHAR * pszDef);
RECTD *		pdfObjGetRect(S_PDF_OBJ * psObj,CHAR * pszKey,RECTD * prdRect);
void		pdfObjShow(S_PDF_OBJ * psObj,INT iIndent,BOOL bRamo,BOOL bShowArray);
/*
POINTD *	objGetPoint(S_PDF_OBJ * psObj,CHAR * pszKey,POINTD * prdPoint);
EH_COLORD * objGetRgbColor(S_PDF_OBJ * psObj,CHAR * pszKey,EH_COLORD * prdColor);
*/

//
//  pwdPdfFile() - Importa un pdf in un powerDoc
//

void		pwdPdfFile(	UTF8 *		pszFileName,	
						INT			iPage,
						BOOL		bImport,		// T/F se deve importare nel documento: false=Solo calcolo per posizionamento

						PWD_POINT *	ppumPos,		// Posiziono il file
						PWD_SIZE  *	psumSize,		// Indico dimensioni orizzontali
						PWD_ALIGN	enAlign,		// Posizionamento
						
						BOOL		bBestInFit,		// T/F se deve calcolare la maggiore dimensione possibile
						BOOL		bOnlyCalc,		// T/F Solo calcolo del posizionamento, usato per predeterminare l'occupazione e la dimenzione
						PWD_RECT  *	precPage);		// Ritorna l'area occupata (se richiesto)