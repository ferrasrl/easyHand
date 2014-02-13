//	-------------------------------------------
//	| FBFileW  FileAsk                        |
//	|                                         |
//	|                                         |
//	-------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include <shlobj.h>

#include "/easyhand/ehtool/fbfile.h"


#ifdef _WIN32

SINT FileAskOpenWin(CHAR *Titolo,CHAR szFilter[],CHAR *NomeFile,SINT FlagExist)
{
    OPENFILENAME ofn;
	BOOL Ret=FALSE;
/*
	static CHAR szFilter[]= "Tabella di Claudio (*.tdc)\0*.tdc\0" \
							"Tutti i file (*.*)\0*.*\0\0";
*/	
	CHAR File[MAXPATH];
	strcpy(File,NomeFile);

	ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = WIN_info[sys.WinInputFocus].hWnd;
    ofn.hInstance         = sys.EhWinInstance;
    ofn.lpstrFilter       = szFilter;
	ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = File; // File che si vogliono indicare
    ofn.nMaxFile          = _MAX_PATH; // Grandezza del Buffer
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;//Appoggio.Path;
//    ofn.lpstrTitle        = ultTranslateCheck(ULT_TYPE_WINI,Titolo);
    ofn.lpstrTitle        = ultTranslate(ULT_TYPE_WINI,Titolo);
    ofn.nFileOffset       = 0;//strlen(szFile)+1; // Offset dei file
    ofn.nFileExtension    = 0;//"\"*.*\"";
    ofn.lpstrDefExt       = NULL;
    ofn.lCustData         = 0;//(LPARAM)&sMyData; // Passo un Puntatore ai Miei dati
	ofn.lpfnHook 		   = NULL;//ComDlg32DlgProc; // Procedura agganciata
	ofn.lpTemplateName    = NULL;//MAKEINTRESOURCE(IDD_COMDLG32); // Dialogo agganciato
    ofn.Flags             = OFN_EXPLORER |    // Abilito il tipo Explorer
							OFN_HIDEREADONLY;// |    // Nascondo Read only
							 //OFN_SHOWHELP | 
		                     //OFN_NOVALIDATE|
							//OFN_CREATEPROMPT;
	if (FlagExist)  ofn.Flags|=OFN_FILEMUSTEXIST;

	ipt_noedit();
    //if (GetOpenFileName(&ofn)) {FileTouch=TRUE;ipt_writevedi(5,TLayOut.FileTabella,0);}
    if (GetOpenFileName(&ofn)) 
	{strcpy(NomeFile,File);
     Ret=TRUE;
	}

// sys.prs1_x=sys.ms_x; 
// sys.prs1_y=sys.ms_y; 
 return Ret;
}

SINT FileAskSaveWin(CHAR *Titolo,CHAR szFilter[],CHAR *NomeFile,SINT FlagExist)
{
    OPENFILENAME ofn;
/*
	static CHAR szFilter[]= "Tabella di Claudio (*.tdc)\0*.tdc\0" \
							"Tutti i file (*.*)\0*.*\0\0";
*/	
		CHAR File[MAXPATH];
	strcpy(File,NomeFile);

	ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = WIN_info[sys.WinInputFocus].hWnd;
    ofn.hInstance         = sys.EhWinInstance;
    ofn.lpstrFilter       = szFilter;
	ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = File; // File che si vogliono indicare
    ofn.nMaxFile          = _MAX_PATH; // Grandezza del Buffer
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;//Appoggio.Path;
    ofn.lpstrTitle        = ultTranslate(ULT_TYPE_WINI,Titolo);
    ofn.nFileOffset       = 0;//strlen(szFile)+1; // Offset dei file
    ofn.nFileExtension    = 0;//"\"*.*\"";
    ofn.lpstrDefExt       = NULL;
    ofn.lCustData         = 0;//(LPARAM)&sMyData; // Passo un Puntatore ai Miei dati
	ofn.lpfnHook 		   = NULL;//ComDlg32DlgProc; // Procedura agganciata
	ofn.lpTemplateName    = NULL;//MAKEINTRESOURCE(IDD_COMDLG32); // Dialogo agganciato
    ofn.Flags             = OFN_EXPLORER |    // Abilito il tipo Explorer
							OFN_HIDEREADONLY;// |    // Nascondo Read only
							 //OFN_SHOWHELP | 
		                     //OFN_NOVALIDATE|
							//OFN_CREATEPROMPT;
	if (FlagExist)  ofn.Flags|=OFN_FILEMUSTEXIST;

	ipt_noedit();
    //if (GetOpenFileName(&ofn)) {FileTouch=TRUE;ipt_writevedi(5,TLayOut.FileTabella,0);}
    if (GetSaveFileName(&ofn)) 
	{strcpy(NomeFile,File);
     return TRUE;
	}
 return FALSE;
}

// +-----------------------------------------+
// | 	  Elenco dei percorsi disponibili	 |
// +-----------------------------------------+

static void * DTreeView(SINT cmd,LONG info,CHAR *str);
static SINT CALLBACK BrowseNotify(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
static CHAR *lpPath;

BOOL PathAskWin(CHAR *Titolo,CHAR *NomePath)
{

	BOOL  fRet;
	LPITEMIDLIST pidl;
//	LPITEMIDLIST pidlRoot;
	LPMALLOC lpMalloc;

	BROWSEINFO bi = {
		WindowNow(),
		NULL,
		NomePath,
		ultTranslate(ULT_TYPE_WINI,Titolo),
		BIF_RETURNONLYFSDIRS,
		BrowseNotify, 
		0L, 0	};

	lpPath=NomePath;

//	Questo fa' visualizzare solo le directory
//	if (0 != SHGetSpecialFolderLocation(HWND_DESKTOP, CSIDL_DRIVES, &pidlRoot)) return FALSE;
//	if (NULL == pidlRoot) return FALSE;
//	bi.pidlRoot = pidlRoot;

	ipt_noedit();
	pidl = SHBrowseForFolder(&bi);

	if (NULL != pidl) fRet = SHGetPathFromIDList(pidl, NomePath);
	                  else
		              fRet = FALSE;

	// Get the shell's allocator to free PIDLs
	if (!SHGetMalloc(&lpMalloc) && (NULL != lpMalloc))
	{
		/*
		if (NULL != pidlRoot)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidlRoot);
		}
*/
		if (NULL != pidl)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidl);
		}

		lpMalloc->lpVtbl->Release(lpMalloc);
	}

 return fRet;
}

BOOL PathAskWinEx(HWND hWnd,CHAR *Titolo,CHAR *NomePath)
{
	BOOL  fRet;
	LPITEMIDLIST pidl;
//	LPITEMIDLIST pidlRoot;
	LPMALLOC lpMalloc;

	BROWSEINFO bi = {
		hWnd,
		NULL,
		NomePath,
		ultTranslate(ULT_TYPE_WINI,Titolo),
		BIF_RETURNONLYFSDIRS,
		BrowseNotify, 
		0L, 0	};

	lpPath=NomePath;

//	Questo fa' visualizzare solo le directory
//	if (0 != SHGetSpecialFolderLocation(HWND_DESKTOP, CSIDL_DRIVES, &pidlRoot)) return FALSE;
//	if (NULL == pidlRoot) return FALSE;
//	bi.pidlRoot = pidlRoot;

	//ipt_noedit();
	pidl = SHBrowseForFolder(&bi);

	if (NULL != pidl) fRet = SHGetPathFromIDList(pidl, NomePath);
	                  else
		              fRet = FALSE;

	// Get the shell's allocator to free PIDLs
	if (!SHGetMalloc(&lpMalloc) && (NULL != lpMalloc))
	{
		/*
		if (NULL != pidlRoot)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidlRoot);
		}
*/
		if (NULL != pidl)
		{
			lpMalloc->lpVtbl->Free(lpMalloc, pidl);
		}

		lpMalloc->lpVtbl->Release(lpMalloc);
	}

 return fRet;
}

static SINT CALLBACK BrowseNotify(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

	switch (uMsg)
	{
	case BFFM_INITIALIZED:
	{
		// Set initial folder
		/*
		LPSTR pszEnd = PathAddBackslash(pThis->m_pfdin->psz3);
		if (pszEnd - pThis->m_pfdin->psz3 > 3)
		{
			// No problems if not drive root
			*(pszEnd - 1) = '\0';
		}
		*/
//		SendMessage(hwnd, BFFM_SETSELECTION, 1, (LPARAM)pThis->m_pfdin->psz3);
		SendMessage(hwnd, BFFM_SETSELECTION, 1, (LPARAM) lpPath);
		break;
	}

	default:
		return(0);
	}
return(1);
}
#endif