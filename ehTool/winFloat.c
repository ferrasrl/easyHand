//   ---------------------------------------------
//   | winFloat  Utilità per il di finestre in un WorkSpace
//   |                                           
//   |
//   ---------------------------------------------
//
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/winFloat.h"

static struct {

	HWND	hwndClient;
	_DMI	dmiWF;
	SINT	pixResize;

	BOOL	bCapture;
	HWND	wndBarCapture;
	RECT	rcBarStart;
	RECT	rcBarLast;
	POINT	pntStart;
	SINT	idxResize;
	EN_POS_TYPE enType;
	SINT	iDelta;

} sWFInfo;


static void wfRegisterClass(void);
static void wfOrganizer(SINT,HWND wndParent);

void wfBarBuilder(HWND hwndAlign,SINT idxElement,EN_POS_TYPE enType) {

	EH_WIN_FLOAT sWF,sWFEle;
	BYTE *pszClass;
	DWORD dwStyle=WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	DMIRead(&sWFInfo.dmiWF,idxElement,&sWFEle);

	ZeroFill(sWF);
	switch (enType)
	{
		case WFP_ANCHOR_LEFT:
		case WFP_ANCHOR_RIGHT:
			sWF.sizWin.cx=sWFInfo.pixResize;
			pszClass=WC_FLOAT_RESIZE_WE;
			break;

		case WFP_ANCHOR_TOP:
		case WFP_ANCHOR_BOTTOM:
			sWF.sizWin.cy=sWFInfo.pixResize;
			pszClass=WC_FLOAT_RESIZE_NS;
			break;
	}

	sWF.hWnd=CreateWindowEx(0,
							  pszClass,
							  "",
							  dwStyle,//WS_CLIPCHILDREN|WS_CHILD|WS_VISIBLE,
							  0,0,
							  10,//IMG.bmiHeader.biWidth+2,
							  10,//IMG.bmiHeader.biHeight+2,
							  sWFInfo.hwndClient,
							  NULL,//GetDesktopWindow(),
							  sys.EhWinInstance,
							  NULL);
	sWF.bResizeBar=TRUE;
	sWF.enType=enType;
	sWF.wndRef=hwndAlign;
	sWF.wndResize=sWFEle.hWnd;
	winFloat(WS_INSERT,idxElement+1,&sWF);
}

static SINT wfParentSearch(HWND hwnd)
{
	SINT a;
	EH_WIN_FLOAT sWF;
	if (!hwnd) return -1;
	for (a=0;a<sWFInfo.dmiWF.Num;a++) {
		DMIRead(&sWFInfo.dmiWF,a,&sWF);
		if (sWF.hWnd==hwnd)
		{
			return a; 
		}
	}				
	return -1;
}


//
// winFloat()
//
void * winFloat(EN_MESSAGE enMess,SINT iData,void *ptr) {
	
	SINT a;
	EH_WIN_FLOAT *psWF=(EH_WIN_FLOAT *) ptr;
	EH_WIN_FLOAT sWF;//,sWFParent;
	void *pRet=NULL;

	switch (enMess) {
		
			case WS_OPEN:
				wfRegisterClass();
				ZeroFill(sWFInfo);
				DMIReset(&sWFInfo.dmiWF);
				sWFInfo.hwndClient=(HWND) ptr;
				sWFInfo.pixResize=4;
				DMIOpen(&sWFInfo.dmiWF,RAM_AUTO,20,sizeof(EH_WIN_FLOAT),"winFloat");
				break;

			case WS_CLOSE:
				DMIClose(&sWFInfo.dmiWF,"winFloat");
				break;

			case WS_ADD:
				DMIAppendDyn(&sWFInfo.dmiWF,psWF);
				break;

			case WS_INSERT:
				DMIInsertDyn(&sWFInfo.dmiWF,iData,psWF);
				break;

			case WS_PROCESS:

				//
				// Cancello le finestre di ridimensionamento
				//
				for (a=0;a<sWFInfo.dmiWF.Num;a++) {
					BOOL bSet=FALSE;
					DMIRead(&sWFInfo.dmiWF,a,&sWF);
					if (sWF.bResizeBar) 
					{
						DestroyWindow(sWF.hWnd);
						DMIDelete(&sWFInfo.dmiWF,a,NULL); a--;
					}
				}

				wfOrganizer(0,NULL);
				break;


			case WS_DISPLAY:

				for (a=0;a<sWFInfo.dmiWF.Num;a++) {
					BOOL bSet=FALSE;
					DMIRead(&sWFInfo.dmiWF,a,&sWF);
					if (sWF.bHidden)
					{
						//winChangeParam(sWF.hWnd,GWL_STYLE,WS_VISIBLE,FALSE);
						ShowWindow(sWF.hWnd,SW_HIDE);
					}
					else
					{
						SetWindowPos(	sWF.hWnd, 
										(sWF.bResizeBar?HWND_BOTTOM:NULL),
										sWF.rcWin.left,sWF.rcWin.top,
										sWF.sizWin.cx,sWF.sizWin.cy,
										(sWF.bResizeBar?0:SWP_NOZORDER));
						//winChangeParam(sWF.hWnd,GWL_STYLE,WS_VISIBLE,TRUE);
						ShowWindow(sWF.hWnd,SW_NORMAL);

					}
				}
				break;

			case WS_FIND:
				
				for (a=0;a<sWFInfo.dmiWF.Num;a++) {
				//	BOOL bSet=FALSE;
					DMIRead(&sWFInfo.dmiWF,a,&sWF);
					if (!iData) 
					{
						if (!strCmp(sWF.pszName,psWF->pszName)) 
						{
							memcpy(psWF,&sWF,sizeof(EH_WIN_FLOAT));
							pRet=(SINT *) a;
							break;
						}
					}
					else if (sWF.hWnd==(HWND) iData) 
					{
						memcpy(psWF,&sWF,sizeof(EH_WIN_FLOAT));
						pRet=(SINT *) a;
						break;
					}
				}
				break;


	}
	return pRet;
}

/*
void winChangeParam(HWND hWnd,INT iIndex,DWORD dwParam,BOOL bEnable)
{
	DWORD dwData;
	dwData=GetWindowLong(hWnd,iIndex);
	if (bEnable) dwData|=dwParam; else dwData&=~dwParam;
	SetWindowLong(hWnd,iIndex,dwData);
}
*/

void wfBarDraw(RECT *psRect) {
	
	HDC hDC;
	HBRUSH hbr,hbrOld;

	hDC=GetDC(NULL);
	//dcBoxBrush(hDC,&sWFInfo.rcBarLast,HS_DIAGCROSS,RGB(255,255,255),0);
	//psRect=&sWFInfo.rcBarLast;
	
	rectAdjust(psRect);

	hbr = GetStockObject(GRAY_BRUSH);
	hbrOld= SelectObject(hDC, hbr);
	SetBkColor (hDC, 0) ; 
	
	//rop=SetROP2(hDC,R2_XORPEN);
	PatBlt(hDC,psRect->left,psRect->top,psRect->right-psRect->left+1,psRect->bottom-psRect->top+1,PATINVERT);

	SelectObject(hDC, hbrOld);

	ReleaseDC(NULL,hDC);
}

//
// funcResize()
//
LRESULT CALLBACK funcResize(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	EH_WIN_FLOAT sWF,sWFEle;
	POINT pntMouse;

	switch (message)
	{
		case WM_PAINT:
			hDC=BeginPaint(hWnd,&ps);
			dcBoxp(hDC,&ps.rcPaint,sys.ColorBackGround);//RGB(255,0,0));
			EndPaint(hWnd,&ps);
			return 0;

		//
		// Inizio spostamento
		//
		case WM_LBUTTONDOWN:
			winFloat(WS_FIND,(SINT) hWnd,&sWF); // <-- Trovo la barra
			sWFInfo.enType=sWF.enType;
			sWFInfo.idxResize=(SINT) winFloat(WS_FIND,(SINT) sWF.wndResize,&sWFEle); // <-- Trovo l'elemento da ridimensionare
			GetWindowRect(sWF.hWnd,&sWFInfo.rcBarStart);
			rectCopy(sWFInfo.rcBarLast,sWFInfo.rcBarStart);
			GetCursorPos(&sWFInfo.pntStart);
			sWFInfo.bCapture=TRUE;
			SetCapture(hWnd);
			wfBarDraw(&sWFInfo.rcBarLast);
//			dispx(" | %s | ",sWFEle.pszName);
			break;

		//
		// Fine spostamento
		//
		case WM_LBUTTONUP:
			wfBarDraw(&sWFInfo.rcBarLast);
			winFloat(WS_FIND,(SINT) hWnd,&sWF); // <-- Trovo la barra
			
			//InvalidateRect(NULL,&sWFInfo.rcBarLast,TRUE);
			sWFInfo.bCapture=FALSE;
			ReleaseCapture();

			// Ricalcolo dimensioni
			DMIRead(&sWFInfo.dmiWF,sWFInfo.idxResize,&sWFEle);
			switch (sWFInfo.enType)
			{
					case WFP_ANCHOR_LEFT: sWFEle.sizWin.cx+=sWFInfo.iDelta; break;
					case WFP_ANCHOR_RIGHT: sWFEle.sizWin.cx-=sWFInfo.iDelta; break;
					case WFP_ANCHOR_TOP: sWFEle.sizWin.cy+=sWFInfo.iDelta; break;
					case WFP_ANCHOR_BOTTOM: sWFEle.sizWin.cy-=sWFInfo.iDelta; break;
			}
			DMIWrite(&sWFInfo.dmiWF,sWFInfo.idxResize,&sWFEle);

			winFloat(WS_PROCESS,0,NULL);
			winFloat(WS_DISPLAY,0,NULL);
			break;

		//
		// MouseMove
		//
		case WM_MOUSEMOVE: 

			if (sWFInfo.bCapture)
			{
				RECT rcOld;
				DMIRead(&sWFInfo.dmiWF,sWFInfo.idxResize,&sWFEle);
				GetCursorPos(&pntMouse);

				rectCopy(rcOld,sWFInfo.rcBarLast);
				switch (sWFInfo.enType)
				{
					case WFP_ANCHOR_LEFT:
					case WFP_ANCHOR_RIGHT:
						sWFInfo.iDelta=(pntMouse.x-sWFInfo.pntStart.x);
						sWFInfo.rcBarLast.left=sWFInfo.rcBarStart.left+sWFInfo.iDelta;
						sWFInfo.rcBarLast.right=sWFInfo.rcBarStart.right+sWFInfo.iDelta;
						break;

					case WFP_ANCHOR_TOP:
					case WFP_ANCHOR_BOTTOM:
						sWFInfo.iDelta=(pntMouse.y-sWFInfo.pntStart.y);
						sWFInfo.rcBarLast.top=sWFInfo.rcBarStart.top+sWFInfo.iDelta;
						sWFInfo.rcBarLast.bottom=sWFInfo.rcBarStart.bottom+sWFInfo.iDelta;
//						dispx("^  | %s | %d,%d ",sWFEle.pszName,pntMouse.x-sWFInfo.pntStart.x,pntMouse.y);
						break;
				}
				wfBarDraw(&sWFInfo.rcBarLast);
				wfBarDraw(&rcOld);
			}
			break;

		case WM_DESTROY:
		   // SetFocus(WIN_info[0].hWnd);			
			break;

	}
	return(DefWindowProc(hWnd, message, wParam, lParam));
}

static void wfRegisterClass(void)
{
   WNDCLASSEX wc;

   ZeroFill(wc);
   wc.cbSize        = sizeof(wc);
   wc.style         = CS_BYTEALIGNCLIENT;

   // Funzione chiamata per la gestione della finestra
   wc.lpfnWndProc   = funcResize;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = sys.EhWinInstance;
   wc.hIcon         = NULL;//LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICOMARE));
   wc.hCursor       = LoadCursor(NULL,IDC_SIZEWE);
   wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);//COLOR_WINDOW;
   wc.lpszMenuName  = NULL;//szAppName;
   wc.lpszClassName = WC_FLOAT_RESIZE_WE;
   wc.hIconSm       = NULL;//LoadIcon(sys.EhWinInstance,MAKEINTRESOURCE(IDI_ICOMARE));
   RegisterClassEx(&wc);

   ZeroFill(wc);
   wc.cbSize        = sizeof(wc);
   wc.style         = CS_BYTEALIGNCLIENT;

   // Funzione chiamata per la gestione della finestra
   wc.lpfnWndProc   = funcResize;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = sys.EhWinInstance;
   wc.hIcon         = NULL;//LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICOMARE));
   wc.hCursor       = LoadCursor(NULL,IDC_SIZENS);
   wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);//COLOR_WINDOW;
   wc.lpszMenuName  = NULL;//szAppName;
   wc.lpszClassName = WC_FLOAT_RESIZE_NS;
   wc.hIconSm       = NULL;//LoadIcon(sys.EhWinInstance,MAKEINTRESOURCE(IDI_ICOMARE));
   RegisterClassEx(&wc);


}

static void wfOrganizer(SINT idxParent,HWND wndParent) {
	
	SIZE sizWS;
	RECT rcWS,rcFree;
	SINT a;
	EH_WIN_FLOAT sWF,sWFParent;

	// Leggo la dimensione del WorkSpace
	if (!wndParent) 
	{
		GetClientRect(sWFInfo.hwndClient,&rcWS);
		rcWS.bottom--;
	}
	else
	{
		//GetClientRect(wndParent,&rcWS);
		DMIRead(&sWFInfo.dmiWF,idxParent,&sWFParent);
		rectCopy(rcWS,sWFParent.rcWin);
	}
	rectCopy(rcFree,rcWS);
	sizeCalc(&sizWS,&rcWS);

	//
	// Organizza il WorkSpace (loop sugli elementi)
	//
	for (a=0;a<sWFInfo.dmiWF.Num;a++) {
		BOOL bSet=FALSE;
		DMIRead(&sWFInfo.dmiWF,a,&sWF);
		if (sWF.bHidden) continue;
		if (sWF.wndRef!=wndParent) continue;
		switch (sWF.enType) {

			case WFP_MAIN:
				rectCopy(sWF.rcWin,rcWS);
				//sWFInfo.idxMain=a;
				break;

			case WFP_FLOAT: break;

			//
			// WFP_ANCHOR_LEFT - Ancorata a sinistra
			//
			case WFP_ANCHOR_LEFT:
				sWF.rcWin.left=rcFree.left;
				sWF.rcWin.top=rcFree.top;
				sWF.rcWin.bottom=rcFree.bottom;
				sWF.rcWin.right=sWF.rcWin.left+sWF.sizWin.cx-1;
				rcFree.left=sWF.rcWin.right+1;
				break;

			//
			// WFP_ANCHOR_RIGHT - Allineamento a destra
			//
			case WFP_ANCHOR_RIGHT:
				sWF.rcWin.top=rcFree.top;
				sWF.rcWin.bottom=rcFree.bottom;
				sWF.rcWin.right=rcFree.right;
				sWF.rcWin.left=sWF.rcWin.right-sWF.sizWin.cx+1;
				rcFree.right=sWF.rcWin.left-1;
				break;

			//
			// Ancorata in ALTO
			//
			case WFP_ANCHOR_TOP:
				sWF.rcWin.left=rcFree.left;
				sWF.rcWin.top=rcFree.top;
				sWF.sizWin.cx=rcFree.right-sWF.rcWin.left+1;
				sWF.rcWin.right=sWF.rcWin.left+sWF.sizWin.cx-1;
				sWF.rcWin.bottom=sWF.rcWin.top+sWF.sizWin.cy-1;
				rcFree.top=sWF.rcWin.bottom;
				break;

			//
			// Ancorata in BASSO
			//
			case WFP_ANCHOR_BOTTOM:
				sWF.rcWin.left=rcFree.left;
				sWF.rcWin.right=rcFree.right;
				sWF.rcWin.bottom=rcFree.bottom;
				sWF.rcWin.top=sWF.rcWin.bottom-sWF.sizWin.cy+1;
				sizeCalc(&sWF.sizWin,&sWF.rcWin);
				rcFree.bottom=sWF.rcWin.top-1;
				break;
		}
		sizeCalc(&sWF.sizWin,&sWF.rcWin);
		DMIWrite(&sWFInfo.dmiWF,a,&sWF);
		
		if (!sWF.bResizeBar)
		{
			if (sWF.bResizeManual) wfBarBuilder(wndParent,a,sWF.enType);
			wfOrganizer(a,sWF.hWnd);
		}
	}
	
	if (wndParent)
	{
		DMIRead(&sWFInfo.dmiWF,idxParent,&sWFParent);
		rectCopy(sWFParent.rcWin,rcFree);
		sizeCalc(&sWFParent.sizWin,&sWFParent.rcWin);
		DMIWrite(&sWFInfo.dmiWF,idxParent,&sWFParent);
	
	}
}

