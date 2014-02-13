//   ---------------------------------------------
//     IMGutil  Image Utility                    
//              Lettura e scrittura delle        
//              immagini                         
//                 
//                by Ferrà Art & Technology 2004                               
//                 8 Maggio 2003 (JPG)
//                 7 Luglio 2004 (PNG)
//                 
//                 by Ferrà srl 2005 
//  
//   ---------------------------------------------
//   Aggiunge EH_PNG come macro nei "Preprocessor definitions" per avere le librerie PNG

#include "/easyhand/inc/easyhand.h"
#include <setjmp.h>
#include "/easyhand/ehtool/imgutil.h"
//#include "Wingdi.h"

#ifndef _WIN32_WCE

	#ifndef EH_MEMO_DEBUG
		#pragma message("--> Includo /easyhand/lib/win32/ehImg9.lib <-------------------")
		#pragma comment(lib, "/easyhand/lib/win32/ehImg9.lib")
	#else
		#pragma message("--> Includo /easyhand/lib/win32/ehImg9_dm.lib <-------------------")
		#pragma comment(lib, "/easyhand/lib/win32/ehImg9_dm.lib")
	#endif

#endif

#ifdef _PNGLIB
#error ATTENZIONE: Usare EH_PNG in sistituzione di _PNGLIB
#endif

// PER IL PNG
#ifdef EH_PNG
#include "/easyhand/inc/png.h"
#include "/easyhand/inc/cexcept.h"
#pragma message("--> Includo /easyhand/lib/win32/ehPng9.lib <-------------------")
#pragma comment(lib, "/easyhand/lib/win32/ehPng9.lib")

define_exception_type(const char *);
extern struct exception_context the_exception_context[1];
struct exception_context the_exception_context[1];
png_const_charp msg;
#endif

static INT iIMGMode=IMG_SINGLETHREAD;
static CRITICAL_SECTION csJpg;

#ifdef EH_PNG
  static CRITICAL_SECTION csPng;
#endif

void IMG_Mode(INT cmd,INT iMode)
{
	switch (cmd)
	{
		case WS_OPEN:
			InitializeCriticalSection(&csJpg);

#ifdef EH_PNG
			InitializeCriticalSection(&csPng);
#endif
			iIMGMode=iMode;
			break;
		
		case WS_CLOSE:
			DeleteCriticalSection(&csJpg);

#ifdef EH_PNG
			DeleteCriticalSection(&csPng);
#endif
			break;
	}
}

INT _BMPReadService(TCHAR *imageFileName, IMGHEADER *psImgHead ,BOOL bOnlyHeader);

#if (!defined(_NODC)&&defined(__windows__))

//
// imgBitmapInfoCreate() - Crea una struttura di BitmapInfo
//
void * imgBitmapInfoCreate(IMGHEADER *psImgHeader) {
			
	INT	a,iColor;
	void *pRet;
	iColor=0; if (psImgHeader->bmiHeader.biBitCount<=8) iColor=1<<psImgHeader->bmiHeader.biBitCount;	

	if (psImgHeader->enPixelType==IMG_PIXEL_RGB) {
	
		BITMAPV5HEADER *psHeader;

		//
		// Creo un bitmap con l'alpha channel 
		//
		psHeader=ehAllocZero(sizeof(BITMAPV5HEADER));
		psHeader->bV5Size           = sizeof(BITMAPV5HEADER);
		psHeader->bV5Width           = psImgHeader->bmiHeader.biWidth;
		psHeader->bV5Height          = psImgHeader->bmiHeader.biHeight;
		psHeader->bV5Planes = 1;
		psHeader->bV5BitCount = 24;
		psHeader->bV5Compression = BI_RGB;//BI_BITFIELDS;
		pRet=psHeader;

	}
	else if (psImgHeader->enPixelType==IMG_PIXEL_BGR) {
	
		BITMAPV5HEADER *psHeader;

		//
		// Creo un bitmap con l'alpha channel 
		//
		psHeader=ehAllocZero(sizeof(BITMAPV5HEADER));
		psHeader->bV5Size           = sizeof(BITMAPV5HEADER);
		psHeader->bV5Width           = psImgHeader->bmiHeader.biWidth;
		psHeader->bV5Height          = psImgHeader->bmiHeader.biHeight;
		psHeader->bV5Planes = 1;
		psHeader->bV5BitCount = 24;
		psHeader->bV5Compression = BI_RGB;//BI_BITFIELDS;
		pRet=psHeader;

	}
	else if (psImgHeader->enPixelType==IMG_PIXEL_RGB_ALPHA) {

		BITMAPV5HEADER *psHeader;

		//
		// Creo un bitmap con l'alpha channel 
		//
		psHeader=ehAllocZero(sizeof(BITMAPV5HEADER));
		psHeader->bV5Size           = sizeof(BITMAPV5HEADER);
		psHeader->bV5Width           = psImgHeader->bmiHeader.biWidth;
		psHeader->bV5Height          = psImgHeader->bmiHeader.biHeight;
		psHeader->bV5Planes = 1;
		psHeader->bV5BitCount = 32;
		psHeader->bV5Compression = BI_BITFIELDS;

		// The following mask specification specifies a supported 32 BPP
		// alpha format for Windows XP.
		psHeader->bV5RedMask   =  0x00FF0000;
		psHeader->bV5GreenMask =  0x0000FF00;
		psHeader->bV5BlueMask  =  0x000000FF;
		psHeader->bV5AlphaMask =  0xFF000000; 
		pRet=psHeader;
	}
	else {
	
		BITMAPINFO *psBi;

		psBi=ehAllocZero(sizeof(BITMAPINFOHEADER)+iColor*sizeof(RGBQUAD));

		//
		// Creo Header + Palette (se server)
		//
		psBi->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
		psBi->bmiHeader.biWidth=psImgHeader->bmiHeader.biWidth;
		psBi->bmiHeader.biHeight=psImgHeader->bmiHeader.biHeight; 
		psBi->bmiHeader.biPlanes=1;  // Deve essere a 1
		psBi->bmiHeader.biBitCount=psImgHeader->bmiHeader.biBitCount;// bit colore
		psBi->bmiHeader.biSizeImage=0;
		psBi->bmiHeader.biXPelsPerMeter=1;
		psBi->bmiHeader.biYPelsPerMeter=1;
		psBi->bmiHeader.biCompression=BI_RGB;// Non compresso

		// Trascodifico la palette > in RGBQUAD
		if (iColor) {
			psBi->bmiHeader.biClrImportant=iColor;
			psBi->bmiHeader.biClrUsed=iColor;
			for (a=0;a<iColor;a++) {
				psBi->bmiColors[a].rgbBlue=psImgHeader->psRgbPalette[a].b;
				psBi->bmiColors[a].rgbGreen=psImgHeader->psRgbPalette[a].g;
				psBi->bmiColors[a].rgbRed=psImgHeader->psRgbPalette[a].r;
			}
		}
		pRet=psBi;
	
	}
	return pRet;
}

#ifndef EH_CONSOLE

//
// dcImageShow()
//
void dcImageShow(HDC hdc,INT x,INT y,DWORD dwCx,DWORD dwCy,EH_IMG hImage) // New fine 2004/2010 modified
{
	IMGHEADER *psImgHeader;
	HBITMAP hBitmap;
	SIZE	sizTile={1280,1280};
	SIZE	sizImage;
	BOOL	bPrintByBand;
	POINT	ptRead,ptWrite;
	SIZE	sizReadSector,sizWriteSector;
	DWORD	dwTileBlock;
	BYTE	*pbBitmap;
	BITMAPINFO *psBi=NULL;
	INT	iColor;

	if (!hImage) ehError();
	psImgHeader=memoLockEx(hImage,"IMGDispEx"); if (!psImgHeader) return;
	iColor=0; if (psImgHeader->bmiHeader.biBitCount<=8) iColor=1<<psImgHeader->bmiHeader.biBitCount;	
	sizImage.cy=psImgHeader->bmiHeader.biHeight; sizImage.cx=psImgHeader->bmiHeader.biWidth;
	
	// Calcolo automatico delle dimensioni orizzontali
	//  SizeY:sizImg.cy=x:sizImg.cx;
	if ((dwCy>0)&&(dwCx==0)) dwCx=dwCy*sizImage.cx/sizImage.cy;
	// Calcolo automatico delle dimensioni verticali
	if ((dwCx>0)&&(dwCy==0)) dwCy=dwCx*sizImage.cy/sizImage.cx;

	if (!dwCx&&!dwCy) {
		dwCx=sizImage.cx; dwCy=sizImage.cy;
	}
	//
	//	Stampa con Alpha Channel
	//
	if (psImgHeader->enPixelType==IMG_PIXEL_RGB_ALPHA) {
	
		memoUnlock(hImage);
		hBitmap=ImgToBitmap(hImage,NULL,FALSE);
		if (hBitmap) {
		
			dcBmpDispAlpha(	hdc,
							x,y,
							sizImage.cx,sizImage.cy,
							dwCx,dwCy,
							hBitmap,
							0xFF);
			DeleteObject(hBitmap);

		}
		memoLock(hImage);

	}
	//
	//	Stampa RGB (Colori inverti per windows)
	//
	else if (psImgHeader->enPixelType==IMG_PIXEL_RGB) {

		memoUnlock(hImage);
		hBitmap=ImgToBitmap(hImage,NULL,FALSE);
		if (hBitmap) {
		
			dcBmpDisp(	hdc,
						x,y,dwCx,dwCy,
						hBitmap,0);
			DeleteObject(hBitmap);
		}
		memoLock(hImage);

	}
	//
	//  Stampa diretta 
	//
	else {

		psBi=imgBitmapInfoCreate(psImgHeader);
		pbBitmap=psImgHeader->pbImage;

		bPrintByBand=FALSE;
		psBi->bmiHeader.biHeight=-psBi->bmiHeader.biHeight;
		psBi->bmiHeader.biCompression=BI_RGB;
//		hBitmap = CreateDIBSection(hdc, psBi, DIB_RGB_COLORS,  (void **) & pbBitmap, NULL, (DWORD)0);
//		psBi->bmiHeader.biSizeImage=
		hBitmap=CreateDIBitmap(hdc, //Handle del contesto
							   &psBi->bmiHeader,
							   CBM_INIT,
							   pbBitmap,// Dati del'icone
							   psBi,
							   DIB_RGB_COLORS);
/*
		// Creo un bitmap compatibile
		hBitmap=CreateCompatibleBitmap(hdc,psBi->bmiHeader.biWidth,psBi->bmiHeader.biHeight);
		SetDIBits(	hdc,
					hBitmap,
					0,
					psBi->bmiHeader.biHeight,
					pbBitmap,
					psBi,
					DIB_RGB_COLORS);

		{
			BITMAP sBmpInfo;
			GetObject(hBitmap, sizeof(BITMAP), &sBmpInfo);
			printf("qui");
		}
*/

		if (hBitmap) {
			dcBmpDisp(hdc,
					x,y,
					dwCx,
					dwCy,hBitmap,
					0);
			DeleteObject(hBitmap);
		} 
		else 
			bPrintByBand=FALSE; // Provo la stampa a bande (era TRUE)
		
		if (bPrintByBand) {

			// Inversamente proporzionale : y:1024=TileY:TileX;
			// Calcolo dimensione della Band (Striscia di stampa)
			sizTile.cx=sizImage.cx; sizTile.cy=sizTile.cy*1024/sizTile.cx;
			if (dwCx||dwCy) bPrintByBand=TRUE; else bPrintByBand=FALSE;

			// Calcolo automatico delle dimensioni orizzontali
			//  SizeY:sizImg.cy=x:sizImg.cx;
			if ((dwCy>0)&&(dwCx==0)) dwCx=dwCy*sizImage.cx/sizImage.cy;
			// Calcolo automatico delle dimensioni verticali
			if ((dwCx>0)&&(dwCy==0)) dwCy=dwCx*sizImage.cy/sizImage.cx;

			//
			// Loop (Stampa a bande orizzontali)
			//
			_(ptRead); _(ptWrite);
			pbBitmap=psImgHeader->pbImage;
			dwTileBlock=psImgHeader->linesize*sizTile.cy;
			sizWriteSector.cx=dwCx;
			sizReadSector.cx=sizImage.cx;
			for (ptRead.y=0;;ptRead.y+=sizTile.cy)   // Loop Verticale
			{
				sizReadSector.cy=(sizImage.cy-ptRead.y); 
				if (sizReadSector.cy>sizTile.cy) sizReadSector.cy=sizTile.cy; 
				else if (sizReadSector.cy<1) break;		  // Fine del file

				// Calcolo dimensione verticale ridimensionata
				if (bPrintByBand)
				{
					if ((sizReadSector.cy+sizTile.cy)>=sizImage.cy) 
							sizWriteSector.cy=(dwCy-ptWrite.y); 
							else 
							sizWriteSector.cy=dwCy*sizReadSector.cy/sizImage.cy;
				} else sizWriteSector.cy=sizReadSector.cy;
			
		//	 win_infoarg("Py=%d [%d]",Py,Img->linesize*SectY);
				psBi->bmiHeader.biHeight=-sizReadSector.cy;
				psBi->bmiHeader.biWidth=sizReadSector.cx;

				hBitmap=CreateDIBitmap(hdc, //Handle del contesto
									  &psBi->bmiHeader,
									  CBM_INIT,
									  pbBitmap,// Dati del'icone
									  psBi,
									  DIB_RGB_COLORS);

				if (!hBitmap) 
				{
					INT iErr=GetLastError();
					osError(TRUE,iErr,__FUNCTION__": Bitmap Null");
		//				ReleaseDC(NULL,hDC); 
					//ehExit(__FUNCTION__": Bitmap Null");
				}
				else
				{
					dcBmpDisp(hdc,
							x,y+ptWrite.y,
							sizWriteSector.cx,
							sizWriteSector.cy,hBitmap,0);
					DeleteObject(hBitmap);
				}
					
				ptWrite.y+=sizWriteSector.cy;
				pbBitmap+=dwTileBlock;
			}
		}
		ehFree(psBi);
	}
	
	memoUnlock(hImage);
}


void IMGDisp(INT x1,INT y1,INT hImage) {

	HDC hdc;
	HRGN hrgn;

	x1+=relwx; y1+=relwy;
	hdc=UsaDC(WS_OPEN,0);
	hrgn=ClipRgnMake(hdc);

	dcImageShow(hdc,x1,y1,0,0,hImage);

	//if (!fAbsolute)
	dcClipFree(hdc,hrgn);
//	I_Show_Bitmap(x1,y1,x1+sBmpInfo.bmWidth-1,y1+sBmpInfo.bmHeight-1);
	UsaDC(WS_CLOSE,hdc);

}

void IMGDispEx(INT PosX,INT PosY,INT SizeX,INT SizeY,INT OfsX,INT OfsY,INT Hdl)
{
	IMGHEADER *Img;
	HDC hDC;
	WORD    a=1;
	#ifdef _WIN32_WCE
	#else
	HBITMAP BitMap;
	#endif
	BITMAPINFOHEADER sBmpHeaderBackup;
	BITMAPINFOHEADER *psBmpHeader;
	BITMAPINFO *BmpInfo;
	BYTE *Sorg;
	INT TileY=1024;// Dimenzione piastrella verticale
	INT TileX=2048;// Dimenzione piastrella orizzontale (Da Fare)
	LONG Lx,Ly;
	BOOL bPrintByBand;
	INT ReadPy=0,WritePy=0; 
	INT ReadPx=0,WritePx=0; 
	INT iVerticalSector=0,ReadSectX=0; 
	INT WriteSectY=0,WriteSectX=0; 
	LONG dwTileBlock;

	Img=memoLockEx(Hdl,"IMGDispEx"); if (!Img) return;
	Sorg=(BYTE *) Img;
	Sorg+=Img->Offset;

	hDC=GetDC(WindowNow()); // DA VEDERE
	BmpInfo=(BITMAPINFO *) &Img->bmiHeader;
	psBmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
	ehMemCpy(&sBmpHeaderBackup,psBmpHeader,sizeof(sBmpHeaderBackup));

	// -------------------------------------
	// CREA ED STAMPA IL BITMAP            !
	// -------------------------------------

	Ly=psBmpHeader->biHeight; Lx=psBmpHeader->biWidth;

	// Inversamente proporzionale : y:1024=TileY:TileX;
	if (Lx>TileX) {TileX=Lx; TileY=TileY*1024/TileX;}
  
	// Calcolo automatico delle dimensioni orizzontali
	//  SizeY:Ly=x:Lx;
	if ((SizeY>0)&&(SizeX==0)) SizeX=SizeY*Lx/Ly;
	// Calcolo automatico delle dimensioni verticali
	if ((SizeX>0)&&(SizeY==0)) SizeY=SizeX*Ly/Lx;
	if (SizeX) bPrintByBand=TRUE; else bPrintByBand=FALSE;

	WritePy=0; 
	dwTileBlock=Img->linesize*TileY;
	for (ReadPy=0;;ReadPy+=TileY)   // Loop Verticale
	{
		iVerticalSector=(Ly-ReadPy); 
		if (iVerticalSector>TileY) iVerticalSector=TileY; 
		if (iVerticalSector<1) break;		  // Fine del file

		if (bPrintByBand)
		{
		 if ((ReadPy+TileY)>=Ly) WriteSectY=(SizeY-WritePy); else WriteSectY=SizeY*iVerticalSector/Ly;
		}

		WritePx=0;
		for (ReadPx=0;;ReadPx+=TileX) // Loop orizzontale
		{
			// Calcolo la grandezza del settore
			ReadSectX=(Lx-ReadPx); 
			if (ReadSectX>TileX) ReadSectX=TileX; 
			if (ReadSectX<1) break;  // Fine della lettura orizzontale

			if (bPrintByBand)
			{
				//   SizeY:Ly=x:iVerticalSector
				if ((ReadPx+TileX)>=Lx) WriteSectX=(SizeX-WritePx); else WriteSectX=SizeX*ReadSectX/Lx;
				//RealTileX=SizeX*ReadSectX/Lx;
				//WriteSectX=(SizeX-WritePx); if (WriteSectX>RealTileX) WriteSectX=RealTileX;
			}

//	 win_infoarg("Py=%d [%d]",Py,Img->linesize*SectY);
			psBmpHeader->biHeight=-iVerticalSector;
			psBmpHeader->biWidth=ReadSectX;
#ifdef _WIN32_WCE

#else
			BitMap=CreateDIBitmap(hDC, //Handle del contesto
								  psBmpHeader,
								  CBM_INIT,
								  Sorg,// Dati del'icone
								  BmpInfo,
								  DIB_RGB_COLORS);
			if (!BitMap) 
			{
				INT iErr=GetLastError();
				osError(TRUE,iErr,__FUNCTION__": Bitmap Null");
				ReleaseDC(NULL,hDC); 
				//ehExit(__FUNCTION__": Bitmap Null");
			}
			
			DeleteObject(BitMap);
#endif 
			if (bPrintByBand) WritePy+=WriteSectY; else WritePy+=TileY;
		}
		Sorg+=dwTileBlock;
	}
	ReleaseDC(NULL,hDC); 
	ehMemCpy(psBmpHeader,&sBmpHeaderBackup,sizeof(sBmpHeaderBackup));
	memoUnlockEx(Hdl,"IMGDispEx");
}


#endif

//
//	ImgToBitmap()
//  Trasfroma un hdlImage in un hBitmap
// 
#ifdef EH_MOBILE
HBITMAP ImgToBitmap(INT hdlImage)
{
	BYTE *pm;
	IMGHEADER *pImgHead;
	HBITMAP hBitmap;
	BITMAPINFO bitInfo;

	pm=memoLock(hdlImage);
	pImgHead=(IMGHEADER *) pm;
	memcpy(&bitInfo,&pImgHead->bmiHeader,sizeof(pImgHead->bmiHeader));

	{	
		BITMAPFILEHEADER bmfh;
		FILE *ch;
		TCHAR *lpFileTemp=ehAlloc(1024);//=TEXT("\\Programmi\\Gimba\\test.bmp");
		_(bmfh);
		fileTempName(NULL, // Cartella dove crearlo se NULL è quella temporanea standard
				  TEXT("Img"),
				  lpFileTemp, // Nome del file generato
				  TRUE); // Crea il file
//		wcscpy(lpFileTemp,TEXT("\\Programmi\\Gimba\\test.bmp"));
		bmfh.bfType=19778;
		bmfh.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+pImgHead->lBitmapSize; // Da calcolare
		bmfh.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);

	//	win_infoarg("%S",lpFileTemp);
		ch=_tfopen(lpFileTemp,TEXT("wb")); if (!ch) ehExit("error");
		fwrite(&bmfh,sizeof(BITMAPFILEHEADER),1,ch);
		pImgHead->bmiHeader.biHeight*=-1;
		fwrite(&pImgHead->bmiHeader,sizeof(BITMAPINFOHEADER),1,ch);
		pImgHead->bmiHeader.biHeight*=-1;
		fwrite(pm+pImgHead->Offset,pImgHead->lBitmapSize,1,ch);
		fclose(ch);
		hBitmap=SHLoadDIBitmap(lpFileTemp);
		DeleteFile(lpFileTemp);
		ehFree(lpFileTemp);
		return hBitmap;
	}

	return NULL;
}
#endif

#ifndef EH_MOBILE

// ------------------------------------------------------------
//  ImgToBitmap()
//  Converte EH_IMG in un Bitmap di Windows
//  ospite (per ora solo windows)
// 
//
// ------------------------------------------------------------
HBITMAP ImgToBitmap(INT hdlImage,SIZE *psizImage,BOOL bAlphaEver) {

    BITMAPV5HEADER bi;
    HBITMAP hBitmap;
    void *lpBits;
    DWORD x,y;
    HCURSOR hAlphaCursor = NULL;
    HDC hdc;
    DWORD *lpdwPixel,dwColor;

	BOOL bLock=FALSE;
	BYTE *pbSource,*pbDest;
	IMGHEADER *psImgHead;
	INT iColor;

	psImgHead=memoLock(hdlImage);
	iColor=0; if (psImgHead->bmiHeader.biBitCount<=8) iColor=1<<psImgHead->bmiHeader.biBitCount;	

	hdc=CreateCompatibleDC(0);
	if (psImgHead->bmiHeader.biBitCount<=24&&
		!bAlphaEver&&
		psImgHead->enPixelType!=IMG_PIXEL_RGB) {
		
		BITMAPINFO *psBi;
		BITMAP sBitmap;
		psBi=imgBitmapInfoCreate(psImgHead);
		hBitmap = CreateDIBSection(hdc, psBi, DIB_RGB_COLORS,  (void **) &lpBits, NULL, (DWORD)0);
		GetObject(hBitmap, sizeof(BITMAP), &sBitmap);
		ehFree(psBi);

		//
		// Copio sotto sopra
		//
		pbSource=psImgHead->pbImage+(psImgHead->linesize*(psImgHead->bmiHeader.biHeight-1));
		pbDest=lpBits;
		for (y=0;y<(DWORD) psImgHead->bmiHeader.biHeight;y++)
		{
			memcpy(pbDest,pbSource,sBitmap.bmWidthBytes);
			pbDest+=sBitmap.bmWidthBytes;
			pbSource-=psImgHead->linesize;
		}
	}
	else if (psImgHead->enPixelType==IMG_PIXEL_RGB)
	{

		BITMAPINFO *psBi;
		BITMAP sBitmap;
		psBi=imgBitmapInfoCreate(psImgHead);
		hBitmap = CreateDIBSection(hdc, psBi, DIB_RGB_COLORS,  (void **) &lpBits, NULL, (DWORD)0);
		if (!hBitmap) ehError();
		GetObject(hBitmap, sizeof(BITMAP), &sBitmap);
		ehFree(psBi);

		//
		// Copio sotto sopra
		//
		pbSource=psImgHead->pbImage+(psImgHead->linesize*(psImgHead->bmiHeader.biHeight-1));
		pbDest=lpBits;
		for (y=0;y<(DWORD) psImgHead->bmiHeader.biHeight;y++)
		{
//			memcpy(pbDest,pbSource,sBitmap.bmWidthBytes);
//			pb=pbDest;
			BYTE *pbs,*pbd;
			DWORD * ps32, *pd32;
			pbs=pbSource; pbd=pbDest;
			for (x=0;x<(UINT) psImgHead->bmiHeader.biWidth;x++) {
				ps32=(DWORD *) pbs; pd32=(DWORD *) pbd;
				*pd32=	(*ps32&0xFF0000)>>16|
						(*ps32&0x00FF00)|
						(*ps32&0x0000FF)<<16;
				// *ps32?

				pbs+=3; pbd+=3;
			}

			pbDest+=sBitmap.bmWidthBytes;
			pbSource-=psImgHead->linesize;
		}
	}
	else {

		//
		// Creo un bitmap con l'alpha channel 
		//
		_(bi);
		bi.bV5Size           = sizeof(BITMAPV5HEADER);
		bi.bV5Width          = psImgHead->bmiHeader.biWidth;
		bi.bV5Height         = psImgHead->bmiHeader.biHeight;
		bi.bV5Planes = 1;
		bi.bV5BitCount = 32;
		bi.bV5Compression = BI_BITFIELDS;
		if (psizImage) {psizImage->cx=psImgHead->bmiHeader.biWidth; psizImage->cy=psImgHead->bmiHeader.biHeight;}

		// The following mask specification specifies a supported 32 BPP
		// alpha format for Windows XP.
		bi.bV5AlphaMask =  0xFF000000; 
		bi.bV5RedMask   =  0x00FF0000;
		bi.bV5GreenMask =  0x0000FF00;
		bi.bV5BlueMask  =  0x000000FF;
		hBitmap = CreateDIBSection(hdc, (BITMAPINFO *) &bi, DIB_RGB_COLORS,  (void **) &lpBits, NULL, (DWORD)0);

		//
		// Creo il bitmap
		//
		lpdwPixel = (DWORD *) lpBits;
		switch (psImgHead->enPixelType)
		{
			case IMG_PIXEL_COLOR1:
				for (y=0;y<(DWORD) psImgHead->bmiHeader.biHeight;y++)
				{
				   pbSource=psImgHead->pbImage+(psImgHead->linesize*(psImgHead->bmiHeader.biHeight-y-1));
				   for (x=0;x<(DWORD) psImgHead->bmiHeader.biWidth;x++)
				   {
					   
					   //if (!(x&1)) dwColor=sys.arColorBGR[*pSource>>4]; else dwColor=sys.arColorBGR[*pSource&0xf];
					   dwColor=pbSource[x>>3]; //0x80
					   dwColor&=(0x80>>(x&7)); 
					   dwColor=(dwColor)?1:0;
					   dwColor=RGB(psImgHead->psRgbPalette[dwColor].r,psImgHead->psRgbPalette[dwColor].g,psImgHead->psRgbPalette[dwColor].b);
					   *lpdwPixel = dwColor|0xff000000; lpdwPixel++; //if (x&1) pSource++;
				   }
				   //pSource+=psImgHead->linesize;
				}
				break;


			default:
				case IMG_PIXEL_GRAYSCALE:
				case IMG_PIXEL_COLOR8:
				case IMG_PIXEL_CMYK:
				case IMG_PIXEL_YCbCr:
				case IMG_PIXEL_YCCK:
					ehError();

			// 
			// Formato icone 4 bit (Pallet Easyhand Standard) con maskera a 1 bit
			// Trascodifica in 32bit con alpha channel
			//
			case IMG_PIXEL_COLOR4:  // Trascodifica 4bit+mask > (AlphaChannel + 24bit)
					for (y=0;y<(DWORD) psImgHead->bmiHeader.biHeight;y++)
					{
					   pbSource=psImgHead->pbImage+(psImgHead->linesize*(psImgHead->bmiHeader.biHeight-y-1));
					   for (x=0;x<(DWORD) psImgHead->bmiHeader.biWidth;x++)
					   {
						   if (!(x&1)) dwColor=sys.arColorBGR[*pbSource>>4]; else dwColor=sys.arColorBGR[*pbSource&0xf];
						   *lpdwPixel = dwColor|=0xff000000; lpdwPixel++; if (x&1) pbSource++;
					   }
					}
					break;

			// 
			// Formato icone 24 bit con maskera a 1 bit
			// Trascodifica in 32bit con alpha channel
			//
			case IMG_PIXEL_BGR:
			case IMG_PIXEL_RGB:

					for (y=0;y<(DWORD) psImgHead->bmiHeader.biHeight;y++)
					{
						pbSource=psImgHead->pbImage+(psImgHead->linesize*(psImgHead->bmiHeader.biHeight-y-1));
						for (x=0;x<(DWORD) psImgHead->bmiHeader.biWidth;x++)
						{
							*lpdwPixel = (* (DWORD *) pbSource)|0xff000000; lpdwPixel++; pbSource+=3;
					   }
					}
					break;

			// 
			// Formato icone 32 bit senza maskera con alpha-channel
			// Trascodifica in 32bit con alpha channel
			//
			// Se non scopro qualcos'altro Windows ha bisogno di un codifica dei RGB direttamente proporzionale all'alpha channel
			// Non ho ancora capito come mai ...
			//
			case IMG_PIXEL_RGB_ALPHA:
/*
					for (y=0;y<(DWORD) psImgHead->bmiHeader.biHeight;y++)
					{
						DWORD dwPixel;
						BYTE ubAlpha;
						float fAlphaFactor;    // used to do premultiply

						pbSource=psImgHead->pbImage+(psImgHead->linesize*(psImgHead->bmiHeader.biHeight-y-1));
						lpdwSource=(DWORD *) pbSource;
						for (x=0;x<(DWORD) psImgHead->bmiHeader.biWidth;x++)
						{
							// Correzione con alpha factor
							dwPixel=*lpdwSource++;
							ubAlpha=(BYTE) (dwPixel>>24); 
							fAlphaFactor = (float) ubAlpha / (float)0xff;
							
							*lpdwPixel++ = (ubAlpha << 24) |                       //0xaa000000
							 ((UCHAR)(((dwPixel)&0xFF) * fAlphaFactor) << 16) |		//0x00rr0000
							 ((UCHAR)(((dwPixel>>8)&0xFF) * fAlphaFactor) << 8) | //0x0000gg00
							 ((UCHAR)(((dwPixel>>16)&0xFF)   * fAlphaFactor));      //0x000000bb
					   }
					}
					*/
					winBitmapAlphaFactor(	psImgHead->bmiHeader.biWidth,
											psImgHead->bmiHeader.biHeight,
											psImgHead->pbImage,lpBits);
					break;
		}
	}

	//
    // Create the DIB section with an alpha channel.
	//
	memoUnlock(hdlImage);
	DeleteDC(hdc);
	return hBitmap;
}


#endif

//
// BitmapToImg() 
// Attenzione: di default il bitmap è sotto/sopra se la coordinata è positiva
//				Questa funzione è in beta, non è completa !!!!
//
INT BitmapToImg(HBITMAP hBitmap,BOOL bGetBitmap,RECT *prcClip,EN_PIXEL_TYPE enPixelTypeDest) {

	// 
	// Converto il bitmap in EH_IMG
	//
	IMGHEADER * psImgHeaderDest;
	BITMAP sBmpInfo;
//	DWORD dwSizeSource=0;//,dwSizeDest;
	BYTE * pbSorg,* pDest;
	INT hdlImage;
	INT iLineSize;
	BYTE * pbImageStart;

	RECT rcClip;
	SIZE sizClip;
	INT i,a,b;
	EN_PIXEL_TYPE enPixelTypeSource=0;
	INT iBpp;
	LONG lMemo;
	BOOL bAlphaDest;

	//
	// Leggo Bits del sorgente
	//
	GetObject(hBitmap, sizeof(BITMAP), &sBmpInfo);
	pbSorg=sBmpInfo.bmBits;
	
	if (bGetBitmap) {
		
		// Provato su 7(64bit)
		lMemo=GetBitmapBits(hBitmap,0,NULL); if (!lMemo) ehError();
		lMemo=GetBitmapBits(hBitmap,lMemo,pbSorg);
	///	printf("%d",lMemo);
	}

	//
	// Pezzo di bitmap
	//
	if (prcClip) memcpy(&rcClip,prcClip,sizeof(RECT)); else {_(rcClip); rcClip.right=sBmpInfo.bmWidth-1; rcClip.bottom=sBmpInfo.bmHeight-1;}
	sizeCalc(&sizClip,&rcClip);

	//
	// Alloco memoria destinazione (dovrebbe essere del tipo dell bitmap --- da fare!!!)
	//
	switch (sBmpInfo.bmBitsPixel) {

		case 1:		enPixelTypeSource=IMG_PIXEL_COLOR1; break;
		case 4:		enPixelTypeSource=IMG_PIXEL_COLOR4; break;
		case 8:		enPixelTypeSource=IMG_PIXEL_COLOR8; break;
		case 24:	enPixelTypeSource=IMG_PIXEL_BGR; iBpp=3; break;
		case 32:	enPixelTypeSource=IMG_PIXEL_RGB_ALPHA; iBpp=4; break;
		default:
			ehExit("BitmapToImg(): %d ?",sBmpInfo.bmBitsPixel);
	}


	//
	// La destinazione di transito può essere RGB con ALPHA o senza
	// 
	// 
	if (!enPixelTypeDest) enPixelTypeDest=IMG_PIXEL_BGR;
	bAlphaDest=false; if (enPixelTypeDest==IMG_PIXEL_RGB_ALPHA) bAlphaDest=true;
	hdlImage=IMGCreate(enPixelTypeDest,"Memo",sizClip.cx,sizClip.cy,NULL,false);
	psImgHeaderDest=memoLock(hdlImage); pDest=psImgHeaderDest->pbImage;

	switch (enPixelTypeSource) {

		case IMG_PIXEL_COLOR1:
		case IMG_PIXEL_COLOR4:
		case IMG_PIXEL_COLOR8:
			if (prcClip) ehError(); // Clip non gestito ad 1 bit

			//
			// Devo leggere la palette
			//
			//
			// Palette (Provvisorio)
			//
			for (a=0;a<(INT) psImgHeaderDest->dwColors;a++) {
				psImgHeaderDest->psRgbPalette[a].r=a*255;
				psImgHeaderDest->psRgbPalette[a].g=a*255;
				psImgHeaderDest->psRgbPalette[a].b=a*255;
			}

			if (sBmpInfo.bmHeight<0) 
				memcpy(pDest,pbSorg,psImgHeaderDest->dwBitmapSize);
			else {

				BYTE *pd,*ps;
				INT bmWidthBytes;

				// Scrivo sotto sopra
				pd=psImgHeaderDest->pbImage+((sBmpInfo.bmHeight-1)*psImgHeaderDest->linesize);
				ps=pbSorg;
				bmWidthBytes=sBmpInfo.bmWidthBytes;//(sBmpInfo.bmWidth+7)/8; // Calcolo io la riga perché windows me la ritorna errata ???

				for (a=0;a<sBmpInfo.bmHeight;a++) {
				
					memcpy(pd,ps,bmWidthBytes);//psImgHeaderDest->linesize);
					pd-=psImgHeaderDest->linesize; 
					ps+=bmWidthBytes;//psImgHeaderDest->linesize;
				}

			}
			break;

		case IMG_PIXEL_RGB: // Sorgente
		case IMG_PIXEL_BGR:
		
			if (prcClip) {

				BYTE *pd;

				//
				// Sposto i byte nella destinazione
				//
				pbImageStart=pbSorg+(rcClip.left*iBpp)+(rcClip.top*sBmpInfo.bmWidthBytes); // Inizio in base al clip
				//memset(pDest,255,psImgHeaderDest->dwBitmapSize);

				iLineSize=sizClip.cx*iBpp;
				pd=pDest; 
				for (i=0;i<sizClip.cy;i++) {
					memcpy(pd,pbImageStart,iLineSize);//iLineSize); 
					pbImageStart+=sBmpInfo.bmWidthBytes;
					pd+=psImgHeaderDest->linesize;
				}

			}
			else
			{
				//
				// Copio Tutto <--------------------------------------------------------
				//
				BYTE *pd,*ps;
				DWORD * pws32,* pwd32;
				pd=pDest; ps=pbSorg;

				// Con alpha channel
				if (bAlphaDest) {

					for (a=0;a<sBmpInfo.bmHeight;a++) {

						pws32=(int *) ps; pwd32=(int *) pd;
						for (b=0;b<sBmpInfo.bmWidth;b++,pws32++,pwd32++) {

							*pwd32=	0xFF000000|				// <-- Metto opaco (leggo da RGB)
									((*pws32&0xFF)<<16)|
									(*pws32&0xFF00)|
									(*pws32&0xFF0000)>>16;

						}
						pd+=psImgHeaderDest->linesize; 
						ps+=sBmpInfo.bmWidthBytes;//psImgHeaderDest->linesize;
					}

				}
				else
				{
					pd+=(psImgHeaderDest->linesize*(sBmpInfo.bmHeight-1));
					for (a=0;a<sBmpInfo.bmHeight;a++) {
					
						memcpy(pd,ps,psImgHeaderDest->linesize);
						pd-=psImgHeaderDest->linesize; ps+=psImgHeaderDest->linesize;
					}

					//sBmpInfo.bmWidthBytes=sBmpInfo.bmWidth*3;
					//sBmpInfo.bmWidthBytes=(sBmpInfo.bmWidthBytes+1)/2*2;
					/*
					if (psImgHeaderDest->linesize==sBmpInfo.bmWidthBytes) {

						memcpy(pd,ps,psImgHeaderDest->linesize*sBmpInfo.bmHeight);

					}
					else {

						for (a=0;a<sBmpInfo.bmHeight;a++) {
						
							memcpy(pd,ps,psImgHeaderDest->linesize);
//							pd-=psImgHeaderDest->linesize; ps+=psImgHeaderDest->linesize;
							pd+=psImgHeaderDest->linesize; 
							ps+=sBmpInfo.bmWidthBytes;//psImgHeaderDest->linesize;

						}
					}
					*/
					
				}
			}
			break;

		case IMG_PIXEL_RGB_ALPHA: // <---Sorgente

			if (prcClip) {
/*
				BYTE *pd;

				//
				// Sposto i byte nella destinazione
				//
				pbImageStart=pbSorg+(rcClip.left*iBpp)+(rcClip.top*sBmpInfo.bmWidthBytes); // Inizio in base al clip
				//memset(pDest,255,psImgHeaderDest->dwBitmapSize);

				iLineSize=sizClip.cx*iBpp;
				pd=pDest; 
				for (i=0;i<sizClip.cy;i++) {
					memcpy(pd,pbImageStart,iLineSize);//iLineSize); 
					pbImageStart+=sBmpInfo.bmWidthBytes;
					pd+=psImgHeaderDest->linesize;
				}
				*/
				ehError(); // da fare

			}
			else
			{
				//
				// Copio Tutto
				//
				DWORD dwColor;
				BYTE *pd,*ps;
				DWORD * pws32,* pwd32;
				pd=pDest;//+((sBmpInfo.bmHeight-1)*psImgHeaderDest->linesize);
				ps=pbSorg;

				// da RGBA to RGB
				if (!bAlphaDest) {

					pd+=(psImgHeaderDest->linesize*(psImgHeaderDest->bmiHeader.biHeight-1));
					for (a=0;a<sBmpInfo.bmHeight;a++) {
						
						pws32=(DWORD *) ps; pwd32=(DWORD *) pd;
						for (b=0;b<sBmpInfo.bmWidth;b++,pws32++) {
							dwColor=*pws32;
							*pwd32=	//0xFF000000|	// A
									((dwColor&0xFF)<<16)|	// B
									(dwColor&0xFF00)|		// G		
									(dwColor&0xFF0000)>>16; // R
							pwd32=(DWORD *) ((BYTE *) pwd32+3);

						}
						pd-=psImgHeaderDest->linesize; 
						ps+=sBmpInfo.bmWidthBytes;
					}

				}
				// da ABGR to ARGB
				else
				{
					pd+=(psImgHeaderDest->linesize*(psImgHeaderDest->bmiHeader.biHeight-1));
					for (a=0;a<sBmpInfo.bmHeight;a++) {
						
						pws32=(DWORD *) ps; pwd32=(DWORD *) pd;
						for (b=0;b<sBmpInfo.bmWidth;b++,pws32++,pwd32++) {
							dwColor=*pws32;
							*pwd32=	(dwColor&0xFF000000)|	// A
									((dwColor&0xFF)<<16)|	// B
									(dwColor&0xFF00)|		// G		
									(dwColor&0xFF0000)>>16; // R

						}
						pd-=psImgHeaderDest->linesize; 
						ps+=sBmpInfo.bmWidthBytes;
					}


				}
			}

			break;
	}

	memoUnlock(hdlImage);
//	ehFree(pbSorg);

//	if (enPixelTypeDest) hdlImage=IMGConvert(hdlImage,enPixelTypeDest);

	return hdlImage;
}

#endif


//  --------------------------------------------------------------------------------------
//  | FORMATO JPEG                              
//  --------------------------------------------------------------------------------------

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void ima_jpeg_error_exit (j_common_ptr cinfo)
{
	  struct ima_error_mgr  * myerr = (struct ima_error_mgr * ) cinfo->err;
	  CHAR buffer[JMSG_LENGTH_MAX];

	  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	#ifndef EH_CONSOLE
	  HWND hWnd;
	  myerr->pub.format_message (cinfo, buffer);
	  hWnd=GetFocus();
	  win_infoarg("ima_jpeg_error_exit:\n%s\n",buffer);
	  winSetFocus(hWnd);
	#else

	  myerr->pub.format_message (cinfo, buffer);
	  printf("ima_jpeg_error_exit():\n%s\n",buffer);

	#endif

	  /* Send it to stderr, adding a newline */
	  /* Return control to the setjmp point */
	  longjmp(myerr->setjmp_buffer, 1);
	  //return;
	}

	static void ima_jpeg_error_exit_noview(j_common_ptr cinfo)
	{
		struct ima_error_mgr  * myerr = (struct ima_error_mgr * ) cinfo->err;
	#ifdef EH_CONSOLE
		printf("(ima_jpeg_error_exit_noview)");
	#endif
		longjmp(myerr->setjmp_buffer, 1);
	}

	void CreateGrayColourMap(BYTE *Palette,int nGrays)
	{
	  int a;
	  RGBQUAD *pRGB=(RGBQUAD *) Palette;
	  //byte g[256];
	//  for (i=0; i<n; i++) 
		  //g[i] = (byte)(256*i/n); 
		  //for (i=0;i<3;i++) {memcpy(Zone,g,256); Zone+=256;}
	  for (a=0;a<nGrays;a++)
	  {
		 byte col=(byte)(256*a/nGrays); 
		 pRGB[a].rgbBlue=col;
		 pRGB[a].rgbGreen=col;
		 pRGB[a].rgbRed=col;
		 pRGB[a].rgbReserved=0;
	  }
}

void SetPalette(BYTE *Palette,INT nColors, BYTE *Red, BYTE *Green, BYTE *Blue)
{
 RGBQUAD *pRGB=(RGBQUAD *) Palette;
 int a;

 for (a=0;a<nColors;a++)
 {
   pRGB[a].rgbBlue=Blue[a];
   pRGB[a].rgbGreen=Green[a];
   pRGB[a].rgbRed=Red[a];
   pRGB[a].rgbReserved=0;
 }
}

void JPGSyncEnter(void)
{
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);
}

void JPGSyncLeave(void)
{
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
}


//
// isImage (Probably!)
//
EN_FILE_TYPE isImage(UTF8 * pszFileName) {

	FILE *ch;
	BYTE szHeader[20];
	INT a;
	EN_FILE_TYPE iRet=IMG_UNKNOW;

	ch=fopen(pszFileName,"rb"); if (!ch) return iRet;//ehError();
	a=fread(szHeader,10,1,ch); 
	if (a) {
		if (!memcmp(szHeader,"ÿØÿ",3)) iRet=IMG_JPEG; 
		else if (!memcmp(szHeader,"GIF8",4)) iRet=IMG_GIF;
		else if (!memcmp(szHeader+1,"PNG",3)) iRet=IMG_PNG;
	}
	fclose(ch);
	/*
	if (!iRet) printf("%c%c%c%c|%d,%d,%d,%d",
					szHeader[0],szHeader[1],szHeader[2],szHeader[3],
					(INT) szHeader[0],
					(INT) szHeader[1],
					(INT) szHeader[2],
					(INT) szHeader[3]);
					*/
	return iRet;
}

// --------------------------------------------------------
// IMGBuilder()
// Costruisce un immagine in memoria per poterla usare
// 
//
// --------------------------------------------------------

TCHAR *IMGTextType(EN_PIXEL_TYPE enPixelType)
{
	switch (enPixelType)
	{
	case IMG_PIXEL_UNKNOW: return TEXT("Sconosciuto");
	case IMG_PIXEL_GRAYSCALE: return TEXT("GrayScale 8bit"); 
	case IMG_PIXEL_COLOR8: return TEXT("Color 8bit"); 

	case IMG_PIXEL_BGR: return TEXT("Color 24bit BGR"); 
	case IMG_PIXEL_RGB: return TEXT("Color 24bit RGB"); 
	case IMG_PIXEL_RGB_ALPHA: return TEXT("Color 24bit RGB + Alpha");
	case IMG_PIXEL_CMYK: return TEXT("Color 32bit CMYK");

	case IMG_PIXEL_YCbCr: return TEXT("Color 24bit RGB/YCbCr");
	case IMG_PIXEL_YCCK: return TEXT("Color 32bit CMYK/YCCK");

//	case IMG_PIXEL_RGB_16: return TEXT("Color 32bit CMYK");
	}
	return TEXT("?");

}
//
// IMGCreate()
// Crea un oggetto immagine 
//
INT IMGCreate(	EN_PIXEL_TYPE enPixelType,
				CHAR *lpMemoName, // 66421_1029
				DWORD dwWidth,
				DWORD dwHeight, 
				IMGHEADER *psImgHeader,
				BOOL bOnlyHeader)
{
	IMGHEADER sImgHead,*psImgHead;
	INT hdlImage;

	if (bOnlyHeader) psImgHead=psImgHeader; else psImgHead=&sImgHead;

	memset(psImgHead,0,sizeof(IMGHEADER));
	psImgHead->enPixelType=enPixelType;
	strcpy(psImgHead->utfFileName,lpMemoName);
	psImgHead->lFileSize=0;
	psImgHead->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
	psImgHead->bmiHeader.biWidth=dwWidth;
	psImgHead->bmiHeader.biHeight=dwHeight; // forse * -1
	psImgHead->bmiHeader.biPlanes=1;
//	psImgHead->iChannels=3;
 
	switch (enPixelType)
	{
		case IMG_PIXEL_COLOR1:
				psImgHead->iChannels=0;			
				psImgHead->dwColors=2;
				psImgHead->bmiHeader.biBitCount=1;
				break;

		case IMG_PIXEL_COLOR4:
				psImgHead->iChannels=0;			
				psImgHead->dwColors=16;
				psImgHead->bmiHeader.biBitCount=4;
				break;

		case IMG_PIXEL_GRAYSCALE:
				psImgHead->iChannels=1;			
				break;

		case IMG_PIXEL_COLOR8:
				psImgHead->iChannels=1;			
				psImgHead->dwColors=256;
				break;

		case IMG_PIXEL_RGB_ALPHA:
		case IMG_PIXEL_CMYK:
				psImgHead->iChannels=4;			
				break;

		case IMG_PIXEL_RGB:
		case IMG_PIXEL_BGR:
				psImgHead->iChannels=3;			
				break;

		default:
			ehError();
	}

	//
	// Calcolo le dimensioni in byte di una linea
	//
	if (psImgHead->iChannels) {

		psImgHead->bmiHeader.biBitCount=8*psImgHead->iChannels;//iDeep;// bit colore
		psImgHead->linesize=psImgHead->bmiHeader.biWidth * psImgHead->iChannels; 
	}
	else {

		if (psImgHead->bmiHeader.biBitCount==1) psImgHead->linesize=(psImgHead->bmiHeader.biWidth+7)/8;
//		if (psImgHead->bmiHeader.biBitCount==1) psImgHead->linesize=(psImgHead->bmiHeader.biWidth+15)/8;
		else if (psImgHead->bmiHeader.biBitCount==4) psImgHead->linesize=(psImgHead->bmiHeader.biWidth+1)/2;
		else ehError();

	}

	psImgHead->bmiHeader.biCompression=BI_RGB;// Non compresso
	psImgHead->bmiHeader.biSizeImage=0;
	psImgHead->bmiHeader.biClrUsed=0;//psImgHead->dwColors;// Colori usati 0=Massimo
	psImgHead->bmiHeader.biClrImportant=0;//psImgHead->dwColors;// 0=Tutti i colori importanti

	//
	// Allineo a 32bit
	//
	//if (psImgHead->bmiHeader.biBitCount>=24)
	psImgHead->linesize=((psImgHead->linesize+3)>>2)<<2;
	psImgHead->dwBitmapSize=psImgHead->bmiHeader.biHeight*psImgHead->linesize;
	psImgHead->dwMemoSize=sizeof(IMGHEADER)+(psImgHead->dwColors*sizeof(S_RGB))+psImgHead->dwBitmapSize;
	psImgHead->Offset=sizeof(IMGHEADER)+(psImgHead->dwColors*sizeof(S_RGB)); // Header + Pallet

	if (bOnlyHeader) return 0;

	hdlImage=memoAlloc(RAM_AUTO,psImgHead->dwMemoSize,psImgHead->utfFileName); 
	if (hdlImage<0) return hdlImage;
	psImgHead=memoLock(hdlImage); 
	ehMemCpy(psImgHead,&sImgHead,sizeof(sImgHead));
	psImgHead->pbImage=(BYTE *) psImgHead+psImgHead->Offset;
	if (psImgHead->dwColors) 
	{
		psImgHead->psRgbPalette=(S_RGB *) ((BYTE *) psImgHead+sizeof(IMGHEADER)); // Posizione della palette
		memset(psImgHead->psRgbPalette,0,(sizeof(S_RGB)*psImgHead->dwColors)); 
		psImgHead->idxTrasparent=-1; // default = Nessuna trasparenze
	}

	if (psImgHeader) memcpy(psImgHeader,psImgHead,sizeof(IMGHEADER));
	memoUnlockEx(hdlImage,"imgCreate"); 
	return hdlImage;
}

void	IMGDestroy(EH_IMG hdlImage) {

	IMGHEADER * psImgHead;
	psImgHead=memoLock(hdlImage); 
	ehFreePtr(&psImgHead->pbOriginal);
	memoFree(hdlImage,"memo");
}

//
// IMGFill()
//
void IMGFill(INT hdlImage,DWORD cBackColor)
{
	INT kx,y;
	BYTE arbColor[10];
	BYTE *lpImg;
	BYTE *ap;
	IMGHEADER *psImgHead;
	psImgHead=memoLock(hdlImage); 

	switch (psImgHead->enPixelType)
	{
		case IMG_PIXEL_RGB_ALPHA:
				arbColor[0]=GetRValue(cBackColor); // R
				arbColor[1]=GetGValue(cBackColor); // G
				arbColor[2]=GetBValue(cBackColor); // B
				arbColor[3]=GetAValue(cBackColor); // A
				break;

		case IMG_PIXEL_BGR:
				arbColor[0]=GetBValue(cBackColor); // B
				arbColor[1]=GetGValue(cBackColor); // G
				arbColor[2]=GetRValue(cBackColor); // R
				break;

		case IMG_PIXEL_RGB:
				arbColor[0]=GetRValue(cBackColor); // B
				arbColor[1]=GetGValue(cBackColor); // G
				arbColor[2]=GetBValue(cBackColor); // R
				break;

		default:
				ehError();
	}

	lpImg=psImgHead->pbImage;
	for (y=0;y<psImgHead->bmiHeader.biHeight;y++)
	{
	 ap=lpImg;
	 for (kx=0;kx<psImgHead->bmiHeader.biWidth;kx++) {ehMemCpy(ap,arbColor,psImgHead->iChannels); ap+=psImgHead->iChannels;}
	 lpImg+=psImgHead->linesize;
	}
	memoUnlockEx(hdlImage,"imgFill"); 

}

void IMG_RGBSwapping(IMGHEADER *psImgHead)
{
   INT kx,ky;
   //BYTE k;
   //BYTE *ap;
   int *p32;
   BYTE *psSorg;
   int register w32;

//   if (lpImgHead->iChannels!=3) ehExit("RGB Swapping Channels error");

   if (psImgHead->enPixelType!=IMG_PIXEL_RGB&&
	   psImgHead->enPixelType!=IMG_PIXEL_BGR) ehExit("RGB Swapping Channels error");

   psSorg=psImgHead->pbImage;
   for (ky=0;ky<(INT) psImgHead->bmiHeader.biHeight;ky++)//cinfo.image_height;ky++)
   {
	   p32=(int *) psSorg;
	   for (kx=0;kx<(psImgHead->linesize-2);kx+=3)
		{
			w32=*p32; 
			*p32=(w32&0xFF00FF00)|
				 ((w32&0xFF)<<16) |   // Rosso al posto del blu
				 ((w32>>16)&0xFF); // Blu al posto del rosso
			p32=(int *) ((BYTE *) p32+3);
		}
       psSorg+=psImgHead->linesize;
   }
   // lpImgHead->fRGBInverted^=1;
   if (psImgHead->enPixelType==IMG_PIXEL_BGR) 
	   psImgHead->enPixelType=IMG_PIXEL_RGB; 
   else 
	   psImgHead->enPixelType=IMG_PIXEL_BGR;

}


//
// _JPGReadService()
// Funzione di servizio per leggere l'header o l'immagine JPG
//
static INT _JPGReadService(	BOOL bOnlyHeader,
							UTF8 * putfImageFileName,
							IMGHEADER *psImgHeadDest,

						    INT iFactor,
						    INT iDct,	// JDCT_ISLOW (Default), JDCT_IFAST (Veloce ma meno accurato), JDCT_FLOAT

							BOOL *pbStop,
							INT iModeError,
							INT *lpiErr
						   )
{
	struct jpeg_decompress_struct sCInfo;
	struct ima_error_mgr sJerr;
	FILE * chFile;
	WCHAR * pwcFile;
	CHAR szMemoName[200];
	INT hdlImage,iError;
	BOOL bStop=FALSE;
	IMGHEADER sImgHeader,*psImgHead;
	EN_PIXEL_TYPE enPixelTypeSource;
	EN_PIXEL_TYPE enPixelTypeCreate;
	
	// Controllo puntatori NULL
	if (!pbStop) pbStop=&bStop;
	if (!lpiErr) lpiErr=&iError;
	if (!psImgHeadDest) psImgHeadDest=&sImgHeader;

	if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);

	//
	// Apertura del file
	//
#ifdef __windows__
	pwcFile=strEnc(1,putfImageFileName,SD_UTF8,NULL);
	if ((chFile=_wfopen(pwcFile,L"rb")) == NULL) 
#else
	if ((chFile=fopen(imageFileName,"rb")) == NULL) 
#endif
	{
		if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
		ehFree(pwcFile);
		*lpiErr=-1;
		return FALSE;
	}
#ifdef __windows__
	ehFree(pwcFile);
#endif  

//  --------------------------------------------------------------------------------------
//  Fase 1: alloco ed inizializzo l'oggetto JPEG per la gestione degli errori
//  Settiamo la normale JPEG error routines, che avverra in uscita con errore.
//  --------------------------------------------------------------------------------------
	sCInfo.err = jpeg_std_error(&sJerr.pub);
	jpeg_create_decompress(&sCInfo);
	sCInfo.dct_method=JDCT_IFAST;

	//
	switch (iModeError)
	{
		case JME_HIDE:
		  sJerr.pub.error_exit = ima_jpeg_error_exit_noview; // Dichiarata in testa
		  break;

		case JME_NORMAL:
		default:
		  sJerr.pub.error_exit = ima_jpeg_error_exit; // Dichiarata in testa
		  break;
	}
 
	if (setjmp(sJerr.setjmp_buffer))  
	{
		printf("** Jmp: error");
		jpeg_destroy_decompress(&sCInfo); 
		fclose(chFile);
		if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
		*lpiErr=-2;
		printf("**" CRLF);
		return FALSE;
	}

//  --------------------------------------------------------------------------------------
//  Fase 2 : specifico lo stream del file contenente il JPEG
//  --------------------------------------------------------------------------------------
	jpeg_stdio_src(&sCInfo, chFile);
	/*
	if (iDct>0)
	{
		sCInfo.dct_method=iDct;
		sCInfo.scale_num=1;
		sCInfo.scale_denom=iFactor;
		if (iFactor>1)
			iFactor=iFactor;
	}
	*/
//  --------------------------------------------------------------------------------------
//  Fase 3 : Leggo l'header del JPEG
//  --------------------------------------------------------------------------------------
	jpeg_read_header(&sCInfo, TRUE);

//#ifdef UNICODE
	//UnicodeToChar(szMemoName,fileName(imageFileName));
//#else
	strcpy(szMemoName,fileName(putfImageFileName));
//#endif

	switch (sCInfo.jpeg_color_space)
	{
		case JCS_GRAYSCALE: 
			 enPixelTypeSource=IMG_PIXEL_GRAYSCALE;
			 enPixelTypeCreate=IMG_PIXEL_GRAYSCALE;
			 break;

		case JCS_CMYK: 
			 enPixelTypeSource=IMG_PIXEL_CMYK;
			 enPixelTypeCreate=IMG_PIXEL_CMYK;
			 break;

		case JCS_YCbCr: 
			enPixelTypeSource=IMG_PIXEL_YCbCr;
			enPixelTypeCreate=IMG_PIXEL_RGB;
			break;
		 
		case JCS_YCCK: 
			enPixelTypeSource=IMG_PIXEL_YCCK;
			enPixelTypeCreate=IMG_PIXEL_CMYK;
			break;

		default:
			enPixelTypeSource=IMG_PIXEL_RGB;
			enPixelTypeCreate=IMG_PIXEL_RGB;
			break;
	}

	//
	// Indico a che velocita devo caricarlo
	//
	if (iDct>0)
	{
		sCInfo.dct_method=iDct;
		sCInfo.scale_num=1;
		sCInfo.scale_denom=iFactor;
		if (iFactor>1)
			iFactor=iFactor;
	}

	jpeg_start_decompress(&sCInfo);
	hdlImage=IMGCreate(	enPixelTypeCreate,
						szMemoName,
						sCInfo.output_width,
						sCInfo.output_height,
						psImgHeadDest,
						bOnlyHeader);

	if (!bOnlyHeader) psImgHead=memoLockEx(hdlImage,"JPGReadFile"); else psImgHead=psImgHeadDest;
	psImgHead->enType=IMG_JPEG;
	psImgHead->lFileSize=fileSize(putfImageFileName);
	psImgHead->enFilePixelType=enPixelTypeSource;
	strcpy(psImgHead->utfFullFileName,putfImageFileName);
	
	//
	// Solo l'header
	//
	if (bOnlyHeader)
	{
		jpeg_destroy_decompress(&sCInfo);
		fclose(chFile);
		hdlImage=TRUE; // Dico ch è tutto ok
	}
	//
	// Carico l'immagine in memoria
	//
	else
	{
		BYTE *pbCB,*pbDest=psImgHead->pbImage;
		JSAMPARRAY arBuffer;

		arBuffer= (*sCInfo.mem->alloc_sarray) ((j_common_ptr) &sCInfo, JPOOL_IMAGE, sCInfo.output_width * sCInfo.output_components, 1);

		//  --------------------------------------------------------------------------------------
		//  Fase 7  : Loop sulle linee che rimangono da leggere
		//  Usiamo per il loop una variabile di stato delle librerie cinfo.output_scanline
		//  come contatore, cosi fancendo non .....
		//  --------------------------------------------------------------------------------------
		pbCB=pbDest;
		while (sCInfo.output_scanline < sCInfo.output_height) {
		 jpeg_read_scanlines(&sCInfo, arBuffer, 1);
		 ehMemCpy(pbCB,arBuffer[0],psImgHead->linesize); if (*pbStop) break;
		 pbCB+=psImgHead->linesize;
		}

		//  --------------------------------------------------------------------------------------
		//  Fase 8
		//  Fine della decompressione rilascio le memorie usate
		//  --------------------------------------------------------------------------------------

		if (*pbStop) jpeg_abort_decompress(&sCInfo); else jpeg_finish_decompress(&sCInfo);

		//  --------------------------------------------------------------------------------------
		//  Fase 9
		//  Rilascio il JPEG decompression Object
		//  --------------------------------------------------------------------------------------

		memoUnlockEx(hdlImage,"JPGReadFile2");
		jpeg_destroy_decompress(&sCInfo);
		fclose(chFile);

		if (*pbStop)
		{
			if (hdlImage>0) memoFree(hdlImage,"Stop");
			hdlImage=-1;
		}
	}

	if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	return hdlImage;
}

//
// JPGGetFactor()
//
INT JPGGetFactor(SIZE *lpsSource,SIZE *lpsDest,INT *lpiPerc)
{
	INT iScale,iPerc;
	iScale=1;
	
	// Foto orizzontale/quadrata
	if (lpsSource->cx>=lpsSource->cy)
	{
		iPerc=lpsDest->cx*100/lpsSource->cx;
	}
	// Foto verticale
	else
	{
		iPerc=lpsDest->cy*100/lpsSource->cy;
	}

	while (TRUE)
	{
		if (!iPerc) break;
		if ((lpsSource->cy*iPerc/100)>lpsDest->cy||(lpsSource->cx*iPerc/100)>lpsDest->cx)			
		{
			iPerc--;
		} 
		else break;
	}

	if (lpiPerc) *lpiPerc=iPerc;

	if (iPerc<100&&iPerc>1)
	{
		iScale=100/iPerc; // 100% 1     50%  2     25%  4      10%  5
	} 
	else iScale=1;

	if (iScale<1) iScale=1;
	return iScale;
}


// ---------------------------------------------------
// JPGReadFile
// LEGGE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da leggere
// *HdlImage	 Puntatore all'handle che conterrà l'immagine
// TrueColor     TRUE=True Color/FALSE=256 colori
// *fStop        Puntatore ad un BOOL che determina lo stop del processo 
//               serve per applicazione Multi/Thread
//               NULL se non si usa
// Ritorna: 0 Errore
//          1 Tutto OK
//


INT JPGReadHeader(UTF8 *imageFileName,IMGHEADER *psImgHead,INT iModeError)
{
	INT hdlImage;
	hdlImage=_JPGReadService(true,
							 imageFileName,
							 psImgHead,

							 -1,
							 -1,	// JDCT_ISLOW (Default), JDCT_IFAST (Veloce ma meno accurato), JDCT_FLOAT

							 NULL,
							 iModeError,
							 NULL);
	return hdlImage;
}


BOOL JPGReadFile(TCHAR *imageFileName,INT *piImageHandle,BOOL *pbStop,INT *piErr,BOOL iModeError,EN_PIXEL_TYPE enPixelType)
{
	return JPGReadFileEx(imageFileName,piImageHandle,pbStop,-1,-1,piErr,iModeError,enPixelType);
}

//
// JPGReadFileEx()
// Ritorna TRUE se tutto OK
//
BOOL JPGReadFileEx(TCHAR *imageFileName,
				   INT *piImageHandle,
				   BOOL *pbStop,
				   
				   INT iFactor,
				   INT iDct,	// JDCT_ISLOW (Default), JDCT_IFAST (Veloce ma meno accurato), JDCT_FLOAT
				   
				   INT *lpiErr,
				   BOOL iModeError,
				   EN_PIXEL_TYPE enPixelType)
{
	INT hdlImage;
	if (!piImageHandle) piImageHandle=&hdlImage;
	hdlImage=_JPGReadService(FALSE,
							 imageFileName,
							 NULL,

							 iFactor,
							 iDct,	// JDCT_ISLOW (Default), JDCT_IFAST (veloce ma meno accurato), JDCT_FLOAT

							 pbStop,
							 iModeError,
							 lpiErr);
	if (hdlImage<1) {

		*piImageHandle=-1;
		return false; // Errore

	}

	if (enPixelType==IMG_PIXEL_UNKNOW) enPixelType=IMG_PIXEL_BGR; // Compatibilità con il passato
	hdlImage=IMGConvert(hdlImage,enPixelType);
	*piImageHandle=hdlImage;
	return true;
}


// 
// JPGSaveFile
// SCRIVE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da scrivere
// HdlImage	     Hdl che contiene l'immagine in formato IMGHEADER
// iQuality      Fattore di qualità dell'immagine
// 0= errore >0 tutto ok
//
BOOL JPGSaveFile(TCHAR *imageFileName,INT HdlImage,INT iQuality)
{
  FILE * outfile;		/* Il File sorgente */
  INT rt;
  // Apertura del file
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);
  if ((outfile=fopen(imageFileName,"wb")) == NULL) 
  {	 
	  win_infoarg("JPGSaveFile():Errore in creazione file [%s]",imageFileName);
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	  return 0;
  }

  rt=JPGPutStream(outfile,HdlImage,iQuality);
  fclose(outfile);
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
  return rt;
}

BOOL JPGPutStream(FILE *outfile,INT HdlImage,INT iQuality)
{
  IMGHEADER *Img;
  BYTE *GBuffer;
  struct jpeg_compress_struct cinfo;

  /* Usiamo una nostra gestione privata di JPEG error handler. */
  struct ima_error_mgr jerr;

  /* More stuff */
  JSAMPARRAY buffer;	/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  BYTE *lpLine;

  /*

     In questo esempio dobbiamo aprire il file di input prima di ogni cosa ,
	 facendo cosi' setjmp() error recovery assume che il file sia aperto.
	 IMPORTANTE: usere l'opzione "b" per aprire il file
    
  */
  INT kx;

  Img=memoLock(HdlImage);
  GBuffer=(BYTE *) Img;
  GBuffer+=Img->Offset;

//  --------------------------------------------------------------------------------------
//  Fase 1: alloco ed inizializzo l'oggetto JPEG per la gestione degli errori
//  Settiamo la normale JPEG error routines, che avverra in uscita con errore.
//  --------------------------------------------------------------------------------------
  memset(&cinfo,0,sizeof(cinfo));
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = ima_jpeg_error_exit; // Dichiarata in testa

  // Stabilisco il salto di ritorno su errore nel mio contesto.
  // Se avviene un errore JPEG lo segnala
  // Noi dobbiamo pulire JPEG object,chiudere l'input file, e ritornare

  if (setjmp(jerr.setjmp_buffer)) {jpeg_destroy_compress(&cinfo); fclose(outfile); memoUnlockEx(HdlImage,"JpgPutStream"); return 0;}
  
  // Possiamo ora inizializzare l'oggetto JPEG per la compressione
  jpeg_create_compress(&cinfo);

//  --------------------------------------------------------------------------------------
//  Fase 2 : specifico lo streem del file contenente il JPEG
//  --------------------------------------------------------------------------------------
  jpeg_stdio_dest(&cinfo, outfile);

//  --------------------------------------------------------------------------------------
//  Fase 3 : Setto i parametri per la compressione
//  --------------------------------------------------------------------------------------

  // Descrivo la grandezza il tipo dell'immagine
  // 4 campi nella struttura cinfo devono essere riempiti:
  //
  cinfo.image_width = Img->bmiHeader.biWidth; 
  cinfo.image_height = Img->bmiHeader.biHeight;
  cinfo.input_components = Img->iChannels;//3;//Img->bmiHeader.biBitCount>>3;
  
  //cinfo.in_color_space = JCS_RGB; 	// colorspace of input image 
  switch (Img->enPixelType) {
	  default:
		  cinfo.in_color_space = JCS_RGB;
		  break;
	  case IMG_PIXEL_UNKNOW:
	  case IMG_PIXEL_GRAYSCALE:
		  cinfo.in_color_space = JCS_GRAYSCALE;
		  break;

  }
 

  // Setto i parametri di default nella libreria
  // (You must set at least cinfo.in_color_space before calling this,
  // since the defaults depend on the source color space.)
  jpeg_set_defaults(&cinfo);

  // Setto un parametro non di "default"; la qualità
  // Here we just illustrate the use of quality (quantization table) scaling:
  jpeg_set_quality(&cinfo, iQuality, TRUE); /* limit to baseline-JPEG values */

  //  --------------------------------------------------------------------------------------
  //  Fase 4 : Parte il compressore
  //  --------------------------------------------------------------------------------------
  // TRUE ensures that we will write a complete interchange-JPEG file.
  // Pass TRUE unless you are very sure of what you're doing.
  jpeg_start_compress(&cinfo, TRUE);

  row_stride = Img->bmiHeader.biWidth*cinfo.input_components;	/* JSAMPLEs per row in image_buffer */

  // Costruisco una riga/array di appoggio che che verra liberata quando l'immagine verra' fatta
  buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
/*
  while (cinfo.next_scanline < cinfo.image_height) {
    // jpeg_write_scanlines espetta un array of puntatori alle "scanlines".
    // L'array qui è di un solo elemento, ma puoi passare più di un elemento alla volta
    // se pensi sia più conveniente
    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
*/

//  --------------------------------------------------------------------------------------
//  Fase 5  : Loop sulle linee per scriverle
//  --------------------------------------------------------------------------------------
  
  while (cinfo.next_scanline < cinfo.image_height) 
  {
	 // Leggo la linea 
	 lpLine=buffer[0];
	 ehMemCpy(lpLine,GBuffer,row_stride);
	 
	 if (Img->enPixelType==IMG_PIXEL_BGR)
	 {
	  // Converto BGR con RGB (sempre per lo stronzo di windows)
	  for (kx=0;kx<(row_stride-2);kx+=3)
		{
		 BYTE k;
		 k=*lpLine; *lpLine=*(lpLine+2); *(lpLine+2)=k;
		 lpLine+=3;
		}
	 }

	 // SCrivo la linea
	 jpeg_write_scanlines(&cinfo, buffer, 1);
	 GBuffer+=Img->linesize;
  }

//  --------------------------------------------------------------------------------------
//  Fase 6  : Libero le risorse impegnate
//  --------------------------------------------------------------------------------------
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  
  memoUnlockEx(HdlImage,"JPGPutStream2");

//  --------------------------------------------------------------------------------------
//  E siamo fatti ....!!!
  return 1;
}
//
// ritorna FALSE se ci sono errori
//
/*
BOOL JPGReadHeader(TCHAR *imageFileName,IMGHEADER *ImgHead,INT iModeError)
{
	
	struct jpeg_decompress_struct cinfo;

	struct ima_error_mgr jerr;

	FILE * infile;		// Il File sorgente 
	int row_stride;		// physical row width in output buffer 

	if (!imageFileName) return FALSE;
	if (!*imageFileName) return FALSE;
	if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);

	if ((infile=_tfopen(imageFileName,TEXT("rb"))) == NULL) 
	{	
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	  return FALSE;
	}

//  --------------------------------------------------------------------------------------
//  Fase 1: alloco ed inizializzo l'oggetto JPEG per la gestione degli errori
//  Settiamo la normale JPEG error routines, che avverra in uscita con errore.
//  --------------------------------------------------------------------------------------
	memset(&cinfo,0,sizeof(cinfo));
	cinfo.err = jpeg_std_error(&jerr.pub);

	switch (iModeError)
	{
		case JME_HIDE:
		  jerr.pub.error_exit = ima_jpeg_error_exit_noview; // Dichiarata in testa
		  break;

		case JME_NORMAL:
		default:
		  jerr.pub.error_exit = ima_jpeg_error_exit; // Dichiarata in testa
		  break;
	}

	// Stabilisco il salto di ritorno su errore nel mio contesto.
	// Se avviene un errore JPEG lo segnala
	// Noi dobbiamo pulire JPEG object,chiudere l'input file, e ritornare

	if (setjmp(jerr.setjmp_buffer))  {//jpeg_destroy_decompress(&cinfo); 
									fclose(infile);
									if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
									return FALSE;}
	// Possiamo ora inizializzare l'oggetto JPEG per la decompressione
	jpeg_create_decompress(&cinfo);

	//  --------------------------------------------------------------------------------------
	//  Fase 2 : specifico lo streem del file contenente il JPEG
	//  --------------------------------------------------------------------------------------
	jpeg_stdio_src(&cinfo, infile);

	//  --------------------------------------------------------------------------------------
	//  Fase 3 : Leggo l'header del JPEG
	//  --------------------------------------------------------------------------------------
	jpeg_read_header(&cinfo, TRUE);

	//  --------------------------------------------------------------------------------------
	//  FASE 6: Riempo la struttura IMGHEADER 
	//  --------------------------------------------------------------------------------------
	jpeg_start_decompress(&cinfo);
	memset(ImgHead,0,sizeof(IMGHEADER));

	// Specifico i dati dell'header
	ImgHead->enType=IMG_JPEG;
	strcpy(ImgHead->szFileName,imageFileName);
	ImgHead->lFileSize=fileSize(imageFileName);
	ImgHead->iChannels=cinfo.output_components; 
	ImgHead->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
	ImgHead->bmiHeader.biWidth=cinfo.image_width;
	ImgHead->bmiHeader.biHeight=cinfo.image_height; // forse * -1
	ImgHead->bmiHeader.biPlanes=1;
	ImgHead->bmiHeader.biBitCount=cinfo.output_components*8;
	ImgHead->bmiHeader.biCompression=BI_RGB;// Non compresso
	ImgHead->bmiHeader.biSizeImage=0;
	ImgHead->bmiHeader.biClrUsed=0;// Colori usati 0=Massimo
	ImgHead->bmiHeader.biClrImportant=0;// 0=Tutti i colori importanti
	row_stride = cinfo.output_width * cinfo.output_components;
	ImgHead->linesize=row_stride;
	ImgHead->linesize=((ImgHead->linesize+ImgHead->iChannels)>>2)<<2;
	ImgHead->dwBitmapSize=ImgHead->bmiHeader.biHeight*ImgHead->linesize;

	jpeg_destroy_decompress(&cinfo);
	fclose(infile);

	if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	return TRUE;
}

*/
void IMGCalc(INT *SizeX,INT *SizeY,INT Hdl)
{
	IMGHEADER *Img;
	WORD    a=1;
	BITMAPINFOHEADER BmpHeaderBackup;
	BITMAPINFOHEADER *BmpHeader;
	BITMAPINFO *BmpInfo;
	BYTE *Sorg;
	LONG Lx,Ly;

	//win_info("=%d",Hdl);
	Img=memoLockEx(Hdl,"IMGCalc");
	Sorg=(BYTE *) Img;
	Sorg+=Img->Offset;

	BmpInfo=(BITMAPINFO *) &Img->bmiHeader;
	BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
	ehMemCpy(&BmpHeaderBackup,BmpHeader,sizeof(BmpHeaderBackup));

	Ly=BmpHeader->biHeight;
	Lx=BmpHeader->biWidth;
	//win_infoarg("%d,%d, (%d,%d)",SizeX,SizeY,Lx,Ly);
	//  SizeY:Ly=x:Lx;
	if ((*SizeY>0)&&(*SizeX==0)) *SizeX=*SizeY*Lx/Ly;
	// Calcolo automatico delle dimensioni verticali
	if ((*SizeX>0)&&(*SizeY==0)) *SizeY=*SizeX*Ly/Lx;

	memoUnlockEx(Hdl,"IMGCalc");
	return;
}


// ------------------------------------------------------------------
// IMGRemaker
// Costruisce un immagine partendo da un'altra in altre dimensioni
// Per ora solo true color
// ------------------------------------------------------------------
static INT LocalIMGRemaker(INT HdlImage,RECT *psrRectSource,INT xNew,INT yNew);
static INT LocalIMGRemakerAA(INT HdlImage,RECT *psrRectSource,INT xNew,INT yNew,INT iTRS);

INT IMGRemaker(INT HdlImage,RECT *psrRectSource,INT xNew,INT yNew,INT fPhotoQuality,INT iTRS)
{
  if (fPhotoQuality) return LocalIMGRemakerAA(HdlImage,psrRectSource,xNew,yNew,iTRS);
  return LocalIMGRemaker(HdlImage,psrRectSource,xNew,yNew);
}

static INT LocalIMGRemaker(INT HdlImage,RECT *psrRectSource,INT xNew,INT yNew)
{
	BYTE *lpi;
	IMGHEADER *ImgSorg,*ImgDest;
	INT iNewSize;
	INT iLineSize;
	INT HdlNew;
	BYTE *lpSorg,*lpDest;
	BITMAPINFOHEADER *BHSorg;
	BITMAPINFOHEADER *BHDest;
	POINT pbSorg;
	INT yPf,iPf;
	INT xc;
	SIZE sizSource;
	POINT poOfs;
	register INT x,y;
   	
	lpi=memoLockEx(HdlImage,"LocalIMGRemaker");
	//if (lpi==NULL) ehExit("LocalIMGRemaker(): ");
	ImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+ImgSorg->Offset;
	
	BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;

	// Ricalcolo l'occupazione di spazio

	// Quantizzazione a 32 bit 
	iLineSize=xNew*ImgSorg->iChannels; iLineSize=((iLineSize+ImgSorg->iChannels)>>2)<<2;
//	win_infoarg("%d - %d - %d",xNew,xNew*3,iLineSize);
	iNewSize=(iLineSize*yNew)+ImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memoAlloc(RAM_AUTO,iNewSize,"NewImage");
	if (HdlNew<0) ehExit("IMGRemaker:non memory");
	lpDest=memoLockEx(HdlNew,"LocalIMGRemaker2");
	memset(lpDest,0xFF,iNewSize);

	// Copio l'header
	ehMemCpy(lpDest,lpi,ImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	ImgDest=(IMGHEADER *) lpDest;
	ImgDest->linesize=iLineSize;
	BHDest=(BITMAPINFOHEADER *) &ImgDest->bmiHeader;
	BHDest->biHeight=yNew;
	BHDest->biWidth=xNew;
	lpDest+=ImgDest->Offset;
	ImgDest->pbImage=lpDest;

	if (!psrRectSource) 
		{sizSource.cy=BHSorg->biHeight; 
		 sizSource.cx=BHSorg->biWidth;
		 poOfs.x=0;
		 poOfs.y=0;
		} // Compatibilità con il passato
		else
		{sizSource.cy=psrRectSource->bottom-psrRectSource->top+1; 
		 sizSource.cx=psrRectSource->right-psrRectSource->left+1;
		 poOfs.x=psrRectSource->left;
		 poOfs.y=psrRectSource->top;
		} // Compatibilità con il passato
	// Effettuo lo streching da Grande a piccolo
//	if ((xNew<BHSorg->biWidth)&&(yNew<BHSorg->biHeight)) 
	{
	 for (y=0;y<yNew;y++)
	 {
		// pbSorg.y= Posizione Y del sorgente formula  y:pbSorg.y=yNew:sizSource.cy;//BHSorg->biHeight
		//pbSorg.y=y*BHSorg->biHeight/yNew;
		pbSorg.y=poOfs.y+(y*sizSource.cy/yNew);

		// yPF Puntantore a Y Reale calcolato su ImgSorg->linesize
		yPf=(pbSorg.y*ImgSorg->linesize);
		xc=0;
		for (x=0;x<xNew;x++)
		{
			// Calcolo la posizione del punto X nel sorgente
			// xSorg? : xSorgSize = yDest : yDestSize;
			//pbSorg.x=x*BHSorg->biWidth/xNew;
			pbSorg.x=poOfs.x+(x*sizSource.cx/xNew);

			// Calcolo la posizione fisica
			iPf=yPf+(pbSorg.x*ImgSorg->iChannels);
			// Copia il pixel
			ehMemCpy(lpDest+xc,lpSorg+iPf,ImgSorg->iChannels);
			xc+=ImgSorg->iChannels;
		}
		lpDest+=ImgDest->linesize;
	 }
	}
	// Effettuo lo streching da Piccolo -> a GRANDE
	/*
	else
	{
	 for (y=0;y<yNew;y++)
	 {
		// pbSorg.y= Posizione Y del sorgente formula  y:pbSorg.y=yNew:BHSorg->biHeight
		pbSorg.y=y*BHSorg->biHeight/yNew;
		// yPF Puntantore a Y Reale calcolato su ImgSorg->linesize
		yPf=(pbSorg.y*ImgSorg->linesize);
		xc=0;
		memset(lpDest+xc,0,3);
		lpDest+=ImgDest->linesize;
	 }

	}
	*/
	memoUnlockEx(HdlNew,"LocalIMGRemaker2");
	memoUnlockEx(HdlImage,"LocalIMGRemaker3");
	return HdlNew;
}


// ------------------------------------------------------------------
// IMGRemakerAA
// Costruisce un immagine partendo da un'altra in altre dimensioni
// Effettua antialiasing sulla compressione
// Per ora solo true color
// ------------------------------------------------------------------
static INT LocalIMGRemakerAA(INT HdlImage,RECT *psrRectSource,INT xNew,INT yNew,INT iTRS)
{
	BYTE *lpi;
	IMGHEADER *ImgSorg,*ImgDest;
	INT iNewSize;
	SIZE sImage;
	INT iLineSize;
	INT HdlNew;
	BYTE *lpSorg,*lpDest;
	BITMAPINFOHEADER *BHSorg;
	BITMAPINFOHEADER *BHDest;
	POINT pbSorg;
	POINT pSorg2;
	INT yPf;
	INT xc;
	INT xx3;
	BYTE *lpj;
	INT rgb[6];
	register INT x,y;
	register INT yk,xk;
	register INT iCount;
	INT a,c;
   	
	lpi=memoLockEx(HdlImage,"LocalIMGRemakerAA");
	ImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+ImgSorg->Offset;
	
	BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;
	sImage.cy=BHSorg->biHeight;
    sImage.cx=BHSorg->biWidth;

	// Se le dimensioni sono uguali o maggiori usa l'altro metodo
	if ((xNew>=sImage.cx)&&(yNew>=sImage.cy))
	{
	 	memoUnlockEx(HdlImage,"AA1");
		return LocalIMGRemaker(HdlImage,psrRectSource,xNew,yNew);
	}

	// Ricalcolo l'occupazione di spazio

	// Quantizzazione a 32 bit
	iLineSize=xNew*ImgSorg->iChannels; iLineSize=((iLineSize+ImgSorg->iChannels)>>2)<<2;
	iNewSize=(iLineSize*yNew)+ImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memoAlloc(RAM_AUTO,iNewSize,"NewImage");
	if (HdlNew<0) ehExit("IMGRemaker: non memory");
	lpDest=memoLockEx(HdlNew,"LocalIMGRemakerAA2");
	memset(lpDest,0,iNewSize);

	// Copio l'header
	ehMemCpy(lpDest,lpi,ImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	ImgDest=(IMGHEADER *) lpDest;
	ImgDest->linesize=iLineSize;
	BHDest=(BITMAPINFOHEADER *) &ImgDest->bmiHeader;
	BHDest->biHeight=yNew;
	BHDest->biWidth=xNew;
	lpDest+=ImgDest->Offset;
	ImgDest->pbImage=lpDest;

	// -----------------------------------------------
	// Effettuo lo streching con antialiasing
	//
	for (y=0;y<yNew;y++)
	{
		pbSorg.y=y*sImage.cy/yNew;
		pSorg2.y=(y+1)*sImage.cy/yNew;
		if (pbSorg.y>sImage.cy) pbSorg.y=sImage.cy;
		xc=0;
		for (x=0;x<xNew;x++)
		{
			// Calcolo la posizione del punto X nel sorgente
			// xSorg? : xSorgSize = yDest : yDestSize;
			pbSorg.x=x*sImage.cx/xNew; 
			pSorg2.x=(x+1)*sImage.cx/xNew;
			if (pbSorg.x>sImage.cx) pbSorg.x=sImage.cx;
		    yPf=(pbSorg.y*ImgSorg->linesize); // Posizione fisica di inizio linea
			xx3=(pbSorg.x*ImgSorg->iChannels);

			// A) Somma di tutti i colori del blocco
			_(rgb);
			iCount=0;
			for (yk=pbSorg.y;yk<pSorg2.y;yk++)
			{
				lpj=lpSorg+yPf+xx3;
				for (xk=pbSorg.x;xk<pSorg2.x;xk++)
				{
					for (c=0;c<ImgSorg->iChannels;c++)
					{
					  rgb[c]+=(INT) *lpj; lpj++;
					}
				  //rgb[1]+=(INT) *lpj; lpj++;
				  //rgb[2]+=(INT) *lpj; lpj++;
				  //if (ImgSorg->iChannels==4) rgb[3]+=(INT) *lpj; lpj++;
				}
				yPf+=ImgSorg->linesize; // Incremento di una linea
			}
			iCount=(yk-pbSorg.y)*(xk-pbSorg.x);
			// B) Calcolo della media
			if (iCount<1) iCount=1;
			for (a=0;a<ImgSorg->iChannels;a++,xc++) {*(lpDest+xc)=(BYTE) (rgb[a]/iCount);}
			// Copia il pixel
			//memcpy(lpDest+xc,lpSorg+iPf,3);
			//xc+=3;
		}
		lpDest+=ImgDest->linesize;
	}

	memoUnlockEx(HdlNew,"LocalIMGRemakerAA3");
	memoUnlockEx(HdlImage,"LocalIMGRemakerAA4");
	return HdlNew;
}

// ----------------------------------------------------------------------------
// JPGNewFile
// Crea un nuovo file di nuove dimensioni e qualità differenti 
// partendo da un'altro file JPG
//
// NB: uno dei due Lx/Ly deve essere a 0
//
// Ritorna  0= Tutto OK
//         -1= Errore in lettura file
//         -2= Errore in scrittura file
//         -3= Errore in ridimensionamento
//
// -----------------------------------------------------------------------------
#ifndef EH_MOBILE
INT JPGNewFile(TCHAR *lpFileSource,
				TCHAR *lpFileDest,
				INT iLx,INT iLy,
				INT iQuality,
				BOOL fAntiAlias,
				INT iTRS,
				EN_PIXEL_TYPE enPixelType)
{
	INT hdlImage,hdlImageNew;
	INT err;
    IMGHEADER ImgHead;
	
	// Leggo l'header
	if (!JPGReadHeader(lpFileSource,&ImgHead,JME_HIDE))
	{
		if (!GIFReadHeader(lpFileSource,&ImgHead,JME_HIDE)) 
		{
			//printf("ko");
			//printf("%s: errore in lettura Header",lpFileSource);
			//ehLogWrite("%s: errore in lettura Header",lpFileSource);
			//return TRUE;
			return -1;
		} 
	}

	// Calcolo automatico delle dimensioni orizzontali
    //  SizeY:Ly=x:Lx;
    if ((iLy>0)&&(iLx==0)) iLx=iLy*ImgHead.bmiHeader.biWidth/ImgHead.bmiHeader.biHeight;
    // Calcolo automatico delle dimensioni verticali
    if ((iLx>0)&&(iLy==0)) iLy=iLx*ImgHead.bmiHeader.biHeight/ImgHead.bmiHeader.biWidth;

	// ------------------------------------------------------------------------
	// C) Carica il sorgente in memoria
	//
	if (!enPixelType) enPixelType=IMG_PIXEL_BGR;
	switch (ImgHead.enType)
	{
		case IMG_JPEG:
			//printf("Jpg");
			if (!JPGReadFile(lpFileSource,&hdlImage,NULL,NULL,FALSE,0)) 
			{
				//printf("%s: errore in lettura JPG",lpFileSource);
				//ehLogWrite("%s: errore in lettura JPG",lpFileSource);
				return -1;
			}
//			hdlImage=IMGToRGB(hdlImage,1);
			hdlImage=IMGConvert(hdlImage,enPixelType);
			break;

		case IMG_GIF:
			//printf("Gif");
			if (!GIFReadFile(lpFileSource,&hdlImage,RGB(255,255,255),FALSE,enPixelType)) 
			{
				//printf("ko");
				//printf("%s: errore in lettura GIF",lpFileSource);
				//ehLogWrite("%s: errore in lettura GIF",lpFileSource);
				//printf("Errore"); //getch();
				return -1;
//				return TRUE;
			}
			break;
		
		default:
//			printf("ko");
//			printf("%s: formato non gestito",lpFileSource);
//			ehLogWrite("%s: formato non gestito",lpFileSource);
			return -1;
	}

//	if (!JPGReadFile(lpFileSource,&HdlImage,TRUE,NULL,NULL,TRUE)) return -1;
	if (iLy<5)	hdlImageNew=IMGRemaker(hdlImage,NULL,iLx,iLy,fAntiAlias,iTRS);
				else
				hdlImageNew=IMGResampling(hdlImage,NULL,iLx,iLy,iTRS);

	//HdlImageNew=IMGRemaker(HdlImage,iLx,iLy,fAntiAlias,iTRS);
				    
	if (hdlImageNew<0)
	{
		//win_infoarg("JPGNewFile:Errore in ridimensionamento %s",lpFileSource);
		memoFree(hdlImage,"Img1");
		return -3;
	}

	err=JPGSaveFile(lpFileDest,hdlImageNew,iQuality);
	memoFree(hdlImage,"Img1");
	memoFree(hdlImageNew,"Img1");
	if (!err) return -2;
	return 0;
}
#endif

// ----------------------------------------------------------------------------
// IMGTopdown
// Mette sotto sopra il bitmap (Necessario per alcuni tipi di driver stampanti
//
//
//
void IMGTopdown(INT HdlImage)
{
	IMGHEADER *ImgSorg;
	BITMAPINFOHEADER *BHSorg;
	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	INT x,y;
	INT HdlMemo;
	BYTE *lpMemo;

	lpi=memoLock(HdlImage);
	ImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+ImgSorg->Offset;
	
	BHSorg=(BITMAPINFOHEADER *) &ImgSorg->bmiHeader;
	sImage.cy=BHSorg->biHeight;
    sImage.cx=BHSorg->biWidth;

	if (ImgSorg->bmiHeader.biBitCount!=24) {ehExit("IMGTopdown solo a 24 bit");}

	// -----------------------------------------------
	// Chiedo memoria per il processo
	//
	HdlMemo=memoAlloc(RAM_AUTO,sImage.cy*3,"Buff");
	lpMemo=memoLock(HdlMemo);
	
	// -----------------------------------------------
	// Effettuo il mirroring
	//
	
	for (x=0;x<sImage.cx;x++)
	{
		lpj=lpSorg+x*3;
		for (y=0;y<sImage.cy;y++)
		{	
			ehMemCpy(lpMemo+(y*3),lpj,3); lpj+=ImgSorg->linesize;
		}

		lpj-=ImgSorg->linesize;
		for (y=0;y<sImage.cy;y++)
		{	
			ehMemCpy(lpj,lpMemo+(y*3),3); lpj-=ImgSorg->linesize;
		}
	}
	BHSorg->biHeight=-BHSorg->biHeight;
	memoFree(HdlMemo,"Buf");
	memoUnlockEx(HdlImage,"IMGTopDown");
}


/*
#define MAX_PIXEL_VALUE 255

#define MAX_OUT_DIMENSION ((MAX_OUT_WIDTH > MAX_OUT_HEIGHT) ? \
                           MAX_OUT_WIDTH : MAX_OUT_HEIGHT)

float a;

static float C0(float t) {return -a * t * t * t + a * t * t;}
static float C1(float t) {return -(a + 2.0f) * t * t * t + (2.0f * a + 3.0f) * t * t - a * t;}
static float C2(float t) {return (a + 2.0f) * t * t * t - (a + 3.0f) * t * t + 1.0f;}
static float C3(float t) {return a * t * t * t - 2.0f * a * t * t + a * t;}

void IMGMagnify(void)
{
  int in_width, in_height, out_width, out_height, larger_out_dimension;
  int n, d, j, k, l, m, index;
  //int lcs[MAX_OUT_DIMENSION];
  INT *lcs;
  //BYTE f[MAX_IN_HEIGHT][MAX_IN_WIDTH];
  //BYTE g[MAX_OUT_HEIGHT][MAX_OUT_WIDTH];
  float x;
  //float c[4][MAX_OUT_DIMENSION];
  float *tvc;
  //float h[MAX_IN_WIDTH];
  float *lph;


  larger_out_dimension = (out_width > out_height) ? out_width : out_height;
  lcs=ehAlloc(larger_out_dimension*sizeof(INT));
  tvc=ehAlloc(larger_out_dimension*4*sizeof(float));
  lph=ehAlloc(out_width*sizeof(float));
  for (k = 0; k < larger_out_dimension; k++) lcs[k] = (k * d) / n;

  for (k = 0; k < n; k++) {
    x = (float)((k * d) % n) / (float)n;
    tvc[(0+1)*k] = C0(x);
    tvc[(1+1)*[k] = C1(x);
    tvc[(2+1)*k] = C2(x);
    tvc[(3+1)*k] = C3(x);
  }

  for (k = n; k < larger_out_dimension; k++)
    for (l = 0; l < 4; l++)
      tvc[(l+1*k)] = tvc[(l+1)*(k % n)];

  for (k = 0; k < out_height; k++) {
    for (j = 0; j < in_width; j++) {
      h[j] = 0.0f;
      for (l = 0; l < 4; l++) {
        index = lcs[k] + l - 1;
        if ((index >= 0) && (index < in_height))
          h[j] += f[index][j] * c[3 - l][k];
      }
    }
    for (m = 0; m < out_width; m++) {
      x = 0.5f;
      for (l = 0; l < 4; l++) {
        index = lcs[m] + l - 1;
        if ((index >= 0) && (index < in_width))
          x += h[index] * c[3 - l][m];
      }
      if (x <= 0.0f)
        g[k][m] = 0;
      else if (x >= MAX_PIXEL_VALUE)
        g[k][m] = MAX_PIXEL_VALUE;
      else
        g[k][m] = (unsigned char)x;
    }
  }
  ehFree(lcs);
  ehFree(tvc);

}

*/


// ---------------------------------------------------
// JPGReadFileEx
// LEGGE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da leggere
// *HdlImage	 Puntatore all'handle che conterrà l'immagine
// TrueColor     TRUE=True Color/FALSE=256 colori
// *fStop        Puntatore ad un BOOL che determina lo stop del processo 
//               serve per applicazione Multi/Thread
//               NULL se non si usa
// Ritorna: 0 Errore
//          1 Tutto OK
/*
BOOL JPGReadFileEx(TCHAR *imageFileName,
				   INT *piImageHandle,
				   BOOL TrueColor,
				   BOOL *fStop,
				   INT iScale,
				   INT iDct, // JDCT_ISLOW (Default), JDCT_IFAST (Veloce ma meno accurato), JDCT_FLOAT
				   INT iModeError,
				   EN_PIXEL_TYPE enPixelType) // Fattore di scala 1/x 
{
  CHAR szMemoName[200];
  IMGHEADER sImgHead,*psImgHead;
  BYTE *GBuffer,*CB;
  INT hdlImage;
//  int register a;
  struct jpeg_decompress_struct cinfo;
  struct ima_error_mgr jerr;

  FILE * infile;		
  JSAMPARRAY buffer;	
  int row_stride;		
  LONG MemoSize;
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csJpg);
  *piImageHandle=-1;

  // Apertura del file
  if ((infile=_tfopen(imageFileName,TEXT("rb"))) == NULL) 
  {	 // win_infoarg("JPGReader:[%s] %d?",imageFileName,GetLastError());
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	  return 0;
  }
  
//  --------------------------------------------------------------------------------------
//  Fase 1: alloco ed inizializzo l'oggetto JPEG per la gestione degli errori
//  Settiamo la normale JPEG error routines, che avverra in uscita con errore.
//  --------------------------------------------------------------------------------------
  //memset(&cinfo,0,sizeof(cinfo));
  cinfo.err = jpeg_std_error(&jerr.pub);
  jpeg_create_decompress(&cinfo);

  cinfo.err = jpeg_std_error(&jerr.pub);

  //
  switch (iModeError)
  {
	case JME_HIDE:
	  jerr.pub.error_exit = ima_jpeg_error_exit_noview; // Dichiarata in testa
	  break;

	case JME_NORMAL:
	default:
	  jerr.pub.error_exit = ima_jpeg_error_exit; // Dichiarata in testa
	  break;
  }

  //cinfo.in_color_space = JCS_RGB; 
  //jpeg_set_defaults(&cinfo);
  // Stabilisco il salto di ritorno su errore nel mio contesto.
  // Se avviene un errore JPEG lo segnala
  // Noi dobbiamo pulire JPEG object,chiudere l'input file, e ritornare

  // Possiamo ora inizializzare l'oggetto JPEG per la decompressione
  //jpeg_create_decompress(&cinfo);
  if (setjmp(jerr.setjmp_buffer))  
	{jpeg_destroy_decompress(&cinfo); 
     fclose(infile);
	 if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	 return 0;
	}
  cinfo.dct_method=iDct;
//  cinfo.scale_num=1;
//  cinfo.scale_denom=32;

//  --------------------------------------------------------------------------------------
//  Fase 2 : specifico lo stream del file contenente il JPEG
//  --------------------------------------------------------------------------------------
  jpeg_stdio_src(&cinfo, infile);

//  --------------------------------------------------------------------------------------
//  Fase 3 : Leggo l'header del JPEG
//  --------------------------------------------------------------------------------------
  jpeg_read_header(&cinfo, TRUE);
  if (fStop) {if (*fStop) goto STOP1;}

  cinfo.dct_method=iDct;
  cinfo.scale_num=1;
  cinfo.scale_denom=iScale;

 //  --------------------------------------------------------------------------------------
//  Fase 4 : Setto i parametri per la decompressione
//  --------------------------------------------------------------------------------------

  if (!TrueColor)
  {
   if (cinfo.jpeg_color_space!=JCS_GRAYSCALE) 
   {
	cinfo.quantize_colors = TRUE;
	cinfo.desired_number_of_colors = 256;
   }
  }

  //cinfo.scale_num=1;
  //cinfo.scale_denom=2;

//  --------------------------------------------------------------------------------------
//  Fase 5 : Avvio il decompressore
//  --------------------------------------------------------------------------------------
  jpeg_start_decompress(&cinfo);

  // Potremmo avere bisogno di fare il setup di un nostro "own" in questo punto prima di leggere i dati.
  // Dopo il "jpeg_start_decompress()"  abbiamo la corretta dimensione dell'immagine disponibile.
  // e la colormap di uscita se desideriamo la quantizzazione del colore
  // delle dimensioni
  //Create(cinfo.image_width, cinfo.image_height, 8*cinfo.output_components);
  // Tira fuori il formato

//  --------------------------------------------------------------------------------------
//  FASE 6: Riempo la struttura IMGHEADER 
//  --------------------------------------------------------------------------------------
  // Specifico i dati dell'header
  //memset(&ImgHead,0,sizeof(ImgHead));

  _(sImgHead);
  sImgHead.enType=IMG_JPEG;
  sImgHead.enPixelType=IMG_PIXEL_RGB;
  strcpy(sImgHead.szFileName,imageFileName);
  sImgHead.iChannels=cinfo.output_components;
  sImgHead.lFileSize=fileSize(imageFileName);
  sImgHead.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
  sImgHead.bmiHeader.biWidth=cinfo.output_width;
  sImgHead.bmiHeader.biHeight=cinfo.output_height; // forse * -1
  sImgHead.bmiHeader.biPlanes=1;
  sImgHead.bmiHeader.biBitCount=8*cinfo.output_components;// bit colore
  sImgHead.bmiHeader.biCompression=BI_RGB;// Non compresso
  sImgHead.bmiHeader.biSizeImage=0;
  sImgHead.bmiHeader.biClrUsed=0;// Colori usati 0=Massimo
  sImgHead.bmiHeader.biClrImportant=0;// 0=Tutti i colori importanti
  
  // JSAMPLEs per riga dell'output buffer
  row_stride = cinfo.output_width * cinfo.output_components;
  sImgHead.linesize=row_stride;

  // Allineo a 32bit
  sImgHead.linesize=((sImgHead.linesize+3)>>2)<<2;
  sImgHead.lBitmapSize=ImgHead.bmiHeader.biHeight*ImgHead.linesize;

  MemoSize=sizeof(IMGHEADER)+ImgHead.lBitmapSize;

  // C'e' la pallette
  if (cinfo.output_components==1) MemoSize+=sizeof(RGBQUAD)*256;
  if (fStop) {if (*fStop) goto STOP2;}

  //win_infoarg("%d",cinfo.output_components);
#ifdef UNICODE
   UnicodeToChar(szMemoName,fileName(imageFileName));
#else
   strcpy(szMemoName,fileName(imageFileName));
#endif
  *HdlImage=memoAlloc(RAM_AUTO,MemoSize,szMemoName);
  if (*HdlImage<0) ehExit("No memory");

  GBuffer=memoLockEx(*HdlImage,"JPGReadFile"); 
  if (!GBuffer) ehExit("GBuffer = NULL hdl=%d",HdlImage);
  ImgHead.Offset=sizeof(IMGHEADER);
  // Carico la palettes
  if (ImgHead.bmiHeader.biBitCount==8)
  {
    if (cinfo.jpeg_color_space==JCS_GRAYSCALE) 
      CreateGrayColourMap(GBuffer+sizeof(ImgHead),256);
      else
	  SetPalette(GBuffer+sizeof(ImgHead),cinfo.actual_number_of_colors, cinfo.colormap[0], cinfo.colormap[1], cinfo.colormap[2]);
	  
	ImgHead.Offset+=(256*sizeof(RGBQUAD));
  }

  ehMemCpy(GBuffer,&ImgHead,sizeof(ImgHead));
  lpImgHead=(IMGHEADER *) GBuffer;
  GBuffer+=ImgHead.Offset;

  // Costruisco una riga/array di appoggio che che verra liberata quando l'immagine verra' fatta
  buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

//  --------------------------------------------------------------------------------------
//  Fase 7  : Loop sulle linee che rimangono da leggere
//  Usiamo per il loop una variabile di stato delle librerie cinfo.output_scanline
//  come contatore, cosi fancendo non .....
//  --------------------------------------------------------------------------------------

  CB=GBuffer;
  while (cinfo.output_scanline < cinfo.output_height) {
	 
	 jpeg_read_scanlines(&cinfo, buffer, 1);
     //if (cinfo.output_scanline <10) win_infoarg("%d",cinfo.output_scanline );
	 // Memorizzo le linee in un buffer
	 //memoWrite(*HdlImage,PtDest,buffer,row_stride);
	 //memset(GBuffer,120,row_stride);

	 ehMemCpy(GBuffer,buffer[0],ImgHead.linesize);
	 if (fStop) {if (*fStop) break;}

	 //PtDest+=row_stride;
     // memset(GBuffer,0,3*10);
	 GBuffer+=ImgHead.linesize;
  }

//  --------------------------------------------------------------------------------------
//  Fase 8
//  Fine della decompressione rilascio le memorie usate
//  --------------------------------------------------------------------------------------

STOP2:
  if (fStop) 
	{if (*fStop) jpeg_abort_decompress(&cinfo);}
	else
    jpeg_finish_decompress(&cinfo);

  // Ignoriamo un valore di fine sospessione possibile dello standard IO data source.


//  --------------------------------------------------------------------------------------
//  Fase 9
//  Rilascio il JPEG decompression Object
//  --------------------------------------------------------------------------------------

STOP1:

   jpeg_destroy_decompress(&cinfo);
   fclose(infile);

   // Richiesto stop
   if (fStop) 
   {
	 if (*fStop)
	 {
	   if (*HdlImage>0)
	   {
		 memoUnlockEx(*HdlImage,"JPGReadFile");	
		 memoFree(*HdlImage,"Stop");
		 *HdlImage=-1;
	   }
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
      return FALSE;
	 }
   }
   
//  --------------------------------------------------------------------------------------
//  Fase 10
//  Inverto i colori per quello stronzo di "windows" che li vuole in formato BGR
//  --------------------------------------------------------------------------------------

	switch (enPixelType)
	{
		case IMG_PIXEL_UNKNOW: // Compatibilita con il passato, formato non definito = Automatico
			if (cinfo.output_components==3) L_RGBSwapping(psImgHead);
			memoUnlockEx(*HdlImage,"JPGReadFile2");
			break;

		default:
			memoUnlockEx(*HdlImage,"JPGReadFile2");
			//IMGToRGB(*HdlImage)
			*HdlImage=IMGConvert(*HdlImage,enPixelType);
			break;
	}

	memoUnlockEx(*HdlImage,"JPGReadFile2");

//  --------------------------------------------------------------------------------------
//  E siamo fatti ....!!!
	if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csJpg);
	return 1;
}
*/

void IMGGetSize(INT hImage,SIZE *lps)
{
  IMGHEADER *Img;
  BITMAPINFOHEADER *BmpHeader;

  Img=memoLockEx(hImage,"IMGGetSize"); if (Img==NULL) ehExit("IMGGetSize");
  BmpHeader=(BITMAPINFOHEADER *)  &Img->bmiHeader;
  lps->cy=BmpHeader->biHeight;
  lps->cx=BmpHeader->biWidth;
  memoUnlockEx(hImage,"GetSize");
}
/*
static void BmpMaskDispRedim(INT x,INT y,
							 INT iLxOriginal,INT iLyOriginal,
							 INT iLxReal,INT iLyReal,
							 HBITMAP BitMap,
							 HBITMAP MaskBit,
							 HDC hDC)
{
	// Creo un area di memoria compatibile al DC
	// Seleziono l'oggetto BITMAP nella memoria nuova

	// -----------------------------------------------------
	// Trasferisco il bitmap dalla Copia --> Al DCwindows  !
	// Versione senza maschera                             !
	// -----------------------------------------------------

	if (MaskBit==NULL)
	 {
		HDC hdcMemory;
		hdcMemory = CreateCompatibleDC(hDC);
		if (hdcMemory==NULL) ehExit("BMD:4");

		SelectObject(hdcMemory, BitMap);
		*/
		/*
		BitBlt(hDC,  // --- DESTINAZIONE ---
			   x, y, // Coordinate X-Y
			   lx, // Larghezza
			   ly, //Altezza
			   hdcMemory, // --- SORGENTE ---
			   0, 0, //Cordinate x-y
			   SRCCOPY);
			   */
		/*
    if (StretchDIBits(hDC, 
					  x,y
					  TileX,iVerticalSector,
					  0,0,
					  TileX,iVerticalSector,
					  Sorg,
					  BmpInfo,
					  DIB_RGB_COLORS, 
					  SRCCOPY) == GDI_ERROR) 
*/
/*
		DeleteDC(hdcMemory);
	 }
	 else
	// -----------------------------------------------------
	// Trasferisco il bitmap dalla Copia --> Al DCwindows  !
	// Versione con la MASKERA                             !
	// -----------------------------------------------------
	 {
		HDC hdcMemory;
		HDC hdcMem2;
		RECT Dim_xy;	

		// Creo i due DC
		hdcMemory = CreateCompatibleDC(hDC); if (hdcMemory==NULL) ehExit("BMD:1");
		hdcMem2 = CreateCompatibleDC(hDC); if (hdcMem2==NULL) ehExit("BMD:2");
		SelectObject(hdcMem2, MaskBit);
		SetTextColor(hDC,sys.arsColor[15]);
		SetBkColor(hDC,sys.arsColor[0]);

	    // ----------------------
	    // Rovescio la maskera  !
	    // ----------------------
		Dim_xy.left=0; Dim_xy.top=0;
		Dim_xy.right=lx; Dim_xy.bottom=ly;

		// ----------------------
		// Buco con la maskera  !
		// ----------------------
		BitBlt(hDC,  // --- DESTINAZIONE ---
			   x, y, // Coordinate X-Y
			   lx, // Larghezza
			   ly, //Altezza
			   hdcMem2, // --- SORGENTE ---
			   0, 0, //Cordinate x-y
			   SRCAND); //Questo ma invertito

		// RiRovescio la Maskera
		//InvertRect(hdcMem2, &Dim_xy);
	    // -----------------------------------
	    // Buco l'icone Pulisco l'icona      !
	    // -----------------------------------
	    SelectObject(hdcMem2, BitMap);
	    SelectObject(hdcMemory, MaskBit);

	    BitBlt(hdcMem2,  // --- DESTINAZIONE ---
			   0, 0, // Coordinate X-Y
			   lx, // Larghezza
			   ly, //Altezza
			   hdcMemory, // --- SORGENTE ---
			   0, 0, //Cordinate x-y
			   SRCAND);
		// ----------------------
		// Stampo l'icona       !
		// ----------------------

		BitBlt(hDC,  // --- DESTINAZIONE ---
			   x, y, // Coordinate X-Y
			   lx, // Larghezza
			   ly, //Altezza
			   hdcMem2, // --- SORGENTE ---
			   0, 0, //Cordinate x-y
			   SRCPAINT);

		DeleteDC(hdcMem2);
	    DeleteDC(hdcMemory);
	 }
}
*/


//#if (!defined(_NODC)&&!defined(EH_MOBILE))
#ifdef EH_WIN_GDI 
// Ex IcoToImage
INT IMGFromIcone(CHAR * lpIcone,INT iLx,INT iLy,struct ICO_HEAD *lpHead)
{
	EH_ICON *	psIcon;
	EH_ICOHEAD *head;
	WORD    a=1;
	CHAR    *sorg,*mask;
	BOOL	fLock=FALSE;
	INT	HdlImage=0;//,HdlImage2;

	// Cerco l'icone

    if (lpIcone==NULL) return -1;
	sorg=ico_getptr(lpIcone,&fLock,&psIcon); if (!sorg) return -1;
	sorg+=psIcon->dwOffset;

	// DATI DELL'ICONE                     
	head=(EH_ICOHEAD *) sorg;
	if (lpHead) ehMemCpy(lpHead,head,sizeof(EH_ICOHEAD));
	mask=sorg+head->ofs_mask;
	sorg+=head->ofs_icone;

//	IMGCreate();
	ehError();
/*
	// Creo un IMGHEADER virtuale
	_(ImgHead);
	ImgHead.enPixelType=IMG_PIXEL_BGR;
#ifdef UNICODE
	CharToUnicode(ImgHead.FileName,lpIcone);
#else
	strcpy(ImgHead.tszFileName,lpIcone);
#endif
	ImgHead.lFileSize=psIcon->dwSize;
    ImgHead.iChannels=3;
	ImgHead.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
	ImgHead.bmiHeader.biWidth=head->dimx;
	ImgHead.bmiHeader.biHeight=head->dimy; // forse * -1
	ImgHead.bmiHeader.biPlanes=1;
	ImgHead.bmiHeader.biBitCount=8*3;// bit colore
	ImgHead.bmiHeader.biCompression=BI_RGB;// Non compresso
	ImgHead.bmiHeader.biSizeImage=0;
	ImgHead.bmiHeader.biClrUsed=0;// Colori usati 0=Massimo
	ImgHead.bmiHeader.biClrImportant=0;// 0=Tutti i colori importanti

    // Calcolo ed allineo a 32bit
	ImgHead.linesize=ImgHead.bmiHeader.biWidth*3;
    ImgHead.linesize=((ImgHead.linesize+3)>>2)<<2;
	ImgHead.dwBitmapSize=ImgHead.bmiHeader.biHeight*ImgHead.linesize;

	MemoSize=sizeof(IMGHEADER);
    MemoSize+=sizeof(RGBQUAD)*256;
	MemoSize+=psIcon->dwSize;
	HdlImage=memoAlloc(RAM_AUTO,MemoSize,"ico_dispEx"); if (HdlImage<0) ehExit("No memory");
	GBuffer=memoLockEx(HdlImage,"ico_dispEx"); 
    ImgHead.Offset=sizeof(ImgHead);
	ImgHead.Offset+=(256*sizeof(RGBQUAD));

    ehMemCpy(GBuffer,&ImgHead,sizeof(ImgHead));
    GBuffer+=ImgHead.Offset;
	ehMemCpy(GBuffer,sorg,psIcon->dwSize);
	if (fLock) memoUnlock(psIcon->wHdl);
	memoUnlock(HdlImage);

	// Misure originali
	if (!iLy&&!iLx) 
	{

	}
	else
	{ 
	 // head->dimx:iLx=head->dimy:y;
	 if (!iLy) {iLy=iLx*head->dimy/head->dimx;}
	 // head->dimx:iLx=head->dimy:y;
	 if (!iLx) {iLx=iLy*head->dimx/head->dimy;}
	 // Ridimensiono
	 HdlImage2=IMGRemaker(HdlImage,NULL,iLx,iLy,TRUE,1);
	 memoFree(HdlImage,"ok");
	 HdlImage=HdlImage2;
	}
*/

	return HdlImage;
}

#endif

// -------------------------------------------------------------
// Librerie PNG
// -------------------------------------------------------------

#ifdef EH_PNG

static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;

static void
png_cexcept_error(png_structp png_ptr, png_const_charp msg)
{
   if (png_ptr)
     ;
#ifndef PNG_NO_CONSOLE_IO
   fprintf(stderr, "libpng error: %s\n", msg);
#endif
   ehLogWrite("libpng error: %s\n", msg);
   {
      Throw msg;
   }
}

// PNG image handler functions
static BOOL PngLoadImage(TCHAR *pstrFileName, INT *lpHdlImage,INT *lpiErr,IMGHEADER *pImgHead);

//
// PNGReadHeader()
//

BOOL PNGReadHeader(TCHAR *imageFileName,IMGHEADER *psImgHead,INT iModeError)
{
  INT iErr,iErr2;
  //if (!lpiErr) lpiErr=&iErr2;//ehExit("PNGReadFile(): lpiErr ??");
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csPng);
  iErr=PngLoadImage(imageFileName, 
					NULL, // Non handle per avere solo header
					&iErr2,
					psImgHead);
  if (iErr) {
		if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csPng);
		return 0;
	}
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csPng);
  return 1;
}

// ---------------------------------------------------
// PNGReadFile
// LEGGE UN FILE IN FORMATO PNG
//
// imageFileName Nome del file da leggere
// *HdlImage	 Puntatore all'handle che conterrà l'immagine
// Ritorna: 0 Errore
//          1 Tutto ok (rovesciato nel 2009
//
//static BOOL PngLoadImage (PTSTR pstrFileName, INT *lpHdlImage);

BOOL PNGReadFile(TCHAR *imageFileName,
				 INT *lpHdlImage,
				 INT *lpiErr,
				 BOOL iModeError)
{
  INT iErr,iErr2;
  if (!lpiErr) lpiErr=&iErr2;//ehExit("PNGReadFile(): lpiErr ??");
  if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csPng);
  iErr=PngLoadImage(imageFileName, lpHdlImage,lpiErr,NULL);
  if (!iErr)
		{
  			if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csPng);
			//if (lpiErr) *lpiErr=-1;
			return 1;
		}
  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csPng);
  return 0;
}

// Ritorna FALSE= Tutto ok
//
// PngLoadImage() - CORE
//
static BOOL PngLoadImage(TCHAR *	pstrFileName, 
						 INT *		piImageHandle,
						 INT *		lpiErr,
						 IMGHEADER *psImgHead) // psImgHead !=NULL: caricolo solo l'header, NULL= Caricolo l'immagine
{
	// IMGHEADER ImgHead;
    static FILE        *pfFile;
    png_byte            pbSig[8];
    int                 iBitDepth;
    int                 iColorType;
    double              dGamma;
    png_color_16       *pBackground;
    png_uint_32         ulChannels;
    png_uint_32         ulRowBytes;
    png_byte           *pbImageData = NULL;//*ppbImageData;
    static png_byte   **ppbRowPointers = NULL;
    int                 i;
	int iWidth;
	int iHeight;
	BOOL bOnlyHeader=FALSE;
	INT hdlImage;

	*lpiErr=0; // Eccezione
    // open the PNG input file
    if (strEmpty((CHAR *) pstrFileName))  {
		*lpiErr=-1; // Manca il nome del file
        return TRUE;
    }

    if (!(pfFile = fopen((CHAR *) pstrFileName, "rb")))
    {
		*lpiErr=-2; // Il file non si può aprire
        return TRUE;
    }

    // first check the eight byte PNG signature

    fread(pbSig, 1, 8, pfFile);
    if (!png_check_sig(pbSig, 8))
    {
		*lpiErr=-2; // Non si può leggere la firma
		fclose(pfFile);
        return TRUE;
    }

    // create the two png(-info) structures

	// Creo struttura di appoggio
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
									 NULL,
									 (png_error_ptr)png_cexcept_error, 
									 (png_error_ptr)NULL);
    if (!png_ptr)  {
		*lpiErr=-3; // Non posso creare la struttura di ptr
		fclose(pfFile);
        return TRUE;
    }

	// Creo struttura info di appoggio
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)  {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
		*lpiErr=-4; // Non posso creare la struttura di informazioni
		fclose(pfFile);
        return TRUE;
    }
/*
	// Struttura di Fine
	end_ptr = png_create_info_struct(png_ptr);
	if (!end_ptr) 
	{
		*lpiErr=-5; // Non posso creare la struttura di fine
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
		fclose(pfFile);
		return 4; 
	}
*/
    Try
    {
        png_init_io(png_ptr, pfFile);
        png_set_sig_bytes(png_ptr, 8);
        
        // read all PNG info up to image data
        
        png_read_info(png_ptr, info_ptr);
        
        // get width, height, bit-depth and color-type
        // Leggo le informazioni del Png
        png_get_IHDR(png_ptr, 
					 info_ptr, 
					 &iWidth, 
					 &iHeight, 
					 &iBitDepth,
					 &iColorType, 
					 NULL, 
					 NULL, 
					 NULL);
        
        // Espando l'immaggine con tutti i color e tuttie le pfotondita in una 3x8 bit RGB images
        // let the library process things like alpha, transparency, background
     //   if (!bOnlyHeader)
		{
			if (iBitDepth == 16) png_set_strip_16(png_ptr);
			if (iColorType == PNG_COLOR_TYPE_PALETTE) png_set_expand(png_ptr);
			if (iBitDepth < 8) png_set_expand(png_ptr);
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_expand(png_ptr);

			if (iColorType == PNG_COLOR_TYPE_GRAY ||
				iColorType == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);
	        
			// set the background color to draw transparent and alpha images over.
			if (png_get_bKGD(png_ptr, info_ptr, &pBackground))
			{
				png_set_background(png_ptr, pBackground, PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
			}
			else
			{
				//pBkgColor = NULL;
			}
	        
			// if required set gamma conversion
			if (png_get_gAMA(png_ptr, info_ptr, &dGamma)) png_set_gamma(png_ptr, (double) 2.2, dGamma);
		}
        
        // after the transformations have been registered update info_ptr data
        png_read_update_info(png_ptr, info_ptr);
        
        // get again width, height and the new bit-depth and color-type
		// Rileggo i dati aggiornati
        png_get_IHDR(png_ptr, 
					 info_ptr, 
					 &iWidth, 
					 &iHeight, 
					 &iBitDepth,
					 &iColorType, 
					 NULL, 
					 NULL, 
					 NULL);
        
        // row_bytes is the width x number of channels
        
        ulRowBytes = png_get_rowbytes(png_ptr, info_ptr);
        ulChannels = png_get_channels(png_ptr, info_ptr);

		if (psImgHead) bOnlyHeader=TRUE; else bOnlyHeader=FALSE;

		//
		// Riempo la struttura EasyHand
		//
		switch (iColorType)
		{ 
			case PNG_COLOR_TYPE_RGB:
				hdlImage=IMGCreate(IMG_PIXEL_RGB,fileName(pstrFileName),iWidth,iHeight,psImgHead,bOnlyHeader);
				break;

			case PNG_COLOR_TYPE_RGB_ALPHA:
				hdlImage=IMGCreate(IMG_PIXEL_RGB_ALPHA,fileName(pstrFileName),iWidth,iHeight,psImgHead,bOnlyHeader);
				break; 

			default:
				ehError();
		}

		if (!bOnlyHeader)
		{
			if (hdlImage<0) ehExit("PNG:No memory");
			psImgHead=memoLock(hdlImage);
		}

		psImgHead->enType=IMG_PNG;
		strcpy(psImgHead->utfFileName,fileName(pstrFileName));
		strcpy(psImgHead->utfFullFileName,pstrFileName);
		psImgHead->lFileSize=fileSize(pstrFileName);
		psImgHead->enFilePixelType=psImgHead->enPixelType;

		// Cerco memoria necessaria
		if (!bOnlyHeader)
		{
			pbImageData=psImgHead->pbImage;
			//ImgHead.Offset=sizeof(ImgHead);
	        
			if ((ppbRowPointers = (png_bytepp) ehAlloc(iHeight * sizeof(png_bytep))) == NULL) {
				png_error(png_ptr, "Visual PNG: out of memory");
			}
	        
			// set the individual row-pointers to point at the correct offsets
			for (i = 0; i < iHeight; i++) ppbRowPointers[i] = pbImageData + (i * psImgHead->linesize);
	        
			// now we can go ahead and just read the whole image
			// Carico l'immagine in memoria passando la struttura e l'array di puntatori
			png_read_image(png_ptr, ppbRowPointers);
	        
			// read the additional chunks in the PNG file (not really needed)
			//png_read_end(png_ptr, NULL);
			png_read_end(png_ptr, info_ptr); 
			// and we're done
	        
			ehFree(ppbRowPointers); ppbRowPointers = NULL;
			memoUnlockEx(hdlImage,"PNGReadFile2");
		}

		//png_read_destroy(&png_ptr, &info_ptr, NULL);
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		png_ptr=NULL; info_ptr=NULL;
        fclose(pfFile); pfFile=NULL;

		if (piImageHandle) *piImageHandle=hdlImage;

        // yepp, done
    }

	Catch (msg)
    {
    //    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		png_ptr=NULL; info_ptr=NULL;

        //*ppbImageData = pbImageData = NULL;
        
        if (ppbRowPointers) ehFree(ppbRowPointers);
        if (pfFile) fclose(pfFile);
		*lpiErr=-100;
		if (piImageHandle) *piImageHandle=-1;
        return TRUE;
    }

	// if (psImgHead) memcpy(psImgHead,&ImgHead,sizeof(ImgHead));
	if (pfFile) fclose(pfFile);
    return FALSE;
}

//
// PngSaveImage()
//

BOOL PngSaveImage(PTSTR pstrFileName, INT hdlImage)
{
	INT			iFormat=1;

    static FILE        *pfFile;
    png_uint_32         ulRowBytes;
    static png_byte   **ppbRowPointers = NULL;
    int                 i;
	IMGHEADER *psImg;
	png_byte *	pDiData;

    // open the PNG output file

    if (strEmpty(pstrFileName)) return FALSE;
    if (!(pfFile = fopen(pstrFileName, "wb"))) return FALSE;

	psImg=memoLock(hdlImage);
	pDiData=psImg->pbImage;

//	if (psImg->fRGBInverted) L_RGBSwapping(psImg,pDiData); // ATTEZIUONE: E' distruttivo, dovrei rigirarlo alla fine
	switch (psImg->enPixelType)
	{
		case IMG_PIXEL_BGR: 
			IMG_RGBSwapping(psImg); // ATTEZIUONE: E' distruttivo, dovrei rigirarlo alla fine
			break;

		case IMG_PIXEL_RGB: 
			break;

		case IMG_PIXEL_RGB_ALPHA:
			iFormat=0;
			break;
		
		default:
			ehError();
	}


	//
    // a) Preparo una struttura per scrivere
	//
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, (png_error_ptr) png_cexcept_error, (png_error_ptr) NULL);
    if (!png_ptr)
    {
        fclose(pfFile);
        return FALSE;
    }

	//
	// b) Preparo una struttura di informazione
	//
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fclose(pfFile);
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return FALSE;
    }

    Try
    {
		//
        // c) Inizioalizzo il file
        //
#if !defined(PNG_NO_STDIO)
        png_init_io(png_ptr, pfFile);
#else
        png_set_write_fn(png_ptr, (png_voidp)pfFile, png_write_data, png_flush);
#endif

		// Indico come voglio scrivere
		switch(iFormat)
		{
			case 0:
				png_set_IHDR(png_ptr, 
							 info_ptr, 
							 psImg->bmiHeader.biWidth,//iWidth, 
							 psImg->bmiHeader.biHeight,//iHeight, 							
							 8, 
							 PNG_COLOR_TYPE_RGB_ALPHA, 
							 PNG_INTERLACE_NONE, 
							 PNG_COMPRESSION_TYPE_DEFAULT, 
							 PNG_FILTER_TYPE_DEFAULT);
				/*
				_(SigBits);
				SigBits.red = 8;
				SigBits.green = 8;
				SigBits.blue = 8;
				SigBits.alpha = 8;
				png_set_sBIT(png_ptr, info_ptr, &SigBits);
				*/
				break;


			case 1: // Senza Alpha channerl
				png_set_IHDR(png_ptr, 
							 info_ptr, 
							 psImg->bmiHeader.biWidth,//iWidth, 
							 psImg->bmiHeader.biHeight,//iHeight, 							
							 8, 
							 PNG_COLOR_TYPE_RGB, 
							 PNG_INTERLACE_NONE, 
							 PNG_COMPRESSION_TYPE_DEFAULT, 
							 PNG_FILTER_TYPE_DEFAULT);
/*
					
					PngStruct, PngInfo, Width, Height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
				SigBits.red = 8;
				SigBits.green = 8;
				SigBits.blue = 8;
				png_set_sBIT(PngStruct, PngInfo, &SigBits);
				*/
				break;
/*
			case ImgA8:
				png_set_IHDR(PngStruct, PngInfo, Width, Height, 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
				SigBits.gray = 8;
				png_set_sBIT(PngStruct, PngInfo, &SigBits);
				break;

			case ImgA16:
				png_set_IHDR(PngStruct, PngInfo, Width, Height, 16, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
				SigBits.gray = 16;
				png_set_sBIT(PngStruct, PngInfo, &SigBits);
				break;

			case ImgR8G8B8:
				png_set_IHDR(PngStruct, PngInfo, Width, Height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
				SigBits.red = 8;
				SigBits.green = 8;
				SigBits.blue = 8;
				png_set_sBIT(PngStruct, PngInfo, &SigBits);
				break;
*/

			default:
				//png_destroy_write_struct(&PngStruct, &PngInfo);
				//return false;
				ehError();
			}

		/*

        // we're going to write a very simple 3x8 bit RGB image
        png_set_IHDR(png_ptr, 
					 info_ptr, 
					 psImg->bmiHeader.biWidth,//iWidth, 
					 psImg->bmiHeader.biHeight,//iHeight, 
					 ciBitDepth,
					 PNG_COLOR_TYPE_RGB_ALPHA,//PNG_COLOR_TYPE_RGB, 
					 PNG_INTERLACE_NONE, 
					 PNG_COMPRESSION_TYPE_BASE,
					 PNG_FILTER_TYPE_BASE);
        

*/

        // write the file header information
        png_write_info(png_ptr, info_ptr);
        
        // swap the BGR pixels in the DiData structure to RGB
        // png_set_bgr(png_ptr);
        
        // row_bytes is the width x number of channels
        
        //ulRowBytes = iWidth * ciChannels;
		ulRowBytes=psImg->linesize;
        
        // we can allocate memory for an array of row-pointers
        
        if ((ppbRowPointers = (png_bytepp) ehAlloc(psImg->bmiHeader.biHeight * sizeof(png_bytep))) == NULL) ehExit("Visualpng: Out of memory");
        
        // set the individual row-pointers to point at the correct offsets
        
        for (i = 0; i < psImg->bmiHeader.biHeight; i++)
//            ppbRowPointers[i] = pDiData + i * (((ulRowBytes + 3) >> 2) << 2);
            ppbRowPointers[i] = pDiData + i * psImg->linesize;//(((ulRowBytes + 3) >> 2) << 2);
        
        // write out the entire image data in one call
        
//		png_set_rows(png_ptr, info_ptr, ppbRowPointers);
//		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR | PNG_TRANSFORM_SWAP_ALPHA, NULL); // g_PngTransforms = PNG_TRANSFORM_BGR | PNG_TRANSFORM_SWAP_ALPHA

        //
		// e) Scrivo L'immagine
		//
		png_write_image (png_ptr, ppbRowPointers);
        
		//
        // f) write the additional chunks to the PNG file (not really needed)
        //
        png_write_end(png_ptr, info_ptr);
        
        // and we're done
        
        ehFree(ppbRowPointers); ppbRowPointers=NULL;
        
        // g) clean up after the write, and free any memory allocated
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);

/*



png_byte ** RowPtrs = new png_byte *[Height];

int Pitch = Width * GetBytesPerPixel();
for(int y = 0; y < Height; ++y)
	RowPtrs[y] = &m_ImageData[y * Pitch];

png_set_rows(PngStruct, PngInfo, RowPtrs);

png_write_png(PngStruct, PngInfo, g_PngTransforms, NULL); // g_PngTransforms = PNG_TRANSFORM_BGR | PNG_TRANSFORM_SWAP_ALPHA

png_destroy_write_struct(&PngStruct, &PngInfo);

delete [] RowPtrs;

return true;
*/

        // yepp, done
    }

    Catch (msg)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);

        if(ppbRowPointers) ehFree(ppbRowPointers);

        fclose(pfFile);
		memoUnlock(hdlImage);
        return FALSE;
    }
    
	memoUnlock(hdlImage);
    fclose (pfFile);
    return TRUE;
}


// 
// PNGSaveFile
// SCRIVE UN FILE IN FORMATO JPEG
//
// imageFileName Nome del file da scrivere
// HdlImage	     Hdl che contiene l'immagine in formato IMGHEADER
// iQuality      Fattore di qualità dell'immagine
// 0= errore >0 tutto ok
//
BOOL PNGSaveFile(TCHAR *imageFileName,INT HdlImage,INT iQuality)
{
	FILE * outfile;		
	INT rt=TRUE;
	CHAR szError[300];

	if (iIMGMode==IMG_MULTITHREAD) EnterCriticalSection(&csPng);

	if ((outfile=fopen(imageFileName,"wb")) == NULL) 
	{	 
#ifdef EH_CONSOLE
	  printf("PNGSaveFile():Errore in creazione file [%s] [%s]",imageFileName,osErrorStr(0,szError,sizeof(szError)));
#else
	  win_infoarg("PNGSaveFile():Errore in creazione file [%s] [%s]",imageFileName,osErrorStr(0,szError,sizeof(szError)));
#endif
	  if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csPng);
	  return 0;
	}
	fclose(outfile);

	PngSaveImage (imageFileName, HdlImage);
	if (iIMGMode==IMG_MULTITHREAD) LeaveCriticalSection(&csPng);
	return rt;
}


#ifdef PNG_NO_STDIO

static void
png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_size_t check;

   /* fread() returns 0 on error, so it is OK to store this in a png_size_t
    * instead of an int, which is what fread() actually returns.
    */
   check = (png_size_t)fread(data, (png_size_t)1, length,
      (FILE *)png_ptr->io_ptr);

   if (check != length)
   {
      png_error(png_ptr, "Read Error");
   }
}

static void
png_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
   png_uint_32 check;

   check = fwrite(data, 1, length, (FILE *)(png_ptr->io_ptr));
   if (check != length)
   {
      png_error(png_ptr, "Write Error");
   }
}

static void
png_flush(png_structp png_ptr)
{
   FILE *io_ptr;
   io_ptr = (FILE *)CVT_PTR((png_ptr->io_ptr));
   if (io_ptr != NULL)
      fflush(io_ptr);
}

#endif

//-----------------
//  end of source
//-----------------

void * srvPng(INT cmd,INT info,CHAR *ptr)
{
	static _DMI dmiPng=DMIRESET;
	INT hdlPng;
	static PNG_MEMO sPing;
	CHAR *p;
	INT a;

	switch (cmd)
	{
		case WS_OPEN:
			if (dmiPng.Hdl>0) srvPng(WS_CLOSE,0,NULL);
			DMIOpen(&dmiPng,RAM_AUTO,info,sizeof(PNG_MEMO),"PingList");
			memoLock(dmiPng.Hdl);
			break;

		case WS_FIND:
			for (a=0;a<dmiPng.Num;a++)
			{
				DMIRead(&dmiPng,a,&sPing);
				if (!strcmp(sPing.szNome,ptr)) return &sPing;
			}
			return NULL;
		
		case WS_DEL:
			if (!ptr)
			{
				for (a=0;a<dmiPng.Num;a++)
				{
					DMIRead(&dmiPng,a,&sPing);
					if (sPing.iGrp==info) 
					{
						memoFree(sPing.iHdl,"png");			
						DMIDelete(&dmiPng,a,&sPing); a--;
					}
				}
			}
			break;

		case WS_ADD:
			p=strstr(ptr,"|"); if (p) *p=0;
			if (PNGReadFile(ptr,&hdlPng,NULL,0))
			{
				_(sPing);
				if (p) 
				{
					strcpy(sPing.szNome,p+1);
				}
				else
				{
					strcpy(sPing.szNome,fileName(ptr));
					p=strReverseStr(sPing.szNome,"."); if (p) *p=0;
				}
				strupr(sPing.szNome);
				sPing.iHdl=hdlPng;
				sPing.iGrp=info;
				//win_infoarg("%s",sPing.szNome);
				DMIAppend(&dmiPng,&sPing);
			}
			break;

		case WS_CLOSE:
			for (a=0;a<dmiPng.Num;a++)
			{
				DMIRead(&dmiPng,a,&sPing);
				memoFree(sPing.iHdl,"png");			
			}
			DMIClose(&dmiPng,"PingList");
			break;
	}
	return NULL;
}


#endif // EH_PNG


// 6 Settembre 2005
// void CMYKtoRGB(S_CMYK *lpCmyk,S_RGB lpRgb)
// tRGB = {1 - (C(1 - K) + K),1 - (M(1 - K) + K),1 - (Y(1 - K) + K)} = {1 - C(1 - K) - K,1 - M(1 - K) - K,1 - Y(1 - K) - K} 
/*
void CMYKtoRGB(S_CMYK *lpCmyk,S_RGB *lpRgb)
{
    double c, m, y, k;
	
	if (!lpCmyk) return;
	if (!lpRgb) return;

	// Convertire in double il dato
	// x:1.0=k:255;
    c = (double) ((255-lpCmyk->c) / 255.0);
    m = (double) ((255-lpCmyk->m) / 255.0);
    y = (double) ((255-lpCmyk->y) / 255.0);
    k = (double) ((255-lpCmyk->k) / 255.0);
    if (k < 1.0)
	{
      c = c * (1.0 - k) + k;
      m = m * (1.0 - k) + k;
      y = y * (1.0 - k) + k;
    }
  else
    {
      c = 1.0;
      m = 1.0;
      y = 1.0;
    }

  // L = 0.3R+0.59G+0.11B

  // x:255=k:1
  lpRgb->r = (BYTE) (double) ((1.0 - c) * 255.0);
  lpRgb->g = (BYTE) (double) ((1.0 - m) * 255.0);
  lpRgb->b = (BYTE) (double) ((1.0 - y) * 255.0);
  //rgb->a = cmyk->a;
}
*/

// 02/2010
static void CMYKtoRGB(DWORD *lpCmyk,DWORD *lpRgb)
{
    double c, m, y, k;

	// Convertire in double il dato
	// x:1.0=k:255;
    c = (double) ((255-(*lpCmyk&0xFF)) / 255.0);
    m = (double) ((255-((*lpCmyk>>8)&0xFF)) / 255.0);
    y = (double) ((255-((*lpCmyk>>16)&0xFF)) / 255.0);
    k = (double) ((255-((*lpCmyk>>24)&0xFF)) / 255.0);
    if (k < 1.0)
	{
      c = c * (1.0 - k) + k;
      m = m * (1.0 - k) + k;
      y = y * (1.0 - k) + k;
    }
  else
    {
      c = 1.0;
      m = 1.0;
      y = 1.0;
    }

  // L = 0.3R+0.59G+0.11B
  *lpRgb= (BYTE) (double) ((1.0 - c) * 255.0) |
		  ((BYTE) (double) ((1.0 - m) * 255.0) << 8) |
		  ((BYTE) (double) ((1.0 - y) * 255.0) << 16);
}

static void CMYKtoBGR(DWORD *lpCmyk,DWORD *lpRgb)
{
    double c, m, y, k;

	// Convertire in double il dato
	// x:1.0=k:255;
    c = (double) ((255-(*lpCmyk&0xFF)) / 255.0);
    m = (double) ((255-((*lpCmyk>>8)&0xFF)) / 255.0);
    y = (double) ((255-((*lpCmyk>>16)&0xFF)) / 255.0);
    k = (double) ((255-((*lpCmyk>>24)&0xFF)) / 255.0);
    if (k < 1.0)
	{
      c = c * (1.0 - k) + k;
      m = m * (1.0 - k) + k;
      y = y * (1.0 - k) + k;
    }
  else
    {
      c = 1.0;
      m = 1.0;
      y = 1.0;
    }

  // L = 0.3R+0.59G+0.11B
  *lpRgb= ((BYTE) (double) ((1.0 - c) * 255.0) << 16) |
		  ((BYTE) (double) ((1.0 - m) * 255.0) << 8) |
		  ((BYTE) (double) ((1.0 - y) * 255.0) );
}

//void CMYKtoBGR(DWORD *lpCmyk,DWORD *lpRgb)

//
// Controlla che l'immagine sia un JPG RGB
// Se non è così la converte a RGB e ritorna il nuovo Handle liberando il vecchio
//

//INT IMGToRGB(INT hdlImage)
INT IMGConvert(INT hdlImageSource,EN_PIXEL_TYPE enPixelTypeDest)
{
	IMGHEADER *psImgSorg;
	IMGHEADER *psImgDest;
//	BYTE *lpi;
	BYTE *lpSorg,*lpDest;
	INT hdlNew;

    INT kx,ky;
    BYTE *ps,*pd;

	BOOL bNotSupport=FALSE; // Conversione non supportata
	BOOL bNotChange=FALSE;	// Non è avvenuta nessuna conversione
	int * p32;
	EN_PIXEL_TYPE enPixelTypeSource;

	psImgSorg=memoLock(hdlImageSource); lpSorg=psImgSorg->pbImage;

	// Non ha bisogno di conversione
	if (psImgSorg->enPixelType==enPixelTypeDest)
	{
		memoUnlockEx(hdlImageSource,"imgc1");
		return hdlImageSource;
	}

	//
	// Creo la Nuova immagine
	//
	hdlNew=IMGCreate(enPixelTypeDest,
					 "NewImage",
				 	 psImgSorg->bmiHeader.biWidth,
					 psImgSorg->bmiHeader.biHeight,
					 FALSE,
					 0);

	psImgDest=memoLockEx(hdlNew,"newImage");

	strcpy(psImgDest->utfFileName,psImgSorg->utfFileName);
	strcpy(psImgDest->utfFullFileName,psImgSorg->utfFullFileName);
	psImgDest->enType=psImgSorg->enType;
	psImgDest->enFilePixelType=psImgSorg->enFilePixelType;
 	lpDest=psImgDest->pbImage;
	enPixelTypeSource=psImgSorg->enPixelType;
	switch (enPixelTypeSource)
	{

		//
		// SCALA DI GRIGIO >
		//
		case IMG_PIXEL_COLOR1: 

			switch (enPixelTypeDest)
			{
				case IMG_PIXEL_BGR:
				case IMG_PIXEL_RGB:

					// B) Effettuo la trasformazione
					// Loop sul sorgente scrittura del destinatario
					for (ky=0;ky<(INT) psImgSorg->bmiHeader.biHeight;ky++)//cinfo.image_height;ky++)
					{
						
						
						//spsImgSorg->psRgbPalette->
						ps=lpSorg; pd=lpDest;
						for (kx=0;kx<psImgSorg->bmiHeader.biWidth;kx++)
						{
							INT idx=kx>>3;
							INT idxBitColor=(ps[idx]&(0x80>>(kx&7)))?1:0;
							memcpy(pd,&psImgSorg->psRgbPalette[idxBitColor],3); pd+=3;
						}

						lpSorg+=psImgSorg->linesize;
						lpDest+=psImgDest->linesize;
					}
					break;

				default: 
					bNotSupport=TRUE;
					break;
			}
			break;

		//
		// SCALA DI GRIGIO >
		//
		case IMG_PIXEL_GRAYSCALE: 

			switch (enPixelTypeDest)
			{
				case IMG_PIXEL_BGR:
				case IMG_PIXEL_RGB:

					// B) Effettuo la trasformazione
					// Loop sul sorgente scrittura del destinatario
					for (ky=0;ky<(INT) psImgSorg->bmiHeader.biHeight;ky++)//cinfo.image_height;ky++)
					{
						ps=lpSorg; pd=lpDest;
						for (kx=0;kx<psImgSorg->linesize;kx++)
						{
							*pd++=*ps; *pd++=*ps; *pd++=*ps; ps++;
						}
						lpSorg+=psImgSorg->linesize;
						lpDest+=psImgDest->linesize;
					}
					break;

				default: 
					bNotSupport=TRUE;
					break;
			}
			break;

		//
		// RGB
		//

		case IMG_PIXEL_RGB: // E' gia buono

			switch (enPixelTypeDest)
			{
				case IMG_PIXEL_RGB:
					bNotChange=TRUE;
					break;
				
				case IMG_PIXEL_BGR:
					memcpy(psImgDest->pbImage,psImgSorg->pbImage,psImgDest->dwBitmapSize);
					IMG_RGBSwapping(psImgDest);
					psImgDest->enPixelType = enPixelTypeDest; // aggiunto perchè IMG_RGBSwapping cambia il tipo
					break;

				default: 
					bNotSupport=TRUE;
					break;
			}
			break;


		//
		// BGR
		//
		case IMG_PIXEL_BGR: // Da vedere
			switch (enPixelTypeDest)
			{
				case IMG_PIXEL_RGB:
					memcpy(psImgDest->pbImage,psImgSorg->pbImage,psImgDest->dwBitmapSize);
					IMG_RGBSwapping(psImgDest);
					psImgDest->enPixelType = enPixelTypeDest; // aggiunto perchè IMG_RGBSwapping cambia il tipo
					break;
				
				case IMG_PIXEL_BGR:
					bNotChange=TRUE;
					break;

				default: 
					bNotSupport=TRUE;
					break;
			}
			break;


		//
		// CMYK
		//
		case IMG_PIXEL_CMYK: // E' un CMYK ... siamo qua per questo

			switch (enPixelTypeDest)
			{
				case IMG_PIXEL_BGR:

					for (ky=0;ky<(INT) psImgSorg->bmiHeader.biHeight;ky++)//cinfo.image_height;ky++)
					{
						p32=(int *) lpSorg; pd=lpDest;
						for (kx=0;kx<psImgSorg->bmiHeader.biWidth;kx++) {
							
							/*Cmyk.c=*ps++; 
							Cmyk.m=*ps++; 
							Cmyk.y=*ps++; 
							Cmyk.k=*ps++; 
							CMYKtoRGB(&Cmyk,&Rgb);
							*pd++=Rgb.b; *pd++=Rgb.g; *pd++=Rgb.r;
							*/
							CMYKtoBGR(p32,(DWORD *) pd); pd+=3; p32++;
						}
						lpSorg+=psImgSorg->linesize;
						lpDest+=psImgDest->linesize;
					}
					break;

				case IMG_PIXEL_RGB:
					for (ky=0;ky<(INT) psImgSorg->bmiHeader.biHeight;ky++)//cinfo.image_height;ky++)
					{
						p32=(int *) lpSorg; pd=lpDest;
						for (kx=0;kx<psImgSorg->bmiHeader.biWidth;kx++) {
							CMYKtoRGB(p32,(DWORD *) pd); pd+=3; p32++;
						}
						lpSorg+=psImgSorg->linesize; 
						lpDest+=psImgDest->linesize;
					}
					break;

				default: 
					bNotSupport=TRUE;
					break;
			}
			break;

		//
		// RGB + ALPHA
		//
		case IMG_PIXEL_RGB_ALPHA: // E' un CMYK ... siamo qua per questo

			switch (enPixelTypeDest)
			{

				case IMG_PIXEL_RGB:
					for (ky=0;ky<(INT) psImgSorg->bmiHeader.biHeight;ky++)//cinfo.image_height;ky++)
					{
						p32=(int *) lpSorg; pd=lpDest;
						for (kx=0;kx<psImgSorg->bmiHeader.biWidth;kx++) {
							*pd=(*p32&0xFF); pd++;
							*pd=(*p32&0x00FF00)>>8; pd++;
							*pd=(*p32&0xFF0000)>>16; pd++;
							p32++;
						}
						lpSorg+=psImgSorg->linesize; 
						lpDest+=psImgDest->linesize;
					}
					break;

				case IMG_PIXEL_BGR:
					for (ky=0;ky<(INT) psImgSorg->bmiHeader.biHeight;ky++)//cinfo.image_height;ky++)
					{
						p32=(int *) lpSorg; pd=lpDest;
						for (kx=0;kx<psImgSorg->bmiHeader.biWidth;kx++) {
							*pd=(*p32&0xFF0000)>>16; pd++;
							*pd=(*p32&0x00FF00)>>8; pd++;
							*pd=(*p32&0xFF); pd++;
							p32++;
							/*
							* (INT *) pd=
								(*p32&0xFF)<<16|
								(*p32&0x00FF00) |
								(*p32&0xFF0000)>>16;
							*/
						}
						lpSorg+=psImgSorg->linesize; 
						lpDest+=psImgDest->linesize;
					}
					break;

				default: 
					bNotSupport=TRUE;
					break;
			}
			break;

		//
		// Tipo di Pixel sorgente non supportato
		//
		default:
			bNotSupport=TRUE;
 			break;
	}

	if (!bNotChange)
	{
		memoFree(hdlImageSource,"x");
		memoUnlockEx(hdlNew,"imgc2");
	}
	else
	{
		memoFree(hdlNew,"NonServe");
		memoUnlockEx(hdlImageSource,"imgc3");
		hdlNew=hdlImageSource;
	}

	if (bNotSupport) 
	{
		ehExit("IMGConvert: Non supportata la conversione da %d a %d",enPixelTypeSource,enPixelTypeDest);
				
	}
	return hdlNew;
}


// 
// BMPSaveFile
// SCRIVE UN FILE IN FORMATO BMP
//
// imageFileName Nome del file da scrivere
// HdlImage	     Hdl che contiene l'immagine in formato IMGHEADER
// 0= errore >0 tutto ok
//
BOOL BMPSaveFile(TCHAR *tszFileName, INT HdlImage)
{
	IMGHEADER * psImgHead;
	BITMAPFILEHEADER sBFDest;
	WORD wBm=19778;
	HANDLE hFile;
	RGBQUAD	sRgbQuad;
	INT iPaletteSize,i;
	DWORD dwRealWrite;
	INT iLine32,iDiff;
	CHAR szEmpty[]={0,0,0,0};
	BITMAPINFOHEADER sBih;
	BYTE *pbLine;

	psImgHead=memoLock(HdlImage);
	if (psImgHead->enPixelType==IMG_PIXEL_RGB_ALPHA) {
	
		INT hImageDup;
		BOOL bRet;

		memoUnlock(HdlImage);

		hImageDup=memoClone(HdlImage);
		hImageDup=IMGConvert(hImageDup,IMG_PIXEL_BGR); // <-- Converto in RGB
//		IMGDisp(0,100,hImageDup);
		bRet=BMPSaveFile(tszFileName,hImageDup);
		memoFree(hImageDup,"xx");
		return bRet;
	}

	hFile= CreateFile((LPCTSTR) tszFileName,
						GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						NULL,
						CREATE_ALWAYS,
						FILE_FLAG_SEQUENTIAL_SCAN,
						(HANDLE)NULL);
	if (hFile==(HANDLE) INVALID_HANDLE_VALUE) ehExit("Creazione file [%s] error",tszFileName);

	//
	// Sistemo Header
	//

	memcpy(&sBih,&psImgHead->bmiHeader,sizeof(sBih));
	sBih.biSizeImage=psImgHead->dwBitmapSize;
	sBih.biClrImportant=psImgHead->dwColors;
	sBih.biClrUsed=psImgHead->dwColors;
	iPaletteSize=psImgHead->dwColors*sizeof(RGBQUAD);

	//
	// Scrivo FileHeader
	//
	_(sBFDest);
	sBFDest.bfType=wBm;
	sBFDest.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+iPaletteSize;
	sBFDest.bfSize= sBFDest.bfOffBits + psImgHead->dwBitmapSize;
	if (!WriteFile(hFile, &sBFDest,sizeof(sBFDest),&dwRealWrite,NULL)) ehError();

	//
	// Header del bitmap 
	//
	if (!WriteFile(hFile, &sBih,sizeof(BITMAPINFOHEADER),&dwRealWrite,NULL)) ehError();

	//
	// Palette
	//
	for (i=0;i<(INT) psImgHead->dwColors;i++)
	{
		_(sRgbQuad);
		sRgbQuad.rgbBlue=psImgHead->psRgbPalette[i].b;
		sRgbQuad.rgbGreen=psImgHead->psRgbPalette[i].g;
		sRgbQuad.rgbRed=psImgHead->psRgbPalette[i].r;
		if (!WriteFile(hFile, &sRgbQuad,sizeof(RGBQUAD),&dwRealWrite,NULL)) ehError();
	}
	
	//
	// Scrivo bitmap
	//
	iLine32=((psImgHead->linesize+3)>>2)<<2; 
	iDiff=iLine32-psImgHead->linesize;
	pbLine=psImgHead->pbImage+((psImgHead->bmiHeader.biHeight-1)*psImgHead->linesize);
	if (psImgHead->dwColors) {
		
		for (i=0;i<psImgHead->bmiHeader.biHeight;i++)
		{
			WriteFile(hFile, pbLine, psImgHead->linesize, &dwRealWrite, NULL);
			if (iDiff) WriteFile(hFile, szEmpty, iDiff, &dwRealWrite, NULL);
			pbLine-=psImgHead->linesize;
		}
	}
	else
	{

		for (i=0;i<psImgHead->bmiHeader.biHeight;i++)
		{
//			BYTE *pb;
//			pb=psImgHead->pbImage+(i*psImgHead->linesize);
			WriteFile(hFile, pbLine, psImgHead->linesize, &dwRealWrite, NULL);
			if (iDiff) WriteFile(hFile, szEmpty, iDiff, &dwRealWrite, NULL);
			pbLine-=psImgHead->linesize;

		}
	
	}

	CloseHandle(hFile);
	memoUnlock(HdlImage);

	return TRUE;

}


// ---------------------------------------------------
// BMPReadHeader
// LEGGE L' HEADER DI UN FILE BMP
//
// imageFileName Nome del file da leggere
// *psImgHead	 Puntatore alla struttura di ritorno contenente le info [header] dell'immagine
// Ritorna: 0 Errore
//          >0 Handle dell' immagine aperta
//
INT BMPReadHeader(TCHAR *imageFileName, IMGHEADER * psImgHead)  
{
	INT hdlImage;
	hdlImage=_BMPReadService(imageFileName, psImgHead ,TRUE) ;
	return hdlImage;
}


// ---------------------------------------------------
// BMPReadFile
// LEGGE UN FILE IN FORMATO BMP
//
// imageFileName Nome del file da leggere
// *piImageHandle	 Puntatore all'handle che conterrà l'immagine
// Ritorna: 0 Errore
//          1 Tutto OK
//
BOOL BMPReadFile(TCHAR *imageFileName, INT *piImageHandle) {

	IMGHEADER * psImgHead=NULL;
	*piImageHandle=_BMPReadService(imageFileName, psImgHead ,FALSE) ;
	if (*piImageHandle<1)
		return FALSE;
	else
		return TRUE;
}

#ifndef EH_CONSOLE
//
// BMPLoadBitmap()
//
HBITMAP BMPLoadBitmap(TCHAR *imageFileName) {

	EH_IMG himImage;
	HBITMAP hBitmap;
	if (!BMPReadFile(imageFileName, &himImage)) return NULL;
	hBitmap=ImgToBitmap(himImage,NULL,FALSE);
	IMGDestroy(himImage);
	return hBitmap;
}
#endif

//
// _BMPReadService()
// Funzione di servizio per leggere l'header o l'immagine BMP
//
static INT _BMPReadService(TCHAR *pszFileName, IMGHEADER *psImgHead ,BOOL bOnlyHeader)  
{

	BYTE *pFile=NULL;
	SIZE_T ptSize,iSize;
	INT	i;
	LPBITMAPINFO pBih;
	INT hdlImage;
	EN_PIXEL_TYPE enImg;
	BITMAPFILEHEADER *psInfoHeader;
	IMGHEADER ImgHeadDest;
	FILE *fpImage;
	BYTE *pDest;

	if (!psImgHead) 
		psImgHead=&ImgHeadDest;

	if (!fileCheck(pszFileName)) ehExit("il file %s non esiste",pszFileName);
	if (bOnlyHeader)
	{
		fpImage = fopen(pszFileName,"rb");
		if (fpImage == NULL) ehError();
		iSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFO);
		pFile=ehAlloc(iSize);
		fread(pFile,iSize,1,fpImage);
		fclose(fpImage);

	}
	else pFile=fileMemoRead(pszFileName,&ptSize);
	
	psInfoHeader=(BITMAPFILEHEADER *) pFile;
	pBih=(LPBITMAPINFO) (pFile+sizeof(BITMAPFILEHEADER));

	switch (pBih->bmiHeader.biBitCount)
	{
		case 1:
			enImg=IMG_PIXEL_COLOR1;
			break;

		case 4:
			enImg=IMG_PIXEL_COLOR4;
			break;

		case 8:
			enImg=IMG_PIXEL_COLOR8;
			break;

		case 24:
			enImg=IMG_PIXEL_BGR;
			break;

		case 32:
			enImg=IMG_PIXEL_RGB_ALPHA;
			break;

		default:
			ehError();
	}


	hdlImage=IMGCreate( enImg,
						fileName(pszFileName),
					    pBih->bmiHeader.biWidth,
						pBih->bmiHeader.biHeight,
						psImgHead,
						bOnlyHeader);

	psImgHead=memoLockEx(hdlImage,fileName(pszFileName));
	psImgHead->enFilePixelType=enImg;
	memcpy(&psImgHead->bmiHeader,&pBih->bmiHeader,sizeof(BITMAPINFOHEADER));

	//if (hdlImage<1) ehError();
	if (!bOnlyHeader) {

		//BYTE *pSource;
		BYTE *pSource;

		for (i=0;i<(INT) psImgHead->dwColors;i++)
		{
			_(psImgHead->psRgbPalette[i]);
			psImgHead->psRgbPalette[i].b=pBih->bmiColors[i].rgbBlue;
			psImgHead->psRgbPalette[i].g=pBih->bmiColors[i].rgbGreen;
			psImgHead->psRgbPalette[i].r=pBih->bmiColors[i].rgbRed;
		}

		// devo modificare questa parte per caricare l' immagine al contrario
		//memcpy(psImgHead->pbImage,pFile+psInfoHeader->bfOffBits,psImgHead->dwBitmapSize);

		//pSource=pFile+psInfoHeader->bfOffBits+psImgHead->dwBitmapSize;

		pSource=pFile+psInfoHeader->bfOffBits;//
		pDest=psImgHead->pbImage+((psImgHead->bmiHeader.biHeight-1)*psImgHead->linesize); 
		//pSource=pFile+psInfoHeader->bfOffBits+((psImgHead->bmiHeader.biHeight-i-1)*psImgHead->linesize);
		for (i=0;i<(INT) psImgHead->bmiHeader.biHeight;i++)
		{
			memcpy(pDest,pSource,psImgHead->linesize);
			pDest-=psImgHead->linesize; 
			pSource+=psImgHead->linesize;
		}

		
		/*
		// se il numero  di righe è uguale 
		if (psImgHead->linesize==(pBih->bmiHeader.biSizeImage/pBih->bmiHeader.biHeight))
			memcpy(psImgHead->pbImage,pFile+psInfoHeader->bfOffBits,psImgHead->dwBitmapSize);
		else
			ehError();
			*/
	}
	memoUnlock(hdlImage);

	ehFree(pFile);


	return hdlImage;

}


#ifdef EH_MOBILE
HBITMAP JPGReadLikeBitmap(TCHAR *tsFile)
{
	INT hdl;
	HBITMAP hBitmap;
	if (!JPGReadFile(tsFile,&hdl,TRUE,NULL,NULL,FALSE)) return NULL;
	hBitmap=ImgToBitmap(hdl);
	memoFree(hdl,"jpg");
	return hBitmap;
}
HBITMAP GIFReadLikeBitmap(TCHAR *tsFile)
{
	INT hdl=0;
	GIFINFO GIFInfo;
	HBITMAP hBitmap=NULL;
	if (!GIFOpenFile(tsFile,&GIFInfo)) {win_infoarg("KO"); return NULL;}
	if (!GIFtoIMG(&GIFInfo,		// Struttura GIF
				  0,0,			// Fino al Frame
			 	  sys.ColorBackGround,
				  &hdl))
	{
		hBitmap=ImgToBitmap(hdl);
		memoFree(hdl,"gif");
	}
	GIFCloseFile(&GIFInfo);
//	win_infoarg("%d",hBitmap);
	return hBitmap;
}
#endif


// --------------------------------------------------------
// ImgCalcSize
// Calcola le nuove dimensioni di un immagine
// Creando un nuovo rettangolo di destinazione, 
// partendo dal rettangolo di un sorgente dell'immagine
//
// --------------------------------------------------------
void IMGCalcSize(IMGHEADER *ImgHead,      // Dimensioni del sorgente
				 SIZE sDim,		   // Area disponibile
				 INT iPhotoAdatta,  // Tipo di adattamento
				 INT iAlignH,	   // Allineamento orizzontale
				 INT iAlignV,	   // Allineamento verticale
				 SIZE *sDest,		   // Dimensioni della destinazione
				 RECT *rDest,
				 RECT *psrSource) 	   // Posizionamento in destinazione
{
	SIZE sSource;
	sSource.cx=ImgHead->bmiHeader.biWidth;
	sSource.cy=ImgHead->bmiHeader.biHeight;
	RectCalcSize(sSource,sDim,iPhotoAdatta,iAlignH,iAlignV,sDest,rDest,psrSource);
}

void RectCalcSize(SIZE sSource,		// Dimensioni del sorgente
				  SIZE sAreaDest,		// Dimensione Area destinazione desiderata
				  EN_IMGPT iPhotoAdatta,// Tipo di adattamento
				  INT iAlignH,	    // Allineamento richiesto orizzontale
				  INT iAlignV,	    // Allineamento richiesto verticale
				  SIZE *lpsDest,		// Dimensioni della destinazione
				  RECT *lprDest,		// Posizionamento in destinazione
				  RECT *lprSource) 	    // Rettangolo da prendere nel sorgente
{
	SIZE sCut;
	if (sSource.cy<1) sSource.cy=1;
	if (sSource.cx<1) sSource.cx=1;

	// Default setting
	memset(lpsDest,0,sizeof(SIZE));	memset(lprDest,0,sizeof(RECT));
	if (lprSource) {memset(lprSource,0,sizeof(RECT)); lprSource->right=sSource.cx-1; lprSource->bottom=sSource.cy-1;}

	//
	// Trova dimensioni della destinazione
	//
	switch (iPhotoAdatta)
	{
		case IMGPT_PROPORZIONALE: // Calcola il lato mancante 
			if ((sAreaDest.cy>0)&&(sAreaDest.cx==0)) sAreaDest.cx=sAreaDest.cy*sSource.cx/sSource.cy;
			if ((sAreaDest.cx>0)&&(sAreaDest.cy==0)) sAreaDest.cy=sAreaDest.cx*sSource.cy/sSource.cx;
			lpsDest->cx=sAreaDest.cx;
			lpsDest->cy=sAreaDest.cy;
			break;

		case IMGPT_AL_FORMATO:
			lpsDest->cx=sAreaDest.cy*sSource.cx/sSource.cy;
			if (lpsDest->cx>sAreaDest.cx) 
				{lpsDest->cy=sAreaDest.cx*sSource.cy/sSource.cx; lpsDest->cx=sAreaDest.cx;}
				else 
				{lpsDest->cy=sAreaDest.cy;}
			break;

		case IMGPT_AL_CORTO: 
			if (sSource.cx<sSource.cy) 
				{lpsDest->cy=sAreaDest.cx*sSource.cy/sSource.cx; lpsDest->cx=sAreaDest.cx;}
				else
				{lpsDest->cx=sAreaDest.cy*sSource.cx/sSource.cy; lpsDest->cy=sAreaDest.cy;}
			break;

		case IMGPT_AL_LUNGO: 
			if (sSource.cx>sSource.cy) 
				{lpsDest->cy=sAreaDest.cx*sSource.cy/sSource.cx; lpsDest->cx=sAreaDest.cx;}
				else
				{lpsDest->cx=sAreaDest.cy*sSource.cx/sSource.cy; lpsDest->cy=sAreaDest.cy;}
			break;
		
		case IMGPT_NO:
			memcpy(lpsDest,&sSource,sizeof(SIZE));
			break;

		case IMGPT_PROP_TAGLIA: // Calcola il lato mancante 
			lpsDest->cx=sAreaDest.cx; lpsDest->cy=sAreaDest.cy;
			if (!lpsDest->cx||!lpsDest->cy) ehError();
			break;
	}

	//
	// Trova il posizionamento
	//
	switch (iPhotoAdatta)
	{
	//	case IMGPT_PROPORZIONALE: break;
		case IMGPT_PROP_TAGLIA:

			//
			// Cerco dimensione e posizione nel sorgente
			//
			sCut.cx=sSource.cx; sCut.cy=sAreaDest.cy*sSource.cx/sAreaDest.cx; 
			if (sCut.cy>sSource.cy) {sCut.cx=sAreaDest.cx*sSource.cy/sAreaDest.cy; sCut.cy=sSource.cy;}

			lprDest->right=sAreaDest.cx-1; lprDest->bottom=sAreaDest.cy-1;
			if (!lprSource) ehError();
			// 
			// Allineamento Orizzontale
			//
			switch (iAlignH)
			{
				default:
				case 0: // Centra (Default)
					lprSource->left=(sSource.cx-sCut.cx)>>1;
					break;

				case 1: // Left
					lprSource->left=0;
					break;

				case 2: // Right
					lprSource->left=sSource.cx-sCut.cx+1;
					break;
			}
			lprSource->right=lprSource->left+sCut.cx-1;

			// -------------------------------------------
			// Allinamento Verticale
			//
			switch (iAlignV)
			{
				default:
				case 0: // Centra (Default)
					lprSource->top=(sSource.cy-sCut.cy)>>1;
					break;

				case 1: // Top
					lprSource->top=0;
					break;

				case 2: // Bottom
					lprSource->top=sSource.cy-sCut.cy+1;
					break;
			}
			lprSource->bottom=lprSource->top+sCut.cy-1;
			break;

			//
			// Allineamento di default
			//
		default: 

			// Allinamento Orizzontale
			switch (iAlignH)
			{
				default:
				case 0: // Centra (Default)
					lprDest->left=(sAreaDest.cx-lpsDest->cx)/2;
					lprDest->right=lprDest->left+lpsDest->cx-1;
					break;

				case 1: // Left
					lprDest->left=0;
					lprDest->right=lpsDest->cx-1;
					break;

				case 2: // Right
					lprDest->left=sAreaDest.cx-lpsDest->cx+1;
					lprDest->right=sAreaDest.cx-1;
					break;
			}

			switch (iAlignV)
			// Allinamento Verticale
			{
				default:
				case 0: // Centra (Default)
					lprDest->top=(sAreaDest.cy-lpsDest->cy)/2;
					lprDest->bottom=lprDest->top+lpsDest->cy-1;
					break;

				case 1: // Top
					lprDest->top=0;
					lprDest->bottom=lpsDest->cy-1;
					break;

				case 2: // Bottom
					lprDest->top=sAreaDest.cy-lpsDest->cy+1;
					lprDest->bottom=sAreaDest.cy-1;
					break;
			}
			break;

	}
}
