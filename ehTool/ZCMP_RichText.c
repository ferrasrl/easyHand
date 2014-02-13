//   -----------------------------------------------------------
//   | ZCMP_RichText
//   | ZoneComponent RichText
//   |                                              
//   | Gestisce una StaticText di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |										by Ferrà Srl 2005
//   -----------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/fbfile.h"
#include "/easyhand/inc/ZCMP_RichText.h"
#include "/easyhand/ehtool/EditFloat.h"


static BOOL InsertListViewItems(HWND hwndListView);
static HWND _createRichText(HINSTANCE hInstance, HWND hwndParent);
static void SwitchView(HWND hwndListView, DWORD dwView);
static LRESULT CALLBACK funcRichText(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static struct {

	BOOL		bReady;
	WNDPROC		funcTextOld;
	SINT		iHotKey;
	BOOL		fBreak;
	WNDPROC		funcSelectOld;
	WNDPROC		funcButtonOld;
	HINSTANCE   hRichEdit; 

} sEdit = {0,0,0,0,0};
static void _LRichInitialize(void);

static BOOL		_GetCarret(void * this, POINT * ptScroll,CHARRANGE * psChar) {

	EHZ_RICHTEXT * psRT=this;
	if (ptScroll) SendMessage(psRT->wnd, EM_GETSCROLLPOS, (WPARAM)0, (LPARAM) ptScroll);
	if (psChar) SendMessage(psRT->wnd, EM_EXGETSEL, (WPARAM)0, (LPARAM) psChar);
	return false;

}

static BOOL		_SetCarret(void * this, POINT * ptScroll,CHARRANGE * psChar) {

	EHZ_RICHTEXT * psRT=this;
	if (ptScroll) SendMessage(psRT->wnd, EM_SETSCROLLPOS, (WPARAM)0, (LPARAM) ptScroll);
	if (psChar) SendMessage(psRT->wnd, EM_EXSETSEL, (WPARAM)0, (LPARAM) psChar);
	return false;

}

static BOOL		_TextColor(void * this, INT iStartPos,INT iEndPos, COLORREF crNewColor) {

	EHZ_RICHTEXT * psRT=this;

	CHARFORMAT cf;

	// SendMessage(psRT->wnd, EM_SETSEL, (WPARAM)(int)iStartPos, (LPARAM)(int)iEndPos);
	
	_(cf);
	cf.cbSize      = sizeof(CHARFORMAT);
	cf.dwMask      = CFM_COLOR;// | CFM_UNDERLINE | CFM_BOLD;
	cf.dwEffects   = (unsigned long)~(CFE_AUTOCOLOR);// | CFE_UNDERLINE | CFE_BOLD |);
	cf.crTextColor = crNewColor;

	SendMessage(psRT->wnd, EM_SETSEL, (WPARAM)(int)iStartPos, (LPARAM)(int) iEndPos);
	SendMessage(psRT->wnd, EM_SETCHARFORMAT, (WPARAM)(UINT) SCF_SELECTION, (LPARAM) &cf);
//	SendMessage(psRT->wnd, EM_HIDESELECTION, (WPARAM)(BOOL) true, (LPARAM)(BOOL) false);
	
	return false;
}


static BOOL		_CharFormat(void * this, INT iStartPos,INT iEndPos, CHARFORMAT2 * psCharFormat) {

	EHZ_RICHTEXT * psRT=this;

	SendMessage(psRT->wnd, EM_SETSEL, (WPARAM)(int)iStartPos, (LPARAM)(int)iEndPos);
	SendMessage(psRT->wnd, EM_SETCHARFORMAT, (WPARAM)(UINT) SCF_SELECTION, (LPARAM) psCharFormat);
//	SendMessage(psRT->wnd, EM_HIDESELECTION, (WPARAM)(BOOL) TRUE, (LPARAM)(BOOL)false);
	
	return false;
}


static BOOL		_Paint(void * this, BOOL bPaint) {

	EHZ_RICHTEXT * psRT=this;
	psRT->bPaint=bPaint;
	if (bPaint) InvalidateRect(psRT->wnd,NULL,false);
	return false;
}


//
// EhEditText()
//
void * ehzRichText(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr)
{
	EH_DISPEXT * psExt=ptr;
	static SINT HdbMovi=-1;
	static INT16 iSend;
	DWORD dwExStyle;
	EHZ_RICHTEXT *psRT;
	EH_FONT *psFont;
	WCHAR *pwc;
	SINT iLen;
	if (!objCalled) return NULL; // 

	psRT=objCalled->pOther;
	if (!sEdit.bReady) _LRichInitialize();

	switch(cmd)
	{
		case WS_INF: return psRT;

		case WS_CREATE: 
			psRT=objCalled->pOther=ehAllocZero(sizeof(EHZ_RICHTEXT));
			psRT->wnd=objCalled->hWnd=_createRichText(sys.EhWinInstance,WindowNow());
			SetWindowLong(objCalled->hWnd,GWL_USERDATA,(LONG) psRT);
			psRT->bPaint=true;
			psRT->enEncode=SE_ANSI; // Default
			psRT->GetCarret=_GetCarret;
			psRT->SetCarret=_SetCarret;
			psRT->TextColor=_TextColor;
			psRT->Paint=_Paint;
			psRT->CharFormat=_CharFormat;
			break;

		case WS_DESTROY:
			DestroyWindow(objCalled->hWnd);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_EXTNOTIFY:
			psRT->subPostNotify=ptr;
			break;

		case WS_SETFLAG:

			if (!strcmp(ptr,"ENCODE")) // Setta lo stile della finestra
			{
				psRT->enEncode=info;
			}


			if (!strcmp(ptr,"STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				dwStyle|=info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"!STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				dwStyle&=~info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"STYLEMASK")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCalled->hWnd, GWL_STYLE);
				dwStyle=dwStyle&~LVS_TYPESTYLEMASK|info;
				SetWindowLong(objCalled->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(objCalled->hWnd);
				dwExStyle|=info;
				ListView_SetExtendedListViewStyle(objCalled->hWnd,dwExStyle);
			}

			if (!strcmp(ptr,"!EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(objCalled->hWnd);
				dwExStyle&=~info;
				ListView_SetExtendedListViewStyle(objCalled->hWnd,dwExStyle);
			}
			break;


		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,psExt->px+relwx,psExt->py+relwy,psExt->lx,psExt->ly,TRUE);
			break;

		case WS_REALSET: 

			switch (psRT->enEncode)
			{
				default:
				case SE_ANSI:
					SetWindowText(objCalled->hWnd,ptr);
					break;

				case SE_UTF8:
					pwc=strDecode(ptr,SD_UTF8,NULL);
					SetWindowTextW(objCalled->hWnd,pwc);
					ehFree(pwc);
					break;
			}
			break;

		case WS_REALGET: 

			iLen=GetWindowTextLength(objCalled->hWnd)+1;
			switch (psRT->enEncode)
			{
				default:
				case SE_ANSI:
					if (!info) 
						ptr=ehAllocZero(iLen+1);
						else
						{
							if (iLen>(info-1)) ehError();
							GetWindowText(objCalled->hWnd,ptr,info);
						}
					break;

				case SE_UTF8:
					{
						BYTE *psz;
						pwc=ehAlloc(iLen<<1);
						GetWindowTextW(objCalled->hWnd,pwc,iLen);
						psz=strEncodeW(pwc,SE_UTF8,NULL);
						if (info&&strlen(psz)>(DWORD) info) ehError();
						if (info) {strcpy(ptr,psz); ehFree(psz);} else ptr=psz;
						ehFree(pwc);
					}
					break;
			}
			return ptr;
			
			break;

		case WS_DISPLAY: 
			InvalidateRect(objCalled->hWnd,NULL,TRUE);
			break;

		case WS_SET_FONT:
			psFont=(EH_FONT *) ptr;
			SendMessage(objCalled->hWnd, WM_SETFONT, (WPARAM)psFont->hFont, MAKELPARAM(FALSE, 0));
			break;

	}
	return NULL;
}


static HWND _createRichText(HINSTANCE hInstance, HWND hwndParent)
{
	DWORD       dwStyle;
	HWND        hwndEditText;
	BOOL        bSuccess = TRUE;

/*
	dwStyle =   WS_TABSTOP| 
				WS_CHILD| 
				WS_VSCROLL|
				WS_VISIBLE|
				ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN;
*/

	dwStyle=WS_VISIBLE| WS_CHILD | WS_VSCROLL| WS_CLIPCHILDREN | ES_WANTRETURN | ES_MULTILINE | WS_VISIBLE | ES_LEFT | ES_AUTOVSCROLL;
            
	hwndEditText = CreateWindowExW(   WS_EX_CONTROLPARENT|
									  WS_EX_CLIENTEDGE,          // ex style
									  WC_RICHTEXT,               // class name - defined in commctrl.h
									  L"",                       // dummy text
									  dwStyle,                   // style
									  0,                         // x position
									  0,                         // y position
									  1,                         // width
									  1,                         // height
									  hwndParent,                // parent
									  (HMENU) ID_RICHTEXT,        // ID
									  sys.EhWinInstance,                   // instance
									  NULL);                     // no extra data
	if (!hwndEditText) 
		ehError();
	
	SendMessage(hwndEditText,WM_SETFONT,(WPARAM) GetStockObject(ANSI_VAR_FONT),MAKELPARAM(TRUE, 0));

	return hwndEditText;
}

//
// _LRichTextFree()
//
void _LRichTextFree(SINT cmd) {

	if (sEdit.hRichEdit) FreeLibrary(sEdit.hRichEdit);
	sEdit.hRichEdit = NULL;
}

//
// Lista Metodi: http://msdn.microsoft.com/en-us/library/windows/desktop/bb787605(v=vs.85).aspx
// http://msdn.microsoft.com/en-us/library/windows/desktop/bb787873(v=vs.85).aspx
//
static void _LRichInitialize(void)
{
	WNDCLASSW	wndClassW;
	ATOM aRet;
	WCHAR * pwcClass=MSFTEDIT_CLASS;

	sEdit.hRichEdit = LoadLibrary("Msftedit.dll"); // RICHED32.DLL
	
	if (!sEdit.hRichEdit) {sEdit.hRichEdit = LoadLibrary("Riched20.dll"); pwcClass=RICHEDIT_CLASSW; }// RICHED32.DLL
	if (!sEdit.hRichEdit) {sEdit.hRichEdit = LoadLibrary("Riched32.dll");  pwcClass=RICHEDIT_CLASSW; }// RICHED32.DLL
	ZeroFill(wndClassW);
	if (!GetClassInfoW(sys.EhWinInstance,pwcClass,&wndClassW)) {
		osError(TRUE,0,"Richtext");
		ehError();
	}
	wndClassW.hInstance=sys.EhWinInstance;
	wndClassW.lpszClassName=WC_RICHTEXT;
	sEdit.funcTextOld=wndClassW.lpfnWndProc;
	wndClassW.lpfnWndProc=funcRichText;
	aRet=RegisterClassW(&wndClassW);
	sEdit.bReady=TRUE;

	// Aggiungo di rilasciare la risorsa in chiusura
	ehAddExit(_LRichTextFree);
}




static LRESULT CALLBACK funcRichText(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    SINT nVirtKey;
	BOOL fShift;
	EHZ_RICHTEXT * psRT=(EHZ_RICHTEXT * ) GetWindowLong(hWnd,GWL_USERDATA);
	
	switch (message)
	{

		case WM_PAINT:
			if (psRT) {
				if (!psRT->bPaint) return 0;
			}
			break;

		case WM_CHAR:
			/*
			if (iCurrentType==NUME)
			{
				TCHAR chChar = (TCHAR) wParam;
				if (chChar<32) break;
				if (chChar=='-') {wParam='-'; break;}

				if (chChar==','||chChar=='.')
					wParam='.';
				else
				{

					if (chChar<'0'||chChar>'9')
						return FALSE;
				}
			}
			*/
			break;

		case WM_LBUTTONUP:
			ReleaseCapture();
			break;

		case WM_KEYDOWN:
		
			nVirtKey = (int) wParam;    
			sEdit.iHotKey=0;
			fShift=GetAsyncKeyState(VK_SHIFT);

			if (nVirtKey==VK_ESCAPE) sEdit.iHotKey=EF_ESC;
	/*
			if (iCurrentType!=NOTE)
			{
			  if (nVirtKey==VK_RETURN) iHotKey=EF_CR; 
			  // Arrow Up
			  if (nVirtKey==VK_UP)	 iHotKey=EF_FUP;
			  // Arrow Down
			  if (nVirtKey==VK_DOWN)   iHotKey=EF_FDOWN; 
			}
			*/

 			if (nVirtKey==VK_F9)	 sEdit.iHotKey=EF_F9; 
			// Page Up
			if (nVirtKey==VK_PRIOR)  sEdit.iHotKey=EF_PAGEUP;
			// Page Down
			if (nVirtKey==VK_NEXT)   sEdit.iHotKey=EF_PAGEDOWN; 

			// Tab       
			if (nVirtKey==VK_TAB)  
			{
				if (fShift) 
					sEdit.iHotKey=EF_ALT_TAB; 
					else   
					sEdit.iHotKey=EF_TAB;
			}

			if ((sEdit.iHotKey==EF_CR)||(sEdit.iHotKey==EF_TAB)||(sEdit.iHotKey==EF_ALT_TAB))
				{
				// GetWindowText(hWnd,lpEEBuf,iEEBufCount);
				}
			
			//dispx("[%d]  %d  ",(SINT) nVirtKey,(SINT) GetAsyncKeyState(VK_SHIFT));
			if (sEdit.iHotKey) sEdit.fBreak=TRUE;//DestroyWindow(sSetup.hwndEdit);
			break;

		case WM_KILLFOCUS:
			sEdit.fBreak=TRUE;
			break;
	}

	return CallWindowProc(sEdit.funcTextOld,hWnd,message,wParam,lParam);
}
