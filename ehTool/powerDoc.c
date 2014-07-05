//-----------------------------------------------------------------------
// powerDoc	Gestione di un documento in stampa	
//			  Ex LptReportImg/PowerDoc
//
//  
//											              Ferrà srl 2010
//-----------------------------------------------------------------------

// Creare un buffer fonts

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/powerDoc.h"
#include "/easyhand/ehtool/imgutil.h"
#include <math.h>

// #define TRACE_MOUSE_PREVIEW	// Mostra la posizione in UM del mouse sul preview
#define HD_DPI 30000.0 // Profondita HD in DPI per determinare le dimensioni in UM (2014)

#define _DTXD (_sPd.bVirtual?PUM_DTXP:PUM_DTX)
#define _DTYD (_sPd.bVirtual?PUM_DTYP:PUM_DTY)

#ifdef EH_PDF 
// /easyhand/extSource/libharu
	#include "/easyhand/Ehtoolx/libharu-2.1.0/include/hpdf.h" 
	#include "/easyhand/Ehtoolx/libharu-2.1.0/include/hpdf_conf.h" 
	#include "/easyhand/Ehtoolx/libharu-2.1.0/include/hpdf_encoder.h" 
	#include "/easyhand/Ehtoolx/libharu-2.1.0/include/hpdf_utils.h" 

	#pragma message("--> Includo /easyhand/ehtoolx/libharu-2.1.0/lib/libhpdf.lib <-------------------")
	#pragma comment(lib, "/easyhand/ehtoolx/libharu-2.1.0/lib/libhpdf.lib")

/*
	
//	#pragma comment(lib, "/easyhand/extSource/libharu/lib/libhpdf.lib")
	#include "/easyhand/extSource/libharu/include/hpdf.h" 
	#include "/easyhand/extSource/libharu/include/hpdf_conf.h" 
	#include "/easyhand/extSource/libharu/include/hpdf_encoder.h" 
	#include "/easyhand/extSource/libharu/include/hpdf_utils.h" 

	#pragma message("--> Includo /easyhand/extSource/libharu/lib/libhpdf.lib <-------------------")
	#pragma comment(lib, "/easyhand/extSource/libharu/lib/libhpdf.lib")
*/

static CHAR *_pdfStrDecode(WCHAR *pwcText);

#endif

#ifndef ICM_DONE_OUTSIDE_DC
#define ICM_DONE_OUTSIDE_DC 4
#endif

#define PREVIEW_MARGIN 5

typedef struct {

	WCHAR *		pwcText;
	INT			iLength;
	HFONT		hFont;
	//SIZE		sizText;
	PWD_SIZE	sumText;

} S_FAC;


#define PWZ_PAGE	0
#define PWZ_WIDTH	-1

static struct {
	
	BOOL		bReady;
	INT			idWinWait;

	// Pubblica --------------------
//	EH_PWD	*	psDi;

	// Privata --------------------

//	CHAR		szTempPath[500];
	CHAR		szTempFile[500];
	FILE *		chTempFile;
//	INT		hmField;//=-1;
//	BOOL		fArray;
	EH_LST		lstElement;

	BOOL		bVirtual;	// T/F se devo fare i calcoli fisici in modo virtuale

	// Preview	
	HWND		wndPreview;	// Finestra di preview
	INT			winPreview;	// indicie Finestra di easyhand
	HWND		wndPreviewMain;
	RECT		recClient;		// Dimensioni della finesta di preview
	SIZE		sizClient;
	INT			iPageZoom;
	double		dZoomPerc;		// Percentuale di zoom attuale

	// VP - Virtual Page
#ifdef EH_GDI_API
	EH_DG *		psDg;
#endif

	SIZE		sizWorkSpace;	// Dimensioni dell'area che contiene la pagina
	SIZE		sizPreviewPage;	// Dimensioni della pagina di preview (bitmap)
	RECT		recPreviewPage;	// Posizione della virtual page nel workspace
	POINT		ptWorkOffset;	// Offset della visualizzazione del workspace

	UINT		yBarra;         // Spesso TOOLBAR in alto
//	UINT		yBorder,xBorder;   // Margini
//	POINT		ptPage;
//	SHORT		iLayOrientation;

	BYTE *		pbFileTemp;	
	INT			nItems;			// Numero degli items
	PWD_ITEM **	arsItem;
	INT   *		ariPageIndex;
	INT			iPageView;
	HFONT		hFontBase;
	POINT		ptOffset;

	BYTE *		lpItemBuffer;
	LONG		lItemBufferSize;
	LONG		lItemBufferCount;

	BOOL		bUserAbort;
	HWND		hDlgPrint;


#ifdef EH_PDF

	HPDF_Doc	pdfDoc;
	HPDF_Page *	pdfPage;
	HPDF_Font	pdfFont;
	HPDF_Image	pdfImage;
	jmp_buf		pdfEnv;
	BYTE	  * pbCharMap;	
	EH_AR		arPngTemp;
//	_DMI		dmiPdfFont;

#endif


} _sPd={0,-1};

static	EH_PWD	_sPower;

//
//	Dichiarazioni funzioni statiche private
//
static void *	_itemBuilder(	PWD_TE		uType,
								PWD_RECT *	prumObj,	// Puntatore all'area occupata dall'oggetto
								void *		psObj,			// Puntatore alla struttura di dettaglio del tipo
								INT			iObjSize,		// Dimensioni della struttura
								INT *		lpiSize);

static PWD_ITEM *_LItemNext(PWD_ITEM *pbItem);

static void		_LFileTempLoad(void);
static void		_freeResource(BOOL bDeleteFile);
static void		_LFileTempReplacing(void);
static EH_TSTYLE _getActualStyle(void);
static WCHAR *	_strTextDecode(BYTE *pText);
static BOOL		_docPreview(CHAR *);
static BOOL		_LPrintDirect(CHAR *lpParam);

static void		_drawLayout(void);
static void		_flushBuffer(BOOL fOutput);
static void		_LWinStart(void);
static BOOL		_addItem(PWD_TE uType,PWD_RECT *prumObj,void *psObj,INT iSizeObj);
static BOOL		_addItemMem(PWD_TE uType,PWD_RECT *prumObj,void *psObj,INT iSizeObj);

static void		_LItemReposition(void);


typedef enum {

	PMA_MISURE,		// Misurazione
	PMA_PREVIEW,	// Stampa in preview
	PMA_PRINT,		// Stampa definitiva

} EN_PMODE;	// Modalità di calcolo/stampa

static	void	_fontPowerText( EN_PMODE	enMode,	
								HDC			hdc,
								PDO_TEXT *	psText,
//								BOOL		bPrint,
								PWD_TXI *	psTi); // new 2014
								
						//BOOL		bPreview
					

static S_FAC *	_fontPowerCreate(	HDC			hdc,
									BOOL		bVirtual,	// T/F se devo calcolare in virtual (preview)
									PDO_TEXT *	psText,		// OGgetto da stampare
									BOOL		bDcAlign,
									TEXTMETRIC * psTextMetrics);

static void		_LFontAmbientDestroy(S_FAC *psFac);
static void		_LdcBoxDotted(HDC hdc,INT x,INT y,INT x2,INT y2,LONG Color);
static void		_LTempWrite(void *pb,SIZE_T iLen);
static void		_fontEnumeration(BOOL bFontAlloc);
static BOOL     _fontFileSearch(PWD_FONT *psPwdFont, CHAR  *pszFontPath, BOOL * pbEmb);
static LONG		_getNextNameValue(HKEY key, LPCTSTR pszSubkey, LPTSTR pszName, LPTSTR pszData);

static EH_LST	_pathCreate(PDO_PATH * psPath);
static void		_pathDestroy(EH_LST lstPath);

//
// Funzioni statiche PDF
//
#ifdef EH_PDF
static void		_pdfErrorHandler(HPDF_STATUS   error_no,
					   HPDF_STATUS   detail_no,
                       void          *user_data);

INT _LPdfTextAlign(INT iAllinea);
static BOOL		_pdfBuilder(INT iPageStart,INT iPageEnd);
static void		_pdfMapBuilder(CHAR *pszEncode);
#endif

static void _imageResampleSave(INT hImage, SIZE sizImageSorg, SIZE sizImageDest,CHAR *pszTempFile);
//
// Funzioni locali di disegno
//
#ifndef EH_CONSOLE
static void		_pagePaint(HDC hDC,INT nPage,RECT *lpRect);
static void		_drawText(PWD_DRAW * psDraw); 
static void		_drawTextBox(PWD_DRAW *psDraw);
static void		_drawRect(PWD_DRAW *psDraw);
static void		_drawLine(PWD_DRAW *psDraw);
static void		_drawImage(PWD_DRAW *psDraw);
static void		_drawPath(PWD_DRAW * psDraw);
static void		_ImgOutEx(HDC hDC,
					   INT PosX,INT PosY,
					   INT SizeX,INT SizeY,
					   INT hdlImage);
#endif




//
// PowerDoc()
//
void * PowerDoc(EN_MESSAGE enCmd,LONG info,void *ptr)
{
	PWD_FIELD *psField;
	INT a;
	LONG l;
	INT iAuto=0;
	double dTotPerc=0;
	PWD_FONT_RES * psFontRes=NULL;

	INT iFieldPerRiga;

//	PDO_TEXT		sText;
	PDO_BOXLINE     sBox;
	PWD_RECT		rumRect;
	PWD_RECT		rumChar;

#ifndef EH_CONSOLE
	S_WINSCENA WScena;
#endif
	PWD_NOTIFY sNotify;
	void	*pRet=NULL;

	// Inizializzo la prima volta
	if (!_sPd.bReady) {
		
		_(_sPd);
		//_sPd.hmField=-1;
		_sPd.bReady=TRUE;
		_sPd.yBarra=45;
	}

	switch (enCmd)
	{

		// Inizializzo
		case WS_CREATE:

			_flushBuffer(FALSE);
			if (_sPd.lstElement) PowerDoc(WS_CLOSE,FALSE,NULL);
			_(_sPower);
			_sPower.iVer=0; // Default per compatibilità

			_sPower.enLayStyle=PWD_PAGEFREE;	// Tipo di Layout
			_sPower.enMisure=info;
			_sPower.fDate=FALSE;				// SI/NO Stampa della data
			_sPower.lpDate="/";				// Formato data
	
			_sPower.fPag=FALSE;				// Stampa del numero di pagina
			_sPower.iPagStyle=1;				// Formato numero di pagina


			_sPower.fLineHorzField=FALSE;//TRUE;		// Stampa separazioni orizzontali
			_sPower.colLineVert=pwdGray(.8);//RGB(128,128,128));
			_sPower.colLineHorz=pwdGray(.8);//RGB(128,128,128));
			_sPower.iLineHorzStyle=PS_SOLID;
			_sPower.colTitleBack=pwdGray(.6);//RGB(128,128,128));
			_sPower.colTitleText=PDC_WHITE;

			_sPower.fLineVertField=true;		// Stampa separazione verticali

			_sPower.lpTitolo=NULL;
			_sPower.lpSottoTitolo=NULL;//szAzienda;//Titolo;	// Sotto titolo del report
			_sPower.pszFontTitleDefault="Arial";			// Font del titolo
			_sPower.pszFontBodyDefault="Arial";//Times New Roman"; // Font del corpo

			DMIReset(&_sPower.dmiFont);
			_sPower.arsFont=NULL;
			_sPower.lstFontRes=lstCreate(sizeof(PWD_FONT_RES));
			pRet=&_sPower;
			_sPd.bVirtual=false;
			break;

		case WS_INF:
			pRet=&_sPower;
			break;




		//-----------------------------------------------------------------------
		//	WS_OPEN                         		                            |
		//											                            |
		//	Apertura del server di Stampa report                                |
		//  info=0                                                              |
		//  ptr= * EH_PWD (Inizializzazione dei parametri)                      |
		//                                                                      |
		//-----------------------------------------------------------------------
		case WS_OPEN: // Inizializzazione del report
			
			// Inizializzo il resto della struttura
			_sPower.iPageCount=0;
			_sPower.bBold=FALSE;
			_sPower.bItalic=FALSE;
			_sPower.bUnderLine=FALSE;
			if (_sPd.lstElement) _sPd.lstElement=lstDestroy(_sPd.lstElement);
			//_sPd.fArray=FALSE; 
			//_sPd.hmField=-1;


			// Apro un'array di LRField
			if (_sPower.enLayStyle!=PWD_PAGEFREE)
			{
//				 a=sizeof(PWD_FIELD);
				 _sPd.lstElement=lstCreate(sizeof(PWD_FIELD));
				 //ARMaker(WS_OPEN,&a);
//				 _sPd.fArray=TRUE; 
			}
			
			_sPower.iFieldNum=0;
			//GetTempPath(sizeof(_sPd.szTempPath),_sPd.szTempPath);
			// GetTempFileName(_sPower.pszTempFolder,"LREP",0,_sPd.szTempFile);
			fileTempName(_sPower.pszTempFolder,"PDR",_sPd.szTempFile,FALSE);
			_sPd.chTempFile=fopen(_sPd.szTempFile,"wb"); if (!_sPd.chTempFile) ehExit("LR1");

			_sPower.iLinePerRiga=0; // Azzero il numero di linee per riga (gestione multi riga)
			_sPower.iRowsSize=0;
			
			//
			// Notifico apertura del documento
			//
			if (_sPower.funcNotify) 
			{
				_(sNotify);
				sNotify.psPower=&_sPower;
				_sPower.funcNotify(WS_OPEN,0,NULL); 
			}

#ifndef EH_CONSOLE			
			_LWinStart();
#endif

			//
			// Cerco le dimensioni del documento
			// Se vuoto, uso il PD di sistema (compatibilità con il vecchio)
			//

			if (strEmpty(_sPower.pszDeviceDefine)&&sys.sPrintDlg.hDC) {
					
					_sPower.hdcPrinter=sys.sPrintDlg.hDC;
					_sPower.pDevMode=GlobalLock(sys.sPrintDlg.hDevMode);
					_sPower.iOrientation=_sPower.pDevMode->dmOrientation;
					GlobalUnlock(_sPower.pDevMode);

			} else {

				// Cerco la stampante di default
//#ifdef ehPrinterGetDefault
#ifdef EH_GDI_API
				if (!_sPower.pszDeviceDefine) {
					ehPrinterGetDefault(&_sPower.pszDeviceDefine);
				}
#endif
//#endif
				//
				// *********** PDF *****************
				//
				if (!strBegin(_sPower.pszDeviceDefine,"!PDF"))
				{
#ifdef EH_PDF 
					PWD_FORMINFO sFormInfo;
					HPDF_Page psPage;
					INT iPdfPage;

					ehPdfGetForm(_sPower.pszDeviceDefine,&sFormInfo);

					// 
					// Chiedo le misure del documento
					//
					_sPower.bPdf=TRUE; // Generazione del pdf
					_sPower.iOrientation=sFormInfo.iOrientation;
					_sPower.sizDotPerInch.cx=300;
					_sPower.sizDotPerInch.cy=300;

					// Creazione Documento PDF
					_sPd.pdfDoc= HPDF_New(_pdfErrorHandler, NULL); if (!_sPd.pdfDoc) ehError();
					// Aggiungo un pagina
					psPage=HPDF_AddPage(_sPd.pdfDoc);//,HPDF_PAGE_SIZE_A4,HPDF_PAGE_PORTRAIT);

					//
					// Setto le dimensioni del documento
					//
					iPdfPage=HPDF_PAGE_SIZE_A4;
					switch (sFormInfo.iPaper) {
					
							case DMPAPER_LETTER: iPdfPage=HPDF_PAGE_SIZE_LETTER; break;
							case DMPAPER_LEGAL: iPdfPage=HPDF_PAGE_SIZE_LEGAL; break;
							case DMPAPER_A4: iPdfPage=HPDF_PAGE_SIZE_A4; break;
								/*
								    HPDF_PAGE_SIZE_A5,
    HPDF_PAGE_SIZE_B4,
    HPDF_PAGE_SIZE_B5,
    HPDF_PAGE_SIZE_EXECUTIVE,
    HPDF_PAGE_SIZE_US4x6,
    HPDF_PAGE_SIZE_US4x8,
    HPDF_PAGE_SIZE_US5x7,
    HPDF_PAGE_SIZE_COMM10,
*/    
							default: ehError();
					
					}
					HPDF_Page_SetSize(psPage,iPdfPage,(sFormInfo.iOrientation==DMORIENT_PORTRAIT)?HPDF_PAGE_PORTRAIT:HPDF_PAGE_LANDSCAPE);
					_sPower.sumPaper.cx=_sPower.sumPhysicalPage.cx=pwdUm(PUM_PT,HPDF_Page_GetWidth(psPage));
					_sPower.sumPaper.cy=_sPower.sumPhysicalPage.cy=pwdUm(PUM_PT,HPDF_Page_GetHeight(psPage));
					HPDF_Free(_sPd.pdfDoc);

#else
					ehExit("EH_PDF non definito");					
#endif
				}
#ifdef EH_GDI_API
				else
				{
					_sPower.hdcPrinter=ehPrinterCreateDC(_sPower.pszDeviceDefine,&_sPower.pDevMode);
					_sPower.iOrientation=_sPower.pDevMode->dmOrientation;
				}
#endif

	//					alert("qui");
	/*
					HANDLE hPrinter;
					BOOL bRet;
					PRINTER_DEFAULTS sPrnDefault;

					bRet=OpenPrinter((char*) _sPower.szPrinterName, &hPrinter, &sPrnDefault);
					if (!bRet) ehError();


					ClosePrinter(&hPrinter);
	*/			

			}

			if (_sPower.hdcPrinter) {

				_sPower.sizDotPerInch.cx=GetDeviceCaps(_sPower.hdcPrinter,LOGPIXELSX);
				_sPower.sizDotPerInch.cy=GetDeviceCaps(_sPower.hdcPrinter,LOGPIXELSY);

				_sPower.sumPhysicalPage.cx=pwdUm(PUM_DTX,GetDeviceCaps(_sPower.hdcPrinter,PHYSICALWIDTH));
				_sPower.sumPhysicalPage.cy=pwdUm(PUM_DTY,GetDeviceCaps(_sPower.hdcPrinter,PHYSICALHEIGHT));
				_sPower.sumPaper.cx=pwdUm(PUM_DTX,GetDeviceCaps(_sPower.hdcPrinter,HORZRES));
				_sPower.sumPaper.cy=pwdUm(PUM_DTY,GetDeviceCaps(_sPower.hdcPrinter,VERTRES));
			}
						
			if (!_sPower.sumPaper.cx||!_sPower.sumPaper.cy) ehExit("Stampante/Formato carta non selezionato (%s:%d)",__FILE__,__LINE__);
			break;

	//-----------------------------------------------------------------------
	//	WS_CLOSE                        		                            |
	//											                            |
	//	Chiusura del server di Stampa report                                |
	//  info=FALSE non stampare (abort)                                     |
	//		 TRUE  stampa il rapport                                        |
	//											                            |
	//	ptr = NULL stampa diretta                                           |
	//  ptr = !=NULL  Preview della stampa                                  |
	//                                                                      |
	//-----------------------------------------------------------------------
		case WS_CLOSE: // Stampa/Preview del report

			//
			// Ho una pagina in sospeso (chiudo la pagina)
			//
			if (_sPower.bPageInProgress)
			{
				//
				// Richiedo stampa del footer
				//
				if (_sPower.funcNotify) 
				{
					_(sNotify);
					sNotify.psPower=&_sPower;
					sNotify.enType=PDN_FOOT;
					sNotify.bLastPage=TRUE;
					sNotify.rumRect.left=_sPower.rumPage.left;
					sNotify.rumRect.top=_sPower.umBodyBottom;//>rPage.bottom-_sPower.yCueDot;
					sNotify.rumRect.right=_sPower.rumPage.right;
					sNotify.rumRect.bottom=_sPower.rumPage.bottom;
					_sPower.funcNotify(WS_DO,0,&sNotify);
				}

				_sPower.bPageInProgress=FALSE;
			}

#ifndef EH_CONSOLE			
			win_close();
#endif
			// Chiudo il file
			_addItem(PDT_ENDFILE,NULL,NULL,0);
			ehFreePtr(&_sPower.psLastItemWrite);
			fclose(_sPd.chTempFile); 

			_LFileTempReplacing();

#ifndef EH_CONSOLE			
			if (info) // stampare
			{
				if (ptr) 
					_docPreview((CHAR *) ptr);	
					else
					_LPrintDirect((CHAR *) ptr);
			}
#else
			_LPrintDirect((CHAR *) ptr);
#endif

			if (_sPower.funcNotify) 
			{
				_(sNotify);
				sNotify.psPower=&_sPower;
				_sPower.funcNotify(WS_CLOSE,0,NULL);
			}

			// Libera le risorse impegnate
			_freeResource(true);

			//
			// Rilascio font temporanei
			//
			for (lstLoop(_sPower.lstFontRes,psFontRes)) {
				if (!strEmpty(psFontRes->szFileName)) fontUninstall(psFontRes->szFileName);
			}
			_sPower.lstFontRes=lstDestroy(_sPower.lstFontRes);
			if (_sPd.lstElement) _sPd.lstElement=lstDestroy(_sPd.lstElement);
			if (_sPower.dmiFont.Hdl>-1) DMIClose(&_sPower.dmiFont,"Font");
			break;
	
	//-----------------------------------------------------------------------
	//	WS_SETFLAG                      		                            |
	//											                            |
	//	Inizializzo la struttura LRFIELD con i dati di default              |
	//  ptr= * LRFIELD                                                      |
	//                                                                      |
	//-----------------------------------------------------------------------

		case WS_SETFLAG:
			
			if (ptr==NULL) ehExit("LR2");
			psField=(PWD_FIELD *) ptr;
			memset(psField,0,sizeof(PWD_FIELD));
			psField->iRowsSize=1; // Altezza di default
			switch (info)
			{
				case ALFA:

					psField->fFix=FALSE;
					psField->enAlign=PDA_LEFT;
					psField->iTipo=ALFA;
					psField->enTitleAlign=PDA_LEFT;
					break;

				case NUME:

					psField->fFix=TRUE;
					psField->enAlign=PDA_RIGHT;
					psField->iTipo=NUME;
					psField->enTitleAlign=PDA_RIGHT;
					break;
			}

			psField->colTitBack=_sPower.colTitleBack;//pwdGray(.6);
			psField->colTitText=_sPower.colTitleText;
			psField->colText=PDC_BLACK;
			psField->colBack=PDC_TRASP;//pwdColor(-1);
			break;

	//-----------------------------------------------------------------------
	//	WS_ADD                          		                            |
	//											                            |
	//	Aggiunge un definizione di un campo                                 |
	//  ptr= * LRFIELD                                                      |
	//                                                                      |
	//-----------------------------------------------------------------------
		case WS_ADD:

			switch (info)
			{
				// Aggiunga la definizione di un campo
				case LR_ADDFIELD: 

					if (ptr==NULL) ehExit("LR5");
					psField=(PWD_FIELD *) ptr;
					// Controlla il numero di linee
					if (psField->iLineInRow>_sPower.iLinePerRiga) 
						_sPower.iLinePerRiga=psField->iLineInRow;
					// Controlla il numero di "rows" per linea
					if (psField->iRowsSize!=LR_ROWSDYNAMIC) 
					{
						if ((psField->iLineInRow+psField->iRowsSize)>_sPower.iRowsSize) 
						_sPower.iRowsSize=psField->iLineInRow+psField->iRowsSize;
					}
					else
					{
						_sPower.bRowsDynamic=true; // Segnalo che sono presenti linee dinamiche
					}
					pRet=lstPush(_sPd.lstElement,ptr);
//					ARMaker(WS_ADD,ptr);
					break;

				case LR_ADDPAGE:

					if (_sPower.bPageInProgress)
					{
						if (_sPower.funcNotify) {
							_(sNotify);
							sNotify.psPower=&_sPower;
							sNotify.enType=PDN_FOOT;
							sNotify.rumRect.left=_sPower.rumPage.left;
							sNotify.rumRect.top=_sPower.umBodyBottom;
							sNotify.rumRect.right=_sPower.rumPage.right;
							sNotify.rumRect.bottom=_sPower.rumPage.bottom;
							(*_sPower.funcNotify)(WS_DO,0,&sNotify);
						}
					    _sPower.bPageInProgress=FALSE;
					}

					_sPower.iVirtualLineCount=0;
					_sPower.umRowCursor=_sPower.umBodyTop;
					_sPower.iPageCount++; 
#ifndef EH_CONSOLE
					WinWriteSet(_sPd.idWinWait,&WScena);
					ipt_writevedi(0,"",_sPower.iPageCount);
					WinWriteRestore(&WScena);
#endif
					_drawLayout();

					// Se esiste un HookSubProc segnalo il cambio di pagina
					if (_sPower.funcNotify)
					{
						_(sNotify);
						sNotify.psPower=&_sPower;
						sNotify.enType=PDN_HEAD;
						sNotify.rumRect.left=_sPower.rumPage.left;
						sNotify.rumRect.top=_sPower.rumPage.top;
						sNotify.rumRect.right=_sPower.rumPage.right;
						sNotify.rumRect.bottom=_sPower.umHeadBottom;
						_sPower.funcNotify(WS_DO,0,&sNotify);
					}
					_sPower.bPageInProgress=TRUE;

			}
			break;

	//-----------------------------------------------------------------------
	//	WS_DO                           		                            |
	//											                            |
	//	Prima elaborazione e definizione array                              |
	//											                            |
	//	Chiudo l'array di definizione campi                                 |
	//  Nessun parametro                                                    |
	//-----------------------------------------------------------------------
		
		case WS_DO:
		
			if (_sPower.enLayStyle!=PWD_PAGEFREE) 
			{
				if (!_sPd.lstElement) ehExit("LR6"); 
				_sPower.iFieldNum=_sPd.lstElement->iLength;

			} else {
			//	_sPower.arsField=NULL;  
				_sPower.iFieldNum=0;
			}
			

			// -------------------------------------------------------------------------
			// Leggo e calcolo i dati metrici del foglio di stampa
			// -------------------------------------------------------------------------
			_sPower.iLinePerRiga++;


			/*
			{
				DEVMODE *lpDevMode;
				lpDevMode=GlobalLock(sys.sPrintDlg.hDevMode);
				win_infoarg("%s\n[%d,%d] [%d,%d]",
						    lpDevMode->dmDeviceName,
							_sPower.sPaper.cx,_sPower.sPaper.cy,
							GetDeviceCaps(sys.sPrintDlg.psDraw->hDC,HORZRES),
							(INT) lpDevMode->dmPaperLength);
				GlobalUnlock(sys.sPrintDlg.hDevMode);
			}
*/

			// Dovrei settarlo in modo parametrico
			// Field Margin
			_sPower.rumFieldPadding.left=pwdUm(PUM_STD,8);//LRInchToDotX(8);
			_sPower.rumFieldPadding.right=pwdUm(PUM_STD,8);//LRInchToDotX(8); // LRInchToDotX(2);
			_sPower.rumFieldPadding.top=pwdUm(PUM_STD,8);//pwdUm(PUM_STD,8);
			_sPower.rumFieldPadding.bottom=pwdUm(PUM_STD,8);//pwdUm(PUM_STD,8);
			
			//_sPower.umRowHeight=pwdUm(PUM_STD,_sPower.ptRowHeight);
//			_sPower.ySectDot=_sPower.iRowsSize*_sPower.umCharHeight;

			_sPower.umTitlePadded=(_sPower.iRowsSize*_sPower.umTitleHeight)+(_sPower.rumFieldPadding.top+_sPower.rumFieldPadding.bottom);
			_sPower.umRowPadded=(_sPower.iRowsSize*_sPower.umRowHeight)+(_sPower.rumFieldPadding.top+_sPower.rumFieldPadding.bottom);

			// Tolgo i margini e calcolo le dimensioni possibili della pagina

			_sPower.rumPage.left=_sPower.rumMargin.left;
			_sPower.rumPage.right=_sPower.sumPaper.cx-_sPower.rumMargin.right;
			_sPower.rumPage.top=_sPower.rumMargin.top;
			_sPower.rumPage.bottom=_sPower.sumPaper.cy-_sPower.rumMargin.bottom;

			_sPower.sumPage.cx=_sPower.rumPage.right-_sPower.rumPage.left;
			_sPower.sumPage.cy=_sPower.rumPage.bottom-_sPower.rumPage.top;
			
			//
			// Calcolo dell' header
			//
			if (_sPower.umHeadHeight==0)  
				{
					switch (_sPower.enLayStyle)
					{
						case PWD_LAYHIDE:
						case 1: _sPower.umHeadHeight=pwdUm(PUM_STD,110)+_sPower.umTitlePadded; break;
						case 2: _sPower.umHeadHeight=pwdUm(PUM_STD,140)+_sPower.umTitlePadded; break;
						default: break;
					}
				}
				else 
			
//			_sPower.umBodyTop=_sPower.umHeadHeight;

			if (_sPower.umFootHeight==0)   
				{
					switch (_sPower.enLayStyle)
					{
						case PWD_LAYHIDE:
						case 1: _sPower.umFootHeight=pwdUm(PUM_INCH,1); break;
						case 2: _sPower.umFootHeight=pwdUm(PUM_INCH,1); break;//pwdUm(PUM_STD,60); break;
						default: break;
					}
				}
//				else 
//				_sPower.umFootHeight=_sPower.umFootHeight);//_sPower.yCueDot=(LONG) (_sPower.sPage.cy*_sPower.fCuePerc)/100;
			
			//
			// umBodyHeight: Calcolo altezza del corpo
			//
			_sPower.umBodyHeight=_sPower.sumPage.cy-(_sPower.umHeadHeight+_sPower.umFootHeight);
			
			//
			// umBodyTop/umBodyBottom: Cordinate Top/Botto della pagina
			//
			_sPower.iVirtualLinePerPage=0; // Azzero il numero di linee per pagina
			_sPower.iVirtualLineCount=0;  // Azzero il contatorei di linee per pagina
			_sPower.umBodyTop=_sPower.rumPage.top+_sPower.umHeadHeight;
			_sPower.umBodyBottom=_sPower.rumPage.bottom-_sPower.umFootHeight;
			_sPower.umRowCursor=_sPower.umBodyTop;

			// -------------------------------------------------------------------------
			// Trasformo le dimensioni dei campi in percentuali in dimensioni fisiche
			// Calcolo quante linee ci stanno nell'area tolta la testa e la coda
			// -------------------------------------------------------------------------
			if (_sPower.umRowPadded) _sPower.iVirtualLinePerPage=(INT) (_sPower.umBodyHeight/_sPower.umRowPadded);
			_sPower.iVirtualLineCount=_sPower.iVirtualLinePerPage; // Per stampare la prima pagina porto al massimo
			
			// Se la stampa è senza campi, cioe PAGEFREE mi fermo quà
			if (_sPd.lstElement==NULL) break;

			//
			//
			//
			if (_sPower.iFieldNum) {

				for (l=0;l<_sPower.iLinePerRiga;l++)
				{
					PWD_FIELD * psFld;
					// -------------------------------------------------------------------------------------------
					// A) Calcolo il totale delle percentuali stabilite e conti i campi per linea (colonne)
					// -------------------------------------------------------------------------------------------
					iFieldPerRiga=0; iAuto=0; dTotPerc=0;
					for (lstLoop(_sPd.lstElement,psFld))
					{
						if (psFld->iLineInRow!=l) continue;
						if (psFld->xPercSize<.01F) 
								{iAuto++; psFld->xPercSize=0;} else {dTotPerc+=psFld->xPercSize;}
						iFieldPerRiga++;
					}
					if (!iFieldPerRiga) break; // Finito
				 
					 // Calcolo i campi per riga
					 _sPower.iFieldPerRiga[l]=iFieldPerRiga;
					
					 // -------------------------------------------------------------------------------------------
					 // B) Sistemo le percentuali per i campi non stabiliti                                     
					 // -------------------------------------------------------------------------------------------
					 dTotPerc=100-dTotPerc;
					 if (iAuto&&(dTotPerc>0))
					 {
						for (lstLoop(_sPd.lstElement,psFld))
						{
							//PWD_FIELD * psField=lstGet(_sPd.lstElement,a);
							if (psFld->iLineInRow!=l) continue;
							if (psFld->xPercSize==0) psFld->xPercSize=dTotPerc/iAuto;
						}
					 }

					 // -------------------------------------------------------------------------------------------
					 // C) Calcolo le percentuali di posizione e la grandezza fisica dei campi                  
					 // -------------------------------------------------------------------------------------------
					 dTotPerc=0;
					 //for (a=0;a<_sPower.iFieldNum;a++)
					 //for (psFld=lstFirst(_sPd.lstElement);psFld;psFld=lstNext(_sPd.lstElement))
					 for (lstLoop(_sPd.lstElement,psFld))
					 {
						 //PWD_FIELD * psField=lstGet(_sPd.lstElement,a);
						if (psFld->iLineInRow!=l) continue;
						psFld->xPercPos=dTotPerc;
						dTotPerc+=psFld->xPercSize;

						// Se non indicato diversamente, setto il patting del campo uguale a quello di default della tabella
						if (!psFld->bPadding) memcpy(&psFld->rumPadding,&_sPower.rumFieldPadding,sizeof(PWD_RECT));

						// Grandezza Fisica
						// x:_sPower.xPageDot=Perc:100;				
						psFld->umPosStartDot   =_sPower.rumPage.left+(_sPower.sumPage.cx*psFld->xPercPos/100);
						psFld->umPosEndDot     =_sPower.rumPage.left+(_sPower.sumPage.cx*dTotPerc/100);
						if (!_sPd.lstElement->psCurrent->psNext||psFld->umPosEndDot>_sPower.rumPage.right) 
							psFld->umPosEndDot=_sPower.rumPage.right;
						/*
						win_infoarg("Assegno: %.2f , %.2f   -   %d,%d",
									psField->xPercPos,dTotPerc,
									psField->umPosStartDot,
									psField->umPosEndDot);
									*/
					 }
				}
			}
			_sPower.umHeadBottom=(_sPower.umBodyTop-(_sPower.iLinePerRiga*_sPower.umTitleHeight)-(_sPower.rumFieldPadding.bottom+_sPower.rumFieldPadding.top)); 
			break;
	
	//-----------------------------------------------------------------------
	//	WS_LINK                         		                            |
	//											                            |
	//	Start passaggio campi della RIGA                                    |
	//  info  Long di riferimento alla linea: usato per la PostPaint()      |
	//	ptr   (non usato)					                                |
	//											                            |
	//-----------------------------------------------------------------------

		case WS_LINK:
			
			// ----------------------------------
			// Cambio di Pagina
			// Intestazione Pagina
			// ----------------------------------
			//win_infoarg("%d - %d",_sPower.iLineCount,_sPower.iLinePerRiga);

			//
			// Dovrebbe essere
			// Se la dimensione della nuova linea non sta nella pagina dovrei creare una pagina nuova
			// Per fare questo dovrei prima memorizzarmi la fine della linea per determinare
			// l'altezza dinamica e poi quindi determinare se ci sta all'interno dell'area del corpo
			//
//			win_infoarg("%d,%d",_sPower.iVirtualLineCount,_sPower.iVirtualLinePerPage);
			if (_sPower.iVirtualLineCount>=_sPower.iVirtualLinePerPage) 
			{
				//
				// Ho una pagina in sospeso (chiudo la pagina)
				//
				  if (_sPower.bPageInProgress)
				  {
					   if (_sPower.funcNotify) 
					   {
						   _(sNotify);
						   sNotify.psPower=&_sPower;
						   sNotify.enType=PDN_FOOT;
						   sNotify.rumRect.left=_sPower.rumPage.left;
						   sNotify.rumRect.top=_sPower.umBodyBottom;//>rPage.bottom-_sPower.yCueDot;
						   sNotify.rumRect.right=_sPower.rumPage.right;
						   sNotify.rumRect.bottom=_sPower.rumPage.bottom;
						   (*_sPower.funcNotify)(WS_DO,0,&sNotify);
					   }
					   _sPower.bPageInProgress=FALSE;
				  }

				  //
				  // Reset contatori - Preparo per compilazione corpo
				  //
				  _sPower.umRowCursor=_sPower.umBodyTop;
				  _sPower.iVirtualLineCount=0;
				  _sPower.iPageCount++; 
				  _sPower.bPageInProgress=TRUE;

#ifndef EH_CONSOLE
				  WinWriteSet(_sPd.idWinWait,&WScena);
				  ipt_writevedi(0,"",_sPower.iPageCount);
				  WinWriteRestore(&WScena);
#endif
				  _drawLayout();

				  // Se esiste un HookSubProc segnalo il cambio di pagina
				  if (_sPower.funcNotify)
				  {
					   sNotify.psPower=&_sPower;
					   sNotify.enType=PDN_HEAD;
					   sNotify.rumRect.left=_sPower.rumPage.left;
					   sNotify.rumRect.top=_sPower.rumPage.top;
					   sNotify.rumRect.right=_sPower.rumPage.right;
					   sNotify.rumRect.bottom=_sPower.umHeadBottom;
					   _sPower.funcNotify(WS_DO,0,&sNotify);
				 }

				   /*
				   _(LRALink);
				   LRALink.iType=LRA_FOOT;
				   LRALink.rRect.left=_sPower.rPage.left;
				   LRALink.rRect.top=_sPower.umBodyBottom;//>rPage.bottom-_sPower.yCueDot;
				   LRALink.rRect.right=_sPower.rPage.right;
				   LRALink.rRect.bottom=_sPower.rPage.bottom;
				   (*_sPower.HookSubProc)(WS_DO,0,&LRALink);
				   */

			}
			
			//if (_sPower.bRowsDynamic) {_sPower.iRowLastVirtualLine=1; _sPower.iRowLastHeight=_sPower.iRowPadded;}
			if (_sPower.bRowsDynamic) 
			{
//				_sPower.iRowLastVirtualLine=1; 
//				_sPower.umRowMaxHeight=0;
				_sPower.umRowMaxHeight=_sPower.umRowPadded;
			}
			break;
	
	//-----------------------------------------------------------------------
	//	WS_REALSET                      		                            |
	//											                            |
	//	passaggio campi della linea											|
	//  info Numero del campo a cui si fa riferimento                       |
	//	ptr  Puntatore al valore                                            |
	//		 ALFA      CHAR *                                               |
	//		 NUME      double *                                             |
	//											                            |
	//	Aggiungo al buffer di linea                                         |
	//											                            |
	//											                            |
	//-----------------------------------------------------------------------
		case WS_REALSET:
			pwdSet(info,ptr);
			break;
	
	//-----------------------------------------------------------------------
	//	WS_INSERT                       		                            |
	//											                            |
	//	Inserisco i valori caricati nel file di appoggio                    |
	//  info Non usato                                                      |
	//	ptr  Non Usato                                                      |
	//											                            |
	//											                            |
	//	flush del buffer
	//-----------------------------------------------------------------------
		case WS_INSERT:
			
			// Scrivo il buffer nel file
			_flushBuffer(true);
			
			// Se esiste un SubPaintProcedure 
			// La chiamo con l'informazione della linea
			if (_sPower.funcNotify)
			{
				_(sNotify);
				sNotify.psPower=&_sPower;
				sNotify.enType=PDN_BODY;
				sNotify.rumRect.left=_sPower.rumPage.left;
				sNotify.rumRect.top=_sPower.umRowCursor;//_sPower.umBodyTop+(_sPower.iLineCount*_sPower.ySectDot);
				sNotify.rumRect.right=_sPower.rumPage.right;
				sNotify.rumRect.bottom=sNotify.rumRect.top+_sPower.umRowPadded-1;
				(*_sPower.funcNotify)(WS_DO,0,&sNotify);
			}

			// Incremento linea virtuale e cursor in PWD_VAL per il posizionamento
			_sPower.iVirtualLineCount++;
			if (_sPower.bRowsDynamic)
			{
				//_sPower.iVirtualLineCount+=_sPower.iRowLastVirtualLine;
//				if (_sPower.umRowLastHeight>_sPower.umRowMaxHeight) _sPower.umRowMaxHeight=_sPower.umRowLastHeight;
				_sPower.umRowCursor+=_sPower.umRowMaxHeight;
			}
			else
			{
				_sPower.umRowCursor+=_sPower.umRowPadded;
			}
			_sPower.umRowMaxHeight=_sPower.umRowPadded;

			//-----------------------------------------------------------------------
			// Linea orizzontale
			//-----------------------------------------------------------------------
			if (_sPower.fLineHorzField)
			{
				pwdLine(pwdRectFill(&rumChar,_sPower.rumPage.left,_sPower.umRowCursor,_sPower.rumPage.right,_sPower.umRowCursor),
						_sPower.colLineHorz,//arField[0]->colText,
						pwdUm(PUM_PT,1),//(_sPower.iLinePerRiga>1)?2:1,
						_sPower.iLineHorzStyle);
						
			}
		
			//
			// Linee verticali
			//
			if (_sPower.fLineVertField&&_sPower.iLinePerRiga>1)
			{
				PWD_VAL umBase=_sPower.umRowCursor-_sPower.umRowHeight;

				// Linea verticale di sinistra
				rumRect.left=_sPower.rumPage.left;
				rumRect.right=_sPower.rumPage.left;
				rumRect.top=umBase;
				rumRect.bottom=_sPower.umRowCursor;

				_(sBox); sBox.sPenBrush.colPen.ehColor=0;
				_addItem(PDT_RECT,&rumRect,&sBox,sizeof(sBox));

				for (a=0;a<_sPower.iFieldNum;a++)
				{
					PWD_FIELD * psField=lstGet(_sPd.lstElement,a);	

					// Linea verticale
					rumRect.left=psField->umPosEndDot;
					rumRect.right=psField->umPosEndDot;
					rumRect.top=umBase+(psField->iLineInRow*_sPower.umRowHeight);
					rumRect.bottom=rumRect.top+(_sPower.umRowHeight*psField->iRowsSize);

					_(sBox);
					sBox.sPenBrush.colPen=_sPower.colLineVert; 
					_addItem(PDT_RECT,&rumRect,&sBox,sizeof(sBox));
					
					if (_sPower.fLineHorzField&&(psField->iLineInRow<(_sPower.iLinePerRiga-1)))
					{
						rumRect.left=psField->umPosStartDot;
						rumRect.right=psField->umPosEndDot;
						rumRect.top=rumRect.bottom=umBase+(psField->iLineInRow*_sPower.umRowHeight)+(_sPower.umRowHeight*psField->iRowsSize)-1;
						_(sBox); _addItem(PDT_RECT,&rumRect,&sBox,sizeof(sBox));
					}
				}
			}
			
			break;

	}
 
	return pRet;
}

//
// pwdColAdd()
//
PWD_FIELD * pwdColAdd(INT iType,PWD_ALIGN enAlign,double dPerc,CHAR * pszName) 
{
	PWD_FIELD	pwdField; 
	PowerDoc(WS_SETFLAG,iType,&pwdField); 
	strcpy(pwdField.szTitolo,pszName); 
	pwdField.enAlign=pwdField.enTitleAlign=enAlign;
	pwdField.xPercSize=dPerc; 
	return PowerDoc(WS_ADD,LR_ADDFIELD,&pwdField);
}

//
// pwdGetField()
//
PWD_FIELD *	pwdGetField(INT a,CHAR *pszName) {

	PWD_FIELD * psField;
	INT iCount=0;	
	for (lstLoop(_sPd.lstElement,psField)) {
		
		if (!pszName&&a==iCount) return psField;
		if (pszName&&!strCmp(psField->szTitolo,pszName)) return psField;
		iCount++;
		
	}

	return NULL;
}

//
//  pwdSet() Setta il valore di un campo 
//

void pwdSet(INT iCol,void * pValue) {

	PWD_COLOR		colText;
	PWD_FIELD *		psField;
	
	if (iCol>=_sPower.iFieldNum) ehExit("pwdSet():RealSet Field Errato %d>%d",iCol,_sPower.iFieldNum);
	psField=pwdGetField(iCol,NULL);	if (!psField) ehError();
	if (_sPower.fGColor) 
		colText=_sPower.colGlobalText;
		else 
		colText=psField->colText;
	pwdSetEx(iCol,pValue,colText,psField->colBack,_getActualStyle(),psField->enAlign);
}

void pwdSetEx(INT iCol,void * pValue,PWD_COLOR colText,PWD_COLOR colBack,EH_TSTYLE enStyles,PWD_ALIGN enAlign) {

	PWD_FIELD *		psFld;
	PWD_RECT		rumRect;
	PDO_BOXLINE		sBox;
	PWD_RECT		rumChar;
	PDO_TEXT		sText;
	double			dValore;
	CHAR * pszText=(pValue) ? pValue : NULL;
	CHAR			szServ[200],*p;

	if (iCol>=_sPower.iFieldNum) ehExit("pwdSet():RealSet Field Errato %d>%d",iCol,_sPower.iFieldNum);
	psFld=pwdGetField(iCol,NULL);			

	//
	// Controllo se lpText và pulito
	//
	pszText=strDup(strEver(pszText));
	if (psFld->fCharClean) strTrim(pszText);

	//
	// Calcola il rettangolo del campo
	//
	_(rumRect);
	rumRect.left=psFld->umPosStartDot;
	rumRect.right=psFld->umPosEndDot;
	rumRect.top=_sPower.umRowCursor;

	if (psFld->iRowsSize==LR_ROWSDYNAMIC) 
			rumRect.bottom=rumRect.top+_sPower.umRowHeight;
			else
			rumRect.bottom=rumRect.top+(_sPower.umRowHeight*psFld->iRowsSize);

	//
	// Inserisce il colore di sfondo
	//
	if (colBack.dAlpha)
	{
		sBox.sPenBrush.colPen=colBack;//.ehColor=psField->colBack;
		_addItemMem(PDT_RECT,&rumRect,&sBox,sizeof(sBox));
	}

	// ------------------------------------------------------------------
	// Stampa ad una linea
	//
	if (psFld->iRowsSize==1)
	{
		_(rumChar); _(sText);
		rumChar.left=psFld->umPosStartDot+psFld->rumPadding.left;
		rumChar.top=rumRect.top+psFld->rumPadding.top;
		rumChar.right=psFld->umPosEndDot-psFld->rumPadding.right;
		rumChar.bottom=rumChar.top+_sPower.umRowHeight;

		sText.colText=colText;
		sText.enAlign=enAlign;
		sText.enStyles=enStyles;

		switch (sText.enAlign&0xf) {
		
			case PDA_RIGHT:
				sText.umX=rumChar.right;
				sText.umY=rumChar.top;
				break;

			case PDA_CENTER:
				sText.umX=rumChar.left+((rumChar.right-rumChar.left)/2);
				sText.umY=rumChar.top;
				break;


			case PDA_LEFT:
			default:
				sText.umX=rumChar.left;
				sText.umY=rumChar.top;
				break;

		
		}
		

		switch (psFld->iTipo)
		{
			case ALFA: // Campo alfanumerico

				// Scritta
				sText.umCharHeight=_sPower.umRowHeight; if (strEmpty(pszText)) break; // Salto i campi vuoti
				sText.pszText=pszText;
				_addItemMem(PDT_TEXT,&rumChar,&sText,sizeof(sText));
				break;

			case NUME:
				if (pszText) 
				{
					dValore=* (double *) pszText;
					strcpy(szServ,Snummask(dValore,psFld->iCifre,psFld->iDec,psFld->fSep,0));
				}

				sText.umCharHeight=_sPower.umRowHeight;
				sText.fFix=psFld->fFix;
				if (_sPower.fNoDecimal) {p=strstr(szServ,","); if (p) *p=0;}
				sText.pszText=szServ;
				_addItemMem(PDT_TEXT,&rumChar,&sText,sizeof(sText));//sizeof(sText),lpText,0);
				break;
		}
		
	}
	else
	// 
	// Stampa a più linee
	//
	{
		_(sText);
		sText.bMultiline=true;
		memcpy(&rumChar,&rumRect,sizeof(PWD_RECT));
		rumChar.left+=psFld->rumPadding.left;//_sPower.rFieldMargin.left;
		rumChar.right-=psFld->rumPadding.right;
		rumChar.top+=psFld->rumPadding.top;
		rumChar.bottom-=psFld->rumPadding.bottom;
		sText.enAlign=enAlign;
		sText.enStyles=enStyles;

		sText.umInterlinea=0;
		sText.colText=colText;
		sText.umCharHeight=_sPower.umRowHeight;
		  
		//if (ptr) {if (!* (CHAR *) ptr) break;} // Salto i campi vuoti
		pszText=strEver(pszText);

		// Prima di aggiungere l'Idem :
		// A) Verifico se sto stampando in un sistema "dinamico"
		// B) Verifico se è un campo ad altezza "Rows" dinamico
		if (_sPower.bRowsDynamic)
			// &&psField->iRowsSize)
		{
			double umFieldBottom;
			INT iRows;
			PWD_VAL umTextHeight;
			
			sText.bMultiline=true;
			sText.pszText=pszText;
			umTextHeight=pwdGetTextInRectAlt(&rumChar,&sText,&iRows); // Altezza del testo in UM

// 					printf(" -> \"%s\" = %d | %.2f (%.2f)" CRLF,sText.pszText,iRows,umTextHeight,sText.umCharHeight);
			//
			// E) SI: Inserisco l'oggetto
			// _addItem(PDT_CHARBOX,&sText,sizeof(sText),lpText,0);
			//
			_addItemMem(PDT_TEXTBOX,&rumChar,&sText,sizeof(sText));

			//
			// C) Verifico se è POSSIBILE CONTENERE il campo all'interno della pagina
			//

			_sPower.umRowLastHeight=umTextHeight+(psFld->rumPadding.top+psFld->rumPadding.bottom);	// <-- Aggiungo il padding
			if (_sPower.umRowLastHeight>_sPower.umRowMaxHeight) _sPower.umRowMaxHeight=_sPower.umRowLastHeight;	// <-- Memorizzo l'altezza della riga se supero quella attuale
			if (_sPower.umRowMaxHeight==0) 
				ehError();
			umFieldBottom=_sPower.umRowCursor+_sPower.umRowMaxHeight;
			
			//
			// Case a: non ci sto nel corpo
			//
			if (umFieldBottom>_sPower.umBodyBottom)
			{
				// .1 Chiedo il cambio pagina
				PWD_VAL umRowMaxHeight=_sPower.umRowMaxHeight;
				_sPower.iVirtualLineCount=_sPower.iVirtualLinePerPage;
				PowerDoc(WS_LINK,0,NULL); 

				// .2 Rimappo tutti i componenti inseriti della linea corrente
				_LItemReposition();

				// Calcolo in che ultima linea virtuale sono
//						if (_sPower.umRowPadded>0) _sPower.iRowLastVirtualLine=(INT) (umTextHeight/_sPower.umRowPadded);

				// Ripristino i valori di altezza di linea
				_sPower.umRowMaxHeight=umRowMaxHeight;


			}
			//
			// case b: Ci sto nel corpo
			//
			else
			{
				/*
				// Se il (numero di righe del testo attuale + la riga di posizionamento) va oltre l'ultilma linea
				if ((iRows+psField->iLineInRow)>_sPower.iRowLastVirtualLine)
				{
					_sPower.iRowLastVirtualLine=(INT) (umTextHeight/_sPower.umRowPadded);//iRows+psField->iLine;
				}
				*/
			}
		}
		else
		{
			
			sText.pszText=pszText;
			_addItemMem(PDT_TEXTBOX,&rumChar,&sText,sizeof(sText));
		}
	}
	
	ehFree(pszText);
}

//
// pwdAddFont() > Aggiunge un font da usare (provvisoriamente per la stampa)
//
BOOL pwdFontAdd(CHAR * pszNameFont, UTF8 * pszFileName,BOOL bAddInFile) {
	
	PWD_FONT_RES sFont;

	if (!strEmpty(pszFileName)) {

		if (fontInstall(pszFileName,false)) return true;
//		fontAddCollection(pszFileName); // GDI+
	}
	
	_(sFont);
	strcpy(sFont.szName,pszNameFont);
	if (!strEmpty(pszFileName)) strcpy(sFont.szFileName,pszFileName);
	sFont.bAddInFile=bAddInFile;
	lstPush(_sPower.lstFontRes,&sFont);
	return false;

}


// -------------------------------------------------------------------------------
// SERVICE FUNCTION FOR PRINTER                                                  |
// -------------------------------------------------------------------------------


//
//  _pointToPixel() - Trasforma una coordinata Reale nel pixel HDC
//
static void _pointToPixel(PWD_VAL umX,
						  PWD_VAL umY,
						  double * piX,
						  double * piY) {

	double x,y;

	if (_sPd.bVirtual) {
		x=pwdUmTo(umX,PUM_DTXP); y=pwdUmTo(umY,PUM_DTXP);
		x+=_sPd.recPreviewPage.left;
		y+=_sPd.recPreviewPage.top;
	}
	else {
		x=pwdUmTo(umX,PUM_DTX); y=pwdUmTo(umY,PUM_DTY);
	}

	*piX=x;	*piY=y;

}

#ifndef EH_CONSOLE
static BOOL CALLBACK PrintDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
     {
     switch (msg)
          {
          case WM_INITDIALOG :
               EnableMenuItem (GetSystemMenu(hDlg, FALSE), SC_CLOSE,MF_GRAYED) ;
               return TRUE ;

          case WM_COMMAND :
               _sPd.bUserAbort = TRUE ;
               EnableWindow (GetParent (hDlg), TRUE) ;
               DestroyWindow (hDlg) ;
               _sPd.hDlgPrint = 0 ;
               return TRUE ;
          }
     return FALSE ;
     }          


static BOOL CALLBACK AbortProc (HDC hPrinterDC, INT iCode)
     {
     MSG msg ;

     while (!_sPd.bUserAbort && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
          {
          if (!_sPd.hDlgPrint || !IsDialogMessage (_sPd.hDlgPrint, &msg))
               {
					TranslateMessage (&msg);
					DispatchMessage (&msg);
				   
               }
          }
     return !_sPd.bUserAbort ;
     }

// -------------------------------------------------------------------------------
// PRINT CONSOLE PREVIEW                                                         |
// -------------------------------------------------------------------------------

static LRESULT CALLBACK funcPreviewProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

#define swapInt(a,b) {INT c=a; a=b; b=c;}





//
// _LPage () (Disegna una pagina, o porzione di essa
//
static void _pagePaint(HDC hDC,INT nPage,RECT * recArea)
{
	INT a;
	PWD_DRAW sItemDraw;
	RECT recComp;
	PWD_ITEM * psItem;

	_(sItemDraw);
	sItemDraw.hDC=hDC;

	for (a=_sPd.ariPageIndex[nPage];a<_sPd.nItems;a++)
	{
		psItem=_sPd.arsItem[a];
		if (psItem->iPage!=(nPage+1)) break;
		
		// Per ora cosi
		if (psItem->enType!=PDT_PATH) {

			//
			// Calcolo le nuove coordinate / dimensioni
			//
			_pointToPixel(	psItem->rumObj.left,
							psItem->rumObj.top,
							&sItemDraw.recObj.left,
							&sItemDraw.recObj.top);

			_pointToPixel(	psItem->rumObj.right,
							psItem->rumObj.bottom,
							&sItemDraw.recObj.right,
							&sItemDraw.recObj.bottom);
			
			rectToI(&recComp,&sItemDraw.recObj);
			if (recComp.left>recComp.right) swapInt(recComp.left,recComp.right);
			if (recComp.top>recComp.bottom) swapInt(recComp.top,recComp.bottom);

			if (recArea) {
			
				if (recComp.left>recArea->right||
					recComp.right<recArea->left||
					recComp.top>recArea->bottom||
					recComp.bottom<recArea->top)
							continue;
			}
		}

		//
		// Verifico che sia nell'area richiesta
		//
		
		sItemDraw.psItem=psItem; 
		//pb=(BYTE *) sItemDraw.psItem;
		//sItemDraw.pObj=(pb+sizeof(PWD_ITEM));

		//
		// Function router 
		//
		switch (psItem->enType)
		{
			case PDT_TEXT:		_drawText(&sItemDraw); break;
			case PDT_TEXTBOX:	_drawTextBox(&sItemDraw);break;
			case PDT_RECT:		_drawRect(&sItemDraw);	break;
			case PDT_LINE:		_drawLine(&sItemDraw); break;

			case PDT_EHIMG:
			case PDT_IMAGELIST:
			case PDT_BITMAP:	_drawImage(&sItemDraw);break;

			case PDT_PATH:		_drawPath(&sItemDraw); break;
	 }
  }
}

#endif

#ifndef EH_PRINT_SAPI
static void _PagePrinter(INT PageStart,INT PageEnd)
{
#ifdef EH_PDF
	if (_sPower.bPdf) {
		_pdfBuilder(PageStart,PageEnd);
		return;
	}
#else
	ehError();
#endif
}
#else
//
// _PagePrinter() - Stampa usando il device context
//
static void _PagePrinter(INT PageStart,INT PageEnd)
{
    //static DOCINFO  di = { sizeof (DOCINFO), "", NULL ,NULL,0} ;
	static DOCINFO  di;
	BOOL	bSuccess ;
	INT		iPage;
	INT		iColCopy,iNoiColCopy;

#ifndef EH_CONSOLE
	HWND hWnd;
#endif
//	PWD_SIZE sumPage;

	//
	// Non ho device context
	//
#ifdef EH_PDF
	if (_sPower.bPdf) {
		_pdfBuilder(PageStart,PageEnd);
		return;
	}
#endif

#ifndef EH_CONSOLE
	//HWND hWnd=WindowNow();
	hWnd=WindowNow();
#endif
	_sPd.bVirtual=false;

#ifndef EH_CONSOLE
	EnableWindow (WIN_info[sys.WinInputFocus].hWnd, FALSE) ;
#endif

    bSuccess   = TRUE;
    _sPd.bUserAbort = FALSE ;

#ifndef EH_CONSOLE
    _sPd.hDlgPrint = CreateDialog(sys.EhWinInstance, (LPCTSTR) "PrintDlgBox", WIN_info[sys.WinInputFocus].hWnd, PrintDlgProc);
    //SetDlgItemText (hDlgPrint, IDD_FNAME, "Stampa della pagina");//szTitleName) ;
	SetAbortProc (_sPower.hdcPrinter, AbortProc) ;
#endif

	if (!_sPower.iPageCopies) _sPower.iPageCopies=1;
	_(di);
	di.cbSize=sizeof (DOCINFO);
	if (_sPower.pszDocumentName) di.lpszDocName=_sPower.pszDocumentName; else di.lpszDocName="Ferrà srl Power Document";
	{
	   if (StartDoc(_sPower.hdcPrinter, &di) > 0)
	   {
        // Numero di copie
		for (iColCopy = 0 ;
//			 iColCopy < ((WORD) _sPower.sPrinter.Flags & PD_COLLATE ? _sPower.sPrinter.nCopies : _sPower.iCopyNumber) ;
			 iColCopy < (INT) ((_sPower.dwPrinterFlags & PD_COLLATE) ? _sPower.iPageCopies : _sPower.iCopyNumber) ;
             iColCopy++)
             {
               // Loop sulle Pagine
			   for (iPage = PageStart ; iPage <= PageEnd; iPage++)
               {
				   //printf("%d",_sPower.sPrinter.Flags & PD_COLLATE ? NumCopie : _sPower.sPrinter.nCopies); getch();
				   // Numero di copie 
				   for (iNoiColCopy = 0 ;
//                        iNoiColCopy < _sPower.iCopyNumber;//(_sPower.dwPrinterFlags & PD_COLLATE ? _sPower.iCopyNumber : _sPower.sPrinter.nCopies) ;
						iNoiColCopy < (INT) ((_sPower.dwPrinterFlags & PD_COLLATE) ? _sPower.iCopyNumber : _sPower.iPageCopies) ;
                        iNoiColCopy++)
                    {
                        // Inizio stampa della pagina
							if (StartPage (_sPower.hdcPrinter) <= 0)
                            {
								bSuccess = FALSE ;break ;
                             }

							// New 2005 / Non funziona su 98 etc...
							// Modello di colore divero da Windows
							// I tested although ICM_ON
						
							if (_sPower.lpszICCFilename)
							{
								if (!SetICMMode(_sPower.hdcPrinter, ICM_DONE_OUTSIDE_DC)) {bSuccess = FALSE; break ;}
								if (!SetICMProfile(_sPower.hdcPrinter, _sPower.lpszICCFilename)) {bSuccess = FALSE; break ;}
							}


							_pagePaint(_sPower.hdcPrinter,iPage,NULL);  

						 // Fine della pagina
							if (EndPage (_sPower.hdcPrinter) <= 0)
                              {
                               bSuccess = FALSE ;
                               break ;
                              }

						  if (_sPd.bUserAbort) break ;
					} // Loop sulle copia

                    
					if (!bSuccess || _sPd.bUserAbort) break ;
			   } // Loop sulle Pagine

               if (!bSuccess || _sPd.bUserAbort) break ;
			 } // Loop sulle Copie
          }
     else
          bSuccess = FALSE ;

     if (bSuccess) 
	 {
		 EndDoc(_sPower.hdcPrinter);
		 if (_sPower.funcNotify) {
			 PWD_NOTIFY sNotify;
			 _(sNotify);
			 sNotify.psPower=&_sPower;
			 _sPower.funcNotify(WS_DESTROY,0,&sNotify);
		 }
	 }

#ifndef EH_CONSOLE
     if (!_sPd.bUserAbort)
	 {
          EnableWindow (WIN_info[sys.WinInputFocus].hWnd, TRUE) ;
          DestroyWindow (_sPd.hDlgPrint) ;
     }
#endif

	}
#ifndef EH_CONSOLE
	EnableWindow (WIN_info[sys.WinInputFocus].hWnd, TRUE) ;
	winSetFocus (hWnd);
#endif
}

#endif


// -------------------------------------------------------------------------------
// RICALCOLA LA DIMENSIONE DEL LAYOUT                                            |
// -------------------------------------------------------------------------------
#ifndef EH_CONSOLE
static void LayResize(void)
{
	if (_sPd.wndPreview!=NULL)
	{
		MoveWindow(_sPd.wndPreview,0,_sPd.yBarra,WIN_info[_sPd.winPreview].CLx,WIN_info[_sPd.winPreview].CLy-45,TRUE);
	}
}

// -------------------------------------------------------------------------------
//
// _previewSizeCalc 
// Calcolo la dimensione della pagina visualizzata in preview
//
// -------------------------------------------------------------------------------

static void _previewSizeCalc(void)
{
	SIZE sizMaxPage;
	if (!_sPd.wndPreview) return;

	//
	// Determino le dimensioni fisiche della finestra di preview
	//
	GetClientRect(_sPd.wndPreview,&_sPd.recClient);
	sizeCalc(&_sPd.sizClient,&_sPd.recClient);
	memcpy(&sizMaxPage,&_sPd.sizClient,sizeof(SIZE));


	// Tolgo il margine
	sizMaxPage.cx-=PREVIEW_MARGIN*2;
	sizMaxPage.cy-=PREVIEW_MARGIN*2;

	// Pagina troppo piccol
	if (sizMaxPage.cx<5||sizMaxPage.cy<5) return;

	//
	// Determino la grandezza fisica della pagina visible di preview
	//
	switch (_sPd.iPageZoom)
	{
		//
		// ADATTA ALLA PAGINA
		//
		case PWZ_PAGE: // Vedi la pagina

			switch (_sPower.iOrientation)
			{
				// xPage:yPage=x:Vly;
				case DMORIENT_PORTRAIT:
					_sPd.sizPreviewPage.cy=sizMaxPage.cy;
					_sPd.sizPreviewPage.cx=(INT) (_sPower.sumPaper.cx*(double) _sPd.sizPreviewPage.cy/_sPower.sumPaper.cy);

					if (_sPd.sizPreviewPage.cx>(INT) sizMaxPage.cx)
					{
						_sPd.sizPreviewPage.cx=sizMaxPage.cx;
						_sPd.sizPreviewPage.cy=(INT) (_sPower.sumPaper.cy*(double) _sPd.sizPreviewPage.cx/_sPower.sumPaper.cx);
					}
					break;

				// VLx:xPage=y:yPage			       
				case DMORIENT_LANDSCAPE:
					_sPd.sizPreviewPage.cx=sizMaxPage.cx;
					_sPd.sizPreviewPage.cy=(INT) (_sPower.sumPaper.cy*(double) _sPd.sizPreviewPage.cx/_sPower.sumPaper.cx);
					if (_sPd.sizPreviewPage.cy>(INT) sizMaxPage.cy)
					{
						_sPd.sizPreviewPage.cy=sizMaxPage.cy;
						_sPd.sizPreviewPage.cx=(INT) (_sPower.sumPaper.cx*(double) _sPd.sizPreviewPage.cy/_sPower.sumPaper.cy);
					}
					break;
				
				default: ehExit("iLayOrientation ?");
			}
			// x:100=_sPd.sizPreviewPage.cx:_sPower.sumPaper.cx
			break;

		//
		// Largheza della pagina
		//
		case PWZ_WIDTH: 

			_sPd.sizPreviewPage.cx=sizMaxPage.cx;
			_sPd.sizPreviewPage.cy=(INT) (_sPower.sumPaper.cy*(double) _sPd.sizPreviewPage.cx/_sPower.sumPaper.cx);
			break;

		//
		// Calcolo in percentuale
		//

		default:

			// Calcolo dimensioni in percentuali sui punti fisici
//			_sPd.sizPreviewPage.cx=(INT) (pwdUm(PUM_DTX,_sPower.sumPaper.cx)*(double) _sPd.iPageZoom/100);
			_sPd.sizPreviewPage.cx=(INT) pwdUmTo((_sPower.sumPaper.cx*(double) _sPd.iPageZoom/100),PUM_DTXP);
			_sPd.sizPreviewPage.cy=(INT) (_sPower.sumPaper.cy*(double) _sPd.sizPreviewPage.cx/_sPower.sumPaper.cx);
			break;
	}

//	_sPd.dZoomPerc=(double) _sPd.sizPreviewPage.cx*100/pwdUm(PUM_DTX,_sPower.sumPaper.cx);
	_sPd.dZoomPerc=(double) _sPd.sizPreviewPage.cx*100/pwdUmTo(_sPower.sumPaper.cx,PUM_DTXP);
	ehPrintd("Zoom: [%.2f]" CRLF,_sPd.dZoomPerc);//(INT) pwdUm(PUM_DTX,_sPower.sumPaper.cx));

	// Calcolo dimensioni del workspace
	_sPd.sizWorkSpace.cx=_sPd.sizPreviewPage.cx+(PREVIEW_MARGIN*2); if (_sPd.sizWorkSpace.cx<_sPd.sizClient.cx) _sPd.sizWorkSpace.cx=_sPd.sizClient.cx;
	_sPd.sizWorkSpace.cy=_sPd.sizPreviewPage.cy+(PREVIEW_MARGIN*2); if (_sPd.sizWorkSpace.cy<_sPd.sizClient.cy) _sPd.sizWorkSpace.cy=_sPd.sizClient.cy;
	
	//if (hFontBase!=NULL) DeleteObject(hFontBase);

	//
	// Centro la pagina al workspace
	//
	_sPd.recPreviewPage.left=(_sPd.sizClient.cx-_sPd.sizPreviewPage.cx)>>1; 
	if (_sPd.recPreviewPage.left<PREVIEW_MARGIN) _sPd.recPreviewPage.left=PREVIEW_MARGIN;

	_sPd.recPreviewPage.top=(_sPd.sizClient.cy-_sPd.sizPreviewPage.cy)>>1; 
	if (_sPd.recPreviewPage.top<PREVIEW_MARGIN) _sPd.recPreviewPage.top=PREVIEW_MARGIN;

	_sPd.recPreviewPage.right=_sPd.recPreviewPage.left+_sPd.sizWorkSpace.cx-1;
	_sPd.recPreviewPage.bottom=_sPd.recPreviewPage.top+_sPd.sizWorkSpace.cy-1;

	//
	// Creo Dg
	//
	if (_sPd.psDg) {dgDestroy(_sPd.psDg); _sPd.psDg=NULL;}
	_sPd.psDg=dgCreate(0,_sPd.sizClient.cx,_sPd.sizClient.cy);
	

	//
	// Range Adjust
	//
	GetClientRect(_sPd.wndPreview,&_sPd.recClient);
	_sPd.ptWorkOffset.y=ehBarRangeAdjust(_sPd.wndPreview,SB_VERT,_sPd.ptWorkOffset.y,_sPd.sizWorkSpace.cy,_sPd.sizClient.cy);
	_sPd.ptWorkOffset.x=ehBarRangeAdjust(_sPd.wndPreview,SB_HORZ,_sPd.ptWorkOffset.x,_sPd.sizWorkSpace.cx,_sPd.sizClient.cx);

	InvalidateRect(_sPd.wndPreview,NULL,TRUE);

/*
	// Calcolo l'area di stampa (-5 pixel per parte)
	_sPd.recVPR.left=_sPd.ptPage.x+5; 
	_sPd.recVPR.top=_sPd.ptPage.y+5;
	_sPd.recVPR.right=_sPd.recVPR.left+_sPd.sizPreviewPage.cx-5;
	_sPd.recVPR.bottom=_sPd.recVPR.top+_sPd.sizPreviewPage.cy-5;
	sizeCalc(&_sPd.sizVPR,&_sPd.recVPR);
	*/
}


static void * _winPrinterPaint(EH_SUBWIN_PARAMS * psSwp)
{
//	PAINTSTRUCT *ps;
	switch (psSwp->enMess)
	{
	case WS_DISPLAY:
		//ps=(PAINTSTRUCT *) ptr;
		//	box3d(0,50,WIN_info[sys.WinWriteFocus].lx,50,2);
		break;

	case WM_SIZE:
		//if (Obj!=NULL) CalcolaLayout(TE,Obj,ON);

		LayResize();

		break;

	case WM_EXITSIZEMOVE:
		////boxp(0,50,WIN_info[sys.WinWriteFocus].lx,80,3,SET);
		//_previewSizeCalc();
		//InvalidateRect(_sPd.wndPreview,NULL,TRUE);
		_previewSizeCalc();
		InvalidateRect(_sPd.wndPreview,NULL,TRUE);
		break;

	case WS_LINK: // Libero
		break;
	}
	return NULL;
}
#endif

static void CommandCloseAnalisi(CHAR *lpParam,CHAR *pDestCommand)
{
	CHAR szServ[30],*p;
	INT iNumCopie=0;
	*pDestCommand=0;
	if (*lpParam)
	{
		strcpy(szServ,lpParam);
		p=strchr(szServ,'|');
		if (p!=NULL) // Se passo una stringa tipo "VIEW|2"
		{
			*p=0;
			strcpy(pDestCommand,szServ);
			iNumCopie=atoi(p+1);
		}
		else
		{
			if (atoi (lpParam)>0) // Se passo una stringa tipo "2"
			{
				*pDestCommand=0;
				iNumCopie=atoi (lpParam);
			}
			else // Se passo una stringa tipo "VIEW" o ""
			{
				strcpy(pDestCommand,szServ);
				iNumCopie=1;
			}
		}
	}

	if (!_sPower.iCopyNumber) _sPower.iCopyNumber=iNumCopie;
}


#ifndef EH_CONSOLE			

static void _pageShow(INT iDir) {

	INT iPage=_sPd.iPageView+iDir;
	if (iPage<0) iPage=0; 
	if (iPage>(_sPower.iPageCount-1)) iPage=_sPower.iPageCount-1; 
	if (iPage!=_sPd.iPageView) {
		_sPd.iPageView=iPage;
		InvalidateRect(_sPd.wndPreview,NULL,TRUE);
	}
}


//
// _docPreview()
//
static BOOL _docPreview(CHAR *lpParam)
{

	WNDCLASSEX wc;
//	DEVMODE *lLREvMode;   
	//  INT HdlFile;
	//  INT a,b,LPage;
	CHAR Buf[30];
	CHAR szComando[21];
	//  INT iNumCopie=0;

	CHAR *ListView[]={	"Pagina\t0",
						"Larghezza\t-1",
						"10%\t10",
						"25%\t25",
						"50%\t50",
						"100%\t100",
	//					"200%\t200",
	//					"400%\t400",
					NULL};

	struct OBJ obj[]={
	// TipObj    Nome    Sts Lck   x   y CL1 CL2 TESTO
	{O_KEYDIM,"+"      ,OFF,ON , 31, 2, 30, 21,"+"},
	{O_KEYDIM,"-"      ,OFF,ON ,  2, 2, 30, 21,"-"},
	{O_KEYDIM,"FIRST"  ,OFF,ON ,  2, 23, 75, 19,"La prima"},
	{O_KEYDIM,"LAST"   ,OFF,ON , 76, 23, 76, 19,"L'ultima"},
	{O_KEYDIM,"ESC"    ,OFF,ON ,720, 5, 74, 19,"Esci"},
	{O_KEYDIM,"PAGE"   ,OFF,ON ,153, 2,145, 19,"Stampa la Pagina"},
	{O_KEYDIM,"ALL"    ,OFF,ON ,153, 23,145, 20,"Stampa Tutto"},
	{OW_LIST ,"ZOOM"   ,OFF,ON ,302, 2,  0, 10,"101","",0,ListView},
	{O_KEYDIM,"P-"   ,OFF,ON ,383, 24, 22, 17,"-"},
	{O_KEYDIM,"P+"   ,OFF,ON ,404, 24, 22, 17,"+"},
	{STOP}
	};

	OBJS Objs[]={
		{"#4"     ,OS_TEXT ,305, 25,  0, -1,ON,SET,"SMALL F",3,"N. copie"},
		{NULL   ,STOP}};


	AUTOMOVE am[]={
		//   Type     Name    Vertice Top/Left             Vertice Bottom/Right
		{AMT_OBJ ,"ESC"   ,AMA_RIGHT ,  0,AMA_TOP,  0,AMP_FIX   ,0,AMP_FIX,0},
		{AMT_STOP}};

	struct IPT ipt[]={
		{ 1, 1,ALFA,QUAD, 67, 6, 82, 15,  0, 15,  0,  1},
		{ 1, 1,NUME,QUAD,352, 27, 28,  3,  0, 15,  0,  0,"No","No"},
		{ 0, 0,STOP}
		};

	_sPd.iPageView=0;
	_sPd.bVirtual=TRUE;
  

	_LFileTempLoad();
	CommandCloseAnalisi(lpParam,szComando);

	if (!*szComando) // Stampo tutto
	{
		mouse_graph(0,0,"CLEX"); //eventGet(NULL);
		eventGet(NULL);
		_PagePrinter(0,_sPower.iPageCount-1);
		MouseCursorDefault();
		goto FINE;
	}
  
    win_openEx(EHWP_SCREENCENTER,0,"Power Document preview",800,500,-1,OFF,0,WS_EHMOVEHIDE,FALSE,_winPrinterPaint);
	win_SizeLimits(AUTOMATIC,AUTOMATIC,-1,-1);

	_sPd.winPreview=sys.WinInputFocus;
	_sPd.wndPreviewMain=WindowNow();//WIN_info[_sPd.winPreview].hWnd;
//	lLREvMode=GlobalLock(_sPower.sPrinter.hDevMode);
//	if (!lLREvMode) ehError();

	wc.cbSize        = sizeof(wc);
	wc.style         = CS_NOCLOSE;
	wc.lpfnWndProc   = funcPreviewProcedure;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = sys.EhWinInstance;
	wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	wc.hCursor       = NULL;//LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(ColorLum(sys.ColorBackGround,-20));;//(HBRUSH) COLOR_WINDOW;//(HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = NULL;///LoadMenu;//szAppName;
	wc.lpszClassName = "EHPrintConsole";
	wc.hIconSm       = NULL;//LoadIcon(NULL,IDI_APPLICATION);
	RegisterClassEx(&wc);
					 
	_sPd.wndPreview=CreateWindow("EHPrintConsole", 
							  "",
							  WS_BORDER|WS_CHILD|WS_VSCROLL,
							  0,0,0,0,
							  WIN_info[_sPd.winPreview].hWnd, 
							  (HMENU) NULL,//100,// Menu
							  sys.EhWinInstance, 
							  NULL);

//	_sPd.iLayOrientation=_sPower.iOrientation;//
	LayResize();
	_previewSizeCalc();

	//  Carico OBJ & variazioni sui Font
	obj_open(obj); objs_open(Objs);

	obj_font("+","COURIER E",0);
	obj_font("-","COURIER E",0);
	obj_vedi();

	//  Carico IPT & font

	ipt_font("VGASYS",0);
	ipt_open(ipt);
	ipt_fontnum(0,"SMALL F",3);
	ipt_fontnum(1,"SMALL F",3);
	ipt_reset(); ipt_vedi();

	obj_AutoMoveAssign(am);
	_sPd.iPageZoom=atoi(obj_listcodget("ZOOM"));

	ShowWindow(WindowNow(),SW_MAXIMIZE);
	ShowWindow(_sPd.wndPreview,SW_NORMAL);
	MouseCursorDefault();

	ipt_writevedi(1,NULL,_sPower.iCopyNumber);
	
	// Loop di controllo EH
	while (TRUE)
	{
		EH_EVENT sEvent;
		sprintf(Buf,"%d/%d",_sPd.iPageView+1,_sPower.iPageCount);
		ipt_writevedi(0,Buf,0);
		//_sPd.iZoom=obj_listget("ZOOM");
//		_sPd.iPageZoom=atoi(obj_listcodget("ZOOM"));
		winSetFocus(_sPd.wndPreview);
		eventGetWait(&sEvent);

		if (obj_press("ZOOM")) 
		{ 
			//_sPd.iZoom=obj_listget("ZOOM");
			_sPd.iPageZoom=atoi(obj_listcodget("ZOOM"));
			_(_sPd.ptOffset);
			_previewSizeCalc();
		}

		if (key_press(ESC)||obj_press("ESCOFF")) break;

		if (obj_press("P+ON")||key_press('+')) 
		{
			_sPower.iCopyNumber++; if (_sPower.iCopyNumber>100) _sPower.iCopyNumber=100;
			ipt_writevedi(1,NULL,_sPower.iCopyNumber);
		}

		if (obj_press("P-ON")||key_press('-')) 
		{
			_sPower.iCopyNumber--;  if (_sPower.iCopyNumber<1) _sPower.iCopyNumber=1;
			ipt_writevedi(1,NULL,_sPower.iCopyNumber);
		}

		if (obj_press("+ON")||key_press2(_PGDN)) 
		{
			_pageShow(+1);
			//_sPd.iPageView++; if (_sPd.iPageView>(_sPower.iPageCount-1)) {_sPd.iPageView=_sPower.iPageCount-1; continue;}
			//InvalidateRect(_sPd.wndPreview,NULL,TRUE);
		}

		if (obj_press("-ON")||key_press2(_PGUP)) 
		{
			_pageShow(-1);
			//_sPd.iPageView--; if (_sPd.iPageView<0) {_sPd.iPageView=0; continue;}
			//InvalidateRect(_sPd.wndPreview,NULL,TRUE);
		}

		if (obj_press("FIRSTOFF")||key_press2(_HOME)) {_sPd.iPageView=0; InvalidateRect(_sPd.wndPreview,NULL,TRUE);}
		if (obj_press("LASTOFF")||key_press2(_END)) {_sPd.iPageView=_sPower.iPageCount-1; InvalidateRect(_sPd.wndPreview,NULL,TRUE);}

		if (key_press2(_FUP))  SendMessage(_sPd.wndPreview,WM_VSCROLL,SB_LINEUP,0);
		if (key_press2(_FDN))  SendMessage(_sPd.wndPreview,WM_VSCROLL,SB_LINEDOWN,0);
		if (key_press2(_FDX))  SendMessage(_sPd.wndPreview,WM_HSCROLL,SB_LINEDOWN,0);
		if (key_press2(_FSX))  SendMessage(_sPd.wndPreview,WM_HSCROLL,SB_LINEUP,0);

		if (obj_press("PAGEOFF"))
		{
			_PagePrinter(_sPd.iPageView,_sPd.iPageView);
			_sPd.bVirtual=TRUE;
		}

		if (obj_press("ALLOFF"))
		{
		   //mouse_graph(0,0,"CLEX"); 
		   MouseCursorWait();
		   eventGet(NULL);
		   _PagePrinter(0,_sPower.iPageCount-1);
		   MouseCursorDefault();
		}
  };
  
  DestroyWindow(_sPd.wndPreview);
  _sPd.wndPreview=NULL;

FINE:

//  _freeResource(FALSE);

  if (_sPd.hFontBase!=NULL) DeleteObject(_sPd.hFontBase);
#ifndef EH_CONSOLE			
  if (*szComando) win_close();
#endif
//  GlobalUnlock(_sPower.sPrinter.hDevMode);  
  return 0;
}

static void _LdcBoxDotted(HDC hdc,INT x,INT y,INT x2,INT y2,LONG Color) {

    dcLineEx(hdc,x,y,x2,y,Color,SET,PS_DOT,0);
    dcLineEx(hdc,x,y2,x2,y2,Color,SET,PS_DOT,0);
    dcLineEx(hdc,x,y,x,y2,Color,SET,PS_DOT,0);
    dcLineEx(hdc,x2,y,x2,y2,Color,SET,PS_DOT,0);

}


void _dotToUm(INT xPos,INT yPos,PWD_POINT * pumPoint) {

	xPos-=_sPd.recPreviewPage.left;
	yPos-=_sPd.recPreviewPage.top;
	pumPoint->x=pwdUm(_DTXD,xPos);
	pumPoint->y=pwdUm(_DTYD,yPos);
//	ehPrintd("(%d x %d) (%.2f,%2f)" CRLF,xPos,yPos,pumPoint->x,pumPoint->y);

}


//
// funcPreviewProcedure()
//
static LRESULT CALLBACK funcPreviewProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;
  HDC hdcDest;
  RECTD rcd;

  static LCount=0;
  static INT LastPosx;
  static INT LastPosy;
  static BOOL BarActive=FALSE;

  switch (message)
  {
		// Prima chiamata
		case WM_CREATE:  

			break;

		case WM_SIZE:
			_previewSizeCalc();
			break;

		// Tasti Funzioni e freccie
		case WM_CHAR:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:

			// 112-123 F1-F12
			// F10 Riservato
			// 16 Shift
			// 17 Ctrl
			// 18 Alt GR (No repeat)
			// 37 <--    38 su      39 -->      40 giu
			// 45 Ins    46 Canc    36 - Home   35 Fine
			// 33 Pag Up            34 Pag Down

			// Shift
			//sprintf(buf,"%d rpt:%ld  cont:%d    ",wParam,lParam&0xF,cont++);
			//Adispfm(200,0,14,0,SET,"SMALL F",2,buf);
			
			I_KeyTraslator_Windows(GetParent(hWnd),message,lParam,wParam);
			break;


//		case WM_CHAR: 
//			I_KeyTraslator_Windows(hWnd,message,lParam,wParam);
//			break;


	 case WM_MOUSEMOVE: 
		 
#if defined(TRACE_MOUSE_PREVIEW)
		 {
			INT	xPos = GET_X_LPARAM(lParam); 
			INT	yPos = GET_Y_LPARAM(lParam); 
			PWD_POINT pumPoint;
			_dotToUm(xPos,yPos,&pumPoint);
			ehPrintd("x:%.3f y:%.3f" CRLF,pumPoint.x,pumPoint.y);
		  }
#endif
		  break;

	 // Intercetto il mouse Wheel
	case WM_MOUSEWHEEL:
		{
			WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
			short zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/120;
			INT	xPos = GET_X_LPARAM(lParam); 
			INT	yPos = GET_Y_LPARAM(lParam); 

			// if (zDelta>1) dispx("%d",zDelta);
			//if (zDelta<0) MouseWheelManager(WS_ADD,IN_MW_DOWN);
			//if (zDelta>0) MouseWheelManager(WS_ADD,IN_MW_UP);
			ehPrintd("--> %d:%d (%d,%d) %d",fwKeys,zDelta,xPos,yPos,_sPd.ptOffset.y);

			//
			// Cambio di Zoom (Control)
			//
			if (fwKeys==8) {

				PWD_POINT pumMouse;
				//PWD_POINT pumOffset;
				POINT ptDelta;
				_dotToUm(xPos,yPos,&pumMouse); // Posizione del mouse

				// Calcolo l'offset di posizionamento
				// A) il punto su cui è il cursore deve rimanere posizionato
				ptDelta.x=xPos;//-_sPd.ptOffset.x;
				ptDelta.y=yPos;//-_sPd.ptOffset.y;
				// pumOffset.y=pumUm(PUM_DTXY,_sPd.ptOffset.y);

				_sPd.iPageZoom=(INT) (_sPd.dZoomPerc+((_sPd.dZoomPerc/4)*(double) zDelta));
				_previewSizeCalc();
				xPos=(INT) pwdUmTo(pumMouse.x,_DTXD);
				yPos=(INT) pwdUmTo(pumMouse.y,_DTYD);
				
				_(_sPd.ptOffset);
				_sPd.ptOffset.x=xPos-ptDelta.x;
				_sPd.ptOffset.y=yPos-ptDelta.y; 
				if (_sPd.sizClient.cx>_sPd.sizPreviewPage.cx)  _sPd.ptOffset.x=0;
				if (_sPd.sizClient.cy>_sPd.sizPreviewPage.cy)  _sPd.ptOffset.y=0;

//				
				
			}

			else if (!fwKeys) {

				//_pageShow(-zDelta);
				INT y=_sPd.ptOffset.y;
				_sPd.ptOffset.y=ehScrollTranslate(hWnd,SB_VERT,(zDelta>0)?SB_PAGEUP:SB_PAGEDOWN,_sPd.ptOffset.y,_sPd.sizWorkSpace.cy,_sPd.sizClient.cy,_sPd.sizClient.cy/9,false); // _sPd.sizWorkSpace.cy/3
				if (y==_sPd.ptOffset.y) {
					//_sPd.ptOffset.y=ehScrollTranslate(hWnd,SB_VERT,SB_TOP,_sPd.ptOffset.y,_sPd.sizWorkSpace.cy,_sPd.sizClient.cy,_sPd.sizClient.cy/9,false); // _sPd.sizWorkSpace.cy/3				
					_pageShow(-zDelta);
				}
//				_sPd.ptOffset.y=ehScrollTranslate(hWnd,SB_VERT,32,_sPd.ptOffset.y,_sPd.sizWorkSpace.cy,_sPd.sizClient.cy,zDelta*16,false); // _sPd.sizWorkSpace.cy/3
				
			}

		}
		break;

	 case WM_LBUTTONDOWN:
	 case WM_RBUTTONDOWN:
	 case WM_LBUTTONUP:
	 case WM_RBUTTONUP:
	 case WM_LBUTTONDBLCLK:
  //       if (sys.WinInputFocus<0) break;
//         WinMouseAction(sys.WinInputFocus,WIN_info[sys.WinInputFocus].hWnd,message,wParam,lParam);
		 break;
//         WinMouseAction(hWnd,message,wParam,lParam);
//		 break;

	 case WM_NCHITTEST:
		  break;

	// ----------------------------------------------- 
	// Disegno la Pagina                             |
    // ----------------------------------------------- 

	 case WM_PAINT:
			hdc=BeginPaint(hWnd,&ps);
//			hdcDest=_sPd.psDg->hdc;// hdc;
			hdcDest=hdc;

			//
			// Calcolo il posizionamento dell pagina nel workspace
			//
			_sPd.recPreviewPage.left=(_sPd.sizClient.cx-_sPd.sizPreviewPage.cx)>>1; 
			if (_sPd.recPreviewPage.left<PREVIEW_MARGIN) _sPd.recPreviewPage.left=PREVIEW_MARGIN;

			_sPd.recPreviewPage.top=(_sPd.sizClient.cy-_sPd.sizPreviewPage.cy)>>1; 
			if (_sPd.recPreviewPage.top<PREVIEW_MARGIN) _sPd.recPreviewPage.top=PREVIEW_MARGIN;

			_sPd.recPreviewPage.left-=_sPd.ptOffset.x;
			_sPd.recPreviewPage.top-=_sPd.ptOffset.y;

			_sPd.recPreviewPage.right=_sPd.recPreviewPage.left+_sPd.sizPreviewPage.cx-1;
			_sPd.recPreviewPage.bottom=_sPd.recPreviewPage.top+_sPd.sizPreviewPage.cy-1;

			//
			// Disegno la carta bianca (forse dovrei aggiungere il colore della carta ?)
			//
			rectToD(&rcd,&_sPd.recPreviewPage);
			dcBoxp(hdcDest,&_sPd.recPreviewPage,RGB(255,255,255));
			dcBox(hdcDest,&rcd,0xFF000000|0,1);

			// Mostro i margini (se ci sono)
			if ((_sPower.rumMargin.left+_sPower.rumMargin.top+_sPower.rumMargin.right+_sPower.rumMargin.bottom)>0)
			{
				_LdcBoxDotted(	hdcDest,
								(INT) pwdUmTo(_sPower.rumPage.left,_DTXD)+_sPd.recPreviewPage.left,
								(INT) pwdUmTo(_sPower.rumPage.top,_DTYD)+_sPd.recPreviewPage.top,
								(INT) pwdUmTo(_sPower.rumPage.right,_DTXD)+_sPd.recPreviewPage.left,
								(INT) pwdUmTo(_sPower.rumPage.bottom,_DTYD)+_sPd.recPreviewPage.top,
 								sys.arsColor[2]);
			}

			_pagePaint(hdcDest,_sPd.iPageView,&ps.rcPaint);
			
//			dgCopyDC(_sPd.psDg,&ps.rcPaint,0,0,hdc);
			EndPaint(hWnd,&ps);
			return 0;


/*


			_sPd.ptOffset.x=-_sPd.sPreview.iHScrollPos*_sPd.sPreview.CharX;
			_sPd.ptOffset.y=-_sPd.sPreview.iVScrollPos*_sPd.sPreview.CharY;

			if (_sPd.ariPageIndex!=NULL)
			{
				//dcBoxp(HDC psDraw->hDC,INT x1,INT y1,INT x2,INT y2,LONG col);
				dcBoxp(hdc,rectFill(&Rect,
				  _sPd.ptPage.x+_sPd.ptOffset.x,_sPd.ptPage.y+_sPd.ptOffset.y,
				  _sPd.ptPage.x+_sPd.ptOffset.x+_sPd.sizPreviewPage.cx,
				  _sPd.ptPage.y+_sPd.ptOffset.y+_sPd.sizPreviewPage.cy),RGB(255,255,255));

				dcBox(hdc,
				   rectFill(&Rect,_sPd.ptPage.x+_sPd.ptOffset.x,_sPd.ptPage.y+_sPd.ptOffset.y,_sPd.ptPage.x+_sPd.ptOffset.x+_sPd.sizPreviewPage.cx,_sPd.ptPage.y+_sPd.ptOffset.y+_sPd.sizPreviewPage.cy),
				   0,0);

				// Area stampabile
				_LdcBoxDotted(	hdc,
					_sPd.recVPR.left+_sPd.ptOffset.x,
					_sPd.recVPR.top+_sPd.ptOffset.y,
					_sPd.recVPR.right+_sPd.ptOffset.x,
					_sPd.recVPR.bottom+_sPd.ptOffset.y,sys.arsColor[2]);

				_pagePaint(TRUE,hdc,_sPd.iPageView,&ps.rcPaint);
			}
			EndPaint(hWnd,&ps);
			return 0;
*/

				/*
				// Margine superiore
				dcLineEx(hdc,
					_sPd.ptPage.x+_sPd.ptOffset.x,
					DotToVideoY(_sPower.rPage.top),
					_sPd.ptPage.x+_sPd.ptOffset.x+_sPd.sizPreviewPage.cx,
					DotToVideoY(_sPower.rPage.top),
					7,SET,PS_DOT,0);

				dcLineEx(hdc,
					_sPd.ptPage.x+_sPd.ptOffset.x,
					DotToVideoY(_sPower.rPage.bottom),
					_sPd.ptPage.x+_sPd.ptOffset.x+_sPd.sizPreviewPage.cx,
					DotToVideoY(_sPower.rPage.bottom),
					7,SET,PS_DOT,0);

				dcLineEx(hdc,
					DotToVideoX(_sPower.rPage.left),
					_sPd.ptPage.y+_sPd.ptOffset.y,
					DotToVideoX(_sPower.rPage.left),
					_sPd.ptPage.y+_sPd.ptOffset.y+_sPd.sizPreviewPage.cy,
					7,SET,PS_DOT,0);

				dcLineEx(hdc,
					DotToVideoX(_sPower.rPage.right),
					_sPd.ptPage.y+_sPd.ptOffset.y,
					DotToVideoX(_sPower.rPage.right),
					_sPd.ptPage.y+_sPd.ptOffset.y+_sPd.sizPreviewPage.cy,
					7,SET,PS_DOT,0);
					*/

	 // Ultimaa Chiamata
	 case WM_DESTROY: break;
	 case WM_COMMAND: break;
			


	 // --------------------------------------------------------------------------------
     // Controllo Scorrimento VERTICALE                                                |
     // --------------------------------------------------------------------------------
     case WM_VSCROLL:
		 _sPd.ptOffset.y=ehScrollTranslate(hWnd,SB_VERT,wParam,_sPd.ptOffset.y,_sPd.sizWorkSpace.cy,_sPd.sizClient.cy,16,false);
//		 ehPrintd("%d" CRLF,_sPd.ptOffset.y);
		 break;

	 case WM_HSCROLL:
		 _sPd.ptOffset.x=ehScrollTranslate(hWnd,SB_HORZ,wParam,_sPd.ptOffset.x,_sPd.sizWorkSpace.cx,_sPd.sizClient.cx,16,false);
//		 ehPrintd("%d" CRLF,_sPd.ptOffset.y);
		 break;



  }  // switch message

  return(DefWindowProc(hWnd, message, wParam, lParam));
// return(0L);
}  // end of WndProc()
#endif // EH_CONSOLE

static BOOL _LPrintDirect(CHAR *lpParam)
{
	CHAR szComando[21];

	_sPd.iPageView=0;
	_LFileTempLoad();
	CommandCloseAnalisi(lpParam,szComando);

	_PagePrinter(0,_sPower.iPageCount-1);
	_freeResource(TRUE);

	if (_sPd.hFontBase!=NULL) DeleteObject(_sPd.hFontBase);
	//GlobalUnlock(_sPower.sPrinter.hDevMode);  
	return 0;
}
  
//
// _LItemNext)
//
static PWD_ITEM *	_LItemNext(PWD_ITEM *psItem)
{
	BYTE *pb=(BYTE *) psItem; 
//	PWD_ITEM *psItem=(PWD_ITEM *)pb;
	if (pb==NULL) ehExit("LRE:Null");

	if (psItem->enType<PDT_ENDFILE||psItem->enType>=PDT_ERROR)
		ehError();

	if (psItem->enType==PDT_ENDFILE) 
		return NULL;

	//pt+=sizeof(PWD_ITEM); pt+=Lre->LenPostData;
	pb+=psItem->iLenItem; return (PWD_ITEM *) pb;
}

	
//
// _LFileTempLoad()
//
static void _LFileTempLoad(void)
{
	INT iLastPage,iLine,iPage;
	PWD_ITEM *psItem;
	SIZE_T tSize;
//	BYTE *pb;
	PDO_TEXT * psText;

	// --------------------------------------------------------------------
	// Creo array dei puntatori alle pagine del file caricato in memoria
	// --------------------------------------------------------------------
	_sPd.pbFileTemp=fileMemoRead(_sPd.szTempFile,&tSize); if (!_sPd.pbFileTemp) ehExit("LRE:Non memory");
	
	// Controllo gli elementi
	_sPd.nItems=0; psItem=(PWD_ITEM *) _sPd.pbFileTemp; do {_sPd.nItems++;} while (psItem=_LItemNext(psItem));
	_sPd.arsItem=ehAllocZero(sizeof (PWD_ITEM *)*(_sPd.nItems+1));
	_sPd.ariPageIndex=ehAllocZero(sizeof(INT)*(_sPower.iPageCount+1));

	//
	// Indicizzo il documento
	//
	iLastPage=-1; iLine=0; iPage=0;
	psItem=(PWD_ITEM *) _sPd.pbFileTemp;
	do
	{
		_sPd.arsItem[iLine]=psItem;

		switch (psItem->enType) {
		
			case PDT_TEXT:
			case PDT_TEXTBOX:
		
				//
				// Rimappo i puntatori al testo e font
				//
//				psText=(PDO_TEXT * ) psItem->psObj;
				psText=pwdGetObj(psItem);;
				//pb=(BYTE *) psText+sizeof(PDO_TEXT);//psText=(BYTE *) psItem+sizeof(PWD_ITEM);
				psText->pszText=(BYTE *) psText+sizeof(PDO_TEXT);
				if (psText->pszFontFace) psText->pszFontFace=psText->pszText+strlen(psText->pszText)+1;
				break;

			case PDT_PATH:
				
				break;



		}

		
		
		if (_sPd.arsItem[iLine]->iPage!=iLastPage) 
		{
			_sPd.ariPageIndex[iPage]=iLine; 
			iLastPage=_sPd.arsItem[iLine]->iPage; 
			iPage++;
		}
		iLine++;
	} while (psItem=_LItemNext(psItem));
//	_sPd.arsItem[iLine]=NULL;
}

//
// _freeResource()
//
static void _freeResource(BOOL bDeleteAll)
{
	INT a;
	PWD_ITEM *psElement;
	BYTE *ptr;

	if (bDeleteAll) 
	{
		//
		// Rimuovo il file temporane
		//
		fileRemove(_sPd.szTempFile);

		//
		// Cancello le risorse Extra
		//
		if (_sPd.arsItem) {
			for (a=0;a<_sPd.nItems;a++)
			{
				// Element = PWD_ITEM+<TAG TYPE>+<DATI>
				psElement=_sPd.arsItem[a]; 
				
				ptr=(BYTE *) psElement+sizeof(PWD_ITEM);
				switch (psElement->enType)
				{
					case PDT_EHIMG:
					case PDT_IMAGELIST:
					case PDT_BITMAP:
							{
								PDO_IMAGE *ps=(PDO_IMAGE *) ptr;
								if (ps->pszFileTemp) 
									fileRemove(ps->pszFileTemp);
							}
							break;
/*
					case PDT_PATH:
						{
							PDO_PATH * ps=(PDO_PATH *) ptr;
							pwdPathFree(ps->lstPath);
						}
						break;
						*/
				}
			}
		}

		if (!strEmpty(_sPower.pszDeviceDefine)) {
			//ReleaseDC(NULL,_sPower.hdcPrinter);
			if (_sPower.hdcPrinter) {
				DeleteDC(_sPower.hdcPrinter); 
				_sPower.hdcPrinter=NULL;
			}
			ehFreeNN(_sPower.pDevMode);
		}
#ifndef EH_CONSOLE
		if (_sPd.psDg) {dgDestroy(_sPd.psDg); _sPd.psDg=NULL;}
#endif

#ifdef EH_PDF
		ehFreePtr(&_sPd.pbCharMap);
#endif
	}

	//if (_sPd.hdlFileTemp) {memoFree(_sPd.hdlFileTemp,"LRE"); _sPd.hdlFileTemp=0;}
	ehFreePtr(&_sPd.pbFileTemp);
	if (_sPd.arsItem) {ehFree(_sPd.arsItem); _sPd.arsItem=NULL;}
	if (_sPd.ariPageIndex) {ehFree(_sPd.ariPageIndex); _sPd.ariPageIndex=NULL;}
	


}


//
// _LTagPageReplacing() New 2007 quasi 2008
// Sostituisce nelle scritte dei tag
// @#PAGE#@ numero di pagina
// @#TOTPAGE#@ numero totale di pagina
//
// 
BYTE *_LTagPageReplacing(PWD_ITEM *pElement,BYTE *pString)
{
	CHAR * lpNewString=ehAlloc((strlen(pString)*2)+20);
	BYTE szServ[80];
	strcpy(lpNewString,pString);

	sprintf(szServ,"%d",pElement->iPage);
	while (strReplace(lpNewString,"@#PAGE#@",szServ));

	sprintf(szServ,"%d",_sPower.iPageCount);
	while (strReplace(lpNewString,"@#TOTPAGE#@",szServ));
	return lpNewString;
}
//
// _LFileTempReplacing()
//
static void _LFileTempReplacing(void)
{
	PWD_ITEM *pElement;
	BYTE *ptr,*pNewString;
	INT a,iSize;
	PDO_TEXT *psText;
	BYTE *pb;
	PWD_ITEM *psNewItem;

	_LFileTempLoad();

	//
	// Riscrivo il file
	//
	_sPd.chTempFile=fopen(_sPd.szTempFile,"wb");
	for (a=0;a<_sPd.nItems;a++)
	{
		// Element = PWD_ITEM+<TAG TYPE>+<DATI>
		pElement=_sPd.arsItem[a]; ptr=(BYTE *) pElement+sizeof(PWD_ITEM);
		switch (_sPd.arsItem[a]->enType)
		{
			case PDT_TEXT: 
			case PDT_TEXTBOX: 

					pb=(BYTE *) psText=(BYTE *) _sPd.arsItem[a]+sizeof(PWD_ITEM);

					pNewString=_LTagPageReplacing(pElement,psText->pszText);
					psText->pszText=pNewString;
					psNewItem=_itemBuilder(	
												_sPd.arsItem[a]->enType,
												&_sPd.arsItem[a]->rumObj,
												psText,		// Puntatore alla struttura di dettaglio del tipo
												sizeof(PDO_TEXT),
												&iSize);
					ehFree(pNewString);
					psNewItem->iPage=pElement->iPage;
					_LTempWrite(psNewItem,iSize);
					ehFree(psNewItem);
				    break;

			default: // Gli altri tipi li trascrivo senza modifica
					_LTempWrite(pElement,pElement->iLenItem);
					break;
		}
	}
	_addItem(PDT_ENDFILE,NULL,NULL,0);
	fclose(_sPd.chTempFile); _sPd.chTempFile=NULL;
	_freeResource(FALSE);
}

static EH_TSTYLE _getActualStyle(void)
{
	EH_TSTYLE enStyle=0;
	enStyle|=_sPower.bBold?STYLE_BOLD:0;
	enStyle|=_sPower.bItalic?STYLE_ITALIC:0;
	enStyle|=_sPower.bUnderLine?STYLE_UNDERLINE:0;
	return enStyle;
}
    
static WCHAR * _strTextDecode(BYTE *pText)
{
	WCHAR *pwText;
	switch (_sPower.enCharEncode)
	{
		case SE_UTF8:
			pwText=strDecode(pText,SE_UTF8,NULL);
			break;

		case SE_ANSI:
		default:
			if (sys.bOemString) 
			{
				CHAR *pBuf=strDup(pText);
				OemToChar(pText,pBuf); 
				pwText=strToWcs(pBuf);
				ehFree(pBuf);
			}
			else 
			pwText=strToWcs(pText);
			break;
	}
	return pwText;
}	

static void	_LTempWrite(void *pb,SIZE_T iLen) {

	PWD_ITEM * psItem=pb;
	fwrite(pb, iLen, 1, _sPd.chTempFile);

}


//
// _itemBuilder                             
//  Costruisce in un nuova memoria allocata Item con i dati al seguito
//
// ATTENZIONE:Va liberata la memoria        
//                                        
//                           by Ferrà 2002/2010
//
static void * _itemBuilder(PWD_TE		uType,
						   PWD_RECT	*	prumObj,	// Puntatore all'area occupata dall'oggetto
						   void *		psObj,		// Puntatore alla struttura di dettaglio del tipo
						   INT			iObjSize,	// Dimensioni della struttura 
						   INT *		lpiSize)
						   
{
    PWD_ITEM	sItem;
	PWD_ITEM * psItem;
	BYTE *	pbMemo,*pbExtra;
	INT	iMemoSize;
	INT	iLenText;

	PDO_TEXT *	psText;
	PDO_IMAGE *	psImage;
	PDO_PATH *	psPath;
	
	//
	// Inizializzo e calcolo la lunghezza
	//
	_(sItem);
	if (prumObj) memcpy(&sItem.rumObj,prumObj,sizeof(PWD_RECT));
	sItem.iPage=_sPower.iPageCount;
    sItem.enType=uType;

	sItem.iLenItem=sizeof(sItem);
	sItem.iLenItem+=iObjSize;
	switch(uType)
	{
		case PDT_TEXT: 
		case PDT_TEXTBOX: 
			psText=psObj;
			if (psText->pszText) sItem.iLenItem+=strlen(psText->pszText)+1;
			if (psText->pszFontFace) sItem.iLenItem+=strlen(psText->pszFontFace)+1;
			break;

		case PDT_EHIMG:
		case PDT_BITMAP:
		case PDT_IMAGELIST:
			psImage=psObj;
			if (psImage->pszFileTemp) sItem.iLenItem+=strlen(psImage->pszFileTemp)+1;
			break;

		case PDT_PATH:
			psPath=psObj;
			sItem.iLenItem+=psPath->dwElements;
			break;


	}

	// PWD_ITEM+(STRUCT)+(DATI) 
	iMemoSize=sItem.iLenItem;//sizeof(sItem)+iLenStruct+iLenDati;
//	pbPoint=pbMemo=ehAlloc(iMemoSize); //if (lpMemo==NULL) ehExit("LREMAkeItem: out of memory");
	pbMemo=ehAllocZero(iMemoSize); //if (lpMemo==NULL) ehExit("LREMAkeItem: out of memory");
	psItem=(PWD_ITEM * ) pbMemo;

	//
	// Item header
	//
	memcpy(psItem,&sItem,sizeof(sItem)); 
	
//s	pbPoint+=sizeof(sItem);
	// Aggiungo struttura
	if (psObj) {
		
		BYTE * pb=pwdGetObj(psItem);
		memcpy(pb,psObj,iObjSize);
		pbExtra=(BYTE *) pb; pbExtra+=iObjSize;	
		psObj=pb;

	} else pbExtra=NULL;

	// Se ci sono accoda i dati dinamici
	switch(uType)
	{
		case PDT_TEXT: 
		case PDT_TEXTBOX: 

			psText=psObj;
			if (psText->pszText) {
				iLenText=strlen(psText->pszText)+1;
				memcpy(pbExtra,psText->pszText,iLenText); 
				psText->pszText=pbExtra;
				pbExtra+=iLenText;
			}
			if (psText->pszFontFace) {
				iLenText=strlen(psText->pszFontFace)+1;
				memcpy(pbExtra,psText->pszFontFace,iLenText); 
				psText->pszFontFace=pbExtra;
				pbExtra+=iLenText;
			}
			break;

		case PDT_EHIMG:
		case PDT_BITMAP:
		case PDT_IMAGELIST:
			psImage=psObj;
			if (psImage->pszFileTemp) {
				iLenText=strlen(psImage->pszFileTemp)+1;
				memcpy(pbExtra,psImage->pszFileTemp,iLenText); 
				pbExtra+=iLenText;
			}
			break;

		case PDT_PATH:
			psPath=psObj;
			memcpy(pbExtra,psPath->psEle,psPath->dwElements); 
			break;

	    default:
			break;

	}

	*lpiSize=iMemoSize;
 	return pbMemo;
}

//
// _addItem()  Aggiunge un idem su disco
// 
static BOOL	_addItem(PWD_TE uType,PWD_RECT *prumObj,void *psObj,INT iSizeObj) {

	INT iSize;
	PWD_ITEM * psItem;
	if (_sPower.psLastItemWrite) ehFree(_sPower.psLastItemWrite);
	psItem=_itemBuilder(uType,prumObj,psObj,iSizeObj,&iSize);

	_sPower.psLastItemWrite=psItem;
	_LTempWrite(psItem,iSize);
//	ehFree(psItem);
	return false;

}


// -------------------------------------------------------------
//
// _addItemMem()
// Gestione di un buffer di Linea
// Progettato nel 01/2002 per permettere la traslazione 
// di una linea dinamica sulla pagina successiva
//
// -------------------------------------------------------------

static BOOL	_addItemMem(PWD_TE uType,PWD_RECT *prumObj,void *psObj,INT iSizeObj) {

	INT iSize;
	BYTE *pbResult;
	pbResult=_itemBuilder(uType,prumObj,psObj,iSizeObj,&iSize);
	
	// Se è la prima volta
	if (_sPd.lItemBufferSize==0) 
		{_sPd.lItemBufferSize=2048;  // Buffer iniziale 2k
		 _sPd.lItemBufferCount=0;
		 _sPd.lpItemBuffer=ehAlloc(_sPd.lItemBufferSize);
	//	 if (_sPd.lpItemBuffer==NULL) ehExit("_addItemMem: out of memory A");
		}
	
	// Se andiamo fuori dal buffer
	if ((_sPd.lItemBufferCount+iSize)>=_sPd.lItemBufferSize)
	{
		DWORD dwOldMemo=_sPd.lItemBufferSize;
		_sPd.lItemBufferSize=_sPd.lItemBufferCount+iSize+128;
//		_sPd.lpItemBuffer=realloc(_sPd.lpItemBuffer,_sPd.lItemBufferSize);
		_sPd.lpItemBuffer=ehRealloc(_sPd.lpItemBuffer,dwOldMemo,_sPd.lItemBufferSize);
		if (_sPd.lpItemBuffer==NULL) ehExit("_addItemMem: out of memory B");
	}
	
	memcpy(_sPd.lpItemBuffer+_sPd.lItemBufferCount,pbResult,iSize);
	_sPd.lItemBufferCount+=iSize;
	ehFree(pbResult);
	return FALSE;
}

static void _flushBuffer(BOOL fOutput)
{
	if (!_sPd.lpItemBuffer) return;
	if (fOutput) {_LTempWrite(_sPd.lpItemBuffer,_sPd.lItemBufferCount);}
	ehFreePtr(&_sPd.lpItemBuffer);
	_sPd.lItemBufferSize=0;
}

//
//
//
static void _LItemReposition(void)
{
	BYTE *pt=_sPd.lpItemBuffer; 
	PWD_ITEM	*	psItem;

	if (pt==NULL) ehExit("_LItemReposition:Null");

	while (pt<(_sPd.lpItemBuffer+_sPd.lItemBufferCount))
	{
		psItem=(PWD_ITEM *) pt;

		psItem->iPage=_sPower.iPageCount; // Setta la pagina attuale
//		lpLrtChar=(PDO_CHAR *) (pt+sizeof(PWD_ITEM)); 
		switch(psItem->enType)
		{
			case PDT_TEXT:    
			case PDT_TEXTBOX:  
				psItem->rumObj.top=_sPower.umRowCursor+_sPower.rumFieldPadding.top;
				break;

			default:
			   ehExit("LRE:???");
		}
		pt+=sizeof(PWD_ITEM)+psItem->iLenItem;
	}

}


// Crea la stringa relativa alla pagina
//static CHAR *_LGetCurrentPage(EH_PWD *_sPower)
static CHAR *_LGetCurrentPage(void)
{
	static CHAR Buf[40];
	*Buf=0;
	switch (_sPower.iPagStyle)
	{
		case 0: sprintf(Buf,"Pag. %d",_sPower.iPageCount);
				break;
		case 1: sprintf(Buf,"%d",_sPower.iPageCount);
				break;
		case 2: sprintf(Buf,"%03d",_sPower.iPageCount);
				break;
	}
	return Buf;
}

//
// _drawBodyPrepare()
// Disegna Titolo e spazio per contenere le colonne
//
static void _drawBodyPrepare(void) {

	PWD_RECT rumRect;
	PWD_FIELD * psFld;
	INT a;
	for (lstLoop(_sPd.lstElement,psFld)) 
	{
		if (*psFld->szTitolo)
		{
			//
			// Stampa lo sfondo titolo corpo
			//				  
			rumRect.left=psFld->umPosStartDot; // +psFld->rumPadding.left;
			rumRect.right=psFld->umPosEndDot; // -psFld->rumPadding.right;
			if (!_sPd.lstElement->psCurrent->psNext) rumRect.right+=pwdUm(PUM_STD,1); 
			rumRect.top=_sPower.umHeadBottom;
			rumRect.bottom=_sPower.umBodyTop;
			pwdRect(&rumRect,PDC_TRASP,psFld->colTitBack,0);

//			pwdTextInRect(&rumRect,psFld->colTitText,STYLE_BOLD,psFld->enTitleAlign,_sPower.pszFontTitleDefault,_sPower.umTitleHeight,0,psFld->szTitolo,false);
			//
			//
			//
			switch (psFld->enTitleAlign) {

				case PDA_LEFT : 
					rumRect.left=psFld->umPosStartDot+psFld->rumPadding.left; // pwdUm(PUM_STD,4); 
					break;

				case PDA_RIGHT: 
					rumRect.left=psFld->umPosEndDot-psFld->rumPadding.right; // -pwdUm(PUM_STD,2); 
					break;

				case PDA_CENTER:
					rumRect.left=psFld->umPosStartDot+((psFld->umPosEndDot-psFld->umPosStartDot)/2); 
					break;

			}

			pwdText(rumRect.left,
					_sPower.umHeadBottom+psFld->rumPadding.top,
					psFld->colTitText,STYLE_BOLD,
					psFld->enTitleAlign,
					NULL,
					_sPower.umTitleHeight,
					psFld->szTitolo);
		}
	}


	// Se esite solo una linea di stampa 
	// Ed è abilitata 
	// creo la divisione delle righe
	if (_sPower.iLinePerRiga==1)
	{
		a=0;
		for (lstLoop(_sPd.lstElement,psFld),a++)			
		{
			//PWD_FIELD * psField=lstGet(_sPd.lstElement,a);

			rumRect.left=psFld->umPosStartDot;
			rumRect.right=psFld->umPosEndDot;
			rumRect.top=_sPower.umBodyTop;
			rumRect.bottom=_sPower.umBodyBottom;

			// Colore di sfondo
			if (psFld->colBack.dAlpha) pwdRect(&rumRect,PDC_TRASP,psFld->colBack,0);

			if (_sPower.fLineVertField)
			{
				PWD_RECT rumLine;
				if (!a) {

					pwdLine(pwdRectFill(&rumLine,
										rumRect.left,
										rumRect.top,
										rumRect.left,
										rumRect.bottom),
										_sPower.colLineVert,
										pwdUm(PUM_STD,1),
										PS_SOLID);

				}
				pwdLine(pwdRectFill(&rumLine,
									rumRect.right,
									rumRect.top,
									rumRect.right,
									rumRect.bottom),
									_sPower.colLineVert,
									pwdUm(PUM_STD,1),
									PS_SOLID);

			}
		}
	}

}

// 
// _drawLayout()
// 
static void _drawLayout(void)
{
	PWD_RECT rumRect;


	switch (_sPower.enLayStyle)
	{
	case PWD_LAYHIDE: // Non header
		break;

	case PWD_LAYTYPE0:

		// -------------------------------------------------------
		// LayOut Tipo 0 : Standard DOS
		// -------------------------------------------------------
		// DATA: Scrivo la data di oggi in alto a destra
		// -------------------------------------------------------
		if (_sPower.fDate) {
			pwdText(_sPower.rumPage.left,_sPower.rumPage.top,PDC_BLACK,STYLE_NORMAL,PDA_LEFT,"Arial",pwdUm(PUM_STD,40),dateFor(dateToday(),_sPower.lpDate));
		}

		// PAGINA: Scrivo il numero di pagina
		if (_sPower.fPag) {
			pwdText(_sPower.rumPage.right,_sPower.rumPage.top,PDC_BLACK,STYLE_NORMAL,PDA_RIGHT,"Tahoma",pwdUm(PUM_STD,40),_LGetCurrentPage());

		}

		// TITOLO : Scrivo il titolo centrato
		if (!strEmpty(_sPower.lpTitolo)) {
			pwdText(	_sPower.rumPage.left+_sPower.sumPage.cx/2,
						_sPower.rumPage.top,PDC_BLACK,STYLE_BOLD,PDA_CENTER,
						"Tahoma",pwdUm(PUM_STD,60),
						_sPower.lpTitolo);
		}

		// SOTTOTITOLO : Scrivo il sotto titolo centrato
		if (!strEmpty(_sPower.lpSottoTitolo))
		{
			pwdText(	_sPower.rumPage.left+_sPower.sumPage.cx/2,
						_sPower.rumPage.top+pwdUm(PUM_STD,55),
						PDC_BLACK,
						STYLE_ITALIC,PDA_CENTER,
						"Tahoma",pwdUm(PUM_STD,40),
						_sPower.lpSottoTitolo);
		}

		_drawBodyPrepare();
		break;

	case PWD_LAYTYPE1:

		// -------------------------------------------------------
		// LayOut Tipo 1 : Nuovo Tipo
		// -------------------------------------------------------
		// DATA: Scrivo la data di oggi in alto a destra
		// -------------------------------------------------------
		if (_sPower.fDate)
		{
			pwdText(	_sPower.rumPage.right,
				_sPower.rumPage.top,PDC_BLACK,
				STYLE_NORMAL,PDA_RIGHT,
				"Tahoma",pwdUm(PUM_STD,50),
				dateFor(dateToday(),_sPower.lpDate));
		}

		// PAGINA: Scrivo il numero di pagina
		if (_sPower.fPag)
		{
			pwdText(_sPower.rumPage.right,_sPower.rumPage.bottom-pwdUm(PUM_STD,40),PDC_BLACK,STYLE_NORMAL,PDA_RIGHT,"Tahoma",pwdUm(PUM_STD,40),_LGetCurrentPage());
		}

		// TITOLO : Scrivo il titolo centrato
		if (_sPower.lpTitolo)
		{
			pwdText(_sPower.rumPage.left,_sPower.rumPage.top,PDC_BLACK,STYLE_BOLD,PDA_LEFT,"Tahoma",pwdUm(PUM_STD,70),_sPower.lpTitolo);
		}

		// SOTTOTITOLO : Scrivo il sotto titolo centrato
		if (_sPower.lpSottoTitolo)
		{
			pwdText(_sPower.rumPage.left,_sPower.rumPage.top+pwdUm(PUM_STD,60),PDC_BLACK,STYLE_ITALIC,PDA_LEFT,"Tahoma",pwdUm(PUM_STD,70),_sPower.lpSottoTitolo);
		}

		//---------------------------------------------------
		// Tiro le linee di divisione
		//---------------------------------------------------


		// Linea in fondo alla pagina
		rumRect.left=_sPower.rumPage.left;
		rumRect.right=_sPower.rumPage.right;
		rumRect.top=_sPower.umHeadBottom;//-pwdUm(PUM_STD,6);
		rumRect.bottom=_sPower.umHeadBottom;
		pwdLine(&rumRect,_sPower.colLineVert,pwdUm(PUM_STD,1),PS_SOLID);

		//			  memset(&sBox,0,sizeof(sBox));
		// Al top della pagina
		rumRect.left=_sPower.rumPage.left;
		rumRect.right=_sPower.rumPage.right;
		rumRect.top=_sPower.umBodyTop;//-pwdUm(PUM_STD,3);
		rumRect.bottom=_sPower.umBodyTop;
		pwdLine(&rumRect,_sPower.colLineVert,pwdUm(PUM_STD,1),PS_SOLID);

		//
		// Linea in basso che chiude il corpo
		//
		rumRect.top=_sPower.umBodyBottom;//-pwdUm(PUM_STD,3);
		rumRect.bottom=_sPower.umBodyBottom;
		pwdLine(&rumRect,_sPower.colLineVert,pwdUm(PUM_STD,1),PS_SOLID);

		_drawBodyPrepare();
		break;
	}
}

// ------------------------------------------
// | CREA6 Builder  by Ferrà A&T 10/08/1999 |
// ------------------------------------------+


#ifndef EH_CONSOLE			
static void _LWinStart(void)
{
// Creato dal progetto : Nuovo.vpg 

  static struct IPT ipt[]={
	{ 1, 1,NUME,RIGA,105, 42, 37,  4,  0, 15,  0,  1,"No","No"},
	{ 0, 0,STOP}
	};

//  Header di attivazione
	win_open(EHWP_SCREENCENTER,42,160,75,-1,3,ON,"Attendere ...");
	_sPd.idWinWait=WIN_ult;
	ico_disp(13,32,"STAMPA");
	dispf(63,39,0,-1,0,"SMALL F",3,"Pagina");

//  Carico OBJ & variazioni sui Font
	ipt_font("SMALL F",3);
	ipt_open(ipt);
	//ipt_fontnum(0,"SMALL F",3);
	ipt_reset(); ipt_vedi();
	//eventGet(NULL);
	//eventGet(NULL);
	OsEventLoop(2);
}
#endif


//
// pwdUm() > Converte un tipo/valore in valore dotReal
//
PWD_VAL pwdUm(PWD_UM enUm,double dValore) {

	double dInch,dValue=0;

	if (_sPower.enMisure==enUm) return dValore;
	if (dValore==0) return dValore;

	// 
	switch (enUm) {

		case PUM_STD:
			dValue=pwdUm(PUM_INCH,1.0/300.0*dValore);
			break;

		case PUM_PT:
			dValue=pwdUm(PUM_INCH,1.0/72.0*dValore);
			break;

		case PUM_CM: 
		case PUM_MM:

			if (enUm==PUM_MM) dValore/=10;
			switch (_sPower.enMisure) { 

				case PUM_INCH:
				case PUM_PT:
					dValue=dValore*0.393700787; // 0.393700787
					if (_sPower.enMisure==PUM_PT) dValue*=72;
					break;
		
				case PUM_MM:
					dValue=dValore*10; // 29,591
					break;

				case PUM_STD:
					dValue=dValore*0.393700787*300; // 0.393700787
					break;

				default:
					ehError();
			}
			
			break;

		case PUM_INCH:
//		case PUM_UM_PT:

//			if (enUm==PUM_UM_PT) dValore/=72;
			switch (_sPower.enMisure) {
		
				case PUM_STD:
					dValue=dValore*300;
					break;

				case PUM_CM:
					dValue=dValore*2.54; // 29,591
					break;

				case PUM_MM:
					dValue=dValore*25.4; // 29,591
					break;

				case PUM_PT:
					dValue=dValore*72.0; // 29,591
					break;

				default:
					ehError();
			}
			
//			if (enUm==PUM_TWIN) dValue*=1440;
			break;

		//
		// Chiedo le dimensioni in UM partendo da quelle fisiche
		// Calcolo i pollici e converti in UM
		//
		
//		case PUM_DTXD_TO_UM :
		case PUM_DTX:
				dInch=(dValore*1000)/(double) _sPower.sizDotPerInch.cx;
				dValue=pwdUm(PUM_INCH,dInch)/1000;
				break;

		case PUM_DTY:
				dInch=(dValore*1000)/(double) _sPower.sizDotPerInch.cy;
				dValue=pwdUm(PUM_INCH,dInch)/1000;
				break;

		case PUM_DTHDX:
				//dInch=(dValore)/(double) _sPower.sizDotPerInch.cx;
				dValue=pwdUm(PUM_INCH,dValore/HD_DPI);
				break;

		case PUM_DTHDY:
				//dInch=(dValore)/(double) _sPower.sizDotPerInch.cy;
				dValue=pwdUm(PUM_INCH,dValore/HD_DPI);
				break;


		// Pixel del preview (to >) Um di misura powerDoc
		case PUM_DTXP:
				// dValore:_sPd.sizPreviewPage.cx=x:_sPower.sumPaper.cx;
				dValue=(_sPower.sumPaper.cx*dValore/_sPd.sizPreviewPage.cx);
				break;

		case PUM_DTYP:
				// dValore:_sPd.sizPreviewPage.cx=x:_sPower.sumPaper.cx;
				dValue=(_sPower.sumPaper.cy*dValore/_sPd.sizPreviewPage.cy);
				break;

//		case PUM_STD: // 1/300
//			break;
		default:
			ehError();
	}
	return dValue;
}

//
// pwdUmTo() UM > Unità di misura richiesta
//
double pwdUmTo(double dValore,PWD_UM enUmTo) {

	double dValue=0;

	if (_sPower.enMisure==enUmTo) return dValore;
	
	switch (enUmTo) {

		//
		// Chiedo le dimensioni "fisiche" dall'um corrente
		// Calcolo i pollici e converti in UM
		//
		case PUM_DTX:
				dValue=pwdUmTo(dValore*(double)_sPower.sizDotPerInch.cx*1000,PUM_INCH)/1000;
				break;

		case PUM_DTXP:
				dValue=((double) _sPd.sizPreviewPage.cx*dValore/_sPower.sumPaper.cx);


/*
				if (_sPd.bVirtual&&enUmTo!=PUM_DTX)//PUM_DTX) // dValore:realx=x:virtualx
					dValue=(_sPd.sizPreviewPage.cx*dValore/_sPower.sumPaper.cx);
					else
					dValue=pwdUmTo(dValore,PUM_INCH)*_sPower.sizDotPerInch.cx;
					*/
				break;
		
//		case _DTYD:
//		case PUM_DTY:

		case PUM_DTY:
			dValue=pwdUmTo(dValore*(double)_sPower.sizDotPerInch.cy*1000,PUM_INCH)/1000;
			break;


		case PUM_DTYP://PUM_DTY:
			dValue=((double) _sPd.sizPreviewPage.cy*dValore/_sPower.sumPaper.cy);
			/*
				if (_sPd.bVirtual&&enUmTo!=PUM_DTY)//PUM_DTY) // dValore:realx=x:virtualx
					dValue=(_sPd.sizPreviewPage.cy*dValore/_sPower.sumPaper.cy);
					else
					dValue=pwdUmTo(dValore,PUM_INCH)*_sPower.sizDotPerInch.cy;
					*/
			break;

		case PUM_DTHDX: // High Definition
			dValue=pwdUmTo(dValore*HD_DPI,PUM_INCH);
			break;

		case PUM_DTHDY:
			dValue=pwdUmTo(dValore*HD_DPI,PUM_INCH);
			break;

		//
		// da UM > in Inch
		//
		case PUM_INCH:
		case PUM_PT:

			switch (_sPower.enMisure) {
			
				case PUM_INCH:	dValue=dValore; break;
				case PUM_PT:	dValue=dValore/72; break;
				case PUM_STD:	dValue=dValore*300; break;
				case PUM_CM:	dValue=dValore*0.393700787; break;
				case PUM_MM:	dValue=dValore*0.039370079; break;
				default:
					ehError();
			}	
			if (enUmTo==PUM_PT)  dValue*=72;
			break;

		default:
			ehError();

	}
	return dValue;

}


PWD_POINT *	pwdPointFill(PWD_POINT * ppum,PWD_VAL x,PWD_VAL y) {
	
	ppum->x=x;
	ppum->y=y;
	return ppum;
}

PWD_SIZE *	pwdSizeFill(PWD_SIZE *psum,PWD_VAL width,PWD_VAL height) {

	psum->cx=width;
	psum->cy=height;
	return psum;

}


//
// pwdRectFill()
//
PWD_RECT * pwdRectFill(PWD_RECT *prumRect,PWD_VAL left,PWD_VAL top,PWD_VAL right,PWD_VAL bottom) {

	prumRect->left=left;
	prumRect->top=top;
	prumRect->right=right;
	prumRect->bottom=bottom;
	return prumRect;
}

void		pwdSizeCalc(PWD_SIZE *psumSize,PWD_RECT *prumRect) {
	psumSize->cx=prumRect->right-prumRect->left;
	psumSize->cy=prumRect->bottom-prumRect->top;
}




// ##########################################################################################################################
//  
//	GRUPPO DELLE FUNZIONI CHE DISEGNANO
//
// ##########################################################################################################################
//
// Visualizza una stringa pacchetizzata come PDO_TEXT
//

void * pwdGetObj(PWD_ITEM * psItem) {

	BYTE * pbObj=(BYTE * ) psItem+sizeof(PWD_ITEM);
	return pbObj;
}

static void _drawText(PWD_DRAW * psDraw)//BOOL Printer,HDC psDraw->hDC,PDO_CHAREX *Lex,CHAR *pText,RECT *lpRect)
{
	PDO_TEXT * psText=pwdGetObj(psDraw->psItem);
	BOOL bDebug=false;


	if (strEmpty(psText->pszText)) return;
	_fontPowerText(	_sPd.bVirtual?PMA_PREVIEW:PMA_PRINT,
					psDraw->hDC,
					psText, 
//					true,
					NULL);
//					_sPd.bVirtual);
/*
	if (bDebug)
	{
		RECT recQuad;
		rectToI(&recQuad,&psDraw->recObj);
		dcRect(psDraw->hDC,&recQuad,RGB(255,0,0),-1,1); // per Debug
	}
	*/
		/*
	psFac=_fontPowerCreate(psDraw->hDC,_sPd.bVirtual,psText,false,&sTM); // [font] > Stampa
	iLen=wcslen(psFac->pwcText);

	switch (psText->enAlign&0xf) {

		case PDA_LEFT: 
			uFormat|=DT_LEFT; 
			break;

		case PDA_RIGHT:
			uFormat|=DT_RIGHT; 
			break;

		case PDA_CENTER: 
			uFormat|=DT_CENTER; 
			break;
	
	}
	
	iVertical=TA_TOP;
	if (psText->enAlign&PDA_TOP) iVertical=TA_TOP;
	else if (psText->enAlign&PDA_BOTTOM) iVertical=TA_BOTTOM;
	else if (psText->enAlign&PDA_BASELINE) iVertical=TA_BASELINE;

	if (psText->bBaseLine) {
		rectCopy(sRect,psDraw->recObj);
		sRect.top-=sTM.tmAscent;
		sRect.bottom-=sTM.tmAscent;
		psRect=&sRect;
	}
	else {
		psRect=&psDraw->recObj;
	}
// 	dcRect(psDraw->hDC,&psDraw->recObj,RGB(255,0,0),-1,1); // per Debug
	
	// Stampa

	iVerticalOld=SetTextAlign(psDraw->hDC,iVertical);
	DrawTextW(	psDraw->hDC,
				psFac->pwcText,iLen,
				psRect,//&psDraw->recObj,
				uFormat);
	// _textInRect(psDraw->hDC,&psDraw->recObj,psText,psFac->pwcText,TRUE,&iRows);
	SetTextAlign(psDraw->hDC,iVerticalOld);
	_LFontAmbientDestroy(psFac);
	*/
}




/*

	// Se esiste un range
	// Controllo se il Box da fare è nel range richiesto
	if (lpRect) {

		 if (((x+SizeEx.cx-1)<lpRect->left)||
			  (x>lpRect->right)||
			  (y>lpRect->bottom)||
			  ((y+umCharHeight-1)<lpRect->top)) {} else TextOutW(psDraw->hDC,x,y, pwText,iLength);

	} 

	*/
	 


//
// _drawTextBox()
//
/*
static void _drawTextBox(PWD_DRAW * psDraw)// BOOL Printer,HDC psDraw->hDC,PDO_CHARBOX *lpLCharBox,CHAR *pbText,RECT *lpRect)
{
//	INT iRows;
	S_FAC *psFac;

//	PDO_TEXT * psText=psDraw->psObj;
	
//	psFac=_fontPowerCreate(psDraw->hDC,psText,true,NULL);
//	_textInRect(psDraw->hDC,&psDraw->recObj,psText,psFac->pwcText,TRUE,&iRows);
	_textInRect(psDraw,true);
//	_LFontAmbientDestroy(psFac);
}
*/

/*
	//
	// Calcolo le grandezze fisiche
	//
	psText=psDraw->psObj;
	iCharHeight=(INT) pwdUm(_DTYD,psText->umCharHeight);
	iCharWidth=(INT) pwdUm(_DTXD,psText->umCharWidth);
	if (iCharHeight<1) return;
	iExtraCharSpace=(INT) pwdUm(_DTXD,psText->umExtraCharSpace);

	// Trovo testo e font
	pszText=(BYTE *) psText; pszText+=sizeof(PDO_TEXT);
	if (psText->pszFontFace) {pszFontFace=pszText+strlen(pszText)+1;}
	if (!pszFontFace) pszFontFace=_sPower.pszFontBodyDefault;

	hFont=CreateFont(
					iCharHeight,	// Altezza del carattere
					iCharWidth,		// Larghezza del carattere (0=Default)
					0,				// Angolo di rotazione x 10
					0,				// Angolo di orientamento bo ???

					(psText->enStyles&STYLE_BOLD)?FW_BOLD:0,//_sPower.fBold;//sys.fFontBold;
					(psText->enStyles&STYLE_ITALIC)?1:0,//_sPower.fItalic;//sys.fFontItalic;
					(psText->enStyles&STYLE_UNDERLINE)?1:0,//_sPower.fUnderLine;//sys.fFontItalic;					 

					0, // Flag StrikeOut  ON/OFF
					DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
					//				  FERRA_OUTPRECISION, // Output precision
					OUT_DEVICE_PRECIS,//OUT_DEFAULT_PRECIS, // Output precision
					0, // Clipping Precision
					PROOF_QUALITY,//DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
					VARIABLE_PITCH,//DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
					//0,
					//"Arial"); // Nome del font
					pszFontFace); // Nome del font "Courier New"

	OldFont=SelectObject(psDraw->hDC, hFont);
	SetBkMode(psDraw->hDC,TRANSPARENT); 
	SetTextColor(psDraw->hDC,psText->colText);
	SetMapMode(psDraw->hDC, MM_TEXT);
	SetTextCharacterExtra(psDraw->hDC,iExtraCharSpace);

	//_textInRect(psDraw->hDC,psText,pszText,TRUE,&iRows);//,lpLCharBox->iMaxRows);			
	*/



//
// pwdTextInfoCreate()
// Crea una struttura di informazione delle dimensioni del testo in stampa
//
PWD_TXI * pwdTextInfoCreate(PDO_TEXT * psText,PWD_RECT * pumRect) {

	PWD_TXI *	psTi=ehNew(PWD_TXI);
	WCHAR *		pwcText,* pwBegin, * pwEnd;
	S_FAC *		psFac;
	INT			yStart;
	INT			iBreakCount;
	BOOL		bCRLF;
	BOOL		bJustify;
	INT			iStrLen;
	INT			iRowsCount=0;
	HDC			hdc;

	psTi->lstRow=lstCreate(sizeof(WCHAR *));
	psTi->psText=psText;

	hdc=CreateCompatibleDC(0);

	// Calcolo dimensioni in dot dell'area
	if (pumRect) {

		psFac=_fontPowerCreate(hdc,false,psTi->psText,true,NULL); // [font] NO PRINT

		psTi->recDot.left=(INT) pwdUmTo(pumRect->left,PUM_DTX);
		psTi->recDot.top=(INT) pwdUmTo(pumRect->top,PUM_DTY);
		psTi->recDot.right=(INT) pwdUmTo(pumRect->right,PUM_DTX);
		psTi->recDot.bottom=(INT) pwdUmTo(pumRect->bottom,PUM_DTY);
		sizeCalc(&psTi->sizDot,&psTi->recDot);
		psTi->iCharHeight=(INT) pwdUm(psText->umCharHeight,PUM_DTY);
		psTi->iInterlinea=(INT) pwdUm(psText->umInterlinea,PUM_DTY);

		//
		// Spezzo le linee in base all'allineamento
		//
		pwcText=psFac->pwcText;

		//
		// Regole
		// A) La linea và troncata con CR o LF
		// B) CR e LF devono essere tolti
		// C) Se la linea finisce con CR (ed è piu corta dello spazio, NON VIENE GIUSTIFICATA)
		//
		wcsTrim(pwcText);
		while (wcsReplace(pwcText,LCRLF,L"\r"));
		while (wcsReplace(pwcText,L"\n\r",L"\r"));

		// Non ne sono sicuro .... (se vuoto ritorno l'altezza di uno spazio
		if (wcsEmpty(pwcText)) {

			// Vuoto ?
		}

		yStart = psTi->recDot.top;//psDraw->recObj.top;
		do {                            // for each text line 

			SIZE sizPhrase;
			iBreakCount=0;
			pwBegin=pwcText;
			
			// Trova la linea piu lunga che sta nel Rect
			bCRLF=false;
			do                       // until the line is known / 
			{
				pwEnd=pwcText;
				// Cerca uno spazio (Punto di interruzione possibile
				//while (*pText!='\0'&&*pText++!=' '); // Era così
				for (;*pwcText;pwcText++)
				{
					if (*pwcText==L' ') {pwcText++; break;}
					if (*pwcText==L'\n'||*pwcText==L'\r') {pwEnd=pwcText; bCRLF=TRUE; break;}
				}

				// Fino alla fine o allo spazio
				// Avanzo; Fino alla fine della stringa o al primo spazio o al primo ritorno a capo
				// for each space, calculate extents
				iBreakCount++;
				SetTextJustification(hdc,0,0);
				GetTextExtentPoint32W(hdc, pwBegin, pwcText - pwBegin, &sizPhrase) ;
				if (!*pwcText) break;
				if (bCRLF) break; // E' un fine linea mi fermo

			} while (sizPhrase.cx<psTi->sizDot.cx) ;
			iBreakCount--; 

			while (*(pwEnd - 2)==L' ')   // eliminate trailing blanks (Elimina i blank in coda)
			{
				pwEnd--; iBreakCount--;
			}
			
			// Controllo se è l'ultima linea
			bJustify=true;
			if ((!*pwcText && sizPhrase.cx<psTi->sizDot.cx) || iBreakCount <= 0)
			{
				pwEnd=pwcText;
				if (!psText->bJustifyLast) bJustify=FALSE; // new 2007
			}
			// Controllo se finisce con un ritorno a capo
			if (bCRLF) bJustify=FALSE; // E' un fine linea mi fermo
			
			iStrLen=pwEnd-pwBegin;
			if (iStrLen>0) iRowsCount++;
			if (psText->iMaxRows&&iRowsCount>=psText->iMaxRows) break; // new 2004

			// Frase
			if (!psTi->lstRow) {

				memcpy(&psTi->sizText,&sizPhrase,sizeof(SIZE));
			
			} else {
			
				if (sizPhrase.cx>psTi->sizText.cx) psTi->sizText.cx=sizPhrase.cx;
				psTi->sizText.cy=(yStart+sizPhrase.cy)-psTi->recDot.top;

			}


			{
				WCHAR * pwc=wcsTake(pwBegin,pwEnd);
				lstPushPtr(psTi->lstRow,pwc);
			}
			


			yStart+= sizPhrase.cy + psTi->iInterlinea;
			// Se c'è un CR o LF avanzo
			while (*pwEnd==L'\n'||*pwEnd==L'\r') {pwEnd++;}
			pwcText= pwEnd;

		} while (*pwcText);// && yStart < psDraw->recObj.bottom);

		psTi->sumText.cx= pwdUm(PUM_DTX,psTi->sizText.cx);
		psTi->sumText.cy= pwdUm(PUM_DTY,psTi->sizText.cy);
		_LFontAmbientDestroy(psFac);
	}
	else {
	
		_fontPowerText(	PMA_MISURE,	// Chiedo la dimensione
						hdc,
						psText, 
//						false,
						psTi);
//						hdc,false);


	
	}
 	DeleteDC(hdc);
	
	return psTi;
}

//
// pwdTextInfoDestroy()
//
PWD_TXI * pwdTextInfoDestroy(PWD_TXI * psTi) {
	
	WCHAR * pwc;
	if (psTi->lstRow) {
		for (lstLoop(psTi->lstRow,pwc)) {
			ehFreeNN(pwc);
		}
	}

	lstDestroy(psTi->lstRow);
	
	ehFree(psTi);
	return NULL;
}

//
// _textInRect()
//
//static INT	_textInRect(HDC hdc,RECT * precObj,PDO_TEXT * psText,WCHAR * pwcText,BOOL bOutput,INT *lpiRows)
void _drawTextBox(PWD_DRAW * psDraw)
{
	INT	xStart=0, yStart=0, iBreakCount=0;
//	WCHAR *		pwBegin,*pwEnd;
	SIZE		sizText;
	SIZED		sizBox;
	BOOL		bJustify;
	INT			iRowsCount=0;
	INT			iCharHeight,iInterlinea;
	PDO_TEXT *  psText;
	
	PWD_TXI *	psTi;
	WCHAR *		pwcText;
	HDC			hdc;
	S_FAC *		psFac;
	TEXTMETRIC	sTM;
	WCHAR *		pwc;
	INT			iRow;
	INT			iBodyAlt;

	psTi=pwdTextInfoCreate(pwdGetObj(psDraw->psItem),&psDraw->psItem->rumObj);
	psText=psTi->psText;
	
	sizeCalcD(&sizBox,&psDraw->recObj);

	//
	// Calcolo le grandezze fisiche
	//
	hdc=psDraw->hDC;
	iCharHeight=(INT) pwdUmTo(psText->umCharHeight,_DTYD);
	iInterlinea=(INT) pwdUmTo(psText->umInterlinea,_DTYD);
	iBodyAlt=(INT) pwdUmTo(psTi->sumText.cy,_DTYD);
	psFac=_fontPowerCreate(psDraw->hDC,_sPd.bVirtual,psText,false,&sTM); // [draw-box]
	pwcText=psFac->pwcText;

	switch (psText->enAlign&0xf0)
	{
		case PDA_TOP:
		default:
			yStart = (INT) psDraw->recObj.top;
			break;

		case PDA_MIDDLE:
			yStart = (INT) (psDraw->recObj.top+((sizBox.cy-iBodyAlt)/2));
			break;

		case PDA_BOTTOM:
			yStart = (INT) psDraw->recObj.bottom-iBodyAlt;
			break;
			

	}

	iRow=1;
	for (lstLoop(psTi->lstRow,pwc)) {

		//UINT uFormat=DT_BOTTOM|DT_SINGLELINE|DT_NOCLIP;
		UINT uMode=TA_TOP;
		RECT recText;
		INT	iStrLen=wcslen(pwc);

		SetTextJustification(hdc, 0, 0) ;
		GetTextExtentPoint32W(hdc, pwc, iStrLen, &sizText) ;
		bJustify=true; if (iRow==psTi->lstRow->iLength) bJustify=false;

		if (bJustify&&psText->enAlign==PDA_JUSTIFY) SetTextJustification(hdc,(INT) sizBox.cx-sizText.cx,iBreakCount) ;
		memcpy(&recText,&psDraw->recObj,sizeof(RECT));
		//recText.top=yStart;
		switch (psText->enAlign&0xf)
		{
			case PDA_JUSTIFY:
			case PDA_LEFT: 
				uMode|=TA_LEFT; 
				xStart = (INT) psDraw->recObj.left; 
				break;

			case PDA_RIGHT: 
				uMode|=TA_RIGHT; 
				xStart = (INT) psDraw->recObj.right;  // ###
				break;

			case PDA_CENTER: 
				uMode|=TA_CENTER; 
				xStart = (INT) (psDraw->recObj.left+(sizBox.cx/2));//((sizBox.cx-sizText.cx)>>1); 
				break;
		}
		SetTextAlign(hdc,uMode);
		TextOutW(hdc, xStart, yStart, pwc, wcslen(pwc)) ;
		iRow++;
	
	}
/*

	//
	// Regole
	// A) La linea và troncata con CR o LF
	// B) CR e LF devono essere tolti
	// C) Se la linea finisce con CR (ed è piu corta dello spazio, NON VIENE GIUSTIFICATA)
	//
	wcsTrim(pwcText);
	while (wcsReplace(pwcText,LCRLF,L"\r"));
	while (wcsReplace(pwcText,L"\n\r",L"\r"));

	sizeCalc(&sizBox,&psDraw->recObj);

	// Non ne sono sicuro .... (se vuoto ritorno l'altezza di uno spazio
	if (wcsEmpty(pwcText)) {
	
		GetTextExtentPoint32W(hdc, L" ", 1, &sizText);
//		*lpiRows=0;
		pwdTextInfoDestroy(psTi);
//		return sizText.cy;
	}

	yStart = psDraw->recObj.top;//psDraw->recObj.top;
	do {                            // for each text line 

		iBreakCount=0;
		pwBegin=pwcText;
		
		// Trova la linea piu lunga che sta nel Rect
		bCRLF=FALSE;
		do                       // until the line is known / 
		{
			pwEnd=pwcText;
			// Cerca uno spazio (Punto di interruzione possibile
			//while (*pText!='\0'&&*pText++!=' '); // Era così
			for (;*pwcText;pwcText++)
			{
				if (*pwcText==L' ') {pwcText++; break;}
				if (*pwcText==L'\n'||*pwcText==L'\r') {pwEnd=pwcText; bCRLF=TRUE; break;}
			}

			// Fino alla fine o allo spazio
			// Avanzo; Fino alla fine della stringa o al primo spazio o al primo ritorno a capo
			// for each space, calculate extents
			iBreakCount++;
			SetTextJustification(hdc,0,0);
			GetTextExtentPoint32W(hdc, pwBegin, pwcText - pwBegin, &sizText) ;
			if (!*pwcText) break;
			if (bCRLF) break; // E' un fine linea mi fermo

		} while (sizText.cx<sizBox.cx) ;
		iBreakCount--; 

		while (*(pwEnd - 2)==L' ')   // eliminate trailing blanks (Elimina i blank in coda)
		{
			pwEnd--; iBreakCount--;
		}
		
		// Controllo se è l'ultima linea
		bJustify=TRUE;
		if ((!*pwcText && sizText.cx<sizBox.cx) || iBreakCount <= 0)
		{
			pwEnd=pwcText;
			if (!psText->bJustifyLast) bJustify=FALSE; // new 2007
		}
		// Controllo se finisce con un ritorno a capo
		if (bCRLF) bJustify=FALSE; // E' un fine linea mi fermo
		
		iStrLen=pwEnd-pwBegin;
		if (iStrLen>0) iRowsCount++;
		if (psText->iMaxRows&&iRowsCount>=psText->iMaxRows) break; // new 2004

		// Stampa dell'oggetto
//		if (bOutput) 
		{
			//UINT uFormat=DT_BOTTOM|DT_SINGLELINE|DT_NOCLIP;
			UINT uMode=TA_TOP;
			RECT recText;

			SetTextJustification(hdc, 0, 0) ;
			GetTextExtentPoint32W(hdc, pwBegin, iStrLen, &sizText) ;
			if (bJustify&&psText->enAlign==PDA_JUSTIFY) SetTextJustification(hdc,sizBox.cx-sizText.cx,iBreakCount) ;
			memcpy(&recText,&psDraw->recObj,sizeof(RECT));
			//recText.top=yStart;
			switch (psText->enAlign&0xf)
			{
				case PDA_JUSTIFY:
				case PDA_LEFT: 
					uMode|=TA_LEFT; 
					xStart = psDraw->recObj.left; 
					break;

				case PDA_RIGHT: 
					uMode|=TA_RIGHT; 
					xStart = psDraw->recObj.right;  // ###
					break;

				case PDA_CENTER: 
					uMode|=TA_CENTER; 
					xStart = psDraw->recObj.left+(sizBox.cx>>1);//((sizBox.cx-sizText.cx)>>1); 
					break;
			}
			//DrawTextW(	hdc, pwBegin, iStrLen, &recText, uFormat); 
			SetTextAlign(hdc,uMode);
			 TextOutW(hdc, xStart, yStart, pwBegin, iStrLen) ;
		}
	
		yStart+= sizText.cy + iInterlinea;
		// Se c'è un CR o LF avanzo
		while (*pwEnd==L'\n'||*pwEnd==L'\r') {pwEnd++;}
		pwcText= pwEnd;

	} while (*pwcText);// && yStart < psDraw->recObj.bottom);

	SetTextJustification(hdc, 0, 0);
	*/
//	if (lpiRows) *lpiRows=iRowsCount;
//	return (yStart-precObj->top);
	
}


//
// _drawRect() //BOOL Printer,HDC psDraw->hDC,INT iType,PDO_BOXR *Box,RECT *lpRect)
//
static void _drawRect(PWD_DRAW *psDraw) {

	INT		iPenWidth;
	SIZE	sizRound;
	HPEN	hPen=0,hPenOld;
	HBRUSH	hBrush,hBrushOld;
	PDO_BOXLINE *psBox=pwdGetObj(psDraw->psItem);
	BOOL	bBrushFree;
	RECT	recObjInt;


	iPenWidth=(INT) pwdUmTo(psBox->sPenBrush.umPenWidth,_DTXD); //Box->iPenWidth;
	sizRound.cx=(INT) pwdUmTo(psBox->sumRound.cx,_DTXD); 
	sizRound.cy=(INT) pwdUmTo(psBox->sumRound.cy,_DTYD); 

	rectToI(&recObjInt,&psDraw->recObj);
	
	//
	// Seleziono la penna (se ho colore e spessore
	//
	if (!psBox->sPenBrush.colPen.dAlpha&&psBox->sPenBrush.umPenWidth)
	{
		hPen=CreatePen(psBox->sPenBrush.iPenStyle, iPenWidth, psBox->sPenBrush.colPen.ehColor); // era 1
	}
	else {
		hPen=CreatePen(psBox->sPenBrush.iPenStyle, iPenWidth, psBox->sPenBrush.colPen.ehColor); // era 1
	}

	//
	// Seleziono il pennellone!
	//
//	if (psBox->sPenBrush.colBrush!=EH_COLOR_TRASPARENT) 	{
	if (psBox->sPenBrush.colBrush.dAlpha) 	{
		if (!psBox->sPenBrush.iBrushStyle)
			hBrush=CreateSolidBrush(psBox->sPenBrush.colBrush.ehColor); // era 1
			else
			hBrush=CreateHatchBrush(psBox->sPenBrush.iBrushStyle,psBox->sPenBrush.colBrush.ehColor); // era 1
		bBrushFree=TRUE;
	}
	else {
		hBrush=GetStockObject(NULL_BRUSH);
		bBrushFree=FALSE;
	}

	hPenOld		= SelectObject(psDraw->hDC, hPen);
	hBrushOld	= SelectObject(psDraw->hDC, hBrush);

	if (sizRound.cx||sizRound.cy) 
		RoundRect(psDraw->hDC,recObjInt.left,recObjInt.top,recObjInt.right,recObjInt.bottom,sizRound.cx,sizRound.cy);
		else
		Rectangle(psDraw->hDC,recObjInt.left,recObjInt.top,recObjInt.right,recObjInt.bottom);

	/*
	SelectObject(psDraw->hDC, hOldPen);
	SelectObject(psDraw->hDC, hbrOld);
*/
	if (hPen)	DeleteObject(hPen);		// Cancella la penna usata
	if (bBrushFree) DeleteObject(hBrush);	// Cancella il pennello usato
}


//
// _drawLine() 
//
static void _drawLine(PWD_DRAW *psDraw) {

	HPEN hPen, hOldPen;
	LOGBRUSH sLogBrush;
	INT iPenWidth;
	PDO_BOXLINE *psBox=pwdGetObj(psDraw->psItem);

	//iPenWidth=(INT) pwdUm(_DTXD,psBox->sPenBrush.umPenWidth); 
	iPenWidth=(INT) pwdUmTo(psBox->sPenBrush.umPenWidth,_DTXD); //Box->iPenWidth;

	if (iPenWidth>1)
	{
		_(sLogBrush);
		sLogBrush.lbStyle=BS_SOLID;
		sLogBrush.lbColor=psBox->sPenBrush.colPen.ehColor;
		sLogBrush.lbHatch=HS_BDIAGONAL;
		hPen = ExtCreatePen(PS_GEOMETRIC|psBox->sPenBrush.iPenStyle|PS_ENDCAP_ROUND, iPenWidth, &sLogBrush,psBox->sPenBrush.colPen.ehColor,NULL); // era 1
	}
	else
	{
		hPen = CreatePen(psBox->sPenBrush.iPenStyle, iPenWidth, psBox->sPenBrush.colPen.ehColor); // era 1
	}
	hOldPen = SelectObject(psDraw->hDC, hPen);
	MoveToEx(psDraw->hDC,(INT) psDraw->recObj.left,(INT) psDraw->recObj.top,NULL); // Cambiare NULL
	LineTo(psDraw->hDC,(INT) psDraw->recObj.right,(INT) psDraw->recObj.bottom); 

	DeleteObject(hPen); // Cancella la penna usata
}


#ifndef EH_CONSOLE


//
// _drawPath() 
//
static void _drawPath(PWD_DRAW * psDraw) {

	PDO_PATH * psPath=pwdGetObj(psDraw->psItem);
	PWD_PTHE * psEle;
	INT		iPenWidth;
	HPEN	hPen=0,hPenOld;
	HBRUSH	hBrush,hBrushOld;
	BOOL	bBrushFree=false;
	BOOL	bPenFree=false;
	EH_LST  lstPath;

	iPenWidth=(INT) pwdUmTo(psPath->sPenBrush.umPenWidth,_DTXD); //Box->iPenWidth;

	//
	// Seleziono la penna (se ho colore e spessore
	//
	if (!psPath->sPenBrush.colPen.dAlpha||!psPath->sPenBrush.umPenWidth)
	{
		//hPen=CreatePen(psPath->sPenBrush.iPenStyle, iPenWidth, psPath->sPenBrush.colPen.ehColor); // era 1
		hPen=GetStockObject(NULL_PEN);
		bPenFree=false;
	}
	else {

		hPen=CreatePen(psPath->sPenBrush.iPenStyle, iPenWidth, psPath->sPenBrush.colPen.ehColor); // era 1
		bPenFree=true;
	}

	//
	// Seleziono il pennellone!
	//
	if (psPath->sPenBrush.colBrush.dAlpha) 	{
		if (!psPath->sPenBrush.iBrushStyle)
			hBrush=CreateSolidBrush(psPath->sPenBrush.colBrush.ehColor); // era 1
			else
			hBrush=CreateHatchBrush(psPath->sPenBrush.iBrushStyle,psPath->sPenBrush.colBrush.ehColor); // era 1
		bBrushFree=true;
	}
	else {
		hBrush=GetStockObject(NULL_BRUSH);
		bBrushFree=false;
	}

	hPenOld		= SelectObject(psDraw->hDC, hPen);
	hBrushOld	= SelectObject(psDraw->hDC, hBrush);

	// Traccio e disegno il percorso
	BeginPath(psDraw->hDC);
	lstPath=_pathCreate(psPath);
	for (lstLoop(lstPath,psEle)) {
	
		//INT iX,iY;
		double dx,dy;
		PWD_POINT * pumPoint;
		pumPoint=psEle->psData;
	
		switch (psEle->enType) {
		
			case PHT_MOVETO:
				_pointToPixel(	pumPoint->x,
								pumPoint->y,
								&dx,
								&dy);
				MoveToEx(psDraw->hDC,(INT) dx,(INT) dy,NULL); // Cambiare NULL
				break;

			case PHT_LINETO:
				_pointToPixel(	pumPoint->x,
								pumPoint->y,
								&dx,
								&dy);
				LineTo(psDraw->hDC,(INT) dx,(INT) dy); // Cambiare NULL
				break;

			case PHT_POLYBEZIERTO:
				{
					PWD_BEZIER * pumBezier=psEle->psData;
					INT a;
					POINT * arsPoint=ehAlloc(pumBezier->cCount*sizeof(POINT));
					for (a=0;a<(INT) pumBezier->cCount;a++) {

						_pointToPixel(	pumBezier->arsPoint[a].x,
										pumBezier->arsPoint[a].y,
										&dx,&dy);
						arsPoint[a].x=(INT) dx;
						arsPoint[a].y=(INT) dy;

					}
					PolyBezierTo(psDraw->hDC,arsPoint,pumBezier->cCount);
					ehFree(arsPoint);
				}
				break;

			case PHT_CLOSEFIGURE:
				CloseFigure(psDraw->hDC);
				break;

			default:
				ehError();
		}

	}
	_pathDestroy(lstPath);
	EndPath(psDraw->hDC);

	if (bBrushFree&&bPenFree)
		StrokeAndFillPath(psDraw->hDC);
	else if (bPenFree)
		StrokePath(psDraw->hDC);
	else if (bBrushFree)
		FillPath(psDraw->hDC);

	if (bPenFree)	DeleteObject(hPen);		// Cancella la penna usata
	if (bBrushFree) DeleteObject(hBrush);	// Cancella il pennello usato

}

//
// _drawImage() - BOOL Printer,HDC psDraw->hDC,INT iType,PDO_BOX *Box,RECT *lpRect)
//
static void _drawImage(PWD_DRAW *psDraw) //BOOL Printer,HDC psDraw->hDC,INT iType,PDO_BOX *Box,RECT *lpRect)
{
//	INT x,y,xDim,yDim;
	PDO_IMAGE	*psImage=pwdGetObj(psDraw->psItem);
	SIZE		sizImage;
	IMAGEINFO	sImageInfo;
	RECT		recObjInt;

	rectToI(&recObjInt,&psDraw->recObj);
	sizeCalc(&sizImage,&recObjInt);

	switch (psDraw->psItem->enType)
	{
		//
		//	Stampo immagine EasyHand
		//
		case PDT_EHIMG:  
				if (psImage->hImage) 
					_ImgOutEx(psDraw->hDC, recObjInt.left, recObjInt.top, sizImage.cx, sizImage.cy,psImage->hImage);
				else
				{
					//
					// Leggo da disco l'immagine (Versione Windows)
					//
					HBITMAP hBitmap;

					// hBitmap=BMPLoadBitmap(psImage->pszFileTemp);
					hBitmap=LoadImage(NULL,psImage->pszFileTemp,IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
					if (!hBitmap) ehExit("[%s] ?",psImage->pszFileTemp);
					dcBmpDisp(	psDraw->hDC,
								recObjInt.left, recObjInt.top,
								sizImage.cx, sizImage.cy,
								hBitmap,0);
					DeleteObject(hBitmap);

				}
				break;

		case PDT_IMAGELIST:
				// Va rivisto come quello sotto
			if (ImageList_GetImageInfo(psImage->himl,psImage->hImage,&sImageInfo))
				{
					INT hdlImage=BitmapToImg(sImageInfo.hbmImage,TRUE,&sImageInfo.rcImage,0);
					_ImgOutEx(psDraw->hDC,recObjInt.left, recObjInt.top, sizImage.cx, sizImage.cy,hdlImage);
					memoFree(hdlImage,"memo");
				}

		case PDT_BITMAP:
				dcBmpDisp(	psDraw->hDC,
							recObjInt.left, recObjInt.top,
							sizImage.cx, sizImage.cy,
							psImage->hBitmap,0);
				break;
	}
}



static void _ImgOutEx(	HDC hDC,
						INT PosX,INT PosY,
						INT SizeX,INT SizeY,
						INT hImage)
{
	IMGHEADER *psImg;
	WORD    a=1;
	BITMAPINFOHEADER *psBmpHeader;
	BITMAPINFO *psBmpInfo;
//	INT TileX=256;// Dimensione piastrella orizzontale (Da Fare)
//	INT TileY=0;

	LONG Lx,Ly;
	INT hImageNew;
 
	psImg=memoLock(hImage);
	psBmpInfo=(BITMAPINFO *) &psImg->bmiHeader;
	psBmpHeader=(BITMAPINFOHEADER *) &psImg->bmiHeader;

	// -------------------------------------
	// CALCOLO LE DIMENSIONI DEL NUOVO BITMAP
	// -------------------------------------

	Ly=psBmpHeader->biHeight; Lx=psBmpHeader->biWidth;

	// Calcolo della piastrella
	// Inversamente proporzionale : y:1024=TileY:TileX;
	//if (Lx>TileX) {TileX=Lx; TileY=TileY*1024/TileX;}

	// Calcolo automatico delle dimensioni orizzontali
	//  SizeY:Ly=x:Lx;
	if ((SizeY>0)&&(SizeX==0)) SizeX=SizeY*Lx/Ly;
	// Calcolo automatico delle dimensioni verticali
	if ((SizeX>0)&&(SizeY==0)) SizeY=SizeX*Ly/Lx;
	memoUnlock(hImage);
	if (SizeX<1||SizeY<1) return;

	/*
	//
	// Stampa file diretto
	//
	if (psImg->enFilePixelType==IMG_PIXEL_RGB_ALPHA&&
		psImg->enType==IMG_PNG&&
		!strEmpty(psImg->utfFileName)) {
	
			INT iRet;

			BITMAPINFO bmi;
			ULONG	ul = CHECKJPEGFORMAT;


			if (!psImg->pbOriginal) {
				SIZE_T sizData;
				BYTE * pb;
				pb=psImg->pbOriginal=fileMemoRead(psImg->utfFullFileName,&sizData);
				if (!psImg->pbOriginal) return;


				if (
					// Check if CHECKJPEGFORMAT exists: 
					(ExtEscape(hDC, QUERYESCSUPPORT,
							   sizeof(ul), (CHAR *) &ul, 0, 0) > 0) &&

					// Check if CHECKJPEGFORMAT executed without error: 

					(ExtEscape(hDC, CHECKPNGFORMAT,
							   psImg->lFileSize, psImg->pbOriginal, sizeof(ul), &ul) > 0) &&

					// Check status code returned by CHECKJPEGFORMAT: 

					(ul == 1)
				   ) 
				{
					printf("qui");
				}

			}
			_(bmi);
			bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth       = Lx;
			bmi.bmiHeader.biHeight      = -Ly; // top-down image 
			bmi.bmiHeader.biPlanes      = 1;
			bmi.bmiHeader.biBitCount    = 0;
			bmi.bmiHeader.biCompression = BI_PNG;
			bmi.bmiHeader.biSizeImage   = psImg->lFileSize;
			iRet = StretchDIBits(hDC,
								 // destination rectangle 
								 PosX, PosY, SizeX, SizeY,
								 // source rectangle 
								 0, 0, Lx, Ly,
								 psImg->pbOriginal,
								 &bmi,
								 DIB_RGB_COLORS,
								 SRCCOPY);
			printf("qui");
	}
	else {
	// Non trasparente vado
*/	
	{
	//
	// Creo il nuovo bitmap
	//
		if (SizeY>4) {
			hImageNew=IMGResampling(	hImage,  // Handle dell'immagine
										NULL,
										SizeX,SizeY,
										TRS_LANCZOS);
			//HdlImageNew=IMGRemaker(hdlImage,NULL,SizeX,SizeY,TRUE,TRS_NONE);
			if (hImageNew<1) ehExit("_ImgOutEx():Non memory");

			dcImageShow(hDC,PosX,PosY,0,0,hImageNew);
			memoFree(hImageNew,"LREPIMG");
		}

	}


 	
	
}
#endif

//
// ##########################################################################################################################
//  
//	API pwd PER IL DISEGNO DELLA PAGINA
//
// ##########################################################################################################################

static PWD_VAL _colCtrl(PWD_VAL col) {

	if (col<0) col=0;
	if (col>1.0) col=1.0;
	return col;
}

//
// pwdColor()
//
PWD_COLOR pwdColor(EH_COLOR ehCol) {

	PWD_COLOR sCol;
	_(sCol);
	sCol.enFormat=PWC_RGB;
	if (ehCol==EH_COLOR_TRASPARENT) sCol.dAlpha=0; else 
	{
		sCol.dAlpha=1.0;
		sCol.ehColor=ehCol;
		sCol.dComp[2]=colorReal((ehCol>>16)&0xFF);
		sCol.dComp[1]=colorReal((ehCol>>8)&0xFF);
		sCol.dComp[0]=colorReal(ehCol&0xFF);
	}
	return sCol;

}

//
// pwdRGB()
//
PWD_COLOR pwdRGB(PWD_VAL dRed,PWD_VAL dGreen,PWD_VAL dBlue,PWD_VAL dAlpha) {

	PWD_COLOR sCol;
	_(sCol);
	sCol.enFormat=PWC_RGB;
	sCol.dAlpha=dAlpha;
	sCol.dComp[0]=_colCtrl(dRed); 
	sCol.dComp[1]=_colCtrl(dGreen);
	sCol.dComp[2]=_colCtrl(dBlue);

	// x:255=val:1 (BGR)
	sCol.ehColor= ((INT) (sCol.dComp[0]*255)) | ((INT) (sCol.dComp[1]*255)<<8)|(((INT) (sCol.dComp[2]*255))<<16);
	return sCol;

}

//
// pwdCMYK()
//
PWD_COLOR pwdCMYK(PWD_VAL dCyan,PWD_VAL dMag,PWD_VAL dYel,PWD_VAL dKey,PWD_VAL dAlpha) {

	PWD_COLOR sCol;
	DWORD dwCMYK,dw;
	_(sCol);
	sCol.enFormat=PWC_CMYK;
	sCol.dAlpha=dAlpha;
	sCol.dComp[0]=_colCtrl(dCyan);
	sCol.dComp[1]=_colCtrl(dMag);
	sCol.dComp[2]=_colCtrl(dYel);
	sCol.dComp[3]=_colCtrl(dKey);
	
	dwCMYK= ((INT) (sCol.dComp[0]*255)<<24) | ((INT) (sCol.dComp[1]*255)<<16) | ((INT) (sCol.dComp[2]*255)<<8)| (INT) (sCol.dComp[3]*255);
	dw=colorCMYKtoRGB(dwCMYK);
	sCol.ehColor=((dw>>16)&0xff) | (dw&0x00ff00) | ((dw&0xff)<<16); // RGB > BGR

	return sCol;

}



//
// pwdLine() - Disegna una linea
//
void pwdLine(PWD_RECT *prumRect,PWD_COLOR colPen,PWD_VAL umWidth,INT iStyle)
{
    PDO_BOXLINE sObj;

	_(sObj);
	sObj.sPenBrush.colPen=colPen;
	sObj.sPenBrush.umPenWidth=umWidth; //win_infoarg("%d",iPenWidth);
	sObj.sPenBrush.iPenStyle=iStyle;
	_addItem(PDT_LINE,prumRect,&sObj,sizeof(sObj));
}

//
// pwdRect() - Disegna un quadrato
//
void pwdRect(PWD_RECT *prumRect,PWD_COLOR colPen,PWD_COLOR colSolidBrush,PWD_VAL umWidth) {

    PDO_BOXLINE sObj;

	_(sObj);
	sObj.sPenBrush.colPen=colPen;
	sObj.sPenBrush.colBrush=colSolidBrush;
	sObj.sPenBrush.umPenWidth=umWidth; //win_infoarg("%d",iPenWidth);
	sObj.sPenBrush.iPenStyle=PS_SOLID;
	_addItem(PDT_RECT,prumRect,&sObj,sizeof(sObj));

}

//
// pwdRectEx() - Disegna un quadrato con i bordi arrotondati
//
void pwdRectEx(PWD_RECT * prumRect,
			   PWD_COLOR colPen,PWD_VAL umWidth,INT iPenStyle,
			   PWD_COLOR colBrush,INT iBrushStyle,
			   PWD_VAL umRoundWidth,PWD_VAL umRoundHeight) {

    PDO_BOXLINE sObj;

	_(sObj);
	sObj.sPenBrush.colPen=colPen;
	sObj.sPenBrush.umPenWidth=umWidth; //win_infoarg("%d",iPenWidth);
	sObj.sPenBrush.iPenStyle=iPenStyle;

	sObj.sPenBrush.colBrush=colBrush;
	sObj.sPenBrush.iBrushStyle=iBrushStyle;

	sObj.sumRound.cx=umRoundWidth;
	sObj.sumRound.cy=umRoundHeight;

	_addItem(PDT_RECT,prumRect,&sObj,sizeof(sObj));

}


//
// pwdPath() - Disegna una linea
//
void pwdPath(EH_LST lstPath,
			 PWD_COLOR colPen,PWD_VAL umWidth,INT iStyle,
			 PWD_COLOR colBrush,INT iBrushStyle)
{
    PDO_PATH sObj;
	PWD_PTHE * psEle; 
	DWORD dwMemo;
	BYTE * pb;

	_(sObj);
	sObj.sPenBrush.colPen=colPen;
	sObj.sPenBrush.umPenWidth=umWidth; //win_infoarg("%d",iPenWidth);
	sObj.sPenBrush.iPenStyle=iStyle;

	sObj.sPenBrush.colBrush=colBrush;
	sObj.sPenBrush.iBrushStyle=iBrushStyle;

	//
	// Clono il percorso
	//
	sObj.iElements=lstPath->iLength; // Numero degli elementi (punti)
	
	//
	// Calcolo occupazione memoria per serializzazione
	// |PWD_PTHE+dati|PWD_PTHE+dati|PWD_PTHE+dati| ecc...
	//
	dwMemo=0;
	for (lstLoop(lstPath,psEle)) {
		dwMemo+=sizeof(PWD_PTHE)+psEle->dwSizeData;
	}	
	sObj.dwElements=dwMemo;	// Dimensione di tutti gli elementi dell'array [PWD_PTHE+dati]
	sObj.psEle=(PWD_PTHE *) pb=ehAlloc(dwMemo);
	
	// lst > serializzato
	for (lstLoop(lstPath,psEle)) {
		
		memcpy(pb,psEle,sizeof(PWD_PTHE)); pb+=sizeof(PWD_PTHE);
		if (psEle->dwSizeData) {
			memcpy(pb,psEle->psData,psEle->dwSizeData); pb+=psEle->dwSizeData;
		}

	}

	//
	// Calcolo le dimensioni del rettangolo occupato (da fare)
	//
	_addItem(PDT_PATH,NULL,&sObj,sizeof(sObj));
	ehFree(sObj.psEle);
}

//
//  _pathCreate() > Trasforma l'oggetto serializzato in una lista (per usarla meglio)
//
static EH_LST _pathCreate(PDO_PATH * psPath) {

	INT pt;
	PWD_PTHE sEle,* psEle, * psNext;
	EH_LST lst=lstCreate(sizeof(PWD_PTHE));
	
	
	psEle=(PWD_PTHE  *) ((BYTE * ) psPath+sizeof(PDO_PATH));
	for (pt=0;pt<psPath->iElements;pt++,psEle=psNext) {

		_(sEle);
		memcpy(&sEle,psEle,sizeof(sEle));
		if (sEle.dwSizeData) sEle.psData=(BYTE *) psEle+sizeof(PWD_PTHE);
		lstPush(lst,&sEle);

		psNext=(PWD_PTHE *) ((BYTE *) psEle+sizeof(PWD_PTHE)+psEle->dwSizeData);

	}
	return lst;
}

static void _pathDestroy(EH_LST lstPath) {
	
	lstDestroy(lstPath);

}

//
// pwdRectRound() - Disegna un quadrato con i bordi arrotondati
//
void pwdRectRound(PWD_RECT *prumRect,PWD_COLOR colPen,PWD_COLOR colSolidBrush,PWD_VAL umWidth,PWD_VAL umRoundWidth,PWD_VAL umRoundHeight) {

    PDO_BOXLINE sObj;

	_(sObj);
	sObj.sPenBrush.colPen=colPen;
	sObj.sPenBrush.colBrush=colSolidBrush;
	sObj.sPenBrush.umPenWidth=umWidth; //win_infoarg("%d",iPenWidth);
	sObj.sPenBrush.iPenStyle=PS_SOLID;

	sObj.sumRound.cx=umRoundWidth;
	sObj.sumRound.cy=umRoundHeight;
	_addItem(PDT_RECT,prumRect,&sObj,sizeof(sObj));

}

//
// _fontControl() - Controllo del font
//

static int CALLBACK _EnumFontFamiliesExProc( ENUMLOGFONTEXW * psLogFont, NEWTEXTMETRICEX *lpntme, int FontType, LPARAM lParam )
{ 
    //int far * aiFontCount = (int far *) aFontCount; 
	EH_LST lst=(EH_LST) lParam;
//	CHAR szName[300];

	if (FontType & TRUETYPE_FONTTYPE) {
//		_(sFont);
		//strcpy(szName,psLogFont->elfLogFont.lfFaceName);

//		sFont.enStyle=(psLogFont->elfLogFont.lfItalic?STYLE_ITALIC:0)|((psLogFont->elfLogFont.lfWeight>400)?STYLE_BOLD:0);
//		if (strstr(szName," Bold")) sFont.enStyle|=STYLE_BOLD;
		lstPush(lst,&psLogFont->elfLogFont);//&sFont);
	}

	return true;
//    UNREFERENCED_PARAMETER( lplf ); 
  //  UNREFERENCED_PARAMETER( lpntm ); 
} 

static BOOL _fontSearch(PDO_TEXT * psText,LOGFONTW * psDest) {

	EH_LST lst=lstCreate(sizeof(LOGFONTW));
	LOGFONTW * psLf=NULL;
	WCHAR * pwcFontFace;
	HDC hdc;
	INT a=0;
	BOOL bFound=FALSE;

	hdc=CreateCompatibleDC(0);
	pwcFontFace=utfToWcs(psText->pszFontFace?psText->pszFontFace:_sPower.pszFontBodyDefault);
	EnumFontsW(hdc,  pwcFontFace, (FONTENUMPROCW) _EnumFontFamiliesExProc, (LPARAM) lst);
	DeleteDC(hdc);
	ehFree(pwcFontFace);
	
	if (!psText->dwWeight) {

		psText->dwWeight=FW_NORMAL;
		if (psText->enStyles&STYLE_BOLD) psText->dwWeight=FW_BOLD;
	
	}
	if (psText->enStyles&STYLE_ITALIC) psText->dwItalic=true;

	if (lst->iLength) {

		bFound=true;

		if (lst->iLength>1) {
			
			for (lstLoop(lst,psLf)) {
/*
							DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
							//				  FERRA_OUTPRECISION, // Output precision
							OUT_DEVICE_PRECIS,//OUT_DEFAULT_PRECIS, // Output precision
							0, // Clipping Precision
							PROOF_QUALITY,//DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
							VARIABLE_PITCH,//DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)*/

				if (//psLf->lfCharSet==DEFAULT_CHARSET&&
					//psLf->lfOutPrecision==OUT_DEVICE_PRECIS&&
					//psLf->lfQuality==PROOF_QUALITY&&
					//psLf->lfPitchAndFamily==VARIABLE_PITCH&&
					psLf->lfWeight==psText->dwWeight&&
					psLf->lfItalic==psText->dwItalic) {
					
						break;
				}
			
//				printf("%d) %S" CRLF,a++,psLf->lfFaceName);

			}

			if (!psLf) psLf=lstGet(lst,0); // Non trovato, becco il primo


		} else {

			psLf=lstGet(lst,0);
		
		}
	}
	if (psDest&&psLf) memcpy(psDest,psLf,sizeof(LOGFONTW));
	lstDestroy(lst);

	return bFound;

}


//
// pwdText() - Disegna un testo
//
void pwdText(PWD_VAL	umX,
			 PWD_VAL	umY,
			 PWD_COLOR	colText,
			 EH_TSTYLE	enStyles,
			 EN_DPL		enAlign,
			 CHAR *		pszFont,
			 PWD_VAL	umCharHeight,
			 CHAR *		pszStr) 

{

	PDO_TEXT	sObj;
	
//	PWD_RECT sumRect;
//	PWD_SIZE sumSize;
	PWD_TXI sTi;
//	S_FAC *psFac;
//	HDC hdc;
	
	if (strEmpty(pszStr)) return; 

	_(sObj);
//	_(sumRect);
	sObj.umX=umX; 
	sObj.umY=umY;
	sObj.colText=colText;
	sObj.enStyles=enStyles;
	sObj.enAlign=enAlign;
	sObj.pszFontFace=pszFont;

	sObj.bFontFound=_fontSearch(&sObj,&sObj.sLogFontw);
	if (!sObj.bFontFound)
	{	
		printf("(*) Font not found" CRLF);
		return;
	}
	if (umCharHeight>0) sObj.umCharHeight=umCharHeight; else sObj.umCharHeight=_sPower.umRowHeight;
	sObj.pszText=pszStr;

	_fontPowerText(	PMA_MISURE,
					NULL,
					&sObj, 
					//false,
					&sTi);
					//NULL,
					//false);

	_addItem(PDT_TEXT,&sTi.rumText,&sObj,sizeof(sObj));

	//DeleteDC(hdc);

	// Da vedere
	// Calcolo in base ai DPI della dimensione dell'oggetto
/*
	hdc=CreateCompatibleDC(0);
	psFac=_fontPowerCreate(hdc,false,&sObj,false,NULL); // [font] NO PRINT
	sumSize.cx=psFac->sumText.cx; // pwdUm(PUM_DTX,psFac->sizText.cx);
	sumSize.cy=psFac->sumText.cy; // pwdUm(PUM_DTY,psFac->sizText.cy);
	DeleteDC(hdc);
 	switch (sObj.enAlign) {

		case PDA_LEFT:
			break;

		case PDA_CENTER:
			sumRect.left=umX-sumSize.cx/2;
			break;

		case PDA_RIGHT:
			sumRect.left=umX-sumSize.cx;
			break;
	
	}
	sumRect.right=sumRect.left+sumSize.cx;
	sumRect.bottom=sumRect.top+sumSize.cy;
	
	_addItem(PDT_TEXT,&sumRect,&sObj,sizeof(sObj));
	_LFontAmbientDestroy(psFac);
	*/

}



//
// pwdTextInRect() - Disegna un testo in un quadrato
//
void pwdTextInRect(PWD_RECT *prum,
				   PWD_COLOR colText,
				   EH_TSTYLE enStyles,
				   PWD_ALIGN enAlign,
				   CHAR *pszFont,
				   PWD_VAL umCharHeight,
				   PWD_VAL umInterLinea,
				   CHAR *pszStr,
				   BOOL bJustifyLastRow)
{
	PDO_TEXT sText;
	
	if (strEmpty(pszStr)) return; 

	_(sText);
	
	sText.bMultiline=true;
	sText.colText=colText;
	sText.enStyles=enStyles;
	sText.enAlign=enAlign;
	sText.pszFontFace=pszFont;
	if (umCharHeight>0) sText.umCharHeight=umCharHeight; else sText.umCharHeight=_sPower.umRowHeight;
	sText.pszText=pszStr;

	sText.umInterlinea=umInterLinea;
	sText.bJustifyLast=bJustifyLastRow;

	_addItem(PDT_TEXTBOX,prum,&sText,sizeof(sText));

}

//
// pwdTextInRectEx() - 
//
PWD_VAL pwdTextInRectEx(PWD_RECT *	prum,
						PWD_COLOR	colText,
						EH_TSTYLE	enStyles,
						PWD_ALIGN	enAlign,
						CHAR *		pszFont,
						PWD_VAL		umCharHeight,
						PWD_VAL		umInterLinea,
						CHAR *		pszStr,
						BOOL		bJustifyLastRow,
						PWD_VAL		umExtraCharSpace,
						INT			iMaxRows,
						INT *		lpiRows)
{
	PDO_TEXT sText;
	
	if (strEmpty(pszStr)) return 0; 

	_(sText);
	
	sText.bMultiline=true;
	sText.colText=colText;
	sText.enStyles=enStyles;
	sText.enAlign=enAlign;
	sText.pszFontFace=pszFont;
	if (umCharHeight>0) sText.umCharHeight=umCharHeight; else sText.umCharHeight=_sPower.umRowHeight;
	sText.pszText=pszStr;

	sText.umInterlinea=umInterLinea;
	sText.umExtraCharSpace=umExtraCharSpace;
	sText.bJustifyLast=bJustifyLastRow;
	sText.iMaxRows=iMaxRows;

	_addItem(PDT_TEXTBOX,prum,&sText,sizeof(sText));
	
	return pwdGetTextInRectAlt(prum,&sText,lpiRows);
}

//
// pwdText() - Disegna un testo
//
void pwdTextJs(PWD_VAL umX,PWD_VAL umY,CHAR * pszJsParams,CHAR *pszText) 
{
	PDO_TEXT sText;
	PWD_RECT sumRect;
	PWD_SIZE sumSize;
	PWD_VAL umCharHeight;
	S_FAC *psFac;
	HDC hdc;
	EH_JSON * psJs;
	CHAR * psz;
	EH_TSTYLE enStyles=0;
	PWD_ALIGN enAlign=0;
	CHAR * pszFontFace=NULL;
	TEXTMETRIC sTm;
	BOOL	bBaseLine=false;

	if (strEmpty(pszText)) return; 
	psJs=jsonCreate(pszJsParams); if (!psJs) ehError();

	_(sText);
	_(sumRect);
	sText.umX=umX;
	sText.umY=umY;
	sumRect.left=umX; sumRect.top=umY;
	sText.pszText=pszText;

	psz=jsonGet(psJs,"color"); if (psz) sText.colText=pwdColor(colorWeb(psz));
	
	//
	// Stili
	//
	psz=jsonGet(psJs,"style"); if (psz) enStyles=atoi(psz);
	psz=jsonGet(psJs,"italic"); if (psz) {if (!strCmp(psz,"true")) enStyles|=STYLE_ITALIC; else sText.dwItalic=atoi(psz);}
	psz=jsonGet(psJs,"underline"); if (psz) {if (!strCmp(psz,"true")) enStyles|=STYLE_UNDERLINE; else sText.dwUnderline=atoi(psz);}
	sText.enStyles=enStyles;

	psz=jsonGet(psJs,"weight"); if (psz) sText.dwWeight=atoi(psz);
		
	//
	// Allineamento
	//
	psz=jsonGet(psJs,"align"); 
	if (!strCmp(psz,"left"))  enAlign=PDA_LEFT;
	else if (!strCmp(psz,"center"))  enAlign=PDA_CENTER;
	else if (!strCmp(psz,"right"))  enAlign=PDA_RIGHT;
	else if (!strCmp(psz,"justify"))  enAlign=PDA_JUSTIFY;

	psz=jsonGet(psJs,"vertical-align"); 
	if (!strCmp(psz,"top"))  enAlign|=PDA_TOP;
	else if (!strCmp(psz,"middle"))  enAlign|=PDA_MIDDLE;
	else if (!strCmp(psz,"bottom"))  enAlign|=PDA_BOTTOM;

	sText.enAlign=enAlign;

	// Font
	psz=jsonGet(psJs,"fontface"); if (psz) pszFontFace=psz;
	sText.pszFontFace=pszFontFace; if (strEmpty(sText.pszFontFace)) ehError();

	// Altezza
	psz=jsonGet(psJs,"height"); if (psz) umCharHeight=atof(psz);
	if (umCharHeight!=0) sText.umCharHeight=umCharHeight; else sText.umCharHeight=_sPower.umRowHeight;

	// Larghezza del carattere
	psz=jsonGet(psJs,"width"); if (psz) sText.umCharWidth=atof(psz);

	// Spaces
	psz=jsonGet(psJs,"charspace"); if (psz) sText.umExtraCharSpace=atof(psz);


	// Calcolo in base ai DPI la dimensione dell'oggetto
	hdc=CreateCompatibleDC(0);
	psFac=_fontPowerCreate(hdc,false,&sText,false,&sTm); // [font] NO PRINT
	sumSize.cx=psFac->sumText.cx;//pwdUm(PUM_DTX,psFac->sizText.cx);
	sumSize.cy=psFac->sumText.cy;//pwdUm(PUM_DTY,psFac->sizText.cy);
	
	psz=jsonGet(psJs,"baseline"); if (psz) bBaseLine=atoi(psz);
	if (bBaseLine) {
	//	sumRect.top-=pwdUm(PUM_DTY,sTm.tmAscent);
		sText.enAlign|=PDA_BASELINE;
	}
	DeleteDC(hdc);
 	switch (sText.enAlign) {

		case PDA_LEFT:
			break;

		case PDA_CENTER:
			sumRect.left=umX-sumSize.cx/2;
			break;

		case PDA_RIGHT:
			sumRect.left=umX-sumSize.cx;
			break;
	
	}
	
	sumRect.right=sumRect.left+sumSize.cx;
	sumRect.bottom=sumRect.top+sumSize.cy;
	
	_addItem(PDT_TEXT,&sumRect,&sText,sizeof(sText));
	_LFontAmbientDestroy(psFac);
	jsonDestroy(psJs);

}


//
// pwdTextRiemp() > DA FARE
//
void pwdTextRiemp(PWD_RECT *prum,PWD_COLOR colText,EH_TSTYLE enStyles,CHAR *lpFont,PWD_VAL umHeight,CHAR *pszStr,CHAR Riemp) {

}


/*
//void LRDispRiemp(RECT rArea,INT Color,INT enStyle,CHAR *lpFont,INT umCharHeight,CHAR *lpString,CHAR Riemp)
{
	HFONT hFont,OldFont; 
	INT  xChar=0;
	CHAR *NewText;
	CHAR szServ[1024];
	RECT Rect;
	HDC hdc;

	SIZE size ;
	PDO_CHAREX LRTCharEx;

	memcpy(&Rect,&rArea,sizeof(RECT));
	if (umCharHeight<1) return;

	hdc=GetDC(NULL);
	NewText=ehAlloc(strlen(lpString)+1);
	if (sys.bOemString) OemToChar(lpString,NewText); else strcpy(NewText,lpString);

	hFont=CreateFont(umCharHeight, // Altezza del carattere
					 xChar, // Larghezza del carattere (0=Default)
					 0, // Angolo di rotazione x 10
					 0, //  Angolo di orientamento bo ???
					  (enStyle&STYLE_BOLD)?FW_BOLD:0,//_sPower.fBold;//sys.fFontBold;
					  (enStyle&STYLE_ITALIC)?1:0,//_sPower.fItalic;//sys.fFontItalic;
					  (enStyle&STYLE_UNDERLINE)?1:0,//_sPower.fUnderLine;//sys.fFontItalic;					 

					 0, // Flag StrikeOut  ON/OFF
					 DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
	//				   FERRA_OUTPRECISION, // Output precision
					 OUT_DEFAULT_PRECIS, // Output precision
					 0, // Clipping Precision
					 DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
					 DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
					 //0,
					 //"Arial"); // Nome del font
					 lpFont); // Nome del font "Courier New"

	OldFont=SelectObject(hdc, hFont);
	SetBkMode(hdc,TRANSPARENT); 
	SetMapMode(hdc, MM_TEXT);
    SetTextCharacterExtra(hdc,0);

	sprintf (szServ,"%s %c",lpString,Riemp);
	do
	{
		GetTextExtentPoint32(hdc,szServ,strlen (szServ),&size) ;
		sprintf (szServ,"%s%c",szServ,Riemp);

	} while ((INT) size.cx < (rArea.right - rArea.left)) ;

	szServ[strlen (szServ)-1]=0;

	SelectObject(hdc, OldFont);
	DeleteObject(hFont);

	ehFree(NewText);
	ReleaseDC(NULL,hdc);

	memset(&LRTCharEx,0,sizeof(LRTCharEx));
	LRTCharEx.Point.x=rArea.left;
	LRTCharEx.Point.y=rArea.top;
	LRTCharEx.enAlign=PDA_LEFT;
	LRTCharEx.umCharHeight=umCharHeight;
	LRTCharEx.fFix=FALSE;
	LRTCharEx.enStyle=enStyle;
//	LRTCharEx.fBold=_sPower.fBold;//sys.fFontBold;
//	LRTCharEx.fItalic=_sPower.fItalic;//sys.fFontItalic;
	LRTCharEx.colText=Color;
	strcpy(LRTCharEx.szFontSearch,lpFont);
	_addItem(PDT_CHAREX,&LRTCharEx,sizeof(LRTCharEx),szServ,0);
	return;
}
*/

void pwdSideCalc(PWD_SIZE *psSize,	// Dimensioni UM
				 DWORD dwWidth,
				 DWORD dwHeight)	// Dimensioni Fisiche
{
	PWD_VAL umLx,umLy;
	umLx=dwWidth; umLy=dwHeight;
	if ((psSize->cy>0)&&(psSize->cx==0)) psSize->cx=psSize->cy*umLx/umLy;
	// Calcolo automatico delle dimensioni verticali
	else if ((psSize->cx>0)&&(psSize->cy==0)) psSize->cy=psSize->cx*umLy/umLx;
}



//
// pwdImage()
//
void pwdImage(PWD_VAL px,PWD_VAL py,PWD_SIZE *psumSize,INT hImage,BOOL bStore) {

    PDO_IMAGE sImage;
	PWD_RECT rumRect;
	SIZE	sizImageSorg,sizImageDest;
	IMGHEADER *psImageHeader;
	BITMAPINFOHEADER *psBmpHeader;

	rumRect.left=px; rumRect.top=py;

	_(sImage);

	//
	// Leggo le dimensioni dell'immagine
	//
	psImageHeader=memoLockEx(hImage,"IMGCalc");
	psBmpHeader=(BITMAPINFOHEADER *) &psImageHeader->bmiHeader;
	sizImageSorg.cx=psBmpHeader->biWidth;
	sizImageSorg.cy=psBmpHeader->biHeight;
	memoUnlockEx(hImage,"IMGCalc");

	//
	// Potrei arrivare con una sola dimensione cx o cy
	//
	if (psumSize->cx==0||psumSize->cy==0) {
		pwdSideCalc(psumSize,sizImageSorg.cx,sizImageSorg.cy); // <----
	}	

	rumRect.right=rumRect.left+psumSize->cx;
	rumRect.bottom=rumRect.top+psumSize->cy;

	if (!bStore) {
	
		sImage.hImage=hImage;

	} 
	//
	// Memorizzo l'hdlImage in un file temporaneo (BMP)
	//
	else {

			CHAR szFileTemp[500];
			sImage.hImage=0; // Non in memoria

			// b) Calcolo dimensioni fisiche necessarie
			sizImageDest.cx= (LONG) pwdUm(PUM_DTX,psumSize->cx); // PUM_DTX
			sizImageDest.cy= (LONG) pwdUm(PUM_DTY,psumSize->cy);

			_imageResampleSave(hImage, sizImageSorg, sizImageDest,szFileTemp);
			sImage.pszFileTemp=strDup(szFileTemp);

	}

	_addItem(PDT_EHIMG,&rumRect,&sImage,sizeof(sImage));


}


static void _BmpDisp(HBITMAP hBitmap)//,BOOL fAbsolute)
{
	HDC hdc, hdcMemory;
	HBITMAP hbmpOld;
	BITMAP sBmpInfo;

	hdc=GetDC(NULL);

	// Leggo dimensioni
	GetObject(hBitmap, sizeof(BITMAP),&sBmpInfo);

	// Creo un area di memoria compatibile al DC
	hdcMemory	= CreateCompatibleDC(0);
	hbmpOld		= SelectObject(hdcMemory, hBitmap);

	// Trasferisco il bitmap dalla Copia --> Al DCwindows
	if (!BitBlt(hdc,  // --- DESTINAZIONE ---
		   0,0,
		   sBmpInfo.bmWidth, // Larghezza
		   sBmpInfo.bmHeight, //Altezza
 		   hdcMemory, // --- SORGENTE ---
		   0, 0, //Cordinate x-y
		   SRCCOPY))
		   ehExit("%d",GetLastError());

	SelectObject(hdcMemory, hbmpOld);
	DeleteDC(hdcMemory);
	ReleaseDC(NULL,hdc);
}




//
// pwdBitmap() - LRBitmap()
//
void pwdBitmap(PWD_VAL px,
			   PWD_VAL py,
			   PWD_SIZE *psumSize,
			   HBITMAP hBitmap,
			   BOOL bStore,
			   DWORD iColorDeepBit) // 0=Non ricodifico 1/4/8/24
{
	PDO_IMAGE sImage;
	PWD_RECT rumRect;
	SIZE sizImageSorg,sizImageDest;
	BITMAP sBmpInfo;

	if (psumSize->cx==0&&psumSize->cy==0) return; // Zero dimensioni, non stampo

	//
	// Leggo le dimensioni dell'immagine
	//
	GetObject(hBitmap, sizeof(BITMAP), &sBmpInfo);
	sizImageSorg.cx=sBmpInfo.bmWidth;
	sizImageSorg.cy=sBmpInfo.bmHeight;

	//
	// Potrei arrivare con una sola dimensione cx o cy > Calcolo l'altra
	//
	if (psumSize->cx==0||psumSize->cy==0) {
		pwdSideCalc(psumSize,sizImageSorg.cx,sizImageSorg.cy); // <----
	}	

	rumRect.left=px; rumRect.top=py;
	rumRect.right=rumRect.left+psumSize->cx;
	rumRect.bottom=rumRect.top+psumSize->cy;

	if (!bStore) {
	
		sImage.hBitmap=hBitmap;
		_addItem(PDT_BITMAP,&rumRect,&sImage,sizeof(sImage));

	} else {

		CHAR szFileTemp[500];
		INT hImageTemp;

		sImage.hBitmap=0;
		sizImageDest.cx= (LONG) pwdUm(PUM_DTX,psumSize->cx);
		sizImageDest.cy= (LONG) pwdUm(PUM_DTY,psumSize->cy);

		//
		// Converto l'immagine nel coloreDepth desiderato
		//
		if (iColorDeepBit) {
		
			HBITMAP hBitmapNew=winBitmapConvert(hBitmap,iColorDeepBit,NULL);
			hImageTemp = BitmapToImg(hBitmapNew,FALSE,NULL,0);
			DeleteObject(hBitmapNew);
		}
		else {
			hImageTemp = BitmapToImg(hBitmap,FALSE,NULL,0);
		}
		
		_imageResampleSave(hImageTemp, sizImageSorg, sizImageDest,szFileTemp);
		sImage.pszFileTemp=strDup(szFileTemp);
		memoFree(hImageTemp,"memo");

		_addItem(PDT_EHIMG,&rumRect,&sImage,sizeof(sImage));
	
	}

	
}

//
// LRImage()
//
void pwdImageList(PWD_VAL px,PWD_VAL py,PWD_SIZE *psumSize,HIMAGELIST himl,INT imgNumber,BOOL bStore) {

	PDO_IMAGE sImage;
	PWD_RECT rumRect;

	// Calcolo dimensione in UM (da fare)



	if (!bStore)  {

		sImage.himl=himl;
		sImage.hImage=imgNumber;
		rumRect.left=px; rumRect.top=py;
		rumRect.right=rumRect.left+psumSize->cx;
		rumRect.bottom=rumRect.top+psumSize->cy;
		_addItem(PDT_IMAGELIST,&rumRect,&sImage,sizeof(sImage));
	}
	else
	{

		// Crei il file e salvi come PDT_EHIMG
/*
		sizImageDest.cx= pwdUm(PUM_DTX,psumSize->cx);
		sizImageDest.cy= pwdUm(PUM_DTY,psumSize->cy);

		_imageResampleSave(INT HdlImage, SIZE sizImageSorg, SIZE sizImageDest);
*/	
		_addItem(PDT_EHIMG,&rumRect,&sImage,sizeof(sImage));
	}

}
/*
		BITMAP  sInfo;
		BITMAPINFO bi;
		HDC hdcTemp;

		//
		// Leggo le informazioni del bitmap
		//
		GetObject( hBitmap,sizeof( BITMAP ), &sInfo);
		if (!iBitsPixel) iBitsPixel=sInfo.bmBitsPixel;
		sImage.bAllocated=TRUE; // Allocato (da liberare alla fine)

		//
		// Creo il bitmap
		//
		_(bi);
		bi.bmiHeader.biSize = sizeof(bi.bmiHeader); // size of this struct
        bi.bmiHeader.biBitCount = 24;//iBitsPixel; // rgb 8 bytes for each component(3)
        bi.bmiHeader.biCompression = BI_RGB;// rgb = 3 components
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biWidth = sInfo.bmWidth;
		bi.bmiHeader.biHeight= sInfo.bmHeight;
		hdcTemp=CreateCompatibleDC(0); // create dc to store the image
        sImage.hBitmap = CreateDIBSection(hdcTemp, &bi, DIB_RGB_COLORS, 0, 0, 0); // create a dib section for the dc
		SelectObject(hdcTemp, sImage.hBitmap); // assign the dib section to the dc

		// Copio il bitmap
		BitBlt(hdcTemp, 0, 0, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, hdc, 0, 0, SRCCOPY); // copy hdc to our hdc
		DeleteDC(hdcTemp); // delete dc
		*/


//
// pwdGetTextInRectAlt()
//
//static double pwdGetTextInRectAlt(PDO_CHARBOX *LCharBox,CHAR *lpString,INT *lpiRows)
PWD_VAL pwdGetTextInRectAlt(PWD_RECT * pumRect,PDO_TEXT * psText,INT * lpiRows) {


	PWD_TXI * psTi;
	PWD_VAL cyRet;
//	xChar=0;
	if (psText->umCharHeight<1) return 0;

	psTi=pwdTextInfoCreate(psText,pumRect);
	if (lpiRows) * lpiRows=psTi->lstRow->iLength;
	cyRet=psTi->sumText.cy;
	pwdTextInfoDestroy(psTi);

/*
	//
	// ATTENZIONE
	// Devo avere un device context di dimensioni identiche alla stampante per simulare
	// la dimensione dinamica del carattere in modo corretto
	//
	// Faccio fare il calcolo in Dot
	// 
	hdc=CreateCompatibleDC(0);

	psFac=_fontPowerCreate(hdc,psText,false,NULL);
	recArea.left=(INT) pwdUm(_DTXD,prRect->left);
	recArea.right=(INT) pwdUm(_DTXD,prRect->right);
	recArea.top=(INT) pwdUm(_DTXD,prRect->top);
	recArea.bottom=(INT) pwdUm(_DTXD,prRect->bottom);

	y=_textInRect(hdc,&recArea,psText,psFac->pwcText,FALSE,lpiRows);//,INT iMaxRows);

	_LFontAmbientDestroy(psFac);
	DeleteDC(hdc);
	if (!y) //y=(INT) pwdUm(_DTYD,psText->umCharHeight);
		y=_textInRect(hdc,&recArea,psText,L" ",FALSE,lpiRows);//,INT iMaxRows);
	cyRet=pwdUm(PUM_DTYD_TO_UM ,y);
	*/
	return cyRet;
}

//
// _fontPowerText() > Calcola (le dimensioni) e Stampa font/testo
//
void _fontPowerText(EN_PMODE	enMode,	
					HDC			hdc,
					PDO_TEXT *	psText,
//					BOOL		bPrint,
					PWD_TXI *	psTi)
//					BOOL		bPreview
					 
{

	WCHAR * pwcFontFace, * pwcText;
	INT	 iCharHeight,iCharWidth,iExtraSpace;
	SIZE	sizText;
	INT iLen;
//	LOGFONTW sLogFont;
	HFONT hFont,hOldFont;
	
	BOOL bDcDelete=false;
	TEXTMETRIC sTextMetrics;
	PWD_UM	enDpiX,enDpiY; // Tipo di conversione

	switch (enMode) {
	
		case PMA_MISURE:
			enDpiX=PUM_DTHDX; enDpiY=PUM_DTHDY;
			break;
	
		case PMA_PREVIEW:
			enDpiX=_DTXD; enDpiY=_DTYD;
			break;

		case PMA_PRINT:
			enDpiX=PUM_DTX; enDpiY=PUM_DTY;
			break;

	}

	if (!hdc) {hdc=CreateCompatibleDC(0); bDcDelete=true;}

	// Trovo testo e font
	if (!strEmpty(psText->pszFontFace)) pwcFontFace=_strTextDecode(psText->pszFontFace); else pwcFontFace=_strTextDecode(_sPower.pszFontBodyDefault);
	
	// Testo
	pwcText=_strTextDecode(psText->pszText); iLen=wcslen(pwcText);

	//
	// Trasformo x/y e dimensioni del testo in punti sulla base della densità del DIP
	//

	iCharHeight=(INT) numRound(pwdUmTo(psText->umCharHeight,enDpiY),0,true);
	iCharWidth=(INT) numRound(pwdUmTo(psText->umCharWidth,enDpiX),0,true);	 // <-- Non gestito
	iExtraSpace=(INT) numRound(pwdUmTo(psText->umExtraCharSpace,enDpiX),0,true); // <-- Non gestito 
	
	//	if (iCharHeight==0) return psFac;

	//
	// Stampa il testp
	//
	if (psText->enAlign&DPL_HEIGHT_TYPO) iCharHeight=-iCharHeight;

	hFont=CreateFontW(	
							iCharHeight,	// Altezza del carattere
							iCharWidth,		// Larghezza del carattere (0=Default)
							0,				// Angolo di rotazione x 10
							0,				// Angolo di orientamento bo ???

							(psText->enStyles&STYLE_BOLD)?FW_BOLD:psText->dwWeight,//_sPower.fBold;//sys.fFontBold;
							(psText->enStyles&STYLE_ITALIC)?1:psText->dwItalic,//_sPower.fItalic;//sys.fFontItalic;
							(psText->enStyles&STYLE_UNDERLINE)?1:psText->dwUnderline,//_sPower.fUnderLine;//sys.fFontItalic;					 

							0, // Flag StrikeOut  ON/OFF
							DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
							//				  FERRA_OUTPRECISION, // Output precision
							OUT_DEVICE_PRECIS,//OUT_DEFAULT_PRECIS, // Output precision
							0, // Clipping Precision
							PROOF_QUALITY,//DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
							VARIABLE_PITCH,//DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
							//0,
							//"Arial"); // Nome del font
							pwcFontFace); // Nome del font "Courier New"


	//
	// Allineamento
	//

	hOldFont=SelectObject(hdc, hFont);
	SetMapMode(hdc, MM_TEXT);
	SetTextCharacterExtra(hdc,iExtraSpace);//1); // <--- Cambiato nel 2009
	SetTextJustification(hdc,0,0);

	//
	// Calcolo le grandezze fisiche in UM
	//
	switch (enMode) {
	
		case PMA_MISURE:

			//
			// a. Testo Su una linea
			//
			if (!psText->bMultiline) {

				double py=psText->umY;

				GetTextExtentPoint32W(hdc, pwcText, iLen,&sizText);
				psTi->sumText.cx= pwdUm(enDpiX,sizText.cx);
				psTi->sumText.cy= pwdUm(enDpiY,sizText.cy);

				// Calcolare spostamento in asse Y
				if (psText->enAlign&DPL_HEIGHT_TYPO) {
				
					GetTextMetrics(hdc,&sTextMetrics);
					py-=pwdUm(enDpiY,sTextMetrics.tmAscent);

				}

				switch (psText->enAlign&0xF)
				{
					case PDA_LEFT: 
						psTi->rumText.left=psText->umX;
						psTi->rumText.top=py;
						psTi->rumText.right=psTi->rumText.left+psTi->sumText.cx;
						psTi->rumText.bottom=psTi->rumText.top+psTi->sumText.cy;
						break;

					case PDA_RIGHT: 
						psTi->rumText.right=psText->umX;
						psTi->rumText.top=py;
						psTi->rumText.left=psTi->rumText.right-psTi->sumText.cx;
						psTi->rumText.bottom=psTi->rumText.top+psTi->sumText.cy;
						break;

					case PDA_CENTER: 
						psTi->rumText.left=psText->umX-psTi->sumText.cx/2;
						psTi->rumText.top=py;
						psTi->rumText.right=psTi->rumText.left+psTi->sumText.cx;
						psTi->rumText.bottom=psTi->rumText.top+psTi->sumText.cy;
						break;

					default:
						ehError();
				}	

			}
			//
			// b. Misura del Testo in un rettangolo (multiline)
			//
			else  {

				ehError();

			}
			break;


		/*

				pt.x=psText->umX;
				pt.y=psText->umY;
				dcFontText(	hdc,
							pwcText,	
							&pt, // Colore non indicato
							0,

							&psText->sLogFontw,
							pwcFontFamily,
							psText->umCharHeight,	// Altezza in un di misura
							psText->enStyles,
							psText->enAlign,

							&sizText,
							&recText);
				if (psTi) {

					memset(psTi,0,sizeof(PWD_TXI));

					//
					// Ritorno in unità di misura
					//
					psTi->sumText.cx= sizText.cx; // pwdUm(bPreview?_DTXD:PUM_DTX,sizText.cx); // psFac->sumText.cx; //pwdUm(PUM_DTX,psFac->sizText.cx);
					psTi->sumText.cy= sizText.cy; // pwdUm(bPreview?_DTYD:PUM_DTY,sizText.cy); // psFac->sumText.cy; //pwdUm(PUM_DTY,psFac->sizText.cy);
					psTi->rumText.left= recText.left;
					psTi->rumText.top= recText.top;
					psTi->rumText.right= recText.right;
					psTi->rumText.bottom= recText.bottom;

				}
		*/
	
		case PMA_PREVIEW:
		case PMA_PRINT:
			{
				RECT	sRect;
				POINTD	pt;
				INT		iVertical;
				UINT	uOld;

				SetBkMode(hdc,TRANSPARENT); 
				SetTextColor(hdc,psText->colText.ehColor);

				// Stampa
				_pointToPixel(psText->umX,psText->umY,&pt.x,&pt.y);

				//
				// a. Testo Su una linea
				//
				if (!psText->bMultiline) {
					
					UINT uMode=0;

					switch (psText->enAlign&0xf)
					{
						case PDA_JUSTIFY:
						case PDA_LEFT: 
							uMode|=TA_LEFT; 
							break;

						case PDA_RIGHT: 
							uMode|=TA_RIGHT; 
							break;

						case PDA_CENTER: 
							uMode|=TA_CENTER; 
							break;
					}

					uMode|=TA_TOP;
					if (psText->enAlign&PDA_TOP) uMode|=TA_TOP;
					else if (psText->enAlign&PDA_BOTTOM) uMode|=TA_BOTTOM;
					else if (psText->enAlign&PDA_BASELINE) uMode|=TA_BASELINE;

					uOld=SetTextAlign(hdc,uMode);
					TextOutW(hdc, (INT) pt.x, (INT) pt.y, pwcText, iLen);

					SetTextAlign(hdc,uOld);

 				} else {

				//
				// b. Multiline
				//
					UINT	uFormat=DT_BOTTOM|DT_SINGLELINE|DT_NOCLIP;

					switch (psText->enAlign&0xf) {

						case PDA_LEFT: 
							uFormat|=DT_LEFT; 
							break;

						case PDA_RIGHT:
							uFormat|=DT_RIGHT; 
							break;

						case PDA_CENTER: 
							uFormat|=DT_CENTER; 
							break;
					
					}
					
					iVertical=TA_TOP;
					if (psText->enAlign&PDA_TOP) iVertical=TA_TOP;
					else if (psText->enAlign&PDA_BOTTOM) iVertical=TA_BOTTOM;
					else if (psText->enAlign&PDA_BASELINE) iVertical=TA_BASELINE;

			/*
					if (psText->bBaseLine) {
						rectCopy(sRect,psDraw->recObj);
						sRect.top-=sTM.tmAscent;
						sRect.bottom-=sTM.tmAscent;
						psRect=&sRect;
					} 
					else {
						psRect=&psDraw->recObj;
					}
					*/
				// 	dcRect(psDraw->hDC,&psDraw->recObj,RGB(255,0,0),-1,1); // per Debug

					sRect.left=(INT) floor(pt.x);
					sRect.top=(INT) floor(pt.y);
					
					sRect.right=sRect.left+10000; // Devo avere le dimensioni
					sRect.bottom=sRect.top+10000;

					uOld=SetTextAlign(hdc,iVertical);
					DrawTextW(	hdc,
								pwcText,iLen,
								&sRect,//&psDraw->recObj,
								uFormat);

					// _textInRect(psDraw->hDC,&psDraw->recObj,psText,psFac->pwcText,TRUE,&iRows);
					SetTextAlign(hdc,uOld);
				}
				break;
			}
	}		
	ehFreePtrs(2,&pwcText,&pwcFontFace);

	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);

	if (bDcDelete) 
		DeleteDC(hdc);

}


//
// _fontPowerCreate()
//
static S_FAC * _fontPowerCreate(HDC hdc,
								BOOL bVirtual,
								PDO_TEXT * psText,
								BOOL bDcAlign,
								TEXTMETRIC * psTextMetrics) {

	INT	iCharHeight,iCharWidth,iExtraSpace;
	BYTE	*	pszText=NULL;
	BYTE	*	pszFontFace=NULL;
	S_FAC	*	psFac=ehAllocZero(sizeof(S_FAC));	
	SIZE		sizText;

	// Trovo testo e font
	pszText=(BYTE *) psText->pszText; //pszText+=sizeof(PDO_TEXT);
	if (!strEmpty(psText->pszFontFace)) pszFontFace=psText->pszFontFace; else pszFontFace=_sPower.pszFontBodyDefault;
	
	// Testo
	psFac->pwcText=_strTextDecode(pszText);
	psFac->iLength=wcslen(psFac->pwcText);


	//
	// Calcolo le grandezze fisiche
	//
	iCharHeight=(INT) pwdUmTo(psText->umCharHeight,bVirtual?_DTYD:PUM_DTY);
	iCharWidth=(INT) pwdUmTo(psText->umCharWidth,bVirtual?_DTXD:PUM_DTX);
	iExtraSpace=(INT) pwdUmTo(psText->umExtraCharSpace,bVirtual?_DTXD:PUM_DTX);
	if (iCharHeight==0) return psFac;

	
	//
	// Stampa il testp
	//
	if (psText->enAlign&DPL_HEIGHT_TYPO) iCharHeight=-iCharHeight;
	psFac->hFont=CreateFont(	
							iCharHeight,	// Altezza del carattere
							iCharWidth,		// Larghezza del carattere (0=Default)
							0,				// Angolo di rotazione x 10
							0,				// Angolo di orientamento bo ???

							(psText->enStyles&STYLE_BOLD)?FW_BOLD:psText->dwWeight,//_sPower.fBold;//sys.fFontBold;
							(psText->enStyles&STYLE_ITALIC)?1:psText->dwItalic,//_sPower.fItalic;//sys.fFontItalic;
							(psText->enStyles&STYLE_UNDERLINE)?1:psText->dwUnderline,//_sPower.fUnderLine;//sys.fFontItalic;					 

							0, // Flag StrikeOut  ON/OFF
							DEFAULT_CHARSET, // Tipo di codepage vedi CreateFont (0=Ansi)
							//				  FERRA_OUTPRECISION, // Output precision
							OUT_DEVICE_PRECIS,//OUT_DEFAULT_PRECIS, // Output precision
							0, // Clipping Precision
							PROOF_QUALITY,//DEFAULT_QUALITY,//PROOF_QUALITY, // Qualità di stampa (DEFAULT,DRAFT ecc...)
							VARIABLE_PITCH,//DEFAULT_PITCH,//FIXED_PITCH, // Pitch & Family (???)
							//0,
							//"Arial"); // Nome del font
							pszFontFace); // Nome del font "Courier New"

//	OldFont=
	SelectObject(hdc, psFac->hFont);
	SetBkMode(hdc,TRANSPARENT); 
	SetTextColor(hdc,psText->colText.ehColor);
	SetMapMode(hdc, MM_TEXT);
	SetTextCharacterExtra(hdc,iExtraSpace);//1); // <--- Cambiato nel 2009

	//
	// Allineamento
	//
	SetTextJustification(hdc,0,0);
	if (bDcAlign) {
		switch (psText->enAlign)
		{
			case PDA_LEFT: 
				SetTextAlign(hdc,TA_LEFT);
				break;

			case PDA_RIGHT: 
				SetTextAlign(hdc,TA_RIGHT);
				break;
			case PDA_CENTER: 
				SetTextAlign(hdc,TA_CENTER);
				break;
		}
	} else SetTextAlign(hdc,TA_LEFT);
	if (psTextMetrics) GetTextMetrics(hdc,psTextMetrics);

//		GetTextExtentPoint32W(hdc, psFac->pwcText, wcslen(psFac->pwcText),&psFac->sizText);
	GetTextExtentPoint32W(hdc, psFac->pwcText, wcslen(psFac->pwcText),&sizText);
	psFac->sumText.cx= pwdUm(PUM_DTX,sizText.cx);
	psFac->sumText.cy= pwdUm(PUM_DTY,sizText.cy);

	return psFac;
}


static void _LFontAmbientDestroy(S_FAC *psFac) {

	ehFreePtr(&psFac->pwcText);
	if (psFac->hFont) DeleteObject(psFac->hFont);//_sPd.hFontBase);
	ehFree(psFac);
}

//
// _fontEnumeration() 
//  Enumero i font presenti nel documento
//  Costruisco _sPower.dmiFont con i font usati nel documento
//  I fonts sono divisi per fontFace/Peso/Style/Larghezza del carattere (per i fix) 
//
//
static void	_fontEnumeration(BOOL bFontAlloc) {

	PWD_FONT sFont;
	PDO_TEXT *psText;
	INT a,b,nPage,idx;
	PWD_DRAW sItemDraw;
//	BYTE *pb;
	

	DMIOpen(&_sPower.dmiFont,RAM_AUTO,100,sizeof(PWD_FONT),"Font");
	_sPower.arsFont=DMILock(&_sPower.dmiFont,NULL);
	// Loop sulle pagine
	for (nPage=0;nPage<_sPower.iPageCount;nPage++)
	{
		_(sItemDraw);
		// Loop sugli item delle pagine
		for (a=_sPd.ariPageIndex[nPage];a<_sPd.nItems;a++)
		{
			sItemDraw.psItem=_sPd.arsItem[a]; 

			// Solo testi
			if ((sItemDraw.psItem->enType!=PDT_TEXTBOX) && (sItemDraw.psItem->enType!=PDT_TEXT)) continue;
			
			psText=pwdGetObj(sItemDraw.psItem);
			
			_(sFont);
			sFont.enStyles=psText->enStyles;
			sFont.dwItalic=psText->dwItalic;
			sFont.dwWeight=psText->dwWeight;

			/*
			printf("- %s %s %d %d" CRLF,
					psText->pszText,
					psText->pszFontFace,
					psText->dwItalic,
					psText->dwWeight);
*/
			if (psText->pszFontFace) 
				strcpy(sFont.szFontFace,psText->pszFontFace);
				else 
				strcpy(sFont.szFontFace,_sPower.pszFontBodyDefault);

			sFont.umCharHeight=0;
			sFont.umCharWidth=psText->umCharWidth;

			idx=-1;
			for (b=0;b<_sPower.dmiFont.Num;b++)
			{
				PWD_FONT sFontTmp;
				_(sFontTmp);
				if (!memcmp(_sPower.arsFont+b,&sFont,sizeof(PWD_FONT))) {
					idx=b; break;
				}
			}

			// se non trovo il font lo inserisco nella dmi
			if (idx<0) {
				// 
				// 400 = FW_NORMAL
				DMIAppendDyn(&_sPower.dmiFont,&sFont); 
				idx=_sPower.dmiFont.Num-1;
				_sPower.arsFont=DMILock(&_sPower.dmiFont,NULL);
			}

			// Aggiorno l'elemento font con l'indice
			psText->idxFont=idx;
		}
	}
	
}

//
// pwdImgCalc() - Calcola proporzionalmente il lato mancante di un immagine
//
void pwdImgCalc(PWD_SIZE *psSize,INT hImage)
{
	IMGHEADER *imgHead;
	WORD    a=1;
	BITMAPINFOHEADER *sBmpHeader;
	PWD_VAL umLx,umLy;

	imgHead=memoLockEx(hImage,"pwdImgCalc");
	sBmpHeader=(BITMAPINFOHEADER *) &imgHead->bmiHeader;
	memoUnlockEx(hImage,"pwdImgCalc");

	umLy=sBmpHeader->biHeight; umLx=sBmpHeader->biWidth;

	if ((psSize->cy>0)&&(psSize->cx==0)) psSize->cx=psSize->cy*umLx/umLy;
	// Calcolo automatico delle dimensioni verticali
	else if ((psSize->cx>0)&&(psSize->cy==0)) psSize->cy=psSize->cx*umLy/umLx;

}

//
// _imageResampleSave
// esegue il resampling (se conveniente) dell' immagine e la salva in un file temporaneo
//
static void _imageResampleSave(INT hImage, SIZE sizImageSorg, SIZE sizImageDest,CHAR *pszTempFile)
{
	BOOL	bFree=FALSE;
	INT	hImageTemp=0;

	//
	// c) Ricampioni
	//
	hImageTemp=hImage;
	if ((sizImageDest.cx<sizImageSorg.cx) || (sizImageDest.cy<sizImageSorg.cy)) {

		IMGHEADER *psImgHeader;
		INT iBitColor;
		psImgHeader=memoLock(hImage); 
		iBitColor=psImgHeader->bmiHeader.biBitCount;
		memoUnlock(hImage);
	}
	
	fileTempName( _sPower.pszTempFolder, "pwd", pszTempFile,  FALSE ); 
	if (!BMPSaveFile(pszTempFile, hImageTemp)) ehExit("errore salvataggio bitmap temporaneo!");
	if (bFree) memoFree(hImageTemp,"memo");
	//return pszTempFile;

}




#ifdef EH_PDF

//
// _fontFileSearch()
//
static BOOL _fontFileSearch(PWD_FONT * psPwdFont, CHAR * pszFontPathFile,BOOL * pbEmbedd)
{
	static	HKEY hkey = NULL;
	CHAR	szName[1000];
	CHAR	szNameFont[1000];
  	CHAR	szData[1000]; // <------------- ERRORRRONE !!!!!!!!!!!!!!!!!!
	BOOL	bResult = FALSE;
   	CHAR	szFontFullPath[1000];
	CHAR	szFontSearch[300];
	LPCTSTR lpszFontName=NULL;
	LPTSTR	lpszDisplayName=NULL;
	LPTSTR	lpszFontFile=NULL;
	CHAR *	pszName;
    PWD_FONT_RES * psFontRes=NULL;
	INT iTent;

	// calcolo del nome font
	for (iTent=0;iTent<4;iTent++) {

		strcpy(szFontSearch,psPwdFont->szFontFace);

		switch (iTent) {
			
			case 0:
				if ((psPwdFont->enStyles&STYLE_BOLD)||(psPwdFont->dwWeight>=700)) strcat(szFontSearch," Bold");
				if ((psPwdFont->enStyles&STYLE_ITALIC)||psPwdFont->dwItalic) strcat(szFontSearch," Italic");
				break;

			case 1:
				if ((psPwdFont->enStyles&STYLE_BOLD)||(psPwdFont->dwWeight>=700)) strcat(szFontSearch," Bold");
				break;

			case 2:
				if ((psPwdFont->enStyles&STYLE_ITALIC)||psPwdFont->dwItalic) strcat(szFontSearch," Italic");
				break;
			
			case 3:
				break;

		}
//	if (psPwdFont->enStyles&STYLE_UNDERLINE) strcat(szFontSearch," Underline");
	
	// 	printf("Cerco '%s'" CRLF ,szFontSearch);
		*pbEmbedd=HPDF_FALSE;

		//
		// Risorse temporanee
		//
		for (lstLoop(_sPower.lstFontRes,psFontRes)) {
		
			if (!strCmp(psFontRes->szName,szFontSearch)) {
			
				bResult = true;
				*pbEmbedd=psFontRes->bAddInFile;
				if (!strEmpty(psFontRes->szFileName)) {
					strcpy(pszFontPathFile,psFontRes->szFileName);
					return bResult;
				}
			}
		}

	// psFontRes se carico contiene le indicazioni per il caricamento nel PDF

		//
		// Cerco il font
		//
	
		*szName=0;	*szData=0;
 		strcpy(szFontFullPath, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts");
  		while (_getNextNameValue(HKEY_LOCAL_MACHINE, szFontFullPath, szName, szData) == ERROR_SUCCESS)
  		{

			if (*szName==*szFontSearch) {

				pszName=strExtract(szName,NULL," (",false,false);
				if (pszName) {strcpy(szNameFont,pszName);  ehFree(pszName);} else strcpy(szNameFont,szName);
				strTrim(szNameFont);
			//	printf("[%s]" CRLF ,szNameFont);

				if (!strCaseCmp(szNameFont,szFontSearch)) {
	  				
					sprintf(szFontFullPath,"%s\\Fonts\\%s",_sPower.szWinDir,szData); // <--------------
					if (fileCheck(szFontFullPath)) {
						bResult = true; break;
					}
  				}
			}
			*szFontFullPath=0;

  		}
		_getNextNameValue(HKEY_LOCAL_MACHINE, NULL, NULL, NULL); // Chiudo la funzione di accesso al registro

		if (!strEmpty(szData)) break;
	}

	// se non trovo la corrispondenza nel registro lo segnalo
	if (strEmpty(szData)) {
		//if (strEmpty(szFontPathFile)) {
		
			ehLogWrite("Errore nella ricerca del nome file per il font : %s",szFontSearch);
	#ifdef EH_CONSOLE
			printf("font %s non trovato" CRLF,szFontSearch);
	#endif
			return FALSE;
	}

	strcpy(pszFontPathFile,szFontFullPath);
	return bResult;
}




static LONG _getNextNameValue(HKEY key, LPCTSTR pszSubkey, LPTSTR pszName, LPTSTR pszData)
{
  	static HKEY hKey = NULL;	// registry handle, kept open between calls
  	static DWORD dwIndex = 0;	// count of values returned
  	LONG retval;
  	TCHAR szValueName[MAX_PATH];
  	DWORD dwValueNameSize = sizeof(szValueName)-1;
  	BYTE szValueData[MAX_PATH];
  	DWORD dwValueDataSize = sizeof(szValueData)-1;
  	DWORD dwType = 0;
  
  	// if all parameters are NULL then close key
  	if (!pszSubkey && !pszName && !pszData)
  	{
  		//TRACE(_T("closing key\n"));
  		if (hKey) RegCloseKey(hKey);
  		hKey = NULL;
  		return ERROR_SUCCESS;
  	}
  
  	// if subkey is specified then open key (first time)
  	if (!hKey&&!strEmpty((CHAR *) pszSubkey))//  pszSubkey && pszSubkey[0] != 0)
  	{
  		retval = RegOpenKeyEx(key, pszSubkey, 0, KEY_ALL_ACCESS, &hKey);
  		if (retval != ERROR_SUCCESS)
  		{
  			//TRACE(_T("ERROR: RegOpenKeyEx failed\n"));
  			return retval;
  		}
  		else
  		{
  			//TRACE(_T("RegOpenKeyEx ok\n"));
  		}
  		dwIndex = 0;
  	}
  	else
  	{
 		dwIndex++;
  	}
  
  	//_ASSERTE(pszName != NULL && pszData != NULL);
  
  	*pszName = 0;
  	*pszData = 0;
  
	if (!hKey) ehError();
  
  	retval = RegEnumValue(hKey, dwIndex, szValueName, &dwValueNameSize, NULL, &dwType, szValueData, &dwValueDataSize);
  	if (retval == ERROR_SUCCESS)
  	{
  		//TRACE(_T("szValueName=<%s>  szValueData=<%s>\n"), szValueName, szValueData);
  		lstrcpy(pszName, (LPTSTR) szValueName);
  		lstrcpy(pszData, (LPTSTR) szValueData);
  	}
  	else
  	{
  		//TRACE(_T("RegEnumKey failed\n"));
  	}
  
  	return retval;
}



//
//	Gestione PDF
//
//  Links:
//  https://github.com/libharu/libharu/wiki/API%3A-Page
// 
//

typedef struct {
	
	HPDF_REAL x;
	HPDF_REAL y;

} HPDF_POINT;

typedef struct {
	
	HPDF_REAL left;
	HPDF_REAL top;
	HPDF_REAL right;
	HPDF_REAL bottom;

} HPDF_RECT;

typedef struct {
	
	HPDF_REAL cx;
	HPDF_REAL cy;

} HPDF_SIZE;


BOOL ehPdfGetForm(CHAR *pszDeviceDefine,PWD_FORMINFO *psFormInfo) {

	SIZE_T	tSize;
	BYTE *	psTemp;
	EH_ARF arfPrinter;
	if (!pszDeviceDefine) pszDeviceDefine="";
	arfPrinter=ARFSplit(pszDeviceDefine,"\1");
	
	memset(psFormInfo,0,sizeof(PWD_FORMINFO));

	//
	// Codifica base64
	//
	if (!strCmp(arfPrinter[1],"RES")) {
	
		psTemp=base64Decode(arfPrinter[2],&tSize);
		if (tSize==sizeof(PWD_FORMINFO)) memcpy(psFormInfo,psTemp,sizeof(PWD_FORMINFO));
		ehFree(psTemp);

	} 
	//
	// Codifica Json
	//
	else if (!strCmp(arfPrinter[1],"JSON")) {
	
		CHAR * psz;
		EH_JSON * js=jsonCreate(arfPrinter[2]);
		psz=jsonGet(js,"format");

/*
		A0 	841 x 1.189		33 x 47
		A1 	594 x 841		23 x 33
		A2 	420 x 594		16,5 x 23
		A3 	297 x 420		11,5 x 16,5
		A4 	210 x 297		8,3 x 11,7
		A5 	148 x 210		5,8 x 8,3
		B0 	1.000 x 1.414	39,37 x 55,67
		B1 	700 x 1.000		27,83 x 39,37
		B2 	500 x 707		19,69 x 27,83
		B3 	353 x 500		13,90 x 19,69
		B4 	250 x 353		9,84 x 13,90
		B5 	176 x 250		6,93 x 9,84
*/

		if (!strCaseCmp(psz,"A4")) {
			psFormInfo->iPaper=DMPAPER_A4;
			strcpy(psFormInfo->szPaperName,"A4 (210 x 297 mm)");
			psFormInfo->sizForm.cx=pwdUm(PUM_MM,210);
			psFormInfo->sizForm.cy=pwdUm(PUM_MM,297);
		}
		else if (!strCaseCmp(psz,"LETTER")) {
			psFormInfo->iPaper=DMPAPER_LETTER;
			strcpy(psFormInfo->szPaperName,"Letter (8 1/2 x 11 in)");
			psFormInfo->sizForm.cx=pwdUm(PUM_INCH,8.5);
			psFormInfo->sizForm.cy=pwdUm(PUM_INCH,11);
		}
		else if (!strCaseCmp(psz,"LEGAL")) {
			psFormInfo->iPaper=DMPAPER_LEGAL;
			strcpy(psFormInfo->szPaperName,"Legal (8 1/2 x 14 in)");
			psFormInfo->sizForm.cx=pwdUm(PUM_INCH,8.5);
			psFormInfo->sizForm.cy=pwdUm(PUM_INCH,14);
		}

		psz=jsonGet(js,"orientation");
		if (!strCmp(psz,"portrait")) psFormInfo->iOrientation=DMORIENT_PORTRAIT;
		if (!strCmp(psz,"landscape")) psFormInfo->iOrientation=DMORIENT_LANDSCAPE;

		jsonDestroy(js);

	}

	if (!psFormInfo->iPaper) {
		psFormInfo->iPaper=DMPAPER_A4;
		psFormInfo->iOrientation=DMORIENT_PORTRAIT;
		strcpy(psFormInfo->szPaperName,"A4 (210 x 297 mm)");
		psFormInfo->sizForm.cx=pwdUm(PUM_CM,21.0);
		psFormInfo->sizForm.cy=pwdUm(PUM_CM,29.7);
	}

	if (!psFormInfo->iOrientation) {

		psFormInfo->iOrientation=DMORIENT_PORTRAIT;
	}

	ehFree(arfPrinter);
	return true;

}

static void _pdfErrorHandler(HPDF_STATUS   error_no,
					   HPDF_STATUS   detail_no,
                       void          *user_data)
{
    printf ("ERROR: error_no=x%04X, detail_no=%u\n", (HPDF_UINT)error_no, (HPDF_UINT)detail_no);
    longjmp(_sPd.pdfEnv, 1);
}

//
// _LPdfColorRGB() > Converte il colore nel formato RGB del pdf
//
/*
static void _LPdfColorRGB(EH_COLOR col,HPDF_RGBColor *psRgb) {

	// x:1=col:255
	psRgb->b=(HPDF_REAL) ((col>>16)&0xFF); psRgb->b/=255; 
	psRgb->g=(HPDF_REAL) ((col>>8)&0xFF); psRgb->g/=255;
	psRgb->r=(HPDF_REAL) (col&0xFF); psRgb->r/=255; 

}
*/

void _pdfColorFill(HPDF_Page psPage,PWD_COLOR * psColor) {

	HPDF_STATUS hSts;
	switch (psColor->enFormat) {

		case PWC_RGB:

			hSts=HPDF_Page_SetRGBFill(	psPage,
									(HPDF_REAL) psColor->dComp[0],
									(HPDF_REAL) psColor->dComp[1],
									(HPDF_REAL) psColor->dComp[2]);
			break;

		case PWC_CMYK:

			hSts=HPDF_Page_SetCMYKFill(	psPage,
						            (HPDF_REAL) psColor->dComp[0],
									(HPDF_REAL) psColor->dComp[1],
									(HPDF_REAL) psColor->dComp[2],
									(HPDF_REAL) psColor->dComp[3]);
			break;
	}
	if (hSts!=HPDF_OK) ehError();
}

void _pdfColorStroke(HPDF_Page psPage,PWD_COLOR * psColor) {

	switch (psColor->enFormat) {

		case PWC_RGB:

			HPDF_Page_SetRGBStroke(	psPage,
									(HPDF_REAL) psColor->dComp[0],
									(HPDF_REAL) psColor->dComp[1],
									(HPDF_REAL) psColor->dComp[2]);
			break;

		case PWC_CMYK:

			HPDF_Page_SetCMYKStroke(psPage,
						            (HPDF_REAL) psColor->dComp[0],
									(HPDF_REAL) psColor->dComp[1],
									(HPDF_REAL) psColor->dComp[2],
									(HPDF_REAL) psColor->dComp[3]);
			break;
	}
}




static INT _LPdfTextAlign(INT iAllinea)
{
	
	switch (iAllinea) {
		default:
		case PDA_LEFT :
			return HPDF_TALIGN_LEFT ;
		break;
		case PDA_CENTER :
			return HPDF_TALIGN_CENTER ;
		break;
		case PDA_RIGHT :
			return HPDF_TALIGN_RIGHT ;
		break;
		case PDA_JUSTIFY :
			return HPDF_TALIGN_JUSTIFY ;
		break;

	}
}

/*
static void _pdfSetFont(HPDF_Page psPage,HPDF_Font psFont,PWD_VAL dFontAlt,HPDF_REAL *pdCharOffset) {

	PWD_VAL dAscent,dDescent,dFontSum,dFontAltCalc;
	PWD_VAL dXHeight,dCap;
	dCap=(PWD_VAL) HPDF_Font_GetCapHeight(psFont);
	dAscent	=(PWD_VAL) HPDF_Font_GetAscent(psFont);
	dDescent=(PWD_VAL) HPDF_Font_GetDescent(psFont); if (dDescent<0) dDescent*=-1;
	dXHeight=(PWD_VAL) HPDF_Font_GetXHeight(psFont);
	dFontSum=dAscent+dDescent;//+dXHeight;
	// dFontAlt:dFontSum=x:dAscent;
	dFontAltCalc=((dFontAlt*dAscent)/dFontSum);

	// dFontAlt:dFontSum=x:dXHeight;
	*pdCharOffset=(HPDF_REAL) ((dFontAlt*dDescent)/dFontSum);
	HPDF_Page_SetFontAndSize(	psPage, 
								psFont,//arsPdfFont[psText->idxFont], 
								(HPDF_REAL) dFontAltCalc);//pwdUm(PUM_UM_PT,psText->umCharHeight) );
}
*/

static void _pdfSetFont(HPDF_Page psPage,
						PDO_TEXT *	psText,
						HPDF_REAL * pdBaseLineOffset) {
//						HPDF_Font psFont,PWD_VAL dFontAlt,HPDF_REAL * pdBaseLineOffset) {

	PWD_VAL dAscent,dDescent,dFontPdf,dFontAltCalc;
	PWD_VAL dXHeight,dCap,dFontWin;
	HPDF_REAL drBaseLine;
	HPDF_STATUS hStatus;
	HPDF_Font psFont=_sPower.arsFont[psText->idxFont].pVoid;
	HPDF_REAL dFontAlt;

//_sPower.arsFont[psText->idxFont].pVoid,
										//pwdUmTo(psText->umCharHeight,PUM_PT),

	dFontAlt=(HPDF_REAL) pwdUmTo(psText->umCharHeight,PUM_PT);
	if (dFontAlt<0) dFontAlt=-dFontAlt; // Probabilmente in baseline
	

		dCap=(PWD_VAL) HPDF_Font_GetCapHeight(psFont);
		dAscent	=(PWD_VAL) HPDF_Font_GetAscent(psFont);
		dDescent=(PWD_VAL) HPDF_Font_GetDescent(psFont); if (dDescent<0) dDescent*=-1;
		dXHeight=(PWD_VAL) HPDF_Font_GetXHeight(psFont);

		// dFontAlt:dFontSum=x:dAscent;
		dFontWin=1000;		// Altezza intesa da Windows è Totale = 1000
		dFontPdf=dAscent;	// Altezza Pdf: intende solo parte ascendente del carattere

		if ((psText->enAlign&DPL_HEIGHT_TYPO)) 
			{
	 			dFontAltCalc=dFontAlt;
				// *pdBaseLineOffset=drBaseLine;
			}
			else
			{
				// Calcolo proporzionalmente l'altezza "logica" del font
				// dFontAlt:dFontWin=x:dFontPdf
				dFontAltCalc=((dFontAlt*dFontPdf)/dFontWin);

				// Calcolo la "baseline" (ritorno la parte discendente)
				// dDescent:1000=x:dFontAltCalc;
				// drBaseLine=(HPDF_REAL) ((dFontAltCalc*dDescent)/1000);
				
			}

		drBaseLine=(HPDF_REAL) ((dFontAltCalc*dDescent)/1000);
		*pdBaseLineOffset=drBaseLine;


	if (!_sPower.iVer) {
		hStatus=HPDF_Page_SetFontAndSize(	psPage, 
									psFont, // arsPdfFont[psText->idxFont], 
									(HPDF_REAL) dFontAltCalc);//dFontAltCalc);// pwdUm(PUM_UM_PT,psText->umCharHeight) );
	}
		else
		hStatus=HPDF_Page_SetFontAndSize(	psPage, 
									psFont, // arsPdfFont[psText->idxFont], 
 									(HPDF_REAL) dFontAlt);//dFontAltCalc);// pwdUm(PUM_UM_PT,psText->umCharHeight) );
/*
	if (hStatus!=HPDF_OK) {
		printf("qui");
	}
	*/
}


static BOOL _PdfChooseFile(CHAR *pszFile) {

    OPENFILENAME ofn;

	static CHAR szFilter[]= "Adobe Pdf (*.pdf)\0*.pdf\0" \
							"Tutti i file (*.*)\0*.*\0\0";
	CHAR szFile[_MAX_PATH];
	strcpy(szFile,pszFile);
	_(ofn);

	ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = WindowNow();
    ofn.hInstance         = sys.EhWinInstance;
    ofn.lpstrFilter       = szFilter;
	ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = szFile; // File che si vogliono indicare
    ofn.nMaxFile          = _MAX_PATH; // Grandezza del Buffer
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;//Appoggio.Path;
    ofn.lpstrTitle        = "Nome del file pdf";
    ofn.nFileOffset       = 0;//strlen(szFile)+1; // Offset dei file
    ofn.nFileExtension    = 0;//"\"*.*\"";
    ofn.lpstrDefExt       = NULL;
    ofn.Flags             = OFN_EXPLORER |    // Abilito il tipo Explorer
							OFN_HIDEREADONLY;// |    // Nascondo Read only
							 //OFN_SHOWHELP | 
		                     //OFN_NOVALIDATE|
							//OFN_CREATEPROMPT;
    if (GetSaveFileName(&ofn)) 
	{
		BYTE *pExt;
		strcpy(pszFile,szFile);
		pExt=fileExt(pszFile);
		if (strEmpty(pExt)) strcat(pszFile,".pdf");
		return TRUE;
	}

	return FALSE;
}


static void _pdfBox(HPDF_Page psPage,PWD_RECT * prumArea,PWD_COLOR colPen,PWD_VAL umPenWidth) {

	PWD_VAL rumWidth,rumHeight;
	rumWidth=prumArea->right-prumArea->left;
	rumHeight=prumArea->top-prumArea->bottom;

	HPDF_Page_SetLineWidth(psPage, (HPDF_REAL) pwdUmTo(umPenWidth,PUM_PT));
	_pdfColorStroke(psPage,&colPen);

	HPDF_Page_Rectangle(psPage,
						(HPDF_REAL) prumArea->left,
						(HPDF_REAL) prumArea->bottom,
						(HPDF_REAL) rumWidth,
						(HPDF_REAL) rumHeight);

	HPDF_Page_Stroke(psPage); 


}

static void _pdfLine(HPDF_Page psPage,HPDF_RECT * prumArea,PWD_COLOR colPen,PWD_VAL umPenWidth) {

	_pdfColorStroke(psPage,&colPen);
	HPDF_Page_SetDash(psPage, NULL, 0, 0);
	HPDF_Page_SetLineWidth(psPage,(HPDF_REAL) umPenWidth);
	HPDF_Page_SetLineCap(psPage,HPDF_BUTT_END);
	HPDF_Page_MoveTo	(psPage,
						(HPDF_REAL) prumArea->left,
						(HPDF_REAL) prumArea->top);

	HPDF_Page_LineTo	(psPage,
						(HPDF_REAL) prumArea->right,
						(HPDF_REAL) prumArea->bottom);

	HPDF_Page_Stroke(psPage); 

}

static void _pointToReal(	PWD_SIZE	sumPage,
							PWD_VAL		umX,
							PWD_VAL		umY,
							HPDF_REAL	*	piX,
							HPDF_REAL	*	piY) {
/*
	INT x,y;
	x=(INT) pwdUm(_DTXD,umX);	y=(INT) pwdUm(_DTYD,umY);

	if (_sPd.bVirtual) {
		x+=_sPd.recPreviewPage.left;
		y+=_sPd.recPreviewPage.top;
	}

	*piX=x;	*piY=y;
	*/
	*piX=(HPDF_REAL) pwdUmTo(umX,PUM_PT);
	*piY=(HPDF_REAL) (sumPage.cy-pwdUmTo(umY,PUM_PT));

}

static void _pdfSetPenBrush(HPDF_Page psPage,PWD_PB * psPenBrush) {

//	HPDF_Page_SetLineWidth(psPage, (HPDF_REAL) pwdUm(PUM_UM_PT,psPenBrush->umPenWidth));
//	if (psPenBrush->colPen.dAlpha&&psPenBrush->umPenWidth) _pdfColorStroke(psPage,&psPenBrush->colPen);						

	// Brush

	if (psPenBrush->colBrush.dAlpha) _pdfColorFill(psPage,&psPenBrush->colBrush);

	// Pen
	// PenBrush

	if (psPenBrush->colPen.dAlpha&&psPenBrush->umPenWidth) {
		HPDF_Page_SetLineWidth(psPage, (HPDF_REAL) pwdUmTo(psPenBrush->umPenWidth,PUM_PT));
		_pdfColorStroke(psPage,&psPenBrush->colPen);
	} else {
		HPDF_Page_SetLineWidth(psPage, (HPDF_REAL) 0);
	}

}

static void _pdfFillStroke(HPDF_Page psPage,PWD_PB * psPenBrush) {

	BOOL bStroke=(psPenBrush->colPen.dAlpha&&psPenBrush->umPenWidth)?true:false;
	BOOL bFill=psPenBrush->colBrush.dAlpha?true:false;
	if (bStroke&&bFill)
		HPDF_Page_FillStroke(psPage);
	else if (bFill)
		HPDF_Page_Fill(psPage); 
	else if (bStroke) 
		HPDF_Page_Stroke(psPage);
}

//
//  _pdfFontLoad() > Cerce il file del font per allegarlo al PDF
//
static void  _pdfFontLoad(CHAR * pszPdfEncode) {

	INT a;
	CHAR		szFontFullPath[500];
	BOOL		bEmbedded;
	HPDF_Font	psDefPdfFont;

	psDefPdfFont = HPDF_GetFont(_sPd.pdfDoc,"Helvetica",pszPdfEncode); // font di default

	for (a=0;a<_sPower.dmiFont.Num;a++)//ARLen(_sPower.arFontFace);a++)
	{
		PWD_FONT sPwdFont;

		DMIRead(&_sPower.dmiFont,a,&sPwdFont);

		//
		// devo associare l' handle del font all' indice della dmi
		//
		*szFontFullPath=0;
		bEmbedded=HPDF_FALSE;
		if (_fontFileSearch(&sPwdFont,szFontFullPath,&bEmbedded))
		{	
			BOOL bFound=FALSE;
			if (!_sPower.arsFont[a].pVoid)
			{
				CHAR *pszFontTmp=NULL;
				INT x;
				BOOL bFound=false;
				for (x=0;x<_sPower.dmiFont.Num;x++) {
					
					if (!strCaseCmp(_sPower.arsFont[x].szFontFullPath,szFontFullPath)) {
						
						_sPower.arsFont[a].pVoid=_sPower.arsFont[x].pVoid;
						_sPower.arsFont[a].bFontClone=true;
						bFound=true;
						break;
					}

				}
				if (!bFound) {
					lstPushf(_sPower.lstPdfReport,"importo %s (%s) ...",sPwdFont.szFontFace,szFontFullPath);
					pszFontTmp = (CHAR *) HPDF_LoadTTFontFromFile(_sPd.pdfDoc, szFontFullPath, bEmbedded);
					_sPower.arsFont[a].pVoid = HPDF_GetFont(_sPd.pdfDoc, pszFontTmp, pszPdfEncode);  //"90ms-RKSJ-H"); 
					if (!_sPower.arsFont[a].pVoid) 
						ehError();
					strcpy(_sPower.arsFont[a].szFontFullPath,szFontFullPath);
				}


			}
/*
			for (b=0;b<_sPd.dmiPdfFont.Num;b++)
			{
				_(sPFont);
				DMIRead(&_sPd.dmiPdfFont,b,&sPFont);

				if (!strcmp(sPFont.szFontFullPath,szFontFullPath)) { // trovato !!
					bFound=true; 
					break;
				} 

			}

			//
			// Aggiungo il font al file PDF (se non l'ho gia fatto)
			//
			if (!bFound) { 

				CHAR *pszFontTmp=NULL;
				
				_(sPFont);
				pszFontTmp = (CHAR *) HPDF_LoadTTFontFromFile (_sPd.pdfDoc, szFontFullPath, HPDF_FALSE);
				sPFont.psPdfFont = HPDF_GetFont (_sPd.pdfDoc, pszFontTmp, pszPdfEncode);  //"90ms-RKSJ-H"); 
				lstPushf(_sPower.lstPdfReport,"importo %s (%s) ...",sPwdFont.szFontFace,szFontFullPath);
				strcpy(sPFont.szFontFullPath,szFontFullPath);
				DMIAppendDyn(&_sPd.dmiPdfFont,&sPFont);
				
			}

			arsPdfFont[a]=sPFont.psPdfFont;
			*/
		} 
		else // font di default

			_sPower.arsFont[a].pVoid=psDefPdfFont;
	}

//	return arsPdfFont;
}



//
// _pdfBuilder()
//
static BOOL _pdfBuilder(INT PageStart,INT PageEnd)
{
	INT nPage;
	HPDF_Page psPage;
	INT a;
	PWD_DRAW sItemDraw;
	PDO_TEXT *	psText;
	PDO_PATH *	psPath;
	PWD_PTHE *	psEle;
	EH_LST		lstPath;
//	BYTE *pb;
	CHAR *pszFontFile=NULL;
	CHAR *pszFontName=NULL;
	CHAR *pszFontFace=NULL;

	PWD_SIZE sumPage;
	HPDF_UINT uChar;
	PWD_VAL rumHeight,rumWidth;
	WCHAR *pwcText;

	PDO_BOXLINE *psBox;
//	HPDF_Image   hPdfimage;
	PDO_IMAGE	*psImage;
	HPDF_ColorSpace    hPdefcolor_space;
	HPDF_REAL	drValue;
	IMGHEADER *	psImg;
	HPDF_REAL	yPos;
	CHAR		szFileOutput[500];

	BYTE *	pszText;
	BYTE *	pszPdfEncode;
	BYTE *	pbTableEncode;
	HPDF_REAL dBaseLineOffset;

	EH_IMG himFile;

	typedef struct {

		EH_IMG		imgPwd;
		HPDF_Image	pdfImage;
	
	} S_IMG_PDF;

	EH_LST lstImgPdf;


	//
	// Nome del file PDF generato
	//

	*szFileOutput=0;
	if (!strEmpty(_sPower.pszPdfFileName)) strcpy(szFileOutput,_sPower.pszPdfFileName);
/*
#ifdef _DEBUG
	strcpy(szFileOutput,"c:\\test.pdf");
#endif
*/
#ifndef EH_CONSOLE
	if (_sPower.bPdfChooseFile||!*szFileOutput) {
		if (!_PdfChooseFile(szFileOutput)) return TRUE;
	}
#endif

	if (strEmpty(szFileOutput)) {
		printf("File PDF non indicato.\7" CRLF);
		return TRUE;
	}

	pbTableEncode = ehAlloc(0xFFFF);
	GetWindowsDirectory(_sPower.szWinDir,sizeof(_sPower.szWinDir)-1);
	_sPower.lstPdfReport=lstNew(); // lst di report

#ifndef EH_CONSOLE
	MouseCursorWait();
#endif

	sumPage.cx=pwdUmTo(_sPower.sumPaper.cx,PUM_PT);
	sumPage.cy=pwdUmTo(_sPower.sumPaper.cy,PUM_PT);
	lstImgPdf=lstCreate(sizeof(S_IMG_PDF));

	_sPd.pdfDoc= HPDF_New(_pdfErrorHandler, NULL); if (!_sPd.pdfDoc) ehError();
	pszPdfEncode=_sPower.pszPdfEncode; if (!pszPdfEncode) pszPdfEncode="ISO8859-16";
	if (strCmp(pszPdfEncode,"UTF-8")) {
		_pdfMapBuilder(pszPdfEncode); // Crea la mappa di ricodifica ....
	}
	
	HPDF_SetCompressionMode(_sPd.pdfDoc, HPDF_COMP_ALL);

	//
	// FONT 
	// - Enumerazione usati nel documento
	// - Rintraccio il font come file
	//
	
	_fontEnumeration(0);			// Enumero i font presenti nel documento in (riempo _sPower.dmiFont)
	_pdfFontLoad(pszPdfEncode);		// Alloco i fonts (se presenti) inserendoli nel documento  (riempo _sPd.dmiPdfFont)
	

	//	ARPrint(arFonts);
	//
	//	Loop di rendering (esportazione) sulle pagine
	//
 	for (nPage=PageStart;nPage<=PageEnd;nPage++)
	{
		psPage = HPDF_AddPage (_sPd.pdfDoc);
		HPDF_Page_SetHeight (psPage, (HPDF_REAL) sumPage.cy);
		HPDF_Page_SetWidth (psPage, (HPDF_REAL) sumPage.cx);
		HPDF_SetCurrentEncoder(_sPd.pdfDoc, pszPdfEncode);
		HPDF_Page_SetTextRenderingMode(psPage,HPDF_FILL);
		_(sItemDraw);

		for (a=_sPd.ariPageIndex[nPage];a<_sPd.nItems;a++) 
		{
			HPDF_RECT pdfRect;
			HPDF_SIZE pdfSize;
			HPDF_POINT pdfPoint;

			if (_sPd.arsItem[a]->iPage!=(nPage+1)) break;

			//
			// Calcolo le nuove coordinate / dimensioni
			//
			
			_pointToReal(sumPage,
						 _sPd.arsItem[a]->rumObj.left,
						 _sPd.arsItem[a]->rumObj.top,
						 &pdfRect.left,
						 &pdfRect.top);

			_pointToReal(sumPage,
						 _sPd.arsItem[a]->rumObj.right,
						 _sPd.arsItem[a]->rumObj.bottom,
						 &pdfRect.right,
						 &pdfRect.bottom);

//			rumArea.left=pwdUm(PUM_UM_PT,_sPd.arsItem[a]->rumObj.left);
//			rumArea.right=pwdUm(PUM_UM_PT,_sPd.arsItem[a]->rumObj.right);
//			rumArea.top=sumPage.cy-pwdUm(PUM_UM_PT,_sPd.arsItem[a]->rumObj.top);
//			rumArea.bottom=sumPage.cy-pwdUm(PUM_UM_PT,_sPd.arsItem[a]->rumObj.bottom);

			pdfSize.cx=pdfRect.right-pdfRect.left;
			pdfSize.cy=pdfRect.bottom-pdfRect.top;
//			pwdSizeCalc(&sumArea,&rumArea); 
			pdfSize.cy*=-1;

			sItemDraw.psItem=_sPd.arsItem[a]; 
			//pb=(BYTE *) sItemDraw.psItem;
			//sItemDraw.psObj=(pb+sizeof(PWD_ITEM)); 

			switch (_sPd.arsItem[a]->enType)
			{
				//
				// TESTO
				//

				case PDT_TEXT:	

					psText=pwdGetObj(sItemDraw.psItem);
					if (strEmpty(psText->pszText)) break;	

					pwcText=_strTextDecode(psText->pszText);  pszText=_pdfStrDecode(pwcText); ehFree(pwcText);

					HPDF_Page_BeginText(psPage); 
					_pdfSetFont(	psPage,psText,
									&dBaseLineOffset);

					_pdfColorFill(psPage,&psText->colText);
					HPDF_Page_SetCharSpace(psPage, (HPDF_REAL) pwdUmTo(psText->umExtraCharSpace,PUM_PT));

					drValue = HPDF_Page_TextWidth (psPage, pszText); // Calcolo Larghezza del testo
					
					_pointToReal(sumPage,
								 psText->umX,
								 psText->umY,
								 &pdfPoint.x,
								 &pdfPoint.y);

					//drValue = HPDF_Page_TextWidth (psPage, psText->pszText);
					switch (psText->enAlign&0xf)
					{
						case PDA_RIGHT:
//								rumArea.left=rumArea.right-drValue;
//								pdfRect.left=pdfRect.right-drValue;
								pdfPoint.x-=drValue;
								break;

						case PDA_CENTER:
								//pdfRect.left+=(pdfSize.cx/2)-(drValue/2);
								pdfPoint.x-=(drValue/2);
								break;

						default:
						case PDA_LEFT:
						case PDA_JUSTIFY:
								break;
					}



					if (psText->enAlign&PDA_BASELINE) {
					


					} else {
					
						pdfPoint.y = pdfRect.bottom + (HPDF_REAL) (dBaseLineOffset);

					}
					
					if (psText->enAlign&DPL_HEIGHT_TYPO) yPos=(HPDF_REAL) (pdfRect.top);
					HPDF_Page_MoveTextPos(psPage,
								pdfPoint.x,
								pdfPoint.y); // + perché va SU !
					HPDF_Page_ShowText(psPage,pszText);
					HPDF_Page_EndText(psPage);

					if (psText->enStyles&STYLE_UNDERLINE) {
					
						HPDF_REAL dLine=(HPDF_REAL) 1.0;//pwdUmTo(pwdUm(PUM_PT,1),PUM_PT);
						pdfRect.top=pdfRect.bottom+dBaseLineOffset-dLine*2;
						pdfRect.bottom=pdfRect.top;
						pdfRect.right=pdfRect.left+drValue;
						_pdfLine(psPage,&pdfRect,psText->colText,dLine/2);
					}
					ehFree(pszText);
					break;
				
				//
				// TESTO in BOX
				//
//				case PDT_TEXT:	
				case PDT_TEXTBOX:	

						psText=pwdGetObj(sItemDraw.psItem);
						if (strEmpty(psText->pszText)) break;	
						
						pwcText=_strTextDecode(psText->pszText);
						pszText = _pdfStrDecode(pwcText);
						ehFree(pwcText);
						
						HPDF_Page_BeginText(psPage);

						_pdfSetFont(	psPage,psText,
										//arsPdfFont[psText->idxFont],
										//_sPower.arsFont[psText->idxFont].pVoid,
										//pwdUmTo(psText->umCharHeight,PUM_PT),
										&dBaseLineOffset);
/*
#ifdef _DEBUG
						_pdfBox(psPage,&rumArea,pwdColor(RGB(255,0,0)),.1);
#endif
*/
						// HPDF_Page_BeginText(psPage);
						

						_pdfColorFill(psPage,&psText->colText);
						HPDF_Page_SetCharSpace(psPage, (HPDF_REAL) pwdUmTo(psText->umExtraCharSpace,PUM_PT));
						drValue = HPDF_Page_TextWidth (psPage, pszText);
				
						HPDF_Page_TextRect (psPage,
											(HPDF_REAL) pdfRect.left,
											(HPDF_REAL) pdfRect.top+dBaseLineOffset,
											(HPDF_REAL) pdfRect.right,
											(HPDF_REAL) pdfRect.bottom+dBaseLineOffset,
											pszText,
											_LPdfTextAlign(psText->enAlign),
											&uChar);

						HPDF_Page_EndText (psPage);
						
						ehFree(pszText);

					break;

				//
				// Disegno ------------------------------------------------------------------
				//


				//
				// LINEA
				//

				case PDT_LINE:		//_drawLine(&sItemDraw); 
						
						psBox=pwdGetObj(sItemDraw.psItem);
						_pdfSetPenBrush(psPage,&psBox->sPenBrush);

						HPDF_Page_MoveTo(psPage, (HPDF_REAL) pdfRect.left, (HPDF_REAL) pdfRect.top);
						HPDF_Page_LineTo(psPage, (HPDF_REAL) pdfRect.right, (HPDF_REAL) pdfRect.bottom);
						
						_pdfFillStroke(psPage,&psBox->sPenBrush);

						break;

				//
				// RETTANGOLO
				//
				case PDT_RECT:		//_drawRect(&sItemDraw);
					
					psBox=pwdGetObj(sItemDraw.psItem);
					if (psBox->sPenBrush.colPen.dAlpha==0.0&&
						psBox->sPenBrush.colBrush.dAlpha==0.0) 
					{
						break;
					
					}

					rumWidth=pdfRect.right-pdfRect.left;
					rumHeight=pdfRect.top-pdfRect.bottom;

					_pdfSetPenBrush(psPage,&psBox->sPenBrush);

					if (psBox->sumRound.cx||psBox->sumRound.cy) {

						HPDF_Page_SetDash(psPage, NULL, 0, 0);
						HPDF_Page_SetLineCap(psPage,HPDF_BUTT_END);

						// Curva Left/Top
						HPDF_Page_MoveTo	(psPage,
											(HPDF_REAL) pdfRect.left,
											(HPDF_REAL) (pdfRect.top-psBox->sumRound.cy));

						HPDF_Page_CurveTo  (psPage,
											(HPDF_REAL) pdfRect.left,
											(HPDF_REAL) (pdfRect.top-psBox->sumRound.cy/2),
											(HPDF_REAL) (pdfRect.left+psBox->sumRound.cx/2),
											(HPDF_REAL) pdfRect.top,
											(HPDF_REAL) (pdfRect.left+psBox->sumRound.cx),
											(HPDF_REAL) pdfRect.top);

						// Line Top
						HPDF_Page_LineTo	(psPage,
											(HPDF_REAL) (pdfRect.right-psBox->sumRound.cx),
											(HPDF_REAL) pdfRect.top);
						// Curva Right/Top
						HPDF_Page_CurveTo  (psPage,
											(HPDF_REAL) (pdfRect.right-psBox->sumRound.cx/2),
											(HPDF_REAL) pdfRect.top,
											(HPDF_REAL) pdfRect.right,
											(HPDF_REAL) (pdfRect.top-psBox->sumRound.cy/2),
											(HPDF_REAL) pdfRect.right,
											(HPDF_REAL) (pdfRect.top-psBox->sumRound.cy));

						// Line Right
						HPDF_Page_LineTo	(psPage,
											(HPDF_REAL) pdfRect.right,
											(HPDF_REAL) (pdfRect.bottom+psBox->sumRound.cy));

						// Curva Right/Bottom
						HPDF_Page_CurveTo  (psPage,
											(HPDF_REAL) pdfRect.right,
											(HPDF_REAL) (pdfRect.bottom+psBox->sumRound.cy/2),
											(HPDF_REAL) (pdfRect.right-psBox->sumRound.cx/2),
											(HPDF_REAL) (pdfRect.bottom),
											(HPDF_REAL) (pdfRect.right-psBox->sumRound.cx),
											(HPDF_REAL) pdfRect.bottom);

						// Line Bottom
						HPDF_Page_LineTo	(psPage,
											(HPDF_REAL) (pdfRect.left+psBox->sumRound.cx),
											(HPDF_REAL) pdfRect.bottom);

						// Curva left/Bottom
						HPDF_Page_CurveTo  (psPage,
											(HPDF_REAL) (pdfRect.left+psBox->sumRound.cx/2),
											(HPDF_REAL) pdfRect.bottom,
											(HPDF_REAL) pdfRect.left,
											(HPDF_REAL) (pdfRect.bottom+psBox->sumRound.cy/2),
											(HPDF_REAL) pdfRect.left,
											(HPDF_REAL) (pdfRect.bottom+psBox->sumRound.cy));

						// Line Left
						HPDF_Page_LineTo	(psPage,
											(HPDF_REAL) pdfRect.left,
											(HPDF_REAL) (pdfRect.top-psBox->sumRound.cy));

					}
					else {

						HPDF_Page_Rectangle(psPage,
											(HPDF_REAL) pdfRect.left,
											(HPDF_REAL) pdfRect.bottom,
											(HPDF_REAL) rumWidth,
											(HPDF_REAL) rumHeight);
					}

					_pdfFillStroke(psPage,&psBox->sPenBrush);
					break;

				//
				// PATH
				//
				case PDT_PATH:

					psPath=pwdGetObj(sItemDraw.psItem);
					_pdfSetPenBrush(psPage,&psPath->sPenBrush); // Preset pen/Brush
					lstPath=_pathCreate(psPath);
					for (lstLoop(lstPath,psEle)) {

						HPDF_REAL umX,umY;
						PWD_POINT * pumPoint=psEle->psData;
					
						switch (psEle->enType) {
						
							case PHT_MOVETO:
								_pointToReal(sumPage,
											 pumPoint->x,
											 pumPoint->y,
											 &umX,
											 &umY);
								HPDF_Page_MoveTo(psPage, (HPDF_REAL) umX, (HPDF_REAL) umY);
								break;

							case PHT_LINETO:

								_pointToReal(sumPage,
											 pumPoint->x,
											 pumPoint->y,
											 &umX,
											 &umY);
								HPDF_Page_LineTo(psPage, (HPDF_REAL) umX, (HPDF_REAL) umY );
								break;

							case PHT_POLYBEZIERTO:

								{
									PWD_BEZIER * pumBezier=psEle->psData;
									INT a;
									
									if (pumBezier->cCount==3) {
										HPDF_POINT * arsPoint=ehAlloc(pumBezier->cCount*sizeof(HPDF_POINT));
										for (a=0;a<(INT) pumBezier->cCount;a++) {

											_pointToReal(	sumPage,
															pumBezier->arsPoint[a].x,
															pumBezier->arsPoint[a].y,
															&arsPoint[a].x,
															&arsPoint[a].y);

										}
										HPDF_Page_CurveTo  (psPage,
															arsPoint[0].x,
															arsPoint[0].y,
															arsPoint[1].x,
															arsPoint[1].y,
															arsPoint[2].x,
															arsPoint[2].y);

										//PolyBezierTo(psDraw->hDC,arsPoint,pumBezier->cCount);
										ehFree(arsPoint);
									}
								}
								break;

							case PHT_CLOSEFIGURE:
								HPDF_Page_ClosePath(psPage);
								//CloseFigure(psDraw->hDC);
								break;

							default:
								ehError();
						}
					}
//					EndPath(psDraw->hDC);

					_pdfFillStroke(psPage,&psPath->sPenBrush);
					_pathDestroy(lstPath);
					// printf("qui");
					break;

				//
				// Immagine (formato easyHand)
				//

				case PDT_EHIMG:

					
					psImage=pwdGetObj(sItemDraw.psItem);
					if (psImage->hImage) {
				
						S_IMG_PDF * psImgPdf=NULL;

						//
						// Cerco se l'ho già letta
						//
						for (lstLoop(lstImgPdf,psImgPdf)) {
							if (psImgPdf->imgPwd==psImage->hImage) break;
						}
						
						if (!psImgPdf) {

							S_IMG_PDF sImg;
							_(sImg);
							sImg.imgPwd=psImage->hImage;

							psImg=memoLock(psImage->hImage); // brutto ma temporaneo

							//
							// Se JPG > inserisco il file originale
							//
							if (psImg->enType==IMG_JPEG&&
								*psImg->utfFullFileName) {
									sImg.pdfImage=HPDF_LoadJpegImageFromFile(_sPd.pdfDoc,psImg->utfFullFileName);
									if (!sImg.pdfImage) {
										ehExit("Errore in caricamento nel PDF: %s",psImg->utfFullFileName);
									}
									// break;
							}
							//
							// PNG > Da fare e da provare
							//
							else if (psImg->enType==IMG_PNG&&
									*psImg->utfFullFileName) {
									sImg.pdfImage=HPDF_LoadPngImageFromFile(_sPd.pdfDoc,psImg->utfFullFileName);
									if (!sImg.pdfImage) {
										ehExit("Errore in caricamento nel PDF: %s",psImg->utfFullFileName);
									}
									// break;
							}

							//
							// Altro tipo > inserisco il bitmap non compresso
							//
							else 
							{
								switch(psImg->enPixelType) {
									case IMG_PIXEL_RGB:
									case IMG_PIXEL_BGR:
											hPdefcolor_space=HPDF_CS_DEVICE_RGB;
										break;
									case IMG_PIXEL_CMYK:
											hPdefcolor_space=HPDF_CS_DEVICE_CMYK;
										break;
									case IMG_PIXEL_GRAYSCALE:
											hPdefcolor_space=HPDF_CS_DEVICE_GRAY;
										break;
									
									default:
											ehError();
										break;
								}
					
								sImg.pdfImage = HPDF_LoadRawImageFromMem  (_sPd.pdfDoc,
																	   psImg->pbImage,
																	   psImg->bmiHeader.biWidth,
																	   psImg->bmiHeader.biHeight,
																	   hPdefcolor_space,
																	   8);
								// 
								// DAFARE: Bisogna rovesciare l'immagine SOTTOSOPRA
								//
							}

//							if (!sImg.pdfImage) ehError();
							psImgPdf=lstPush(lstImgPdf,&sImg);	

						}
						else {
						
							psImg=memoLock(psImage->hImage); 

						}
						if (!psImgPdf->pdfImage) ehError();

						HPDF_Page_DrawImage  (psPage,
											  psImgPdf->pdfImage,//hPdfimage,
											  pdfRect.left,
											  (pdfRect.top-pdfSize.cy),
											  pdfSize.cx,//Img->bmiHeader.biWidth,
											  pdfSize.cy);

						memoUnlock(psImage->hImage);
					}
					else
					{
						//
						// Converto BMP in PNG 
						//
						HPDF_Image   hPdfimage;
						CHAR szTempPngFile[500];

						if (!BMPReadFile(psImage->pszFileTemp, &himFile) ) ehExit("errore nel caricamento del bitmap temporaneo");
									
						
						fileTempName( NULL, "pwd", szTempPngFile,  FALSE ); 
						if (!PNGSaveFile(szTempPngFile, himFile,10)) ehExit("errore salvataggio bitmap temporaneo!");
	
						
						hPdfimage = HPDF_LoadPngImageFromFile (_sPd.pdfDoc, szTempPngFile);
						
						HPDF_Page_DrawImage  (psPage,
											  hPdfimage,
											  (HPDF_REAL) pdfRect.left,
											  (HPDF_REAL) (pdfRect.top-pdfSize.cy),
											  (HPDF_REAL) pdfSize.cx,//Img->bmiHeader.biWidth,
											  (HPDF_REAL) pdfSize.cy);
					
						fileRemove(szTempPngFile);
						IMGDestroy(himFile);
					}
					
					break;

				case PDT_BITMAP:	//_drawImage(&sItemDraw);
					
					psImage=pwdGetObj(sItemDraw.psItem);
			
					if (psImage->hBitmap) { // da finire / vedere
	
						// Creo BMP al volo
						CHAR szTempPngFile[500];
						SIZE sizImageSorg,sizImageDest;
						INT HdlTemp;
						IMGHEADER *psImg;

						// Calcolo dimensione in UM (da fare)
						HdlTemp = BitmapToImg(psImage->hBitmap,false,NULL,0);
						psImg=memoLock(HdlTemp);
						sizImageSorg.cx=psImg->bmiHeader.biWidth;
						sizImageSorg.cy=psImg->bmiHeader.biHeight;
						memoUnlock(HdlTemp); 

						//sizImageDest.cx= (LONG) pwdUm(PUM_DTX,psumSize->cx);
						//sizImageDest.cy= (LONG) pwdUm(PUM_DTY,psumSize->cy);

						sizImageDest.cx= (LONG) pwdUm(PUM_DTX,pdfSize.cx);
						sizImageDest.cy= (LONG) pwdUm(PUM_DTY,pdfSize.cy);

//						HdlTemp = BitmapToImg(psImage->hBitmap,NULL);
						_imageResampleSave(HdlTemp, sizImageSorg, sizImageDest,szTempPngFile);

						remove(szTempPngFile);	// rimuovo il bitmap	
					
					}  // da finire / vedere
					else // Leggo BMP
					{
						HPDF_Image   hPdfimage;
						CHAR szTempPngFile[500];

						// Converto BMP in PNG e importo il PNG (DA FARE)
						if (!BMPReadFile(psImage->pszFileTemp, &himFile) ) ehExit("errore nel caricamento del bitmap temporaneo");
						
						
						fileTempName( NULL, "pwd", szTempPngFile,  FALSE ); 
						if (!PNGSaveFile(szTempPngFile, himFile,10)) ehExit("errore salvataggio bitmap temporaneo!");
							
						hPdfimage = HPDF_LoadPngImageFromFile (_sPd.pdfDoc, szTempPngFile);
						HPDF_Page_DrawImage  (psPage,
											  hPdfimage,
											  (HPDF_REAL) pdfRect.left,
											  (HPDF_REAL) (pdfRect.top-pdfSize.cy),
											  (HPDF_REAL) pdfSize.cx,//Img->bmiHeader.biWidth,
											  (HPDF_REAL) pdfSize.cy);
					
						fileRemove(szTempPngFile);					
						IMGDestroy(himFile);

					}

					// Converto BMP in PNG e import il PNG

					// Cancello BMP creato
					//if (psImage->hBitmap) {}
					break;

				case PDT_IMAGELIST:

				psImage=pwdGetObj(sItemDraw.psItem);
#ifndef EH_CONSOLE
					if (psImage->himl) {
						// Creo BMP al volo
						IMAGEINFO sImageInfo;
						if (ImageList_GetImageInfo(psImage->himl,psImage->hImage,&sImageInfo))
						{
							INT hdlImage=BitmapToImg(sImageInfo.hbmImage,TRUE,&sImageInfo.rcImage,0);
							BMPSaveFile("temporaneo",hdlImage);
							memoFree(hdlImage,"temp");

						}
					} 
					else // Leggo BMP
					{
							
					}
#endif

					// Converto BMP in PNG e import il PNG

					// Cancello BMP creato
					if (psImage->himl) {}
					break;


					/*
							psImage=sItemDraw.pObj;

							prcClip->left=rumArea.left;
							prcClip->right=rumArea.right;
							prcClip->bottom=rumArea.bottom;
							prcClip->top=rumArea.top;


							HdlImage = BitmapToImg(psImage->hBitmap,prcClip);

							psImg=memoLock(HdlImage); 


							switch(psImg->enPixelType) {
								case IMG_PIXEL_RGB:
								case IMG_PIXEL_BGR:
										hPdefcolor_space=HPDF_CS_DEVICE_RGB;
									break;
								case IMG_PIXEL_CMYK:
										hPdefcolor_space=HPDF_CS_DEVICE_CMYK;
									break;
								case IMG_PIXEL_GRAYSCALE:
										hPdefcolor_space=HPDF_CS_DEVICE_GRAY;
									break;
								
								default:
										ehError();
									break;
							}
				
							hPdfimage = HPDF_LoadRawImageFromMem  (_sPd.pdfDoc,
																   psImg->pbImage,
																   psImg->bmiHeader.biWidth,
																   psImg->bmiHeader.biHeight,
																   hPdefcolor_space,
																   8);

							HPDF_Page_DrawImage  (psPage,
												  hPdfimage,
												  (HPDF_REAL) rumArea.left,
												  (HPDF_REAL) (rumArea.top-pdfSize.cy),
												  (HPDF_REAL) pdfSize.cx,
												  (HPDF_REAL) pdfSize.cy);

							memoUnlock(HdlImage); 
							
*/
//						break;
			}

		} // loop sugli elementi
	

	} // loop sulle pagine

	// save the document to a file 
	{
		EH_FILE * psFile=fileOpen(szFileOutput,FO_WRITE|FO_OPEN_EXCLUSIVE); 
		if (!psFile) ehExit("Il file %s non puo essere scritto",szFileOutput);
		fileClose(psFile);
		HPDF_SaveToFile(_sPd.pdfDoc, szFileOutput);
	}

	
	HPDF_Free(_sPd.pdfDoc);
	lstDestroy(lstImgPdf);

//	DMIClose(&_sPd.dmiPdfFont,"FontName");
#ifndef EH_CONSOLE
	MouseCursorDefault();
#endif

#ifdef _DEBUG
	_sPower.bPdfShow=true;
#endif

	ehFreePtrs(1,&pbTableEncode);
	if (_sPower.bPdfShow) ShellExecute(NULL,"open",szFileOutput,NULL,NULL,SW_NORMAL);
	return FALSE;
}


//
// _pdfStrDecode()
//
static CHAR * _pdfStrDecode(WCHAR *pwcText)
{
	CHAR *	pszRet,* pbDest;
	WCHAR * pwcSource=pwcText;

	pszRet=pbDest=ehAlloc((INT) wcslen(pwcText)+1); *pszRet=0;
	for (;*pwcText;pwcText++) {*pbDest++=_sPd.pbCharMap[*pwcText];}
	*pbDest=0;
	return pszRet;
}	


//
//	 _pdfMapBuilder() - Crea mappa caratteri
//
static void _pdfMapBuilder(CHAR *pszEncode) {

	typedef struct {
		BYTE	bCode;
		DWORD	dwCode;
	} ISO_ENCODE;


	ISO_ENCODE arsIso1[] = {
	 {0x01,	0x0001},
	 {0x02,	0x0002},
	 {0x03,	0x0003},
	 {0x04,	0x0004},
	 {0x05,	0x0005},
	 {0x06,	0x0006},
	 {0x07,	0x0007},
	 {0x08,	0x0008},
	 {0x09,	0x0009},
	 {0x0A,	0x000A},
	 {0x0B,	0x000B},
	 {0x0C,	0x000C},
	 {0x0D,	0x000D},
	 {0x0E,	0x000E},
	 {0x0F,	0x000F},
	 {0x10,	0x0010},
	 {0x11,	0x0011},
	 {0x12,	0x0012},
	 {0x13,	0x0013},
	 {0x14,	0x0014},
	 {0x15,	0x0015},
	 {0x16,	0x0016},
	 {0x17,	0x0017},
	 {0x18,	0x0018},
	 {0x19,	0x0019},
	 {0x1A,	0x001A},
	 {0x1B,	0x001B},
	 {0x1C,	0x001C},
	 {0x1D,	0x001D},
	 {0x1E,	0x001E},
	 {0x1F,	0x001F},
	 {0x20,	0x0020},
	 {0x21,	0x0021},
	 {0x22,	0x0022},
	 {0x23,	0x0023},
	 {0x24,	0x0024},
	 {0x25,	0x0025},
	 {0x26,	0x0026},
	 {0x27,	0x0027},
	 {0x28,	0x0028},
	 {0x29,	0x0029},
	 {0x2A,	0x002A},
	 {0x2B,	0x002B},
	 {0x2C,	0x002C},
	 {0x2D,	0x002D},
	 {0x2E,	0x002E},
	 {0x2F,	0x002F},
	 {0x30,	0x0030},
	 {0x31,	0x0031},
	 {0x32,	0x0032},
	 {0x33,	0x0033},
	 {0x34,	0x0034},
	 {0x35,	0x0035},
	 {0x36,	0x0036},
	 {0x37,	0x0037},
	 {0x38,	0x0038},
	 {0x39,	0x0039},
	 {0x3A,	0x003A},
	 {0x3B,	0x003B},
	 {0x3C,	0x003C},
	 {0x3D,	0x003D},
	 {0x3E,	0x003E},
	 {0x3F,	0x003F},
	 {0x40,	0x0040},
	 {0x41,	0x0041},
	 {0x42,	0x0042},
	 {0x43,	0x0043},
	 {0x44,	0x0044},
	 {0x45,	0x0045},
	 {0x46,	0x0046},
	 {0x47,	0x0047},
	 {0x48,	0x0048},
	 {0x49,	0x0049},
	 {0x4A,	0x004A},
	 {0x4B,	0x004B},
	 {0x4C,	0x004C},
	 {0x4D,	0x004D},
	 {0x4E,	0x004E},
	 {0x4F,	0x004F},
	 {0x50,	0x0050},
	 {0x51,	0x0051},
	 {0x52,	0x0052},
	 {0x53,	0x0053},
	 {0x54,	0x0054},
	 {0x55,	0x0055},
	 {0x56,	0x0056},
	 {0x57,	0x0057},
	 {0x58,	0x0058},
	 {0x59,	0x0059},
	 {0x5A,	0x005A},
	 {0x5B,	0x005B},
	 {0x5C,	0x005C},
	 {0x5D,	0x005D},
	 {0x5E,	0x005E},
	 {0x5F,	0x005F},
	 {0x60,	0x0060},
	 {0x61,	0x0061},
	 {0x62,	0x0062},
	 {0x63,	0x0063},
	 {0x64,	0x0064},
	 {0x65,	0x0065},
	 {0x66,	0x0066},
	 {0x67,	0x0067},
	 {0x68,	0x0068},
	 {0x69,	0x0069},
	 {0x6A,	0x006A},
	 {0x6B,	0x006B},
	 {0x6C,	0x006C},
	 {0x6D,	0x006D},
	 {0x6E,	0x006E},
	 {0x6F,	0x006F},
	 {0x70,	0x0070},
	 {0x71,	0x0071},
	 {0x72,	0x0072},
	 {0x73,	0x0073},
	 {0x74,	0x0074},
	 {0x75,	0x0075},
	 {0x76,	0x0076},
	 {0x77,	0x0077},
	 {0x78,	0x0078},
	 {0x79,	0x0079},
	 {0x7A,	0x007A},
	 {0x7B,	0x007B},
	 {0x7C,	0x007C},
	 {0x7D,	0x007D},
	 {0x7E,	0x007E},
	 {0x7F,	0x007F},
	 {0x80,	0x0080},
	 {0x81,	0x0081},
	 {0x82,	0x0082},
	 {0x83,	0x0083},
	 {0x84,	0x0084},
	 {0x85,	0x0085},
	 {0x86,	0x0086},
	 {0x87,	0x0087},
	 {0x88,	0x0088},
	 {0x89,	0x0089},
	 {0x8A,	0x008A},
	 {0x8B,	0x008B},
	 {0x8C,	0x008C},
	 {0x8D,	0x008D},
	 {0x8E,	0x008E},
	 {0x8F,	0x008F},
	 {0x90,	0x0090},
	 {0x91,	0x0091},
	 {0x92,	0x0092},
	 {0x93,	0x0093},
	 {0x94,	0x0094},
	 {0x95,	0x0095},
	 {0x96,	0x0096},
	 {0x97,	0x0097},
	 {0x98,	0x0098},
	 {0x99,	0x0099},
	 {0x9A,	0x009A},
	 {0x9B,	0x009B},
	 {0x9C,	0x009C},
	 {0x9D,	0x009D},
	 {0x9E,	0x009E},
	 {0x9F,	0x009F},
	 {0xA0,	0x00A0},
	 {0xA1,	0x00A1},
	 {0xA2,	0x00A2},
	 {0xA3,	0x00A3},
	 {0xA4,	0x00A4},
	 {0xA5,	0x00A5},
	 {0xA6,	0x00A6},
	 {0xA7,	0x00A7},
	 {0xA8,	0x00A8},
	 {0xA9,	0x00A9},
	 {0xAA,	0x00AA},
	 {0xAB,	0x00AB},
	 {0xAC,	0x00AC},
	 {0xAD,	0x00AD},
	 {0xAE,	0x00AE},
	 {0xAF,	0x00AF},
	 {0xB0,	0x00B0},
	 {0xB1,	0x00B1},
	 {0xB2,	0x00B2},
	 {0xB3,	0x00B3},
	 {0xB4,	0x00B4},
	 {0xB5,	0x00B5},
	 {0xB6,	0x00B6},
	 {0xB7,	0x00B7},
	 {0xB8,	0x00B8},
	 {0xB9,	0x00B9},
	 {0xBA,	0x00BA},
	 {0xBB,	0x00BB},
	 {0xBC,	0x00BC},
	 {0xBD,	0x00BD},
	 {0xBE,	0x00BE},
	 {0xBF,	0x00BF},
	 {0xC0,	0x00C0},
	 {0xC1,	0x00C1},
	 {0xC2,	0x00C2},
	 {0xC3,	0x00C3},
	 {0xC4,	0x00C4},
	 {0xC5,	0x00C5},
	 {0xC6,	0x00C6},
	 {0xC7,	0x00C7},
	 {0xC8,	0x00C8},
	 {0xC9,	0x00C9},
	 {0xCA,	0x00CA},
	 {0xCB,	0x00CB},
	 {0xCC,	0x00CC},
	 {0xCD,	0x00CD},
	 {0xCE,	0x00CE},
	 {0xCF,	0x00CF},
	 {0xD0,	0x00D0},
	 {0xD1,	0x00D1},
	 {0xD2,	0x00D2},
	 {0xD3,	0x00D3},
	 {0xD4,	0x00D4},
	 {0xD5,	0x00D5},
	 {0xD6,	0x00D6},
	 {0xD7,	0x00D7},
	 {0xD8,	0x00D8},
	 {0xD9,	0x00D9},
	 {0xDA,	0x00DA},
	 {0xDB,	0x00DB},
	 {0xDC,	0x00DC},
	 {0xDD,	0x00DD},
	 {0xDE,	0x00DE},
	 {0xDF,	0x00DF},
	 {0xE0,	0x00E0},
	 {0xE1,	0x00E1},
	 {0xE2,	0x00E2},
	 {0xE3,	0x00E3},
	 {0xE4,	0x00E4},
	 {0xE5,	0x00E5},
	 {0xE6,	0x00E6},
	 {0xE7,	0x00E7},
	 {0xE8,	0x00E8},
	 {0xE9,	0x00E9},
	 {0xEA,	0x00EA},
	 {0xEB,	0x00EB},
	 {0xEC,	0x00EC},
	 {0xED,	0x00ED},
	 {0xEE,	0x00EE},
	 {0xEF,	0x00EF},
	 {0xF0,	0x00F0},
	 {0xF1,	0x00F1},
	 {0xF2,	0x00F2},
	 {0xF3,	0x00F3},
	 {0xF4,	0x00F4},
	 {0xF5,	0x00F5},
	 {0xF6,	0x00F6},
	 {0xF7,	0x00F7},
	 {0xF8,	0x00F8},
	 {0xF9,	0x00F9},
	 {0xFA,	0x00FA},
	 {0xFB,	0x00FB},
	 {0xFC,	0x00FC},
	 {0xFD,	0x00FD},
	 {0xFE,	0x00FE},
	 {0xFF,	0x00FF},
	 {0x00,	0x0000}
	};

ISO_ENCODE arsIso2[] = {
	{0x01,	0x0001},	
	{0x02,	0x0002},	
	{0x03,	0x0003},	
	{0x04,	0x0004},	
	{0x05,	0x0005},	
	{0x06,	0x0006},	
	{0x07,	0x0007},	
	{0x08,	0x0008},	
	{0x09,	0x0009},	
	{0x0A,	0x000A},	
	{0x0B,	0x000B},	
	{0x0C,	0x000C},	
	{0x0D,	0x000D},	
	{0x0E,	0x000E},	
	{0x0F,	0x000F},	
	{0x10,	0x0010},	
	{0x11,	0x0011},	
	{0x12,	0x0012},	
	{0x13,	0x0013},	
	{0x14,	0x0014},	
	{0x15,	0x0015},	
	{0x16,	0x0016},	
	{0x17,	0x0017},	
	{0x18,	0x0018},	
	{0x19,	0x0019},	
	{0x1A,	0x001A},	
	{0x1B,	0x001B},	
	{0x1C,	0x001C},	
	{0x1D,	0x001D},	
	{0x1E,	0x001E},	
	{0x1F,	0x001F},	
	{0x20,	0x0020},	
	{0x21,	0x0021},	
	{0x22,	0x0022},	
	{0x23,	0x0023},	
	{0x24,	0x0024},	
	{0x25,	0x0025},	
	{0x26,	0x0026},	
	{0x27,	0x0027},	
	{0x28,	0x0028},	
	{0x29,	0x0029},	
	{0x2A,	0x002A},	
	{0x2B,	0x002B},	
	{0x2C,	0x002C},	
	{0x2D,	0x002D},	
	{0x2E,	0x002E},	
	{0x2F,	0x002F},	
	{0x30,	0x0030},	
	{0x31,	0x0031},	
	{0x32,	0x0032},	
	{0x33,	0x0033},	
	{0x34,	0x0034},	
	{0x35,	0x0035},	
	{0x36,	0x0036},	
	{0x37,	0x0037},	
	{0x38,	0x0038},	
	{0x39,	0x0039},	
	{0x3A,	0x003A},	
	{0x3B,	0x003B},	
	{0x3C,	0x003C},	
	{0x3D,	0x003D},	
	{0x3E,	0x003E},	
	{0x3F,	0x003F},	
	{0x40,	0x0040},	
	{0x41,	0x0041},	
	{0x42,	0x0042},	
	{0x43,	0x0043},	
	{0x44,	0x0044},	
	{0x45,	0x0045},	
	{0x46,	0x0046},	
	{0x47,	0x0047},	
	{0x48,	0x0048},	
	{0x49,	0x0049},	
	{0x4A,	0x004A},	
	{0x4B,	0x004B},	
	{0x4C,	0x004C},	
	{0x4D,	0x004D},	
	{0x4E,	0x004E},	
	{0x4F,	0x004F},	
	{0x50,	0x0050},	
	{0x51,	0x0051},	
	{0x52,	0x0052},	
	{0x53,	0x0053},	
	{0x54,	0x0054},	
	{0x55,	0x0055},	
	{0x56,	0x0056},	
	{0x57,	0x0057},	
	{0x58,	0x0058},	
	{0x59,	0x0059},	
	{0x5A,	0x005A},	
	{0x5B,	0x005B},	
	{0x5C,	0x005C},	
	{0x5D,	0x005D},	
	{0x5E,	0x005E},	
	{0x5F,	0x005F},	
	{0x60,	0x0060},	
	{0x61,	0x0061},	
	{0x62,	0x0062},	
	{0x63,	0x0063},	
	{0x64,	0x0064},	
	{0x65,	0x0065},	
	{0x66,	0x0066},	
	{0x67,	0x0067},	
	{0x68,	0x0068},	
	{0x69,	0x0069},	
	{0x6A,	0x006A},	
	{0x6B,	0x006B},	
	{0x6C,	0x006C},	
	{0x6D,	0x006D},	
	{0x6E,	0x006E},	
	{0x6F,	0x006F},	
	{0x70,	0x0070},	
	{0x71,	0x0071},	
	{0x72,	0x0072},	
	{0x73,	0x0073},	
	{0x74,	0x0074},	
	{0x75,	0x0075},	
	{0x76,	0x0076},	
	{0x77,	0x0077},	
	{0x78,	0x0078},	
	{0x79,	0x0079},	
	{0x7A,	0x007A},	
	{0x7B,	0x007B},	
	{0x7C,	0x007C},	
	{0x7D,	0x007D},	
	{0x7E,	0x007E},	
	{0x7F,	0x007F},	
	{0x80,	0x0080},	
	{0x81,	0x0081},	
	{0x82,	0x0082},	
	{0x83,	0x0083},	
	{0x84,	0x0084},	
	{0x85,	0x0085},	
	{0x86,	0x0086},	
	{0x87,	0x0087},	
	{0x88,	0x0088},	
	{0x89,	0x0089},	
	{0x8A,	0x008A},	
	{0x8B,	0x008B},	
	{0x8C,	0x008C},	
	{0x8D,	0x008D},	
	{0x8E,	0x008E},	
	{0x8F,	0x008F},	
	{0x90,	0x0090},	
	{0x91,	0x0091},	
	{0x92,	0x0092},	
	{0x93,	0x0093},	
	{0x94,	0x0094},	
	{0x95,	0x0095},	
	{0x96,	0x0096},	
	{0x97,	0x0097},	
	{0x98,	0x0098},	
	{0x99,	0x0099},	
	{0x9A,	0x009A},	
	{0x9B,	0x009B},	
	{0x9C,	0x009C},	
	{0x9D,	0x009D},	
	{0x9E,	0x009E},	
	{0x9F,	0x009F},	
	{0xA0,	0x00A0},	
	{0xA1,	0x0104},	
	{0xA2,	0x02D8},	
	{0xA3,	0x0141},	
	{0xA4,	0x00A4},	
	{0xA5,	0x013D},	
	{0xA6,	0x015A},	
	{0xA7,	0x00A7},	
	{0xA8,	0x00A8},	
	{0xA9,	0x0160},	
	{0xAA,	0x015E},	
	{0xAB,	0x0164},	
	{0xAC,	0x0179},	
	{0xAD,	0x00AD},	
	{0xAE,	0x017D},	
	{0xAF,	0x017B},	
	{0xB0,	0x00B0},	
	{0xB1,	0x0105},	
	{0xB2,	0x02DB},	
	{0xB3,	0x0142},	
	{0xB4,	0x00B4},	
	{0xB5,	0x013E},	
	{0xB6,	0x015B},	
	{0xB7,	0x02C7},	
	{0xB8,	0x00B8},	
	{0xB9,	0x0161},	
	{0xBA,	0x015F},	
	{0xBB,	0x0165},	
	{0xBC,	0x017A},	
	{0xBD,	0x02DD},	
	{0xBE,	0x017E},	
	{0xBF,	0x017C},	
	{0xC0,	0x0154},	
	{0xC1,	0x00C1},	
	{0xC2,	0x00C2},	
	{0xC3,	0x0102},	
	{0xC4,	0x00C4},	
	{0xC5,	0x0139},	
	{0xC6,	0x0106},	
	{0xC7,	0x00C7},	
	{0xC8,	0x010C},	
	{0xC9,	0x00C9},	
	{0xCA,	0x0118},	
	{0xCB,	0x00CB},	
	{0xCC,	0x011A},	
	{0xCD,	0x00CD},	
	{0xCE,	0x00CE},	
	{0xCF,	0x010E},	
	{0xD0,	0x0110},	
	{0xD1,	0x0143},	
	{0xD2,	0x0147},	
	{0xD3,	0x00D3},	
	{0xD4,	0x00D4},	
	{0xD5,	0x0150},	
	{0xD6,	0x00D6},	
	{0xD7,	0x00D7},	
	{0xD8,	0x0158},	
	{0xD9,	0x016E},	
	{0xDA,	0x00DA},	
	{0xDB,	0x0170},	
	{0xDC,	0x00DC},	
	{0xDD,	0x00DD},	
	{0xDE,	0x0162},	
	{0xDF,	0x00DF},	
	{0xE0,	0x0155},	
	{0xE1,	0x00E1},	
	{0xE2,	0x00E2},	
	{0xE3,	0x0103},	
	{0xE4,	0x00E4},	
	{0xE5,	0x013A},	
	{0xE6,	0x0107},	
	{0xE7,	0x00E7},	
	{0xE8,	0x010D},	
	{0xE9,	0x00E9},	
	{0xEA,	0x0119},	
	{0xEB,	0x00EB},	
	{0xEC,	0x011B},	
	{0xED,	0x00ED},	
	{0xEE,	0x00EE},	
	{0xEF,	0x010F},	
	{0xF0,	0x0111},	
	{0xF1,	0x0144},	
	{0xF2,	0x0148},	
	{0xF3,	0x00F3},	
	{0xF4,	0x00F4},	
	{0xF5,	0x0151},	
	{0xF6,	0x00F6},	
	{0xF7,	0x00F7},	
	{0xF8,	0x0159},	
	{0xF9,	0x016F},	
	{0xFA,	0x00FA},	
	{0xFB,	0x0171},	
	{0xFC,	0x00FC},	
	{0xFD,	0x00FD},	
	{0xFE,	0x0163},	
	{0xFF,	0x02D9},
	{0x00,	0x0000}	
};


ISO_ENCODE arsIso3[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x0126},
{0xA2,	0x02D8},
{0xA3,	0x00A3},
{0xA4,	0x00A4},
{0xA6,	0x0124},
{0xA7,	0x00A7},
{0xA8,	0x00A8},
{0xA9,	0x0130},
{0xAA,	0x015E},
{0xAB,	0x011E},
{0xAC,	0x0134},
{0xAD,	0x00AD},
{0xAF,	0x017B},
{0xB0,	0x00B0},
{0xB1,	0x0127},
{0xB2,	0x00B2},
{0xB3,	0x00B3},
{0xB4,	0x00B4},
{0xB5,	0x00B5},
{0xB6,	0x0125},
{0xB7,	0x00B7},
{0xB8,	0x00B8},
{0xB9,	0x0131},
{0xBA,	0x015F},
{0xBB,	0x011F},
{0xBC,	0x0135},
{0xBD,	0x00BD},
{0xBF,	0x017C},
{0xC0,	0x00C0},
{0xC1,	0x00C1},
{0xC2,	0x00C2},
{0xC4,	0x00C4},
{0xC5,	0x010A},
{0xC6,	0x0108},
{0xC7,	0x00C7},
{0xC8,	0x00C8},
{0xC9,	0x00C9},
{0xCA,	0x00CA},
{0xCB,	0x00CB},
{0xCC,	0x00CC},
{0xCD,	0x00CD},
{0xCE,	0x00CE},
{0xCF,	0x00CF},
{0xD1,	0x00D1},
{0xD2,	0x00D2},
{0xD3,	0x00D3},
{0xD4,	0x00D4},
{0xD5,	0x0120},
{0xD6,	0x00D6},
{0xD7,	0x00D7},
{0xD8,	0x011C},
{0xD9,	0x00D9},
{0xDA,	0x00DA},
{0xDB,	0x00DB},
{0xDC,	0x00DC},
{0xDD,	0x016C},
{0xDE,	0x015C},
{0xDF,	0x00DF},
{0xE0,	0x00E0},
{0xE1,	0x00E1},
{0xE2,	0x00E2},
{0xE4,	0x00E4},
{0xE5,	0x010B},
{0xE6,	0x0109},
{0xE7,	0x00E7},
{0xE8,	0x00E8},
{0xE9,	0x00E9},
{0xEA,	0x00EA},
{0xEB,	0x00EB},
{0xEC,	0x00EC},
{0xED,	0x00ED},
{0xEE,	0x00EE},
{0xEF,	0x00EF},
{0xF1,	0x00F1},
{0xF2,	0x00F2},
{0xF3,	0x00F3},
{0xF4,	0x00F4},
{0xF5,	0x0121},
{0xF6,	0x00F6},
{0xF7,	0x00F7},
{0xF8,	0x011D},
{0xF9,	0x00F9},
{0xFA,	0x00FA},
{0xFB,	0x00FB},
{0xFC,	0x00FC},
{0xFD,	0x016D},
{0xFE,	0x015D},
{0xFF,	0x02D9},
{0x00,	0x0000}
};


ISO_ENCODE arsIso4[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x0104},
{0xA2,	0x0138},
{0xA3,	0x0156},
{0xA4,	0x00A4},
{0xA5,	0x0128},
{0xA6,	0x013B},
{0xA7,	0x00A7},
{0xA8,	0x00A8},
{0xA9,	0x0160},
{0xAA,	0x0112},
{0xAB,	0x0122},
{0xAC,	0x0166},
{0xAD,	0x00AD},
{0xAE,	0x017D},
{0xAF,	0x00AF},
{0xB0,	0x00B0},
{0xB1,	0x0105},
{0xB2,	0x02DB},
{0xB3,	0x0157},
{0xB4,	0x00B4},
{0xB5,	0x0129},
{0xB6,	0x013C},
{0xB7,	0x02C7},
{0xB8,	0x00B8},
{0xB9,	0x0161},
{0xBA,	0x0113},
{0xBB,	0x0123},
{0xBC,	0x0167},
{0xBD,	0x014A},
{0xBE,	0x017E},
{0xBF,	0x014B},
{0xC0,	0x0100},
{0xC1,	0x00C1},
{0xC2,	0x00C2},
{0xC3,	0x00C3},
{0xC4,	0x00C4},
{0xC5,	0x00C5},
{0xC6,	0x00C6},
{0xC7,	0x012E},
{0xC8,	0x010C},
{0xC9,	0x00C9},
{0xCA,	0x0118},
{0xCB,	0x00CB},
{0xCC,	0x0116},
{0xCD,	0x00CD},
{0xCE,	0x00CE},
{0xCF,	0x012A},
{0xD0,	0x0110},
{0xD1,	0x0145},
{0xD2,	0x014C},
{0xD3,	0x0136},
{0xD4,	0x00D4},
{0xD5,	0x00D5},
{0xD6,	0x00D6},
{0xD7,	0x00D7},
{0xD8,	0x00D8},
{0xD9,	0x0172},
{0xDA,	0x00DA},
{0xDB,	0x00DB},
{0xDC,	0x00DC},
{0xDD,	0x0168},
{0xDE,	0x016A},
{0xDF,	0x00DF},
{0xE0,	0x0101},
{0xE1,	0x00E1},
{0xE2,	0x00E2},
{0xE3,	0x00E3},
{0xE4,	0x00E4},
{0xE5,	0x00E5},
{0xE6,	0x00E6},
{0xE7,	0x012F},
{0xE8,	0x010D},
{0xE9,	0x00E9},
{0xEA,	0x0119},
{0xEB,	0x00EB},
{0xEC,	0x0117},
{0xED,	0x00ED},
{0xEE,	0x00EE},
{0xEF,	0x012B},
{0xF0,	0x0111},
{0xF1,	0x0146},
{0xF2,	0x014D},
{0xF3,	0x0137},
{0xF4,	0x00F4},
{0xF5,	0x00F5},
{0xF6,	0x00F6},
{0xF7,	0x00F7},
{0xF8,	0x00F8},
{0xF9,	0x0173},
{0xFA,	0x00FA},
{0xFB,	0x00FB},
{0xFC,	0x00FC},
{0xFD,	0x0169},
{0xFE,	0x016B},
{0xFF,	0x02D9},
{0x00,	0x0000}
};


ISO_ENCODE arsIso5[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x0401},
{0xA2,	0x0402},
{0xA3,	0x0403},
{0xA4,	0x0404},
{0xA5,	0x0405},
{0xA6,	0x0406},
{0xA7,	0x0407},
{0xA8,	0x0408},
{0xA9,	0x0409},
{0xAA,	0x040A},
{0xAB,	0x040B},
{0xAC,	0x040C},
{0xAD,	0x00AD},
{0xAE,	0x040E},
{0xAF,	0x040F},
{0xB0,	0x0410},
{0xB1,	0x0411},
{0xB2,	0x0412},
{0xB3,	0x0413},
{0xB4,	0x0414},
{0xB5,	0x0415},
{0xB6,	0x0416},
{0xB7,	0x0417},
{0xB8,	0x0418},
{0xB9,	0x0419},
{0xBA,	0x041A},
{0xBB,	0x041B},
{0xBC,	0x041C},
{0xBD,	0x041D},
{0xBE,	0x041E},
{0xBF,	0x041F},
{0xC0,	0x0420},
{0xC1,	0x0421},
{0xC2,	0x0422},
{0xC3,	0x0423},
{0xC4,	0x0424},
{0xC5,	0x0425},
{0xC6,	0x0426},
{0xC7,	0x0427},
{0xC8,	0x0428},
{0xC9,	0x0429},
{0xCA,	0x042A},
{0xCB,	0x042B},
{0xCC,	0x042C},
{0xCD,	0x042D},
{0xCE,	0x042E},
{0xCF,	0x042F},
{0xD0,	0x0430},
{0xD1,	0x0431},
{0xD2,	0x0432},
{0xD3,	0x0433},
{0xD4,	0x0434},
{0xD5,	0x0435},
{0xD6,	0x0436},
{0xD7,	0x0437},
{0xD8,	0x0438},
{0xD9,	0x0439},
{0xDA,	0x043A},
{0xDB,	0x043B},
{0xDC,	0x043C},
{0xDD,	0x043D},
{0xDE,	0x043E},
{0xDF,	0x043F},
{0xE0,	0x0440},
{0xE1,	0x0441},
{0xE2,	0x0442},
{0xE3,	0x0443},
{0xE4,	0x0444},
{0xE5,	0x0445},
{0xE6,	0x0446},
{0xE7,	0x0447},
{0xE8,	0x0448},
{0xE9,	0x0449},
{0xEA,	0x044A},
{0xEB,	0x044B},
{0xEC,	0x044C},
{0xED,	0x044D},
{0xEE,	0x044E},
{0xEF,	0x044F},
{0xF0,	0x2116},
{0xF1,	0x0451},
{0xF2,	0x0452},
{0xF3,	0x0453},
{0xF4,	0x0454},
{0xF5,	0x0455},
{0xF6,	0x0456},
{0xF7,	0x0457},
{0xF8,	0x0458},
{0xF9,	0x0459},
{0xFA,	0x045A},
{0xFB,	0x045B},
{0xFC,	0x045C},
{0xFD,	0x00A7},
{0xFE,	0x045E},
{0xFF,	0x045F},
{0x00,	0x0000}
};

ISO_ENCODE arsIso6[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA4,	0x00A4},
{0xAC,	0x060C},
{0xAD,	0x00AD},
{0xBB,	0x061B},
{0xBF,	0x061F},
{0xC1,	0x0621},
{0xC2,	0x0622},
{0xC3,	0x0623},
{0xC4,	0x0624},
{0xC5,	0x0625},
{0xC6,	0x0626},
{0xC7,	0x0627},
{0xC8,	0x0628},
{0xC9,	0x0629},
{0xCA,	0x062A},
{0xCB,	0x062B},
{0xCC,	0x062C},
{0xCD,	0x062D},
{0xCE,	0x062E},
{0xCF,	0x062F},
{0xD0,	0x0630},
{0xD1,	0x0631},
{0xD2,	0x0632},
{0xD3,	0x0633},
{0xD4,	0x0634},
{0xD5,	0x0635},
{0xD6,	0x0636},
{0xD7,	0x0637},
{0xD8,	0x0638},
{0xD9,	0x0639},
{0xDA,	0x063A},
{0xE0,	0x0640},
{0xE1,	0x0641},
{0xE2,	0x0642},
{0xE3,	0x0643},
{0xE4,	0x0644},
{0xE5,	0x0645},
{0xE6,	0x0646},
{0xE7,	0x0647},
{0xE8,	0x0648},
{0xE9,	0x0649},
{0xEA,	0x064A},
{0xEB,	0x064B},
{0xEC,	0x064C},
{0xED,	0x064D},
{0xEE,	0x064E},
{0xEF,	0x064F},
{0xF0,	0x0650},
{0xF1,	0x0651},
{0xF2,	0x0652},
{0x00,	0x0000}
};

ISO_ENCODE arsIso7[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x2018},
{0xA2,	0x2019},
{0xA3,	0x00A3},
{0xA4,	0x20AC},
{0xA5,	0x20AF},
{0xA6,	0x00A6},
{0xA7,	0x00A7},
{0xA8,	0x00A8},
{0xA9,	0x00A9},
{0xAA,	0x037A},
{0xAB,	0x00AB},
{0xAC,	0x00AC},
{0xAD,	0x00AD},
{0xAF,	0x2015},
{0xB0,	0x00B0},
{0xB1,	0x00B1},
{0xB2,	0x00B2},
{0xB3,	0x00B3},
{0xB4,	0x0384},
{0xB5,	0x0385},
{0xB6,	0x0386},
{0xB7,	0x00B7},
{0xB8,	0x0388},
{0xB9,	0x0389},
{0xBA,	0x038A},
{0xBB,	0x00BB},
{0xBC,	0x038C},
{0xBD,	0x00BD},
{0xBE,	0x038E},
{0xBF,	0x038F},
{0xC0,	0x0390},
{0xC1,	0x0391},
{0xC2,	0x0392},
{0xC3,	0x0393},
{0xC4,	0x0394},
{0xC5,	0x0395},
{0xC6,	0x0396},
{0xC7,	0x0397},
{0xC8,	0x0398},
{0xC9,	0x0399},
{0xCA,	0x039A},
{0xCB,	0x039B},
{0xCC,	0x039C},
{0xCD,	0x039D},
{0xCE,	0x039E},
{0xCF,	0x039F},
{0xD0,	0x03A0},
{0xD1,	0x03A1},
{0xD3,	0x03A3},
{0xD4,	0x03A4},
{0xD5,	0x03A5},
{0xD6,	0x03A6},
{0xD7,	0x03A7},
{0xD8,	0x03A8},
{0xD9,	0x03A9},
{0xDA,	0x03AA},
{0xDB,	0x03AB},
{0xDC,	0x03AC},
{0xDD,	0x03AD},
{0xDE,	0x03AE},
{0xDF,	0x03AF},
{0xE0,	0x03B0},
{0xE1,	0x03B1},
{0xE2,	0x03B2},
{0xE3,	0x03B3},
{0xE4,	0x03B4},
{0xE5,	0x03B5},
{0xE6,	0x03B6},
{0xE7,	0x03B7},
{0xE8,	0x03B8},
{0xE9,	0x03B9},
{0xEA,	0x03BA},
{0xEB,	0x03BB},
{0xEC,	0x03BC},
{0xED,	0x03BD},
{0xEE,	0x03BE},
{0xEF,	0x03BF},
{0xF0,	0x03C0},
{0xF1,	0x03C1},
{0xF2,	0x03C2},
{0xF3,	0x03C3},
{0xF4,	0x03C4},
{0xF5,	0x03C5},
{0xF6,	0x03C6},
{0xF7,	0x03C7},
{0xF8,	0x03C8},
{0xF9,	0x03C9},
{0xFA,	0x03CA},
{0xFB,	0x03CB},
{0xFC,	0x03CC},
{0xFD,	0x03CD},
{0xFE,	0x03CE},
{0x00,	0x0000}
};

ISO_ENCODE arsIso8[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA2,	0x00A2},
{0xA3,	0x00A3},
{0xA4,	0x00A4},
{0xA5,	0x00A5},
{0xA6,	0x00A6},
{0xA7,	0x00A7},
{0xA8,	0x00A8},
{0xA9,	0x00A9},
{0xAA,	0x00D7},
{0xAB,	0x00AB},
{0xAC,	0x00AC},
{0xAD,	0x00AD},
{0xAE,	0x00AE},
{0xAF,	0x00AF},
{0xB0,	0x00B0},
{0xB1,	0x00B1},
{0xB2,	0x00B2},
{0xB3,	0x00B3},
{0xB4,	0x00B4},
{0xB5,	0x00B5},
{0xB6,	0x00B6},
{0xB7,	0x00B7},
{0xB8,	0x00B8},
{0xB9,	0x00B9},
{0xBA,	0x00F7},
{0xBB,	0x00BB},
{0xBC,	0x00BC},
{0xBD,	0x00BD},
{0xBE,	0x00BE},
{0xDF,	0x2017},
{0xE0,	0x05D0},
{0xE1,	0x05D1},
{0xE2,	0x05D2},
{0xE3,	0x05D3},
{0xE4,	0x05D4},
{0xE5,	0x05D5},
{0xE6,	0x05D6},
{0xE7,	0x05D7},
{0xE8,	0x05D8},
{0xE9,	0x05D9},
{0xEA,	0x05DA},
{0xEB,	0x05DB},
{0xEC,	0x05DC},
{0xED,	0x05DD},
{0xEE,	0x05DE},
{0xEF,	0x05DF},
{0xF0,	0x05E0},
{0xF1,	0x05E1},
{0xF2,	0x05E2},
{0xF3,	0x05E3},
{0xF4,	0x05E4},
{0xF5,	0x05E5},
{0xF6,	0x05E6},
{0xF7,	0x05E7},
{0xF8,	0x05E8},
{0xF9,	0x05E9},
{0xFA,	0x05EA},
{0xFD,	0x200E},
{0xFE,	0x200F},
{0x00,	0x0000}
};

ISO_ENCODE arsIso10[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x0104},
{0xA2,	0x0112},
{0xA3,	0x0122},
{0xA4,	0x012A},
{0xA5,	0x0128},
{0xA6,	0x0136},
{0xA7,	0x00A7},
{0xA8,	0x013B},
{0xA9,	0x0110},
{0xAA,	0x0160},
{0xAB,	0x0166},
{0xAC,	0x017D},
{0xAD,	0x00AD},
{0xAE,	0x016A},
{0xAF,	0x014A},
{0xB0,	0x00B0},
{0xB1,	0x0105},
{0xB2,	0x0113},
{0xB3,	0x0123},
{0xB4,	0x012B},
{0xB5,	0x0129},
{0xB6,	0x0137},
{0xB7,	0x00B7},
{0xB8,	0x013C},
{0xB9,	0x0111},
{0xBA,	0x0161},
{0xBB,	0x0167},
{0xBC,	0x017E},
{0xBD,	0x2015},
{0xBE,	0x016B},
{0xBF,	0x014B},
{0xC0,	0x0100},
{0xC1,	0x00C1},
{0xC2,	0x00C2},
{0xC3,	0x00C3},
{0xC4,	0x00C4},
{0xC5,	0x00C5},
{0xC6,	0x00C6},
{0xC7,	0x012E},
{0xC8,	0x010C},
{0xC9,	0x00C9},
{0xCA,	0x0118},
{0xCB,	0x00CB},
{0xCC,	0x0116},
{0xCD,	0x00CD},
{0xCE,	0x00CE},
{0xCF,	0x00CF},
{0xD0,	0x00D0},
{0xD1,	0x0145},
{0xD2,	0x014C},
{0xD3,	0x00D3},
{0xD4,	0x00D4},
{0xD5,	0x00D5},
{0xD6,	0x00D6},
{0xD7,	0x0168},
{0xD8,	0x00D8},
{0xD9,	0x0172},
{0xDA,	0x00DA},
{0xDB,	0x00DB},
{0xDC,	0x00DC},
{0xDD,	0x00DD},
{0xDE,	0x00DE},
{0xDF,	0x00DF},
{0xE0,	0x0101},
{0xE1,	0x00E1},
{0xE2,	0x00E2},
{0xE3,	0x00E3},
{0xE4,	0x00E4},
{0xE5,	0x00E5},
{0xE6,	0x00E6},
{0xE7,	0x012F},
{0xE8,	0x010D},
{0xE9,	0x00E9},
{0xEA,	0x0119},
{0xEB,	0x00EB},
{0xEC,	0x0117},
{0xED,	0x00ED},
{0xEE,	0x00EE},
{0xEF,	0x00EF},
{0xF0,	0x00F0},
{0xF1,	0x0146},
{0xF2,	0x014D},
{0xF3,	0x00F3},
{0xF4,	0x00F4},
{0xF5,	0x00F5},
{0xF6,	0x00F6},
{0xF7,	0x0169},
{0xF8,	0x00F8},
{0xF9,	0x0173},
{0xFA,	0x00FA},
{0xFB,	0x00FB},
{0xFC,	0x00FC},
{0xFD,	0x00FD},
{0xFE,	0x00FE},
{0xFF,	0x0138},
{0x00,	0x0000}
};

ISO_ENCODE arsIso11[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x0E01},
{0xA2,	0x0E02},
{0xA3,	0x0E03},
{0xA4,	0x0E04},
{0xA5,	0x0E05},
{0xA6,	0x0E06},
{0xA7,	0x0E07},
{0xA8,	0x0E08},
{0xA9,	0x0E09},
{0xAA,	0x0E0A},
{0xAB,	0x0E0B},
{0xAC,	0x0E0C},
{0xAD,	0x0E0D},
{0xAE,	0x0E0E},
{0xAF,	0x0E0F},
{0xB0,	0x0E10},
{0xB1,	0x0E11},
{0xB2,	0x0E12},
{0xB3,	0x0E13},
{0xB4,	0x0E14},
{0xB5,	0x0E15},
{0xB6,	0x0E16},
{0xB7,	0x0E17},
{0xB8,	0x0E18},
{0xB9,	0x0E19},
{0xBA,	0x0E1A},
{0xBB,	0x0E1B},
{0xBC,	0x0E1C},
{0xBD,	0x0E1D},
{0xBE,	0x0E1E},
{0xBF,	0x0E1F},
{0xC0,	0x0E20},
{0xC1,	0x0E21},
{0xC2,	0x0E22},
{0xC3,	0x0E23},
{0xC4,	0x0E24},
{0xC5,	0x0E25},
{0xC6,	0x0E26},
{0xC7,	0x0E27},
{0xC8,	0x0E28},
{0xC9,	0x0E29},
{0xCA,	0x0E2A},
{0xCB,	0x0E2B},
{0xCC,	0x0E2C},
{0xCD,	0x0E2D},
{0xCE,	0x0E2E},
{0xCF,	0x0E2F},
{0xD0,	0x0E30},
{0xD1,	0x0E31},
{0xD2,	0x0E32},
{0xD3,	0x0E33},
{0xD4,	0x0E34},
{0xD5,	0x0E35},
{0xD6,	0x0E36},
{0xD7,	0x0E37},
{0xD8,	0x0E38},
{0xD9,	0x0E39},
{0xDA,	0x0E3A},
{0xDF,	0x0E3F},
{0xE0,	0x0E40},
{0xE1,	0x0E41},
{0xE2,	0x0E42},
{0xE3,	0x0E43},
{0xE4,	0x0E44},
{0xE5,	0x0E45},
{0xE6,	0x0E46},
{0xE7,	0x0E47},
{0xE8,	0x0E48},
{0xE9,	0x0E49},
{0xEA,	0x0E4A},
{0xEB,	0x0E4B},
{0xEC,	0x0E4C},
{0xED,	0x0E4D},
{0xEE,	0x0E4E},
{0xEF,	0x0E4F},
{0xF0,	0x0E50},
{0xF1,	0x0E51},
{0xF2,	0x0E52},
{0xF3,	0x0E53},
{0xF4,	0x0E54},
{0xF5,	0x0E55},
{0xF6,	0x0E56},
{0xF7,	0x0E57},
{0xF8,	0x0E58},
{0xF9,	0x0E59},
{0xFA,	0x0E5A},
{0xFB,	0x0E5B},
{0x00,	0x0000}
};


ISO_ENCODE arsIso13[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x201D},
{0xA2,	0x00A2},
{0xA3,	0x00A3},
{0xA4,	0x00A4},
{0xA5,	0x201E},
{0xA6,	0x00A6},
{0xA7,	0x00A7},
{0xA8,	0x00D8},
{0xA9,	0x00A9},
{0xAA,	0x0156},
{0xAB,	0x00AB},
{0xAC,	0x00AC},
{0xAD,	0x00AD},
{0xAE,	0x00AE},
{0xAF,	0x00C6},
{0xB0,	0x00B0},
{0xB1,	0x00B1},
{0xB2,	0x00B2},
{0xB3,	0x00B3},
{0xB4,	0x201C},
{0xB5,	0x00B5},
{0xB6,	0x00B6},
{0xB7,	0x00B7},
{0xB8,	0x00F8},
{0xB9,	0x00B9},
{0xBA,	0x0157},
{0xBB,	0x00BB},
{0xBC,	0x00BC},
{0xBD,	0x00BD},
{0xBE,	0x00BE},
{0xBF,	0x00E6},
{0xC0,	0x0104},
{0xC1,	0x012E},
{0xC2,	0x0100},
{0xC3,	0x0106},
{0xC4,	0x00C4},
{0xC5,	0x00C5},
{0xC6,	0x0118},
{0xC7,	0x0112},
{0xC8,	0x010C},
{0xC9,	0x00C9},
{0xCA,	0x0179},
{0xCB,	0x0116},
{0xCC,	0x0122},
{0xCD,	0x0136},
{0xCE,	0x012A},
{0xCF,	0x013B},
{0xD0,	0x0160},
{0xD1,	0x0143},
{0xD2,	0x0145},
{0xD3,	0x00D3},
{0xD4,	0x014C},
{0xD5,	0x00D5},
{0xD6,	0x00D6},
{0xD7,	0x00D7},
{0xD8,	0x0172},
{0xD9,	0x0141},
{0xDA,	0x015A},
{0xDB,	0x016A},
{0xDC,	0x00DC},
{0xDD,	0x017B},
{0xDE,	0x017D},
{0xDF,	0x00DF},
{0xE0,	0x0105},
{0xE1,	0x012F},
{0xE2,	0x0101},
{0xE3,	0x0107},
{0xE4,	0x00E4},
{0xE5,	0x00E5},
{0xE6,	0x0119},
{0xE7,	0x0113},
{0xE8,	0x010D},
{0xE9,	0x00E9},
{0xEA,	0x017A},
{0xEB,	0x0117},
{0xEC,	0x0123},
{0xED,	0x0137},
{0xEE,	0x012B},
{0xEF,	0x013C},
{0xF0,	0x0161},
{0xF1,	0x0144},
{0xF2,	0x0146},
{0xF3,	0x00F3},
{0xF4,	0x014D},
{0xF5,	0x00F5},
{0xF6,	0x00F6},
{0xF7,	0x00F7},
{0xF8,	0x0173},
{0xF9,	0x0142},
{0xFA,	0x015B},
{0xFB,	0x016B},
{0xFC,	0x00FC},
{0xFD,	0x017C},
{0xFE,	0x017E},
{0xFF,	0x2019},
{0x00,	0x0000}
};

ISO_ENCODE arsIso14[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x1E02},
{0xA2,	0x1E03},
{0xA3,	0x00A3},
{0xA4,	0x010A},
{0xA5,	0x010B},
{0xA6,	0x1E0A},
{0xA7,	0x00A7},
{0xA8,	0x1E80},
{0xA9,	0x00A9},
{0xAA,	0x1E82},
{0xAB,	0x1E0B},
{0xAC,	0x1EF2},
{0xAD,	0x00AD},
{0xAE,	0x00AE},
{0xAF,	0x0178},
{0xB0,	0x1E1E},
{0xB1,	0x1E1F},
{0xB2,	0x0120},
{0xB3,	0x0121},
{0xB4,	0x1E40},
{0xB5,	0x1E41},
{0xB6,	0x00B6},
{0xB7,	0x1E56},
{0xB8,	0x1E81},
{0xB9,	0x1E57},
{0xBA,	0x1E83},
{0xBB,	0x1E60},
{0xBC,	0x1EF3},
{0xBD,	0x1E84},
{0xBE,	0x1E85},
{0xBF,	0x1E61},
{0xC0,	0x00C0},
{0xC1,	0x00C1},
{0xC2,	0x00C2},
{0xC3,	0x00C3},
{0xC4,	0x00C4},
{0xC5,	0x00C5},
{0xC6,	0x00C6},
{0xC7,	0x00C7},
{0xC8,	0x00C8},
{0xC9,	0x00C9},
{0xCA,	0x00CA},
{0xCB,	0x00CB},
{0xCC,	0x00CC},
{0xCD,	0x00CD},
{0xCE,	0x00CE},
{0xCF,	0x00CF},
{0xD0,	0x0174},
{0xD1,	0x00D1},
{0xD2,	0x00D2},
{0xD3,	0x00D3},
{0xD4,	0x00D4},
{0xD5,	0x00D5},
{0xD6,	0x00D6},
{0xD7,	0x1E6A},
{0xD8,	0x00D8},
{0xD9,	0x00D9},
{0xDA,	0x00DA},
{0xDB,	0x00DB},
{0xDC,	0x00DC},
{0xDD,	0x00DD},
{0xDE,	0x0176},
{0xDF,	0x00DF},
{0xE0,	0x00E0},
{0xE1,	0x00E1},
{0xE2,	0x00E2},
{0xE3,	0x00E3},
{0xE4,	0x00E4},
{0xE5,	0x00E5},
{0xE6,	0x00E6},
{0xE7,	0x00E7},
{0xE8,	0x00E8},
{0xE9,	0x00E9},
{0xEA,	0x00EA},
{0xEB,	0x00EB},
{0xEC,	0x00EC},
{0xED,	0x00ED},
{0xEE,	0x00EE},
{0xEF,	0x00EF},
{0xF0,	0x0175},
{0xF1,	0x00F1},
{0xF2,	0x00F2},
{0xF3,	0x00F3},
{0xF4,	0x00F4},
{0xF5,	0x00F5},
{0xF6,	0x00F6},
{0xF7,	0x1E6B},
{0xF8,	0x00F8},
{0xF9,	0x00F9},
{0xFA,	0x00FA},
{0xFB,	0x00FB},
{0xFC,	0x00FC},
{0xFD,	0x00FD},
{0xFE,	0x0177},
{0xFF,	0x00FF},
{0x00,	0x0000}
};

ISO_ENCODE arsIso15[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x00A1},
{0xA2,	0x00A2},
{0xA3,	0x00A3},
{0xA4,	0x20AC},
{0xA5,	0x00A5},
{0xA6,	0x0160},
{0xA7,	0x00A7},
{0xA8,	0x0161},
{0xA9,	0x00A9},
{0xAA,	0x00AA},
{0xAB,	0x00AB},
{0xAC,	0x00AC},
{0xAD,	0x00AD},
{0xAE,	0x00AE},
{0xAF,	0x00AF},
{0xB0,	0x00B0},
{0xB1,	0x00B1},
{0xB2,	0x00B2},
{0xB3,	0x00B3},
{0xB4,	0x017D},
{0xB5,	0x00B5},
{0xB6,	0x00B6},
{0xB7,	0x00B7},
{0xB8,	0x017E},
{0xB9,	0x00B9},
{0xBA,	0x00BA},
{0xBB,	0x00BB},
{0xBC,	0x0152},
{0xBD,	0x0153},
{0xBE,	0x0178},
{0xBF,	0x00BF},
{0xC0,	0x00C0},
{0xC1,	0x00C1},
{0xC2,	0x00C2},
{0xC3,	0x00C3},
{0xC4,	0x00C4},
{0xC5,	0x00C5},
{0xC6,	0x00C6},
{0xC7,	0x00C7},
{0xC8,	0x00C8},
{0xC9,	0x00C9},
{0xCA,	0x00CA},
{0xCB,	0x00CB},
{0xCC,	0x00CC},
{0xCD,	0x00CD},
{0xCE,	0x00CE},
{0xCF,	0x00CF},
{0xD0,	0x00D0},
{0xD1,	0x00D1},
{0xD2,	0x00D2},
{0xD3,	0x00D3},
{0xD4,	0x00D4},
{0xD5,	0x00D5},
{0xD6,	0x00D6},
{0xD7,	0x00D7},
{0xD8,	0x00D8},
{0xD9,	0x00D9},
{0xDA,	0x00DA},
{0xDB,	0x00DB},
{0xDC,	0x00DC},
{0xDD,	0x00DD},
{0xDE,	0x00DE},
{0xDF,	0x00DF},
{0xE0,	0x00E0},
{0xE1,	0x00E1},
{0xE2,	0x00E2},
{0xE3,	0x00E3},
{0xE4,	0x00E4},
{0xE5,	0x00E5},
{0xE6,	0x00E6},
{0xE7,	0x00E7},
{0xE8,	0x00E8},
{0xE9,	0x00E9},
{0xEA,	0x00EA},
{0xEB,	0x00EB},
{0xEC,	0x00EC},
{0xED,	0x00ED},
{0xEE,	0x00EE},
{0xEF,	0x00EF},
{0xF0,	0x00F0},
{0xF1,	0x00F1},
{0xF2,	0x00F2},
{0xF3,	0x00F3},
{0xF4,	0x00F4},
{0xF5,	0x00F5},
{0xF6,	0x00F6},
{0xF7,	0x00F7},
{0xF8,	0x00F8},
{0xF9,	0x00F9},
{0xFA,	0x00FA},
{0xFB,	0x00FB},
{0xFC,	0x00FC},
{0xFD,	0x00FD},
{0xFE,	0x00FE},
{0xFF,	0x00FF},
{0x00,	0x0000}
};

ISO_ENCODE arsIso16[] = {
{0x01,	0x0001},
{0x02,	0x0002},
{0x03,	0x0003},
{0x04,	0x0004},
{0x05,	0x0005},
{0x06,	0x0006},
{0x07,	0x0007},
{0x08,	0x0008},
{0x09,	0x0009},
{0x0A,	0x000A},
{0x0B,	0x000B},
{0x0C,	0x000C},
{0x0D,	0x000D},
{0x0E,	0x000E},
{0x0F,	0x000F},
{0x10,	0x0010},
{0x11,	0x0011},
{0x12,	0x0012},
{0x13,	0x0013},
{0x14,	0x0014},
{0x15,	0x0015},
{0x16,	0x0016},
{0x17,	0x0017},
{0x18,	0x0018},
{0x19,	0x0019},
{0x1A,	0x001A},
{0x1B,	0x001B},
{0x1C,	0x001C},
{0x1D,	0x001D},
{0x1E,	0x001E},
{0x1F,	0x001F},
{0x20,	0x0020},
{0x21,	0x0021},
{0x22,	0x0022},
{0x23,	0x0023},
{0x24,	0x0024},
{0x25,	0x0025},
{0x26,	0x0026},
{0x27,	0x0027},
{0x28,	0x0028},
{0x29,	0x0029},
{0x2A,	0x002A},
{0x2B,	0x002B},
{0x2C,	0x002C},
{0x2D,	0x002D},
{0x2E,	0x002E},
{0x2F,	0x002F},
{0x30,	0x0030},
{0x31,	0x0031},
{0x32,	0x0032},
{0x33,	0x0033},
{0x34,	0x0034},
{0x35,	0x0035},
{0x36,	0x0036},
{0x37,	0x0037},
{0x38,	0x0038},
{0x39,	0x0039},
{0x3A,	0x003A},
{0x3B,	0x003B},
{0x3C,	0x003C},
{0x3D,	0x003D},
{0x3E,	0x003E},
{0x3F,	0x003F},
{0x40,	0x0040},
{0x41,	0x0041},
{0x42,	0x0042},
{0x43,	0x0043},
{0x44,	0x0044},
{0x45,	0x0045},
{0x46,	0x0046},
{0x47,	0x0047},
{0x48,	0x0048},
{0x49,	0x0049},
{0x4A,	0x004A},
{0x4B,	0x004B},
{0x4C,	0x004C},
{0x4D,	0x004D},
{0x4E,	0x004E},
{0x4F,	0x004F},
{0x50,	0x0050},
{0x51,	0x0051},
{0x52,	0x0052},
{0x53,	0x0053},
{0x54,	0x0054},
{0x55,	0x0055},
{0x56,	0x0056},
{0x57,	0x0057},
{0x58,	0x0058},
{0x59,	0x0059},
{0x5A,	0x005A},
{0x5B,	0x005B},
{0x5C,	0x005C},
{0x5D,	0x005D},
{0x5E,	0x005E},
{0x5F,	0x005F},
{0x60,	0x0060},
{0x61,	0x0061},
{0x62,	0x0062},
{0x63,	0x0063},
{0x64,	0x0064},
{0x65,	0x0065},
{0x66,	0x0066},
{0x67,	0x0067},
{0x68,	0x0068},
{0x69,	0x0069},
{0x6A,	0x006A},
{0x6B,	0x006B},
{0x6C,	0x006C},
{0x6D,	0x006D},
{0x6E,	0x006E},
{0x6F,	0x006F},
{0x70,	0x0070},
{0x71,	0x0071},
{0x72,	0x0072},
{0x73,	0x0073},
{0x74,	0x0074},
{0x75,	0x0075},
{0x76,	0x0076},
{0x77,	0x0077},
{0x78,	0x0078},
{0x79,	0x0079},
{0x7A,	0x007A},
{0x7B,	0x007B},
{0x7C,	0x007C},
{0x7D,	0x007D},
{0x7E,	0x007E},
{0x7F,	0x007F},
{0x80,	0x0080},
{0x81,	0x0081},
{0x82,	0x0082},
{0x83,	0x0083},
{0x84,	0x0084},
{0x85,	0x0085},
{0x86,	0x0086},
{0x87,	0x0087},
{0x88,	0x0088},
{0x89,	0x0089},
{0x8A,	0x008A},
{0x8B,	0x008B},
{0x8C,	0x008C},
{0x8D,	0x008D},
{0x8E,	0x008E},
{0x8F,	0x008F},
{0x90,	0x0090},
{0x91,	0x0091},
{0x92,	0x0092},
{0x93,	0x0093},
{0x94,	0x0094},
{0x95,	0x0095},
{0x96,	0x0096},
{0x97,	0x0097},
{0x98,	0x0098},
{0x99,	0x0099},
{0x9A,	0x009A},
{0x9B,	0x009B},
{0x9C,	0x009C},
{0x9D,	0x009D},
{0x9E,	0x009E},
{0x9F,	0x009F},
{0xA0,	0x00A0},
{0xA1,	0x0104},
{0xA2,	0x0105},
{0xA3,	0x0141},
{0xA4,	0x20AC},
{0xA5,	0x201E},
{0xA6,	0x0160},
{0xA7,	0x00A7},
{0xA8,	0x0161},
{0xA9,	0x00A9},
{0xAA,	0x0218},
{0xAB,	0x00AB},
{0xAC,	0x0179},
{0xAD,	0x00AD},
{0xAE,	0x017A},
{0xAF,	0x017B},
{0xB0,	0x00B0},
{0xB1,	0x00B1},
{0xB2,	0x010C},
{0xB3,	0x0142},
{0xB4,	0x017D},
{0xB5,	0x201D},
{0xB6,	0x00B6},
{0xB7,	0x00B7},
{0xB8,	0x017E},
{0xB9,	0x010D},
{0xBA,	0x0219},
{0xBB,	0x00BB},
{0xBC,	0x0152},
{0xBD,	0x0153},
{0xBE,	0x0178},
{0xBF,	0x017C},
{0xC0,	0x00C0},
{0xC1,	0x00C1},
{0xC2,	0x00C2},
{0xC3,	0x0102},
{0xC4,	0x00C4},
{0xC5,	0x0106},
{0xC6,	0x00C6},
{0xC7,	0x00C7},
{0xC8,	0x00C8},
{0xC9,	0x00C9},
{0xCA,	0x00CA},
{0xCB,	0x00CB},
{0xCC,	0x00CC},
{0xCD,	0x00CD},
{0xCE,	0x00CE},
{0xCF,	0x00CF},
{0xD0,	0x0110},
{0xD1,	0x0143},
{0xD2,	0x00D2},
{0xD3,	0x00D3},
{0xD4,	0x00D4},
{0xD5,	0x0150},
{0xD6,	0x00D6},
{0xD7,	0x015A},
{0xD8,	0x0170},
{0xD9,	0x00D9},
{0xDA,	0x00DA},
{0xDB,	0x00DB},
{0xDC,	0x00DC},
{0xDD,	0x0118},
{0xDE,	0x021A},
{0xDF,	0x00DF},
{0xE0,	0x00E0},
{0xE1,	0x00E1},
{0xE2,	0x00E2},
{0xE3,	0x0103},
{0xE4,	0x00E4},
{0xE5,	0x0107},
{0xE6,	0x00E6},
{0xE7,	0x00E7},
{0xE8,	0x00E8},
{0xE9,	0x00E9},
{0xEA,	0x00EA},
{0xEB,	0x00EB},
{0xEC,	0x00EC},
{0xED,	0x00ED},
{0xEE,	0x00EE},
{0xEF,	0x00EF},
{0xF0,	0x0111},
{0xF1,	0x0144},
{0xF2,	0x00F2},
{0xF3,	0x00F3},
{0xF4,	0x00F4},
{0xF5,	0x0151},
{0xF6,	0x00F6},
{0xF7,	0x015B},
{0xF8,	0x0171},
{0xF9,	0x00F9},
{0xFA,	0x00FA},
{0xFB,	0x00FB},
{0xFC,	0x00FC},
{0xFD,	0x0119},
{0xFE,	0x021B},
{0xFF,	0x00FF},
{0x00,	0x0000}
};

	ISO_ENCODE * pbMap=NULL;
	INT a;

	ehFreePtr(&_sPd.pbCharMap);
	_sPd.pbCharMap=ehAlloc(0xFFFF);
	memset(_sPd.pbCharMap,'?',0xFFFF);

	if (!strcmp(pszEncode,"ISO8859-2")) pbMap=arsIso2;
	if (!strcmp(pszEncode,"ISO8859-3")) pbMap=arsIso3;
	if (!strcmp(pszEncode,"ISO8859-4")) pbMap=arsIso4;
	if (!strcmp(pszEncode,"ISO8859-5")) pbMap=arsIso5;
	if (!strcmp(pszEncode,"ISO8859-6")) pbMap=arsIso6;
	if (!strcmp(pszEncode,"ISO8859-7")) pbMap=arsIso7;
	if (!strcmp(pszEncode,"ISO8859-8")) pbMap=arsIso8;
	//if (!strcmp(pszEncode,"ISO8859-9")) pbMap=arsIso9;
	if (!strcmp(pszEncode,"ISO8859-10")) pbMap=arsIso10;
	if (!strcmp(pszEncode,"ISO8859-11")) pbMap=arsIso11;
	if (!strcmp(pszEncode,"ISO8859-13")) pbMap=arsIso13;
	if (!strcmp(pszEncode,"ISO8859-14")) pbMap=arsIso14;
	if (!strcmp(pszEncode,"ISO8859-15")) pbMap=arsIso15;
	if (!strcmp(pszEncode,"ISO8859-16")) pbMap=arsIso16;

	if (!pbMap) 
		ehError();
	for (a=0;pbMap[a].dwCode;a++) {
		_sPd.pbCharMap[pbMap[a].dwCode]=pbMap[a].bCode;
	}
}

#endif


//
// MetaFile
//

static struct {

//	SIZE			sizSourcePixel;	// Dimensioni del sorgente in mm
//	SIZE			sizDestPixel;	// Dimensioni della destinazione in mm
//	PWD_SIZE		sumArea;		// Dimensioni della area in UM

	PWD_SIZE		sumSource;		// Dimensioni del sorgente in UM
	PWD_SIZE		sumDest;		// Dimensioni del sorgente in UM
	PWD_RECT		rumMeta;		// Retangolo del MetaFile nella pagina in UM
	EMRSETMAPMODE	sMap;
	double			dScale;


	INT					iBkMode;		//  OPAQUE =2 / TRASPARENT =1
	LOGBRUSH32			sLogBrush;
	BOOL				bBrush;			// T/F se è settato il pennello

	LOGPEN				sLogPen;
	EXTLOGPEN *			psExtLogPen;
	BOOL				bPen;			// T/F se è settato il pennello
	POINTL				ptCursor;

	COLORREF			crText;
//	PWD_POINT			pumOrg;

	LOGFONTW			sLogFont;
	EMRSETVIEWPORTEXTEX sWinExtEx;
	EMRSETVIEWPORTEXTEX sViewExtEx;	// Device Logic unit
	EMRSETVIEWPORTEXTEX sWinOrgEx;	// Origine del disegno
	EMRSETVIEWPORTEXTEX sViewOrgEx;

	EH_LST				lstObj;

	EH_LST				lstPath;		// Percorso in costruzione
	EH_LST				lstPathReady;	// Percorso pronto

} _sMf; // struct Meta File

typedef struct {

	DWORD		idObject;
	INT			iType; // 1=Brush,2=Pen,3=Font
	LOGBRUSH32	sLogBrush;
	LOGPEN		sLogPen;
	LOGFONTW	sLogFont;
	EXTLOGPEN * psExtLogPen;
	LOGPALETTE * psLogPalette;

} S_METAOBJ;

//
// iType: 0=X, 1=Y
//
static PWD_VAL _mapConvert(INT iValue,INT iType,BOOL bOffset) {

	PWD_VAL umVal=0;
//	INT iRap;
	switch (_sMf.sMap.iMode) {

		// TWIP è un ventesimo di un punto
		// 1/1440 di pollice
			case MM_TWIPS://            6
				/*
//#define TWIP 56.69291367152334
#define TWIP 58.6
//				#define TWIP 1
				if (!iType)
					umVal=_sMf.rumMeta.left+pwdUm(PUM_MM,(double) iValue/TWIP*_sMf.dScale);
					else
					umVal=_sMf.rumMeta.top+pwdUm(PUM_MM,-(double) iValue/TWIP*_sMf.dScale);
					*/

				if (!iType) {
				
					umVal=pwdUm(PUM_INCH,(double) iValue/1440);
					if (bOffset) umVal+=_sMf.rumMeta.left;
				}
				
				else {
					umVal=pwdUm(PUM_INCH,-(double) iValue/1440);
					if (bOffset) umVal+=_sMf.rumMeta.top;
				}

				break;

				// MM_TEXT	Each unit in page space is mapped to one pixel; 
				//			that is, no scaling is performed at all. When no translation is in effect (this is the default), 
				//			page space in the MM_TEXT mapping mode is equivalent to physical device space. 
				//			The value of x increases from left to right. The value of y increases from top to bottom.
			case MM_TEXT://             1
				if (iValue<0) iValue=-iValue;
				umVal=iValue;
//				printf("qui");
				break;

			case MM_LOMETRIC://         2
			case MM_HIMETRIC://         3
			case MM_LOENGLISH://       4
			case MM_HIENGLISH://        5
			case MM_ISOTROPIC://        7
					printf("qui");
					break;
			case MM_ANISOTROPIC://      8	
				if (!iType) {
					// iValue:x=_sMf.sWinExtEx.szlExtent.cx:_sMf.sViewExtEx.szlExtent.cx
					//umVal=_sMf.sViewExtEx.szlExtent.cx*iValue/_sMf.sWinExtEx.szlExtent.cx;
					// iValue:x=_sMf.sViewExtEx.szlExtent.cx:_sMf.sumSource.cx
					//iRap=_sMf.sViewExtEx.szlExtent.cx*iValue/_sMf.sWinExtEx.szlExtent.cx;
//					umVal=_sMf.sumDest.cx*iRap/_sMf.sViewExtEx.szlExtent.cx;	
					umVal=_sMf.sumDest.cx*(double)iValue/(double)_sMf.sWinExtEx.szlExtent.cx;	
					//if (bOffset) umVal=_sMf.sumDest.cx+umVal; //else umVal=-umVal;
					if (bOffset) umVal+=_sMf.rumMeta.left;
					
				} else {
				

					//umVal=_sMf.sViewExtEx.szlExtent.cy+(_sMf.sViewExtEx.szlExtent.cy*iValue/_sMf.sWinExtEx.szlExtent.cy);
//					iRap=_sMf.sViewExtEx.szlExtent.cy*iValue/_sMf.sWinExtEx.szlExtent.cy;
					umVal=_sMf.sumDest.cy*(double)iValue/(double)_sMf.sWinExtEx.szlExtent.cy;
//					if (bOffset) umVal=_sMf.sumDest.cy+umVal; //else umVal=-umVal;
					if (bOffset) umVal+=_sMf.sumDest.cy+_sMf.rumMeta.top;
				}
				break;

			default:
				printf("Modo ? %d",_sMf.sMap.iMode);
				ehError();
	}
	return umVal;
}

static CHAR * _desc(INT iType) {

	CHAR * psz="*";
	switch (iType) {

		case EMR_HEADER: psz="header"; break;
		case EMR_EOF: psz="eof"; break;
		case EMR_SETWINDOWORGEX: psz="EMR_SETWINDOWORGEX"; break;
		case EMR_SETWINDOWEXTEX: psz="EMR_SETWINDOWEXTEX"; break;
		case EMR_SETVIEWPORTORGEX: psz="EMR_SETVIEWPORTORGEX"; break;
		case EMR_SETVIEWPORTEXTEX:	psz="EMR_SETVIEWPORTEXTEX"; break;
		case EMR_SELECTOBJECT: psz="EMR_SELECTOBJECT";  break;
		case EMR_DELETEOBJECT: psz="EMR_DELETEOBJECT";   break;
		case EMR_GDICOMMENT: psz="EMR_GDICOMMENT";   break;
		case EMR_SETMAPMODE: psz="EMR_SETMAPMODE";   break;
		case EMR_SETBKMODE: psz="EMR_SETBKMODE";   break;
		case EMR_CREATEPALETTE: psz="EMR_CREATEPALETTE";  break;  
		case EMR_SELECTPALETTE:  psz="EMR_SELECTPALETTE"; break;
		case EMR_REALIZEPALETTE:  psz="EMR_REALIZEPALETTE"; break;
		case EMR_CREATEBRUSHINDIRECT:  psz="EMR_CREATEBRUSHINDIRECT"; break;
		case EMR_CREATEPEN:  psz="EMR_CREATEPEN"; break;
        case EMR_SETTEXTCOLOR: psz="EMR_SETTEXTCOLOR"; break;
        case EMR_SETTEXTALIGN: psz="EMR_SETTEXTALIGN"; break;
        case EMR_POLYBEZIERTO: psz="EMR_POLYBEZIERTO"; break;

        case EMR_SETBKCOLOR: psz="EMR_SETBKCOLOR"; break;
		//
		case EMR_SAVEDC: psz="EMR_SAVEDC"; break;
		case EMR_RESTOREDC: psz="EMR_RESTOREDC"; break;

		// Disegno
		case EMR_SETARCDIRECTION:  psz="EMR_SETARCDIRECTION"; break;
		case EMR_RECTANGLE: psz="EMR_RECTANGLE"; break;
		case EMR_MOVETOEX: psz="EMR_MOVETOEX"; break;
		case EMR_LINETO: psz="EMR_LINETO"; break;
		case EMR_BEGINPATH: psz="EMR_BEGINPATH"; break;
		case EMR_CLOSEFIGURE: psz="EMR_CLOSEFIGURE"; break;
		case EMR_ENDPATH: psz="EMR_ENDPATH"; break;
		case EMR_STROKEANDFILLPATH: psz="EMR_STROKEANDFILLPATH"; break;
		case EMR_EXTCREATEPEN: psz="EMR_EXTCREATEPEN"; break;
			
			
		
		case EMR_STRETCHDIBITS: psz="EMR_STRETCHDIBITS"; break;

		// Font
		case EMR_EXTCREATEFONTINDIRECTW: psz="EMR_EXTCREATEFONTINDIRECTW"; break;
		case EMR_EXTTEXTOUTA: psz="EMR_EXTTEXTOUTA"; break;
		case EMR_EXTTEXTOUTW: psz="EMR_EXTTEXTOUTW"; break;
	
	}
	return psz;
}

//
// _moAdd() - Aggiungo un oggetto allo stack
//
static void _moAdd(INT idObject,INT iType,void * pVoid) {

	S_METAOBJ sMo;
	_(sMo);
	sMo.idObject=idObject;//_sMf.lstObj->iLength+1;
	sMo.iType=iType;
	switch (iType) {
		
		case 1: // Brush
			memcpy(&sMo.sLogBrush,pVoid,sizeof(LOGBRUSH32));
			break;
	
		case 2: // Pen
			memcpy(&sMo.sLogPen,pVoid,sizeof(LOGPEN));
			break;

		case 3: // Brush
			memcpy(&sMo.sLogFont,pVoid,sizeof(LOGFONT));
			break;

		case 4: // Palette
			{
				LOGPALETTE * psSource=pVoid;
				DWORD dwSize;
				dwSize=sizeof(LOGPALETTE)+sizeof(PALETTEENTRY)*psSource->palNumEntries;
				sMo.psLogPalette=ehAlloc(dwSize);
				memcpy(sMo.psLogPalette,psSource,dwSize);
			}
		case 5:
			{
				EMREXTCREATEPEN * psPen=(EMREXTCREATEPEN *) pVoid;
				// Da fare: non faccio una copia dei bitmap usabili come penna
				if (psPen->cbBmi) ehError(); // Non gestito oggetto cone bitmap accodato
				sMo.psExtLogPen=ehAlloc(sizeof(EXTLOGPEN)+1024); // Giusto per provare
				memcpy(sMo.psExtLogPen,&psPen->elp,sizeof(EXTLOGPEN));
			
			}
			break;

	}
	lstPush(_sMf.lstObj,&sMo);

}

//
// _moGet() - > Prendo un oggetto
//

static void _moGet(DWORD idObject) {
	
	S_METAOBJ * psMo;

	for (lstLoop(_sMf.lstObj,psMo)) {
		if (psMo->idObject==idObject) {
		
			switch (psMo->iType) {
				
				case 1: // Brush
					memcpy(&_sMf.sLogBrush,&psMo->sLogBrush,sizeof(LOGBRUSH32));
					_sMf.bBrush=true;
					break;
			
				case 2: // Pen
					_sMf.psExtLogPen=NULL;
					memcpy(&_sMf.sLogPen,&psMo->sLogPen,sizeof(LOGPEN));
					_sMf.bPen=true;
					break;

				case 3: // Brush
					memcpy(&_sMf.sLogFont,&psMo->sLogFont,sizeof(LOGFONT));
					break;

				case 5: // extPen
					_sMf.psExtLogPen=psMo->psExtLogPen;
					break;

			}

			break;
		}
	}

}

static void _moRemove(DWORD idObject) {
	
	S_METAOBJ * psMo;

	for (lstLoop(_sMf.lstObj,psMo)) {
		if (psMo->idObject==idObject) {

			EH_LST_I * psLi;

			switch (psMo->iType) {
				
				case 1: // Brush
					_(_sMf.sLogBrush);
					break;
			
				case 2: // Pen
					_(_sMf.sLogPen);
					break;

				case 3: // Brush
					_(_sMf.sLogFont);
					break;

				case 4:
					ehFreePtr(&psMo->psLogPalette);
					break;

				case 5:
					ehFreePtr(&psMo->psExtLogPen);
					break;

			}

			psLi=lstSearch(_sMf.lstObj,psMo); if (!psLi) ehError();
			lstRemoveI(_sMf.lstObj,psLi);
			break;
		}
	}

}


void * pwdPathFree(EH_LST lstPathReady,BOOL bDestroy) {
	
	PWD_PTHE * pwdPath;
	if (!lstPathReady) return NULL;
	for (lstLoop(lstPathReady,pwdPath)) {
		ehFreeNN(pwdPath->psData);
	}
	if (bDestroy) lstPathReady=lstDestroy(lstPathReady); else lstClean(lstPathReady);
	return lstPathReady;

}



//
// http://msdn.microsoft.com/en-us/library/cc230818.aspx
//

static int CALLBACK _funcMetaFile (
									HDC hdc, 
									HANDLETABLE * pHandleTable,
									CONST ENHMETARECORD * pEmfRecord, 
									int iHandles, LPARAM pData)
{
 
	BOOL				$bTrace=false;
	PWD_PTHE			sPthe;
	PWD_RECT			rumRect;
	EMRSETBKMODE *		psWord=(EMRSETBKMODE *) pEmfRecord;
	EMRSELECTOBJECT *	psObj=(EMRSELECTOBJECT *) pEmfRecord;
	EMRLINETO *			psPoint=(EMRLINETO *) pEmfRecord;
	EMRRECTANGLE *		psRect=(EMRRECTANGLE *) pEmfRecord;
	PEMREXTTEXTOUTW		psTextW=(PEMREXTTEXTOUTW) pEmfRecord;
//	PWD_RECT		rumChar;

#ifdef _DEBUG
	if ($bTrace) printf(". %d: %s " ,pEmfRecord->iType,_desc(pEmfRecord->iType));
#endif

	switch (pEmfRecord->iType) {

		case EMR_HEADER: break; // Inizio file
		case EMR_EOF: 
			break; // end of file ?

		// Setta l'origine della windows
		case EMR_SETWINDOWORGEX:	
			memcpy(&_sMf.sWinOrgEx, (void *) pEmfRecord,sizeof(EMRSETVIEWPORTEXTEX)); 
		//	_sMf.pumOrg.x=_mapConvert(_sMf.sWinOrgEx.szlExtent.cx,0,false);
		//	_sMf.pumOrg.y=_mapConvert(_sMf.sWinOrgEx.szlExtent.cy,1,false);
			break;

		// Setta l'origine del view port
		case EMR_SETVIEWPORTORGEX:
			memcpy(&_sMf.sViewOrgEx, (void *) pEmfRecord,sizeof(EMRSETVIEWPORTEXTEX)); 
			break;

		//
		// Windows logic unit (dimensioni dell'area in unità logiche)
		// Indica la dimensione della pagine nella dimensione che verrà usata
		//

		case EMR_SETWINDOWEXTEX:	// Windows logic unit (dimensioni dell'area in unità logiche
			memcpy(&_sMf.sWinExtEx, (void *) pEmfRecord,sizeof(EMRSETVIEWPORTEXTEX)); 
			break;

		//
		// viewPort > Dimensione area in visione
		//

		case EMR_SETVIEWPORTEXTEX:	// Device logic unit
			memcpy(&_sMf.sViewExtEx, (void *) pEmfRecord,sizeof(EMRSETVIEWPORTEXTEX)); 
			break;


		// Nota: Gli oggetti dovrebbero essere gestiti in una lista
		case EMR_SELECTOBJECT: // Se inferiori a (?) sono oggetto creati nel metafile

		//	printf(": %x",psObj->ihObject);
			//
			// http://msdn.microsoft.com/en-us/library/office/gg264801.aspx
			//
			if (psObj->ihObject&0x80000000) {

				INT idx=psObj->ihObject&~0x80000000;
				switch (idx) {
				
					case WHITE_BRUSH:
						_(_sMf.sLogBrush);
						_sMf.sLogBrush.lbColor=RGB(255,255,255);
						_sMf.sLogBrush.lbStyle=BS_SOLID;
						_sMf.bBrush=true; // NULL
						break;

					case LTGRAY_BRUSH://        1
						_(_sMf.sLogBrush);
						_sMf.sLogBrush.lbColor=RGB(230,230,230);
						_sMf.sLogBrush.lbStyle=BS_SOLID;
						_sMf.bBrush=true; // NULL
						break;

					case GRAY_BRUSH://          2
						_(_sMf.sLogBrush);
						_sMf.sLogBrush.lbColor=RGB(128,128,128);
						_sMf.sLogBrush.lbStyle=BS_SOLID;
						_sMf.bBrush=true; // NULL
						break;

					case DKGRAY_BRUSH://        3
						_(_sMf.sLogBrush);
						_sMf.sLogBrush.lbColor=RGB(60,60,60);
						_sMf.sLogBrush.lbStyle=BS_SOLID;
						_sMf.bBrush=true; // NULL
						break;

					case BLACK_BRUSH://         4
						_(_sMf.sLogBrush);
						_sMf.sLogBrush.lbColor=0;
						_sMf.sLogBrush.lbStyle=BS_SOLID;
						_sMf.bBrush=true; // NULL
						break;
						

					case NULL_BRUSH://          5
						_(_sMf.sLogBrush);
						_sMf.sLogBrush.lbStyle=BS_NULL;
						_sMf.bBrush=true; // NULL
						break;

//					case HOLLOW_BRUSH://        NULL_BRUSH
					case WHITE_PEN://           6
						_(_sMf.sLogPen);
						_sMf.sLogPen.lopnColor=RGB(255,255,255);
						_sMf.sLogPen.lopnStyle=PS_SOLID;
						_sMf.bPen=true; // NULL
						break;

					case BLACK_PEN ://          7
						_(_sMf.sLogPen);
						_sMf.sLogPen.lopnColor=0;
						_sMf.sLogPen.lopnStyle=PS_SOLID;
						_sMf.bPen=true; // NULL
						break;

					case NULL_PEN://            8
						_(_sMf.sLogPen);
						_sMf.sLogPen.lopnStyle=PS_NULL;
						_sMf.bPen=false; // NULL
						break;


					case OEM_FIXED_FONT://      10
					case ANSI_FIXED_FONT://     11
					case ANSI_VAR_FONT://       12
					case SYSTEM_FONT://         13
						break;
					case DEVICE_DEFAULT_FONT:// 14
					case DEFAULT_PALETTE://     15
					case SYSTEM_FIXED_FONT://   16
						ehError();
				
				}
				
			} else {
				// Gestire una lista di oggetti in memoria
				// printf("qui: 
				_moGet(psObj->ihObject);
				
			}
			
			break; // ?
		case EMR_DELETEOBJECT: 
			//printf(" > Object: %x",psObj->ihObject);
		//	printf(": %x",psObj->ihObject);
			_moRemove(psObj->ihObject);
			break;

		case EMR_GDICOMMENT: 
			break;

		// Setta in che modo sono date le coordinate
		case EMR_SETMAPMODE: 
			memcpy(&_sMf.sMap, (EMRSETMAPMODE *) pEmfRecord,sizeof(EMRSETMAPMODE));
			break; // Settaggio delle dimensioni dei testi, credo

		// Colori e Background
		case EMR_SETBKMODE:  _sMf.iBkMode=psWord->iMode;  break; // ?

		// Cambi di stile

		case EMR_CREATEPALETTE: 
			{
				EMRCREATEPALETTE * psPalette=(EMRCREATEPALETTE * ) pEmfRecord;
				_moAdd(psPalette->ihPal,4,&psPalette->lgpl);
			}
			break;
		case EMR_SELECTPALETTE: 
			{

				EMRSELECTPALETTE * psPalette=(EMRSELECTPALETTE *) pEmfRecord;
				//printf(": %x",psPalette->ihPal);
			}
			break; // Seleziona una palette
		case EMR_REALIZEPALETTE: 
			{
				EMRRESIZEPALETTE * psPalette=(EMRRESIZEPALETTE *) pEmfRecord;
				//printf(": %d, %d",psPalette->ihPal,psPalette->cEntries);
			}
			break;

		//
		// NOTA: Dovrei meorizzarlo in uno stack di oggetti richiamabili
		//
		case EMR_CREATEBRUSHINDIRECT: 
			{
				EMRCREATEBRUSHINDIRECT * psLb=(EMRCREATEBRUSHINDIRECT *) pEmfRecord;
//				memcpy(&_sMf.sLogBrush,&psLb->lb,sizeof(LOGBRUSH32));
//				_sMf.bBrush=true;
				_moAdd(psLb->ihBrush,1,&psLb->lb);
			 }
			// 
			break; // Cambio di riempimento


		case EMR_CREATEPEN: 

			{
				EMRCREATEPEN * psPen=(EMRCREATEPEN *) pEmfRecord;
//				memcpy(&_sMf.sLogPen,&psLb->lopn,sizeof(LOGPEN )); _sMf.bPen=true;
				_moAdd(psPen->ihPen,2,&psPen->lopn);

			 }
			break; // Cambio di penna

		case EMR_EXTCREATEPEN:
				{
					EMREXTCREATEPEN * psPen=(EMREXTCREATEPEN *) pEmfRecord;
					_moAdd(psPen->ihPen,5,psPen);
				}
			
			break;


		// Font
		case EMR_EXTCREATEFONTINDIRECTW: 
			{
				EMREXTCREATEFONTINDIRECTW * psFont=(EMREXTCREATEFONTINDIRECTW *) pEmfRecord;
//				memcpy(&_sMf.sLogFont,&psFont->elfw.elfLogFont,sizeof(LOGFONT));
				_moAdd(psFont->ihFont,3,&psFont->elfw.elfLogFont);
	
				//printf("qui");

			}
			break;

        case EMR_SETTEXTCOLOR:
			{
				EMRSETTEXTCOLOR * psColor=(EMRSETTEXTCOLOR *) pEmfRecord;
				_sMf.crText=psColor->crColor;
			}
			break;

        case EMR_SETBKCOLOR: // Non gestito
			break;

		//
		case EMR_SAVEDC: break; // ?? 
		case EMR_RESTOREDC: break; // ??
		case EMR_SETARCDIRECTION:  break;// ?

		//
		// Disegno
		//
		case EMR_RECTANGLE: 
			
			_(rumRect);
			rumRect.left=_mapConvert(psRect->rclBox.left,0,true);
			rumRect.top=_mapConvert(psRect->rclBox.top,1,true); // pwdUm(PUM_MM,-ps->rclBox.top/100);
			rumRect.right=_mapConvert(psRect->rclBox.right,0,true); // pwdUm(PUM_MM,ps->rclBox.right/100);
			rumRect.bottom=_mapConvert(psRect->rclBox.bottom,1,true); // pwdUm(PUM_MM,-ps->rclBox.bottom/100);

			if (_sMf.bBrush) { // Se è stato settato

//					PWD_COLOR pwdColorPen;
//					memcpy(&pwdColorPen,pwdColor(_sMf.sLogPen.lopnColor),sizeof(PWD_COLOR));
				pwdRectEx(	&rumRect,
							_sMf.sLogPen.lopnStyle!=PS_NULL?pwdColor(_sMf.sLogPen.lopnColor):PDC_TRASP,_sMf.sLogPen.lopnWidth.x,_sMf.sLogPen.lopnStyle,//pwdGray(0),0,0,
							_sMf.sLogBrush.lbStyle!=BS_NULL?pwdColor(_sMf.sLogBrush.lbColor):PDC_TRASP,_sMf.sLogBrush.lbStyle,
							0,0);
			}
			
			break;		

		//
		// Muove il cursore
		//
		case EMR_MOVETOEX: 

			if (!_sMf.lstPath) {

				_sMf.ptCursor.x=psPoint->ptl.x;//_mapConvert(psPoint->ptl.x,0,true);
				_sMf.ptCursor.y=psPoint->ptl.y;//_mapConvert(psPoint->ptl.y,1,true);

			} else {

				// Aggiungo la coordinata al percorso
				PWD_POINT * pPoint=ehNew(PWD_POINT);
				_(sPthe);
				pPoint->x=_mapConvert(psPoint->ptl.x,0,true);
				pPoint->y=_mapConvert(psPoint->ptl.y,1,true);

				sPthe.enType=PHT_MOVETO;
				sPthe.dwSizeData=sizeof(PWD_POINT);
				sPthe.psData=pPoint;
				lstPush(_sMf.lstPath,&sPthe);
			
			}
			break;	// setta l'inizio

		// Disegna una linea
		case EMR_LINETO: 

			if (!_sMf.lstPath) {

				_(rumRect);
				rumRect.left=_mapConvert(_sMf.ptCursor.x,0,true);
				rumRect.top=_mapConvert(_sMf.ptCursor.y,1,true); // pwdUm(PUM_MM,-ps->rclBox.top/100);
				rumRect.right=_mapConvert(psPoint->ptl.x,0,true); // pwdUm(PUM_MM,ps->rclBox.right/100);
				rumRect.bottom=_mapConvert(psPoint->ptl.y,1,true); //pwdUm(PUM_MM,-ps->rclBox.bottom/100);

				// Linea con pennello normale
				if (!_sMf.psExtLogPen) {
				
					pwdLine(&rumRect,
							pwdColor(_sMf.sLogPen.lopnColor),
							_mapConvert(_sMf.sLogPen.lopnWidth.x,0,false),
							_sMf.sLogPen.lopnStyle);

				// Linea con pennello esteso (semi implementata
				} else {
				
					pwdLine(&rumRect,
							pwdColor(_sMf.psExtLogPen->elpColor),
							_mapConvert(_sMf.psExtLogPen->elpWidth,0,false),
							_sMf.psExtLogPen->elpPenStyle);
				
				}
				_sMf.ptCursor.x=psPoint->ptl.x;//_mapConvert(psPoint->ptl.x,0,true);
				_sMf.ptCursor.y=psPoint->ptl.y;//_mapConvert(psPoint->ptl.y,1,true);
			}
			else {
			
			
				// Aggiungo la coordinata al percorso
				PWD_POINT * pPoint=ehNew(PWD_POINT);
				_(sPthe);
				pPoint->x=_mapConvert(psPoint->ptl.x,0,true);
				pPoint->y=_mapConvert(psPoint->ptl.y,1,true);

				sPthe.enType=PHT_LINETO;
				sPthe.dwSizeData=sizeof(PWD_POINT);
				sPthe.psData=pPoint;
				lstPush(_sMf.lstPath,&sPthe);

			}
			break;		// Tira una linea

		// Disegna una linea
		case EMR_POLYBEZIERTO: 

			if (!_sMf.lstPath) {
							   
			} else {

				
				INT a;
 				EMRPOLYBEZIERTO * psPoly=(EMRPOLYBEZIERTO *) pEmfRecord;
				PWD_BEZIER * psBezier;


				_(sPthe);
				sPthe.enType=PHT_POLYBEZIERTO;
				sPthe.dwSizeData=sizeof(PWD_BEZIER)+(sizeof(PWD_POINT)*(psPoly->cptl+1));
				sPthe.psData=psBezier=ehAllocZero(sPthe.dwSizeData);
				psBezier->cCount=psPoly->cptl;
				//psBezier->arsPoint=(PWD_POINT *) dwPos;
				for (a=0;a<(INT) psBezier->cCount;a++) {
					psBezier->arsPoint[a].x=_mapConvert(psPoly->aptl[a].x,0,true);
					psBezier->arsPoint[a].y=_mapConvert(psPoly->aptl[a].y,1,true);
				}
				lstPush(_sMf.lstPath,&sPthe);

			}

			
			break;


		case EMR_BEGINPATH: // Inizia a definire un percorso
			_sMf.lstPath=lstCreate(sizeof(PWD_PTHE));
			break;

		case EMR_ENDPATH:	// Fine di definizione del percorso
			pwdPathFree(_sMf.lstPathReady,true);
			_sMf.lstPathReady=_sMf.lstPath;
			_sMf.lstPath=NULL;
			break;


		case EMR_CLOSEFIGURE:
//			printf("qui");
			if (_sMf.lstPath) {
				_(sPthe);
				sPthe.enType=PHT_CLOSEFIGURE;
				lstPush(_sMf.lstPath,&sPthe);
			}
			break;

		case EMR_STROKEPATH:
			// Linea con pennello normale
			if (!_sMf.psExtLogPen) {
			
				pwdPath(_sMf.lstPathReady,
						pwdColor(_sMf.sLogPen.lopnColor),
						_mapConvert(_sMf.sLogPen.lopnWidth.x,0,false),
						_sMf.sLogPen.lopnStyle,
						PDC_TRASP,0);

			// Linea con pennello esteso (semi implementata
			} else {
			
				pwdPath(_sMf.lstPathReady,
						pwdColor(_sMf.psExtLogPen->elpColor),
						_mapConvert(_sMf.psExtLogPen->elpWidth,0,false),
						_sMf.psExtLogPen->elpPenStyle,
						PDC_TRASP,0);
			
			}

			break;

		case EMR_STROKEANDFILLPATH:


			// Linea con pennello normale
			if (!_sMf.psExtLogPen) {
			
				pwdPath(_sMf.lstPathReady,
						pwdColor(_sMf.sLogPen.lopnColor),
						_mapConvert(_sMf.sLogPen.lopnWidth.x,0,false),
						_sMf.sLogPen.lopnStyle,
						_sMf.sLogBrush.lbStyle!=BS_NULL?pwdColor(_sMf.sLogBrush.lbColor):PDC_TRASP,_sMf.sLogBrush.lbStyle);

			// Linea con pennello esteso (semi implementata
			} else {
			
				pwdPath(_sMf.lstPathReady,
						pwdColor(_sMf.psExtLogPen->elpColor),
						_mapConvert(_sMf.psExtLogPen->elpWidth,0,false),
						_sMf.psExtLogPen->elpPenStyle,
						_sMf.sLogBrush.lbStyle!=BS_NULL?pwdColor(_sMf.sLogBrush.lbColor):PDC_TRASP,_sMf.sLogBrush.lbStyle);
			
			}

			break;

		//
		// Da vedere ********
		//

		case EMR_FILLPATH:

		case EMR_SELECTCLIPPATH: // 67 http://msdn.microsoft.com/en-us/library/windows/desktop/dd162954(v=vs.85).aspx
			//printf("EMR_SELECTCLIPPATH <--- da implementare");
			break;

		//
		// Immagine
		//
		case EMR_STRETCHDIBITS: 
			{

				EMRSTRETCHDIBITS * psSdb=(EMRSTRETCHDIBITS * ) pEmfRecord;
				BITMAP bm;
				HBITMAP hBitmap;
				PWD_SIZE sumSize;
				HDC hdc;
				

				hdc=CreateCompatibleDC(0);
				hBitmap = CreateDIBitmap(hdc, (BITMAPINFOHEADER * ) ((BYTE * ) pEmfRecord + psSdb->offBmiSrc), psSdb->iUsageSrc, NULL, NULL, 0);
				if (hBitmap)
				{
					GetObjectA(hBitmap, sizeof(BITMAP), &bm);
					SetDIBits(	hdc, 
								hBitmap,
								1, 
								abs(bm.bmHeight), 
								(BYTE *) pEmfRecord + psSdb->offBitsSrc,
								(BITMAPINFO * )((BYTE *)pEmfRecord + psSdb->offBmiSrc), 
								psSdb->iUsageSrc);
				}
				DeleteDC(hdc);

				_(rumRect);
				rumRect.left=_mapConvert(psSdb->xDest,0,true);
				rumRect.top=_mapConvert(psSdb->yDest,1,true); // pwdUm(PUM_MM,-ps->rclBox.top/100);
				sumSize.cx=_mapConvert(psSdb->cxDest,0,false); // pwdUm(PUM_MM,-ps->rclBox.top/100);
				sumSize.cy=_mapConvert(psSdb->cyDest,1,false); // pwdUm(PUM_MM,-ps->rclBox.top/100);
				rumRect.right=rumRect.left+sumSize.cx;
				rumRect.bottom=rumRect.top+sumSize.cy;

				pwdRect(&rumRect,PDC_RED,PDC_BLUE,pwdUm(PUM_PT,1));
				/*
				pwdBitmap(	rumRect.left,
							rumRect.top,
							&sumSize,
							hBitmap,
							false,
							0); // 0=Non ricodifico 1/4/8/24
							*/
		//		pwdBitmap( 
				//EMR_STRETCHDIBITS
				printf("(image)");
			}
			break; // Immagine ?


		case EMR_SETTEXTALIGN: break; // da vedere

		case EMR_EXTTEXTOUTA: break;	// Stampa del testo (A)

		case EMR_EXTTEXTOUTW: 
			{
				WCHAR * pw=(WCHAR *) ((BYTE * ) pEmfRecord+psTextW->emrtext.offString);
				WCHAR * pwStr=ehAllocZero(psTextW->emrtext.nChars*2+10);
				UTF8 * putf,*putf8Face;
				memcpy(pwStr,pw,psTextW->emrtext.nChars*2);
				putf=strEncodeW(pwStr,SE_UTF8,NULL);
				putf8Face=strEncodeW(_sMf.sLogFont.lfFaceName,SE_UTF8,NULL); strTrim(putf8Face);

				/*
				pwdText(_mapConvert(psTextW->emrtext.ptlReference.x,0,true),
						_mapConvert(psTextW->emrtext.ptlReference.y,1,true),
						PDC_BLACK,
						STYLE_NORMAL,
						DPL_LEFT,
						putf8Face,
						_mapConvert(_sMf.sLogFont.lfHeight,1,false),
						putf);
*/
				if (!strEmpty(putf)) {
					
//					PDO_TEXT sObj;
					PWD_VAL yAlt;
					CHAR szParams[800];
					
					if (strEmpty(putf)) return 0; 
					yAlt=_mapConvert(_sMf.sLogFont.lfHeight,1,false);
					sprintf(	szParams,
								"{fontface:'%s', color:'#%06x', align:'left', height:%.4f, weight:%d, italic:%d, underline:%d, baseline:%d}",
								putf8Face,
								0,
								yAlt,
								_sMf.sLogFont.lfWeight,
								_sMf.sLogFont.lfItalic,
								_sMf.sLogFont.lfUnderline,
								(yAlt<0)?true:false
								);
//					if (strlen(putf)>2)
//						printf("- %s : %d" CRLF,putf,_sMf.sLogFont.lfWeight);
					pwdTextJs(	_mapConvert(psTextW->emrtext.ptlReference.x,0,true),
								_mapConvert(psTextW->emrtext.ptlReference.y,1,true),
								szParams,
								putf);

/*
					_(sObj);
					sObj.pszText=putf;
					sObj.colText=PDC_BLACK; // <-- Da Fare
					sObj.enStyles=(_sMf.sLogFont.lfItalic?STYLE_ITALIC:0); // <-- Da Fare
					sObj.enAlign=DPL_LEFT;
					sObj.pszFontFace=putf8Face;
					sObj.umCharHeight=_mapConvert(_sMf.sLogFont.lfHeight,1,false); 

//					sObj.umInterlinea=umInterLinea;
					sObj.dwWeight=_sMf.sLogFont.lfWeight;
//					sObj.umExtraCharSpace=_sMf.sLogFont.umExtraCharSpace;
					//sObj.bJustifyLast=bJustifyLastRow;
					//sObj.iMaxRows=iMaxRows;
					_addItem(PDT_TEXTBOX,prum,&sObj,sizeof(sObj));
*/
				}

				ehFreePtrs(3,&pwStr,&putf,&putf8Face);
			}
			break;	// Stampa del testo (W)

		case EMR_MODIFYWORLDTRANSFORM:
			// printf("EMR_MODIFYWORLDTRANSFORM");
			break;

		default:
			printf("metaType ? %d" CRLF ,pEmfRecord->iType);
			break;
	
	}

	if ($bTrace) printf(CRLF);
    return true;
}

//
// Non funge
// 
static HENHMETAFILE _wmfLoad(UTF8 * pszFileNameUtf) {

	SIZE_T tSize;
	BYTE * pb;
	METAFILEPICT sMfp;
	HENHMETAFILE enh=NULL;
	typedef struct _PlaceableMetaHeader
	{
		  UINT Key;           /* Magic number (always 9AC6CDD7h) */
		  WORD Handle;        /* Metafile HANDLE number (always 0) */
		  INT16  Left;          /* Left coordinate in metafile units */
		  INT16  Top;           /* Top coordinate in metafile units */
		  INT16  Right;         /* Right coordinate in metafile units */
		  INT16  Bottom;        /* Bottom coordinate in metafile units */
		  WORD Inch;          /* Number of metafile units per inch */
		  INT32 Reserved;      /* Reserved (always 0) */
		  INT16 Checksum;      /* Checksum value for previous 10 WORDs */

	} PLACEABLEMETAHEADER;

	PLACEABLEMETAHEADER * hdr;
	HDC hdc;
//	HMETAFILE      hmf;

	return NULL;
	pb=fileMemoRead(pszFileNameUtf,&tSize);
	if (!pb) return NULL;

//     ULONGLONG size = f.GetLength();                                      

     // We will assume we have no metafiles > 4.2GB in length...

     hdr = (PLACEABLEMETAHEADER*) pb;                 
     if(hdr->Key != 0x9AC6CDD7)                                             
      { 
         return NULL;
      } 
     
     pb = pb + sizeof(PLACEABLEMETAHEADER);                              // [11]

	_(sMfp);
	sMfp.hMF = 0;
	sMfp.mm = MM_ANISOTROPIC;
	sMfp.xExt = 0;
	sMfp.yExt = 0;

	tSize-=sizeof(PLACEABLEMETAHEADER);
	hdc=CreateCompatibleDC(0);
    enh = SetWinMetaFileBits((UINT) tSize, pb, hdc, &sMfp);  // [12]
	DeleteDC(hdc);
     return enh ;
         
  //   meta = enh;
//     ProcessMetafile(filename);                                              // [14]

    // return meta != NULL;                                                    // [15]

}

//
// pwdMetaFile() > 12/2013
//
void		pwdMetaFile(PWD_POINT *	ppumPos,		// Posiziono il meta
						PWD_SIZE  *	psumSize,		// Indico dimensioni orizzontali
						PWD_ALIGN	enAlign,		// Posizionamento
						UTF8 *		pszFileNameUtf,
						BOOL		bBestInFit,		// T/F se deve calcolare la maggiore dimensione possibile
						BOOL		bOnlyCalc,		// T/F Solo calcolo del posizionamento, usato per predeterminare l'occupazione e la dimenzione
						PWD_RECT  *	precMeta)		// Ritorna l'area occupata (se richiesto)

{

	ENHMETAHEADER	emh ;
	HENHMETAFILE	hemf ;
	WCHAR			wcsFileName[500];
	PWD_SIZE		umsMeta; // Area di stampa
	_(_sMf);
	_sMf.lstObj=lstCreate(sizeof(S_METAOBJ));
	if (strEmpty(pszFileNameUtf)) ehError();
	wcsCpyUtf(wcsFileName,pszFileNameUtf,sizeof(wcsFileName));

	//
	// Leggo il file
	//
	if (!fileCheck(pszFileNameUtf)) ehExit("Il file %s non esiste",pszFileNameUtf);
	hemf = GetEnhMetaFileW(wcsFileName); 
	if (!hemf) {
		ehError(); // è un wmf ?
	}
	GetEnhMetaFileHeader (hemf, sizeof (emh), &emh) ;
	
	//
	// NOTA: Le dimensioni del template sono (dovrebbero essere) in centesimi di millimetro
	//
//	#define WIN_X_FACTOR 7.0754716981132077
//	#define WIN_Y_FACTOR 5.5035773252614206

	_sMf.sumSource.cx=pwdUm(PUM_MM,(double) (emh.rclFrame.right - emh.rclFrame.left)/100);
	_sMf.sumSource.cy=pwdUm(PUM_MM,(double) (emh.rclFrame.bottom - emh.rclFrame.top)/100);

	// 
	// Coordinate e dimensioni
	//
 	// if (ppumPos) memcpy(&umpMeta,ppumPos,sizeof(PWD_POINT)); else {umpMeta.x=_sPower.rumPage.left; umpMeta.y=_sPower.rumPage.top;}
	if (psumSize) memcpy(&umsMeta,psumSize,sizeof(PWD_SIZE)); //else {memcpy(&umsMeta,&_sPower.sumPage,sizeof(PWD_SIZE)); bBestInFit=true;}

	//
	//  bBestInFit - Inserisce e centra nello spazio disponibile
	//
	if (bBestInFit) {

		// Calcolo dimensioni della massima area stampabile
		//

		if (!psumSize) memcpy(&umsMeta,&_sPower.sumPage,sizeof(PWD_SIZE));
		//
		// Scelgo la scala più appropriata (best in fit) 
		// Per stampare il template nell'area di stampa massima
		//
		if (!umsMeta.cx) _sMf.dScale = (double) umsMeta.cy / _sMf.sumSource.cy;
		else if (!umsMeta.cy) _sMf.dScale = (double) umsMeta.cx / _sMf.sumSource.cx;
		else _sMf.dScale = min ((double) umsMeta.cx / _sMf.sumSource.cx, (double) umsMeta.cy / _sMf.sumSource.cy) ;


	} else {
		
//		memcpy(&_sMf.sumArea,&_sMf.sumSource,sizeof(_sMf.sumSource));
//		pwdSizeCalc(&_sMf.sumArea,prumPrintArea);
		_sMf.dScale=1;


	}

	//
	// Ridimensiono il template in scala
	// 
	// printf("Dimensioni meta: %.2fmm x %.2fmm (zoom: %.2f%%)" CRLF,_sMf.sumSource.cx,_sMf.sumSource.cy,_sMf.dScale*100);
	_sMf.sumDest.cx = (_sMf.dScale * _sMf.sumSource.cx) ; if (!umsMeta.cx)  umsMeta.cx=_sMf.sumDest.cx;
	_sMf.sumDest.cy = (_sMf.dScale * _sMf.sumSource.cy) ; if (!umsMeta.cy)  umsMeta.cy=_sMf.sumDest.cy;

	//
	// Trasformo la dimensione Reale in pixel 
	//
	/*
	_sEmr.sizSourcePixel.cx=(DWORD) pwdUm(PUM_DTX,sumSource.cx);
	_sEmr.sizSourcePixel.cy=(DWORD) pwdUm(PUM_DTY,sumSource.cy);
	_sEmr.sizDestPixel.cx=(DWORD) pwdUm(PUM_DTX,sumDest.cx);
	_sEmr.sizDestPixel.cy=(DWORD) pwdUm(PUM_DTY,sumDest.cy);
*/
	if (!enAlign) enAlign=PDA_CENTER|PDA_MIDDLE;
	
	switch (enAlign&0xf) {
		
		case PDA_LEFT:
			if (ppumPos) 
					_sMf.rumMeta.left=ppumPos->x; 
					else
					_sMf.rumMeta.left=_sPower.rumPage.left;
			break;


		case PDA_CENTER:
			
			if (ppumPos) // Posiziono e centro al rettangolo indicato
					_sMf.rumMeta.left=ppumPos->x+(umsMeta.cx-_sMf.sumDest.cx)/2; 
					else
					_sMf.rumMeta.left=_sPower.rumPage.left+(_sPower.sumPage.cx-_sMf.sumDest.cx)/2; 
			break;

		case PDA_RIGHT:
			if (ppumPos) 
					_sMf.rumMeta.left=ppumPos->x+umsMeta.cx-_sMf.sumDest.cx; 
					else
					_sMf.rumMeta.left=_sPower.rumPage.right-_sMf.sumDest.cx; 
			break;

	
	}

	switch (enAlign&0xf0) {
	
		case PDA_TOP:
			if (ppumPos) 
				_sMf.rumMeta.top=ppumPos->y; 
				else
				_sMf.rumMeta.top=_sPower.rumPage.top;
			break;

		case PDA_MIDDLE:
			if (ppumPos) 
				_sMf.rumMeta.top=ppumPos->y+(umsMeta.cy-_sMf.sumDest.cy)/2; 
				else
				_sMf.rumMeta.top=_sPower.rumPage.top+(_sPower.sumPage.cy-_sMf.sumDest.cy)/2; 
			break;

		case PDA_BOTTOM:
			if (ppumPos) 
				_sMf.rumMeta.top=ppumPos->y+umsMeta.cy-_sMf.sumDest.cy; 
				else
				_sMf.rumMeta.top=_sPower.rumPage.bottom-_sMf.sumDest.cy; 
			break;

	}			
	
	_sMf.rumMeta.right=_sMf.rumMeta.left+_sMf.sumDest.cx;
	_sMf.rumMeta.bottom=_sMf.rumMeta.top+_sMf.sumDest.cy;

	if (precMeta) memcpy(precMeta,&_sMf.rumMeta,sizeof(PWD_RECT));
	if (!bOnlyCalc)	EnumEnhMetaFile (NULL, hemf, _funcMetaFile, NULL, NULL) ;

	pwdPathFree(_sMf.lstPathReady,true);
	lstDestroy(_sMf.lstObj);
	DeleteEnhMetaFile(hemf) ; // Libero la memoria

}



