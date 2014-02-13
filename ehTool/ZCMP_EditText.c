//   -----------------------------------------------------------
//   | ZCMP_EditText
//   | ZoneComponent EditText
//   |                                              
//   | Gestisce una StaticText di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |										by Ferrà Srl 2005
//   -----------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/fbfile.h"
#include "/easyhand/ehtool/ZCMP_EditText.h"
#include "/easyhand/ehtool/EditFloat.h"

#define WC_EDIT_TEXT L"ehzEditText"

static HWND _LCreateEditText(EHZ_EDITTEXT *pszEdit,DWORD dwStyle,HINSTANCE hInstance, HWND hwndParent);
static LRESULT CALLBACK funcEditText(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

static struct {

	BOOL	bReady;
	WNDPROC funcTextOld;
//	SINT	iHotKey;
//	BOOL	fBreak;
//	WNDPROC funcSelectOld;
//	WNDPROC funcButtonOld;

} sEdit = {0};//,0,0,0,0};

static void _LInitialize(void);

//
// EhEditText()
//
void * ehzEditText(struct OBJ *objCaller,EN_MESSAGE cmd,LONG info,void *ptr)
{
	EH_EVENT * psEvent;
	EH_DISPEXT *DExt=ptr;
	static SINT HdbMovi=-1;
	static INT16 iSend;
	DWORD dwStyle,dwExStyle;
	EHZ_EDITTEXT *psEditText;
	EH_FONT *psFont;
	WCHAR *pwc;
	SINT iLen;
	BYTE *psz=ptr;
	if (!objCaller) return NULL; // 

	psEditText=objCaller->pOther;
	if (!sEdit.bReady) _LInitialize();

	switch(cmd)
	{
		case WS_INF: return psEditText;

		case WS_CREATE: 
			psEditText=objCaller->pOther=ehAllocZero(sizeof(EHZ_EDITTEXT));
			psEditText->psObj=objCaller;
			dwStyle =   //WS_TABSTOP| 
						WS_CHILDWINDOW| 
						WS_VSCROLL|
						WS_VISIBLE|
						ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN;

			objCaller->hWnd=_LCreateEditText(psEditText,dwStyle,sys.EhWinInstance,WindowNow());
			psEditText->hWnd=objCaller->hWnd;
			psEditText->enEncode=SE_ANSI; // Default
			if (!strCmp(objCaller->text,"UNICODE")) psEditText->enEncode=SE_UNICODE;
			else if (!strCmp(objCaller->text,"UTF-8")) psEditText->enEncode=SE_UTF8;
			break;

		case WS_DESTROY:
			DestroyWindow(objCaller->hWnd);
			ehFreePtr(&objCaller->pOther);
			break;

		case WS_EXTNOTIFY:
			psEditText->subPostNotify=ptr;
			break;

		case WS_SETFLAG:

			// Setta lo stile della finestra ------------------>
			if (!strcmp(ptr,"ENCODE")) {
				psEditText->enEncode=info;
			}

			// Setta lo stile della finestra ------------------>
			if (!strcmp(ptr,"STYLE")) {
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCaller->hWnd, GWL_STYLE);
				dwStyle|=info;
				DestroyWindow(objCaller->hWnd);objCaller->hWnd=_LCreateEditText(psEditText,dwStyle,sys.EhWinInstance,WindowNow());
				MoveWindow(objCaller->hWnd,objCaller->sClientRect.left,objCaller->sClientRect.top,objCaller->sClientSize.cx,objCaller->sClientSize.cy,TRUE);
				//SetWindowLong(objCaller->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"!STYLE")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCaller->hWnd, GWL_STYLE);
				dwStyle&=~info;
				DestroyWindow(objCaller->hWnd);objCaller->hWnd=_LCreateEditText(psEditText,dwStyle,sys.EhWinInstance,WindowNow());
				MoveWindow(objCaller->hWnd,objCaller->sClientRect.left,objCaller->sClientRect.top,objCaller->sClientSize.cx,objCaller->sClientSize.cy,TRUE);
			}

			if (!strcmp(ptr,"STYLEMASK")) // Setta lo stile della finestra
			{
				DWORD dwStyle;
				dwStyle=GetWindowLong(objCaller->hWnd, GWL_STYLE);
				dwStyle=dwStyle&~LVS_TYPESTYLEMASK|info;
				SetWindowLong(objCaller->hWnd, GWL_STYLE, (DWORD) dwStyle);
			}

			if (!strcmp(ptr,"EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(objCaller->hWnd);
				dwExStyle|=info;
				ListView_SetExtendedListViewStyle(objCaller->hWnd,dwExStyle);
			}

			if (!strcmp(ptr,"!EXSTYLE")) // Setta lo stile della finestra
			{
				dwExStyle=ListView_GetExtendedListViewStyle(objCaller->hWnd);
				dwExStyle&=~info;
				ListView_SetExtendedListViewStyle(objCaller->hWnd,dwExStyle);
			}
			break;

		case WS_DO: // Spostamento / Ridimensionamento
//			MoveWindow(objCaller->hWnd,DExt->px+relwx,DExt->py+relwy,DExt->lx,DExt->ly,TRUE);
			MoveWindow(objCaller->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
			break;

		case WS_REALSET: 

			switch (psEditText->enEncode)
			{
				default:
				case SE_ANSI:
					pwc=strToWcs(ptr);
					SetWindowTextW(objCaller->hWnd,pwc);
					ehFree(pwc);
					break;

				case SE_UTF8:
					pwc=strDecode(ptr,SD_UTF8,NULL);
					SetWindowTextW(objCaller->hWnd,pwc);
					ehFree(pwc);
					break;

				case SE_UNICODE:
					SetWindowTextW(objCaller->hWnd,ptr);
					break;

			}
			break;

		case WS_REALGET: 

			iLen=GetWindowTextLength(objCaller->hWnd)+1;
			switch (psEditText->enEncode)
			{
				default:
				case SE_ANSI:
					if (!info) {info=iLen+1; ptr=ehAllocZero(info);}
					if (iLen>(info-1)) 
					{
						ehAlert("Field:%s, buffer insufficiente: %d>%d",
								objCaller->nome,
								iLen,
								info);
						ehError();
					}
					GetWindowText(objCaller->hWnd,ptr,info);
					break;

				case SE_UTF8:
					{
						BYTE *psz;
						pwc=ehAlloc(iLen<<1);
						GetWindowTextW(objCaller->hWnd,pwc,iLen);
						psz=strEncodeW(pwc,SE_UTF8,NULL);
						if (info&&strlen(psz)>(DWORD) info) ehError();
						if (info) {strcpy(ptr,psz); ehFree(psz);} else ptr=psz;
						ehFree(pwc);
					}
					break;

				case SE_UNICODE:
					pwc=ehAlloc(iLen<<1);
					GetWindowTextW(objCaller->hWnd,pwc,iLen);
					if (info&&iLen>info) ehError(); // Buffer troppo piccolo
					if (iLen) {wcscpy(ptr,pwc); ehFree(pwc);} else ptr=pwc;
					break;
			}
			return ptr;
			
			break;

		case WS_DISPLAY: 
			InvalidateRect(objCaller->hWnd,NULL,TRUE);
			break;

		case WS_SET_FONT:
			psFont=(EH_FONT *) ptr;
			SendMessage(objCaller->hWnd, WM_SETFONT, (WPARAM)psFont->hFont, MAKELPARAM(FALSE, 0));
			break;

		case WS_SET_ROWFOCUS:
			winSetFocus(objCaller->hWnd);
			break;

		case WS_EVENT:

			psEvent=(EH_EVENT *) ptr;
			//switch (psEvent->iEvent)
			//{
			//}
			break;


	}
	return NULL;
}


static HWND _LCreateEditText(EHZ_EDITTEXT * psEditText,DWORD dwStyle,HINSTANCE hInstance, HWND hwndParent)
{
	
	HWND        hwndEditText;
	BOOL        bSuccess = TRUE;

            
	hwndEditText = CreateWindowExW(	WS_EX_CONTROLPARENT|
									WS_EX_CLIENTEDGE,          // ex style
									WC_EDIT_TEXT,                // class name - defined in commctrl.h
									L"",                        // dummy text
									dwStyle,                   // style
									0,                         // x position
									0,                         // y position
									0,                         // width
									0,                         // height
									hwndParent,                // parent
									(HMENU) ID_STATICTEXT,        // ID
									sys.EhWinInstance,                   // instance
									NULL);                     // no extra data
	if(!hwndEditText) return NULL;
	
	SendMessage(hwndEditText,WM_SETFONT,(WPARAM) GetStockObject(ANSI_VAR_FONT),MAKELPARAM(TRUE, 0));
	SetWindowLong(hwndEditText,GWL_USERDATA,(LONG) psEditText);
	return hwndEditText;
}

static void _LInitialize(void)
{
	WNDCLASSW	wndClassW;
	
	ZeroFill(sEdit);

	GetClassInfoW(sys.EhWinInstance,L"Edit",&wndClassW);
	wndClassW.hInstance=sys.EhWinInstance;
	wndClassW.lpszClassName=WC_EDIT_TEXT;
	sEdit.funcTextOld=wndClassW.lpfnWndProc;
	wndClassW.lpfnWndProc=funcEditText;
	RegisterClassW(&wndClassW);
	sEdit.bReady=TRUE;
}

static LRESULT CALLBACK funcEditText(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    SINT nVirtKey;
	BOOL fShift;
	DWORD dwStyle;
	
	EHZ_EDITTEXT * psEditText;
	BOOL bBlur;

	psEditText=(EHZ_EDITTEXT *) GetWindowLong(hWnd,GWL_USERDATA);
	dwStyle=GetWindowLong(hWnd, GWL_STYLE);	
	switch (message)
	{
		case WM_CHAR:
			break;

		case WM_LBUTTONUP:
			ReleaseCapture();
			break;


		case WM_KEYDOWN:
		
/*			nVirtKey = (int) wParam;    
			sEdit.iHotKey=0;
			fShift=GetAsyncKeyState(VK_SHIFT);

			if (nVirtKey==VK_ESCAPE) sEdit.iHotKey=EF_ESC;
 			if (nVirtKey==VK_F9)	 sEdit.iHotKey=EF_F9; 
			if (nVirtKey==VK_PRIOR)  sEdit.iHotKey=EF_PAGEUP; // Page Up
			if (nVirtKey==VK_NEXT)   sEdit.iHotKey=EF_PAGEDOWN;  // Page Down
//			if (nVirtKey==EF_CR)	 {sEdit.iHotKey=EF_CR; return 0;}
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
			return 0;
			break;
			*/

		case WM_KEYUP:

			if  (dwStyle&ES_MULTILINE) break;

			bBlur=FALSE;
			nVirtKey = (int) wParam; 
			fShift=GetAsyncKeyState(VK_SHIFT);
			if (nVirtKey==VK_ESCAPE) {obj_putevent("%sESC",psEditText->psObj->nome); bBlur=TRUE;}
			if (nVirtKey==VK_RETURN) {obj_putevent("%sCR",psEditText->psObj->nome); bBlur=TRUE;}
			if (nVirtKey==VK_UP) {obj_putevent("%sUP",psEditText->psObj->nome);}
			if (nVirtKey==VK_DOWN) {obj_putevent("%sDOWN",psEditText->psObj->nome);}
			if (nVirtKey==VK_TAB) 
			{
				if (fShift) 
					obj_putevent("%s<-",psEditText->psObj->nome); 
					else
					obj_putevent("%s->",psEditText->psObj->nome); 
				bBlur=TRUE; 
			}
			if (bBlur) return 0;
			break;

		case WM_KILLFOCUS:
//			sEdit.fBreak=TRUE;
			break;
	}

	return CallWindowProc(sEdit.funcTextOld,hWnd,message,wParam,lParam);
}
