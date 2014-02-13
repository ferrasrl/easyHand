//   ---------------------------------------------
//   | cssParser  Utilità per analisi di un formato css
//   |				
//   |	
//   |								by Ferrà srl 2014
//   ---------------------------------------------
//
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/cssParser.h"

//
// cssCreate()
//
EH_CSS * cssCreate(UTF8 * pszText) {

	EH_CSS *	pCss;
	EH_AR		arRow,arFld;
	INT			a,iToken;
	S_CSS_ELE	sEle;

	if (strEmpty(pszText)) return NULL;
	pCss=ehNew(EH_CSS);
	pCss->lstEle=lstCreate(sizeof(S_CSS_ELE));
	arRow=strSplit(pszText,";");
	for (a=0;arRow[a];a++) {
		
		arFld=ARCreate(strTrim(arRow[a]),":",&iToken);

		if (iToken==2) {

			strTrim(arFld[0]); strTrim(arFld[1]); 

			_(sEle);
			sEle.pszName=strDup(arFld[0]);
			sEle.utfValue=strDup(arFld[1]);
			sEle.pwcValue=utfToWcs(sEle.utfValue);
//			if (!strEmpty(sEle.utfValue)) sEle.iError=true;
			lstPush(pCss->lstEle,&sEle);
		}
		ARDestroy(arFld);
	}
	ehFree(arRow);
	return pCss;
}

//
// cssDestroy()
//
EH_CSS * cssDestroy(EH_CSS * psCss) {

	S_CSS_ELE * psEle;
	if (psCss) {
		for (lstLoop(psCss->lstEle,psEle)) {
			ehFreePtrs(3,&psEle->pszName,&psEle->utfValue,&psEle->pwcValue);
		}
		lstDestroy(psCss->lstEle);
		ehFree(psCss);
	}
	return NULL;

}

//
// cssGet()
//
UTF8 * cssGet(EH_CSS * psCss, CHAR * pszName) {

	S_CSS_ELE * psEle;
	if (psCss) {
		for (lstLoop(psCss->lstEle,psEle)) {
			if (!strcmp(psEle->pszName,pszName)) return psEle->utfValue;
		}
	}
	return NULL;

}


//
// cssFont()
//
BOOL cssFont(CHAR * pszText,EH_FONT ** ppsFont) {

//	INT iCount;
	DWORD dwStyle,dwHeight;
	CHAR szFont[200];
	EH_FONT * psFont=NULL;
	BOOL bRet=false;
	EH_AR ar;
	CHAR * psz;
	CHAR * pszWeight=NULL;
	CHAR * pszHeight=NULL;
	CHAR * pszFonts=NULL;
	INT a;

	while (strReplace(pszText,"  "," "));
	
	// Peso
	psz=strstr(pszText," "); if (psz) {pszWeight=strTake(pszText,psz-1); pszText=psz+1;} else pszText="";
	
	// Altezza
	psz=strstr(pszText," "); if (psz) {pszHeight=strTake(pszText,psz-1); pszText=psz+1;} else pszText="";

	// Fonts
	if (pszText) {pszFonts=strDup(pszText);}

	if (pszWeight&&pszHeight&&pszFonts) {
	
		dwStyle=STYLE_NORMAL;
		if (!strcmp(pszWeight,"normal")) dwStyle=STYLE_NORMAL;
		else if (!strcmp(pszWeight,"bold")) dwStyle=STYLE_BOLD;
		else if (!strcmp(pszWeight,"italic")) dwStyle=STYLE_ITALIC;

		dwHeight=atoi(strKeep(pszHeight,"0123456789"));
		ar=strSplit(pszFonts,",");
		for (a=0;ar[a];a++) {
		
			sprintf(szFont,"#%s",ar[0]);
			psFont=fontCreate(szFont,dwHeight,dwStyle,true,NULL,NULL);
			if (psFont) {
			
				if (*ppsFont) fontDestroy(*ppsFont,true);
				*ppsFont=psFont;
				bRet=true;
				break;
			}
		}
		ehFree(ar);

	}
	ehFreePtrs(3,&pszWeight,&pszHeight,&pszFonts);
	return bRet;
	
}

//
// cssAlign()
//
EN_DPL	cssAlign(CHAR * pszAlign) {

	EN_DPL enAlign=DPL_LEFT;
	if (!strcmp(pszAlign,"center")) enAlign=DPL_CENTER;
	else if (!strcmp(pszAlign,"right")) enAlign=DPL_RIGHT;
	else if (!strcmp(pszAlign,"justify")) enAlign=DPL_JUSTIFY;
	return enAlign;
}

//
// cssBool()
//
BOOL	cssBool(CHAR * pszAlign) {
	
	if (!strcmp(pszAlign,"true")) 
		return true; 
		else 
		return false;
}

//
// cssVisibility()
//
BOOL	cssVisibility(CHAR * pszVisi) {

	BOOL bVisible=false;
	if (!strcmp(pszVisi,"visible")) bVisible=true;
	else if (!strcmp(pszVisi,"hidden")) bVisible=false;
	return bVisible;
}

//
// cssAssign()
//
BOOL	cssAssign(EH_CSS *	pCss,
				  CHAR *	pszName,
				  CHAR *	pszType,
				  void *	pVoid) {
	
	CHAR * pszValue;
	pszValue=cssGet(pCss,pszName); if (!pszValue) return false;

	if (!strcmp(pszType,"bool")) {

		BOOL * pbBool=pVoid;
		*pbBool=cssBool(pszValue);

	}
	else if (!strcmp(pszType,"int32")) {

		INT * pbInt=pVoid;
		*pbInt=atoi(pszValue);

	}
	else if (!strcmp(pszType,"double")) {

		double * pbDub=pVoid;
		*pbDub=atof(pszValue);

	}
	else if (!strcmp(pszType,"char")) {

		CHAR * psz=pVoid;
		strcpy(psz,pszValue);

	}
	else if (!strcmp(pszType,"char*")) {

		CHAR ** psz=pVoid;
		*psz=strDup(pszValue);

	}
	else if (!strcmp(pszType,"align")) {

		EN_DPL * pDpl=pVoid;
		*pDpl=cssAlign(pszValue);

	}

	else if (!strcmp(pszType,"font")) {

		EH_FONT ** ppsFont=pVoid;
		cssFont(pszValue,ppsFont);

	}
	else if (!strcmp(pszType,"visibility")) {

		BOOL * pbBool=pVoid;
		*pbBool=cssVisibility(pszValue);

	}


	else 
		ehExit("cssAssign: [%s] ?",pszType);

	return true;
}

//
// cssAssignWidth()
//
BOOL	cssAssignWidth(EH_CSS * pCss, CHAR * pszName,double * pdWidth, EN_CSS_WIDTH * pEnWidth) {

	CHAR * pszValue;
	CHAR szServ[200];
	double dWidth;
	pszValue=cssGet(pCss,pszName); if (!pszValue) {*pEnWidth=CSW_UNDEFINED; return false;}

	strCpy(szServ,pszValue,sizeof(szServ));
	strcpy(szServ,strKeep(szServ,"0123456789."));
	dWidth=atoi(szServ);
	*pdWidth=dWidth;
	if (strstr(pszValue,"px")) {
		*pEnWidth=CSW_PIXEL;
	} 
	else if (strstr(pszValue,"%")) {
		*pEnWidth=CSW_PERC;
	} else {
		*pEnWidth=CSW_PIXEL; // Default
	}
	return true;


}
