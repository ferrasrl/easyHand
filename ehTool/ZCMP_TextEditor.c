//
//  ZCMP_TextEditor
//               Gestione di editor testi      
//               Edita un testo tipo word      
//               versione Windows/ZonaPlus     
//                                             
//               by Ferrà Art & Technology 2000 
//				 by Ferrà srl 2008
//

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/ZCMP_TextEditor.h"

static INT IPT_ins=0;
	// extern SINT IPT_ins;
// 	extern SINT WIN_fz;

	static SINT L_TextEditorEvent(EH_TEXTEDITOR *TE,EH_EVENT *psEvent);
	static BOOL L_SpecialKeyManager_A(EH_TEXTEDITOR *psTE,BYTE cSecond);
	static BOOL L_SpecialKeyManager_B(EH_TEXTEDITOR *psTE,BYTE cFirst);
	
	static void TE_LineWrite(EH_TEXTEDITOR *psTE,LONG RigaFile,CHAR *Rig);
	static void TE_LineRead(EH_TEXTEDITOR *psTE,LONG RigaFile);
	static void TE_LineInsert(EH_TEXTEDITOR *psTE,LONG RigaFile,LONG num);
	static void TE_LineDelete(EH_TEXTEDITOR *psTE,LONG RigaFile,LONG num);
	static void TE_AreaRefresh(EH_TEXTEDITOR *psTE,SINT iBegin,SINT iEnd);
	static void TE_LineDraw(EH_TEXTEDITOR *psTE,HDC hDC,SINT Py,SINT Line);
	static void L_TextEditorControl(EH_TEXTEDITOR *psTE);
	
	static SINT LTE_SelOrder(TED_SEL *psBegin,TED_SEL *psEnd);//EH_TEXTEDITOR *psTE);
	static void LTE_ClearSelection(EH_TEXTEDITOR *psTE);
	static void SELcopia(EH_TEXTEDITOR *TE);
	static void SELincolla(EH_TEXTEDITOR *TE,CHAR *pString);
	static void SELdelete(EH_TEXTEDITOR *TE);
	static void LTE_SetSelection(EH_TEXTEDITOR *psTE,SINT iTipo);

	#define MARGINDX 2

//#define TEMAX 3

//static EH_TELIST TEList[TEMAX];
//static BOOL fReset=TRUE;
#define TE_FINDOBJ  0
#define TE_FINDHWND 1

#define LOCAL_BREAK -120

// Poi da togliere
void Wdispfh(SINT x,SINT y,LONG col1,LONG col2,HFONT hfont,CHAR *String);
SINT Wfont_dimh(CHAR *Str,UINT Num,HFONT hFont);
static void SELreset(EH_TEXTEDITOR *TE);
/*
static SINT TEFind(SINT iCosa,void *ptr)
{
	SINT a;
	for (a=0;a<TEMAX;a++)
	{
		switch (iCosa)
		{
			case TE_FINDOBJ:  if (TEList[a].lpObj==(struct OBJ *) ptr) return a;
							  break;
			case TE_FINDHWND: if (TEList[a].TE.hWnd==(HWND) ptr) return a;
							  break;
		}
	}
	return -1;
}

static SINT TEAlloc(struct OBJ *obj)
{
	SINT a;
	for (a=0;a<TEMAX;a++)
	{
	  if (TEList[a].lpObj==NULL)
	  {
		TEList[a].lpObj=obj;
		return a;
	  }
	}
	ehExit("LV: overload");
	return 0;
}
*/

void * EhTextEditor(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr)
{
 struct WS_DISPEXT *DExt=ptr;
 EH_TEXTEDITOR *psTE;
 static struct OBJ *lpFocusObj=NULL;
 EH_EVENT *psEvent;

//if (objCalled==NULL) ehExit("ELV: NULL %d",cmd);
/* 
 if (fReset)
 {
	 memset(&TEList,0,sizeof(EH_TELIST)*TEMAX);
	 fReset=FALSE;
	 //return 0;
 }
*/

 // Cerco ed alloco all'occorenza un TexEH_TEXTEDITORor
 //iTEIndex=TEFind(TE_FINDOBJ,objCalled);
 //if (iTEIndex<0) iTEIndex=TEAlloc(objCalled);
 psTE=objCalled->pOther;//&TEList[iTEIndex].TE;
 switch(cmd)
	{
		case WS_INF: return psTE;//&TEList[iTEIndex];
 
		case WS_CREATE:
			psTE=objCalled->pOther=ehAllocZero(sizeof(EH_TEXTEDITOR));
			memset(psTE,0,sizeof(EH_TEXTEDITOR));
			psTE->LogFont.lfHeight=14;
			psTE->pObj=objCalled;
			strcpy(psTE->LogFont.lfFaceName,"FixedSys");
			psTE->Col1=sys.arsColor[15];
			psTE->Col2=sys.arsColor[0];
			psTE->ColCur=sys.arsColor[14];
			psTE->MaxLines=5000L;
			psTE->SizeLine=400;
			psTE->Px=objCalled->px;
			psTE->Py=objCalled->py;
			psTE->Lx=objCalled->col1;
			psTE->Ly=objCalled->col2;
			psTE->fNoYOffsetCalc=FALSE;
			//LTE_ClearSelection(psTE);

			// Creo hFont
			DirectTextEditor(psTE,WS_OPEN,0,NULL);
			DirectTextEditor(psTE,WTE_NEW,0,NULL);
			break;

//		case WS_CLOSE: // Distruzione
		case WS_DESTROY: // Distruzione
			DirectTextEditor(psTE,WS_CLOSE,0,NULL);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_DO: // Spostamento / Ridimensionamento
			psTE->Px=objCalled->px;
			psTE->Py=objCalled->py;
			psTE->Lx=objCalled->col1;
			psTE->Ly=objCalled->col2;

			DirectTextEditor(psTE,WTE_REDRAW,FALSE,NULL);
			//MoveWindow(objCalled->hWnd,DExt->px,DExt->py,DExt->lx,DExt->ly,TRUE);
			break;

		case WS_DISPLAY: break;

		case WS_SEL: // Ingresso con focus nella gestione dell'oggetto
			break;
/*
			//efx1();
			if (lpFocusObj==objCalled) break;
			lpFocusObj=objCalled;
			
			//obj_MouseGhost(lpFocusObj->nome); // Nascondo l'intercettazione
			obj_lock(lpFocusObj->nome);
			DirectTextEditor(WTE_CURSORON,0,NULL,lpTe);
			//dispx("INTRO    ");  ehSleep(100);
			while (TRUE)
			{
			 ms=DirectTextEditor(WTE_EDIT,0,NULL,lpTe);

			 if (ms==LOCAL_BREAK) break;

			 //dispx("%d)           ",ms);
			 if (sys.sLastEvent.iEvent==EE_LBUTTONDOWN) break; // Fuori Focus
			 if (ms==IN_OBJ) 
				{//win_infoarg("[%s]",sys.sLastEvent.szObjName); 
				 obj_addevent(sys.sLastEvent.szObjName);
				 break;
				}

			 if (key_press(ESC)||obj_press("ESCOFF")) break;
			 if (key_press2(KEY_F9)) break;
			}
			
			// Ripristino l'intercettazione
			obj_unlock(lpFocusObj->nome);
			//obj_RefreshSolo(lpFocusObj->nome,FALSE,FALSE);
			lpFocusObj=NULL;
			DirectTextEditor(WTE_CURSOROFF,0,NULL,lpTe);
			//win_infoarg("STOP");
*/
			break;

		case WS_EVENT:

			psEvent=(EH_EVENT *) ptr; 
			switch (psEvent->iEvent)
			{
				//
				// Controllo del click sinistro
				//
				case EE_NONE: break;

				case EE_LBUTTONDOWN:
				case EE_RBUTTONDOWN:
					L_TextEditorEvent(psTE,psEvent);
					return "FOCUS";

				case EE_MOUSEWHEEL:
//				case EE_MOUSEWHEELUP:
//				case EE_MOUSEWHEELDOWN: 
				case EE_LBUTTONUP:
				case EE_CHAR:
					//DirectTextEditor(WTE_EDIT,0,psEvent,lpTe);
					L_TextEditorEvent(psTE,psEvent);
					L_TextEditorControl(psTE);
					//efx2();
					break;

/*
					// Opzioni (se ci sono)
					if (psSearch->iLayOut==1&&psEvent->sPoint.x<24)
					{
						obj_putevent("%sFLT",objCalled->nome); return "ESC";
					}

					// Clicco sul tasto clean
					if (psSearch->bButtonClean&&psEvent->sPoint.x>(psSearch->lpObj->sClientSize.cx-27)) 
					{
						//efx2();
						*psSearch->pInputValue=0;
						*psSearch->pwInputValue=0;
						obj_putevent("%sCLS",psSearch->lpObj->nome); // Segnalo pulizia
					}

					// Pulizia della ricerca ( DA FARE)

					LEditCreate(psSearch);*/
					

				case EE_FOCUS:
//					if (!strcmp(psEvent->szObjName,objCalled->nome))
//					{
						//LEditCreate(psSearch); 
						DirectTextEditor(psTE,WTE_CURSORON,0,NULL);
						return "FOCUS";
//					}
					break;

				case EE_BLUR:
//					efx1();
//					if (!strcmp(psEvent->szObjName,objCalled->nome))
//					{
						DirectTextEditor(psTE,WTE_CURSOROFF,0,NULL);
//					}
					break;
			}
			break;
	}
	return NULL;
}


/*
static void SelVedi(EH_TEXTEDITOR *psTE)
{
   CHAR Serv[120];
   //S_WINSCENA WScena;
   sprintf(Serv," (%d,%d) - (%d,%d) ",
			psTE->sSelBegin.iChar,psTE->sSelBegin.iRow,
		    psTE->sSelEnd.iChar,psTE->sSelEnd.iRow);
  
}
*/


static LRESULT CALLBACK EhTextEditorProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

//EH_TEXTEDITOR *LastTE=NULL;

/*
SINT WFontDimWnd(CHAR *Str,UINT Num,HWND hWnd,LOGFONT *LogFont)
{
 HDC hDC;
 SINT Valore;
 hDC=GetDC(hWnd);
 Valore=WFontDim(Str,Num,hDC,LogFont); 
 ReleaseDC(hWnd,hDC);
 return Valore;
}
*/


// ------------------------------
//  Scrive una riga nella file  !
// ------------------------------
void TE_LineWrite(EH_TEXTEDITOR *psTE,LONG RigaFile,CHAR *Rig)
{
  psTE->Touch=ON;
  memoWrite(psTE->Hdl,RigaFile*psTE->SizeLine,Rig,psTE->SizeLine);
}
// -----------------------------
//  Legge una riga dal file    !
// -----------------------------
void TE_LineRead(EH_TEXTEDITOR *psTE,LONG RigaFile) {memoRead(psTE->Hdl,RigaFile*psTE->SizeLine,psTE->pszBufferRow,psTE->SizeLine);}

void Tonk(void) {efx1();};//sonic(5000,1,1,2,2,5);}

// +-----------------------------------------+
// | RIGAInsert                              |
// |                                         |
// |                                         |
// |         Ferrà Art & Technology (c) 1997 |
// +-----------------------------------------+
void TE_LineInsert(EH_TEXTEDITOR *psTE,LONG RigaFile,LONG num)
{
 LONG l;
 //printf("%d",psTE->SizeLine);
 // -----------------------------------------------
 // Inserisce una riga                            !
 // -----------------------------------------------
 for (l=psTE->Lines;l>=RigaFile;l--)
			{
			 memoRead(psTE->Hdl,l*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
			 memoWrite(psTE->Hdl,(l+num)*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
			}
 memset(psTE->Riga2,0,psTE->SizeLine);
 //RIGAWrite(RigaFile,Riga);
 memoWrite(psTE->Hdl,RigaFile*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
 psTE->Lines+=num;
}

void TE_LineDelete(EH_TEXTEDITOR *psTE,LONG RigaFile,LONG num)
{
 LONG l;

 // -----------------------------------------------
 // Cancella una riga                             !
 // -----------------------------------------------
 for (l=RigaFile;l<psTE->Lines;l++)
			{
			 memoRead(psTE->Hdl,(l+num)*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
			 memoWrite(psTE->Hdl,l*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
			}

 memset(psTE->Riga2,0,psTE->SizeLine);
 //RIGAWrite(RigaFile,Riga);
 memoWrite(psTE->Hdl,(psTE->Lines-1)*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
 psTE->Lines-=num;
}

// -----------------------------------------------
// HCTDisp
// Visualizzazione complessa a multi-colore/multitipo/multifont/multistronz
// Ritorna la dimensione della stringa stampata
//
static SINT HCTDisp(SINT x,SINT y,BYTE *HCTArray)
{
  BYTE *lp;
  HCT *lpHct;
  SINT lx=0;

  lp=HCTArray;
  while (TRUE)
  {
    lpHct=(HCT *) lp; 
	if (!lpHct->iLen) break;
	
	lp+=sizeof(HCT);
	
	//if (fSelect) {col1=lCol2;col2=lCol1;} else {col1=lpHct->lCol1;col2=lpHct->lCol2;}
    Wdispfh(x+lx,y,lpHct->lCol1,lpHct->lCol2,lpHct->hFont,lp);
	lx+=Wfont_dimh(lp,lpHct->iLen,lpHct->hFont);
	lp+=lpHct->iLen+1;
  }
  return lx;
}

// -----------------------------------------------
// HCTLen
// Ritorna la dimensione in pixel orizzontali oggetto HCT
//
static SINT HCTLen(BYTE *HCTArray)
{
  BYTE *lp;
  HCT *lpHct;
  SINT lx=0;
  lp=HCTArray;
  while (TRUE)
  {
    lpHct=(HCT *) lp; 
	if (!lpHct->iLen) break;
	lp+=sizeof(HCT);
	lx+=Wfont_dimh(lp,lpHct->iLen,lpHct->hFont);
	lp+=lpHct->iLen+1;
  }
  return lx;
}

// ---------------------------------------
// HCTMakeDisp
// 1) Costrusce una HCT object
// 2) Lancia la funzione esterna per variazione sull'oggetto
// 3) Stampa l'oggetto HCT
//
// Ritorna la dimensione della stringa stampata

static SINT HCTMakeDisp(EH_TEXTEDITOR *psTE,SINT x,SINT y,LONG lCol1,LONG lCol2,CHAR *String,BOOL fSelect,SINT iLine)
{
  const SINT iSize=4096;
  BYTE *HCTObject=ehAlloc(iSize);
  SINT lx;
  HCT *lpHct;
//  LONG col1,col2;

  memset(HCTObject,0,iSize);

  // --------------------------------------------
  // Creo una elaborazione base
  lpHct=(HCT *) HCTObject;
  memset(lpHct,0,sizeof(HCT));
  lpHct->lCol1=lCol1;
  lpHct->lCol2=lCol2;
  lpHct->hFont=psTE->hFont;
  lpHct->iLen=(INT) strlen(String);
  strcpy(HCTObject+sizeof(HCT),String);

  // --------------------------------------------
  // Chiedo di elaborare la stringa
  // Se esiste una funzione esterna, chiedo di compormi la stringa in
  // "HCT format" e di metterna in p
  if (psTE->FuncExtern) (*psTE->FuncExtern)(WS_DO,(LONG) HCTObject,String,psTE,fSelect,iLine);
  lx=HCTDisp(x,y,HCTObject);
  ehFree(HCTObject);
  return lx;
}

// ---------------------------------------
// HCTMakeDim
// 1) Costrusce una HCT object della lunghezza desiderata
// 2) Lancia la funzione esterna per variazione sull'oggetto
//
// Ritorna la dimensione della stringa stampata

static SINT HCTMakeDim(EH_TEXTEDITOR *psTE,CHAR *String,SINT iDim,SINT iLine)
{
//  const SINT iSize=1024;
  const SINT iSize=4096;
  BYTE *HCTObject=ehAlloc(iSize);
  SINT lx;
  HCT *lpHct;
  CHAR *lpNewString=ehAlloc(iDim+1);
  
  // Nuova stringa
  memcpy(lpNewString,String,iDim); 
  lpNewString[iDim]=0;
  
  memset(HCTObject,0,iSize);

  // --------------------------------------------
  // Creo una elaborazione base
  lpHct=(HCT *) HCTObject;
  memset(lpHct,0,sizeof(HCT));
  lpHct->lCol1=0;
  lpHct->lCol2=0;
  lpHct->hFont=psTE->hFont;
  lpHct->iLen=(INT) iDim;
  strcpy(HCTObject+sizeof(HCT),lpNewString);

  // --------------------------------------------
  // Chiedo di elaborare la stringa
  // Se esiste una funzione esterna, chiedo di compormi la stringa in
  // "HCT format" e di metterna in p
  if (psTE->FuncExtern) (*psTE->FuncExtern)(WS_DO,(LONG) HCTObject,lpNewString,psTE,FALSE,iLine);
  lx=HCTLen(HCTObject);
  ehFree(HCTObject);
  ehFree(lpNewString);
  return lx;
}

// ---------------------------------------
// LDisplay
// Display locale
// Ritorna la dimenzione della stringa stampata

static SINT LDisplay(EH_TEXTEDITOR *psTE,SINT x,SINT y,LONG col1,LONG col2,CHAR *String,BOOL fSelect,SINT iLine)
{
	SINT spiazi;
	SINT a;
	if (!psTE->fHCT) 
		{
		 if (fSelect) {a=col2; col2=col1; col1=a;}
		 Wdispfh(x,y,col1,col2,psTE->hFont,String);
		 spiazi=Wfont_dimh(String,strlen(String),psTE->hFont);
		}
		else
		{
		 spiazi=HCTMakeDisp(psTE,x,y,col1,col2,String,fSelect,iLine);
		}
 return spiazi;
}
	  

// -------------------------------------------
// | TE_AreaRefresh   Visualizza i dati del			 |
// |          text-windows     				 |
// |                               			 |
// |                            by Ferrà 1997|
// -------------------------------------------

void TE_LineDraw(EH_TEXTEDITOR *psTE,HDC hDC,SINT Py,SINT Line)
{
	SINT  x1,y1,x2,cam_x,cam_y;
	SINT  spiazi,lx,ly,refre;
	LONG koffset;
	SINT  ncam;
	LONG offset;
	SINT  fontalt;
	CHAR *PtRiga=psTE->Riga2;

	SINT daPulire=ON;
	LONG ColPulish;
	SINT Col1,Col2;
	BOOL bSelection;

	TED_SEL sSelBegin,sSelEnd;
	memcpy(&sSelBegin,&psTE->sSelBegin,sizeof(TED_SEL));
	memcpy(&sSelEnd,&psTE->sSelEnd,sizeof(TED_SEL));
	if (psTE->bSelection) LTE_SelOrder(&sSelBegin,&sSelEnd);

	// 
	//  Swapping delle Coordinate di Selezione  
	// 

	Col1=psTE->Col1; Col2=psTE->Col2;
	fontalt=psTE->Falt;//font_alt(poj->fonthdl,poj->nfi);
	refre=ON;//psTE->refre;

	lx=psTE->Lx; ncam=psTE->ncam; ly=psTE->Ly;

    x1=0;y1=Py;  
	x2=psTE->Lx; 
    if (Line<0) Line=0; 

	offset=psTE->Y_offset;
	koffset=psTE->Y_koffset;

	cam_x=x1+MARGINDX; cam_y=y1;

	// Tapullo
	if (Line>=psTE->Lines) {boxp(x1,cam_y,x2,cam_y+fontalt-1,4,SET); return;}
	memoRead(psTE->Hdl,Line*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
	ColPulish=Col2;
	spiazi=0;

	// ------------------------------------------------------
	// Stampa con la SELEZIONE                              !
	// ------------------------------------------------------

	// Nessuna selezione se .....
	bSelection=FALSE;
	if (psTE->bSelection)
	{
		bSelection=TRUE;
	    if ((Line==sSelEnd.iRow)&&(sSelEnd.iChar==0)) bSelection=FALSE; // Sono sull'ultima linea e x=0
		if ((Line<sSelBegin.iRow)||(Line>sSelEnd.iRow)) bSelection=FALSE; // sono sopra o sotto la selezione
		if ((psTE->sSelBegin.iChar==psTE->sSelEnd.iChar)&&(psTE->sSelBegin.iRow==psTE->sSelEnd.iRow)) bSelection=FALSE;
	}
	if (!bSelection)
	{
		Tboxp(x1,cam_y,x2-1,cam_y+psTE->Falt-1,Col2,SET);
		spiazi=LDisplay(psTE,cam_x-(SINT) psTE->X_offset,cam_y,Col1,Col2,psTE->Riga2,FALSE,Line);
	}
	else
	{
		//
		// Stampa con selezione
		//
		while (TRUE)
		{

			if ((Line>sSelBegin.iRow)&&(Line<sSelEnd.iRow))
			{
//				Tboxp(x1,cam_y,x2-1,cam_y+psTE->Falt-1,Col2,SET);
				spiazi=LDisplay(psTE,cam_x-(SINT) psTE->X_offset,cam_y,Col1,Col2,psTE->Riga2,TRUE,Line);

				//spiazi=Wfont_len(psTE->Riga2,&psTE->LogFont);
				ColPulish=Col1;

				// Richiedo il colore della selezione alla funzione esterna (se me lo da)
				if (psTE->FuncExtern) 
				{
					CHAR *p;
					p=(*psTE->FuncExtern)(WS_INF,10,NULL,psTE,TRUE,Line);
					if (p) ColPulish=atoi(p);
				}
				break;
			}

			// 
			// TESTA: Non selezionata   
			// 
			if (Line==sSelBegin.iRow)
			{
				CHAR bak;

				if (sSelBegin.iChar>0)
				{
					bak=*(psTE->Riga2+sSelBegin.iChar);
					*(psTE->Riga2+sSelBegin.iChar)=0;

					//spiazi=Wfont_len(PtRiga,&psTE->LogFont);
					if (psTE->fHCT) spiazi=HCTMakeDim(psTE,PtRiga,strlen(PtRiga),Line);
								else
								spiazi=Wfont_dimh(PtRiga,strlen(PtRiga),psTE->hFont);

					LDisplay(psTE,cam_x-(SINT) psTE->X_offset,cam_y,Col1,Col2,psTE->Riga2,FALSE,Line);
					*(psTE->Riga2+sSelBegin.iChar)=bak;
				}

			 // ------------------------------------
			 // Y1=Y2 Corpo Centrale SELEZIONATO   | 3 Parti  NO|SI|NO
			 // ------------------------------------
			 if (Line==sSelEnd.iRow) 
				{bak=*(psTE->Riga2+sSelEnd.iChar);
			     *(psTE->Riga2+sSelEnd.iChar)=0;
			     LDisplay(psTE,cam_x-(SINT) psTE->X_offset+spiazi,cam_y,Col1,Col2,PtRiga+sSelBegin.iChar,TRUE,Line);
                 //spiazi+=Wfont_len(PtRiga+sSelBegin.iChar,&psTE->LogFont);
				 if (psTE->fHCT) spiazi+=HCTMakeDim(psTE,PtRiga+sSelBegin.iChar,strlen(PtRiga+sSelBegin.iChar),Line);
							   else
							   spiazi+=Wfont_dimh(PtRiga+sSelBegin.iChar,strlen(PtRiga+sSelBegin.iChar),psTE->hFont);
				 *(psTE->Riga2+sSelEnd.iChar)=bak;

				 // Coda Non selezionata     !
				 LDisplay(psTE,cam_x-(SINT) psTE->X_offset+spiazi,cam_y,Col1,Col2,PtRiga+sSelEnd.iChar,FALSE,Line);
			     //spiazi+=Wfont_len(PtRiga+sSelEnd.iChar,&psTE->LogFont);
				 if (psTE->fHCT) spiazi+=HCTMakeDim(psTE,PtRiga+sSelEnd.iChar,strlen(PtRiga+sSelEnd.iChar),Line);
							   else
							   spiazi+=Wfont_dimh(PtRiga+sSelEnd.iChar,strlen(PtRiga+sSelEnd.iChar),psTE->hFont);
			     ColPulish=Col2;
				}
			 // ------------------------------------
			 // Y1!=Y2 Corpo Centrale SELEZIONATO   | 2 Parti  NO|SI
			 // ------------------------------------
                else
				{
			     LDisplay(psTE,cam_x-(SINT) psTE->X_offset+spiazi,cam_y,Col1,Col2,PtRiga+sSelBegin.iChar,TRUE,Line);
                 //spiazi+=Wfont_len(PtRiga+sSelBegin.iChar,&psTE->LogFont);
				 if (psTE->fHCT) spiazi+=HCTMakeDim(psTE,PtRiga+sSelBegin.iChar,strlen(PtRiga+sSelBegin.iChar),Line);
							   else
							   spiazi+=Wfont_dimh(PtRiga+sSelBegin.iChar,strlen(PtRiga+sSelBegin.iChar),psTE->hFont);

				 ColPulish=Col1;
				 // Richiedo il colore della selezione alla funzione esterna (se me lo da)
				 if (psTE->FuncExtern) 
				 {CHAR *p;
				  p=(*psTE->FuncExtern)(WS_INF,10,NULL,psTE,TRUE,Line);
				  if (p) ColPulish=atoi(p);
				 }
				}
				break;
			}

			if (Line==sSelEnd.iRow)
				{
				 CHAR bak;
				 // Non selezionato
				 bak=*(psTE->Riga2+sSelEnd.iChar); *(psTE->Riga2+sSelEnd.iChar)=0;
				 spiazi=LDisplay(psTE,cam_x-(SINT) psTE->X_offset,cam_y,Col1,Col2,PtRiga,TRUE,Line);
				 //spiazi=Wfont_len(PtRiga,&psTE->LogFont);
				 *(psTE->Riga2+sSelEnd.iChar)=bak;

				 //spiazi+=Wfont_len(PtRiga+sSelEnd.iChar,&psTE->LogFont);
				 spiazi+=LDisplay(psTE,cam_x-(SINT) psTE->X_offset+spiazi,cam_y,Col1,Col2,PtRiga+sSelEnd.iChar,FALSE,Line);
				 //spiazi+=Wfont_len(PtRiga+sSelEnd.iChar,&psTE->LogFont);
				 ColPulish=Col2;
			   break;
				}
		} // Fine while
	} // Fine Else con selezione
	 
	//if (daPulire)
//	Tboxp(cam_x+spiazi-(SINT) psTE->X_offset,cam_y,x2+1,cam_y+fontalt-1,RGB(255,255,0),SET);
	Tboxp(cam_x+spiazi-(SINT) psTE->X_offset,cam_y,x2+1,cam_y+fontalt-1,ColPulish,SET);

//	psTE->Y_koffset=psTE->Y_offset;
//	psTE->X_koffset=psTE->X_offset;

//	if (refre==ON) psTE->refre=OFF; // 	refresh schermo fatto
}

void TE_LineRefresh(EH_TEXTEDITOR *psTE,LONG Line,CHAR *pszBufferRow)
{
	TE_LineWrite(psTE,Line,pszBufferRow);
	psTE->refre=ON;
	TE_AreaRefresh(psTE,Line,Line);//psTE->Cy,psTE->Cy);
}

static SINT L_Shift(void)
{
    if (key_pressS(_SHIFTDX|_SHIFTSX,OR)) return ON;
	return OFF;
}

static SINT L_Control(void)
{
    if (key_pressS(_CTRL,AND)) return ON;
	return OFF;
}

static void SELreset(EH_TEXTEDITOR *psTE)
{
 SINT k1,k2;
 SINT k;

 if (psTE->sSelEnd.iRow==-1) return;
 k1=(SINT) (psTE->sSelBegin.iRow-psTE->Y_offset);
 k2=(SINT) (psTE->sSelEnd.iRow-psTE->Y_offset);     

 //printf("DEL %d->%d",k1,k2);
 psTE->sSelEnd.iChar=-1;
 psTE->sSelEnd.iRow=-1;
 if (k2<0) return;

 if (k1<0) k1=0;
 if (k2>psTE->ncam) k2=psTE->ncam;
 psTE->refre=ON;

 if (k1>k2) {k=k2; k2=k1; k1=k;}

 TE_AreaRefresh(psTE,k1,k2);
 LTE_ClearSelection(psTE);
 //psTE->sSelBegin.iChar=-1;
 //psTE->sSelBegin.iRow=-1;

}



static void SELstart(EH_TEXTEDITOR *psTE)
{
 static SINT UltimoShift=OFF;

 if (!L_Shift()) {UltimoShift=OFF;return;}

 if ((UltimoShift==ON)&&(psTE->sSelBegin.iChar!=-1)) return;

 //if (psTE->sSelBegin.iChar==-1)
 SELreset(psTE);

 psTE->sSelBegin.iChar=psTE->Cx;
 psTE->sSelBegin.iRow=psTE->ptre;
 psTE->sSelEnd.iChar=-1;
 psTE->sSelEnd.iRow=-1;

 UltimoShift=ON;
}

// Fine della selezione

static void SELend(EH_TEXTEDITOR *psTE)
{
 UINT x,k1,k2;
 SINT OldTEY=psTE->sSelEnd.iRow;
 if (!L_Shift()) return;

 TE_LineRead(psTE,psTE->ptre);

 x=psTE->Cx;
 if (x>strlen(psTE->pszBufferRow)) x=strlen(psTE->pszBufferRow);

 psTE->sSelEnd.iChar=x;
 psTE->sSelEnd.iRow=psTE->ptre;

 psTE->refre=ON;
 k1=psTE->Cy; k2=psTE->Cy;
 if (OldTEY!=psTE->sSelEnd.iRow) {k1--; k2++;}

// if (psTE->sSelBegin.iRow<psTE->ptre) {k1=psTE->Cy-1; if (k1<0) k1=0;}
// if (psTE->sSelBegin.iRow>psTE->ptre) {k2=psTE->Cy+1; if (k2>psTE->ncam) k2=psTE->ncam;}
 if (k1<0) k1=0;
 if (k2>(UINT)psTE->ncam) k2=psTE->ncam;
 TE_AreaRefresh(psTE,k1,k2);
 //dispx("SE (%d,%d) - (%d,%d)        ",psTE->sSelBegin.iRow,psTE->sSelBegin.iChar,psTE->sSelEnd.iRow,psTE->sSelEnd.iChar);
}

// ------------------------------------------
//  Swapping delle Coordinate di Selezione  !
// ------------------------------------------

static void LTE_SetSelection(EH_TEXTEDITOR *psTE,SINT iTipo)//,SINT *X1,LONG *Y1,SINT *X2,LONG *Y2)
{
	RECT rRect;
	switch (iTipo)
	{
		case 0: // Point Start

			if (psTE->bSelection) InvalidateRect(psTE->pObj->hWnd,NULL,FALSE);
			psTE->sSelBegin.iChar=psTE->sSelEnd.iChar=psTE->Cx;
			psTE->sSelBegin.iRow=psTE->sSelEnd.iRow=psTE->Cy+psTE->Y_offset;
			psTE->bSelection=FALSE;
			break;

		case 1:
			psTE->sSelEnd.iChar=psTE->Cx;
			psTE->sSelEnd.iRow=psTE->Cy+psTE->Y_offset;

			if (memcmp(&psTE->sSelBegin,&psTE->sSelEnd,sizeof(TED_SEL))) psTE->bSelection=TRUE; else psTE->bSelection=FALSE;

			//
			// Calcolo il rettangolo di invalidazione
			//
			{
				TED_SEL sSelBegin,sSelEnd;
				memcpy(&sSelBegin,&psTE->sSelBegin,sizeof(TED_SEL));
				memcpy(&sSelEnd,&psTE->sSelEnd,sizeof(TED_SEL));
				LTE_SelOrder(&sSelBegin,&sSelEnd);
				rRect.left=0; rRect.right=psTE->pObj->col2;
				rRect.top=sSelBegin.iRow*psTE->Falt;
				rRect.bottom=((sSelEnd.iRow+1)*psTE->Falt)-1;
				InvalidateRect(psTE->pObj->hWnd,NULL,FALSE);
			}
			break;
	}
/*
#ifdef _DEBUG
	dispx("%d (%d,%d) > (%d,%d)           ",
			psTE->bSelection,
			psTE->sSelBegin.iRow,psTE->sSelBegin.iChar,
			psTE->sSelEnd.iRow,psTE->sSelEnd.iChar);
#endif
*/
}

static SINT LTE_SelOrder(TED_SEL *psBegin,TED_SEL *psEnd)//EH_TEXTEDITOR *psTE);
{
	TED_SEL sSelBegin,sSelEnd;
	memcpy(&sSelBegin,psBegin,sizeof(TED_SEL));
	memcpy(&sSelEnd,psEnd,sizeof(TED_SEL));

	if (sSelEnd.iRow!=-1)
	{
	if ((sSelBegin.iRow==sSelEnd.iRow)&&(sSelBegin.iChar>sSelEnd.iChar))
	 {SINT a;
		a=sSelEnd.iChar;sSelEnd.iChar=sSelBegin.iChar;sSelBegin.iChar=a;
	 }

	if (sSelBegin.iRow>sSelEnd.iRow)
	 {SINT a;
		LONG b;
		a=sSelEnd.iChar;sSelEnd.iChar=sSelBegin.iChar;sSelBegin.iChar=a;
		b=sSelEnd.iRow;sSelEnd.iRow=sSelBegin.iRow;sSelBegin.iRow=b;
	 }
	} else return ON;


	memcpy(psBegin,&sSelBegin,sizeof(TED_SEL));
	memcpy(psEnd,&sSelEnd,sizeof(TED_SEL));

    return OFF;
}
/*
//-----------------------------------------------
//                                              !
// MODIFICA DINAMICA DEL BLOCCO SELEZIONE       !
// DURANTE L'EDITING                            !
//                                              !
// ATTENZIONE: Dire una preghiera prima di      !
//             metterci le mani.                !
//                                              !
//                              G.Tassistro     !
//                                              !
//-----------------------------------------------
static void SELLineMinus(EH_TEXTEDITOR *psTE,LONG dove,SINT Flag)
{
	//if (SELcoo(psTE,&sSelBegin.iChar,&sSelBegin.iRow,&sSelEnd.iChar,&sSelEnd.iRow)) return;
	if (LTE_SelOrder(psTE)) return;
	if (Flag) psTE->sSelBegin.iChar=0;
	//printf("DELline %ld, X1:%d Y1:%ld X2:%d Y2:%ld \n",dove,sSelBegin.iChar,sSelBegin.iRow,sSelEnd.iChar,sSelEnd.iRow);

	if (dove>psTE->sSelEnd.iRow) return;

	// Cancello la selezione
	if ((dove==psTE->sSelEnd.iRow)&&(psTE->sSelEnd.iChar==0)) return;

	while (TRUE)
	{
		if ((psTE->sSelBegin.iRow==psTE->sSelEnd.iRow)&&(dove==psTE->sSelBegin.iRow)) 
		{
			psTE->bSelection=FALSE;
			break;
			//psTE->sSelEnd.iRow=-1; psTE->sSelBegin.iRow=-1; goto java;
		}
		if ((dove==(psTE->sSelEnd.iRow-1))&&(psTE->sSelEnd.iChar==0)) {psTE->sSelEnd.iRow--; psTE->sSelEnd.iChar=psTE->Cx; break;}

		if ((dove==(psTE->sSelEnd.iRow-1))&&(psTE->sSelEnd.iChar>0))
			{if (psTE->sSelEnd.iRow==psTE->sSelBegin.iRow) {psTE->sSelBegin.iRow--;psTE->sSelBegin.iChar+=psTE->Cx;}
			 psTE->sSelEnd.iRow--; 
			 psTE->sSelEnd.iChar+=psTE->Cx;
			 break;
			}

		if ((dove==psTE->sSelEnd.iRow)&&(psTE->sSelEnd.iChar>0)) break;

		if ((psTE->Cx>0)&&(dove==(psTE->sSelBegin.iRow-1)))
		{
			psTE->sSelBegin.iRow--;
			psTE->sSelEnd.iRow--; 
			psTE->sSelBegin.iChar+=psTE->Cx;
			 break;
		}

		if (dove<psTE->sSelBegin.iRow) {psTE->sSelBegin.iRow--; psTE->sSelEnd.iRow--;}
		if (dove>=psTE->sSelBegin.iRow) psTE->sSelEnd.iRow--;
		break;
	}

	if ((psTE->sSelBegin.iRow==psTE->sSelEnd.iRow)&&(psTE->sSelBegin.iChar==psTE->sSelEnd.iChar)) psTE->bSelection=FALSE;
	if (!psTE->bSelection) {ZeroFill(psTE->sSelBegin); ZeroFill(psTE->sSelEnd);}
}

//-----------------------------------------------
// Modificato il file la selezione si aggiorna  !
//-----------------------------------------------
static void SELLinePlus(EH_TEXTEDITOR *psTE,LONG dove)
{
//	SINT sSelBegin.iChar,sSelEnd.iChar;
//	LONG sSelBegin.iRow,sSelEnd.iRow;

	//if (SELcoo(psTE,&sSelBegin.iChar,&sSelBegin.iRow,&sSelEnd.iChar,&sSelEnd.iRow)) return;
	
	if (LTE_SelOrder(psTE)) return;

	//printf("INSline %ld, X1:%d Y1:%ld X2:%d Y2:%ld \n",dove,sSelBegin.iChar,sSelBegin.iRow,sSelEnd.iChar,sSelEnd.iRow);

	if ((dove==psTE->sSelEnd.iRow)&&(psTE->sSelEnd.iChar==0)) return;
	if ((dove==psTE->sSelEnd.iRow)&&(psTE->Cx>psTE->sSelEnd.iChar)) return;

	while (TRUE)
	{
		if ((dove==psTE->sSelBegin.iRow)&&(psTE->Cx<psTE->sSelBegin.iChar))
		 {
			psTE->sSelBegin.iRow++; psTE->sSelEnd.iRow++; psTE->sSelBegin.iChar-=psTE->Cx;
			if (psTE->sSelBegin.iRow==psTE->sSelEnd.iRow) psTE->sSelEnd.iChar-=psTE->Cx;
			break;
		}

		// Sistema la fine orizzontale della selezione
		if ((dove==psTE->sSelEnd.iRow)||((dove==(psTE->sSelEnd.iRow-1))&&(psTE->sSelEnd.iChar==0)))
			{
			 memoRead(psTE->Hdl,dove*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
			 if ((dove==(psTE->sSelEnd.iRow-1))&&(psTE->sSelEnd.iChar==0)&&psTE->Cx<(SINT) strlen(psTE->Riga2)) psTE->sSelEnd.iRow--;
			 if (psTE->sSelEnd.iChar==0) psTE->sSelEnd.iChar=strlen(psTE->Riga2);
			 psTE->sSelEnd.iChar-=psTE->Cx;
			}

		if (dove>psTE->sSelEnd.iRow) return;
		if (dove<psTE->sSelBegin.iRow) {psTE->sSelBegin.iRow++; psTE->sSelEnd.iRow++;}
		if (dove>=psTE->sSelBegin.iRow) {psTE->sSelEnd.iRow++;}
		break;
	}
//	printf("[%ld]",sSelEnd.iRow);

//	psTE->sSelBegin.iRow=sSelBegin.iRow; psTE->sSelEnd.iRow=sSelEnd.iRow;
//	psTE->sSelBegin.iChar=sSelBegin.iChar; psTE->sSelEnd.iChar=sSelEnd.iChar;
}

//-----------------------------------------------
// Modificato il file la selezione si aggiorna  !
//-----------------------------------------------
static void SELCarPlus(EH_TEXTEDITOR *psTE)
{
//	SINT sSelBegin.iChar,sSelEnd.iChar;
//	LONG sSelBegin.iRow,sSelEnd.iRow;

	if (!IPT_ins) return;
	if (LTE_SelOrder(psTE)) return;
	//if (SELcoo(psTE,&sSelBegin.iChar,&sSelBegin.iRow,&sSelEnd.iChar,&sSelEnd.iRow)) return;

	if ((psTE->ptre==psTE->sSelEnd.iRow)&&(psTE->Cx<psTE->sSelEnd.iChar)) psTE->sSelEnd.iChar++;
	if ((psTE->ptre==psTE->sSelBegin.iRow)&&(psTE->Cx<psTE->sSelBegin.iChar)) psTE->sSelBegin.iChar++;

}

//-----------------------------------------------
// Modificato il file la selezione si aggiorna  !
//-----------------------------------------------
static void SELCarMinus(EH_TEXTEDITOR *psTE)
{
//	SINT sSelBegin.iChar,sSelEnd.iChar;
//	LONG sSelBegin.iRow,sSelEnd.iRow;

//	if (SELcoo(psTE,&sSelBegin.iChar,&sSelBegin.iRow,&sSelEnd.iChar,&sSelEnd.iRow)) return;
	if (LTE_SelOrder(psTE)) return;

	if ((psTE->ptre==psTE->sSelEnd.iRow)&&(psTE->Cx<psTE->sSelEnd.iChar)) psTE->sSelEnd.iChar--;
	if ((psTE->ptre==psTE->sSelBegin.iRow)&&(psTE->Cx<psTE->sSelBegin.iChar)) psTE->sSelBegin.iChar--;
	if (psTE->sSelEnd.iChar<0) psTE->sSelEnd.iChar=0;

	if ((psTE->sSelBegin.iRow==psTE->sSelEnd.iRow)&&(psTE->sSelBegin.iChar==psTE->sSelEnd.iChar)) {psTE->sSelEnd.iRow=-1; psTE->sSelBegin.iRow=-1;}

//	psTE->sSelBegin.iRow=sSelBegin.iRow; psTE->sSelEnd.iRow=sSelEnd.iRow;
//	psTE->sSelBegin.iChar=sSelBegin.iChar; psTE->sSelEnd.iChar=sSelEnd.iChar;
}
*/
//------------------------------------
// Copia la selezione in memoria     !
//------------------------------------

static void SELcopia(EH_TEXTEDITOR *psTE)
{
	LONG iRow;
	TCHAR *pszDest;
	CHAR *pszBase;
	HANDLE hgm;

	if (LTE_SelOrder(&psTE->sSelBegin,&psTE->sSelEnd)) {efx2(); return;}

	iRow=psTE->sSelEnd.iRow-psTE->sSelBegin.iRow+1;
	hgm=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,iRow*psTE->SizeLine);
	pszBase=pszDest=GlobalLock(hgm); 
	memset(pszBase,0,iRow*psTE->SizeLine);
	//sprintf(CRLF,"%c%c",13,10);

	if (OpenClipboard(psTE->pObj->hWnd))
	{
		EmptyClipboard();
		for (iRow=psTE->sSelBegin.iRow;iRow<=psTE->sSelEnd.iRow;iRow++)
		{
			BYTE *pStart,*pEnd;
			SINT iSize;
			TE_LineRead(psTE,iRow);

			// Prima Linea
			if (iRow==psTE->sSelBegin.iRow) pStart=psTE->pszBufferRow+psTE->sSelBegin.iChar; else pStart=psTE->pszBufferRow;
			if (iRow==psTE->sSelEnd.iRow) pEnd=psTE->pszBufferRow+psTE->sSelEnd.iChar; else pEnd=psTE->pszBufferRow+strlen(psTE->pszBufferRow);

			// Ultima Linea
			//	 if (pt==(Line-1)) *(p+psTE->sSelEnd.iChar)=0;
			if (iRow>psTE->sSelBegin.iRow) {strcat(pszBase,CRLF); pszDest+=2;}
			//strcat(Dest,p);
			iSize=(SINT) pEnd-(SINT) pStart;
			memcpy(pszDest,pStart,iSize); pszDest+=iSize; *pszDest=0;
		}  
		GlobalUnlock(hgm);
		SetClipboardData(CF_TEXT,hgm);
		CloseClipboard();
	}
}

//------------------------------------
// Incolla la selezione              !
//------------------------------------

static void SELincolla(EH_TEXTEDITOR *psTE,CHAR *pString)
{
	LONG ofs=0;
//	SINT Hlocal;
//	CHAR *Buf;
//	CHAR *BufTC;
	SINT start=0;
    HANDLE hmem=NULL;
	SINT iRow;
	CHAR *pszClip=NULL;
//    CHAR *p;//,*ptr;
    CHAR *szTesta=NULL;
	CHAR *szCoda=NULL;
//	CHAR *lpn;
	EH_AR arRow;
	TED_SEL sCursor;

	if (!pString)
	{
	 if (OpenClipboard(psTE->pObj->hWnd))
	 {
		 hmem=GetClipboardData(CF_TEXT);
		 if (hmem!=NULL)
		 {
			pString=pszClip=GlobalLock(hmem); 
		 }
	 }
	}

	arRow=ARCreate(pString,CRLF,NULL);
	sCursor.iChar=psTE->Cx;
	sCursor.iRow=psTE->Cy+psTE->Y_offset;
	for (iRow=0;arRow[iRow];iRow++)
	{
		if (iRow>0) // Spezzo la riga e vado a capo
		{
			TE_LineInsert(psTE,sCursor.iRow+1,1);
			TE_LineWrite(psTE,sCursor.iRow+1,psTE->pszBufferRow+sCursor.iChar);
			psTE->pszBufferRow[sCursor.iChar]=0;
			TE_LineWrite(psTE,sCursor.iRow,psTE->pszBufferRow);
			sCursor.iRow++; sCursor.iChar=0;
		}

		TE_LineRead(psTE,sCursor.iRow);
		//
		// ATTENZIONE: Non controllo la lunghezza (della riga) ma dovrei farlo per evitare un crash
		//
		strIns(psTE->pszBufferRow+sCursor.iChar,arRow[iRow]); sCursor.iChar+=strlen(arRow[iRow]);
		TE_LineWrite(psTE,sCursor.iRow,psTE->pszBufferRow);
	}
	ARDestroy(arRow);
	psTE->Cx=sCursor.iChar;
	psTE->Cy=sCursor.iRow-psTE->Y_offset;
		
	if (hmem) CloseClipboard();

	psTE->refre=ON;
	TE_AreaRefresh(psTE,psTE->ptre,-1);
}
/*
	  Hlocal=memoAlloc(RAM_AUTO,strlen(Clip)*(psTE->SizeLine*2),"Local");
	  if (Hlocal<0) ehExit("<--->");
	  Buf=memoLock(Hlocal);
	  BufTC=Buf+psTE->SizeLine;

	// Determino la Testa/Coda della linea
	  TE_LineRead(psTE,psTE->ptre);
	  strcpy(BufTC,psTE->pszBufferRow);
	  szTesta=BufTC;
	  szCoda=BufTC+psTE->Cx;
	  strIns(szCoda," "); *szCoda=0; szCoda++;

	  // Loop di inserimento

	  for (ptr=Clip;;)
	  {
	   
	   p=strstr(ptr,CRLF); if (p) {*p=0; lpn=p+2;}
	   if (!p) {p=strstr(ptr,"\r"); if (p) *p=0; lpn=p+1;}
	   if (!p) {p=strstr(ptr,"\n"); if (p) *p=0; lpn=p+1;}
	   strcpy(Buf,ptr); 

	   if (szTesta!=NULL) // Prima Linea
	   {strcpy(Buf,szTesta); strcat(Buf,ptr);
		szTesta=NULL;
	   } 

	   if (!p) //
	   {
		 strcat(Buf,szCoda);
		 szCoda=NULL;
	   }
	   
	   //win_infoarg("[%s] = [%s]",ptr,Buf);
	   TE_AreaRefresh(psTE,start,-1); 
	   TE_LineWrite(psTE,psTE->ptre,Buf);
	   psTE->ptre++;
	   if (p) TE_LineInsert(psTE,psTE->ptre,1);
	   if (!p) break; else ptr=lpn;
	  }

	  GlobalUnlock(hmem);
	  memoFree(Hlocal,"<-->");
	 }
	 */

/*
static void SELreset(EH_TEXTEDITOR *TE)
{
	psTE->sSelBegin.iChar=-1;
	psTE->sSelBegin.iRow=-1;
	psTE->sSelEnd.iChar=-1;
	psTE->sSelEnd.iRow=-1;
	psTE->refre=ON; TE_AreaRefresh(TE, 0,-1,FALSE);
}
*/

//------------------------------------
// Cancella la Selezione             !
//------------------------------------

static void SELdelete(EH_TEXTEDITOR *psTE)
{
	LONG ofs=0;
	LONG Line;

	if (LTE_SelOrder(&psTE->sSelBegin,&psTE->sSelEnd)) {efx2(); return;}
	Line=psTE->sSelEnd.iRow-psTE->sSelBegin.iRow+1;

	// -------------------------
	// Inizio Troncato         !
	// -------------------------

	if ((psTE->sSelBegin.iChar>0)||(psTE->sSelBegin.iRow==psTE->sSelEnd.iRow))
		 {
			TE_LineRead(psTE,psTE->sSelBegin.iRow);
			strcpy(psTE->Riga2,psTE->pszBufferRow);
			*(psTE->pszBufferRow+psTE->sSelBegin.iChar)=0;
			if (psTE->sSelBegin.iRow==psTE->sSelEnd.iRow) strcat(psTE->pszBufferRow,psTE->Riga2+psTE->sSelEnd.iChar);
			TE_LineWrite(psTE,psTE->sSelBegin.iRow,psTE->pszBufferRow);
			ofs++;
		 }

	// ----------------------------
	//  Corpo Centrale            !
	// ----------------------------

	//if (!sSelEnd.iChar&&(Line>0))
	Line--;

//	printf("Chiudo : %ld (ofs:%ld)\n",Line-ofs,ofs);
//	getch();

	if ((Line-ofs)>0)
	 {
		TE_LineDelete(psTE,psTE->sSelBegin.iRow+ofs,(Line-ofs));
	 }

 // ----------------------------
 //  CODA SE C'E'              !
 // ----------------------------

	 //if (sSelEnd.iChar&&(Line>0))
	 //	{

	 if ((Line-ofs)>-1)
	 {
		if (psTE->sSelBegin.iChar>0)
		 {
			TE_LineRead(psTE,psTE->sSelBegin.iRow);
			strcpy(psTE->Riga2,psTE->pszBufferRow);
			TE_LineRead(psTE,psTE->sSelEnd.iRow-(Line-ofs));
			strcat(psTE->Riga2,psTE->pszBufferRow+psTE->sSelEnd.iChar);
			TE_LineWrite(psTE,psTE->sSelBegin.iRow,psTE->Riga2);
			TE_LineDelete(psTE,psTE->sSelEnd.iRow-(Line-ofs),1);
		 }
		 else
		 {
			TE_LineRead(psTE,psTE->sSelEnd.iRow-(Line-ofs));
			strcat(psTE->Riga2,psTE->pszBufferRow+psTE->sSelEnd.iChar);
			TE_LineWrite(psTE,psTE->sSelEnd.iRow-(Line-ofs),psTE->Riga2);
		 }
	 }

	 //	}

	 //psTE->sSelBegin.iChar=-1; psTE->sSelEnd.iChar=-1;
	 //psTE->sSelBegin.iRow=-1; psTE->sSelEnd.iRow=-1;
	 LTE_ClearSelection(psTE);

	 psTE->refre=ON; TE_AreaRefresh(psTE,0,-1);
	 DirectTextEditor(psTE,WTE_CURSOR,psTE->sSelBegin.iRow,&psTE->sSelBegin.iChar);
}

static void LTE_ClearSelection(EH_TEXTEDITOR *psTE)
{
	psTE->bSelection=FALSE;
	//ZeroFill(psTE->sSelBegin); 
	ZeroFill(psTE->sSelEnd);
}




// +-------------------------------------------+
//  DirectTextEditor()
//	Gestione di editor testi       
//             Edita un testo tipo word      
//                                           
//  comandi                                  
//                                           
//  WS_OPEN : predefinire EH_TEXTEDITOR struct  
//                                           
//                                            
//              by Ferrà Art & Technology 1997 
// +-------------------------------------------+

SINT DirectTextEditor(EH_TEXTEDITOR *psTE,SINT iCommand,LONG dato,void *ptr)
{
 //static EH_TEXTEDITOR *TE=NULL;
 
 // Cercare i Return

 SINT iRet=0;
 if (!psTE) ehExit("lpTE = NULL");
 
 switch (iCommand)
 {
 // --------------------------------------------------------------------
 // OPEN                                                               !
 //                                                                    !
 // -------------------------------------------------------------------!
 case WS_OPEN:
	
	 {//SINT a;
     WNDCLASSEX wc;

     //TE=(EH_TEXTEDITOR *) ptr;
	 //LastTE=TE;
	 psTE->Px2=psTE->Px+psTE->Lx-1;
	 psTE->Py2=psTE->Py+psTE->Ly-1;
//	 psTE->Falt=font_altf(psTE->Font,psTE->Nfi);
	 //fontFind(psTE->Font,&a,&psTE->FontHdl);

	 psTE->Cx=0;psTE->Cy=0;

	 // Registro la classe delle OW_SCR
	 wc.cbSize        = sizeof(wc);
	 wc.style         = CS_NOCLOSE;
	 wc.lpfnWndProc   = EhTextEditorProcedure;
	 wc.cbClsExtra    = 0;
	 wc.cbWndExtra    = 0;
	 wc.hInstance     = sys.EhWinInstance;
	 wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	 wc.hCursor       = NULL;//LoadCursor(NULL,IDC_ARROW);
	 wc.hbrBackground = NULL;//(HBRUSH) GetStockObject(WHITE_BRUSH);
	 wc.lpszMenuName  = NULL;///LoadMenu;//szAppName;
	 wc.lpszClassName = "EH_TEXTEDITOR";
	 wc.hIconSm       = NULL;//LoadIcon(NULL,IDI_APPLICATION);
	 RegisterClassEx(&wc);
	 //win_infoarg("%d,%d - %d,%d",psTE->Px+relwx,psTE->Py+relwy,psTE->Lx,psTE->Ly);
	 psTE->pObj->hWnd=CreateWindow("EH_TEXTEDITOR", 
								   "",
								   WS_BORDER|WS_CHILD|WS_VSCROLL,
								   psTE->Px+relwx,psTE->Py+relwy,
								   psTE->Lx,psTE->Ly,
								   WIN_info[sys.WinWriteFocus].hWnd,
								   (HMENU) 1000,
								   sys.EhWinInstance,
								   NULL);

	 // Setto il font
	 DirectTextEditor(psTE,WTE_NEWFONT,0,NULL);

	 //boxp(psTE->Px,psTE->Py,psTE->Px2,psTE->Py2,psTE->Col2,SET);
	 psTE->Falt=Wfont_alt(&psTE->LogFont);

	 // Assegno il font al device context
	 //WM_SETFONT
	 //hDC=GetDC(psTE->pObj->hWnd);
	 //SelectObject(hDC, psTE->hFont);
	 //ReleaseDC(psTE->pObj->hWnd,hDC);

//	 Wcursor(psTE->pObj->hWnd);
//	 txtCursorAspect(3,psTE->Falt,psTE->ColCur,2);
	 
	 // Riserva la memoria
	 if (psTE->MaxLines<10) ehExit("TEerr1");
	 psTE->Hdl=memoAlloc(RAM_AUTO,(LONG) psTE->MaxLines*psTE->SizeLine,"EH_TEXTEDITORor");

	 // Buffer di linea
	 psTE->RigaHdl=memoAlloc(M_HEAP,psTE->SizeLine*2,"TE_Buflinea");
	 if (psTE->RigaHdl<0) ehExit("TErr2");
	 psTE->pszBufferRow=memoPtr(psTE->RigaHdl,NULL);
	 psTE->Riga2=psTE->pszBufferRow+psTE->SizeLine;

	 if (psTE->Hdl<0) ehExit("TE:WS_OPEN");
	 psTE->ncam=((psTE->Ly-1)/psTE->Falt)+1;
	 psTE->ncamE=(psTE->Ly)/psTE->Falt;

	 psTE->Y_offset=0;
	 psTE->Y_koffset=-1;
	 psTE->X_offset=0;
	 psTE->X_koffset=-1;
	 psTE->refre=ON;

	 psTE->sSelBegin.iChar=-1;
	 psTE->sSelBegin.iRow=-1;
	 psTE->sSelEnd.iChar=-1;
	 psTE->sSelEnd.iRow=-1;
     
	 UpdateWindow(psTE->pObj->hWnd);
     ShowWindow(psTE->pObj->hWnd,SW_SHOW);
	}
	break;

 //if (!strcmp(psTE->Test,"E' APERTO!")) ehExit("TexEH_TEXTEDITORor da aprire");

 // --------------------------------------------------------------------
 // CLOSE                                                              !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WS_CLOSE:
			DirectTextEditor(psTE,WTE_CURSOROFF,0,NULL);
			if (psTE->Hdl!=-1) memoFree(psTE->Hdl,"TE_CLOSE");
			if (psTE->RigaHdl!=-1) memoFree(psTE->RigaHdl,"RG");
		    if (psTE->pObj->hWnd) DestroyWindow(psTE->pObj->hWnd);
			if (psTE->hFont) DeleteObject(psTE->hFont);
			psTE->hFont=NULL;
			//txtCursor(FALSE);
			//Wcursor(psTE->pObj->hWnd);
			break;
    
 // --------------------------------------------------------------------
 // Assegna un nuovo font                                              !
 //                                                                    !
 // -------------------------------------------------------------------!
	
	case WTE_NEWFONT:
		if (psTE->hFont) DeleteObject(psTE->hFont);
		psTE->hFont=CreateFontIndirect(&psTE->LogFont);
		psTE->Falt=Wfont_alt(&psTE->LogFont);
		psTE->ncam=((psTE->Ly-1)/psTE->Falt)+1;
		psTE->ncamE=(psTE->Ly)/psTE->Falt;

		if (psTE->pObj->hWnd)
		{
		 HDC hDC=GetDC(psTE->pObj->hWnd);
		 SelectObject(hDC, psTE->hFont);
		 ReleaseDC(psTE->pObj->hWnd,hDC);
		}

		break;

 // --------------------------------------------------------------------
 // NEW                                                                !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_NEW:		 
		{
			LONG cnt;
			CHAR zz[20];
			memset(zz,0,20);

			psTE->Touch=OFF;
			psTE->Lines=0;
			psTE->Cx=0;psTE->Cy=0;
			// Abblancko
			mouse_graph(0,0,"CLEX");
			for (cnt=0;cnt<psTE->MaxLines;cnt++)
					{
					//TE_LineWrite(cnt," ",TE);
					 memoWrite(psTE->Hdl,cnt*psTE->SizeLine,zz,sizeof(zz));
					}
			mouse_graph(0,0,"MS01");
			DirectTextEditor(psTE,WTE_RESET,0,NULL);
			//goto RESETTA;
			//psTE->refre=ON;// da sostituire con zona
			//TE_AreaRefresh(TE,-1,-1);
		 }
		 break;
 // --------------------------------------------------------------------
 // RESET
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_RESET:		 

			psTE->Touch=OFF;
			psTE->Y_offset=0;
			psTE->X_offset=0;
			psTE->refre=ON;
			psTE->Cx=0;psTE->Cy=0;

			psTE->sSelBegin.iChar=-1;
			psTE->sSelBegin.iRow=-1;
			psTE->sSelEnd.iChar=-1;
			psTE->sSelEnd.iRow=-1;

			TE_AreaRefresh(psTE,-1,-1);
			break;

 // --------------------------------------------------------------------
 // WS_LOAD                                                               !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WS_LOAD:
		 {
			FILE *pf1;
			LONG a;
//			SINT Flag;

			// ------------------------------
			// Carico il file in memoria    !
			// ------------------------------

			if (!fileCheck(ptr)) return -1;
			pf1=fopen(ptr,"r"); if (!pf1) return -1;
			mouse_graph(0,0,"CLEX");
			fseek(pf1,0,SEEK_SET);
			for (a=0;;a++)
			{ CHAR *p;
				if (!fgets(psTE->Riga2,psTE->SizeLine,pf1)) break;
				p=psTE->Riga2;
				for (;*p!=0;p++) {if ((*p==13)||(*p==10)) *p=0;}
				memoWrite(psTE->Hdl,a*psTE->SizeLine,psTE->Riga2,psTE->SizeLine);
			}
			//EOF

			fclose(pf1);
			mouse_graph(0,0,"MS01");

			strcpy(psTE->FileName,ptr);
			psTE->Lines=a;

			DirectTextEditor(psTE,WTE_RESET,0,NULL);
		 }
		 break;

 // --------------------------------------------------------------------
 //                                                                    !
 // SAVE                                                               !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_SAVE:
		 {
			FILE *pf1;
			LONG a;

			// ------------------------------
			// Salvo il file in memoria    !
			// ------------------------------

			mouse_graph(0,0,"CLEX");
			//printf(ptr); getch();
			pf1=fopen(ptr,"w"); if (!pf1) win_errgrave("File ??");

			for (a=0;a<psTE->Lines;a++) { 
				TE_LineRead(psTE,a);
				fprintf(pf1,"%s\n",psTE->pszBufferRow);
			}
			fclose(pf1);
			mouse_graph(0,0,"MS01");

			strcpy(psTE->FileName,ptr);
			psTE->Touch=OFF;
		 }
		 break;

 // --------------------------------------------------------------------
 // CURSOR                                                             !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_CURSOR:
	 {SINT *px;

		px=(SINT *) ptr;
		rifo:
		psTE->Cy=(SINT) (dato-psTE->Y_offset);
		if ((psTE->Cy<0)||(psTE->Cy>=psTE->ncamE))
			 {psTE->Y_offset=dato-(psTE->ncamE/2);
				if (psTE->Y_offset<0) psTE->Y_offset=0;
				psTE->refre=ON;
				goto rifo;
			 }

		//if (psTE->Lines<=psTE->ncamE) psTE->Y_offset=0;
		//if (psTE->refre) DirectTextEditor("REDRAW",0,0);
		TE_AreaRefresh(psTE,0,-1);
		psTE->Cx=*px;
		psTE->ptre=psTE->Y_offset+psTE->Cy;
	 }
	 break;

	case WTE_CURSORON:
		//Wcursor(psTE->pObj->hWnd);
		txtCursorPosEx(psTE->pObj->hWnd,0,0);
		{SINT Color;
		 Color=ModeColor(TRUE);
	     txtCursorAspect(3,psTE->Falt,psTE->ColCur,2);
		 ModeColor(Color);
		}
		if (dato) {if (!sys.sTxtCursor.bVisible) txtCursor(TRUE);}
		break;

	case WTE_CURSOROFF:
		if (sys.sTxtCursor.bVisible) txtCursor(FALSE);
		//Wcursor(WindowNow());
	    //txtCursorAspect(3,psTE->Falt,psTE->ColCur,2);
		break;


 // --------------------------------------------------------------------
 // DISP refresh di una linea                                          !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_DISP:
			{
				SINT rigan;

				rigan=(SINT) (dato-psTE->Y_offset);
				if ((rigan<0)||(rigan>psTE->ncam)) return 0;
				psTE->refre=ON;
				TE_AreaRefresh(psTE,dato,dato);
			}
			break;

 // --------------------------------------------------------------------
 //                                                                    !
 // REDRAW ridisegna tutta la finestra                                 !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_REDRAW:
			{
				//SINT a;
				SINT Color;
				psTE->Px2=psTE->Px+psTE->Lx-1;
				psTE->Py2=psTE->Py+psTE->Ly-1;
				psTE->refre=ON;// da sostituire con zona
				//psTE->Falt=psTE->LogFont.lfHeight;
				psTE->Falt=Wfont_alt(&psTE->LogFont);
				//psTE->Falt=font_altf(psTE->Font,psTE->Nfi);
				//fontFind(psTE->Font,&a,&psTE->FontHdl);
				//Wcursor(psTE->pObj->hWnd);
				Color=ModeColor(TRUE);
				if (sys.sTxtCursor.bVisible) txtCursorAspect(3,psTE->Falt,psTE->ColCur,2); 
				ModeColor(Color);
				psTE->ncam=((psTE->Ly-1)/psTE->Falt)+1;
				psTE->ncamE=(psTE->Ly)/psTE->Falt;
				if (dato) InvalidateRect(psTE->pObj->hWnd,NULL,FALSE);
				MoveWindow(psTE->pObj->hWnd,
					       psTE->Px+relwx,psTE->Py+relwy,
						   psTE->Lx-1,psTE->Ly-1,
						   TRUE);// TRUE
				

				
				//TE_AreaRefresh(TE,(SINT) dato,-1,FALSE);
                /*
				InvalidateRect(psTE->pObj->hWnd,NULL,FALSE);
                UpdateWindow(psTE->pObj->hWnd);
                ShowWindow(psTE->pObj->hWnd,SW_SHOW);
				*/
			}
			break;

 // --------------------------------------------------------------------
 //                                                                    !
 // XCONTROL Controlla che il cursore sia nella finestra               !
 //          se no la sposta                                           !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_XCONTROL:
			{
				CHAR *pszBufferRow;
				SINT x;
			    SINT Rx;

				DACAPO2:
				pszBufferRow=psTE->pszBufferRow;
				psTE->ptre=psTE->Y_offset+psTE->Cy;
				memoRead(psTE->Hdl,psTE->ptre*psTE->SizeLine,pszBufferRow,psTE->SizeLine);

				if (psTE->Cx>(SINT) strlen(pszBufferRow)) psTE->Cx=strlen(pszBufferRow);
				//x=font_dim(pszBufferRow,psTE->Cx,psTE->FontHdl,psTE->Nfi);
//                x=WFontDimWnd(pszBufferRow,psTE->Cx,psTE->pObj->hWnd,&psTE->LogFont);
                //x=Wfont_dim(pszBufferRow,psTE->Cx,&psTE->LogFont);
				if (psTE->fHCT) x=HCTMakeDim(psTE,pszBufferRow,psTE->Cx,psTE->ptre);
							  else
							  x=Wfont_dimh(pszBufferRow,psTE->Cx,psTE->hFont);

				Rx=psTE->Px+x-(SINT) psTE->X_offset;

				// Controllo spostamento orizzontale
				if (Rx>(psTE->Px2-(psTE->Lx/5)))
							{
							 do {
							 psTE->X_offset+=(psTE->Lx/3);
							 Rx=psTE->Px+x-(SINT) psTE->X_offset;
							 } while (Rx>psTE->Px2);

							 psTE->refre=ON;
							 TE_AreaRefresh(psTE, psTE->Y_offset,-1);
							 goto DACAPO2;
							}

				if (Rx<psTE->Px)
							{
							 do {
							 psTE->X_offset-=(psTE->Lx/3);
							 if (psTE->X_offset<0) psTE->X_offset=0;
							 Rx=psTE->Px+x-(SINT) psTE->X_offset;
							 } while (Rx<psTE->Px);

							 psTE->refre=ON;
							 TE_AreaRefresh(psTE, psTE->Y_offset,-1);
							 goto DACAPO2;
							}
				//Rx=psTE->Px+x-psTE->X_offset;

				if (!sys.sTxtCursor.bVisible) txtCursor(TRUE); 
				txtCursorPosEx(psTE->pObj->hWnd,Rx-psTE->Px+MARGINDX,psTE->Cy*psTE->Falt);
				//Acursor_xy(Rx-psTE->Px+MARGINDX,psTE->Cy*psTE->Falt);
			 }
			break;

 // --------------------------------------------------------------------
 //                                                                    !
 // COPIA Selezione nel clipboard                                      !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_COPY:
			SELcopia(psTE);
			break;

 // --------------------------------------------------------------------
 // TAGLIA Selezione nel clipboard                                     !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_CUT:
			SELcopia(psTE);
			SELdelete(psTE);
			break;

 // --------------------------------------------------------------------
 //                                                                    !
 // INCOLLA Selezione nel testo                                        !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WTE_PASTE:
			 SELincolla(psTE,ptr);
			 break;

 // --------------------------------------------------------------------
 // LINEREAD Legge una linea                                           !
 // -------------------------------------------------------------------!
	case WTE_LINEREAD:
			memoRead(psTE->Hdl,dato*psTE->SizeLine,ptr,psTE->SizeLine);
			break;

 // --------------------------------------------------------------------
 // LINEWRITE Scrive una linea                                         !
 // -------------------------------------------------------------------!
	case WTE_LINEWRITE:
			TE_LineWrite(psTE,dato,ptr);
			break;

 // --------------------------------------------------------------------
 // LINEINSERT Inserisce delle linee                                   !
 // -------------------------------------------------------------------!
	case WTE_LINEINSERT:
			TE_LineInsert(psTE,dato,atol(ptr));
			break;

 // --------------------------------------------------------------------
 // LINEAPPEND Accoda una linea al file                                !
 // -------------------------------------------------------------------!
	case WTE_LINEAPPEND:
			TE_LineWrite(psTE,psTE->Lines,ptr);
			psTE->Lines++;
			break;

 // --------------------------------------------------------------------
 //                                                                    !
 // FIND Ricerca una parola                                            !
 //                                                                    !
 // -------------------------------------------------------------------!
	case WS_FIND:
	 {
		EH_TEXTEDITORFIND *TEF;
		LONG Da,Finoa;
		LONG Pt;
		CHAR *p;
		//SINT Flag=ON;
DACAPOF:
		iRet=TRUE;

		TEF=(EH_TEXTEDITORFIND *) ptr;
	
	//	win_infoarg("Find [%s]",TEF->szFind);
//		rifai:
/*
		if (!TEF->Dove) {Da=0; Finoa=psTE->Lines;}
						 else
						 {Da=psTE->sSelBegin.iRow;
						  Finoa=psTE->sSelEnd.iRow;
						 }
						 */
		Da=0; Finoa=psTE->Lines;
		if (TEF->fStart) {TEF->iX=0; TEF->iY=0; Da=0;}  // Parto dall'inizio
						 else 
						 Da=psTE->ptre; // Parto dalla posizione del cursore
		TEF->fStart=FALSE;

		if (TEF->iY!=-1) {Da=TEF->iY;TEF->iX++;} // Avanzo di un carattere nell'ultima posizione
						 else 
						 {TEF->iX=0; }

		//if (!TEF->fCase) strupr(TEF->Cerco);
		//dispx("%d,%d,%d       ",Da,TEF->iX,Finoa);
		if (TEF->iX<0) TEF->iX=0;
		for (Pt=Da;Pt<Finoa;Pt++,TEF->iX=0)
		{
		 TE_LineRead(psTE,Pt);

		 //if (!TEF->Case) strupr(psTE->pszBufferRow);
		 if (TEF->iX>(SINT) strlen(psTE->pszBufferRow)) {TEF->iX=0; continue;}

		 if (TEF->fCase) p=strstr(psTE->pszBufferRow+TEF->iX,TEF->szFind);
						 else
						 p=strCaseStr(psTE->pszBufferRow+TEF->iX,TEF->szFind);

		 //printf("[%s]\n",psTE->pszBufferRow+TEF->X);
		 if (p)
			{SINT iX; 
		     SINT iCursorEnd;

			 iX=(SINT) ((LONG) p-(LONG) psTE->pszBufferRow);
			 iCursorEnd=iX+strlen(TEF->szFind);

			 if (TEF->fSelect)
			 {
			  psTE->sSelBegin.iRow=Pt;
			  psTE->sSelEnd.iRow=Pt;
			  psTE->sSelBegin.iChar=iX;
			  psTE->sSelEnd.iChar=iCursorEnd;
			 }

			 TEF->iX=iX;
			 DirectTextEditor(psTE,WTE_CURSOR,Pt,&iCursorEnd);
			 TEF->iY=Pt;
			 //win_infoarg("[%s] >> %d,%d",p,TEF->iY,TEF->iX);
			 //Flag=OFF;
			 iRet=FALSE;
			 break;
			}
		 
		}

//		if (TEF->iY==-1) {WIN_fz=OFF;
		if (iRet)
		{
			if (!TEF->iY&&!TEF->iX) 
			{WIN_fz=OFF;
			 win_info("La stringa ricercata non esiste.");
			 WIN_fz=ON;
			 return -1;
			}
			else
			{
				TEF->fStart=TRUE;
				goto DACAPOF; 
			}

		}

	 }
	 break;
 }
 return iRet;
}

static void L_TextEditorControl(EH_TEXTEDITOR *psTE)
{
	SINT x,iRx;
	psTE->ptre=psTE->Y_offset+psTE->Cy;

	// 
	// Controllo linea esterno
	// 
	if (psTE->FuncExtern) (*psTE->FuncExtern)(WS_LINK,0,NULL,psTE,FALSE,0);
	if (psTE->ptre<psTE->Lines) memoRead(psTE->Hdl,psTE->ptre*psTE->SizeLine,psTE->pszBufferRow,psTE->SizeLine); else *psTE->pszBufferRow=0;
	psTE->iRowSize=strlen(psTE->pszBufferRow);

//	if (psTE->Cx>psTE->iRowSize) psTE->Cx=psTE->iRowSize;

	// Va a capo
	if (psTE->Cx>psTE->iRowSize)
		{
		 psTE->ptre++; 
		 if (psTE->ptre>=psTE->Lines) 
			 Tonk();
			else
			{
				psTE->Cx=0; psTE->Cy++;
			}
		}

	if (psTE->fHCT) 
		x=HCTMakeDim(psTE,psTE->pszBufferRow,psTE->Cx,psTE->ptre);
		else
		x=Wfont_dimh(psTE->pszBufferRow,psTE->Cx,psTE->hFont);
	iRx=psTE->Px+x-(SINT) psTE->X_offset;

	// Controllo spostamento orizzontale
	if (iRx>psTE->Px2)
		{
		 do {
			 psTE->X_offset+=(psTE->Lx/3);
			 iRx=psTE->Px+x-(SINT) psTE->X_offset;
		 } while (iRx>psTE->Px2);

		 psTE->refre=ON;
		 TE_AreaRefresh(psTE, psTE->Y_offset,-1);
		}

	if (iRx<psTE->Px)
		{
		 do {
			 psTE->X_offset-=(psTE->Lx/5);
			 if (psTE->X_offset<0) psTE->X_offset=0;
			 iRx=psTE->Px+x-(SINT) psTE->X_offset;
		 } while (iRx<psTE->Px);

		 psTE->refre=ON;
		 TE_AreaRefresh(psTE, psTE->Y_offset,-1);
		 //goto DARICAPO;
		 //return 0;
		}

	//Acursor_xy(iRx-psTE->Px+MARGINDX,psTE->Cy*psTE->Falt); if (!sys.sTxtCursor.bVisible) txtCursor(TRUE);
	txtCursorPosEx(	psTE->pObj->hWnd,
					iRx-psTE->Px+MARGINDX,
					psTE->Cy*psTE->Falt); 
	if (!sys.sTxtCursor.bVisible) txtCursor(TRUE);
	
}

//
// L_TextEditorEvent()
// CORE dell'editor
//
static SINT L_TextEditorEvent(EH_TEXTEDITOR *psTE,EH_EVENT *psEvent)
{
	BOOL bRedrawAll=FALSE;
	BOOL bBreak=FALSE;
	SINT iRetData=0;
	BYTE cFirst,cSecond;
//	SINT a,iRowsScroll;

//	SelVedi(psTE);

	L_TextEditorControl(psTE);

	if (psTE->FuncExtern) 
	{
		if ((*psTE->FuncExtern)(WS_EVENT,0,psEvent,psTE,FALSE,0)) 
		{
			bBreak=TRUE; //goto USCITA;
		}
	}

	if (!bBreak)
	{
		switch (psEvent->iEvent)
		{
			case EE_NONE:
			//	efx1();
				break;

				// Mouse rilasciato
			case EE_LBUTTONUP:
				if (!sys.sTxtCursor.bVisible) txtCursor(TRUE);
				break;

			case EE_MOUSEWHEEL:
				ehWheelScroll(psTE->pObj->hWnd,psEvent,psTE->ncam);
				break;
/*
			case EE_MOUSEWHEELUP:
				iRowsScroll = (SINT) (GET_WHEEL_DELTA_WPARAM(psEvent->dwParam)/WHEEL_DELTA)*psTE->ncam/5;
				if (iRowsScroll<1) iRowsScroll=1;
				for (a=0;a<iRowsScroll;a++) SendMessage(psTE->pObj->hWnd,WM_VSCROLL,SB_LINEUP,0); //bContinue=TRUE;
				break;

			case EE_MOUSEWHEELDOWN: 
				iRowsScroll = (SINT) (GET_WHEEL_DELTA_WPARAM(psEvent->dwParam)/WHEEL_DELTA)*psTE->ncam/5; iRowsScroll=-iRowsScroll;
				if (iRowsScroll<1) iRowsScroll=1;
				for (a=0;a<iRowsScroll;a++) SendMessage(psTE->pObj->hWnd,WM_VSCROLL,SB_LINEDOWN,0); //bContinue=TRUE;
				break;
*/
			//
			// Click del mouse (sinistro)
			//
			case EE_LBUTTONDOWN:
				 {
					SINT x,y,x2,y2;
					SINT cpy;
					SINT dif,a,crs,b,lenr;

					// Trovo le coordinate assolute
					if (sys.sTxtCursor.bVisible) txtCursor(FALSE);
					x=psTE->Px+relwx;
					y=psTE->Py+relwy;
					x2=psTE->Px+psTE->Lx-1+relwx;
					y2=psTE->Py+psTE->Ly-1+relwy;
					// dispx("%d,%d - %d,%d    (%d,%d) " ,x,y,x2,y2,sys.ms_x,sys.ms_y);
					if ((sys.ms_y<y)||(sys.ms_y>y2)) break;
					if ((sys.ms_x<x)||(sys.ms_x>x2)) break;

					// Controllo se sono sull'input
					//Aboxp(x,y,x2,y2,15,XOR);

					// Settaggio della Y
					cpy=sys.ms_y-y; cpy/=psTE->Falt;
					if ((cpy+psTE->Y_offset)<psTE->Lines) psTE->Cy=cpy;

					// Settaggio della X
					psTE->ptre=psTE->Y_offset+psTE->Cy;
					memoRead(psTE->Hdl,psTE->ptre*psTE->SizeLine,psTE->pszBufferRow,psTE->SizeLine);
					psTE->iRowSize=strlen(psTE->pszBufferRow);
					dif=30000;
					x2=sys.ms_x-x;
					x=-1;
//					LenCL=strlen(pszBufferRow); // Lunghezza in caratteri dell'input

					for (a=0;a<(psTE->iRowSize+1);a++)
					 {
						if (psTE->fHCT) 
							crs=HCTMakeDim(psTE,psTE->pszBufferRow,a,psTE->ptre);
							else
							crs=Wfont_dimh(psTE->pszBufferRow,a,psTE->hFont);

						//b=(mx-px)+psTE->X_offset;
						b=x2+psTE->X_offset-4;
						if (crs>b) lenr=crs-b; else lenr=b-crs;

						//sprintf(ss,"crs=%3d b=%3d lenr=%3d x=%3d",crs,b,lenr,x);
						//Adispm(0,20,0,14,OFF,SET,ss); getch();

						if (lenr<dif) {x=a;dif=lenr;}
					 //x=((mx-px)+((CHR_O-1)/2))/CHR_O; if (x>(lenx-1)) x=lenx-1;
					 }

					 if (x==-1) {x=x2; efxtext();}
					 psTE->Cx=x;
					iRetData=-100;
					if (!L_Shift()) LTE_SetSelection(psTE,0);
					
				}
				break;

			//
			// Pressione di un tasto
			//
			case EE_CHAR:

				cFirst=psEvent->iTasto&0xff;
				cSecond=psEvent->iTasto>>8;

				//	
				//	PRE-CONTROLLO SELEZIONE
				//	
	/*
				if (psTE->sSelBegin.iRow!=-1)
				{
					if (!cFirst&&!Shift())
					{
						BOOL fBreak=FALSE;
						switch (cSecond)
						{
							case _FDX:  psTE->Cx=psTE->sSelEnd.iChar;
										if (psTE->sSelBegin.iChar>psTE->Cx) psTE->Cx=psTE->sSelBegin.iChar;
										SELreset(TE);
										fBreak=TRUE;
										break;

							case _FSX:  psTE->Cx=psTE->sSelBegin.iChar;
										if (psTE->sSelEnd.iChar<psTE->Cx) psTE->Cx=psTE->sSelEnd.iChar;
										SELreset(TE);
										fBreak=TRUE;
										break;

							case _CANC: //psTE->Cx=psTE->sSelBegin.iChar;
										//if (psTE->sSelEnd.iChar<psTE->Cx) psTE->Cx=psTE->sSelEnd.iChar;
										//win_info("SI");
										SELdelete(TE);
										fBreak=TRUE;
										break;
							case _FUP:
							case _FDN:
										SELreset(TE);
										break;
						}
						if (fBreak) break; // 
					}
				}
	*/

				//
				// Controllo tasti speciali 
				//
				if (!cFirst) 
				{ 
					L_SpecialKeyManager_A(psTE,cSecond); break;
				}

				if (L_SpecialKeyManager_B(psTE,cFirst)) break;

				//
				//	         GESTIONE TASTO SEMPLICE 				
				//	
				//if (psTE->sSelBegin.iRow!=-1) {if (cFirst>31) SELdelete(TE);}

				if (cFirst==ESC) 
				{
					I_obj_setfocus(NULL);
					obj_putevent("%sESC",psTE->pObj->nome);
					break;
				}

				if ((cFirst<' ')&&(cFirst!=CR)) break;

				//
				//	Modalità INSERT Default
				//
				if (psTE->ptre>=psTE->Lines) {*psTE->pszBufferRow=0; psTE->Lines++;}

				if (IPT_ins)
				{
					if (psTE->iRowSize<psTE->SizeLine)
					{
//						SINT a;
						SINT iPiece=strlen(psTE->pszBufferRow+psTE->Cx);

						//for (a=psTE->iRowSize+1;a>psTE->Cx;a--) *(psTE->pszBufferRow+a)=*(psTE->pszBufferRow+a-1);
						if (iPiece) memmove(psTE->pszBufferRow+psTE->Cx+1,psTE->pszBufferRow+psTE->Cx,iPiece+1);
						psTE->pszBufferRow[psTE->Cx]=cFirst; if (!iPiece) psTE->pszBufferRow[psTE->Cx+1]=0;
						TE_LineRefresh(psTE,psTE->ptre,psTE->pszBufferRow);
						cSecond=0;
						psTE->Cx++;
								//goto control;
					}
					else
					{
						Tonk(); 
					}
				}
				else
				{
				
					//	Modalità REWRITE
					if (psTE->Cx<psTE->SizeLine)
					{
						*(psTE->pszBufferRow+psTE->Cx)=cFirst;
						if ((psTE->Cx+1)>psTE->iRowSize) *(psTE->pszBufferRow+psTE->Cx+1)=0;
						TE_LineRefresh(psTE,psTE->ptre,psTE->pszBufferRow);
						cSecond=0; //goto control;
					}
					else Tonk();
				}
				
				break;
		}
	}

	// 
	// POST CONTROLLO X SCROLL  
	// 
	bRedrawAll=FALSE;
	// ehPrintd("psTE->Cy=%d  " CRLF,psTE->Cy);
	// Limite inferiore
	if (psTE->Cy>(psTE->ncamE-1))
	{
		SINT spost;
		SINT kl;
		spost=(psTE->Cy-(psTE->ncamE-1));
		// Movimento per linee
		if (spost<3)
		{
			psTE->Cy=(psTE->ncamE-1);
			if (psTE->Cy>psTE->Lines) psTE->Cy=(SINT) psTE->Lines;
			for (kl=0;kl<spost;kl++)
			{
			  SendMessage(psTE->pObj->hWnd,WM_VSCROLL,SB_LINEDOWN,0);
			}
		}
		else
		// Movimento per pagine
		{
			psTE->Y_offset+=spost;
			if (psTE->Y_offset>(psTE->Lines-psTE->ncamE)) psTE->Y_offset=psTE->Lines-psTE->ncamE; // Controllo che l'offset non sia in overloa
			if (psTE->Y_offset<0) psTE->Y_offset=0; // ne troppo piccolo
			psTE->Cy=(psTE->ncamE-1);
			if (psTE->Cy>psTE->Lines) psTE->Cy=(SINT) psTE->Lines;
		//	psTE->Cy=(psTE->ncamE-1);
			//if (psTE->Cy>psTE->Lines) psTE->Cy=(SINT) psTE->Lines;
			if (spost>1) psTE->refre=ON;
			bRedrawAll=TRUE;
		}
	}

	// Limite superiore OK
	if (psTE->Cy<0)
	{
		if (psTE->Y_offset!=0)
		{
			SINT spost,kl;
			spost=-psTE->Cy;
			// Movimento per linee
			if (spost<3)
			{
				psTE->Cy=0;
				for (kl=0;kl<spost;kl++)
				{
					SendMessage(psTE->pObj->hWnd,WM_VSCROLL,SB_LINEUP,0);
				}
			}
			else
			// Movimento per pagine
			{
				psTE->Y_offset-=spost;
				if (psTE->Y_offset<0) psTE->Y_offset=0;
				psTE->Cy=0;
				if (spost>1) psTE->refre=ON;
				bRedrawAll=ON;
			 }
		}
		else psTE->Cy=0;
	}

	psTE->ptre=psTE->Y_offset+psTE->Cy;


	//
	// Il tasto shift è premuto (Selezione in corso)
	//
	//L_TextEditorControl(psTE);
	if (L_Shift()) 
	{
		if (!cFirst&&cSecond)
		{
			if (strchr("KMPHGO",cSecond)) LTE_SetSelection(psTE,1);
		}
	}

/*
	if (!cFirst&&cSecond&&(strchr("QIts",cSecond)!=NULL)) {SELend(psTE); if (Shift()) psTE->refre=ON;}
	if (!cFirst&&cSecond&&(strchr("KMPHGO",cSecond)!=NULL)) SELend(psTE);
	*/
	if (bRedrawAll) InvalidateRect(psTE->pObj->hWnd,NULL,FALSE);
//		TE_AreaRefresh(psTE, psTE->Y_offset,-1,FALSE);
	return 0;
}


// -------------------------------------------------------------
// OWSCRProcedure                                              |
// Funzione di Controllo della Finestra Child "Tabellone"      |
//                                                             |
// -------------------------------------------------------------

static void TE_RangeAdjust(EH_TEXTEDITOR *psTE)
{
  SCROLLINFO ScrollInfo;
  SINT MaxScroll;
  psTE->ncam=((psTE->Ly-1)/psTE->Falt)+1;
  
  MaxScroll=max(0,(SINT) (psTE->Lines+2-psTE->ncam));
  ScrollInfo.cbSize=sizeof(ScrollInfo);
  ScrollInfo.fMask=SIF_ALL;
  ScrollInfo.nPage=0;//TLayOut.Lines/((TLayOut.AreaY/TLayOut.CharY));
  ScrollInfo.nPos=psTE->Y_offset;
  ScrollInfo.nMin=0;
  ScrollInfo.nMax=MaxScroll;
  SetScrollInfo(psTE->pObj->hWnd,SB_VERT,&ScrollInfo,TRUE);
}


static LRESULT CALLBACK EhTextEditorProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;

  SINT iVScrollInc;
  EH_TEXTEDITOR *psTE;
  LONG lNewParam;

  S_WINSCENA Scena;
  SINT MaxScroll;
  SINT a,y;
  RECT Rect;
  EH_OBJ *poj=WndToObj(hWnd); 
  if (!poj) return(DefWindowProc(hWnd, message, wParam, lParam));
  psTE=poj->pOther;

  switch (message)
  {
     // Prima chiamata
	 case WM_CREATE: break;

     case WM_SIZE:
		  break;
/*
	 case WM_LBUTTONDOWN:
	 case WM_RBUTTONDOWN:
	 case WM_LBUTTONUP:
	 case WM_RBUTTONUP:
	 case WM_LBUTTONDBLCLK:
         WinMouseAction(hWnd,message,wParam,lParam);
		 break;
*/
  /*
	 case WM_MOUSEMOVE: 
	      if (poj==NULL) break;  
		  x=(INT16) poj->px+relwx+LOWORD(lParam)+2;
		  y=(INT16) poj->py+poj->yTitle+relwy+2+HIWORD(lParam);
		  WinMouse(x,y,0xFFFF);//(WORD) wParam); // Ho toccato questo
		  break;
    */
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_CHAR:
			//I_KeyTraslator_Windows(hWnd,message,lParam,wParam);
			CallWindowProc(EHStdWndProc,WIN_info[sys.WinInputFocus].hWnd,message,wParam,lParam);	
			break;

     case WM_MOUSEMOVE: 
		  WinMouse((INT16) psTE->Px+relwx+LOWORD(lParam)+2,
				  (INT16) psTE->Py+relwy+2+HIWORD(lParam),0xFFFF);
		  break;


	 case WM_LBUTTONDOWN: 
	 case WM_RBUTTONDOWN:
	 case WM_LBUTTONUP:
	 case WM_RBUTTONUP:
	 case WM_LBUTTONDBLCLK:

         //if (sys.WinInputFocus<0) break;
		 // Ricalcolo la posizione dell'intercettazione
		 // relativa alla posizione dell'oggetto
		 lNewParam=MAKELONG((LOWORD(lParam)+psTE->Px),(HIWORD(lParam)+psTE->Py));
         WinMouseAction(sys.WinInputFocus,WIN_info[sys.WinInputFocus].hWnd,message,wParam,lNewParam);
		 SetFocus(hWnd);
		 //GetWindowRect(hWnd,&Rect);
		 //WinMouse(psTE->Px+LOWORD(lParam)-Rect.left,psTE->Py+HIWORD(lParam)-Rect.top,lParam);
		 break;

	 case WM_NCHITTEST:
		  GetWindowRect(hWnd,&Rect);
		  WinMouse(psTE->Px+LOWORD(lParam)-Rect.left,psTE->Py+HIWORD(lParam)-Rect.top,0xFFFF);

		  break;
     // --------------------------------------------------------------------------------
     // Controllo Scorrimento VERTICALE                                                |
     // --------------------------------------------------------------------------------

     case WM_VSCROLL:

         TE_RangeAdjust(psTE);
		 iVScrollInc=0;
		 switch (LOWORD(wParam))
		   {
			case SB_TOP:      iVScrollInc=-iVScrollInc; break;
			case SB_BOTTOM:   iVScrollInc=psTE->ncam-iVScrollInc; break;
			case SB_LINEUP:   iVScrollInc=-1; break;
			case SB_LINEDOWN: iVScrollInc=1;break;
            case SB_PAGEUP:   iVScrollInc=min(-1,-psTE->ncam); break;
			case SB_PAGEDOWN: iVScrollInc=max(1,psTE->ncam); break;
			case SB_THUMBTRACK: 
			case SB_THUMBPOSITION:
			  iVScrollInc=HIWORD(wParam)-psTE->Y_offset;
			  break;
		   }

         MaxScroll=max(0,(SINT) (psTE->Lines+2-psTE->ncam));
		 iVScrollInc=max(-psTE->Y_offset,
			             min(iVScrollInc,MaxScroll-psTE->Y_offset));
		 /*
		 iVScrollInc=max(-TLayOut.iVScrollPos,
			             min(iVScrollInc,TLayOut.iVScrollMax-TLayOut.iVScrollPos));
*/
		 if (iVScrollInc!=0)
		   {
			BOOL fCur=sys.sTxtCursor.bVisible;
			if (fCur) txtCursor(FALSE);
			ScrollWindow(psTE->pObj->hWnd,0,-psTE->Falt*iVScrollInc,NULL,NULL);	
			if (fCur) txtCursor(TRUE);
			psTE->Y_offset+=iVScrollInc;
			SetScrollPos(psTE->pObj->hWnd,SB_VERT,psTE->Y_offset,TRUE);
			UpdateWindow(psTE->pObj->hWnd);
		   }
           break;

		 
	 case WM_DESTROY: break;
	 case WM_COMMAND: break;

	// ----------------------------------------------- 
	// Disegno la Tabella                            |
    // ----------------------------------------------- 

	 case WM_PAINT:

			hdc=BeginPaint(hWnd,&ps);
		    TE_RangeAdjust(psTE);
			WinDirectDC(hdc,&Scena,"p1");

			y=0;
			for (a=0;a<psTE->ncam;a++,y+=psTE->Falt)
			{
				if ((y+psTE->Falt)<ps.rcPaint.top) continue;
				if (y>ps.rcPaint.bottom) break;
				TE_LineDraw(psTE,hdc,y,a+psTE->Y_offset);
			}
			WinDirectDC(0,&Scena,NULL);
			EndPaint(hWnd,&ps);
			//win_infoarg("ws.numcam=%d %d",ws->numcam,poj->CharY);
			return 0;
			//break;

  }  

 return(DefWindowProc(hWnd, message, wParam, lParam));
}  

//
// L_SpecialKeyManager_A()
// Tasti a due caratteri (freccie, page down e up ed altro)
//
static BOOL L_SpecialKeyManager_A(EH_TEXTEDITOR *psTE,BYTE cSecond)
{
	BOOL bBreak=TRUE;
	//SINT LenCL=strlen(psTE->pszBufferRow);

	// Inizializzo la prima posizione della selezione
	if (strchr("KMPHQIGO",cSecond)!=NULL) SELstart(psTE);

	//				control:
	switch (cSecond) 
	{

		case  0: 
			//SELCarPlus(psTE);
			psTE->Cx++; // Carattere in inserito in pi—
			if (psTE->Cx>psTE->SizeLine)
			{
				psTE->Cx=psTE->SizeLine;
				Tonk();
			}
			break;

		case _FDX: //					FRECCIA DESTRA
			
			//if (!L_Shift()) LTE_SetSelection(psTE,0);
			if (!L_Shift()&&psTE->bSelection) 
			{
				LTE_SelOrder(&psTE->sSelBegin,&psTE->sSelEnd);
				psTE->Cx=psTE->sSelEnd.iChar;
				psTE->Cy=psTE->sSelEnd.iRow-psTE->Y_offset;
				break;
			}


			if (L_Control())
			{
				CHAR *p;
				CHAR bCar;
				SINT Type;
				bCar=psTE->pszBufferRow[psTE->Cx];
				Type=0;  if ((bCar>='0')&&(bCar<='z')) Type=1;
				psTE->Cx++;
				for (p=psTE->pszBufferRow+psTE->Cx;*p;p++,psTE->Cx++)
				{
				  if (*p<'!') continue;
				  if (!Type&&((*p>='0')&&(*p<='z'))) break; 
				  if (Type&&((*p<'0')||(*p>'z'))) break; 
				}
			}
			else
			psTE->Cx++;

			break;

		case _FSX: // 					FRECCIA SINISTRA

			if (!L_Shift()&&psTE->bSelection) 
			{
				LTE_SelOrder(&psTE->sSelBegin,&psTE->sSelEnd);
				psTE->Cx=psTE->sSelBegin.iChar;
				psTE->Cy=psTE->sSelBegin.iRow-psTE->Y_offset;
				break;
			}

			if (L_Control())
			{
				CHAR *p;
				CHAR bCar;
				SINT Type;
				psTE->Cx--;
				if (psTE->Cx>0)
				{
				 bCar=psTE->pszBufferRow[psTE->Cx];
				 //win_infoarg("[%c]",bCar);
				 Type=0;  if ((bCar>='0')&&(bCar<='z')) Type=1;
				 psTE->Cx--;
				 if (psTE->Cx>0)
				 {
				  for (p=psTE->pszBufferRow+psTE->Cx;*p;p--,psTE->Cx--)
				  {
					if (*p<'!') continue;
					if (!Type&&((*p>='0')&&(*p<='z'))) break; 
					if (Type&&((*p<'0')||(*p>'z'))) break; 
				  }
				  psTE->Cx++;
				 }
				}
			}
			else
			psTE->Cx--;
			if (psTE->Cx<0) {psTE->Cx=0;goto CASO1;}
			break;

		//					FRECCIA GIU'
		case _FDN: 
			if (psTE->ptre>=psTE->Lines) efx1(); else psTE->Cy++;
			break;

		//					FRECCIA SU'
		case _FUP: 
			if (!L_Shift()&&psTE->bSelection) 
			{
				LTE_SelOrder(&psTE->sSelBegin,&psTE->sSelEnd);
				psTE->Cx=psTE->sSelBegin.iChar;
				psTE->Cy=psTE->sSelBegin.iRow-psTE->Y_offset;
				break;
			}

			if (psTE->ptre==0) efx1(); else psTE->Cy--;
			break;

		//					PageDown
		case _PGDN: 
			if (psTE->ptre>=(psTE->Lines-1)) efx1(); else psTE->Cy+=psTE->ncamE;
			break;

		//					PageUP
		case _PGUP: 
			if (psTE->ptre==0) efx1(); else psTE->Cy-=psTE->ncamE;
			break;

		//					DELETE
		case _CANC:
									    
			if (L_Shift()) {SELcopia(psTE); SELdelete(psTE); break;}
			if (psTE->bSelection) {SELdelete(psTE); break;}

			if (psTE->Cx<psTE->iRowSize)
				 // Cancellazione semplice
				 {SINT z;
				  z=psTE->Cx;
				  for(;*(psTE->pszBufferRow+z);) *(psTE->pszBufferRow+z++)=*(psTE->pszBufferRow+z+1);
				  //SELCarMinus(psTE);
				  TE_LineRefresh(psTE,psTE->ptre,psTE->pszBufferRow);
				 }
				 else
				 // Aggiungi riga in coda
				 {SINT Size2,TotLen;

					//dispx("[%d][%d]        ",psTE->ptre,psTE->Lines);
					if (psTE->ptre>=(psTE->Lines-1)) {Tonk();break;}
					memoRead(psTE->Hdl,(psTE->ptre+1)*psTE->SizeLine,psTE->pszBufferRow,psTE->SizeLine);
					//win_infoarg("A) [%s] [%d]",pszBufferRow,psTE->Y_offset);
					Size2=strlen(psTE->pszBufferRow);
					TotLen=psTE->Cx+Size2;
					//if (psTE->ptre!=(psTE->Y_offset+psTE->Cy)) efx3();

					if (TotLen>(psTE->SizeLine-1))
						{
						 win_info("ADDIZIONE LINEA IMPOSSIBILE:\nLinea troppo lunga");
						 break;
						}

					memoRead(psTE->Hdl,
									psTE->ptre*psTE->SizeLine,
									psTE->pszBufferRow,
									psTE->SizeLine);

					memoRead(psTE->Hdl,
									(psTE->ptre+1)*psTE->SizeLine,
									psTE->pszBufferRow+psTE->Cx,
									Size2+1);

					TE_LineWrite(psTE,psTE->ptre,psTE->pszBufferRow);
					TE_LineDelete(psTE,psTE->ptre+1,1);
					//SELLineMinus(psTE,psTE->Cy,OFF);
					psTE->refre=ON;
					psTE->fNoYOffsetCalc=TRUE;
					TE_AreaRefresh(psTE,psTE->Cy+psTE->Y_offset,-1);
					psTE->fNoYOffsetCalc=FALSE;
				 }
			break;

		//					INSERT
		case 'R':
			// Control+INS
			
			if (key_pressS(_CTRL,OR)) 
			{
				SELcopia(psTE);
			}

			if (L_Shift())
				 {//win_time("INCOLLA",1|NOWIN);
					SELincolla(psTE,NULL);
					break;
				 }

			if (IPT_ins) IPT_ins=OFF; else IPT_ins=ON;
			break;

		//					CONTROL + HOME
		case 'w': break;
		//					CONTROL + END
		case 'u': break;    

		//					HOME
		case _HOME:
			psTE->Cx=0; 
			if (L_Control()) {psTE->Cy=0; psTE->Y_offset=0; TE_AreaRefresh(psTE,-1,-1);}
			break;
		//					END
		case 'O': 
			psTE->Cx=strlen(psTE->pszBufferRow); 
			break;

		//					CONTROL + -->
		case 't':
			if (psTE->Cx>=psTE->iRowSize)
				 {psTE->ptre++; if (psTE->ptre>=(psTE->Lines-1)) {Tonk();break;}
					memoRead(psTE->Hdl,(psTE->ptre+1)*psTE->SizeLine,psTE->pszBufferRow,psTE->SizeLine);
					psTE->iRowSize=strlen(psTE->pszBufferRow);
					psTE->Cx=0; psTE->Cy++;
				 }
			psTE->Cx+=strcspn(psTE->pszBufferRow+psTE->Cx," ");
			// Cerca il non spazio
			for (;psTE->Cx<psTE->iRowSize;psTE->Cx++)
					{if (*(psTE->pszBufferRow+psTE->Cx)>' ') break;}
			break;

		//					CONTROL + <--
		case 's':
			if (psTE->Cx==0)
				 {CASO1:
					if (psTE->ptre==0) {Tonk();break;}
					psTE->ptre--;
					memoRead(psTE->Hdl,psTE->ptre*psTE->SizeLine,psTE->pszBufferRow,psTE->SizeLine);
					psTE->iRowSize=strlen(psTE->pszBufferRow);
					//psTE->Cx=0; psTE->Cy++;
					psTE->Cx=strlen(psTE->pszBufferRow);
					psTE->Cy--;
					break;
				 }
			// Cerca il non spazio

			if (psTE->Cx>0) psTE->Cx--;
			for (;psTE->Cx>-1;psTE->Cx--) if (*(psTE->pszBufferRow+psTE->Cx)>' ') break;
			// Cerca lo spazio
			for (;psTE->Cx>-1;psTE->Cx--) if (*(psTE->pszBufferRow+psTE->Cx)==' ') break;
			// Cerca il non spazio
			for (;psTE->Cx<(SINT) strlen(psTE->pszBufferRow);psTE->Cx++) if (*(psTE->pszBufferRow+psTE->Cx)>' ') break;

			if (psTE->Cx<0) psTE->Cx=0;
			break;

						//	CONTROL + Ins (Copia)
		default:
			bBreak=FALSE;
			break;

	//default : printf("%c (%d)",keybuf[1],(SINT) keybuf[1]);getch();break;
	};
	if (!L_Shift()) LTE_SetSelection(psTE,0);
	return bBreak;
}

//
// L_SpecialKeyManager_B()
// Tasti ad un carattere (control + qualcosa)
//
static BOOL L_SpecialKeyManager_B(EH_TEXTEDITOR *psTE,BYTE cFirst)
{
	BOOL bBreak=TRUE;
//	SINT psTE->iRowSize=strlen(psTE->pszBufferRow);
	switch(cFirst)
	{
		case 3: // Control + C  (Copia)
				SELcopia(psTE);   
				break;

		case 22: // Control + V (Incolla)
				SELincolla(psTE,NULL);
				break;
	/*
	case 11 : 
			mouse_graph(0,3,"MS05");
//					eventGetWait(NULL);

			// ----------------------
			// Toglie la selezione  !
			// ----------------------
			if (key_press('H')||key_press('h')&&(psTE->sSelEnd.iRow!=-1))
				{
					SELRES:
					psTE->sSelBegin.iChar=-1;
					psTE->sSelBegin.iRow=-1;
					psTE->sSelEnd.iChar=-1;
					psTE->sSelEnd.iRow=-1;

					rinfresca:
					psTE->refre=ON; TE_AreaRefresh(TE, 0,-1,FALSE);
					goto OKVABENE;
				}

			// -----------------------
			// Cancella la Selezione !
			// -----------------------
			if (key_press('Y')||key_press('y')&&(psTE->sSelEnd.iRow!=-1))
				{

					SELdelete(TE);
					goto SELRES;
				}

			// ----------------
			// Marca l'inizio !
			// ----------------
			if (key_press('B')||key_press('b'))
				{
					SEL1:
					psTE->sSelBegin.iChar=psTE->Cx;
					psTE->sSelBegin.iRow=psTE->ptre;
					if (psTE->sSelEnd.iRow!=-1) goto rinfresca;
				}

			// ----------------
			// Marca la fine  !
			// ----------------
			if (key_press('K')||key_press('k'))
				{
					psTE->sSelEnd.iChar=psTE->Cx;
					psTE->sSelEnd.iRow=psTE->ptre;
					if (psTE->sSelBegin.iRow==-1) goto SEL1;
					goto rinfresca;
				}

	OKVABENE:
	mouse_graph(0,0,"MS01");
	goto USCITA;
	*/

		//	Ctrl+Y cancella il campo o una riga
		case  25 : 
			if (psTE->ptre>=psTE->Lines) {Tonk();break;}
			psTE->Cx=0;
			TE_LineDelete(psTE,psTE->ptre,1);
//			SELLineMinus(psTE,psTE->Cy,ON);
			psTE->refre=ON;
			TE_AreaRefresh(psTE,psTE->Cy+psTE->Y_offset,psTE->ncamE+psTE->Y_offset);
			break;

		//	Carrage Return
		case  CR:
			TE_LineInsert(psTE,psTE->ptre+1,1);
			TE_LineWrite(psTE,psTE->ptre+1,psTE->pszBufferRow+psTE->Cx);
			*(psTE->pszBufferRow+psTE->Cx)=0;
			TE_LineWrite(psTE,psTE->ptre,psTE->pszBufferRow);

			// Ristampa tutto
			psTE->refre=ON;
			TE_AreaRefresh(psTE,psTE->Cy+psTE->Y_offset,psTE->ncamE+psTE->Y_offset);
			psTE->Cx=0;psTE->Cy++;
			//goto USCITA;
			break;

		// Tabulazione
		case  9:
			if (IPT_ins)
			{if ((SINT) strlen(psTE->pszBufferRow)<psTE->SizeLine)
				
			 memmove(psTE->pszBufferRow+psTE->Cx+1,psTE->pszBufferRow+psTE->Cx,psTE->SizeLine-psTE->Cx-1);
			 *(psTE->pszBufferRow+psTE->Cx)=cFirst;
			 TE_LineRefresh(psTE,psTE->ptre,psTE->pszBufferRow);
			 //cSecond=0; goto control;
			}
			break;

		//	Backspace
		case  8: 

			if ((psTE->Cx<=psTE->iRowSize)&&(psTE->iRowSize!=0))
			{
				SINT z;
				if (psTE->Cx>0)
				{
					psTE->Cx--;
					FAINORM:
					z=psTE->Cx;
					for(;*(psTE->pszBufferRow+z);) *(psTE->pszBufferRow+z++)=*(psTE->pszBufferRow+z+1);
					//SELCarMinus(psTE);
					TE_LineRefresh(psTE,psTE->ptre,psTE->pszBufferRow);
				}
				else
				{
					//
					// Aggiungo la riga sotto a quella sopra
					//
					BYTE *pBuffer;
					SINT iNextSize;
					if (psTE->ptre==0) {psTE->Cx=0; goto FAINORM;}
					pBuffer=strDup(psTE->pszBufferRow);
					psTE->ptre--;
					memoRead(psTE->Hdl,psTE->ptre*psTE->SizeLine,psTE->pszBufferRow,psTE->SizeLine);
					iNextSize=strlen(psTE->pszBufferRow)+strlen(pBuffer)+1;
					if (iNextSize>psTE->SizeLine) {ehFree(pBuffer); break;}
					psTE->iRowSize=strlen(psTE->pszBufferRow);
					psTE->Cx=psTE->iRowSize; psTE->Cy--;
					strcat(psTE->pszBufferRow,pBuffer);
					memoWrite(psTE->Hdl,psTE->ptre*psTE->SizeLine,psTE->pszBufferRow,psTE->SizeLine);

					//
					// Cancello la riga sotto
					//
					TE_LineDelete(psTE,psTE->ptre+1,1);
					//TE_AreaRefresh(psTE,psTE->Cy,psTE->ncamE,FALSE);
					ehFree(pBuffer); // Devo rinfrescare -> le due linee (quella prima e quella corrente)
					psTE->refre=ON;
					TE_AreaRefresh(psTE,psTE->ptre,psTE->ncamE);
				}
			 }
			//goto USCITA;
			break;
		default:
			bBreak=FALSE;
			break;
	}
	return bBreak;
}

//
// TE_AreaRefresh()
// Chiede di rinfrescare un tot di linee
//
static void TE_AreaRefresh(EH_TEXTEDITOR *psTE,SINT iLineBegin,SINT iLineEnd)
{
	RECT rcRect;

	// Ridisegno totale
	if (iLineBegin<0&&iLineEnd<0)
	{
		InvalidateRect(psTE->pObj->hWnd,NULL,FALSE);
		return;
	}

	if (iLineEnd<0) iLineEnd=psTE->Y_offset+psTE->ncamE;

	//
	// Calcolo il rettangolo da Rinfrescare
	//
	iLineBegin-=psTE->Y_offset; if (iLineBegin<0) iLineBegin=0;
	iLineEnd-=psTE->Y_offset; if (iLineEnd<0) return;

	rcRect.left=0;
	rcRect.right=sys.video_x;
	rcRect.top=iLineBegin*psTE->Falt;
	rcRect.bottom=(iLineEnd+1)*psTE->Falt;
	InvalidateRect(psTE->pObj->hWnd,&rcRect,FALSE); UpdateWindow(psTE->pObj->hWnd);
}

/*
static void TE_AreaRefresh(EH_TEXTEDITOR *psTE,SINT da,SINT a,BOOL Flag)
{
	RECT Rect;
	// Ridisegno totale
	if ((da==-1)&&(a==-1))
	{
	InvalidateRect(psTE->pObj->hWnd,NULL,Flag);
	//  InvalidateRect(psTE->pObj->hWnd,NULL,FALSE);
	return;
	}

	if (da<0) da=0;
	if (a<0) a=psTE->ncam;

	Rect.left=0;
	Rect.top=da*psTE->Falt;
	Rect.right=psTE->Lx;
	Rect.bottom=(a+1)*psTE->Falt;
	InvalidateRect(psTE->pObj->hWnd,&Rect,FALSE);
}
*/