//   ---------------------------------------------
//   | ehzBrowser.h                                    
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

void * ehzBrowser(EH_OBJPARAMS);

typedef struct {

	IWebBrowser2 *				piWebBrowser2;
	LPDISPATCH 					lpDispatch;
	IHTMLDocument2 *			piHtmlDoc2;
	IHTMLDocument3 *			piHtmlDoc3;
	IHTMLWindow2   *			piHtmlWin2;

	IHTMLElementCollection *	piElemTag;
	LONG						iTags; // Numero dei tags

} _IEI; // (Internet Explorere Interfaces)

typedef struct {
	
	void	*			psWebPage;

	DISPID				dispIdMember;
	DISPPARAMS FAR*		pDispParams;

} S_BWS_NOTIFY;


typedef struct {
		
	struct OBJ *lpObj;
	HWND hWnd;
	LRESULT (*subWebNotify)(EH_NOTIFYPARAMS);	// Usato con le finiestre
	LRESULT (*funcNotify)(S_BWS_NOTIFY * psBws); // Funziona di notifica interna

	INT	iStatusLoading; // 0=Non definito, 1=Incaricamento, 2=Completato
	INT	fPageReady; // T/F se l'oggetto ha una pagina in memoria
	
	void *			pBrowserResource; // Memoria allocate per il componente 
	EN_STRENC		enEncoding; // Default SE_ANSI;
	IOleObject *	browserObject;
	BOOL			bHtmlReady;
	_IEI			sIe;

	DWORD			msTimeReady;	// Tempo in cui è stata letta la pagina
	SIZE			sizWin;

	DWORD			msTimeoutField;

} EH_WEBPAGE;//EH_WPLIST;

//BOOL	bwsCallJScript(CHAR *lpObjName,CHAR *lpFunction,CHAR *lpResult,INT iBufferSize,INT iArgs,...);
//BOOL	bwsObjectReady(CHAR *lpObjName,CHAR *lpFunction,INT iTimeOutSec); // new 2009
BOOL	bwsSetTimeout(CHAR * pszObj,DWORD dwTimeoutField);
BOOL	bwsSetUrl(CHAR * pszObj,CHAR * pszUrl,BOOL bSilent,DWORD dwTimeout);
BOOL	bwsSetText(CHAR * pszObj,BYTE * pszText,BOOL bUtf8,DWORD dwTimeout);
BOOL	bwsVisible(CHAR * pszObj,BOOL bShow);
BOOL	bwsWaitUrl(CHAR * pszObj,CHAR * pszWildCharUrl,CHAR * pszUrl,INT iSizeUrl,DWORD msWaiting);
BOOL	bwsElementReady(CHAR * pszObj,CHAR * pszSearch, BOOL bQuery,DWORD msReply, IHTMLElement **  ppiElement);
BOOL	bwsSetValue(CHAR * pszObj,CHAR * pszIdName,BOOL bIsUtf8,CHAR * pszFormat,...);
CHAR *	bwsGetValue(CHAR * pszObj,CHAR * pszFormat, ...);
BOOL	bwsJavascript(CHAR * pszObj,CHAR * pszFormat,...);
BOOL	bwsJavascriptEx(CHAR * pszObj,DWORD dwWait,CHAR * pszFormat,...);
BOOL	bwsSetScroll(CHAR * pszObj,INT iValue);
//CHAR *	bwsEventFunction(void);

void	bwsEvent(CHAR * pszObj,IHTMLElement * piElement,CHAR * pszEvent);
BOOL	bwsEventStr(CHAR * pszObj,CHAR * pszStr,BOOL bQuery,CHAR * pszEvent,DWORD msWaitBefore,DWORD msReply);
BOOL	bwsSilent(CHAR * pszObj,BOOL bSilent);
EH_LST	bwsQuery(CHAR * pwsWeb,IHTMLElement * piTag,CHAR * pszFormat, ...);
EH_LST	bwsQueryDestroy(EH_LST lst);
IHTMLElement *	bwsGetParent(CHAR * pwsWeb,IHTMLElement * piTag);
BOOL	bwsSetInputFile(CHAR * pszObj,CHAR * pszFld,CHAR * pszPhoto);
CHAR  * bwsGetCookie(CHAR * pszObj);
EH_LST  bwsGetListFields(CHAR * pszObj,CHAR * pszNameForm);
DWORD	bwsSetEmulation(DWORD dwEmulation);
BOOL	bwsSetLoading(CHAR * pszObj, DWORD dwStatus);
BOOL	bwsLoadingWaiting(CHAR * pszObj, DWORD dwStatus, DWORD dwTimeout);

// Opzioni possibili su update
#define WP_URL 0
#define WP_PTR 1
#define WP_WAITING 2
#define WP_MT 4
