//   /-------------------------------------------|
//   | imgPowerToy
//   |          
//   |                                           
//   |            by Ferrà Art & Technology 2004 
//   \-------------------------------------------|

#include "/easyhand/inc/easyhand.h"
#include <setjmp.h>
#include <math.h>
#include "/easyhand/ehtool/imgutil.h"
#include "/easyhand/ehtool/imgtool.h"
#include "/easyhand/ehtool/imgPowerToy.h"
#include <stdlib.h>						// by dam


static void _LIMGCopyRGB(RECT *prClip,
						 IMGHEADER *lpImghDst,
						 IMGHEADER *lpImghSrc,
						 INT iAlpha8);

static void _LIMGCopyRGB_ALPHA(RECT *prClip,
						       IMGHEADER *lpImghDst,
						       IMGHEADER *lpImghSrc,
						       INT iAlpha8);
static BOOL _LPointerClip(RECT *prClip,
				   IMGHEADER *lpImghDst,
				   IMGHEADER *lpImghSrc,
				   BYTE **ppDst,
				   BYTE **ppSrc);
 
// --------------------------------------------------------
// IMGCopy()
// Copia un immmagine in un altra
// ritorna FALSE = tutto ok
// 
// Giorgio/Ettore		Ferrà srl 2010
// --------------------------------------------------------
BOOL IMGCopy(INT hdlImageDst,	// La destinazione
			 INT hdlImageSrc,	// Il sorgente
			 POINT pArea,		    // La posizione e la dimensione
			 double dAlphaMain)
{
	IMGHEADER *lpImghDst;
	IMGHEADER *lpImghSrc;

	RECT rClip;
	INT iAlpha8;

	lpImghDst=(IMGHEADER *) memoLock(hdlImageDst);
	lpImghSrc=(IMGHEADER *) memoLock(hdlImageSrc);

	rClip.left=pArea.x; 
	rClip.right=pArea.x+lpImghSrc->bmiHeader.biWidth-1;
	rClip.top=pArea.y; 
	rClip.bottom=pArea.y+lpImghSrc->bmiHeader.biHeight-1;
	iAlpha8=(INT) (dAlphaMain*255/100); // Trasformo double in char (0/255)

	switch (lpImghDst->enPixelType)
	{
		case IMG_PIXEL_BGR:
		case IMG_PIXEL_RGB:
				_LIMGCopyRGB(&rClip,lpImghDst,lpImghSrc,iAlpha8);
				break;

		case IMG_PIXEL_RGB_ALPHA:
				_LIMGCopyRGB_ALPHA(&rClip,lpImghDst,lpImghSrc,iAlpha8);
				break;

		default:
			ehError(); // Destinazione in un formato non gestito
	}
	memoUnlock(hdlImageSrc);
	memoUnlock(hdlImageDst);
	return FALSE;
}

//
// _LPointerClip() 
// Controlla area di clip e ricalcola i puntatori
// Ritorna TRUE se si è al di fuori dell'area
//
static BOOL _LPointerClip(RECT *prClip,
				   IMGHEADER *lpImghDst,
				   IMGHEADER *lpImghSrc,
				   BYTE **ppDst,
				   BYTE **ppSrc)
{
	BYTE *lpDst=lpImghDst->pbImage;
	BYTE *lpSrc=lpImghSrc->pbImage;

	// Controllo generale sui limit
	if (prClip->left>=lpImghDst->bmiHeader.biWidth|| // Troppo a destra
		prClip->right<0|| // Troppo a sinistra
		prClip->top>=lpImghDst->bmiHeader.biHeight|| // Troppo sotto
		prClip->bottom<0) // Troppo sopra
		{	
			return TRUE;
		}
	else
	{
		if (prClip->left<0) {
			lpSrc+=-prClip->left*lpImghSrc->iChannels; prClip->left=0;
		} 

		if (prClip->top<0) {
			lpSrc+=-prClip->top*lpImghSrc->linesize; prClip->top=0;
		}

		if (prClip->right>=lpImghDst->bmiHeader.biWidth) {prClip->right=lpImghDst->bmiHeader.biWidth-1;}
		if (prClip->bottom>=lpImghDst->bmiHeader.biHeight) {prClip->bottom=lpImghDst->bmiHeader.biHeight-1;}
		lpDst+=(prClip->top*lpImghDst->linesize)+(prClip->left*lpImghDst->iChannels);

		*ppSrc=lpSrc;
		*ppDst=lpDst;
		return FALSE;
	}
}


//
// _LIMGCopyRGB()	Copia in una destinazione BGR
//
static void _LIMGCopyRGB(RECT *prClip,
						 IMGHEADER *lpImghDst,
						 IMGHEADER *lpImghSrc,
						 INT iAlpha8)
{
	int register x,y;
	int unsigned register r32;
	BYTE r,g,b;
	BYTE rs,gs,bs;
	DWORD *pdw32Source,*pdw32Dest;//,l32;
	BYTE *lpSrc;
	BYTE *lpDst;
	BYTE *lpd,*lps;
	BOOL bInverted=FALSE;
	INT iAlpha;

	if (iAlpha8<1) return;
	if (_LPointerClip(	prClip,
						lpImghDst,
						lpImghSrc,
						&lpDst,
						&lpSrc)) return;

	switch (lpImghSrc->enPixelType)
	{
		case IMG_PIXEL_RGB_ALPHA:

			if (lpImghDst->enPixelType==IMG_PIXEL_BGR) bInverted=TRUE;

			for (y=prClip->top; y<=prClip->bottom; y++)
			{
				pdw32Source=(DWORD *) lpSrc; lpd=lpDst;
				for (x=prClip->left;x<=prClip->right;x++,pdw32Source++,lpd+=3)
				{
					// Leggo il pixel con l'alpha
					r32=*pdw32Source; 
					iAlpha=(r32>>24)&0xFF; 
					if (iAlpha&&iAlpha8<255) iAlpha=iAlpha8*iAlpha/255;
					if (!iAlpha) continue; // Trasparenza totale (salto)
					
					//    
					// RGBA
					// x
					// Nota: quando avrò voglia (e tempo) di giocare, da fare in assembler !
					pdw32Dest=(DWORD *) lpd; 
					if (iAlpha==255) // Fusione Rapida (senza alpha)
					{
						if (!bInverted)
							*pdw32Dest=(*pdw32Dest&0xFF000000)|(r32&0xFFFFFF);//b|(g<<8)|(r<<16);
							else
							{
							 r=(BYTE) ((r32>>16)&0xFF); g=(BYTE) ((r32>>8)&0xFF); b=(BYTE) (r32&0xFF); 
							 *pdw32Dest=(*pdw32Dest&0xFF000000)|r|(g<<8)|(b<<16);
							}
					} 
					else 
					{
						//
						// Fuzione dei due colori
						//
						r=(BYTE) ((r32>>16)&0xFF); g=(BYTE) ((r32>>8)&0xFF); b=(BYTE) (r32&0xFF); // Sorgente (PNG)
						r32=*pdw32Dest;
						
						
						//bs=(BYTE) ((r32>>16)&0xFF); gs=(BYTE) ((r32>>8)&0xFF); rs=(BYTE) (r32&0xFF);  // JPG -- non corretto --
						rs=(BYTE) ((r32>>16)&0xFF); gs=(BYTE) ((r32>>8)&0xFF); bs=(BYTE) (r32&0xFF);  // JPG
						
						
						rs=rs+( (((INT) r- (INT) rs)*iAlpha) / 255 );
						gs=gs+( (((INT) g- (INT) gs)*iAlpha) / 255 );
						bs=bs+( (((INT) b- (INT) bs)*iAlpha) / 255 );


						// if (bInverted) -- non corretto ---
						if (!bInverted) 
							*pdw32Dest=(r32&0xFF000000)|(bs|(gs<<8)|(rs<<16)); 
							else
							*pdw32Dest=(r32&0xFF000000)|(rs|(gs<<8)|(bs<<16)); 
							
					}

				}
				lpSrc+=lpImghSrc->linesize;
				lpDst+=lpImghDst->linesize;
			}
			break;

		// -----------------------------------------------------
		// Fusione di un JPG
		//
		//case IMG_JPEG:
		//case IMG_GIF:
		case IMG_PIXEL_BGR:
		case IMG_PIXEL_RGB:

			if (lpImghSrc->enPixelType!=lpImghDst->enPixelType) bInverted=TRUE;

			//
			// Copia con inversione (DA VERIFICARE) <-- !!!
			//
			if (bInverted) 
			{
				if (iAlpha8>=255) // copia senza trasparenza
				{
					INT xSize=(prClip->right-prClip->left+1)*lpImghSrc->iChannels;

					for (y=prClip->top; y<=prClip->bottom; y++)
					{
						 ehMemCpy(lpDst,lpSrc,xSize);
						 lpSrc+=lpImghSrc->linesize; lpDst+=lpImghDst->linesize;
					}

					/*
					for (y=prClip->top; y<=prClip->bottom; y++) 
					{
						lps=lpSrc; lpd=lpDst;
						for (x=prClip->left;x<=prClip->right;x++) 
						{
							
							lps+=3;
							
							*lpd=*lps; lpd++; lps--;
							*lpd=*lps; lpd++; lps--;
							*lpd=*lps; lpd++; lps+=2;

						}
						lpSrc+=lpImghSrc->linesize;	lpDst+=lpImghDst->linesize;
					}*/
					

				}
				else 
				{
					for (y=prClip->top; y<=prClip->bottom; y++) 
					{
						lps=lpSrc; lpd=lpDst;
						for (x=prClip->left;x<=prClip->right;x++) 
						{
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;

							/*
							lps+=3;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps--;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps--;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps+=2;		
							*/

						}
						lpSrc+=lpImghSrc->linesize;	lpDst+=lpImghDst->linesize;
					}
				}
			}
			//
			// Copia SENZA INVERSIONE
			//
			else 
			{
				if (iAlpha8>=255) // copia senza trasparenza
				{
					INT xSize=(prClip->right-prClip->left+1)*lpImghSrc->iChannels;

					for (y=prClip->top; y<=prClip->bottom; y++)
					{
						 ehMemCpy(lpDst,lpSrc,xSize);
						 lpSrc+=lpImghSrc->linesize; lpDst+=lpImghDst->linesize;
					}
				}
				else // copia con trasparenza
				{
					for (y=prClip->top; y<=prClip->bottom; y++)
					{
						lps=lpSrc; lpd=lpDst;
						for (x=prClip->left;x<=prClip->right;x++) 
						{
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
						}
						lpSrc+=lpImghSrc->linesize;	lpDst+=lpImghDst->linesize;
					}
				}
			}
			break;

		default:
			ehError();
	}
}



//
// _LIMGCopyRGB_ALPHA()	
//  Copia in una destinazione IMG_PIXEL_RGB_ALPHA
//
static void _LIMGCopyRGB_ALPHA(RECT *prClip,
						       IMGHEADER *lpImghDst,
						       IMGHEADER *lpImghSrc,
						       INT iAlpha8)
{
	BYTE *lpSrc;
	BYTE *lpDst;
	BYTE *lpd,*lps;
	INT x,y;
	int register r32,*p32;
	BYTE r,g,b,rs,gs,bs;
	LONG *p32Source,*p32Dest;	

	if (iAlpha8<1) return;

	if (_LPointerClip(prClip,
				lpImghDst,
				lpImghSrc,
				&lpDst,
				&lpSrc)) return;

	switch (lpImghSrc->enPixelType)
	{
		case IMG_PIXEL_RGB_ALPHA:

			// copia senza trasparenza
				if (iAlpha8>=255) {

					INT xSize=(prClip->right-prClip->left+1)*lpImghSrc->iChannels;
					for (y=prClip->top; y<=prClip->bottom; y++) {
						 ehMemCpy(lpDst,lpSrc,xSize);
						 lpSrc+=lpImghSrc->linesize; lpDst+=lpImghDst->linesize;
					}

				}
				else // copia con trasparenza
				{
					for (y=prClip->top; y<=prClip->bottom; y++) 
					{
						lps=lpSrc; lpd=lpDst;
						for (x=prClip->left;x<=prClip->right;x++) 
						{
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
							*lpd=*lpd+(BYTE) (((*lps-*lpd)*iAlpha8)>>8); lpd++; lps++;
						}
						lpSrc+=lpImghSrc->linesize;	lpDst+=lpImghDst->linesize;
					}
				}
		
			break;

		case IMG_PIXEL_BGR:
		
			if (iAlpha8>=255) // copia senza trasparenza
			{
				for (y=prClip->top; y<=prClip->bottom; y++) 
				{
					lps=lpSrc; p32=(int *) lpDst;
					for (x=prClip->left;x<=prClip->right;x++,lps+=3,p32++) // dest. con alfa (4b) - sorg. no alfa (3b)
					{
						r32=* (DWORD *) lps;
						*p32=(0xFF000000|		// Opaco
							 ((r32>>16)&0xFF)|	// Red
							 ((r32>>8)&0xFF)<<8|// Green
							 (r32&0xFF)<<16);	// Blu			
					}
					lpSrc+=lpImghSrc->linesize;	
					lpDst+=lpImghDst->linesize;
				}
			}
			else // copia con trasparenza
			{
				for (y=prClip->top; y<=prClip->bottom; y++) 
				{
					lpd=lpDst; 
					lps=lpSrc; 
					for (x=prClip->left;x<=prClip->right;x++,lps+=3,lpd+=4) // dest. con alfa (4b) - sorg. no alfa (3b)
					{
						p32Source=(DWORD *) lps; 
						p32Dest=(DWORD *) lpd; 

						r32=*p32Source;
						r=(BYTE) ((r32>>16)&0xFF); g=(BYTE) ((r32>>8)&0xFF); b=(BYTE) (r32&0xFF); 

						r32=*p32Dest;
						bs=(BYTE) ((r32>>16)&0xFF); gs=(BYTE) ((r32>>8)&0xFF); rs=(BYTE) (r32&0xFF); // <-- da verificare
						bs=(bs+(BYTE) ((b-bs)*iAlpha8/255))&0xFF;
						gs=(gs+(BYTE) ((g-gs)*iAlpha8/255))&0xFF;
						rs=(rs+(BYTE) ((r-rs)*iAlpha8/255))&0xFF;

						//*p32Dest=(r32&0xFF000000)|(rs|(gs<<8)|(bs<<16)); // abbiamo deciso di lasciare l' alfa della dest
						*p32Dest=(0xFF000000)|(rs|(gs<<8)|(bs<<16)); 
					}
					lpSrc+=lpImghSrc->linesize;	lpDst+=lpImghDst->linesize;
				}
			}

			break;
		
		case IMG_PIXEL_RGB:

			if (iAlpha8>=255) // copia senza trasparenza
			{
				for (y=prClip->top; y<=prClip->bottom; y++) 
				{
					lpd=lpDst; 
					lps=lpSrc; 
					for (x=prClip->left;x<=prClip->right;x++,lps+=3,lpd+=4) // dest. con alfa (4b) - sorg. no alfa (3b)
					{
						p32Source=(DWORD *) lps;  p32Dest=(DWORD *) lpd; 
						*p32Dest=(0xFF000000)|(*p32Source);						
					}
					lpSrc+=lpImghSrc->linesize;	lpDst+=lpImghDst->linesize;
				}
			}
			else // copia con trasparenza
			{
				for (y=prClip->top; y<=prClip->bottom; y++) 
				{
					lpd=lpDst; 
					lps=lpSrc; 
					for (x=prClip->left;x<=prClip->right;x++,lps+=3,lpd+=4) // dest. con alfa (4b) - sorg. no alfa (3b)
					{
						p32Source=(DWORD *) lps; 
						p32Dest=(DWORD *) lpd; 

						r32=*p32Source;
						r=(BYTE) ((r32>>16)&0xFF); g=(BYTE) ((r32>>8)&0xFF); b=(BYTE) (r32&0xFF); 

						r32=*p32Dest;
						//bs=(BYTE) ((r32>>16)&0xFF); gs=(BYTE) ((r32>>8)&0xFF); rs=(BYTE) (r32&0xFF); // <-- da verificare
						
						rs=(BYTE) ((r32>>16)&0xFF); gs=(BYTE) ((r32>>8)&0xFF); bs=(BYTE) (r32&0xFF);   // <-- da verificare

						bs=(bs+(BYTE) ((b-bs)*iAlpha8/255))&0xFF;
						gs=(gs+(BYTE) ((g-gs)*iAlpha8/255))&0xFF;
						rs=(rs+(BYTE) ((r-rs)*iAlpha8/255))&0xFF;

						//*p32Dest=(r32&0xFF000000)|((rs<<16)|(gs<<8)|bs); // abbiamo deciso di lasciare l' alfa della dest
						*p32Dest=(0xFF000000)|((rs<<16)|(gs<<8)|bs);														
					}
					lpSrc+=lpImghSrc->linesize;	lpDst+=lpImghDst->linesize;
				}
			}
			break;

		default:
			ehError();
	}
}



INT ColorConvert(CHAR *lpColore)
{
	CHAR szServ[200];
	INT iColor[3];
	if (*lpColore=='#')
	{
		strncpy(szServ,lpColore+1,2); szServ[2]=0; _strupr(szServ); iColor[0]=xtoi(szServ); 
		strncpy(szServ,lpColore+3,2); szServ[2]=0; _strupr(szServ); iColor[1]=xtoi(szServ);
		strncpy(szServ,lpColore+5,2); szServ[2]=0; _strupr(szServ); iColor[2]=xtoi(szServ);
		//iColor=RGB(iColor[0],iColor[1],iColor[2]);
		return RGB(iColor[0],iColor[1],iColor[2]);
	}

	if (*lpColore>='0'&&*lpColore<='9') return atoi(lpColore);

	_strlwr(lpColore);
	if (!strcmp(lpColore,"white"))  return RGB(255,255,255);
	if (!strcmp(lpColore,"black"))  return RGB(0,0,0);
	if (!strcmp(lpColore,"gray"))   return RGB(127,127,127);
	if (!strcmp(lpColore,"red"))    return RGB(255,0,0);
	if (!strcmp(lpColore,"blue"))   return RGB(0,0,255);
	if (!strcmp(lpColore,"green"))  return RGB(0,255,0);
	if (!strcmp(lpColore,"yellow")) return RGB(255,255,0);
	if (!strcmp(lpColore,"orange")) return RGB(255,127,0);
	if (!strcmp(lpColore,"t")) return -1;
	return 0;
}
/*
INT AlignConvert(CHAR *lpColore)
{
	strlwr(lpColore);
	if (!strcmp(lpColore,"left")||!strcmp(lpColore,"l")) return LR_LEFT;
	if (!strcmp(lpColore,"right")||!strcmp(lpColore,"r")) return LR_RIGHT;
	if (!strcmp(lpColore,"center")||!strcmp(lpColore,"c")) return LR_CENTER;
	if (!strcmp(lpColore,"top")||!strcmp(lpColore,"t")) return LR_TOP;
	if (!strcmp(lpColore,"bottom")||!strcmp(lpColore,"b")) return LR_BOTTOM;
	return LR_LEFT;
}
*/


static void LAutoLevelMaker(INT iLevel[],INT *lpiLevelInMin,INT *lpiLevelInMax)
{
	INT a,iTot;
	double dMax;
	double dPerc,dCy;
	double dBlackThresold=6; // Era 4
	double dDelayBlack=(256/2);
	double dWhiteThresold=7; // Era 4
	double dDelayWhite=(256/2);
	double dLim;
	dMax=0;
	iTot=0;
	for (a=0;a<256;a++)
	{
		if (iLevel[a]>dMax) dMax=iLevel[a];
		iTot+=iLevel[a];
	}
	
	dPerc=dMax*100/iTot;
	//dComp=32;
	if (dPerc>2) dMax=(dMax/dPerc);

	// Livello minimo
	dLim=dBlackThresold;
	for (a=0;a<240;a++)
	{
	  if (iLevel[a]) dCy=(double) iLevel[a]*100/dMax; else dCy=0;
	  if (dDelayBlack) dLim=dBlackThresold-(dBlackThresold*a/dDelayBlack); //if (dLim<1) dLim=1;

	  if (dCy>dLim) {*lpiLevelInMin=a; break;}
	}
	
	// Livello massimo
	dLim=dWhiteThresold;
	for (a=255;a>80;a--)
	{
	  if (iLevel[a]) dCy=(double) iLevel[a]*100/dMax; else dCy=0;
	  if (dDelayWhite) dLim=dWhiteThresold-(dWhiteThresold*(255-a)/dDelayWhite); //if (dLim<1) dLim=1;

	  if (dCy>dLim) {*lpiLevelInMax=a; break;}
	}
}

INT IMGAutoLevel(INT hdlImage)
{
	INT ariLevels[256];
	INT iLevelInMin,iLevelInMax;
	double dGamma=1;
	INT iLevelOutMin=0;
	INT iLevelOutMax=255;

	IMGLevelMaker(hdlImage,ariLevels); // Cerco i livelli della foto
	LAutoLevelMaker(ariLevels,&iLevelInMin,&iLevelInMax); // Cerco il minimo / massimo
	hdlImage=IMGLevel(hdlImage,iLevelInMin,dGamma,iLevelInMax,iLevelOutMin,iLevelOutMax,TRUE); // Livello la foto
	return hdlImage;
}


// --------------------------------------------------------------
// IMGFactCalc()
// SIZE lpsSource  Dimensioni originali del sorgente
// SIZE lpsDest    Area massima in cui può essere stampato
// Ritorna scale ottimale di carimamento da passare JPGReadFileEx
// --------------------------------------------------------------


INT IMGFactCalcH(IMGHEADER *ImgHead,SIZE *lpsDest,INT *lpiPerc)
{
	SIZE sSource;
	sSource.cx=ImgHead->bmiHeader.biWidth;
	sSource.cy=ImgHead->bmiHeader.biHeight;
	return IMGFactCalc(&sSource,lpsDest,lpiPerc);
}

INT IMGFactCalc(SIZE *lpsSource,SIZE *lpsDest,INT *lpiPerc)
{
	INT iScale,iPerc;
	iScale=1;
	//if (!lpsSource->cx||lpsSource->cy) ehExit("IMGFactCalc=0");
	
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
		if ((lpsSource->cy*iPerc/100)>lpsDest->cy||
			(lpsSource->cx*iPerc/100)>lpsDest->cx)
		{
			iPerc--;
		} else break;
	}

	//win_infoarg("%d,%d iPerc %d",lpsSource->cx,lpsDest->cx,iPerc);

	//dispx("[%d]",iPerc); ehSleep(1000);
	if (lpiPerc) *lpiPerc=iPerc;

	if (iPerc<100&&iPerc>1)
	{
	// 100% 1
	// 50%  2
	// 25%  4
	// 10%  5
		iScale=100/iPerc; 
		//iFact>>=1; // 50%
		//iScale&=~1; // Base 2
	} else iScale=1;
	if (iScale<1) iScale=1;
	//dispxEx(0,20,"[%d ---> %d]=%d               ",lpsSource->cx,lpsDest->cx,iScale);
	return iScale;
}


BOOL JPGReadSize(TCHAR *imageFileName,INT *HdlImage,BOOL *lpfStop,INT *lpiErr,INT iModeError,SIZE * psDest)
{
	IMGHEADER sImg;
	SIZE sNewDest;
	RECT rDest,rSource;
	INT iFact;
	SIZE sSource;
	INT hdlOriginal;
	INT hdlBuild;
	BOOL fError=TRUE;

	if (!JPGReadHeader(imageFileName,&sImg,JME_NORMAL)) return fError;
	
	sSource.cx=sImg.bmiHeader.biWidth;
	sSource.cy=sImg.bmiHeader.biHeight;

	// Calcolo le dimensione della nuova foto
	IMGCalcSize(&sImg,      // Dimensioni del sorgente
				psDest,		// Area disponibile
				NULL,
				NULL,
				IMGPT_AL_FORMATO,  // Tipo di adattamento
				0,	   // Allineamento orizzontale
				0,	   // Allineamento verticale
				&sNewDest,		   // Dimensioni della destinazione
				&rDest,
				&rSource); 	   // Posizionamento in destinazione

	// Calcolo il fattore di loading
	iFact=IMGFactCalc(&sSource,&sNewDest,NULL); // <-- Percentuale di caricamento (in ritorno)

	// Carico la foto
	JPGReadFileEx(imageFileName,
				  &hdlOriginal,
				  lpfStop,
				  iFact,
				  JDCT_IFAST,
				  NULL,
				  iModeError,
				  IMG_PIXEL_BGR);

	if (lpfStop) {if (*lpfStop) return fError;}
	
	hdlBuild=IMGResampling(hdlOriginal,&rSource,sNewDest.cx,sNewDest.cy,TRS_LANCZOS);
	memoFree(hdlOriginal,"foto");
	fError=FALSE;
	*HdlImage=hdlBuild;
	
	return fError;
}