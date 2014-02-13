//   /-------------------------------------------\
//   | IMGTool  Image Tool                       |
//   |          Elaborazione di immagini         |
//   |                                           |
//   |             by Ferrà Art & Technology 2000 |
//   \-------------------------------------------/

#include "/easyhand/inc/easyhand.h"
#include <setjmp.h>
#include <math.h>
#include "/easyhand/ehtool/imgutil.h"
#include "/easyhand/ehtool/imgtool.h"

//
// Rotazione di immagine di 90 gradi in senso orario
// Rotazione di immagine di 90 gradi in senso antiorario
// Rotazione di immagine di 180 gradi in senso antiorario
//

// ----------------------------------------------
// IMGMirrorX
// Rifletti orizzontale
//
//
//
void IMGMirrorX(INT HdlImage)
{
	IMGHEADER *psImgSorg;
	BITMAPINFOHEADER *psBHSorg;
	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	INT x,y;
	INT HdlMemo;
	BYTE *lpMemo;

	lpi=memoLock(HdlImage);
	psImgSorg=(IMGHEADER *) lpi; lpSorg=psImgSorg->pbImage;
	
	psBHSorg=(BITMAPINFOHEADER *) &psImgSorg->bmiHeader;
	sImage.cy=psBHSorg->biHeight;
    sImage.cx=psBHSorg->biWidth;

/*
	if (psImgSorg->bmiHeader.biBitCount!=24) 
	{
		ehExit("MirrorX solo a 24 bit");
	}
*/

	// -----------------------------------------------
	// Chiedo memoria per il processo
	//
	HdlMemo=memoAlloc(RAM_AUTO,psImgSorg->linesize,"Buff");
	lpMemo=memoLock(HdlMemo);
	
	// -----------------------------------------------
	// Effettuo il mirroring
	//
	
	lpj=lpSorg;
	for (y=0;y<sImage.cy;y++)
	{
		memset(lpMemo,0,psImgSorg->linesize);
		for (x=0;x<sImage.cx;x++)
		{	
			ehMemCpy(lpMemo+(x*psImgSorg->iChannels),lpj+((sImage.cx-x-1)*psImgSorg->iChannels),psImgSorg->iChannels);
		}
		ehMemCpy(lpj,lpMemo,psImgSorg->linesize);
	  lpj+=psImgSorg->linesize;
	}

	memoFree(HdlMemo,"Buf");
	memoUnlock(HdlImage);
}

// ----------------------------------------------
// IMGMirrorY
// Rifletti verticale
//
//
//
void IMGMirrorY(INT HdlImage)
{
	IMGHEADER *psImgSorg;
	BITMAPINFOHEADER *psBHSorg;
	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	INT x,y;
	INT HdlMemo;
	BYTE *lpMemo;

	lpi=memoLock(HdlImage);
	psImgSorg=(IMGHEADER *) lpi; lpSorg=psImgSorg->pbImage;
	
	psBHSorg=(BITMAPINFOHEADER *) &psImgSorg->bmiHeader;
	sImage.cy=psBHSorg->biHeight;
    sImage.cx=psBHSorg->biWidth;

//	if (psImgSorg->bmiHeader.biBitCount!=24) {ehExit("MirrorY solo a 24 bit");}

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
		lpj=lpSorg+x*psImgSorg->iChannels;
		for (y=0;y<sImage.cy;y++)
		{	
			ehMemCpy(lpMemo+(y*psImgSorg->iChannels),lpj,psImgSorg->iChannels); lpj+=psImgSorg->linesize;
		}

		lpj-=psImgSorg->linesize;
		for (y=0;y<sImage.cy;y++)
		{	
			ehMemCpy(lpj,lpMemo+(y*psImgSorg->iChannels),psImgSorg->iChannels); lpj-=psImgSorg->linesize;
		}
	}
	memoFree(HdlMemo,"Buf");
	memoUnlockEx(HdlImage,"IMMY");
}

// ----------------------------------------------
// IMGRotation90H
// Rotazione di 90 Grandi
// restituisce un nuovo Handle con l'immagine ruotata
// Nota: Fà un nuovo handle perche 99su100 l'immagine cambia di dimensioni per l'effetto della quantizzazione a 32 bit
//

INT IMGRotation(EN_ROT enRotation,INT hSource)
{
	IMGHEADER *psImgSorg;
	IMGHEADER *psImgDest;
	BITMAPINFOHEADER *psBHSorg;
	BITMAPINFOHEADER *psBHDest;
//	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	INT x,y;
	INT hImage;
	BYTE *lpDest;
//	INT iNewLineSize;
//	INT iNewSize;
	BYTE *ps;

	if (!enRotation) {
		return memoClone(hSource);
	
	}
	psImgSorg=(IMGHEADER *) memoLock(hSource); lpSorg=psImgSorg->pbImage;
	
	psBHSorg=(BITMAPINFOHEADER *) &psImgSorg->bmiHeader;
	sImage.cy=psBHSorg->biHeight;
    sImage.cx=psBHSorg->biWidth;

//	if (psImgSorg->bmiHeader.biBitCount!=24) {ehExit("MirrorY solo a 24 bit");}
	// Ricalcolo l'occupazione di spazio

	// Quantizzazione a 32 bit
	if (enRotation!=ROT_180) {
		hImage=IMGCreate(psImgSorg->enPixelType,"imgRotate",sImage.cy,sImage.cx,NULL,FALSE);
	}
	else {
		hImage=IMGCreate(psImgSorg->enPixelType,"imgRotate",sImage.cx,sImage.cy,NULL,FALSE);
	}

	// Alloco la memoria
	if (hImage<0) ehExit("IMGRemaker: non memory");
	psImgDest=memoLock(hImage);
	lpDest=psImgDest->pbImage;
	psBHDest=(BITMAPINFOHEADER *) &psImgDest->bmiHeader;

	// Copio la palette se presente
	if (psImgSorg->dwColors) ehMemCpy(psImgDest->psRgbPalette,psImgSorg->psRgbPalette,sizeof(S_RGB)*psImgSorg->dwColors);
	

//	memset(lpDest,0,iNewSize);
//	ehMemCpy(psImgDest,psImgSorg,psImgSorg->Offset);
/*
	// Aggiorno i dati delle dimensioni
	if (enRotation!=ROT_180)
	{
		psImgDest->linesize=iNewLineSize;
		psBHDest->biHeight=sImage.cx;
		psBHDest->biWidth=sImage.cy;
	}
*/


	switch (enRotation)
	{
		case ROT_NONE:
			ehMemCpy(psImgDest->pbImage,lpSorg,psImgSorg->dwBitmapSize);
			break;
	

		//
		// Rotazione 90 gradi orari ============================================================
		//
		case ROT_270:

			switch (psImgDest->enPixelType)
			{
				//
				// --> 24bit color deep
				//
				case IMG_PIXEL_BGR:
				case IMG_PIXEL_RGB:

					lpj=lpSorg;
					for (y=0;y<sImage.cy;y++)
					{
						for (x=0;x<sImage.cx;x++)
						{	
							ehMemCpy(lpDest+((sImage.cy-y-1)*psImgSorg->iChannels)+(x*psImgDest->linesize),lpj+(x*psImgSorg->iChannels),psImgSorg->iChannels);
						}
					lpj+=psImgSorg->linesize;
					}

					break;

				//
				// --> 1bit color deep
				//
				case IMG_PIXEL_COLOR1:
					
					memset(lpDest,0,psImgDest->dwBitmapSize); // Pulisco
					// Legge il bit dal sorgente
					for (y=0;y<(INT) psImgSorg->bmiHeader.biHeight;y++) {
						ps=lpSorg; 
						for (x=0;x<psImgSorg->bmiHeader.biWidth;x++) {
							INT iBitColor=(ps[x>>3]&(0x80>>(x&7)))?1:0; // Valore del bit sorgente
							
							// Dove dobbiamo scrivere
							INT xDest=psImgSorg->bmiHeader.biHeight-y-1, yDest=x;

							// Rintracciato il byte
							INT idxByteDest=(xDest>>3)+((psImgDest->linesize)*yDest);
							INT iPosBit=(0x80>>(xDest&7)); // 1000 0000 > nPixel & 7 (da 0 a 7)

							// Rintracciare il bit ?
							lpDest[idxByteDest]=(lpDest[idxByteDest]&~iPosBit)|(iBitColor?iPosBit:0);
						}
						lpSorg+=psImgSorg->linesize;
					}
					break;

				default:
					ehError();
					break;
			}
			break;

		//
		// Rotazione 90 gradi antiorari ============================================================
		//
		case ROT_90:

			switch (psImgDest->enPixelType)
			{
				//
				// --> 24bit color deep
				//
				case IMG_PIXEL_BGR:
				case IMG_PIXEL_RGB:

					lpj=lpSorg;
					for (y=0;y<sImage.cy;y++) {
						for (x=0;x<sImage.cx;x++)
						{	
							ehMemCpy(lpDest+(y*psImgSorg->iChannels)+((sImage.cx-x-1)*psImgDest->linesize),lpj+(x*psImgSorg->iChannels),psImgSorg->iChannels);
						}
					lpj+=psImgSorg->linesize;
					}
					break;

				//
				// --> 1bit color deep
				//
				case IMG_PIXEL_COLOR1:
					
					// Legge il bit dal sorgente
					for (y=0;y<(INT) psImgSorg->bmiHeader.biHeight;y++) {
						ps=lpSorg; 
						for (x=0;x<psImgSorg->bmiHeader.biWidth;x++) {
							INT iBitColor=(ps[x>>3]&(0x80>>(x&7)))?1:0; // Valore del bit sorgente
							
							// Dove dobbiamo scrivere
							INT xDest=y, yDest=psImgSorg->bmiHeader.biWidth-x-1;

							// Rintracciato il byte
							INT idxByteDest=(xDest>>3)+((psImgDest->linesize)*yDest);
							INT iPosBit=(0x80>>(xDest&7)); // 1000 0000 > nPixel & 7 (da 0 a 7)

							// Setto il bit
							lpDest[idxByteDest]=(lpDest[idxByteDest]&~iPosBit)|(iBitColor?iPosBit:0);
						}
						lpSorg+=psImgSorg->linesize;
					}
					break;

				default:
					ehError();
					break;
			}
			break;

		//
		// Rotazione 180 gradi antiorari ============================================================
		//
		case ROT_180:

			switch (psImgDest->enPixelType) {

				//
				// --> 24bit color deep
				//
				case IMG_PIXEL_BGR:
				case IMG_PIXEL_RGB:

					lpj=lpSorg;
					for (y=0;y<sImage.cy;y++)
					{
						for (x=0;x<sImage.cx;x++)
						{	
							ehMemCpy(lpDest+((sImage.cy-y-1)*psImgDest->linesize)+((sImage.cx-x-1)*psImgSorg->iChannels),lpj+(x*psImgSorg->iChannels),psImgSorg->iChannels);
						}
					lpj+=psImgSorg->linesize;
					}
					break;

				//
				// --> 1bit color deep
				//
				case IMG_PIXEL_COLOR1:
						
					// Legge il bit dal sorgente
					for (y=0;y<(INT) psImgSorg->bmiHeader.biHeight;y++) {
						ps=lpSorg; 
						for (x=0;x<psImgSorg->bmiHeader.biWidth;x++) {
							INT iBitColor=(ps[x>>3]&(0x80>>(x&7)))?1:0; // Valore del bit sorgente
							
							// Dove dobbiamo scrivere
							INT xDest=psImgSorg->bmiHeader.biWidth-x-1, yDest=psImgSorg->bmiHeader.biHeight-y-1;

							// Rintracciato il byte
							INT idxByteDest=(xDest>>3)+((psImgDest->linesize)*yDest);
							INT iPosBit=(0x80>>(xDest&7)); // 1000 0000 > nPixel & 7 (da 0 a 7)

							// Setto il bit
							lpDest[idxByteDest]=(lpDest[idxByteDest]&~iPosBit)|(iBitColor?iPosBit:0);
						}
						lpSorg+=psImgSorg->linesize;
					}
					break;

				default:
					ehError();
					break;

			}
			break;
	
	}

	memoUnlock(hImage);
	memoUnlock(hSource);
	return hImage;
}
/*
INT IMGRotation90H(INT HdlImage)
{
	return IMGRotation(ROT_90H,HdlImage);
}

INT IMGRotation90A(INT HdlImage)
{
	return IMGRotation(ROT_90A,HdlImage);
}

INT IMGRotation180(INT HdlImage)
{
	return IMGRotation(ROT_180,HdlImage);
}
*/
//
// IMGBrightness()
//
INT IMGBrightness(INT iPerc,INT HdlImage)
{
	IMGHEADER *psImgSorg;
	IMGHEADER *psImgDest;
	BITMAPINFOHEADER *psBHSorg;
	BITMAPINFOHEADER *psBHDest;
	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	INT x,y;
	INT HdlNew;
	BYTE *lpDest;
	INT iNewLineSize;
	INT iNewSize;
	BYTE *lpbSource;
	BYTE *lpbDest;
	INT Col;

	lpi=memoLock(HdlImage);
	psImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+psImgSorg->Offset;
	
	psBHSorg=(BITMAPINFOHEADER *) &psImgSorg->bmiHeader;
	sImage.cy=psBHSorg->biHeight;
    sImage.cx=psBHSorg->biWidth;

	//if (psImgSorg->bmiHeader.biBitCount!=24) {ehExit("IMGBrightness 24 bit");}

	// Ricalcolo l'occupazione di spazio
	iNewLineSize=psImgSorg->linesize;
	iNewSize=(sImage.cy*iNewLineSize)+psImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memoAlloc(RAM_AUTO,iNewSize,"NewImage(B)");
	if (HdlNew<0) ehExit("IMGRemaker: non memory");
	lpDest=memoLock(HdlNew);
	memset(lpDest,0,iNewSize);

	// Copio l'header
	ehMemCpy(lpDest,lpi,psImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	psImgDest=(IMGHEADER *) lpDest;
	psBHDest=(BITMAPINFOHEADER *) &psImgDest->bmiHeader;
	lpDest+=psImgDest->Offset;

	// -----------------------------------------------
	// Effettuo la variazione di luminosità
	//

	lpj=lpSorg; lpi=lpDest;
	switch (psImgSorg->enPixelType)
	{
		case IMG_PIXEL_RGB_ALPHA: // Con Alpha channel
			for (y=0;y<sImage.cy;y++)
			{
				lpbSource=lpj; lpbDest=lpi;
				for (x=0;x<psImgSorg->linesize;x++)
				{	
					if ((x%4)==3) {*lpbDest++=*lpbSource++; continue;} // Solo questo di modifica
					Col=*lpbSource++;
					if (iPerc<0) {Col=(100+iPerc)*Col; if (Col) Col/=100;}
					if (iPerc>0) {Col=(100-iPerc)*Col+255*iPerc; if (Col) Col/=100;}
					*lpbDest++=(BYTE) Col;
					//lpbDest++; lpbSource++;
				}
				lpj+=psImgSorg->linesize; lpi+=psImgSorg->linesize;
			}
			break;

		case IMG_PIXEL_RGB: 
		case IMG_PIXEL_BGR: 

			for (y=0;y<sImage.cy;y++)
			{
				lpbSource=lpj; lpbDest=lpi;
				for (x=0;x<psImgSorg->linesize;x++)
				{	
					Col=*lpbSource++;
					if (iPerc<0) {Col=(100+iPerc)*Col; if (Col) Col/=100;}
					if (iPerc>0) {Col=(100-iPerc)*Col+255*iPerc; if (Col) Col/=100;}
					*lpbDest++=(BYTE) Col;
					//lpbDest++; lpbSource++;
				}
				lpj+=psImgSorg->linesize; lpi+=psImgSorg->linesize;
			}
			break;

		default: ehError();
	}

	memoUnlock(HdlNew);
	memoUnlock(HdlImage);
	return HdlNew;
}

INT IMGContrast(INT iPerc,INT HdlImage)
{
	IMGHEADER *psImgSorg;
	IMGHEADER *psImgDest;
	BITMAPINFOHEADER *psBHSorg;
	BITMAPINFOHEADER *psBHDest;
	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	INT x,y;
	INT HdlNew;
	BYTE *lpDest;
	INT iNewLineSize;
	INT iNewSize;

	lpi=memoLock(HdlImage);
	psImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+psImgSorg->Offset;
	
	psBHSorg=(BITMAPINFOHEADER *) &psImgSorg->bmiHeader;
	sImage.cy=psBHSorg->biHeight;
    sImage.cx=psBHSorg->biWidth;

//	if (psImgSorg->bmiHeader.biBitCount!=24) {ehExit("IMGContrast 24 bit");}

	// Ricalcolo l'occupazione di spazio
	iNewLineSize=psImgSorg->linesize;
	iNewSize=(sImage.cy*iNewLineSize)+psImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memoAlloc(RAM_AUTO,iNewSize,"NewImage(C)");
	if (HdlNew<0) ehExit("IMGRemaker: non memory");
	lpDest=memoLock(HdlNew);
	memset(lpDest,0,iNewSize);

	// Copio l'header
	ehMemCpy(lpDest,lpi,psImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	psImgDest=(IMGHEADER *) lpDest;
	psBHDest=(BITMAPINFOHEADER *) &psImgDest->bmiHeader;
	lpDest+=psImgDest->Offset;

	// -----------------------------------------------
	// Effettuo la variazione di Contrasto
	//

	lpj=lpSorg;
	lpi=lpDest;
	if (iPerc<0) iPerc/=2;
	if (iPerc>0) iPerc*=2;
	for (y=0;y<sImage.cy;y++)
	{
		BYTE *lpbSource=lpj;
		BYTE *lpbDest=lpi;
		INT Col;
		for (x=0;x<(sImage.cx*psImgSorg->iChannels);x++)
		{	
			INT m;
			Col=*lpbSource;
			m=(100+iPerc);
			if (iPerc!=0) {Col=(m)*Col+128*(100-m); if (Col) Col/=100;}
			if (Col<0) Col=0;
			if (Col>255) Col=255;

			*lpbDest=(BYTE) Col;
			lpbDest++; lpbSource++;
		}
		lpj+=psImgSorg->linesize;
		lpi+=psImgSorg->linesize;
	}

	memoUnlock(HdlNew);
	memoUnlock(HdlImage);
	return HdlNew;
}


INT IMGBrightnessContrast(INT iPercBright,INT iPercContrast,INT HdlImage)
{
	IMGHEADER *psImgSorg;
	IMGHEADER *psImgDest;
	BITMAPINFOHEADER *psBHSorg;
	BITMAPINFOHEADER *psBHDest;
	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	INT x,y;
	INT HdlNew;
	BYTE *lpDest;
	INT iNewLineSize;
	INT iNewSize;

	lpi=memoLock(HdlImage);
	psImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+psImgSorg->Offset;
	
	psBHSorg=(BITMAPINFOHEADER *) &psImgSorg->bmiHeader;
	sImage.cy=psBHSorg->biHeight;
    sImage.cx=psBHSorg->biWidth;

	if (psImgSorg->bmiHeader.biBitCount!=24) {ehExit("IMGContrast 24 bit");}

	// Ricalcolo l'occupazione di spazio
	iNewLineSize=psImgSorg->linesize;
	iNewSize=(sImage.cy*iNewLineSize)+psImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memoAlloc(RAM_AUTO,iNewSize,"NewImage(BC)");
	if (HdlNew<0) ehExit("IMGRemaker: non memory");
	lpDest=memoLock(HdlNew);
	memset(lpDest,0,iNewSize);

	// Copio l'header
	ehMemCpy(lpDest,lpi,psImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	psImgDest=(IMGHEADER *) lpDest;
	psBHDest=(BITMAPINFOHEADER *) &psImgDest->bmiHeader;
	lpDest+=psImgDest->Offset;

	// -----------------------------------------------
	// Effettuo la variazione di luminosità
	//

	lpj=lpSorg;
	lpi=lpDest;
	if (iPercContrast<0) iPercContrast/=2;
	if (iPercContrast>0) iPercContrast*=2;
	for (y=0;y<sImage.cy;y++)
	{
		BYTE *lpbSource=lpj;
		BYTE *lpbDest=lpi;
		INT Col;
		for (x=0;x<(sImage.cx*3);x++)
		{	
			INT m;
			Col=*lpbSource;
			
			// Luminosità
			if (iPercBright<0) {Col=(100+iPercBright)*Col; if (Col) Col/=100;}
			if (iPercBright>0) {Col=(100-iPercBright)*Col+255*iPercBright; if (Col) Col/=100;}
			
			// Contrasto
			m=(100+iPercContrast);
			if (iPercContrast!=0) {Col=(m)*Col+128*(100-m); if (Col) Col/=100;}
			
			if (Col<0) Col=0;
			if (Col>255) Col=255;

			*lpbDest=(BYTE) Col;
			lpbDest++; lpbSource++;
		}
		lpj+=psImgSorg->linesize;
		lpi+=psImgSorg->linesize;
	}

	memoUnlock(HdlNew);
	memoUnlock(HdlImage);
	return HdlNew;
}



// -------------------------------------------------------
// Trasforma il raster in memoria in DIB Bitmap          |
// -------------------------------------------------------
static HBITMAP IMGToBitMap(INT HdlImage)
{
	HBITMAP BitMap;
	LONG MemoSize;
    BYTE *GBuffer;
	HDC hDC;
	HGLOBAL hgm;
	IMGHEADER *lpIMG;
	BYTE *lpSource;

    lpSource=memoLock(HdlImage);
	lpIMG=(IMGHEADER *) lpSource;
	lpSource+=lpIMG->Offset;

	// ---------------------------------------------
	// Creazione del Bitmap True Color             |
	// ---------------------------------------------
    MemoSize=(lpIMG->bmiHeader.biHeight*lpIMG->linesize);
	hgm=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,MemoSize);
	if (hgm==NULL) ehExit("No memory");
	GBuffer=GlobalLock(hgm);

	// ---------------------------------------------
	// Stampa del Bitmap                           |
	// ---------------------------------------------
	hDC=GetDC(NULL); // DA VEDERE
	lpIMG->bmiHeader.biHeight*=-1;
	BitMap=CreateDIBitmap(hDC, //Handle del contesto
						  &lpIMG->bmiHeader,
						  CBM_INIT,
						  lpSource,// Dati del'icone
						  (BITMAPINFO *) &lpIMG->bmiHeader,
						  DIB_RGB_COLORS);
	lpIMG->bmiHeader.biHeight*=-1;
    memoUnlock(HdlImage);

	ReleaseDC(NULL,hDC); 
	if (BitMap==NULL) 
	{
	  GlobalUnlock(hgm);
	  GlobalFree(hgm);
	  return NULL;
	}
  
  GlobalUnlock(hgm);
  GlobalFree(hgm);
  return BitMap;
}

// -------------------------------------------------------------------
// Copia nel ClipBoard
// -------------------------------------------------------------------
#ifndef _NODC
BOOL IMGToClipboard(INT HdlImage)
{
	HBITMAP hBitMap=NULL;
	if (OpenClipboard(WindowNow()))
	{
	EmptyClipboard();
	hBitMap=IMGToBitMap(HdlImage);
	if (hBitMap!=NULL) SetClipboardData(CF_BITMAP,hBitMap);
	CloseClipboard();
	}
	if (hBitMap!=NULL) return FALSE; else return TRUE;
}
#endif

INT IMGCut(INT iHdlPhoto, // Handle della foto
			POINT *lpFoto, // Punto in cui vuoi tagliarla
			SIZE  *lpsArea, // Dimensioni del taglio
			SIZE  *lpsReal,
			BOOL  fFreeSource) // T/F se devo liberare il sorgente
{
	IMGHEADER *psImgSorg;
	IMGHEADER *psImgDest;
	BITMAPINFOHEADER *psBHSorg;
	BITMAPINFOHEADER *psBHDest;
	BYTE *lpi;
	BYTE *lpj;
	BYTE *lpSorg;
	SIZE sImage;
	INT y;
	INT HdlNew;
	BYTE *lpDest;
	INT iNewLineSize;
	INT iNewSize;
	INT xCutSize;

	lpi=memoLock(iHdlPhoto);
	psImgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+psImgSorg->Offset;
	
	psBHSorg=(BITMAPINFOHEADER *) &psImgSorg->bmiHeader;
	sImage.cy=psBHSorg->biHeight;
    sImage.cx=psBHSorg->biWidth;

	//if (psImgSorg->bmiHeader.biBitCount!=24) {ehExit("IMGCut 24 bit");}

	// Controllo le dimensioni del taglio
	ehMemCpy(lpsReal,lpsArea,sizeof(SIZE));
	if ((lpFoto->x+lpsArea->cx)>sImage.cx) lpsReal->cx=sImage.cx-lpFoto->x; else lpsReal->cx=lpsArea->cx;
	if ((lpFoto->y+lpsArea->cy)>sImage.cy) lpsReal->cy=sImage.cy-lpFoto->y; else lpsReal->cy=lpsArea->cy;

	// Ricalcolo l'occupazione di spazio
	iNewLineSize=lpsReal->cx*psImgSorg->iChannels;
	iNewLineSize=((iNewLineSize+psImgSorg->iChannels)>>2)<<2;
	iNewSize=(lpsReal->cy*iNewLineSize)+psImgSorg->Offset;

	// Alloco la memoria
	HdlNew=memoAlloc(RAM_AUTO,iNewSize,"NewImage(CUT)");
	if (HdlNew<0) ehExit("IMGRemaker: non memory");
	lpDest=memoLock(HdlNew);
	memset(lpDest,0,iNewSize);

	// Copio l'header
	ehMemCpy(lpDest,lpi,psImgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	psImgDest=(IMGHEADER *) lpDest;
	psBHDest=(BITMAPINFOHEADER *) &psImgDest->bmiHeader;
	psImgDest->linesize=iNewLineSize;
	psBHDest->biWidth=lpsReal->cx;
	psBHDest->biHeight=lpsReal->cy;
	lpDest+=psImgDest->Offset;

	// -----------------------------------------------
	// Effettuo la variazione di luminosità
	//

	lpj=lpSorg+(lpFoto->y*psImgSorg->linesize)+(lpFoto->x*psImgSorg->iChannels);
	xCutSize=lpsReal->cx*psImgSorg->iChannels;
	lpi=lpDest;
	for (y=0;y<lpsReal->cy;y++)
	{
		ehMemCpy(lpi,lpj,xCutSize);
		lpj+=psImgSorg->linesize;
		lpi+=iNewLineSize;
	}
	memoUnlock(HdlNew);
	memoUnlock(iHdlPhoto);
	if (fFreeSource) memoFree(iHdlPhoto,"IMGCUT");
	return HdlNew;
}


void IMGLevelMaker(INT hdlImage,INT *lpiLevels)
{
	INT iRowSize,x,y;
	IMGHEADER *psImgHeader;
	BYTE *pbSorg;

	memset(lpiLevels,0,256*sizeof(INT));
	psImgHeader=memoLock(hdlImage);
	pbSorg=psImgHeader->pbImage;

	iRowSize=psImgHeader->bmiHeader.biWidth*psImgHeader->iChannels;
	for (y=0;y<psImgHeader->bmiHeader.biHeight;++y) 
    {
      for (x=0;x<iRowSize; ++x) 
	  {
		lpiLevels[pbSorg[x]]++;
      }
	  pbSorg+=psImgHeader->linesize;
    }
	memoUnlock(hdlImage);
}


enum { CBuffer=320 };
BYTE bClip[256+CBuffer*2];
BYTE bLevelMap[256];

INT IMGLevel(INT hdl,
			  INT in_min, double gamma, INT in_max, 
			  INT out_min, INT out_max,
			  BOOL fFreeSource)
{
	INT divisor = in_max - in_min + (in_max == in_min);
	INT i;
	double p;
	IMGHEADER *psHeadSource;
	IMGHEADER *psHeadDest;
	BYTE *pbSorg,*pbDest;
	INT hdlNewImage;
	INT x,y,xSize;

	// ---------------------------------------------------
	// Preparo la mappa per il calcolo
	//
	if (gamma <= 0.0) return hdl; //win_infoarg("Levels: gamma must be positive");
	gamma = 1/gamma;

	//memset(bClip, 0, CBuffer); // Azzero il buffer
	ZeroFill(bClip);
	for (i=0; i<256; ++i) bClip[i+CBuffer] = i; // Inserisco la scala
	memset(bClip+CBuffer+256, 255, CBuffer); // Metto il fondo scala 

	for (i=0; i<256; ++i) 
	{
	  p = (double) (i - in_min) / divisor;
	  p = pow(min(max(p, 0.0), 1.0), gamma);
	  p = p * (out_max - out_min) + out_min;
	  bLevelMap[i] = bClip[(int) (p+0.5)+CBuffer];
	}

 // 
 // A) Creo la nuova immagine
 //

	psHeadSource=(IMGHEADER *) memoLock(hdl);
    pbSorg=psHeadSource->pbImage;
	if (psHeadSource->iChannels!=3) ehExit(__FUNCTION__":iChannels !=3 byte");
	//psBHSorg=(BITMAPINFOHEADER *) &imgSorg->bmiHeader;

	if (!fFreeSource)
	{
		hdlNewImage=IMGCreate(	psHeadSource->enPixelType,
								psHeadSource->utfFileName,
								psHeadSource->bmiHeader.biWidth,
								psHeadSource->bmiHeader.biHeight,
								NULL,
								FALSE);
		
		if (hdlNewImage<0) ehExit("IMGRemaker: non memory");
		psHeadDest=(IMGHEADER *) memoLock(hdlNewImage);
		pbDest=psHeadDest->pbImage;
	}
	else
	{
		psHeadDest=psHeadSource;
		pbDest=pbSorg;
		hdlNewImage=hdl;
	}

	//
	// Copio l'header
	//
	xSize=(psHeadSource->bmiHeader.biWidth*psHeadSource->iChannels);
	for (y=0;y<psHeadSource->bmiHeader.biHeight;y++) 
	{
		for (x=0;x<xSize; x++) 
		{
 			pbDest[x] = bLevelMap[pbSorg[x]];
		}
		pbDest+=psHeadDest->linesize;
		pbSorg+=psHeadSource->linesize;
	}

   if (!fFreeSource) memoUnlock(hdlNewImage);
   memoUnlock(hdl);
//   if (fFreeSource) memoFree(hdl,"IMGLEVEL");
   return hdlNewImage;
}


static void ControlloRGB(double *Red,double *Green,double *Blue)
{
	if (*Red>255.0) *Red=255.0;
	if (*Red<0.0) *Red=0.0;
	if (*Green>255.0) *Green=255.0;
	if (*Green<0.0) *Green=0.0;	
	if (*Blue>255.0) *Blue=255.0;
	if (*Blue<0.0) *Blue=0.0;	
}

void RGBtoHSL(double dRed,double dGreen,double dBlue,double *Hue,double *Sat,double *Lum)
{
	DOUBLE	CMax,CMin,Delta;

	dRed/=255.0;
	dGreen/=255.0;
	dBlue/=255.0;

	CMax=dRed;
	if (dGreen>CMax) CMax=dGreen;
	if (dBlue>CMax) CMax=dBlue;	

	CMin=dRed;
	if (dGreen<CMin) CMin=dGreen;
	if (dBlue<CMin) CMin=dBlue;	
		
	*Lum=(CMax+CMin)/2.0;
	
	if (CMax==CMin)
	{
		*Sat=0.0;
		*Hue=0.0;
	}
	else
	{
		if (*Lum<0.5) *Sat=(CMax-CMin)/(CMax+CMin);
		else *Sat=(CMax-CMin)/(2.0-CMax-CMin);	    
		Delta=CMax-CMin;
		while (1)
		{
			if (dRed==CMax) {*Hue=(dGreen-dBlue)/Delta; break;}
			if (dGreen==CMax) {*Hue=2.0+(dBlue-dRed)/Delta; break;}
			*Hue=4.0+(dRed-dGreen)/Delta; break;		
		}		
		*Hue=*Hue/6.0;
		if (*Hue<0.0) *Hue=*Hue+1.0;
	}
							
	*Hue*=360.0;
	*Sat*=100.0;
	*Lum*=100.0;
}

void HSLtoRGB(double dHue,double dSat,double dLum,
			  double *Red,double *Green,double *Blue)
{
	while (1)
	{
		if (dHue<=60.0)
		{
			*Red=255.0;
			*Green=(255.0/60.0)*dHue;
			*Blue=0.0;
			break;
		}
		if (dHue<=120.0)
		{
			*Red=255.0-(255.0/60.0)*(dHue-60.0);
			*Green=255.0;
			*Blue=0.0;
			break;
		}
		if (dHue<=180.0)
		{
			*Red=0.0;
			*Green=255.0;
			*Blue=(255.0/60.0)*(dHue-120.0);
			break;
		}
		if (dHue<=240.0)
		{
			*Red=0.0;
			*Green=255.0-(255.0/60.0)*(dHue-180.0);
			*Blue=255.0;
			break;
		}
		if (dHue<=300.0)
		{
			*Red=(255.0/60.0)*(dHue-240.0);
			*Green=0.0;
			*Blue=255.0;
			break;
		}
				     
		*Red=255.0;
		*Green=0.0;
		*Blue=255.0-(255.0/60.0)*(dHue-300.0);
		break;
		
	}		
		
	dSat=(dSat-100.0)/100.0;
	if (dSat<0.0) dSat*=-1;
	*Red-=(*Red-128.0)*dSat;
	*Green-=(*Green-128.0)*dSat;
	*Blue-=(*Blue-128.0)*dSat;

	dLum=(dLum-50.0)/50.0;
	if (dLum>0.0)
	{
		*Red+=(255.0-*Red)*dLum;
		*Green+=(255.0-*Green)*dLum;
		*Blue+=(255.0-*Blue)*dLum;
	}
	else if (dLum<0.0)
	{
		*Red+=*Red*dLum;
		*Green+=*Green*dLum;
		*Blue+=*Blue*dLum;
	}		
}


INT IMGSatLum(INT hdlSource,
			   INT iSaturazione,
			   INT iTonalita,
			   INT iLuminosita,
			   INT iContrasto)
{
	INT	b,c;	
    INT    iNewSize;
    INT    hdlDest;
	INT		iRed,iGreen,iBlue,iGray;
	double  dRed,dGreen,dBlue;
	double	dHue,dSat,dLum;
	BYTE	*lpRead,*lpWrite;
	BYTE	*lpi;
	BYTE	*lpSorg;
	BYTE	*lpDest;

	BITMAPINFOHEADER *psBHSorg;
	IMGHEADER *imgSorg;
	IMGHEADER *imgDest;


	lpi=memoLock(hdlSource);
	imgSorg=(IMGHEADER *) lpi;
    lpSorg=lpi+imgSorg->Offset;

	psBHSorg=(BITMAPINFOHEADER *) &imgSorg->bmiHeader;

	if (imgSorg->iChannels!=3) {ehExit("iChannels !=3 bit");}
	iNewSize=(psBHSorg->biHeight*imgSorg->linesize)+imgSorg->Offset;

	// Alloco la memoria
	hdlDest=memoAlloc(RAM_AUTO,iNewSize,"NewImage(Sat)");	if (hdlDest<0) ehExit("IMGSatLum(): non memory");
	lpDest=memoLock(hdlDest);	

	// Copio l'header
	ehMemCpy(lpDest,lpi,imgSorg->Offset);

	// Aggiorno i dati delle dimensioni
	imgDest=(IMGHEADER *) lpDest;
	lpDest+=imgDest->Offset;


	for (b=0;b<imgSorg->bmiHeader.biHeight;b++)
	{
		lpRead=lpSorg; lpWrite=lpDest;
		for (c=0;c<imgSorg->linesize;c+=3)
		{					
			iBlue=lpRead[0]; iGreen=lpRead[1]; iRed=lpRead[2];
			
			if (iSaturazione)
			{
				iGray=(iRed+iGreen+iBlue)/3;
				iRed+=((iRed-iGray)*iSaturazione)/100;
				iGreen+=((iGreen-iGray)*iSaturazione)/100;
				iBlue+=((iBlue-iGray)*iSaturazione)/100;
			}			

			if (iTonalita || iLuminosita || iContrasto) 
			{
				dRed=iRed; dGreen=iGreen; dBlue=iBlue;
				
				if (iTonalita)
				{
					RGBtoHSL(dRed,dGreen,dBlue,&dHue,&dSat,&dLum);						 			
					dHue+=dHue*iTonalita/100.0;
					if (dHue>360.0) dHue=360.0;
					if (dSat>100.0) dSat=100.0;
					if (dLum>100.0) dLum=100.0;
					if (dHue<0.0) dHue=0.0;
					if (dSat<0.0) dSat=0.0;
					if (dLum<0.0) dLum=0.0;
					HSLtoRGB(dHue,dSat,dLum,&dRed,&dGreen,&dBlue);								
				}

				if (iLuminosita)
				{
					dRed*=1.0+iLuminosita/100.0;
					dGreen*=1.0+iLuminosita/100.0;
					dBlue*=1.0+iLuminosita/100.0;
				}

				if (iContrasto)
				{
					dRed+=iContrasto/100.0*(iRed-127.0);
					dGreen+=iContrasto/100.0*(iGreen-127.0);
					dBlue+=iContrasto/100.0*(iBlue-127.0);
				}

				iRed=(INT) dRed; iGreen=(INT) dGreen; iBlue=(INT) dBlue;
			}

			if (iRed>255) iRed=255;
			if (iRed<0) iRed=0;
			if (iGreen>255) iGreen=255;
			if (iGreen<0) iGreen=0;	
			if (iBlue>255) iBlue=255;
			if (iBlue<0) iBlue=0;

			lpWrite[0]=(CHAR) iBlue; lpWrite[1]=(CHAR) iGreen; lpWrite[2]=(CHAR) iRed;
			lpRead+=3;	lpWrite+=3;
		}
		lpSorg+=imgSorg->linesize;
		lpDest+=imgDest->linesize;
	}
	memoUnlock(hdlSource);	
	memoUnlock(hdlDest);	
	return hdlDest;
}

