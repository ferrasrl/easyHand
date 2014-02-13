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
/*
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <dir.h>
#include <alloc.h>

#include <Flm_main.h>
#include <FLM_VID.H>
#include <FLM_VARI.H>
  */
#include "\ehtool\include\ehsw_i.h"

#include "c:\ehtool\textedit.h"

	//CHAR   Riga[300]; // E' da togliere al pi— presto
	extern SINT IPT_ins;
	extern SINT WIN_fz;

static void TE_LineWrite(LONG RigaFile,CHAR *Rig,TEDIT *TE);
static void TE_LineRead(LONG RigaFile,TEDIT *TE);
static void TE_LineInsert(LONG RigaFile,LONG num,TEDIT *TE);
static void TE_LineDelete(LONG RigaFile,LONG num,TEDIT *TE);

void TE_drw(TEDIT *TE,SINT da,SINT a);

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
 sonic(5000,1,1,2,2,5);
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

// +-----------------------------------------+
// | TE_drw   Visualizza i dati del			     |
// |          text-windows     					     |
// |                               					 |
// |                            by Ferr… 1997|
// +-----------------------------------------+
void TE_drw(TEDIT *TE,SINT daLin, SINT aLin)
{
	SINT  x1,y1,x2,y2,cam_x,cam_y;
	SINT  spiazi,lx,ly,refre;
	LONG ptre;
	LONG koffset;
	SINT  a,ncam;
	SINT  pt_start,pt_end;
	LONG offset;
	SINT  fontalt;
	CHAR *PtRiga=TE->Riga2;

	SINT daLine,aLine;
	SINT daPulire=ON;
	LONG ColPulish;

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
	}

	fontalt=TE->Falt;//font_alt(poj->fonthdl,poj->nfi);
	refre=TE->refre;

	lx=TE->Lx; ncam=TE->ncam;
	ly=TE->Ly;

	//								Calcola posizioni finestra
	x1=TE->Px; y1=TE->Py;
	x2=TE->Px2; y2=TE->Py2; // +6
	//maxcam=TE->Lines;
 //	bar_dim=(y2-16)-(y1+16)-3;
	//bar_area=bar_dim-13;

	if ((refre==ON)&&(daLin<0))
	{//if (poj->lock==OFF) a=1; else a=15;
	 boxp(x1,y1,x2,y2,TE->Col2,SET); // Pieno bianco
	 daPulire=OFF;
	 }
	if (daLin<0) daLine=0; else daLine=daLin;
	if (aLin<0) aLine=ncam-1; else aLine=aLin;

	offset=TE->Y_offset;
	koffset=TE->Y_koffset;

	//								Calcola posizioni CAMPI e BARRA
//cam_x=x1+4; cam_y=y1+5;
	cam_x=x1; cam_y=y1;
	//bar_x=x2-13; bar_y=y1+18;

	// Effettua lo scroll necessario
	pt_start=-1; pt_end=-1;

	clip_set(cam_x,cam_y,cam_x+lx-1,cam_y+ly-1,"TEDITOR");

	if (refre!=ON)
	{
	if (offset<koffset)
	 {scroll(x1,y1,x1+lx-1,y1+ly,
					 DOWN,(SINT) (fontalt*(koffset-offset)),TE->Col2);
		pt_start=0;
		pt_end=(SINT) (koffset-offset);
	 }

	if (offset>koffset)
	 {scroll(x1,y1,x1+lx-1,y1+ly,
					 UP,(SINT) (fontalt*(offset-koffset)),TE->Col2);
		pt_end=ncam;
		pt_start=(SINT) (ncam-(offset-koffset));

		// --------------------------------------------
		// Se non Š preciso il taglio dello scroll    !
		// --------------------------------------------

		//printf("%d - %d",TE->ncam,TE->ncamE);
		//getch();
		if (TE->ncam!=TE->ncamE)
		 {   SINT emey;

				 emey=y1+(TE->ncamE-1)*TE->Falt;
				 // -------------------------------------------
				 //  TIPO CON STAMPA ESTERNA FATTA DAL DRIVER !
				 // -------------------------------------------
				 if (TE->dispext) // Stampa riga esterna
				 {
				 //if (offset!=koffset)
					 //	 boxp(cam_x,cam_y,cam_x+zz-1,cam_y+fontalt-1,15,SET);
				 boxp(cam_x,cam_y,cam_x+lx,emey+fontalt-1,TE->Col2,SET);

				 // Carica i parametri per la stampa esterna
				 //dispext.px=cam_x; dispext.py=emey; 		// Coordinate
				 //dispext.col1=TE->Col1; dispext.col2=TE->Col2;// Colori usati
				 //dispext.ncam=TE->ncamE;
				 //(TE->sub)(WS_DISPLAY,(TE->offset+TE->ncamE-1),(CHAR *) &dispext);
				 }
				 else
				 // -------------------------------------------
				 //      TIPO CON STAMPA INTERNA SOLO TESTO   !
				 // -------------------------------------------
				 {
				 //printf(">%ld-%s",ptre,Riga);
				 memo_leggivar(TE->Hdl,(TE->Y_offset+TE->ncamE-1)*TE->SizeLine,TE->Riga2,TE->SizeLine);

				 // ---------------------------
				 // Stampa con la selezione   !
				 // ---------------------------

				 if ((SEL_Y2!=-1)&&(SEL_Y1!=SEL_Y2))
					{
					 spiazi=dispfm_h(cam_x-(SINT) TE->X_offset,emey,
													 TE->Col1,TE->Col2,ON,SET,
													 TE->FontHdl,TE->Nfi,TE->Riga2);
					}
					else
					// ----------------------
					// Stampa normale       !
					// ----------------------
					spiazi=dispfm_h(cam_x-(SINT) TE->X_offset,emey,
													TE->Col1,TE->Col2,ON,SET,
													TE->FontHdl,TE->Nfi,TE->Riga2);
					if (daPulire) {boxp(cam_x+spiazi,emey,cam_x+lx,
															emey+fontalt-1,TE->Col2,SET);}
				 }
		 }
	 }
	}

	//						Visualizza i campi
	//sc=(poj->sub)(WS_BUF,0,""); //	Richiede buffer

	//if (TE->dispext)
	// {dispext.ly=fontalt; 		// Larghezza & Altezza
		//dispext.hdl=TE->fonthdl;
	//	dispext.nfi=TE->Nfi;
	//	dispext.lx=lx; dispext.lock=ON;
	// }

	ptre=offset;
	// Tapullo
	//pt_start=0; pt_end=50;
	//printf("%d",ncam);
	for (a=0;a<ncam;a++) {

		if (ptre>=TE->Lines)
				{
//				 boxp(cam_x,cam_y,cam_x+lx,cam_y+fontalt-1,TE->Col2,SET);
				 boxp(cam_x,cam_y,cam_x+lx,cam_y+fontalt-1,4,SET);
				 goto avanti;
				 }

		if (refre&&( (a<daLine)||(a>aLine) )) goto avanti;

		ColPulish=TE->Col2;
		if (((a>=pt_start)&&(a<pt_end))||(refre==ON))
				{
				 // -------------------------------------------
				 //  TIPO CON STAMPA ESTERNA FATTA DAL DRIVER !
				 // -------------------------------------------
				 if (TE->dispext) // Stampa riga esterna
				 {
				 //if (offset!=koffset)
					 //	 boxp(cam_x,cam_y,cam_x+zz-1,cam_y+fontalt-1,15,SET);

				 boxp(cam_x,cam_y,cam_x+lx,cam_y+fontalt-1,TE->Col2,SET);

				 // Carica i parametri per la stampa esterna
				 //dispext.px=cam_x; dispext.py=cam_y; 		// Coordinate
				 //dispext.col1=TE->Col1; dispext.col2=TE->Col2;// Colori usati
				 //dispext.ncam=a;
				 //(TE->sub)(WS_DISPLAY,ptre,(CHAR *) &dispext);
				 }
				 else
				 // -------------------------------------------
				 //          TIPO CON STAMPA INTERNA          !
				 // -------------------------------------------
				 {
				 memo_leggivar(TE->Hdl,ptre*TE->SizeLine,TE->Riga2,TE->SizeLine);
				 spiazi=0;
				 // ------------------------------------------------------
				 // Stampa con la SELEZIONE                              !
				 // ------------------------------------------------------

				 // Nessuna selezione
				 if ((TE->SEL_Xstart==TE->SEL_Xend)&&
						 (TE->SEL_Ystart==TE->SEL_Yend)) SEL_Y2=-1;

				 if (SEL_Y2!=-1)
					{// Tutto Selezionato
					 if ((ptre>SEL_Y1)&&(ptre<SEL_Y2))
							{
							 spiazi=dispfm_h(cam_x-(SINT) TE->X_offset,cam_y,
															 TE->Col2,TE->Col1,ON,SET,
															 TE->FontHdl,TE->Nfi,TE->Riga2);
							 spiazi=font_len(TE->Riga2,TE->FontHdl,TE->Nfi);
							 ColPulish=TE->Col1;
							 goto PULISH;
							}

					 // Tutto non selezionato
					 if ((ptre<SEL_Y1)||(ptre>SEL_Y2))
							{
							 dispfm_h(cam_x-(SINT) TE->X_offset,cam_y,
															 TE->Col1,TE->Col2,ON,SET,
															 TE->FontHdl,TE->Nfi,TE->Riga2);
							 spiazi=font_len(TE->Riga2,TE->FontHdl,TE->Nfi);
							 goto PULISH;
							}

					 if (ptre==SEL_Y1)
							 {
								CHAR bak;

								// --------------------------
								// Non selezionato          !
								// --------------------------
								if (SEL_X1>0)
								{
								 bak=*(TE->Riga2+SEL_X1);
								 *(TE->Riga2+SEL_X1)=0;

								 spiazi=font_len(PtRiga,TE->FontHdl,TE->Nfi);
								 dispfm_h(cam_x-(SINT) TE->X_offset,cam_y,
																 TE->Col1,TE->Col2,ON,SET,
																 TE->FontHdl,TE->Nfi,PtRiga);

								 *(TE->Riga2+SEL_X1)=bak;
								}

								// --------------------------
								// Selezionato              !
								// --------------------------
								if (ptre==SEL_Y2)
									 {bak=*(TE->Riga2+SEL_X2);
										*(TE->Riga2+SEL_X2)=0;
									 }

								dispfm_h(cam_x-(SINT) TE->X_offset+spiazi,cam_y,
														 TE->Col2,TE->Col1,ON,SET,
														 TE->FontHdl,TE->Nfi,PtRiga+SEL_X1);

								spiazi+=font_len(PtRiga+SEL_X1,TE->FontHdl,TE->Nfi);
								ColPulish=TE->Col1;

								// --------------------------
								// Coda Non selezionata     !
								// --------------------------

								if ((ptre==SEL_Y2)&&(SEL_X2>0))
								 {*(TE->Riga2+SEL_X2)=bak;

									dispfm_h(cam_x-(SINT) TE->X_offset+spiazi,cam_y,
														 TE->Col1,TE->Col2,ON,SET,
														 TE->FontHdl,TE->Nfi,PtRiga+SEL_X2);

									//spiazi+=font_len(PtRiga+SEL_X1,TE->FontHdl,TE->Nfi);
									spiazi+=font_len(PtRiga+SEL_X2,TE->FontHdl,TE->Nfi);

									ColPulish=TE->Col2;
								 }
								goto PULISH;
							 }


					 if (ptre==SEL_Y2)
							 {
								CHAR bak;
								// Non selezionato
								bak=*(TE->Riga2+SEL_X2);
								*(TE->Riga2+SEL_X2)=0;

								spiazi=font_len(PtRiga,TE->FontHdl,TE->Nfi);
								dispfm_h(cam_x-(SINT) TE->X_offset,cam_y,
														TE->Col2,TE->Col1,ON,SET,
														TE->FontHdl,TE->Nfi,PtRiga);

								*(TE->Riga2+SEL_X2)=bak;

								spiazi+=font_len(PtRiga+SEL_X2,TE->FontHdl,TE->Nfi);
								dispfm_h(cam_x-(SINT) TE->X_offset+spiazi,cam_y,
														 TE->Col1,TE->Col2,ON,SET,
														 TE->FontHdl,TE->Nfi,PtRiga+SEL_X2);

								goto PULISH;
							 }

					}
					else
					// ----------------------
					// Stampa normale       !
					// ----------------------
					{
					 dispfm_h(cam_x-(SINT) TE->X_offset,cam_y,TE->Col1,TE->Col2,ON,SET,
										TE->FontHdl,TE->Nfi,TE->Riga2);
					 spiazi=font_len(TE->Riga2,TE->FontHdl,TE->Nfi);
					}
				 PULISH:
				 //if (daPulire)
				 boxp(cam_x+spiazi-(SINT) TE->X_offset,cam_y,x2+1,cam_y+fontalt-1,ColPulish,SET);
				 }
				}
	 avanti:
	 cam_y+=fontalt; ptre++;
	}

	clip_pop();

/*
	//					Aggiorna barra selettrice (solo con campi sufficienti)
	if (maxcam>ncam) {

	 a=(SINT) ((offset*bar_area)/(maxcam-ncam));
	 if (offset<koffset) lamp=1; else lamp=0;

	 if (poj->lock==ON) {
		ico_disp(bar_x,bar_y+a,"MKEY3");
		if ((a>0)&&(!lamp))
				boxp(bar_x,bar_y,bar_x+10,bar_y+a-1,5,SET);

		if ((a<bar_area)&lamp)
				boxp(bar_x,bar_y+a+14,bar_x+10,bar_y+bar_dim,5,SET);//10

	 }
		else boxp(bar_x,bar_y,bar_x+10,bar_y+bar_dim,5,SET);
	}
	//(poj->sub)(WS_SEL,selez,""); //	Aggiorna selezione
	*/

		TE->Y_koffset=TE->Y_offset;
		TE->X_koffset=TE->X_offset;

	if (refre==ON) TE->refre=OFF; // 	refresh schermo fatto
}

void TE_LineRefresh(LONG ptre,CHAR *CampoLinea,TEDIT *TE)
{
	TE_LineWrite(ptre,CampoLinea,TE);
	TE->refre=ON;
	TE_drw(TE,TE->Cy,TE->Cy);
}
static SINT Shift(void)
{
/*
	SINT shk;
	shk=peek(0,0x417);
	if (shk&1) return ON;
	if (shk&2) return ON;
	*/
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
 SINT x,k1,k2;
 if (!Shift()) return;

 TE_LineRead(TE->ptre,TE);

 x=TE->Cx;
 if (x>strlen(TE->CampoLinea)) x=strlen(TE->CampoLinea);

 TE->SEL_Xend=x;
 TE->SEL_Yend=TE->ptre;

 TE->refre=ON;
 k1=TE->Cy-1; k2=TE->Cy+1;
// if (TE->SEL_Ystart<TE->ptre) {k1=TE->Cy-1; if (k1<0) k1=0;}
// if (TE->SEL_Ystart>TE->ptre) {k2=TE->Cy+1; if (k2>TE->ncam) k2=TE->ncam;}
 if (k1<0) k1=0;
 if (k2>TE->ncam) k2=TE->ncam;
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

static SINT SELcoo(TEDIT *TE,
									SINT *X1,LONG *Y1,
									SINT *X2,LONG *Y2)
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
		 if ((dove==(SEL_Y2-1))&&(SEL_X2==0)&&TE->Cx<strlen(TE->Riga2)) SEL_Y2--;
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

	if (SELcoo(TE,&SEL_X1,&SEL_Y1,&SEL_X2,&SEL_Y2)) {efx2(); return;}

// sprintf(serv,"Copia da %d:%ld a %d:%ld",SEL_X1,SEL_Y1,SEL_X2,SEL_Y2);
// win_time(serv,1|NOWIN);

 if (TE->HdlSEL>-1) memo_libera(TE->HdlSEL,"SELECT");

 Line=SEL_Y2-SEL_Y1+1;
 TE->HdlSEL=memo_chiedi(RAM_AUTO,(LONG) Line*TE->SizeLine,"TE:Select");
 if (TE->HdlSEL<0) {win_info("No memory for COPIA/INCOLLA");return;}
 TE->SelLine=Line;
 TE->SelX1=SEL_X1;
 TE->SelX2=SEL_X2;

 for (pt=0;pt<Line;pt++)
 {
	 TE_LineRead(SEL_Y1+pt,TE);
	 memo_scrivivar(TE->HdlSEL,pt*TE->SizeLine,TE->CampoLinea,TE->SizeLine);
 }

}

//------------------------------------
// Incolla la selezione              !
//------------------------------------

static void SELincolla(TEDIT *TE)
{
	LONG pt;
	LONG ofs=0;
	LONG Line;
	SINT Hlocal;
	CHAR *Buf;
	SINT start;

	if (TE->HdlSEL<0) {efx2(); return;}

	Hlocal=memo_chiedi(RAM_HEAP,TE->SizeLine,"Local");
	if (Hlocal<0) PRG_end("<--->");
	Buf=memo_heap(Hlocal);

	Line=TE->SelLine;
	//printf("In ingresso : %ld\n",Line);

	// -------------------------
	// Inizio Troncato         !
	// -------------------------

	if (TE->Cx>0)
		 {
			TE_LineRead(TE->ptre,TE);

			// Primo pezzo
			*(TE->CampoLinea+TE->Cx)=0;
			strcpy(Buf,TE->CampoLinea);

			// Aggiunta
			memo_leggivar(TE->HdlSEL,0,TE->CampoLinea,TE->SizeLine);
			if (Line==1) *(TE->CampoLinea+TE->SelX2)=0;
			strcat(Buf,TE->CampoLinea+TE->SelX1);

			TE_LineRead(TE->ptre,TE);
			if (Line==1)
			 {
				// ---------------------------
				// Ultimo Pezzo in coda      !
				// ---------------------------
				strcat(Buf,TE->CampoLinea+TE->Cx);
			 }
			 else
			 {
				// ---------------------------
				// Ultimo pezzo = Riga Nuova !
				// ---------------------------
				TE_LineInsert(TE->ptre+1,1,TE);
				TE_LineWrite(TE->ptre+1,TE->CampoLinea+TE->Cx,TE);
			 }

			TE_LineWrite(TE->ptre,Buf,TE);

			ofs++;
		 }


 // ----------------------------
 //  Corpo Centrale            !
 // ----------------------------

 Line--;
 if ((Line-ofs)>0)
	{
	 //printf("Apro di: %ld (ofs:%ld)\n",Line-ofs,ofs);
	 SINT spost=0;

	 if ((Line-ofs)==1) spost=TE->SelX1;

	 TE_LineInsert(TE->ptre+ofs,(Line-ofs),TE);

	 for (pt=ofs;pt<Line;pt++)
	 {
		//printf("%ld\n",pt);
		//if ((pt==(TE->SelLine-1))&&!TE->SelX2) break;
		memo_leggivar(TE->HdlSEL,
									pt*TE->SizeLine,
									TE->CampoLinea,
									TE->SizeLine);

		TE_LineWrite(TE->ptre+pt,TE->CampoLinea+spost,TE);

	 }

	}

 // ----------------------------
 //  CODA SE C'E'              !
 // ----------------------------

	 if ((TE->SelX2)&&((Line-ofs)>=0))
		{
			if (!TE->Cx) ofs++; // Tappuletto !!!
			memo_leggivar(TE->HdlSEL,
										(TE->SelLine-1)*TE->SizeLine,
										Buf,
										TE->SizeLine);

			Buf[TE->SelX2]=0;
			//printf("[%s]",Buf);
			TE_LineRead(TE->ptre+(ofs+Line-1),TE);
			strcat(Buf,TE->CampoLinea);
			TE_LineWrite(TE->ptre+(ofs+Line-1),Buf,TE);

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
			{SINT a;
			 TE=(TEDIT *) ptr;
			 TE->Px2=TE->Px+TE->Lx-1;
			 TE->Py2=TE->Py+TE->Ly-1;
			 TE->Falt=font_altf(TE->Font,TE->Nfi);
			 font_find(TE->Font,&a,&TE->FontHdl);

			 TE->Cx=0;TE->Cy=0;
			 boxp(TE->Px,TE->Py,TE->Px2,TE->Py2,TE->Col2,SET);
			 cursor_graph(4,TE->Falt,TE->ColCur,2);

			 // Riserva la memoria
			 if (TE->MaxLines<10) PRG_end("TEerr1");
			 TE->Hdl=memo_chiedi(RAM_AUTO,
													(LONG) TE->MaxLines*TE->SizeLine,
													"TEditor");
			 TE->HdlSEL=-1;
			 TE->SelLine=0;
			 TE->SelX1=0;
			 TE->SelX2=0;

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
			 if (TE->HdlSEL>-1) memo_libera(TE->HdlSEL,"SELECT");
			 cursor_off();
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

				DARICAPO:
				CampoLinea=TE->CampoLinea;
				TE->ptre=TE->Y_offset+TE->Cy;
				if (TE->ptre<TE->Lines)
				 {memo_leggivar(TE->Hdl,TE->ptre*TE->SizeLine,CampoLinea,TE->SizeLine);
					}
					else
					{*CampoLinea=0;}

				LenCL=strlen(CampoLinea);

				if (TE->Cx>strlen(CampoLinea)) TE->Cx=strlen(CampoLinea);
				x=font_dim(CampoLinea,TE->Cx,TE->FontHdl,TE->Nfi);

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
				if (!sys.txc_vedi) cursor_on();
				cursor_xy(Rx,TE->Py+TE->Cy*TE->Falt);

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
						crs=font_dim(CampoLinea,a,TE->FontHdl,TE->Nfi);
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

									case 'M': //					FRECCIA DESTRA

														// Controllo della selezione

														TE->Cx++;

														// Va a capo
														if (TE->Cx>LenCL)
															{
															 TE->ptre++; if (TE->ptre>=TE->Lines) {Tonk();break;}
															 TE->Cx=0; TE->Cy++;
															}

														break;

									case 'K': // 					FRECCIA SINISTRA
														TE->Cx--;
														if (TE->Cx<0) {TE->Cx=0;goto CASO1;}
														break;



									case 'P': //					FRECCIA GIU'
														//SELstart(TE);
														if (TE->ptre>=TE->Lines)
															 sonic(5000,1,1,1,1,5);
															 else
															 {
																TE->Cy++;
															 }

														break;

									case 'H': //					FRECCIA SU'
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
									case 'S':
														if (Shift())
															 {//win_time("TAGLIA",1|NOWIN);
																SELcopia(TE);
																SELdelete(TE);
																break;
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
														for (;TE->Cx<strlen(CampoLinea);TE->Cx++) if (*(CampoLinea+TE->Cx)>' ') break;

														if (TE->Cx<0) TE->Cx=0;
														break;

														//	CONTROL + Ins (Copia)
									case 146 : SELcopia(TE);
														 break;

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
		{if (strlen(CampoLinea)<TE->SizeLine)
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

	 if ((!keybuf[0])&&(keybuf[1])&&(strstr("QI",keybuf+1)!=NULL)) {SELend(TE); if (Shift()) TE->refre=ON;}
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
				SINT a;
				TE->Px2=TE->Px+TE->Lx-1;
				TE->Py2=TE->Py+TE->Ly-1;
				TE->refre=ON;// da sostituire con zona
				TE->Falt=font_altf(TE->Font,TE->Nfi);
				font_find(TE->Font,&a,&TE->FontHdl);
				cursor_graph(4,TE->Falt,TE->ColCur,2);
				TE->ncam=((TE->Ly-1)/TE->Falt)+1;
				TE->ncamE=(TE->Ly)/TE->Falt;
				TE_drw(TE,(SINT) dato,-1);
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

				if (TE->Cx>strlen(CampoLinea)) TE->Cx=strlen(CampoLinea);
				x=font_dim(CampoLinea,TE->Cx,TE->FontHdl,TE->Nfi);

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
				cursor_xy(Rx,TE->Py+TE->Cy*TE->Falt);
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

		rifai:
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
		 if (TEF->X>strlen(TE->CampoLinea)) goto avanti;

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

