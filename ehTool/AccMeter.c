// ------------------------------------------------
//  Accelerometer
//  "la necessità aguzza l'ingegno"
// 
//  Giorgio                      by Ferrà srl 2010
// ------------------------------------------------

#include "/easyhand/inc/easyhand.h" 
#include "/easyhand/inc/AccMeter.h" 


double _sigmoid(double x, 
			   double max,	// Massimo valore della curva
			   double K,	// Valore di meta'
			   double n);

static DWORD WINAPI _Accelerometer(LPVOID pvoid);
static void _AccMeterSetOffset(S_ACCMETER *psAccMeter,SINT xOffset,SINT yOffset);

//
// AccMeter()
//
void * AccMeter(EN_MESSAGE ehMess,SINT info ,void *ptr) {

	S_ACCMETER *psAccMeter;
	void *pRet=NULL;
	switch (ehMess) {
		
		case WS_CREATE:
			psAccMeter=ehAllocZero(sizeof(S_ACCMETER));
			psAccMeter->hwnd=(HWND) ptr;
			psAccMeter->dAccMaxThresold=80;

			//ehSetTimer(NULL,_Accelerometer,40,TRUE);

// B) Lancio l'acquisizione della pagina
			psAccMeter->hThread=CreateThread(NULL,0,_Accelerometer,psAccMeter,0,&psAccMeter->dwThread); 
			pRet=psAccMeter;
			break;

		case WS_DESTROY:
			psAccMeter=ptr;
			psAccMeter->bExit=TRUE;
			if (WaitForSingleObject(psAccMeter->hThread,1000)==WAIT_TIMEOUT) 
			{
				TerminateThread(psAccMeter->hThread,1); 
			}
			memset(psAccMeter,0,sizeof(S_ACCMETER));
			ehFree(psAccMeter);
			break;

	}
	return pRet;
}

//
// _Accelerometer() > Thread
//
static DWORD WINAPI _Accelerometer(LPVOID pvoid)
{
	S_ACCMETER *psAccMeter=pvoid;
	POINT ptMaxOffset;
	POINT ptOffset;
	while (TRUE) {
		WaitForSingleObject(psAccMeter->hThread,40);
		if (psAccMeter->bExit) break;

		//
		// Aggiorno accelerometro
		//
		if (psAccMeter->bButton) {
			psAccMeter->dxAcc=psAccMeter->ptPressPoint.x-psAccMeter->ptPrevious.x;
			psAccMeter->dyAcc=psAccMeter->ptPressPoint.y-psAccMeter->ptPrevious.y;
			psAccMeter->dwReleaseCounter=0;
			psAccMeter->bAccLoaded=TRUE;
		}
		else // Rilascio > decadenza
		{
			double xMaxStep=30;
			if (!psAccMeter->bAccLoaded) continue;
			ptOffset.y=psAccMeter->ptOffset.y;
			ptOffset.x=psAccMeter->ptOffset.x;

			if (!psAccMeter->dwReleaseCounter) {
			
				psAccMeter->dyAccRilev=psAccMeter->dyAcc;
//				printf("%.2f",psAccMeter->dyAccRilev);
				if (psAccMeter->dyAccRilev>psAccMeter->dAccMaxThresold) psAccMeter->dyAccRilev=psAccMeter->dAccMaxThresold;
				if (psAccMeter->dyAccRilev<-psAccMeter->dAccMaxThresold) psAccMeter->dyAccRilev=-psAccMeter->dAccMaxThresold;
//				printf("=%.2f",psAccMeter->dyAccRilev);
			}
			psAccMeter->dwReleaseCounter+=1;
			psAccMeter->dyAcc=psAccMeter->dyAccRilev-_sigmoid(psAccMeter->dwReleaseCounter, 
							psAccMeter->dyAccRilev, // Massimo valore della curva
							psAccMeter->dyAccRilev/2, // Valore di meta'
							6); 

			ptOffset.y-=(SINT) psAccMeter->dyAcc; 

			if (ptOffset.y<0) {psAccMeter->dyAcc=0; ptOffset.y=0;}

			// Calcolo il limite massimo
//			printf("%d,%d" CRLF,psAccMeter->sizContent.cy,psAccMeter->sizClient.cy);
			ptMaxOffset.y=psAccMeter->sizContent.cy-psAccMeter->sizClient.cy; if (ptMaxOffset.y<0) ptMaxOffset.y=0;
			if (ptOffset.y>ptMaxOffset.y) {psAccMeter->dyAcc=0; ptOffset.y=ptMaxOffset.y;}

			if (psAccMeter->dyAcc>-.5&&psAccMeter->dyAcc<.5) {psAccMeter->dyAcc=0; psAccMeter->bAccLoaded=FALSE;}
			_AccMeterSetOffset(psAccMeter,0,ptOffset.y);
//			InvalidateRect(psAccMeter->hwnd,NULL,FALSE);
		}
		memcpy(&psAccMeter->ptPrevious,&psAccMeter->ptPressPoint,sizeof(POINT));

	#ifdef _DEBUG
	//	dispx("%d,%d",psAccMeter->ptDiff.x,psAccMeter->ptDiff.y);
		{
			HDC hdc;
			RECT recBox;
			SINT xBar=(SINT) psAccMeter->dyAcc;
			if (xBar<0) xBar*=-1;
			hdc=GetDC(NULL);
			dcBoxp(hdc,rectFill(&recBox,0,0,xBar,20),RGB(0,0,255));
			dcBoxp(hdc,rectFill(&recBox,xBar+1,0,400,20),RGB(0,0,0));
			dispxEx(0,40,"%d:%.4f ",psAccMeter->dwReleaseCounter,psAccMeter->dyAcc);
			ReleaseDC(NULL,hdc);
		}
	#endif

	}
	return FALSE;
}

static void _AccMeterSetOffset(S_ACCMETER *psAccMeter,SINT xOffset,SINT yOffset) {

	POINT ptScroll;
	ptScroll.y=psAccMeter->ptOffset.y-yOffset;
	psAccMeter->ptOffset.y=yOffset;
	//ScrollWindow(psAccMeter->hwnd,0,ptScroll.y,NULL,NULL);	
	InvalidateRect(psAccMeter->hwnd,NULL,FALSE);
}


//
// AccMeterNotify()
//
void AccMeterNotify(S_ACCMETER *psAccMeter,UINT message,WPARAM wParam,LPARAM lParam) {

	POINT ptMouse;
	if (!psAccMeter) return;

	ptMouse.x=LOWORD(lParam);
	ptMouse.y=HIWORD(lParam);

	switch (message) {

		case WM_SIZE:
			if (!psAccMeter) break;
			GetClientRect(psAccMeter->hwnd, &psAccMeter->recClient);
			sizeCalc(&psAccMeter->sizClient,&psAccMeter->recClient);
			break;

		case WM_LBUTTONDOWN: 
				psAccMeter->bDrag=TRUE;
				memcpy(&psAccMeter->ptDragPoint,&ptMouse,sizeof(ptMouse));
				memcpy(&psAccMeter->ptPrevious,&ptMouse,sizeof(ptMouse));

		case WM_MOUSEMOVE: 
		case WM_LBUTTONUP:


			if (wParam&1&&!psAccMeter->bButton) {
				psAccMeter->bDrag=TRUE;
				memcpy(&psAccMeter->ptDragPoint,&ptMouse,sizeof(ptMouse));
				psAccMeter->ptOffsetDrag.y=psAccMeter->ptOffset.y;
			}

			psAccMeter->bButton=wParam&1;
			memcpy(&psAccMeter->ptPressPoint,&ptMouse,sizeof(ptMouse));
			if (!psAccMeter->bButton) psAccMeter->bDrag=FALSE;

			if (psAccMeter->bDrag) {
			
				POINT ptRel;
				ptRel.y=psAccMeter->ptDragPoint.y-ptMouse.y;
				ptRel.x=psAccMeter->ptDragPoint.x-ptMouse.x;
				_AccMeterSetOffset(psAccMeter,0,psAccMeter->ptOffsetDrag.y+ptRel.y);
			}
			break;
	}
}

void AccMeterReset(S_ACCMETER *psAccMeter) {

	if (!psAccMeter) return;
	psAccMeter->bButton=FALSE;
	psAccMeter->bAccLoaded=FALSE;
	ZeroFill(psAccMeter->ptOffset);
}


void AccMeterContentSize(S_ACCMETER *psAccMeter,SIZE *psSiz) {

	if (!psAccMeter) return;
	memcpy(&psAccMeter->sizContent,psSiz,sizeof(SIZE));
}


//
// Accelerometro
//
static double _sigmoid(double x, 
			   double max,	// Massimo valore della curva
			   double K,	// Valore di meta'
			   double n) 
{
    double value, dXn;
#ifndef MAC
    extern double pow();
#endif

    dXn = pow(x, n);
    value = (max * dXn) / (dXn + pow(K, n));
    return (value);
}
