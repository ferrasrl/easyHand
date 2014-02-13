//   ---------------------------------------------
//   | ZCMP_ISearch         
//   ---------------------------------------------

#define ID_ISEARCH 12000
typedef enum {
	ZIS_DEFAULT,
	ZIS_LENS_SELECT=1,
	ZIS_LENS=2,
} EN_ZISTYPE;

void * ehzISearch(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);
#define EhISearch ehzISearch

typedef struct {
	struct OBJ *lpObj;
//	BOOL	bOptionFilter;		// T/F se si vuole l'icona con le opzioni cliccabile
	EN_ZISTYPE	iLayOut;			// 0=Niente, 1=Lente con freccia opzioni , 2=Lente a sinistra senza opzioni
	BOOL		bEventFocus;

	BOOL		bLens;				// T/F se c'è la lente
	RECT		srLens;				// Area occupata dalla lente
	BOOL		bTextEditEnable;	// T/F se è possibile editare il testo
	EH_COLOR	colBackground;		// Default
	EH_COLOR	colOutline;

	BOOL		bLightFocus;		// T/F Se si vuole il campo "illuminato" quando è in focus (TRUE default)
	INT			iRoundCorner;		// Valore della curva dell'angolo (0=Niente, 10 default)
	INT			iShadow;			// 0=no, 1=Obmbra interna
	BOOL		bFocus;				// T/F se sono in fase di input (input focus)
	RECT		sRectInput;			// Rettangolo del campo di input (rettangolo
	SIZE		sSizeInput;			// Dimensioni del campo di input
	RECT		sRectClean;			// Rettangolo dell'icone di clean
	
	BYTE *		pEmptyMessage;		// Messaggio quando il campo è vuoto
	
	INT			iTypeInput;			// ALFA/NUME/DATA
	INT			iFontHeight;		// 0= Altezza automatica proporzionale all'oggetto
	INT			iFontHeightReal;	// Font calcolato
	HWND		hwndInput;
	HFONT		hFont;

	INT			iInputSize;			// Default = 1024
	BOOL		bNotifyEveryKey;	// T/F se deve uscire ad ogni pressione di tasto (default FALSE)
	BYTE	*	pInputValue;		// Valore del campo di input (può essere NULL)
	WCHAR	*	pwInputValue;		// Valore del campo di input in unicode
	BOOL		bButtonClean;		// T/F se è presente il button clean (Uso Interno non toccare)

	INT			iPaddingX;			// (riservato) Padding orizzontale

	BOOL		(*set)(void *,void * pszValue,BOOL bEvent);


} EH_ISEARCH;