//   ---------------------------------------------
//   | ehzBarFrame
//   | 
//   |                                              
//   |						by Ferrà srl 2013
//   ---------------------------------------------

void * ehzBarFrame(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);

typedef struct {
	/*
	EH_FONT		* psFont;
	BOOL		bFreeFontEnd;	// T/F libera il font in uscita
	EN_STRENC	enEncode;	// T/F se unicode
	WCHAR *		pwcText;
	UINT		uFormat;
	EH_COLOR	colText;
	EH_COLOR	colBack;
	*/
	INT			iWin;
	HWND		hWnd;
	BOOL		bVert;		// T/F se è un barra verticale
	INT			iPos;		// Posizione
	AUTOMOVE *	psAm;
	EH_OBJ *	psObj;
	EH_FONT *	psFont;

	// Limiti Minimo/Massimo
	INT			iMin;
	INT			iMinType;		// 1=In pixel,2=In percentuale sulla grandezza della finestra
	INT			iMax;
	INT			iMaxType;

	BOOL		bMouseOver;
	BOOL		bAction;	// T/F se stiamo spostando
	INT			idMgz;

} EHZ_BARFRAME;
