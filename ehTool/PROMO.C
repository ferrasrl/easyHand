//   +-------------------------------------------+
//   | Promo Finale dei programma                |
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
#include <process.h>

#include <flm_main.h>
#include <flm_vid.h>
#include <flm_vari.h>
*/
#include "\ehtool\include\ehsw_i.h"

static void messaggio_promo_2(void);
void MessaggioPromozionale(void)
{
	SINT x,y,y2;

	x=-200; y=0; y2=100;
	cls();
	clip_set(215,59,422,143,"Promo_end");
//	boxp(416+x,28+y+y2,622+x,112+y+y2,15,SET);
	for (y=0;y<70;y+=3)
	{
	//boxp(416+x,28-y+y2,622+x,112-y+y2,15,SET);
	boxp(416+x,28-y+y2,622+x,33-y+y2,15,SET);
	 ico_disp(422+x,34-y+y2,"LOGOF1");
	boxp(416+x,72-y+y2,622+x,75-y+y2,15,SET);
	 ico_disp(422+x,77-y+y2,"LOGOF2");
	boxp(416+x,107-y+y2,622+x,109-y+y2,15,SET);
	 box(417+x,29-y+y2,621+x,111-y+y2,1,SET);
	 //getch();
	//pausa(10);
	}
	box(416+x,28-y+y2,622+x,115-y+y2,15,SET);
	clip_pop();

//	IcoOndaView(422+x,108,"LOGOF2",10,2,OFF);
	messaggio_promo_2();

}

static void messaggio_promo_2()
{
 CHAR *pt[]={
						 "www.ferra.com",
						 "E-mail: info@ferra.com",
						 //"Tel. ++39 (0182) 54.84.18",
						 "",
						 "Created by G.Tassistro 1998"
						 };

 SINT car_nfi,car_hdl;
 SINT a,x,l;
 SINT nfi=3,y=182;

 a=font_find("SANS SERIF F",&car_nfi,&car_hdl);
 if ((a<0)||(nfi>car_nfi)) return;

 //car_alt=font_alt(car_hdl,nfi);

 for (a=0;a<3;a++)
	{l=font_len(pt[a],car_hdl,nfi);
	 x=(sys.video_x-l)>>1; // Posizione scritta
	 Adispfm_h(x-1,y-1,1,0,ON,SET,car_hdl,nfi,pt[a]);
	 pausa(100);
	 Adispfm_h(x,y,2,0,ON,SET,car_hdl,nfi,pt[a]);
	 pausa(100);
	 Adispfm_h(x+1,y-1,15,0,ON,SET,car_hdl,nfi,pt[a]);
	 //boxp(x,y,x+l,y+car_alt,0,SET);
	 y+=40;
	}
 ico_disp(284,y+10,"FABMASK2");

 pausa(1600);
}
