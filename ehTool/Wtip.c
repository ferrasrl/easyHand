//   ---------------------------------------------
//	 | WTIP                                      |
//	 | Tip di scritta a comparsa                 |
//	 |                             by Ferrà 1999 |
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/WToolBar.h"

static BOOL TipInit=FALSE;
static LRESULT CALLBACK TipProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static CHAR *TipBuffer=NULL;
static RECT  TipZone;

// Se x=-1; coordinate automatiche

HWND WTipOpen(SINT x,SINT y,CHAR *Mess,...)
{
   HWND hWnd;
   va_list Ah;
   WNDCLASSEX wc;
   SINT lx,ly;

   TipBuffer=ehAlloc(1000);
   va_start(Ah,Mess);
   vsprintf(TipBuffer,Mess,Ah); // Messaggio finale

   if (!TipInit)
	{
	 wc.cbSize        = sizeof(wc);
	 wc.style         = 0;//CS_DBLCLKS;

	 // Funzione chiamata per la gestione della finestra
	 wc.lpfnWndProc   = TipProcedure;
	 wc.cbClsExtra    = 0;
	 wc.cbWndExtra    = 0;
	 wc.hInstance     = sys.EhWinInstance;
	 wc.hIcon         = NULL;//LoadIcon(hInstance,MAKEINTRESOURCE(IDI_CREA1));
	 wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
	 wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
	 wc.lpszMenuName  = NULL;
	 wc.lpszClassName = "EhTipClass";
	 wc.hIconSm       = NULL; // Small Icone  (NULL = Automatico)
	 RegisterClassEx(&wc);
	 TipInit=TRUE;
	}

    lx=font_lenf(TipBuffer,"SMALL F",3,0);
    ly=font_altf("SMALL F",3,0	);
    lx+=4; ly+=4;

    TipZone.left=0;   TipZone.top=0;
    TipZone.right=lx; TipZone.bottom=ly;
    
    if (x==-1)
	{
		POINT Point;
		GetCursorPos(&Point);
		x=Point.x+20; y=Point.y-18;
		if (x+lx>=sys.video_x) x=sys.video_x-lx-1;
		if (y+ly>=sys.video_y) y=sys.video_y-ly-1;
	}
    
	
	hWnd=CreateWindowEx(0,
					   "EhTipClass", 
			           "",
					   //WS_POPUP|WS_BORDER|WS_VISIBLE|WS_THICKFRAME
					   WS_CLIPCHILDREN|WS_POPUP|WS_VISIBLE|
					   SS_BLACKRECT|SS_LEFT,
//						   WS_CLIPCHILDREN|WS_OVERLAPPED|WS_THICKFRAME,
					   x, y,
					   lx,ly,
					   WIN_info[sys.WinInputFocus].hWnd, 
					   0,
					   sys.EhWinInstance, 
					   NULL);

  // WinDirectDC(NULL,&WScena);
  // ReleaseDC(NULL,hDC);
   va_end(Ah);
   return hWnd;
}

static LRESULT CALLBACK TipProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	S_WINSCENA WScena;
	HDC hDC;
	PAINTSTRUCT ps;
	
	switch (message)
	{
		case WM_CREATE:  break;
		case WM_PAINT: 

			hDC=BeginPaint(hWnd,&ps); 
			WinDirectDC(hDC,&WScena,NULL);
			
            boxp(0,0,TipZone.right,TipZone.bottom,14,SET);
            box(0,0,TipZone.right-1,TipZone.bottom-1,1,SET);
			dispf(2,1,0,-1,ON,"SMALL F",3,TipBuffer);

			WinDirectDC(NULL,&WScena,NULL);
			EndPaint(hWnd,&ps);
			break;
	}  // switch message
 return(DefWindowProc(hWnd, message, wParam, lParam));
} 


void WTipClose(HWND hWnd)
{
   
////	InvalidateRect(NULL,Rect,FALSE);
//	InvalidateRect(sSetup.hwndEdit,Rect,FALSE);
     DestroyWindow(hWnd);
	 if (TipBuffer) ehFree(TipBuffer);
	 TipBuffer=NULL;
}
