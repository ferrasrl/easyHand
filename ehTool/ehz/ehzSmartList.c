//   ---------------------------------------------------------------
//   | ehSmartList
//   | SmartList
//   |                                              
//   | Gestisce una visione tipo "ListView" ma più "smart"
//   |                                              
//   |								by Ferrà Srl 2013
//   ---------------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehzSmartList.h"
#include "/easyhand/ehtool/EditFloat.h"


#define ID_SMARTLIST 12101
#define ID_SMARTLISTHEADER 12102

static struct {

	BOOL	bReady;

} _p = {0};

//#define FORM_ID_OFFSET 1000
 
static void		_initialize(void);
static BOOL		_colDestroy(S_SL_COL * psCol);
static void		_paramParser(S_SL_COL *psCol,CHAR *pszParam);
static void		_tableCalc(EHZ_SMARTLIST *psSl);
static void		_headerButton(EHZ_SMARTLIST *psSl);
//static void		_fontParser(CHAR * pszFont,EH_FONT ** ppsFont);

static LRESULT CALLBACK _funcWinSL(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static LRESULT CALLBACK _funcWinSLHeader(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static void		_this_Reset(void * this);
static BOOL		_this_addCol(void * this, INT idCode, CHAR * pszCode, CHAR * pszTitle, CHAR * pszStyle); 
static BOOL		_this_setItemCount(void * this, INT iItems); 
static BOOL		_this_setNotify(void * this,void * (*funcNotify)(void * this,EH_SRVPARAMS));
static BOOL		_this_refresh(void * this);
static BOOL		_this_refreshEx(void * this,INT iRow,INT iCol);
static BOOL		_this_ensureVisible(void * this,INT iRow,INT iCol,RECT * psRect);
static BOOL		_this_getRect(void * this,INT iRow,INT iCol,RECT * psRect);
static BOOL		_this_getCell(void * this,INT iRow,INT iCol,S_SL_CELL * psCell);
static BOOL		_this_getCellEvent(void * this,EH_EVENT * psEvent,S_SL_CELL * psCell);
static BOOL		_this_cellWalk(void * this,INT * piRow,INT * piCol, INT iKey);
static BOOL		_this_setFocusRow(void * this,INT iRow);
static BOOL		_this_setFocusCell(void * this,INT iRow,INT iCel);
static BOOL		_this_setVScroll(void * this,INT iSBMode,INT iOffset);
static BOOL		_this_setHeaderHeight(void * this,INT iHeight);
static BOOL		_this_setColVisibility(void * this,CHAR * pszCode,BOOL bVisible);
static BOOL		_this_setBodyStyle(void * this,CHAR * pszParam);
static BOOL		_this_clean(void * this,BOOL bWait);
static BOOL		_this_setWaiting(void * this,BOOL bWait,CHAR * pszWaiting);

//
// ehzSmartList()
//
void * ehzSmartList(EH_OBJPARAMS)
{
	EH_DISPEXT *psDex=pVoid;
	EHZ_SMARTLIST * psSl;
	EH_EVENT * psEvent;
	BOOL bAllocated;
	
	psSl=psObjCaller->pOther;
	if (!_p.bReady) _initialize();
	switch(enMess)
	{
		//
		// Creazione della finestra Form
		//
		case WS_CREATE:

			psObjCaller->hWnd=CreateWindowEx(	WS_EX_CONTROLPARENT,
												WC_EH_SMARTLIST,
												"",
												WS_TABSTOP | 
												WS_CHILD | 
	//											WS_BORDER |
												WS_CLIPSIBLINGS |
												WS_VISIBLE //|
												//WS_VSCROLL
												,
												0,                         // x position
												0,                         // y position
												0,                         // width
												0,                         // height
												WindowNow(),                // parent
												(HMENU) ID_SMARTLIST,       // ID
												sys.EhWinInstance,         // Instance
												NULL);     

			psObjCaller->pOther=ehAllocZero(sizeof(EHZ_SMARTLIST));
			psSl=psObjCaller->pOther;

			psSl->enEncode=SE_ANSI;
			psSl->psObj=psObjCaller;
//			psSl->idxFocus=-1;
			SetWindowLong(psObjCaller->hWnd,GWL_USERDATA,(LONG) psSl);

			psSl->wnd=psObjCaller->hWnd;
			DMIReset(&psSl->dmiCol);
			DMIOpen(&psSl->dmiCol,RAM_AUTO,100,sizeof(S_SL_COL),"slCols");
			psSl->arsCol=DMILock(&psSl->dmiCol,NULL);

			psSl->colBack=sys.ColorWindowBack;
			psSl->colText=sys.ColorWindowText;
			psSl->iFocusRow=-1;
			psSl->iFocusCol=-1;
			psSl->iBodyMouseOverRow=-1;
			psSl->iBodyMouseOverCol=-1;

			// Like C++ ;-)
			psSl->addCol=_this_addCol;
			psSl->setItemCount=_this_setItemCount;
			psSl->setNotify=_this_setNotify;
			psSl->refresh=_this_refresh;
			psSl->refreshEx=_this_refreshEx;
			psSl->ensureVisible=_this_ensureVisible;
			psSl->getRect=_this_getRect;
			psSl->getCell=_this_getCell;
			psSl->getCellEvent=_this_getCellEvent;
			psSl->cellWalk=_this_cellWalk;
			psSl->setFocusRow=_this_setFocusRow;
			psSl->setFocusCell=_this_setFocusCell;
			psSl->setVScroll=_this_setVScroll;
			psSl->setHeaderHeight=_this_setHeaderHeight;
			psSl->setColVisibility=_this_setColVisibility;
			psSl->setBodyStyle=_this_setBodyStyle;
			psSl->clean=_this_clean;
			psSl->setWaiting=_this_setWaiting;

/*
			psSl->Add=_this_Add;
			psSl->Show=_this_Show;
			psSl->SetOptions=_this_SetOptions;
			psSl->Focus=_this_Focus;
			psSl->Set=_this_Set;
			psSl->SetTitle=_this_SetTitle;

			*/

			psSl->psFontTitle=fontCreate("#Tahoma",13,STYLE_BOLD,true,&bAllocated,NULL);
			psSl->psFontText=fontCreate("#Arial",15,STYLE_NORMAL,true,&bAllocated,NULL);
			psSl->pszTextWaiting=strDup(ultTag("Attendere prego ..."));
			

//			SendMessageW(psSl->wnd,WM_SETFONT,(WPARAM) psSl->psFontTitle->hFont,MAKELPARAM(TRUE, 0));
			psSl->iCellPadding=4;
			psSl->bHeadToolBar=false;
			psSl->iHeadToolBarHeight=24;
			psSl->iHeadHeight=psSl->psFontText->iHeight+psSl->iCellPadding*2;
			psSl->iHeadHeightNormal=psSl->iHeadHeight;

//			psSl->iHeadHeightOpen=psSl->iHeadHeightNormal+psSl->iHeadToolBarHeight;
			psSl->colLine=sys.colScrollDiv; // RGB(220,220,220);
			psSl->pszCellBuffer=ehAlloc(0xffff);
			psSl->iArrowWidth=7;

			psSl->wndHeader=CreateWindowEx(		WS_EX_CONTROLPARENT,
												WC_EH_SMARTLIST_HEADER,
												"",
												WS_TABSTOP | 
												WS_CHILD | 
												WS_CLIPSIBLINGS 
												//WS_VISIBLE 
												,
												0,                         // x position
												0,                         // y position
												0,                         // width
												0,                         // height
												WindowNow(),                // parent
												(HMENU) ID_SMARTLISTHEADER,       // ID
												sys.EhWinInstance,         // Instance
												NULL);     
			SetWindowLong(psSl->wndHeader,GWL_USERDATA,(LONG) psSl);
			SetWindowPos(psSl->wndHeader,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);

			// Checkbox
			psSl->addCol(psSl, 0xF0000001, "#CHECK","", "width:30px; align:center; fix:true; visibility:hidden; shadow-right:16px; reserved:checkbox");


			break;
		

		case WS_DESTROY: 

			DestroyWindow(psSl->wndHeader);
			DestroyWindow(psSl->wnd);
			_this_Reset(psSl);
			if (psSl->funcNotify) psSl->funcNotify(psSl,WS_DESTROY,0,NULL);
			fontDestroy(psSl->psFontTitle,TRUE);
			fontDestroy(psSl->psFontText,TRUE);
			
			ehFreePtrs(4,&psSl->pszCellBuffer,&psSl->pszTextProcess,&psSl->pszTextWaiting,&psSl->arsRowsInfo);
			ehFreePtr(&psObjCaller->pOther); psSl=NULL; // -> cioè, psSl
			break;

		case WS_DO: 
//			MoveWindow(psObjCaller->hWnd,psDex->px,psDex->py,psDex->lx,psDex->ly,true);
			SetWindowPos(psSl->wndHeader,NULL,psDex->px,psDex->py,0,0,SWP_NOSIZE|SWP_NOZORDER);
			MoveWindow(psObjCaller->hWnd,psDex->px,psDex->py,psDex->lx,psDex->ly,true);
			_tableCalc(psSl);
			break;

		case WS_EVENT:

			psEvent=(EH_EVENT *) pVoid;
			switch (psEvent->iEvent)
			{
				//
				// Controllo del click sinistro
				//
				case EE_LBUTTONDOWN:
/*
					// Clicco sulla lente (se c'è)
					//if //psSearch->iLayOut!=ZIS_&&psEvent->sPoint.x<24)
					if (psSearch->bLens&&psEvent->sPoint.x<=psSearch->srLens.right)
					{
						obj_putevent("%sFLT",objCalled->nome); 
						return "ESC";
					}

					// Clicco sul tasto clean
					if (psSearch->bButtonClean&&psEvent->sPoint.x>(psSearch->lpObj->sClientSize.cx-27)) 
					{
						//efx2();
						*psSearch->pInputValue=0;
						*psSearch->pwInputValue=0;
						obj_putevent("%sCLS",psSearch->lpObj->nome); // Segnalo pulizia
						LCleanControl(psSearch);
						return "ESC";
					}

					// Pulizia della ricerca ( DA FARE)
					if (!psSearch->bTextEditEnable)
					{
						obj_putevent("%s",objCalled->nome); 
						return "ESC";
					}
					_EditFocus(psSearch);
					if (psSearch->bEventFocus) obj_putevent("%sFOCUS",objCalled->nome); 
					*/
					return "FOCUS";

				case EE_FOCUS:
					return "FOCUS";
					/*
					// if (psSearch->iLayOut==ZIS_LENS_ONLY) break;
					if (!strcmp(psEvent->szObjName,objCalled->nome))
					{
						_EditFocus(psSearch); 
						return "FOCUS";
					}
					else
					{
						_EditBlur(psSearch);
					}
					*/
					break;
			}
			break;


		case WS_INF: 
			return psSl;

	}
	return NULL;
}

//
// _initialize()
//
static void _initialize(void) {

	WNDCLASSEX	wc;

	//
	// Funzione principale del Form
	//
	_(wc);
	wc.cbSize        = sizeof(wc);
	wc.lpszClassName = WC_EH_SMARTLIST;
	wc.style         = CS_BYTEALIGNCLIENT| CS_HREDRAW | CS_VREDRAW; // | CS_PARENTDC;
	wc.lpfnWndProc   = _funcWinSL;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;//CreateSolidBrush(RGB(255,255,255));//GetSysColor(COLOR_BTNFACE));
	wc.lpszMenuName  = NULL;
	wc.hIconSm       = NULL;//LoadIcon(sys.EhWinInstance,MAKEINTRESOURCE(IDI_ICOMARE));
	RegisterClassEx(&wc);


	//
	// Funzione principale del Form
	//
	wc.cbSize        = sizeof(wc);
	wc.lpszClassName = WC_EH_SMARTLIST_HEADER;
	wc.style         = CS_BYTEALIGNCLIENT| CS_HREDRAW | CS_VREDRAW; // | CS_PARENTDC;

	wc.lpfnWndProc   = _funcWinSLHeader;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL; // CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	wc.lpszMenuName  = NULL;
	wc.hIconSm       = NULL;//LoadIcon(sys.EhWinInstance,MAKEINTRESOURCE(IDI_ICOMARE));
	RegisterClassEx(&wc);

	_p.bReady=TRUE;
}


//
// _this_Reset()
//
static void	_this_Reset(void *this) {

	EHZ_SMARTLIST * psSl=this;
	INT a;

	ShowWindow(psSl->wnd,SW_HIDE);
	for (a=0;a<psSl->dmiCol.Num;a++) {
		_colDestroy(&psSl->arsCol[a]);
	}
	psSl->dmiCol.Num=0;
//	ARDestroy(psSl->arBlurNotify); psSl->arBlurNotify=NULL;

}

//
// _this_setItemCount()
//
static BOOL	_this_setItemCount(void * this, INT iItems) {

	EHZ_SMARTLIST * psSl=this;
	
	if (psSl->iRows!=iItems) {
	
		psSl->ofsVert=0;
		psSl->ofsHorz=0;

	}
	psSl->iRows=iItems;
	ehFreePtr(&psSl->arsRowsInfo);
	if (psSl->iRows>0) {
		psSl->arsRowsInfo=ehAllocZero(sizeof(S_SLROWI)*iItems);
		psSl->bWaiting=false;
	}
	_tableCalc(psSl);
	return false;	

}

//
// _this_clean()
//
static BOOL	_this_clean(void * this, BOOL bWait) {
	
	EHZ_SMARTLIST * psSl=this;
	_this_setItemCount(this,0);
	if (bWait) 
		_this_setWaiting(this,bWait,NULL);
		else
		_this_refresh(this);
	return false;	
}


//
// _this_setWaiting()
//
static BOOL	_this_setWaiting(void * this, BOOL bWait,CHAR * pszWait) {
	
	EHZ_SMARTLIST * psSl=this;
	psSl->bWaiting=bWait;
	if (pszWait) strAssign(&psSl->pszTextWaiting,pszWait);
	_this_refresh(this);
	return false;	
}

//
// _colDestroy() > Libera le risorse impegnate con un campo
//
static BOOL _colDestroy(S_SL_COL * psCol)
{
	ehFreePtr(&psCol->pszCode);
	ehFreePtr(&psCol->pszTitle);
	if (psCol->psFont) fontDestroy(psCol->psFont,true);
	return FALSE;
}

//
// _cellGetPosition()
//
static void _cellGetPosition(EHZ_SMARTLIST * psSl,S_SL_CELL * psCell) {

	S_SL_COL * psCol=psCell->psCol; 

	psCell->recCell.left=psCol->recHeader.left-psSl->ofsHorz;
	psCell->recCell.right=psCol->recHeader.right-psSl->ofsHorz;
	psCell->recCell.top=psSl->iHeadHeight+(psCell->iRow*psSl->iItemHeight)-psSl->ofsVert;
	psCell->recCell.bottom=psCell->recCell.top+psSl->iItemHeight-1;
	rectCopy(psCell->recText,psCell->recCell);
	psCell->recText.left+=psSl->iCellPadding;
	psCell->recText.top+=psSl->iCellPadding;
	psCell->recText.right-=psSl->iCellPadding;
	psCell->recText.bottom-=psSl->iCellPadding;

}

//
// _cellFromPoint()
//
static BOOL _cellFromPoint(	EHZ_SMARTLIST * psSl,
							S_SL_CELL * psCell,
							POINT * psPoint,
							BOOL bIsHeader) {

	INT a;
	S_SL_COL * psCol;
	INT iCol=-1;
	INT iRow=-1;

	//
	// Localizzo la colonna
	//
	for (a=0;a<psSl->dmiCol.iLength;a++) {
		psCol=psSl->arsCol+a;
		if (!psCol->bVisible) continue;
		if (psPoint->x>=psCol->recHeader.left-(psCol->bFix?0:psSl->ofsHorz)&&
			psPoint->x<=psCol->recHeader.right-(psCol->bFix?0:psSl->ofsHorz)) {
				iCol=a; break;
			}
	
	}
	if (iCol<0) return true; // Fuori dall'area

	if (!bIsHeader) {
		
		for (a=0;a<psSl->iRows;a++) {
			
			INT y=psSl->iHeadHeight+(a*psSl->iItemHeight)-psSl->ofsVert;
			if (y<psSl->rcClient.top) continue;
			if (y>psSl->rcClient.bottom) break;

			if (psPoint->y>=y&&
				psPoint->y<=(y+psSl->iItemHeight)) {iRow=a; break;}
		
		}
	} else {
		
		iRow=0; if (psPoint->y<0||psPoint->y>psSl->iHeadHeight) iRow=-1;
	
	}
	memset(psCell,0,sizeof(S_SL_CELL));
	psCell->psCol=psCol;
	psCell->psRowInfo=&psSl->arsRowsInfo[iRow];
	psCell->iCol=iCol;
	psCell->iRow=iRow;
	if (!bIsHeader) _cellGetPosition(psSl,psCell);

	return false;

}


static void _headerMoveUdpdate(EHZ_SMARTLIST * psSl) {

	GetClientRect(psSl->wnd, &psSl->rcClient);
	sizeCalc(&psSl->sizClient,&psSl->rcClient);

	//			sizBar.cx=GetSystemMetrics(SM_CXVSCROLL);
	/*
	SetWindowPos(	psSl->wndHeader,HWND_TOP,
					psSl->psObj->px-psSl->ofsHorz,
					psSl->psObj->py,
					psSl->sizClient.cx+psSl->ofsHorz,
					psSl->iHeadHeight,0);
					*/
	SetWindowPos(	psSl->wndHeader,HWND_TOP,
					0,0,
					psSl->sizClient.cx,
					psSl->iHeadHeight,SWP_NOMOVE);
	InvalidateRect(psSl->wndHeader,NULL,false);
	//UpdateWindow(psSl->wndHeader);
//	printf("> %d,%d" CRLF,psSl->ofsHorz,psSl->sizClient.cx);

}

static void _rowLocation(EHZ_SMARTLIST * psSl,INT iRow,RECT * prcRect,BOOL bLeftRight) {

	prcRect->top=(psSl->iHeadHeightNormal+(iRow*psSl->iItemHeight)-psSl->ofsVert); // Elimino linea sopra
	prcRect->bottom=(prcRect->top+psSl->iItemHeight-1);

	if (bLeftRight) {
		prcRect->left=0;
		prcRect->right=psSl->sizTable.cx;
	}

}

static void _rowRedraw(EHZ_SMARTLIST * psSl,INT iRow) {

	RECT rcRow;
	_rowLocation(psSl,iRow,&rcRow,true);
	InvalidateRect(psSl->wnd,&rcRow,false);

}


static void _headerAnimation(EH_ANIMATION psAni,EN_MESSAGE enMess,LONG info,void * ptr) {

	S_ANI_ITEM * psItem=ptr;
	EHZ_SMARTLIST * psSl;
	switch (enMess) {
	
		case WS_DO:
			psSl=psItem->pExtra;
			psSl->iHeadHeight=psItem->rcAni.bottom;
			_tableCalc(psSl);
			_headerMoveUdpdate(psSl);
			break;
	
	
	}

}

static void _headerToolBarOpen(EHZ_SMARTLIST * psSl) {

	EH_ANIMATION psAni;
	S_ANI_OBJECT sObj;
	
	CHAR szServ[200];
	sprintf(szServ,"SL%s",psSl->psObj->nome);

	_(sObj);
	sObj.funcNotify=_headerAnimation;
	sObj.pExtra=psSl;
	rectFill(&sObj.rcObj,0,0,psSl->sizClient.cx,psSl->iHeadHeight);
	sObj.sizObj.cy=psSl->iHeadHeightNormal+psSl->iHeadToolBarHeight;
	sObj.pszTrans=ANI_TRANS_EASY;
	sObj.enPosRef=PSR_CY_ABS;
	sObj.dwTimeMilliSec=500;
	
	psAni=aniCreate(szServ,0);
	psAni->addObject(psAni,&sObj);
	psAni->play(psAni);

}

static void _headerToolBarClose(EHZ_SMARTLIST * psSl) {

	EH_ANIMATION psAni;
	S_ANI_OBJECT sObj;
	
	CHAR szServ[200];
	sprintf(szServ,"SL%s",psSl->psObj->nome);

	_(sObj);
	sObj.funcNotify=_headerAnimation;
	sObj.pExtra=psSl;
	rectFill(&sObj.rcObj,0,0,psSl->sizClient.cx,psSl->iHeadHeight);
	sObj.sizObj.cy=psSl->iHeadHeightNormal;
	sObj.pszTrans=ANI_TRANS_EASY;
	sObj.enPosRef=PSR_CY_ABS;
	sObj.dwTimeMilliSec=500;
	
	psAni=aniCreate(szServ,0);
	psAni->addObject(psAni,&sObj);
	psAni->play(psAni);

}

static void _setBodyMouseOver(EHZ_SMARTLIST * psSl,INT iRow,INT iCol) {

	INT iExRow=psSl->iBodyMouseOverRow;
	if (iRow==psSl->iBodyMouseOverRow&&
		iCol==psSl->iBodyMouseOverCol) return;


	psSl->iBodyMouseOverRow=iRow;
	psSl->iBodyMouseOverCol=iCol;
	if (iExRow!=-1) _rowRedraw(psSl,iExRow);
	if (psSl->iBodyMouseOverRow!=-1&&iExRow!=psSl->iBodyMouseOverRow) _rowRedraw(psSl,psSl->iBodyMouseOverRow);

}

static void _setHeadMouseOver(EHZ_SMARTLIST * psSl,INT iRow,INT iCol) {

	BOOL bRow=(iRow==0)?true:false;
	if (bRow==psSl->bHeadMouserOver&&
		iCol==psSl->iHeadMouseOverCol) return;

	if (bRow!=psSl->bHeadMouserOver) {
		switch (bRow) {
		
			case false: // mouse out
				if (psSl->bHeadToolBar) _headerToolBarClose(psSl);
//				printf("OUT" CRLF);
				break;

			case true: // mouse out
				if (psSl->bHeadToolBar) _headerToolBarOpen(psSl);
//				printf("OVER" CRLF);
				break;
		}
	
	}

	psSl->bHeadMouserOver=bRow;
	psSl->iHeadMouseOverCol=iCol;

}

//
// _funcWinSL > Funzione delegata principale
//
// static void _LSelectDrawItem(LPDRAWITEMSTRUCT psDis);
LRESULT CALLBACK _funcWinSL(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	EHZ_SMARTLIST * psSl;
	WORD wNotifyCode;
	INT iScanCode;
//	SIZE sizBar;

	psSl=(EHZ_SMARTLIST *) GetWindowLong(hWnd,GWL_USERDATA);

	switch (message)
	{
		case WM_CREATE:
			SetTimer(hWnd, 100, 50, NULL);
			break;

		case WM_DESTROY: 
			KillTimer(hWnd,100); 
			psSl->psDgMain=dgDestroy(psSl->psDgMain);
			break;

		case WM_TIMER: // Uscita sul capture su un altra finestra
			{
				POINT sPoint;
				S_SL_CELL sCell;
				HWND wndPoint;
				GetCursorPos(&sPoint);
				wndPoint=WindowFromPoint(sPoint);
				if (wndPoint==hWnd) {
					ScreenToClient(hWnd, &sPoint); 
					if (_cellFromPoint(psSl,&sCell,&sPoint,false)) sCell.iRow=-1;
				} else sCell.iRow=-1;
				_setBodyMouseOver(psSl,sCell.iRow,sCell.iCol);

			}
			break;


		case WM_SIZE:
			if (!psSl) break;
			_this_refresh(psSl);
			_headerMoveUdpdate(psSl);
			break;

		case WM_MOVE:
			if (!psSl) break;
			_headerMoveUdpdate(psSl);
			break;

	 // --------------------------------------------------------------------------------
     // Controllo Scorrimento VERTICALE                                                |
     // --------------------------------------------------------------------------------
		case WM_VSCROLL:
			psSl->ofsVert=ehScrollTranslate(hWnd,SB_VERT,wParam,psSl->ofsVert,psSl->sizTable.cy,psSl->sizClient.cy,psSl->iItemHeight,false);
			break;

		case WM_HSCROLL:
			psSl->ofsHorz=ehScrollTranslate(hWnd,SB_HORZ,wParam,psSl->ofsHorz,psSl->sizTable.cx,psSl->sizClient.cx,psSl->iItemHeight/3,true);
			InvalidateRect(hWnd,NULL,false);
			_headerMoveUdpdate(psSl);
			break;



		case WM_CHAR:
		case WM_SYSCHAR:
//			printf("qui");		
			break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
//			break;

//	case WM_SYSKEYUP:
//	case WM_KEYUP:
			iScanCode=(int) ((lParam>>16)&0xFF);
			// printf("qui [%d]",iScanCode);
			switch (iScanCode) {

					case 71: // Home
						SendMessage(hWnd,WM_VSCROLL,SB_TOP,0);
						break;
					case 79: // End
						SendMessage(hWnd,WM_VSCROLL,SB_BOTTOM,0);
						break;
					case 80: // Arrow Down
						SendMessage(hWnd,WM_VSCROLL,SB_LINEDOWN,0);
						break;
					case 72: // Up
						SendMessage(hWnd,WM_VSCROLL,SB_LINEUP,0);
						break;
					case 81: // PageDown
						SendMessage(hWnd,WM_VSCROLL,SB_PAGEDOWN,0);
						break;
					case 73: // PageUp
						SendMessage(hWnd,WM_VSCROLL,SB_PAGEUP,0);
						break;


					case 77: // Right
						SendMessage(hWnd,WM_HSCROLL,SB_LINERIGHT,0);
						break;
					case 75: // Left
						SendMessage(hWnd,WM_HSCROLL,SB_LINELEFT,0);
						break;

			}
			
			break;


	 case WM_MOUSEWHEEL:
			{
			 //WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
			 //short zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/120;
			 WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
			 short zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
			 INT x= GET_X_LPARAM(lParam);
			 INT y= GET_Y_LPARAM(lParam);
			 // printf("%d,%d,%d" CRLF,zDelta,x,y);
			 if (zDelta>0) {SendMessage(hWnd,WM_VSCROLL,SB_LINEUP,0); SendMessage(hWnd,WM_VSCROLL,SB_LINEUP,0); SendMessage(hWnd,WM_VSCROLL,SB_LINEUP,0);}
			 if (zDelta<0) {SendMessage(hWnd,WM_VSCROLL,SB_LINEDOWN,0); SendMessage(hWnd,WM_VSCROLL,SB_LINEDOWN,0); SendMessage(hWnd,WM_VSCROLL,SB_LINEDOWN,0);}
			 //psSl->ofsVert=ehScrollTranslate(hWnd,SB_VERT,zDelta*20,psSl->ofsVert,psSl->sizTable.cy,psSl->sizClient.cy);
			}
			break;

	 case WM_LBUTTONDOWN: 
	 case WM_RBUTTONDOWN:
	 case WM_LBUTTONUP:
	 case WM_RBUTTONUP:
	 case WM_LBUTTONDBLCLK:

         //if (sys.WinInputFocus<0) break;
		 // Ricalcolo la posizione dell'intercettazione
		 // relativa alla posizione dell'oggetto
		{
			POINT sPoint;
			LONG lPar;
			S_SL_CELL sCell;
			sPoint.x=LOWORD(lParam); sPoint.y=HIWORD(lParam);
			if (!_cellFromPoint(psSl,&sCell,&sPoint,false)) {

				EH_EVENT sEvent;
				_(sEvent);
				sEvent.sPoint.x=sPoint.x; 
				sEvent.sPoint.y=sPoint.y;
				sEvent.dwParam=sCell.iRow*1000+sCell.iCol; // Colonne massimo 999

				// Operazioni interne
				if (sCell.psCol->enReserved==CRES_CHECKBOX&&
					!sCell.psRowInfo->bCheckboxHidden) {

					S_SLROWI * psRowInfo;
					psRowInfo=&psSl->arsRowsInfo[sCell.iRow];
					if (message==WM_LBUTTONUP) {
						psRowInfo->bChecked^=1;
						_this_refreshEx(psSl,sCell.iRow,-1);
					}
				}

				switch (message) {

					case WM_LBUTTONDOWN:	sEvent.iEvent=EE_LBUTTONDOWN;	break;
					case WM_LBUTTONUP:		sEvent.iEvent=EE_LBUTTONUP;		break;
					case WM_RBUTTONDOWN:	sEvent.iEvent=EE_RBUTTONDOWN;	break;
					case WM_LBUTTONDBLCLK:	sEvent.iEvent=EE_LBUTTONDBLCLK; break;

				}
				psSl->funcNotify(psSl,WS_EVENT,1,&sEvent);

			}
			// printf("%d,%d" CRLF,sPoint.x,sPoint.y);

			lPar=MAKELONG((LOWORD(lParam)+psSl->psObj->px),(HIWORD(lParam)+psSl->psObj->py));
			WinMouseAction(sys.WinInputFocus,WIN_info[sys.WinInputFocus].hWnd,message,wParam,lPar);
			winSetFocus(hWnd);
		}
		 //GetWindowRect(hWnd,&Rect);
		 //WinMouse(psTE->Px+LOWORD(lParam)-Rect.left,psTE->Py+HIWORD(lParam)-Rect.top,lParam);
		 break;

     case WM_MOUSEMOVE: 
		 /*
			WinMouse((INT16) psSl->psObj->px+LOWORD(lParam),
					 (INT16) psSl->psObj->px+HIWORD(lParam),0xFFFF);
					 */
		 /*
		 	{
				POINT sPoint;
				S_SL_CELL sCell;
				sPoint.x=LOWORD(lParam); sPoint.y=HIWORD(lParam);
				if (_cellFromPoint(psSl,&sCell,&sPoint)) sCell.iRow=-1;
				_setBodyMouseOver(psSl,sCell.iRow,sCell.iCol);
				
			}
			*/
			break;

		case WM_SETFOCUS: 
			break;

		case WM_KILLFOCUS:

			break;

		case WM_COMMAND: 

			wNotifyCode = HIWORD(wParam);
			/*
			wID=LOWORD(wParam); // id del oggetto
			if (wID<FORM_ID_OFFSET) break;
			wID-=FORM_ID_OFFSET;
			if (wID>=psSl->dmiCol.Num) break;
			psCol=&psSl->arField[wID];
			switch (psCol->iType) {

				case FLD_CHECKBOX:
				case FLD_BUTTON:
					obj_putevent("%s.%s",psSl->lpObj->nome,psCol->pszName);
					break;

				case FLD_SELECT:
					if (wNotifyCode==CBN_SELENDOK) obj_putevent("%s.%s",psSl->lpObj->nome,psCol->pszName);
					break;

				case FLD_LIST:
					// LBN_SELCHANGE
//					printf("> %d" CRLF,wNotifyCode);
					if (wNotifyCode==LBN_SELCHANGE) obj_putevent("%s.%s",psSl->lpObj->nome,psCol->pszName);
					break;
				
				default: break;
			}
			*/
			break;


		case WM_PAINT: // Tabella
			{
				INT iCol,iRow;
				PAINTSTRUCT ps;
				S_SL_COL * psCol;
				INT x;
				SIZE sizPaint;
				EH_DG * psDg;
				RECT rcArea,rcClient,rc;
				SIZE sizClient;

				BeginPaint(hWnd,&ps);
				sizeCalc(&sizPaint,&ps.rcPaint);

				GetClientRect(hWnd,&rcClient); 
				sizeCalc(&sizClient,&rcClient);
				if (!psSl->psDgMain) psSl->psDgMain=dgCreate(0,sizClient.cx,sizClient.cy);
				if (memcmp(&psSl->psDgMain->sizArea,&sizClient,sizeof(SIZE))) {
					dgDestroy(psSl->psDgMain);
					psSl->psDgMain=dgCreate(0,sizClient.cx,sizClient.cy);
				}
				psDg=psSl->psDgMain;
				rectFill(&rcArea,0,0,psDg->sizArea.cx-1,psDg->sizArea.cy-1);
				if (!psSl->iRows) {

					CHAR * pszText;
					pszText=psSl->bWaiting?psSl->pszTextWaiting:psSl->pszTextProcess;
					//dcBoxp(psDg->hdc,&rcClient,sys.ColorShadow);
					if (psSl->bWaiting)
						dcBoxBrush(psDg->hdc,&rcClient,HS_BDIAGONAL,sys.Color3DLight,sys.Color3DShadow);
						else
						dcBoxBrush(psDg->hdc,&rcClient,HS_BDIAGONAL,sys.Color3DShadow,ColorLum(sys.Color3DShadow,-10));
					
					if (!strEmpty(pszText)) {

						RECT rc;
						RECTD rcd;
						SIZE sizArea;
						SIZE sizClient;
						
						sizeCalc(&sizClient,&rcClient);
						sizArea.cx=200; sizArea.cy=38;
						rc.left=rc.right=rcClient.left+((sizClient.cx-sizArea.cx)/2);
						rc.top=rc.bottom=rcClient.top+((sizClient.cy-sizArea.cy)/2);
						rc.right+=sizArea.cx;
						rc.bottom+=sizArea.cy;
						
						rectToD(&rcd,&rc);
						dcRectRoundEx(	psDg->hdc,&rcd,
										AlphaColor(160,sys.Color3DShadow),
										AlphaColor(255,sys.ColorBackGround),8,8,8);

						dcDrawText(	psDg->hdc,
									&rc,
									0,
									-1,//psCell->colBack,
									psSl->psFontTitle,
									1,
									pszText,
									-1,
									DPL_CENTER,
									DT_WORD_ELLIPSIS|DT_SINGLELINE|DT_VCENTER);

					}

				}
				else {

					RECT rec;
					rectFill(&rec,0,0,rcClient.right,psSl->iHeadHeight);
					//dcBoxp(psDg->hdc,&rec,sys.ColorBackGround);
					dcBoxBrush(psDg->hdc,&rcClient,HS_BDIAGONAL,sys.Color3DLight,sys.Color3DHighLight);

				}

				//
				// Contenuto
				//
//				if (psSl->funcNotify&&psSl->iRows) {
				if (psSl->funcNotify) {
					INT iFix;

					//
					// Prerichiesta di informazioni sulle righe in stampa
					//
					if (psSl->bRowRequest) {
						if (psSl->funcNotify&&psSl->iRows) {

							for (iRow=0;iRow<psSl->iRows;iRow++) {

								S_SL_CELL sCell;
								_(sCell);
								sCell.psRowInfo=&psSl->arsRowsInfo[iRow];
								sCell.iRow=iRow;
								sCell.iCol=0;
								psSl->funcNotify(psSl,WS_GET,0,&sCell);

							}
						
						}
					}

					for (iFix=0;iFix<2;iFix++) {

						// Loop sulle colonne (fatto cosi per i FIX)
						for (iCol=0;iCol<psSl->dmiCol.iLength;iCol++) {

							RECT recCell; // Posizione oggetto
							psCol=&psSl->arsCol[iCol]; 
							if (!psCol->bVisible) continue;
							if (psCol->bFix!=iFix) continue; 

							recCell.left=psCol->recHeader.left; if (!psCol->bFix) recCell.left-=psSl->ofsHorz;
							recCell.right=psCol->recHeader.right;  if (!psCol->bFix) recCell.right-=psSl->ofsHorz;
							if (recCell.left>ps.rcPaint.right||
								recCell.right<ps.rcPaint.left) continue; // Scremo Tolgo le colonne prima o dopo

							if (psSl->funcNotify&&psSl->iRows) {

								// Da fare: Calcolare il loop iRow>iRow dal base al ps.rcPaint.top <------------------
								for (iRow=0;iRow<psSl->iRows;iRow++) {

									S_SL_CELL sCell;
									_rowLocation(psSl,iRow,&recCell,false);
									if (recCell.bottom<ps.rcPaint.top) continue; // Da non stampare
									if (recCell.top>ps.rcPaint.bottom) break;

									_(sCell);
									sCell.hdc=psDg->hdc;
									sCell.psCol=psCol;
									sCell.psRowInfo=&psSl->arsRowsInfo[iRow];
									sCell.iRow=iRow;
									sCell.iCol=iCol;
									rectCopy(sCell.recCell,recCell);
									rectCopy(sCell.recText,recCell);
									sCell.recText.left+=psSl->iCellPadding;
									sCell.recText.top+=psSl->iCellPadding;
									sCell.recText.right-=psSl->iCellPadding;
									sCell.recText.bottom-=psSl->iCellPadding;
									sCell.pszText=psSl->pszCellBuffer;
									sCell.colBack=psCol->colBack;
									sCell.colText=psCol->colText;
									sCell.psFont=psCol->psFont?psCol->psFont:psSl->psFontText;

									//
									//  Stampo la cell ------------------------------------>
									//

									//
									// Preparo Background
									//
									if (psCol->enItemPaint&SCP_INTERNAL||
										psCol->enItemPaint==SCP_CUSTOM) {
										
										psSl->funcNotify(psSl,WS_REALGET,0,&sCell);
										
										// Focus
										if (iRow==psSl->iFocusRow&&
											(psSl->iFocusCol==-1||psSl->iFocusCol==iCol)) 
											sCell.colBack=ColorFusion(sCell.colBack,sys.ColorSelectBack,50);
										
										// Over
										if (iRow==psSl->iBodyMouseOverRow)
											sCell.colBack=ColorFusion(sCell.colBack,sys.ColorSelectBack,8);

										dcBoxp(sCell.hdc,&recCell,sCell.colBack);

									}

									//
									// Colonne reserved
									//
									if (psCol->enReserved) {
									
										switch (psCol->enReserved) {
										
											//
											// Disegno checkbox
											//
											case CRES_CHECKBOX:

												if (!sCell.psRowInfo->bCheckboxHidden) 
												{
													SIZE siz;
													RECTD recBox;
													double dSide;
													double dWeight;
													POINTD arsPoint[3];
													


													sizeCalc(&siz,&recCell);
													dSide=min(siz.cx,siz.cy);
													dWeight=(dSide*6/100);
													dSide-=(dSide*25/100);

													recBox.right=recBox.left=recCell.left+(siz.cx-dSide)/2;
													recBox.bottom=recBox.top=recCell.top+(siz.cy-dSide)/2;
													recBox.right+=(double) dSide;
													recBox.bottom+=(double) dSide;

													dcRectRoundEx(	sCell.hdc,&recBox,
																	AlphaColor(255,sys.Color3DShadow),
																	AlphaColor(255,sys.ColorBackGround),4,4,
																	dWeight);
													rectCopyD(sCell.psRowInfo->recCheck,recBox);

													if (sCell.psRowInfo->bChecked) {

														double dPix=dSide/9;
														arsPoint[0].x=dPix*1.5; arsPoint[0].y=dPix*4;
														arsPoint[1].x=dPix*3.5; arsPoint[1].y=dPix*6;
														arsPoint[2].x=dPix*7; arsPoint[2].y=dPix*2.5;
														pointTranslateD(3,arsPoint,recBox.left,recBox.top);
														dcLines(	psDg->hdc,
																	AlphaColor(255,0),
																	dWeight*1.5,
																	3,arsPoint);

													}
												}
												break;
										
										}

									}
									else {

										//
										//  -> Richiesta esterna ->
										//
										if (psCol->enItemPaint&SCP_BEFORE) psSl->funcNotify(psSl,WS_DISPLAY,SCP_BEFORE,&sCell);

										//
										// Stampa Testo Interna
										//
										if (psCol->enItemPaint&SCP_INTERNAL) {

											dcDrawText(	sCell.hdc,
														&sCell.recText,
														sCell.colText,
														-1,//sCell.colBack,
														sCell.psFont,
														1,
														sCell.pszText,
														-1,
														psCol->enAlign,
														DT_WORD_ELLIPSIS);
										}

										//
										//  -> Custom Post
										//
										if (psCol->enItemPaint&SCP_AFTER) psSl->funcNotify(psSl,WS_DISPLAY,SCP_AFTER,&sCell);

										//
										// Richiesta di stampa customizzata totale
										//
										if (psCol->enItemPaint==SCP_CUSTOM) {

											psSl->funcNotify(psSl,WS_DISPLAY,SCP_CUSTOM,&sCell);

										}
										
									}				
							/*
									// Linea verticale colonna
									x=psCol->recHeader.right-1; if (!psCol->bFix) x-=psSl->ofsHorz;
									dcLine(psDg->hdc,x,ps.rcPaint.top,x,ps.rcPaint.bottom,ColorLum(psSl->colLine,20));

									//
									// Ombra verticale colonna
									//
									if (psCol->recShadow.right) {
										
										RECT rcBox,rcGradient;
										RECTD rcd;
										rcBox.left=psCol->recHeader.right-1;
										rcBox.right=rcBox.left+psCol->recShadow.right;
										rcBox.top=ps.rcPaint.top;
										rcBox.bottom=ps.rcPaint.bottom;

										rectCopy(rcGradient,rcBox); 
										rcGradient.bottom=rcGradient.top; rcGradient.left++; rcGradient.right++; //rcGradient.top--; rcGradient.bottom++;
										dcBoxGradient(psDg->hdc,&rcBox,&rcGradient,AlphaColor(120,sys.Color3DShadow),AlphaColor(0,sys.Color3DShadow));

									}
*/
								}

							// Post se ho delle righe

							// Linea verticale colonna
							x=psCol->recHeader.right-1; if (!psCol->bFix) x-=psSl->ofsHorz;
							dcLine(psDg->hdc,x,ps.rcPaint.top,x,ps.rcPaint.bottom,ColorLum(psSl->colLine,20));

							//
							// Ombra verticale colonna
							//
							if (psCol->recShadow.right) {
								
								RECT rcBox,rcGradient;
//								RECTD rcd;
								rcBox.left=psCol->recHeader.right-1;
								rcBox.right=rcBox.left+psCol->recShadow.right;
								rcBox.top=ps.rcPaint.top;
								rcBox.bottom=ps.rcPaint.bottom;

								rectCopy(rcGradient,rcBox); 
								rcGradient.bottom=rcGradient.top; rcGradient.left++; rcGradient.right++; //rcGradient.top--; rcGradient.bottom++;
								dcBoxGradient(psDg->hdc,&rcBox,&rcGradient,AlphaColor(120,sys.Color3DShadow),AlphaColor(0,sys.Color3DShadow));

							}

							} // if iRow
						}
					}

					//
					// Loop Righe (linee orizzontali)
					//
					for (iRow=0;iRow<psSl->iRows;iRow++) {
						
						INT y=psSl->iHeadHeightNormal+(iRow*psSl->iItemHeight)-psSl->ofsVert;
						if (y<ps.rcPaint.top||y>ps.rcPaint.bottom) continue;
						dcLine(psDg->hdc,ps.rcPaint.left,y,ps.rcPaint.right,y,psSl->colLine);
					
					}
				}

				//
				// Pulisco quello che va oltre (a destra)
				//
				if (psSl->iRows&&psSl->sizTable.cx-psSl->ofsHorz>=ps.rcPaint.left) {
					rectCopy(rc,ps.rcPaint);
					rc.left=psSl->sizTable.cx-psSl->ofsHorz;
					//dcBoxp(psDg->hdc,&rc,sys.Color3DShadow);

					dcBoxBrush(psDg->hdc,&rc,HS_BDIAGONAL,sys.Color3DLight,sys.Color3DHighLight);
					dcLine(psDg->hdc,rc.left,rc.top,rc.left,rc.bottom,psSl->colLine);

				}

				//
				// Pulisco quello che va oltre (a sotto)
				//
				if (psSl->iRows&&psSl->sizTable.cy>=ps.rcPaint.top) {

					rectCopy(rc,ps.rcPaint);
					rc.top=psSl->sizTable.cy;
					dcBoxBrush(psDg->hdc,&rc,HS_BDIAGONAL,sys.Color3DLight,sys.Color3DHighLight);
					dcLine(psDg->hdc,rc.left,rc.top,psSl->sizTable.cx-psSl->ofsHorz+1,rc.top,psSl->colLine);

				}

				//
				// Fine del disegno, aggiorno
				//
				dgCopyDC(psDg,&ps.rcPaint,ps.rcPaint.left,ps.rcPaint.top,ps.hdc);
//				dgCopyDC(psDg,&rcClient,ps,0,0,ps.hdc);
				EndPaint(hWnd,&ps);

			}
			return 0;

	}
	
	return(DefWindowProc(hWnd, message, wParam, lParam));
}

static INT _colFromPoint(EHZ_SMARTLIST * psSl,INT x,INT y) {
	
	INT a;
	S_SL_COL * psCol;
	POINT ptCord;
	RECT recCell;
	ptCord.x=x; ptCord.y=y;
	recCell.top=0; recCell.bottom=psSl->iHeadHeight;
	for (a=0;a<psSl->dmiCol.iLength;a++) {
		psCol=psSl->arsCol+a;
		if (!psCol->bVisible) continue;
		recCell.left=psCol->recHeader.left; if (!psCol->bFix) recCell.left-=psSl->ofsHorz;
		recCell.right=psCol->recHeader.right;  if (!psCol->bFix) recCell.right-=psSl->ofsHorz;
		if (isPointInRect(&ptCord,&recCell)) return a;
	}
	return -1;

}


//
// _funcWinSLHeader - Procedura Window Header
//
LRESULT CALLBACK _funcWinSLHeader(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	EHZ_SMARTLIST * psSl;
	WORD wNotifyCode;

	psSl=(EHZ_SMARTLIST *) GetWindowLong(hWnd,GWL_USERDATA);

	switch (message)
	{
		case WM_CREATE:
			SetTimer(hWnd, 101, 50, NULL);
			break;

		case WM_DESTROY: 
			KillTimer(hWnd,101); 
			psSl->psDgHeader=dgDestroy(psSl->psDgHeader);
			break;


		case WM_TIMER: // Uscita sul capture su un altra finestra
			{
				POINT sPoint;
				S_SL_CELL sCell;
				HWND wndPoint;
				GetCursorPos(&sPoint);
				wndPoint=WindowFromPoint(sPoint);
				if (wndPoint==hWnd) {
					ScreenToClient(hWnd, &sPoint); 
					if (_cellFromPoint(psSl,&sCell,&sPoint,false)) sCell.iRow=-1;
				} else sCell.iRow=-1;
				_setHeadMouseOver(psSl,sCell.iRow,sCell.iCol);

			}
			break;

		case WM_SIZE:
			break;

		case WM_COMMAND: 

			wNotifyCode = HIWORD(wParam);
			break;

		case WM_PAINT: // Header
			{
				PAINTSTRUCT ps;
				S_SL_COL * psCol;
				INT iCol;
				RECT rcClient,rcArea;
				SIZE sizClient;
				EH_DG * psDg;
				INT iFix;

				BeginPaint(hWnd,&ps);

				GetClientRect(hWnd,&rcClient); 
				sizeCalc(&sizClient,&rcClient);
				if (!psSl->psDgHeader) psSl->psDgHeader=dgCreate(0,sizClient.cx,sizClient.cy);
				if (memcmp(&psSl->psDgHeader->sizArea,&sizClient,sizeof(SIZE))) {
					dgDestroy(psSl->psDgHeader);
					psSl->psDgHeader=dgCreate(0,sizClient.cx,sizClient.cy);
				}
				psDg=psSl->psDgHeader;
				rectFill(&rcArea,0,0,psDg->sizArea.cx-1,psDg->sizArea.cy-1);
				dcBoxp(psDg->hdc,&rcClient,sys.ColorBackGround);

				//
				// Loop Colonne (linee verticali)
				//
				for (iFix=0;iFix<2;iFix++) {

					for (iCol=0;iCol<psSl->dmiCol.iLength;iCol++) {

						RECT rectBtn,rectText;
						psCol=&psSl->arsCol[iCol];
						if (!psCol->bVisible) continue;
						if (psCol->bFix!=iFix) continue;

						psCol->recHeader.bottom=psSl->iHeadHeight-1;
						rectCopy(rectBtn,psCol->recHeader);
						rectCopy(rectText,psCol->recText);
						if (!psCol->bFix) {
							rectBtn.left-=psSl->ofsHorz;
							rectBtn.right-=psSl->ofsHorz;
							rectText.left-=psSl->ofsHorz;
							rectText.right-=psSl->ofsHorz;
						}

						rectText.top+=psSl->iHeadHeight-psSl->iHeadHeightNormal;
						if (psCol->iSort)
							dcBoxp(psDg->hdc,&rectBtn,psCol->iOrder?ColorFusion(sys.Color3DShadow,sys.Color3DHighLight,50):sys.ColorBackGround);
							else	
							dcBoxp(psDg->hdc,&rectBtn,ColorFusion(sys.ColorBackGround,sys.Color3DHighLight,50));
						dcDrawText(psDg->hdc,&rectText,psSl->colText,-1,psSl->psFontTitle,1,psCol->pszTitle,-1,psCol->enAlign,DT_TOP);
						dcLine(psDg->hdc,rectBtn.right-1,ps.rcPaint.top,rectBtn.right-1,ps.rcPaint.bottom,psSl->colLine);

						if (psCol->iOrder) {

							POINTD arsPoint[3];

							// Triangolo dell'ordine
							if (psCol->iOrder==2) {

								arsPoint[0].x=0; arsPoint[0].y=0;
								arsPoint[1].x=psSl->iArrowWidth; arsPoint[1].y=0;
								arsPoint[2].x=(DOUBLE) psSl->iArrowWidth/2; arsPoint[2].y=psSl->iArrowWidth;

							} else {
							
								arsPoint[0].x=0; arsPoint[0].y=psSl->iArrowWidth;
								arsPoint[1].x=psSl->iArrowWidth; arsPoint[1].y=psSl->iArrowWidth;
								arsPoint[2].x=(DOUBLE) psSl->iArrowWidth/2; arsPoint[2].y=0;
							}

							pointTranslateD(3,arsPoint,
											rectText.left+(psCol->sizHeader.cx/2)-psSl->iArrowWidth,
											rectBtn.bottom-8-psSl->iArrowWidth/2);

							dcPolygon(	psDg->hdc,
										AlphaColor(128,sys.Color3DShadow),
										3,
										AlphaColor(0,sys.Color3DHighLight),
										3,arsPoint);
							dcPolygon(	psDg->hdc,
										AlphaColor(0,sys.Color3DShadow),
										0,
										AlphaColor(255,sys.Color3DHighLight),
										3,arsPoint);
						}
					}
				}
				dcLine(psDg->hdc,0,0,psSl->sizTable.cx,0,sys.Color3DHighLight);
				dcLine(psDg->hdc,0,psSl->iHeadHeight-1,psSl->sizTable.cx,psSl->iHeadHeight-1,ColorFusion(sys.Color3DShadow,sys.Color3DHighLight,40));

				dgCopyDC(psDg,&rcClient,0,0,ps.hdc);
				EndPaint(hWnd,&ps);
			}
			return 0;

     case WM_MOUSEMOVE:
		  {
			INT x=LOWORD(lParam),y=HIWORD(lParam);
			INT iCol=_colFromPoint(psSl,LOWORD(lParam),HIWORD(lParam));
			// printf("%d,%d,%d" CRLF,iCol,x,y);
		  }
		  break;

	 case WM_LBUTTONDOWN:
	 case WM_RBUTTONDOWN:
	 case WM_LBUTTONUP:
	 case WM_RBUTTONUP:
	 case WM_LBUTTONDBLCLK:
	 case WM_RBUTTONDBLCLK:

			if (message==WM_RBUTTONUP) {

					BYTE *pChoose;
					EH_MENU *psMenu;
					INT a;
					CHAR szCode[200];
					CHAR szServ[200];
					psMenu=ehMenuCreate();
					ehMenuAdd(psMenu,EHM_ITEM,"Selezione colonne",FALSE,"",NULL,0,0,NULL);
					ehMenuAdd(psMenu,EHM_UNCHECK,"Mostra tutto",TRUE,"ALL",NULL,0,0,NULL);
					for (a=0;a<psSl->dmiCol.iLength;a++) {
					
						if (!psSl->arsCol[a].bSelectable) continue;
						sprintf(szCode,"#%d",a);	
						strCpy(szServ,psSl->arsCol[a].pszTitle,sizeof(szServ));
						while (strReplace(szServ,"\n"," "));
						ehMenuAdd(psMenu,psSl->arsCol[a].bVisible?EHM_CHECK:EHM_UNCHECK,szServ,true,szCode,NULL,0,0,NULL);
					}

					pChoose=ehMenu(psMenu->arsItem,"",NULL,NULL);
					if (*pChoose) 
					{
						if (!strCmp(pChoose,"ALL")) {

							for (a=0;a<psSl->dmiCol.iLength;a++) {

								if (!psSl->arsCol[a].bSelectable) continue;
								psSl->arsCol[a].bVisible=true;

							}
							psSl->refresh(psSl);

						}
						else if (*pChoose=='#') {
							a=atoi(pChoose+1);
							psSl->arsCol[a].bVisible^=1;
							psSl->refresh(psSl);
							
						}
					 
					}
					ehMenuDestroy(psMenu);
			}


			if (psSl->funcNotify)	
			{
				INT x=LOWORD(lParam),y=HIWORD(lParam);
				INT iCol=_colFromPoint(psSl,x,y);
				if (iCol>-1) {
					EH_EVENT sEvent;
					_(sEvent);
					sEvent.sPoint.x=x; sEvent.sPoint.y=y;
					sEvent.dwParam=iCol;
					switch (message) {
						case WM_LBUTTONDOWN: sEvent.iEvent=EE_LBUTTONDOWN; break;
						case WM_LBUTTONUP: sEvent.iEvent=EE_LBUTTONUP; break;
						case WM_RBUTTONDOWN: sEvent.iEvent=EE_RBUTTONDOWN; break;
						case WM_LBUTTONDBLCLK: sEvent.iEvent=EE_LBUTTONDBLCLK; break;
					}
					psSl->funcNotify(psSl,WS_EVENT,0,&sEvent);
				}

		   }
		  winSetFocus(psSl->wnd); // Sposto il focus sulla finestra principale
		   break;

	}
	
	return(DefWindowProc(hWnd, message, wParam, lParam));
}


//
// -_this_addCol() > Aggiunge una colonna
//
static BOOL	_this_addCol(void * this, INT idCode, CHAR * pszCode, CHAR * pszTitle, CHAR * pszStyle) {

	EHZ_SMARTLIST *psSl=this;
	S_SL_COL sCol;
	BOOL bRet=false;

	_(sCol);
	sCol.idCode=idCode;
	sCol.pszCode=strDup(strEver(pszCode));
	sCol.pszTitle=strDup(strEver(pszTitle));
	sCol.colBack=psSl->colBack;
	sCol.colText=psSl->colText;
	sCol.bVisible=true;
	sCol.enItemPaint=SCP_INTERNAL;
	_paramParser(&sCol,pszStyle);

	DMIAppendDyn(&psSl->dmiCol,&sCol);
	psSl->arsCol=DMILock(&psSl->dmiCol,NULL);

	_tableCalc(psSl);
	_headerButton(psSl);

	return bRet;
}


//
// _paramParser()
//
static void _paramParser(S_SL_COL * psCol,CHAR *pszParams) {
	
	//EH_AR arRow,arFld;
	//INT a,iToken;
	EH_CSS * pCss;
	CHAR * psz;

	if (strEmpty(pszParams)) return;

	pCss=cssCreate(pszParams);

	cssAssignWidth(pCss,"width",&psCol->dWidth,&psCol->enWidth);
	cssAssignWidth(pCss,"min-width",&psCol->dMinWidth,&psCol->enMinWidth);
	cssAssignWidth(pCss,"max-width",&psCol->dMaxWidth,&psCol->enMaxWidth);

//	cssAssign(pCss,"width","int32",&psCol->iWidth);
	cssAssign(pCss,"align","align",&psCol->enAlign);
	

	psz=cssGet(pCss,"sort"); 
	if (psz) {
		psCol->iSort=0;
		if (!strcmp(psz,"asc")) psCol->iSort=1;
		else if (!strcmp(psz,"desc")) psCol->iSort=2;
	}

	cssAssign(pCss,"fix","bool",&psCol->bFix);
	cssAssign(pCss,"shadow-right","int32",&psCol->recShadow.right);
	cssAssign(pCss,"cell-edit","bool",&psCol->bCellEdit);
	cssAssign(pCss,"selectable","bool",&psCol->bSelectable);
	cssAssign(pCss,"font","font",&psCol->psFont);
	cssAssign(pCss,"visibility","visibility",&psCol->bVisible);
	
	psz=cssGet(pCss,"visibility"); if (psz) psCol->bVisible=cssVisibility(psz);
	psz=cssGet(pCss,"paint");
	if (psz) {

		psCol->enItemPaint=SCP_INTERNAL; 
		if (!strcmp(psz,"custom")) psCol->enItemPaint=SCP_CUSTOM;
		else if (!strcmp(psz,"before")) psCol->enItemPaint=SCP_INTERNAL|SCP_BEFORE;
		else if (!strcmp(psz,"after")) psCol->enItemPaint=SCP_INTERNAL|SCP_AFTER;
		else if (!strcmp(psz,"before-after")) psCol->enItemPaint=SCP_INTERNAL|SCP_BEFORE|SCP_AFTER;
		else if (!strcmp(psz,"internal")) psCol->enItemPaint=SCP_INTERNAL;

	}
	psz=cssGet(pCss,"reserved");
	if (psz) {
	
		if (!strcmp(psz,"checkbox")) 
			psCol->enReserved=CRES_CHECKBOX;;
	}
	pCss=cssDestroy(pCss);


/*
	arRow=ARCreate(pszParam,";",NULL);
	
	for (a=0;arRow[a];a++) {
		
		arFld=ARCreate(strTrim(arRow[a]),":",&iToken);

		if (iToken==2) {
			strTrim(arFld[0]); strTrim(arFld[1]); 
			if (!strcmp(arFld[0],"width")) 
				psCol->iWidth=atoi(arFld[1]);

			else if (!strcmp(arFld[0],"align")) 
			{
				psCol->enAlign=DPL_LEFT;
				if (!strcmp(arFld[1],"center")) psCol->enAlign=DPL_CENTER;
				else if (!strcmp(arFld[1],"right")) psCol->enAlign=DPL_RIGHT;

			}
			else if (!strcmp(arFld[0],"sort")) 
			{
				psCol->iSort=0;
				if (!strcmp(arFld[1],"asc")) psCol->iSort=1;
				else if (!strcmp(arFld[1],"desc")) psCol->iSort=2;

			}
			else if (!strcmp(arFld[0],"fix")) 
			{
				psCol->bFix=false;
				if (!strcmp(arFld[1],"true")) psCol->bFix=true;

			}
			else if (!strcmp(arFld[0],"shadow-right")) 
			{
				psCol->recShadow.right=atoi(arFld[1]);

			}
			else if (!strcmp(arFld[0],"cell-edit")) 
			{
				psCol->bCellEdit=false;
				if (!strcmp(arFld[1],"true")) psCol->bCellEdit=true;

			}
			else if (!strcmp(arFld[0],"selectable")) 
			{
				psCol->bSelectable=false;
				if (!strcmp(arFld[1],"true")) psCol->bSelectable=true;

			}
			else if (!strcmp(arFld[0],"font")) 
			{
				INT iCount;
				DWORD dwStyle,dwSize;
				CHAR szFont[200];
				EH_AR ar=ARCreate(arFld[1]," ",&iCount);
				if (iCount==3) {

					if (!strcmp(ar[0],"normal")) dwStyle=STYLE_NORMAL;
					if (!strcmp(ar[0],"bold")) dwStyle=STYLE_BOLD;
					dwSize=atoi(strKeep(ar[1],"0123456789"));
					sprintf(szFont,"#%s",ar[2]);
					if (psCol->psFont) fontDestroy(psCol->psFont,true);
					psCol->psFont=fontCreate(szFont,dwSize,dwStyle,true,NULL,NULL);

				}
				ARDestroy(ar);
				_fontParser(arFld[1],&psCol->psFont);
			}
			else if (!strcmp(arFld[0],"visibility")) 
			{
				if (!strcmp(arFld[1],"visible")) psCol->bVisible=true;
				else if (!strcmp(arFld[1],"hidden")) psCol->bVisible=false;
			}
			else if (!strcmp(arFld[0],"paint")) 
			{
				psCol->enItemPaint=SCP_INTERNAL; // Default interna
				if (!strcmp(arFld[1],"custom")) psCol->enItemPaint=SCP_CUSTOM;
				else if (!strcmp(arFld[1],"before")) psCol->enItemPaint=SCP_INTERNAL|SCP_BEFORE;
				else if (!strcmp(arFld[1],"after")) psCol->enItemPaint=SCP_INTERNAL|SCP_AFTER;
				else if (!strcmp(arFld[1],"before-after")) psCol->enItemPaint=SCP_INTERNAL|SCP_BEFORE|SCP_AFTER;
				else if (!strcmp(arFld[1],"internal")) psCol->enItemPaint=SCP_INTERNAL;
			}

		}
		ARDestroy(arFld);
	}
	ARDestroy(arRow);
*/

}


static INT _widthCalc(EHZ_SMARTLIST * psSl,double dWidth,EN_CSS_WIDTH enMode) {

	INT iWidth=0; // Pixel

	switch (enMode) {
		
		case CSW_PIXEL:
			iWidth=(INT) dWidth; break;
			break;
		
		case CSW_PERC:
			iWidth=(INT) ((double) psSl->sizClient.cx*dWidth/100);
			break;
	
		default: 
			iWidth=0;
			break;
	}
	return iWidth;
}

//
// _tableCalc() - Calcolo delle dimensioni 
//
static void _tableCalc(EHZ_SMARTLIST * psSl) {

	INT a,iPos=0;
	S_SL_COL * psCol;

	GetClientRect(psSl->wnd, &psSl->rcClient);
	sizeCalc(&psSl->sizClient,&psSl->rcClient);

	psSl->arsCol[0].bVisible=psSl->bRowCheckBox;
//	if (psSl->bRowCheckBox) psSl->setColVisibility(void * this,CHAR * pszCodes,BOOL bVisible) {

	for (a=0;a<psSl->dmiCol.Num;a++) {
	
		psCol=&psSl->arsCol[a];
		if (psCol->bVisible) {

			INT iWidth=0;
			if (psCol->enWidth) {
				iWidth=_widthCalc(psSl,psCol->dWidth,psCol->enWidth);
			}

			if (psCol->enMinWidth) {
				INT iMinWidth=_widthCalc(psSl,psCol->dMinWidth,psCol->enMinWidth);
				if (iWidth<iMinWidth) iWidth=iMinWidth;
			}

			if (psCol->enMaxWidth) {
				INT iMaxWidth=_widthCalc(psSl,psCol->dMaxWidth,psCol->enMaxWidth);
				if (iWidth>iMaxWidth) iWidth=iMaxWidth;
			}

			psCol->sizHeader.cx=iWidth+(psSl->iCellPadding<<1);


			psCol->sizHeader.cy=psSl->iHeadHeight;
			psCol->recHeader.left=iPos;
			psCol->recHeader.right=iPos+psCol->sizHeader.cx;
			psCol->recHeader.top=0;
			psCol->recHeader.bottom=psSl->iHeadHeight-1;
			rectCopy(psCol->recText,psCol->recHeader);
			psCol->recText.left+=psSl->iCellPadding;
			psCol->recText.top+=psSl->iCellPadding;
			psCol->recText.right-=psSl->iCellPadding;
			psCol->recText.bottom-=psSl->iCellPadding;
			iPos+=psCol->sizHeader.cx;

		}
	}
	psSl->sizTable.cx=iPos;
	psSl->iItemHeight=(psSl->iCellPadding<<1)+psSl->psFontText->iHeight;
	psSl->sizTable.cy=psSl->iHeadHeight+(psSl->iRows*psSl->iItemHeight);

	// Determino se ho la barra laterale
	psSl->ofsVert=ehBarRangeAdjust(	psSl->wnd,
									SB_VERT,
									psSl->ofsVert,
									psSl->sizTable.cy, // Altezza del form
									psSl->sizClient.cy); // Altezza della finestra

	psSl->ofsHorz=ehBarRangeAdjust(	psSl->wnd,
									SB_HORZ,
									psSl->ofsHorz,
									psSl->sizTable.cx, // Altezza del form
									psSl->sizClient.cx); // Altezza della finestra
	
	// psSl->ofsVert=ehScrollTranslate(psSl->hwnd,SB_VERT,wParam,psSl->ofsVert,psSl->sizTable.cy,psSl->sizClient.cy);
}

//
// Costruisce i bottone dell'header
//

static void _headerButton(EHZ_SMARTLIST *psSl) {

}

//
// _this_SetFunction()
//
static BOOL		_this_setNotify(void * this,void * (*funcNotify)(void * this,EH_SRVPARAMS)) {

	EHZ_SMARTLIST *psSl=this;
	psSl->funcNotify=funcNotify;
	psSl->funcNotify(psSl,WS_CREATE,0,NULL);
	return false;
}


static BOOL		_this_refresh(void * this) {
	
	EHZ_SMARTLIST * psSl=this;
	_tableCalc(psSl);
	InvalidateRect(psSl->wnd,NULL,false);

//	if (psSl->iRows) 
	ShowWindow(psSl->wndHeader,SW_NORMAL);
//		else
//		ShowWindow(psSl->wndHeader,SW_HIDE);
	InvalidateRect(psSl->wndHeader,0,false);
	return false;

}

static BOOL _this_refreshEx(void * this,INT iRow,INT iCol) {

	EHZ_SMARTLIST * psSl=this;
	RECT rcRec;
	_this_getRect(this,iRow,iCol,&rcRec);
	InvalidateRect(psSl->wnd,&rcRec,false);
	return false;
}
//
// _this_ensureVisible()
//
static BOOL		_this_ensureVisible(void * this,INT iRow,INT iCol,RECT * psRect) {

	EHZ_SMARTLIST *psSl=this;
	RECT rec;
	SIZE sizClient;
	INT iDiff=0;
	GetClientRect(psSl->wnd, &psSl->rcClient);
	sizeCalc(&sizClient,&psSl->rcClient);
//	sizClient.cy-=psSl->iHeadHeight;
	if (_this_getRect(this,iRow,iCol,&rec)) return true; // Cella o colonna non esistono
	

	//
	// Troppo a sinistra
	//
	if (rec.left<psSl->rcClient.left) {
		
		iDiff=psSl->rcClient.left-rec.left;
		psSl->ofsHorz-=iDiff;
		psSl->ofsHorz=ehScrollTranslate(psSl->wnd,SB_HORZ,0,psSl->ofsHorz,psSl->sizTable.cx,psSl->sizClient.cx,psSl->iItemHeight/3,true);
		InvalidateRect(psSl->wnd,NULL,false);
		_this_getRect(this,iRow,iCol,&rec);
		_headerMoveUdpdate(psSl);

	}

	//
	// Troppo a destra
	//
	if (rec.right>psSl->rcClient.right) {
		
		iDiff=rec.right-psSl->rcClient.right;
		psSl->ofsHorz+=iDiff;
		psSl->ofsHorz=ehScrollTranslate(psSl->wnd,SB_HORZ,0,psSl->ofsHorz,psSl->sizTable.cx,psSl->sizClient.cx,psSl->iItemHeight/3,true);
		_this_getRect(this,iRow,iCol,&rec);
		InvalidateRect(psSl->wnd,NULL,false);
		_headerMoveUdpdate(psSl);

	}

	// Campo fuori in alto > scroll verso il basso
	printf(">top:  %d, offset: %d" CRLF,rec.top-psSl->ofsVert-psSl->iHeadHeight,psSl->rcClient.top);
	if ((rec.top-psSl->ofsVert-psSl->iHeadHeight)<psSl->rcClient.top) {

		/*
		iDiff=psSl->rcClient.top-(rec.top-psSl->ofsVert);
		psSl->ofsVert-=iDiff;
		psSl->ofsVert=ehScrollTranslate(psSl->wnd,SB_VERT,0,psSl->ofsVert,psSl->sizTable.cy,psSl->sizClient.cy,psSl->iItemHeight,true);
		InvalidateRect(psSl->wnd,NULL,false);
		_this_getRect(this,iRow,iCol,&rec);
		*/
		INT iOfs;
		iOfs=(rec.top-(psSl->iItemHeight/3)-psSl->iHeadHeight); if (iOfs<0) iOfs=0;
		psSl->ofsVert=ehScrollTranslate(psSl->wnd,SB_VERT,32,psSl->ofsVert,psSl->sizTable.cy,psSl->sizClient.cy,iOfs,false);


	}

	//
	// Scroll- su | Campo fuori in basso 
	//
	if ((rec.bottom-psSl->ofsVert)>psSl->rcClient.bottom) {
		
		INT iOfs;
		iOfs=(rec.bottom+(psSl->iItemHeight/3))-sizClient.cy; if (iOfs<0) iOfs=0;
		psSl->ofsVert=ehScrollTranslate(psSl->wnd,SB_VERT,32,psSl->ofsVert,psSl->sizTable.cy,psSl->sizClient.cy,iOfs,false);

	}
	if (psRect) 
	{
		memcpy(psRect,&rec,sizeof(rec));
		psRect->left+=psSl->ofsHorz;
		psRect->right+=psSl->ofsHorz;

	}

	return false;
}

//
// _this_getRect()
//
static BOOL	_this_getRect(void * this,INT iRow,INT iCol,RECT * psRect) {
	
	EHZ_SMARTLIST *psSl=this;
	S_SL_COL * psCol;

	if (iRow<0||iRow>=psSl->iRows) return true;
	if (iCol==-1) { // Ritorno il rettangolo della riga

		psRect->left=0;
		psRect->right=psSl->sizTable.cx;// psSl->
		psRect->top=(psSl->iHeadHeight+(iRow*psSl->iItemHeight)); // Elimino linea sopra
		psRect->bottom=(psRect->top+psSl->iItemHeight-1);
		return false;
	}
	if (iCol<0||iCol>=psSl->dmiCol.iLength) return true;

	psCol=&psSl->arsCol[iCol]; 
	psRect->left=psCol->recHeader.left; if (!psCol->bFix) psRect->left-=psSl->ofsHorz;
	psRect->right=psCol->recHeader.right;  if (!psCol->bFix) psRect->right-=psSl->ofsHorz;
	psRect->top=(psSl->iHeadHeight+(iRow*psSl->iItemHeight)); // Elimino linea sopra
	psRect->bottom=(psRect->top+psSl->iItemHeight-1);
	return false;
}

//
// _this_getCell()
//
static BOOL		_this_getCell(void * this,INT iRow,INT iCol,S_SL_CELL * psCell) {

	EHZ_SMARTLIST * psSl=this;

	//memset(psCell,0,sizeof(S_SL_CELL)); 
	psCell->psCol=NULL;
	psCell->psRowInfo=NULL;
	if (iRow<0||iRow>=psSl->iRows) return true;
	if (iCol<0||iCol>=psSl->dmiCol.iLength) return true;

	psCell->psCol=&psSl->arsCol[iCol];
	psCell->psRowInfo=&psSl->arsRowsInfo[iRow];
	psCell->iCol=iCol;
	psCell->iRow=iRow;
	_cellGetPosition(psSl,psCell);


	return false;
}

//
// _this_getCellEvent()
//
BOOL	_this_getCellEvent(void * this,EH_EVENT * psEvent,S_SL_CELL * psCell) {
	
	INT iRow=psEvent->dwParam/1000;
	INT iCol=psEvent->dwParam%1000;
	return _this_getCell(this,iRow,iCol,psCell);
}

//
// _colEditSearch()
//
static INT  _colEditSearch(EHZ_SMARTLIST * psSl,INT iRow,INT iCol,INT iDir) {

	iCol+=iDir;
	while (true) {

		if (iCol<0||iCol>=psSl->dmiCol.iLength) return -1;
		if (psSl->arsCol[iCol].bCellEdit) return iCol;
		iCol+=iDir;
	}
	return -1;
}


//
// _this_cellWalk()
//
static BOOL		_this_cellWalk(void * this,INT * piRow,INT * piCol, INT iKey) {

	EHZ_SMARTLIST * psSl=this;
	BOOL bError=false;
	EN_EF enKey=iKey;
	INT iColNext=-1,iLoop=0;
	INT iRow=*piRow;
	INT iCol=*piCol;
	INT iDir;

	switch (enKey) {
	
		case EF_TAB:
		case EF_ALT_TAB:
			if (enKey==EF_TAB) iDir=1; else iDir=-1;
			while (true) {
				iCol=_colEditSearch(psSl,iRow,iCol,iDir);
				// Fine corsa
				if (iCol<0) { 
					if (iDir==1) {iRow++; iCol=-1;} else {iRow--; iCol=psSl->dmiCol.iLength;}
					if (iRow<0||iRow>=psSl->iRows) { 
						iRow=*piRow; iCol=*piCol;
						break;
					}
				} else break;
			}
			break;

		case EF_FUP:
			iRow--; 
			break;

		case EF_FDOWN:
		case EF_CR:
			iRow++; 
			break;

		
		case EF_ESC:
		case EF_BLUR:
			bError=true;
			break;
	}

	if (iRow<0) iRow=0;
	if (iRow>=psSl->iRows) iRow=psSl->iRows-1;
	*piRow=iRow;
	*piCol=iCol;
	return bError;
}



static BOOL		_this_setFocusCell(void * this,INT iRow,INT iCell) {
	
	EHZ_SMARTLIST * psSl=this;
	psSl->iFocusRow=iRow;
	psSl->iFocusCol=iCell;
	InvalidateRect(psSl->wnd,NULL,false);
	return false;
	
}
static BOOL		_this_setFocusRow(void * this,INT iRow) {
	
	_this_setFocusCell(this,iRow,-1);
	return false;
}


static BOOL		_this_setVScroll(void * this,INT iSBMode,INT iOffset) {

	EHZ_SMARTLIST * psSl=this;
	SendMessage(psSl->wnd,WM_VSCROLL,iSBMode,iOffset);
	return false;
}

static BOOL		_this_setHeaderHeight(void * this,INT iHeight) {

	EHZ_SMARTLIST * psSl=this;
	psSl->iHeadHeightNormal=iHeight;
	psSl->iHeadHeight=iHeight;
	return false;
}


static BOOL		_this_setColVisibility(void * this,CHAR * pszCodes,BOOL bVisible) {

	EHZ_SMARTLIST * psSl=this;
	INT a,f;
	S_SL_COL * psCol;
	BOOL bTouch=false;

	EH_AR ar=ARFSplit(pszCodes,",");
	for (f=0;ar[f];f++) {
		for (a=0;a<psSl->dmiCol.iLength;a++) {
			psCol=psSl->arsCol+a;
			if (!strCmp(psCol->pszCode,ar[f])) 
			{
				if (psCol->bVisible!=bVisible) bTouch=true;
				psCol->bVisible=bVisible;
				break;
			}
		}
	}
	ehFree(ar);
	if (bTouch) psSl->refresh(psSl);
	return bTouch;

}


static BOOL		_this_setBodyStyle(void * this,CHAR * pszParams) {


//	EH_AR arRow,arFld;
	EHZ_SMARTLIST * psSl=this;
	EH_CSS * pCss;

	if (strEmpty(pszParams)) return false;
	pCss=cssCreate(pszParams);

	cssAssign(pCss,"padding","int32",&psSl->iCellPadding);
	cssAssign(pCss,"font","font",&psSl->psFontText);
	cssAssign(pCss,"checkbox","bool",&psSl->bRowCheckBox);
	if (psSl->bRowCheckBox) psSl->bRowRequest=true;

	pCss=cssDestroy(pCss);
	_this_refresh(psSl);
	return true;
}
