//   ---------------------------------------------------------
//   | WAnimation                                
//   | Gestore di servizi di animazione
//   | Windows 32bit Only
//   |                                           
//   | Tipo                                           
//   | FWA_VIRTUALDC                                          
//   | Contesto "clone" di dimensione stabilita collegato                                          
//   | ad una finestra proprietaria.                                          
//   | Permette di "costruire" in un'area "clone"
//   | prima di spedirlo in video.
//   | Crea un DC compatibile dove è possibile scrivere                                          
//   | con istruzione EH e non                                          
//   |                                           
//   | Tipo                                           
//   | FWA_ANIMATION
//   | Contesto multi-thread collegato ad una finestra proprietaria.                                          
//   | Crea un Thread con possibilità di definire frame per sec
//   | che invoca direttamente una subProcedure per la stampa
//   | Può essere abbinato alla sub GIFAnimation() per visualizzare
//   | le Gif animate.
//   |                                           
//   |							Created by Tassistro 05/2001  
//   ---------------------------------------------------------
#include "\ehtool\include\ehsw_i.h"
#include "\ehtool\dmiutil.h"
#include "\ehtool\imgutil.h"
#include "\ehtool\WAnimation.h"

static CRITICAL_SECTION csAnimation;
static DRVMEMOINFO dmiAnimaz=DMIRESET;
static FWANIM *lpFwaDirect;
static DWORD WINAPI ThreadAnimationDirect(LPVOID Dato);
static DWORD WINAPI ThreadAnimationChild(LPVOID Dato);
#define EANIM_BREAK 0
#define EANIM_ANALISE 1


// ---------------------------------------------------------------------
// Gestisce la sincronizzazione di accesso alle aree Frame Virtuali
// ---------------------------------------------------------------------

static void LocalFrameLock(SINT iAnim,SINT iFrame)
{
	if (iFrame==-1) return;
	EnterCriticalSection(&lpFwaDirect[iAnim].csFrameLock[iFrame]);
}

// ---------------------------------------------------------------------
// Gestisce la sincronizzazione di accesso alle aree Frame Virtuali
// ---------------------------------------------------------------------
static void LocalFrameUnlock(SINT iAnim,SINT iFrame)
{
   LeaveCriticalSection(&lpFwaDirect[iAnim].csFrameLock[iFrame]);
}

void *srvWAnimation(SINT cmd,LONG info,void *lpt)
{
	HANDLE hThread;
	DWORD dwThread;
	FWANIM sFwa;
	FWANIM sFwaB;
	FWANIM *lpFwa;
	SINT a;
	SINT iPt;
    WINSCENA Scena;
	HDC hDC;
	HDC hdcLocal;
	PAINTSTRUCT *lps;
	RECT rZone;
	RECT *lprZone;
	SINT iFrameShow;

	static SINT iNum;
	switch (cmd)
	{
		case WS_OPEN:	// Apro il gestore delle animazioni
			DMIOpen(&dmiAnimaz,RAM_AUTO,info,sizeof(FWANIM),"*WANIMAZ");
			lpFwaDirect=Wmemo_lock(dmiAnimaz.Hdl);
			InitializeCriticalSection(&csAnimation);
			break;

		case WS_CLOSE:	// Chiudo il gestore delle animazioni
			EnterCriticalSection(&csAnimation);
			// Prima chiudo i Thread
			for (a=0;a<dmiAnimaz.Num;a++)
			{
				DMIRead(&dmiAnimaz,a,&sFwa);
				switch (sFwa.iType)
				{
					case FWA_ANIMATION:
					case FWA_ANICHILD:
						SetEvent(sFwa.hEvent[EANIM_BREAK]);
						// Termino il Thread se non risponde
						if (WaitForSingleObject(sFwa.hThread,5000)==WAIT_TIMEOUT) TerminateThread(sFwa.hThread,1);
						CloseHandle(sFwa.hEvent[EANIM_BREAK]);
						CloseHandle(sFwa.hEvent[EANIM_ANALISE]);
						break;
				}
			}
			
			// Poi chiudo i VirtualDC
			for (a=0;a<dmiAnimaz.Num;a++)
			{
				DMIRead(&dmiAnimaz,a,&sFwa);
				switch (sFwa.iType)
				{
					case FWA_VIRTUALDC:
						DeleteObject(sFwa.hBitmapFrame[0]);
						DeleteObject(sFwa.hBitmapFrame[1]);
						DeleteDC(sFwa.hDCFrame[0]);
						DeleteDC(sFwa.hDCFrame[1]);
						DeleteCriticalSection(&lpFwaDirect[a].csFrameLock[0]);
						DeleteCriticalSection(&lpFwaDirect[a].csFrameLock[1]);
						break;
				}
			}

			LeaveCriticalSection(&csAnimation);
			DeleteCriticalSection(&csAnimation);
			DMIClose(&dmiAnimaz,"*WANIMAZ");
			break;
		
		case WS_REALGET:
			DMIRead(&dmiAnimaz,info,lpt);
			break;

		case WS_REALSET:

			EnterCriticalSection(&csAnimation);
			lpFwa=lpt;
			DMIRead(&dmiAnimaz,info,&sFwa);
			// -------------------------------------------------
			// Se è un'animazione DC e ho cambiato la posizione
			//
			if ((sFwa.rZone.left!=lpFwa->rZone.left||sFwa.rZone.top!=lpFwa->rZone.top)&&
				sFwa.fShow&&
				sFwa.iType==FWA_ANIMATION)
			{
				// Spengo l'animazione e aspetto
				sFwa.fShow=FALSE;
				sFwa.lProcess=TRUE; 
				DMIWrite(&dmiAnimaz,info,lpFwa);
				SetEvent(sFwa.hEvent[EANIM_ANALISE]); // Segnalo che deve leggere
				while (lpFwaDirect[info].lProcess) {}; // Attendo che abbia letto
			}

			// Auto sistemo il rettangolo
			lpFwa->rZone.bottom=lpFwa->rZone.top+lpFwa->sZone.cy-1;
			lpFwa->rZone.right=lpFwa->rZone.left+lpFwa->sZone.cx-1;
			if (lpFwa->iCount<0&&lpFwa->iType==FWA_ANICHILD) lpFwa->iCount=lpFwaDirect[info].iCount;
			switch (lpFwa->iType)
			{
				case FWA_VIRTUALDC: break;
				case FWA_ANIMATION: 
				case FWA_ANICHILD:
							if (lpFwa->fRun)
							{
								if (!lpFwa->iFrame) sFwa.iMill=INFINITE; else lpFwa->iMill=1000/lpFwa->iFrame;
							}
							else
							{
								lpFwa->iMill=INFINITE; 
							}
							break;
			}

			DMIWrite(&dmiAnimaz,info,lpFwa);
			switch (lpFwa->iType)
			{
				case FWA_VIRTUALDC: break;
				case FWA_ANIMATION: 
							//srvWAnimation(WS_PROCESS,sAmbient.hIDAnim[iGif],&sFwa2.rZone);
							// Chiedo un refresh della zona se non è più visibile
							//if (!lpFwa->fShow) InvalidateRect(lpFwa->hOwnerWnd,&lpFwa->rZone,FALSE);
				case FWA_ANICHILD:
							SetEvent(lpFwa->hEvent[EANIM_ANALISE]);
							break;
			}
			LeaveCriticalSection(&csAnimation);
			break;

		case WS_ADD: 

			// --------------------------------------
			// Aggiunge servizio di animazione
			//
			DMIAppend(&dmiAnimaz,lpt);
			iPt=dmiAnimaz.Num-1;
			// Creo eventi per il controllo del Thread
			DMIRead(&dmiAnimaz,iPt,&sFwa);
			switch (sFwa.iType)
			{
				// ----------------------------------------------------
				// Servizio di Virtual DC
				//
				case FWA_VIRTUALDC:
					sFwa.rZone.bottom=sFwa.rZone.top+sFwa.sZone.cy-1;
					sFwa.rZone.right=sFwa.rZone.left+sFwa.sZone.cx-1;
					// Creo una copia del DC da usare per VirtualDC
					// *** POINT DC ***
					hDC=GetDC(sFwa.hOwnerWnd);
					for (a=0;a<2;a++)
					{
					 sFwa.hDCFrame[a]=CreateCompatibleDC(hDC);
					 sFwa.hBitmapFrame[a]=CreateCompatibleBitmap(hDC,sFwa.sZone.cx,sFwa.sZone.cy);
					 SelectObject(sFwa.hDCFrame[a],sFwa.hBitmapFrame[a]);
					 SetMapMode(sFwa.hDCFrame[a], MM_TEXT);
					 SetBkMode(sFwa.hDCFrame[a],OPAQUE);
					 SetTextColor(sFwa.hDCFrame[a],0);
					 WinDirectDC(sFwa.hDCFrame[a],&Scena);
					 Tboxp(0,0,sFwa.sZone.cx,sFwa.sZone.cy,sFwa.cBackGround,SET);
					 WinDirectDC(0,&Scena);
					}
					ReleaseDC(sFwa.hOwnerWnd,hDC);
					sFwa.iFrameShow=-1;
					sFwa.iFrameWrite=0;
					DMIWrite(&dmiAnimaz,iPt,&sFwa);

					for (a=0;a<2;a++)
					{
					 InitializeCriticalSection(&lpFwaDirect[iPt].csFrameLock[a]);
					}
					break;

				// ----------------------------------------------------
				// Servizio Animation
				//
				case FWA_ANIMATION:
					sFwa.rZone.bottom=sFwa.rZone.top+sFwa.sZone.cy-1;
					sFwa.rZone.right=sFwa.rZone.left+sFwa.sZone.cx-1;
					sFwa.hEvent[EANIM_BREAK]=CreateEvent(NULL,TRUE,FALSE,NULL); // 
					sFwa.hEvent[EANIM_ANALISE]=CreateEvent(NULL,TRUE,FALSE,NULL); // 
					sFwa.iMill=INFINITE;  
					if (sFwa.fRun&&sFwa.iFrame) sFwa.iMill=1000/sFwa.iFrame;
					//if (!sFwa.iFrame) sFwa.iMill=INFINITE; else sFwa.iMill=1000/sFwa.iFrame;
					DMIWrite(&dmiAnimaz,iPt,&sFwa);

					// Crea il nuovo thread di gestione dell'animazione
					// Creo il Thread per l'elaborazione
					hThread = CreateThread(NULL, 
											0, 
											ThreadAnimationDirect, 
											(LPDWORD) iPt,
											0, 
											&dwThread);

					// Setto la priorità del Thread
//					SetThreadPriority(hThread,THREAD_PRIORITY_NORMAL);
					SetThreadPriority(hThread,THREAD_PRIORITY_LOWEST);

					DMIRead(&dmiAnimaz,iPt,&sFwa);
					sFwa.hThread=hThread;
					sFwa.dwThread=dwThread;
					DMIWrite(&dmiAnimaz,iPt,&sFwa);
					break;

				// ----------------------------------------------------
				// Servizio Animation Child
				//
				case FWA_ANICHILD:

					DMIRead(&dmiAnimaz,(SINT) sFwa.hOwnerWnd,&sFwaB);
					//sFwa.hDCTarget=NULL;//sFwaB.hDC; // Assegno come target il DC del "FWA padre"

					sFwa.rZone.bottom=sFwa.rZone.top+sFwa.sZone.cy;
					sFwa.rZone.right=sFwa.rZone.left+sFwa.sZone.cx;
					sFwa.hEvent[EANIM_BREAK]=CreateEvent(NULL,TRUE,FALSE,NULL); // 
					sFwa.hEvent[EANIM_ANALISE]=CreateEvent(NULL,TRUE,FALSE,NULL); // 
					if (!sFwa.iFrame) sFwa.iMill=INFINITE; else sFwa.iMill=1000/sFwa.iFrame;
					DMIWrite(&dmiAnimaz,iPt,&sFwa);

					// Crea il nuovo thread di gestione dell'animazione
					// Creo il Thread per l'elaborazione
					hThread = CreateThread(NULL, 
											0, 
											ThreadAnimationChild, 
											(LPDWORD) iPt,
											0, 
											&dwThread);

					// Setto la priorità del Thread
					SetThreadPriority(hThread,THREAD_PRIORITY_NORMAL);
//					SetThreadPriority(hThread,THREAD_PRIORITY_HIGHEST);

					DMIRead(&dmiAnimaz,iPt,&sFwa);
					sFwa.hThread=hThread;
					sFwa.dwThread=dwThread;
					DMIWrite(&dmiAnimaz,iPt,&sFwa);
					break;
			}

			iNum=iPt;
			return &iNum;

		// ------------------------------------------------
		// WS_PROCESS
		// Richieste varie
		// info  = ID servizio FWA
		//
		case WS_PROCESS: 
			DMIRead(&dmiAnimaz,info,&sFwa); 
			switch (sFwa.iType)
			{
				case FWA_VIRTUALDC:
					lprZone=lpt;
					rZone.left=sFwa.rZone.left+lprZone->left;
					rZone.top=sFwa.rZone.top+lprZone->top;
					rZone.right=sFwa.rZone.left+lprZone->right;
					rZone.bottom=sFwa.rZone.top+lprZone->bottom;
					InvalidateRect(sFwa.hOwnerWnd,&rZone,FALSE);
					break;

				case FWA_ANIMATION:
					lprZone=lpt;
					InvalidateRect(sFwa.hOwnerWnd,lprZone,FALSE);
					break;

				case FWA_ANICHILD:
					break;
			}
			break;

		// ------------------------------------------------
		// WS_LINK
		// Accesso/Rilascio di scrittura in frame Virtuale
		// info  = ID servizio FWA
		//
		case WS_LINK:
			DMIRead(&dmiAnimaz,info,&sFwa); 
			switch (sFwa.iType)
			{
				case FWA_VIRTUALDC:
					
					// -------------------------------
					// Richiesta di scrittura Frame
					//
					if (lpt) 
					{
						// A) Richiedo accesso esclusivo all'animazione
						
						// Seleziono il Frame che userò per la scrittura
						if (lpFwaDirect[info].iFrameShow) lpFwaDirect[info].iFrameWrite=0; 
														  else 
														  lpFwaDirect[info].iFrameWrite=1;

						// B) Mi riservo l'uso del DC in Scrittura
						LocalFrameLock(info,lpFwaDirect[info].iFrameWrite);
						memcpy(lpt,lpFwaDirect+info,sizeof(sFwa));
						
						// C) Rilascio accesso esclusivo all'animazione
					}
					else
					// -------------------------------
					// Rilascio DC
					//
					{
						// A) Richiedo accesso esclusivo all'animazione ???
						// Prima di rilasciarlo devo aggiornarlo con tutte le animazioni figlio
						for (a=info;a<dmiAnimaz.Num;a++)
						{
							DMIRead(&dmiAnimaz,a,&sFwaB); 
							if (sFwaB.iType==FWA_ANICHILD&&sFwaB.hOwnerWnd==(HWND) info)
							{
								if (sFwaB.prcSub&&sFwaB.fShow) 
								{	// Il target è il frame di cui sto completando la scrittura
									// Chiamo la sotto procedura che dovrà stampare il risultato
									(sFwaB.prcSub)(WS_DISPLAY,0,&sFwaB,lpFwaDirect[info].hDCFrame[lpFwaDirect[info].iFrameWrite]);
								}
							}
						}

						lpFwaDirect[info].iFrameShow=lpFwaDirect[info].iFrameWrite;
						
						// C) Rilascio accesso esclusivo all'animazione
						LocalFrameUnlock(info,lpFwaDirect[info].iFrameWrite);
					}
					break;
				
				default:
					break;
			}
			break;

		// ---------------------------------------------------------
		// Richiesta trasferimento Rendering
		// di solito richiesta da Windows
		// 
		// 
		// info=HWND 
		// lpt=(PAINTSTRUCT *)
		// se lpt->hdc==0 chiede il device contest di HWND (info)
		// ---------------------------------------------------------

		case WS_DO: 

			lps=lpt;
			hdcLocal=lps->hdc;
			// *** POINT DC ***
//			if (lps->hdc==0) hdcLocal=GetDCEx((HWND) info,NULL,DCX_CACHE);

			if (lps->hdc==0) hdcLocal=GetDC((HWND) info);
			for (a=0;a<dmiAnimaz.Num;a++)
			{
				DMIRead(&dmiAnimaz,a,&sFwa); 
				if (sFwa.hOwnerWnd!=(HWND) info) continue; // hWnd diverso dal richiesto, vado via !
				//if (!sFwa.fRun) continue;
				if ((sFwa.rZone.top>lps->rcPaint.bottom)||
					(sFwa.rZone.bottom<lps->rcPaint.top)||
					(sFwa.rZone.left>lps->rcPaint.right)||
					(sFwa.rZone.right<lps->rcPaint.left))  continue;

				switch (sFwa.iType)
				{
					//
					// Virtual DC
					// Trasferisco dall'area Virtuale (sFwa.hDC[?]) nel Device Context Reale (hdcLocal)
					//
					case FWA_VIRTUALDC:
						
						// Definisco l'esatta zona da copiare
						memcpy(&rZone,&lps->rcPaint,sizeof(RECT));
						rZone.left=(rZone.left<sFwa.rZone.left)?sFwa.rZone.left:rZone.left;
						rZone.right=(rZone.right>sFwa.rZone.right)?sFwa.rZone.right:rZone.right;
						rZone.top=(rZone.top<sFwa.rZone.top)?sFwa.rZone.top:rZone.top;
						rZone.bottom=(rZone.bottom>sFwa.rZone.bottom)?sFwa.rZone.bottom:rZone.bottom;

						iFrameShow=lpFwaDirect[a].iFrameShow;
						// A) Blocco il cambio di Frame DC
						LocalFrameLock(a,iFrameShow);
						
						// B) Trasferisco il BitMap
						// Trasferisco il bitmap
						if (iFrameShow>-1)
						{
						 BitBlt(hdcLocal,  // --- DESTINAZIONE ---
							   rZone.left, rZone.top, // Coordinate X-Y
							   rZone.right-rZone.left+1, // Larghezza
							   rZone.bottom-rZone.top+1, //Altezza
 							   lpFwaDirect[a].hDCFrame[iFrameShow], // --- SORGENTE ---
							   rZone.left-sFwa.rZone.left,
							   rZone.top-sFwa.rZone.top, //Cordinate x-y
							   SRCCOPY);
						}
						// C) Sblocco il cambio di Frame DC
						LocalFrameUnlock(a,iFrameShow);

						break;

					// Se rientrano nel range chiedo di stampare il GIF
					case FWA_ANIMATION: 
						if (sFwa.prcSub&&sFwa.fShow&&!sFwa.fRun) {(sFwa.prcSub)(WS_DISPLAY,0,&sFwa,hdcLocal);}
						break;
				}
			}
			if (lps->hdc==0) ReleaseDC((HWND) info,hdcLocal);
			break;
	}
	return NULL;
}

// -----------------------------------------------------------
// Thread per animazione Direct DC
//
static DWORD WINAPI ThreadAnimationDirect(LPVOID Dato)
{
  DWORD dwWait;
  SINT iNum=(SINT) Dato;
  FWANIM sFwa;
  SINT iCount;
  SINT iCount2=0;

  DMIRead(&dmiAnimaz,iNum,&sFwa);
  while (TRUE)
  {
	 dwWait=WaitForMultipleObjects(2,sFwa.hEvent,FALSE,sFwa.iMill);
//	 if (iNum==3) 
	 // Chiusura del thread
	 if (dwWait==WAIT_OBJECT_0+EANIM_BREAK) break;

	 // Analizza
	 if (dwWait==WAIT_OBJECT_0+EANIM_ANALISE) 
	 {
		EnterCriticalSection(&csAnimation);
		iCount=sFwa.iCount;
		DMIRead(&dmiAnimaz,iNum,&sFwa); // Count -1  ? = Non toccarlo
		if (sFwa.iCount==-1) sFwa.iCount=iCount;
		LeaveCriticalSection(&csAnimation);
		ResetEvent(sFwa.hEvent[EANIM_ANALISE]); 
		if (sFwa.fShow) dwWait=WAIT_TIMEOUT; // Faccio comunque un refresh
		lpFwaDirect[iNum].lProcess=FALSE;
	 }

	 // Chiede il Cambio di frame
	 if (dwWait==WAIT_TIMEOUT)
	 {
		if (sFwa.prcSub&&sFwa.fShow) 
		{
			// *** POINT DC ***
			// Commentato momentaneamente
			HDC hdcTarget=GetDCEx(sFwa.hOwnerWnd,NULL,DCX_CACHE); // DA VEDERE
//			sFwa.hDCTarget=GetDC(sFwa.hOwnerWnd); // DA VEDERE
			if (hdcTarget!=NULL)
			{
				(sFwa.prcSub)(WS_DISPLAY,1,&sFwa,hdcTarget);
				ReleaseDC(sFwa.hOwnerWnd,hdcTarget);
			}
		}
		
		// Pulisco la zona
		if (!sFwa.fShow) InvalidateRect(sFwa.hOwnerWnd,&sFwa.rZone,FALSE);
	 }

  }
   return 0;
}

// -----------------------------------------------------------
// Thread per animazione FWA Child 
//
static DWORD WINAPI ThreadAnimationChild(LPVOID Dato)
{
  DWORD dwWait;
  SINT iNum=(SINT) Dato;
  SINT iFuther;
  FWANIM *lpsFwa;
  FWANIM *lpsFwaFurther;
  SINT iFrameShow;
//  SINT iCount;
//  SINT iCount2=0;
  RECT rZone;
  //DMIRead(&dmiAnimaz,iNum,&sFwa);
  lpsFwa=lpFwaDirect+iNum;
  iFuther=(SINT) lpsFwa->hOwnerWnd;
  lpsFwaFurther=lpFwaDirect+iFuther;
  //DMIRead(&dmiAnimaz,(SINT) lpsFwa->hOwnerWnd,&sFwaFurther);

  while (TRUE)
  {
//	 dwWait=WaitForMultipleObjects(2,sFwa.hEvent,FALSE,sFwa.iMill);
	 dwWait=WaitForMultipleObjects(2,lpsFwa->hEvent,FALSE,lpsFwa->iMill);
	 // Chiusura del trhead
	 if (dwWait==WAIT_OBJECT_0+EANIM_BREAK) break;

	 // Chiede il Cambio di frame
	 if (dwWait==WAIT_TIMEOUT)
	 {
		if (lpsFwa->prcSub&&lpsFwa->fShow) 
		{
			//if (iNum==3) 
			//dispx("%d) [%s] %d   %d",iNum,lpsFwa->lpName,lpsFwa->iCount,lpsFwa->fShow);
			//LocalDC_Lock(iNum);

			// B) Chiedo refresh del DC
			if (lpsFwaFurther->iFrameShow!=-1) 
			{
				// A) Chiedo accesso esclusivo al DCVirtuale
				iFrameShow=lpsFwaFurther->iFrameShow;
				LocalFrameLock(iFuther,iFrameShow);
				
				//dispx("%d) [%s] %d   %d",iNum,lpsFwa->lpName,lpsFwa->iCount,lpsFwa->fShow);
				//if (lpsFwaFurther->iFrameShow) dispx("%d      ",lpsFwaFurther->iFrameShow);
				(lpsFwa->prcSub)(WS_DISPLAY,1,lpsFwa,lpsFwaFurther->hDCFrame[iFrameShow]);

				// C) Rilascio accesso esclusivo
				LocalFrameUnlock(iFuther,iFrameShow);

				// D) Richiedo refresh della zona modificata

				rZone.left=lpsFwaFurther->rZone.left+lpsFwa->rZone.left;
				rZone.top=lpsFwaFurther->rZone.top+lpsFwa->rZone.top;
				rZone.right=lpsFwaFurther->rZone.left+lpsFwa->rZone.right-1;
				rZone.bottom=lpsFwaFurther->rZone.top+lpsFwa->rZone.bottom-1;
				InvalidateRect(lpsFwaFurther->hOwnerWnd,&rZone,FALSE);
			}
		}
	 }

	 // Chiusura del trhead
	 if (dwWait==WAIT_OBJECT_0+EANIM_ANALISE) 
	 {
		//EnterCriticalSection(&csAnimation);
		//iCount=lpsFwa->iCount;
		//DMIRead(&dmiAnimaz,iNum,&sFwa); // Count -1  ? = Non toccarlo
		//if (sFwa.iCount==-1) sFwa.iCount=iCount;
		//LeaveCriticalSection(&csAnimation);
		//if (iNum==3) dispx("Dato %d/%d         ",sFwa.lParam,iCount2++);
		ResetEvent(lpsFwa->hEvent[EANIM_ANALISE]); 
	 }
  }
   return 0;
}


// --------------------------------------------
// Gestore di GIF che sfrutta WAnimation
//
static void IMGDispDC(HDC hdc,SINT PosX,SINT PosY,SINT Hdl);
typedef struct {
  GIFINFO GIFInfo;
  CRITICAL_SECTION cs;
} MT_GIFINFO;

void *GIFAnimation(SINT cmd,LONG info,void *lpPtr,HDC hdcTarget) //      CHIAMATA ALLA ROUTINE
{
	FWANIM *lpFwa=lpPtr;
	MT_GIFINFO MT_GIFInfo;
	static DRVMEMOINFO dmiGifs=DMIRESET;
	static MT_GIFINFO *lpMT_GIFInfo;
	static SINT iNum;
	SINT Hdl;
	SINT a;
	BOOL fCheck;

	switch (cmd)
	{
		case WS_OPEN: // Apro un file FLC
			DMIOpen(&dmiGifs,RAM_AUTO,info,sizeof(MT_GIFInfo),"*WANIGIF");
			lpMT_GIFInfo=Wmemo_lock(dmiGifs.Hdl);
			break;
		
		case WS_CLOSE:
			for (a=0;a<dmiGifs.Num;a++)
			{
				EnterCriticalSection(&lpMT_GIFInfo[a].cs);
				GIFCloseFile(&lpMT_GIFInfo[a].GIFInfo); // Recupero le risorse impegnate
				ZeroFill(lpMT_GIFInfo[a].GIFInfo);
				LeaveCriticalSection(&lpMT_GIFInfo[a].cs);
			    DeleteCriticalSection(&lpMT_GIFInfo[a].cs);
			}
			DMIClose(&dmiGifs,"*WANIGIF");
			break;

		case WS_ADD:
			//if (GIFOpenFile((CHAR *) lpPtr,&GIFInfo)) {win_infoarg(lpPtr);return NULL;}
			ZeroFill(MT_GIFInfo);
			if (!GIFOpenFile((CHAR *) lpPtr,&MT_GIFInfo.GIFInfo)) {win_infoarg(lpPtr);return NULL;}
			DMIAppend(&dmiGifs,&MT_GIFInfo);
			iNum=dmiGifs.Num-1;
			InitializeCriticalSection(&lpMT_GIFInfo[iNum].cs);
			return &iNum;

		case WS_PROCESS: // Sistema le dimensioni del size
			switch (info)
			{
				case 0: // Setta in CX le misure del gif
						EnterCriticalSection(&lpMT_GIFInfo[lpFwa->lParam].cs);
						lpFwa->sZone.cx=lpMT_GIFInfo[lpFwa->lParam].GIFInfo.sArea.cx;
						lpFwa->sZone.cy=lpMT_GIFInfo[lpFwa->lParam].GIFInfo.sArea.cy;
						LeaveCriticalSection(&lpMT_GIFInfo[lpFwa->lParam].cs);
						break;
			}
			break;

		case WS_DISPLAY:
			//DMIRead(&dmiGifs,lpFwa->lParam,&GIFInfo);
			// Se non deve essere visualizzato, mi fermo
			if (!lpFwa->fShow) break;
			
			EnterCriticalSection(&lpMT_GIFInfo[lpFwa->lParam].cs);
			fCheck=GIFtoIMG(
					//&GIFInfo,			// Struttura GIF
					&lpMT_GIFInfo[lpFwa->lParam].GIFInfo,
					lpFwa->iCount,
					lpFwa->iCount,					// Fino al Frame
			 		lpFwa->cBackGround,
					&Hdl);
			
			// se info=1 indica di andare avanti di un frame
			if (info&lpFwa->fRun) {lpFwa->iCount++; if (lpFwa->iCount>=lpMT_GIFInfo[lpFwa->lParam].GIFInfo.dmiFrames.Num) lpFwa->iCount=0;}
			LeaveCriticalSection(&lpMT_GIFInfo[lpFwa->lParam].cs);
			
			if (!fCheck)
			{  
			  if (hdcTarget) IMGDispDC(hdcTarget,lpFwa->rZone.left,lpFwa->rZone.top,Hdl);
			  memo_libera(Hdl,"Image");
			}
			break;
		}
	return NULL;
}

static void IMGDispDC(HDC hdcDest,SINT PosX,SINT PosY,SINT Hdl)
{
 IMGHEADER *Img;
 HDC hdcSorg;
 HBITMAP BitMap;
 BITMAPINFOHEADER BmpHeaderBackup;
 BITMAPINFOHEADER *BmpHeader;
 BITMAPINFO *BmpInfo;
 BYTE *Sorg;
 LONG Lx,Ly;

 Img=Wmemo_lock(Hdl);
 Sorg=(BYTE *) Img;
 Sorg+=Img->Offset;

 // -------------------------------------
 // DATI DELL'ICONE                     
 // -------------------------------------
 BmpInfo=(BITMAPINFO *) &Img->bmiHeader;
 BmpHeader=(BITMAPINFOHEADER *) &Img->bmiHeader;
 memcpy(&BmpHeaderBackup,BmpHeader,sizeof(BmpHeaderBackup));

 // -------------------------------------
 // CREA ED STAMPA IL BITMAP            
 // -------------------------------------
 Ly=BmpHeader->biHeight;
 Lx=BmpHeader->biWidth;
 
 BmpHeader->biHeight=-Ly;
 BmpHeader->biWidth=Lx;
 BitMap=CreateDIBitmap(hdcDest, //Handle del contesto
					   BmpHeader,
					   CBM_INIT,
					   Sorg,// Dati del'icone
					   BmpInfo,
					   DIB_RGB_COLORS);
 
 hdcSorg=CreateCompatibleDC(hdcDest);
 SelectObject(hdcSorg, BitMap);
 
 BitBlt(hdcDest,  // --- DESTINAZIONE ---
		PosX, PosY, // Coordinate X-Y
		Lx, // Larghezza
		Ly, //Altezza
		hdcSorg, // --- SORGENTE ---
		0, 0, //Cordinate x-y
		SRCCOPY);

 DeleteDC(hdcSorg);
 DeleteObject(BitMap);
// ReleaseDC(hWnd,hdcDest); 
 memcpy(BmpHeader,&BmpHeaderBackup,sizeof(BmpHeaderBackup));
 Wmemo_unlock(Hdl);
}
