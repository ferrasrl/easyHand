//-------------------------------------------
// eh_core.h
// Easyhand core header
//
// Ferrà Art Technology 1996
// Ferrà srl 2006-2008
//
// ==> KERNEL SECTION
//   --> MATH SECTION
// ==> NUMBER SECTION
// ==> MEMORY SECTION
// ==> STRING SECTION
// ==> TASK SECTION
// ==> AUDIO SECTION
// ==> ULT SECTION (Translator)
// ==> FILE SECTION
//
// ==> SERIAL COM SECTION (RS232) (optional RS_COM MACRO)
//
// only not console (!EH_CONSOLE)
//
// ==> IN SECTION (human interface for input)
// ==> FONT SECTION
// ==> GDI SECTION
// ==> WIN SECTION
// ==> OBJ SECTION
// ==> TEXT INPUT SECTION
//
// ==> PRINTER SECTION (EH_PRINT)
//
//          by Ferrà Art & Technology 1996
//          by Ferrà srl 2007
//          by Ferrà srl 2008
//-------------------------------------------


#if (!defined(EH_COM)&&!defined(EH_COMPLETED)&&!defined(EH_PRINT)&&!defined(EH_CONSOLE)&&!defined(EH_MOBILE)&&!defined(EH_FASTCGI))
#error Easyhand error. richiesto almeno un EH_<macro>
#endif

#if (!defined(_WIN32)&&!defined(__linux__)&&!defined(__APPLE__)&&!defined(__marmalade__))
#error Easyhand error. Piattaforma/OS sconosciuti.
#endif

#if  (defined(EH_CONSOLE)&&!defined(_CONSOLE)&&defined(_WIN32)) 
#define _CONSOLE
#endif

// Modalità GDI di window
#if (!defined(EH_CONSOLE)&&!defined(EH_MOBILE))
#define EH_WIN_GDI
#endif

// =================================================================================
//                                                                                 =
// ==> KERNEL SECTION                                                              =
//                                                                                 =
// =================================================================================

//
//	Inizializzo in base al sistema operativo
//

//
// Windows 32bit < ########################################################################
//
#if defined(_WIN32)

    #define __windows__

    #ifndef _CRT_SECURE_NO_WARNINGS
     #define _CRT_SECURE_NO_WARNINGS
     #define _CRT_SECURE_NO_DEPRECATE
    #endif

    #ifdef _WIN_0501		// Allow use of features specific to Windows XP or later.
        #define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
    #endif

	#ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
	#endif

    #include <winsock2.h>
    #include <windows.h>
    #include <windowsx.h>
#ifdef __cplusplus
//	#include <shobjidl.h> 
//	#include <atlbase.h> // Contains the declaration of CComPtr.
#endif
    #include <commctrl.h>
//    #include <tchar.h>
    #include <stdio.h>

    #ifndef EH_MOBILE
        #include <direct.h>
    #endif

    #ifdef EH_CONSOLE
        #include <conio.h>
        #include <Wincon.h>
        #define _EH_DEBUGPOINT_ printf("#>%s:%d" CRLF,__FILE__,__LINE__);
    #endif

	#ifndef __STDARG_H
        #include <stdarg.h>
	#endif

	#ifdef _CRT_SECURE_NO_DEPRECATE

        #define strlwr _strlwr
        #define strupr _strupr
        #define chdir _chdir
        #define fcloseall _fcloseall
		#define snprintf sprintf_s
	#endif

	#ifdef _UNICODE
        #define TEXTSIZE(x) (x*2)
        #define _TagExchange TagExchangeUnicode
	#else
        #define TEXTSIZE(x) x
        #define _TagExchange TagExchange
	#endif
    typedef signed __int64		INT64;
    typedef unsigned __int64	UINT64;
	typedef __time64_t TIME64;
	#define strLwr(a) strlwr(a)
	#define strUpr(a) strupr(a)
    #define wcsLwr(a) _wcslwr(a)
	#define wcsUpr(a) _wcsupr(a)

	#define _BREAK_POINT_ __asm { int 3 }

#endif


//                                       =
// LINUX	< ########################################################################
//                                       =
#ifdef __linux__
    #define _GNU_SOURCE
#endif

//                                       =
// APPLE OS/X  < ########################################################################
//                                       =
#ifdef __APPLE__

	#define __apple__
    #define _GNU_SOURCE

	//  #include <Carbon/Carbon.h>
	//	#define TEXT(a) (a)

#endif


//                                       
// GNU_SOURCE < ########################################################################
//                                       

#if defined(_GNU_SOURCE)

	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include <stddef.h>
    #include <sys/stat.h>
	#include <sys/types.h>
	#include <dirent.h> // Directory
	#include <errno.h>
	#include <signal.h>
	#include <unistd.h>
    #include <stdarg.h>
    #include <ctype.h>
    #include <wchar.h>
	
	//
    // Versione FTS
    //
    #define DIROPEN_FTS 1
    #ifdef DIROPEN_FTS
    #include <fts.h>
    #endif

	typedef signed char			BOOL;
	typedef signed short int	WORD;
	typedef signed int			LONG;
	typedef char				CHAR;
	typedef unsigned char		BYTE;
	typedef char				TCHAR, *PTCHAR;
	typedef unsigned long		DWORD;
	typedef unsigned int 		UINT;
	typedef unsigned int 		ULONG;
	typedef signed int			INT;
	typedef wchar_t 		    WCHAR;
	typedef void *				HANDLE;

	#define TEXT(a) (a)
    #define TRUE   1
    #define FALSE  0

	typedef LONG				HWND;
	typedef int					HDC; // Da vedere
	typedef struct {int x; int y;} POINT;
	typedef struct {int left; int top; int right; int botton;} RECT;
	typedef struct {int cx; int cy;} SIZE;

    typedef int64_t	INT64;
    typedef u_int64_t UINT64;

#ifdef __linux__
	typedef __time_t TIME64;
#else
	typedef time_t TIME64;
#endif

	
	typedef size_t SIZE_T;
//	#define	ehGetch getchar

    CHAR * strLwr(CHAR *);
    CHAR * strUpr(CHAR *);
    WCHAR * wcsLwr(WCHAR *);
    WCHAR * wcsUpr(WCHAR *);

	#define LOWORD(l)           ((WORD)(((l) & 0xffff)))
	#define HIWORD(l)           ((WORD)(((l) >> 16) & 0xffff))
	#define LOBYTE(w)           ((BYTE)((w) & 0xff))
	#define HIBYTE(w)           ((BYTE)((w) >> 8) & 0xff)
	#define GetRValue(rgb)      (LOBYTE(rgb))
	#define GetGValue(rgb)      (LOBYTE(((WORD)(rgb)) >> 8))
	#define GetBValue(rgb)      (LOBYTE((rgb)>>16))
	#define RGB(r,g,b)          ((EH_COLOR)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#endif

//                                       
// MARMALADE < ########################################################################
//                                       

#ifdef __marmalade__
	#include <stdlib.h>
	#include <stdio.h>
	#include <string.h>
	#include <stddef.h>
    #include <sys/stat.h>
	#include <sys/types.h>
	#include <dirent.h> // Directory
	#include <errno.h>
	#include <signal.h>
	#include <unistd.h>
    #include <stdarg.h>
    #include <ctype.h>
 //   #include <wchar.h>

	typedef signed char			BOOL;
	typedef signed short int	WORD;
	typedef signed int			LONG;
	typedef char				CHAR;
	typedef unsigned char		BYTE;
	typedef char				TCHAR, *PTCHAR;
	typedef unsigned long		DWORD;
	typedef unsigned int 		UINT;
	typedef unsigned int 		ULONG;
	typedef signed int			INT;
	typedef wchar_t 		    WCHAR;
	typedef void *				HANDLE;

	#define TEXT(a) (a)
    #define TRUE   1
    #define FALSE  0

	typedef LONG				HWND;
	typedef int					HDC; // Da vedere
	typedef struct {int x; int y;} POINT;
	typedef struct {int left; int top; int right; int botton;} RECT;
	typedef struct {int cx; int cy;} SIZE;

    typedef int64_t	INT64;
    typedef u_int64_t UINT64;

#ifdef __linux__
	typedef __time_t TIME64;
#else
	typedef time_t TIME64;
#endif

	
	typedef size_t SIZE_T;
//	#define	ehGetch getchar

    CHAR * strLwr(CHAR *);
    CHAR * strUpr(CHAR *);
    WCHAR * wcsLwr(WCHAR *);
    WCHAR * wcsUpr(WCHAR *);

	#define LOWORD(l)           ((WORD)(((l) & 0xffff)))
	#define HIWORD(l)           ((WORD)(((l) >> 16) & 0xffff))
	#define LOBYTE(w)           ((BYTE)((w) & 0xff))
	#define HIBYTE(w)           ((BYTE)((w) >> 8) & 0xff)
	#define GetRValue(rgb)      (LOBYTE(rgb))
	#define GetGValue(rgb)      (LOBYTE(((WORD)(rgb)) >> 8))
	#define GetBValue(rgb)      (LOBYTE((rgb)>>16))
	#define RGB(r,g,b)          ((EH_COLOR)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#endif


	//
	// Easyhands' type
	//
	typedef signed int   	    SINT;
	typedef signed int   	    INT32;
	typedef signed short int	INT16;
	typedef char 		UTF8;
	typedef char *		PUTF8;
	typedef int			HMEM;
	typedef CHAR **		EH_AR;
	typedef CHAR **		EH_ARF;
	
	typedef struct {double x; double y;} POINTD;
	typedef struct {double left; double top; double right; double bottom;} RECTD;
	typedef struct {double cx; double cy; } SIZED;

//
//	KERNEL's macro
//
	#define ON 	-1
	#define OFF  0
	#define SEMIOFF 2 // Per eredit‡ con il passato
	#define STOP      127
	#define TXTSTOP -1
	#define CR 13
	#define LF 10
	#define ESC 27
	#define CRLF "\r\n"
	#define LCRLF L"\r\n"
	#ifndef true
	#define true 1
	#endif
	#ifndef false
	#define false 0
	#endif

#define ZeroFill(p) memset(&p,0,sizeof(p))
#define _(p) memset(&p,0,sizeof(p))

// Boolean
#define SET 0
#define AND 1
#define OR  2
#define XOR 3

// Directions
#define UP    10
#define DOWN  11
#define SX    12
#define DX    13

typedef enum {
	ROT_NONE,	// 0
	ROT_90,		// 1 : ex ROT_90A
	ROT_180,	// 2 : 
	ROT_270		// 3 : ex ROT_90H
} EN_ROT;
/*
// Messages
#define APRI   20
#define CHIUDI 21
#define WS_COUNT  22
#define LOAD   30
#define CAMBIO 31
#define ADBHDB 32
#define ADBIDX 33
#define ADBDSP 34
#define ADBREC 35
#define ADBFUNZ 40

#define WS_OPEN    APRI
#define WS_CLOSE   CHIUDI
#define WS_LOAD    LOAD
#define WS_CHANGE  CAMBIO
#define WS_HDB     ADBHDB
#define WS_IDX     ADBIDX
#define WS_EXTDSP  ADBDSP
#define WS_EXTFUNZ ADBFUNZ
#define WS_EXTREC  ADBREC
*/
typedef enum {

	WS_INF=    	  0, 	// Richiede informazioni sulla lista
	WS_BUF=    	  1, 	// Richiede il buffer (lista campi)
	WS_OFF=    	  2, 	// Setta l'offset
	WS_SEL=    	  3, 	// Setta la selezione del campo
	WS_PTREC=  	  4,		// Ritorna il puntatore al record del campo selezionato
	WS_ISEL=   	103, 	// Come sopra usati in obj_listset
	WS_IPTREC= 	104, 	// Come sopra usato in obj_listget
	WS_FIND=   	  5,		// Ricerca una chiave
	WS_FINDKEY=	 15,		// Ricerca una tasto premuto
	WS_REFON=  	  6,		// Setta il refresh a ON
	WS_REFOFF=    7,		// Setta il refresh a OFF
	WS_DISPLAY=   8,		// Chiamata con il tipo scroll a display
	WS_DBSEEK=    9,		// Spostamento in + e -
	WS_DBPERC=   10,		// Spostamento in percentuale
	WS_FIRST=      11,		// Chiede il primo record alla funzione Ext
	WS_LAST=       13,		// Comando di fare (qualcosa)    (New 98)
	WS_FILTER=     12,		// Controlla che sia un record valido
	WS_SETFILTER=  14,		// Setta il numero di filtro scelto
	WS_SETFLAG=	   16,		// Ricerca una tasto premuto
	WS_REALGET=    18,   // Legge il puntatore REALE (solo ADB)
	WS_REALSET=    19,   // Setta il puntatore REALE (solo ADB)

	WS_OPEN=	   20,
	WS_CLOSE=	   21,
	WS_LOAD=	   30,
	WS_CHANGE=	   31,
	WS_HDB=		   32,
	WS_IDX=		   33,
	WS_EXTDSP=	   34,
	WS_EXTREC=	   35,
	WS_EXTNOTIFY=  40,
//	WS_EXTFUNZ=    40,
	WS_GET=		   41,		// GET - Legge (2010)
	WS_SAVE=	   45,		// new 2011
	WS_ICONE=      52,   // Richiesta di icone collegata

	WS_DRAG=      100,	// Comando di DRAG  (new 98) Prendo oggetto
	WS_DROP=      101,		// Comando di DROP  (new 98) Rilascio oggetto
	WS_DROPFOCUS= 102,		// Comando di DROP  (new 98) Focus su oggetto con click premuto
	WS_DROPBLUR=  103,		// Comando di DROPBLUR  (2008) perdita del BLUR

	WS_EVENT=	  199,		// Evento (new 2008)
	WS_KEYPRESS=  200,		// Ricerca una tasto premuto
	WS_ADD=       201,		// Aggiungi Elemento
	WS_DEL=       202,		// Cancella Elemento
	WS_UPDATE=    203,		// Update Elemento
	WS_INSERT=    204,		// Inserisci un Elemento
	WS_SEND=	  205,		// Invia (2010)
	WS_COUNT=     210,		// Ritorna il numero di elementi (New 98)
	WS_LINK=      211,		// Ricerca un collegamento       (New 98)
	WS_DO=        212,		// Comando di fare (qualcosa)    (New 98)
	WS_PROCESS=   215,		// Comando di fare (qualcosa)    (New 98)

	WS_DOLIST=    216,	// Crea la lista di puntatori    (New 99)
	WS_ADBFILTER= 217,	// Chiede di usare l'adbfilter   (New 99)
	WS_FINDLAST=  218,	// Cerca l'ultimo                (New 99)
	WS_CLONE=     219,	// Chiede una clonazione         (New 99)
	WS_LINEVIEW=  220,	// Cambiamento di grandezza in visione (New99) Display dinamico
	WS_LINEEDIT=  221,	// Cambiamento di grandezza in visione (New99) Display dinamico
	WS_SETTITLE=  222,    // Abilitazione/Disabilitazione dei titoli fields scroll (New99)
	WS_SETHRDW=   223,    // Redraw orizzontale in caso di variazione <--> scroll  (New99)
	WS_CLEAR=	  224,	  // new 2011
	WS_START=     300,    // Inizializza il driver      (new2000)
	WS_CHECK=     301,    // Controllo (new2002)
	WS_LOCK=	  302,    // Controlla il blocco del driver
	WS_RELOAD=    303,    // Chiedi un reload dei dati (new 2003)
	WS_READY=     304,    // New 2009 (verifica se l'oggetto è pronto)

	WS_PRINT=     400,    // Richiesta di stampa (new 2005)

	WS_CREATE=	  410,	// Richiesta di creazione (Inviata Una tantum all'apertura dell oggetto) viceversa WS_KILL in chiusura
	WS_DESTROY=   411,	// Comunico la fine uso driver   (New 99) WS_KILL
	WS_MOVE=	  413,    // L'oggetto Ë in posizione differente
	WS_SIZE=	  415,    // L'oggetto ha dimensioni differenti
	WS_SET_TEXT,		//  Cambio di un testo su un oggetto (2010)
	WS_SET_FONT,		//  Cambio del font (2010)

// New 2008 usato negli scroll testuali

	WS_CODENAME=         501,  // Nome del campo che si usa come codice di ricerca (Es. CODICE)
	WS_SET_ROWFOCUS=     502,  // Valore del codice attualmente in uso
	WS_GET_ROWFOCUS=     503,  // Chiede se la riga è selezionata
	WS_SET_ROWSELECTED=  504,  // Valore del codice attualmente in uso
	WS_GET_ROWSELECTED=  505,   // Chiede se la riga è selezionata
	WS_GET_SELECTED,			   // Ritorna una struttura S_RET con i valore selezionato
	WS_SUBPARAM

} EN_MESSAGE; // <-- What Standard Message

typedef enum {
	EXT_PREV,
	EXT_AFTER,
	EXT_CALL
} EN_EXT_NOTIFY;

#define EH_SRVPARAMS EN_MESSAGE enMess,LONG lParam,void *pVoid

#ifndef EH_CONSOLE
 #define EH_OBJPARAMS struct OBJ * psObjCaller,EH_SRVPARAMS
 #define EH_NOTIFYPARAMS EH_OBJ * pObj,BOOL * pbReturn, EN_EXT_NOTIFY enMode, HWND hWnd, UINT message, LONG wParam, LONG lParam

#endif

// Stampa con posizionamento New 2000
typedef enum {

	DPL_DEFAULT,
	DPL_LEFT=0x0001,	// 0=Default è left
	DPL_CENTER=0x0002,
	DPL_RIGHT=0x0004,
	DPL_JUSTIFY=0x0008,

	DPL_TOP=0x0010,
	DPL_MIDDLE=0x0020,
	DPL_BOTTOM=0x0040,

	DPL_BASELINE=0x0100,
	DPL_HEIGHT_TYPO=0x0200

} EN_DPL;

typedef enum {

	PSR_X_ABS	=0x0001,		// La posizione indicata è assoluta (x=x)
	PSR_X_REL	=0x0002,		// La posizione indicata è relativa (x = xOriginal+x)
	PSR_X_ABSEND=0x0003,		// La posizione indicata è assoluta (x = (max x Container) -x) 

	PSR_Y_ABS	=0x0100,		
	PSR_Y_REL	=0x0200,		
	PSR_Y_ABSEND=0x0300,	

	PSR_CX_REL	=0x0010, 
	PSR_CX_ABS	=0x0020, 
	PSR_CY_REL	=0x1000, 
	PSR_CY_ABS	=0x2000 

} EN_POSREF;

#define PSR_ABS PSR_X_ABS|PSR_Y_ABS
#define PSR_REL PSR_X_REL|PSR_Y_REL

typedef struct {

	double dRed;
	double dGreen;
	double dBlue;
	double dAlpha;

} EH_COLORD;
#define EH_COLOR DWORD
#define EH_COLOR_BLACK sys.arsColor[0]
#define EH_COLOR_RED sys.arsColor[11]
#define EH_COLOR_WHITE sys.arsColor[15]
#define EH_COLOR_BLUE RGB(0,0,255)
#define EH_COLOR_GREEN RGB(0,255,0)
#define EH_COLOR_YELLOW RGB(255,255,0)
#define EH_COLOR_ORANGE RGB(255,0x60,0)
#define EH_COLOR_TRASPARENT -1

typedef DWORD EH_ACOLOR;	// Alpha Color

#define wordAlign(num) ((num+1)>>1)<<1; // WordAlign
#define dwordAlign(num) ((num+3)>>2)<<2; // DoubleWordAlign
// WS_KILL 999 Eliminato

//
// Definizioni Standard di tipi di dati/campo
//

	typedef enum {
		_UNKNOW=-1,
		_ALFA=0, // Alfanumerico testo fisso
		_NUMBER=1, // Numero con decimali
		_TEXT=2, // Testo dinamico
		_DATE=3, // Date (testo): YYYYMMDD
		_PASSWORD=5, // Password
		_INTEGER=10, // Intero (numero senza decimali.. può essere 16/32/64 bit)
		_BOOL,		// Vero/Falso 1 bit
		_ID,	// Campo intero ad autoincremento
		_BINARY,	// Binario 
		_GEOMETRY, // Geometrico
		_POINT, // Punto Geometrico
		_TIME // Time (Tempo : default Local)
	} EH_DATATYPE;

//
// x compatibilità: Da vedere
//
#define	ALFA _ALFA
#define	NUME _NUMBER
#define	DATA _DATE
#define NOTE _TEXT
#define	NUMS 4
#define	APSW _PASSWORD
#define	NUMZ 6
#define	ALFU 7

//#include "/easyhand/ehtool/main/dmiutil.h"


#ifdef __cplusplus
extern "C" {
#endif

	//
	// 2011
	//
	typedef struct {
		
		BOOL bAllocated;
		EH_DATATYPE enDaTy;
		union {
			LONG	lValue;
			DWORD	dwValue;
		};
		union {
			void *	pVoid;
			BYTE *  pszString;
			WCHAR * pwcString;
		};
		double		dValue;
		
	} S_UNIVAL; // Universal return

	typedef struct {

		DWORD *	pdwBuffer;
		DWORD	dwSize;			// Massima dimensione
		DWORD	dwRead;			// Puntatore alla lettura
		DWORD	dwWrite;		// Puntatore alla scrittura
		DWORD	dwLength;		// Lunghezza della coda
	
		HANDLE	hMutex;

	} S_CIRCLE_CUE;
	
	typedef S_CIRCLE_CUE * PS_CC;
	PS_CC	ccCreate(DWORD dwSize,BOOL bMT);
	BOOL	ccPut(PS_CC psCC,DWORD);
	BOOL	ccPush(PS_CC psCC,DWORD dwData);
	DWORD	ccGet(PS_CC psCC,BOOL  * pbEmpty);
	INT		ccGetBytes(PS_CC psCC,BYTE * pbBuffer,INT iSize);
	DWORD	ccCopy(PS_CC psCC,DWORD * pdwBuffer,DWORD dwSize,BOOL bLifo);
	BOOL	ccEmpty(PS_CC psCC);
	DWORD	ccLength(PS_CC psCC);
	void	ccClear(PS_CC psCC);
	PS_CC	ccDestroy(PS_CC);

/*
			 CHAR *		ptbuf;  // Puntatore al buffer locale
//			 INT		hdlbuf;  // Handle del buffer locale
			 WORD		buflen;  // Grandezza del buffer locale
			 WORD		bufstart; // Puntatore a coda circolare di start e di end
			 WORD		bufend;
*/




	//
	// MEMO ELEMENT
	//

	// Macro
	typedef enum {
		M_HDLFREE,
		M_HEAP,
		M_MOVEABLE,
		M_LOCKED,
		M_AUTO
		} EH_MEMO_TYPE;// M_HEAP+128

    // Per compatibilità
//    #define M_MOVEABLE M_MOVEABLE
//    #define M_MOVEABLE M_MOVEABLE
//    #define M_FIXED M_LOCKED
    #define RAM_AUTO M_AUTO
	#define MEMOUSER_SIZE 16

	typedef struct {
			EH_MEMO_TYPE   iTipo; 	  // RAM_????
			void    *lpvMemo; // Puntatore far alla memoria
			SIZE_T   dwSize;   // Dimensioni
#ifdef __windows__
			HGLOBAL hGlobal;  // Handle Global
#endif
			CHAR	User[MEMOUSER_SIZE];
			} EH_MEMO_ELEMENT;

	//
	// Debug della memoria
	//
	#ifdef EH_MEMO_DEBUG

		#define EMD_END_FUNC ,CHAR * pszProg,INT iLine
		typedef struct {
			void *pMemo;
			CHAR *pszProgram;
			INT iLine;
			SIZE_T dwSize;
		} S_ALLOC_CONTROL;
		// void MemoDebugShow(void);
		void MemoDebugSet(BOOL bSet);
		void memoDebugOutput(FILE * ch,BOOL bBreak);

	#else
		#define EMD_END_FUNC

	#endif


#ifndef EH_MEMO_DEBUG
	S_UNIVAL * valCreate(EH_DATATYPE enDaTy,LONG lValue,void *pVoid);
	S_UNIVAL * valCreateNumber(double dValue);
#else
	S_UNIVAL * _valCreate(EH_DATATYPE enDaTy,LONG lValue,void *pVoid EMD_END_FUNC);
	S_UNIVAL * _valCreateNumber(double dValue EMD_END_FUNC);
	#define	valCreate(a,b,c) _valCreate(a,b,c,__FILE__,__LINE__)
	#define	valCreateNumber(a) _valCreateNumber(a,__FILE__,__LINE__)
#endif
S_UNIVAL * valDup(S_UNIVAL * psRet);
void valPrint(S_UNIVAL *);
void IValDestroy(S_UNIVAL **);
#define valDestroy(a) IValDestroy(&a);


//
//  DMI Direct Memory Indexed
//

typedef struct  {
	 BOOL   Reset;
	 INT    Hdl;
	 union	{
		DWORD   Size;
		DWORD	dwSize;
	 };
	 union	{
		LONG	Max;
		INT		iLengthMax;
	 };
	 union {
		LONG	Num;
		INT		iLength;
	 };
	 void *	pVoid;	// Puntatore all'handle in caso di lock

 } _DMI;

#define DRVMEMOINFO _DMI
#define DMIRESET {-1,-1,0,0,0}

void	DMIReset(_DMI *pdmi);
void	DMIOpen(_DMI *DRVinfo,EH_MEMO_TYPE enMemoType,LONG Max,INT Size,CHAR *Nome);
void	DMIAppendDyn(_DMI *DRVinfo,void *Dato); // Only 32bit
void	DMIInsertDyn(_DMI *DRVinfo,LONG Pos,void *Dato);
void	DMIDelete(_DMI *DRVinfo,LONG Pos,void *Dato);
void	DMIInsert(_DMI *DRVinfo,LONG Pos,void *Dato);
void	DMIClose(_DMI *DRVinfo,CHAR *Nome);

// New 2010
void *	DMILock(_DMI *pDmi,BOOL *pbUnLock);
void	DMIUnlock(_DMI *pDmi);
void	DMIMixing(_DMI *pdmi);
INT	DMIMove(_DMI * pDmi,INT idxFrom,INT idxTo,INT iCount); // new 2011

// New 2006
CHAR *	DMIDebug(CHAR *,LONG);

// New 2003
void DMIReadEx(_DMI *DRVinfo,LONG Pos,void *Dato,CHAR *lpWho);
void DMIWriteEx(_DMI *DRVinfo,LONG Pos,void *Dato,CHAR *lpWho);

#ifdef _DEBUG
#define DMIAppend(a,b) DMIAppendEx(a,b,DMIDebug(__FILE__,__LINE__))
#define DMIRead(a,b,c) DMIReadEx(a,b,c,DMIDebug(__FILE__,__LINE__))
#define DMIWrite(a,b,c) DMIWriteEx(a,b,c,DMIDebug(__FILE__,__LINE__))
#else
#define DMIAppend(a,b) DMIAppendEx(a,b,NULL)
#define DMIRead(a,b,c) DMIReadEx(a,b,c,NULL)
#define DMIWrite(a,b,c) DMIWriteEx(a,b,c,NULL)
#endif

void DMIAppendEx(_DMI *DRVinfo,void *Dato,CHAR *lpWho);
void DMISort(_DMI *DRVinfo, int (*compare )(const void *elem1, const void *elem2 ));

// New 9/2007
BOOL DMIBinaryFind(_DMI *pDmi,void *pElement,INT iSize,INT *pIndex,BOOL bCaseUnsensitive);

typedef struct  {
	 INT Id;
	 CHAR IdName[15];
 } IDARRAY;


//
//  FONT
//

typedef struct			// Usato per compatibilit‡ con i vecchi font
{
	CHAR *pOldName;
	CHAR *pFontFace;
	CHAR *listHeight;
	BOOL iStyles;
	BYTE **arNfiToAlt;
	INT iMaxNfi;

} EH_FONT_OLD;

typedef struct {

	INT		iType;			// 0= Vecchio tipo con nfi fissi
	CHAR *	pszFontFace;	// Nome reale nel sistema operativo
	INT		iHeight;			// Altezza di trasformazione in base all'nfi style
	INT		iStyles;			// Altezza di trasformazione in base all'nfi style
	INT		iCharExtra;	// Spazio extra tra i caratteri

	#ifdef __windows__		// windows 32
	HFONT	hFont;
	#endif

//	DWORD	dwCount;
	BOOL	bLock;		// T/F non è "bloccato" non è liberabile automaticamente
	INT		idx;		// Id del font allocato > Simulazione per compatibilità con il passato (array)

} EH_FONT;

typedef enum {

	STYLE_NORMAL=0x0,
	STYLE_BOLD=0x10,
	STYLE_ITALIC=0x20,
	STYLE_UNDERLINE=0x40

} EH_TSTYLE;
//
//	MOUSE EVENT
//
#ifndef EH_CONSOLE

typedef enum {
		EE_NONE=0,
		EE_LBUTTONDOWN,
		EE_LBUTTONUP,
		EE_RBUTTONDOWN,
		EE_RBUTTONUP,
		EE_LBUTTONDBLCLK,
		EE_RBUTTONDBLCLK,
		EE_MOUSEWHEEL,
//		EE_MOUSEWHEELUP,
//		EE_MOUSEWHEELDOWN,
		EE_MOUSEOVER=16,
		EE_MOUSEOUT,
		EE_MOUSEMOVE,
		EE_CHAR=32,
		EE_OBJ=64,
		EE_FOCUS=0x1000,
		EE_BLUR
} EN_EHEVENT;

// Easyhand Event
typedef enum {
	DD_NONE=0,
	DD_DRAGREADY, // 1=Drag fissato (ma ancora da partire),
	DD_DRAGDROP,  // 2-Drag & Drop partito,
	DD_READY	  // 3=Drag & Drop Ready Fissato il punto
} EN_DRAGDROP;

typedef struct {
	EN_DRAGDROP enDragStatus;	// 0= No, 1=Drag fissato (ma ancora da partire), 2-Drag & Drop partito, 3=Punto di Drop Fissato DD_
	BOOL	bDragNotify;	// T/F se è stato notificato all'oggetto di Drag che è stato selezionato (notifico dopo un po)
	POINT	sDragPointAbsolute; // Punto assoluto di Drag

	//INT iDragType;			// Punto di Drag
	void *	pDragPtr;
	POINT	sDragPoint;
	INT		iDragParam;

	//INT iDropType;			// Punto di Drop
	void *	pDropPtr;
	POINT	sDropPoint;
	INT		iDropParam;

} EH_DRAGDROP;

typedef struct {
	//BOOL fActive;
	EN_EHEVENT	iEvent;		// EH_LBUTTON??

	INT		iWin;		// Finestra di appartenenza (numero di easyhand)
	BOOL	bObj;		// Evento associato ad un oggetto
	HWND	hWnd;		// windows di appartenenza dell'evento
	POINT	sPoint;	// Coordinata dell'evento (per il mouse)

	INT16	iTasto;
	INT		iObject;	// Oggetto selezionato
	CHAR	szObjName[20];
	DWORD	dwParam;	// di Parametro aggiunto senza segno
	INT		iParam;		// Parametro intero
	void *	pExtra;		// Parametro Void (usato da funzioni esterne)
	
	DWORD	dwTime;		// clock() di inserimento

 } EH_EVENT;

//#define MAX_DEFAULT_EVENT 50
#ifndef EH_MOBILE
void *	ehEvent(EH_SRVPARAMS);
#define eventWinClean(wnd) ehEvent(WS_DO,0,wnd)
void	eventPush(EN_EHEVENT enType,DWORD dwParam,void * pExtra,CHAR * pszFormat,...);

#endif

#endif


// ------------------------------------------------------------------------
//
//	SYS structure
//
// ------------------------------------------------------------------------

#ifndef EH_CONSOLE

	#define NOMEICONE_SIZE 9
	#define LUNNOMEOBJ 10 // Massima lunghezza nome oggetto

	typedef struct  {

		CHAR	szName[NOMEICONE_SIZE]; //                Nome della icone
		WORD	wHdl;      //  Handle memory
		SIZE	sDim;		// Dimensioni dell'icone (originali)
		DWORD	dwSize;
		DWORD	dwOffset;	//  Offset di spiazzamento

		INT	iBitColor;	// Bit-Colore dell'icone

		DWORD	dwRowSize;		// Dimensioni della riga del bitmap
		DWORD	dwMaskRowSize;	// dimensioni della riga della mask
		BOOL	bLic;		// 0/1 se appartiene ad una LIC
		INT	iGroup;		// Gruppo di appartenenza (-1=Sistema protetto)

		BOOL	bAlpha;			// Il bitmap ha l'alpha channel
		void	*hBitmapIcon;	// !=NULL puntatore ad oggetto bitmap del sistema operativo
		void	*hBitmapMask;	// !=NULL puntatore ad oggetto bitmap di maskera
		void	*pbBits;

		SIZE	sDimOut;	// Dimensioni dell'icone stampante (2012)

	} EH_ICON;

	// Definizione macro per oggetti
	typedef enum {

		O_ZONA=1,
		O_KEY=2,
		O_KEYDIM=20,
		O_MKUP=21,
		O_MKDOWN=22,
		O_MKLEFT=23,
		O_MKRIGHT=24,
		O_KEYDRV=25,
		//O_TEXT          3
		O_RADIO=4,
		O_MARK=5,
		O_SCROLL=6,
		O_SCRDB=18,
		O_ICONEA=7,
		O_ICONEB=8,
		O_IMARKA=9,
		O_IMARKB=10,
		O_IRADIA=11,
		O_IRADIB=12,
		O_LIST=14,
		O_LISTP=101, // List Plus  NovitÖ 99

		O_PMENU=15,
		O_BARY=16,
		O_BARX=17,

		// Oggetti disponibili solo in Windows
		OW_PMENU=50,
		OW_SCR=51,
		OW_LIST=52,
		OW_LISTP=53,
		OW_COMBO=54,
		OW_SCRDB=56,
		O_ZONAP=60, // Zona parametrica
		OW_MENU		// Menu 2010 (Fatto con EH_MENUITEM

	} EH_OBJTYPE;

	// new 2007
	// Bit 1 (TRUE= tasto premuto/FALSE=TASTO ALZATO)
	// Bit 2 (Mouse focus)
	// Bit 5 (Il mouse Ë sopra)
	typedef enum {
		
			O_KEYSTS_UP=0x00,	// Lo status del tasto Ë su
			O_KEYSTS_DOWN=0x01,	// Lo status del tasto Ë giu
			O_MOUSE_FOCUS=0x02,	// Il mouse focus Ë sul tasto (Ë premuto ma il mouse non Ë sopra)
			O_MOUSE_OVER=0x10   // Il mouse Ë sopra l'oggetto

	} EN_OBJ_STS;
	#define O_KEYSTS_MASK 0x01
	#define O_MOUSE_MASK 0x10    // Il mouse Ë sopra l'oggetto

	struct OBJ {
			EH_OBJTYPE tipo;
			CHAR	nome[LUNNOMEOBJ+1]; // TIPO OGGETTO,NOME ASSEGNATO
			EN_OBJ_STS status;			  //
			INT		bEnable;			  // T/F se l'oggetto Ë attivo
			INT		px,py,col1,col2; //                        POSIZIONE E COLORI
			CHAR	text[80];   //                     TESTO DA SCRIVERE SULLO SCHERMO
			CHAR	grp[2];     //                     GRUPPO DI APPARTENENZA (x O_RADIO)
			void * (*funcExtern)(struct OBJ *psObj,EN_MESSAGE enMess,LONG lParam,void *pVoid); //  funzione esterna di controllo
			CHAR **	ptr; // Puntatore ad una lista di parametri aggiuntivi
			INT		idxFont; // Indice del font in memoria da usare
			INT		CharY;
			INT		iStyle;
			HWND	hWnd;
			HMENU	hMenu;

			INT		iHmz;			// Tooltip associato all'oggetto
			INT		idMgz;			// id Mouse cursor associato all'oggetto
			BOOL	bFreeze;		// T/F se devo congelare l'oggetto (se congelato, non verranno effettuate chimate esterne)
			INT		yTitle;			// Dimensione del titolo (OW_SCR,OW_SCRDB)
			BOOL	fVertBar;		// Se esiste la barra verticale negli scroll
			BOOL	fHRedraw;		// Redraw completo in caso di variazione orizzontale
			BOOL	fMouseGhost;	// Disabilita l'intercettazione del mouse
			INT		idxWin;			// New 2007
			BOOL	bVisible;		// new 2007 T/F se visibile
			void *	pOther;			// Puntatore ad altro (new) 11/2007
			void *	pOtherEx;		// Dati aggiuntivi in fase di ricerca Estesi (2008) usato Es. OdbcScroll()
			RECT	sClientRect;	// New 2008 Rettangolo dell'area client dell'oggetto
			SIZE	sClientSize;	// Dimensioni area Cliente
			POINT	sClientCursor;	// Punto in cui si trova il cursore del mouse
			BOOL	bTabCode;		// T/F se è ho una array con tab code (new 2010)
			BOOL	bHidden;		// T/F se l'oggetto è nascosto (incredibile 2011)
		};

#define EH_OBJ struct OBJ

		typedef struct  {

			INT				iLength;  // Numero di oggetti contenuti
			EH_OBJ *		arsObj;
			struct WIN *	psWin;	// winParent backTrace

		} EH_OBJINFO;

#endif


//
//	KERNEL functions
//
    CHAR *  ehComputerName(CHAR * pszBuffer,INT iSizeBuffer);
	void	ehPathSet(UTF8 * pszString);
	UTF8 *	ehPath(UTF8 * pszString);

	CHAR *	ehAppPath(CHAR *pszProgram,CHAR *pszBuffer,SIZE_T sztBuffer); // Percorso dell'applicazione
	CHAR *	ehAppDataPath(CHAR *pszProgram,CHAR *pszBuffer,SIZE_T sztBuffer); // Percorso dei dati dell'applicazione
	CHAR *	ehUserPath(CHAR *pszProgram,CHAR *pszBuffer,SIZE_T sztBuffer);// Percorso dei dati dell'utente
	CHAR *	ehWorkingPath(CHAR *pszProgram,CHAR *pszBuffer,SIZE_T sztBuffer); // Percorso di lavoro dell'applicazione
	CHAR *	ehAppGetVersion(void); // New 2009
	CHAR	*GetSystemProductKey(void); // new 2008
	CHAR	*GetCurrentUserName(void); // new 2008
	void	OsEventLoop(INT iNum);

	INT		ehStart(void (*funcNotifyStart)(INT),void (*funcNotifyEnd)(INT));
	void	ehExit(CHAR *pszMess,...);
#define		ehError() ehExit("-> %s,%s():%d",__FILE__,__FUNCTION__,__LINE__);
	void	ehGetch(void);
	INT		ehTimeElapsed(void);
	void	ehAlert(CHAR *pszMess,...);
	INT		ehPrintf(CHAR *pszMessage,...); // output standard
	void	ehPrintfSet(INT (*extVfprintf)(const char * _Format, va_list _ArgList));
#define 	alert(msg,...) ehAlert(msg,__VA_ARGS__)
#define 	win_infoarg(msg,...) ehAlert(msg,__VA_ARGS__)
	CHAR *	ehGetEnvironment(CHAR * pszSep);

	// new 2012
	void	paramPreset(INT iArgs, CHAR * arArgs[]);
	BOOL	paramStr(CHAR * pszName, CHAR ** pszStr);
	CHAR *	paramStrAlloc(CHAR * pszName, CHAR * pszDefault);
	BOOL	paramStrCpy(CHAR * pszName, CHAR * pszStr, INT iSize);
	BOOL	paramInt(CHAR * pszName, INT * piValue);
	BOOL	paramCheck(CHAR * pszName);

#ifdef EH_IOS
    UTF8 *  iosGetDevice(UTF8 * pszDest,SIZE_T iLen);
	void	uiAlertView(CHAR * pszTitle, CHAR * pszMessage, void *);
 #ifdef __OBJC__
    BYTE * uiGetUrl(CHAR *pszUrl,SIZE_T * piLength);
 #endif
#endif
	
	void	ehSocketInit(void); // new 2010 - Avvio dei socket
	void	ehReport(BOOL bShow,CHAR *Mess);
	void	ehConsole(BOOL bShow,BOOL bSaveRestorePosition); // Solo GDI

#ifdef __windows__

	SINT ehWinFileOpen(UTF8 * pszTitolo,UTF8 * pszFilter,UTF8 * utfFileName,SIZE_T tFileName,BOOL bFlagExist);
	SINT ehWinFileSave(UTF8 * pszTitolo,UTF8 * pszFilter,UTF8 * utfFileName,SIZE_T tFileName,BOOL bFlagExist);
	BOOL ehWinDirChoose(UTF8 * pszTitolo,UTF8 * pszDirName,SIZE_T tFileName);

	void WindowsMessageDispatch(void);
    LRESULT CALLBACK EHStdWndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
	#define ehSleep Sleep
#else
    void    ehSleep(INT iWaitMillisec);
#endif

	void	PauseActive(LONG dwSleepms);
	// Obsolete
	// void	ledblink(INT cmd,void *ptr);
	// void	orologio(INT cmd,void *ptr);

	//
	// Gestione del Log
	//
	void	ehLogOpen(UTF8 * pszSource,void (*funz)(CHAR *ptr));
	void	ehLogWrite(CHAR *Mess,...); // new fine 2004
	BOOL	ehLogWriteEx(UTF8 * pszSource,CHAR *Mess,...); // new 2008
	void	ehLogSetProcess(DWORD id); // new 2009
#ifdef _DEBUG
	#define ehPrintd(msg,...) fprintf(stderr,msg,__VA_ARGS__)
#else
	#define ehPrintd(msg,...) ((void)0)
#endif
	//void ehPrint(CHAR *Mess,...);

	//
	// New 2005
	//
	BOOL strToClipboard(CHAR * pszFormat,...);
	BOOL wcsToClipboard(WCHAR * wcsFormat,...);

//	INT	ini_find(CHAR *cerca,CHAR *serv);
//	void	ini_replace(CHAR *lpOldString,CHAR *lpNewValue);

#if !defined(EH_CONSOLE)
	void	main(void);
	void	mainarg(INT,CHAR **);
#else
	 HWND WindowNow(void);
#endif

	INT		osGetError(void);
	void	osError(BOOL fBreak,INT iError,CHAR * pszMess,...);
	CHAR *  osErrorStr(INT iError,CHAR * pszBuffer,INT iSizeBuffer);
	CHAR *  osErrorStrAlloc(INT iError);

	#define JOBBER {PtJobber=__FILE__"|"__FUNCTION__;}
//	void	EasyHand(void);

	//
	// Exit Manager 2007
	//
	typedef void (*P_EXIT_FUNC)(BOOL);
	void ehAddExit(P_EXIT_FUNC pfnFunc);
	void ehRemoveExit(P_EXIT_FUNC pfnFunc);
	//void EM_FuncCall(BOOL fExitOnError);

	//
	// Finestre di avviso o richiesta
	// Vanno cambiate con altro, chiamare win
	//
#ifdef EH_WIN_GDI
	INT ehMessageBox(CHAR *lpTitle,CHAR *lpMessage,INT iType); // new 2007
	void	win_info(CHAR *lpMessage);
	void	win_err(CHAR *lpMessage);
	void	win_errgrave(CHAR *lpMessage);
	INT		win_ask(CHAR *lpMessage);
	INT		win_askarg(INT iType,CHAR *lpTitle,CHAR *Mess,...);
	void	win_time(CHAR *lpMessage,INT time);
	WORD	win_time2(CHAR *lpMessage,INT time);
	void	win_infoarg(CHAR *Mess,...);

	typedef struct {
		
		BOOL		bReady;
		HWND		wnd;
		SIZE		sizWin;

		HWND		wndTitle;
		EH_FONT	*	psFontTitle;

		HWND		wndSubtitle;
		EH_FONT	*	psFontSubtitle;

		EH_COLOR	cBackground;
		INT			iBarHeight;

		HWND		wndProgress;
		BOOL		bProgressBar;
		INT			iCounter;
		INT			iMax;
		INT			iPerc; // Percentuale di avanzamento

	} S_WINPROGRESS;

	void * winProgress(	EN_MESSAGE	enMess,
						INT			iValue,
						void *		pVoid); // 2012
#endif

	//
	// Math Section (2010)
	//
	BOOL isPointInPolygon(INT iPoints, POINT *arPoint, INT  x, INT y);
	BOOL isPointInRect(POINT * psPoint,RECT *psRect);
	void pointTranslate(INT iNum,POINT * arsPoint,INT x,INT y); // 2013
	void pointTranslateD(INT iNum,POINTD * arsPointd,double dX,double dY); // 2013
	POINTD * pointToPointd(INT iPoints,POINT * arsPoint); // 2013
	#define ehAtoi64(a) _atoi64(a)
	#define ehWtoi64(w) _atoi64(w)

//
// <== KERNEL SECTION (END) ========================================================
//

// =================================================================================
//
// ==> TIME SECTION
//
// =================================================================================

	//
	// time[functions]
	//
	typedef struct
	{
		WORD	wYear;		// [0000-2xxx]
		WORD	wMonth;		// [1-12]
		WORD	wDay;		// [1-31]

		WORD	wHour;		// [0-23]
		WORD	wMin;		// [0-59]
		WORD	wSec;		// [0-59]

		WORD	wDayWeek;   // days since Sunday - [0,6]
		WORD	wDayYear;   // days since January 1 - [0,365]

		BOOL	bIsDst;		// daylight savings time flag (siamo in Ora Legale)
		INT		iDayLightBias;	// Differenza del DayLight
		INT		iBias;		// Differenza da UTC (Local TimeZone+Bias DayLight) 

		TIME64	utcTime;	// UTC (Coordinated Universal Time = tempo coordinato universale) 

	} EH_TIME;

	EH_TIME *	timeNow(EH_TIME * psEht);
//#define	 timeNow(a) timeNowEx(a,true)
//	EH_TIME *	timeUtc(EH_TIME * pehtSource); // Converte il fomrato gg/mm/aaaa h,m,s > in UTC
	EH_TIME *	timeSet(EH_TIME *psEht,CHAR *pDateStringDMY,CHAR *pTimeStringHMS);
	EH_TIME *	timeSetEx(	EH_TIME *	psEht,
							CHAR *		pszDateStringDMY,
							CHAR *		pszTimeStringHMS,
							BOOL		bIsDst,		// T/F se nell'ora è ora legale
							INT			iTimeZone);	// Differenza GMT
	CHAR *		timeGetDate(EH_TIME *psEht,CHAR *pDateTarget,BOOL bFormatYMD);
	// new 2010
	void		timeCalc(EH_TIME *psEht,
						INT iDays,
						INT iMonth,
						INT iYear,
						INT iOre,
						INT iMin,
						INT iSec);
	void		timeValidate(EH_TIME *psEht);

	typedef struct {


		 INT iSegno; //

		 DWORD dwDays;
		 DWORD dwHours;
		 DWORD dwMinutes;
		 DWORD dwSeconds;
		 time_t tTotalSeconds;

		 BOOL	iError;	// 1=Data uno non valida, 2=Data 2 non valida

	} EH_TIME_DIFF;

	INT64			timeDifference(EH_TIME *pehtStart,EH_TIME *pehtEnd,EH_TIME_DIFF *pTimeDiffReturn);
	INT				timeCmp(EH_TIME *psA,EH_TIME *psB);
	WORD			timeGetDayMonth(EH_TIME *psEht);
	void			timeSetEOM(EH_TIME *psEht);
	CHAR *			timeString(EH_TIME *psEht,INT iFormat,CHAR *pszDest); // 2010
	CHAR *			timeFormat(EH_TIME *ptime,CHAR *pszBuffer,INT iSizeBuffer,CHAR * pszTimeFormat,CHAR * pszLanguage);
	//INT			timeDayOfYear(EH_TIME *psTime);
	#define			timeCpy(a,b) memcpy(a,b,sizeof(EH_TIME))

	EH_TIME *		timeDtToEht(EH_TIME *psEht,CHAR *dtDate); // 2010
	TIME64			timeDtToT64(CHAR *dtDate); // 2010
	CHAR *			timeDtToTimeStamp(CHAR * pszTimeStamp,INT iTimeStampSize,CHAR * dtDate); // Dt > timeStamp

	CHAR *			timeEhtToDt(CHAR *psDtDest,EH_TIME *psEht);			// Ehd	> Dt
	struct tm *		timeEhtToTm(struct tm * psTm,EH_TIME * psEht);
	TIME64			timeEhtToT64(EH_TIME * pFileTime);	// Eht > T64
	#define timeEhtToTimeStamp(a,b,c) timeFormat(c,a,b,"'%Y-%m-%d %H:%M:%S'",NULL)

	struct tm *		timeT64ToTm(struct tm * psTm,TIME64 iTime64);	// tm > T64
	CHAR *			timeT64ToDt(CHAR *psDtDest, TIME64 iTime64);		// time64 > Dt
	EH_TIME *		timeT64ToEht(EH_TIME * psTime, TIME64 iTime64);

	EH_TIME *		timeTmToEht(EH_TIME *psTime, struct tm * psTm);		// tm > eht
	TIME64			timeTmToT64(struct tm * psTm);

	//#define timeGetDayMonth(psEht) ((!(psEht->wYear%4)&&(psEht->wMonth==2))?arDayPerMonth[psEht->wMonth-1]+1:arDayPerMonth[psEht->wMonth-1])

	//
	// Conversioni
	//
	#ifdef __windows__

		FILETIME *		timeT64ToFt(FILETIME* pFileTime , TIME64 iTime64);	// time64 > FileTime
		SYSTEMTIME	*	timeTmToSt(SYSTEMTIME *ps,struct tm *t);			// Tm > SystemTime

		SYSTEMTIME *	timeFtToSt(SYSTEMTIME * psSt,FILETIME *psFt);		// FileTime > SystemTime
		TIME64			timeFtToT64(const FILETIME* pFileTime);				// FileTime > time64
		CHAR *			timeFtToDt(CHAR *psDtDest,FILETIME *psFileTime);	// FileTime > Dt (string)

		struct tm *		timeStToTm(struct tm *psTm,SYSTEMTIME *s);			// SystemTime > Tm
		EH_TIME *		timeStToEht(EH_TIME *psEht,SYSTEMTIME *psSt);		// SystemTime > Ehd
		TIME64			timeStToT64(SYSTEMTIME * psSystemTime);				// SystemTime > time64

		SYSTEMTIME *	timeEhtToSt(SYSTEMTIME *psSt, EH_TIME *pehd);		// Ehd	> St


	#endif
//		FILETIME *		timeUtToFt(FILETIME *psFt, time_t t);				// UnixTime > FileTime

	CHAR * timeStampNow(void);

	//
	// date[functions]
	//

	CHAR *	dateToday(void);
	CHAR *	dateTodayRev(void);
	CHAR *	dateTodayRfc(void); // new 2009
	CHAR *	dateCalc(CHAR *pszDateDMY,INT iDays);
	INT		dateDiff(CHAR * pszDateDMY_A,CHAR * pszDateDMY_B);
#define DATE_DIFF_ERROR -10000000

	CHAR *	dateDtoY(CHAR *pszDateDMY); // da ddmmyyyy > yyyymmdd
	CHAR *	dateYtoD(CHAR *pszDateYMD); // da yyyymmdd > ddmmyyyy
	CHAR *	dateFormat(CHAR * pszDateDMY,CHAR * pszBuffer,INT iSizeBuffer,CHAR *pszTimeFormat,CHAR * pszLanguage);
	CHAR *	dateFor(CHAR * pszDateDMY,CHAR * pszFor); // ex dateFor
	CHAR *	dateForEx(CHAR * pszDateDMY,CHAR * pszFor,CHAR * pszBuffer,INT iSizeBuffer); //

	CHAR *	hourNow(CHAR * pszType);
	CHAR *	hourNowEx(CHAR * pszType,CHAR *pszBuffer);
	CHAR *	hourFor(CHAR * pszHMS,INT iMode);

	//
	// dt[functions]
	//

	CHAR *	dtNow(void);
	CHAR *	dtGetDate(CHAR *pDT);
	CHAR *	dtGetTime(CHAR *pDT);//,BOOL bSep);
	INT	dtCompare(CHAR *pDT1,CHAR *pDT2);
	CHAR *	dtView(CHAR *ptDtFormat);//,BYTE *pDataFormat); // new 2007

	//
	// chrono[functions]
	//
	void	chronoStart(DWORD *);
	DWORD	chronoGet(DWORD *);  //  GetChrono
	double	chronoGetSec(DWORD *); 
	CHAR *	chronoFormat(time_t tClockTime,INT iMode);

//
// <== TIME SECTION (END) ========================================================
//

// =================================================================================
//
// ==> NUMBER SECTION
//
// =================================================================================
	DWORD	xtoi(CHAR * psz);
	CHAR *	numFormatEx(CHAR *	pszBuffer,
						INT		iSizeBuffer,
						double	dNumber,
						INT		iDigit,
						INT		iDec,
						BOOL	bSegno,
						CHAR *	pszSep);

	CHAR *	numFormat(double dNumber,INT iDigit,INT iDec,BOOL bSep,BOOL bSegno);
	CHAR *	strFormat(CHAR * pszNumber,INT iDigit,INT iDec,BOOL bSep,BOOL bSegno);
	double	numRound(double dNumber,INT iPrecision,BOOL bUp);
	CHAR *	numLetter(double dNumber,CHAR * pszDest,INT iSizeDest);
#define	Snummask numFormat
#define	nummask strFormat
	LONG	DToL(double Numero);
	double	DNoS(double dato);

	typedef struct {
		BOOL	bReady;

		double p1x;
		double p1y;
		double p2x;
		double p2y;

		double	cx;// = 3.0 * p1x; // X component of Bezier coefficient C
		double	bx;//  = 3.0 * (p2x - p1x) - cx; // * X component of Bezier coefficient B
		double	ax;//  = 1.0 - cx -bx; // X component of Bezier coefficient A
		double	cy;//  = 3.0 * p1y; // Y component of Bezier coefficient C
		double	by;//  = 3.0 * (p2y - p1y) - cy; // Y component of Bezier coefficient B
		double	ay;//  = 1.0 - cy - by; // Y component of Bezier coefficient A

	} S_BEZIER;

	void	bezInit(S_BEZIER * psBez,double p1x, double p1y, double p2x, double p2y);
	double	bezEngine(S_BEZIER * psBez, double dX, double dTimeMilliSec);

	//	double	Ddecfix(double Numero,INT dec);
	//	double	DecFixAppro(double Numero,SINT Cifre);

//
// <== NUMBER SECTION (END) ========================================================
//

// =================================================================================
//
// ==> STRING SECTION
//
// =================================================================================
#define strLen(a) (DWORD) strlen(a)
#define wcsLen(a) (DWORD) wcslen(a)

	#define AddBs(path) {if (path[strlen(path)-1]!='\\') strcat(path,"\\");}
	#define AddBsW(path) {if (path[wcslen(path)-1]!=L'\\') wcscat(path,L"\\");}
	#define AddSlash(path) {if (path[strlen(path)-1]!='/') strcat(path,"/");}
	#define AddSlashW(path) {if (path[wcslen(path)-1]!=L'/') wcscat(path,L"/");}

#if defined(__windows__)||defined(__marmalade__)
	CHAR *	strCaseStr(CHAR * pszString,CHAR * pszSearch);
	WCHAR * wcsCaseStr(WCHAR *String,WCHAR *pwFind);
#else
	CHAR *	strCaseStr(__const CHAR * pszString,__const CHAR * pszSearch);
	WCHAR * wcsCaseStr(__const WCHAR *String,__const WCHAR *pwFind);
#endif

#ifdef __windows__
	#define OS_DIR_SLASH "\\"
	#define AddSl AddBs
	#define strCaseCmp _stricmp
	#define memCaseCmp _memicmp
#else
	#define OS_DIR_SLASH "/"
	#define AddSl AddSlash
	#define strCaseCmp strcasecmp
    int		memCaseCmp (const void *vs1,const void *vs2,size_t n);
#endif

	// simil ansi
	void	strIns(CHAR *p,CHAR *Cosa);
	INT		strCmp(CHAR *pszA,CHAR *pszB); // 2010
	INT		wcsCmp(WCHAR * pszA,WCHAR * pszB); // 2012
	
	CHAR *	strCpy(CHAR *pszDest,CHAR *pszSource,SIZE_T tSize); // 2010
#define		strCopy(a,b) strCpy(a,b,sizeof(a))
	CHAR *  strCpyAsc(CHAR *pszDest,WCHAR *pwcSource); // 2011
	CHAR *	strCpyUtf(CHAR *pszDest,WCHAR *pwcSource,INT iSizeDest); // 2011
	WCHAR *	wcsCpyUtf(WCHAR *pwcDest,UTF8 * pszSource,INT iSizeDest); // 2011

	WCHAR * wcsReverseStr(WCHAR *pString,WCHAR *pFind);
	CHAR *	strReverseStr(CHAR *String,CHAR *Find);
	CHAR *	strReverseCaseStr(CHAR *String,CHAR *Find);


	BOOL	strReplace(CHAR *lpSource,CHAR *lpFindStr,CHAR *lpReplaceStr);
	BOOL	wcsReplace(WCHAR *lpSource,WCHAR *lpFindStr,WCHAR *lpReplaceStri);
	BOOL	strCaseReplace(CHAR *lpSource,CHAR *lpFindStr,CHAR *lpReplaceStr);

	BOOL	strBegin(CHAR *lpString,CHAR *lpControllo); // new 8/2007
	BOOL	strCaseBegin(CHAR *lpString,CHAR *lpControllo); // new 8/2007
	BOOL	strEnd(CHAR *lpString,CHAR *lpControllo); // new 11/2007
	BOOL	strCaseEnd(CHAR *lpString,CHAR *lpControllo); // new 11/2007
	CHAR *	strWordCase(CHAR *); //new 9/1007
	void	strUtfBreak(CHAR *pStringUft,UINT iMaxSize); // new 11/2008
	CHAR *	strUtfLower(CHAR *pszSource,SIZE_T dwSize); // 10/2010

	CHAR *	strTrimRight(CHAR *lpStr); // Ex NoSpaceCue()
	WCHAR *	wcsTrimRight(WCHAR *lpStr);
	CHAR *	strTrimNaRight(CHAR *lpStr); // ChrNullCue

	CHAR *	strTrim(CHAR *lpSource);
	WCHAR *	wcsTrim(WCHAR *pSource);
	WCHAR*	strTrimw(CHAR *lpSource);
	CHAR *  strPad(CHAR cChar,INT iRepeat); // 2010

	CHAR *  strTrimEx(CHAR * pSource,CHAR *pszExtra); // 2013

//	void	CharToUnicode(WCHAR *lpDest,CHAR *lpSource);
	//void	UnicodeToChar(CHAR *lpDest,WCHAR *lpSource);
//	wchar_t *ToUnicodeAlloc(CHAR *lpSource); // New 2006

	CHAR	* strOmit(CHAR *lpSource,CHAR *lpChar); // Limitati a 250byte (no MT)
	WCHAR   * wcsOmit(WCHAR *pwcSource,WCHAR *pwcChars,WCHAR *pwcDest);
	CHAR	* strKeep(CHAR *lpSource,CHAR *lpChar); // Limitati a 250byte (no MT)
	CHAR	* strOmitAlloc(CHAR *lpSource,CHAR *lpChar);
	CHAR	* strKeepAlloc(CHAR *lpSource,CHAR *lpChar);
	INT		strCount(CHAR *pSource,CHAR *pChar); // new 2009
	BOOL	strControl(CHAR *pszCharList,CHAR *pszStringCheck); // new 2010
#define strSplit ARFSplit

	//
	// String allocation
	//
	#define wcsAssign(a,b) {ehFreePtr(&a); a=wcsDup(b);}

	CHAR *	strTakeCpy(CHAR *pStart,CHAR *pEnd,CHAR * pszDest,SIZE_T iSize); // new 11/2012

#ifndef EH_MEMO_DEBUG
	void	strAssign(CHAR **ptr,CHAR *pNewValue); // Era SetString - (alloca) una stringa 2007 in un puntatore (se il puntatore Ë pieno lo libera)
	CHAR *	strDup(CHAR *lpString);
	WCHAR *	wcsDup(WCHAR *lpString);
	CHAR  *	strTake(CHAR *pStart,CHAR *pEnd);	// new 11/2008
	WCHAR *	wcsTake(WCHAR *pStart,WCHAR *pEnd); // new 11/2008
//	#define StringAlloc(a) strDup(a)
//	#define wcsDup(a) wcsDup(a)
	CHAR *	wcsToStr(WCHAR *pwcString);
	WCHAR *	strToWcs(CHAR *pszString);

#else
	CHAR  * _strDupEx(CHAR *lpString EMD_END_FUNC);
	WCHAR *	_wcsDupEx(WCHAR *lpString EMD_END_FUNC);
	CHAR  *	_strPtsEx(CHAR *pStart,CHAR *pEnd EMD_END_FUNC); // new 2008
	void	_strAssignEx(CHAR **ptr,CHAR *pNewValue EMD_END_FUNC); // new 2008
	CHAR *	_strTakeEx(CHAR *pStart,CHAR *pEnd EMD_END_FUNC);
	WCHAR * _wcsTakeEx(WCHAR *pStart,WCHAR *pEnd EMD_END_FUNC);
	CHAR *  _wcsToStr(WCHAR * pwcSource EMD_END_FUNC);
	WCHAR *	_strToWcs(CHAR *pszString EMD_END_FUNC);

	#define strDup(a) _strDupEx(a,__FILE__,__LINE__)
	#define wcsDup(a) _wcsDupEx(a,__FILE__,__LINE__)

	#define strTake(a,b) _strTakeEx(a,b,__FILE__,__LINE__)
	#define wcsTake(a,b) _wcsTakeEx(a,b,__FILE__,__LINE__)
	#define strAssign(a,b) _strAssignEx(a,b,__FILE__,__LINE__)
	#define wcsDup(a) _wcsDupEx(a,__FILE__,__LINE__)
	#define wcsToStr(a) _wcsToStr(a,__FILE__,__LINE__)
	#define strToWcs(a) _strToWcs(a,__FILE__,__LINE__)
#endif

    #define wcsToUtf(a) strEncodeW(a,SE_UTF8,NULL);

	BOOL	strEmpty(CHAR *ptr);
	BOOL	wcsEmpty(WCHAR *ptr);
#define	 strNext(a) a+strlen(a)
#define  strAppend(str,mes,...) sprintf(str+strlen(str),mes,__VA_ARGS__)
	void	strAdd(CHAR *pContainer,CHAR *pString,CHAR *pSep,BOOL bNoEmpty); // new 2008 Ex ARAdd
	void	strCat(CHAR **ptr,CHAR *pNewValue); // new 2008 (Cat dinamico)
#define		strEver(a) (!a?"":a)
#define		wcsEver(a) (!a?L"":a)
#define		strEverB(a,b) (strEmpty(a)?b:a)
#define		wcsEverB(a,b) (wcsEmpty(a)?b:a)

	BOOL	strOnlyNumber(CHAR *pString); // Stringa di soli numeri
	DWORD	strWordCount(CHAR *psz);
	DWORD	wcsWordCount(WCHAR *pwc);

	// Estrae un stringa dando il tag di inizio e fine
	CHAR *	strExtract(CHAR *pString,CHAR *pTagStart,CHAR *pTagEnd,BOOL bCaseUnsensitive,BOOL bWithTag);
	WCHAR *	wcsExtract(WCHAR *pwString,WCHAR *pwTagStart,WCHAR *pwTagEnd,BOOL bCaseUnsensitive,BOOL bWithTag);

	CHAR *	strFileSize(UINT64 tiSize,INT iDec); // 02/2010 GDF
	CHAR *	strFileLimiter(CHAR *pszFileName,CHAR *pszDest,DWORD iMaxString); // 04/2010
	WCHAR *	wcsFileLimiter(WCHAR *pwcFileName,WCHAR *pwcDest,DWORD iMaxString);

	INT		strGetInt(CHAR *psz,INT iLen);
	void	strSlash(CHAR * pszPath,CHAR * pszSlash); // 2012
	CHAR *	strPathRelative(CHAR * pszDest,INT iSize,CHAR * pszPathAbs,CHAR * pszPathRel); // 2012/09

	BOOL	strWildMatch(CHAR *pszPattern, CHAR *pszString);
	BOOL	wcsWildMatch(WCHAR *pwcPattern, WCHAR *pwcString);

	BOOL	isNaN(BYTE *pString); // Come Javascript new 2009
	BOOL	isEmail(CHAR *pEmail); // 2010

	CHAR *	strFromAh(va_list ah,CHAR * pszFormat);
	WCHAR *	wcsFromAh(va_list ah,WCHAR * pszFormat);
	#define strFromArgs(pszFormat,pszElement) {va_list ah; if (pszFormat) va_start(ah,pszFormat); pszElement=strFromAh(ah,pszFormat);}
	#define wcsFromArgs(pszFormat,pszElement) {va_list ah; if (pszFormat) va_start(ah,pszFormat); pszElement=wcsFromAh(ah,pszFormat);}



//
// <== STRING SECTION (END) ========================================================
//

// =================================================================================
//
// ==> TASK SECTION
//
// =================================================================================

#ifdef __windows__

	DWORD	processGetCurrentId(void);
	BOOL	processRun(CHAR *lpProgram,
					   CHAR *lpCommandLine,
					   CHAR *lpEnvironment, // Separare con /n CICCIO=PIPPO/nA=2/n
					   BOOL fHide,
					   INT *lpiErr);
	BOOL	processRunEx(CHAR *lpProgram,
				      CHAR *lpCommandLine,
				      CHAR *lpEnvironment, // Separare con /n CICCIO=PIPPO/nA=2/n
				      BOOL fHide,
				      INT *lpiErr,
				      PROCESS_INFORMATION *lpProcessInformation,
					  HANDLE hdlOut);

	CHAR *	processRunAlloc(CHAR *lpProgram,
					  CHAR *lpCommandLine,
					  CHAR *lpEnvironment, // Separare con /n CICCIO=PIPPO/nA=2/n
					  BOOL fHide,
					  INT *lpiErr,
					  BOOL fEnvironmentParent); // T/F Aggiungo l'Enviroment del Task Padre
	void	processWait(DWORD dwMillSec);
	void	processWin(CHAR * pszWinTitle,BOOL bMinimized); // 2012
	
#endif

//
// <== TASK SECTION (END) ========================================================
//

// =================================================================================
//
// ==> MEMORY SECTION
//
// =================================================================================


	//#define farPtr(a,b) ((BYTE *) a)+(b)
	void *	farPtr(void *p,LONG l);


	SIZE_T		memoUsed(void);
	HMEM		memoAlloc(EH_MEMO_TYPE enType,SIZE_T sizebyte,CHAR *pszOwner);
	INT			memoFree(HMEM hdl,CHAR *chisei);
	INT			memoWrite(HMEM hMemo,SIZE_T tOffset,void *ptSourceBuffer,SIZE_T tSize);
	INT			memoRead(HMEM hMemo,SIZE_T tOffset,void *ptDestBuffer,SIZE_T tSize);
	HMEM		memoClone(HMEM Hdl);
	INT			memoCopyAll(HMEM hdl1,HMEM hdl2);
	void	*	memoLockEx(HMEM hMemo,CHAR *lpWho);
	void		memoUnlockEx(HMEM hMemo,CHAR *lpWho);
#define memoLock(a) memoLockEx(a,NULL)
#define memoUnlock(a) memoUnlockEx(a,NULL)

	CHAR	*	memoGetName(HMEM hdl);
	void		memoSetName(HMEM hdl,CHAR *pszName);
	EH_MEMO_ELEMENT * memoGetInfo(HMEM hdl);
	void *		memoPtr(HMEM hdl,BOOL * pbLock); // Ex. memo_heap

//
//	Windows
//
//#ifdef __windows__

	EH_MEMO_TYPE	memoGetType(HMEM hdl);
	BOOL	memo_manager(HMEM iNewSize);

	#define CPU_MMX (0x00000002L)
	#define CPUF_SUPPORTS_SSE                       (0x00000010L)
	#define CPUF_SUPPORTS_SSE2                      (0x00000020L)

//#endif
/*
//
// Apple
//
#ifdef __APPLE__
    void	* memoLock(INT hMemo);
	void	* Wmemo_lockEx(INT hMemo,CHAR *lpWho);
    void	memoUnlock(INT hMemo);
	void	Wmemo_unlockEx(INT hMemo,CHAR *lpWho);
	INT	memo_clone(INT Hdl);
	BOOL	memo_manager(INT iNewSize);
#endif
*/
	// es far_bsearch() (Ritorna TRUE se non la trova)
	BOOL ehBinSearch(CHAR *cerca,
				      CHAR *dbase,
				      DWORD *pos,
				      DWORD size,
				      DWORD offset,
				      DWORD nkey);

//
// DOS
//
#ifdef __DOS__
	INT 	memo_info(INT hdl,INT *tipo,WORD *sgm);
	WORD	memo_seg(INT hdl);
	WORD	memo_off(INT hdl);

	struct RAM {
			INT  tipo; 	// RAM_????
			WORD  sgm;    // Segmento di riferimento memoria
			WORD  ofs;    // Offset   di riferimento memoria
			CHAR *ptmemo; // Puntatore far alla memoria
			FILE  *fp;			// Usato per RAM_disk
			LONG  size;	  // Dimensioni
			INT  xms;		// Handle per XMS
			CHAR far User[MEMOUSER];
			};

	    // XMS.H

     INT  XMS_alloca(INT,INT *);
     INT  XMS_libera(INT);
     INT  XMS_size(INT *);

#endif

//
// Windows
//
//	#define M_FIXED M_HEAP+128
	#define ehNew(a) ehAllocZero(sizeof(a))
	#ifdef EH_MEMO_DEBUG

        void *		_ehAlloc(SIZE_T iMemo,BOOL bZero EMD_END_FUNC);
        void		_ehFree(void *ptr EMD_END_FUNC);
        void		memDebugUpdate(void *ptr EMD_END_FUNC); // 2010

        #define	ehAlloc(a) _ehAlloc(a,FALSE,__FILE__,__LINE__)
        #define	ehAllocZero(a) _ehAlloc(a,TRUE,__FILE__,__LINE__)
        #define	ehFree(a) _ehFree(a,__FILE__,__LINE__)

	#else
        // ehAlloc Group in modalità release
        void *	ehAlloc(SIZE_T iMemo);
        void *	ehAllocZero(SIZE_T iMemo); // Alloca Ë mette a zero
        void	ehFree(void *ptr);
	#endif

	void *	ehRealloc(void *ptr,SIZE_T OldMemo,SIZE_T iMemo);
	#define ehFreeNN(lp) {if (lp) ehFree(lp);}
	void	__ehFreePtr(void * ptr);
//	#define ehFreePtr(x) __ehFreePtr((void **) x)
	#define ehFreePtr(x) __ehFreePtr(x)
	void	ehFreePtrs(INT iNum,...);  // 2010

	#ifdef __windows__
        void	ehMemCpy(void* dst, const void* src, SIZE_T len);
		SIZE_T memoUsedProcess(HANDLE hProcess);
	#else
		#define ehMemCpy(a,b,c)	memcpy(a,b,c)
	#endif


//
// <== MEMORY SECTION (END) ==========================================================
//

// =================================================================================
//
// ==> AUDIO SECTION
//
// =================================================================================

	void AudioStart(void);
	void AudioEnd(void);

	//		FLM_AUDI HEADER

	void sonic(WORD part,INT lop1,INT inc1,INT lop2,INT inc2,INT wait);
	void beep1(void);
	void efx1(void);
	void efx2(void);
	void efx3(void);
	void efxtext(void);
	void efxmem(INT num);

	#ifdef __windows__
	 void PlayMySound(CHAR *Nome);
	#endif
//
// <== AUDIO SECTION (END) =========================================================
//

// =================================================================================
//
// ==> ULT SECTION (Translator)
//
// =================================================================================
typedef enum {

	EH_LANG_ITALIAN,
	EH_LANG_FRENCH,
	EH_LANG_ENGLISH,
	EH_LANG_DEUTCH,
	EH_LANG_SPAIN,

} EN_LANGUAGE;
	 
#define ULT_MAXTYPE 20 // <--- Importante

	 typedef enum {

		// Usati internamente
		ULT_TYPE_WINT=0,
		ULT_TYPE_WINI=1,
		ULT_TYPE_OBJS=2,
		ULT_TYPE_HMZ=3,
		ULT_TYPE_OBJ=4,
		ULT_TYPE_MENU=5,
		ULT_TYPE_SPF=6,
		ULT_TYPE_DISP=7,
		ULT_TYPE_LIST=8,

		// Usati con il web
		ULT_TYPE_HTML=16,
		ULT_TYPE_JAVA=17,
//		ULT_TYPE_WTAG_NO=18 // Ex sistema di diversificazione di encoding, lasciato con la versione 4 in UTF

	 } EN_ULTYPE;


#define ULT_MAXLANG 50 // Numero Massimo di lingue gestite

// CharType
typedef enum {

	ULT_CS_UNKNOW=-1,
	ULT_CS_LATIN1=0,
	ULT_CS_UTF8=1,
	ULT_CS_ASCII=2,
	ULT_CS_ANSI=3,
	ULT_CS_UNICODE=0x20,

} EN_CHARTYPE;

typedef enum {
	UU_UNKNOW=0,	// Sconosciuto
	UU_SOURCE=1,	// Sorgente
	UU_ADMIN=2,		// Admin
	UU_CPU=3		// CPU - Tradotto da Computer
} EN_USERTYPE;

#define UU_CTRL 0x10000 // Traduzione che Necessita di controllo, cambiata voce principale
#define UU_MASK 0x0FFFF // Masketa Utenti

// Encoding in generazione Htl
#define ULT_ENCODE_ISO 0  // Latin1 (per compatilit‡ con il passato)
#define ULT_ENCODE_UTF8 1 // Consigliato con UTF-8

typedef struct {

	EN_LANGUAGE		idLang;
	CHAR *			lpIsoPrefix;
	CHAR *			lpLangName;			// Uso Interno
	WCHAR *			pwcLangNameNativo;
	CHAR *			lpTransName;
	CHAR *			lpXiff;

} EH_ULT_LANG;

typedef struct {
	INT iFirst;
	INT iNum;
} ULT_TYPE;

typedef struct {

	EN_ULTYPE	iType:16; // Tipo di definizione
	DWORD		dwID;  // Identificativo univoco della definizione
	INT			iTransStatus; // percentuale lingue tradotte 100=Tutto tradotto
	BOOL		fLost:1;  // Non Ë associato a nessun file (il collegamento Ë stato perso)
//	BYTE  *lpcText[ULT_MAXLANG]; // Puntatore alla lingua codificata come da CS_Dictionary

	BYTE  *		lpbText; // Puntatore alla traduzione CHAR (inserita per compatibilità con i programmi One Char)
	WCHAR *		lpwText[ULT_MAXLANG]; // Puntatore alla lingua codificata come da CS_Dictionary
	DWORD		dwUser[ULT_MAXLANG]; // Utente che ha fatto la traduzione 0=non indicato, 1=Amministratore, 2=Traduttore automatico | new 2009

	// I uso solo sui programmi EasyHand per compatibilit‡ con il passato

} ULTVOICE;

typedef struct {

	BOOL	bReady;

	UTF8	szFile[300];
	CHAR	szUltFolder[300];
	DWORD	dwLastId;
	INT		iVersion;
	INT		iTypeEncDict;			 // 0=con @# e @<, 1=con \1\2\3

	INT		iLangNum;				// Numero di lingue/Codici presenti nel dizionario
	INT		iLangWant;				// Lingua voluta (Macro della lingua) / Solo uso Esecutivo per caricare solo la lingua desiderata
	INT		iLangReady;				// Numero di lingue realmente presenti nell'ult, tolte quelle di appoggio come codice,files,note ecc..
	INT		iLangNative;			// Codice Interno della lingua nativa |iLangNative|

	INT		hdlLangReady;			  // Memoria che contiene hdlLangReady
	EH_ULT_LANG *arLangGeneral; // Array con le lingue gestibili
	EH_ULT_LANG *arLangReady;	  // Array cone le lingue presenti nel dizionario
	
	CHAR **	lpLangSuffix;		// Puntatore a suffissi di lingua

	BOOL	fRealCode;				// T/F se il idxLangCode è un codice reale: FALSE è un "Testo da sorgente", TRUE un Codice
	INT		idxLangCode;			// indice il "Codice" sorgente (default 0)
	INT		idxLangNative;			// indice lingua NATIVA da cui deriva il codice Sorgente(es 1)
	INT		idxLangTranslated;		// Indice della lingua da usare (solo esecutivo)
	INT		idxLangTransAlternative;// indice lingua Alternativa in caso di traduzione inesistente (indice in Item)
	INT		idxFiles;				// indice Files > 0
	INT		idxNote;				// indice Note > 0

	INT		iLangTransAlternative;		// Codice Interno della lingua alternativa alla traduzione

	BOOL	fLoadAll;				// Tutto il dizionario caricato in memoria
	BOOL	bModified;				// T/F se Ë stato modificato (o Ë stata aggiunta qualche voce)

	_DMI	dmiDict;		// dmi del dizionario
	INT		iNewItem; // Nuovi item (Es. usato in Scan)

	ULT_TYPE arType[ULT_MAXTYPE];
	ULTVOICE *lpVoiceShare;		// Memoria di appoggio condivisa
	INT		iSizeVoice;

	BOOL	fWebFolder;		// T/F se agganciato ad un WebFolder
	BOOL	bWebAutoScan;		// T/F se deve autoscansire il sito in apertura
	CHAR *	pszDoPurge;		// Elenco dei lingua/Dominio per effettuare la pulizia della cache

	INT		iCS_Source;		// CharType del "codice" presi dal sorgente (default ANSI LATIN1/CHAR)
	INT		iCS_Dictionary;	// CharType dei termini tradotti (default UTF-8/CHAR)
	INT		iEncodeHtml;		// Encoding in Html (default ISO = Es &nbsp)
	INT		iItemDup;			// Item duplicati in caricamento (stesso Tipo/Parola codice)
	
	CHAR *	pszEncodingFilesSource;	// Tipo di encoding dei files source: di default non indicato, indicare le eccezioni tipo file.xml:utf-8,
	EH_ARF	arEFS;

	// Logo di creazione files
	CHAR	szLogFile[500];
	FILE *	chLog;
	BOOL	fLogWrite;			// Scrive il log in fase di costruzione files
	BOOL	fLogShowEnd;		// Mostra il log alla fine
	BOOL	fLogError;			// Se non stati riscontrati errori durante la creazione dei files

} EH_ULT;

//
// functions 
//

INT		ultNew(EH_ULT * psUlt,UTF8 * pszFile); 
BOOL	ultOpen(EH_ULT * psUlt,UTF8 * pszFile,INT iLanguage,BOOL fLoadAll);
BOOL	ultOpenEx(	EH_ULT * psUlt,
					BOOL	bMemoryData,
					CHAR *	pszFileName,
					INT iLanguage,
					BOOL fLoadAll,
					BOOL fOnlyHeader);
BOOL	ultAppOpen(UTF8 * pszFileName,CHAR * pszLang,CHAR * pszLangAlternative);
BOOL	ultResource(CHAR * pszLang,CHAR * pszLangAlternative);

//void	ULTNewDictionary(EH_ULT * psUlt,CHAR *lpFileName); // ??
INT		ultSave(EH_ULT * psUlt,BOOL fShowError);
BOOL	ultSaveAs(	EH_ULT * psUlt,
					CHAR * pszFileName,
					BOOL fShowError);
void	ultClose(EH_ULT * psUlt); // ex ULTResourceFree
EH_ULT_LANG * ultLangInfo(EH_ULT * psUlt, INT idLang,CHAR *lpIsoFind);

SINT	ultLangReady(EH_ULT * psUlt,SINT iCode,CHAR * lpIsoPrefix);
void	ultDictionaryMake(void);

INT		ultItemInsert(EH_ULT * psUlt,INT iType,WCHAR *pwcWord,DWORD dwID,BOOL *lpfDup);
void	ultWordFileControl(EH_ULT * psUlt,INT iIndex,CHAR *lpFile);
void	ultAddWord(EH_ULT * psUlt,void *lpElement,INT idxLang,INT iCharSize,DWORD dwUser); // 1=CHAR 8 Bit originale, 2=Wide Char
void	ultWordDelete(EH_ULT * psUlt,INT Index,INT iLang,DWORD dwUser);
void	ultItemDelete(EH_ULT * psUlt,SINT Index,DWORD dwUser);
BOOL	ultWordUpdate(EH_ULT * psUlt,INT Index,INT iLang,void *lpWord,INT iCharSize,DWORD dwUser);

void	ultSetLang(EH_ULT * psUlt,EN_LANGUAGE enLanguage,EN_LANGUAGE enLangAlternative);
void	ultSetLangIdx(EH_ULT * psUlt,INT idxLang,INT idxLangMiss);
void	ultSetLangText(EH_ULT * psUlt,CHAR * pszLang,CHAR * pszLangAlternative);

void	ultItemSetFlag(EH_ULT * psUlt);

//BYTE *ULTTranslateCheck(INT iType,BYTE *lpWord);  // Vecchio sistema
void *	ultTranslateEx(	EH_ULT * psUlt,
						EN_ULTYPE iType,
						WCHAR *pwcWordCode, // Ricerca Wide Char
						CHAR *lpFile,
						BOOL fBuildOn,
						INT *lpRow); // Dove si trova nelle righe

BOOL	ultFolderScan(EH_ULT * psUlt);
void	ultGeneralFileBuilder(	EH_ULT * psUlt,
								CHAR *lpFileUlt,
								BOOL fOpenDizionario,
								BOOL fRebuildAll,
								BOOL fShowFilesTouch);

CHAR *	ultCodeToText(EH_ULT * psUlt,INT iWhat,INT iCode);
INT		ultTextToCode(EH_ULT * psUlt,INT iWhat,CHAR *p);
INT		ultDwToRow(EH_ULT *psUlt,DWORD dwId); // da ID univoco a id Row
INT		ultLangAdd(EH_ULT * psUlt,CHAR *lpIsoPrefix);
INT		ultLangRemove(EH_ULT * psUlt,CHAR *lpIsoPrefix);

CHAR *	ultTypeToText(INT iCode);
BOOL	ultExtEnable(CHAR *lpExt);
BOOL	ultIdxIsLang(EH_ULT * psUlt,INT idx);
BOOL	ultMultiFileBuilder(EH_ULT * psUlt,
							CHAR *lpFolderBase, // Il folder dove si trovano le cartelle in lingua Es. c:\inetpub\wwwroot\mondialbroker
							CHAR *lpIsoLang,	 // Prefisso della lingua interessata
							WCHAR *pwcFileList, // Elenco dei file da rielaborare separati da virgola
							INT *lpiFileMissing,
							BOOL fShowFilesTouch);	// Numero dei File scomparsi

//void *ULTTranslateCheck(INT iType,BYTE *lpWord);//,INT iCharSet);
//
// Usato internamente da Easyhand
//
//#define ultTranslateCheck(a,b) sys.fTranslate?ultTranslate(a,b):b
UTF8 *	ultTag(CHAR * pszStr); // new 2012
void	ultCloseInternal(EH_ULT * psUlt); // ULTFree - Se il caso salva il dizionario cambiato su disco
BYTE *	ultTranslate(INT iType,BYTE *lpWord);
CHAR *	ultTranslateSZZAlloc(CHAR *lpStringZZ); // New 2005 Double Zero String

#define ULT_BLOWFISH "Gioele12"

//
// <== ULT SECTION (END) ===========================================================
//

// =================================================================================
//
// ==> FILE SECTION
//
// =================================================================================

#define IGNORA   -101
#define RIPROVA  -102
#define ABORT    -103
#define DOSERR   -104

#define NOSEEK  -1

#define ONVEDI   2
#define POP     -2

#define CP_MAX_PATH 2048 // Cross-Platform MaxPath
#define CP_MAX_NAME 300  // Cross-Platform MaxName

//
// Win 32
//
#if defined(__windows__)&&!defined(_WIN32_WCE)
	#include <io.h>
	#define MAXDIR  _MAX_DIR
	#define MAXPATH _MAX_PATH

	#define FA_ARCH   _A_ARCH
	#define FA_HIDDEN _A_HIDDEN
	#define FA_RDONLY _A_RDONLY
	#define FA_SYSTEM _A_SYSTEM
	#define FA_DIREC  _A_SUBDIR

	typedef struct {
		 intptr_t Handle;
		 INT AttribSel;
#ifdef UNICODE
		 struct _wfinddata_t ffile;
#else
		 struct _finddata_t ffile;
#endif
		 CHAR  *ff_name;
		 INT   ff_attrib;
		 CHAR   ff_date[9];
	} FFBLK;
#endif

//
// Linux
//
#if defined(__linux__)
	#include <linux/limits.h>
	#define MAXDIR  _MAX_DIR
	#define MAXPATH PATH_MAX

	#define FA_ARCH   _A_ARCH
	#define FA_HIDDEN _A_HIDDEN
	#define FA_RDONLY _A_RDONLY
	#define FA_SYSTEM _A_SYSTEM
	#define FA_DIREC  _A_SUBDIR

	typedef struct {
		 LONG Handle;
		 INT AttribSel;
		 /*
#ifdef UNICODE
		 struct _wfinddata_t ffile;
#else
		 struct _finddata_t ffile;
#endif
*/
		 CHAR  *ff_name;
		 INT   ff_attrib;
		 CHAR   ff_date[9];
	} FFBLK;
#endif


//
// Per Windows Mobile
//
#if defined(__windows__)&&defined(_WIN32_WCE)
	#define MAXDIR  200
	#define MAXPATH _MAX_PATH

	#define FA_ARCH   _A_ARCH
	#define FA_HIDDEN _A_HIDDEN
	#define FA_RDONLY _A_RDONLY
	#define FA_SYSTEM _A_SYSTEM
	#define FA_DIREC  _A_SUBDIR

	typedef struct {
		 LONG Handle;
		 INT AttribSel;
//		 struct _finddata_t ffile;
		 CHAR  *ff_name;
		 INT   ff_attrib;
		 CHAR   ff_date[9];
	} FFBLK;
#endif

#ifdef _DOS

	typedef struct {
		CHAR  *ff_name;
		INT   ff_attrib;
		INT   AttribSel;
		CHAR  ff_date[9];
		struct ffblk ffblk;
	} FFBLK;
#endif

//
// Apple
//
#ifdef __APPLE__

#include <AvailabilityMacros.h>
#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/attr.h>
// #include <sys/kernel_types.h>

//	#define MAXDIR  _MAX_DIR
	#define MAXPATH MAXPATHLEN

	#define FA_ARCH   _A_ARCH
	#define FA_HIDDEN _A_HIDDEN
	#define FA_RDONLY _A_RDONLY
	#define FA_SYSTEM _A_SYSTEM
	#define FA_DIREC  _A_SUBDIR

	typedef struct {
		 LONG Handle;
		 INT AttribSel;
		 /*
#ifdef UNICODE
		 struct _wfinddata_t ffile;
#else
		 struct _finddata_t ffile;
#endif
*/
		 CHAR  *ff_name;
		 INT   ff_attrib;
		 CHAR   ff_date[9];
	} FFBLK;
#endif

	typedef struct {

		UTF8 *	pszName;

	#ifdef __windows__
		HANDLE	hFile;
		DWORD	dwRow;
		INT64	tiPos;
    #else
        FILE * chFile;
	#endif

	} EH_FILE;

	typedef enum {	// FO_PARAM - FileOpen Paramaters
		FO_READ=1,
		FO_WRITE=2,
		FO_CREATE_ALWAYS=4,
		FO_OPEN_EXCLUSIVE=8
	} FO_PARAM;

	EH_FILE *	fileOpen(UTF8 *psNomeFile,FO_PARAM dwParam);
	INT64		fileRead(EH_FILE *psFile,void *pbDest,INT64 tRead);
	SIZE_T		fileGetEx(EH_FILE *psFile,void * pbMemo,SIZE_T tSizeMemo,CHAR *pszEOR,BOOL *pbNotEOR);
	#define		fileGet(a,b,c) fileGetEx(a,b,c,NULL,NULL)

	BOOL		fileSearchIn(	EH_FILE *	psFile,
								BOOL		bBinary,	// 0=Testo, 1 =Binario
								BYTE *		pSearchEntity,
								SIZE_T		tiSizeEntity,
								INT64 *		ptiPos,
								BYTE *		pBuffer,
								INT64		tiSizeBuffer);

	BOOL		fileSetPos(EH_FILE *psFile,INT64 tiPos,INT iMethod);
	SIZE_T		fileWrite(EH_FILE *psFile,void *pbDest,SIZE_T tWrite);
	SIZE_T		filePrint(EH_FILE *psFile,CHAR *pszFormat, ...);
	BOOL		fileClose(EH_FILE *psFile);

	/*
	INT   f_open(CHAR *nome,CHAR *tipo,FILE **ch);
	INT   f_close(FILE *ch);
	INT   f_put(FILE *ch,LONG location,void *sorg,UINT dim);
	INT   f_get(FILE *ch,LONG location,void *dest,UINT dim);
	INT   f_gets(FILE *ch,CHAR *buf,UINT dim);
*/

#ifdef __windows__
	INT   f_getdir(INT drive,CHAR *dir);
	INT   f_getdrive(void);
#endif

	typedef struct {
		CHAR Name[255];
		CHAR OSName[255];
		DWORD SerialNumber;
		DWORD MaxPath;
		DWORD SystemFlags;

		DWORD dwSectorsPerCluster;  // pointer to sectors per cluster
		DWORD dwBytesPerSector;  // pointer to bytes per sector
		DWORD dwNumberOfFreeClusters;// pointer to number of free clusters
		DWORD dwTotalNumberOfClusters;

#ifdef __windows__
		ULARGE_INTEGER uliFreeBytesAvailableToCaller; // receives the number of bytes on disk available to the caller
		ULARGE_INTEGER uliTotalNumberOfBytes;    // receives the number of bytes on disk
		ULARGE_INTEGER uliTotalNumberOfFreeBytes; // receives the free bytes on disk
#endif
		} VOLINFO;

	typedef struct {

		INT64	tSize; // 64bit
		TIME64	tTimeCreation;
		TIME64	tTimeAccess;
		TIME64	tTimeWrite;
		DWORD	dwOsAttrib;
		BOOL    bIsDir:1;       // E' una cartella
		BOOL    bIsLink:1;      // E' un link simbolico/collegamento
		BOOL    bIsHidden:1;    // E' un file nascosto
		BOOL    bIsSystem:1;    // E' un file è di sistema

		BOOL    bSigned:1;		// Uso interno, il file è contrassegnato

	} S_FILEDETAILS;

	typedef struct {

		UTF8	szFileName[CP_MAX_NAME];
		UTF8	* pszFullPath;
		S_FILEDETAILS sFd;

	} S_FILEINFO;
//		WCHAR	wcsFileName[MAXPATH];
//		WCHAR	*pwcFullPath;

/*
	typedef struct {

		WCHAR	wcsFileName[MAXPATH];
		WCHAR	*pwcFullPath;
		S_FILEDETAILS sFd;

	} S_FILEINFOW;
*/

	typedef enum {
		FDE_SUBFOLDER=0x0001,		// Analizza anche le sotto cartelle
		FDE_UNICODE=0x0002,			// In unicode
		FDE_DMIMODE=0x0004,			// Modalita DMI
		FDE_FULLPATH=0x0008,		// Richiedo costruizione FullPath
		FDE_RELAPATH=0x0010,		// Richiedo costruizione Percorco Relativo

		FDE_ADDFOLDER=0x0100,		// Aggiunge le cartelle trovate alla lista
		FDE_ADDHIDDEN=0x0200,		// Aggiunge i files nascosti
		FDE_ADDSYSTEM=0x0400,		// Aggiunge i files di sistema
		FDE_DELEMPTYFOLDER=0x1000	// Cancella le cartelle che si svuotano (solo in delete)
	} EN_FD_PARAM;

	BOOL   f_volumeinfo(CHAR *lpRootPathName,VOLINFO *VolInfo);
//	SIZE_T	fileSize(CHAR *file);
//	INT	fileLoad(CHAR *file,INT tipo);

	SIZE_T	fileSize(UTF8 * pszFileNameUtf);
	INT		fileTimeGet(UTF8 * pszFileNameUtf,TIME64  * ptTimeCreation, TIME64  * ptTimeAccess, TIME64  * ptTimeWrite);
	INT		fileTimeSet(UTF8 * pszFileNameUtf,TIME64  tTimeCreation, TIME64  tTimeAccess, TIME64  tTimeWrite);
//	SIZE_T	fileSizeW(WCHAR *file);
	INT	fileLoad(UTF8 * pszFileNameUtf,INT Tipo);
//	INT	fileLoad(CHAR *file,INT tipo);

//	CHAR *	fileStrRead(BYTE * pszFileNameUtf);
//	CHAR *	fileStrWrite(BYTE * pszFileNameUtf,BYTE *pszString);
//	CHAR *	FileToStringW(WCHAR *wcpFile);

	UTF8 *	filePath(UTF8 * pszFileNameUtf);
	UTF8 *	fileName(UTF8 * pszFileNameUtf);
	UTF8 *	fileNameOnly(UTF8 * pszFileName);
	UTF8 *	fileExt(UTF8 * pszFileName); // new 2010
	BOOL	fileCheck(UTF8 * pszFileName); // new 2011
	UTF8 *	fileRoot(UTF8 * utfFileSource,CHAR * pszRoot,INT iSizeRoot); // 2012
	UTF8 *	fileNameCat(UTF8 * pszFileSource,CHAR * pszAddString); // new 2011
	BOOL	fileRemove(UTF8 * pszFileNameUtf);
	BOOL	fileCopy(UTF8 * pszFileSource,UTF8 * pszFileDest,BOOL bFailIfExists);
	BOOL	fileMove(UTF8 * pszFileSource,UTF8 * pszFileDest);
	BOOL	fileRename(UTF8 * pszOldName,UTF8 * pszNewName);
	void	fileView(CHAR * pszUtfFileName);

	// command line options
	typedef enum {
		COPY_NO_DEREFERENCE = 0x00010000, // copy symlink link instead of target
		COPY_TRY_EXE        = 0x00020000, // on Win32, try adding '.exe' to filename
		COPY_FORCE          = 0x00040000, // override access permissions
		COPY_PERMISSIONS    = 0x00080000, // preserve mode, ownership, timestamps
		COPY_TIMESTAMPS     = 0x00100000, // preserve mode, ownership, timestamps
		COPY_RECURSIVE      = 0x00200000, // copy directories
		COPY_UPDATE_ONLY    = 0x00400000, // only copy if source file is newer
		COPY_VERBOSE_MASK   = 0x000000ff  // talk lots
	} EN_COPY_OPTIONS;

	INT		fileCopyEx(UTF8 * pszFileNameSource, UTF8 * pszFileNameDest, EN_COPY_OPTIONS enOptions);

	CHAR *	fileStrRead(UTF8 * pszFileNameUtf); // new 2007
	BOOL	fileStrWrite(UTF8 * pszFileNameUtf,CHAR *pszString); // New 2007
	BOOL	fileStrWriteSure(UTF8 * pszFileName,CHAR * pszString,INT iTempt,INT dwPauseMs);

	BOOL	fileStrAppend(UTF8 * pszFileNameUtf,CHAR *lpString); // new 2007

	BYTE *	fileMemoReadEx(UTF8 * pszFileNameUtf,SIZE_T *ptSize,FO_PARAM dwParam); // new 2010
#define		fileMemoRead(a,b) fileMemoReadEx(a,b,FO_READ)

	BOOL	fileMemoWriteEx(UTF8 * pszFileNameUtf,void *pPtr,SIZE_T tSize,FO_PARAM dwParam); // new 2010
#define		fileMemoWrite(a,b,c) fileMemoWriteEx(a,b,c,FO_WRITE|FO_CREATE_ALWAYS)

	//
	//	File Temporanei
	//
	typedef struct {
		FILE * ch;
		UTF8 * pszFileName;
	} EH_FILETEMP;
	EH_FILETEMP * fileTempOpen(void); // new 2009
	BYTE *		fileTempClose(EH_FILETEMP * psFileTemp,BOOL bGetFile,SIZE_T * ptSize);
	INT		fileTempName(UTF8 * pszFolder, // Cartella dove crearlo se NULL è quella temporanea standard
							 UTF8 * pPrefix,
							 UTF8 * pTempFile, // Nome del file generato
							 BOOL   bCrea); // Crea il file

//	WCHAR *	filePathW(WCHAR *file); // new 2010
//	WCHAR *	fileNameW(WCHAR *file); // new 2010
//	WCHAR *	fileExtW(WCHAR *pFile); // new 2010
//	BOOL	fileCheckW(WCHAR *pFile); // new 2011
//	BOOL	fileRemoveW(WCHAR *pwcFile);

	//
	// Gestione Directory
	//

	typedef struct {
        INT iCount;
		S_FILEINFO sFileInfo;			// Informazioni file di Easyhand

#ifdef __windows__
		WCHAR *	pwcFolderDir;
		BOOL	bTranslate;		// T/F (true Default) traduce nel formato Easyhand i parametri del file
//		struct	_wfinddata64i32_t sFind;	// Informazioni file di Windows
		struct	_wfinddata64_t sFind;
//		LONG	lStream;
		intptr_t lStream;
#endif

#if defined(__linux__)||defined(__apple__)
		CHAR *	pszDirectory;
		CHAR *	pszWildCard;
		BOOL	bTranslate;		// T/F (true Default) traduce nel formato Easyhand i parametri del file
		dev_t   sDev;

    #ifndef DIROPEN_FTS
        DIR *   psDIR;
        struct  stat sStat;
        struct  dirent * psDirent;
        CHAR *  pszNameBuffer;  // Buffer dove scrivere il nome del file con il percorso completo
//        CHAR *  pszNamePoint;   // Punto dove scrivere il nome del file
    #else
        CHAR ** arFts;
        FTS *   ftsTree;
        FTSENT* ftsNode;
    #endif

#endif
	} EH_DIR;

//
// fileDirOpen() - Apro directory
//
	BOOL fileDirOpen(UTF8 * pszDirJollyUtf,EH_DIR *psDir);
//	BOOL fileDirOpenW(WCHAR *pwcDirJolly,EH_DIR *psDir);
	BOOL fileDirGet(EH_DIR *psDir);
	void fileDirClose(EH_DIR *psDir);

	/*
	INT   f_findFirst(CHAR *fname,FFBLK *,INT attrib);
	INT   f_findNext(FFBLK *);
	void   f_findClose(FFBLK *);
*/

	typedef struct {
		EN_FD_PARAM	dwParam;
		INT64	tDateLimit;
		BOOL	(*subControl)(void *lpFolder,S_FILEINFO *psFileInfo);

		// Risultato
		SIZE_T	tCount;
		_DMI	dmiFiles;
		EH_AR	arFiles;

		CHAR *	pszFolderStart;
		INT		iFolderStartLen;

	} EH_FILEDIR;
	EH_FILEDIR * fileDirCreate(	UTF8 * pszWildCardUtf,
								CHAR * pszDate,
								EN_FD_PARAM dwParam,
								BOOL (*funcNotify)(void *lpFolder,S_FILEINFO *psFileInfo));
	void fileDirDestroy(EH_FILEDIR *);

	INT fileDelete(UTF8 * pszDirJollyUtf,CHAR * pszDateYMD,INT64 * ptiDiskFree); // Modificato 2007 file_delete()
	INT fileDeleteEx(	UTF8 * pszDirJollyUtf,
						CHAR * pszDateYMD,
						EN_FD_PARAM dwParam,
						INT64 * ptiDiskFree,
						BOOL (*funcNotify)(void *lpFolder,S_FILEINFO *psFileInfo)); // ritrona TRUE se va saltato

#ifdef __windows__
	CHAR *	fileTimeToDt(CHAR *psDt,FILETIME *psFileTime);
#endif
	BOOL	fileGetInfo(UTF8 * pszFileNameUtf,S_FILEINFO *psFileInfo);
	BOOL	fileSetDate(UTF8 * pszFileNameUtf,TIME64 tCreateFile,TIME64 tLastAccess,TIME64 tLastWrite); // 2011
	BOOL	fileSemaphore(EN_MESSAGE enMess,INT iSec,CHAR *psFile); // new 2010

/*
#define filePath(a) filePath(a) // per compatibilità
#define fileName(a) fileName(a)
*/

//	void   os_errset(INT tipo);
//	void   os_errcode(INT *code,INT *clas,INT *act,INT *locus);
//	INT   os_error(CHAR *ms1);
//	INT   os_drvtype(INT drive);
//	void   os_errvedi(CHAR *ms);
//	INT   handler(INT errval,INT ax,INT bp,INT si);
//	void os_ErrorShow(CHAR *ms,INT iError);


//	BOOL	ArgToFile(CHAR *lpFile,CHAR *Mess,...); // new 2009

//	void FileView(CHAR *File);

	INT		MsDriveType(INT iDrive); // Solo su Windows

	BOOL	dirCreate(UTF8 * pszDirectory); // new 2011
	BOOL	dirCreateFromFile(CHAR *lpFileName); // New 8/2007
	BOOL	dirRemove(UTF8 * pszDirectory); // new 2011

//
// <== FILE SECTION (END) ===========================================================
//


// =================================================================================
//
// ==> INI SECTION START
//
// =================================================================================

	void	iniSetMode(BOOL iMode);

	INT		iniGet(UTF8 *pszFileName,CHAR * pszKey,CHAR * pszValue,SINT iSizeValue);
	#define iniGetStr(a,b,c) iniGet(a,b,c,sizeof(c))
//	INT		iniGetEx(UTF8 *pszFileName,CHAR * pszKey,CHAR * pszValue,INT *piSizeValore);
	CHAR *	iniGetAlloc(UTF8 *pszFileName,CHAR * pszKey,CHAR * pszDefault); // new 2007

	void	iniSet(UTF8 *pszFileName,CHAR * pszKey,CHAR * pszValue);

	INT		iniGetInt(UTF8 *pszFileName,CHAR * pszKey,INT iDefault);
	void	iniSetInt(UTF8 *pszFileName,CHAR * pszKey,INT iValue);
	void	iniSetBin(UTF8 *pszFileName,CHAR * pszKey,void * pValue,SIZE_T tiLen);

	BOOL	iniGetBin(UTF8 *pszFileName,CHAR * pszKey,void *pbDest, SIZE_T iLen);
	void *	iniGetBinAlloc(UTF8 *pszFileName,CHAR * pszKey,SIZE_T * ptiLen);

#ifdef _WIN32
	void	iniGetRect(UTF8 *pszFileName,CHAR * pszKey,RECT *Rect);
	void	iniSetRect(UTF8 *pszFileName,CHAR * pszKey,RECT *Rect);
	BOOL	iniGetWP(UTF8 *pszFileName,CHAR * pszKey,BOOL fShow);// New 2005
	void	iniSetWP(UTF8 *pszFileName,CHAR * pszKey); // New 2005
#endif

//
// <== INI SECTION (END) ===========================================================
//



// =================================================================================
//
// ==> SERIAL COM SECTION
//
// =================================================================================
#if defined(EH_COM) || defined(EH_COMPLETED)

	/*
	#define COM1 0
	#define COM2 1
	#define COM3 2
	#define COM4 3

	#ifdef __windows__
	#define COM5 4
	#define COM6 5
	#define COM7 6
	#define COM8 7
	#endif
	*/
	
	typedef enum {

		COM_OPEN=0,
		COM_CLOSE=1,
		COM_CLOSEALL=2,
		COM_SETBUF=3,
		COM_LOCAL=4,
		COM_REMOTE=5,
		COM_ECHON=6,
		COM_ECHOFF=7,
		COM_SEND=10,
		COM_SENDDIR=11,
		COM_SENDBIN=12,
		COM_SENDLOC=20,
		COM_RICE=30,
		COM_INPUT=31,
		COM_BUFREADY=32,
		COM_CHECKBUF=33, // Solo Window
		COM_TIMEOUT=34, // Solo Window
		COM_STATUS=40,
		COM_CLEAR=50

	} EN_COM_MESS;

	typedef struct  {

			WORD dataready:1;
			WORD overrun:1;
			WORD parity:1;
			WORD frame:1;
			WORD ebreak:1;
			WORD reg_tras:1;
			WORD reg_scor:1;
			WORD timeout:1;

			WORD var_cts:1;
			WORD var_dsr:1;
			WORD riv_ring:1;
			WORD var_linea:1;
			WORD cts:1;
			WORD dsr:1;
			WORD ring:1;
			WORD linea:1;

	} S_COM_STS; // COM_STS

	INT 	comStart(void);
	void 	comEnd(void);

//
// Versione DOS
//
	#ifdef __DOS__

		struct COMINFO
		{
			 INT power; // ON/OFF se la porta ä aperta
			 INT irq;   // Irq usato
			 INT rcm;
			 INT ind;
			 CHAR *ptbuf; // Puntatore al buffer
			 INT hdlbuf;  // Handle del buffer
			 WORD buflen; // Grandezza del buffer
			 WORD bufstart; // Puntatore a coda circolare di start e di end
			 WORD bufend;
			 WORD maxinp; // Auto CR dopo maxinp caratteri
			 void interrupt (*oldirq) (void);
			 INT cd;//    Carrier Detect 		: ON=Controllo,OFF=No
			 INT dsr;//   DataTerminalReady : ON/OFF controllo
			 INT cts;//   ClearToSend       : ON/OFF controllo
			 INT echo;//  Echo Locale				: ON/OFF
		};
		struct COM_STS rs232(INT cmd,CHAR *buf,INT port);

	#endif

//
// Versione Windows
//
	#ifdef __windows__


		typedef struct 
		{
			INT		iPortNumber;	// 1=COM1, 2=COM2 etcc
			CHAR	szDeviceName[80]; // 
			CHAR	szDeviceFile[80]; // 
			BOOL	bEnable;// T/F se la porta Ë abilitata (sempre true in windows)
			BOOL	bOpen; // T/F se la porta è in uso (aperta)
			INT		rcm;
			INT		ind;
			INT		irq;
/*
			 CHAR *		ptbuf;  // Puntatore al buffer locale
//			 INT		hdlbuf;  // Handle del buffer locale
			 WORD		buflen;  // Grandezza del buffer locale
			 WORD		bufstart; // Puntatore a coda circolare di start e di end
			 WORD		bufend;
*/
			PS_CC		pccInput;
			PS_CC		pccOutput;
			INT			iOutBlock;	// Numero Byte che spedisco per volta in uscita
			INT			iTimeoutWait; // Default 200 (= 2s)
			INT			iTimeOutCounter;

			WORD		maxinp; // Auto CR dopo maxinp caratteri
			BOOL		cd;//    Carrier Detect 		: ON=Controllo,OFF=No
			BOOL		dsr;//   DataTerminalReady : ON/OFF controllo
			BOOL		bCts;//   T/F se deve usare ClearToSend 
			BOOL		echo;//  Echo Locale				: ON/OFF
			HANDLE		hCom; // File delle comunicazione
			HANDLE		hThread;// Thread attivo allo stoccaggio dei dati
			HANDLE		hObject[3]; // Objects per la sincronizzazione del Thread
			OVERLAPPED	hComEvent;
			OVERLAPPED	ovr;	// Struttura per I/O overlapped Input
			OVERLAPPED	ovw;	// Struttura per I/O overlapped OutPut

			S_COM_STS	sStatus;

		} S_COM_PORT;

		S_COM_STS *		comPort(EN_COM_MESS enMess,CHAR * pzBuf,INT iPortNumber);
		BOOL			comIsReady(INT iPortNumber);// INT			IsCommPort(INT port_com);
		S_COM_PORT *	comPortGet(INT iPortNumber);
		INT				CommSelect(INT iComPort); // Selezione di una porta

	#endif

	void test_com(INT port);
	void rs232_err(INT err,CHAR *buf);
#endif

//
// <== SERIAL COM SECTION (END) =======================================================
//

//
//	GESTIONE DEL COLORE
//

	EH_COLOR	ColorLum(LONG Color,INT Perc);
	EH_COLOR	ColorFusion(LONG ColorBase,LONG ColorNew,INT Perc);
	EH_COLOR	ColorLumRGB(LONG Color,INT PercR,INT PercG,INT PercB);
	EH_COLOR	colorWeb(CHAR * pszStr); // ColorWebConvert
	EH_ACOLOR	colorWeba(CHAR * pszStr); 
	EH_COLOR	ColorGray(INT Perc);
	DWORD		colorCMYKtoRGB(DWORD dwCMYK); // new 2012
	EH_COLOR	colorToGray(EH_COLOR col); // 2013
#define			colorReal(col) ((double) (col&0xFF)/255)
	#define gdi_AlphaShift 24
	#define gdi_RedShift 16
	#define gdi_GreenShift 8
	#define gdi_BlueShift 0
	#define AlphaRGB(a,r,g,b) ( ((b) << gdi_BlueShift) | ((g) << gdi_GreenShift) | ((r) << gdi_RedShift) | ((a) << gdi_AlphaShift) )
	#define AlphaColor(a,b) ( ((a) << gdi_AlphaShift) | ((b&0xff0000)>>16) | (b&0xff00) | ((b&0xff)<<16))
	#define rgba(r,g,b,a) ( ((BYTE) (b*255.0) << gdi_BlueShift) | ((BYTE) (g*255.0) << gdi_GreenShift) | ((BYTE) (r*255.0) << gdi_RedShift) | ((BYTE) (a*255.0) << gdi_AlphaShift) )

	// gray = 0.3 × red + 0.59 × green + 0.11 × blue (RGB)
	// gray = 1.0 - min(1.0, 0.3 × cyan + 0.59 × magenta + 0.11 × yellow + black ) (CMYK)

#define GrayConvertRGB(r,g,b) (((DWORD) r*27+ (DWORD) g*86+ (DWORD) b*51)/164)
#define GrayConvert(color) ((GetBValue(color)*27+GetGValue(color)*86+GetRValue(color)*51)/164)

	RECT *	rectFill(RECT *rc,INT iLeft,INT iTop,INT iRight,INT iBottom);
	RECTD * rectFillD(RECTD *rc,double dLeft,double dTop,double dRight,double dBottom);
	RECTD * rectToD(RECTD * psdRect,RECT * psRect);
	RECT *	rectToI(RECT * psdRect,RECTD * psRect);
	void	rectAdjust(RECT *rcRect);
	void	rectAdjustD(RECTD *rcRect);
	SIZE *  sizeCalc(SIZE *psSize,RECT *psRect);
	SIZED * sizeCalcD(SIZED *psSize,RECTD *psRect);
	


// =================================================================================
//
// ==> ARRAY START
//
// =================================================================================

// new 9/2007
//#define EH_AR BYTE **

#ifndef EH_MEMO_DEBUG
	EH_AR	ARCreate(CHAR *lpString,CHAR *lpSepm,INT *piRows);
	EH_ARF	ARFSplit(CHAR *lpString,CHAR *lpSep); //CHAR **ARSplitC(CHAR *lpStringConst,CHAR *lpSep); // La stringa è una costante

	#define ARNew() ARCreate(NULL,NULL,NULL) // new 2008
#else
	EH_AR _ARCreateEx(CHAR *lpString, CHAR *lpSepm, INT *piRows EMD_END_FUNC);
	EH_AR _ARFSplit(CHAR *lpString,CHAR *lpSep EMD_END_FUNC);
	#define ARCreate(a,b,c) _ARCreateEx(a,b,c,__FILE__,__LINE__)
	#define ARFSplit(a,b) _ARFSplit(a,b,__FILE__,__LINE__)
	#define ARNew() _ARCreateEx(NULL,NULL,NULL,__FILE__,__LINE__) // new 2008
#endif

	INT		ARLen(EH_AR ar);
	EH_ARF	ARFClone(EH_ARF ar,CHAR *lpEnd);
	EH_ARF	ARFDistinct(EH_ARF ar,BOOL fCase);
	void	ARSort(EH_ARF ar,INT iOrder); // 0 =asceding

	EH_AR	ARDestroy(EH_AR);
	void	ARClean(EH_AR *par);	// Svuota l'array 2010

	CHAR *	ARAdd(EH_AR *pAr,CHAR *pString); // new 2008
	CHAR *	ARAddU(EH_AR *pAr,CHAR *pString,BOOL bCaseUnsensitive); // 2010
	CHAR *	ARAddarg(EH_AR *pAr,CHAR *pString,...); // new 2008
	CHAR *  ARUpdate(EH_AR ar,INT idx,CHAR *pString,...); // new 2008/12 ex arRowUpdate
	CHAR *  ARIns(EH_AR *pAr,INT idx,CHAR *pString,...); // new 2009/8/8
	void	ARDel(EH_AR *pAr,INT idx); // new 2009/8
	EH_AR	ARDup(EH_AR arSource); // new 2009

	double	ARGetNum(EH_AR arDati,INT iRow,double dDefault);
	INT		ARGetInt(EH_AR arDati,INT iRow,INT iDefault);
	CHAR *	ARGetPtr(EH_AR arDati,INT iRow,CHAR *pDefault);
	CHAR *	ARToString(EH_AR arAcc,CHAR *pdzDelim,CHAR *pszStart,CHAR *pszStop); // new 2008
	INT		ARGetIdx(EH_AR ar,CHAR *pSearch,BOOL bCaseUnsensitive); // 2010 > ritorna l'indice della pozione della stringa
	CHAR *	ARIsIn(EH_AR ar,CHAR *pSearch,BOOL bCaseUnsensitive); // NULL non esiste 2010
	BOOL	ARIsStr(EH_AR ar,CHAR *pString,BOOL bCaseUnsensitive); // new 2009
	CHAR *	ARSearch(EH_AR ar,CHAR *pString,BOOL bCaseUnsensitive); // new 2009
	INT		ARSearchIdx(EH_AR ar,CHAR *pString,BOOL bCaseSensitive); // new 2009
	void	ARAddAr(EH_AR *pAr,EH_AR arSource,INT iArgs); // new 2010
	CHAR *	ARSearchBegin(EH_AR ar,CHAR *pString,BOOL bCaseSensitive,BOOL bAfterString); // new 2009
	CHAR *  ARWildMatch(EH_AR ar,CHAR * pszWildMatch,INT * pidx); // new 2012
	CHAR *	ARWildCheck(EH_AR arPatterns,CHAR * pszString,INT * pidx); // new 2012
	EH_ARF	ARToARF(EH_AR arSource,BOOL bFreeSource); // new 2012
	
	//
	// CUT ARRAY
	//
typedef struct {
	CHAR *	pFieldName;
	DWORD	dwSize;
	INT		iProcess; // 0=Nulla, 1=Trim, 2=Cobol 2 decimal

	CHAR *	pValue;
} S_CUTARRAY;

	BOOL	ARCutFill(S_CUTARRAY *arCut,CHAR *lpSource);
	void	ARCutFree(S_CUTARRAY *arCut);
	CHAR *	ARCutGet(S_CUTARRAY *arCut,CHAR *pName);

	//
	// ARG Array
	//
	BOOL	ARArgStr(EH_AR arArg,CHAR *pszWord,CHAR **ptr);
	BOOL	ARArgInt(EH_AR arArg,CHAR *pszWord,INT *pInt);
	void	ARPrint(EH_AR ar);
	EH_AR	ARFromTxt(UTF8 * pszFileName,BOOL bNotEmptyLines); // new 2012
	void	ARMixing(EH_AR ar); // new 2012
	#define ARArgFound(a,b)	ARIsIn(a,b,TRUE)


	//
	// AD (Agregate Data API) - New 2009/2
	//
	typedef CHAR * EH_AD;
	typedef EH_AD * EH_ADPTR;
	void	ADWrite(EH_ADPTR ppAggregato,CHAR *pCode,INT iValue); // <-- Modifica l'aggregato
	void	ADInc(EH_ADPTR ppAgregato,CHAR *pCode); // <-- Modifica l'aggregato
	void	ADAdd(EH_ADPTR padAgregato,CHAR *pCode,INT iValueAdd);
	BOOL	ADRead(EH_AD ppAggregato,CHAR *pCode,INT *piValue);
	INT		ADGet(EH_AD pAggregato,CHAR *pName,INT iDefault); // Legge il valore
	CHAR *	ADPtr(EH_AD pAggregato,CHAR *pCode);
	EH_AD	ADSum(	EH_AD adAggA, // Prima Aggregato
					EH_AD adAggB, // Secondo Aggregato
					BOOL bFreeA);

	void	ADSort(	EH_AD *psAd,	// new 2010
					INT iWhat,		// 0=Codice,1=Numero
					INT iOrder);	// 0 =asceding,1=Descending
	EH_AR	ADArrayCode(EH_AD adAgg);
	EH_AD	ADUnion(EH_AD padAgregatoA,EH_AD padAgregatoB); // 2010


	//
	// LST Liste dati 2012
	//

	struct ehLstItem {
		union {

			CHAR *	pszStr;
			void *	ptr;
		};
		struct ehLstItem * psNext;
	};
	typedef struct ehLstItem EH_LST_I;

	typedef struct {

		INT			iLength;
		INT			iSize;
		BOOL		bLop;	// T/F Lista di puntatori (settato in automatico on lstPushPtr() 
		EH_LST_I *	psFirst;
		EH_LST_I *	psLast;
		
		EH_LST_I *	psCurrent;	// Per i loop
		BOOL		bNextFirst;	// Uso interno: il next riparte dal primo

	} EH_LST_S;
	
	typedef EH_LST_S * EH_LST;
	
	#ifdef EH_MEMO_DEBUG
		EH_LST 		_lstCreate(SIZE_T iMemo EMD_END_FUNC);
		#define		lstCreate(a) _lstCreate(a,__FILE__,__LINE__)
	#else
		EH_LST 		lstCreate(SIZE_T iMemo);
	#endif

	#define		lstNew()	lstCreate(0)
	void		lstClean(EH_LST psList);
	EH_LST 		lstDestroy(EH_LST psList);
	
	void *		lstPush(EH_LST psList,void * ps);
	void *		lstPushf(EH_LST psList,CHAR *pString,...);
	void *		lstPushPtr(EH_LST psList,void * ptr);
	void *		lstPop(EH_LST psList);
	BOOL		lstRemoveI(EH_LST psList,EH_LST_I * psElement);

//	#define 	lstPushf(lst,frm,...) lstPushf(lst,fom,__VA_ARGS__)

	void *		lstInsf(EH_LST psList,INT idx,CHAR *pString,...);
	void *		lstGet(EH_LST psList,INT idx);
	void *		lstFirst(EH_LST psList);
	void *		lstLast(EH_LST psList);

	void *		lstNext(EH_LST psList);
	EH_LST_I *  lstSearch(EH_LST psList,void * pValue);
	void		lstPrint(EH_LST psList);
#ifndef EH_MEMO_DEBUG
	EH_LST		lstDup(EH_LST lstSource);
#else
	EH_LST		_lstDup(EH_LST lstSource,CHAR * pszProgram,INT iLine);
	#define		lstDup(psList) _lstDup(psList,__FILE__,__LINE__) 

#endif
	BOOL		lstIsIn(EH_LST psList,void * pElement);
	void		lstSort(EH_LST psList, int (*compare )(const void *elem1, const void *elem2 ));
	#define		lstLoop(lst,psEle) psEle=lstFirst(lst);psEle;psEle=lstNext(lst)
#define		lstForSafe(lst,psEle,lstI) lstI=lst->psFirst; for (lstI=lst->psFirst,psEle=lstI?lstI->ptr:NULL;lstI;lstI=lstI->psNext,psEle=lstI?lstI->ptr:NULL)

#ifndef EH_MEMO_DEBUG
	CHAR *		lstToString(EH_LST psList,CHAR *pdzDelim,CHAR *pszStart,CHAR *pszStop); 
#else
	CHAR *		_lstToString(EH_LST psList,CHAR *pdzDelim,CHAR *pszStart,CHAR *pszStop,CHAR * pszProg,INT iLine);
	#define		lstToString(psList,pdzDelim,pszStart,pszStop) _lstToString(psList,pdzDelim,pszStart,pszStop,__FILE__,__LINE__) 
#endif
	void **		lstToArf(EH_LST psList,BOOL bFreeList);
	EH_LST		lstFrag(CHAR * pszString,INT * ariSize);
	EH_LST		lstFromTxt(EH_LST lst,UTF8 * pszFileName,BOOL bNotEmptyLines);

	
	// LAD -- (liste di aggregati)
	typedef struct {

		CHAR *	pszCode;
		INT		iQta;
		double	dQta;		
	} S_LAD;
	typedef S_LAD * LADPTR;
	#define ladCreate() lstCreate(sizeof(S_LAD))
	LADPTR ladGet(EH_LST lst,CHAR * pszCode);
	LADPTR ladAddi(EH_LST lst,CHAR * pszCode,INT iQta);
	LADPTR ladAdd(EH_LST lst,CHAR * pszCode,double dQta);
	EH_LST ladDestroy(EH_LST lst);

//
// <== ARRAY (END) =======================================================
//



// =================================================================================
//
// ==> ENCODING START
//
// =================================================================================

#define SEN_MAX_STRING 1000

typedef struct {
	INT iNum;
	CHAR *arStr[SEN_MAX_STRING];
} S_SEN;

typedef enum {
	// Encoding
	SE_FERRA=		0x0001,	// Ferrà Encoding usato in Create/Imagine ed altro gino{1}

	SE_HTML=		0x0002,	// HTML ISO-8859-1 Es (&#128;) Converte TUTTO <>& e anche i ritorni a capo in <br>
	SE_HTML_CODE=	0x0003,	// (new 2010) Converte Tutto ma usando solo i codici &#<xxx>;
	SE_HTMLS=		0x0008,	// HTML ISO-8859-1 Lascia la struttura <> e converte solo i caratteri nono ASCII (solo i tag che contegono #)
	SE_ISO_LATIN1=  0x0200,	// HTML ISO-8859-1 CONVERTE TUTTO TRANNE i ritorni a capo
	SE_HTML_XML=	0x0201,	// HTML ISO-8859-1 Converte il <>&

	SE_WTC=			0x0004,	// Wide To Char (converte perdendo il secondo byte un Widechar<>Char : Nessun encoding
	SE_ANSI=		0x0005,	// Wide To Char Ansi
	SE_UNICODE=		0x0006,	// Unicode
	SE_CFORMAT=		0x0010,	// C Format \1 \2 \t
	SE_JSON=		0x0011,	// JSON (Javascript Format) \u &#  \1 \2 \t
	SE_CQUOTE=		0x0012,	// Quote (Usato i Php e altro solo ' e ")
	SE_UTF8=		0x0020,	// UTF-8 usato in Xml ed adottato da Google
	SE_SQL=			0x0040,	// Mette il doppio apice dove serve
	SE_SQLSTR=		0x0041,	// Idem come sopra, ma restituisce NULL oppure '<stringa passata codificata>'
	SE_SQLMY=		0x0042,	// Mette il doppio apice dove serve
	SE_SQLMYSTR=	0x0043,	// Idem come sopra, ma restituisce NULL oppure '<stringa passata codificata>'
	SE_URL=			0x0080,	// URL Internet (space = +)
	SE_URLSPC=		0x0081,	// Come Sopra ma space = %20
	SE_BASE64=		0x0100, // Base 64
	SE_BASE64MAIL=	0x0101, // Base 64 (a capo ogni 77 caratteri)
	SE_QP=			0x0400, // Quote-printable
	SE_EBCDIC_ASCII=0x0500, // IMB EBCDIC Us-Ascii
	SE_EBCDIC_ISO=	0x0501, // IMB EBCDIC ISO-8859-1

	// Special
	SE_AS400=		0x0801,	// Codifica per AS400 non UTF8

	// Decoding
	SD_FERRA=		0x1001,	// Ferrà Encoding usato in Create/Imagine ed altro gino{1}

	SD_HTML=		0x1002,	// HTML ISO-8859-1 Es (&#128;) Converte TUTTO <>& e anche i ritorni a capo in <br>
	SD_HTML_CODE=	0x1003,	// (new 2010) Converte Tutto ma usando solo i codici &#<xxx>;
	SD_HTMLS=		0x1008,	// HTML ISO-8859-1 Lascia la struttura <> e converte solo i caratteri nono ASCII (solo i tag che contegono #)
	SD_ISO_LATIN1=  0x1200,	// HTML ISO-8859-1 CONVERTE TUTTO TRANNE i ritorni a capo
	SD_HTML_XML=	0x1201,	// HTML ISO-8859-1 Converte il <>&

	SD_WTC=			0x1004,	// Wide To Char (converte perdendo il secondo byte un Widechar<>Char : Nessun encoding
	SD_ANSI=		0x1004,	// Wide To Char Ansi
	SD_UNICODE=		0x1006, //
	SD_CFORMAT=		0x1010,	// C Format \1 \2 \t
	SD_JSON=		0x1011,	// Jason (Javascript Format) \u &#  \1 \2 \t
	SD_UTF8=		0x1020,	// UTF-8 usato in Xml ed adottato da Google
	SD_SQL=			0x1040,	// Mette il doppio apice dove serve
	SD_SQLSTR=		0x1041,	// Idem come sopra, ma restituisce NULL oppure '<stringa passata codificata>'
	SD_URL=			0x1080,	// %20 URL Internet
	SD_BASE64=		0x1100, // Base 64
	SD_BASE64MAIL=	0x1101, // Base 64 (a capo ogni 77 caratteri)
	SD_QP=			0x1400, // Quote-printable
	SD_EBCDIC_ASCII=0x1500, // IMB EBCDIC Us-Ascii
	SD_EBCDIC_ISO=	0x1501, // IMB EBCDIC ISO-8859-1

} EN_STRENC;

//#define SE_MASK    0x0FFF // Maskera Encoding Base

void encSqlMode(BOOL bSlash);

// Codifica
#ifndef EH_MEMO_DEBUG
	void * strEncode(CHAR *pSource,EN_STRENC iMode,INT *pType);
	void * strDecode(CHAR *lp,EN_STRENC iMode,INT *pType);
	void * strEncodeW(WCHAR *lp,EN_STRENC iMode,INT *pType);
	void * strDecodeW(WCHAR *lp,EN_STRENC iMode,INT *pType);
	void * strEnc(INT iSize,void *pSource,EN_STRENC enEnc,INT *pType); // 2010
#else

	void *_strEncode(CHAR *pSource,INT iMode,INT *pType,CHAR *pProg,INT iLine);
	#define strEncode(a,b,c) _strEncode(a,b,c,__FILE__,__LINE__)

	void *_strDecode(CHAR *pSource,INT iMode,INT *pType,CHAR *pProg,INT iLine);
	#define strDecode(a,b,c) _strDecode(a,b,c,__FILE__,__LINE__)

	void *_strEncodeW(WCHAR *lp,INT iMode,INT *pType,CHAR *pProg,INT iLine);
	#define strEncodeW(a,b,c) _strEncodeW(a,b,c,__FILE__,__LINE__)

	void *_strDecodeW(WCHAR *lp,INT iMode,INT *pType,CHAR *pProg,INT iLine);
	#define strDecodeW(a,b,c) _strDecodeW(a,b,c,__FILE__,__LINE__)

	void *_strEnc(INT iSize,void *pSource,EN_STRENC enEnc,INT *piType,CHAR *pFile,INT iLine); // 2010
	#define strEnc(iSize,pSource,enEnc,piType) _strEnc(iSize,pSource,enEnc,piType,__FILE__,__LINE__)

#endif

void *	strEncodeEx(INT iSizeInput, // 0=Char 1=Wchar
				  void *pString,
				  INT iNum,...);

#define utfToWcs(a) (WCHAR *) strDecode(a,SE_UTF8,NULL)
//#define strToUtf(pszFileName);
CHAR *	base64Encode(INT iMode,void * lpSource,SIZE_T tSizeSource);
void *	base64Decode(CHAR * pszSource,SIZE_T *ptSizeRet);
CHAR *	hexEncode(void * lpSource,SIZE_T tSizeSource);
void *	hexDecode(CHAR * pszSource,SIZE_T *ptSizeRet);

BYTE *	Ansi2Ascii7(BYTE *lpString); // Toglie accenti vari
BOOL	isAscii(CHAR * pszString,BOOL bModeSpace); // new 2009/2010
BOOL	isAsciiW(WCHAR * pwcString,BOOL bModeSpace);
BOOL	isUtf8(CHAR * pszSource,BOOL bFalseWithError); // 2012
CHAR *	strSwapSql(CHAR **ppsz,BOOL bQuote); // new 2008 Ex void SqlFormatSwap(BYTE **arString); // New 2007
CHAR *	strSwapUtf(CHAR **ppsz); // new 2008

CHAR *	strEncrypt(CHAR * pszSource,CHAR * pszKey);
CHAR *	strDecrypt(CHAR * pszSource,CHAR * pszKey);

typedef enum {

	BOM_UNKNOW,
	BOM_UTF_32_BIG_ENDIAN,
	BOM_UTF_32_LITTLE_ENDIAN,
	BOM_UTF_16_BIG_ENDIAN,
	BOM_UTF_16_LITTLE_ENDIAN,
	BOM_UTF_8

} EN_BOM;

EN_BOM bomDecode(CHAR *pszBuffer,DWORD dwBuffer);


// Stack Encoding Server

S_SEN *	senCreate(void);
CHAR *	senEncode(S_SEN *pSen,INT iEncode,CHAR *lpStr);
#define sqlStr(psSen,str) senEncode(psSen,SE_SQL,str)
#define senSql(str) senEncode(psSen,SE_SQL,str)
void *	senEncodeEx(S_SEN *pSen,
				  INT iInputStart,
				  void *pString,
				  INT iNum,...); // new 2008 (quasi 2009)
void	senDestroy(S_SEN *pSen);

//
// Sql Section
//
CHAR * strWordToQueryLike(CHAR *pWordIn,CHAR *pField,BOOL bUpperCase);


//
// JSON Decoding
//
typedef enum {
	JSON_UNKNOW,
	JSON_STRING,
	JSON_NUMBER,
	JSON_OBJ,
	JSON_ARRAY,
	JSON_ERROR

} EN_JSON_TYPE;


typedef struct {

	EN_JSON_TYPE enType; // 0=Simple Element
	CHAR *	pszName;	// Nome dell'elemento
	EH_LST	lstJson;	// Lista degli elementi (caso di array)

	UTF8 *	utfValue;
	WCHAR * pwcValue;
	INT		iValueError;

	void *  psParent;
	void *  psNext;
	void *  psChild;

	CHAR *	pszEnd;		// <- uso interno 

} EH_JSON;

EH_JSON *	jsonCreate(CHAR * pszString);
EH_JSON *	jsonDestroy(EH_JSON * psJson);
CHAR *		jsonGet(EH_JSON * psJson,CHAR *pszCode);
CHAR *		jsonGetd(EH_JSON * psJson,CHAR *pszCode,CHAR * pszDefault);
CHAR *		jsonGetf(EH_JSON * psJson,CHAR * pszFormat, ...);
CHAR *		jsonGetEx(EH_JSON * psJson,CHAR * pszCode,EH_JSON ** ppsElement);
S_UNIVAL *	jsonGetVal(EH_JSON * psJson,CHAR * pszFormat, ...);
void		jsonPrint(EH_JSON * psJson,INT iIndent);
CHAR *		jsonCleanSource(CHAR * pszSource,BOOL bComment,BOOL bCrLfTab);



//
// <== ENCODING (END) =======================================================
//


// =================================================================================
//
// ==> FONT SECTION
//
// =================================================================================
	#define FONT_STANDARD sys.pFontStandardName,sys.hFontStandardHandle,sys.iFontStandardHeight,STYLE_NORMAL
//	void	Font_Start(void);
//	void	Font_End(void);
	BOOL		fontSearch(CHAR *	pszFontFace, INT iStyles);

	EH_FONT *	fontCreate(	CHAR *	pFontFace, // Font name
							INT		iAlt,		// Altezza
							INT		iStyles, // Stili
							BOOL	bNotStore, // 2010 = Chiedo di non memorizzarlo nei fonti in memoria
							BOOL *	fAllocated, // T/F se il font Ë stato allocato ed Ë da liberare
							INT *	idxFont);

	#ifdef EH_MEMO_DEBUG
	#define		fontCreate(a,b,c,d,e,f) _fontCreate(a,b,c,d,e,f,__FILE__,__LINE__)
	EH_FONT *	_fontCreate(
	#else
	EH_FONT *	fontCreate(	
	#endif
					CHAR *	pszFontFace, // Font name
					INT		iHeight,	 // Altezza
					INT		iStyles,	 // Stili

					BOOL	bAlloc,		// 2010 = Chiedo di non memorizzarlo nei font in memoria (dovrà essere liberato a mano)
					BOOL *	pbNotCached, // T/F se il font è stato allocato ed è da liberare
					INT *	pidxFont
					EMD_END_FUNC);



	EH_FONT *	fontCreateEx(CHAR *	pszFontFace, // Font name
							INT	iHeight,	 // Altezza
							INT	iWidth,		 // Larghezza
							INT	iWeight,	 // Peso
							INT	iStyles,	 // Stili
							INT	iCharExtra,
							INT	iRotation,
							INT	iPitch);
	void *		fontDestroy(EH_FONT *psFont,BOOL bFreeThis);
	BOOL		fontInstall(UTF8 * pszFileName,BOOL bSilenceMode);
	BOOL		fontUninstall(UTF8 * pszFileName);
	EH_FONT *	fontPtr(INT idxFont);

	void		fontGetSize(CHAR *lpTextOriginal,DWORD dwLen,EH_FONT *psFont,SIZE *psSize); // new 2010
	void		dcFontGetSize(HDC hdc,INT iCharByte,void *pTextOriginal,DWORD dwLen,EH_FONT *psFont,SIZE *psSize,TEXTMETRIC * psText);
	INT			font_lenh	(CHAR *lpTextOriginal,DWORD dwChar,EH_FONT *psFont);//,INT Nfi,INT iStyles)

	INT 		font_dim	(CHAR *lpText,DWORD dwChar,INT idxFont);//,INT nfi,INT iStyles);
	INT 		font_len	(CHAR *lpText,INT idxFont);//,INT nfi,INT iStyles);
	INT 		font_alt	(INT idxFont);//,INT nfi,INT iStyles);

	INT 		font_lenf	(CHAR *lpText,CHAR *font,INT iAlt,INT iStyles);
	INT 		font_altf   (CHAR *font,INT iAlt,INT iStyles);
	INT			fontFind(CHAR *pFontName,INT iAlt,INT iStyles);
	BOOL		fontLock(INT idx,BOOL bLock);
	INT			font_style(INT idxFont,INT iStyles);

	INT			Wfont_altChar(LOGFONT *LogFont); // Conversione da pitch a pixel
	INT			Wfont_alt(LOGFONT *LogFont); // Altezza reale del carattere in pixel
	INT			Wfont_dim(CHAR *Str,UINT Num,LOGFONT *LogFont);
	INT			Wfont_len(CHAR *Str,LOGFONT *LogFont);
	BOOL		WChooseFont(LOGFONT *LogFont);
//
// <== FONT SECTION (END) =======================================================
//



//
//	SUPPORTO BITMAP
//

#ifdef __windows__	
	HBITMAP winBitmapBuilder(DWORD cx,DWORD cy,INT iColorDepthBit,RGBQUAD *psPalette, BYTE **ppBits); // new 2010
	HBITMAP winBitmapConvert(HBITMAP hBitmap,INT iColorDepthBit,RGBQUAD *pPalette); // new 2010
	void winBitmapAlphaFactor(INT cx,INT cy,BYTE *pbImage,BYTE *pbBits); // new 2010
#endif

	POINT * arPoint(INT iPoints,...); // new 2011
	POINTD * arPointd(INT iPoints,...); // new 2011

//
//	SUPPORTO DELLA GRAFICA (NO MODALITA CONSOLE)
//

#define rectCopy(a,b) memcpy(&a,&b,sizeof(RECT))
#define rectCopyD(a,b) memcpy(&a,&b,sizeof(RECTD))


#if !defined(EH_CONSOLE)
#define EH_GDI_API

#if (!defined(__cplusplus)&&defined(__windows__))
	#include "/easyhand/inc/ehAnimation.h"
#endif

	// =================================================================================
	//
	// ==> IN SECTION (Tastiera e mouse)
	//
	// =================================================================================

	// Macro

	#define IN_SX 1
	#define IN_DX 2
	#define IN_SXR 4
	#define IN_DXR 8
	#define IN_DBLSX 20
	#define IN_DBLDX 24
	#define IN_KEY -1
	#define IN_OBJ -2
	#define IN_KEYBREAK -3
	//#define IN_MW_UP 64
	//#define IN_MW_DOWN 65

	#define KEY_F1   59
	#define KEY_F2   60
	#define KEY_F3   61
	#define KEY_F4   62
	#define KEY_F5   63
	#define KEY_F6   64
	#define KEY_F7   65
	#define KEY_F8   66
	#define KEY_F9   67
	#define KEY_F10  68
	#define KEY_F11  69
	#define KEY_F12  70

	#define _CANC 'S'
	#define _INS  'R'
	#define _END  'O'
	#define _HOME 'G'
	#define _FDX  'M'
	#define _FSX  'K'
	#define _FUP  'H'
	#define _FDN  'P'
	#define _PGUP 'I'
	#define _PGDN 'Q'

	#define	_SHIFTDX    1
	#define	_SHIFTSX    2
	#define	_CTRL       4
	#define	_ALT        8
	#define	_BLOCSCORR  16
	#define	_BLOCNUM    32
	#define	_BLOCMAIUS  64
	#define	_INSERTON   128
	#define	_ANYSHIFT   3

	BOOL in_start(void);
	void in_end(void);

	// Mouse
	void mouse_on(void); //  Abilita la visualizzazione del cursore
	void mouse_off(void); // Disabilita ^

	void mouse_set(INT x,INT y); // Setta la posizione del cursore
	void mouse_prs(INT x); // Rileva ultimo tasto premuto(NON FUNGE *)
	void mouse_rel(INT x); // Rileva ultimo tasto rilascato(IDEM ^)
	void mouse_rangex(INT x1,INT x2);
	void mouse_rangey(INT y1,INT y2);
	void mouse_range(INT x1,INT y1,INT x2,INT y2); // Definisce RANGE cursore
	INT mouse_graph(INT x,INT y,CHAR *car); // Disegna il cursore grafico
	INT mouse_motion(INT a); // Legge il movimento assoluto del mouse
	void mouse_sens(INT x,INT y); // Setta la sensibilitÖ
	void mouse_vel(INT m); // VelocitÖ di spostamento

#ifndef __windows__
	void mouse_click(void);
#endif

	typedef struct {

		INT		idCode;
		INT		x,y,x2,y2;
		INT		sx,sy;
		CHAR	nome[9];
		CHAR *	pf;
		INT		dimx,dimy;
		INT		grp;
		INT		win;
		BOOL	bDisabled;

	} S_MGZ ;

	//
	//	Help Mouse Zone (ToolTip)
	//
	struct HMZ {
		CHAR *help;//[HMZSTR];
		INT x,y;
		INT x2,y2;
		INT win;
	};
 
	// Timer 2010
	void CALLBACK ehEventTimer(HWND hwnd, UINT msg, UINT uipTimer, DWORD dwTime);
	UINT_PTR ehSetTimer(BYTE *pszEvent,void	(*funcNotify)(void *),DWORD dwTimeoutMs,BOOL bInterval,void * pvParam);
	void	ehCleanTimer(INT uipTimer);

//	INT WaitEvent(HWND hWnd,BOOL bWait); // new 2008
	INT		ehWaitEvent(HWND hWnd,BOOL bWait,BOOL bGetEvent,EH_EVENT * psEvent,BOOL bOnlyOsEvent);
	void	ehWheelScroll(HWND hWnd,EH_EVENT * psEvent,SINT iFullRow); // Solo GDI

// Compatibilità con il passato

	#define eventGet(a) ehWaitEvent(WIN_info[sys.WinInputFocus].hWnd,FALSE,TRUE,a,FALSE)
	#define eventGetWait(a) ehWaitEvent(WIN_info[sys.WinInputFocus].hWnd,TRUE,TRUE,a,FALSE)

//	INT	mouse_ce(INT x1,INT y1,INT x2,INT y2);
//	INT	Amouse_ce(INT x1,INT y1,INT x2,INT y2);

	// Keyboard
	INT		key_press(CHAR car);
	INT		key_press2(CHAR car);
	BOOL	key_pressS(INT BitCheck,INT mode);
	INT		key_SGet(void);

	void	key_clean(void); // new 2008
	CHAR	key_getchar(void); // new 2008
	void	key_hit(INT iTasto); // new 2008
	void	key_hitEx(HWND hwnd,INT iTasto); // new 2008

	void	keyPutString(CHAR *pString); // new 2008
	void	keyPutStringEx(HWND hwnd,CHAR *pString); // new 2008


	//
	// Mouse Graph Zone
	//
	INT		mgzStart(void);
	void	mgzExit(void);
	INT		mgz_on(INT x,INT y,INT x2,INT y2,INT grp,INT sx,INT sy,CHAR *ico);
	INT		Amgz_on(INT x,INT y,INT x2,INT y2,INT grp,INT sx,INT sy,CHAR *ico);
	INT		mgz_off(INT x,INT y,INT x2,INT y2);
	INT		Amgz_off(INT x,INT y,INT x2,INT y2);
	INT		mgz_grpoff(INT grp);
	INT		mgz_winoff(void);
	INT		mgz_del(INT  num);
	INT		mgzRemove(INT idNume);
	BOOL	mgzEnable(INT idCode,BOOL bEnable);
	S_MGZ * mgzSearch(INT idCode,DWORD * pdwIndex);

	void	mgz_ask(void);

	INT hmz_start(void);
	void hmz_end(void);
	INT hmz_on(INT x,INT y,INT x2,INT y2,CHAR *str);
	INT Ahmz_on(INT x,INT y,INT x2,INT y2,CHAR *str);
	INT hmz_off(CHAR *help);
	INT hmz_winoff(void);
	INT hmz_del(INT num);
	void hmz_ask(INT cmd,void *ptr);
	INT hmz_obj(CHAR *nome,CHAR *str);

	//
	// Windows
	//
	#ifdef __windows__

		// void LKBWinInsert(INT16 Tasto,INT Num);
		void OsEventTranslate(INT iWin,BOOL bObj,HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

		void WinMouse(INT x,INT y,WORD b);
		void WinMouseAction(INT iWin,HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
		HWND WCreateTooltip(HWND hwndOwner);
		void TooltipAdd(INT id,INT x,INT y,INT x2,INT y2,CHAR *str);
		void TooltipDel(INT id);

		//void hmz_objdel(CHAR *nome);
		void MouseCursorDefault(void);
#define MouseCursorWait() mouse_graph(0,0,"W:WAIT")
		DWORD ehGetMouseButton(void);
		void ehSetCapture(HWND hWnd);
		void ehReleaseCapture(void);
		INT EHMouseControl(INT iMode);

		void I_KeyTraslator_Windows(HWND hwnd,UINT cmd,LPARAM lParam,WPARAM wParam); // Uso Interno
		//INT MouseWheelManager(INT cmd,INT iValue); // new 2007
	#endif

	//
	// <== IN SECTION (END) =======================================================
	//

	// =================================================================================
	//
	// ==> GDI SECTION
	//
	// =================================================================================


	typedef struct {
		BOOL	bVisible;
		HWND	hWnd;	// Finestra
		INT	x,y;	// Posizione
		INT	cx,cy;	// Dimensioni
		DWORD	dwColor;
		DWORD	dwSpeed;

		BOOL    bBlink;
		DWORD	dwCounter;
	} EH_TCURSOR;


	//
	//  FTIME
	//
	typedef struct {
		INT		idTimer;
		CHAR *		pszEvent;
		void		(*funcNotify)(void *); // 2/2008 - Funzione da usare in alternativa a Exit() in ehExit()
		TIMERPROC	lpfnMyTimerProc;
		UINT_PTR	uipTimer;
		BOOL		bInterval;
		void * pvParam;
	} S_TIMER;
	#define FTIME_MAX 10
	void FTControl(void);
	struct FTIME {           // Struttura di controllo funzioni temporali
				void (*funz)(INT cmd,void *ptr);
				INT     cont;
				INT timer;
				};
	INT	FTIME_on(void (*funz)(INT cmd,void *ptr),INT timer);
	INT	FTIME_off(void (*funz)(INT cmd,void *ptr));

	//#define STYLE_WIDTHFIX 0x50

	#define MAXCLIP 20
	#define CLIPUSER 10
	void GDI_start(void);
	void GDI_end(void);
	void GDIGetInfo(void); // new 2007
	void lineAdjust(RECT *rcRect);


		


//
// Device Graphic (2008)
//

	typedef struct {
		INT	iType;
		SIZE	sizArea;
		RECT	rcArea;

		// Windows technology
#ifdef __windows__
	#ifndef EH_MOBILE
		BITMAPV5HEADER bi;
		HDC hdc;
		HBITMAP hBitmap;
		HBITMAP hBitmapOld;
	#else
		//BITMAPHEADER bi;
		HDC hdc;
		HBITMAP hBitmap;
		HBITMAP hBitmapOld;
	#endif
#endif
		void *pBits;

	} EH_DG;

	EH_DG * dgCreate(INT iType,DWORD dwWidth,DWORD dwHeight);
	EH_DG * dgDestroy(EH_DG *psDgs);
	void dgCopy(EH_DG *psDgs,RECT *pRectDgs,INT x,INT y,INT idxWin);
	void dgCopyDC(EH_DG *psDgs,RECT *pRectDgs,INT x,INT y,HDC hdcDest); //2010
	struct CLIP {
		CHAR user[CLIPUSER];
		INT x1,y1;
		INT x2,y2;
	};

	struct LKUP { // Struttura per dichiarazione colori
			CHAR blue;
			CHAR green;
			CHAR red;
			CHAR unused;
			};

	struct BMP_HD {
	 CHAR                   bit;    //      Bit per pixel (1/4/8/24)
	 LONG                   larg;   //      Larghezza in pixel
	 LONG                   alt;            //  Altezza in pixel
	 WORD riga;   //    Larghezza in byte
	 LONG                   size;           //      Grandezza in byte
	 LONG                   col1;           // Colore del bit 1 per il B/W
	 LONG                   col0;           // Colore del bit 0 per il B/W (-1= Trasparente)
	};

	#define DBH sizeof(struct BMP_HD)


	//
	//  DOS
	//
	#ifdef __DOS__
		struct BMPINFO {
			INT 	type;
			LONG 	numbyte;
			INT 	reserved1;
			INT 	reserved2;
			LONG 	offset;
			LONG	size;
			LONG	dim_x;
			LONG	dim_y;
			INT  planes;
			INT  bitcount;
			LONG  compression;
			LONG  sizeimage;
			LONG  xpel;
			LONG  ypel;
			LONG  color_us;
			LONG  color_im;
		};
		struct CHR_HEAD {
			CHAR  id[10];  // Identificazione del file
			CHAR  ver[8];  // Versione
			CHAR  font[31]; // Nome del font MAX 30b
			CHAR  made[81]; // Nome del costruttore del font MAX 30b
			CHAR  eof;
			INT   num_font; // Numero di font contenuti
		};

		struct CHR_FONT {
			INT	chr_y;	// Altezza del carattere
			LONG    bitmap;
			LONG	tabloc;
		};

		struct CHR_CELL {
			WORD	larg;
			WORD	pos;
		};

		struct ICONE {
			CHAR nome[NOMEICONE]; //                Nome della icone
			INT  hdl;      //    Handle memory
			LONG  size;
			LONG  offset;  //                Offset di spiazzamento
			//CHAR lic[9];  //                Libreria di appartenenza
			BOOL  Lic; // 0/1 se appartiene ad una LIC
			INT grp;                //    Gruppo di appartenenza (-1=Sistema protetto)
			};

		struct LIBLIC {
			 CHAR icone[NOMEICONE];
			 LONG size;
			 LONG offset;
		};

		struct D_FILE {
				FILE *ch;
				LONG ofs_file;
				INT pt;
				INT hdl;
				LONG offset;
				};

		struct D_TXT {
				INT bmp;
				INT px,py;
				INT lx,ly;
				INT ofs_x,ofs_y;
				INT modo;
				};

		struct D_GRAF {
				INT px,py;
				INT px2,py2;
				LONG colore;
				INT modo;
				};

		struct D_MEMO {
				INT hdl,hdl2;
				LONG dest;
				WORD sgm,off;
				LONG numbyte;
				};

		struct LIC_HEAD
			 { CHAR id[10];
				 CHAR eof;
			   INT  NumIco;
			   LONG dati;
			 };

		struct ICO_HEAD {
			CHAR id[10]; // Indentificazione del file
			CHAR eof;
			CHAR bit; // Bit-Colore dell'icone
			INT dimx;// Dimensioni orizzontali
			INT dimy;// Dimensioni verticali
			INT byte;// Dimensioni in byte orizzontali
			LONG blk; // Dimensioni in byte dell'icone
			LONG ofs_mask; // Posizione della maskera
			LONG ofs_icone;  // Posizione dell'icone
			INT fpal;// Flag : ON cä la pallette OFF no
		 };

		struct VGAMEMO {
				INT hMemo;  // Handle della memoria collegata
				INT px,py;     // Coordinata in pixel dell'area di backup
				INT dimx;      // Dimensioni orizzontali
				INT dimy;      // Dimensioni verticali
				INT lenx;
				INT leny;
				INT nbx;
				LONG size;
				};
	#endif

	//
	//  WINDOWS
	//
	#ifdef __windows__
		#pragma pack(1)
		 typedef struct {
			INT hdlMemo;
			BITMAP sBitMap;
			HBITMAP hBitmap;
			HPALETTE hPalette;
		 } BITMAP_INFO;

		#pragma pack()

		LRESULT CALLBACK EH_DoPaint(HWND hWnd,WPARAM wParam,LPARAM lParam);
		void EhDirectDC(HDC hdc);
		EH_COLOR RealColor(LONG col1);
		LONG ModeColor(LONG Col);

		//
		//	BitMap
		//
		void BmpDisp(INT x1,INT y1,HBITMAP BitMap);//,BOOL fAbsolute);
		void BmpDispMask(INT x,INT y,HBITMAP BitMap,HBITMAP Mask);//,BOOL fAbsolute);
		void BmpDispEx(INT PosX,INT PosY,
					   INT Lx,INT Ly,
					   INT LxNew,INT LyNew,
					   HBITMAP BitMap,
					   BOOL Ridim);//,
					   //BOOL fAbsolute); // Ridimensiona

		HDC UsaDC(INT cmd,HDC LastDC);

		//
		// SaveBitMode functions
		//
		void DCSB_RectCopy(RECT *pRect,INT win);
		#define _SaveBit_RectChange(r) {if (sys.WinWriteFocus>-1) {if (WIN_info[sys.WinWriteFocus].dgClone) DCSB_RectCopy(&r,sys.WinWriteFocus);}}
//#define DCSB_Copy(INT x,INT y,INT x2,INT y2,INT win) {RECT rcBox; DCSB_RectCopy(rectFill(&rcBox,x,y,x2,y2),idxWin);}
#define I_Show_Bitmap(x,y,x2,y2) {if (sys.WinWriteFocus>-1) {if (WIN_info[sys.WinWriteFocus].dgClone) {RECT rcBox; DCSB_RectCopy(rectFill(&rcBox,x,y,x2,y2),sys.WinWriteFocus);}}} // DCSB_Copy(x,y,x2,y2,sys.WinWriteFocus);}}}
	//	EH_FONT *FontGetInfo(INT iNum,INT iNfi);
		INT		Tbox(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);
		INT		Tboxp(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);
		INT		Tline(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);
		INT		TPolygon(LONG cPen,LONG cBrush,INT iPoints,...); // New Fine 2006
		void	TRect(RECT *psRect,EH_COLOR cBorderPen,EH_COLOR cBrushInside);
		void	TRectRound(RECT *psRect,
						EH_COLOR cBorderPen,
						EH_COLOR cBrushInside,
						INT lRoundWidth,
						INT lRoundHeight);

		INT boxFocus(INT x1,INT y1,INT x2,INT y2);

		INT hWndToWin(HWND hWnd);
//		void Wcursor(HWND hWnd);

		INT Wdispf(INT x,INT y,LONG col1,LONG col2,LOGFONT *LogFont,CHAR *String);
		INT Tdispf(INT px,INT py,LONG colcar,LONG colbg,INT iStyles,CHAR *font,INT nfi,CHAR *str);


		//
		// Direct DC Dirette
		//
		INT		dcDisp(		HDC hdc,INT px,INT py,
							LONG colcar,LONG colbg,
							INT idxFont,
							CHAR *str);
		/*
		INT		dcDispf(	HDC hdc,
							INT px,INT py,
							EH_COLOR cText,
							EH_COLOR cBack,
							INT iStyles,
							CHAR *pFontFace,
							INT iNfi,
							CHAR *pString); // new 2007
							*/
		INT		dcDispf(	HDC hdc,
							INT px,INT py,
							EH_COLOR cText,
							EH_COLOR cBack,
							INT iStyles,
							CHAR * pszFontFace,
							INT	 iNfi,
							CHAR * pszString,
							EN_DPL enDpl);

		DWORD	dcDispNum(	HDC hdc,INT x,INT y,LONG col1,LONG col2,INT iStyles,CHAR *pFontFace,INT iNfi,INT cifre,INT dec,INT sep,BOOL bShowSign,double numero);
		INT		dcDisp3D(HDC hdc,INT px,INT py,INT idxFont,CHAR *str);
		INT		dcIconeGray(HDC hdc,INT px,INT py,CHAR *nome,LONG lColor,INT iLumCorrection);

		DWORD	dcTextout(HDC hdc,INT x,INT y,EH_COLOR colText,EH_COLOR colBack,EH_FONT *psFont,INT iByteChar,void *pString,INT iLen,EN_DPL enAlign);
		BOOL	dcDrawText(	HDC hdc,
							RECT * psRec,
							EH_COLOR colText,
							EH_COLOR colBack,
							EH_FONT *psFont,
							INT iByteChar,
							void * pString,
							INT iLen,
							EN_DPL enHorzAlign,
							DWORD	dwExtra);

		//INT dcIcone3d(HDC hdc,INT px,INT py,LONG col,CHAR *nome);
//		INT dcIcone3d(HDC hdc,INT px,INT py,LONG col,CHAR *nome,INT *piWidth,INT *piHeight);

		void dcBoxp(HDC hDC,RECT *rcBox,DWORD dwColor);//INT x1,INT y1,INT x2,INT y2,LONG col);

		void dcLine(HDC hDC,INT x1,INT y1,INT x2,INT y2,LONG col);
		void dcBoxBrush(HDC hDC,RECT *psRect,INT iType,EH_COLOR cBrush,EH_COLOR cBackground); // 2008
		void dcRectRound(HDC	hDC,
						RECT *	psRect,
						EH_COLOR cBorderPen,
						EH_COLOR cBrushInside,
						INT	lRoundWidth,
						INT	lRoundHeight,
						INT	iPenSize);
		void dcRect(HDC hDC,
					RECT *psRect,
					EH_COLOR cBorderPen,
					EH_COLOR cBrushInside,
					INT iPenSize);

		void dcLineEx(HDC  hDC,INT x1,INT y1,INT x2,INT y2,LONG col,INT opera,INT fnPenStyle,INT nWidth);
		void dcBmpDisp(	HDC hdc,			// Strano .. ma nel 2010
						INT PosX,INT PosY,
						INT LxNew,INT LyNew,
						HBITMAP hBitmap,
						EH_COLOR colPen);
		void dcBmpDispMask(	HDC hdc,
							INT x,INT y,
							INT lxSource,INT lySource,
							INT lxDest,INT lyDest,
							HBITMAP BitMap,
							HBITMAP MaskBit);

		BOOL dcBmpDispAlpha(HDC hdc,
							INT x,INT y,
							INT lxSource,INT lySource,
							INT lxDest,INT lyDest,
							HBITMAP hBitMap,
							BYTE  bSourceConstantAlpha // 0xFF=Opaco, 0x00 invisibile
							);


		//INT dcIconeEx(HDC hdc,INT x1,INT y1,CHAR *nome,INT *piWidth,INT *piHeight);
		typedef enum {
			IEFX_GRAYSCALE=1,
			IEFX_ALPHA
		} EN_ICON_EFX;
		typedef struct {
			
			EN_ICON_EFX	enType;
			EH_COLOR	colColor;
			DOUBLE		dAlpha;
			INT			iLuminance;
			BOOL		bResize;
			SIZE		sizIcone;

		} S_ICONE_EFFECT;

		EH_ICON * dcIconeEx(HDC hdc,INT x1,INT y1,CHAR *pIconName,S_ICONE_EFFECT *psEffect); // new 2010
		EH_ICON * dcIcone(HDC hdc,INT x1,INT y1,CHAR *pIconName);
//		EH_ICON * dcIconeTrasp(HDC hdc,INT x1,INT y1,CHAR *pIconName,INT Perc,EH_COLOR cColorBack);
		EH_ICON * dcIconeAlpha(HDC hdc,INT x1,INT y1,CHAR *pIconName,double dPercAlpha,EH_COLOR cColorBack); // dcIconeAlpha
		EH_ICON * dcIcone3d(HDC hdc,INT px,INT py,EH_COLOR cColorBack,CHAR *pIconName);
		//#define dcIcone(a,b,c,d) dcIconeEx(a,b,c,d,NULL)
		//#define dcIconeTrasp(a,b,c,d,e,f) dcIconeTraspEx(a,b,c,d,e,f,NULL,NULL)
		void dcEllipse(	HDC hdc,
						RECTD * rcfBox,
						EH_ACOLOR acColorBrush,
						EH_ACOLOR acColorPen,
						float fPenSize);
		HBITMAP IconeToHB(CHAR *nome);

		HDC UsaDC(INT cmd,HDC LastDC);
		INT AlineEx(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera,INT fnPenStyle,INT nWidth);
		INT lineEx(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera,INT fnPenStyle,INT nWidth);

		HRGN ClipRgnMake(HDC hDC);
		HRGN dcClipRect(HDC hDC,INT iLeft,INT iTop,INT iRight,INT iBottom);
		void dcClipFree(HDC hDC,HRGN hrgn); // ClipRgnKill


	//
	// GDI++ (new 2007)
	//

		void GDIPlus(INT iCmd);
		void dcBoxGradient(	HDC hdc,
							RECT *rcBox,
							RECT *rcGradient,
							EH_ACOLOR cColorStart,
							EH_ACOLOR cColorEnd);
		void BoxGradient(RECT *rcBox,
						 RECT *rcGradient,
						 EH_ACOLOR cColorStart,
						 EH_ACOLOR cColorEnd);
		/*
		void dcBoxFill(	HDC hdc,
						RECT *rcBox,
						EH_ACOLOR cColorPen,
						double iPenSize,
						EH_ACOLOR cColorFill);
						*/
		void dcBoxFill(	HDC hdc,
						RECTD * prcBox,
						EH_ACOLOR cColorPen,
						double	  dPenSize,
						EH_ACOLOR cColorFill);

		void BoxFill(	RECT *rcBox,
						DWORD cColorPen,
						DWORD iPenSize,
						DWORD cColorFill);
		void dcBox(	HDC hdc,
					RECTD * rcfBox,
					EH_ACOLOR acColorPen,
					double fPenSize);

		void dcLinePlus(HDC hdc,
						double x,double y,double x2,double y2,
						EH_ACOLOR acColorPen,
						double iPenSize,
						INT dashStyle);

		void dcLineGradient(HDC hdc,
							RECT *rcLine,
							EH_ACOLOR cColorStart,
							EH_ACOLOR cColorEnd);

		void LineGradient(RECT *rcLine,
						 EH_ACOLOR cColorStart,
						 EH_ACOLOR cColorEnd);
		INT PlusPolygon(EH_ACOLOR cColorPen,
						 EH_ACOLOR cColorBrush,
						 INT iPoints,...);
		INT dcPolygon(	HDC	hDC,
						EH_ACOLOR cColorPen,
						double dSizePen,
						EH_ACOLOR cColorBrush,
						INT iPoints,
						POINTD * arPointd);
		INT dcPolygonf(HDC	hdc,
					   EH_ACOLOR cColorPen,
					   double dSizePen,
					   EH_ACOLOR cColorBrush,
					   INT iPoints,...);
		INT	 dcLines(HDC	hdc,
						EH_ACOLOR cColorPen,
						double dSizePen,
						INT iPoints,
						POINTD * arsPointd);

		void dcRectRoundEx(	HDC	hdc,
							RECTD *	prcBox,
							EH_ACOLOR acBorderPen,
							EH_ACOLOR acBrushInside,
							double	dRoundWidth,
							double	dRoundHeight,
							double	dPenSize);

		void dcPie( HDC	hdc,
					RECT *	prcBox,
					EH_COLOR cBorderPen,
					EH_COLOR cBrushInside,
					double dStartAngle,
					double dSweepAngle);

		void dcFontText(	HDC			hdc,
							WCHAR *		pwcText,	
							POINTD	*	pptPos,
							EH_ACOLOR	colFill,

							LOGFONTW *	psLogFontW,	
							WCHAR *		pwcFontFamily,
							double		dHeight,
							EH_TSTYLE	enStyles,
							EN_DPL		enDpl,	// Posizionamento stampa

							SIZED	*	psizFont,
							RECTD	*	precText); // 2014
		BOOL fontAddCollection(UTF8 * pszFileName);

//		void dcImageDraw(HDC hdc);

		// Icone
		#pragma pack (1) // pack(1) obbliga il compilatore ad allinerare al byte
		 struct LIC_HEAD
			 { CHAR id[10];
			   CHAR eof;
			   INT16 NumIco;
			   DWORD dati;
			 };

		typedef struct  {
			CHAR id[10]; // Indentificazione del file
			CHAR eof;
			CHAR bit; // Bit-Colore dell'icone
			INT16 dimx;// Dimensioni orizzontali
			INT16 dimy;// Dimensioni verticali
			INT16 byte;// Dimensioni in byte orizzontali
			LONG blk; // Dimensioni in byte dell'icone
			LONG ofs_mask; // Posizione della maskera
			LONG ofs_icone;  // Posizione dell'icone
			INT16 fpal;// Flag : ON cä la pallette OFF no
		 } EH_ICOHEAD; // Ex ICO_HEAD

		typedef struct  {
			CHAR	id[10]; // Indentificazione del file
			CHAR	eof;
			INT	iType;	// 0
			INT	iChannels;
			INT	iColorDeep; // Bit-Colore dell'icone
			BOOL	bAlpha;
			SIZE	sSize;
			DWORD	dwRowWidth;	// Dimensione orizzontale in byte della linea
			DWORD	dwSize;		// Dimensione totale del bitmap
			DWORD	dwOffsetBitmap; // Offset inizio bitmap
			DWORD	dwOffsetMask; // Offset inizio maskera (0 se assente)
		 } EH_ICOHEAD_2; // Versione 2

		struct LIBLIC
		{
			 CHAR icone[NOMEICONE_SIZE];
			 LONG size;
			 LONG offset;
		};
		#pragma pack ()


		struct VGAMEMO {
				INT memo_flag;
				INT x;
				INT y;
				HBITMAP bitmap; // Handle della memoria collegata
				};

	#endif

	INT	HwndToWin(HWND hWnd);
	INT	setpal(struct LKUP *info,LONG col_start,LONG col_num);
	CHAR	TC_decoder	(CHAR red,CHAR green,CHAR blue);

	//
	//		Servizio
	//
	void 	xy_rel			(INT x,INT y);
	void	dcProgress(	HDC hdc,
						RECT * psRect,
						DWORD dwProgress,
						DWORD dwMax,
						EH_COLOR colText,
						EH_COLOR colBack,
						CHAR * pszFontFace,
						INT nfi,
						EH_TSTYLE enStyle);
	void	Abar_perc(INT x,INT y,INT lx,LONG riemp,LONG max,CHAR *car,INT nfi);
	void 	bar_perc		(INT x,INT y,INT lx,LONG riemp,LONG max);
	void	bar_percf(INT x,INT y,INT lx,LONG riemp,LONG max,CHAR *car,INT nfi);
//	void	dcProgress(HDC hdc,RECT * pszRect,DWORD dwProgress,DWORD dwMax,CHAR * pszFontFace,INT nfi);
	void 	clip_set		(INT x,INT y,INT x2,INT y2,CHAR *user);
	void 	Aclip_set		(INT x,INT y,INT x2,INT y2,CHAR *user);
	void	clip_pop(void);

	struct WS_DISPEXT // Struttura di display extern
	{
		INT		px,py;
		INT		lx,ly;
		RECT	rClientArea;			// Rettangolo dell'area scrivibile senza barre di scorrimento
		RECTD	rdClientArea;			// Rettangolo dell'area scrivibile RECTD (new 2014)

		RECT	rWindowArea;			// Rettangolo dell'intera area window (inteso come suboggetto Child non come OBJ - Nota per SCR sono differenti, vedi titolo e contorni vari)
		INT		hdl,nfi;
		INT		idxFont;
		union {
			EH_COLOR col1;
			EH_COLOR colText;
		};
		
		union {
			EH_COLOR col2;
			EH_COLOR colBack;
		};
		INT		ncam;
		INT		bEnable;
		BOOL	bFocus;	 // ex .selected quello evidenziato usando lo scroll
		BOOL	bSelected; // bSelected (se la riga è selezionata) (può essere anche multiplo

		HDC		hdc;		
		EH_FONT * psFont;	
	 };
//#define EH_DISPEXT struct WS_DISPEXT
	typedef struct WS_DISPEXT EH_DISPEXT;


	// Debug functions
	void	dispx(CHAR *Mess,...);
	void	dispxEx(INT x,INT y,CHAR *Mess,...);
	void	bmpDispx(HBITMAP hBitmap);

	INT		disp(INT px,INT py,LONG colcar,LONG colbg,CHAR *str);
	INT  	dispf(INT px,INT py,LONG colcar,LONG colbg,INT iStyles,CHAR *font,INT nfi,CHAR *str);
	INT  	dispfm	(INT px,INT py,LONG colcar,LONG colbg,INT iStyles,CHAR *font,INT nfi,CHAR *str);
	INT		dispfmS	(INT x,INT y,INT lx,LONG col1,LONG col2,INT iStyles,CHAR *Font,INT nfi,CHAR *Desc);
	INT  	Adispfm	(INT px,INT py,LONG colcar,LONG colbg,INT iStyles,CHAR *font,INT nfi,CHAR *str);
	INT		dispf_h	(INT px,INT py,LONG colcar,LONG colbg,INT idxFont,CHAR *str);
	INT		dispfm_h(INT px,INT py,LONG colcar,LONG colbg,INT idxFont,CHAR *str);
	INT		dispExt(INT px,INT py,EH_DISPEXT * psExt,CHAR * pszStr);
	INT		Adispfm_h(INT px,INT py,LONG colcar,LONG colbg,INT idxFont,CHAR *str);
	INT  	Adisp_drv(INT x,INT y,LONG col1,LONG col2,INT lx,INT ly,INT ofx,INT ofy,INT idxFont,CHAR *str);
	INT		Adisp_drvf(INT x,INT y,LONG col1,LONG col2,INT lx,INT ly,INT ofx,INT ofy,INT iStyles,BYTE *pFontFace,INT iAlt,CHAR *pString);
	INT		dispfLim(INT x,INT y,LONG col1,LONG col2,INT lx,INT ly,INT iStyles,BYTE *pFontFace,INT iAlt,CHAR *pString);
	INT		disp3Dfm_h(INT px,INT py,INT idxFont,CHAR *str);
	INT		disp3Df(INT px,INT py,INT iStyle,CHAR *lpFont,INT nfi,CHAR *str);
	INT		dispnum_h(INT x,INT y,LONG col1,LONG col2,INT idxFont,INT cifre,INT dec,INT sep,double numero,BOOL fSign);
	INT		dispnum(INT x,INT y,LONG col1,LONG col2,INT iStyle,CHAR *Font,INT nfi,INT cifre,INT dec,INT sep,double numero);
	INT		dispnums(INT x,INT y,LONG col1,LONG col2,INT iStyle,CHAR *Font,INT nfi,INT cifre,INT dec,INT sep,double numero);
	INT  	dispfp	(INT px,INT py,EN_DPL enDpl,LONG colcar,LONG colbg,INT iStyles,CHAR *font,INT nfi,CHAR *str);

	// Grafica
	INT clip_disp (INT *ptx,INT *pty,
					INT *ptofx,INT *ptofy,
					INT *lx,INT *ly,
					INT idxFont,CHAR *str,INT flag);

	//			Gestione bitmap
	INT bmp_info(CHAR *lpFile,BITMAP *sBitMap);
	INT bmp_load(CHAR *lpFile,BITMAP_INFO *bmp,INT tipo);
	INT bmp_disp (CHAR *file,INT px,INT py,INT modo);
	INT clip_bmp		(INT *ptx,INT *pty,INT *ptofx,INT *ptofy,
									 INT *lx,INT *ly,
									 INT hdl,INT flag);

	//			Disegno
	INT pset	(INT x1,INT y1,LONG col,INT opera);
	INT Apset	(INT x1,INT y1,LONG col,INT opera);
	LONG pget(INT x1,INT y1);
	LONG Apget(INT x1,INT y1);

	INT line		(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);
	INT Aline	(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);

	INT box		(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);
	INT boxp		(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);
//	INT boxBrush	(INT x1,INT y1,INT x2,INT y2,HBRUSH hbr); // new 2008
	void boxBrush(INT x1,INT y1,INT x2,INT y2,INT iType,EH_COLOR cBrush,EH_COLOR cBackground); // 2008

	INT Abox		(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);
	INT Aboxp	(INT x1,INT y1,INT x2,INT y2,LONG col,INT opera);
	void box3d	(INT x1,INT y1,INT x2,INT y2,INT tipo);
	void dcBox3d(HDC hdc,INT x1,INT y1,INT x2,INT y2,INT tipo);

	INT clip_box		(INT *x1,INT *y1,INT *x2,INT *y2,INT flag);
	INT clip_boxp	(INT *x1,INT *y1,INT *x2,INT *y2,INT flag);
/*
	INT Avideo_bck (INT x1,INT y1,INT x2,INT y2,INT modo,struct VGAMEMO *vga);
	INT video_bck	(INT x1,INT y1,INT x2,INT y2,INT modo,struct VGAMEMO *vga);
	INT video_rst  (struct VGAMEMO *vga);
	INT video_vedi (struct VGAMEMO *vga);
	INT video_free(struct VGAMEMO *vga);
*/

	//			Gestione ICONE
	BOOL ico_cerca(DWORD *puIdx,CHAR *lpIcone);
	INT ico_info(INT *x1,INT *y1,CHAR *nome);
	void ico_sfondo(CHAR *icone);
	INT ico_load(CHAR *lpFile,INT grp,INT memo);
	INT ico_add(CHAR *pszIcoName,INT iGroup,BYTE *pIconFile); // new 2008

	// New 2008
	EH_ICON *ico_getinfo(CHAR *pIconName);
	void *ico_getptr(CHAR *pIconName,BOOL *pbLock,EH_ICON **psIconGet);
	HBITMAP ehIconToBitmap(	INT iMode,
							CHAR *pIconName,
							BOOL *pbAlpha,
							HBITMAP *phBitmapMask,
							VOID **ppvBits);
	BOOL ico_bitmapBuider(CHAR *pIconName);
	// --------

	INT lic_load(CHAR *lpFile,INT grp,INT memo);
	INT lic_import(BYTE *pNome,BYTE *pFile,INT iGroup,INT iMemoType);
	BOOL lic_fromResource(LPCSTR pName,LPCSTR pType,INT iGrp);
	BOOL LicLoad(CHAR *pszName,INT iGrp); // new 2008

	//INT Dico_disp(INT x1,INT y1,CHAR *nome,INT cmd);
	INT		ico_disp(INT x,INT y,CHAR *pIconName);
	INT		ico_dispAlpha(INT x1,INT y1,CHAR *nome,double dPercAlpha,LONG Color);
	INT		ico_dispGray(INT px,INT py,CHAR *nome,LONG lColor,INT iLumCorrection);
	SINT	icoShowEx(	CHAR * pszName,
						INT x,INT y,
						BOOL bResize,INT cx,INT cy,
						double dPercAlpha, EH_COLOR cColorBack);

	void	ico_grpdel(INT iGrp);
	INT		ico_disp3D(INT px,INT py,EH_COLOR col,CHAR *nome);

	INT Aico_disp(INT x1,INT y1,CHAR *pIconName);
	INT ico_del(CHAR *pIconName);

	INT dcScroll(HDC hdc,INT x1,INT y1,INT x2,INT y2,INT tipo,INT pix,COLORREF col);
	INT scroll(INT x1,INT y1,INT x2,INT y2,INT tipo,INT pix,COLORREF col);
	INT Ascroll(INT x1,INT y1,INT x2,INT y2,INT tipo,INT pix,COLORREF col);

	INT ehBarRangeAdjust(HWND hwnd,INT iWhere,INT iOffset,INT iMaxCam,INT iNumCam); // 2010
	INT ehScrollTranslate(HWND hWnd,INT iWhere,DWORD wParam,INT iOffset,INT iMaxArea,INT iViewArea,INT iStep,BOOL bNotScroll); // 2010/2013

	//
	//	Gestione Carret (ex cursore)
	//
//	void cursor_on	  (void);
//	void cursor_off   (void);
	void	txtCursor(BOOL bEnable);
	void	txtCursorAspect(INT dimx,INT dimy,LONG col,INT speed); // 	void cursor_graph(INT dimx,INT dimy,LONG col,INT speed);
	void	txtCursorPos(INT x,INT y); //void cursor_xy(INT px,INT py);
	void	txtCursorPosEx(HWND hwnd,INT x,INT y); //void cursor_xy(INT px,INT py);
	void	txtCursorDraw(BOOL bShow); // CursorDisplay(BOOL fShow); // new 2007 (uso interno)
	//void Acursor_xy(INT px,INT py);
//	void CursorDisplay(BOOL fShow); // new 2007
	void oscura(INT x1,INT y1,INT x2,INT y2,CHAR tipo);
	//
	// <== GDI SECTION (END) =======================================================
	//

	// =================================================================================
	//
	// ==> WIN & OBJ (Common structure)
	//
	// =================================================================================

	// Strutture comuni
	typedef struct {
		 CHAR *Name;
		 INT iType;
		 INT iPx,iPy;
		 INT iD1,iD2;
		 INT iD3,iD4;
		 CHAR *lpChar1;
		 INT iD5;
		 CHAR *lpChar2;
		 RECT Rect;
		} OBJS;
#define EH_OBJS OBJS;

	typedef struct {

		INT		Type;
		CHAR *	pszName;
		INT		xTLAction;
		CHAR *	lpxTLParent;
		INT		yTLAction;
		CHAR *	lpyTLParent;
		INT		xBRAction;
		CHAR *	lpxBRParent;
		INT		yBRAction;
		CHAR *	lpyBRParent;

		INT		xTLActionReal;
		INT		xTLValue;
		INT		yTLActionReal;
		INT		yTLValue;
		INT		xBRActionReal;
		INT		xBRValue;
		INT		yBRActionReal;
		INT		yBRValue;

		INT		iWin;
		//	CHAR *lpParent; // In relazione ad un oggetto parente
	} AUTOMOVE;
/*
	typedef struct {

		AUTOMOVE *	Am;
		INT			ObjPt;
		INT			IptPt;
//		OBJS *		arsObjs;

	} AUTOMOVEINFO;
*/


	// =================================================================================
	//
	// ==> WIN SECTION
	//
	// =================================================================================

	// Macro
	#define WS_EHMOVE     WS_CLIPCHILDREN|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_SIZEBOX
	#define WS_EHMOVEHIDE WS_CLIPCHILDREN|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_OVERLAPPEDWINDOW|WS_SIZEBOX
	#define WS_EHNOMOVE WS_POPUPWINDOW| WS_CAPTION| WS_VISIBLE
	#define AUTOMATIC -2
	#define EHWP_FULLSIZE	  -1
	#define EHWP_MOUSECENTER  -2
	#define EHWP_SCREENCENTER -3
	#define EHWP_FOCUSCENTER  -4
	#define EHWP_MENUCENTER  -5

	#define NOACTIVE 128
	#define NOWIN 0x1000


	//                                                                      Apre e chiude finestra
	INT    win_start       (void);
	void    win_end         (void);

	//
	//	Windows
	//
	#ifdef __windows__

		typedef struct {
			
			EN_MESSAGE		enMess;
			INT				iWin;
			void	*		psWin;	
			BOOL			bAfter;
			//PAINTSTRUCT *	ps;
			LONG			lParam;
			void	*		pVoid;

		} EH_SUBWIN_PARAMS;	// Sub Win Params

		
		struct WIN {

			CHAR  *	titolo;
			LONG    col1;    // Colore dello sfondo -1=Standard
			LONG    col2;    // DOS: Colore del titolo
			INT		x,y;     // Posizione della finestra
			INT		Lx,Ly;   // Dimensioni reali della finestra
			INT		CLx,CLy; // Dimensioni dell'area Client
			INT		zooming,efx;
			INT		zm_x1,zm_y1,zm_x2,zm_y2;
			INT		bck_relx,bck_rely;
			/*
			INT    txc_vedi;
			INT    txc_x,txc_y;
			INT    txc_dim_x,txc_dim_y;
			LONG    txc_color;
			INT    txc_speed;
			*/
			EH_TCURSOR	sTxtCursor;
			HWND    txc_hWnd;
//			INT		obj_num;
//			INT		ipt_num;
			INT		tipo;

			INT		PMhdl;
			INT		PMnfi;
			CHAR    PMaltO;
			CHAR    PMaltC;

			//CHAR    job[16];
			CHAR *	job;
			HWND	hWnd; // Handle della finestra

			HWND    wndTool;	// Tooltip Windows
			//INT		ObjPt;// Puntatore alla struttura degli oggetti in azione
			//INT		IptPt;// Puntatore alla struttura degli input in azione
			INT		OldInputFocus;
			INT		OldWriteFocus;

			//
			// Oggettistica associata alla finestra
			//
			EH_OBJINFO	*		psObj;		//EH_OBJ *			arsObj;
			EH_LST				lstObjPtr;  // Stack per cambio di struttura oggetti

			struct  S_IPTINFO *	psIpt;
			OBJS *				arsObjs;// Puntatore alla struttura degli Obj statici in azione
			AUTOMOVE *			arsAutoMove; // AMInfo

			// Limiti di dimensioni Minime e massime
			INT		xSizeMin;
			INT		xSizeMax;
			INT		ySizeMin;
			INT		ySizeMax;
			WORD    wTypeShow; // Mod. 2001: Tipo di visualizzazione (Es. SW_MAXIMIZE)
			BOOL    PhaseCritical;

			//
			// SaveBit Mode
			// Usata per compatibilit‡ con applicazioni generate senza windows o a finestra fissa
			//
//			BOOL	bSaveBitMode;	// T/F
//			HDC		hdcBitmap;		// hdcBitmap Clone
//			HBITMAP	hBitmap;
//			HBITMAP	hOldBitmap;
			EH_DG *		dgClone;	// Clone della finestra
			//	RECT	DCZone;
			BOOL		EhWinType;
			void    * (*funcWin)(EH_SUBWIN_PARAMS *); //  CHIAMATA ALLA ROUTINE
			BOOL    fBackGroundExtern; // FALSE/TRUE Gestione del Background della finestra esterno
									   // Serve si vuole gestire un'immagine di background in una finestra
			BOOL    ColorMode; // False/True
			INT		Relwx,Relwy;
		//	INT    clp_x1,clp_x2,clp_y1,clp_y2;
			INT		ClipNum;
			struct  CLIP Clip[MAXCLIP];
			};

		typedef struct WIN EH_WIN;


		WORD win_openEx(INT  x,INT y,  // Posizione a video (x=EHWP_macro)
						CHAR  *lpTitle,  // Titolo
						INT  ClientLx,INT ClientLy, // Dimensioni area Client Interessata
						LONG  col1,  // Colore finestra (Real Color) -1=Default
						DWORD dwParam, // 1 = Ci sar‡ un menu collegato (Usi Futuri)
						LONG  WinStyleEx,// Style Esteso
						LONG  WinType,   // Syle Normale
						BOOL  DosEmulation, // Emulazione Dos
						void * (*funcSubWin)(EH_SUBWIN_PARAMS *)); // Sotto procedura dedicata al Paint

		WORD win_open(INT x,INT y,
					  INT lx,INT ly,
					  LONG col1,LONG col2,
	 				  INT modo,CHAR *car);
#define winOpen(tit,x,y,style)   {win_openEx(EHWP_SCREENCENTER,0,tit,x,y,-1,0,0,style?style:WS_EHMOVEHIDE,FALSE,NULL); win_SizeLimits(AUTOMATIC,AUTOMATIC,-1,-1);}

		typedef struct {
			
			CHAR *	pszWho;
			INT		Relwx;
			INT		Relwy;
			INT		WinWrite;
			HDC		hDC;
			BOOL	DcActive;
			INT		IPT_ult;

		} S_WINSCENA;

		void winSetFocus(HWND hWnd); // 2013
		void WinDirectDC(HDC hdc,S_WINSCENA *Scena,CHAR * pszWho);
		void WinWriteSet(INT Win,S_WINSCENA *WScena);
		void WinWriteRestore(S_WINSCENA *WScena);
		#define MYSCRY 4
		#define MYSCRX 4

		void  win_SizeLimits(INT xMin,INT yMin,INT xMax,INT yMax);
		HWND  WindowNow(void);
		EH_WIN * winGetFocus(void);
		void  WinGetClip(struct CLIP *lpClip,INT win,BOOL Flag);

	#endif

	//
	//	Apple
	//
	#ifdef __APPLE__

	#endif

	//
	//	DOS
	//
	#ifdef __DOS__
			struct WIN {
				struct VGAMEMO info;
				CHAR   *titolo;
				LONG    col1,col2;
				INT    x,y,Lx,Ly;
				INT    zooming,efx;
				INT    zm_x1,zm_y1,zm_x2,zm_y2;
				INT    bck_relx,bck_rely;
				INT    txc_vedi;
				INT    txc_x,txc_y;
				INT    txc_dim_x,txc_dim_y;
				LONG    txc_color;
				INT    txc_speed;
				INT    obj_num;
				INT    ipt_num;
				INT    tipo;

				INT    PMhdl;
				INT    PMnfi;
				CHAR    PMaltO;
				CHAR    PMaltC;

				//CHAR    job[16];
				CHAR   *job;

				};

	#endif

	void win_openAdd(HWND hWnd); // new 2007 (x CE)

	INT win_close(void);

	void win_draw(INT num);
	void win_drawon(INT num);
	void win_drawoff(INT num);

	WORD win_errprot(CHAR info[]);
	INT win_mess(CHAR info[],INT tipo,INT time);
	void win_cart(INT cmd,void *ptr);



	//
	// <== WIN SECTION (END) =======================================================
	//


	// =================================================================================
	//
	// ==> OBJ SECTION
	//
	// =================================================================================


	// Macro per la gestione degli oggetti windows
	#define EH_COMBOBOX 1024



	//
	// Oggetti statici
	//
	#define OS_TEXT  1000
	#define OS_ICON  1001
	#define OS_BOX   1002
	#define OS_BOXP  1003
	#define OS_BOX3D 1004
	#define OS_TEXTB 1005 // Text Bold
	#define OS_TEXTE 1006 // Text Extended

	struct WINSCR {
		LONG    record;
		CHAR    *keypt;
		};


	//
	// AUTOMOVE Manager
	//
	#define AMT_STOP 0
	#define AMT_OBJ  1
	#define AMT_OBJS 2
	#define AMT_IPT  3

	#define AMP_AUTO    10
	#define AMP_FIX     11

	#define AMP_TOP     12 // Posizione stabilita
	#define AMP_BOTTOM  13
	#define AMP_LEFT    14
	#define AMP_RIGHT   15
	#define AMP_HCENT   16
	#define AMP_VCENT   17
	#define AMP_PX      18
	#define AMP_PY      19

	#define AMP_HP25    40
	#define AMP_HP33    41
	#define AMP_HP66    43
	#define AMP_HP75    44

	#define AMP_VP25    50
	#define AMP_VP33    51
	#define AMP_VP66    53
	#define AMP_VP75    54

	#define AMA_TOP     20 // Posizione autocalcolata
	#define AMA_BOTTOM  21
	#define AMA_LEFT    22
	#define AMA_RIGHT   23
	#define AMA_CENTER  24
	#define AMA_AUTO    25

	#define AMA_PERC25  30
	#define AMA_PERC33  31
	#define AMA_PERC66  33
	#define AMA_PERC75  34

	// News fine 2002/quasi 2003
	#define AMA_LINK    100 // Posizione autocalcolata in relazione ad un oggetto padre (sinitra o top)
	#define AMA_LINKB   101 // Posizione autocalcolata in relazione ad un oggetto padre (destra o bottom) new 2007
	#define ORP_TOP     110 // Auto Relation Position
	#define ORP_BOTTOM  111
	#define ORP_LEFT    112
	#define ORP_RIGHT   113

	void	obj_AutoMoveAssign(AUTOMOVE *am);
	void	objAutoMoveEngine(INT win,BOOL Show);
	void	obj_AutoMoveRefresh(AUTOMOVE *Am);// New2000
	void	obj_AutoMoveAssignSolo(INT iWin,AUTOMOVE * Am,INT iTipo,CHAR *lpNome); // New 2002
	INT		obj_GetPosition(CHAR *nome,INT *px,INT *py,INT *ph,INT *lx); // New2003

	//
	// WINDOWS version
	//
	#ifdef __windows__

		#define SCRDRVMAX 5
		void *ScrDrvTest(EH_OBJPARAMS);

	struct WS_INFO {
		LONG    sizecam; // Dimenzione orizzontale (obsoleto)
		LONG    numcam;  // Linee verticali visualizzate
		LONG    maxcam;  // Numero massimo di Item contenuti
		LONG    offset;  // Indice prima linea in alto
		LONG    selez;   // Indice della selezione
		LONG    koffset; // Interno
		LONG    kselez;  // Interno
		BOOL    refre; // Refresh
		BOOL    dispext; // Procedura per il display dei dati: FALSE=Interno; TRUE=Esterno
		BOOL    doDrag; //  Drag & Drop ? : FALSE=NO, TRUE=SI
		BOOL    doNoFollow;// ON/OFF Bloccaggio MouseFollow ??
		LONG    dbult;

		LONG    Enumcam;  // Numero delle righe "intere" visualizzate
		BOOL	fNoBlankLine; // T/F per non "pulire" la riga prima della richiesta di stampa
		BOOL    doDragExt; //  TRUE/FALSE : Drag & Drop Esterno  (CioË gestito da una terza parte e non da EH)

		BOOL	bExtSelection; // T/F Gestione esterna della selezione (nuovo metodo 2008)
		};

		//void  obj_refresh(INT win);
//		void  I_objRepaint(
//							CHAR *lpNome,		 // Una funzione derivata/semplificata di obj_AloRefreshSolo()
//							  BOOL fDelete,		 // Cancello la vecchia posizione
//							  BOOL fShow);		 // Mostro l'oggetto nella nuova posizione

		void	Wobj_refresh(INT win);
		void	Wobj_disp(struct OBJ *poj);
		void	Wpmenuriga(LPDRAWITEMSTRUCT DIs);
		BOOL	WPmenuTranslate(WPARAM wParam);
		DWORD	WPmenuKey(WPARAM wParam,LPARAM lParam);
		//void	drawComboRow(LPDRAWITEMSTRUCT DIs); // (Use Internal only)
		BOOL	WOListTranslate(WORD wNotify,LPARAM lParam);

		//void   obj_dataRefreshI(struct OBJ *poj,LONG R_Inizio,LONG R_Fine);
		void	obj_dataRefreshI(struct OBJ *poj,LONG R_Inizio,LONG R_Fine);
		void	obj_LineH(CHAR *nome,INT LineH,BOOL Show);
		void	obj_SetText(CHAR *Nome,CHAR *Text);
		INT		OBJ_RangeAdjust(struct OBJ *poj,INT Offset,INT MaxCam,INT NumCam);//,BOOL *Barra);
		
		BOOL	objMoveTo(CHAR * pszName,EN_POSREF enPos,INT vx,INT vy);
		//INT	obj_GetWidth(struct OBJ *poj);

		//
		//	Funzioni private uso interno
		//
		void	I_obj_ClientRectMake(EH_OBJ *poj);
		void	I_objChildClientRect(EH_OBJ *poj,RECT *psScrRect,SIZE *psScrSize);
		void	I_LightMouseOver(HDC hdc,RECT *prcArea,RECT *prcGradient); // Luce con il mouse sopra oggetto
		void	I_drawCombo(EH_OBJ *poj);
		void	I_drawComboRow(LPDRAWITEMSTRUCT DIs);
		void	I_drawButton(EH_OBJ *poj);
		void	I_drawIconeKey(EH_OBJ *poj);
		//void	I_objSetRect(EH_OBJ * psObj,RECT * prcArea,BOOL bShow,INT iWin,BOOL bAutomove);
		void	I_objSetRect(EH_OBJ * psObj,RECT * prcArea,BOOL bDraw,BOOL bWinUpdate);
		void	I_objRepaint(	EH_OBJ * poj,
								BOOL bRemoveOld,		 // Cancello la vecchia posizione
								BOOL bDraw,
								BOOL bWinUpdate);

		EH_OBJ *	OBJInput(void);
		INT		OBJFocus(void);

		//INT   EHGetLine(INT Leny,INT CharY,BOOL Real);
		INT		EHGetLine(struct OBJ *poj,BOOL Real);
	 	void	obj_RefreshSolo(CHAR *lpName,BOOL fDelete,BOOL fView);
		void	obj_Refresh(BOOL fDelete,BOOL fView);
		void	I_objRepaint(struct OBJ *pObj, // Posizione Alo = Cambio di posizione o grandezza
							  BOOL fDelete, // Cancello la vecchia posizione
							  BOOL fShow, // Mostro l'oggetto nella nuova posizione
							  INT win);  // Finestra di rimferimento

		void	obj_StackClear(void);
		BOOL	obj_StackControl(void); // New ultimi tristi giorni 2006 ...
		//void	obj_VStack(CHAR *Nome);
		//void	obj_PutStack(CHAR *Mess,...);
		
#define	obj_addevent(a) eventPush(EE_OBJ,0,NULL,a)
#define obj_putevent(msg,...) eventPush(EE_OBJ,0,NULL,msg,__VA_ARGS__)

		//void	obj_addevent(CHAR *lpNome);
		//void	obj_putevent(CHAR *Mess,...);

		BOOL	obj_StackFind(CHAR *lpNome);

		void	objs_open(OBJS *Objs);
		void	objs_show(void);
		void	objs_disp(OBJS *Objs);
		void	objs_GetRect(CHAR *Nome,RECT *Rect);
		BOOL	obj_GetRect(CHAR *lpNome,RECT *lpRect); // New 2005
		#define obj_tooltip(a,b) hmz_obj(a,b) // New 2006
		INT		obj_mgz(CHAR *pObjName,CHAR *pIcon,INT ofx,INT ofy); // new 2008

		struct	OBJ *WndToObj(HWND hWnd);
		void	WinFocus(INT win);
		BOOL	IsWinMaximize(void);
		INT	objs_dfind(OBJS *Objs,CHAR *Nome);
		INT	objs_find(CHAR *Nome);
		void	win_StatusUpdate(HWND win);
		void	winFullScreen(HWND hwnd,BOOL bFull); // new 2011

		// Assegnazione di nome alfanumero ad un oggetto del OS

		#define IDA_OFFSET 0x6000 // (?) Boh ???
		INT	obj_IDLock(CHAR *Name);
		BOOL	obj_IDUnlock(CHAR *Nome);
		CHAR	*obj_IDNameFind(INT Id);
		void	win_HideAll(void); // new 2006

	#endif

	//
	// DOS VERSION
	//
	#ifdef __DOS__

			// Questo ä il DOS
			typedef struct {
			 CHAR *Name; // Add 2000
			 INT iType;
			 INT iPx,iPy;
			 INT iD1,iD2;
			 INT iD3,iD4;
			 CHAR *lpChar1;
			 INT iD5;
			 CHAR *lpChar2;
			} OBJS;

			struct OBJ {
				CHAR tipo,nome[LUNNOMEOBJ+1]; // TIPO OGGETTO,NOME ASSEGNATO
				INT  status,lock;
				INT  px,py,col1,col2; //                        POSIZIONE E COLORI
				CHAR text[40];   //                     TESTO DA SCRIVERE SULLO SCHERMO
				CHAR grp[2];     //                     GRUPPO DI APPARTENENZA (x O_RADIO)
				void * (*sub)(INT cmd,LONG info,CHAR *str); //      CHIAMATA ALLA ROUTINE
				CHAR **ptr; // Puntatore ad una lista di parametri aggiuntivi
				INT fonthdl;
				INT nfi;
				};

			struct WS_INFO {
				LONG    sizecam;
				LONG    numcam;
				LONG    maxcam;
				LONG    offset;
				LONG    selez;
				LONG    koffset;
				LONG    kselez;
				WORD    refre:1;
				WORD    dispext:1; // OFF=Interno; ON=Esterno
				WORD    doDrag:1; // ON/OFF Gestionre Drag
				WORD    doNoFollow:1; // ON/OFF Bloccaggio MouseFollow
				LONG    dbult;
				};

			struct OBJ_INFO {
					INT numobj;  // Numero di oggetti contenuti
					INT alo_hdl; // Handle_memory della ALO Table
					struct OBJ_AMB *status; // Puntatore alla ALO
					struct OBJ_ALO *alo; // Puntatore alla ALO
					};

	#endif

	INT		obj_start(void);
	void	obj_end(void);

	void	obj_open(struct OBJ *mask);// reset e load oggetti
	void	obj_add(struct OBJ *mask);// aggiunge oggetti
	void	obj_close(void);// chiude gli oggetti
	void	obj_vedi(void); // mostra tutti gli oggetti
	INT		obj_vedisolo(CHAR *nome);// mostra un oggetto
	void	obj_disp(EH_OBJ * poj);

//	EH_OBJINFO *obj_info_get(void);

//	EH_OBJ *	I_objGetArray(INT iWin);	// Ritorna l'array degli oggetti in corso
	EH_OBJINFO *	objGetInfo(INT iWin);
	EH_OBJ *		objGetArray(INT iWin);
	BOOL			I_obj_EventProcessing(EH_OBJINFO * psObj);
	INT  obj_press(BYTE *pName); // Ritorna se ä stato cliccato l'oggetto <nome>
	INT  obj_Mpress(CHAR *status,CHAR *lista);
	INT  obj_mouse(BYTE *pName); // Ritorna il numero di oggetto sotto il mouse
	struct OBJ * obj_mouseover(BOOL bOnlyEnable);
	struct OBJ * obj_fromPoint(POINT *psPoint,INT *piNumber,BOOL bOnlyEnable);

//	INT		obj_findEx(INT iObjStruct,CHAR *lpNome);
	INT		obj_find(CHAR * pzName); 
	INT		obj_findEx(EH_OBJINFO * psObjInfo,CHAR * pszName);

//	struct OBJ *obj_lpFind(CHAR *lpNome);
	struct OBJ *obj_GetInfo(CHAR *lpNome); // era obj_lpFind

	// New 2007
	struct OBJ *	obj_ptrEx(EH_OBJINFO * psObj,BYTE *pName,BYTE *pWhoRequest);
	#define			obj_ptr(a,b) obj_ptrEx(NULL,a,b);
	void			obj_setsize(BYTE *pName,INT iWidth,INT iHeight,BOOL bShow);
	void			obj_setposition(BYTE *pName,INT ix,INT iy,BOOL bShow);
	struct OBJ *	ObjFindStruct(struct OBJ *poj,CHAR *Nome);
	EH_OBJ *		objSearch(INT iWin,CHAR * pszName);

	INT		obj_type(BYTE *pName); // Ritorna il tipo dell'oggetto nome
	INT		obj_sopra(BYTE *pName);// Ritorna se il mouse ä sull'oggetto <nome>
	INT		obj_lock(BYTE *pName);   // Chiude l'uso di un oggetto
	INT		obj_unlock(BYTE *pName); // Ripristina l'uso di un'oggetto
	INT		obj_lockgrp(CHAR grp[]);   // Chiude l'uso di un gruppo
	INT		obj_unlockgrp(CHAR grp[]); // Riapre l'uso di un gruppo
	INT		obj_status(BYTE *pName);
	void	obj_setstatus(CHAR *Nome,BOOL Status);
	BOOL	obj_freeze(CHAR *Nome,BOOL bFreeze); // new 2007

	void	obj_visible(BYTE *pName,BOOL fTrue);   // Setta il flag di visibilit‡ dell'oggetto New 2007

	BOOL	objVisible(CHAR *pszName,BOOL bVisible);
	HWND	objWindowParent(EH_OBJ * psObj);

	INT	obj_on(BYTE *pName);
	INT	obj_off(BYTE *pName);
	// INT  obj_prezoom(BYTE *pName);

	INT	obj_listset(BYTE *pName,INT num);
	INT	obj_listget(BYTE *pName);
	INT	obj_listcodset(BYTE *pName,void *ptr);
	void *	obj_listcodget(BYTE *pName);
	void	objSetArray(CHAR * pszNome,EH_AR arData);

	INT	obj_barinc(BYTE *pName,INT inc);
	INT	obj_barset(BYTE *pName,INT inc);
	INT	obj_barproset(BYTE *pName,LONG inc,LONG limite);
	INT	obj_barget(BYTE *pName);
	INT	obj_font(CHAR *nome,CHAR *font,INT nfi);
	INT	obj_grpfont(CHAR *grp,CHAR *font,INT nfi);
	INT	obj_typefont(INT tipo,CHAR *font,INT nfi);

	INT	obj_reset(CHAR *nome,INT flag);
	INT	obj_resetEx(INT iWin,CHAR *nome,INT flag); // new 2008

	BOOL	obj_setfocus(BYTE *pName);
	BOOL	I_obj_setfocus(EH_OBJ *poj);
	INT		obj_dataRefresh(CHAR *Nome,LONG R_Inizio,LONG R_Fine);

	void *	obj_message(CHAR *nome,INT cmd,LONG info,void *str);
	void *	obj_messageEx(INT iWin,CHAR *nome,INT cmd,LONG info,void *str); // new 2007
	void *	objSendMessage(EH_OBJ * pObj,EN_MESSAGE enMess,LONG lDato,void * pVoid); // new 2013 (ah,ah,ah!)
	void	obj_MouseGhost(CHAR *Nome,BOOL fStatus);

	void	objs_vedi(OBJS *objs);

	void	drawKey(struct OBJ *poj);
	void	drw_mk(struct OBJ *poj);
	void	drawCheckbox(struct OBJ *poj);
	void	drawRadio(struct OBJ *poj);
	void	drw_scrol(struct OBJ *poj,INT col);
	void	drw_list(struct OBJ *poj);
	void	drw_lstdat(INT Npy,struct OBJ *poj,INT offset);
	void	drw_bar(struct OBJ *poj);

	void	scr_draw(INT x1,INT y1,INT lxc,INT lyc,INT lock,INT flagfrc,INT fontalt,INT col);
	void	drawIconeKey(struct OBJ *poj);
	INT	sep_icone(CHAR *text,CHAR *iup,CHAR *idn);
	void	grp_ctrl(struct OBJ *poj); // Sistema appartenenza al gruppo
	INT	obj_grpoff(CHAR grp[]);
//	void  memo_vedi(INT x1,INT y1);
	INT	obj_listload(BYTE *pName);
	void	drw_pmenu(struct OBJ *poj);
	void	pmenu(struct OBJ *mask);
	INT	pmenu_font(CHAR *font,INT nfi);
	HWND	objWindow(CHAR *pszObjName); // new 2011
	void	objSetNotify(CHAR * pszObjName,INT iParam,LRESULT (*funcNotify)(EH_NOTIFYPARAMS)); // 2012


	typedef enum {
		EHM_STOP,
		EHM_ITEM,
		EHM_SEP,
		EHM_CHECK,
		EHM_UNCHECK,
		EHM_HIDE=10
	} EHN_MIT; // Menu Item Type)

	typedef struct {
		EHN_MIT		iType;
		CHAR *		pszText;
		BOOL		bEnable;
		CHAR *		pszCode;
		void *		pLink;
		INT		iMacroImage;
		UINT		uiEnum;
		HBITMAP		hBmp;
	} EH_MENUITEM;
	//typedef EH_MENUITEM * EH_MENU;

	// 2010
	typedef struct {
		_DMI			dmiMenu;
		EH_MENUITEM *	arsItem;
	} EH_MENU;

	EH_MENU *	ehMenuCreate(void);
	void *		ehMenuAdd(EH_MENU *,EHN_MIT iType,CHAR *lpItem,BOOL fEnable,CHAR *lpCode,void *Link,SINT iMacroImage,UINT uiEnum,HBITMAP hBmp);
	void		ehMenuDestroy(EH_MENU *psMenu);

	HMENU		ehMenuBuilder(EH_MENUITEM *psEmi,INT iValue);
	CHAR *		ehMenuGetCode(EH_MENUITEM *Ehm,WPARAM uiEnum);
	CHAR *		ehMenu(EH_MENUITEM *psEmi,CHAR *pszDefault,POINT * psPoint,HWND hWnd);
	EH_MENUITEM * ehMenuSearch(EH_MENUITEM *arsItem,CHAR * pszCode);
	void		ehMenuEnable(EH_MENUITEM *arMenu,CHAR * pszCode,BOOL bEnable);
	void		ehMenuSetText(EH_MENUITEM *arMenu,CHAR * pszCode,CHAR * pText);
	void		ehMenuSetType(EH_MENUITEM *arMenu,CHAR * pszCode,SINT iType);
	void		ehMenuSetLink(EH_MENUITEM *arMenu,CHAR * pszCode,void * pLink);
	void		ehMenuSetCode(EH_MENUITEM *arMenu,CHAR * pszCode,CHAR * pNewCode);

	//
	// <== OBJ SECTION (END) =======================================================
	//

	// =================================================================================
	//
	// ==> TEXT INPUT SECTION
	//
	// =================================================================================

	INT 	ipt_start(void);
	void 	ipt_end(void);


	#define NORM 0
	#define RIGA 1
	#define QUAD 2

	#define IPTS_BOLD 1
	#define IPTS_WRAP 2
	#define IPTS_VSCROLL 4

	#define IPTMODE_ALLEVENT      1 // Esce al verificarsi di ogni eventi
	#define IPTMODE_AUTOSELECT    2 // Autoseleziona il campo in ingresso (tipo Windows)

#if (defined(_ADB32)||defined(_ADB32P)) // In disuso
	struct ADB_HOOK {
			 CHAR pTableFieldName[30]; // Nome del campo NomeFld
			 CHAR pObjName[30];	// NomeObj
			 INT iInput;
			 };
#endif
	#define	TH_MASK 0x0fff
	#define TH_BIND 0x1000 // Data Binding (new 2008)
	#define TH_FUNC 0x2000 // Funzione esterna

	typedef struct {
		EH_DATATYPE iType;		// ALFA,NUME,DATA,BIND
		CHAR *	pTableFieldName; // Nome del campo
		CHAR *	pObjName; // Se bind indica dove verrà scritto il campo, se null chiama la funzione
		INT	iInput;
		void * (*funcExt)(EN_MESSAGE enMess,LONG iParam,void * pVoid);
	 } EH_TABLEHOOK;

	typedef struct {
		CHAR	*	pszFieldName;
		CHAR	*	pszFieldValue;
		EH_TABLEHOOK *	psTableHookField;
	} EH_TABLEHOOKNOTIFY;

	BYTE * hookGetText(EH_TABLEHOOK *arHook,CHAR *pszFieldName,CHAR *pszBuffer,SIZE_T sizBuffer); // 2010

	struct IPT {
		CHAR	avanti,indietro;
		INT 	tipo;
		INT		tipovis;
		INT		x,y;
		INT 	dimx;
		INT		len;
		LONG	col1;
		LONG	col2;
		INT		num1,num2;
		CHAR	lim1[15];
		CHAR	lim2[15];
		CHAR	*dati;
		INT		ofs_x;
		INT		ofs_y;
		INT		idxFont;
		INT		alt;

		INT		bFocus; // T/F se il campo è in focus (ex edit Di servizio per l'editor)
		INT		cx;
		INT		cy;
		INT		nrig; // Per le note
		INT		bEnable; // ON/OFF chiusura del campo

		BOOL    fEnable; // T/F Editing
		DWORD   dwStyle; // Style IPTS_BOLD|IPTS_WRAP
		HWND	hWnd; // Oggetto Window agganciato
		BOOL	bColorSet; // T/F se i colori sono stati cambiati

		RECT	recText;	// Dimensioni del testo soltanto
		RECT	recObj;		// Dimensioni dell'oggetto (con il 3d e/o altro)
	};

	typedef struct IPT EH_IPT;

	struct  S_IPTINFO {
		// struct 	IPT *	arsIpt;// Puntatore alle tabelle di configurazione input
		struct 	IPT * arsIpt;
		/*
	#ifdef __windows__
		INT	WinIptPt; // Backup puntatore agli input in apertura nuovi input (new 99)
	#endif
	*/
		INT  		numcampi; // Numero campi presenti
		INT  		campo;    // Numero campo corrente
		INT  		procampo; // Prossimo campo da "inputare"
		CHAR *		ptbuf;   // Puntatore al buffer dei campi
		INT 		objpass_hdl; // ptoggetti che otrepassano i controlli di campo
		CHAR 		*objpass; // ptoggetti che otrepassano i controlli di campo
		INT			window;   // Window che contiene l'input
		void *		Hook;// Vecchio sistema
		INT			Modo; // 0 = Non esce dall'ipt_ask() se non con i tasti standard
									// Nuovo sistema
//		INT  		buf_hdl;  // Handle del buffer creato
									// 1 = Esce ad ogni pressione di tasto
	};
	typedef struct S_IPTINFO EH_IPTINFO;

	CHAR	*mask(CHAR *num,INT numcif,INT dec,INT sep,INT segno);
	INT 	ipt_font(CHAR *font,INT nfi);
	INT		ipt_fontnum(INT numcampo,CHAR *font,INT nfi);

	void 	ipt_open(struct IPT *mask);
	EH_IPT *		iptGetArray(INT iWin);
	EH_IPTINFO *	iptGetInfo(INT iWin);


#if (defined(_ADB32)||defined(_ADB32P))
	void 	ipt_Hopen(struct IPT *mask,struct ADB_HOOK *Hook);
#else
	void 	ipt_Hopen(struct IPT *mask,EH_TABLEHOOK *arHook);
#endif
	void	ipt_close(void);

	INT		IptEventGet(EH_EVENT *psEvent);
#define		ipt_ask() IptEventGet(NULL) // Per compatibilità

	void 	ipt_reset(void);
	void 	ipt_rescampo(INT pt);
	void	ipt_Hrescampo(CHAR *Campo);
	INT		ipt_noteriga(CHAR *dest,CHAR *sorg,INT width,INT riga);

	void 	ipt_vedi(void);
	void 	ipt_vedisolo(INT pt);
	void	ipt_Hvedisolo(CHAR *Campo);

	INT  	ipt_ctrlcampo(INT pt);

	void 	ipt_write(INT pt,CHAR *buf,double num);
	void 	ipt_Hwrite(CHAR *nome,CHAR *buf,double num);

	void 	ipt_writevedi(INT pt,CHAR buf[],double num);
	void 	ipt_Hwritevedi(CHAR *nome,CHAR buf[],double num);

	CHAR *	ipt_read(INT pt);
	CHAR *	ipt_Hread(CHAR *nome);

	void 	ipt_err(INT a,INT b);
	void	ipt_objpass(CHAR *pt);
	INT 	ipt_objctrl(CHAR *pt);
	void	ipt_noedit(void);
	INT 	ipt_sel(void);
	INT 	ipt_next(void);

	INT 	ipt_setnum(INT pro);
	INT		ipt_Hsetnum(CHAR *Campo);

	INT		ipt_prezoom(INT pt);

	void	ipt_lock(INT pt);
	void	ipt_unlock(INT pt);
	void	ipt_Hlock(CHAR *Campo);
	void	ipt_Hunlock(CHAR *Campo);

	void	ipt_mode(INT modo);
	INT	ipt_mouseover(void);

	//
	//	Windows
	//
	#ifdef __windows__
		// void    ipt_resize(struct IPT *pin,INT px1,INT py1,INT px2,INT py2);
		
		void	ipt_hide(INT pt);
		void	ipt_disp(struct IPT *pin);
		void	ipt_ObjectShow(EH_IPT * psIpt);
		DWORD   ipt_GetStyle(INT pt);
		void	ipt_SetStyle(INT pt,DWORD dwNewStyle,BOOL fView);
		void    ipt_SetStatus(INT,BOOL);
		void	ipt_SetColor(INT pt,INT cText,INT cBackground); // new 2007
		void	iptSetRect(struct IPT * pin,RECT * precArea);
		RECT *  iptGetRect(struct IPT * pin,RECT * prec);
		struct IPT *ipt_GetInfo(INT pt);

	#endif


	//
	// <== TEXT INPUT SECTION (END) =======================================================
	//
#endif


	#define LPT_PORTRAIT  0
	#define LPT_LANDSCAPE 1

#if defined(EH_PRINT) || defined(EH_COMPLETED)

	// =================================================================================
	//
	// ==> PRINTER SECTION
	//
	// =================================================================================
#define LPT_MAX 3
#define LPTD -20
#define EH_PRINT_API 1

	#ifdef __windows__
		INT			lpt_ask(INT Mode);
		BOOL		ehPrinterChoose(CHAR **ppszPrinterDefine,BOOL bUseDefaultSetting);
		BOOL		ehPrinterPage(CHAR **ppszPrinterDefine);
		BOOL		ehPrinterGetDefault(CHAR **ppszPrinterDefine);
		BOOL		ehPrinterExist(CHAR **ppszPrinterDefine,BOOL bAddInfo);
		BYTE *		ehPrinterDeviceDefine(CHAR * pszPrinterName,DEVMODE *psDevMode);
		HDC			ehPrinterCreateDC(CHAR *pszDeviceDefine,DEVMODE **ppDev);
		DEVMODE *	ehPrinterGetDev(CHAR *pszDeviceDefine);
		BOOL		enPrinterDoc(CHAR ** ppzPrinterDefine); // 2014


	#else
		#define LPT1 0
		#define LPT2 1
		#define LPT3 2
		#define EPSON1 0
		#define EPSON2 1
		#define EPSON3 2
		#define EPSON4 3

		#define EPSON24_1 10
		#define EPSON24_2 11
		#define EPSON24_3 12
		#define EPSON24_4 13

		#define LPT_EPSON   0
		#define LPT_EPSON24 1
		#define LPT_HP      2

		struct LPT_STS {
			WORD timeout:1;
			WORD no1:1;
			WORD no2:1;
			WORD errio:1;
			WORD selez:1;
			WORD paper:1;
			WORD ack:1;
			WORD busy:1;
		};

		struct LPT_STS lpt_init(INT port);
		struct LPT_STS lpt_status(INT port);
		INT  lpt_bmp(CHAR file[],CHAR send[],INT mode,INT px,INT py,INT scl);
		INT  lpt_ask(void);
	#endif

	INT  lpt_send(CHAR buf[],INT len);
	INT  lprint(CHAR str[]);
	INT  lpt_file(CHAR file[]);
	void  lpt_pag(void);
	INT  lprintn(CHAR *car);
	INT  lptspace(INT num);
	INT  lptchar(INT num,CHAR *ch);
	INT  lprintCRtype(INT num,CHAR *buf);
	INT  lprintCRsel(INT linea,CHAR *buf);

	INT  lpt_pagcont(CHAR *Titolo,CHAR *Campi);
	void  lpt_start(void);
	void  lpt_end(void);
	CHAR *lpt_capture(INT lpt,CHAR *file);
	INT  lpt_endcapture(INT lpts,INT lptd);
	void  lpt_command(INT lpt,CHAR *comando);
	void  lpt_orient(INT lpt,INT flag);

	void lpt_funON(void * (*LptFun)(INT,LONG,CHAR *));
	void lpt_funOFF(void);
    void lpt_Margin(INT left,INT top,INT right,INT bottom);


#ifdef __windows__
	typedef struct {
	  BOOL Bold;
	  BOOL Italic;
	  BOOL UnderLine;
	  COLORREF ColorChar;
	} PDESTYLE;

	#define EHP_DEFAULT       0
	#define EHPM_EMULATION    1
	#define EHPM_DIRECT       2
	#define EHP_ASKPAGE       3

	struct LPT_INFO
	{
		INT iMode;
		CHAR file[MAXPATH];
		FILE *ch;
		LONG len;
		//INT  type;// LPT_EPSON,LPT_ESPON24,LPT_HP

		// Per Printer Dos Emulation
		INT capture;
		CHAR orientamento; // 0= Verticale 1 = orizzontale
		UINT yChar;
		UINT xChar;
        UINT iCharsPerLine;
        UINT iLinesPerPage;

        UINT Page;
		UINT VCx; // Virtual Cursor
		UINT VCy;
		PDESTYLE PStyle;

		// Direct Information (Dati "diretti" alla stampante)
		INT iDirectActive;
		INT iDirectType;
		CHAR szDirectName[100];
		CHAR szDirectDesc[60];
		INT fDirectRemote;  // ON/OFF stampante remota
		INT fDirectClose;  // ON/OFF chiusura porta a fine stampa
		INT iDirectOrient; // 0= Verticale 1 = orizzontale
		INT iDirectMaxline_V; // Massimo numero di linee Verticali
		INT iDirectMaxline_O; // Massimo numero di linee orizzontali

	};
	void lpt_update(void);
	void lpt_xChar(INT num);
	INT lpt_SetMode(INT Mode);
	BOOL InizializePrinter(void);
	BOOL lpt_GetDefault(void);

	// new 2006
	BOOL FGetPrinterDefault(CHAR *pPrinterName,INT iSize);
	BOOL FSetPrinterDefault(CHAR *pPrinterName);

	#endif

	#ifdef __DOS__
		struct LPT_INFO
			{
				INT attiva;
				INT capture;
				CHAR file[120];  // c'era max path
				FILE *ch;
				LONG len;
				CHAR nome[30];  // c'era max path
				CHAR descrizione[60];  // c'era max path
				INT type;// LPT_EPSON,LPT_ESPON24,LPT_HP
				INT remota;  // ON/OFF stampante remota
				INT close;  // ON/OFF chiusura porta a fine stampa

				CHAR orientamento; // 0= Verticale 1 = orizzontale
				INT maxline_V; // Massimo numero di linee Verticali
				INT maxline_O; // Massimo numero di linee orizzontali
			};
	#endif

	//
	// <== PRINTER SECTION (END) =======================================================
	//

#endif

typedef enum {
	
	UAT_BUTTON		=0x0001, // Bottoni, Radio,Mark
	UAT_WIN_TITLE	=0x0002, // Titolo Finestra
	UAT_MESSAGE		=0x0004, // MessageBox
	UAT_OBJS_TEXT	=0x0010, // Testo negli objs
	UAT_SELECT		=0x0020, // Contenuto nelle combolist
	UAT_MENU		=0x0040, // Menu
	UAT_TOOLTIP		=0x0100	 // Tooltip

} ULT_AT;


//
//	EH_SYSTEM  - Easyhand system main structure
//

typedef struct {

		INT		iSystemStatus;	// 0=Non operativo, 1=Operativo, 2=In chiusura
		BOOL	bSocketReady;	// Se le librerie di socket sono pronte
		CHAR	szAppNameFull[512]; // Nome del programma in esecuzione (compreso di persorso)
		CHAR	szApplication[512]; // Nome del programma in esecuzione (compreso di persorso)
		CHAR	szAppLanguage[120];	// Lingua attualmente in uso Es. su Windows "Italian"
		CHAR	szDevice[128];	
		INT		arDayPerMonth[12]; // 2011
		EH_AR	arParams;
		INT		iClockStart;
		DWORD	dwChronoStart;	// Usato chrono<function>

		BOOL	bOnExitReport; // T/F se voglio il rapporto in uscita
		BOOL	bCgiAmbient;	// 2012 - T/F sono in un ambiente CGI (Es. non posso premere un tasto)

		// Transaltor subSystem
		EH_ULT	ultApp;
		//INT		bAutoTranslate;     // Esegui traduzione automatica delle stringhe negli oggetti (TRUE/FALSE)
		ULT_AT	dwAutoTrans;
		BOOL	bUtf8;				// T/F se le stringhe (GDI) sono in utf8
		INT		iLanguage;			// Lingua in uso
		INT		iSourceLang;		// Lingua in cui Ë scritto il sorgente
//		INT		fTranslateDisp; // Esegui la traduzione dei display (TRUE/FALSE)


		//                                                      Gestione memoria
		EH_MEMO_ELEMENT * arMemoElement;
		INT		iMemoCnt;
		INT		iMemoMax;

		//                                                      Sezione video
		INT		videomodo;	// Tipo di risoluzione
		INT		video_x;    // Dimensioni orizzontali massime monitor
		INT		video_y;    // Dimensioni verticali massime monitor
		WORD	colori;     // Numero massimo colori disponibili

		INT     iWinKeySpecial; // Situazione shift e control
		INT		bIniShared; //T/F se l'apertura dei file ini è condivisa tra processi

#ifdef __windows__
		HICON   LocalIcone;
		HINSTANCE EhWinInstance;
		BOOL    EhPaint;
		CHAR	tzWinClassBase[80];
		BOOL    fTitleLock;// Blocca il cambio di focus sulla title bar

		INT		WinWriteFocus;        // Finestra correntemente in usi per le istruzioni di stampa (WinScena)
		INT		WinInputFocus;        // Finestra correntemente in uso per le input da utente
		HWND	hWndInput;		 // Handle Win corrente che pu‡ ricevere gli input
														 // (mi server per far prima ad intercettare i messaggi)
		HDC		DirectDC;
		BOOL	DirectDCActive;
		WORD    wMenuHeightMin; // Altezza minima dell'item del menu in apertura (0=Default)
		WORD    wMenuLeftMin;   // Spazio a sinistra delle voci nell'item (0=Default)

		// Colori di base per il disegno generale
		EH_COLOR	ColorBackGround;
		EH_COLOR	ColorButtonText;
		EH_COLOR	Color3DHighLight; // La luce chiara del 3D (def.Bianco)
		EH_COLOR	Color3DLight;     // Il colore dell'oggetto (def.Grigio chiaro)
		EH_COLOR	Color3DShadow;    // L'ombra media (def.grigio scuro)
		EH_COLOR	ColorWindowText;  // Colore delle testo nelle finestre (def. Nero)
		EH_COLOR	ColorWindowBack;  // Colore dello sfondo delle finestre (def. bianco)

		EH_COLOR	ColorSelectText;  // Colore delle testo selezionato (def. Bianco)
		EH_COLOR	ColorSelectBack;  // Colore dello sfondo selezionato (def. Blu)
		EH_COLOR	ColorButtonGradientTop;
		EH_COLOR	ColorButtonGradientBottom;
		EH_COLOR	colEhWinBackground; // new 2010
		EH_COLOR	colScrollDiv;	// new 2011 (Color della linea di divisione negli scroll)
		DWORD		arColorBGR[16];		// Pallet Easyhand in BGR (per icone)

		DWORD	xWidthScroll;	  // Dimensioni della barra di scroll orizzontale

		BOOL    fSoundEfx;        // Attivazione Suoni ad effetto su oggetti

		//INT    DeskTop_x;
		//INT    DeskTop_y;
		RECT	rcDesktop;	// 2010
		SIZE	sizeDesktop; //2010

		BOOL    WDblClick;      // (Reserved)
		BOOL    fMouseCapture;  // TRUE/FALSE (Riservato)
// 		BOOL	OemTranslate;   // TRUE/FALSE se effettuare la traduzione dei caratteri speciali
 		BOOL	bOemString;   // TRUE/FALSE se effettuare la traduzione dei caratteri speciali
		BOOL    EhWinType;      // TRUE Finestre tipo EasyHand    FALSE Finestre tipo Windows
		CHAR *  pszCommandLine;
        BOOL    fLightRight;	// TRUE/FALSE se la luce viene da destra

        BOOL	iColorMode;   // 0=Easyhand 16 1=TRUE Color

		POINT   WinBorder;     // Bordo della Finestra (x/y)
		INT		yWinTitle;     // Altezza del titolo
		INT		yWinTop;       // Altezza Titolo + Bordo
		INT		yWinMenu;      // Altezza Menu
		BOOL	bLicSystemError;	// T/F se non sono state caricate le librerie standard di easyhand
#endif

		BOOL	bErrorContentType;	// T/F se voglio l'intestazione Content in caso di errore
		DWORD   dwIptDefaultStyle; // Default style degli input

#ifndef _WIN32_WCE
		_DMI DmiIDArray;
		IDARRAY   *IDArray;
		LONG g_lCPUExtensionsAvailable; // Status CPU
#endif

		BOOL	bGetKeyInEnd; // T/F se devo chiedere un tasto nel ehExit
		BOOL	bGetKeyInEndError; // T/F se devo chiedere un tasto in caso di errore
		BOOL	(*funcExitAlternative)(CHAR  * pszMessage); // 9/2012 - Funzione da usare in alternativa a Exit() in ehExit() alla fine exSubExit
		void	(*postExitAlternative)(INT iError); // 2/2008 - Funzione da usare in alternativa a Exit() in ehExit() alla fine exSubExit
		INT		(*extVfprintf)(const char * _Format, va_list _ArgList);
		BOOL	bNotUseExit;

#ifdef __windows__
		BOOL	(*funcPreTranslate)(MSG *pMsg); // 1/2009 controllo pretranslate della coda dei messaggi
#endif

		BOOL	fInternalError; // TRUE/FALSE se si Ë in internalError
		FILE	*chLog; // New fine 2004
		BOOL	bLogShow; // T/F se inviare del log anche al stderr
		BOOL	bConReady; // T/F se la console degli errori è attiva
		BOOL	fOleReady; // T/F se la Tecnologia OLE Ë stata inizializzata
		INT	uACP;

#ifndef _WIN32_WCE

		// Font
//		INT			iFonts;					// Font caricati in memoria
//		EH_FONT *	arsFonts;				// array dei font gestiti
		INT			iFontEnum;				// Enumeratore degli indici del font
		EH_LST		lstFontCache;			// Contenitore del font

		CHAR *	pFontStandardName;		// Nome del font standard
		INT		iFontStandardIndex;	// Handle del font standard
		INT		iFontStandardHeight;	// Altezza del font standard

		INT		iFontInputDefault;
		INT		iFontInputHeight;

#endif

		//
		// Modicalità grafica ----------------------
		//
#ifndef EH_CONSOLE

		_DMI dmiTimer;	// 2010
		EH_TCURSOR	sTxtCursor;

// Mouse
		CHAR *	pMouseCursorBase; // Nome Icone del cursore Base
		BOOL	bMouseCursorEnable; // T/F ON SE C'E' OFF = NO
		CHAR	szMouseCursorName[80]; // Icone corrente del mouse
		POINT	sMouseCursorHotPoint;
		INT		mouse_type; // Se cä >0 cä numero dei tasti
		INT		ms_x,ms_y,ms_b;
		INT		ms_miny,ms_maxy;
		INT		ms_minx,ms_maxx;

//		INT    hdl_icone;     // Handle delle icone
//		INT    ICO_num; // Numero di ICONE
//		INT    ICO_max; // Numero massimo di Icone
		INT		iIconMaxStart;	// Spazio per icone in avvio
		_DMI	dmiIcon;
		EH_ICON	*arIcon; // Puntatore all'array delle icone

//
//		Gestore eventi
//
		BOOL	bEventTrace;	// T/F se si vuole il trace in console, degli eventi
//		INT		iEventStart;	// Mouse Cue Event Start
//		INT		iEventStop;	// Mouse Cue Event Stop
//		INT		iEventMax;	// Numero massimo di eventi in coda
//		INT		iEventLength;	// Mouse Cue Event Number
		//EH_EVENT *arEvent;

		EH_LST		lstEvent;
		EH_EVENT	sLastEvent; // Per compatibilità
		EH_DRAGDROP sDragDrop;


//
//		Gestore oggetti
//
		BOOL	bObjectAniMouse;	// T/F se si vogliono durante la gestione degli oggetti le animazioni con la manina
		EH_OBJ * pObjFocus; //	Puntatore all'oggetto in focus

//		_DMI	dmiObject;
//		EH_OBJINFO *arObject;
		INT	iObjFocus2;//=-1;
		//INT  OBJ_sel;//=-1; // Numero oggetto selezionato
		INT	OBJ_time;//=-1; // Livello di passaggio per oggetto selezionato

		INT	OBJ_rep1;//=300; // In futuro parametrizzabile da file
		INT	OBJ_rep2;//=20;

	// Font per il men—
		INT	pmenu_hdl,pmenu_nfi;
		CHAR	pmenu_altC; // Altezza chiusura
		CHAR	pmenu_altO; // Altezza apertura
//#endif

		UINT_PTR uipTimer;

 //		Funzioni window GDI
		struct FTIME arsFtime[FTIME_MAX];
		INT iFtime; // Nessuna funzione temporale installata
		//COLORREF ColorPal[16];
		EH_COLOR	arsColor[16];

		INT CUR_Width,CUR_Height,CUR_Size;

		INT CLO_flag;
		INT CLO_bck;
		RECT CLO_space;
		struct VGAMEMO CLO_memo;


		// Gestione Mouse Graphic Zone (settaggio automatico aspetto mouse)
		INT		iMgzCounter;
		S_MGZ *	arsMgz;//=NULL;
		INT		hdlMgz;//=-1;
		INT		iMgzNum;//MGZ_ult;//=-1; // Numero di MGZ
		INT		iMgzMax;
		INT		MGZ_zone;//=-1;

		CHAR	MS_icob[NOMEICONE_SIZE+1];//="";
		INT		MS_axb,MS_ayb;

		INT		iHmzMax;//=10;
		INT		iHmzNum;//HMZ_ult;//=-1;
		INT		hdlHmz;//HMZ_hdl;//=-1;
		struct HMZ *arsHmz;//=NULL;
//		INT		mouse_disp;//=OFF;
/*
		LONG		dbclkvel=6;  //	Velocita DoubleClick
		INT		dbclktm=0;   //	(riservata) Variabile di comparazione
		INT		dbclick=0;   // Variabile per verifica evento
		WORD crdim_x=0,crdim_y=0; //	Dimenzioni MSC ;servono a mousece

*/


// ----------------------------------------------------
//	     VARIABILI PER LA GESTIONE DELE PORTE SERIALI !
// ----------------------------------------------------
#if defined(EH_COM) || defined(EH_COMPLETED)

		_DMI			dmiComm;
		S_COM_PORT *	arsComm;
		//INT				iComMax; // Numero di COM gestite
		INT				iComOpen; // Numero di COM aperte
		//INT				hdlCom; // Handle della gestione seriale

		CHAR		COM_port8259; // Backup 8259
#endif

#endif

 #if (!defined(__cplusplus)&&defined(__windows__))

	#if (defined(EH_PRINT)||defined(EH_COMPLETED))
		struct LPT_INFO arsPrinter[LPT_MAX];
	#endif
		INT				lpt_cor; // Stampante di default
		PRINTDLG 		sPrintDlg;// pd;		// Struttura per la finestra di dialogo della stampante
		PAGESETUPDLG 	sPageSetupDlg;		// Struttura per la finestra di dialogo della scelta del tipo di pagina

	//#endif
 #endif

 } EH_SYSTEM;


#ifdef __cplusplus
}
#endif
