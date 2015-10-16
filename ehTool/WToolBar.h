// -------------------------------------
// WTOOLBAR
// Gestore di ToolBar
// -------------------------------------

typedef struct  {
   
	INT		iType;
	CHAR *	lpName;
	CHAR *	lpGrp;
	CHAR *	lpHmz;
	CHAR *	lpIcone;
	BOOL	fChecked; // TRUE/FALSE
	BOOL	bEnable;  // TRUE/FALSE

} S_WTOBJ;


typedef struct {

  S_WTOBJ *		arsObj;
  INT			iElement;
  HWND			hWndOwner; // Proprietario
  HWND			hWnd;
  HWND			hWndTT;
  HWND *		arhButton;	// hwnd dei bottoni

  INT			WinLx,WinLy; // Dimensioni della finestra
  INT			KeyLx,KeyLy; // Dimensioni della toolbar KeyLx=28,KeyLy=28;
  INT			KeySx,KeySy;//  Distanza in Y ed in X KeySx=3,KeySy=10; // Spazio nella ToolBar
  INT			ClientLx,ClientLy;

  BOOL			bCapture;
  RECT			rcShot;
  POINT			ptStart;
  POINT			ptNow;

} S_WTOOLBAR;


void ToolSelect(S_WTOOLBAR *Tb,CHAR *Name);
void ToolClick(S_WTOOLBAR *Tb,CHAR *Name);
S_WTOOLBAR * ToolCreate(S_WTOBJ *,INT KeyLx,INT KeyLy,INT KeyNumx);
void ToolDestroy(S_WTOOLBAR *);
void ToolLock(S_WTOOLBAR *Tb,CHAR *Name);
void ToolUnlock(S_WTOOLBAR *Tb,CHAR *Name);


#define WTB_CLIENT  0x00000001
#define WTB_TOOL    0x00000002
#define WTB_TOP     0x00000004
#define WTB_LEFT    0x00000008
#define WTB_RIGHT   0x00000010
#define WTB_BOTTOM  0x00000020
#define WTB_OVERX   0x00000040
#define WTB_OVERY   0x00000080

#define O_TBCOLORRADIO 200

typedef struct {
 INT x,y;
 INT Lx,Ly;
} WTOOLINFO;

void ToolMove(S_WTOOLBAR *Tb,UINT Mode,S_WTOOLBAR *TbRef);
void ToolGetInfo(S_WTOOLBAR *Tb,WTOOLINFO *Ti);
void ToolSet(S_WTOOLBAR *Tb,WTOOLINFO *Ti);
