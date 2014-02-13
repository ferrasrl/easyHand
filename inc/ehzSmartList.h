//   ---------------------------------------------
//   | ehSmartList
//   ---------------------------------------------

#include "/easyhand/inc/cssParser.h"
#define WC_EH_SMARTLIST "ehSmartList"
#define WC_EH_SMARTLIST_HEADER  "ehSmartListHeader"


typedef enum {

	SCP_CUSTOM=0x00,
	SCP_INTERNAL=0x01,		// Stampa interna no custome
	SCP_BEFORE=0x02,
	SCP_AFTER=0x04

} EN_SL_CP;

typedef enum {

	CRES_NONE,
	CRES_CHECKBOX

} EN_COL_RES;

typedef struct {

	INT			idCode;
	CHAR *		pszCode;
	CHAR *		pszTitle;

	EN_CSS_WIDTH	enWidth;
	double			dWidth;			// Larghezza della colonna

	EN_CSS_WIDTH	enMinWidth;
	double			dMinWidth;			// Larghezza della colonna

	EN_CSS_WIDTH	enMaxWidth;
	double			dMaxWidth;			// Larghezza della colonna

	EN_DPL		enAlign;		// Allineamento
	BOOL		bVisible;		// Se la colonna è visible
	BOOL		bSelectable;	// Se è una colonna selezionabile (per renderla visibile oppure no)
	EN_SL_CP	enItemPaint;	// Informazioni sul paint della colonna è customizzato
	EN_COL_RES  enReserved;		// Colonne con disegno riservato

	SIZE		sizHeader;
	RECT		recHeader;
	RECT		recText;
	EH_COLOR	colBack;
	EH_COLOR	colText;
	INT			iSort;		// 0 la colonna non prevede ordinamento (click disattivato), 1=Ascendente, 2=Discendente
	INT			iOrder;		// 0=non usata, 1= Ascendente, 2=Discendente
	BOOL		bFix;		// La colonna non scorre : è fissa
	BOOL		bCellEdit;
	RECT		recShadow;
	EH_FONT	*	psFont;
	
} S_SL_COL;

typedef struct {

	INT			idRow;
	BOOL		bChecked;			// T/F se è selezionato
	BOOL		bCheckboxHidden;	// T/F se il checkbox è nascosto
	RECTD		recCheck;

} S_SLROWI;

typedef struct {

	HDC			hdc;
	S_SL_COL *  psCol;
	S_SLROWI *	psRowInfo;
	INT			iRow;
	INT			iCol;
	RECT		recCell;
	RECT		recText;
	EH_COLOR	colBack;
	EH_COLOR	colText;
	EH_FONT	*	psFont;
	CHAR *		pszText;	
	
} S_SL_CELL; // Non memorizzata, di appoggio per comunicazioni


typedef struct {

	EH_OBJ *	psObj;
	HWND		wnd;	// window principale con elenco
	EH_DG *		psDgMain;
	
	HWND		wndHeader;
	EH_DG *		psDgHeader;

	_DMI		dmiCol;
	S_SL_COL *	arsCol;
	BOOL		bBuilded;		// Form Pronto per essere visualizzato	
	EN_STRENC	enEncode;		// SE_ANSI (Default)
	INT			iErrorLevel;	// 0= No error, 1=Warning, 2=Several

	INT			iFocusRow;
	INT			iFocusCol;
	INT			iBodyMouseOverRow;
	INT			iBodyMouseOverCol;

	INT			bHeadMouserOver;
	INT			iHeadMouseOverCol;

	// Font da usare
	EH_FONT	*	psFontTitle;
	EH_FONT	*	psFontText;
	EH_COLOR	colBack;
	EH_COLOR	colText;

	BOOL		bHeadToolBar;		// Se è gestita la tool bar
	BOOL		bHeadToolBarShow;	// Se è visibile
	BOOL		iHeadToolBarHeight;	// Altezza Tool bar
	INT			iHeadHeightNormal;	// Altezza dell'header normale
	INT			iHeadHeight;		// Altezza dell'header normale
//	INT			iHeadHeightOpen;	// Altezza dell'header aperto per la tool bar
	INT			iItemHeight;	
	INT			iCellPadding;
	INT			ofsVert;		// Offset verticale
	INT			ofsHorz;		// Offset orizzontale
	INT			iRows;			// Numero di righe
	INT			iArrowWidth;	// default 9
	S_SLROWI *  arsRowsInfo;	// Array di informazioni delle righe memorizzate
	
	BOOL		bRowCheckBox;		// T/F Le linee hanno il checkbox di selezione
	BOOL		bRowRequest;		// T/F Se bisogna richiedere lo status delle righe
	CHAR *		pszTitleProcess;	// 
	
	RECT		rcClient;		// Dimensione Client (Workarea)
	SIZE		sizClient;		// 
	SIZE		sizTable;		// Dimenzione della tabella da scorrere
	EH_COLOR	colLine;		// Color linea di separazione
	CHAR *		pszCellBuffer;	// Buffer usato per le celle
	void *		(*funcNotify)(void * this,EH_SRVPARAMS);

	// Interfacche
	BOOL	(* addCol)(void *, INT idCode, CHAR * pszCode, CHAR * pszTitle, CHAR * pszStyle); 
	BOOL	(* setItemCount)(void * this, INT iItems); 
	BOOL	(* setNotify)(void * this,void * (*funcNotify)(void * this,EH_SRVPARAMS));
	BOOL	(* refresh)(void * this);
	BOOL	(* refreshEx)(void * this,INT iRow,INT iCol);
	BOOL	(* ensureVisible)(void * this,INT iRow,INT iCol,RECT * psRect);
	BOOL	(* getRect)(void * this,INT iRow,INT iCol,RECT * psRect);
	BOOL	(* getCell)(void * this,INT iRow,INT iCol,S_SL_CELL * psCell);
	BOOL	(* getCellEvent)(void * this,EH_EVENT * psEvent,S_SL_CELL * psCell);
	BOOL	(* cellWalk)(void * this,INT * piRow,INT * piCol, INT iKey);
	BOOL	(* setFocusRow)(void * this,INT iRow);
	BOOL	(* setFocusCell)(void * this,INT iRow,INT iCell);
	BOOL	(* setVScroll)(void * this,INT iSBMode,INT iOffset);
	BOOL	(* setHeaderHeight)(void * this,INT iHeight);
	BOOL	(* setColVisibility)(void * this,CHAR * pszCode,BOOL bVisible);
	BOOL	(* setBodyStyle)(void * this,CHAR * pszParams);

} EHZ_SMARTLIST;

//	INT		(* colEdit)(void * this,INT * piRow,INT * piCol,INT iColStep);

void * ehzSmartList(EH_OBJPARAMS);