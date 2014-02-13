//   ---------------------------------------------
//   | ZCMP_RichText
//   | ZoneComponent RichText
//   |                                              
//   |						by Ferrà srl 2010
//   ---------------------------------------------

#include  <richedit.h>
#define WC_RICHTEXT L"ehRichText"
#define ID_RICHTEXT 53010

void * ehzRichText(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);

typedef struct {

	HWND	wnd;
	LRESULT (*subPostNotify)(EH_NOTIFYPARAMS);
	/*
	SINT iMode,
							 struct OBJ *poj,
						     HWND hWnd,
							 UINT msg,
							 WPARAM wParam,
						     LPARAM lParam,
						     BOOL *fReturn);
							 */
	HFONT		hFont;
	EN_STRENC	enEncode;	// T/F se unicode
	BOOL		bPaint;
	
	void	(*GetCarret)(void *this,POINT * ptScroll,CHARRANGE * psChar);
	void	(*SetCarret)(void *this,POINT * ptScroll,CHARRANGE * psChar);
	void	(*TextColor)(void *this,INT iStartPos,INT iEndPos, COLORREF crNewColor);
	void	(*CharFormat)(void *this,INT iStartPos,INT iEndPos, CHARFORMAT2 * psCharFormat);
	void	(*Paint)(void *this,BOOL bPaint);

} EHZ_RICHTEXT;
