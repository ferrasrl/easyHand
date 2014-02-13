//   +-------------------------------------------+
//   | scrollex  Driver scroll esempio           |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+

#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <dir.h>

#include <flm_main.h>
#include <flm_vid.h>
#include <flm_vari.h>
void sort(void *base,SINT num,SINT dim);

//  +-----------------------------------------+
//	| scroll_ex- Esempio di scroll            |
//	|            carica i file della directory|
//	|                                         |
//	|                        by Ferr… 1995/96 |
//	+-----------------------------------------+
void * scroll_ex(SINT cmd,LONG info,CHAR *str)
{
	#define	 MAX_X 20
	#define	 MAX_Y 30

	static struct WINSCR buf[MAX_Y];
	static struct WS_INFO ws;

	LONG a;
	LONG pt;

	static SINT IptMirror;
	static SINT scrhdl=-1;
	struct ffblk file;
	SINT fine;
	static CHAR *p,*pmem=NULL;
	//WORD sgm;
	CHAR serv[MAXDIR];

	CHAR BMS_ico[NOMEICONE+1]; // Icone corrente del mouse
	SINT BMS_ax,BMS_ay;

	//-------------------------------------------------

	if (cmd==WS_INF) return &ws;
	switch (cmd) {

	case WS_BUF : //			  			Richiesta buffer

				if (scrhdl==-1) break;
				for (a=0;a<ws.numcam;a++) {
				 pt=a+ws.offset; if (pt>=ws.maxcam) break;
				 buf[(SINT) a].keypt=pmem+((SINT) pt*MAX_X);
				}
				break;

	case WS_OFF : //			  												Settaggio offset

				ws.offset=info;
				break;

	case WS_FINDKEY:
				strupr(str);

	case WS_FIND : //			  						Ricerca la Chiave selezionata

				if (scrhdl==-1) break;
				a=ws.selez+1;

				if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))==0)
						{scroll_ex(WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								 ws.offset=(ws.maxcam-ws.numcam);
						 scroll_ex(WS_SEL,a,"");
						 break;}
				{

				for(a=0;a<ws.maxcam;a++)
				{
				 if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))<=0)
						{scroll_ex(WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								{ws.offset=(ws.maxcam-ws.numcam);}
						 scroll_ex(WS_SEL,a,"");
						 break;}
				 }
				}
				break;

	case WS_SEL : //			  			Settaggio selez

				ws.selez=info;

				if ((info>-1)&&IptMirror)
					{ipt_write(1,pmem+((SINT) info*MAX_X),0);
					 ipt_vedisolo(1);
					 }

				//sonic(2000,1,1,1,1,6); //pausa(30);
				break;

	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata

				buf[0].record=ws.selez;
				buf[0].keypt=pmem+((SINT) ws.selez*MAX_X);
				break;


	case WS_REFON : //			  			       Richiesta di refresh schermo

				ws.refre=ON;
				break;

	case WS_REFOFF : //			  											 Schermo rifreshato

				ws.refre=OFF;
				break;

	case APRI : //														  PREPARA I DATI

				if ((info<3)||(info>MAX_Y))
						{
						PRG_end("Errore campi scroll_ex");
						}

				ws.sizecam=MAX_X;
				ws.numcam=info;// Assegna il numero di campi da visualizzare

	case LOAD :

				if (scrhdl>-1) memo_libera(scrhdl,"scr1");// Libera la memoria

				scrhdl=-1;
				ws.maxcam=0;
				ws.offset=0;
				ws.selez=-1;
				ws.koffset=-1;
				ws.kselez=-1;
				ws.dispext=OFF;
				ws.refre=ON;

				//	Conta i file
				*serv=0;
				//if (*str) strcpy(serv,str);
				//strcpy(serv,pathcur);
				strcpy(serv,"*.*");
				//strcat(serv,extcur); //strcat(serv,extcur);

				// Cambia il mouse
				strcpy(BMS_ico,MS_ico);
				BMS_ax=MS_ax; BMS_ay=MS_ay;
				mouse_graph(0,0,"CLEX");

				os_errset(OFF);

				fine=f_findfirst(serv,&file,0);
				while (!fine) {ws.maxcam++; fine=f_findnext(&file);}

				// Non ci sono pi— files
				if ((fine)&&(DE_coden==0x12)) {fine=0;}

				os_errset(POP);

				if (fine) dos_errvedi("listfile()\n");

				if (ws.maxcam==0) goto FINEC;//	No file

				scrhdl=memo_chiedi(RAM_HEAP,
													 (LONG) ws.maxcam*MAX_X,
													 "listfile()");

				if (scrhdl<0) PRG_end("Memoria insufficiente in line __LINE__");
				pmem=memo_heap(scrhdl);

				//	Copia i nomi dei file in memoria

				dos_errset(OFF);

				fine=f_findfirst(serv,&file,0);
				if (fine) goto FINEC;
				p=pmem;
				while (!fine) {
					 strcpy(p,file.ff_name);
					 p+=MAX_X;
					 fine=f_findnext(&file);
				}

				dos_errset(POP);

				if ((fine)&&(DE_coden==0x12)) {fine=0;}
				if (fine) dos_errvedi("");

			//	ORDINA I FILE IN MODO ALFABETICO
				sort(pmem,(SINT) ws.maxcam,MAX_X);

				FINEC:
				mouse_graph(BMS_ax,BMS_ay,BMS_ico);

				return (SINT *) fine;
				//break;

	case CHIUDI : //														  LIBERA LA MEMORIA

				if (scrhdl>-1) memo_libera(scrhdl,"scr2");// Libera la memoria
				scrhdl=-1;
				break;

//	default : efx1();efx1();
	 }
	return &buf;
#undef MAX_X
#undef MAX_Y
}
