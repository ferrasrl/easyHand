// -----------------------------------------------------
// ehSocket
// .h -\Easyhand\Ehtoolx\OpenSSL - GnuWin32\include
//
// -----------------------------------------------------
//#include <wininet.h>
#include <time.h>
#include <excpt.h>

#ifndef EH_SOCKET
#define EH_SOCKET
#endif

typedef struct {
	EN_HTTP_ERR enErr;
	DWORD dwSize;
	BYTE *pData;
} S_SOCKET_EVENT;

typedef struct {
	SOCKET	fdSocket;	// Id del socket > richiesto con socket() per la lettura
	struct	sockaddr_in sPeer; // Socket per la comunicazione  Ex .aWebServer
} EH_SOCK;

typedef struct {

	//CRITICAL_SECTION csSocket;
	HANDLE		mtxSocket;

	EN_HTTP_ERR iErrorCode; // Ritorna il codice di errore
	INT		iSocketErr; // Errore durante l'uso dei socket // Es WSAENETDOWN 10054
	
	INT iWebGetError;
	CHAR *pWebGetError;

	int	iTimeoutRead;
	int	iTimeoutWrite;

	char *	pszAddress;		// Nome del server (Trasmissione)
	int		port;
	BOOL	bAutoReconnect;

//	BYTE *	pbSend;	// In alternativa si indicare qui i dati da inviare in modalità post.
//	DWORD	dwSendSize;

	BYTE *	pbReaded;			// Dati Letti dal socket
	DWORD	dwReadBufferSize; // Dimensioni del buffer in lettura
	DWORD	dwReaded;
	BOOL	bReadEvent;		// T/F se si vuole la lettura per eventi (a blocchi)

	_DMI	dmiEvent;		// array dei blocchi
	
    INT (*funcNotify)(EH_SRVPARAM); // Funzioni per le notifiche
	void	*pCustomData;
	// BOOL fNoHttpErrorControl; // T/F se disabilitare il controllo HTTP degli errori del server
	

	// Uso interno ----------------
	//EH_SOCK sSocketRead;
	EH_SOCK sSocket;

//	SOCKET	iSocketWrite;	// Per la scrittura
//	struct	sockaddr_in sPeerWrite; // Socket per la comunicazione  Ex .aWebServer

	WSADATA wsaData;
//	INT iPeerLen;

	BOOL bCommToLog; // Scrive le comuinicazione ne log di EasyHand

#ifdef WITH_OPENSSL
	BIO *bio;
	SSL *ssl;
	SSL_CTX *ctx;
	short require_server_auth;
	short require_client_auth;
	short rsa;			/* when set, use RSA instead of DH */
	const CHAR *keyfile;
	const CHAR *password;
	const CHAR *dhfile;
	const CHAR *cafile;
	const CHAR *capath;
	const CHAR *crlfile;
	const CHAR *randfile;
	SSL_SESSION *session;
	CHAR session_host[300];
	int session_port;
#endif

	CHAR szHost[256];
	int	imode;	// Input Mode
	int	omode;	// Output Mode
	int connect_flags; // Da vedere

//	const char * proxy_host;	/* Proxy Server host name */
	CHAR * proxy_host;	/* Proxy Server host name */
	int proxy_port;		/* Proxy Server port (default = 8080) */
	const char *proxy_userid;	/* Proxy Authorization user name */
	const char *proxy_passwd;	/* Proxy Authorization password */
	char msgbuf[1024];
	int keep_alive;
	DWORD dwTimeConnect; // new 2009 : tempo di caricamento
	DWORD dwTimeLoad; // new 2009 : tempo di caricamento

	HANDLE	hThread;
	DWORD	dwThread;
	BOOL	bThreadRun;		//T/F se il thread di ricezione è in corso
	BOOL	bThreadStop;

} S_EH_SOCKET;


EN_HTTP_ERR ehSocket(S_EH_SOCKET *psSock,EN_MESSAGE enCmd,void *ptr);