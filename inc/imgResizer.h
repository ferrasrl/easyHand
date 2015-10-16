//   
//  imgResizer
//  Crea un'immagine di differente grandezza
//    
//             by Ferrà Art & Technology 1993-2002
//   

typedef struct {

	CHAR	szFile[500];
	BOOL	bPosWhere;		// 0=Posizionato su la foto nuova (compresi i bordi), 1=posizionato sulla vecchia foto
	INT		iAlignHor;
	INT		iAlignVer;
	double	dPerc;
	SIZE *	psSizeFix;			// Dimensioni fise del logo (non proporzionali alla foto) (new 2015)
	INT		iAlpha;
	INT		iOffsetX;
	INT		iOffsetY;

} IMG_RSZ_PARAM;
// CHAR *AddToName(CHAR *lpFileSource,CHAR *lpAdd);

typedef struct {

	CHAR *			pszFileSource;	// Nome del file sorgente

	EN_FILE_TYPE	enImageTypeSave;
	CHAR *			pszFileDest;		// Stringa da aggiungere al nome del file

	SIZE			sFix;			// Dimensioni volute della foto finale
	SIZE	*		psMin;			// Dimensioni massime della foto finale (new 2015)
	SIZE	*		psMax;			// Dimensioni massime della foto finale (new 2015)
	SIZE			sDest;			// Dimensioni della foto finale effettive (new 2015)
	INT				iQuality;		// 10>100 Qualità del JPG
	INT				iOrientation; // Rotazione Exif
	IMG_RSZ_PARAM	sPhoto;

	//	double dPhotoPerc;		// Percentuale dell'immagine da occupare 25= 1/4
	BOOL			fMakeEver;		// T/F: Costruisce sempre
	INT				iResampling;		// Tipo di ricampionamento TRS_LANCZOS=10
	EN_IMGPT		enPhotoAdatta;	// Tipo di adattamento alla foto
						// 0= Proporzionale
						// 1= Best in fit (Adatta al formato)
						// 2= adatta al corto
						// 3= adatta al lungo
	INT				cBackColor;		// Colore di background (per riempire gli spazi vuoti)
	BOOL			bAutoLevel;
	BOOL			bQuickLoading;	//T/F

	IMG_RSZ_PARAM	sLogo;
	IMG_RSZ_PARAM	sPaint;
	INT				iEchoPaint;

	CHAR *lpText;
} IMG_RESIZE;


BOOL imgResize(IMG_RESIZE *psImgResize);		// Testo da stampare
BOOL imgResizer(CHAR *lpFile, CHAR *lpAdd, INT iX,INT iY,INT iQuality,INT iOrientation,BOOL fMakeEver,INT iResampling);
BOOL imgResizerEx(UTF8 *	pszFileSource,	// Nome del file sorgente
				  UTF8 *	pszpFileDest,			// Stringa da aggiungere al nome del file
				  SIZE *	psDimFix,			// Dimensioni della foto finale
				  SIZE *	psDimMin,			// Dimensioni massime della foto finale (new 2015)
				  SIZE *	psDimMax,			// Dimensioni massime della foto finale (new 2015)
				  INT		iQuality,		// 10>100 Qualità del JPG
				  double	dPhotoPerc,		// Percentuale dell'immagine da occupare 25= 1/4
				  BOOL		fMakeEver,		// T/F: Costruisce sempre
				  INT		iResampling,		// Tipo di ricampionamento TRS_LANCZOS=10
				  EN_IMGPT	iPhotoAdatta,	// Tipo di adattamento alla foto
											// 0= Proporzionale
											// 1= Best in fit (Adatta al formato)
											// 2= adatta al corto
											// 3= adatta al lungo

				  INT		cBackColor,		// Colore di background (per riempire gli spazi vuoti)
				  INT		iAlignH,			// Allineamento foto orizzontale 0=Centrale, 1=Left, 2=Right
				  INT		iAlignV,			// Allineamento foto verticale   0=Centrale, 1=Top,  2=Down
				  INT		iPhotoOffsetX,	// Correzione su allineamento Orizzontale
				  INT		iPhotoOffsetY,   // Correzione su allineamento Verticale
				  INT		iImageAlpha,		// Percentuale di trasparenza dell'immagine con il background
				  BOOL		fAutoLevel,
				  INT		iOrientation,	// Orientamento EXIF 0 = non indicato
				  
				  CHAR *	lpLogoFile,		// Logo del file da fondere all'immagine (può non esserci)
				  double	dLogoPerc,		// Percentuale dell'immagine da occupare 25= 1/4
				  SIZE *	psLogoFix,
				  INT		iLogoAlignH,		// Allineamento foto orizzontale 0=Centrale, 1=Left, 2=Right
				  INT		iLogoAlignV,		// Allineamento foto verticale 0=Centrale, 1=Top, 2=Down
				  INT		iLogoOffsetX,	// Correzione su allineamento Orizzontale
				  INT		iLogoOffsetY,    // Correzione su allineamento Verticale
				  INT		iLogoAlpha,
				  BOOL		fLogoPos,		// 0 sulla nuova Immagine calcolata, 1= Sull'immagine originale

				  CHAR *	lpText);			// Testo da stampare



