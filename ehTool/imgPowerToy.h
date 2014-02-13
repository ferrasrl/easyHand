//   /-------------------------------------------\
//   | imgPowerToy
//   |          
//   |                                           
//   |            by Ferrà Art & Technology 2004 
//   \-------------------------------------------/

// ----------------------------------------------
// ImgCalcSize
//
#ifdef __cplusplus
extern "C" {
#endif

/*
void RectCalcSize(SIZE sSource,		// Dimensioni del sorgente
				  SIZE sAreaDest,		// Dimensione Area destinazione desiderata
				  EN_IMGPT iPhotoAdatta,// Tipo di adattamento
				  SINT iAlignH,	    // Allineamento richiesto orizzontale
				  SINT iAlignV,	    // Allineamento richiesto verticale
				  SIZE *lpsDest,		// Dimensioni della destinazione
				  RECT *lprDest,		// Posizionamento in destinazione
				  RECT *lprSource);	    // Rettangolo da prendere nel sorgente
void IMGCalcSize(IMGHEADER *ImgHead,      // Dimensioni del sorgente
				 SIZE sDim,		   // Area disponibile
				 EN_IMGPT iPhotoAdatta,  // Tipo di adattamento
				 SINT iAlignH,	   // Allineamento orizzontale
				 SINT iAlignV,	   // Allineamento verticale
				 SIZE *sDest,		   // Dimensioni della destinazione
				 RECT *rDest,	// Posizionamento in destinazione
				 RECT *psrSource); 	   

*/
SINT IMGBuilder(SINT iType,CHAR *lpMemoName,SIZE sDim,SINT cBackColor);

BOOL IMGCopy(SINT hdlImageDst, // La destinazione
			 SINT hdlImageSrc, // Il sorgente
			 POINT pArea,    // La posizione e la dimensione
			 double dAlphaMain);

SINT IMGFactCalc(SIZE *lpsSource,SIZE *lpsDest,SINT *lpiPerc);
SINT IMGFactCalcH(IMGHEADER *ImgHead,SIZE *lpsDest,SINT *lpiPerc);

SINT IMGAutoLevel(SINT hdlImageNew);
SINT ColorConvert(CHAR *lpColore);

BOOL JPGReadSize(CHAR *imageFileName,SINT *HdlImage,BOOL *lpfStop,SINT *lpiErr,SINT iModeError,SIZE sDest);

#ifdef __cplusplus
}
#endif
