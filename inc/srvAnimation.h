//   ---------------------------------------------
//   | WAnimation                                
//   | Gestore di zone animate 
//   |                                           
//   |               Created by Tassistro 05/2001  
//   ---------------------------------------------

typedef enum {
	AT_UNDEFINED,	
	AT_CONTAINER,	// Contenitore di Animazioni (DC clone)
	AT_ANIMATION
} EN_ANITYPE;

#define ANI_MILLIBASE 50 // Pausa del thread in millisecondi (Unità base del tempo) equivalente a 20
typedef struct 
{
	EN_ANITYPE	enType;		// 0=Virtual DC, 1=MultiThread Animation
	CHAR *		lpName;			// Nome dell'Area (animazione)
	LONG		lParam;		// Parametro libero (indice di riconoscimento)

	INT			idParent;	// Se FWA_ANICHILD id dell'animazione parente a cui è associata
	EH_OBJ *	psObj;		// !=NULL > Associata ad un oggetto	
	HWND		hWindow;	// !=NULL > Associata Windows "proprietaria" che contiene l'area animazione
							// se FWA_ANICHILD è l'FWA proprietaria
	RECT		rZone;      // Posizione della zona nella DC Windows
	SIZE		sZone;      // Dimensioni dell'area da gestire (solo Type=0)
	INT			iFrame;	    // Ogni quanti fotogrammi chiama il frame il Thread attivato
	INT			iCount;		// Contatore per contare i frames
	LONG		cBackGround;// Colore del background 
	BOOL		bVisible;	// True/False se va vista
	BOOL		bRun;		// True/False dell'animazione (solo tipo 1)
	BOOL		bDcRedraw;	// T/F usato in threadAnimation, quando cambia un figlio
	
	INT			iFrameShow;	   // Frame in visione   -1=Nessuna
	INT			iFrameWrite;   // Frame in Scrittura 

	//CRITICAL_SECTION csFrameLock[2]; // Semaforo di accesso in scrittura al Frame
	HANDLE	mtxLock[2];
	HDC		dcFrame[2];   // Devices contest della Virtual Area (solo tipo 0)
	HBITMAP hBitmapFrame[2]; // Bitmaps che contiene la copia della zona (solo tipo 0)
	BOOL	fRefresh;	// Se và rinfrescata perchè qualcosa è cambiato ???
	void *  (*funcFrame)(EH_SRVPARAMS,HDC hdcTarget); // funzione di creazione del frame

	// Dati interni non toccare
//	HANDLE	hThread;   // Handle del thread (Interno non toccare)
//	DWORD	dwThread;  // 
//	HANDLE  hEvent[2]; // Eventi usati per segnalare azioni (Interno non toccare)
//	INT	iMill;	   // Pausa in ms del frame (Interno non toccare)
//	HDC		hDCTarget; // Usato in WS_DISPLAY è il device contest dove scrivere l'animazione (solo Direct)
	INT		iTimeCount; // Contatore del tempo
	INT		iTimeFrame;  // Limite che determina l'avanzamento del frame
	LONG    lProcess;  // Semaforo di lettura delle informazioni da parte del thread

} S_FWANIM;

typedef struct {
	
	EH_OBJ * psObj;	
	HWND	hWindow;
	HDC		hdc;
	RECT	rcArea;

} S_FW_DISPLAY;

void * srvAnimation(EH_SRVPARAMS);
void * gifAnimation(EH_SRVPARAMS,HDC hdcTarget);

