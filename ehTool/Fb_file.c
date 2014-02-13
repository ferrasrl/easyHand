//	-------------------------------------------
//	| FBFile  FileAsk                         |
//	|                                         |
//	|                                         |
//	-------------------------------------------

#include "/easyhand/inc/easyhand.h"
#ifdef _WIN32
#include <shlobj.h>
#endif

#include "/easyhand/ehtool/fbfile.h"

extern CHAR pathcur[];

//void * listfile(SINT cmd,LONG info,TCHAR *str);
//void * listdir(SINT cmd,LONG info,TCHAR *str);
//void * listdrive(SINT cmd,LONG info,TCHAR *str);
//SINT path_leggi(TCHAR *path,TCHAR *drive);

static void AllPathChange(CHAR *PathNow)
{
	listdir(NULL,WS_REALSET,0,PathNow);
	listfile(NULL,WS_REALSET,0,PathNow);
	listdrive(NULL,WS_REALSET,0,PathNow);
//	obj_listcodset("DRV",PathNow);
}

static SINT Prompt(CHAR *comm,CHAR *PathNow,CHAR *FileName)
{
	/*
 SINT a;
 CHAR *pt,*pk;
 BOOL DirFlag=OFF;
 struct WS_INFO *ws;
// struct ffblk file;
 FFBLK FfBlk;
 SINT Flag;
 CHAR Path[MAXPATH];
 CHAR Drv[2];
 CHAR *p;
 pt=comm;
 a=strlen(pt);

 ipt_noedit();
 // Nessun dato
 if (!a) goto CommandOk;

 if (a==1) return OFF;

 // -----------------------------------------------------------
 // Controllo se contiene un indicazione di cambio drive      |
 // Cambio di drive                                           |
 // -----------------------------------------------------------

 if (*(pt+1)==':')
	 {
		Drv[0]=*pt; Drv[1]=0; strupr(Drv);

		obj_listcodset("DRV",Drv);
		pk=obj_listcodget("DRV"); if (*pk!=*Drv) goto Sintax;
		pt+=2;

		// -----------------------------
		// Azzero le voci di directory |
		// -----------------------------
		DirFlag=ON;
		ws=listdir(NULL,WS_INF,0,""); ws->maxcam=0;
		if (path_leggi(Path,Drv))
			 {obj_listcodset("DRV",Path);
				goto Sintax;
			 }
	 }

 // Errore di posizione dei :
 if (strstr(pt,":")) goto Sintax;
	// Solo il drive
 if (!*pt)
	 {strcpy(PathNow,Path);
		goto CommandOk;
		}

 // Controllo all'indietro
 loop:
 if (!strcmp(pt,"..")) {listdir(NULL,WS_CHANGE,0,".."); DirFlag=ON; goto CommandOk;}
 if (!memcmp(pt,"..\\",3)) {listdir(NULL,WS_CHANGE,0,".."); pt+=3; goto loop;}
// strcat(PathNow,pt);
 if (!*pt) goto CommandOk;

 Drv[0]=*Path; Drv[1]=0; strupr(Drv);
 // Verifico se Š una directory
 if (*pt=='\\') sprintf(Path,"%s:%s\\*.*",Drv,pt);
								else
								sprintf(Path,"%s%s\\*.*",PathNow,pt);

 //printf("[%s]",Path); getch(); goto Sintax;
 Flag=f_findFirst(Path,&FfBlk,0);
 if (!Flag)
	{DirFlag=ON;
	 //strcat(PathNow,pt);
	 //strcat(PathNow,"\\");
	 p=fileName(Path); if (p) *p=0;
	 strcpy(PathNow,Path);
	 //listdir(WS_REALSET,0,PathNow);

	 goto CommandOk;
	}
 f_findClose(&FfBlk);
 p=fileName(Path); if (p) *(p-1)=0;
 strcpy(FileName,Path);
// if (*fileName(comm)) strcat(FileName,fileName(comm));

// printf("File [%s]",FileName); getch();

 return OFF;

 CommandOk:
 if (DirFlag)
	 {obj_reset("DIR",ON);
		obj_reset("FILE",ON);
		ipt_write(0,PathNow,0);
		}
 *comm=0; return ON;

 Sintax:
 //if (DirFlag)
 //printf("[%s]",PathNow); getch();
 obj_listcodset("DRV",PathNow);
 obj_reset("DIR",ON);
 obj_reset("FILE",ON);
 ipt_write(0,PathNow,0);
 //		 }
 efx2(); return ON;
 */
	return ON;
}


// +-----------------------------------------+
// | 						LEGGE UN PROGETTO 					 |
// +-----------------------------------------+
SINT FileAsk(CHAR *Titolo,CHAR *Ext,CHAR *NomeFile,SINT FlagExist,CHAR *Tasto,CHAR *SopraIPT)
{
 return FileAskF(Titolo,Ext,NomeFile,FlagExist,Tasto,SopraIPT,NULL);
}

SINT FileAskF(CHAR *Titolo,CHAR *Ext,
							CHAR *NomeFile,SINT FlagExist,
							CHAR *Tasto,CHAR *SopraIPT,
							FBFILEDRV *FBFileDrv
							)
{
//	FILE 	*pf1;
	struct WS_INFO *ws;
	SINT a,flag=ON;
	CHAR file[MAXPATH];
	CHAR *p,serv[MAXPATH];
	CHAR PathNow[MAXPATH];
//	TCHAR pathbak[MAXPATH];
//	TCHAR pathbk2[MAXPATH];

	struct OBJ obj[]={
// TipObj    Nome    Sts Lck   x   y CL1 CL2 TESTO
	{O_SCROLL,"FILE"   ,OFF,ON , 14,113,0, 12,"122","",listfile},
	{O_SCROLL,"DIR"    ,OFF,ON ,164,149,0,  9,"126","",listdir},
//	{O_MARK  ,"MERGE"   ,OFF,ON ,149,76,0,-1,"Overload"},
	{OW_LIST ,"DRV"    ,OFF,ON ,164,113,  0,  3,"126","",listdrive},
	{O_KEYDIM,"ESC"    ,OFF,ON ,236,273, 77, 24,"Annulla"},
	{O_KEYDIM,"CR"     ,OFF,ON , 13,273, 84, 24,"Apri"},
	{O_ICONEB,"DELIPT" ,OFF,ON ,281, 70, 30, 30,"DELIPT"},
	{STOP}
	};

	struct IPT ipt[]={
	{ 0, 0,ALFA,NORM, 14, 42,298,128,  3,  2,  1,  0},
	{ 3, 3,ALFA,RIGA, 16, 77,260,128,  0, 15,  1,  0},
	{ 0, 0,ALFA,NORM, 14, 62,263, 80,  0,  2,  1,  0},
	{ 0, 0,STOP}
	};

	JOBBER; // Riconoscimento programma
	if (!FBFileDrv)
	{
//  Header di attivazione
	 win_open(59,10,321,309,2,3,ON,Titolo);
	 dispf( 14, 29,  0, -1,ON,"SMALL F",3,"Percorso corrente");
//	dispf( 14, 61,  0, -1,ON,"SMALL F",3,SopraIPT);
	 dispf(164,101,  0, -1,ON,"SMALL F",2,"Unità");
	 dispf(164,137,  0, -1,ON,"SMALL F",2,"Percorso");
	 dispf( 13,101,  0, -1,ON,"SMALL F",2,"Progetto");
	}

	listfile(NULL,FBEXT,ON,Ext);

	// -------------------------------------------
	//  Trova il PathCorrente                    |
	// -------------------------------------------
	strcpy(PathNow,NomeFile);
	p=fileName(PathNow); if (p) *p=0;
	if (!*PathNow) strcpy(PathNow,pathcur);

	// -------------------------------------------
	//  Comunica il Path a tutti i driver        |
	// -------------------------------------------
	AllPathChange(PathNow);
	if (!FBFileDrv)
	 {
		obj_open(obj);
		a=obj_find("CR"); if (a>-1) strcpy(obj[a].text,Tasto);
		obj_font("CR","VGASYS",0);

//  Carico IPT & font
		ipt_font("VGASYS",0);
		ipt_open(ipt);
		ipt_fontnum(2,"SMALL F",3);
		ipt_fontnum(0,"SMALL F",4);
	}
	else
	{
	 obj_reset("DRV",OFF);
	 obj_reset("FILE",OFF);
	 obj_reset("DIR",OFF);
	}


	ipt_reset();
	ipt_write(1,fileName(NomeFile),0);
	ipt_write(0,PathNow,0);
	ipt_write(2,SopraIPT,0);
//	listfile(WS_FIND,0,ipt_read(1));
	obj_message("FILE",WS_FIND,0,ipt_read(1));
	ipt_vedi();
	obj_vedi();
	ipt_setnum(1);
#ifdef _WIN32
	MouseCursorDefault();
#else
	mouse_graph(0,0,"MS01");
#endif

	for (;;) {
		ipt_writevedi(0,PathNow,0);
		ipt_ask();

//		printf("[%s]\n",sys.sLastEvent.szObjName);
//		if (key_press2('P')) efx2();
		if (obj_press("FILE->")) {obj_setfocus("DRV"); continue;}
		if (obj_press("DRV->")) {obj_setfocus("DIR"); continue;}
		if (obj_press("DIR->")) {obj_setfocus(""); continue;}
		if (obj_press("FILE<-")) {obj_setfocus(""); continue;}
		if (obj_press("DIR<-")) {obj_setfocus("FILE"); continue;}
		if (obj_press("DRV<-")) {obj_setfocus("DIR"); continue;}

		if (key_press(9)||key_press2(_FDN))
			 {ws=obj_message("FILE",WS_INF,0,"");
				if (ws->maxcam==0) obj_setfocus("DRV");
													 else
													 obj_setfocus("FILE");
				continue;}

		if (key_press2(15)) {obj_setfocus("DRV"); continue;}
		if (key_press2(_HOME)||obj_press("DELIPTOFF")) ipt_writevedi(1,"",0);
		if (obj_press("ESCOFF")||key_press(ESC))
			{//strcpy(pathcur,pathbak);
			 break;
			}

		// 			               -
		//	  WS_CHANGE DI DRIVE  -
		//									   -
		if (obj_press("DRV")||obj_press("DRVSPC"))
				{
				 //strcpy(pathbk2,pathcur);
				 if (path_leggi(PathNow,OBJ_keyname))
					 {// Path errato
						obj_listcodset("DRV",pathcur);
						continue;
					 }

				 if (listdir(NULL,WS_LOAD,0,"")==NULL)
							{strcpy(PathNow,ipt_read(0));
							 listdir(NULL,WS_LOAD,0,"");
							 obj_listcodset("DRV",PathNow);
							}
				 obj_message("FILE",WS_LOAD,0,""); // rilegge i file
				 ipt_write(0,PathNow,0); ipt_vedi();
				 obj_vedi();
				 continue;
				}

		// 			               -
		//	  WS_CHANGE DI PATH   -
		//									   -
		if (obj_press("DIR")||obj_press("DIRSPC"))
				{
				 listdir(NULL,WS_CHANGE,0,OBJ_keyname);
				 listdir(NULL,WS_LOAD,0,""); // rilegge i file
				 obj_message("FILE",WS_LOAD,0,""); // rilegge i file
				 ipt_writevedi(0,PathNow,0); //ipt_vedi();
				 obj_vedi();
				 if (obj_press("DIRSPC")) obj_setfocus("DIR");
				 continue;
				 }

		// 			               -
		//	  SELEZIONE FILE   -
		//									   -
		if (obj_press("FILE"))
				{strcpy(serv,OBJ_keyname);
				 ipt_writevedi(1,OBJ_keyname,0);
				 }

		// 			                    -
		//	  MEMORIZZAZIONE FILE   -
		//									        -
		if (key_press(CR)||obj_press("FILE")||obj_press("CROFF"))
		{
			//obj_prezoom("CR");
//			strcpy(serv,ipt_read(1));
//			if (!strlen(serv)) {win_err("Inserire il nome di file valido");
//													continue;}

			if (Prompt(ipt_read(1),PathNow,file)) continue;

			//strcpy(file,PathNow);
			//strcat(file,ipt_read(1));

//		for (p=serv;*p++;) {if (*p=='.') break;}
			//printf("[File : %s]",file); getch();
			p=strstr(file,".");
			if (p) // cŠ un estensione
			 {p++; strupr(p);
				if (strcmp(p,Ext)&&(*Ext!='*')) {win_info("Estensione del file non valida");
														obj_reset("DRV",ON);
														continue;}
			 }
			 else
			 {strcat(file,".");
				strcat(file,Ext);
			 }

		if (!fileCheck(file)&&FlagExist)
		 {win_info("File inesistente.");
			obj_reset("DRV",ON);
			continue;}
// +-----------------------------------------+
// |Û        LEGGE FISICAMENTE IL FILE      Û|
// +-----------------------------------------+
		strcpy(NomeFile,file);
		flag=OFF;
		break;
		}

		if (obj_press("ESCOFF"))
			 {//strcpy(pathcur,pathbak);
				break;
			 }
	};

//	obj_close();
 if (!FBFileDrv)	win_close();
	return flag;
}
// +-----------------------------------------+
// | 	  Elenco dei percorsi disponibili	 |
// +-----------------------------------------+
SINT PathAsk(CHAR *Titolo,CHAR *NomePath,CHAR *Tasto,CHAR *SopraIPT)
{
//	FILE 	*pf1;
//	struct WS_INFO *ws;
	SINT a,flag=ON;
	CHAR file[MAXPATH];
	CHAR *p;//,serv[MAXPATH];
	CHAR PathNow[MAXPATH];
//	TCHAR pathbak[MAXPATH];
//	TCHAR pathbk2[MAXPATH];

	struct OBJ obj[]={
// TipObj    Nome    Sts Lck   x   y CL1 CL2 TESTO
	{O_SCROLL,"DIR"    ,OFF,ON , 14,148,0,  9,"235","",listdir},
//	{O_MARK  ,"MERGE"   ,OFF,ON ,149,76,0,-1,"Overload"},
	{OW_LIST ,"DRV"    ,OFF,ON , 14,112,  0,  3,"199","",listdrive},
	{O_KEYDIM,"ESC"    ,OFF,ON ,195,275, 77, 24,"Annulla"},
	{O_KEYDIM,"CR"     ,OFF,ON , 13,275, 84, 24,"Apri"},
	{O_ICONEA,"ICO13"  ,OFF,ON ,240, 70, 32, 30,"DELIPT"},
	{STOP}
	};

	struct IPT ipt[]={
	{ 0, 0,ALFA,NORM, 14, 42,263,128,  3,  2,  1,  0},
	{ 2, 2,ALFA,RIGA, 16, 77,218,128,  0, 15,  1,  0},
	{ 0, 0,STOP}
	};

//  Header di attivazione
	JOBBER; // Riconoscimento programma
	win_open(59,10,285,309,2,3,ON,Titolo);
	dispf( 14, 29,  0, -1,ON,"SMALL F",3,"Percorso corrente");
	dispf( 14, 61,  0, -1,ON,"SMALL F",3,SopraIPT);
	dispf( 14,100,  0, -1,ON,"SMALL F",2,"Unità");
	dispf( 14,136,  0, -1,ON,"SMALL F",2,"Percorso");

	// -------------------------------------------
	//  Trova il PathCorrente                    |
	// -------------------------------------------
	strcpy(PathNow,NomePath);
	p=fileName(PathNow); if (p) *p=0;
	if (!*PathNow) strcpy(PathNow,pathcur);

	// -------------------------------------------
	//  Comunica il Path a tutti i driver        |
	// -------------------------------------------
	AllPathChange(PathNow);
	obj_open(obj);

	a=obj_find("CR"); if (a>-1) strcpy(obj[a].text,Tasto);
	obj_font("CR","VGASYS",0);

//  Carico IPT & font
	ipt_font("VGASYS",0);
	ipt_open(ipt);
    ipt_fontnum(0,"SMALL F",4);
	ipt_reset();

	//mouse_graph(0,0,sys.MouseCursorBase);
//	MouseCursorDefault();
#ifdef _WIN32
	MouseCursorDefault();
#else
	mouse_graph(0,0,"MS01");
#endif

//	ipt_write(1,fileName(NomeFile),0);
	ipt_write(1,"",0);
	ipt_write(0,PathNow,0);
	ipt_vedi();

	obj_vedi();

	ipt_setnum(1);

	for (;;) {
		ipt_writevedi(0,PathNow,0);
		ipt_ask();

//		printf("[%s]\n",sys.sLastEvent.szObjName);
//		if (key_press2('P')) efx2();
		if (obj_press("DRV->")) {obj_setfocus("DIR"); continue;}
		if (obj_press("DIR->")) {obj_setfocus(""); continue;}
		if (obj_press("DRV<-")) {obj_setfocus("DIR"); continue;}

		if (key_press(9)||key_press2(_FDN)) {obj_setfocus("DRV");continue;}

		if (key_press2(15)) {obj_setfocus("DRV"); continue;}
		if (key_press2(_HOME)||obj_press("DELIPTOFF")) ipt_writevedi(1,"",0);
		if (obj_press("ESCOFF")||key_press(ESC))
			{//strcpy(pathcur,pathbak);
			 break;
			}

		// 			               -
		//	  WS_CHANGE DI DRIVE  -
		//									   -
		if (obj_press("DRV")||obj_press("DRVSPC"))
				{
				 //strcpy(pathbk2,pathcur);
				 if (path_leggi(PathNow,OBJ_keyname))
					 {// Path errato
						obj_listcodset("DRV",pathcur);
						continue;
					 }

				 if (listdir(NULL,WS_LOAD,0,"")==NULL)
							{strcpy(PathNow,ipt_read(0));
							 listdir(NULL,WS_LOAD,0,"");
							 obj_listcodset("DRV",PathNow);
							}

				 ipt_writevedi(0,PathNow,0);
				 obj_vedi();
				 continue;
				}

		// 			               -
		//	  WS_CHANGE DI PATH   -
		//									   -

		if (obj_press("DIR")||obj_press("DIRSPC"))
				{
				 listdir(NULL,WS_CHANGE,0,OBJ_keyname);
				 listdir(NULL,WS_LOAD,0,""); // rilegge i file
				 ipt_writevedi(0,PathNow,0); //ipt_vedi();
				 obj_vedi();
				 if (obj_press("DIRSPC")) obj_setfocus("DIR");
				 continue;
				 }

		// 			                    -
		//	  MEMORIZZAZIONE FILE   -
		//									        -
		if (key_press(CR)||obj_press("CROFF"))
		{
			if (!*ipt_read(1))
			{strcpy(NomePath,PathNow);
			 flag=OFF;
			 break;
			}

			//obj_prezoom("CR");
//			strcpy(serv,ipt_read(1));
//			if (!strlen(serv)) {win_err("Inserire il nome di file valido");
//													continue;}

			if (Prompt(ipt_read(1),PathNow,file)) continue;

			//strcpy(file,PathNow);
			//strcat(file,ipt_read(1));

// +-----------------------------------------+
// |Û        LEGGE FISICAMENTE IL FILE      Û|
// +-----------------------------------------+
		}

		if (obj_press("ESCOFF"))
			 {//strcpy(pathcur,pathbak);
				break;
			 }
	};

//	obj_close();
	win_close();
	return flag;
}

#ifdef _WIN32

SINT FileAskOpenWin(CHAR *Titolo,CHAR szFilter[],CHAR *NomeFile,SINT FlagExist)
{
    OPENFILENAME ofn;
	BOOL Ret=FALSE;
/*
	static CHAR szFilter[]= "Tabella di Claudio (*.tdc)\0*.tdc\0" \
							"Tutti i file (*.*)\0*.*\0\0";
*/	
		CHAR File[MAXPATH];
	strcpy(File,NomeFile);

	ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = WIN_info[sys.WinInputFocus].hWnd;
    ofn.hInstance         = sys.EhWinInstance;
    ofn.lpstrFilter       = szFilter;
	ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = File; // File che si vogliono indicare
    ofn.nMaxFile          = _MAX_PATH; // Grandezza del Buffer
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;//Appoggio.Path;
    ofn.lpstrTitle        = Titolo;
    ofn.nFileOffset       = 0;//strlen(szFile)+1; // Offset dei file
    ofn.nFileExtension    = 0;//"\"*.*\"";
    ofn.lpstrDefExt       = NULL;
    ofn.lCustData         = 0;//(LPARAM)&sMyData; // Passo un Puntatore ai Miei dati
	ofn.lpfnHook 		   = NULL;//ComDlg32DlgProc; // Procedura agganciata
	ofn.lpTemplateName    = NULL;//MAKEINTRESOURCE(IDD_COMDLG32); // Dialogo agganciato
    ofn.Flags             = OFN_EXPLORER |    // Abilito il tipo Explorer
							OFN_HIDEREADONLY;// |    // Nascondo Read only
							 //OFN_SHOWHELP | 
		                     //OFN_NOVALIDATE|
							//OFN_CREATEPROMPT;
	if (FlagExist) ofn.Flags|=OFN_FILEMUSTEXIST;

	ipt_noedit();
    //if (GetOpenFileName(&ofn)) {FileTouch=TRUE;ipt_writevedi(5,TLayOut.FileTabella,0);}
    if (GetOpenFileName(&ofn)) 
	{strcpy(NomeFile,File);
     Ret=TRUE;
	}

 //sys.prs1_x=sys.ms_x; 
 //sys.prs1_y=sys.ms_y; 
 return Ret;
 }

SINT FileAskSaveWin(CHAR *Titolo,CHAR szFilter[],CHAR *NomeFile,SINT FlagExist)
{
    OPENFILENAME ofn;
/*
	static CHAR szFilter[]= "Tabella di Claudio (*.tdc)\0*.tdc\0" \
							"Tutti i file (*.*)\0*.*\0\0";
*/	
	CHAR File[MAXPATH];
	strcpy(File,NomeFile);

	ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = WIN_info[sys.WinInputFocus].hWnd;
    ofn.hInstance         = sys.EhWinInstance;
    ofn.lpstrFilter       = szFilter;
	ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = File; // File che si vogliono indicare
    ofn.nMaxFile          = _MAX_PATH; // Grandezza del Buffer
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;//Appoggio.Path;
    ofn.lpstrTitle        = Titolo;
    ofn.nFileOffset       = 0;//strlen(szFile)+1; // Offset dei file
    ofn.nFileExtension    = 0;//"\"*.*\"";
    ofn.lpstrDefExt       = NULL;
    ofn.lCustData         = 0;//(LPARAM)&sMyData; // Passo un Puntatore ai Miei dati
	ofn.lpfnHook 		   = NULL;//ComDlg32DlgProc; // Procedura agganciata
	ofn.lpTemplateName    = NULL;//MAKEINTRESOURCE(IDD_COMDLG32); // Dialogo agganciato
    ofn.Flags             = OFN_EXPLORER |    // Abilito il tipo Explorer
							OFN_HIDEREADONLY;// |    // Nascondo Read only
							 //OFN_SHOWHELP | 
		                     //OFN_NOVALIDATE|
							//OFN_CREATEPROMPT;
	if (FlagExist)  ofn.Flags|=OFN_FILEMUSTEXIST;

	ipt_noedit();
    //if (GetOpenFileName(&ofn)) {FileTouch=TRUE;ipt_writevedi(5,TLayOut.FileTabella,0);}
    if (GetSaveFileName(&ofn)) 
	{strcpy(NomeFile,File);
     return TRUE;
	}
 return FALSE;
}

// +-----------------------------------------+
// | 	  Elenco dei percorsi disponibili	 |
// +-----------------------------------------+

static void * DTreeView(SINT cmd,LONG info,CHAR *str);
static SINT CALLBACK BrowseNotify(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
static CHAR *lpPath;

BOOL PathAskWin(CHAR *Titolo,CHAR *NomePath)
{

	BOOL  fRet;
	LPITEMIDLIST pidl;
//	LPITEMIDLIST pidlRoot;
	LPMALLOC lpMalloc;

	BROWSEINFO bi = {
		WIN_info[0].hWnd,
		NULL,
		NomePath,
		Titolo,
		BIF_RETURNONLYFSDIRS,
		BrowseNotify, 
		0L, 0	};

	lpPath=NomePath;

//	Questo fa' visualizzare solo le directory
//	if (0 != SHGetSpecialFolderLocation(HWND_DESKTOP, CSIDL_DRIVES, &pidlRoot)) return FALSE;
//	if (NULL == pidlRoot) return FALSE;
//	bi.pidlRoot = pidlRoot;

	ipt_noedit();
	pidl = SHBrowseForFolder(&bi);

	if (NULL != pidl) fRet = SHGetPathFromIDList(pidl, NomePath);
	                  else
		              fRet = FALSE;

	// Get the shell's allocator to free PIDLs
	if (!SHGetMalloc(&lpMalloc) && (NULL != lpMalloc))
	{
		/*
		if (NULL != pidlRoot)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidlRoot);
		}
*/
		if (NULL != pidl)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidl);
		}

		lpMalloc->lpVtbl->Release(lpMalloc);
	}

 return fRet;
}

static SINT CALLBACK BrowseNotify(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

	switch (uMsg)
	{
	case BFFM_INITIALIZED:
	{
		// Set initial folder
		/*
		LPSTR pszEnd = PathAddBackslash(pThis->m_pfdin->psz3);
		if (pszEnd - pThis->m_pfdin->psz3 > 3)
		{
			// No problems if not drive root
			*(pszEnd - 1) = '\0';
		}
		*/
//		SendMessage(hwnd, BFFM_SETSELECTION, 1, (LPARAM)pThis->m_pfdin->psz3);
		SendMessage(hwnd, BFFM_SETSELECTION, 1, (LPARAM) lpPath);
		break;
	}

	default:
		return(0);
	}
return(1);
}



#endif