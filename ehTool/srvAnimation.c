//   ---------------------------------------------------------
//   | WAnimation                                
//   | Gestore di servizi di animazione
//   | Windows 32bit Only
//   |                                           
//   | Tipo                                           
//   | AT_CONTAINER                                          
//   | Contesto "clone" di dimensione stabilita collegato                                          
//   | ad una finestra proprietaria.                                          
//   | Permette di "costruire" in un'area "clone"
//   | prima di spedirlo in video.
//   | Crea un DC compatibile dove è possibile scrivere                                          
//   | con istruzione EH e non                                          
//   |                                           
//   | Tipo                                           
//   | AT_ANIMATION
//   | Contesto multi-thread collegato ad una finestra proprietaria.                                          
//   | Crea un Thread con possibilità di definire frame per sec
//   | che invoca direttamente una subProcedure per la stampa
//   | Può essere abbinato alla sub GIFAnimation() per visualizzare
//   | le Gif animate.
//   |                                           
//   |							Created by Tassistro 05/2001  
//   ---------------------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/srvAnimation.h"
#include "/easyhand/ehtool/imgutil.h"

static struct {
	
	BOOL		bReset;
	HANDLE		mtxAnim;	// CRITICAL_SECTION csAnimation;
	_DMI		dmiAnim;
	S_FWANIM *	arsAnim;

	//HANDLE		arThreadAni[2];		// Handle del thread (Interno non toccare)
	HANDLE		thrdAnimation;
	DWORD		dwThreadAnimation;//arwThread[2];		// 
	HANDLE		arhEventDir[2];	// Eventi usati per segnalare azioni (Interno non toccare)
//	HANDLE		arhEventChild[2];		// Eventi usati per segnalare azioni (Interno non toccare)

} _l={true};


static DWORD WINAPI _thrAnimation(LPVOID Dato);
static void _displayRefresh(S_FWANIM * psFwa,RECT * psRect);
// static DWORD WINAPI _thrAnimationChild(LPVOID Dato);

//static DRVMEMOINFO _l.dmiAnim=DMIRESET;
//static S_FWANIM * _l.arsAnim;
//static HANDLE _l.arThreadAni[2];   // Handle del thread (Interno non toccare)
//static DWORD  _l.arwThread[2];  // 
//static HANDLE  _l.arhEventDir[2]; // Eventi usati per segnalare azioni (Interno non toccare)
//static HANDLE  _l.arhEventChild[2]; // Eventi usati per segnalare azioni (Interno non toccare)

#define EANIM_BREAK 0
//#define EANIM_ANALISE 1


// ---------------------------------------------------------------------
// Gestisce la sincronizzazione di accesso alle aree Frame Virtuali
// ---------------------------------------------------------------------

static void _frameLock(INT iAnim,INT iFrame)
{
	if (iFrame==-1) return;
	//EnterCriticalSection(&_l.arsAnim[iAnim].csFrameLock[iFrame]);
	WaitForSingleObject(_l.arsAnim[iAnim].mtxLock[iFrame%2],INFINITE);  // no time-out interval
}

// ---------------------------------------------------------------------
// Gestisce la sincronizzazione di accesso alle aree Frame Virtuali
// ---------------------------------------------------------------------
static void _frameUnlock(INT iAnim,INT iFrame)
{
	if (iFrame<0) return;
//	LeaveCriticalSection(&_l.arsAnim[iAnim].csFrameLock[iFrame]);
	if (!ReleaseMutex(_l.arsAnim[iAnim].mtxLock[iFrame%2])) {ehPrintf("_frameUnlock mutex error"); exit(128);}
}

//
// srvAnimation()
//
void * srvAnimation(EH_SRVPARAMS)
{
	S_FWANIM sFwa;
	S_FWANIM *	psAni;
	S_FW_DISPLAY * psFwd;
	INT a;
	INT iPt;
	HDC hDC;
	HDC hdcLocal;
	RECT rZone;
	INT iFrameShow;

	static INT iNum;
	switch (enMess)
	{

		//
		// Apro il server delle animazioni
		// Lavora con due thread
		//
		case WS_OPEN:	

			if (_l.bReset) {_(_l); DMIReset(&_l.dmiAnim);}
			DMIOpen(&_l.dmiAnim,RAM_AUTO,lParam,sizeof(S_FWANIM),"*WANIMAZ");
			_l.arsAnim=DMILock(&_l.dmiAnim,NULL);
			//InitializeCriticalSection(&csAnimation);
			_l.mtxAnim=CreateMutex(NULL,false,NULL);
			_l.arhEventDir[EANIM_BREAK]=CreateEvent(NULL,TRUE,FALSE,NULL); // 
			// Crea il nuovo thread di gestione dell'animazione DIRETTE
			// Creo il Thread per l'elaborazione
			_l.thrdAnimation = CreateThread(NULL, 
											 0, 
											 _thrAnimation, 
											 0,
											 0, 
											 &_l.dwThreadAnimation);

			// Setto la priorità del Thread
			SetThreadPriority(_l.thrdAnimation,THREAD_PRIORITY_HIGHEST);

			_(sFwa);
			DMIAppend(&_l.dmiAnim,&sFwa); // Appendo un elemento vuoto per non avere idParent=0
			//SetThreadPriority(_l.arThreadAni[0],THREAD_PRIORITY_LOWEST);

			// Crea il nuovo thread di gestione dell'animazione CHILD
			// Creo il Thread per l'elaborazione
			/*
			_l.arThreadAni[1] = CreateThread(NULL, 
											 0, 
											 _thrAnimationChild, 
											 0,
											 0, 
											 &_l.arwThread[1]);

			// Setto la priorità del Thread
//					SetThreadPriority(hThread,THREAD_PRIORITY_HIGHEST);
			SetThreadPriority(_l.arThreadAni[1],THREAD_PRIORITY_HIGHEST);
			*/
			//SetThreadPriority(_l.arThreadAni[1],THREAD_PRIORITY_LOWEST);
			break;

		case WS_CLOSE:	
			
			if (_l.bReset) break;
			if (!_l.arhEventDir[EANIM_BREAK]) ehError();
			// Chiudo il gestore delle animazioni
			WaitForSingleObject(_l.mtxAnim,INFINITE);  // EnterCriticalSection(&csAnimation);

			// Prima chiudo i Thread
			SetEvent(_l.arhEventDir[EANIM_BREAK]);
			// Termino il Thread se non risponde
			if (WaitForSingleObject(_l.thrdAnimation,5000)==WAIT_TIMEOUT) TerminateThread(_l.thrdAnimation,1);
			CloseHandle(_l.arhEventDir[EANIM_BREAK]);
			_l.arhEventDir[EANIM_BREAK]=NULL;
//			CloseHandle(_l.arhEventDir[EANIM_ANALISE]);

/*
			// Termino il Thread se non risponde
			SetEvent(_l.arhEventChild[EANIM_BREAK]);
			if (WaitForSingleObject(_l.arThreadAni[1],5000)==WAIT_TIMEOUT) TerminateThread(_l.arThreadAni[1],1);
			CloseHandle(_l.arhEventChild[EANIM_BREAK]);
			CloseHandle(_l.arhEventChild[EANIM_ANALISE]);
*/			
			// Poi chiudo i VirtualDC
			for (a=0;a<_l.dmiAnim.Num;a++)
			{
				DMIRead(&_l.dmiAnim,a,&sFwa);
				switch (sFwa.enType)
				{
					case AT_CONTAINER:
						DeleteObject(sFwa.hBitmapFrame[0]);
						DeleteObject(sFwa.hBitmapFrame[1]);
						DeleteDC(sFwa.dcFrame[0]);
						DeleteDC(sFwa.dcFrame[1]);
						CloseHandle(sFwa.mtxLock[0]); sFwa.mtxLock[0]=NULL;
						CloseHandle(sFwa.mtxLock[1]); sFwa.mtxLock[1]=NULL;
						//DeleteCriticalSection(&_l.arsAnim[a].csFrameLock[0]);
						//DeleteCriticalSection(&_l.arsAnim[a].csFrameLock[1]);
						break;
				}
			}

			ReleaseMutex(_l.mtxAnim);//LeaveCriticalSection(&csAnimation);
			CloseHandle(_l.mtxAnim); 
			_l.mtxAnim=NULL; //DeleteCriticalSection(&csAnimation);

			DMIClose(&_l.dmiAnim,"*WANIMAZ");
			_l.bReset=true;
			break;
		
		case WS_REALGET:
			DMIRead(&_l.dmiAnim,lParam,pVoid);
			break;

		case WS_REALSET:

			WaitForSingleObject(_l.mtxAnim,INFINITE);  //EnterCriticalSection(&csAnimation);
			psAni=pVoid;
//			DMIRead(&_l.dmiAnim,lParam,&sFwa);

			// Auto sistemo il rettangolo
			psAni->rZone.bottom=psAni->rZone.top+psAni->sZone.cy-1;
			psAni->rZone.right=psAni->rZone.left+psAni->sZone.cx-1;
			if (psAni->iCount<0&&psAni->idParent) psAni->iCount=_l.arsAnim[lParam].iCount;
			switch (psAni->enType)
			{
				case AT_CONTAINER: break;

				case AT_ANIMATION: 
//				case FWA_ANICHILD:
							if (psAni->bRun) {
//								if (!psAni->iFrame) sFwa.iMill=INFINITE; else psAni->iMill=1000/psAni->iFrame;
								// 24:1=x:1000
								if (!psAni->iFrame) psAni->iTimeFrame=INFINITE; else psAni->iTimeFrame=(1000/ANI_MILLIBASE)/psAni->iFrame;
							}
							else
							{
								psAni->iTimeFrame=INFINITE; 
							}
							break;
			}

			DMIWrite(&_l.dmiAnim,lParam,psAni);
			/*
			switch (lpFwa->enType)
			{
				case AT_CONTAINER: break;
				case AT_ANIMATION: 
							//srvAnimation(WS_PROCESS,sAmbient.hIDAnim[iGif],&sFwa2.rZone);
							// Chiedo un refresh della zona se non è più visibile
							//if (!lpFwa->fShow) InvalidateRect(lpFwa->hOwnerWnd,&lpFwa->rZone,FALSE);
							SetEvent(_l.arhEventDir[EANIM_ANALISE]);
							break;
				case FWA_ANICHILD:
							SetEvent(_l.arhEventChild[EANIM_ANALISE]);
							break;
			}
			*/
			ReleaseMutex(_l.mtxAnim);//LeaveCriticalSection(&csAnimation);
			break;

		case WS_ADD: 

			// --------------------------------------
			// Aggiunge servizio di animazione
			//
			DMIAppend(&_l.dmiAnim,pVoid);
			iPt=_l.dmiAnim.Num-1;
			// Creo eventi per il controllo del Thread
			DMIRead(&_l.dmiAnim,iPt,&sFwa);
			switch (sFwa.enType)
			{
				// 
				// Creo Container (virtual DC)
				// NOTA: Le dimensioni del container sono "fisse" e dichiarate in questo punto
				//
				case AT_CONTAINER:

					if (sFwa.psObj) _(sFwa.rZone);
					sFwa.rZone.bottom=sFwa.rZone.top+sFwa.sZone.cy-1;
					sFwa.rZone.right=sFwa.rZone.left+sFwa.sZone.cx-1;

					//
					// WINDOWS
					// Creo due copie del DC da usare per VirtualDC
					// 
					hDC=GetDC(sFwa.hWindow); // in caso di psObj, sFwa.hWindow deve essere NULL (=desktop)
					for (a=0;a<2;a++)
					{
						 sFwa.dcFrame[a]=CreateCompatibleDC(hDC);
						 sFwa.hBitmapFrame[a]=CreateCompatibleBitmap(hDC,sFwa.sZone.cx,sFwa.sZone.cy);
						 SelectObject(sFwa.dcFrame[a],sFwa.hBitmapFrame[a]);
						 SetMapMode(sFwa.dcFrame[a], MM_TEXT);
						 SetBkMode(sFwa.dcFrame[a],OPAQUE);
						 SetTextColor(sFwa.dcFrame[a],0);
						 dcBoxp(sFwa.dcFrame[a],&sFwa.rZone,sFwa.cBackGround);
					}
					ReleaseDC(sFwa.hWindow,hDC);
					sFwa.iFrameShow=-1;
					sFwa.iFrameWrite=0;
					DMIWrite(&_l.dmiAnim,iPt,&sFwa);

					_l.arsAnim[iPt].mtxLock[0]=CreateMutex(NULL,FALSE,NULL);
					_l.arsAnim[iPt].mtxLock[1]=CreateMutex(NULL,FALSE,NULL);
/*
					for (a=0;a<2;a++)
					{
						InitializeCriticalSection(&_l.arsAnim[iPt].csFrameLock[a]);
					}
					*/
					break;

				// ----------------------------------------------------
				// Servizio Animation
				//
				case AT_ANIMATION:

//					if (!sFwa.idParent) {
						sFwa.rZone.bottom=sFwa.rZone.top+sFwa.sZone.cy-1;
						sFwa.rZone.right=sFwa.rZone.left+sFwa.sZone.cx-1;
						sFwa.iTimeCount=0;
						sFwa.iTimeFrame=INFINITE;  
						if (sFwa.iFrame) sFwa.iTimeFrame=(1000/ANI_MILLIBASE)/sFwa.iFrame;
						DMIWrite(&_l.dmiAnim,iPt,&sFwa);
//					}
			/*
					else { // CHILD
					
//						DMIRead(&_l.dmiAnim,(INT) sFwa.hOwnerWnd,&sFwaB);
						sFwa.rZone.bottom=sFwa.rZone.top+sFwa.sZone.cy-1;
						sFwa.rZone.right=sFwa.rZone.left+sFwa.sZone.cx-1;
						if (sFwa.iFrame) sFwa.iTimeFrame=(1000/ANI_MILLIBASE)/sFwa.iFrame;
//						DMIWrite(&_l.dmiAnim,iPt,&sFwa);
					}
*/
					break;
			}

			iNum=iPt;
			return &iNum;

		// ------------------------------------------------
		// WS_PROCESS
		// Richieste varie
		// lParam  = ID servizio FWA
		//
		case WS_PROCESS: 

			DMIRead(&_l.dmiAnim,lParam,&sFwa); 
			_displayRefresh(&sFwa,NULL);
			/*
			switch (sFwa.enType)
			{
				case AT_CONTAINER:
					lprZone=pVoid;
					rZone.left=sFwa.rZone.left+lprZone->left;
					rZone.top=sFwa.rZone.top+lprZone->top;
					rZone.right=sFwa.rZone.left+lprZone->right;
					rZone.bottom=sFwa.rZone.top+lprZone->bottom;
					InvalidateRect(sFwa.hOwnerWnd,&rZone,FALSE);
					break;

				case AT_ANIMATION:
					lprZone=pVoid;
					InvalidateRect(sFwa.hOwnerWnd,lprZone,FALSE);
					break;

				case FWA_ANICHILD:
					break;
			}
			*/
			break;

		// 
		// WS_LINK
		// Accesso/Rilascio di scrittura esclusiva di un Contenitore DC
		// lParam  = id del Contenitore
		// pVoid = >NULL richiesto accesso in scrittura , =NULL Rilascio
		//
		case WS_LINK:

			psAni=_l.arsAnim+lParam;
//			DMIRead(&_l.dmiAnim,lParam,&sFwa); 
			if (psAni->enType!=AT_CONTAINER) ehError();
					
			// 
			// Richiesta di accesso in esclusiva
			//
			if (pVoid) 
			{
				//
				// A)	Richiedo accesso esclusivo all'animazione
				//		Seleziono il Frame che userò per la scrittura
				// 
				if (psAni->iFrameShow) 
						psAni->iFrameWrite=0; 
						else 
						psAni->iFrameWrite=1;

				// B) Mi riservo l'uso del DC in Scrittura
				_frameLock(lParam,psAni->iFrameWrite);
				memcpy(pVoid,psAni,sizeof(sFwa));
				
			}
			else
			// 
			// Rilascio DC
			//
			{
				// Prima di rilasciarlo richiedo l'aggiornamento di tutte le animazioni figlie
				for (a=lParam+1;a<_l.dmiAnim.Num;a++)
				{
					psAni=_l.arsAnim+a;
//					DMIRead(&_l.dmiAnim,a,&sFwaB); 
					//if (sFwaB.enType==FWA_ANICHILD&&sFwaB.hOwnerWnd==(HWND) lParam)
					if (psAni->idParent==lParam)
					{
						if (psAni->funcFrame&&psAni->bVisible) 
						{	// Il target è il frame di cui sto completando la scrittura
							// Chiamo la sotto procedura che dovrà stampare il risultato
							(psAni->funcFrame)(WS_DISPLAY,0,psAni,
								_l.arsAnim[lParam].dcFrame[_l.arsAnim[lParam].iFrameWrite]); // hdc del parente
						}
					}
				}

				_l.arsAnim[lParam].iFrameShow=_l.arsAnim[lParam].iFrameWrite;
				
				// C) Rilascio accesso esclusivo all'animazione
				_frameUnlock(lParam,_l.arsAnim[lParam].iFrameWrite);
			}
			break;

		// ---------------------------------------------------------
		// Richiesta trasferimento Rendering
		// di solito richiesta da Windows
		// 
		// 
		// lParam=HWND 
		// lpt=(PAINTSTRUCT *)
		// se lpt->hdc==0 chiede il device contest di HWND (lParam)
		// ---------------------------------------------------------

//		case WS_DO: 
		case WS_DO:
			ehError();

		//
		// srvAnimation(WS_DISPLAY .. 
		// Se è un container, trasferisco il contaner nel DC passato
		// 
		// può essere invocato d
		// - dal gestore degli oggetti (viene valorizzato psFwd->psObj)
		// - da un paint di una finestra (viene valorizzato psFwd->hWindow)
		//
		case WS_DISPLAY: 

			psFwd=pVoid;
			hdcLocal=psFwd->hdc;

			for (a=0;a<_l.dmiAnim.Num;a++)
			{
				DMIRead(&_l.dmiAnim,a,&sFwa); 
				if (psFwd->psObj&&sFwa.psObj!=psFwd->psObj) continue;
				if (psFwd->hWindow&&sFwa.hWindow!=psFwd->hWindow) continue;
// 				if (sFwa.hOwnerWnd!=(HWND) lParam) continue; // hWnd diverso dal richiesto, vado via !
				//if (!sFwa.fRun) continue;

				if ((sFwa.rZone.top>psFwd->rcArea.bottom)||
					(sFwa.rZone.bottom<psFwd->rcArea.top)||
					(sFwa.rZone.left>psFwd->rcArea.right)||
					(sFwa.rZone.right<psFwd->rcArea.left)) { 
						continue;
				}

				switch (sFwa.enType)
				{
					//
					// Virtual DC
					// Trasferisco dall'area Virtuale (sFwa.hDC[?]) nel Device Context Reale (hdcLocal)
					//
					case AT_CONTAINER:
						
						//
						// Definisco l'esatta zona da copiare (richiesta)
						// Attenzione: per l'oggetto la posizione di destinazione non puà essere assoluta
						// 
						memcpy(&rZone,&psFwd->rcArea,sizeof(RECT));
						rZone.left=(rZone.left<sFwa.rZone.left)?sFwa.rZone.left:rZone.left;
						rZone.right=(rZone.right>sFwa.rZone.right)?sFwa.rZone.right:rZone.right;
						rZone.top=(rZone.top<sFwa.rZone.top)?sFwa.rZone.top:rZone.top;
						rZone.bottom=(rZone.bottom>sFwa.rZone.bottom)?sFwa.rZone.bottom:rZone.bottom;

						iFrameShow=_l.arsAnim[a].iFrameShow;
						// A) Blocco il cambio di Frame DC
						_frameLock(a,iFrameShow);
						
						// B) Trasferisco il BitMap
						// Trasferisco il bitmap
						if (iFrameShow>-1)
						{
							 BitBlt(hdcLocal,  // --- DESTINAZIONE ---
								   rZone.left, rZone.top, // Coordinate X-Y
								   rZone.right-rZone.left+1, // Larghezza
								   rZone.bottom-rZone.top+1, //Altezza
 								   _l.arsAnim[a].dcFrame[iFrameShow], // --- SORGENTE ---
								   rZone.left-sFwa.rZone.left,
								   rZone.top-sFwa.rZone.top, //Cordinate x-y
								   SRCCOPY);
						}
						// C) Sblocco il cambio di Frame DC
						_frameUnlock(a,iFrameShow);

						break;

					//
					// Se rientrano nel range chiedo di stampare il GIF
					//
					case AT_ANIMATION: 
						if (sFwa.funcFrame&&sFwa.bVisible&&!sFwa.bRun) {(sFwa.funcFrame)(WS_DISPLAY,0,&sFwa,hdcLocal);}
						break;
				}
			}
//			if (lps->hdc==0) ReleaseDC((HWND) lParam,hdcLocal);
			break;
	}
	return NULL;
}

// -----------------------------------------------------------
// Thread per animazione Direct DC
// Controlla le animazioni Direct
//
static DWORD WINAPI _thrAnimation(LPVOID Dato)
{
	DWORD dwWait;
	// HDC hdcTarget;
	INT a;
	S_FWANIM * psAni, * psAniParent;

	printf("_thrAnimation start .." CRLF);
	while (true)
	{
		dwWait=WaitForMultipleObjects(1,_l.arhEventDir,FALSE,ANI_MILLIBASE); // 50simi di secondo (20 frame)
		if (dwWait==WAIT_OBJECT_0+EANIM_BREAK) break; // Chiusura del thread

		// Incremento i frame
		if (dwWait==WAIT_TIMEOUT)
		{
			// Loop sulle animazioni per aggiornare
			for (a=0;a<_l.dmiAnim.Num;a++) {

				psAni=_l.arsAnim+a; if (psAni->enType!=AT_ANIMATION) continue;
				if (!psAni->bVisible||!psAni->bRun) continue;  // Se non è visible e non è in movimento salto

				psAni->iTimeCount++;
				if (psAni->iTimeCount>=psAni->iTimeFrame) {

					psAni->iTimeCount=0;
					/*

					if (psAni->funcFrame)		// Ho una Procedura associata
					{
					// *** POINT DC ***
					hdcTarget=GetDCEx(psAni->hOwnerWnd,NULL,DCX_CACHE); // DA VEDERE
					if (hdcTarget!=NULL)
					{
					(psAni->funcFrame)(WS_DISPLAY,1,&_l.arsAnim[a],hdcTarget);
					ReleaseDC(psAni->hOwnerWnd,hdcTarget);
					}
					// Refresh della zona
					//if (!psAni->fShow) 
					//		InvalidateRect(psAni->hOwnerWnd,&psAni->rZone,FALSE);
					}
					*/

					if (psAni->idParent) {

						if (!psAni->funcFrame) continue;

						psAniParent=_l.arsAnim+psAni->idParent;
						// B) Chiedo refresh del DC
						if (psAniParent->iFrameShow!=-1) {

							// A) Chiedo accesso esclusivo al DCVirtuale
							INT iFrameShow=psAniParent->iFrameShow;
							_frameLock(psAni->idParent,iFrameShow);

							// ???
							(psAni->funcFrame)(WS_DISPLAY,1,psAni,psAniParent->dcFrame[iFrameShow]);

							// C) Rilascio accesso esclusivo
							_frameUnlock(psAni->idParent,iFrameShow);

							// D) Richiedo refresh della zona padre modificata
							//_displayRefresh(psAniParent,NULL);
							psAniParent->bDcRedraw=true;

							/*
							rZone.left=lpsFwaFurther->rZone.left+psAni->rZone.left;
							rZone.top=lpsFwaFurther->rZone.top+psAni->rZone.top;
							rZone.right=lpsFwaFurther->rZone.left+psAni->rZone.right-1;
							rZone.bottom=lpsFwaFurther->rZone.top+psAni->rZone.bottom-1;
							//InvalidateRect(lpsFwaFurther->hOwnerWnd,&rZone,FALSE);
							*/

						} 

					} else {_displayRefresh(psAni,NULL);}
				} // Animazione attiva
			} // Loop

			// Rinfreso i "parenti"
			for (a=0;a<_l.dmiAnim.Num;a++) {
				psAni=_l.arsAnim+a; 
				if (psAni->bDcRedraw) {
					psAni->bDcRedraw=false;
					_displayRefresh(psAniParent,NULL);
				}
			}
		}
	} // while Event
	printf("_thrAnimation exit .." CRLF);
	return 0;
}

// -----------------------------------------------------------
// Thread per animazione FWA Child 
//
/*
static DWORD WINAPI _thrAnimationChild(LPVOID Dato)
{
  DWORD dwWait;
  INT iFuther;
  S_FWANIM *lpsFwa;
  S_FWANIM *lpsFwaFurther;
  INT iFrameShow;
  INT a;
  RECT rZone;
  static INT iCount=0;

  while (TRUE)
  {
//	 dwWait=WaitForMultipleObjects(2,sFwa.hEvent,FALSE,sFwa.iMill);
	 dwWait=WaitForMultipleObjects(2,_l.arhEventChild,FALSE,ANI_MILLIBASE); // 50simi di secondo
	
	 // Chiusura del trhead
	 if (dwWait==WAIT_OBJECT_0+EANIM_BREAK) break;

	 // Analisi
	 if (dwWait==WAIT_OBJECT_0+EANIM_ANALISE) 
	 {
		ResetEvent(_l.arhEventChild[EANIM_ANALISE]); 
	 }

	 // Incremento i frame
	 if (dwWait==WAIT_TIMEOUT)
	 {
		// Incremento i contatori
		for (a=0;a<_l.dmiAnim.Num;a++) 
		{
			lpsFwa=&_l.arsAnim[a];
			if (lpsFwa->enType!=FWA_ANICHILD) continue;
			lpsFwa->iTimeCount++;
			if (!a) dispx("%d/%d",lpsFwa->iTimeCount,lpsFwa->iTimeFrame);
			if (lpsFwa->iTimeCount>lpsFwa->iTimeFrame)
			{
			 _l.arsAnim[a].iTimeCount=0;
			 if (_l.arsAnim[a].funcFrame&&_l.arsAnim[a].fShow) 
			 {
				iFuther=(INT) _l.arsAnim[a].hOwnerWnd;
				lpsFwaFurther=_l.arsAnim+iFuther;

				// B) Chiedo refresh del DC
				if (lpsFwaFurther->iFrameShow!=-1) 
				{
					// A) Chiedo accesso esclusivo al DCVirtuale
					iFrameShow=lpsFwaFurther->iFrameShow;
					_frameLock(iFuther,iFrameShow);
				
					//dispx("%d) [%s] %d   %d",iNum,lpsFwa->lpName,lpsFwa->iCount,lpsFwa->fShow);
					//if (lpsFwaFurther->iFrameShow) dispx("%d      ",lpsFwaFurther->iFrameShow);
					(_l.arsAnim[a].funcFrame)(WS_DISPLAY,1,&_l.arsAnim[a],lpsFwaFurther->hDCFrame[iFrameShow]);

					// C) Rilascio accesso esclusivo
					_frameUnlock(iFuther,iFrameShow);

					// D) Richiedo refresh della zona modificata
					rZone.left=lpsFwaFurther->rZone.left+_l.arsAnim[a].rZone.left;
					rZone.top=lpsFwaFurther->rZone.top+_l.arsAnim[a].rZone.top;
					rZone.right=lpsFwaFurther->rZone.left+_l.arsAnim[a].rZone.right-1;
					rZone.bottom=lpsFwaFurther->rZone.top+_l.arsAnim[a].rZone.bottom-1;
					InvalidateRect(lpsFwaFurther->hOwnerWnd,&rZone,FALSE);
					
				}
			 }
			}
		}
	 }
  }
   return 0;
}
*/

// --------------------------------------------
// Gestore di GIF che sfrutta WAnimation
//
// static void IMGDispDC(HDC hdc,INT PosX,INT PosY,INT Hdl);
typedef struct {
  GIFINFO	sGIFInfo;
  //CRITICAL_SECTION cs;
  HANDLE	mtx;
} MT_GIFINFO;

//
// gifAnimation()
//
void * gifAnimation(INT cmd,LONG lParam,void *lpPtr,HDC hdcTarget) //      CHIAMATA ALLA ROUTINE
{
	S_FWANIM *	lpFwa=lpPtr;
	MT_GIFINFO sGifInfo;
	static _DMI		dmiGifs=DMIRESET;
	static MT_GIFINFO * arsGifInfo;
	static INT iNum;
	INT Hdl;
	INT a;
	BOOL fCheck;

	switch (cmd)
	{
		case WS_OPEN: // Apro un file FLC
			DMIOpen(&dmiGifs,RAM_AUTO,lParam,sizeof(sGifInfo),"*WANIGIF");
			arsGifInfo=memoLock(dmiGifs.Hdl);
			break;
		
		case WS_CLOSE:
			for (a=0;a<dmiGifs.Num;a++)
			{
				WaitForSingleObject(arsGifInfo[a].mtx,INFINITE);  // EnterCriticalSection(&arsGifInfo[a].cs);
				
				GIFCloseFile(&arsGifInfo[a].sGIFInfo); // Recupero le risorse impegnate
				_(arsGifInfo[a].sGIFInfo);
		//		LeaveCriticalSection(&arsGifInfo[a].cs);
		//
				ReleaseMutex(arsGifInfo[a].mtx);	//	LeaveCriticalSection(&csAnimation);
				CloseHandle(arsGifInfo[a].mtx); 
				arsGifInfo[a].mtx=NULL; //DeleteCriticalSection(&csAnimation);

			}
			DMIClose(&dmiGifs,"*WANIGIF");
			arsGifInfo=NULL;
			break;

		case WS_ADD:
			//if (GIFOpenFile((CHAR *) lpPtr,&sGIFInfo)) {win_infoarg(lpPtr);return NULL;}
			_(sGifInfo);
			if (!GIFOpenFile((CHAR *) lpPtr,&sGifInfo.sGIFInfo)) {win_infoarg(lpPtr);return NULL;}
			DMIAppend(&dmiGifs,&sGifInfo);
			iNum=dmiGifs.Num-1;
			//InitializeCriticalSection(&arsGifInfo[iNum].cs);
			arsGifInfo[iNum].mtx=CreateMutex(NULL,false,NULL);
			return &iNum;

		case WS_PROCESS: // Sistema le dimensioni del size
			switch (lParam)
			{
				case 0: // Setta in CX le misure del gif
						WaitForSingleObject(arsGifInfo[lpFwa->lParam].mtx,INFINITE);  // EnterCriticalSection(&arsGifInfo[lpFwa->lParam].cs);
						lpFwa->sZone.cx=arsGifInfo[lpFwa->lParam].sGIFInfo.sArea.cx;
						lpFwa->sZone.cy=arsGifInfo[lpFwa->lParam].sGIFInfo.sArea.cy;
						ReleaseMutex(arsGifInfo[lpFwa->lParam].mtx);	//	LeaveCriticalSection(&arsGifInfo[lpFwa->lParam].cs);
						break;
			}
			break;

		//
		// Richiamata da animazione > Chiede di scrive in un DC
		//
		case WS_DISPLAY:

			//DMIRead(&dmiGifs,lpFwa->lParam,&sGIFInfo);
			// Se non deve essere visualizzato, mi fermo
			if (!lpFwa->bVisible) break;
			
			WaitForSingleObject(arsGifInfo[lpFwa->lParam].mtx,INFINITE);  // EnterCriticalSection(&arsGifInfo[lpFwa->lParam].cs);
			fCheck=GIFtoIMG(
					//&sGIFInfo,			// Struttura GIF
						&arsGifInfo[lpFwa->lParam].sGIFInfo,
						lpFwa->iCount,
						lpFwa->iCount,					// Fino al Frame
			 			lpFwa->cBackGround,
						&Hdl,
						IMG_PIXEL_BGR);
//			if (!strBegin(lpFwa->lpName,"SPR_")) {
//				printf("%s:%d,%d" CRLF,lpFwa->lpName,lpFwa->rZone.left,lpFwa->rZone.top);
//			}
			
			// se lParam=1 indica di andare avanti di un frame
			if (lParam&lpFwa->bRun) {lpFwa->iCount++; if (lpFwa->iCount>=arsGifInfo[lpFwa->lParam].sGIFInfo.dmiFrames.Num) lpFwa->iCount=0;}
			ReleaseMutex(arsGifInfo[lpFwa->lParam].mtx);	//	LeaveCriticalSection(&arsGifInfo[lpFwa->lParam].cs);
			
			if (!fCheck)
			{  
			  if (hdcTarget) 
				  //IMGDispDC(hdcTarget,lpFwa->rZone.left,lpFwa->rZone.top,Hdl);
				  dcImageShow(hdcTarget,lpFwa->rZone.left,lpFwa->rZone.top,0,0,Hdl);
			  memoFree(Hdl,"Image");
			}
			break;
		}
	return NULL;
}

/*
static void IMGDispDC(HDC hdcDest,INT PosX,INT PosY,INT Hdl)
{
	
	IMGHEADER *Img;
	HDC hdcSorg;
	HBITMAP BitMap;
	BITMAPINFOHEADER BmpHeaderBackup;
	BITMAPINFOHEADER *BmpHeader;
	BITMAPINFO *BmpInfo;
	BYTE *Sorg;
	LONG Lx,Ly;


	return;

	Img=memoLock(Hdl);
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
	memoUnlock(Hdl);
}
*/

static void _displayRefresh(S_FWANIM * psAni,RECT * psRect) {

	if (psAni->idParent) return;
	if (psAni->hWindow) {
		InvalidateRect(psAni->hWindow,&psAni->rZone,FALSE);	
	}
	else if (psAni->psObj) {
	
		HWND hwd=WIN_info[psAni->psObj->idxWin].hWnd;
		InvalidateRect(hwd,&psAni->psObj->sClientRect,FALSE);	

		//printf("qui");
	}
	

}