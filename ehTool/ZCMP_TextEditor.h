//   +-------------------------------------------+
//   | ZCMP_TextEditor                           | 
//   |             Gestione di editor testi      |
//   |             Edita un testo tipo word      |
//   |             versione Windows/ZonaPlus     |
//   |                                           |
//   |             by Ferrà Art & Tecnology 2000 |
//   +-------------------------------------------+

#define WTE_NEW			100 // Nuovo documento
#define WTE_REDRAW		101 // Ridisegna l'oggetto
#define WTE_CURSOR		102 // Settaggio del cursore
#define WTE_CURSORON    103 // Accende il cursore
#define WTE_CURSOROFF   104 // Spegne il cursore
#define WTE_RESET		105 // Reset dei puntatori alla selezione
#define WTE_SAVE		106 // Salva il testo scritto
#define WTE_EDIT		107 // Gestione dell'editing
//#define WTE_APPEND		106 // Appende una riga al documento
#define WTE_DISP		108 // Display di una linea
#define WTE_XCONTROL	109 // Controllo della posizione X
#define WTE_COPY        110 // Copia la selezione
#define WTE_CUT         111 // Taglia la selezione
#define WTE_PASTE       112 // Incolla la selezione
#define WTE_LINEREAD	113 // Legge una linea
#define WTE_LINEWRITE	114 // Scrive una linea
#define WTE_LINEINSERT	115 // Inserisce una linea
#define WTE_LINEAPPEND	116 // Aggiunge una linea in coda
#define WTE_NEWFONT     117 // Nuovo Font

typedef struct
	{CHAR szFind[80];		// Cosa cerco
	 CHAR szReplace[80];	// Stringa da sostituire
	 BOOL fCase;		// Sensibile al Case (TRUE/FALSE)
	 SINT iX;			// Posizione X
	 LONG iY;			// Posizione Y
	 //SINT iDove;		// ?
	 SINT fStart;		// TRUE/FALSE se deve partire dall'inizio
	 SINT xWinPos;
	 SINT yWinPos;
	 BOOL fSelect;		// TRUE/FALSE se deve selezionare la ricerca
	} EH_TEXTEDITORFIND;


typedef struct {
	SINT iRow;
	SINT iChar;
} TED_SEL;

typedef struct
	{
		EH_OBJ *pObj;
		CHAR	FileName[MAXPATH]; // Nome del file
		SINT	Touch;       // Se Š stato cambiato ON/OFF
		SINT	Hdl;         // Hdl del file
		SINT	RigaHdl;     // Handle di linea
		CHAR *	pszBufferRow; // Posizione linea corrente (Ex CampoLinea)
		SINT	iRowSize;
		CHAR  *	Riga2; // Posizione linea corrente

		SINT  Px,Py;
		SINT  Lx,Ly;
		SINT  Col1,Col2;
		SINT  ColCur;

//	 CHAR  Font[30]; // Font di visualizzazione
//	 SINT  FontHdl;
//	 SINT  Nfi;
     LOGFONT LogFont;
	 HFONT   hFont;

	 SINT  SizeLine; // Dimensioni della riga in byte
	 LONG  Lines;    // Linee esistenti 
	 LONG  MaxLines; // Numero massimo di linee possibili
//	 SINT  dispext;  // Display OFF=Interno; ON=Esterno

	 // Calcolate
	 SINT  Px2,Py2;
	 SINT  Falt;     // Altezza del carattere 
	 SINT  Cx,Cy;    // Posizione virtuale del cursore
	 SINT  ncam;     // Numero di righe reali visibili
	 SINT  ncamE;    // Numero di righe editabili
	 LONG  ptre;

	 LONG  Y_offset;
	 LONG  Y_koffset;
	 SINT  X_offset;
	 SINT  X_koffset;
	 SINT  refre;

	 BOOL	bSelection; // T/F se una selezione è attiva
	 TED_SEL	sSelBegin;
	 TED_SEL	sSelEnd;
//	 SINT  SEL_Xstart;
//	 LONG  SEL_Ystart;
//	 SINT  SEL_Xend;
//	 LONG  SEL_Yend;

	 BOOL  fNoYOffsetCalc; // Blocco del calcolo automatico in scroll del Yoffset
//	 SINT  HdlSEL;      // Hdl del COPIA/INCOLLA
//	 LONG  SelLine;      // Numero linee copia incolla
//	 SINT  SelX1;		// Larghezza ultima linea
//	 SINT  SelX2;		// Larghezza ultima linea

	 //HWND  hWnd;        // Handle della finestra
	 BOOL  fHCT;       // Abilita la gestione del testo a colori HCT
     void  * (*FuncExtern)(SINT cmd,LONG info,void *ptr,void *TE,BOOL fSelect,SINT iLine); // Funzione esterna

	 //	 void * (far *sub)(SINT cmd,LONG info,CHAR *str,TEDITFIND *TE); //      CHIAMATA ALLA ROUTINE
	} EH_TEXTEDITOR;

SINT DirectTextEditor(EH_TEXTEDITOR *lpTE,SINT Command,LONG dato,void *ptr);
void * EhTextEditor(EH_OBJPARAMS);

// Head Color Text
typedef struct {
	SINT  lCol1; // RGB colore 1
	SINT  lCol2; // RGB colore 2
	HFONT hFont;  // Handle del Font da usare
	SINT  iLen; // Lunghezza della stringa che segue
} HCT;
/*
typedef struct {
	struct OBJ *lpObj;
	TEDIT TE;
	HWND hWndText;
} EH_TELIST;
*/
