//   ---------------------------------------------
//   | ZCMP_ISearch                 
//   | ZoneComponent Input Search
//   |                                              
//   | Gestisce un campo di input per effettuare delle ricerche
//   | in una oggetto ZONAP                        
//   |                                              
//   |                                              
//   |							by Ferrà Srl 2008
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ZCMP_ISearch.h"

// Aggiungere nello script.rc:

//
// ISEARCH            LIC			"C:\\Easyhand\\EhTool\\resource\\ISearch.lic"
//
#define ISEARCH_EDIT_CLASS L"ehzISearch"

static struct {
	BOOL bReady;
	WNDPROC OldEditProc;
} sApp={0};


static void _EditCreate(EH_ISEARCH *psSearch);
static void _EditDestroy(EH_ISEARCH *psSearch);
static void _EditFocus(EH_ISEARCH *psSearch);
static void _EditBlur(EH_ISEARCH *psSearch);
static LRESULT CALLBACK _ISearchEditProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static void _EditRepaint(EH_ISEARCH *psSearch);
static void _areaCalc(EH_ISEARCH *psSearch);
//static BOOL bFirst=TRUE;
static void LCleanControl(EH_ISEARCH *psSearch);


//
// metodi pubblici
//
static BOOL		_this_set(void *this,void *pszValue,BOOL bEvent);


static void _iSearchInitialize(void) {

	WNDCLASSW wndClass;
	GetClassInfoW(sys.EhWinInstance,L"Edit",&wndClass);
	wndClass.hInstance=sys.EhWinInstance;
	wndClass.lpszClassName=ISEARCH_EDIT_CLASS;
	sApp.OldEditProc=wndClass.lpfnWndProc;
	wndClass.lpfnWndProc=_ISearchEditProc;
	RegisterClassW(&wndClass);

	lic_fromResource("ISEARCH","LIC",2);
	sApp.bReady=TRUE;

}

//
// ehzISearch()
//
void * ehzISearch(struct OBJ *objCalled,INT cmd,LONG info,void *ptr)
{
	EH_DISPEXT *psDE;
	EH_EVENT *psEvent;
	EH_ISEARCH *psSearch;
	RECT rArea;
	POINT sText;
	HRGN hrgn;
	WCHAR *pw;
	HDC hdc;
	RECTD rcdClient,rcdArea;
	
	if (!sApp.bReady) _iSearchInitialize();

	psSearch=objCalled->pOther;
	switch(cmd)
	{
		//
		// Creazione dell'oggetto
		//
		case WS_CREATE:
			objCalled->pOther=ehAllocZero(sizeof(EH_ISEARCH));
			psSearch=objCalled->pOther;
			psSearch->lpObj=objCalled;
			psSearch->iInputSize=1024;
			psSearch->pwInputValue=ehAllocZero((psSearch->iInputSize*2)+4);
			psSearch->pInputValue=ehAllocZero(psSearch->iInputSize+1);
			psSearch->iRoundCorner=18;
			psSearch->bLightFocus=TRUE; // Verde sotto quando in focus
			psSearch->bTextEditEnable=TRUE;
			psSearch->colBackground=sys.ColorWindowBack;
			psSearch->colOutline=sys.ColorSelectBack;

			psSearch->set=_this_set;
			_areaCalc(psSearch);
			_EditCreate(psSearch);
			break;

		case WS_DESTROY: 
			// Se ho l'input aperto devo liberare la finistra
			_EditDestroy(psSearch);
			ehFreePtr(&psSearch->pwInputValue);
			ehFreePtr(&psSearch->pInputValue);
			ehFreePtr(&psSearch->pEmptyMessage);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_INF: return psSearch;

		case WS_SET_TEXT:
		case WS_REALSET:
			strcpy(psSearch->pInputValue,ptr);
			pw=strToWcs(psSearch->pInputValue);
			wcscpy(psSearch->pwInputValue,pw);
			ehFree(pw);
			LCleanControl(psSearch);
			if (cmd==WS_REALSET) _EditRepaint(psSearch);//obj_vedisolo(psSearch->lpObj->nome);
			break;

		case WS_EVENT:

			psEvent=(EH_EVENT *) ptr;
			switch (psEvent->iEvent)
			{
				//
				// Controllo del click sinistro
				//
				case EE_LBUTTONDOWN:

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
					return "FOCUS";

				case EE_FOCUS:
					
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
					break;
			}
			break;

 		case WS_OPEN: break;
//		case WS_EXTFUNZ: break;
		case WS_SETFLAG: break;
		case WS_CLOSE: break; // Distruzione

		case WS_DO: // Spostamento / Ridimensionamento | //MoveWindow(objCalled->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
			_areaCalc(psSearch);
			break;

		case WS_DISPLAY: 
			_areaCalc(psSearch);
			psDE=ptr;
			memcpy(&rArea,&psDE->rClientArea,sizeof(RECT));
			rArea.left+=1; rArea.top+=1; rArea.bottom-=1; rArea.right-=1;
			hdc=psDE->hdc;
			rectToD(&rcdClient,&psDE->rClientArea);
			//rcdClient.left+=4; rcdClient.right-=4;
			rectToD(&rcdArea,&rArea);
			if (psSearch->iShadow) {

				RECT rec;
				INT a;
				rectCopy(rec,psDE->rClientArea);
//				TRectRound(&psDE->rClientArea,sys.Color3DShadow,psSearch->cBackground,psSearch->iRoundCorner,psSearch->iRoundCorner);
				dcRectRoundEx(	hdc,
								&rcdArea,
								AlphaColor(255,sys.Color3DShadow),
								AlphaColor(255,psSearch->colBackground),
								psSearch->iRoundCorner,
								psSearch->iRoundCorner,
								2);

				for (a=0;a<psSearch->iShadow;a++) {
					
					// x:100=a:psSearch->iShadow
					rec.top=psDE->rClientArea.top+a;
					rec.left=psDE->rClientArea.left+(a/5);
					rec.right=psDE->rClientArea.right-(a/5);
					TRectRound(	&rec,
								ColorFusion(ColorLum(sys.Color3DShadow,-50),sys.ColorBackGround,a*100/psSearch->iShadow),
								-1,
								psSearch->iRoundCorner,
								psSearch->iRoundCorner);
				}
				TRectRound(&psDE->rClientArea,sys.Color3DShadow,-1,psSearch->iRoundCorner,psSearch->iRoundCorner);

			}

			else if (psSearch->bLightFocus)
			{
				if (psSearch->bFocus) 
				{
					//TRectRound(&psDE->rClientArea,sys.arsColor[12],sys.arsColor[12],psSearch->iRoundCorner,psSearch->iRoundCorner);
					//TRectRound(&rArea,sys.Color3DShadow,psSearch->cBackground,psSearch->iRoundCorner,psSearch->iRoundCorner);

						dcRectRoundEx(	hdc,
								&rcdClient,
								AlphaColor(80,psSearch->colOutline),
								AlphaColor(0,sys.ColorBackGround),
								psSearch->iRoundCorner,
								psSearch->iRoundCorner,
								1.5);

						dcRectRoundEx(	hdc,
								&rcdClient,
								AlphaColor(120,psSearch->colOutline),
								AlphaColor(0,sys.ColorBackGround),
								psSearch->iRoundCorner,
								psSearch->iRoundCorner,
								1.0);

						dcRectRoundEx(hdc,
								&rcdArea,
								AlphaColor(50,sys.Color3DShadow),
								AlphaColor(255,psSearch->colBackground),
								psSearch->iRoundCorner,
								psSearch->iRoundCorner,
								.5);
				}
				else
				{
					//TRectRound(&psDE->rClientArea,sys.ColorBackGround,sys.ColorBackGround,psSearch->iRoundCorner,psSearch->iRoundCorner);
					//TRectRound(&rArea,sys.Color3DShadow,psSearch->cBackground,psSearch->iRoundCorner,psSearch->iRoundCorner);
						dcRectRoundEx(	hdc,
								&rcdClient,
								AlphaColor(255,sys.Color3DShadow),
								AlphaColor(255,sys.ColorBackGround),
								psSearch->iRoundCorner,
								psSearch->iRoundCorner,
								1);

						dcRectRoundEx(hdc,
								&rcdArea,
								AlphaColor(70,sys.Color3DShadow),
								AlphaColor(255,psSearch->colBackground),
								psSearch->iRoundCorner,
								psSearch->iRoundCorner,
								1);

				}
			}
			else
			{
				TRectRound(&psDE->rClientArea,sys.Color3DShadow,psSearch->colBackground,psSearch->iRoundCorner,psSearch->iRoundCorner);
			}

			
			switch (psSearch->iLayOut)
			{
				case ZIS_LENS_SELECT:
						dcIcone(hdc,psDE->rClientArea.left+psSearch->iPaddingX+1,psDE->rClientArea.top+((psDE->ly-17)>>1),"LENTES");
						break;
				case ZIS_LENS:
						dcIcone(hdc,psDE->rClientArea.left+psSearch->iPaddingX+2,psDE->rClientArea.top+((psDE->ly-14)>>1),"LENTE14");
						break;
			}

			sText.x=psSearch->sRectInput.left-psSearch->lpObj->sClientRect.left+3;
			sText.y=psSearch->sRectInput.top-psSearch->lpObj->sClientRect.top;
			/*
			Aclip_set(sText.x,sText.y,
					sText.x+20,//psSearch->sRectInput.right-psSearch->lpObj->sClientRect.left-5,
					psSearch->sRectInput.bottom-psSearch->lpObj->sClientRect.top,
					"ISRCH");
*/
			hrgn = CreateRectRgn(sText.x,sText.y,
								 psSearch->sRectInput.right-psSearch->lpObj->sClientRect.left-5,
								 psSearch->sRectInput.bottom-psSearch->lpObj->sClientRect.top);
			SelectClipRgn(hdc, hrgn);
			if (*psSearch->pInputValue)
			{
				dcDispf(hdc,sText.x,sText.y,sys.ColorWindowText,-1,STYLE_NORMAL,"#Arial",psSearch->iFontHeightReal,psSearch->pInputValue,0);
			}
			else
			{
				if (psSearch->pEmptyMessage) dcDispf(hdc,sText.x,sText.y,sys.Color3DShadow,-1,STYLE_NORMAL,"#Arial",psSearch->iFontHeightReal,psSearch->pEmptyMessage,0);
			}
			
			DeleteObject(hrgn);
			SelectClipRgn(hdc, NULL);

			if (psSearch->bButtonClean) 
			{
				dcIcone(hdc,psDE->rClientArea.right-(17+psSearch->iPaddingX),psDE->rClientArea.top+((psDE->ly-16)>>1),"CCLOSE");
			}

			break;
	}
	return NULL;
}


//
// _EditCreate()
//
static void _EditCreate(EH_ISEARCH *psSearch)
{
	DWORD dwExStyle=0;
	DWORD Style;
	INT Count=0;

	_areaCalc(psSearch);

	//
	//  Calcolo le dimensioni del campo di input
	//
	
	Style=WS_CHILDWINDOW|ES_LEFT|ES_AUTOHSCROLL;

	switch (psSearch->iTypeInput)
	{
		case ALFU: Style = (Style|ES_UPPERCASE); break;
		case APSW: Style = (Style|ES_PASSWORD); break;
		case NUME: //Style = (Style|ES_NUMBER); 
			break;

		case NOTE:
			Style = (Style|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL);
			break;
	}

	psSearch->hwndInput=CreateWindowExW(dwExStyle,
										ISEARCH_EDIT_CLASS,
										L"",//lpBuffer,
										Style,
										psSearch->sRectInput.left,psSearch->sRectInput.top,
										psSearch->sSizeInput.cx, psSearch->sSizeInput.cy,
										WindowNow(),
										(HMENU) ID_ISEARCH,
										sys.EhWinInstance,
										0);
	if (psSearch->hwndInput==NULL) return;
	SetWindowLong(psSearch->hwndInput,GWL_USERDATA,(LONG) psSearch);
}

static void _EditDestroy(EH_ISEARCH *psSearch)
{
	DestroyWindow(psSearch->hwndInput); psSearch->hwndInput=NULL;
	DeleteObject(psSearch->hFont); psSearch->hFont=NULL;
}


//
// _EditFocus()
//
static void _EditFocus(EH_ISEARCH *psSearch)
{

	if (psSearch->bFocus) return;
	
	_areaCalc(psSearch);
	psSearch->bFocus=TRUE;
	ipt_noedit(); 

	if (psSearch->hFont) DeleteObject(psSearch->hFont);
	// 
	// Creo il Font
	//
	psSearch->hFont=CreateFont(psSearch->iFontHeightReal, // Altezza del carattere
				     0, // Larghezza del carattere (0=Default)
				     0, // Angolo di rotazione x 10
				     0, //  Angolo di orientamento bo ???
				     0,//sys.fFontBold ? FW_BOLD : 0, // Spessore del carattere (MACRO Ex: FW_BOLD)
				     0, // Flag Italico    ON/OFF
				     0, // Flag UnderLine  ON/OFF
				     0, // Flag StrikeOut  ON/OFF
				     0,//DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
				     OUT_DEFAULT_PRECIS, // Output precision
				     0, // Clipping Precision
				     DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
				     DEFAULT_PITCH,//!FONT_fix ? VARIABLE_PITCH : FIXED_PITCH, // Pitch & Family (???)
				     //0,
					 "#Arial"); // Nome del font

	SendMessageW(psSearch->hwndInput,WM_SETFONT,(WPARAM) psSearch->hFont,MAKELPARAM(TRUE, 0));
	SetWindowTextW(psSearch->hwndInput,psSearch->pwInputValue);
	SendMessageW(psSearch->hwndInput,EM_SETSEL,0,wcslen(psSearch->pwInputValue));
	MoveWindow(	psSearch->hwndInput,
				psSearch->sRectInput.left,
				psSearch->sRectInput.top,
				psSearch->sSizeInput.cx,
				psSearch->sSizeInput.cy,TRUE);
	ShowWindow(psSearch->hwndInput,SW_SHOW);
	winSetFocus(psSearch->hwndInput);
	SetWindowTextW(psSearch->hwndInput,psSearch->pwInputValue);
	SendMessageW(psSearch->hwndInput,EM_SETSEL,0,wcslen(psSearch->pwInputValue));
	//obj_vedisolo(psSearch->lpObj->nome);
	_EditRepaint(psSearch);

}

static void _EditBlur(EH_ISEARCH *psSearch)
{
	if (!psSearch->bFocus) return;
	psSearch->bFocus=FALSE;
	ShowWindow(psSearch->hwndInput,SW_HIDE);
	//obj_vedisolo(psSearch->lpObj->nome);
	_EditRepaint(psSearch);
	if (psSearch->bEventFocus) obj_putevent("%sBLUR",psSearch->lpObj->nome); 
}

// ----------------------------------------------------------------
// Procedura di controllo sul SuperClassing dell'editor di testo
//

static void LCleanControl(EH_ISEARCH *psSearch)
{
	if (!psSearch->hwndInput) ehError();
	// Mostro il button clean
	if (!psSearch->bButtonClean&&*psSearch->pInputValue)
	{
		psSearch->bButtonClean=TRUE;
		_areaCalc(psSearch);
		MoveWindow(	psSearch->hwndInput,
					psSearch->sRectInput.left,
					psSearch->sRectInput.top,
					psSearch->sSizeInput.cx,
					psSearch->sSizeInput.cy,TRUE);
		
		_EditRepaint(psSearch);
	}

	// Tolgo il button clean
	if (psSearch->bButtonClean&&!*psSearch->pInputValue)
	{
		psSearch->bButtonClean=FALSE;
		_areaCalc(psSearch);
		MoveWindow(psSearch->hwndInput,psSearch->sRectInput.left,psSearch->sRectInput.top,psSearch->sSizeInput.cx,psSearch->sSizeInput.cy,TRUE);
		_EditRepaint(psSearch);
	//	if (psSearch->hwndInput) InvalidateRect(GetParent(psSearch->hwndInput),&psSearch->sRectClean,FALSE);
	}
}

static CHAR *_GetValue(EH_ISEARCH *psSearch) {

	INT iSize=GetWindowTextLength(psSearch->hwndInput);
	CHAR *pString;
	if (!psSearch->bNotifyEveryKey) pString=strDup(psSearch->pInputValue);
	GetWindowTextW(psSearch->hwndInput,psSearch->pwInputValue,psSearch->iInputSize);  
	GetWindowText(psSearch->hwndInput,psSearch->pInputValue,psSearch->iInputSize);  
	psSearch->pwInputValue[iSize]=0;
	psSearch->pInputValue[iSize]=0;
//	printf("[%s] %d" CRLF,psSearch->pInputValue,iSize);

	if (psSearch->bNotifyEveryKey) 
	{
		obj_addevent(psSearch->lpObj->nome);
	}
	else
	{
		if (strcmp(pString,psSearch->pInputValue)) obj_addevent(psSearch->lpObj->nome);
		ehFree(pString);
	}
	LCleanControl(psSearch);
	return pString;

}

//
// _ISearchEditProc
//
static LRESULT CALLBACK _ISearchEditProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    INT nVirtKey;
	BOOL fShift;
	BOOL bBlur=FALSE;
	EH_ISEARCH *psSearch;

	psSearch=(EH_ISEARCH *) GetWindowLong(hWnd,GWL_USERDATA);
	
	switch (message)
	{
		// Fatto per intercettare quando si clicca su qualcosa che non fa perdere il focus della finestra
		case WM_CREATE:		SetTimer(hWnd, 100, 50, NULL);break;
		case WM_DESTROY:	KillTimer(hWnd,100); break;
		case WM_TIMER:
			// Controllo se devo aggiungere la xblue
			LCleanControl(psSearch);
			break;

		/*
			if (GetCapture()!=0&&GetCapture()!=hWnd) //fBreak=TRUE;
			{
				DestroyWindow(hWnd);
			}
			break;
*/
		case WM_CHAR:
			if (psSearch->iTypeInput==NUME)
			{
				TCHAR chChar = (TCHAR) wParam;
				if (chChar<32) break;
				if (chChar=='-') {wParam='-'; break;}

				if (chChar==','||chChar=='.')
					wParam='.';
				else
				{
					if (chChar<'0'||chChar>'9') return FALSE;
				}
			}
			_GetValue(psSearch);
			break;

	//	case WM_CAPTURECHANGED:
//		case WM_ACTIVATE:
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			//return CallWindowProc(sApp.OldEditProc,hWnd,message,wParam,lParam);
			break;

		case WM_SETCURSOR: break;

		case WM_KEYUP:

			_GetValue(psSearch);
			nVirtKey = (int) wParam;    
			fShift=GetAsyncKeyState(VK_SHIFT);
			if (nVirtKey==VK_ESCAPE) {obj_putevent("%sESC",psSearch->lpObj->nome); bBlur=TRUE;}
			if (nVirtKey==VK_RETURN) {obj_putevent("%sCR",psSearch->lpObj->nome); bBlur=TRUE;}
			if (nVirtKey==VK_UP) {obj_putevent("%sUP",psSearch->lpObj->nome);}
			if (nVirtKey==VK_DOWN) {obj_putevent("%sDOWN",psSearch->lpObj->nome);}
			if (nVirtKey==VK_TAB) 
			{
				if (fShift) 
					obj_putevent("%s<-",psSearch->lpObj->nome); 
					else
					obj_putevent("%s->",psSearch->lpObj->nome); 
				bBlur=TRUE; 
			}
			
			if (!bBlur) 
			{
				/*
				BYTE *pString=_GetValue(psSearch);
				if (psSearch->bNotifyEveryKey)
				{
					obj_addevent(psSearch->lpObj->nome);
				}
				else
				{
					if (strcmp(pString,psSearch->pInputValue)) obj_addevent(psSearch->lpObj->nome);
					ehFree(pString);
				}
				LCleanControl(psSearch);
				*/
			}
			else
			{
				EH_EVENT sEvent;
				// Richiesta di focus alla finestra principale
			    ZeroFill(sEvent);
			    sEvent.iEvent=EE_FOCUS;
			    sEvent.hWnd=WindowNow();
			    sEvent.iWin=hWndToWin(sEvent.hWnd);
			    ehEvent(WS_ADD,0,&sEvent);

//				_EditDestroy(psSearch);
				_EditBlur(psSearch);
				return 0;
			}

			break;

		case WM_KILLFOCUS:
			_EditBlur(psSearch);
			/*
			hwndGetFocus = (HWND) wParam;
			GetWindowTextW(hWndEdit,lpEfWord,iEEBufCount); // Proviamo
			DestroyWindow(hWnd);
			fBreak=TRUE;
			*/
			break;
	}
  return CallWindowProc(sApp.OldEditProc,hWnd,message,wParam,lParam);
}

//
// Calcolo le dimensioni dei sotto-oggetti
//
static void _areaCalc(EH_ISEARCH *psSearch)
{
	INT xLens;
	// Calcolo della dimensione dell'icone di Clean
	psSearch->sRectClean.left=psSearch->lpObj->sClientRect.right-20;//+relwx;
	psSearch->sRectClean.top=psSearch->lpObj->sClientRect.top;//+relwy;
	psSearch->sRectClean.right=psSearch->lpObj->sClientRect.right-1;//+relwx;
	psSearch->sRectClean.bottom=psSearch->lpObj->sClientRect.bottom;//+relwy;

	// Calcolo le dimensioni del box del testo
//	psSearch->sRectInput.left=psSearch->lpObj->sClientRect.left+5;
	psSearch->sRectInput.left=psSearch->lpObj->sClientRect.left+psSearch->iRoundCorner/3;

	psSearch->sRectInput.top=psSearch->lpObj->sClientRect.top;
	psSearch->sRectInput.right=psSearch->lpObj->sClientRect.right-6;
	psSearch->sRectInput.bottom=psSearch->lpObj->sClientRect.bottom;
	
	switch (psSearch->iLayOut)
	{
		case 0: break;
		case ZIS_LENS_SELECT: xLens=24; break;
		case ZIS_LENS: xLens=18; break;
	}

	if (psSearch->iLayOut)
	{
		psSearch->bLens=TRUE;
		psSearch->srLens.left=0;
		psSearch->srLens.top=0;
		psSearch->srLens.right=xLens-1;
		psSearch->srLens.bottom=0;
		psSearch->sRectInput.left+=xLens;
	}

	if (*psSearch->pInputValue)	{psSearch->sRectInput.right-=18;}

	psSearch->sSizeInput.cx=psSearch->sRectInput.right-psSearch->sRectInput.left+1;
	psSearch->sSizeInput.cy=psSearch->sRectInput.bottom-psSearch->sRectInput.top+1;

	if (psSearch->iFontHeight)
	{
		psSearch->iFontHeightReal=psSearch->iFontHeight;
	}
	else
	{
		switch (psSearch->iTypeInput)
		{
				case NOTE:
					psSearch->iFontHeightReal=14;
					break;

				default:
				case ALFA:
				case ALFU: 
				case APSW: 
				case NUME: 
					psSearch->iFontHeightReal=psSearch->sSizeInput.cy-4;
				break;
		}
	
	}
	// Centro il font
	psSearch->sRectInput.top+=(psSearch->sSizeInput.cy-psSearch->iFontHeightReal)>>1;
	psSearch->sRectInput.bottom=psSearch->sRectInput.top+psSearch->iFontHeightReal-1;
	psSearch->sSizeInput.cy=psSearch->sRectInput.bottom-psSearch->sRectInput.top+1;


	if (psSearch->bLightFocus)
	{
		psSearch->iPaddingX=2+(psSearch->iRoundCorner/10);
//		psSearch->iPaddingX=2+(psSearch->iRoundCorner*3/2);//2;
	}
	else
	{
		psSearch->iPaddingX=2;
	}
}

static void _EditRepaint(EH_ISEARCH *psSearch) {

	RECT recBox;
	if (!psSearch->hwndInput) ehError();
	rectCopy(recBox,psSearch->lpObj->sClientRect); recBox.bottom++; recBox.right++;
	InvalidateRect(GetParent(psSearch->hwndInput),&recBox,FALSE);
}

// _this_set
static BOOL		_this_set(void *this,void *pszValue,BOOL bEvent) {

	EH_ISEARCH * psSearch=this;
	strAssign(&psSearch->pInputValue,pszValue);
	ehFreeNN(psSearch->pwInputValue);
	psSearch->pwInputValue=strToWcs(pszValue);
	_EditRepaint(psSearch);
	if (bEvent) obj_putevent("%s",psSearch->lpObj->nome);
	return false;

}
