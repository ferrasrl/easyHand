// -------------------------------------
// WTOOLBAR
// Gestore di ToolBar
// -------------------------------------

typedef struct  {
   SINT iType;
   CHAR *lpName;
   CHAR *lpGrp;
   CHAR *lpHmz;
   CHAR *lpIcone;
   BOOL fChecked; // TRUE/FALSE
   BOOL bEnable;  // TRUE/FALSE
} WTOOLBAROBJ;


typedef struct {
  WTOOLBAROBJ *ObjBar;
  SINT			hMemo;
  SINT iElement;
  HWND hWndOwner; // Proprietario
  HWND hWnd;
  HWND hWndTT;
  HWND *hButton; // Lista degli Handle
  SINT WinLx,WinLy; // Dimensioni della finestra
  SINT KeyLx,KeyLy; // Dimensioni della toolbar KeyLx=28,KeyLy=28;
  SINT KeySx,KeySy;//  Distanza in Y ed in X KeySx=3,KeySy=10; // Spazio nella ToolBar
  SINT ClientLx,ClientLy;
} WTOOLBAR;


void ToolSelect(WTOOLBAR *Tb,CHAR *Name);
void ToolClick(WTOOLBAR *Tb,CHAR *Name);
WTOOLBAR *ToolCreate(WTOOLBAROBJ *,SINT KeyLx,SINT KeyLy,SINT KeyNumx);
void ToolDestroy(WTOOLBAR *);
void ToolLock(WTOOLBAR *Tb,CHAR *Name);
void ToolUnlock(WTOOLBAR *Tb,CHAR *Name);


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
 SINT x,y;
 SINT Lx,Ly;
} WTOOLINFO;

void ToolMove(WTOOLBAR *Tb,UINT Mode,WTOOLBAR *TbRef);

void ToolGetInfo(WTOOLBAR *Tb,WTOOLINFO *Ti);
void ToolSet(WTOOLBAR *Tb,WTOOLINFO *Ti);
