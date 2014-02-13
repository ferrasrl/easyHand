//   ---------------------------------------------
//	 | FB_DRIVE Driver di utilità                |
//	 |                                           |
//	 |                          by Ferrà 1995/96 |
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"

void sort(void *base,SINT num,SINT dim);

//#include "FBASIC.H"

#include "/easyhand/ehtool/fbfile.h"
//extern TCHAR pathcur[];
//static CHAR *PathNow;
static BYTE szFolder[500];

//  +-----------------------------------------+
//	| LISTFILE - Funzione di gestione dati    |
//	|            finestra di scroll	 		  |
//	|                                         |
//	|                        by Ferrà 1995/96 |
//	+-----------------------------------------+
#ifdef _WIN32
//														versione Windows
void * listfile(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,CHAR *str)
{
	#define	 MAX_X 20
	#define	 MAX_Y 30

	static struct WINSCR buf[MAX_Y];
	static struct WS_INFO ws;
	CHAR *ptr;

	LONG a;
	LONG pt;
	struct WS_DISPEXT *DExt;

	static SINT IptMirror;
	static SINT scrhdl=-1;
	//struct ffblk file;
//	FFBLK Fblk;
	EH_DIR sDir;
	SINT fine=0;
	static CHAR *p,*pmem=NULL;
	static CHAR extcur[4];
	//WORD sgm;
	CHAR serv[255];

	//CHAR Bsys.szMouseCursorName[NOMEICONE_SIZE+1]; // Icone corrente del mouse
	//SINT BMS_ax,BMS_ay;

	//-------------------------------------------------

	if (cmd==WS_INF) return &ws;
	switch (cmd) {

	case WS_BUF : //			  			Richiesta buffer

				if (scrhdl==-1) break;
				for (a=0;a<ws.numcam;a++) {
				 pt=a+ws.offset; if (pt>=ws.maxcam) break;
				 buf[(SINT) a].keypt=(CHAR *) (pmem+((SINT) pt*MAX_X));
				}
				break;

	case WS_DISPLAY : //			  			Richiesta buffer

				DExt=(struct WS_DISPEXT *) str;
				ptr=pmem+((SINT) info*MAX_X);
				dispfm_h(DExt->px+2,DExt->py,DExt->col1,DExt->col2,DExt->hdl,ptr);
				
				break;

	case WS_OFF : //			  												Settaggio offset
				ws.offset=info;
				break;

	case WS_KEYPRESS :
				if (key_press(9)||key_press2(_FDX)) strcpy(str,"ESC:->");
				if (key_press2(15)||key_press2(_FSX)) strcpy(str,"ESC:<-");
				//if (key_press(9)) strcpy(str,"ESC:->");
				//if (key_press2(15)) strcpy(str,"ESC:<-");
				break;


	case WS_FINDKEY :
	case WS_FIND : //			  						Ricerca la Chiave selezionata

				if (scrhdl==-1) break;

				strupr(str);
				a=ws.selez+1;

				if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))==0)
						{listfile(NULL,WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								 ws.offset=(ws.maxcam-ws.numcam);
						 if (ws.offset<0) ws.offset=0;
						 listfile(NULL,WS_SEL,a,"");
						 break;}
				{

				for(a=0;a<ws.maxcam;a++)
				{
				 if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))<=0)
						{listfile(NULL,WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								{ws.offset=(ws.maxcam-ws.numcam);}
						 if (ws.offset<0) ws.offset=0;
						 listfile(NULL,WS_SEL,a,"");
						 break;}
				 }
				}
				break;

	case WS_SEL : //			  			Settaggio selez

				ws.selez=info;

				if ((info>-1)&&IptMirror)
					{ipt_write(1,(CHAR *) (pmem+((SINT) info*MAX_X)),0);
					 ipt_vedisolo(1);
					 }

				//sonic(2000,1,1,1,1,6); //ehSleep(30);
				break;

	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata

				buf[0].record=ws.selez;
				buf[0].keypt=(CHAR *) (pmem+((SINT) ws.selez*MAX_X));
				break;


	case WS_REFON : //			  			       Richiesta di refresh schermo

				ws.refre=ON;
				break;

	case WS_REFOFF : //			  											 Schermo rifreshato

				ws.refre=OFF;
				break;

	case WS_OPEN : //														  PREPARA I DATI

				if ((info<4)||(info>MAX_Y))
						{
						ehExit("Errore di assegnazione campi in listfile");
						}

				ws.sizecam=MAX_X;
				ws.numcam=info;// Assegna il numero di campi da visualizzare

	case WS_LOAD :

				if (scrhdl>-1) memoFree(scrhdl,"Cr2");// Libera la memoria

				scrhdl=-1;
				ws.maxcam=0;
				ws.offset=0;
				ws.selez=-1;
				ws.koffset=-1;
				ws.kselez=-1;
				ws.dispext=ON;
				ws.refre=ON;

				//	Conta i file
				strcpy(serv,szFolder); AddBs(serv);
				strcat(serv,"*.");
				strcat(serv,extcur); //strcat(serv,extcur);

				// Cambia il mouse
//				strcpy(Bsys.szMouseCursorName,sys.szMouseCursorName);
//				BMS_ax=MS_ax; BMS_ay=MS_ay;
				mouse_graph(0,0,"CLEX");

		/*
				fine=f_findFirst(serv,&Fblk,FA_ARCH);
				while (!fine) {ws.maxcam++; fine=f_findNext(&Fblk);}
				f_findClose(&Fblk);
				*/
				fileDirOpen(serv,&sDir);
				while (fileDirGet(&sDir)) {ws.maxcam++;}
				fileDirClose(&sDir);
/*
				// Non ci sono pi— files
				if (ws.maxcam)
				{
					if ((DE_coden==ERROR_FILE_NOT_FOUND)||(DE_coden==ERROR_NO_MORE_FILES)) fine=0;
				}
				if (fine) win_infoarg("ListFile() %d\n",DE_coden);
				*/
				if (ws.maxcam==0) goto FINEC;//	No file

				scrhdl=memoAlloc(M_HEAP,(LONG) ws.maxcam*(MAX_X),"listfile()");
				if (scrhdl<0) ehExit("Memoria insufficiente in line");
				pmem=memoPtr(scrhdl,NULL);
				//	Copia i nomi dei file in memoria
				fileDirOpen(serv,&sDir);
				p=pmem; a=0;
				while (fileDirGet(&sDir)) {
//					BYTE *psz;
					a++;
//					if (a>ws.maxcam) ehExit("Errore in listafile");
//					psz=wcsToStr(sDir.sFileInfoW.wcsFileName);
					strcpy((CHAR *) p,sDir.sFileInfo.szFileName);
//					ehFree(psz);
					*p=(BYTE) toupper((SINT) *p);
					p+=MAX_X;
				}
				fileDirClose(&sDir);
/*
				// Non ci sono pi— files
				if (fine)
				{if ((DE_coden==ERROR_FILE_NOT_FOUND)||
						 (DE_coden==ERROR_NO_MORE_FILES)) fine=0;
				}

				if (fine) win_infoarg("ListFile() %d\n",DE_coden);
*/
			//	ORDINA I FILE IN MODO ALFABETICO
				sort(pmem,(SINT) ws.maxcam,MAX_X);

				FINEC:
//				mouse_graph(BMS_ax,BMS_ay,Bsys.szMouseCursorName);
				return (SINT *) fine;
				//break;

	case WS_CLOSE : //														  LIBERA LA MEMORIA

				if (scrhdl>-1) memoFree(scrhdl,"Cr3");// Libera la memoria
				scrhdl=-1;
				break;

	case FBEXT:

				if (strlen(str)>3) break;
				strcpy(extcur,str);
				IptMirror=(SINT) info; // Per la copia nell'input
				break;

	case WS_REALSET :
			 strcpy(szFolder,str);
			 break;
	 }
	return &buf;

#undef MAX_X
#undef MAX_Y
}

#else
//														versione Dos

void * listfile(SINT cmd,LONG info,CHAR *str)
{
	#define	 MAX_X 20
	#define	 MAX_Y 30

	static struct WINSCR buf[MAX_Y];
	static struct WS_INFO ws;

	LONG a;
	LONG pt;

	static SINT IptMirror;
	static SINT scrhdl=-1;
	//struct ffblk file;
	FFBLK Fblk;
	SINT fine;
	static CHAR *p,*pmem=NULL;
	static CHAR extcur[4];
	//WORD sgm;
	CHAR serv[255];

	CHAR Bsys.szMouseCursorName[NOMEICONE_SIZE+1]; // Icone corrente del mouse
	SINT BMS_ax,BMS_ay;

	//-------------------------------------------------

	if (cmd==WS_INF) return &ws;
	switch (cmd) {

	case WS_BUF : //			  			Richiesta buffer

				if (scrhdl==-1) break;
				for (a=0;a<ws.numcam;a++) {
				 pt=a+ws.offset; if (pt>=ws.maxcam) break;
				 buf[(SINT) a].keypt=(CHAR *) (pmem+((SINT) pt*MAX_X));
				}
				break;

	case WS_OFF : //			  												Settaggio offset

				ws.offset=info;
				break;

	case WS_KEYPRESS :
				if (key_press(9)||key_press2(_FDX)) strcpy(str,"ESC:->");
				if (key_press2(15)||key_press2(_FSX)) strcpy(str,"ESC:<-");
				//if (key_press(9)) strcpy(str,"ESC:->");
				//if (key_press2(15)) strcpy(str,"ESC:<-");
				break;


	case WS_FINDKEY :
	case WS_FIND : //			  						Ricerca la Chiave selezionata

				if (scrhdl==-1) break;

				strupr(str);
				a=ws.selez+1;

				if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))==0)
						{listfile(WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								 ws.offset=(ws.maxcam-ws.numcam);
						 if (ws.offset<0) ws.offset=0;
						 listfile(WS_SEL,a,"");
						 break;}
				{

				for(a=0;a<ws.maxcam;a++)
				{
				 if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))<=0)
						{listfile(WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								{ws.offset=(ws.maxcam-ws.numcam);}
						 if (ws.offset<0) ws.offset=0;
						 listfile(WS_SEL,a,"");
						 break;}
				 }
				}
				break;

	case WS_SEL : //			  			Settaggio selez

				ws.selez=info;

				if ((info>-1)&&IptMirror)
					{ipt_write(1,(CHAR *) (pmem+((SINT) info*MAX_X)),0);
					 ipt_vedisolo(1);
					 }

				//sonic(2000,1,1,1,1,6); //ehSleep(30);
				break;

	case WS_PTREC : //			  			Restituisce pt alla chiave selezionata

				buf[0].record=ws.selez;
				buf[0].keypt=(CHAR *) (pmem+((SINT) ws.selez*MAX_X));
				break;


	case WS_REFON : //			  			       Richiesta di refresh schermo

				ws.refre=ON;
				break;

	case WS_REFOFF : //			  											 Schermo rifreshato

				ws.refre=OFF;
				break;

	case WS_OPEN : //														  PREPARA I DATI

				if ((info<4)||(info>MAX_Y))
						{
						ehExit("Errore di assegnazione campi in listfile");
						}

				ws.sizecam=MAX_X;
				ws.numcam=info;// Assegna il numero di campi da visualizzare

	case WS_LOAD :

				if (scrhdl>-1) memoFree(scrhdl,"Cr2");// Libera la memoria

				scrhdl=-1;
				ws.maxcam=0;
				ws.offset=0;
				ws.selez=-1;
				ws.koffset=-1;
				ws.kselez=-1;
				ws.dispext=ON;
				ws.refre=ON;

				//	Conta i file
				strcpy(serv,PathNow);
				strcat(serv,"*.");
				strcat(serv,extcur); //strcat(serv,extcur);

				// Cambia il mouse
				strcpy(Bsys.szMouseCursorName,sys.szMouseCursorName);
				BMS_ax=MS_ax; BMS_ay=MS_ay;
				mouse_graph(0,0,"CLEX");

	/*
	typedef struct {
		 LONG Handle;
		 struct _finddata_t ffile;
		 CHAR  *ff_name;
		 SINT   ff_attrib;
		 CHAR  ff_date[9];
	} FFBLK;

	#endif

	SINT   f_findfirst(CHAR *fname,FFBLK *,SINT attrib);
		*/
				os_errset(OFF);
				fine=f_findFirst(serv,&Fblk,FA_ARCH);
				while (!fine) {ws.maxcam++; fine=f_findNext(&Fblk);}
				f_findClose(&Fblk);

				// Non ci sono pi— files
				if ((fine)&&(DE_coden==0x12)) {fine=0;}
				os_errset(POP);
				if (fine) os_errvedi("ListFile()\n");
				if (ws.maxcam==0) goto FINEC;//	No file

				scrhdl=memoAlloc(M_HEAP,
								   (LONG) ws.maxcam*MAX_X,
								   "listfile()");

				if (scrhdl<0) ehExit("Memoria insufficiente in line");
				pmem=memoPtr(scrhdl);
				//	Copia i nomi dei file in memoria
				os_errset(OFF);
				fine=f_findFirst(serv,&Fblk,FA_ARCH);

				if (fine) {os_errset(POP);goto FINEC;}
				p=pmem; a=0;
				while (!fine) {
					 a++;

					 if (a>ws.maxcam) ehExit("Errore in listafile");
					 strcpy((CHAR *) p,Fblk.ff_name);

					 p+=MAX_X;
					 fine=f_findNext(&Fblk);
				}
				f_findClose(&Fblk);
				// Non ci sono pi— files
				if ((fine)&&(DE_coden==0x12)) {fine=0;}
				os_errset(POP);
				if (fine) os_errvedi("ListFile2()\n");

			//	ORDINA I FILE IN MODO ALFABETICO
				sort(pmem,(SINT) ws.maxcam,MAX_X);

				FINEC:
				mouse_graph(BMS_ax,BMS_ay,Bsys.szMouseCursorName);

				return (SINT *) fine;
				//break;

	case WS_CLOSE : //														  LIBERA LA MEMORIA

				if (scrhdl>-1) memoFree(scrhdl,"Cr3");// Libera la memoria
				scrhdl=-1;
				break;

	case FBEXT:

				if (strlen(str)>3) break;
				strcpy(extcur,str);
				IptMirror=(SINT) info; // Per la copia nell'input
				break;

	case WS_REALSET :
			 PathNow=str;
			 break;

	 }
	return &buf;
#undef MAX_X
#undef MAX_Y

}



#endif




//  +-----------------------------------------+
//	| LISTDIR  - Funzione di gestione dati    |
//	|            finestra di scroll						|
//	|                                         |
//	|                           by Ferrà 1996 |
//	+-----------------------------------------+

void * listdir(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,CHAR *str)
{
	#define	 MAX_X 50
	#define	 MAX_Y 30

	static struct WINSCR buf[MAX_Y];
	static struct WS_INFO ws;
	struct WS_DISPEXT *DExt;
//	static TCHAR *PathNow=NULL;

	LONG a;
	LONG pt;

	CHAR *FlagFine;
	static SINT scrhdl=-1;
//	FFBLK file;
	SINT fine=0;
	static CHAR *p,*pmem=NULL;
	CHAR *ptr;
	//CHAR bb[80];
	CHAR serv[MAXPATH];
	CHAR icotype[12];

//	CHAR Bsys.szMouseCursorName[NOMEICONE_SIZE+1]; // Icone corrente del mouse
//	SINT BMS_ax,BMS_ay;

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

	case WS_DISPLAY : //			  			Richiesta buffer

					DExt=(struct WS_DISPEXT *) str;

					//boxp(DExt->px,DExt->py,DExt->px+5,DExt->py+5,14,SET);
					ptr=pmem+((SINT) info*MAX_X);
					if (!strcmp(ptr,".."))
									strcpy(icotype,"path1");
									else
									{
									if (ws.selez==info) strcpy(icotype,"path2");
																			else strcpy(icotype,"path3");
									}

					ico_disp(DExt->px+1,DExt->py,icotype);

					dispfm_h(DExt->px+20,DExt->py,DExt->col1,DExt->col2,DExt->hdl,ptr);

					break;

	case WS_OFF : //			  			Settaggio offset

				//ws.koffset=ws.offset;
				ws.offset=info;
				break;

	case WS_KEYPRESS :
				if (key_press(9)||key_press2(_FDX)) strcpy(str,"ESC:->");
				if (key_press2(15)||key_press2(_FSX)) strcpy(str,"ESC:<-");
//				if (key_press(9)) strcpy(str,"ESC:->");
//				if (key_press2(15)) strcpy(str,"ESC:<-");
				if (key_press(' ')) strcpy(str,"CR:SPC");
				break;

	case WS_FINDKEY :
	case WS_FIND : //			  						Ricerca la Chiave selezionata

				if (scrhdl==-1) break;

				strupr(str);

				a=ws.selez+1;

				if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))==0)
						{listdir(NULL,WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								 ws.offset=(ws.maxcam-ws.numcam);
						 if (ws.offset<0) ws.offset=0;
						 listdir(NULL,WS_SEL,a,"");
						 break;}

				{

				for(a=0;a<ws.maxcam;a++)
				{

				 if (memcmp(str,pmem+((SINT) a*MAX_X),strlen(str))<=0)
						{listdir(NULL,WS_OFF,a,"");
						 if (ws.offset>(ws.maxcam-ws.numcam))
								{ws.offset=(ws.maxcam-ws.numcam);}
						 if (ws.offset<0) ws.offset=0;
						 listdir(NULL,WS_SEL,a,"");
						 break;}
				 }
				}
				break;

	case WS_SEL : //			  			Settaggio selez

				ws.selez=info;
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

	case WS_OPEN : //														  PREPARA I DATI

				if ((info<4)||(info>MAX_Y))
						{
						ehExit("Errore di assegnazione campi in listfile");
						}

				ws.sizecam=MAX_X;
				ws.numcam=info;// Assegna il numero di campi da visualizzare

	case WS_LOAD :

				if (scrhdl>-1) memoFree(scrhdl,"Cr4");// Libera la memoria

				scrhdl=-1;
				ws.maxcam=0;
				ws.offset=0;
				ws.selez=-1;
				ws.koffset=-1;
				ws.kselez=-1;
				ws.dispext=ON;
				ws.refre=ON;

				if (!*szFolder) break;
				//	Conta i file
				*serv=0;
				//if (*str) strcpy(serv,str);
				strcpy(serv,szFolder); AddBs(serv);
				strcat(serv,"*.");
				strcat(serv,"*"); //strcat(serv,extcur);

				// Cambia il mouse
//				strcpy(Bsys.szMouseCursorName,sys.szMouseCursorName);
//				BMS_ax=MS_ax; BMS_ay=MS_ay;
				mouse_graph(0,0,"CLEX");
				printf("Da fare");
/*
				fine=f_findFirst(serv,&file,FA_DIREC);
				while (!fine)
						{if (file.ff_attrib!=FA_DIREC) goto av2;
						 if (strcmp(file.ff_name,".")) ws.maxcam++;
						 av2:
						 fine=f_findNext(&file);
						}
				f_findClose(&file);
				*/
				// Non ci sono pi— files
				//printf("listdir: %x - %x\n\r",fine,DE_coden);
//				if ((fine)&&(DE_coden==0x12)) {fine=0;}
				//printf("Fine %d",fine);
				if (fine) goto FINEC;

				if (ws.maxcam==0) goto FINEC;//	No file
				scrhdl=memoAlloc(M_HEAP,(LONG) ws.maxcam*MAX_X,"listdir()");
				if (scrhdl<0) ehExit("Memoria insufficiente in listdir");
				pmem=(CHAR *) memoPtr(scrhdl,NULL);

				//	Copia i nomi dei file in memoria
/*
				fine=f_findFirst(serv,&file,FA_DIREC);
				if (fine) {goto FINEC;}
				p=pmem; a=0;
				while (!fine) {
					 if (file.ff_attrib!=FA_DIREC) goto avanti;
					 if (!strcmp(file.ff_name,".")) goto avanti;

					 if (a>ws.maxcam) ehExit("Errore in listdir");
					 strcpy(p,file.ff_name);
					 *p=(BYTE) toupper((SINT) *p);
					 p+=MAX_X;
					 avanti:
					 fine=f_findNext(&file);
				}
				f_findClose(&file);
*/

//				if ((fine)&&(DE_coden==0x12)) {fine=0;}
				if (fine) goto FINEC;

			//	ORDINA I FILE IN MODO ALFABETICO
				sort(pmem,(SINT) ws.maxcam,MAX_X);

				FINEC:
//				mouse_graph(BMS_ax,BMS_ay,Bsys.szMouseCursorName);
				MouseCursorDefault();

				if (fine) {osError(FALSE,GetLastError(),"listdir()");FlagFine=NULL;} else FlagFine=serv;


				return FlagFine; // Ritorna Null se qualcosa Š andato sorto
				//break;

	case WS_CHANGE :

				 //	Legge la directory prescelta
				 //sc=listdir(LEGGI,info);
				 //strcpy(serv,&sc->key[1]);
				 //		Toglie le parentesi quadre
				 strcpy(serv,str);
				 //Adispm(0,0,15,1,ON,SET,serv);

				 //for (a=strlen(serv);(serv[a]!=' ');a--);
				 //serv[a]=0;

				 //				Gestisce il torna indietro
				 if (!strcmp(serv,".."))
					 {//efx2();
						for (a=strlen(szFolder)-2;(szFolder[(SINT) a]!='\\');a--);
						szFolder[(SINT) (a+1)]=0;
						//if (pathcur[a-1]==':') strcat(pathcur,"\\");
						}

				 else {if (szFolder[strlen(szFolder)-1]!='\\') strcat(szFolder,"\\");
							 strcat(szFolder,serv); strcat(szFolder,"\\");}

				 //chdir(pathcur);
//				 listdir(WS_LOAD,0,""); // e le subdirectory
				 //sprintf(serv,"path=%s",pathcur);
				 //Adispm(0,30,15,1,ON,SET,serv);

				 break;


	case WS_CLOSE : //														  LIBERA LA MEMORIA

				if (scrhdl>-1) memoFree(scrhdl,"Cr5");// Libera la memoria
				scrhdl=-1;
				break;

	case WS_REALSET :
			 strcpy(szFolder,str);
			 break;

	//default :

					//efx1();efx1();
	 }
	return &buf;
#undef MAX_X
#undef MAX_Y

}

// +-----------------------------------------+
// |LIST_DRIVE   Crea una array modello x    |
// |             OBJ_list con all'interno    |
// |             Tutti i drive riconosciuti  |
// |             dal DOS & NETWORK           |
// |                                         |
// |                                         |
// |                                         |
// |                                         |
// |                                         |
// | return	Handle della memoria lista       |
// |        Su errore blocca il sistema      |
// |                                         |
// +-----------------------------------------+
void * listdrive(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,CHAR *str)
{
 SINT a,drv_type;
 static CHAR serv[80];
 CHAR label[40];
 //struct ffblk file;
// FFBLK File;
// SINT fine;
 SINT ndrv;
 CHAR *dati,**ptdati;
 LONG buf;
 //struct OBJ *obj=(struct OBJ *) info;
 CHAR *PtCod;
 SINT num;

// static TCHAR *PathNow=NULL;
 static SINT hdl=-1;

 CHAR *typ_drv[]={"DRIVEFD","DRIVEHD","DRIVECD","DRIVENT"};

 // Dimensioni riga lista
 #define DIMRIGA (1+4+4+9+4+3+8+1+3+1+1)

	// ------------------------------------------------------
	//           WS_OPEN : COSTRUISCE PUNTATORI E LISTA  			!
	// ------------------------------------------------------

switch (cmd)
{
 case WS_OPEN:

// l=str;// Serve per togliere warning error su str

 //                                                            !
 //	Conta i drive per determinare la memoria dinamica occupata !
 //                                                            !

 for(a=1,ndrv=0;a!=27;a++) {if (MsDriveType(a)!=-1) ndrv++;}

 //                                                      !
 // Chiede memoria per lista puntatori e zona di memoria !
 //                                                      !
	buf=(LONG) ((ndrv+1)*4);
	buf+=(LONG) ((ndrv+1)*DIMRIGA);
	hdl=memoAlloc(M_HEAP,buf,"listdrive()");

	if (hdl<0) ehExit("Non c'Š memoria HEAP per listdrv()");

	dati=(CHAR *) memoPtr(hdl,NULL);
	ptdati=(CHAR **) dati; objCalled->ptr=ptdati;
	dati+=(ndrv+1)*4;

 //                                                       !
 //										  PREPARA LISTA									    !
 //                                                       !

 ndrv=0;
 for(a=1;a!=27;a++)
 {
	drv_type=MsDriveType(a);
	if (drv_type==-1) continue;// Drive inesistente

	*label=0; // Azzero label
/*
	// Trova nome del drive se non Š un floppy
	if (drv_type>0)
		{ sprintf(serv,"%c:\*.*",(CHAR)(a+64));
			fine=f_findFirst(serv,&file,FA_LABEL);
			if (!fine) strcpy(label,file.ff_name);
		} else strcpy(label,"");

	//		Scrivo lista
	sprintf(serv,"\3  1  0%-8s 21%c: %s",
								typ_drv[drv_type],(CHAR) (a+64),label);
*/
  // Trova nome del drive se non Š un floppy
	if (drv_type>0)
		{ VOLINFO VolInfo;
			sprintf(serv,"%c:\\",(CHAR)(a+64));
			if (!f_volumeinfo(serv,&VolInfo)) strcpy(label,VolInfo.Name);
		} else strcpy(label,"");

	//		Scrivo lista
	sprintf(serv,"\3.  1.  0.%-8s. 21.%c: %s",
								typ_drv[drv_type],(CHAR) (a+64),label);

	strcpy(dati,serv);
	ptdati[ndrv]=dati;

	ndrv++;
	dati+=DIMRIGA;
 }
	strcpy(dati,"\nSTOP");
	ptdati[ndrv]=dati;

	//obj_listset(objCalled->nome,getdisk()-1);// Setta la posizione del puntatore nella lista
	//objCalled->col1=*PathNow-'A';
	objCalled->col2=ndrv;

	// Rintraccia il drive corrente
	ptdati=memoPtr(hdl,NULL);
	num=0;
	for (a=0;a<ndrv;a++)
	{PtCod=*(objCalled->ptr+a); PtCod+=23;//farPtr(PtCod,23);
	 if (*PtCod==*szFolder) {num=a;break;}
	}
	objCalled->col1=num;
	break;


	// ------------------------------------------------------
	//   CHIEDE IL CODICE RELATIVO ALLA SELEZIONE IN LISTA  !
	// ------------------------------------------------------

 case WS_PTREC :
			PtCod=*(objCalled->ptr+objCalled->col1);
			PtCod+=23;//farPtr(PtCod,23);
			*serv=*PtCod;
			serv[1]=0;
			return serv; // Solo il drive

	// ---------------------------------------------------------
	//   RICERCA UN CODICE RELATIVO PER LA SELEZIONE IN LISTA  !
	// ---------------------------------------------------------

 case WS_SEL :
			//DescSize=adb_FldSize(Hdb,"DESCRIZIONE");
//			printf("CI PASSO [%s]",str); getch();
			ptdati=memoPtr(hdl,NULL);
			num=-1;

			for (a=0;a<atoi(objCalled->text+4);a++)
			{PtCod=*(objCalled->ptr+a);
			 PtCod+=23;//farPtr(PtCod,23);
			 //if (!strcmp(PtCod,str)) {num=a;break;}
			 if (*PtCod==*str) {num=a;break;}
			}
			if (num==-1) return NULL;
			objCalled->col1=num;
			return PtCod;

	case WS_KEYPRESS :
				if (key_press(9)||key_press2(_FDX)) strcpy(str,"ESC:->");
				if (key_press2(15)||key_press2(_FSX)) strcpy(str,"ESC:<-");
				if (key_press(' ')) strcpy(str,"CR:SPC");
				break;

	case WS_REALSET :
			 strcpy(szFolder,str);
			 break;

	// ------------------------------------------------------
	//             WS_CLOSE : LIBERA MEMORIA USATA       			!
	// ------------------------------------------------------

 case WS_CLOSE: // Close
	if (hdl!=-1) memoFree(hdl,"listdrive");
	hdl=-1;
	break;
}
return 0;
}

// +-----------------------------------------+
// | leggi_path Legge il drive e percorso    |
// |            corrente                     |
// |                                         |
// | path = Puntatore a stringa len MAXPATH+2|
// |                                         |
// | ritorna 0 = Tutto ok                    |
// |         ABORT = Se non va a buon fine   |
// |                                         |
// +-----------------------------------------+
SINT path_leggi(CHAR *path,CHAR *drive)
{
	SINT a;
	//CHAR bf[MAXPATH+2];
	//CHAR serv[80];

	// Calcolo il pathcur
	if (*drive==0) a=f_getdrive(); // Drive corrente
				   else 
				   a=(SINT) ((CHAR) *drive-64);

	a=f_getdir(a,path);
	if (a) return a;
	return 0;
}
/*
SINT path_leggi(CHAR *path,CHAR *drive)
{
	SINT a;
	//CHAR bf[MAXPATH+2];

	// Calcolo il pathcur
	if (*drive==0) a=getdisk(); // Drive corrente
							 else a=(SINT) ((CHAR) *drive-65);

	os_errset(ONVEDI);
	a=f_getdir(a+1,path);
	os_errset(POP);
	if (a) return a;
	return 0;
}
*/
int cmps(const void *p1,const void *p2)
{
 return strcmp((CHAR *) p1,(CHAR *) p2);
}

void sort(void *base,SINT num,SINT dim)
{
	SINT cmps();
	qsort(base,num,dim,cmps);
}

