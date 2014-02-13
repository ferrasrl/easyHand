//   +-------------------------------------------+
//    accmeter.h
//    Header dell'accelerometro
//
//                by Ferrà srl 2010
//   +-------------------------------------------+
typedef struct {

	HWND	hwnd;
	HANDLE	hThread;
	DWORD	dwThread;
	BOOL	bExit;

	BOOL	bButton;			// T/F se il tasto è premuto
	POINT	ptPressPoint;		// pt dove si trova il cursore / dito
	POINT	ptPrevious;			// pt precedente
	
	double	dyAccRilev;			// Accelerazione rilevata (asse Y)
	double	dAccMaxThresold;	// Soglia massima di accelerazione 

	double	dyAcc;				// Accelerazione calcolata
	double	dxAcc;

	DWORD	dwReleaseCounter;	// Contatore di rilascio
	BOOL	bAccLoaded;			// Dati accelerometro pronti

	BOOL	bDrag;				// T/F se è in corso un drag
	POINT	ptDragPoint;		// Punto del drag

	POINT	ptOffset;			// Offset del contenuto
	POINT	ptOffsetDrag;

	SIZE	sizContent;			// Dimensioni del contenuto
	RECT	recClient;
	SIZE	sizClient;

} S_ACCMETER;

void *	AccMeter(EN_MESSAGE cmd,SINT,void *ptr);
void	AccMeterNotify(S_ACCMETER *psAccMeter,UINT message,WPARAM wParam,LPARAM lParam);
void	AccMeterReset(S_ACCMETER *psAccMeter);
void	AccMeterContentSize(S_ACCMETER *psAccMeter,SIZE *psSiz);
