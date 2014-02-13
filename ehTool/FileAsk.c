//	+-----------------------------------------+
//	| FileAsk                                 |
//	|                                         |
//	|                                         |
//	+-----------------------------------------+

#include "\ehtool\include\ehsw_i.h"
#include "c:\ehtool\fbfile.h"


SINT FileAskWin(CHAR *Titolo,CHAR *Ext,CHAR *NomeFile,SINT FlagExist,CHAR *Tasto,CHAR *SopraIPT)
{
    OPENFILENAME ofn;
	static CHAR szFilter[]= "Tabella di Claudio (*.tdc)\0*.tdc\0" \
							"Tutti i file (*.*)\0*.*\0\0";
	

	ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = WindowNow();//WIN_info[sys.WinInputFocus].Hwnd;
    ofn.hInstance         = sys.EhWinInstance;
    ofn.lpstrFilter       = szFilter;
	ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = NomeFile; // File che si vogliono indicare
    ofn.nMaxFile          = _MAX_PATH; // Grandezza del Buffer
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;//Appoggio.Path;
    ofn.lpstrTitle        = Titolo;
    ofn.nFileOffset       = 0;//strlen(szFile)+1; // Offset dei file
    ofn.nFileExtension    = 0;//"\"*.*\"";
    ofn.lpstrDefExt       = NULL;
    ofn.lCustData         = 0;//(LPARAM)&sMyData; // Passo un Puntatore ai Miei dati
	ofn.lpfnHook 		   = NULL;//ComDlg32DlgProc; // Procedura agganciata
	ofn.lpTemplateName    = NULL;//MAKEINTRESOURCE(IDD_COMDLG32); // Dialogo agganciato
    ofn.Flags             = OFN_EXPLORER |    // Abilito il tipo Explorer
							OFN_HIDEREADONLY |    // Nascondo Read only
							 //OFN_SHOWHELP | 
		                     //OFN_NOVALIDATE|
							OFN_CREATEPROMPT;
	if (FlagExist)  ofn.Flags|=OFN_FILEMUSTEXIST;

	ipt_noedit();
    //if (GetOpenFileName(&ofn)) {FileTouch=TRUE;ipt_writevedi(5,TLayOut.FileTabella,0);}
    GetOpenFileName(&ofn);
}
