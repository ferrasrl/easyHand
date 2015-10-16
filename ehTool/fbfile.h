#define FBEXT 8976

typedef struct
{
 struct OBJ *Obj;
 struct IPT *IPT;
 SINT (far *AddProcedure)(SINT ms); //      CHIAMATA ALLA ROUTINE
} FBFILEDRV;

typedef struct
{
	CHAR Nome[20];
	BOOL  Select;
} MULTIFILEINFO;

SINT path_leggi(CHAR *path,CHAR *drive);
SINT FileAsk(CHAR *Titolo,CHAR *Ext,CHAR *NomeFile,SINT FlagExist,CHAR *Tasto,CHAR *SopraIPT);
SINT FileAskF(CHAR *Titolo,CHAR *Ext,CHAR *NomeFile,SINT FlagExist,CHAR *Tasto,CHAR *SopraIPT,FBFILEDRV *);
SINT PathAsk(CHAR *Titolo,CHAR *NomePath,CHAR *Tasto,CHAR *SopraIPT);

void * listfile(EH_OBJPARAMS);
void * listdir(EH_OBJPARAMS);
void * listdrive(EH_OBJPARAMS);

#ifdef _WIN32
SINT FileAskOpenWin(CHAR *Titolo,CHAR szFilter[],CHAR *NomeFile,SINT FlagExist);
SINT FileAskSaveWin(CHAR *Titolo,CHAR szFilter[],CHAR *NomeFile,SINT FlagExist);
SINT PathAskWin(CHAR *Titolo,CHAR *NomePath);
BOOL PathAskWinEx(HWND hWnd,CHAR *Titolo,CHAR *NomePath);

INT	fileChoose(	HWND	hwnd,
				CHAR *	pszTitle,
				CHAR	arFilter[],
				CHAR *	pszPath,
				EH_LST	lstFiles,
				BOOL	bFileMultiple,
				BOOL	bFileMustExist);
#endif
