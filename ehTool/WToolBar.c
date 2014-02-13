//   ---------------------------------------------
//	 | WTOOLBAR                                  |
//	 | Toolbar Mobile Windows                    |
//	 |                             by Ferrà 1999 |
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/WToolBar.h"

LRESULT CALLBACK ToolProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static void GiveToolRect(WTOOLBAR *Tb,SINT x,SINT y,LPRECT lprc);
static void LMoveKeys(WTOOLBAR *Tb);
static void LKeySelect(WTOOLBAR *Tb,SINT wID);

typedef struct {
  HWND hWnd;
  WTOOLBAR *Tb;
} TOOLBARLIST;

static BOOL Init=FALSE;
static TOOLBARLIST TbList[10];
static SINT ToolNow=-1;
static SINT pPres=12;


WTOOLBAR *ToolCreate(WTOOLBAROBJ *ObjBar,SINT KeyLx,SINT KeyLy,SINT KeyNumx)
{
    CHAR *ClassName="Crea6ToolBar";
    WNDCLASSEX wc;
	SINT Hdl;
	WTOOLBAR *Tb;
	SINT a,c;
	RECT Rect;

	if (!Init)
	{
	 wc.cbSize        = sizeof(wc);
	 wc.style         = 0;//CS_DBLCLKS;

	 // Funzione chiamata per la gestione della finestra
	 wc.lpfnWndProc   = ToolProcedure;
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

     for (a=0;a<10;a++) TbList[a].hWnd=NULL;
	 Init=TRUE;
	}
    

	// Creo memoria per struttura
	// Conto gli oggetti
	for (c=0;;c++) {if (ObjBar[c].iType==STOP) break;}

	//
    // Trovo il primo posto disponibile
	//
    ToolNow=-1;
    for (a=0;a<10;a++) {if (TbList[a].hWnd==NULL) {ToolNow=a; break;}}
    if (ToolNow==-1) ehExit("ToolBar Full");

	Hdl=memoAlloc(RAM_AUTO,sizeof(WTOOLBAR)+(c*sizeof(HWND)),"WToolBar");
    Tb=memoLock(Hdl);
    Tb->hMemo=Hdl;
	Tb->KeyLx=KeyLx;
	Tb->KeyLy=KeyLy;
	Tb->KeySx=2;
	Tb->KeySy=2;
    Tb->iElement=c; // Numero di bottoni
	Tb->hWndOwner=WindowNow();//WIN_info[sys.WinInputFocus].hWnd;
	Tb->hButton=(HWND *) ((TCHAR *) (Tb)+sizeof(WTOOLBAR)); // Metto gli handle in fondo dopo la struttura
	//Tb->hButtonTT=(HWND *) ((TCHAR *) (Tb->hButton)+(sizeof(HWND)*Tb->Num));
	Tb->ObjBar=ObjBar;
	TbList[ToolNow].Tb=Tb;

	Tb->hWnd=CreateWindowEx(WS_EX_NOPARENTNOTIFY|WS_EX_TOOLWINDOW,
						   ClassName, 
			               "",
						   //WS_POPUP|WS_BORDER|WS_VISIBLE|WS_THICKFRAME
						   WS_CLIPCHILDREN|WS_POPUP|WS_THICKFRAME|WS_BORDER,
//						   WS_CLIPCHILDREN|WS_OVERLAPPED|WS_THICKFRAME,
						   0, 90,
						   100,100,
						   WIN_info[sys.WinInputFocus].hWnd, 
						   0,
						   sys.EhWinInstance, 
						   NULL);
	//Rect.top=//GetSystemMetrics(SM_CYEDGE)+ // Bordo 3D
    //          GetSystemMetrics(SM_CYSIZE)+GetSystemMetrics(SM_CYMENU);
	Rect.top=0; Rect.left=0;
	GiveToolRect(Tb,KeyNumx,(Tb->iElement/KeyNumx),&Rect);
	MoveWindow(Tb->hWnd,Rect.left,Rect.top,Rect.right-Rect.left,Rect.bottom-Rect.top,TRUE);

	LMoveKeys(Tb);
	//LKeySelect(Tb,0);
	//EnableWindow(Tb->hWnd,FALSE);
	ShowWindow(Tb->hWnd,SW_SHOW);
	return Tb;
}

void ToolDestroy(WTOOLBAR *Tb)
{
  SINT a;
  DestroyWindow(Tb->hWnd); 
  for (a=0;a<10;a++)
  {
	if (TbList[a].hWnd==Tb->hWnd) {TbList[a].hWnd=NULL; break;}
  }
  memoFree(Tb->hMemo,"Crea");
}



// ---------------------------------------------------------------
// Riposiziona i tasti in base alle dimenzioni dell'area client
// ---------------------------------------------------------------

static void LMoveKeys(WTOOLBAR *Tb)
{
 SINT a,x,y;
 SINT WLx;
 RECT Rect;
 SINT adx,ady;

 GetClientRect(Tb->hWnd,&Rect);  WLx=Rect.right-Rect.left+1;
 x=Tb->KeySx;y=Tb->KeySy;

 if (Tb->ClientLx>Tb->ClientLy) {adx=pPres/2;ady=0;} else {adx=0;ady=pPres/2;}
 for (a=0;a<Tb->iElement;a++)
 {
  // Muovo il Tasto
  MoveWindow(Tb->hButton[a],x+adx,y+ady,Tb->KeyLx,Tb->KeyLy,FALSE);
  x=x+Tb->KeyLx; if ((x+Tb->KeyLx+Tb->KeySx)>WLx) {x=Tb->KeySx; y=y+Tb->KeyLy;}
 }
 
 InvalidateRect(Tb->hWnd,NULL,TRUE);
 for (a=0;a<Tb->iElement;a++)
 {
  //ShowWindow(hButton[a],SW_SHOW);
  InvalidateRect(Tb->hButton[a],NULL,TRUE);
 }
 
 GetWindowRect(Tb->hWnd,&Rect);  
 Tb->WinLx=Rect.right-Rect.left;
 Tb->WinLy=Rect.bottom-Rect.top;
}


// ------------------------------------------
// Seleziona uno dei tasti ed alza l'altro
// ------------------------------------------
static void LKeySelect(WTOOLBAR *Tb,SINT wID)
{
  SINT a;
  CHAR *Grp;

  if (Tb->ObjBar[wID].iType==O_IRADIB||Tb->ObjBar[wID].iType==O_TBCOLORRADIO)
  {
   Grp=Tb->ObjBar[wID].lpGrp;
   for (a=0;a<Tb->iElement;a++)
   {
      if (a==wID) 
	  {
		  Tb->ObjBar[a].fChecked=TRUE;
		  InvalidateRect(Tb->hButton[a],NULL,TRUE);
	  }
	   else
	  {
		//SendMessage(hButton[wID],BM_SETCHECK,BST_UNCHECKED,0);
		if ((Tb->ObjBar[a].fChecked!=FALSE)&&(!strcmp(Tb->ObjBar[a].lpGrp,Grp)))
		{
		 Tb->ObjBar[a].fChecked=FALSE;
         //SendMessage(hButton[wID],BM_SETSTATE,FALSE,0);
		 InvalidateRect(Tb->hButton[a],NULL,TRUE);
		}

		//SendMessage(hButton[wID],BM_SETSTATE,FALSE,0);
	  }
   }
  }
}


static void GiveToolRect(WTOOLBAR *Tb,SINT x,SINT y,LPRECT lprc)
{			
  SINT a;
  SINT SizePlus=GetSystemMetrics(SM_CXFRAME)*2;
  for (;;) {if ((x*y)<Tb->iElement) y++; else break;}

  a=y*Tb->KeyLy+(Tb->KeySy<<1); lprc->bottom=lprc->top+a+SizePlus;
  a=x*Tb->KeyLx+(Tb->KeySx<<1); lprc->right=lprc->left+a+SizePlus;
  // Aggiungo le presine
  if (x>y) lprc->right+=pPres; else lprc->bottom+=pPres;
}


static SINT ToolFind(WTOOLBAR *Tb,CHAR *Name)
{
 SINT a;
 for (a=0;a<Tb->iElement;a++)
 {
	 if (!strcmp(strupr(Name),strupr(Tb->ObjBar[a].lpName))) return a;
 }
 return -1;
}


static WTOOLBAR *GiveMeTb(HWND hWnd)
{
 SINT a;
 for (a=0;a<10;a++)
  {
    if (TbList[a].hWnd==hWnd) return TbList[a].Tb;  
  }
  //ehExit("ToolBar: Hwnd ?");
  ehExit("ToolBar: Hwnd ?");
  return NULL;
}

static SINT GiveMeId(WTOOLBAR *Tb,HWND hWndButton)
{
  SINT a;
  for (a=0;a<Tb->iElement;a++)
  {
    if (Tb->hButton[a]==hWndButton) return a;
  }
  return -1;
}

static LObjEvent(WTOOLBAR *Tb,SINT wID)
{
 CHAR szNome[30];
 strcpy(szNome,Tb->ObjBar[wID].lpName);
 if (Tb->ObjBar[wID].iType==O_ICONEB) strcat(szNome,"OFF");
 if (Tb->ObjBar[wID].iType==O_IRADIB) strcat(szNome,"ON");
 obj_addevent(szNome);
}


static LRESULT CALLBACK ToolProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	static BOOL Capture=FALSE;
	RECT Rect; 
	POINT Point;
	static POINT Dif;
    SINT Child=0;
	LPDRAWITEMSTRUCT DIs;
    S_WINSCENA WScena;
	CHAR *lpIcone;
	SINT OldCol;
    SINT wID;	
	SINT a,x,y;
	PAINTSTRUCT ps;
	LPRECT lprc;
	SINT ColBg;
	WTOOLBAR *Tb;
	HDC hDC;
    TOOLINFO ti;
    SINT SizePlus=GetSystemMetrics(SM_CXFRAME)*2;
	SINT ilx,ily,slx,sly;
	CHAR Buf[10];
	SINT idx;

	switch (message)
	{
		case WM_CREATE: 

            if (ToolNow==-1) ehExit("ToolBar Full");
			Tb=TbList[ToolNow].Tb;
			TbList[ToolNow].hWnd=hWnd;
			Tb->hWnd=hWnd;
			Tb->hWndTT= CreateWindowEx(0, TOOLTIPS_CLASS, (LPSTR) NULL, 
							           TTS_ALWAYSTIP, 
							           CW_USEDEFAULT, CW_USEDEFAULT, 
							           CW_USEDEFAULT, CW_USEDEFAULT, 
							           Tb->hWnd, (HMENU) NULL, 
							           sys.EhWinInstance, NULL); 

			// Creo la ToolTip
			//InitCommonControls();  
			for (a=0;a<Tb->iElement;a++)
			{
			  Tb->hButton[a]=CreateWindow("Button","",
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
			 ti.uId    = (UINT) Tb->hButton[a];             
			 ti.lpszText    = Tb->ObjBar[a].lpHmz; 
			 SendMessage(Tb->hWndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
			 
			 if (!Tb->ObjBar[a].bEnable) EnableWindow(Tb->hButton[a],FALSE); 
			}
			break;

		case WM_DESTROY: 
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			for (a=0;a<Tb->iElement;a++) DestroyWindow(Tb->hButton[a]);
            DestroyWindow(Tb->hWndTT);
			break;

		case WM_MOVE: break;
		
		case WM_SIZE: 
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			GetWindowRect(Tb->hWnd,&Rect);  
            Tb->WinLx=Rect.right-Rect.left;
            Tb->WinLy=Rect.bottom-Rect.top;

			GetClientRect(Tb->hWnd,&Rect);  
            Tb->ClientLx=Rect.right-Rect.left;
            Tb->ClientLy=Rect.bottom-Rect.top;

			LMoveKeys(Tb); //return 0;
			break;
        
		case WM_EXITSIZEMOVE: 
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			SetFocus(Tb->hWndOwner); 
			break;

		case WM_SIZING:
			//fwSide = wParam;       
            lprc = (LPRECT) lParam;
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			//dispx(" ->> %d      ",GetSystemMetrics(SM_CYEDGE));
			switch (wParam)
			{
			 case WMSZ_TOP: 
			 case WMSZ_BOTTOM: 
			 case WMSZ_BOTTOMLEFT: 
			 case WMSZ_BOTTOMRIGHT: 
							   a=lprc->bottom-lprc->top-Tb->KeySy-SizePlus; 
				               y=(a/Tb->KeyLy); if (y<1) y=1;
							   x=(Tb->iElement/y); if (x<1) x=1;
							   for (;;)
							   {
								   if ((x*y)<Tb->iElement) x++; else break;
							   }
                               
							   if ((x*y)>(Tb->iElement-x))  {y=((Tb->iElement+x-1)/x);}

							   break;
			 case WMSZ_LEFT: 
			 case WMSZ_RIGHT: 
			 default:
							   a=lprc->right-lprc->left-(Tb->KeySx<<1)-SizePlus; 
							   x=(a/Tb->KeyLx); if (x<1) x=1;
				               y=(Tb->iElement/x); if (y<1) y=1;
							   for (;;)
							   {
								   if ((x*y)<Tb->iElement) y++; else break;
							   }
							   break;
			}


			if ((y*x)>Tb->iElement)
			{
			  if (x==1) y=Tb->iElement; 
              if (y==1) x=Tb->iElement;
			}

            GiveToolRect(Tb,x,y,lprc);
            return TRUE;

		case WM_MEASUREITEM:
            //MIs=(LPMEASUREITEMSTRUCT) lParam;
			//MIs->itemWidth=24;
			//MIs->itemHeight=24;
			return TRUE;

		case WM_DRAWITEM: 

			// DRAWITEMSTRUCT
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			DIs=(LPDRAWITEMSTRUCT) lParam;
//			dispx("%d- %d",DIs->CtlID,DIs->itemState&ODS_CHECKED);

			WinDirectDC(DIs->hDC,&WScena,NULL);
			
			OldCol=ModeColor(TRUE);
			idx=DIs->CtlID%Tb->iElement;
            lpIcone=Tb->ObjBar[idx].lpIcone;
		
			switch(Tb->ObjBar[idx].iType)
			{
			 case O_TBCOLORRADIO:
                boxp(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,sys.ColorBackGround,SET);
				if (strlen(lpIcone)==6)
				{
					Buf[2]=0;
					memcpy(Buf,lpIcone,2);   ColBg=xtoi(Buf);
					memcpy(Buf,lpIcone+2,2); ColBg+=(xtoi(Buf)<<8);
					memcpy(Buf,lpIcone+4,2); ColBg+=(xtoi(Buf)<<16);
				}
				boxp(DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3,ColBg,SET); 

				if (*lpIcone=='*')
				{
                  boxp(DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3,sys.arsColor[15],SET); 
				  line(DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3,0,SET);
				  line(DIs->rcItem.right-3,DIs->rcItem.top+2,DIs->rcItem.left+2,DIs->rcItem.bottom-3,0,SET);
				}

				if ((DIs->itemState&ODS_SELECTED)||Tb->ObjBar[DIs->CtlID].fChecked)
				{
					box(DIs->rcItem.left+2,DIs->rcItem.top+2,DIs->rcItem.right-3,DIs->rcItem.bottom-3,15,XOR); 
					box3d(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,1);
					box3d(DIs->rcItem.left+1,DIs->rcItem.top+1,DIs->rcItem.right-2,DIs->rcItem.bottom-2,4);
				}
				else
				{
				 box3d(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,0);
				}
				break;
			
			 default:
                

				if (DIs->itemState&ODS_SELECTED) ColBg=ColorLum(sys.ColorBackGround,-4); else ColBg=sys.ColorBackGround;
//				if (Tb->ObjBar[DIs->CtlID].fChecked)  ColBg=ColorLumRGB(sys.ColorBackGround,+20,+30,+40);
				if (Tb->ObjBar[DIs->CtlID].fChecked)  
				{   SINT a,b;
					SINT Len=DIs->rcItem.right-DIs->rcItem.left+1;
				//	ColBg=ColorFusion(sys.ColorBackGround,RGB(0,255,255),70);
					for (a=0,b=0;a<Len;a++,b+=4)
					{
					// X:100:A:(len-1)  

					 boxp(DIs->rcItem.left+a,DIs->rcItem.top,DIs->rcItem.left+a,DIs->rcItem.bottom,ColorFusion(RGB(0,80,200),RGB(0,200,255),a*100/(Len-1)),SET);	
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
				boxp(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,ColBg,SET);
            
				if (ico_info(&ilx,&ily,lpIcone)<0) break; //{ilx=1;ily=1;}
				slx=(Tb->KeyLx-ilx)>>1; sly=(Tb->KeyLy-ily)>>1;
				
				if ((DIs->itemState&ODS_SELECTED)||Tb->ObjBar[idx].fChecked)
				{
					ico_disp(DIs->rcItem.left+1+slx,DIs->rcItem.top+1+sly,lpIcone);
					box3d(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,1);
					box3d(DIs->rcItem.left+1,DIs->rcItem.top+1,DIs->rcItem.right-2,DIs->rcItem.bottom-2,4);
				}
				else
				{
					if (DIs->itemState&ODS_DISABLED) ico_disp3D(DIs->rcItem.left+slx,DIs->rcItem.top+sly,sys.ColorBackGround,lpIcone);
													 else
													 ico_disp(DIs->rcItem.left+slx,DIs->rcItem.top+sly,lpIcone);
					box3d(DIs->rcItem.left,DIs->rcItem.top,DIs->rcItem.right-1,DIs->rcItem.bottom-1,0);
				}
				break;
			}
//AVANTISSIMO:
			ModeColor(OldCol);
			WinDirectDC(NULL,&WScena,NULL);
			break;

		case WM_LBUTTONDOWN:

			Capture=TRUE; SetCapture(hWnd);
			 GetWindowRect(hWnd,&Rect);
			 GetCursorPos(&Point);
             Dif.x=Point.x-Rect.left;
             Dif.y=Point.y-Rect.top;
			 return TRUE;
			 //break;
		
		case WM_LBUTTONUP:
			if (Capture) {ReleaseCapture();}
			Capture=FALSE;
			return TRUE;
			//break;
	
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_LBUTTONDBLCLK:

		case WM_ACTIVATEAPP:
		case WM_MOUSEMOVE: 
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			if (Capture)
			{
			 GetCursorPos(&Point);
			 MoveWindow(hWnd,Point.x-Dif.x,Point.y-Dif.y,Tb->WinLx,Tb->WinLy,TRUE);
			}
			break;

        case WM_NCHITTEST: 
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			//if (Capture) {ReleaseCapture();}
			//Capture=FALSE;
			//SetFocus(Tb->hWndOwner); 
			break;

		// Messaggio di cambio cursore
		//case WM_SETCURSOR: break;
		// Intercettazione degli oggetti Windows
		//case WM_COMMAND: break;
		case WM_COMMAND: 
			wID = LOWORD(wParam);
			if (HIWORD(wParam)==BN_CLICKED)
			{
			 if ((Tb=GiveMeTb(hWnd))==NULL) break;
			 //dispx("%d [%s]        ",wID,ObjBar[wID].nome);
			 LKeySelect(Tb,wID);
			 LObjEvent(Tb,wID);
			 SetFocus(Tb->hWndOwner); 
			}
			
			break;

		case WM_PAINT: 

			hDC=BeginPaint(hWnd,&ps); 
			if ((Tb=GiveMeTb(hWnd))==NULL) break;
			WinDirectDC(hDC,&WScena,NULL);
			if (Tb->ClientLy>Tb->ClientLx)
			{
			 box3d(0,0,Tb->ClientLx,1,0);
			 box3d(0,2,Tb->ClientLx,3,0);
			 box3d(0,Tb->ClientLy-2,Tb->ClientLx,Tb->ClientLy-1,0);
			 box3d(0,Tb->ClientLy-4,Tb->ClientLx,Tb->ClientLy-3,0);
			}
			 else
			{
			 box3d(0,0,1,Tb->ClientLy,0);
			 box3d(2,0,3,Tb->ClientLy,0);
			 box3d(Tb->ClientLx-2,0,Tb->ClientLx-1,Tb->ClientLy-1,0);
			 box3d(Tb->ClientLx-4,0,Tb->ClientLx-3,Tb->ClientLy-1,0);
			 //box3d(2,0,3,Tb->ClientLy,0);
			}
			WinDirectDC(NULL,&WScena,NULL);
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
void ToolSelect(WTOOLBAR *Tb,CHAR *Name)
{
  SINT Id;
  Id=ToolFind(Tb,Name); if (Id<0) return;
  LKeySelect(Tb,Id);
}

// ---------------------------------------------------
// Seleziona un tasto ed emette il messaggio       
// ---------------------------------------------------
void ToolClick(WTOOLBAR *Tb,CHAR *Name)
{
  SINT Id;
  Id=ToolFind(Tb,Name); if (Id<0) return;
  LKeySelect(Tb,Id);
//  obj_addevent(Tb->ObjBar[Id].lpName);
  LObjEvent(Tb,Id);
}

// ---------------------------------------------------
// Blocca l'uso di un oggetto
// ---------------------------------------------------
void ToolLock(WTOOLBAR *Tb,CHAR *Name)
{
  SINT Id;
  Id=ToolFind(Tb,Name); if (Id<0) return;
  if (!Tb->ObjBar[Id].bEnable) return;
  Tb->ObjBar[Id].bEnable=FALSE;
  EnableWindow(Tb->hButton[Id],FALSE); 
}

// ---------------------------------------------------
// Sblocca l'uso di un oggetto
// ---------------------------------------------------
void ToolUnlock(WTOOLBAR *Tb,CHAR *Name)
{
  SINT Id;
  Id=ToolFind(Tb,Name); if (Id<0) return;
  if (Tb->ObjBar[Id].bEnable) return;
  Tb->ObjBar[Id].bEnable=TRUE;
  EnableWindow(Tb->hButton[Id],TRUE); 
}

void ToolMove(WTOOLBAR *Tb,UINT Mode,WTOOLBAR *TbRef)
{
	RECT CliRect;
	WINDOWINFO sWi;
	WINDOWPLACEMENT Wp;

	SINT x,y;
//	RECT rcWin;
 
 // In riferimento all'area client
 if (Mode&WTB_CLIENT)
 {
   // Calcolo la posizione assoluta dell'area client
	Wp.length=sizeof(Wp);
	GetWindowInfo(Tb->hWndOwner,&sWi);
	/*
	//GetWindowPlacement(Tb->hWndOwner,&Wp);
	//GetClientRect(Tb->hWndOwner,&CliRect);
	//GetWindowRect(Tb->hWndOwner,&rcWin);

   if (Wp.showCmd==SW_SHOWMAXIMIZED)
   {
     CliRect.top+=GetSystemMetrics(SM_CYSIZE);
   }
   else
   {
    CliRect.top=Wp.rcNormalPosition.top;
    CliRect.left=Wp.rcNormalPosition.left;
    CliRect.top+=GetSystemMetrics(SM_CYFRAME)+GetSystemMetrics(SM_CYSIZE);
    CliRect.left+=GetSystemMetrics(SM_CXFRAME);
   }

   // Controllo se c'è un menu
   if (GetMenu(Tb->hWndOwner)!=NULL) CliRect.top+=GetSystemMetrics(SM_CYMENU);
   CliRect.right+=CliRect.left;
   CliRect.bottom+=CliRect.top;
*/
	if (Mode&WTB_TOP)    y=sWi.rcClient.top;//CliRect.top;
	if (Mode&WTB_BOTTOM) y=sWi.rcClient.bottom-Tb->WinLy+1;
	if (Mode&WTB_LEFT)   x=sWi.rcClient.left;
	if (Mode&WTB_RIGHT)  x=sWi.rcClient.right-Tb->WinLx+1;
	MoveWindow(Tb->hWnd,x,y,Tb->WinLx,Tb->WinLy,TRUE);
	return;
 }

 // In riferimento ad un'altra tool bar
 if (Mode&WTB_TOOL)
 {
   GetWindowRect(TbRef->hWnd,&CliRect);
   if (Mode&WTB_TOP)    
   {y=CliRect.top;
    if (Mode&WTB_OVERY) y-=Tb->WinLy;
   }
   if (Mode&WTB_BOTTOM) 
   {y=CliRect.bottom-Tb->WinLy+1;
    if (Mode&WTB_OVERY) y=CliRect.bottom;
   }

   if (Mode&WTB_LEFT)   
   {x=CliRect.left; if (Mode&WTB_OVERX) x-=Tb->WinLy;
   }

   if (Mode&WTB_RIGHT)  
   {x=CliRect.right-Tb->WinLx+1;
    if (Mode&WTB_OVERX) x=CliRect.right;
   }
   MoveWindow(Tb->hWnd,x,y,Tb->WinLx,Tb->WinLy,TRUE);
   return;
 }
}

void ToolGetInfo(WTOOLBAR *Tb,WTOOLINFO *Ti)
{
 RECT Rect;
 GetWindowRect(Tb->hWnd,&Rect);

 Ti->x=Rect.left;
 Ti->y=Rect.top;
 Ti->Lx=Tb->WinLx;
 Ti->Ly=Tb->WinLy;
}

void ToolSet(WTOOLBAR *Tb,WTOOLINFO *Ti)
{
	MoveWindow(Tb->hWnd,Ti->x,Ti->y,Ti->Lx,Ti->Ly,TRUE);
}


