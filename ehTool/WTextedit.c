//   +-------------------------------------------+
//   | WTextEditor Gestione di editor testi      |
//   |             Edita un testo tipo word      |
//   |             versione Windows              |
//   | comandi                                   |
//   |                                           |
//   | APRI : predefinire TEDIT struct           |
//   |                                           |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1997 |
//   +-------------------------------------------+

#include "\ehtool\include\ehsw_i.h"
#include "\ehtool\textedit.h"

	//CHAR   Riga[300]; // E' da togliere al pi— presto
	extern SINT IPT_ins;
	extern SINT WIN_fz;

static void TE_LineWrite(LONG RigaFile,CHAR *Rig,TEDIT *TE);
static void TE_LineRead(LONG RigaFile,TEDIT *TE);
static void TE_LineInsert(LONG RigaFile,LONG num,TEDIT *TE);
static void TE_LineDelete(LONG RigaFile,LONG num,TEDIT *TE);

#define MARGINDX 2

static void SelVedi(TEDIT *TE)
{
   CHAR Serv[120];
   //WINSCENA WScena;
   sprintf(Serv," (%d,%d) - (%d,%d) ",
	       TE->SEL_Xstart,TE->SEL_Ystart,
		   TE->SEL_Xend,TE->SEL_Yend);
   
//   WinWriteSet(0,&WScena);
//   dispf(400,10,0,15,ON,"SMALL F",3,Serv);
//   WinWriteRestore(&WScena);
}

void TE_drw(TEDIT *TE,SINT da,SINT a)
{
 RECT Rect;
 // Ridisegno totale
 if ((da==-1)&&(a==-1))
 {
  InvalidateRect(TE->hWnd,NULL,TRUE);
//  InvalidateRect(TE->hWnd,NULL,FALSE);
  return;
 }

 if (da<0) da=0;
 if (a<0) a=TE->ncam;

 Rect.left=0;
 Rect.top=da*TE->Falt;
 Rect.right=TE->Lx;
 Rect.bottom=(a+1)*TE->Falt;
 InvalidateRect(TE->hWnd,&Rect,FALSE);
}

static LRESULT CALLBACK TEDITProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

TEDIT *LastTE=NULL;

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
void TE_LineWrite(LONG RigaFile,CHAR *Rig,TEDIT *TE)
{
 TE->Touch=ON;
// memo_scrivivar(TE->Hdl,RigaFile*TE->SizeLine,Rig,strlen(Rig)+2);
 memo_scrivivar(TE->Hdl,RigaFile*TE->SizeLine,Rig,TE->SizeLine);
}
// -----------------------------
//  Legge una riga dal file    !
// -----------------------------
void TE_LineRead(LONG RigaFile,TEDIT *TE)
{
 // memo_leggivar(TE->Hdl,RigaFile*TE->SizeLine,&Riga,TE->SizeLine);
 memo_leggivar(TE->Hdl,RigaFile*TE->SizeLine,TE->CampoLinea,TE->SizeLine);
}

void Tonk(void)
{
 //sonic(5000,1,1,2,2,5);
}

// +-----------------------------------------+
// | RIGAInsert                              |
// |                                         |
// |                                         |
// |         Ferr… Art & Technology (c) 1997 |
// +-----------------------------------------+
void TE_LineInsert(LONG RigaFile,LONG num,TEDIT *TE)
{
 LONG l;
 //printf("%d",TE->SizeLine);
 // -----------------------------------------------
 // Inserisce una riga                            !
 // -----------------------------------------------
 for (l=TE->Lines;l>=RigaFile;l--)
			{
			 memo_leggivar(TE->Hdl,l*TE->SizeLine,TE->Riga2,TE->SizeLine);
			 memo_scrivivar(TE->Hdl,(l+num)*TE->SizeLine,TE->Riga2,TE->SizeLine);
			}
 memset(TE->Riga2,0,TE->SizeLine);
 //RIGAWrite(RigaFile,Riga);
 memo_scrivivar(TE->Hdl,RigaFile*TE->SizeLine,TE->Riga2,TE->SizeLine);
 TE->Lines+=num;
}

void TE_LineDelete(LONG RigaFile,LONG num,TEDIT *TE)
{
 LONG l;

 // -----------------------------------------------
 // Cancella una riga                             !
 // -----------------------------------------------
 for (l=RigaFile;l<TE->Lines;l++)
			{
			 memo_leggivar(TE->Hdl,(l+num)*TE->SizeLine,TE->Riga2,TE->SizeLine);
			 memo_scrivivar(TE->Hdl,l*TE->SizeLine,TE->Riga2,TE->SizeLine);
			}

 memset(TE->Riga2,0,TE->SizeLine);
 //RIGAWrite(RigaFile,Riga);
 memo_scrivivar(TE->Hdl,(TE->Lines-1)*TE->SizeLine,TE->Riga2,TE->SizeLine);
 TE->Lines-=num;
}

// -------------------------------------------
// | TE_drw   Visualizza i dati del			 |
// |          text-windows     				 |
// |                               			 |
// |                            by Ferr… 1997|
// -------------------------------------------

void TE_LineDraw(TEDIT *TE,HDC hDC,SINT Py,SINT Line)
{
	SINT  x1,y1,x2,cam_x,cam_y;
	SINT  spiazi,lx,ly,refre;
	LONG koffset;
	SINT  ncam;
	LONG offset;
	SINT  fontalt;
	CHAR *PtRiga=TE->Riga2;

	SINT daPulire=ON;
	LONG ColPulish;
	SINT Col1,Col2;

	SINT SEL_X1;
	SINT SEL_X2;
	LONG SEL_Y1;
	LONG SEL_Y2;

	SEL_X1=TE->SEL_Xstart;
	SEL_X2=TE->SEL_Xend;
	SEL_Y1=TE->SEL_Ystart;
	SEL_Y2=TE->SEL_Yend;

	// ------------------------------------------
	//  Swapping delle Coordinate di Selezione  !
	// ------------------------------------------
	Col1=TE->Col1;
	Col2=TE->Col2;

	if (SEL_Y2!=-1)
	{
	 if ((SEL_Y1==SEL_Y2)&&(SEL_X1>SEL_X2))	 {SINT a;a=SEL_X2;SEL_X2=SEL_X1;SEL_X1=a;}
	 
	 if (SEL_Y1>SEL_Y2)
	 {SINT a;
	  LONG b;
	  a=SEL_X2;SEL_X2=SEL_X1;SEL_X1=a;
	  b=SEL_Y2;SEL_Y2=SEL_Y1;SEL_Y1=b;
	 }
	}

	fontalt=TE->Falt;//font_alt(poj->fonthdl,poj->nfi);
	refre=ON;//TE->refre;

	lx=TE->Lx; ncam=TE->ncam; ly=TE->Ly;

    x1=0;y1=Py;  
	x2=TE->Lx; 
    if (Line<0) Line=0; 

	offset=TE->Y_offset;
	koffset=TE->Y_koffset;

	cam_x=x1+MARGINDX; cam_y=y1;

	// Tapullo
	if (Line>=TE->Lines) {boxp(x1,cam_y,x2,cam_y+fontalt-1,4,SET); return;}
	Tboxp(x1,cam_y,x2-1,cam_y+TE->Falt-1,Col2,SET);
	
	memo_leggivar(TE->Hdl,Line*TE->SizeLine,TE->Riga2,TE->SizeLine);

	ColPulish=Col2;
	spiazi=0;

	// ------------------------------------------------------
	// Stampa con la SELEZIONE                              !
	// ------------------------------------------------------

	// Nessuna selezione

	if (SEL_Y2!=-1)
	{
	    if ((Line==SEL_Y2)&&(SEL_X2==0)) SEL_Y2=-1;
		if ((Line<SEL_Y1)||(Line>SEL_Y2)) SEL_Y2=-1;
	}

	if ((TE->SEL_Xstart==TE->SEL_Xend)&&(TE->SEL_Ystart==TE->SEL_Yend)) SEL_Y2=-1;

	if (SEL_Y2!=-1)
		{
		 // Tutto Selezionato
		 if ((Line>SEL_Y1)&&(Line<SEL_Y2))
			{
			 Wdispf(cam_x-(SINT) TE->X_offset,cam_y,Col2,Col1,&TE->LogFont,TE->Riga2);
			 spiazi=Wfont_len(TE->Riga2,&TE->LogFont);
			 ColPulish=Col1;
			 goto PULISH;
			}
/*
		 // Tutto non selezionato
		 if ((Line<SEL_Y1)||(Line>SEL_Y2))
			{
			 Wdispf(cam_x-(SINT) TE->X_offset,cam_y,Col2,Col1,&TE->LogFont,TE->Riga2);
			 spiazi=Wfont_len(TE->Riga2,&TE->LogFont);
			 goto PULISH;
			}
*/

		 if (Line==SEL_Y1)
			{CHAR bak;

			 // --------------------------
			 // TESTA: Non selezionata   |
			 // --------------------------
			 if (SEL_X1>0)
			 {
			  bak=*(TE->Riga2+SEL_X1);
			  *(TE->Riga2+SEL_X1)=0;
              spiazi=Wfont_len(PtRiga,&TE->LogFont);
			  Wdispf(cam_x-(SINT) TE->X_offset,cam_y,Col1,Col2,&TE->LogFont,TE->Riga2);
              *(TE->Riga2+SEL_X1)=bak;
			 }

			 // ------------------------------------
			 // Y1=Y2 Corpo Centrale SELEZIONATO   | 3 Parti  NO|SI|NO
			 // ------------------------------------
			 if (Line==SEL_Y2) 
				{bak=*(TE->Riga2+SEL_X2);
			     *(TE->Riga2+SEL_X2)=0;
			     Wdispf(cam_x-(SINT) TE->X_offset+spiazi,cam_y,Col2,Col1,&TE->LogFont,PtRiga+SEL_X1);
                 spiazi+=Wfont_len(PtRiga+SEL_X1,&TE->LogFont);
				 *(TE->Riga2+SEL_X2)=bak;

				 // Coda Non selezionata     !
				 Wdispf(cam_x-(SINT) TE->X_offset+spiazi,cam_y,Col1,Col2,&TE->LogFont,PtRiga+SEL_X2);
			     spiazi+=Wfont_len(PtRiga+SEL_X2,&TE->LogFont);
			     ColPulish=Col2;
				}
			 // ------------------------------------
			 // Y1!=Y2 Corpo Centrale SELEZIONATO   | 2 Parti  NO|SI
			 // ------------------------------------
                else
				{
			     Wdispf(cam_x-(SINT) TE->X_offset+spiazi,cam_y,Col2,Col1,&TE->LogFont,PtRiga+SEL_X1);
                 spiazi+=Wfont_len(PtRiga+SEL_X1,&TE->LogFont);
				 ColPulish=Col1;
				}


			 goto PULISH;
			}

	if (Line==SEL_Y2)
		{
		 CHAR bak;
		 // Non selezionato
		 bak=*(TE->Riga2+SEL_X2); *(TE->Riga2+SEL_X2)=0;
		 Wdispf(cam_x-(SINT) TE->X_offset,cam_y,Col2,Col1,&TE->LogFont,PtRiga);
		 spiazi=Wfont_len(PtRiga,&TE->LogFont);
		 *(TE->Riga2+SEL_X2)=bak;

		 //spiazi+=Wfont_len(PtRiga+SEL_X2,&TE->LogFont);
		 Wdispf(cam_x-(SINT) TE->X_offset+spiazi,cam_y,Col1,Col2,&TE->LogFont,PtRiga+SEL_X2);
		 spiazi+=Wfont_len(PtRiga+SEL_X2,&TE->LogFont);
		 ColPulish=Col2;
         goto PULISH;
		}
	}
	
	else

	// ----------------------
	// Stampa normale       !
	// ----------------------
	{
	  // ColorText
	  // CBii<------------>
	  Wdispf(cam_x-(SINT) TE->X_offset,cam_y,Col1,Col2,&TE->LogFont,TE->Riga2);
	  spiazi=Wfont_len(TE->Riga2,&TE->LogFont);
	}
	 
PULISH:
	//if (daPulire)
//	Tboxp(cam_x+spiazi-(SINT) TE->X_offset,cam_y,x2+1,cam_y+fontalt-1,RGB(255,255,0),SET);
	Tboxp(cam_x+spiazi-(SINT) TE->X_offset,cam_y,x2+1,cam_y+fontalt-1,ColPulish,SET);

//	TE->Y_koffset=TE->Y_offset;
//	TE->X_koffset=TE->X_offset;

//	if (refre==ON) TE->refre=OFF; // 	refresh schermo fatto
}

void TE_LineRefresh(LONG Line,CHAR *CampoLinea,TEDIT *TE)
{
	TE_LineWrite(Line,CampoLinea,TE);
	TE->refre=ON;
	TE_drw(TE,TE->Cy,TE->Cy);
}

static SINT Shift(void)
{
    if (key_pressS(_SHIFTDX|_SHIFTSX,OR)) return ON;
	return OFF;
}

static void SELreset(TEDIT *TE)
{
 SINT k1,k2;
 SINT k;

 if (TE->SEL_Yend==-1) return;
 k1=(SINT) (TE->SEL_Ystart-TE->Y_offset);
 k2=(SINT) (TE->SEL_Yend-TE->Y_offset);     

 //printf("DEL %d->%d",k1,k2);
 TE->SEL_Xend=-1;
 TE->SEL_Yend=-1;
 if (k2<0) return;

 if (k1<0) k1=0;
 if (k2>TE->ncam) k2=TE->ncam;
 TE->refre=ON;

 if (k1>k2) {k=k2; k2=k1; k1=k;}

 TE_drw(TE,k1,k2);
}



static void SELstart(TEDIT *TE)
{
 static SINT UltimoShift=OFF;

 if (!Shift()) {UltimoShift=OFF;return;}

 if ((UltimoShift==ON)&&(TE->SEL_Xstart!=-1)) return;

 //if (TE->SEL_Xstart==-1)
 SELreset(TE);

 TE->SEL_Xstart=TE->Cx;
 TE->SEL_Ystart=TE->ptre;
 TE->SEL_Xend=-1;
 TE->SEL_Yend=-1;

 UltimoShift=ON;
}

static void SELend(TEDIT *TE)
{
 UINT x,k1,k2;
 SINT OldTEY=TE->SEL_Yend;
 if (!Shift()) return;

 TE_LineRead(TE->ptre,TE);

 x=TE->Cx;
 if (x>strlen(TE->CampoLinea)) x=strlen(TE->CampoLinea);

 TE->SEL_Xend=x;
 TE->SEL_Yend=TE->ptre;

 TE->refre=ON;
 k1=TE->Cy; k2=TE->Cy;
 if (OldTEY!=TE->SEL_Yend) {k1--; k2++;}

// if (TE->SEL_Ystart<TE->ptre) {k1=TE->Cy-1; if (k1<0) k1=0;}
// if (TE->SEL_Ystart>TE->ptre) {k2=TE->Cy+1; if (k2>TE->ncam) k2=TE->ncam;}
 if (k1<0) k1=0;
 if (k2>(UINT)TE->ncam) k2=TE->ncam;
 TE_drw(TE,k1,k2);
}

static void SELcopia(TEDIT *TE);
static void SELincolla(TEDIT *TE);
static void SELdelete(TEDIT *TE);

static void SELLinePlus(TEDIT *TE,LONG dove);
static void SELLineMinus(TEDIT *TE,LONG dove,SINT Flag);

static void SELCarPlus(TEDIT *TE);
static void SELCarMinus(TEDIT *TE);

// ------------------------------------------
//  Swapping delle Coordinate di Selezione  !
// ------------------------------------------

static SINT SELcoo(TEDIT *TE,SINT *X1,LONG *Y1,SINT *X2,LONG *Y2)
{
	SINT SEL_X1,SEL_X2;
	LONG SEL_Y1,SEL_Y2;

	SEL_X1=TE->SEL_Xstart; SEL_X2=TE->SEL_Xend;
	SEL_Y1=TE->SEL_Ystart; SEL_Y2=TE->SEL_Yend;

	if (SEL_Y2!=-1)
	{
	if ((SEL_Y1==SEL_Y2)&&(SEL_X1>SEL_X2))
	 {SINT a;
		a=SEL_X2;SEL_X2=SEL_X1;SEL_X1=a;
	 }
	if (SEL_Y1>SEL_Y2)
	 {SINT a;
		LONG b;
		a=SEL_X2;SEL_X2=SEL_X1;SEL_X1=a;
		b=SEL_Y2;SEL_Y2=SEL_Y1;SEL_Y1=b;
	 }
	} else return ON;


   *X1=SEL_X1;
   *X2=SEL_X2;
   *Y1=SEL_Y1;
   *Y2=SEL_Y2;

   return OFF;
}

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
static void SELLineMinus(TEDIT *TE,LONG dove,SINT Flag)
{
	SINT SEL_X1,SEL_X2;
	LONG SEL_Y1,SEL_Y2;

	if (SELcoo(TE,&SEL_X1,&SEL_Y1,&SEL_X2,&SEL_Y2)) return;

	if (Flag) SEL_X1=0;
	//printf("DELline %ld, X1:%d Y1:%ld X2:%d Y2:%ld \n",dove,SEL_X1,SEL_Y1,SEL_X2,SEL_Y2);

	if (dove>SEL_Y2) return;

	// Cancello la selezione
	if ((dove==SEL_Y2)&&(SEL_X2==0)) return;

	if ((SEL_Y1==SEL_Y2)&&(dove==SEL_Y1)) {SEL_Y2=-1; SEL_Y1=-1; goto java;}

	if ((dove==(SEL_Y2-1))&&(SEL_X2==0)) {SEL_Y2--; SEL_X2=TE->Cx; goto java;}

	if ((dove==(SEL_Y2-1))&&(SEL_X2>0))
		{if (SEL_Y2==SEL_Y1) {SEL_Y1--;SEL_X1+=TE->Cx;}
		 SEL_Y2--;SEL_X2+=TE->Cx;
		 goto java;}
	if ((dove==SEL_Y2)&&(SEL_X2>0)) {goto java;}

	if ((TE->Cx>0)&&(dove==(SEL_Y1-1)))
		{SEL_Y1--;SEL_Y2--; SEL_X1+=TE->Cx;
		 goto java;}

	if (dove<SEL_Y1) {SEL_Y1--;SEL_Y2--;}
	if (dove>=SEL_Y1) SEL_Y2--;


	java:
	if ((SEL_Y1==SEL_Y2)&&(SEL_X1==SEL_X2)) {SEL_Y2=-1; SEL_Y1=-1;}

	TE->SEL_Ystart=SEL_Y1; TE->SEL_Yend=SEL_Y2;
	TE->SEL_Xstart=SEL_X1; TE->SEL_Xend=SEL_X2;
}

//-----------------------------------------------
// Modificato il file la selezione si aggiorna  !
//-----------------------------------------------
static void SELLinePlus(TEDIT *TE,LONG dove)
{
	SINT SEL_X1,SEL_X2;
	LONG SEL_Y1,SEL_Y2;

	if (SELcoo(TE,&SEL_X1,&SEL_Y1,&SEL_X2,&SEL_Y2)) return;

	//printf("INSline %ld, X1:%d Y1:%ld X2:%d Y2:%ld \n",dove,SEL_X1,SEL_Y1,SEL_X2,SEL_Y2);

	if ((dove==SEL_Y2)&&(SEL_X2==0)) return;
	if ((dove==SEL_Y2)&&(TE->Cx>SEL_X2)) return;

	if ((dove==SEL_Y1)&&(TE->Cx<SEL_X1))
	 {
		SEL_Y1++; SEL_Y2++;SEL_X1-=TE->Cx;
		if (SEL_Y1==SEL_Y2) SEL_X2-=TE->Cx;
		goto java;}


	// Sistema la fine orizzontale della selezione
	if ((dove==SEL_Y2)||((dove==(SEL_Y2-1))&&(SEL_X2==0)))
		{
		 memo_leggivar(TE->Hdl,dove*TE->SizeLine,TE->Riga2,TE->SizeLine);
		 if ((dove==(SEL_Y2-1))&&(SEL_X2==0)&&TE->Cx<(SINT) strlen(TE->Riga2)) SEL_Y2--;
		 if (SEL_X2==0) SEL_X2=strlen(TE->Riga2);
		 SEL_X2-=TE->Cx;
		}

	if (dove>SEL_Y2) return;
	if (dove<SEL_Y1) {SEL_Y1++;SEL_Y2++;}
	if (dove>=SEL_Y1) {SEL_Y2++;}
//	printf("[%ld]",SEL_Y2);

	java:
	TE->SEL_Ystart=SEL_Y1; TE->SEL_Yend=SEL_Y2;
	TE->SEL_Xstart=SEL_X1; TE->SEL_Xend=SEL_X2;
}

//-----------------------------------------------
// Modificato il file la selezione si aggiorna  !
//-----------------------------------------------
static void SELCarPlus(TEDIT *TE)
{
	SINT SEL_X1,SEL_X2;
	LONG SEL_Y1,SEL_Y2;

	if (!IPT_ins) return;

	if (SELcoo(TE,&SEL_X1,&SEL_Y1,&SEL_X2,&SEL_Y2)) return;

	if ((TE->ptre==SEL_Y2)&&(TE->Cx<SEL_X2)) SEL_X2++;
	if ((TE->ptre==SEL_Y1)&&(TE->Cx<SEL_X1)) SEL_X1++;

	TE->SEL_Ystart=SEL_Y1; TE->SEL_Yend=SEL_Y2;
	TE->SEL_Xstart=SEL_X1; TE->SEL_Xend=SEL_X2;
}

//-----------------------------------------------
// Modificato il file la selezione si aggiorna  !
//-----------------------------------------------
static void SELCarMinus(TEDIT *TE)
{
	SINT SEL_X1,SEL_X2;
	LONG SEL_Y1,SEL_Y2;

	if (SELcoo(TE,&SEL_X1,&SEL_Y1,&SEL_X2,&SEL_Y2)) return;

	if ((TE->ptre==SEL_Y2)&&(TE->Cx<SEL_X2)) SEL_X2--;
	if ((TE->ptre==SEL_Y1)&&(TE->Cx<SEL_X1)) SEL_X1--;
	if (SEL_X2<0) SEL_X2=0;

	if ((SEL_Y1==SEL_Y2)&&(SEL_X1==SEL_X2)) {SEL_Y2=-1; SEL_Y1=-1;}

	TE->SEL_Ystart=SEL_Y1; TE->SEL_Yend=SEL_Y2;
	TE->SEL_Xstart=SEL_X1; TE->SEL_Xend=SEL_X2;
}

//------------------------------------
// Copia la selezione in memoria     !
//------------------------------------

static void SELcopia(TEDIT *TE)
{
 LONG pt;
 LONG Line;
 SINT SEL_X1,SEL_X2;
 LONG SEL_Y1,SEL_Y2;
 CHAR *p;
 TCHAR *Dest;
 CHAR *Base;
 HANDLE hgm;
 CHAR CRLF[10];
// HWND hWnd;
 if (SELcoo(TE,&SEL_X1,&SEL_Y1,&SEL_X2,&SEL_Y2)) {efx2(); return;}

 Line=SEL_Y2-SEL_Y1+1;
 hgm=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,Line*TE->SizeLine);
 Base=Dest=GlobalLock(hgm); *Dest=0; 
 memset(Base,0,Line*TE->SizeLine);
 sprintf(CRLF,"%c%c",13,10);
 
 if (OpenClipboard(TE->hWnd))
 {
  EmptyClipboard();
  for (pt=0;pt<Line;pt++)
  {
	 TE_LineRead(SEL_Y1+pt,TE);
	 p=TE->CampoLinea;

	 // Prima Linea
	 if (pt==0) p+=SEL_X1;
	 // Ultima Linea
	 if (pt==(Line-1)) *(p+SEL_X2)=0;
 
	 if (pt>0) strcat(Dest,CRLF);
	 strcat(Dest,p);
  }  
  GlobalUnlock(hgm);
  SetClipboardData(CF_TEXT,hgm);
  CloseClipboard();
 }
}

//------------------------------------
// Incolla la selezione              !
//------------------------------------

static void SELincolla(TEDIT *TE)
{
	//LONG pt;
	LONG ofs=0;
	//LONG Line;
	SINT Hlocal;
	CHAR *Buf;
	CHAR *BufTC;
	SINT start=0;
    HANDLE hmem;
	CHAR *Clip;
    CHAR *p,*ptr;
	CHAR CRLF[10];
    CHAR *szTesta=NULL;
	CHAR *szCoda=NULL;

	sprintf(CRLF,"%c%c",13,10);
	Hlocal=memo_chiedi(RAM_HEAP,TE->SizeLine*2,"Local");
	if (Hlocal<0) PRG_end("<--->");
	Buf=memo_heap(Hlocal);
	BufTC=Buf+TE->SizeLine;
	
   if (OpenClipboard(TE->hWnd))
   {
     hmem=GetClipboardData(CF_TEXT);
 	 if (hmem!=NULL)
	 {

	  Clip=GlobalLock(hmem); 
      // Determino la Testa/Coda della linea
	  TE_LineRead(TE->ptre,TE);
      strcpy(BufTC,TE->CampoLinea);
      szTesta=BufTC;
	  szCoda=BufTC+TE->Cx;
	  strins(szCoda," "); *szCoda=0; szCoda++;

      // Loop di inserimento

	  for (ptr=Clip;;)
	  {
	   p=strstr(ptr,CRLF); if (p) *p=0; 
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
	   TE_drw(TE,start,-1); 
	   TE_LineWrite(TE->ptre,Buf,TE);
	   TE->ptre++;
	   if (p) TE_LineInsert(TE->ptre,1,TE);
	   if (!p) break; else ptr=p+2;
	  }
	  
	  GlobalUnlock(hmem);
	 }
   
   }

 memo_libera(Hlocal,"<-->");
 TE->refre=ON;
 start=(SINT) (TE->ptre-TE->Y_offset); if (start<0) start=0;
 TE_drw(TE,start,-1);
}

//------------------------------------
// Cancella la Selezione             !
//------------------------------------

static void SELdelete(TEDIT *TE)
{
	LONG ofs=0;
	LONG Line;

	SINT SEL_X1,SEL_X2;
	LONG SEL_Y1,SEL_Y2;

	if (SELcoo(TE,&SEL_X1,&SEL_Y1,&SEL_X2,&SEL_Y2)) {efx2(); return;}

	Line=SEL_Y2-SEL_Y1+1;

//	printf("Linee %ld",Line);

	// -------------------------
	// Inizio Troncato         !
	// -------------------------

	if ((SEL_X1>0)||(SEL_Y1==SEL_Y2))
		 {
			TE_LineRead(SEL_Y1,TE);
			strcpy(TE->Riga2,TE->CampoLinea);
			*(TE->CampoLinea+SEL_X1)=0;
			if (SEL_Y1==SEL_Y2) strcat(TE->CampoLinea,TE->Riga2+SEL_X2);
			TE_LineWrite(SEL_Y1,TE->CampoLinea,TE);
			ofs++;
		 }

	// ----------------------------
	//  Corpo Centrale            !
	// ----------------------------

	//if (!SEL_X2&&(Line>0))
	Line--;

//	printf("Chiudo : %ld (ofs:%ld)\n",Line-ofs,ofs);
//	getch();

	if ((Line-ofs)>0)
	 {
		TE_LineDelete(SEL_Y1+ofs,(Line-ofs),TE);
	 }

 // ----------------------------
 //  CODA SE C'E'              !
 // ----------------------------

	 //if (SEL_X2&&(Line>0))
	 //	{

	 if ((Line-ofs)>-1)
	 {
		if (SEL_X1>0)
		 {
			TE_LineRead(SEL_Y1,TE);
			strcpy(TE->Riga2,TE->CampoLinea);
			TE_LineRead(SEL_Y2-(Line-ofs),TE);
			strcat(TE->Riga2,TE->CampoLinea+SEL_X2);
			TE_LineWrite(SEL_Y1,TE->Riga2,TE);
			TE_LineDelete(SEL_Y2-(Line-ofs),1,TE);
		 }
		 else
		 {
			TE_LineRead(SEL_Y2-(Line-ofs),TE);
			strcat(TE->Riga2,TE->CampoLinea+SEL_X2);
			TE_LineWrite(SEL_Y2-(Line-ofs),TE->Riga2,TE);
		 }
	 }

	 //	}

	 TE->SEL_Xstart=-1; TE->SEL_Xend=-1;
	 TE->SEL_Ystart=-1; TE->SEL_Yend=-1;


	 TE->refre=ON; TE_drw(TE,0,-1);
	 TextEditor("CURSOR",SEL_Y1,&SEL_X1);
}

//   +-------------------------------------------+
//   | TextEditor Gestione di editor testi       |
//   |            Edita un testo tipo word       |
//   |                                           |
//   | comandi                                   |
//   |                                           |
//   | APRI : predefinire TEDIT struct           |
//   |                                           |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1997 |
//   +-------------------------------------------+

SINT TextEditor(CHAR *cmd,LONG dato,void *ptr)
{
 static TEDIT *TE=NULL;

 // --------------------------------------------------------------------
 //                                                                    !
 // APRI                                                               !
 //                                                                    !
 // -------------------------------------------------------------------!

 if (!strcmp(cmd,"APRI"))
	{//SINT a;
     WNDCLASSEX wc;

     TE=(TEDIT *) ptr;
	 LastTE=TE;
	 TE->Px2=TE->Px+TE->Lx-1;
	 TE->Py2=TE->Py+TE->Ly-1;
//	 TE->Falt=font_altf(TE->Font,TE->Nfi);
	 //font_find(TE->Font,&a,&TE->FontHdl);

	 TE->Cx=0;TE->Cy=0;

	 // Registro la classe delle OW_SCR
	 wc.cbSize        = sizeof(wc);
	 wc.style         = CS_NOCLOSE;
	 wc.lpfnWndProc   = TEDITProcedure;
	 wc.cbClsExtra    = 0;
	 wc.cbWndExtra    = 0;
	 wc.hInstance     = sys.EhWinInstance;
	 wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
	 wc.hCursor       = NULL;//LoadCursor(NULL,IDC_ARROW);
	 wc.hbrBackground = NULL;//(HBRUSH) GetStockObject(WHITE_BRUSH);
	 wc.lpszMenuName  = NULL;///LoadMenu;//szAppName;
	 wc.lpszClassName = "EHTEDIT";
	 wc.hIconSm       = NULL;//LoadIcon(NULL,IDI_APPLICATION);
	 RegisterClassEx(&wc);
	 //win_infoarg("%d,%d - %d,%d",TE->Px+relwx,TE->Py+relwy,TE->Lx,TE->Ly);
	 TE->hWnd=CreateWindow("EHTEDIT", 
						   "",
                           WS_BORDER|WS_CHILD|WS_VSCROLL,
						   TE->Px+relwx,TE->Py+relwy,
						   TE->Lx,TE->Ly,
						   WIN_info[sys.WinWriteFocus].hWnd,
						   (HMENU) 1000,
						   sys.EhWinInstance,
						   NULL);

	 //boxp(TE->Px,TE->Py,TE->Px2,TE->Py2,TE->Col2,SET);
	 TE->Falt=Wfont_alt(&TE->LogFont);
	 Wcursor(TE->hWnd);
	 cursor_graph(3,TE->Falt,TE->ColCur,2);
	 
	 // Riserva la memoria
	 if (TE->MaxLines<10) PRG_end("TEerr1");
	 TE->Hdl=memo_chiedi(RAM_AUTO,(LONG) TE->MaxLines*TE->SizeLine,"TEditor");
//	 TE->Hdl=memo_chiedi(RAM_HEAP,(LONG) TE->MaxLines*TE->SizeLine,"TEditor");
//	 TE->HdlSEL=-1;
     //TE->SelLine=0;
	 //TE->SelX1=0;
	 //TE->SelX2=0;

	 // Buffer di linea
	 TE->RigaHdl=memo_chiedi(RAM_HEAP,TE->SizeLine*2,"TE_Buflinea");
	 if (TE->RigaHdl<0) PRG_end("TErr2");
	 TE->CampoLinea=memo_heap(TE->RigaHdl);
	 TE->Riga2=TE->CampoLinea+TE->SizeLine;

	 if (TE->Hdl<0) PRG_end("TE:APRI");
	 TE->ncam=((TE->Ly-1)/TE->Falt)+1;
	 TE->ncamE=(TE->Ly)/TE->Falt;

	 TE->Y_offset=0;
	 TE->Y_koffset=-1;
	 TE->X_offset=0;
	 TE->X_koffset=-1;
	 TE->refre=ON;

	 TE->SEL_Xstart=-1;
	 TE->SEL_Ystart=-1;
	 TE->SEL_Xend=-1;
	 TE->SEL_Yend=-1;
     
	 UpdateWindow(TE->hWnd);
     ShowWindow(TE->hWnd,SW_SHOW);
	 return 0;
	}

 if (TE==NULL) PRG_end("TE = NULL");
 //if (!strcmp(TE->Test,"E' APERTO!")) PRG_end("TextEditor da aprire");

 // --------------------------------------------------------------------
 //                                                                    !
 // CHIUDI                                                             !
 //                                                                    !
 // -------------------------------------------------------------------!

 if (!strcmp(cmd,"CHIUDI"))
			{if (TE->Hdl!=-1) memo_libera(TE->Hdl,"TE_CLOSE");
			 if (TE->RigaHdl!=-1) memo_libera(TE->RigaHdl,"RG");
//			 if (TE->HdlSEL>-1) memo_libera(TE->HdlSEL,"SELECT");
		     if (TE->hWnd) DestroyWindow(TE->hWnd);
			 cursor_off();
			 Wcursor(TE->hWnd);
			 return 0;
			}

 // --------------------------------------------------------------------
 //                                                                    !
 // NUOVO                                                              !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"NUOVO"))
		 {
			LONG cnt;
			CHAR zz[20];
			memset(zz,0,20);

			TE->Touch=OFF;
			TE->Lines=1;
			TE->Cx=0;TE->Cy=0;
			// Abblancko
			mouse_graph(0,0,"CLEX");
			for (cnt=0;cnt<TE->MaxLines;cnt++)
					{
					//TE_LineWrite(cnt," ",TE);
					 memo_scrivivar(TE->Hdl,cnt*TE->SizeLine,zz,sizeof(zz));
					}
			mouse_graph(0,0,"MS01");
			goto RESETTA;
			//TE->refre=ON;// da sostituire con zona
			//TE_drw(TE,-1,-1);
		 }

 // --------------------------------------------------------------------
 //                                                                    !
 // LOAD                                                               !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"LOAD"))
		 {

			FILE *pf1;
			LONG a;
			SINT Flag;

			// ------------------------------
			// Carico il file in memoria    !
			// ------------------------------

			os_errset(OFF);
			Flag=f_open(ptr,"r",&pf1); //win_errgrave("File ??");
			os_errset(POP);
			if (Flag) return -1;
			mouse_graph(0,0,"CLEX");
			fseek(pf1,0,SEEK_SET);
			for (a=0;;a++)
			{ CHAR *p;
				if (fgets(TE->Riga2,TE->SizeLine,pf1)==NULL) break;
				p=TE->Riga2;
				for (;*p!=0;p++) {if ((*p==13)||(*p==10)) *p=0;}
				memo_scrivivar(TE->Hdl,a*TE->SizeLine,TE->Riga2,TE->SizeLine);
			}
			//EOF

			f_close(pf1);
			mouse_graph(0,0,"MS01");

			strcpy(TE->FileName,ptr);
			TE->Lines=a;

			RESETTA:
			TE->Touch=OFF;
			TE->Y_offset=0;
			TE->X_offset=0;
			TE->refre=ON;
			TE->Cx=0;TE->Cy=0;

			TE->SEL_Xstart=-1;
			TE->SEL_Ystart=-1;
			TE->SEL_Xend=-1;
			TE->SEL_Yend=-1;

			TE_drw(TE,-1,-1);

		 }

 // --------------------------------------------------------------------
 //                                                                    !
 // SAVE                                                               !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"SAVE"))
		 {

			FILE *pf1;
			LONG a;

			// ------------------------------
			// Salvo il file in memoria    !
			// ------------------------------

			mouse_graph(0,0,"CLEX");
			//printf(ptr); getch();
			if (f_open(ptr,"w",&pf1)) win_errgrave("File ??");


			//fseek(pf1,0,SEEK_SET);
			for (a=0;a<TE->Lines;a++)
			{ //CHAR *p;
				TE_LineRead(a,TE);
				fprintf(pf1,"%s\n",TE->CampoLinea);
			}
			f_close(pf1);
			mouse_graph(0,0,"MS01");

			strcpy(TE->FileName,ptr);
			TE->Touch=OFF;
		 }


 // --------------------------------------------------------------------
 //                                                                    !
 // EDIT                                                               !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"EDIT"))
			{
			 SINT tipo;
			 CHAR *CampoLinea;
			 SINT LenCL;
			 SINT x;
			 SINT Rx;
			 SINT SuperFlag;

			 //		 for(;;)
	//		 {
				// Copia la riga in uso nell'heap
				//RigaHdl=memo_chiedi(RAM_HEAP,TE->SizeLine,"RG");
				//if (RigaHdl<0) PRG_end("RG");
				//CampoLinea=memo_heap(RigaHdl);

             SelVedi(TE);

			 DARICAPO:
				CampoLinea=TE->CampoLinea;
				TE->ptre=TE->Y_offset+TE->Cy;
				if (TE->ptre<TE->Lines)
				 {memo_leggivar(TE->Hdl,TE->ptre*TE->SizeLine,CampoLinea,TE->SizeLine);
					}
					else
					{*CampoLinea=0;}

				LenCL=strlen(CampoLinea);

				if (TE->Cx>(SINT)strlen(CampoLinea)) TE->Cx=strlen(CampoLinea);
				//x=font_dim(CampoLinea,TE->Cx,TE->FontHdl,TE->Nfi);
//				x=WFontDimWnd(CampoLinea,TE->Cx,TE->hWnd,&TE->LogFont);
				x=Wfont_dim(CampoLinea,TE->Cx,&TE->LogFont);

				Rx=TE->Px+x-(SINT) TE->X_offset;

				// Controllo spostamento orizzontale
				if (Rx>TE->Px2)
							{
							 do {
							 TE->X_offset+=(TE->Lx/3);
							 Rx=TE->Px+x-(SINT) TE->X_offset;
							 } while (Rx>TE->Px2);

							 TE->refre=ON;
							 TE_drw(TE, 0,-1);
							 goto DARICAPO;
							}

				if (Rx<TE->Px)
							{
							 do {
							 TE->X_offset-=(TE->Lx/5);
							 if (TE->X_offset<0) TE->X_offset=0;
							 Rx=TE->Px+x-(SINT) TE->X_offset;
							 } while (Rx<TE->Px);

							 TE->refre=ON;
							 TE_drw(TE, 0,-1);
							 goto DARICAPO;
							 //return 0;
							}
				//Rx=TE->Px+x-TE->X_offset;
				Acursor_xy(Rx-TE->Px+MARGINDX,TE->Cy*TE->Falt);
				if (!sys.txc_vedi) cursor_on();

				gigimarzullo:
				tipo=input_wait();
				if ((tipo==IN_KEY)&&(!keybuf[0])&&(!keybuf[1])) goto gigimarzullo;
				if ((tipo==IN_OBJ)&&(!OBJ_name[0])) goto gigimarzullo;

	//	+-----------------------------------------+
	//	| GESTIONE DEL MOUSE                 		  |
	//	+-----------------------------------------+
			 if (tipo==IN_SX)
				 {
					SINT x,y,x2,y2;
					SINT cpy;
					SINT dif,a,crs,b,lenr;

					// Trovo le coordinate assolute
					x=TE->Px+relwx;
					y=TE->Py+relwy;
					x2=TE->Px+TE->Lx-1+relwx;
					y2=TE->Py+TE->Ly-1+relwy;
					if ((sys.ms_y<y)||(sys.ms_y>y2)) goto USCITA;
					if ((sys.ms_x<x)||(sys.ms_x>x2)) goto USCITA;

					// Controllo se sono sull'input
					//Aboxp(x,y,x2,y2,15,XOR);

					// Settaggio della Y
					cpy=sys.ms_y-y; cpy/=TE->Falt;
					if ((cpy+TE->Y_offset)<TE->Lines) TE->Cy=cpy;


					// Settaggio della X
					TE->ptre=TE->Y_offset+TE->Cy;
					CampoLinea=TE->CampoLinea;
					memo_leggivar(TE->Hdl,
												TE->ptre*TE->SizeLine,
												CampoLinea,
												TE->SizeLine);
					dif=30000;
					x2=sys.ms_x-x;
					x=-1;
					LenCL=strlen(CampoLinea); // Lunghezza in caratteri dell'input


					for (a=0;a<(LenCL+1);a++)
					 {
						//crs=font_dim(CampoLinea,a,TE->FontHdl,TE->Nfi);
//						crs=WFontDimWnd(CampoLinea,a,TE->hWnd,&TE->LogFont);
						crs=Wfont_dim(CampoLinea,a,&TE->LogFont);

						//b=(mx-px)+TE->X_offset;
						b=x2+TE->X_offset;
						if (crs>b) lenr=crs-b; else lenr=b-crs;

						//sprintf(ss,"crs=%3d b=%3d lenr=%3d x=%3d",crs,b,lenr,x);
						//Adispm(0,20,0,14,ON,SET,ss); getch();

						if (lenr<dif) {x=a;dif=lenr;}
					 //x=((mx-px)+((CHR_O-1)/2))/CHR_O; if (x>(lenx-1)) x=lenx-1;
					 }

					 if (x==-1) {x=x2; efxtext();}
					 TE->Cx=x;

					goto USCITA;
					}
	//	+-----------------------------------------+
	//	|         GESTIONE TASTI A 2 BYTES			  |
	//	+-----------------------------------------+

			if (tipo!=IN_KEY) goto USCITA;

			if (keybuf[0]==0) {

			if (strstr("KMPHQIGO",keybuf+1)!=NULL) SELstart(TE);

			control:
			switch (keybuf[1]) {

									case  0 : SELCarPlus(TE);
														TE->Cx++; // Carattere in inserito in pi—
														if (TE->Cx>TE->SizeLine)
															 {TE->Cx=TE->SizeLine;
																Tonk();
																}
														break;

									case _FDX: //					FRECCIA DESTRA

														// Controllo della selezione

														TE->Cx++;

														// Va a capo
														if (TE->Cx>LenCL)
															{
															 TE->ptre++; if (TE->ptre>=TE->Lines) {Tonk();break;}
															 TE->Cx=0; TE->Cy++;
															}

														break;

									case _FSX: // 					FRECCIA SINISTRA
														TE->Cx--;
														if (TE->Cx<0) {TE->Cx=0;goto CASO1;}
														break;



									case _FDN: //					FRECCIA GIU'
														//SELstart(TE);
														if (TE->ptre>=TE->Lines)
															 sonic(5000,1,1,1,1,5);
															 else
															 {
																TE->Cy++;
															 }

														break;

									case _FUP: //					FRECCIA SU'
														if (TE->ptre==0)
															 sonic(5000,1,1,1,1,5);
															 else
															 TE->Cy--;
														break;

									case 'Q': //					PageDown
														if (TE->ptre>=(TE->Lines-1))
															 sonic(5000,1,1,1,1,5);
															 else
															 TE->Cy+=TE->ncamE;
														break;


									case 'I': //					PageUP
														if (TE->ptre==0)
															 sonic(5000,1,1,1,1,5);
															 else
															 TE->Cy-=TE->ncamE;
														break;

														//					DELETE
									case _CANC:
													    
														if (Shift())
															 {//win_time("TAGLIA",1|NOWIN);
																SELcopia(TE);
																SELdelete(TE);
																break;
															 }
														if (key_pressS(_SHIFTDX|_SHIFTSX|_CTRL|_ALT,OR)) 
														{if (TE->SEL_Ystart!=-1)
															{
																SELdelete(TE);
																break;
															}
														}

														if (TE->Cx<LenCL)
															 // Cancellazione semplice
															 {SINT z;
																z=TE->Cx;
																for(;*(CampoLinea+z);) *(CampoLinea+z++)=*(CampoLinea+z+1);
																SELCarMinus(TE);
																TE_LineRefresh(TE->ptre,CampoLinea,TE);
															 }
															 else
															 // Aggiungi riga in coda
															 {SINT Size2,TotLen;
																ADDLINE:

																if (TE->ptre>=TE->Lines) {Tonk();break;}
																memo_leggivar(TE->Hdl,(TE->ptre+1)*TE->SizeLine,CampoLinea,TE->SizeLine);
																Size2=strlen(CampoLinea);
																TotLen=TE->Cx+Size2;
																//if (TE->ptre!=(TE->Y_offset+TE->Cy)) efx3();

																if (TotLen>(TE->SizeLine-1))
																	{
																	 win_info("ADDIZIONE LINEA IMPOSSIBILE:\nLinea troppo lunga");
																	 break;
																	}

																memo_leggivar(TE->Hdl,
																							TE->ptre*TE->SizeLine,
																							CampoLinea,
																							TE->SizeLine);

																memo_leggivar(TE->Hdl,
																							(TE->ptre+1)*TE->SizeLine,
																							CampoLinea+TE->Cx,
																							Size2+1);

																TE_LineWrite(TE->ptre,CampoLinea,TE);

																TE_LineDelete(TE->ptre+1,1,TE);
																SELLineMinus(TE,TE->Cy,OFF);
																TE->refre=ON;
																TE_drw(TE,TE->Cy,-1);
															 }
														break;

														//					INSERT
									case 'R':
														// Control+INS
														if (key_pressS(_CTRL,OR)) SELcopia(TE);

														if (Shift())
															 {//win_time("INCOLLA",1|NOWIN);
																SELincolla(TE);
																break;
															 }

														if (IPT_ins) IPT_ins=OFF; else IPT_ins=ON;
														break;

														//					CONTROL + HOME
									case 'w': break;
														//					CONTROL + END
									case 'u': break;    

														//					HOME
									case 'G':
														TE->Cx=0;
														break;
														//					END
									case 'O':
														TE->Cx=strlen(CampoLinea);
														break;

														//					CONTROL + -->
									case 't':
														if (TE->Cx>=LenCL)
															 {TE->ptre++; if (TE->ptre>=(TE->Lines-1)) {Tonk();break;}
																memo_leggivar(TE->Hdl,(TE->ptre+1)*TE->SizeLine,CampoLinea,TE->SizeLine);
																LenCL=strlen(CampoLinea);
																TE->Cx=0; TE->Cy++;
															 }
														TE->Cx+=strcspn(CampoLinea+TE->Cx," ");
														// Cerca il non spazio
														for (;TE->Cx<LenCL;TE->Cx++)
																{if (*(CampoLinea+TE->Cx)>' ') break;}
														break;

														//					CONTROL + <--
									case 's':
														if (TE->Cx==0)
															 {CASO1:
																if (TE->ptre==0) {Tonk();break;}
																TE->ptre--;
																memo_leggivar(TE->Hdl,TE->ptre*TE->SizeLine,CampoLinea,TE->SizeLine);
																LenCL=strlen(CampoLinea);
																//TE->Cx=0; TE->Cy++;
																TE->Cx=strlen(CampoLinea);
																TE->Cy--;
																break;
															 }
														// Cerca il non spazio

														if (TE->Cx>0) TE->Cx--;
														for (;TE->Cx>-1;TE->Cx--) if (*(CampoLinea+TE->Cx)>' ') break;
														// Cerca lo spazio
														for (;TE->Cx>-1;TE->Cx--) if (*(CampoLinea+TE->Cx)==' ') break;
														// Cerca il non spazio
														for (;TE->Cx<(SINT) strlen(CampoLinea);TE->Cx++) if (*(CampoLinea+TE->Cx)>' ') break;

														if (TE->Cx<0) TE->Cx=0;
														break;

														//	CONTROL + Ins (Copia)

									//default : printf("%c (%d)",keybuf[1],(SINT) keybuf[1]);getch();break;
								};

							 goto USCITA;};

//	+-----------------------------------------+
//	|         GESTIONE TASTO SEMPLICE 				|
//	+-----------------------------------------+
	/*
				if ((keybuf[0]==9)||(keybuf[0]==13))
					{
					 if (ipt_ctrlcampo(iptnum)) {efx1();continue;}
					 //pin->ofs_x=0;
					 }
		*/
			//sprintf(ss,"%3d",(SINT) *keybuf);
			//disp(20,20,15,1,ss);
                
				switch(keybuf[0])
				{

				case 3: // Control + C  (Copia)
				        SELcopia(TE);   
					    break;

				case 22: // Control + V (Incolla)
					    SELincolla(TE);
						break;

				case 11 : mouse_graph(0,3,"MS05");
									input_wait();

									// ----------------------
									// Toglie la selezione  !
									// ----------------------
									if (key_press('H')||key_press('h')&&(TE->SEL_Yend!=-1))
										 {
											 SELRES:
											 TE->SEL_Xstart=-1;
											 TE->SEL_Ystart=-1;
											 TE->SEL_Xend=-1;
											 TE->SEL_Yend=-1;

											 rinfresca:
											 TE->refre=ON; TE_drw(TE, 0,-1);
											 goto OKVABENE;
										 }

									// -----------------------
									// Cancella la Selezione !
									// -----------------------
									if (key_press('Y')||key_press('y')&&(TE->SEL_Yend!=-1))
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
											 TE->SEL_Xstart=TE->Cx;
											 TE->SEL_Ystart=TE->ptre;
											 if (TE->SEL_Yend!=-1) goto rinfresca;
										 }

									// ----------------
									// Marca la fine  !
									// ----------------
									if (key_press('K')||key_press('k'))
										 {
											 TE->SEL_Xend=TE->Cx;
											 TE->SEL_Yend=TE->ptre;
											 if (TE->SEL_Ystart==-1) goto SEL1;
											 goto rinfresca;
										 }

									OKVABENE:
									mouse_graph(0,0,"MS01");
									goto USCITA;

				//	Ctrl+Y cancella il campo o una riga
				case  25 : if (TE->ptre>=TE->Lines) {Tonk();break;}
									 TE->Cx=0;
//									 TE_LineDelete(TE->Cy,1,TE);
									 TE_LineDelete(TE->ptre,1,TE);
									 SELLineMinus(TE,TE->Cy,ON);
									 TE->refre=ON;
									 TE_drw(TE,TE->Cy,TE->ncamE);
									 break;

				//	Carrage Return
				case  CR :
									 // Inserisce una nuova linea
									 //TE_LineInsert(TE->Cy+1,TE);
									 //TE_LineWrite(TE->Cy+1,CampoLinea+TE->Cx,TE);
									 //*(CampoLinea+TE->Cx)=0;
									 //TE_LineWrite(TE->Cy,CampoLinea,TE);

									 TE_LineInsert(TE->ptre+1,1,TE);
									 SELLinePlus(TE,TE->ptre);

									 TE_LineWrite(TE->ptre+1,CampoLinea+TE->Cx,TE);
									 *(CampoLinea+TE->Cx)=0;
									 TE_LineWrite(TE->ptre,CampoLinea,TE);

									 // Ristampa tutto
									 TE->refre=ON;
									 TE_drw(TE,TE->Cy,TE->ncamE);
									 TE->Cx=0;TE->Cy++;
									 goto USCITA;

				//	Backspace
				case  8	 : if ((TE->Cx<=LenCL)&&(LenCL!=0))
									 {SINT z;

										if (TE->Cx>0)
											{TE->Cx--;
											 //if (TE->Cx<0)
											 //{//a=ipt_noteriga(cr3,CampoLinea,0,(y-1));
												//if (a==-1) x=0; else {x=strlen(cr3);y--;}
												// Attacco la linea di sotto alla riga di sopra
											 //}
											 //else
											 //{
												FAINORM:
												z=TE->Cx;
												for(;*(CampoLinea+z);) *(CampoLinea+z++)=*(CampoLinea+z+1);
												SELCarMinus(TE);
												TE_LineRefresh(TE->ptre,CampoLinea,TE);
											 }
											 else
											 {// Porto la riga su
												if (TE->ptre==0) {TE->Cx=0;goto FAINORM;}
												TE->ptre--;
												memo_leggivar(TE->Hdl,TE->ptre*TE->SizeLine,CampoLinea,TE->SizeLine);
												LenCL=strlen(TE->CampoLinea);
												TE->Cx=LenCL;
												TE->Cy--;
												goto ADDLINE;
											 }

											//}
									 }



									 goto USCITA;


				}

//	+-----------------------------------------+
//	|         INSERIMENTO DI UN CARATTERE    	|
//	+-----------------------------------------+
		if ((keybuf[0]<' ')&&(keybuf[0]!=CR)) //continue; // Carattere non valido
				{//rt=keybuf[0];
				 //RTipt=IN_KEY;
				 goto USCITA;}

		//	Modalit… INSERT
		if (TE->ptre>=TE->Lines) {*CampoLinea=0;TE->Lines++;}

		//if ((y==pin->nrig)&&(keybuf[0]!=CR)&&(strlen(cr3)==0)) pin->nrig++;

//		if ((IPT_ins)&&(TE->Cx<(strlen(CampoLinea))) )
		if (IPT_ins)
		{if ((SINT) strlen(CampoLinea)<TE->SizeLine)
		 {SINT a;

			//printf("TE->ptre %ld %s",TE->ptre,CampoLinea);
			for (a=LenCL+1;a>TE->Cx;a--) *(CampoLinea+a)=*(CampoLinea+a-1);
			*(CampoLinea+TE->Cx)=keybuf[0];

				//		Inserimento di un CR
				/*
				if (keybuf[0]==CR)
					{x=0;y++; pin->nrig++;continue;}
					*/

				//TE_LineWrite(TE->ptre,CampoLinea,TE);
				//TE->refre=ON;// da sostituire con zona linea
				//TE_drw(TE,TE->Cy,TE->Cy);
				TE_LineRefresh(TE->ptre,CampoLinea,TE);
				keybuf[1]=0; goto control;

		 }
		 else
		 {Tonk(); goto USCITA;}

		}
		//	Modalit… REWRITE
		if (TE->Cx<TE->SizeLine)
			 {
				*(CampoLinea+TE->Cx)=keybuf[0];
				if ((TE->Cx+1)>LenCL) *(CampoLinea+TE->Cx+1)=0;
				//		Inserimento di un CR
				/*
				if (keybuf[0]==CR)
						{x=0;y++; pin->nrig++;
						continue;}
					*/
				//TE_LineWrite(TE->ptre,CampoLinea,TE);
				//TE->refre=ON;// da sostituire con zona linea
				//TE_drw(TE,TE->Cy,TE->Cy);
				TE_LineRefresh(TE->ptre,CampoLinea,TE);
				keybuf[1]=0; goto control;
			 }
			 else
		//		Segnala la fine di riga
			 Tonk();

	 USCITA:

	 // ----------------------------
	 // POSTO DI CONTROLLO SCROLL  !
	 // ----------------------------
	 SuperFlag=OFF;
	 // Limite inferiore
	 if (TE->Cy>(TE->ncamE-1))
			{SINT spost;
			 spost=(TE->Cy-(TE->ncamE-1));
			 TE->Y_offset+=spost;
			 if (TE->Y_offset>(TE->Lines-TE->ncamE)) TE->Y_offset=TE->Lines-TE->ncamE;
			 if (TE->Y_offset<0) TE->Y_offset=0;

			 TE->Cy=(TE->ncamE-1);
			 if (TE->Cy>TE->Lines) TE->Cy=(SINT) TE->Lines;
			 if (spost>1) TE->refre=ON;
       SuperFlag=ON;
//			 TE_drw(TE,-1,-1);
			}

	 // Limite superiore OK
	 if (TE->Cy<0)
			 {if (TE->Y_offset!=0)
				{SINT spost;
				 spost=-TE->Cy;
				 TE->Y_offset-=spost;
				 if (TE->Y_offset<0) TE->Y_offset=0;
				 TE->Cy=0;
				 if (spost>1) TE->refre=ON;
				 SuperFlag=ON;
	//			 TE_drw(TE,-1,-1);
				}
				else TE->Cy=0;

			 }

	 TE->ptre=TE->Y_offset+TE->Cy;

	 if ((!keybuf[0])&&(keybuf[1])&&(strstr("QIts",keybuf+1)!=NULL)) {SELend(TE); if (Shift()) TE->refre=ON;}
	 if (SuperFlag) TE_drw(TE, 0,-1);
	 if ((!keybuf[0])&&(keybuf[1])&&(strstr("KMPHGO",keybuf+1)!=NULL)) SELend(TE);

	 return tipo;
 } // End if EDIT

 // --------------------------------------------------------------------
 //                                                                    !
 // CURSOR                                                             !
 //                                                                    !
 // -------------------------------------------------------------------!

 if (!strcmp(cmd,"CURSOR"))
	 {SINT *px;

		px=(SINT *) ptr;
		rifo:
		TE->Cy=(SINT) (dato-TE->Y_offset);
		if ((TE->Cy<0)||(TE->Cy>=TE->ncamE))
			 {TE->Y_offset=dato-(TE->ncamE/2);
				if (TE->Y_offset<0) TE->Y_offset=0;
				TE->refre=ON;
				goto rifo;
			 }

		//if (TE->Lines<=TE->ncamE) TE->Y_offset=0;
		//if (TE->refre) TextEditor("REDRAW",0,0);
		TE_drw(TE,0,-1);
		TE->Cx=*px;
		TE->ptre=TE->Y_offset+TE->Cy;

	 }


 // --------------------------------------------------------------------
 //                                                                    !
 // DISP refresh di una linea                                          !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"DISP"))
			{
				SINT rigan;

				rigan=(SINT) (dato-TE->Y_offset);
				if ((rigan<0)||(rigan>TE->ncam)) return 0;
				TE->refre=ON;
				TE_drw(TE,rigan,rigan);
			}

 // --------------------------------------------------------------------
 //                                                                    !
 // REDRAW ridisegna tutta la finestra                                 !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"REDRAW"))
			{
				//SINT a;
				SINT Color;
				TE->Px2=TE->Px+TE->Lx-1;
				TE->Py2=TE->Py+TE->Ly-1;
				TE->refre=ON;// da sostituire con zona
				//TE->Falt=TE->LogFont.lfHeight;
				TE->Falt=Wfont_alt(&TE->LogFont);
				//TE->Falt=font_altf(TE->Font,TE->Nfi);
				//font_find(TE->Font,&a,&TE->FontHdl);
				//Wcursor(TE->hWnd);
				Color=ModeColor(TRUE);
				cursor_graph(3,TE->Falt,TE->ColCur,2);
				ModeColor(Color);
				TE->ncam=((TE->Ly-1)/TE->Falt)+1;
				TE->ncamE=(TE->Ly)/TE->Falt;
				MoveWindow(TE->hWnd,
					       TE->Px+relwx,TE->Py+relwy,
						   TE->Lx-1,TE->Ly-1,TRUE);
				

				TE_drw(TE,(SINT) dato,-1);
                /*
				InvalidateRect(TE->hWnd,NULL,FALSE);
                UpdateWindow(TE->hWnd);
                ShowWindow(TE->hWnd,SW_SHOW);
				*/
			}

 // --------------------------------------------------------------------
 //                                                                    !
 // XCONTROL Controlla che il cursore sia nella finestra               !
 //          se no la sposta                                           !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"XCONTROL"))
			{
				CHAR *CampoLinea;
				SINT x;
			  SINT Rx;

				DACAPO2:
				CampoLinea=TE->CampoLinea;
				TE->ptre=TE->Y_offset+TE->Cy;
				memo_leggivar(TE->Hdl,TE->ptre*TE->SizeLine,CampoLinea,TE->SizeLine);

				if (TE->Cx>(SINT) strlen(CampoLinea)) TE->Cx=strlen(CampoLinea);
				//x=font_dim(CampoLinea,TE->Cx,TE->FontHdl,TE->Nfi);
//                x=WFontDimWnd(CampoLinea,TE->Cx,TE->hWnd,&TE->LogFont);
                x=Wfont_dim(CampoLinea,TE->Cx,&TE->LogFont);

				Rx=TE->Px+x-(SINT) TE->X_offset;

				// Controllo spostamento orizzontale
				if (Rx>(TE->Px2-(TE->Lx/5)))
							{
							 do {
							 TE->X_offset+=(TE->Lx/3);
							 Rx=TE->Px+x-(SINT) TE->X_offset;
							 } while (Rx>TE->Px2);

							 TE->refre=ON;
							 TE_drw(TE, 0,-1);
							 goto DACAPO2;
							}

				if (Rx<TE->Px)
							{
							 do {
							 TE->X_offset-=(TE->Lx/3);
							 if (TE->X_offset<0) TE->X_offset=0;
							 Rx=TE->Px+x-(SINT) TE->X_offset;
							 } while (Rx<TE->Px);

							 TE->refre=ON;
							 TE_drw(TE, 0,-1);
							 goto DACAPO2;
							}
				//Rx=TE->Px+x-TE->X_offset;
				if (!sys.txc_vedi) cursor_on();
				Acursor_xy(Rx-TE->Px+MARGINDX,TE->Cy*TE->Falt);
			 }

 // --------------------------------------------------------------------
 //                                                                    !
 // COPIA Selezione nel clipboard                                      !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"COPIA")) SELcopia(TE);

 // --------------------------------------------------------------------
 //                                                                    !
 // TAGLIA Selezione nel clipboard                                     !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"TAGLIA")) {SELcopia(TE);SELdelete(TE);}

 // --------------------------------------------------------------------
 //                                                                    !
 // INCOLLA Selezione nel testo                                        !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"INCOLLA")) SELincolla(TE);

 // --------------------------------------------------------------------
 // LINEREAD Legge una linea                                           !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"LINEREAD"))
		{
			memo_leggivar(TE->Hdl,dato*TE->SizeLine,ptr,TE->SizeLine);
		}

 // --------------------------------------------------------------------
 // LINEWRITE Scrive una linea                                         !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"LINEWRITE"))
		{
		 TE_LineWrite(dato,ptr,TE);
		}

 // --------------------------------------------------------------------
 // LINEINSERT Inserisce delle linee                                   !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"LINEINSERT"))
		{
			TE_LineInsert(dato,atol(ptr),TE);
		}

 // --------------------------------------------------------------------
 // LINEAPPEND Accoda una linea al file                                !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"LINEAPPEND"))
		{
			TE_LineWrite(TE->Lines,ptr,TE);
			TE->Lines++;
		}

 // --------------------------------------------------------------------
 //                                                                    !
 // FIND Ricerca una parola                                            !
 //                                                                    !
 // -------------------------------------------------------------------!
 if (!strcmp(cmd,"FIND"))
	 {
		TEDITFIND *TEF;
		LONG Da,Finoa;
		LONG Pt;
		CHAR *p;
		SINT Flag=ON;

		TEF=(TEDITFIND *) ptr;

//		rifai:
		if (!TEF->Dove) {Da=0; Finoa=TE->Lines;}
										else
										{Da=TE->SEL_Ystart;
										 Finoa=TE->SEL_Yend;}
		if (TEF->Dal) Da=0; else Da=TE->ptre;

		if (TEF->Y!=-1) {Da=TEF->Y;TEF->X++;} else TEF->X=0;

		if (!TEF->Case) strupr(TEF->Cerco);

		for (Pt=Da;Pt<Finoa;Pt++)
		{
		 TE_LineRead(Pt,TE);

		 if (!TEF->Case) strupr(TE->CampoLinea);
		 if (TEF->X>(SINT) strlen(TE->CampoLinea)) goto avanti;

		 p=strstr(TE->CampoLinea+TEF->X,TEF->Cerco);
		 //printf("[%s]\n",TE->CampoLinea+TEF->X);
		 if (p!=NULL)
				{SINT X;
				 X=(SINT) ((LONG) p-(LONG) TE->CampoLinea);
				 TEF->X=X;
				 //printf("[%d]",X);
				 TextEditor("CURSOR",Pt,&X);
				 TEF->Y=Pt;
				 Flag=OFF;
				 break;
				}
		 avanti:
		 TEF->X=0;
		}

		if (TEF->Y==-1) {WIN_fz=OFF;
						 win_info("La stringa ricercata non esiste.");
						 WIN_fz=ON;
						 return -1;
						}

		if (Flag) {TEF->Y=-1; return -1;}

	 }

 return 0;
}
// -------------------------------------------------------------
// OWSCRProcedure                                              |
// Funzione di Controllo della Finestra Child "Tabellone"      |
//                                                             |
// -------------------------------------------------------------

static void TEDITRangeAdjust(TEDIT *TE)
{
  SCROLLINFO ScrollInfo;
  SINT MaxScroll;
//  ws->numcam=((poj->col2+poj->CharY-1)/poj->CharY);
  //ws->numcam=((poj->col2+TE->Falt-1)/TE->Falt);
  TE->ncam=((TE->Ly-1)/TE->Falt)+1;
  
  // Massimo range di scorrimento
  MaxScroll=max(0,(SINT) (TE->Lines+2-TE->ncam));
  TE->Y_offset=min(TE->Y_offset,MaxScroll);
  
  ScrollInfo.cbSize=sizeof(ScrollInfo);
  ScrollInfo.fMask=SIF_ALL;
  ScrollInfo.nPage=0;//TLayOut.Lines/((TLayOut.AreaY/TLayOut.CharY));
  ScrollInfo.nPos=TE->Y_offset;
  ScrollInfo.nMin=0;
  ScrollInfo.nMax=MaxScroll;
  SetScrollInfo(TE->hWnd,SB_VERT,&ScrollInfo,TRUE);
}


static LRESULT CALLBACK TEDITProcedure(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  HDC hdc;
  PAINTSTRUCT ps;

  SINT iVScrollInc;
  TEDIT *TE;
  //  SINT iHScrollInc;
  //  SINT a,b,c,pt,y,ptC,kx;
  //  CHAR Serv[80];

  WINSCENA Scena;
  //struct WS_INFO *ws;
  SINT MaxScroll;
  SINT a,y;//,lx;
  HRGN hrgn;
  RECT Rect;
  TE=LastTE;
  
  switch (message)
  {
     // Prima chiamata
	 case WM_CREATE:
		  break;

     case WM_SIZE:
		  break;

	 case WM_LBUTTONDOWN:
	 case WM_RBUTTONDOWN:
	 case WM_LBUTTONUP:
	 case WM_RBUTTONUP:
	 case WM_LBUTTONDBLCLK:
         WinMouseAction(hWnd,message,wParam,lParam);
		 break;

	 case WM_NCHITTEST:
		  
		  GetWindowRect(hWnd,&Rect);

		  WinMouse(TE->Px+LOWORD(lParam)-Rect.left,
			       TE->Py+HIWORD(lParam)-Rect.top, 
				   0xFFFF);

		 //if (wParam==HTCLIENT) break;
		  //win_infoarg("%d,%d", (INT) LOWORD(lParam), (INT) HIWORD(lParam));
		  //WinMouse(LOWORD(lParam),HIWORD(lParam), 0);
		 /*
		  {
					    CHAR *p="?";
						CHAR Serv[100];
					    HDC hDC;
						WINSCENA Scena;
						SINT Val=DefWindowProc(hWnd, message, wParam, lParam);

						switch (Val)
						{
                          case HTBORDER      : p="HTBorder"; break;
                          case HTBOTTOM      : p="HTBottom"; break;
                          case HTBOTTOMLEFT  : p="HTBottomL"; break;
                          case HTBOTTOMRIGHT : p="HTBottomR"; break;
                          case HTCAPTION     : p="HTCaption"; break;
                          case HTCLIENT      : p="HTClient"; break;
                          case HTCLOSE       : p="HTClose"; break;
                          case HTERROR       : p="HTErrore"; break;
                          case HTGROWBOX     : p="HTGrowBox"; break;
                          case HTHELP        : p="HTHelp"; break;
                          case HTHSCROLL     : p="HTHScroll"; break;
                          case HTLEFT        : p="HTLeft"; break;
                          case HTRIGHT       : p="HTRight"; break;
                          case HTMENU        : p="HTMenu"; break;
                          case HTMAXBUTTON   : p="HTMaxBut"; break;
                          case HTMINBUTTON   : p="HTMinBut"; break;
                          case HTNOWHERE     : p="!"; break;
                          //case HTREDUCE      : p="HTReduce"; break;
                          //case HTSIZE        : p="HTSize"; break;
                          case HTSYSMENU     : p="HTSysMenu"; break;
                          case HTTOP         : p="HTTop"; break;
                          case HTTOPLEFT     : p="HTTopLeft"; break;
                          case HTTOPRIGHT    : p="HTTopRight"; break;
                          case HTTRANSPARENT : p="HTTrasparent"; break;
                          case HTVSCROLL     : p="HTVScroll"; break;
                          //case HTZOOM        : p="HTZoom"; break;
						}
                        
						hDC=GetWindowDC(WIN_info[sys.WinWriteFocus].hWnd);
                    	WinDirectDC(hDC,&Scena);
						sprintf(Serv,"%-30.30s %4d,%4d     ",p,(INT) LOWORD(lParam), (INT) HIWORD(lParam));
						dispf(15,5,15,3,ON,"SMALL F",3,Serv);
						ReleaseDC(WIN_info[sys.WinWriteFocus].hWnd,hDC);
						WinDirectDC(0,&Scena);
						
						//if (wParam==HTCLIENT) break;
		                //
						//win_infoarg("%d,%d", (INT) LOWORD(lParam), (INT) HIWORD(lParam));
		                //WinMouse(LOWORD(lParam),HIWORD(lParam), 0);
					}
*/
					break;
     // --------------------------------------------------------------------------------
     // Controllo Scorrimento VERTICALE                                                |
     // --------------------------------------------------------------------------------

     case WM_VSCROLL:

		 //if (poj==NULL) break;
         TEDITRangeAdjust(TE);
         
		 iVScrollInc=0;
		 switch (LOWORD(wParam))
		   {
			case SB_TOP:      iVScrollInc=-iVScrollInc; break;
			case SB_BOTTOM:   iVScrollInc=TE->ncam-iVScrollInc; break;
			case SB_LINEUP:   iVScrollInc=-1; break;
			case SB_LINEDOWN: iVScrollInc=1;break;
            case SB_PAGEUP:   iVScrollInc=min(-1,-TE->ncam); break;
			case SB_PAGEDOWN: iVScrollInc=max(1,TE->ncam); break;
			case SB_THUMBTRACK: 
			case SB_THUMBPOSITION:
			  iVScrollInc=HIWORD(wParam)-TE->Y_offset;
			  break;
		   }

         MaxScroll=max(0,(SINT) (TE->Lines+2-TE->ncam));
		 iVScrollInc=max(-TE->Y_offset,
			             min(iVScrollInc,MaxScroll-TE->Y_offset));
		 /*
		 iVScrollInc=max(-TLayOut.iVScrollPos,
			             min(iVScrollInc,TLayOut.iVScrollMax-TLayOut.iVScrollPos));
*/
		 if (iVScrollInc!=0)
		   {
			cursor_off();
			ScrollWindow(TE->hWnd,0,-TE->Falt*iVScrollInc,NULL,NULL);	
			cursor_on();
			TE->Y_offset+=iVScrollInc;
			SetScrollPos(TE->hWnd,SB_VERT,TE->Y_offset,TRUE);
			UpdateWindow(TE->hWnd);
		   }
           break;

		 
	 case WM_DESTROY: break;
	 case WM_COMMAND: break;

	// ----------------------------------------------- 
	// Disegno la Tabella                            |
    // ----------------------------------------------- 

	 case WM_PAINT:
            
			//if (LockLayOut) break;
			hdc=BeginPaint(hWnd,&ps);
		    TEDITRangeAdjust(TE);

			WinDirectDC(hdc,&Scena);
             // Da ottimizzare
			//win_infoarg("%d",TE->ncam);
			y=0;
            
			hrgn=CreateRectRgnIndirect(&ps.rcPaint);
            SelectClipRgn(hdc, hrgn);
            
			for (a=0;a<TE->ncam;a++)
			{
             if ((y>ps.rcPaint.bottom)) continue;
			 TE_LineDraw(TE,hdc,y,a+TE->Y_offset);
			 y+=TE->Falt;
			}
            DeleteObject(hrgn);
            SelectClipRgn(hdc, NULL);
			WinDirectDC(0,&Scena);
			EndPaint(hWnd,&ps);
			//win_infoarg("ws.numcam=%d %d",ws->numcam,poj->CharY);
			return 0;
  }  
 return(DefWindowProc(hWnd, message, wParam, lParam));
}  
