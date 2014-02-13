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
extern struct SYSTEM far sys; // Area di sistema (risorse)
#define VIDEO 0xa000

// +-----------------------------------------+
// | SETPAL Setta la pallette dei colori     |
// |                                         |
// | *tav : Puntatore ad una struttura       |
// |        LKUP (LOOKUP)                    |
// |                                         |
// | col_start : colore da settare di        |
// |             partenza                    |
// |                 												 |
// | col_num   : Numero dei colori da        |
// |             settare                     |
// |                 												 |
// | Ritorna 0 se Š tutti OK              	 |
// | oppure :        												 |
// |                 												 |
// | -1 = Se il settaggio Š fuori range      |
// |                 												 |
// |                           by Ferr… 1995 |
// +-----------------------------------------+
SINT decod_64(struct LKUP far *info)
{
	SINT b,c;
	WORD blue,red,green;

	blue=(WORD ) (WORD CHAR *) info->blue;
	red=(WORD ) (WORD CHAR *) info->red;
	green=(WORD ) (WORD CHAR *) info->green;

	//			 Decodifica il settaggio TRUECOLOR in 64 colori
	b=(SINT) (blue)/84;
	c=((b&1)<<3)|((b&2)>>1);
	b=(SINT) (green)/84;
	c=c|(((b&1)<<4)|(b&2));
	b=(SINT) (red)/84;
	c=c|(((b&1)<<5)|((b&2)<<1));
 return c;
}

SINT setpal(struct LKUP far *info,LONG col_start,LONG col_num)
{
	SINT a;
	CHAR pallette[17];

	SINT r_dx,r_es;

	if ((col_start+col_num)>sys.colori) return -1;

	for (a=0;a<16;a++)
	{
	 pallette[a]=(CHAR) decod_64(info+a);
	}

	pallette[16]=0; // colore bordo schermo
	r_dx=FP_OFF(&pallette);
	r_es=FP_SEG(&pallette);

	asm	push ax
	asm push dx
	asm push es

	asm	mov ah,0x10
	asm	mov al,2
	asm mov dx,r_dx
	asm push dx
	asm mov dx,r_es
	asm mov es,dx
	asm pop dx

	asm	int 0x10

	asm	pop es
	asm pop dx
	asm pop ax

 return 0;

}

SINT pic_vedi(CHAR *file)
{
	WORD pf;
	CHAR buf[512];
	//CHAR serv[80];
	CHAR *ptbuf=(CHAR *) &buf;
	SINT a;
	CHAR test;
	SINT regj,posvid;
	SINT npag;
	CHAR register xa;
	CHAR xb;
	SINT r_ds,r_dx;
	FILE *pf1;

 struct LKUP color_b[16]={
	 // B   G   R  nu
	 {  0,  0,  0,  0}, //  0 Nero
	 {  0,  0,  0,  0}, //  1 Grigio scuro
	 {  0,  0,  0,  0}, //  2 Grigio chiaro
	 {  0,  0,  0,  0}, //  3 Blu marino scuro
	 {  0,  0,  0,  0}, //  4 Verdino
	 {  0,  0,  0,  0}, //  5 Lilla
	 {  0,  0,  0,  0}, //  6
	 {  0,  0,  0,  0}, //  7 Blu chiaro
	 {  0,  0,  0,  0}, //  8 Blu medio
	 {  0,  0,  0,  0}, //  9 Rosso
	 {  0,  0,  0,  0}, // 10
	 {  0,  0,  0,  0}, // 11
	 {  0,  0,  0,  0}, // 12
	 {  0,  0,  0,  0}, // 13 Verde
	 {  0,  0,  0,  0}, // 14 Giallo chiaro
	 {  0,  0,  0,  0}  // 15 Nero
 };

	struct LKUP color_tab[16]={
	 // B   G   R  nu
	 {  0,  0,  0,  0}, //  0 Nero
	 {128,128,128,  0}, //  1 Grigio scuro
	 {192,192,192,  0}, //  2 Grigio chiaro
	 {192,  0,  0,  0}, //  3 Blu marino scuro
	 {192,192,  0,  0}, //  4 Verdino
	 {255,192,192,  0}, //  5 Lilla
	 {  0,168,168,  0}, //  6
	 {255,  0,  0,  0}, //  7 Blu chiaro
	 {192,128,128,  0}, //  8 Blu medio
	 {  0,  0,192,  0}, //  9 Rosso
	 {192,192,255,  0}, // 10
	 {  0,  0,255,  0}, // 11
	 {192,255,  0,  0}, // 12
	 {  0,192,  0,  0}, // 13 Verde
	 {  0,255,255,  0}, // 14 Giallo chiaro
	 {255,255,255,  0}  // 15 Nero
 };

// +-----------------------------------------+
// |            ELABORAZIONE DATI            |
// +-----------------------------------------+

	// Apre il file

	a=f_open(file,"rb",&pf1);
	if (a) return a;
	pf=fileno(pf1);

	mouse_off();
	regj=12;
	setpal(color_b,0,16);
	outportb(0x3ce,5); outportb(0x3cf,0);	// Setta modo normale
	outportb(0x3ce,8); outportb(0x3cf,255);// Setta maskera
	outportb(0x3ce,3); outportb(0x3cf,0); 	// setta operatore

	r_ds=FP_SEG(ptbuf);
	r_dx=FP_OFF(ptbuf);

 for (npag=0;npag<4;npag++)
 {
	// Cambio pagina
	//display("passo"); attendi();
	posvid=0;
	outportb(0x3C4,2); outportb(0x3C5,1<<npag);

	for (;;)
	 {
		// Legge un record

		asm push ds
		asm push es
		asm push di
		asm push si
		asm mov ah,0x3f

		asm mov bx,pf
		asm mov cx,512
		asm mov dx,r_dx
		asm push dx
		asm mov dx,r_ds
		asm mov ds,dx
		asm pop dx

		asm int 0x21

		asm pop si
		asm pop di
		asm pop es
		asm pop ds

		//fread(&buf,512,1,pf1);

	 for (;regj<512;)
	 {
		test=buf[regj++];
		if ((test==0) || (test==0x80)) break;

		if (test&0x80)
		 {
		 // +-----------------------------------------+
		 // |           ESPLODE CHAR UGUALI           |
		 // +-----------------------------------------+

			xa=test&0x7f; xb=buf[regj++];
			for (;xa>0;xa--) pokeb(VIDEO,posvid++,xb);
			}
			else
			// +-----------------------------------------+
			// |          ESPLODE CHAR DIVERSI           |
			// +-----------------------------------------+
			{

			xa=test;
			for (;xa>0;xa--) pokeb(VIDEO,posvid++,buf[regj++]);
			}

	 };
	 regj=0;
	 if (test==0) break;
	};
 }
	outportb(0x3D4,0xC); outportb(0x3D5,0);
	outportb(0x3D4,0xD); outportb(0x3D5,0);
 setpal(color_tab,0,16);// da rimettere a posto

 fine:
 f_close(pf1);
 mouse_on();
 return 0;
}
