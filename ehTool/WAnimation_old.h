//   ---------------------------------------------
//   | WAnimation                                
//   | Gestore di zone animate 
//   |                                           
//   |               Created by Tassistro 05/2001  
//   ---------------------------------------------

#define FWA_VIRTUALDC 0
#define FWA_ANIMATION 1
#define FWA_ANICHILD  2

typedef struct 
{
	SINT	iType;		// 0=Virtual DC, 1=MultiThread Animation
	CHAR	*lpName;	// Nome dell'Area
	LONG	lParam;		// Parametro libero (indice di riconoscimento)
	HWND	hOwnerWnd;  // Windows "proprietaria" che contiene l'area animazione
						// se FWA_ANICHILD è l'FWA proprietaria
	RECT	rZone;      // Posizione della zona nella DC Windows
	SIZE	sZone;      // Dimensioni dell'area da gestire (solo Type=0)
	SINT	iFrame;	    // Ogni quanti fotogrammi chiama il frame il Thread attivato
	SINT    iCount;		// Contatore per contare i frames
	LONG	cBackGround;// Colore del background 
	BOOL	fShow;		// True/False se va vista
	BOOL	fRun;		// True/False dell'animazione (solo tipo 1)

	
	SINT	iFrameShow;	   // Frame in visione   -1=Nessuna
	SINT	iFrameWrite;   // Frame in Scrittura 
	//LONG    lFrameLock[2]; // Semaforo di accesso in scrittura al Frame
	CRITICAL_SECTION csFrameLock[2]; // Semaforo di accesso in scrittura al Frame
	HDC		hDCFrame[2];   // Devices contest della Virtual Area (solo tipo 0)
	HBITMAP hBitmapFrame[2]; // Bitmaps che contiene la copia della zona (solo tipo 0)
	BOOL	fRefresh;	// Se và rinfrescata perchè qualcosa è cambiato ???
	void *  (*prcSub)(SINT cmd,LONG info,void *str,HDC hdcTarget); // Sub procedura collegata all'area

	// Dati interni non toccare
	HANDLE	hThread;   // Handle del thread (Interno non toccare)
	DWORD	dwThread;  // 
	HANDLE  hEvent[2]; // Eventi usati per segnalare azioni (Interno non toccare)
	SINT	iMill;	   // Pausa in ms del frame (Interno non toccare)
//	HDC		hDCTarget; // Usato in WS_DISPLAY è il device contest dove scrivere l'animazione (solo Direct)
	LONG    lProcess;  // Semaforo di lettura delle informazioni da parte del thread

} FWANIM;

void *srvWAnimation(SINT cmd,LONG info,void *lpt);
void *GIFAnimation(SINT cmd,LONG info,void *lpPtr,HDC hdcTarget);

