//   ---------------------------------------------
//   | ZCMP_Printer
//   | ZoneComponent > Form
//   |                                              
//   | Seleziona un stampante
//   |                                              
//   |							by Ferrà Srl 2010
//   ---------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/PowerDoc.h"
#include "/easyhand/ehtool/ZCMP_Printer.h"
#include "/easyhand/ehtool/EditFloat.h"

static struct {

	BOOL	bReady;

} sPrinter= {0};

static void _LPrinterCreateInfo(EHZ_PRINTER *psPrinter);
static void _LPrinterDestroyInfo(EHZ_PRINTER *psPrinter);
static void _PaperFormatShow(EH_DISPEXT *psDex,EHZ_PRINTER *psPrinter,DWORD cx,DWORD cy,CHAR *pszNamePaper);
#ifdef EH_PDF
static SINT _ehPdfPrinterPage(CHAR **ppszPrinterDefine);
#endif

//
// ehzPrinter()
//
void * ehzPrinter(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr)
{
	EH_DISPEXT *psDex=ptr;
	EHZ_PRINTER *psPrinter;
	EH_EVENT *psEvent;
	RECT rcArea;//,rcOrient;
	EH_COLOR colBack;
	SIZE sizArea;
//	EH_ARF arfp;
	EH_ARF arfPrinter=NULL;
//	CHAR szServ[200];
	BYTE *pszName;
	SINT x;
	BOOL bSet;
	
	psPrinter=objCalled->pOther;
//	if (!psPrinter.bReady) _LInitialize();
	switch(cmd)
	{
		//
		// Creazione della finestra Form
		//
		case WS_CREATE:

			psPrinter=objCalled->pOther=ehAllocZero(sizeof(EHZ_PRINTER));
			psPrinter->lpObj=objCalled;
			break;

		case WS_DESTROY: 
			//ehFreePtr(&psPrinter->psDevMode);
			_LPrinterDestroyInfo(psPrinter);
			ehFreePtr(&psPrinter->pszDeviceDefine);
			ehFreePtr(&objCalled->pOther);
			break;

		case WS_DO: 
			MoveWindow(objCalled->hWnd,psDex->px,psDex->py,psDex->lx,psDex->ly,TRUE);
			break;

		case WS_DISPLAY:
			if (!psDex) break;

			ModeColor(TRUE);
			Tboxp(psDex->rClientArea.left,psDex->rClientArea.top,psDex->rClientArea.right,psDex->rClientArea.bottom,sys.ColorBackGround,SET);
			colBack=ColorFusion(sys.Color3DLight,sys.Color3DHighLight,50); if (psPrinter->bMouseOver) colBack=sys.Color3DHighLight;

			arfPrinter=ARFSplit(psPrinter->pszDeviceDefine?psPrinter->pszDeviceDefine:"","\1");
			switch (psPrinter->iLayout) {

				default:
					rectCopy(rcArea,psDex->rClientArea); rcArea.bottom=rcArea.top+19;
					sizeCalc(&sizArea,&rcArea);
					dcRectRound(psDex->hdc,&rcArea,
								 sys.Color3DShadow,
								 colBack,
								 10,10,1);


					if (!strEmpty(psPrinter->pszDeviceDefine)) {


						//
						// Nome della stampante -------------------------
						//
						ico_disp(psDex->px+2,psDex->py+2,"PRNLOC");
						pszName=arfPrinter[0]; 
#ifdef EH_PDF						
						if (!strcmp(pszName,"!PDF")) pszName="Salva come PDF (file)";
#endif
						x=dispf(psDex->px+22,psDex->py+2,0,colBack,STYLE_BOLD,"#Arial",16,pszName);
						psPrinter->xName=psDex->px+22+x;

						//
						// Formato Carta e orientamento -----------------
						//
#ifdef EH_PDF
						if (!strCmp(arfPrinter[0],"!PDF")) {
							PWD_FORMINFO sFormInfo;

								ehPdfGetForm(psPrinter->pszDeviceDefine,&sFormInfo);
								_PaperFormatShow(	psDex,psPrinter,
													(SINT) (sFormInfo.sizForm.cx*300),
													(SINT) (sFormInfo.sizForm.cy*300),
													sFormInfo.szPaperName);
								/*
							} else {
								_PaperFormatShow(psDex,psPrinter,0,0,"Formato Carta indefinito");
							}
							*/

						}
						else 
#endif
						{

							if (!psPrinter->psDevMode) _LPrinterCreateInfo(psPrinter);

							if (psPrinter->psDevMode) {

								_PaperFormatShow(	psDex,psPrinter,
													psPrinter->sPhysicalPage.cx,
													psPrinter->sPhysicalPage.cy,
													psPrinter->psDevMode->dmFormName);
							}						
							else {
								_PaperFormatShow(psDex,psPrinter,0,0,"Formato Carta indefinito");
							}
						}

					} else
					 {
						 //ico_disp3D(psDex->px+1,psDex->py+3,sys.Color3DShadow,"PRNLOC");
						 dispf(psDex->px+4,psDex->py+2,0,-1,STYLE_ITALIC,"#Arial",16,"Stampante non selezionata");
					}
					break;
			}
			ModeColor(FALSE);
			ehFree(arfPrinter);
			break;

		case WS_REALGET:
			return psPrinter->pszDeviceDefine;

		case WS_REALSET:

			ehFreePtr(&psPrinter->pszDeviceDefine);
			arfPrinter=ARFSplit(ptr?ptr:"","\1");
			if (!strCmp(ptr,"default")) 
			{
				ehPrinterGetDefault(&psPrinter->pszDeviceDefine);
			}
			else if (!strEmpty(ptr)) {
			
				strAssign(&psPrinter->pszDeviceDefine,ptr);
#ifdef EH_PDF
				if (strCmp(arfPrinter[0],"!PDF")) {
					if (ehPrinterExist(&psPrinter->pszDeviceDefine,TRUE)) 
					{
						// Azzero le caratteristiche (le leggo in display)
						_LPrinterDestroyInfo(psPrinter);
					}
					else {strAssign(&psPrinter->pszDeviceDefine,"");}
				}
#else
				if (ehPrinterExist(&psPrinter->pszDeviceDefine,TRUE)) 
				{
					// Azzero le caratteristiche (le leggo in display)
					_LPrinterDestroyInfo(psPrinter);
				}
				else {strAssign(&psPrinter->pszDeviceDefine,"");}
#endif
			}
			obj_reset(psPrinter->lpObj->nome,TRUE);
			ehFreeNN(arfPrinter);
			break;


		case WS_EVENT:

			psEvent=(EH_EVENT *) ptr;
			switch (psEvent->iEvent)
			{
				case EE_LBUTTONDOWN:

					arfPrinter=ARFSplit(psPrinter->pszDeviceDefine?psPrinter->pszDeviceDefine:"","\1");
					switch (psPrinter->iLayout) {


						default:
							ehPrintd("%d/%d" CRLF,psEvent->sPoint.x,psPrinter->xPaper);
							if (psEvent->sPoint.x>=psPrinter->xPaper&&
							//
							// Controllo del click sinistro (Richiedo Formato)
							//
								psPrinter->xPaper&&
								!strEmpty(psPrinter->pszDeviceDefine)) {

								//
								// PDF
								//
#ifdef EH_PDF

								if (!strCmp(arfPrinter[0],"!PDF")) {

									//alert("PDF");
									bSet=_ehPdfPrinterPage(&psPrinter->pszDeviceDefine);

								}
								//
								// Selezione tipo di pagina
								//
								else 
#endif
									bSet=ehPrinterPage(&psPrinter->pszDeviceDefine);

								if (bSet)  {

									obj_reset(psPrinter->lpObj->nome,TRUE);
									obj_addevent(psPrinter->lpObj->nome);
									_LPrinterDestroyInfo(psPrinter);

								}


							}
							else
							{
							//
							// Controllo del click sinistro (Richiedo stampante)
							//

//								PRINTER_INFO_2 *psLabelDev=NULL;
//								PRINTER_INFO_1 *psLabelDev=NULL;
								PRINTER_INFO_4 *psLabelDev=NULL;
								DWORD dNeeded,dReturned;
								EH_ARF arf;
								//SINT i;

								EH_MENU * psMenu;
								BYTE *pszChoose;
								
								SINT x;
								//SQL_RS rsArt;
								//S_LAYOUT_DATA sLD;

								//
								// Selezione della stampante
								//
								/*if (ehPrinterChoose(&psPrinter->pszDeviceDefine,FALSE))
								{
									obj_reset(psPrinter->lpObj->nome,TRUE);
									obj_addevent(psPrinter->lpObj->nome);
									_LPrinterDestroyInfo(psPrinter);
								}*/

								EnumPrinters(	PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS,
												NULL,
												4,
												NULL,
												0,
												&dNeeded,
												&dReturned);

								psLabelDev = ehAllocZero(dNeeded);

								if (!EnumPrinters( PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS,
												   NULL,
												   4,
												   (LPBYTE) psLabelDev,
												   dNeeded,
												   &dNeeded,
												   &dReturned)) 
								   ehExit("errore nella enumerazione stampanti!");

								psMenu=ehMenuCreate();
								arf=strSplit(psPrinter->pszDeviceDefine?psPrinter->pszDeviceDefine:"","\1");

								ehMenuAdd(psMenu,EHM_ITEM,"Preferenze di stampa",TRUE,"!PREFERENCE",NULL,0,0,NULL);
								ehMenuAdd(psMenu,EHM_ITEM,"Pannello di controllo",TRUE,"!PANEL",NULL,0,0,NULL);
								ehMenuAdd(psMenu,EHM_SEP,"",FALSE,"",NULL,0,0,NULL);

								ehMenuAdd(psMenu,EHM_ITEM,"Stampanti:",FALSE,"",NULL,0,0,NULL);

#ifdef EH_PDF
								ehMenuAdd(psMenu,!strcmp(arf[0],"!PDF")?EHM_CHECK:EHM_ITEM,"Salva come PDF",TRUE,"!PDF",NULL,0,0,NULL);
//								ehMenuAdd(psMenu,EHM_SEP,"",FALSE,"",NULL,0,0,NULL);
#endif
								for (x=0;x<(SINT) dReturned;x++) {
//									ehMenuAdd(psMenu,!strcmp(arf[0],psLabelDev[x].pPrinterName)?EHM_CHECK:EHM_ITEM,psLabelDev[x].pPrinterName,TRUE,psLabelDev[x].pPrinterName,NULL,0,0,NULL);
//									ehMenuAdd(psMenu,!strcmp(arf[0],psLabelDev[x].pName)?EHM_CHECK:EHM_ITEM,psLabelDev[x].pName,TRUE,psLabelDev[x].pName,NULL,0,0,NULL);
									ehMenuAdd(psMenu,!strcmp(arf[0],psLabelDev[x].pPrinterName)?EHM_CHECK:EHM_ITEM,psLabelDev[x].pPrinterName,TRUE,psLabelDev[x].pPrinterName,NULL,0,0,NULL);
								}
//								ehMenuAdd(psMenu,EHM_SEP,"",FALSE,"",NULL,0,0,NULL);

								pszChoose=MenuFloat(psMenu->arsItem,""); 
								if (!strEmpty(pszChoose)) //sLD.pszPF=strDup(pszChoose);
								{

									//
									// Selezione con pannello di controllo
									//
									if (!strcmp(pszChoose,"!PANEL")) {
									
										if (ehPrinterChoose(&psPrinter->pszDeviceDefine,FALSE))
										{
											obj_reset(psPrinter->lpObj->nome,TRUE);
											obj_addevent(psPrinter->lpObj->nome);
											_LPrinterDestroyInfo(psPrinter);
										}

									}
									else if (!strcmp(pszChoose,"!PREFERENCE")) {
									
										if (enPrinterDoc(&psPrinter->pszDeviceDefine))
										{
											//obj_reset(psPrinter->lpObj->nome,true);
											//obj_addevent(psPrinter->lpObj->nome);
											_LPrinterDestroyInfo(psPrinter);
										}

									}

//enPrinterDoc(&pszPrinterDefine);

									//
									// Selezione diretta
									//
									else {

										strAssign(&psPrinter->pszDeviceDefine,pszChoose); // psPrinter->pszDeviceDefine=strDup(pszChoose);
										obj_reset(psPrinter->lpObj->nome,TRUE);
										obj_addevent(psPrinter->lpObj->nome);
										_LPrinterDestroyInfo(psPrinter);
									}
								}
								ehMenuDestroy(psMenu);
								ehFree(psLabelDev);
								ehFree(arf);
							}
					}
					ehFreeNN( arfPrinter);
					break;

				case EE_MOUSEOVER:
					psPrinter->bMouseOver=TRUE; obj_disp(psPrinter->lpObj);
					break;

				case EE_MOUSEOUT: // MOUSEOUT ?!
					psPrinter->bMouseOver=FALSE; obj_disp(psPrinter->lpObj);
					break;
			}
			break;

		case WS_INF: 
			return psPrinter;

	}
	return NULL;
}

static void _PaperFormatShow(EH_DISPEXT *psDex,EHZ_PRINTER *psPrinter,DWORD cx,DWORD cy,CHAR *pszNamePaper) 
{
	RECT rcArea;
	SIZE sizArea,sizPaper;
	RECT rcOrient;
	EH_ARF arfp;
	DWORD cxMin;

	rectCopy(rcArea,psDex->rClientArea); rcArea.bottom=rcArea.top+19;
	sizeCalc(&sizArea,&rcArea);

	// Calcolo dimensioni box carta
	// realx:realy=x:(rcOrient.bottom-rcOrient.top+1)
	if (cx&&cy) {
		sizPaper.cy=16; // (rcOrient.bottom-rcOrient.top+1)
		sizPaper.cx=(sizPaper.cy*cx/cy);
		if (sizPaper.cx>16) {sizPaper.cx=16; sizPaper.cy=(sizPaper.cx*cy/cx);}
	} else ZeroFill(sizPaper);
	rcOrient.right=rcArea.right-5;
	rcOrient.top=rcArea.top+(sizArea.cy/2-sizPaper.cy/2);
	rcOrient.bottom=rcOrient.top+sizPaper.cy-1;
	rcOrient.left=rcOrient.right-sizPaper.cx+1;
	if (cx) dcRect(psDex->hdc,&rcOrient,sys.Color3DShadow,RGB(255,255,255),1);

	arfp=ARFSplit(pszNamePaper," ");
	cx=sizArea.cx-23-psPrinter->xName;
	cxMin=font_lenf(arfp[0],"#Arial",14,STYLE_ITALIC)+3;
	if (cx>cxMin) {

		cxMin=font_lenf(pszNamePaper,"#Arial",14,STYLE_ITALIC);
		psPrinter->xPaper=rcOrient.left-cxMin-2;
		if (psPrinter->xPaper<(psPrinter->xName+2)) psPrinter->xPaper=psPrinter->xName+2;
		dispfmS(psPrinter->xPaper,psDex->py+3,cx,0,-1,STYLE_ITALIC,"#Arial",14,pszNamePaper);

	}
	else {

		psPrinter->xPaper=rcOrient.left-cxMin;
		dispfp(psPrinter->xPaper,psDex->py+3,DPL_LEFT,0,-1,STYLE_ITALIC,"#Arial",14,arfp[0]);
	}
	ehFree(arfp);
}

//
// _LPrinterCreateInfo()
//
static void _LPrinterCreateInfo(EHZ_PRINTER *psPrinter) {

//	SINT a;
//	BYTE *pszPapers;
//	WORD *pw;
	HDC hdcPrinter;

	psPrinter->psDevMode=ehPrinterGetDev(psPrinter->pszDeviceDefine);

	hdcPrinter= ehPrinterCreateDC(psPrinter->pszDeviceDefine,NULL);
	if (hdcPrinter) {
		psPrinter->sPhysicalPage.cx=GetDeviceCaps(hdcPrinter,PHYSICALWIDTH);
		psPrinter->sPhysicalPage.cy=GetDeviceCaps(hdcPrinter,PHYSICALHEIGHT);
		DeleteDC(hdcPrinter);
	} // 	DeleteDC(hdcPrinter);

/*
	 psPrinter->iNumPapers= DeviceCapabilities( psPrinter->psDevMode->dmDeviceName, NULL, DC_PAPERNAMES, NULL, psPrinter->psDevMode );
	// Alloco gli indici
    pw = ehAllocZero(psPrinter->iNumPapers*sizeof(WORD));
	DeviceCapabilities( psPrinter->psDevMode->dmDeviceName, NULL, DC_PAPERS, (char*) pw , psPrinter->psDevMode);
	
	// Alloco il nome dei formati
	pszPapers=ehAlloc(psPrinter->iNumPapers*64);
	DeviceCapabilities( psPrinter->psDevMode->dmDeviceName, "LPT1", DC_PAPERNAMES, (BYTE *) pszPapers, psPrinter->psDevMode );
	psPrinter->arPapers=ARNew();
	for (a=0;a<psPrinter->iNumPapers;a++) {
		ARAddarg(&psPrinter->arPapers,"[%d]%s",pw[a],pszPapers+(a*64));
	}
	ehFree(pw);
	ehFree(pszPapers);
	*/
}

//
// _LPrinterDestroyInfo()
//
static void _LPrinterDestroyInfo(EHZ_PRINTER *psPrinter) {

	ehFreePtr(&psPrinter->psDevMode);
//	ARDestroy(psPrinter->arPapers); psPrinter->arPapers=NULL;
//	psPrinter->iNumPapers=0;
}




//
// _ehPdfPrinterPage() > Seleziona il foglio di un pdf
// Ritorna TRUE se è stata cambiata
//
#ifdef EH_PDF

static SINT _ehPdfPrinterPage(CHAR **ppszPrinterDefine) {

	EH_MENU * psMenu;
	CHAR *pszChoose;
	BOOL bRet=FALSE;
	PWD_FORMINFO sFormInfo;
	EH_ARF arfPrinter;

	arfPrinter=ARFSplit(*ppszPrinterDefine,"\1");

	psMenu=ehMenuCreate();
	ZeroFill(sFormInfo);

	ehPdfGetForm(*ppszPrinterDefine,&sFormInfo);
/*
	if (!strCmp(arfPrinter[1],"RES")) {
	
		SIZE_T tSize;
		BYTE *pBin=base64Decode(arfPrinter[2],&tSize);
		memcpy(&sFormInfo,pBin,sizeof(sFormInfo));
		ehFree(pBin);
	
	}
*/
	ehMenuAdd(psMenu,EHM_ITEM,"Formato carta",FALSE,"",NULL,0,0,NULL);

	ehMenuAdd(psMenu,(sFormInfo.iPaper==DMPAPER_LETTER)?EHM_CHECK:EHM_ITEM,"Letter (8 1/2 x 11 in)",TRUE,"P1",NULL,0,0,NULL); // DMPAPER_LETTER
	ehMenuAdd(psMenu,(sFormInfo.iPaper==DMPAPER_LEGAL)?EHM_CHECK:EHM_ITEM,"Legal (8 1/2 x 14 in)",TRUE,"P5",NULL,0,0,NULL); // DMPAPER_LEGAL
	ehMenuAdd(psMenu,(sFormInfo.iPaper==DMPAPER_A4)?EHM_CHECK:EHM_ITEM,"A4 (210 x 297 mm)",TRUE,"P9",NULL,0,0,NULL); // DMPAPER_A4

	ehMenuAdd(psMenu,EHM_SEP,"",FALSE,"",NULL,0,0,NULL);
	ehMenuAdd(psMenu,EHM_ITEM,"Orientamento",FALSE,"",NULL,0,0,NULL);
	ehMenuAdd(psMenu,(sFormInfo.iOrientation==DMORIENT_PORTRAIT)?EHM_CHECK:EHM_ITEM,"Verticale",TRUE,"O1",NULL,0,0,NULL); // DMORIENT_PORTRAIT
	ehMenuAdd(psMenu,(sFormInfo.iOrientation==DMORIENT_LANDSCAPE)?EHM_CHECK:EHM_ITEM,"Orizzontale",TRUE,"O2",NULL,0,0,NULL); // DMORIENT_LANDSCAPE

	
	pszChoose=MenuFloat(psMenu->arsItem,""); 
	if (!strEmpty(pszChoose)) {

		SINT iLen;
		BYTE *pRet=NULL,*pBase;

		if (*pszChoose=='P') sFormInfo.iPaper=atoi(pszChoose+1);
		if (*pszChoose=='O') sFormInfo.iOrientation=atoi(pszChoose+1);

		if (!sFormInfo.iPaper) sFormInfo.iPaper=DMPAPER_A4;
		if (!sFormInfo.iOrientation) sFormInfo.iOrientation=DMORIENT_PORTRAIT;
		
		switch (sFormInfo.iPaper) {
		
			case DMPAPER_LETTER: // 8 1/2 x 11 in 
				strcpy(sFormInfo.szPaperName,"Letter (8.5 x 11 in)");
				sFormInfo.sizForm.cx=8.5*25.4;
				sFormInfo.sizForm.cy=11*25.4;
				break;

			case DMPAPER_LEGAL: // Legal 8 1/2 x 14 in
				strcpy(sFormInfo.szPaperName,"Legal (8.5 x 14 in)");
				sFormInfo.sizForm.cx=8.5*25.4;
				sFormInfo.sizForm.cy=14*25.4;
				break;

			case DMPAPER_A4: // A4 210 x 297 mm   
				strcpy(sFormInfo.szPaperName,"A4 (210 x 297 mm)");
				sFormInfo.sizForm.cx=210;
				sFormInfo.sizForm.cy=297;
				break;
		}

		if (sFormInfo.iOrientation==DMORIENT_LANDSCAPE) {
			double dVal;
			dVal=sFormInfo.sizForm.cx;
			sFormInfo.sizForm.cx=sFormInfo.sizForm.cy;
			sFormInfo.sizForm.cy=dVal;
		}

		// Converto in stringa
		iLen=sizeof(sFormInfo);
		pBase=base64Encode(0,(BYTE *) &sFormInfo,iLen);
		pRet=ehAlloc(strlen(pBase)+50);
		sprintf(pRet,"!PDF\1RES\1%s",pBase);
		strAssign(ppszPrinterDefine,pRet);
		ehFree(pBase);
		ehFree(pRet);
		bRet=TRUE;

	}

	ehMenuDestroy(psMenu);
	ehFree(arfPrinter);
	return bRet;
}
#endif 