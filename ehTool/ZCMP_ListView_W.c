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

#include "\ehtool\include\ehsw_i.h"
#include "\ehtool\look98.h"
#include "\ehtool\dmiutil.h"
#include "\ehtool\fbfile.h"
#include "\ehtool\ZCMP_ListView.h"

static LRESULT ListViewNotify(HWND hWnd, LPARAM lParam,SINT iLVIndex,LPNMHDR pnmh);
static LRESULT ListHeaderNotify(HWND hWnd, LPARAM lParam,SINT iLVIndex,LPNMHDR pnmhr);

static BOOL InsertListViewItems(HWND hwndListView);
static HWND CreateListView(CHAR *lpName,HINSTANCE hInstance,HWND hwndParent);
static HWND CreateListViewSort(CHAR *lpName,HINSTANCE hInstance,HWND hwndParent);
static void ResizeListView(HWND, struct WS_DISPEXT *DExt);
static void SwitchView(HWND hwndListView, DWORD dwView);

#define LVMAX 16
static EH_LVLIST LVList[LVMAX];
static BOOL fReset=TRUE;
#define LV_FINDOBJ  0
#define LV_FINDHWND 1
#define LV_FINDHEAD 2

/*
static DWORD dwStyleCreate=0;

void LVSetStyleCreate(DWORD dwStyle)
{
	dwStyleCreate=dwStyle;
}
*/
void LVItemClick(SINT *lpiItem,SINT *lpSubItem)
{
//			SINT iItem,iSubItem;
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
			case LV_FINDOBJ:  if (LVList[a].lpObj==(struct OBJ *) ptr) return a;
							  break;
			case LV_FINDHWND: if (LVList[a].hWndList==(HWND) ptr) return a;
							  break;
			case LV_FINDHEAD: if (LVList[a].hWndHeader==(HWND) ptr) return a;
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
	  if (LVList[a].lpObj==NULL)
	  {
		LVList[a].lpObj=obj;
		return a;
	  }
	}
	PRG_end("LV: overload");
	return 0;
}

static WNDPROC EhOriginalProc=NULL;
static LRESULT CALLBACK LIntercept(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	/*
	if (message==WM_NOTIFY) // Controllo se è mio
	{
	 LPNMHDR  pnmh = (LPNMHDR) lParam;
	 SINT iLVIndex=LVFind(LV_FINDHWND,pnmh->hwndFrom);
	 if (iLVIndex>=0)
	 {
		return ListViewNotify(hWnd, lParam,iLVIndex,pnmh );                                              
	 }

	}
	*/
	LPNMHDR  pnmh = (LPNMHDR) lParam;
	SINT iLVIndex;
	static BOOL fReturn;
	LRESULT lRes;

	//dispx("%d  ",message);
	//GetCursor(

	switch (message)
	{
		case WM_NOTIFY:
			iLVIndex=LVFind(LV_FINDHWND,pnmh->hwndFrom);
			if (iLVIndex>=0)
			{
				return ListViewNotify(hWnd,lParam,iLVIndex,pnmh);                                              
			}
			
			iLVIndex=LVFind(LV_FINDHEAD,pnmh->hwndFrom);
			if (iLVIndex>=0)
			{
				return ListHeaderNotify(hWnd,lParam,iLVIndex,pnmh);                                              
			}
			break;


		// Notifica di tutti i messaggi (fine 2002)
		
		default:

			iLVIndex=LVFind(LV_FINDHWND,hWnd);
			if (iLVIndex>=0)
			{
				if (LVList[iLVIndex].subMessage)
				{
					fReturn=FALSE;
					lRes=(*LVList[iLVIndex].subMessage)(hWnd,message,wParam,lParam,&fReturn);
					if (fReturn) return lRes;
				}
			}
			break;
	}

	return CallWindowProc(EhOriginalProc,hWnd,message,wParam,lParam);
} 

/*
static void EhListInizialize(void)
{
	WNDCLASSEXW wc;
	// Cambio il puntamento standarda della procedura di classe Eh
	CHAR NewClass[80];
	WCHAR *lpNewClass;
	WCHAR *lpWinClassBase;

	sprintf(NewClass,"%sL",sys.WinClassBase);
	lpNewClass=StringAllocToUnicode(NewClass);
	lpWinClassBase=StringAllocToUnicode(sys.WinClassBase);
	wc.cbSize=sizeof(wc);
	if (!GetClassInfoExW(sys.EhWinInstance,
						 lpWinClassBase,
						 &wc)) PRG_end("Errore 1");

	if (wc.lpfnWndProc!=LIntercept)
	{
		EhOriginalProc=wc.lpfnWndProc;
		wc.lpfnWndProc=LIntercept;
		wc.lpszClassName=lpNewClass;
		if (!RegisterClassExW(&wc)) win_infoarg("error");
		strcpy(sys.WinClassBase,NewClass);
	}

	EhFree(lpWinClassBase);
	EhFree(lpNewClass);
}
*/
static void EhListInizialize(void)
{
	WNDCLASSEX wc;

	// Cambio il puntamento standarda della procedura di classe Eh
	CHAR NewClass[80];

	sprintf(NewClass,"%sL",sys.WinClassBase);
	wc.cbSize=sizeof(wc);
	if (!GetClassInfoEx(sys.EhWinInstance,
						sys.WinClassBase,
						&wc)) PRG_end("Errore 1");

	if (wc.lpfnWndProc!=LIntercept)
	{
		EhOriginalProc=wc.lpfnWndProc;
		wc.lpfnWndProc=LIntercept;
		wc.lpszClassName=NewClass;
		if (!RegisterClassEx(&wc)) win_infoarg("error");
		strcpy(sys.WinClassBase,NewClass);
	}

}


// -----------------------------------------------
// List view di lettura (senza sort header)
// -----------------------------------------------
void * EhListView(SINT cmd,LONG info,void *ptr)
{
 struct WS_DISPEXT *DExt=ptr;
 static SINT HdbMovi=-1;
 static INT16 iSend;
 DWORD dwExStyle;
 SINT iLVIndex;
 
 if (fReset)
 {
	 if (cmd!=WS_START) win_infoarg("Inizializzare EhListView()");
	 memset(&LVList,0,sizeof(EH_LVLIST)*LVMAX);
	 EhListInizialize();
	 fReset=FALSE;
	 return 0;
 }

 if (OBJ_CallSub==NULL) PRG_end("ELV: NULL %d",cmd);

 // Cerco una cella in LVLIST
 iLVIndex=LVFind(LV_FINDOBJ,OBJ_CallSub);
 if (iLVIndex<0) iLVIndex=LVAlloc(OBJ_CallSub);

 switch(cmd)
	{
		case WS_INF:
			return &LVList[iLVIndex];
 
		case WS_OPEN: // Creazione
			
			OBJ_CallSub->hWnd=CreateListView(OBJ_CallSub->nome,sys.EhWinInstance,WindowNow());
			LVList[iLVIndex].hWndList=OBJ_CallSub->hWnd;
			LVList[iLVIndex].hWndHeader=GetWindow(OBJ_CallSub->hWnd, GW_CHILD);
			LVList[iLVIndex].fLeftClick=TRUE;
			LVList[iLVIndex].fRightClick=TRUE;
			LVList[iLVIndex].fDoubleClick=FALSE;
			break;

		case WS_EXTFUNZ:
			switch (info)
			{
				case 0: LVList[iLVIndex].subListNotify=ptr; break;
				case 1: LVList[iLVIndex].subMessage=ptr; break;
				case 2: LVList[iLVIndex].subHeaderNotify=ptr; break;
			}				
			break;

		case WS_SETFLAG:

			if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE);
				dwStyle|=info;
				SetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"!STYLE")) // Setta lo stile della finestra
			{
				LONG lStyle;
				lStyle=GetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE);
				lStyle&=~info;
				SetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE, lStyle);
			}

			if (!strcmp(ptr,"STYLEMASK")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE);
				//win_infoarg("%08x",~LVS_TYPESTYLEMASK);
				dwStyle=dwStyle&~LVS_TYPESTYLEMASK|info;
				SetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(OBJ_CallSub->hWnd);
				dwExStyle|=info;
				ListView_SetExtendedListViewStyle(OBJ_CallSub->hWnd,dwExStyle);
			}

			if (!strcmp(ptr,"!EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(OBJ_CallSub->hWnd);
				dwExStyle&=~info;
				ListView_SetExtendedListViewStyle(OBJ_CallSub->hWnd,dwExStyle);
			}

			if (!strcmp(ptr,"DCLK")) // Setta il Double Click
			{
				LVList[iLVIndex].fDoubleClick=info;
			}
			break;

		case WS_CLOSE: // Distruzione
			DestroyWindow(OBJ_CallSub->hWnd);
			break;

		case WS_DEL: // Cancella le colonne
			while (TRUE)
			{
				if (!ListView_DeleteColumn(OBJ_CallSub->hWnd,0)) break;
			}
    		break;

		case WS_ADD: // Aggiungi una colonna
			ListView_InsertColumn(OBJ_CallSub->hWnd, info, (LV_COLUMN *) ptr);
			break;

		case WS_LINK: // Aggiunge un Item
			//
			if (ListView_SetItem(OBJ_CallSub->hWnd,(LPLVITEM) ptr)==FALSE) 
				{ListView_InsertItem(OBJ_CallSub->hWnd,(LPLVITEM) ptr);}
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(OBJ_CallSub->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
			break;

		case WS_DISPLAY: 
			InvalidateRect(OBJ_CallSub->hWnd,NULL,TRUE);
			break;
	}
	return NULL;
}

// -----------------------------------------------
// List view di lettura (+ sort header)
// -----------------------------------------------

void *EhListViewSort(SINT cmd,LONG info,void *ptr)
{
 struct WS_DISPEXT *DExt=ptr;
 static SINT HdbMovi=-1;
 static INT16 iSend;
 DWORD dwExStyle;
 SINT iLVIndex;
 
 if (fReset)
 {
	 if (cmd!=WS_START) win_infoarg("Inizializzare EhListView()");
	 memset(&LVList,0,sizeof(EH_LVLIST)*LVMAX);
	 EhListInizialize();
	 fReset=FALSE;
	 return 0;
 }

 if (OBJ_CallSub==NULL) PRG_end("ELV: NULL %d",cmd);

 // Cerco una cella in LVLIST
 iLVIndex=LVFind(LV_FINDOBJ,OBJ_CallSub);
 if (iLVIndex<0) iLVIndex=LVAlloc(OBJ_CallSub);

 switch(cmd)
	{
		case WS_INF:
			return &LVList[iLVIndex];
 
		case WS_OPEN: // Creazione
			
			OBJ_CallSub->hWnd=CreateListViewSort(OBJ_CallSub->nome,sys.EhWinInstance,WindowNow());
			//LVList[iLVIndex].hWndList=OBJ_CallSub->hWnd;
			LVList[iLVIndex].hWndList=OBJ_CallSub->hWnd;
			LVList[iLVIndex].hWndHeader=GetWindow(OBJ_CallSub->hWnd, GW_CHILD);
			LVList[iLVIndex].fLeftClick=TRUE;
			LVList[iLVIndex].fRightClick=TRUE;
			LVList[iLVIndex].fDoubleClick=FALSE;
			break;

		case WS_EXTFUNZ:
			switch (info)
			{
				case 0: LVList[iLVIndex].subListNotify=ptr; break;
				case 1: LVList[iLVIndex].subMessage=ptr; break;
				case 2: LVList[iLVIndex].subHeaderNotify=ptr; break;
			}				
			break;

		case WS_SETFLAG:

			if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE);
				dwStyle|=info;
				SetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"!STYLE")) // Setta lo stile della finestra
			{
				LONG lStyle;
				lStyle=GetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE);
				lStyle&=~info;
				SetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE, lStyle);
			}

			if (!strcmp(ptr,"STYLEMASK")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE);
				//win_infoarg("%08x",~LVS_TYPESTYLEMASK);
				dwStyle=dwStyle&~LVS_TYPESTYLEMASK|info;
				SetWindowLongW(OBJ_CallSub->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(OBJ_CallSub->hWnd);
				dwExStyle|=info;
				ListView_SetExtendedListViewStyle(OBJ_CallSub->hWnd,dwExStyle);
			}

			if (!strcmp(ptr,"!EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(OBJ_CallSub->hWnd);
				dwExStyle&=~info;
				ListView_SetExtendedListViewStyle(OBJ_CallSub->hWnd,dwExStyle);
			}

			if (!strcmp(ptr,"DCLK")) // Setta il Double Click
			{
				LVList[iLVIndex].fDoubleClick=info;
			}
			break;

		case WS_CLOSE: // Distruzione
			DestroyWindow(OBJ_CallSub->hWnd);
//			LVList[iLVIndex].lpObj=NULL;
			break;

		case WS_ADD: // Aggiungi una colonna
			ListView_InsertColumn(OBJ_CallSub->hWnd, info, (LV_COLUMN *) ptr);
			break;

		case WS_LINK: // Aggiunge un Item
			//
			if (ListView_SetItem(OBJ_CallSub->hWnd,(LPLVITEM) ptr)==FALSE) 
				{ListView_InsertItem(OBJ_CallSub->hWnd,(LPLVITEM) ptr);}
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(OBJ_CallSub->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
			break;

		case WS_DISPLAY: 
			InvalidateRect(OBJ_CallSub->hWnd,NULL,FALSE);
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
				WS_VISIBLE |
				LVS_NOSORTHEADER |
				LVS_REPORT;

	if (strstr(lpName,"_O")) dwStyle|=LVS_OWNERDATA;
            
	hwndListView = CreateWindowExW(WS_EX_CONTROLPARENT|
								   WS_EX_CLIENTEDGE,          // ex style
                                   WC_LISTVIEWW,               // class name - defined in commctrl.h
                                   L"",                        // dummy text
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


//set the image lists
/*
himlSmall = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 1, 0);
himlLarge = ImageList_Create(32, 32, ILC_COLORDDB | ILC_MASK, 1, 0);
if (himlSmall && himlLarge)
   {
   HICON hIcon;

   //set up the small image list
   hIcon = LoadImage(sys.EhWinInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
   ImageList_AddIcon(himlSmall, hIcon);

   //set up the large image list
   hIcon = LoadIcon(sys.EhWinInstance, MAKEINTRESOURCE(IDI_ICON1));
   ImageList_AddIcon(himlLarge, hIcon);

   ListView_SetImageList(hwndListView, himlSmall, LVSIL_SMALL);
   ListView_SetImageList(hwndListView, himlLarge, LVSIL_NORMAL);
   }
*/
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
DWORD dwStyle = GetWindowLongW(hwndListView, GWL_STYLE);

	/*To ensure that the first item will be visible, create the control without 
	the LVS_NOSCROLL style and then add it here*/
	dwStyle |= LVS_NOSCROLL;
	SetWindowLongW(hwndListView, GWL_STYLE, dwStyle);

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
   ListViewNotify()
   Interpreta le notifiche della listview
**************************************************************************/

static LRESULT ListViewNotify(HWND hWnd, LPARAM lParam,SINT iLVIndex,LPNMHDR  pnmh)
{
 LPNMLVKEYDOWN pnkd;
 LPNMLISTVIEW lpnmlv;
 struct OBJ *poj;
 CHAR Serv[20];
 BOOL fReturn;
 LRESULT lRes;

 poj=LVList[iLVIndex].lpObj;
 if (LVList[iLVIndex].subListNotify)
 {
    fReturn=FALSE;
	lRes=(*LVList[iLVIndex].subListNotify)(LVSUB_PRE,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

 if (pnmh->code==0) efx2();
 switch(pnmh->code)
   {

	case NM_SETFOCUS:
		if (poj!=NULL)
		{
		 sprintf(Serv,"%sFS",poj->nome);
		 obj_VStack(Serv);
		}
		break;

	case NM_DBLCLK:
		if (!LVList[iLVIndex].fDoubleClick) break;
		lpnmlv = (LPNMLISTVIEW) lParam;
		if (poj!=NULL)
		{
		 OBJ_key=(LONG) (lpnmlv->iItem+1)*10000+(lpnmlv->iSubItem+1);
		 sprintf(Serv,"%sDCLK",poj->nome);
		 obj_VStack(Serv);
		}
		break;

	case NM_RCLICK:
		lpnmlv = (LPNMLISTVIEW) lParam;
		if (poj!=NULL)
		{
		 OBJ_key=(LONG) (lpnmlv->iItem+1)*10000+(lpnmlv->iSubItem+1);
		 sprintf(Serv,"%sRC",poj->nome);
		 obj_VStack(Serv);
		}
		break;

	case NM_CLICK:

		lpnmlv = (LPNMLISTVIEW) lParam;
		if (LVList[iLVIndex].lpClickMask&&(lpnmlv->iSubItem>-1))
		{
			if (LVList[iLVIndex].lpClickMask[lpnmlv->iSubItem]==' ') {break;}
		}
		
		if (poj!=NULL)
		{
			OBJ_key=(LONG) (lpnmlv->iItem+1)*10000+(lpnmlv->iSubItem+1);
			//SetFocus(WindowNow());
			sprintf(Serv,"%sLW",poj->nome);
			obj_VStack(Serv);
		}
		break;
 
	case LVN_KEYDOWN:
		pnkd = (LPNMLVKEYDOWN) lParam;   
		if (pnkd->wVKey==ESC) SetFocus(WindowNow());
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
		OBJ_CallSub=LGetOBJ(hWnd);
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
 if (LVList[iLVIndex].subListNotify)
 {
    fReturn=FALSE;
	lRes=(*LVList[iLVIndex].subListNotify)(LVSUB_POST,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

return 0;
}

static LRESULT ListHeaderNotify(HWND hWnd, LPARAM lParam,SINT iLVIndex,LPNMHDR  pnmh)
{
 struct OBJ *poj;
 BOOL fReturn;
 LRESULT lRes;

 poj=LVList[iLVIndex].lpObj;
 if (LVList[iLVIndex].subHeaderNotify)
 {
    fReturn=FALSE;
	lRes=(*LVList[iLVIndex].subHeaderNotify)(LVSUB_PRE,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

 if (LVList[iLVIndex].subHeaderNotify)
 {
    fReturn=FALSE;
	lRes=(*LVList[iLVIndex].subHeaderNotify)(LVSUB_POST,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

return 0;
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

/*
void LVItemSetState(HWND hwndList,SINT iItem,SINT iSubItem,CHAR *lpText)
{
	LVITEM lvItem;

	lvItem.mask=LVIF_TEXT|LVIF_STATE;
    lvItem.pszText=lpText;
	lvItem.iItem=iItem;
	lvItem.iSubItem=iSubItem;
	//ListView_SetItem(hwndLVFields,&lvItem);
	if (ListView_SetItem(hwndList,&lvItem)==FALSE) {ListView_InsertItem(hwndList,&lvItem);}
}
*/

/**************************************************************************
   SwitchView()
**************************************************************************/

