//   -----------------------------------------------------------
//   | ehzSignature
//   | 
//   |                                              
//   |									    by Ferrà Srl 02/2014
//   -----------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehzSignature.h"

#define SIGN_CLASS_NAME "ehzSignature"

static HWND _LCreateStaticText(HINSTANCE hInstance, HWND hwndParent);
static void SwitchView(HWND hwndListView, DWORD dwView);
static LRESULT CALLBACK funcStaticText(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

//
// Static Functions
//
//static void _moveBar(EHZ_BARFRAME * psBF);
//static void _objRebuild(EHZ_BARFRAME * psBF);


//
// Static functions
//

static void *		_signZone(EH_OBJPARAMS);
/*
static void			_signReset(	EHZ_SIGNATURE * psSign,
								BOOL fClear,
								BOOL fType,
								S_SIGNAREA * lps,HWND hWndSign,BOOL fBGDraw);
*/
//static void			_signImgBuilder(S_SIGNAREA *lps,INT cx,INT cy,CHAR *lpName);
//static void			_signImgDestroy(S_SIGNAREA *lps);
//static void			_penServer(EHZ_SIGNATURE * psSign,INT enMess,LONG info,void *str);

LRESULT CALLBACK	_signProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static void			_areaControl(RECT *lprArea,SIZE *lpSize,RECT *lpsLimite);
static void			_addPoint(	EHZ_SIGNATURE * psSign,S_SIGNPOINT * psPoint);
static void			_penWrite(	EHZ_SIGNATURE * psSign,
								EN_PTTY enType,
								INT x,INT y,
								BOOL bAdd,
								BOOL bRefreshPoint);


//
// This
//
static void		_this_reset(void *this);
static void		_this_refresh(void *this);
static void		_this_setFont(void *this,CHAR * pszText);
static BOOL		_this_writeSign(void * this, CHAR * pszFileName);
static BOOL		_this_readSign(void * this, CHAR * pszFileName);
static BOOL		_this_writeImage(void * this, CHAR * pszFileName, EN_FILE_TYPE, INT cx,INT cy);
static void		_this_getSignRect(void * this,RECT * psRect);


static struct {
	BOOL bReady;

} _l={0};


static void _init(void) {

	WNDCLASSEX wc;

	// Registro la classe delle OW_SCR
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_NOCLOSE|CS_VREDRAW|CS_HREDRAW;
	wc.lpfnWndProc   = _signProcedure;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;//CreateSolidBrush(sSetup.iPenColor);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = SIGN_CLASS_NAME;
	wc.hIconSm       = NULL;//LoadIcon(NULL,IDI_APPLICATION);
	if (!RegisterClassEx(&wc)) ehError();

	_l.bReady=true;

}


//
// ehzSignature()
//
void * ehzSignature(EH_OBJPARAMS)
{
	EH_DISPEXT *	psDex=pVoid;
	EHZ_SIGNATURE * psSign;
	EH_EVENT * psEvent;

	if (!psObjCaller) return NULL; // 
	if (!_l.bReady) _init();

	psSign=psObjCaller->pOther;
	switch(enMess)
	{
		case WS_INF: return psSign;

		case WS_CREATE: 
			
			//
			// Inizializzo firma
			//
			psSign=psObjCaller->pOther=ehAllocZero(sizeof(EHZ_SIGNATURE));
			psSign->iWin=sys.WinWriteFocus;
			psSign->wndParent=WindowNow();
			psSign->psObj=psObjCaller;
			psSign->lstPoint=lstCreate(sizeof(S_SIGNPOINT));
			psSign->sizPen.cx=psSign->sizPen.cy=6;
			psSign->acPen=rgba(0,0,.55,.4);
			psSign->dBaseLinePos=25.0;
			psSign->dBaseLineWeight=.5;
			psSign->acBaseLine=rgba(0,0,0,.5);
			psSign->colBaseLineText=colorWeb("#aaa");
			psSign->enBaseLineStyle=PS_DASH;
			psSign->pszBaseLineText="Firmare qui";
			psSign->dScale=1;

			// Like C++ ;-)
			psSign->reset=_this_reset;
			psSign->refresh=_this_refresh;
			psSign->setFont=_this_setFont;
			psSign->getSignRect=_this_getSignRect;
			psSign->readSign=_this_readSign;
			psSign->writeSign=_this_writeSign;
			psSign->writeImage=_this_writeImage;

			psSign->setFont(psSign,"normal 14px Arial");
			psObjCaller->hWnd=CreateWindowEx(	0,
												SIGN_CLASS_NAME, 
												"",//(LPSTR) NULL, 
												WS_CHILD|WS_VISIBLE, 
												0,0,
												0,0,
												psSign->wndParent, 
												(HMENU) 0, 
												sys.EhWinInstance, psSign); 
			psSign->wndSign=psObjCaller->hWnd;
			break;

		case WS_DESTROY:

			//
			// Libero le risorse
			//
			if (psSign->psFontBaseLine) fontDestroy(psSign->psFontBaseLine,true);
			lstDestroy(psSign->lstPoint);
			DestroyWindow(psObjCaller->hWnd);
			ehFreePtr(&psObjCaller->pOther);
			break;

		case WS_PROCESS:
/*
			psJson=jsonCreate(ptr);			
			psz=jsonGet(psJson,"min"); if (psz) psBF->iMin=atoi(psz);
			psz=jsonGet(psJson,"minType"); if (psz) psBF->iMinType=atoi(psz);
			psz=jsonGet(psJson,"max"); if (psz) psBF->iMax=atoi(psz);
			psz=jsonGet(psJson,"maxType"); if (psz) psBF->iMaxType=atoi(psz);
			jsonDestroy(psJson);
			*/
			break;

		case WS_EVENT:

			psEvent=(EH_EVENT *) pVoid;
//			rectToI(&rc,&psInfo->rcdNext);
			switch (psEvent->iEvent) {

				case EE_CHAR:
					obj_addevent("ESCOFF");
					break;

				case EE_FOCUS:
					return "ESC";

		
			}
/*
			psEvent=ptr;
			switch (psEvent->iEvent)
			{
				case 0:
					MouseCursorDefault();
					break;

				case O_MOUSE_OVER:
					psBF->bMouseOver=true;
					obj_RefreshSolo(psObjCaller->nome,false,true); // Visualizzo la nuova posizione
					//mouse_graph(0,0,"W:HAND");
					break;

				case 17:
					psBF->bMouseOver=false;
					obj_RefreshSolo(psObjCaller->nome,false,true); // Visualizzo la nuova posizione
					break;


				case EE_LBUTTONDOWN:
					_moveBar(psBF);
				//	obj_putevent("%s",objCalled->nome); return "ESC";

					break;
			}
			*/
			break;


		case WS_DO: 
			MoveWindow(psObjCaller->hWnd,psDex->px,psDex->py,psDex->lx,psDex->ly,true);
			break;

		case WS_REALGET:
			break;


		case WS_REALSET: 

			break;

		case WS_DISPLAY: 
			break;

	}
	return NULL;
}
 

static void _areaControl(RECT *lprArea,SIZE *lpSize,RECT *lpsLimite)
{
	INT a;

	if (lprArea->left>lprArea->right) {a=lprArea->left; lprArea->left=lprArea->right; lprArea->right=a;}
	if (lprArea->top>lprArea->bottom) {a=lprArea->top; lprArea->top=lprArea->bottom; lprArea->bottom=a;}

	if (lprArea->left<lpsLimite->left) lprArea->left=lpsLimite->left;
	if (lprArea->right<lpsLimite->left) lprArea->right=lpsLimite->left;
	if (lprArea->left>=lpsLimite->right) lprArea->left=lpsLimite->right;
	if (lprArea->right>=lpsLimite->right) lprArea->right=lpsLimite->right;

	if (lprArea->top<lpsLimite->top) lprArea->top=lpsLimite->top;
	if (lprArea->bottom<lpsLimite->top) lprArea->bottom=lpsLimite->top;
	if (lprArea->top>=lpsLimite->bottom) lprArea->top=lpsLimite->bottom;
	if (lprArea->bottom>=lpsLimite->bottom) lprArea->bottom=lpsLimite->bottom;

	lpSize->cx=lprArea->right-lprArea->left+1;
	lpSize->cy=lprArea->bottom-lprArea->top+1;
}
/*

// Traccia un quadrato
void SF_boxp(INT px,INT py,INT cx,INT cy,INT cColor,S_SIGNAREA *lps)
{
	INT x,y;
	RECT rArea;
	SIZE sArea;
	BYTE bPixel[3];
	CHAR *lpbMemo,*lpb;
	INT iMemoSize;

	if (!cx) return;

	rArea.left=px;
	rArea.right=cx;
	rArea.top=py;
	rArea.bottom=cy;

	AreaControl(&rArea,&sArea,&lps->rArea);

	// Preparo area di stampa
	iMemoSize=sArea.cx*3;
	lpbMemo=lpb=ehAlloc(iMemoSize);
	bPixel[0]=GetBValue(cColor); 
	bPixel[1]=GetGValue(cColor); 
	bPixel[2]=GetRValue(cColor);
	
	for (x=0;x<sArea.cx;x++) {memcpy(lpb,&bPixel,3); lpb+=3;}

	lpb=lps->lpbSignBitMap;
	lpb+=(rArea.top*lps->ImgHead.linesize)+(rArea.left*3);
	for (y=0;y<sArea.cy;y++)
	{
		memcpy(lpb,lpbMemo,iMemoSize);
		lpb+=lps->ImgHead.linesize;
	}
	ehFree(lpbMemo);
}
*/
/*
INT ColorWebConvert(CHAR *lpColore)
{
	CHAR szServ[200];
	INT iColor[3];
	if (*lpColore=='#')
	{
		strncpy(szServ,lpColore+1,2); szServ[2]=0; strupr(szServ);
		iColor[0]=xtoi(szServ); 
		strncpy(szServ,lpColore+3,2); szServ[2]=0; strupr(szServ);
		iColor[1]=xtoi(szServ);
		strncpy(szServ,lpColore+5,2); szServ[2]=0; strupr(szServ);
		iColor[2]=xtoi(szServ);
		//iColor=RGB(iColor[0],iColor[1],iColor[2]);
		return RGB(iColor[0],iColor[1],iColor[2]);
	}

	if (*lpColore>='0'&&*lpColore<='9') return atoi(lpColore);

	strlwr(lpColore);
	if (!strcmp(lpColore,"white"))  return RGB(255,255,255);
	if (!strcmp(lpColore,"black"))  return RGB(0,0,0);
	if (!strcmp(lpColore,"gray"))   return RGB(127,127,127);
	if (!strcmp(lpColore,"red"))    return RGB(255,0,0);
	if (!strcmp(lpColore,"blue"))   return RGB(0,0,255);
	if (!strcmp(lpColore,"green"))  return RGB(0,255,0);
	if (!strcmp(lpColore,"yellow")) return RGB(255,255,0);
	if (!strcmp(lpColore,"orange")) return RGB(255,127,0);
	if (!strcmp(lpColore,"t")) return -1;
	return 0;
}
*/

/*
static void _signReset(	EHZ_SIGNATURE * psSign,
						BOOL fClear,
						BOOL fType,
						S_SIGNAREA * lps,HWND hWndSign,BOOL fBGDraw)
{
	INT x;
	INT iCnt=0;
//	INT iColor[2]={RGB(255,200,200),RGB(255,240,240)};
	
	INT iColor[2]={ColorWebConvert("#BAC89B"),ColorWebConvert("#C4D69D")};

	// CFE1A7
	if (fBGDraw) SF_boxp(lps->rArea.left,lps->rArea.top,lps->rArea.right,lps->rArea.bottom,psSign->colBack,lps);

	switch (fType)
	{

		case 0:
		for (x=lps->rArea.left-5;x<lps->rArea.right;x+=4)
		{
			if (x>=0)
			{
				if (!(iCnt%5))  SF_boxp(x,lps->rArea.top,x,lps->rArea.bottom,iColor[0],lps);
		 						else
		 						SF_boxp(x,lps->rArea.top,x,lps->rArea.bottom,iColor[1],lps);
			}
			iCnt++;
		}

		iCnt=0;
		for (x=-lps->rArea.top;x<lps->rArea.bottom;x+=4)
		{
			if (x>=0)
			{
				if (!(iCnt%5)) 
						SF_boxp(lps->rArea.left,x,lps->rArea.right,x,iColor[0],lps);
						else
						SF_boxp(lps->rArea.left,x,lps->rArea.right,x,iColor[1],lps);
			}
			iCnt++;
		}
		SF_boxp(lps->rArea.left,psSign->iBaseLine,lps->rArea.right,psSign->iBaseLine+1,ColorWebConvert("#D9AB47"),lps);
		break;

	case 1:
		break;
	}
	
	if (hWndSign) InvalidateRect(hWndSign,NULL,TRUE);
	if (fClear) lstClean(psSign->lstPoint); //_penServer(psSign,WS_OPEN,0,NULL);


}
*/
/*
void SF_AlphaPen(POINT p,INT iPenSize,INT cColor,INT iPenAlpha,S_SIGNAREA *lps)
{
	RECT rArea;
	SIZE sArea;
	BYTE bPixel[3];
	INT x,y;
	BYTE *lpbx,*lpby;
	INT iAlpha=10;

	BYTE bAlpha[][10]={
		{ 0, 0, 0, 0, 0, 0, 0 ,0 ,0, 0},
		{ 0, 3, 8, 9,10,10, 9, 8, 3, 0},
		{ 0, 8,10,15,20,20,15,10, 8, 0},
		{ 0, 9,15,30,50,50,30,15, 9, 0},
		{ 0,10,20,50,90,90,50,20,10, 0},
		{ 0,10,20,50,90,90,50,20,10, 0},
		{ 0, 9,15,30,50,50,30,15, 9, 0},
		{ 0, 8,10,15,20,20,15,10, 8, 0},
		{ 0, 3, 8, 9,10,10, 9, 8, 3, 0},
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	};


	rArea.left=p.x-(iPenSize>>1); rArea.right=rArea.left+iPenSize-1;
	rArea.top=p.y-(iPenSize>>1); rArea.bottom=rArea.top+iPenSize-1;

	AreaControl(&rArea,&sArea,&lps->rArea);
	bPixel[0]=GetBValue(cColor); 
	bPixel[1]=GetGValue(cColor); 
	bPixel[2]=GetRValue(cColor);

	lpby=lps->lpbSignBitMap;
	lpby+=(rArea.top*lps->ImgHead.linesize)+(rArea.left*3);
	for (y=0;y<sArea.cy;y++)
	{
		lpbx=lpby;
		for (x=0;x<sArea.cx;x++)
		{
			iAlpha=(INT) bAlpha[y%10][x%10]; //dispx("%d   ",iAlpha);
			if (iPenAlpha) iAlpha=iAlpha*iPenAlpha/100;
			if (iAlpha)
			{
				*lpbx=*lpbx+(BYTE) ((bPixel[0]-*lpbx)*((iAlpha%101))/100); lpbx++;
				*lpbx=*lpbx+(BYTE) ((bPixel[1]-*lpbx)*((iAlpha%101))/100); lpbx++;
				*lpbx=*lpbx+(BYTE) ((bPixel[2]-*lpbx)*((iAlpha%101))/100); lpbx++;
				//pbx=bPixel[0]; lpbx++;
				//pbx=bPixel[1]; lpbx++;
				//pbx=bPixel[2]; lpbx++;
			}
		}
	  lpby+=lps->ImgHead.linesize;
	}
}
*/

static void _penWrite(	 EHZ_SIGNATURE * psSign,
						 EN_PTTY enType,
						 INT x,INT y,
						 BOOL bAdd,
						 BOOL bRefreshPoint)
{
	RECT rArea;
	SIZE sArea;
	POINT p;
	INT j,jx=1;
	S_SIGNPOINT sSP;
	RECT rcRefresh;
	INT iPen;
	SIZE sizPen;
	POINT poiPen;

	x+=psSign->ptOfs.x; y+=psSign->ptOfs.y;
	if (psSign->dScale!=1.0) {x=(INT) (x*psSign->dScale); y=(INT) (y*psSign->dScale);}

	if (bAdd)  {
		_(sSP);
		sSP.pt.x=x; sSP.pt.y=y; sSP.enType=enType;
		_addPoint(psSign,&sSP);
	}
	
	if (enType==PEN_DOWN) {

		psSign->bPenWrite=true; 
		psSign->bPenStart=false;
	}

	if (enType==PEN_UP) {

		psSign->bPenWrite=false; 

	}

	if (enType) return;

	if (!psSign->bPenStart) {psSign->pLast.x=x; psSign->pLast.y=y; psSign->bPenStart=true;}
	
	// Calcolo la distanza tra i due punti
	rArea.left=psSign->pLast.x; rArea.right=x;
	rArea.top=psSign->pLast.y; rArea.bottom=y;

	_areaControl(&rArea,&sArea,&psSign->recArea); // Area da rinfrescare
	rectCopy(rcRefresh,rArea);	rectAdjust(&rcRefresh);

	sizPen.cx=(INT) (psSign->sizPen.cx*psSign->dScale);
	sizPen.cy=(INT) (psSign->sizPen.cy*psSign->dScale);
	poiPen.x=sizPen.cx/2; poiPen.y=sizPen.cy/2;
	// Calcolo ipotenusa
	jx=(INT) sqrt(sArea.cx*sArea.cx+sArea.cy*sArea.cy);
	for (j=0;j<jx;j+=1)
	{
		RECTD rdArea;

		// j:jx=x:pLast.x+(x-pLast.x)
		p.x=psSign->pLast.x+((x-psSign->pLast.x)*j/jx);
		p.y=psSign->pLast.y+((y-psSign->pLast.y)*j/jx);

		rdArea.left=(double) p.x-poiPen.x; rdArea.right=rdArea.left+sizPen.cx;
		rdArea.top=(double) p.y-poiPen.y; rdArea.bottom=rdArea.top+sizPen.cy;
		
		// SF_AlphaPen(p,iPenSize,iPenColor,iPenAlpha,lps);

		if (psSign->imgPen)
			dcImageShow(psSign->dgSign->hdc,(INT) rArea.left,(INT) rArea.top,(INT) psSign->sizPen.cx,(INT) psSign->sizPen.cy,psSign->imgPen);
			else
			dcEllipse(psSign->dgSign->hdc,&rdArea,psSign->acPen,0,0);

	}
	if (bRefreshPoint) {
		iPen=poiPen.x+1;
		rcRefresh.left-=iPen; rcRefresh.right+=iPen;
		rcRefresh.top-=iPen; rcRefresh.bottom+=iPen;
		InvalidateRect(psSign->wndSign,&rcRefresh,false);
	}
	psSign->pLast.x=x; psSign->pLast.y=y;
}

static void _signRender(EHZ_SIGNATURE * psSign);

//
// _signClose()
//
static void _signDestroy(EHZ_SIGNATURE * psSign) {

//	if (psSign->hBitmap) {DeleteObject(psSign->hBitmap); psSign->hBitmap=NULL;}
//	if (psSign->hdc) {DeleteDC(psSign->hdc); psSign->hdc=NULL;}
	if (psSign->dgSign) psSign->dgSign=dgDestroy(psSign->dgSign);

} 

//
// Ridimensionamento dell'area
//
static void _signCreate(EHZ_SIGNATURE * psSign,SIZE * psSize) {

	_signDestroy(psSign);
	if (psSize) {
		
		psSign->sizArea.cx=psSize->cx;
		psSign->sizArea.cy=psSize->cy;
		psSign->recArea.left=psSign->recArea.top=0;
		psSign->recArea.right=psSign->recArea.left+psSign->sizArea.cx;
		psSign->recArea.bottom=psSign->recArea.top+psSign->sizArea.cy;

		/*
		psSign->hdc=CreateCompatibleDC(0);
		psSign->hBitmap=winBitmapBuilder(psSign->sizArea.cx,psSign->sizArea.cy,24,NULL,NULL);
		SelectObject(psSign->hdc,psSign->hBitmap);
		*/		
		psSign->dgSign=dgCreate(0,psSign->sizArea.cx>10?psSign->sizArea.cx:10,psSign->sizArea.cy>10?psSign->sizArea.cy:10);
		_signRender(psSign);
	}

}
//
// _traceSign() - Traccia la firma
//
static void _traceSign(EHZ_SIGNATURE * psSign) {

	if (psSign->lstPoint->iLength) {
	
		S_SIGNPOINT * psPoint;
		for (lstLoop(psSign->lstPoint,psPoint)) {
			_penWrite(psSign,psPoint->enType,psPoint->pt.x,psPoint->pt.y,false,false);
		}
	
	}

}


static void _signRender(EHZ_SIGNATURE * psSign) {

	RECT rcRect,rcGradient;
	RECTD rcRectd;
	double dBaseLine;

	if (!psSign->dgSign) return;
	rectFillD(&rcRectd,0,0,psSign->sizArea.cx,psSign->sizArea.cy);
	
	rectFill(&rcRect,0,0,psSign->sizArea.cx,psSign->sizArea.cy),
	rectCopy(rcGradient,rcRect); rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;
	dcBoxGradient(	psSign->dgSign->hdc,
					&rcRect,
					&rcGradient,
					AlphaColor(255,sys.Color3DHighLight),
					AlphaColor(255,sys.Color3DLight));


	if (psSign->dBaseLinePos) {
		dBaseLine=(double) psSign->sizArea.cy-(psSign->sizArea.cy*psSign->dBaseLinePos/100);
		if (psSign->dBaseLineWeight) 
			dcLinePlus(psSign->dgSign->hdc,rcRectd.left+10,dBaseLine,rcRectd.right-10,dBaseLine,psSign->acBaseLine,psSign->dBaseLineWeight,psSign->enBaseLineStyle);

		if (psSign->psFontBaseLine&&!strEmpty(psSign->pszBaseLineText))
			dcTextout(	psSign->dgSign->hdc,
						(INT) rcRectd.left+10, (INT) dBaseLine+5,
						psSign->colBaseLineText,-1,
						psSign->psFontBaseLine,
						1,psSign->pszBaseLineText,strlen(psSign->pszBaseLineText),DPL_LEFT);
	}

	_traceSign(psSign);
	

}
//
// _signProcedure()
//
LRESULT CALLBACK _signProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;

//	S_WINSCENA Scena;
	struct OBJ *poj;
	static INT iCounter=0;
	static UINT uiTimer=0;
	static BOOL fReset=TRUE;
//	POINT pSource,pDest;
//	SIZE sSource,sDest;
//	S_SIGNPOINT sSP;
	EHZ_SIGNATURE * psSign;
//	static S_SIGNPEN spOne;
//	static BOOL fPenWrite=FALSE;
	RECT rectClient;
	SIZE sizClient;

	// Inizializzo
	if (message==WM_CREATE) {
		
		CREATESTRUCT * psCreate=(CREATESTRUCT * ) lParam;
		psSign=(EHZ_SIGNATURE *) psCreate->lpCreateParams;
		SetWindowLong(hWnd,GWL_USERDATA,(LONG) psSign);
	
	}
	else 
	{
		psSign=(EHZ_SIGNATURE *) GetWindowLong(hWnd,GWL_USERDATA);
	}


	

	if (psSign) {

		poj=psSign->psObj;

		switch (message)
		{
			case WM_CREATE: 

	//			uiTimer=SetTimer(hWnd,10,50,NULL);
				// Creo un bitmap di dimensioni abbastanza grandi
				GetClientRect(hWnd,&rectClient);
				_signCreate(psSign,sizeCalc(&sizClient,&rectClient));

//				SelectObject(sLI.hdc,sLI.hBitmap);
/*
				psSign->hdc=CreateCompatibleDC(0);
				psSign->hBitmap=winBitmapBuilder(sLI.sizArea.cx,sLI.sizArea.cy,24,NULL,NULL);

				_signImgBuilder(&psSign->sSign,1200,800,"MainSign");
				_(spOne);
				_signReset(psSign,FALSE,0,&psSign->sSign,hWnd,TRUE);
				*/

				break;


			case WM_DESTROY: 
				//_signImgDestroy(&psSign->sSign);
				_signDestroy(psSign);
				break;

			case WM_MOUSEMOVE: 

			  WinMouse((INT16) (poj->px+LOWORD(lParam)),(INT16) (poj->py+HIWORD(lParam)),0xFFFF);
			  if (psSign->bSignRecordLock) break;
			  if (psSign->bPenWrite)
			  {
				_penWrite(	psSign,
							PEN_POINT,
							(short int) LOWORD(lParam),
							(short int) HIWORD(lParam), 
							true,
							true);
			  }
			  break;

			case WM_LBUTTONDOWN:  

				if (psSign->bSignRecordLock) break;
				if (psSign->bEngineLock) break;

				_penWrite(	psSign,
							PEN_DOWN,
							(short int) LOWORD(lParam),
							(short int) HIWORD(lParam),
							true,
							true);
				SetCapture(hWnd);
				break;

			case WM_LBUTTONUP:  
				if (psSign->bSignRecordLock) break;
				if (psSign->bEngineLock) break;

				_penWrite(	psSign,
							PEN_UP,
							(short int) LOWORD(lParam),
							(short int) HIWORD(lParam),
							true,
							true);

				ReleaseCapture();
				break;

			case WM_TIMER: break;

			case WM_SIZE: 
				GetClientRect(hWnd,&rectClient);
				_signCreate(psSign,sizeCalc(&sizClient,&rectClient));
				break;

			case WM_EXITSIZEMOVE: break;

			// Messaggio di cambio cursore
			case WM_SETCURSOR: break;

			// Intercettazione degli oggetti Windows
			case WM_COMMAND: break;
			case WM_PAINT: 
				hdc=BeginPaint(hWnd,&ps);
				dgCopyDC(psSign->dgSign,&ps.rcPaint,ps.rcPaint.left,ps.rcPaint.top,hdc);
				EndPaint(hWnd,&ps);
				return 0;

/*
//				WinDirectDC(hdc,&Scena,"1");
				
				// Calcolo sezione di stampa
				pSource.x=pDest.x=ps.rcPaint.left; 
				pSource.y=pDest.y=ps.rcPaint.top;
				sSource.cx=sDest.cx=ps.rcPaint.right-ps.rcPaint.left+1; 
				sSource.cy=sDest.cy=ps.rcPaint.bottom-ps.rcPaint.top+1; 
				 
				if (StretchDIBits(hdc, 
									  // Coordinate e dimensioni di stampa a video
									  pDest.x,pDest.y,
									  sDest.cx,sDest.cy,

									  // Coordinate e dimensioni di lettura nel sorgente
									  pSource.x,
									  pSource.y+sSource.cy+1,
									  sSource.cx,-sSource.cy,
									  psSign->sSign.lpbSignBitMap,
									  (BITMAPINFO *) &psSign->sSign.ImgHead.bmiHeader,
									  DIB_RGB_COLORS, 
									  SRCCOPY) == GDI_ERROR) {dispx("StretchDIBits Failed");}

//				WinDirectDC(0,&Scena,"1");
*/
			}  // switch message
	}
	return(DefWindowProc(hWnd, message, wParam, lParam));
} 

static void _addPoint(EHZ_SIGNATURE * psSign,S_SIGNPOINT * psPoint) {
	
	psPoint->iClock=clock();
	lstPush(psSign->lstPoint,psPoint);

}
/*
void _penServer(EHZ_SIGNATURE * psSign,INT enMess,LONG info,void *str)
{
	S_SIGNPOINT *lpSP;
	switch (enMess)
	{
		case WS_OPEN:	
			// DMIOpen(&psSign->dmiPenPoint,RAM_AUTO,1000000,sizeof(S_SIGNPOINT),"SignPoint");
			break;

		case WS_ADD:
			lpSP=str;
			// dispx("%d) %d - %d-%d        ",dmiPenPoint.Num,lpSP->iClock,lpSP->p.x,lpSP->p.y);
			lpSP->iClock=clock();
			DMIAppend(&psSign->dmiPenPoint,lpSP);
			break;

		case WS_CLOSE:
			// if (psSign->dmiPenPoint.Hdl>0) DMIClose(&psSign->dmiPenPoint,"SignPoint");
			break;
	}
}
*/

/*
//   ---------------------------------------------
//   ³ SignDiskWrite
//   ³                                           
//   ³         by Ferrà Art & Technology 1993-2004
//   ---------------------------------------------
CHAR *lpFolderArc="c:\\SignScan\\Archive\\";



CHAR *LToken(CHAR *lpScan, CHAR *lpChars,CHAR *lpLastSep);

BOOL SignDiskRead(CHAR *lpFile)
{
	FILE *ch;
	CHAR szFile[255];
	CHAR szServ[255];
	CHAR szChar[2];
	S_SIGNPOINT sSP;
//	INT a;
	INT iNum;
	BOOL fLoad=FALSE;
	CHAR *lpVar;

	sprintf(szFile,"%s%s.txt",lpFolderArc,lpFile);
	ch=fopen(szFile,"rb"); if (!ch) return FALSE;
	dmiPenPoint.Num=0;

	while (TRUE)
	{
		if (!fgets(szServ,sizeof(szServ)-1,ch)) break;

		if (fLoad&&*szServ=='>')
		{
			INT i=0;
			lpVar=LToken(szServ+1,",",szChar);
			do
			{
				switch (i)
				{
					case 0: _(sSP); sSP.fType=atoi(lpVar); break;
					case 1: sSP.p.x=atoi(lpVar); break;
					case 2: sSP.p.y=atoi(lpVar); break;
					case 3: sSP.iClock=atoi(lpVar); break;
					case 4: DMIAppend(&dmiPenPoint,&sSP); break;
				}
				i++;
				
			} while (lpVar=LToken( NULL, ",\r\n", szChar));
			
			//break;
		}

		if (!memcmp(szServ,"SAMPLES=",8))
		{
			iNum=atoi(szServ+8);
			fLoad=TRUE;
		}

		if (!memcmp(szServ,"GIORNO=",7)) sSetup.iGiorno=atoi(szServ+7);
		if (!memcmp(szServ,"MESE=",5)) sSetup.iMese=atoi(szServ+5);
		if (!memcmp(szServ,"ANNO=",5)) sSetup.iAnno=atoi(szServ+5);
	}
	fclose(ch);
	return TRUE;
}

static CHAR *lpLastStr=NULL;
CHAR *LToken(CHAR *lpScan, CHAR *lpChars,CHAR *lpLastSep)
{
	CHAR *lp,*lpt;
	CHAR *lpStart;
	if (lpScan) lpLastStr=lpScan;

	if (!lpChars) return lpLastStr; // Attuale posizione
	lp=lpStart=lpLastStr;
	
	// Ricerco uno dei caratteri di separazione
	for (;*lp;lp++)
	{
		lpt=strchr(lpChars,*lp); // Cerco il carattere nella lpChars
		if (lpt)
		{
			*lpLastSep=*lpt; *lp=0; lpLastStr=lp+1;
			return lpStart;
		}
	}
	return NULL;
}
*/
/*
void _signImgBuilder(S_SIGNAREA *lps,INT cx,INT cy,CHAR *lpName)
{
	INT iMemoSize;
	
	memset(lps,0,sizeof(S_SIGNAREA));
	
	lps->rArea.left=0;
	lps->rArea.top=0;
	lps->rArea.right=lps->rArea.left+cx-1;
	lps->rArea.bottom=lps->rArea.top+cy-1;

	lps->ImgHead.enType=IMG_JPEG;
//	lps->ImgHead.lSize=0;
	lps->ImgHead.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);//??
	lps->ImgHead.bmiHeader.biWidth=cx;
	lps->ImgHead.bmiHeader.biHeight=cy; // forse * -1
	lps->ImgHead.bmiHeader.biPlanes=1;
	lps->ImgHead.bmiHeader.biBitCount=8*3;// bit colore
	lps->ImgHead.bmiHeader.biCompression=BI_RGB;// Non compresso
	lps->ImgHead.bmiHeader.biSizeImage=0;
	lps->ImgHead.bmiHeader.biClrUsed=0;// Colori usati 0=Massimo
	lps->ImgHead.bmiHeader.biClrImportant=0;// 0=Tutti i colori importanti

	lps->ImgHead.linesize=lps->ImgHead.bmiHeader.biWidth*3;

	// Allineo a 32bit
	lps->ImgHead.linesize=((lps->ImgHead.linesize+3)>>2)<<2;

	iMemoSize=sizeof(IMGHEADER)+(lps->ImgHead.bmiHeader.biHeight*lps->ImgHead.linesize);
	lps->hdlSign=memoAlloc(RAM_AUTO,iMemoSize,lpName);
	if (lps->hdlSign<0) ehExit("No memory");
	lps->lpbSignStruct=memoLock(lps->hdlSign); 
	lps->ImgHead.Offset=sizeof(lps->ImgHead);
	memset(lps->lpbSignStruct,255,iMemoSize);
	memcpy(lps->lpbSignStruct,&lps->ImgHead,sizeof(lps->ImgHead));
	lps->lpbSignBitMap=lps->lpbSignStruct+lps->ImgHead.Offset;
}


void _signImgDestroy(S_SIGNAREA *lps)
{
	memoFree(lps->hdlSign,"Sign");
}
*/

static void		_this_reset(void *this) {

	EHZ_SIGNATURE * psSign=this;
	lstClean(psSign->lstPoint);
	_signRender(psSign);
	InvalidateRect(psSign->wndSign,NULL,false);

}

static void		_this_refresh(void *this) {

	EHZ_SIGNATURE * psSign=this;
	_signRender(psSign);
	InvalidateRect(psSign->wndSign,NULL,false);

}


static void		_this_setFont(void *this,CHAR * pszFont) {

	EHZ_SIGNATURE * psSign=this;

	cssFont(pszFont,&psSign->psFontBaseLine);
	_signRender(psSign);
	InvalidateRect(psSign->wndSign,NULL,false);

}

//
// _this_writeSign() > Ritorna FALSE se riesce
//
static BOOL _this_writeSign(void * this, CHAR * pszFileName)
{
	EHZ_SIGNATURE * psSign=this;
	S_SIGNPOINT * psPoint;
	EH_LST lst;
	CHAR * psz;
	BOOL bRet=false;
	
	lst=lstNew();
	lstPushf(lst,"{ver:'1.0', penSize:%.2f, penColor:'%x', \n",
				psSign->sizPen.cx,			// Dimensioni della penna
				psSign->acPen);
		
	lstPush(lst,"arLine:[");

	for (lstLoop(psSign->lstPoint,psPoint)) {
		
		if (psSign->lstPoint->psFirst!=psSign->lstPoint->psCurrent) lstPush(lst,", ");
		lstPushf(lst,
				"{t:%d, clk:%d, x:%d, y:%d}\n",
				psPoint->enType,
				psPoint->iClock,
				psPoint->pt.x,
				psPoint->pt.y);
		

	}
	lstPushf(lst,"]}");
	psz=lstToString(lst,"","","");
	bRet=fileStrWrite(pszFileName,psz);

	lstDestroy(lst);
	ehFree(psz);

	return bRet;

}

//
// _this_readSign() > Ritorna FALSE se riesce
//
static BOOL _this_readSign(void * this, CHAR * pszFileName)
{
	EHZ_SIGNATURE * psSign=this;

	EH_JSON * psJson;
	CHAR * psz;
	INT a,iPoint;
	S_SIGNPOINT sPoint;

	psz=fileStrRead(pszFileName); if (!psz) return true; // Il file non esiste
	psJson=jsonCreate(psz); ehFree(psz);
	if (psJson) {
		lstClean(psSign->lstPoint);
		iPoint=atoi(jsonGet(psJson,"arLine.length"));
		for (a=0;a<iPoint;a++) {
			_(sPoint);
			sPoint.enType=atoi(jsonGetf(psJson,"arLine[%d].t",a));
			sPoint.iClock=atoi(jsonGetf(psJson,"arLine[%d].clk",a));
			sPoint.pt.x=atoi(jsonGetf(psJson,"arLine[%d].x",a));
			sPoint.pt.y=atoi(jsonGetf(psJson,"arLine[%d].y",a));
			lstPush(psSign->lstPoint,&sPoint);
		}

		jsonDestroy(psJson);
	}
	psSign->refresh(psSign);
	return false;
}	


//
//
//
static void _this_getSignRect(void * this,RECT * psRect) {

	EHZ_SIGNATURE * psSign=this;
	S_SIGNPOINT * psPoint;
	RECT rcSign;
	BOOL bFirst=true;
	_(rcSign);

	for (lstLoop(psSign->lstPoint,psPoint)) {
		if (psPoint->enType) continue;

		if (bFirst) {
			rcSign.left=rcSign.right=psPoint->pt.x;
			rcSign.top=rcSign.bottom=psPoint->pt.y;
			bFirst=false;
		}
		else {
		
			if (psPoint->pt.x<rcSign.left) rcSign.left=psPoint->pt.x;
			if (psPoint->pt.x>rcSign.right) rcSign.right=psPoint->pt.x;
			if (psPoint->pt.y<rcSign.top) rcSign.top=psPoint->pt.y;
			if (psPoint->pt.y>rcSign.bottom) rcSign.bottom=psPoint->pt.y;
		
		}
	}
	rcSign.left-=(INT) ((psSign->sizPen.cx/2)+1);
	rcSign.right+=(INT) ((psSign->sizPen.cx/2)+1);
	rcSign.top-=(INT) ((psSign->sizPen.cy/2)+1);
	rcSign.bottom+=(INT) ((psSign->sizPen.cy/2)+1);
	memcpy(psRect,&rcSign,sizeof(RECT));
}


//
// _this_writeImage() > Ritorna FALSE se riesce
//
static BOOL _this_writeImage(void * this, UTF8 * pszFileName, EN_FILE_TYPE enFileType, INT cx,INT cy) {

	EHZ_SIGNATURE * psSign=this;
	EH_DG * dgSign;
	RECT rcSign;
	SIZE sizSign;
	double dScale;
	EHZ_SIGNATURE sClone;
	EH_IMG	imgSave;
	RECTD	rcd;
	CHAR szFileName[500], * psz;
	RECT rcRect,rcGradient;

	//
	// Estraggo dimensioni firma
	//
	_this_getSignRect(psSign,&rcSign);
	sizeCalc(&sizSign,&rcSign);

	//
	// Calcolo dimensioni immagine
	//

	if (!cx&&!cy) {cx=sizSign.cx; cy=sizSign.cy;}
	else if (!cy) {cy=cx*sizSign.cy/sizSign.cx;}
	else if (!cx) {cx=cy*sizSign.cx/sizSign.cy;}

	//
	// Calcolo fattore di scala
	//
	dScale = min ((double) cx / sizSign.cx, (double) cy / sizSign.cy) ;

	//
	// Creo bitmap in memoria (ok);
	//
	dgSign=dgCreate(1,cx,cy);
//	
	rectFill(&rcRect,0,0,cx,cy);
	rectCopy(rcGradient,rcRect); rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;
	if (enFileType==IMG_JPEG) 
		dcBoxFill( dgSign->hdc,rectFillD(&rcd,0,0,cx,cy),0,0,rgba(1,1,1,1));
		else
		dcBoxFill( dgSign->hdc,rectFillD(&rcd,0,0,cx,cy),0,0,rgba(1,1,1,0));
/*
	dcBoxGradient(	dgSign->hdc,
					&rcRect,
					&rcGradient,
					AlphaColor(255,sys.Color3DHighLight),
					AlphaColor(255,sys.Color3DShadow));

	dcTextout(	dgSign->hdc,
				10,10,
				psSign->colBaseLineText,-1,
				psSign->psFontBaseLine,
				1,psSign->pszBaseLineText,strlen(psSign->pszBaseLineText),DPL_LEFT);
  */              
	//
	// Renderizzo nel bitmap la firma
	//

	memcpy(&sClone,psSign,sizeof(EHZ_SIGNATURE));
	sClone.sizArea.cx=cx; sClone.sizArea.cy=cy;
	sClone.recArea.left=sClone.recArea.top=0;
	sClone.recArea.right=sClone.recArea.left+cx;
	sClone.recArea.bottom=sClone.recArea.top+cy;
	sClone.dgSign=dgSign;
	sClone.dScale=dScale;
	sClone.ptOfs.x=-rcSign.left;
	sClone.ptOfs.y=-rcSign.top;
 	_traceSign(&sClone);
/*
	{
		HDC hdc=GetDC(NULL);
		dgCopyDC(sClone.dgSign,NULL,0,0,hdc);
		ReleaseDC(NULL,hdc);
	}
*/	
	//
	// Trasformo il bitmap in IMG
	// Salvo il file su disco, .. facile ;-)
	//
	strcpy(szFileName,pszFileName); 
	psz=fileExt(szFileName); if (psz) *(psz-1)=0;
	switch (enFileType) {
	
		case IMG_JPEG:
			imgSave=BitmapToImg(sClone.dgSign->hBitmap,false,NULL,IMG_PIXEL_RGB);
			/*
	{
		HDC hdc=GetDC(NULL);
		dcImageShow(hdc,0,0,0,0,imgSave);
		ReleaseDC(NULL,hdc);
	}
	*/
			strcat(szFileName,".jpg");
			if (!JPGSaveFile(szFileName,imgSave,90)) ehError();
			break;

		case IMG_PNG:
			imgSave=BitmapToImg(sClone.dgSign->hBitmap,false,NULL,IMG_PIXEL_RGB_ALPHA);
			strcat(szFileName,".png");
			if (!PNGSaveFile(szFileName,imgSave,90)) ehError();
			break;

		default:
			ehExit("Formato del file non gestito");
	
	}
	IMGDestroy(imgSave);
	dgDestroy(dgSign);
	
	return false;
}
