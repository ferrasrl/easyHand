// -------------------------------------------------------------
// ehSocket
// 
// 
// 
// -------------------------------------------------------------

#ifndef WITH_OPENSSL 
//#define WITH_OPENSSL
#endif

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehSocket.h"
#ifdef EH_INTERNET

#ifdef _WIN32_WCE
#include "connmgr.h"
// WindowCE:Attenzione Server cellcore.lib nel link
#endif

#define WT_INVALID_SOCKET (-1)
#define wt_valid_socket(n) ((n) != WT_INVALID_SOCKET)
#define wt_socket_errno WSAGetLastError()

//static int _tcpGetHost(S_EH_SOCKET *ws, const char *addr, struct in_addr *inaddr);
static int _tcpGetHost(S_EH_SOCKET *ws, CHAR * addr, struct in_addr *inaddr);
static int _tcpConnect(S_EH_SOCKET *psSock, CHAR * pszHostName, int port, BOOL bHttps);
static int ehTcpDisconnect(S_EH_SOCKET *psSock);
static BOOL enTcpSend(S_EH_SOCKET *psSock,S_SOCKET_EVENT *psEvent);
static int ehTcpShutdownSocket(int fd, int how);
//static BOOL ehTcpRead(S_EH_SOCKET *psSock);
static size_t ehTcpRecv(S_EH_SOCKET *psSock, char *s, size_t n);
static void _tcpBufRealloc(S_EH_SOCKET *psSock,DWORD dwNewSize);
static DWORD WINAPI _socketThreadRice(LPVOID pvoid);
static CHAR *	_ehSocketError(INT enError);
static void		_ehSetTimeout(struct timeval *psTimeout,INT iSec);
static void		_ehErrorEvent(S_EH_SOCKET *psSock, EN_HTTP_ERR enErr);
static BOOL		bInitialize=TRUE;

static void _errorSocket() {ehError();}
static void _errorHost() {ehError();}
#define _enterSocket(x) WaitForSingleObject(x->mtxSocket,INFINITE)
#define _leaveSocket(x) if (!ReleaseMutex(x->mtxSocket)) _errorSocket()

#define _enterHost() WaitForSingleObject(_sPrivate.mtxHost,INFINITE)
#define _leaveHost() if (!ReleaseMutex(_sPrivate.mtxHost)) _errorHost()


struct {

//	CRITICAL_SECTION csGetHost;
	BOOL	bReset;
	HANDLE	mtxHost;
	EH_AR arHost;

} _sPrivate={true};

//
// EN_HTTP_ERR ehSocket(S_EH_SOCKET *psSock,EN_MESSAGE iCmd,void *ptr);
//
static void _ehEventFree(S_SOCKET_EVENT *psEvent) {

	ehFreePtr(&psEvent->pData);
	psEvent->dwSize=0;

}

//
// ehSocket()
//
EN_HTTP_ERR ehSocket(S_EH_SOCKET * psSock,EN_MESSAGE enCmd,void *ptr)
{
	INT a;
	EN_HTTP_ERR iErr=0;
	S_SOCKET_EVENT sEvent;

	switch (enCmd)
	{
		//
		// Apro il socket
		//
		case WS_OPEN:
			
			if (!sys.bSocketReady) ehSocketInit();
			if (!*psSock->pszAddress) ehError();
			// InitializeCriticalSection(&psSock->csSocket);
			psSock->mtxSocket=CreateMutex(NULL,FALSE,NULL);
			
			//if (!psSock->iTimeoutSec) psSock->iTimeoutSec=10;
//			printf("[%s]",psSock->pszAddress);
			iErr=_tcpConnect(psSock,psSock->pszAddress,psSock->port,FALSE);
			if (iErr<0) {ehLogWrite("_tcpConnect() Error READ"); iErr=psSock->iErrorCode; break;}

			DMIReset(&psSock->dmiEvent);
			DMIOpen(&psSock->dmiEvent,RAM_AUTO,10,sizeof(S_SOCKET_EVENT),"SocketEvent");
			psSock->hThread=CreateThread(NULL,0,_socketThreadRice,psSock,0,&psSock->dwThread); 
			iErr=0;
			// Notifico
			if (psSock->funcNotify) (*psSock->funcNotify)(WS_OPEN,0,psSock);

			break;

		case WS_CLOSE:
			ehTcpDisconnect(psSock);
			if (psSock->bThreadRun)
			{
				psSock->bThreadStop=TRUE;
				if (WaitForSingleObject(psSock->hThread,1000)==WAIT_TIMEOUT) TerminateThread(psSock->hThread,1); 
			}
			psSock->bThreadRun=FALSE;

			// Libero i blocchi in memoria
			for (a=0;a<psSock->dmiEvent.Num;a++) {
				DMIRead(&psSock->dmiEvent,a,&sEvent);
				_ehEventFree(&sEvent);
			}
			if (psSock->funcNotify) (*psSock->funcNotify)(WS_CLOSE,0,psSock);
			DMIClose(&psSock->dmiEvent,"Event");
			ehFreePtr(&psSock->pbReaded);
//			DeleteCriticalSection(&psSock->csSocket);
			CloseHandle(psSock->mtxSocket); psSock->mtxSocket=NULL;
			
			memset(psSock,0,sizeof(S_EH_SOCKET));
			break;

		//
		// Send 
		//
		case WS_SEND:
			if (enTcpSend(psSock,(S_SOCKET_EVENT *) ptr)) 
			{
				iErr=psSock->iErrorCode; break;
			}
			iErr=0;
			break;

		//
		// GET - Legge un evento 
		//
		case WS_GET:
			
			WaitForSingleObject(psSock->mtxSocket,INFINITE); // EnterCriticalSection(&psSock->csSocket);
			iErr=1;
			if (psSock->dmiEvent.Num) 
			{
				DMIRead(&psSock->dmiEvent,0,ptr);
				DMIDelete(&psSock->dmiEvent,0,NULL);
				iErr=0;
			}
			ReleaseMutex(psSock->mtxSocket); // LeaveCriticalSection(&psSock->csSocket);
			break;


	}
	return iErr;
}

//
// Thread di Ricezione
//
static DWORD WINAPI _socketThreadRice(LPVOID pvoid)
{
	S_EH_SOCKET *psSock=pvoid;
	S_SOCKET_EVENT sEvent;
	#define RICE_SIZE_BUFFER 0xFFFFF
	BYTE  *pszBuffer;

	BOOL bHeaderError=FALSE;
	INT iReaded;

	ehFreePtr(&psSock->pbReaded);
	psSock->dwReadBufferSize=RICE_SIZE_BUFFER; // 8kb
	psSock->pbReaded=ehAlloc(psSock->dwReadBufferSize);
	psSock->dwReaded=0;

	bHeaderError=FALSE;
	psSock->bThreadRun=TRUE;
	pszBuffer=ehAlloc(RICE_SIZE_BUFFER);
	while (TRUE)
	{
		 if (psSock->bThreadStop) break;
		 iReaded=ehTcpRecv(psSock,pszBuffer,RICE_SIZE_BUFFER);

		 //
		 // Controllo degli errori
		 //
		 /*
#ifdef _DEBUG
		 for (a=0;a<iReaded;a++) ehPrintd("%c",pszBuffer[a]);
		 ehPrintd(CRLF);
#endif
		 */
		 if (!iReaded) 
		 {
			// bHeaderError=TRUE; 
			 if (!psSock->iErrorCode) psSock->iErrorCode=WTE_CONNECTION_CLOSE; 
			 // ehPrintd("-%s-" CRLF,_ehSocketError(psSock->iErrorCode));

			 //
			 // Genero evento di chiusura connessione / timeout da gestire
			 // Devo riaprire la connessione o riuscire ?
			 //
			 if (psSock->iErrorCode==WTE_CONNECTION_CLOSE&&
				 psSock->bAutoReconnect)
			 {
				//ehPrintd("AutoReconnect ...");
				if (_tcpConnect(psSock,psSock->pszAddress,psSock->port,FALSE)<0) 
					{
					//	ehPrintd("autoReconnect() Error"); 
						psSock->iErrorCode=psSock->iErrorCode; 
						break;
					}
			 } else 
			 {
				 _ehErrorEvent(psSock,psSock->iErrorCode);
				 break;
			 }
		 } // Definisco che un header non può essere più grande di 2048

		 //		 if (!iB) break;
		 if (iReaded<0) 
		 {
			 if (!psSock->iErrorCode) psSock->iErrorCode=WTE_RECV;
			 psSock->iSocketErr=WSAGetLastError();
			 //ehPrintd("-%s-" CRLF,_ehSocketError(psSock->iErrorCode));
			 _ehErrorEvent(psSock,psSock->iErrorCode);

			 //
			 // Genero evento di chiusura connessione / timeout da gestire
			 // Devo riaprire la connessione o riuscire ?
			 //
			 break;
		 }

		 //
		 // Memorizzo il dato nel buffer
		 //
		 _enterSocket(psSock); //EnterCriticalSection(&psSock->csSocket);
		 if (!psSock->bReadEvent)
		 {
			if ((psSock->dwReaded+iReaded)>=psSock->dwReadBufferSize) _tcpBufRealloc(psSock,psSock->dwReadBufferSize+iReaded+8192);
			memcpy(psSock->pbReaded+psSock->dwReaded,pszBuffer,iReaded);
			psSock->dwReaded+=iReaded;
		 }
		 else
		 {
			_(sEvent);
			sEvent.dwSize=iReaded;
			sEvent.pData=ehAllocZero(iReaded+1);
			memcpy(sEvent.pData,pszBuffer,iReaded);
			DMIAppendDyn(&psSock->dmiEvent,&sEvent);
			// ehPrintd("-> %d (%d)" CRLF,psSock->dmiBlock.Num,iReaded);
		 }
		 _leaveSocket(psSock); // LeaveCriticalSection(&psSock->csSocket);

		 // Notifico
		 if (psSock->funcNotify) (*psSock->funcNotify)(WS_EVENT,0,psSock);
 
	}
	psSock->bThreadRun=FALSE;
	ehFree(pszBuffer);
	return FALSE;
}
//
// enTcpSend()
// Send multi protocollo (ritorna TRUE se ci sono errori
//

static BOOL enTcpSend(S_EH_SOCKET *psSock,S_SOCKET_EVENT *psEvent)
{
	INT iWritten,iError;
	BYTE *pBuf=psEvent->pData;
	INT iSize=psEvent->dwSize;
	INT iToken;

	while (iSize)
	{

	#ifdef WITH_OPENSSL
		if (psSock->ssl)
		   iWritten = SSL_write(psSock->ssl, pBuf, iSize);
		else if (psSock->bio)
		   iWritten = BIO_write(psSock->bio, pBuf, iSize);
		else
	#endif
		iToken=iSize; if (iToken>2048) iToken=2048;
		iWritten = send(psSock->sSocket.fdSocket, pBuf, iToken, 0);//psSock->iSocket_flags);

	//	iErr=send(->iSocket,lpBuf,iSize,0);
		// dispx("%d (%d) ....",iSize,iWritten);

		if (iWritten <= 0) {

	#ifdef WITH_OPENSSL

			int err = SSL_get_error(psSock->ssl, iWritten);
			if (psSock->ssl && 
				err != SSL_ERROR_NONE && 
				err != SSL_ERROR_WANT_READ && 
				err != SSL_ERROR_WANT_WRITE) return TRUE;
	#endif

			iError=WSAGetLastError();
			if (iError != WSAEINTR && iError != WSAEWOULDBLOCK)
			{	psSock->iErrorCode=WTE_SSL_SEND_ERROR;
				psSock->iSocketErr= iError;
				ehLogWrite("Errore: %d [%s:%d]",iError,pBuf,iSize);
				return TRUE;
			}
			iWritten = 0; /* and call write() again */
		 }
		 pBuf+=iWritten;
		 iSize-=iWritten;
	}

	// Segnalo che il trasferimento è finito
#ifdef WITH_OPENSSL
  if (!psSock->ssl && wt_valid_socket(psSock->iSocket) && !psSock->keep_alive && !(psSock->omode & WT_IO_UDP))
    ehTcpShutdownSocket(psWS->iSocket, SD_SEND); 
#else
	if (wt_valid_socket(psSock->sSocket.fdSocket) && !psSock->keep_alive && !(psSock->omode & WT_IO_UDP))
	{
		if (ehTcpShutdownSocket( psSock->sSocket.fdSocket, SD_SEND)!=WT_OK) return TRUE;
//		closesocket(psSock->sSocketWrite.iSocket);
//		psSock->sSocketWrite.iSocket = WT_INVALID_SOCKET;
	}
#endif

	return FALSE;
	  
/*
	if (iWritten==SOCKET_ERROR)
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

static void _ehErrorEvent(S_EH_SOCKET *psSock, EN_HTTP_ERR enErr)
{
	S_SOCKET_EVENT sEvent;
	_enterSocket(psSock); // EnterCriticalSection(&psSock->csSocket);
	ZeroFill(sEvent);
	sEvent.enErr=enErr;
	sEvent.pData=strDup(_ehSocketError(enErr));
	sEvent.dwSize=strlen(sEvent.pData)+1;
	DMIAppendDyn(&psSock->dmiEvent,&sEvent);
	_leaveSocket(psSock); //LeaveCriticalSection(&psSock->csSocket);
    if (psSock->funcNotify) (*psSock->funcNotify)(WS_EVENT,0,psSock);
}

//
// ehTcpRecv()
//
static size_t ehTcpRecv(S_EH_SOCKET *psSock, char *s, size_t n)
{ 
	register int r;
	int iErr;
	psSock->iSocketErr = 0;
/*
	if (!psSock->iTimeoutRead)//recv_timeout) 
	{
		//if (!psSock->bHttps) 
		psSock->iTimeoutSec=10; // Proviamo 11/2009 <-########################
	}
*/
	if (wt_valid_socket(psSock->sSocket.fdSocket))
	{
		for (;;)
	    { 

#ifndef WITH_LEAN

			//
			// Gestione del Timeout
			//
#ifdef WITH_OPENSSL
			if (psSock->iTimeoutSec&&!psSock->ssl)
#else
			if (psSock->iTimeoutRead)
#endif
			{

				struct timeval timeout;
				fd_set fd;
				_ehSetTimeout(&timeout,psSock->iTimeoutRead);

				FD_ZERO(&fd);
				FD_SET(psSock->sSocket.fdSocket, &fd);

				r = select(sizeof(fd), &fd, NULL, NULL, &timeout); // (psSock->iSocket + 1)
				if (!r) { 
					psSock->iErrorCode=WTE_TIMEOUT;
					psSock->iSocketErr = 0; 
					return 0;
				}
				if (r < 0 && wt_socket_errno != WSAEINTR) 
					{	
						psSock->iErrorCode=WTE_TIMEOUT;//WTE_SOCKET_SELECT; 
						psSock->iSocketErr=wt_socket_errno; 
						return 0; 
					}
			  }

#endif

#ifdef WITH_OPENSSL
		  if (psSock->ssl)
		  {  
				int errcode;
				r = SSL_read(psSock->ssl, s, n);  // DA RIMETTERE n se no si blocca
				//if (!r) return 0;
				if (r >= 0) return (size_t) r;

				errcode = SSL_get_error(psSock->ssl, r);

				// if (errcode==SSL_ERROR_ZERO_RETURN||SSL_ERROR_SYSCALL) return 0;

				if (errcode != SSL_ERROR_NONE && 
					errcode != SSL_ERROR_WANT_READ && 
					errcode != SSL_ERROR_WANT_WRITE) 
				{
					int sslerr = ERR_get_error();
					CHAR *eptr=NULL;
					CHAR errbuf[2000],*errptr;
					
					psSock->iErrorCode = WTE_SSL_RECV;

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
		  else if (psSock->bio)
		  { r = BIO_read(psSock->bio, s, n);
			if (r > 0) return (size_t)r;
			return 0;
		  }
		  else
#endif

#ifdef WITH_UDP
        if ((psSock->omode & SOAP_IO_UDP))
        { SOAP_SOCKLEN_T k = (SOAP_SOCKLEN_T)sizeof(psSock->peer);
	  memset((void*)&psSock->peer, 0, sizeof(psSock->peer));
          r = recvfrom(psSock->iSocket, s, n, psSock->iSocket_flags, (struct sockaddr*)&psSock->peer, &k);	/* portability note: see SOAP_SOCKLEN_T definition in stdsoap2.h */
	  psSock->peerlen = (size_t)k;
#ifndef WITH_IPV6
          psSock->ip = ntohl(psSock->peer.sin_addr.s_addr);
          psSock->port = (int)ntohs(psSock->peer.sin_port);
#endif
        }
	else
#endif
		{
			// Lettura classica da socket
			r = recv( psSock->sSocket.fdSocket, s, n, 0);//psSock->iSocket_flags);
			iErr=wt_socket_errno;
			if (r >= 0) return (size_t) r;
			if (iErr != WSAEINTR && iErr != WSAEWOULDBLOCK && iErr != WSAEWOULDBLOCK) 
			{
				psSock->iErrorCode = WTE_RECV;
				psSock->iSocketErr = wt_socket_errno; 
				return 0;
			}
			//printf("qui");
		}
	}
  }
  return 0;
}

static void _tcpBufRealloc(S_EH_SOCKET *psSock,DWORD dwNewSize)
{
	BYTE *lp;
	psSock->dwReadBufferSize=dwNewSize; // Calcolo le nuove dimensioni del buffer
	lp=ehAllocZero(psSock->dwReadBufferSize); if (!lp) ehExit("No memory");
	if (psSock->dwReaded>dwNewSize) ehError();
	if (psSock->dwReaded>0) memcpy(lp,psSock->pbReaded,psSock->dwReaded);
	ehFreePtr(&psSock->pbReaded);
	psSock->pbReaded=lp;
}


/******************************************************************************/
static const char * wt_tcp_error(S_EH_SOCKET *psSock)
{ 
  register const char *msg = NULL;
  msg="?";
  return msg;
}

//#define WS_BUFLEN (65536)

static void _ehSetTimeout(struct timeval *psTimeout,INT iSec) {
	if (iSec > 0)
	{ 
		psTimeout->tv_sec = iSec;
		psTimeout->tv_usec = 0;
	}
	else
	{ 
		psTimeout->tv_sec = -iSec/1000000;
		psTimeout->tv_usec = -iSec%1000000;
	}
}


//
// _tcpConnect()
// Crea una connessione con il server (multipiattaforma e multi protocollo)
// Ritorna il socket o errore
//
static int _tcpConnect(S_EH_SOCKET *psSock, CHAR * pszHostName, int port,BOOL bHttps)
{
#ifdef WITH_IPV6
	struct addrinfo hints, *res, *ressave;
	int err;
#endif
	register int fd;
	int len = WT_BUFFERSIZE;
	int set = 1;
	EH_SOCK *psSocket;

	//if (dwRW==1) psSocket=&psSock->sSocketRead; else psSocket=&psSock->sSocketWrite;
	psSocket=&psSock->sSocket;
	
	if (wt_valid_socket(psSocket->fdSocket)) closesocket(psSocket->fdSocket);
	psSocket->fdSocket = WT_INVALID_SOCKET;
	

//  psSock->ierrmode = 0;
#ifdef WITH_IPV6 
	memset((void*)&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
 #ifdef WITH_UDP
	if ((psSock->omode & WT_IO_UDP))
	hints.ai_socktype = SOCK_DGRAM;
	else
 #endif
	hints.ai_socktype = SOCK_STREAM;
	psSock->errmode = 2;
	if (psSock->proxy_host)
	err = getaddrinfo(psSock->proxy_host, soap_int2s(soap, psSock->proxy_port), &hints, &res);
	else
	err = getaddrinfo(host, soap_int2s(soap, port), &hints, &res);
	if (err)
	{ soap_set_sender_error(soap, gai_strerror(err), "getaddrinfo failed in tcp_connect()", WT_TCP_ERROR);
	return WT_INVALID_SOCKET;
	}
	ressave = res;
	again:
	fd = (int)socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	psSock->errmode = 0;
#else

#ifdef WITH_UDP
  if ((psSock->omode & WT_IO_UDP))
    fd = (int) socket(AF_INET, SOCK_DGRAM, 0);
  else
#endif
    fd = (int) socket(AF_INET, SOCK_STREAM, 0);
#endif
	
	if (fd < 0)
	{ 
		psSock->iErrorCode=WTE_SOCKET_CREATE;
		psSock->iSocketErr = wt_socket_errno;
		ehLogWrite("socket failed in tcp_connect()"); // eh_tcp_error(pWS)
		return WT_INVALID_SOCKET;
	}

#ifndef WITH_IPV6

//	psSock->iPeerLen = sizeof(psSocket->sPeer);
	memset((void*)&psSocket->sPeer, 0, sizeof(psSocket->sPeer));
	psSocket->sPeer.sin_family = AF_INET;

	//psSock->errmode = 2;
	if (psSock->proxy_host)
	{ 
		if (_tcpGetHost(psSock, psSock->proxy_host, &psSocket->sPeer.sin_addr))
		{ 
			psSock->iErrorCode=WTE_GET_HOST;
			ehLogWrite("Get proxy host by name failed in tcp_connect()");
			closesocket(fd);
			return WT_INVALID_SOCKET;
		}
		psSocket->sPeer.sin_port = htons((short) psSock->proxy_port);
	}
	else
	{ 
		if (_tcpGetHost(psSock, pszHostName, &psSocket->sPeer.sin_addr))
		{ 
			ehLogWrite("Get host by name failed in tcp_connect()");
			psSock->iErrorCode=WTE_GET_HOST;
			closesocket(fd);
			return WT_INVALID_SOCKET;
		}
		psSocket->sPeer.sin_port = htons((short)port);
	}

	//psSock->errmode = 0;
	if ((psSock->omode & WT_IO_UDP)) return fd;

#endif

#ifndef WITH_LEAN

	if (psSock->iTimeoutWrite)

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

	for (;;)
	{ 
#ifdef WITH_IPV6
		if (connect(fd, res->ai_addr, res->ai_addrlen))
#else
		if (connect(fd, (SOCKADDR*) &psSocket->sPeer, sizeof(psSocket->sPeer))) // (struct sockaddr*)
#endif
		{ 
#ifndef WITH_LEAN
			int iError=wt_socket_errno;//WSAGetLastError();
			if (iError==WSAETIMEDOUT)
			{
				  psSock->iErrorCode=WTE_TIMEOUT_CONNECT;
				  psSock->iSocketErr = 0;
	//			  ehLogWrite("Timeout (%ds)" CRLF "connect failed in tcp_connect()",psSock->iTimeoutSec);
				  closesocket(fd);
				  return WT_INVALID_SOCKET;			
			}

			if (psSock->iTimeoutWrite && (iError == WSAEINPROGRESS || iError == WSAEWOULDBLOCK))
			{ 
				struct timeval timeout;
				WT_SOCKLEN_T k;
				fd_set fds;
				_ehSetTimeout(&timeout,psSock->iTimeoutWrite);

				// Provo ad aprire un socket in scrittura
				FD_ZERO(&fds);
				FD_SET(fd, &fds);
				for (;;)
				{ 
					int r = select((fd + 1), NULL, &fds, NULL, &timeout);
					if (r > 0) break;

					  if (!r)
					  { 
						  psSock->iErrorCode=WTE_TIMEOUT_CONNECT;
						  psSock->iSocketErr = 0;
						  //DBGLOG(TEST, WT_MESSAGE(fdebug, "Connect timeout\n"));
						  ehLogWrite("Timeout (%ds)" CRLF "connect failed in tcp_connect()",psSock->iTimeoutWrite);
						  closesocket(fd);
						  return WT_INVALID_SOCKET;
					  }

					  if (wt_socket_errno != WSAEINTR)
					  { 
						  psSock->iErrorCode=WTE_SOCKET_SELECT_2;
						  psSock->iSocketErr = wt_socket_errno;
						//DBGLOG(TEST, WT_MESSAGE(fdebug, "Could not connect to host\n"));
						ehLogWrite("connect failed in tcp_connect()");
						closesocket(fd);
						return WT_INVALID_SOCKET;
					  }
				}

				k = (WT_SOCKLEN_T) sizeof(psSock->iSocketErr);
				if (!getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&psSock->iSocketErr, &k) && !psSock->iSocketErr) break;
				psSock->iErrorCode=WTE_SOCKET_SELECT_3;
				psSock->iSocketErr = wt_socket_errno;
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
				psSock->iErrorCode=WTE_SOCKET_CONNECT;
				psSock->iSocketErr = wt_socket_errno;
				ehLogWrite("connect failed in tcp_connect()");
				closesocket(fd);
			return WT_INVALID_SOCKET;
		  }
    }  
    else
      break;
  }

#ifdef WITH_IPV6
//  psSock->iPeerLen = 0; /* IPv6: already connected so use send() */
  freeaddrinfo(ressave);
#endif

#ifndef WITH_LEAN
  if (psSock->iTimeoutWrite)
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

	psSocket->fdSocket = fd;
	psSock->imode &= ~WT_ENC_SSL;
	psSock->omode &= ~WT_ENC_SSL;

	//
	// HTTPS = SSL
	//
	if (bHttps)
	{
#ifdef WITH_OPENSSL
		BIO *bio;
		int r;
		if (!psSock->ctx && (psSock->iSocketErr = wt_ssl_auth_init(psSock)))
		{ 
			psSock->iErrorCode=WTE_SSL_AUTH_ERROR;
			ehLogWrite("SSL error" CRLF "SSL authentication failed in tcp_connect(): check password, key file, and ca file.");
			return WT_INVALID_SOCKET;
		}

		psSock->ssl = SSL_new(psSock->ctx); // Nuova sessione 
		if (!psSock->ssl)
		{ 
			//psSock->error = WT_SSL_ERROR;
			psSock->iErrorCode=WTE_SSL_NEW_ERROR;
			ehLogWrite("SSL error");
			return WT_INVALID_SOCKET;
		}

		if (psSock->session)
		{ 
			if (!strcmp(psSock->session_host, pHostName) && psSock->session_port == port)
			{
				SSL_set_session(psSock->ssl, psSock->session);
				SSL_SESSION_free(psSock->session);
				psSock->session = NULL;
			}
		}
		psSock->imode |= WT_ENC_SSL;
		psSock->omode |= WT_ENC_SSL;
		bio = BIO_new_socket(fd, BIO_NOCLOSE);
		SSL_set_bio(psSock->ssl, bio, bio);

#ifndef WITH_LEAN
		if (psSock->iTimeoutSec)
#ifdef WIN32
		{ 
			u_long nonblocking = 1;
			ioctlsocket(fd, FIONBIO, &nonblocking);
		}
#else
		fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
#endif
#endif

		for (;;)
		{ 
			if ((r = SSL_connect(psSock->ssl)) <= 0)
			{ 
				int err = SSL_get_error(psSock->ssl, r);
				if (err != SSL_ERROR_NONE && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
				{ //soap_set_sender_error(soap, ssl_error(soap, r), "SSL connect failed in tcp_connect()", WT_SSL_ERROR);
					ehLogWrite("SSL connect failed in tcp_connect()" CRLF);
				  return WT_INVALID_SOCKET;
				}

				if (psSock->iTimeoutSec)
				{ 
					struct timeval timeout;
					fd_set fds;
					if (psSock->iTimeoutSec> 0)
					{ timeout.tv_sec = psSock->iTimeoutSec;
					timeout.tv_usec = 0;
					}
					else
					{ timeout.tv_sec = -psSock->iTimeoutSec/1000000;
					timeout.tv_usec = -psSock->iTimeoutSec%1000000;
					}
					FD_ZERO(&fds);
					FD_SET(psSock->iSocket, &fds);
					for (;;)
					{ 
						int r = select((psSock->iSocket + 1), &fds, NULL, &fds, &timeout);
						if (r > 0)
						break;
					
						if (!r)
						{
							psSock->iSocketErr = 0;
							//DBGLOG(TEST, WT_MESSAGE(fdebug, "Connect timeout\n"));
							//soap_set_sender_error(soap, "Timeout", "connect failed in tcp_connect()", WT_TCP_ERROR);
							psSock->iErrorCode=WTE_SSL_TIMEOUT;
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
    if (psSock->iTimeoutSec)
#ifdef WIN32
    { u_long blocking = 0;
      ioctlsocket(fd, FIONBIO, &blocking);
    }
#else
      fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)&~O_NONBLOCK);
#endif
#endif

    if (psSock->require_server_auth)
    {
		X509 *peer;
	    int err;

		if ((err = SSL_get_verify_result(psSock->ssl)) != X509_V_OK)
		{
			psSock->iErrorCode=WTE_SSL_CERT_ERROR_1;
			ehLogWrite("SSL certificate presented by peer cannot be verified in tcp_connect() %s", X509_verify_cert_error_string(err));
			return WT_INVALID_SOCKET;
		}

		peer = SSL_get_peer_certificate(psSock->ssl);
		if (!peer)
		{
			psSock->iErrorCode=WTE_SSL_CERT_ERROR_2;
			ehLogWrite("SSL error" CRLF "No SSL certificate was presented by the peer in tcp_connect()");
	        return WT_INVALID_SOCKET;
		}

		X509_NAME_get_text_by_NID(X509_get_subject_name(peer), NID_commonName, psSock->msgbuf, sizeof(psSock->msgbuf));
		X509_free(peer);
		if (eh_tag_cmp(psSock->msgbuf, pHostName))
		{
			psSock->iErrorCode=WTE_SSL_CERT_ERROR_3;
			ehLogWrite("SSL error" CRLF "SSL certificate host name mismatch in tcp_connect()");
			return WT_INVALID_SOCKET;
		}
    }
#else
	psSock->iErrorCode= WTE_SSL_SEND_ERROR;
    return WT_INVALID_SOCKET;
#endif
  }

  return fd;
}
// <---------------- fine _tcpConnect

//
// _tcpGetHost() - Traduce un nome nell'ip
//
static int _tcpGetHost(S_EH_SOCKET *psSock, char * pszAddr, struct in_addr *inaddr)
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
		  iadd = inet_addr(pszAddr); // http://msdn.microsoft.com/en-us/library/ms738563(VS.85).aspx
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
	  if (gethostbyname_r(addr, &hostent, psSock->buf, WT_BUFLEN, &host, &psSock->iSocketError) < 0)
		host = NULL;

#elif defined(_AIXVERSION_431) || defined(TRU64)
	  memset((void*)&ht_data, 0, sizeof(ht_data));
	  if (gethostbyname_r(addr, &hostent, &ht_data) < 0)
	  { 
			host = NULL;
			psSock->iSocketError = h_errno;
	  }

#elif defined(HAVE_GETHOSTBYNAME_R)
	  host = gethostbyname_r(addr, &hostent, psSock->buf, WT_BUFLEN, &psSock->iSocketError);

#elif defined(VXWORKS)
	  /* If the DNS resolver library resolvLib has been configured in the vxWorks
	   * image, a query for the host IP address is sent to the DNS server, if the
	   * name was not found in the local host table. */
	  hostint = hostGetByName(addrcopy);
	  if (hostint == ERROR)
	  { host = NULL;
		psSock->iSocketError = soap_errno; 
	  }
	  WT_FREE(soap, addrcopy);  /*free() is placed after the error checking to assure that
				* errno captured is that from hostGetByName() */
#else

	  if (_sPrivate.bReset) {
		_(_sPrivate);
		_sPrivate.mtxHost=CreateMutex(NULL,FALSE,NULL); // ATTENZIONE: devo liberarlo in uscita
		_sPrivate.arHost=ARNew();
	  }
	//
	// A) Enter critical
	//
	_enterHost(); //EnterCriticalSection(&sSocket.csGetHost);

	//
	// B) Controllo la cache
	//
	sprintf(szServ,"%s|",pszAddr);
	p=ARSearch(_sPrivate.arHost,szServ,FALSE);
	if (p)
	{
		ULONG hostint;
		CHAR *pHostInt;

		pHostInt=strstr(p,"|"); if (!p) ehError();
		pHostInt++;
		hostint=(ULONG) _atoi64(pHostInt); 
		if (!hostint) ehError();
		inaddr->s_addr = hostint;
		_leaveHost();//LeaveCriticalSection(&sSocket.csGetHost);
		return WT_OK;
	}

	if (!(host = gethostbyname(pszAddr))) // http://msdn.microsoft.com/en-us/library/ms738524(VS.85).aspx
	psSock->iSocketErr = h_errno; 
	//LeaveCriticalSection(&sSocket.csGetHost);
	_leaveHost();

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
	sprintf(szServ,"%s|%u",pszAddr,inaddr->s_addr);
	ARAdd(&_sPrivate.arHost,szServ);

	//
	// C) Leave critical
	//

//	 LeaveCriticalSection(&sSocket.csGetHost);
	return WT_OK;
}
static int ehTcpShutdownSocket(int fd, int how) {return shutdown(fd, how);}

//
// ehTcpDisconnect()
//

static int ehTcpDisconnect(S_EH_SOCKET *psSock)
{
#ifdef WITH_OPENSSL
  if (psSock->ssl)
  { int r, s = 0;
    if (psSock->session) SSL_SESSION_free(psSock->session);
    if (*psSock->szHost)
    { psSock->session = SSL_get1_session(psSock->ssl);
      if (psSock->session)
      { strcpy(psSock->session_host, psSock->szHost);
        psSock->session_port = psSock->port;
      }
    }
    r = SSL_shutdown(psSock->ssl);
     if (r != 1)
    { s = ERR_get_error();
      if (s)
      { if (wt_valid_socket(psSock->iSocket))
        { ehTcpShutdownSocket(psSock->iSocket, 1);
          psSock->iSocket = WT_INVALID_SOCKET;
        }
        r = SSL_shutdown(psSock->ssl);
      }
    }
   //  DBGLOG(TEST, if (s) SOAP_MESSAGE(fdebug, "Shutdown failed: %d\n", SSL_get_error(psSock->ssl, r)));
    SSL_free(psSock->ssl);
    psSock->ssl = NULL;
    if (s) return WT_SSL_ERROR;
    ERR_remove_state(0);
  }
#endif

  if (wt_valid_socket(psSock->sSocket.fdSocket) && !(psSock->omode & WT_IO_UDP))
  { 
		ehTcpShutdownSocket(psSock->sSocket.fdSocket, 2);
		closesocket(psSock->sSocket.fdSocket);
		psSock->sSocket.fdSocket = WT_INVALID_SOCKET;
  }
  return WT_OK;
}
/*
//
// ehTcpRead() - Ritorna TRUE se ci sono errori
//
static BOOL ehTcpRead(S_EH_SOCKET *psSock)
{
	BYTE bData[2];
	WCHAR *pw;
//	DWORD  dwCount;
	BOOL bHeaderError=FALSE;
	INT iB;

	ehFreePtr(&psSock->pbReaded);
	psSock->dwReadBufferSize=8192; // 8kb
	psSock->pbReaded=ehAlloc(psSock->dwReadBufferSize);
	psSock->dwReaded=0;

	bHeaderError=FALSE;
	while (TRUE)
	{
		 iB=ehTcpRecv(psSock,bData,1);
		 if (!iB) 
		 {
			 bHeaderError=TRUE; 
			 if (!psSock->iErrorCode) psSock->iErrorCode=WTE_CONNECTION_CLOSE; 
			 break;
		 } // Definisco che un header non può essere più grande di 2048

		 //		 if (!iB) break;
		 if (iB<0) 
		 {
			 if (!psSock->iErrorCode) psSock->iErrorCode=WTE_RECV;
			 psSock->iSocketErr=WSAGetLastError();
#ifdef _DEBUG
			bHeaderError=TRUE; 
			//printf("recv failed: %d\n", psSock->iSocketError); 
#endif
			break;
		 }

		 if (psSock->dwReaded>=psSock->dwReadBufferSize) _tcpBufRealloc(psSock,psSock->dwReadBufferSize+8192);
		 pw=psSock->pbReaded;
		 psSock->pbReaded[psSock->dwReaded]=bData[0]; psSock->dwReaded++;
		 //ehPrintd("%d" CRLF,psSock->dwReaded);
 
	}
	return bHeaderError;
}
*/
/*
INT enSocketFree(S_EH_SOCKET *psSock) // GET/HEAD
{
	ehFreePtr(&psSock->pbReaded);
	return FALSE;
}
*/
/*
// ##################################
static int WGFilter(unsigned int code, LPEXCEPTION_POINTERS ep) {

	ehLogWrite("WebGet EXCEPTION code: %d",code);
	return EXCEPTION_CONTINUE_SEARCH;
}
*/

//
// _ehSocketError()
//
static CHAR *_ehSocketError(INT enError)
{
	static CHAR szServ[200];
	BYTE *p;
	switch (enError)
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
			sprintf(szServ,"ehWebError() ? %d", enError);
			p=szServ;
			break;
	}
	return p;
}

/*
static void WebGetFreeResource(S_EH_SOCKET *psSock)
{
	ehTcpDisconnect(psSock);
	ehFreePtr(&psSock->lpThreadRequest); 
	ehFreePtr(&psSock->lpThreadURI); 
	//LeaveCriticalSection(&sSocket.csMT);
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
  psSock->fsslauth = wt_ssl_auth_init;
  psSock->fsslverify = ssl_verify_callback;
  */

int eh_ssl_server_context(S_EH_SOCKET *psSock, 
							unsigned short flags, 
							const char *keyfile, 
							const char *password, 
							const char *cafile, 
							const char *capath, 
							const char *dhfile, 
							const char *randfile, 
							const char *sid)
{ int err;
  psSock->keyfile = keyfile;
  psSock->password = password;
  psSock->cafile = cafile;
  psSock->capath = capath;
  if (dhfile)
  { psSock->dhfile = dhfile;
    psSock->rsa = 0;
  }
  else
  { psSock->dhfile = NULL;
    psSock->rsa = 1;
  }
  psSock->randfile = randfile;
  psSock->require_client_auth = (flags & _SSL_REQUIRE_CLIENT_AUTHENTICATION);
  if (!(err = wt_ssl_auth_init(psSock)))
  if (sid) SSL_CTX_set_session_id_context(psSock->ctx, (unsigned char*)sid, strlen(sid));
  return err; 
}

//
// eh_ssl_client_context()
// Funzione di settaggio informazioni per autentificazione SSL
//
INT eh_ssl_client_context(S_EH_SOCKET *psSock, unsigned short flags, const char *keyfile, const char *password, const char *cafile, const char *capath, const char *randfile)
{ psSock->keyfile = keyfile;
  psSock->password = password;
  psSock->cafile = cafile;
  psSock->capath = capath;
  psSock->dhfile = NULL;
  psSock->rsa = 0;
  psSock->randfile = randfile;
  psSock->require_server_auth = (flags & _SSL_REQUIRE_SERVER_AUTHENTICATION);
  return 0;
}

//
// ssl_error()
//
static const char * ssl_error(S_EH_SOCKET *psSock, int ret)
{ 
	int err = SSL_get_error(psSock->ssl, ret);
	// DA CAPIRE

//	const char *msg = soap_str_code(h_ssl_error_codes, err);
//	if (msg)
//	strcpy(psSock->msgbuf, msg);
//	else
//	return ERR_error_string(err, psSock->msgbuf);
/*
	if (ERR_peek_error())
	{ 
		unsigned long r;
		strcat(psSock->msgbuf, "\n");
		while ((r = ERR_get_error()))
		ERR_error_string_n(r, psSock->msgbuf + strlen(psSock->msgbuf), sizeof(psSock->msgbuf) - strlen(psSock->msgbuf));
	} 
	else
	{ switch (ret)
	{ case 0:
		strcpy(psSock->msgbuf, "EOF was observed that violates the protocol. The client probably provided invalid authentication information.");
		break;
	  case -1:
		sprintf(psSock->msgbuf, "Error observed by underlying BIO: %s", strerror(errno));  
		break;
	}
	}
	return psSock->msgbuf;
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
static int wt_ssl_auth_init(S_EH_SOCKET *psSock)
{ 
	wt_ssl_init();
	if (!psSock->ctx)
	{ 
		if (!(psSock->ctx = SSL_CTX_new(SSLv23_method()))) 
		{
			ehLogWrite("SSL error" CRLF "Can't setup context");
			return -1;
		}
	}

	if (psSock->randfile)
	{ 
		if (!RAND_load_file(psSock->randfile, -1))
		{
			ehLogWrite("SSL error" CRLF "Can't load randomness");
			return -2;
		}
	}

	if (psSock->cafile || psSock->capath)
	{
		if (!SSL_CTX_load_verify_locations(psSock->ctx, psSock->cafile, psSock->capath))
		{
			ehLogWrite("SSL error" CRLF "Can't read CA file and directory");
			return -3;
		}
	}

	if (!SSL_CTX_set_default_verify_paths(psSock->ctx))
	{
		ehLogWrite("SSL error" CRLF "Can't read default CA file and/or directory");
		return -4;
	}

	if (psSock->keyfile)
	{ 
		if (!SSL_CTX_use_certificate_chain_file(psSock->ctx, psSock->keyfile))
		{
			ehLogWrite("SSL error" CRLF "Can't read certificate key file");
			return -5;
		}

		if (psSock->password)
		{ 
			SSL_CTX_set_default_passwd_cb_userdata(psSock->ctx, (void*) psSock->password);
			SSL_CTX_set_default_passwd_cb(psSock->ctx, ssl_password);
		}

		if (!SSL_CTX_use_PrivateKey_file(psSock->ctx, psSock->keyfile, SSL_FILETYPE_PEM))
		{
			ehLogWrite("SSL error" CRLF "Can't read key file");
			return -5;
		}
	}

/* Suggested alternative approach to check cafile first before the key file:
  if (psSock->password)
  { SSL_CTX_set_default_passwd_cb_userdata(psSock->ctx, (void*)psSock->password);
    SSL_CTX_set_default_passwd_cb(psSock->ctx, ssl_password);
  }
  if (!psSock->cafile || !SSL_CTX_use_certificate_chain_file(psSock->ctx, psSock->cafile))
  { if (psSock->keyfile)
    { if (!SSL_CTX_use_certificate_chain_file(psSock->ctx, psSock->keyfile))
        ehLogWrite("SSL error" CRLF "Can't read certificate or key file");
      if (!SSL_CTX_use_PrivateKey_file(psSock->ctx, psSock->keyfile, SSL_FILETYPE_PEM))
        ehLogWrite("SSL error" CRLF "Can't read key file");
    }
  }
*/

	if (psSock->rsa) // Credo CHECK del certificato
  { 
	  RSA *rsa = RSA_generate_key(512, RSA_F4, NULL, NULL);
      if (!SSL_CTX_set_tmp_rsa(psSock->ctx, rsa))
      {
			if (rsa) RSA_free(rsa);
			ehLogWrite("SSL error" CRLF "Can't set RSA key");
			return -6;
	  }
	  RSA_free(rsa);
  }
  else if (psSock->dhfile)
  { 
		DH *dh = 0;
		BIO *bio;
		bio = BIO_new_file(psSock->dhfile, "r");
		if (!bio) { ehLogWrite("SSL error" CRLF "Can't read DH file"); return -7;}
		dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		BIO_free(bio);
		if (SSL_CTX_set_tmp_dh(psSock->ctx, dh) < 0)
		{ 
			if (dh) DH_free(dh);
			ehLogWrite("SSL error" CRLF "Can't set DH parameters");
			return -8;
		}
		DH_free(dh);
  }
  SSL_CTX_set_options(psSock->ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2);
  SSL_CTX_set_verify(psSock->ctx, psSock->require_client_auth ? (SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT) : psSock->require_server_auth ? SSL_VERIFY_PEER : SSL_VERIFY_NONE, ssl_verify_callback);

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
  SSL_CTX_set_verify_depth(psSock->ctx, 1); 
#else
  SSL_CTX_set_verify_depth(psSock->ctx, 9); 
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
int eh_ssl_accept(S_EH_SOCKET *psSock)
{ 
	BIO *bio;
	int i, r;
	if (!wt_valid_socket(psSock->iSocket)) {ehLogWrite("SSL error" CRLF "No socket in soap_ssl_accept()"); return -1;}
//	if (!psSock->ctx && (psSock->error = wt_ssl_auth_init(pWS))) return _INVALID_SOCKET;
	if (!psSock->ssl)
	{	
		psSock->ssl = SSL_new(psSock->ctx);
		if (!psSock->ssl) { ehLogWrite("SSL error" CRLF "SSL_new() failed in soap_ssl_accept()"); return -2;}
	}
	else SSL_clear(psSock->ssl);

	// Contrassegno che sono in https (ma credo di averlo già fatto)
	psSock->imode |= WT_ENC_SSL;
	psSock->omode |= WT_ENC_SSL;
	#ifdef WIN32
	{ 
		u_long nonblocking = 1;
		ioctlsocket(psSock->iSocket, FIONBIO, &nonblocking);
	}
	#else
	fcntl(psSock->iSocket, F_SETFL, fcntl(psSock->iSocket, F_GETFL)|O_NONBLOCK);
	#endif

	bio = BIO_new_socket(psSock->iSocket, BIO_NOCLOSE);
	SSL_set_bio(psSock->ssl, bio, bio);
	i = 100; 
	while ((r = SSL_accept(psSock->ssl)) <= 0)
	{ 
		int err = SSL_get_error(psSock->ssl, r);
		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
		{ struct timeval timeout;
		  fd_set fd;
		  if (i-- <= 0)
			break;
		  timeout.tv_sec = 0;
		  timeout.tv_usec = 100000;
		  FD_ZERO(&fd);
		  FD_SET(psSock->iSocket, &fd);
		  r = select((psSock->iSocket + 1), &fd, NULL, &fd, &timeout);
		  if (r < 0 && WSAGetLastError() != WSAEINTR)
		  { 
			  psSock->iErrorCode=WTE_SOCKET_SELECT;
			  psSock->iSocketErr = WSAGetLastError();
			return -1;
		  }
		}
		else
		{ psSock->iSocketErr = err;
		  break;
		}
	}

#ifdef WIN32
  { u_long blocking = 0;
    ioctlsocket(psSock->iSocket, FIONBIO, &blocking);
  }
#else
  fcntl(psSock->iSocket, F_SETFL, fcntl(psSock->iSocket, F_GETFL)&~O_NONBLOCK);
#endif

  if (r <= 0)
  { //soap_set_receiver_error(soap, ssl_error(soap, r), "SSL_accept() failed in soap_ssl_accept()");
	  ehLogWrite("SSL_accept() failed in soap_ssl_accept() : %s",ssl_error(psSock, r));
	  return -1;
    //soap_closesock(soap);
    //return WT_SSL_ERROR;
  }

  if (psSock->require_client_auth)
  { 
	X509 *peer;
	int err;
    if ((err = SSL_get_verify_result(psSock->ssl)) != X509_V_OK)
    { //soap_closesock(soap);
      //return soap_set_sender_error(soap, X509_verify_cert_error_string(err), "SSL certificate presented by peer cannot be verified in soap_ssl_accept()");
		ehLogWrite("SSL certificate presented by peer cannot be verified in soap_ssl_accept(): %s",X509_verify_cert_error_string(err));
		return -1;
    }
    peer = SSL_get_peer_certificate(psSock->ssl);
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


#else
#error Easyhand error. richiesto per usare webtool la macro EH_INTERNET
#endif

