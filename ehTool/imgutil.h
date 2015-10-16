//   +-------------------------------------------+
//   | imgUtil  Image Utility                    |
//   |          Lettura e scrittura delle        |
//   |          immagini                         |
//   |                                           |
//   |             8 Maggio                      |
//   |             by Ferr… Art & Tecnology 1999 |
//   +-------------------------------------------+

#define XMD_H
#define XMD_H

#ifndef EH_IMGUTIL
#define EH_IMGUTIL

	//#include "c:\Fvisual\IMG2002\Jpeg_6b\jpeglib.h"
	#include "/easyhand/inc/Jpeg_6b/jpeglib.h"

	#ifdef __cplusplus
	extern "C" {
	#endif

	typedef int EH_IMG;

	#ifndef GetAValue
		#define GetAValue(rgb) ((rgb>>24)&0xFF)
	#endif
		
	typedef enum {
		IMG_UNKNOW=0,
		IMG_JPEG,
		IMG_GIF,
		IMG_PNG,
		IMG_BMP
	} EN_FILE_TYPE;

	typedef enum {
		IMG_PIXEL_UNKNOW,
		IMG_PIXEL_COLOR1,	// 2 colori 1 bit x pixel > con pallet
		IMG_PIXEL_COLOR4,	// 16 colori 4 bit x pixel > con pallet

		IMG_PIXEL_GRAYSCALE,  
		IMG_PIXEL_COLOR8,	// 256 colori 1 byte = pixel > con pallet

		IMG_PIXEL_BGR,  // Standard <-------------
		IMG_PIXEL_RGB,  
		IMG_PIXEL_RGB_ALPHA, // Colore + Alpha
		IMG_PIXEL_CMYK, // 4 colori

		IMG_PIXEL_YCbCr,
		IMG_PIXEL_YCCK,

		IMG_PIXEL_BGR_16 // Da fare

	} EN_PIXEL_TYPE;

	/*
	#define IMG_JPEG 0
	#define IMG_GIF  1
	#define IMG_PNG  2
	#define IMG_BMP  3
	*/
	#define IMG_SINGLETHREAD 0
	#define IMG_MULTITHREAD 1
	void IMG_Mode(INT cmd,INT iMode);

	typedef struct {
		BYTE r;
		BYTE g;
		BYTE b;
	} S_RGB;

	typedef struct {
		BYTE c;
		BYTE m;
		BYTE y;
		BYTE k;
	} S_CMYK;

	typedef enum {
		IMGPT_PROPORZIONALE=0,	// Ricalcola un lato mancante (in sDmi viene indicato un alto solo)
		IMGPT_AL_FORMATO=1,		// La foto viene fatta del formato indicato in sDmin
		IMGPT_AL_CORTO=2,		// Viene calcolato il lato LUNGO (proporzionalmente) tenendo inviriato il CORTO
		IMGPT_AL_LUNGO=3,		// Viene calcolato il lato CORTO (proporzionalmente) tenendo inviriato il LUNGO
		IMGPT_PROP_TAGLIA=5,	// La foto viene tagliata delle dimensioni proporzionali contenute in sDim
		IMGPT_MAX_SIDE=6,		// La foto viene rimensionata in modo da essere contenuta in una dimensione sDim (max lato)
		IMGPT_NO=100
	} EN_IMGPT;

	//void CMYKtoRGB(S_CMYK *lpCmyk,S_RGB *lpRgb); // New 2005
	//INT IMGToRGB(INT hdlImage);
	EH_IMG IMGConvert(EH_IMG hdlImage,EN_PIXEL_TYPE enPixelType); // new 2010

	typedef struct {

		EN_PIXEL_TYPE	enPixelType;

		EN_FILE_TYPE	enType;			// Tipo vedi IMG_?
		EN_PIXEL_TYPE	enFilePixelType;
		UTF8			utfFileName[MAXPATH];
		UTF8			utfFullFileName[500];	// new 2010
		BYTE *			pbOriginal;				// new 2014
		LONG			lFileSize;				// Dimensione del file

		LONG			linesize;				// Larghezza linea in byte
		INT			iChannels; // Dimensioni i byte del pixel
		BITMAPINFOHEADER bmiHeader;
		LONG			Offset;
		SIZE_T			dwBitmapSize;	// Dimensione dell'immagine in memoria
		SIZE_T			dwMemoSize;		// Dimensione di tutta la memoria (HEADER+BITMAP+ALTRO es Pallette)
		
		// Solo con la palette COLOR8
		INT			idxTrasparent;	// Indice della trasparenza (-1) se non usato
		DWORD			dwColors;		// Numero dei colori della palette
		S_RGB *			psRgbPalette;
		//BYTE *	pbPalette;		// Puntatore al primo bute della palette

		BYTE *			pbImage;		// Puntatore al primo byte dell'immagine

	} IMGHEADER;

	//	LONG lBitmapSize; // Dimensione del bitmap (dopo l'offset)
	//  BOOL fAlpha; // T/F se ha alpha Channel
	//  BOOL fRGBInverted; // Se l'RGB è invertito per Windows in BGR
	EN_FILE_TYPE isImage(UTF8 * pszFileName);

	TCHAR *	IMGTextType(EN_PIXEL_TYPE enPixelType);
	EH_IMG	IMGCreate(EN_PIXEL_TYPE enPixelType,
				   CHAR *lpMemoName,
				   DWORD dwWidth,
				   DWORD dwHeight, 
				   IMGHEADER *psImgHead, // Se vuoi l'header di ritorno - può esser NULL
				   BOOL bOnlyHeader); // TRUE se vuoi solo l'header senza allocare memoria
	void	IMGDestroy(EH_IMG);
	void	IMGFill(INT hdlImage,DWORD cBackColor);

	//void IMGDisplay(HDC hdc,INT x,INT y,INT hdl); // New fine 2004

	INT	IMGFromIcone(CHAR *lpIcone,INT iLx,INT iLy,struct  ICO_HEAD *lpHead);

	// ATTENZIONE:
	// Ofx,Ofxy da fare
	// Immagini oltre i 1024 punti di larghezza potrebbere avere problemi
	void	IMGDisp(INT PosX,INT PosY,INT Hdl);
	void	IMGDispEx(INT PosX,INT PosY,INT SizeX,INT SizeY,INT OfsX,INT OfsY,INT Hdl);
	void	dcImageShow(HDC hdc,INT x,INT y,DWORD dwCx,DWORD cxCy,INT hImage); // New fine 2004
	//#define IMGDisplay dcImageShow

	void IMGCalc(INT *SizeX,INT *SizeY,INT Hdl);
	void IMGTopdown(INT HdlImage);
	void IMG_RGBSwapping(IMGHEADER *psImgHead);
	void IMGCalcSize(IMGHEADER *ImgHead,      // Dimensioni del sorgente
					 SIZE * psFix,		   // Area disponibile
					 SIZE * psMin,		   // Area disponibile
					 SIZE * psMax,		   // Area disponibile
					 EN_IMGPT iPhotoAdatta,  // Tipo di adattamento
					 INT iAlignH,	   // Allineamento orizzontale
					 INT iAlignV,	   // Allineamento verticale
					 SIZE *sDest,		   // Dimensioni della destinazione
					 RECT *rDest,
					 RECT *psrSource); 	   // Posizionamento in destinazione{
	void RectCalcSize(	SIZE * pssSource,		// Dimensioni del sorgente
						SIZE * psFix,		   // Area disponibile
						SIZE * psMin,		   // Area disponibile
						SIZE * psMax,		   // Area disponibile
						EN_IMGPT iPhotoAdatta,// Tipo di adattamento
						INT iAlignH,	    // Allineamento richiesto orizzontale
						INT iAlignV,	    // Allineamento richiesto verticale
						SIZE *lpsDest,		// Dimensioni della destinazione
						RECT *lprDest,		// Posizionamento in destinazione
						RECT *lprSource);	    // Rettangolo da prendere nel sorgente

	#ifndef jmp_buf
	#include <setjmp.h>
	#endif

	  struct ima_error_mgr {
	  struct jpeg_error_mgr pub;	
	  jmp_buf setjmp_buffer;	
	};

	void CreateGrayColourMap(BYTE *Palette,int nGrays);
	void SetPalette(BYTE *Palette,INT nColors, BYTE *Red, BYTE *Green, BYTE *Blue);

	// -----------------------------------------------------------
	// JPEG AREA
	// -----------------------------------------------------------
	//BOOL JPGReadFile(CHAR *imageFileName,INT *HdlImage,BOOL Quantize);

	// Tipo di ricampionamento
	#define TRS_NONE 0
	#define TRS_BILINEAR 1
	#define TRS_BICUBIC12 2
	#define TRS_BICUBIC13 3
	#define TRS_BICUBIC14 4
	#define TRS_BICUBIC15 5
	#define TRS_BICUBIC16 6
	#define TRS_BICUBIC17 7
	#define TRS_BICUBIC18 8
	#define TRS_BICUBIC19 9
	#define TRS_LANCZOS 10

	#define JME_NORMAL 0 // Vedi l'errore
	#define JME_HIDE 1 // Non mostrare l'errore

	INT IMGRemaker(INT HdlImage,RECT *psrRectSource,INT xNew,INT yNew,BOOL fQuality,INT iResampling);
	INT IMGResampling(INT HdlImage,  // Handle dell'immagine
					   RECT *lpSubRange,
					   int target_width, 
					   int target_height, 
					   int iTypeResampling);
	void IMGGetSize(INT hImage,SIZE *lps);
	INT IMGLevels(INT HdlImage,
				   INT in_min, 
				   double gamma, 
				   INT in_max, 
				   INT out_min, 
				   INT out_max);

	// new 2007
	//HBITMAP ImgToBitmap(INT hdlImage);
	HBITMAP ImgToBitmap(INT hdlImage,SIZE *psizImage,BOOL bAlphaEver);
	INT BitmapToImg(HBITMAP hBitmap,
					 BOOL bGetBitmap, // Usare TRUE se diverso da DIB 
					 RECT *prcClip,
					 EN_PIXEL_TYPE enPixelTypeDest); // new 2010

	HBITMAP JPGReadLikeBitmap(TCHAR *tsFile);
	HBITMAP GIFReadLikeBitmap(TCHAR *tsFile);

	typedef struct {byte r,g,b;} GIFrgb ;



	// -----------------------------------------------------------
	// BMP AREA
	// -----------------------------------------------------------

	BOOL BMPSaveFile(TCHAR *imageFileName, INT HdlImage);
	INT BMPReadFile(TCHAR *imageFileName, INT *piImageHandle);
	INT BMPReadHeader(TCHAR *imageFileName, IMGHEADER * psImgHead);
	HBITMAP BMPLoadBitmap(TCHAR *imageFileName); // 2010
	//INT _BMPReadService(TCHAR *imageFileName, IMGHEADER *psImgHead ,BOOL bOnlyHeader);


	// -----------------------------------------------------------
	// JPG AREA
	// -----------------------------------------------------------
	INT JPGGetFactor(SIZE *lpsSource,SIZE *lpsDest,INT *lpiPerc); // 2011
	BOOL JPGReadFile(UTF8 *imageFileName,INT *HdlImage,BOOL *fStop,INT *iErr,INT iModeError,EN_PIXEL_TYPE enPixelType);
	BOOL JPGReadFileEx(UTF8 *imageFileName,
					   INT *piImageHandle,
					   BOOL *fStop,
					   
					   INT iFactor,
					   INT iDct,	// JDCT_ISLOW (Default), JDCT_IFAST (Veloce ma meno accurato), JDCT_FLOAT
					   
					   INT *lpiErr,
					   BOOL iModeError,
					   EN_PIXEL_TYPE enPixelType);

	BOOL JPGSaveFile(UTF8 *imageFileName,INT HdlImage,INT iQuality);
	BOOL JPGPutStream(FILE *outfile,INT HdlImage,INT iQuality);
	INT JPGNewFile(UTF8 *lpFileSource,UTF8 *lpFileDest,INT iLx,INT iLy,INT iQuality,BOOL fAliasing,INT iResampling,EN_PIXEL_TYPE enPixelType);
	INT JPGReadHeader(UTF8 *imageFileName,IMGHEADER *ImgHead,INT iModeError);


	// -----------------------------------------------------------
	// GIF AREA
	// -----------------------------------------------------------
	typedef struct {           /*   Colors Table*/
	  SHORT iRes;  /* color resolution in bit*/
	  SHORT iColors;   /* Numero di colori nella palette */
	  GIFrgb palette[256];    
	} GIFTABCOLOR;

	typedef struct {
		BOOL  fTrasparent;	// TRUE/FALSE
		INT  iTraspIndex;	// Indice della trasparenza
		INT  iDelay;		// Tempo di Ritardo (1/100 di secondo)
		POINT pImage;		// Posizione Left/Top
		SIZE  sImage;		// Dimensione orizzontale e verticale
		BOOL  fLocalColor;  // C'è una tabella palletes collegata all'immagine
		INT  iColors;		// Numeri dei colori della tabella
		BOOL  fInterlace;   // Immagine interlacciata
		INT  iDisposal;	// Metodo di disposizione
		INT  iPointer;		// Puntatore al blocco di definizione dell'immagine
	} S_GIFFRAMEINFO;

	typedef struct {
	  TCHAR szFileName[300];
	  INT hdlFile;				// Handle della memoria che contiene il file compresso
	  LONG lSize;				// Dimensione del file
	  SIZE sArea;				// Area del Logical Screen (Dimensioni dell'immagine)
	  BYTE header[7];			/* Firma and version */

	  GIFTABCOLOR  sTPCGlobal;	// Tabella dei colori generale
	  _DMI dmiFrames;	// DMI dei frame (GIFFRAMEINFO)
	  
	  GIFTABCOLOR  sTPCImage;	// Tabella dei colori Immagine
	  INT		  hdlImage8;	// Handle Memoria che contiene l'immagine a 256 colori (8 bit)
	  INT		  iImageSize;	// Dimensioni dell'area immagine a 8bit
	  BOOL		bTrasparent;	// Esiste almeno un frame con la trasparenza
	} GIFINFO;

	BOOL GIFOpenFile(TCHAR *imageFileName,GIFINFO *lpGIFInfo);
	BOOL GIFSaveFile(TCHAR *imageFileName,GIFINFO *lpGIFInfo);
	BOOL GIFSaveFile_IMG(TCHAR *pszImageFileName,INT hdlImage);
	void GIFCloseFile(GIFINFO *lpGIFInfo);
	INT GIFProcess(GIFINFO *lpGIFInfo,INT iFromFrame,INT iToFrame);

	BOOL GIFtoIMG(GIFINFO *lpGIFInfo,	// Struttura GIFINFO
				  INT iFromFrame,		// Rendering fino al Frame
				  INT iToFrame,		// Rendering fino al Frame
				  INT cBackGround,		// Color del background in TRUE Color
				  INT *lpHdlImage,			// Handle che conterrà l'immagine
				  EN_PIXEL_TYPE enPixelType);
	BOOL IMGToGIF(INT hdlImage,
				  GIFINFO *lpGIFInfo);

	BOOL GIFReadHeader(TCHAR *imageFileName,IMGHEADER *psImgHead,INT iModeError);
	BOOL GIFReadFile(TCHAR *imageFileName,INT *lpHdlNewImage,INT cBackGround,BOOL iModeError,EN_PIXEL_TYPE enPixelType);

	void JPGSyncEnter(void);
	void JPGSyncLeave(void);

	// PNG
	BOOL PNGReadHeader(TCHAR *imageFileName,IMGHEADER *ImgHead,INT iModeError); // new 2009
	BOOL PNGReadFile(TCHAR *imageFileName,
					 INT *HdlImage,
					 INT *lpiErr,
					 BOOL iModeError);
	BOOL PNGSaveFile(TCHAR *imageFileName,INT HdlImage,INT iQuality); // new 2009

	typedef struct {
		CHAR szNome[80];
		INT iHdl;
		INT iGrp;
	} PNG_MEMO;
	void *srvPng(INT cmd,INT info,CHAR *ptr);

	#ifdef __cplusplus
	}
	#endif
#endif