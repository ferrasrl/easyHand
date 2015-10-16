// -----------------------------------------------------
// eh_internet.h - API per l'accesso ad Internet
//
// -----------------------------------------------------

#define WT_BUFFERSIZE (65536)
#define WT_OK 0
#define WT_ERR (-1)
#define WT_SSL_ERROR			23

#ifdef __linux__
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    typedef UINT *        SOCKET;
#endif

typedef enum
{
	WTE_OK, // Nessun problema
	WTE_RECV, // Errore durante la ricezione (normale)
	WTE_SSL_RECV, // Errore durante la ricezione in SSL
	WTE_SOCKET_CREATE,	// Errore in creazione del socket
	WTE_SOCKET_ERROR, //
	WTE_SEND_ERROR, //
	WTE_SSL_SEND_ERROR,
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
	WTE_GUNZIP,	// Unzip error

//	WTE_HOST_ERROR,//=-4,
	WTE_TIMEOUT_CONNECT,
	WTE_TIMEOUT,
	WTE_TIMEOUT_FORCED,
	WTE_OUTOFMEMORY,
	WTE_FILE_WRITE_ERROR, // Usato in webUrlToFile
	
	WTE_URL_CRACK_ERROR,
	WTE_URL_SCHEME_UNKNOW, 
	WTE_URI_NULL,
	WTE_USER_ABORT

} EN_HTTP_ERR;

// wt_error
/*
#define EHWT_OK 0
#define EHWT_SERVER_UNKNOW -2	// il server non esiste
#define EHWT_SOCKET_ERROR -3	// non posso creare il socket
#define EHWT_HOST_ERROR -4		// non posso creare la connessione con l'host
#define EHWT_TIMEOUT -200		// Time out in attesa della pagina
#define EHWT_OUTOFMEMORY -300	// Non ho memoria a sufficienza
*/

//
// Errori di ritorno dalle richieste Web (WebGetDirect/WebCopySocket)
//
#define WT_IO			0x00000003	/* IO mask */
#define WT_IO_FLUSH		0x00000000	/* flush output immediately, no buffering */
#define WT_IO_BUFFER	0x00000001	/* buffer output in packets of size WT_BUFLEN */
#define WT_IO_STORE		0x00000002	/* store entire output to determine length for transport */
#define WT_IO_CHUNK		0x00000003	/* use HTTP chunked transfer AND buffer packets */

#define WT_IO_UDP		0x00000004
#define WT_IO_LENGTH	0x00000008
#define WT_IO_KEEPALIVE	0x00000010

#define WT_ENC_LATIN	0x00800020	/* iso-8859-1 encoding */
#define WT_ENC_XML		0x00000040	/* plain XML encoding, no HTTP header */
#define WT_ENC_DIME		0x00000080
#define WT_ENC_MIME		0x00000100
#define WT_ENC_MTOM		0x00000200
#define WT_ENC_ZLIB		0x00000400
#define WT_ENC_SSL		0x00000800

#define WT_ENC		0x00000FFF	/* IO and ENC mask */

#ifdef WITH_OPENSSL
# define OPENSSL_NO_KRB5
# include <openssl/ssl.h>
# include <openssl/err.h>
# include <openssl/rand.h>
# ifndef ALLOW_OLD_VERSIONS
#  if (OPENSSL_VERSION_NUMBER < 0x00905100L)
#   error "Usare OpenSSL 0.9.6 o successivo"
#  endif
#define _SSL_NO_AUTHENTICATION		0x00	/* for testing purposes */
#define _SSL_REQUIRE_SERVER_AUTHENTICATION	0x01	/* client requires server to authenticate */
#define _SSL_REQUIRE_CLIENT_AUTHENTICATION	0x02	/* server requires client to authenticate */


// MD5
#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif
CHAR * strToMd5(CHAR * pszStr, INT iLength);
# endif

#endif



typedef struct webMultiPart * EH_WEB_MP_PTR;

typedef struct webMultiPart {

	CHAR	szName[80];
	INT	iType;	// 0= Campo Normale, 1=File
	CHAR *	lpFileName; // Usato con tipo 1
	CHAR *	lpContentType; // Es images/gif
	CHAR *	lpContentTransferEncoding; // Es binary
	ULONG	ulSize;
	BYTE *	lpData;
	EH_WEB_MP_PTR psNext;

} EH_WEB_MP; // MultiPart Data


typedef struct {

	EN_HTTP_ERR		enCode; // Ritorna il codice di errore
	CHAR 			szDesc[200];
	CHAR			szHttpErrorCode[10];
	CHAR			szHttpErrorDesc[80];
	INT				iSocketErr; // Errore durante l'uso dei socket // Es WSAENETDOWN 10054

} S_WEB_ERR;


typedef struct {
	
	CHAR *	pszAddr; // Indirizzo Web

//	CHAR	szHost[256];
	int		idThread;
	int		iPort;
	int		imode;	// Input Mode
	int		omode;	// Output Mode
	struct	sockaddr_in sPeer;
	SOCKET	iSocket;
	DWORD	dwLastReq;		// clock() dell'ultima richiesta (in previsione di una chiusura automatica a tempo dei socket aperti)
	DWORD	dwCounter;		// Numero di volte che si sta utilizzando la connessione

	BOOL	bHttps;

#ifdef WITH_OPENSSL
	
	SSL * ssl;
	BIO * bio;
	SSL_CTX * ctx;
	short require_server_auth;
	short require_client_auth;
	short rsa;			// when set, use RSA instead of DH
	const CHAR * keyfile;
	const CHAR * password;
	const CHAR * dhfile;
	const CHAR * cafile;
	const CHAR * capath;
	const CHAR * crlfile;
	const CHAR * randfile;
	SSL_SESSION * session;
	CHAR	session_host[300];
	int		session_port;

#endif

} S_HOST_CONNECT;

typedef struct _EH_WEBS {

	BOOL			bAllocated; // La struttura è allocata (da liberare)
	CHAR			szReq[10];	// Tipo di richiesta GET/POST/HEAD

	S_WEB_ERR		sError;

	BYTE *			pData;			// Dati letti senza intestazione
	BYTE *			pPageHeader;	// Intestazione
	DWORD			dwContentLength;
	DWORD			dwDataLength;	// Dimensione dei dati, in caso di content-lenght (ex iContentSize)
	DWORD			dwDataBuffer;	// Dimensione dei dati, in caso di content-lenght (ex iContentSize)
	INT				iDataReaded;	// Dimensione dei dati letti effettiva

	CHAR *			lpUri;		// Indirizzo URI indicato originale
	CHAR *			lpWebServer;	// Nome del server (Trasmissione)
	CHAR *			lpWebPage;		// Pagina(Trasmissione)

	CHAR *			lpUser;			// Nome dell'user
	CHAR *			lpPass;			// Password
	
	CHAR			szHost[256];
	INT				iPort;			// Porta
	BOOL			iKeepAlive;		// T/F se deve tenere la connessione aperta

	CHAR *			lpContentType;	// se NULL = application/x-www-form-urlencoded
//	CHAR *			lpSendOtherHeader; // Altri parametri in header da spedire Es. SOAPAction: "" CRLF
	EH_LST			lstHeader;		// Altri valori da inserire nell'header
	CHAR *			lpUserAgent;	// Se non dichiarato = default
	CHAR *			lpPostArgs;		// In alternativa si indicare qui i dati da inviare in modalità post.
	CHAR *			pszAccept;		// Accept (NULL Automatico)
	BOOL			bPostDirect;	// Modalità diversa di invio con parametri sia in URL che in POST

	// New 2007
	// MULTIPART
	BOOL			fMultiPartData; // T/F se spedisco in multipart (solo in modalità POST)
	EH_WEB_MP_PTR	psMp;			// Coda MultiPart
	EH_WEB_MP **	arsMP;

    void *			(*funcNotify)(struct _EH_WEBS *psWeb,EN_MESSAGE,LONG,void *); // Sotto procedura dedicata al Paint
	void *			pVoid;	// Dati aggiuntivi da inviare alla funzione

	BOOL			fNoHttpErrorControl; // T/F se disabilitare il controllo HTTP degli errori del server

	// Uso interno ----------------
	EH_LST			lstRequest;
	CHAR *			pszThreadRequest;
	CHAR *			pszThreadURI;

	BOOL			bHttps;	// T/F se Https
	BOOL			bReceiveData; // T/F se ci sono dati da leggere nel socket
#ifdef __windows__
//	SOCKET			iSocket;	// Id del socket > richiesto con socket()
    WSADATA			wsaData;
#else
//    INT             iSocket;
#endif
	_DMI *			pdmiCookie; // new 2008

	// Debuging
	BOOL			bVerbose;			// T/F Traccia quello che succede (stdout e log)
	BOOL			bVerboseDetails;	// T/F Traccia tutti i dettagli (es bCommToLog)

	S_HOST_CONNECT *psHost;	
	int				connect_flags; // Da vedere
	int				iTimeoutSec;
	CHAR	*		proxy_host;	// Proxy Server host name
	int				proxy_port;		// Proxy Server port (default = 8080)
	CHAR	*		proxy_userid;	// Proxy Authorization user name
	CHAR	*		proxy_passwd;	// Proxy Authorization password
	char			msgbuf[1024];
	DWORD			dwTimeConnect; // new 2009 : tempo di caricamento
	DWORD			dwTimeLoad; // new 2009 : tempo di caricamento

	CHAR *			pszSocketBuffer;

	// x thread
	BOOL			fRun;		// T/F se il thread sta girando

#ifdef __windows__
	HANDLE			hThread;
	DWORD			dwThread;
#endif

	// Tempo impiegato
	time_t			tStart;
	double			dElapsedTime;	// Tempo impiegato in secondi per caricare la pagina
	BOOL			bAbortRequest;	// T/F se voglio uscire da una richiesta in corso (usato in multithread)
	BOOL			bNotRetryWithError; // non riprova due volte in caso di errore

} EH_WEB;


/*

typedef struct {

	CHAR *	lpCmd;
	BOOL	fRun;
	EN_HTTP_ERR enWebError;
//	void *	(*lpSubNotify)(INT cmd,LONG Info,TCHAR *);
	void	(*funcNotifyEnd)(void *);

	HANDLE	hThread;
	DWORD	dwThread;
	time_t	tStart;
	EH_WEB sWS;	// usato in ehGet
	void	*pVoid;

} EH_URLGET;
*/


// Portability: define WT_SOCKLEN_T
#if defined(SOCKLEN_T)
# define WT_SOCKLEN_T SOCKLEN_T
#elif defined(__socklen_t_defined) || defined(_SOCKLEN_T) || defined(CYGWIN) || defined(FREEBSD) || defined(__FreeBSD__) || defined(__QNX__) || defined(QNX) || defined(_AIX)
# define WT_SOCKLEN_T socklen_t
#elif defined(IRIX) || defined(WIN32) || defined(__APPLE__) || defined(HP_UX) || defined(SUN_OS) || defined(OPENSERVER) || defined(TRU64) || defined(VXWORKS)
# define WT_SOCKLEN_T int
#else
# define WT_SOCKLEN_T size_t
#endif


//
// functions
//

EH_WEB *	webHttpReq(	CHAR *	pszUrl,
						CHAR *	pszReq,
						void *	(*funcNotify)(EH_WEB *,EN_MESSAGE cmd,LONG Info,void *),
						BOOL	bAsync,
						INT	iTimeoutSec);
EH_WEB *	webHttpReqEx(EH_WEB * psWebReq,BOOL bAsync);
void		webHttpReqFree(EH_WEB *);
BOOL		webHttpReqAbort(EH_WEB *psWeb);

EN_HTTP_ERR webHttpReqEngine(EH_WEB * psWeb);
CHAR *		webErrorString(EN_HTTP_ERR enError);
void		webAddMultiPart(EH_WEB * psWeb,EH_WEB_MP * psMultiPart);

typedef struct {
	CHAR *pName;		// Nome del coockie
	CHAR *pValue;		// Valore del coockie
	CHAR *pPath;		// Percorso
	CHAR *pExpires;
	CHAR *pSecure;
	CHAR *pDomain;
} EH_WEB_COOKIE;

void	webCookieCreate(_DMI *pdmiCookie);
CHAR *	webCookieString(_DMI *pdmiCookie);
void	webCookieDestroy(_DMI *pdmiCookie);
void	webCookieFree(EH_WEB_COOKIE *psCookie);
void	webCookieClean(_DMI *pdmiCookie);
void	webCookieGet(_DMI *pdmiCookie,CHAR * pPageHeader);
void	webCookieAdd(_DMI *pdmiCookie,CHAR * pSetCookie);
BOOL	webUrlToFile(	CHAR * pszUrl,
						UTF8 * pszFileSaveUtf,
						BOOL   fFailExist,		  // T/F fallisce se esiste
						BOOL   fAsciiEqualControl, // T/F se il file su disco è uguale a quello letto non sovrascrivo  // serve ad esempio per limitare i trasferimenti in FTP
						INT		iHtmlCleanLevel,		// T/F se devo togliere ritorni a capo e spazi
						_DMI * pdmiCookie,
						CHAR * pUser,
						CHAR * pPassword,
						S_WEB_ERR * psError);
BOOL	webUrlToFileEx(	CHAR *	pszUrl,
						UTF8 *	pszFileSaveUtf,
						BOOL	fFailExist,		  // T/F fallisce se esiste
						BOOL	fAsciiEqualControl, // T/F se il file su disco è uguale a quello letto non sovrascrivo  // serve ad esempio per limitare i trasferimenti in FTP
						INT		iHtmlCleanLevel,		// T/F se devo togliere ritorni a capo e spazi
						_DMI *	pdmiCookie,
						CHAR *	pUser,
						CHAR *	pPassword,
						CHAR *	szUserAgent,
						S_WEB_ERR * psError,
						void * (*funcNotify)(struct _EH_WEBS *psWeb,EN_MESSAGE,LONG,void *),
						INT iTimeOut);
//
// Update dei programmi Ferrà
//
BOOL	webUpdateVerifyEx(CHAR *pszProject,CHAR *pszApplication,CHAR *pszVersion, CHAR * pszCommandLine, BOOL bBackGround,INT iEveryMin);
#define webUpdateVerify(iEveryMin) webUpdateVerifyEx(NULL,NULL,NULL,NULL,TRUE,iEveryMin)
void	webUpdateSetUrl(CHAR *pszDomain);
CHAR *	webUpdateGetUrl(void);

BOOL	webHostKeepAlive(BOOL bKeep);
void	webHostCloseConnect(EH_WEB * psWeb);
void	webHostCloseConnections(void); 


