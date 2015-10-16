//   ---------------------------------------------
//	 | WTOOLBAR                                  |
//	 | Toolbar Mobile Windows                    |
//	 |                             by Ferrà 1999 |
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/WToolBar.h"

LRESULT CALLBACK _funcToolProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static void GiveToolRect(S_WTOOLBAR *Tb,INT x,INT y,LPRECT lprc);
static void _keysMove(S_WTOOLBAR *Tb);
static void LKeySelect(S_WTOOLBAR *Tb,INT wID);
/*
typedef struct {

//	HWND hWnd;
	WTOOLBAR *	psTb;

} TOOLBARLIST;
*/
static struct {

	BOOL	bReady;
//	TOOLBARLIST arsToolBat[10];
//	INT		iToolNow;
	INT		iPres;
//	RECT	sRect;
//	POINT	sClick;

} _s={false};

/*
static BOOL Init=FALSE;
static TOOLBARLIST arsToolBat[10];
static INT ToolNow=-1;
static INT pPres=12;
*/

S_WTOOLBAR * ToolCreate(S_WTOBJ * arsObj,INT KeyLx,INT KeyLy,INT KeyNumx)
{
    CHAR *ClassName="Crea6ToolBar";
    WNDCLASSEX wc;
	S_WTOOLBAR * psTb;
	INT c;
	RECT Rect;

	if (!_s.bReady)
	{
		_(_s);
//		_s.iToolNow=-1;
		_s.iPres=12;

		wc.cbSize        = sizeof(wc);
		wc.style         = 0;//CS_DBLCLKS;

		// Funzione chiamata per la gestione della finestra
		wc.lpfnWndProc   = _funcToolProcedure;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = sys.EhWinInstance;
		wc.hIcon         = NULL;//LoadIcon(hInstance,MAKEINTRESOURCE(IDI_CREA1));
		wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = ClassName;
		wc.hIconSm       = NULL; // Small Icone  (NULL = Automatico)
		RegisterClassEx(&wc);


		// for (a=0;a<10;a++) _s.arsToolBat[a].hWnd=NULL;
		_s.bReady=true;
	}
    

	// Creo memoria per struttura
	// Conto gli oggetti
	for (c=0;;c++) {if (arsObj[c].iType==STOP) break;}

	//
    // Trovo il primo posto disponibile
	//
//    _s.iToolNow=-1;
//    for (a=0;a<10;a++) {if (_s.arsToolBat[a].hWnd==NULL) {_s.iToolNow=a; break;}}
  //  if (_s.iToolNow==-1) ehExit("ToolBar Full");

//	Hdl=memoAlloc(RAM_AUTO,sizeof(WTOOLBAR)+(c*sizeof(HWND)),"WToolBar");
//	psTb=memoLock(Hdl);
	psTb=ehAllocZero(sizeof(S_WTOOLBAR));
	psTb->KeyLx=KeyLx;
	psTb->KeyLy=KeyLy;
	psTb->KeySx=2;
	psTb->KeySy=2;
    psTb->iElement=c; // Numero di bottoni
	psTb->hWndOwner=WindowNow();//WIN_info[sys.WinInputFocus].hWnd;
//	psTb->arhButton=(HWND *) ((TCHAR *) (psTb)+sizeof(WTOOLBAR)); // Metto gli handle in fondo dopo la struttura
	psTb->arhButton=ehAllocZero(sizeof(HWND)*psTb->iElement);
	//psTb->hButtonTT=(HWND *) ((TCHAR *) (psTb->arhButton)+(sizeof(HWND)*psTb->Num));
	psTb->arsObj=arsObj;
//	_s.arsToolBat[_s.iToolNow].Tb=Tb;

//	psTb->hWnd=CreateWindowEx(WS_EX_NOPARENTNOTIFY|WS_EX_TOOLWINDOW,
	psTb->hWnd=CreateWindowEx( WS_EX_NOPARENTNOTIFY|WS_EX_TOOLWINDOW, // |WS_EX_TOOLWINDOW
							   ClassName, 
							   "",
							   //WS_POPUP|WS_BORDER|WS_VISIBLE|WS_THICKFRAME
	//						   WS_CLIPCHILDREN|WS_POPUP|WS_THICKFRAME|WS_BORDER,
							   WS_CHILD|WS_THICKFRAME|WS_BORDER,
							   0, 90,
							   100,100,
							   WIN_info[sys.WinInputFocus].hWnd, 
							   0,
							   sys.EhWinInstance,
							   psTb);
	Rect.top=0; Rect.left=0;
	GiveToolRect(psTb,KeyNumx,(psTb->iElement/KeyNumx),&Rect);
	MoveWindow(psTb->hWnd,Rect.left,Rect.top,Rect.right-Rect.left,Rect.bottom-Rect.top,TRUE);

	_keysMove(psTb);
	ShowWindow(psTb->hWnd,SW_SHOW);
	return psTb;
}


//
// ToolDestroy()
// 
void ToolDestroy(S_WTOOLBAR * psTb)
{
  DestroyWindow(psTb->hWnd); 
  ehFree(psTb->arhButton);
  ehFree(psTb);
  /*
  for (a=0;a<10;a++)
  {
	if (_s.arsToolBat[a].hWnd==psTb->hWnd) {_s.arsToolBat[a].hWnd=NULL; break;}
  }
  memoFree(psTb->hMemo,"Crea");
  */
}



// ---------------------------------------------------------------
// Riposiziona i tasti in base alle dimenzioni dell'area client
// ---------------------------------------------------------------

static void _keysMove(S_WTOOLBAR * psTb)
{
	INT a,x,y;
	INT WLx;

	RECT Rect;
	INT adx,ady;

	GetClientRect(psTb->hWnd,&Rect);  WLx=Rect.right-Rect.left+1;
	x=psTb->KeySx;y=psTb->KeySy;

	if (psTb->ClientLx>psTb->ClientLy) {adx=_s.iPres/2;ady=0;} else {adx=0;ady=_s.iPres/2;}
	for (a=0;a<psTb->iElement;a++)
	{
		// Muovo il Tasto
		MoveWindow(psTb->arhButton[a],x+adx,y+ady,psTb->KeyLx,psTb->KeyLy,FALSE);
		x=x+psTb->KeyLx; if ((x+psTb->KeyLx+psTb->KeySx)>WLx) {x=psTb->KeySx; y=y+psTb->KeyLy;}
	}

	InvalidateRect(psTb->hWnd,NULL,TRUE);
	for (a=0;a<psTb->iElement;a++)
	{
		//ShowWindow(hButton[a],SW_SHOW);
		InvalidateRect(psTb->arhButton[a],NULL,TRUE);
	}

	GetWindowRect(psTb->hWnd,&Rect);  
	psTb->WinLx=Rect.right-Rect.left;
	psTb->WinLy=Rect.bottom-Rect.top;
}


// ------------------------------------------
// Seleziona uno dei tasti ed alza l'altro
// ------------------------------------------
static void LKeySelect(S_WTOOLBAR * psTb,INT wID)
{
  INT a;
  CHAR *Grp;

  if (psTb->arsObj[wID].iType==O_IRADIB||psTb->arsObj[wID].iType==O_TBCOLORRADIO)
  {
   Grp=psTb->arsObj[wID].lpGrp;
   for (a=0;a<psTb->iElement;a++)
   {
      if (a==wID) 
	  {
		  psTb->arsObj[a].fChecked=TRUE;
		  InvalidateRect(psTb->arhButton[a],NULL,TRUE);
	  }
	   else
	  {
		if ((psTb->arsObj[a].fChecked!=FALSE)&&(!strcmp(psTb->arsObj[a].lpGrp,Grp)))
		{
		 psTb->arsObj[a].fChecked=FALSE;
		 InvalidateRect(psTb->arhButton[a],NULL,TRUE);
		}

	  }
   }
  }
}


static void GiveToolRect(S_WTOOLBAR * psTb,INT x,INT y,LPRECT lprc)
{			
  INT a;
  INT SizePlus=GetSystemMetrics(SM_CXFRAME)*2;
  for (;;) {if ((x*y)<psTb->iElement) y++; else break;}

  a=y*psTb->KeyLy+(psTb->KeySy<<1); lprc->bottom=lprc->top+a+SizePlus;
  a=x*psTb->KeyLx+(psTb->KeySx<<1); lprc->right=lprc->left+a+SizePlus;
  // Aggiungo le presine
  if (x>y) lprc->right+=_s.iPres; else lprc->bottom+=_s.iPres;
}


static INT ToolFind(S_WTOOLBAR * psTb,CHAR *Name)
{
 INT a;
 for (a=0;a<psTb->iElement;a++)
 {
	 if (!strcmp(strupr(Name),strupr(psTb->arsObj[a].lpName))) return a;
 }
 return -1;
}

/*
static S_WTOOLBAR * GiveMeTb(HWND hWnd)
{
 INT a;
 for (a=0;a<10;a++)
  {
    if (_s.arsToolBat[a].hWnd==hWnd) return _s.arsToolBat[a].Tb;  
  }
  //ehExit("ToolBar: Hwnd ?");
  ehExit("ToolBar: Hwnd ?");
  return NULL;
}
*/
static INT _toolBarGetId(S_WTOOLBAR * psTb,HWND hWndButton)
{
  INT a;
  for (a=0;a<psTb->iElement;a++)
  {
    if (psTb->arhButton[a]==hWndButton) return a;
  }
  return -1;
}

static void _objEvent(S_WTOOLBAR * psTb,INT wID)
{
	CHAR szNome[30];
	strcpy(szNome,psTb->arsObj[wID].lpName);
	if (psTb->arsObj[wID].iType==O_ICONEB) strcat(szNome,"OFF");
	if (psTb->arsObj[wID].iType==O_IRADIB) strcat(szNome,"ON");
	obj_addevent(szNome);
}



static LRESULT CALLBACK _funcToolProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
//	static BOOL bCapture=false;
//	static POINT Dif;
	RECT Rect; 
//	POINT Point;
    INT Child=0;
	
	LPDRAWITEMSTRUCT DIs;
    // S_WINSCENA WScena;
	CHAR *lpIcone;
//	INT OldCol;
    INT wID;	
	INT a,x,y;
	PAINTSTRUCT ps;
	LPRECT lprc;
	INT ColBg;
	S_WTOOLBAR * psTb;
	HDC hDC;
    TOOLINFO ti;
    INT SizePlus=GetSystemMetrics(SM_CXFRAME)*2;
	INT ilx,ily,slx,sly;
	CHAR Buf[10];
	INT idx;
	LPCREATESTRUCT psCs;
	WINDOWPLACEMENT  sWp;
	
	psTb=(S_WTOOLBAR *) GetWindowLong(hWnd,GWL_USERDATA);

	switch (message)
	{
		case WM_CREATE: 

            //if (_s.iToolNow==-1) ehExit("ToolBar Full");

			psCs=(LPCREATESTRUCT) lParam;
			psTb=psCs->lpCreateParams;
			SetWindowLong(hWnd,GWL_USERDATA,(LONG) psTb);

			psTb->hWnd=hWnd;
			psTb->hWndTT= CreateWindowEx(0, TOOLTIPS_CLASS, (LPSTR) NULL, 
							           TTS_ALWAYSTIP, 
							           CW_USEDEFAULT, CW_USEDEFAULT, 
							           CW_USEDEFAULT, CW_USEDEFAULT, 
							           psTb->hWnd, (HMENU) NULL, 
							           sys.EhWinInstance, NULL); 

			// Creo la ToolTip
			//InitCommonControls();  
			for (a=0;a<psTb->iElement;a++)
			{
				psTb->arhButton[a]=CreateWindow("Button","",
					WS_CHILD|BS_OWNERDRAW|WS_VISIBLE|
					//BS_FLAT|
					BS_PUSHLIKE|BS_NOTIFY|
					BS_VCENTER,						        
					0,0,10,10,
					hWnd,
					//GetForegroundWindow(),
					(HMENU) a,
					sys.EhWinInstance,
					NULL);

				// Inserisco i Nuovi ToolTip
				ti.cbSize = sizeof(TOOLINFO);             
				ti.uFlags = TTF_IDISHWND|TTF_SUBCLASS; 
				ti.hwnd   = hWnd;             
				ti.hinst  = 0;//sys.EhWinInstance; 
				ti.uId    = (UINT) psTb->arhButton[a];             
				ti.lpszText    = psTb->arsObj[a].lpHmz; 
				SendMessage(psTb->hWndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

				if (!psTb->arsObj[a].bEnable) EnableWindow(psTb->arhButton[a],FALSE); 
			}
			break;

		case WM_DESTROY: 
//			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			
			if (!psTb) break;
			for (a=0;a<psTb->iElement;a++) DestroyWindow(psTb->arhButton[a]);
            DestroyWindow(psTb->hWndTT);
			break;

		case WM_MOVE: break;
		
		case WM_SIZE: 
			// if ((Tb=GiveMeTb(hWnd))==NULL) break;
			GetWindowRect(psTb->hWnd,&Rect);  
            psTb->WinLx=Rect.right-Rect.left;
            psTb->WinLy=Rect.bottom-Rect.top;

			GetClientRect(psTb->hWnd,&Rect);  
            psTb->ClientLx=Rect.right-Rect.left;
            psTb->ClientLy=Rect.bottom-Rect.top;

			_keysMove(psTb); //return 0;
			break;
        
		case WM_EXITSIZEMOVE: 
//			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			SetFocus(psTb->hWndOwner); 
			break;

		case WM_SIZING:
			//fwSide = wParam;       
            lprc = (LPRECT) lParam;
//			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			//dispx(" ->> %d      ",GetSystemMetrics(SM_CYEDGE));
			switch (wParam)
			{
				 case WMSZ_TOP: 
				 case WMSZ_BOTTOM: 
				 case WMSZ_BOTTOMLEFT: 
				 case WMSZ_BOTTOMRIGHT: 
								   a=lprc->bottom-lprc->top-psTb->KeySy-SizePlus; 
								   y=(a/psTb->KeyLy); if (y<1) y=1;
								   x=(psTb->iElement/y); if (x<1) x=1;
								   for (;;)
								   {
									   if ((x*y)<psTb->iElement) x++; else break;
								   }
	                               
								   if ((x*y)>(psTb->iElement-x))  {y=((psTb->iElement+x-1)/x);}

								   break;
				 case WMSZ_LEFT: 
				 case WMSZ_RIGHT: 
				 default:
								   a=lprc->right-lprc->left-(psTb->KeySx<<1)-SizePlus; 
								   x=(a/psTb->KeyLx); if (x<1) x=1;
								   y=(psTb->iElement/x); if (y<1) y=1;
								   for (;;)
								   {
									   if ((x*y)<psTb->iElement) y++; else break;
								   }
							   break;
			}


			if ((y*x)>psTb->iElement)
			{
			  if (x==1) y=psTb->iElement; 
              if (y==1) x=psTb->iElement;
			}

            GiveToolRect(psTb,x,y,lprc);
            return TRUE;

		case WM_MEASUREITEM:
            //MIs=(LPMEASUREITEMSTRUCT) lParam;
			//MIs->itemWidth=24;
			//MIs->itemHeight=24;
			return TRUE;

		case WM_DRAWITEM: 

			// DRAWITEMSTRUCT
			{
				RECT rc;
				RECTD rcd;
				HDC dc;
				DIs=(LPDRAWITEMSTRUCT) lParam;
				dc=DIs->hDC;

	//			dispx("%d- %d",DIs->CtlID,DIs->itemState&ODS_CHECKED);

				// WinDirectDC(DIs->hDC,&WScena,NULL);
				
//				OldCol=ModeColor(TRUE);
				idx=DIs->CtlID%psTb->iElement;
				lpIcone=psTb->arsObj[idx].lpIcone;
			
				switch(psTb->arsObj[idx].iType)
				{
					case O_TBCOLORRADIO:
						
						rectCopy(rc,DIs->rcItem); rc.right--; rc.bottom--;
						dcBoxp(dc,&rc,sys.ColorBackGround);
					//boxp(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,sys.ColorBackGround,SET);
						if (strlen(lpIcone)==6)
						{
							Buf[2]=0;
							memcpy(Buf,lpIcone,2);   ColBg=xtoi(Buf);
							memcpy(Buf,lpIcone+2,2); ColBg+=(xtoi(Buf)<<8);
							memcpy(Buf,lpIcone+4,2); ColBg+=(xtoi(Buf)<<16);
						}

						rc.left+=2; rc.top+=2; rc.right--; rc.bottom--;
						dcBoxp(dc,&rc,ColBg);

						//boxp(DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3,ColBg,SET); 

					if (*lpIcone=='*')
					{
						dcBoxp(dc,rectFill(&rc,DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3),sys.arsColor[15]);
					  //boxp(DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3,sys.arsColor[15],SET); 
						dcLine(dc,DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3,0);
						dcLine(dc,DIs->rcItem.right-3,DIs->rcItem.top+2,DIs->rcItem.left+2,DIs->rcItem.bottom-3,0);
					}

					if ((DIs->itemState&ODS_SELECTED)||psTb->arsObj[DIs->CtlID].fChecked)
					{
						//box(DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3,15,XOR);
						dcBoxp(dc,rectFill(&rc,DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3),sys.arsColor[15]);
						//box3d(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,1);
						//box3d(DIs->rcItem.left+1,DIs->rcItem.top+1,DIs->rcItem.right-2,DIs->rcItem.bottom-2,4);
					}
					else
					{
					 //box3d(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,0);
					}
					break;
				
				 default:
                

					if (DIs->itemState&ODS_SELECTED) ColBg=ColorLum(sys.ColorBackGround,-4); else ColBg=sys.ColorBackGround;
					if (psTb->arsObj[DIs->CtlID].fChecked)  
					{   INT a,b;
						INT Len=DIs->rcItem.right-DIs->rcItem.left+1;
						for (a=0,b=0;a<Len;a++,b+=4)
						{
						// X:100:A:(len-1)  

						 //boxp(DIs->rcItem.left+a,DIs->rcItem.top,DIs->rcItem.left+a,DIs->rcItem.bottom,ColorFusion(RGB(0,80,200),RGB(0,200,255),a*100/(Len-1)),SET);	
							dcBoxp(dc,rectFill(&rc,DIs->rcItem.left+a,DIs->rcItem.top,DIs->rcItem.left+a,DIs->rcItem.bottom),ColorFusion(RGB(0,80,200),RGB(0,200,255),a*100/(Len-1)));
						}
						/*
						ColBg=ColorLum(sys.ColorBackGround,-20);
						boxp(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.left,DIs->rcItem.bottom,ColBg,SET);	
						for (a=0,b=0;a<7;a++,b+=16)
						{
						 box(DIs->rcItem.left+a,DIs->rcItem.top+a,DIs->rcItem.right-a,DIs->rcItem.bottom-a,
							  ColorFusion(ColorLum(sys.ColorBackGround,70),ColBg,b),SET);	
						}
						*/

					} 
					else
					//boxp(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,ColBg,SET);
						dcBoxp(dc,rectFill(&rc,DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1),ColBg);
	            
					if (ico_info(&ilx,&ily,lpIcone)<0) break; //{ilx=1;ily=1;}
					slx=(psTb->KeyLx-ilx)>>1; sly=(psTb->KeyLy-ily)>>1;
					
					if ((DIs->itemState&ODS_SELECTED)||psTb->arsObj[idx].fChecked)
					{
						//ico_disp(DIs->rcItem.left+1+slx,DIs->rcItem.top+1+sly,lpIcone);
						dcIcone(dc,DIs->rcItem.left+1+slx,DIs->rcItem.top+1+sly,lpIcone);
					//	box3d(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,1);
					//	box3d(DIs->rcItem.left+1,DIs->rcItem.top+1,DIs->rcItem.right-2,DIs->rcItem.bottom-2,4);
					}
					else
					{
						S_ICONE_EFFECT sEfx;
						sEfx.enType=IEFX_GRAYSCALE;

						if (DIs->itemState&ODS_DISABLED) //ico_disp3D(DIs->rcItem.left+slx,DIs->rcItem.top+sly,sys.ColorBackGround,lpIcone);
														 //dcIconeEx(dc,DIs->rcItem.left+slx,DIs->rcItem.top+sly,lpIcone,&sEfx);
														 dcIconeGray(dc,DIs->rcItem.left+slx,DIs->rcItem.top+sly,lpIcone,sys.Color3DShadow,0);
														 else
														 //ico_disp(DIs->rcItem.left+slx,DIs->rcItem.top+sly,lpIcone);
														 dcIcone(dc,DIs->rcItem.left+slx,DIs->rcItem.top+sly,lpIcone);
						dcBox(dc,rectFillD(&rcd,DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1),rgba(0,0,0,255),1.0);
						//box3d(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,0);
					}
					
					break;
				}
	//AVANTISSIMO:
//				ModeColor(OldCol);
			//WinDirectDC(NULL,&WScena,NULL);
			}
			break;

		case WM_LBUTTONDOWN:

			psTb->bCapture=true;
			sWp.length=sizeof(sWp);
			GetWindowPlacement(hWnd,&sWp);
			//GetWindowRect(hWnd,&psTb->rcShot); // Posizione attuale
			rectCopy(psTb->rcShot,sWp.rcNormalPosition);
			GetCursorPos(&psTb->ptStart);
			SetCapture(hWnd);
//			Dif.x=Point.x-Rect.left;
//			Dif.y=Point.y-Rect.top;
			//return TRUE;
			break;
		
		case WM_LBUTTONUP:
			if (psTb->bCapture) ReleaseCapture();
			psTb->bCapture=false;
			break;
			//return TRUE;
			//break;
	
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_ACTIVATEAPP:
			break;

		case WM_MOUSEMOVE: 
			if (psTb->bCapture)
			{
				POINT ptNow;
				POINT ptDelta;
				GetCursorPos(&ptNow);
				ptDelta.x=ptNow.x-psTb->ptStart.x;
				ptDelta.y=ptNow.y-psTb->ptStart.y;
//				printf("%d,%d" CRLF,ptDelta.x,ptDelta.y);
				MoveWindow(hWnd,psTb->rcShot.left+ptDelta.x,psTb->rcShot.top+ptDelta.y,psTb->WinLx,psTb->WinLy,TRUE);
			}
			break;

        case WM_NCHITTEST: 
//			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			break;

		// Messaggio di cambio cursore
		//case WM_SETCURSOR: break;
		// Intercettazione degli oggetti Windows
		//case WM_COMMAND: break;
		case WM_COMMAND: 
			wID = LOWORD(wParam);
			if (HIWORD(wParam)==BN_CLICKED)
			{
//				 if ((Tb=GiveMeTb(hWnd))==NULL) break;
				 LKeySelect(psTb,wID);
				 _objEvent(psTb,wID);
				 SetFocus(psTb->hWndOwner); 
			}
			
			break;

		case WM_PAINT: 

			hDC=BeginPaint(hWnd,&ps); 
/*
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			// WinDirectDC(hDC,&WScena,NULL);
			if (psTb->ClientLy>psTb->ClientLx)
			{
			 box3d(0,0,psTb->ClientLx,1,0);
			 box3d(0,2,psTb->ClientLx,3,0);
			 box3d(0,psTb->ClientLy-2,psTb->ClientLx,psTb->ClientLy-1,0);
			 box3d(0,psTb->ClientLy-4,psTb->ClientLx,psTb->ClientLy-3,0);
			}
			 else
			{
			 box3d(0,0,1,psTb->ClientLy,0);
			 box3d(2,0,3,psTb->ClientLy,0);
			 box3d(psTb->ClientLx-2,0,psTb->ClientLx-1,psTb->ClientLy-1,0);
			 box3d(psTb->ClientLx-4,0,psTb->ClientLx-3,psTb->ClientLy-1,0);
			 //box3d(2,0,3,psTb->ClientLy,0);
			}
			WinDirectDC(NULL,&WScena,NULL);
			*/
			EndPaint(hWnd,&ps);
			break;
		//case WM_SETFOCUS: IsFocus=TRUE; break;
		//case WM_KILLFOCUS: IsFocus=FALSE; break;


	}  // switch message

 return(DefWindowProc(hWnd, message, wParam, lParam));
} 

// ---------------------------------------------------
// Seleziona un tasto senza emettere il messaggio
// ---------------------------------------------------
void ToolSelect(S_WTOOLBAR *Tb,CHAR *Name)
{
  INT Id;
  Id=ToolFind(Tb,Name); if (Id<0) return;
  LKeySelect(Tb,Id);
}

// ---------------------------------------------------
// Seleziona un tasto ed emette il messaggio       
// ---------------------------------------------------
void ToolClick(S_WTOOLBAR *Tb,CHAR *Name)
{
  INT Id;
  Id=ToolFind(Tb,Name); if (Id<0) return;
  LKeySelect(Tb,Id);
//  obj_addevent(psTb->arsObj[Id].lpName);
  _objEvent(Tb,Id);
}

// ---------------------------------------------------
// Blocca l'uso di un oggetto
// ---------------------------------------------------
void ToolLock(S_WTOOLBAR * psTb,CHAR *Name)
{
  INT Id;
  Id=ToolFind(psTb,Name); if (Id<0) return;
  if (!psTb->arsObj[Id].bEnable) return;
  psTb->arsObj[Id].bEnable=FALSE;
  EnableWindow(psTb->arhButton[Id],FALSE); 
}

// ---------------------------------------------------
// Sblocca l'uso di un oggetto
// ---------------------------------------------------
void ToolUnlock(S_WTOOLBAR * psTb,CHAR *Name)
{
  INT Id;
  Id=ToolFind(psTb,Name); if (Id<0) return;
  if (psTb->arsObj[Id].bEnable) return;
  psTb->arsObj[Id].bEnable=TRUE;
  EnableWindow(psTb->arhButton[Id],TRUE); 
}

//
// ToolMove()
//
void ToolMove(S_WTOOLBAR * psTb,UINT Mode,S_WTOOLBAR *TbRef)
{
//	RECT CliRect;
//	WINDOWINFO sWi;
//	WINDOWPLACEMENT Wp;

	INT x=0,y=0;
	RECT rcCli;

	GetClientRect(psTb->hWndOwner,&rcCli);

	// In riferimento all'area client
	if (Mode&WTB_CLIENT)
	{

		if (Mode&WTB_TOP)    y=rcCli.top;//CliRect.top;
		if (Mode&WTB_BOTTOM) y=rcCli.bottom-psTb->WinLy+1;
		if (Mode&WTB_LEFT)   x=rcCli.left;
		if (Mode&WTB_RIGHT)  x=rcCli.right-psTb->WinLx+1;
		MoveWindow(psTb->hWnd,x,y,psTb->WinLx,psTb->WinLy,TRUE);
		return;
	}

	// In riferimento ad un'altra tool bar
	if (Mode&WTB_TOOL)
	{
		RECT rcTool;
		WINDOWPLACEMENT sWp;
		sWp.length=sizeof(sWp);
		GetWindowPlacement(TbRef->hWnd,&sWp);
		rectCopy(rcTool,sWp.rcNormalPosition);

		// GetWindowRect(TbRef->hWnd,&rcTool);
		if (Mode&WTB_TOP)    
		{	y=rcTool.top;
			if (Mode&WTB_OVERY) y-=psTb->WinLy;
		}
		if (Mode&WTB_BOTTOM) 
		{	y=rcTool.bottom-psTb->WinLy+1;
			if (Mode&WTB_OVERY) y=rcTool.bottom;
		}

		if (Mode&WTB_LEFT)   
		{	x=rcTool.left; if (Mode&WTB_OVERX) x-=psTb->WinLy;
		}

		if (Mode&WTB_RIGHT)  
		{	x=rcTool.right-psTb->WinLx+1;
			if (Mode&WTB_OVERX) x=rcTool.right;
		}
		MoveWindow(psTb->hWnd,x,y,psTb->WinLx,psTb->WinLy,TRUE);
		return;
	}
}

//
// ToolGetInfo()
//
void ToolGetInfo(S_WTOOLBAR * psTb,WTOOLINFO *Ti)
{
 RECT Rect;
 GetWindowRect(psTb->hWnd,&Rect);

 Ti->x=Rect.left;
 Ti->y=Rect.top;
 Ti->Lx=psTb->WinLx;
 Ti->Ly=psTb->WinLy;
}

//
// ToolSet()
//
void ToolSet(S_WTOOLBAR * psTb,WTOOLINFO *Ti)
{
	MoveWindow(psTb->hWnd,Ti->x,Ti->y,Ti->Lx,Ti->Ly,TRUE);
}


