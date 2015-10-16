//   ---------------------------------------------
//   | ImgResizer
//   | Crea un'immagine di differente grandezza
//   | 
//   |          by Ferrà Art & Tecnology 1993-2002
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include <fcntl.h>
#include "/easyhand/ehtool/imgutil.h"
#include "/easyhand/ehtool/imgtool.h"
#include "/easyhand/ehtool/imgPowerToy.h"
#include "/easyhand/inc/imgResizer.h"

// INT JPGGetFactor(SIZE *lpsSource,SIZE *lpsDest,INT *lpiPerc)
// Indicar
static void TextProcessor(INT hdlImageEnd,CHAR *lpText);
void LPrint(HDC hdc,
			INT x,
			INT y,
			LONG col1,
			LONG col2,
			CHAR *lpFont,
			INT iAlt,
			BOOL fBold,
			CHAR *lpString);

static CHAR *LNoSpaceCue(CHAR *lp)
{
  CHAR *lpRet=lp;
  INT a,iLen;
  iLen=strlen(lp); 
  if (!iLen) return lpRet;
  
  lp=lp+(iLen-1);
  for (a=0;a<iLen;a++,lp--)
  {
	if (*lp==' ') *lp=0; else break;		  
  }
  return lpRet;
}

//
// imgResizer()
//

BOOL imgResizer(CHAR *	lpFile, 
				CHAR *	lpAdd, 
				INT		iX,INT iY,
				INT		iQuality,
				INT		iOrientation,
				BOOL	fMakeEver,
				INT		iResampling)
{
	CHAR szNewFile[255];
	CHAR szExt[10];
	CHAR *lpMess="";
	INT fTrack=FALSE;
	CHAR *lp;
	INT iErr;

	if (!lpFile) return 0;

	//
	// Controllo se esiste il file
	//
	if (!fileCheck(lpFile)) 
	{
		printf("ko:Il file %s non esiste",lpFile);
		return TRUE;
	}

	strcpy(szNewFile,lpFile);
	*szExt=0;
	lp=strReverseStr(szNewFile,"."); if (lp) {strcpy(szExt,lp); *lp=0;}
	strcat(szNewFile,lpAdd); strcat(szNewFile,".jpg");//szExt);
	iErr=JPGNewFile(lpFile,szNewFile,iX,iY,iQuality,TRUE,iResampling,IMG_PIXEL_BGR);
	if (iErr!=0)
	{
		if (!fMakeEver)
		{
			printf("ko");
			switch (iErr)
			{
			case -1: printf("[%s] Errore in lettura file",lpFile); break;
			case -2: printf("[%s] Errore in scrittura file",szNewFile); break;
			case -3: printf("[%s] Errore in ridimensionamento",lpFile); break;
			}
		}
		else
		{
			if (!CopyFile(lpFile,szNewFile,FALSE))
			{
				printf("ko");
				printf("[%s] Errore in copia file",lpFile);
			}
		}
	}

	//printf("ok");
	return FALSE;
} // ExtSrcExecuteQuery

//
// imgResizerEx()
//
BOOL imgResizerEx(UTF8 *	pszFileSource,		// Nome del file sorgente
				  UTF8 *	pszFileDest,			// Stringa da aggiungere al nome del file
				  SIZE *	psDimFix,			// Dimensioni della foto finale
				  SIZE *	psMin,			// Dimensioni massime della foto finale (new 2015)
				  SIZE *	psMax,			// Dimensioni massime della foto finale (new 2015)
				  INT		iQuality,			// 10>100 Qualità del JPG
				  double	dPhotoPerc,			// Percentuale dell'immagine da occupare 25= 1/4
				  BOOL		fMakeEver,			// T/F: Costruisce sempre
				  INT		iResampling,		// Tipo di ricampionamento TRS_LANCZOS=10
				  EN_IMGPT	enPhotoAdatta,		// Tipo di adattamento alla foto (vedi EN_IMGPT)
												// 0 = Proporzionale
												// 1 = Best in fit (Adatta al formato)
												// 2 = adatta al corto
												// 3 = adatta al lungo
												// 5 = Taglia

				  INT		cBackColor,			// Colore di background (per riempire gli spazi vuoti)
				  INT		iAlignH,			// Allineamento foto orizzontale 0=Centrale, 1=Left, 2=Right
				  INT		iAlignV,			// Allineamento foto verticale   0=Centrale, 1=Top,  2=Down
				  INT		iPhotoOffsetX,		// Correzione su allineamento Orizzontale
				  INT		iPhotoOffsetY,		// Correzione su allineamento Verticale
				  INT		iImageAlpha,		// Percentuale di trasparenza dell'immagine con il background
				  BOOL		fAutoLevel,
				  INT		iOrientation,		// Rotazione Exif dell'immagine

				  CHAR *	lpLogoFile,			// Logo del file da fondere all'immagine (può non esserci)
				  double	dLogoPerc,			// Percentuale dell'immagine da occupare 25= 1/4
				  SIZE *	psLogoFix,			// Dimensioni fise del logo (non proporzionali alla foto) (new 2015)
				  INT		iLogoAlignH,		// Allineamento foto orizzontale 0=Centrale, 1=Left, 2=Right
				  INT		iLogoAlignV,		// Allineamento foto verticale 0=Centrale, 1=Top, 2=Down
				  INT		iLogoOffsetX,		// Correzione su allineamento Orizzontale
				  INT		iLogoOffsetY,		// Correzione su allineamento Verticale
				  INT		iLogoAlpha,
				  BOOL		fLogoPos,			// 0 sulla nuova Immagine calcolata, 1= Sull'immagine originale

				  CHAR *	lpText)				// Testo da stampare
{
	IMG_RESIZE sImgResize;
	_(sImgResize);
	sImgResize.pszFileSource=pszFileSource;
	sImgResize.pszFileDest=pszFileDest;
	memcpy(&sImgResize.sFix,psDimFix,sizeof(SIZE));
	sImgResize.psMin=psMin;
	sImgResize.psMax=psMax;
//	memcpy(&sImgResize.sMax,&sDimMax,sizeof(SIZE));
	sImgResize.iQuality=iQuality;
	sImgResize.fMakeEver=fMakeEver;
	sImgResize.iResampling=iResampling;
	sImgResize.iOrientation=iOrientation;
	sImgResize.enPhotoAdatta=enPhotoAdatta;
	sImgResize.cBackColor=cBackColor;
	sImgResize.bAutoLevel=fAutoLevel;
	sImgResize.lpText=lpText;
	sImgResize.bQuickLoading=true;

	sImgResize.sPhoto.dPerc=dPhotoPerc;
	sImgResize.sPhoto.iAlignHor=iAlignH;
	sImgResize.sPhoto.iAlignVer=iAlignV;
	sImgResize.sPhoto.iOffsetX=iPhotoOffsetX;
	sImgResize.sPhoto.iOffsetY=iPhotoOffsetY;
	sImgResize.sPhoto.iAlpha=iImageAlpha;

	if (lpLogoFile)
	{
		sImgResize.sLogo.bPosWhere=fLogoPos;
		strcpy(sImgResize.sLogo.szFile,lpLogoFile);
		sImgResize.sLogo.iAlignHor=iLogoAlignH;
		sImgResize.sLogo.iAlignVer=iLogoAlignV;
		sImgResize.sLogo.iOffsetX=iLogoOffsetX;
		sImgResize.sLogo.iOffsetY=iLogoOffsetY;
		sImgResize.sLogo.iAlpha=iLogoAlpha;
		sImgResize.sLogo.dPerc=dLogoPerc;
		sImgResize.sLogo.psSizeFix=psLogoFix;
	}
	return imgResize(&sImgResize);
}



//
// imgResize(()
//
BOOL imgResize(IMG_RESIZE * psImgResize)
{
	CHAR *lpMess="";
	INT fTrack=FALSE;
	INT iErr;
	EN_FILE_TYPE enImageType=0;
    IMGHEADER ImgHead;
	POINT ptArea;
	POINT ptPhoto;
	RECT srRectSource,srRectLogo;

	SIZE sDest; // Dimensione destinazione
	RECT rDest; // Rettangolo destinazione

	INT		hdlImage;
	INT		hdlImageNew;
	INT		hdlImageEnd;
	SIZE	sPhotoDest;
	DWORD	dw;
	BOOL	bFillBack;

	if (!psImgResize->pszFileSource) return TRUE;

	// 
	// Controllo se esiste il file
	//
	if (!fileCheck(psImgResize->pszFileSource)) 
	{
		printf("Il file %s non esiste",psImgResize->pszFileSource);
		ehLogWrite("Il file %s non esiste",psImgResize->pszFileSource);
		return TRUE;
	}

	// 
	// Controllo LogoFile
	//
	if (*psImgResize->sLogo.szFile)
	{
		if (!fileCheck(psImgResize->sLogo.szFile)) 
		{
			printf("Il logofile %s non esiste",psImgResize->sLogo.szFile);
			ehLogWrite("Il logofile %s non esiste",psImgResize->sLogo.szFile);
			return TRUE;
		}
	}

//	remove(szNewFile);

	// ------------------------------------------------------------------------
	// A) Leggere le dimensioni del sorgente
	//
	
	enImageType=IMG_UNKNOW;
	switch (isImage(psImgResize->pszFileSource))
	{
		case IMG_JPEG:
			if (JPGReadHeader(psImgResize->pszFileSource,&ImgHead,JME_HIDE)) enImageType=IMG_JPEG;
			break;

		case IMG_GIF:
			if (GIFReadHeader(psImgResize->pszFileSource,&ImgHead,JME_HIDE)) enImageType=IMG_GIF;
			break;

		case IMG_PNG:
			if (PNGReadHeader(psImgResize->pszFileSource,&ImgHead,JME_HIDE)) enImageType=IMG_PNG;
			break;
		
		default:
			break;
	}

	if (!enImageType)
	{
//		printf("ko");
		printf("%s: errore in lettura Header",psImgResize->pszFileSource);
		ehLogWrite("%s: errore in lettura Header",psImgResize->pszFileSource);
		return TRUE;
	}
	

	sPhotoDest.cx=(INT) (psImgResize->sFix.cx*psImgResize->sPhoto.dPerc/100);
	sPhotoDest.cy=(INT) (psImgResize->sFix.cy*psImgResize->sPhoto.dPerc/100);

	// 
	// B) Calcolare le nuove dimensioni
	//
	IMGCalcSize(&ImgHead,		// Dimensioni del sorgente
				&sPhotoDest,	// Area disponibile
				psImgResize->psMin,
				psImgResize->psMax,
				psImgResize->enPhotoAdatta,  // Tipo di adattamento
				psImgResize->sPhoto.iAlignHor,	   // Allineamento orizzontale
				psImgResize->sPhoto.iAlignVer,	   // Allineamento verticale
				&sDest,		   // Dimensioni della destinazione
				&rDest,
				&srRectSource); 	   // Posizionamento in destinazione
	if (!sPhotoDest.cy||!sPhotoDest.cx) 
		memcpy(&sPhotoDest,&sDest,sizeof(sDest));

	memcpy(&psImgResize->sDest,&sDest,sizeof(sDest));
	//
	// C) Carica il sorgente in memoria
	//
	switch (ImgHead.enType)
	{
		BOOL bError=false;
		
		case IMG_JPEG:

			//
			// Lettura veloce
			//
			if (psImgResize->bQuickLoading) {

				INT iPerc,iFactor;
				SIZE sizSource,sAp1;
				RECT sAp2;
				sizSource.cx=ImgHead.bmiHeader.biWidth;
				sizSource.cy=ImgHead.bmiHeader.biHeight;
				iFactor=JPGGetFactor(&sizSource,&sPhotoDest,&iPerc);
				bError=!JPGReadFileEx(psImgResize->pszFileSource,&hdlImage,NULL,iFactor,JDCT_IFAST,NULL,FALSE,IMG_PIXEL_RGB);
				if (!bError) {
						
					// Letto + piccolo, calcolo nuove dimensioni
					IMGHEADER * psImgHead;
					//printf("[%d:%d]" CRLF,bError,hdlImage);
					if (hdlImage>-1) {
						psImgHead=(IMGHEADER *) memoLock(hdlImage);
						IMGCalcSize(psImgHead,      // Dimensioni del sorgente
									&sPhotoDest,	   // Area disponibile
									psImgResize->psMin,
									psImgResize->psMax,
									psImgResize->enPhotoAdatta,  // Tipo di adattamento
									psImgResize->sPhoto.iAlignHor,	   // Allineamento orizzontale
									psImgResize->sPhoto.iAlignVer,	   // Allineamento verticale
									&sAp1,		   // Dimensioni della destinazione
									&sAp2,
									&srRectSource); 	   // Posizionamento in destinazione				
									}
						memoUnlockEx(hdlImage,"a1");
					} else bError=TRUE;
			}
			//
			// Lettura dell'intera immagine
			//
			else {
				bError=!JPGReadFile(psImgResize->pszFileSource,&hdlImage,NULL,NULL,FALSE,IMG_PIXEL_RGB);
			}

			if (bError)
			{
#ifdef EH_CONSOLE
				printf("ko:[%s] errore in lettura JPG",psImgResize->pszFileSource);
#endif
				ehLogWrite("%s: errore in lettura JPG",psImgResize->pszFileSource);
				return TRUE;
			}
			//hdlImage=IMGToRGB(hdlImage,1);
			break;

		case IMG_GIF:
			if (!GIFReadFile(psImgResize->pszFileSource,&hdlImage,psImgResize->cBackColor,FALSE,IMG_PIXEL_RGB)) 
			{
				printf("ko");
				printf("%s: errore in lettura GIF",psImgResize->pszFileSource);
				ehLogWrite("%s: errore in lettura GIF",psImgResize->pszFileSource);
				printf("Errore"); //getch();
				return TRUE;
			}
			break;

		case IMG_PNG:
			if (!PNGReadFile(psImgResize->pszFileSource,&hdlImage,NULL,FALSE)) 
			{
				printf("ko");
				printf("%s: errore in lettura PNG",psImgResize->pszFileSource);
				ehLogWrite("%s: errore in lettura PNG",psImgResize->pszFileSource);
				printf("Errore"); //getch();
				return TRUE;
			}
			break;

		default:
			printf("ko");
			printf("%s: formato non gestito",psImgResize->pszFileSource);
			ehLogWrite("%s: formato non gestito",psImgResize->pszFileSource);
			return TRUE;
	}

	// 
	// D) Ridimensionarlo
	//

	if (sDest.cy<16) 
		hdlImageNew=IMGRemaker(	hdlImage,
								&srRectSource,
								sDest.cx,
								sDest.cy,
								TRUE,
								psImgResize->iResampling);
		else
		hdlImageNew=IMGResampling(	hdlImage,
									&srRectSource,
									sDest.cx,
									sDest.cy,
									psImgResize->iResampling);
	if (hdlImageNew<0)
	{
#ifdef EH_CONSOLE
		printf("ko:%s: errore in IMGRemaker() %d",psImgResize->pszFileSource,hdlImageNew);
#endif
		ehLogWrite("%s: errore in IMGRemaker() %d",psImgResize->pszFileSource,hdlImageNew);
		memoFree(hdlImage,"Img1"); // Libero memoria immagine
		hdlImage=-1;
		return TRUE;
	}

	//
	// DX) Eseguo (eventuali) rotazioni/mirror dell'immagine 
	//
	if (psImgResize->iOrientation>1) {

		INT hdlImage;
		
		switch (psImgResize->iOrientation) {
		
			case 2: // "flip horizontal",  // left right reversed mirror
				IMGMirrorX(hdlImageNew);
				break;

			case 3: // Rotate 180
				hdlImage=IMGRotation(ROT_180,hdlImageNew);
				memoFree(hdlImageNew,"rot");
				hdlImageNew=hdlImage;
				break;

			case 4: // upside down mirror
				IMGMirrorY(hdlImageNew);
				break;

			case 5: // Flipped about top-left <--> bottom-right axis.
			case 7: // flipped about top-right <--> bottom-left axis
				IMGMirrorX(hdlImageNew);
				IMGMirrorY(hdlImageNew);
				break;

			case 6: // rotate 90 cw to right it.
				hdlImage=IMGRotation(ROT_270,hdlImageNew);
				dw=sPhotoDest.cx; sPhotoDest.cx=sPhotoDest.cy; sPhotoDest.cy=dw;
				memoFree(hdlImageNew,"rot");
				hdlImageNew=hdlImage;
				break;

			case 8: // rotate 270 to right it.
				hdlImage=IMGRotation(ROT_90,hdlImageNew);
				dw=sPhotoDest.cx; sPhotoDest.cx=sPhotoDest.cy; sPhotoDest.cy=dw;
				memoFree(hdlImageNew,"rot");
				hdlImageNew=hdlImage;
				break;
				
		}
	}

	// ------------------------------------------------------------------------
	// E) Creare un'immagine con il colore di background scelto	 
	//	  2012 release: se trasparente rimane tale
	//
	bFillBack=true;
	if (ImgHead.enPixelType!=IMG_PIXEL_RGB_ALPHA) {

		if (psImgResize->enPhotoAdatta!=IMGPT_MAX_SIDE) 
			hdlImageEnd=IMGCreate(IMG_PIXEL_RGB,"newImage",sPhotoDest.cx,sPhotoDest.cy,NULL,FALSE);
			else
			{hdlImageEnd=IMGCreate(IMG_PIXEL_RGB,"newImage",sDest.cx,sDest.cy,NULL,FALSE); bFillBack=false;}

	} else {

		if (psImgResize->enPhotoAdatta!=IMGPT_MAX_SIDE) 
			hdlImageEnd=IMGCreate(IMG_PIXEL_RGB_ALPHA,"newImage",sPhotoDest.cx,sPhotoDest.cy,NULL,FALSE);
			else
			{hdlImageEnd=IMGCreate(IMG_PIXEL_RGB_ALPHA,"newImage",sDest.cx,sDest.cy,NULL,FALSE); }
	
	}
	
	if (bFillBack) IMGFill(hdlImageEnd,psImgResize->cBackColor);

	// ------------------------------------------------------------------------
	// E2) Applico gli autolivelli se richiesto
	//

	if (psImgResize->bAutoLevel)
	{
		if (ImgHead.iChannels==3) 
		{	
			hdlImageNew=IMGAutoLevel(hdlImageNew);
		}
	}

	//
	//	Ricalcolo il posizionamento
	//
	if (psImgResize->enPhotoAdatta!=IMGPT_MAX_SIDE) {

		if (psImgResize->enPhotoAdatta) {

			RectCalcSize(	&sDest,		// Dimensioni del sorgente
							&psImgResize->sFix,			// Dimensione Area destinazione finale
							psImgResize->psMin,			// Dimensione Area destinazione finale
							psImgResize->psMax,			// Dimensione Area destinazione finale
							IMGPT_NO, // Tipo di adattamento
							psImgResize->sPhoto.iAlignHor,		// Allineamento orizzontale
							psImgResize->sPhoto.iAlignVer,		// Allineamento verticale
							&psImgResize->sDest,		// Dimensioni della destinazione
							&rDest,
							NULL); 		// Posizionamento in destinazione
			memcpy(&sDest,&psImgResize->sDest,sizeof(SIZE));

		 } else _(rDest);

		// ------------------------------------------------------------------------
		// F) Inserire il sorgente nella destinazione
		//
		ptPhoto.x=rDest.left+psImgResize->sPhoto.iOffsetX;
		ptPhoto.y=rDest.top+psImgResize->sPhoto.iOffsetY;

	} else {ptPhoto.x=0; ptPhoto.y=0;}

	IMGCopy(hdlImageEnd, // --> La destinazione
			hdlImageNew, // <-- Il sorgente
			ptPhoto,		 // La posizione
			psImgResize->sPhoto.iAlpha);
	
	memoFree(hdlImageNew,"Img1");
	memoFree(hdlImage,"Img1"); // Libero memoria immagine
	hdlImage=-1;

	// 
	// G2) PAINT - Ping moltiplicato -------------------------------------------------------------
	//
	if (!strEmpty(psImgResize->sPaint.szFile))
	{
		INT iErr;
		INT hdlLogo=-1,hdlLogoNew=-1;
		IMGHEADER ImgLogoHead;
		SIZE sLogoDest;
		RECT rLogoDest;
//		BOOL bOffset;
		
		// a) Carico il PNG in memoria
		if (!PNGReadFile(psImgResize->sPaint.szFile,&hdlLogo,&iErr,FALSE))
		{
			printf("ko");
			printf("%s: errore in PNGReadFile() %d",psImgResize->sPaint.szFile,iErr);
			ehLogWrite("%s: errore in PNGReadFile() %d",psImgResize->sPaint.szFile,iErr);
			memoFree(hdlImageEnd,"Img2");
			return TRUE;
		}

		memcpy(&ImgLogoHead,memoLock(hdlLogo),sizeof(IMGHEADER));
		memoUnlockEx(hdlLogo,"A3");

		if (psImgResize->sPaint.psSizeFix) {

			memcpy(&sLogoDest,psImgResize->sLogo.psSizeFix,sizeof(SIZE));

		} else {
			sLogoDest.cx=(INT) (ImgLogoHead.bmiHeader.biWidth*psImgResize->sPaint.dPerc/100);
			sLogoDest.cy=(INT) (ImgLogoHead.bmiHeader.biHeight*psImgResize->sPaint.dPerc/100);
		}

		// b) Calcolo le nuove dimensioni
		IMGCalcSize(&ImgLogoHead,      // Dimensioni del sorgente
					&sLogoDest,		   // Area disponibile
					psImgResize->psMin,
					psImgResize->psMax,
					IMGPT_AL_FORMATO,  // Tipo di adattamento
					0,	   // Allineamento orizzontale
					0,	   // Allineamento verticale
					&sLogoDest,		   // Dimensioni della destinazione
					&rLogoDest,
					&srRectLogo); 	   // Posizionamento in destinazione

		
		// c) Creo (ridimensionando) il nuovo Ping
		hdlLogoNew=IMGResampling(hdlLogo,NULL,sLogoDest.cx,sLogoDest.cy,psImgResize->iResampling);
		memoFree(hdlLogo,"Img1"); // Libero memoria immagine
		hdlLogo=-1;

		if (hdlLogoNew<0)
		{
			printf("ko");
			printf("%s: errore in IMGRemaker(LOGO) %d",psImgResize->sPaint.szFile,hdlLogoNew);
			ehLogWrite("%s: errore in IMGRemaker(LOGO) %d",psImgResize->sPaint.szFile,hdlLogoNew);
			memoFree(hdlImageEnd,"Img2");
			return TRUE;
		}
	
		// Applico
		for (ptArea.y=(psImgResize->sPaint.iOffsetY-sLogoDest.cy);ptArea.y<psImgResize->sDest.cy;ptArea.y+=sLogoDest.cy)
		{
			INT iOffset=psImgResize->sPaint.iOffsetX-sLogoDest.cx;

			for (ptArea.x=iOffset;ptArea.x<psImgResize->sDest.cx;ptArea.x+=sLogoDest.cx)
			{
				POINT ptArea2;
				// printf("Copio: %d,%d (%d)",pArea.x,pArea.y,iLogoAlpha);
				IMGCopy(hdlImageEnd,	// La destinazione
						hdlLogoNew,		// Il sorgente
						ptArea,			// La posizione
						psImgResize->sPaint.iAlpha);

				if (psImgResize->iEchoPaint)
				{
					ptArea2.x=ptArea.x+sLogoDest.cx/4;
					ptArea2.y=ptArea.y+sLogoDest.cy/4;
					IMGCopy(hdlImageEnd,	// La destinazione
							hdlLogoNew,		// Il sorgente
							ptArea2,			// La posizione
							psImgResize->sPaint.iAlpha+psImgResize->iEchoPaint);
				}
			}
		}



		// e) Libero le risorse impegnate
		memoFree(hdlLogoNew,"Logo1"); // Libero memoria immagine
	}


	// 
	// G) Fonde il Ping (se presente)
	//
	if (!strEmpty(psImgResize->sLogo.szFile))
	{
		INT iErr;
		INT hdlLogo=-1,hdlLogoResized=-1;
		IMGHEADER sImgLogoHead;
		SIZE sLogoDest;
		RECT rLogoDest;
		
		if (!PNGReadFile(psImgResize->sLogo.szFile,&hdlLogo,&iErr,FALSE))
		{
			printf("ko: %s: errore in PNGReadFile() %d",psImgResize->sLogo.szFile,iErr);
			ehLogWrite("%s: errore in PNGReadFile() %d",psImgResize->sLogo.szFile,iErr);
			memoFree(hdlImageEnd,"Img2");
			return true;
		}

		memcpy(&sImgLogoHead,memoLock(hdlLogo),sizeof(IMGHEADER));
		memoUnlockEx(hdlLogo,"A4");

		if (psImgResize->sLogo.psSizeFix) {
			
			memcpy(&sLogoDest,psImgResize->sLogo.psSizeFix,sizeof(SIZE));
		
		} else {

			if (psImgResize->sLogo.dPerc<1) psImgResize->sLogo.dPerc=100;
			if (psImgResize->sLogo.bPosWhere) // Posiziono con riferimento alla foto
			{
				sLogoDest.cx=(INT) (sDest.cx*psImgResize->sLogo.dPerc/100);
				sLogoDest.cy=(INT) (sDest.cy*psImgResize->sLogo.dPerc/100);
			
			}
			else {
				sLogoDest.cx=(INT) (psImgResize->sDest.cx*psImgResize->sLogo.dPerc/100);
				sLogoDest.cy=(INT) (psImgResize->sDest.cy*psImgResize->sLogo.dPerc/100);
			}
		}
		
		//
		// Se serve ridimensiono il logo
		//
		if (sImgLogoHead.bmiHeader.biWidth!=sLogoDest.cx||
			sImgLogoHead.bmiHeader.biHeight!=sLogoDest.cy) {

				// b) Calcolo le nuove dimensioni
				IMGCalcSize(&sImgLogoHead,      // Dimensioni del sorgente
							&sLogoDest,		   // Area disponibile
							NULL,
							NULL,
							IMGPT_AL_FORMATO,  // Tipo di adattamento
							0,	   // Allineamento orizzontale
							0,	   // Allineamento verticale
							&sLogoDest,		   // Dimensioni della destinazione
							&rLogoDest,
							&srRectLogo); 	   // Posizionamento in destinazione

				// c) Creo (ridimensionando) il nuovo Ping
				
				if (sLogoDest.cy<16) 
					hdlLogoResized=IMGRemaker(hdlLogo,NULL,sLogoDest.cx,sLogoDest.cy,TRUE,psImgResize->iResampling);
					else
					hdlLogoResized=IMGResampling(hdlLogo,NULL,sLogoDest.cx,sLogoDest.cy,psImgResize->iResampling);

				memoFree(hdlLogo,"Img1"); // Libero memoria immagine
				hdlLogo=-1;
		} else {

			hdlLogoResized=hdlLogo;
			hdlLogo=-1;
		
		}

		if (hdlLogoResized<0)
		{
			printf("ko");
			printf("%s: errore in IMGRemaker(LOGO) %d",psImgResize->sLogo.szFile,hdlLogoResized);
			ehLogWrite("%s: errore in IMGRemaker(LOGO) %d",psImgResize->sLogo.szFile,hdlLogoResized);
			memoFree(hdlImageEnd,"Img2");
			return TRUE;
		}
	

		// 
		// Allineamento Orizzontale
		//
		switch (psImgResize->sLogo.iAlignHor)
		{
			case 1: // Left
				if (psImgResize->sLogo.bPosWhere) ptArea.x=rDest.left; else ptArea.x=0; 
				break;

			case 2: // Right
				if (psImgResize->sLogo.bPosWhere) 
						ptArea.x=rDest.right-sLogoDest.cx;
						else
						ptArea.x=psImgResize->sDest.cx-sLogoDest.cx;
				break;

			default:
			case 0: // Centra (Default)
//				pArea.x=((psImgResize->sDim.cx-sLogoDest.cx)/2);
				ptArea.x=rDest.left+((sDest.cx-sLogoDest.cx)/2);
				break;
		}

		// 
		// Allinamento Verticale
		//
		switch (psImgResize->sLogo.iAlignVer)
		{
			case 1: // Top
				if (psImgResize->sLogo.bPosWhere) ptArea.y=rDest.top; else ptArea.y=0;
				break;

			case 2: // Bottom
				if (psImgResize->sLogo.bPosWhere) ptArea.y=rDest.bottom-sLogoDest.cy; else ptArea.y=sPhotoDest.cy-sLogoDest.cy; // C'era un +1 cazzo
				break;

			default:
			case 0: // Centra (Default)
//				pArea.y=((psImgResize->sDim.cy-sLogoDest.cy)/2);
				ptArea.y=rDest.top+((sDest.cy-sLogoDest.cy)/2);
				break;
		}
		
		ptArea.x+=psImgResize->sLogo.iOffsetX;//iLogoOffsetX;
		ptArea.y+=psImgResize->sLogo.iOffsetY;//iLogoOffsetY;

		// printf("Copio: %d,%d (%d)",pArea.x,pArea.y,iLogoAlpha);
		IMGCopy(hdlImageEnd, // La destinazione
				hdlLogoResized, // Il sorgente
				ptArea,		 // La posizione
				psImgResize->sLogo.iAlpha); //(double) 100);//psImgResize->sLogo.iAlpha);  <-- da controllare il valore dell' alpha

//		if (strstr(psImgResize->pszFileDest,"\\big"))
//			printf("qui");

		// e) Libero le risorse impegnate
		memoFree(hdlLogoResized,"Logo1"); // Libero memoria immagine
	}


	//
	// Se ho dei comandi di testo li processo
	//
	if (!strEmpty(psImgResize->lpText)) TextProcessor(hdlImageEnd,psImgResize->lpText);


	// ------------------------------------------------------------------------
	// H) Salvare l'immagine
	//
	if (!psImgResize->enImageTypeSave) psImgResize->enImageTypeSave=IMG_JPEG; // Per compatibilità con il passato
	switch (psImgResize->enImageTypeSave)
	{
		case IMG_PNG:
			iErr=PNGSaveFile(psImgResize->pszFileDest,hdlImageEnd,psImgResize->iQuality);
			break;

		case IMG_JPEG:
			iErr=JPGSaveFile(psImgResize->pszFileDest,hdlImageEnd,psImgResize->iQuality);
			break;
		
		default:
			ehError();
			break;

	}

	memoFree(hdlImageEnd,"Img2");
	if (!iErr)
	{
#ifdef EH_CONSOLE
		printf("ko:%s: errore in JPGSave() %d",psImgResize->pszFileDest,iErr);
#endif
		ehLogWrite("%s: errore in JPGSave() %d",psImgResize->pszFileDest,iErr);
		return TRUE;
	}

 //printf("ok");
 return FALSE;
}
/*
CHAR * AddToName(CHAR *lpFileSource,CHAR *lpAdd)
{
	static CHAR szNewFile[255];
	CHAR *lp;
	CHAR szExt[10];

	strcpy(szNewFile,lpFileSource);
	*szExt=0;
	lp=strReverseStr(szNewFile,"."); if (lp) {strcpy(szExt,lp); *lp=0;}
	strcat(szNewFile,lpAdd); strcat(szNewFile,szExt);
	return szNewFile;
}
*/

// ---------------------------------------
// TextProcessor()
// Processo i compandi di stampa del testo
// ---------------------------------------

static void TextProcessor(INT hdlImage,CHAR *lpText)
{
	BYTE *lp;
	CHAR szTesto[1024];
	POINT pt;
	IMGHEADER *Img;
	HDC hDC,hDClone;
	HBITMAP hbCopy;
	BYTE *lpSorg;
	INT iLx,iLy;
	BITMAPINFOHEADER *BmpHeader;
	BITMAPINFO *BmpInfo;
	INT iAlt=12;
	INT iCol1=0;
	INT iCol2=-1;
	BOOL fBold=FALSE;

	// ----------------------------------------------------------------------------
	// Creo una zona di memoria tipo video delle dimensioni dell'immagine
	//
	//memcpy(&ImgHeader,memoLock(hdlImage),sizeof(IMGHEADER));
	printf("Elaborazione TextProcessor [%s]" CRLF,lpText); //Sleep(2000);
	//ehLogWrite("> %s" CRLF,lpText);
	if (!*lpText) return;

	Img=memoLock(hdlImage);
	BmpInfo=(BITMAPINFO *) &Img->bmiHeader;
	BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
    iLy=BmpHeader->biHeight; if (iLy<0) iLy=-BmpHeader->biHeight;
    iLx=BmpHeader->biWidth;

    hDC=GetDC(NULL); hDClone=CreateCompatibleDC(hDC); 
	SetMapMode(hDClone, MM_TEXT);
	hbCopy = CreateCompatibleBitmap(hDC, iLx, iLy);
	SelectObject(hDClone, hbCopy);
	ReleaseDC(NULL,hDC);

    lpSorg=(BYTE *) Img;  lpSorg+=Img->Offset;

	// Scrivo l'immagine in questa zona di memoria
	if (StretchDIBits(hDClone, 
					  // Coordinate e dimensioni di stampa a video
					  0,0,
					  iLx, 
					  iLy,

					  // Coordinate e dimensioni di lettura nel sorgente
					  0,
					  iLy+1,
					  iLx,
					  -iLy,
					  lpSorg,
					  (BITMAPINFO *) &Img->bmiHeader,
					  DIB_RGB_COLORS, 
					  SRCCOPY) == GDI_ERROR) {printf("StretchDIBits Failed");}

	// Ci faccio quello che ci devo fare con i comandi
	lp=strtok(lpText,"|");
	*szTesto=0;
	ZeroFill(pt);
	while (lp)
	{
		//printf("[%s]" CRLF,lp); 

		if (!memcmp(lp,"TEXT=",5)) strcpy(szTesto,lp+5);
		if (!memcmp(lp,"PX=",3)) pt.x=atoi(lp+3);
		if (!memcmp(lp,"PY=",3)) pt.y=atoi(lp+3);
		if (!memcmp(lp,"ALT=",3)) iAlt=atoi(lp+3);
		if (!memcmp(lp,"COL=",4)) iCol1=ColorConvert(lp+4);
		if (!memcmp(lp,"BG=",3)) iCol2=ColorConvert(lp+3);
		if (!memcmp(lp,"BOLD=",5)) fBold=atoi(lp+5);
		
		if (*lp=='*') 
		{
			//printf("Stampo: %d,%d,%s" CRLF,pt.x,pt.y,szTesto);
			//ehLogWrite("Stampo: %d,%d,%s",pt.x,pt.y,szTesto);
			LPrint(hDClone,pt.x,pt.y,iCol1,iCol2,"Arial",iAlt,fBold,szTesto);
		}

		lp=strtok(NULL,"|");
	}

	// Mi riprendo la zona di memoria video e la rimetto nell'immagine
	//BmpHeader->biHeight*=-1;
	GetDIBits(
		hDClone,           // handle to device context
		hbCopy,      // handle to bitmap
		0,   // first scan line to set in destination bitmap
		iLy,   // number of scan lines to copy
		lpSorg,    // address of array for bitmap bits
		(BITMAPINFO *) &Img->bmiHeader, // address of structure with bitmap data
		DIB_RGB_COLORS        // RGB or palette index
		);
	
	if (!DeleteDC(hDClone)) 
	{
		ehLogWrite("Errore in cancellazione DC %d",GetLastError());
		ehExit("Errore in cancellazione DC");
	}

	if (!DeleteObject(hbCopy))
	{
		ehLogWrite("Errore in cancellazione Bitmap %d",GetLastError());
		ehExit("Errore in cancellazione Bitmap");
	}

	memoUnlockEx(hdlImage,"A5");
	IMGMirrorY(hdlImage);
	
	// Libero le risorse

	//TextOut(


}


void LPrint(HDC hdc,INT x,INT y,LONG col1,LONG col2,CHAR *lpFont,INT iAlt,BOOL fBold,CHAR *lpString)
{ 
	HFONT hfont=NULL,hfontOld;

	// --------------------------------------------
	// Creo il Font
	hfont=CreateFont(iAlt, // Altezza del carattere
				     0, // Larghezza del carattere (0=Default)
				     0, // Angolo di rotazione x 10
				     0, //  Angolo di orientamento bo ???
				     fBold ? FW_BOLD : 0, // Spessore del carattere (MACRO Ex: FW_BOLD)
				     0,//sys.fFontItalic, // Flag Italico    ON/OFF
				     0,//sys.fFontUnderline, // Flag UnderLine  ON/OFF
				     0, // Flag StrikeOut  ON/OFF
				     DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
//					   FERRA_OUTPRECISION, // Output precision
				     OUT_DEFAULT_PRECIS, // Output precision
				     0, // Clipping Precision
				     DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
				     DEFAULT_PITCH,//!FONT_fix ? VARIABLE_PITCH : FIXED_PITCH, // Pitch & Family (???)
				     //0,
				     lpFont); // Nome del font

	if (hfont!=NULL) hfontOld = SelectObject(hdc, hfont);

	if (col2==-1) SetBkMode(hdc,TRANSPARENT); 
	              else 
				  {
				   SetBkMode(hdc,OPAQUE);
				   SetBkColor(hdc,col2);
				  }

	 SetTextColor(hdc,col1);
	 TextOut(hdc, x, y, lpString,strlen(lpString));

	 if (hfont!=NULL) 
	 {
		SelectObject(hdc, hfontOld);
		DeleteObject(hfont);
	 }

}

