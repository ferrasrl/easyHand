//   +-------------------------------------------+
//   | HPTOOLS Utilit… per la stampa su HP       |
//   |                                           |
//   |                                           |
//   |                                           |
//   |                                           |
//   |                                           |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
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

#include <flm_main.h>
#include <flm_vid.h>
*/
#include "\ehtool\include\ehsw_i.h"

#include "c:\ehtool\hptools.h"

extern struct SYSTEM far sys; // Area di sistema (risorse)
extern struct LPT_INFO LPT_info[];

// Di default
static SINT  Fine_X=0;
static SINT  Fine_Y=0;
static LONG Grid_X=300/10;// Decimi di pollice
static LONG Grid_Y=300/6;//  Sesti  di pollice

//   +-------------------------------------------+
//   | HPcontrol Controlla se la stampante di    |
//   |           default Š una HP compatibile    |
//   |                                           |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+
#ifndef _WIN32
SINT HPcontrol(void)
{
 if (LPT_info[sys.lpt_cor].type==LPT_HP) return ON; else return OFF;
}
#endif
//   +-------------------------------------------+
//   | HPEsc     Spedisce la stringa dopo un Esc |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+

void HPEsc(CHAR *str)
{
 CHAR Esc[]={ESC,0};

 lprintn(Esc);
 lprintn(str);
}

//   +-------------------------------------------+
//   | HPCursor  Posizione il cursore            |
//   |                                           |
//   |           Dopo Š possibile stampare       |
//   |           una stringa o un raster         |
//   |                                           |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+

void HPCursor(SINT CoType,LONG x,LONG y)
{
	CHAR serv[30];
	/*
	if (CoType==HP_DOT) sprintf(serv,"*p%ldx%ldY",x,y);
											else
											sprintf(serv,"&a%ldc%ldR",x,y);
	*/

	if (CoType==HP_GRID) {x*=Grid_X; y*=Grid_Y;}
	x+=Fine_X; y+=Fine_Y;
	sprintf(serv,"*p%ldx%ldY",x,y);
	HPEsc(serv);
}

//   +-------------------------------------------+
//   | HPBoxp  Traccia un quadrato pieno         |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+
void HPBoxp(SINT CoType,LONG x,LONG y,LONG x2,LONG y2)
{
	CHAR serv[80];

	HPCursor(CoType,x,y);
	if (CoType==HP_GRID) {x2*=Grid_X; y2*=Grid_Y;}

	sprintf(serv,"*c%lda%ldb%dP",x2-x+1,y2-y+1,0);
	// al posto di 0 c'era il tipo (Black,White,Gray ...) ma non v…
	HPEsc(serv);
}

//   +-------------------------------------------+
//   | HPBoxp  Traccia un quadrato pieno         |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+
void HPBox(SINT CoType,LONG x,LONG y,LONG x2,LONG y2,SINT sizeline)
{
	sizeline--;
	if (CoType==HP_GRID)
		 {x*=Grid_X; y*=Grid_Y;
			x2*=Grid_X; y2*=Grid_Y;}

	HPBoxp(HP_DOT,x,y,x2,y+sizeline);// Linea sopra
	HPBoxp(HP_DOT,x,y2-sizeline,x2,y2);// Linea sotto
	HPBoxp(HP_DOT,x,y,x+sizeline,y2);// Linea sinistra
	HPBoxp(HP_DOT,x2-sizeline,y,x2,y2);// Linea destra
}

//   +-------------------------------------------+
//   | HPDispf Stampa una stringa con un font    |
//   |         prescelto                         |
//   |                                           |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+

SINT HPDispf(SINT CoType,LONG x1,LONG y1,CHAR *font,SINT nfi,CHAR *str)
{
 SINT bmp;
 SINT hdl,maxnfi;
 SINT col1,col2;
 SINT car,y;

 CHAR *p;
 CHAR serv[80];
 struct BMP_HD *BmpHead;

 col1=15;col2=0;

 car=font_find(font,&maxnfi,&hdl); // Cerca il font
 if (car<0) {return -200;}
// Tapullo
#ifndef _WIN32
 bmp=font_bmp(str,RAM_HEAP,hdl,nfi,col1,col2,ON);
#endif
 if (bmp==-4) //bmp=font_bmp(str,RAM_ALTA,hdl,nfi,col1,col2,status);
	 {efx2(); return -100;}

 if (bmp<0) return bmp;
 p=memo_heap(bmp);
 BmpHead=(struct BMP_HD *) p;
 p+=sizeof(struct BMP_HD);

 for (y=0;y<BmpHead->alt;y++)
	{
	 HPCursor(CoType,x1,y1+y);
	 HPEsc("*r1A"); // start raster
	 HPEsc("*b0M"); // metodo di compressione
//	 HPesc("*b80W"); // Inizio trasferimento dati
	 sprintf(serv,"%c*b%dW",ESC,BmpHead->riga);
	 lprintn(serv);
	 lpt_send(p,BmpHead->riga);
	 p+=BmpHead->riga;
	}


 memo_libera(bmp,"disp_lpt");
 return 0;
}

//   +-------------------------------------------+
//   | HPBmpDisp  Stampa un bitmap MONOCROMATICO |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+
/*
SINT HPBmpDisp(SINT CoType,LONG x1,LONG y1,CHAR *file)
{
 SINT bmp;
 LONG y;
 LONG pos;
 SINT HdlLocal;
 CHAR *BufLocal;

 CHAR serv[80];
 struct BMP_HD BmpHead;
 struct BMPINFO BmpInfo;

 if (CoType==HP_GRID) {x1*=Grid_X; y1*=Grid_Y;}
 bmp=bmp_load(file,&BmpInfo,RAM_AUTO);
 if (bmp<0) return bmp;

 memo_leggivar(bmp,0,&BmpHead,sizeof(BmpHead));
 pos=sizeof(BmpHead);
 HdlLocal=memo_chiedi(RAM_HEAP,BmpHead.riga,"HPlocal");
 if (HdlLocal<0) goto FINE;
 BufLocal=memo_heap(HdlLocal);

 HPCursor(HP_DOT,x1,y1);
 sprintf(serv,"*r%lds0t1A",BmpHead.larg); // Raster area
 HPEsc(serv);

 for (y=0;y<BmpHead.alt;y++)
	{
//	 int pixel,mask,pt;
	 sprintf(serv,"*b%dW",BmpHead.riga); HPEsc(serv);
	 memo_leggivar(bmp,pos,BufLocal,BmpHead.riga);
	 // Andizza (neologina su AND) la fine
	 //pixel=(SINT) (BmpHead.larg&0x7); pt=(SINT) (BmpHead.larg>>3);
	 //mask=0xFF>>pixel; mask^=0xFF; BufLocal[pt]&=mask;
	 lpt_send(BufLocal,BmpHead.riga);
	 pos+=BmpHead.riga;
	}

 memo_libera(HdlLocal,"HP");
 HPEsc("*rb"); // end raster
 // uscita
 FINE:
 memo_libera(bmp,"disp_lpt");
 return 0;
}
	*/

// ----------------------------------
// COMPRIME I DATI IN RLE           |
// ----------------------------------
static SINT HPcompress(CHAR *BufLocal,CHAR *BufCompress,SINT riga)
{
	SINT a;
 //	SINT pt,pixel,mask;
	CHAR *PSorg;
	CHAR *PDest;
	SINT Ultimo,CntByte,CntReal;

//	pixel=(int) (riga&0x7); pt=(int) (riga>>3);
//	mask=0xFF>>pixel; mask^=0xFF; BufLocal[pt]&=mask;

	// ---------------------------------
	// 1) Elimino gli zero in coda     |
	// ---------------------------------
	for (a=(SINT) (riga-1);a>=0;a--) if (BufLocal[a]) break;
	if (a<0) riga=0; else riga=a+1;

	// ---------------------------------
	// 2) Compressione RLE             |
	// ---------------------------------

	CntReal=0;
	CntByte=0;
	Ultimo=-1;
	PSorg=BufLocal;
	PDest=BufCompress;

	for (a=0;a<riga;a++,PSorg++)
	{
	 if (*PSorg==Ultimo) CntByte++;
	 if ((*PSorg!=Ultimo)&&(Ultimo!=-1))
		 {
			*PDest++=CntByte; *PDest++=Ultimo;
			CntReal+=2; CntByte=0;
		 }
	 Ultimo=*PSorg;
	}

	*PDest++=CntByte; *PDest++=Ultimo;
	CntReal+=2;

	if (CntReal>riga) {memcpy(BufCompress,BufLocal,(SINT) riga);
										 riga=-riga;
										}
										else
										riga=CntReal;
	return riga;
}

SINT HPBmpDisp(SINT CoType,LONG x1,LONG y1,CHAR *file)
{
 SINT x,bmp;
 LONG y;
 LONG pos;
 SINT HdlLocal;
 CHAR *BufLocal;
 CHAR *BufCompress;

 CHAR serv[80];
 struct BMP_HD BmpHead;
 struct BMPINFO BmpInfo;

 if (CoType==HP_GRID) {x1*=Grid_X; y1*=Grid_Y;}
 bmp=bmp_load(file,&BmpInfo,RAM_AUTO);
 if (bmp<0) return bmp;

 memo_leggivar(bmp,0,&BmpHead,sizeof(BmpHead));
 pos=sizeof(BmpHead);
 HdlLocal=memo_chiedi(RAM_HEAP,BmpHead.riga+(BmpHead.riga*2),"HPlocal");
 if (HdlLocal<0) goto FINE;
 BufLocal=memo_heap(HdlLocal);
 BufCompress=BufLocal+BmpHead.riga;

 HPCursor(HP_DOT,x1,y1);
 sprintf(serv,"*r%lds0t1A",BmpHead.larg); // Raster area
 HPEsc(serv);

 for (y=0;y<BmpHead.alt;y++)
	{
//	 int pixel,mask,pt;
	 memo_leggivar(bmp,pos,BufLocal,BmpHead.riga);
	 x=HPcompress(BufLocal,BufCompress,BmpHead.riga);
	 if (x<=0)
			{sprintf(serv,"*b0m%dW",(SINT) -x); HPEsc(serv);
			 lpt_send(BufLocal,-x);
			}
			 else
			{sprintf(serv,"*b1m%dW",(SINT ) x); HPEsc(serv);
			 lpt_send(BufCompress,x);
			}

	 pos+=BmpHead.riga;
	}

 memo_libera(HdlLocal,"HP");
 HPEsc("*rb"); // end raster
 // uscita
 FINE:
 memo_libera(bmp,"disp_lpt");
 return 0;
}

//   +-------------------------------------------+
//   | HPDisptype Stampa utilizzando i font      |
//   |            typefaces scalabili            |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+
typedef struct {
		CHAR Nome[20];
//		char PreSeq[10];
		CHAR Sequence[20];
		} HPFONT ;

SINT HPDispType(SINT CoType,LONG x1,LONG y1,CHAR *font,SINT pitch,CHAR *str)
{
 CHAR serv[10];
 SINT a,nf;
 SINT fix=OFF;

 HPFONT HpFont[]={
 {"Courier"         ,"(s0p0s0b4099T"},
 {"Courier Bd"      ,"(s0p0s3b4099T"},
 {"Courier It"      ,"(s0p1s0b4099T"},
 {"Courier Bd It"   ,"(s0p1s3b4099T"},
 {"CG Times"        ,"(s1p0s0b4101T"},
 {"CG Times Bd"     ,"(s1p0s3b4101T"},
 {"CG Times It"     ,"(s1p1s0b4101T"},
 {"CG Times Bd It"  ,"(s1p1s3b4101T"},

 {"CG Omega"        ,"(s1p0s0b4113T"},
 {"CG Omega Bd"     ,"(s1p0s3b4113T"},
 {"CG Omega It"     ,"(s1p1s0b4113T"},
 {"CG Omega Bd It"  ,"(s1p1s3b4113T"},

 {"Coronet"         ,"(s1p1s0b4116T"},
 {"Claredon Cd"     ,"(s1p4s3b4140T"},
 {"Univers Md"      ,"(s1p0s0b4148T"},
 {"Antique Olive"   ,"(s1p0s0b4168T"},
 {"Garamond"        ,"(s1p0s0b4197T"},
 {"Letter Ghotic"   ,"(s1p0s0b4102T"},
 {"",""}};


 if (!*font) goto SOLOSTAMPA;
 if (*font=='#') {fix=ON;font++;}

 nf=-1;
 for (a=0;;a++)
 {
	if (!HpFont[a].Nome[0]) break;
	if (!strcmp(HpFont[a].Nome,font)) {nf=a;break;}
 }
 if (nf==-1) return -1;

	//HPEsc(HpFont[nf].PreSeq);

	HPEsc(HpFont[nf].Sequence);
	if (fix) HPEsc("(s0P");
	if (pitch>0)
		{sprintf(serv,"(s%dV",pitch);
		 HPEsc(serv);
		 }

	SOLOSTAMPA:
	HPCursor(CoType,x1,y1);lprintn(str);
 return 0;
}

void HPSetGrid(SINT x,SINT y)
{
 Grid_X=x; Grid_Y=y;
}

void HPSetFine(SINT x,SINT y)
{
 Fine_X=x; Fine_Y=y;
}

LONG HPgrid_y(SINT y)
{
 return y*Grid_Y;
}
LONG HPgrid_x(SINT x)
{
 return x*Grid_X;
}

void HPMark(SINT CoType,LONG x1,LONG y1,SINT finex,SINT finey,SINT flag)
{
 const SINT dim=30;

 if (CoType==HP_GRID) {x1*=Grid_X; y1*=Grid_Y;}
 x1+=finex; y1+=finey;

 if (flag)
	 {HPBox(HP_DOT,x1,y1-dim,x1+dim,y1,3);
		HPDispType(HP_DOT,x1+6,y1-6,"CG Omega Bd",6,"X");
	 }
	 else HPBox(HP_DOT,x1,y1-dim,x1+dim,y1,1);
}