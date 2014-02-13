//   -----------------------------------------------------------
//   | ehzBarFrame
//   | Gestisce una barra di separazione e movimento
//   |                                              
//   |										by Ferr� Srl 2005
//   -----------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehzBarFrame.h"

static HWND _LCreateStaticText(HINSTANCE hInstance, HWND hwndParent);
static void SwitchView(HWND hwndListView, DWORD dwView);
static LRESULT CALLBACK funcStaticText(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static void _moveBar(EHZ_BARFRAME * psBF);
static void _objRebuild(EHZ_BARFRAME * psBF);

//
// ehzStaticText()
//
void * ehzBarFrame(struct OBJ * psObjCaller,EN_MESSAGE cmd,LONG info,void *ptr)
{
	EH_DISPEXT *	psExt=ptr;
	EHZ_BARFRAME * psBF;
	EH_EVENT *psEvent;
	WCHAR * pwc;
	DWORD * pdw;
	SIZE siz,sizText;
	EH_JSON * psJson;
	CHAR * psz;
	RECT rcGradient;
	
	//INT y;

	if (!psObjCaller) return NULL; // 

	psBF=psObjCaller->pOther;
	switch(cmd)
	{
		case WS_INF: return psBF;

		case WS_CREATE: 
			psBF=psObjCaller->pOther=ehAllocZero(sizeof(EHZ_BARFRAME));
			psBF->iWin=sys.WinWriteFocus;
			psBF->hWnd=WindowNow();
			psBF->iMaxType=1;
			psBF->iMax=98; // 98%
			psBF->psObj=psObjCaller;
			if (!strCmp(psBF->psObj->text,"V")) psBF->bVert=true;
			if (!psBF->bVert)
				psBF->psFont=fontCreateEx("Arial",9,0,0,STYLE_NORMAL,0,0,0);
				else
				psBF->psFont=fontCreateEx("Arial",9,0,0,STYLE_NORMAL,0,900,0);
			break;

		case WS_DESTROY:
			if (psBF->idMgz) mgzRemove(psBF->idMgz);
			fontDestroy(psBF->psFont,true);
			ehFreePtr(&psObjCaller->pOther);
			break;

		case WS_PROCESS:
			psJson=jsonCreate(ptr);			

			psz=jsonGet(psJson,"min"); if (psz) psBF->iMin=atoi(psz);
			psz=jsonGet(psJson,"minType"); if (psz) psBF->iMinType=atoi(psz);
			psz=jsonGet(psJson,"max"); if (psz) psBF->iMax=atoi(psz);
			psz=jsonGet(psJson,"maxType"); if (psz) psBF->iMaxType=atoi(psz);

			jsonDestroy(psJson);
			break;

		case WS_EVENT:

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
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			if (psBF->idMgz) mgzRemove(psBF->idMgz);
			psBF->idMgz=Amgz_on(psExt->rClientArea.left,psExt->rClientArea.top,psExt->rClientArea.right,psExt->rClientArea.bottom,3,0,0,"W:HAND");
			break;

		case WS_REALGET:
			pdw=(DWORD *) ptr;
			*pdw=psBF->iPos;
			break;

		case WS_REALSET: 
			psBF->iPos=info; 
			_objRebuild(psBF);
			break;

		case WS_DISPLAY: 
			sizeCalc(&siz,&psExt->rClientArea);
			pwc=utfToWcs("●●●●");
			dcFontGetSize(psExt->hdc,2,pwc,wcslen(pwc),psBF->psFont,&sizText,NULL);
			
			if (!psBF->bVert) { // Barra orizzontale
				INT x=(psExt->rClientArea.left+siz.cx/2-sizText.cx/2);
				INT y=psExt->rClientArea.top+(siz.cy/2-sizText.cy/2);

				rectCopy(rcGradient,psExt->rClientArea); rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;
				if (psBF->bAction)
					dcBoxp(psExt->hdc,&psExt->rClientArea,sys.Color3DLight);
				else if (psBF->bMouseOver) 
					dcBoxGradient(psExt->hdc,&psExt->rClientArea,&rcGradient,AlphaColor(255,sys.ColorBackGround),AlphaColor(255,sys.Color3DLight));
					else
					dcBoxp(psExt->hdc,&psExt->rClientArea,sys.ColorBackGround);

					//dcBoxGradient(psExt->hdc,&psExt->rClientArea,&rcGradient,AlphaColor(255,sys.Color3DLight),AlphaColor(255,sys.ColorBackGround));
				dcTextout(psExt->hdc,x,y-2,sys.Color3DHighLight,-1,psBF->psFont,2,pwc,-1,DPL_CENTER);
				dcTextout(psExt->hdc,x-1,y-3,sys.Color3DShadow,-1,psBF->psFont,2,pwc,-1,DPL_CENTER);
//				dcTextout(psExt->hdc,x,y,sys.Color3DHighLight,-1,psBF->psFont,2,pwc,-1,DPL_CENTER);
				dcTextout(psExt->hdc,x,y+2,sys.Color3DHighLight,-1,psBF->psFont,2,pwc,-1,DPL_CENTER);
				dcTextout(psExt->hdc,x-1,y+1,sys.Color3DShadow,-1,psBF->psFont,2,pwc,-1,DPL_CENTER);
			} else {
			
				INT x=psExt->rClientArea.left+(siz.cx/2)+4;///2;//siz.cx/2-psBF->psFont->iHeight/2;
				INT y=psExt->rClientArea.top+(siz.cy/2)-(sizText.cx/2);

				rectCopy(rcGradient,psExt->rClientArea); rcGradient.top=rcGradient.bottom; rcGradient.left--; rcGradient.right++;
				if (psBF->bAction)
					dcBoxp(psExt->hdc,&psExt->rClientArea,sys.Color3DLight);
				else if (psBF->bMouseOver) 
					dcBoxGradient(psExt->hdc,&psExt->rClientArea,&rcGradient,AlphaColor(255,sys.ColorBackGround),AlphaColor(255,sys.Color3DLight));
					else
					dcBoxp(psExt->hdc,&psExt->rClientArea,sys.ColorBackGround);

				dcTextout(psExt->hdc,x,y,sys.Color3DHighLight,-1,psBF->psFont,2,pwc,-1,DPL_CENTER);
				dcTextout(psExt->hdc,x-1,y-1,sys.Color3DShadow,-1,psBF->psFont,2,pwc,-1,DPL_CENTER);
			
			}
			ehFree(pwc);
			break;

	}
	return NULL;
}


static void _moveBar(EHZ_BARFRAME * psBF)
{
	INT iCurrent;
	POINT pStart;
	POINT pMouse;
	INT yOriginal;
	EH_EVENT sEvent;
	BOOL bPress;
	//HWND hWnd=WindowNow();
	INT iPos=0;
	EH_OBJ * psObj=psBF->psObj;
	HWND hWnd=psBF->hWnd;

	if (psBF->idMgz) {mgzRemove(psBF->idMgz); psBF->idMgz=0;}
//	SetCapture(hWnd); 
	sys.pObjFocus=NULL;
	if (!psBF->bVert)
		yOriginal=psObj->sClientRect.top; 
		else
		yOriginal=psObj->sClientRect.left; 
	
	iCurrent=-1;

	//mouse_graph(0,0,"W:HAND");
	GetCursorPos(&pMouse); 
	pStart.y=pMouse.y;
	pStart.x=pMouse.x;
	psBF->bAction=true;
	obj_RefreshSolo(psObj->nome,true,true); // Visualizzo la nuova posizione
//	SetCapture(hWnd); 
	while (true) {

		WindowsMessageDispatch();
		ehWaitEvent(hWnd,
					false,
					true,
					&sEvent,
					true);		

		GetCursorPos(&pMouse);
		bPress=GetKeyState(VK_LBUTTON) & 0x80;
		if (!psBF->bVert) 
			iPos=yOriginal+(pMouse.y-pStart.y);
			else
			iPos=yOriginal+(pMouse.x-pStart.x);

		//if (sEvent.iEvent)  
		// if (!sEvent.iEvent) break;
		//	printf(">Event: %d (%d)" CRLF,sEvent.iEvent,bPress);
		if ((!psBF->bVert&&iCurrent!=pMouse.y)||
			(psBF->bVert&&iCurrent!=pMouse.x))

		{
			if (iCurrent!=-1) {
				psBF->iPos=iPos; 
				_objRebuild(psBF);
				// Notifico lo spostamento a funzione esterna

			}
			iCurrent=psBF->bVert?pMouse.x:pMouse.y;
		}
		if (!bPress) break;
	//	if (sEvent.iEvent==EE_LBUTTONUP) break;
	}
	winSetFocus(hWnd);
//	ReleaseCapture();
	psBF->bAction=false;
	psBF->iPos=iPos; _objRebuild(psBF);
	obj_RefreshSolo(psObj->nome,true,true); // Visualizzo la nuova posizione
	obj_addevent(psObj->nome);

}

//
// _objRebuild()
//

static void _objRebuild(EHZ_BARFRAME * psBF)
{
	RECT rc;
	SIZE siz;
	INT iMin,iMax,iSize;
	EH_OBJ * psObj=psBF->psObj;
	BOOL bTouch;
	
	if (!psBF->psAm&&WIN_info[psBF->iWin].arsAutoMove) {
		psBF->psAm=WIN_info[psBF->iWin].arsAutoMove;	
	}

	GetClientRect(psBF->hWnd,&rc);
	sizeCalc(&siz,&rc);

	// Calcolo del Minimo/massimo
	iSize=(psBF->bVert?siz.cx:siz.cy);
	iMin=psBF->iMin; if (psBF->iMinType==1) iMin=iSize*iMin/100; else if (psBF->iMinType==2) iMin=iSize-iMin;
	iMax=psBF->iMax; if (psBF->iMaxType==1) iMax=iSize*iMax/100; else if (psBF->iMaxType==2) iMax=iSize-iMax; 
	if (iMin<0) iMin=0;
	if (iMax>iSize) iMax=iSize;

// 	printf("%s:[%d,%d]:%d" CRLF,psObj->nome,iMin,iMax,psBF->iPos);
	// Controllo limiti
	if (psBF->iPos<iMin) psBF->iPos=iMin;
	if (psBF->iPos>iMax) psBF->iPos=iMax;

	bTouch=false;
	if (psBF->bVert&&psBF->iPos!=psObj->px) {psObj->px=psBF->iPos; bTouch=true;}
	if (!psBF->bVert&&psBF->iPos!=psObj->py) {psObj->py=psBF->iPos; bTouch=true;}

	if (bTouch) {

		// Aggiorno Oggetti
		obj_RefreshSolo(psObj->nome,true,true); // Visualizzo la nuova posizione
		if (psBF->psAm) {
			obj_AutoMoveAssignSolo(psBF->iWin,psBF->psAm,AMT_OBJ,psObj->nome); // Notifico a AUTOMOVE la nuova posizione
			objAutoMoveEngine(psBF->iWin,true); // Rigenero oggetti collegati
		}
	}
}
 