//   ---------------------------------------------
//   | ehzStaticText
//   | ZoneComponent RichText
//   |                                              
//   |						by Ferrà srl 2010
//   ---------------------------------------------

#define WC_STATICTEXT L"ehStaticText"
#define ID_STATICTEXT 53011

void * ehzStaticText(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);

typedef struct {
	EH_FONT		* psFont;
	BOOL		bFreeFontEnd;	// T/F libera il font in uscita
	EN_STRENC	enEncode;	// T/F se unicode
	WCHAR *		pwcText;
	UINT		uFormat;
	EH_COLOR	colText;
	EH_COLOR	colBack;

} EHZ_STATICTEXT;
