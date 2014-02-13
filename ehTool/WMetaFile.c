//   ---------------------------------------------
//	 | WMETAFILE                                 |
//	 | Gestione Windows Metafile
//	 |                             by Ferrà 2004 |
//   ---------------------------------------------

#include "\ehtool\include\ehsw_i.h"

PROC    gpfnGetEnhMetaFilePixelFormat = (PROC)NULL; 
HMODULE  hMod; 

BOOL    gbUseMfPFD = TRUE; //??
BOOL    gbDB = FALSE;
// 
// GDI32 may not support the OpenGL enhanced metafile (Win95 doesn't), in 
// which case, the entry point doesn't exist, so we will do a GetProcAddress 
// instead. 
// 
static void DllControl(void)
{
	if (gpfnGetEnhMetaFilePixelFormat) return;

    hMod = LoadLibrary("GDI32.DLL"); 
    if (hMod != (HMODULE)NULL) { 
 
        gpfnGetEnhMetaFilePixelFormat = 
                GetProcAddress(hMod, "GetEnhMetaFilePixelFormat"); 
 
        if (gpfnGetEnhMetaFilePixelFormat == (PROC)NULL) 
			win_infoarg("Main: GetProcAddress(GetEnhMetaFilePixelFormat) failed"); 
 
    } else { 
        PRG_end("Main: LoadLibrary(GDI32) failed");}
}

BOOL APIENTRY bPlayRecord(HDC hDC, LPHANDLETABLE lpHandleTable, 
                                   LPENHMETARECORD lpEnhMetaRecord, 
                                   UINT nHandles, 
                                   LPVOID lpData) { 
    BOOL bSuccess; 
    static int  iCnt=0; 
    int         i; 
    char        ach[128]; 
    char        achTmp[128]; 
    LONG        lNumDword; 
 
    bSuccess = TRUE; 
 
    lNumDword = (lpEnhMetaRecord->nSize-8) / 4; 
 
    iCnt++; 
//    if (((PLAYINFO *) lpData)->bPlayContinuous) { 
        bSuccess = PlayEnhMetaFileRecord(hDC, lpHandleTable, 
                                             lpEnhMetaRecord, nHandles); 
		/*
        if (iCnt == ((PLAYINFO *) lpData)->iRecord) { 
            wsprintf((LPSTR) ach, "%s", rgMetaName[lpEnhMetaRecord->iType]); 
            for (i=0; i < lNumDword; i++) { 
                wsprintf((LPSTR) achTmp, "%ld ", lpEnhMetaRecord->dParm[i]); 
                if ((strlen(ach)+strlen(achTmp))/sizeof(char) >= 128) 
                    break; 
                strcat(ach, achTmp); 
            } 
        SetWindowText(ghTextWnd, ach); 
		*/
        //} 
/*
    } else { 
 
        switch (lpEnhMetaRecord->iType) { 
            case EMR_SETWINDOWEXTEX: 
            case EMR_SETWINDOWORGEX: 
            case EMR_SETVIEWPORTEXTEX: 
            case EMR_SETVIEWPORTORGEX: 
            case EMR_SETBRUSHORGEX: 
            case EMR_SETMAPMODE: 
            case EMR_SETBKMODE: 
            case EMR_SETPOLYFILLMODE: 
            case EMR_SETROP2: 
            case EMR_SETSTRETCHBLTMODE: 
            case EMR_SETTEXTALIGN: 
            case EMR_SETTEXTCOLOR: 
            case EMR_SETBKCOLOR: 
            case EMR_OFFSETCLIPRGN: 
            case EMR_MOVETOEX: 
            case EMR_SETMETARGN: 
            case EMR_EXCLUDECLIPRECT: 
            case EMR_INTERSECTCLIPRECT: 
            case EMR_SCALEVIEWPORTEXTEX: 
            case EMR_SCALEWINDOWEXTEX: 
            case EMR_SAVEDC: 
            case EMR_RESTOREDC: 
            case EMR_SETWORLDTRANSFORM: 
            case EMR_MODIFYWORLDTRANSFORM: 
            case EMR_SELECTOBJECT: 
            case EMR_CREATEPEN: 
            case EMR_CREATEBRUSHINDIRECT: 
            case EMR_DELETEOBJECT: 
            case EMR_SELECTPALETTE: 
            case EMR_CREATEPALETTE: 
            case EMR_SETPALETTEENTRIES: 
            case EMR_RESIZEPALETTE: 
            case EMR_REALIZEPALETTE: 
            case EMR_SETARCDIRECTION: 
            case EMR_SETMITERLIMIT: 
            case EMR_BEGINPATH: 
            case EMR_ENDPATH: 
            case EMR_CLOSEFIGURE: 
            case EMR_SELECTCLIPPATH: 
            case EMR_ABORTPATH: 
            case EMR_EXTCREATEFONTINDIRECTW: 
            case EMR_CREATEMONOBRUSH: 
            case EMR_CREATEDIBPATTERNBRUSHPT: 
            case EMR_EXTCREATEPEN: 
                goto PlayRec; 
            default: 
                break; 
        } //switch 
 
        if (iCnt == ((PLAYINFO *) lpData)->iRecord) { 
PlayRec: 
            bSuccess = PlayEnhMetaFileRecord(hDC, lpHandleTable, 
                                             lpEnhMetaRecord, nHandles); 
            wsprintf((LPSTR) ach, "%s", rgMetaName[lpEnhMetaRecord->iType]); 
            for (i=0; i < lNumDword; i++) { 
                wsprintf((LPSTR) achTmp, "%ld ", lpEnhMetaRecord->dParm[i]); 
                if ((strlen(ach)+strlen(achTmp))/sizeof(char) >= 128) 
                    break; 
                strcat(ach, achTmp); 
            } 
            SetWindowText(ghTextWnd, ach); 
        } 
    } 
 
    if (iCnt == ((PLAYINFO *) lpData)->iRecord) { 
        iCnt = 0; 
 
        if (gbDB) 
            SwapBuffers(hDC); 
 
        return FALSE; 
    } 
	*/
    return bSuccess; 
}

BOOL bSetupRC(HDC hDC, PIXELFORMATDESCRIPTOR *ppfdIn) 
{ 
    PIXELFORMATDESCRIPTOR   pfd, *ppfd; 
    INT                     iPfmt; 
    BOOL                    bRet=FALSE; 
    HPALETTE                hPal=(HPALETTE)NULL; 
    HGLRC                   hRC; 
    char                    text[128]; 
 
    if ((ppfdIn != (PIXELFORMATDESCRIPTOR*)NULL) && gbUseMfPFD) { 
        ppfd = ppfdIn; 
    } else { 
        ppfd = &pfd; 
        pfd.nSize   = sizeof(PIXELFORMATDESCRIPTOR); 
        pfd.nVersion= 1; 
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL; 
        pfd.iPixelType  = PFD_TYPE_RGBA; 
        pfd.cColorBits  = 24; 
        pfd.cRedBits    = 
        pfd.cRedShift   = 
        pfd.cGreenBits  = 
        pfd.cGreenShift = 
        pfd.cBlueBits   = 
        pfd.cBlueShift  = 
        pfd.cAlphaBits  = 
        pfd.cAlphaShift = 0; 
        pfd.cAccumBits  = 24; 
        pfd.cAccumRedBits   = 
        pfd.cAccumGreenBits = 
        pfd.cAccumBlueBits  = 
        pfd.cAccumAlphaBits = 0; 
        pfd.cDepthBits  = 24; 
        pfd.cStencilBits = 24; 
        pfd.cAuxBuffers = 
        pfd.iLayerType  = 
        pfd.bReserved   = 0; 
        pfd.dwLayerMask = PFD_MAIN_PLANE; 
        pfd.dwVisibleMask = 0; 
        pfd.dwDamageMask = 0; 
    } 
 /*
    if ((iPfmt = ChoosePixelFormat(hDC, ppfd)) != 0) { 
        bRet = SetPixelFormat(hDC, iPfmt, ppfd); 
 
        gbDB = (ppfd->dwFlags & PFD_DOUBLEBUFFER) ? TRUE : FALSE; 
 
        if ((hPal = hCreateRGBPalette(hDC, iPfmt)) != (HPALETTE)NULL) { 
            SelectPalette(hDC, hPal, FALSE); 
            RealizePalette(hDC); 
            bRet = bRet && TRUE; 
        } else { 
            bRet = FALSE; 
        } 
		bRet=FALSE;
        hRC = wglCreateContext(hDC); 
        wglMakeCurrent(hDC, hRC); 
 
        win_infoarg( "bSetupRC: hDC=%lx, hRC=%lx", hDC, hRC); 
 
    } 
 */
    return bRet; 
}


BOOL bSetRC2MatchEmfRC(HDC                      hDC, 
                       ENHMETAHEADER            EnhMetaHdr, 
                       HENHMETAFILE             hEnhMf, 
                       PIXELFORMATDESCRIPTOR    *ppfdIn) 
{ 
    BOOL                    bHasPFD = FALSE, bRCSet = FALSE; 
    char                    text[128]; 
    PIXELFORMATDESCRIPTOR   pfd; 
 
    if (EnhMetaHdr.bOpenGL) { 
        if (gpfnGetEnhMetaFilePixelFormat != (PROC)NULL) { 
 
            bHasPFD = ((gpfnGetEnhMetaFilePixelFormat)(hEnhMf, 
                                      sizeof(PIXELFORMATDESCRIPTOR), 
                                      &pfd) == 0) ? FALSE : TRUE; 
 
            bRCSet = bSetupRC(hDC, (bHasPFD ? &pfd : NULL)); 
 
            sprintf(text, 
                     "bSetRC2MatchEmfRC: bRCSet=%ld, bHasPFD=%ld", 
                     bRCSet, bHasPFD); 
            win_infoarg(text); 
 
        } else { 
            PRG_end("bSetRC2MatchEmfRC: gpfnGetEnhMetaFilePixelFormat is NULL"); 
        } 
		win_infoarg("Cazzus");
    } else { 
        win_infoarg("bSetRC2MatchEmfRC: EnhMetaHdr.bOpenGL is FALSE"); 
    } 
 
    if (ppfdIn != (PIXELFORMATDESCRIPTOR*)NULL) { 
        if (bHasPFD) { 
            memcpy(ppfdIn, &pfd, sizeof(PIXELFORMATDESCRIPTOR)); 
        } else { 
            ppfdIn->nSize = 0; 
        } 
    } 
 
    return bRCSet; 
} 

LONG ViewMeta(void) 
{ 
    HDC           hDCDrawSurf; 
    ENHMETAHEADER EnhMetaHdr; 
    RECT          rcClientDS; 
    int           iRecord; 
//    PLAYINFO      PlayInfo; 
    BOOL          bRCSet = FALSE; 
	HENHMETAFILE  ghMetaf;
 
	//win_infoarg("CI ARRIVO");
	DllControl();
	ghMetaf =GetClipboardData(CF_ENHMETAFILE);

    if (ghMetaf == 0) return 0L; 
 
    GetEnhMetaFileHeader(ghMetaf, sizeof(EnhMetaHdr), &EnhMetaHdr); 
    iRecord = 1;//LOWORD(wParam) - DID_ZERO; 
    //SetDlgItemInt(ghwndCtrlPanel, DID_COUNTER, iRecord, FALSE); 
    //PlayInfo.iRecord = iRecord; 
    //PlayInfo.bPlayContinuous = FALSE; 
    //*piPlus = 0; 
 
    if ((EnhMetaHdr.nRecords > 1) && (iRecord > 0) && 
        (iRecord <= (INT) EnhMetaHdr.nRecords)) { 
        hDCDrawSurf = GetDC(WindowNow()); 
 
        bRCSet = bSetRC2MatchEmfRC(hDCDrawSurf, EnhMetaHdr, ghMetaf, NULL); 
		
        if (!bRCSet) { 
            win_infoarg("lProcessDIDPlayStep failed"); 
        } 
 
//        if (gbFit2Wnd) { 
  //          GetClientRect(ghwndDrawSurf, &rcClientDS); 
    //        EnumEnhMetaFile(hDCDrawSurf, ghMetaf, (ENHMFENUMPROC)bPlayRecord, (LPVOID) &PlayInfo, &rcClientDS); 
      //  } else { 
            EnumEnhMetaFile(hDCDrawSurf, ghMetaf, (ENHMFENUMPROC) bPlayRecord, (LPVOID) NULL, (LPRECT)&EnhMetaHdr.rclBounds); 
        //} 
        // 
        // Enabling the user to record a metafile record selectively 
        // 
 
        //if ((gbRecording) && (ghDCMetaf != NULL)) { 
         //   EnumEnhMetaFile(ghDCMetaf, ghMetaf, (ENHMFENUMPROC)bPlayRecord, (LPVOID) &PlayInfo, (LPRECT)&EnhMetaHdr.rclBounds); 
        //} 
 /*
        if (bRCSet) { 
            bCleanUpRC(); 
        } 
 */
        ReleaseDC(WindowNow(), hDCDrawSurf); 
    } 
    return 0L; 
}
