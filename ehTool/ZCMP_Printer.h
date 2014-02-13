//   ---------------------------------------------
//   | ZCMP_Printer
//   ---------------------------------------------

void * ehzPrinter(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);

typedef struct {

	struct OBJ *lpObj;
	SINT	iLayout;
	CHAR *	pszDeviceDefine;
	BOOL	bDefault;
	BOOL	bMouseOver;

	DEVMODE *psDevMode;
	//SINT	iNumPapers;
	//EH_AR	arPapers;
	SINT	xName;	// x: dove finisce il nome della stampante
	SINT	xPaper;	// x: dove inizia il nome della carta
	SIZE	sPhysicalPage;

} EHZ_PRINTER;

