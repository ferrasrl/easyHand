//   -----------------------------------------------------------
//   | ehzDirectX
//   | SDK per l'uso di DirectX
//   |                                              
//   |										by Ferrà Srl 2011
//   -----------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehzDirectX.h"
#include "/easyhand/ehtool/imgutil.h"

static HWND _createDirectX(HINSTANCE hInstance, HWND hwndParent,SINT cx,SINT cy);
static void _LInitialize(void);
LRESULT CALLBACK _funcDirectXControl(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);


static struct {

	BOOL			bReady;
	S_DX_RESOURCE	sRes;
	_DMI			dmiImg;	// Immagini allocate in memoria
	S_DX_IMAGE *	arpImg;	
	BOOL			bValidate;


} _dx={0};

static void _dxFreeResource(BOOL bMode);

static void _LInitialize(void) {

	WNDCLASSEX	wc;
	HRESULT hr;

	if (_dx.bReady) return;

	ZeroFill(_dx);
	//
	// Creo la D2D Factory
	//
//	hr=D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,&_dx.sRes.piD2DFactory);
	hr=D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,&_dx.sRes.piD2DFactory);
	if (hr!=S_OK) osError(true,0,"D2D1CreateFactory");

	//
	// Create WIC factory. ( Gestore delle immagini)
	//
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&_dx.sRes.piWicImageFactory)
		);
	if (hr!=S_OK) osError(true,0,"CLSID_WICImagingFactory");
	// FreeGlobalBitmaps()

	//
	// Testo
	//
	DWriteCreateFactory(	DWRITE_FACTORY_TYPE_SHARED,
							__uuidof(IDWriteFactory),
							reinterpret_cast<IUnknown**>(&_dx.sRes.piDWriteFactory)
						);
	// Creo la finestra di controllo
	ZeroFill(wc);
	wc.cbSize        = sizeof(wc);
	wc.lpszClassName = WC_EH_DIRECTX;
	wc.style         = 0;//CS_BYTEALIGNCLIENT| CS_HREDRAW | CS_VREDRAW; // | CS_PARENTDC;

	//
	// Funzione principale del Form
	//
	wc.lpfnWndProc   = _funcDirectXControl;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;//CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	wc.lpszMenuName  = NULL;
	wc.hIconSm       = NULL;//LoadIcon(sys.EhWinInstance,MAKEINTRESOURCE(IDI_ICOMARE));
	RegisterClassEx(&wc);

	DMIReset(&_dx.dmiImg);
	DMIOpen(&_dx.dmiImg,M_AUTO,50,sizeof(S_DX_IMAGE),"dxImage");
	_dx.arpImg=(S_DX_IMAGE *) DMILock(&_dx.dmiImg,NULL);

	_dx.bReady=TRUE;
	ehAddExit(_dxFreeResource);

}

//
// _dxFreeResource() - Libera le risorse in uscita
//
static void _dxFreeResource(BOOL bMode) {

	INT a;
	if (!_dx.bReady) return;

	for (a=0;a<_dx.dmiImg.Num;a++) {
		dxImageFree(&_dx.arpImg[a],FALSE);
	}
	SafeRelease(&_dx.sRes.piD2DFactory);
	SafeRelease(&_dx.sRes.piDWriteFactory);
	DMIClose(&_dx.dmiImg,"dxImage");

	_dx.bReady=FALSE;

}

// 
// ehzDirectX()
//
void * ehzDirectX(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr)
{
	EH_DISPEXT * psExt=(EH_DISPEXT *) ptr;
	EHZ_DIRECTX * psDx;
	HRESULT hr;
	RECT rcWin;
	if (!objCalled) return NULL; // 

	if (!_dx.bReady) _LInitialize();

	psDx=(EHZ_DIRECTX *) objCalled->pOther;
	switch(cmd)
	{
		case WS_INF: return psDx;

		case WS_CREATE: 

			objCalled->pOther=ehAllocZero(sizeof(EHZ_DIRECTX));
			psDx=(EHZ_DIRECTX *) objCalled->pOther;
			psDx->hWnd=objCalled->hWnd=_createDirectX(sys.EhWinInstance,WindowNow(),objCalled->col1,objCalled->col2);
			psDx->pRes=&_dx.sRes;

			GetWindowRect(psDx->hWnd,&rcWin);
			//D2D1_SIZE_U size = D2D1::SizeU(rcWin.right - rcWin.left,rcWin.bottom - rcWin.top);

			hr = _dx.sRes.piD2DFactory->CreateHwndRenderTarget(
					D2D1::RenderTargetProperties(),
					D2D1::HwndRenderTargetProperties(psDx->hWnd, D2D1::SizeU(rcWin.right - rcWin.left,rcWin.bottom - rcWin.top)),
					&_dx.sRes.piRender
				);

			SetWindowLong(objCalled->hWnd,GWL_USERDATA,(LONG) psDx);
			psDx->psObj=objCalled;
			break;

		case WS_DESTROY:
			if (psDx->funcNotify) psDx->funcNotify(psDx->psObj,WS_DESTROY,0,NULL);
			SafeRelease(&_dx.sRes.piRender);
			DestroyWindow(objCalled->hWnd);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,psExt->px+relwx,psExt->py+relwy,psExt->lx,psExt->ly,TRUE);
			break;

		case WS_EXTFUNZ:
			psDx->funcNotify=(void * (*)(EH_OBJPARAMS)) ptr;
			if (psDx->funcNotify) psDx->funcNotify(psDx->psObj,WS_CREATE,0,NULL);
			break;

		case WS_REALSET: 
			_dx.bValidate=info;
			InvalidateRect(psDx->hWnd,NULL,FALSE);
			SetFocus(psDx->hWnd);
			break;

		case WS_DISPLAY: 
			break;

	}
	return NULL;
}

static HWND _createDirectX(HINSTANCE hInstance, HWND hwndParent,SINT cx,SINT cy)
{
	DWORD       dwStyle;
	HWND        hwndEditText;
	BOOL        bSuccess = TRUE;

	dwStyle=WS_VISIBLE| WS_CHILD;// | WS_VSCROLL;
            
	hwndEditText = CreateWindowEx(   0,//WS_EX_CONTROLPARENT,          // ex style
									  WC_EH_DIRECTX,               // class name - defined in commctrl.h
									  "",                       // dummy text
									  dwStyle,                   // style
									  0,                         // x position
									  0,                         // y position
									  cx,                         // width
									  cy,                         // height
									  hwndParent,                // parent
									  (HMENU) 1010,        // ID
									  sys.EhWinInstance,                   // instance
									  NULL);                     // no extra data
	if (!hwndEditText) ehError();
	return hwndEditText;
}


EN_EHEVENT ehOsEventTranslate(INT iEvent) {

#ifdef __windows__

	switch (iEvent) {
		case WM_MOUSEMOVE:		return EE_MOUSEMOVE;
		case WM_LBUTTONDOWN:	return EE_LBUTTONDOWN;
		case WM_RBUTTONDOWN:	return EE_RBUTTONDOWN;
		case WM_LBUTTONUP:		return EE_LBUTTONUP;
		case WM_RBUTTONUP:		return EE_LBUTTONUP;
		case WM_LBUTTONDBLCLK:	return EE_LBUTTONDBLCLK;
		case WM_RBUTTONDBLCLK:	return EE_RBUTTONDBLCLK;
		case WM_MOUSEWHEEL:		return EE_MOUSEWHEEL;

	}
#else
#error Not defined
#endif
	return EE_NONE;
}

LRESULT CALLBACK _funcDirectXControl(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	EHZ_DIRECTX * psDx;
	EH_EVENT sEvent;

// 	return(DefWindowProc(hWnd, message, wParam, lParam));

	psDx=(EHZ_DIRECTX *) GetWindowLong(hWnd,GWL_USERDATA);
	switch (message)
	{
		case WM_PAINT:

			if (psDx->funcNotify) {
				psDx->funcNotify(psDx->psObj,WS_DISPLAY,0,NULL);
				if (_dx.bValidate) 
					ValidateRect(hWnd, NULL);
				return 0;
			}
			break;
			

		case WM_SIZE:
			{
				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);
				if (psDx->funcNotify) psDx->funcNotify(psDx->psObj,WS_SIZE,lParam,NULL);
				if (_dx.sRes.piRender) _dx.sRes.piRender->Resize(D2D1::SizeU(width, height));
			}
			break;

		case WM_DISPLAYCHANGE:
			InvalidateRect(hWnd, NULL, FALSE);
			break;

		//
		// Tastiera
		//
		case WM_CHAR:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
			I_KeyTraslator_Windows(WindowNow(),message,lParam,wParam);
			break;

		//
		// Controllo del mouse
		//
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			SetFocus(hWnd);

		case WM_MOUSEMOVE:
		case WM_RBUTTONUP:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:

			if (!psDx->funcNotify) break;

			ZeroFill(sEvent);
			sEvent.iEvent=ehOsEventTranslate(message);
			sEvent.hWnd=hWnd;
			sEvent.sPoint.x=psDx->psObj->sClientCursor.x=LOWORD(lParam);
			sEvent.sPoint.y=psDx->psObj->sClientCursor.y=HIWORD(lParam);
			psDx->funcNotify(psDx->psObj,WS_EVENT,0,&sEvent);			  
			//return 0;
			break;

		case WM_MOUSEWHEEL:
		{
			WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
			short zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;

			ZeroFill(sEvent);
			sEvent.iEvent=ehOsEventTranslate(message);
			sEvent.hWnd=hWnd;

			sEvent.sPoint.x= GET_X_LPARAM(lParam);
			sEvent.sPoint.y= GET_Y_LPARAM(lParam);
			sEvent.dwParam=(DWORD) wParam;
			sEvent.iParam=zDelta;
			psDx->funcNotify(psDx->psObj,WS_EVENT,0,&sEvent);
	//		if (zDelta<0) {sEvent.iEvent=EE_MOUSEWHEELDOWN; EhEvent(WS_ADD,0,&sEvent);} //MouseWheelManager(WS_ADD,IN_MW_DOWN);
	//		if (zDelta>0) {sEvent.iEvent=EE_MOUSEWHEELUP; EhEvent(WS_ADD,0,&sEvent);} //MouseWheelManager(WS_ADD,IN_MW_UP);
		}
		break;

	}
	return(DefWindowProc(hWnd, message, wParam, lParam));
}

//
// dxGetBitmap() Trasforma un Decodere in un Bitmap Direct X
//
BOOL dxGetBitmap(S_DX_IMAGE * psImage) {

	BOOL bRet=TRUE;
	HRESULT hr;
	IWICImagingFactory * pIWICFactory=_dx.sRes.piWicImageFactory;

//	IWICBitmapFrameDecode * piSource=psImage->piSource;
    IWICFormatConverter *	piConverter=NULL;
    IWICBitmapScaler *		piScaler=NULL;

	psImage->pBitmap=NULL;

	while (true) {

		//
		// Ritorna il sorgente del file
		//
		if (!psImage->piSource) {
			hr = psImage->pDecoder->GetFrame(0, &psImage->piSource); if (!SUCCEEDED(hr)) break;
		}

		//
		// CreateFormatConverter() - Crea un istanza del format Converter
		//
		// Convert the image format to 32bppPBGRA
		// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
		hr = pIWICFactory->CreateFormatConverter(&piConverter); if (!SUCCEEDED(hr)) break;

		//
		// Legge dimensioni originali
		//
		hr = psImage->piSource->GetSize((UINT *) &psImage->sizOriginal.cx,(UINT *)  &psImage->sizOriginal.cy); if (!SUCCEEDED(hr)) break;

		//
		// If a new width or height was specified, create an
		// IWICBitmapScaler and use it to resize the image.
		//
		if (psImage->sizDest.cx!= 0 || psImage->sizDest.cy != 0) {

			if (psImage->sizDest.cx == 0)
			{
				FLOAT scalar = static_cast<FLOAT>(psImage->sizDest.cy) / static_cast<FLOAT>(psImage->sizOriginal.cy);
				psImage->sizDest.cx = static_cast<UINT>(scalar * static_cast<FLOAT>(psImage->sizOriginal.cx));
			}
			else if (psImage->sizDest.cy == 0)
			{
				FLOAT scalar = static_cast<FLOAT>(psImage->sizDest.cx) / static_cast<FLOAT>(psImage->sizOriginal.cx);
				psImage->sizDest.cy = static_cast<UINT>(scalar * static_cast<FLOAT>(psImage->sizOriginal.cy));
			}

			// Crea un istanza per scalare l'immagine
			hr = pIWICFactory->CreateBitmapScaler(&piScaler); if (!SUCCEEDED(hr)) break;

			// Ricampiona in modo bicubico
			hr = piScaler->Initialize(	psImage->piSource,
										psImage->sizDest.cx,
										psImage->sizDest.cy,
										WICBitmapInterpolationModeCubic);
			if (!SUCCEEDED(hr)) break;

			hr = piConverter->Initialize(	piScaler,
											GUID_WICPixelFormat32bppPBGRA,
											WICBitmapDitherTypeNone,
											NULL,
											0.f,
											WICBitmapPaletteTypeMedianCut
										);
			if (!SUCCEEDED(hr)) break;
		}
		else {
	                    
			hr = piConverter->Initialize(	psImage->piSource,
											GUID_WICPixelFormat32bppPBGRA,
											WICBitmapDitherTypeNone,
											NULL,
											0.f,
											WICBitmapPaletteTypeMedianCut
										);
			if (!SUCCEEDED(hr)) break;
			memcpy(&psImage->sizDest,&psImage->sizOriginal,sizeof(SIZE));
		}

		//
		// Create a Direct2D bitmap from the WIC bitmap.
		//
		hr = _dx.sRes.piRender->CreateBitmapFromWicBitmap(	piConverter,
															NULL,
															&psImage->pBitmap
															);
		if (SUCCEEDED(hr)) bRet=FALSE;
		break;

	}	

	SafeRelease(&piScaler);
	SafeRelease(&piConverter);
//	SafeRelease(&piSource);

	return bRet;
}

//
// dxImageFromFile()
//
S_DX_IMAGE * dxImageFromFile(	S_DX_IMAGE * psImage,	
								CHAR * pszName,
								UTF8 * utfFileName,
								UINT destinationWidth,
								UINT destinationHeight)
{
	
	
	IWICImagingFactory * pIWICFactory=_dx.sRes.piWicImageFactory;
//	IWICBitmapDecoder * pDecoder=NULL;
	BOOL bError=TRUE;

	WCHAR wcsFile[1024];
	HRESULT hr;

	if (!psImage) psImage=(S_DX_IMAGE *) ehAlloc(sizeof(S_DX_IMAGE));
	memset(psImage,0,sizeof(S_DX_IMAGE));
	wcsCpyUtf(wcsFile,utfFileName,sizeof(wcsFile));

	//
	// CreateDecoderFromFilename() - Crea una nuova istanza del bitmap decoder sul file wcsFile
	//
    hr = pIWICFactory->CreateDecoderFromFilename(	wcsFile,
													NULL,
													GENERIC_READ,
													WICDecodeMetadataCacheOnLoad,
													&psImage->pDecoder
												);
	if (SUCCEEDED(hr)) {

		psImage->sizDest.cx=destinationWidth;
		psImage->sizDest.cy=destinationHeight;

		if (pszName) strCpy(psImage->szName,pszName,sizeof(psImage->szName));
		bError=dxGetBitmap(psImage); 

	} else {

		printf("> %s non esiste",utfFileName); 
	}

	SafeRelease(&psImage->pDecoder);
	SafeRelease(&psImage->piSource);

	if (bError) {
		
		dxImageFree(psImage,TRUE);
		psImage=NULL;

	}
    return psImage;//pBitmap;
}

//
// dxImageFromFile()
//
/*
S_DX_IMAGE * dxImageFromFile2(	CHAR * pszName,
								UTF8 * utfFileName,
								UINT uiWidth,
								UINT uiHeight)
{
	INT iFact;
	SIZE	sizSource;
	SIZE	sizDest={uiWidth,uiHeight};
	IMGHEADER sImg,*psImg;
	S_DX_IMAGE * psImage;//=(S_DX_IMAGE *) ehAllocZero(sizeof(S_DX_IMAGE));
	INT		hdlImage;
	HRESULT hr;

	D2D1_SIZE_U d2Size;
	D2D1_BITMAP_PROPERTIES d2Bitmap;
	
	if (!JPGReadHeader(utfFileName,&sImg,0)) return NULL;
	sizSource.cx=sImg.bmiHeader.biWidth;
	sizSource.cy=sImg.bmiHeader.biHeight;
	iFact=JPGGetFactor(&sizSource,&sizDest,NULL);	// x:100=sSize.cx:lpImageData->sDim.cx (fattore di scala )
	
	if (!JPGReadFileEx(utfFileName,&hdlImage,NULL,iFact,JDCT_IFAST,NULL,JME_HIDE,IMG_PIXEL_RGB))
	{
		return NULL;
	}
	
	psImage=(S_DX_IMAGE *) ehAllocZero(sizeof(S_DX_IMAGE));
	psImage->pszName=strDup(pszName);
	memcpy(&psImage->sizOriginal,&sizSource,sizeof(RECT));
	memcpy(&psImage->sizDest,&sizDest,sizeof(RECT));
	d2Size.width=sizDest.cx;
	d2Size.height=sizDest.cy;
	psImg=(IMGHEADER *) memoLock(hdlImage);
	d2Bitmap.pixelFormat.alphaMode=D2D1_ALPHA_MODE_IGNORE;
	d2Bitmap.pixelFormat.format=DXGI_FORMAT_B8G8R8A8_UNORM;
	d2Bitmap.dpiX=96; d2Bitmap.dpiY=96;
	hr = _dx.sRes.piRender->CreateBitmap(	d2Size,psImg->pbImage,psImg->linesize,d2Bitmap,&psImage->pBitmap);
	memoFree(hdlImage,"img");
	return psImage;

}
*/

//
// dxImageFromResource()
//
S_DX_IMAGE * dxImageFromResource(	S_DX_IMAGE * psImage,
									CHAR * pszResourceName,
									CHAR * pszResourceType,
									UINT destinationWidth,
									UINT destinationHeight)
{

	IWICStream * pStream = NULL;
	//S_DX_IMAGE * psImage=(S_DX_IMAGE *) ehAllocZero(sizeof(S_DX_IMAGE));

//	IWICBitmapDecoder * pDecoder=NULL;
	HRSRC	imageResHandle = NULL;
	HGLOBAL imageResDataHandle = NULL;
	void *	pImageFile = NULL;
	DWORD	imageFileSize = 0;
	HRESULT hr=0;
	BOOL	bError;

	memset(psImage,0,sizeof(S_DX_IMAGE));

	while (true) {
		
		if (!_dx.sRes.piWicImageFactory) break;

		// Locate the resource.
		imageResHandle = FindResource(sys.EhWinInstance, pszResourceName, pszResourceType); 
		hr = imageResHandle ? S_OK : E_FAIL; if (!SUCCEEDED(hr)) break;

		
		// Load the resource.
		imageResDataHandle = LoadResource(sys.EhWinInstance, imageResHandle);
		hr = imageResDataHandle ? S_OK : E_FAIL; if (!SUCCEEDED(hr)) break;

		// Lock it to get a system memory pointer.
		pImageFile = LockResource(imageResDataHandle);
		hr = pImageFile ? S_OK : E_FAIL; if (!SUCCEEDED(hr)) break;

		// Calculate the size.
		imageFileSize = SizeofResource(sys.EhWinInstance, imageResHandle);
		hr = imageFileSize ? S_OK : E_FAIL; if (!SUCCEEDED(hr)) break;
	        
		// Create a WIC stream to map onto the memory.
		hr = _dx.sRes.piWicImageFactory->CreateStream(&pStream); if (!SUCCEEDED(hr)) break;

		// Initialize the stream with the memory pointer and size.
		hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile),imageFileSize); if (!SUCCEEDED(hr)) break;

		// Create a decoder for the stream.
		hr = _dx.sRes.piWicImageFactory->CreateDecoderFromStream(		pStream,
														NULL,
														WICDecodeMetadataCacheOnLoad,
														&psImage->pDecoder); 
		if (!SUCCEEDED(hr)) break;

		psImage->sizDest.cx=destinationWidth;
		psImage->sizDest.cy=destinationHeight;
		strCpy(psImage->szName,pszResourceName,sizeof(psImage->szName));
		bError=dxGetBitmap(psImage); //else bError=true;
		break;
	}

	SafeRelease(&psImage->pDecoder);
	SafeRelease(&psImage->piSource);
	SafeRelease(&pStream);
    return psImage;
}


/*


	// Create the initial frame.
    hr = psImage->pDecoder->GetFrame(0, &piSource);
    }
    if (SUCCEEDED(hr))
    {
        // Convert the image format to 32bppPBGRA
        // (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
        hr = pIWICFactory->CreateFormatConverter(&piConverter);
    }
    if (SUCCEEDED(hr))
    {
        // If a new width or height was specified, create an
        // IWICBitmapScaler and use it to resize the image.
        if (destinationWidth != 0 || destinationHeight != 0)
        {
            UINT originalWidth, originalHeight;
            hr = piSource->GetSize(&originalWidth, &originalHeight);
            if (SUCCEEDED(hr))
            {
                if (destinationWidth == 0)
                {
                    FLOAT scalar = static_cast<FLOAT>(destinationHeight) / static_cast<FLOAT>(originalHeight);
                    destinationWidth = static_cast<UINT>(scalar * static_cast<FLOAT>(originalWidth));
                }
                else if (destinationHeight == 0)
                {
                    FLOAT scalar = static_cast<FLOAT>(destinationWidth) / static_cast<FLOAT>(originalWidth);
                    destinationHeight = static_cast<UINT>(scalar * static_cast<FLOAT>(originalHeight));
                }

                hr = pIWICFactory->CreateBitmapScaler(&piScaler);
                if (SUCCEEDED(hr))
                {
                    hr = piScaler->Initialize(
                            piSource,
                            destinationWidth,
                            destinationHeight,
                            WICBitmapInterpolationModeCubic
                            );
                    if (SUCCEEDED(hr))
                    {
                        hr = piConverter->Initialize(
                            piScaler,
                            GUID_WICPixelFormat32bppPBGRA,
                            WICBitmapDitherTypeNone,
                            NULL,
                            0.f,
                            WICBitmapPaletteTypeMedianCut
                            );
                    }
                }
            }
        }
        else
        {
                    
            hr = piConverter->Initialize(
                piSource,
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.f,
                WICBitmapPaletteTypeMedianCut
                );
        }
    }
    if (SUCCEEDED(hr))
    {
        //create a Direct2D bitmap from the WIC bitmap.
        hr = pRenderTarget->CreateBitmapFromWicBitmap(
            piConverter,
            NULL,
			&psImage->pBitmap
            );
    
    }
*/

//
// dxImageFree()
//
void dxImageFree(S_DX_IMAGE * psImage,BOOL bFree) {

	if (!psImage) return;
	SafeRelease(&psImage->piSource);
	SafeRelease(&psImage->pDecoder);
	SafeRelease(&psImage->pBitmap);
	if (bFree) ehFree(psImage);
	
}

//
// dxImageDraw()
//
void dxImagePush(S_DX_IMAGE * psImage) {

	if (!psImage) ehError();
	DMIUnlock(&_dx.dmiImg);
	DMIAppendDyn(&_dx.dmiImg,psImage);
	_dx.arpImg=(S_DX_IMAGE *) DMILock(&_dx.dmiImg,NULL);

}

//
// dxImageDraw()
//
S_DX_IMAGE * dxImageGet(CHAR *pszName) {
	
	INT a,idx=-1;
	if (!_dx.arpImg) return NULL;
	for (a=0;a<_dx.dmiImg.Num;a++) {
		
		if (!strcmp(_dx.arpImg[a].szName,pszName)) return &_dx.arpImg[a];

	}
	return NULL;
}

//
// dxImageDraw()
//
BOOL dxImageDraw(D2D1_POINT_2F pnt,CHAR *pszName,FLOAT fOpacity) {
	
	D2D1_RECT_F rectDest;
	S_DX_IMAGE * psImage=dxImageGet(pszName);
	if (!psImage) {printf("dxImageDraw: %s ?",pszName); return TRUE;}

	rectDest.left=pnt.x; rectDest.top=pnt.y;
	rectDest.right=rectDest.left+psImage->sizDest.cx-1;
	rectDest.bottom=rectDest.top+psImage->sizDest.cy-1;
	_dx.sRes.piRender->DrawBitmap(psImage->pBitmap,rectDest,fOpacity);

	return FALSE;
}
						