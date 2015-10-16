// -------------------------------------------------------------
// WEBTOOL   Tool per l'accesso ad internet                    |
// by Ferrà Art & Technology                                   |
// Created by G.Tassistro   (Gennaio 99)                       |
// Revisioned by G.Tassistro 2004 (x Pocket PC 2003)
// -------------------------------------------------------------

#ifndef WITH_OPENSSL 
//#define WITH_OPENSSL
#endif

#include "/easyhand/inc/easyhand.h"

#ifdef EH_INTERNET

#include "webtool.h" 
#include <time.h>
#include <excpt.h>
#define WT_USER_AGENT "Mozilla/5.0 (compatible; Konqueror/3.2; Linux; Ferra srl)"

//
// Richiesto SSL x Https
//
#ifdef WITH_OPENSSL
#pragma message("--> + /easyhand/ehtoolx/OpenSSL - GnuWin32/lib/libcrypto.lib <--------------------")
#pragma comment(lib, "/easyhand/ehtoolx/OpenSSL - GnuWin32/lib/libcrypto.lib")
#pragma message("--> + /easyhand/ehtoolx/OpenSSL - GnuWin32/lib/libssl.lib <--------------------")
#pragma comment(lib, "/easyhand/ehtoolx/OpenSSL - GnuWin32/lib/libssl.lib")

static int wt_ssl_auth_init(FWEBSERVER *pWS);
static int ssl_verify_callback(int ok, X509_STORE_CTX *store);
#endif


#ifdef _WIN32_WCE
#include "connmgr.h"
// WindowCE:Attenzione Server cellcore.lib nel link
#endif

#define WT_INVALID_SOCKET (-1)
#define wt_valid_socket(n) ((n) != WT_INVALID_SOCKET)
#define wt_socket_errno WSAGetLastError()

static int eh_tcp_gethost(FWEBSERVER *ws, const char *addr, struct in_addr *inaddr);

// -------------------------------------------------------------
// WEBCOPY  Preleva una pagina WEB e la scrive su disco        
//                                                             
// Created by G.Tassistro   (Gennaio)                          
// -------------------------------------------------------------

#define TAG_LEN "@#LENGTH#@"

static HINTERNET hOpen;

//extern CHAR LastServer[];

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

static BOOL PassInput(CHAR *szUser,CHAR *szPass);
static BOOL _errorOut (EH_WEB * psWeb,FILE *pf,DWORD dError,CHAR *szCallFunc);
void PutOut(EH_WEB * psWeb,DWORD dwError,CHAR *Mess,...);
static void _WebGetFreeResource(FWEBSERVER *psWS);

static BOOL bInitialize=TRUE;

// new 2009 - struttura di controllo generale
typedef struct {


	// Per sicronizzare il Thread
	//CRITICAL_SECTION csMT;
	CRITICAL_SECTION csGetHost;
	EH_AR arHost;
	//INT fMTReset=TRUE;


} S_WEBTOOL;
static S_WEBTOOL sWebTool;

void WebClenaup(INT iErr) 
{
	//WSACleanup();
	//DeleteCriticalSection(&sWebTool.csMT); 
	
	DeleteCriticalSection(&sWebTool.csGetHost); 
	ARDestroy(sWebTool.arHost);

}
void WebStartup(void) // Una tantum carica la DLL in memoria per i sockets
{
	if (!bInitialize) return;
	ehAddExit(&WebClenaup); 
	if (!sys.bSocketReady) ehSocketInit();
	ZeroFill(sWebTool);
	//InitializeCriticalSection(&sWebTool.csMT); 
	InitializeCriticalSection(&sWebTool.csGetHost);
	sWebTool.arHost=ARNew();

	bInitialize=FALSE;
	
}


void WebToolStart(void)
{
  hOpen=NULL;
}

// WEBOPEN Apro la gestione Windows di internet
// ritorna: FALSE = OK,  TRUE=Error
BOOL WebOpen(CHAR *User)
{
	WebStartup();
	hOpen = InternetOpen(
		User,
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		0);
	if (!hOpen)
	 {
	  _errorOut(NULL,NULL,osGetError(),TEXT("InternetOpen"));
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

void webPreset(EH_WEB * psWeb,void * (*funcNotify)(EH_WEB * psFtp,EH_SRVPARAMS),void * pParam) {

	memset(psWeb,0,sizeof(EH_WEB));
	psWeb->funcNotify=funcNotify;
	psWeb->pVoid=pParam;

}

// Ritorna FALSE=Tutto OK TRUE=Errore
BOOL WebCopy(CHAR *User,        // Indicazione dell'utente
	         CHAR *lpWebServer, // Server da Aprire
	         CHAR *lpPage,      // Pagina da richiedere
	         CHAR *lpMethod,    // Metodo (GET/POST)
	         CHAR *lpDati,      // Dati da inviare
	         CHAR *lpFileName,  // File name da scrivere con il nome della pagina
	         void * (*funcNotify)(EH_WEB * psFtp,EH_SRVPARAMS), // Funzione di controllo e display
	         BOOL bVedi, // TRUE/FALSE Visione dei passaggi di transazione
			 BOOL fSecure) // TRUE/FALSE Se transazione sicura
{
	HINTERNET  hConnect=NULL;
	HINTERNET  hReq=NULL;
	DWORD  dwSize, dwCode;
	CHAR   szHeaders[100],*lpHeaders=szHeaders;
	DWORD  dwHeaders;
	FILE *pf=NULL;
	DWORD dwDati;
	BYTE  *lpDatiBuf=NULL;
	CHAR  *lpBuffer=NULL;
	DWORD Count=0;
	BOOL MethodPost=FALSE;
	CHAR *lpIBuf=NULL;
//	INT iSizeBuffer=1024;
	INT iSizeBuffer=200000;
	const CHAR *Accept[]={TEXT("text/*"),TEXT("*/*"),NULL};
	BOOL fRet=TRUE;

	EH_WEB sWeb;
	webPreset(&sWeb,funcNotify,NULL);

	WebStartup();

	if (lpFileName)
	{
#ifdef _UNICODE
		pf=_wfopen(lpFileName,_T("wb"));
#else
		pf=fopen(lpFileName,"wb");
#endif
		if (!pf)
		{_errorOut(&sWeb,NULL,osGetError(), TEXT("File in save"));
		 goto MYERRORE;
		}
	} else pf=NULL;

	// -------------------------------------------------
	// CONNESSIONE CON IL SERVER                       |
	// Se c'è un cambio Server effettuo la connessione |
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

	if (bVedi) PutOut(&sWeb,0,TEXT("URL: http://%s/%s\n"),lpWebServer,lpPage);
	if (bVedi) PutOut(&sWeb,0,TEXT("Server %s connection ...."),lpWebServer);
	//lstrcpy(LastServer,lpWebServer);
	if (!(hConnect=InternetConnect(hOpen, // Handle della apertura in internet
								   lpWebServer, // Nome del server
								   fSecure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, // Porta di accesso 80=HTTP
								   //INTERNET_DEFAULT_HTTP_PORT, // Porta di accesso 80=HTTP
								   NULL, // User Name
								   NULL, // Password
								   INTERNET_SERVICE_HTTP, // Servizio richiesto HTTP/FTP ecc...
								   0, //Flag ?
								   0))) // dwContext (Funzione ?)
								{
									_errorOut(&sWeb,pf,osGetError(), TEXT("InternetConnect")); 
									goto MYERRORE;
									//return TRUE;
								}
	if (bVedi) PutOut(&sWeb,0,TEXT(" ok" CRLF));
	
	// -----------------------------------------------\
	// Preparo una richiesta di apertura             |
	// -----------------------------------------------

	lpDatiBuf=ehAlloc(8000);//new CHAR [4000];
	strcpy(lpDatiBuf,lpPage);
	if (lpMethod!=NULL)
	{
		 if (!strCaseCmp(lpMethod,TEXT("GET")))
		 {
			strcat(lpDatiBuf,TEXT("?"));
			strcat(lpDatiBuf,lpDati);
		 }
		 if (!strCaseCmp(lpMethod,TEXT("POST"))) MethodPost=TRUE;
	}
	
	// Accept sono i file che posso accettare in risposta
	if (!(hReq=HttpOpenRequest(hConnect,     // Handle della connessione
							   lpMethod,
							   lpDatiBuf,    // Nome della pagina o del comando da aprire
							   NULL,//HTTP_VERSION, // Versione dell'http
							   TEXT(""),   // Documento che fà la richiesta
							   NULL,//Accept, // Indica cosa accetta il client (Es. "text/*")
							   //fSecure ? INTERNET_FLAG_IGNORE_CERT_CN_INVALID |INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |INTERNET_FLAG_SECURE|INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE : INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE, // Modo di lettura ed usa della cache
							   fSecure?INTERNET_FLAG_SECURE:0,//(INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE)|fSecure ? INTERNET_FLAG_SECURE : 0, // Modo di lettura ed usa della cache
							   0))) // dwContext
	{
		_errorOut(&sWeb,pf,osGetError(),TEXT("HttpOpenRequest")); 
		goto MYERRORE;
	}

//	win_infoarg("Request ok");
	if (bVedi) PutOut(&sWeb,0,TEXT( CRLF "Post HttpOpenRequest() ...."));
	
	lpIBuf=ehAlloc(iSizeBuffer);

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
		/*
		if (fSecure)
		{
			lpHeaders=NULL;
			dwHeaders=0;
			dwDati=0;
			lpDati=NULL;
		}
		else
		*/
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
	if (bVedi) PutOut(&sWeb,0,TEXT("\rPre HttpSendRequest() ...."));
	// printf("####[%s]",lpHeaders);
	strcpy(lpDatiBuf,lpDati);
	if (!HttpSendRequest(hReq, // Handle della richiesta
						 lpHeaders,
		                 dwHeaders,//-1L,
		                 lpDatiBuf,
		                 dwDati))
	{
	 INT iErr=osGetError();
	 osError(FALSE,iErr,"HttpSend");
	 _errorOut(&sWeb,pf,iErr,TEXT("HttpSend")); 
	 goto MYERRORE;
	 //return TRUE;
	}

	//printf("[%s] %d",lpDatiBuf,dwHeaders);
	//ehFree(lpDatiBuf); lpDatiBuf=NULL;
	dwSize = sizeof (DWORD) ;
	if (bVedi) PutOut(&sWeb,0,TEXT( CRLF "Post HttpSendRequest() ...."));

	// -----------------------------------------------
	// Chiedo i flag del Query                       |
	// -----------------------------------------------
	if (!HttpQueryInfo(hReq, // Handle della richiesta
					   HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
					   &dwCode, // Buffer che riceve
		               &dwSize, // Lunghezza del buffer
		               NULL)) // Indice enumerativo ?
	{
	 _errorOut(&sWeb,pf,osGetError(),TEXT("HttpQueryInfo"));  goto MYERRORE;
	 //return TRUE;
	}

	if (bVedi) PutOut(&sWeb,0,TEXT("\rPost HttpQueryInfo() ...."));

	// -----------------------------------------------
	// Se l'accesso ‚ negato richiedo la Password    |
	// -----------------------------------------------
	if (dwCode == HTTP_STATUS_DENIED ||dwCode == HTTP_STATUS_PROXY_AUTH_REQ)
	{
#ifdef _CONSOLE
	 _errorOut(&sWeb,pf,osGetError(), "HTTP_STATUS_DENIED"); 
	 goto MYERRORE;
#else
	 CHAR szUser[50]=TEXT("");
	 CHAR szPass[50]=TEXT("");
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
       //cerr << "InternetSetOptionFailed: " << osGetError() << endl;
	   win_infoarg(TEXT("InternetSetOptionFailed(1): %d"),osGetError());
       goto MYERRORE;
	   //return TRUE;
     }

	 if (!InternetSetOption(hConnect,INTERNET_OPTION_PASSWORD,(LPVOID) szPass,lstrlen (szPass)))
     {
       //cerr << "InternetSetOptionFailed: " << osGetError() << endl;
	   win_infoarg(TEXT("InternetSetOptionFailed(2): %d"),osGetError());
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
			  _errorOut(&sWeb,pf,osGetError(),TEXT("HttpQueryInfo(1)")); 
			  goto MYERRORE;
			}

	//lpBuffer =  new CHAR [dwSize + 1 ];
	lpBuffer=ehAlloc(dwSize + 1);

	// -------------------------------------------------------
	// Dopo aver dimensionato il buffer carichiamo l'header  |
	// -------------------------------------------------------
	// Now we call HttpQueryInfo again to get the headers.
	if (bVedi) PutOut(&sWeb,0,TEXT("\rPre HttpQueryInfo() ...."));
	if (!HttpQueryInfo(hReq,
					   HTTP_QUERY_RAW_HEADERS_CRLF,
					   (LPVOID) lpBuffer, // CArica il Buffer
					   &dwSize,
					   NULL))
			{
			  _errorOut(&sWeb,pf,osGetError(),TEXT("HttpQueryInfo")); 
			  //win_infoarg("HttpQueryInfo [%d]",osGetError());
			  goto MYERRORE;
			  //return TRUE;
			}
	
	//printf("[%s]",lpBuffer);
	*(lpBuffer + dwSize) = '\0';  // Metto il terminatore nel buffer
	if (bVedi) PutOut(&sWeb,0,TEXT("\rPost HttpQueryInfo() ...."));
/*
#ifndef _UNICODE
	if (!strstr(lpBuffer,TEXT("200 OK")))
	{
		_errorOut (funcNotify,pf,osGetError(), "NOT 200 OK!"); 
		goto MYERRORE;
	}
#endif
*/

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
	if (bVedi) PutOut(&sWeb,0,TEXT("\nDownLoad ...\n"));
	if (bVedi) PutOut(&sWeb,0,TEXT("Attendere prego ..."));
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
			_errorOut(&sWeb,pf,osGetError (),TEXT("InternetReadFile")); 
			win_infoarg(TEXT("InternetReadFile [%d]"),osGetError());
			goto MYERRORE;
			//efx2();continue;
		}

		//printf("[%d]",dwSize); getch();
		if (!dwSize) break;
/*
			if (_kbhit())
				{
					if (waitKey()==27) ehExit("User Escape !");
				}
				*/
		
		//eventGet(NULL); if (key_press(ESC)) break;//ehExit("User Escape !");
//		if (_kbhit()) break;
		
//		lpIBuf[dwSize]='\0'; if (pf) fprintf(pf,lpIBuf);
		fwrite(lpIBuf,dwSize,1,pf);
		Count+=dwSize;

        // Monitoraggio dei Byte letti
		if (bVedi) PutOut(&sWeb,0,TEXT("\r%s:>> %d byte                 "),User,(INT) Count);
		
		// Spedisco i Byte letti alla funzione di controllo
		if (funcNotify) 
		{
			(funcNotify)(&sWeb,WS_DO,(LONG) dwSize,lpIBuf);
			(funcNotify)(&sWeb,WS_COUNT,(LONG) Count,NULL);
		}

	} while (TRUE);

	if (bVedi) PutOut(&sWeb,0,TEXT("\rTotale letti: %d byte\n"),(INT) Count);
/*
	//printf("Letti [%d]",Count);
	// --------------------------------------
	// Chiudo la richiesta                  |
	// --------------------------------------
	if (!InternetCloseHandle (hReq) )
	{
		_errorOut (funcNotify,pf,osGetError (), "CloseHandle on hReq");
		goto MYERRORE;
	}

	// --------------------------------------
	// Chiudo la Connessione                |
	// --------------------------------------
	if (!InternetCloseHandle (hConnect) )
	{
		_errorOut (funcNotify,pf,osGetError (), "CloseHandle on hConnect");
		goto MYERRORE;
	}
	
	ehFree(lpBuffer);
	if (lpIBuf) ehFree(lpIBuf);
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
		_errorOut(&sWeb,pf,osGetError (),TEXT("CloseHandle on hReq"));
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
		_errorOut(&sWeb,pf,osGetError (),TEXT("CloseHandle on hConnect"));
		goto MYERRORE;
	 }
	}

	if (pf) {fclose(pf); pf=NULL;}
	ehFreeNN(lpBuffer);
	ehFreeNN(lpDatiBuf);
	ehFreeNN(lpIBuf);
	return fRet;
}


// -------------------------------------------------------
// Mostra gli errori                                     
// 
//
static BOOL _errorOut(EH_WEB * psWeb,FILE *pf,DWORD dwError, CHAR * szCallFunc)
{
	CHAR szTemp[100] =TEXT(""),*pszBuffer=NULL,*pszBufferFinal = NULL;
	CHAR *lpError=TEXT("?");
	DWORD  dwIntError , dwLength = 0;

	switch (dwError)
	{
		case ERROR_HTTP_INVALID_SERVER_RESPONSE: lpError=TEXT("ERROR_HTTP_INVALID_SERVER_RESPONSE"); break;
		case ERROR_INTERNET_DISCONNECTED: lpError=TEXT("ERROR_INTERNET_DISCONNECTED"); break;
		case ERROR_INTERNET_NAME_NOT_RESOLVED: lpError=TEXT(" ERROR_INTERNET_NAME_NOT_RESOLVED"); break;
	}

	if (pf) fprintf(pf,  "<! FerraERR_%s error %d>\n", szCallFunc, dwError );
#ifdef _UNICODE
	swprintf(szTemp,TEXT("%s error %d (%s)\n "),szCallFunc,dwError,lpError);
#else
	sprintf(szTemp,"%s error %d (%s)\n ",szCallFunc,dwError,lpError);
#endif

	//if (dwError == ERROR_INTERNET_EXTENDED_ERROR)
	//  {
	InternetGetLastResponseInfo(&dwIntError, NULL, &dwLength);

	if (dwLength)
	{
		if ( !(pszBuffer = (CHAR *) LocalAlloc(LPTR,dwLength) ) )
		{
			lstrcat(szTemp, TEXT ( "Non posso allocare memoria per visualizzare l'errore: ") );
			lstrcat(szTemp, ITO (osGetError(), pszBuffer, 10)   );
			lstrcat(szTemp, TEXT ("\n") );
			//cerr << szTemp << endl;
			PutOut(psWeb,dwError,szTemp);
			return FALSE;
		}
		if (!InternetGetLastResponseInfo (&dwIntError, (LPTSTR) pszBuffer, &dwLength))
		{
			lstrcat (szTemp, TEXT ( "Unable to get Intrnet error. Error code: ") );
			lstrcat (szTemp, ITO (osGetError(), pszBuffer, 10)  );
			lstrcat (szTemp, TEXT ("\n") );
			//		cerr << szTemp << endl;
			PutOut(psWeb,dwError,szTemp);
			return FALSE;
		}
		if ( !(pszBufferFinal = (CHAR *) LocalAlloc ( LPTR,  (lstrlen (pszBuffer) +lstrlen (szTemp) + 1)  ) )  )
		{
			lstrcat (szTemp, TEXT ( "Unable to allocate memory. Error code: ") );
			lstrcat (szTemp, ITO (osGetError(), pszBuffer, 10)  );
			lstrcat (szTemp, TEXT ("\n") );
			//		cerr << szTemp << endl;
			PutOut(psWeb,dwError,szTemp);
			return FALSE;
		}

		strcpy(pszBufferFinal, szTemp);
		lstrcat(pszBufferFinal, pszBuffer);
		LocalFree(pszBuffer);
		//cerr <<  szBufferFinal  << endl;
		PutOut(psWeb,dwIntError,pszBufferFinal);
		ehLogWrite("InternetError:%s",pszBufferFinal);
		printf(CRLF "(!) InternetError:%s",pszBufferFinal);
		if (psWeb->funcNotify) psWeb->funcNotify(psWeb,9000,dwError,pszBufferFinal);
		LocalFree(pszBufferFinal);
	}
// }
// else
// {
//  PutOut(&sWeb,dwError,szTemp);
//  switch (dwError)
 // {

//	case ERROR_INTERNET_CONNECTION_ABORTED: lpError=TEXT("ERROR_INTERNET_CONNECTION_ABORTED"); break;
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
//}
//  if (funcNotify) (funcNotify)(9000,dwError,lpError);
// }
 ehLogWrite("WebError:%d:%s",dwError,lpError);
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

 if (*mess) {_errorOut(NULL,osGetError(),mess); exit(1);}
 exit(0);
}
*/

// -------------------------------------------------------------
// Cerca un valore segnalato con apici e lo scrive in Receive  |
// -------------------------------------------------------------

void ThereIsPut(CHAR *Line,CHAR *Tag,CHAR *Receive,DWORD dwSize)
{
 CHAR b,*p,*p2;
 p=lstrstr(Line,Tag); if (!p) return;
 p+=lstrlen(Tag);
 if (*p=='\"')
  {
   //p=lstrstr(p,"\""); if (!p) return; else
    p++;
    p2=lstrstr(p,TEXT("\"")); if (!p2) return;
    *p2=0;
    if ((LONG) lstrlen(p)>(LONG) dwSize) ehExit(TEXT("TIS: overload"));
    strcpy(Receive,p);
    *p2='"';
  }
  else
  {
    p2=lstrstr(p,TEXT(" "));
    if (!p2) p2=lstrstr(p,TEXT(">"));
    if (!p2) return;
    b=*p2; *p2=0;
    if (lstrlen(p)>(LONG) dwSize) ehExit(TEXT("TIS: overload 2"));
    strcpy(Receive,p);
    *p2=b;
  }
}

// -------------------------------------------------------------
// Estra una variabile tipo ed la accoda al Form               |
// -------------------------------------------------------------

void VarExtractCat(CHAR *Line,CHAR *Tipo,CHAR *Form,DWORD dwSize)
{
 CHAR *p,*p2;

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

void VarAddCat(CHAR *name,CHAR *value,CHAR *Form,DWORD dwSize)
{
 lstrcat(Form,name);
 lstrcat(Form,TEXT("="));
 lstrcat(Form,value);
 if (lstrlen(Form)>(LONG) dwSize) ehExit(TEXT("TIS: overload 3"));
 lstrcat(Form,TEXT("&"));

}

void VarFinal(CHAR *Form)
{
 if (!Form) return;
 if (!*Form) return;
 if (Form[lstrlen(Form)-1]=='&') Form[lstrlen(Form)-1]=0;
}

// Estraggo il server di Link
void LinkDivide(CHAR *Buf,
				CHAR *Protocol, // Http,Ftp ecc...
				CHAR *Domain,   // Nome del dominio
				CHAR *Page,     // Nome della pagina
				CHAR *Param)   // Parametri se è un GET mode
{
 CHAR *p,*p2;

 *Protocol=0;
 *Domain=0;
 *Page=0;
 *Param=0;

 // Tipo di protocollo
 p=lstrstr(Buf,TEXT("://"));
 if (p)
  {
   *p=0;
   strcpy(Protocol,Buf);
   *p=':'; Buf=p+3;
  }

 // Cerco il dominio
 p2=lstrstr(Buf,TEXT("/")); if (p2) *p2=0;
 strcpy(Domain,Buf);
 if (!p2) return;
 Buf=p2+1;

 // Cerco se è un GET reguest
 p2=lstrstr(Buf,TEXT("?")); if (p2) *p2=0;
 strcpy(Page,Buf);
 if (!p2) return;
 Buf=p2+1;

 strcpy(Param,Buf);

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
	  _errorOut (NULL, NULL,osGetError(), TEXT("InternetOpen"));
	  //return 0;
	 }
 //printf("Ok\n");
}

static BOOL PassInput(CHAR *szUser,CHAR *szPass)
{
  alert(TEXT("DA FARE:\nRichiesta di user e password per area riservata"));
  return FALSE;

}

// -------------------------------------------------------------
// PUTOUT  Spedisce un messaggio alla sub di controllo         |
// passata alla funzione di WebCopy                            |
// Created by G.Tassistro                         (Maggio 99)  |
// -------------------------------------------------------------

void PutOut(EH_WEB * psWeb,DWORD dwError,CHAR *Mess,...)
{	
	va_list Ah;
	CHAR *Buffer;

	Buffer=ehAlloc(2000);
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

    if (psWeb->funcNotify) psWeb->funcNotify(psWeb,WS_DISPLAY,dwError,Buffer);
#ifdef _CONSOLE
	                   else
                       win_infoarg(Buffer);
#endif
	ehFree(Buffer);
	va_end(Ah);
}

INT FtpCheckConnect(CHAR *UserName,        // Indicazione dell'utente
					 CHAR *PassWord,
					 CHAR *lpFtpServer		 // Server da Aprire
					 )
{
	HINTERNET hFtpSession=NULL;
	DWORD dwError;
	CHAR *lpBuf=NULL;

	WebStartup();

	if (!(hFtpSession=InternetConnect(hOpen, // Handle della apertura in internet
									  lpFtpServer, // Nome del server
									  INTERNET_DEFAULT_FTP_PORT, // Porta di accesso 80=HTTP 21=FTP
									  UserName, // User Name
									  PassWord, // Password
									  INTERNET_SERVICE_FTP, // Servizio richiesto HTTP/FTP ecc...
									  0, //Flag ?
									  0))) // dwContext (Funzione ?)
	{
		dwError=osGetError();
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
		dwError=osGetError();
		return dwError;
	 }
	}
//	return InternetCheckConnection(lpFtpServer,FLAG_ICC_FORCE_CONNECTION ,0);
	return 0;
}
CHAR * InternetLastError(BOOL bAlloc,BOOL bShow)
{
	DWORD dwSize=32000;
	DWORD dwErr;
	CHAR *pBuffer=ehAlloc(dwSize);
	dwErr=osGetError();

	InternetGetLastResponseInfo(&dwErr,pBuffer,&dwSize);
	if (bShow) win_infoarg("%s",pBuffer);
	if (!bAlloc) 
		{ehFree(pBuffer); return NULL;}
		else 
		{return pBuffer;}
}

// 
// FtpSend()
// Trasferisce un file utilizzando il protocollo FTP
// Ritorna FALSE se tutto OK
//
BOOL FtpSend(CHAR *		lpUserName,        // Indicazione dell'utente
			 CHAR *		lpPassWord,
	         CHAR *		lpFtpServer, // Server da Aprire
			 CHAR *		lpLocalFileSource,
	         CHAR *		lpNewRemoteFile,  // Pagina da richiedere
			 INT		dwBlock, // Byte trasferiti per volta
	         void *		(*funcNotify)(EH_WEB * psFtp,EH_SRVPARAMS), // Funzione di controllo e display
	         BOOL		Vedi,
			 INT *		lpiError,
			 void *		pVoidParam) // TRUE/FALSE Visione dei passaggi di transazione
{
	HINTERNET hFtpSession=NULL,hFtpFile=NULL;
	FILE *	pfSource=NULL;
	BOOL	iRet=TRUE;
	DWORD	dwSize=dwBlock;
	BYTE *	lpBuf;
	BOOL	fAllOK=FALSE;
	BOOL	bError;
	CHAR	szFtpHostName[512];

	EH_WEB sWeb;
	webPreset(&sWeb,funcNotify,pVoidParam);

	WebStartup();

	if (!hOpen) WebOpen("Ferra");
	if (lpiError) *lpiError=0;
	if (!fileCheck(lpLocalFileSource))
	{
		if (funcNotify)
			_errorOut(&sWeb,NULL,0,TEXT("FTP:File Sorgente %s inesistente")); 
			else
			win_infoarg(TEXT("FTP:File Sorgente %s inesistente"),lpLocalFileSource);
		return iRet;
	}

	// if (*lpNewRemoteFile=='/') lpNewRemoteFile++;

//	if (!strBegin(lpFtpServer,"ftp:"))
	strcpy(szFtpHostName,lpFtpServer);
//		else
//		sprintf(szFtpHostName,"ftp://%s",lpFtpServer);

	// -------------------------------------------------
	// APRO LA CONNESSIONE CON IL SERVER FTP            
	// Se c'è un cambio Server effettuo la connessione 
	// 
	if (funcNotify) funcNotify(&sWeb,WS_DO,0,TEXT("Richiesta connessione FTP"));

	bError=0;
	if (!(hFtpSession=InternetConnect(hOpen, // Handle della apertura in internet
									  szFtpHostName, // Nome del server
									  INTERNET_DEFAULT_FTP_PORT, // Porta di accesso 80=HTTP 21=FTP
									  lpUserName, // User Name
									  lpPassWord, // Password
									  INTERNET_SERVICE_FTP, // Servizio richiesto HTTP/FTP ecc...
									  INTERNET_FLAG_PASSIVE, //Flag ? modifica 2002 non era così
									  0))) // dwContext (Funzione ?)
	{
		bError=true;
		if (funcNotify) 
			_errorOut(&sWeb,NULL,osGetError(),TEXT("InternetConnecterror:")); 
	}


	if (bError) {
		CHAR szMicroUser[500];
		bError=false;
		sprintf(szMicroUser,"%s|%s",lpFtpServer,lpUserName);
		if (!(hFtpSession=InternetConnect(hOpen, // Handle della apertura in internet
										  szFtpHostName, // Nome del server
										  INTERNET_DEFAULT_FTP_PORT, // Porta di accesso 80=HTTP 21=FTP
										  szMicroUser, // User Name
										  lpPassWord, // Password
										  INTERNET_SERVICE_FTP, // Servizio richiesto HTTP/FTP ecc...
										  INTERNET_FLAG_PASSIVE, //Flag ? modifica 2002 non era così
										  0))) // dwContext (Funzione ?)
		{
			bError=true;
		}
	}

	if (bError) {
	
		if (lpiError) * lpiError=osGetError();
		if (funcNotify) 
			_errorOut(&sWeb,NULL,osGetError(),TEXT("InternetConnecterror:")); 
			else
			win_infoarg(TEXT("InternetConnecterror: %d [%s:%s:%s]"),
						osGetError(),
						lpFtpServer,
						lpUserName,
						lpPassWord,
						ERROR_INTERNET_CANNOT_CONNECT);
		
		goto MYERRORE;
	
	}

	//
	// Trasferimento non a blocchi
	//
	if (!dwBlock)
	{

		if (!FtpPutFile(hFtpSession,
						lpLocalFileSource,
						lpNewRemoteFile,
						FTP_TRANSFER_TYPE_BINARY,
						0))
		{

			// InternetLastError(FALSE,TRUE);

			if (lpiError) *lpiError=osGetError();
			/*
			printf("FtpPutFile: %d [%s:%s:%s]",
						osGetError(),
						lpFtpServer,
						lpUserName,
						lpPassWord,
						ERROR_INTERNET_CANNOT_CONNECT);
*/
			if (funcNotify) 
				_errorOut(&sWeb,NULL,osGetError(),TEXT("FtpPutFile:")); 
				else
				{
				win_infoarg(TEXT("FtpPutFile: %d [%s:%s:%s]"),
							osGetError(),
							lpFtpServer,
							lpUserName,
							lpPassWord,
							ERROR_INTERNET_CANNOT_CONNECT);
			}
			goto MYERRORE;
		}	
		fAllOK=TRUE;
		iRet=FALSE;
		goto MYERRORE;
	}

//
// Trasferimento con ftpput ( a blocchi)
//
	sWeb.dwDataBuffer=dwBlock;
	sWeb.dwDataLength=fileSize(lpLocalFileSource);

#ifdef _UNICODE
	pfSource=_wfopen(lpLocalFileSource,TEXT("rb"));
#else
	pfSource=fopen(lpLocalFileSource,"rb");
#endif

	if (pfSource==NULL)
	{
		//printf("FTP:errore in apertura %s",lpLocalFileSource);
		if (funcNotify) 
			_errorOut(&sWeb,NULL,0,TEXT("FTP:errore in apertura file sorgente locale")); 
			else
			win_infoarg(TEXT("FTP:errore in apertura file sorgente locale"));
		return iRet;
	}

	// -------------------------------------------------
	// APRO IL FILE
	// 
	if (funcNotify) funcNotify(&sWeb,WS_DO,0,TEXT("Inizio fase di scrittura ..."));

	if (!(hFtpFile=FtpOpenFile(hFtpSession,
							   lpNewRemoteFile,
						       GENERIC_WRITE,
						       FTP_TRANSFER_TYPE_BINARY,
						       0)))
							{
								if (lpiError) *lpiError=osGetError();

								if (funcNotify) 
									_errorOut(&sWeb,NULL,osGetError(), TEXT("FtpOpenFile")); 
									else
									win_infoarg(TEXT("FtpOpenFile: %d [%s:%s:%s]"),
												osGetError(),
												lpFtpServer,
												lpUserName,
												lpPassWord,
												ERROR_INTERNET_CANNOT_CONNECT);
								goto MYERRORE;
								//return TRUE;
							}

	if (funcNotify) 
	{
		funcNotify(&sWeb,WS_DO,0,TEXT("Trasferimento file ..."));
		funcNotify(&sWeb,WS_PROCESS,0,NULL); // Segnalo inizio trasferimento
	}
	lpBuf=ehAlloc(dwSize);
	while (TRUE)
	{
		DWORD dwRealSize;
		DWORD dwRemoteWrite;
		BOOL fCheck;

		dwRealSize=fread(lpBuf,1,dwSize,pfSource);
		if (dwRealSize==0) {fAllOK=TRUE; break;}
		fCheck=InternetWriteFile(hFtpFile,lpBuf,dwRealSize,&dwRemoteWrite);
		
		// ----------------------------------------
		// Se tutto è OK
		if (fCheck)
		{
			sWeb.dwContentLength+=dwRemoteWrite;
			if (funcNotify) funcNotify(&sWeb,WS_PROCESS,dwRealSize,NULL);
		}
		else
		// ----------------------------------------
		// Se qualcosa non và
		{
			if (lpiError) *lpiError=osGetError();
			if (funcNotify) _errorOut(&sWeb,NULL,osGetError(),TEXT("in WriteFile:"));
						 else
						 win_infoarg(TEXT("InternetWriteFile(): %d [%s]"),
										  osGetError(),
										  lpFtpServer,
										  ERROR_INTERNET_CANNOT_CONNECT);

			fAllOK=FALSE;
			break;
		}

		// ----------------------------------------
		// Controllo se mi devo fermare

		if (funcNotify)
			{
				if (funcNotify(&sWeb,WS_INF,0,NULL)) 
				{
					_errorOut(&sWeb,NULL,1,TEXT("User break."));
					fAllOK=FALSE;
					break;
				}
			}
	}

	//os_errset(POP);
	ehFree(lpBuf);

	if (funcNotify)
	{
		(*funcNotify)(&sWeb,WS_DO,0,TEXT("Chiusura trasferimento ..."));
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
			if (lpiError) *lpiError=osGetError();
			if (funcNotify) _errorOut(&sWeb,NULL,osGetError (), TEXT("CloseFileFtp"));
			fAllOK=FALSE;
//			goto MYERRORE;
		 }
		 hFtpFile=NULL;
	}
	
	// -------------------------------------------
	// Chiudo la Connessione con il server FTP
	// 
	//
	if (hFtpSession!=NULL)
	{
//		 iErr=FtpSetCurrentDirectory(hFtpSession,szCurrentDir);
		 if (!InternetCloseHandle (hFtpSession) )
		 {
			if (lpiError) *lpiError=osGetError();
			_errorOut(&sWeb,NULL,osGetError (),TEXT( "CloseHandle on hConnect"));
			fAllOK=FALSE;
	//		goto MYERRORE;
		 }
		 hFtpSession=NULL;
	}

	if (pfSource) fclose(pfSource);

	// ---------------------------------------------------
	// Trasmetto il responso della trasmissione
	//
	if (funcNotify) funcNotify(&sWeb,WS_LINK,fAllOK,NULL);
	if (funcNotify) funcNotify(&sWeb,WS_DO,0,TEXT("Fine sessione FTP."));

	if (fAllOK) iRet=FALSE;
	return iRet;
}




//
// Trasferisce un file utilizzando il protocollo FTP
//

EH_AR FtpDir(CHAR *	lpUserName,        // Indicazione dell'utente
			 CHAR *	lpPassWord,
	         CHAR *	lpFtpServer, // Server da Aprire
	         CHAR *	lpszSearchFile,  // Pagina da richiedere
	         void *	(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS),
			 INT *	lpiError)
{
	HINTERNET hFtpSession=NULL;
	HINTERNET hFtpFile=NULL;
	HINTERNET hFtpFind=NULL;
	FILE *pfSource=NULL;
	BOOL iRet=TRUE;
	BOOL fAllOK=FALSE;
	EH_AR arDir;
	WIN32_FIND_DATA sFindFileData;
	CHAR szServ[1024];

	EH_WEB sWeb;
	webPreset(&sWeb,funcNotify,NULL);

	WebStartup();
	arDir=ARNew();
	if (lpiError) *lpiError=0;

	// 
	// APRO LA CONNESSIONE CON IL SERVER FTP            
	// Se c'è un cambio Server effettuo la connessione 
	// 
	if (funcNotify) funcNotify(&sWeb,WS_DO,0,TEXT("Richiesta connessione FTP"));

	if (!(hFtpSession=InternetConnect(hOpen,		// Handle della apertura in internet
									  lpFtpServer,	// Nome del server
									  INTERNET_DEFAULT_FTP_PORT, // Porta di accesso 80=HTTP 21=FTP
									  lpUserName,	// User Name
									  lpPassWord,	// Password
									  INTERNET_SERVICE_FTP, // Servizio richiesto HTTP/FTP ecc...
									  INTERNET_FLAG_PASSIVE, //Flag ? modifica 2002 non era così
									  0))) // dwContext (Funzione ?)
							{
								if (lpiError) *lpiError=osGetError();
								if (funcNotify) 
									_errorOut(&sWeb,NULL,osGetError(),TEXT("InternetConnecterror:")); 
									else
									win_infoarg(TEXT("InternetConnecterror: %d [%s:%s:%s]"),
												osGetError(),
												lpFtpServer,
												lpUserName,
												lpPassWord,
												ERROR_INTERNET_CANNOT_CONNECT);
								return arDir;
							}


	//
	// Cerco il primo file
	//
	hFtpFind=FtpFindFirstFile(	hFtpSession,
								lpszSearchFile,
								&sFindFileData,
								INTERNET_FLAG_RELOAD,
								0);
	if (hFtpFind)
	{
		do 
		{
			BYTE *pszFileName=sFindFileData.cFileName;
			sprintf(szServ,"%d|%s|%d",sFindFileData.dwFileAttributes,sFindFileData.cFileName,sFindFileData.nFileSizeLow);
			ARAdd(&arDir,szServ);

		} while (InternetFindNextFile(hFtpFind,&sFindFileData));
		InternetCloseHandle(hFtpFind);
	}

	
	// 
	// Chiudo la Connessione con il server FTP
	//
	if (hFtpSession!=NULL)
	{
	 if (!InternetCloseHandle (hFtpSession) )
	 {
		if (lpiError) *lpiError=osGetError();
		_errorOut(&sWeb,NULL,osGetError (),TEXT( "CloseHandle on hConnect"));
		fAllOK=FALSE;
		return arDir;
	 }
	}

	if (pfSource) fclose(pfSource);

	// ---------------------------------------------------
	// Trasmetto il responso della trasmissione
	//
	if (funcNotify) 
	{
		funcNotify(&sWeb,WS_LINK,fAllOK,NULL);
		funcNotify(&sWeb,WS_DO,0,TEXT("Fine sessione FTP."));
	}

	if (fAllOK) iRet=FALSE;
	return arDir;
} 




// 
// FtpGet()
// Riceve un file utilizzando il protocollo FTP
//
// Ritorna 0 se è tutto ok
// 1 = Se non riesce ad aprire FTP
// 2 = Se non trova o non riesce ad aprire il file
//

BOOL FtpGet(CHAR *	lpUserName,        // Indicazione dell'utente 
			CHAR *	lpPassWord,
	        CHAR *	pszFtpServer, // Server da Aprire
			CHAR *	lpLocalFileDest,
	        CHAR *	lpRemoteFileSource,  // Pagina da richiedere
	        void *	(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS), // Funzione di controllo e display
	        BOOL	fFailIfExists, // TRUE/FALSE Visione dei passaggi di transazione
			INT *	lpiErrorFtp, 
			BOOL	bModoAttivo,  // TRUE se si vuole il modo attivo
			void *	pVoidParam)
{
	HINTERNET hFtpSession=NULL;
	FILE *pfSource=NULL;
	BOOL iRet=1;
	BOOL fAllOK=FALSE;
	INT iError=0;
	INT iPort;
	EH_AR ar=NULL;

	EH_WEB sWeb;
	webPreset(&sWeb,funcNotify,pVoidParam);

	WebStartup();
	if (lpiErrorFtp) *lpiErrorFtp=0;

	// 
	// APRO LA CONNESSIONE CON IL SERVER FTP            
	// Se c'è un cambio Server effettuo la connessione 
	// 
	if (funcNotify) funcNotify(&sWeb,WS_DO,0,TEXT("Richiesta connessione FTP"));
		
	ar=strSplit(pszFtpServer,":");
	iPort=INTERNET_DEFAULT_FTP_PORT;
	if (ARLen(ar)>1) {iPort=atoi(ar[1]); pszFtpServer=ar[0];}


	if (!(hFtpSession=InternetConnect(hOpen, // Handle della apertura in internet
									  pszFtpServer, // Nome del server
									  iPort, // Porta di accesso 80=HTTP 21=FTP
									  lpUserName, // User Name
									  lpPassWord, // Password
									  INTERNET_SERVICE_FTP, // Servizio richiesto HTTP/FTP ecc...
									  bModoAttivo?0:INTERNET_FLAG_PASSIVE, //Flag ? modifica 2002 non era così
									  0))) // dwContext (Funzione ?)
									{
										iError=osGetError();
										if (funcNotify) 
											_errorOut(&sWeb,NULL,osGetError(),TEXT("InternetConnecterror:")); 
											else
											win_infoarg(TEXT("InternetConnecterror: %d [%s:%s:%s]"),
														osGetError(),
														pszFtpServer,
														lpUserName,
														lpPassWord,
														ERROR_INTERNET_CANNOT_CONNECT);
										goto MYERRORE;
									}

	// -------------------------------------------------
	// APRO IL FILE
	// 
	if (funcNotify) funcNotify(&sWeb,WS_DO,0,TEXT("Inizio fase di lettura ..."));

	//
	// Solo controllo esistenza del file
	//
	if (!lpLocalFileDest)
	{
		HINTERNET hOpen;
		hOpen=FtpOpenFile(hFtpSession,
						lpRemoteFileSource,
						GENERIC_READ,
						FTP_TRANSFER_TYPE_BINARY,//bModoAttivo?FTP_TRANSFER_TYPE_ASCII:FTP_TRANSFER_TYPE_BINARY,
						0);
		if (!hOpen)
		{
			fAllOK=FALSE;
			goto MYERRORE;

		} 
		else 
		{
			fAllOK=TRUE;
			InternetCloseHandle(hOpen);
			goto MYERRORE;
		}

	}


	if (!FtpGetFile(hFtpSession,
					lpRemoteFileSource,
					lpLocalFileDest,
					fFailIfExists,
					0,
					FTP_TRANSFER_TYPE_BINARY,//bModoAttivo?FTP_TRANSFER_TYPE_ASCII:FTP_TRANSFER_TYPE_BINARY,
					0))
	{

		iError=osGetError();
		if (funcNotify) 
		{
			funcNotify(&sWeb,WS_DO,0,TEXT("Errore durante la ricezione ..."));
		}
		ehLogWrite("Errore %d durante la lettura",osGetError());
#ifdef _DEBUG
		printf("Errore %d durante trasferimento",osGetError());
#endif
		iRet=2;
		goto MYERRORE;
	}

	if (funcNotify) {funcNotify(&sWeb,WS_DO,0,TEXT("Chiusura trasferimento ..."));}
	iRet=FALSE; fAllOK=TRUE;

MYERRORE:
	ehFree(ar);

	if (lpiErrorFtp) *lpiErrorFtp=iError;

	// -------------------------------------------
	// Chiudo la Connessione con il server FTP
	// 
	//
	if (hFtpSession!=NULL)
	{
	 if (!InternetCloseHandle (hFtpSession) )
	 {
		_errorOut(&sWeb,NULL,osGetError(),TEXT( "CloseHandle on hConnect"));
		fAllOK=FALSE;
		goto MYERRORE;
	 }
	}

	// ---------------------------------------------------
	// Trasmetto il responso della trasmissione
	//
	if (funcNotify) 
	{
		funcNotify(&sWeb,WS_LINK,fAllOK,NULL);
		
		if (fAllOK)
		{
			funcNotify(&sWeb,WS_DO,0,TEXT("Fine sessione FTP."));
		}
		else
		{
			CHAR *p=osErrorStrAlloc(iError);
			funcNotify(&sWeb,WS_DO,0,p);
			ehFree(p);
		}
	}

	if (fAllOK) iRet=FALSE;
	return iRet;
}



// 
// FtpDelete()
// Cancella un file utilizzando il protocollo FTP
//
BOOL FtpDelete(	CHAR *lpUserName,        // Indicazione dell'utente
				CHAR *lpPassWord,
				CHAR *lpFtpServer, // Server da Aprire
				CHAR *lpFileDelete,
				void *	(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS), // Funzione di controllo e display
				INT *lpiError,    // TRUE/FALSE Visione dei passaggi di transazione
				BOOL bModoAttivo)  // TRUE se si vuole il modo attivo 
{
	HINTERNET hFtpSession=NULL;
	FILE *pfSource=NULL;
	BOOL iRet=TRUE;
	BOOL fAllOK=FALSE;

	EH_WEB sWeb;
	webPreset(&sWeb,funcNotify,NULL);

	WebStartup();
	if (lpiError) *lpiError=0;

	// 
	// APRO LA CONNESSIONE CON IL SERVER FTP            
	// Se c'è un cambio Server effettuo la connessione 
	// 
	if (funcNotify) funcNotify(&sWeb,WS_DO,0,TEXT("Richiesta connessione FTP"));

	if (!(hFtpSession=InternetConnect(hOpen, // Handle della apertura in internet
									  lpFtpServer, // Nome del server
									  INTERNET_DEFAULT_FTP_PORT, // Porta di accesso 80=HTTP 21=FTP
									  lpUserName, // User Name
									  lpPassWord, // Password
									  INTERNET_SERVICE_FTP, // Servizio richiesto HTTP/FTP ecc...
									  bModoAttivo?0:INTERNET_FLAG_PASSIVE, //Flag ? modifica 2002 non era così
									  0))) // dwContext (Funzione ?)
							{
								if (lpiError) *lpiError=osGetError();
								if (funcNotify) 
									_errorOut(&sWeb,NULL,osGetError(),TEXT("InternetConnecterror:")); 
									else
									win_infoarg(TEXT("InternetConnecterror: %d [%s:%s:%s]"),
												osGetError(),
												lpFtpServer,
												lpUserName,
												lpPassWord,
												ERROR_INTERNET_CANNOT_CONNECT);
								goto MYERRORE;
							}

	// -------------------------------------------------
	// APRO IL FILE
	// 
	if (!FtpDeleteFile(hFtpSession,lpFileDelete))
	{
		if (lpiError) *lpiError=osGetError();
		if (funcNotify) 
		{
			funcNotify(&sWeb,WS_DO,0,TEXT("Errore durante la cancellazione ..."));
		}
		ehLogWrite("Errore %d durante la cancellazione",osGetError());
#ifdef _DEBUG
		printf("Errore %d durante la cancellazione",osGetError());
#endif
		goto MYERRORE;
	}

	if (funcNotify) {funcNotify(&sWeb,WS_DO,0,TEXT("Chiusura cancellazione ..."));}
	iRet=FALSE;

MYERRORE:

	// -------------------------------------------
	// Chiudo la Connessione con il server FTP
	// 
	//
	if (hFtpSession!=NULL)
	{
	 if (!InternetCloseHandle (hFtpSession) )
	 {
		_errorOut(&sWeb,NULL,osGetError (),TEXT( "CloseHandle on hConnect"));
		fAllOK=FALSE;
		goto MYERRORE;
	 }
	}

	// ---------------------------------------------------
	// Trasmetto il responso della trasmissione
	//
	if (funcNotify) 
	{
		funcNotify(&sWeb,WS_LINK,fAllOK,NULL);
		funcNotify(&sWeb,WS_DO,0,TEXT("Fine sessione FTP."));
	}

	if (fAllOK) iRet=FALSE;
	return iRet;
}

// ---------------------------------------------
// WebCheckConnect
// ritorna TRUE se si è connessi ad internet
// lp=se si passa un puntatore ad una stringa
//    nella stringa viene scritto il modo in cui si è connessi ad internet
//

BOOL WebCheckConnect(CHAR *lp)
{
  DWORD dwFlags;//,dwIn;
  BOOL bRet;

  ZeroFill(dwFlags);

//  dwIn=INTERNET_AUTODIAL_FORCE_ONLINE;
  //InternetAutodial(dwIn,0);
  WebStartup();

  bRet=InternetGetConnectedState(&dwFlags,0);
  if (lp)
  {
	strcpy(lp,TEXT("?"));
	switch (dwFlags&0xF)
	{
		case INTERNET_CONNECTION_MODEM: strcpy(lp,TEXT("Modem")); break;
		case INTERNET_CONNECTION_LAN: strcpy(lp,TEXT("Lan")); break;
		case INTERNET_CONNECTION_PROXY: strcpy(lp,TEXT("Proxy")); break;
		case INTERNET_CONNECTION_MODEM_BUSY: strcpy(lp,TEXT("Busy")); break;
	}
  }
  return bRet;
}

static struct hostent *hServer;    // Host SMTP
static struct sockaddr_in aServer; // Socket per la comunicazione in SMTP
//INT iServerSocket;

// 0=FTP 1=WEB (porta)

//static BOOL fSocketStart=FALSE;
//static INT iSocketCount=0;

void FWebSocketOpen(void)
{
	/*
	if (!iSocketCount) 
	{
		if (WSAStartup(0x101, &wsaData)) ehExit("WSAStartup()");
		//fSocketStart=TRUE;
	}

	iSocketCount++;
	*/
}

void FWebSocketClose(INT iServerSocket)
{
	if (iServerSocket) {closesocket(iServerSocket);} 
	iServerSocket=0;
}

// 0= Tutto ok
// <0 Un errore di collegamento
//
BOOL FInternetCheck(CHAR *lpAddress,unsigned short usiPort,CHAR *lpBuffer,INT iSizeBuffer)
{
	INT iByte;
	INT iServerSocket=0;

	WebStartup();

	FWebSocketOpen();

	ZeroFill(hServer);
	ZeroFill(aServer);
	hServer=gethostbyname(lpAddress);	  // WinSock - Ritorna una struttura hostest
	if (hServer==NULL) {FWebSocketClose(iServerSocket); return -1;} // L'host SMTP non esiste

	// -----------------------------------------------------
	// Richiedo socket per porta iPort (SMTP)
	//
	aServer.sin_family = AF_INET;
	aServer.sin_port = htons(usiPort); 
	memcpy(&(aServer.sin_addr.s_addr), hServer->h_addr, sizeof(hServer->h_addr));
	iServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (iServerSocket==0) {FWebSocketClose(iServerSocket); return -2;} // Non mi posso creare un socket con l'host SMTP

	//_d_("%d [%s]",iServerSocket,lpAddress);

	// -----------------------------------------------------
	// Richiedo connessione alla porta tramite socket
	//
	if (connect(iServerSocket, (struct sockaddr *)&aServer, sizeof(aServer)))
	{
		FWebSocketClose(iServerSocket); return -3;
	}

	if (lpBuffer)
	{
		iByte=recv(iServerSocket,lpBuffer,iSizeBuffer,0);
		if (iByte>=0) lpBuffer[iByte]=0;
	}

	FWebSocketClose(iServerSocket);
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

	INT a,b;
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
  CHAR szBuffer[100];
  HANDLE hRasConn;
  BOOL fCheck;
  DWORD dwCb,dwDevice;
  RASDEVINFOW *lpRasDevice=NULL;
  INT a;

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
  CHAR szBuffer[100];
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

/*
void WebFree(FWEBSERVER *psWS)
{
	// Libero le risorse
	//if (psWS->iSocket) {closesocket(psWS->iSocket); psWS->iSocket=0;}
	//if (psWS->lpBufRice) {ehFree(psWS->lpBufRice); psWS->lpBufRice=NULL;}
	//if (psWS->lpBufSend) {ehFree(psWS->lpBufSend); psWS->lpBufSend=NULL;}

	//WSACleanup();
	//fSocketStart=FALSE;
	FWebSocketClose(psWS->iSocket);
}
*/

//
// eh_tcp_send()
// Send multi protocollo
//

static BOOL eh_tcp_send(FWEBSERVER *psWS,BYTE *pBuf,INT iSize)
{
	INT nwritten,iError;

	while (iSize)
	{
	//
	// Ho un timeout > lo setto  (DA FARE)
	//
/* 
      if (psWS->send_timeout)
      { struct timeval timeout;
        fd_set fd;
        if (psWS->send_timeout > 0)
        { timeout.tv_sec = psWS->send_timeout;
          timeout.tv_usec = 0;
        }
        else
        { timeout.tv_sec = -psWS->send_timeout/1000000;
          timeout.tv_usec = -psWS->send_timeout%1000000;
        }
        FD_ZERO(&fd);
        FD_SET(psWS->iSocket, &fd);
        while(TRUE)
        { register int r = select((psWS->iSocket + 1), NULL, &fd, &fd, &timeout);
          if (r > 0)
            break;
          if (!r)
          { psWS->iSocketError = 0;
            return WT_EOF;
          }
          if (wt_socket_errno != WT_EINTR && wt_socket_errno != WT_EAGAIN)
          { psWS->iSocketError = wt_socket_errno;
            return WT_EOF;
          }
        }
      }
*/
#ifdef WITH_OPENSSL
    if (psWS->ssl)
       nwritten = SSL_write(psWS->ssl, pBuf, iSize);
    else if (psWS->bio)
       nwritten = BIO_write(psWS->bio, pBuf, iSize);
    else
#endif
	nwritten = send(psWS->iSocket, pBuf, iSize, 0);//psWS->iSocket_flags);
//	iErr=send(->iSocket,lpBuf,iSize,0);

      if (nwritten <= 0)
      {
#ifdef WITH_OPENSSL
		int err = SSL_get_error(psWS->ssl, nwritten);
        if (psWS->ssl && err != SSL_ERROR_NONE && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
          return TRUE;
#endif
		iError=WSAGetLastError();
        if (iError != WSAEINTR && iError != WSAEWOULDBLOCK)
		{	psWS->iErrorCode=WTE_SSL_SEND_ERROR;
			psWS->iSocketErr= iError;
			ehLogWrite("Errore: %d [%s:%d]",iError,pBuf,iSize);
			return TRUE;
        }
        nwritten = 0; /* and call write() again */
      }
	  pBuf+=nwritten;
	  iSize-=nwritten;
	}
	return FALSE;
	  
/*
	if (nwritten==SOCKET_ERROR)
	{
		INT iNumError;
		iNumError=WSAGetLastError();
		win_infoarg("Errore: %d [%s:%d]",iNumError,lpBuf,iSize);
	}
	if (iErr==SOCKET_ERROR) return TRUE; else return FALSE;// Errore
	*/
}

#ifdef WITH_OPENSSL
static CHAR *ssl_ErrorToString(INT iErr)
{
	switch (iErr)
	{
		case SSL_ERROR_NONE			: return "Ok";
		case SSL_ERROR_SSL			: return "SSL_ERROR_SSL";
		case SSL_ERROR_WANT_READ	: return "SSL_ERROR_WANT_READ";
		case SSL_ERROR_WANT_WRITE	: return "SSL_ERROR_WANT_WRITE";
		case SSL_ERROR_WANT_X509_LOOKUP	: return "SSL_ERROR_WANT_X509_LOOKUP";
		case SSL_ERROR_SYSCALL		: return "SSL_ERROR_SYSCALL";
		case SSL_ERROR_ZERO_RETURN	: return "SSL_ERROR_ZERO_RETURN";
		case SSL_ERROR_WANT_CONNECT	: return "SSL_ERROR_WANT_CONNECT";
		case SSL_ERROR_WANT_ACCEPT	: return "SSL_ERROR_WANT_ACCEPT";
		default: return "?";
	}
}
#endif

//
// eh_tcp_recv()
//
static size_t eh_tcp_recv(FWEBSERVER *psWS, char *s, size_t n)
{ 
	register int r;
	int iErr;
	psWS->iSocketErr = 0;

	if (!psWS->iTimeoutSec)//recv_timeout) 
	{
		if (!psWS->bHttps) psWS->iTimeoutSec=10; // Proviamo 11/2009 <-########################
	}

	if (wt_valid_socket(psWS->iSocket))
	{
		while(TRUE)
	    { 

#ifndef WITH_LEAN

			//
			// Gestione del Timeout
			//
#ifdef WITH_OPENSSL
			if (psWS->iTimeoutSec&&!psWS->ssl)
#else
			if (psWS->iTimeoutSec)
#endif
			{
				/*
				struct timeval timeout;
				fd_set fd;

				if (psWS->recv_timeout > 0)
				{ 
					timeout.tv_sec = 0;
					timeout.tv_usec = psWS->recv_timeout*1000;
				}
				else
				{ 
					timeout.tv_sec = -psWS->recv_timeout/1000000;
					timeout.tv_usec = -psWS->recv_timeout%1000000;
				}

				FD_ZERO(&fd);
				FD_SET(psWS->iSocket, &fd);
				while(TRUE)
				{ 
					r = select((psWS->iSocket + 1), &fd, NULL, &fd, &timeout);
					if (r > 0) break;
					if (!r)
					{ 
						psWS->iSocketErr = 0;
						return 0;
					}

					if (wt_socket_errno != WSAEINTR && wt_socket_errno != WSAEWOULDBLOCK)
					{  
						psWS->iErrorCode=WTE_SOCKET_SELECT;
						psWS->iSocketErr = wt_socket_errno;
						return 0;
					}
				}
				*/
				struct timeval timeout;
				fd_set fd;
				timeout.tv_sec = psWS->iTimeoutSec;
				timeout.tv_usec = 0;// 10000;

				FD_ZERO(&fd);
				FD_SET(psWS->iSocket, &fd);
				r = select(sizeof(fd), &fd, NULL, NULL, &timeout); // (psWS->iSocket + 1)
				if (!r) { 
					psWS->iErrorCode=WTE_TIMEOUT;
					psWS->iSocketErr = 0; 
					return 0;
				}
				if (r < 0 && wt_socket_errno != WSAEINTR) 
					{	
						psWS->iErrorCode=WTE_TIMEOUT;//WTE_SOCKET_SELECT; 
						psWS->iSocketErr=wt_socket_errno; 
						return 0; 
					}
			  }

#endif

#ifdef WITH_OPENSSL
		  if (psWS->ssl)
		  {  
				int errcode;
				r = SSL_read(psWS->ssl, s, n);  // DA RIMETTERE n se no si blocca
				//if (!r) return 0;
				if (r >= 0) return (size_t) r;

				errcode = SSL_get_error(psWS->ssl, r);

				// if (errcode==SSL_ERROR_ZERO_RETURN||SSL_ERROR_SYSCALL) return 0;

				if (errcode != SSL_ERROR_NONE && 
					errcode != SSL_ERROR_WANT_READ && 
					errcode != SSL_ERROR_WANT_WRITE) 
				{
					int sslerr = ERR_get_error();
					CHAR *eptr=NULL;
					CHAR errbuf[2000],*errptr;
					
					psWS->iErrorCode = WTE_SSL_RECV;

					if (errno) eptr = strerror(errno);
					if (sslerr)
					{ 
						ERR_error_string(sslerr, errbuf); 
						errptr = errbuf;
					}
					if (sslerr)  /*  SSL IO error?  */
					{ 
					  if (errptr && *errptr)
						  ehLogWrite("errcode %d: %s , %s , %d", errcode, errptr, __FILE__, __LINE__);
						else
						ehLogWrite("errcode %d: %s , %s , %d" , errcode, "SSL_ERROR_SYSCALL" , __FILE__, __LINE__);
					}
					else if (eptr && *eptr) /*  Some system error - check errno */
					  ehLogWrite("errcode %d: %s , %s , %d", errcode, eptr, __FILE__, __LINE__);
					else if (r == 0)
					  ehLogWrite("errcode %d: %s , %s , %d", errcode, "SSL_ERROR_SYSCALL/EOF" , __FILE__, __LINE__);  // XXXXXXX
					else
					  ehLogWrite("errcode %d: %s , %s , %d", errcode, "SSL_ERROR_SYSCALL/SOCKET" , __FILE__, __LINE__);

							//break;
					//	}
				//	}
					return -1;
				}
		  }
		  else if (psWS->bio)
		  { r = BIO_read(psWS->bio, s, n);
			if (r > 0) return (size_t)r;
			return 0;
		  }
		  else
#endif

#ifdef WITH_UDP
        if ((psWS->omode & SOAP_IO_UDP))
        { SOAP_SOCKLEN_T k = (SOAP_SOCKLEN_T)sizeof(psWS->peer);
	  memset((void*)&psWS->peer, 0, sizeof(psWS->peer));
          r = recvfrom(psWS->iSocket, s, n, psWS->iSocket_flags, (struct sockaddr*)&psWS->peer, &k);	/* portability note: see SOAP_SOCKLEN_T definition in stdsoap2.h */
	  psWS->peerlen = (size_t)k;
#ifndef WITH_IPV6
          psWS->ip = ntohl(psWS->peer.sin_addr.s_addr);
          psWS->port = (int)ntohs(psWS->peer.sin_port);
#endif
        }
	else
#endif
		{
			// Lettura classica da socket
			r = recv( psWS->iSocket, s, n, 0);//psWS->iSocket_flags);
			iErr=wt_socket_errno;
			if (r >= 0) return (size_t) r;
			if (iErr != WSAEINTR && iErr != WSAEWOULDBLOCK && iErr != WSAEWOULDBLOCK) 
			{
				psWS->iErrorCode = WTE_RECV;
				psWS->iSocketErr = wt_socket_errno; 
				return 0;
			}
//			printf("qui");
		}
/*
      { 
		struct timeval timeout;
		fd_set fd;
		timeout.tv_sec = psWS->recv_timeout;
		timeout.tv_usec = 10000;

		FD_ZERO(&fd);
		FD_SET(psWS->iSocket, &fd);

#ifdef WITH_OPENSSL
        if (psWS->ssl && SSL_get_error(psWS->ssl, r) == SSL_ERROR_WANT_WRITE)
			r = select((psWS->iSocket + 1), NULL, &fd, &fd, &timeout);
			else
			r = select((psWS->iSocket + 1), &fd, NULL, &fd, &timeout);
#else
		r = select((psWS->iSocket + 1), &fd, NULL, &fd, &timeout);
#endif
		if (r < 0 && wt_socket_errno != WSAEINTR) {psWS->iErrorCode=WTE_SOCKET_SELECT; psWS->iSocketErr=wt_socket_errno; return 0; }
      }
	  */

	}
  }
  return 0;
/*
#ifdef WITH_FASTCGI
  return fread(s, 1, n, stdin);
#else

	#ifdef UNDER_CE
	  return fread(s, 1, n, psWS->recvfd);
	#else
	#ifdef WMW_RPM_IO
	  if (psWS->rpmreqid)
		r = httpBlockRead(psWS->rpmreqid, s, n);
	#endif
  r = _read(psWS->recvfd, s, n);
  if (r >= 0) return (size_t)r;
  psWS->iSocketError = soap_errno;
  return 0;
#endif
#endif
*/
}

/*


#include <windows.h>
#include <winsock2.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int __cdecl main() {

    //----------------------
    // Declare and initialize variables.
    WSADATA wsaData;
    int iResult;

    SOCKET ConnectSocket;
    struct sockaddr_in clientService; 

    char *sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
  
    //----------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != NO_ERROR) {
      printf("WSAStartup failed: %d\n", iResult);
      return 1;
    }

    //----------------------
    // Create a SOCKET for connecting to server
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError() );
        WSACleanup();
        return 1;
    }

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    clientService.sin_port = htons( 27015 );

    //----------------------
    // Connect to server.
    iResult = connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) );
    if ( iResult == SOCKET_ERROR) {
        closesocket (ConnectSocket);
        printf("Unable to connect to server: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
    if (iResult == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
            printf("Bytes received: %d\n", iResult);
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
*/


static void WSBufRealloc(FWEBSERVER *psWS,DWORD dwNewSize)
{
	BYTE *lp;
//	win_infoarg("%d,%d",psWS->iSizeBuf,iNewSize);
	psWS->dwDataSize=dwNewSize; // Calcolo le nuove dimensioni del buffer
	lp=ehAllocZero(dwNewSize); if (!lp) ehExit("No memory");
	if (psWS->iDataReaded>=(INT) dwNewSize) ehError();
	// Copio i dati fino adesso ricevuti nel nuovo buffer
	if (psWS->iDataReaded>0) memcpy(lp,psWS->pData,psWS->iDataReaded);
	if (psWS->pData) ehFree(psWS->pData);
	psWS->pData=lp;
}
/*
static BYTE *LGetContentPoint(BYTE *lpString,BOOL fChunked,INT *iOffset)
{
	BYTE *lpStart=strstr(lpString,CRLF CRLF);  
	if (lpStart) 
	{
		BYTE *lp;
		lpStart+=4;
		if (fChunked) 
		{
			lp=strstr(lpStart,CRLF); 
			if (lp) lpStart=lp+2;
		}
	}
	
	if (!lpStart) 
	{
		lpStart=strstr(lpString,"\n\n");  
		if (lpStart) 
		{
			lpStart+=2;
		}	
	}

	if (!*iOffset) *iOffset=(INT) lpStart-(INT) lpString;
	return lpStart;
}
*/

int eh_tag_cmp(const char *s, const char *t)
{ while(TRUE)
  { register int c1 = *s;
    register int c2 = *t;
    if (!c1 || c1 == '"')
      break;
    if (c2 != '-')
    { if (c1 != c2)
      { if (c1 >= 'A' && c1 <= 'Z')
          c1 += 'a' - 'A';
        if (c2 >= 'A' && c2 <= 'Z')
          c2 += 'a' - 'A';
      }
      if (c1 != c2)
      { if (c2 != '*')
          return 1;
	c2 = *++t;
        if (!c2)
	  return 0;
        if (c2 >= 'A' && c2 <= 'Z')
          c2 += 'a' - 'A';
        while(TRUE)
        { c1 = *s;
	  if (!c1 || c1 == '"')
	    break;
	  if (c1 >= 'A' && c1 <= 'Z')
            c1 += 'a' - 'A';
          if (c1 == c2 && !eh_tag_cmp(s + 1, t + 1))
            return 0;
	  s++;
        }
        break;
      }
    }
    s++;
    t++;
  }
  if (*t == '*' && !t[1])
    return 0;
  return *t;
}
/******************************************************************************/
static const char * wt_tcp_error(FWEBSERVER *psWS)
{ 
  register const char *msg = NULL;
  /*
  switch (psWS->errmode)
  { case 0:
      msg = soap_strerror(soap);
      break;
    case 1:
      msg = "WSAStartup failed";
      break;
    case 2:
    {
#ifndef WITH_LEAN
      msg = soap_str_code(h_error_codes, soap->errnum);
      if (!msg)
#endif
      { sprintf(soap->msgbuf, "TCP/UDP IP error %d", soap->errnum);
        msg = soap->msgbuf;
      }
    }
  }
  */
  msg="?";
  return msg;
}

//#define WS_BUFLEN (65536)

//
// eh_tcp_connect()
// Crea una connessione con il server (multipiattaforma e multi protocollo)
// Ritorna il socket o errore
//
static int eh_tcp_connect(FWEBSERVER *psWS, const char *endpoint, const char *pHostName, int port)
{
#ifdef WITH_IPV6
	struct addrinfo hints, *res, *ressave;
	int err;
#endif
	register int fd;
	int len = WT_BUFFERSIZE;
	int set = 1;

	if (wt_valid_socket(psWS->iSocket)) closesocket(psWS->iSocket);
	psWS->iSocket = WT_INVALID_SOCKET;

//  psWS->ierrmode = 0;
#ifdef WITH_IPV6 
	memset((void*)&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
 #ifdef WITH_UDP
	if ((psWS->omode & WT_IO_UDP))
	hints.ai_socktype = SOCK_DGRAM;
	else
 #endif
	hints.ai_socktype = SOCK_STREAM;
	psWS->errmode = 2;
	if (psWS->proxy_host)
	err = getaddrinfo(psWS->proxy_host, soap_int2s(soap, psWS->proxy_port), &hints, &res);
	else
	err = getaddrinfo(host, soap_int2s(soap, port), &hints, &res);
	if (err)
	{ soap_set_sender_error(soap, gai_strerror(err), "getaddrinfo failed in tcp_connect()", WT_TCP_ERROR);
	return WT_INVALID_SOCKET;
	}
	ressave = res;
	again:
	fd = (int)socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	psWS->errmode = 0;
#else
#ifdef WITH_UDP
  if ((psWS->omode & WT_IO_UDP))
    fd = (int) socket(AF_INET, SOCK_DGRAM, 0);
  else
#endif
    fd = (int) socket(AF_INET, SOCK_STREAM, 0);//IPPROTO_TCP);
#endif
	
	if (fd < 0)
	{ 
		psWS->iErrorCode=WTE_SOCKET_CREATE;
		psWS->iSocketErr = wt_socket_errno;
		ehLogWrite("socket failed in tcp_connect()"); // eh_tcp_error(pWS)
		return WT_INVALID_SOCKET;
	}

  //
  // Settaggio FLAG (da vedere)
  //
  /*
  if (psWS->connect_flags & SO_LINGER)
  { struct linger linger;
    memset((void*)&linger, 0, sizeof(linger));
    linger.l_onoff = 1;
    linger.l_linger = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(struct linger)))
    { 
		psWS->iErrorCode=WTE_SETSOCKOPT;
		psWS->iSocketErr = wt_socket_errno;
      //soap_set_sender_error(soap, wt_tcp_error(psWS), "setsockopt SO_LINGER failed in tcp_connect()", WT_TCP_ERROR);
	  ehLogWrite("setsockopt SO_LINGER failed in tcp_connect() %s", wt_tcp_error(psWS));
      closesocket(fd);
      return WT_INVALID_SOCKET;
    }
  }

  if ((psWS->connect_flags & ~SO_LINGER) && setsockopt(fd, SOL_SOCKET, psWS->connect_flags & ~SO_LINGER, (char*)&set, sizeof(int)))
  {	  psWS->iErrorCode=WTE_SETSOCKOPT;
	  psWS->iSocketErr = wt_socket_errno;
    ehLogWrite("setsockopt failed in tcp_connect() %s", wt_tcp_error(psWS));
    closesocket(fd);
    return WT_INVALID_SOCKET;
  }
  if (psWS->keep_alive && setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&set, sizeof(int)))
  {   psWS->iErrorCode=WTE_SETSOCKOPT;
	  psWS->iSocketErr = wt_socket_errno;
	ehLogWrite("Setsockopt SO_KEEPALIVE failed in tcp_connect(): %s", wt_tcp_error(psWS));
    closesocket(fd);
    return WT_INVALID_SOCKET;
  }

  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*) &len, sizeof(int)))
  { 
	  psWS->iErrorCode = WTE_SETSOCKOPT;
	  psWS->iSocketErr = wt_socket_errno;
	ehLogWrite("Setsockopt SO_SNDBUF failed in tcp_connect() %s", wt_tcp_error(psWS));
    closesocket(fd);
    return WT_INVALID_SOCKET;
  }
  if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*) &len, sizeof(int)))
  { psWS->iErrorCode=WTE_SETSOCKOPT;
    psWS->iSocketErr = wt_socket_errno;
	ehLogWrite("Setsockopt SO_RCVBUF failed in tcp_connect() %s", wt_tcp_error(psWS));
    closesocket(fd);
    return WT_INVALID_SOCKET;
  }

#ifdef TCP_NODELAY
  if (!(psWS->omode & WT_IO_UDP) && setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&set, sizeof(int)))
  { psWS->iErrorCode=WTE_SETSOCKOPT;
    psWS->iSocketErr = wt_socket_errno;
	ehLogWrite("Setsockopt TCP_NODELAY failed in tcp_connect() %s", wt_tcp_error(psWS));
    closesocket(fd);
    return WT_INVALID_SOCKET;
  }
#endif

*/

#ifndef WITH_IPV6

	psWS->iPeerLen = sizeof(psWS->sPeer);
	memset((void*)&psWS->sPeer, 0, sizeof(psWS->sPeer));
	psWS->sPeer.sin_family = AF_INET;

	//psWS->errmode = 2;
	if (psWS->proxy_host)
	{ 
		if (eh_tcp_gethost(psWS, psWS->proxy_host, &psWS->sPeer.sin_addr))
		{ 
			psWS->iErrorCode=WTE_GET_HOST;
			ehLogWrite("Get proxy host by name failed in tcp_connect()");
			closesocket(fd);
			return WT_INVALID_SOCKET;
		}
		psWS->sPeer.sin_port = htons((short) psWS->proxy_port);
	}
	else
	{ 
		if (eh_tcp_gethost(psWS, pHostName, &psWS->sPeer.sin_addr))
		{ 
			ehLogWrite("Get host by name failed in tcp_connect()");
			psWS->iErrorCode=WTE_GET_HOST;
			closesocket(fd);
			return WT_INVALID_SOCKET;
		}
		psWS->sPeer.sin_port = htons((short)port);
	}

	//psWS->errmode = 0;
	if ((psWS->omode & WT_IO_UDP)) return fd;
#endif


#ifndef WITH_LEAN
	if (psWS->iTimeoutSec)

	#if defined(WIN32)
	  { 
			u_long nonblocking = 1;
			ioctlsocket(fd, FIONBIO, &nonblocking);
	  }
	#elif defined(VXWORKS)
	  { 
			vx_nonblocking = TRUE;
			ioctl(fd, FIONBIO, (int)(&vx_nonblocking)); 
	  }
	#else
			fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
	#endif

  else

	#if defined(WIN32)
		{ 
			u_long blocking = 0;
			ioctlsocket(fd, FIONBIO, &blocking);
		}

	#elif defined(VXWORKS)

	  { 
		  vx_nonblocking = FALSE;
	    
		  ioctl(fd, FIONBIO, (int)(&vx_nonblocking)); 
	  }

	#else

		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);

	#endif

#endif

	while(TRUE)
	{ 
#ifdef WITH_IPV6
    if (connect(fd, res->ai_addr, res->ai_addrlen))
#else
    if (connect(fd, (SOCKADDR*) &psWS->sPeer, sizeof(psWS->sPeer))) // (struct sockaddr*)
#endif
    { 
#ifndef WITH_LEAN
      if (psWS->iTimeoutSec && (wt_socket_errno == WSAEINPROGRESS || wt_socket_errno == WSAEWOULDBLOCK))
      { struct timeval timeout;
        WT_SOCKLEN_T k;
        fd_set fds;
        if (psWS->iTimeoutSec > 0)
        { timeout.tv_sec = psWS->iTimeoutSec;
          timeout.tv_usec = 0;
        }
        else
        { timeout.tv_sec = -psWS->iTimeoutSec/1000000;
          timeout.tv_usec = -psWS->iTimeoutSec%1000000;
        }
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        while(TRUE)
        { 
			int r = select((fd + 1), NULL, &fds, NULL, &timeout);
            if (r > 0) break;

          if (!r)
          { 
			  psWS->iErrorCode=WTE_TIMEOUT_CONNECT;
			  psWS->iSocketErr = 0;
			  //DBGLOG(TEST, WT_MESSAGE(fdebug, "Connect timeout\n"));
              ehLogWrite("Timeout (%ds)" CRLF "connect failed in tcp_connect()",psWS->iTimeoutSec);
              closesocket(fd);
              return WT_INVALID_SOCKET;
          }

          if (wt_socket_errno != WSAEINTR)
          { 
			  psWS->iErrorCode=WTE_SOCKET_SELECT_2;
			  psWS->iSocketErr = wt_socket_errno;
            //DBGLOG(TEST, WT_MESSAGE(fdebug, "Could not connect to host\n"));
            ehLogWrite("connect failed in tcp_connect()");
            closesocket(fd);
            return WT_INVALID_SOCKET;
          }
        }
		k = (WT_SOCKLEN_T) sizeof(psWS->iSocketErr);
	    if (!getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&psWS->iSocketErr, &k) && !psWS->iSocketErr) break;
		psWS->iErrorCode=WTE_SOCKET_SELECT_3;
        psWS->iSocketErr = wt_socket_errno;
        ehLogWrite("Connect failed in tcp_connect()");
        closesocket(fd);
        return WT_INVALID_SOCKET;
      }
      else

#endif
#ifdef WITH_IPV6
      if (res->ai_next)
      { res = res->ai_next;
        closesocket(fd);
        goto again;
      }
      else
#endif
      if (wt_socket_errno != WSAEINTR)
      { 
		  psWS->iErrorCode=WTE_SOCKET_CONNECT;
		  psWS->iSocketErr = wt_socket_errno;
        ehLogWrite("connect failed in tcp_connect()");
        closesocket(fd);
        return WT_INVALID_SOCKET;
      }
    }  
    else
      break;
  }
#ifdef WITH_IPV6
  psWS->iPeerLen = 0; /* IPv6: already connected so use send() */
  freeaddrinfo(ressave);
#endif

#ifndef WITH_LEAN
  if (psWS->iTimeoutSec)
#if defined(WIN32)
  { u_long blocking = 0;
    ioctlsocket(fd, FIONBIO, &blocking);
  }
#elif defined(VXWORKS)
  { vx_nonblocking = FALSE;
    ioctl(fd, FIONBIO, (int)(&vx_nonblocking)); /* modified to use fd */
  }
#else
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);
#endif
#endif
	psWS->iSocket = fd;
	psWS->imode &= ~WT_ENC_SSL;
	psWS->omode &= ~WT_ENC_SSL;

  //
  // HTTPS = SSL
  //
  if (!strncmp(endpoint, "https:", 6))
  {
#ifdef WITH_OPENSSL
    BIO *bio;
    int r;
	/*
    if (psWS->proxy_host)
    {
		short v;
		unsigned int k = psWS->omode; 
		size_t n = psWS->count; // save the content length 
		psWS->omode &= ~WT_ENC; // mask IO and ENC 
		psWS->omode |= WT_IO_BUFFER;
		soap_begin_send(soap);

		DBGLOG(TEST, WT_MESSAGE(fdebug, "Connecting to proxy server\n"));
		sprintf(psWS->tmpbuf, "CONNECT %s:%d HTTP/%s", host, port, psWS->http_version);
		if ((psWS->error = psWS->fposthdr(soap, psWS->tmpbuf, NULL)))  return WT_INVALID_SOCKET;

#ifndef WITH_LEAN
      if (psWS->proxy_userid && psWS->proxy_passwd && strlen(psWS->proxy_userid) + strlen(psWS->proxy_passwd) < 761)
      { sprintf(psWS->tmpbuf + 262, "%s:%s", psWS->proxy_userid, psWS->proxy_passwd);
        strcpy(psWS->tmpbuf, "Basic ");
        soap_s2base64(soap, (const unsigned char*)(psWS->tmpbuf + 262), psWS->tmpbuf + 6, strlen(psWS->tmpbuf + 262));
        if ((psWS->error = psWS->fposthdr(soap, "Proxy-Authorization", psWS->tmpbuf)))
          return psWS->error;
      }
#endif
      if ((psWS->error = psWS->fposthdr(soap, NULL, NULL)) || soap_flush(soap)) return WT_INVALID_SOCKET;
      psWS->omode = k;
      k = psWS->imode;
      psWS->imode &= ~WT_ENC; // mask IO and ENC 
      v = psWS->version; // preserve 
      if (soap_begin_recv(soap))
        return WT_INVALID_SOCKET;
      psWS->version = v; 
      psWS->imode = k; 
      psWS->count = n; 
      soap_begin_send(soap);
    }
	*/
    if (!psWS->ctx && (psWS->iSocketErr = wt_ssl_auth_init(psWS)))
    { 
		psWS->iErrorCode=WTE_SSL_AUTH_ERROR;
		ehLogWrite("SSL error" CRLF "SSL authentication failed in tcp_connect(): check password, key file, and ca file.");
		return WT_INVALID_SOCKET;
    }

    psWS->ssl = SSL_new(psWS->ctx); // Nuova sessione 
    if (!psWS->ssl)
    { 
		//psWS->error = WT_SSL_ERROR;
		psWS->iErrorCode=WTE_SSL_NEW_ERROR;
		ehLogWrite("SSL error");
		return WT_INVALID_SOCKET;
    }

    if (psWS->session)
    { 
		if (!strcmp(psWS->session_host, pHostName) && psWS->session_port == port)
		{
			SSL_set_session(psWS->ssl, psWS->session);
			SSL_SESSION_free(psWS->session);
			psWS->session = NULL;
		}
    }
    psWS->imode |= WT_ENC_SSL;
    psWS->omode |= WT_ENC_SSL;
    bio = BIO_new_socket(fd, BIO_NOCLOSE);
    SSL_set_bio(psWS->ssl, bio, bio);
#ifndef WITH_LEAN
    if (psWS->iTimeoutSec)
#ifdef WIN32
		{ u_long nonblocking = 1;
		  ioctlsocket(fd, FIONBIO, &nonblocking);
		}
#else
		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
#endif
#endif

    while(TRUE)
    { 
		if ((r = SSL_connect(psWS->ssl)) <= 0)
      { int err = SSL_get_error(psWS->ssl, r);
        if (err != SSL_ERROR_NONE && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
        { //soap_set_sender_error(soap, ssl_error(soap, r), "SSL connect failed in tcp_connect()", WT_SSL_ERROR);
			ehLogWrite("SSL connect failed in tcp_connect()" CRLF);
          return WT_INVALID_SOCKET;
        }

        if (psWS->iTimeoutSec)
        { 
			struct timeval timeout;
			fd_set fds;
			if (psWS->iTimeoutSec> 0)
			{ timeout.tv_sec = psWS->iTimeoutSec;
			timeout.tv_usec = 0;
			}
			else
			{ timeout.tv_sec = -psWS->iTimeoutSec/1000000;
			timeout.tv_usec = -psWS->iTimeoutSec%1000000;
			}
			FD_ZERO(&fds);
			FD_SET(psWS->iSocket, &fds);
			while(TRUE)
			{ 
				int r = select((psWS->iSocket + 1), &fds, NULL, &fds, &timeout);
				if (r > 0)
				break;
			
				if (!r)
				{
					psWS->iSocketErr = 0;
					//DBGLOG(TEST, WT_MESSAGE(fdebug, "Connect timeout\n"));
					//soap_set_sender_error(soap, "Timeout", "connect failed in tcp_connect()", WT_TCP_ERROR);
					psWS->iErrorCode=WTE_SSL_TIMEOUT;
					ehLogWrite("Timeout" CRLF "connect failed in tcp_connect()");
					return WT_INVALID_SOCKET;
				}
          }
		  continue;
        }
      }
      break;
    }
#ifndef WITH_LEAN
    if (psWS->iTimeoutSec)
#ifdef WIN32
    { u_long blocking = 0;
      ioctlsocket(fd, FIONBIO, &blocking);
    }
#else
      fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);
#endif
#endif

    if (psWS->require_server_auth)
    {
		X509 *peer;
	    int err;

		if ((err = SSL_get_verify_result(psWS->ssl)) != X509_V_OK)
		{
			psWS->iErrorCode=WTE_SSL_CERT_ERROR_1;
			ehLogWrite("SSL certificate presented by peer cannot be verified in tcp_connect() %s", X509_verify_cert_error_string(err));
			return WT_INVALID_SOCKET;
		}

		peer = SSL_get_peer_certificate(psWS->ssl);
		if (!peer)
		{
			psWS->iErrorCode=WTE_SSL_CERT_ERROR_2;
			ehLogWrite("SSL error" CRLF "No SSL certificate was presented by the peer in tcp_connect()");
	        return WT_INVALID_SOCKET;
		}

		X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, psWS->msgbuf, sizeof(psWS->msgbuf));
		X509_free(peer);
		if (eh_tag_cmp(psWS->msgbuf, pHostName))
		{
			psWS->iErrorCode=WTE_SSL_CERT_ERROR_3;
			ehLogWrite("SSL error" CRLF "SSL certificate host name mismatch in tcp_connect()");
			return WT_INVALID_SOCKET;
		}
    }
#else
	psWS->iErrorCode= WTE_SSL_SEND_ERROR;
    return WT_INVALID_SOCKET;
#endif
  }
  return fd;
}
// <---------------- fine eh_tcp_connect

//
// eh_tcp_gethost() - Traduce un nome nell'ip
//
static int eh_tcp_gethost(FWEBSERVER *psWS, const char *addr, struct in_addr *inaddr)
{ 
	__int32 iadd = -1;
	struct hostent hostent, *host = &hostent;
	static struct hostent lastHost;
	CHAR *p,szServ[400];

	//
	// Prima controllo che l'indirizzo non sia un quartetto IP
	//
#ifdef VXWORKS
	  int hostint;
	  char *addrcopy = (char*)WT_MALLOC(soap, strlen(addr) + 1); /*copy of addr. */
	  /* inet_addr(), and hostGetByName() expect "char *"; addr is a "const char *". */
	  strncpy(addrcopy, addr, strlen(addr)+1);
	  iadd = inet_addr(addrcopy);
#else

		#if defined(_AIXVERSION_431) || defined(TRU64)
		  struct hostent_data ht_data;
		#endif
		  iadd = inet_addr(addr); // http://msdn.microsoft.com/en-us/library/ms738563(VS.85).aspx
#endif

	  if (iadd != -1)
	  { 
		  memcpy(inaddr, &iadd, sizeof(iadd));
			#ifdef VXWORKS
				WT_FREE(soap, addrcopy);
			#endif
			return WT_OK;
	  }

#if defined(__GLIBC__)
	  if (gethostbyname_r(addr, &hostent, psWS->buf, WT_BUFLEN, &host, &psWS->iSocketError) < 0)
		host = NULL;

#elif defined(_AIXVERSION_431) || defined(TRU64)
	  memset((void*)&ht_data, 0, sizeof(ht_data));
	  if (gethostbyname_r(addr, &hostent, &ht_data) < 0)
	  { 
			host = NULL;
			psWS->iSocketError = h_errno;
	  }

#elif defined(HAVE_GETHOSTBYNAME_R)
	  host = gethostbyname_r(addr, &hostent, psWS->buf, WT_BUFLEN, &psWS->iSocketError);

#elif defined(VXWORKS)
	  /* If the DNS resolver library resolvLib has been configured in the vxWorks
	   * image, a query for the host IP address is sent to the DNS server, if the
	   * name was not found in the local host table. */
	  hostint = hostGetByName(addrcopy);
	  if (hostint == ERROR)
	  { host = NULL;
		psWS->iSocketError = soap_errno; 
	  }
	  WT_FREE(soap, addrcopy);  /*ehFree() is placed after the error checking to assure that
				* errno captured is that from hostGetByName() */
#else

	//
	// A) Enter critical
	//
	EnterCriticalSection(&sWebTool.csGetHost);

	//
	// B) Controllo la cache
	//
	if (!addr) {
		return WT_ERR;
	}
	sprintf(szServ,"%s|",addr);
	p=ARSearch(sWebTool.arHost,szServ,FALSE);
	if (p)
	{
		ULONG hostint;
		CHAR *pHostInt;

		pHostInt=strstr(p,"|"); if (!p) ehError();
		pHostInt++;
		hostint=(ULONG) _atoi64(pHostInt); 
		if (!hostint) ehError();
		inaddr->s_addr = hostint;
		LeaveCriticalSection(&sWebTool.csGetHost);
		return WT_OK;
	}

	if (!(host = gethostbyname(addr))) // http://msdn.microsoft.com/en-us/library/ms738524(VS.85).aspx
	psWS->iSocketErr = h_errno; 
	LeaveCriticalSection(&sWebTool.csGetHost);

#endif

	if (!host)
	{ //DBGLOG(TEST, WT_MESSAGE(fdebug, "Host name not found\n"));
		return WT_ERR;
	}


#ifdef VXWORKS
	  inaddr->s_addr = hostint;
#else
	  memcpy(inaddr, host->h_addr, host->h_length);
#endif

	//
	// B) Aggiungo nella cache il dominio ricercato
	//
	sprintf(szServ,"%s|%u",addr,inaddr->s_addr);
	ARAdd(&sWebTool.arHost,szServ);

	//
	// C) Leave critical
	//

//	 LeaveCriticalSection(&sWebTool.csGetHost);
	return WT_OK;
}
static int eh_tcp_shutdownsocket(int fd, int how) {return shutdown(fd, how);}

//
// eh_tcp_disconnect()
//

static int eh_tcp_disconnect(FWEBSERVER *psWS)
{
#ifdef WITH_OPENSSL
  if (psWS->ssl)
  { int r, s = 0;
    if (psWS->session) SSL_SESSION_free(psWS->session);
    if (*psWS->szHost)
    { psWS->session = SSL_get1_session(psWS->ssl);
      if (psWS->session)
      { strcpy(psWS->session_host, psWS->szHost);
        psWS->session_port = psWS->port;
      }
    }
    r = SSL_shutdown(psWS->ssl);
     if (r != 1)
    { s = ERR_get_error();
      if (s)
      { if (wt_valid_socket(psWS->iSocket))
        { eh_tcp_shutdownsocket(psWS->iSocket, 1);
          psWS->iSocket = WT_INVALID_SOCKET;
        }
        r = SSL_shutdown(psWS->ssl);
      }
    }
   //  DBGLOG(TEST, if (s) SOAP_MESSAGE(fdebug, "Shutdown failed: %d\n", SSL_get_error(psWS->ssl, r)));
    SSL_free(psWS->ssl);
    psWS->ssl = NULL;
    if (s) return WT_SSL_ERROR;
    ERR_remove_state(0);
  }
#endif

  if (wt_valid_socket(psWS->iSocket) && !(psWS->omode & WT_IO_UDP))
  { eh_tcp_shutdownsocket(psWS->iSocket, 2);
    closesocket(psWS->iSocket);
    psWS->iSocket = WT_INVALID_SOCKET;
  }
  return WT_OK;
}

static CHAR * wt_host_format(FWEBSERVER *psWS)
{
	static CHAR szHostFormat[256];
	if (psWS->port!=80)
		sprintf(szHostFormat, "%s:%d", psWS->szHost, psWS->port);
		else
		strcpy(szHostFormat, psWS->szHost); 
	return szHostFormat;
}

static CHAR * wt_user_agent(FWEBSERVER *psWS)
{
	static CHAR szUserAgent[256];
	strcpy(szUserAgent,"User-Agent: ");
	if (psWS->lpUserAgent) strcat(szUserAgent,psWS->lpUserAgent); else strcat(szUserAgent,WT_USER_AGENT);
	strcat(szUserAgent,CRLF);
	return szUserAgent;
}


static CHAR * wt_authorization(FWEBSERVER *psWS)
{
	static CHAR szRet[256];
	CHAR *pCode,szServ[200];

	sprintf(szServ,"%s:%s",psWS->lpUser,psWS->lpPass);
	pCode=strEncode(szServ,SE_BASE64,NULL);
	sprintf(szRet,"Authorization: basic %s" CRLF,pCode);
	ehFree(pCode);
	return szRet;
}

static BOOL eh_tcp_getheader(FWEBSERVER *psWS,CHAR *lpBufferDest,DWORD dwSizeBuffer)
{
	CHAR *lp;
	DWORD  dwCount;
	BOOL bHeaderError;
	INT iB;

	lp=lpBufferDest; dwCount=0;
	bHeaderError=FALSE;
	while (TRUE)
	{
		 iB=eh_tcp_recv(psWS,lp,1);
		 if (!iB) 
		 {
			 bHeaderError=TRUE; 
			 if (!psWS->iErrorCode) psWS->iErrorCode=WTE_CONNECTION_CLOSE; 
			 break;
		 } // Definisco che un header non può essere più grande di 2048

		 //		 if (!iB) break;
		 if (iB<0) 
		 {
			 if (!psWS->iErrorCode) psWS->iErrorCode=WTE_RECV;
			 psWS->iSocketErr=WSAGetLastError();
#ifdef _DEBUG
			bHeaderError=TRUE; 
			//printf("recv failed: %d\n", psWS->iSocketError); 
#endif
			break;
		 }

		 lp++; dwCount++;

		 //printf("%c,%d",iB,iCount); waitKey();
		 if (dwCount>=2048||dwCount>=dwSizeBuffer) {bHeaderError=TRUE; psWS->iErrorCode=WTE_HEADER_ERROR; break;} // Definisco che un header non può essere più grande di 2048
		 if (dwCount>3) {if (!memcmp(lp-4,CRLF CRLF,4)||!memcmp(lp-2,"\n\n",2)) break;}
	}
	*lp=0;
	return bHeaderError;
}


//#define WT_BUFFERSIZE 0xF000

// Ritorna 0 = Tutto OK, pagina caricata
// Ritorna 1 = Errore
static INT WT_SocketSendControl(FWEBSERVER *psWS,
								BYTE *lpSendData,
								ULONG iSizeData,
								void *(*funcNotify)(INT cmd,LONG Info,CHAR *))
{
	INT i,iB,iBlocco;
//	BYTE *psWS->pszSocketBuffer;
	BYTE *lp;

	BOOL fLoop=FALSE;
	CHAR *p;
	BOOL fChunked=FALSE;
	DWORD dwNextSize;//,iContentLength=0;
	INT iCount,iFirst;//,iContentOffset=0;
	EH_AR arHeader;
	BOOL bHeaderError;
	BOOL bSizeDef=FALSE;

	psWS->pszSocketBuffer=ehAlloc(WT_BUFFERSIZE+1);	if (!psWS->pszSocketBuffer) win_infoarg("GlobalAlloc failed (%d)\n", osGetError());

	*psWS->pszSocketBuffer=0;

	//
	// Trasmetto la richiesta al server ------------------------------------------>
	//
	if (psWS->bCommToLog) ehLogWrite("WS:Request" CRLF "--->" CRLF "%s",lpSendData);
	if (eh_tcp_send(psWS,lpSendData,iSizeData)) 
	{
		ehFreePtr(&psWS->pszSocketBuffer); 
		return 1;
	}

	// Segnalo che il trasferimento è finito
#ifdef WITH_OPENSSL
  if (!psWS->ssl && wt_valid_socket(psWS->iSocket) && !psWS->keep_alive && !(psWS->omode & WT_IO_UDP))
    eh_tcp_shutdownsocket(psWS->iSocket, SD_SEND); 
#else
	if (wt_valid_socket(psWS->iSocket) && !psWS->keep_alive && !(psWS->omode & WT_IO_UDP))
	{
		if (eh_tcp_shutdownsocket( psWS->iSocket, SD_SEND)!=WT_OK) {ehFreePtr(&psWS->pszSocketBuffer); return 1;}
	}
#endif

	psWS->dwDataSize=0; // Resetto il contatore dei byte ricevuti
	psWS->pPageHeader=NULL;
	if (funcNotify) 
	{
		(funcNotify)(WS_REALSET,0,0); // Dico che non conosco le dimensioni
		(funcNotify)(WS_PROCESS,iSizeData,lpSendData);
	}
	
	//
	// Implementazione chunked del 18/10/2007
	// di default è false
	//

	//
	// Leggo intestazione
	//
	bHeaderError=eh_tcp_getheader(psWS,psWS->pszSocketBuffer,WT_BUFFERSIZE);
	if (bHeaderError) ehLogWrite("HeadError:[%s]>Timeout: %d",psWS->lpWebPage,psWS->iTimeoutSec);
	if (psWS->bCommToLog) ehLogWrite("bHeaderError:%d",bHeaderError);

	if (!bHeaderError&&strstr(psWS->pszSocketBuffer,"100 Continue")) 
	{
		if (psWS->bCommToLog) ehLogWrite("-100 Continue");
		memset(psWS->pszSocketBuffer,0,WT_BUFFERSIZE);
		bHeaderError=eh_tcp_getheader(psWS,psWS->pszSocketBuffer,2048);
	}


	//
	//  Ho un Header
	//
	if (!bHeaderError)  {

		psWS->pPageHeader=strDup(psWS->pszSocketBuffer);
		if (psWS->bCommToLog) ehLogWrite("Header[%s]",psWS->pPageHeader);

		//
		// Creo array Intestazione ed analizzo Header
		//
		arHeader=ARCreate(psWS->pPageHeader,CRLF,&iCount);
		if (iCount<2) {ARDestroy(arHeader); arHeader=ARCreate(psWS->pPageHeader,"\n",&iCount);}
		if (iCount<1) {ARDestroy(arHeader); ehFreePtr(&psWS->pszSocketBuffer); psWS->iErrorCode=WTE_HEADER_ERROR; return 1;} // Errore header

		//
		// Controllo della prima linea (status)
		//
		iFirst=0; 

		// Se devo controllare gli errore è non è 200 , ritorno
		if (!psWS->fNoHttpErrorControl&&
			!strstr(arHeader[iFirst],"200 OK"))
		{
			psWS->iErrorCode=WTE_HTTP_HEADER_NOT_200;
			ARDestroy(arHeader); 
			ehFreePtr(&psWS->pszSocketBuffer); 
			return 1;
		}
		iFirst++;
		for (i=iFirst;arHeader[i];i++)
		{
			CHAR *pHeader=arHeader[i];
			//
			// Content-Length
			//
			if (!strCaseBegin(pHeader,"Content-Length:"))
			{
				p=pHeader+15;//arHeader[i]+15;
				psWS->dwContentLength=atoi(p);
				fLoop=TRUE;
				if (funcNotify) (funcNotify)(WS_REALSET,psWS->dwContentLength,0); // Dico che non conosco le dimensioni
				WSBufRealloc(psWS,psWS->dwContentLength+1); // Dimensioni che servono + 1 ( 0 finale)
				bSizeDef=TRUE;
			}

			//
			// Altri parametri
			//
			if (!strCaseBegin(pHeader,"Connection: close")) fLoop=TRUE;
			if (!strCaseBegin(pHeader,"Connection: Keep-Alive")) fLoop=TRUE;
			if (!strCaseBegin(pHeader,"Transfer-Encoding: chunked")) fChunked=TRUE;
		}
		ARDestroy(arHeader);
		psWS->iDataReaded=0;
		if (psWS->bCommToLog) ehLogWrite("fLoop:%d, fChunked:%d",fLoop,fChunked);

	}
	else
	{
		psWS->pPageHeader=strDup(""); // Niente Header
		if (!psWS->fNoHttpErrorControl) {ehFreePtr(&psWS->pszSocketBuffer); return 1;}

		// MAI PROVATO !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		psWS->iDataReaded=strlen(psWS->pszSocketBuffer);
		memcpy(psWS->pData+psWS->iDataReaded,psWS->pszSocketBuffer,psWS->iDataReaded); 
	}
	dwNextSize=0;

	// _dx_(0,20,"Chunked=%d, Datasize:%d iContentLength:%d",fChunked,psWS->iDataSize,iContentLength);
	
	//
	// Carico Corpo della pagina CHUNKED
	//
	if (fChunked)
	{
		INT iStep=0;
		if (psWS->bCommToLog) ehLogWrite("Chunked start");
		while (TRUE)
		{
			if (funcNotify) (funcNotify)(WS_DISPLAY,psWS->iDataReaded,(CHAR *) &psWS->dwDataSize);

			//
			// Leggo dimensioni del blocco in arrivo
			//
			lp=psWS->pszSocketBuffer; iCount=0; memset(psWS->pszSocketBuffer,0,WT_BUFFERSIZE);
			while (TRUE)
			{
				//iB=recv(psWS->iSocket, lp, 1, 0); 
				iB=eh_tcp_recv(psWS,lp,1);
				if (!iCount&&*lp<'0') continue; // Cerco il primo cattere buono
			
				lp++; iCount++;
				if (iB==SOCKET_ERROR) {ehFreePtr(&psWS->pszSocketBuffer); psWS->iErrorCode=WTE_SOCKET_ERROR; return 1;}
				if (iCount>2) {if (!memcmp(lp-2,CRLF,2)) {break;}}
			}
			*lp=0;

			for (lp=psWS->pszSocketBuffer;*lp;lp++)  {if (*lp<'0') {*lp=0; break;}}//lp=strstr(lp,CRLF); *lp=0;
			
			if (!strcmp(psWS->pszSocketBuffer,"0")) break; // Finito
			iBlocco=xtoi(psWS->pszSocketBuffer); //if (!iBlocco) break; // Finito
			if (!iBlocco) break;
			//_dx_(0,300+iStep*16,"CHUNKED %d -> Blocco:%d [%s], Readed:%d (Adesso:%d)",iStep,iBlocco,psWS->pszSocketBuffer,psWS->iDataReaded,psWS->iDataReaded+iB); //Sleep(1000);
			//iStep++;
			if (iBlocco<1||iBlocco>12000000) //iBlocco>128000) 
			{
				ehFreePtr(&psWS->pszSocketBuffer); 
				psWS->iErrorCode=WTE_BLOCK_TOO_BIG; 
				return 1; // BLocco TRoppo Grande
			} 

			//
			// Controllo overloading dei dati
			//
			dwNextSize=(psWS->iDataReaded+iBlocco); 
			if (dwNextSize>=psWS->dwDataSize) WSBufRealloc(psWS,dwNextSize+(dwNextSize/10)); // Dimensioni che servono + un 10%
			
			// Carico il blocco
			iCount=0;
			while (TRUE)
			{
				//iB=recv(psWS->iSocket, psWS->pData+psWS->iDataReaded, iBlocco-iCount, 0);
				iB=eh_tcp_recv(psWS,psWS->pData+psWS->iDataReaded, iBlocco-iCount);
				
				//_dx_(0,300+iStep*16,"[%s]",psWS->pData+psWS->iDataReaded); //Sleep(1000);
				//iStep++;
				if (iB==SOCKET_ERROR) {ehFreePtr(&psWS->pszSocketBuffer); psWS->iErrorCode=WTE_SOCKET_ERROR; return 1;}
				
				iCount+=iB; if (iCount==iBlocco) break;
				psWS->iDataReaded+=iB;
			}
			psWS->iDataReaded=dwNextSize;
			//psWS->pData[psWS->iDataReaded]=0; // Mi serve per i controlli
		}
	}
	//
	// Carico Corpo della pagina con lunghezza
	//
	else
	{
		iCount=0;
		if (psWS->bCommToLog) ehLogWrite("WT_BUFFERSIZE:%d",WT_BUFFERSIZE);
		while (fLoop)
		{
			if (funcNotify) (funcNotify)(WS_DISPLAY,psWS->iDataReaded,(CHAR *) &psWS->dwDataSize);

			//
			// MSG_PEEK
			//
			//iB=recv(psWS->iSocket, psWS->pszSocketBuffer, WT_BUFFERSIZE, 0); 
			iB=eh_tcp_recv(psWS, psWS->pszSocketBuffer, WT_BUFFERSIZE); 

			if (psWS->bCommToLog) ehLogWrite("iB:%d - iDataReaded:%d, iDataSize:%d",iB,psWS->iDataReaded,psWS->dwDataSize);

			//_dx_(0,40+iCount*16,"-> Letti:%d, Readed:%d (Adesso:%d)   %d",iB,psWS->iDataReaded,psWS->iDataReaded+iB,psWS->iDataSize); //Sleep(1000);
			//iCount++;
			if (iB==SOCKET_ERROR) {ehFreePtr(&psWS->pszSocketBuffer); psWS->iErrorCode=WTE_SOCKET_ERROR; return WSAGetLastError();}
			
			if (!iB) 
			{
				 //if (fDebug) win_infoarg("iB=0");
				 //if (!bSizeDef) psWS->iDataReaded=psWS->iDataSize;
				 break; // Mi esce di qui
			}

			//
			// Controllo overloading dei dati
			//
			dwNextSize=(psWS->iDataReaded+iB); //	 iCount+=iB;
			if (dwNextSize>=psWS->dwDataSize)
			{
				if (bSizeDef) ehError();
				WSBufRealloc(psWS,dwNextSize+(dwNextSize/10)+1); // Dimensioni che servono + un 10% + (1 per il byte di fine)
			}

			//
			// Copio i dati ricevuti nel buffer
			//
			memcpy(psWS->pData+psWS->iDataReaded,psWS->pszSocketBuffer,iB); 
			psWS->iDataReaded=dwNextSize;
			psWS->pData[psWS->iDataReaded]=0; // Mi serve per i controlli

			if (bSizeDef) // DImensione predefinita
			{
				if (psWS->iDataReaded>=(INT) (psWS->dwDataSize-1)) 
				{
					if (psWS->bCommToLog) ehLogWrite("Break-oversize: psWS->iDataReaded=%d|psWS->iDataSize=%d",psWS->iDataReaded,psWS->dwDataSize);
					break; // Caricati
				}
			}
		
		}
	}
 
/*
30/11/2009 18:31:14 | fLoop:1, fChunked:0
30/11/2009 18:31:14 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:0
30/11/2009 18:31:14 | iB:2760 - WT_BUFFERSIZE:65536, iDataSize:1519
30/11/2009 18:31:14 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:4555
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:6073
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:7591
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:9109
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:10627
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:12145
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:13663
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:15181
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:15181
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:18217
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:18217
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:21253
30/11/2009 18:31:15 | iB:1380 - WT_BUFFERSIZE:65536, iDataSize:21253
30/11/2009 18:31:15 | iB:1071 - WT_BUFFERSIZE:65536, iDataSize:24289
30/11/2009 18:31:15 | iB:0 - WT_BUFFERSIZE:65536, iDataSize:24289
30/11/2009 18:31:15 | iDataReaded: (0:23151)
30/11/2009 18:31:15 | WS:WT_SocketSendControl()
*/

	//
	//
	//

	if (psWS->bCommToLog) ehLogWrite("iDataReaded: (%d:%d)",psWS->iDataReaded,iB);

	//if (!iSizeLoad) iSizeLoad=psWS->iPageSize;
	//psWS->iDataSize=psWS->iDataReaded; // Tolto

	// Notifico la fine alla procedura esterna
	if (funcNotify) (funcNotify)(WS_DISPLAY,psWS->iDataReaded,(CHAR *) &psWS->iDataReaded);
	if (!psWS->pData) WSBufRealloc(psWS,1); // Dimensioni che servono + un 10% + (1 per il byte di fine)
	psWS->pData[psWS->iDataReaded]=0; // Ci metto uno zero alla fine non si sa mai

/*
	psWS->lpPageNoHeader=LGetContentPoint(psWS->lpPageResult,fChunked,&iContentOffset);

	if (!psWS->lpPageNoHeader) psWS->lpPageNoHeader=psWS->lpPageResult;
	if (iContentLength)
		psWS->iContentSize=iContentLength;//-2; // Dovrebbe essere cosi - CRLF
		else
		psWS->iContentSize=psWS->iByteStored-iContentOffset;
		*/
	ehFreePtr(&psWS->pszSocketBuffer);
	return FALSE;
}

INT FWebGetFree(FWEBSERVER *psWS) // GET/HEAD
{
	if (psWS->pPageHeader) {ehFree(psWS->pPageHeader); psWS->pPageHeader=NULL;}
	if (psWS->pData) 
		{ehFree(psWS->pData); 
		 psWS->pData=NULL;
		}
	return FALSE;
}


// ##################################

static int WGFilter(unsigned int code, LPEXCEPTION_POINTERS ep) {

	ehLogWrite("WebGet EXCEPTION code: %d",code);
	return EXCEPTION_CONTINUE_SEARCH;
}

void FWebMpFree(FWS_MP **arMP)
{
	INT a;
	for (a=0;arMP[a];a++)
	{
		if (arMP[a]->lpData) {ehFree(arMP[a]->lpData); arMP[a]->lpData=NULL;}
		if (arMP[a]->lpContentType) {ehFree(arMP[a]->lpContentType); arMP[a]->lpContentType=NULL;}
		
		if (arMP[a]->lpContentTransferEncoding) {ehFree(arMP[a]->lpContentTransferEncoding); arMP[a]->lpContentTransferEncoding=NULL;}
		if (arMP[a]->lpFileName) {ehFree(arMP[a]->lpFileName); arMP[a]->lpFileName=NULL;}
	}
}

//
// FWebGetDirect()
//
EN_HTTP_ERR FWebGetDirect(FWEBSERVER *psWS,CHAR *lpCmd,void * (* funcNotify)(INT cmd,LONG Info,CHAR *)) // GET/HEAD
{
	INT iWE;
	INT iRet;
	UINT iUrlSize=32000;

	URL_COMPONENTS uc;
	INT a;
	DWORD dwRequestSize;
	ULONG ulSize;
	BYTE *p;
	BYTE *pSendOtherHeader=NULL;
	time_t dwStart;

	WebStartup();
	
	dwRequestSize=strlen(psWS->lpUri)+(psWS->lpPostArgs?strlen(psWS->lpPostArgs):0);
	if (dwRequestSize>iUrlSize) iUrlSize=dwRequestSize+1024;
	psWS->lpThreadRequest=ehAllocZero(iUrlSize);
	if (!psWS->lpThreadRequest) {ehLogWrite("FWebGetDirect(): lpThreadRequest = NULL"); return WTE_OUTOFMEMORY;}

	psWS->lpThreadURI=ehAllocZero(iUrlSize);
	if (!psWS->lpThreadURI) {ehLogWrite("FWebGetDirect(): lpThreadURI = NULL"); return WTE_OUTOFMEMORY;}

	//
	// Gestione Cookies (new 11/2008)
	//
	if (psWS->pdmiCookie)
	{
		CHAR *pCookie=web_cookie_string(psWS->pdmiCookie);
		if (psWS->lpSendOtherHeader)
		{
			pSendOtherHeader=ehAlloc(strlen(psWS->lpSendOtherHeader)+strlen(pCookie)+10);
			strcpy(pSendOtherHeader,psWS->lpSendOtherHeader);
			strcat(pSendOtherHeader,pCookie);
			ehFree(pCookie);
		}
		else
		{
			pSendOtherHeader=pCookie;
		}
	}
	else
	{
		if (psWS->lpSendOtherHeader)
			{
				pSendOtherHeader=strDup(psWS->lpSendOtherHeader);
			}	
	}

	if (funcNotify) (funcNotify)(WS_REALSET,0,0); // Dico che non conosco le dimensioni

// __try {
	
	iRet=0; //*lpiErr=0;
	psWS->iErrorCode=0;
	psWS->iSocket=WT_INVALID_SOCKET;

	if (psWS->lpUri)
	{
		BYTE *pQuest=NULL;
		BYTE *pBuf;
		ZeroFill(uc);
		uc.dwStructSize = sizeof(uc);
		uc.lpszHostName = psWS->szHost;
		uc.dwHostNameLength = sizeof(psWS->szHost)-1;
		uc.lpszUrlPath = psWS->lpThreadURI;
		uc.dwUrlPathLength = iUrlSize-1;
		pBuf=strDup(psWS->lpUri);
		pQuest=strstr(pBuf,"?"); if (pQuest) *pQuest=0; // Tolgo i dati dietro
		if (!InternetCrackUrl(pBuf,0, 0, &uc)) {ehLogWrite("Errore in InternetCrackUrl(%s) %d",psWS->lpUri,osGetError()); iRet=100; goto ESCI;}
		if (uc.nScheme==INTERNET_SCHEME_UNKNOWN) {ehLogWrite("INTERNET_SCHEME_UNKNOWN (%s) %d",psWS->lpUri,osGetError()); iRet=101; goto ESCI;}
		psWS->lpWebServer=psWS->szHost;
		if (!psWS->lpThreadURI) {ehLogWrite("psWS->lpThreadURI==NULL"); iRet=101; goto ESCI;}
		if (!*psWS->lpThreadURI) strcpy(psWS->lpThreadURI,"/");
		if (pQuest) {*pQuest='?'; strcat(psWS->lpThreadURI,pQuest);}
		psWS->lpWebPage=psWS->lpThreadURI;
		if (uc.nScheme==INTERNET_SCHEME_HTTPS) psWS->bHttps=TRUE;
		//
		// Setto la porta di accesso con un altra porta 1/2009
		//
		psWS->port=uc.nPort;
		ehFree(pBuf);
	}

	if (iRet) goto ESCI;

	//
	// HTTPS = SSL
	//
	
	if (psWS->bHttps) 
	{
#ifdef WITH_OPENSSL
		if (!psWS->port) psWS->port=443;
		if (wt_ssl_auth_init(psWS)) 
		{
			ehLogWrite("SSL Error");  iRet=-100; goto ESCI; // L'host non esiste
		}
#else
		ehExit("Compilare con WITH_OPENSSL");
#endif
	}   

/*
		// -----------------------------------------------------
		// Richiedo socket per porta iPort (80)
		//
		switch (psWS->bHttps)
		{
			case 1:
				psWS->sPeer.sin_family = AF_INET;
				psWS->sPeer.sin_port = htons(443); // 443= SSL
				memcpy(&(psWS->sPeer.sin_addr.s_addr), psWS->hWebServer->h_addr, sizeof(int));
				psWS->iSocket= socket(AF_INET, SOCK_STREAM, 0);//IPPROTO_TCP);
				break;
					
			default:
				psWS->sPeer.sin_family = AF_INET;
				psWS->sPeer.sin_port = htons(80);  // 80= HTTP
				memcpy(&(psWS->sPeer.sin_addr.s_addr), psWS->hWebServer->h_addr, sizeof(int));
				psWS->iSocket= socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				break;
		}
		if (psWS->iSocket==0) {iRet=-3; goto ESCI;} 

		// -----------------------------------------------------
		// Richiedo connessione alla porta tramite socket
		//
		if (connect(psWS->iSocket, (struct sockaddr *)&psWS->sPeer, sizeof(psWS->sPeer)))
		{
			iRet=-4; goto ESCI;
		}
*/

		// GET /wiki/Pagina_principale HTTP/1.1 
		// Connection: Keep-Alive
		// User-Agent: Mozilla/5.0 (compatible; Konqueror/3.2; Linux) (KHTML, like Gecko)
		// Accept: text/html, image/jpeg, image/png, text/*, image/*, */*
		// Accept-Encoding: x-gzip, x-deflate, gzip, deflate, identity
		// Accept-Charset: iso-8859-1, utf-8;q=0.5, *;q=0.5 
		// Accept-Language: en
		// Host: it.wikipedia.org

		if (!strCaseCmp(lpCmd,"GET")||!strCaseCmp(lpCmd,"HEAD"))
		{
			sprintf(psWS->lpThreadRequest,"%s %s HTTP/1.1" CRLF, lpCmd,psWS->lpWebPage);
			if (!strCaseCmp(lpCmd,"GET")) strcat(psWS->lpThreadRequest,wt_user_agent(psWS)); // User-agent
			if (!strEmpty(pSendOtherHeader)) strcat(psWS->lpThreadRequest,pSendOtherHeader);
			sprintf(psWS->lpThreadRequest+strlen(psWS->lpThreadRequest),"Host: %s" CRLF,wt_host_format(psWS));
			if (!strEmpty(psWS->lpUser)) strcat(psWS->lpThreadRequest,wt_authorization(psWS)); // User-agent
			
			strcat(psWS->lpThreadRequest,CRLF);
			ulSize=strlen(psWS->lpThreadRequest);
		}
		else if (!strCaseCmp(lpCmd,"NOHEAD")) {
		
			strcpy(psWS->lpThreadRequest,psWS->lpPostArgs);
			ulSize=strlen(psWS->lpThreadRequest);
		}
		else
		{
			 // ---------------------------------------------------------------------
			 // Non è un MULTI PART = POST NORMALE x-www-form-urlencoded
			 //
			 if (!psWS->fMultiPartData)
			 {
				CHAR *lpWebUri=strDup(psWS->lpWebPage);
				CHAR *pq=strstr(lpWebUri,"?");
				CHAR *pArgs;

				if (pq) 
				{
					*pq=0; pArgs=pq+1;
				}
				else
				{
					if (!psWS->lpPostArgs) ehExit("Richiesta POST senza argomenti");
					pArgs=psWS->lpPostArgs;
				}
				
				if (!psWS->lpContentType) psWS->lpContentType="application/x-www-form-urlencoded";

				//if (!psWS->lpSendOtherHeader) psWS->lpSendOtherHeader="";
				sprintf(psWS->lpThreadRequest,
						"POST %s HTTP/1.1" CRLF
						"Host: %s" CRLF
						"%s" //WT_USER_AGENT_DEFAULT CRLF
						"Content-Type: %s" CRLF 
						"Content-Length: %d" CRLF 
						"%s" // pOtherHeader
						CRLF
						"%s" ,
						psWS->lpWebPage,
						wt_host_format(psWS),
						wt_user_agent(psWS),
						psWS->lpContentType,// application/x-www-form-urlencoded
						strlen(pArgs),
						strEver(pSendOtherHeader),
						pArgs);

				if (strlen(psWS->lpThreadRequest)>iUrlSize) ehError();
				ehFree(lpWebUri);
				ulSize=strlen(psWS->lpThreadRequest);
			 }
			 else
			 {
				#define BOUNDARY_ITEM "AaB03x"
				CHAR *lpBuffer=ehAllocZero(8000);
				INT iHeaderSize;
				BYTE *lpDest;

				// A) Calcolo approsimativamente le dimensioni del lpReq in multi part
				ulSize=1000;
				for (a=0;psWS->arMP[a];a++)
				{
					ulSize+=psWS->arMP[a]->ulSize+200;
				}

				// B) Rialloco memoria necessaria alla richiesta
				// Vedi -----------> http://www.diodati.org/w3c/html401/interact/forms.html#h-17.13.4.2
				//
				ehFreePtr(&psWS->lpThreadRequest); psWS->lpThreadRequest=ehAllocZero(ulSize);
				sprintf(psWS->lpThreadRequest,
						"POST %s HTTP/1.1" CRLF
						"Host: %s" CRLF
						"Content-Length: " TAG_LEN CRLF
						"%s" //WT_USER_AGENT_DEFAULT CRLF
						"Content-Type: multipart/form-data; boundary=" BOUNDARY_ITEM CRLF CRLF,
						psWS->lpWebPage,
						wt_host_format(psWS),
						wt_user_agent(psWS));
				iHeaderSize=strlen(psWS->lpThreadRequest);
				lpDest=psWS->lpThreadRequest+iHeaderSize;
				for (a=0;psWS->arMP[a];a++)
				{
					switch (psWS->arMP[a]->iType)
					{
						// Valore alfanumerico
						case 0: 
								sprintf(lpBuffer,"--" BOUNDARY_ITEM CRLF
										"Content-Disposition: form-data; name=\"%s\"" CRLF CRLF,
										psWS->arMP[a]->szName);
								strcat(lpDest,lpBuffer); 
								strcat(lpDest,psWS->arMP[a]->lpData); strcat(lpDest,CRLF);
								lpDest=lpDest+strlen(lpDest);
								break;

						// File in formato Binario
						case 1: 
								sprintf(lpBuffer,"--" BOUNDARY_ITEM CRLF
										"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"" CRLF,
										psWS->arMP[a]->szName,
										psWS->arMP[a]->lpFileName);
								strcat(lpDest,lpBuffer); 

								if (psWS->arMP[a]->lpContentType) 
								{
								 sprintf(lpBuffer,"Content-Type: %s" CRLF,psWS->arMP[a]->lpContentType);
								 strcat(lpDest,lpBuffer); 
								}

								if (psWS->arMP[a]->lpContentTransferEncoding) 
								{
								 sprintf(lpBuffer,"Content-Transfer-Encoding: %s" CRLF,psWS->arMP[a]->lpContentTransferEncoding);
								 strcat(lpDest,lpBuffer); 
								}

								 sprintf(lpBuffer,"Content-Length: %u" CRLF,psWS->arMP[a]->ulSize+2); strcat(lpDest,lpBuffer); 

								 strcat(lpDest,CRLF);
								 lpDest=lpDest+strlen(lpDest);
								 memcpy(lpDest,psWS->arMP[a]->lpData,psWS->arMP[a]->ulSize);
								 lpDest+=psWS->arMP[a]->ulSize;
								 strcat(lpDest,CRLF);
								 lpDest=lpDest+strlen(lpDest);
								 break;
						}
					 }
					 strcat(lpDest,"--" BOUNDARY_ITEM "--" CRLF); lpDest=lpDest+strlen(lpDest);
					 ulSize=(LONG) lpDest - (LONG) psWS->lpThreadRequest; 
					 p=strstr(psWS->lpThreadRequest,TAG_LEN); sprintf(lpBuffer,"%10d",ulSize-iHeaderSize); memcpy(p,lpBuffer,strlen(lpBuffer));
					 ehFree(lpBuffer);
					 //win_infoarg("%s",lpReq);
				 }
			}

		psWS->pPageHeader=NULL;
		psWS->pData=NULL;
		dwStart=clock();

		iRet=eh_tcp_connect(psWS,psWS->lpUri,psWS->lpWebServer,psWS->port);
		if (iRet<0) {ehLogWrite("eh_tcp_connect() Error"); iRet=psWS->iErrorCode; goto ESCI;}
		iRet=0;

		psWS->dwTimeConnect=(DWORD) (clock()-dwStart);

		//
		// Spedisco e aspetto la risposta <---------------------------------------
		//
		iWE=WT_SocketSendControl(psWS,psWS->lpThreadRequest,ulSize,funcNotify);
  		if (iWE) {iRet=psWS->iErrorCode;}

		if (psWS->bCommToLog)
		{
				ehLogWrite("WS:WT_SocketSendControl()" CRLF "<---- (Error: %d) " CRLF "%s",iWE,psWS->pPageHeader);
//				ehLogWrite("Data:"CRLF" %s",psWS->pData);
		}

		psWS->dwTimeLoad=(DWORD) (clock()-dwStart);

		ESCI:
			iRet=iRet; // non toccare !

		if (psWS->bCommToLog) ehLogWrite("Post:1");

		if (!iRet&&psWS->pdmiCookie&&psWS->pPageHeader)
		{ 
			if (psWS->pdmiCookie->Hdl<0) web_cookie_create(psWS->pdmiCookie);
			web_cookie_get(psWS->pdmiCookie,psWS->pPageHeader);
		}

	//FINE:
	ehFreeNN(pSendOtherHeader);
	if (psWS->bCommToLog) ehLogWrite("Post:2");

	_WebGetFreeResource(psWS);

	psWS->iWebGetError=iRet;
	psWS->pWebGetError=ehWebError(psWS->iWebGetError);

	return iRet;
}

static void _WebGetFreeResource(FWEBSERVER *psWS)
{
	eh_tcp_disconnect(psWS);
	ehFreePtr(&psWS->pszSocketBuffer);
	ehFreePtr(&psWS->lpThreadRequest); 
	ehFreePtr(&psWS->lpThreadURI); 
	psWS->lpWebPage=0;
}

//
// FWebCopySocket()
// ritorna FALSE se ci sono errori
// 
BOOL FWebCopySocket(CHAR *lpWebAddress,
					CHAR *lpLocalFile,
					BOOL fFailIsExist,
					void * (*funcNotify)(INT cmd,LONG Info,CHAR *),
					INT iTimeOut,
					BOOL fOsEventAnalisys,
					FWEBSERVER *psWS) // Può essere null
{
	FWEBSERVER sWS;
	INT iWebError;
	FILE *ch;
	BOOL bRet=TRUE;

	WebStartup();
	if (fileCheck(lpLocalFile)&&fFailIsExist) return TRUE;
	
	ZeroFill(sWS);
	sWS.lpUri=lpWebAddress;
	sWS.keep_alive=TRUE;

	if (!iTimeOut)  iWebError=FWebGetDirect(&sWS,"GET",funcNotify);
					else
					iWebError=FWebGetDirectMT(&sWS,"GET",funcNotify,iTimeOut,fOsEventAnalisys);
	if (psWS) memcpy(psWS,&sWS,sizeof(FWEBSERVER));

	if (!iWebError)
	{
		ch=fopen(lpLocalFile,"wb"); if (ch==NULL) return TRUE;
		fwrite(sWS.pData,sWS.iDataReaded,1,ch);
		fclose(ch);
	//	FWebGetFree(&sWS);  
		bRet=false;
	}
	
	FWebGetFree(&sWS);
	return bRet;

	//return FALSE;
}


//
// _UrlGetThread()
//
static DWORD WINAPI _UrlGetThread(LPVOID pvoid) {

	EH_URLGET *psWeb=pvoid;
	if (!psWeb->psWS) return FALSE;
	psWeb->enWebError=FWebGetDirect(psWeb->psWS,psWeb->lpCmd,psWeb->psWS->SubControl);
	if (psWeb->funcNotifyEnd) psWeb->funcNotifyEnd(psWeb);
	psWeb->fRun=FALSE;

	return FALSE;
}

//
// FWebGetDirectMT()
//
// Legge una pagina Web creando un Thread
// = FWebGetDirect ma con il controllo del Time Out
// iTimeOutSec secondi di attesa
// ritorna
// 0= Tutto ok
// -2= IL server non esiste
// -3= non posso creare il socket
// -4= non posso creare la connessione con l'host
// -5= page error / page not found
// -100 = errore in creazione del thread
// -200 = Time out
// -300 = Out of memory
//
INT FWebGetDirectMT(FWEBSERVER *psWS,
					 CHAR *lpCmd, // GET/HEAD
					 void * (*funcNotify)(INT cmd,LONG Info,CHAR *),
					 INT iTimeOutSec,
					 BOOL fOsEventAnalisys) 
{
	HANDLE hURLThread;
	DWORD dwURLThread;

	INT start,finish;
	INT iSec; 
	BOOL fStop=FALSE;
	EH_URLGET sUGM;

	// A) Presetto le variabili

	WebStartup();

	ZeroFill(sUGM);
	psWS->iTimeoutSec=iTimeOutSec; 
//	sUGM.=funcNotify;
	sUGM.psWS=psWS;
	sUGM.psWS->SubControl=funcNotify;
	sUGM.lpCmd=lpCmd;
	sUGM.enWebError=0; 
	sUGM.fRun=TRUE;

	// B) Lancio l'acquisizione della pagina
	hURLThread=CreateThread(NULL,0,_UrlGetThread,&sUGM,0,&dwURLThread); 
	if (!hURLThread) return -100;
	start = clock();
	
	// C) Attendo per iTimeOutSec
	iSec=0;
	while (TRUE)
	{
		finish = clock();
		if (!sUGM.fRun) fStop=TRUE; 
		if (fStop) break; // Finito

		if (fOsEventAnalisys) 
			WindowsMessageDispatch();
			else
			// Attendo 200ms e controllo
			Sleep(200);

		finish = clock();
		if (finish!=start) iSec=(INT) (finish - start) / CLOCKS_PER_SEC;
		if (iSec>(iTimeOutSec+5)) break; // Time Out // Non dovrebbe arrivare qui
	}

	if (sUGM.fRun) // errore di Time out
		{
			TerminateThread(hURLThread,1);
	//		sUGM.enWebError=WTE_TIMEOUT_FORCED;
			psWS->iWebGetError=WTE_TIMEOUT_FORCED;
			_WebGetFreeResource(psWS);
		}
		else
		{
			if (WaitForSingleObject(hURLThread,1000)==WAIT_TIMEOUT) 
				{
					TerminateThread(hURLThread,1); 
				}
		}
		
	CloseHandle(hURLThread); hURLThread=0;
	sUGM.enWebError=psWS->iWebGetError;
	return sUGM.enWebError;
}

//
// ehWebError()
//
CHAR *ehWebError(INT iWebError)
{
	static CHAR szServ[200];
	BYTE *p;
	switch (iWebError)
	{
		case WTE_OK: p="WTE_OK| Ok."; break;
		case WTE_RECV: p="WTE_RECV| recv() socket error"; break;
		case WTE_SSL_RECV: p="WTE_SSL_RECV| SSL_read() SSL get error (Details in log)"; break;
		case WTE_SOCKET_CREATE: p="WTE_SOCKET_CREATE| socket(): errore in creazione socket"; break;
		case WTE_SSL_SEND_ERROR: p="WTE_SSL_SEND_ERROR| SSL Send Error"; break;
		case WTE_SSL_AUTH_ERROR: p="WTE_SSL_AUTH_ERROR| SSL error:SSL authentication failed in tcp_connect(): check password, key file, and ca file."; break;
		case WTE_SSL_NEW_ERROR: p="WTE_SSL_NEW_ERROR| SSL error: Errore in creazione nuova sezione."; break;
		case WTE_SSL_CERT_ERROR_1: p="WTE_SSL_CERT_ERROR_1| SSL error"; break;
		case WTE_SSL_CERT_ERROR_2: p="WTE_SSL_CERT_ERROR_2| SSL error"; break;
		case WTE_SSL_CERT_ERROR_3: p="WTE_SSL_CERT_ERROR_3| SSL error"; break;
		case WTE_SOCKET_SELECT_2: p="WTE_SOCKET_SELECT_2| select()"; break;
		case WTE_SOCKET_SELECT_3: p="WTE_SOCKET_SELECT_3| select()"; break;
		case WTE_SOCKET_CONNECT: p="WTE_SOCKET_SELECT| connect()"; break;
		case WTE_GET_HOST: p="WTE_GET_HOST| Non trovo host, controllare DNS"; break;
		case WTE_SETSOCKOPT: p="WTE_SETSOCKOPT| ?"; break;
		case WTE_HTTP_HEADER_NOT_200: p="WTE_HTTP_HEADER_NOT_200| L'status header != 200"; break;
		case WTE_HEADER_ERROR: p="WTE_HEADER_ERROR| L'header errato"; break;
		case WTE_CONNECTION_CLOSE: p="WTE_CONNECTION_CLOSE| Connessione interrotta."; break;
		case WTE_BLOCK_TOO_BIG: p="WTE_BLOCK_TOO_BIG| Chunk > 128k"; break;

		case WTE_SSL_TIMEOUT: p="WTE_SSL_TIMEOUT| SSL error: Timeout."; break;
		case WTE_TIMEOUT_CONNECT: p="WTE_TIMEOUT_CONNECT| tcp_connect()"; break;
		case WTE_TIMEOUT_FORCED: p="WTE_TIMEOUT_FORCED| Timeout forzato (non da socket)"; break;

		case WTE_OUTOFMEMORY: p="WTE_OUTOFMEMORY| ehAlloc() not resource!"; break;
		default: 
			sprintf(szServ,"ehWebError() ? %d", iWebError);
			p=szServ;
			break;
	}
	return p;
}

/*
		case 0: return "Ok.";
		case -2: return "gethostbyname()";
		case -3: return "socket";
		case -4: return "address";
		case -5: return "page error";
		case -6: return "SocketSend(): error";
		case -100: return "Thread error";
		case -200: return "timeout";
	}
	return "?";
	*/


//
// DA STUDIARE
//

/*
   u_long nonblocking = 1;
    ioctlsocket(fd, FIONBIO, &nonblocking); // ???

#ifdef WIN32

	#ifndef UNDER_CE
  SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, 0);
#endif
#else
  fcntl(fd, F_SETFD, 1);
#endif


#endif

	VEDI http://telemat.die.unifi.it/book/1997/winsock_prg/wsk_doc3.html


  if (psWS->connect_flags & SO_LINGER) ?? 


  if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&len, sizeof(int)))
  { psWS->iSocketError = wt_socket_errno;
    soap_set_sender_error(soap, wt_tcp_error(psWS), "setsockopt SO_SNDBUF failed in tcp_connect()", WT_TCP_ERROR);
    closesocket(fd);
    return _INVALID_SOCKET;
  }
  if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&len, sizeof(int)))
  { psWS->iSocketError = wt_socket_errno;
    soap_set_sender_error(soap, wt_tcp_error(psWS), "setsockopt SO_RCVBUF failed in tcp_connect()", WT_TCP_ERROR);
    closesocket(fd);
    return _INVALID_SOCKET;

#ifdef TCP_NODELAY
  if (!(psWS->omode & WT_IO_UDP) && setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&set, sizeof(int)))
  { psWS->iSocketError = wt_socket_errno;
    soap_set_sender_error(soap, wt_tcp_error(psWS), "setsockopt TCP_NODELAY failed in tcp_connect()", WT_TCP_ERROR);
    closesocket(fd);
    return _INVALID_SOCKET;
  }
#endif


      if (psWS->connect_timeout && (wt_socket_errno == WT_EINPROGRESS || wt_socket_errno == WT_EWOULDBLOCK))
      { struct timeval timeout;
        WT_SOCKLEN_T k;
        fd_set fds;
        if (psWS->connect_timeout > 0)
        { timeout.tv_sec = psWS->connect_timeout;
          timeout.tv_usec = 0;
        }
        else
        { timeout.tv_sec = -psWS->connect_timeout/1000000;
          timeout.tv_usec = -psWS->connect_timeout%1000000;
        }
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        while(TRUE)
        { int r = select((fd + 1), NULL, &fds, NULL, &timeout);
          if (r > 0)
	    break;
          if (!r)
          { psWS->iSocketError = 0;
            DBGLOG(TEST, WT_MESSAGE(fdebug, "Connect timeout\n"));
            soap_set_sender_error(soap, "Timeout", "connect failed in tcp_connect()", WT_TCP_ERROR);
            closesocket(fd);
            return _INVALID_SOCKET;
          }
          if (wt_socket_errno != WT_EINTR)
          { psWS->iSocketError = wt_socket_errno;
            DBGLOG(TEST, WT_MESSAGE(fdebug, "Could not connect to host\n"));
            soap_set_sender_error(soap, wt_tcp_error(psWS), "connect failed in tcp_connect()", WT_TCP_ERROR);
            closesocket(fd);
            return _INVALID_SOCKET;
          }
        }


  { u_long blocking = 0;
    ioctlsocket(fd, FIONBIO, &blocking);
  }
	*/

//
// OPEN_SSL
//
#ifdef WITH_OPENSSL 

//
// wt_ssl_init()
// Inizializza la gestione del SSL
//

static void wt_ssl_init(void)
{ 
	static int done = 0;
	if (!done)
	{ 
		done = 1;
		SSL_library_init();
		SSL_load_error_strings();

		if (!RAND_load_file("/dev/urandom", 1024))
		{ 
			char buf[1024];
			RAND_seed(buf, sizeof(buf));
			while (!RAND_status()) { int r = rand(); RAND_seed(&r, sizeof(int)); }
		}
	}
}
/*
  psWS->fsslauth = wt_ssl_auth_init;
  psWS->fsslverify = ssl_verify_callback;
  */

int eh_ssl_server_context(FWEBSERVER *psWS, 
							unsigned short flags, 
							const char *keyfile, 
							const char *password, 
							const char *cafile, 
							const char *capath, 
							const char *dhfile, 
							const char *randfile, 
							const char *sid)
{ int err;
  psWS->keyfile = keyfile;
  psWS->password = password;
  psWS->cafile = cafile;
  psWS->capath = capath;
  if (dhfile)
  { psWS->dhfile = dhfile;
    psWS->rsa = 0;
  }
  else
  { psWS->dhfile = NULL;
    psWS->rsa = 1;
  }
  psWS->randfile = randfile;
  psWS->require_client_auth = (flags & _SSL_REQUIRE_CLIENT_AUTHENTICATION);
  if (!(err = wt_ssl_auth_init(psWS)))
  if (sid) SSL_CTX_set_session_id_context(psWS->ctx, (unsigned char*)sid, strlen(sid));
  return err; 
}

//
// eh_ssl_client_context()
// Funzione di settaggio informazioni per autentificazione SSL
//
INT eh_ssl_client_context(FWEBSERVER *psWS, unsigned short flags, const char *keyfile, const char *password, const char *cafile, const char *capath, const char *randfile)
{ psWS->keyfile = keyfile;
  psWS->password = password;
  psWS->cafile = cafile;
  psWS->capath = capath;
  psWS->dhfile = NULL;
  psWS->rsa = 0;
  psWS->randfile = randfile;
  psWS->require_server_auth = (flags & _SSL_REQUIRE_SERVER_AUTHENTICATION);
  return 0;
}

//
// ssl_error()
//
static const char * ssl_error(FWEBSERVER *psWS, int ret)
{ 
	int err = SSL_get_error(psWS->ssl, ret);
	// DA CAPIRE

//	const char *msg = soap_str_code(h_ssl_error_codes, err);
//	if (msg)
//	strcpy(psWS->msgbuf, msg);
//	else
//	return ERR_error_string(err, psWS->msgbuf);
/*
	if (ERR_peek_error())
	{ 
		unsigned long r;
		strcat(psWS->msgbuf, "\n");
		while ((r = ERR_get_error()))
		ERR_error_string_n(r, psWS->msgbuf + strlen(psWS->msgbuf), sizeof(psWS->msgbuf) - strlen(psWS->msgbuf));
	} 
	else
	{ switch (ret)
	{ case 0:
		strcpy(psWS->msgbuf, "EOF was observed that violates the protocol. The client probably provided invalid authentication information.");
		break;
	  case -1:
		sprintf(psWS->msgbuf, "Error observed by underlying BIO: %s", strerror(errno));  
		break;
	}
	}
	return psWS->msgbuf;
	*/
	return NULL;
}

static int ssl_password(char *buf, int num, int rwflag, void *userdata)
{ 
	if (num < (int)strlen((char*)userdata) + 1) return 0;
	return strlen(strcpy(buf, (char*)userdata));
}
/*
static DH * ssl_tmp_dh(SSL *ssl, int is_export, int keylength)
{ static DH *dh512 = NULL;
  static DH *dh1024 = NULL;
  DH *dh;
  switch (keylength)
  { case 512:
      if (!dh512)
      { BIO *bio = BIO_new_file("dh512.pem", "r");
        if (bio)
        { dh512 = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
          BIO_free(bio);
          return dh512;
        }
      }
      else
        return dh512;
    default:
      if (!dh1024)
      { BIO *bio = BIO_new_file("dh1024.pem", "r");
        if (bio)
        { dh1024 = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
          BIO_free(bio);
        }
      }
      dh = dh1024;
  }
  return dh;
}
*/

//
// Inizializza l'autentificazione
//
static int wt_ssl_auth_init(FWEBSERVER *psWS)
{ 
	wt_ssl_init();
	if (!psWS->ctx)
	{ 
		if (!(psWS->ctx = SSL_CTX_new(SSLv23_method()))) 
		{
			ehLogWrite("SSL error" CRLF "Can't setup context");
			return -1;
		}
	}

	if (psWS->randfile)
	{ 
		if (!RAND_load_file(psWS->randfile, -1))
		{
			ehLogWrite("SSL error" CRLF "Can't load randomness");
			return -2;
		}
	}

	if (psWS->cafile || psWS->capath)
	{
		if (!SSL_CTX_load_verify_locations(psWS->ctx, psWS->cafile, psWS->capath))
		{
			ehLogWrite("SSL error" CRLF "Can't read CA file and directory");
			return -3;
		}
	}

	if (!SSL_CTX_set_default_verify_paths(psWS->ctx))
	{
		ehLogWrite("SSL error" CRLF "Can't read default CA file and/or directory");
		return -4;
	}

	if (psWS->keyfile)
	{ 
		if (!SSL_CTX_use_certificate_chain_file(psWS->ctx, psWS->keyfile))
		{
			ehLogWrite("SSL error" CRLF "Can't read certificate key file");
			return -5;
		}

		if (psWS->password)
		{ 
			SSL_CTX_set_default_passwd_cb_userdata(psWS->ctx, (void*) psWS->password);
			SSL_CTX_set_default_passwd_cb(psWS->ctx, ssl_password);
		}

		if (!SSL_CTX_use_PrivateKey_file(psWS->ctx, psWS->keyfile, SSL_FILETYPE_PEM))
		{
			ehLogWrite("SSL error" CRLF "Can't read key file");
			return -5;
		}
	}

/* Suggested alternative approach to check cafile first before the key file:
  if (psWS->password)
  { SSL_CTX_set_default_passwd_cb_userdata(psWS->ctx, (void*)psWS->password);
    SSL_CTX_set_default_passwd_cb(psWS->ctx, ssl_password);
  }
  if (!psWS->cafile || !SSL_CTX_use_certificate_chain_file(psWS->ctx, psWS->cafile))
  { if (psWS->keyfile)
    { if (!SSL_CTX_use_certificate_chain_file(psWS->ctx, psWS->keyfile))
        ehLogWrite("SSL error" CRLF "Can't read certificate or key file");
      if (!SSL_CTX_use_PrivateKey_file(psWS->ctx, psWS->keyfile, SSL_FILETYPE_PEM))
        ehLogWrite("SSL error" CRLF "Can't read key file");
    }
  }
*/

	if (psWS->rsa) // Credo CHECK del certificato
  { 
	  RSA *rsa = RSA_generate_key(512, RSA_F4, NULL, NULL);
      if (!SSL_CTX_set_tmp_rsa(psWS->ctx, rsa))
      {
			if (rsa) RSA_free(rsa);
			ehLogWrite("SSL error" CRLF "Can't set RSA key");
			return -6;
	  }
	  RSA_free(rsa);
  }
  else if (psWS->dhfile)
  { 
		DH *dh = 0;
		BIO *bio;
		bio = BIO_new_file(psWS->dhfile, "r");
		if (!bio) { ehLogWrite("SSL error" CRLF "Can't read DH file"); return -7;}
		dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		BIO_free(bio);
		if (SSL_CTX_set_tmp_dh(psWS->ctx, dh) < 0)
		{ 
			if (dh) DH_free(dh);
			ehLogWrite("SSL error" CRLF "Can't set DH parameters");
			return -8;
		}
		DH_free(dh);
  }
  SSL_CTX_set_options(psWS->ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2);
  SSL_CTX_set_verify(psWS->ctx, psWS->require_client_auth ? (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT) : psWS->require_server_auth ? SSL_VERIFY_PEER : SSL_VERIFY_NONE, ssl_verify_callback);

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
  SSL_CTX_set_verify_depth(psWS->ctx, 1); 
#else
  SSL_CTX_set_verify_depth(psWS->ctx, 9); 
#endif  

  return 0;
}


static int ssl_verify_callback(int ok, X509_STORE_CTX *store)
{
	/*
#ifdef WT_DEBUG
  if (!ok) 
  { char data[256];
    X509 *cert = X509_STORE_CTX_get_current_cert(store);
    fprintf(stderr, "SSL verify error or warning with certificate at depth %d: %s\n", X509_STORE_CTX_get_error_depth(store), X509_verify_cert_error_string(X509_STORE_CTX_get_error(store)));
    X509_NAME_oneline(X509_get_issuer_name(cert), data, sizeof(data));
    fprintf(stderr, "certificate issuer %s\n", data);
    X509_NAME_oneline(X509_get_subject_name(cert), data, sizeof(data));
    fprintf(stderr, "certificate subject %s\n", data);
  }
#endif
  */
  return ok;
}
/*
int eh_ssl_accept(FWEBSERVER *psWS)
{ 
	BIO *bio;
	int i, r;
	if (!wt_valid_socket(psWS->iSocket)) {ehLogWrite("SSL error" CRLF "No socket in soap_ssl_accept()"); return -1;}
//	if (!psWS->ctx && (psWS->error = wt_ssl_auth_init(pWS))) return _INVALID_SOCKET;
	if (!psWS->ssl)
	{	
		psWS->ssl = SSL_new(psWS->ctx);
		if (!psWS->ssl) { ehLogWrite("SSL error" CRLF "SSL_new() failed in soap_ssl_accept()"); return -2;}
	}
	else SSL_clear(psWS->ssl);

	// Contrassegno che sono in https (ma credo di averlo già fatto)
	psWS->imode |= WT_ENC_SSL;
	psWS->omode |= WT_ENC_SSL;
	#ifdef WIN32
	{ 
		u_long nonblocking = 1;
		ioctlsocket(psWS->iSocket, FIONBIO, &nonblocking);
	}
	#else
	fcntl(psWS->iSocket, F_SETFL, fcntl(psWS->iSocket, F_GETFL)|O_NONBLOCK);
	#endif

	bio = BIO_new_socket(psWS->iSocket, BIO_NOCLOSE);
	SSL_set_bio(psWS->ssl, bio, bio);
	i = 100; 
	while ((r = SSL_accept(psWS->ssl)) <= 0)
	{ 
		int err = SSL_get_error(psWS->ssl, r);
		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
		{ struct timeval timeout;
		  fd_set fd;
		  if (i-- <= 0)
			break;
		  timeout.tv_sec = 0;
		  timeout.tv_usec = 100000;
		  FD_ZERO(&fd);
		  FD_SET(psWS->iSocket, &fd);
		  r = select((psWS->iSocket + 1), &fd, NULL, &fd, &timeout);
		  if (r < 0 && WSAGetLastError() != WSAEINTR)
		  { 
			  psWS->iErrorCode=WTE_SOCKET_SELECT;
			  psWS->iSocketErr = WSAGetLastError();
			return -1;
		  }
		}
		else
		{ psWS->iSocketErr = err;
		  break;
		}
	}

#ifdef WIN32
  { u_long blocking = 0;
    ioctlsocket(psWS->iSocket, FIONBIO, &blocking);
  }
#else
  fcntl(psWS->iSocket, F_SETFL, fcntl(psWS->iSocket, F_GETFL)&~O_NONBLOCK);
#endif

  if (r <= 0)
  { //soap_set_receiver_error(soap, ssl_error(soap, r), "SSL_accept() failed in soap_ssl_accept()");
	  ehLogWrite("SSL_accept() failed in soap_ssl_accept() : %s",ssl_error(psWS, r));
	  return -1;
    //soap_closesock(soap);
    //return WT_SSL_ERROR;
  }

  if (psWS->require_client_auth)
  { 
	X509 *peer;
	int err;
    if ((err = SSL_get_verify_result(psWS->ssl)) != X509_V_OK)
    { //soap_closesock(soap);
      //return soap_set_sender_error(soap, X509_verify_cert_error_string(err), "SSL certificate presented by peer cannot be verified in soap_ssl_accept()");
		ehLogWrite("SSL certificate presented by peer cannot be verified in soap_ssl_accept(): %s",X509_verify_cert_error_string(err));
		return -1;
    }
    peer = SSL_get_peer_certificate(psWS->ssl);
    if (!peer)
    { //soap_closesock(soap);
      //return soap_set_sender_error(soap, "SSL error" CRLF "No SSL certificate was presented by the peer in soap_ssl_accept()");
		ehLogWrite("SSL error" CRLF "No SSL certificate was presented by the peer in soap_ssl_accept()");
		return -1;
    }
    X509_free(peer);
  }
  return 0;
}
*/
#endif

/*
	// Da aggiungere alla struttura WS per avviare una connessione protetta




psWS->iSocket = fd;
  psWS->imode &= ~_ENC_SSL;
  psWS->omode &= ~_ENC_SSL;
  if (!strncmp(endpoint, "https:", 6))
  {

  // -----------------------------------------------------
    BIO *bio;
    int r;

	psWS->ssl = SSL_new(psWS->ctx);
    if (!psWS->ssl)
    { psWS->error = WT_SSL_ERROR;
      return _INVALID_SOCKET;
    }


  // -----------------------------------------------------
    if (psWS->session)
    { if (!strcmp(psWS->session_host, host) && psWS->session_port == port)
        SSL_set_session(psWS->ssl, psWS->session);
      SSL_SESSION_free(psWS->session);
      psWS->session = NULL;
    }
    psWS->imode |= _ENC_SSL;
    psWS->omode |= _ENC_SSL;

  // -----------------------------------------------------
    bio = BIO_new_socket( fd, BIO_NOCLOSE);
    SSL_set_bio(psWS->ssl, bio, bio);
#ifndef WITH_LEAN
    if (psWS->connect_timeout)
#ifdef WIN32
    { u_long nonblocking = 1;
      ioctlsocket(fd, FIONBIO, &nonblocking);
    }
#else
      fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
#endif
#endif

    while(TRUE)
    { if ((r = SSL_connect(psWS->ssl)) <= 0)
      { int err = SSL_get_error(psWS->ssl, r);
        if (err != SSL_ERROR_NONE && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
        { soap_set_sender_error(soap, ssl_error(soap, r), "SSL connect failed in tcp_connect()");
          return _INVALID_SOCKET;
        }
        if (psWS->connect_timeout)
        { struct timeval timeout;
          fd_set fds;
          if (psWS->connect_timeout > 0)
          { timeout.tv_sec = psWS->connect_timeout;
            timeout.tv_usec = 0;
          }
          else
          { timeout.tv_sec = -psWS->connect_timeout/1000000;
            timeout.tv_usec = -psWS->connect_timeout%1000000;
          }
          FD_ZERO(&fds);
          FD_SET(psWS->iSocket, &fds);
          while(TRUE)
          { int r = select((psWS->iSocket + 1), &fds, NULL, &fds, &timeout);
            if (r > 0)
	      break;
            if (!r)
            { psWS->iSocketError = 0;
              DBGLOG(TEST, WT_MESSAGE(fdebug, "Connect timeout\n"));
              soap_set_sender_error(soap, "Timeout", "connect failed in tcp_connect()", WT_TCP_ERROR);
              return _INVALID_SOCKET;
            }
          }
	  continue;
        }
      }
    }
#ifndef WITH_LEAN
    if (psWS->connect_timeout)
#ifdef WIN32
    { u_long blocking = 0;
      ioctlsocket(fd, FIONBIO, &blocking);
    }
#else
      fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);
#endif
#endif
       break;
  // -----------------------------------------------------
	 Se è richiesto un certificato
  // -----------------------------------------------------
    if (psWS->require_server_auth)
    { X509 *peer;
      int err;
      if ((err = SSL_get_verify_result(psWS->ssl)) != X509_V_OK)
      { soap_set_sender_error(soap, X509_verify_cert_error_string(err), "SSL certificate presented by peer cannot be verified in tcp_connect()");
        return _INVALID_SOCKET;
      }
      peer = SSL_get_peer_certificate(psWS->ssl);
      if (!peer)
      { soap_set_sender_error(soap, "SSL error" CRLF "No SSL certificate was presented by the peer in tcp_connect()");
        return _INVALID_SOCKET;
      }
      X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, psWS->msgbuf, sizeof(psWS->msgbuf));
      X509_free(peer);
      if (soap_tag_cmp(psWS->msgbuf, host))
      { soap_set_sender_error(soap, "SSL error" CRLF "SSL certificate host name mismatch in tcp_connect()");
        return _INVALID_SOCKET;
      }
    }

  */

void web_cookie_create(_DMI *pdmiCookie) {DMIOpen(pdmiCookie,RAM_AUTO,20,sizeof(EH_COOKIE),"Cookies");}
void web_cookie_destroy(_DMI *pdmiCookie) 
{
	web_cookie_clean(pdmiCookie);
	DMIClose(pdmiCookie,"Cookies");
}

void web_cookie_free(EH_COOKIE *psCookie)
{
	ehFreePtr(&psCookie->pName);
	ehFreePtr(&psCookie->pValue);
	ehFreePtr(&psCookie->pPath);
	ehFreePtr(&psCookie->pExpires);
}

void web_cookie_clean(_DMI *pdmiCookie)
{
	INT a;
	EH_COOKIE sCookie;
	for (a=0;a<pdmiCookie->Num;a++)
	{
		DMIRead(pdmiCookie,a,&sCookie);
		web_cookie_free(&sCookie);
	}
	pdmiCookie->Num=0;
}

void web_cookie_get(_DMI *pdmiCookie,CHAR *pPageHeader)
{
	INT a;
	EH_AR arHeader;
	arHeader=ARCreate(pPageHeader,CRLF,NULL);
	//printf("%s",pPageHeader);
	for (a=0;arHeader[a];a++)
	{
		if (!strCaseBegin(arHeader[a],"Set-Cookie: ")) 
		{
			web_cookie_add(pdmiCookie,arHeader[a]+12);
		}
	}
	ARDestroy(arHeader);
}

CHAR *web_cookie_string(_DMI *pdmiCookie)
{
	CHAR szServ[1024];
	INT a,x,iSizeMemory=0;
	BYTE *pReturn=NULL;
	EH_COOKIE sCookie;

	if (pdmiCookie->Hdl<0) return strDup("");
	for (x=0;x<2;x++)
	{
		if (x) pReturn=ehAllocZero(iSizeMemory);
	
		for (a=0;a<(INT) pdmiCookie->Num;a++)
		{
			DMIRead(pdmiCookie,a,&sCookie);
			sprintf(szServ,"%s=%s",sCookie.pName,sCookie.pValue);
			if (sCookie.pExpires) sprintf(szServ+strlen(szServ),"; expires=%s",sCookie.pExpires);
			if (sCookie.pPath) sprintf(szServ+strlen(szServ),"; path=%s",sCookie.pPath);
			switch (x)
			{
				case 0:	
					iSizeMemory+=(strlen(szServ)+20);
					break;
				case 1:
					sprintf(pReturn+strlen(pReturn),"Cookie: %s" CRLF,szServ);
					break;
			}
		}
	}
	return pReturn;
}


//
// web_cookie_add()
//
void web_cookie_add(_DMI *pdmiCookie,CHAR *pSetCookie)
{
	INT a,idx=-1;
	INT iParam;
	CHAR *pName;
	EH_COOKIE sCookie,sCookie2;

	EH_AR arRow,arPart;

	//
	// Estraggo le parti del cookie
	//
	arRow=ARCreate(pSetCookie,";",&iParam);
	arPart=ARCreate(arRow[0],"=",NULL);
	pName=strTrim(arPart[0]);
	ZeroFill(sCookie);
	sCookie.pName=strDup(pName);
	sCookie.pValue=strDup(arPart[1]);
	ARDestroy(arPart);

	for (a=1;a<iParam;a++)
	{
		arPart=ARCreate(arRow[a],"=",NULL);
		pName=strTrim(arPart[0]);
		if (!strcmp(pName,"path"))  strAssign(&sCookie.pPath,arPart[1]);
		if (!strcmp(pName,"expires"))  strAssign(&sCookie.pExpires,arPart[1]);
		ARDestroy(arPart);
	}
	ARDestroy(arRow);

	//
	// Cerco se ho già il coockie
	//
	for (a=0;a<pdmiCookie->Num;a++)
	{
		DMIRead(pdmiCookie,a,&sCookie2);
		if (!strcmp(sCookie2.pName,sCookie.pName)) {idx=a; break;}
	}

	if (idx>-1) 
	{
		DMIRead(pdmiCookie,idx,&sCookie2); // Libero il vecchio
		web_cookie_free(&sCookie2);
		DMIWrite(pdmiCookie,idx,&sCookie); // Scrivo il nuovo
	}
	else
	{
		DMIAppendDyn(pdmiCookie,&sCookie); // Scrivo il nuovo
	}
}



//
//	UrlToFile()
//	Scrive il contenuto di un URL su disco
//  Ritorna TRUE se l'ha registrato (tutto ok) ,altrimenti FALSE se c'è errore
//
BOOL UrlToFile(CHAR *lpUrl,
			   BYTE *pszFileSaveUtf,
			   BOOL fFailExist,		  // T/F fallisce se esiste
			   BOOL fAsciiEqualControl, // T/F se il file su disco è uguale a quello letto non sovrascrivo  // serve ad esempio per limitare i trasferimenti in FTP
			   BOOL bHtmlClean,		// T/F se devo togliere ritorni a capo e spazi
			   _DMI *pdmiCookie,
			   CHAR *pUser,
			   CHAR *pPassword) 
										
{
	FWEBSERVER sWS;
	BOOL fCheck;
	clock_t start,finish;
	double dSec;
	BOOL fBuild=FALSE;
	BOOL bTheFileExist=FALSE;

	WebStartup();

	//
	// Se devo fallire se esiste ed esistono tutte e due ritorno subito
	//
	if (pszFileSaveUtf)
	{
		/*
		if (bFileNameUnicode)
		{
			BYTE *psz=utfToWcs(pFileNameSave);
			
			if (fileCheck(psz)) bTheFileExist=TRUE;
			ehFree(psz);
		}
		else
		{
			if (fileCheck((CHAR *) pFileNameSave)) bTheFileExist=TRUE;
		}
		*/
		if (fileCheck(pszFileSaveUtf)) bTheFileExist=TRUE;
		if (bTheFileExist&&fFailExist) return fBuild;
	}

	_(sWS);
	sWS.lpUri=lpUrl;
	sWS.keep_alive=true;
	sWS.pdmiCookie=pdmiCookie;
	sWS.lpUser=pUser;
	sWS.lpPass=pPassword;

	start=clock();

#ifdef EH_CONSOLE
	if (fAsciiEqualControl) printf("=");
	printf("< %s ..." CRLF,lpUrl);
#endif

	fCheck=FWebGetDirect(&sWS,"GET",NULL);
//	printf("%s-%d",sWS.pPageHeader,sWS.iDataReaded);
	if (fCheck)
	{
		printf(">webError (%d) %s" CRLF,sWS.iErrorCode,sWS.pWebGetError);//,sWS.pPageHeader,strlen(sWS.pPageHeader));
		ehLogWrite("%s:webError (%d) %s",lpUrl,sWS.iErrorCode,sWS.pWebGetError);
		FWebGetFree(&sWS);  
		return fBuild;
	}
	finish = clock();
	dSec=(double)(finish - start) / CLOCKS_PER_SEC;

#ifdef EH_CONSOLE
    printf("%.2fs",dSec);
#endif

	if (bHtmlClean) {
	
		CHAR *pSource;
		for (pSource=sWS.pData;*pSource;pSource++) {
			if (*pSource>' ') break;
		}
		memcpy(sWS.pData,pSource,strlen(pSource)); sWS.pData[strlen(pSource)]=0;
		while (strReplace(sWS.pData," \n" ,"\n"));
		while (strReplace(sWS.pData," \r" ,"\r"));
		while (strReplace(sWS.pData,"\t",""));
		while (strReplace(sWS.pData,"\n\n","\n"));
		while (strReplace(sWS.pData,"\r\r","\r"));
		while (strReplace(sWS.pData,CRLF CRLF,CRLF));
		while (strReplace(sWS.pData,">"CRLF,">"));
		sWS.iDataReaded=strlen(sWS.pData);
	}

	//
	// File normale
	//
	if (pszFileSaveUtf)
	{
		BOOL fSave=true;
		if (fAsciiEqualControl&&bTheFileExist) // Funziona solo con i file ASCII
		{
			BYTE *lpFC;
			//if (bFileNameUnicode) lpFC=fileStrReadW(pFileNameSave); else 
			lpFC=fileStrRead(pszFileSaveUtf);
			if (lpFC)
			{
				if (strlen(lpFC)==sWS.iDataReaded)
				{
					if (!memcmp(lpFC,sWS.pData,sWS.iDataReaded)) fSave=FALSE;
				}
				ehFree(lpFC);
			}
		}
		
		if (fSave)
		{
			if (!fFailExist||fFailExist&&!bTheFileExist) // Se devo sempre scriverlo o non esiste
			{
//				 HANDLE hFile;
//				 DWORD dwWrited;

				 #ifdef EH_CONSOLE
				 printf("> %s" CRLF,pszFileSaveUtf);
				 #endif

				 fileMemoWrite(pszFileSaveUtf,sWS.pData,sWS.iDataReaded);
				 fBuild=TRUE;
			}
		}
		else 
		{
#ifdef EH_CONSOLE
//		if (bFileNameUnicode) 
//			printf("(equal no save) %S" CRLF,pFileNameSave);
//			else
			printf("(equal no save) %s" CRLF,pszFileSaveUtf);
#endif
		}

	}

/*
				 if (bFileNameUnicode)
				 {
#ifdef EH_CONSOLE
					 {	
						BYTE *pUtf8=strEncodeW(pFileNameSave,SE_UTF8,NULL);
						printf("> %s" CRLF,pUtf8);
						ehFree(pUtf8);
					 }
#endif
				    hFile = CreateFileW(pFileNameSave,
										GENERIC_WRITE,
										FILE_SHARE_READ,
										NULL,
										CREATE_ALWAYS,
										0,
										(HANDLE)NULL);
				 }
				 else
				 {
#ifdef EH_CONSOLE
					printf("> %s" CRLF,pFileNameSave);
#endif
				    hFile = CreateFile(pFileNameSave,
										GENERIC_WRITE,
										FILE_SHARE_READ,
										NULL,
										CREATE_ALWAYS,
										0,
										(HANDLE)NULL);
				 }

				 if (hFile!=(HANDLE) INVALID_HANDLE_VALUE) 
				 {
					WriteFile(hFile,sWS.pData,sWS.iDataReaded,&dwWrited,NULL);
					CloseHandle(hFile);
					fBuild=TRUE;
				 }
			}
		} 
			*/

	FWebGetFree(&sWS);  
	return fBuild;
}


BOOL SFTPGet(CHAR *	lpUserName,        // Indicazione dell'utente 
			CHAR *	lpPassWord,
	        CHAR *	pszFtpServer, // Server da Aprire
			CHAR *	lpLocalFileDest,
	        CHAR *	lpRemoteFileSource,  // Pagina da richiedere
	        void *	(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS), // Funzione di controllo e display
	        BOOL	fFailIfExists, // TRUE/FALSE Visione dei passaggi di transazione
			INT *	lpiErrorFtp, 
			BOOL	bModoAttivo,  // TRUE se si vuole il modo attivo
			void *	pVoidParam) 
{
	
	CHAR *	pszBuffer;
//	CHAR szFolder[200];
	BOOL bRet=false;
	CHAR * psz;
	EH_LST	lst=lstNew();
	CHAR szFileScript[300];
	
	CHAR * pszScpFolder="c:\\comFerra\\WinSCP";
	CHAR * pszScpApp="winscp /script=";
	
	lstPush(lst,"option batch abort");
	lstPush(lst,"option confirm off");
	lstPush(lst,"open sftp://#[USER]#:#[PASSWORD]#@#[HOST]#/ -hostkey=\"ssh-rsa 2048 xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx\"");
	lstPush(lst,"option transfer binary");
	if (lpLocalFileDest)
		lstPush(lst,"get #[FILES_SOURCE]# #[FILES_DEST]#");
		else
		lstPush(lst,"ls #[FILES_SOURCE]#");
	lstPush(lst,"close");
	lstPush(lst,"exit");
	 
	psz=lstToString(lst,CRLF,"","");
	pszBuffer=ehAllocZero(32000);
	strcpy(pszBuffer,psz);
	ehFree(psz);
	lstDestroy(lst);

	strReplace(pszBuffer,"#[USER]#",lpUserName);
	strReplace(pszBuffer,"#[PASSWORD]#",lpPassWord);
	strReplace(pszBuffer,"#[HOST]#",pszFtpServer);

	//strcpy(szFolder,filePath(lpRemoteFileSource));
	strReplace(pszBuffer,"#[FILES_SOURCE]#",lpRemoteFileSource);
//	if (!lpLocalFileDest) lpLocalFileDest="c:\\comferra\\inprod\\SFTP.txt";
	fileRemove(lpLocalFileDest);
	strReplace(pszBuffer,"#[FILES_DEST]#",lpLocalFileDest);
	
	sprintf(szFileScript,"%s\\ftpScript.txt",pszScpFolder);
	fileStrWrite(szFileScript,pszBuffer);

	sprintf(pszBuffer,"%s\\%s%s",pszScpFolder,pszScpApp,szFileScript);
	system(pszBuffer);
//	printf("[%s]",pszBuffer);
	if (lpLocalFileDest) {
	
		if (!fileCheck(lpLocalFileDest)) bRet=true; // File non trovato
	}
	ehFree(pszBuffer);
	return bRet;
}


#else
#error Easyhand error. richiesto per usare webtool la macro EH_INTERNET
#endif