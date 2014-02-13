//   ---------------------------------------------
//   | ZCMP_Form   
//   | ZoneComponent > Form
//   |                                              
//   | Gestisce un form tipo Html
//   |                                              
//   |							by Ferrà Srl 2010
//   ---------------------------------------------

#define ISOLATION_AWARE_ENABLED 1
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehzForm.h"

static struct {

	BOOL	bReady;
	WNDPROC funcTextOld;
	WNDPROC funcSelectOld;
	WNDPROC funcButtonOld;
	WNDPROC funcListOld;

} sForm = {0,0};
#define FORM_ID_OFFSET 1000

//
// Funzioni private locali
//
static void				_ehzFormInitialize(void);
static EH_IPT_INFO *	_LGetForm(HWND hWnd);
static BOOL				_fldDestroy(EH_FORM_FIELD * psFld);
static void				_fieldsReposition(EHZ_FORM *psForm);
static void				_getParamParser(EH_FORM_FIELD * psFld,CHAR *pszParam);
static EH_FORM_FIELD *	_fldSearch(EHZ_FORM *psForm,CHAR *pszName,INT *pIdx,INT iLine);
static void				_LFocusTab(EHZ_FORM *psForm,INT iDir);
static void				_keyTabManager(HWND hWnd,WPARAM wParam);
static void				_LFldSetFocus(EH_FORM_FIELD * psFld);
static WCHAR *			_LTextFldGet(EH_FORM_FIELD * psFld);
static WCHAR *			_LNumberFormat(EH_FORM_FIELD * psFld,double dNumber);
static WCHAR *			_LNumberText(EH_FORM_FIELD * psFld,BOOL bForInput);
static BOOL				_LSetFld(EHZ_FORM *psForm,EH_FORM_FIELD *psFldvoid, CHAR *pszValue);
static void				_datePickerSetField(EH_FORM_FIELD * psFld);
static BOOL				_isDataFld(EH_FORM_FIELD * psFld);
static void				_extNotify(EHZ_FORM * psForm,CHAR * pszEvent,EH_FORM_FIELD * psFld);

//
// Windows functions
//
LRESULT CALLBACK _funcFormControl(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

LRESULT CALLBACK _funcTitle(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK _funcZone(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK _funcTextField(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam); 
LRESULT CALLBACK _LfuncSelect(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam); 
LRESULT CALLBACK _LfuncButton(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK _LfuncList(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam); 

//
// metodi pubblici
//
static void		_this_Reset(void *this);
static BOOL		_this_Add(void *this, EN_FORM_IPT iType, CHAR *pszID, CHAR *pszText,CHAR *pszButton,CHAR *pszParam);
static BOOL		_this_addTitle(void *this, CHAR * pszName,CHAR *pszText,CHAR *pszParam);
static BOOL		_this_addEx(void *this, EN_FORM_IPT iType, CHAR *pszID, CHAR *pszText,CHAR *pszButton,CHAR *pszParam,void * (*funcExtern)(EH_OBJPARAMS));
static BOOL		_this_Show(void *this);
static BOOL		_this_Redraw(void *this);
static BOOL		_this_SetOptions(void *this,CHAR *pszName,EH_AR ar);
static BOOL		_this_Focus(void *this,CHAR *pszName);
static BOOL		_this_Set(void *this,CHAR *pszName,void *pszValue);
static BOOL		_this_SetNumber(void *this,CHAR *pszName,double dNumber);
static void		_this_SetTitle(void *this,CHAR *pszName,void *pszValue);
static void		_this_SetAfter(void *this,CHAR *pszName,void *pszValue);
static BOOL		_this_SetParams(void *this,CHAR *pszName,CHAR *pszParams,...);

static void *	_this_Get(void *this,CHAR *pszName);
static double	_this_GetNumber(void *this,CHAR *pszName);
static HWND		_this_getWnd(void *this,CHAR *pszName);
static EH_FORM_FIELD *	_this_getFld(void *this,CHAR *pszName);

static BOOL		_this_BlurNotify(void *this,CHAR *pszList);
static BOOL		_this_CharNotify(void *this,CHAR *pszList);

static void		_this_Clean(void *this,CHAR *pszFieldFocus,CHAR *lstFldNotClean);
static void 	_this_Enable(void *this,CHAR *pszName,BOOL bEnable);
static void		_this_Exclude(void * this,CHAR * pszNames,BOOL bExclude);
static void 	_this_Visible(void *this,CHAR *pszName,BOOL bVisible);
static void		_this_SetFunction(void *this,CHAR *pszField,void * (*funcExtern)(EH_OBJPARAMS));
static void		_this_SendMessage(void *this,CHAR *pszField,INT cmd, LONG info,void *str);
static void		_this_Refresh(void *this,CHAR *pszField);
static void		_this_setNotifyFunc(void *this,void * (*funcExtern)(void * this,EH_SRVPARAMS));
static void		_this_ensureVisible(void *this,CHAR * pszField);

#ifdef SQL_RS
	static BOOL		_this_SqlGetRs(void *this,SQL_RS rsSet);
	static BOOL		_this_SqlSelect(void *this,CHAR *pszQuery,...);
	static BOOL		_this_SqlUpdate(void *this,CHAR *pszFields,CHAR *pszQuery,...);
	static BOOL		_this_SqlInsert(void *this,CHAR *pszFields,CHAR *pszQuery,...);
#endif

#if (defined(_ADB32)||defined(_ADB32P))
	static BOOL	_this_adbRead(void *this,HDB hdb) ;
	static BOOL	_this_adbWrite(void *this, HDB hdb, CHAR * pszFields);
	static BOOL	_this_adbGetDifference(void *this, HDB hdb, HREC hRec, EH_LST lstField);
#endif
//
// ehzForm()
//
void * ehzForm(struct OBJ *objCalled,INT cmd,LONG info,void *ptr)
{
	EH_DISPEXT *psDex=ptr;
	EHZ_FORM *psForm;
	BOOL bAllocated;
	
	psForm=objCalled->pOther;
	if (!sForm.bReady) _ehzFormInitialize();
	switch(cmd)
	{
		//
		// Creazione della finestra Form
		//
		case WS_CREATE:

			objCalled->hWnd=CreateWindowEx( WS_EX_CONTROLPARENT,
											WC_EH_FORM,
											"Form",
											WS_TABSTOP | 
											WS_CHILD | 
//											WS_BORDER |
											WS_CLIPSIBLINGS |
									//		WS_VISIBLE |
											WS_VSCROLL|WS_HSCROLL
											,
											0,                         // x position
											0,                         // y position
											0,                         // width
											0,                         // height
											WindowNow(),                // parent
											(HMENU) ID_FORM,       // ID
											sys.EhWinInstance,         // Instance
											NULL);     

			objCalled->pOther=ehAllocZero(sizeof(EHZ_FORM));
			psForm=objCalled->pOther;

			psForm->enEncode=SE_ANSI;
			psForm->psObj=objCalled;
			psForm->idxFocus=-1;
			psForm->psFldFocus=NULL;
			psForm->iTitleMin=80;
			SetWindowLong(objCalled->hWnd,GWL_USERDATA,(LONG) psForm);

			psForm->wnd=objCalled->hWnd;
			DMIReset(&psForm->dmiField);
			DMIOpen(&psForm->dmiField,RAM_AUTO,100,sizeof(EH_FORM_FIELD),"FormField");
			psForm->arField=DMILock(&psForm->dmiField,NULL);

			// Like C++ ;-)
			psForm->Reset=_this_Reset;
			psForm->Add=_this_Add;

			psForm->Show=_this_Show;
			psForm->Redraw=_this_Redraw;
			psForm->SetOptions=_this_SetOptions;
			psForm->Focus=_this_Focus;
			psForm->Set=_this_Set;
			psForm->SetTitle=_this_SetTitle;
			psForm->SetAfter=_this_SetAfter;
			psForm->SetParams=_this_SetParams;
			psForm->SetNumber=_this_SetNumber;

			psForm->Get=_this_Get;
			psForm->GetNumber=_this_GetNumber;
			psForm->getWnd=_this_getWnd;
			psForm->getFld=_this_getFld;

			psForm->BlurNotify=_this_BlurNotify;
			psForm->CharNotify=_this_CharNotify;
			psForm->Clean=_this_Clean;
			psForm->Enable=_this_Enable;
			psForm->Exclude=_this_Exclude;
			psForm->Visible=_this_Visible;
			psForm->SetFunction=_this_SetFunction;
			psForm->SendMessage=_this_SendMessage;
			psForm->Refresh=_this_Refresh;
			psForm->addTitle=_this_addTitle;
			psForm->addEx=_this_addEx;
			psForm->setNotifyFunc=_this_setNotifyFunc;
			psForm->ensureVisible=_this_ensureVisible;

#ifdef SQL_RS
			psForm->SqlGetRs=_this_SqlGetRs;
			psForm->SqlSelect=_this_SqlSelect;
			psForm->SqlUpdate=_this_SqlUpdate;
			psForm->SqlInsert=_this_SqlInsert;
#endif

#if (defined(_ADB32)||defined(_ADB32P))

			psForm->adbRead=_this_adbRead;
			psForm->adbWrite=_this_adbWrite;
			psForm->adbGetDifference=_this_adbGetDifference;

#endif
			psForm->psFontTitle=fontCreate("#Tahoma",13,STYLE_NORMAL,TRUE,&bAllocated,NULL);
			psForm->psFontInput=fontCreate("#Arial",15,STYLE_BOLD,TRUE,&bAllocated,NULL);
			SendMessageW(psForm->wnd,WM_SETFONT,(WPARAM) psForm->psFontTitle->hFont,MAKELPARAM(TRUE, 0));
			psForm->iCellPadding=2;
			psForm->recFormPadding.left=5;
			psForm->recFormPadding.right=5;
			psForm->recFormPadding.bottom=5;
			break;

		case WS_DESTROY: 
			/*
			ShowWindow(psForm->wnd,SW_HIDE);
			for (a=0;a<psForm->dmiField.Num;a++) {
				DMIRead(&psForm->dmiField,a,&sFld);
				_fldDestroy(&sFld);
			}
			ARDestroy(psForm->arBlurNotify);
			*/
			_this_Reset(psForm);
			fontDestroy(psForm->psFontTitle,TRUE);
			fontDestroy(psForm->psFontInput,TRUE);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_DO: 
			MoveWindow(objCalled->hWnd,psDex->px,psDex->py,psDex->lx,psDex->ly,true);
			break;

		case WS_INF: 
			return psForm;

	}
	return NULL;
}

//
// -_this_Add() > Aggiunge un campo in coda al form
//
static BOOL _this_Add(void *this, EN_FORM_IPT iType, CHAR *pszName, CHAR *pszText,CHAR *pszButton,CHAR *pszParam) {

	EHZ_FORM * psForm=this;
	EH_FORM_FIELD sFld,*psFld;
	EH_IPT_INFO * psIptInfo;
	INT idx,idWindow;
	WCHAR *pwcs;
	EH_FONT *psFont;
	DWORD dwParam;
	
	_(sFld);
	sFld.psForm=psForm;
	sFld.iType=iType&FLD_TYPE_MASK;
	sFld.pszName=strDup(pszName);
	sFld.pszText=strDup(pszText);
	sFld.pszButton=strDup(pszButton);
	sFld.bAppend=(iType&FLD_APPEND)?1:0;
	sFld.bQueryNot=(iType&FLD_QNOT)?1:0;

	switch (sFld.iType) {

		case FLD_TEXT:
			sFld.enClass=FCLS_TEXT;
			break;

		case FLD_DATE:
			sFld.enClass=FCLS_DATE;
			break;

		case FLD_TEXTAREA:
			sFld.iTextRows=3;
			sFld.enClass=FCLS_TEXT;
			break;

		case FLD_NUMBER:
		case FLD_PASSWORD:
			sFld.iWidth=80;
			sFld.enClass=FCLS_TEXT;
			break;

		case FLD_SELECT:
			sFld.enClass=FCLS_SELECT;
			break;

		case FLD_LIST:
			sFld.enClass=FCLS_LIST;
			break;

		case FLD_BUTTON:
		case FLD_CHECKBOX:
		case FLD_RADIO: // da fare <----------------
			sFld.enClass=FCLS_BUTTON;
			break;
	}
	
	//
	// Parser dei parametri aggiuntivi
	//
	sFld.iTitleWidth=-1; // Usa il default
	sFld.enTitleAlign=DPL_LEFT;
	sFld.dwTitleParam=DT_VCENTER;
	_getParamParser(&sFld,pszParam);

	idx=psForm->dmiField.Num;
	idWindow=FORM_ID_OFFSET+psForm->dmiField.Num;
	DMIUnlock(&psForm->dmiField);
	DMIAppendDyn(&psForm->dmiField,&sFld);
	psForm->arField=DMILock(&psForm->dmiField,NULL);
	psFld=&psForm->arField[idx];

	//
	// > Creo Titolo
	//
	dwParam=WS_TABSTOP | 
			WS_CHILD | 
			WS_CLIPSIBLINGS |
			WS_VISIBLE
			//|WS_BORDER
			;
/*
	switch (psFld->enTitleAlign) {
		
		default:
		case DPL_LEFT: dwParam|=SS_LEFT; break;
		case DPL_RIGHT: dwParam|=SS_RIGHT; break;
		case DPL_CENTER: dwParam|=SS_CENTER; break;

	}
*/	
	if (!strEmpty(psFld->pszText)) {


		psFld->wndTitle=CreateWindowEx(	0, // WS_EX_CONTROLPARENT,
										WC_EH_TITLE,
										psFld->pszText,
										dwParam,
										0,                         // x position
										0,                         // y position
										0,                         // width
										0,                         // height
										psForm->wnd,                // parent
										(HMENU) 0,       // ID
										sys.EhWinInstance,         // Instance
										NULL);
		SendMessageW(psFld->wndTitle,WM_SETFONT,(WPARAM) psForm->psFontTitle->hFont,MAKELPARAM(TRUE, 0));
		SetWindowLong(psFld->wndTitle,GWL_USERDATA,(LONG) psFld);
	}

	//
	// > Creo il campo
	//
	psIptInfo=ehAllocZero(sizeof(EH_IPT_INFO));
	psIptInfo->psForm=psForm;
	psIptInfo->idxField=idx;
	
	psFont=psForm->psFontInput;
 	if (psFld->iType==FLD_CHECKBOX) psFont=psForm->psFontTitle;
	if (psFld->psFont) psFont=psFld->psFont;
	psFld->psFontApply=psFont;
	psFld->pvIptInfo=psIptInfo;
	if (psFld->iType!=FLD_TITLE) psFld->psParent=psForm->psGrpTitle;

	switch(psFld->iType) {
	
		//
		// text
		//
		case FLD_TEXT:

			psFld->wndInput=CreateWindowExW(	WS_EX_CLIENTEDGE,
												WC_EH_FORM_TEXT,
												L"",//lpBuffer,
												WS_CHILD|WS_VISIBLE|ES_LEFT|ES_AUTOHSCROLL,
												0,0,0,0,
												psForm->wnd,
												(HMENU) idWindow,
												sys.EhWinInstance,
												0);
			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psIptInfo);
			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
			break;


		//
		// FLD_DATE
		//
		// http://msdn.microsoft.com/en-us/library/windows/desktop/dd319849(v=vs.85).aspx
		case FLD_DATE:
			
			psFld->wndInput = CreateWindowEx(0,
								   DATETIMEPICK_CLASS,
								   TEXT("DateTime"),
								   WS_CHILD|WS_VISIBLE|DTS_SHOWNONE,
								   0,0,0,0,
								   psForm->wnd,
								   (HMENU) idWindow,
								   sys.EhWinInstance,
								   NULL);
			//DateTime_SetFormat( psFld->wndInput, "dd/MM/yyyy");
			_datePickerSetField(psFld);
//			DateTime_SetFormat( psFld->wndInput, " ");
			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psIptInfo);
			psIptInfo->psFld=psFld;
			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
//			DateTime_SetMonthCalColor(psFld->wndInput,MCSC_TRAILINGTEXT,RGB(255,255,0));

			break;

		//
		// number
		//
		case FLD_NUMBER:

			pwcs=_LNumberFormat(psFld,(double) 0);
			psFld->wndInput=CreateWindowExW(	WS_EX_CLIENTEDGE,
												WC_EH_FORM_TEXT,
												pwcs,//lpBuffer,
												WS_CHILD|WS_VISIBLE|ES_RIGHT|ES_AUTOHSCROLL,
												0,0,0,0,
												psForm->wnd,
												(HMENU) idWindow,
												sys.EhWinInstance,
												0);
			ehFree(pwcs);
			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psIptInfo);
			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
			break;

		//
		// textarea
		//
		case FLD_TEXTAREA:
			psFld->wndInput=CreateWindowExW(WS_EX_CLIENTEDGE,
											WC_EH_FORM_TEXT,
											L"",//lpBuffer,
											WS_CHILD|WS_VSCROLL|WS_VISIBLE|ES_LEFT|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_MULTILINE,
											0,0,0,0,
											psForm->wnd,
											(HMENU) idWindow,
											sys.EhWinInstance,
											0);
			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psIptInfo);
			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
			break;

		//
		// select
		//
		case FLD_SELECT:
			psFld->wndInput=CreateWindowEx(		0,//WS_EX_CLIENTEDGE,
												WC_EH_FORM_SELECT,
												"",//lpBuffer,
												WS_CHILD|
												WS_VSCROLL|
												WS_TABSTOP|
												WS_VISIBLE|
												CBS_OWNERDRAWFIXED|
												//CBS_OWNERDRAWVARIABLE|
												CBS_DROPDOWNLIST
												//CBS_AUTOHSCROLL
												, // Mha ?
												0,0,0,250,
												psForm->wnd,
												(HMENU) idWindow,
												sys.EhWinInstance,
												0);

			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psIptInfo);
			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
			SendMessage(psFld->wndInput,CB_SETITEMHEIGHT,-1,(psFont->iHeight+2)); // Item selezionato (altezza riga)
			SendMessage(psFld->wndInput,CB_SETITEMHEIGHT,0,(psFont->iHeight+4));  // Altezza degli item
			break;

		//
		// list
		//
		case FLD_LIST:

			psFld->wndInput=CreateWindowEx(		WS_EX_CLIENTEDGE,
												WC_EH_FORM_LIST,
												"",
												WS_CHILD|
												WS_VSCROLL|
												WS_TABSTOP|
												WS_VISIBLE|
												LBS_COMBOBOX|
												LBS_NOTIFY|
												LBS_OWNERDRAWFIXED
												, // Mha ?
												0,0,0,0,
												psForm->wnd,
												(HMENU) idWindow,
												sys.EhWinInstance,
												0);

			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psIptInfo);
			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
			//SendMessage(psFld->wndInput,LB_SETITEMHEIGHT,-1,(psFont->iHeight+2)); // Item selezionato (altezza riga)
			SendMessage(psFld->wndInput,LB_SETITEMHEIGHT,0,(psFont->iHeight+4));  // Altezza degli item
			break;


		//
		// Checkbox
		//
		case FLD_CHECKBOX:

			psFld->wndInput=CreateWindowEx(	0,
											WC_EH_FORM_BUTTON,
											psFld->pszButton,//lpBuffer,
											WS_TABSTOP|WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, // |WS_BORDER
											0,0,0,0,
											psForm->wnd,
											(HMENU) idWindow,
											sys.EhWinInstance,
											0);

			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psIptInfo);
//			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psForm->psFontTitle->hFont,MAKELPARAM(TRUE, 0));
			SendMessage(psFld->wndInput,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
			break;

		//
		// Button
		//
		case FLD_BUTTON:

			psFld->wndInput=CreateWindowEx(	0,
											WC_EH_FORM_BUTTON,
											psFld->pszButton,//lpBuffer,
											WS_TABSTOP|WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
											0,0,0,0,
											psForm->wnd,
											(HMENU) idWindow,
											sys.EhWinInstance,
											0);

			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psIptInfo);
			SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psForm->psFontTitle->hFont,MAKELPARAM(TRUE, 0));
			//Button_SetNote(psFld->wndInput,L"Prova");
			break;

		case FLD_TITLE:
			SetWindowLong(psFld->wndTitle,GWL_USERDATA,(LONG) psFld);
	//		SendMessageW(psFld->wndTitle,WM_SETFONT,(WPARAM) psFont->hFont,MAKELPARAM(TRUE, 0));
			ehFree(psIptInfo);
			psFld->pvIptInfo=NULL;
			psForm->psGrpTitle=psFld;
			break;


		default:
			ehFree(psIptInfo);
			psFld->pvIptInfo=NULL;
			break;
	}

	//
	// Controllo aggiuntivi sul testo
	//
	if (psFld->enClass==FCLS_TEXT) {

		if (psFld->iMaxChar) {
			SendMessage(psFld->wndInput,EM_SETLIMITTEXT,psFld->iMaxChar,0);
		}
		SendMessage(psFld->wndInput,EM_SETREADONLY,psFld->bReadOnly,0);

		//
		// > Creo il testo dopo il campo (se presente)
		//
		psFld->wndAfter=CreateWindowEx(	0,
										"STATIC",
										psFld->pszButton,
										//WS_TABSTOP | 
										WS_CHILD | 
										WS_CLIPSIBLINGS |
										WS_VISIBLE|
					//					WS_BORDER 
										//SS_VCENTER|
										SS_WORDELLIPSIS
										,
										0,                         // x position
										0,                         // y position
										0,                         // width
										0,                         // height
										psForm->wnd,                // parent
										(HMENU) 0,       // ID
										sys.EhWinInstance,         // Instance
										NULL);
		SendMessageW(psFld->wndAfter,WM_SETFONT,(WPARAM) psForm->psFontTitle->hFont,MAKELPARAM(TRUE, 0));

	}

	if (psFld->psFont&&psFld->wndInput)
		SendMessageW(psFld->wndInput,WM_SETFONT,(WPARAM) psFld->psFont->hFont,MAKELPARAM(TRUE, 0));

	return FALSE;
}
//
// _this_addTitle()
//
static BOOL		_this_addTitle(void *this, CHAR * pszName,CHAR *pszText,CHAR *pszParam) {

	return _this_Add(this, FLD_TITLE, pszName, pszText,"",pszParam);

}
//
// _this_addEx()
//
static BOOL	 _this_addEx(void *this, EN_FORM_IPT iType, CHAR *pszID, CHAR *pszText,CHAR *pszButton,CHAR *pszParam,void * (*funcExtern)(struct OBJ *psObj,EN_MESSAGE enMess,LONG lParam,void *pVoid)) {

	EHZ_FORM * psForm=this;
	EH_FORM_FIELD * psFld;
	_this_Add(this, iType, pszID, pszText, pszButton, pszParam);
	psFld=_fldSearch(psForm,pszID,NULL,__LINE__);

	// Creo il bridge
	if (funcExtern) {
		psFld->funcExtern=funcExtern;
		_(psFld->sObj);
		psFld->sObj.funcExtern=funcExtern;
		strCpy(psFld->sObj.nome,psFld->pszName,sizeof(psFld->sObj.nome));
		psFld->funcExtern(&psFld->sObj,WS_CREATE,0,NULL);
		psFld->funcExtern(&psFld->sObj,WS_OPEN,0,NULL);

		//	Se l'oggetto non prevede una finestra lavora con display
		//	gli assegno una finestra di output automatica
		if (!psFld->sObj.hWnd) { 
		
			DWORD dwParam=//WS_BORDER|
				WS_TABSTOP | 
					WS_CHILD | 
					WS_CLIPSIBLINGS |
					WS_VISIBLE;

			psFld->sObj.hWnd=CreateWindowEx(0,//WS_EX_CONTROLPARENT,
											WC_EH_ZONE,
											"",
											dwParam,
											// |
											//	WS_BORDER |
											// SS_WORDELLIPSIS
											0,                         // x position
											0,                         // y position
											0,                         // width
											0,                         // height
											psForm->wnd,                // parent
											(HMENU) 0,       // ID
											sys.EhWinInstance,         // Instance
											NULL);
			psFld->wndInput=psFld->sObj.hWnd; 
			SetWindowLong(psFld->wndInput,GWL_USERDATA,(LONG) psFld);

		} else {

			psFld->wndInput=psFld->sObj.hWnd; 
			SetParent(psFld->wndInput,psForm->wnd); // Cambio il parent
		}
	}
	return false;
}

//
// _alignParser
//
EN_DPL _alignParser(CHAR * psz) {

	EN_DPL enDpl=DPL_LEFT;
	if (!strCaseCmp(psz,"left")) enDpl=DPL_LEFT;
	else if (!strCaseCmp(psz,"right")) enDpl=DPL_RIGHT;
	else if (!strCaseCmp(psz,"center")) enDpl=DPL_CENTER;
	return enDpl;
}

//
// _alignParser
//
DWORD _valignParser(CHAR * psz) {

	DWORD dw=0;
	if (!strCaseCmp(psz,"top")) dw=DT_TOP;
	else if (!strCaseCmp(psz,"center")) dw=DT_VCENTER;
	else if (!strCaseCmp(psz,"bottom")) dw=DT_BOTTOM;
	return dw;
}

//
// _getParamParser()
//
static void _getParamParser(EH_FORM_FIELD * psFld,CHAR *pszParam) {

	EH_AR arRow,arFld;
	INT a,iToken;
	CHAR *pStart;

	if (strEmpty(pszParam)) return;

	arRow=ARCreate(pszParam,";",NULL);
	
	for (a=0;arRow[a];a++) {
		
		arFld=ARCreate(strTrim(arRow[a]),":",&iToken);
		if (iToken==2) {
			strTrim(arFld[0]); strTrim(arFld[1]); 
			if (!strcmp(arFld[0],"width")) 
			{
				if (!strCmp(arFld[1],"max")) 
				{
					psFld->bWidthPerc=false; 
					psFld->iWidth=0;
				}
				else if (strstr(arFld[1],"%")) 
				{
					psFld->bWidthPerc=true; 
					psFld->iWidth=atoi(strOmit(arFld[1],"%"));
				} else {
					psFld->bWidthPerc=false; 
					psFld->iWidth=atoi(arFld[1]);
				}
			}
			else if (!strcmp(arFld[0],"minWidth")) psFld->iMinWidth=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"maxWidth")) psFld->iMaxWidth=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"height")) psFld->iHeight=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"maxchar")) psFld->iMaxChar=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"afterWidth")) psFld->iAfterWidth=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"color")) psFld->colText=colorWeb(arFld[1]);
			else if (!strcmp(arFld[0],"readonly")) psFld->bReadOnly=!strcmp(arFld[1],"true")?true:false;
			else if (!strcmp(arFld[0],"exclude")) psFld->bExclude=!strcmp(arFld[1],"true")?true:false;
			else if (!strcmp(arFld[0],"visible")) psFld->bVisible=!strcmp(arFld[1],"true")?true:false;
			else if (!strcmp(arFld[0],"row")) psFld->iTextRows=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"rows")) psFld->iTextRows=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"format")) 
			{
				CHAR *p;
				if (psFld->iType!=FLD_NUMBER) ehError();
				pStart=arFld[1];
				// t: thousand separator
				if (*pStart=='t') {psFld->bNumberThousandSep=1; pStart++;}
				p=strstr(pStart,"."); 
				if (p) {*p=0; psFld->iNumberDecimal=atoi(p+1);}
				psFld->iNumberSize=atoi(pStart);
			}
			
			else if (!strcmp(arFld[0],"margin-left")) psFld->rcMargin.left=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"margin-top")) psFld->rcMargin.top=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"append")) psFld->bAppend=!strcmp(arFld[1],"true")?TRUE:FALSE;
			
			else if (!strcmp(arFld[0],"font")) 
			{
				INT iCount;
				DWORD dwStyle,dwSize;
				CHAR szFont[200];
				EH_AR ar=ARCreate(arFld[1]," ",&iCount);
				if (iCount==3) {

					INT f;
					EH_AR arFont;
					if (!strcmp(ar[0],"normal")) dwStyle=STYLE_NORMAL;
					if (!strcmp(ar[0],"bold")) dwStyle=STYLE_BOLD;
					dwSize=atoi(strKeep(ar[1],"0123456789"));

					arFont=ARFSplit(ar[2],",");
					for (f=0;arFont[f];f++) {

						CHAR *	pszName=arFont[f];
						BOOL	bExist=false;
						while (strReplace(pszName,"_"," "));
						// Controllo se il font esiste
						bExist=fontSearch(pszName,dwStyle);
						if (bExist) {
							sprintf(szFont,"#%s",pszName);
							if (psFld->psFont) fontDestroy(psFld->psFont,TRUE);
							psFld->psFont=fontCreate(szFont,dwSize,dwStyle,TRUE,NULL,NULL);
							break;
						}
					}
					ehFree(arFont);

				}
				ARDestroy(ar);
			}
			else if (!strcmp(arFld[0],"title-width")) psFld->iTitleWidth=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"title-align")) psFld->enTitleAlign=_alignParser(arFld[1]);
			else if (!strcmp(arFld[0],"title-style")) psFld->iTitleStyle=atoi(arFld[1]);
			else if (!strcmp(arFld[0],"title-valign")) psFld->dwTitleParam=_valignParser(arFld[1]);
			else if (!strcmp(arFld[0],"title-group")) psFld->bTitleGroup=!strcmp(arFld[1],"true")?true:false;
			else if (!strcmp(arFld[0],"title-close")) psFld->bTitleClose=!strcmp(arFld[1],"true")?true:false;
#ifdef _DEBUG
			else 
				alert("param ? [%s] = [%s]",psFld->pszName,arRow[a]);
#endif

		}
		ARDestroy(arFld);
	}
	ARDestroy(arRow);

}

//
// -_this_FldDestroy() > Libera le risorse impegnate con un campo
//
static BOOL _fldDestroy(EH_FORM_FIELD * psFld)
{
	ehFreePtr(&psFld->pszName);
	ehFreePtr(&psFld->pszText);
	ehFreePtr(&psFld->pszButton);
	ehFreePtr(&psFld->pszValue);
//	ehFreePtr(&psFld->pwcValue);

	if (psFld->wndTitle) DestroyWindow(psFld->wndTitle);
	if (psFld->wndInput) DestroyWindow(psFld->wndInput);
	if (psFld->wndAfter) DestroyWindow(psFld->wndAfter);
	if (psFld->funcExtern) 
	{
		psFld->funcExtern(&psFld->sObj,WS_CLOSE,0,NULL);
		psFld->funcExtern(&psFld->sObj,WS_DESTROY,0,NULL);
		psFld->funcExtern=NULL;
	}
	if (psFld->psFont) fontDestroy(psFld->psFont,TRUE);
	ehFreePtr(&psFld->pvIptInfo);
	return FALSE;
}

//
// _ehzFormInitialize()
//
static void _ehzFormInitialize(void) {

	WNDCLASSEX	wc;
	WNDCLASSW	wndClassW;
	WNDCLASS	wndClass;

	//
	// Funzione principale del Form
	//
	_(wc);
	wc.cbSize        = sizeof(wc);
	wc.lpszClassName = WC_EH_FORM;
	wc.style         = CS_PARENTDC;// CS_BYTEALIGNCLIENT| CS_HREDRAW | CS_VREDRAW; // | CS_PARENTDC;
	wc.lpfnWndProc   = _funcFormControl;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	wc.lpszMenuName  = NULL;
	wc.hIconSm       = NULL;
	RegisterClassEx(&wc);

	//
	// Funzione di stampa del titolo
	//
	_(wc);
	wc.cbSize        = sizeof(wc);
	wc.lpszClassName = WC_EH_TITLE;
	wc.style         = CS_PARENTDC|CS_HREDRAW | CS_VREDRAW;;// CS_BYTEALIGNCLIENT| CS_HREDRAW | CS_VREDRAW; // | CS_PARENTDC;
	wc.lpfnWndProc   = _funcTitle;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;// CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	wc.lpszMenuName  = NULL;
	wc.hIconSm       = NULL;
	RegisterClassEx(&wc);

	//
	// Funzione di stampa di una zona
	//
	_(wc);
	wc.cbSize        = sizeof(wc);
	wc.lpszClassName = WC_EH_ZONE;
	wc.style         = CS_BYTEALIGNCLIENT| CS_HREDRAW | CS_VREDRAW; // | CS_PARENTDC;
	wc.lpfnWndProc   = _funcZone;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
	wc.lpszMenuName  = NULL;
	wc.hIconSm       = NULL;
	RegisterClassEx(&wc);

	//
	// TextInput > Edit superClassing
	//
	GetClassInfoW(sys.EhWinInstance,L"Edit",&wndClassW);
	wndClassW.hInstance=sys.EhWinInstance;
	wndClassW.lpszClassName=WC_EH_FORM_TEXT;
	sForm.funcTextOld=wndClassW.lpfnWndProc;
	wndClassW.lpfnWndProc=_funcTextField;
	RegisterClassW(&wndClassW);

	//
	// Select > Combo superClassing
	//
	GetClassInfo(sys.EhWinInstance,"ComboBox",&wndClass);
	wndClass.hInstance=sys.EhWinInstance;
	wndClass.lpszClassName=WC_EH_FORM_SELECT;
	sForm.funcSelectOld=wndClass.lpfnWndProc;
	wndClass.lpfnWndProc=_LfuncSelect;
	RegisterClass(&wndClass);

	//
	// List > Combo superClassing
	//

	GetClassInfo(sys.EhWinInstance,"ListBox",&wndClass);
	wndClass.hInstance=sys.EhWinInstance;
	wndClass.lpszClassName=WC_EH_FORM_LIST;
	sForm.funcListOld=wndClass.lpfnWndProc;
	wndClass.lpfnWndProc=_LfuncList;
	RegisterClass(&wndClass);

	//
	// Button > Combo superClassing
	//
	GetClassInfo(sys.EhWinInstance,"Button",&wndClass);
	wndClass.hInstance=sys.EhWinInstance;
	wndClass.lpszClassName=WC_EH_FORM_BUTTON;
	sForm.funcButtonOld=wndClass.lpfnWndProc;
	wndClass.lpfnWndProc=_LfuncButton;
	RegisterClass(&wndClass);

	sForm.bReady=TRUE;
}




static void _datePickerSetField(EH_FORM_FIELD * psFld) {


	SYSTEMTIME sST;
	DWORD dw;
	dw=DateTime_GetSystemtime(psFld->wndInput,&sST);
	if (dw==GDT_NONE) 
		DateTime_SetFormat(psFld->wndInput, " ");
		else
		DateTime_SetFormat( psFld->wndInput, "dd/MM/yyyy"); 

}

//
// Form > Funzione delegata prinxipale
//
static void _LSelectDrawItem(LPDRAWITEMSTRUCT psDis);
LRESULT CALLBACK _funcFormControl(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	EHZ_FORM *psForm;
	EH_FORM_FIELD * psFld;
	LPDRAWITEMSTRUCT psDis;
	WORD wNotifyCode,wID;
	LPNMHDR pnmh;
	EH_IPT_INFO * psIptInfo;

	psForm=(EHZ_FORM *) GetWindowLong(hWnd,GWL_USERDATA);

	switch (message)
	{
		case WM_CREATE:
			break;

		case WM_DESTROY: 
			break;

		case WM_SIZE:
			if (psForm) _fieldsReposition(psForm);
			break;

	 // --------------------------------------------------------------------------------
     // Controllo Scorrimento VERTICALE                                                |
     // --------------------------------------------------------------------------------
		case WM_VSCROLL:
			psForm->ofsVert=ehScrollTranslate(hWnd,SB_VERT,wParam,psForm->ofsVert,psForm->sizForm.cy,psForm->sizClient.cy,8,false);
			break;

		case WM_HSCROLL:
			psForm->ofsHorz=ehScrollTranslate(hWnd,SB_HORZ,wParam,psForm->ofsHorz,psForm->sizForm.cx,psForm->sizClient.cx,8,false);
			break;

		case WM_MOUSEWHEEL:
			{
				WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
				short zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
				INT a,iRowsScroll = (INT) zDelta*psForm->sizClient.cy/5/8;
				printf("zDelta:%d",zDelta);
				if (zDelta>0) {
					
					if (iRowsScroll<1) iRowsScroll=1;
					for (a=0;a<iRowsScroll;a++) SendMessage(hWnd,WM_VSCROLL,SB_LINEUP,0); //bContinue=TRUE;
				}
				else 
				{
					iRowsScroll=-iRowsScroll; if (iRowsScroll<1) iRowsScroll=1;
					for (a=0;a<iRowsScroll;a++) SendMessage(hWnd,WM_VSCROLL,SB_LINEDOWN,0); //bContinue=TRUE;
				}
			}
			break;
				
		case WM_COMMAND: 

			wNotifyCode = HIWORD(wParam);
			wID=LOWORD(wParam); // id del oggetto
			if (wID<FORM_ID_OFFSET) break;
			wID-=FORM_ID_OFFSET;
			if (wID>=psForm->dmiField.Num) break;
			psFld=&psForm->arField[wID];
			switch (psFld->iType) {

//					lRes=Button_GetState(psFld->wndInput);
//					obj_putevent("%s.%s",psForm->lpObj->nome,psFld->pszName);//,(lRes&BST_CHECKED)?"ON":"OFF");
//					break;
		
				case FLD_CHECKBOX:
				case FLD_BUTTON:
					obj_putevent("%s.%s",psForm->psObj->nome,psFld->pszName);
					_extNotify(psForm,"inputChange",psFld);
					break;

				case FLD_SELECT:
					if (wNotifyCode==CBN_SELENDOK) obj_putevent("%s.%s",psForm->psObj->nome,psFld->pszName);
					break;

				case FLD_LIST:
					// LBN_SELCHANGE
//					printf("> %d" CRLF,wNotifyCode);
					if (wNotifyCode==LBN_SELCHANGE) obj_putevent("%s.%s",psForm->psObj->nome,psFld->pszName);
					break;
				
				default: break;
			}
			
			break;


		case WM_DRAWITEM: 
				psDis=(LPDRAWITEMSTRUCT) lParam;
				switch (psDis->CtlType) {

					case ODT_LISTBOX:
					case ODT_COMBOBOX: 
						_LSelectDrawItem(psDis); 
						return TRUE;
				}
				break;

		case WM_CTLCOLOREDIT:
			{
			    HDC hdcEdit = (HDC) wParam; 
			    //HWND hwndEdit = (HWND) lParam; 
				EH_IPT_INFO * psIptInfo=_LGetForm((HWND) lParam);
				if (psIptInfo) {
					if (psIptInfo->psFld->colText) {
						SetTextColor(hdcEdit, psIptInfo->psFld->colText);
						return 0;
					}
				} 
			}
			break;

		case WM_NOTIFY:
            pnmh = (LPNMHDR) lParam ;
			
			switch (pnmh->code) {
			
				case NM_SETFOCUS:
					psIptInfo=(EH_IPT_INFO *) GetWindowLong(pnmh->hwndFrom,GWL_USERDATA);
					if (psIptInfo->psFld) {
					
						if (psIptInfo->psFld->enClass==FCLS_DATE) {
							DateTime_SetFormat( psIptInfo->psFld->wndInput, "dd/MM/yyyy");
						}
					}
					break;

				case NM_KILLFOCUS:
					psIptInfo=(EH_IPT_INFO *) GetWindowLong(pnmh->hwndFrom,GWL_USERDATA);
					if (psIptInfo->psFld) {
						_datePickerSetField(psIptInfo->psFld);
					}
					break;
			}
				
			break;
	}
	
	return(DefWindowProc(hWnd, message, wParam, lParam));
}

//
// _funcTitle()
//
LRESULT CALLBACK _funcTitle(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	EHZ_FORM * psForm;
	EH_FORM_FIELD * psFld;
	DWORD dwParam;
	RECTD rcd;
	SIZE sizRow;

	psFld=(EH_FORM_FIELD *) GetWindowLong(hWnd,GWL_USERDATA);
	switch (message)
	{

//			case WM_LBUTTONDOWN:
//			case WM_RBUTTONDOWN:
//			case WM_MOUSEMOVE:
//			case WM_RBUTTONUP:
		case WM_LBUTTONUP:
//			case WM_LBUTTONDBLCLK:
				psForm=psFld->psForm;
				psFld->bTitleClose^=1;
				psForm->Redraw(psForm);
				_extNotify(psForm,"titleChange",psFld);
		//		printf("qui [%d]",psFld->bTitleClose);
				break;

		case WM_SIZE:
			// InvalidateRect(hWnd,NULL,false);
			break;


		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				RECT rc,rcx;
				RECT rcGradient;
				HDC hdc=BeginPaint(hWnd,&ps);

				GetClientRect(hWnd,&rc);
				switch (psFld->iTitleStyle) {

					case 0: // Default
						dwParam=psFld->dwTitleParam;
						if (strstr(psFld->pszText,"\n")) dwParam|=DT_WORDBREAK; else dwParam|=(DT_SINGLELINE|DT_END_ELLIPSIS);
						psForm=psFld->psForm;
						dcDrawText(hdc,&rc,0,-1,psForm->psFontTitle,1,psFld->pszText,strlen(psFld->pszText),psFld->enTitleAlign,dwParam);
						break;

					case 1: // Light
						// dcLinePlus(hdc,0,1,rc.right,1,AlphaColor(255,sys.Color3DHighLight),1);
						/*
						rectCopy(rcx,rc); rcx.bottom=rcx.top+4; rectCopy(rcGradient,rcx);
						rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;
						dcBoxGradient(	hdc,
										&rcx,&rcGradient,
										AlphaColor(0xc0,sys.Color3DShadow),
										AlphaColor(0,sys.Color3DShadow));
*/
						rectCopy(rcx,rc); 
						sizeCalc(&sizRow,&rc);
						rectCopy(rcGradient,rcx);
						rcGradient.right=rcGradient.left; rcGradient.top--; rcGradient.bottom++;
						if (!psFld->bTitleGroup) {
							dcBoxGradient(	hdc,
											&rcx,&rcGradient,
											AlphaColor(0xa0,sys.Color3DHighLight),
											AlphaColor(0,sys.Color3DHighLight));
							dcLinePlus(hdc,0,0,rc.right,0,AlphaColor(255,sys.Color3DShadow),1,0);

							dcTextout(hdc,1+2,3,sys.Color3DHighLight,-1,psFld->psFont,1,psFld->pszText,strlen(psFld->pszText),DPL_LEFT);
							dcTextout(hdc,0+2,2,ColorLum(sys.Color3DShadow,-30),-1,psFld->psFont,1,psFld->pszText,strlen(psFld->pszText),DPL_LEFT);
						} 
						else {
						
						
							INT iWidth=psFld->psFont->iHeight*55/100;
							POINTD arsPoint[3];

							//
							// Title > CLOSE
							//
							if (psFld->bTitleClose) { // Chiuso

								arsPoint[0].x=0; arsPoint[0].y=0;
								arsPoint[1].x=0; arsPoint[1].y=iWidth; 
								arsPoint[2].x=iWidth; arsPoint[2].y=(DOUBLE) iWidth/2; 
/*
								dcBoxGradient(	hdc,
												&rcx,&rcGradient,
												AlphaColor(0xe0,sys.Color3DShadow),
												AlphaColor(0xff,sys.Color3DShadow));
								// dcLinePlus(hdc,0,0,rc.right,0,AlphaColor(255,sys.Color3DShadow),1);
								*/
								rectToD(&rcd,&rc);
								dcRectRoundEx(	hdc,&rcd,	
												AlphaColor(0xe0,sys.ColorSelectBack),
												AlphaColor(0xff,sys.ColorSelectBack),
												8,8,1);

								dcTextout(hdc,1+3+iWidth+8,5,ColorLum(sys.ColorSelectBack,-40),-1,psFld->psFont,1,psFld->pszText,strlen(psFld->pszText),DPL_LEFT);
								dcTextout(hdc,1+2+iWidth+8,4,sys.ColorSelectText,-1,psFld->psFont,1,psFld->pszText,strlen(psFld->pszText),DPL_LEFT);

								pointTranslateD(3,arsPoint,
												rc.left+6,
												rc.top+((sizRow.cy-iWidth)/2)-1);
								dcPolygon(	hdc,
											0,
											0,
											AlphaColor(200,sys.Color3DHighLight),
											3,arsPoint);

							} else {
							
								arsPoint[0].x=0; arsPoint[0].y=0;
								arsPoint[1].x=iWidth; arsPoint[1].y=0;
								arsPoint[2].x=(DOUBLE) iWidth/2; arsPoint[2].y=iWidth;

								dcBoxGradient(	hdc,
												&rcx,&rcGradient,
												AlphaColor(0xa0,sys.Color3DHighLight),
												AlphaColor(0,sys.Color3DHighLight));
								dcLinePlus(hdc,0,0,rc.right,0,AlphaColor(255,sys.Color3DHighLight),1,0);

								dcTextout(hdc,1+2+iWidth+5,3,sys.Color3DHighLight,-1,psFld->psFont,1,psFld->pszText,strlen(psFld->pszText),DPL_LEFT);
								dcTextout(hdc,0+2+iWidth+5,2,sys.ColorSelectBack,-1,psFld->psFont,1,psFld->pszText,strlen(psFld->pszText),DPL_LEFT);
								pointTranslateD(3,arsPoint,
												rc.left+3,
												rc.top+((sizRow.cy-iWidth)/2)-1);
								dcPolygon(	hdc,
											AlphaColor(128,sys.Color3DShadow),
											0,
											AlphaColor(200,sys.Color3DShadow),
											3,arsPoint);
							}




				
				

						}
						break;

					case 2:
						dcLinePlus(hdc,0,0,rc.right,0,AlphaColor(255,sys.Color3DShadow),1,0);
						dcLinePlus(hdc,0,1,rc.right,1,AlphaColor(255,sys.Color3DHighLight),1,0);
						dcTextout(hdc,1,3,sys.Color3DHighLight,-1,psFld->psFont,1,psFld->pszText,strlen(psFld->pszText),DPL_LEFT);
						dcTextout(hdc,0,2,sys.Color3DShadow,-1,psFld->psFont,1,psFld->pszText,strlen(psFld->pszText),DPL_LEFT);
						break;
				}


				EndPaint(hWnd,&ps);
			}
			break;

	}
	
	return(DefWindowProc(hWnd, message, wParam, lParam));
}


//
// _funcZone()
//
LRESULT CALLBACK _funcZone(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	EH_FORM_FIELD * psFld;
	psFld=(EH_FORM_FIELD *) GetWindowLong(hWnd,GWL_USERDATA);
	switch (message)
	{
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
			//	RECT rcArea;
				EH_DG *psDg;
				EH_OBJ * poj=&psFld->sObj;
				HDC hdc=BeginPaint(hWnd,&ps);

				INT lx,ly;
				EH_DISPEXT sDispExt;
				S_WINSCENA sWinScena;

				GetClientRect(hWnd,&poj->sClientRect); 
				poj->sClientRect.right--;
				poj->sClientRect.bottom--;
				sizeCalc(&poj->sClientSize,&poj->sClientRect);
				psDg=dgCreate(0,poj->sClientSize.cx,poj->sClientSize.cy);
				dcBoxp(psDg->hdc,&poj->sClientRect,sys.colEhWinBackground);


				if (*psFld->pszName!='@') 
					WinDirectDC(psDg->hdc,&sWinScena,__FUNCTION__); // @<nome> solo uso HDC (per MT)

				// Se non ho la funzione esterna
				if (poj->text&&!poj->funcExtern)
				{
					// Se esiste un'icone collegata
					if (*poj->text)
					{
						if (ico_info(&lx,&ly,poj->text)>=0)
						{
							dcIcone(hdc,
									poj->sClientRect.left+((poj->sClientSize.cx-lx)>>1),
									poj->sClientRect.top+((poj->sClientSize.cy-ly)>>1),
									poj->text);
						}
					}
				}

				//
				// Ho la funzione esterna
				//
				else if (poj->funcExtern)
				{

					sDispExt.lx=poj->sClientSize.cx;//poj->col1;
					sDispExt.ly=poj->sClientSize.cy;
					sDispExt.hdl=poj->idxFont;
					sDispExt.bEnable=poj->bEnable;
					sDispExt.px=0;
					sDispExt.py=0;
					sDispExt.col1=sys.ColorWindowText;
					sDispExt.col2=sys.ColorBackGround;
					sDispExt.ncam=-1;
					rectCopy(sDispExt.rClientArea,poj->sClientRect); // poj->sClientRect);
					rectCopy(sDispExt.rWindowArea,poj->sClientRect); // poj->sClientRect);
					sDispExt.hdc=psDg->hdc;
					sDispExt.psFont=fontPtr(poj->idxFont);
					(poj->funcExtern)(poj,WS_DISPLAY,poj->col1,(CHAR *) &sDispExt); // OK - Bufferizzato con DC

				}

			//FINE:

				if (*psFld->pszName!='@') 
				{	
					WinDirectDC(NULL,&sWinScena,NULL);
					dgCopyDC(psDg,&poj->sClientRect,poj->px+relwx,poj->py+relwy,hdc);	
				} 
				else {
					 
					dgCopyDC(psDg,&poj->sClientRect,poj->px,poj->py,hdc);
				}
				psDg=dgDestroy(psDg);

				EndPaint(hWnd,&ps);

			}
			break;

	}
	
	return(DefWindowProc(hWnd, message, wParam, lParam));
}

//
// _LSelectDrawItem()
//
static void _LSelectDrawItem(LPDRAWITEMSTRUCT psDis)
{
	CHAR *picone=NULL;
	CHAR *plus=NULL,icoplus=0;
	const INT toright=8+16;
    LONG ColTx,ColBg;
	INT a;
	BYTE *psz;
	HDC hdc;
	BOOL bEnable=TRUE;
	BOOL bSelect;
	POINT ptOfs;

	a=psDis->itemID; if (a<0) return;
	a=psDis->itemData; if (a<0) return;
	psz=(BYTE *) psDis->itemData; 
	hdc=psDis->hDC;

//	Flag=(psDis->itemState&ODS_GRAYED) ? FALSE : TRUE;
    bSelect=(psDis->itemState&ODS_SELECTED) ? TRUE : FALSE;
	bEnable=(psDis->itemState&ODS_DISABLED) ? FALSE : TRUE;
	
	if (bSelect)
	{
		ColTx=sys.ColorSelectText; ColBg=sys.ColorSelectBack; 
	}
	else 
	{
		if (bEnable)
		{
		  ColTx=sys.ColorWindowText; ColBg=sys.ColorWindowBack;
		}
		else
		{
		  ColTx=0; ColBg=-1;//sys.ColorBackGround;
		}
	}
	
	// Inizio a disegnare 
	if (ColBg!=-1) dcBoxp(hdc,&psDis->rcItem,ColBg); // bottom-1 ?
  
	if (psz) 
	{
		ptOfs.x=psDis->rcItem.left+2;
		ptOfs.y=psDis->rcItem.top+1;

		if (*psz!=3) // Scritta normale
		{
			dcTextout(hdc,ptOfs.x,ptOfs.y,ColTx,-1,NULL,1,psz,strlen(psz),DPL_LEFT);
			/*
			if (bEnable) 
				dcDisp(hdc,ptOfs.x,ptOfs.y,ColTx,-1,poj->idxFont,pts); //dispfm_h(ptOfs.x,ptOfs.y,ColTx,ColBg,poj->idxFont,pts);
				else
				dcDisp3D(hdc,ptOfs.x,ptOfs.y-1,poj->idxFont,pts);// disp3Dfm_h(ptOfs.x,ptOfs.y-1,poj->idxFont,pts);
				*/

		}
		else
		{						      // Scritta con icone
			INT xx,yy,zz;
			CHAR *pIcone=strTake(psz+10,psz+17);
			CHAR *pText=psz+23;
			xx=strGetInt(psz+2,3);//atoi(psz+2);  // Stampa icone
			yy=strGetInt(psz+6,3);//atoi(psz+6);  // Stampa icone
			zz=strGetInt(psz+19,3); // Stampa scritta

			strTrim(pIcone);
 			if (bEnable)
			{
				dcIcone(hdc,ptOfs.x+xx,ptOfs.y+yy,pIcone);
				//dcDisp(hdc,ptOfs.x+zz,ptOfs.y,ColTx,-1,poj->idxFont,pText);
				//TextOut(hdc,0,0,psz,strlen(psz));
				dcTextout(hdc,ptOfs.x+zz,ptOfs.y,ColTx,-1,NULL,1,pText,strlen(pText),DPL_LEFT);
			}
			else
			{
				dcIcone3d(hdc,ptOfs.x+xx,ptOfs.y+yy,ColBg,pIcone);
				//dcDisp3D(hdc,ptOfs.x+zz,ptOfs.y,poj->idxFont,pText);
				dcTextout(hdc,ptOfs.x+zz,ptOfs.y,ColTx,-1,NULL,1,pText,strlen(pText),DPL_LEFT);
			}
			ehFree(pIcone);
		}

		if (bSelect) DrawFocusRect(hdc,&psDis->rcItem);
	}
//FINE:
	/*
	if (!psDis->hDC) 
	{
		DeleteObject(hrgn);
		SelectClipRgn(hdc, NULL);
//		UsaDC(WS_CLOSE,hdc);
//		_SaveBit_RectChange(psDis->rcItem);
	}
	*/

}

//
// _this_Reset()
//
static void	_this_Reset(void *this) {

	EHZ_FORM *psForm=this;
	INT a;

	ShowWindow(psForm->wnd,SW_HIDE);
	for (a=0;a<psForm->dmiField.Num;a++) {
		_fldDestroy(&psForm->arField[a]);
	}
	psForm->dmiField.Num=0;
	ARDestroy(psForm->arBlurNotify); psForm->arBlurNotify=NULL;
	ARDestroy(psForm->arCharNotify); psForm->arCharNotify=NULL;
//	ShowWindow(psForm->wnd,SW_SHOW);

}

//
// -_this_Show() > Aggiunge un campo in coda al form
//
static BOOL _this_Show(void *this) {

	EHZ_FORM *psForm=this;
	_fieldsReposition(psForm);
	ShowWindow(psForm->wnd,SW_SHOW);
	return FALSE;

}

//
// -_this_Redraw() > Aggiunge un campo in coda al form
//
static BOOL _this_Redraw(void *this) {

	EHZ_FORM *psForm=this;
	_fieldsReposition(psForm);
	return FALSE;

}


//
// -_this_SetOptions() > Seleziona un array per un campo select (FLD_SELECT)
//
static BOOL _this_SetOptions(void *this,CHAR *pszName,EH_AR ar) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	INT a;
	BYTE *p;

	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) {ehAlert("ehzForm: '%s' not found",pszName);return TRUE;}

	if (psFld->arOption) ARDestroy(psFld->arOption);
	psFld->arOption=ARDup(ar);

	//
	// A) Cancello gli attuali elementi dalla lista
	//
	SendMessage(psFld->wndInput,CB_RESETCONTENT,0,0);

	//
	// B) Inserisco gli elementi
	//
	for (a=0;psFld->arOption[a];a++) {
		INT iRet;
		p=strstr(psFld->arOption[a],"\t"); 
		//
		// Non ho il codice definito ? ne assegno uno di ufficio uguale alla posizione dell'indice
		//
		if (!p) 
		{
			p=ehAlloc(strlen(psFld->arOption[a])+20);
			sprintf(p,"%s\t%d",psFld->arOption[a],a);
			strAssign(&psFld->arOption[a],p);
			p=strstr(psFld->arOption[a],"\t");
		}
		//if (!p) ehExit("form [%s] manca nel campo select il \\t [%s]",pszName,psFld->arOption[a]);
		*p=0;
		switch (psFld->iType) {

			case FLD_SELECT:
				iRet=SendMessage(psFld->wndInput,CB_ADDSTRING,0,(LPARAM) psFld->arOption[a]);
				break;
			case FLD_LIST:
				iRet=SendMessage(psFld->wndInput,LB_ADDSTRING,0,(LPARAM) psFld->arOption[a]);
				break;
		}
	}

	switch (psFld->iType) {

			case FLD_SELECT:
				SendMessage(psFld->wndInput,CB_SETCURSEL,0,0); // Seleziono il primo
				break;

			case FLD_LIST:
				SendMessage(psFld->wndInput,LB_SETCURSEL,0,0); // Seleziono il primo
				break;
	}

	return FALSE;
}

//
// _this_Focus()
//
static BOOL _this_Focus(void *this,CHAR *pszName) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	INT idx;

	if (!(psFld=_fldSearch(psForm,pszName,&idx,__LINE__))) return TRUE;
	_LFldSetFocus(psFld);
	psForm->idxFocus=idx;
	psForm->psFldFocus=psFld;
	return FALSE;
}


//
// _this_Set()
//
static BOOL	_this_Set(void *this,CHAR *pszName,void *pszValue) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;

	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return TRUE;
	_LSetFld(psForm,psFld,pszValue);
	return FALSE;
}

//
// _this_SetTitle()
//
static void _this_SetTitle(void *this,CHAR *pszName,void *pszValue) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;

	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return ;
	SetWindowText(psFld->wndTitle,pszValue);	
}

//
// _this_SetAfter()
//
static void _this_SetAfter(void *this,CHAR *pszName,void *pszValue) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;

	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return;
	if (psFld->iType==FLD_BUTTON)
	{
		SetWindowText(psFld->wndInput,pszValue);	
	}
	else
	{
		SetWindowText(psFld->wndAfter,pszValue);	
	}
	// _fieldsReposition(psForm);
}

//
// _this_SetParams()
//
static BOOL		_this_SetParams(void *this,CHAR *pszName,CHAR *pszFormat,...) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	CHAR * pszRet;

	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return false;
	strFromArgs(pszFormat,pszRet);
	_getParamParser(psFld,pszRet);
	ehFree(pszRet);

	return true;

}

//
// _this_SetNumber()
//
static BOOL	_this_SetNumber(void *this,CHAR *pszName,double dNumber) {

	CHAR szServ[200];

	if (dNumber<32000&&
		dNumber==(INT) dNumber)
		sprintf(szServ,"%d",(INT) dNumber);
		else
		sprintf(szServ,"%f",dNumber);
	return _this_Set(this,pszName,szServ);
}


//
// _this_Get()
//
static void *_this_Get(void *this,CHAR *pszName) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	INT idx;
	WCHAR *wcsValue=NULL;
	TCHAR *p;
	LRESULT lRes;

	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return NULL;

	//
	// Cerco il valore del campo
	//
	switch (psFld->enClass) {

		case FCLS_TEXT:
			
			switch (psFld->iType)
			{
				case FLD_NUMBER:
					wcsValue=_LNumberText(psFld,FALSE);
					break;

				default:
					wcsValue=_LTextFldGet(psFld);
					break;
			}
			break;

		case FCLS_SELECT:

			idx=SendMessage(psFld->wndInput,CB_GETCURSEL,0,0);	if (idx<0) break;
			p=psFld->arOption[idx]; p+=strlen(p)+1;
			wcsValue=strToWcs(p); //ehFree(p);
			break;


		case FCLS_LIST:

			idx=SendMessage(psFld->wndInput,LB_GETCURSEL,0,0);	if (idx<0) break;
			p=psFld->arOption[idx]; p+=strlen(p)+1;
			wcsValue=strToWcs(p); //ehFree(p);
			break;

		case FCLS_BUTTON:

			switch (psFld->iType)
			{
				case FLD_CHECKBOX:
					lRes=Button_GetState(psFld->wndInput);
					if (lRes&BST_CHECKED) p="1"; else p="0";
					wcsValue=strToWcs(p); 
					break;
			}
			break;

		case FCLS_DATE: // Read
			{
				SYSTEMTIME sST;
				EH_TIME eht;
				CHAR szServ[200];
				DWORD dw;
				dw=DateTime_GetSystemtime(psFld->wndInput,&sST);
				p="";
				if (dw==GDT_VALID) {

					timeStToEht(&eht,&sST);
					//
					// Formato data
					//
					switch (psFld->enFdm) {

						case FDM_DMY8:
							timeFormat(&eht,szServ,sizeof(szServ),"%d%m%Y",NULL);
							break;

						case FDM_YMD8:
							timeFormat(&eht,szServ,sizeof(szServ),"%Y%m%d",NULL);
							break;

						case FDM_DT:
							timeEhtToDt(szServ,&eht); 
							break;

					}

					
					
					p=szServ;
				}
				wcsValue=strToWcs(p); 
			}
			break;

		default:
			ehError();
	}

	//
	//
	//
	ehFreePtr(&psFld->pszValue);
	switch (psForm->enEncode)
	{
		case SE_ANSI:
			psFld->pszValue=wcsValue?wcsToStr(wcsValue):NULL;
			break;

		case SE_UTF8:
			psFld->pszValue=wcsValue?strEncodeW(wcsValue,SE_UTF8,NULL):NULL;
			break;

		default: ehError(); // Encoding non gestito
			break;

	}
	ehFreeNN(wcsValue);
	return psFld->pszValue;
}

static double _this_GetNumber(void *this,CHAR *pszName) {

	CHAR * psz= _this_Get(this,pszName);
	return atof(strEver(psz));

}

static HWND	_this_getWnd(void *this,CHAR *pszName) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return NULL;
	return psFld->wndInput;

}

static EH_FORM_FIELD  * 	_this_getFld(void *this,CHAR *pszName) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return NULL;
	return psFld;

}


//
// _this_BlurNotify() - Elenco dei campi di cui voglio la notifica della perdita del focus
//
static BOOL	_this_BlurNotify(void *this,CHAR *pszList) {

	EHZ_FORM *psForm=this;
	ARDestroy(psForm->arBlurNotify);
	psForm->arBlurNotify=ARCreate(pszList,",",NULL);
	return FALSE;
}

//
//_this_CharNotify() - Elenco dei campi di cui voglio la notifica su ogni pressione di tasto
//
static BOOL	_this_CharNotify(void *this,CHAR *pszList) {

	EHZ_FORM *psForm=this;
	ARDestroy(psForm->arCharNotify);
	psForm->arCharNotify=ARCreate(pszList,",",NULL);
	return FALSE;
}

//
// _this_Clean()
//
static void _this_Clean(void *this,CHAR * pszFieldFocus,CHAR * lstFldNotClean) { // Da implementare

	EHZ_FORM *psForm=this;
	INT a;

	for (a=0;a<psForm->dmiField.Num;a++) {
		psForm->Set(psForm,psForm->arField[a].pszName,"");
	}
	psForm->idxFocus=-1;
	psForm->psFldFocus=NULL;

}

//
// _this_Enable()
//
static void _this_Enable(void *this,CHAR *pszName,BOOL bEnable) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return ;
	EnableWindow(psFld->wndInput,bEnable);

}

static void _fieldVisible(EH_FORM_FIELD * psFld,BOOL bVisible) {

	if (psFld->wndInput) ShowWindow(psFld->wndInput,bVisible?SW_NORMAL:SW_HIDE);
	if (psFld->wndTitle) ShowWindow(psFld->wndTitle,bVisible?SW_NORMAL:SW_HIDE);
	if (psFld->wndAfter) ShowWindow(psFld->wndAfter,bVisible?SW_NORMAL:SW_HIDE);
}

//
// _this_Visible() T/F se visibile
//
static void _this_Visible(void *this,CHAR *pszName,BOOL bVisible) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	
	if (!pszName) {
		ShowWindow(psForm->wnd,bVisible?SW_NORMAL:SW_HIDE);
		return;
	}
	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return;
	_fieldVisible(psFld,bVisible);
	/*
	ShowWindow(psFld->wndInput,bVisible?SW_NORMAL:SW_HIDE);
	ShowWindow(psFld->wndTitle,bVisible?SW_NORMAL:SW_HIDE);
	ShowWindow(psFld->wndAfter,bVisible?SW_NORMAL:SW_HIDE);
	*/

}


//
// _this_Exclude() T/F se visibile
//
static void _this_Exclude(void * this,CHAR * pszNames,BOOL bExclude) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	INT a;
	
	EH_AR ar=ARFSplit(pszNames,",");
	for (a=0;ar[a];a++) {

		if ((psFld=_fldSearch(psForm,ar[a],NULL,__LINE__))) {
			psFld->bExclude=bExclude;
			_fieldVisible(psFld,!bExclude);
		}
	}
	ehFree(ar);
	_fieldsReposition(psForm);
}

//
// _this_SetFunction()
//
static void	_this_SetFunction(void *this,CHAR *pszName,void * (*funcExtern)(struct OBJ *psObj,EN_MESSAGE enMess,LONG lParam,void *pVoid)) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return;
	psFld->funcExtern=funcExtern;

	// Oggetto precedente
	if (psFld->funcExtern) 
	{
		psFld->funcExtern(&psFld->sObj,WS_CLOSE,0,NULL);
		psFld->funcExtern(&psFld->sObj,WS_DESTROY,0,NULL);
	}

	_(psFld->sObj);
	psFld->funcExtern(&psFld->sObj,WS_CREATE,0,NULL);
	psFld->funcExtern(&psFld->sObj,WS_OPEN,0,NULL);
	_this_SetOptions(this,pszName,psFld->sObj.ptr); // Setto con l'array ritornato
}

//
// _this_SendMessage()
//
static void	_this_SendMessage(void *this,CHAR *pszName,INT cmd, LONG info,void *str) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return;

	if (!psFld->funcExtern) ehError();
	psFld->funcExtern(&psFld->sObj,cmd,info,str);
}


//
// _this_Refresh()
//
static void	_this_Refresh(void *this,CHAR *pszName) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return;

	if (!psFld->funcExtern) ehError();

	switch (psFld->enClass)
	{
		case FCLS_SELECT:
//		case FCLS_SELECT_MULTI:
		case FCLS_LIST:
			// Oggetto precedente
			if (psFld->funcExtern) psFld->funcExtern(&psFld->sObj,WS_CLOSE,0,NULL);
			psFld->funcExtern(&psFld->sObj,WS_OPEN,0,NULL);
			_this_SetOptions(this,pszName,psFld->sObj.ptr); // Setto con l'array ritornato
			break;

		default:
			break;
	}
}

//
// _this_setNotifyFunc()
//
static void	 _this_setNotifyFunc(void *this,void * (*funcExtern)(void * this,EH_SRVPARAMS)) {

	EHZ_FORM * psForm=this;
	psForm->funcNotify=funcExtern;

}

//
// _this_ensureVisible()
//
static void		_this_ensureVisible(void *this,CHAR * pszName) {

	EHZ_FORM * psForm=this;
	EH_FORM_FIELD * psFld, * psFl;
	RECT rec;
	INT a;
	BOOL bFirst=false;
	SIZE sizDiv;

	if (!(psFld=_fldSearch(psForm,pszName,NULL,__LINE__))) return;

	// a. Cerco il rettangolo dell'oggetto
	//	  Se un titolo area di tutti gli elementi racchiusi nel titolo
	if (psFld->iType==FLD_TITLE) {

		bFirst=true;
		for (a=0;a<psForm->dmiField.Num;a++) {

			psFl=&psForm->arField[a];
			if (psFl->psParent==psFld) {

				if (bFirst) 
					rectCopy(rec,psFl->recDiv);		
					else {
				
						if (psFl->recDiv.right>rec.right) rec.right=psFl->recDiv.right;
						if (psFl->recDiv.bottom>rec.bottom) rec.bottom=psFl->recDiv.bottom;

					}
				bFirst=false;
					
			}
		}

	} else {
	
		rectCopy(rec,psFld->recDiv);
	}

	//
	// b. Cerco l'offset di far vedere l'oggetto
	//
	sizeCalc(&sizDiv,&rec);

	//a=psForm->ofsVert;
	psForm->ofsVert=ehBarRangeAdjust(psForm->wnd, 
									 SB_VERT,
									 rec.top,
									 psForm->sizForm.cy, // Altezza del form
									 psForm->sizClient.cy); // Altezza della finestra
	//iOffset=;
	printf("qui");


}
//
// SQL Functions
//

#ifdef SQL_RS
//
// SqlGetRs() _this_SqlGetRs - 
// Scrive i valori di una query nel form 
//
static BOOL	_this_SqlGetRs(void *this,SQL_RS rsSet) {

	EHZ_FORM *psForm=this;
	EH_FORM_FIELD * psFld;
	INT a;
	BYTE *pFieldValue;
	INT idxField;
	BOOL bError=TRUE;

	bError=FALSE;
	for (a=0;a<psForm->dmiField.Num;a++) {

		psFld=&psForm->arField[a];
		if (psFld->iType==FLD_BUTTON|| psFld->bQueryNot) continue;
		idxField=sql_find(rsSet,psFld->pszName); // ,TRUE
		if (idxField<0)
		{
			ehPrintd(__FUNCTION__":NomeField ? \n[%s] in Query[%s]" CRLF,psFld->pszName,rsSet->pszQuery);
			continue;
		}
		pFieldValue=sql_ptr(rsSet,psFld->pszName);
		_LSetFld(psForm,psFld,pFieldValue);
	}

	return FALSE;
}


//
// _this_SqlSelect - Select (ritorna TRUE se non lo trova)
//
static BOOL	_this_SqlSelect(void *this,CHAR *pQuery,...) {

	SQL_RS rsSet;
	CHAR *pszQuery=ehAlloc(64000);
	
	//
	// Formo la query
	//
	va_list Ah;
	va_start(Ah,pQuery);
	vsprintf(pszQuery,pQuery,Ah); // Messaggio finale
	va_end(Ah);

	rsSet=sql_row("%s",pszQuery);
	ehFree(pszQuery);

	if (!rsSet) return TRUE;

	_this_SqlGetRs(this,rsSet);
	sql_free(rsSet);
	return FALSE;
	
}



//
// _this_SqlUpdate - Update 
//
static BOOL	_this_SqlUpdate(void *this,CHAR *pszInfo,CHAR *pQuery,...) {

	EHZ_FORM *psForm=this;
	CHAR *pszQuery=ehAlloc(64000),*pszFields;
	EH_AR arFld,ar;
	INT a;
	EH_FORM_FIELD * psFld;
	INT iNum;
	BOOL bNumber;
	EH_AR arField;
	BOOL bError=false;

	//
	// Formo la query
	//
	va_list Ah;
	va_start(Ah,pQuery);
	vsprintf(pszQuery,pQuery,Ah); // Messaggio finale
	va_end(Ah);
	
//	return false;
	arField=ARNew();

	//
	// Campi selezionati
	//
	if (pszInfo) 
		arFld=ARCreate(pszInfo,",",NULL);
	//
	// Tutti i campi
	//
	else
	{
		arFld=ARNew();
		for (a=0;a<psForm->dmiField.Num;a++) {
			ARAdd(&arFld,psForm->arField[a].pszName);
		}
	}

	for (a=0;arFld[a];a++) {
		
		BYTE *pszFormField,*pszSqlField;
		CHAR * pszFieldValue;
		ar=ARCreate(arFld[a],"=",&iNum);
		if (iNum==1) pszFormField=pszSqlField=ar[0]; else {pszFormField=ar[0]; pszSqlField=ar[1];}
		psFld=_fldSearch(psForm,pszFormField,NULL,__LINE__); 
		if (!psFld) ehExit("sqlUpdate(): campo %s inesistente",pszFormField);
		bNumber=FALSE; if (psFld->iType==FLD_NUMBER||psFld->iType==FLD_CHECKBOX) bNumber=TRUE;
		if (strstr(pszSqlField,"(number)")) {strReplace(pszSqlField,"(number)",""); bNumber=TRUE;}
		pszFieldValue=psForm->Get(psForm,pszFormField);

		if (psFld->iMaxChar&&(INT) strlen(pszFieldValue)>psFld->iMaxChar) {
			ehAlert("Update non eseguito\nIl valore del campo %s è maggiore di %d caratteri.\nProbabile problema dopo codifica utf-8.",psFld->pszName,psFld->iMaxChar);
			bError=true;
			break;
		}

		if (bNumber) 
			ARAddarg(&arField,"%s=%s",pszSqlField,pszFieldValue);
			else
			{
				CHAR *pSql;
				pSql=strEncode(pszFieldValue,SE_SQL,NULL);
				ARAddarg(&arField,"%s='%s'",pszSqlField,pSql);
				ehFree(pSql);
			}
		ARDestroy(ar);
	}
	ARDestroy(arFld);
	pszFields=ARToString(arField,",","","");
	strReplace(pszQuery,"[FIELDS]",pszFields);
	ehFree(pszFields);

#ifdef _DEBUG
	ehPrintd("%s",pszQuery);
#endif
	if (!bError) 
		bError=sql_querybig(100000,"%s",pszQuery);

	// Rilascio le risorse
	ehFree(pszQuery);
	return bError;
}


//
// _this_SqlInsert - Insert
//
static BOOL	_this_SqlInsert(void *this,CHAR *pszInfo,CHAR *pQuery,...) {

	EHZ_FORM *psForm=this;
	BOOL bError;
	CHAR *pszQuery=ehAlloc(64000),*pszFields;
	EH_AR arFld,ar;
	INT a;
	EH_FORM_FIELD * psFld;
	INT iNum;
	BOOL bNumber;
	EH_AR arFields,arValues;

	//
	// Formo la query
	//
	va_list Ah;
	va_start(Ah,pQuery);
	vsprintf(pszQuery,pQuery,Ah); // Messaggio finale
	va_end(Ah);


	arFields=ARNew();
	arValues=ARNew();
	
	//
	// Campi selezionati
	//
	if (pszInfo) 
		arFld=ARCreate(pszInfo,",",NULL);
	//
	// Tutti i campi
	//
	else
	{
		arFld=ARNew();
		for (a=0;a<psForm->dmiField.Num;a++) {
			ARAdd(&arFld,psForm->arField[a].pszName);
		}
	}
	for (a=0;arFld[a];a++) {
		
		BYTE *pszFormField,*pszSqlField;
		ar=ARCreate(arFld[a],"=",&iNum);
		if (iNum==1) pszFormField=pszSqlField=ar[0]; else {pszFormField=ar[0]; pszSqlField=ar[1];}
		psFld=_fldSearch(psForm,pszFormField,NULL,__LINE__); 
		if (!psFld) ehExit("sqlUpdate(): campo %s inesistente",pszFormField);
		bNumber=FALSE; if (psFld->iType==FLD_NUMBER||psFld->iType==FLD_CHECKBOX) bNumber=TRUE;
		if (strstr(pszSqlField,"(number)")) {strReplace(pszSqlField,"(number)",""); bNumber=TRUE;}
		
		ARAdd(&arFields,pszSqlField);
		if (bNumber) 
			ARAdd(&arValues,psForm->Get(psForm,pszFormField));
			else
			{
				CHAR *pSql;
				pSql=strEncode(psForm->Get(psForm,pszFormField),SE_SQL,NULL);
				ARAddarg(&arValues,"'%s'",pSql);
				ehFree(pSql);
			}
		ARDestroy(ar);
	}
	ARDestroy(arFld);

	pszFields=ARToString(arFields,",","","");
	strReplace(pszQuery,"[FIELDS]",pszFields);
	ehFree(pszFields);

	pszFields=ARToString(arValues,",","","");
	strReplace(pszQuery,"[VALUES]",pszFields);
	ehFree(pszFields);

	bError=sql_query("%s",pszQuery);

	// Rilascio le risorse
	ehFree(pszQuery);
	ARDestroy(arFields);
	ARDestroy(arValues);
	return bError;
}

#endif


static void		_extNotify(EHZ_FORM * psForm,CHAR * pszEvent,EH_FORM_FIELD * psFld) {

	if (!psForm->funcNotify) return;
	psForm->funcNotify(psForm,WS_EVENT,(INT) psFld,pszEvent);

}


//
// RIPOSIZIONAMENTO
//

static void _postRowAdjust(EHZ_FORM * psForm,INT iRow,RECT * precRow) {

//	cyRow=1;
	INT b;
	INT iFirst=-1,iLast=0;
//	SIZE sizArea;
	EH_FORM_FIELD * psFld;
	memset(precRow,0,sizeof(RECT));

	//
	// Loop sulla riga per determinare le dimensioni
	//
	for (b=0;b<psForm->dmiField.Num;b++) {

		psFld=&psForm->arField[b];
		if (psFld->bExclude) continue;
		if (psFld->iRow==iRow) {

			if (iFirst<0) {

				iFirst=b;
				memcpy(precRow,&psFld->recDiv,sizeof(RECT));

			}
			else {

				if (psFld->recDiv.left<precRow->left) precRow->left=psFld->recDiv.left;
				if (psFld->recDiv.top<precRow->top) precRow->top=psFld->recDiv.top;
				if (psFld->recDiv.right>precRow->right) precRow->right=psFld->recDiv.right;
				if (psFld->recDiv.bottom>precRow->bottom) precRow->bottom=psFld->recDiv.bottom;

			}
			iLast=b;
			 
		}
	}
	if (precRow->bottom>psForm->recForm.bottom)  psForm->recForm.bottom=precRow->bottom;
	if (precRow->right>psForm->recForm.right)  psForm->recForm.right=precRow->right;

	for (b=iFirst;b<=iLast;b++) {

		psFld=&psForm->arField[b];
		if (psFld->wndTitle) {
//			SIZE sizTitle;
			psFld->recTitle.bottom=precRow->bottom;
//			sizeCalc(&sizTitle,&psFld->recTitle);
//			DeferWindowPos(hdwp, psFld->wndTitle, HWND_TOP, psFld->recTitle.left, psFld->recTitle.top, sizTitle.cx,sizTitle.cy,0);
		}


	}
}

static DWORD _getSizeText(HWND hwnd,CHAR * pszText,SIZE * pSize) {

	HDC hdc;
	DWORD dwText;

	hdc=GetDC(hwnd);
	dwText=GetTextExtentPoint32(hdc, pszText, strlen(pszText), pSize);
	ReleaseDC(hwnd,hdc);					
	return dwText;
}


//	
// _fieldsReposition() Ricalcola le posizioni dei campi del form
//
static void _fieldsReposition(EHZ_FORM * psForm) {

	EH_FORM_FIELD * psFld=NULL,*psFldPrev;
	SIZE sizTitle,sizField;
	INT a,y,cyRow;
	INT iCellPadDouble,yWinPad;
	POINT ptCursor; // ptInput,ptAfter;
	HWND hdwp;
	INT iRow,iAltFont,cxMax;
	SIZE sizClient;
	RECT recRow;

	GetClientRect(psForm->wnd, &psForm->rcClient);
	sizeCalc(&sizClient,&psForm->rcClient);
	memcpy(&psForm->sizClient,&sizClient,sizeof(SIZE));
	_extNotify(psForm,"beforeReposition",NULL);

	if (!psForm->iRowHeight) psForm->iRowHeight=psForm->psFontInput->iHeight+(psForm->iCellPadding<<1);
	_(psForm->recForm);
	psForm->sizForm.cx=psForm->sizClient.cx;

	//
	// Decido le dimensioni
	//
	if (strEmpty(psForm->pszTitleWidth)) psForm->pszTitleWidth="25%";
	if (strstr(psForm->pszTitleWidth,"%")) 	{
		psForm->iTitleWidth=psForm->sizClient.cx*atoi(strKeep(psForm->pszTitleWidth,"0123456789"))/100;
	}
	else {
		psForm->iTitleWidth=atoi(strKeep(psForm->pszTitleWidth,"0123456789"));
	}

	if (psForm->iTitleWidth<psForm->iTitleMin) psForm->iTitleWidth=psForm->iTitleMin; // Controllo minimo

	sizTitle.cx=psForm->iTitleWidth;

	//
	// Reset
	//
	for (a=0;a<psForm->dmiField.Num;a++) {
		psFld=&psForm->arField[a];
		if (psFld->bExclude) continue;
		psFld->iRow=0;
		psFld->bNotAppend=false;
	}

	//
	// Calcolo le posizioni degli oggetti
	//
	yWinPad=(GetSystemMetrics(SM_CYFIXEDFRAME)<<1);//+(GetSystemMetrics(SM_CYEDGE)<<1);
	iCellPadDouble=psForm->iCellPadding<<1; 
	psFldPrev=NULL;
	iRow=0;
	for (a=0;a<psForm->dmiField.Num;a++) {

		psFld=&psForm->arField[a];	_(psForm->rcClient);
		if (psFld->psParent) {
			EH_FORM_FIELD * psFldParent=psFld->psParent;
			if (psFldParent->bTitleClose) {
				_fieldVisible(psFld,false);
				continue;
			}
		
		}
		if (psFld->bExclude) 
		{
			_fieldVisible(psFld,false);
			continue;
		}

		//
		// > Titolo
		//
		cyRow=psForm->iRowHeight;
		sizTitle.cy=cyRow-iCellPadDouble;
		sizTitle.cx=((psFld->iTitleWidth>-1)?psFld->iTitleWidth:psForm->iTitleWidth)-iCellPadDouble;

		//
		// Posizionamento
		//
		if (psFld->bAppend&&!psFld->bNotAppend&&psFldPrev) // bNotAppend attivato in caso di campo "overclient"
		{
			if (!a) ehError(); // Il primo non può essere accodato
			// psFldPrev=&psForm->arField[a-1];
			if (psFldPrev->bInputSizeMax)
				ptCursor.x=psForm->sizForm.cx;
				else
				ptCursor.x=psFldPrev->recDiv.right+psForm->iCellPadding;
			ptCursor.y=psFldPrev->recDiv.top;
			psFld->iRow=iRow;
			if (strEmpty(psFld->pszText)) 
				{sizTitle.cy=sizTitle.cx=0;} // Azzero il titolo se non ce l'ho in append
			else
			{
				ptCursor.x+=iCellPadDouble; // Mi distanzio
			}

		}
		else
		{
			iRow++;
			psFld->iRow=iRow;
			ptCursor.x=0;//psForm->iCellPadding; 
			if (psFld->iType!=FLD_TITLE) ptCursor.x+=psForm->recFormPadding.left;
//			if (psFld->bAppend&&psFld->bNotAppend&&!psFld->wndTitle) {
			if (psFld->bAppend&&psFld->bNotAppend&&strEmpty(psFld->pszText)) {
				ptCursor.x+=sizTitle.cx;
			}

			//
			// Controllo Riga
			//
			_(recRow);
			if (iRow>1) _postRowAdjust(psForm,iRow-1,&recRow);
			ptCursor.y=recRow.bottom+psForm->iCellPadding; // Cacolo riga sotto

		}

		psFld->recDiv.left=ptCursor.x;
		psFld->recDiv.top=ptCursor.y;
		ptCursor.x+=psFld->rcMargin.left;
		ptCursor.y+=psFld->rcMargin.top;

		//
		// > Calcolo le dimensioni del campo > CX
		//
		iAltFont=psForm->psFontInput->iHeight;
		if (psFld->psFont) iAltFont=psFld->psFont->iHeight;

		if (!psFld->iWidth) // Fino al font
		{
			sizField.cx=30;// indicativo, viene calcolato alal fine > psForm->sizClient.cx-ptCursor.x-iCellPadDouble;//psForm->iTitleWidth-iCellPad;	
			psFld->bInputSizeMax=true;
		}
		else {
			psFld->bInputSizeMax=false;
			if (psFld->bWidthPerc)
				sizField.cx=psForm->sizClient.cx*psFld->iWidth/100;
				else
				sizField.cx=psFld->iWidth;
		}

		switch(psFld->iType) {

			case FLD_CHECKBOX:
				// Larghezza automatica
				if (!psFld->iWidth) {
					SIZE sizText;
					if (!psFld->pszButton) psFld->pszButton="";
					//_getSizeText(psFld->wndInput,psFld->pszButton,&sizText);
					fontGetSize(psFld->pszButton,strlen(psFld->pszButton),psFld->psFontApply,&sizText);
					sizField.cx=sizText.cx+10+GetSystemMetrics(SM_CXMENUCHECK);
					psFld->bInputSizeMax=false;
				}
//				sizField.cx=30;
				break;

			default:
				break;

		}
		//
		// > Calcolo le dimensioni del campo > CY 
		//

		switch(psFld->iType) {
		
			//
			// Campo di testo
			//
			case FLD_TEXT:
			case FLD_NUMBER:
			case FLD_PASSWORD:
					
					if (!psFld->iHeight)
						sizField.cy=iAltFont+yWinPad;
						else
						sizField.cy=psFld->iHeight+yWinPad;
					break;

			case FLD_DATE:
				{
					if (!psFld->iHeight)
						sizField.cy=iAltFont+yWinPad;
						else
						sizField.cy=psFld->iHeight+yWinPad;
				}
				break;


			case FLD_BUTTON:
			case FLD_CHECKBOX:
					if (!psFld->iHeight)
						sizField.cy=iAltFont+(GetSystemMetrics(SM_CYEDGE)<<1)+2;
						else
						sizField.cy=psFld->iHeight+(GetSystemMetrics(SM_CYEDGE)<<1)+2;
					sizTitle.cy=sizField.cy;
					break;

			//
			// Area di testo
			//
			case FLD_LIST:
				iAltFont+=4;
				if (!psFld->iTextRows) psFld->iTextRows=2; // Almeno due linee
				if (!psFld->iHeight)
						sizField.cy=(iAltFont*psFld->iTextRows)+yWinPad;
						else
						sizField.cy=psFld->iHeight+yWinPad;
				sizField.cy+=(GetSystemMetrics(SM_CYEDGE)<<1)+2;
				break;

			case FLD_TITLE:
				sizTitle.cx=30;//psForm->sizClient.cx;
				psFld->bTitleSizeMax=true;
				if (!psFld->iHeight)
					sizTitle.cy=sizField.cy=iAltFont+yWinPad;
					else
					sizTitle.cy=sizField.cy=psFld->iHeight+yWinPad;
				break;

			case FLD_ZCMP:
				if (!psFld->iHeight)
					sizField.cy=iAltFont+yWinPad;
					else
					sizField.cy=psFld->iHeight+yWinPad;
// 				psFld->wndInput=psFld->sObj.hWnd;
				break;


			case FLD_TEXTAREA:
			default:  
					if (!psFld->iTextRows) psFld->iTextRows=1;
					if (!psFld->iHeight)
						sizField.cy=(iAltFont*psFld->iTextRows)+yWinPad;
						else
						sizField.cy=psFld->iHeight+yWinPad;
					break;
		}

		// Controlli minimo massimi
		if (sizField.cx<psFld->iMinWidth) sizField.cx=psFld->iMinWidth;
		if (sizField.cx>psFld->iMaxWidth&&psFld->iMaxWidth) sizField.cx=psFld->iMaxWidth;
		// sizField.cx-=iCellPadDouble;

		//
		// Cerco la dimensione massima della riga
		//
		y=(sizField.cy+iCellPadDouble); if (y>cyRow) cyRow=y;

		//
		// > Posiziono il titolo
		//
		if (psFld->iType==FLD_TITLE) { 

			// Deve essere lungo tutto il form
			
			rectFill(	&psFld->recTitle,
						ptCursor.x,ptCursor.y,
						ptCursor.x+sizTitle.cx,//+GetSystemMetrics(SM_CXBORDER),
						ptCursor.y+sizTitle.cy-1);//+GetSystemMetrics(SM_CYBORDER));
			rectCopy(psFld->recDiv,psFld->recTitle);

		}/*
		else if (psFld->iType==FLD_ZCMP) {

			printf("qui");
			// DeferWindowPos(hdwp, psFld->sObj.hWnd, HWND_TOP, ptTitle.x, ptTitle.y, sizTitle.cx,sizTitle.cy,0); 


		} 
		*/
		else {

			if (psFld->wndTitle) {

				rectFill(&psFld->recTitle,ptCursor.x,ptCursor.y,ptCursor.x+sizTitle.cx-1,ptCursor.y+sizTitle.cy-1);
				psFld->recDiv.right=ptCursor.x+sizTitle.cx-1;
				psFld->recDiv.bottom=ptCursor.y+sizTitle.cy-1;

				if (sizTitle.cx) {
					
					// DeferWindowPos(hdwp, psFld->wndTitle, HWND_TOP, ptTitle.x, ptTitle.y, sizTitle.cx,sizTitle.cy,0); 

				} else
				{
					//DeferWindowPos(hdwp, psFld->wndTitle, HWND_TOP, ptCursor.x, ptCursor.y, sizTitle.cx, sizTitle.cy,SWP_HIDEWINDOW);
				}
				ptCursor.x=psFld->recDiv.right+1;
				// ptCursor.y=psFld->recDiv.top;
			}

		}

		//
		// > Posiziono il Campo di input e calcolo occupazione
		//
		if (psFld->wndInput) 
		{
			//DeferWindowPos(hdwp, psFld->wndInput, HWND_TOP, ptCursor.x, ptCursor.y,sizField.cx,sizField.cy,0); 
			rectFill(&psFld->recInput,ptCursor.x, ptCursor.y, ptCursor.x+sizField.cx, ptCursor.y+sizField.cy);
			//rectFill(&psFld->rcClient,ptObject.x,ptObject.y,ptInput.x+sizField.cx-1+GetSystemMetrics(SM_CXBORDER),ptInput.y+sizField.cy-1+GetSystemMetrics(SM_CYBORDER));
			psFld->recDiv.right=ptCursor.x+sizField.cx-1+GetSystemMetrics(SM_CXBORDER);
			psFld->recDiv.bottom=ptCursor.y+sizField.cy-1+GetSystemMetrics(SM_CYBORDER);
			ptCursor.x=psFld->recDiv.right+1;

		}

		//
		// AFTER
		//
		if (psFld->wndAfter) 
		{
			SIZE sizAfter;
			DWORD dwText;

			if (!strEmpty(psFld->pszButton)) {

				//ptAfter.y=ptInput.y;
				ptCursor.x+=psForm->iCellPadding;// .x=psFld->recDiv.right+psForm->iCellPadding;

				if (psFld->iAfterWidth)
				{
					sizAfter.cx=psFld->iAfterWidth;
					sizAfter.cy=sizField.cy;
				}
				else
				{
					HDC hdc;
					hdc=GetDC(psFld->wndAfter);
					dwText=GetTextExtentPoint32(hdc, psFld->pszButton, strlen(psFld->pszButton), &sizAfter);
					ReleaseDC(psFld->wndAfter,hdc);
				}

				//sizAfter.cx=40; sizAfter.cy=+sizField.cy;

				//DeferWindowPos(hdwp, psFld->wndAfter, HWND_TOP, ptCursor.x, ptCursor.y,sizAfter.cx,sizAfter.cy,0); 
				rectFill(&psFld->recAfter,ptCursor.x, ptCursor.y, ptCursor.x+sizAfter.cx-1, ptCursor.y+sizAfter.cy-1);
				psFld->recDiv.right=ptCursor.x+sizAfter.cx-1;
//				ptCursor.x=psFld->recDiv.right+1;

			} 
			else {  
				// DeferWindowPos(hdwp, psFld->wndAfter, HWND_TOP, 0,0,0,0,SWP_HIDEWINDOW); 
			}
			//rectFill(&psFld->rcClient,ptObject.x,ptObject.y,ptInput.x+sizField.cx-1+psForm->iCellPadding,ptInput.y+sizField.cy-1+psForm->iCellPadding);
		
		}

		// Aggiungo il padding
		psFld->recDiv.right+=psForm->iCellPadding;
		psFld->recDiv.bottom+=psForm->iCellPadding;

		if ((psFld->recDiv.right>=psForm->sizForm.cx)&&
			psFld->bAppend&&
			!psFld->bNotAppend) 
		{
			psFld->bNotAppend=true;
			psFld->iRow=0;
			a--;
		} else {
			_fieldVisible(psFld,true);
			psFldPrev=psFld;
		}

	}
	if (iRow>1) _postRowAdjust(psForm,iRow,&recRow);
/*
	if (psFld) 
		psForm->sizForm.cy=psFld->recDiv.bottom+psForm->recFormPadding.bottom; 
		else 
		psForm->sizForm.cy=0;
*/
	sizeCalc(&psForm->sizForm,&psForm->recForm);
	psForm->sizForm.cy+=psForm->recFormPadding.bottom;
	cxMax=(psForm->sizClient.cx>psForm->sizForm.cx)?psForm->sizClient.cx:psForm->sizForm.cx;

	//
	// Calcolo gli oggetti che devono arrivare alla fine -------------------
	//
	for (a=0;a<psForm->dmiField.Num;a++) {

		psFld=&psForm->arField[a];
		if (psFld->bInputSizeMax) {
			psFld->recInput.right=cxMax-psForm->iCellPadding-psForm->recFormPadding.right;
			if (psFld->iMaxWidth) 
			{
				INT cx=psFld->recInput.right-psFld->recInput.left+1;
				if (cx>psFld->iMaxWidth) psFld->recInput.right=psFld->recInput.left+psFld->iMaxWidth-1;

			}

		}
		if (psFld->bTitleSizeMax) {
			psFld->recTitle.right=cxMax-psForm->iCellPadding;
		}
	}

	//
	// Posiziono gli oggetti
	//
	hdwp = BeginDeferWindowPos(psForm->dmiField.Num*2); 
	for (a=0;a<psForm->dmiField.Num;a++) {

		psFld=&psForm->arField[a];

		//
		// Qui posso posizionare gli oggetti
		//
		if (psFld->wndTitle) { 
			DeferWindowPos(hdwp, psFld->wndTitle, HWND_TOP, 
							psFld->recTitle.left, psFld->recTitle.top-psForm->ofsVert, 
							psFld->recTitle.right-psFld->recTitle.left+1,
							psFld->recTitle.bottom-psFld->recTitle.top+1,
							0); 
			if (psFld->iType==FLD_TITLE) {
				InvalidateRect(psFld->wndTitle,NULL,false);
			}
		}

		if (psFld->wndInput) { 

			DeferWindowPos(	hdwp, 
							psFld->wndInput, HWND_TOP, 
							psFld->recInput.left, psFld->recInput.top-psForm->ofsVert, 
							psFld->recInput.right-psFld->recInput.left+1,
							psFld->recInput.bottom-psFld->recInput.top+1,
							0); 
		}

		if (psFld->wndAfter) { 

			DeferWindowPos(	hdwp, 
							psFld->wndAfter, HWND_TOP, 
							psFld->recAfter.left, psFld->recAfter.top-psForm->ofsVert, 
							psFld->recAfter.right-psFld->recAfter.left+1,
							psFld->recAfter.bottom-psFld->recAfter.top+1,
							0); 
		}
	}
	EndDeferWindowPos(hdwp); 



	//
	// Determino se ho la barra laterale
	//
//	printf("> %d (%d / %d)" CRLF,psForm->ofsVert,(psForm->ofsVert+psForm->sizClient.cy),psForm->sizForm.cy);
	a=psForm->ofsVert;
	psForm->ofsVert=ehBarRangeAdjust(psForm->wnd, 
									 SB_VERT,
									 psForm->ofsVert,
									 psForm->sizForm.cy, // Altezza del form
									 psForm->sizClient.cy); // Altezza della finestra
	if (a!=psForm->ofsVert) _fieldsReposition(psForm);
//	printf("Dopo: %d" CRLF,psForm->ofsVert);
	psForm->ofsHorz=ehBarRangeAdjust(psForm->wnd,
									 SB_HORZ,
									 psForm->ofsHorz,
									 psForm->sizForm.cx, // Altezza del form
									 psForm->sizClient.cx); // Altezza della finestra

	_extNotify(psForm,"afterReposition",NULL);

}

//
// _fldSearch()
//
EH_FORM_FIELD * _fldSearch(EHZ_FORM *psForm,CHAR *pszName,INT *pIdx,INT iLine) {
	
	INT a;
	EH_FORM_FIELD * psFld=NULL;

	for (a=0;a<psForm->dmiField.Num;a++) {
		psFld=&psForm->arField[a];
		if (!strCmp(psFld->pszName,pszName)) {
			if (pIdx) *pIdx=a;
			return psFld;
		}
	}

	switch (psForm->iErrorLevel) {
		
			case 0: break;
			case 1: win_infoarg("ehForm:%d - Campo %s inesistente",iLine,pszName); break;
			case 2: ehExit("ehForm:%d - Campo %s inesistente",iLine,pszName); break;
	}

	return NULL;
}

//
// _isDataFld() - E un campo dati
//
static BOOL _isDataFld(EH_FORM_FIELD * psFld) {

	if (psFld->iType==FLD_BUTTON||
		psFld->iType==FLD_TITLE||
		psFld->iType==FLD_ZCMP||
		psFld->bQueryNot) return false;
	return true;
}

//
// Tabulazione (iDir +1=Avanti, -1=Indietro)
//
static void	_LFocusTab(EHZ_FORM *psForm,INT iDir) {

	EH_FORM_FIELD * psFld;
	while (TRUE) {
		psForm->idxFocus+=iDir;
		if (psForm->idxFocus<0) psForm->idxFocus=psForm->dmiField.Num-1; 
		if (psForm->idxFocus>=psForm->dmiField.Num) psForm->idxFocus=0; 
		psFld=&psForm->arField[psForm->idxFocus];
		psForm->psFldFocus=psFld;
		if (psFld->bReadOnly) continue;
		if (!IsWindowEnabled(psFld->wndInput)) continue;
		break;
	}
	winSetFocus(psFld->wndInput);
	_LFldSetFocus(psFld);
}

//
// _LSetFocusIdx)
//
static void	_LSetFocusIdx(HWND hWnd) {

	EH_IPT_INFO *psIptInfo=_LGetForm(hWnd);
	WCHAR *pwcs;

	if (!psIptInfo->psFld) return;
	psIptInfo->psForm->idxFocus=psIptInfo->idxField;
	psIptInfo->psForm->psFldFocus=psIptInfo->psFld;
	switch (psIptInfo->psFld->iType)
	{
		case FLD_TEXT:
		case FLD_TEXTAREA:
		case FLD_PASSWORD:
				SendMessageW(psIptInfo->psFld->wndInput,EM_SETSEL,0,-1);
				break;

		case FLD_DATE:
				SendMessageW(psIptInfo->psFld->wndInput,EM_SETSEL,0,-1);
				break;

		case FLD_NUMBER:
				pwcs=_LNumberText(psIptInfo->psFld,TRUE);
				SetWindowTextW(psIptInfo->psFld->wndInput,pwcs);
				ehFree(pwcs);
				SendMessageW(psIptInfo->psFld->wndInput,EM_SETSEL,0,-1);
				break;
	}
}

//
// _LGetForm() -> Trova la truttura del Form partendo dalla Hwnd dell' oggetto input
//
static EH_IPT_INFO * _LGetForm(HWND hWnd) {

	EH_IPT_INFO *psIptInfo=(EH_IPT_INFO *) GetWindowLong(hWnd,GWL_USERDATA);
	if (psIptInfo)
	{
		if (psIptInfo->psForm->dmiField.Hdl>0) psIptInfo->psFld=&psIptInfo->psForm->arField[psIptInfo->idxField];
	}
	return psIptInfo;
}

//
// _keyTabManager()
//
static void _keyTabManager(HWND hWnd,WPARAM wParam) {

	EH_FORM_FIELD * psFld=NULL;
	EH_IPT_INFO *psIptInfo;
	SHORT sL=GetAsyncKeyState(VK_LSHIFT);
	psIptInfo=(EH_IPT_INFO *) GetWindowLong(hWnd,GWL_USERDATA);
	if (psIptInfo) {

		if (psIptInfo->psForm->dmiField.Hdl>0) psFld=&psIptInfo->psForm->arField[psIptInfo->idxField];

	}
	if (!psFld) return;

	// Notifica se richiesta
	/*
	if (psIptInfo->psForm->BlurNotify&&psIptInfo->psForm->arBlurNotify)
	{
		if (ARIsIn(psIptInfo->psForm->arBlurNotify,psFld->pszName,FALSE)) 
			obj_putevent("%s.%s",psIptInfo->psForm->lpObj->nome,psFld->pszName);
	}
	*/
	_LFocusTab(psIptInfo->psForm,sL?-1:1);
}

//
// _LCRManager()
//
static void _LCRManager(HWND hWnd,WPARAM wParam) {

	EH_FORM_FIELD * psFld=NULL;
	EH_IPT_INFO *psIptInfo;

	psIptInfo=(EH_IPT_INFO *) GetWindowLong(hWnd,GWL_USERDATA);
	if (psIptInfo) {
		if (psIptInfo->psForm->dmiField.Hdl>0) psFld=&psIptInfo->psForm->arField[psIptInfo->idxField];
		if (psIptInfo->psForm->bCRSubmit) 
		{
			obj_putevent("%s.%s.CR",psIptInfo->psForm->psObj->nome,psFld->pszName);
			obj_putevent("%s:CR",psIptInfo->psForm->psObj->nome);
		}
	}
}

//
// _funcTextField - Funzione di superClassing per i campi di testo
//
LRESULT CALLBACK _funcTextField(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	BOOL bBlur=FALSE;
	EH_FORM_FIELD * psFld=NULL;
	EH_IPT_INFO *psIptInfo;
	WCHAR chChar,*pwcs;
	double dNumber;

	psIptInfo=_LGetForm(hWnd);
	if (psIptInfo) psFld=psIptInfo->psFld;
	
	switch (message)
	{
		case WM_DESTROY:	
			SetWindowLong(hWnd,GWL_USERDATA,(LONG) 0);
			psFld->pvIptInfo=NULL;
			ehFreeNN(psIptInfo);
			break;

		case WM_CHAR:
			chChar =  wParam;
			if (chChar==9) return FALSE;
			else if (chChar==13) 
			{
				if (psIptInfo->psForm->bCRSubmit) return FALSE;
			}
			if (psFld->iType==FLD_NUMBER)
			{
				
				if (chChar<32) break;
				if (chChar=='-') {wParam='-'; break;}

				if (chChar==','||chChar=='.')
					wParam=',';
				else
				{
					if (chChar<'0'||chChar>'9') return FALSE;
				}
			}
			break;

		case WM_KEYUP:
			if (psIptInfo->psForm->arCharNotify)
			{
				if (ARIsIn(psIptInfo->psForm->arCharNotify,psFld->pszName,FALSE)) 
					obj_putevent("%s.%s.CH",psIptInfo->psForm->psObj->nome,psFld->pszName);
			}
			break;

		case WM_KEYDOWN:
			if (wParam==9) {_keyTabManager(hWnd,wParam);}
			else if (wParam==13) {_LCRManager(hWnd,wParam);}
			break;

		case WM_SETFOCUS: 
			_LSetFocusIdx(hWnd);
			break;

		case WM_KILLFOCUS:

//			if (psIptInfo->psForm->BlurNotify&&psIptInfo->psForm->arBlurNotify)
			if (psIptInfo->psForm->arBlurNotify)
			{
				if (ARIsIn(psIptInfo->psForm->arBlurNotify,psFld->pszName,FALSE)) 
					obj_putevent("%s.%s",psIptInfo->psForm->psObj->nome,psFld->pszName);
			}


			if (psFld->iType==FLD_NUMBER) {

				pwcs=_LNumberText(psFld,FALSE);
				dNumber=_wtof(pwcs); 
				ehFree(pwcs);
				pwcs=_LNumberFormat(psFld,dNumber);
				SetWindowTextW(psFld->wndInput,pwcs);
				ehFree(pwcs);

			}
			break;

			
	}
	return CallWindowProc(sForm.funcTextOld,hWnd,message,wParam,lParam);
}


//
// _LfuncSelect()
//
LRESULT CALLBACK _LfuncSelect(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	switch (message)
	{
		case WM_SETFOCUS: _LSetFocusIdx(hWnd); break;
		case WM_KEYDOWN:
			if (wParam==9) _keyTabManager(hWnd,wParam);
			break;
	}

	return CallWindowProc(sForm.funcSelectOld,hWnd,message,wParam,lParam);
}

// _LfuncSelect()
//
LRESULT CALLBACK _LfuncList(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	switch (message)
	{
		case WM_SETFOCUS: _LSetFocusIdx(hWnd); break;
		case WM_KEYDOWN:
			if (wParam==9) _keyTabManager(hWnd,wParam);
			break;
	}
	return CallWindowProc(sForm.funcListOld,hWnd,message,wParam,lParam);
}

//
// _LfuncSelect()
//
LRESULT CALLBACK _LfuncButton(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {

	switch (message)
	{
		case WM_SETFOCUS: 
			_LSetFocusIdx(hWnd); 
			break;

		case WM_KEYDOWN:
			if (wParam==9) _keyTabManager(hWnd,wParam);
			break;

		case WM_KILLFOCUS:
		//	dispx("here");
			break;
	}
	return CallWindowProc(sForm.funcButtonOld,hWnd,message,wParam,lParam);
}

//
// LFldSetFocus()
// 
static void _LFldSetFocus(EH_FORM_FIELD * psFld) {

	if (psFld->enClass==FCLS_BUTTON)
		{
			winSetFocus(psFld->wndInput);
		// 	Button_SetState(psFld->wndInput,TRUE);
		}
		else 
		{
			winSetFocus(psFld->wndInput);
		}
}

//
// _LTextFldGet() - Text field get
//
static WCHAR * _LTextFldGet(EH_FORM_FIELD * psFld) {

	INT iMemo;
	WCHAR *pwcs;

	iMemo=GetWindowTextLength(psFld->wndInput);
	pwcs=ehAllocZero((iMemo+1)*2);
	GetWindowTextW(psFld->wndInput,pwcs,iMemo+1);
	return pwcs;

}

//
// _LSetFld()
//
static BOOL	_LSetFld(EHZ_FORM *psForm,EH_FORM_FIELD * psFld,CHAR *pszValue) {

	WCHAR *wcsValue;
	BYTE *pCode;
	INT a;
	BOOL bFound;

	//
	// Encoding
	//
	switch (psForm->enEncode)
	{
		case SE_ANSI:
			wcsValue=strToWcs(pszValue);
			break;

		case SE_UTF8:
			wcsValue=strDecode(pszValue,SE_UTF8,NULL);
			break;

		default: ehError(); // Encoding non gestito
			break;
	}

	switch (psFld->enClass) {

		case FCLS_TEXT:

			if (psFld->iType==FLD_NUMBER) {
				BYTE *p=wcsToStr(wcsValue); ehFree(wcsValue);
				wcsValue=_LNumberFormat(psFld,atof(p)); ehFree(p);
				
			}
			else if (psFld->iType==FLD_DATE) {

				BYTE *p=wcsToStr(wcsValue); ehFree(wcsValue);
				if (strlen(p)!=8) strAssign(&p,"");
				else {
					strcpy(p,dateYtoD(p));
					strAssign(&p,dateFor(p,"/"));
				}
				wcsValue=strToWcs(p);
				ehFree(p);
			}
			SetWindowTextW(psFld->wndInput,wcsValue);
			break;

		case FCLS_SELECT:
//		case FCLS_SELECT_MULTI:
			if (!psFld->arOption) break;
			bFound=FALSE;
			for (a=0;psFld->arOption[a];a++) {
				pCode=psFld->arOption[a]+strlen(psFld->arOption[a])+1;
				if (!strcmp(pCode,(BYTE *) pszValue)) 
				{
					SendMessage(psFld->wndInput,CB_SETCURSEL,a,0); // Seleziono 
					bFound=TRUE;
					break;
				}
			}
			if (!bFound) SendMessage(psFld->wndInput,CB_SETCURSEL,0,0); // Seleziono 
			break;

		case FCLS_LIST:
			if (!psFld->arOption) break;
			bFound=FALSE;
			for (a=0;psFld->arOption[a];a++) {
				pCode=psFld->arOption[a]+strlen(psFld->arOption[a])+1;
				if (!strcmp(pCode,(BYTE *) pszValue)) 
				{
					SendMessage(psFld->wndInput,LB_SETCURSEL,a,0); // Seleziono 
					bFound=TRUE;
					break;
				}
			}
			if (!bFound) SendMessage(psFld->wndInput,LB_SETCURSEL,0,0); // Seleziono 
			break;

		case FCLS_BUTTON:

			switch (psFld->iType) {
				case FLD_CHECKBOX:
					Button_SetCheck(psFld->wndInput,atoi(pszValue)?BST_CHECKED:BST_UNCHECKED);
					break;
			}
			break;

		case FCLS_UNKNOW:
			break;


		case FCLS_DATE: // Set
			{
				EH_TIME eht;
				SYSTEMTIME sST;
				if (strEmpty(pszValue)) {
					timeNow(&eht);
					DateTime_SetSystemtime(psFld->wndInput,GDT_NONE,timeEhtToSt(&sST, &eht));
				} else {

					//
					// Formato data
					//
					switch (psFld->enFdm) {

						case FDM_DMY8:
							timeSet(&eht,pszValue,NULL); 
							break;

						case FDM_YMD8:
							timeSet(&eht,dateYtoD(pszValue),NULL); 
							break;

						case FDM_DT:
							timeDtToEht(&eht,pszValue); 
							break;

					}
					DateTime_SetSystemtime(psFld->wndInput,GDT_VALID,timeEhtToSt(&sST, &eht));
				}
				_datePickerSetField(psFld);
				
			}
			break;

		default:
			ehExit("Non implementato: %s:%d",__FILE__,__LINE__);
	
	}

	ehFree(wcsValue);
	return FALSE;
}

//
// _LNumberFormat() - Formatta il numero per visualizzarlo
//
static WCHAR * _LNumberFormat(EH_FORM_FIELD * psFld,double dNumber) {

	CHAR *p;
	WCHAR *pw;
	p=Snummask(dNumber,psFld->iNumberSize,psFld->iNumberDecimal,psFld->bNumberThousandSep,FALSE);
	strTrim(p);
	pw=strToWcs(p); 
	return pw;
}

//
// _LNumberText() - Converte il testo formattato in stringa senza le formattazioni
//
static WCHAR * _LNumberText(EH_FORM_FIELD * psFld,BOOL bForInput) {
	
	WCHAR *pwcs;
	CHAR *psz,*pv;

	pwcs=_LTextFldGet(psFld);
	psz=wcsToStr(pwcs); ehFree(pwcs);
	while (strReplace(psz,".","")); // <-- Tolgo i punti della separazione delle Virgole

	//
	// Tolgo gli 0 in coda <------------------
	//
	pv=strstr(psz,","); 
	if (pv) { // Con virgola

		BYTE *p;
		for (p=psz+strlen(psz)-1;p>=pv;p--) {
			if (*p=='0'||*p==',') *p=0; else break;
		}
	}

	if (!bForInput) {
		strReplace(psz,",",".");
	}
	else
	{
		strReplace(psz,".",",");
	}
	pwcs=strToWcs(psz); ehFree(psz);
	return pwcs;

}






//
// ADB Functions
//

#if (defined(_ADB32)||defined(_ADB32P))

	//
	// adbGet() _this_AdbGet - 
	// Legge i valori
	//
	static BOOL	_this_adbRead(void *this, HDB hdb) {

		EHZ_FORM *psForm=this;
		EH_FORM_FIELD * psFld;
		INT a;
		BYTE *pFieldValue;
		BOOL bError=TRUE;
		struct ADB_REC * psFldInfo;
		EN_FLDTYPE enType;
		CHAR szServ[200];

		bError=FALSE;
		for (a=0;a<psForm->dmiField.Num;a++) {

			psFld=&psForm->arField[a]; if (!_isDataFld(psFld)) continue;
			if (!adb_FldInfo(hdb,psFld->pszName,&psFldInfo)) 
			{
				ehPrintd(__FUNCTION__":NomeField ? \n[%s]" CRLF,psFld->pszName);
				continue;
			}
			enType=psFldInfo->tipo;
			switch (enType) {
			
					case ADB_ALFA:
						pFieldValue=adb_FldPtr(hdb,psFld->pszName);//sql_ptr(rsSet,psFld->pszName);
						break;

					case ADB_BLOB:		// TEXT Note da 5 a 32K
						pFieldValue=adb_BlobGetAlloc(hdb,psFld->pszName);
						break;

					case ADB_DATA:
						pFieldValue=dateYtoD(adb_FldPtr(hdb,psFld->pszName));//sql_ptr(rsSet,psFld->pszName);
						break;

					case ADB_INT:		// Intero a 16bit
					case ADB_BOOL:		// Valore vero o falso
					case ADB_AINC:		// AutoIncrement  New 2000
					case ADB_INT32:		// Intero a 32 bit
						sprintf(szServ,"%d",adb_FldInt(hdb,psFld->pszName));
						pFieldValue=szServ;
						break;

					case ADB_NUME:
					case ADB_FLOAT:
					case ADB_COBD:		// Cobol Decimal  New 2000
					case ADB_COBN:		// Cobol Numeric  New 2000
						sprintf(szServ,"%.3f",adb_FldNume(hdb,psFld->pszName));
						pFieldValue=szServ;
						break;


					default:
					case ADB_GEOMETRY:	// Geometrico
					case ADB_POINT:		// Point
					case ADB_BINARY:		// Dati binary
					case ADB_TIMESTAMP:	// TimeStamp (UTC value)
						ehError();
			
			}
				

			_LSetFld(psForm,psFld,pFieldValue);
			if (enType==ADB_BLOB) ehFree(pFieldValue);
		}

		return FALSE;
	}


	//
	// adbUpdate() _this_AdbUpdate - 
	// Legge i valori
	//
	static BOOL	_this_adbWrite(void *this, HDB hdb, CHAR * pszField) {

		EHZ_FORM *psForm=this;
		EH_FORM_FIELD * psFld;
		INT a;
//		BYTE *pFieldValue;
		BOOL bError=TRUE;
		EH_AR arFld;
		struct ADB_REC * psFldInfo;
		EH_AR ar;
		INT iNum;
		BOOL bNumber;
		CHAR * pszFieldValue;

		//
		// Campi selezionati
		//
		if (pszField) 
			arFld=ARCreate(pszField,",",NULL);
		//
		// Tutti i campi
		//
		else
		{
			arFld=ARNew();
			for (a=0;a<psForm->dmiField.Num;a++) {
				psFld=psForm->arField+a; if (!_isDataFld(psFld)) continue;
				ARAdd(&arFld,psFld->pszName);
			}
		}

		for (a=0;arFld[a];a++) {
		
			BYTE *pszFormField,*pszAdbField;
			
			ar=ARCreate(arFld[a],"=",&iNum);
			if (iNum==1) pszFormField=pszAdbField=ar[0]; else {pszFormField=ar[0]; pszAdbField=ar[1];}
			psFld=_fldSearch(psForm,pszFormField,NULL,__LINE__); 
			
			if (!psFld) ehExit("adbUpdate(): campo %s inesistente",pszFormField);

			bNumber=false; 
			if (psFld->iType==FLD_NUMBER||psFld->iType==FLD_CHECKBOX) bNumber=true;

			if (strstr(pszAdbField,"(number)")) {strReplace(pszAdbField,"(number)",""); bNumber=true;}
			pszFieldValue=psForm->Get(psForm,pszFormField);

			// Cerco su DN
			if (!adb_FldInfo(hdb,pszAdbField,&psFldInfo)) 
				ehExit("adbUpdate(): campo db %s inesistente",pszAdbField);


			if (psFld->iMaxChar&&(INT) strlen(pszFieldValue)>psFld->iMaxChar) {
				ehAlert("Update non eseguito\nIl valore del campo %s è maggiore di %d caratteri.\nProbabile problema dopo codifica utf-8.",psFld->pszName,psFld->iMaxChar);
				bError=true;
				break;
			}
			pszFieldValue=strEver(pszFieldValue);
			switch (psFldInfo->tipo) {

					case ADB_ALFA:
					case ADB_BLOB:		// TEXT Note da 5 a 32K
						adb_FldWrite(hdb,pszAdbField,pszFieldValue,0);
						break;

					case ADB_DATA:
						adb_FldWrite(hdb,pszAdbField,pszFieldValue,0);
						break;

					case ADB_INT:		// Intero a 16bit
					case ADB_BOOL:		// Valore vero o falso
					case ADB_AINC:		// AutoIncrement  New 2000
					case ADB_INT32:		// Intero a 32 bit
						adb_FldWrite(hdb,pszAdbField,NULL,atoi(pszFieldValue));
						break;

					case ADB_NUME:
					case ADB_FLOAT:
					case ADB_COBD:		// Cobol Decimal  New 2000
					case ADB_COBN:		// Cobol Numeric  New 2000
						adb_FldWrite(hdb,pszAdbField,NULL,atof(pszFieldValue));
						break;


					default:
					case ADB_GEOMETRY:	// Geometrico
					case ADB_POINT:		// Point
					case ADB_BINARY:		// Dati binary
					case ADB_TIMESTAMP:	// TimeStamp (UTC value)
						ehError();
			}

/*
			if (bNumber) 
				ARAddarg(&arField,"%s=%s",pszSqlField,pszFieldValue);
				else
				{
					CHAR *pSql;
					pSql=strEncode(pszFieldValue,SE_SQL,NULL);
					ARAddarg(&arField,"%s='%s'",pszSqlField,pSql);
					ehFree(pSql);
				}
				*/
			ARDestroy(ar);


		}
/*
		bError=FALSE;
		for (a=0;a<psForm->dmiField.Num;a++) {

			psFld=&psForm->arField[a]; if (psFld->iType==FLD_BUTTON|| psFld->bQueryNot) continue;
			if (!adb_FldInfo(hdb,psFld->pszName,&psFldInfo)) 
			{
				ehPrintd(__FUNCTION__":NomeField ? \n[%s]" CRLF,psFld->pszName);
				continue;
			}
			if (psFldInfo->tipo==ADB_BLOB)
				pFieldValue=adb_BlobGetAlloc(hdb,psFld->pszName);
				else
				pFieldValue=adb_FldPtr(hdb,psFld->pszName);//sql_ptr(rsSet,psFld->pszName);

			_LSetFld(psForm,psFld,pFieldValue);
			if (psFldInfo->tipo==ADB_BLOB) ehFree(pFieldValue);
		}
*/
		return FALSE;
	}


	//
	// adbUpdate() _this_AdbUpdate - 
	// Legge i valori
	//
	static BOOL	_this_adbGetDifference(void *this, HDB hdb, HREC hRec, EH_LST lstField) {

		EHZ_FORM *psForm=this;
		EH_FORM_FIELD * psFld;
		struct ADB_REC * psFldInfo;
		INT a;
		EN_FLDTYPE enType;
		CHAR * pszFieldValue;
		INT iCompare;
		CHAR * pszValue;
		BOOL bNotEqual;

		// Loop sui campi nel form per determinare la differenza
		adb_get(hdb,hRec,-1);
		for (a=0;a<psForm->dmiField.Num;a++) {

			psFld=psForm->arField+a; if (!_isDataFld(psFld)) continue;
			
			// Cerco su Db
			if (!adb_FldInfo(hdb,psFld->pszName,&psFldInfo)) 
				ehExit("adbUpdate(): campo db %s inesistente",psFld->pszName);

			enType=psFldInfo->tipo;
			pszFieldValue=strEver(psForm->Get(psForm,psFld->pszName));
			bNotEqual=false;
			switch (enType) {
			
					case ADB_ALFA:
						pszValue=adb_FldPtr(hdb,psFld->pszName);
						bNotEqual=strCmp(pszFieldValue,pszValue);
						break;

					case ADB_BLOB:		// TEXT Note da 5 a 32K
						pszValue=adb_BlobGetAlloc(hdb,psFld->pszName);
						bNotEqual=strCmp(pszFieldValue,pszValue);
						ehFree(pszValue);
						break;

					case ADB_DATA:
						pszValue=dateYtoD(adb_FldPtr(hdb,psFld->pszName));
						iCompare=strCmp(pszFieldValue,pszValue);
						break;

					case ADB_INT:		// Intero a 16bit
					case ADB_BOOL:		// Valore vero o falso
					case ADB_AINC:		// AutoIncrement  New 2000
					case ADB_INT32:		// Intero a 32 bit
						bNotEqual=(adb_FldInt(hdb,psFld->pszName)!=atoi(pszFieldValue));
						break;

					case ADB_NUME:
					case ADB_FLOAT:
					case ADB_COBD:		// Cobol Decimal  New 2000
					case ADB_COBN:		// Cobol Numeric  New 2000
						bNotEqual=(adb_FldNume(hdb,psFld->pszName)!=atof(pszFieldValue));
						break;


					default:
					case ADB_GEOMETRY:	// Geometrico
					case ADB_POINT:		// Point
					case ADB_BINARY:		// Dati binary
					case ADB_TIMESTAMP:	// TimeStamp (UTC value)
						ehError();
			
			}
			if (bNotEqual) 
				lstPush(lstField,psFld->pszName);

		}
	
		return false;
	}
	


#endif
