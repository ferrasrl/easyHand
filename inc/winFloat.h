//   +-------------------------------------------+
//    winFloat.h
//
//								by Ferrà srl 2010
//   +-------------------------------------------+

#define WC_FLOAT_RESIZE_WE "wfBarResizeWE"
#define WC_FLOAT_RESIZE_NS "wfBarResizeNS"

typedef enum {
	WFP_FLOAT,			// Fluttuante
	WFP_MAIN,			// Finestra principale (il rimanente dello spazio)

	WFP_ANCHOR_LEFT,
	WFP_ANCHOR_TOP,
	WFP_ANCHOR_RIGHT,
	WFP_ANCHOR_BOTTOM,

} EN_POS_TYPE;

typedef struct {
	CHAR *	pszName;
	HWND	hWnd;
	BOOL	bHidden;

	HWND	wndRef;		// Finestra di riferimento

	EN_POS_TYPE enType;
	BOOL	bResizeManual;	// Chiedo ridimensionamento manuale
	RECT	rcWin;
	SIZE	sizWin;

	BOOL	bResizeBar;		// l'elemento è una barra di ridimensionamento
	HWND	wndResize;		// Elemento che vuole essere ridimensionato

} EH_WIN_FLOAT;

void * winFloat(EN_MESSAGE enMess,SINT iInfo,void *ptr);
void winChangeParam(HWND hWnd,INT iIndex,DWORD dwParam,BOOL bEnable);