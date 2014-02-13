//   ---------------------------------------------
//   | ZCMP_Form
//   ---------------------------------------------

#define WC_EH_FORM "ehForm"
#define WC_EH_TITLE "ehFormTitle"
#define WC_EH_ZONE "ehFormZone"
#define WC_EH_FORM_TEXT L"ehFormText"
#define WC_EH_FORM_SELECT "ehFormSelect"
#define WC_EH_FORM_BUTTON "ehFormButton"
#define WC_EH_FORM_LIST "ehFormList"
#define ID_FORM 12100

typedef enum {

	FLD_TITLE,	// Titolo della sezione

	FLD_TEXT,
	FLD_NUMBER,
	FLD_DATE,
	FLD_TEXTAREA,
	FLD_PASSWORD,
	
	FLD_BUTTON,
	FLD_CHECKBOX,
	FLD_RADIO,
	FLD_SELECT,			
	FLD_LIST,			

	FLD_ZCMP,	// é una zona per usare le zCmp

	FLD_APPEND=0x100,
	FLD_QNOT=0x200		// Da non usare in query con il db (in Get)
	
} EN_FORM_IPT;
#define FLD_TYPE_MASK 0xff

typedef enum {
	FDM_DMY8,	// ddmmyyyy
	FDM_YMD8,	// yyyymmdd
	FDM_DT		// yyyymmdd_hhmmss
} EN_FDM; // Formar Date Mode

typedef enum {
	FCLS_UNKNOW,
	FCLS_BUTTON,
	FCLS_TEXT,
	FCLS_SELECT,
	FCLS_LIST,
	FCLS_DATE
//	FCLS_SELECT_MULTI,

} EN_FLD_CLS;

typedef struct {

	void	*	psForm;
	void	*	psParent;	// Campo parente (serve per raggruppare i titolo)
	EN_FORM_IPT	iType;	// Tipo di input
	EN_FLD_CLS	enClass;	// Interno: Testo,Select,Check

	CHAR *		pszName;	// Nome del campo (usato per rintracciarlo e negli eventi)
	CHAR *		pszText;	// Testo di descrizione della riga (del campo)
	CHAR *		pszButton;	// Testo del "button" (usato con i button o a destra del campo)
	CHAR *		pszRight;	// Testo a destra del campo
	INT			iWidth;		// Larghezza 0=fino alla fine del form
	BOOL		bWidthPerc;	// T/F iWidth è una percentuale dell'area client
	INT			iMinWidth;	// Dimensioni minima
	INT			iMaxWidth;	// Dimensione massima


	INT			iHeight;	// Larghezza 0=automatica (altezza riga)
	INT			iAfterWidth;// Larghezza del testo after (dopo il campo di test0) 0= automatica
	INT			iAlt;		// Altezza del campo (0=Default)
	INT			iRow;		// Riga (Virtuale) della tabella
	EH_COLOR	colText;	
	RECT		rcMargin;
	EN_FDM		enFdm;
	
	INT			iTitleStyle;	// Usato solo per gli oggetti TITOLO
	INT			iTitleWidth;	// Larghezza alternativa del nome (titolo) del campo
	EN_DPL		enTitleAlign;	// Allineamento del nome del campo
	DWORD		dwTitleParam;	// Pametri di allineamento del titolo
	BOOL		bTitleGroup;	// T/F se il titolo è richiudibile
	BOOL		bTitleClose;	// T/F se il titolo (gruppi di input) è chiuso

	BYTE *		pszValue;	// Valore del campo ANSI
//	WCHAR *		pwcValue;	// Valore del campo UNICODE

	BOOL		bAppend;	// Accodato al precedente
	BOOL		bNotAppend;	// T/F se non va appeso (Interno per auto a capo)
	BOOL		bQueryNot;	// T/F se non è un campo da associare ad una query (esempio testuali o descrittivi)

//	BOOL		bVisible;	// T/F se è visibile
	BOOL		bDisable;	// T/F se disabilitato alla modifica
	BOOL		bVisible;	// T/F il campo non è visible (ma inserito nel riposizionamento)
	BOOL		bExclude;	// T/F se va escluso dal form (il campo è presente ma non inserito nel riposizionamento)

	EH_AR		arParam;	// Parametri aggiuntivi
	EH_AR		arOption;	// Elenco di opzioni per i campi select
	BOOL		bCheck;		// Se settato per i checkbox
	INT			iTextRows;	// Numero di righe (Text/Area)
	INT			iMaxChar;	// Numero massimo di caratteri
	BOOL		bReadOnly;	// T/F se sono il lettura
	
	INT			iNumberSize;		// Dimensioni del numero intero (1000 = 4)
	INT			iNumberDecimal;		// Numero di decimali
	BOOL		bNumberThousandSep; // T/F Separatore delle migliaia

	// Windows
	HWND		wndTitle;	// Handle del titolo
	RECT		recTitle;
	BOOL		bTitleSizeMax;
	
	HWND		wndInput;	// Handle del titolo
	RECT		recInput;
	BOOL		bInputSizeMax;

	HWND		wndAfter;	// Handle del titolo
	RECT		recAfter;
	
	RECT		recDiv;		// Inteso come blocco Titolo+Input+After

	// Emulazione per eredità
	EH_OBJ		sObj;
	void *		(*funcExtern)(struct OBJ *psObj,EN_MESSAGE enMess,LONG lParam,void *pVoid);
	EH_FONT	*	psFont;		// Font Custom 
	EH_FONT	*	psFontApply;	// Font applicato
	void *		pvIptInfo;	// Mi serve per liberare memoria alla fine

} EH_FORM_FIELD;


void * ehzForm(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);

typedef struct {

	struct OBJ *	psObj;
	HWND		wnd;
	_DMI		dmiField;
	EH_FORM_FIELD *arField;
	BOOL		bBuilded;		// Form Pronto per essere visualizzato	
	EN_STRENC	enEncode;		// SE_ANSI (Default)
	INT			iErrorLevel;	// 0= No error, 1=Warning, 2=Several
	BOOL		bCRSubmit;		// T/F se accetta l'invio come submit nel FORM

	// Font da usare
	EH_FONT	*	psFontTitle;
	EH_FONT	*	psFontInput;

	// Colonna titoli
	CHAR	*	pszTitleWidth;	// Es 30% oppure 90
	INT			iTitleWidth;	// Larghezza titolo a sinistra
	INT			iTitleMin;		// 
	INT			iTitleMax;		// 
	INT			iRowHeight;			// Altezza di un campo di input di default

	RECT		rcClient;
	SIZE		sizClient;		// Dimensioni dell'area client
	SIZE		sizForm;		// Dimensioni del form
	RECT		recForm;		// Rect del form
	RECT		recFormPadding;
	INT			iCellPadding;	// Padding fra le celle
	INT			ofsVert;		// Offset verticale
	INT			ofsHorz;		// Offset orizzontale
	INT			idxFocus;		// indice del campo in Focus
	EH_FORM_FIELD * psGrpTitle;	// Ultimo titolo inserito (per raggruppare gli input)
	EH_FORM_FIELD * psFldFocus;	// Nome del campo in focus (NULL=NO)
	EH_AR		arBlurNotify;	// Array con i campi semsibili ad evento BLUR 
	EH_AR		arCharNotify;	// Array con i campi semsibili ad evento CHAR
	void *		(*funcNotify)(void * this,EH_SRVPARAMS); // Funzione per notifiche esterne sul form


	void	(*Reset)(void *);
	BOOL	(*Add)(void *, EN_FORM_IPT iType, CHAR *pName, CHAR *pszText,CHAR *pszButton,CHAR *pszParam); 
	void	(*addEx)(void *this, EN_FORM_IPT iType, CHAR * pszName, CHAR *pszText,CHAR *pszButton,CHAR *pszParam,void * (*funcExtern)(EH_OBJPARAMS));
	void	(*addTitle)(void *this, CHAR * pszName, CHAR * pszText,CHAR *pszParam);

	BOOL	(*Show)(void *); //  Visualizza il form
	BOOL	(*Redraw)(void *); //  Ridisegna il form senza richiedere uno Show (solo reposition)
	BOOL	(*SetOptions)(void *,CHAR *pszId,EH_AR ar);
	BOOL	(*Focus)(void *,CHAR *pszId);
	void *	(*Get)(void *,CHAR *pszId);
	double	(*GetNumber)(void *,CHAR *pszId);
	HWND	(*getWnd)(void *,CHAR *pszId);
	EH_FORM_FIELD * (*getFld)(void *,CHAR *pszId);

	BOOL	(*Set)(void *,CHAR *pszId,void *pszValue);
	BOOL	(*SetNumber)(void *,CHAR *pszId,double dValue);
	void	(*SetTitle)(void *,CHAR *pszId,void *pszValue);
	void	(*SetAfter)(void *,CHAR *pszId,void *pszValue);
	void	(*SetParams)(void *,CHAR *pszId,CHAR * pszParams,...);

	BOOL	(*BlurNotify)(void *this,CHAR *pszList);
	BOOL	(*CharNotify)(void *this,CHAR *pszList);

	void	(*Clean)(void *this,CHAR *pszFieldFocus,CHAR *lstFldNotClean);
	void	(*Enable)(void *this,CHAR *pszFieldFocus,BOOL bEnable);
	void	(*Exclude)(void * this,CHAR * pszNames,BOOL bExclude);

	void	(*Visible)(void *this,CHAR *pszFieldFocus,BOOL bVisible);
	void	(*SetFunction)(void *this,CHAR *pszField,void * (*funcExtern)(EH_OBJPARAMS));
	void	(*SendMessage)(void *this,CHAR *pszField, INT cmd, LONG info,void *ptr);
	void	(*Refresh)(void *this,CHAR *pszField);
	void	(*setNotifyFunc)(void *this,void * (*funcExtern)(void * this,EH_SRVPARAMS));
	void	(*ensureVisible)(void *this,CHAR * pszField);
//	BOOL	(*SqlGet)(void *this,BYTE *pQuery);
#ifdef SQL_RS
	BOOL	(*SqlGetRs)(void *this,SQL_RS rsSet);
	BOOL	(*SqlSelect)(void *this,CHAR *pszQuery,...);
	BOOL	(*SqlUpdate)(void *this,CHAR *pszFields,CHAR *pszQuery,...);
	BOOL	(*SqlInsert)(void *this,CHAR *pszFields,CHAR *pszQuery,...);
#endif

// Sezione ADB functions
#if (defined(_ADB32)||defined(_ADB32P))
	BOOL	(*adbRead)(void * this,HDB hdb);
	BOOL	(*adbWrite)(void * this, HDB hdb, CHAR * pszFields);
	BOOL	(*adbGetDifference)(void *this, HDB hdb, HREC hRec, EH_LST lstField);
#endif

} EHZ_FORM;


typedef struct {

	EHZ_FORM *psForm;
	INT	idxField;
	EH_FORM_FIELD * psFld;

} EH_IPT_INFO;

