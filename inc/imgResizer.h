//   
//  imgResizer
//  Crea un'immagine di differente grandezza
//    
//             by Ferrà Art & Technology 1993-2002
//   

typedef struct {

	CHAR	szFile[500];
	BOOL	bPosWhere;		// 0=Posizionato su la foto nuova (compresi i bordi), 1=posizionato sulla vecchia foto
	SINT	iAlignHor;
	SINT	iAlignVer;
	double	dPercSize;
	SINT	iAlpha;
	SINT	iOffsetX;
	SINT	iOffsetY;

} IMG_RSZ_PARAM;
// CHAR *AddToName(CHAR *lpFileSource,CHAR *lpAdd);

typedef struct {

	CHAR *			pszFileSource;	// Nome del file sorgente

	EN_FILE_TYPE	enImageTypeSave;
	CHAR *			pszFileDest;		// Stringa da aggiungere al nome del file

	SIZE			sDim;			// Dimensioni della foto finale
	SINT			iQuality;		// 10>100 Qualità del JPG
	SINT			iOrientation; // Rotazione Exif
	IMG_RSZ_PARAM	sPhoto;

	//	double dPhotoPerc;		// Percentuale dell'immagine da occupare 25= 1/4
	BOOL			fMakeEver;		// T/F: Costruisce sempre
	SINT			iResampling;		// Tipo di ricampionamento TRS_LANCZOS=10
	EN_IMGPT		iPhotoAdatta;	// Tipo di adattamento alla foto
						// 0= Proporzionale
						// 1= Best in fit (Adatta al formato)
						// 2= adatta al corto
						// 3= adatta al lungo
	SINT			cBackColor;		// Colore di background (per riempire gli spazi vuoti)
	BOOL			bAutoLevel;
	BOOL			bQuickLoading;	//T/F

	IMG_RSZ_PARAM	sLogo;
	IMG_RSZ_PARAM	sPaint;
	SINT			iEchoPaint;

	CHAR *lpText;
} IMG_RESIZE;


BOOL imgResize(IMG_RESIZE *psImgResize);		// Testo da stampare
BOOL imgResizer(CHAR *lpFile, CHAR *lpAdd, SINT iX,SINT iY,SINT iQuality,SINT iOrientation,BOOL fMakeEver,SINT iResampling);
BOOL imgResizerEx(CHAR *	lpFileSource,	// Nome del file sorgente
				  CHAR *	lpFileDest,			// Stringa da aggiungere al nome del file
				  SIZE		sDim,			// Dimensioni della foto finale
				  SINT		iQuality,		// 10>100 Qualità del JPG
				  double	dPhotoPerc,		// Percentuale dell'immagine da occupare 25= 1/4
				  BOOL		fMakeEver,		// T/F: Costruisce sempre
				  SINT		iResampling,		// Tipo di ricampionamento TRS_LANCZOS=10
				  EN_IMGPT	iPhotoAdatta,	// Tipo di adattamento alla foto
											// 0= Proporzionale
											// 1= Best in fit (Adatta al formato)
											// 2= adatta al corto
											// 3= adatta al lungo

				  SINT		cBackColor,		// Colore di background (per riempire gli spazi vuoti)
				  SINT		iAlignH,			// Allineamento foto orizzontale 0=Centrale, 1=Left, 2=Right
				  SINT		iAlignV,			// Allineamento foto verticale   0=Centrale, 1=Top,  2=Down
				  SINT		iPhotoOffsetX,	// Correzione su allineamento Orizzontale
				  SINT		iPhotoOffsetY,   // Correzione su allineamento Verticale
				  SINT		iImageAlpha,		// Percentuale di trasparenza dell'immagine con il background
				  BOOL		fAutoLevel,
				  SINT		iOrientation,	// Orientamento EXIF 0 = non indicato
				  
				  CHAR *	lpLogoFile,		// Logo del file da fondere all'immagine (può non esserci)
				  double	dLogoPerc,		// Percentuale dell'immagine da occupare 25= 1/4
				  SINT		iLogoAlignH,		// Allineamento foto orizzontale 0=Centrale, 1=Left, 2=Right
				  SINT		iLogoAlignV,		// Allineamento foto verticale 0=Centrale, 1=Top, 2=Down
				  SINT		iLogoOffsetX,	// Correzione su allineamento Orizzontale
				  SINT		iLogoOffsetY,    // Correzione su allineamento Verticale
				  SINT		iLogoAlpha,
				  BOOL		fLogoPos,		// 0 sulla nuova Immagine calcolata, 1= Sull'immagine originale

				  CHAR *	lpText);			// Testo da stampare



