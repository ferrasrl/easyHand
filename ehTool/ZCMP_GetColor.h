//   ---------------------------------------------
//   | ZCMP_GetColor
//   |                                              
//   |                                              
//   |         by Ferrà Art & Tecnology 1993-2000
//   ---------------------------------------------

typedef struct {
	struct OBJ *lpObj;
	LONG lColor;
	//HWND hWndTrack;
	LRESULT (*subPostNotify)(SINT iMode,
							 struct OBJ *poj,
						     HWND hWndTrackBar, // 
						     WPARAM wParam,
							 LPARAM lParam,
						     BOOL *fReturn);
} EH_COLORLIST;
void * EhGetColor(EH_OBJPARAMS);

