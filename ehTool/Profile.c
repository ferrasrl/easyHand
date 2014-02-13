//   +-------------------------------------------+
//   | Profile                                   |
//   | Gestisce la gestione dei .INI             |
//   |                                           |
//   |             Created by Tassistro 7/3/98   |
//   |             by Ferrà Art & Technology 1998 |
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/profile.h"


#ifdef _WIN32

// 0= diretto su disco senza condivisione
// 1= In condivisione del file

static SINT iProfileMode=0;


// Modo del profilo
void FSetProfileMode(SINT iMode)
{
	iProfileMode=iMode;
}
#endif

static void LinkIni(TCHAR *lpFile)
{
#ifdef _UNICODE
	wchar_t *p;
 	p=rwcsstr(lpFile,L"."); 
	if (p==NULL) {wcscat(lpFile,L".ini"); return;}
    p=wcsstr(p,L"\\"); 
	if (p!=NULL) {wcscat(lpFile,L".ini"); return;}
#else
	CHAR *p;
	p=strReverseStr(lpFile,"."); if (p==NULL) {strcat(lpFile,".ini"); return;}
    p=strstr(p,"\\"); if (p!=NULL) {strcat(lpFile,".ini"); return;}
#endif
	return;
}


SINT FGetProfileInt(TCHAR *file,CHAR *Var,SINT Dammi)
{
 SINT rt,iSize;
 CHAR Serv[80];
 *Serv=0; iSize=sizeof(Serv);
 rt=FGetProfileCharEx(file,Var,Serv,&iSize);
 if (rt) return Dammi; else return atoi(Serv);
}

void FSetProfileInt(TCHAR *file,CHAR *Var,SINT Valore)
{
 CHAR Serv[10];
 sprintf(Serv,"%d",Valore);
 FSetProfileChar(file,Var,Serv);
}

#ifdef _WIN32

// -------------------------------------------------------------------------------------------------
// FGetProfileChar in WIN32
// -------------------------------------------------------------------------------------------------
BYTE *FGetProfileAlloc(TCHAR *TheFile,CHAR *Var,BYTE *pDefault)
{
	SINT iSize=0;
	BYTE *pData;
	FGetProfileCharEx(TheFile,Var,NULL,&iSize);
	if (!iSize) return strDup(pDefault);
	pData=ehAlloc(iSize);
	FGetProfileCharEx(TheFile,Var,pData,&iSize);
	return pData;
}

SINT FGetProfileChar(TCHAR *TheFile,CHAR *Var,BYTE *Valore)
{
	SINT iSize=-1;
	return FGetProfileCharEx(TheFile,Var,Valore,&iSize);
}

static HANDLE hdlIniRead=INVALID_HANDLE_VALUE;
TCHAR szLastFile[255]={0};

SINT FGetProfileCharEx(TCHAR *TheFile,CHAR *Var,BYTE *Valore,SINT *piSizeValore)
{
 TCHAR File[MAXPATH];
 SINT Flag=ON;
 DWORD dwFileSize;
 SINT hdlMemo;
 CHAR *lpMemo;
 CHAR *pLF;
 CHAR *pEQ;
 DWORD dwRealRead;
 CHAR *lpVAR;
 CHAR *lpFind;
 CHAR *lpVar;
 SINT iSizeData,iSize;
 SECURITY_ATTRIBUTES sSA;
 //static SINT iCount=0;

 if (!*TheFile) ehError();

#ifdef _UNICODE
 wcscpy(File,TheFile);
#else
 strcpy(File,TheFile);
#endif

 LinkIni(File);
 if (Valore) iSizeData=strlen(Valore)+1024; else iSizeData=1024;
 lpVar=GlobalAlloc(GPTR,iSizeData); if (lpVar==NULL) ehExit("FGetProfileChar: no memory");

 if (!fileCheck(File)) {*Valore=0; GlobalFree(lpVar); return -1;}

 ZeroFill(sSA); sSA.nLength=sizeof(sSA);

#ifdef _UNICODE
 if (wcscmp(File,szLastFile)&&hdlIniRead!=INVALID_HANDLE_VALUE&&iProfileMode&&iProfileMode==2)
#else
 if (strcmp(File,szLastFile)&&hdlIniRead!=INVALID_HANDLE_VALUE&&iProfileMode&&iProfileMode==2)
#endif
 {
	CloseHandle(hdlIniRead); hdlIniRead=INVALID_HANDLE_VALUE;
 }
 else
 {
#ifdef _UNICODE
 hdlIniRead = CreateFile((LPCTSTR) File,
					 GENERIC_READ,
					 iProfileMode?FILE_SHARE_READ|FILE_SHARE_WRITE:0,
					 NULL,//&sSA,
			         OPEN_EXISTING,
					 0,
					 (HANDLE)NULL);
 wcscpy(szLastFile,File);
#else

 hdlIniRead = CreateFile((LPCTSTR) File,
//					 GENERIC_READ|GENERIC_WRITE,
					 GENERIC_READ,
					 iProfileMode?FILE_SHARE_READ|FILE_SHARE_WRITE:0,//FILE_SHARE_READ,
					 NULL,//&sSA,
			         OPEN_EXISTING,
					 FILE_FLAG_SEQUENTIAL_SCAN,
					 (HANDLE) NULL);
  //_dx_(0,20,"%d   |%d|",iCount++,iProfileMode);
 strcpy(szLastFile,File);
#endif
 }

 if (hdlIniRead==(HANDLE) INVALID_HANDLE_VALUE) 
 {
	*Valore=0; GlobalFree(lpVar); return -1;
 }


 dwFileSize=GetFileSize(hdlIniRead,NULL);
 hdlMemo=memoAlloc(RAM_AUTO,dwFileSize+1,"INIGet");
 lpMemo=memoLock(hdlMemo); //lpMemo[dwFileSize]=0;
 memset(lpMemo,0,dwFileSize+1);
 if (!ReadFile(hdlIniRead,lpMemo,dwFileSize,&dwRealRead,NULL)) ehExit("Errore in lettura INI");
 //CloseHandle(hdlIni);
 if (iProfileMode!=2)
 {
 	CloseHandle(hdlIniRead); hdlIniRead=INVALID_HANDLE_VALUE;
 }

 // ------------------------------------------------------
 // Cerco la variabile
 //
 lpFind=lpMemo;
 //if (!strcmp(Var,"TERMCODE")) win_infoarg("OK");
 sprintf(lpVar,"%s=",Var);
 while (TRUE)
 {
  lpVAR=strstr(lpFind,lpVar);
  if (lpVAR!=NULL)
  { 
   if (lpVAR>lpMemo) // Controllo che non sia un pezzo di un'altro tag
   {
	   if (*(lpVAR-1)>32) {lpFind=lpVAR+1; continue;}
   }
   pLF=strstr(lpVAR,"\n"); if (pLF) *pLF=0;
   pLF=strstr(lpVAR,"\r"); if (pLF) *pLF=0;
   pEQ=strstr(lpVAR,"="); if (pEQ==NULL) ehExit("Errore");
   iSize=strlen(pEQ+1)+1;
   if (piSizeValore)
   {
	   if (*piSizeValore>0)
	   {
		if (iSize>*piSizeValore) ehExit("INIGetChar [%s][%d] Out of memory",Var,*piSizeValore);
	   }
	   if (!*piSizeValore) {*piSizeValore=iSize; memoFree(hdlMemo,"INIGet"); GlobalFree(lpVar); return FALSE;}
   }
   strcpy(Valore,pEQ+1); Flag=OFF; 
  }
  break;
 }

 GlobalFree(lpVar);
 memoFree(hdlMemo,"INIGet");
 return Flag;
}
#else
// -------------------------------------------------------------------------------------------------
// FGetProfileChar in DOS
//
// -------------------------------------------------------------------------------------------------
SINT FGetProfileChar(CHAR *file,CHAR *Var,CHAR *Valore,SINT iSize)
{
 CHAR File[MAXPATH];
 SINT a;
 FILE *ch;
 CHAR *p;
 SINT Flag=ON;
 CHAR Buf[120];
 SINT SizeBuf=sizeof(Buf);

 strcpy(File,file);
 LinkIni(File);

 if (!fileCheck(File)) {*Valore=0; return -1;}

 f_open(File,"rb",&ch);
 os_errset(OFF);

 while(TRUE)
 {
	if (f_gets(ch,Buf,SizeBuf-1)) break;

	 ///		Tolgo CR O LF
	 for (a=0;a<(SINT) strlen(Buf);a++) if ((Buf[a]=='\n')||(Buf[a]=='\r')) Buf[a]=0;

	 // Se lo trova
	 if (memcmp(Buf,Var,strlen(Var))==0)
		 {p=strstr(Buf,"="); if (!p) break;
			strcpy(Valore,p+1); Flag=OFF; break;
		 }
 }

 fclose(ch);
 os_errset(POP);
 return Flag;
}
#endif

// -------------------------------------------------------------------------------------------------
// FSetProfileChar in WIN32
//
// -------------------------------------------------------------------------------------------------
#ifdef _WIN32

static CRITICAL_SECTION csProfile;
static BOOL fWin32Init=FALSE;

void FSetProfileChar(TCHAR *file,CHAR *lpVariabile,CHAR *lpValore)
{
 TCHAR FileINI[MAXPATH];
 TCHAR NewFile[MAXPATH];
 SINT Flag=OFF;
 HANDLE hdlIni;
 DWORD dwFileSize;
 DWORD dwRealRead;
 //SINT hdlMemo=-1;
 CHAR *lpMemory;
 //CHAR *lpVAR;
 CHAR *lpJolly;
 CHAR *lpVarName;
 BOOL fNew;
 CHAR *lpFind;
 SINT iSize;
 BOOL fError=TRUE;
 SINT a;
 SINT fRename=FALSE;
 SECURITY_ATTRIBUTES sSA;
 SINT iCount;
 BYTE *pLF;

 if (!*file) ehError();

 // Sincronizzo per il MultiThread
 if (!fWin32Init)
 {
	InitializeCriticalSection(&csProfile);
	fWin32Init=TRUE;
 }
 
 EnterCriticalSection(&csProfile);

 iSize=strlen(lpValore)+1024;
 lpVarName=ehAlloc(iSize); 
 if (!lpVarName) ehExit("FSetProfileChar: no memory");
#ifdef _UNICODE
 wcscpy(FileINI,file);
 LinkIni(FileINI);
 if (fRename)
 {
	wcscpy(NewFile,file);
	wcscat(NewFile,L"X");
	LinkIni(NewFile);
 }
#else
 strcpy(FileINI,file);
 LinkIni(FileINI);
 if (fRename)
 {
	strcpy(NewFile,file);
	strcat(NewFile,"X");
	LinkIni(NewFile);
 }
#endif

 //_d_("[%d]",iProfileMode);
 //win_infoarg("[%s - %s]",File,NewFile);
 // ---------------------------------------------
 // IL FILE ESISTE
 // a) Load in memoria
 // b) Ricerco la variabile
 // c) la sostituisco
 //
 fNew=TRUE;
 //win_infoarg("SET (%s)",lpVariabile);
 if (fileCheck(FileINI)) 
  {
	 
	 // ----------------------------------------
	 // A) Carico in memoria il file
	 //
	 for (a=0;a<20;a++)
	 {
	  ZeroFill(sSA);  sSA.nLength=sizeof(sSA);
	  hdlIni= CreateFile((LPCTSTR) FileINI,
						 GENERIC_READ,
						 iProfileMode?FILE_SHARE_READ|FILE_SHARE_WRITE:0,//FILE_SHARE_READ|FILE_SHARE_WRITE,
						 NULL,//&sSA,
						 OPEN_EXISTING,
						 FILE_FLAG_SEQUENTIAL_SCAN,
						 (HANDLE)NULL);

		if (hdlIni==(HANDLE) INVALID_HANDLE_VALUE) {Sleep(200); continue;}
		break;
	 }

	 if (hdlIni==(HANDLE) INVALID_HANDLE_VALUE) ehExit("INISET:Errore in open INI (%s)",FileINI);
	 dwFileSize=GetFileSize(hdlIni,NULL);
	 //hdlMemo=memoAlloc(RAM_AUTO,dwFileSize+iSize,"INISet");
	 lpMemory=ehAllocZero(dwFileSize+iSize);//memoLock(hdlMemo); //lpMemo[dwFileSize]=0;
	 if (!ReadFile(hdlIni,lpMemory,dwFileSize,&dwRealRead,NULL)) ehExit("INISET:Errore in lettura INI");
	 CloseHandle(hdlIni);
	
	 //win_infoarg("Prima: [%s]",lpVariabile);

	 // ------------------------------------------------------
	 // B) Cerco la variabile
	 //
	 lpFind=lpMemory;
	 sprintf(lpVarName,"%s=",lpVariabile);
	 //if (strlen(lpValore)<500) win_infoarg("IN FILE (%s)[%s]",lpVar,lpValore);
	 while (TRUE)
	 {
	  lpJolly=strstr(lpFind,lpVarName);
	  if (lpJolly)
	  { 
		SINT iNewSize;
		SINT lenOld,lenNew,lenCoda;
		BYTE lc;
		BOOL fEnd=FALSE;
		// Se non è la prima variabile
		// Controllo che non sia un pezzo di un'altro tag
		if (lpJolly>lpMemory) {if (*(lpJolly-1)>32) {lpFind=lpJolly+1; continue;}}

		// -----------------------------------------
		// C) Variabile trovata, sostituisco
		//
		iNewSize=strlen(lpVarName)+strlen(lpValore)+2; // Nuova dimensione da scrivere

		//win_infoarg("[%s] %d",lpVar,iNewSize);
		// Cerco dove finisce
		for (pLF=lpJolly;*pLF;pLF++)
		{
			// Prima della fine
			if (fEnd&&*pLF>31) {break;}
			if (!fEnd&&*pLF<32) fEnd=TRUE;
		} 

		lenNew=strlen(lpVarName)+strlen(lpValore)+2; // Len New : Nuova lunghezza dato

		// pLF = punta al byte successivo, all'ultimo byte della variabile
		lc=*pLF; *pLF=0;
		//pLF=strstr(lpJolly,"\n"); if (pLF) *pLF=0;
		lenOld=strlen(lpJolly); // Len Old = Vecchia lunghezza dato
		//win_infoarg("[%s][%s]",lpVar,lpJolly);
		*pLF=lc;

		//if (pLF) *pLF='\n';
		lenCoda=strlen(lpJolly+lenOld)+1; // Lunghezza da spostare (coda del file) +1 è per lo zero
		//win_infoarg("Prima [%d]",lenCoda);
		//_d_(lpJolly+lenOld);
		memmove(lpJolly,lpJolly+lenOld,lenCoda); // Sposto la coda su se stesso (cancello il Tag
		memmove(lpJolly+iNewSize,lpJolly,lenCoda); // Sposto la coda su se stesso (cancello il Tag
		//for (pLF=lpJolly;*pLF;pLF++); // Cerco la fine
		pLF=lpJolly;
		memcpy(pLF,lpVarName,strlen(lpVarName));
		pLF+=strlen(lpVarName);
		memcpy(pLF,lpValore,strlen(lpValore));
		pLF+=strlen(lpValore);
		memcpy(pLF,"\r\n",2);
		//memmove(lpJolly+ln,lpJolly+lo,le); //lpBlock[ln+le]=0;
		//memcpy(lpJolly,lpVar,strlen(lpVar));
		//memcpy(lpJolly+strlen(lpVar),lpValore,strlen(lpValore));
		fNew=FALSE;
	  }
	  break;
	 }
	 //GlobalFree(lpVar);
 }
 else
 // ---------------------------------------------
 // IL FILE NON ESISTE
 // a) Creo l'area di memoria per memorizzare la nuova variabile
 //
 {
	// hdlMemo=memoAlloc(RAM_AUTO,iSize,"ININew");
	 //lpMemo=memoLock(hdlMemo);
	 lpMemory=ehAllocZero(iSize);
//	 memset(lpMemo,0,iSize);
	 //lpMemo[0]=26; lpMemo[1]=0;
	 fNew=TRUE;
 }

 // -----------------------------------------
 // C) Variabile nuova,aggiungo
 //
 if (fNew)
 {
	 CHAR *pLF=lpMemory;
	 //if (strlen(lpValore)<500) win_infoarg("NUOVO [%s][%s]",lpVariabile,lpValore);
	 for (;*pLF;pLF++); // Cerco la fine
	 if (pLF!=lpMemory)
	 {
		 if (*(pLF-1)!='\n') {strcpy(pLF,"\r\n"); pLF+=2;}
	 }
	 sprintf(lpVarName,"%s=",lpVariabile);
     //pEOF=strchr(lpMemo,26); if (pEOF) *pEOF=0;
	 strcpy(pLF,lpVarName);
	 strcat(pLF,lpValore);
	 strcat(pLF,"\r\n");
	 //win_infoarg("OK");
	 //if (pEOF) lpMemo[strlen(lpMemo)-1]=26;
 }
 
 // -------------------------------------
 // Scrivo il file su disco             |
 // -------------------------------------

 if (fileCheck(FileINI)&&fRename)
 {
	for (a=0;a<20;a++)
	{
		fError=FALSE;
#ifdef _UNICODE
		if (MoveFile(FileINI,NewFile)) 
#else
		if (rename(FileINI,NewFile)) 
#endif
		{
			Sleep(500); 
			fError=TRUE;
			continue;
		} else break;
	}
	if (fError) ehExit("INISET:Errore in rinomina file %s > %s",FileINI,NewFile);
 }

  ZeroFill(sSA); sSA.nLength=sizeof(sSA);
  hdlIni= CreateFile((LPCTSTR) FileINI,
					 GENERIC_WRITE,
					 iProfileMode?FILE_SHARE_READ|FILE_SHARE_WRITE:0,//FILE_SHARE_READ,
					 NULL,//&sSA,
			         CREATE_ALWAYS,
					 FILE_FLAG_SEQUENTIAL_SCAN,
					 (HANDLE)NULL);
 
  if (hdlIni==(HANDLE) INVALID_HANDLE_VALUE) 
  {
	  SINT iErr=GetLastError();
#ifndef _USRDLL
	  osError(TRUE,iErr,"INISET:");
#endif

#ifdef _UNICODE
	  if (fRename) 
		{
		  DeleteFile(FileINI);  
	      MoveFile(NewFile,FileINI);
		}
#else
	  if (fRename) {remove(FileINI);  rename(NewFile,FileINI);}
#endif
	  //ehExit("INISET:Errore in OpenforWrite INI [%s]",FileINI);
  }


  for (iCount=0,pLF=lpMemory;*pLF;pLF++,iCount++);
  //win_infoarg("[%s]",lpMemo);
  if (!WriteFile(hdlIni,lpMemory,iCount,&dwRealRead,NULL)) 
  {
//	  if (fRename) {remove(FileINI);  rename(NewFile,FileINI);}
#ifdef _UNICODE
	  if (fRename) 
		{
		  DeleteFile(FileINI);  
	      MoveFile(NewFile,FileINI);
		}
#else
	  if (fRename) {remove(FileINI);  rename(NewFile,FileINI);}
#endif

	  ehExit("INISET:Errore in WriteFile INI ,%d",GetLastError());
  }
  if (!iProfileMode) FlushFileBuffers(hdlIni);
  CloseHandle(hdlIni);
  //if (!strcmp(lpVariabile,"CODEWS")) win_infoarg("%s : %s",FileINI,lpVariabile);

#ifdef _UNICODE
	if (fRename) 
		{
		  DeleteFile(FileINI);  
		}
#else
	if (fRename) {remove(FileINI);}
#endif

  //if (hdlMemo>-1) memoFree(hdlMemo,"INISet");
  ehFree(lpMemory);
  ehFree(lpVarName);
  LeaveCriticalSection(&csProfile);
}

#else
// -------------------------------------------------------------------------------------------------
// FSetProfileChar in DOS 16bit
//
// -------------------------------------------------------------------------------------------------
void FSetProfileChar(CHAR *file,CHAR *Var,CHAR *Valore)
{
 CHAR File[MAXPATH];
 LONG a;
 FILE *ch;
 CHAR *p;
 SINT Flag=OFF;
 _DMIInfo={-1,-1};
 CHAR Buf[120];
 SINT SizeBuf=sizeof(Buf);

 strcpy(File,file);
 LinkIni(File);
 Info.Hdl=-1;
 Info.Num=0;

 os_errset(OFF);
 if (!fileCheck(File)) {Flag=ON;
					  Info.Num=-1;
					  Info.Hdl=-1;
					  goto scrivi;
					}
 // -------------------------------------
 // Alloco il file in memoria           |
 // -------------------------------------

 Info.Max=0;
 if (!f_open(File,"rb",&ch))
 {
	while(TRUE)
	{
	 if (f_gets(ch,Buf,SizeBuf-1)) break;
	 //printf("%s",buf);
	 Info.Max++;
	}

 Info.Size=SizeBuf-1;
 Info.Hdl=memoAlloc(RAM_AUTO,Info.Size*Info.Max,"Memo");
 if (Info.Hdl<0) ehExit("SetProfile ?");

 fseek(ch,0,SEEK_SET);
 Info.Num=0;
 while(TRUE)
 {
	if (f_gets(ch,Buf,SizeBuf-1)) break;
	memoWrite(Info.Hdl,Info.Num*Info.Size,Buf,Info.Size);
	Info.Num++;
 }
// printf("InfoNum:%ld\n",Info.Num); getch();
	fclose(ch);

 }

 // -------------------------------------
 // Cambio il dato interessato          |
 // -------------------------------------

 Flag=ON;
 for (a=0;a<Info.Num;a++)
 {
	memoRead(Info.Hdl,a*Info.Size,Buf,Info.Size);
	//win_infoarg("%d) \nLeggo[%s]\nCerco[%s]",a,Buf,Var);
//	printf("%ld)InfoNum:%s\n",a,buf); getch();
	// Se lo trova
	if (memcmp(Buf,Var,strlen(Var))==0)
		 {p=strstr(Buf,"="); if (!p) break;
		  strcpy(p+1,Valore); strcat(p,"\n");
		  memoWrite(Info.Hdl,a*Info.Size,Buf,Info.Size);
	//	  printf("E che "); getch();
		  Flag=OFF;
		  break;
		 }
 }
//	printf("CI"); getch();

 // -------------------------------------
 // Scrivo il file su disco             |
 // -------------------------------------
 scrivi:
 if (!f_open(File,"wb",&ch))
 {
	for (a=0;a<Info.Num;a++)
	{
	memoRead(Info.Hdl,a*Info.Size,Buf,Info.Size);
	f_put(ch,NOSEEK,Buf,(WORD) strlen(Buf));
	}

	if (Flag) 
	{//f_put(ch,NOSEEK,Var,strlen(Var));
	 //f_put(ch,NOSEEK,"=",1);
	 //put(ch,NOSEEK,Valore,strlen(Valore));
     //f_put(ch,NOSEEK,"" CRLF,2);
	 fprintf(ch,"%s=%s\n",Var,Valore);
	 //win_infoarg("New [%s]=[%s]",Var,Valore);
	}
	if (fflush(ch)) win_info("INI error write");
	fclose(ch);
 }

 if (Info.Hdl!=-1) memoFree(Info.Hdl,"SetProfile");
 os_errset(POP);
}
#endif

#ifdef _WIN32
BOOL FSetProfileBin(TCHAR *file,CHAR *Var,void *Valore,SINT Len)
{
  BYTE *lpVar;
  CHAR Base[20];
  BYTE *Ptr;
  SINT a;
  lpVar=GlobalAlloc(GPTR,Len*2+10); 
  if (lpVar==NULL) ehExit("FGetProfileChar: no memory");
  //if (Str==NULL) return TRUE;
  *lpVar=0; Ptr=Valore;
  for (a=0;a<Len;a++)
  {
    sprintf(Base,"%02x",*Ptr); Ptr++;
	strcat(lpVar,Base);
  }

 //win_infoarg("%s=%s",Var,Str);
 FSetProfileChar(file,Var,lpVar);
 GlobalFree(lpVar);
 return FALSE;
}

BOOL FGetProfileBin(TCHAR *file,CHAR *Var,void *Valore,SINT Len)
{
    SINT rt;
    CHAR *Str;
	CHAR *Dest=Valore;
	CHAR Base[10];
	SINT a;
	SINT iLen=Len*2+10;

	if (!Valore) ehExit("GetProfileBin: [%s] Dest=NULL",Var);
	Str=GlobalAlloc(GPTR,iLen); if (Str==NULL) ehExit("FGetProfileBin(1)");
	rt=FGetProfileCharEx(file,Var,Str,&iLen);
    
	//win_infoarg("[%s]",Str);
	if (rt) {GlobalFree(Str); return TRUE;}
	if ((SINT) strlen(Str)>iLen) {win_infoarg("FGetProfileBin(2) [%s:%s]",file,Var); 
							 	 GlobalFree(Str);
								 return TRUE;
								}
    for (a=0;a<(SINT) strlen(Str)>>1;a++)
	{
	 memcpy(Base,Str+(a*2),2); Base[2]=0;
     //win_infoarg("[%s][%d]",Base,xtoi(Base));
     *Dest=(CHAR) xtoi(Base); Dest++;
	}
	GlobalFree(Str);
	//return TRUE;
	return FALSE;
}

void FGetProfileRect(TCHAR *file,CHAR *Var,RECT *Rect)
{
 CHAR Serv[80];
 CHAR *token;
 SINT rt,iLen=sizeof(Serv);
 rt=FGetProfileCharEx(file,Var,Serv,&iLen);
 if (rt) return; // Non c'è

 token=strtok(Serv,"|"); Rect->left=atoi(token);
 token=strtok(NULL,"|"); Rect->top=atoi(token);
 token=strtok(NULL,"|"); Rect->right=atoi(token);
 token=strtok(NULL,"|"); Rect->bottom=atoi(token);

}

void FSetProfileRect(TCHAR *file,CHAR *Var,RECT *Rect)
{
 CHAR Serv[80];
 sprintf(Serv,"%d|%d|%d|%d",Rect->left,Rect->top,Rect->right,Rect->bottom);
 FSetProfileChar(file,Var,Serv);
}

#ifndef _CONSOLE
#ifndef _WIN32_WCE
#ifndef _NODC
BOOL FGetProfileWP(TCHAR *file,CHAR *Var,BOOL fShow)
{
	WINDOWPLACEMENT WP;
	BOOL fRet=TRUE;
	if (!FGetProfileBin(file,Var,&WP,sizeof(WP))) 
	{
	 WP.length=sizeof(WP);
	 SetWindowPlacement(WindowNow(),&WP); 
	 fRet=FALSE; 
	}
	if (fShow) ShowWindow(WindowNow(),SW_SHOW);
	return fRet;
}

void FSetProfileWP(TCHAR *file,CHAR *Var)
{
	WINDOWPLACEMENT WP;
	WP.length=sizeof(WP);
	GetWindowPlacement(WindowNow(),&WP); 
	FSetProfileBin(file,Var,&WP,sizeof(WP));
}
#endif
#endif
#endif


#endif
