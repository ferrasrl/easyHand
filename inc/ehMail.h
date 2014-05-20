// -------------------------------------------------------------
// ehMail.h  Clienti di posta elettronica
//
// Created by G.Tassistro   (Aprile 2000)
// by Ferra' srl 2010
// -------------------------------------------------------------

// Tipo di decodifica
typedef enum {

	MIMEENCOD_NO ,// 0
	MIMEENCOD_QP,//  1 // Quote-printable
	MIMEENCOD_B64,// 2 // base64
	MIMEENCOD_7BIT,//  3 // 7bit
	MIMEENCOD_8BIT//  4 // 8bit

} EN_MIME_ENCODE;

// Dove si trova l'oggetto
#define MIMEOBJ_MEMORY 0
#define MIMEOBJ_FILE 1
#define MIMEOBJ_MULTI 2
#define MIMEOBJ_END 3

#define FMAIL_TEXT 0
#define FMAIL_MIME 1
#define FMAIL_MIME_SP 2 // Mime single part (new 2006)
#define FMAIL_DIRECT 1

typedef struct {

	INT				iType;			 // 0=memory (lpData=memoria), 1=File (lpData=Nome del file) MIMEOBJ_MULTI=Intestazione per Multiparti
	EN_MIME_ENCODE	iEncoding;		 // 0=Nessuna 1=Quoted-printable 2=base64
	BOOL			bSourceEncoded; // T/F se il sorgente indicato è già codificato come da iEncoding

	//CHAR *lpSource;		 // Sorgente
	BOOL			bFreeSource;		// T/F se deve liberare le risorse
	CHAR *			lpSource;
	INT				iSourceSize;	 // Solo se il contenuto è binario (iEncoding=base64)
	CHAR *			lpBoundary;    // Es. application/msword; name="nome.doc" (E' OBBLIGATORIO)
	CHAR *			lpContentType;  // Es. application/msword; name="nome.doc" (E' OBBLIGATORIO)
	//CHAR *lpCharSet;  // Es. application/msword; name="nome.doc" (E' OBBLIGATORIO)
	CHAR *			lpDisposition; // Es. attachment; filename="nome.doc"
	CHAR *			lpID;			 // un valore alfanumerico (facoltativo)
	CHAR *			lpDescription; // Una descrizione (facoltativo)
	INT				iParent;		 // Per le multidefinizioni alternative dice a chi fa riferimento
	//INT iFirstChild;

} S_MIMEPARTLINK;

typedef struct {

	INT iType; // 0=memory (lpData=memoria), 1=File (lpData=Nome del file)

	BOOL			bFreeSourceOriginal;		// T/F se deve liberare le risorse (i puntatori li sotto)
	CHAR *			pSourceOriginal; // Puntatore al sorgente originale
	CHAR *			pSourceDecoded;
	WCHAR *			pwSource;	// Traduzione in Unicode

	INT				iSourceSize; // Solo se il contenuto è binario
	EN_MIME_ENCODE	iEncoding; // 0=Nessuna 1=Quoted-printable 2=base64
	BOOL			bSourceEncoded; // T/F se il sorgente indicato è già codificato come da iEncoding
	CHAR			szContentType[255]; // Es. application/msword; name="nome.doc" (E' OBBLIGATORIO)
	//CHAR szCharSet[255]; // Es. application/msword; name="nome.doc" (E' OBBLIGATORIO)
	CHAR			szDisposition[255]; // Es. attachment; filename="nome.doc"
	CHAR			szID[255]; // un valore alfanumerico (facoltativo)
	CHAR			szDescription[255]; // Una descrizione (facoltativo)
	CHAR			szBoundary[255]; // Definizione del boundary
	INT				hdlConvert; // Usato in ricezione se >0 contiene la conversione in binario della parte MIME
	INT				iParent;
	BOOL			fError; // Se esiste un errore su questa parte

} S_MIMEPARTMEMO;

typedef struct {

	CHAR * pEmail;
	CHAR * putfName;

} FMAIL_ADDR;

typedef struct {

	INT iSize;
	INT iType; // 0=TEXT , 1=MIME MultiPart
	S_MIMEPARTLINK MimePartLink;

	/*
	CHAR *lpFromName;   // Nome di chi manda il messaggio
	CHAR *lpFromEmail;  // E-mail di chi manda il messaggio
	CHAR *lpToName;     // Nome a cui va spedito il messaggio
	CHAR *lpToEmail;    // E-mail a cui va spedito il messaggio
	*/

	FMAIL_ADDR	sFrom;
	BYTE *		putfOrganization;
	EH_LST		lstHeaderExtra;

	FMAIL_ADDR	sTo;
	FMAIL_ADDR	sCc;
	FMAIL_ADDR	sBcc;
	FMAIL_ADDR	sReplyTo;
	FMAIL_ADDR	sNotifyTo;

	CHAR *		putfSubject;     // Soggetto
	CHAR *		pSubjectEncoded; // New 2009 (Se non espressamente indicato fatto in automatico dal sistema)

	INT			iMessLocation; // Dove si trova il messaggio da spedire (era iMessType)

	INT			iMailEncoding; // 0=Automatico SE_BASE64, Quote Printable
//	INT	iCharSet; // 0=Test Automatico oppure SE_UTF8
	CHAR *		pContentType; // Es text/plain; charset=ISO-8859-15; format=flowed
	CHAR *		pContentTransferEncoding;	// Es Content-Transfer-Encoding: 7bit
	CHAR *		putfMessage; // Messaggio originale

	CHAR *		lpDate;       // Data del messaggio (solo ricezione)

	/*
	CHAR *lpNotifyName;   // Notify:Nome di chi deve ricevere la notifica del messaggio
	CHAR *lpNotifyEmail;  // Notify:E-mail di chi deve ricevere la notifica del messaggio

	CHAR *lpToCCName;     // Nome a cui va spedito il messaggio
	CHAR *lpToCCEmail;    // E-mail a cui va spedito il messaggio

	CHAR *lpToBCCName;     // Nome a cui va spedito il messaggio
	CHAR *lpToBCCEmail;    // E-mail a cui va spedito il messaggio

	CHAR *lpReplyToName;     // Nome a cui va spedito il messaggio
	CHAR *lpReplyToEmail;    // E-mail a cui va spedito il messaggio
*/
//	CHAR *lpSubjectEncode; // New 2007
	INT	iPriority;

	INT	iLastErrorSend; // Ultimo errore in invio
	time_t	tElapsedTime;	// Tempo impiegato per spedire

} EH_MAILTO;

typedef struct {

	INT		iSize;
	CHAR *	lpMessProcess; // Puntatore al messaggio originale da processare
	INT		iType; // 0=US-ASCII , 1=MIME MultiPart

	CHAR *	pDate;       // Data del messaggio

	CHAR *	pSubjectOriginal;    // Soggetto
	WCHAR *	pwSubject;
	CHAR *	pSubjectUtf8;

	CHAR *	pFromOriginal;   // Nome di chi manda il messaggio
	WCHAR *	pwFromName;
	CHAR *	pFromNameUtf8;

	CHAR *	pFromEmail;	// E-mail di chi manda il messaggio

	//	CHAR *lpFromName;  // E-mail di chi manda il messaggio
//	CHAR *lpFromEmail;  // E-mail di chi manda il messaggio
	CHAR	*pToOriginal;     // Nome a cui va spedito il messaggio
	WCHAR	*pwToName;
	CHAR	*pToNameUtf8;
	//CHAR *pToNameUtf8;     // Nome a cui va spedito il messaggio
	CHAR	*pToEmail;    // E-mail a cui va spedito il messaggio

	INT	iMessType;	// Se in memoria o su disco
	CHAR *	lpMyServer;   // Chi sono

	CHAR *	lpUSMessage;	// Puntatore al messaggio semplice non MIME
	CHAR *	pszAutoSubmitted;

	_DMI *lpMIME; // Puntatore a zona Mime per la lettura delle parti (FMIMEPARTMEMO)

} S_MAILFROM;


typedef enum {

	MP_SMTP_STANDARD,
	MP_SMTP_AUTH, // 11
	MP_IMAP

} EN_MAILPROT;


//
// S_MAIL_PARAM - Parametri per connettersi ai server
//
typedef struct {
	
	union {
	
		INT			iSMTPMode; // 0=RFC822 standard, 1=Smtp Con autentica
		EN_MAILPROT	enProtocol;
	};

	CHAR 	szSMTPServer[300];   // Nome del server SMTP (Trasmissione) in caso di IMAP, server IMAP
	INT		iPortSend;	// Se indicato porta di invio alternativa (default 25 per SMTP)
	CHAR 	szSMTPUser[300];	// Solo con autentica
	CHAR 	szSMTPPassword[300];   // Solo con autentica
	CHAR 	szPOP3Server[300];   // Nome del server POP3 (Ricezione)
	CHAR 	szPOP3User[300];         // Nome dell'user
	CHAR 	szPOP3Password[300];         // Password
	CHAR 	szMyServer[300];   // Chi sono Es www.ferra.com
    INT		(*SubControl)(INT,LONG,void *); // Sotto procedura dedicata al Paint

} S_MAIL_PARAM;

//
// S_MAIL_STATUS - Struttura interna statica usata per il controllo del ehMail
//

typedef struct {

	BOOL		bInit;	// T/F se la strututra è stata inizializzata
	BOOL		bOpen;	// T/F (ex bEngine) se i server sono aperti
	S_MAIL_PARAM sParams;

	// Gestione errori
//	BOOL		bError;	


//	BOOL	bEngine; // TRUE/FALSE se il Fmailer è pronto
	BOOL		bLogWrite;
	BOOL		bTrace; // Traccia la comunicazione
	EH_LST		lstTrace;	// Lista dell'ultima comunicazione


	// Gestione degli errori
	INT			iError;
	CHAR *		pszError;
	INT			iSMTPLastCode;
	CHAR *		pszLastReceive;
	CHAR *		pszLastReceiveError;

	// Uso Interno
	EH_LST		lstTemp;
	BOOL		bHelo;		// T/F se helo è stato fatto
	INT			iCheck;

#ifdef __windows__
    struct		hostent *hPOP3;    // Host POP3
	struct		sockaddr_in aPOP3; // Socket per la comunicazione in POP3
    struct		hostent *hSMTP;    // Host SMTP
	struct		sockaddr_in aSMTP; // Socket per la comunicazione in SMTP
#endif

	INT			POP3socket;
	INT			SMTPsocket;
	CHAR *		lpBufSend; // Send Buffer
	CHAR *		pbBufRice; // Rice Buffer
	INT			iBufRice;
	INT			iSizeBuf;  // Dimensione del buffer di Send & Rice
	INT			iMessages; // Numero dei messaggi
	INT			iMessSize; // Dimensione dei messaggi
	INT			bProgressShow; // T/F se faccio vedere l'invio

} S_MAIL_STATUS;

/*

typedef struct {

	BOOL	bEngine; // TRUE/FALSE se il Fmailer è pronto
	BOOL	bLogWrite;
	BOOL	bTrace; // Traccia la comunicazione
	EH_LST	lstTrace;	// Lista dell'ultima comunicazione


	BOOL	fHelo;
	INT		iCheck;
	INT		iSMTPMode; // 0=RFC822 standard, 1=Con autentica

	CHAR *	lpSMTPServer;   // Nome del server SMTP (Trasmissione)
	CHAR *	lpSMTPUser;		// Solo con autentica
	CHAR *	lpSMTPPassword;   // Solo con autentica
	CHAR *	lpPOP3Server;   // Nome del server POP3 (Ricezione)
	CHAR *	lpUser;         // Nome dell'user
	CHAR *	lpPass;         // Passoword

	CHAR	*lpMyServer;   // Chi sono Es www.ferra.com

  //  WSADATA wsaData;
#ifdef __windows__
    struct	hostent *hPOP3;    // Host POP3
	struct	sockaddr_in aPOP3; // Socket per la comunicazione in POP3
    struct	hostent *hSMTP;    // Host SMTP
	struct	sockaddr_in aSMTP; // Socket per la comunicazione in SMTP
#endif
	INT		POP3socket;
	INT		SMTPsocket;
	CHAR *	lpBufSend; // Send Buffer
	CHAR *	lpBufRice; // Rice Buffer
	INT		iSizeBuf;  // Dimensione del buffer di Send & Rice
	INT		iMessages; // Numero dei messaggi
	INT		iMessSize; // Dimensione dei messaggi
    INT		(*SubControl)(INT,LONG,void *); // Sotto procedura dedicata al Paint
	INT		bProgressShow; // T/F se faccio vedere l'invio

	INT		iLastError;
	CHAR *	pLastError;
	INT		iSMTPLastCode;
	CHAR *	pszLastReceive;
	CHAR *	pszLastReceiveError;

} S_MAIL_STATUS;
*/

//INT FMailTo(EH_MAILTO *MailTo,CHAR *lpSMTPServer);
CHAR *	ehMailError(INT err);
INT		ehMail(EN_MESSAGE cmd,LONG lDato3,void *lpDato); // ex FMailServer
void	ehMailSetTrace(BOOL bTrace);
CHAR *	ehMailGetTrace(void);
#define ehMailStatus() (S_MAIL_STATUS *) ehMail(WS_INF,0,NULL)

INT QPToBinary(CHAR *lpPointer,INT *lpiSize);
INT BASE64ToBinary(CHAR *lpPointer,INT *lpiSize);

void *QPToText(CHAR *lpSorgDest,INT iByteChar);
void *BASE64ToText(CHAR *lpSorgDest,INT iByteChar);

// new 2008 (AntiSpamAssassins)
CHAR * ehBoundaryBuilder(void);
CHAR * ehHtmlToTextAlloc(CHAR *lpTestoFinito);
typedef enum {
	MIMEHTML_1=0,
//	MIMEHTML_2,
	TEXTPLAIN_1
} EN_MAILSENDTYPE;

INT	ehMailToEx(EH_MAILTO * psMailTo,
				EN_MAILSENDTYPE iType, // Se testo il messaggio viene preso in utf8
				BYTE * putfEmailFrom,
				BYTE * putfEmailReplyTo,
				BYTE * putfEmailTo,
				BYTE * putfEmailCC,
				BYTE * putfEmailBCC,
				INT   iCharSetBody, // SE_UTF8 / SE_ISO_LATIN1
				BYTE * putfSubject,		// Sempre UTF8
				CHAR * pszMessageBody,	// Corpo del messaggio
				CHAR * pszOtherSetting,
				UTF8 * pszFilesAttach,
				CHAR * pszTextMessageAlt	//  NULL > calcolato in automatico
				);

void	ehMailFromFree(S_MAILFROM *lpFMF); //  FMMailFromFree
CHAR *	ehMailLastRice(void); // 2011
void	ehMailAddr(FMAIL_ADDR *psAddr,CHAR *pStringSep);
void	ehMailFree(FMAIL_ADDR *psAddr);
void	ehMailTracePrint(void);