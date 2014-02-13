// -------------------------------------------------------------
// MAILTOOL  Tool per l'accesso diretto ai servizi di e-mail  
// by Ferr… Art & Technology             
//                      
// Created by G.Tassistro   (Aprile 2000)                      
// ATTENZIONE:
// Aggiungere WSOCK32.LIB
// -------------------------------------------------------------

// Tipo di decodifica
#define MIMEENCOD_NO  0
#define MIMEENCOD_QP  1 // Quote-printable 
#define MIMEENCOD_B64 2 // base64
#define MIMEENCOD_7BIT  3 // 7bit
#define MIMEENCOD_8BIT  4 // 8bit

// Dove si trova l'oggetto
#define MIMEOBJ_MEMORY 0
#define MIMEOBJ_FILE 1
#define MIMEOBJ_MULTI 2
#define MIMEOBJ_END 3

#define FMAIL_US_ASCII 0
#define FMAIL_MIME 1
#define FMAIL_MIME_SP 2 // Mime single part (new 2006)
#define FMAIL_DIRECT 1

typedef struct {
	SINT iType;			 // 0=memory (lpData=memoria), 1=File (lpData=Nome del file) MIMEOBJ_MULTI=Intestazione per Multiparti
	SINT iEncoding;		 // 0=Nessuna 1=Quoted-printable 2=base64
	CHAR *lpSource;		 // Sorgente
	SINT iSourceSize;	 // Solo se il contenuto è binario (iEncoding=base64)
	CHAR *lpContentType; // Es. application/msword; name="nome.doc" (E' OBBLIGATORIO)
	CHAR *lpDisposition; // Es. attachment; filename="nome.doc"
	CHAR *lpID;			 // un valore alfanumerico (facoltativo)
	CHAR *lpDescription; // Una descrizione (facoltativo)
	CHAR *lpBoundary;    // Definizione del boundary 
	SINT iParent;		 // Per le multidefinizioni alternative dice a chi fa riferimento
	//SINT iFirstChild;
} FMIMEPARTLINK;

typedef struct {
	SINT iType; // 0=memory (lpData=memoria), 1=File (lpData=Nome del file)
	CHAR *lpSource; // Puntatore al sorgente originale
	SINT iSourceSize; // Solo se il contenuto è binario
	SINT iEncoding; // 0=Nessuna 1=Quoted-printable 2=base64
	CHAR szContentType[255]; // Es. application/msword; name="nome.doc" (E' OBBLIGATORIO)
	CHAR szDisposition[255]; // Es. attachment; filename="nome.doc"
	CHAR szID[255]; // un valore alfanumerico (facoltativo)
	CHAR szDescription[255]; // Una descrizione (facoltativo)
	CHAR szBoundary[255]; // Definizione del boundary 
	SINT hdlConvert; // Usato in ricezione se >0 contiene la conversione in binario della parte MIME	
	SINT iParent;
	BOOL fError; // Se esiste un errore su questa parte
} FMIMEPARTMEMO;

typedef struct {
	SINT iSize;
	SINT iType; // 0=US-ASCII , 1=MIME MultiPart
	FMIMEPARTLINK MimePartLink;
	CHAR *lpFromName;   // Nome di chi manda il messaggio
	CHAR *lpFromEmail;  // E-mail di chi manda il messaggio
	CHAR *lpToName;     // Nome a cui va spedito il messaggio
	CHAR *lpToEmail;    // E-mail a cui va spedito il messaggio
	CHAR *lpSubject;    // Soggetto
	SINT  iMessLocation; // Dove si trova il messaggio da spedire (era iMessType)
	CHAR *lpMessage;    // Messaggio
	CHAR *lpDate;       // Data del messaggio (solo ricezione)

	CHAR *lpNotifyName;   // Notify:Nome di chi deve ricevere la notifica del messaggio
	CHAR *lpNotifyEmail;  // Notify:E-mail di chi deve ricevere la notifica del messaggio

	CHAR *lpToCCName;     // Nome a cui va spedito il messaggio
	CHAR *lpToCCEmail;    // E-mail a cui va spedito il messaggio

	CHAR *lpToBCCName;     // Nome a cui va spedito il messaggio
	CHAR *lpToBCCEmail;    // E-mail a cui va spedito il messaggio

	CHAR *lpSubjectEncode; // New 2007
	SINT iPriority;

} FMAILTO;

typedef struct {
	SINT iSize;
	CHAR *lpMessProcess; // Puntatore al messaggio originale da processare
	CHAR *lpDate;       // Data del messaggio 
	SINT iType; // 0=US-ASCII , 1=MIME MultiPart
	CHAR *lpFromName;   // Nome di chi manda il messaggio
	CHAR *lpFromEmail;  // E-mail di chi manda il messaggio
	CHAR *lpToName;     // Nome a cui va spedito il messaggio
	CHAR *lpToEmail;    // E-mail a cui va spedito il messaggio
	CHAR *lpSubject;    // Soggetto
	SINT  iMessType;	// Se in memoria o su disco
	CHAR *lpMyServer;   // Chi sono
	CHAR *lpUSMessage;	// Puntatore al messaggio semplice non MIME
	DRVMEMOINFO *lpMIME; // Puntatore a zona Mime per la lettura delle parti (FMIMEPARTMEMO)
} FMAILFROM;

typedef struct {
	BOOL	bEngine; // TRUE/FALSE se il Fmailer è pronto

	BOOL	fHelo;
	SINT	iCheck;
	SINT	iSMTPMode; // 0=RFC822 standard, 1=Con autentica

	CHAR	*lpSMTPServer;   // Nome del server SMTP (Trasmissione)
	CHAR	*lpSMTPUser;		// Solo con autentica
	CHAR	*lpSMTPPassword;   // Solo con autentica
	CHAR	*lpPOP3Server;   // Nome del server POP3 (Ricezione)      
	CHAR	*lpUser;         // Nome dell'user
	CHAR	*lpPass;         // Passoword

	CHAR	*lpMyServer;   // Chi sono Es www.ferra.com

  //  WSADATA wsaData;
    struct	hostent *hPOP3;    // Host POP3
	struct	sockaddr_in aPOP3; // Socket per la comunicazione in POP3
    struct	hostent *hSMTP;    // Host SMTP
	struct	sockaddr_in aSMTP; // Socket per la comunicazione in SMTP
	SINT	POP3socket;
	SINT	SMTPsocket;
	CHAR	*lpBufSend; // Send Buffer
	CHAR	*lpBufRice; // Rice Buffer
	SINT	iSizeBuf;  // Dimensione del buffer di Send & Rice
	SINT	iMessages; // Numero dei messaggi
	SINT	iMessSize; // Dimensione dei messaggi 
    SINT	(*SubControl)(SINT,LONG,void *); // Sotto procedura dedicata al Paint
	SINT	bProgressShow; // T/F se faccio vedere l'invio

} FMAILINFO;

SINT FMailTo(FMAILTO *MailTo,CHAR *lpSMTPServer);
CHAR *FMailError(SINT err);
SINT FMailServer(SINT cmd,LONG lDato3,void *lpDato);
SINT QPToBinary(CHAR *lpPointer,SINT *lpiSize);
SINT BASE64ToBinary(CHAR *lpPointer,SINT *lpiSize);
CHAR *QPToText(CHAR *lpSorgDest);
CHAR *BASE64ToText(CHAR *lpSorgDest);

// new 2008 (AntiSpamAssassins)
CHAR *FmBoundaryBuilder(void);
CHAR *FmHtmlToTextAlloc(CHAR *lpTestoFinito);
SINT FmMimeHtmlSend(FMAILTO *psMailTo,
					SINT iType,
					CHAR *lpEmailFrom,
					CHAR *lpEmailTo,
					CHAR *lpEmailNotify,
					CHAR *pszHtmlSubject,
					CHAR *pszHtmlMessage
					);

