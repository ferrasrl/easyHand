//   +-------------------------------------------+
//    ehTranslate.h
//								by Ferrà srl 2011
//   +-------------------------------------------+

#ifndef EH_TRANSLATE

	#define EH_TRANSLATE 1

	// Enumerazione tecnologia usata
	typedef enum {

		ET_LOCAL=1, 
		ET_GOOGLE=2,
		ET_BING=3

	} ET_TECH;

	typedef enum {

		GE_NONE,
		GE_LANG_DIFFERENT,	// La lingua indicata come sorgente non è esatta
		GE_STRING_SUSPECT,	// La stringa da analizzare è da controllare
		GE_SERVER_ERROR,	// Dopo 5 tentativi non ho la traduzione
		GE_STRING_TOO_LONG,	// Stringa troppo lunga e non divisibile
		GE_ABUSE,			// Dopo 5 tentativi non ho la traduzione
		GE_UNAVAILABLE,		// Traduzione sconosciuta
		GE_TOO_PHRASES		// Troppe frasi

	} ET_ERROR;

	typedef enum {
		ET_TRY_LOCAL	=0x00000001,		// Uso assistente locale
		ET_TRY_GOOGLE	=0x00000002,		// Chiedo a Google
		ET_TRY_BING		=0x00000004,		// Chiedo a Bing
		ET_TRANS		=0x00010000,		// Richiedo la traduzione
		ET_LANGDETECT	=0x00020000,		// Determina la lingua sorgente
		ET_STOPERROR	=0x00040000,		// Fermati in caso di errore
		ET_OUTPUTDETT	=0x00080000,		// Richiedo output dei dettagli nella traduzione
		ET_STAT			=0x00100000,		// Conta caratteri e parole richieste in traduzione
	/*
		ET_LOCAL		=0x00010000,		// Uso assistente locale
		ET_GOOGLE		=0x00020000,		// Chiedo a Google
		ET_BING			=0x00040000,		// Chiedo a Bing
	*/	
	} ET_PARAM;
	#define ET_MASK 0xFF
	#define ET_TRY_ALL ET_TRY_LOCAL|ET_TRY_GOOGLE|ET_TRY_BING

	typedef struct {

		ET_TECH enTech;
		CHAR *	pszLangSource;
		CHAR *	pszLangDest;
		UTF8 *  utfSource;
		UTF8 *  utfDest;

	} S_TRANSLATION;


	typedef struct {

		ET_ERROR	etError;	
		ET_TECH		enTech;			// Tecnologia usata per la traduzione

		CHAR	szLangSource[20];
		CHAR	szLangDest[20];
		DWORD	dwChars;		// Caratteri richiesti
		DWORD	dwWords;		// Parole richieste

		INT		iLocalResponse;	// Traduzioni prelevate fornite dall'assistente
		DWORD	dwLocalChars;		// Caratteri richiesti
		DWORD	dwLocalWords;		// Parole richieste

		// Google
		INT		iGoogleRequest;
		INT		iGoogleResponse;
		INT		iGoogleErrors;
		INT		iGoogleLastError;
		DWORD	dwGoogleChars;		// Caratteri richiesti
		DWORD	dwGoogleWords;		// Parole richieste

		// Google
		INT		iBingRequest;
		INT		iBingResponse;
		INT		iBingErrors;
		INT		iBingLastError;
		DWORD	dwBingChars;		// Caratteri richiesti
		DWORD	dwBingWords;		// Parole richieste

	} ET_REPORT;


	UTF8 * ehTranslate(	ET_PARAM	enParam,
						UTF8 *		pszSource,
						CHAR *		pszLangSource,
						CHAR *		pszLangDest,
						CHAR *		pszJsonParams,	// {googlekey:"asda", bingkey:"asd"}
						ET_REPORT * psReport,
						void * (* extAssist)(EN_MESSAGE enMess,LONG lParam,void *pVoid)
						);

	//
	// Translate Tag Encode 
	//

	typedef enum {
		TTE_DEFAULT,
		TTE_GOOGLE,
		TTE_BING
	} EN_TTE;

	typedef struct {
		EH_AR	arTag;
		CHAR *	pszSource;
		CHAR *	pszSourceTagged;
		DWORD	dwSizeTag;
		BOOL	bHtml;
	} S_TTE;

	S_TTE * tagEncode(CHAR *pSource);//,ET_PARAM iMode); 
	void	tagFree(S_TTE * psTagEnc); 
	CHAR *	tagDecode(S_TTE * psTagEnc,CHAR *pSource,BOOL bFreeSource,BOOL bFreeTag); 

#endif