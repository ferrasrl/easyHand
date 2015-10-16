//   ---------------------------------------------------------------------------------
//   | ehzBrowser
//   | ZoneComponent per visualizzazione Html
//   |                                              
//   | Gestisce una WebPage di window attraverso una tecnologia OLE/ActiveX
//   | in una oggetto ZONAP                        
//   |                                              
//   |							by Ferrà srl 2005
//   |							by Ferrà srl 2015
//   ---------------------------------------------------------------------------------

/*
	Documentazione
	IWebBrowser2		https://msdn.microsoft.com/en-us/library/aa752127(v=vs.85).aspx
	IHTMLDocument2		https://msdn.microsoft.com/en-us/library/aa752574(v=vs.85).aspx


	[(HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE)\Software\Microsoft\Internet Explorer\Main\FeatureControl\FEATURE_BROWSER_EMULATION]
	"MyApplication.exe" = dword 8000 (Hex: 0x1F40)
	http://www.codeproject.com/Articles/793687/Configuring-the-emulation-mode-of-an-Internet-Expl

*/

#define CINTERFACE
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehzBrowser.h"
// #include "/easyhand/ehtool/main/armaker.h"
#include <time.h>

// Rilascio standard di un interfaccia
#define _$Release(pi) if (pi) {pi->lpVtbl->Release(pi); pi=NULL;}
#define NOTIMPLEMENTED _ASSERT(0); return(E_NOTIMPL)

// Includo le librerie
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "ole32.lib")

#define WP_FINDOBJ  0
#define WP_FINDHWND 1
#define WP_FINDOBJN  2

static EH_WEBPAGE *	_WebPageSearch(HWND hwnd);
static BYTE *		_HtmlBuilderAlloc(BYTE *lpSource);
static void			_oldFormSetField(EH_WEBPAGE * psWebPage,INT iMode,CHAR *lpFormName);
LRESULT CALLBACK	_browserWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//static long			_ieWritePage(EH_WEBPAGE *psWebPage, LPCTSTR string,INT iParam);
static long			_ieNavigatePage(EH_WEBPAGE *psWebPage, CHAR * pszWebPage,INT iParam);

//static CHAR *		_JavaLikeGetValue(EH_WEBPAGE *psWebPage,INT iMode,CHAR *lpFormName);
static BOOL			_JavaLikeSetValue(EH_WEBPAGE *psWebPage,BYTE *pszInputName,void * pszInputValue);//BSTR bsNewValue);

void				PrintDocument(HWND hwnd,INT iMode);
static EH_LST		_formArrayMaker(HWND hwnd,CHAR *lpFormName);
static void			_LWaiting(EH_WEBPAGE *psWebPage,INT iParam);

//	Ricerco/Alloco la collezione di interfacce
static BOOL			_ieCreate(EH_WEBPAGE *psWebPage,BOOL bGetTag);
static LONG			_ieDestroy(_IEI * psIe);

static void			_StrToBstr(EH_WEBPAGE * psWebPage, CHAR * pszSource, BSTR * bstrDest);
static CHAR *		_BstrToStr(EH_WEBPAGE * psWebPage, BSTR bstrSource);
static IDispatch *	_getElementInterface(EH_WEBPAGE * psWebPage,CHAR *pszInputName,BSTR * pbstrName);

// This is a simple C example. There are lots more things you can control about the browser 
// object, but we don't do it in this example. _Many_ of the functions we provide below for 
// the browser to call, will never actually be called by the browser in our example. Why? 
// Because we don't do certain things with the browser that would require it to call those 
// functions (even though we need to provide at least some stub for all of the functions).
//
// So, for these "dummy functions" that we don't expect the browser to call, we'll just stick 
// in some assembly code that causes a debugger breakpoint and tells the browser object that 
// we don't support the functionality. That way, if you try to do more things with the browser 
// object, and it starts calling these "dummy functions", you'll know which ones you should add 
// more meaningful code to.

DWORD dwGlobalCookieWB = 0;				// Per registrare e rilasciare un gestore di eventi per il WebBrowser
static const CHAR	* _pszBrowserClassName = "ehzBrowser";	// The class name of our Window to host the browser. It can be anything of your choosing.
// This is used by _ieWritePage(). It can be global because we never change it.
static const SAFEARRAYBOUND ArrayBound = {1, 0};


static EH_WEBPAGE * _getWebPage(CHAR * pszObj) {

	EH_OBJ *		psObj;
	EH_WEBPAGE *	psWebPage=NULL;
 	psObj=obj_GetInfo(pszObj);  if (psObj) 	psWebPage=psObj->pOther; 
	if (!psWebPage) ehExit("_getWebPage %s ?",pszObj);
	
	return psWebPage;

}

static _IEI * _getIE(CHAR * pszObj) {

	EH_WEBPAGE *	psWebPage=_getWebPage(pszObj);
	return &psWebPage->sIe;

}


// 
// ehzWebPage() 
// 
void * ehzBrowser(EH_OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr)
{
	EH_DISPEXT *	DExt=ptr;
	static BOOL bFirst=TRUE;
	static INT iProgress=0;

	BYTE *lpAlloc;
	EH_WEBPAGE *psWebPage;

	if (!objCalled) return NULL;
	psWebPage=objCalled->pOther;
	switch(cmd)	{

		//
		// Creazione dell'oggetto
		//
		case WS_CREATE:

			if (bFirst) {

				WNDCLASSEX	wc;

				if (!sys.fOleReady) {
					if (OleInitialize(NULL) != S_OK) ehExit("OLE Initialize error");
					sys.fOleReady=TRUE;
				}

				ZeroMemory(&wc, sizeof(WNDCLASSEX));
				wc.cbSize = sizeof(WNDCLASSEX);
				wc.hInstance = sys.EhWinInstance;
				wc.lpfnWndProc = _browserWindowProc;
				wc.lpszClassName = _pszBrowserClassName;
				wc.style = CS_HREDRAW|CS_VREDRAW;
				wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

				RegisterClassEx(&wc);
				bFirst=FALSE;

			}

			objCalled->pOther=ehAllocZero(sizeof(EH_WEBPAGE));
			psWebPage=objCalled->pOther;
			psWebPage->lpObj=objCalled;
			psWebPage->enEncoding=SE_ANSI;
			psWebPage->msTimeoutField=5000; // Default aspetto 5 secondi per vedere se trovo un campo
			psWebPage->hWnd=CreateWindowEx(	0, 
											_pszBrowserClassName, 
											0, 
											WS_CHILD|WS_VISIBLE,
											0, 0, 0, 0,
											WindowNow(), 
											(HMENU) 2000+iProgress, 
											sys.EhWinInstance, 
											psWebPage);
			//SetWindowLong(psWebPage->hWnd,GWL_USERDATA,(LONG) psWebPage);
			objCalled->hWnd=psWebPage->hWnd;
			_ieCreate(psWebPage,FALSE);//&objCalled->sIe, HWND hwnd, BOOL bGetTag
			iProgress++;
			break;

		case WS_DESTROY: 
			// Se ho l'input aperto devo liberare la finistra
			//ehFreePtr(&psSearch->pwInputValue);
			//ehFreePtr(&psSearch->pInputValue);
			DestroyWindow(objCalled->hWnd);
			_ieDestroy(&psWebPage->sIe);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_INF:
			ehError();
			break;
			//return &WPList[iWPIndex];
 
		case WS_OPEN: // Creazione
			objCalled->hWnd=psWebPage->hWnd;
			break;

		case WS_EXTNOTIFY:
			psWebPage->subWebNotify=ptr;
			break;

		case WS_PROCESS:
			break;

		case WS_UPDATE: // Inserisce una pagina
			psWebPage->iStatusLoading=0;
			switch (info&WP_PTR)
			{
				case WP_PTR: // Trova e sostituisce i tag predefiniti
						lpAlloc=_HtmlBuilderAlloc(ptr);
						//_ieWritePage(psWebPage, lpAlloc,info);
						bwsSetText(psWebPage->lpObj->nome,lpAlloc,false,0);
						ehFree(lpAlloc);
						break;

				default:
						//_ieWritePage(psWebPage, ptr,info);
						bwsSetText(psWebPage->lpObj->nome,ptr,false,0);
						break;
			}
			break;

		case WS_LINK: // Commuta in un link
			_ieNavigatePage(psWebPage, ptr ,info&WP_WAITING?TRUE:FALSE);
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,true);
			break;

		case WS_PRINT: 
			PrintDocument(objCalled->hWnd,info);
			break;

		// Compatibilità con il passato
		case WS_REALGET:
			if (strstr(ptr,".")) 
//					return _JavaLikeGetValue(psWebPage,info,ptr);
			alert("WS_REALGET obsoleto\nusare per leggere il campo bwsGetValue('OBJ','%s')",ptr);
					else
			alert("WS_REALGET obsoleto\nusare per leggere i campi del form bwsGetListFields('OBJ','%s')",ptr);
//					return (CHAR *) _formArrayMaker(objCalled->hWnd,ptr);
			break;
		
		case WS_REALSET:
			if (strstr(ptr,".")) {_oldFormSetField(psWebPage,info,ptr); return NULL;}
			ehError();//("Errore manca il punto");
			break;

		case WS_CHECK:
			//_dx_(0,20,"%d, iStatusLoading [%d]",iWPIndex,psWebPage.iStatusLoading);
			return &psWebPage->iStatusLoading;

		case WS_DISPLAY: 
			//InvalidateRect(objCalled->hWnd,NULL,TRUE);
			//_d_("Rinfresco");
			//BwsRefresh(objCalled->hWnd);
			break;
	}
	return NULL;
}

//
// _FormSetField()
//
static void _oldFormSetField(EH_WEBPAGE *psWebPage,INT iMode,CHAR *lpFormName) {

	CHAR *	pszInputName=NULL;
	CHAR *	pszInputValue=NULL;
	BOOL	bFound=FALSE;
	CHAR	szForm[200];
	BYTE *	p;

	p=strExtract(lpFormName,"",".",FALSE,FALSE); if (!p) ehExit("_FormSetField: [%s] indicare il campo da cercare",lpFormName);
	strcpy(szForm,p); 
	ehFree(p);
	pszInputName=strExtract(lpFormName,".","=",FALSE,FALSE); if (!pszInputName) ehExit("_oldFormSetField: [%s] indicare il campo da cercare",pszInputName);
	pszInputValue=strstr(lpFormName,"="); if (!pszInputValue) ehExit("_oldFormSetField: [%s] indicare il valore",pszInputName);
	pszInputValue++;

	if (!_ieCreate(psWebPage,TRUE)) {
		bFound=_JavaLikeSetValue(psWebPage,pszInputName,pszInputValue);
	}	
	if (iMode&&!bFound) ehExit("_FormSetField():Non trovato %s.%s",szForm,pszInputName);
	ehFree(pszInputName);

}

//
// _StrToBstr()
//
static void	_StrToBstr(EH_WEBPAGE * psWebPage, CHAR * pszSource, BSTR * bstrDest)
{
	WCHAR * pwc =NULL;
	switch (psWebPage->enEncoding) {
		
		case SE_UTF8:
			pwc=strEncode(pszSource,SE_UTF8,NULL);
			break;

		default:
		case SE_ANSI:
			pwc=strToWcs(pszSource);
			break;
	
	}
	* bstrDest = SysAllocString(pwc); 
	ehFreeNN(pwc);
}


//
// _BstrToStr()
//
static CHAR * _BstrToStr(EH_WEBPAGE * psWebPage, BSTR bstrSource)
{
	CHAR * pb =NULL;
	switch (psWebPage->enEncoding) {
		
		case SE_UTF8:
			pb=strEncodeW((WCHAR *) bstrSource,SE_UTF8,NULL);
			break;

		default:
		case SE_ANSI:
			pb=wcsToStr((WCHAR *) bstrSource);
			break;
	}
	return pb;
}


// ****************************************************************************
// * EVENTI del browser  (da personalizzare se utilizzati)

static void _BeforeNavigate2(EH_WEBPAGE *psWebPage,IDispatch *pDisp, VARIANT * url,VARIANT * Flags,VARIANT * TargetFrameName, VARIANT * PostData,VARIANT * Headers,  VARIANT_BOOL * Cancel) 
{
	CHAR *lpProtocol="easyhand:";
	CHAR *lpOut,*p;
	BOOL fReturn;
	
	lpOut=wcsToStr(url->bstrVal);
	p=strstr(lpOut,lpProtocol);
	if (p)
	{
		p+=strlen(lpProtocol);
		// Lancio se connessa la procedura di controllo esterna
		if (psWebPage->subWebNotify)
		{
			fReturn=FALSE;
			//lRes=
			(*psWebPage->subWebNotify)(psWebPage->lpObj,&fReturn, 0,psWebPage->hWnd,0,0,(LONG) p);
			//if (fReturn) return lRes;
		}

		// MESSAGGIO DA GESTIRE
		*Cancel = TRUE;
	}
	else
	{
		//MessageBox( NULL,  chrOUT, "OnBeforeNavigate2", 0); @#@#@#
		*Cancel = FALSE;
	}
	ehFree(lpOut);
}

/*
void ClientToHostWindow(long * CX,long * CY)										{ TRACE("ClientToHostWindow"); }
void CommandStateChange(long Command,VARIANT_BOOL Enable)							{ TRACE("CommandStateChange"); }
void DocumentComplete(IDispatch *pDisp, VARIANT *URL)								{ TRACE("DocumentComplete"); }
void DownloadBegin(VOID)															{ TRACE("DownloadBegin"); }
void DownloadComplete(VOID)															{ TRACE("DownloadComplete"); }
void FileDownload(VARIANT_BOOL * ActiveDocument, VARIANT_BOOL * Cancel)				{ TRACE("FileDownload"); }
void NavigateComplete2(IDispatch *pDisp, VARIANT *URL)								{ TRACE("NavigateComplete2"); }
void NavigateError(IDispatch *pDisp,VARIANT *URL,VARIANT *TargetFrameName,VARIANT *StatusCode,VARIANT_BOOL *Cancel)		{ TRACE("NavigateError"); }
void NewWindow2(IDispatch ** ppDisp,VARIANT_BOOL * Cancel)							{ TRACE("NewWindow2"); }
void OnFullScreen(VARIANT_BOOL FullScreen)											{ TRACE("OnFullScreen"); }
void OnMenuBar(VARIANT_BOOL MenuBar)												{ TRACE("OnMenuBar"); } 
void OnQuit(VOID)																	{ TRACE("OnQuit"); }
void OnStatusBar(VARIANT_BOOL StatusBar)											{ TRACE("OnStatusBar"); }
void OnTheaterMode(VARIANT_BOOL TheaterMode)										{ TRACE("OnTheaterMode"); }
void OnToolBar(VARIANT_BOOL ToolBar)												{ TRACE("OnToolBar"); }
void OnVisible(VARIANT_BOOL Visible)												{ TRACE("OnVisible"); }
void PrintTemplateInstantiation(IDispatch *pDisp)									{ TRACE("PrintTemplateInstantiation"); }
void PrintTemplateTeardown(IDispatch *pDisp)										{ TRACE("PrintTemplateTeardown"); }
void PrivacyImpactedStateChange(VARIANT_BOOL PrivacyImpacted)						{ TRACE("PrivacyImpactedStateChange"); }
void ProgressChange(long Progress, long ProgressMax)								{ TRACE("ProgressChange"); }
void PropertyChange(BSTR szProperty)												{ TRACE("PropertyChange"); }
void SetSecureLockIcon(long SecureLockIcon)											{ TRACE("SetSecureLockIcon"); }
void StatusTextChange(BSTR Text)													{ TRACE("StatusTextChange"); }
void TitleChange(BSTR Text)															{ TRACE("TitleChange"); }
void WindowClosing(VARIANT_BOOL IsChildWindow, VARIANT_BOOL *Cancel)				{ TRACE("WindowClosing"); }
void WindowSetHeight(long Height)													{ TRACE("WindowSetHeight"); }
void WindowSetLeft(long Left)														{ TRACE("WindowSetLeft"); }
void WindowSetResizable(VARIANT_BOOL Resizable)										{ TRACE("WindowSetResizable"); }
void WindowSetTop(long Top)															{ TRACE("WindowSetTop"); }
void WindowSetWidth(long Width)														{ TRACE("WindowSetWidth"); }
*/


// ****************************************************************************
// * DEFINIZIONI Per la gestione degli eventi del browser 

// Interfaccia IUnknown
HRESULT STDMETHODCALLTYPE WebEvents_QueryInterface(DWebBrowserEvents2 FAR* This, REFIID riid, void ** ppvObject);
HRESULT STDMETHODCALLTYPE WebEvents_AddRef(DWebBrowserEvents2 FAR* This);
HRESULT STDMETHODCALLTYPE WebEvents_Release(DWebBrowserEvents2 FAR* This);

// Interfaccia IDispatch
HRESULT STDMETHODCALLTYPE WebEvents_GetTypeInfoCount(DWebBrowserEvents2 FAR* This, unsigned int FAR*  pctinfo );
HRESULT STDMETHODCALLTYPE WebEvents_GetTypeInfo(DWebBrowserEvents2 FAR* This, unsigned int  iTInfo, LCID  lcid, ITypeInfo FAR* FAR* ppTInfo );
HRESULT STDMETHODCALLTYPE WebEvents_GetIDsOfNames(DWebBrowserEvents2 FAR* This, REFIID  riid, OLECHAR FAR* FAR*  rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgDispId);
HRESULT STDMETHODCALLTYPE WebEvents_Invoke(DWebBrowserEvents2 FAR* This, DISPID  dispIdMember, REFIID  riid, LCID  lcid, WORD  wFlags, DISPPARAMS FAR*  pDispParams, VARIANT FAR*  pVarResult, EXCEPINFO FAR*  pExcepInfo, unsigned int FAR*  puArgErr );

// Estensione della struct DWebBrowserEvents2 con l'aggiunte del contatore dei riferimenti
typedef struct __DWebBrowserEvents2Ex 
{
	DWebBrowserEvents2 WebEventsObj;
	LONG refCount;
    HWND hWnd;
} _DWebBrowserEvents2Ex;


// Virtual Table 
DWebBrowserEvents2Vtbl MyDWebBrowserEvents2Vtbl = 
{
	WebEvents_QueryInterface,
	WebEvents_AddRef,
	WebEvents_Release,
	WebEvents_GetTypeInfoCount,
	WebEvents_GetTypeInfo,
	WebEvents_GetIDsOfNames,
	WebEvents_Invoke
};


// ****************************************************************************
// * IMPLEMENTAZIONI Per la gestione degli eventi del WebBrowser 
// ****************************************************************************

// Interfaccia IUNKNOWN
HRESULT STDMETHODCALLTYPE WebEvents_QueryInterface(DWebBrowserEvents2 FAR* This, REFIID riid, void ** ppvObject) 
{
	*ppvObject = NULL;

	if ( IsEqualGUID(riid, &IID_IUnknown) )						*ppvObject = (void *) This;
	else if ( IsEqualGUID(riid, &IID_IDispatch) )				*ppvObject = (void *) This;
	else if ( IsEqualGUID(riid, &DIID_DWebBrowserEvents2) )		*ppvObject = (void *) This;

	if (*ppvObject)	
	{
		This->lpVtbl->AddRef(This);
		return S_OK;
	}
	else 
		return E_NOINTERFACE;
}


HRESULT STDMETHODCALLTYPE WebEvents_AddRef(DWebBrowserEvents2 FAR* This) 
{
	return InterlockedIncrement( &((_DWebBrowserEvents2Ex *) This)->refCount );
}

HRESULT STDMETHODCALLTYPE WebEvents_Release(DWebBrowserEvents2 FAR* This) 
{
	if (InterlockedDecrement( &((_DWebBrowserEvents2Ex *) This)->refCount ) == 0) 
	{
		GlobalFree(This);
		return 0;
	}
	return ( (_DWebBrowserEvents2Ex *) This)->refCount;
}


// Interfaccia IDISPATCH
HRESULT STDMETHODCALLTYPE WebEvents_GetTypeInfoCount(DWebBrowserEvents2 FAR* This, unsigned int FAR*  pctinfo ) 
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebEvents_GetTypeInfo(DWebBrowserEvents2 FAR* This, unsigned int  iTInfo, LCID  lcid, ITypeInfo FAR* FAR* ppTInfo ) 
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebEvents_GetIDsOfNames(DWebBrowserEvents2 FAR* This, REFIID  riid, OLECHAR FAR* FAR*  rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgDispId) 
{
	return S_OK;
}

static BOOL _notify(EH_WEBPAGE * psWebPage,EN_MESSAGE enMess,CHAR *pszEvent) {

	BOOL bReturn=FALSE;
	if (psWebPage->subWebNotify)
	{
		psWebPage->subWebNotify(psWebPage->lpObj, &bReturn,  enMess, psWebPage->hWnd,0,0,(LONG) pszEvent);
	}
	return bReturn;
}


HRESULT STDMETHODCALLTYPE WebEvents_Invoke(DWebBrowserEvents2 FAR* This, 
										   DISPID  dispIdMember, 
										   REFIID  riid, 
										   LCID  lcid, 
										   WORD  wFlags, 
										   DISPPARAMS FAR*  pDispParams, 
										   VARIANT FAR*  pVarResult, 
										   EXCEPINFO FAR*  pExcepInfo, 
										   unsigned int FAR*  puArgErr ) 
{
  // This function is called to recevie an event. The event is identified by the
  // dispIdMember argument. It is our responsibility to retrieve the event arguments
  // from the pDispParams->rgvarg array and call the event function.
  // If we do not handle an event we can return DISP_E_MEMBERNOTFOUND.
  // The variant member that we use for each argument is determined by the argument
  // type of the event function. eg. If an event has the argument long x we would
  // use the lVal member of the VARIANT struct.
//	INT iWPIndex;
	EH_WEBPAGE *psWebPage;
  _DWebBrowserEvents2Ex *lpD=(_DWebBrowserEvents2Ex *) This;
  CHAR szServ[1024],*psz;

  psWebPage=_WebPageSearch(lpD->hWnd);

  if (psWebPage->funcNotify) {
	S_BWS_NOTIFY sBws;
	sBws.psWebPage=psWebPage;
	sBws.dispIdMember=dispIdMember;
	sBws.pDispParams=pDispParams;
	return  psWebPage->funcNotify(&sBws);
  }
  //return S_OK;

  printf("invoke: %d " ,dispIdMember);
  // Here is our message map, where we map dispids to function calls.
  switch (dispIdMember) 
  {
	case DISPID_BEFORENAVIGATE2:
		
		// (parameters are on stack, thus in reverse order)
		_BeforeNavigate2(	psWebPage,
							pDispParams->rgvarg[6].pdispVal,    // pDisp
							pDispParams->rgvarg[5].pvarVal,     // url
							pDispParams->rgvarg[4].pvarVal,     // Flags
							pDispParams->rgvarg[3].pvarVal,     // TargetFrameName
							pDispParams->rgvarg[2].pvarVal,     // PostData
							pDispParams->rgvarg[1].pvarVal,     // Headers
							pDispParams->rgvarg[0].pboolVal);   // Cancel
		break;

	case DISPID_DOWNLOADBEGIN:
		//iWPIndex=WPFind(WP_FINDHWND,lpD->hWnd);
		//WPList[iWPIndex].iStatusLoading=1; // Completato
//		psWebPage=_WebPageSearch(lpD->hWnd);
		psWebPage->iStatusLoading=1;
		break;

	case DISPID_DOWNLOADCOMPLETE:
		printf("Download complete");
		break;

	case DISPID_DOCUMENTCOMPLETE: // <--- Fires when a document is completely loaded and initialized.
		//iWPIndex=WPFind(WP_FINDHWND,lpD->hWnd);
		
		psWebPage->iStatusLoading|=2; // Completato
		psWebPage->fPageReady=true;
		if (pDispParams->rgvarg) {
			if (pDispParams->rgvarg[0].pvarVal) {
				if (pDispParams->rgvarg[0].pvarVal->bstrVal) {
					psz=_BstrToStr(psWebPage, pDispParams->rgvarg[0].pvarVal->bstrVal);
					sprintf(szServ,"docCompleted|%s",psz);
					ehFree(psz);
					_notify(psWebPage,WS_PROCESS,szServ);
				}
			}
		}
		break; 

	case DISPID_COMMANDSTATECHANGE:
		{
			long cmd = pDispParams-> rgvarg [0].lVal; 
			printf("Command State Change %d" CRLF,cmd);
		}
		return DISP_E_MEMBERNOTFOUND;
/*
	case DISPID_COMMANDSTATECHANGE:
         { 
            long cmd = pDispParams-> rgvarg [0].lVal; 
//            TOLEBOOL enabled = pDispParams-> rgvarg [1].boolVal; 
            //event happens 
            switch (cmd) 
            { 
            case CSC_NAVIGATEBACK: 
               ;//here anything 
            break; 
            case CSC_NAVIGATEFORWARD: 
               ;//here anything 
            break; 
            case CSC_UPDATECOMMANDS: 
				printf("qui");
               ;//here anything 
            break; 
            }
         }
	//	psWebPage->fPageReady=true;
		break;
*/

		// Fires after a navigation to a link is completed on a window element or a frameSet element.
		// Inviato quando la navigazione è completa
	case DISPID_NAVIGATECOMPLETE2:
		psWebPage->iStatusLoading|=4; // Completato
//		psWebPage->fPageReady=true;
		psz=_BstrToStr(psWebPage, pDispParams->rgvarg[0].pvarVal->bstrVal);
		sprintf(szServ,"navCompleted|%s",psz);
		ehFree(psz);
		_notify(psWebPage,WS_PROCESS,szServ);
		break;

	case DISPID_PRINTTEMPLATEINSTANTIATION:
		_notify(psWebPage,WS_PROCESS,"printInit");
		break;

	case DISPID_PRINTTEMPLATETEARDOWN:
		_notify(psWebPage,WS_PROCESS,"printDown");
		break;

/*
	case DISPID_DOWNLOADCOMPLETE:
//		MessageBox( 0, "DISPID_DOWNLOADCOMPLETE", "Evento WB", 0);
//		psWebPage=_WebPageSearch(lpD->hWnd);
		//iWPIndex=WPFind(WP_FINDHWND,lpD->hWnd);
		psWebPage->iStatusLoading|=8; // Completato
		_notify(psWebPage,WS_PROCESS,"DownloadCompleted");
		break;
*/

/*	case DISPID_CLIENTTOHOSTWINDOW:
		ClientToHostWindow(pDispParams->rgvarg[1].plVal, pDispParams->rgvarg[0].plVal);
		break;
	case DISPID_COMMANDSTATECHANGE:
		MessageBox( 0, "DISPID_COMMANDSTATECHANGE", "Evento WB", 0);
		break;
		CommandStateChange(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_DOCUMENTCOMPLETE:
		DocumentComplete(pDispParams->rgvarg[1].pdispVal, pDispParams->rgvarg[0].pvarVal);
		break;
	case DISPID_FILEDOWNLOAD:
		FileDownload(pDispParams->rgvarg[1].pboolVal, pDispParams->rgvarg[0].pboolVal);
		break;
	case DISPID_NAVIGATECOMPLETE2:
		MessageBox( 0, "DISPID_NAVIGATECOMPLETE2", "Evento WB", 0);
		break;
		NavigateComplete2(pDispParams->rgvarg[1].pdispVal, pDispParams->rgvarg[0].pvarVal);
		break;
	case DISPID_NAVIGATEERROR:
		NavigateError(pDispParams->rgvarg[4].pdispVal, pDispParams->rgvarg[3].pvarVal, pDispParams->rgvarg[2].pvarVal, pDispParams->rgvarg[1].pvarVal, pDispParams->rgvarg[0].pboolVal);
		break;
	case DISPID_NEWWINDOW2:
		NewWindow2(pDispParams->rgvarg[1].ppdispVal, pDispParams->rgvarg[0].pboolVal);
		break;
	case DISPID_ONFULLSCREEN:
		OnFullScreen(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_ONMENUBAR:
		OnMenuBar(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_ONQUIT:
		OnQuit();
		break;
	case DISPID_ONSTATUSBAR:
		OnStatusBar(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_ONTHEATERMODE:
		OnTheaterMode(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_ONTOOLBAR:
		OnToolBar(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_ONVISIBLE:
		OnVisible(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_PRINTTEMPLATEINSTANTIATION:
		PrintTemplateInstantiation(pDispParams->rgvarg[0].pdispVal);
		break;
	case DISPID_PRINTTEMPLATETEARDOWN:
		PrintTemplateTeardown(pDispParams->rgvarg[0].pdispVal);
		break;
	case DISPID_PRIVACYIMPACTEDSTATECHANGE:
		PrivacyImpactedStateChange(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_PROGRESSCHANGE:
		ProgressChange(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_PROPERTYCHANGE:
		PropertyChange(pDispParams->rgvarg[0].bstrVal);
		break;
	case DISPID_SETSECURELOCKICON:
		SetSecureLockIcon(pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_STATUSTEXTCHANGE:
		StatusTextChange(pDispParams->rgvarg[0].bstrVal);
		break;
	case DISPID_TITLECHANGE:
		TitleChange(pDispParams->rgvarg[0].bstrVal);
		break;
	case DISPID_WINDOWCLOSING:
		WindowClosing(pDispParams->rgvarg[1].boolVal, pDispParams->rgvarg[0].pboolVal);
		break;
	case DISPID_WINDOWSETHEIGHT:
		WindowSetHeight(pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_WINDOWSETLEFT:
		WindowSetLeft(pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_WINDOWSETRESIZABLE:
		WindowSetResizable(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_WINDOWSETTOP:
		WindowSetTop(pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_WINDOWSETWIDTH:
		WindowSetWidth(pDispParams->rgvarg[0].lVal);
		break;
*/
		/*
	case DISPID_STATUSTEXTCHANGE:
		//StatusTextChange(pDispParams->rgvarg[0].bstrVal);
		_d_("[%s]   ",pDispParams->rgvarg[0].bstrVal);
		break;
		*/

	default:
	    printf(CRLF);
		return DISP_E_MEMBERNOTFOUND;
  } //end switch

  printf(CRLF);
  return S_OK;
}

// Our IStorage functions that the browser may call
HRESULT STDMETHODCALLTYPE Storage_QueryInterface(IStorage FAR* This, REFIID riid, LPVOID FAR* ppvObj);
HRESULT STDMETHODCALLTYPE Storage_AddRef(IStorage FAR* This);
HRESULT STDMETHODCALLTYPE Storage_Release(IStorage FAR* This);
HRESULT STDMETHODCALLTYPE Storage_CreateStream(IStorage FAR* This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm);
HRESULT STDMETHODCALLTYPE Storage_OpenStream(IStorage FAR* This, const WCHAR * pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);
HRESULT STDMETHODCALLTYPE Storage_CreateStorage(IStorage FAR* This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg);
HRESULT STDMETHODCALLTYPE Storage_OpenStorage(IStorage FAR* This, const WCHAR * pwcsName, IStorage * pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);
HRESULT STDMETHODCALLTYPE Storage_CopyTo(IStorage FAR* This, DWORD ciidExclude, IID const *rgiidExclude, SNB snbExclude,IStorage *pstgDest);
HRESULT STDMETHODCALLTYPE Storage_MoveElementTo(IStorage FAR* This, const OLECHAR *pwcsName,IStorage * pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags);
HRESULT STDMETHODCALLTYPE Storage_Commit(IStorage FAR* This, DWORD grfCommitFlags);
HRESULT STDMETHODCALLTYPE Storage_Revert(IStorage FAR* This);
HRESULT STDMETHODCALLTYPE Storage_EnumElements(IStorage FAR* This, DWORD reserved1, void * reserved2, DWORD reserved3, IEnumSTATSTG ** ppenum);
HRESULT STDMETHODCALLTYPE Storage_DestroyElement(IStorage FAR* This, const OLECHAR *pwcsName);
HRESULT STDMETHODCALLTYPE Storage_RenameElement(IStorage FAR* This, const WCHAR *pwcsOldName, const WCHAR *pwcsNewName);
HRESULT STDMETHODCALLTYPE Storage_SetElementTimes(IStorage FAR* This, const WCHAR *pwcsName, FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime);
HRESULT STDMETHODCALLTYPE Storage_SetClass(IStorage FAR* This, REFCLSID clsid);
HRESULT STDMETHODCALLTYPE Storage_SetStateBits(IStorage FAR* This, DWORD grfStateBits, DWORD grfMask);
HRESULT STDMETHODCALLTYPE Storage_Stat(IStorage FAR* This, STATSTG * pstatstg, DWORD grfStatFlag);

// Our IStorage VTable. This is the array of pointers to the above functions in our C
// program that someone may call in order to store some data to disk. We must define a
// particular set of functions that comprise the IStorage set of functions (see above),
// and then stuff pointers to those functions in their respective 'slots' in this table.
// We want the browser to use this VTable with our IStorage structure (object).
IStorageVtbl MyIStorageTable = 
{
	Storage_QueryInterface,
	Storage_AddRef,
	Storage_Release,
	Storage_CreateStream,
	Storage_OpenStream,
	Storage_CreateStorage,
	Storage_OpenStorage,
	Storage_CopyTo,
	Storage_MoveElementTo,
	Storage_Commit,
	Storage_Revert,
	Storage_EnumElements,
	Storage_DestroyElement,
	Storage_RenameElement,
	Storage_SetElementTimes,
	Storage_SetClass,
	Storage_SetStateBits,
	Storage_Stat
};

// Our IStorage structure. NOTE: All it contains is a pointer to our IStorageVtbl, so we can easily initialize it
// here instead of doing that programmably.
IStorage MyIStorage = 
{ 
	&MyIStorageTable 
};


// Our IOleInPlaceFrame functions that the browser may call
HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame FAR* This, REFIID riid, LPVOID FAR* ppvObj);
HRESULT STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame FAR* This);
HRESULT STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame FAR* This);
HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame FAR* This, HWND FAR* lphwnd);
HRESULT STDMETHODCALLTYPE Frame_ContextSensitiveHelp(IOleInPlaceFrame FAR* This, BOOL fEnterMode);
HRESULT STDMETHODCALLTYPE Frame_GetBorder(IOleInPlaceFrame FAR* This, LPRECT lprectBorder);
HRESULT STDMETHODCALLTYPE Frame_RequestBorderSpace(IOleInPlaceFrame FAR* This, LPCBORDERWIDTHS pborderwidths);
HRESULT STDMETHODCALLTYPE Frame_SetBorderSpace(IOleInPlaceFrame FAR* This, LPCBORDERWIDTHS pborderwidths);
HRESULT STDMETHODCALLTYPE Frame_SetActiveObject(IOleInPlaceFrame FAR* This, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);
HRESULT STDMETHODCALLTYPE Frame_InsertMenus(IOleInPlaceFrame FAR* This, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
HRESULT STDMETHODCALLTYPE Frame_SetMenu(IOleInPlaceFrame FAR* This, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
HRESULT STDMETHODCALLTYPE Frame_RemoveMenus(IOleInPlaceFrame FAR* This, HMENU hmenuShared);
HRESULT STDMETHODCALLTYPE Frame_SetStatusText(IOleInPlaceFrame FAR* This, LPCOLESTR pszStatusText);
HRESULT STDMETHODCALLTYPE Frame_EnableModeless(IOleInPlaceFrame FAR* This, BOOL fEnable);
HRESULT STDMETHODCALLTYPE Frame_TranslateAccelerator(IOleInPlaceFrame FAR* This, LPMSG lpmsg, WORD wID);

// Our IOleInPlaceFrame VTable. This is the array of pointers to the above functions in our C
// program that the browser may call in order to interact with our frame window that contains
// the browser object. We must define a particular set of functions that comprise the
// IOleInPlaceFrame set of functions (see above), and then stuff pointers to those functions
// in their respective 'slots' in this table. We want the browser to use this VTable with our
// IOleInPlaceFrame structure.
IOleInPlaceFrameVtbl MyIOleInPlaceFrameTable = 
{	
	Frame_QueryInterface,
	Frame_AddRef,
	Frame_Release,
	Frame_GetWindow,
	Frame_ContextSensitiveHelp,
	Frame_GetBorder,
	Frame_RequestBorderSpace,
	Frame_SetBorderSpace,
	Frame_SetActiveObject,
	Frame_InsertMenus,
	Frame_SetMenu,
	Frame_RemoveMenus,
	Frame_SetStatusText,
	Frame_EnableModeless,
	Frame_TranslateAccelerator
};

// We need to return an IOleInPlaceFrame struct to the browser object. And one of our IOleInPlaceFrame
// functions (Frame_GetWindow) is going to need to access our window handle. So let's create our own
// struct that starts off with an IOleInPlaceFrame struct (and that's important -- the IOleInPlaceFrame
// struct *must* be first), and then has an extra data field where we can store our own window's HWND.
//
// And because we may want to create multiple windows, each hosting its own browser object (to
// display its own web page), then we need to create a IOleInPlaceFrame struct for each window. So,
// we're not going to declare our IOleInPlaceFrame struct globally. We'll allocate it later using
// GlobalAlloc, and then stuff the appropriate HWND in it then, and also stuff a pointer to
// MyIOleInPlaceFrameTable in it. But let's just define it here.
typedef struct 
{
	IOleInPlaceFrame	frame;		// The IOleInPlaceFrame must be first!

	///////////////////////////////////////////////////
	// Here you add any extra variables that you need
	// to access in your IOleInPlaceFrame functions.
	// You don't want those functions to access global
	// variables, because then you couldn't use more
	// than one browser object. (ie, You couldn't have
	// multiple windows, each with its own embedded
	// browser object to display a different web page).
	//
	// So here is where I added my extra HWND that my
	// IOleInPlaceFrame function Frame_GetWindow() needs
	// to access.
	///////////////////////////////////////////////////
	HWND				window;
} _IOleInPlaceFrameEx;






// Our IOleClientSite functions that the browser may call
HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite FAR* This, REFIID riid, void ** ppvObject);
HRESULT STDMETHODCALLTYPE Site_AddRef(IOleClientSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_Release(IOleClientSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_SaveObject(IOleClientSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_GetMoniker(IOleClientSite FAR* This, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk);
HRESULT STDMETHODCALLTYPE Site_GetContainer(IOleClientSite FAR* This, LPOLECONTAINER FAR* ppContainer);
HRESULT STDMETHODCALLTYPE Site_ShowObject(IOleClientSite FAR* This);
HRESULT STDMETHODCALLTYPE Site_OnShowWindow(IOleClientSite FAR* This, BOOL fShow);
HRESULT STDMETHODCALLTYPE Site_RequestNewObjectLayout(IOleClientSite FAR* This);

// Our IOleClientSite VTable. This is the array of pointers to the above functions in our C
// program that the browser may call in order to interact with our frame window that contains
// the browser object. We must define a particular set of functions that comprise the
// IOleClientSite set of functions (see above), and then stuff pointers to those functions
// in their respective 'slots' in this table. We want the browser to use this VTable with our
// IOleClientSite structure.
IOleClientSiteVtbl MyIOleClientSiteTable = {Site_QueryInterface,
Site_AddRef,
Site_Release,
Site_SaveObject,
Site_GetMoniker,
Site_GetContainer,
Site_ShowObject,
Site_OnShowWindow,
Site_RequestNewObjectLayout};


// Our IDocHostUIHandler functions that the browser may call
HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler FAR* This, REFIID riid, void ** ppvObject);
HRESULT STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler FAR* This);
HRESULT STDMETHODCALLTYPE UI_Release(IDocHostUIHandler FAR* This);
HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler FAR* This, DWORD dwID, POINT __RPC_FAR *ppt, IUnknown __RPC_FAR *pcmdtReserved, IDispatch __RPC_FAR *pdispReserved);
HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler FAR* This, DOCHOSTUIINFO __RPC_FAR *pInfo);
HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler FAR* This, DWORD dwID, IOleInPlaceActiveObject __RPC_FAR *pActiveObject, IOleCommandTarget __RPC_FAR *pCommandTarget, IOleInPlaceFrame __RPC_FAR *pFrame, IOleInPlaceUIWindow __RPC_FAR *pDoc);
HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler FAR* This);
HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler FAR* This);
HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler FAR* This, BOOL fEnable);
HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler FAR* This, BOOL fActivate);
HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler FAR* This, BOOL fActivate);
HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler FAR* This, LPCRECT prcBorder, IOleInPlaceUIWindow __RPC_FAR *pUIWindow, BOOL fRameWindow);
HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler FAR* This, LPMSG lpMsg, const GUID __RPC_FAR *pguidCmdGroup, DWORD nCmdID);
HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler FAR* This, LPOLESTR __RPC_FAR *pchKey, DWORD dw);
HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler FAR* This, IDropTarget __RPC_FAR *pDropTarget, IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget);
HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler FAR* This, IDispatch __RPC_FAR *__RPC_FAR *ppDispatch);
HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler FAR* This, DWORD dwTranslate, OLECHAR __RPC_FAR *pchURLIn, OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut);
HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler FAR* This, IDataObject __RPC_FAR *pDO, IDataObject __RPC_FAR *__RPC_FAR *ppDORet);

// Our IDocHostUIHandler VTable. This is the array of pointers to the above functions in our C
// program that the browser may call in order to replace/set certain user interface considerations
// (such as whether to display a pop-up context menu when the user right-clicks on the embedded
// browser object). We must define a particular set of functions that comprise the
// IDocHostUIHandler set of functions (see above), and then stuff pointers to those functions
// in their respective 'slots' in this table. We want the browser to use this VTable with our
// IDocHostUIHandler structure.
IDocHostUIHandlerVtbl MyIDocHostUIHandlerTable =  
{
	UI_QueryInterface,
	UI_AddRef,
	UI_Release,
	UI_ShowContextMenu,
	UI_GetHostInfo,
	UI_ShowUI,
	UI_HideUI,
	UI_UpdateUI,
	UI_EnableModeless,
	UI_OnDocWindowActivate,
	UI_OnFrameWindowActivate,
	UI_ResizeBorder,
	UI_TranslateAccelerator,
	UI_GetOptionKeyPath,
	UI_GetDropTarget,
	UI_GetExternal,
	UI_TranslateUrl,
	UI_FilterDataObject
};

// We'll allocate our IDocHostUIHandler object dynamically with GlobalAlloc() for reasons outlined later.



// Our IOleInPlaceSite functions that the browser may call
HRESULT STDMETHODCALLTYPE InPlace_QueryInterface(IOleInPlaceSite FAR* This, REFIID riid, void ** ppvObject);
HRESULT STDMETHODCALLTYPE InPlace_AddRef(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE InPlace_Release(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE InPlace_GetWindow(IOleInPlaceSite FAR* This, HWND FAR* lphwnd);
HRESULT STDMETHODCALLTYPE InPlace_ContextSensitiveHelp(IOleInPlaceSite FAR* This, BOOL fEnterMode);
HRESULT STDMETHODCALLTYPE InPlace_CanInPlaceActivate(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceActivate(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE InPlace_OnUIActivate(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE InPlace_GetWindowContext(IOleInPlaceSite FAR* This, LPOLEINPLACEFRAME FAR* lplpFrame,LPOLEINPLACEUIWINDOW FAR* lplpDoc,LPRECT lprcPosRect,LPRECT lprcClipRect,LPOLEINPLACEFRAMEINFO lpFrameInfo);
HRESULT STDMETHODCALLTYPE InPlace_Scroll(IOleInPlaceSite FAR* This, SIZE scrollExtent);
HRESULT STDMETHODCALLTYPE InPlace_OnUIDeactivate(IOleInPlaceSite FAR* This, BOOL fUndoable);
HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceDeactivate(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE InPlace_DiscardUndoState(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE InPlace_DeactivateAndUndo(IOleInPlaceSite FAR* This);
HRESULT STDMETHODCALLTYPE InPlace_OnPosRectChange(IOleInPlaceSite FAR* This, LPCRECT lprcPosRect);

// Our IOleInPlaceSite VTable. This is the array of pointers to the above functions in our C
// program that the browser may call in order to interact with our frame window that contains
// the browser object. We must define a particular set of functions that comprise the
// IOleInPlaceSite set of functions (see above), and then stuff pointers to those functions
// in their respective 'slots' in this table. We want the browser to use this VTable with our
// IOleInPlaceSite structure.
IOleInPlaceSiteVtbl MyIOleInPlaceSiteTable =  
{
	InPlace_QueryInterface,
	InPlace_AddRef,
	InPlace_Release,
	InPlace_GetWindow,
	InPlace_ContextSensitiveHelp,
	InPlace_CanInPlaceActivate,
	InPlace_OnInPlaceActivate,
	InPlace_OnUIActivate,
	InPlace_GetWindowContext,
	InPlace_Scroll,
	InPlace_OnUIDeactivate,
	InPlace_OnInPlaceDeactivate,
	InPlace_DiscardUndoState,
	InPlace_DeactivateAndUndo,
	InPlace_OnPosRectChange,

};

// We need to pass our IOleClientSite structure to OleCreate (which in turn gives it to the browser).
// But the browser is also going to ask our IOleClientSite's QueryInterface() to return a pointer to
// our IOleInPlaceSite and/or IDocHostUIHandler structs. So we'll need to have those pointers handy.
// Plus, some of our IOleClientSite and IOleInPlaceSite functions will need to have the HWND to our
// window, and also a pointer to our IOleInPlaceFrame struct. So let's create a single struct that
// has the IOleClientSite, IOleInPlaceSite, IDocHostUIHandler, and IOleInPlaceFrame structs all inside
// it (so we can easily get a pointer to any one from any of those structs' functions). As long as the
// IOleClientSite struct is the very first thing in this custom struct, it's all ok. We can still pass
// it to OleCreate() and pretend that it's an ordinary IOleClientSite. We'll call this new struct a
// _IOleClientSiteEx.
//
// And because we may want to create multiple windows, each hosting its own browser object (to
// display its own web page), then we need to create a unique _IOleClientSiteEx struct for
// each window. So, we're not going to declare this struct globally. We'll allocate it later
// using GlobalAlloc, and then initialize the structs within it.

typedef struct 
{
	IOleInPlaceSite			inplace;	// My IOleInPlaceSite object. Must be first with in _IOleInPlaceSiteEx.

	///////////////////////////////////////////////////
	// Here you add any extra variables that you need
	// to access in your IOleInPlaceSite functions.
	//
	// So here is where I added my IOleInPlaceFrame
	// struct. If you need extra variables, add them
	// at the end.
	///////////////////////////////////////////////////
	_IOleInPlaceFrameEx		frame;		// My IOleInPlaceFrame object. Must be first within my _IOleInPlaceFrameEx
} _IOleInPlaceSiteEx;

typedef struct 
{
	IDocHostUIHandler		ui;			// My IDocHostUIHandler object. Must be first.

	///////////////////////////////////////////////////
	// Here you add any extra variables that you need
	// to access in your IDocHostUIHandler functions.
	///////////////////////////////////////////////////
} _IDocHostUIHandlerEx;

typedef struct 
{
	IOleClientSite			client;		// My IOleClientSite object. Must be first.
	_IOleInPlaceSiteEx		inplace;	// My IOleInPlaceSite object. A convenient place to put it.
	_IDocHostUIHandlerEx	ui;			// My IDocHostUIHandler object. Must be first within my _IDocHostUIHandlerEx.

	///////////////////////////////////////////////////
	// Here you add any extra variables that you need
	// to access in your IOleClientSite functions.
	///////////////////////////////////////////////////
} _IOleClientSiteEx;


// This is a simple C example. There are lots more things you can control about the browser object, but
// we don't do it in this example. _Many_ of the functions we provide below for the browser to call, will
// never actually be called by the browser in our example. Why? Because we don't do certain things
// with the browser that would require it to call those functions (even though we need to provide
// at least some stub for all of the functions).
//
// So, for these "dummy functions" that we don't expect the browser to call, we'll just stick in some
// assembly code that causes a debugger breakpoint and tells the browser object that we don't support
// the functionality. That way, if you try to do more things with the browser object, and it starts
// calling these "dummy functions", you'll know which ones you should add more meaningful code to.
#define NOTIMPLEMENTED _ASSERT(0); return(E_NOTIMPL)


//////////////////////////////////// My IDocHostUIHandler functions  //////////////////////////////////////
// The browser object asks us for the pointer to our IDocHostUIHandler object by calling our IOleClientSite's
// QueryInterface (ie, Site_QueryInterface) and specifying a REFIID of IID_IDocHostUIHandler.
//
// NOTE: You need at least IE 4.0. Previous versions do not ask for, nor utilize, our IDocHostUIHandler functions.

HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler FAR* This, REFIID riid, LPVOID FAR* ppvObj)
{
	// The browser assumes that our IDocHostUIHandler object is associated with our IOleClientSite
	// object. So it is possible that the browser may call our IDocHostUIHandler's QueryInterface()
	// to ask us to return a pointer to our IOleClientSite, in the same way that the browser calls
	// our IOleClientSite's QueryInterface() to ask for a pointer to our IDocHostUIHandler.
	//
	// Rather than duplicate much of the code in IOleClientSite's QueryInterface, let's just get
	// a pointer to our _IOleClientSiteEx object, substitute it as the 'This' arg, and call our
	// our IOleClientSite's QueryInterface. Note that since our _IDocHostUIHandlerEx is embedded right
	// inside our _IOleClientSiteEx, and comes immediately after the _IOleInPlaceSiteEx, we can employ
	// the following trickery to get the pointer to our _IOleClientSiteEx.
	return(Site_QueryInterface((IOleClientSite *)((char *)This - sizeof(IOleClientSite) - sizeof(_IOleInPlaceSiteEx)), riid, ppvObj));
}

HRESULT STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE UI_Release(IDocHostUIHandler FAR* This)
{
	return(1);
}

// Called when the browser object is about to display its context menu.
HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler FAR* This, DWORD dwID, POINT __RPC_FAR *ppt, IUnknown __RPC_FAR *pcmdtReserved, IDispatch __RPC_FAR *pdispReserved)
{
    // If desired, we can pop up your own custom context menu here. Of course,
	// we would need some way to get our window handle, so what we'd probably
	// do is like what we did with our IOleInPlaceFrame object. We'd define and create
	// our own IDocHostUIHandlerEx object with an embedded IDocHostUIHandler at the
	// start of it. Then we'd add an extra HWND field to store our window handle.
	// It could look like this:
	//
	// typedef struct _IDocHostUIHandlerEx {
	//		IDocHostUIHandler	ui;		// The IDocHostUIHandler must be first!
	//		HWND				window;
	// } IDocHostUIHandlerEx;
	//
	// Of course, we'd need a unique IDocHostUIHandlerEx object for each window, so
	// EmbedBrowserObjectBuilder() would have to allocate one of those too. And that's
	// what we'd pass to our browser object (and it would in turn pass it to us
	// here, instead of 'This' being a IDocHostUIHandler *).

    // ritorneremo S_OK per dire al browser di NON visualizzare il suot default context menu,
	// or ritornereno S_FALSE per drigli di visualizzare il suo default context menu. 
	if (!dwID) return(S_OK);
	return(S_FALSE);
}

// Called at initialization of the browser object UI. We can set various features of the browser object here.
// http://msdn.microsoft.com/en-us/library/aa753257(VS.85).aspx

//
HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler FAR* This, DOCHOSTUIINFO __RPC_FAR *pInfo)
{
	pInfo->cbSize = sizeof(DOCHOSTUIINFO);

	// Set some flags. We don't want any 3D border (DOCHOSTUIFLAG_NO3DBORDER). You can do other things like 
	// hide the scroll bar (DOCHOSTUIFLAG_SCROLL_NO), 
	// display picture display (DOCHOSTUIFLAG_NOPICS),
	// disable any script running when the page is loaded (DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE),
	// open a site in a new browser window when the user clicks on some link (DOCHOSTUIFLAG_OPENNEWWIN),
	// and lots of other things. 
	// See the MSDN docs on the DOCHOSTUIINFO struct passed to us. @#@#@#
	
	// Per evitare le selezioni del testo e barre di scorrimento DOCHOSTUIFLAG_SCROLL_NO |
	// http://msdn.microsoft.com/en-us/library/aa753277(VS.85).aspx
	//
	pInfo->dwFlags = DOCHOSTUIFLAG_DIALOG |  
					 DOCHOSTUIFLAG_NO3DBORDER;
					 // | 0x08000000; 
	//win_infoarg("Ci passo");

	// Set what happens when the user double-clicks on the object. Here we use the default.
	pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

	return(S_OK);
}

// Called when the browser object shows its UI. This allows us to replace its menus and toolbars by creating our
// own and displaying them here.
HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler FAR* This, DWORD dwID, IOleInPlaceActiveObject __RPC_FAR *pActiveObject, IOleCommandTarget __RPC_FAR *pCommandTarget, IOleInPlaceFrame __RPC_FAR *pFrame, IOleInPlaceUIWindow __RPC_FAR *pDoc)
{
	// We've already got our own UI in place so just return S_OK to tell the browser
	// not to display its menus/toolbars. Otherwise we'd return S_FALSE to let it do that.
	return(S_OK);
}

// Called when browser object hides its UI. This allows us to hide any menus/toolbars we created in ShowUI.
HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler FAR* This)
{
	return(S_OK);
}

// Called when the browser object wants to notify us that the command state has changed. We should update any
// controls we have that are dependent upon our embedded object, such as "Back", "Forward", "Stop", or "Home"
// buttons.
HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler FAR* This)
{
	// We update our UI in our window message loop so we don't do anything here.
	return(S_OK);
}

// Called from the browser object's IOleInPlaceActiveObject object's EnableModeless() function. Also
// called when the browser displays a modal dialog box.
HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler FAR* This, BOOL fEnable)
{
	return(S_OK);
}

// Called from the browser object's IOleInPlaceActiveObject object's OnDocWindowActivate() function.
// This informs off of when the object is getting/losing the focus.
HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler FAR* This, BOOL fActivate)
{
	return(S_OK);
}

// Called from the browser object's IOleInPlaceActiveObject object's OnFrameWindowActivate() function.
HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler FAR* This, BOOL fActivate)
{
	return(S_OK);
}

// Called from the browser object's IOleInPlaceActiveObject object's ResizeBorder() function.
HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler FAR* This, LPCRECT prcBorder, IOleInPlaceUIWindow __RPC_FAR *pUIWindow, BOOL fRameWindow)
{
	return(S_OK);
}

// Chiamato dal browser object's TranslateAccelerator routines per tradurre i tasti premuti in comandi.
HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler FAR* This, LPMSG lpMsg, const GUID __RPC_FAR *pguidCmdGroup, DWORD nCmdID)
{
	// We don't intercept any keystrokes, so we do nothing here. But for example, if we wanted to
	// override the TAB key, perhaps do something with it ourselves, and then tell the browser
	// not to do anything with this keystroke, we'd do:
/*
	//
		if (lpMsg && lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_TAB)
		{
			// Here we do something as a result of a TAB key press.
			MessageBox( 0, "TAB", "UI", 0 );

			// Tell the browser not to do anything with it.
			return(S_FALSE);
		}
	
		// Otherwise, let the browser do something with this message.
		return(S_OK);
	*/

	// For our example, we want to make sure that the user can invoke some key to popup the context
	// menu, so we'll tell it to ignore all messages.
	return(S_FALSE);
}

// Called by the browser object to find where the host wishes the browser to get its options in the registry.
// We can use this to prevent the browser from using its default settings in the registry, by telling it to use
// some other registry key we've setup with the options we want.
HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler FAR* This, LPOLESTR __RPC_FAR *pchKey, DWORD dw)
{
	// Let the browser use its default registry settings.
	return(S_FALSE);
}

// Called by the browser object when it is used as a drop target. We can supply our own IDropTarget object,
// IDropTarget functions, and IDropTarget VTable if we want to determine what happens when someone drags and
// drops some object on our embedded browser object.
HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler FAR* This, IDropTarget __RPC_FAR *pDropTarget, IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget)
{
	// Return our IDropTarget object associated with this IDocHostUIHandler object. I don't
	// know why we don't do this via UI_QueryInterface(), but we don't.

	// NOTE: If we want/need an IDropTarget interface, then we would have had to setup our own
	// IDropTarget functions, IDropTarget VTable, and create an IDropTarget object. We'd want to put
	// a pointer to the IDropTarget object in our own custom IDocHostUIHandlerEx object (like how
	// we may add an HWND field for the use of UI_ShowContextMenu). So when we created our
	// IDocHostUIHandlerEx object, maybe we'd add a 'idrop' field to the end of it, and
	// store a pointer to our IDropTarget object there. Then we could return this pointer as so:
	//
	// *pDropTarget = ((IDocHostUIHandlerEx FAR *)This)->idrop;
    // return(S_OK);

	// But for our purposes, we don't need an IDropTarget object, so we'll tell whomever is calling
	// us that we don't have one.
    return(S_FALSE);
}

// Called by the browser when it wants a pointer to our IDispatch object. This object allows us to expose
// our own automation interface (ie, our own COM objects) to other entities that are running within the
// context of the browser so they can call our functions if they want. An example could be a javascript
// running in the URL we display could call our IDispatch functions. We'd write them so that any args passed
// to them would use the generic datatypes like a BSTR for utmost flexibility.
HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler FAR* This, IDispatch __RPC_FAR *__RPC_FAR *ppDispatch)
{
	// Return our IDispatch object associated with this IDocHostUIHandler object. I don't
	// know why we don't do this via UI_QueryInterface(), but we don't.

	// NOTE: If we want/need an IDispatch interface, then we would have had to setup our own
	// IDispatch functions, IDispatch VTable, and create an IDispatch object. We'd want to put
	// a pointer to the IDispatch object in our custom _IDocHostUIHandlerEx object (like how
	// we may add an HWND field for the use of UI_ShowContextMenu). So when we defined our
	// _IDocHostUIHandlerEx object, maybe we'd add a 'idispatch' field to the end of it, and
	// store a pointer to our IDispatch object there. Then we could return this pointer as so:
	//
	// *ppDispatch = ((_IDocHostUIHandlerEx FAR *)This)->idispatch;
    // return(S_OK);

	// But for our purposes, we don't need an IDispatch object, so we'll tell whomever is calling
	// us that we don't have one. Note: We must set ppDispatch to 0 if we don't return our own
	// IDispatch object.
	*ppDispatch = 0;
	return(S_FALSE);
}

// Called by the browser object to give us an opportunity to modify the URL to be loaded.
HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler FAR* This, DWORD dwTranslate, OLECHAR __RPC_FAR *pchURLIn, OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut)
{
	// We don't need to modify the URL. Note: We need to set ppchURLOut to 0 if we don't
	// return an OLECHAR (buffer) containing a modified version of pchURLIn.
	*ppchURLOut = 0;
    return(S_FALSE);
}

// Called by the browser when it does cut/paste to the clipboard. This allows us to block certain clipboard
// formats or support additional clipboard formats.
HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler FAR* This, IDataObject __RPC_FAR *pDO, IDataObject __RPC_FAR *__RPC_FAR *ppDORet)
{
	// Return our IDataObject object associated with this IDocHostUIHandler object. I don't
	// know why we don't do this via UI_QueryInterface(), but we don't.

	// NOTE: If we want/need an IDataObject interface, then we would have had to setup our own
	// IDataObject functions, IDataObject VTable, and create an IDataObject object. We'd want to put
	// a pointer to the IDataObject object in our custom _IDocHostUIHandlerEx object (like how
	// we may add an HWND field for the use of UI_ShowContextMenu). So when we defined our
	// _IDocHostUIHandlerEx object, maybe we'd add a 'idata' field to the end of it, and
	// store a pointer to our IDataObject object there. Then we could return this pointer as so:
	//
	// *ppDORet = ((_IDocHostUIHandlerEx FAR *)This)->idata;
    // return(S_OK);

	// But for our purposes, we don't need an IDataObject object, so we'll tell whomever is calling
	// us that we don't have one. Note: We must set ppDORet to 0 if we don't return our own
	// IDataObject object.
	*ppDORet = 0;
	return(S_FALSE);
}





////////////////////////////////////// My IStorage functions  /////////////////////////////////////////
// NOTE: The browser object doesn't use the IStorage functions, so most of these are us just returning
// E_NOTIMPL so that anyone who *does* call these functions knows nothing is being done here.
//
// We give the browser object a pointer to our IStorage object when we call OleCreate().

HRESULT STDMETHODCALLTYPE Storage_QueryInterface(IStorage FAR* This, REFIID riid, LPVOID FAR* ppvObj)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_AddRef(IStorage FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Storage_Release(IStorage FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Storage_CreateStream(IStorage FAR* This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_OpenStream(IStorage FAR* This, const WCHAR * pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_CreateStorage(IStorage FAR* This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_OpenStorage(IStorage FAR* This, const WCHAR * pwcsName, IStorage * pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_CopyTo(IStorage FAR* This, DWORD ciidExclude, IID const *rgiidExclude, SNB snbExclude,IStorage *pstgDest)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_MoveElementTo(IStorage FAR* This, const OLECHAR *pwcsName,IStorage * pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_Commit(IStorage FAR* This, DWORD grfCommitFlags)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_Revert(IStorage FAR* This)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_EnumElements(IStorage FAR* This, DWORD reserved1, void * reserved2, DWORD reserved3, IEnumSTATSTG ** ppenum)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_DestroyElement(IStorage FAR* This, const OLECHAR *pwcsName)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_RenameElement(IStorage FAR* This, const WCHAR *pwcsOldName, const WCHAR *pwcsNewName)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_SetElementTimes(IStorage FAR* This, const WCHAR *pwcsName, FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_SetClass(IStorage FAR* This, REFCLSID clsid)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Storage_SetStateBits(IStorage FAR* This, DWORD grfStateBits, DWORD grfMask)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Storage_Stat(IStorage FAR* This, STATSTG * pstatstg, DWORD grfStatFlag)
{
	NOTIMPLEMENTED;
}









////////////////////////////////////// My IOleClientSite functions  /////////////////////////////////////
// We give the browser object a pointer to our IOleClientSite object when we call OleCreate() or DoVerb().

/************************* Site_QueryInterface() *************************
 * The browser object calls this when it wants a pointer to one of our
 * IOleClientSite, IDocHostUIHandler, or IOleInPlaceSite structures. They
 * are all accessible via the _IOleClientSiteEx struct we allocated in
 * EmbedBrowserObjectBuilder() and passed to DoVerb() and OleCreate().
 *
 * This =		A pointer to whatever _IOleClientSiteEx struct we passed to
 *				OleCreate() or DoVerb().
 * riid =		A GUID struct that the browser passes us to clue us as to
 *				which type of struct (object) it would like a pointer
 *				returned for.
 * ppvObject =	Where the browser wants us to return a pointer to the
 *				appropriate struct. (ie, It passes us a handle to fill in).
 *
 * RETURNS: S_OK if we return the struct, or E_NOINTERFACE if we don't have
 * the requested struct.
 */

HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite FAR* This, REFIID riid, void ** ppvObject)
{
	// It just so happens that the first arg passed to us is our _IOleClientSiteEx struct we allocated
	// and passed to DoVerb() and OleCreate(). Nevermind that 'This' is declared is an IOleClientSite *.
	// Remember that in EmbedBrowserObjectBuilder(), we allocated our own _IOleClientSiteEx struct, and lied
	// to OleCreate() and DoVerb() -- passing our _IOleClientSiteEx struct and saying it was an
	// IOleClientSite struct. It's ok. An _IOleClientSiteEx starts with an embedded IOleClientSite, so
	// the browser didn't mind. So that's what the browser object is passing us now. The browser doesn't
	// know that it's really an _IOleClientSiteEx struct. But we do. So we can recast it and use it as
	// so here.

	// If the browser is asking us to match IID_IOleClientSite, then it wants us to return a pointer to
	// our IOleClientSite struct. Then the browser will use the VTable in that struct to call our
	// IOleClientSite functions. It will also pass this same pointer to all of our IOleClientSite
	// functions.
	//
	// Actually, we're going to lie to the browser again. We're going to return our own _IOleClientSiteEx
	// struct, and tell the browser that it's a IOleClientSite struct. It's ok. The first thing in our
	// _IOleClientSiteEx is an embedded IOleClientSite, so the browser doesn't mind. We want the browser
	// to continue passing our _IOleClientSiteEx pointer wherever it would normally pass a IOleClientSite
	// pointer.
	// 
	// The IUnknown interface uses the same VTable as the first object in our _IOleClientSiteEx
	// struct (which happens to be an IOleClientSite). So if the browser is asking us to match
	// IID_IUnknown, then we'll also return a pointer to our _IOleClientSiteEx.

	if (!memcmp(riid, &IID_IUnknown, sizeof(GUID)) || !memcmp(riid, &IID_IOleClientSite, sizeof(GUID)))
		*ppvObject = &((_IOleClientSiteEx *)This)->client;

	// If the browser is asking us to match IID_IOleInPlaceSite, then it wants us to return a pointer to
	// our IOleInPlaceSite struct. Then the browser will use the VTable in that struct to call our
	// IOleInPlaceSite functions.  It will also pass this same pointer to all of our IOleInPlaceSite
	// functions (except for Site_QueryInterface, Site_AddRef, and Site_Release. Those will always get
	// the pointer to our _IOleClientSiteEx.
	//
	// Actually, we're going to lie to the browser. We're going to return our own _IOleInPlaceSiteEx
	// struct, and tell the browser that it's a IOleInPlaceSite struct. It's ok. The first thing in
	// our _IOleInPlaceSiteEx is an embedded IOleInPlaceSite, so the browser doesn't mind. We want the
	// browser to continue passing our _IOleInPlaceSiteEx pointer wherever it would normally pass a
	// IOleInPlaceSite pointer.
	else if (!memcmp(riid, &IID_IOleInPlaceSite, sizeof(GUID)))
		*ppvObject = &((_IOleClientSiteEx *)This)->inplace;

	// If the browser is asking us to match IID_IDocHostUIHandler, then it wants us to return a pointer to
	// our IDocHostUIHandler struct. Then the browser will use the VTable in that struct to call our
	// IDocHostUIHandler functions.  It will also pass this same pointer to all of our IDocHostUIHandler
	// functions (except for Site_QueryInterface, Site_AddRef, and Site_Release. Those will always get
	// the pointer to our _IOleClientSiteEx.
	//
	// Actually, we're going to lie to the browser. We're going to return our own _IDocHostUIHandlerEx
	// struct, and tell the browser that it's a IDocHostUIHandler struct. It's ok. The first thing in
	// our _IDocHostUIHandlerEx is an embedded IDocHostUIHandler, so the browser doesn't mind. We want the
	// browser to continue passing our _IDocHostUIHandlerEx pointer wherever it would normally pass a
	// IDocHostUIHandler pointer. My, we're really playing dirty tricks on the browser here. heheh.
	else if (!memcmp(riid, &IID_IDocHostUIHandler, sizeof(GUID)))
		*ppvObject = &((_IOleClientSiteEx *)This)->ui;

	// For other types of objects the browser wants, just report that we don't have any such objects.
	// NOTE: If you want to add additional functionality to your browser hosting, you may need to
	// provide some more objects here. You'll have to investigate what the browser is asking for
	// (ie, what REFIID it is passing).
	else
	{
		*ppvObject = 0;
		return(E_NOINTERFACE);
	}

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Site_AddRef(IOleClientSite FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Site_Release(IOleClientSite FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Site_SaveObject(IOleClientSite FAR* This)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_GetMoniker(IOleClientSite FAR* This, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_GetContainer(IOleClientSite FAR* This, LPOLECONTAINER FAR* ppContainer)
{
	// Tell the browser that we are a simple object and don't support a container
	*ppContainer = 0;

	return(E_NOINTERFACE);
}

HRESULT STDMETHODCALLTYPE Site_ShowObject(IOleClientSite FAR* This)
{
	return(NOERROR);
}

HRESULT STDMETHODCALLTYPE Site_OnShowWindow(IOleClientSite FAR* This, BOOL fShow)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Site_RequestNewObjectLayout(IOleClientSite FAR* This)
{
	NOTIMPLEMENTED;
}











////////////////////////////////////// My IOleInPlaceSite functions  /////////////////////////////////////
// The browser object asks us for the pointer to our IOleInPlaceSite object by calling our IOleClientSite's
// QueryInterface (ie, Site_QueryInterface) and specifying a REFIID of IID_IOleInPlaceSite.

HRESULT STDMETHODCALLTYPE InPlace_QueryInterface(IOleInPlaceSite FAR* This, REFIID riid, LPVOID FAR* ppvObj)
{
	// The browser assumes that our IOleInPlaceSite object is associated with our IOleClientSite
	// object. So it is possible that the browser may call our IOleInPlaceSite's QueryInterface()
	// to ask us to return a pointer to our IOleClientSite, in the same way that the browser calls
	// our IOleClientSite's QueryInterface() to ask for a pointer to our IOleInPlaceSite.
	//
	// Rather than duplicate much of the code in IOleClientSite's QueryInterface, let's just get
	// a pointer to our _IOleClientSiteEx object, substitute it as the 'This' arg, and call our
	// our IOleClientSite's QueryInterface. Note that since our IOleInPlaceSite is embedded right
	// inside our _IOleClientSiteEx, and comes immediately after the IOleClientSite, we can employ
	// the following trickery to get the pointer to our _IOleClientSiteEx.
	return(Site_QueryInterface((IOleClientSite *)((char *)This - sizeof(IOleClientSite)), riid, ppvObj));
}

HRESULT STDMETHODCALLTYPE InPlace_AddRef(IOleInPlaceSite FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE InPlace_Release(IOleInPlaceSite FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE InPlace_GetWindow(IOleInPlaceSite FAR* This, HWND FAR* lphwnd)
{
	// Return the HWND of the window that contains this browser object. We stored that
	// HWND in our _IOleInPlaceSiteEx struct. Nevermind that the function declaration for
	// Site_GetWindow says that 'This' is an IOleInPlaceSite *. Remember that in
	// EmbedBrowserObjectBuilder(), we allocated our own _IOleInPlaceSiteEx struct which
	// contained an embedded IOleInPlaceSite struct within it. And when the browser
	// called Site_QueryInterface() to get a pointer to our IOleInPlaceSite object, we
	// returned a pointer to our _IOleClientSiteEx. The browser doesn't know this. But
	// we do. That's what we're really being passed, so we can recast it and use it as
	// so here.
	*lphwnd = ((_IOleInPlaceSiteEx FAR*)This)->frame.window;

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_ContextSensitiveHelp(IOleInPlaceSite FAR* This, BOOL fEnterMode)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE InPlace_CanInPlaceActivate(IOleInPlaceSite FAR* This)
{
	// Tell the browser we can in place activate
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceActivate(IOleInPlaceSite FAR* This)
{
	// Tell the browser we did it ok
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_OnUIActivate(IOleInPlaceSite FAR* This)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_GetWindowContext(IOleInPlaceSite FAR* This, LPOLEINPLACEFRAME FAR* lplpFrame, LPOLEINPLACEUIWINDOW FAR* lplpDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	// Give the browser the pointer to our IOleInPlaceFrame struct. We stored that pointer
	// in our _IOleInPlaceSiteEx struct. Nevermind that the function declaration for
	// Site_GetWindowContext says that 'This' is an IOleInPlaceSite *. Remember that in
	// EmbedBrowserObjectBuilder(), we allocated our own _IOleInPlaceSiteEx struct which
	// contained an embedded IOleInPlaceSite struct within it. And when the browser
	// called Site_QueryInterface() to get a pointer to our IOleInPlaceSite object, we
	// returned a pointer to our _IOleClientSiteEx. The browser doesn't know this. But
	// we do. That's what we're really being passed, so we can recast it and use it as
	// so here.
	//
	// Actually, we're giving the browser a pointer to our own _IOleInPlaceSiteEx struct,
	// but telling the browser that it's a IOleInPlaceSite struct. No problem. Our
	// _IOleInPlaceSiteEx starts with an embedded IOleInPlaceSite, so the browser is
	// cool with it. And we want the browser to pass a pointer to this _IOleInPlaceSiteEx
	// wherever it would pass a IOleInPlaceSite struct to our IOleInPlaceSite functions.
	*lplpFrame = (LPOLEINPLACEFRAME)&((_IOleInPlaceSiteEx *)This)->frame;

	// We have no OLEINPLACEUIWINDOW
	*lplpDoc = 0;

	// Fill in some other info for the browser
	lpFrameInfo->fMDIApp = FALSE;
	lpFrameInfo->hwndFrame = ((_IOleInPlaceFrameEx *)*lplpFrame)->window;
	lpFrameInfo->haccel = 0; // ???'
	lpFrameInfo->cAccelEntries = 0;

	// Give the browser the dimensions of where it can draw. We give it our entire window to fill.
	// We do this in InPlace_OnPosRectChange() which is called right when a window is first
	// created anyway, so no need to duplicate it here.
//	GetClientRect(lpFrameInfo->hwndFrame, lprcPosRect);
//	GetClientRect(lpFrameInfo->hwndFrame, lprcClipRect);

	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_Scroll(IOleInPlaceSite FAR* This, SIZE scrollExtent)
{
	NOTIMPLEMENTED;
//	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_OnUIDeactivate(IOleInPlaceSite FAR* This, BOOL fUndoable)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceDeactivate(IOleInPlaceSite FAR* This)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE InPlace_DiscardUndoState(IOleInPlaceSite FAR* This)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE InPlace_DeactivateAndUndo(IOleInPlaceSite FAR* This)
{
	NOTIMPLEMENTED;
}

// Called when the position of the browser object is changed, such as when we call the IWebBrowser2's put_Width(),
// put_Height(), put_Left(), or put_Right().
HRESULT STDMETHODCALLTYPE InPlace_OnPosRectChange(IOleInPlaceSite FAR* This, LPCRECT lprcPosRect)
{
	IOleObject			*browserObject;
	IOleInPlaceObject	*inplace;

	// We need to get the browser's IOleInPlaceObject object so we can call its SetObjectRects
	// function.
	browserObject = *((IOleObject **) ((char *)This - sizeof(IOleObject *) - sizeof(IOleClientSite)));
	if (!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IOleInPlaceObject, (void**)&inplace))
	{
		// Give the browser the dimensions of where it can draw.
		inplace->lpVtbl->SetObjectRects(inplace, lprcPosRect, lprcPosRect);
	}

	return(S_OK);
}







////////////////////////////////////// My IOleInPlaceFrame functions  /////////////////////////////////////////

HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame FAR* This, REFIID riid, LPVOID FAR* ppvObj)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame FAR* This)
{
	return(1);
}

HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame FAR* This, HWND FAR* lphwnd)
{
	// Give the browser the HWND to our window that contains the browser object. We
	// stored that HWND in our IOleInPlaceFrame struct. Nevermind that the function
	// declaration for Frame_GetWindow says that 'This' is an IOleInPlaceFrame *. Remember
	// that in EmbedBrowserObjectBuilder(), we allocated our own IOleInPlaceFrameEx struct which
	// contained an embedded IOleInPlaceFrame struct within it. And then we lied when
	// Site_GetWindowContext() returned that IOleInPlaceFrameEx. So that's what the
	// browser is passing us. It doesn't know that. But we do. So we can recast it and
	// use it as so here.
	*lphwnd = ((_IOleInPlaceFrameEx *)This)->window;
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_ContextSensitiveHelp(IOleInPlaceFrame FAR* This, BOOL fEnterMode)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_GetBorder(IOleInPlaceFrame FAR* This, LPRECT lprectBorder)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_RequestBorderSpace(IOleInPlaceFrame FAR* This, LPCBORDERWIDTHS pborderwidths)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetBorderSpace(IOleInPlaceFrame FAR* This, LPCBORDERWIDTHS pborderwidths)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetActiveObject(IOleInPlaceFrame FAR* This, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_InsertMenus(IOleInPlaceFrame FAR* This, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetMenu(IOleInPlaceFrame FAR* This, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_RemoveMenus(IOleInPlaceFrame FAR* This, HMENU hmenuShared)
{
	NOTIMPLEMENTED;
}

HRESULT STDMETHODCALLTYPE Frame_SetStatusText(IOleInPlaceFrame FAR* This, LPCOLESTR pszStatusText)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_EnableModeless(IOleInPlaceFrame FAR* This, BOOL fEnable)
{
	return(S_OK);
}

HRESULT STDMETHODCALLTYPE Frame_TranslateAccelerator(IOleInPlaceFrame FAR* This, LPMSG lpmsg, WORD wID)
{
	return(S_OK);
//	NOTIMPLEMENTED;
}






//************************** UnEmbedBrowserObject() ************************
// Called to detach the browser object from our host window, and free its
// resources, right before we destroy our window.
//
// hwnd =		Handle to the window hosting the browser object.
//
// NOTE: The pointer to the browser object must have been stored in the
// window's USERDATA field. In other words, don't call UnEmbedBrowserObject().
// with a HWND that wasn't successfully passed to EmbedBrowserObjectBuilder().
void UnEmbedBrowserObject(HWND hwnd)
{
	//IOleObject	**browserHandle;
	//IOleObject	*browserObject;
	EH_WEBPAGE *psWebPage=_WebPageSearch(hwnd);

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	//if ((browserHandle = (IOleObject **) GetWindowLong(hwnd, GWL_USERDATA)))
	//if ((browserHandle = psWebPage->browserObject))
	if (psWebPage->pBrowserResource)

	{
		// Unembed the browser object, and release its resources.
		//browserObject = *browserHandle;
		psWebPage->browserObject->lpVtbl->Close(psWebPage->browserObject, OLECLOSE_NOSAVE);
		psWebPage->browserObject->lpVtbl->Release(psWebPage->browserObject);
//		ehFreePtr(&psWebPage->pBrowserResource);
		GlobalFree(psWebPage->pBrowserResource);
		return;
	}

	// You must have called this for a window that wasn't successfully passed to 
	// EmbedBrowserObjectBuilder(). Bad boy!
	_ASSERT(0);
}



/***************************** EmbedBrowserObjectBuilder() **************************
 * Puts the browser object inside our host window, and save a pointer to this
 * window's browser object in the window's GWL_USERDATA field.
 *
 * hwnd =		Handle of our window into which we embed the browser object.
 *
 * RETURNS: 0 if success, or non-zero if an error.
 *
 * NOTE: We tell the browser object to occupy the entire client area of the
 * window.
 *
 * NOTE: No HTML page will be displayed here. We can do that with a subsequent
 * call to either _ieNaviagePage() or _ieWritePage(). This is merely once-only
 * initialization for using the browser object. In a nutshell, what we do
 * here is get a pointer to the browser object in our window's GWL_USERDATA
 * so we can access that object's functions whenever we want, and we also pass
 * pointers to our IOleClientSite, IOleInPlaceFrame, and IStorage structs so that
 * the browser can call our functions in our struct's VTables.
 */

long EmbedBrowserObjectBuilder(HWND hwnd,EH_WEBPAGE	* psWebPage)
{
	IOleObject *		browserObject;
	IWebBrowser2 *		webBrowser2;
	RECT				rect;
	BYTE				*ptr;
	_IOleClientSiteEx	*_iOleClientSiteEx;
	//EH_WEBPAGE	*psWebPage=(EH_WEBPAGE*) lParam;

	// Our IOleClientSite, IOleInPlaceSite, and IOleInPlaceFrame functions need to get our window handle. We
	// could store that in some global. But then, that would mean that our functions would work with only that
	// one window. If we want to create multiple windows, each hosting its own browser object (to display its
	// own web page), then we need to create unique IOleClientSite, IOleInPlaceSite, and IOleInPlaceFrame
	// structs for each window. And we'll put an extra field at the end of those structs to store our extra
	// data such as a window handle. So, our functions won't have to touch global data, and can therefore be
	// re-entrant and work with multiple objects/windows.
	//
	// Remember that a pointer to our IOleClientSite we create here will be passed as the first arg to every
	// one of our IOleClientSite functions. Ditto with the IOleInPlaceFrame object we create here, and the
	// IOleInPlaceFrame functions. So, our functions are able to retrieve the window handle we'll store here,
	// and then, they'll work with all such windows containing a browser control.
	//
	// Furthermore, since the browser will be calling our IOleClientSite's QueryInterface to get a pointer to
	// our IOleInPlaceSite and IDocHostUIHandler objects, that means that our IOleClientSite QueryInterface
	// must have an easy way to grab those pointers. Probably the easiest thing to do is just embed our
	// IOleInPlaceSite and IDocHostUIHandler objects inside of an extended IOleClientSite which we'll call
	// a _IOleClientSiteEx. As long as they come after the pointer to the IOleClientSite VTable, then we're
	// ok.
	//
	// Of course, we need to GlobalAlloc the above structs now. We'll just get all 4 with a single call to
	// GlobalAlloc, especially since 3 of them are all contained inside of our _IOleClientSiteEx anyway.
	//
	// So, we're not actually allocating separate IOleClientSite, IOleInPlaceSite, IOleInPlaceFrame and
	// IDocHostUIHandler structs.
	//
	// One final thing. We're going to allocate extra room to store the pointer to the browser object.
	//ptr=ehAllocZero(sizeof(_IOleClientSiteEx) + sizeof(IOleObject *)); if (!ptr) return(-1);
	if (!(ptr = (char *)GlobalAlloc(GMEM_FIXED, sizeof(_IOleClientSiteEx) + sizeof(IOleObject *)))) return(-1);

	// Initialize our IOleClientSite object with a pointer to our IOleClientSite VTable.
	_iOleClientSiteEx = (_IOleClientSiteEx *)(ptr + sizeof(IOleObject *));
	_iOleClientSiteEx->client.lpVtbl = &MyIOleClientSiteTable;

	// Initialize our IOleInPlaceSite object with a pointer to our IOleInPlaceSite VTable.
	_iOleClientSiteEx->inplace.inplace.lpVtbl = &MyIOleInPlaceSiteTable;

	// Initialize our IOleInPlaceFrame object with a pointer to our IOleInPlaceFrame VTable.
	_iOleClientSiteEx->inplace.frame.frame.lpVtbl = &MyIOleInPlaceFrameTable;

	// Save our HWND (in the IOleInPlaceFrame object) so our IOleInPlaceFrame functions can retrieve it.
	_iOleClientSiteEx->inplace.frame.window = hwnd;

	// Initialize our IDocHostUIHandler object with a pointer to our IDocHostUIHandler VTable.
	_iOleClientSiteEx->ui.ui.lpVtbl = &MyIDocHostUIHandlerTable;

	// Get a pointer to the browser object and lock it down (so it doesn't "disappear" while we're using
	// it in this program). We do this by calling the OS function OleCreate().
	//	
	// NOTE: We need this pointer to interact with and control the browser. With normal WIN32 controls such as a
	// Static, Edit, Combobox, etc, you obtain an HWND and send messages to it with SendMessage(). Not so with
	// the browser object. You need to get a pointer to its "base structure" (as returned by OleCreate()). This
	// structure contains an array of pointers to functions you can call within the browser object. Actually, the
	// base structure contains a 'lpVtbl' field that is a pointer to that array. We'll call the array a 'VTable'.
	//
	// For example, the browser object happens to have a SetHostNames() function we want to call. So, after we
	// retrieve the pointer to the browser object (in a local we'll name 'browserObject'), then we can call that
	// function, and pass it args, as so:
	//
	// browserObject->lpVtbl->SetHostNames(browserObject, SomeString, SomeString);
	//
	// There's our pointer to the browser object in 'browserObject'. And there's the pointer to the browser object's
	// VTable in 'browserObject->lpVtbl'. And the pointer to the SetHostNames function happens to be stored in an
	// field named 'SetHostNames' within the VTable. So we are actually indirectly calling SetHostNames by using
	// a pointer to it. That's how you use a VTable.
	//
	// NOTE: We pass our _IOleClientSiteEx struct and lie -- saying that it's a IOleClientSite. It's ok. A
	// _IOleClientSiteEx struct starts with an embedded IOleClientSite. So the browser won't care, and we want
	// this extended struct passed to our IOleClientSite functions.

	if (!OleCreate(	&CLSID_WebBrowser,  // <------------------------------------------
					&IID_IOleObject, 
					OLERENDER_DRAW, 
					0, 
					(IOleClientSite *)_iOleClientSiteEx, 
					&MyIStorage, 
					(void**)&browserObject))
	{
		// Ok, we now have the pointer to the browser object in 'browserObject'. Let's save this in the
		// memory block we allocated above, and then save the pointer to that whole thing in our window's
		// USERDATA field. That way, if we need multiple windows each hosting its own browser object, we can
		// call EmbedBrowserObjectBuilder() for each one, and easily associate the appropriate browser object with
		// its matching window and its own objects containing per-window data.
		*((IOleObject **) ptr) = browserObject;
		psWebPage->pBrowserResource=ptr;
		psWebPage->browserObject=browserObject;//ptr;
		SetWindowLong(hwnd, GWL_USERDATA, (LONG) psWebPage);//(LONG) ptr);

		// We can now call the browser object's SetHostNames function. SetHostNames lets the browser object know our
		// application's name and the name of the document in which we're embedding the browser. (Since we have no
		// document name, we'll pass a 0 for the latter). When the browser object is opened for editing, it displays
		// these names in its titlebar.
		//	
		// We are passing 3 args to SetHostNames. You'll note that the first arg to SetHostNames is the base
		// address of our browser control. This is something that you always have to remember when working in C
		// (as opposed to C++). When calling a VTable function, the first arg to that function must always be the
		// structure which contains the VTable. (In this case, that's the browser control itself). Why? That's
		// because that function is always assumed to be written in C++. And the first argument to any C++ function
		// must be its 'this' pointer (ie, the base address of its class, which in this case is our browser object
		// pointer). In C++, you don't have to pass this first arg, because the C++ compiler is smart enough to
		// produce an executable that always adds this first arg. In fact, the C++ compiler is smart enough to
		// know to fetch the function pointer from the VTable, so you don't even need to reference that. In other
		// words, the C++ equivalent code would be:
		//
		// browserObject->SetHostNames(L"My Host Name", 0);
		//
		// So, when you're trying to convert C++ code to C, always remember to add this first arg whenever you're
		// dealing with a VTable (ie, the field is usually named 'lpVtbl') in the standard objects, and also add
		// the reference to the VTable itself.
		//
		// Oh yeah, the L is because we need UNICODE strings. And BTW, the host and document names can be anything
		// you want.

		browserObject->lpVtbl->SetHostNames(browserObject, L"My Host Name", 0);

		GetClientRect(hwnd, &rect);

		// Let browser object know that it is embedded in an OLE container.
		if (!OleSetContainedObject((struct IUnknown *)browserObject, TRUE) &&

			// Set the display area of our browser control the same as our window's size
			// and actually put the browser object into our window.
			!browserObject->lpVtbl->DoVerb(browserObject, OLEIVERB_SHOW, NULL, (IOleClientSite *)_iOleClientSiteEx, -1, hwnd, &rect) &&

			// Ok, now things may seem to get even trickier, One of those function pointers in the browser's VTable is
			// to the QueryInterface() function. What does this function do? It lets us grab the base address of any
			// other object that may be embedded within the browser object. And this other object has its own VTable
			// containing pointers to more functions we can call for that object.
			//
			// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
			// object, so we can call some of the functions in the former's table. For example, one IWebBrowser2 function
			// we intend to call below will be Navigate2(). So we call the browser object's QueryInterface to get our
			// pointer to the IWebBrowser2 object.
			!browserObject->lpVtbl->QueryInterface(browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
		{
			// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
			// webBrowser2->lpVtbl.

			// Let's call several functions in the IWebBrowser2 object to position the browser display area
			// in our window. The functions we call are put_Left(), put_Top(), put_Width(), and put_Height().
			// Note that we reference the IWebBrowser2 object's VTable to get pointers to those functions. And
			// also note that the first arg we pass to each is the pointer to the IWebBrowser2 object.
			psWebPage->sIe.piWebBrowser2=webBrowser2;
			webBrowser2->lpVtbl->put_Left(webBrowser2, 0);
			webBrowser2->lpVtbl->put_Top(webBrowser2, 0);
			webBrowser2->lpVtbl->put_Width(webBrowser2, rect.right);
			webBrowser2->lpVtbl->put_Height(webBrowser2, rect.bottom);

			// We no longer need the IWebBrowser2 object (ie, we don't plan to call any more functions in it
			// right now, so we can release our hold on it). Note that we'll still maintain our hold on the
			// browser object until we're done with that object.
			//webBrowser2->lpVtbl->Release(webBrowser2);

			// Success
			return(0);
		}

		// Something went wrong!
		UnEmbedBrowserObject(hwnd);
		return(-3);
	}

	GlobalFree(ptr);
	return(-2);
}


/******************************* _browserResize() ****************************
 * Resizes the browser object for the specified window to the specified
 * width and height.
 *
 * hwnd =			Handle to the window hosting the browser object.
 * width =			Width.
 * height =			Height.
 *
 * NOTE: EmbedBrowserObjectBuilder() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObjectBuilder() once only, and then you can make multiple calls to
 * this function to resize the browser object.
 */

static void _browserResize(HWND hwnd, DWORD width, DWORD height)
{
	// EH_OBJ *	psObj;
	IWebBrowser2	*	webBrowser2;
//	IOleObject		*browserObject;
	EH_WEBPAGE		*	psWebPage=_WebPageSearch(hwnd);
	
//	browserObject=psWebPage->browserObject;
	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	//browserObject = *((IOleObject **)GetWindowLong(hwnd, GWL_USERDATA));

	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
	// object, so we can call some of the functions in the former's table.
	//if (!psWebPage->browserObject->lpVtbl->QueryInterface(psWebPage->browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
	if (webBrowser2=psWebPage->sIe.piWebBrowser2) 
	{
		// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
		// webBrowser2->lpVtbl.

		// Call are put_Width() and put_Height() to set the new width/height.
		psWebPage->sizWin.cx=width;
		psWebPage->sizWin.cy=height;
		webBrowser2->lpVtbl->put_Width(webBrowser2, width);
		webBrowser2->lpVtbl->put_Height(webBrowser2, height);

		// We no longer need the IWebBrowser2 object (ie, we don't plan to call any more functions in it,
		// so we can release our hold on it). Note that we'll still maintain our hold on the browser
		// object.
//		webBrowser2->lpVtbl->Release(webBrowser2);
	}
}
/*
static void _browserShow(HWND hwnd)
{
	IWebBrowser2	*	webBrowser2;
	EH_WEBPAGE		*	psWebPage=_WebPageSearch(hwnd);
	
	printf("%s=%x" CRLF,psWebPage->lpObj->nome,hwnd);
	if (webBrowser2=psWebPage->sIe.piWebBrowser2) 
	{

		// Call are put_Width() and put_Height() to set the new width/height.
		//webBrowser2->lpVtbl->put_Left(webBrowser2, 0);
		//webBrowser2->lpVtbl->put_Top(webBrowser2, 0);
		webBrowser2->lpVtbl->put_Visible(webBrowser2,VARIANT_TRUE);
//		webBrowser2->lpVtbl->Release(webBrowser2);
	}
}
*/

void PrintDocument(HWND hwnd,INT iMode)
{
	//IOleObject		*browserObject;
	IWebBrowser2 *	webBrowser2;
	LPDISPATCH		lpDispatch;
	EH_WEBPAGE		*psWebPage=_WebPageSearch(hwnd);
	//browserObject=psWebPage->browserHandle;
	
	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	//win_infoarg("1");
//	browserObject = *((IOleObject **)GetWindowLong(hwnd, GWL_USERDATA));

	//win_infoarg("2");
	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded 
	// within the browser object, so we can call some of the functions in the former's table.
//	if (!psWebPage->browserObject->lpVtbl->QueryInterface(psWebPage->browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
	if (webBrowser2=psWebPage->sIe.piWebBrowser2)
	{
		// Now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is:
		//  webBrowser2->lpVtbl.
		//win_infoarg("3");

		// Call the IWebBrowser2 object's get_Document so we can get its DISPATCH object. 
		// I don't know why you don't get the DISPATCH object via the browser object's 
		// QueryInterface(), but you don't.
		if (!webBrowser2->lpVtbl->get_Document(webBrowser2, &lpDispatch))
		{

			// Recupero del CommandTarget
			LPOLECOMMANDTARGET lpOleCommandTarget = NULL;
			if (!lpDispatch->lpVtbl->QueryInterface(lpDispatch, &IID_IOleCommandTarget, (void**)&lpOleCommandTarget))
			{
				//win_infoarg("WebPage: QUI %d",iMode);

				//VARIANT	bStr;
				//VariantInit(&bStr);
				/*
				bStr.vt = VT_BSTR;
				bStr.bstrVal = SysAllocString(L"Testina");
				lpOleCommandTarget->lpVtbl->Exec(lpOleCommandTarget, NULL, OLECMDID_SETTITLE, 0, &bStr, NULL);
				*/
				switch (iMode)
				{
					case 0: // Comando di stampa diretta
						lpOleCommandTarget->lpVtbl->Exec(lpOleCommandTarget, NULL, OLECMDID_PRINT, 0, NULL, NULL);
						break;

					case 1: // Comando di stampa con preview
						//win_infoarg("QUI");
						lpOleCommandTarget->lpVtbl->Exec(lpOleCommandTarget, NULL, OLECMDID_PRINTPREVIEW, 0, NULL, NULL);
						break;

					case 2: // Comando di stampa diretta senza chiedere la stampante
						lpOleCommandTarget->lpVtbl->Exec(lpOleCommandTarget, NULL, OLECMDID_PRINT, OLECMDEXECOPT_DONTPROMPTUSER , NULL, NULL);
						break;

				}
				//ehLogWrite("[%s]",bStr.bstrVal);
				//VariantClear(&bStr);


//Altri comandi
/*	 OLECMDID_OPEN                   = 1, 
     OLECMDID_NEW                    = 2, 
     OLECMDID_SAVE                   = 3, 
     OLECMDID_SAVEAS                 = 4, 
     OLECMDID_SAVECOPYAS             = 5, 
     OLECMDID_PRINT                  = 6, 
     OLECMDID_PRINTPREVIEW           = 7, 
     OLECMDID_PAGESETUP              = 8, 
     OLECMDID_SPELL                  = 9, 
     OLECMDID_PROPERTIES             = 10, 
     OLECMDID_CUT                    = 11, 
     OLECMDID_COPY                   = 12, 
     OLECMDID_PASTE                  = 13, 
     OLECMDID_PASTESPECIAL           = 14, 
     OLECMDID_UNDO                   = 15, 
     OLECMDID_REDO                   = 16, 
     OLECMDID_SELECTALL              = 17, 
     OLECMDID_CLEARSELECTION         = 18, 
     OLECMDID_ZOOM                   = 19, 
     OLECMDID_GETZOOMRANGE           = 20, 
     OLECMDID_UPDATECOMMANDS         = 21, 
     OLECMDID_REFRESH                = 22, 
     OLECMDID_STOP                   = 23, 
     OLECMDID_HIDETOOLBARS           = 24, 
     OLECMDID_SETPROGRESSMAX         = 25, 
     OLECMDID_SETPROGRESSPOS         = 26, 
     OLECMDID_SETPROGRESSTEXT        = 27, 
     OLECMDID_SETTITLE               = 28, 
     OLECMDID_SETDOWNLOADSTATE       = 29, 
     OLECMDID_STOPDOWNLOAD           = 30, 
     OLECMDID_ONTOOLBARACTIVATED     = 31, 
     OLECMDID_FIND                   = 32, 
     OLECMDID_DELETE                 = 33, 
     OLECMDID_HTTPEQUIV              = 34, 
     OLECMDID_HTTPEQUIV_DONE         = 35, 
     OLECMDID_ENABLE_INTERACTION     = 36, 
     OLECMDID_ONUNLOAD               = 37, 
     OLECMDID_PROPERTYBAG2           = 38, 
     OLECMDID_PREREFRESH             = 39, 
     OLECMDID_SHOWSCRIPTERROR        = 40, 
     OLECMDID_SHOWMESSAGE            = 41, 
     OLECMDID_SHOWFIND               = 42, 
     OLECMDID_SHOWPAGESETUP          = 43, 
     OLECMDID_SHOWPRINT              = 44, 
     OLECMDID_CLOSE                  = 45, 
     OLECMDID_ALLOWUILESSSAVEAS      = 46, 
     OLECMDID_DONTDOWNLOADCSS        = 47, 
     OLECMDID_UPDATEPAGESTATUS       = 48, 
     OLECMDID_PRINT2                 = 49, 
     OLECMDID_PRINTPREVIEW2          = 50, 
     OLECMDID_SETPRINTTEMPLATE       = 51, 
     OLECMDID_GETPRINTTEMPLATE       = 52, 
     OLECMDID_PAGEACTIONBLOCKED      = 55,
     OLECMDID_PAGEACTIONUIQUERY      = 56,
     OLECMDID_FOCUSVIEWCONTROLS      = 57,
     OLECMDID_FOCUSVIEWCONTROLSQUERY = 58,
     OLECMDID_SHOWPAGEACTIONMENU     = 59
*/


				// Rilascio
				lpOleCommandTarget->lpVtbl->Release(lpOleCommandTarget);
			}

/*			// We want to get a pointer to the IHTMLDocument2 object embedded within the 
			// DISPATCH object, so we can call some of the functions in the former's table.
			if (!lpDispatch->lpVtbl->QueryInterface(lpDispatch, &IID_IHTMLDocument2, (void**)&htmlDoc2))
			{
				// The pointer to our IHTMLDocument2 object is in 'htmlDoc2', and so its VTable is:
				// htmlDoc2->lpVtbl.

				*pbrowserObject = browserObject;
				*pwebBrowser2 = webBrowser2;
				*plpDispatch = lpDispatch;
				*phtmlDoc2 = htmlDoc2;
				return 0;
			}
*/


			// Release the DISPATCH object.
			lpDispatch->lpVtbl->Release(lpDispatch);
		}


		// We no longer need the IWebBrowser2 object (ie, we don't plan to call any more functions in it,
		// so we can release our hold on it). Note that we'll still maintain our hold on the browser
		// object.
		webBrowser2->lpVtbl->Release(webBrowser2);
	}
}






/******************************* DoPageAction() **************************
 * Implements the functionality of a "Back". "Forward", "Home", "Search",
 * "Refresh", or "Stop" button.
 *
 * hwnd =		Handle to the window hosting the browser object.
 * action =		One of the following:
 *				0 = Move back to the previously viewed web page.
 *				1 = Move forward to the previously viewed web page.
 *				2 = Move to the home page.
 *				3 = Search.
 *				4 = Refresh the page.
 *				5 = Stop the currently loading page.
 *
 * NOTE: EmbedBrowserObjectBuilder() must have been successfully called once with the
 * specified window, prior to calling this function. You need call
 * EmbedBrowserObjectBuilder() once only, and then you can make multiple calls to
 * this function to display numerous pages in the specified window.
 */

#define WEBPAGE_GOBACK		0
#define WEBPAGE_GOFORWARD	1
#define WEBPAGE_GOHOME		2
#define WEBPAGE_SEARCH		3
#define WEBPAGE_REFRESH		4
#define WEBPAGE_STOP		5

void DoPageAction(HWND hwnd, DWORD action)
{	
	IWebBrowser2 *	webBrowser2;
	EH_WEBPAGE *psWebPage=_WebPageSearch(hwnd);
	//IOleObject		*browserObject;

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	//browserObject = *((IOleObject **)GetWindowLong(hwnd, GWL_USERDATA));

	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded within the browser
	// object, so we can call some of the functions in the former's table.
	//if (!psWebPage->browserObject->lpVtbl->QueryInterface(psWebPage->browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
	if (webBrowser2=psWebPage->sIe.piWebBrowser2)
	{
		// Ok, now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is
		// webBrowser2->lpVtbl.

		// Call the desired function
		switch (action)
		{
			case WEBPAGE_GOBACK:
				webBrowser2->lpVtbl->GoBack(webBrowser2);
				break;
			
			case WEBPAGE_GOFORWARD:
				webBrowser2->lpVtbl->GoForward(webBrowser2);
				break;
			
			case WEBPAGE_GOHOME:
				webBrowser2->lpVtbl->GoHome(webBrowser2);
				break;
			
			case WEBPAGE_SEARCH:
				webBrowser2->lpVtbl->GoSearch(webBrowser2);
				break;
			
			case WEBPAGE_REFRESH:
				webBrowser2->lpVtbl->Refresh(webBrowser2);
				break;
			
			case WEBPAGE_STOP:
				webBrowser2->lpVtbl->Stop(webBrowser2);
				break;
		}

		// Release the IWebBrowser2 object.
		webBrowser2->lpVtbl->Release(webBrowser2);
	}
}




//******************************* _ieWritePage() ****************************
// Takes a string containing some HTML BODY, and displays it in the specified
// window. For example, perhaps you want to display the HTML text of...
//
// <P>This is a picture.<P><IMG src="mypic.jpg">
//
// hwnd =		Handle to the window hosting the browser object.
// string =		Pointer to nul-terminated string containing the HTML BODY.
//				(NOTE: No <BODY></BODY> tags are required in the string).
//
// RETURNS: 0 if success, or non-zero if an error.
//
// NOTE: EmbedBrowserObjectBuilder() must have been successfully called once with the
// specified window, prior to calling this function. You need call
// EmbedBrowserObjectBuilder() once only, and then you can make multiple calls to
// this function to display numerous pages in the specified window.

//
// _ieWritePage()
//

long _ieWritePage(EH_WEBPAGE * psWebPage, LPCTSTR string,INT iParam)
{	
	SAFEARRAY		*sfArray;
	
	VARIANT			*pVar;
	BSTR			bstr=NULL;
	INT iLevelError;
	_IEI *			psIe=&psWebPage->sIe;

	if (!psWebPage) return (-1);

	bstr = 0;
	iLevelError=1;
	/*
	if (!psWebPage->bHtmlReady) {

		VARIANT	myURL;
		VariantInit(&myURL);
		myURL.vt = VT_BSTR;
		myURL.bstrVal = SysAllocString(L"about:blank");
		// Call the Navigate2() function to actually display the page.
		psIe->piWebBrowser2->lpVtbl->Navigate2(psIe->piWebBrowser2, &myURL, 0, 0, 0, 0);
		// Free any resources (including the BSTR).
		VariantClear(&myURL);
	}
*/
	iLevelError=2;
	_ieCreate(psWebPage,FALSE);
	if ((sfArray = SafeArrayCreate(VT_VARIANT, 1, (SAFEARRAYBOUND *)&ArrayBound))) // Creo l'array di strutture VARIANT
	{
		iLevelError=5;

		if (!SafeArrayAccessData(sfArray, (void**) &pVar)) // Chiedo l'accesso all'array e gli passo pVar
		{
			WCHAR *pw;
			pVar->vt = VT_BSTR;
#ifndef UNICODE
			// Trasformo la CHAR in WCHAR
			//wchar_t		*buffer;
			//DWORD		size;
			iLevelError=6;
			if (iParam&&!(iParam&WP_MT)) OsEventLoop(5); // {for (x=0;x<5;x++) eventGet(NULL);}
			pw=strToWcs((CHAR *) string); if (!pw) ehError();
			bstr = SysAllocString(pw);//buffer);
			ehFree(pw);
#else
			bstr = SysAllocString(string);
#endif
			iLevelError=7;
			//
			// memorizzo il nostro BSTR pointer nel VARIANT.
			if (pVar->bstrVal = bstr)
			{
				IDispatch* pElemDisp = NULL;
				VARIANT vnull;
				vnull.vt=VT_ERROR;
				vnull.scode=DISP_E_PARAMNOTFOUND; 
				iLevelError=8;

				// Passo il VARIENT con il suot BSTR a write() in ordine per mostrare
				// l'HTML string nel corpo della pagina vuota da noi creata sopra
				if (psIe->piHtmlDoc2) {
					psIe->piHtmlDoc2->lpVtbl->open(psIe->piHtmlDoc2, L"html/txt", vnull, vnull, vnull, &pElemDisp); // new 2009
					psIe->piHtmlDoc2->lpVtbl->write(psIe->piHtmlDoc2, sfArray);
					psIe->piHtmlDoc2->lpVtbl->close(psIe->piHtmlDoc2); // new 2009
				}
				// Normally, we'd need to free our BSTR, but SafeArrayDestroy() does // <--------------- GUARDA!
				// it for us:
				// SysFreeString(bstr);
			}
		}
		SafeArrayDestroy(sfArray);
		SysFreeString(bstr);
	}

	// No error?
	if (iLevelError==8) 
	{
		if (iParam) _LWaiting(psWebPage,iParam);
		psWebPage->bHtmlReady=true;
		return(0);
	}

	win_infoarg("_ieWritePage error %d",iLevelError);
	return(-1);
}



//******************************* _ieNaviagePage() ****************************
// Displays a URL, or HTML file on disk.
//
// hwnd =		Handle to the window hosting the browser object.
// webPageName =	Pointer to nul-terminated name of the URL/file.
//
// RETURNS: 0 if success, or non-zero if an error.
//
// NOTE: EmbedBrowserObjectBuilder() must have been successfully called once with the
// specified window, prior to calling this function. You need call
// EmbedBrowserObjectBuilder() once only, and then you can make multiple calls to
// this function to display numerous pages in the specified window.
//

//
// _ieWritePage()
//
long _ieNavigatePage(EH_WEBPAGE *psWebPage, CHAR * pszWebPage,INT iParam)
{
	VARIANT			myURL;
	WCHAR * pwcUrl;

	psWebPage->iStatusLoading=0; // Resetto lo status della pagina da caricare

	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded 
	// within the browser object, so we can call some of the functions in the former's table.
//	if (!psWebPage->browserObject->lpVtbl->QueryInterface(psWebPage->browserObject, &IID_IWebBrowser2, (void**)&webBrowser2))
	if (psWebPage->sIe.piWebBrowser2) {
		// Now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is:
		// webBrowser2->lpVtbl.

		// Our URL (ie, web address, such as "http://www.microsoft.com" or an HTM filename on disk
		// such as "c:\myfile.htm") must be passed to the IWebBrowser2's Navigate2() function as 
		// a BSTR. A BSTR is like a pascal version of a double-byte character string. In other 
		// words, the first unsigned short is a count of how many characters are in the string, 
		// and then this is followed by those characters, each expressed as an unsigned short 
		// (rather than a char). The string is not nul-terminated. The OS function SysAllocString 
		// can allocate and copy a UNICODE C string to a BSTR. Of course, we'll need to free that 
		// BSTR after we're done with it. If we're not using UNICODE, we first have to convert 
		// to a UNICODE string.
		//
		// What's more, our BSTR needs to be stuffed into a VARIENT struct, and that VARIENT 
		// struct is then passed to Navigate2(). Why? The VARIENT struct makes it possible to 
		// define generic 'datatypes' that can be used with all languages. Not all languages 
		// support things like nul-terminated C strings. So, by using a VARIENT, whose first 
		// field tells what sort of data (ie, string, float, etc) is in the VARIENT, 
		// COM interfaces can be used by just about any language.

		VariantInit(&myURL);
		myURL.vt = VT_BSTR;
		pwcUrl=strToWcs(pszWebPage);
		myURL.bstrVal = SysAllocString(pwcUrl);
		ehFree(pwcUrl);
		//GlobalFree(pwcUrl);

		if (!myURL.bstrVal) return(-6);
		psWebPage->sIe.piWebBrowser2->lpVtbl->Navigate2(psWebPage->sIe.piWebBrowser2, &myURL, 0, 0, 0, 0);
		VariantClear(&myURL);

		if (iParam) _LWaiting(psWebPage,iParam);
		psWebPage->bHtmlReady=TRUE;
		return(0);
	}
	return(-5);
}

static void _LWaiting(EH_WEBPAGE *psWebPage,INT iParam)
{
	INT a=0;
	if (iParam&WP_WAITING)
	{
	  while (psWebPage->iStatusLoading&2) 
	  {
		  if (!(iParam&WP_MT)) {a++; if (!(a%5)) WindowsMessageDispatch();;}//eventGet(NULL);}
//		  if (psWebPage->iStatusLoading&8) break;
		  if (psWebPage->iStatusLoading&4) break;
	  }
	}
}





// ****************************************************************************
// * Procedure per manipolare il controllo WebBrowser e il suo contenuto
// * In particolare ci sono funzioni per ottenere/rilasciare la pagina HTML e 
// * gli elementi HTML. Si può anche attivare/disattivare la gestione degli eventi
// * del controllo WebBrowser.
/*
void ReleaseHTMLDocument(IOleObject *browserObject, IWebBrowser2 *webBrowser2, LPDISPATCH lpDispatch, IHTMLDocument2 *htmlDoc2)
{
	// Release the IHTMLDocument2 object.
	htmlDoc2->lpVtbl->Release(htmlDoc2);

	// Release the DISPATCH object.
	lpDispatch->lpVtbl->Release(lpDispatch);
	
	// Release the IWebBrowser2 object.
	webBrowser2->lpVtbl->Release(webBrowser2);
}
*/

/*

IOleObject **pbrowserObject
  |D> IWebBrowser2 ... ->get_Document( =D)
        |D> IHTMLDocument2
 */

/*
						IOleObject **pbrowserObject, 
						IWebBrowser2 **pwebBrowser2, 
						LPDISPATCH *plpDispatch, 
						IHTMLDocument2 ** phtmlDoc2,
						IHTMLDocument3 ** phtmlDoc3)
						*/

//
// _ieCreate() - Crea un struttura con i puntatori alle interfacce
//
static BOOL _ieCreate(EH_WEBPAGE * psWebPage,BOOL bGetTag) //_IEI * psIe, HWND hwnd, BOOL bGetTag) {	
{
	BOOL	bError=TRUE;
	_IEI *	psIe=&psWebPage->sIe;

	// We want to get the base address (ie, a pointer) to the IWebBrowser2 object embedded 
	// within the browser object, so we can call some of the functions in the former's table.
	if (!psIe->piWebBrowser2) psWebPage->browserObject->lpVtbl->QueryInterface(psWebPage->browserObject, &IID_IWebBrowser2, (void**) &psIe->piWebBrowser2);
	if (psIe->piWebBrowser2) {
		// Now the pointer to our IWebBrowser2 object is in 'webBrowser2', and so its VTable is:
		//  webBrowser2->lpVtbl.

		// Call the IWebBrowser2 object's get_Document so we can get its DISPATCH object. 
		// I don't know why you don't get the DISPATCH object via the browser object's 
		// QueryInterface(), but you don't.
		if (!psIe->lpDispatch) {
			psIe->piWebBrowser2->lpVtbl->get_Document(psIe->piWebBrowser2, &psIe->lpDispatch);
			if (!psIe->lpDispatch) {

				VARIANT vEmpty;
				BSTR bstrURL = SysAllocString(L"about:blank");
				HRESULT hr;
				VariantInit(&vEmpty);
				
				hr = psIe->piWebBrowser2->lpVtbl->Navigate(psIe->piWebBrowser2,bstrURL, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
				if (SUCCEEDED(hr))
				{
				   psIe->piWebBrowser2->lpVtbl->put_Visible(psIe->piWebBrowser2,VARIANT_TRUE);
				   psIe->piWebBrowser2->lpVtbl->get_Document(psIe->piWebBrowser2, &psIe->lpDispatch);
				}
				   /*
				   else
				   {
					   pBrowser2->Quit();
				   }
				   */
				SysFreeString(bstrURL);
//				   pBrowser2->Release();
//			   }
				// psIe->piWebBrowser2->put_lpVtbl->get_Document(psIe->piWebBrowser2, &psIe->lpDispatch);
			
			}
		}
		if (psIe->lpDispatch) {

			if (!psIe->piHtmlDoc3) psIe->lpDispatch->lpVtbl->QueryInterface(psIe->lpDispatch, &IID_IHTMLDocument3, (void**)&psIe->piHtmlDoc3);
			if (!psIe->piHtmlDoc2) psIe->lpDispatch->lpVtbl->QueryInterface(psIe->lpDispatch, &IID_IHTMLDocument2, (void**)&psIe->piHtmlDoc2);
			//if (!psIe->piHtmlDoc2) psIe->lpDispatch->lpVtbl->QueryInterface(psIe->lpDispatch, &IID_IHTMLDocument2, (void**)&psIe->piHtmlDoc2);

//			if (!psIe->piHtmlWin2) psIe->piHtmlDoc2->lpVtbl->get_parentWindow(psIe->piHtmlDoc2,&psIe->piHtmlWin2);
			
			if (psIe->piHtmlDoc2) {
				bError=FALSE;

				//
				// Leggo l'interfaccia per avere l'elenco dei Tag se richiesto
				//
				if (bGetTag) {
					if ( SUCCEEDED( psIe->piHtmlDoc2->lpVtbl->get_all(psIe->piHtmlDoc2, &psIe->piElemTag) ) )
					{
						bError=FALSE;
					} else bError=TRUE;
				}
			}
		}
	}

	//
	// Se ho l'interfaccia, aggiorno il numero dei Tag
	//
	if (psIe->piElemTag) psIe->piElemTag->lpVtbl->get_length(psIe->piElemTag,&psIe->iTags);
	return bError;
}
/*
static BOOL _ieGetTag(EH_WEBPAGE *psWebPage) {

	if (!psWebPage->sIe.piElemTag) ehError();
	psWebPage->sIe.piElemTag->lpVtbl->get_length(psWebPage->sIe.piElemTag,&psWebPage->sIe.iTags);
	return FALSE;
}
*/

//
// _ieDestroy(psIe);
// 
static long _ieDestroy(_IEI * psIe) {

	_$Release(psIe->piHtmlDoc3);
	_$Release(psIe->piHtmlDoc2);
	_$Release(psIe->lpDispatch);
	_$Release(psIe->piWebBrowser2);
	_$Release(psIe->piElemTag);
	memset(psIe,0,sizeof(_IEI));
	return FALSE;
}

BOOL DestroyWebEvents(HWND hwnd, DWORD dwAdviseCookie) 
{
	// This function disconnects our event handler. This allows the refCount to
	// be reduced to zero and the memory it uses deallocated. This function
	// should be called just before you destroy the web browser object.
	// The dwAdviseCookie argument should be the return value of the call
	// to InitializeWebEvents.

	//IOleObject					*browserObject;
	HRESULT 					hret;
	IConnectionPoint			*pCP;
	IConnectionPointContainer	*pCPC;
	EH_WEBPAGE *psWebPage=_WebPageSearch(hwnd);
	// Get browser object
	//browserObject = *((IOleObject **)GetWindowLong(hwnd, GWL_USERDATA));

	// Get the connection point container
	hret = psWebPage->browserObject->lpVtbl->QueryInterface(psWebPage->browserObject,&IID_IConnectionPointContainer, (void**)(&pCPC));
	if (hret == S_OK) 
	{
		// Get the appropriate connection point
		hret = pCPC->lpVtbl->FindConnectionPoint(pCPC,&DIID_DWebBrowserEvents2, &pCP);
		if (hret == S_OK) 
		{
			// Unadvise the connection point of our event sink
			hret = pCP->lpVtbl->Unadvise(pCP,dwAdviseCookie);
			pCP->lpVtbl->Release(pCP);
		}
		pCPC->lpVtbl->Release(pCPC);
	}

	return (hret == S_OK);

}

DWORD ConnectWebEvents(HWND hwnd) 
{
	// This function will connect our event sink to the web browser control
	// Upon completion we should start receving events.
	// The function returns an advise cookie. This cookie should be passed
	// to the DestroyWebEvents function to cleanup the event handler
	// before the web browser object is destroyed.
	// A return value of 0 means failure.

	_DWebBrowserEvents2Ex		*WebEventsEx;
	//IOleObject					*browserObject;
	HRESULT 					hret;
	IConnectionPoint			*pCP;
	IConnectionPointContainer	*pCPC;
	DWORD                       dwAdviseCookie = 0;
	EH_WEBPAGE *psWebPage=_WebPageSearch(hwnd);

	// Retrieve the browser object's pointer we stored in our window's GWL_USERDATA when
	// we initially attached the browser object to this window.
	//browserObject = *((IOleObject **) GetWindowLong(hwnd, GWL_USERDATA));

	// Allocate memory for our event sink object
	if ( !(WebEventsEx = (_DWebBrowserEvents2Ex *) GlobalAlloc(GMEM_FIXED, sizeof(_DWebBrowserEvents2Ex) ) ) ){
		return 0;
	}

	// Initialize the _DWebBrowserEventsEx object 
	WebEventsEx->WebEventsObj.lpVtbl = &MyDWebBrowserEvents2Vtbl;
	WebEventsEx->refCount = 0;
	WebEventsEx->hWnd = hwnd;

	// Get the connection point container
	hret = psWebPage->browserObject->lpVtbl->QueryInterface(psWebPage->browserObject,&IID_IConnectionPointContainer, (void**)(&pCPC));
	if (hret == S_OK) 
	{
		// Get the appropriate connection point
		hret = pCPC->lpVtbl->FindConnectionPoint(pCPC,&DIID_DWebBrowserEvents2, &pCP);
		if (hret == S_OK) 
		{
			// Advise the connection point of our event sink
			hret = pCP->lpVtbl->Advise(pCP, (IUnknown *) WebEventsEx, &dwAdviseCookie);
			pCP->lpVtbl->Release(pCP);
		}
		pCPC->lpVtbl->Release(pCPC);
	}

	if (dwAdviseCookie == 0) 
	{
		// connection failed - free our event sink memory
		GlobalFree(WebEventsEx);
		return 0;
	}

	return dwAdviseCookie;
}

/*
void EventBuildConnector(HWND hwnd)
{
	if ( dwGlobalCookieWB==0 )
	{
		// Associo il gestore degli eventi
		DWORD dwResult = ConnectWebEvents( hwnd );
		if (  dwResult != 0)
		{
			dwGlobalCookieWB = dwResult;
			//MessageBox( hwnd, "Gestore eventi WB attivato", "Eventi WB", 0);
		}
		else
			MessageBox( hwnd, "Impossibile attivare il gestore eventi WB", "Eventi WB", 0);
		}
	else
	{
		// Disassocio il gestore degli eventi
		DestroyWebEvents( hwnd, dwGlobalCookieWB );
		dwGlobalCookieWB = 0;					
		//MessageBox( hwnd, "Gestore eventi WB disattivato", "Eventi WB", 0);
	}		
}
*/

//
// _browserWindowProc()
//
LRESULT CALLBACK _browserWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPCREATESTRUCT pCreate;
//	printf("%d|",uMsg);
	switch (uMsg)
	{
		case WM_CREATE:

			// Embed the browser object into our host window. We need do this only
			// once. Note that the browser object will start calling some of our
			// IOleInPlaceFrame and IOleClientSite functions as soon as we start
			// calling browser object functions in EmbedBrowserObjectBuilder().
			pCreate=(LPCREATESTRUCT) lParam;
			if (EmbedBrowserObjectBuilder(hwnd,(void *) pCreate->lpCreateParams)) return(-1); // Creo l'oggetto
			// EventBuildConnector( hwnd ); // Aggancio il gestore degli eventi
			ConnectWebEvents(hwnd);

			// Another window created with an embedded browser object
			//++WindowCount;

			// Success
			return(0);

		case WM_DESTROY:
			
			// Roby
			//if ( dwGlobalCookie!=0 )	ProvaEventi_PerElementi( hwnd );
			//if ( dwGlobalCookieWB!=0 )	ProvaEventi_PerWebBrowser( hwnd );

			// Detach the browser object from this window, and free resources.
			UnEmbedBrowserObject(hwnd);
			DestroyWebEvents( hwnd, 1);
			// One less window
			//--WindowCount;

			// If all the windows are now closed, quit this app
			//if (!WindowCount) PostQuitMessage(0);
			return(TRUE);

		case WM_SIZE:
			// Resize the browser object to fit the window
	//		printf("resize" CRLF);
			_browserResize(hwnd, LOWORD(lParam), HIWORD(lParam));
			return(0);
/*
		case WM_SHOWWINDOW:
		case WM_PAINT:
			_browserShow(hwnd);
//			printf("show" CRLF);
//		 	return(0);
		
*/


	}

	return (DefWindowProc(hwnd, uMsg, wParam, lParam));
}

//
//  _getElementInterface() - Ritorna il Interface IDispatch di un item
//
static IDispatch * _getElementInterface(EH_WEBPAGE * psWebPage,CHAR *pszInputName,BSTR * pbStrIdName) {

	BSTR bsName;
	HRESULT hr;
	IDispatch * piItemDispatch=NULL;
	IHTMLElementCollection * piCollection=NULL;
	IHTMLElement * piTag=NULL;
	
	_StrToBstr(psWebPage, pszInputName, &bsName);
	
	hr=psWebPage->sIe.piHtmlDoc3->lpVtbl->getElementById(psWebPage->sIe.piHtmlDoc3,bsName,&piTag);
	if ( SUCCEEDED(hr) && piTag ) {
		
		piItemDispatch=(IDispatch *) piTag;
		piTag->lpVtbl->get_tagName(piTag,pbStrIdName); 
		/*
		hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLElement, (void**) &piTag );
		if ((SUCCEEDED(hr)) && (piTag!=NULL) ) {
			piTag->lpVtbl->get_tagName(piTag,pbStrIdName); 
			_$Release(piTag);

		}
		*/
		
	}
	else {

		hr=psWebPage->sIe.piHtmlDoc3->lpVtbl->getElementsByName(psWebPage->sIe.piHtmlDoc3,bsName,&piCollection);
		*pbStrIdName=NULL;
		//
		// Recupero il singolo Tag all'indice "varIndex" da pElemFields -> pTagDisp
		//
		if ( (SUCCEEDED(hr)) && piCollection ) 
		{
			// Leggo il dispatch del primo elemento della collezione di elementi
			VARIANT varIndex;
			VariantInit(&varIndex);
			varIndex.vt = VT_I4;
			varIndex.lVal = 0;
			hr = piCollection->lpVtbl->item( piCollection, varIndex, varIndex, &piItemDispatch); if (hr) ehError();
			VariantClear(&varIndex);

			if (piItemDispatch) {
				hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLElement, (void**) &piTag );
				if ((SUCCEEDED(hr)) && (piTag!=NULL) ) {
					piTag->lpVtbl->get_tagName(piTag,pbStrIdName); 
					_$Release(piTag);
				}	
			} else  // Non ho trovato l'elemento
			{

				// printf("qui"); N
			}
		}
		_$Release(piCollection);
	}
	
	SysFreeString(bsName);
	// Se non ho trovato il nome dela TAG (Es SELECT/INPUT) azzero l'interfaccia
	if (!*pbStrIdName) {
		_$Release(piItemDispatch);
		piItemDispatch=NULL;
	}
	return piItemDispatch;

}

//
// _JavaLikeSetValue() 
//	- Controllo se abbiamo trovato il nome del "input"
//	- Setta un valore di un campo di input (a seconda del campo SELECT/CHECKBOX/INPUT/TEXTAREA
//
static BOOL	_JavaLikeSetValue(EH_WEBPAGE * psWebPage,BYTE * pszInputName,void * pszInputValue) {

	IDispatch * piItemDispatch;
	BSTR	bsName,bsValue;

	HRESULT	hr;
	BOOL	bFound=FALSE;
	
	//
	// Cerco il nome
	//
	piItemDispatch=_getElementInterface(psWebPage,pszInputName,&bsName);
	if (piItemDispatch) 
	{
		_StrToBstr(psWebPage, pszInputValue, &bsValue);
		bFound=TRUE;

		//
		// SELECT -> Se l'elemento è di tipo INPUT posso recuperare l'interfaccia più appropriata
		//
		if (!wcsCaseCmp(bsName,L"SELECT"))
		{
			IHTMLSelectElement * piSelect= NULL;
			hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLSelectElement, (void**) &piSelect);
			if ( SUCCEEDED(hr) && piSelect )
			{
				bFound=TRUE;
				piSelect->lpVtbl->put_value(piSelect,bsValue); 
				//piSelect->lpVtbl->Release(piSelect);
			}
			_$Release(piSelect);
		}
		//
		// INPUT > Se l'elemento è di tipo INPUT posso recuperare l'interfaccia più appropriata
		//
		else if (!wcsCaseCmp(bsName,L"INPUT"))
		{
			IHTMLInputElement* piInput = NULL;
			short fChecked;
			hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLInputElement, (void**) &piInput );
			if ( SUCCEEDED(hr) && piInput )
			{
				BSTR bsType;
				piInput->lpVtbl->get_type(piInput,&bsType); 
				if (bsType) {
					
					if (!wcsCaseCmp(bsType,L"checkbox")||
						!wcsCaseCmp(bsType,L"radio"))
					{
						fChecked=TRUE;
						piInput->lpVtbl->put_checked(piInput,fChecked); 
					}

					if (!wcsCaseCmp(bsType,L"text")||
						!wcsCaseCmp(bsType,L"hidden"))
					{
						piInput->lpVtbl->put_value(piInput,bsValue); 
					}
					SysFreeString(bsType);
				}
				_$Release(piInput);//piInput->lpVtbl->Release(piInput);
			}
		}
		
		//
		// TEXTAREA > Se l'elemento è di tipo INPUT posso recuperare l'interfaccia più appropriata
		//
		else if (!wcsCaseCmp(bsName,L"TEXTAREA"))
		{
			IHTMLTextAreaElement * piTextArea= NULL;
			hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLTextAreaElement, (void**) &piTextArea);
			if ( SUCCEEDED(hr) && piTextArea ) {
				piTextArea->lpVtbl->put_value(piTextArea,bsValue); 
			}
			_$Release(piTextArea);
		}

		// Rilascio risorse assegnazione
		_$Release(piItemDispatch);
		SysFreeString(bsValue);
	}
	return bFound;
}



static BYTE * _HtmlBuilderAlloc(BYTE *lpSource)
{
	CHAR szServ[200];
	BYTE *lpAlloc=ehAlloc(strlen(lpSource)*2);
	strcpy(lpAlloc,lpSource);
	sprintf(szServ,"#%6x",sys.ColorBackGround);
	strReplace(lpAlloc,"@#COLOR.BACKGROUND#@",szServ);
	return lpAlloc;
}

//
// bwsSetUrl()
//
static LRESULT _bwsSetUrlControl(S_BWS_NOTIFY * psBws) {

	EH_WEBPAGE * psWebPage=psBws->psWebPage;
// 	printf("> %d" CRLF,psBws->dispIdMember);
	switch (psBws->dispIdMember) 
	{
		//int count = webBrowser2.Document.Cookie.Length;
		//webBrowser2.Document.Cookie.Remove(0,count);
		case DISPID_DOWNLOADBEGIN:
			break;

		// Fires after a navigation to a link is completed on a window element or a frameSet element.
		// Inviato quando la navigazione è completa
		case DISPID_NAVIGATECOMPLETE2:
			psWebPage->iStatusLoading|=4; // Completato
			break;

		case DISPID_DOCUMENTCOMPLETE: // <--- Fires when a document is completely loaded and initialized.

			psWebPage->iStatusLoading|=2; // Completato
			if (psBws->pDispParams->rgvarg) {
				if (psBws->pDispParams->rgvarg[0].pvarVal) {
					
					if (psBws->pDispParams->rgvarg[0].pvarVal->bstrVal) {

						CHAR * psz;
						psz=_BstrToStr(psWebPage, psBws->pDispParams->rgvarg[0].pvarVal->bstrVal);
						printf("docCompleted:%s" CRLF,psz);
					//	strToClipboard("%s",psz);
						ehFree(psz);

						psWebPage->fPageReady=true;
					}
				}
			}
			break; 

		case DISPID_NAVIGATEERROR:
				psWebPage->iStatusLoading|=8; // Completato
				break;
	}
/*
case DISPID_DOCUMENTCOMPLETE: // <--- Fires when a document is completely loaded and initialized.
		//iWPIndex=WPFind(WP_FINDHWND,lpD->hWnd);
		
		psWebPage->iStatusLoading|=2; // Completato
		psWebPage->fPageReady=TRUE;
		if (pDispParams->rgvarg) {
			if (pDispParams->rgvarg[0].pvarVal) {
				if (pDispParams->rgvarg[0].pvarVal->bstrVal) {
					psz=_BstrToStr(psWebPage, pDispParams->rgvarg[0].pvarVal->bstrVal);
					sprintf(szServ,"docCompleted|%s",psz);
					ehFree(psz);
					_notify(psWebPage,WS_PROCESS,szServ);
				}
			}
		}
		break; 

		// Fires after a navigation to a link is completed on a window element or a frameSet element.
		// Inviato quando la navigazione è completa
	case DISPID_NAVIGATECOMPLETE2:
		psWebPage->iStatusLoading|=4; // Completato
		psz=_BstrToStr(psWebPage, pDispParams->rgvarg[0].pvarVal->bstrVal);
		sprintf(szServ,"navCompleted|%s",psz);
		ehFree(psz);
		_notify(psWebPage*/

/*	case DISPID_CLIENTTOHOSTWINDOW:
		ClientToHostWindow(pDispParams->rgvarg[1].plVal, pDispParams->rgvarg[0].plVal);
		break;
	case DISPID_COMMANDSTATECHANGE:
		MessageBox( 0, "DISPID_COMMANDSTATECHANGE", "Evento WB", 0);
		break;
		CommandStateChange(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_FILEDOWNLOAD:
		FileDownload(pDispParams->rgvarg[1].pboolVal, pDispParams->rgvarg[0].pboolVal);
		break;
	case DISPID_NAVIGATEERROR:
		NavigateError(pDispParams->rgvarg[4].pdispVal, pDispParams->rgvarg[3].pvarVal, pDispParams->rgvarg[2].pvarVal, pDispParams->rgvarg[1].pvarVal, pDispParams->rgvarg[0].pboolVal);
		break;
	case DISPID_NEWWINDOW2:
		NewWindow2(pDispParams->rgvarg[1].ppdispVal, pDispParams->rgvarg[0].pboolVal);
		break;
	case DISPID_ONFULLSCREEN:
		OnFullScreen(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_ONMENUBAR:
		OnMenuBar(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_ONQUIT:
		OnQuit();
		break;
	case DISPID_ONVISIBLE:
		OnVisible(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_PROGRESSCHANGE:
		ProgressChange(pDispParams->rgvarg[1].lVal, pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_PROPERTYCHANGE:
		PropertyChange(pDispParams->rgvarg[0].bstrVal);
		break;
	case DISPID_SETSECURELOCKICON:
		SetSecureLockIcon(pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_STATUSTEXTCHANGE:
		StatusTextChange(pDispParams->rgvarg[0].bstrVal);
		break;
	case DISPID_TITLECHANGE:
		TitleChange(pDispParams->rgvarg[0].bstrVal);
		break;
	case DISPID_WINDOWCLOSING:
		WindowClosing(pDispParams->rgvarg[1].boolVal, pDispParams->rgvarg[0].pboolVal);
		break;
	case DISPID_WINDOWSETHEIGHT:
		WindowSetHeight(pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_WINDOWSETLEFT:
		WindowSetLeft(pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_WINDOWSETRESIZABLE:
		WindowSetResizable(pDispParams->rgvarg[0].boolVal);
		break;
	case DISPID_WINDOWSETTOP:
		WindowSetTop(pDispParams->rgvarg[0].lVal);
		break;
	case DISPID_WINDOWSETWIDTH:
		WindowSetWidth(pDispParams->rgvarg[0].lVal);
		break;
*/

	return S_OK;

}

//
// bwsSetText()
//
/*
BOOL _bwsSetText(CHAR * pszObj,BYTE * pszText,BOOL bUtf8,DWORD dwTimeout) {

	CHAR szFile[500];
	INT ret;
	BOOL			bDone=false;
	//ret=fileTempName(NULL, "html-",szFile,true);
	//if (ret) {
	
		strcpy(szFile,progenesiTemp("htmlPreview.htm"));
		fileStrWrite(szFile,pszText);

		bwsSetUrl(pszObj,szFile,false,2000);
		fileRemove(szFile);

	//}
	// printf("qui");
	return bDone;

}
*/

BOOL bwsSetText(CHAR * pszObj,BYTE * pszText,BOOL bUtf8,DWORD dwTimeout) { // pause

	EH_OBJ *		psObj;
	EH_WEBPAGE *	psWebPage=NULL;
	BOOL			bDone=false;

	SAFEARRAY	*	sfArray;
	VARIANT		*	pVar;
	
 	psObj=obj_GetInfo(pszObj);  if (psObj) 	psWebPage=psObj->pOther; if (!psWebPage) ehError();
	if (dwTimeout) psWebPage->funcNotify=_bwsSetUrlControl;

	psWebPage->iStatusLoading=0; // Resetto lo status della pagina da caricare
	psWebPage->fPageReady=false;

	if (psWebPage->sIe.piWebBrowser2) {

		if ((sfArray = SafeArrayCreate(VT_VARIANT, 1, (SAFEARRAYBOUND *) &ArrayBound))) // Creo l'array di strutture VARIANT
		{
			if (!SafeArrayAccessData(sfArray, (void**) &pVar)) // Chiedo l'accesso all'array e gli passo pVar
			{
				WCHAR *pw;
				pVar->vt = VT_BSTR;

				if (bUtf8)  pw=utfToWcs((CHAR *) pszText);  else pw=strToWcs((CHAR *) pszText); 
				pVar->bstrVal=SysAllocString(pw);//buffer);
				ehFree(pw);

				if (pVar->bstrVal)
				{
					IDispatch* pElemDisp = NULL;
					VARIANT vnull;
					_IEI *			psIe=&psWebPage->sIe;
					vnull.vt=VT_ERROR;
					vnull.scode=DISP_E_PARAMNOTFOUND; 

					// Passo il VARIENT con il suot BSTR a write() in ordine per mostrare
					// l'HTML string nel corpo della pagina vuota da noi creata sopra
					if (psIe->piHtmlDoc2) {
						psIe->piHtmlDoc2->lpVtbl->open(psIe->piHtmlDoc2, L"html/txt", vnull, vnull, vnull, &pElemDisp); // new 2009
						psIe->piHtmlDoc2->lpVtbl->write(psIe->piHtmlDoc2, sfArray);
						psIe->piHtmlDoc2->lpVtbl->close(psIe->piHtmlDoc2); // new 2009
				//		bDone=true;
					}
				}
			}
			SafeArrayDestroy(sfArray);
			SysFreeString(pVar->bstrVal);

			//
			// Controllo che il documento sia pronto
			//
			if (dwTimeout) {
				DWORD dwRif,msElapsed;
				_IEI *	psIe=&psWebPage->sIe;
				chronoStart(&dwRif);
				while (true) {

					BSTR bsState;
					msElapsed=(DWORD) ((double) chronoGet(&dwRif)/CLOCKS_PER_SEC*1000);

					psIe->piHtmlDoc2->lpVtbl->get_readyState(psIe->piHtmlDoc2, &bsState);
					printf("> %d : %S" CRLF,msElapsed,bsState);
					if (!wcsCmp(bsState,L"complete")) {
						bDone=true;
						break;
					}

					if (msElapsed>dwTimeout) break;
					PauseActive(300);
				}
			}

/*
			if (dwTimeout) {
				chronoStart(&dwRif);
				while (true) {

					msElapsed=(DWORD) ((double) chronoGet(&dwRif)/CLOCKS_PER_SEC*1000);
					printf("> %d" CRLF,msElapsed);
					if (psWebPage->fPageReady) {
						
						psWebPage->msTimeReady=msElapsed;
						bDone=true;
						break;
					}

					if (msElapsed>dwTimeout) break;
					PauseActive(100);
				}
			}
*/

		}
	}
	return bDone;
}


//
// bwsSetTimeout()
//
BOOL	bwsSetTimeout(CHAR * pszObj,DWORD dwTimeoutField) {

	EH_WEBPAGE * psWebPage;
	psWebPage=_getWebPage(pszObj); if (!psWebPage) return true;
	psWebPage->msTimeoutField=dwTimeoutField;
	return false;
}

//
// bwsSetUrl()
//
BOOL bwsSetUrl(CHAR * pszObj,CHAR * pszUrl,BOOL bSilent,DWORD dwTimeout) {
	
	EH_OBJ *	psObj;
	EH_WEBPAGE *psWebPage=NULL;
	BOOL		bDone=false;
	VARIANT		vUrl;
	WCHAR *		pwcUrl;
	
 	psObj=obj_GetInfo(pszObj);  if (psObj) 	psWebPage=psObj->pOther; if (!psWebPage) ehError();
	if (dwTimeout) psWebPage->funcNotify=_bwsSetUrlControl;

	psWebPage->iStatusLoading=0; // Resetto lo status della pagina da caricare
	psWebPage->fPageReady=false;

	if (psWebPage->sIe.piWebBrowser2) {

		DWORD dwRif;
		DWORD msElapsed;

		chronoStart(&dwRif);
//		psWebPage->sIe.piWebBrowser2->lpVtbl->put_Silent(psWebPage->sIe.piWebBrowser2, (VARIANT_BOOL) true); // MOdalità silenziona
		psWebPage->sIe.piWebBrowser2->lpVtbl->put_Silent(psWebPage->sIe.piWebBrowser2, bSilent?VARIANT_TRUE:VARIANT_FALSE); // MOdalità silenziona
	
		VariantInit(&vUrl);
		vUrl.vt = VT_BSTR;
		pwcUrl=strToWcs(pszUrl);
		vUrl.bstrVal = SysAllocString(pwcUrl);
		ehFree(pwcUrl);
		psWebPage->sIe.piWebBrowser2->lpVtbl->Navigate2(psWebPage->sIe.piWebBrowser2, &vUrl, 0, 0, 0, 0);
	//	VariantClear(&vUrl);

		if (dwTimeout) {
			while (true) {

				msElapsed=(DWORD) ((double) chronoGet(&dwRif)/CLOCKS_PER_SEC*1000);
				if (psWebPage->fPageReady) {
					
					psWebPage->msTimeReady=msElapsed;
					bDone=true;
					break;
				}

				if (msElapsed>dwTimeout) break;
				PauseActive(500);
			}
		}

		VariantClear(&vUrl);


	}

	return bDone;
}

//
// bwsVisible()
//
BOOL	bwsVisible(CHAR * pszObj,BOOL bShow) {

	_IEI * psIe;
	EH_OBJ *	psObj;
	EH_WEBPAGE *psWebPage=NULL;
	BOOL		bDone=false;
//	VARIANT		vUrl;
//	WCHAR *		pwcUrl;
	
 	psObj=obj_GetInfo(pszObj); if (psObj) 	psWebPage=psObj->pOther; 
	psIe=&psWebPage->sIe;

	if (bShow) {
		//psIe->piWebBrowser2->lpVtbl->put_Visible(psIe->piWebBrowser2,VARIANT_TRUE);		
			psIe->piWebBrowser2->lpVtbl->put_Width(psIe->piWebBrowser2, psWebPage->sizWin.cx);
			psIe->piWebBrowser2->lpVtbl->put_Height(psIe->piWebBrowser2, psWebPage->sizWin.cy);

	}
		else	
		//psIe->piWebBrowser2->lpVtbl->put_Visible(psIe->piWebBrowser2,VARIANT_FALSE);		
		{
	
			psIe->piWebBrowser2->lpVtbl->put_Width(psIe->piWebBrowser2, 0);
			psIe->piWebBrowser2->lpVtbl->put_Height(psIe->piWebBrowser2, 0);

		}	

	return false;
}

//
// bwsWaitUrl()
//
BOOL	bwsWaitUrl(CHAR * pszObj,CHAR * pszWildCharUrl,CHAR * pszUrl,INT iUrlSize,DWORD msWaiting) {

	EH_OBJ * psObj;
	EH_WEBPAGE * psWebPage=NULL;
	HRESULT hr;
	BOOL	bReady=false;

 	psObj=obj_GetInfo(pszObj);  if (psObj) 	psWebPage=psObj->pOther; if (!psWebPage) ehExit("bwsWaitUrl ([%s]) ?",pszObj);

	__try
	{      

		// Cerco Interfacce
		if (!_ieCreate(psWebPage,FALSE))//,psWebPage->hWnd,FALSE))//&psWebPage->browserObject, &webBrowser2, &lpDispatch, &htmlDoc2))
		{
			IDispatch* pElemDisp = NULL;
			IHTMLElement* pElem = NULL; 
			LPDISPATCH spItem=NULL;
			BSTR	bsName;
			DWORD	dwRif;
			IDispatch * piItemDispatch=NULL;
			IHTMLElement * piElement=NULL;
			DWORD	msElapsed;
			CHAR * psz;
			
			chronoStart(&dwRif);
			while (true) {

				//
				// Leggo la pagina corrente
				//
				hr=psWebPage->sIe.piHtmlDoc2->lpVtbl->get_URL(psWebPage->sIe.piHtmlDoc2,&bsName);
				if (SUCCEEDED(hr)) {
					psz=wcsToStr((WCHAR *) bsName);
					if (strWildMatch(pszWildCharUrl,psz)) {
						if (pszUrl) strCpy(pszUrl,psz,iUrlSize);
						bReady=true;
					}
					ehFree(psz);
				//	if (strWild
				}	

				/*

				_StrToBstr(psWebPage, pszNameId, &bsName);
				SysFreeString(bsName);
				*/
				if (!msWaiting||bReady) break;
				msElapsed=(DWORD) ((double) chronoGet(&dwRif)/CLOCKS_PER_SEC*1000);
				if (msElapsed>msWaiting) break;
				PauseActive(300);

			}

		}
	}__except (ehLogWrite("Errorex"), 1) {
			ehLogWrite("bwsElementReady %s.%s",pszObj,pszWildCharUrl);   
	}  

	return bReady;


}


//
// bwsElementReady()
//
BOOL bwsElementReady(CHAR * pszObj,CHAR * pszSearch, BOOL bQuery,DWORD msReply, IHTMLElement **  ppiElement) {
	
	HRESULT hr;
	BOOL	bReady=false;
	EH_WEBPAGE *psWebPage=_getWebPage(pszObj);

	__try
	{      

		// Cerco Interfacce
		if (!_ieCreate(psWebPage,FALSE))//,psWebPage->hWnd,FALSE))//&psWebPage->browserObject, &webBrowser2, &lpDispatch, &htmlDoc2))
		{
			IDispatch* pElemDisp = NULL;
			IHTMLElement* pElem = NULL; 
			LPDISPATCH spItem=NULL;
			BSTR	bsName;
			DWORD	dwRif;
			IDispatch * piItemDispatch=NULL;
			IHTMLElement * piElement=NULL;
			DWORD	msElapsed;
			EH_LST	lstQry=NULL;
			
			chronoStart(&dwRif);
			while (true) {

				//
				// Leggo l'interfaccia dell'elemento
				//
				if (bQuery) {
					lstQry=bwsQuery(pszObj,NULL,pszSearch);
					if (lstQry) {piElement=lstFirst(lstQry); bReady=true; lstDestroy(lstQry);}
					
				} else {
				
					_StrToBstr(psWebPage, pszSearch, &bsName);
					hr=psWebPage->sIe.piHtmlDoc3->lpVtbl->getElementById(psWebPage->sIe.piHtmlDoc3,bsName,&piElement);
					if ( SUCCEEDED(hr) && piElement ) {
						bReady=true;
					}	
					SysFreeString(bsName);
				}
				

				if (bReady) {
					if (ppiElement) *ppiElement=piElement; else _$Release(piElement);
				}

				if (!msReply||bReady) break;
				msElapsed=(DWORD) ((double) chronoGet(&dwRif)/CLOCKS_PER_SEC*1000);
				if (msElapsed>msReply) break;
				PauseActive(500);

			}

		}
	}__except (ehLogWrite("Errorex"), 1) {
			ehLogWrite("bwsElementReady %s.%s",pszObj,pszSearch);   
	}  

	return bReady;
}
//
// bwsSilent()
//
BOOL bwsSilent(CHAR * pszObj,BOOL bSilent) {

	_IEI * psIe=_getIE(pszObj);
	psIe->piWebBrowser2->lpVtbl->put_Silent(psIe->piWebBrowser2, (VARIANT_BOOL) bSilent);
	return false;
}
//
// bwsJavascript()
//
BOOL bwsJavascript(CHAR * pszObj,CHAR * pszFormat,...) {

	BOOL	bDone=false;
	EH_WEBPAGE * psWebPage=_getWebPage(pszObj);
	HRESULT hr;
	CHAR * pszScript;

	strFromArgs(pszFormat,pszScript);

	__try
	{      

		// Cerco Interfacce
		if (!_ieCreate(psWebPage,FALSE))//,psWebPage->hWnd,FALSE))//&psWebPage->browserObject, &webBrowser2, &lpDispatch, &htmlDoc2))
		{
			BSTR bsScript;
			BSTR bsLanguage;
			VARIANT		 var;
			IHTMLWindow2   *	piHtmlWin2;
			_StrToBstr(psWebPage, pszScript, &bsScript);
			_StrToBstr(psWebPage, "javascript", &bsLanguage);
			psWebPage->sIe.piHtmlDoc2->lpVtbl->get_parentWindow(psWebPage->sIe.piHtmlDoc2,&piHtmlWin2);
 			hr=piHtmlWin2->lpVtbl->execScript(piHtmlWin2,bsScript,bsLanguage,&var);
			if (SUCCEEDED(hr)) {
				bDone=true;
			} 
			SysFreeString(bsScript);
			SysFreeString(bsLanguage);


		}

	}__except (ehLogWrite("Errorex"), 1) {
			ehLogWrite("bwsJavascript %s.%s",pszObj,pszScript);   
	}  

	ehFree(pszScript);
	return bDone;
//	HTMLWindow2 iHtmlWindow2 = (HTMLWindow2) doc.Script ;
//	iHtmlWindow2.execScript( "functionName(param1);" , "javascript" );
								// 
}

//
// bwsJavascriptEx()
//
BOOL bwsJavascriptEx(CHAR * pszObj,DWORD msReply,CHAR * pszFormat,...) {

	DWORD	dwRif,msElapsed;
	CHAR *	pszScript;
	BOOL	bDone=false;
	strFromArgs(pszFormat,pszScript);

	chronoStart(&dwRif);
	while (true) {

		bDone=bwsJavascript(pszObj,"%s",pszScript);

		if (!msReply||bDone) break;
		msElapsed=(DWORD) ((double) chronoGet(&dwRif)/CLOCKS_PER_SEC*1000);
		if (msElapsed>msReply) break;
		PauseActive(300);

	}
	return bDone;

}

//
// bwsSetValue()
//
BOOL bwsSetValue(CHAR * pszObj,CHAR * pszNameId,BOOL bUtf8,CHAR * pszFormat,...) {
	
	BOOL bDone=false;
	IHTMLElement *  piElement;
	EH_WEBPAGE *	psWebPage=_getWebPage(pszObj);
	HRESULT hr;
	CHAR * pszValue;
	strFromArgs(pszFormat,pszValue);

	if (bwsElementReady(pszObj,pszNameId, false,psWebPage->msTimeoutField, &piElement)) {

			VARIANT		 var;
			BSTR		bsValue;
			VariantInit(&var);
			var.vt = VT_BSTR;
			if (pszValue) 
			{
				if (bUtf8) 
				{
					WCHAR * pwc=strDecode(pszValue,SD_UTF8,NULL);
					var.bstrVal= SysAllocString(pwc); 
					ehFreeNN(pwc);

				} else {
					_StrToBstr(psWebPage, pszValue, &var.bstrVal);
				}
			}
			
			_StrToBstr(psWebPage, "value", &bsValue);
			hr=piElement->lpVtbl->setAttribute(piElement,bsValue,var,false);
			if ( !SUCCEEDED(hr) ) {
				
				printf("qui");
			}

			SysFreeString(bsValue);
			_$Release(piElement);
			bDone=true;
	
	} else {

		printf("bwsSetValue(%s) error" CRLF,pszNameId);

	}
	ehFree(pszValue);
	return bDone;
}

//
// bwsSetScroll()
//
BOOL bwsSetScroll(CHAR * pszObj,INT iValue) {

	EH_WEBPAGE * psWebPage=_getWebPage(pszObj);
	_IEI *	psIe=&psWebPage->sIe;
	IHTMLWindow2   *	piHtmlWin2;

	if (!psWebPage) return true;

	psIe->piHtmlDoc2->lpVtbl->get_parentWindow(psIe->piHtmlDoc2,&piHtmlWin2);
	piHtmlWin2->lpVtbl->scroll(piHtmlWin2,iValue,0);

	return false;


}


/*
//
// _JavaLikeGetValue()
//
static CHAR * _JavaLikeGetValue(EH_WEBPAGE *psWebPage,INT iMode,CHAR *pszFormInputName)
{
	HRESULT hr;
	INT	hdlForm=-1;
	CHAR *	pszInputName;
	CHAR *	pszReturn=NULL;
	BOOL fFound=FALSE;
	CHAR szForm[200];
	IDispatch * piItemDispatch=NULL;
	BSTR	bsName,bstrReturn=NULL;
	CHAR * lpType=NULL;

	strcpy(szForm,pszFormInputName);
	pszInputName=strstr(szForm,"."); //if (!pTagFind) ehExit("_JavaLikeGetValue: [%s] indicare il campo da cercare",pTagFind);
	if (!pszInputName) pszInputName=pszInputName; else {*pszInputName=0; pszInputName++;}

	piItemDispatch=_getElementInterface(psWebPage,pszInputName,&bsName);
	if (piItemDispatch) 
	{
		fFound=TRUE;
		//
		// INPUT -> 
		//
		if (!wcsCaseCmp(bsName,L"INPUT")) {

			IHTMLInputElement* piInput = NULL;
			short fChecked;
			hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLInputElement, (void**) &piInput );
			if ( (SUCCEEDED(hr)) && piInput ){

				BSTR bstrType;
				piInput->lpVtbl->get_type(piInput,&bstrType); 
				if (bstrType) {
				
					if (!wcsCaseCmp(bstrType,L"checkbox")||!wcsCaseCmp(bstrType,L"radio")) {
						fChecked=FALSE; piInput->lpVtbl->get_checked(piInput,&fChecked); 
						if (fChecked) piInput->lpVtbl->get_value(piInput,&bstrReturn);
					}
					else if (!wcsCaseCmp(bstrType,L"text")||!wcsCaseCmp(bstrType,L"hidden")) 
						{piInput->lpVtbl->get_value(piInput,&bstrReturn);}
				}
				SysFreeString(bstrType);
				_$Release(piInput);
			}
		}
		//
		// TEXTAREA
		//
		else if (!wcsCaseCmp(bsName,L"TEXTAREA")) {

			IHTMLTextAreaElement * piTextArea= NULL;
			hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLTextAreaElement, (void**) &piTextArea);
			if ( SUCCEEDED(hr) && piTextArea ) {

				piTextArea->lpVtbl->get_value(piTextArea,&bstrReturn); 
				_$Release(piTextArea);

			}
		}
		//
		// SELECT
		//
		else if (!wcsCaseCmp(bsName,L"SELECT")) {
		
			IHTMLSelectElement * piSelect= NULL;
			hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLSelectElement, (void**)&piSelect);
			if ( SUCCEEDED(hr) && piSelect ) {
				piSelect->lpVtbl->get_value(piSelect,&bstrReturn);
				_$Release(piSelect);
			}
		}

		SysFreeString(bsName);
		_$Release(piItemDispatch);
		
		if (bstrReturn) {
			pszReturn=_BstrToStr(psWebPage,bstrReturn);
			SysFreeString(bstrReturn);
		}

	}	
	if (iMode&&!fFound) ehExit("_JavaLikeGetValue():Non trovato %s",szForm,pszFormInputName);
	if (fFound&&!pszReturn)  {pszReturn=ehAlloc(2); *pszReturn=0;}
	return pszReturn; // Va liberato
}
*/
//
// bwsGetValue()
//
CHAR * bwsGetValue(CHAR * pszObj,CHAR * pszFormat, ...) {
	
	BOOL bDone=false;
	IHTMLElement *  piElement;
	EH_WEBPAGE *psWebPage=_getWebPage(pszObj);
	HRESULT hr;
	CHAR * pszValue=NULL;
	CHAR * pszNameId;
	strFromArgs(pszFormat,pszNameId);
	if (bwsElementReady(pszObj,pszNameId, false,psWebPage->msTimeoutField, &piElement)) {

			BSTR	bsName,bstrReturn=NULL;
			IDispatch * piItemDispatch;

			_$Release(piElement);
			piItemDispatch=_getElementInterface(psWebPage,pszNameId,&bsName);
			if (!piItemDispatch) ehError();

			//
			// INPUT -> 
			//
			if (!wcsCaseCmp(bsName,L"INPUT")) {

				IHTMLInputElement* piInput = NULL;
				short fChecked;
				hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLInputElement, (void**) &piInput );
				if ( (SUCCEEDED(hr)) && piInput ){

					BSTR bstrType;
					piInput->lpVtbl->get_type(piInput,&bstrType); 
					if (bstrType) {
					
						if (!wcsCaseCmp(bstrType,L"checkbox")||!wcsCaseCmp(bstrType,L"radio")) {
							fChecked=FALSE; piInput->lpVtbl->get_checked(piInput,&fChecked); 
							if (fChecked) piInput->lpVtbl->get_value(piInput,&bstrReturn);
						}
						else if (!wcsCaseCmp(bstrType,L"text")||!wcsCaseCmp(bstrType,L"hidden")) 
						{
							piInput->lpVtbl->get_value(piInput,&bstrReturn);

						}
					}
					SysFreeString(bstrType);
					_$Release(piInput);
				}
			}
			//
			// TEXTAREA
			//
			else if (!wcsCaseCmp(bsName,L"TEXTAREA")) {

				IHTMLTextAreaElement * piTextArea= NULL;
				hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLTextAreaElement, (void**) &piTextArea);
				if ( SUCCEEDED(hr) && piTextArea ) {

					piTextArea->lpVtbl->get_value(piTextArea,&bstrReturn); 
					_$Release(piTextArea);

				}
			}
			//
			// SELECT
			//
			else if (!wcsCaseCmp(bsName,L"SELECT")) {
			
				IHTMLSelectElement * piSelect= NULL;
				hr = piItemDispatch->lpVtbl->QueryInterface(piItemDispatch, &IID_IHTMLSelectElement, (void**)&piSelect);
				if ( SUCCEEDED(hr) && piSelect ) {
					piSelect->lpVtbl->get_value(piSelect,&bstrReturn);
					_$Release(piSelect);
				}
			}

			SysFreeString(bsName);
			_$Release(piItemDispatch);
			
			
			if (bstrReturn) {
				pszValue=_BstrToStr(psWebPage,bstrReturn);
				SysFreeString(bstrReturn);
			} else pszValue=_BstrToStr(psWebPage,L"");


			_$Release(piElement);
	
	} else {
		printf("bwsGetValue(%s) error ?",pszNameId);
	}
	ehFree(pszNameId);
	return pszValue;
}

//
// bwsEvent()
//
void bwsEvent(CHAR * pszObj,IHTMLElement * piElement,CHAR * pszEvent) {

	if (!strCmp(pszEvent,"click")) piElement->lpVtbl->click(piElement);

}
//
// bwsEventId()
//
BOOL bwsEventStr(CHAR * pszObj,CHAR * pszStr,BOOL bQuery,CHAR * pszEvent,DWORD msWaitBefore,DWORD msReply) {

	BOOL	bOk=false;
	IHTMLElement * piElement=NULL;

	if (bwsElementReady(pszObj,pszStr,bQuery,msReply,&piElement)) {
		bwsEvent(pszObj,piElement,pszEvent);
		bOk=true;
	}
	return bOk;

}



// Dalla finestra recupero l'oggetto
static EH_WEBPAGE * _WebPageSearch(HWND hWnd)
{
	return (EH_WEBPAGE *) GetWindowLong(hWnd,GWL_USERDATA);
}

typedef struct {

	EH_WEBPAGE *	psWebPage;
	EH_LST			lstEle;
	EH_AR			arStep;
	INT				iStep;
	INT				iStepLen;

} SE_SCAN;

static void _eleFind(SE_SCAN * psScan,IHTMLElement * piEle) {

	HRESULT hr;
	IDispatch * pDispCh; 
	static INT iLevel=0;
	IHTMLElement * psRet=NULL;
	BOOL	bFound=false;

	BSTR bsName=NULL;
	BSTR bsTag=NULL;
	hr=piEle->lpVtbl->get_tagName(piEle,&bsName);
//	printf("%s%S" CRLF,strPad('.',iLevel),bsName); // Debug

	_StrToBstr(psScan->psWebPage, psScan->arStep[psScan->iStep], &bsTag);

	//
	// Ricerco la classe
	//
	if (*bsTag=='.') {

		BSTR bsClass=NULL;
		hr=piEle->lpVtbl->get_className(piEle,&bsClass);
		if (SUCCEEDED(hr)&&bsClass) {
				
			BSTR pw,pwName=wcsDup(bsClass),pwStart;
			WCHAR c;
			EH_LST lst=lstNew();
			pw=pwName;
			while (true) {
				
				for (pw;*pw<L'!'&&*pw;pw++); // Mi sposto avanti dove non ho spazi
				if (*pw) pwStart=pw; else break;

				for (pw;*pw>L' '&&*pw;pw++); // Cerco il prossimo spazio
				c=*pw; *pw=0;
				lstPushPtr(lst,pwStart);
				if (!c) break; else pw++;
			
			}

			for (lstLoop(lst,pw)) {
				if (!wcscmp(pw,bsTag+1)) 
				{
					bFound=true; 
					break;
				}
			}
			lstDestroy(lst);
			ehFree(pwName);
			
//			if (bFound) 
//				piRet=piEle;
//				printf("Analizzo se contengo la classe/classi");
		}
	}
	//
	// Ricerca ID
	//
	else if (*bsTag=='#') {

		BSTR bsClass=NULL;
		hr=piEle->lpVtbl->get_id(piEle,&bsClass);
		if (SUCCEEDED(hr)&&bsClass) {
			if (!wcscmp(bsClass,bsTag+1)) bFound=true;
		}
	}
	//
	// Ricerca Testo
	//
	else if (*bsTag=='$') {

			BSTR bsText;
			if (SUCCEEDED(piEle->lpVtbl->get_innerHTML(piEle,&bsText))) {
				if (bsText) {
					if (wcsstr(bsText,bsTag+1)) 
						bFound=true;
				}
			}
		
	
	}
	else {

		if (!wcsCaseCmp(bsName,bsTag)) bFound=true;

	}

	if (!bFound) {

		hr=piEle->lpVtbl->get_children(piEle,&pDispCh);
		if (SUCCEEDED(hr)) {
			IHTMLElementCollection * pChColl;

			// Loop sui figli
			hr = pDispCh->lpVtbl->QueryInterface(pDispCh,&IID_IHTMLElementCollection, &pChColl);
			if (SUCCEEDED(hr)) {
				LONG l,iLength;
				iLevel++;
				pChColl->lpVtbl->get_length(pChColl,&iLength);
				for (l=0;l<iLength;l++) {
				
					IDispatch * pDispEle; 
					VARIANT varIndex;
					VARIANT varName;
	//				TComPtr pDisp;
					
					VariantInit(&varIndex);
					varIndex.vt = VT_UINT;
					varIndex.lVal = (LONG) l;
					VariantInit(&varName);
					hr = pChColl->lpVtbl->item(pChColl,varIndex,varName,&pDispEle);
					if(SUCCEEDED(hr) && pDispEle)
					{
						IHTMLElement * piTag=NULL;
						hr = pDispEle->lpVtbl->QueryInterface(pDispEle, &IID_IHTMLElement, (void**) &piTag );
						if (SUCCEEDED(hr) && piTag) {
							_eleFind(psScan,piTag);
						} 
					}
				}
				iLevel--;
				_$Release(pChColl);	
			}	

			_$Release(pDispCh);
		}
		
	} else {

	//
	// Trovato ! Se è l'ultimo figlio aggiunto altrimenti step successivo
	// 
		if (psScan->iStep==(psScan->iStepLen-1)) {
		
			lstPushPtr(psScan->lstEle,piEle);
		//	printf("qui");

		} else {
		
			psScan->iStep++;
			_eleFind(psScan,piEle);
			psScan->iStep--;
		
		}
	}

	SysFreeString(bsTag);

}

//
// bwsQuery()
//
EH_LST bwsQuery(CHAR * pszObj,IHTMLElement * piTagStart,CHAR * pszFormat, ...) {

	CHAR * pszQuery;
	HRESULT hr;
	BOOL	bReady=false;
	EH_WEBPAGE *psWebPage=_getWebPage(pszObj);
	EH_LST lstEle=NULL;
	strFromArgs(pszFormat,pszQuery);

	__try
	{      
		// Cerco Interfacce
		if (!_ieCreate(psWebPage,FALSE))//,psWebPage->hWnd,FALSE))//&psWebPage->browserObject, &webBrowser2, &lpDispatch, &htmlDoc2))
		{
			SE_SCAN seScan;
			IHTMLElement * piTag=piTagStart;
			if (!piTag) 
			{
				hr=psWebPage->sIe.piHtmlDoc3->lpVtbl->get_documentElement(psWebPage->sIe.piHtmlDoc3,&piTag);
			}
			_(seScan);
			seScan.psWebPage=psWebPage;
			seScan.arStep=strSplit(pszQuery," ");
			seScan.iStepLen=ARLen(seScan.arStep);
			seScan.iStep=0;
			seScan.lstEle=lstNew();
			_eleFind(&seScan,piTag);
			
			if (piTagStart!=piTag) _$Release(piTag);
			ehFree(seScan.arStep);
			lstEle=seScan.lstEle;


		}
	}__except (ehLogWrite("Errorex"), 1) {
			ehLogWrite("bwsQuery %s.%s",pszObj,pszQuery);   
	}  

	ehFree(pszQuery);
	if (!lstEle->iLength) lstEle=lstDestroy(lstEle);
	return lstEle;

}

//
// bwsQueryDestroy()
//
EH_LST bwsQueryDestroy(EH_LST lst) {

	IHTMLElement * piTag;
	for (lstLoop(lst,piTag)) {
		_$Release(piTag);
	}
	return lstDestroy(lst);
}

//
// bwsGetParent
//
IHTMLElement *	bwsGetParent(CHAR * pwsWeb,IHTMLElement * piTag) {

	IHTMLElement * piRet=NULL;
	piTag->lpVtbl->get_parentElement(piTag,&piRet);
	return piRet;

}


//
// Cerco la finistra e carico un valore
//
BOOL CALLBACK _EnumChildProc(HWND hWnd, LPARAM lParam) {

	INPUT ip;
//	CHAR szText[2000];
	CHAR szClass[2000];
	GetClassName(hWnd,szClass,sizeof(szClass));
//	GetWindowText(hWnd,szText,sizeof(szText));
//	printf(".%s:%s" CRLF,szClass,szText);

	if (!strCmp(szClass,"Edit")) 
	{
		SendMessageW(hWnd,WM_SETTEXT,0,(LPARAM) lParam); // TESTING
		
		_(ip);
		SetFocus(hWnd);
		ip.type = INPUT_KEYBOARD;
		ip.ki.wScan = 0;
		ip.ki.time = 0;
		ip.ki.dwExtraInfo = 0;
		ip.ki.wVk = VK_RETURN;
		//For key press Flag=0
		ip.ki.dwFlags = 0;
		SendInput(1, &ip, sizeof(INPUT));

		//For key relese Flag = KEYEVENTF_KEYUP
		ip.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &ip, sizeof(INPUT));

	}
	return TRUE; // must return TRUE; If return is FALSE it stops the recursion
}


//
// bwsSetInputFile()
//
BOOL bwsSetInputFile(CHAR * pszObj,CHAR * pszFld,CHAR * pszPhoto) {

	BOOL bReady=false;

	EH_WEBPAGE * psWebPage=_getWebPage(pszObj);

	if (bwsEventStr(pszObj,pszFld,false,"click",100,5000)) {
	
		HWND hWnd=FindWindow(NULL,"Selezionare il file da caricare");
		if (hWnd)  {
			SetForegroundWindow(hWnd);
			EnumChildWindows(hWnd, _EnumChildProc, (LPARAM) L"C:\\temp\\39i\\Dsc_0001.jpg");
			bReady=true;
		}
	}
	return bReady;
}

//
// bwsGetCookie()
//
CHAR  * bwsGetCookie(CHAR * pszObj) {

	BSTR bsCookie;
	HRESULT hr;
	CHAR * pszRet=NULL;

	EH_WEBPAGE * psWebPage=_getWebPage(pszObj);
	hr=psWebPage->sIe.piHtmlDoc2->lpVtbl->get_cookie(psWebPage->sIe.piHtmlDoc2,&bsCookie);
	if (SUCCEEDED(hr)) {

		pszRet=_BstrToStr(psWebPage,bsCookie);

	}

	return pszRet;
}

static void _localAddFree(BYTE *lpType,BYTE *lpTagName,BYTE *lpValue,EH_LST lst,BOOL fChecked)
{
	BYTE *lpString;
	BOOL bTake=TRUE;
	if (lpTagName&&lpType)
	{
		strlwr(lpType);
		if (!strcmp(lpType,"checkbox")&&!fChecked) bTake=FALSE;
		if (!strcmp(lpType,"radio")&&!fChecked) bTake=FALSE;

		if (bTake)
		{
		 CHAR *lpValueNew;
		 lpValueNew=lpValue; if (!lpValueNew) lpValueNew="";
		 lpString=ehAlloc(strlen(lpTagName)+strlen(lpValueNew)+10);
		 sprintf(lpString,"%s\1%s",lpTagName,lpValueNew);
		 //ARMakerEx(WS_ADD,lpString,lps);
		 lstPush(lst,lpString);
		 ehFree(lpString); 
		}
	}
	if (lpTagName) ehFree(lpTagName); 
	if (lpType) ehFree(lpType); 
	if (lpValue) ehFree(lpValue);
}


//
// bwsGetListFields)
//
EH_LST bwsGetListFields(CHAR * pszObj,CHAR * pszNameForm) {


	HRESULT hr;
	INT hdlForm=-1;
	// S_ARMAKER sArMaker;
	EH_LST lstInput=NULL;
	EH_WEBPAGE * psWebPage=_getWebPage(pszObj);
	INT i;

	if (strstr(pszNameForm,".")) 
			ehError(); // Da implementare

	if (!_ieCreate(psWebPage,TRUE))
	{
		if (psWebPage->sIe.iTags>0) lstInput=lstNew();//ARMakerEx(WS_OPEN,NULL,&sArMaker);

		//
		// Ciclo sulla collezione di elementi
		//
		for (i=0;i<psWebPage->sIe.iTags;i++)
		{
			IDispatch	 *pTagDisp;
			IHTMLElement *piTag;
			VARIANT		 varIndex;	
			CHAR *lpName,*lpType,*lpValue,*lpTagName;
			BSTR bstr;

			// Recupero il singolo elemento all'indice "varIndex" 
			VariantInit(&varIndex);
			varIndex.vt = VT_I4;
			varIndex.lVal = (long) i;
			hr = psWebPage->sIe.piElemTag->lpVtbl->item(psWebPage->sIe.piElemTag, varIndex, varIndex, &pTagDisp);
			VariantClear(&varIndex);

			if ((SUCCEEDED(hr)) && (pTagDisp!=NULL)) {

				hr = pTagDisp->lpVtbl->QueryInterface(pTagDisp, &IID_IHTMLElement, (void**) &piTag );
				if ( (SUCCEEDED(hr)) && (piTag!=NULL) )
				{
					// Accedo finalmente all'elemento per capire che tipo è
					piTag->lpVtbl->get_tagName(piTag,&bstr);
					lpName=wcsToStr(bstr);
					strupr(lpName);

					// Se l'elemento è di tipo INPUT posso recuperare l'interfaccia più appropriata
					if (!strcmp(lpName,"INPUT"))
					{
						IHTMLInputElement* piInput = NULL;
						short fChecked;
						hr = pTagDisp->lpVtbl->QueryInterface(pTagDisp, &IID_IHTMLInputElement, (void**) &piInput );
						if ( (SUCCEEDED(hr)) && piInput )
						{
							piInput->lpVtbl->get_type(piInput,&bstr); lpType=bstr?wcsToStr(bstr):strDup("");
							piInput->lpVtbl->get_name(piInput,&bstr); lpTagName=bstr?wcsToStr(bstr):strDup("");
							piInput->lpVtbl->get_value(piInput,&bstr); lpValue=bstr?wcsToStr(bstr):strDup("");

							fChecked=FALSE; piInput->lpVtbl->get_checked(piInput,&fChecked); 

							//win_infoarg("[%s][%s][%s] %d",lpName,lpType,lpValue,fChecked);
							_localAddFree(lpType,lpTagName,lpValue,lstInput,fChecked);

							// Rilascio l'elemento specifico
							_$Release(piInput);
						}
					}
					
					// Se l'elemento è di tipo INPUT posso recuperare l'interfaccia più appropriata
					if (!strcmp(lpName,"TEXTAREA")) {
						IHTMLTextAreaElement * piTextArea= NULL;
						hr = pTagDisp->lpVtbl->QueryInterface(pTagDisp, &IID_IHTMLTextAreaElement, (void**)&piTextArea);
						if ( SUCCEEDED(hr) && piTextArea )
						{
							piTextArea->lpVtbl->get_type(piTextArea,&bstr); lpType=bstr?wcsToStr(bstr):strDup("");
							piTextArea->lpVtbl->get_name(piTextArea,&bstr); lpTagName=bstr?wcsToStr(bstr):strDup("");
							piTextArea->lpVtbl->get_value(piTextArea,&bstr); lpValue=bstr?wcsToStr(bstr):strDup("");
							
							_localAddFree(lpType,lpTagName,lpValue,lstInput,TRUE);
							_$Release(piTextArea);
						}
					}

					// Se l'elemento è di tipo INPUT posso recuperare l'interfaccia più appropriata
					if (!strcmp(lpName,"SELECT"))
					{
						IHTMLSelectElement * piSelect= NULL;
						hr = pTagDisp->lpVtbl->QueryInterface(pTagDisp, &IID_IHTMLSelectElement, (void**)&piSelect);
						if ( SUCCEEDED(hr) && piSelect )
						{
							piSelect->lpVtbl->get_type(piSelect,&bstr); lpType=wcsToStr(bstr);
							piSelect->lpVtbl->get_name(piSelect,&bstr); lpTagName=wcsToStr(bstr);
							piSelect->lpVtbl->get_value(piSelect,&bstr); lpValue=wcsToStr(bstr);
							
							_localAddFree(lpType,lpTagName,lpValue,lstInput,TRUE);
							piSelect->lpVtbl->Release(piSelect);
							_$Release(piSelect);
						}
					
					}

					ehFree(lpName);
					_$Release(piTag);//piTag->lpVtbl->Release(piTag);							
				}
				_$Release(pTagDisp);
			}
			
		}
		//if (psWebPage->sIe.iTags>0) hdlForm=ARMakerEx(WS_CLOSE,"arForm",&sArMaker);
	}

	return lstInput;

}

//
// bwsSetEmulation()
//
DWORD	bwsSetEmulation(DWORD dwEmulation) {

	DWORD dwVersionSet=0;
	DWORD dwVersionNow=0;
	CHAR szServ[500];

	EH_AR ar;
	CHAR * pszVersion;
//	DWORD dwType;

	// Versione attuale del browser
	pszVersion=msGetRegisterKey("HKEY_LOCAL_MACHINE/SOFTWARE/Microsoft/Internet Explorer/svcVersion",NULL,NULL);
	if (!pszVersion) pszVersion=msGetRegisterKey("HKEY_LOCAL_MACHINE/SOFTWARE/Microsoft/Internet Explorer/Version",NULL,NULL);
	
	if (!pszVersion) ehError();
	ar=strSplit(pszVersion,".");
	dwVersionNow=atoi(ar[0]);
	ehFree(ar);
	ehFree(pszVersion);

	//
	// Controllo fattibilità
	//
	if (dwVersionNow>=(dwEmulation/1000)) dwVersionSet=dwEmulation; else dwVersionSet=dwVersionNow*1000;

	//
	// Setto al versione
	//
	// Fino a /main, speriamo che esista

	sprintf(szServ,"HKEY_CURRENT_USER/Software/Microsoft/Internet Explorer/Main/FeatureControl");
	if (msSetRegisterKey(szServ,&dwVersionSet,sizeof(dwVersionSet),REG_NONE)) {
		alert("Non posso creare %s",szServ);
	}

	sprintf(szServ,"HKEY_CURRENT_USER/Software/Microsoft/Internet Explorer/Main/FeatureControl/FEATURE_BROWSER_EMULATION");
	if (msSetRegisterKey(szServ,&dwVersionSet,sizeof(dwVersionSet),REG_NONE)) {
		alert("Non posso creare %s",szServ);
	}

	sprintf(szServ,"HKEY_CURRENT_USER/Software/Microsoft/Internet Explorer/Main/FeatureControl/FEATURE_BROWSER_EMULATION/%s",fileName(sys.szAppNameFull));
	if (msSetRegisterKey(szServ,&dwVersionSet,sizeof(dwVersionSet),REG_DWORD)) dwVersionSet=0;

	return dwVersionSet;
}

//
// bwsSetLoading()
//
BOOL bwsSetLoading(CHAR * pszObj, DWORD dwStatus) {

   	EH_WEBPAGE * psWebPage;
	psWebPage=_getWebPage(pszObj); if (!psWebPage) return true;
	psWebPage->iStatusLoading=dwStatus;

	return false;

}

//
// bwsLoadingWaiting()
//
BOOL	bwsLoadingWaiting(CHAR * pszObj, DWORD dwStatus, DWORD dwTimeout) {

   	EH_WEBPAGE * psWebPage;
	BOOL bDone=false;
	psWebPage=_getWebPage(pszObj); if (!psWebPage) return false;

	if (dwTimeout) {

		DWORD dwRif,msElapsed;
		_IEI *	psIe=&psWebPage->sIe;
		chronoStart(&dwRif);
		while (true) {

			msElapsed=(DWORD) ((double) chronoGet(&dwRif)/CLOCKS_PER_SEC*1000);

			if ((psWebPage->iStatusLoading&dwStatus)==dwStatus) {
				bDone=true;
				break;
			}

			if (msElapsed>dwTimeout) break;
			PauseActive(300);
		}
	}
	return bDone;

}