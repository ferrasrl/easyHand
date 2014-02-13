//   ---------------------------------------------
//   | ZCMP_ListBox                                
//   | ZoneComponent ListBox
//   |                                              
//   | Gestisce una ListBox di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |  ATTENZIONE:                               
//   |  Inserire in winstart.c                                            
//   |                                              
//   |  case WM_NOTIFY: return ListBoxNotify(hWnd, lParam);                                              
//   |                                              
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#define ID_LISTBOX  52000
#define LVSUB_PRE  0
#define LVSUB_POST 1

void * ehzListBox(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);
void * ehzListBoxSort(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);

#define EhListBox ehzListBox
#define EhListBoxSort ehzListBoxSort

void LBItemSet(HWND hwndList,SINT iItem,SINT iSubItem,CHAR *lpText);
void LBItemSetParam(HWND hwndList,SINT iItem,SINT iSubItem,CHAR *lpText,LONG lParam);

//LRESULT ListViewNotify(HWND hWnd, LPARAM lParam);

typedef struct {
//	struct OBJ *lpObj;
	HWND wnd;
	CHAR *lpClickMask;
	LRESULT (*subPostNotify)(EH_NOTIFYPARAMS);
	/*
	//SINT iMode,
							 struct OBJ *poj,
						     HWND hWnd, 
						     LPARAM lParam,
						     LPNMHDR pnmh,
						     BOOL *fReturn);
							 */
	BOOL fDoubleClick;
	BOOL fLeftClick;
	BOOL fRightClick;
	SINT iMaxLine;
} EHZ_LISTBOX;

/*
typedef struct {
	HWND	wnd;
	LRESULT (*subPostNotify)(SINT iMode,
							 struct OBJ *poj,
						     HWND hWnd,
							 UINT msg,
							 WPARAM wParam,
						     LPARAM lParam,
						     BOOL *fReturn);
	HFONT		hFont;
	EN_STRENC	enEncode;	// T/F se unicode
} EHZ_RICHTEXT;
*/