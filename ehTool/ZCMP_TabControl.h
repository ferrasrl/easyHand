//   ---------------------------------------------
//   | ZCMP_TabControl                                  
//   | ZoneComponent ListView                    
//   |                                              
//   | Gestisce una ListView di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#define ID_TABCONTROL 50012
#define TCSUB_PRE  0
#define TCSUB_POST 1
void * ehzTabControl(EH_OBJ * pojCalled,EN_MESSAGE cmd,LONG info,void *ptr);
#define EhTabControl ehzTabControl

typedef struct {
	struct OBJ *lpObj;
	HWND hWndList;
	CHAR *lpClickMask;
	LRESULT (*subPostNotify)(SINT iMode,
							 struct OBJ *poj,
						     HWND hWnd, 
						     LPARAM lParam,
						     LPNMHDR  pnmh,
						     BOOL *fReturn);
} EH_TCLIST;

typedef struct {
	HTREEITEM hDrag;
	HTREEITEM hDrop;
} EH_TCDRAGDROP;
