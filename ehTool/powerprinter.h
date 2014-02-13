//  ---------------------------------------------
//	PowerPrinter	   Gestione di un documento in stampa	
//   ---------------------------------------------
#define LR_MAXLINERIGA 10

typedef enum {

	LR_LAYHIDE,
	LR_LAYTYPE0,
	LR_LAYTYPE1,
	LR_PAGEFREE		// Pagina libera: non impostate

} PD_LAY;

#define LR_ADDFIELD    0
#define LR_ADDPAGE     1

#define LR_ROWSDYNAMIC  -100

// Definizione del campo verticale
typedef struct {
	CHAR szTitolo[180];
	INT iLine; // Linea (Per le stampe multilinea)
	INT iRowsSize; // Righe che prende per la stampa (Default=1) LR_ROWSDINAMIC
	BOOL fRowsOver; // true/false per concedere l'over printing in caso di RowsSize (default=False)
	BOOL fCharClean; // true/false per attivare la pulizia in testa e coda dei carattere under ASCII 32
	INT iTipo; // ALFA,NUME,DATA
	INT iAllinea;// 0=Left,1=Center,2=Right 4=Justify(default per i numeri)
	INT iAllineaTitolo;// 0=Left,1=Center,2=Right (default per i numeri)
	INT iMaxByte; // Dimensioni massime
	INT iCifre; // Cifre del numero
	INT iDec; // Decimali
	BOOL fSep; // Separazione migliaia
	BOOL fFix; // Larghezza del carattere fissa
	LONG lColChar; // Colore di default carattere
	LONG lColBack; // Colore di default BackGround
	LONG lColTitChar; // Colore di default Titolo carattere
	LONG lColTitBack; // Colore di default Titolo BackGround

	// Dimensioni
	double xPercSize; // Dimensioni del campo in percentuale 0=Automatica
	double xPercPos;  // Posizione del campo in percentuale 0=Automatica
	BOOL bPadding;	// T/F se si vuole assegnare il padding per colonna
	RECT rPadding; // new 2007 - dimensioni in punti del padding della cella (se non assegnato viene preso di default il generale)

// Usi interno
	LONG   xPosStartDot; // (Interno) posizione fisica del campo in partenza
	LONG   xPosEndDot;   // (Interno) posizione fisica del campo alla fine
} LRFIELDS;

//
// Definizione del documento
//
typedef struct {

	//
	// Caratteristiche del documento
	//
	PD_LAY	iLayStyle;     // Stile del Layout di stampa (LR_PAGEFREE)
	//CHAR	szPrinterName[250];	// Nome della stampante (se non indicato usa quella di default di easyhand)
	CHAR *	pszDeviceDefine;	// Stringa in fomrato DeviceDefince (Se non presente uso stampate di default easyhand)
	INT		iOrientation;		// LPT_LANDSCAPE (non usato se la stampante non è indicata)
	DWORD	dwPrinterFlags;		// Flag : PD_COLLATE
	INT		iPrinternCopies;
	INT		iCopyNumber;		// Numero di copie: Numero di copie
	RECT	rMargin;		// Margine in millimetri
	INT		iCharEncode;	// 0=Default ANSI Es. SE_UTF8

	//
	// Caratteristiche fisiche (ricavate dal driver o indicate da software esterno Es PDF)
	//

	//PRINTDLG sPrinter;			// Struttura per la finestra di dialogo della stampante
	HDC		hdcPrinter;
	DEVMODE	*pDevMode;
	

    INT xInchDot;     // Dot per fare un pollice orizzontale
    INT yInchDot;     // Dot per fare un pollice verticale
	SIZE sPaper;       // Larghezza possibile (x e y) della pagina
	SIZE sPage;        // Larghezza ed altezza della pagina in scrittura (in Dot Real)
	RECT rPage;        // Posizione e dimensione della pagina senza margini (in Dot Real)
	SIZE sPhysicalPage;

	//
	// Layout preimpostato come report
	//
	BOOL fDate;			 // Inserisci data in alto a sinistra
	CHAR *lpDate;		 // Parametro da passare a data_sep
	BOOL fPag;			 // Visualizza il numero di pagina
	INT iPagStyle;      // 0=Pag. n  1= n
	CHAR *lpTitolo;		 // Titolo della stampa
	CHAR *lpSottoTitolo; // Sotto Titolo della stampa

	BOOL fLineVertField; // Flag di separazione campi con linea verticale
	INT iLineVertColor; // Colore linea verticale	
	BOOL fLineHorzField; // Flag di separazione campi con linea orizzontale
	INT iLineHorzColor; // Colore linea orizzontale
	INT iLineHorzStyle; // PS_SOLID
	BOOL bRowsDynamic;   // true/false se ci sono da gestire linee dinamiche

	LONG ptHeadHeight;	 // Altezza testa del report in 1/300 di pollice (0=Automatica)
	LONG ptFootHeight;	 // Altezza piede del report in 1/300 di pollice (0=Automatica) ->  era ptCueHeight
	void *(*HookSubProc)(INT,LONG,void *); //  Sub di PostPainting

	RECT rFieldPadding; // Padding dei campi
	LONG yBodyHeight;     // Altezza del corpo        
    LONG yBodyTop;     // Posizione fisica verticale (in dot) dell'inizio del corpo "righe" (ex yHeadDot)
	LONG yBodyBottom;	// Posizione fisica verticale (in dot) della fine del corpo "righe" 
	LONG yHeadBottom;  // Posizione fisica verticale della testata (senza il titolo del layout)	
    LONG yFootHeight;      // Altezza in dot della "footer" di stampa ->   era yCueHeight
	INT iRowsSize;    // Altezza in righe della linea (Es. ci può essere 1 sola riga di descrizione ma alta (3 righe) per la presenza di campi testo/note dinamici)
	INT iFieldNum;    // Reserved: Numero dei campi totali
	INT iFieldPerRiga[LR_MAXLINERIGA]; // Reserved: Massimo 10 Linee (Numero di campi per linea)
	LRFIELDS **LRFields;// Reserved: (Viene caricato dal LptReport)
	
	INT iLinePerRiga; // Reserved: Numero di linee per riga (linee fisiche che contengono campi) (Viene calcolato in WS_ADD)
	INT iPageCount;   // Reserved: Contatore delle pagine
	INT iRowOffset;		// Posizione fisica della linea corrente (new 2007) ex yCurrentLine
	INT iVirtualLineCount;   // Reserved: Contatore delle linee    
	INT iVirtualLinePerPage; // Reserved: Line "Virtuali" possibili per pagina
	
//	INT iRowsLastLine; // Reserved: Ultime righe dell'ultima linea in costruzione (Per RowsDynamic)
	INT iRowLastHeight;
	INT iRowLastVirtualLine;
	INT iRowMaxHeight;

	// Usati solo durante il REALSET delle linee
	BOOL bBold;
	BOOL bItalic;
	BOOL bUnderLine;
	BOOL fGColor;  // Abilito la modifica globale del colore
	LONG lGColor;
	BOOL fNoDecimal;

	INT xChar;

	INT ptRowHeight;	  // Altezza (in punti) del carattere nella linea (senza padding) (era yChar)
	INT iRowHeight;	  // Altezza fisica in dot del carattere nella linea (senza padding) >> CALCOLATA
	
	INT ptTitleHeight;	  // Altezza in punti (1/300) del titolo (senza padding) (Ex yTitleChar)
	INT iTitleHeight;	  // Altezza fisica in dot del titolo (senza padding)  >> CALCOLATA

	CHAR *lpFontTitleDefault;
	CHAR *lpFontBodyDefault;
	CHAR *lpPrintName; // Nome della stampa (=NULL viene assegnato dal programma)
	BOOL fMessageProcess; // T/F se voglio il mouse_inp durante il loop di stampa

	CHAR *lpszICCFilename; // Puntatore al file di gestione del colore NULL=Standard windows

	INT	iTitlePadded;	// Altezza del titolo+padding sopra e sotto
	INT	iRowPadded;	// Altezza di una riga +padding sopra e sotto

	void * (*funcNotify)(void *, INT cmd, LONG info,CHAR *str); //  funzione esterna di notifica
	LONG lParam; // Per uso esterno

	BOOL bPageInProgress; // T/F se la pagina è corso di completamento

} EH_PD; // Easyhand PowerDoc
#define LRINIT EH_PD


void *PowerPrinter(INT cmd,LONG info,void *ptr);

#define LR_LEFT     0
#define LR_CENTER   1
#define LR_RIGHT    2
#define LR_JUSTIFY  3
#define LR_TOP 4
#define LR_BOTTOM   5

// Definizione del Tipo LINK

typedef struct {
  RECT rRect; // Posizione e dimensione dell'area
  INT iType; // LRA_HEAD,LRA_BODY,LRA_CUE  
  INT iLine; // Se LRA_BODY è il numero di Linea
  LONG lLink; // Dato LONG di riferimento passato da WS_LINK
  BOOL bLastPage; // Indica se è l'ultima pagina su (LRA_CUE)
} LRT_LINK;

typedef struct {
  HDC  hDC;
//  INT xInchToPixel; // Dimensione reale di un Pollice orizzontale
//  INT yInchToPixel; // Dimensione reale di un Pollice verticale
  BOOL fPrinter;	 // E' una richiesta per un DC della stampante
  LRT_LINK *lpLtrLink; // Dato LONG di riferimento passato da WS_LINK
  void (*xyConvert)(INT cmd,void *);
} LRT_LINKEXTERN;

// LptReportArea (per i Link)
#define LRA_HEAD 0
#define LRA_BODY 1
#define LRA_FOOT 2

#define LR_INCHTODOTX  0
#define LR_INCHTODOTY  1
#define LR_DOTTOVIDEOX 2
#define LR_DOTTOVIDEOY 3
#define LR_DOTTOPIXELX 4
#define LR_DOTTOPIXELY 5
#define LR_RECTTOPIXEL 10

// Definizione del Tipo Box/Boxp
typedef struct {
  RECT rRect;
  LONG Colore; 
} LRT_BOX;

// Definizione del Tipo BoxR/Boxpr
typedef struct {
  RECT rRect;
  LONG lColore; 
  LONG lRoundHeight;
  LONG lRoundWidth;
  INT iPenWidth;
} LRT_BOXR;

typedef struct {
  RECT rRect;
  LONG Colore; 
  INT iPenWidth; // Spessore della linea
  INT iStyle; // PS_SOLID,PS_DASH,PS_DOT
  INT iMode; // Modo di scrittura (SET,XOR)
} LRT_LINE;

// Definizione del Font           
typedef struct {
  POINT Point;  // Posizione assoluta in stampa
  INT  xChar;
  INT  yChar;
  LONG  lColore; 
  INT  xAllinea;
  BOOL  fFix; // Larghezza fissa
//  BOOL  fBold;
//  BOOL  fItalic;
//  BOOL  fUnderLine;
  INT iStyles;

} LRT_CHAR;

// Definizione del Font           
typedef struct {
  POINT Point;  // Posizione assoluta in stampa
  INT  xChar;
  INT  yChar;
  LONG  lColore; 
  EN_DPL xAllinea;
  BOOL  fFix; // Larghezza fissa
  INT  iStyles;
  CHAR  szFontName[80];
  INT  iCharEncode;
} LRT_CHAREX;
/*
// Definizione del Font           
typedef struct {
  RECT  rArea;  // Area di stampa
  INT  xChar;
  INT  yChar;
  INT	yInterlinea; // Altezza Interlinea
  LONG  lColore; 
  INT  xAllinea;
  BOOL  fFix; // Larghezza fissa
  BOOL  fBold;
  BOOL  fItalic;
  BOOL  fUnderLine;
  CHAR  szFontName[80];
} LRT_CHARJUST;
*/
typedef struct {
  RECT		rArea;  // Area di stampa
  INT		xChar;
  INT		yChar;
  INT		yInterlinea; // Altezza Interlinea (spazi verticali aggiunti a dimensione carattere)
  LONG		lColore; 
  INT		xAllinea;
  INT		fOver;// True/False se si può stampare fuori dal Box
  BOOL		fFix; // Larghezza fissa
  INT		iStyles;
  CHAR		szFontName[80];
  INT		iExtraCharSpace;
  INT		iMaxRows;
  BOOL		bJustifyLast; // T/F se deve giustificare anche l'ultima linea
} LRT_CHARBOX;

typedef struct {
  INT iPage;
  UINT iType;       // LRT_?
  UINT iOfsDynamics;     // Posizione dei dati dinamici
  UINT iLenDynamics;     // Lunghezza dei dati dinamici
  UINT iLenTag; // Lunghezza complessiva del tag
} LRETAG;

typedef struct {
	//RECT	rRect;
	POINT	ptPos;
	SIZE	sizDim;
	RECT	recImage;
	INT	imgNumber; 
	HIMAGELIST himl;
} LRT_IMAGE;

typedef struct {
	RECT	rArea;		// Area di stampa
	HBITMAP	hBitmap;
	BOOL	bAllocated;
} S_PWD_BITMAP;

/*
// LReport Struct for Item Addiction
typedef struct {
  CHAR  *lpString;
  LRT_CHAR LrtChar;
} SIA_CHAR;

typedef struct {
  CHAR  *lpString;
  LRT_CHAREX LrtCharEx;
} SIA_CHAREX;

  */
// Formula misura:300=x:LRInit.yInchDot
// Converte da centesimi di millimetro a 1/300 di Pollice
#define TMillToInch(p)  (p*300/2540)
/*
// 3,95 pollici sono 10cm
// ne consegue che 10:3,95
// 1cm= 0,395
#define LRInchToDotX(p) (INT) ((double) p*LRInit.xInchDot/300.00)
#define LRInchToDotY(p) (INT) ((double) p*LRInit.yInchDot/300.00)
//#define LRCmToDotX(p) (INT) (p*.395*LRInit.xInchDot/2.54)
// p:2.54=x:LRInit.xInchDot
#define LRCmToDotX(p) (INT) ((double) p*LRInit.xInchDot/2.54)
#define LRCmToDotY(p) (INT) ((double) p*LRInit.yInchDot/2.54)
// p:72=x:LRInit.xInchDot
#define LRPtToDotX(p) (INT) ((double) p*LRInit.xInchDot/72.00)
#define LRPtToDotY(p) (INT) ((double) p*LRInit.yInchDot/72.00)

// Formula misura:LRInit.yInchDot=x:300
// Converte da 1/300 di Pollice a dot
#define LRDotToInchX(p) (p*300/LRInit.xInchDot)
#define LRDotToInchY(p) (p*300/LRInit.yInchDot)
#define LRCenterY(r,b) (((r.bottom-r.top+1)-b)/2) 
#define LRCenterX(r,b) (((r.right-r.left+1)-b)/2) 
*/


INT LRInchToDotX(INT d);
INT LRInchToDotY(INT p);

//#define LRCmToDotX(p) (INT) (p*.395*lpLRInit->xInchDot/2.54)
// p:2.54=x:lpLRInit->xInchDot
INT  LRCmToDotX(double p);
INT  LRCmToDotY(double p);
// p:72=x:lpLRInit->xInchDot
INT  LRPtToDotX(INT p);
INT  LRPtToDotY(INT p);

// Formula misura:lpLRInit->yInchDot=x:300
// Converte da 1/300 di Pollice a dot
INT LRDotToInchX(INT p);
INT LRDotToInchY(INT p);
INT LRCenterY(RECT r,INT b);
INT LRCenterX(RECT r,INT b);

void LRLine(INT x1,INT y1,INT x2,INT y2,INT Color,INT iWidth,INT iStyle,INT iMode);
void LRBoxRound(INT x1,INT y1,INT x2,INT y2,INT Color,INT lRoundWidth,INT lRoundHeight,INT iPenWidth);
void LRBoxpRound(INT x1,INT y1,INT x2,INT y2,INT Color,INT lRoundWidth,INT lRoundHeight,INT iPenWidth);
void LRBox(INT x1,INT y1,INT x2,INT y2,INT Color,INT iWidth);
void LRBoxp(INT x1,INT y1,INT x2,INT y2,INT Color);

void LRHBitmap(INT x1,INT y1,INT x2,INT y2,INT Hdl); // LRHBitmap()
void LRHBitmapDir(INT x1,INT y1,INT Hdl); // LRHBitmapDir()
void LRImage(INT x1,INT y1,INT lx,INT ly,HIMAGELIST himl,INT imgNumber); // new 2008
void LRBitmap(RECT *prcArea,INT iBitsPixel,HDC hdc,HBITMAP hBitmap,BOOL bDupBitmap); // new 2010

void LRDispf(INT px,INT py,INT Color,INT iStyles,EN_DPL iAlign,CHAR *lpFont,INT yChar,CHAR *lpString);
void LRDispLine(INT px,INT py,INT Color,INT iStyles,EN_DPL iAlign,CHAR *lpFont,INT yChar,CHAR *lpString);
void LRDispInBox(RECT rArea,INT Color,INT iStyles,EN_DPL iAlign,CHAR *lpFont,INT yChar,INT iInterLinea,CHAR *lpString,BOOL bJustifyLastRow);
INT LRDispInBoxEx(RECT rArea,INT Color,INT iStyles,EN_DPL iAlign,CHAR *lpFont,INT yChar,INT iInterLinea,CHAR *lpString,BOOL bJustifyLastRow,INT iExtraCharSpace,INT iMaxRows,INT *lpiRows);
void LRDispRiemp(RECT rArea,INT Color,INT iStyles,CHAR *lpFont,INT yChar,CHAR *lpString,CHAR Riemp);
//INT LRGetDispInBoxAlt(LRT_CHARBOX *LCharBox,CHAR *lpString,INT *);
INT LRGetDispInBoxAlt(LRT_CHARBOX *LCharBox,CHAR *lpString,INT *lpiRows); // Lo stile è nel CHARBOX
