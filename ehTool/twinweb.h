// ----------------------------------------------------
// | TWINWEB 
// | Api per l'elaborazione dei file TWP
// |                                           
// |                                           
// |					 by Ferrà Art & Tecnology 2000
// ----------------------------------------------------

typedef enum {

	TW_VERSION,//    0 // Versione del TwinWeb
	TW_USER ,//      1 // Dati per il riconoscimento e verifica dell'utente
	TW_ADMINEMAIL,// 2 // Email dell'amministratore
	TW_FERRAEMAIL,// 3 // Email della ferrà           
	TW_ADBIMPORT,//  4 // Importa un file ADB
	TW_ADBDELETE,//  5 // Cancella un record di un ADB
	TW_FILEDELETE,// 6 // Cancella un File
	TW_FILECOPY,//   7 // Copia un file 
	TW_PROCEDURE,//  8 // Procedura che gestisce il controllo pre/post acquisizione
	TW_TWCCOPY,//    9 // Copia del file TWC
	TW_ADBDELALL,//  10 // Cancella tutte le voci nel DB
	TW_ADBDELEX,//   11 // Cancella un record di un ADB Estesa (!)
	TW_FILEPRO=14, // Allego un file da processare
	TW_COMMAND,//    15 // Comando
	TW_ADBHIDDEN,//  16 // Nascondi

} TW_CMD;


typedef struct {
	INT iType;
	CHAR szCode[200];
	CHAR szFile[400];
	CHAR szNewFileName[20];
} TW_OBJECT;

typedef struct {
  CHAR szUser[30];
  CHAR szPassword[30];
  CHAR szEmailAssociato[100];
  CHAR szEmailFerra[100];
} TW_KEY;

typedef struct {
  CHAR szVer[10]; // Versione
  INT iFile; // File collegati
  LONG iSize; // Grandezza del file
} TWP_HEADER;

typedef struct {
  INT iNameLen;
  INT iFileLen;
} TWP_OBJECT;

#define TWC_FTPFILE 0
#define TWC_TIMER 1

typedef struct {

  INT		iMode; // Modo di chiamata
  CHAR		szTempFolder[500]; // Folder dove è stato espanso il TWP
  CHAR		szFtpPath[500];  // Folder dove è arrivato il TWP
  CHAR		szTWPFile[500];  // Nome originale del TWP
  BOOL		fLog;
  HWND		wndProgress;
  HWND		wndMessage;
  CHAR		szSMTPServer[128];
  CHAR		szPOP3Server[128];

  CHAR		szError[500];
  EH_LST	lstFiles;

} TWP_EXPAND;

//#define LPTWEXPANDER BOOL *(*)(TWEXPAND *,CHAR *)

typedef BOOL (FAR TWEXPANDER)(
    TWP_EXPAND * lpTwExpand,
	CHAR *lpsz);
typedef TWEXPANDER FAR *LPTWEXPANDER;

//#define L_MAXTIME 48
typedef struct {
	CHAR szCode[20];
	CHAR szDayTimer[8];
	CHAR szHourTimer[49];
	//INT iOra[L_MAXTIME];
	//INT iMin[L_MAXTIME];
	INT iLastOra;
	INT iLastMin;
	INT iLastDay;
	CHAR szLastDate[20];
} TMFIELD;

//INT TWServer(INT cmd,LONG info,void *str);
INT	TWAsciiToObject(CHAR *lpBuffer,TW_OBJECT *lpTWObject);
void	TWObjectToAscii(CHAR *lpBuffer,TW_OBJECT *lpTWObject);

CHAR *	twpExpand(TWP_EXPAND * psTwpExpand);
BOOL	twpFreeResource(TWP_EXPAND *psTwpExpand);

INT TWServer(INT cmd,LONG info,void *str);
