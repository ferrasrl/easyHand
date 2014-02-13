//   ---------------------------------------------
//   | ZCMP_ListView                                    
//   | ZoneComponent ListView                    
//   |                                              
//   | Gestisce una ListView di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |  ATTENZIONE:                               
//   |  Inserire in winstart.c                                            
//   |                                              
//   |  case WM_NOTIFY: return ListViewNotify(hWnd, lParam);                                              
//   |                                              
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#define ID_LISTVIEW  50000
void *	ehzListView(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);
void *	ehzListViewSort(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);
#define EhListView ehzListView
#define EhListViewSort ehzListViewSort
//LRESULT ListViewNotify(HWND hWnd, LPARAM lParam);



typedef struct {

	struct OBJ *lpObj;
	HWND	hWndList;
	HWND	hWndHeader;
	INT		idHeader;
	CHAR *	lpClickMask;
	LRESULT (*subListNotify)(EH_NOTIFYPARAMS);
	LRESULT (*subHeaderNotify)(EH_NOTIFYPARAMS);
	LRESULT (*subMessage)(EH_NOTIFYPARAMS);//HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam,BOOL *fReturn);

	BOOL fDoubleClick;
	BOOL fLeftClick;
	BOOL fRightClick;

} EH_LVLIST;

void LVItemSet(HWND hwndList,INT iItem,INT iSubItem,CHAR *lpText);
void LVItemSetParam(HWND hwndList,INT iItem,INT iSubItem,CHAR *lpText,LONG lParam);
void lvItemGetPosition(INT *lpiItem,INT *lpSubItem);
#define LVItemClick lvItemGetPosition
void LVItemGet(HWND hWnd,INT iItem,INT iSubItem,CHAR *lpBuffer,INT iBufferSize);
void LVItemSetCheckAll(HWND hWnd,BOOL bFlag);
void lvAddHeaderCheckbox(HWND  lvTarget);

#ifndef ListView_SetCheckState
   #define ListView_SetCheckState(hwndLV, i, fCheck) \
      ListView_SetItemState(hwndLV, i, \
      INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif
