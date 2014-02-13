//   ---------------------------------------------
//   | ZCMP_TreeView                                    
//   | ZoneComponent TreeView                    
//   |                                              
//   | Gestisce una TreeView di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#include "\ehtool\include\ehsw_i.h"
#include "\ehtool\look98.h"
#include "\ehtool\dmiutil.h"
#include "\ehtool\fbfile.h"
#include "\ehtool\ZCMP_TreeView.h"

//#define ID_LISTVIEW 25000

static LRESULT TreeViewNotify(HWND hWnd, LPARAM lParam,SINT iLVIndex,LPNMHDR  pnmh);
static BOOL InsertListViewItems(HWND hwndListView);
static HWND CreateTreeView(HINSTANCE hInstance, HWND hwndParent);
static void ResizeListView(HWND, struct WS_DISPEXT *DExt);
static BOOL InitListView(HWND);
static void SwitchView(HWND hwndListView, DWORD dwView);

#define TVMAX 16
static WNDPROC EhOriginalProc=NULL;
static LRESULT CALLBACK LIntercept(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static EH_TVLIST TVList[TVMAX];
static BOOL fReset=TRUE;
#define TV_FINDOBJ  0
#define TV_FINDHWND 1

static SINT TVFind(SINT iCosa,void *ptr)
{
	SINT a;
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

static SINT LVAlloc(struct OBJ *obj)
{
	SINT a;
	for (a=0;a<TVMAX;a++)
	{
	  if (TVList[a].lpObj==NULL)
	  {
		TVList[a].lpObj=obj;
		return a;
	  }
	}
	PRG_end("LV: overload");
	return 0;
}

static void EhTreeInizialize(void)
{
	WNDCLASSEX wc;
	// Cambio il puntamento standarda della procedura di classe Eh
	CHAR NewClass[80];

	sprintf(NewClass,"%sT",sys.WinClassBase);
	wc.cbSize=sizeof(wc);
	if (!GetClassInfoEx(sys.EhWinInstance,sys.WinClassBase,&wc)) 
		PRG_end("Errore 1");
	if (wc.lpfnWndProc!=LIntercept)
	{
		EhOriginalProc=wc.lpfnWndProc;
		wc.lpfnWndProc=LIntercept;
		wc.lpszClassName=NewClass;
		if (!RegisterClassEx(&wc)) win_infoarg("error");
		strcpy(sys.WinClassBase,NewClass);
	}
}

void * EhTreeView(SINT cmd,LONG info,void *ptr)
{
 struct WS_DISPEXT *DExt=ptr;
 static SINT HdbMovi=-1;
 static INT16 iSend;
// DWORD dwExStyle;
 SINT iLVIndex;
// DWORD dwExStyle;
 
 if (fReset)
 { 
	if (cmd!=WS_START) win_infoarg("Inizializzare EhTreeView()");
	memset(&TVList,0,sizeof(EH_TVLIST)*TVMAX);
	EhTreeInizialize();
	fReset=FALSE;
	return 0;
 }

 if (OBJ_CallSub==NULL) PRG_end("ETV: NULL %d",cmd);
 // Cerco una cella in LVLIST
 iLVIndex=TVFind(TV_FINDOBJ,OBJ_CallSub);
 if (iLVIndex<0) iLVIndex=LVAlloc(OBJ_CallSub);
 
 switch(cmd)
	{
		case WS_INF: return &TVList[iLVIndex];
 
		case WS_OPEN: // Creazione
			
			OBJ_CallSub->hWnd=CreateTreeView(sys.EhWinInstance,WindowNow());
			TVList[iLVIndex].hWndList=OBJ_CallSub->hWnd;
			break;

		case WS_EXTFUNZ:
			TVList[iLVIndex].subPostNotify=ptr;
			break;

		case WS_SETFLAG:
			
			if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(OBJ_CallSub->hWnd, GWL_STYLE);
				dwStyle|=info;
				SetWindowLong(OBJ_CallSub->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}
/*
			if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=TreeView_GetExtendedListViewStyle(OBJ_CallSub->hWnd);
				dwExStyle|=info;
				TreeView_SetExtendedListViewStyle(OBJ_CallSub->hWnd,dwExStyle);
			}
			*/
			break;

		case WS_CLOSE: // Distruzione
			DestroyWindow(OBJ_CallSub->hWnd);
			break;

		case WS_ADD: // Aggiungi una colonna
			//TreeView_InsertItem(OBJ_CallSub->hWnd,(LPTVINSERTSTRUCT) ptr);
			break;

		case WS_REALSET: // Aggiunge un Item
			//TreeView_SetItem(OBJ_CallSub->hWnd,(LPTVITEM) ptr));
			break;

		case WS_DO: // Spostamento / Ridimensionamento

			MoveWindow(OBJ_CallSub->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
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
    CHAR		*lpClass="EhSysTreeView32";
	WNDCLASS	wndClass;
	dwStyle =   WS_TABSTOP | 
				WS_CHILD | 
				WS_BORDER |
				WS_CLIPSIBLINGS |
				WS_VISIBLE|
				TVS_HASBUTTONS|
				TVS_SHOWSELALWAYS|
				TVS_HASLINES;

	GetClassInfo(NULL,"SysTreeView32",&wndClass);
	wndClass.hInstance=sys.EhWinInstance;
	wndClass.lpszClassName=lpClass;
	OldTVProc=wndClass.lpfnWndProc;
	wndClass.lpfnWndProc=EhTVProc;
	RegisterClass(&wndClass);

	hwndTreeView = CreateWindowEx(WS_EX_CONTROLPARENT|
								  WS_EX_CLIENTEDGE,          // ex style
                                  TEXT("SysTreeView32"),
								  "EhSysTreeView32",
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
static DrawTheImage(HIMAGELIST himl)
{
	HDC hDC;
	hDC=GetDC(NULL);
	ImageList_Draw(himl,0,hDC,0,0,ILD_NORMAL);
	ImageList_Draw(himl,1,hDC,20,0,ILD_NORMAL);
	ReleaseDC(NULL,hDC);
}

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
	SINT iIndex;
	CHAR Serv[80];
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
	sprintf(Serv,"%sDD",TVList[iIndex].lpObj->nome);
	obj_VStack(Serv);
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
	SINT iIndex;
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

static void CursorAutoSelect(SINT xCur,SINT yCur,SINT iTVIndex)//HWND hwndTV,struct OBJ *poj)
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
	SINT iTVIndex;

	switch (message)
	{
		case WM_RBUTTONDOWN:
			break;

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
		if (fDragging)
		{
			Main_EndDrag();
		}
		break;

	}
	return CallWindowProc(EhOriginalProc,hWnd,message,wParam,lParam);
}
 
// -----------------------------------------------------------------------------------------
// Gestione delle notifiche alla windows parent                                                        
// -----------------------------------------------------------------------------------------

static LRESULT TreeViewNotify(HWND hWnd, LPARAM lParam,SINT iLVIndex,LPNMHDR pnmh)
{
 LPNMLVKEYDOWN pnkd;
 
 struct OBJ *poj;
 CHAR Serv[20];
 LPNMHDR lpnmh;
 HTREEITEM hItem;
 LPNMTREEVIEW lpnmtv;
 TVITEM item;
 BOOL fReturn;
 LRESULT lRes;
 poj=TVList[iLVIndex].lpObj;
 if (pnmh->code==0) efx2();
 lpnmh = (LPNMHDR) lParam; 
 
 // Lancio se connessa la procedura di controllo esterna
 if (TVList[iLVIndex].subPostNotify)
 {
    fReturn=FALSE;
	lRes=(*TVList[iLVIndex].subPostNotify)(TVSUB_PRE,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

 switch(pnmh->code)
   {
	case TVN_SELCHANGING:
		 lpnmtv = (LPNMTREEVIEW) lParam;
		 TreeView_SelectDropTarget(lpnmh->hwndFrom,lpnmtv->itemNew.hItem); 
		 break;
	
	case NM_DBLCLK :
		 ZeroFill(item);
		 hItem=TreeView_GetSelection(lpnmh->hwndFrom);
		 item.mask=TVIF_PARAM; 
		 item.hItem=hItem;
		 item.pszText=Serv;
		 item.cchTextMax=sizeof(Serv);
		 TreeView_GetItem(lpnmh->hwndFrom,&item);

		 OBJ_key=item.lParam;
		 sprintf(Serv,"%sDCLK",poj->nome);
		 obj_VStack(Serv);
		 break;


	case NM_RCLICK:
		 hItem=TreeView_GetSelection(lpnmh->hwndFrom);
		 if (hItem==NULL) break;
		 //memset(&item,0,sizeof(item));
		 ZeroFill(item);
		 item.mask=TVIF_TEXT|TVIF_PARAM; 
		 item.hItem=hItem;
		 item.pszText=Serv;
		 item.cchTextMax=sizeof(Serv);

		 TreeView_GetItem(lpnmh->hwndFrom,&item);
		 //dispx("[%s]      ",Serv);
		 
		 
		 OBJ_key=item.lParam;
		 sprintf(Serv,"%sRC",poj->nome);
		 obj_VStack(Serv);
		 break;

	 case TVN_BEGINDRAG:
	 //case TVN_BEGINRDRAG:
		 Main_OnBeginDrag(lpnmh->hwndFrom, (LPNMTREEVIEW) lParam); 
         break; 

		//efx1();
		//break;

	case TVN_KEYDOWN:
		pnkd = (LPNMLVKEYDOWN) lParam;   
		if (pnkd->wVKey==ESC) SetFocus(WindowNow());
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
	lRes=(*TVList[iLVIndex].subPostNotify)(TVSUB_POST,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

return 0;
}

// -------------------------------------------------------------
// Intercettazione della procedura di classe TreeView
// -------------------------------------------------------------

static LRESULT CALLBACK EhTVProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	SINT iTVIndex;
//	PAINTSTRUCT ps;
//	WINSCENA WScena;
//	HDC hDC;
	
	switch (message)
	{
		case WM_RBUTTONDOWN:

			//efx2();
			iTVIndex=TVFind(TV_FINDHWND,hWnd);
			//dispx("%d       ",iTVIndex);
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
				{GetWindowText(hWndEdit,lpEEBuf,iEEBufCount);}
			
			//dispx("[%d]  %d  ",(SINT) nVirtKey,(SINT) GetAsyncKeyState(VK_SHIFT));
			if (iHotKey) fBreak=TRUE;//DestroyWindow(hWndEdit);
			break;
			*/

	}
	
  return CallWindowProc(OldTVProc,hWnd,message,wParam,lParam);
}
