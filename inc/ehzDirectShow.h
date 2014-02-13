//   ---------------------------------------------
//   | ehzDirectShow.h
//   | 
//   |                                              
//   |							by Ferrà srl 2013
//   ---------------------------------------------

/*
typedef struct {

	BOOL	(*Open)(void *, EN_FORM_IPT iType, CHAR *pName, CHAR *pszText,CHAR *pszButton,CHAR *pszParam); 
	/*
	void	(*Reset)(void *);
	BOOL	(*Add)(void *, EN_FORM_IPT iType, CHAR *pName, CHAR *pszText,CHAR *pszButton,CHAR *pszParam); 
	BOOL	(*Show)(void *); //  Visualizza il form
	BOOL	(*SetOptions)(void *,CHAR *pszId,EH_AR ar);
	BOOL	(*Focus)(void *,CHAR *pszId);
	void *	(*Get)(void *,CHAR *pszId);

} EHZ_DS_WRAPPER;
*/


typedef struct {

	HWND			hWnd;	// Finestra create
	void	*		pClass;	// Puntatore alla Classe C++ del video
	EH_OBJ *		psObj;

	// Metodi
	BOOL	(*Open)	(void *pClass, UTF8 * putfFileName); 
	BOOL	(*Close)(void *pClass); 
	BOOL	(*Show)(void *pClass); 
	BOOL	(*Hide)(void *pClass); 

	BOOL	(*Play)(void *pClass); 
	BOOL	(*Pause)(void *pClass); 
	BOOL	(*Stop)(void *pClass); 
	BOOL	(*SetPosition)(void *pClass,double dStart,double dStop); 

} EHZ_DIRECTSHOW;

#ifdef __cplusplus
extern "C" {
#endif


void * ehzDirectShow(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);
//int WINAPI playStart(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow);

#ifdef __cplusplus
		}
#endif


#ifdef __cplusplus

#include <dshow.h>
#include <d3d9.h>
#include <Vmr9.h>
#include <Evr.h>

//
// iceCream (c++)
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

class CVideoRenderer;

enum PlaybackState
{
    STATE_NO_GRAPH,
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_STOPPED,
};

const UINT WM_GRAPH_EVENT = WM_APP + 1;

typedef void (CALLBACK *GraphEventFN)(HWND hwnd, long eventCode, LONG_PTR param1, LONG_PTR param2);

class DShowPlayer
{
public:
    DShowPlayer(HWND hwnd);
    ~DShowPlayer();

    PlaybackState State() const { return m_state; }

    HRESULT OpenFile(PCWSTR pszFileName);
    
    HRESULT Play();
    HRESULT Pause();
    HRESULT Stop();
	HRESULT SetPosition(double dSecStart,double dSecStop);

    BOOL    HasVideo() const;
    HRESULT UpdateVideoWindow(const LPRECT prc);
    HRESULT Repaint(HDC hdc);
    HRESULT DisplayModeChanged();

    HRESULT HandleGraphEvent(GraphEventFN pfnOnGraphEvent);

private:
    HRESULT InitializeGraph();
    void    TearDownGraph();
    HRESULT CreateVideoRenderer();
    HRESULT RenderStreams(IBaseFilter *pSource);

    PlaybackState   m_state;

    HWND m_hwnd; // Video window. This window also receives graph events.

    IGraphBuilder   *	m_pGraph;
    IMediaControl   *	m_pControl;
    IMediaEventEx   *	m_pEvent;
    CVideoRenderer  *	m_pVideo;
	IMediaSeeking   *	m_pSeek; 
//	IAMVideoTransform * m_Trans;
//	IVMRMixerControl9 *	m_pMixer;
};



// video.h

// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once


template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

HRESULT RemoveUnconnectedRenderer(IGraphBuilder *pGraph, IBaseFilter *pRenderer, BOOL *pbRemoved);
HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName);

// Abstract class to manage the video renderer filter.
// Specific implementations handle the VMR-7, VMR-9, or EVR filter.

class CVideoRenderer
{
public:
    virtual ~CVideoRenderer() {};
    virtual BOOL    HasVideo() const = 0;
    virtual HRESULT AddToGraph(IGraphBuilder *pGraph, HWND hwnd) = 0;
    virtual HRESULT FinalizeGraph(IGraphBuilder *pGraph) = 0;
    virtual HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc) = 0;
    virtual HRESULT Repaint(HWND hwnd, HDC hdc) = 0;
    virtual HRESULT DisplayModeChanged() = 0;
};

// Manages the VMR-7 video renderer filter.

class CVMR7 : public CVideoRenderer
{
    IVMRWindowlessControl   *m_pWindowless;

public:
    CVMR7();
    ~CVMR7();
    BOOL    HasVideo() const;
    HRESULT AddToGraph(IGraphBuilder *pGraph, HWND hwnd);
    HRESULT FinalizeGraph(IGraphBuilder *pGraph);
    HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc);
    HRESULT Repaint(HWND hwnd, HDC hdc);
    HRESULT DisplayModeChanged();
};


// Manages the VMR-9 video renderer filter.

class CVMR9 : public CVideoRenderer
{
    IVMRWindowlessControl9 *m_pWindowless;

public:
    CVMR9();
    ~CVMR9();
    BOOL    HasVideo() const;
    HRESULT AddToGraph(IGraphBuilder *pGraph, HWND hwnd);
    HRESULT FinalizeGraph(IGraphBuilder *pGraph);
    HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc);
    HRESULT Repaint(HWND hwnd, HDC hdc);
    HRESULT DisplayModeChanged();
};


// Manages the EVR video renderer filter.

class CEVR : public CVideoRenderer
{
    IBaseFilter            *m_pEVR;
    IMFVideoDisplayControl *m_pVideoDisplay;

public:
    CEVR();
    ~CEVR();
    BOOL    HasVideo() const;
    HRESULT AddToGraph(IGraphBuilder *pGraph, HWND hwnd);
    HRESULT FinalizeGraph(IGraphBuilder *pGraph);
    HRESULT UpdateVideoWindow(HWND hwnd, const LPRECT prc);
    HRESULT Repaint(HWND hwnd, HDC hdc);
    HRESULT DisplayModeChanged();
};
#endif