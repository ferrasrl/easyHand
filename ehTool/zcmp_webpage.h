//   ---------------------------------------------
//   | ZCMP_WebPage.h                                    
//   | ZoneComponent WebPage
//   |                                              
//   | Gestisce un Browser
//   | in una oggetto ZONAP                        
//   |                                              
//   |					by Ferrà srl 1993-2005
//   ---------------------------------------------

#include <exdisp.h>		// Defines of stuff like IWebBrowser2. This is an include file with Visual C 6 and above
#include <exdispid.h>	// per gli eventi del WebBrowser
#include <mshtml.h>		// Defines of stuff like IHTMLDocument2. This is an include file with Visual C 6 and above
#include <mshtmhst.h>	// Defines of stuff like IDocHostUIHandler. This is an include file with Visual C 6 and above
#include <mshtmdid.h>	// Defines of stuff like DISPID_HTMLELEMENTEVENTS2_XXXX

#include <crtdbg.h>		// for _ASSERT()

void * ehzWebPage(EH_OBJPARAMS);
#define EhWebPage ehzWebPage

typedef struct {

	IWebBrowser2 *				piWebBrowser2;
	LPDISPATCH 					lpDispatch;
	IHTMLDocument2 *			piHtmlDoc2;
	IHTMLDocument3 *			piHtmlDoc3;

	IHTMLElementCollection *	piElemTag;
	LONG						iTags; // Numero dei tags

} _IEI; // (Internet Explorere Interfaces)

typedef struct {
	struct OBJ *lpObj;
	HWND hWnd;
	LRESULT (*subWebNotify)(EH_NOTIFYPARAMS);

	SINT	iStatusLoading; // 0=Non definito, 1=Incaricamento, 2=Completato
	SINT	fPageReady; // T/F se l'oggetto ha una pagina in memoria
	
	void *			pBrowserResource; // Memoria allocate per il componente 
	EN_STRENC		enEncoding; // Default SE_ANSI;
	IOleObject *	browserObject;
	BOOL			bHtmlReady;
	_IEI			sIe;
	

} EH_WEBPAGE;//EH_WPLIST;

BOOL	WPCallJScript(CHAR *lpObjName,CHAR *lpFunction,CHAR *lpResult,SINT iBufferSize,SINT iArgs,...);
BOOL	WPObjectReady(CHAR *lpObjName,CHAR *lpFunction,SINT iTimeOutSec); // new 2009
BOOL	WPSetVar(CHAR *pObjName,CHAR *pFormInput,CHAR *pArgs,...);
CHAR *	WPGetVar(CHAR *pObjName,CHAR *pFormInput);
CHAR *	WPEventFunction(void);


// Opzioni possibili su update
#define WP_URL 0
#define WP_PTR 1
#define WP_WAITING 2
#define WP_MT 4
