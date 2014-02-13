//   ---------------------------------------------
//   | EditFloat                                  
//   | Campo di input fluttuante                 
//   |                                           
//   | OS supportati                                         
//   | - Windows 32 bit                          
//   |                                           
//   |         by Ferrà Art & Technology 1993-2000
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/fbfile.h"
#include "/easyhand/ehtool/EditFloat.h"

#define TEXT_FLOAT L"EhEditFloat"

// -------------------------------------------------------
// EditFloat()
// Editor fluttuante
// 
// -------------------------------------------------------

static WNDPROC OldEditProc=NULL;
static LRESULT CALLBACK _EhEditProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
// static HWND hwndEdit=NULL;
static INT iHotKey;
static CHAR *lpEfChar;
static WCHAR *lpEfWord;
static INT iEEBufCount;
// static SIZE LSize;
static BOOL fBreak=FALSE;

static WNDPROC OwnerWinProc;
static WNDPROC ParentWinProc;
static HWND hOwner;
static HWND hWndParent;
//static LRESULT CALLBACK LWndEditProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
//static LRESULT CALLBACK LWndParentProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static LRESULT CALLBACK LWndParentProcCombo(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static INT iCurrentType=-1;

#ifndef EH_MOBILE

INT EditFloatEx(	HWND hWndOwner,
					RECT * precPos,
					INT iType,
					INT iView,
					WCHAR *lpwString,
					INT iSizeBuffer,
					BOOL fToEnd,
					void *Ptr,
					CHAR *lpFont,
					INT iAltFont,
					BOOL bAddScroll)
{
	MSG  msg;
	WNDCLASSW wndClass;
	INT yS,ySz;
	DWORD dwExStyle;
	DWORD Style;
	INT Count=0;
	HWND hwndOriginalFocus;
	HFONT hFont=NULL;
	SIZE sizReq,sizText;
	HWND hwndEdit=NULL;
	RECT recPos;

	while (ehGetMouseButton()) {eventGet(NULL);} // Controllo che non ci sia il mouse premuto in ingresso

	iCurrentType=iType;
	hwndOriginalFocus=GetFocus();
	if (!hwndOriginalFocus) hwndOriginalFocus=WindowNow();
	memcpy(&recPos,precPos,sizeof(RECT));
	if (bAddScroll) {
	
		recPos.left-=GetScrollPos(hWndOwner,SB_HORZ); // Novità che potrebbe rompere
		recPos.right-=GetScrollPos(hWndOwner,SB_HORZ); // Novità che potrebbe rompere
		recPos.top-=GetScrollPos(hWndOwner,SB_VERT);
		recPos.bottom-=GetScrollPos(hWndOwner,SB_VERT);

	}

	//OwnerWinProc=(WNDPROC) GetWindowLong(hWndOwner,GWL_WNDPROC);
	//SetWindowLong(hWndOwner,GWL_WNDPROC,(LONG) LWndEditProc);
	
	// SuperClassing di Editing
	GetClassInfoW(sys.EhWinInstance,L"Edit",&wndClass);
	wndClass.hInstance=sys.EhWinInstance;
	wndClass.lpszClassName=TEXT_FLOAT;
	OldEditProc=wndClass.lpfnWndProc;
	wndClass.lpfnWndProc=_EhEditProc;
	RegisterClassW(&wndClass);
	lpEfWord=lpwString;
	iEEBufCount=iSizeBuffer;

	sizeCalc(&sizReq,&recPos);
	sizeCalc(&sizText,&recPos);
//	LSize.cx=Rect->right-Rect->left+1;
//	LSize.cy=Rect->bottom-Rect->top+1;
	if (!iAltFont) 
	{
		switch (iType)
		{
			case _TEXT:
				iAltFont=14;
				break;

			default:
			case _ALFA:
			case ALFU: 
			case _PASSWORD: 
			case _NUMBER: 
				if (iView==RIGA) iAltFont=sizReq.cy-4; else iAltFont=sizReq.cy-4;
				break;
		}
	}

	if (iView==RIGA) 
		{
		 sizText.cx+=4; sizText.cy+=4; yS=2; ySz=4;
		 dwExStyle=WS_EX_CLIENTEDGE;
		} 
		else 
		{ 
		 sizText.cx+=2; sizText.cy+=2; yS=2; ySz=2;
		 dwExStyle=0;
		}
		

	// dwExStyle|=WS_EX_LAYERED;
#ifdef _WIN32_WCE
	Style=WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL;
#else
//	Style=WS_CHILDWINDOW|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL;
	Style=WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_AUTOHSCROLL;
#endif

	switch (iType)
	{
		case ALFU: Style = (Style|ES_UPPERCASE); break;
		case APSW: Style = (Style|ES_PASSWORD); break;
		case NUME: //Style = (Style|ES_NUMBER); 
			break;

		case NOTE:
			Style = (Style|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL);
			//dwExStyle|=WS_EX_RIGHTSCROLLBAR;
			break;
	}

	

	hwndEdit=CreateWindowExW(dwExStyle,
						    TEXT_FLOAT,
						    L"",//lpBuffer,
						    Style,
							/*
						    psRect->left,psRect->top-yS,
						    psRect->right-psRect->left+1,
						    psRect->bottom-psRect->top+1+ySz,
							*/
						    recPos.left,recPos.top,
							sizReq.cx,
						    sizReq.cy,
						    hWndOwner,
						    (HMENU) ID_EDITFLOAT,
						    sys.EhWinInstance,
						    0);
	
	if (hwndEdit==NULL) return 0;

	// 
	// Creo il Font
	//

//	win_infoarg("%d",iAltFont);
	hFont=CreateFont(iAltFont, // Altezza del carattere
				     0, // Larghezza del carattere (0=Default)
				     0, // Angolo di rotazione x 10
				     0, //  Angolo di orientamento bo ???
				     0,//sys.fFontBold ? FW_BOLD : 0, // Spessore del carattere (MACRO Ex: FW_BOLD)
				     0, // Flag Italico    ON/OFF
				     0, // Flag UnderLine  ON/OFF
				     0, // Flag StrikeOut  ON/OFF
				     0,//DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
//					   FERRA_OUTPRECISION, // Output precision
				     OUT_DEFAULT_PRECIS, // Output precision
				     0, // Clipping Precision
				     DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
				     DEFAULT_PITCH,//!FONT_fix ? VARIABLE_PITCH : FIXED_PITCH, // Pitch & Family (???)
				     //0,
				     lpFont); // Nome del font

	SendMessageW(hwndEdit,WM_SETFONT,(WPARAM) hFont,MAKELPARAM(TRUE, 0));
//	SendMessage(hwndEdit,WM_SETFONT,(WPARAM) GetStockObject(ANSI_VAR_FONT),MAKELPARAM(TRUE, 0));
	winSetFocus(hwndEdit);
	SetWindowTextW(hwndEdit,lpwString);
	if (fToEnd) 
		{SendMessageW(hwndEdit,EM_SETSEL,wcslen(lpwString),wcslen(lpwString));}
		else
		{SendMessageW(hwndEdit,EM_SETSEL,0,wcslen(lpwString));}

	iHotKey=0; fBreak=FALSE;
	// SetLayeredWindowAttributes(hwndEdit, 0x00000000, 255, LWA_COLORKEY);

//	SetCapture(hwndEdit); // ##
	while (GetMessage(&msg, NULL, 0x00, 0x00))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (fBreak) break;
	}

//	ReleaseCapture(); // ##
	printf("Uscito ... " CRLF);

	//winSetFocus(WindowNow());
//	win_infoarg("QUI");
	winSetFocus(hwndOriginalFocus);
//	if (hwndEdit) {DestroyWindow(hwndEdit); hwndEdit=NULL;} 
	// iHotKey=EF_BLUR;

//	InvalidateRect(hWndOwner,NULL,TRUE); // Ridisegno il proprietario
//	_d_("------------------------> %d",iHotKey); ehSleep(500);
	if (iHotKey==0) iHotKey=EF_BLUR;
	if (hFont) DeleteObject(hFont);
	//win_infoarg("lpEfWord=%d [%ls]",wcslen(lpEfWord),lpEfWord);
	return iHotKey;
}

// -------------------------------------------------------------------------------------
// EditFloat()
// -------------------------------------------------------------------------------------
INT EditFloat(	HWND	hWndOwner,
				RECT *	psRect,
				INT		iType,
				INT		iView,
				CHAR *	lpString,
				INT		iSizeBuffer,
				BOOL	fToEnd,
				void *	Ptr,
				BOOL	bAddScroll)
{
	INT iErr;
	WCHAR * pwcBuffer=ehAlloc(iSizeBuffer*2),*pwc;
	CHAR * psz;
	pwc=strToWcs(lpString); wcscpy(pwcBuffer,pwc); ehFree(pwc);
	iErr=EditFloatEx(hWndOwner,psRect,iType,iView,pwcBuffer,iSizeBuffer,fToEnd,Ptr,TEXT("Arial"),0,bAddScroll);
	psz=wcsToStr(pwcBuffer); strcpy(lpString,psz);
	ehFreePtrs(2,&pwcBuffer,&psz);
	return iErr;
}
#endif
/*
// -------------------------------------------------------------------------------------
// Procedura per il reindirizzamento dei messaggi di notifica alla parent (hWndOwner)
// Controllo delle Notifiche sulla finestra proprietaria dell'oggetto
// -------------------------------------------------------------------------------------
static LRESULT CALLBACK LWndEditProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	static INT Count=0;
#ifndef _WIN32_WCE
	INT nHittest;
	UINT uMsg;
	HWND hwndTopLevel;
#endif
	switch (message)
	{
		case WM_COMMAND: break;
#ifndef _WIN32_WCE
		case WM_INITMENU: break;
		case WM_MOUSEACTIVATE: 
			hwndTopLevel = (HWND) wParam;    // handle of top-level parent 
			nHittest = (INT) LOWORD(lParam); // hit-test value 
			uMsg = (UINT) HIWORD(lParam);    // mouse message
			if (nHittest!=1) {DestroyWindow(hwndEdit); hwndEdit=NULL;}
			break;
#endif

		case WM_SETFOCUS: 
			break;
	}
  return CallWindowProc(OwnerWinProc,hWnd,message,wParam,lParam);
}
*/


// -------------------------------------------------------------------------------------
// Procedura di controllo sulla finestra parent
// 
// 
/*
static LRESULT CALLBACK LWndParentProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	static INT Count=0;
	INT nHittest;
	WORD wMouseMsg;
//	_d_("%d  %d              ",Count++,message);
	switch (message)
	{
		case WM_SETCURSOR:
			//hwnd = (HWND) wParam;       // handle to window with cursor 
			nHittest = LOWORD(lParam);  // hit-test code 
			wMouseMsg = HIWORD(lParam); // mouse-message identifier 
			if ((hWnd==(HWND) wParam)&&(nHittest==HTCLIENT||nHittest==HTMENU)&&wMouseMsg==WM_LBUTTONDOWN) 
				{DestroyWindow(hwndEdit); hwndEdit=NULL;}
			return 0;

	}
  return CallWindowProc(ParentWinProc,hWnd,message,wParam,lParam);
}
*/
// ----------------------------------------------------------------
// Procedura di controllo sul SuperClassing dell'editor di testo
//

static LRESULT CALLBACK _EhEditProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    INT nVirtKey;
	BOOL fShift;
	HWND hwndGetFocus;
	
//	printf("message: %d" CRLF,message);
	switch (message)
	{

		// Fatto per intercettare quando si clicca su qualcosa che non fa perdere il focus della finestra
		case WM_CREATE:		SetTimer(hWnd, 100, 50, NULL);break;
		case WM_DESTROY:	KillTimer(hWnd,100); break;

		case WM_TIMER: // Uscita sul capture su un altra finestra
			/*
			hwndCapt=GetCapture();
			if (hwndCapt&&hwndCapt!=hWnd) {

				DestroyWindow(hWnd);
				fBreak=TRUE;
			
			}
			*/
			break;

			/*
			if (GetCapture()!=0&&GetCapture()!=hWnd) //fBreak=TRUE;
			{
				DestroyWindow(hWnd);
				fBreak=TRUE;
			}
			break;
*/

		case WM_NCHITTEST:
			// printf("WM_NCHITTEST" CRLF);
			break;

		case WM_CHAR:

			if (iCurrentType==NUME)
			{
				CHAR chChar = (CHAR) wParam;
				if (chChar<32) break;
				if (chChar=='-') {wParam='-'; break;}

				if (chChar==','||chChar=='.')
					wParam='.';
				else
				{
					if (chChar<'0'||chChar>'9') return FALSE;
				}
			}
			break;

	//	case WM_CAPTURECHANGED:
///		case WM_ACTIVATE:
			break;

		case WM_MOVE:
				
//				DestroyWindow(hWnd);
//				fBreak=TRUE;
				break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			{
				INT xPos = GET_X_LPARAM(lParam); 
				INT yPos = GET_Y_LPARAM(lParam); 
				RECT rect;
				SIZE size;
				GetWindowRect(hWnd,&rect);
				sizeCalc(&size,&rect);

				// printf("edit:%d,%d" CRLF,xPos,yPos);
				// Click fuori dalla Testo
				if (yPos<0||yPos>size.cy||xPos<0||xPos>size.cx) {
					DestroyWindow(hWnd);
					fBreak=TRUE;
				}
				//dispx("qui: %d,%d         ",xPos,yPos);
				//efx2();
			}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			return CallWindowProc(OldEditProc,hWnd,message,wParam,lParam);
//			return 0;

		case WM_KEYDOWN:
		
			nVirtKey = (int) wParam;    
			iHotKey=0;
			fShift=GetAsyncKeyState(VK_SHIFT);
			if (nVirtKey==VK_ESCAPE) iHotKey=EF_ESC;
			if (iCurrentType!=NOTE)
			{
			  if (nVirtKey==VK_RETURN) iHotKey=EF_CR; 
			  // Arrow Up
			  if (nVirtKey==VK_UP)	 iHotKey=EF_FUP;
			  // Arrow Down
			  if (nVirtKey==VK_DOWN)   iHotKey=EF_FDOWN; 
			}

 			if (nVirtKey==VK_F9)	 iHotKey=EF_F9; 
			// Page Up
			if (nVirtKey==VK_PRIOR)  iHotKey=EF_PAGEUP;
			// Page Down
			if (nVirtKey==VK_NEXT)   iHotKey=EF_PAGEDOWN; 

			// Tab       
			if (nVirtKey==VK_TAB)  
			{
				if (fShift) 
					iHotKey=EF_ALT_TAB; 
					else   
					iHotKey=EF_TAB;
			}

			if ((iHotKey==EF_CR)||(iHotKey==EF_TAB)||(iHotKey==EF_ALT_TAB))
				{
					//GetWindowTextW(hwndEdit,lpEfWord,iEEBufCount);
					GetWindowTextW(hWnd,lpEfWord,iEEBufCount);
			}
			
			//_d_("[%d]  %d  ",(INT) nVirtKey,(INT) GetAsyncKeyState(VK_SHIFT));
			if (iHotKey) 
			{
				DestroyWindow(hWnd);
				fBreak=TRUE;
			}
				//DestroyWindow(hwndEdit);
			break;

		case WM_KILLFOCUS:
			hwndGetFocus = (HWND) wParam;
			//GetWindowTextW(hwndEdit,lpEfWord,iEEBufCount); // Proviamo
			GetWindowTextW(hWnd,lpEfWord,iEEBufCount); // Proviamo
			//fBreak=TRUE;
			DestroyWindow(hWnd);
			fBreak=TRUE;
			break;


	}
  return CallWindowProc(OldEditProc,hWnd,message,wParam,lParam);
}

// -------------------------------------------------------
// ComboFloat()
// ComboList fluttuante
// 
// -------------------------------------------------------
 
static WNDPROC LOldComboProc=NULL;
static LRESULT CALLBACK LNewComboProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
static LRESULT CALLBACK LWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static HWND hWndCombo=NULL;
static INT iHotKey;
static INT iEEBufCount;


// ------------------------------------------------------------
// COMBOFLOAT
// ------------------------------------------------------------

INT ComboFloat(HWND hWndOwner,RECT *Rect,CHAR *lpBuffer,INT iSizeBuffer,INT iList,CHAR **lppList)
{
 INT a;
 INT yS=2,ySz=0;
 MSG  msg;
 INT yAlt=0;
 WNDCLASS wndClass;
 CHAR *lpClass=TEXT("EHCOMBOF");

 // A) Leggo la classe della finestra Proprietaria
 // B) Sposto la subprocedura di controllo in quella nuova
 //    per permettere l'intercettazione dei messaggi di notifica alla parent

 OwnerWinProc=(WNDPROC) GetWindowLong(hWndOwner,GWL_WNDPROC);
 SetWindowLong(hWndOwner,GWL_WNDPROC,(LONG) LWndProc);

	// --------------------------------------------------------------------
	// Cerco e reindirizzo il controllo della finestra Padre
	// della finestra proprietaria
	// --------------------------------------------------------------------
	hWndParent=GetParent(hWndOwner);
	if (hWndParent)
	{
	 ParentWinProc=(WNDPROC) GetWindowLong(hWndParent,GWL_WNDPROC);
	 SetWindowLong(hWndParent,GWL_WNDPROC,(LONG) LWndParentProcCombo);
	}

 // C) SubClassing di ComboBox per permettere 
 //    l'intercettazione della pressione dei tasti
 GetClassInfo(NULL,TEXT("COMBOBOX"),&wndClass);
 wndClass.hInstance=sys.EhWinInstance;
 wndClass.lpszClassName=lpClass;
 LOldComboProc=wndClass.lpfnWndProc;
 wndClass.lpfnWndProc=LNewComboProc;
 RegisterClass(&wndClass);

 // D) Backup delle variabili di controllo in statiche
 lpEfChar=lpBuffer;
 iEEBufCount=iSizeBuffer;
 yAlt=Rect->bottom-Rect->top+1+ySz;
 
 // E) Creo la nuova combo box
 hWndCombo=CreateWindow(lpClass,
						TEXT(""),
						WS_CHILD|WS_VSCROLL|WS_TABSTOP|
						//WS_THICKFRAME|
						WS_BORDER|
						//CBS_OWNERDRAWFIXED|
						CBS_DROPDOWNLIST
						,
						Rect->left,Rect->top-yS,
						Rect->right-Rect->left+1,
						Rect->bottom-Rect->top+1+(iList*yAlt),
						hWndOwner,
						//WindowNow(),
						(HMENU) ID_EDITFLOAT,
						sys.EhWinInstance,
						NULL);

 if (hWndCombo==NULL) ehExit("COMBOFLOAT: error 1");
 
 // F) Parametrizzo la combo (Font,Dati,ecc...)
#ifndef _WIN32_WCE
 SendMessage(hWndCombo,WM_SETFONT,(WPARAM) GetStockObject(DEFAULT_GUI_FONT),MAKELPARAM(TRUE, 0));
#endif
 for (a=0;;a++)
 {
   if (lppList[a]==NULL) break;
   SendMessage(hWndCombo,CB_ADDSTRING,0,(LPARAM) lppList[a]);
 }

 SendMessage(hWndCombo,CB_SETITEMHEIGHT,-1,yAlt);
 SendMessage(hWndCombo,CB_SETITEMHEIGHT,0,yAlt);
 SendMessage(hWndCombo,CB_SETCURSEL,0,0);
 ShowWindow(hWndCombo,SW_SHOW);
 SendMessage(hWndCombo,CB_SHOWDROPDOWN,TRUE,0);
 SendMessage(hWndCombo,CB_SELECTSTRING,0,(LPARAM) lpBuffer);
 winSetFocus(hWndCombo);

 // G) Loop di attesa dati
 iHotKey=0;
 fBreak=FALSE;

 while (GetMessage(&msg, NULL, 0x00, 0x00))
 {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	if (fBreak) break;
 }


 // H) Ripristino la vecchia procedura
 SetWindowLong(hWndOwner,GWL_WNDPROC,(LONG) OwnerWinProc);
 // I) Ripritino anche il parent se modificato
 if (hWndParent)
 {
	SetWindowLong(hWndParent,GWL_WNDPROC,(LONG) ParentWinProc);
 }
 winSetFocus(WindowNow());
 if (hWndCombo) {DestroyWindow(hWndCombo); hWndCombo=NULL;}
// win_infoarg("[%d]",iHotKey);
 return iHotKey;
}

// -------------------------------------------------------------------------------------
// Procedura per il reindirizzamento dei messaggi di notifica alla parent (hWndOwner)
// Controllo delle Notifiche
// -------------------------------------------------------------------------------------
static LRESULT CALLBACK LWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	WORD wNotifyCode;
	INT Index;  
	switch (message)
	{

#ifndef _WIN32_WCE
		case WM_MOUSEACTIVATE: 
				iHotKey=EF_ESC;
				fBreak=TRUE;
				break;
#endif
		case WM_COMMAND:

			wNotifyCode = HIWORD(wParam);
			
			 switch (wNotifyCode)
			 {
				case CBN_SELENDOK: 	
									Index=SendMessage(hWndCombo,CB_GETCURSEL,0,0);			 
									SendMessage(hWndCombo,CB_GETLBTEXT,(WPARAM) Index,(LPARAM) lpEfChar);
								    fBreak=TRUE;
									if (!iHotKey) iHotKey=EF_SELECT;//EF_TAB;
								    break;
				case CBN_CLOSEUP:   
				case CBN_KILLFOCUS:  
				case CBN_SELENDCANCEL: 	
									//DestroyWindow(hWndCombo);		
									//winSetFocus(WindowNow());
									//PostQuitMessage(0);
									if (!iHotKey) iHotKey=EF_ESC;
									fBreak=TRUE;
									break;
			 }
			
			 break;

	}
 return CallWindowProc(OwnerWinProc,hWnd,message,wParam,lParam);
}

// -------------------------------------------------------------------------------------
// Procedura per il controllo dei tasti speciali sul subclassing della combobox  
// Controllo dei tasti        
// -------------------------------------------------------------------------------------

static LRESULT CALLBACK LNewComboProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    INT nVirtKey;
	BOOL fShift=0;

	switch (message)
	{
		case WM_KEYDOWN:
		
			nVirtKey = (int) wParam;    
			iHotKey=0;
			fShift=GetAsyncKeyState(VK_SHIFT);

			if (nVirtKey==VK_ESCAPE) iHotKey=EF_ESC;
			if (nVirtKey==VK_RETURN) iHotKey=EF_CR; 
			
			// Arrow Up
			//if (nVirtKey==VK_UP)	 iHotKey=EF_FUP;
			// Arrow Down
			//if (nVirtKey==VK_DOWN)   iHotKey=EF_FDOWN; 
			
			// Page Up
			if (nVirtKey==VK_PRIOR)  iHotKey=EF_PAGEUP;
			// Page Down
			if (nVirtKey==VK_NEXT)   iHotKey=EF_PAGEDOWN; 

			// Tab       
			if (nVirtKey==VK_TAB)  
			{
				if (fShift) 
					iHotKey=EF_ALT_TAB; 
					else   
					iHotKey=EF_TAB;
			}

			if ((iHotKey==EF_CR)||(iHotKey==EF_TAB)||(iHotKey==EF_ALT_TAB))
				{
			     INT Index;  
				 Index=SendMessage(hWndCombo,CB_GETCURSEL,0,0);
				 SendMessage(hWndCombo,CB_GETLBTEXT,(WPARAM) Index,(LPARAM) lpEfChar);
				}
			
			if (iHotKey) fBreak=TRUE; 
			break;

		case WM_KILLFOCUS:
			fBreak=TRUE;
			break;
	}

	return CallWindowProc(LOldComboProc,hWnd,message,wParam,lParam);
}


// -------------------------------------------------------------------------------------
// Procedura di controllo sulla finestra parent
// 
// 
static LRESULT CALLBACK LWndParentProcCombo(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	static INT Count=0;
	INT nHittest;
	WORD wMouseMsg;
//	_d_("%d  %d              ",Count++,message);
	switch (message)
	{
		case WM_SETCURSOR:
			//hwnd = (HWND) wParam;       // handle to window with cursor 
			nHittest = LOWORD(lParam);  // hit-test code 
			wMouseMsg = HIWORD(lParam); // mouse-message identifier 
			if ((hWnd==(HWND) wParam)&&nHittest==HTCLIENT&&wMouseMsg==WM_LBUTTONDOWN) 
				{DestroyWindow(hWndCombo); hWndCombo=NULL;}
			return 0;

	}
  return CallWindowProc(ParentWinProc,hWnd,message,wParam,lParam);
}




EH_MENUITEM * MenuFloatSearch(EH_MENUITEM *Ehm,BYTE *lpItemCode)
{
	INT a;
	for (a=0;;a++)
	{
		if (Ehm[a].iType==EHM_STOP) break;
		if (!Ehm[a].pszCode) continue;

		if (!strcmp(Ehm[a].pszCode,lpItemCode))
		{
			return &Ehm[a];
		}
	}
	return NULL;
}

void MenuFloatEnable(EH_MENUITEM *arMenu,BYTE *lpItemCode,BOOL bEnable)
{
	EH_MENUITEM *psMenu;
	psMenu=MenuFloatSearch(arMenu,lpItemCode);
	if (psMenu) psMenu->bEnable=bEnable;
}

void MenuFloatSetText(EH_MENUITEM *arMenu,BYTE *lpItemCode,CHAR *pText)
{
	EH_MENUITEM *psMenu;
	psMenu=MenuFloatSearch(arMenu,lpItemCode);
	if (psMenu) psMenu->pszText=pText;
}

void MenuFloatSetType(EH_MENUITEM *arMenu,BYTE *lpItemCode,INT iType)
{
	EH_MENUITEM *psMenu;
	psMenu=MenuFloatSearch(arMenu,lpItemCode);
	if (psMenu) psMenu->iType=iType;
}

void MenuFloatSetLink(EH_MENUITEM *arMenu,BYTE *lpItemCode,void * pLink)
{
	EH_MENUITEM *psMenu;
	psMenu=MenuFloatSearch(arMenu,lpItemCode);
	if (psMenu) psMenu->pLink=pLink;
}

void MenuFloatRename(EH_MENUITEM *arMenu,BYTE *lpItemCode,CHAR * pNewCode)
{
	EH_MENUITEM *psMenu;
	psMenu=MenuFloatSearch(arMenu,lpItemCode);
	if (psMenu) psMenu->pszCode=pNewCode;
}

//
// MenuFloat()
//
CHAR * MenuFloat(EH_MENUITEM * arsItem,CHAR * pszDefault) {
	return ehMenu(arsItem,pszDefault,NULL,NULL);
}
	/*
	HMENU	hPopMenu;
	POINT	Point;
	INT	iValore;
	HWND	hwnd=WindowNow();

//	iMenuEnum=0;
	hPopMenu=ehMenuBuilder(Ehm,1);

	GetCursorPos(&Point);

#ifndef _WIN32_WCE
	iValore=TrackPopupMenu(hPopMenu,         // handle to shortcut menu
				       TPM_NONOTIFY|
					   TPM_LEFTBUTTON|
					   TPM_LEFTALIGN|//TPM_VCENTERALIGN|
					   TPM_RETURNCMD
					   ,         // screen-position and mouse-button flags
				       Point.x,Point.y,
				       0,
				       hwnd,           // handle to owner window
				       NULL);
	//win_infoarg("%d",iValore);
	//eventGet(NULL);
#else
	iValore=TrackPopupMenu(hPopMenu,         // handle to shortcut menu
				       TPM_NONOTIFY|
					   TPM_LEFTALIGN|//TPM_VCENTERALIGN|
					   TPM_RETURNCMD,         // screen-position and mouse-button flags
				       Point.x,Point.y,
				       0,
				       WindowNow(),           // handle to owner window
				       NULL);
#endif
	
	DestroyMenu(hPopMenu);
    LMenuFreeBmp(Ehm);	
	if (iValore) 
	{
		CHAR *pRet=ehMenuGetCode(Ehm,iValore);
		if (!pRet) pRet="";
		return pRet;
	}
	return lpDefault;
}
	*/

//
// MenuFloatPosWnd()
//
CHAR * MenuFloatPosWnd(INT x,INT y,EH_MENUITEM * arsItem,CHAR * pszDefault,HWND hWnd)
{
//	HMENU hPopMenu;
//	INT iValore;
	POINT pt;
	RECT Rect;

//	iMenuEnum=0;
	//hPopMenu=ehMenuBuilder(Ehm,1);

	pt.x=x; pt.y=y;
	GetWindowRect(hWnd,&Rect);
	pt.x+=Rect.left; if (x<0) x=0;
	pt.y+=Rect.top; if (y<0) y=0;
	
	return ehMenu(arsItem,pszDefault,&pt,hWnd);
/*
	//win_infoarg("%d,%d,%d",WindowNow(),x,y);
	//_d_("%d",Rect.left);

#ifndef _WIN32_WCE
	iValore=TrackPopupMenu(hPopMenu,         // handle to shortcut menu
//						TPM_RETURNCMD |    // Return menu code
//						TPM_RIGHTBUTTON,
				       TPM_NONOTIFY|TPM_LEFTBUTTON|
					   TPM_LEFTALIGN|//TPM_VCENTERALIGN|
					   TPM_RETURNCMD,         // screen-position and mouse-button flags
				       x,y,
				       0,
				       hWnd,           // handle to owner window
				       NULL);
#else
	iValore=TrackPopupMenu(hPopMenu,         // handle to shortcut menu
				       TPM_NONOTIFY|
					   TPM_LEFTALIGN|//TPM_VCENTERALIGN|
					   TPM_RETURNCMD,         // screen-position and mouse-button flags
				       x,y,
				       0,
				       hWnd,           // handle to owner window
				       NULL);
#endif
	DestroyMenu(hPopMenu);
    LMenuFreeBmp(Ehm);	
	
	if (iValore) return ehMenuGetCode(Ehm,iValore);//return Ehm[a-1].pszCode; 
	return lpDefault;
*/
}

CHAR *MenuFloatPos(INT x,INT y,EH_MENUITEM *Ehm,CHAR *lpDefault)
{
	return MenuFloatPosWnd(x,y,Ehm,lpDefault,WindowNow());
}


//
// DynMenu()
//
void * DynMenu(INT cmd,INT iType,CHAR * pszText,
			   BOOL bEnable,CHAR *pszCode,void *pLink,
			   INT iMacroImage,UINT uiEnum,HBITMAP hBmp)
{
	static EH_MENUITEM *lpMenu=NULL;
	static INT iVoci=0;
	EH_MENUITEM *lpRet;

	switch (cmd)
	{
		case WS_OPEN:
			//if (!lpMenu) DynMenu(WS_CLOSE,lpMenu);
			lpMenu=NULL;
			iVoci=0;
			break;

		// Aggiunge una voce
		case WS_ADD:
			if (iVoci==0) lpMenu=ehAlloc(sizeof(EH_MENUITEM)); 
						  else 
						  lpMenu=realloc(lpMenu,sizeof(EH_MENUITEM)*(iVoci+1));
			memset(lpMenu+iVoci,0,sizeof(EH_MENUITEM));
			lpMenu[iVoci].iType=iType;
			if (pszText)
			{
 			 lpMenu[iVoci].pszText=strDup(pszText);//ehAlloc(strlen(pszText)+1);
			 //strcpy(lpMenu[iVoci].pszText,pszText);
			}
			lpMenu[iVoci].bEnable=bEnable;
			if (pszCode)
			{
			 lpMenu[iVoci].pszCode=strDup(pszCode);//ehAlloc(strlen(pszCode)+1);
			 //strcpy(lpMenu[iVoci].pszCode,pszCode);
			}
			lpMenu[iVoci].pLink=pLink;
			lpMenu[iVoci].iMacroImage=iMacroImage;
			lpMenu[iVoci].uiEnum=uiEnum;
			lpMenu[iVoci],hBmp=hBmp;
			iVoci++;
			break;

		// Chiudo l'array e ritorno il puntatore ad un clone
		case WS_DO:
			DynMenu(WS_ADD,EHM_STOP,0,0,0,0,0,0,0);
			lpRet=ehAlloc(sizeof(EH_MENUITEM)*iVoci);
			memcpy(lpRet,lpMenu,sizeof(EH_MENUITEM)*iVoci);
			ehFree(lpMenu);
			return lpRet;

		// Libero la memoria usata
		case WS_CLOSE:
			lpRet=pLink;
			while (lpRet->iType!=EHM_STOP)
			{
				if (lpRet->pszText) ehFree(lpRet->pszText);		
				if (lpRet->pszCode) ehFree(lpRet->pszCode);
				lpRet++;
			}
			ehFree(pLink);
			break;
	}
	return NULL;
}



