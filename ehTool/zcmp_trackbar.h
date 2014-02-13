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
/*

void * EhListView(SINT cmd,LONG info,void *ptr);
void * EhListViewSort(SINT cmd,LONG info,void *ptr);

void TBItemSet(HWND hwndList,SINT iItem,SINT iSubItem,CHAR *lpText);

//LRESULT ListViewNotify(HWND hWnd, LPARAM lParam);

*/
#define ID_TRACKBAR  50020
typedef struct {
	struct OBJ *lpObj;
	HWND hWndTrack;
	LRESULT (*subPostNotify)(EH_NOTIFYPARAMS);

} EH_TBLIST;
void * EhTrackBar(EH_OBJPARAMS);
