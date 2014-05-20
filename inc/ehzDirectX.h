//   +-------------------------------------------+
//    ehzDirectX.h
//
//								by Ferrà srl 2011
//   +-------------------------------------------+

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#define WC_EH_DIRECTSHOW "ehDirectShow"

template<class Interface>
inline void SafeRelease(
    Interface **ppInterfaceToRelease
    )
{
    if (*ppInterfaceToRelease != NULL)
    {
        (*ppInterfaceToRelease)->Release();

        (*ppInterfaceToRelease) = NULL;
    }
}
#define WC_EH_DIRECTX "ehDirectX"

typedef struct {

	ID2D1Factory * piD2DFactory;				// Interfaccia Direct2
	IDWriteFactory * piDWriteFactory;			// Write Factory
	IWICImagingFactory * piWicImageFactory;		// Interfaccia WIC
	ID2D1HwndRenderTarget * piRender; // Render Targhet

	
} S_DX_RESOURCE;

typedef struct {
	HWND hWnd;	// Finestra create
	S_DX_RESOURCE * pRes;	// Puntatore alle risorse condivise del directX
	EH_OBJ * psObj;
	LRESULT (*funcNotify)(EH_NOTIFYPARAMS);

} EHZ_DIRECTX;


typedef struct {

	CHAR 	szName[256];

	IWICBitmapDecoder *		pDecoder;
	IWICBitmapFrameDecode * piSource;
	ID2D1Bitmap *			pBitmap;

	SIZE	sizOriginal;
	SIZE	sizDest;
	BOOL	bAlloc;

} S_DX_IMAGE;


S_DX_IMAGE * dxImageFromFile(	S_DX_IMAGE * psImage,		
								CHAR * pszName,
								UTF8 * utfFileName,
								UINT destinationWidth,
								UINT destinationHeight);

S_DX_IMAGE * dxImageFromFile2(  S_DX_IMAGE * psImage,		
							    CHAR * pszName,
								UTF8 * utfFileName,
								UINT destinationWidth,
								UINT destinationHeight);
S_DX_IMAGE * dxImageFromResource(	S_DX_IMAGE * psImage,	
									CHAR * pszNameResource,
									CHAR * pszTypeResource,
									UINT destinationWidth,
									UINT destinationHeight);
void dxImageFree(S_DX_IMAGE * psImage,BOOL bFree);
void dxImagePush(S_DX_IMAGE * psImage);
S_DX_IMAGE * dxImageGet(CHAR *pszName);
BOOL dxImageDraw(D2D1_POINT_2F center,CHAR *pszName,FLOAT fOpacity);
BOOL dxGetBitmap(S_DX_IMAGE * psImage);

#ifdef __cplusplus
extern "C" {
#endif

void * ehzDirectX(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);

#ifdef __cplusplus
}
#endif