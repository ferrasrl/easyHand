//   +-------------------------------------------+
//   | FRAPI
//   | Funzione Ferrà di complemento a RAPI 
//   | per accesso ad devicescon Active Sync
//   |             
//   |							by Ferrà srl 2007
//   +-------------------------------------------+
#include "/easyhand/inc/easyhand.h"
// Necessita di aggiungere nelle directory
// C:\Programmi\Windows CE Tools\wce500\Windows Mobile 5.0 Pocket PC SDK\Activesync\Inc
// C:\Programmi\Windows CE Tools\wce500\Windows Mobile 5.0 Pocket PC SDK\Activesync\Lib
//
// rapi.lib
//
#include <rapi.h>
//#include "/easyhand/inc/rapi.h"
#include "/easyhand/ehtool/frapi.h"

// Ritorna TRUE se esiste
BOOL CeFileExist(WCHAR *pFile)
{
	HANDLE hFile;
	//win_infoarg("%S",pFile);
	hFile= CeCreateFile(pFile,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_ARCHIVE,
						(HANDLE)NULL);
	if (hFile==INVALID_HANDLE_VALUE) return FALSE;
	CeCloseHandle(hFile);
	return TRUE;
}

BOOL CopyFileToCE(CHAR *lpSource,CHAR *lpDest,BOOL fView,void* (*procNotify)(SINT,LONG,void *))
{
	HANDLE hSource;
	HANDLE hDest;

	DWORD dwFileCopy=0;
	DWORD dwFileSize;
	DWORD dwRealWrite;
	DWORD dwRealRead;
	LPBYTE *lpBuffer;
	//WCHAR lpwDest[MAX_PATH];
	WCHAR * pwcDest;
	DWORD dwBuffer=0x1FFFF;
	BOOL fRet=FALSE;

	if (fView)
	{
		win_open(EHWP_SCREENCENTER,82,296,77,-1,3,ON,"Trasferimento su periferica");
		bar_percf(13,47,272,0L,100L,"VGASYS",0);

		dispf(13,31,0,-1,ON,"SMALL F",3,fileName(lpSource));
	}

	if (procNotify) {(procNotify)(WS_START,0,lpSource);}
	hSource= CreateFile((LPCTSTR) lpSource,
						 GENERIC_READ,
						 FILE_SHARE_READ,
						 NULL,//&sSA,
						 OPEN_EXISTING,
						 FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
						 (HANDLE)NULL);
	if (hSource==INVALID_HANDLE_VALUE)
	{
		if (fView) win_infoarg("Errore in apertura file Sorgente [%s][%d]",lpSource,CeGetLastError());
		fRet=TRUE; 
		if (fView) {win_close();}
		return fRet;
	}
	dwFileSize=GetFileSize(hSource,NULL);
	if (procNotify) {(procNotify)(WS_LAST,dwFileSize,lpSource);} // Notifico la dimensione massima

//	CharToUnicode(lpwDest,lpDest);
	pwcDest=strToWcs(lpDest);
	hDest= CeCreateFile(pwcDest,
						GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						NULL,//&sSA,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_ARCHIVE,
						(HANDLE)NULL);
	ehFree(pwcDest);
	if (hDest==INVALID_HANDLE_VALUE)
	{
		if (fView) win_infoarg("Errore in apertura file Destinazione [%s][%d]",lpDest,CeGetLastError());
		CloseHandle(hSource);
		fRet=TRUE; 
		if (fView)
		{
			win_close();
		}
		return fRet;
	}

	lpBuffer=GlobalAlloc(LPTR,dwBuffer);

	while (TRUE)
	{
		if (!ReadFile(hSource,lpBuffer,dwBuffer,&dwRealRead,NULL)) 
			{
			 if (fView) win_infoarg("Errore in lettura Sorgente"); 
			 GlobalFree(lpBuffer);
			 CloseHandle(hSource);
			 CeCloseHandle(hDest);
			 fRet=TRUE; 
			 break;
			}

		if (!CeWriteFile(hDest,lpBuffer,dwRealRead,&dwRealWrite,NULL)) 
			{
				if (fView) win_infoarg("Errore in scrittura Destinazione [%d]",CeGetLastError()); 
				GlobalFree(lpBuffer);
				CloseHandle(hSource);
				CeCloseHandle(hDest);
				fRet=TRUE; 
				break;
			}
		dwFileCopy+=dwRealRead; 
		if (fView)	
		{
			bar_percf(13,47,272,dwFileCopy,dwFileSize,"VGASYS",0);
/*
		dispx("%s: Copiati %ld bytes (%3d%%)\r",
									   fileName(lpDest),
									   dwFileCopy,
									   dwFileCopy*100/dwFileSize);
									   */
		}
		if (procNotify) {(procNotify)(WS_COUNT,dwFileCopy,lpSource);} // Notifico la dimensione massima

		if (dwRealRead<dwBuffer) break;
	}
	if (procNotify) {(procNotify)(WS_DO,dwFileCopy,lpSource);} // Notifico la fine del copiaggio

	GlobalFree(lpBuffer);
	CloseHandle(hSource);
	CeCloseHandle(hDest);
	//if (!fRet&&fView) dispx("File %s copiato.             \r",lpDest);
	if (fView)
	{
		win_close();
	}
	return fRet;
}

void CopyFolderToCE(CHAR *lpFileSearch,WCHAR *wsFolderDest)
{
	struct _finddata_t ffbk;
	CHAR szFolderSource[600],*lpFileName;
	BYTE *lpFileSelection;
	LONG lFind;
	BYTE *lpFileSource,*lpFileDest;
//	SINT a;

	strcpy(szFolderSource,filePath(lpFileSearch)); 
//	win_infoarg("%s",szFolderSource);
	lpFileSelection=strDup(fileName(lpFileSearch));
	lpFileSource=ehAlloc(1024);
	lpFileDest=ehAlloc(1024);

	lFind=_findfirst(lpFileSearch, &ffbk);
	if (lFind!=-1)
	{
		do
		{
			lpFileName=ffbk.name;
			//
			// Salto i file di sistema e quelli nascosti
			//
			if (ffbk.attrib&_A_HIDDEN||ffbk.attrib&_A_SYSTEM) continue;

			if (ffbk.attrib&_A_SUBDIR) 
			{
				if (*lpFileName=='.'||*lpFileName=='_') continue; // non va bene
			}

			//
			// E' una sotto cartella ed è richiesta l'analisi
			//
			if (ffbk.attrib&_A_SUBDIR) 
			{
				CHAR szSubFolderSource[600];
				WCHAR wsSubFolderDest[600];
				sprintf(szSubFolderSource,"%s%s\\%s",szFolderSource,lpFileName,lpFileSelection);
				swprintf(wsSubFolderDest,600,L"%s\\%S",wsFolderDest,lpFileName);
				CeCreateDirectory(wsSubFolderDest,NULL);
				CopyFolderToCE(szSubFolderSource,wsSubFolderDest);
				continue;
			}
/*			
			win_infoarg("Copy %s%s > %S\\%s",
					szFolderSource,lpFileName,
					wsFolderDest,lpFileName);
*/
			sprintf(lpFileSource,"%s%s",szFolderSource,lpFileName);
			sprintf(lpFileDest,"%S\\%s",wsFolderDest,lpFileName);
			CopyFileToCE(lpFileSource,lpFileDest,TRUE,NULL);

		} while(_findnext(lFind, &ffbk )==0);
	}		 
	_findclose( lFind );
	ehFree(lpFileSelection);
	ehFree(lpFileSource);
	ehFree(lpFileDest);
}

void GetDeviceInfo(F_DEVICE_INFO *pDeviceInfo)
{
	CHAR *lpBuffer=ehAlloc(1024);


	pDeviceInfo->sVersionInfo.dwOSVersionInfoSize=sizeof(CEOSVERSIONINFO);
	CeGetVersionEx(&pDeviceInfo->sVersionInfo);

	CeGetSystemInfo(&pDeviceInfo->sSystemInfo);
  
	pDeviceInfo->szMill.cx=CeGetDesktopDeviceCaps(HORZSIZE);
	pDeviceInfo->szMill.cy=CeGetDesktopDeviceCaps(VERTSIZE);
	pDeviceInfo->szPixel.cx=CeGetDesktopDeviceCaps(HORZRES);
	pDeviceInfo->szPixel.cy=CeGetDesktopDeviceCaps(VERTRES);
	pDeviceInfo->iBitPixel=CeGetDesktopDeviceCaps(BITSPIXEL);

	pDeviceInfo->sMemoryStatus.dwLength=sizeof(MEMORYSTATUS);
	CeGlobalMemoryStatus(&pDeviceInfo->sMemoryStatus);
}
