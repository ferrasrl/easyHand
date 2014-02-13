//   -----------------------------------------------------------
//   | ZCMP_ListBox
//   | ZoneComponent ListBox
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
//   |						by Ferrà Art & Technology 1993-2000
//   -----------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ZCMP_ListBox.h"

static LRESULT ListViewNotify(HWND hWnd, LPARAM lParam,SINT iLBIndex,LPNMHDR  pnmh);
static BOOL InsertListViewItems(HWND hwndListView);
static HWND CreateListBox(HINSTANCE hInstance, HWND hwndParent);
static BOOL InitListView(HWND);
static void SwitchView(HWND hwndListView, DWORD dwView);

static struct {

	BOOL	bReady;
	WNDPROC EhOriginalProc;
//	WNDPROC funcTextOld;
//	WNDPROC funcSelectOld;
//	WNDPROC funcButtonOld;

} sListBox= {0};

//#define LBMAX 16
//static EH_LBLIST LBList[LBMAX];
//static BOOL fReset=TRUE;
//#define LB_FINDOBJ  0
//#define LB_FINDHWND 1

static void _ListBoxInitialize(void)
{
//	WNDCLASSEX wc;
//	CHAR NewClass[80];

	sListBox.bReady=TRUE;

/*

	sprintf(NewClass,"%sL",sys.tzWinClassBase);
	wc.cbSize=sizeof(wc);
	if (!GetClassInfoEx(sys.EhWinInstance,sys.tzWinClassBase,&wc))  ehExit("Errore 1");

	if (wc.lpfnWndProc!=LIntercept)
	{
		EhOriginalProc=wc.lpfnWndProc;
		wc.lpfnWndProc=LIntercept;
		wc.lpszClassName=NewClass;
		if (!RegisterClassEx(&wc)) win_infoarg("error");
		strcpy(sys.tzWinClassBase,NewClass);
	}
	*/
}

// -----------------------------------------------
// List view di lettura (senza sort header)
// -----------------------------------------------
void * EhListBox(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr)
{
	EH_DISPEXT *DExt=ptr;
	static SINT HdbMovi=-1;
	static INT16 iSend;
	DWORD dwExStyle;
	SINT iLine;
	SINT idx;
	EHZ_LISTBOX *psListBox;
/*
	if (sListBox.bReady)
	{
	//	 if (cmd!=WS_START) win_infoarg("Inizializzare EhListBox()");
	//	 memset(&LBList,0,sizeof(EH_LBLIST)*LBMAX);
	 _ListBoxInitialize();
	//	 fReset=FALSE;
	 return 0;
	}

//	if (objCalled==NULL) ehExit("ELV: NULL %d",cmd);

	// Cerco una cella in LBList
//	iLBIndex=LBFind(LB_FINDOBJ,objCalled);
//	if (iLBIndex<0) iLBIndex=LBAlloc(objCalled);
*/
	if (!objCalled) {alert("Togliere EhListBox(NULL,WS_START,0,NULL);"); return NULL;} // Compatibilità con il vecchio
	psListBox=objCalled->pOther;
	if (!sListBox.bReady) _ListBoxInitialize();

	switch(cmd)
	{
		case WS_INF:
			return psListBox;//&LBList[iLBIndex];
 
		case WS_CREATE: 
			psListBox=objCalled->pOther=ehAllocZero(sizeof(EHZ_LISTBOX));
			psListBox->wnd=objCalled->hWnd=CreateListBox(sys.EhWinInstance,WindowNow());//_LCreateRichText(sys.EhWinInstance,WindowNow());
			psListBox->fLeftClick=TRUE;
			psListBox->fRightClick=TRUE;
			psListBox->fDoubleClick=FALSE;
			psListBox->iMaxLine=500;
			SendMessage(psListBox->wnd,LB_INITSTORAGE,psListBox->iMaxLine,psListBox->iMaxLine*80);
			break;

		case WS_DESTROY: // Distruzione
			DestroyWindow(objCalled->hWnd);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_OPEN: // Creazione
			//objCalled->hWnd=CreateListBox(sys.EhWinInstance,WindowNow());
			break;

		case WS_EXTNOTIFY:
			if (psListBox) psListBox->subPostNotify=ptr;
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
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				dwStyle&=~info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
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
				psListBox->fDoubleClick=info;
			}
			break;


		case WS_DEL: // Cancella le colonne
	
			while (TRUE)
			{
				if (!ListView_DeleteColumn(objCalled->hWnd,0)) break;
			}
    		break;

		case WS_ADD: // Aggiungi una colonna
			//ListView_InsertColumn(objCalled->hWnd, info, (LV_COLUMN *) ptr);
			switch (info)
			{
				case 1: // Autoscroll
						idx=SendMessage(objCalled->hWnd,LB_ADDSTRING,0,(LPARAM) ptr);
						while (TRUE)
						{
						 iLine=SendMessage(objCalled->hWnd,LB_GETCOUNT,0,0);
						 if (iLine>=psListBox->iMaxLine) 
						 {
							SendMessage(objCalled->hWnd,LB_DELETESTRING,0,0);
						 } else break;
						}
//						SendMessage(objCalled->hWnd,LB_SETCARETINDEX ,iLine,MAKELPARAM(TRUE, 0));
						SendMessage(objCalled->hWnd,LB_SETTOPINDEX,iLine-1,0);
						break;
				default:

						idx=SendMessage(objCalled->hWnd,LB_ADDSTRING,0,(LPARAM) ptr);
						break;
			}
			break;

		case WS_UPDATE: // Sostituisco l'ultima stringa scritta
				 iLine=SendMessage(objCalled->hWnd,LB_GETCOUNT,0,0);
				 if (iLine>0) 
				 {
				   SendMessage(objCalled->hWnd,LB_DELETESTRING,iLine-1,0);
				   SendMessage(objCalled->hWnd,LB_ADDSTRING,0,(LPARAM) ptr);
				 // SendMessage(objCalled->hWnd,LB_SETTOPINDEX,iLine-1,0);
				  //SendMessage(objCalled->hWnd,WM_SETREDRAW,TRUE,0);
				  

				  /*
					 BYTE *pStr;
					 //SINT idx=SendMessage(objCalled->hWnd,LB_SETCARETINDEX,0,0);
					 pStr=(BYTE *) SendMessage(objCalled->hWnd,LB_GETITEMDATA,(WPARAM) (iLine-1),0);
					 SendMessage(objCalled->hWnd,LB_SETITEMDATA,iLine-1,(LPARAM) ptr);
					 */
				  //InvalidateRect(objCalled->hWnd,NULL,FALSE);
				 }
				 break;
/*

		case WS_LINK: // Aggiunge un Item
			//
			if (ListView_SetItem(objCalled->hWnd,(LPLVITEM) ptr)==FALSE) 
				{ListView_InsertItem(objCalled->hWnd,(LPLVITEM) ptr);}
			break;
*/
		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
			break;
			
		case WS_DISPLAY: 
			InvalidateRect(objCalled->hWnd,NULL,TRUE);
			break;
	}
	return NULL;
}


/******************************************************************************

   CreateListView

******************************************************************************/

static WNDPROC OldListProc=NULL;

static HWND CreateListBox(HINSTANCE hInstance, HWND hwndParent)
{
	DWORD       dwStyle;
	HWND        hwndListBox;
	BOOL        bSuccess = TRUE;

	dwStyle =   WS_TABSTOP| 
				WS_CHILD| 
				WS_BORDER| 
				WS_VSCROLL|
				//WS_HSCROLL|
				WS_VISIBLE|
				LBS_NOSEL| 
				LBS_HASSTRINGS|
				LBS_DISABLENOSCROLL|
				LBS_USETABSTOPS;
				//LBS_NOREDRAW ;
				//LBS_EXTENDEDSEL
				//LVS_AUTOARRANGE 
				//LVS_NOSORTHEADER |
				//LVS_REPORT;
				//LVS_OWNERDATA;
            
	hwndListBox = CreateWindowEx( WS_EX_CONTROLPARENT//|
								  //WS_EX_CLIENTEDGE
								  ,          // ex style
                                  "LISTBOX",                // class name - defined in commctrl.h
                                  "",                        // dummy text
                                  dwStyle,                   // style
                                  0,                         // x position
                                  0,                         // y position
                                  0,                         // width
                                  0,                         // height
                                  hwndParent,                // parent
                                  (HMENU) ID_LISTBOX,        // ID
                                  sys.EhWinInstance,                   // instance
                                  NULL);                     // no extra data
	if(!hwndListBox) return NULL;
	
	SendMessage(hwndListBox,WM_SETFONT,(WPARAM) GetStockObject(ANSI_VAR_FONT),MAKELPARAM(TRUE, 0));
	return hwndListBox;
}


/**************************************************************************
   ListViewNotify()
   Interpreta le notifiche della listview
**************************************************************************/
/*
static LRESULT ListViewNotify(HWND hWnd, LPARAM lParam,SINT iLBIndex,LPNMHDR  pnmh)
{

	LPNMLVKEYDOWN pnkd;
	LPNMLISTVIEW lpnmlv;
	struct OBJ *poj;
	CHAR Serv[20];
	BOOL fReturn;
	LRESULT lRes;

 poj=LBList[iLBIndex].lpObj;
 
 if (LBList[iLBIndex].subPostNotify)
 {
    fReturn=FALSE;
	lRes=(*LBList[iLBIndex].subPostNotify)(LVSUB_PRE,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

 if (pnmh->code==0) efx2();
 switch(pnmh->code)
   {

	case NM_SETFOCUS:
		if (poj!=NULL)
		{
		 sprintf(Serv,"%sFS",poj->nome);
		 obj_addevent(Serv);
		}
		break;

	case NM_DBLCLK:
		if (!LBList[iLBIndex].fDoubleClick) break;
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
		if (LBList[iLBIndex].lpClickMask&&(lpnmlv->iSubItem>-1))
		{
			if (LBList[iLBIndex].lpClickMask[lpnmlv->iSubItem]==' ') {break;}
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
      }
      //return 0;
	  break;

	  //return 0;
   }

 // Lancio se connessa la procedura di controllo esterna
 if (LBList[iLBIndex].subPostNotify)
 {
    fReturn=FALSE;
	lRes=(*LBList[iLBIndex].subPostNotify)(LVSUB_POST,poj,hWnd,lParam,pnmh,&fReturn);
	if (fReturn) return lRes;
 }

return 0;
}
*/
void LBItemSet(HWND hwndList,SINT iItem,SINT iSubItem,CHAR *lpText)
{
	LVITEM lvItem;

	lvItem.mask=LVIF_TEXT;
    lvItem.pszText=lpText;
	lvItem.iItem=iItem;
	lvItem.iSubItem=iSubItem;
	//ListView_SetItem(hwndLVFields,&lvItem);
	if (ListView_SetItem(hwndList,&lvItem)==FALSE) {ListView_InsertItem(hwndList,&lvItem);}
}

void LBItemSetParam(HWND hwndList,SINT iItem,SINT iSubItem,CHAR *lpText,LONG lParam)
{
	LVITEM lvItem;

	lvItem.mask=LVIF_TEXT|LVIF_PARAM;
    lvItem.pszText=lpText;
	lvItem.iItem=iItem;
	lvItem.iSubItem=iSubItem;
	lvItem.lParam=lParam;
	if (ListView_SetItem(hwndList,&lvItem)==FALSE) {ListView_InsertItem(hwndList,&lvItem);}
}
