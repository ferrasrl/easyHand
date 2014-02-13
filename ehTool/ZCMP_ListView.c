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
//   |  case WM_NOTIFY: return _ListViewNotify(hWnd, lParam);                                              
//   |                                              
//   |                                              
//   |         by Ferrà Art & Technology 1993-2000
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ZCMP_ListView.h"

//#define ID_LISTVIEW 25000
static LRESULT _viewNotify(INT iLVIndex, HWND hWnd, UINT message, WPARAM  wParam,LPARAM lParam);
static LRESULT _headNotify(INT iLVIndex, HWND hWnd,UINT message,WPARAM  wParam,LPARAM lParam);

static BOOL InsertListViewItems(HWND hwndListView);
static HWND CreateListView(CHAR *lpName,HINSTANCE hInstance,HWND hwndParent);
static HWND CreateListViewSort(CHAR *lpName,HINSTANCE hInstance,HWND hwndParent);
static void ResizeListView(HWND, EH_DISPEXT *psExt);
static void SwitchView(HWND hwndListView, DWORD dwView);

#define LVMAX 16
static EH_LVLIST _arsLv[LVMAX];
static BOOL fReset=TRUE;
#define LV_FINDOBJ  0
#define LV_FINDHWND 1
#define LV_FINDHEAD 2

void lvItemGetPosition(SINT *lpiItem,SINT *lpSubItem) {
	*lpiItem=(OBJ_key/10000)-1;
	*lpSubItem=(OBJ_key%10000)-1;
}

static SINT LVFind(SINT iCosa,void *ptr)
{
	SINT a;
	for (a=0;a<LVMAX;a++)
	{
		switch (iCosa)
		{
			case LV_FINDOBJ:  if (_arsLv[a].lpObj==(struct OBJ *) ptr) return a;
							  break;
			case LV_FINDHWND: if (_arsLv[a].hWndList==(HWND) ptr) return a;
							  break;
			case LV_FINDHEAD: if (_arsLv[a].hWndHeader==(HWND) ptr) return a;
							  break;
		}
	}
	return -1;
}

static SINT LVAlloc(struct OBJ *obj)
{
	SINT a;
	for (a=0;a<LVMAX;a++)
	{
	  if (_arsLv[a].lpObj==NULL)
	  {
		_arsLv[a].lpObj=obj;
		return a;
	  }
	}
	ehExit("LV: overload");
	return 0;
}

static WNDPROC EhOriginalProc=NULL;
static LRESULT CALLBACK _Intercept(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	LPNMHDR  pnmh = (LPNMHDR) lParam;
	SINT iLVIndex;
	static BOOL fReturn;
	LRESULT lRes;

	switch (message)
	{
		case WM_NOTIFY:

			iLVIndex=LVFind(LV_FINDHWND,pnmh->hwndFrom);
			if (iLVIndex>=0)
			{
				return _viewNotify(iLVIndex,hWnd,message,wParam,lParam);
					//_ListViewNotify(hWnd,lParam,iLVIndex,pnmh);                                              
			}

			iLVIndex=LVFind(LV_FINDHEAD,pnmh->hwndFrom);
			if (iLVIndex>=0) {
				return _headNotify(iLVIndex,hWnd,message,wParam,lParam);                                              
			}
			break;

		// Notifica di tutti i messaggi (fine 2002)
		default:

			iLVIndex=LVFind(LV_FINDHWND,hWnd);
			if (iLVIndex>=0)
			{
				if (_arsLv[iLVIndex].subMessage)
				{
					fReturn=FALSE;
					lRes=(*_arsLv[iLVIndex].subMessage)(NULL,&fReturn,EXT_PREV,hWnd,message,wParam,lParam);
					if (fReturn) return lRes;
				}
			}
			break;
	}

	return CallWindowProc(EhOriginalProc,hWnd,message,wParam,lParam);
} 

static void EhListInizialize(void)
{
	WNDCLASSEX wc;

	// Cambio il puntamento standarda della procedura di classe Eh
	CHAR NewClass[80];

	sprintf(NewClass,"%sL",sys.tzWinClassBase);
	wc.cbSize=sizeof(wc);
	if (!GetClassInfoEx(sys.EhWinInstance,
						sys.tzWinClassBase,
						&wc)) ehExit("Errore 1");

	if (wc.lpfnWndProc!=_Intercept)	{
		EhOriginalProc=wc.lpfnWndProc;
		wc.lpfnWndProc=_Intercept;
		wc.lpszClassName=NewClass;
		if (!RegisterClassEx(&wc)) win_infoarg("Errorx|");
		strcpy(sys.tzWinClassBase,NewClass);
	}
}

// -----------------------------------------------
// List view di lettura (senza sort header)
// -----------------------------------------------
void * ehzListView(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr)
{
	EH_DISPEXT *psExt=ptr;
	static SINT HdbMovi=-1;
	static INT16 iSend;
	DWORD dwExStyle;
	SINT iLVIndex;

	if (fReset)
	{
		if (cmd!=WS_START) win_infoarg("Inizializzare ehzListView()");
		memset(&_arsLv,0,sizeof(EH_LVLIST)*LVMAX);
		EhListInizialize();
		fReset=FALSE;
		return 0;
	}

	iLVIndex=LVFind(LV_FINDOBJ,objCalled);
	if (iLVIndex<0) iLVIndex=LVAlloc(objCalled);

 switch(cmd)
	{
		case WS_INF:
			return &_arsLv[iLVIndex];
 
		case WS_OPEN: // Creazione
			
			objCalled->hWnd=CreateListView(objCalled->nome,sys.EhWinInstance,WindowNow());
			_arsLv[iLVIndex].hWndList=objCalled->hWnd;
			_arsLv[iLVIndex].hWndHeader=ListView_GetHeader(objCalled->hWnd);//GetWindow(objCalled->hWnd, GW_CHILD);
			_arsLv[iLVIndex].idHeader=GetDlgCtrlID(_arsLv[iLVIndex].hWndHeader);
			_arsLv[iLVIndex].fLeftClick=TRUE;
			_arsLv[iLVIndex].fRightClick=TRUE;
			_arsLv[iLVIndex].fDoubleClick=FALSE;
			break;

		case WS_EXTNOTIFY:
			switch (info)
			{
				case 0: _arsLv[iLVIndex].subListNotify=ptr; break;
				case 1: _arsLv[iLVIndex].subMessage=ptr; break;
				case 2: _arsLv[iLVIndex].subHeaderNotify=ptr; break;
			}				
			break;

		case WS_SETFLAG:

			if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				dwStyle|=info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"!STYLE")) // Setta lo stile della finestra
			{
				LONG lStyle;
				lStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				lStyle&=~info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, lStyle);
			}

			if (!strcmp(ptr,"STYLEMASK")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				//win_infoarg("%08x",~LVS_TYPESTYLEMASK);
				dwStyle=dwStyle&~LVS_TYPESTYLEMASK|info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(objCalled->hWnd);
				dwExStyle|=info;
				ListView_SetExtendedListViewStyle(objCalled->hWnd,dwExStyle);
			}

			if (!strcmp(ptr,"!EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(objCalled->hWnd);
				dwExStyle&=~info;
				ListView_SetExtendedListViewStyle(objCalled->hWnd,dwExStyle);
			}

			if (!strcmp(ptr,"DCLK")) // Setta il Double Click
			{
				_arsLv[iLVIndex].fDoubleClick=info;
			}
			break;

		case WS_CLOSE: // Distruzione
			DestroyWindow(objCalled->hWnd);
			break;

		case WS_DEL: // Cancella le colonne
			while (TRUE)
			{
				if (!ListView_DeleteColumn(objCalled->hWnd,0)) break;
			}
    		break;

		case WS_ADD: // Aggiungi una colonna
			ListView_InsertColumn(objCalled->hWnd, info, (LV_COLUMN *) ptr);
			break;

		case WS_LINK: // Aggiunge un Item
			//
			if (ListView_SetItem(objCalled->hWnd,(LPLVITEM) ptr)==FALSE) 
				{ListView_InsertItem(objCalled->hWnd,(LPLVITEM) ptr);}
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,psExt->px,psExt->py,psExt->lx,psExt->ly,TRUE);
			break;

		case WS_DISPLAY: 
			InvalidateRect(objCalled->hWnd,NULL,TRUE);
			break;
	}
	return NULL;
}

// -----------------------------------------------
// List view di lettura (+ sort header)
// -----------------------------------------------

void * ehzListViewSort(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr)
{
	EH_DISPEXT *psExt=ptr;
	static SINT HdbMovi=-1;
	static INT16 iSend;
	DWORD dwExStyle;
	SINT iLVIndex;

	if (fReset)
	{
		if (cmd!=WS_START) win_infoarg("Inizializzare EhListView()");
		memset(&_arsLv,0,sizeof(EH_LVLIST)*LVMAX);
		EhListInizialize();
		fReset=FALSE;
		return 0;
	}

	if (objCalled==NULL) ehExit("ELV: NULL %d",cmd);

	// Cerco una cella in LVLIST
	iLVIndex=LVFind(LV_FINDOBJ,objCalled);
	if (iLVIndex<0) iLVIndex=LVAlloc(objCalled);

	switch(cmd)
	{
	case WS_INF:
		return &_arsLv[iLVIndex];

	case WS_OPEN: // Creazione

		objCalled->hWnd=CreateListViewSort(objCalled->nome,sys.EhWinInstance,WindowNow());
		//_arsLv[iLVIndex].hWndList=objCalled->hWnd;
		_arsLv[iLVIndex].hWndList=objCalled->hWnd;
		_arsLv[iLVIndex].hWndHeader=GetWindow(objCalled->hWnd, GW_CHILD);
		_arsLv[iLVIndex].fLeftClick=TRUE;
		_arsLv[iLVIndex].fRightClick=TRUE;
		_arsLv[iLVIndex].fDoubleClick=FALSE;
		break;

	case WS_EXTNOTIFY:
		switch (info)
		{
		case 0: _arsLv[iLVIndex].subListNotify=ptr; break;
		case 1: _arsLv[iLVIndex].subMessage=ptr; break;
		case 2: _arsLv[iLVIndex].subHeaderNotify=ptr; break;
		}				
		break;

	case WS_SETFLAG:

		if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
		{
			DWORD dwStyle;
			dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
			dwStyle|=info;
			SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
		}

		if (!strcmp(ptr,"!STYLE")) // Setta lo stile della finestra
		{
			LONG lStyle;
			lStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
			lStyle&=~info;
			SetWindowLong(objCalled->hWnd, GWL_STYLE, lStyle);
		}

		if (!strcmp(ptr,"STYLEMASK")) // Setta lo stile della finestra
		{
			DWORD dwStyle;
			dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
			//win_infoarg("%08x",~LVS_TYPESTYLEMASK);
			dwStyle=dwStyle&~LVS_TYPESTYLEMASK|info;
			SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
		}

		if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
		{
			dwExStyle=ListView_GetExtendedListViewStyle(objCalled->hWnd);
			dwExStyle|=info;
			ListView_SetExtendedListViewStyle(objCalled->hWnd,dwExStyle);
		}

		if (!strcmp(ptr,"!EXSTYLE")) // Setta lo stile della finestra
		{
			dwExStyle=ListView_GetExtendedListViewStyle(objCalled->hWnd);
			dwExStyle&=~info;
			ListView_SetExtendedListViewStyle(objCalled->hWnd,dwExStyle);
		}

		if (!strcmp(ptr,"DCLK")) // Setta il Double Click
		{
			_arsLv[iLVIndex].fDoubleClick=info;
		}
		break;

	case WS_CLOSE: // Distruzione
		DestroyWindow(objCalled->hWnd);
		//			_arsLv[iLVIndex].lpObj=NULL;
		break;

	case WS_ADD: // Aggiungi una colonna
		ListView_InsertColumn(objCalled->hWnd, info, (LV_COLUMN *) ptr);
		break;

	case WS_LINK: // Aggiunge un Item
		//
		if (ListView_SetItem(objCalled->hWnd,(LPLVITEM) ptr)==FALSE) 
		{ListView_InsertItem(objCalled->hWnd,(LPLVITEM) ptr);}
		break;

	case WS_DO: // Spostamento / Ridimensionamento
		MoveWindow(objCalled->hWnd,psExt->px,psExt->py,psExt->lx,psExt->ly,TRUE);
		break;

	case WS_DISPLAY: 
		InvalidateRect(objCalled->hWnd,NULL,FALSE);
		break;
	}
	return NULL;
}


/******************************************************************************

   CreateListView

******************************************************************************/

static WNDPROC OldListProc=NULL;

static HWND CreateListView(CHAR *lpName,HINSTANCE hInstance, HWND hwndParent)
{
	DWORD       dwStyle;
	HWND        hwndListView;
	BOOL        bSuccess = TRUE;

	dwStyle =   WS_TABSTOP | 
				WS_CHILD | 
				//WS_BORDER | 
				WS_VISIBLE |
				LVS_NOSORTHEADER |
				LVS_REPORT;
				//LVS_OWNERDRAWFIXED;

				//LVS_OWNERDATA;
	if (strstr(lpName,"_O")) dwStyle|=LVS_OWNERDATA;
	hwndListView = CreateWindowEx(WS_EX_CONTROLPARENT,//|
								 // WS_EX_CLIENTEDGE,          // ex style
                                  WC_LISTVIEW,               // class name - defined in commctrl.h
                                  "",                        // dummy text
                                  dwStyle,                   // style
                                  0,                         // x position
                                  0,                         // y position
                                  0,                         // width
                                  0,                         // height
                                  hwndParent,                // parent
                                  (HMENU) ID_LISTVIEW,        // ID
                                  sys.EhWinInstance,                   // instance
                                  NULL);                     // no extra data

	if(!hwndListView) return NULL;

	return hwndListView;
}

static HWND CreateListViewSort(CHAR *lpName,HINSTANCE hInstance, HWND hwndParent)
{
	DWORD       dwStyle;
	HWND        hwndListView;
	BOOL        bSuccess = TRUE;

	dwStyle =   WS_TABSTOP | 
				WS_CHILD | 
				//WS_BORDER | 
				WS_VISIBLE |
				LVS_REPORT; //| LVS_SORTASCENDING;
				//LVS_OWNERDRAWFIXED;				
				//LVS_OWNERDATA; // New
	if (strstr(lpName,"_O")) dwStyle|=LVS_OWNERDATA;
            
	hwndListView = CreateWindowEx(WS_EX_CONTROLPARENT|
								  WS_EX_CLIENTEDGE,          // ex style
                                  WC_LISTVIEW,               // class name - defined in commctrl.h
                                  "",                        // dummy text
                                  dwStyle,                   // style
                                  0,                         // x position
                                  0,                         // y position
                                  0,                         // width
                                  0,                         // height
                                  hwndParent,                // parent
                                  (HMENU) ID_LISTVIEW,        // ID
                                  sys.EhWinInstance,                   // instance
                                  NULL);                     // no extra data

	if(!hwndListView) return NULL;

	return hwndListView;
}

/******************************************************************************
   PositionHeader

   this needs to be called when the ListView is created, resized, the view is 
   changed or a WM_SYSPARAMETERCHANGE message is recieved
******************************************************************************/

static void PositionHeader(HWND hwndListView)
{
HWND  hwndHeader = GetWindow(hwndListView, GW_CHILD);
DWORD dwStyle = GetWindowLong(hwndListView, GWL_STYLE);

	/*To ensure that the first item will be visible, create the control without 
	the LVS_NOSCROLL style and then add it here*/
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

	SetWindowPos( hwndHeader, 
                  wpos.hwndInsertAfter, 
                  wpos.x, 
                  wpos.y,
                  wpos.cx, 
                  wpos.cy, 
                  wpos.flags | SWP_SHOWWINDOW);

	ListView_EnsureVisible(hwndListView, 0, FALSE);
   }
}


/**************************************************************************
   _ListViewNotify()
   Interpreta le notifiche della listview
**************************************************************************/

static LRESULT _viewNotify(INT iLVIndex,HWND hWnd, UINT message,WPARAM wParam,LPARAM lParam)// LPNMHDR  pnmh)
{
	LPNMLVKEYDOWN pnkd;
	LPNMLISTVIEW lpnmlv;
	struct OBJ *poj;
	CHAR Serv[20];
	BOOL bReturn;
	LRESULT lRes;
	LPNMHDR  pnmh=(LPNMHDR ) lParam;

	poj=_arsLv[iLVIndex].lpObj;
	if (_arsLv[iLVIndex].subListNotify)
	{
		bReturn=FALSE;
		lRes=(*_arsLv[iLVIndex].subListNotify)(poj,&bReturn, EXT_PREV, hWnd,0,wParam,lParam);
		if (bReturn) return lRes;
	}

	if (pnmh->code==0) efx2();
	if (pnmh->code==-121) return 0;
#ifdef _DEBUG
	{
		CHAR szServ[20],*psz=szServ;
		sprintf(psz,"%d",pnmh->code);
		switch (pnmh->code) {

			
			case NM_CLICK: psz="NM_CLICK"; break;
			case NM_SETFOCUS: psz="NM_SETFOCUS"; break;
			case NM_KILLFOCUS: psz="NM_KILLFOCUS"; break;
			case NM_CUSTOMDRAW: psz="NM_CUSTOMDRAW"; break;
			case NM_RELEASEDCAPTURE: psz="NM_RELEASEDCAPTURE"; break;

		}
		// printf("- %s" CRLF,psz);

	}
#endif
	// printf("[%d]" CRLF,pnmh->code);
	
	switch(pnmh->code)
	{

		case NM_SETFOCUS:

//			lpnmlv = (LPNMLISTVIEW) lParam;
			if (poj!=NULL) {
				sprintf(Serv,"%sFS",poj->nome);
				obj_addevent(Serv);
			}
			break;

		case NM_RELEASEDCAPTURE:
			lpnmlv = (LPNMLISTVIEW) lParam;

			break;

		case NM_DBLCLK:

			if (!_arsLv[iLVIndex].fDoubleClick) break;
			lpnmlv = (LPNMLISTVIEW) lParam;
			if (poj!=NULL)
			{
				OBJ_key=(LONG) (lpnmlv->iItem+1)*10000+(lpnmlv->iSubItem+1);
				sprintf(Serv,"%sDCLK",poj->nome);
				obj_addevent(Serv);
			}
			break;

		case NM_RCLICK:

			lpnmlv = (LPNMLISTVIEW) lParam;
			if (poj!=NULL)
			{
				OBJ_key=(LONG) (lpnmlv->iItem+1)*10000+(lpnmlv->iSubItem+1);
				sprintf(Serv,"%sRC",poj->nome);
				obj_addevent(Serv);
			}
			break;

		case NM_CLICK:

			lpnmlv = (LPNMLISTVIEW) lParam;
			if (_arsLv[iLVIndex].lpClickMask&&(lpnmlv->iSubItem>-1))
			{
				if (_arsLv[iLVIndex].lpClickMask[lpnmlv->iSubItem]==' ') {break;}
			}
			
			if (poj!=NULL)
			{
				OBJ_key=(LONG) (lpnmlv->iItem+1)*10000+(lpnmlv->iSubItem+1);
				//winSetFocus(WindowNow());
				sprintf(Serv,"%sLW",poj->nome);
				obj_addevent(Serv);
			}
			break;

		case LVN_KEYDOWN:
			pnkd = (LPNMLVKEYDOWN) lParam;   
			if (pnkd->wVKey==ESC) winSetFocus(WindowNow());
			if ((pnkd->wVKey>=' ')&&(pnkd->wVKey<='<')) 
			{
				//efx1();		
			}
			break;

		

		case LVN_GETDISPINFO:
		  {
			LV_DISPINFO *lpdi = (LV_DISPINFO *)lParam;

			// Risalire dall'HWND al OBJCall sub collegato
		/*
			objCalled=LGetOBJ(hWnd);
			EhListView(WS_REALGET,0,(CHAR *) lpdi);

			switch (lpdi->item.iSubItem) // Colonna
			{
			
			
			
			
			}
		*/	
			
			/*
			TCHAR szString[MAX_PATH];
		  if(lpdi->item.iSubItem)
			 {
			 if(lpdi->item.mask & LVIF_TEXT)
				{
				wsprintf(szString, "Item %d - Column %d", lpdi->item.iItem + 1, lpdi->item.iSubItem);
				lstrcpy(lpdi->item.pszText, szString);
				}
			 }
		  else
			 {
			 if(lpdi->item.mask & LVIF_TEXT)
				{
				wsprintf(szString, "Item %d", lpdi->item.iItem + 1);
				lstrcpy(lpdi->item.pszText, szString);
				}

			 if(lpdi->item.mask & LVIF_IMAGE)
				{
				lpdi->item.iImage = 0;
				}
			 }
			 */
		  }
		  //return 0;
		  break;
		/*
		case LVN_ODCACHEHINT:
		  {
		  LPNMLVCACHEHINT   lpCacheHint = (LPNMLVCACHEHINT)lParam;
		  This sample doesn't use this notification, but this is sent when the 
		  ListView is about to ask for a range of items. On this notification, 
		  you should load the specified items into your local cache. It is still 
		  possible to get an LVN_GETDISPINFO for an item that has not been cached, 
		  therefore, your application must take into account the chance of this 
		  occurring.
		  }
		  return 0;
		*/
		case LVN_ODFINDITEM:
		  {
		  LPNMLVFINDITEM lpFindItem = (LPNMLVFINDITEM)lParam;
		  /*
		  This sample doesn't use this notification, but this is sent when the 
		  ListView needs a particular item. Return -1 if the item is not found.
		  */
		  }
		  return 0;
	}

	// Lancio se connessa la procedura di controllo esterna
	if (_arsLv[iLVIndex].subListNotify)
	{
		bReturn=FALSE;
		lRes=(*_arsLv[iLVIndex].subListNotify)(poj,&bReturn, EXT_AFTER,hWnd,0,wParam,lParam);
		if (bReturn) return lRes;
	}

	//return 0;
	return CallWindowProc(EhOriginalProc,hWnd,message,wParam,lParam);
}

static LRESULT _headNotify(INT iIndex,HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam) 
{
	struct OBJ *poj;
	BOOL bReturn=false;
	LRESULT lRes;

	poj=_arsLv[iIndex].lpObj;
//	printf("[%d] - %d" CRLF,wParam,pnmh);//,HDN_ITEMSTATEICONCLICK);
	
	if (_arsLv[iIndex].subHeaderNotify)
	{
		//LVN_BEGINDRAG
		bReturn=false;
 		lRes=_arsLv[iIndex].subHeaderNotify(poj,&bReturn, EXT_PREV, hWnd,iMessage,wParam,lParam);
		if (bReturn) return lRes;
		
	}
/*
	if (_arsLv[iLVIndex].subHeaderNotify)
	{
		fReturn=FALSE;
		lRes=(*_arsLv[iLVIndex].subHeaderNotify)(LVSUB_POST,poj,hWnd,lParam,pnmh,&fReturn);
		if (fReturn) return lRes;
	}
*/
	return CallWindowProc(EhOriginalProc,hWnd,iMessage,wParam,lParam);
}

void LVItemSet(HWND hwndList,SINT iItem,SINT iSubItem,CHAR *lpText)
{
	LVITEM lvItem;
	lvItem.mask=LVIF_TEXT;
    lvItem.pszText=lpText;
	lvItem.iItem=iItem;
	lvItem.iSubItem=iSubItem;
	if (ListView_SetItem(hwndList,&lvItem)==FALSE) {ListView_InsertItem(hwndList,&lvItem);}
}

void LVItemSetParam(HWND hwndList,SINT iItem,SINT iSubItem,CHAR *lpText,LONG lParam)
{
	LVITEM lvItem;
	lvItem.mask=LVIF_TEXT|LVIF_PARAM;
    lvItem.pszText=lpText;
	lvItem.iItem=iItem;
	lvItem.iSubItem=iSubItem;
	lvItem.lParam=lParam;
	if (ListView_SetItem(hwndList,&lvItem)==FALSE) {ListView_InsertItem(hwndList,&lvItem);}
}

void LVItemGet(HWND hWnd,SINT iItem,SINT iSubItem,CHAR *lpBuffer,SINT iBufferSize)
{
	 LVITEM lvItem;
	 *lpBuffer=0;
	 lvItem.mask=LVIF_TEXT;
	 lvItem.pszText=lpBuffer;
	 lvItem.iItem=iItem;
	 lvItem.iSubItem=iSubItem;
	 lvItem.cchTextMax=iBufferSize;
	 ListView_GetItem(hWnd,&lvItem); 
}

void LVItemSetCheckAll(HWND hWnd,BOOL bFlag) {

	INT iItem,iMax;
	iMax=ListView_GetItemCount(hWnd); if (!iMax) return;
	for (iItem=0;iItem<iMax;iItem++) 
	{
		BOOL bNow=ListView_GetCheckState(hWnd,iItem);
		if (!bFlag&&bNow) ListView_SetCheckState(hWnd,iItem,false);
		if (bFlag&&!bNow) ListView_SetCheckState(hWnd,iItem,true);
	}

}

//
// lvAddHeaderCheckbox()
//
#if _WIN32_WINNT >= 0x0600
void lvAddHeaderCheckbox(HWND lvTarget) {

	HWND header;
	DWORD dwHeaderStyle;
	INT iIndex;

	iIndex=LVFind(LV_FINDHWND,lvTarget); if (iIndex<0) ehError();
	header = ListView_GetHeader(lvTarget);
	dwHeaderStyle = GetWindowLong(header, GWL_STYLE);
	dwHeaderStyle |= HDS_CHECKBOXES;
	SetWindowLong(header, GWL_STYLE, dwHeaderStyle);
	{
		// Now, we can update the format for the first header item,
		// which corresponds to the first column
		HDITEM hdi = { 0 };
		hdi.mask = HDI_FORMAT;
		Header_GetItem(header, 0, &hdi);
		hdi.fmt |= HDF_CHECKBOX | HDF_FIXEDWIDTH;
		Header_SetItem(header, 0, &hdi);	
	
	}
	//_arsLv[iIndex].hWndHeader=ListView_GetHeader(lvTarget);//GetWindow(objCalled->hWnd, GW_CHILD);
	//_arsLv[iIndex].idHeader=GetDlgCtrlID(_arsLv[iIndex].hWndHeader);
}
#endif