//
// ehPdf
//

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

typedef struct {
	CHAR * pszSource;
	_DMI dmiPart;
	S_PDFPART * arsPart;
} S_PDF;

#define PS_PDF S_PDF *

S_PDF * pdfLoad(UTF8 * pszFileName);
BOOL pdfSave(S_PDF * psPdf,UTF8 * pszFileName);
BOOL pdfFree(S_PDF * psPdf);
BOOL pdfStrReplace(S_PDF * psPdf,UTF8 * pszStrSearch,UTF8 * pszStrReplace);
