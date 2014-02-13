// +-----------------------------------------+
// | WinStart  Inizializzazione di Windows   |
// |           ATTENZIONE:                   |
// |           Non modificare se non con     |
// |           estrema consapevolezza e      |
// |           prudenza                      |
// |                                         |
// | Creato da Tassistro       by Ferr… 1999 |
// +-----------------------------------------+
#include <io.h>
#include <errno.h>
#include <stdio.h>
#include <conio.h> // x kbhit
#include <string.h>
#include <stdlib.h>
//#include "resource.h"

#include "\ehtool\include\flm_main.h"
#include "\ehtool\include\flm_vid.h"
#include "\ehtool\include\flm_vari.h"

#include <dlgs.h>       // includes common dialog template defines

extern SINT dbclktm;
LRESULT CALLBACK WDriver(HWND hWnd,
						 UINT message,
						 WPARAM wParam,
						 LPARAM lParam);

static void MouseClone(void);
CHAR szAppName[]="EH3WIN";

SINT WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPSTR lpszCmdLine, 
				   SINT cmdShow)
{
//		SINT a;
		HWND hwndDesktop;
		HDC hdc;
		//DWORD ThreadID;

		sys.EhWinInstance=hInstance;
		sys.DosEmulation=TRUE;
		sys.WinFocus=-1; // Nessuna finestra a fuoco
        sys.HwndInput=NULL;
        sys.EhWinType=FALSE;
        
		//        sys.EhWinType=TRUE;
		// ------------------------------------------------------------------------------
	    // Semaforo di controllo per non lanciare due volte la stessa applicazione      |
	    // ------------------------------------------------------------------------------
/*
		hSem = CreateSemaphore( NULL, 1,1,"Guardian" );
	    if( hSem == NULL )  {return( 0 );}

	    dwWait = WaitForSingleObject( hSem, 0L); if( dwWait != WAIT_OBJECT_0 ) return( 0 );
         if (!InitApplication(hInstance)) return (FALSE);              
*/

		//for (a=0;a<WinKeySize;a++) {WinKeyBuffer[a]=0;}
		sys.EhPaint=FALSE;// Annullo la chiamata al repaint

		// ------------------------------
		//  CREA LA CLASSE              !
		// ------------------------------

		if (!hPrevInstance)         // if no prev instance, this is first
			{
		    WNDCLASSEX wc;

			// Definisce il tipo di finestra dell'applicazione
			// (window class)
//			wcTTFClass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
			wc.cbSize        = sizeof(wc);
			wc.style         = CS_NOCLOSE|CS_DBLCLKS;

			// Funzione chiamata per la gestione della finestra
			wc.lpfnWndProc   = WDriver;
			wc.cbClsExtra    = 0;
			wc.cbWndExtra    = 0;
			wc.hInstance     = hInstance;
//			wcTTFClass.hIcon         = LoadIcon(hInstance, "BTRA");
			wc.hIcon         = LoadIcon(NULL,IDI_APPLICATION);
			wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
			wc.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
		 	//wcTTFClass.hbrBackground = NULL;
			wc.lpszMenuName  = NULL;//szAppName;
			wc.lpszClassName = szAppName;
			wc.hIconSm       = LoadIcon(NULL,IDI_APPLICATION);
			RegisterClassEx(&wc);
		}


		// ------------------------------
		// CREA LA FINESTRA             !
		// ------------------------------

		lpszCmdLine--;// per non dare il warning
/*
		winhandle=CreateWindow(szAppName,// Nome della classe
											TitoloFinestra,// Titolo finestra
											WS_OVERLAPPEDWINDOW,
											0,0,      // Posizione x,y
											639,479,  // Dimensione x,y
											NULL,NULL,hInstance,NULL);
  */
		hwndDesktop = GetDesktopWindow();
		hdc = GetDC (hwndDesktop);
//		sScreenCx = GetDeviceCaps (hdc, HORZRES);
//		sScreenCy = GetDeviceCaps (hdc, VERTRES);

  // ORIGINALE ERA COSI
/*
		sys.HwndCurrent= CreateWindow (szAppName, "",
									   WS_POPUP | WS_VISIBLE,
									   0, 0,
									   GetDeviceCaps (hdc, HORZRES),
									   GetDeviceCaps (hdc, VERTRES)-30,
									   NULL, NULL,
									   hInstance, NULL);
*/


/*
		sys.HwndCurrent= CreateWindow (szAppName, 
			                           "",
									   WS_POPUP|WS_VISIBLE ,
									   50, 50,
									   640,480,
									   NULL, NULL,
									   hInstance, 
									   NULL);
*/
										 /*
		sys.HwndCurrent= CreateWindow (szAppName, "EasyHand 2000",
								   WS_VISIBLE|WS_MINIMIZE|WS_MINIMIZEBOX,
								   0, 0,
								   640,480,
//										 GetSystemMetrics (SM_CXSCREEN),
//										 GetSystemMetrics (SM_CYSCREEN),
								   NULL, NULL,
								   hInstance, NULL);

		
*/	

		ReleaseDC (hwndDesktop, hdc);
/*
		// -----------------------------------------------
		// CREA I THREAD PER LA GESTIONE SIMULATA        |
		// -----------------------------------------------
		CreateThread(NULL,
			         0,
			         (LPTHREAD_START_ROUTINE) MouseClone,
			         NULL,
			         0,
					 &ThreadID);
*/
		
		//sys.HwndCurrent=GetDesktopWindow(); // Desktop
		// Area di Clip
		main();
		cmdShow++; // x warning
		
//		ExitThread(ThreadID);
		return 0;
		//return (WinRitorno);
}


// +-----------------------------------------+
// | WDriver Dispatcher Driver per           |
// |         emulazione Easyhand             |
// |                                         |
// |                           by Ferr… 1996 |
// +-----------------------------------------+

LRESULT CALLBACK WDriver(HWND hWnd,
						 UINT message,
						 WPARAM wParam,
						 LPARAM lParam)
{
	 //CHAR buf[200];
	 
	 SINT win;
	 INT16 Tasto;

	 //static SINT cont=0;
		switch (message)
		{
				case WM_MOVE:
					win=HwndToWin(hWnd);
					if (win>-1)
					{
					 WIN_info[win].x=LOWORD(lParam);
					 WIN_info[win].y=HIWORD(lParam);
					}
					break;

				// Prima chiamata
				case WM_CREATE:
						//VediMess("CREATE");
						break;

				// Messaggio di cambio cursore
				case WM_SETCURSOR:
						mouse_graph(0,0,"#WM_SETCURSOR#");
						return TRUE;
						//break;

				// Ultia Chiamata
				case WM_DESTROY:
						// This is the end if we were closed by a DestroyWindow call.
						
						//if (FlagBck) DeleteObject(MapBck.bitmap);
						//PRG_end("");
						if (WIN_ult==0) PostQuitMessage(0);
						break;

				case WM_COMMAND:
						break;

				case WM_ERASEBKGND:
						if (EHPower) return TRUE;
						goto ritorna;
						//break;

				case WM_PAINT:
						//VediMess("PAINT");
						return EH_DoPaint(hWnd,wParam,lParam);
						//if (sys.EhPaint) sys.EhPaint();
						break;

				//----------------------------
				// Intercettazione Tastiera  !
				//----------------------------

				case WM_CHAR:
						// Inserisce nel buffer
						//if (wParam==0) break;//

						if (hWnd!=sys.HwndInput) break;
						Tasto=wParam;
						// Tasti emulazione DOS
						if ((Tasto==9)&&(sys.WinKeySpecial&3)) Tasto=0Xf00; // Tab Indietro

						LKBWinInsert(Tasto,lParam&0xF);
				        break;

				// Tasti Funzioni e freccie
				case WM_KEYDOWN:
						if (hWnd!=sys.HwndInput) break;
						 // 112-123 F1-F12
						 // F10 Riservato
						 // 16 Shift
						 // 17 Ctrl
						 // 18 Alt GR (No repeat)
						 // 37 <--    38 su      39 -->      40 giu
						 // 45 Ins    46 Canc    36 - Home   35 Fine
						 // 33 Pag Up            34 Pag Down

						 // Shift
						 //sprintf(buf,"%d rpt:%ld  cont:%d    ",wParam,lParam&0xF,cont++);
						 //Adispfm(200,0,14,0,ON,SET,"SMALL F",2,buf);

						 switch (wParam)
								{case 16: sys.WinKeySpecial|=3; break; // Shift's
								 case 17: sys.WinKeySpecial|=4; break; // Ctrl
								 //case 18: WinKeyShft|=8; break; // Alt grafico/Alt DOS
								}

						 //sprintf(buf,"shift : %d   ",WinKeyShft);
						 //Adispfm(200,30,14,0,ON,SET,"SMALL F",2,buf);

						Tasto=0;
						switch (wParam)
							{
								case 37 : Tasto=((SINT) ('K'))<<8;  // | Left         |
													if (sys.WinKeySpecial&4) Tasto=((SINT) ('s'))<<8;//+Ctrl
													break;
								case 38 : Tasto=((SINT) ('H'))<<8; break; // | Up           |
								case 39 : Tasto=((SINT) ('M'))<<8;  // | Right        |
													if (sys.WinKeySpecial&4) Tasto=((SINT) ('t'))<<8;//+Ctrl
													break;
								case 40 : Tasto=((SINT) ('P'))<<8; break; // | Down         |
								case 45 : Tasto=((SINT) ('R'))<<8; break; // | Insert       |
								case 36 : Tasto=((SINT) ('G'))<<8; break; // | Home         |
								case 33 : Tasto=((SINT) ('I'))<<8; break; // | Pag Up       |
								case 34 : Tasto=((SINT) ('Q'))<<8; break; // | Pag Down     |
								case 35 : Tasto=((SINT) ('O'))<<8; break; // | Fine         |
								case 46 : Tasto=((SINT) ('S'))<<8; break; // | Canc         |

								case 112 : Tasto=((SINT) (';'))<<8; break; //| F1           |
								case 113 : Tasto=((SINT) ('<'))<<8; break; //| F2           |
								case 114 : Tasto=((SINT) ('='))<<8; break; //| F3           |
								case 115 : Tasto=((SINT) ('>'))<<8; break; //| F4           |
								case 116 : Tasto=((SINT) ('?'))<<8; break; //| F5           |
								case 117 : Tasto=((SINT) ('@'))<<8; break; //| F6           |
								case 118 : Tasto=((SINT) ('A'))<<8; break; //| F7           |
								case 119 : Tasto=((SINT) ('B'))<<8; break; //| F8           |
								case 120 : Tasto=((SINT) ('C'))<<8; break; //| F9           |
								case 121 : Tasto=((SINT) ('D'))<<8; break; //| F10          |
								case 122 : Tasto=((SINT) ('E'))<<8; break; //| F11          |
								case 123 : Tasto=((SINT) ('F'))<<8; break; //| F12          |

							}

						if (Tasto) 	LKBWinInsert(Tasto,lParam&0xF);
						break;

				case WM_KEYUP:
						if (hWnd!=sys.HwndInput) break;

						 switch (wParam)
								{case 16: sys.WinKeySpecial&=(3^0xFF); break; // Shift's
								 case 17: sys.WinKeySpecial&=(4^0xFF); break; // Ctrl
								 //case 18: WinKeyShft&=(8^0xFF); break; // Alt grafico/Alt DOS
								}

						break;

				// Controllo delle dimensioni
				case WM_SIZE:
						 //sys.video_x=LOWORD(lParam);
						 //sys.video_y=HIWORD(lParam);
						 break;

				case WM_GETMINMAXINFO:
						// Controlla che i limiti della finestra non vengano
						// ridotti sotto una certa dimensione
						//((POINT far *)lParam)[3].x = 639;
						//((POINT far *)lParam)[3].y = 508;
						break;

				// -----------------------------
				//  Controllo del Mouse        !
				// -----------------------------

				case WM_MOUSEMOVE: 
					    if (hWnd==sys.HwndInput) WinMouse(wParam,lParam);
						break;

				case WM_LBUTTONDOWN:
								if (hWnd!=sys.HwndInput) break;
								sys.ms_b |= 1;
								//if (mouse_disp) mouse_VediMouse();
								break;

				case WM_RBUTTONDOWN:
								if (hWnd!=sys.HwndInput) break;
								sys.ms_b |= 2;
								//if (mouse_disp) mouse_VediMouse();
								break;

				case WM_LBUTTONUP:
								if (hWnd!=sys.HwndInput) break;
								sys.ms_b &= (2+4);
								//if (mouse_disp) mouse_VediMouse();
								break;

				case WM_RBUTTONUP:
								if (hWnd!=sys.HwndInput) break;
								sys.ms_b &= (1+4);
								//if (mouse_disp) mouse_VediMouse();
								break;

				case WM_LBUTTONDBLCLK:
								if (hWnd!=sys.HwndInput) break;
								if (dbclktm) {sys.ms_b |= 1;break;} // Reset
								sys.ms_b |= 4;
								break;
				default:
		ritorna:
		return(DefWindowProc(hWnd, message, wParam, lParam));
		}  // switch message

		return(0L);
}  // end of WndProc()



