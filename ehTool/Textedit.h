
typedef struct
	{CHAR Cerco[30];
	 CHAR Replace[30];
	 SINT  X;
	 LONG Y;
	 SINT  Case;
	 SINT  Dove;
	 SINT  Dal;
	} TEDITFIND;


#ifdef _WIN32
typedef struct
	{
	 CHAR  FileName[MAXPATH]; // Nome del file
	 SINT  Touch;       // Se Š stato cambiato ON/OFF
	 SINT  Hdl;         // Hdl del file
	 SINT  RigaHdl;     // Handle di linea
	 CHAR  *CampoLinea; // Posizione linea corrente
	 CHAR  *Riga2; // Posizione linea corrente

	 SINT  Px,Py;
	 SINT  Lx,Ly;
	 SINT  Col1,Col2;
	 SINT  ColCur;

//	 CHAR  Font[30]; // Font di visualizzazione
//	 SINT  FontHdl;
//	 SINT  Nfi;
     LOGFONT LogFont;

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

	 SINT  SEL_Xstart;
	 LONG  SEL_Ystart;
	 SINT  SEL_Xend;
	 LONG  SEL_Yend;

//	 SINT  HdlSEL;      // Hdl del COPIA/INCOLLA
//	 LONG  SelLine;      // Numero linee copia incolla
//	 SINT  SelX1;		// Larghezza ultima linea
//	 SINT  SelX2;		// Larghezza ultima linea

	 HWND  hWnd;        // Handle della finestra
	 //	 void * (far *sub)(SINT cmd,LONG info,CHAR *str,TEDITFIND *TE); //      CHIAMATA ALLA ROUTINE
	} TEDIT;
#else
typedef struct
	{
	 CHAR  FileName[MAXPATH]; // Nome del file
	 SINT  Touch;       // Se Š stato cambiato ON/OFF
	 SINT  Hdl;         // Hdl del file
	 SINT  RigaHdl;     // Handle di linea
	 CHAR  *CampoLinea; // Posizione linea corrente
	 CHAR  *Riga2; // Posizione linea corrente

	 SINT  Px,Py;
	 SINT  Lx,Ly;
	 SINT  Col1,Col2;
	 SINT  ColCur;

	 CHAR  Font[30]; // Font di visualizzazione
	 SINT  FontHdl;
	 SINT  Nfi;

	 SINT  SizeLine; // Dimensioni della riga
	 LONG  Lines;    // Linee esistenti
	 LONG  MaxLines; // Numero massimo di linee
	 SINT  dispext;  // Display OFF=Interno; ON=Esterno

	 // Calcolate
	 SINT  Px2,Py2;
	 SINT  Falt;
	 SINT  Cx,Cy;    // Posizione virtuale del cursore
	 SINT  ncam;     // Numero di righe reali
	 SINT  ncamE;    // Numero di righe editabili
	 LONG  ptre;

	 LONG  Y_offset;
	 LONG  Y_koffset;
	 SINT  X_offset;
	 SINT  X_koffset;
	 SINT  refre;

	 SINT  SEL_Xstart;
	 LONG  SEL_Ystart;
	 SINT  SEL_Xend;
	 LONG  SEL_Yend;

	 SINT  HdlSEL;      // Hdl del COPIA/INCOLLA
	 LONG  SelLine;      // Numero linee copia incolla
	 SINT  SelX1;		// Larghezza ultima linea
	 SINT  SelX2;		// Larghezza ultima linea

	 void * (far *sub)(SINT cmd,LONG info,CHAR *str,TEDITFIND *TE); //      CHIAMATA ALLA ROUTINE
	} TEDIT;
#endif

SINT  TextEditor(CHAR *cmd,LONG dato,void *ptr);

