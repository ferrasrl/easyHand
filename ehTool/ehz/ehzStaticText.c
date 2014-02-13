//   -----------------------------------------------------------
//   | ehzStaticText
//   | Gestisce una StaticText di window                                             
//   |                                              
//   |										by Ferrà Srl 2005
//   -----------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/fbfile.h"
#include "/easyhand/inc/ehzStaticText.h"

static HWND _LCreateStaticText(HINSTANCE hInstance, HWND hwndParent);
static void SwitchView(HWND hwndListView, DWORD dwView);
static LRESULT CALLBACK funcStaticText(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

//
// ehzStaticText()
//
void * ehzStaticText(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr)
{
	EH_DISPEXT *psExt=ptr;
	EHZ_STATICTEXT *psStaticText;
	HFONT hFontOld;
	if (!objCalled) return NULL; // 

	psStaticText=objCalled->pOther;
	switch(cmd)
	{
		case WS_INF: return psStaticText;

		case WS_CREATE: 
			psStaticText=objCalled->pOther=ehAllocZero(sizeof(EHZ_STATICTEXT));
			psStaticText->enEncode=SE_ANSI; // Default
			psStaticText->uFormat=DT_LEFT|DT_NOPREFIX|DT_TOP|DT_WORDBREAK;
			psStaticText->colText=sys.ColorButtonText;
			psStaticText->colBack=-1;
			psStaticText->psFont=fontCreate("SMALL F",3,STYLE_NORMAL,FALSE,&psStaticText->bFreeFontEnd,NULL);
			break;

		case WS_DESTROY:
			if (psStaticText->bFreeFontEnd) fontDestroy(psStaticText->psFont,TRUE);
			ehFreePtr(&psStaticText->pwcText);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_SETFLAG:
			if (!strcmp(ptr,"ENCODE")) {

				psStaticText->enEncode=info;
			}
			break;


		case WS_DO: // Spostamento / Ridimensionamento
			MoveWindow(objCalled->hWnd,psExt->px+relwx,psExt->py+relwy,psExt->lx,psExt->ly,TRUE);
			break;

		case WS_REALSET: 
			ehFreePtr(&psStaticText->pwcText);
			switch (psStaticText->enEncode)
			{
				default:
				case SE_ANSI:
					psStaticText->pwcText=strToWcs(ptr);
					break;

				case SE_UTF8:
					psStaticText->pwcText=strDecode(ptr,SD_UTF8,NULL);
					break;
			}
			obj_vedisolo(objCalled->nome);
			break;

		case WS_DISPLAY: 
			//InvalidateRect(objCalled->hWnd,NULL,TRUE);
			if (!psStaticText->pwcText) break;

			if (psStaticText->psFont) hFontOld = SelectObject(psExt->hdc, psStaticText->psFont->hFont);
			if (psStaticText->colBack==-1) 
					SetBkMode(psExt->hdc,TRANSPARENT); 
					else 
					{
						SetBkMode(psExt->hdc,OPAQUE);
						SetBkColor(psExt->hdc,psStaticText->colBack);
					}
			SetTextColor(psExt->hdc,psStaticText->colText);
			DrawTextW(psExt->hdc,psStaticText->pwcText,wcslen(psStaticText->pwcText),&psExt->rClientArea,psStaticText->uFormat);
			if (psStaticText->psFont!=NULL) SelectObject(psExt->hdc, hFontOld);
			break;

		case WS_SET_FONT:
			psStaticText->psFont=(EH_FONT *) ptr;
			psStaticText->bFreeFontEnd=info;
			obj_vedisolo(objCalled->nome);
			break;

	}
	return NULL;
}


