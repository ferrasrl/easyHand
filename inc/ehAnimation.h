//   +-------------------------------------------+
//    ahAnimation.h
//    Header di Easyhand
//
//                by Ferrà Art & Technology 1999
//                by Ferrà srl 2013
//   +-------------------------------------------+

/*
typedef enum {

	ANI_MOVE_LINEAR,
	ANI_MOVE_LOG

} EN_ANI_TRNS;
*/

#ifndef EH_ANIMATION_SYSTEM
#define EH_ANIMATION_SYSTEM 1

#define ANI_TRANS_EASY			".25,.1,.25,1"
#define ANI_TRANS_LINEAR		"0,0,1,1"
#define ANI_TRANS_EASY_IN		"42,0,1,1"
#define ANI_TRANS_EASY_OUT		"0,0,.58,1"
#define ANI_TRANS_EASY_IN_OUT	".42,0,.58,1"
#define ANI_TRANS_FERRA_A		".1,1,.86,1"


// Item di animazione
typedef struct {

	INT			iType;		// 0=Nulla (sono notifica),1=Obj,2=Objs

	CHAR		szName[80];
	void *		pElement;	// obj/objs/wnd
	void		(*funcNotify)(struct ehAnimation * psAni,EN_MESSAGE cmd,LONG lInfo,void * ptr);	
	void *		pExtra;	// Dati aggiuntivi per funzione esterna
	
	S_BEZIER	sBez;
	DWORD		dwTime;
	RECT		rcStart;		// inizio
	RECT		rcEnd;			// Fine
	RECT		rcDelta;		// Delta degli spostamenti
	RECT		rcAni;			// Animazione (da usare in funzione esterna)

	BOOL		bDone;		// T/F Se l'animazione sull'item è finita

} S_ANI_ITEM;

// Descrizione dell'oggetto da animare
typedef struct {

	CHAR * pszItem;		// Nome dell'oggetto visuale

	DWORD dwTimeMilliSec;
	CHAR * pszTrans;

	EN_POSREF	enPosRef;
	POINT		ptObj;
	SIZE		sizObj;
	DOUBLE		dAlpha; // Da implementare

	// avanzate
	void		(*funcNotify)(struct ehAnimation * psAni,EN_MESSAGE cmd,LONG lInfo,void * ptr);
	void *		pExtra;
	RECT		rcObj;	// Dimensioni iniziali dell'oggetto se non recuperabili in altro modo

} S_ANI_OBJECT;

struct ehAnimation {

	INT idCode;
	CHAR		szName[80];	// Nome dell' animazione
	INT			iWin;		// Finestra di riferimento
	HWND		hWnd;
	BOOL		bBreak;		// usato per fermare l'animazione

	DWORD		idThread;
	HANDLE		hThread;	// Thread Loading
	DWORD		dwThread;
	HANDLE		arhEvent[4];	// Eventi Thread

	DWORD		dwTimeStart;
	EH_LST		lstItem;
	struct ehAnimation * psCue;	// Animazione in coda da far partire alla fine (NULL=Nessuna)
	BOOL		bFreeze;

	void (*addObject)(void * this,S_ANI_OBJECT * psObject);
	void (*addMove)(void * this,CHAR * pszName,DWORD dwTimeMilliSec,CHAR * pszTrans,EN_POSREF enPosRef,INT px,INT py);
	void (*addMoveSize)(void * this,CHAR * pszName,DWORD dwTimeMilliSec,CHAR * pszTrans,EN_POSREF enPosRef,INT px,INT py,INT cx,INT cy);
	void (*play)(void * this);
	void (*pause)(void * this);
	void (*stop)(void * this);
	void (*toEnd)(void * this);

};
typedef struct ehAnimation S_ANIMATION;
typedef S_ANIMATION * EH_ANIMATION;

EH_ANIMATION	aniCreate(CHAR * pszName,INT iWin);
EH_ANIMATION	aniGet(CHAR * pszName);

void aniDestroyWin(INT iWin);

#endif