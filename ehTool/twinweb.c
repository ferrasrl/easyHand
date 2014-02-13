// ----------------------------------------------------
// | TWINWEB 
// | Api per l'elaborazione dei file TWP
// | Gestore di aggiornamento con sito Web
// | Crea un file package con le istruzioni,dati,dbase,file ed oggetti 
// | da trasferire in FTP al server Web
// |                                           
// |                                          
// |					 by Ferrà Art & Technology 2000
// |					 by Ferrà Art & Technology 2004
// ----------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/fbfile.h"
#include "/easyhand/ehtool/imgutil.h"

#include "/easyhand/ehtool/sdbdrive.h"
#include "/easyhand/ehtool/twinweb.h"

#define TWIWEB_MAX 2000

// Ritorna  0= Tutto OK
//         -1= Errore in lettura lunghezza file
//         -2= Errore lettura file

static CHAR *lpType[]={"VER",// TW_VERSION		Versione
					   "USR",// TW_USER			User/Password
				       "A_M",// TW_ADMINEMAIL	E-mail dell'associato
				       "F_M",// TW_FERRAEMAIL   E-mail admin Ferrà
				       "AIM",// TW_ADBIMPORT	Adb Import
				       "ADE",// TW_ADBDELETE	Adb Delete
				       "FDE",// TW_FILEDELETE   FileDelete
				       "FCP",// TW_FILECOPY		FileCopy
				       "PRO",// TW_PROCEDURE
				       "TWC",// TWC Copy      
				       "ADA",// TW_ADBDELALL	Adb Delete All
				       "ADX",// TW_ADBDELEX 	Adb Delete Extended
						"","",
				       // new 2004/2005
					   "FIL",// TW_FILEPRO 		File da processare
					   "CMD",// TW_COMMAND      Comando
					   "HID",// TW_HIDDEN      Comando
				       NULL};

static SINT LTWPAddFile(FILE *pf,CHAR *lpSorg,CHAR *lpName,SINT *lpiSize);
SINT TWServer(SINT cmd,LONG info,void *str)
{
	static _DMI TWInfo=DMIRESET;
	TW_OBJECT TWObject;
	TWP_OBJECT TWPObject;
	CHAR szFileName[MAXPATH];
	CHAR szServ[80];
	CHAR szBuffer[400];
	TWP_HEADER TWPHeader;
	FILE *pf;
	SINT a;
	SINT iSize;
	TW_OBJECT *lpTWObject;
	static SINT iEnum;
	switch (cmd)
	{
		// ---------------------------------
		// Apro il gestione TWINWEB
		// --------------------------------
		case WS_OPEN: 
			
			DMIOpen(&TWInfo,RAM_AUTO,TWIWEB_MAX,sizeof(TW_OBJECT),"TWServer");
			ZeroFill(TWObject);
			TWObject.iType=TW_VERSION;
			strcpy(TWObject.szCode,"1.5:Ferrà srl (C) 2005");
			iEnum=0;
			TWServer(WS_ADD,0,&TWObject);
			break;

		case WS_PROCESS: 

		    // -----------------------------------------
			// Creo il file "ASCII" LIST.TWC
			// Contiene l'elenco dei dati caricati
			//
				pf=fopen(str,"w"); if (!pf) ehExit("TW:Err1");
				for (a=0;a<TWInfo.Num;a++)
				{
					DMIRead(&TWInfo,a,&TWObject);
					TWObjectToAscii(szBuffer,&TWObject);
					fprintf(pf,"%s\n",szBuffer);

				}
				fclose(pf);
				break;

		case WS_CLOSE: 

			// il TWP viene strutturato in questo modo
			// VERSIONE
			// * HEADER
			// - Nome del file 
			// - Dimensioni
			// FILE
			if (str)
			{
				pf=fopen(str,"wb"); if (!pf) ehExit("TW:Err1 [s]",str);
				ZeroFill(TWPHeader);
				strcpy(TWPHeader.szVer,"TWP1.0");
				fwrite(&TWPHeader,sizeof(TWPHeader),1,pf);
				TWPHeader.iSize=sizeof(TWPHeader);
				
				for (a=0;a<TWInfo.Num;a++)
				{
					DMIRead(&TWInfo,a,&TWObject);
					ZeroFill(TWPObject);

					switch (TWObject.iType)
					{
						case TW_ADBIMPORT:
							//LTWPAddFile(pf,TWObject.szFile,fileName(TWObject.szFile));
							sprintf(szFileName,"%s.bti",TWObject.szCode);
							sprintf(szServ,"%s.bti",TWObject.szNewFileName);
							LTWPAddFile(pf,szFileName,szServ,&iSize);
							TWPHeader.iFile++;
							TWPHeader.iSize+=iSize;
							
							sprintf(szFileName,"%s.adb",TWObject.szCode);
							sprintf(szServ,"%s.adb",TWObject.szNewFileName);
							LTWPAddFile(pf,szFileName,szServ,&iSize);
							TWPHeader.iFile++;
							TWPHeader.iSize+=iSize;
							break;

						case TW_FILEPRO:
						case TW_FILECOPY:
						case TW_TWCCOPY:
		
							//win_infoarg(">[%s][%s]",TWObject.szCode,TWObject.szFile);
							LTWPAddFile(pf,
										TWObject.szCode, // Sorgente
										TWObject.szNewFileName, // Nuovo Nome
										&iSize);
							TWPHeader.iFile++;
							TWPHeader.iSize+=iSize;
							break;
					}
				}

				// ---------------------------------------------
				// Chiudo il file
				ZeroFill(TWPObject);
				fwrite(&TWPObject,sizeof(TWPObject),1,pf);
				TWPHeader.iSize+=sizeof(TWPObject);

				// Scrivo il numero di file presenti
				fseek(pf,0,SEEK_SET);
				fwrite(&TWPHeader,sizeof(TWPHeader),1,pf);
				fclose(pf);
			}
			
			DMIClose(&TWInfo,"TWServer");
			if (str) return TWPHeader.iFile;
			break;

		case WS_COUNT: // Restituisce il numero di tabelle presenti
			return TWInfo.Num;
			//break;

		case WS_REALGET: // Legge i dati di una tabella
			DMIRead(&TWInfo,info,str);
			break;

		case WS_REALSET: // Legge i dati di una tabella
			DMIWrite(&TWInfo,info,str);
			break;
		
		case WS_DEL:
			DMIDelete(&TWInfo,info,&TWObject);
			break;

		case WS_INSERT:
		
			if (info<TWInfo.Num) 
			{
				DMIInsert(&TWInfo,info,&TWObject);
				DMIWrite(&TWInfo,info,str);
			}
			else
			{
				DMIAppend(&TWInfo,str);
			}
			break;
		
		case WS_ADD:
			lpTWObject=str;
			switch (lpTWObject->iType)
			{
				case TW_ADBIMPORT:
					sprintf(lpTWObject->szNewFileName,"ADB%04d",iEnum++);
					break;
				
				case TW_FILECOPY:
					sprintf(lpTWObject->szNewFileName,"FILE%04d",iEnum++);
					break;

				case TW_FILEPRO:
					sprintf(lpTWObject->szNewFileName,"FILP%04d",iEnum++);
					break;

				case TW_TWCCOPY:
					strcpy(lpTWObject->szNewFileName,"LIST.TWC");
					break;
			}
			DMIAppend(&TWInfo,str);
			break;
		
		case WS_FIND: // Ricerca
			return -1;

		// --------------------------------------------
		// Decomprimo in file TWP
		//
		
		case WS_DO:
			break;
	}
  return 0;
}

static SINT LTWPAddFile(FILE *pf, // pf -> TWP
						CHAR *lpSorg,// File originale da includere
						CHAR *lpNewFileName,// Nuovo Nome del file da includere
						SINT *lpiSize)
{
  TWP_OBJECT TWPObject;
  SINT Hdl;
  BYTE *lp;

  ZeroFill(TWPObject);
  TWPObject.iFileLen=fileSize(lpSorg); if (TWPObject.iFileLen<1) return -1;
  
  TWPObject.iNameLen=strlen(lpNewFileName);
  fwrite(&TWPObject,sizeof(TWPObject),1,pf);
  fwrite(lpNewFileName,strlen(lpNewFileName),1,pf);
  
  Hdl=fileLoad(lpSorg,RAM_AUTO); if (Hdl<0) return -2;
  lp=memoLock(Hdl);
  fwrite(lp,TWPObject.iFileLen,1,pf);
  memoUnlock(Hdl);
  memoFree(Hdl,"File");
  *lpiSize=sizeof(TWPObject)+TWPObject.iFileLen+strlen(lpNewFileName);
  return 0;
}

//
// twpFreeResource()
//
BOOL twpFreeResource(TWP_EXPAND * psTwp) {

	CHAR * psz;
	for (lstLoop(psTwp->lstFiles,psz)) {
//		printf("rimuovo %s ..." CRLF,psz);
		fileRemove(psz);
	}
	psTwp->lstFiles=lstDestroy(psTwp->lstFiles);
	return false;
}

// --------------------------------------------------------------
// TWPExpand()
// Scompatta,espande,cancella,interpreta un file TWP/TWC
// --------------------------------------------------------------
CHAR * twpExpand(TWP_EXPAND * psTwp)
{

	FILE *	chTwp; // File in lettura
	FILE *	pfw; // File in scrittura
	SINT Hdl;
	TWP_HEADER TWPHeader;
	TWP_OBJECT TWPObject;
	CHAR *lpFileSource;
	CHAR *lpFileMemo;
	CHAR szFileName[500];
//	static CHAR szError[500];

	psTwp->lstFiles=lstNew();
	// ------------------------------------------------------------------------------------------
	// A) Unpackaging del file "enumerati" nel Folder temporaneo
	//
	//
	
	chTwp=fopen(psTwp->szTWPFile,"rb");	if (chTwp==NULL) return "TWP:Errore in apertura file";
			
	// Leggo l'header
	if (fread(&TWPHeader,sizeof(TWPHeader),1,chTwp)!=1) 
		{fclose(chTwp); return "TWP:Errore Header";}

	// Loop sugli oggetti
	while (TRUE)
	{
		if (fread(&TWPObject,sizeof(TWPObject),1,chTwp)!=1) 
			{
				fclose(chTwp); 
				return "TWP:Errore Object read";
			}
			
		// ------------------------------------------------------------------------------------------
		// END-TAG
		if (!TWPObject.iNameLen) break;

		lpFileSource=ehAlloc(TWPObject.iNameLen+1);
		memset(lpFileSource,0,TWPObject.iNameLen+1);

		if (fread(lpFileSource,TWPObject.iNameLen,1,chTwp)!=1) 
			{ehFree(lpFileSource);
			 fclose(chTwp); 
			 return "TWP:Errore Lettura FILENAME/object";
			} // File Name errato

		// ------------------------------------------------------------------------------------------
		// Creo il file name completo della destinazione
		sprintf(szFileName,"%s/%s",psTwp->szTempFolder,fileName(lpFileSource)); strSlash(szFileName,OS_DIR_SLASH);
//		if (!fileCheck(szFileName)) ehError();
		//printf("%s",szFileName); _getch();

		// ------------------------------------------------------------------------------------------
		// Chiedo memoria per allocare il file (in memoria)
		Hdl=memoAlloc(RAM_AUTO,TWPObject.iFileLen,"File");
		lpFileMemo=memoLock(Hdl);

		// 
		// Leggo il file dal TWP
		//
		if (fread(lpFileMemo,TWPObject.iFileLen,1,chTwp)!=1)
		{
			ehFree(lpFileSource);
			memoFree(Hdl,"File");
			fclose(chTwp); 
			return "TWP:File di lunghezza errata";
		}

		// Apro il file in scrittura
		pfw=fopen(szFileName,"wb");
		if (pfw==NULL) 
		{
			ehFree(lpFileSource);
			memoFree(Hdl,"File");
			fclose(chTwp); 
			sprintf(psTwp->szError,"TWP:Non posso aprire il file in scrittura [%s]",szFileName);
			return psTwp->szError;
		}

		// Scrivo il file su disco
		if (fwrite(lpFileMemo,TWPObject.iFileLen,1,pfw)!=1)
		{
			ehFree(lpFileSource);
			memoFree(Hdl,"File");
			fclose(chTwp); 
			sprintf(psTwp->szError,"TWP:Non ho potuto scrivere il file su disco");
			return psTwp->szError;
		}
		fclose(pfw);
		memoUnlock(Hdl);
		memoFree(Hdl,"File");
		ehFree(lpFileSource);

		lstPush(psTwp->lstFiles,szFileName);
	}
   fclose(chTwp);		
//   if (!fileCheck(psTwp->szTWPFile)) ehError();

   return NULL;
}

// ---------------------------------------------------------
//  Traduce una stringa ASCII in un oggetto TW_OBJECT 
//  ritorna il tipo di oggetto 
//  -1= Che cazzo è ?
//

SINT TWAsciiToObject(CHAR *lpBuffer,TW_OBJECT *lpTWObject)
{
 unsigned char *lp;
 CHAR *p;
 CHAR *lpCmd;
 CHAR *lpDati;
 SINT a,iCmd;

 for (lp=lpBuffer;*lp;lp++) if (*lp<' ') *lp=0;
 lpBuffer[3]=0;
 lpCmd=lpBuffer;
 lpDati=lpBuffer+4;

 iCmd=-1;
 for (a=0;;a++)
	{
	 if (lpType[a]==NULL) break;
	 if (!strcmp(lpType[a],lpCmd)) {iCmd=a;break;}
	}

 memset(lpTWObject,0,sizeof(TW_OBJECT));

 switch (iCmd)
 {
	case TW_VERSION:
	case TW_PROCEDURE:
	case TW_USER:
	case TW_ADMINEMAIL:
	case TW_FERRAEMAIL:
		strcpy(lpTWObject->szCode,lpDati);
		break;

	case TW_ADBIMPORT:
		p=strstr(lpDati,"|"); if (p) *p=0;
		strcpy(lpTWObject->szNewFileName,lpDati);
		strcpy(lpTWObject->szCode,p+1);
		break;

	// sprintf(TWObject.szCode,"PERCOD$%s",adb_FldPtr(HdbOld,"CODICE"));
	// szFile=
	// szCode=<NomeIndice> $ A<CampoRicerca>|N<CampoRicerca>....

	case TW_ADBDELETE:
	case TW_ADBHIDDEN:
		p=strstr(lpDati,"#"); if (p) *p=0;
		strcpy(lpTWObject->szFile,lpDati);
		strcpy(lpTWObject->szCode,p+1);
		break;

	// sprintf(TWObject.szCode,"PERCOD$%s",adb_FldPtr(HdbOld,"CODICE"));
	// szCode=<NomeIndice> $ A<CampoRicerca>|N<CampoRicerca>....
    // DA  FARE
	case TW_ADBDELEX:
		p=strstr(lpDati,"#"); if (p) *p=0;
		strcpy(lpTWObject->szFile,lpDati);
		strcpy(lpTWObject->szCode,p+1);
		break;

	case TW_ADBDELALL:
		strcpy(lpTWObject->szFile,lpDati);
		break;

	case TW_FILEDELETE:
		strcpy(lpTWObject->szFile,lpDati);
		break;

   case TW_FILECOPY:
   case TW_TWCCOPY:
		p=strstr(lpDati,"|"); if (p) *p=0;
		strcpy(lpTWObject->szNewFileName,lpDati);
		strcpy(lpTWObject->szFile,p+1);
		break;

	case TW_FILEPRO:
		//sprintf(lpBuffer,"%s:%s|%s",lpAsc,lpTWObject->szCode,lpTWObject->szFile);
		p = strtok(lpDati, "|");
		a=0;
		while(p)
		{
			switch (a)
			{
				case 0: strcpy(lpTWObject->szNewFileName,p); break;
				case 1: strcpy(lpTWObject->szCode,p); break;
				case 2: strcpy(lpTWObject->szFile,p); break;
			}
			a++;
			p = strtok( NULL, "|" );
		}
		break;
			
	case TW_COMMAND:
		//sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szCode);
		strcpy(lpTWObject->szCode,lpDati);
		break;


 }
 return iCmd;
}

// ---------------------------------------------------------
//  Traduce una stringa ASCII in un oggetto TW_OBJECT 
//  ritorna il tipo di oggetto 
//  -1= Che cazzo è ?
//

void TWObjectToAscii(CHAR *lpBuffer,TW_OBJECT *lpTWObject)
{
	CHAR *lpAsc;
	lpAsc=lpType[lpTWObject->iType];
	
	
	switch (lpTWObject->iType)
	{
		case TW_VERSION:
			sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szCode);
			break;

		case TW_PROCEDURE:
			sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szCode);
			break;

		case TW_USER:
			sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szCode);
			break;

		case TW_ADMINEMAIL:
			sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szCode);
			break;

		case TW_FERRAEMAIL:
			sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szCode);
			break;

		case TW_ADBIMPORT:
			sprintf(lpBuffer,"%s:%s|%s",lpAsc,lpTWObject->szNewFileName,lpTWObject->szFile);
			break;

		case TW_ADBDELETE:
		case TW_ADBDELEX:
		case TW_ADBHIDDEN:
			sprintf(lpBuffer,"%s:%s#%s",lpAsc,lpTWObject->szFile,lpTWObject->szCode);
			break;

		case TW_ADBDELALL:
			sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szFile);
			break;

		case TW_FILEDELETE:
			sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szFile);
			break;

		case TW_FILECOPY:
		case TW_TWCCOPY:
			sprintf(lpBuffer,"%s:%s|%s",lpAsc,lpTWObject->szNewFileName,lpTWObject->szFile);
			break;

		case TW_FILEPRO:
			sprintf(lpBuffer,"%s:%s|%s|%s",lpAsc,lpTWObject->szNewFileName,lpTWObject->szCode,lpTWObject->szFile);
			break;
			
		case TW_COMMAND:
			sprintf(lpBuffer,"%s:%s",lpAsc,lpTWObject->szCode);
			break;

		default:
			ehExit("TWP:Comando errato");
	}
}


