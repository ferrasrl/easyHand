//   +-------------------------------------------+
//   | Isapi
//   | Gestisce la gestione dei .INI             |
//   |                                           |
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/isapi.h"
#include "/easyhand/ehtool/profile.h"

BOOL bIsapiAutoTextHeader=FALSE;

void Isapi_texthtml(EXTENSION_CONTROL_BLOCK *pECB)
{
	 if (!bIsapiAutoTextHeader) Isapi_SendHeaderToClient(pECB,200,NULL, "Content-Type: text/html"  CRLF CRLF);
	 bIsapiAutoTextHeader=TRUE;
}

void Isapi_printf(EXTENSION_CONTROL_BLOCK *pECB, char *pszFormat, ...)
{
	BYTE *lpBuffer=ehAlloc(2000);
	DWORD dwSize;
	va_list arg_ptr;

	Isapi_texthtml(pECB);

	va_start(arg_ptr, pszFormat); 
	vsprintf(lpBuffer, pszFormat, arg_ptr);
	va_end(arg_ptr);
	
	dwSize = (DWORD) strlen(lpBuffer);
	if (pECB)
		pECB->WriteClient(pECB->ConnID, lpBuffer, &dwSize, 0);
		else 
		fwrite(lpBuffer,dwSize,1,stdout); // Modifica 2007
	ehFree(lpBuffer);
}

CHAR *Isapi_VarAlloc(EXTENSION_CONTROL_BLOCK *pECB,CHAR *lpVarName)
{
	DWORD dwSize;
	void *lp;
	pECB->GetServerVariable(pECB->ConnID,lpVarName,0,&dwSize); if (!dwSize) return NULL;
	lp=ehAlloc(dwSize);
	if (!pECB->GetServerVariable(pECB->ConnID,lpVarName,lp,&dwSize)) {ehFree(lp); lp=NULL;}
	return lp;
}

CHAR *Isapi_ArgAlloc(EXTENSION_CONTROL_BLOCK *pECB,CHAR *lpVarName)
{
	// pECB->lpszQueryString
	CHAR *p,*p2;
	CHAR szName[180];
	UINT is;
	sprintf(szName,"&%s=",lpVarName);

	if (!pECB->lpszQueryString) return NULL;
	is=(UINT) strlen(szName); if (strlen(pECB->lpszQueryString)<is) return NULL;
	
	// Se inizia per
	if (!_memicmp(pECB->lpszQueryString,szName+1,is-1))
	{
		p=pECB->lpszQueryString; p+=(is-1);
		p2=strstr(p,"&"); if (p2) *p2=0;
		p=strDecode(p,SE_URL,NULL);
		if (p2) *p2='&';
		return p;
	}
	// Cerco all'interno
	p=strCaseStr(pECB->lpszQueryString,szName); if (!p) return NULL;
	p+=strlen(szName); p2=strstr(p,"&"); if (p2) *p2=0;
	p=strDecode(p,SE_URL,NULL);
	if (p2) *p2='&';
	return p;
}

// Isapi_FilePutOut()
// FALSE = Tutto ok
// TRUE = Errore
BOOL Isapi_FilePutOut(EXTENSION_CONTROL_BLOCK *pECB,SINT iStatus,CHAR *lpStatus,CHAR *lpFile,SINT iMode)
{
	HANDLE hFile;
	DWORD dwSize,dwSizeH,dwSizeR;
	BYTE *lpBuf;
	CHAR szServ[1024];
	 
	hFile=CreateFile(lpFile,
					 GENERIC_READ,
					 FILE_SHARE_READ,//0,//FILE_SHARE_WRITE,
					 NULL,
					 OPEN_EXISTING,
					 FILE_ATTRIBUTE_ARCHIVE//FILE_FLAG_SEQUENTIAL_SCAN
					 ,(HANDLE) NULL);

	if (hFile!=INVALID_HANDLE_VALUE) 
	{
		dwSize=GetFileSize(hFile,&dwSizeH);
		if (dwSize<1) {CloseHandle(hFile); return TRUE;}

		switch (iMode)
		{
		 case 1: // HTML Zippato
			Isapi_SendHeaderToClient(pECB,iStatus,lpStatus,"Content-Type: text/html" CRLF "Content-Encoding: gzip" CRLF "Content-Length: %d" CRLF CRLF,dwSize);
			break;
		 
		 default:
		 case 2: // Html
			//Isapi_SendHeaderToClient(pECB,iStatus,lpStatus,"Content-Type: text/html" "\n\n"); // "Connection:Keep-Alive"
			 Isapi_SendHeaderToClient(pECB,iStatus,lpStatus,
									  "Content-Type: text/html" CRLF 
									  "Content-Length: %d" CRLF CRLF
						//			  "Connection:Keep-Alive\n"
									  ,dwSize);
			break;

		 case 3: // Jpg
			Isapi_SendHeaderToClient(pECB,iStatus,lpStatus,"Content-Type: image/jpeg" CRLF "Content-Length: %d" CRLF CRLF ,dwSize);
			break;

		 case 4: // Html
			 Isapi_SendHeaderToClient(pECB,iStatus,lpStatus,
									  "Content-Type: text/html" CRLF 
									  "Content-Length: %d" CRLF CRLF 
									  ,dwSize);
			break;
		}

		lpBuf=ehAlloc(dwSize); 
		if (!lpBuf) // No ho memoria
		{
			CloseHandle(hFile);	return TRUE;
		}

		if (ReadFile(hFile,lpBuf,dwSize,&dwSizeR,NULL))
		{
			if (strCaseCmp(pECB->lpszMethod,"HEAD")) // Non sono HEAD
				pECB->WriteClient(pECB->ConnID, lpBuf, &dwSizeR, 0); // Sparo fuori ...
		} 
		else 
		{
			ehFree(lpBuf);
			CloseHandle(hFile);
			return TRUE;
		}

		ehFree(lpBuf);
		CloseHandle(hFile);
		return FALSE;
	}
	else 
	{
		BYTE *lpError=osErrorStr(osGetError(),szServ,sizeof(szServ));
		ehLogWrite("Isapi_FilePutOut(): %s - Errore: %d : %s\n%s",lpFile,GetLastError(),lpError);
		ehFreeNN(lpError);
		return TRUE;
	}
}

DWORD Isapi_SendHeaderToClient(EXTENSION_CONTROL_BLOCK *pECB,SINT iStatus,char *lpStatus,char *pszFormat, ...)
{
    BOOL fReturn;
    CHAR szHeader[1024] = "";
    DWORD hseStatus = HSE_STATUS_SUCCESS;
	DWORD dwSize;
	HSE_SEND_HEADER_EX_INFO hshe;
	
 	dwSize=(DWORD) strlen(szHeader);
	hshe.pszStatus=(!lpStatus)?"200 OK":lpStatus;
	hshe.cchStatus=(DWORD) strlen(hshe.pszStatus);
	if (!strCaseCmp(pECB->lpszMethod,"HEAD")) 
	{*szHeader=0;}
	else 
	{
		va_list arg_ptr;
		va_start(arg_ptr, pszFormat); 
		vsprintf(szHeader, pszFormat, arg_ptr);
		va_end(arg_ptr);
	}
	hshe.pszHeader=szHeader;
	hshe.cchHeader=(DWORD) strlen(hshe.pszHeader);
	hshe.fKeepConn=FALSE;

	pECB->dwHttpStatusCode=(iStatus==0)?200:iStatus;
	fReturn = 
    pECB->ServerSupportFunction(pECB->ConnID, 
                                HSE_REQ_SEND_RESPONSE_HEADER_EX,
                                &hshe, // Telling the client not to close the connection
                                NULL,
                                NULL);
    if (! fReturn ) {
		//ehLogWrite("HSE_STATUS_ERROR");
        hseStatus = HSE_STATUS_ERROR;
    }
    return (hseStatus);
}


	/*
    fReturn = 
    pECB->ServerSupportFunction(pECB->ConnID, 
                                HSE_REQ_SEND_RESPONSE_HEADER,
                                "Connection:Keep-Alive", // Telling the client not to close the connection
                                NULL,
                                (LPDWORD) szHeader);
 */
