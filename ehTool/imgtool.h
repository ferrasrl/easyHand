//   /-------------------------------------------\
//   | IMGTool  Image Tool                       |
//   |          Elaborazione di immagini         |
//   |                                           |
//   |             by Ferr… Art & Technology 2000 |
//   \-------------------------------------------/




void IMGMirrorX(INT HdlImage);
void IMGMirrorY(INT HdlImage);

INT IMGRotation(EN_ROT enRotation,INT HdlImage); // 90° orari

//INT IMGRotation90H(INT HdlImage); // 90° orari
//INT IMGRotation90A(INT HdlImage); // 90° antiorari
//INT IMGRotation180(INT HdlImage); // 180° 
INT IMGBrightness(INT iPerc,INT HdlImage);
INT IMGContrast(INT iPerc,INT HdlImage);
INT IMGBrightnessContrast(INT iPercBright,INT iPercContrast,INT HdlImage);
INT IMGCut(INT iHdlPhoto, // Handle della foto
			POINT *lpFoto, // Punto in cui vuoi tagliarla
			SIZE  *lpsArea, // Dimensioni del taglio
			SIZE  *lpsReal,BOOL  fFreeSource); // Risultato finale
INT IMGLevel(INT hdl,
			    INT in_min, double gamma, INT in_max, 
			    INT out_min, INT out_max,
				BOOL  fFreeSource);
void IMGLevelMaker(INT hdlImage,INT *lpiLevels);
INT IMGSatLum(INT hdlSource,
			   INT iSaturazione,
			   INT iTonalita,
			   INT iLuminosita,
			   INT iContrasto);

BOOL IMGToClipboard(INT HdlImage);
void RGBtoHSL(double dRed,double dGreen,double dBlue,double *Hue,double *Sat,double *Lum);
void HSLtoRGB(double dHue,double dSat,double dLum,double *Red,double *Green,double *Blue);
