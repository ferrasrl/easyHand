//   +-------------------------------------------+
//   | scrfinto  Driver scroll esempio           |
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
//#include <flm_vari.h>
#include "c:\ehtool\scrfinto.h"

//  +-----------------------------------------+
//	| scroll_ex- Esempio di scroll            |
//	|            carica i file della directory|
//	|                                         |
//	|                        by Ferr… 1995/96 |
//	+-----------------------------------------+
void * ScrFinto(SINT cmd,LONG info,CHAR *str)
{

	struct WS_DISPEXT *DExt;
	static struct WINSCR buf[1];
	static struct WS_INFO ws;

	//-------------------------------------------------

	if (cmd==WS_INF) return &ws;
	switch (cmd) {

	case WS_BUF : //			  			Richiesta buffer
				break;

	case WS_DISPLAY : // stampo sul video l'informazioni

		DExt=(struct WS_DISPEXT *) str;
		dispfm_h(DExt->px,
						 DExt->py,
						 DExt->col1,
						 DExt->col2,
						 DExt->lock,SET,
						 DExt->hdl,DExt->nfi,"<--->");

	case WS_OFF : //			  												Settaggio offset

				ws.offset=info;
				break;

	case WS_FINDKEY:
	case WS_FIND : //			  						Ricerca la Chiave selezionata
				break;

	case WS_SEL : //			  			Settaggio selez
				ws.selez=info;
				break;

	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata

				buf[0].record=ws.selez;
				buf[0].keypt=NULL;
				break;


	case WS_REFON : //			  			       Richiesta di refresh schermo
				ws.refre=ON;
				break;

	case WS_REFOFF : //			  											 Schermo rifreshato
				ws.refre=OFF;
				break;

	case APRI : //														  PREPARA I DATI

				if (info<3)
						{
						PRG_end("Errore campi scroll_ex");
						}

				ws.sizecam=1;
				ws.numcam=info;// Assegna il numero di campi da visualizzare

	case LOAD :

				ws.maxcam=100;
				ws.offset=0;
				ws.selez=-1;
				ws.koffset=-1;
				ws.kselez=-1;
				ws.dispext=ON;
				ws.refre=ON;
				break;

	case CHIUDI : //														  LIBERA LA MEMORIA

				break;

	 }
	return &buf;
}
