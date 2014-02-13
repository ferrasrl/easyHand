void *EhIperButton(SINT cmd,LONG info,void *ptr);

#define IB_MAX 50

#define IB_NORMAL 0
#define IB_OVER 1
#define IB_PRESS 2

typedef struct {
	struct OBJ *lpObj;
	HWND hWndList;
	CHAR szIconeNormal[200];
	CHAR szIconeOver[200];
	CHAR szIconePress[200];
	SINT iStatus;
	SINT iLastStatus;
	BOOL fButton;
} EH_IBLIST;
