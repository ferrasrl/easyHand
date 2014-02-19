//-----------------------------------------------------------------------
// PowerPrinter	   Gestione di un documento in stampa	
//				   Ex LptReportImg
//											                            |
//											                            |
//	Principio di funzionamento 									        |
//	HEADER                  				                            |
//	1) WS_OPEN Apro un Documento
//	   Passo in *ptr un puntatore ad una struttura EH_PD               |
//	   che contiene i parametri di inizializzazione della stampa        |
//	   (Tipo del Layout,titolo,sottotitolo,margini,font da usare,ecc..) |
//											                            |
//	2) WS_ADD,WS_SETFLAG Dichiaro il formato del campo del report       |
//	   Passo in *ptr un puntatore ad una struttura LRFIELDS             |
//	   che contiene la formattazione ed il nome del campo               |
//	   WS_SETFLAG aiuta a presettare i valori di un tipo ALFA o NUME    |
//											                            |
//	3) WS_DO Fine dichiarazione campi                                   |
//	   Segnala la fine della dichiarazione dei campi                    |
//	   PowerPrinter calcola le dimensioni in "dot" dei vari campi       |
//	   e linee
//											                            |
//	BODY									                            |
//	4) WS_LINK    Inizio assegnazione valore campi di una linea         |
//	5) WS_REALSET Assegnazione valore di un campo 						|
//	6) WS_INSERT  Fine assegnazione ed inserimento della linea          |
//											                            |
//	END                                                                 |
//	7) WS_CLOSE     						                            |
//	   Preview e/o stampa del report                                    |
//											                            |
//											                            |
//											                            |
//										 Creato da G.Tassistro          |
//										 Ferrà Art & Technology 1999    |
//										 Ferrà Art & Technology 2000    |
//										 Ferrà Art & Technology 2001    |
//	Versione 2.1                    	 Ferrà Art & Technology 2002    |
//	Versione 2.2                    	 Ferrà Art & Technology 2003    
//	Versione 3                     		 Ferrà srl 2007
//	Versione 3.2 e cambio di nome  		 Ferrà srl 2008
//-----------------------------------------------------------------------

/*
	Istruzioni
	a) Sostituire L Report.h con PowerPrinter.h (in tutto il progetto)
	b) Sostituire L Report( con PowerPrinter(



*/

/*
   lpszFilename :=
'C:\WINDOWS\system32\spool\DRIVERS\COLOR\Photoshop4DefaultCMYK.icc';

   hDC_ := GetPrinterDC();
   if hDC_ = 0 then
       RaiseLastOSError;

   // Fill in the DOCINFO structure
   diDocInfo.cbSize := sizeof(DOCINFO);
   diDocInfo.lpszDocName := 'sdft';
   diDocInfo.lpszOutput := nil;
   diDocInfo.lpszDatatype := nil;
   diDocInfo.fwType := 0;

   // Start the document
   if StartDoc(hDC_, diDocInfo) = SP_ERROR then
       RaiseLastOSError;

   // Start the page
   if StartPage(hDC_) = SP_ERROR then
       RaiseLastOSError;

   // I tested although ICM_ON
   if SetICMMode(hDC_, ICM_DONE_OUTSIDE_DC) = 0 then
       RaiseLastOSError;

   if not SetICMProfile(hdc_, lpszFilename) then
       RaiseLastOSError;
*/

//#define WINVER 0x400

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/imgutil.h"
#include "/easyhand/ehtool/PowerPrinter.h"
// #include "/easyhand/ehtool/main/armaker.h"
//#include "/easyhand/Ehtoolx/libharu-2.1.0/include/hpdf.h" // aggiunto per i pdf

#ifndef ICM_DONE_OUTSIDE_DC
#define ICM_DONE_OUTSIDE_DC 4
#endif

// new 2009

static struct {
	
	BOOL	bReady;
	EH_PD	*psDi;
	CHAR	szTempPath[500];
	CHAR	szTempFile[500];
	FILE *	chTempFile;
	//INT		hmField;//=-1;
	//BOOL	fArray;
	EH_LST	lstFld;

	SIZE	sizVideoPage;
	RECT	rVideoPrintArea;
	SIZE	sVideoPrintSize;

} sPd={0};

typedef enum {

	LRE_ENDFILE,

//#define LRE_TEXT      1
	LRE_CHAR,//=2,
	LRE_CHAREX,//=3,
	LRE_BOX,//=4,
	LRE_BOXP,//=5,
	LRE_BOXR,//=11,
	LRE_BOXPR,//=12,
	LRE_LINE,//=9,
	LRE_CHARBOX,//=10, // New 2002 Testo in box

	LRE_EHIMG,//=7,
	LRE_EHIMGD,//=8, // Stampa DIRETTA senza strech
	LRE_IMAGELIST,//=14,  // New 2008: collegato a imagelist
	LRE_BITMAP // Bitmap Windows

//#define LRE_CHARJUST  8


} EN_PPTE; // PowerPrinterTypeElement


typedef struct {

	BOOL  HideX;
	BOOL  HideY;

	INT  AreaY;
	INT  AreaX;

	INT  CharY;
	INT  CharX;
	LONG  Lines;
	LONG  LineVideoX;
	LONG  LineVideoY;

	INT iVScrollPos;
	INT iVScrollMax;
	INT iHScrollPos;
	INT iHScrollMax;

} TLAYOUT;

//-----------------------------------------------------------------------
//	Stampa di un report                                                 |
//-----------------------------------------------------------------------

static void LREFileLoad(void);
static void LREFileFreeResource(BOOL bDeleteFile);
static void LRETagReplace(void);
static BYTE * LREGetNext(BYTE *ptr);
static INT LRToStyles(void);


static BOOL LREPreview(CHAR *);
static BOOL LREPrintDirect(CHAR *lpParam);

// p:300=x:sPd.psDi->xInchDot
/*
#define LocInchToDotX(p) (INT) (p*sPd.psDi->xInchDot/300.00)
#define LocInchToDotY(p) (INT) (p*sPd.psDi->yInchDot/300.00)
*/
#define DotToVideoX(p) sPd.rVideoPrintArea.left+VideoOffset.x+(sPd.sVideoPrintSize.cx*p/sPd.psDi->sPaper.cx)
#define DotToVideoY(p) sPd.rVideoPrintArea.top+VideoOffset.y+(sPd.sVideoPrintSize.cy*p/sPd.psDi->sPaper.cy)
#define DotToPixelX(p) (sPd.sVideoPrintSize.cx*p/sPd.psDi->sPaper.cx)
#define DotToPixelY(p) (sPd.sVideoPrintSize.cy*p/sPd.psDi->sPaper.cy)

// 3,95 pollici sono 10cm
// ne consegue che 10:3,95
// 1cm= 0,395
INT LRInchToDotX(INT d) {return (INT ) ((double) d*sPd.psDi->xInchDot/300.00);}
INT LRInchToDotY(INT p) {return (INT ) ((double) p*sPd.psDi->yInchDot/300.00);}

//#define LRCmToDotX(p) (INT) (p*.395*sPd.psDi->xInchDot/2.54)
// p:2.54=x:sPd.psDi->xInchDot
INT  LRCmToDotX(double p) {return (INT) (p*sPd.psDi->xInchDot/2.54);}
INT  LRCmToDotY(double p) {return (INT) (p*sPd.psDi->yInchDot/2.54);}
// p:72=x:sPd.psDi->xInchDot
INT  LRPtToDotX(INT p) {return (INT) ((double) p*sPd.psDi->xInchDot/72.00);}
INT  LRPtToDotY(INT p) {return (INT) ((double) p*sPd.psDi->yInchDot/72.00);}

// Formula misura:sPd.psDi->yInchDot=x:300
// Converte da 1/300 di Pollice a dot
INT LRDotToInchX(INT p) {return (INT) (p*300/sPd.psDi->xInchDot);}
INT LRDotToInchY(INT p) {return (INT) (p*300/sPd.psDi->yInchDot);}
INT LRCenterY(RECT r,INT b) {return (INT) (((r.bottom-r.top+1)-b)>>1);}
INT LRCenterX(RECT r,INT b) {return (INT) (((r.right-r.left+1)-b)>>1);}



static WCHAR *LRDecode(BYTE *pText)
{
	WCHAR *pwText;
	switch (sPd.psDi->iCharEncode)
	{
		case SE_UTF8:
			pwText=strDecode(pText,SE_UTF8,NULL);
			break;

		default:
			if (sys.bOemString) 
			{
				CHAR *pBuf=strDup(pText);
				OemToChar(pText,pBuf); 
				pwText=strToWcs(pBuf);
				ehFree(pBuf);
			}
			else 
			pwText=strToWcs(pText);
			break;
	}
	return pwText;
}	

// -------------------------------------------------
// Giustificazione di un testo in un RECT
// New 2001
// by Vadora/Tassistro
// Ritorna l'altezza del testo effettivo
// -------------------------------------------------
/*
static INT LTextJustify(HDC hdc,CHAR *pText,RECT *prc,INT iInterLinea,BOOL fOutput)
{
	INT  xStart, yStart, iBreakCount ;
	PSTR pBegin, pEnd ;
	SIZE size ;
	BOOL bGiustifico;

	yStart = prc->top ;
	do                            // for each text line
	{
		iBreakCount=0;
		while (*pText == ' ') pText++;  // skip over leading blanks (Salta i blank in testa)
		pBegin=pText;
		do                       // until the line is known / (Trova la linea piu lunga che sta nel Rect)
		{
			pEnd=pText;
			while (*pText != '\0' && *pText++ != ' ');
			// for each space, calculate extents
			iBreakCount++;
			SetTextJustification(hdc, 0, 0) ;
			GetTextExtentPoint32(hdc, pBegin, pText - pBegin - 1, &size) ;
			//if (*pText == '\0') break;
			if (!*pText) break;
		} while ((INT) size.cx < (prc->right - prc->left)) ;
		iBreakCount-- ;
		while (*(pEnd - 1) == ' ')   // eliminate trailing blanks (Elimina i blank in coda)
		{
			pEnd-- ;
			iBreakCount-- ;
		}
		// Se la linea non è l'ultima giustifico
		bGiustifico=TRUE;
		if ((!*pText && (INT) size.cx < (prc->right - prc->left)) || iBreakCount <= 0)
		{
			pEnd=pText;
			bGiustifico=FALSE;
		}
		SetTextJustification(hdc, 0, 0) ;
		GetTextExtentPoint32(hdc, pBegin, pEnd - pBegin, &size) ;
		if (bGiustifico) SetTextJustification(hdc,prc->right - prc->left - size.cx,iBreakCount) ;
		xStart = prc->left ;
		if (fOutput) TextOut(hdc, xStart, yStart, pBegin, pEnd - pBegin) ;
		yStart+= size.cy + iInterLinea;
		pText= pEnd;
	
	} while (*pText && yStart < prc->bottom);
	SetTextJustification(hdc, 0, 0) ;
	return (yStart-prc->top);
}
*/
static INT LTextInBox(HDC hdc,LRT_CHARBOX *LCharBox,CHAR *pbText,BOOL fOutput,INT *lpiRows,INT iMaxRows)
{
	INT  xStart=0, yStart=0, iBreakCount=0;
	//PSTR pBegin, pEnd ;
	WCHAR *pwBegin,*pwEnd;
	SIZE size ;
	BOOL bJustify;
	BOOL bCRLF;
	INT iRowsCount=0;
	INT iStrLen;
	//BYTE *pMemory,*pText;
	WCHAR *pwText,*pwMemory;

	// Regole
	// A) La linea và troncata con CR o LF
	// B) CR e LF devono essere tolti
	// C) Se la linea finisce con CR (ed è piu corta dello spazio, NON VIENE GIUSTIFICATA)
	//
	pwMemory=pwText=LRDecode(pbText);

	//pMemory=pText=strDup(pTextOriginal);
	wcsTrim(pwText);
	while (wcsReplace(pwText,LCRLF,L"\r"));
	while (wcsReplace(pwText,L"\n\r",L"\r"));

	yStart = LCharBox->rArea.top ;
	do                            // for each text line
	{
		iBreakCount=0;
	//	while (*pText==' ') pText++;  // skip over leading blanks (Salta i blank in testa)
		pwBegin=pwText;
		
		// Trova la linea piu lunga che sta nel Rect
		// iBreakCount,contiene gli spazi di separazione delle parole
		bCRLF=FALSE;
		do                       // until the line is known / 
		{
			pwEnd=pwText;
			// Cerca uno spazio (Punto di interruzione possibile
			//while (*pText!='\0'&&*pText++!=' '); // Era così
			for (;*pwText;pwText++)
			{
				if (*pwText==L' ') {pwText++; break;}
				if (*pwText==L'\n'||*pwText==L'\r') {pwEnd=pwText; bCRLF=TRUE; break;}
			}

			// Fino alla fine o allo spazio
			// Avanzo; Fino alla fine della stringa o al primo spazio o al primo ritorno a capo
			// for each space, calculate extents
			iBreakCount++;
			SetTextJustification(hdc,0,0);
//			GetTextExtentPoint32(hdc, pBegin, pText - pBegin - 1, &size) ;
			GetTextExtentPoint32W(hdc, pwBegin, pwText - pwBegin, &size) ;
			//if (*pText=='\n'||*pText=='\r') break;			
			if (!*pwText) break;
			if (bCRLF) break; // E' un fine linea mi fermo
		} while ((INT) size.cx < (LCharBox->rArea.right - LCharBox->rArea.left)) ;
		iBreakCount--; 

		while (*(pwEnd - 2)==L' ')   // eliminate trailing blanks (Elimina i blank in coda)
		{
			pwEnd--; iBreakCount--;
		}
		
		// Elimina il cr/lf
		//if (bCRLF) while (*pEnd=='\n'||*pEnd=='\r') {pEnd--;}

		//win_infoarg("[%s]",pBegin);

		// Controllo se è l'ultima linea
		bJustify=TRUE;
		if ((!*pwText && (INT) size.cx < (LCharBox->rArea.right - LCharBox->rArea.left)) || iBreakCount <= 0)
		{
			pwEnd=pwText;
			if (!LCharBox->bJustifyLast) bJustify=FALSE; // new 2007
		}
		// Controllo se finisce con un ritorno a capo
		if (bCRLF) bJustify=FALSE; // E' un fine linea mi fermo
		
		iStrLen=pwEnd-pwBegin;
		if (iStrLen>0) iRowsCount++;
		if (iMaxRows&&iRowsCount>=iMaxRows) break; // new 2004

		// Stampa dell'oggetto
		if (fOutput) 
		{
			 SetTextJustification(hdc, 0, 0) ;
			 GetTextExtentPoint32W(hdc, pwBegin, iStrLen, &size) ;
			 if (bJustify&&LCharBox->xAllinea==LR_JUSTIFY) SetTextJustification(hdc,LCharBox->rArea.right - LCharBox->rArea.left - size.cx,iBreakCount) ;
			 switch (LCharBox->xAllinea)
			 {
				case LR_JUSTIFY:
				case LR_LEFT: 
					xStart = LCharBox->rArea.left; 
					break;

				case LR_RIGHT: 
					xStart = LCharBox->rArea.right-size.cx;  // ###
					break;

				case LR_CENTER: 
					xStart = LCharBox->rArea.left+((LCharBox->rArea.right-LCharBox->rArea.left)>>1)-(size.cx>>1); 
					break;
			 }
			 TextOutW(hdc, xStart, yStart, pwBegin, iStrLen) ;
		}
	
		yStart+= size.cy + LCharBox->yInterlinea;
		// Se c'è un CR o LF avanzo
		while (*pwEnd==L'\n'||*pwEnd==L'\r') {pwEnd++;}
		pwText= pwEnd;

	} while (*pwText);// && yStart < LCharBox->rArea.bottom);

	ehFree(pwMemory);
	SetTextJustification(hdc, 0, 0) ;
	if (lpiRows) *lpiRows=iRowsCount;
	return (yStart-LCharBox->rArea.top);
}

//-------------------------------------------
//| LREMakeItem                             |
//| Aggiunge un Item di stampa nel file     |
//|                                         |
//| ATENZIONE:Va liberata la memoria        |
//|                                         |
//|                           by Ferrà 2002 |
//-------------------------------------------
static void *LREMakeItem(EN_PPTE uType,void *lpStruct,INT iLenStruct,void *lpDati,INT iLenDati,INT *lpiSize)
{
    LRETAG PT;
	BYTE *lpMemo=NULL;
	BYTE *lpPoint;
	INT iMemoSize;
	
	PT.iLenTag=0;
	PT.iPage=sPd.psDi->iPageCount;
    
    PT.iType=uType;
    PT.iOfsDynamics=iLenStruct;
    PT.iLenDynamics=iLenDati;

	switch(uType)
	{
		case LRE_BOX: 
		case LRE_BOXP: 
		case LRE_BOXR: 
		case LRE_BOXPR: 
		case LRE_EHIMG: 
		case LRE_EHIMGD: 
		case LRE_LINE: 
		case LRE_IMAGELIST: 
		case LRE_BITMAP: 
			PT.iLenTag=iLenStruct+iLenDati;
			break;
	   
		case LRE_CHAR: 
		case LRE_CHAREX: 
		//case LRE_CHARJUST: 
		case LRE_CHARBOX:
			iLenDati=strlen((CHAR *) lpDati)+1;
			PT.iLenTag=iLenStruct+iLenDati;
			break;

		case LRE_ENDFILE: 
		    PT.iLenTag=0;
			break;

		default:
		   ehExit("LRE:???");
	}

	// LRETAG+(STRUCT)+(DATI) 
	iMemoSize=sizeof(PT)+iLenStruct+iLenDati;
	lpPoint=lpMemo=ehAlloc(iMemoSize); if (lpMemo==NULL) ehExit("LREMAkeItem: out of memory");

	//    f_put(ch,NOSEEK,&PT,sizeof(PT)); 
	memcpy(lpPoint,&PT,sizeof(PT)); lpPoint+=sizeof(PT);

	if (lpStruct) {memcpy(lpPoint,lpStruct,iLenStruct); lpPoint+=iLenStruct;} //f_put(ch,NOSEEK,lpStruct,iLenStruct); 

	// Se ci sono accoda i dati dinamici
	switch(uType)
	{
		case LRE_ENDFILE:
			break;

		case LRE_CHAR: 
		case LRE_CHAREX: 
		//case LRE_CHARJUST: 
		case LRE_CHARBOX:
	    default:
			if (iLenDati) //f_put(ch,NOSEEK,lpDati,strlen((CHAR *) lpDati)+1);
				{memcpy(lpPoint,lpDati,iLenDati); lpPoint+=iLenDati;}
			break;
	}

	*lpiSize=iMemoSize;
	return lpMemo;
}

// Scrive l'idem su disco
static BOOL LREAddItem(UINT uType,void *lpStruct,INT iLenStruct,void *lpDati,INT iLenDati)
{
	INT iSize;
	BYTE *lpResult;
	lpResult=LREMakeItem(uType,lpStruct,iLenStruct,lpDati,iLenDati,&iSize);
	fwrite(lpResult,iSize,1,sPd.chTempFile);
	ehFree(lpResult);
	return FALSE;
}

// ------------------------------------------------
// Gestione di un buffer di Linea
// Progettato nel 01/2002 per permettere la traslazione 
// di una linea dinamica sulla pagina successiva
// ------------------------------------------------

static BYTE *lpItemBuffer=NULL;
static LONG lItemBufferSize=0;
static LONG lItemBufferCount=0;

static BOOL LREAddLineBuffer(UINT uType,void *lpStruct,INT iLenStruct,void *lpDati,INT iLenDati)
{
	INT iSize;
	BYTE *lpResult;
	lpResult=LREMakeItem(uType,lpStruct,iLenStruct,lpDati,iLenDati,&iSize);
	
	// Se è la prima volta
	if (lItemBufferSize==0) 
		{lItemBufferSize=2048;  // Buffer iniziale 2k
		 lItemBufferCount=0;
		 lpItemBuffer=ehAlloc(lItemBufferSize);
		 if (lpItemBuffer==NULL) ehExit("LREAddLineBuffer: out of memory A");
		}
	
	// Se andiamo fuori dal buffer
	if ((lItemBufferCount+iSize)>=lItemBufferSize)
	{
		lItemBufferSize=lItemBufferCount+iSize+128;
		lpItemBuffer=realloc(lpItemBuffer,lItemBufferSize);
		if (lpItemBuffer==NULL) ehExit("LREAddLineBuffer: out of memory B");
	}
	
	memcpy(lpItemBuffer+lItemBufferCount,lpResult,iSize);
	lItemBufferCount+=iSize;
	ehFree(lpResult);
	return FALSE;
}

static void LREFlushBuffer(BOOL fOutput)
{
	if (lpItemBuffer==NULL) return;
	if (fOutput) {fwrite(lpItemBuffer,lItemBufferCount,1,sPd.chTempFile);}
	ehFree(lpItemBuffer);
	lpItemBuffer=NULL;
	lItemBufferSize=0;
}

static void LREBufferRemap(void)
{
  BYTE *pt=lpItemBuffer; 
  LRETAG *Lre;
  LRT_CHAR	  *lpLrtChar;
  LRT_CHAREX  *lpLrtCharEx;
  LRT_CHARBOX *lpLrtCharBox;

  if (pt==NULL) ehExit("LREBufferRemap:Null");
  
  while (pt<(lpItemBuffer+lItemBufferCount))
  {
	Lre=(LRETAG *) pt;

	Lre->iPage=sPd.psDi->iPageCount; // Setta la pagina attuale
	lpLrtChar=(LRT_CHAR *) (pt+sizeof(LRETAG)); 
	switch(Lre->iType)
	{
	/*
		case LRE_BOX: 
		case LRE_BOXP: 
		case LRE_EHIMG: 
		case LRE_LINE: 
			PT.iLenTag=iLenStruct+iLenDati;
			break;
	  */ 
		case LRE_CHAR:    
			lpLrtChar->Point.y=sPd.psDi->iRowOffset+sPd.psDi->rFieldPadding.top;
			break;
		case LRE_CHAREX:  
			lpLrtCharEx=(LRT_CHAREX *) lpLrtChar; 
			lpLrtCharEx->Point.y=sPd.psDi->iRowOffset+sPd.psDi->rFieldPadding.top;
			break;
		case LRE_CHARBOX: 
			lpLrtCharBox=(LRT_CHARBOX *) lpLrtChar; 
			lpLrtCharBox->rArea.top=sPd.psDi->iRowOffset+sPd.psDi->rFieldPadding.top;
			break;

		default:
		   ehExit("LRE:???");
	}


   pt+=sizeof(LRETAG)+Lre->iLenTag;
  }
 
}


// Crea la stringa relativa alla pagina
//static CHAR *GiveLayPag(EH_PD *sPd.psDi)
static CHAR *GiveLayPag(void)
{
	static CHAR Buf[40];
	*Buf=0;
	switch (sPd.psDi->iPagStyle)
	{
		case 0: sprintf(Buf,"Pag. %d",sPd.psDi->iPageCount);
				break;
		case 1: sprintf(Buf,"%d",sPd.psDi->iPageCount);
				break;
		case 2: sprintf(Buf,"%03d",sPd.psDi->iPageCount);
				break;
	}
	return Buf;
}


// -----------------------------------------------------------------------------------------------------------------
// LAYOUT della Pagina
// -----------------------------------------------------------------------------------------------------------------
static void LptReportLayout(void)
{
	LRT_CHAREX LRTCharEx;
	LRT_CHAR   LRTChar;
	LRT_BOX    LRTBox;
	INT a;

	switch (sPd.psDi->iLayStyle)
	{
		case LR_LAYHIDE: // Non header
				break;

		case LR_LAYTYPE0:
			
			// -------------------------------------------------------
			// LayOut Tipo 0 : Standard DOS
			// -------------------------------------------------------
			// DATA: Scrivo la data di oggi in alto a destra
			// -------------------------------------------------------
			  if (sPd.psDi->fDate)
			  {
				  memset(&LRTCharEx,0,sizeof(LRTCharEx));
				  LRTCharEx.Point.x=sPd.psDi->rPage.left;
				  LRTCharEx.Point.y=sPd.psDi->rPage.top;
				  LRTCharEx.xAllinea=LR_LEFT;
				  LRTCharEx.yChar=LRInchToDotY(40);
				  LRTCharEx.fFix=FALSE;
				  strcpy(LRTCharEx.szFontName,"Arial");
				  LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),dateFor(dateToday(),sPd.psDi->lpDate),0);
			  }

			  // PAGINA: Scrivo il numero di pagina
			  if (sPd.psDi->fPag)
			  {
				  memset(&LRTCharEx,0,sizeof(LRTCharEx));
				  LRTCharEx.Point.x=sPd.psDi->rPage.right;
				  LRTCharEx.Point.y=sPd.psDi->rPage.top;
				  LRTCharEx.xAllinea=LR_RIGHT;
				  LRTCharEx.yChar=LRInchToDotY(40);
				  LRTCharEx.fFix=FALSE;
				  strcpy(LRTCharEx.szFontName,"Tahoma");
				  LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),GiveLayPag(),0);
			  }
			  
			  // TITOLO : Scrivo il titolo centrato
			  if (sPd.psDi->lpTitolo)
			  {
				  memset(&LRTCharEx,0,sizeof(LRTCharEx));
				  LRTCharEx.Point.x=sPd.psDi->rPage.left+sPd.psDi->sPage.cx/2;
				  LRTCharEx.Point.y=sPd.psDi->rPage.top;
				  LRTCharEx.xAllinea=LR_CENTER;
				  LRTCharEx.yChar=LRInchToDotY(60);
				  //LRTCharEx.fBold=TRUE;
				  LRTCharEx.iStyles=STYLE_BOLD;
				  LRTCharEx.fFix=FALSE;
				  strcpy(LRTCharEx.szFontName,"Tahoma");
				  LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),sPd.psDi->lpTitolo,0);
			  }

			  // SOTTOTITOLO : Scrivo il sotto titolo centrato
			  if (sPd.psDi->lpSottoTitolo)
			  {
				  memset(&LRTCharEx,0,sizeof(LRTCharEx));
				  LRTCharEx.Point.x=sPd.psDi->rPage.left+(sPd.psDi->sPage.cx>>1);
				  LRTCharEx.Point.y=sPd.psDi->rPage.top+LRInchToDotY(55);
				  LRTCharEx.xAllinea=LR_CENTER;
				  LRTCharEx.yChar=LRInchToDotY(40);
				  //LRTCharEx.fBold=TRUE;
				  LRTCharEx.iStyles=STYLE_ITALIC;
//				  LRTCharEx.fItalic=TRUE;
				  LRTCharEx.fFix=FALSE;
				  strcpy(LRTCharEx.szFontName,"Tahoma");
				  LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),sPd.psDi->lpSottoTitolo,0);
			  }
				
			  //---------------------------------------------------
			  // Scrivo l'intestazione dei campi in stampa
			  //---------------------------------------------------
//			  yTitle=(sPd.psDi->yBodyTop-sPd.psDi->ySectDot+1);
			  
			  // Posizione di inizio del titolo
			  sPd.psDi->yHeadBottom=(sPd.psDi->yBodyTop-(sPd.psDi->rFieldPadding.top+sPd.psDi->rFieldPadding.bottom)-sPd.psDi->iTitleHeight);
			  for (a=0;a<sPd.psDi->iFieldNum;a++)
			  {
  				 if (*sPd.psDi->LRFields[a]->szTitolo)
				 {
				  // Sfondo
				  memset(&LRTBox,0,sizeof(LRTBox));
				  LRTBox.rRect.left=sPd.psDi->LRFields[a]->xPosStartDot;
			      LRTBox.rRect.right=sPd.psDi->LRFields[a]->xPosEndDot;
				  LRTBox.rRect.top=sPd.psDi->yHeadBottom;//(yTitle+sPd.psDi->LRFields[a]->iLine*sPd.psDi->iTitleHeight);
				  LRTBox.rRect.bottom=sPd.psDi->yBodyTop-1;//LRTBox.rRect.top+sPd.psDi->yChar-1;

			      LRTBox.Colore=sPd.psDi->LRFields[a]->lColTitBack;
				  LREAddItem(LRE_BOXP,&LRTBox,sizeof(LRTBox),0,0);

				  LRTBox.Colore=sPd.psDi->LRFields[a]->lColChar;
				  LREAddItem(LRE_BOX,&LRTBox,sizeof(LRTBox),0,0);

				  // Scritta
				  memset(&LRTChar,0,sizeof(LRTChar));
				  switch (sPd.psDi->LRFields[a]->iAllineaTitolo)
				  {
					case LR_LEFT : LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosStartDot+sPd.psDi->LRFields[a]->rPadding.left;//LocInchToDotX(4); 
								   LRTChar.xAllinea=LR_LEFT;				   
								   break;

					case LR_RIGHT: LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosEndDot-sPd.psDi->LRFields[a]->rPadding.right;//-LocInchToDotX(2); 
								   LRTChar.xAllinea=LR_RIGHT;				   
								   break;
				
					case LR_CENTER:LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosStartDot+((sPd.psDi->LRFields[a]->xPosEndDot-sPd.psDi->LRFields[a]->xPosStartDot)>>1); 
								   LRTChar.xAllinea=LR_CENTER;				   
								   break;
				  }
				 
				  LRTChar.Point.y=sPd.psDi->yHeadBottom+sPd.psDi->LRFields[a]->rPadding.top;//+LRTBox.rRect.top+((sPd.psDi->ySectDot-sPd.psDi->iTitleHeight)>>1);
				  LRTChar.yChar=sPd.psDi->iTitleHeight;
				  //LRTChar.fBold=TRUE;
				  LRTChar.iStyles=STYLE_BOLD;
				  LRTChar.fFix=FALSE;
				  LRTChar.lColore=sPd.psDi->LRFields[a]->lColTitChar;
				  LREAddItem(LRE_CHAR,&LRTChar,sizeof(LRTChar),sPd.psDi->LRFields[a]->szTitolo,0);

				 }
			}
			break;

		case LR_LAYTYPE1:

			// -------------------------------------------------------
			// LayOut Tipo 1 : Nuovo Tipo
			// -------------------------------------------------------
			// DATA: Scrivo la data di oggi in alto a destra
			// -------------------------------------------------------
			  if (sPd.psDi->fDate)
			  {
				  memset(&LRTCharEx,0,sizeof(LRTCharEx));
				  LRTCharEx.Point.x=sPd.psDi->rPage.right;
				  LRTCharEx.Point.y=sPd.psDi->rPage.top;
				  LRTCharEx.xAllinea=LR_RIGHT;
				  LRTCharEx.yChar=LRInchToDotY(50);
				  LRTCharEx.fFix=FALSE;
				  strcpy(LRTCharEx.szFontName,"Arial");
				  LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),dateFor(dateToday(),sPd.psDi->lpDate),0);
			  }

			  // PAGINA: Scrivo il numero di pagina
			  if (sPd.psDi->fPag)
			  {
				  memset(&LRTCharEx,0,sizeof(LRTCharEx));
				  LRTCharEx.Point.x=sPd.psDi->rPage.right;
				  LRTCharEx.Point.y=sPd.psDi->rPage.bottom-LRInchToDotY(40);
				  LRTCharEx.xAllinea=LR_RIGHT;
				  LRTCharEx.yChar=LRInchToDotY(40);
				  LRTCharEx.fFix=FALSE;
				  strcpy(LRTCharEx.szFontName,"Tahoma");
				  LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),GiveLayPag(),0);
			  }
			  
			  // TITOLO : Scrivo il titolo centrato
			  if (sPd.psDi->lpTitolo)
			  {
				  memset(&LRTCharEx,0,sizeof(LRTCharEx));
				  LRTCharEx.Point.x=sPd.psDi->rPage.left;
				  LRTCharEx.Point.y=sPd.psDi->rPage.top;
				  LRTCharEx.xAllinea=LR_LEFT;
				  LRTCharEx.yChar=LRInchToDotY(70);
//				  LRTCharEx.fFix=FALSE;
				  strcpy(LRTCharEx.szFontName,"Tahoma");
				  LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),sPd.psDi->lpTitolo,0);
			  }

			  // SOTTOTITOLO : Scrivo il sotto titolo centrato
			  if (sPd.psDi->lpSottoTitolo)
			  {
				  memset(&LRTCharEx,0,sizeof(LRTCharEx));
				  LRTCharEx.Point.x=sPd.psDi->rPage.left;
				  LRTCharEx.Point.y=sPd.psDi->rPage.top+LRInchToDotY(60);
				  LRTCharEx.xAllinea=LR_LEFT;
				  LRTCharEx.yChar=LRInchToDotY(40);
				  //LRTCharEx.fBold=TRUE;
				  LRTCharEx.iStyles=STYLE_ITALIC;
//				  LRTCharEx.fItalic=TRUE;
				  LRTCharEx.fFix=FALSE;
				  strcpy(LRTCharEx.szFontName,"Tahoma");
				  LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),sPd.psDi->lpSottoTitolo,0);
			  }
			  
			  //---------------------------------------------------
			  // Tiro una linea di divisione
			  //---------------------------------------------------
			  sPd.psDi->yHeadBottom=(sPd.psDi->yBodyTop-(sPd.psDi->iLinePerRiga*sPd.psDi->iTitleHeight)+1)-(sPd.psDi->rFieldPadding.bottom+sPd.psDi->rFieldPadding.top); //sPd.psDi->ySectDot

			  memset(&LRTBox,0,sizeof(LRTBox));
			  LRTBox.rRect.left=sPd.psDi->rPage.left;
			  LRTBox.rRect.right=sPd.psDi->rPage.right;
			  LRTBox.rRect.top=sPd.psDi->yHeadBottom-LRInchToDotY(6);
			  LRTBox.rRect.bottom=sPd.psDi->yHeadBottom;
			  LRTBox.Colore=sPd.psDi->LRFields[0]->lColChar;
			  LREAddItem(LRE_BOXP,&LRTBox,sizeof(LRTBox),0,0);
				
			  memset(&LRTBox,0,sizeof(LRTBox));
			  LRTBox.rRect.left=sPd.psDi->rPage.left;
			  LRTBox.rRect.right=sPd.psDi->rPage.right;
			  LRTBox.rRect.top=sPd.psDi->yBodyTop-LRInchToDotY(3);
			  LRTBox.rRect.bottom=sPd.psDi->yBodyTop;
			  LRTBox.Colore=sPd.psDi->LRFields[0]->lColChar;
			  LREAddItem(LRE_BOXP,&LRTBox,sizeof(LRTBox),0,0);

			  LRTBox.rRect.top=sPd.psDi->yBodyBottom-LRInchToDotY(3);
			  LRTBox.rRect.bottom=sPd.psDi->yBodyBottom;
			  LREAddItem(LRE_BOXP,&LRTBox,sizeof(LRTBox),0,0);

			  //---------------------------------------------------
			  // Scrivo l'intestazione dei campi in stampa
			  //---------------------------------------------------
			  for (a=0;a<sPd.psDi->iFieldNum;a++)
			  {
  				 if (*sPd.psDi->LRFields[a]->szTitolo)
				 {
				  // Sfondo
				  memset(&LRTBox,0,sizeof(LRTBox));
			      LRTBox.rRect.left=sPd.psDi->LRFields[a]->xPosStartDot;
			      LRTBox.rRect.right=sPd.psDi->LRFields[a]->xPosEndDot;
			      LRTBox.rRect.top=sPd.psDi->yHeadBottom;//+sPd.psDi->LRFields[a]->iLine*sPd.psDi->yChar;
				  LRTBox.rRect.bottom=sPd.psDi->yBodyTop-1;
			      LRTBox.Colore=sPd.psDi->LRFields[a]->lColTitBack;
				  //LREAddItem(LRE_BOXP,&LRTBox,sizeof(LRTBox),0,0);

				  //LRTBox.Colore=sPd.psDi->LRFields[a]->lColChar;
				  //LREAddItem(LRE_BOX,&LRTBox,sizeof(LRTBox),0,0);

				  // Scritta
				  memset(&LRTChar,0,sizeof(LRTChar));
				  switch (sPd.psDi->LRFields[a]->iAllineaTitolo)
				  {
					case LR_LEFT : LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosStartDot+sPd.psDi->LRFields[a]->rPadding.left;//LRInchToDotY(4); 
								   LRTChar.xAllinea=LR_LEFT;				   
								   break;

					case LR_RIGHT: LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosEndDot-sPd.psDi->LRFields[a]->rPadding.right;//-LRInchToDotY(2); 
								   LRTChar.xAllinea=LR_RIGHT;				   
								   break;
				
					case LR_CENTER:LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosStartDot+((sPd.psDi->LRFields[a]->xPosEndDot-sPd.psDi->LRFields[a]->xPosStartDot)>>1); 
								   LRTChar.xAllinea=LR_CENTER;				   
								   break;
				  }
				 
				  LRTChar.Point.y=sPd.psDi->yHeadBottom+sPd.psDi->LRFields[a]->rPadding.top;//LRTBox.rRect.top+((sPd.psDi->ySectDot-sPd.psDi->iTitleHeight)>>1);
				  LRTChar.yChar=sPd.psDi->iTitleHeight;
//				  LRTChar.fBold=TRUE;
				  LRTChar.iStyles=STYLE_BOLD;
				  LRTChar.fFix=FALSE;
//				  LRTChar.lColore=sPd.psDi->LRFields[a]->lColTitChar;
				  LRTChar.lColore=sPd.psDi->LRFields[a]->lColChar;
				  LREAddItem(LRE_CHAR,&LRTChar,sizeof(LRTChar),sPd.psDi->LRFields[a]->szTitolo,0);
				 }
			}
			break;
	}

	// Se esite solo una linea di stampa 
	// Ed è abilitata 
	// creo la divisione delle righe
	if (sPd.psDi->iLinePerRiga==1)
	{
		for (a=0;a<sPd.psDi->iFieldNum;a++)
		{
			memset(&LRTBox,0,sizeof(LRTBox));
			LRTBox.rRect.left=sPd.psDi->LRFields[a]->xPosStartDot;
			LRTBox.rRect.right=sPd.psDi->LRFields[a]->xPosEndDot;
			LRTBox.rRect.top=sPd.psDi->yBodyTop;
			LRTBox.rRect.bottom=sPd.psDi->yBodyBottom;
			       
			// Colore di sfondo
			if (sPd.psDi->LRFields[a]->lColBack!=-1) 
				   {   
					   LRTBox.Colore=sPd.psDi->LRFields[a]->lColBack;
					   LREAddItem(LRE_BOXP,&LRTBox,sizeof(LRTBox),0,0);
				   }
	
			if (sPd.psDi->fLineVertField)
			{
				//LRTBox.Colore=sPd.psDi->LRFields[a]->lColChar;
				if (!a)
				{
					LRLine( LRTBox.rRect.left,
							LRTBox.rRect.top,
							LRTBox.rRect.left,
							LRTBox.rRect.bottom,
							sPd.psDi->LRFields[a]->lColChar,
							1,
							PS_SOLID,
							SET);
				}
				LRLine( LRTBox.rRect.right,
						LRTBox.rRect.top,
						LRTBox.rRect.right,
						LRTBox.rRect.bottom,
						sPd.psDi->LRFields[a]->lColChar,
						1,
						PS_SOLID,
						SET);

				//LREAddItem(LRE_BOX,&LRTBox,sizeof(LRTBox),0,0);
			}
		}
	}
}

// ------------------------------------------
// | CREA6 Builder  by Ferrà A&T 10/08/1999 |
// ------------------------------------------+
static INT MyWin=-1;

#ifndef _CONSOLE			
static void WinStart(void)
{
// Creato dal progetto : Nuovo.vpg 

  static struct IPT ipt[]={
	{ 1, 1,NUME,RIGA,105, 42, 37,  4,  0, 15,  0,  1,"No","No"},
	{ 0, 0,STOP}
	};

//  Header di attivazione
	win_open(EHWP_SCREENCENTER,42,160,75,-1,3,ON,"Attendere ...");
	MyWin=WIN_ult;
	ico_disp(13,32,"STAMPA");
	dispf(63,39,0,-1,0,"SMALL F",3,"Pagina");

//  Carico OBJ & variazioni sui Font
	ipt_font("SMALL F",3);
	ipt_open(ipt);
	//ipt_fontnum(0,"SMALL F",3);
	ipt_reset(); ipt_vedi();
	//eventGet(NULL);
	//eventGet(NULL);
	OsEventLoop(2);
}
#endif
//
// PowerPrinter()
//
void * PowerPrinter(INT cmd,LONG info,void *ptr)
{
	LRFIELDS *lpLRField;
	INT a;
	LONG l;
	INT iAuto=0;
	double dTotPerc=0;
	CHAR Buf[80];
	INT iFieldPerRiga;

	LRT_CHAR    LRTChar;
	LRT_CHARBOX LRTCharBox;
	LRT_BOX     LRTBox;

	double Valore;
	CHAR *lp;
	CHAR *lpText;
#ifndef _CONSOLE
	S_WINSCENA WScena;
#endif
	LRT_LINK sLRALink;

	// Inizializzo la prima volta
	if (!sPd.bReady) {
		_(sPd);
		//sPd.hmField=-1;
		sPd.lstFld=NULL;
		sPd.bReady=TRUE;
	}

	switch (cmd)
	{
	//-----------------------------------------------------------------------
	//	WS_OPEN                         		                            |
	//											                            |
	//	Apertura del server di Stampa report                                |
	//  info=0                                                              |
	//  ptr= * EH_PD (Inizializzazione dei parametri)                      |
	//                                                                      |
	//-----------------------------------------------------------------------
		case WS_OPEN: // Inizializzazione del report
			
			LREFlushBuffer(FALSE);
			if (sPd.psDi!=NULL) PowerPrinter(WS_CLOSE,FALSE,NULL);
			sPd.psDi=(EH_PD *) ptr;

			// Inizializzo il resto della struttura
			sPd.psDi->iPageCount=0;
			sPd.psDi->bBold=FALSE;
			sPd.psDi->bItalic=FALSE;
			sPd.psDi->bUnderLine=FALSE;
			//sPd.fArray=FALSE; sPd.hmField=-1;
			sPd.lstFld=NULL;

			// Apro un'array di LRField
			if (sPd.psDi->iLayStyle!=LR_PAGEFREE)
			{
				 //a=sizeof(LRFIELDS);
				 //ARMaker(WS_OPEN,&a);
				 sPd.lstFld=lstCreate(sizeof(LRFIELDS));
				 //sPd.fArray=TRUE; 
			}
			
			sPd.psDi->iFieldNum=0;
			GetTempPath(sizeof(sPd.szTempPath),sPd.szTempPath);
			GetTempFileName(sPd.szTempPath,"LREP",0,sPd.szTempFile);
			sPd.chTempFile=fopen(sPd.szTempFile,"wb");
			if (!sPd.chTempFile) ehExit("LR1");

			sPd.psDi->iLinePerRiga=0; // Azzero il numero di linee per riga (gestione multi riga)
			sPd.psDi->iRowsSize=0;
			if (sPd.psDi->HookSubProc!=NULL)
				{
					(*sPd.psDi->HookSubProc)(WS_OPEN,0,NULL);
				}

#ifndef _CONSOLE			
			WinStart();
#endif

			//
			// Cerco le dimensioni del documento
			// Se vuoto, uso il PD di sistema (compatibilità con il vecchio)
			//
			if (strEmpty(sPd.psDi->pszDeviceDefine)) {
				
				sPd.psDi->hdcPrinter=sys.sPrintDlg.hDC;//=memcpy(&sPd.psDi->sPrinter,&sys.pd,sizeof(sys.pd));
				sPd.psDi->pDevMode=GlobalLock(sys.sPrintDlg.hDevMode);
				sPd.psDi->iOrientation=sPd.psDi->pDevMode->dmOrientation;
				GlobalUnlock(sPd.psDi->pDevMode);

			} else {

				sPd.psDi->hdcPrinter=ehPrinterCreateDC(sPd.psDi->pszDeviceDefine,&sPd.psDi->pDevMode);
				sPd.psDi->iOrientation=sPd.psDi->pDevMode->dmOrientation;

//					alert("qui");
/*
				HANDLE hPrinter;
				BOOL bRet;
				PRINTER_DEFAULTS sPrnDefault;

				bRet=OpenPrinter((char*) sPd.psDi->szPrinterName, &hPrinter, &sPrnDefault);
				if (!bRet) ehError();


				ClosePrinter(&hPrinter);
*/			

			}

			
			sPd.psDi->sPhysicalPage.cx=GetDeviceCaps(sPd.psDi->hdcPrinter,PHYSICALWIDTH);
			sPd.psDi->sPaper.cx=GetDeviceCaps(sPd.psDi->hdcPrinter,HORZRES);
			sPd.psDi->sPhysicalPage.cy=GetDeviceCaps(sPd.psDi->hdcPrinter,PHYSICALHEIGHT);
			sPd.psDi->sPaper.cy=GetDeviceCaps(sPd.psDi->hdcPrinter,VERTRES);
						
			if (!sPd.psDi->sPaper.cx||!sPd.psDi->sPaper.cy) ehExit("Stampante/Formato carta non selezionato (%s:%d)",__FILE__,__LINE__);
			sPd.psDi->xInchDot=GetDeviceCaps(sPd.psDi->hdcPrinter,LOGPIXELSX);
			sPd.psDi->yInchDot=GetDeviceCaps(sPd.psDi->hdcPrinter,LOGPIXELSY);

			break;

	//-----------------------------------------------------------------------
	//	WS_CLOSE                        		                            |
	//											                            |
	//	Chiusura del server di Stampa report                                |
	//  info=FALSE non stampare (abort)                                     |
	//		 TRUE  stampa il rapport                                        |
	//											                            |
	//	ptr = NULL stampa diretta                                           |
	//  ptr = !=NULL  Preview della stampa                                  |
	//                                                                      |
	//-----------------------------------------------------------------------
		case WS_CLOSE: // Stampa/Preview del report

			// Ho una pagina in sospeso (chiudo la pagina)
			if (sPd.psDi->bPageInProgress)
			{
				_(sLRALink);
				sLRALink.iType=LRA_FOOT;
				sLRALink.bLastPage=TRUE;
				sLRALink.rRect.left=sPd.psDi->rPage.left;
				sLRALink.rRect.top=sPd.psDi->yBodyBottom;//>rPage.bottom-sPd.psDi->yCueDot;
				sLRALink.rRect.right=sPd.psDi->rPage.right;
				sLRALink.rRect.bottom=sPd.psDi->rPage.bottom;
				if (sPd.psDi->HookSubProc) (*sPd.psDi->HookSubProc)(WS_DO,0,&sLRALink);
				sPd.psDi->bPageInProgress=FALSE;
			}

#ifndef _CONSOLE			
			win_close();
#endif
			// Chiudo il file
			LREAddItem(LRE_ENDFILE,NULL,0,NULL,0);
			fclose(sPd.chTempFile); sPd.chTempFile=NULL; 

			LRETagReplace();

#ifndef _CONSOLE			
			if (info) // stampare
			{
				if (ptr) 
					LREPreview((CHAR *) ptr);	
					else
					LREPrintDirect((CHAR *) ptr);
			}
#else
			LREPrintDirect((CHAR *) ptr);
#endif
			if (sPd.psDi->HookSubProc!=NULL)
			{
				(*sPd.psDi->HookSubProc)(WS_CLOSE,0,NULL);
			}

			// Libera le risorse impegnate
			LREFileFreeResource(TRUE);
			// remove(sPd.szTempFile);
			// if (sPd.fArray) sPd.hmField=ARMaker(WS_CLOSE,"*LRFields");
			//if (sPd.hmField!=-1) memoFree(sPd.hmField,"*LRFields");
			if (sPd.lstFld) sPd.lstFld=lstDestroy(sPd.lstFld);
			//sPd.fArray=FALSE; sPd.hmField=-1;
			sPd.psDi->LRFields=NULL;
			sPd.psDi=NULL;
			break;
	
	//-----------------------------------------------------------------------
	//	WS_SETFLAG                      		                            |
	//											                            |
	//	Inizializzo la struttura LRFIELD con i dati di default              |
	//  ptr= * LRFIELD                                                      |
	//                                                                      |
	//-----------------------------------------------------------------------

		case WS_SETFLAG:
			
			if (ptr==NULL) ehExit("LR2");
			lpLRField=(LRFIELDS *) ptr;
			memset(lpLRField,0,sizeof(LRFIELDS));
			lpLRField->iRowsSize=1; // Altezza di default
			switch (info)
			{
				case ALFA:

					lpLRField->fFix=FALSE;
					lpLRField->iAllinea=LR_LEFT;
					lpLRField->iTipo=ALFA;
					lpLRField->iAllineaTitolo=LR_LEFT;
					break;

				case NUME:

					lpLRField->fFix=TRUE;
					lpLRField->iAllinea=LR_RIGHT;
					lpLRField->iTipo=NUME;
					lpLRField->iAllineaTitolo=LR_RIGHT;
					break;
			}

			lpLRField->lColTitChar=ColorGray(0);
			lpLRField->lColTitBack=ColorGray(60);
			lpLRField->lColBack=-1;
			lpLRField->lColChar=0;
			break;

	//-----------------------------------------------------------------------
	//	WS_ADD                          		                            |
	//											                            |
	//	Aggiunge un definizione di un campo                                 |
	//  ptr= * LRFIELD                                                      |
	//                                                                      |
	//-----------------------------------------------------------------------
		case WS_ADD:
			switch (info)
			{
				// Aggiunga la definizione di un campo
				case LR_ADDFIELD: 
					if (ptr==NULL) ehExit("LR5");
					lpLRField=(LRFIELDS *) ptr;
					// Controlla il numero di linee
					if (lpLRField->iLine>sPd.psDi->iLinePerRiga) 
						sPd.psDi->iLinePerRiga=lpLRField->iLine;
					// Controlla il numero di "rows" per linea
					if (lpLRField->iRowsSize!=LR_ROWSDYNAMIC) 
					{
					 if ((lpLRField->iLine+lpLRField->iRowsSize)>sPd.psDi->iRowsSize) 
						sPd.psDi->iRowsSize=lpLRField->iLine+lpLRField->iRowsSize;
					}
					else
					{
						sPd.psDi->bRowsDynamic=TRUE; // Segnalo che sono presenti linee dinamiche
					}

					//ARMaker(WS_ADD,ptr);
					lstPush(sPd.lstFld,ptr);
					break;

				case LR_ADDPAGE:

					if (sPd.psDi->bPageInProgress)
					{
					   _(sLRALink);
					   sLRALink.iType=LRA_FOOT;
					   sLRALink.rRect.left=sPd.psDi->rPage.left;
					   sLRALink.rRect.top=sPd.psDi->yBodyBottom;
					   sLRALink.rRect.right=sPd.psDi->rPage.right;
					   sLRALink.rRect.bottom=sPd.psDi->rPage.bottom;
					   (*sPd.psDi->HookSubProc)(WS_DO,0,&sLRALink);
					   sPd.psDi->bPageInProgress=FALSE;
					}

					sPd.psDi->iVirtualLineCount=0;
					sPd.psDi->iRowOffset=sPd.psDi->yBodyTop;
					sPd.psDi->iPageCount++; 
#ifndef _CONSOLE
					WinWriteSet(MyWin,&WScena);
					ipt_writevedi(0,"",sPd.psDi->iPageCount);
					WinWriteRestore(&WScena);
#endif
					LptReportLayout();

					// Se esiste un HookSubProc segnalo il cambio di pagina
					if (sPd.psDi->HookSubProc!=NULL)
					{
			   
						_(sLRALink);
						sLRALink.iType=LRA_HEAD;
						sLRALink.rRect.left=sPd.psDi->rPage.left;
						sLRALink.rRect.top=sPd.psDi->rPage.top;
						sLRALink.rRect.right=sPd.psDi->rPage.right;
						sLRALink.rRect.bottom=sPd.psDi->yHeadBottom;
						(*sPd.psDi->HookSubProc)(WS_DO,0,&sLRALink);
						sPd.psDi->bPageInProgress=TRUE;

/*			   
						_(LRALink);
						LRALink.iType=LRA_FOOT;
						LRALink.rRect.left=sPd.psDi->rPage.left;
						LRALink.rRect.top=sPd.psDi->yBodyBottom;
						LRALink.rRect.right=sPd.psDi->rPage.right;
						LRALink.rRect.bottom=sPd.psDi->rPage.bottom;
						(*sPd.psDi->HookSubProc)(WS_DO,0,&LRALink);
						*/

					}
			}
			break;

	//-----------------------------------------------------------------------
	//	WS_DO                           		                            |
	//											                            |
	//	Prima elaborazione e definizione array                              |
	//											                            |
	//	Chiudo l'array di definizione campi                                 |
	//  Nessun parametro                                                    |
	//-----------------------------------------------------------------------
		
		case WS_DO:
		
			if (sPd.psDi->iLayStyle!=LR_PAGEFREE) 
			{
				//if (!sPd.fArray) ehExit("LR6"); 
				//sPd.fArray=FALSE;
				//sPd.hmField=ARMaker(WS_CLOSE,"*LRFields");
				//sPd.psDi->LRFields=memoPtr(sPd.hmField,NULL);
				sPd.psDi->LRFields=(LRFIELDS **) lstToArf(sPd.lstFld,false);
				// Conto i campi 
				for (a=0;;a++)
				{
					if (sPd.psDi->LRFields[a]==NULL) break;
				//win_infoarg("%d) [%s]",a,sPd.psDi->LRFields[a]->szTitolo);
				}
			   sPd.psDi->iFieldNum=a;
			} else sPd.psDi->LRFields=NULL;
			

			// -------------------------------------------------------------------------
			// Leggo e calcolo i dati metrici del foglio di stampa
			// -------------------------------------------------------------------------
			sPd.psDi->iLinePerRiga++;


			/*
			{
				DEVMODE *lpDevMode;
				lpDevMode=GlobalLock(sys.sPrintDlg.hDevMode);
				win_infoarg("%s\n[%d,%d] [%d,%d]",
						    lpDevMode->dmDeviceName,
							sPd.psDi->sPaper.cx,sPd.psDi->sPaper.cy,
							GetDeviceCaps(sys.sPrintDlg.hDC,HORZRES),
							(INT) lpDevMode->dmPaperLength);
				GlobalUnlock(sys.sPrintDlg.hDevMode);
			}
*/

			// Dovrei settarlo in modo parametrico
			// Field Margin
			sPd.psDi->rFieldPadding.left=LRInchToDotX(8);
			sPd.psDi->rFieldPadding.right=LRInchToDotX(8); // LRInchToDotX(2);
			sPd.psDi->rFieldPadding.top=LRInchToDotY(8);
			sPd.psDi->rFieldPadding.bottom=LRInchToDotY(8);
			
			sPd.psDi->iRowHeight=LRInchToDotY(sPd.psDi->ptRowHeight);
//			sPd.psDi->ySectDot=sPd.psDi->iRowsSize*sPd.psDi->yChar;

			sPd.psDi->iTitlePadded=(sPd.psDi->iRowsSize*sPd.psDi->iTitleHeight)+(sPd.psDi->rFieldPadding.top+sPd.psDi->rFieldPadding.bottom);
			sPd.psDi->iRowPadded=(sPd.psDi->iRowsSize*sPd.psDi->iRowHeight)+(sPd.psDi->rFieldPadding.top+sPd.psDi->rFieldPadding.bottom);

			// Tolgo i margini e calcolo le dimensioni possibili della pagina

			sPd.psDi->rPage.left=LRInchToDotX(sPd.psDi->rMargin.left);
			sPd.psDi->rPage.right=(sPd.psDi->sPaper.cx-1)-LRInchToDotX(sPd.psDi->rMargin.right);
			sPd.psDi->rPage.top=LRInchToDotY(sPd.psDi->rMargin.top);
			sPd.psDi->rPage.bottom=(sPd.psDi->sPaper.cy-1)-LRInchToDotY(sPd.psDi->rMargin.bottom);
			sPd.psDi->sPage.cx=sPd.psDi->rPage.right-sPd.psDi->rPage.left+1;
			sPd.psDi->sPage.cy=sPd.psDi->rPage.bottom-sPd.psDi->rPage.top+1;
			
			sPd.psDi->iTitleHeight=LRInchToDotY(sPd.psDi->ptTitleHeight);
			
			if (sPd.psDi->ptHeadHeight==0)  
				{
					switch (sPd.psDi->iLayStyle)
					{
						case 0:
						case 1: sPd.psDi->yBodyTop=LRInchToDotY(110)+sPd.psDi->iTitlePadded; break;
						case 2: sPd.psDi->yBodyTop=LRInchToDotY(140)+sPd.psDi->iTitlePadded; break;
					}
				}
				else 
				sPd.psDi->yBodyTop=LRInchToDotY(sPd.psDi->ptHeadHeight);//(LONG) (sPd.psDi->sPage.cy*sPd.psDi->fHeadPerc)/100;

			if (sPd.psDi->ptFootHeight==0)   
				{
					switch (sPd.psDi->iLayStyle)
					{
						case 0:
						case 1: sPd.psDi->yFootHeight=LRInchToDotY(1); break;
						case 2: sPd.psDi->yFootHeight=LRInchToDotY(60); break;
					}
				}
				else 
				sPd.psDi->yFootHeight=LRInchToDotY(sPd.psDi->ptFootHeight);//sPd.psDi->yCueDot=(LONG) (sPd.psDi->sPage.cy*sPd.psDi->fCuePerc)/100;
			
			sPd.psDi->yBodyHeight=sPd.psDi->sPage.cy-(sPd.psDi->yBodyTop+sPd.psDi->yFootHeight);
			
			sPd.psDi->yBodyTop+=sPd.psDi->rPage.top;
//			sPd.psDi->yCueDot=sPd.psDi->rPage.bottom-sPd.psDi->yCueDot;
			sPd.psDi->yBodyBottom=sPd.psDi->rPage.bottom-sPd.psDi->yFootHeight;
			
//			sPd.psDi->iRowHeightReal=LRInchToDotY(sPd.psDi->iRowHeight)+sPd.psDi->rFieldPadding.top+sPd.psDi->rFieldPadding.bottom;
//			sPd.psDi->iTitleHeightReal=LRInchToDotY(sPd.psDi->iTitleHeight)+sPd.psDi->rFieldPadding.top+sPd.psDi->rFieldPadding.bottom;

			sPd.psDi->iVirtualLinePerPage=0; // Azzero il numero di linee per pagina
			sPd.psDi->iVirtualLineCount=0;  // Azzero il contatorei di linee per pagina
			sPd.psDi->iRowOffset=sPd.psDi->yBodyTop;

			// -------------------------------------------------------------------------
			// Trasformo le dimensioni dei campi in percentuali in dimensioni fisiche
			// Calcolo quante linee ci stanno nell'area tolta la testa e la coda
			// -------------------------------------------------------------------------
			if (sPd.psDi->iRowPadded) sPd.psDi->iVirtualLinePerPage=(LONG) sPd.psDi->yBodyHeight/sPd.psDi->iRowPadded;
			sPd.psDi->iVirtualLineCount=sPd.psDi->iVirtualLinePerPage; // Per stampare la prima pagina
			
			// Se la stampa è senza campi, cioe PAGEFREE mi fermo quà
			if (sPd.psDi->LRFields==NULL) break;

			for (l=0;l<sPd.psDi->iLinePerRiga;l++)
			{
			 // -------------------------------------------------------------------------------------------
			 // A) Calcolo il totale delle percentuali stabilite e conti i campi per linea (colonne)
		     // -------------------------------------------------------------------------------------------
			 iFieldPerRiga=0; iAuto=0; dTotPerc=0;
			 for (a=0;a<sPd.psDi->iFieldNum;a++)
			 {
				if (sPd.psDi->LRFields[a]->iLine!=l) continue;
				if (sPd.psDi->LRFields[a]->xPercSize<.01F) 
						{iAuto++; sPd.psDi->LRFields[a]->xPercSize=0;} else {dTotPerc+=sPd.psDi->LRFields[a]->xPercSize;}
				iFieldPerRiga++;
			 }
			 if (!iFieldPerRiga) break; // Finito
			 
			 // Calcolo i campi per riga
			 sPd.psDi->iFieldPerRiga[l]=iFieldPerRiga;
			
		     // -------------------------------------------------------------------------------------------
			 // B) Sistemo le percentuali per i campi non stabiliti                                     
		     // -------------------------------------------------------------------------------------------
			 dTotPerc=100-dTotPerc;
			 if (iAuto&&(dTotPerc>0))
			 {
				for (a=0;a<sPd.psDi->iFieldNum;a++)
				{
					if (sPd.psDi->LRFields[a]->iLine!=l) continue;
					if (sPd.psDi->LRFields[a]->xPercSize==0) sPd.psDi->LRFields[a]->xPercSize=dTotPerc/iAuto;
				}
			 }

			 // -------------------------------------------------------------------------------------------
			 // C) Calcolo le percentuali di posizione e la grandezza fisica dei campi                  
		     // -------------------------------------------------------------------------------------------
			 dTotPerc=0;
			 for (a=0;a<sPd.psDi->iFieldNum;a++)
			 {
				if (sPd.psDi->LRFields[a]->iLine!=l) continue;
				sPd.psDi->LRFields[a]->xPercPos=dTotPerc;
				dTotPerc+=sPd.psDi->LRFields[a]->xPercSize;

				// Se non indicato diversamente, setto il patting del campo uguale a quello di default della tabella
				if (!sPd.psDi->LRFields[a]->bPadding) memcpy(&sPd.psDi->LRFields[a]->rPadding,&sPd.psDi->rFieldPadding,sizeof(RECT));

				// Grandezza Fisica
				// x:sPd.psDi->xPageDot=Perc:100;				
				sPd.psDi->LRFields[a]->xPosStartDot   =sPd.psDi->rPage.left+ (LONG) (sPd.psDi->sPage.cx*sPd.psDi->LRFields[a]->xPercPos/100);
				sPd.psDi->LRFields[a]->xPosEndDot     =sPd.psDi->rPage.left+ (LONG) (sPd.psDi->sPage.cx*dTotPerc/100);
				if (sPd.psDi->LRFields[a]->xPosEndDot>sPd.psDi->rPage.right) sPd.psDi->LRFields[a]->xPosEndDot=sPd.psDi->rPage.right;
			    /*
				win_infoarg("Assegno: %.2f , %.2f   -   %d,%d",
					        sPd.psDi->LRFields[a]->xPercPos,dTotPerc,
							sPd.psDi->LRFields[a]->xPosStartDot,
							sPd.psDi->LRFields[a]->xPosEndDot);
							*/
			 }
			}

			break;
	
	//-----------------------------------------------------------------------
	//	WS_LINK                         		                            |
	//											                            |
	//	Start passaggio campi della RIGA                                    |
	//  info  Long di riferimento alla linea: usato per la PostPaint()      |
	//	ptr   (non usato)					                                |
	//											                            |
	//-----------------------------------------------------------------------

		case WS_LINK:
			
			// ----------------------------------
			// Cambio di Pagina
			// Intestazione Pagina
			// ----------------------------------
			//win_infoarg("%d - %d",sPd.psDi->iLineCount,sPd.psDi->iLinePerRiga);

			//
			// Dovrebbe essere
			// Se la dimensione della nuova linea non sta nella pagina dovrei creare una pagina nuova
			// Per fare questo dovrei prima memorizzarmi la fine della linea per determinare
			// l'altezza dinamica e poi quindi determinare se ci sta all'interno dell'area del corpo
			//
//			win_infoarg("%d,%d",sPd.psDi->iVirtualLineCount,sPd.psDi->iVirtualLinePerPage);
			if (sPd.psDi->iVirtualLineCount>=sPd.psDi->iVirtualLinePerPage) 
			{
				//
				// Ho una pagina in sospeso (chiudo la pagina)
				//
				  if (sPd.psDi->bPageInProgress)
				  {
					   _(sLRALink);
					   sLRALink.iType=LRA_FOOT;
					   sLRALink.rRect.left=sPd.psDi->rPage.left;
					   sLRALink.rRect.top=sPd.psDi->yBodyBottom;//>rPage.bottom-sPd.psDi->yCueDot;
					   sLRALink.rRect.right=sPd.psDi->rPage.right;
					   sLRALink.rRect.bottom=sPd.psDi->rPage.bottom;
					   if (sPd.psDi->HookSubProc) (*sPd.psDi->HookSubProc)(WS_DO,0,&sLRALink);
					   sPd.psDi->bPageInProgress=FALSE;
				  }

				  sPd.psDi->iVirtualLineCount=0;
				  sPd.psDi->iRowOffset=sPd.psDi->yBodyTop;
				  sPd.psDi->iPageCount++; 
				  sPd.psDi->bPageInProgress=TRUE;

#ifndef _CONSOLE
				  WinWriteSet(MyWin,&WScena);
				  ipt_writevedi(0,"",sPd.psDi->iPageCount);
				  WinWriteRestore(&WScena);
#endif
				  LptReportLayout();

				  // Se esiste un HookSubProc segnalo il cambio di pagina
				  if (sPd.psDi->HookSubProc!=NULL)
				  {
					   LRT_LINK LRALink;
					   
					   _(LRALink);
					   LRALink.iType=LRA_HEAD;
					   LRALink.rRect.left=sPd.psDi->rPage.left;
					   LRALink.rRect.top=sPd.psDi->rPage.top;
					   LRALink.rRect.right=sPd.psDi->rPage.right;
					   LRALink.rRect.bottom=sPd.psDi->yHeadBottom;
					   (*sPd.psDi->HookSubProc)(WS_DO,0,&LRALink);

				   /*
				   _(LRALink);
				   LRALink.iType=LRA_FOOT;
				   LRALink.rRect.left=sPd.psDi->rPage.left;
				   LRALink.rRect.top=sPd.psDi->yBodyBottom;//>rPage.bottom-sPd.psDi->yCueDot;
				   LRALink.rRect.right=sPd.psDi->rPage.right;
				   LRALink.rRect.bottom=sPd.psDi->rPage.bottom;
				   (*sPd.psDi->HookSubProc)(WS_DO,0,&LRALink);
				   */

				} 
			}
			
//			if (sPd.psDi->bRowsDynamic) sPd.psDi->iRowsLastLine=1;
			//if (sPd.psDi->bRowsDynamic) {sPd.psDi->iRowLastVirtualLine=1; sPd.psDi->iRowLastHeight=sPd.psDi->iRowPadded;}
			if (sPd.psDi->bRowsDynamic) 
			{
				sPd.psDi->iRowLastVirtualLine=1; 
				sPd.psDi->iRowMaxHeight=0;
			}
			break;
	
	//-----------------------------------------------------------------------
	//	WS_REALSET                      		                            |
	//											                            |
	//	Start passaggio campi della linea                                   |
	//  info Numero del campo a cui si fa riferimento                       |
	//	ptr  Puntatore al valore                                            |
	//		 ALFA      CHAR *                                               |
	//		 NUME      double *                                             |
	//											                            |
	//	Aggiungo al buffer di linea                                         |
	//											                            |
	//											                            |
	//-----------------------------------------------------------------------
		case WS_REALSET:

			lpText=(ptr!=NULL) ? ptr : "<?>";
			if (info>=sPd.psDi->iFieldNum) ehExit("LR RealSet: Field Errato");
			a=info;
			
			// Controllo se lpText và pulito
			if (sPd.psDi->LRFields[a]->fCharClean)
			{
				CHAR *lpEnd;
				while (*lpText) {if ((UINT) *lpText>' ') break; else lpText++;} // Mi sposto in avanti fino a trovare un carattere valido
				
				// Tolgo caratteri in coda
				for (lpEnd=lpText+strlen(lpText)-1;lpEnd>lpText;lpEnd--)
				{
					if ((UINT) *lpEnd>' ') break; else *lpEnd=0;
				}
			}

			// Inserisce il colore di sfondo
			_(LRTBox);
			LRTBox.rRect.left=sPd.psDi->LRFields[a]->xPosStartDot;
			LRTBox.rRect.right=sPd.psDi->LRFields[a]->xPosEndDot;
//			LRTBox.rRect.top=sPd.psDi->yBodyTop+1+(sPd.psDi->iLineCount*sPd.psDi->ySectDot)+sPd.psDi->LRFields[a]->iLine*sPd.psDi->yChar;
			LRTBox.rRect.top=sPd.psDi->iRowOffset+(sPd.psDi->LRFields[a]->iLine*sPd.psDi->iRowHeight)+1;
			if (sPd.psDi->LRFields[a]->iRowsSize==LR_ROWSDYNAMIC) 
					LRTBox.rRect.bottom=LRTBox.rRect.top+sPd.psDi->iRowHeight-1;
					else
					LRTBox.rRect.bottom=LRTBox.rRect.top+(sPd.psDi->iRowHeight*sPd.psDi->LRFields[a]->iRowsSize)-1;
				  

			// Colore di sfondo
//			if (sPd.psDi->LRFields[a]->lColBack&&(sPd.psDi->iLinePerRiga>1))
			if (sPd.psDi->LRFields[a]->lColBack!=-1)
			{
				LRTBox.Colore=sPd.psDi->LRFields[a]->lColBack;
				LREAddLineBuffer(LRE_BOXP,&LRTBox,sizeof(LRTBox),0,0);
			}

			// ------------------------------------------------------------------
			// Stampa ad una linea
			//
			if (sPd.psDi->LRFields[a]->iRowsSize==1)
			{
				memset(&LRTChar,0,sizeof(LRTChar));
				LRTChar.Point.y=LRTBox.rRect.top+sPd.psDi->LRFields[a]->rPadding.top;
				switch (sPd.psDi->LRFields[a]->iAllinea)
				{
					case LR_LEFT: LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosStartDot+sPd.psDi->LRFields[a]->rPadding.left;//sPd.psDi->rFieldMargin.left; 
								  LRTChar.xAllinea=LR_LEFT;				   
								  break;

					case LR_RIGHT: LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosEndDot-sPd.psDi->LRFields[a]->rPadding.right;//-sPd.psDi->rFieldMargin.right; 
								   LRTChar.xAllinea=LR_RIGHT;				   
								   break;

					case LR_CENTER: LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosStartDot+((sPd.psDi->LRFields[a]->xPosEndDot-sPd.psDi->LRFields[a]->xPosStartDot)>>1); 
									LRTChar.xAllinea=LR_CENTER;				   
									break;

					case LR_JUSTIFY: LRTChar.Point.x=sPd.psDi->LRFields[a]->xPosStartDot+sPd.psDi->LRFields[a]->rPadding.left;;//+sPd.psDi->rFieldMargin.left; 
									 LRTChar.xAllinea=LR_JUSTIFY;				   
									 break;
			 }

			 /*
			 LRTChar.fBold=sPd.psDi->bBold;
			 LRTChar.fItalic=sPd.psDi->bItalic;
			 LRTChar.fUnderLine=sPd.psDi->bUnderLine;
*/
			 LRTChar.iStyles=LRToStyles();
			 if (sPd.psDi->fGColor) LRTChar.lColore=sPd.psDi->lGColor;
								    else 
								    LRTChar.lColore=sPd.psDi->LRFields[a]->lColChar;

			 switch (sPd.psDi->LRFields[info]->iTipo)
			 {
				 case ALFA: // Campo alfanumerico

				  // Scritta
					LRTChar.yChar=sPd.psDi->iRowHeight;
					if (ptr) {if (!* (CHAR *) ptr) break;} // Salto i campi vuoti
					//LREAddItem(LRE_CHAR,&LRTChar,sizeof(LRTChar),lpText,0);
					LREAddLineBuffer(LRE_CHAR,&LRTChar,sizeof(LRTChar),lpText,0);
					break;

				 case NUME:
					if (ptr!=NULL) 
					{
					 Valore=* (double *) ptr;
					 strcpy(Buf,Snummask(Valore,sPd.psDi->LRFields[a]->iCifre,sPd.psDi->LRFields[a]->iDec,sPd.psDi->LRFields[a]->fSep,0));
					}
				    
					LRTChar.yChar=sPd.psDi->iRowHeight;
					LRTChar.fFix=sPd.psDi->LRFields[a]->fFix;
					if (sPd.psDi->fNoDecimal) {lp=strstr(Buf,","); if (lp) *lp=0;}
				   // LREAddItem(LRE_CHAR,&LRTChar,sizeof(LRTChar),lpText,0);
					lpText=Buf;
					LREAddLineBuffer(LRE_CHAR,&LRTChar,sizeof(LRTChar),lpText,0);
				    break;
			 }
			}
			else
			// ------------------------------------------------------------------
			// Stampa a più linee
			//
			{
				_(LRTCharBox);
				memcpy(&LRTCharBox.rArea,&LRTBox.rRect,sizeof(RECT));
				LRTCharBox.rArea.left+=sPd.psDi->LRFields[a]->rPadding.left;//sPd.psDi->rFieldMargin.left;
				LRTCharBox.rArea.right-=sPd.psDi->LRFields[a]->rPadding.right;
				LRTCharBox.rArea.top+=sPd.psDi->LRFields[a]->rPadding.top;
				LRTCharBox.rArea.bottom-=sPd.psDi->LRFields[a]->rPadding.bottom;
				LRTCharBox.xAllinea=sPd.psDi->LRFields[a]->iAllinea;
				LRTCharBox.iStyles=LRToStyles();

				LRTCharBox.yInterlinea=0;
				if (sPd.psDi->fGColor) LRTCharBox.lColore=sPd.psDi->lGColor;
									else 
									LRTCharBox.lColore=sPd.psDi->LRFields[a]->lColChar;
				LRTCharBox.yChar=sPd.psDi->iRowHeight;
				  
				//if (ptr) {if (!* (CHAR *) ptr) break;} // Salto i campi vuoti
				if (strEmpty(lpText)) lpText="";
				strcpy(LRTCharBox.szFontName,sPd.psDi->lpFontBodyDefault);

				// Prima di aggiungere l'Idem :
				// A) Verifico se sto stampando in un sistema "dinamico"
				// B) Verifico se è un campo ad altezza "Rows" dinamico
				if (sPd.psDi->bRowsDynamic&&
					sPd.psDi->LRFields[a]->iRowsSize)
				{
					INT yAlt,iFieldBottom;
					INT iRows;
					yAlt=LRGetDispInBoxAlt(&LRTCharBox,lpText,&iRows);
//				win_infoarg("Controllo\n%s, %d, %dpx",lpText,iRows,yAlt);

					//
					// E) SI: Inserisco l'oggetto
					// LREAddItem(LRE_CHARBOX,&LRTCharBox,sizeof(LRTCharBox),lpText,0);
					//
					LREAddLineBuffer(LRE_CHARBOX,&LRTCharBox,sizeof(LRTCharBox),lpText,0);

					//
					// C) Verifico se è POSSIBILE CONTENERE il campo all'interno della pagina
					//
	//				if ((sPd.psDi->iLineCount+iRows+sPd.psDi->LRFields[a]->iLine)>sPd.psDi->iLinePerPage)
					sPd.psDi->iRowLastHeight=yAlt+(sPd.psDi->LRFields[a]->rPadding.top+sPd.psDi->LRFields[a]->rPadding.bottom);
					if (sPd.psDi->iRowLastHeight>sPd.psDi->iRowMaxHeight) 
						sPd.psDi->iRowMaxHeight=sPd.psDi->iRowLastHeight;
					iFieldBottom=sPd.psDi->iRowOffset+sPd.psDi->iRowMaxHeight;
					if (iFieldBottom>sPd.psDi->yBodyBottom)//>=sPd.psDi->yBodyTop;
					{
	//					win_infoarg("qui: %d > %d",iFieldBottom,sPd.psDi->yBodyBottom);
						// D) NO! E che cazzo
						// .1 Cambio pagina
						INT iRowMaxHeight=sPd.psDi->iRowMaxHeight;
						sPd.psDi->iVirtualLineCount=sPd.psDi->iVirtualLinePerPage;
						PowerPrinter(WS_LINK,0,NULL); 

						// .2 Rimappo tutti i componenti inseriti della linea corrente
						LREBufferRemap();
						if (sPd.psDi->iRowPadded>0) sPd.psDi->iRowLastVirtualLine=(yAlt/sPd.psDi->iRowPadded);//iRows+sPd.psDi->LRFields[a]->iLine;

						// Ripristino i valori di altezza di linea
						sPd.psDi->iRowMaxHeight=iRowMaxHeight;


					}
					else
					{
						// F) In entrambi i casi calcolo la dimensione dell'oggetto e la scrivo in ->iRowsLastLine se più grande
						if ((iRows+sPd.psDi->LRFields[a]->iLine)>sPd.psDi->iRowLastVirtualLine)
						{
							sPd.psDi->iRowLastVirtualLine=(yAlt/sPd.psDi->iRowPadded);//iRows+sPd.psDi->LRFields[a]->iLine;
						}
					}
				}
				else
				{
					//LREAddItem(LRE_CHARBOX,&LRTCharBox,sizeof(LRTCharBox),lpText,0);
					LREAddLineBuffer(LRE_CHARBOX,&LRTCharBox,sizeof(LRTCharBox),lpText,0);
				}
			}
			break;
	
	//-----------------------------------------------------------------------
	//	WS_INSERT                       		                            |
	//											                            |
	//	Inserisco i valori caricati nel file di appoggio                    |
	//  info Non usato                                                      |
	//	ptr  Non Usato                                                      |
	//											                            |
	//											                            |
	//	flush del buffer
	//-----------------------------------------------------------------------
		case WS_INSERT:
			
			// Scrivo il buffer nel file
			LREFlushBuffer(TRUE);
			
			// Se esiste un SubPaintProcedure 
			// La chiamo con l'informazione della linea
			if (sPd.psDi->HookSubProc!=NULL)
			{
				LRT_LINK LRALink;
				_(LRTBox);
				//memset(&LRALink,0,sizeof(LRALink)); 
				_(LRALink);
				LRALink.iType=LRA_BODY;
				LRALink.rRect.left=sPd.psDi->rPage.left;
				LRALink.rRect.top=sPd.psDi->iRowOffset;//sPd.psDi->yBodyTop+(sPd.psDi->iLineCount*sPd.psDi->ySectDot);
				LRALink.rRect.right=sPd.psDi->rPage.right;
				LRALink.rRect.bottom=LRALink.rRect.top+sPd.psDi->iRowPadded-1;
				(*sPd.psDi->HookSubProc)(WS_DO,0,&LRALink);
			}

			if (sPd.psDi->bRowsDynamic)
			{
				sPd.psDi->iVirtualLineCount+=sPd.psDi->iRowLastVirtualLine;
				sPd.psDi->iRowOffset+=sPd.psDi->iRowMaxHeight;
//				alert("%d,%d",sPd.psDi->iRowLastVirtualLine,sPd.psDi->iRowLastHeight);
//				sPd.psDi->iRowOffset+=(sPd.psDi->iRowHeight*sPd.psDi->iRowsLastLine)+(sPd.psDi->rFieldPadding.top+sPd.psDi->rFieldPadding.bottom);
			 //sPd.psDi->yCurrentLine+=(sPd.psDi->ySectDot*sPd.psDi->iRowsLastLine);
			}
			else
			{
				sPd.psDi->iVirtualLineCount++;			
				sPd.psDi->iRowOffset+=sPd.psDi->iRowPadded;
			}

			//-----------------------------------------------------------------------
			// Linea orizzontale
			//-----------------------------------------------------------------------
			if (sPd.psDi->fLineHorzField)
			{
				LRLine(	sPd.psDi->rPage.left,sPd.psDi->iRowOffset,
						sPd.psDi->rPage.right,sPd.psDi->iRowOffset,
						sPd.psDi->iLineHorzColor,//LRFields[0]->lColChar,
						1,//(sPd.psDi->iLinePerRiga>1)?2:1,
						sPd.psDi->iLineHorzStyle,//PS_SOLID,
						SET);
			}
		
			//
			// Line verticali
			//
			if (sPd.psDi->fLineVertField&&sPd.psDi->iLinePerRiga>1)
			{
				INT yBase=sPd.psDi->iRowOffset-sPd.psDi->iRowHeight;
				// Linea verticale di sinistra
				_(LRTBox);
				LRTBox.rRect.left=sPd.psDi->rPage.left;
				LRTBox.rRect.right=sPd.psDi->rPage.left;
				LRTBox.rRect.top=yBase;
				LRTBox.rRect.bottom=sPd.psDi->iRowOffset;
				LRTBox.Colore=0;
				LREAddItem(LRE_BOX,&LRTBox,sizeof(LRTBox),0,0);

				for (a=0;a<sPd.psDi->iFieldNum;a++)
				{
					// Linea verticale
					_(LRTBox);
					LRTBox.rRect.left=sPd.psDi->LRFields[a]->xPosEndDot;
					LRTBox.rRect.right=sPd.psDi->LRFields[a]->xPosEndDot;
					LRTBox.rRect.top=yBase+(sPd.psDi->LRFields[a]->iLine*sPd.psDi->iRowHeight);
					LRTBox.rRect.bottom=LRTBox.rRect.top+(sPd.psDi->iRowHeight*sPd.psDi->LRFields[a]->iRowsSize)-1;
					LRTBox.Colore=0;
					LREAddItem(LRE_BOX,&LRTBox,sizeof(LRTBox),0,0);
					
					if (sPd.psDi->fLineHorzField&&(sPd.psDi->LRFields[a]->iLine<(sPd.psDi->iLinePerRiga-1)))
					{
					 // Linea orizzontale
					 _(LRTBox);
					 LRTBox.rRect.left=sPd.psDi->LRFields[a]->xPosStartDot;
					 LRTBox.rRect.right=sPd.psDi->LRFields[a]->xPosEndDot;
					 //TBox.rRect.top=sPd.psDi->yCurrentLine+(sPd.psDi->LRFields[a]->iLine*sPd.psDi->yChar);
					 LRTBox.rRect.top=LRTBox.rRect.bottom=yBase+(sPd.psDi->LRFields[a]->iLine*sPd.psDi->iRowHeight)+(sPd.psDi->iRowHeight*sPd.psDi->LRFields[a]->iRowsSize)-1;
					 LRTBox.Colore=0;
					 LREAddItem(LRE_BOX,&LRTBox,sizeof(LRTBox),0,0);
					}
				}
			}
			break;

	}
 return NULL;
}

// -------------------------------------------------------------------------------
// SERVICE FUNCTION FOR PRINTER                                                  |
// -------------------------------------------------------------------------------

BOOL bUserAbort;
HWND hDlgPrint=NULL;

#ifndef _CONSOLE

static BOOL CALLBACK PrintDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
     {
     switch (msg)
          {
          case WM_INITDIALOG :
               EnableMenuItem (GetSystemMenu(hDlg, FALSE), SC_CLOSE,MF_GRAYED) ;
               return TRUE ;

          case WM_COMMAND :
               bUserAbort = TRUE ;
               EnableWindow (GetParent (hDlg), TRUE) ;
               DestroyWindow (hDlg) ;
               hDlgPrint = 0 ;
               return TRUE ;
          }
     return FALSE ;
     }          

#endif
static BOOL CALLBACK AbortProc (HDC hPrinterDC, INT iCode)
     {
     MSG msg ;

     while (!bUserAbort && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
          {
          if (!hDlgPrint || !IsDialogMessage (hDlgPrint, &msg))
               {
				TranslateMessage (&msg);
				DispatchMessage (&msg);
				   
               }
          }
     return !bUserAbort ;
     }

// -------------------------------------------------------------------------------
// PRINT CONSOLE PREVIEW                                                         |
// -------------------------------------------------------------------------------

static LRESULT CALLBACK LayProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static HWND   LayWnd=NULL;
static INT   LREOwner;
static HWND   OwnerWnd=NULL;
static RECT   LayClient;
static UINT   yBarra=45;         // Spesso TOOLBAR in alto
//static UINT   yClient,xClient;   // Altezza del layout Video
static UINT   yBorder,xBorder;   // Margini
//static UINT   VLx,VLy;         // Altezza della Pagina nel Video Virtuale
static UINT   xPos,yPos;      // Posizione su video della Pagina virtuale
//static UINT   yLChar,xLChar;   // Dimensioni del carattere stabilito
static SHORT  LayOrientation;
//static UINT   xPage,yPage;

//static BYTE   *lpData;


static INT hdlFileTemp=0; // L'handle del file in memoria
static INT   iLRE_Elements;
static LRETAG **arElement=NULL;
static INT   *arPageIndex=NULL;
static INT   iPageView;
static HFONT  hFontBase=NULL;
static INT iZoom=0;
static TLAYOUT TLayOut;
static POINT VideoOffset;


//
// _LReposContext(
//
static void _LReposContext(BOOL bPrinter,
						   RECT *prcSrc,	// Posizione dell'oggetto originale
						   RECT *prcDest,	// Posizione ricalcolata
						   SIZE *psizDest) {// Dimensione ricalcolata

	if (!bPrinter) 
	{
		prcDest->left=DotToVideoX(prcSrc->left);	prcDest->top=DotToVideoY(prcSrc->top);
		prcDest->right=DotToVideoX(prcSrc->right);	prcDest->bottom=DotToVideoY(prcSrc->bottom);
	}
	else
	{
		memcpy(prcDest,prcSrc,sizeof(RECT));
	}

	sizeCalc(psizDest,prcDest);
}




// Visualizza una stringa pacchetizzata come LRT_CHAREX
static void LDispLex(BOOL Printer,HDC hDC,LRT_CHAREX *Lex,CHAR *pText,RECT *lpRect)
{
	HFONT hFont,OldFont; 
	INT  yChar,xChar;
	INT x,y;
	SIZE SizeEx;
//	CHAR *NewText;
	WCHAR *pwText;
	INT iLength;

	if (Printer)
	{
		xChar=Lex->xChar;
		yChar=Lex->yChar;
		x=Lex->Point.x;
		y=Lex->Point.y;
	}
	else
	{
		x=DotToVideoX(Lex->Point.x);
		y=DotToVideoY(Lex->Point.y);
		xChar=DotToPixelX(Lex->xChar);
		yChar=DotToPixelY(Lex->yChar);
	}

	if (yChar<1) return;
	pwText=LRDecode(pText);
	hFont=CreateFont(yChar, // Altezza del carattere
				  xChar, // Larghezza del carattere (0=Default)
				  0, // Angolo di rotazione x 10
				  0, //  Angolo di orientamento bo ???
	/*
				  Lex->fBold ? FW_BOLD : 0, // Spessore del carattere (MACRO Ex: FW_BOLD)
				  Lex->fItalic, // Flag Italico    ON/OFF
				  Lex->fUnderLine, // Flag UnderLine  ON/OFF
				  */

				  (Lex->iStyles&STYLE_BOLD)?FW_BOLD:0,//sPd.psDi->fBold;//sys.fFontBold;
				  (Lex->iStyles&STYLE_ITALIC)?1:0,//sPd.psDi->fItalic;//sys.fFontItalic;
				  (Lex->iStyles&STYLE_UNDERLINE)?1:0,//sPd.psDi->fUnderLine;//sys.fFontItalic;					 

				  0, // Flag StrikeOut  ON/OFF
				  DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
	//				  FERRA_OUTPRECISION, // Output precision
				  OUT_DEVICE_PRECIS,//OUT_DEFAULT_PRECIS, // Output precision
				  0, // Clipping Precision
				  PROOF_QUALITY,//DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
				  VARIABLE_PITCH,//DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
				  //0,
				  //"Arial"); // Nome del font
				  Lex->szFontName); // Nome del font "Courier New"

	OldFont=SelectObject(hDC, hFont);
	SetBkMode(hDC,TRANSPARENT); 
	SetTextColor(hDC,Lex->lColore);
	SetMapMode(hDC, MM_TEXT);
	SetTextCharacterExtra(hDC,1); // <--- Cambiato nel 2009
	iLength=wcslen(pwText);
	// Allineamento
	GetTextExtentPoint32W(hDC, pwText, iLength,&SizeEx);
	switch (Lex->xAllinea)
	{
	case LR_LEFT: 
		SetTextAlign(hDC,TA_LEFT);
		break;

	case LR_RIGHT: 
		SetTextAlign(hDC,TA_RIGHT);
	//		x-=SizeEx.cx+1; 
		break;
	case LR_CENTER: 
		SetTextAlign(hDC,TA_CENTER);
		break;
	//		x-=(SizeEx.cx>>1);
	//		break;
	//		return;
	}

	// Se esiste un range
	// Controllo se il Box da fare è nel range richiesto
	if (lpRect)
	{
	 if (((x+SizeEx.cx-1)<lpRect->left)||
		  (x>lpRect->right)||
		  (y>lpRect->bottom)||
		  ((y+yChar-1)<lpRect->top)) {} else TextOutW(hDC,x,y, pwText,iLength);
	} 
	else 
	 TextOutW(hDC,x,y, pwText,iLength);

	SelectObject(hDC, OldFont);
	DeleteObject(hFont);
	ehFree(pwText);
}

// Visualizza una stringa pacchetizzata come LRT_CHARJUST
/*
static void LDispJust(BOOL Printer,HDC hDC,LRT_CHARJUST *LCharJust,CHAR *ptext,RECT *lpRect)
{
 HFONT hFont,OldFont; 
 INT  yChar,xChar;
 CHAR *NewText;
 RECT Rect;
 INT iInterLinea;

 if (Printer)
 {
  xChar=LCharJust->xChar;
  yChar=LCharJust->yChar;
  memcpy(&Rect,&LCharJust->rArea,sizeof(RECT));
  iInterLinea=LCharJust->yInterlinea;
 }
 else
 {
  // Vlx:xPage:x:yChar;
  //yChar=Lex->yChar*VLy/yPage;
  //xChar=Lex->xChar*VLx/xPage;
  Rect.left=DotToVideoX(LCharJust->rArea.left);
  Rect.right=DotToVideoX(LCharJust->rArea.right);
  Rect.top=DotToVideoY(LCharJust->rArea.top);
  Rect.bottom=DotToVideoY(LCharJust->rArea.bottom);
  xChar=DotToPixelX(LCharJust->xChar);
  yChar=DotToPixelY(LCharJust->yChar);
  iInterLinea=DotToPixelY(LCharJust->yInterlinea);
 }

 if (yChar<1) return;
 NewText=ehAlloc(strlen(ptext)+1);
 if (sys.OemTranslate) OemToChar(ptext,NewText); else strcpy(NewText,ptext);

 hFont=CreateFont(yChar, // Altezza del carattere
				  xChar, // Larghezza del carattere (0=Default)
				  0, // Angolo di rotazione x 10
				  0, //  Angolo di orientamento bo ???
				  LCharJust->fBold ? FW_BOLD : 0, // Spessore del carattere (MACRO Ex: FW_BOLD)
				  LCharJust->fItalic, // Flag Italico    ON/OFF
				  LCharJust->fUnderLine, // Flag UnderLine  ON/OFF
				  0, // Flag StrikeOut  ON/OFF
				  DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
//				  FERRA_OUTPRECISION, // Output precision
				  OUT_DEFAULT_PRECIS, // Output precision
				  0, // Clipping Precision
				  DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
				  DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
				  //0,
				  //"Arial"); // Nome del font
				  LCharJust->szFontName); // Nome del font "Courier New"

 OldFont=SelectObject(hDC, hFont);
 SetBkMode(hDC,TRANSPARENT); 
 SetTextColor(hDC,LCharJust->lColore);
 SetMapMode(hDC, MM_TEXT);

 // Allineamento
 //GetTextExtentPoint32(hDC, NewText, strlen(NewText),&SizeEx);
 
 // Se esiste un range
 // Controllo se il Box da fare è nel range richiesto
 if (lpRect)
 {
	 if ((Rect.right<lpRect->left)||
	      (Rect.left>lpRect->right)||
	      (Rect.top>lpRect->bottom)||
		  (Rect.bottom<lpRect->top)) {} else LTextJustify(hDC,NewText,&Rect,iInterLinea,TRUE);
 } else LTextJustify(hDC,NewText,&Rect,iInterLinea,TRUE);

 SelectObject(hDC, OldFont);
 DeleteObject(hFont);

 ehFree(NewText);
}
*/
// Visualizza una stringa pacchetizzata come LRT_CHARJUST
static void LDispCharInBox(BOOL Printer,HDC hDC,LRT_CHARBOX *lpLCharBox,CHAR *pbText,RECT *lpRect)
{
	HFONT hFont,OldFont; 
	LRT_CHARBOX LCharBox;
	BOOL fView;
	INT iRows;

	memcpy(&LCharBox,lpLCharBox,sizeof(LRT_CHARBOX));
	if (!Printer)
	{
		// Vlx:xPage:x:yChar;
		LCharBox.rArea.left=DotToVideoX(lpLCharBox->rArea.left);
		LCharBox.rArea.right=DotToVideoX(lpLCharBox->rArea.right);
		LCharBox.rArea.top=DotToVideoY(lpLCharBox->rArea.top);
		LCharBox.rArea.bottom=DotToVideoY(lpLCharBox->rArea.bottom);
		LCharBox.xChar=DotToPixelX(lpLCharBox->xChar);
		LCharBox.yChar=DotToPixelY(lpLCharBox->yChar);
		LCharBox.yInterlinea=DotToPixelY(lpLCharBox->yInterlinea);
	}

	if (LCharBox.yChar<1) return;
//	NewText=ehAlloc(strlen(ptext)+1);
//	if (sys.bOemString) OemToChar(ptext,NewText); else strcpy(NewText,ptext);

	hFont=CreateFont(LCharBox.yChar, // Altezza del carattere
				  LCharBox.xChar, // Larghezza del carattere (0=Default)
				  0, // Angolo di rotazione x 10
				  0, //  Angolo di orientamento bo ???
	/*
				  lpLCharBox->fBold ? FW_BOLD : 0, // Spessore del carattere (MACRO Ex: FW_BOLD)
				  lpLCharBox->fItalic, // Flag Italico    ON/OFF
				  lpLCharBox->fUnderLine, // Flag UnderLine  ON/OFF
				  */
				  (lpLCharBox->iStyles&STYLE_BOLD)?FW_BOLD:0,//sPd.psDi->fBold;//sys.fFontBold;
				  (lpLCharBox->iStyles&STYLE_ITALIC)?1:0,//sPd.psDi->fItalic;//sys.fFontItalic;
				  (lpLCharBox->iStyles&STYLE_UNDERLINE)?1:0,//sPd.psDi->fUnderLine;//sys.fFontItalic;					 

				  0, // Flag StrikeOut  ON/OFF
				  DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
				  OUT_DEFAULT_PRECIS, // Output precision
				  0, // Clipping Precision
				  DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
				  DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
				  //0,
				  //"Arial"); // Nome del font
				  lpLCharBox->szFontName); // Nome del font "Courier New"
 
	OldFont=SelectObject(hDC, hFont);
	SetBkMode(hDC,TRANSPARENT); 
	SetTextColor(hDC,lpLCharBox->lColore);
	SetMapMode(hDC, MM_TEXT);
	SetTextCharacterExtra(hDC,lpLCharBox->iExtraCharSpace);
	SetTextAlign(hDC,TA_LEFT);

	fView=TRUE;
	if (fView) LTextInBox(hDC,&LCharBox,pbText,TRUE,&iRows,lpLCharBox->iMaxRows);			

	SelectObject(hDC, OldFont);
	DeleteObject(hFont);
}

static void xyConvert(INT cmd,void *lpDato)
{
	LONG *lpLong=lpDato;
	RECT *lpRect=lpDato;

	switch (cmd)
	{
		case LR_INCHTODOTX:  
			if (!*lpLong) return; 
			*lpLong=LRInchToDotX(*lpLong); 
			break;

		case LR_INCHTODOTY:  
			if (!*lpLong) return; 
			*lpLong=LRInchToDotY(*lpLong); 
			break;

		case LR_DOTTOVIDEOX: 
			if (!*lpLong) return; 
			*lpLong=DotToVideoX(*lpLong); 
			break;

		case LR_DOTTOVIDEOY: 
			if (!*lpLong) return; 
			*lpLong=DotToVideoY(*lpLong); 
			break;

		case LR_DOTTOPIXELX: 
			if (!*lpLong) return; 
			*lpLong=DotToPixelX(*lpLong); 
			break;

		case LR_DOTTOPIXELY: 
			if (!*lpLong) return; 
			*lpLong=DotToPixelY(*lpLong); 
			break;

        case LR_RECTTOPIXEL: 
			  // x:ViewPage.x=Lex->Point.x:xPageDot
			  if (lpRect->left)  lpRect->left=DotToVideoX(lpRect->left);
								    else
								    lpRect->left=sPd.rVideoPrintArea.left+VideoOffset.x;
			  if (lpRect->right) lpRect->right=DotToVideoX(lpRect->right);
									else
									lpRect->right=sPd.rVideoPrintArea.left+VideoOffset.x;
			  if (lpRect->top)	lpRect->top=DotToVideoY(lpRect->top);
									else
									lpRect->top=sPd.rVideoPrintArea.top+VideoOffset.y;

			  if (lpRect->bottom) lpRect->bottom=DotToVideoY(lpRect->bottom); 
									 else
									 lpRect->bottom=sPd.rVideoPrintArea.top+VideoOffset.y;
			  break;
	}

}

//static void _ImgOutEx(HDC hDC,INT PosX,INT PosY,INT SizeX,INT SizeY,INT OfsX,INT OfsY,INT Hdl);
static void _ImgOutEx(HDC hDC,INT PosX,INT PosY,INT SizeX,INT SizeY,INT Hdl);
static void LMultiBox(BOOL Printer,HDC hDC,INT iType,LRT_BOXR *Box,RECT *lpRect)
{
	LONG lColor;
	LONG lRoundWidth,lRoundHeight,iPenWidth;
	HPEN hpen, hpenOld;
	HBRUSH hbrOld;
	HBRUSH hbr;
	RECT Rect;

	lRoundWidth=Box->lRoundWidth; 
	lRoundHeight=Box->lRoundHeight;
	iPenWidth=Box->iPenWidth;
	memcpy(&Rect,&Box->rRect,sizeof(Rect));
	if ((lRoundWidth==-1) && (lRoundHeight==-1))
	{
		lRoundWidth=((Rect.right-Rect.left)/4);
		lRoundHeight=((Rect.bottom-Rect.top)/4);
		if (lRoundWidth<lRoundHeight)
			lRoundHeight=lRoundWidth;
		else if (lRoundHeight<lRoundWidth)
			lRoundWidth=lRoundHeight;
	}

	if (!Printer)
	{
		xyConvert(LR_RECTTOPIXEL,&Rect);
		xyConvert(LR_DOTTOVIDEOX,&lRoundWidth);
		xyConvert(LR_DOTTOVIDEOY,&lRoundHeight);
		xyConvert(LR_DOTTOPIXELX,&iPenWidth);
		//if (iPenWidth) iPenWidth=DotToPixelX(DotToPixelX);
		//win_infoarg("A) iPenWidth=%d",iPenWidth);
		//win_infoarg("B) iPenWidth=%d",iPenWidth);
	}

	// Se esiste un range
	// Controllo se il Box da fare è nel range richiesto
	if (lpRect)
	{
		if ((Rect.right<lpRect->left)||
			(Rect.left>lpRect->right)||
			(Rect.top>lpRect->bottom)||
			(Rect.bottom<lpRect->top)) return;
	}


	switch (iType)
	{
		case LRE_BOX:
			//if (!Printer) xyConvert(LR_DOTTOPIXELX,&lWidth);
			hpen = CreatePen(PS_SOLID, iPenWidth, Box->lColore); // era 1
			hpenOld = SelectObject(hDC, hpen);
			hbrOld = SelectObject(hDC,GetStockObject(NULL_BRUSH));
			Rectangle (hDC,Rect.left,Rect.top,Rect.right,Rect.bottom);
			SelectObject(hDC, hpenOld);
			SelectObject(hDC, hbrOld);
			DeleteObject(hpen); // Cancella la penna usata
			break;

		case LRE_BOXP: 
			lColor=Box->lColore;
			if (!Printer)
			{
				if (Rect.top==Rect.bottom) 
					Rect.bottom++; 
			}

			hpen=CreatePen(PS_INSIDEFRAME, 0, lColor); // era 1 Creo un nuova penna
			hpenOld=SelectObject(hDC, hpen);
			if (!lColor)
			{
				hbrOld=SelectObject(hDC,GetStockObject(BLACK_BRUSH));
				Rectangle(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom);
				SelectObject(hDC, hbrOld);
			}
			else
			{
				//CMYK(255,0,0,0);
				//COLORREF
				//hbr=CreateSolidBrush(CMYK(255,0,0,0));        // Creo un nuovo pennello

				hbr=CreateSolidBrush(lColor);        // Creo un nuovo pennello
				hbrOld=SelectObject(hDC, hbr);
				Rectangle(hDC, Rect.left, Rect.top, Rect.right, Rect.bottom);
				SelectObject(hDC, hbrOld);
				DeleteObject(hbr);
			}

			SelectObject(hDC, hpenOld);
			DeleteObject(hpen);
			break;

		case LRE_BOXR:  
			hpen = CreatePen(PS_SOLID, iPenWidth, Box->lColore); // era 1
			hpenOld = SelectObject(hDC, hpen);
			hbrOld = SelectObject(hDC,GetStockObject(NULL_BRUSH));
			RoundRect (hDC,Rect.left,Rect.top,Rect.right,Rect.bottom,lRoundWidth,lRoundHeight);
			SelectObject(hDC, hpenOld);
			SelectObject(hDC, hbrOld);
			DeleteObject(hpen); // Cancella la penna usata
			break;

		case LRE_BOXPR: 
			lColor=Box->lColore;
			if (!Printer)
			{
				if (Rect.top==Rect.bottom) 
					Rect.bottom++; 
			}

			hpen=CreatePen(PS_INSIDEFRAME, 0, lColor); // era 1 Creo un nuova penna
			hpenOld=SelectObject(hDC, hpen);
			if (!lColor)
			{
				hbrOld=SelectObject(hDC,GetStockObject(BLACK_BRUSH));
				RoundRect (hDC,Rect.left,Rect.top,Rect.right,Rect.bottom,lRoundWidth,lRoundHeight);
				SelectObject(hDC, hbrOld);
			}
			else
			{
				hbr=CreateSolidBrush(lColor);        // Creo un nuovo pennello
				hbrOld=SelectObject(hDC, hbr);
				RoundRect (hDC,Rect.left,Rect.top,Rect.right,Rect.bottom,lRoundWidth,lRoundHeight);
				SelectObject(hDC, hbrOld);
				DeleteObject(hbr);
			}

			SelectObject(hDC, hpenOld);
			DeleteObject(hpen);
			break;

	}
}

static void LMultiLine(BOOL Printer,HDC hDC,INT iType,LRT_LINE *Line,RECT *lpRect)
{
 RECT Rect;
 HPEN hpen, hpenOld;
 LOGBRUSH sLogBrush;
 INT OldROP,iPenWidth;
 memcpy(&Rect,&Line->rRect,sizeof(Rect));

 iPenWidth=Line->iPenWidth; 
 if (!Printer) 
 {
	 xyConvert(LR_RECTTOPIXEL,&Rect);
	 xyConvert(LR_DOTTOPIXELX,&iPenWidth);
//	 win_infoarg("A) iPenWidth=%d",iPenWidth);
//	 if (iPenWidth) iPenWidth=DotToPixelX(DotToPixelX);
//	 win_infoarg("B) iPenWidth=%d",iPenWidth);
 }

 // Se esiste un range
 // Controllo se il Box da fare è nel range richiesto
 if (lpRect)
 {
	if ((Rect.right<lpRect->left)||
	    (Rect.left>lpRect->right)||
	    (Rect.top>lpRect->bottom)||
	    (Rect.bottom<lpRect->top)) return;
 }

 switch (iType)
 {
	
	case LRE_LINE:  

		//Tbox(Rect.left,Rect.top,Rect.right,Rect.bottom,Box->Colore,SET); 
		if (Line->iMode==XOR) OldROP=SetROP2(hDC,R2_XORPEN);

		if (iPenWidth>1)
		{
			_(sLogBrush);
			sLogBrush.lbStyle=BS_SOLID;
			sLogBrush.lbColor=Line->Colore;
			sLogBrush.lbHatch=HS_BDIAGONAL;
			hpen = ExtCreatePen(PS_GEOMETRIC|Line->iStyle|PS_ENDCAP_ROUND, iPenWidth, &sLogBrush,Line->Colore ,NULL); // era 1
		}
		else
		{
			hpen = CreatePen(Line->iStyle, iPenWidth, Line->Colore); // era 1
		}
		hpenOld = SelectObject(hDC, hpen);

		
		MoveToEx(hDC,Rect.left,Rect.top,NULL); // Cambiare NULL
		LineTo(hDC,Rect.right,Rect.bottom); 
		SelectObject(hDC, hpenOld);
		DeleteObject(hpen); // Cancella la penna usata
		if (Line->iMode==XOR) SetROP2(hDC,OldROP);
		break;
 }
}

static void LIMGDispExDir(HDC hDC,INT PosX,INT PosY,INT HdlSource);
static void _ImgOut(BOOL bPrinter,HDC hDC,INT iType,LRT_BOX *psBox,RECT *lpRect)
{
//	INT x,y,xDim,yDim;
	INT hdlImage=psBox->Colore;
	RECT rcArea;
	SIZE sizArea;
/*

	x=Box->rRect.left;  y=Box->rRect.top;
	xDim=Box->rRect.right; yDim=Box->rRect.bottom;
	if (!Printer) 
	{
		x=DotToVideoX(x); 
		y=DotToVideoY(y);
		if (xDim) xDim=DotToPixelX(xDim); 
		if (yDim) yDim=DotToPixelY(yDim);
	}
	*/
	_LReposContext(bPrinter,&psBox->rRect,&rcArea,&sizArea);
	if (!psBox->rRect.right) sizArea.cx=0;
	if (!psBox->rRect.bottom) sizArea.cy=0;
	switch (iType)
	{
		case LRE_EHIMG:  
			_ImgOutEx(hDC, rcArea.left, rcArea.top, sizArea.cx, sizArea.cy,hdlImage);
			break;

		case LRE_EHIMGD: 
			//LIMGDispExDir(hDC, x, y, hdlImage); 
//			dcIMGDisp(hDC,rcArea.left,rcArea.top,hdlImage);
			break;
	}
}


/*
//
// _LBitmapOutput()
//
static void _LBitmapOutput(BOOL bPrinter,HDC hdcDest,RECT *prcRepaint,RECT *prcPos,HBITMAP hBitmap,RECT *psRecSource)
{
	RECT	rcArea;
	SIZE	sizArea;
	HDC		hdcSource;
	POINT	ptSource;
	SIZE	sizSource;
	BITMAP  sInfo;

	_LReposContext(bPrinter,prcPos,&rcArea,&sizArea);
	if (!psRecSource) {
		GetObject( hBitmap,sizeof( BITMAP ), &sInfo);
		_(ptSource);
		sizSource.cx=sInfo.bmWidth;
		sizSource.cy=sInfo.bmHeight;
	}
	else {
		ptSource.x=psRecSource->left;
		ptSource.y=psRecSource->right;
		sizeCalc(&sizSource,psRecSource);
	}

	hdcSource=CreateCompatibleDC(0);
	SelectObject(hdcSource, hBitmap); // assign the dib section to the dc
	StretchBlt(	hdcDest, 
				rcArea.left,rcArea.top,
				sizArea.cx,sizArea.cy,
				
				hdcSource,
				ptSource.x,ptSource.y,
				sizSource.cx,sizSource.cy,
				SRCCOPY);
	DeleteDC(hdcSource);
}
	*/

//
// _LImageListOutput()
//
static void _LImageListOutput(BOOL bPrinter,HDC hdcDest,RECT *prcRect,LRT_IMAGE *psImage)
{
	IMAGEINFO sImageInfo;
	RECT rcArea;
	SIZE sizArea;

	_LReposContext(bPrinter,&psImage->recImage,&rcArea,&sizArea);

	/*
	ImageList_DrawEx(	psImage->himl,
						psImage->imgNumber,
						hdcDest,
						rcArea.left,rcArea.top,
						sizArea.cx,sizArea.cy,
						CLR_DEFAULT,CLR_DEFAULT,ILD_NORMAL); // Questo funziona MALE
	*/

	//	ImageList_DrawEx(psImage->himl,psImage->imgNumber,hDC,x,y,17,16,CLR_DEFAULT,CLR_DEFAULT,ILD_NORMAL); // Questo funziona
	if (ImageList_GetImageInfo(psImage->himl,psImage->imgNumber,&sImageInfo))
	{
		INT hdlImage=BitmapToImg(sImageInfo.hbmImage,TRUE,&sImageInfo.rcImage,IMG_PIXEL_RGB); // Estraggo pezzo di immagine (sarebbe da fare direttamente bitmp)
		dcImageShow(hdcDest,rcArea.left,rcArea.top,sizArea.cx,sizArea.cy,hdlImage);

		//HBITMAP hBitPiece=ImgToBitmap(hdlImage,NULL,0); // Converto in bitmap
		//dcBmpDisp(hdcDest,rcArea.left,rcArea.top,sizArea.cx,sizArea.cy,sImageInfo.hbmImage); // Stampo "zoomando"

		// Libero le risorse
		memoFree(hdlImage,"memo"); 
//		DeleteObject(hBitPiece);
	}

}


/*
static void LBmpDispDirect(HDC hDC,
						   HDC hDCSource,
						   //HBITMAP BitMap,
				           INT PosX,INT PosY,
				           INT Lx,INT Ly,
				           INT LxNew,INT LyNew,
				           BOOL Ridim); // Ridimensiona
*/

static void _ImgOutEx(HDC hDC,
					   INT PosX,INT PosY,
					   INT SizeX,INT SizeY,
					   INT hdlImage)
{
	IMGHEADER *psImg;
	WORD    a=1;
	BITMAPINFOHEADER *psBmpHeader;
	BITMAPINFO *psBmpInfo;
	INT TileX=256;// Dimensione piastrella orizzontale (Da Fare)
	INT TileY=0;

	LONG Lx,Ly;
//	INT HdlImageNew;
 
	psImg=memoLock(hdlImage);
	psBmpInfo=(BITMAPINFO *) &psImg->bmiHeader;
	psBmpHeader=(BITMAPINFOHEADER *) &psImg->bmiHeader;

	// -------------------------------------
	// CALCOLO LE DIMENSIONI DEL NUOVO BITMAP
	// -------------------------------------

	Ly=psBmpHeader->biHeight; Lx=psBmpHeader->biWidth;

	// Calcolo della piastrella
	// Inversamente proporzionale : y:1024=TileY:TileX;
	//if (Lx>TileX) {TileX=Lx; TileY=TileY*1024/TileX;}

	// Calcolo automatico delle dimensioni orizzontali
	//  SizeY:Ly=x:Lx;
	if ((SizeY>0)&&(SizeX==0)) SizeX=SizeY*Lx/Ly;
	// Calcolo automatico delle dimensioni verticali
	if ((SizeX>0)&&(SizeY==0)) SizeY=SizeX*Ly/Lx;
	memoUnlock(hdlImage);
	if (SizeX<1||SizeY<1) return;
  
	//
	// Creo il nuovo bitmap
	//

	//dcImageShow(hDC,PosX,PosY,hdlImage);//HdlImageNew);
	{
		INT hdlImageNew;

		__try {

			hdlImageNew=IMGResampling(hdlImage,  // Handle dell'immagine
										NULL,
										SizeX,SizeY, 
										TRS_LANCZOS);
			if (hdlImageNew>1) {
 				dcImageShow(hDC,PosX,PosY,0,0,hdlImageNew);//HdlImageNew);
				memoFree(hdlImageNew,"LREPIMG");
			}

		} __except(EXCEPTION_EXECUTE_HANDLER) {
	
			int iError=GetExceptionCode();
			memoUnlock(hdlImage);
			printf("qui %x",iError);
		}


	}
}

	/*
	TileX=SizeX;
	// TileY=(0x100000)/TileX; // Calcolo la dimensione del Tile
	TileY=Ly;
	if (TileY<3) TileY=2;

	//BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
	memcpy(&BmpBackup,(BITMAPINFO *) &psImg->bmiHeader,sizeof(BITMAPINFO));
	BmpInfo=(BITMAPINFO *) &BmpBackup;
	BmpHeader=(BITMAPINFOHEADER *) &BmpBackup;
	Ly=BmpHeader->biHeight; if (Ly<0) Ly=-BmpHeader->biHeight;
	Lx=BmpHeader->biWidth;
//	Sorg=(BYTE *) Img;
//	Sorg+=Img->Offset;
 
	// Loop sui "tile" per copiarlo
	WritePy=0; 
	TileSizeY=Img->linesize*TileY;

	WritePy=Ly;
	for (ReadPy=0;;ReadPy+=TileY)   // Loop Verticale
	{
		ReadSectY=(Ly-ReadPy); 
		if (ReadSectY>TileY) ReadSectY=TileY; 
		if (ReadSectY<1) break;		  // Fine del file

		BmpHeader->biHeight=ReadSectY;
		BmpHeader->biWidth=TileX;
		if (StretchDIBits(hDC, 
						  PosX,PosY+WritePy-ReadSectY,
						  TileX,ReadSectY,
						  0,0,
						  TileX,ReadSectY,
						  Img->pbImage,
						  BmpInfo,
						  DIB_RGB_COLORS, 
						  SRCCOPY) == GDI_ERROR) 
		{
			ehExit("_ImgOutEx():StretchDIBits Failed"); 
		}

		// E) Cancello DC e Bitmap creati
		WritePy-=ReadSectY;
		Sorg+=TileSizeY;
	}

	memoUnlock(HdlImageNew);
	memoFree(HdlImageNew,"LREPIMG");
}
*/
// Stampa diretta senza straching (DA FARE)
// Per migliorare la stampa di loghi e senza colore
static void LIMGDispExDir(HDC hDC,INT PosX,INT PosY,INT HdlSource)
{
/*
  IMGHEADER *Img;
  WORD    a=1;
  BITMAPINFO BmpBackup;
  BITMAPINFOHEADER *BmpHeader;
  BITMAPINFO *BmpInfo;
  BYTE *Sorg;
  INT TileX=256;// Dimensione piastrella orizzontale (Da Fare)
  INT TileY=0;

  LONG Lx,Ly;
  INT ReadPy,WritePy; 
  INT ReadSectY; 
  LONG TileSizeY;
  HWND hWndRif=NULL;
  INT HdlImageNew;
  INT xScan;
 
  Img=memoLock(HdlSource);
  BmpInfo=(BITMAPINFO *) &Img->bmiHeader;
  BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;

  // -------------------------------------
  // CALCOLO LE DIMENSIONI DEL NUOVO BITMAP
  // -------------------------------------
 
  Ly=BmpHeader->biHeight;
  Lx=BmpHeader->biWidth;
  xScan=Img->linesize;

  // Calcolo della piastrella
  // Inversamente proporzionale : y:1024=TileY:TileX;
  //if (Lx>TileX) {TileX=Lx; TileY=TileY*1024/TileX;}
  
  // Calcolo automatico delle dimensioni orizzontali
  //  SizeY:Ly=x:Lx;
  if ((SizeY>0)&&(SizeX==0)) SizeX=SizeY*Lx/Ly;
  // Calcolo automatico delle dimensioni verticali
  if ((SizeX>0)&&(SizeY==0)) SizeY=SizeX*Ly/Lx;

  memoUnlock(HdlSource);
  
  // Creo il nuovo bitmap
  HdlImageNew=IMGRemaker(HdlSource,SizeX,SizeY,TRUE);
  if (HdlImageNew<1) ehExit("_ImgOutEx():Non memory");
  IMGTopdown(HdlImageNew);
  Img=memoLock(HdlImageNew);

  TileX=SizeX;
  TileY=(0x100000)/TileX; // Calcolo la dimenzione del Tile
//  TileY=(0x8000)/TileX; // Calcolo la dimensione del Tile
  if (TileY<3) TileY=2;

  //BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
  memcpy(&BmpBackup,(BITMAPINFO *) &Img->bmiHeader,sizeof(BITMAPINFO));
  BmpInfo=(BITMAPINFO *) &BmpBackup;
  BmpHeader=(BITMAPINFOHEADER *) &BmpBackup;
  Ly=BmpHeader->biHeight; if (Ly<0) Ly=-BmpHeader->biHeight;
  Lx=BmpHeader->biWidth;
  Sorg=(BYTE *) Img;
  Sorg+=Img->Offset;
 
  // Loop sui "tile" per copiarlo
  WritePy=0; 
  TileSizeY=Img->linesize*TileY;

  WritePy=Ly;
  for (ReadPy=0;;ReadPy+=TileY)   // Loop Verticale
  {
	ReadSectY=(Ly-ReadPy); 
	if (ReadSectY>TileY) ReadSectY=TileY; 
	if (ReadSectY<1) break;		  // Fine del file

	BmpHeader->biHeight=ReadSectY;
	BmpHeader->biWidth=TileX;
    if (StretchDIBits(hDC, 
					  PosX,PosY+WritePy-ReadSectY,
					  TileX,ReadSectY,
					  0,0,
					  TileX,ReadSectY,
					  Sorg,
					  BmpInfo,
					  DIB_RGB_COLORS, 
					  SRCCOPY) == GDI_ERROR) 
    {
        ehExit("_ImgOutEx():StretchDIBits Failed"); 
    }

	// E) Cancello DC e Bitmap creati
	WritePy-=ReadSectY;
	Sorg+=TileSizeY;
  }

  memoUnlock(HdlImageNew);
  memoFree(HdlImageNew,"LREPIMG");
*/
}
static void LRFillPage(BOOL fPrinter,HDC hDC,INT nPage,RECT *lpRect)
{
  INT a;
  BYTE *ptr;
  LRT_LINKEXTERN LRALinkExt;

  _(LRALinkExt);
  LRALinkExt.hDC=hDC;
  LRALinkExt.fPrinter=fPrinter;
  LRALinkExt.xyConvert=xyConvert;
  //yPos=-TLayOut.iVScrollPos*TLayOut.CharY;
  for (a=arPageIndex[nPage];a<iLRE_Elements;a++)
  {
	if (arElement[a]->iPage!=(nPage+1)) break;
	 
	 ptr=(BYTE *) arElement[a]; // Puntatore al Tag
	 ptr+=sizeof(LRETAG);
	 switch (arElement[a]->iType)
	 {

		case LRE_CHAR: 
		         {
				   LRT_CHAREX Lex;
				   CHAR *szString;
				 
				   szString=ptr+sizeof(LRT_CHAR);
				   memcpy(&Lex,ptr,sizeof(LRT_CHAR));
				   strcpy(Lex.szFontName,sPd.psDi->lpFontBodyDefault);
				   LDispLex(fPrinter,hDC,&Lex,szString,lpRect);
				 }
				  break;

		case LRE_CHAREX: 
		         {
				   CHAR *szString;
				   szString=ptr+sizeof(LRT_CHAREX);
				   LDispLex(fPrinter,hDC,(LRT_CHAREX *) ptr,szString,lpRect);
				 }
				  break;

		case LRE_CHARBOX: 
		         {
				   CHAR *szString;
				   szString=ptr+sizeof(LRT_CHARBOX);
				   LDispCharInBox(fPrinter,hDC,(LRT_CHARBOX *) ptr,szString,lpRect);
				 }
				  break;

		case LRE_BOX:
		case LRE_BOXP:
		case LRE_BOXR:
		case LRE_BOXPR:
				LMultiBox(fPrinter,hDC,arElement[a]->iType,(LRT_BOXR *) ptr,lpRect);
				break;

		case LRE_LINE:
				LMultiLine(fPrinter,hDC,arElement[a]->iType,(LRT_LINE *) ptr,lpRect);
				break;

		case LRE_EHIMG:
		case LRE_EHIMGD:
				_ImgOut(fPrinter,hDC,arElement[a]->iType,(LRT_BOX *) ptr,lpRect);
				break;

		case LRE_IMAGELIST:
				_LImageListOutput(fPrinter,hDC,lpRect,(LRT_IMAGE *) ptr);
				break;

		case LRE_BITMAP:
			{
				S_PWD_BITMAP *ps=(S_PWD_BITMAP *) ptr;
				//_LBitmapOutput(fPrinter,hDC,lpRect,&ps->rArea,ps->hBitmap,NULL);
			}
			break;


	 }
  }
  /*
#ifndef _CONSOLE
  WinDirectDC(NULL,&WScena);
#endif
  */
}


static void LPagePrinter(INT PageStart,INT PageEnd)
{
    //static DOCINFO  di = { sizeof (DOCINFO), "", NULL ,NULL,0} ;
	static DOCINFO  di;
	BOOL            bSuccess ;
	INT iPage;
	INT iColCopy,iNoiColCopy;

#ifndef _CONSOLE
	HWND hWnd=WindowNow();
#endif

#ifndef _CONSOLE
	EnableWindow (WIN_info[sys.WinInputFocus].hWnd, FALSE) ;
#endif

    bSuccess   = TRUE;
    bUserAbort = FALSE ;

#ifndef _CONSOLE
    hDlgPrint = CreateDialog(sys.EhWinInstance, (LPCTSTR) "PrintDlgBox", WIN_info[sys.WinInputFocus].hWnd, PrintDlgProc);
    //SetDlgItemText (hDlgPrint, IDD_FNAME, "Stampa della pagina");//szTitleName) ;
#endif

	if (!sPd.psDi->iPrinternCopies) sPd.psDi->iPrinternCopies=1;
	SetAbortProc (sPd.psDi->hdcPrinter, AbortProc) ;
	_(di);
	di.cbSize=sizeof (DOCINFO);
	if (sPd.psDi->lpPrintName) di.lpszDocName=sPd.psDi->lpPrintName; else di.lpszDocName="Ferrà srl Document Spooler";
	{
	if (StartDoc(sPd.psDi->hdcPrinter, &di) > 0)
	{
        // Numero di copie
		for (iColCopy = 0 ;
//			 iColCopy < ((WORD) sPd.psDi->sPrinter.Flags & PD_COLLATE ? sPd.psDi->sPrinter.nCopies : sPd.psDi->iCopyNumber) ;
			 iColCopy < ((WORD) sPd.psDi->dwPrinterFlags & PD_COLLATE ? sPd.psDi->iPrinternCopies : sPd.psDi->iCopyNumber) ;
             iColCopy++)
             {
               // Loop sulle Pagine
			   for (iPage = PageStart ; iPage <= PageEnd; iPage++)
               {
				   //printf("%d",sPd.psDi->sPrinter.Flags & PD_COLLATE ? NumCopie : sPd.psDi->sPrinter.nCopies); getch();
				   // Numero di copie 
				   for (iNoiColCopy = 0 ;
                        iNoiColCopy < (sPd.psDi->dwPrinterFlags & PD_COLLATE ? sPd.psDi->iCopyNumber : sPd.psDi->iPrinternCopies) ;
                        iNoiColCopy++)
                    {
                        // Inizio stampa della pagina
							if (StartPage (sPd.psDi->hdcPrinter) <= 0)
                            {
								bSuccess = FALSE ;break ;
                             }

							// New 2005 / Non funziona su 98 etc...
							// Modello di colore divero da Windows
							// I tested although ICM_ON
						
							if (sPd.psDi->lpszICCFilename)
							{
								if (!SetICMMode(sPd.psDi->hdcPrinter, ICM_DONE_OUTSIDE_DC)) {bSuccess = FALSE; break ;}
								if (!SetICMProfile(sPd.psDi->hdcPrinter, sPd.psDi->lpszICCFilename)) {bSuccess = FALSE; break ;}
							}


							LRFillPage(TRUE,sPd.psDi->hdcPrinter,iPage,NULL);  

						 // Fine della pagina
							if (EndPage (sPd.psDi->hdcPrinter) <= 0)
                              {
                               bSuccess = FALSE ;
                               break ;
                              }

						  if (bUserAbort) break ;
					} // Loop sulle copia

                    
					if (!bSuccess || bUserAbort) break ;
			   } // Loop sulle Pagine

               if (!bSuccess || bUserAbort) break ;
			 } // Loop sulle Copie
          }
     else
          bSuccess = FALSE ;

     if (bSuccess) 
	 {
		 EndDoc(sPd.psDi->hdcPrinter);
		 if (sPd.psDi->funcNotify) (*sPd.psDi->funcNotify)(sPd.psDi,WS_CLOSE,0,NULL);
	 }

#ifndef _CONSOLE
     if (!bUserAbort)
	 {
          EnableWindow (WIN_info[sys.WinInputFocus].hWnd, TRUE) ;
          DestroyWindow (hDlgPrint) ;
     }
#endif

	}
#ifndef _CONSOLE
	EnableWindow (WIN_info[sys.WinInputFocus].hWnd, TRUE) ;
	winSetFocus (hWnd);
#endif
}



// -------------------------------------------------------------------------------
// RICALCOLA LA DIMENSIONE DEL LAYOUT                                            |
// -------------------------------------------------------------------------------

static void LayResize(void)
{
 if (LayWnd!=NULL)
 {
  MoveWindow(LayWnd,0,yBarra,WIN_info[LREOwner].CLx,WIN_info[LREOwner].CLy-45,TRUE);
  winSetFocus(OwnerWnd);
 }
}

// -------------------------------------------------------------------------------
// RICALCOLA LE DIMENSIONI DELL'AREA VIRTUALE                                    |
// -------------------------------------------------------------------------------

static void LayVPResize(void)
{
 if (LayWnd!=NULL)
 {
 // MoveWindow(LayWnd,0,45,WIN_info[LREOwner].lx,WIN_info[LREOwner].ly-45,TRUE);
  GetClientRect(LayWnd,&LayClient);
  
  // Area reale Client
  // 20:xPage:x:xClient
  TLayOut.AreaX=LayClient.right-LayClient.left+1;
  xBorder=20*TLayOut.AreaX/sPd.psDi->sPaper.cx; // 20 Dot
  if (TLayOut.AreaX<1) TLayOut.AreaX=1;
  
  TLayOut.AreaY=(LayClient.bottom-LayClient.top+1); 
  yBorder=20*TLayOut.AreaY/sPd.psDi->sPaper.cy; // 20 Dot
  if (TLayOut.AreaY<1) TLayOut.AreaY=1;

  //xClient-=(xBorder<<1); yClient-=(yBorder<<1);
  TLayOut.AreaX-=(xBorder<<1);
  TLayOut.AreaY-=(yBorder<<1);

  switch (iZoom)
  {
	case 0: // Pagina Intera

	// Calcolo della dimensione della pagina
	// Controllo se sono in PORTRAIT o in LANDSCAPE
	switch (LayOrientation)
	{
		  case DMORIENT_PORTRAIT:
			  sPd.sizVideoPage.cy=TLayOut.AreaY;
              // xPage:yPage=x:Vly;
			  sPd.sizVideoPage.cx=sPd.psDi->sPaper.cx*sPd.sizVideoPage.cy/sPd.psDi->sPaper.cy;
			  break;
              // VLx:xPage=y:yPage			       
			  while(TRUE)
			  {
			   if (sPd.sizVideoPage.cy<20) {sPd.sizVideoPage.cy=-1; break;}
			   sPd.sizVideoPage.cx=sPd.psDi->sPaper.cx*sPd.sizVideoPage.cy/sPd.psDi->sPaper.cy;
			   if (sPd.sizVideoPage.cx>(INT) TLayOut.AreaX) sPd.sizVideoPage.cy-=sPd.sizVideoPage.cy/10; else break;
			  }
			  break;
		  
		  case DMORIENT_LANDSCAPE:
			  sPd.sizVideoPage.cx=TLayOut.AreaX;
              // VLx:xPage=y:yPage			       
			  while(TRUE)
			  {
			   if (sPd.sizVideoPage.cx<20) {sPd.sizVideoPage.cx=-1; break;}
			   sPd.sizVideoPage.cy=sPd.psDi->sPaper.cy*sPd.sizVideoPage.cx/sPd.psDi->sPaper.cx;
			   if (sPd.sizVideoPage.cy>(INT) TLayOut.AreaY) sPd.sizVideoPage.cx-=sPd.sizVideoPage.cx/10; else break;
			  }
			  break;
		  default: ehExit("LayOrientation =");
	}		
	break;
	
	case 1: // Larghezza Pagina

			sPd.sizVideoPage.cx=TLayOut.AreaX-xBorder;
			sPd.sizVideoPage.cy=sPd.psDi->sPaper.cy*sPd.sizVideoPage.cx/sPd.psDi->sPaper.cx;
			break;

	case 2: // 50%
			sPd.sizVideoPage.cx=sPd.psDi->sPaper.cx/10;
			sPd.sizVideoPage.cy=sPd.psDi->sPaper.cy*sPd.sizVideoPage.cx/sPd.psDi->sPaper.cx;
			break;
	case 3: // 50%
			sPd.sizVideoPage.cx=sPd.psDi->sPaper.cx/4;
			sPd.sizVideoPage.cy=sPd.psDi->sPaper.cy*sPd.sizVideoPage.cx/sPd.psDi->sPaper.cx;
			break;
	case 4: // 50%
			sPd.sizVideoPage.cx=sPd.psDi->sPaper.cx>>1;
			sPd.sizVideoPage.cy=sPd.psDi->sPaper.cy*sPd.sizVideoPage.cx/sPd.psDi->sPaper.cx;
			break;
	case 5: // 50%
			sPd.sizVideoPage.cx=sPd.psDi->sPaper.cx;
			sPd.sizVideoPage.cy=sPd.psDi->sPaper.cy*sPd.sizVideoPage.cx/sPd.psDi->sPaper.cx;
			break;
  }

  /*
					"10%",
					"25%",
					"50%",
					"100%",
					"200%",
					"400%",
*/

  //if (hFontBase!=NULL) DeleteObject(hFontBase);
  xPos=xBorder;//((LayClient.right-LayClient.left)-sPd.sizVideoPage.cx)>>1;
  yPos=yBorder;
  //yPos=((LayClient.bottom-LayClient.top)-sPd.sizVideoPage.cy)>>1;
  
  // Calcolo l'area di stampa
  sPd.rVideoPrintArea.left=xPos+5; sPd.rVideoPrintArea.top=yPos+5;
  sPd.rVideoPrintArea.right=xPos+sPd.sizVideoPage.cx-5;
  sPd.rVideoPrintArea.bottom=yPos+sPd.sizVideoPage.cy-5;
  sPd.sVideoPrintSize.cx=sPd.rVideoPrintArea.right-sPd.rVideoPrintArea.left+1;
  sPd.sVideoPrintSize.cy=sPd.rVideoPrintArea.bottom-sPd.rVideoPrintArea.top+1;
 }
}


static void * _winPrinterPaint(EH_SUBWIN_PARAMS * psSwp)
{
 //PAINTSTRUCT *ps;
 switch (psSwp->enMess)
 {
  case WS_DISPLAY:
//	ps=(PAINTSTRUCT *) ptr;
//	box3d(0,50,WIN_info[sys.WinWriteFocus].lx,50,2);
	break;

  case WM_SIZE:
	//if (Obj!=NULL) CalcolaLayout(TE,Obj,ON);

	LayResize();
	
	break;
  
  case WM_EXITSIZEMOVE:
	 ////boxp(0,50,WIN_info[sys.WinWriteFocus].lx,80,3,SET);
     //LayVPResize();
     //InvalidateRect(LayWnd,NULL,TRUE);
	 LayVPResize();
	 InvalidateRect(LayWnd,NULL,TRUE);
	 break;

  case WS_LINK: // Libero
	   break;
 }
return NULL;
}

static void CommandCloseAnalisi(CHAR *lpParam,CHAR *pDestCommand)
{
	CHAR szServ[30],*p;
	INT iNumCopie=0;
	*pDestCommand=0;
	if (*lpParam)
	{
		strcpy(szServ,lpParam);
		p=strchr(szServ,'|');
		if (p!=NULL) // Se passo una stringa tipo "VIEW|2"
		{
			*p=0;
			strcpy(pDestCommand,szServ);
			iNumCopie=atoi(p+1);
		}
		else
		{
			if (atoi (lpParam)>0) // Se passo una stringa tipo "2"
			{
				*pDestCommand=0;
				iNumCopie=atoi (lpParam);
			}
			else // Se passo una stringa tipo "VIEW" o ""
			{
				strcpy(pDestCommand,szServ);
				iNumCopie=1;
			}
		}
	}

	if (!sPd.psDi->iCopyNumber) sPd.psDi->iCopyNumber=iNumCopie;
}


#ifndef _CONSOLE			
static void RangeAdjust(HWND hWnd);
static BOOL LREPreview(CHAR *lpParam)
{

	WNDCLASSEX wc;
//	DEVMODE *lLREvMode;   
	//  INT HdlFile;
	//  INT a,b,LPage;
	CHAR Buf[30];
	CHAR szComando[21];
	//  INT iNumCopie=0;

	CHAR *ListView[]={"Pagina",
					"Larghezza",
					"10%",
					"25%",
					"50%",
					"100%",
	//					"200%",
	//					"400%",
					NULL};

	struct OBJ obj[]={
	// TipObj    Nome    Sts Lck   x   y CL1 CL2 TESTO
	{O_KEYDIM,"+"      ,OFF,ON , 31, 2, 30, 21,"+"},
	{O_KEYDIM,"-"      ,OFF,ON ,  2, 2, 30, 21,"-"},
	{O_KEYDIM,"FIRST"  ,OFF,ON ,  2, 23, 75, 19,"La prima"},
	{O_KEYDIM,"LAST"   ,OFF,ON , 76, 23, 76, 19,"L'ultima"},
	{O_KEYDIM,"ESC"    ,OFF,ON ,720, 5, 74, 19,"Esci"},
	{O_KEYDIM,"PAGE"   ,OFF,ON ,153, 2,145, 19,"Stampa la Pagina"},
	{O_KEYDIM,"ALL"    ,OFF,ON ,153, 23,145, 20,"Stampa Tutto"},
	{OW_LIST ,"ZOOM"   ,OFF,ON ,302, 2,  0, 10,"101","",0,ListView},
	{O_KEYDIM,"P-"   ,OFF,ON ,383, 24, 22, 17,"-"},
	{O_KEYDIM,"P+"   ,OFF,ON ,404, 24, 22, 17,"+"},
	{STOP}
	};

	OBJS Objs[]={
		{"#4"     ,OS_TEXT ,305, 25,  0, -1,ON,SET,"SMALL F",3,"N. copie"},
		{NULL   ,STOP}};


	AUTOMOVE am[]={
		//   Type     Name    Vertice Top/Left             Vertice Bottom/Right
		{AMT_OBJ ,"ESC"   ,AMA_RIGHT ,  0,AMA_TOP,  0,AMP_FIX   ,0,AMP_FIX,0},
		{AMT_STOP}};

	struct IPT ipt[]={
		{ 1, 1,ALFA,QUAD, 67, 6, 82, 15,  0, 15,  0,  1},
		{ 1, 1,NUME,QUAD,352, 27, 28,  3,  0, 15,  0,  0,"No","No"},
		{ 0, 0,STOP}
		};

	iPageView=0;
	sPd.sizVideoPage.cx=-1;
	sPd.sizVideoPage.cy=-1;
	_(TLayOut);
  
	LREFileLoad();
	CommandCloseAnalisi(lpParam,szComando);

	if (!*szComando) // Stampo tutto
	{
		mouse_graph(0,0,"CLEX"); //eventGet(NULL);
		eventGet(NULL);
		LPagePrinter(0,sPd.psDi->iPageCount-1);
		MouseCursorDefault();
		goto FINE;
	}
  
    win_openEx(EHWP_SCREENCENTER,0,"Power Printer preview",800,500,-1,OFF,0,WS_EHMOVEHIDE,FALSE,_winPrinterPaint);
	win_SizeLimits(AUTOMATIC,AUTOMATIC,-1,-1);

	LREOwner=sys.WinInputFocus;
	OwnerWnd=WindowNow();//WIN_info[LREOwner].hWnd;
//	lLREvMode=GlobalLock(sPd.psDi->sPrinter.hDevMode);
//	if (!lLREvMode) ehError();

	// --------------------------------------------------
	// In creazione della Prima Finestra                |
	// --------------------------------------------------
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_NOCLOSE;
	wc.lpfnWndProc   = LayProcedure;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	wc.hCursor       = NULL;//LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH) COLOR_WINDOW;//(HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = NULL;///LoadMenu;//szAppName;
	wc.lpszClassName = "EHPrintConsole";
	wc.hIconSm       = NULL;//LoadIcon(NULL,IDI_APPLICATION);
	RegisterClassEx(&wc);
					 
	LayWnd=CreateWindow("EHPrintConsole", 
					  "",
					  WS_BORDER|WS_CHILD|WS_VISIBLE|WS_VSCROLL,
					  0,0,0,0,
					  WIN_info[LREOwner].hWnd, 
					  (HMENU) NULL,//100,// Menu
					  sys.EhWinInstance, 
					  NULL);

	LayOrientation=sPd.psDi->iOrientation;//
	LayResize();
	LayVPResize();

	//  Carico OBJ & variazioni sui Font
	obj_open(obj); objs_open(Objs);

	obj_font("+","COURIER E",0);
	obj_font("-","COURIER E",0);
	obj_vedi();

	//  Carico IPT & font

	ipt_font("VGASYS",0);
	ipt_open(ipt);
	ipt_fontnum(0,"SMALL F",3);
	ipt_fontnum(1,"SMALL F",3);
	ipt_reset(); ipt_vedi();

	obj_AutoMoveAssign(am);
	iZoom=obj_listget("ZOOM");

	ShowWindow(WindowNow(),SW_MAXIMIZE);
	MouseCursorDefault();

	ipt_writevedi(1,NULL,sPd.psDi->iCopyNumber);

	// Loop di controllo EH
	while (TRUE)
	{
   
		sprintf(Buf,"%d/%d",iPageView+1,sPd.psDi->iPageCount);

		ipt_writevedi(0,Buf,0);

		iZoom=obj_listget("ZOOM");
		eventGetWait(NULL);

		if (obj_press("ZOOM")) 
		{ 
			iZoom=obj_listget("ZOOM");
			TLayOut.iVScrollPos=0;
			TLayOut.iHScrollPos=0;
			LayVPResize();
			RangeAdjust(LayWnd);
			InvalidateRect(LayWnd,NULL,TRUE);
		}

		if (key_press(ESC)||obj_press("ESCOFF")) break;

		if (obj_press("P+ON")||key_press('+')) 
		{
			sPd.psDi->iCopyNumber++; if (sPd.psDi->iCopyNumber>100) sPd.psDi->iCopyNumber=100;
			ipt_writevedi(1,NULL,sPd.psDi->iCopyNumber);
		}

		if (obj_press("P-ON")||key_press('-')) 
		{
			sPd.psDi->iCopyNumber--;  if (sPd.psDi->iCopyNumber<1) sPd.psDi->iCopyNumber=1;
			ipt_writevedi(1,NULL,sPd.psDi->iCopyNumber);
		}

		if (obj_press("+ON")||key_press2(_PGDN)) 
		{
			iPageView++; if (iPageView>(sPd.psDi->iPageCount-1)) {iPageView=sPd.psDi->iPageCount-1; continue;}
			InvalidateRect(LayWnd,NULL,TRUE);
		}

		if (obj_press("-ON")||key_press2(_PGUP)) 
		{
			iPageView--; if (iPageView<0) {iPageView=0; continue;}
			InvalidateRect(LayWnd,NULL,TRUE);
		}

		if (obj_press("FIRSTOFF")) {iPageView=0; InvalidateRect(LayWnd,NULL,TRUE);}
		if (obj_press("LASTOFF"))  {iPageView=sPd.psDi->iPageCount-1; InvalidateRect(LayWnd,NULL,TRUE);}

		if (key_press2(_FUP))  SendMessage(LayWnd,WM_VSCROLL,SB_LINEUP,0);
		if (key_press2(_FDN))  SendMessage(LayWnd,WM_VSCROLL,SB_LINEDOWN,0);
		if (key_press2(_FDX))  SendMessage(LayWnd,WM_HSCROLL,SB_LINEDOWN,0);
		if (key_press2(_FSX))  SendMessage(LayWnd,WM_HSCROLL,SB_LINEUP,0);

		if (obj_press("PAGEOFF"))
		{
			LPagePrinter(iPageView,iPageView);
		}

		if (obj_press("ALLOFF"))
		{
		   mouse_graph(0,0,"CLEX"); eventGet(NULL);
		   LPagePrinter(0,sPd.psDi->iPageCount-1);
		   MouseCursorDefault();
		}
  };
  
  DestroyWindow(LayWnd);
  LayWnd=NULL;

FINE:

//  LREFileFreeResource(FALSE);

  if (hFontBase!=NULL) DeleteObject(hFontBase);
#ifndef _CONSOLE			
  if (*szComando) win_close();
#endif
//  GlobalUnlock(sPd.psDi->sPrinter.hDevMode);  
  return 0;
}

// -------------------------------------------------------------
// RangeAdjust                                                 |
//                                                             |
// -------------------------------------------------------------
static void RangeAdjust(HWND hWnd)
{
  SCROLLINFO ScrollInfo;
  INT yArea;
  INT xArea;
  GetClientRect(LayWnd,&LayClient);
  
  // VERTICALE
  TLayOut.AreaY=(LayClient.bottom-LayClient.top+1);  // =NumCam
  yArea=sPd.sizVideoPage.cy-TLayOut.AreaY;
// sPd.sizVideoPage.cy= 
  if (yArea<=0)
  {
   TLayOut.iVScrollMax=0;
  }
  else
  {
   TLayOut.LineVideoY=40;//(TLayOut.AreaY/TLayOut.CharY)+1;
   TLayOut.CharY=(yArea/TLayOut.LineVideoY)+1;
   TLayOut.iVScrollMax=max(0,TLayOut.LineVideoY+10);
   TLayOut.iVScrollPos=min(TLayOut.iVScrollPos,TLayOut.iVScrollMax);
  }

   ScrollInfo.cbSize=sizeof(ScrollInfo);
   ScrollInfo.fMask=SIF_ALL;
   ScrollInfo.nPage=0;//TLayOut.Lines/((TLayOut.AreaY/TLayOut.CharY));
   ScrollInfo.nPos=TLayOut.iVScrollPos;
   ScrollInfo.nMin=0;
   ScrollInfo.nMax=TLayOut.iVScrollMax;
   SetScrollInfo(hWnd,SB_VERT,&ScrollInfo,TRUE);

   // ORIZZONTALE

   TLayOut.AreaX=LayClient.right-LayClient.left+1;
   xArea=sPd.sizVideoPage.cx-TLayOut.AreaX;

   if (xArea<=0)
   {
    TLayOut.iHScrollMax=0;
   }
   else
   {
    TLayOut.LineVideoX=40;//(TLayOut.AreaY/TLayOut.CharY)+1;
    TLayOut.CharX=(xArea/TLayOut.LineVideoX)+1;
    TLayOut.iHScrollMax=max(0,TLayOut.LineVideoX+10);
    TLayOut.iHScrollPos=min(TLayOut.iHScrollPos,TLayOut.iHScrollMax);
   }

  ScrollInfo.cbSize=sizeof(ScrollInfo);
  ScrollInfo.fMask=SIF_ALL;
  ScrollInfo.nPage=0;//TLayOut.Lines/((TLayOut.AreaY/TLayOut.CharY));
  ScrollInfo.nPos=TLayOut.iHScrollPos;
  ScrollInfo.nMin=0;
  ScrollInfo.nMax=TLayOut.iHScrollMax;
  SetScrollInfo(hWnd,SB_HORZ,&ScrollInfo,TRUE);
}

static void LTbox(HDC hdc,INT x,INT y,INT x2,INT y2,LONG Color)
{
	//INT OldCol=ModeColor(TRUE);
    dcLineEx(hdc,x,y,x2,y,Color,SET,PS_DOT,0);
    dcLineEx(hdc,x,y2,x2,y2,Color,SET,PS_DOT,0);
    dcLineEx(hdc,x,y,x,y2,Color,SET,PS_DOT,0);
    dcLineEx(hdc,x2,y,x2,y2,Color,SET,PS_DOT,0);
	//ModeColor(OldCol);
}

static LRESULT CALLBACK LayProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;
  RECT Rect;
  RECTD recRecd;
  //S_WINSCENA Scena;
  INT iScrInc;
  INT iHScrollInc;

  static LCount=0;
  static INT LastPosx;
  static INT LastPosy;
  static BOOL BarActive=FALSE;

  switch (message)
  {
     // Prima chiamata
	 case WM_CREATE:  break;

     case WM_SIZE:
	//if (info==SIZE_MAXSHOW) {win_info("POI"); }
	//if ((info==SIZE_MAXIMIZED)||(info==SIZE_RESTORED)) 
	//{
		  LayVPResize();
	 //InvalidateRect(LayWnd,NULL,TRUE);
	//}
		  //_d_("%d,%d  ",sPd.rVideoPrintArea.bottom,sPd.rVideoPrintArea.right);
		  RangeAdjust(hWnd);
		  InvalidateRect(LayWnd,NULL,TRUE);
		  break;

	 case WM_MOUSEMOVE: 
		  GetWindowRect(hWnd,&Rect);
		  WinMouse(LOWORD(lParam),yBarra+HIWORD(lParam),0xFFFF);
		  break;
/*
#ifdef WM_MOUSEWHEEL
	// Intercetto il mouse Wheel
	case WM_MOUSEWHEEL:
			{
			 WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
			 short zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/120;;
			// if (zDelta>1) dispx("%d",zDelta);
			 //if (zDelta<0) MouseWheelManager(WS_ADD,IN_MW_DOWN);
			 //if (zDelta>0) MouseWheelManager(WS_ADD,IN_MW_UP);
			 dispx("QUI");
			}
			break;
#endif
			*/

	 case WM_LBUTTONDOWN:
	 case WM_RBUTTONDOWN:
	 case WM_LBUTTONUP:
	 case WM_RBUTTONUP:
	 case WM_LBUTTONDBLCLK:
         if (sys.WinInputFocus<0) break;
         WinMouseAction(sys.WinInputFocus,WIN_info[sys.WinInputFocus].hWnd,message,wParam,lParam);
		 break;
//         WinMouseAction(hWnd,message,wParam,lParam);
//		 break;

	 case WM_NCHITTEST:
		  break;

	// ----------------------------------------------- 
	// Disegno la Pagina                             |
    // ----------------------------------------------- 

	 case WM_PAINT:
			hdc=BeginPaint(hWnd,&ps);
			VideoOffset.x=-TLayOut.iHScrollPos*TLayOut.CharX;
			VideoOffset.y=-TLayOut.iVScrollPos*TLayOut.CharY;

			if (arPageIndex!=NULL)
			{
				//dcBoxp(HDC hDC,INT x1,INT y1,INT x2,INT y2,LONG col);
			 dcBoxp(hdc,rectFill(&Rect,
				  xPos+VideoOffset.x,yPos+VideoOffset.y,
				  xPos+VideoOffset.x+sPd.sizVideoPage.cx,
				  yPos+VideoOffset.y+sPd.sizVideoPage.cy),RGB(255,255,255));
			 
			 dcBox(hdc,
				   rectFillD(&recRecd,xPos+VideoOffset.x,yPos+VideoOffset.y,xPos+VideoOffset.x+sPd.sizVideoPage.cx,yPos+VideoOffset.y+sPd.sizVideoPage.cy),
				   0,0);

			 // Area stampabile
			 LTbox(	hdc,
					sPd.rVideoPrintArea.left+VideoOffset.x,
					sPd.rVideoPrintArea.top+VideoOffset.y,
					sPd.rVideoPrintArea.right+VideoOffset.x,
					sPd.rVideoPrintArea.bottom+VideoOffset.y,sys.arsColor[2]);
            
			 // Margine superiore
			 dcLineEx(hdc,
					xPos+VideoOffset.x,
					DotToVideoY(sPd.psDi->rPage.top),
					xPos+VideoOffset.x+sPd.sizVideoPage.cx,
					DotToVideoY(sPd.psDi->rPage.top),
					7,SET,PS_DOT,0);

			 dcLineEx(hdc,
					xPos+VideoOffset.x,
					DotToVideoY(sPd.psDi->rPage.bottom),
					xPos+VideoOffset.x+sPd.sizVideoPage.cx,
					DotToVideoY(sPd.psDi->rPage.bottom),
					7,SET,PS_DOT,0);

			 dcLineEx(hdc,
					DotToVideoX(sPd.psDi->rPage.left),
					yPos+VideoOffset.y,
					DotToVideoX(sPd.psDi->rPage.left),
					yPos+VideoOffset.y+sPd.sizVideoPage.cy,
					7,SET,PS_DOT,0);

			 dcLineEx(hdc,
					DotToVideoX(sPd.psDi->rPage.right),
					yPos+VideoOffset.y,
					DotToVideoX(sPd.psDi->rPage.right),
					yPos+VideoOffset.y+sPd.sizVideoPage.cy,
					7,SET,PS_DOT,0);
			 LRFillPage(FALSE,hdc,iPageView,&ps.rcPaint);
			}
			EndPaint(hWnd,&ps);
			return 0;

	 // Ultimaa Chiamata
	 case WM_DESTROY: break;
	 case WM_COMMAND: break;
			


	 // --------------------------------------------------------------------------------
     // Controllo Scorrimento VERTICALE                                                |
     // --------------------------------------------------------------------------------
     case WM_VSCROLL:

		 iScrInc=0;
		 switch (LOWORD(wParam))
		   {
			case SB_TOP: 
			   iScrInc=-iScrInc;
			   break;
		    
			case SB_BOTTOM: 
			   iScrInc=TLayOut.Lines-iScrInc;
			   break;

			case SB_LINEUP: 
			   iScrInc=-1;
			   break;
			
			case SB_LINEDOWN: 
			   iScrInc=1;
			   break;

            case SB_PAGEUP: 
			   iScrInc=min(-1,-TLayOut.AreaY/TLayOut.CharY);
			   break;
			
			case SB_PAGEDOWN: 
			   iScrInc=max(1,TLayOut.AreaY/TLayOut.CharY);
			   break;

			case SB_THUMBTRACK: 
			case SB_THUMBPOSITION:
			  iScrInc=HIWORD(wParam)-TLayOut.iVScrollPos;
			  break;
			
			//default: break;
		   }

		    iScrInc=max(-TLayOut.iVScrollPos,
			               min(iScrInc,TLayOut.iVScrollMax-TLayOut.iVScrollPos));
		   if (iScrInc!=0)
		   {
			ScrollWindow(hWnd,0,-TLayOut.CharY*iScrInc,NULL,NULL);	
			TLayOut.iVScrollPos+=iScrInc;
			SetScrollPos(hWnd,SB_VERT,TLayOut.iVScrollPos,TRUE);
			UpdateWindow(hWnd);
		   }
           break;

	 // --------------------------------------------------------------------------------
     // Controllo Scorrimento VERTICALE                                                |
     // --------------------------------------------------------------------------------
     case WM_HSCROLL:

		 iHScrollInc=0;
		 switch (LOWORD(wParam))
		   {
			case SB_TOP: 
			   iHScrollInc=-iHScrollInc;
			   break;
		    
			case SB_BOTTOM: 
			   iHScrollInc=TLayOut.Lines-iHScrollInc;
			   break;

			case SB_LINEUP: 
			   iHScrollInc=-1;
			   break;
			
			case SB_LINEDOWN: 
			   iHScrollInc=1;
			   break;

            case SB_PAGEUP: 
			   iHScrollInc=min(-1,-TLayOut.AreaX/TLayOut.CharX);
			   break;
			
			case SB_PAGEDOWN: 
			   iHScrollInc=max(1,TLayOut.AreaX/TLayOut.CharX);
			   break;

			case SB_THUMBTRACK: 
			case SB_THUMBPOSITION:
			  iHScrollInc=HIWORD(wParam)-TLayOut.iHScrollPos;
			  break;
			
			//default: break;
		   }

		    iHScrollInc=max(-TLayOut.iHScrollPos,
			               min(iHScrollInc,TLayOut.iHScrollMax-TLayOut.iHScrollPos));
		   if (iHScrollInc!=0)
		   {
			//ScrollWindow(hWnd,0,-TLayOut.CharX*iHScrollInc,NULL,NULL);	
			ScrollWindow(hWnd,-TLayOut.CharX*iHScrollInc,0,NULL,NULL);
			TLayOut.iHScrollPos+=iHScrollInc;
			SetScrollPos(hWnd,SB_HORZ,TLayOut.iHScrollPos,TRUE);
			UpdateWindow(hWnd);
		   }
           break;


  }  // switch message

  return(DefWindowProc(hWnd, message, wParam, lParam));
// return(0L);
}  // end of WndProc()
#endif // _CONSOLE

static BOOL LREPrintDirect(CHAR *lpParam)
{
  CHAR szComando[21];
  //INT iNumCopie=0;

  iPageView=0;
  sPd.sizVideoPage.cx=-1;
  sPd.sizVideoPage.cy=-1;
  
  memset(&TLayOut,0,sizeof(TLayOut));
  LREFileLoad();
  CommandCloseAnalisi(lpParam,szComando);

  LPagePrinter(0,sPd.psDi->iPageCount-1);
  LREFileFreeResource(TRUE);

  if (hFontBase!=NULL) DeleteObject(hFontBase);
  //GlobalUnlock(sPd.psDi->sPrinter.hDevMode);  
  return 0;
}
  

static BYTE *LREGetNext(BYTE *ptr)
{
  BYTE *pt=ptr; 
  LRETAG *Lre=(LRETAG *) pt;
  if (pt==NULL) ehExit("LRE:Null");
  if (Lre->iType==LRE_ENDFILE) return NULL;
  //pt+=sizeof(LRETAG); pt+=Lre->LenPostData;
  pt+=sizeof(LRETAG)+Lre->iLenTag;
  return pt;
}
 
void LRBoxRound(INT x1,INT y1,INT x2,INT y2,INT Color,INT lRoundWidth,INT lRoundHeight,INT iPenWidth)
{
    LRT_BOXR LRTBox;
	
	_(LRTBox);
	LRTBox.rRect.left=x1;
	LRTBox.rRect.right=x2;
	LRTBox.rRect.top=y1;
	LRTBox.rRect.bottom=y2;
	LRTBox.lColore=Color;
	LRTBox.lRoundHeight=lRoundHeight;
	LRTBox.lRoundWidth=lRoundWidth;
	LRTBox.iPenWidth=iPenWidth;
	LREAddItem(LRE_BOXR,&LRTBox,sizeof(LRT_BOXR),0,0);
}

void LRBoxpRound(INT x1,INT y1,INT x2,INT y2,INT Color,INT lRoundWidth,INT lRoundHeight,INT iPenWidth)
{
    LRT_BOXR LRTBox;
	
	_(LRTBox);
	LRTBox.rRect.left=x1;
	LRTBox.rRect.right=x2;
	LRTBox.rRect.top=y1;
	LRTBox.rRect.bottom=y2;
	LRTBox.lColore=Color;
	LRTBox.lRoundHeight=lRoundHeight;
	LRTBox.lRoundWidth=lRoundWidth;
	LRTBox.iPenWidth=iPenWidth;
	LREAddItem(LRE_BOXPR,&LRTBox,sizeof(LRT_BOXR),0,0);
}

void LRBox(INT x1,INT y1,INT x2,INT y2,INT Color,INT iWidth)
{
    LRT_BOXR LRTBox;
	
	_(LRTBox);
	LRTBox.rRect.left=x1;
	LRTBox.rRect.right=x2;
	LRTBox.rRect.top=y1;
	LRTBox.rRect.bottom=y2;
	LRTBox.lColore=Color;
	LRTBox.iPenWidth=iWidth;
	LREAddItem(LRE_BOX,&LRTBox,sizeof(LRT_BOXR),0,0);
}

void LRBoxp(INT x1,INT y1,INT x2,INT y2,INT Color)
{
    LRT_BOXR LRTBox;
	
	_(LRTBox);
	LRTBox.rRect.left=x1;
	LRTBox.rRect.right=x2;
	LRTBox.rRect.top=y1;
	LRTBox.rRect.bottom=y2;
	LRTBox.lColore=Color;
	LREAddItem(LRE_BOXP,&LRTBox,sizeof(LRT_BOXR),0,0);
}

void LRLine(INT x1,INT y1,INT x2,INT y2,INT Color,INT iPenWidth,INT iStyle,INT iMode)
{
    LRT_LINE LRTLine;

	_(LRTLine);
	LRTLine.rRect.left=x1;
	LRTLine.rRect.right=x2;
	LRTLine.rRect.top=y1;
	LRTLine.rRect.bottom=y2;
	LRTLine.Colore=Color;
	LRTLine.iPenWidth=iPenWidth; //win_infoarg("%d",iPenWidth);
	LRTLine.iStyle=iStyle;
	LRTLine.iMode=iMode;
	LREAddItem(LRE_LINE,&LRTLine,sizeof(LRTLine),0,0);
}

void LRDispf(INT px,INT py,INT Color,EN_DPL iStyles,INT xAllinea,CHAR *lpFont,INT yChar,CHAR *lpString)
{
   LRT_CHAREX LRTCharEx;

   if (!*lpString) return;

   _(LRTCharEx);
   LRTCharEx.Point.x=px;
   LRTCharEx.Point.y=py;
   LRTCharEx.xAllinea=xAllinea;
   LRTCharEx.yChar=yChar;
   LRTCharEx.fFix=FALSE;
   LRTCharEx.iStyles=iStyles;
   LRTCharEx.lColore=Color;
   strcpy(LRTCharEx.szFontName,lpFont);
   LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),lpString,0);
}

void LRDispLine(INT px,INT py,INT Color,EN_DPL iStyles,INT xAllinea,CHAR *lpFont,INT yChar,CHAR *lpString)
{
   LRT_CHAREX LRTCharEx;

   if (!*lpString) return;
   memset(&LRTCharEx,0,sizeof(LRTCharEx));
   LRTCharEx.Point.x=px;//sPd.psDi->rPage.left+px;
   LRTCharEx.Point.y=sPd.psDi->yBodyTop+1+(sPd.psDi->iVirtualLineCount*sPd.psDi->iRowPadded)+py;
   LRTCharEx.xAllinea=xAllinea;
   if (yChar>0) LRTCharEx.yChar=yChar; else LRTCharEx.yChar=sPd.psDi->iRowHeight;
   LRTCharEx.fFix=FALSE;
   LRTCharEx.iStyles=iStyles;
//   LRTCharEx.fBold=sPd.psDi->fBold;//sys.fFontBold;
  // LRTCharEx.fItalic=sPd.psDi->fItalic;//sys.fFontItalic;
/*
   LRTCharEx.fBold=(iStyles&STYLE_BOLD)?1:0;//sPd.psDi->fBold;//sys.fFontBold;
   LRTCharEx.fItalic=(iStyles&STYLE_ITALIC)?1:0;//sPd.psDi->fItalic;//sys.fFontItalic;
   LRTCharEx.fUnderLine=(iStyles&STYLE_UNDERLINE)?1:0;//sPd.psDi->fUnderLine;//sys.fFontItalic;
  */ 
   if (Color!=-1) LRTCharEx.lColore=Color; else LRTCharEx.lColore=sPd.psDi->lGColor;
   if (lpFont) strcpy(LRTCharEx.szFontName,lpFont); else strcpy(LRTCharEx.szFontName,sPd.psDi->lpFontBodyDefault);
   LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),lpString,0);
}

void LRHBitmap(INT x1,INT y1,INT x2,INT y2,INT Hdl)
{
    LRT_BOX LRTBox;
	
	memset(&LRTBox,0,sizeof(LRTBox));
	LRTBox.rRect.left=x1;
	LRTBox.rRect.right=x2;
	LRTBox.rRect.top=y1;
	LRTBox.rRect.bottom=y2;
	LRTBox.Colore=Hdl;
	LREAddItem(LRE_EHIMG,&LRTBox,sizeof(LRTBox),0,0);
}

void LRHBitmapDir(INT x1,INT y1,INT Hdl)
{
    LRT_BOX LRTBox;
	
	memset(&LRTBox,0,sizeof(LRTBox));
	LRTBox.rRect.left=x1;
	LRTBox.rRect.top=y1;
	LRTBox.Colore=Hdl;
	LREAddItem(LRE_EHIMGD,&LRTBox,sizeof(LRTBox),0,0);
}

//
// LRImage()
//
void LRImage(INT x,INT y,INT lx,INT ly,HIMAGELIST himl,INT imgNumber)
{
    LRT_IMAGE LRTImage;
	IMAGEINFO sImageInfo;
	SIZE sizImage;
	
	memset(&LRTImage,0,sizeof(LRTImage));
	LRTImage.ptPos.x=x;
	LRTImage.ptPos.y=y;
	if (!ImageList_GetImageInfo(himl,imgNumber,&sImageInfo)) ehError();
	sizeCalc(&sizImage,&sImageInfo.rcImage);
	if ((ly>0)&&(lx==0)) lx=ly*sizImage.cx/sizImage.cy;
	if ((lx>0)&&(ly==0)) ly=lx*sizImage.cy/sizImage.cx;
	LRTImage.sizDim.cx=lx;
	LRTImage.sizDim.cy=ly;

	LRTImage.recImage.left=x;		LRTImage.recImage.top=y;
	LRTImage.recImage.right=x+lx-1; LRTImage.recImage.bottom=y+ly-1; 
	LRTImage.imgNumber=imgNumber;
	LRTImage.himl=himl;
	LREAddItem(LRE_IMAGELIST,&LRTImage,sizeof(LRTImage),0,0);
}

//
// LRBitmap()
//
void LRBitmap(RECT *prcArea,INT iBitsPixel,HDC hdc,HBITMAP hBitmap,BOOL bDupBitmap)
{
    S_PWD_BITMAP sBitmap;
	SIZE	sizBitmap;
	BITMAP  sInfo;

	_(sBitmap);
	memcpy(&sBitmap.rArea,prcArea,sizeof(RECT));
	sizeCalc(&sizBitmap,prcArea);

	if (bDupBitmap) {

		BITMAPINFO bi;
		HDC hdcTemp;

		//
		// Leggo le informazioni del bitmap
		//
		GetObject( hBitmap,sizeof( BITMAP ), &sInfo);
		if (!iBitsPixel) iBitsPixel=sInfo.bmBitsPixel;
		sBitmap.bAllocated=TRUE; // Allocato (da liberare alla fine)

		//
		// Creo il bitmap
		//
		_(bi);
		bi.bmiHeader.biSize = sizeof(bi.bmiHeader); // size of this struct
        bi.bmiHeader.biBitCount = 24;//iBitsPixel; // rgb 8 bytes for each component(3)
        bi.bmiHeader.biCompression = BI_RGB;// rgb = 3 components
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biWidth = sInfo.bmWidth;
		bi.bmiHeader.biHeight= sInfo.bmHeight;
		hdcTemp=CreateCompatibleDC(0); // create dc to store the image
        sBitmap.hBitmap = CreateDIBSection(hdcTemp, &bi, DIB_RGB_COLORS, 0, 0, 0); // create a dib section for the dc
		SelectObject(hdcTemp, sBitmap.hBitmap); // assign the dib section to the dc
		// Copio il bitmap
		BitBlt(hdcTemp, 0, 0, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, hdc, 0, 0, SRCCOPY); // copy hdc to our hdc
		DeleteDC(hdcTemp); // delete dc
	}
	else 
	{
		sBitmap.hBitmap=hBitmap;
	}

	LREAddItem(LRE_BITMAP,&sBitmap,sizeof(sBitmap),0,0);
}


	


//INT LRGetJustAlt(RECT rArea,CHAR *lpFont,INT yChar,INT iInterLinea,CHAR *lpString)
INT LRGetDispInBoxAlt(LRT_CHARBOX *LCharBox,CHAR *lpString,INT *lpiRows)
{
	HFONT hFont,OldFont; 
	INT  xChar;
	CHAR *NewText;
	INT y;
	HDC hdc;
	xChar=0;
	if (LCharBox->yChar<1) return 0;

	//hdc=GetDC(NULL);
	hdc=sPd.psDi->hdcPrinter;
	NewText=ehAlloc(strlen(lpString)+1);
	if (sys.bOemString) OemToChar(lpString,NewText); else strcpy(NewText,lpString);

	hFont=CreateFont(LCharBox->yChar, // Altezza del carattere
					 xChar, // Larghezza del carattere (0=Default)
					 0, // Angolo di rotazione x 10
					 0, //  Angolo di orientamento bo ???
					 /*
					 sPd.psDi->bBold ? FW_BOLD : 0, // Spessore del carattere (MACRO Ex: FW_BOLD)
					 sPd.psDi->bItalic, // Flag Italico    ON/OFF
					 sPd.psDi->bUnderLine, // Flag UnderLine  ON/OFF
					 */
					 (LCharBox->iStyles&STYLE_BOLD)?FW_BOLD:0,//sPd.psDi->fBold;//sys.fFontBold;
					 (LCharBox->iStyles&STYLE_ITALIC)?1:0,//sPd.psDi->fItalic;//sys.fFontItalic;
					 (LCharBox->iStyles&STYLE_UNDERLINE)?1:0,//sPd.psDi->fUnderLine;//sys.fFontItalic;					 
					 0, // Flag StrikeOut  ON/OFF
					 DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
	//				   FERRA_OUTPRECISION, // Output precision
					 OUT_DEFAULT_PRECIS, // Output precision
					 0, // Clipping Precision
					 DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
					 DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
					 //0,
					 //"Arial"); // Nome del font
					 LCharBox->szFontName); // Nome del font "Courier New"

	OldFont=SelectObject(hdc, hFont);
	SetBkMode(hdc,TRANSPARENT); 
	SetMapMode(hdc, MM_TEXT);
	SetTextCharacterExtra(hdc,LCharBox->iExtraCharSpace);

	//y=LTextJustify(hdc,lpString,&rArea,iInterLinea,FALSE);
	y=LTextInBox(hdc,LCharBox,lpString,FALSE,lpiRows,LCharBox->iMaxRows);
	SelectObject(hdc, OldFont);
	DeleteObject(hFont);

	ehFree(NewText);
	//ReleaseDC(NULL,hdc);
	//if (*lpiRows>2) win_infoarg("[%s] y=%d %d",lpString,y,*lpiRows);
	if (!y) y=LCharBox->yChar;
	return y;
}

void LRDispInBox(RECT rArea,INT Color,INT iStyles,INT iAlign,CHAR *lpFont,INT yChar,INT iInterLinea,CHAR *lpString,BOOL bJustifyLast)
{
   LRT_CHARBOX LCharBox;

   if (!*lpString) return;
   _(LCharBox);
   memcpy(&LCharBox.rArea,&rArea,sizeof(RECT));

   LCharBox.yChar=yChar;
   LCharBox.yInterlinea=iInterLinea;
   LCharBox.fFix=FALSE;
   LCharBox.iStyles=iStyles;
   /*
   LCharBox.fBold=sPd.psDi->fBold;//sys.fFontBold;
   LCharBox.fItalic=sPd.psDi->fItalic;//sys.fFontItalic;
   */

   LCharBox.lColore=Color;
   LCharBox.xAllinea=iAlign;
   LCharBox.bJustifyLast=bJustifyLast;
   strcpy(LCharBox.szFontName,lpFont);
   LREAddItem(LRE_CHARBOX,&LCharBox,sizeof(LCharBox),lpString,0);
}

INT LRDispInBoxEx(RECT rArea,INT Color,INT iStyles,INT iAlign,CHAR *lpFont,INT yChar,INT iInterLinea,CHAR *lpString,BOOL bJustifyLast,INT iExtraCharSpace,INT iMaxRows,INT *lpiRows)
{
   LRT_CHARBOX LCharBox;

   if (!*lpString) return 0;
   _(LCharBox);
   memcpy(&LCharBox.rArea,&rArea,sizeof(RECT));

   LCharBox.yChar=yChar;
   LCharBox.yInterlinea=iInterLinea;
   LCharBox.fFix=FALSE;
   LCharBox.bJustifyLast=bJustifyLast;
   LCharBox.iStyles=iStyles;

   LCharBox.lColore=Color;
   LCharBox.xAllinea=iAlign;
   LCharBox.iExtraCharSpace=iExtraCharSpace; // New 2004
   LCharBox.iMaxRows=iMaxRows;
   strcpy(LCharBox.szFontName,lpFont);
   LREAddItem(LRE_CHARBOX,&LCharBox,sizeof(LCharBox),lpString,0);
   return LRGetDispInBoxAlt(&LCharBox,lpString,lpiRows);

}

void LRDispRiemp(RECT rArea,INT Color,INT iStyles,CHAR *lpFont,INT yChar,CHAR *lpString,CHAR Riemp)
{
	HFONT hFont,OldFont; 
	INT  xChar=0;
	CHAR *NewText;
	CHAR szServ[1024];
	RECT Rect;
	HDC hdc;

	SIZE size ;
	LRT_CHAREX LRTCharEx;

	memcpy(&Rect,&rArea,sizeof(RECT));
	if (yChar<1) return;

	hdc=GetDC(NULL);
	NewText=ehAlloc(strlen(lpString)+1);
	if (sys.bOemString) OemToChar(lpString,NewText); else strcpy(NewText,lpString);

	hFont=CreateFont(yChar, // Altezza del carattere
					 xChar, // Larghezza del carattere (0=Default)
					 0, // Angolo di rotazione x 10
					 0, //  Angolo di orientamento bo ???
/*
					 sPd.psDi->fBold ? FW_BOLD : 0, // Spessore del carattere (MACRO Ex: FW_BOLD)
					 sPd.psDi->fItalic, // Flag Italico    ON/OFF
					 sPd.psDi->fUnderLine, // Flag UnderLine  ON/OFF
*/
					  (iStyles&STYLE_BOLD)?FW_BOLD:0,//sPd.psDi->fBold;//sys.fFontBold;
					  (iStyles&STYLE_ITALIC)?1:0,//sPd.psDi->fItalic;//sys.fFontItalic;
					  (iStyles&STYLE_UNDERLINE)?1:0,//sPd.psDi->fUnderLine;//sys.fFontItalic;					 

					 0, // Flag StrikeOut  ON/OFF
					 DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
	//				   FERRA_OUTPRECISION, // Output precision
					 OUT_DEFAULT_PRECIS, // Output precision
					 0, // Clipping Precision
					 DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
					 DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
					 //0,
					 //"Arial"); // Nome del font
					 lpFont); // Nome del font "Courier New"

	OldFont=SelectObject(hdc, hFont);
	SetBkMode(hdc,TRANSPARENT); 
	SetMapMode(hdc, MM_TEXT);
    SetTextCharacterExtra(hdc,0);

	sprintf (szServ,"%s %c",lpString,Riemp);
	do
	{
		GetTextExtentPoint32(hdc,szServ,strlen (szServ),&size) ;
		sprintf (szServ,"%s%c",szServ,Riemp);

	} while ((INT) size.cx < (rArea.right - rArea.left)) ;

	szServ[strlen (szServ)-1]=0;

	SelectObject(hdc, OldFont);
	DeleteObject(hFont);

	ehFree(NewText);
	ReleaseDC(NULL,hdc);

	memset(&LRTCharEx,0,sizeof(LRTCharEx));
	LRTCharEx.Point.x=rArea.left;
	LRTCharEx.Point.y=rArea.top;
	LRTCharEx.xAllinea=LR_LEFT;
	LRTCharEx.yChar=yChar;
	LRTCharEx.fFix=FALSE;
	LRTCharEx.iStyles=iStyles;
//	LRTCharEx.fBold=sPd.psDi->fBold;//sys.fFontBold;
//	LRTCharEx.fItalic=sPd.psDi->fItalic;//sys.fFontItalic;
	LRTCharEx.lColore=Color;
	strcpy(LRTCharEx.szFontName,lpFont);
	LREAddItem(LRE_CHAREX,&LRTCharEx,sizeof(LRTCharEx),szServ,0);
	return;
}

//
// LRELoadFile()
//
static void LREFileLoad(void)
{
	BYTE *p,*pData;
	INT iLastPage,iLine,iPage;

	// --------------------------------------------------------------------
	// Creo array dei puntatori alle pagine del file caricato in memoria
	// --------------------------------------------------------------------
	hdlFileTemp=fileLoad(sPd.szTempFile,RAM_AUTO); if (hdlFileTemp<0) ehExit("LRE:Non memory");
	pData=memoLock(hdlFileTemp);

	// Control gli elementi
	iLRE_Elements=0; p=pData; do {iLRE_Elements++;} while (p=LREGetNext(p));
	//  for(iLRE_Elements=0,p=lpData;;) {iLRE_Elements++; p=LREGetNext(p); if (p==NULL) break;}
	//  iLRE_Elements=a;

	arElement=ehAllocZero(sizeof (LRETAG *)*iLRE_Elements);
	arPageIndex=ehAllocZero(sizeof(INT)*(sPd.psDi->iPageCount+1));

	//
	// Trovo il primo Tag di ogni pagina
	//
	iLastPage=-1; iLine=0; iPage=0;
	p=pData; 
	do
	{
		arElement[iLine]=(LRETAG *) p;
		if (arElement[iLine]->iPage!=iLastPage) 
		{
			arPageIndex[iPage]=iLine; 
			iLastPage=arElement[iLine]->iPage; 
			iPage++;
		}
		iLine++;
	} while (p=LREGetNext(p));
}

//
// LREFileFreeResource()
//
static void LREFileFreeResource(BOOL bDeleteAll)
{
	INT a;
	LRETAG *psElement;
	BYTE *ptr;
	if (bDeleteAll) 
	{
		//
		// Rimuovo il file temporane
		//
		remove(sPd.szTempFile);

		//
		// Cancello le risorse Extra
		//
		if (arElement) {
			for (a=0;a<iLRE_Elements;a++)
			{
				// Element = LRETAG+<TAG TYPE>+<DATI>
				psElement=arElement[a]; 
				
				ptr=(BYTE *) psElement+sizeof(LRETAG);
				switch (psElement->iType)
				{
					case LRE_BITMAP:
							{
								S_PWD_BITMAP *ps=(S_PWD_BITMAP *) ptr;
								if (ps->bAllocated) DeleteObject(ps->hBitmap);
							}
							break;
				}
			}
		}
		if (!strEmpty(sPd.psDi->pszDeviceDefine)) {
			DeleteDC(sPd.psDi->hdcPrinter);
			ehFree(sPd.psDi->pDevMode);
		}
	}

	if (hdlFileTemp) {memoFree(hdlFileTemp,"LRE"); hdlFileTemp=0;}
	if (arElement) {ehFree(arElement);  arElement=NULL;}
	if (arPageIndex) {ehFree(arPageIndex); arPageIndex=NULL;}
}


//
// LRETagReplace() New 2007 quasi 2008
// Sostituisce nelle scritte dei tag
// @#PAGE#@ numero di pagina
// @#TOTPAGE#@ numero totale di pagina
//
// 
BYTE *LREstrReplace(LRETAG *pElement,BYTE *pString)
{
	BYTE *lpNewString=ehAlloc(strlen(pString)*2);
	BYTE szServ[80];
	strcpy(lpNewString,pString);

	sprintf(szServ,"%d",pElement->iPage);
	while (strReplace(lpNewString,"@#PAGE#@",szServ));

	sprintf(szServ,"%d",sPd.psDi->iPageCount);
	while (strReplace(lpNewString,"@#TOTPAGE#@",szServ));
	return lpNewString;
}

static void LRETagReplace(void)
{
	LRETAG *pElement;
	CHAR *pString;
	BYTE *ptr,*pNewString;
	INT a;

	//return;

	LREFileLoad();

	//
	// Riscrivo il file
	//
	sPd.chTempFile=fopen(sPd.szTempFile,"wb");
	for (a=0;a<iLRE_Elements;a++)
	{
		// Element = LRETAG+<TAG TYPE>+<DATI>
		pElement=arElement[a]; ptr=(BYTE *) pElement+sizeof(LRETAG);
		switch (arElement[a]->iType)
		{
			case LRE_CHAR: 
					pString=ptr+sizeof(LRT_CHAR);
					pNewString=LREstrReplace(pElement,pString);
					//pElement->iLenDynamics=strlen(pNewString)+1;
					pElement->iLenTag=sizeof(LRT_CHAR)+strlen(pNewString)+1;
					fwrite(pElement,sizeof(LRETAG)+sizeof(LRT_CHAR),1,sPd.chTempFile);
					fwrite(pNewString,strlen(pNewString)+1,1,sPd.chTempFile);
					ehFree(pNewString);
				    break;

			case LRE_CHAREX: 
					pString=ptr+sizeof(LRT_CHAREX);
					pNewString=LREstrReplace(pElement,pString);
					pElement->iLenTag=sizeof(LRT_CHAREX)+strlen(pNewString)+1;
					fwrite(pElement,sizeof(LRETAG)+sizeof(LRT_CHAREX),1,sPd.chTempFile);
					fwrite(pNewString,strlen(pNewString)+1,1,sPd.chTempFile);
					ehFree(pNewString);
				    break;

			case LRE_CHARBOX: 
					pString=ptr+sizeof(LRT_CHARBOX);
					pNewString=LREstrReplace(pElement,pString);
					pElement->iLenTag=sizeof(LRT_CHARBOX)+strlen(pNewString)+1;
					fwrite(pElement,sizeof(LRETAG)+sizeof(LRT_CHARBOX),1,sPd.chTempFile);
					fwrite(pNewString,strlen(pNewString)+1,1,sPd.chTempFile);
					ehFree(pNewString);
				    break;

			default: // Gli altri tipi li trascrivo senza modifica
					fwrite(pElement,sizeof(LRETAG)+pElement->iLenTag,1,sPd.chTempFile);
					break;
		}
	}
	LREAddItem(LRE_ENDFILE,NULL,0,NULL,0);
	fclose(sPd.chTempFile); sPd.chTempFile=NULL;
	LREFileFreeResource(FALSE);
}

static INT LRToStyles(void)
{
	INT iStyles=0;
	iStyles|=sPd.psDi->bBold?STYLE_BOLD:0;
	iStyles|=sPd.psDi->bItalic?STYLE_ITALIC:0;
	iStyles|=sPd.psDi->bUnderLine?STYLE_UNDERLINE:0;
	return iStyles;
}
    