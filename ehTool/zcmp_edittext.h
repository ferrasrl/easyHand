//   ---------------------------------------------
//   | ZCMP_EditText
//   | ZoneComponent EditText
//   |                                              
//   | Gestisce una ListBox di window                                             
//   | in una oggetto ZONAP                        
//   |                                              
//   |  ATTENZIONE:                               
//   |  Inserire in winstart.c                                            
//   |                                              
//   |  case WM_NOTIFY: return ListBoxNotify(hWnd, lParam);                                              
//   |                                              
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

#define ID_STATICTEXT 53000
#define ID_STATICTEXTW 53001

void * ehzEditText(struct OBJ *objCalled,EN_MESSAGE cmd,LONG info,void *ptr);
#define EhEditText ehzEditText
//void * EhEditTextW(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr);

typedef struct {
	HWND		hWnd;
	EH_OBJ	*	psObj;
	LRESULT		(*subPostNotify)(EH_NOTIFYPARAMS);
	HFONT		hFont;
	EN_STRENC	enEncode;	// Es. SE_ANSI (=CHAR),SE_UTF8 (=CHAR),SE_UNICODE (=WCHAR)

} EHZ_EDITTEXT;
