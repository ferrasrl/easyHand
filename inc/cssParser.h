//   ---------------------------------------------
//   | cssParser
//   ---------------------------------------------

#ifndef EH_CSSPARSER
#define EH_CSSPARSER

typedef struct {

	CHAR *	pszName;

	UTF8 *	utfValue;
	WCHAR * pwcValue;

} S_CSS_ELE;


typedef enum {

	CSW_UNDEFINED,
	CSW_PIXEL,
	CSW_PERC

} EN_CSS_WIDTH;


typedef struct {

	EH_LST lstEle;

} EH_CSS;

EH_CSS *	cssCreate(UTF8 * pszText); 
EH_CSS *	cssDestroy(EH_CSS * psCss);
UTF8 *		cssGet(EH_CSS * psCss, CHAR * pszName);
BOOL		cssFont(CHAR * pszFont,EH_FONT ** ppsFont);	
EN_DPL		cssAlign(CHAR * pszAlign);	
BOOL		cssBool(CHAR * pszAlign);
BOOL		cssVisibility(CHAR * pszAlign);
BOOL		cssAssign(	EH_CSS * pCss,
						CHAR * pszName,
						CHAR * pszType,
						void * pVoid);
BOOL		cssAssignWidth(EH_CSS * pCss, CHAR * pszName,double * pdWidth, EN_CSS_WIDTH * pEnWidth);





#endif

