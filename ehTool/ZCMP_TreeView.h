//   ---------------------------------------------
//   | ZCMP_TreeView                                    
//   | ZoneComponent ListView                    
//   |                                              
//   | Gestisce una ListView di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#define ID_TREEVIEW  50010
//#define TVSUB_PRE  0
//#define TVSUB_POST 1
void * ehzTreeView(EH_OBJPARAMS);
#define EhTreeView ehzTreeView
HTREEITEM EhTreeViewSearch(HWND hWnd,LONG lParam);

typedef struct {
	struct OBJ *lpObj;
	HWND hWndList;
	CHAR *lpClickMask;
	LRESULT (*subPostNotify)(EH_NOTIFYPARAMS);
} EH_TVLIST;

typedef struct {
	HTREEITEM hDrag;
	HTREEITEM hDrop;
} EH_TVDRAGDROP;
