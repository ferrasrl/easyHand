//   ---------------------------------------------
//   | ZCMP_TabControl                        
//   | ZoneComponent TabControl
//   |                                              
//   | Gestisce una TabControl Windows                                    
//   | in una oggetto ZONAP                        
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ZCMP_TabControl.h"


static LRESULT TabControlNotify(HWND hWnd, LPARAM lParam,EH_TCLIST * psTc,LPNMHDR  pnmh);
//static BOOL InsertListViewItems(HWND hwndListView);
static HWND CreateTabControl(HINSTANCE hInstance, HWND hwndParent);
static void ResizeTabControl(HWND, struct WS_DISPEXT *DExt);
static BOOL InitTabControl(HWND);
static void SwitchView(HWND hwndListView, DWORD dwView);

#define TCMAX 16
static WNDPROC EhOriginalProc=NULL;
static LRESULT CALLBACK LIntercept(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
// static EH_TCLIST TCList[TCMAX];
static struct {
	BOOL bReady;
	EH_LST lstHwnd;

} _p={false};

// static BOOL fReset=TRUE;

/*
#define TC_FINDOBJ  0
#define TC_FINDHWND 1
static SINT TCFind(SINT iCosa,void *ptr)
{
	SINT a;
	for (a=0;a<TCMAX;a++)
	{
		switch (iCosa)
		{
			case TC_FINDOBJ:  if (TCList[a].lpObj==(struct OBJ *) ptr) return a;
							  break;
			case TC_FINDHWND: if (TCList[a].hWndList==(HWND) ptr) return a;
							  break;
		}
	}
	return -1;
}

static SINT LVAlloc(struct OBJ *obj)
{
	SINT a;
	for (a=0;a<TCMAX;a++)
	{
	  if (TCList[a].lpObj==NULL)
	  {
		TCList[a].lpObj=obj;
		return a;
	  }
	}
	ehExit("TC: overload");
	return 0;
}
*/
static void _tabControlInizialize(void)
{
	WNDCLASSEX wc;
	// Cambio il puntamento standarda della procedura di classe Eh
	CHAR NewClass[80];

	sprintf(NewClass,"%sT",sys.tzWinClassBase);
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


void * ehzTabControl(EH_OBJ * pojCalled,EN_MESSAGE cmd,LONG info,void *ptr)
{
	struct WS_DISPEXT *DExt=ptr;
	static SINT HdbMovi=-1;
	static INT16 iSend;
	EH_TCLIST * psTc=NULL;
 // SINT iTCIndex;
 
 if (!_p.bReady)
 { 
	if (cmd!=WS_START) win_infoarg("Inizializzare EhTabControl()");
	//memset(&TCList,0,sizeof(EH_TCLIST)*TCMAX);
	_tabControlInizialize();
	_p.bReady=true;
	return 0;
 }

// if (OBJ_CallSub==NULL) ehExit("ETV: NULL %d",cmd);
 // Cerco una cella in LVLIST
	psTc=pojCalled->pOther;	
// iTCIndex=TCFind(TC_FINDOBJ,pojCalled);
//	if (iTCIndex<0) iTCIndex=LVAlloc(pojCalled);
 
	switch(cmd)
	{
		case WS_INF: return psTc;// &TCList[iTCIndex];

		case WS_CREATE:
			psTc=ehAllocZero(sizeof(EH_TCLIST));
			psTc->lpObj=pojCalled;
			pojCalled->pOther=psTc;
			psTc->hWndList=pojCalled->hWnd=CreateTabControl(sys.EhWinInstance,WindowNow());
			SetWindowLong(pojCalled->hWnd,GWL_USERDATA,(LONG) psTc);

 
		//case WS_OPEN: // Creazione
			
//			pojCalled->hWnd=CreateTabControl(sys.EhWinInstance,WindowNow());
//			TCList[iTCIndex].hWndList=pojCalled->hWnd;
			break;

		case WS_DESTROY:
			DestroyWindow(psTc->hWndList);
			break;


		case WS_EXTNOTIFY:
			psTc->subPostNotify=ptr;
			break;

		case WS_SETFLAG:
			
			if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(pojCalled->hWnd, GWL_STYLE);
				dwStyle|=info;
				SetWindowLong(pojCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			break;

		case WS_CLOSE: // Distruzione
			DestroyWindow(pojCalled->hWnd);
			break;

		case WS_ADD: // Aggiungi una colonna
			//TreeView_InsertItem(OBJ_CallSub->hWnd,(LPTVINSERTSTRUCT) ptr);
			break;

		case WS_REALSET: // Aggiunge un Item
			//TreeView_SetItem(OBJ_CallSub->hWnd,(LPTVITEM) ptr));
			break;

		case WS_DO: // Spostamento / Ridimensionamento

			MoveWindow(pojCalled->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
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
static LRESULT CALLBACK EhTCProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static HWND CreateTabControl(HINSTANCE hInstance, HWND hwndParent)
{
	DWORD       dwStyle;
	HWND        hwndTab;
	BOOL        bSuccess = TRUE;
    CHAR		*lpClass="EhTabControl";
	WNDCLASS	wndClass;

	dwStyle =   WS_TABSTOP | 
				WS_CHILD | 
				//WS_BORDER |
				WS_CLIPSIBLINGS |
				WS_VISIBLE|
				TCS_TABS; 
				//TCS_FOCUSNEVER|
				//TVS_HASBUTTONS|
				//TVS_SHOWSELALWAYS|
				//TVS_EDITLABELS|
				//TVS_HASLINES;//|TVS_FULLROWSELECT;

	GetClassInfo(NULL,WC_TABCONTROL,&wndClass);
	wndClass.hInstance=sys.EhWinInstance;
	wndClass.lpszClassName=lpClass;
	OldTVProc=wndClass.lpfnWndProc;
	wndClass.lpfnWndProc=EhTCProc;
	RegisterClass(&wndClass);

	hwndTab		= CreateWindowEx(0,//WS_EX_CONTROLPARENT|
								  //WS_EX_CLIENTEDGE,          // ex style
                                  lpClass,
								  "",
								  dwStyle,
								  0,                         // x position
								  0,                         // y position
								  0,                         // width
								  0,                         // height
								  hwndParent,                // parent
								  (HMENU) ID_TABCONTROL,        // ID
								  sys.EhWinInstance,                   // instance
								  NULL);                     // no extra data

	SendMessage(hwndTab,WM_SETFONT,(WPARAM) GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(TRUE, 0));
	if (!hwndTab) return NULL;
	return hwndTab;
}

/******************************************************************************
   PositionHeader

   this needs to be called when the ListView is created, resized, the view is 
   changed or a WM_SYSPARAMETERCHANGE message is recieved
******************************************************************************/
/*
static void PositionHeader(HWND hwndListView)
{+
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
/*
static BOOL fDragging=FALSE;
static HTREEITEM hDragItem;
static HTREEITEM hDropItem;
static HWND hDragWnd=NULL;
*/
// -----------------------------------------------------------------------------------------
// Intercettazione della procedura di controllo della Parent del TreeView per le notifiche
// -----------------------------------------------------------------------------------------
static LRESULT CALLBACK LIntercept(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	LPNMHDR  pnmh = (LPNMHDR) lParam;
	//SINT iTCIndex;
	EH_TCLIST * psTc=NULL;
//	EH_TCLIST * psTc=(EH_TCLIST *) GetWindowLong(hWnd,GWL_USERDATA);
	switch (message)
	{
		case WM_RBUTTONDOWN:
			break;

		case WM_NOTIFY:
		
//			iTCIndex=TCFind(TC_FINDHWND,pnmh->hwndFrom);
			{
				LPNMHDR lpnmh=(LPNMHDR) lParam;
				psTc=(EH_TCLIST *) GetWindowLong(lpnmh->hwndFrom,GWL_USERDATA);
				if (psTc)
				{
				 return TabControlNotify(hWnd, lParam,psTc,pnmh );                                              
				}
			}
			break;


		case WM_MOUSEMOVE:
		//if (fDragging) Main_OnMouseMove(hWnd,hDragWnd,LOWORD(lParam),HIWORD(lParam));
		break;

		case WM_LBUTTONUP:
			/*
		if (fDragging)
		{
			//Main_EndDrag();
		}
		*/
		break;

	}
	return CallWindowProc(EhOriginalProc,hWnd,message,wParam,lParam);
}
 
// -----------------------------------------------------------------------------------------
// Gestione delle notifiche alla windows parent                                                        
// -----------------------------------------------------------------------------------------
static LRESULT TabControlNotify(HWND hWnd, LPARAM lParam,EH_TCLIST * psTc,LPNMHDR  pnmh)
{

// LPNMLVKEYDOWN pnkd;
 
	struct OBJ *poj;
	CHAR Serv[20];
	LPNMHDR lpnmh;
	TCITEM tcItem;
	BOOL fReturn;
	LRESULT lRes;
	//poj=TCList[iTCIndex].lpObj;
	if (pnmh->code==0) efx2();
	lpnmh = (LPNMHDR) lParam; 

	if (!psTc) return 0;
	poj=psTc->lpObj;
 //dispx("%u     ",NM_FIRST-pnmh->code);
 
 //return 0;

 // Lancio se connessa la procedura di controllo esterna
 if (psTc->subPostNotify)
 {
    fReturn=FALSE;
	lRes=psTc->subPostNotify(TCSUB_PRE,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

 switch(pnmh->code)
   {
	case TCN_SELCHANGE:
		 if (!lpnmh) 
			 ehError();
		 TabCtrl_GetItem(lpnmh->hwndFrom,TabCtrl_GetCurSel(lpnmh->hwndFrom),&tcItem);
		 //OBJ_key=tcItem.lParam;
		 OBJ_key=TabCtrl_GetCurSel(lpnmh->hwndFrom);
		 sprintf(Serv,"%sSEL",poj->nome);
		 //dispxEx(0,20,"[%s] %d/%d",Serv,TabCtrl_GetCurSel(lpnmh->hwndFrom),OBJ_key);
		 obj_addevent(Serv);
		 break;
		/*
	case TVN_SELCHANGING:
		 lpnmtv = (LPNMTREEVIEW) lParam;
		 TreeView_SelectDropTarget(lpnmh->hwndFrom, lpnmtv->itemNew.hItem); 
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
		 obj_addevent(Serv);
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
		 obj_addevent(Serv);
		 break;

	 case TVN_BEGINDRAG:
	 //case TVN_BEGINRDRAG:
		 //Main_OnBeginDrag(lpnmh->hwndFrom, (LPNMTREEVIEW) lParam); 
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
		*/
	 }

 // Lancio se connessa la procedura di controllo esterna
 if (psTc->subPostNotify)
 {
    fReturn=FALSE;
	lRes=psTc->subPostNotify(TCSUB_POST,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

return 0;
}

// -------------------------------------------------------------
// Intercettazione della procedura di classe TabControl
// -------------------------------------------------------------

static LRESULT CALLBACK EhTCProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
//	SINT iTCIndex;
	
	switch (message)
	{
//		case WM_RBUTTONDOWN:

			//efx2();
			//iTVIndex=TVFind(TV_FINDHWND,hWnd);
			//dispx("%d       ",iTVIndex);
			//CursorAutoSelect(LOWORD(lParam),HIWORD(lParam),iTVIndex);
			break;
/*
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
