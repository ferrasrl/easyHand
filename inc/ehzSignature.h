//   ---------------------------------------------
//   | ehzSignature
//   | Componente per gestire una firma
//   |                                              
//   |							by Ferrà srl 2013
//   ---------------------------------------------

#include "/easyhand/ehtool/imgutil.h"
#include "/easyhand/inc/cssParser.h"
#include <math.h>
#include <time.h>
void * ehzSignature(EH_OBJPARAMS);

//
// Strutture
//

typedef struct {

	IMGHEADER ImgHead; // Header
	INT hdlSign; // Handle della memoria
	BYTE *lpbSignStruct; // Puntatore alla struttura
	BYTE *lpbSignBitMap; // Puntatore al bitmap
	RECT rArea;

} S_SIGNAREA;

/*
typedef struct {

	BOOL fPenStart;
	POINT pLast;

} S_SIGNPEN;
*/
typedef enum{
	
	PEN_POINT,
	PEN_DOWN,
	PEN_UP

} EN_PTTY;


typedef struct {

	EN_PTTY	enType; // 0=Posizione di un punto , 1=Penna abbassata, 2= Penna alzata
	POINT	pt;
	INT		iClock;

} S_SIGNPOINT;


typedef struct {

	EH_OBJ	*	psObj;
	INT			iWin;
	HWND		wndParent;
	HWND		wndSign;

	EH_COLOR	colBack;
	BOOL		bSignRecordLock; // T/F Blocco il disegno della firma
	BOOL		bEngineLock;	 // T/F blocco engine (?)

	EH_IMG		imgPen;			// Immagine della penna
	SIZED		sizPen;			// Dimensioni della penna
	EH_ACOLOR	acPen;
	double		dAlpha;
	S_SIGNAREA	sSign;

	double		dBaseLinePos;
	double		dBaseLineWeight;
	EH_ACOLOR	acBaseLine;
	INT			enBaseLineStyle;
	CHAR *		pszBaseLineText;
	EH_FONT	*	psFontBaseLine;
	EH_COLOR	colBaseLineText;

	EH_LST		lstPoint;	// Elenco dei segmenti

	SIZE		sizArea;
	RECT		recArea;
	EH_DG *		dgSign;

	// Usato per generare immagini
	double		dScale;	// Scala nel disegnare la firma
	POINT		ptOfs;	// Offset della firma

	// Controllo scrittura
	BOOL		bPenWrite;
	BOOL		bPenStart;
	POINT		pLast;

	// Methods
	void		(*reset)(void *);
	void		(*refresh)(void *);
	void		(*setFont)(void *,CHAR *);
	void		(*getSignRect)(void *,RECT * precSign);
	void		(*maximize)(void *,double dPerc,EN_DPL enAlign);

	void		(*readSign)(void *,CHAR *);
	BOOL		(*writeSign)(void *,CHAR *);
	BOOL		(*writeImage)(void * this, CHAR * pszFileName, EN_FILE_TYPE enType, INT cx,INT cy);

} EHZ_SIGNATURE;

