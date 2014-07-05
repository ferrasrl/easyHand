//   ---------------------------------------------
//   | ZCMP_TreeView                                    
//   | ZoneComponent TreeView                    
//   |                                              
//   | Gestisce una TreeView di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |         by Ferrà Art & Technology 1993-2000
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ZCMP_TreeView.h"

//#define ID_LISTVIEW 25000

static LRESULT TreeViewNotify(HWND hWnd, LPARAM lParam,INT iLVIndex,LPNMHDR  pnmh);
static BOOL InsertListViewItems(HWND hwndListView);
static HWND CreateTreeView(HINSTANCE hInstance, HWND hwndParent);
static void ResizeListView(HWND, EH_DISPEXT *psExt);
static BOOL InitListView(HWND);
static void SwitchView(HWND hwndListView, DWORD dwView);

#define TVMAX 16
static WNDPROC EhOriginalProc=NULL;
static LRESULT CALLBACK LIntercept(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static EH_TVLIST TVList[TVMAX];
static BOOL fReset=TRUE;
#define TV_FINDOBJ  0
#define TV_FINDHWND 1


static INT TVFind(INT iCosa,void *ptr)
{
	INT a;
	for (a=0;a<TVMAX;a++)
	{
		switch (iCosa)
		{
			case TV_FINDOBJ:  if (TVList[a].lpObj==(struct OBJ *) ptr) return a;
							  break;
			case TV_FINDHWND: if (TVList[a].hWndList==(HWND) ptr) return a;
							  break;
		}
	}
	return -1;
}

static INT LVAlloc(struct OBJ *obj)
{
	INT a;
	for (a=0;a<TVMAX;a++)
	{
	  if (TVList[a].lpObj==NULL)
	  {
		TVList[a].lpObj=obj;
		return a;
	  }
	}
	ehExit("LV: overload");
	return 0;
}

static void EhTreeInizialize(void)
{
	WNDCLASSEX wc;
	// Cambio il puntamento standarda della procedura di classe Eh
	CHAR NewClass[80];
	sprintf(NewClass,"%sT",sys.tzWinClassBase); //win_infoarg("%s",NewClass);
	wc.cbSize=sizeof(wc);
	if (!GetClassInfoEx(sys.EhWinInstance,sys.tzWinClassBase,&wc)) ehExit("Errore 1");
	if (wc.lpfnWndProc!=LIntercept)
	{
		EhOriginalProc=wc.lpfnWndProc;
		wc.lpfnWndProc=LIntercept;
		wc.lpszClassName=NewClass;
		if (!RegisterClassEx(&wc)) win_infoarg("error");
		strcpy(sys.tzWinClassBase,NewClass);
	}
}

void * ehzTreeView(struct OBJ *objCalled,INT cmd,LONG info,void *ptr)
{
 EH_DISPEXT *psExt=ptr;
 static INT HdbMovi=-1;
 static INT16 iSend;
 INT iLVIndex;
 
 if (fReset)
 { 
	if (cmd!=WS_START) win_infoarg("Inizializzare ehzTreeView()");
	memset(&TVList,0,sizeof(EH_TVLIST)*TVMAX);
	EhTreeInizialize();
	fReset=FALSE;
	return 0;
 }

// if (objCalled==NULL) ehExit("ETV: NULL %d",cmd);
 // Cerco una cella in LVLIST
 iLVIndex=TVFind(TV_FINDOBJ,objCalled);
 if (iLVIndex<0) iLVIndex=LVAlloc(objCalled);
 
 switch(cmd)
	{
		case WS_INF: return &TVList[iLVIndex];
 
		case WS_OPEN: // Creazione
			
			objCalled->hWnd=CreateTreeView(sys.EhWinInstance,WindowNow());
			TVList[iLVIndex].hWndList=objCalled->hWnd;
			break;

		case WS_EXTNOTIFY:
			TVList[iLVIndex].subPostNotify=ptr;
			break;

		case WS_SETFLAG:
			
			if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				dwStyle|=info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}
/*
			if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=TreeView_GetExtendedListViewStyle(objCalled->hWnd);
				dwExStyle|=info;
				TreeView_SetExtendedListViewStyle(objCalled->hWnd,dwExStyle);
			}
			*/
			break;

		case WS_CLOSE: // Distruzione
			DestroyWindow(objCalled->hWnd);
			break;

		case WS_ADD: // Aggiungi una colonna
			//TreeView_InsertItem(objCalled->hWnd,(LPTVINSERTSTRUCT) ptr);
			break;

		case WS_REALSET: // Aggiunge un Item
			//TreeView_SetItem(objCalled->hWnd,(LPTVITEM) ptr));
			break;

		case WS_DO: // Spostamento / Ridimensionamento

			MoveWindow(objCalled->hWnd,psExt->px+relwx,psExt->py+relwy,psExt->lx,psExt->ly,TRUE);
			break;

			
		case WS_DISPLAY: break;
	}
	return NULL;
}


/******************************************************************************

   CreateListView

******************************************************************************/

static WNDPROC OldListProc=NULL;
//static LRESULT CALLBACK EhListProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static WNDPROC OldTVProc=NULL;
static LRESULT CALLBACK EhTVProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static HWND CreateTreeView(HINSTANCE hInstance, HWND hwndParent)
{
	DWORD       dwStyle;
	HWND        hwndTreeView;
	BOOL        bSuccess = TRUE;
    CHAR		*lpClass="ehzTreeView";
	WNDCLASS	wndClass;
	dwStyle =   WS_TABSTOP | 
				WS_CHILD | 
				//WS_BORDER |
				WS_CLIPSIBLINGS |
				WS_VISIBLE|
				TVS_HASBUTTONS|
				TVS_SHOWSELALWAYS|
				TVS_HASLINES;

	GetClassInfo(NULL,WC_TREEVIEW,&wndClass);
	wndClass.hInstance=sys.EhWinInstance;
	wndClass.lpszClassName=lpClass;
	OldTVProc=wndClass.lpfnWndProc;
	wndClass.lpfnWndProc=EhTVProc;
	RegisterClass(&wndClass);

	hwndTreeView = CreateWindowEx(WS_EX_CONTROLPARENT|
								  WS_EX_CLIENTEDGE,          // Ex style
                                  lpClass,
								  "ehzTreeView",
								  dwStyle,
								  0,                         // x position
								  0,                         // y position
								  0,                         // width
								  0,                         // height
								  hwndParent,                // parent
								  (HMENU) ID_TREEVIEW,       // ID
								  sys.EhWinInstance,         // Instance
								  NULL);                     // no extra data
	if (!hwndTreeView) return NULL;
	return hwndTreeView;
}

/******************************************************************************
   PositionHeader

   this needs to be called when the ListView is created, resized, the view is 
   changed or a WM_SYSPARAMETERCHANGE message is recieved
******************************************************************************/
/*
static void PositionHeader(HWND hwndListView)
{
HWND  hwndHeader = GetWindow(hwndListView, GW_CHILD);
DWORD dwStyle = GetWindowLong(hwndListView, GWL_STYLE);

	dwStyle |= LVS_NOSCROLL;
	SetWindowLong(hwndListView, GWL_STYLE, dwStyle);

	//only do this if we are in report view and were able to get the header hWnd
	if(((dwStyle & LVS_TYPEMASK) == LVS_REPORT) && hwndHeader)
	{
	RECT        rc;
	HD_LAYOUT   hdLayout;
	WINDOWPOS   wpos;

	GetClientRect(hwndListView, &rc);
	hdLayout.prc = &rc;
	hdLayout.pwpos = &wpos;

	Header_Layout(hwndHeader, &hdLayout);

	SetWindowPos(  hwndHeader, 
                  wpos.hwndInsertAfter, 
                  wpos.x, 
                  wpos.y,
                  wpos.cx, 
                  wpos.cy, 
                  wpos.flags | SWP_SHOWWINDOW);

	ListView_EnsureVisible(hwndListView, 0, FALSE);
   }
}
*/

/**************************************************************************
   ListViewNotify()
   Interpreta le notifiche della listview
**************************************************************************/
static BOOL fDragging=FALSE;
static EH_TVDRAGDROP EhDD;
static HTREEITEM hDragItem;
static HTREEITEM hDropItem;
static HWND hDragWnd=NULL;

//extern HIMAGELIST himl;  // handle to image list 
/*
static DrawTheImage(HIMAGELIST himl)
{
	HDC hDC;
	hDC=GetDC(NULL);
	ImageList_Draw(himl,0,hDC,0,0,ILD_NORMAL);
	ImageList_Draw(himl,1,hDC,20,0,ILD_NORMAL);
	ReleaseDC(NULL,hDC);
}
*/
static void Main_OnBeginDrag(HWND hwndTV, LPNMTREEVIEW lpnmtv) 
{ 
    HIMAGELIST himl;    // handle to image list 
    RECT rcItem;        // bounding rectangle of item 

    // Tell the tree view control to create an image to use 
    // for dragging. 
	hDragWnd=hwndTV;
    himl = TreeView_CreateDragImage(hwndTV, lpnmtv->itemNew.hItem);  // Real OK
	hDragItem=lpnmtv->itemNew.hItem;

    // Get the bounding rectangle of the item being dragged. 
    TreeView_GetItemRect(hwndTV, lpnmtv->itemNew.hItem, &rcItem, TRUE); 
  
    // Get the heading level and the amount that the child items are 
    // indented. 
    //dwLevel = 1;//lpnmtv->itemNew.lParam; 
    //dwIndent = (DWORD) SendMessage(hwndTV, TVM_GETINDENT, 0, 0); 
 
    //ImageList_SetDragCursorImage(himl,0,0,0);
	// Start the drag operation. 

    //ShowCursor(FALSE); 
	ImageList_BeginDrag(himl, 0, 0,0);

	//    ImageList_DragEnter(NULL,0,0);
    ImageList_DragEnter(hDragWnd,0,0);
    //ImageList_DragShowNolock(TRUE);
    // Hide the mouse cursor, and direct mouse input to the 
    // parent window. 
	//DrawTheImage(himl);

    SetCapture(GetParent(hwndTV)); 
    fDragging = TRUE; 
    return; 
} 
 
static void Main_EndDrag(void)//HWND hwndTV, LPNMTREEVIEW lpnmtv) 
{
	INT iIndex;
	CHAR szServ[80];
	//static EHDRAGDROP EhDD;

	//ImageList_DragLeave(NULL);
	ImageList_DragLeave(hDragWnd);
	ImageList_EndDrag(); 
	ReleaseCapture(); 
 
	//ShowCursor(TRUE); 
	fDragging = FALSE;

	// Genero il messaggio di Drag&Drop
    iIndex=TVFind(TV_FINDHWND,hDragWnd);

	ZeroFill(EhDD);
	EhDD.hDrag=hDragItem;
	EhDD.hDrop=hDropItem;
	OBJ_key=(LONG) &EhDD;
	sprintf(szServ,"%sDD",TVList[iIndex].lpObj->nome);
	obj_addevent(szServ);
}

// Main_OnMouseMove - drags an item in a tree view control, 
// highlighting the item that is the target. 
// hwndParent - handle to the parent window. 
// hwndTV - handle to the tree view control.
// xCur and yCur - x- and y-coordinates of the mouse cursor. 

static void Main_OnMouseMove(HWND hwndParent, HWND hwndTV, LONG xCur, LONG yCur) 
{ 
    HTREEITEM htiTarget;  // handle to target item 
    TVHITTESTINFO tvht;  // hit test information 
    //POINT Pos;
	INT iIndex;
	struct OBJ *poj;

	// xCur,yCur sono della parent
    // Drag the item to the current position of the mouse cursor. 
    // Muove l'immagine del Drag nella posizione del cursore
    iIndex=TVFind(TV_FINDHWND,hwndTV);
	poj=TVList[iIndex].lpObj;
	xCur-=poj->px; yCur-=poj->py;
	//GetCursorPos(&Pos);
	//ImageList_DragMove(Pos.x,Pos.y); 
	ImageList_DragMove(xCur+12,yCur+12); 
        
	// Find out if the cursor is on the item. If it is, highlight 
    // the item as a drop target. 
	ZeroFill(tvht);
	tvht.pt.x=xCur; tvht.pt.y=yCur; 
	tvht.flags=TVHT_ONITEM|TVHT_ONITEMLABEL;
    if ((hDropItem = htiTarget = TreeView_HitTest(hwndTV, &tvht)) != NULL) 
	{ 
		ImageList_DragShowNolock(FALSE);
		TreeView_SelectDropTarget(hwndTV, htiTarget); 
		//hDropItem=htiTarget;
		ImageList_DragShowNolock(TRUE);
    }  
    return; 
} 

static void CursorAutoSelect(INT xCur,INT yCur,INT iTVIndex)//HWND hwndTV,struct OBJ *poj)
{
    HTREEITEM htiTarget;  // handle to target item 
    TVHITTESTINFO tvht;   // hit test information 

	//xCur-=TVList[iTVIndex].lpObj->px; 
	//yCur-=TVList[iTVIndex].lpObj->py;

	ZeroFill(tvht);
	tvht.pt.x=xCur; tvht.pt.y=yCur; 
	tvht.flags=TVHT_ONITEM|TVHT_ONITEMLABEL;
    if ((htiTarget = TreeView_HitTest(TVList[iTVIndex].hWndList, &tvht)) != NULL) 
	{ 
		TreeView_SelectDropTarget(TVList[iTVIndex].hWndList, htiTarget); 
		TreeView_SelectItem(TVList[iTVIndex].hWndList,htiTarget);
    } 
}

// -----------------------------------------------------------------------------------------
// Intercettazione della procedura di controllo della Parent del TreeView per le notifiche
// -----------------------------------------------------------------------------------------

static LRESULT CALLBACK LIntercept(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	LPNMHDR  pnmh = (LPNMHDR) lParam;
	INT iTVIndex;

	switch (message)
	{
		case WM_RBUTTONDOWN: break;

		case WM_NOTIFY:
		
		iTVIndex=TVFind(TV_FINDHWND,pnmh->hwndFrom);
		if (iTVIndex>=0)
		{
		 return TreeViewNotify(hWnd, lParam,iTVIndex,pnmh );                                              
		}
		break;


		case WM_MOUSEMOVE:
		if (fDragging) Main_OnMouseMove(hWnd,hDragWnd,LOWORD(lParam),HIWORD(lParam));
		break;

		case WM_LBUTTONUP:
		if (fDragging) Main_EndDrag();
		break;

	}
	return CallWindowProc(EhOriginalProc,hWnd,message,wParam,lParam);
}
 
// -----------------------------------------------------------------------------------------
// Gestione delle notifiche alla windows parent                                                        
// -----------------------------------------------------------------------------------------

static LRESULT TreeViewNotify(HWND hWnd, LPARAM lParam,INT iLVIndex,LPNMHDR pnmh)
{
	LPNMLVKEYDOWN pnkd;

	struct OBJ *poj;
	CHAR szServ[20];
	LPNMHDR lpnmh;
	HTREEITEM hItem;
	LPNMTREEVIEW lpnmtv;
	TVITEM item;
	BOOL fReturn;
	LRESULT lRes;

	if (iLVIndex>100||iLVIndex<0) return 0;

	poj=TVList[iLVIndex].lpObj;
	if (pnmh->code==0) efx2();
	lpnmh = (LPNMHDR) lParam; 

 // Lancio se connessa la procedura di controllo esterna
	if (TVList[iLVIndex].subPostNotify)
	{
		fReturn=FALSE;
		lRes=(*TVList[iLVIndex].subPostNotify)(poj,&fReturn, EXT_PREV,hWnd,0,0,lParam);
		if (fReturn) return lRes;
	}

	switch(pnmh->code)
	{
		case TVN_SELCHANGING:
			 lpnmtv = (LPNMTREEVIEW) lParam;
			 TreeView_SelectDropTarget(lpnmh->hwndFrom,lpnmtv->itemNew.hItem); 

			 ZeroFill(item);
			 hItem=TreeView_GetSelection(lpnmh->hwndFrom);
			 item.mask=TVIF_PARAM; 
			 item.hItem=hItem;
			 item.pszText=szServ;
			 item.cchTextMax=sizeof(szServ);
			 TreeView_GetItem(lpnmh->hwndFrom,&item);
			 OBJ_key=item.lParam;

			 sprintf(szServ,"%sSEL",poj->nome);
			 obj_addevent(szServ);
			 break;
		
		case NM_DBLCLK :
			 ZeroFill(item);
			 hItem=TreeView_GetSelection(lpnmh->hwndFrom);
			 item.mask=TVIF_PARAM; 
			 item.hItem=hItem;
			 item.pszText=szServ;
			 item.cchTextMax=sizeof(szServ);
			 TreeView_GetItem(lpnmh->hwndFrom,&item);
			 OBJ_key=item.lParam;

			 sprintf(szServ,"%sDCLK",poj->nome);
			 obj_addevent(szServ);
			 break;


		case NM_RCLICK:
			 hItem=TreeView_GetSelection(lpnmh->hwndFrom);
			 if (hItem==NULL) break;
			 //memset(&item,0,sizeof(item));
			 ZeroFill(item);
			 item.mask=TVIF_TEXT|TVIF_PARAM; 
			 item.hItem=hItem;
			 item.pszText=szServ;
			 item.cchTextMax=sizeof(szServ);

			 TreeView_GetItem(lpnmh->hwndFrom,&item);
			 //_d_("[%s]      ",szServ);
			 
			 
			 OBJ_key=item.lParam;
			 sprintf(szServ,"%sRC",poj->nome);
			 obj_addevent(szServ);
			 break;

		 case TVN_BEGINDRAG:
		 //case TVN_BEGINRDRAG:
			 Main_OnBeginDrag(lpnmh->hwndFrom, (LPNMTREEVIEW) lParam); 
			 break; 

		//efx1();
		//break;

		case TVN_KEYDOWN:
			pnkd = (LPNMLVKEYDOWN) lParam;   
			if (pnkd->wVKey==ESC) winSetFocus(WindowNow());
			if ((pnkd->wVKey>=' ')&&(pnkd->wVKey<='<')) 
			{
				//efx1();		
			}
			break;
	}

	// Lancio se connessa la procedura di controllo esterna
	if (TVList[iLVIndex].subPostNotify)
	{
		fReturn=FALSE;
		lRes=(*TVList[iLVIndex].subPostNotify)(poj,&fReturn, EXT_AFTER,hWnd,0,0,lParam);
//			(TVSUB_POST,poj,hWnd,lParam,pnmh,&fReturn);
		if (fReturn) return lRes;
	}

	return 0;
}

// -------------------------------------------------------------
// Intercettazione della procedura di classe TreeView
// -------------------------------------------------------------

static LRESULT CALLBACK EhTVProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	INT iTVIndex;
//	PAINTSTRUCT ps;
//	S_WINSCENA WScena;
//	HDC hDC;
	
	switch (message)
	{
		case WM_RBUTTONDOWN:

			//efx2();
			iTVIndex=TVFind(TV_FINDHWND,hWnd);
			//_d_("%d       ",iTVIndex);
			CursorAutoSelect(LOWORD(lParam),HIWORD(lParam),iTVIndex);
			break;
/*

		case WM_PAINT:
			break;

		case WM_KEYDOWN:
		
			nVirtKey = (int) wParam;    
			iHotKey=0;
			fShift=GetAsyncKeyState(VK_SHIFT);

			if (nVirtKey==VK_ESCAPE) iHotKey=EF_ESC;
			if (nVirtKey==VK_RETURN) iHotKey=EF_CR; 
			
			// Arrow Up
			if (nVirtKey==VK_UP)	 iHotKey=EF_FUP;
			// Arrow Down
			if (nVirtKey==VK_DOWN)   iHotKey=EF_FDOWN; 
			// Page Up
			if (nVirtKey==VK_PRIOR)  iHotKey=EF_PAGEUP;
			// Page Down
			if (nVirtKey==VK_NEXT)   iHotKey=EF_PAGEDOWN; 

			// Tab       
			if (nVirtKey==VK_TAB)  
			{
				if (fShift) 
					iHotKey=EF_ALT_TAB; 
					else   
					iHotKey=EF_TAB;
			}

			if ((iHotKey==EF_CR)||(iHotKey==EF_TAB)||(iHotKey==EF_ALT_TAB))
				{GetWindowText(sSetup.hwndEdit,lpEEBuf,iEEBufCount);}
			
			//_d_("[%d]  %d  ",(INT) nVirtKey,(INT) GetAsyncKeyState(VK_SHIFT));
			if (iHotKey) fBreak=TRUE;//DestroyWindow(sSetup.hwndEdit);
			break;
			*/
	}
	
  return CallWindowProc(OldTVProc,hWnd,message,wParam,lParam);
}

static HTREEITEM EhTreeViewItemSearch(HWND hWnd,HTREEITEM hItem,LONG lParam)
{
	HTREEITEM hChild,hOk;
	static INT iLevel=0;
	TVITEM pItem;
	iLevel++;
	while (hItem)
	{
		ZeroFill(pItem);
		pItem.hItem=hItem;
		pItem.state=TVIF_PARAM;
		pItem.stateMask=TVIF_PARAM;
		if (!TreeView_GetItem(hWnd,&pItem)) return NULL;
		//win_infoarg("%d: %d <> %d",iLevel,pItem.lParam,lParam);
		if (pItem.lParam==lParam) {iLevel--; return hItem;}

		hChild=TreeView_GetNextItem(hWnd,hItem,TVGN_CHILD);
		hItem=TreeView_GetNextItem(hWnd,hItem,TVGN_NEXT); 
		if (hChild) 
		{
			hOk=EhTreeViewItemSearch(hWnd,hChild,lParam); if (hOk) {iLevel--; return hOk;}
		}
	}
	iLevel--; 
	return NULL;
}

HTREEITEM EhTreeViewSearch(HWND hWnd,LONG lParam)
{
	return EhTreeViewItemSearch(hWnd,TreeView_GetRoot(hWnd),lParam);
}
