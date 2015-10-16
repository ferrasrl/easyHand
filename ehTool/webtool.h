// -----------------------------------------------------
// WEBTOOL.H                                           |
// Tool di Utilities varie per l'accesso ad internet   |
// per SSL aggiungere nelle directory
// .h -\Easyhand\Ehtoolx\OpenSSL - GnuWin32\include
//
// -----------------------------------------------------
#include <wininet.h>
#ifndef EH_INTERNET
#error webtool.h: definire EH_INTERNET
#endif
/*
#ifndef EH_WEBTOOL
#define EH_WEBTOOL
#endif

#define WT_BUFFERSIZE (65536)

#define WT_OK 0
#define WT_ERR (-1)
#define WT_SSL_ERROR			23

typedef enum
{
	WTE_OK, // Nessun problema
	WTE_RECV, // Errore durante la ricezione (normale)
	WTE_SSL_RECV, // Errore durante la ricezione in SSL
	WTE_SOCKET_CREATE,	// Errore in creazione del socket
	WTE_SOCKET_ERROR, //
	WTE_SSL_SEND_ERROR, //
	WTE_SSL_AUTH_ERROR, // SSL authentication failed in tcp_connect(): check password, key file, and ca file
	WTE_SSL_NEW_ERROR,
	WTE_SSL_TIMEOUT,
	WTE_SSL_CERT_ERROR_1,
	WTE_SSL_CERT_ERROR_2,
	WTE_SSL_CERT_ERROR_3,
	WTE_SOCKET_SELECT_2,
	WTE_SOCKET_SELECT_3,
	WTE_SOCKET_CONNECT,
	WTE_GET_HOST,
	WTE_SETSOCKOPT, // Errore in WTE_SETSOCKOPT
	WTE_HTTP_HEADER_NOT_200, // L'header non contiene 200 OK
	WTE_HEADER_ERROR, // Header oltre 2048
	WTE_CONNECTION_CLOSE,
	WTE_BLOCK_TOO_BIG,

//	WTE_HOST_ERROR,//=-4,
	WTE_TIMEOUT_CONNECT,
	WTE_TIMEOUT,//=-200, 22 Timeout
	WTE_TIMEOUT_FORCED,//=-200,
	WTE_OUTOFMEMORY,//=-200,
} EN_WT_ERROR;
// wt_error

*/
typedef struct {

	CHAR szName[80];
	INT iType;	// 0= Campo Normale, 1=File
	CHAR *lpFileName; // Usato con tipo 1
	CHAR *lpContentType; // Es images/gif
	CHAR *lpContentTransferEncoding; // Es binary
	ULONG ulSize;
	BYTE *lpData;

} FWS_MP;

/*
typedef struct {
	
	CHAR *	pszUserName;
	CHAR *	pszUserPass;
	CHAR *	pszServer;
	void *	pVoidParam;

} EH_WEB;
*/

typedef struct {
//	INT iCheck;

	EN_HTTP_ERR iErrorCode; // Ritorna il codice di errore
	INT			iSocketErr; // Errore durante l'uso dei socket // Es WSAENETDOWN 10054

	INT			iWebGetError;
	CHAR *	pWebGetError;

	CHAR *	lpUri;   // Indirizzo URI indicato originale

	CHAR *	lpWebServer;		// Nome del server (Trasmissione)
	CHAR *	lpWebPage;		// Nome del server (Trasmissione)
	CHAR *	lpUser;			// Nome dell'user
	CHAR *	lpPass;			// Password
	CHAR *	lpContentType;	// se NULL = application/x-www-form-urlencoded
	CHAR *	lpSendOtherHeader; // Altri parametri in header da spedire Es. SOAPAction: "" CRLF
	CHAR *	lpUserAgent;	// Se non dichiarato = default
	CHAR *	lpPostArgs;	// In alternativa si indicare qui i dati da inviare in modalità post.

	BOOL	bCommToLog; // Scrive le comuinicazione ne log di EasyHand

	// New 2007
	// MULTIPART
	BOOL	fMultiPartData; // T/F se spedisco in multipart (solo in modalità POST)
	FWS_MP **arMP;

	BYTE *	pPageHeader; // Intestazione
	DWORD	dwContentLength;	// Dimensione dei dati, in caso di content-length (ex iContentSize)
	DWORD	dwDataSize;			// Dimensione allocata della memoria per i dati
	INT		iDataReaded;		// Dimensione dei dati letti effettivi
	BYTE *	pData; // Dati letti senza intestazione

    void  * (*SubControl)(INT,LONG,void *); // Sotto procedura dedicata al Paint
	BOOL	fNoHttpErrorControl; // T/F se disabilitare il controllo HTTP degli errori del server

	// Uso interno ----------------
	CHAR *	lpThreadRequest;
	CHAR *	lpThreadURI;

	BOOL	bHttps;	// T/F se Https
#ifdef __windows__
	SOCKET	iSocket;	// Id del socket > richiesto con socket()
#else
    int     iSocket
#endif
    WSADATA wsaData;
	struct	sockaddr_in sPeer; // Socket per la comunicazione in SMTP Ex .aWebServer
	INT	iPeerLen;
	_DMI *	pdmiCookie; // new 2008

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

	CHAR	szHost[256];
	int		port;
	int		imode;	// Input Mode
	int		omode;	// Output Mode
	int		connect_flags; // Da vedere
	int		iTimeoutSec;
	//int connect_timeout; // Timeout di connessione
	//int recv_timeout; // Timeout di ricezione
	const char *proxy_host;	/* Proxy Server host name */
	int		proxy_port;		/* Proxy Server port (default = 8080) */
	const char *proxy_userid;	/* Proxy Authorization user name */
	const char *proxy_passwd;	/* Proxy Authorization password */
	char	msgbuf[1024];
	int		keep_alive;
	DWORD	dwTimeConnect; // new 2009 : tempo di caricamento
	DWORD	dwTimeLoad; // new 2009 : tempo di caricamento

	CHAR	*pszSocketBuffer;

} FWEBSERVER;


void WebStartup(void); // Una tantum carica la DLL in memoria per i sockets

void WebToolStart(void);
void WebToolEnd(void);
BOOL WebOpen(CHAR *User);
void WebClose(void);

BOOL WebCopy(CHAR *User,
			 CHAR *lpWebServer, // Server da Aprire
			 CHAR *lpPage,      // Pagina da richiedere
			 CHAR *lpMethod,    // Metodo
			 CHAR *lpDati,      // Dati da inviare
			 CHAR *lpFileName,
			 void * (*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS),
			 BOOL Vedi,
			 BOOL fSecure);

void ThereIsPut(CHAR *Line,CHAR *Tag,CHAR *Receive,DWORD dwSize);
void VarExtractCat(CHAR *Line,CHAR *Tipo,CHAR *Form,DWORD dwSize);
void VarAddCat(CHAR *name,CHAR *value,CHAR *Form,DWORD dwSize);
void VarFinal(CHAR *Form);
BOOL WebCheckConnect(CHAR *lp);

void LinkDivide(CHAR *Buf,
				CHAR *Protocol, // Http,Ftp ecc...
				CHAR *Domain,   // Nome del dominio
				CHAR *Page,     // Nome della pagina
				CHAR *Param);  // Parametri se Š un GET mode

void ISOpen(void);


BOOL FtpCheckConnect(CHAR * UserName,        // Nome dell'utente
					 CHAR * PassWord,		// Password
					 CHAR * lpFtpServer		// Server da Aprire
					 );

EH_AR FtpDir(CHAR *	lpUserName,        // Indicazione dell'utente (new 2008)
			 CHAR *	lpPassWord,
	         CHAR *	lpFtpServer, // Server da Aprire
	         CHAR *	lpszSearchFile,  // Pagina da richiedere
	         void *		(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS), // Funzione di controllo e display
			 INT *piError);
/*
BOOL FtpGet(CHAR *lpUserName,        // Indicazione dell'utente (new 2008)
			CHAR *lpPassWord,
	        CHAR *lpFtpServer, // Server da Aprire
			CHAR *lpLocalFileDest,
	        CHAR *lpRemoteFileSource,  // Pagina da richiedere
            void *		(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS), // Funzione di controllo e display
	        BOOL fFailIfExists,
			INT *piError, // TRUE/FALSE Visione dei passaggi di transazione
			BOOL bAttivo,
			void *		pVoidParam);
*/
BOOL FtpGet(CHAR *	lpUserName,        // Indicazione dell'utente 
			CHAR *	lpPassWord,
	        CHAR *	lpFtpServer, // Server da Aprire
			CHAR *	lpLocalFileDest,
	        CHAR *	lpRemoteFileSource,  // Pagina da richiedere
	        void *	(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS), // Funzione di controllo e display
	        BOOL	fFailIfExists, // TRUE/FALSE Visione dei passaggi di transazione
			INT *	lpiErrorFtp, 
			BOOL	bModoAttivo,
			void *	pVoidParam);

BOOL FtpDelete(	CHAR *lpUserName,        // Indicazione dell'utente (new 2008)
				CHAR *lpPassWord,
				CHAR *lpFtpServer, // Server da Aprire
				CHAR *lpFileDelete,
				void *		(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS), // Funzione di controllo e display
				INT *lpiError	 ,   // TRUE/FALSE Visione dei passaggi di transazione
				BOOL bModoAttivo);  // TRUE se si vuole il modo attivo


BOOL FtpSend(CHAR *		lpUserName,        // Indicazione dell'utente
			 CHAR *		lpPassWord,
	         CHAR *		lpFtpServer, // Server da Aprire
			 CHAR *		lpLocalFileSource,
	         CHAR *		lpNewRemoteFile,  // Pagina da richiedere
			 INT		dwBlock, // Byte trasferiti per volta
	         void *		(*funcNotify)(EH_WEB * psWeb,EH_SRVPARAMS), // Funzione di controllo e display
	         BOOL		Vedi,
			 INT *		lpiError,
			 void *		pVoidParam);
			 
BOOL FInternetCheck(CHAR *lpAddress,unsigned short usiPort,CHAR *lpBuffer,INT iSizeBuffer);

#ifndef _WIN32_WCE

/*
INT FWebGetFree(FWEBSERVER *lpsWS); // Da usare dopo
BOOL FWebCopySocket(CHAR *lpWebAddress,CHAR *lpLocalFile,BOOL fFailIsExist,void *(*subNotify)(EN_MESSAGE cmd,LONG Info,CHAR *),INT iTimeOut,BOOL fOsEventAnalisys,FWEBSERVER *psWS);
INT FWebGetDirect(FWEBSERVER *lpsWS,CHAR *lpCmd,void *(*subNotify)(EN_MESSAGE cmd,LONG Info,CHAR *)); // cmd:GET/HEAD
INT FWebGetDirectMT(FWEBSERVER *lpsWS,CHAR *lpCmd,void *(*subNotify)(EN_MESSAGE cmd,LONG Info,CHAR *),INT iTimeOutSec,BOOL fOsEventAnalisys); // GET/HEAD ( New 2006)
*/

INT FWebGetFree(FWEBSERVER *lpsWS); // Da usare dopo
BOOL FWebCopySocket(CHAR *lpWebAddress,CHAR *lpLocalFile,BOOL fFailIsExist,void *(*subNotify)(INT cmd,LONG Info,CHAR *),INT iTimeOut,BOOL fOsEventAnalisys,FWEBSERVER *psWS);
INT FWebGetDirect(FWEBSERVER *lpsWS,CHAR *lpCmd,void *(*subNotify)(INT cmd,LONG Info,CHAR *)); // cmd:GET/HEAD
INT FWebGetDirectMT(FWEBSERVER *lpsWS,CHAR *lpCmd,void *(*subNotify)(INT cmd,LONG Info,CHAR *),INT iTimeOutSec,BOOL fOsEventAnalisys); // GET/HEAD ( New 2006)

typedef struct {

	FWEBSERVER *psWS;
	CHAR *	lpCmd;
	BOOL	fRun;
	EN_HTTP_ERR enWebError;
//	void *	(*lpSubNotify)(INT cmd,LONG Info,CHAR *);
	void	(*funcNotifyEnd)(void *);

	HANDLE	hThread;
	DWORD	dwThread;
	time_t	tStart;
	FWEBSERVER sWS;	// usato in ehGet
	void	*pVoid;

} EH_URLGET;
/*
// 2010
EH_URLGET * ehUrlGet(  FWEBSERVER *psWS,
						CHAR *lpCmd, // GET/HEAD
						void (*funcNotifyEnd)(void *),
						INT iTimeOutSec,
						void *pVoid);
void webHttpReqFree(EH_URLGET *);
*/
void FWebSocketOpen(void);
void FWebSocketClose(INT );
//CHAR *WebErrorDesc(INT iWebError);

#ifdef WITH_OPENSSL
INT eh_ssl_client_context(FWEBSERVER *pWS, unsigned short flags, const char *keyfile, const char *password, const char *cafile, const char *capath, const char *randfile);
#endif

#endif

#ifdef _WIN32_WCE
BOOL CEInternetConnect(void);
BOOL CEInternetDisconnect(void);
#endif

// New 2007

#ifndef _WIN32_WCE
void FWebMpFree(FWS_MP **arMP);
#endif

BOOL UrlToFile(CHAR *lpUrl,
//			   void *pFileNameSave,
//			   BOOL bFileNameUnicode, // T/F se il nome del file è unicode
			   BYTE *pszFileNameUtf,
			   BOOL fFailExist,		  // T/F fallisce se esiste
			   BOOL fAsciiEqualControl, // T/F se il file su disco è uguale a quello letto non sovrascrivo  // serve ad esempio per limitare i trasferimenti in FTP
			   BOOL bSpaceRemove,		// T/F se devo togliere ritorni a campo e spazi
			   _DMI *pdmiCookie,		// NULL Se non usato (contenitore per Cookie)
			   CHAR *pUser,
			   CHAR *pPassword);
typedef struct {
	CHAR *pName;		// Nome del coockie
	CHAR *pValue;		// Valore del coockie
	CHAR *pPath;		// Percorso
	CHAR *pExpires;
} EH_COOKIE;

void	web_cookie_create(_DMI *pdmiCookie);
void	web_cookie_destroy(_DMI *pdmiCookie);
void	web_cookie_add(_DMI *pdmiCookie,CHAR *pSetCookie);
void	web_cookie_free(EH_COOKIE *psCookie);
void	web_cookie_clean(_DMI *pdmiCookie);
void	web_cookie_get(_DMI *pdmiCookie,CHAR *pPageHeader);
CHAR *	web_cookie_string(_DMI *pdmiCookie);

CHAR *ehWebError(INT iWebError); // 2010

/* Portability: define WT_SOCKLEN_T */
#if defined(SOCKLEN_T)
# define WT_SOCKLEN_T SOCKLEN_T
#elif defined(__socklen_t_defined) || defined(_SOCKLEN_T) || defined(CYGWIN) || defined(FREEBSD) || defined(__FreeBSD__) || defined(__QNX__) || defined(QNX) || defined(_AIX)
# define WT_SOCKLEN_T socklen_t
#elif defined(IRIX) || defined(WIN32) || defined(__APPLE__) || defined(HP_UX) || defined(SUN_OS) || defined(OPENSERVER) || defined(TRU64) || defined(VXWORKS)
# define WT_SOCKLEN_T int
#else
# define WT_SOCKLEN_T size_t
#endif
