//  ---------------------------------------------
//	PowerDoc
//  Creazione di un documento
//
//  Ferrà srl 2010
//  ---------------------------------------------

#ifndef EH_POWERDOC

#define EH_POWERDOC 1
#define LR_MAXLINERIGA 10

void * PowerDoc(EH_SRVPARAMS);

typedef enum {

	PWD_LAYHIDE,
	PWD_LAYTYPE0,
	PWD_LAYTYPE1,
	PWD_PAGEFREE		// Pagina libera: non impostate

} PWD_LAY;

typedef double PWD_VAL;

typedef struct {

	PWD_VAL left;
	PWD_VAL top;
	PWD_VAL right;
	PWD_VAL bottom;

} PWD_RECT;

typedef struct {

	PWD_VAL cx;
	PWD_VAL cy;

} PWD_SIZE;

typedef struct {

	PWD_VAL x;
	PWD_VAL y;

} PWD_POINT;

typedef struct {

	DWORD		cCount;
	PWD_POINT	arsPoint[1];

} PWD_BEZIER;


typedef enum {
	
	PDA_LEFT=0x0001,
	PDA_CENTER=0x0002,
	PDA_RIGHT=0x0004,
	PDA_JUSTIFY=0x0008,

	PDA_TOP=0x0010,
	PDA_MIDDLE=0x0020,
	PDA_BOTTOM=0x0040

} PWD_ALIGN;

typedef struct {

	CHAR 		szFontFace[100];
	PWD_VAL		umCharHeight;
	PWD_VAL		umCharWidth;
	EH_TSTYLE	enStyles;
	BOOL		bFixWidth;
	DWORD		dwWeight;
	DWORD		dwItalic;
	HFONT		hFont;			// Handle windows
	void	*	pVoid;			// Valore esterno (usato per pdf)

} PWD_FONT;


typedef enum {

	PDT_UNKNOW,
	PDT_ENDFILE,	// Ultimo Tag

//#define PDT_TEXT      1
//	PDT_CHAR,//=2,
//	PDT_CHAREX,//=3,
	/*
	PDT_BOX,//=4,
	PDT_BOXP,//=5,
	PDT_BOXR,//=11,
	PDT_BOXPR,//=12,
	*/
	PDT_LINE,//=9,
	PDT_RECT,
	PDT_TEXT,//=10, // New 2002 Testo in box
	PDT_TEXTBOX,//=10, // New 2002 Testo in box

	PDT_EHIMG,//=7,
//	PDT_EHIMGD,//=8, // Stampa DIRETTA senza strech
	PDT_IMAGELIST,//=14,  // New 2008: collegato a imagelist
	PDT_BITMAP, // Bitmap Windows
	PDT_PATH,//=Percorso

	PDT_ERROR

//#define PDT_CHARJUST  8

} PWD_TE; // PowerDocumentTypeElement

typedef struct {

	PWD_TE		enType;			 // PDT_?
	INT			iPage;			 // Pagina di apparteneza
	PWD_RECT	rumObj;			 // Posizione dell'oggetto (in centesimi di millimetro)
	INT			iLenItem;		 // Lunghezza complessiva del item > Compreso struttura successiva psObj + dati extra
//	BYTE		psObj[1];

} PWD_ITEM;

typedef struct {

	HDC			hDC;
	RECT		recObj;
	PWD_ITEM *	psItem;

//	void (*xyConvert)(INT cmd,void *);
} PWD_DRAW;

typedef enum {
	PWC_QUICK,	// usa color EH_COLOR
	PWC_RGB,
	PWC_CMYK
} PWD_COLOR_FORMAT;


typedef struct {
	
	PWD_COLOR_FORMAT	enFormat;
	PWD_VAL				dAlpha;
	PWD_VAL				dComp[4];		// Color component (RGB o CMYK)
	EH_COLOR			ehColor;		// Colore standard easyhand

} PWD_COLOR;

#define LR_ADDFIELD    0
#define LR_ADDPAGE     1

#define LR_ROWSDYNAMIC  -100

// Definizione del campo verticale
typedef struct {
	CHAR		szTitolo[180];
	INT			iLineInRow; // Linea (Per le stampe multilinea sulla singola riga)
	INT			iRowsSize;	// Righe che prende per la stampa (Default=1) LR_ROWSDINAMIC
	BOOL		fRowsOver; // true/false per concedere l'over printing in caso di RowsSize (default=False)
	BOOL		fCharClean; // true/false per attivare la pulizia in testa e coda dei carattere under ASCII 32
	INT			iTipo; // ALFA,NUME,DATA
	PWD_ALIGN	enAlign;// 0=Left,1=Center,2=Right 4=Justify(default per i numeri)
	PWD_ALIGN	enTitleAlign;// 0=Left,1=Center,2=Right (default per i numeri)
	INT			iMaxByte; // Dimensioni massime
	INT			iCifre; // Cifre del numero
	INT			iDec; // Decimali
	BOOL		fSep; // Separazione migliaia
	BOOL		fFix; // Larghezza del carattere fissa
	PWD_COLOR	colText; // Colore di default carattere
	PWD_COLOR	colBack; // Colore di default BackGround
	PWD_COLOR	colTitText; // Colore di default Titolo carattere
	PWD_COLOR	colTitBack; // Colore di default Titolo BackGround

	// Dimensioni
	double		xPercSize; // Dimensioni del campo in percentuale 0=Automatica
	double		xPercPos;  // Posizione del campo in percentuale 0=Automatica
	BOOL		bPadding;	// T/F se si vuole assegnare il padding per colonna
	PWD_RECT	rumPadding; // new 2007 - dimensioni in punti del padding della cella (se non assegnato viene preso di default il generale)

	//
	// Usi interno
	//
	double		umPosStartDot; // (Interno) posizione fisica del campo in partenza
	double		umPosEndDot;   // (Interno) posizione fisica del campo alla fine

} PWD_FIELD;

typedef enum {
	PUM_STD, // 300simi di pollice
	PUM_MM,
	PUM_CM,
	PUM_INCH,
	PUM_PT,		// 1/72
	PUM_DTX,	// Dot fisici X (Reali sempre associati alla risoluzione del documento/device)
	PUM_DTY,	// Dot fisici Y (Reali sempre associati alla risoluzione del documento/device)
	PUM_DTXD,	// Dot fisici X (dinamico in base all'hDC)
	PUM_DTYD	// Dot fisici Y (dinamico in base all'hDC)
	/*

	PUM_PHX=100,	// Trasforma UM > in Dot fisici "X" 
	PUM_PHY,		// Trasforma UM > in Dot fisici "Y"
	PUM_PHX=100,	// Trasforma UM > in Dot fisici "X" (dinamico in base all'hDC)
	PUM_PHY,		// Trasforma UM > in Dot fisici "Y"
	PUM_UM_TO_PHX,	// Trasforma UM > in Dot fisici "X" (Reali sempre associati alla risoluzione del documento)
	PUM_UM_TO_PHY,	// Trasforma UM > in Dot fisici "Y"

	PUM_UM_INCH=200,// Da UM > a Inch
	PUM_PHX_TO_UM=201, // Trasforma i Dot Fisici "X" > in UM
	PUM_PHY_TO_UM=202	// Trasforma i Dot Fisici "Y" > in UM
*/
//	PUM_UM_PT

} PWD_UM;

double pwdUm(PWD_UM enUm,double dValore); // Ritorna un valore, espresso in diverse unirà di misura, nella misura corrente
double pwdUmTo(double dValore,PWD_UM enUmTo); // Converte un valore nell'attuale Um, in una misura scelta


typedef struct {

	INT		iPaper;			// DMPAPER_A4
	CHAR		szPaperName[80];
	INT		iOrientation;   // DMORIENT_PORTRAIT / DMORIENT_LANDSCAPE
	PWD_RECT	rumMargin;		// Margini in mm
	PWD_SIZE	sizForm;		// Dimensioni in mm

} PWD_FORMINFO;

BOOL ehPdfGetForm(CHAR *pszDeviceDefine,PWD_FORMINFO *psFormInfo);

//
// Definizione del documento
//
typedef struct {

	DWORD		iVer;				// 0= Default

	//
	// Caratteristiche del documento
	//
	PWD_LAY		enLayStyle;			// Stile del Layout di stampa (LR_PAGEFREE)
	PWD_UM		enMisure;			// Unità di misura con cui vengono indicate le dimensioni 
									// 0 = (default) 1/300 (trecentesimi di pollice) per compatibilità
	CHAR	*	pszDeviceDefine;	// Stringa in fomrato DeviceDefince (Se non presente uso stampante di default easyhand)
	INT			iOrientation;		// LPT_LANDSCAPE (non usato se la stampante non è indicata)
	DWORD		dwPrinterFlags;		// Flag : PD_COLLATE
	INT			iPageCopies;		// se PD_COLLATE: COpia pagina (Es 5 stampe (iCopyNumber) ma si stampa 10 volte la stessa pagina) iPrinternCopies=10
	INT			iCopyNumber;		// Numero di copie: Numero di copie
	PWD_RECT	rumMargin;			// Margine in enMisure
	EN_STRENC	enCharEncode;		// 0=Default ANSI Es. SE_UTF8

	//
	// Caratteristiche fisiche (ricavate dal driver o indicate da software esterno Es PDF)
	//

	//PRINTDLG sPrinter;			// Struttura per la finestra di dialogo della stampante
	HDC			hdcPrinter;
	DEVMODE	*	pDevMode;

	SIZE		sizDotPerInch;
	PWD_SIZE	sumPage;        // Larghezza ed altezza della pagina in scrittura (in Um)
	PWD_RECT	rumPage;        // Posizione e dimensione della pagina senza margini (in Um)
	PWD_SIZE	sumPhysicalPage;
	PWD_SIZE	sumPaper;       // Larghezza possibile (x e y) della pagina

	//
	// Layout preimpostato come report
	//
	BOOL		fDate;			 // Inserisci data in alto a sinistra
	CHAR *		lpDate;		 // Parametro da passare a data_sep
	BOOL		fPag;			 // Visualizza il numero di pagina
	INT			iPagStyle;      // 0=Pag. n  1= n
	CHAR *		lpTitolo;		 // Titolo della stampa
	CHAR *		lpSottoTitolo; // Sotto Titolo della stampa

	BOOL		fLineVertField; // Flag di separazione campi con linea verticale
	INT			iLineVertColor; // Colore linea verticale	
	BOOL		fLineHorzField; // Flag di separazione campi con linea orizzontale
	PWD_COLOR	colLineHorzColor; // Colore linea orizzontale
	INT			iLineHorzStyle; // PS_SOLID
	BOOL		bRowsDynamic;   // true/false se ci sono da gestire linee dinamiche

	PWD_VAL		umHeadHeight; // Altezza testa del report Um di pollice (0=Automatica)
	PWD_VAL		umFootHeight; // Altezza piede del report Um di pollice (0=Automatica) ->  era ptCueHeight
	void *		(*funcNotify)(EN_MESSAGE,LONG,void *); //  Funzione per le notifiche esterne

	PWD_RECT	rumFieldPadding; // Padding dei campi

	PWD_VAL		umBodyHeight;    // Altezza del corpo        
    PWD_VAL		umBodyTop;		// Posizione fisica verticale (in dot) dell'inizio del corpo "righe" (ex yHeadDot)
	PWD_VAL		umBodyBottom;	// Posizione fisica verticale (in dot) della fine del corpo "righe" 
	PWD_VAL		umHeadBottom;	// Posizione fisica verticale della testata (senza il titolo del layout)	
//    double umFootHeight;    // Altezza in dot della "footer" di stampa ->   era yCueHeight
	PWD_VAL		umTitlePadded;	// Altezza del titolo+padding sopra e sotto
	PWD_VAL		umRowPadded;	// Altezza di una riga fisica effettiva (testo+padding sopra e sotto)


	INT			iRowsSize;    // Altezza in righe della linea (Es. ci può essere 1 sola riga di descrizione ma alta (3 righe) per la presenza di campi testo/note dinamici)
	INT			iFieldNum;    // Reserved: Numero dei campi totali
	INT			iFieldPerRiga[LR_MAXLINERIGA]; // Reserved: Massimo 10 Linee (Numero di campi per linea)
//	PWD_FIELD **arsField;	// Reserved: (Viene caricato dal LptReport)
	
	INT			iLinePerRiga; // Reserved: Numero di linee per riga (linee fisiche che contengono campi) (Viene calcolato in WS_ADD)
	INT			iPageCount;   // Reserved: Contatore delle pagine
	PWD_VAL		umRowCursor;		 // Posizione fisica della linea corrente (new 2007) ex yCurrentLine
	INT			iVirtualLineCount;   // Reserved: Contatore delle linee    
	INT			iVirtualLinePerPage; // Reserved: Line "Virtuali" possibili per pagina
	
//	INT iRowsLastLine; // Reserved: Ultime righe dell'ultima linea in costruzione (Per RowsDynamic)
	PWD_VAL		umRowLastHeight;	// report: Altezza dell'ultima riga stampata
	PWD_VAL		umRowMaxHeight;		// report: dimensioni massime di una riga in stampa
//	INT			iRowLastVirtualLine;

	// Usati solo durante il REALSET delle linee
	BOOL		bBold;
	BOOL		bItalic;
	BOOL		bUnderLine;
	BOOL		fGColor;  // Abilito la modifica globale del colore
	PWD_COLOR	colGlobalText; // ex lGColor
	BOOL		fNoDecimal;

	INT			xChar;

	PWD_VAL		umRowHeight;	  // Altezza in um del carattere nella linea (senza padding) (era yChar)
	//INT iRowHeight;	  // Altezza fisica in dot del carattere nella linea (senza padding) >> CALCOLATA
	
	PWD_VAL		umTitleHeight;	  // Altezza in UM del carattere del titolo del corpo (senza padding) (Ex yTitleChar)
	//INT iTitleHeight;	  // Altezza fisica in dot del titolo (senza padding)  >> CALCOLATA

	CHAR *		pszFontTitleDefault;
	CHAR *		pszFontBodyDefault;
	CHAR *		pszDocumentName; // Nome della stampa (=NULL viene assegnato dal programma)
	BOOL		fMessageProcess; // T/F se voglio il mouse_inp durante il loop di stampa

	CHAR *		lpszICCFilename; // Puntatore al file di gestione del colore NULL=Standard windows

	//void * (*funcNotify)(void *, INT cmd, LONG info,CHAR *str); //  funzione esterna di notifica
	LONG		lParam; // Per uso esterno
	BOOL		bPageInProgress; // T/F se la pagina è corso di completamento
	CHAR *		pszTempFolder;	// NULL = Usa il folder di sistema, altrimenti quello indicato

	_DMI		dmiFont;		// Font usati
	PWD_FONT *	arsFont;		// Puntatore alla memoria
	EH_LST		lstFontRes;		// Font allocato provvisoriamente

#ifdef EH_PDF
	BOOL		bPdf;			// T/F se faccio il PDF diretto
	BOOL		bPdfShow;		// T/F se devo mostrare il pdf alla fine
	BOOL		bPdfChooseFile;	// T/F se deve scegliere il file di output
	CHAR *		pszPdfEncode;	// NULL = ISO8859-1
	CHAR *		pszPdfFileName;	// NULL = Chiedo il nome	
	CHAR		szWinDir[MAX_PATH];	// Directory di windows (serve per i fonts)
	EH_LST		lstPdfReport;
#endif

} EH_PWD; // Easyhand PowerDoc


//
// Definizione del subarea da stampare
// LptReportArea (per i Link)
//
typedef enum {
	PDN_HEAD,
	PDN_BODY,
	PDN_FOOT
} EN_PDN; // Power Document Notify

typedef struct {

	//RECT rRect; // Posizione e dimensione dell'area
	EH_PWD	*	psPower;
	EN_PDN		enType; // PDN_HEAD,PDN_BODY,PDN_FOOT
	PWD_RECT	rumRect;
	INT			iLine; // Se LRA_BODY è il numero di Linea
	LONG		lLink; // Dato LONG di riferimento passato da WS_LINK
	BOOL		bLastPage; // Indica se è l'ultima pagina su (LRA_CUE)

} PWD_NOTIFY;


/*
#define LR_INCHTODOTX  0
#define LR_INCHTODOTY  1
#define LR_DOTTOVIDEOX 2
#define LR_DOTTOVIDEOY 3
#define LR_DOTTOPIXELX 4
#define LR_DOTTOPIXELY 5
#define LR_RECTTOPIXEL 10
*/
// Definizione del Tipo Box/Boxp

typedef struct {

  PWD_COLOR	colPen; 
  PWD_VAL	umPenWidth;	// Spessore della linea
  INT		iPenStyle;		// PS_SOLID,PS_DASH,PS_DOT

  PWD_COLOR	colBrush; 
  INT		iBrushStyle;	// 

} PWD_PB;

typedef struct {

  PWD_PB	sPenBrush;
  PWD_SIZE	sumRound;	// Bordi arrotondati

} PDO_BOXLINE;

typedef struct {

	CHAR *		pszText;
	CHAR *		pszFontFace; // NULL= Default	
	PWD_VAL		umCharWidth;
	PWD_VAL		umCharHeight;
	BOOL		fFix; // Larghezza fissa
	PWD_COLOR	colText; 
	PWD_ALIGN	enAlign;
	EH_TSTYLE	enStyles;
	DWORD		dwWeight;
	DWORD		dwItalic;
	DWORD		dwUnderline;
	//CHAR		szFontName[80];
  
	PWD_VAL		umInterlinea; // Altezza Interlinea (spazi verticali aggiunti a dimensione carattere)
	PWD_VAL		umExtraCharSpace;

	INT			fOver;// True/False se si può stampare fuori dal Box
	INT			iMaxRows;
	BOOL		bJustifyLast; // T/F se deve giustificare anche l'ultima linea
	BOOL		bBaseLine;		// y è la baseline
	
	INT			idxFont;	// Indice del font usato in _sPower.dmiFont
	

} PDO_TEXT;


typedef struct {
//	RECT	rRect;
//	INT	imgNumber;  // handle/img number dell' image list
//	BOOL	bAllocated;	// Se ha bisogno di essere disallocato alla fine della generazione
	INT	hImage;		// Handle (easyhand) della immagine in memoria 	0 = file temporaneo
	HIMAGELIST himl;	// Handle Image List 
	HBITMAP	hBitmap;	// Handle del bitmap (se in memoria)	NULL= file temporaneo

	CHAR	*pszFileTemp;	// Nome del file temporaneo ; NULL = non ho file temporaneo
//	BOOL	bTempFile;		// T/F se devo li
} PDO_IMAGE;


typedef enum {

	PHT_UNKNOW,
	PHT_MOVETO,
	PHT_LINETO,
	PHT_POLYBEZIERTO,
	PHT_CLOSEFIGURE // Chiudo la figura

} EN_PHT;


typedef struct {

	EN_PHT	enType;
	DWORD	dwSizeData;
	void *	psData;

} PWD_PTHE;

typedef struct {

	INT			iBrushStyle;	// 
	PWD_PB		sPenBrush;
	INT			iElements;		// Numero di punti del percorso
	DWORD		dwElements;		// Dimensioni in byte degli elementi
	PWD_PTHE *	psEle;			// Puntatore al primo elemento
	
} PDO_PATH;


typedef struct {

	CHAR	szName[128];
	CHAR	szFileName[500];
	BOOL	bAddInFile;

} PWD_FONT_RES;



//
// API per la scrittura del documento
//

PWD_POINT *	pwdPointFill(PWD_POINT *ppum,PWD_VAL left,PWD_VAL top);
PWD_SIZE *	pwdSizeFill(PWD_SIZE *psum,PWD_VAL width,PWD_VAL height);
PWD_RECT *	pwdRectFill(PWD_RECT *prumRect,PWD_VAL left,PWD_VAL top,PWD_VAL right,PWD_VAL bottom);

void		pwdSizeCalc(PWD_SIZE *psumSize,PWD_RECT *prumRect);
PWD_VAL		pwdGetTextInRectAlt(PWD_RECT *prRect,PDO_TEXT *psText,INT *lpiRows);
PWD_COLOR 	pwdColor(EH_COLOR ehCol);
PWD_COLOR	pwdRGB(PWD_VAL dRed,PWD_VAL dGreen,PWD_VAL dBlue,PWD_VAL dAlpha);
PWD_COLOR	pwdCMYK(PWD_VAL dCyan,PWD_VAL dMag,PWD_VAL dYel,PWD_VAL dKey,PWD_VAL dAlpha);
#define		pwdGray(a) pwdCMYK(0,0,0,a,1.0)

#define		PDC_BLACK	pwdCMYK(0,0,0,1,1)
#define		PDC_RED		pwdCMYK(0,.99,1,0,1)
#define		PDC_YELLOW	pwdCMYK(0,0,1,0,1)
#define		PDC_BLUE	pwdCMYK(0.72,.54,0,.45,1)
#define		PDC_WHITE	pwdCMYK(0,0,0,0,1)
#define		PDC_TRASP	pwdCMYK(0,0,0,0,0)

BOOL		pwdFontAdd(CHAR * pszNameFont, UTF8 * pszFileName,BOOL bAddToDoc);

void		pwdLine(PWD_RECT *prumRect,PWD_COLOR colPen,PWD_VAL iWidth,INT iStyle);
void		pwdRect(PWD_RECT *prumRect,PWD_COLOR colPen,PWD_COLOR colSolidBrush,PWD_VAL iWidth);
void		pwdRectRound(PWD_RECT *prumRect,PWD_COLOR colPen,PWD_COLOR colSolidBrush,PWD_VAL lRoundWidth,PWD_VAL lRoundHeight,PWD_VAL iPenWidth);
void		pwdRectEx(PWD_RECT *prumRect,PWD_COLOR colPen,PWD_VAL iPenWidth,INT iPenStyle,PWD_COLOR colBrush,INT iBrushStyle,PWD_VAL umRoundWidth,PWD_VAL umRoundHeight);

void		pwdText(PWD_VAL px,PWD_VAL py,PWD_COLOR colText,EH_TSTYLE enStyles,EN_DPL nAlign,CHAR *pszFont,PWD_VAL umHeight,CHAR *pszString);
//void pwdTextLine(PWD_VAL px,PWD_VAL py,EH_COLOR colText,EH_TSTYLE enStyles,EN_DPL nAlign,CHAR *pszFont,PWD_VAL umHeight,CHAR *pszString); //INT px,INT py,INT Color,INT iStyles,EN_DPL iAlign,CHAR *lpFont,PWD_VAL umHeight,CHAR *pszStr);
void		pwdTextInRect(PWD_RECT *prum,PWD_COLOR colText,EH_TSTYLE enStyles,PWD_ALIGN iAlign,CHAR *lpFont,PWD_VAL umHeight,PWD_VAL umInterLinea,CHAR *pszStr,BOOL bJustifyLastRow);
PWD_VAL		pwdTextInRectEx(PWD_RECT *prum,PWD_COLOR colText,EH_TSTYLE enStyles,PWD_ALIGN enAlign,CHAR *pszFont,PWD_VAL umCharHeight,PWD_VAL umInterLinea,CHAR *pszStr,BOOL bJustifyLastRow,PWD_VAL umExtraCharSpace,INT iMaxRows,INT *lpiRows);
void		pwdTextRiemp(PWD_RECT *prum,PWD_COLOR colText,EH_TSTYLE enStyles,CHAR *lpFont,PWD_VAL umHeight,CHAR *pszStr,CHAR Riemp); // Non implementato
void		pwdTextJs(PWD_VAL umX,PWD_VAL umY,CHAR * pszParams,CHAR *pszText);

void		pwdImgCalc(PWD_SIZE *psSize,INT hImage);	// Utility
void		pwdImage(PWD_VAL px,PWD_VAL py,PWD_SIZE *psumRect,INT hldImg,BOOL bStore); // LRHBitmap()
void		pwdBitmap(PWD_VAL px,PWD_VAL py,PWD_SIZE *psumSize,HBITMAP hBitmap,BOOL bStore,DWORD dwColorDeepBit);
void		pwdImageList(PWD_VAL px,PWD_VAL py,PWD_SIZE *psumRect,HIMAGELIST himl,INT imgNumber,BOOL bStore); // new 2010

PWD_FIELD * pwdColAdd(INT iType,PWD_ALIGN enAlign,double dPerc,CHAR * pszName);
PWD_FIELD *	pwdGetField(INT a,CHAR *pszName);
void		pwdSet(INT iCol,void * pValue);
void		pwdSetEx(INT iCol,void * pValue,PWD_COLOR colText,PWD_COLOR colBack,EH_TSTYLE enStyle,PWD_ALIGN enAlign);

#define 	pwdColBegin()	PowerDoc(WS_OPEN,0,NULL)
#define		pwdColEnd()		PowerDoc(WS_DO,0,NULL);
			
#define 	pwdRowBegin()	PowerDoc(WS_LINK,0,NULL)
#define		pwdRowEnd()		PowerDoc(WS_INSERT,0,NULL) 

void		pwdMetaFile(PWD_POINT *	ppumPos,		// Posiziono il meta // 2013/2014
						PWD_SIZE  *	psumSize,		// Indico dimensioni orizzontali
						PWD_ALIGN	enAlign,		// Posizionamento
						UTF8 *		pszFileNameUtf,
						BOOL		bBestInFit,		// T/F se deve calcolare la maggiore dimensione possibile
						BOOL		bOnlyCalc,		// T/F Solo calcolo del posizionamento, usato per predeterminare l'occupazione e la dimenzione
						PWD_RECT  *	precMeta);		// Ritorna l'area occupata (se richiesto)
						


//void pwdBitmap(PWD_VAL px,PWD_VAL py,PWD_SIZE *psumRect,INT iBitsPixel,HDC hdc,HBITMAP hBitmap,BOOL bDupBitmap); // new 2010

#endif


