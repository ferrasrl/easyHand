//
// ehcCloud - Ferra Clouding ######################################################
//

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehCgi.h"
#include "/easyhand/inc/ehCloud.h"
#include <fcntl.h>

CHAR * pszCryptKey="cloudFerra2012";

#ifdef EH_INTERNET
static const char* _myZlibVersion = ZLIB_VERSION;
static void _ehcDo(EH_CLOUD * psCloud,CHAR *pszFormat,...);
// BYTE * ehcWebUnzip(EH_WEB * psWeb,ULONG * pulSize);

//
// ehcOpen()
//
BOOL ehcOpen(EH_CLOUD * psCloud,CHAR * pszCloud,CHAR * pszUser,CHAR * pszPassword) {

	CHAR szServ[1024];
	EH_WEB * psWeb;
	CHAR * pszKey;
//	CHAR * p;
	BOOL bRet=true;
	memset(psCloud,0,sizeof(EH_CLOUD));
	strCpy(psCloud->szUser,pszUser,sizeof(psCloud->szUser));
	strCpy(psCloud->szPassword,pszPassword,sizeof(psCloud->szPassword));

#ifdef _DEBUG
	//sprintf(psCloud->szUrl,"http://%s/ehCloud.fgi",pszCloud);
	if (strEmpty(pszCloud)) pszCloud="cloud.ferra.com";
//	if (strEmpty(pszCloud)) pszCloud="www.ferra.com";
//	sprintf(psCloud->szUrl,"http://%s/cgi-bin/cloudGate.exe",pszCloud);
#else
	if (strEmpty(pszCloud)) pszCloud="cloud.ferra.com";
//	sprintf(psCloud->szUrl,"http://%s/ehCloud.fgi",pszCloud);
#endif
	sprintf(psCloud->szUrl,"http://%s/ehCloud.fgi",pszCloud);
	/*
#ifdef _DEBUG
//	sprintf(psCloud->szUrl,"http://%s/cgi-bin/_cloudGate_d.exe",pszCloud);
#else
	sprintf(psCloud->szUrl,"http://%s/cgi-bin/cloudGate.exe",pszCloud);
#endif
	*/

	strCpy(psCloud->szCloud,pszCloud,sizeof(psCloud->szCloud));

	if (strEmpty(psCloud->szUser)||strEmpty(psCloud->szPassword)) return true;
	
	sprintf(szServ,"%s|%s",psCloud->szUser,psCloud->szPassword);
	psCloud->pszKey=strEncrypt(szServ,pszCryptKey);
//	p=strDecrypt(psCloud->pszKey,pszCryptKey);

	// Nuovo sistema sincrono
	pszKey=strEnc(1,psCloud->pszKey,SE_URL,NULL);
	sprintf(szServ,"%s?r=login&k=%s",psCloud->szUrl,pszKey);
	ehFree(pszKey);
	/*
#ifdef _DEBUG
	ClipboardPut("%s",szServ);
	alert(szServ);
#endif
	*/
	ehPrintd("%Req:%s ...",szServ);
	psWeb=webHttpReq(szServ,"GET",NULL,false,0);
	ehPrintd("ok [%.2f]" CRLF,psWeb->dElapsedTime);

	if (!psWeb->sError.enCode) { // Leggi pagina caricata 
/*
#ifdef _DEBUG
		alert("[%s][%s]",psWeb->pPageHeader,psWeb->pData);
#endif
*/
		if (!strBegin(psWeb->pData,"KEY:")) {

			CHAR * psz;
			EH_ARF ar;
			psCloud->pszKeyId_flat=strExtract(psWeb->pData,"KEY:","\n",false,false);
			psCloud->pszKeyId_encoded=strEncode(psCloud->pszKeyId_flat,SE_URL,NULL);
			psz=strDecrypt(psCloud->pszKeyId_flat,pszCryptKey);
			//pid=strExtract(psz,NULL,"|",false,false); ehFree(psz);
			ar=ARFSplit(psz,"|");
			psCloud->idUser=atoi(ar[0]);
			strcpy(psCloud->szCodex,ar[2]);
			//ehFree(pid);
			ehFree(ar);

			psCloud->bLogin=true;
			bRet=false;

		}
	}
#ifdef _DEBUG
	else
	{
		alert("Error: %d",psWeb->sError.enCode);
	}
#endif
	webHttpReqFree(psWeb);
	if (bRet) ehcClose(psCloud);
	return bRet;

}

//
// ehcNotify()
//
void ehcNotify(EH_CLOUD * psCloud,void * (*funcNotify)(EH_SRVPARAMS)) {
	
	psCloud->funcNotify=funcNotify;

}
//
// ehcClose()
//
BOOL ehcClose(EH_CLOUD * psCloud) {

	CHAR szUrl[1024];
	EH_WEB * psWeb;
	BOOL bRet=true;

	if (psCloud->bLogin&&psCloud->pszKeyId_encoded) {

		sprintf(szUrl,"%s?r=logout&kid=%s",psCloud->szUrl,psCloud->pszKeyId_encoded);
		psWeb=webHttpReq(szUrl,"GET",NULL,FALSE,0);
		if (!psWeb->sError.enCode) { // Leggi pagina caricata 
			if (!strCmp(psWeb->pData,"OK")) bRet=false;
		}
		webHttpReqFree(psWeb);
	}
	ehFreePtrs(3,&psCloud->pszKey,&psCloud->pszKeyId_flat,&psCloud->pszKeyId_encoded);
	memset(psCloud,0,sizeof(EH_CLOUD));
	return bRet;
}

//
// _fileSync()
//
static INT _fileSync(EH_CLOUD * psCloud,EH_FILEDIR * psDir,CHAR * pb,EH_LST lstInfo) {
	
	INT a,b;
	EH_ARF arRemoteFile;
	S_FILEINFO * psFile;
	INT idx;
	INT iRet=0;
	CHAR * pszFolderBegin;
	CHAR szFileName[1024];

	arRemoteFile=ARFSplit(pb,"\n");
	psFile=DMILock(&psDir->dmiFiles,NULL);
	pszFolderBegin=strExtract(arRemoteFile[0],"DIR:","|",false,false);

	if (!pszFolderBegin) {ehFree(arRemoteFile); return true;}

	//
	// Loop sui file remoti
	// 
	for (a=1;arRemoteFile[a];a++) {

		EH_ARF arFld=ARFSplit(arRemoteFile[a],"|");

		//
		// Controllo se ho il file remoto
		//
		idx=-1;
		for (b=0;b<psDir->dmiFiles.Num;b++) {
			if (!psFile[b].pszFullPath) continue;
			if (!strCaseCmp(psFile[b].pszFullPath,arFld[0])) {idx=b; break;}
		}
		//
		// Controllo se il il file è cambiato (in locale o in remoto)
		//
		if (idx>-1) {

			TIME64 tTimeWriteRemote=ehAtoi64(arFld[2]);

			//
			// Il file non ha modifiche, lo tolgo dalle liste
			//
			if (tTimeWriteRemote==psFile[idx].sFd.tTimeWrite) {

				*arRemoteFile[a]=0;
				strAssign(&psFile[idx].pszFullPath,NULL);

			//
			// Aggiungo da trasferire ai remoti
			//
			} else if (tTimeWriteRemote<psFile[idx].sFd.tTimeWrite) {

				*arRemoteFile[a]=0;

			}
			//
			// Aggiungo da ricevere ai locali
			//
			else {

				strAssign(&psFile[idx].pszFullPath,NULL);

			}



		}
		ehFree(arFld);
	
	}

	//
	// <-- Richiedo tutti i file remoti che non ho oppure sono cambiati
	//
	for (a=1;arRemoteFile[a];a++) {
		if (!strEmpty(arRemoteFile[a])) {
			EH_ARF arFld=ARFSplit(arRemoteFile[a],"|");
			strcpy(szFileName,pszFolderBegin); AddSl(szFileName); strcat(szFileName,arFld[0]); strSlash(szFileName,OS_DIR_SLASH);
			iRet|=ehcFileDownload(psCloud,szFileName,lstInfo);
			ehFree(arFld);
			
		}
	}

	//
	// --> Trasmetto tutti i file locali che sono da traferire in remoto
	//
	for (b=0;b<psDir->dmiFiles.Num;b++) {
		if (!strEmpty(psFile[b].pszFullPath)) {

			strcpy(szFileName,pszFolderBegin); AddSl(szFileName); strcat(szFileName,psFile[b].pszFullPath); strSlash(szFileName,OS_DIR_SLASH);
			iRet|=ehcFileUpload(psCloud,szFileName,lstInfo);
		
		}
	}

	ehFree(arRemoteFile);
	ehFree(pszFolderBegin);

	return iRet; // TRUE se qualcosa è andato storto
}

//
// ehcFileSync() - Ritorna TRUE se ci sono errori
//
BOOL ehcFileSync(EH_CLOUD * psCloud,
				 CHAR * pszFolderCloud,
				 CHAR * pszRemoteFolder,
				 BOOL bRicorsive,
				 EH_LST lstInfo) {

	CHAR szUrl[1024];
	EH_WEB * psWeb;
	BOOL bRet=true;
	CHAR * pszRemote;
	DWORD dwParam;
	CHAR szFileScan[1024];
	EH_FILEDIR * psDir;

	if (!psCloud->bLogin||!psCloud->pszKeyId_flat) return bRet;
	if (!fileCheck(pszFolderCloud)) return bRet; // Non esiste il file folder locale
	strcpy(psCloud->szLocalFolder,pszFolderCloud); strSlash(psCloud->szLocalFolder,OS_DIR_SLASH);
	
	_ehcDo(psCloud,"Sync start.");

	//
	// Estraggo i file locali
	//
	dwParam=FDE_DMIMODE|FDE_RELAPATH;
	if (bRicorsive) dwParam|=FDE_SUBFOLDER;
	strcpy(szFileScan,pszFolderCloud); AddSl(szFileScan); 
	if (!strEmpty(pszRemoteFolder)) strcat(szFileScan,pszRemoteFolder);
	AddSl(szFileScan); strSlash(szFileScan,OS_DIR_SLASH);
	strcat(szFileScan,"*.*");
	psDir=fileDirCreate(szFileScan,NULL,dwParam,NULL);

	//
	// Richiedo il files remoti
	//
	pszRemote=pszRemoteFolder?strEncode(pszRemoteFolder,SE_URL,NULL):strDup("");
	sprintf(szUrl,"%s?r=sync&kid=%s&fld=%s&ric=%d",psCloud->szUrl,psCloud->pszKeyId_encoded,pszRemote,bRicorsive);
	ehFree(pszRemote);
	_ehcDo(psCloud,"Sync Remote request.");

	psWeb=webHttpReq(szUrl,"GET",NULL,FALSE,0);
	if (!psWeb->sError.enCode) { // Leggi pagina caricata 

		bRet=_fileSync(psCloud,psDir,psWeb->pData,lstInfo);
	}
	webHttpReqFree(psWeb);
	fileDirDestroy(psDir);
	_ehcDo(psCloud,"Sync stop.");
	
	return bRet;
}

static INT64 _getIntValue(CHAR * pszSource,CHAR * pszName) {

	CHAR szServ[80];
	CHAR * psz;
	INT64 iValue;
	sprintf(szServ,"%s:",pszName);
	psz=strExtract(pszSource,szServ,"|",false,false); 
	if (!psz) return 0;
	iValue=ehAtoi64(psz);
	ehFree(psz);
	return iValue;
	
}


//
// ehcFileDownload()
//
BOOL ehcFileDownload(EH_CLOUD * psCloud,CHAR * pszFileName,EH_LST lstInfo) {
	
	EH_WEB * psWeb;
	CHAR szUrl[1024],*psz;
	BOOL bRet=true;
	CHAR * pszFile;
//	INT iHeader;
	TIME64 tWrite;

	if (!psCloud->bLogin||!psCloud->pszKeyId_flat) return bRet;
//	if (!fileCheck(pszLocalFolder)) return bRet; // Non esiste il file folder locale

	pszFile=strEncode(pszFileName,SE_URL,NULL);
	sprintf(szUrl,"%s?r=get&kid=%s&fn=%s",psCloud->szUrl,psCloud->pszKeyId_encoded,pszFile);
	ehFree(pszFile);
	_ehcDo(psCloud,"download %s ...",pszFileName);

	ehPrintd("download %s ...",pszFileName);
	psWeb=webHttpReq(szUrl,"GET",NULL,FALSE,0);
	ehPrintd("%.2fs" CRLF,psWeb->dElapsedTime);
	if (!psWeb->sError.enCode) { // Leggi pagina caricata 

		sprintf(szUrl,"%s\\%s",psCloud->szLocalFolder,pszFileName); strSlash(szUrl,OS_DIR_SLASH);
		dirCreateFromFile(szUrl);
		fileMemoWrite(szUrl,psWeb->pData,psWeb->dwDataLength);

		psz=strExtract(psWeb->pPageHeader,"Content-Date: ",NULL,true,false);
		if (psz) {
			tWrite=ehAtoi64(psz);
			fileTimeSet(szUrl,tWrite,tWrite,tWrite);
			ehFree(psz);
			if (lstInfo) lstPushf(lstInfo,"download:%s",pszFileName);
		} else ehError();
		bRet=false;

	} else {
		if (lstInfo) lstPushf(lstInfo,"download:%s :error %s",pszFileName,psWeb->sError.szDesc);
	}
	webHttpReqFree(psWeb);
	if (!bRet) 
		_ehcDo(psCloud,"download %s completed.",pszFileName);
		else
		_ehcDo(psCloud,"download %s error!",pszFileName);

	return bRet;
}

//
// ehcFileUpload()
//
INT ehcFileUpload(EH_CLOUD * psCloud,CHAR * pszFileName,EH_LST lstInfo) {

	BOOL iRet=12;
	CHAR szFileSource[1024];
	CHAR szServ[1024];
	SIZE_T tFileSize;
	ULONG ulSize;
	BYTE *pbFileSource, * pbFileCompress;
	INT iError;
	TIME64 tmWrite;
	EH_WEB * psWeb;

	if (!psCloud->pszKeyId_encoded) return iRet;
	if (zlibVersion()[0] != _myZlibVersion[0]) ehExit("#incompatible zlib version\n");

	_ehcDo(psCloud,"upload %s ...",pszFileName);

	sprintf(szFileSource,"%s/%s",psCloud->szLocalFolder,pszFileName); 
	strSlash(szFileSource,OS_DIR_SLASH);

	if (!fileCheck(szFileSource)) {

		if (lstInfo) lstPushf(lstInfo,"upload: %s file not found",szFileSource);
		ehLogWrite("#500:file not found");
		iRet=1; 
		goto FINE;

	}

	pbFileSource=fileMemoRead(szFileSource,&tFileSize);
	if (!pbFileSource) {
	
		ehLogWrite("#501:file not read in memory");
		if (lstInfo) lstPushf(lstInfo,"upload: %s not memory",szFileSource);
		iRet=2; 
		goto FINE;

	}
	fileTimeGet(szFileSource,NULL,NULL,&tmWrite);

	ulSize=tFileSize+(tFileSize/10)+12;
	pbFileCompress=ehAllocZero(ulSize);

	iError=compress2(pbFileCompress,	// Destinazione in memoria
					 &ulSize,		// Dimensione di memoria da usare
					 pbFileSource,	// Puntatore al file originale
					 tFileSize,		// 
					 Z_BEST_COMPRESSION);	// Livello di compressione 9!!
	ehFree(pbFileSource);
	if (iError==Z_OK) 
	{
		EH_WEB sWeb;
		EH_WEB_MP sMP;

		_(sWeb);
		strcpy(sWeb.szReq,"POST");
		sWeb.lpUri=psCloud->szUrl;

		ZeroFill(sMP); 
		strcpy(sMP.szName,"r");
		sMP.lpData="put"; 
		sMP.ulSize=strlen(sMP.lpData);
		webAddMultiPart(&sWeb,&sMP);

		ZeroFill(sMP); 
		strcpy(sMP.szName,"kid");
		sMP.lpData=psCloud->pszKeyId_flat; 

		sMP.ulSize=strlen(sMP.lpData);
		webAddMultiPart(&sWeb,&sMP);

		ZeroFill(sMP); // File data compresso
		sMP.iType=1; // Non allocare data
		strcpy(sMP.szName,"fd");
		sMP.lpFileName=pszFileName;
		sMP.lpContentTransferEncoding="binary";
		sMP.lpData=pbFileCompress; // Lo libererà webFree()
		sMP.ulSize=ulSize;
		webAddMultiPart(&sWeb,&sMP);

		ZeroFill(sMP); // Nome del file (potrei anche indicarlo nel contesto sopra ma non c'ho voglia!!)
		strcpy(sMP.szName,"fn");
		sMP.lpData=pszFileName; 
		sMP.ulSize=strlen(sMP.lpData);
		webAddMultiPart(&sWeb,&sMP);

		ZeroFill(sMP);  // Dimensioni
		strcpy(sMP.szName,"s"); sprintf(szServ,"%d",tFileSize); 
		sMP.lpData=szServ; 
		sMP.ulSize=strlen(sMP.lpData);
		webAddMultiPart(&sWeb,&sMP);

		ZeroFill(sMP);  // Dimensioni
		strcpy(sMP.szName,"tm"); sprintf(szServ,"%I64d",tmWrite); 
		sMP.lpData=szServ; 
		sMP.ulSize=strlen(sMP.lpData);
		webAddMultiPart(&sWeb,&sMP);

		sWeb.iKeepAlive=true;
		sWeb.iTimeoutSec=60;
		psWeb=webHttpReqEx(&sWeb,false);
		if (!psWeb->sError.enCode) {
		
			if (!strBegin(psWeb->pData,"OK")) 
			{
				iRet=0; 
				if (lstInfo) lstPushf(lstInfo,"upload: %s",szFileSource);
			}
			else {
			
				iRet=20; 
				if (lstInfo) lstPushf(lstInfo,"upload: %s : cloud error ret: %s",szFileSource,psWeb->pData);
			}
			
		} else 
		{
			iRet=10;
			if (lstInfo) lstPushf(lstInfo,"upload: %s : %s",szFileSource,psWeb->sError.szDesc);
		}
		webHttpReqFree(psWeb);

	}
	else {

		if (lstInfo) lstPushf(lstInfo,"upload:%s : compress error",szFileSource);
		ehLogWrite("#502:compressor error");
		iRet=3;

	}


FINE:
	if (!iRet) 
		_ehcDo(psCloud,"upload %s completed.",pszFileName);
		else
		_ehcDo(psCloud,"upload %s error (#%d) !",pszFileName,iRet);

	return iRet;

}


//
// ATTENZIONE
// Non gestisce il trasferimento di file superiori a 4Gb ;-) (non è una battuta ... è vero!!)
//

//
// ehcWebZip()
//
BOOL ehcWebZip(CHAR * pszFileName,BYTE * pbDataSource,ULONG ulLength) {

	BYTE * pbDataCompress;
	INT iError;
	ULONG ulSize;
	BOOL bRet=true;

	if (zlibVersion()[0] != _myZlibVersion[0]) ehExit("#incompatible zlib version\n");

	pbDataCompress=ehAlloc(ulLength);
	ulSize=compressBound(ulLength);//uulLength+(ulLength/100)+12;
//	if (ulSize<2048) ulSize=2048;

	iError=compress2(pbDataCompress,	// Destinazione in memoria
					 &ulSize,		// Dimensione di memoria da usare
					 pbDataSource,	// Puntatore al file originale
					 ulLength,		// 
					 Z_BEST_COMPRESSION);	// Livello di compressione 9!!
// 	ehLogWrite("> %s, %d, %d",pszFileName,iError,ulSize);
	if (iError==Z_OK) {

		_setmode( _fileno(stdout), _O_BINARY );
		printf("Content-Type: application/zip" CRLF "Content-Encoding: gzip" CRLF );
		printf("Content-Length: %d" CRLF,ulSize);//+2);
		printf("Content-Length-Flat: %d" CRLF,ulLength);
		printf(CRLF);
		fwrite(pbDataCompress,ulSize,1,stdout); fflush(stdout);
	//	printf(CRLF);
		bRet=false;

	}
	else {

		ehLogWrite("[%s] compression error %d",pszFileName,iError);
//		printf("? [%s] compression error %d",iError);
		bRet=true;

	}
	ehFree(pbDataCompress);		// Libero il compresso
	return bRet;
}

//
// ehcWebUnzip()
//
BYTE * ehcWebUnzip(EH_WEB * psWeb,ULONG * pulSize) {

	ULONG ulSize;
	INT iError;
	BYTE * pszRet;
	CHAR * psz;

	if (!psWeb->iDataReaded) 
		return NULL;

	psz=strExtract(psWeb->pPageHeader,"Content-Length-Flat: ","\r",false,false);	
	if (!psz) return NULL;
	ulSize=atoi(psz);//compressBound(psWeb->iDataSize);
	ehFree(psz);

	pszRet=ehAllocZero(ulSize+1);
	iError=uncompress(pszRet, // Destinazione
					  &ulSize, 
					  psWeb->pData, // Sorgente
					  (UINT) psWeb->iDataReaded);
	if (iError!=Z_OK)
	{
		
		switch (iError) {

			case Z_MEM_ERROR:	printf("?:Errore in decompressione:Z_MEM_ERROR %ld",ulSize); break;
			case Z_BUF_ERROR:	printf("?:Errore in decompressione:Z_BUF_ERROR %ld",ulSize); break;
			case Z_DATA_ERROR:	printf("?:Errore in decompressione: Z_DATA_ERROR"); break; 
			default:			printf("?:Errore in decompressione: Z_ERROR: %d",iError); break;
		}
		ehFreePtr(&pszRet);

	} else {
		pszRet[ulSize]=0;
		if (pulSize) *pulSize=ulSize;

	}
	return pszRet;
}




//
// ehcFileUpdate() = copia da cloud --> locale
//
// ritorna
// 0=Tutto OK
// bit 1 = uno o più files indicati non sono presenti (non trovati sulla nuvola)
// bit 2 = Errore Web
//
INT ehcFileUpdate(	UTF8 * pszCloudServer, // NULL = Ferra
					UTF8 * pszDirLocal,
					UTF8 * pszDirRemote,
					UTF8 * pszFileNames,
					BOOL   bShowActivity) {

	BOOL bDownload=FALSE;
	EH_WEB * psWeb;
	INT		iRet=0;
	CHAR	szUrl[500];
	CHAR	szServ[200];
	UTF8	utfFileName[500];
	EH_AR	arFilesRequest=NULL;
	EH_AR	arMissing=NULL;
	
	if (pszFileNames) arFilesRequest=ARCreate(pszFileNames,"|",NULL);
	
	if (!pszCloudServer) pszCloudServer="http://cloud.ferra.com/updates";

	//sprintf(szFileName,"%s\\%s.txt",sSetup.szTempFolder,pszFolder);
	sprintf(szUrl,"%s/%s/",pszCloudServer,pszDirRemote);

	//
	// Da fare
	//
	/*
	if (bShowActivity) {
		sprintf(szServ,"Update %s ...",pszDirRemote);
		DoveSono(szServ,TRUE);
	}
	*/

//	MouseCursorWait();
	psWeb=webHttpReq(szUrl,"GET",NULL,FALSE,10);
	if (!psWeb->sError.enCode) {

		BYTE *psz,*pFile,*pFileTag,*pszTime;
		SINT iCounter;
		psz=psWeb->pData;

		//
		// Conto numero di ricorrenze
		//
		pFile=psz; iCounter=0;
		do {
			pFile=strstr(pFile,"<A "); if (pFile) {iCounter++; pFile++;}
		} while (pFile);
			
		// if (bShowWindow) obj_message("BARPRO",WS_SETFLAG,iCounter,"MAX");

		//
		// Scarico i files
		//
		sprintf(szServ,"%s/",pszDirRemote); iCounter=0;
		while (true) {

			EH_AR arFileInfo=NULL;
			CHAR * pStart;
			iCounter++;
			// if (bShowWindow) obj_message("BARPRO",WS_SETFLAG,iCounter,"CNT");
			pFileTag=strExtract(psz,"<A HREF=\"","\">",true,TRUE); 
			if (strEmpty(pFileTag)) break;
	
			pszTime=NULL;
			pStart=strstr(psz,pFileTag);
			if (pStart) {pStart-=80; pszTime=strExtract(pStart,"<br>",pFileTag,true,false);}

			if (pszTime) {
				while (strReplace(pszTime,"  "," "));
				arFileInfo=ARFSplit(pszTime," ");
				//ARPrint(arFileInfo);
				if (ARLen(arFileInfo)>2) {
				
					pFile=strExtract(pFileTag,"<A HREF=\"","\">",true,FALSE); 
					if (strCaseStr(pFile,szServ)) { // Controllo che il folder sia giusto

						CHAR * pszFileName=fileName(pFile); 
						if (strEmpty(pszFileName)) { // Folder
						
						}
						else if (*pszFileName!='_') {

							//
							// Controllo se devo acquisirlo
							//
							BOOL bLoad=true;
							sprintf(szUrl,"http://cloud.ferra.com%s",pFile);
							sprintf(utfFileName,"%s" OS_DIR_SLASH "%s",pszDirLocal,pszFileName);

							if (arFilesRequest) {
								INT idx=ARSearchIdx(arFilesRequest,pszFileName,true);
								if (idx<0) bLoad=FALSE; //else *arFilesRequest[idx]=0;
							}

							if (bLoad&&fileCheck(utfFileName)) {
							
								// Controllo la dimensione
								SIZE_T tSize=fileSize(utfFileName);
								if (tSize==atof(arFileInfo[2])) bLoad=false; // L'indice era il sette
							
							}

							if (bLoad) {
								if (!webUrlToFile(	szUrl,
													utfFileName,
													false,	 // T/F fallisce se esiste
													false, // T/F se il file su disco è uguale a quello letto non sovrascrivo  // serve ad esempio per limitare i trasferimenti in FTP
													false,
													NULL,NULL,NULL,NULL)) {iRet|=1;}
							}
						}
						// printf("[%s]=%d" CRLF,pFile,bRet);
					}
				}
				if (arFileInfo) ehFree(arFileInfo);
			}
			strReplace(psz,pFileTag,"");
			ehFreePtrs(3,&pszTime,&pFile,&pFileTag);
			
		}
	}
	webHttpReqFree(psWeb);
//		if (bShowWindow) DoveSono(NULL,FALSE);
//	MouseCursorDefault();

	//
	// Loop di controllo su i file mancanti (da fare)
	//
	if (arFilesRequest) 
	{
		INT a;
		arMissing=ARNew();

		for (a=0;arFilesRequest[a];a++) {
			
			sprintf(utfFileName,"%s/%s",pszDirLocal,arFilesRequest[a]);
			if (!fileCheck(utfFileName)) ARAdd(&arMissing,arFilesRequest[a]);
		
		}

		if (ARLen(arMissing)) {

			CHAR * pszRet;
			if (bShowActivity) {
				pszRet=ARToString(arMissing,",","",".");
				win_infoarg("Attenzione, mancano i files necessari all'applicativo: %s",pszRet);
				ehFree(pszRet);
			}
			iRet|=2;
		}
		ARDestroy(arFilesRequest);
	}

	if (arMissing) ARDestroy(arMissing);
			
	return iRet; // Ritorna TRUE se anche un solo file non riesce a leggerlo
}


//
// _ehtNotify()
//
static void _ehcDo(EH_CLOUD * psCloud,CHAR *pszFormat,...)
{
	int iRet;
	DWORD dwSizeMemory=10024;
	CHAR * lpBuf;
	va_list Ah;

	if (!psCloud->funcNotify) return;

	lpBuf=ehAlloc(dwSizeMemory);
	va_start(Ah,pszFormat);

	//vsprintf(lpBuf,Mess,Ah); // Messaggio finale
	iRet=vsnprintf(lpBuf,dwSizeMemory-1,pszFormat,Ah); // Messaggio finale
	if (iRet<0) {
		ehFree(lpBuf);
		dwSizeMemory=1000000;
		iRet=vsnprintf(lpBuf,dwSizeMemory-1,pszFormat,Ah); // Messaggio finale
//		if (iRet) printf("%s" CRLF,lpBuf);
//		ehError();
	}
	va_end(Ah);
	ehPrintd("%s" CRLF,lpBuf);

	psCloud->funcNotify(WS_DO,0,lpBuf);
	ehFree(lpBuf);
}
#endif