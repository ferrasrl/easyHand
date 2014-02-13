// ----------------------------------------------------
//  Splash
//  Gestione SplashScreen
//  
// ----------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/imgutil.h"
#include "/easyhand/ehtool/Splash.h"

static HWND hSplashWnd=NULL;
static SINT HdlImage=-1;
#define EH_SPLASHSCREEN "EHSplashScreen"
static void SplashClass(void);

void SplashScreen(CHAR *lpFile,SINT iTime)
{
    IMGHEADER IMG;
	SINT x,y;
	RECT rRect;

	// ------------------------------------
	// Attivazione dello Splash Screen
	//
	if (lpFile)
	{
		BOOL fCheck;

		SplashClass();
		if (!JPGReadFile(lpFile,&HdlImage,NULL,NULL,JME_HIDE,IMG_PIXEL_BGR)) 
		{
			efx2();
			return;
		}

		memoRead(HdlImage,0,&IMG,sizeof(IMG));

		x=(sys.sizeDesktop.cx>>1)-(IMG.bmiHeader.biWidth>>1);
		y=(sys.sizeDesktop.cy>>1)-(IMG.bmiHeader.biHeight>>1);

		rRect.left=0; rRect.right=IMG.bmiHeader.biWidth;
		rRect.top=0; rRect.bottom=IMG.bmiHeader.biHeight;

	//	win_infoarg("Prima: %d,%d-%d,%d",rRect.left,rRect.top,rRect.right,rRect.bottom);
		fCheck=AdjustWindowRectEx(&rRect,WS_POPUP|WS_BORDER|WS_VISIBLE,0,WS_EX_TOOLWINDOW);
	//	win_infoarg("Dopo: %d,%d-%d,%d | %d:%d",rRect.left,rRect.top,rRect.right,rRect.bottom,fCheck,GetLastError());

		// Creo la Window
		hSplashWnd=CreateWindowEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
								  EH_SPLASHSCREEN,
								  NULL,
//							      WS_OVERLAPPEDWINDOW|WS_SYSMENU|WS_VISIBLE,
//								  WS_VISIBLE|WS_BORDER,
								  WS_POPUP|WS_BORDER|WS_VISIBLE,
								  x,y,
								  rRect.right,//IMG.bmiHeader.biWidth+2,
								  rRect.bottom,//IMG.bmiHeader.biHeight+2,
								  NULL,
								  NULL,//GetDesktopWindow(),
								  sys.EhWinInstance,
								  NULL);
		if (hSplashWnd==NULL) win_infoarg("errore");
		ShowWindow(hSplashWnd,SW_SHOW);
		UpdateWindow(hSplashWnd);
		PauseActive(iTime);
	}
	else
	// ------------------------------------
	// Disattivazione dello Splash Screen
	{
		PauseActive(iTime);
		DestroyWindow(hSplashWnd);
		if (HdlImage!=-1) memoFree(HdlImage,"CLIP");
		HdlImage=-1;
	}
}

LRESULT CALLBACK SplashProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static void SplashClass(void)
{
// Registro	
   WNDCLASSEX wc;
   wc.cbSize        = sizeof(wc);
   wc.style         = CS_HREDRAW | CS_VREDRAW;

   // Funzione chiamata per la gestione della finestra
   wc.lpfnWndProc   = SplashProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = sys.EhWinInstance;
   wc.hIcon         = NULL;//LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICOMARE));
   wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
   wc.hbrBackground = NULL;//(HBRUSH) GetStockObject(WHITE_BRUSH);//COLOR_WINDOW;
   wc.lpszMenuName  = NULL;//szAppName;
   wc.lpszClassName = EH_SPLASHSCREEN;
   wc.hIconSm       = NULL;//LoadIcon(sys.EhWinInstance,MAKEINTRESOURCE(IDI_ICOMARE));
   //if (!RegisterClassEx(&wc)) ehExit("Splash class");
   RegisterClassEx(&wc);
}


LRESULT CALLBACK SplashProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hDC;
 // WINSCENA WScena;

  switch (message)
  {
		case WM_PAINT:
			hDC=BeginPaint(hWnd,&ps);
			//WinDirectDC(ps.hdc,&WScena);
		    //IMGDispEx(0,0,0,0,0,0,HdlImage);
			dcImageShow(hDC,0,0,0,0,HdlImage);

			//WinDirectDC(NULL,&WScena);
			EndPaint(hWnd,&ps);
			return 0;

		case WM_DESTROY:
		    SetFocus(WIN_info[0].hWnd);			
			break;

		case WM_LBUTTONDOWN:
			ShowWindow(hSplashWnd,SW_HIDE);
			break;

  }
 return(DefWindowProc(hWnd, message, wParam, lParam));
}
