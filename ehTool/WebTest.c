/* 
+ * WebBrowser Include 
+ * 
+ * Copyright 2005 James Hawkins 
+ * 
+ * This library is free software; you can redistribute it and/or 
+ * modify it under the terms of the GNU Lesser General Public 
+ * License as published by the Free Software Foundation; either 
+ * version 2.1 of the License, or (at your option) any later version. 
+ * 
+ * This library is distributed in the hope that it will be useful, 
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of 
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
+ * Lesser General Public License for more details. 
+ * 
+ * You should have received a copy of the GNU Lesser General Public 
+ * License along with this library; if not, write to the Free Software 
+ * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
+ */ 
 
#ifndef __WINE_WEBBROWSER_H 
#define __WINE_WEBBROWSER_H 
 
#include <stdarg.h> 
 
#define COBJMACROS 
 
#include "windef.h" 
#include "winbase.h" 
#include "winuser.h" 
#include "winnls.h" 
#include "ole2.h" 
 
#include "exdisp.h" 
#include "mshtml.h" 
#include "mshtmhst.h" 
 
#define WB_GOBACK       0 
#define WB_GOFORWARD    1 
#define WB_GOHOME       2 
#define WB_SEARCH       3 
#define WB_REFRESH      4 
#define WB_STOP         5 
 
typedef struct WBInfo 
{ 
    IOleClientSite *pOleClientSite; 
    IWebBrowser2 *pWebBrowser2; 
    IOleObject *pBrowserObject; 
    HWND hwndParent; 
} WBInfo; 
 
BOOL WB_EmbedBrowser(WBInfo *pWBInfo, HWND hwndParent); 
void WB_UnEmbedBrowser(WBInfo *pWBInfo); 
BOOL WB_Navigate(WBInfo *pWBInfo, LPCSTR szUrl); 
void WB_ResizeBrowser(WBInfo *pWBInfo, DWORD dwWidth, DWORD dwHeight); 
void WB_DoPageAction(WBInfo *pWBInfo, DWORD dwAction); 
 
#endif /* __WINE_WEBBROWSER_H */ 
 
/* 
 * WebBrowser Implementation 
 * 
 * Copyright 2005 James Hawkins 
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */ 
 
#define INITGUID 
#include "webbrowser.h" 
 
#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field)) 
 
static const SAFEARRAYBOUND ArrayBound = {1, 0}; 
 
typedef struct IOleClientSiteImpl 
{ 
    IOleClientSiteVtbl *lpVtbl; 
    IOleInPlaceSiteVtbl *lpvtblOleInPlaceSite; 
    IOleInPlaceFrameVtbl *lpvtblOleInPlaceFrame; 
    IDocHostUIHandlerVtbl *lpvtblDocHostUIHandler; 
 
    /* IOleClientSiteImpl data */ 
    IOleObject *pBrowserObject; 
    LONG ref; 
 
    /* IOleInPlaceFrame data */ 
    HWND hwndWindow; 
} IOleClientSiteImpl; 
 
static HRESULT STDMETHODCALLTYPE Site_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppvObj) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpVtbl, iface); 
    *ppvObj = NULL; 
 
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleClientSite)) 
    { 
        *ppvObj = This; 
    } 
    else if (IsEqualIID(riid, &IID_IOleInPlaceSite)) 
    { 
        *ppvObj = &(This->lpvtblOleInPlaceSite); 
    } 
    else if (IsEqualIID(riid, &IID_IDocHostUIHandler)) 
    { 
        *ppvObj = &(This->lpvtblDocHostUIHandler); 
    } 
    else 
        return E_NOINTERFACE; 
 
    return S_OK; 
} 
 
static ULONG STDMETHODCALLTYPE Site_AddRef(IOleClientSite *iface) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpVtbl, iface); 
    return InterlockedIncrement(&This->ref); 
} 
 
static ULONG STDMETHODCALLTYPE Site_Release(IOleClientSite *iface) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpVtbl, iface); 
    LONG refCount = InterlockedDecrement(&This->ref); 
 
    if (refCount) 
        return refCount; 
 
    HeapFree(GetProcessHeap(), 0, This); 
    return 0; 
} 
 
static HRESULT STDMETHODCALLTYPE Site_SaveObject(IOleClientSite *iface) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Site_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Site_GetContainer(IOleClientSite *iface, LPOLECONTAINER *ppContainer) 
{ 
    *ppContainer = NULL; 
 
    return E_NOINTERFACE; 
} 
 
static HRESULT STDMETHODCALLTYPE Site_ShowObject(IOleClientSite *iface) 
{ 
    return NOERROR; 
} 
 
static HRESULT STDMETHODCALLTYPE Site_OnShowWindow(IOleClientSite *iface, BOOL fShow) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Site_RequestNewObjectLayout(IOleClientSite *iface) 
{ 
    return E_NOTIMPL; 
} 
 
static IOleClientSiteVtbl MyIOleClientSiteTable = 
{ 
    Site_QueryInterface, 
    Site_AddRef, 
    Site_Release, 
    Site_SaveObject, 
    Site_GetMoniker, 
    Site_GetContainer, 
    Site_ShowObject, 
    Site_OnShowWindow, 
    Site_RequestNewObjectLayout 
}; 
 
static HRESULT STDMETHODCALLTYPE UI_QueryInterface(IDocHostUIHandler *iface, REFIID riid, LPVOID *ppvObj) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblDocHostUIHandler, iface); 
    return Site_QueryInterface((IOleClientSite *)This, riid, ppvObj); 
} 
 
static ULONG STDMETHODCALLTYPE UI_AddRef(IDocHostUIHandler *iface) 
{ 
    return 1; 
} 
 
static ULONG STDMETHODCALLTYPE UI_Release(IDocHostUIHandler * iface) 
{ 
    return 2; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_ShowContextMenu(IDocHostUIHandler *iface, DWORD dwID, POINT __RPC_FAR *ppt, IUnknown __RPC_FAR *pcmdtReserved, IDispatch __RPC_FAR *pdispReserved) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_GetHostInfo(IDocHostUIHandler *iface, DOCHOSTUIINFO __RPC_FAR *pInfo) 
{ 
    pInfo->cbSize = sizeof(DOCHOSTUIINFO); 
    pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER; 
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT; 
 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_ShowUI(IDocHostUIHandler *iface, DWORD dwID, IOleInPlaceActiveObject __RPC_FAR *pActiveObject, IOleCommandTarget __RPC_FAR *pCommandTarget, IOleInPlaceFrame __RPC_FAR *pFrame, IOleInPlaceUIWindow __RPC_FAR *pDoc) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_HideUI(IDocHostUIHandler *iface) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_UpdateUI(IDocHostUIHandler *iface) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_EnableModeless(IDocHostUIHandler *iface, BOOL fEnable) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_OnDocWindowActivate(IDocHostUIHandler *iface, BOOL fActivate) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_OnFrameWindowActivate(IDocHostUIHandler *iface, BOOL fActivate) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_ResizeBorder(IDocHostUIHandler *iface, LPCRECT prcBorder, IOleInPlaceUIWindow __RPC_FAR *pUIWindow, BOOL fRameWindow) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_TranslateAccelerator(IDocHostUIHandler *iface, LPMSG lpMsg, const GUID __RPC_FAR *pguidCmdGroup, DWORD nCmdID) 
{ 
    return S_FALSE; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_GetOptionKeyPath(IDocHostUIHandler *iface, LPOLESTR __RPC_FAR *pchKey, DWORD dw) 
{ 
    return S_FALSE; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_GetDropTarget(IDocHostUIHandler *iface, IDropTarget __RPC_FAR *pDropTarget, IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget) 
{ 
    return S_FALSE; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_GetExternal(IDocHostUIHandler *iface, IDispatch __RPC_FAR *__RPC_FAR *ppDispatch) 
{ 
    *ppDispatch = NULL; 
    return S_FALSE; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_TranslateUrl(IDocHostUIHandler *iface, DWORD dwTranslate, OLECHAR __RPC_FAR *pchURLIn, OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut) 
{ 
    *ppchURLOut = NULL; 
    return S_FALSE; 
} 
 
static HRESULT STDMETHODCALLTYPE UI_FilterDataObject(IDocHostUIHandler *iface, IDataObject __RPC_FAR *pDO, IDataObject __RPC_FAR *__RPC_FAR *ppDORet) 
{ 
    *ppDORet = NULL; 
    return S_FALSE; 
} 
 
static IDocHostUIHandlerVtbl MyIDocHostUIHandlerTable = 
{ 
    UI_QueryInterface, 
    UI_AddRef, 
    UI_Release, 
    UI_ShowContextMenu, 
    UI_GetHostInfo, 
    UI_ShowUI, 
    UI_HideUI, 
    UI_UpdateUI, 
    UI_EnableModeless, 
    UI_OnDocWindowActivate, 
    UI_OnFrameWindowActivate, 
    UI_ResizeBorder, 
    UI_TranslateAccelerator, 
    UI_GetOptionKeyPath, 
    UI_GetDropTarget, 
    UI_GetExternal, 
    UI_TranslateUrl, 
    UI_FilterDataObject 
}; 
 
static HRESULT STDMETHODCALLTYPE InPlace_QueryInterface(IOleInPlaceSite *iface, REFIID riid, LPVOID *ppvObj) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceSite, iface); 
    return Site_QueryInterface((IOleClientSite *)This, riid, ppvObj); 
} 
 
static ULONG STDMETHODCALLTYPE InPlace_AddRef(IOleInPlaceSite *iface) 
{ 
    return 1; 
} 
 
static ULONG STDMETHODCALLTYPE InPlace_Release(IOleInPlaceSite *iface) 
{ 
    return 2; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_GetWindow(IOleInPlaceSite *iface, HWND *lphwnd) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceSite, iface); 
    *lphwnd = This->hwndWindow; 
 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_CanInPlaceActivate(IOleInPlaceSite *iface) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceActivate(IOleInPlaceSite *iface) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_OnUIActivate(IOleInPlaceSite *iface) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_GetWindowContext(IOleInPlaceSite *iface, LPOLEINPLACEFRAME *lplpFrame, LPOLEINPLACEUIWINDOW *lplpDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceSite, iface); 
    *lplpFrame = (LPOLEINPLACEFRAME)&This->lpvtblOleInPlaceFrame; 
    *lplpDoc = NULL; 
 
    lpFrameInfo->fMDIApp = FALSE; 
    lpFrameInfo->hwndFrame = This->hwndWindow; 
    lpFrameInfo->haccel = NULL; 
    lpFrameInfo->cAccelEntries = 0; 
 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_Scroll(IOleInPlaceSite *iface, SIZE scrollExtent) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_OnInPlaceDeactivate(IOleInPlaceSite *iface) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_DiscardUndoState(IOleInPlaceSite *iface) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_DeactivateAndUndo(IOleInPlaceSite *iface) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE InPlace_OnPosRectChange(IOleInPlaceSite *iface, LPCRECT lprcPosRect) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceSite, iface); 
    IOleInPlaceObject *inplace; 
 
    if (!IOleObject_QueryInterface(This->pBrowserObject, &IID_IOleInPlaceObject, (void **)&inplace)) 
        IOleInPlaceObject_SetObjectRects(inplace, lprcPosRect, lprcPosRect); 
 
    return S_OK; 
} 
 
static IOleInPlaceSiteVtbl MyIOleInPlaceSiteTable = 
{ 
    InPlace_QueryInterface, 
    InPlace_AddRef, 
    InPlace_Release, 
    InPlace_GetWindow, 
    InPlace_ContextSensitiveHelp, 
    InPlace_CanInPlaceActivate, 
    InPlace_OnInPlaceActivate, 
    InPlace_OnUIActivate, 
    InPlace_GetWindowContext, 
    InPlace_Scroll, 
    InPlace_OnUIDeactivate, 
    InPlace_OnInPlaceDeactivate, 
    InPlace_DiscardUndoState, 
    InPlace_DeactivateAndUndo, 
    InPlace_OnPosRectChange 
}; 
 
static HRESULT STDMETHODCALLTYPE Frame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, LPVOID *ppvObj) 
{ 
    return E_NOTIMPL; 
} 
 
static ULONG STDMETHODCALLTYPE Frame_AddRef(IOleInPlaceFrame *iface) 
{ 
    return 1; 
} 
 
static ULONG STDMETHODCALLTYPE Frame_Release(IOleInPlaceFrame *iface) 
{ 
    return 2; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_GetWindow(IOleInPlaceFrame *iface, HWND *lphwnd) 
{ 
    ICOM_THIS_MULTI(IOleClientSiteImpl, lpvtblOleInPlaceFrame, iface); 
    *lphwnd = This->hwndWindow; 
 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_RequestBorderSpace(IOleInPlaceFrame *iface, LPCBORDERWIDTHS pborderwidths) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_SetBorderSpace(IOleInPlaceFrame *iface, LPCBORDERWIDTHS pborderwidths) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_SetActiveObject(IOleInPlaceFrame *iface, IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE Frame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID) 
{ 
    return E_NOTIMPL; 
} 
 
static IOleInPlaceFrameVtbl MyIOleInPlaceFrameTable = 
{ 
    Frame_QueryInterface, 
    Frame_AddRef, 
    Frame_Release, 
    Frame_GetWindow, 
    Frame_ContextSensitiveHelp, 
    Frame_GetBorder, 
    Frame_RequestBorderSpace, 
    Frame_SetBorderSpace, 
    Frame_SetActiveObject, 
    Frame_InsertMenus, 
    Frame_SetMenu, 
    Frame_RemoveMenus, 
    Frame_SetStatusText, 
    Frame_EnableModeless, 
    Frame_TranslateAccelerator 
}; 
 
static HRESULT STDMETHODCALLTYPE Storage_QueryInterface(IStorage *This, REFIID riid, LPVOID *ppvObj) 
{ 
    return E_NOTIMPL; 
} 
 
static ULONG STDMETHODCALLTYPE Storage_AddRef(IStorage *This) 
{ 
    return 1; 
} 
 
static ULONG STDMETHODCALLTYPE Storage_Release(IStorage *This) 
{ 
    return 2; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_CreateStream(IStorage *This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_OpenStream(IStorage *This, const WCHAR * pwcsName, void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_CreateStorage(IStorage *This, const WCHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_OpenStorage(IStorage *This, const WCHAR * pwcsName, IStorage * pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_CopyTo(IStorage *This, DWORD ciidExclude, IID const *rgiidExclude, SNB snbExclude,IStorage *pstgDest) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_MoveElementTo(IStorage *This, const OLECHAR *pwcsName,IStorage * pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_Commit(IStorage *This, DWORD grfCommitFlags) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_Revert(IStorage *This) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_EnumElements(IStorage *This, DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_DestroyElement(IStorage *This, const OLECHAR *pwcsName) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_RenameElement(IStorage *This, const WCHAR *pwcsOldName, const WCHAR *pwcsNewName) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_SetElementTimes(IStorage *This, const WCHAR *pwcsName, FILETIME const *pctime, FILETIME const *patime, FILETIME const *pmtime) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_SetClass(IStorage *This, REFCLSID clsid) 
{ 
    return S_OK; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_SetStateBits(IStorage *This, DWORD grfStateBits, DWORD grfMask) 
{ 
    return E_NOTIMPL; 
} 
 
static HRESULT STDMETHODCALLTYPE Storage_Stat(IStorage *This, STATSTG *pstatstg, DWORD grfStatFlag) 
{ 
    return E_NOTIMPL; 
} 
 
static IStorageVtbl MyIStorageTable = 
{ 
    Storage_QueryInterface, 
    Storage_AddRef, 
    Storage_Release, 
    Storage_CreateStream, 
    Storage_OpenStream, 
    Storage_CreateStorage, 
    Storage_OpenStorage, 
    Storage_CopyTo, 
    Storage_MoveElementTo, 
    Storage_Commit, 
    Storage_Revert, 
    Storage_EnumElements, 
    Storage_DestroyElement, 
    Storage_RenameElement, 
    Storage_SetElementTimes, 
    Storage_SetClass, 
    Storage_SetStateBits, 
    Storage_Stat 
}; 
 
static IStorage MyIStorage = { &MyIStorageTable }; 
 
/* FIXME: Make this a global macro somehow */ 
static LPWSTR WB_ANSIToUnicode(LPCSTR ansi) 
{ 
    LPWSTR unicode; 
    int count; 
 
    count = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0); 
    unicode = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count * sizeof(WCHAR)); 
    MultiByteToWideChar(CP_ACP, 0, ansi, -1, unicode, count); 
 
    return unicode; 
} 
 
BOOL WB_EmbedBrowser(WBInfo *pWBInfo, HWND hwndParent) 
{ 
    IOleClientSiteImpl *iOleClientSiteImpl; 
    IOleObject *browserObject; 
    IWebBrowser2 *webBrowser2; 
    HRESULT hr; 
    RECT rc; 
 
    static const WCHAR hostNameW[] = {'H','o','s','t',' ','N','a','m','e',0}; 
 
    iOleClientSiteImpl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                   sizeof(IOleClientSiteImpl)); 
    if (!iOleClientSiteImpl) 
        return FALSE; 
 
    iOleClientSiteImpl->ref = 0; 
    iOleClientSiteImpl->lpVtbl = &MyIOleClientSiteTable; 
    iOleClientSiteImpl->lpvtblOleInPlaceSite = &MyIOleInPlaceSiteTable; 
    iOleClientSiteImpl->lpvtblOleInPlaceFrame = &MyIOleInPlaceFrameTable; 
    iOleClientSiteImpl->hwndWindow = hwndParent; 
    iOleClientSiteImpl->lpvtblDocHostUIHandler = &MyIDocHostUIHandlerTable; 
 
    hr = OleCreate(&CLSID_WebBrowser, &IID_IOleObject, OLERENDER_DRAW, 0, 
                   (IOleClientSite *)iOleClientSiteImpl, &MyIStorage, 
                   (void **)&browserObject); 
    if (FAILED(hr)) goto error; 
 
    /* make the browser object accessible to the IOleClientSite implementation */ 
    iOleClientSiteImpl->pBrowserObject = browserObject; 
    IOleObject_SetHostNames(browserObject, hostNameW, 0); 
 
    GetClientRect(hwndParent, &rc); 
 
    hr = OleSetContainedObject((struct IUnknown *)browserObject, TRUE); 
    if (FAILED(hr)) goto error; 
 
    hr = OleSetContainedObject((struct IUnknown *)browserObject, TRUE); 
    if (FAILED(hr)) goto error; 
 
    hr = IOleObject_DoVerb(browserObject, OLEIVERB_SHOW, NULL, 
                           (IOleClientSite *)iOleClientSiteImpl, 
                           -1, hwndParent, &rc); 
    if (FAILED(hr)) goto error; 
 
 
    hr = IOleObject_QueryInterface(browserObject, &IID_IWebBrowser2, 
                                   (void **)&webBrowser2); 
    if (SUCCEEDED(hr)) 
    { 
        IWebBrowser2_put_Left(webBrowser2, 0); 
        IWebBrowser2_put_Top(webBrowser2, 0); 
        IWebBrowser2_put_Width(webBrowser2, rc.right); 
        IWebBrowser2_put_Height(webBrowser2, rc.bottom); 
 
        pWBInfo->pOleClientSite = (IOleClientSite *)iOleClientSiteImpl; 
        pWBInfo->pBrowserObject = browserObject; 
        pWBInfo->pWebBrowser2 = webBrowser2; 
        pWBInfo->hwndParent = hwndParent; 
 
        return TRUE; 
    } 
 
error: 
    WB_UnEmbedBrowser(pWBInfo); 
 
    if (webBrowser2) 
        IWebBrowser2_Release(webBrowser2); 
 
    HeapFree(GetProcessHeap(), 0, iOleClientSiteImpl); 
 
    return FALSE; 
} 
 
void WB_UnEmbedBrowser(WBInfo *pWBInfo) 
{ 
    if (pWBInfo->pBrowserObject) 
    { 
        IOleObject_Close(pWBInfo->pBrowserObject, OLECLOSE_NOSAVE); 
        IOleObject_Release(pWBInfo->pBrowserObject); 
        pWBInfo->pBrowserObject = NULL; 
    } 
 
    if (pWBInfo->pWebBrowser2) 
    { 
        IWebBrowser2_Release(pWBInfo->pWebBrowser2); 
        pWBInfo->pWebBrowser2 = NULL; 
    } 
 
    if (pWBInfo->pOleClientSite) 
    { 
        IOleClientSite_Release(pWBInfo->pOleClientSite); 
        pWBInfo->pOleClientSite = NULL; 
    } 
} 
 
BOOL WB_Navigate(WBInfo *pWBInfo, LPCSTR szUrl) 
{ 
    IWebBrowser2 *pWebBrowser2 = pWBInfo->pWebBrowser2; 
    VARIANT myURL; 
    LPWSTR urlW; 
 
    if (!pWebBrowser2) 
        return FALSE; 
 
    VariantInit(&myURL); 
    myURL.n1.n2.vt = VT_BSTR; 
 
    if (!(urlW = WB_ANSIToUnicode(szUrl))) 
        return FALSE; 
 
    myURL.n1.n2.n3.bstrVal = SysAllocString(urlW); 
    HeapFree(GetProcessHeap(), 0, urlW); 
 
    if (!myURL.n1.n2.n3.bstrVal) 
        return FALSE; 
 
    IWebBrowser2_Navigate2(pWebBrowser2, &myURL, 0, 0, 0, 0); 
    VariantClear(&myURL); 
 
    return TRUE; 
} 
 
void WB_ResizeBrowser(WBInfo *pWBInfo, DWORD dwWidth, DWORD dwHeight) 
{ 
    IWebBrowser2 *pWebBrowser2 = pWBInfo->pWebBrowser2; 
 
    if (!pWebBrowser2) 
        return; 
 
    IWebBrowser2_put_Width(pWebBrowser2, dwWidth); 
    IWebBrowser2_put_Height(pWebBrowser2, dwHeight); 
} 
 
void WB_DoPageAction(WBInfo *pWBInfo, DWORD dwAction) 
{ 
    IWebBrowser2 *pWebBrowser2 = pWBInfo->pWebBrowser2; 
 
    if (!pWebBrowser2) 
        return; 
 
    switch (dwAction) 
    { 
        case WB_GOBACK: 
            IWebBrowser2_GoBack(pWebBrowser2); 
            break; 
        case WB_GOFORWARD: 
            IWebBrowser2_GoForward(pWebBrowser2); 
            break; 
        case WB_GOHOME: 
            IWebBrowser2_GoHome(pWebBrowser2); 
           break; 
        case WB_SEARCH: 
            IWebBrowser2_GoSearch(pWebBrowser2); 
            break; 
        case WB_REFRESH: 
            IWebBrowser2_Refresh(pWebBrowser2); 
        case WB_STOP: 
            IWebBrowser2_Stop(pWebBrowser2); 
    } 
} 
  
