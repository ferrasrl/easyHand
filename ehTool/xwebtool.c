// -------------------------------------------------------------
// WEBTOOL   Tool per l'accesso ad internet                    |
// by Ferrà Art & Technology                                   |
// Created by G.Tassistro   (Gennaio 99)                       |
// Revisioned by G.Tassistro 2004 (x Pocket PC 2003)
// -------------------------------------------------------------
// WSOCK32.LIB wininet.lib 
#include "\ehtool\include\ehsw_i.h"
#include "webtool.h"
#include <winsock.h>

#ifdef _WIN32_WCE
#include "connmgr.h"
// WindowCE:Attenzione Server cellcore.lib nel link
#endif

// -------------------------------------------------------------
// WEBCOPY  Preleva una pagina WEB e la scrive su disco        |
//                                                             |
// Created by G.Tassistro   (Gennaio)                          |
// -------------------------------------------------------------

static HINTERNET hOpen;
//extern TCHAR LastServer[];

#ifdef _UNICODE
#define ITO(a,b,c) _itow(a,b,c)
#define lstrstr(a,b) wcsstr(a,b)
#else
#define ITO(a,b,c) _itoa(a,b,c)
#define lstrstr(a,b) strstr(a,b)
#endif
/*
#ifndef lstrlen 
#define lstrlen(p) wcslen(p)
#endif
*/
static BOOL PassInput(TCHAR *szUser,TCHAR *szPass);
static BOOL ErrorOut (void *(*Control)(INT,LONG,TCHAR *),FILE *pf,DWORD dError,TCHAR *szCallFunc);
void PutOut(void *(*Control)(INT cmd,LONG Info,TCHAR *),DWORD dwError,TCHAR *Mess,...);

void WebToolStart(void)
{
  hOpen=NULL;
  //*LastServer=0;
}

// WEBOPEN Apro la gestione Windows di internet
// ritorna: FALSE = OK,  TRUE=Error
BOOL WebOpen(TCHAR *User)
{
	if (!(hOpen=InternetOpen(User,  // Nome di chi visita
			    INTERNET_OPEN_TYPE_DIRECT, // INTERNET_OPEN_TYPE_PRECONFIG 
			    //LOCAL_INTERNET_ACCESS , // Tipo di ricerca del nome
			    NULL,0,0)))
	 {
	  ErrorOut(NULL,NULL,GetLastError(),TEXT("InternetOpen"));
	  return TRUE;
	 }
 return FALSE;
}

void WebClose(void)
{
 if (hOpen)
 {
  //win_time("Chiudo la sezione Internet ...",1|NOWIN);
  InternetCloseHandle(hOpen);
 }
 hOpen=NULL;
}

void WebToolEnd(void)
{
 WebClose();
}

// Ritorna FALSE=Tutto OK TRUE=Errore
BOOL WebCopy(TCHAR *User,        // Indicazione dell'utente
	         TCHAR *lpWebServer, // Server da Aprire
	         TCHAR *lpPage,      // Pagina da richiedere
	         TCHAR *lpMethod,    // Metodo (GET/POST)
	         TCHAR *lpDati,      // Dati da inviare
	         TCHAR *lpFileName,  // File name da scrivere con il nome della pagina
	         void *(*Control)(INT cmd,LONG Info,TCHAR *), // Funzione di controllo e display
	         BOOL Vedi, // TRUE/FALSE Visione dei passaggi di transazione
			 BOOL fSecure) // TRUE/FALSE Se transazione sicura
{
	HINTERNET  hConnect=NULL;
	HINTERNET  hReq=NULL;
	DWORD  dwSize, dwCode;
	TCHAR   Headers[100],*lpHeaders=Headers;
	DWORD  dwHeaders;
	FILE *pf=NULL;
	DWORD dwDati;
	BYTE  *lpDatiBuf=NULL;
	TCHAR  *lpBuffer=NULL;
	DWORD Count=0;
	BOOL MethodPost=FALSE;
	TCHAR *lpIBuf=NULL;
//	SINT iSizeBuffer=1024;
	SINT iSizeBuffer=200000;
	const TCHAR *Accept[]={TEXT("text/*"),TEXT("*/*"),NULL};
	BOOL fRet=TRUE;
//	INTERNET_BUFFERS sBuffersOut;

	if (lpFileName)
	{
#ifdef _UNICODE
		pf=_wfopen(lpFileName,_T("wb"));
#else
		pf=fopen(lpFileName,"wb");
#endif
		if (!pf)
		{ErrorOut(Control,NULL,GetLastError(), TEXT("File in save"));
		 goto MYERRORE;
		}
	} else pf=NULL;

	// -------------------------------------------------
	// CONNESSIONE CON IL SERVER                       |
	// Se c'Š un cambio Server effettuo la connessione |
	// -------------------------------------------------
	if (lpFileName)
	{
		//fprintf(pf,"<! PincoURL   : %s://%s/%s>\n",fSecure?"https":"http",lpWebServer,lpPage);
		//fprintf(pf,"<! PincoMethod: %s>\n",lpMethod);
		//fprintf(pf,"<! PincoParam : %s>\n",lpDati);

	/*
		printf("<! FerraURL   : http://%s/%s>\n",lpWebServer,lpPage);
		printf("<! FerraMethod: %s>\n",lpMethod);
		printf("<! FerraParam : %s>\n",lpDati);
	*/
	}

	if (Vedi) PutOut(Control,0,TEXT("URL: http://%s/%s\n"),lpWebServer,lpPage);
	if (Vedi) PutOut(Control,0,TEXT("Server %s connection ...."),lpWebServer);
	//lstrcpy(LastServer,lpWebServer);
	if (!(hConnect=InternetConnect(hOpen, // Handle della apertura in internet
								   lpWebServer, // Nome del server
								   fSecure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, // Porta di accesso 80=HTTP
								   //INTERNET_DEFAULT_HTTP_PORT, // Porta di accesso 80=HTTP
								   TEXT("anonymous"), // User Name
								   TEXT(""), // Password
								   INTERNET_SERVICE_HTTP, // Servizio richiesto HTTP/FTP ecc...
								   0, //Flag ?
								   0))) // dwContext (Funzione ?)
							{
								ErrorOut (Control,pf,GetLastError(), TEXT("InternetConnect")); 
								goto MYERRORE;
								//return TRUE;
							}
	if (Vedi) PutOut(Control,0,TEXT(" ok\n"));
	
	// -----------------------------------------------
	// Preparo una richiesta di apertura             |
	// -----------------------------------------------

	lpDatiBuf=GlobalAlloc(GPTR,8000);//new CHAR [4000];
	lstrcpy((TCHAR *) lpDatiBuf,lpPage);
	if (lpMethod!=NULL)
	{
	 if (!lstrcmp(lpMethod,TEXT("GET")))
	 {
		lstrcat((TCHAR *) lpDatiBuf,TEXT("?"));
		lstrcat((TCHAR *) lpDatiBuf,lpDati);
	 }
	 if (!lstrcmp(lpMethod,TEXT("POST"))) MethodPost=TRUE;
	}
	
	// Accept sono i file che posso accettare in risposta
	if (!(hReq=HttpOpenRequest(hConnect,     // Handle della connessione
							   lpMethod,
							   (TCHAR *) lpDatiBuf,    // Nome della pagina o del comando da aprire
							   NULL,//HTTP_VERSION, // Versione dell'http
							   TEXT(""),   // Documento che f… la richiesta
							   Accept, // Indica cosa accetta il client (Es. "text/*")
							   //fSecure ? INTERNET_FLAG_IGNORE_CERT_CN_INVALID |INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |INTERNET_FLAG_SECURE|INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE : INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE, // Modo di lettura ed usa della cache
							   fSecure ? INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE : INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE, // Modo di lettura ed usa della cache
							   0))) // dwContext
	{
		ErrorOut(Control,pf,GetLastError(),TEXT("HttpOpenRequest")); 
		goto MYERRORE;
	}

//	win_infoarg("Request ok");
	if (Vedi) PutOut(Control,0,TEXT("\rPost HttpOpenRequest() ...."));
	
	lpIBuf=GlobalAlloc(GPTR,iSizeBuffer);

//ERROR_INTERNET_SECURITY_CHANNEL_ERROR 
	// -----------------------------------------------
	// Spedisco la richiesta di apertura             |
	// -----------------------------------------------
#ifndef _CONSOLE
again:
#endif

	if (MethodPost)
	{
		if (!lpDati) dwDati=0; else dwDati=lstrlen(lpDati);
		if (fSecure)
		{
			lpHeaders=NULL;
			dwHeaders=0;
			dwDati=0;
			lpDati=NULL;
		}
		else
		{
#ifdef _UNICODE
		 swprintf(lpHeaders,_T("Content-Type: application/x-www-form-urlencoded%c%c"),10,10);
#else
		 sprintf(lpHeaders,"Content-Type: application/x-www-form-urlencoded%c%c",10,10);
#endif
		 dwHeaders=-1;//strlen(lpHeaders);
		}
	}
	else
	{
		dwDati=0;
		lpDati=NULL;
		lpHeaders=NULL;
		dwHeaders=0;
	}
/*
	{
		SINT dwSize=1023;
		CHAR lpvBuffer[1024];
		DWORD dwIndex[20];
		*lpvBuffer=0;
		if (!HttpQueryInfo(hReq,
						   HTTP_QUERY_FLAG_REQUEST_HEADERS ,
						   &lpvBuffer,
						   &dwSize,
						   dwIndex))
		{
			ErrorOut (Control,pf,GetLastError(), "HttpQuery"); fclose(pf);
			goto MYERRORE;
		
		}
	}
	*/
	if (Vedi) PutOut(Control,0,TEXT("\rPre HttpSendRequest() ...."));
	// printf("####[%s]",lpHeaders);
	lstrcpy((TCHAR *) lpDatiBuf,lpDati);
	if (!HttpSendRequest(hReq, // Handle della richiesta
						 lpHeaders,
		                 dwHeaders,//-1L,
		                 lpDatiBuf,
		                 dwDati))
	{
	 SINT iErr=GetLastError();
	 os_ErrorShow("HttpSend",iErr);
	 ErrorOut(Control,pf,iErr,TEXT("HttpSend")); 
	 goto MYERRORE;
	 //return TRUE;
	}

	//printf("[%s] %d",lpDatiBuf,dwHeaders);
	//free(lpDatiBuf); lpDatiBuf=NULL;
	dwSize = sizeof (DWORD) ;
	if (Vedi) PutOut(Control,0,TEXT("\rPost HttpSendRequest() ...."));

	// -----------------------------------------------
	// Chiedo i flag del Query                       |
	// -----------------------------------------------

	if (!HttpQueryInfo(hReq, // Handle della richiesta
					   HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
					   &dwCode, // Buffer che riceve
		               &dwSize, // Lunghezza del buffer
		               NULL)) // Indice enumerativo ?
	{
	 ErrorOut (Control,pf,GetLastError(),TEXT("HttpQueryInfo")); 
	 goto MYERRORE;
	 //return TRUE;
	}

	if (Vedi) PutOut(Control,0,TEXT("\rPost HttpQueryInfo() ...."));

	// -----------------------------------------------
	// Se l'accesso ‚ negato richiedo la Password    |
	// -----------------------------------------------
	if (dwCode == HTTP_STATUS_DENIED ||dwCode == HTTP_STATUS_PROXY_AUTH_REQ)
	{
#ifdef _CONSOLE
	 ErrorOut(Control,pf,GetLastError(), "HTTP_STATUS_DENIED"); 
	 goto MYERRORE;
#else
	 TCHAR szUser[50]=TEXT("");
	 TCHAR szPass[50]=TEXT("");
	 if (!PassInput(szUser,szPass)) goto MYERRORE;

	 // We have to read all outstanding data on the Internet handle
	 // before we can resubmit request. Just discard the data.
	 do
	 {
		InternetReadFile (hReq, (LPVOID) lpIBuf, iSizeBuffer-1, &dwSize);
	 }  while (dwSize != 0);

	 if (!InternetSetOption(hConnect,
							INTERNET_OPTION_USERNAME,
							(LPVOID) szUser,
							lstrlen(szUser)))
     {
       //cerr << "InternetSetOptionFailed: " << GetLastError() << endl;
	   win_infoarg(TEXT("InternetSetOptionFailed(1): %d"),GetLastError());
       goto MYERRORE;
	   //return TRUE;
     }

	 if (!InternetSetOption(hConnect,INTERNET_OPTION_PASSWORD,(LPVOID) szPass,lstrlen (szPass)))
     {
       //cerr << "InternetSetOptionFailed: " << GetLastError() << endl;
	   win_infoarg(TEXT("InternetSetOptionFailed(2): %d"),GetLastError());
       goto MYERRORE;
	   //return TRUE;
     }
	 goto again;
#endif
	}

	// -------------------------------------------------------
	// La prima volta cercheremo la grandezza dell'header    |
	// -------------------------------------------------------
	dwSize=0;
	HttpQueryInfo(hReq,
	              HTTP_QUERY_RAW_HEADERS_CRLF,
	              NULL, // Deduco che se NULL non carica il buffer
	              &dwSize,
	              NULL);

	if (dwSize<1) 
			{
			  ErrorOut (Control,pf,GetLastError(),TEXT("HttpQueryInfo(1)")); 
			  goto MYERRORE;
			}

	//lpBuffer =  new CHAR [dwSize + 1 ];
	lpBuffer=malloc(dwSize + 1);

	// -------------------------------------------------------
	// Dopo aver dimensionato il buffer carichiamo l'header  |
	// -------------------------------------------------------
	// Now we call HttpQueryInfo again to get the headers.
	if (Vedi) PutOut(Control,0,TEXT("\rPre HttpQueryInfo() ...."));
	if (!HttpQueryInfo(hReq,
					   HTTP_QUERY_RAW_HEADERS_CRLF,
					   (LPVOID) lpBuffer, // CArica il Buffer
					   &dwSize,
					   NULL))
			{
			  ErrorOut (Control,pf,GetLastError(),TEXT("HttpQueryInfo")); 
			  //win_infoarg("HttpQueryInfo [%d]",GetLastError());
			  goto MYERRORE;
			  //return TRUE;
			}
	
	//printf("[%s]",lpBuffer);
	*(lpBuffer + dwSize) = '\0';  // Metto il terminatore nel buffer
	if (Vedi) PutOut(Control,0,TEXT("\rPost HttpQueryInfo() ...."));

#ifndef _UNICODE
	if (!strstr(lpBuffer,TEXT("200 OK")))
	{
		ErrorOut (Control,pf,GetLastError(), "NOT 200 OK!"); 
		goto MYERRORE;
	}
#endif
//	printf("[%s]",lpBuffer); getch();
	//win_infoarg("[%s]",lpBuffer);
	// We can relay on the CONTENTS_LENGTH header to find the size of the
	// HTML file, since this header may not exist (it is optional for HTTP/1.0

	// ---------------------------------------------
	// DOWNLOAD DEL DOCUMENTO                      |
	// ---------------------------------------------
	// printf("URL: http://%s/%s\n",lpWebServer,lpPage);
	// printf("Method    : %s|\n",lpMethod);
	// printf("Parametri : %s|\n",lpDati);

	//cout << "\n:";
	//printf("\nControllo 0\n");
	if (Vedi) PutOut(Control,0,TEXT("\nDownLoad ...\n"));
	if (Vedi) PutOut(Control,0,TEXT("Attendere prego ..."));
	do
	{
		dwSize=0;
		/*
		ZeroFill(sBuffersOut);
		sBuffersOut.dwStructSize=sizeof(INTERNET_BUFFERS);
		sBuffersOut.Next=&sBuffersOut;
		sBuffersOut.lpvBuffer=lpIBuf;
		sBuffersOut.dwBufferLength=iSizeBuffer;

		if (!InternetReadFileEx(hReq, // Handle della richiesta
							    &sBuffersOut,
							    IRF_NO_WAIT,
								0)) 
								*/
		if (!InternetReadFile(hReq, // Handel della richiesta
							  (LPVOID) lpIBuf, // Puntatore al buffer
							  iSizeBuffer-1, // Dimensioni del buffer
							  &dwSize)) // Byte realmente letti
		{
			ErrorOut (Control,pf,GetLastError (),TEXT("InternetReadFile")); 
			win_infoarg(TEXT("InternetReadFile [%d]"),GetLastError());
			goto MYERRORE;
			//efx2();continue;
		}

		//printf("[%d]",dwSize); getch();
		if (!dwSize) break;
/*
			if (_kbhit())
				{
					if (_getch()==27) PRG_end("User Escape !");
				}
				*/
		
		//mouse_inp(); if (key_press(ESC)) break;//PRG_end("User Escape !");
//		if (_kbhit()) break;
		
//		lpIBuf[dwSize]='\0'; if (pf) fprintf(pf,lpIBuf);
		fwrite(lpIBuf,dwSize,1,pf);
		Count+=dwSize;

        // Monitoraggio dei Byte letti
		if (Vedi) PutOut(Control,0,TEXT("\r%s:>> %d byte                 "),User,(SINT) Count);
		
		// Spedisco i Byte letti alla funzione di controllo
		if (Control) (Control)(WS_DO,(LONG)dwSize,lpIBuf);

	} while (TRUE);

	if (Vedi) PutOut(Control,0,TEXT("\rTotale letti: %d byte\n"),(SINT) Count);
/*
	//printf("Letti [%d]",Count);
	// --------------------------------------
	// Chiudo la richiesta                  |
	// --------------------------------------
	if (!InternetCloseHandle (hReq) )
	{
		ErrorOut (Control,pf,GetLastError (), "CloseHandle on hReq");
		goto MYERRORE;
	}

	// --------------------------------------
	// Chiudo la Connessione                |
	// --------------------------------------
	if (!InternetCloseHandle (hConnect) )
	{
		ErrorOut (Control,pf,GetLastError (), "CloseHandle on hConnect");
		goto MYERRORE;
	}
	
	free(lpBuffer);
	if (lpIBuf) free(lpIBuf);
	if (pf) {fclose(pf); pf=NULL;}
	return FALSE;
*/
	fRet=FALSE;

MYERRORE:
	// --------------------------------------
	// Chiudo la richiesta                  |
	// --------------------------------------
	if (hReq)
	{
	 if (!InternetCloseHandle (hReq) )
	 {
		ErrorOut (Control,pf,GetLastError (),TEXT("CloseHandle on hReq"));
		goto MYERRORE;
	 }
	}

	// --------------------------------------
	// Chiudo la Connessione                |
	// --------------------------------------
	if (hConnect)
	{
	 if (!InternetCloseHandle (hConnect) )
	 {
		ErrorOut (Control,pf,GetLastError (),TEXT("CloseHandle on hConnect"));
		goto MYERRORE;
	 }
	}

	if (pf) {fclose(pf); pf=NULL;}
	if (lpBuffer) free(lpBuffer);
	if (lpDatiBuf) GlobalFree(lpDatiBuf);
	if (lpIBuf) GlobalFree(lpIBuf);
	return fRet;
}







// -------------------------------------------------------
// Mostra gli errori                                     
// 
//
static BOOL ErrorOut(void *(*Control)(INT cmd,LONG Info,TCHAR *),FILE *pf,DWORD dwError, TCHAR * szCallFunc)
{
 TCHAR szTemp[100] =TEXT(""),*szBuffer=NULL,*szBufferFinal = NULL;
 TCHAR *lpError=TEXT("?");
 DWORD  dwIntError , dwLength = 0;

 switch (dwError)
 {
	case ERROR_HTTP_INVALID_SERVER_RESPONSE: lpError=TEXT("ERROR_HTTP_INVALID_SERVER_RESPONSE"); break;
	case ERROR_INTERNET_DISCONNECTED: lpError=TEXT("ERROR_INTERNET_DISCONNECTED"); break;
 }

 if (pf) fprintf(pf,  "<! FerraERR_%s error %d>\n", szCallFunc, dwError );
#ifdef _UNICODE
 swprintf(szTemp,TEXT("%s error %d (%s)\n "),szCallFunc,dwError,lpError);
#else
 sprintf(szTemp,"%s error %d (%s)\n ",szCallFunc,dwError,lpError);
#endif
 
 if (dwError == ERROR_INTERNET_EXTENDED_ERROR)
  {
    InternetGetLastResponseInfo(&dwIntError, NULL, &dwLength);

	if (dwLength)
    {
	if ( !(szBuffer = (TCHAR *) LocalAlloc(LPTR,dwLength) ) )
	{
		lstrcat(szTemp, TEXT ( "Non posso allocare memoria per visualizzare l'errore: ") );
		lstrcat(szTemp, ITO (GetLastError(), szBuffer, 10)   );
		lstrcat(szTemp, TEXT ("\n") );
		//cerr << szTemp << endl;
		PutOut(Control,dwError,szTemp);
		return FALSE;
	}
	if (!InternetGetLastResponseInfo (&dwIntError, (LPTSTR) szBuffer, &dwLength))
	{
		lstrcat (szTemp, TEXT ( "Unable to get Intrnet error. Error code: ") );
		lstrcat (szTemp, ITO (GetLastError(), szBuffer, 10)  );
		lstrcat (szTemp, TEXT ("\n") );
//		cerr << szTemp << endl;
		PutOut(Control,dwError,szTemp);
		return FALSE;
	}
	if ( !(szBufferFinal = (TCHAR *) LocalAlloc ( LPTR,  (lstrlen (szBuffer) +lstrlen (szTemp) + 1)  ) )  )
	{
		lstrcat (szTemp, TEXT ( "Unable to allocate memory. Error code: ") );
		lstrcat (szTemp, ITO (GetLastError(), szBuffer, 10)  );
		lstrcat (szTemp, TEXT ("\n") );
//		cerr << szTemp << endl;
		PutOut(Control,dwError,szTemp);
		return FALSE;
	}

	lstrcpy(szBufferFinal, szTemp);
	lstrcat(szBufferFinal, szBuffer);
	LocalFree(szBuffer);
	//cerr <<  szBufferFinal  << endl;
	PutOut(Control,dwIntError,szBufferFinal);
	if (Control!=NULL) (Control)(9000,dwError,szBufferFinal);
	LocalFree (szBufferFinal);
    }
 }
 else
 {
 //cerr << szTemp << endl;
  PutOut(Control,dwError,szTemp);
  switch (dwError)
  {

	case ERROR_INTERNET_CONNECTION_ABORTED: lpError=TEXT("ERROR_INTERNET_CONNECTION_ABORTED"); break;
	 /*
	case ERROR_INTERNET_OUT_OF_HANDLES           (INTERNET_ERROR_BASE + 1)
	case ERROR_INTERNET_TIMEOUT                  (INTERNET_ERROR_BASE + 2)
	case ERROR_INTERNET_EXTENDED_ERROR           (INTERNET_ERROR_BASE + 3)
	case ERROR_INTERNET_INTERNAL_ERROR           (INTERNET_ERROR_BASE + 4)
	case ERROR_INTERNET_INVALID_URL              (INTERNET_ERROR_BASE + 5)
#define ERROR_INTERNET_UNRECOGNIZED_SCHEME      (INTERNET_ERROR_BASE + 6)
#define ERROR_INTERNET_NAME_NOT_RESOLVED        (INTERNET_ERROR_BASE + 7)
#define ERROR_INTERNET_PROTOCOL_NOT_FOUND       (INTERNET_ERROR_BASE + 8)
#define ERROR_INTERNET_INVALID_OPTION           (INTERNET_ERROR_BASE + 9)
#define ERROR_INTERNET_BAD_OPTION_LENGTH        (INTERNET_ERROR_BASE + 10)
#define ERROR_INTERNET_OPTION_NOT_SETTABLE      (INTERNET_ERROR_BASE + 11)
#define ERROR_INTERNET_SHUTDOWN                 (INTERNET_ERROR_BASE + 12)
#define ERROR_INTERNET_INCORRECT_USER_NAME      (INTERNET_ERROR_BASE + 13)
#define ERROR_INTERNET_INCORRECT_PASSWORD       (INTERNET_ERROR_BASE + 14)
#define ERROR_INTERNET_LOGIN_FAILURE            (INTERNET_ERROR_BASE + 15)
#define ERROR_INTERNET_INVALID_OPERATION        (INTERNET_ERROR_BASE + 16)
#define ERROR_INTERNET_OPERATION_CANCELLED      (INTERNET_ERROR_BASE + 17)
#define ERROR_INTERNET_INCORRECT_HANDLE_TYPE    (INTERNET_ERROR_BASE + 18)
#define ERROR_INTERNET_INCORRECT_HANDLE_STATE   (INTERNET_ERROR_BASE + 19)
#define ERROR_INTERNET_NOT_PROXY_REQUEST        (INTERNET_ERROR_BASE + 20)
#define ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND (INTERNET_ERROR_BASE + 21)
#define ERROR_INTERNET_BAD_REGISTRY_PARAMETER   (INTERNET_ERROR_BASE + 22)
#define ERROR_INTERNET_NO_DIRECT_ACCESS         (INTERNET_ERROR_BASE + 23)
#define ERROR_INTERNET_NO_CONTEXT               (INTERNET_ERROR_BASE + 24)
#define ERROR_INTERNET_NO_CALLBACK              (INTERNET_ERROR_BASE + 25)
#define ERROR_INTERNET_REQUEST_PENDING          (INTERNET_ERROR_BASE + 26)
#define ERROR_INTERNET_INCORRECT_FORMAT         (INTERNET_ERROR_BASE + 27)
#define ERROR_INTERNET_ITEM_NOT_FOUND           (INTERNET_ERROR_BASE + 28)
#define ERROR_INTERNET_CANNOT_CONNECT           (INTERNET_ERROR_BASE + 29)
#define ERROR_INTERNET_CONNECTION_ABORTED       (INTERNET_ERROR_BASE + 30)
#define ERROR_INTERNET_CONNECTION_RESET         (INTERNET_ERROR_BASE + 31)
#define ERROR_INTERNET_FORCE_RETRY              (INTERNET_ERROR_BASE + 32)
#define ERROR_INTERNET_INVALID_PROXY_REQUEST    (INTERNET_ERROR_BASE + 33)
#define ERROR_INTERNET_NEED_UI                  (INTERNET_ERROR_BASE + 34)

#define ERROR_INTERNET_HANDLE_EXISTS            (INTERNET_ERROR_BASE + 36)
#define ERROR_INTERNET_SEC_CERT_DATE_INVALID    (INTERNET_ERROR_BASE + 37)
#define ERROR_INTERNET_SEC_CERT_CN_INVALID      (INTERNET_ERROR_BASE + 38)
#define ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR   (INTERNET_ERROR_BASE + 39)
#define ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR   (INTERNET_ERROR_BASE + 40)
#define ERROR_INTERNET_MIXED_SECURITY           (INTERNET_ERROR_BASE + 41)
#define ERROR_INTERNET_CHG_POST_IS_NON_SECURE   (INTERNET_ERROR_BASE + 42)
#define ERROR_INTERNET_POST_IS_NON_SECURE       (INTERNET_ERROR_BASE + 43)
#define ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED  (INTERNET_ERROR_BASE + 44)
#define ERROR_INTERNET_INVALID_CA               (INTERNET_ERROR_BASE + 45)
#define ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP    (INTERNET_ERROR_BASE + 46)
#define ERROR_INTERNET_ASYNC_THREAD_FAILED      (INTERNET_ERROR_BASE + 47)
#define ERROR_INTERNET_REDIRECT_SCHEME_CHANGE   (INTERNET_ERROR_BASE + 48)
#define ERROR_INTERNET_DIALOG_PENDING           (INTERNET_ERROR_BASE + 49)
#define ERROR_INTERNET_RETRY_DIALOG             (INTERNET_ERROR_BASE + 50)
#define ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR  (INTERNET_ERROR_BASE + 52)
#define ERROR_INTERNET_INSERT_CDROM             (INTERNET_ERROR_BASE + 53)


//
// FTP API errors
//

#define ERROR_FTP_TRANSFER_IN_PROGRESS          (INTERNET_ERROR_BASE + 110)
#define ERROR_FTP_DROPPED                       (INTERNET_ERROR_BASE + 111)
#define ERROR_FTP_NO_PASSIVE_MODE               (INTERNET_ERROR_BASE + 112)

//
// gopher API errors
//

#define ERROR_GOPHER_PROTOCOL_ERROR             (INTERNET_ERROR_BASE + 130)
#define ERROR_GOPHER_NOT_FILE                   (INTERNET_ERROR_BASE + 131)
#define ERROR_GOPHER_DATA_ERROR                 (INTERNET_ERROR_BASE + 132)
#define ERROR_GOPHER_END_OF_DATA                (INTERNET_ERROR_BASE + 133)
#define ERROR_GOPHER_INVALID_LOCATOR            (INTERNET_ERROR_BASE + 134)
#define ERROR_GOPHER_INCORRECT_LOCATOR_TYPE     (INTERNET_ERROR_BASE + 135)
#define ERROR_GOPHER_NOT_GOPHER_PLUS            (INTERNET_ERROR_BASE + 136)
#define ERROR_GOPHER_ATTRIBUTE_NOT_FOUND        (INTERNET_ERROR_BASE + 137)
#define ERROR_GOPHER_UNKNOWN_LOCATOR            (INTERNET_ERROR_BASE + 138)

//
// HTTP API errors
//

#define ERROR_HTTP_HEADER_NOT_FOUND             (INTERNET_ERROR_BASE + 150)
#define ERROR_HTTP_DOWNLEVEL_SERVER             (INTERNET_ERROR_BASE + 151)
#define ERROR_HTTP_INVALID_SERVER_RESPONSE      (INTERNET_ERROR_BASE + 152)
#define ERROR_HTTP_INVALID_HEADER               (INTERNET_ERROR_BASE + 153)
#define ERROR_HTTP_INVALID_QUERY_REQUEST        (INTERNET_ERROR_BASE + 154)
#define ERROR_HTTP_HEADER_ALREADY_EXISTS        (INTERNET_ERROR_BASE + 155)
#define ERROR_HTTP_REDIRECT_FAILED              (INTERNET_ERROR_BASE + 156)
#define ERROR_HTTP_NOT_REDIRECTED               (INTERNET_ERROR_BASE + 160)
#define ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION    (INTERNET_ERROR_BASE + 161)
#define ERROR_HTTP_COOKIE_DECLINED              (INTERNET_ERROR_BASE + 162)
#define ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION  (INTERNET_ERROR_BASE + 168)

//
// additional Internet API error codes
//

#define ERROR_INTERNET_SECURITY_CHANNEL_ERROR   (INTERNET_ERROR_BASE + 157)
#define ERROR_INTERNET_UNABLE_TO_CACHE_FILE     (INTERNET_ERROR_BASE + 158)
#define ERROR_INTERNET_TCPIP_NOT_INSTALLED      (INTERNET_ERROR_BASE + 159)
#define ERROR_INTERNET_DISCONNECTED             (INTERNET_ERROR_BASE + 163)
#define ERROR_INTERNET_SERVER_UNREACHABLE       (INTERNET_ERROR_BASE + 164)
#define ERROR_INTERNET_PROXY_SERVER_UNREACHABLE (INTERNET_ERROR_BASE + 165)

#define ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT    (INTERNET_ERROR_BASE + 166)
#define ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT (INTERNET_ERROR_BASE + 167)
#define ERROR_INTERNET_SEC_INVALID_CERT    (INTERNET_ERROR_BASE + 169)
#define ERROR_INTERNET_SEC_CERT_REVOKED    (INTERNET_ERROR_BASE + 170)

// InternetAutodial specific errors

#define ERROR_INTERNET_FAILED_DUETOSECURITYCHECK  (INTERNET_ERROR_BASE + 171)

#define INTERNET_ERROR_LAST                       ERROR_INTERNET_FAILED_DUETOSECURITYCHECK
*/
}
  if (Control!=NULL) (Control)(9000,dwError,lpError);
 }
 return TRUE;
}

/*
 // --------------------------------------
 // Chiudo la Internet connessione       |
 // --------------------------------------
 if (hOpen)
 {
  printf("\nChiudo la sezione Internet ...\n");
  InternetCloseHandle(hOpen);
 }

 if (*mess) {ErrorOut(NULL,GetLastError(),mess); exit(1);}
 exit(0);
}
*/

// -------------------------------------------------------------
// Cerca un valore segnalato con apici e lo scrive in Receive  |
// -------------------------------------------------------------

void ThereIsPut(TCHAR *Line,TCHAR *Tag,TCHAR *Receive,DWORD dwSize)
{
 TCHAR b,*p,*p2;
 p=lstrstr(Line,Tag); if (!p) return;
 p+=lstrlen(Tag);
 if (*p=='\"')
  {
   //p=lstrstr(p,"\""); if (!p) return; else
    p++;
    p2=lstrstr(p,TEXT("\"")); if (!p2) return;
    *p2=0;
    if ((LONG) lstrlen(p)>(LONG) dwSize) PRG_end(TEXT("TIS: overload"));
    lstrcpy(Receive,p);
    *p2='"';
  }
  else
  {
    p2=lstrstr(p,TEXT(" "));
    if (!p2) p2=lstrstr(p,TEXT(">"));
    if (!p2) return;
    b=*p2; *p2=0;
    if (lstrlen(p)>dwSize) PRG_end(TEXT("TIS: overload 2"));
    lstrcpy(Receive,p);
    *p2=b;
  }
}

// -------------------------------------------------------------
// Estra una variabile tipo ed la accoda al Form               |
// -------------------------------------------------------------

void VarExtractCat(TCHAR *Line,TCHAR *Tipo,TCHAR *Form,DWORD dwSize)
{
 TCHAR *p,*p2;

 p=lstrstr(Line,TEXT("input")); if (!p) return;
 p=lstrstr(p,TEXT("type=\"")); if (!p) return;
 p+=6;
 if (memcmp(p,Tipo,lstrlen(Tipo))) return;
// p=lstrstr(p,"name="); if (!
 p2=Form+lstrlen(Form);
 ThereIsPut(p,TEXT("name="),p2,dwSize-lstrlen(Form));
 p2+=lstrlen(p2);
 lstrcat(p2,TEXT("="));
 p2++;
 ThereIsPut(p,TEXT("value="),p2,dwSize-lstrlen(Form));
 lstrcat(p2,TEXT("&"));

}

// -------------------------------------------------------------
// Aggiunge una variabile in coda ad una richiesta GET/POST    |
// -------------------------------------------------------------

void VarAddCat(TCHAR *name,TCHAR *value,TCHAR *Form,DWORD dwSize)
{
 lstrcat(Form,name);
 lstrcat(Form,TEXT("="));
 lstrcat(Form,value);
 if (lstrlen(Form)>dwSize) PRG_end(TEXT("TIS: overload 3"));
 lstrcat(Form,TEXT("&"));

}

void VarFinal(TCHAR *Form)
{
 if (!Form) return;
 if (!*Form) return;
 if (Form[lstrlen(Form)-1]=='&') Form[lstrlen(Form)-1]=0;
}

// Estraggo il server di Link
void LinkDivide(TCHAR *Buf,
				TCHAR *Protocol, // Http,Ftp ecc...
				TCHAR *Domain,   // Nome del dominio
				TCHAR *Page,     // Nome della pagina
				TCHAR *Param)   // Parametri se Š un GET mode
{
 TCHAR *p,*p2;

 *Protocol=0;
 *Domain=0;
 *Page=0;
 *Param=0;

 // Tipo di protocollo
 p=lstrstr(Buf,TEXT("://"));
 if (p)
  {
   *p=0;
   lstrcpy(Protocol,Buf);
   *p=':'; Buf=p+3;
  }

 // Cerco il dominio
 p2=lstrstr(Buf,TEXT("/")); if (p2) *p2=0;
 lstrcpy(Domain,Buf);
 if (!p2) return;
 Buf=p2+1;

 // Cerco se Š un GET reguest
 p2=lstrstr(Buf,TEXT("?")); if (p2) *p2=0;
 lstrcpy(Page,Buf);
 if (!p2) return;
 Buf=p2+1;

 lstrcpy(Param,Buf);

}

// -------------------------------------------------------------
// Apro una sessione internet                                  |
// -------------------------------------------------------------

void ISOpen(void)
{
 //printf("\nApro la sezione Internet ....");
 if ( !(hOpen = InternetOpen(TEXT("SpyFA&T"),  // Nome di chi visita
			    INTERNET_OPEN_TYPE_DIRECT,
			    //LOCAL_INTERNET_ACCESS , // Tipo di ricerca del nome
			    NULL,0,0)))
	 {
	  ErrorOut (NULL, NULL,GetLastError(), TEXT("InternetOpen"));
	  //return 0;
	 }
 //printf("Ok\n");
}

static BOOL PassInput(TCHAR *szUser,TCHAR *szPass)
{
  win_info(TEXT("DA FARE:\nRichiesta di user e password per area riservata"));
  return FALSE;

}

// -------------------------------------------------------------
// PUTOUT  Spedisce un messaggio alla sub di controllo         |
// passata alla funzione di WebCopy                            |
// Created by G.Tassistro                         (Maggio 99)  |
// -------------------------------------------------------------

void PutOut(void * (*Control)(INT cmd,LONG Info,TCHAR *),DWORD dwError,TCHAR *Mess,...)
{	
	va_list Ah;
	TCHAR *Buffer;

	Buffer=malloc(2000);
	va_start(Ah,Mess);
#ifdef _UNICODE
	vswprintf(Buffer,Mess,Ah); // Messaggio finale
#else
	vsprintf(Buffer,Mess,Ah); // Messaggio finale
#endif

	// Inserire qui la decodifica di dwError
	switch (dwError)
	{
		case ERROR_INTERNET_CONNECTION_RESET:
			lstrcat(Buffer,TEXT(" : INTERNET_CONNECTION_RESET"));
			break;
	}

    if (Control!=NULL) (Control)(WS_DISPLAY,dwError,Buffer);
#ifdef _CONSOLE
	                   else
                       win_infoarg(Buffer);
#endif
	free(Buffer);
	va_end(Ah);
}

SINT FtpCheckConnect(TCHAR *UserName,        // Indicazione dell'utente
					 TCHAR *PassWord,
					 TCHAR *lpFtpServer		 // Server da Aprire
					 )
{
	HINTERNET hFtpSession=NULL;
	DWORD dwError;
	CHAR *lpBuf=NULL;
	if (!(hFtpSession=InternetConnect(hOpen, // Handle della apertura in internet
									  lpFtpServer, // Nome del server
									  INTERNET_DEFAULT_FTP_PORT, // Porta di accesso 80=HTTP 21=FTP
									  UserName, // User Name
									  PassWord, // Password
									  INTERNET_SERVICE_FTP, // Servizio richiesto HTTP/FTP ecc...
									  0, //Flag ?
									  0))) // dwContext (Funzione ?)
	{
		dwError=GetLastError();
		return dwError;
	}
	// -------------------------------------------
	// Chiudo la Connessione con il server FTP
	// 
	//
	if (hFtpSession!=NULL)
	{
	 if (!InternetCloseHandle(hFtpSession) )
	 {
		dwError=GetLastError();
		return dwError;
	 }
	}
//	return InternetCheckConnection(lpFtpServer,FLAG_ICC_FORCE_CONNECTION ,0);
	return 0;
}



// ---------------------------------------------------------
// Trasferisce un file utilizzando il protocollo FTP
//
//

BOOL FtpSend(TCHAR *lpUserName,        // Indicazione dell'utente
			 TCHAR *lpPassWord,
	         TCHAR *lpFtpServer, // Server da Aprire
			 TCHAR *lpLocalFileSource,
	         TCHAR *lpNewRemoteFile,  // Pagina da richiedere
			 SINT dwBlock, // Byte trasferiti per volta
	         void * (*Control)(INT cmd,LONG Info,TCHAR *), // Funzione di controllo e display
	         BOOL Vedi) // TRUE/FALSE Visione dei passaggi di transazione
{

	HINTERNET hFtpSession=NULL;
	HINTERNET hFtpFile=NULL;
	FILE *pfSource=NULL;
	BOOL iRet=TRUE;
	DWORD dwSize=dwBlock;
	BYTE *lpBuf;
	BOOL fAllOK=FALSE;

	/*
	win_infoarg("User:%s / %s\nServer:%s\nLocalFile:%s\nRemoteFile:%s",
				UserName,PassWord,
				lpFtpServer,
				lpLocalFileSource,
				lpNewRemoteFile);
*/
	if (!f_exist(lpLocalFileSource))
	{
		if (Control) ErrorOut (Control,NULL,0,TEXT("FTP:File Sorgente %s inesistente")); 
					 else
					 win_infoarg(TEXT("FTP:File Sorgente %s inesistente"),lpLocalFileSource);
		return iRet;
	}

#ifdef _UNICODE
    pfSource=_wfopen(lpLocalFileSource,TEXT("rb"));
#else
    pfSource=fopen(lpLocalFileSource,"rb");
#endif

	if (pfSource==NULL)
	{
		//printf("FTP:errore in apertura %s",lpLocalFileSource);
		if (Control)  ErrorOut (Control,NULL,0,TEXT("FTP:errore in apertura file sorgente locale")); 
					  else
					  win_infoarg(TEXT("FTP:errore in apertura file sorgente locale"));
		return iRet;
	}

	// -------------------------------------------------
	// APRO LA CONNESSIONE CON IL SERVER FTP            
	// Se c'è un cambio Server effettuo la connessione 
	// 
	if (Control) (*Control)(WS_DO,0,TEXT("Richiesta connessione FTP"));

	if (!(hFtpSession=InternetConnect(hOpen, // Handle della apertura in internet
									  lpFtpServer, // Nome del server
									  INTERNET_DEFAULT_FTP_PORT, // Porta di accesso 80=HTTP 21=FTP
									  lpUserName, // User Name
									  lpPassWord, // Password
									  INTERNET_SERVICE_FTP, // Servizio richiesto HTTP/FTP ecc...
									  INTERNET_FLAG_PASSIVE, //Flag ? modifica 2002 non era così
									  0))) // dwContext (Funzione ?)
							{
								if (Control) 
									ErrorOut (Control,NULL,GetLastError(),TEXT("InternetConnecterror:")); 
									else
									win_infoarg(TEXT("InternetConnecterror: %d [%s:%s:%s]"),
												GetLastError(),
												lpFtpServer,
												lpUserName,
												lpPassWord,
												ERROR_INTERNET_CANNOT_CONNECT);
								goto MYERRORE;
								//return TRUE;
							}

	// -------------------------------------------------
	// APRO IL FILE
	// 
	if (Control) (*Control)(WS_DO,0,TEXT("Inizio fase di scrittura ..."));

	if (!(hFtpFile=FtpOpenFile(hFtpSession,
							   lpNewRemoteFile,
						       GENERIC_WRITE,
						       FTP_TRANSFER_TYPE_BINARY,
						       0)))
							{
								if (Control) 
									ErrorOut(Control,NULL,GetLastError(), TEXT("FtpOpenFile")); 
									else
									win_infoarg(TEXT("FtpOpenFile: %d [%s:%s:%s]"),
												GetLastError(),
												lpFtpServer,
												lpUserName,
												lpPassWord,
												ERROR_INTERNET_CANNOT_CONNECT);
								goto MYERRORE;
								//return TRUE;
							}

	if (Control)
	{
		(*Control)(WS_DO,0,TEXT("Trasferimento file ..."));
	}


	lpBuf=malloc(dwSize);
	while (TRUE)
	{
		DWORD dwRealSize;
		DWORD dwRemoteWrite;
		BOOL fCheck;

		//dwRealSize=f_get(pfSource,NOSEEK,lpBuf,dwSize);
		dwRealSize=fread(lpBuf,1,dwSize,pfSource);
		//if (dwRealSize<dwSize) dispx("%d",dwRealSize);
		if (dwRealSize==0) {fAllOK=TRUE; break;}
		fCheck=InternetWriteFile(hFtpFile,lpBuf,dwRealSize,&dwRemoteWrite);
		
		// ----------------------------------------
		// Se tutto è OK
		if (fCheck)
		{
			if (Control) (*Control)(WS_PROCESS,dwRealSize,NULL);
		}
		else
		// ----------------------------------------
		// Se qualcosa non và
		{
			if (Control) ErrorOut(Control,NULL,GetLastError(),TEXT("in WriteFile:"));
						 else
						 win_infoarg(TEXT("InternetWriteFile(): %d [%s]"),
										  GetLastError(),
										  lpFtpServer,
										  ERROR_INTERNET_CANNOT_CONNECT);

			fAllOK=FALSE;
			break;
		}

		// ----------------------------------------
		// Controllo se mi devo fermare

		if (Control)
			{
				if ((*Control)(WS_INF,0,NULL)) 
				{
					ErrorOut(Control,NULL,1,TEXT("User break."));
					fAllOK=FALSE;
					break;
				}
			}
	}

	//os_errset(POP);
	free(lpBuf);

	if (Control)
	{
		(*Control)(WS_DO,0,TEXT("Chiusura trasferimento ..."));
	}
	
MYERRORE:

	// -------------------------------------------
	// Chiudo il File Aperto
	// 
	//
	if (hFtpFile!=NULL)
	{
	 if (!InternetCloseHandle(hFtpFile))
	 {
		if (Control) ErrorOut (Control,NULL,GetLastError (), TEXT("CloseFileFtp"));
		fAllOK=FALSE;
		goto MYERRORE;
	 }
	}
	
	// -------------------------------------------
	// Chiudo la Connessione con il server FTP
	// 
	//
	if (hFtpSession!=NULL)
	{
	 if (!InternetCloseHandle (hFtpSession) )
	 {
		ErrorOut (Control,NULL,GetLastError (),TEXT( "CloseHandle on hConnect"));
		fAllOK=FALSE;
		goto MYERRORE;
	 }
	}

	if (pfSource) fclose(pfSource);

	// ---------------------------------------------------
	// Trasmetto il responso della trasmissione
	//
	if (Control) (*Control)(WS_LINK,fAllOK,NULL);
	if (Control) (*Control)(WS_DO,0,TEXT("Fine sessione FTP."));

	if (fAllOK) iRet=FALSE;
return iRet;
}

// ---------------------------------------------
// WebCheckConnect
// ritorna TRUE se si è connessi ad internet
// lp=se si passa un puntatore ad una stringa
//    nella stringa viene scritto il modo in cui si è connessi ad internet
//

BOOL WebCheckConnect(TCHAR *lp)
{
  DWORD dwFlags;//,dwIn;
  BOOL bRet;

  ZeroFill(dwFlags);

//  dwIn=INTERNET_AUTODIAL_FORCE_ONLINE;
  //InternetAutodial(dwIn,0);

  bRet=InternetGetConnectedState(&dwFlags,0);
  if (lp)
  {
	lstrcpy(lp,TEXT("?"));
	switch (dwFlags&0xF)
	{
		case INTERNET_CONNECTION_MODEM: lstrcpy(lp,TEXT("Modem")); break;
		case INTERNET_CONNECTION_LAN: lstrcpy(lp,TEXT("Lan")); break;
		case INTERNET_CONNECTION_PROXY: lstrcpy(lp,TEXT("Proxy")); break;
		case INTERNET_CONNECTION_MODEM_BUSY: lstrcpy(lp,TEXT("Busy")); break;
	}
  }
  return bRet;
}

static struct hostent *hServer;    // Host SMTP
static struct sockaddr_in aServer; // Socket per la comunicazione in SMTP
SINT iServerSocket;

// 0=FTP 1=WEB (porta)
static void LInternetFree()
{
	if (iServerSocket) {closesocket(iServerSocket); iServerSocket=0;}
	WSACleanup();
}

// 0= Tutto ok
// <0 Un errore di collegamento

BOOL FInternetCheck(CHAR *lpAddress,unsigned short usiPort,CHAR *lpBuffer,SINT iSizeBuffer)
{
	SINT iByte;
	WSADATA wsaData;
	
	if(WSAStartup(0x101, &wsaData)) return -10;

	ZeroFill(hServer);
	ZeroFill(aServer);
	hServer=gethostbyname(lpAddress);	  // WinSock - Ritorna una struttura hostest
	if (hServer==NULL) {LInternetFree(); return -1;} // L'host SMTP non esiste

	// -----------------------------------------------------
	// Richiedo socket per porta iPort (SMTP)
	//
	aServer.sin_family = AF_INET;
	aServer.sin_port = htons(usiPort); // 25 = SMTP
	memcpy(&(aServer.sin_addr.s_addr), hServer->h_addr, sizeof(int));
	iServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (iServerSocket==0) {LInternetFree(); return -2;} // Non mi posso creare un socket con l'host SMTP

	//dispx("%d [%s]",iServerSocket,lpAddress);

	// -----------------------------------------------------
	// Richiedo connessione alla porta tramite socket
	//
	if (connect(iServerSocket, (struct sockaddr *)&aServer, sizeof(aServer)))
	{
		LInternetFree(); return -3;
	}

	if (lpBuffer)
	{
		iByte=recv(iServerSocket,lpBuffer,iSizeBuffer,0);
		if (iByte>=0) lpBuffer[iByte]=0;
	}

	LInternetFree();
	return 0;
}

// wininet.lib ws2.lib cellcore.lib

#ifdef _WIN32_WCE

CONNMGR_DESTINATION_INFO cdiCE;
CONNMGR_CONNECTIONINFO ConnInfoCE;
HANDLE hCEConnection=NULL; // Handle della connessione ad Internet

// Prova a connettersi ad Internet su Palmare
// ritorna TRUE se si è connessi
BOOL CEInternetConnect(void)
{
	DWORD dwStatus;
	ZeroFill(cdiCE);
	ConnMgrEnumDestinations(1, &cdiCE); // 1 dovrebbe essere sempre Internet

	ZeroFill(ConnInfoCE);
	ConnInfoCE.cbSize=sizeof(CONNMGR_CONNECTIONINFO);
	ConnInfoCE.dwParams = CONNMGR_PARAM_GUIDDESTNET;
	ConnInfoCE.dwPriority=CONNMGR_PRIORITY_USERINTERACTIVE;
	ConnInfoCE.guidDestNet=cdiCE.guid;
	ConnMgrEstablishConnectionSync(&ConnInfoCE,&hCEConnection,3000,&dwStatus);
	if (dwStatus!=CONNMGR_STATUS_CONNECTED) return FALSE; 
	WebOpen(_T("Ferra"));
	return TRUE;
}

BOOL CEInternetDisconnect(void)
{
	if (hCEConnection==NULL) return FALSE;
	ConnMgrReleaseConnection(hCEConnection,FALSE);
	WebClose();
	hCEConnection=NULL;
	return TRUE;
}

/*
	GUID guid;
	DWORD dwIndex;

	SINT a,b;
	CONNMGR_DESTINATION_INFO cdi;
	CONNMGR_CONNECTIONINFO ConnInfo;
	HANDLE hConnection;
	DWORD dwStatus;
	ZeroFill(cdi);
	ConnMgrEnumDestinations(1, &cdi);
	for (a=0;a<15;a++)
	{
	 if (ConnMgrEnumDestinations(a, &cdi)) break;
	 win_infoarg(L"%d[%s]",b,cdi.szDescription);
	}

	ZeroFill(ConnInfo);
	ConnInfo.cbSize=sizeof(CONNMGR_CONNECTIONINFO);
	ConnInfo.dwParams = CONNMGR_PARAM_GUIDDESTNET;
	//ConnInfo.dwFlags=CONNMGR_FLAG_PROXY_HTTP;
	ConnInfo.dwPriority=CONNMGR_PRIORITY_USERINTERACTIVE;
	memcpy(&ConnInfo.guidDestNet,&cdi.guid,sizeof(GUID));

	//ConnMgrEstablishConnection(&ConnInfo,&hConnection);
	win_infoarg(L"prima");
	ConnMgrEstablishConnectionSync(&ConnInfo,&hConnection,3000,&dwStatus);
	win_infoarg(L"after");


	win_infoarg(L"prima");
	ConnMgrReleaseConnection(hConnection,FALSE);
	win_infoarg(L"after");\
*/

/*
  BOOL bPassword;
  TCHAR szBuffer[100];
  HANDLE hRasConn;
  BOOL fCheck;
  DWORD dwCb,dwDevice;
  RASDEVINFOW *lpRasDevice=NULL;
  SINT a;

  DWORD dwRasSize=sizeof(RASDEVINFOW);
  dwDevice=0;
  lpRasDevice=GlobalAlloc(GPTR,dwRasSize);
  memset(lpRasDevice,0,dwRasSize);
  lpRasDevice->dwSize=dwRasSize;
  dwCb=dwRasSize;
  win_infoarg(L"Entro");
  fCheck=RasEnumDevices(lpRasDevice,&dwCb,&dwDevice); 
  win_infoarg(L"Dopo %d",dwCb);
  if (!dwDevice) {win_infoarg(L"Non sono presenti device"); return TRUE;}
  if (dwCb!=sizeof(RASCONN))
  {
	GlobalFree(lpRasDevice); 
	lpRasDevice=GlobalAlloc(GPTR,dwCb);
	lpRasDevice->dwSize=dwRasSize;
	fCheck=RasEnumDevices(lpRasDevice,&dwCb,&dwDevice); 
  }

  for (a=0;a<dwDevice;a++)
  {
   win_infoarg(L"%d) [%s:%s] %d, %d",a,lpRasDevice->szDeviceType,lpRasDevice->szDeviceName,dwCb,dwDevice);
  }
  GlobalFree(lpRasDevice); 
*/
	/*
//  BOOL fCheck;
  HANDLE g_hMsgQueue;
  MSGQUEUEOPTIONS sOptions;
// RNAAPP as autodialer for AutoRas needs to send feedback
// to Autoas through message queue.
sOptions.dwSize         = sizeof(MSGQUEUEOPTIONS);
sOptions.dwFlags        = MSGQUEUE_ALLOW_BROKEN;
sOptions.dwMaxMessages  = 8;
sOptions.cbMaxMessage   = sizeof(DIALER_NOTIFICATION);
sOptions.bReadAccess    = FALSE;
g_hMsgQueue = CreateMsgQueue(AUTORAS_MSGQUEUE_NAME, &sOptions);

win_infoarg(L"Prima");
if (g_hMsgQueue == NULL)
{
   DEBUGMSG(ZONE_ERROR,
      (TEXT("RNAAPP:: Failed CreateMsgQueue..\r\n")));
}
else
{
   //
   // Autoras is at the other side, send DIALER_START to it.
   //
   g_DialerNotification.dwNotificationId = DIALER_START;

   WriteMsgQueue(
      g_hMsgQueue,
      &g_DialerNotification,
      sizeof(DIALER_NOTIFICATION),
      0x00,
      0x00);
}
*/


/*
  BOOL bPassword;
  TCHAR szBuffer[100];
  HANDLE hRasConn;
  BOOL fCheck;
  DWORD dwCb,dwConn;

  RASCONN *lpRasConn=NULL;
  DWORD dwRasSize=sizeof(RASCONN);
  dwConn=0;
  win_infoarg(L"Entro");
  lpRasConn=GlobalAlloc(GPTR,dwRasSize);
  memset(lpRasConn,0,dwRasSize);
  lpRasConn->dwSize=dwRasSize;
  dwCb=dwRasSize;
  fCheck=RasEnumConnections(lpRasConn,&dwCb,&dwConn); 
  if (!dwConn) {win_infoarg(L"Non sono presenti connessioni"); return TRUE;}
  if (dwCb!=sizeof(RASCONN))
  {
	GlobalFree(lpRasConn); lpRasConn=GlobalAlloc(GPTR,sizeof(RASCONN));
	lpRasConn->dwSize=sizeof(RASCONN);
	fCheck=RasEnumConnections(lpRasConn,&dwCb,&dwConn); 
  }

  win_infoarg(L"[%s] %d, %d",lpRasConn->szEntryName,dwCb,dwConn);
  GlobalFree(lpRasConn); 
  */
  /*
  // Non funziona
  win_infoarg(L"Prima");
  fCheck=InternetGoOnline(L"www.marein.it",NULL,0);
  win_infoarg(L"Dopo %d",fCheck);
*/
/*
  static RASDIALPARAMS RasDialParams;
  
  
    // Set hRasConn to NULL before attempting to connect.
    hRasConn = NULL;

    // Initialize the structure.
    //memset (&RasDialParams, 0, sizeof (RASDIALPARAMS));
	ZeroFill(RasDialParams);
    // Configure the RASDIALPARAMS structure. 
    RasDialParams.dwSize = sizeof (RASDIALPARAMS);
    RasDialParams.szPhoneNumber[0] = TEXT('\0');
    RasDialParams.szCallbackNumber[0] = TEXT('\0');
    wcscpy (RasDialParams.szEntryName, L"unitim");
    wcscpy (RasDialParams.szUserName, L"3355232394"); //This is optional    wcscpy (RasDialParams.szPassword, szPassword); //This is optional
    wcscpy (RasDialParams.szDomain, L"448899"); //This is optional

	// Try to establish RAS connection.
  if (RasDial (NULL,            // Extension not supported
               NULL,            // Phone book is in registry
               &RasDialParams,  // RAS configuration for connection
               0xFFFFFFFF,      // Notifier type is a window handle
               hDlgWnd,         // Window receives notification message
               &hRasConn) != 0) // Connection handle
  {
	  win_infoarg(TEXT("Could not connect using RAS"));
    return FALSE;
  }
*/

/*
  if (bUseCurrent)
  {
    // Get the last configuration parameters used for this connection. 
    // If the password was saved, then the logon dialog box will not be
    // displayed.
    if (RasGetEntryDialParams (NULL, &RasDialParams, &bPassword) != 0)
    {
      MessageBox (hDlgWnd, 
                  TEXT("Could not get parameter details"), 
                  szTitle, 
                  MB_OK);
      return FALSE;
    }
  }
  else
  {
    // Display the Authentication dialog box.
    DialogBox (hInst, MAKEINTRESOURCE(IDD_AUTHDLG), hDlgWnd, 
               AuthDlgProc);

    // Set hRasConn to NULL before attempting to connect.
    hRasConn = NULL;

    // Initialize the structure.
    memset (&RasDialParams, 0, sizeof (RASDIALPARAMS));

    // Configure the RASDIALPARAMS structure. 
    RasDialParams.dwSize = sizeof (RASDIALPARAMS);
    RasDialParams.szPhoneNumber[0] = TEXT('\0');
    RasDialParams.szCallbackNumber[0] = TEXT('\0');
    wcscpy (RasDialParams.szEntryName, szRasEntryName);
    wcscpy (RasDialParams.szUserName, szUserName); //This is optional    wcscpy (RasDialParams.szPassword, szPassword); //This is optional
    wcscpy (RasDialParams.szDomain, szDomain); //This is optional
  }

  // Try to establish RAS connection.
  if (RasDial (NULL,            // Extension not supported
               NULL,            // Phone book is in registry
               &RasDialParams,  // RAS configuration for connection
               0xFFFFFFFF,      // Notifier type is a window handle
               hDlgWnd,         // Window receives notification message
               &hRasConn) != 0) // Connection handle
  {
    MessageBox (hDlgWnd, 
                TEXT("Could not connect using RAS"), 
                szTitle, 
                MB_OK);
    return FALSE;
  }

  wsprintf (szBuffer, TEXT("Dialing %s..."), szRasEntryName);

  // Set the Dialing dialog box window name to szBuffer.
  SetWindowText (hDlgWnd, szBuffer);

  return TRUE;
  */

#endif

