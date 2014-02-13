//   ---------------------------------------------
//   | EditFloat                                  
//   | Campo di input fluttuante                 
//   |                                           
//   |                                           
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#define ID_EDITFLOAT 10000

// EditFlow
typedef enum {
	EF_UNKONW=0,
	EF_CR=13,
	EF_ESC=27,
	EF_FUP=38,
	EF_FDOWN=40,
	EF_PAGEUP=33,
	EF_PAGEDOWN=34,
	EF_TAB=9, 
	EF_ALT_TAB=2,
	EF_F9=120,
	EF_SELECT=1000,
	EF_BLUR=1001} EN_EF;

#define EFK_CTRL_LEFT 1
#define EFK_CTRL_RIGHT 2


void	EditFloatDestroy(void);
INT		EditFloat(HWND hWndOwner,RECT *Rect,INT iType,INT iView,TCHAR *lpBuffer,INT iSizeBuffer,BOOL fToEnd,void *Ptr,BOOL bAddScroll);
INT		EditFloatEx(HWND hWndOwner,RECT *Rect,INT iType,
				 INT iView,WCHAR *lpString,INT iSizeBuffer,
				 BOOL fToEnd,void *Ptr,
				 CHAR *lpFont,INT iAltFontm,BOOL bAddScroll);
INT		ComboFloat(HWND hWndOwner,RECT *Rect,TCHAR *lpBuffer,INT iSizeBuffer,INT iList,TCHAR **lppList);

CHAR *	MenuFloat(EH_MENUITEM *Ehm,CHAR *lpCodeDefault);
CHAR *	MenuFloatPos(INT x,INT y,EH_MENUITEM *Ehm,CHAR *lpCodeDefault);

EH_MENUITEM * MenuFloatSearch(EH_MENUITEM *Ehm,BYTE *lpItemCode);
void	MenuFloatEnable(EH_MENUITEM *Ehm,BYTE *lpItemCode,BOOL fEnable);
CHAR *	MenuFloatPosWnd(INT x,INT y,EH_MENUITEM *Ehm,CHAR *lpDefault,HWND hWnd);
void	MenuFloatSetText(EH_MENUITEM *Ehm,BYTE *lpItemCode,TCHAR *pText);
void	MenuFloatSetType(EH_MENUITEM *arMenu,BYTE *lpItemCode,INT iType);
void	MenuFloatSetLink(EH_MENUITEM *arMenu,BYTE *lpItemCode,void * pLink);
void	MenuFloatRename(EH_MENUITEM *arMenu,BYTE *lpItemCode,CHAR * pNewCode);

void *	DynMenu(INT cmd,INT iType,CHAR *lpItem,
			   BOOL fEnable,CHAR *lpCode,void *Link,
			   INT iMacroImage,UINT uiEnum,HBITMAP hBmp);

#define DynMenuCreate() DynMenu(WS_OPEN,0,0,0,0,0,0,0,0) // new 2009
#define DynMenuDestroy(ar) DynMenu(WS_CLOSE,0,0,0,NULL,ar,0,0,0) // new 2009

