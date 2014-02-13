//   ---------------------------------------------
//   ³ ZCMPIperButton
//   ³                                           
//   ³         by Ferrà Art & Technology 1993-2004
//   ---------------------------------------------

#include "\ehtool\include\ehsw_idb.h"

#include "\ehtool\dmiutil.h"
#include "\ehtool\sdbdrive.h"
#include "\ehtool\superfnd.h"
#include "\ehtool\ARMaker.h"
#include "\ehtool\Tool98.h"
#include "\ehtool\imgutil.h"
#include <math.h>
#include <time.h>

#include "ZCMP_IperButton.h"


static EH_IBLIST IBList[IB_MAX];

#define IB_FINDOBJ 0
#define IB_FINDHWND 1

static HWND CreateIperButton(HINSTANCE hInstance, HWND hwndParent);

static SINT IBFind(SINT iCosa,void *ptr)
{
	SINT a;
	for (a=0;a<IB_MAX;a++)
	{
		switch (iCosa)
		{
			case IB_FINDOBJ:  if (IBList[a].lpObj==(struct OBJ *) ptr) return a;
							  break;
			case IB_FINDHWND: if (IBList[a].hWndList==(HWND) ptr) return a;
							  break;
		}
	}
	return -1;
}

static SINT IBAlloc(struct OBJ *obj)
{
	SINT a;
	CHAR *p,*p2;
	for (a=0;a<IB_MAX;a++)
	{
	  if (IBList[a].lpObj==NULL)
	  {
		IBList[a].lpObj=obj;
		strcpy(IBList[a].szIconeNormal,obj->text);

		p=strstr(IBList[a].szIconeNormal,"|"); if (p) *p=0;
		strcpy(IBList[a].szIconeOver,obj->text);
		strcpy(IBList[a].szIconePress,obj->text);
		if (p)
		{
			*p=0; p2=strstr(p+1,"|"); if (p2) *p2=0;
			strcpy(IBList[a].szIconeOver,p+1);
			if (p2) strcpy(IBList[a].szIconePress,p2+1);
		}

		IBList[a].iStatus=IB_NORMAL;
		IBList[a].iLastStatus=IB_NORMAL;
		IBList[a].fButton=FALSE;
		return a;
	  }
	}
	PRG_end("AI: overload");
	return 0;
}

void *EhIperButton(SINT cmd,LONG info,void *ptr)
{
 struct WS_DISPEXT *DExt=ptr;
 static SINT HdbMovi=-1;
 static INT16 iSend;
 SINT iAIIndex;
 
 if (OBJ_CallSub==NULL) PRG_end("EDZ: NULL %d",cmd);

 // Cerco una cella in LVLIST
 iAIIndex=IBFind(IB_FINDOBJ,OBJ_CallSub);
 if (iAIIndex<0) iAIIndex=IBAlloc(OBJ_CallSub);
 switch(cmd)
	{
		case WS_INF: return NULL;
 
		case WS_OPEN: // Creazione
			OBJ_CallSub->hWnd=CreateIperButton(sys.EhWinInstance,WindowNow());
			IBList[iAIIndex].hWndList=OBJ_CallSub->hWnd;
			//lpObjDropA=OBJ_CallSub;
			break;

		case WS_CLOSE: // Distruzione
			DestroyWindow(OBJ_CallSub->hWnd);
			break;

		case WS_EXTFUNZ: break;

		case WS_SETFLAG: break;

		case WS_DEL: // Cancella le colonne
    		break;

		case WS_ADD: // Aggiungi una colonna
			break;

		case WS_LINK: // Aggiunge un Item
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

LRESULT CALLBACK EHAIProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	HWND hWndMouse;

	WINSCENA Scena;
    HRGN hrgn;
	SINT iIndex;
	struct OBJ *poj;
	static SINT iCounter=0;
	static UINT uiTimer=0;
	POINT pCursor;
//	static BOOL fLastStatus=TRUE;
//	static BOOL fStatus=FALSE;
	SINT lx,ly;
	CHAR szServ[200];

    iIndex=IBFind(IB_FINDHWND,hWnd);
	poj=IBList[iIndex].lpObj;

	switch (message)
	{

		case WM_MOUSEMOVE: 
		  WinMouse((INT16) (poj->px+LOWORD(lParam)),
				   (INT16) (poj->py+HIWORD(lParam)),
				   0xFFFF);//(WORD) wParam); // Ho toccato questo
		  break;

		case WM_LBUTTONDOWN:  
			IBList[iIndex].fButton=TRUE;
			sprintf(szServ,"%sON",poj->nome);
			obj_VStack(szServ);
			break;

		case WM_LBUTTONUP:  
			IBList[iIndex].fButton=FALSE;
			sprintf(szServ,"%sOFF",poj->nome);
			obj_VStack(szServ);
			break;

		case WM_CREATE: 
			uiTimer=SetTimer(hWnd,10,50,NULL);
			break;

		case WM_TIMER:
			GetCursorPos(&pCursor);
			hWndMouse=WindowFromPoint(pCursor);
			if (hWndMouse!=hWnd) 
			{
				IBList[iIndex].iStatus=IB_NORMAL;
			//	if (IBList[iIndex].iStatus!=IBList[iIndex].iLastStatus) InvalidateRect(hWnd,NULL,TRUE);
			//	IBList[iIndex].iLastStatus=IBList[iIndex].iStatus;
			//	break; 
			}
			else
			{
				if (IBList[iIndex].fButton) IBList[iIndex].iStatus=IB_PRESS; else IBList[iIndex].iStatus=IB_OVER;
			}

			if (IBList[iIndex].iStatus!=IBList[iIndex].iLastStatus) InvalidateRect(hWnd,NULL,TRUE);
			IBList[iIndex].iLastStatus=IBList[iIndex].iStatus;
			break;

		case WM_SIZE: break;
		case WM_EXITSIZEMOVE: break;

		// Messaggio di cambio cursore
		case WM_SETCURSOR: break;
		case WM_DESTROY: 
			KillTimer(hWnd,uiTimer);
			break;

		// Intercettazione degli oggetti Windows
		case WM_COMMAND: break;
		case WM_PAINT: 
            
			//if (fDragging) ImageList_DragShowNolock(FALSE);
			//if (LockLayOut) break;
			hdc=BeginPaint(hWnd,&ps);
		    //TEDITRangeAdjust(TE);

			WinDirectDC(hdc,&Scena);
             // Da ottimizzare
			//win_infoarg("%d",TE->ncam);
            
			hrgn=CreateRectRgnIndirect(&ps.rcPaint);
            SelectClipRgn(hdc, hrgn);
            
			switch (IBList[iIndex].iStatus)
			{
				case IB_NORMAL: //Tboxp(ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom,sys.ColorBackGround,SET);
								ico_info(&lx,&ly,IBList[iIndex].szIconeNormal);
								ico_disp((poj->col1-lx)>>1,(poj->col2-ly)>>1,IBList[iIndex].szIconeNormal);
								break;

				case IB_OVER:  //Tboxp(ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom,sys.ColorBackGround,SET);
								ico_info(&lx,&ly,IBList[iIndex].szIconeOver);
								ico_disp((poj->col1-lx)>>1,(poj->col2-ly)>>1,IBList[iIndex].szIconeOver);
								break;

				case IB_PRESS:  //Tboxp(ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom,sys.ColorBackGround,SET);
								ico_info(&lx,&ly,IBList[iIndex].szIconePress);
								ico_disp((poj->col1-lx)>>1,(poj->col2-ly)>>1,IBList[iIndex].szIconePress);
								break;
			}
            DeleteObject(hrgn);
            SelectClipRgn(hdc, NULL);
			WinDirectDC(0,&Scena);
			EndPaint(hWnd,&ps);
			//if (fDragging) ImageList_DragShowNolock(TRUE);

			return 0;
		}  // switch message
  return(DefWindowProc(hWnd, message, wParam, lParam));
} 

static HWND CreateIperButton(HINSTANCE hInstance, HWND hwndParent)
{
	DWORD       dwStyle;
	HWND        hwndDrop;
	BOOL        bSuccess = TRUE;
    WNDCLASSEX wc;

	dwStyle =   WS_TABSTOP | 
				WS_CHILD | 
				//WS_BORDER | 
				WS_VISIBLE;

	 // Registro la classe delle OW_SCR
	 wc.cbSize        = sizeof(wc);
	 wc.style         = CS_NOCLOSE;
	 wc.lpfnWndProc   = EHAIProcedure;
	 wc.cbClsExtra    = 0;
	 wc.cbWndExtra    = 0;
	 wc.hInstance     = hInstance;
	 wc.hIcon         = NULL;
	 wc.hCursor       = NULL;
	 wc.hbrBackground = NULL;
	 wc.lpszMenuName  = NULL;
	 wc.lpszClassName = "EH_AI";
	 wc.hIconSm       = NULL;//LoadIcon(NULL,IDI_APPLICATION);
	 RegisterClassEx(&wc);

	 hwndDrop= CreateWindowEx(0//WS_EX_ACCEPTFILES
							  //|WS_EX_CLIENTEDGE
							  ,                             // no extended styles 
							  "EH_AI",                // class name 
							  "",            // title (caption) 
							  dwStyle,  // style 
							  0, 0,                        // position 
							  0, 0,                       // size 
							  hwndParent,                       // parent window 
							  (HMENU) NULL,             // control identifier 
							  hInstance,                       // instance 
							  NULL                           // no WM_CREATE parameter 
							  ); 
	if(!hwndDrop) return NULL;

	return hwndDrop;
}


