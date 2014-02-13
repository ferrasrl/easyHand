//   -----------------------------------------------------------
//   | ehzDirectShow
//   | Movie Player (ehz Wrapper)
//   |                                             
//   |										by Ferrà Srl 2013
//   -----------------------------------------------------------

// -----------------------------------------------
//
// Divisione del sorgente
//
// -----------------------------------------------
// ehz : Main Functions
// funzione ehzDirectShow ed insieme di funzioni dedicate ad essa
//
// -----------------------------------------------
// DShowPlayer (wrapper)
// Classe DShowPlayer(), per inizializzare il componente (ricercare la versione)
//
// -----------------------------------------------
// video versions (wrappers)
// Classi necessarie per il matching con le 3 versioni possibili di componente EVR,VMR7,VMR9
//


#include <new>
#include "/easyhand/inc/easyhand.h" 
#include "/easyhand/inc/ehzDirectShow.h" 
#include <Mfidl.h>

#pragma comment(lib, "strmiids")
#define WC_EH_DIRECTSHOW "ehDirectShow"


// -----------------------------------------------
// ehz : Main Functions
// -----------------------------------------------


static LRESULT CALLBACK _windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void CALLBACK	_onGraphEvent(HWND hwnd, long eventCode, LONG_PTR param1, LONG_PTR param2);
//static INT				_movieOpen(EHZ_DIRECTSHOW * psDs, UTF8 * putfFileName);
static void				_onSize(EHZ_DIRECTSHOW * psDs);
static void				_dsInitialize(void);
static HWND				_createDirectShow(HWND, INT,INT);

static struct {

	BOOL			bReady;

} _dx={0};

static	BOOL	_Open(void * psDs, UTF8 * putfFileName); 
static	BOOL	_Close(void *psDs); 
static	BOOL	_Show(void *psDs); 
static	BOOL	_Hide(void *psDs); 

static	BOOL	_Play(void *psDs); 
static	BOOL	_Pause(void *psDs); 
static	BOOL	_Stop(void *psDs); 
static	BOOL	_SetPosition(void *psDs,double dStart,double dStop); 

//
// ehzDirectShow()
//
void * ehzDirectShow(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr) {


	EHZ_DIRECTSHOW * psDs;
	EH_DISPEXT * psExt=(EH_DISPEXT *) ptr;
	EN_MESSAGE msgSub=(EN_MESSAGE) info;
	DShowPlayer * oPlayer;

	if (!objCalled) return NULL; // 

	if (!_dx.bReady) _dsInitialize();

	psDs=(EHZ_DIRECTSHOW *) objCalled->pOther;
	if (psDs) oPlayer=(DShowPlayer *) psDs->pClass;
	switch(cmd)
	{
			case WS_INF: return psDs;

			case WS_CREATE: 

				objCalled->pOther=ehAllocZero(sizeof(EHZ_DIRECTSHOW));
				psDs=(EHZ_DIRECTSHOW *) objCalled->pOther;
				psDs->psObj=objCalled;
				psDs->hWnd=objCalled->hWnd=_createDirectShow(WindowNow(),objCalled->col1,objCalled->col2);
				SetWindowLong(objCalled->hWnd,GWL_USERDATA,(LONG) psDs);

				//
				// Creo la classe del Player
				//
				oPlayer = new (std::nothrow) DShowPlayer(psDs->hWnd);
				psDs->pClass=oPlayer;
				if (oPlayer == NULL) {
					ehError();
				}
				psDs->Open=_Open;
				psDs->Close=_Close;
				psDs->Hide=_Hide;
				psDs->Show=_Show;

				psDs->Play=_Play;
				psDs->Pause=_Pause;
				psDs->Stop=_Stop;
				psDs->SetPosition=_SetPosition;
				break;

		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,psExt->px+relwx,psExt->py+relwy,psExt->lx,psExt->ly,TRUE);
			break;

		case WS_DISPLAY:
			break;
/*
		case WS_SUBPARAM:

			switch (msgSub) {
			
				case WS_OPEN:
					_movieOpen(psDs,(UTF8 *) ptr);
					break;

				case WS_PRINT:
					ShowWindow(psDs->hWnd,atoi((CHAR *) strEver(ptr))?SW_NORMAL:SW_HIDE);
					break;

				case WS_DO:

					if (!strCmp((CHAR * ) ptr,"play")) oPlayer->Play();
					if (!strCmp((CHAR * ) ptr,"pause")) oPlayer->Pause();
					if (!strCmp((CHAR * ) ptr,"stop")) 
					{
						oPlayer->Stop();
						obj_putevent("%s.STOP",psDs->psObj->nome);
					}
					break;
			
			}
			*/
			break;
/*
		case WS_EVENT:

			psEvent=(EH_EVENT *) ptr;
			switch (psEvent->iEvent)
			{
				//
				// Controllo del click sinistro
				//
				case EE_LBUTTONDOWN:
					
					// individuo l' area per il tasto "successivo"
					//if (psEvent->sPoint.x>objCalled->sClientSize.cx-50) {
						
						obj_putevent("%s:CLK",objCalled->nome);

					//}
					break;
			}
			break;
*/
	}
	return NULL;
}

static void _dsInitialize(void) {
	
    if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE)))
    {
        alert( "CoInitializeEx failed.");
        ehError();
    }

    WNDCLASS wc = { };
    wc.lpfnWndProc   = _windowProc;
    wc.hInstance     = sys.EhWinInstance;
    wc.lpszClassName = WC_EH_DIRECTSHOW;

    if (!RegisterClass(&wc)) 
		ehError();

	_dx.bReady=true;

}
// 
// _createDirectShow() - Crea la finestre per il Direct Show
// 
static HWND _createDirectShow(HWND hwndParent, INT iWidth,INT iHeight) 
{

    HWND hwnd = CreateWindowEx(0, WC_EH_DIRECTSHOW, "DirectShow Playback", 
				WS_VISIBLE| WS_CHILD,// WS_OVERLAPPEDWINDOW, 
				0, 0, iWidth, iHeight, hwndParent, NULL, sys.EhWinInstance, NULL);

    if (hwnd)
    {
		ShowWindow(hwnd, SW_HIDE);
    }
	
	return hwnd;
}

//
// _Open
//
static	BOOL	_Open(void * pClass, UTF8 * putfFileName) {

	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) pClass;
	DShowPlayer * oPlayer=(DShowPlayer *) psDs->pClass;
	WCHAR * pwcFileName=(WCHAR *) strEnc(1,putfFileName,SD_UTF8,NULL);
	HRESULT hr;
	
	//
	// Apro il file di default
	//
	hr = oPlayer->OpenFile(pwcFileName); // Wildlife.wmv");
	if (SUCCEEDED(hr))
	{
		_onSize(psDs);
		InvalidateRect(psDs->hWnd, NULL, FALSE);

//		oPlayer->Play();
	}
	else
	{
		alert("Cannot open this file.");
	}
	ehFree(pwcFileName);

	return hr;
}

static	BOOL	_Show(void *pClass) {

	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) pClass; DShowPlayer * oPlayer=(DShowPlayer *) psDs->pClass;
	ShowWindow(psDs->hWnd,SW_NORMAL);
	return false;

}


static	BOOL	_Hide(void *pClass) {

	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) pClass; DShowPlayer * oPlayer=(DShowPlayer *) psDs->pClass;
	ShowWindow(psDs->hWnd,SW_HIDE);
	return false;

}


//
// _Close (Da fare)
//
static	BOOL	_Close(void *pClass) {
	
	return false;
}

static	BOOL	_Play(void *pClass) {

	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) pClass; DShowPlayer * oPlayer=(DShowPlayer *) psDs->pClass;
	oPlayer->Play();
	return false;

}
static	BOOL	_Pause(void *pClass) {

	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) pClass; DShowPlayer * oPlayer=(DShowPlayer *) psDs->pClass;
	oPlayer->Pause();
	return false;

}
static	BOOL	_Stop(void *pClass) {

	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) pClass; DShowPlayer * oPlayer=(DShowPlayer *) psDs->pClass;
	oPlayer->Stop();
	obj_putevent("%s.STOP",psDs->psObj->nome);
	return false;


}
static	BOOL	_SetPosition(void *pClass,double dStart,double dStop) {

	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) pClass; DShowPlayer * oPlayer=(DShowPlayer *) psDs->pClass;
	oPlayer->SetPosition(dStart,dStop);
	return false;
}



//
// _windowProc()
//
LRESULT CALLBACK _windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) GetWindowLong(hWnd,GWL_USERDATA);
	DShowPlayer *oPlayer;
	if (psDs) oPlayer=(DShowPlayer *) psDs->pClass;
    switch (uMsg)
    {
		case WM_CREATE:
			return 0;

		case WM_DESTROY:
			if (!psDs) break;
			delete oPlayer;
			PostQuitMessage(0);
			return 0;

		case WM_DISPLAYCHANGE:
			if (!psDs) break;
			oPlayer->DisplayModeChanged();
			break;

		case WM_ERASEBKGND:
			return 1;

		case WM_PAINT:
			if (psDs) 
			{
				PAINTSTRUCT ps;
				HDC hdc;

				hdc = BeginPaint(hWnd, &ps);

				if (oPlayer->State() != STATE_NO_GRAPH && oPlayer->HasVideo())
				{
					// The player has video, so ask the player to repaint. 
					oPlayer->Repaint(hdc);
				}
				else
				{
					FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
				}

				EndPaint(hWnd, &ps);
			}

			return 0;

		case WM_SIZE:
			if (psDs) _onSize(psDs);
			return 0;

		case WM_GRAPH_EVENT:
			if (!psDs) break;
		   oPlayer->HandleGraphEvent(_onGraphEvent);
		   return 0;

		//
		// Tastiera
		//
		case WM_CHAR:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
			I_KeyTraslator_Windows(WindowNow(),uMsg,lParam,wParam);
			break;

		//
		// Controllo del mouse
		//
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
	//		SetFocus(hWnd);

		case WM_MOUSEMOVE:
		case WM_RBUTTONUP:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:

	//		if (!psDs->funcNotify) break;

	//		if (uMsg==WM_LBUTTONUP) _OnFileOpen(psDs);

	/*
			ZeroFill(sEvent);
			sEvent.iEvent=ehOsEventTranslate(message);
			sEvent.hWnd=hWnd;
			sEvent.sPoint.x=psDx->psObj->sClientCursor.x=LOWORD(lParam);
			sEvent.sPoint.y=psDx->psObj->sClientCursor.y=HIWORD(lParam);
			psDx->funcNotify(psDx->psObj,WS_EVENT,0,&sEvent);			  
			*/
			//return 0;
			break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
void OnPaint(EHZ_DIRECTSHOW * psDs,HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint(hwnd, &ps);

    if (oPlayer->State() != STATE_NO_GRAPH && oPlayer->HasVideo())
    {
        // The player has video, so ask the player to repaint. 
        oPlayer->Repaint(hdc);
    }
    else
    {
        FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
    }

    EndPaint(hwnd, &ps);
}
*/
//
//
//
static void _onSize(EHZ_DIRECTSHOW * psDs)
{
	DShowPlayer * oPlayer=(DShowPlayer *) psDs->pClass;
    if (oPlayer) {
        RECT rc;
		GetClientRect(psDs->hWnd, &rc);
		/*
		rc.top -=1200;
		rc.left-=1200;
		rc.right+=1200;
		*/
        oPlayer->UpdateVideoWindow(&rc);
    }
}

//
// _onGraphEvent()
//
void CALLBACK _onGraphEvent(HWND hWnd, long evCode, LONG_PTR param1, LONG_PTR param2)
{
	EHZ_DIRECTSHOW * psDs=(EHZ_DIRECTSHOW *) GetWindowLong(hWnd,GWL_USERDATA);
	DShowPlayer * oPlayer=NULL;
	
	if (psDs) oPlayer=(DShowPlayer *) psDs->pClass;
	if (!oPlayer) return;
	// printf("ge:%d" CRLF,evCode);
    switch (evCode)
    {
		case EC_COMPLETE:
		case EC_USERABORT:
			oPlayer->Stop();
			obj_putevent("%s.COMPLETE",psDs->psObj->nome);
			break;

		case EC_ERRORABORT:
			alert("Playback error.");
			oPlayer->Stop();
			obj_putevent("%s.ERROR",psDs->psObj->nome);
			obj_putevent("%s.STOP",psDs->psObj->nome);
			break;
    }
}
/*
void NotifyError(HWND hwnd, PSTR pszMessage)
{
    MessageBox(hwnd, pszMessage, "Error", MB_OK | MB_ICONERROR);
}
*/















// -----------------------------------------------
//
// DShowPlayer (Dichiarazione)                    ##########################################################################
//
// -----------------------------------------------


DShowPlayer::DShowPlayer(HWND hwnd) : // Costruttore

	m_state(STATE_NO_GRAPH),
    m_hwnd(hwnd),
    m_pGraph(NULL),
    m_pControl(NULL),
    m_pEvent(NULL),
    m_pVideo(NULL),
	m_pSeek(NULL)

{

}

DShowPlayer::~DShowPlayer()
{
    TearDownGraph();
}

//
// Open a media file for playback.
//
HRESULT DShowPlayer::OpenFile(PCWSTR pszFileName)
{
    IBaseFilter *pSource = NULL;

    // Create a new filter graph. (This also closes the old one, if any.)
    HRESULT hr = InitializeGraph();
    if (FAILED(hr))
    {
        goto done;
    }
    
    // Add the source filter to the graph.
    hr = m_pGraph->AddSourceFilter(pszFileName, NULL, &pSource);
    if (FAILED(hr))
    {
        goto done;
    }

    // Try to render the streams.
    hr = RenderStreams(pSource);

done:
    if (FAILED(hr))
    {
        TearDownGraph();
    }
    SafeRelease(&pSource);
    return hr;
}


// Respond to a graph event.
//
// The owning window should call this method when it receives the window
// message that the application specified when it called SetEventWindow.
//
// Caution: Do not tear down the graph from inside the callback.

HRESULT DShowPlayer::HandleGraphEvent(GraphEventFN pfnOnGraphEvent)
{
    if (!m_pEvent)
    {
        return E_UNEXPECTED;
    }

    long evCode = 0;
    LONG_PTR param1 = 0, param2 = 0;

    HRESULT hr = S_OK;

    // Get the events from the queue.
    while (SUCCEEDED(m_pEvent->GetEvent(&evCode, &param1, &param2, 0)))
    {
        // Invoke the callback.
        pfnOnGraphEvent(m_hwnd, evCode, param1, param2);

        // Free the event data.
        hr = m_pEvent->FreeEventParams(evCode, param1, param2);
        if (FAILED(hr))
        {
            break;
        }
    }
    return hr;
}

//
// Play
//
HRESULT DShowPlayer::Play()
{
    if (m_state != STATE_PAUSED && m_state != STATE_STOPPED)
    {
        return VFW_E_WRONG_STATE;
    }



    HRESULT hr = m_pControl->Run();
    if (SUCCEEDED(hr))
    {
        m_state = STATE_RUNNING;
    }
    return hr;
}

//
// Pause
//
HRESULT DShowPlayer::Pause()
{
    if (m_state != STATE_RUNNING)
    {
        return VFW_E_WRONG_STATE;
    }

    HRESULT hr = m_pControl->Pause();
    if (SUCCEEDED(hr))
    {
        m_state = STATE_PAUSED;
    }
    return hr;
}

//
// Stop
//
HRESULT DShowPlayer::Stop()
{
    if (m_state != STATE_RUNNING && m_state != STATE_PAUSED)
    {
        return VFW_E_WRONG_STATE;
    }

    HRESULT hr = m_pControl->Stop();
    if (SUCCEEDED(hr))
    {
		SetPosition(0,-1);
        m_state = STATE_STOPPED;
								//&rtStop, AM_SEEKING_AbsolutePositioning);
		
    }
    return hr;
}

//
// Play
//
HRESULT DShowPlayer::SetPosition(double dSecStart,double dSecStop)
{
	#define ONE_SECOND 10000000
	REFERENCE_TIME rtNow=0,rtStop=0;
	REFERENCE_TIME * prtNow=NULL,*prtStop=NULL;
	HRESULT hr;

	if (dSecStart!=-1) {rtNow=(REFERENCE_TIME) (dSecStart * ONE_SECOND); prtNow=&rtNow;}
	if (dSecStop!=-1) {rtStop=(REFERENCE_TIME) (dSecStop * ONE_SECOND); prtStop=&rtStop;}

	hr=m_pSeek->SetPositions( prtNow,  (dSecStart!=-1)?AM_SEEKING_AbsolutePositioning:AM_SEEKING_NoPositioning, 
							 prtStop,  (dSecStop!=-1)?AM_SEEKING_AbsolutePositioning:AM_SEEKING_NoPositioning); 
							 //NULL,AM_SEEKING_NoPositioning);

    return hr;

}
// EVR/VMR functionality

BOOL DShowPlayer::HasVideo() const 
{ 
    return (m_pVideo && m_pVideo->HasVideo()); 
}

// Sets the destination rectangle for the video.

HRESULT DShowPlayer::UpdateVideoWindow(const LPRECT prc)
{
    if (m_pVideo)
    {
        return m_pVideo->UpdateVideoWindow(m_hwnd, prc);
    }
    else
    {
        return S_OK;
    }
}

// Repaints the video. Call this method when the application receives WM_PAINT.

HRESULT DShowPlayer::Repaint(HDC hdc)
{
    if (m_pVideo)
    {
        return m_pVideo->Repaint(m_hwnd, hdc);
    }
    else
    {
        return S_OK;
    }
}


// Notifies the video renderer that the display mode changed.
//
// Call this method when the application receives WM_DISPLAYCHANGE.

HRESULT DShowPlayer::DisplayModeChanged()
{
    if (m_pVideo)
    {
        return m_pVideo->DisplayModeChanged();
    }
    else
    {
        return S_OK;
    }
}


//
// Graph building
// Create a new filter graph. 
// 
// Istanzio il componente
//

HRESULT DShowPlayer::InitializeGraph()
{
    TearDownGraph();

    // Create the Filter Graph Manager.
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, 
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pGraph));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pControl));
    if (FAILED(hr)) goto done;

    hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pSeek));
    if (FAILED(hr)) goto done;

    hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&m_pEvent));
    if (FAILED(hr)) goto done;

    // Set up event notification.
    hr = m_pEvent->SetNotifyWindow((OAHWND)m_hwnd, WM_GRAPH_EVENT, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

    m_state = STATE_STOPPED;

done:
    return hr;
}

//
// TearDownGraph: Chiusura dell'istanza con il componente
//
void DShowPlayer::TearDownGraph()
{
    // Stop sending event messages
    if (m_pEvent)
    {
        m_pEvent->SetNotifyWindow((OAHWND)NULL, NULL, NULL);
    }

    SafeRelease(&m_pGraph);
    SafeRelease(&m_pControl);
    SafeRelease(&m_pEvent);
    SafeRelease(&m_pSeek);

    delete m_pVideo;
    m_pVideo = NULL;

    m_state = STATE_NO_GRAPH;
}

//
// CreateVideoRenderer() - Cerca la versione da usare
//
HRESULT DShowPlayer::CreateVideoRenderer()
{
    HRESULT hr = E_FAIL;

    enum { Try_VMR7, Try_VMR9, Try_EVR };

    for (DWORD i = 0; i <= 2; i++)
    {
        switch (i)
        {
        case Try_EVR:
            m_pVideo = new (std::nothrow) CEVR();
            break;

        case Try_VMR9:
            m_pVideo = new (std::nothrow) CVMR9();
            break;

        case Try_VMR7:
            m_pVideo = new (std::nothrow) CVMR7();
            break;
        }

        if (m_pVideo == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        hr = m_pVideo->AddToGraph(m_pGraph, m_hwnd);
        if (SUCCEEDED(hr))
        {
            break;
        }

        delete m_pVideo;
        m_pVideo = NULL;
    }
    return hr;
}


// Render the streams from a source filter. 

HRESULT DShowPlayer::RenderStreams(IBaseFilter *pSource)
{
    BOOL bRenderedAnyPin = FALSE;

    IFilterGraph2 *pGraph2 = NULL;
    IEnumPins *pEnum = NULL;
    IBaseFilter *pAudioRenderer = NULL;
	
    HRESULT hr = m_pGraph->QueryInterface(IID_PPV_ARGS(&pGraph2));
    if (FAILED(hr))
    {
        goto done;
    }

    // Add the video renderer to the graph
    hr = CreateVideoRenderer();
    if (FAILED(hr))
    {
        goto done;
    }

    // Add the DSound Renderer to the graph.
    hr = AddFilterByCLSID(m_pGraph, CLSID_DSoundRender, 
        &pAudioRenderer, L"Audio Renderer");
    if (FAILED(hr))
    {
        goto done;
    }

    // Enumerate the pins on the source filter.
    hr = pSource->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        goto done;
    }

    // Loop through all the pins
    IPin *pPin;
    while (S_OK == pEnum->Next(1, &pPin, NULL))
    {           
        // Try to render this pin. 
        // It's OK if we fail some pins, if at least one pin renders.
        HRESULT hr2 = pGraph2->RenderEx(pPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, NULL);

        pPin->Release();
        if (SUCCEEDED(hr2))
        {
            bRenderedAnyPin = TRUE;
        }
    }

    hr = m_pVideo->FinalizeGraph(m_pGraph);
    if (FAILED(hr))
    {
        goto done;
    }

    // Remove the audio renderer, if not used.
    BOOL bRemoved;
    hr = RemoveUnconnectedRenderer(m_pGraph, pAudioRenderer, &bRemoved);

done:
    SafeRelease(&pEnum);
    SafeRelease(&pAudioRenderer);
    SafeRelease(&pGraph2);

    // If we succeeded to this point, make sure we rendered at least one 
    // stream.
    if (SUCCEEDED(hr))
    {
        if (!bRenderedAnyPin)
        {
            hr = VFW_E_CANNOT_RENDER;
        }
    }
    return hr;
}




// -----------------------------------------------
//
// Video (Classi wrapper per versioni)			  ##########################################################################
//
// -----------------------------------------------
// 
// Crea una classe per la versione dispoinbile di windows, con metodi usati dal DShowPlayer
//
// ::HasVideo()				Non chiaro.. sembra se esiste una finestra associata alla classe
// ::AddToGraph()			Associa la finestra da usare alla "motore" video 
// ::FinalizeGraph()		(?)
// ::UpdateVideoWindow()	Usato con WM_SIZE (ridimensionamento della finestra)
// ::Repaint()				Da chiamare con l'evento WM_PAINT di windows
// ::DisplayModeChanged()	Da chiamare con l'evento WM_DISPLAYMODECHANGE di windows
//
static HRESULT _InitializeEVR(IBaseFilter *pEVR, HWND hwnd, IMFVideoDisplayControl ** ppWc); 
static HRESULT _InitWindowlessVMR9(IBaseFilter *pVMR, HWND hwnd, IVMRWindowlessControl9 ** ppWc); 
static HRESULT _InitWindowlessVMR(IBaseFilter *pVMR, HWND hwnd, IVMRWindowlessControl** ppWc); 
static HRESULT _FindConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);


/// VMR-7 Wrapper

CVMR7::CVMR7() : m_pWindowless(NULL)
{

}

CVMR7::~CVMR7()
{
    SafeRelease(&m_pWindowless);
}

BOOL CVMR7::HasVideo() const 
{ 
    return (m_pWindowless != NULL); 
}

HRESULT CVMR7::AddToGraph(IGraphBuilder *pGraph, HWND hwnd)
{
    IBaseFilter *pVMR = NULL;

    HRESULT hr = AddFilterByCLSID(pGraph, CLSID_VideoMixingRenderer, 
        &pVMR, L"VMR-7");

    if (SUCCEEDED(hr))
    {
        // Set windowless mode on the VMR. This must be done before the VMR
        // is connected.
        hr = _InitWindowlessVMR(pVMR, hwnd, &m_pWindowless);
    }
    SafeRelease(&pVMR);
    return hr;
}

HRESULT CVMR7::FinalizeGraph(IGraphBuilder *pGraph)
{
    if (m_pWindowless == NULL)
    {
        return S_OK;
    }

    IBaseFilter *pFilter = NULL;

    HRESULT hr = m_pWindowless->QueryInterface(IID_PPV_ARGS(&pFilter));
    if (FAILED(hr))
    {
        goto done;
    }

    BOOL bRemoved;
    hr = RemoveUnconnectedRenderer(pGraph, pFilter, &bRemoved);

    // If we removed the VMR, then we also need to release our 
    // pointer to the VMR's windowless control interface.
    if (bRemoved)
    {
        SafeRelease(&m_pWindowless);
    }

done:
    SafeRelease(&pFilter);
    return hr;
}

HRESULT CVMR7::UpdateVideoWindow(HWND hwnd, const LPRECT prc)
{
    if (m_pWindowless == NULL)
    {
        return S_OK; // no-op
    }

    if (prc)
    {
        return m_pWindowless->SetVideoPosition(NULL, prc);
    }
    else
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        return m_pWindowless->SetVideoPosition(NULL, &rc);
    }
}

HRESULT CVMR7::Repaint(HWND hwnd, HDC hdc)
{
    if (m_pWindowless)
    {
        return m_pWindowless->RepaintVideo(hwnd, hdc);
    }
    else
    {
        return S_OK;
    }
}

HRESULT CVMR7::DisplayModeChanged()
{
    if (m_pWindowless)
    {
        return m_pWindowless->DisplayModeChanged();
    }
    else
    {
        return S_OK;
    }
}



// Initialize the VMR-7 for windowless mode. 

HRESULT _InitWindowlessVMR( 
    IBaseFilter *pVMR,              // Pointer to the VMR
    HWND hwnd,                      // Clipping window
    IVMRWindowlessControl** ppWC    // Receives a pointer to the VMR.
    ) 
{ 

	RECT rcDest;
    IVMRFilterConfig* pConfig = NULL; 
    IVMRWindowlessControl *pWC = NULL;

    // Set the rendering mode.  
    HRESULT hr = pVMR->QueryInterface(IID_PPV_ARGS(&pConfig)); 
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pConfig->SetRenderingMode(VMRMode_Windowless); 
    if (FAILED(hr))
    {
        goto done;
    }

    // Query for the windowless control interface.
    hr = pVMR->QueryInterface(IID_PPV_ARGS(&pWC));
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the clipping window.
    hr = pWC->SetVideoClippingWindow(hwnd);
    if (FAILED(hr))
    {
        goto done;
    }

    // Preserve aspect ratio by letter-boxing
    hr = pWC->SetAspectRatioMode(VMR_ARMODE_LETTER_BOX);
    if (FAILED(hr))
    {
        goto done;
    }

	hr = pWC->SetVideoPosition(NULL,rectFill(&rcDest,0,0,50,50));

	// Return the IVMRWindowlessControl pointer to the caller.
    *ppWC = pWC;
    (*ppWC)->AddRef();

done:
    SafeRelease(&pConfig);
    SafeRelease(&pWC);
    return hr; 
} 

///
/// VMR-9 Wrapper
///

CVMR9::CVMR9() : m_pWindowless(NULL)
{

}

BOOL CVMR9::HasVideo() const 
{ 
    return (m_pWindowless != NULL); 
}

CVMR9::~CVMR9()
{
    SafeRelease(&m_pWindowless);
}

HRESULT CVMR9::AddToGraph(IGraphBuilder *pGraph, HWND hwnd)
{
    IBaseFilter *pVMR = NULL;

    HRESULT hr = AddFilterByCLSID(pGraph, CLSID_VideoMixingRenderer9, 
        &pVMR, L"VMR-9");
    if (SUCCEEDED(hr))
    {
        // Set windowless mode on the VMR. This must be done before the VMR 
        // is connected.
        hr = _InitWindowlessVMR9(pVMR, hwnd, &m_pWindowless);
    }
    SafeRelease(&pVMR);
    return hr;
}

HRESULT CVMR9::FinalizeGraph(IGraphBuilder *pGraph)
{
    if (m_pWindowless == NULL)
    {
        return S_OK;
    }

    IBaseFilter *pFilter = NULL;

    HRESULT hr = m_pWindowless->QueryInterface(IID_PPV_ARGS(&pFilter));
    if (FAILED(hr))
    {
        goto done;
    }

    BOOL bRemoved;
    hr = RemoveUnconnectedRenderer(pGraph, pFilter, &bRemoved);

    // If we removed the VMR, then we also need to release our 
    // pointer to the VMR's windowless control interface.
    if (bRemoved)
    {
        SafeRelease(&m_pWindowless);
    }

done:
    SafeRelease(&pFilter);
    return hr;
}


HRESULT CVMR9::UpdateVideoWindow(HWND hwnd, const LPRECT prc)
{
    if (m_pWindowless == NULL)
    {
        return S_OK; // no-op
    }

    if (prc)
    {
        return m_pWindowless->SetVideoPosition(NULL, prc);
    }
    else
    {

        RECT rc;
        GetClientRect(hwnd, &rc);
        return m_pWindowless->SetVideoPosition(NULL, &rc);
    }
}

HRESULT CVMR9::Repaint(HWND hwnd, HDC hdc)
{
    if (m_pWindowless)
    {
        return m_pWindowless->RepaintVideo(hwnd, hdc);
    }
    else
    {
        return S_OK;
    }
}

HRESULT CVMR9::DisplayModeChanged()
{
    if (m_pWindowless)
    {
        return m_pWindowless->DisplayModeChanged();
    }
    else
    {
        return S_OK;
    }
}


// Initialize the VMR-9 for windowless mode. 

HRESULT _InitWindowlessVMR9( 
    IBaseFilter *pVMR,              // Pointer to the VMR
    HWND hwnd,                      // Clipping window
    IVMRWindowlessControl9** ppWC   // Receives a pointer to the VMR.
    ) 
{ 

    IVMRFilterConfig9 * pConfig = NULL; 
    IVMRWindowlessControl9 *pWC = NULL;

    // Set the rendering mode.  
    HRESULT hr = pVMR->QueryInterface(IID_PPV_ARGS(&pConfig)); 
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pConfig->SetRenderingMode(VMR9Mode_Windowless); 
    if (FAILED(hr))
    {
        goto done;
    }

    // Query for the windowless control interface.
    hr = pVMR->QueryInterface(IID_PPV_ARGS(&pWC));
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the clipping window.
    hr = pWC->SetVideoClippingWindow(hwnd);
    if (FAILED(hr))
    {
        goto done;
    }

    // Preserve aspect ratio by letter-boxing
    hr = pWC->SetAspectRatioMode(VMR9ARMode_LetterBox);
    if (FAILED(hr))
    {
        goto done;
    }

    // Return the IVMRWindowlessControl pointer to the caller.
    *ppWC = pWC;
    (*ppWC)->AddRef();

done:
    SafeRelease(&pConfig);
    SafeRelease(&pWC);
    return hr; 
} 


/// EVR Wrapper

CEVR::CEVR() : m_pEVR(NULL), m_pVideoDisplay(NULL)
{

}

CEVR::~CEVR()
{
    SafeRelease(&m_pEVR);
    SafeRelease(&m_pVideoDisplay);
}

BOOL CEVR::HasVideo() const 
{ 
    return (m_pVideoDisplay != NULL); 
}

HRESULT CEVR::AddToGraph(IGraphBuilder *pGraph, HWND hwnd)
{
    IBaseFilter *pEVR = NULL;

    HRESULT hr = AddFilterByCLSID(pGraph, CLSID_EnhancedVideoRenderer, 
        &pEVR, L"EVR");

    if (FAILED(hr))
    {
        goto done;
    }

    hr = _InitializeEVR(pEVR, hwnd, &m_pVideoDisplay);
    if (FAILED(hr))
    {
        goto done;
    }

    // Note: Because IMFVideoDisplayControl is a service interface,
    // you cannot QI the pointer to get back the IBaseFilter pointer.
    // Therefore, we need to cache the IBaseFilter pointer.

    m_pEVR = pEVR;
    m_pEVR->AddRef();

done:
    SafeRelease(&pEVR);
    return hr;
}

HRESULT CEVR::FinalizeGraph(IGraphBuilder *pGraph)
{
    if (m_pEVR == NULL)
    {
        return S_OK;
    }

    BOOL bRemoved;
    HRESULT hr = RemoveUnconnectedRenderer(pGraph, m_pEVR, &bRemoved);
    if (bRemoved)
    {
        SafeRelease(&m_pEVR);
        SafeRelease(&m_pVideoDisplay);
    }
    return hr;
}

HRESULT CEVR::UpdateVideoWindow(HWND hwnd, const LPRECT prc)
{
    if (m_pVideoDisplay == NULL)
    {
        return S_OK; // no-op
    }

    if (prc)
    {
        return m_pVideoDisplay->SetVideoPosition(NULL, prc);
    }
    else
    {

        RECT rc;
        GetClientRect(hwnd, &rc);
        return m_pVideoDisplay->SetVideoPosition(NULL, &rc);
    }
}

HRESULT CEVR::Repaint(HWND hwnd, HDC hdc)
{
    if (m_pVideoDisplay)
    {
        return m_pVideoDisplay->RepaintVideo();
    }
    else
    {
        return S_OK;
    }
}

HRESULT CEVR::DisplayModeChanged()
{
    // The EVR does not require any action in response to WM_DISPLAYCHANGE.
    return S_OK;
}


// Initialize the EVR filter. 

HRESULT _InitializeEVR( 
    IBaseFilter *pEVR,              // Pointer to the EVR
    HWND hwnd,                      // Clipping window
    IMFVideoDisplayControl** ppDisplayControl
    ) 
{ 
    IMFGetService *pGS = NULL;
    IMFVideoDisplayControl *pDisplay = NULL;

    HRESULT hr = pEVR->QueryInterface(IID_PPV_ARGS(&pGS)); 
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pGS->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplay));
    if (FAILED(hr))
    {
        goto done;
    }

    // Set the clipping window.
    hr = pDisplay->SetVideoWindow(hwnd);
    if (FAILED(hr))
    {
        goto done;
    }

    // Preserve aspect ratio by letter-boxing
    hr = pDisplay->SetAspectRatioMode(MFVideoARMode_PreservePicture);
    if (FAILED(hr))
    {
        goto done;
    }

    // Return the IMFVideoDisplayControl pointer to the caller.
    *ppDisplayControl = pDisplay;
    (*ppDisplayControl)->AddRef();

done:
    SafeRelease(&pGS);
    SafeRelease(&pDisplay);
    return hr; 
} 

// Helper functions.

HRESULT RemoveUnconnectedRenderer(IGraphBuilder *pGraph, IBaseFilter *pRenderer, BOOL *pbRemoved)
{
    IPin *pPin = NULL;

    *pbRemoved = FALSE;

    // Look for a connected input pin on the renderer.

    HRESULT hr = _FindConnectedPin(pRenderer, PINDIR_INPUT, &pPin);
    SafeRelease(&pPin);

    // If this function succeeds, the renderer is connected, so we don't remove it.
    // If it fails, it means the renderer is not connected to anything, so
    // we remove it.

    if (FAILED(hr))
    {
        hr = pGraph->RemoveFilter(pRenderer);
        *pbRemoved = TRUE;
    }

    return hr;
}

HRESULT IsPinConnected(IPin *pPin, BOOL *pResult)
{
    IPin *pTmp = NULL;
    HRESULT hr = pPin->ConnectedTo(&pTmp);
    if (SUCCEEDED(hr))
    {
        *pResult = TRUE;
    }
    else if (hr == VFW_E_NOT_CONNECTED)
    {
        // The pin is not connected. This is not an error for our purposes.
        *pResult = FALSE;
        hr = S_OK;
    }

    SafeRelease(&pTmp);
    return hr;
}

HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult)
{
    PIN_DIRECTION pinDir;
    HRESULT hr = pPin->QueryDirection(&pinDir);
    if (SUCCEEDED(hr))
    {
        *pResult = (pinDir == dir);
    }
    return hr;
}

HRESULT _FindConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, 
    IPin **ppPin)
{
    *ppPin = NULL;

    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;

    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }

    BOOL bFound = FALSE;
    while (S_OK == pEnum->Next(1, &pPin, NULL))
    {
        BOOL bIsConnected;
        hr = IsPinConnected(pPin, &bIsConnected);
        if (SUCCEEDED(hr))
        {
            if (bIsConnected)
            {
                hr = IsPinDirection(pPin, PinDir, &bFound);
            }
        }

        if (FAILED(hr))
        {
            pPin->Release();
            break;
        }
        if (bFound)
        {
            *ppPin = pPin;
            break;
        }
        pPin->Release();
    }

    pEnum->Release();

    if (!bFound)
    {
        hr = VFW_E_NOT_FOUND;
    }
    return hr;
}

HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, 
    IBaseFilter **ppF, LPCWSTR wszName)
{
    *ppF = 0;

    IBaseFilter *pFilter = NULL;
    
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, 
        IID_PPV_ARGS(&pFilter));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pGraph->AddFilter(pFilter, wszName);
    if (FAILED(hr))
    {
        goto done;
    }

    *ppF = pFilter;
    (*ppF)->AddRef();

done:
    SafeRelease(&pFilter);
    return hr;
}

