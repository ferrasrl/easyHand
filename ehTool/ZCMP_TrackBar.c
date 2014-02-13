//   ---------------------------------------------
//   | ZCMP_TrackBar
//   | ZoneComponent TrackBar
//   |                                              
//   | Gestisce una TrackBar di windows
//   | in una oggetto ZONAP                        
//   |                                              
//   |  ATTENZIONE:                               
//   |                                              
//   |                                              
//   |         by Ferrà Art & Technology 1993-2000
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/fbfile.h"
#include "/easyhand/ehtool/ZCMP_TrackBar.h"

//#define ID_LISTVIEW 25000
#define TBMAX 16
static EH_TBLIST TBList[TBMAX];
static BOOL fReset=TRUE;
#define TB_FINDOBJ  0
#define TB_FINDHWND 1

static LRESULT TrackBarNotify(HWND hWnd, WPARAM wParam, LPARAM lParam,SINT iTBIndex);
//static BOOL InsertListViewItems(HWND hwndListView);
//static HWND CreateListViewSort(HINSTANCE hInstance, HWND hwndParent);
static void ResizeTrackBar(HWND, EH_DISPEXT *psExt);
//static BOOL InitTrackBar(HWND);
static void SwitchTBView(HWND hwndListView, DWORD dwView);

static SINT TBFind(SINT iCosa,void *ptr)
{
	SINT a;
	for (a=0;a<TBMAX;a++)
	{
		switch (iCosa)
		{
			case TB_FINDOBJ:  if (TBList[a].lpObj==(struct OBJ *) ptr) return a;
							  break;
			case TB_FINDHWND: if (TBList[a].hWndTrack==(HWND) ptr) return a;
							  break;
		}
	}
	return -1;
}

static SINT TBAlloc(struct OBJ *obj)
{
	SINT a;
	for (a=0;a<TBMAX;a++)
	{
	  if (TBList[a].lpObj==NULL)
	  {
		TBList[a].lpObj=obj;
		return a;
	  }
	}
	ehExit("TB: overload");
	return 0;
}

static HWND CreateTrackBar(HINSTANCE hInstance, HWND hwndParent);

static WNDPROC EhOriginalProc=NULL;
static LRESULT CALLBACK LIntercept(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	if (message==WM_HSCROLL) // Controllo se è mio
	{
		//LPNMHDR  pnmh = (LPNMHDR) lParam;
	 SINT iTBIndex=TBFind(TB_FINDHWND,(HWND) lParam);
	 if (iTBIndex>=0)
	 {
		return TrackBarNotify(hWnd, wParam,lParam,iTBIndex);                                              
	 }

	}
	return CallWindowProc(EhOriginalProc,hWnd,message,wParam,lParam);
} 

static void EhTrackInizialize(void)
{
	//WNDPROC EhProc;
	WNDCLASSEX wc;
	// Cambio il puntamento standarda della procedura di classe Eh
	CHAR NewClass[80];

	sprintf(NewClass,"%sTB",sys.tzWinClassBase);
	wc.cbSize=sizeof(wc);
	if (!GetClassInfoEx(sys.EhWinInstance,sys.tzWinClassBase,&wc)) 
		ehExit("Errore 1");
	if (wc.lpfnWndProc!=LIntercept)
	{
		EhOriginalProc=wc.lpfnWndProc;
		wc.lpfnWndProc=LIntercept;
		wc.lpszClassName=NewClass;
		if (!RegisterClassEx(&wc)) win_infoarg("error");
		strcpy(sys.tzWinClassBase,NewClass);
	}
}


// -----------------------------------------------
// TrackBar di lettura (senza sort header)
// -----------------------------------------------

void * EhTrackBar(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr)
{
 EH_DISPEXT *psExt=ptr;
 static SINT HdbMovi=-1;
 static INT16 iSend;
 DWORD dwExStyle;
 SINT iTBIndex;

 if (fReset)
 {
	 if (cmd!=WS_START) win_infoarg("Inizializzare EhTrackBar()");
	 memset(&TBList,0,sizeof(EH_TBLIST)*TBMAX);
	 EhTrackInizialize();
	 fReset=FALSE;
	 return 0;
 }

 // if (objCalled==NULL) ehExit("ETB: NULL %d",cmd);

 // Cerco una cella in TBLIST
 iTBIndex=TBFind(TB_FINDOBJ,objCalled);
 if (iTBIndex<0) iTBIndex=TBAlloc(objCalled);

 switch(cmd)
	{
		case WS_INF:
			return &TBList[iTBIndex];
 
		case WS_OPEN: // Creazione della Track
			
			objCalled->hWnd=CreateTrackBar(sys.EhWinInstance,WindowNow());
			TBList[iTBIndex].hWndTrack=objCalled->hWnd;
			break;

		case WS_EXTNOTIFY:
			TBList[iTBIndex].subPostNotify=ptr;
			break;

		case WS_SETFLAG:

			if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				dwStyle|=info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(objCalled->hWnd);
				dwExStyle|=info;
				ListView_SetExtendedListViewStyle(objCalled->hWnd,dwExStyle);
			}

			break;

		case WS_CLOSE: // Distruzione
			DestroyWindow(objCalled->hWnd);
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,psExt->px+relwx,psExt->py+relwy,psExt->lx,psExt->ly,TRUE);
			break;

		case WS_DISPLAY: break;
	}
	return NULL;
}


/******************************************************************************

   CreateTrackBar

******************************************************************************/

static WNDPROC OldListProc=NULL;

static HWND CreateTrackBar(HINSTANCE hInstance, HWND hwndParent)
{
	DWORD       dwStyle;
	HWND        hwndTrackBar;
	BOOL        bSuccess = TRUE;

	dwStyle =   WS_CHILD | 
				WS_VISIBLE |
				TBS_AUTOTICKS | 
				//TBS_ENABLESELRANGE |
				TBS_FIXEDLENGTH;//|
				//TBS_HORZ|TBS_TOP;
			
	hwndTrackBar = CreateWindowEx( 
								0,                             // no extended styles 
								TRACKBAR_CLASS,                // class name 
								"Trackbar Control",            // title (caption) 
								dwStyle,  // style 
								0, 0,                        // position 
								0, 0,                       // size 
								hwndParent,                       // parent window 
								(HMENU) ID_TRACKBAR,             // control identifier 
								hInstance,                       // instance 
								NULL                           // no WM_CREATE parameter 
								); 
            
	if(!hwndTrackBar) return NULL;
 return hwndTrackBar;
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

	dwStyle |= TBS_NOSCROLL;
	SetWindowLong(hwndListView, GWL_STYLE, dwStyle);

	//only do this if we are in report view and were able to get the header hWnd
	if(((dwStyle & TBS_TYPEMASK) == TBS_REPORT) && hwndHeader)
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
*/
/**************************************************************************
   ListViewNotify()
   Interpreta le notifiche della listview
**************************************************************************/
/*
	VOID WINAPI TBNotifications( 
    WPARAM wParam,  // wParam of WM_HSCROLL message 
    HWND hwndTrack, // handle of trackbar window 
    UINT iSelMin,   // minimum value of trackbar selection 
    UINT iSelMax)   // maximum value of trackbar selection 
	{ 
    DWORD dwPos;    // current position of slider 
 
    switch (LOWORD(wParam)) { 
        case TB_ENDTRACK: 
            dwPos = SendMessage(hwndTrack, TBM_GETPOS, 0, 0); 
            if (dwPos > iSelMax) 
                SendMessage(hwndTrack, TBM_SETPOS, 
                    (WPARAM) TRUE,       // redraw flag 
                    (LPARAM) iSelMax); 
            else if (dwPos < iSelMin) 
                SendMessage(hwndTrack, TBM_SETPOS, 
                    (WPARAM) TRUE,       // redraw flag 
                    (LPARAM) iSelMin); 
            break; 
 
        default: 
            break; 
 
        } 
	} 
*/
static LRESULT TrackBarNotify(HWND hWnd, WPARAM wParam,LPARAM lParam,SINT iTBIndex)
{
 SINT iSlider;
 SINT iNotify;
 HWND hwndTrackBar=(HWND) lParam;
 struct OBJ *poj;
 BOOL fReturn;
 LRESULT lRes;
 //CHAR Serv[20];

 poj=TBList[iTBIndex].lpObj;
 iSlider=HIWORD(wParam);
 iNotify=LOWORD(wParam);
 
 if (TBList[iTBIndex].subPostNotify)
 {
    fReturn=FALSE;
	lRes=(*TBList[iTBIndex].subPostNotify)(poj,&fReturn, EXT_PREV,hwndTrackBar,0,wParam,lParam);
	if (fReturn) return lRes;
 }
 
 switch (iNotify)
 {
	 /*
	case TB_THUMBTRACK:
				dispx("%d                ",iSlider);
				break;
*/

	case TB_THUMBPOSITION:
				//dispx("POS:%d                ",iSlider);
				OBJ_key=iSlider;
				//sprintf(Serv,"%s",poj->nome);
				obj_addevent(poj->nome);
				break;

	case TB_LINEDOWN:
	case TB_LINEUP:
	case TB_PAGEDOWN:
	case TB_BOTTOM:
	case TB_TOP:
	case TB_ENDTRACK:
				//dispx("GET:%d              ",SendMessage(hwndTrackBar,TBM_GETPOS,0,0));
				OBJ_key=SendMessage(hwndTrackBar,TBM_GETPOS,0,0);
				obj_addevent(poj->nome);
				break;
 }

 if (TBList[iTBIndex].subPostNotify)
 {
    fReturn=FALSE;
	lRes=(*TBList[iTBIndex].subPostNotify)(poj,&fReturn, EXT_AFTER, hWnd,0, wParam,lParam);
	if (fReturn) return lRes;
 }
return 0;
}

