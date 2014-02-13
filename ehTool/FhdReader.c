//  ---------------------------------------------
//    FhdReader
//    Legge i file Fhd          
//                            
//   							by Ferrà srl 2007
//  ---------------------------------------------
#include "\ehtool\include\ehsw_idb.h"
#include "\ehtool\FhdServer.h"

FHD_PROPERTY sFhdProperty;

static CHAR *FHDDecoding(BYTE *lp);

SINT FhdReader(SINT cmd,SINT info,void *str)
{
	static DRVMEMOINFO FhdElementInfo={-1,-1,-1,-1};
	CHAR *lpBuffer=NULL;
	BOOL fTab=FALSE;
	CHAR *p;
	FHDELEMENT FhdElement;
	static SINT iEnumCounter=0;
	SINT i,iMaxItems;
	BYTE *pFile;
	EH_AR arItem;
	CHAR szOldCode[400];

	switch (cmd)
	{
		// ---------------------------------
		// Apro FHD
		// ---------------------------------

		case WS_OPEN: // Carico le Classi/tabelle di un dbase
			
			// Carico il file FHD in memoria
			iEnumCounter=1;
			FhdElementInfo.Num=0;
			ZeroFill(sFhdProperty);
			if (str==NULL) break;
			pFile=FileToString(str); if (!pFile) break;
			arItem=ARCreate(pFile,CRLF,&iMaxItems);
			DMIOpen(&FhdElementInfo,RAM_AUTO,iMaxItems,sizeof(FHDELEMENT),"FhdElement");

			ZeroFill(sFhdProperty);
			strcpy(sFhdProperty.szFileSource,(CHAR *) str);

			//
			// Alloco memoria
			// DMIOpen(&FhdElementInfo,RAM_AUTO,FHDMAXRECORD,sizeof(FhdElement),"FhdElement");
			//
			for (i=0;arItem[i];i++)
			{
				BYTE *pItem=arItem[i];
				if (!strcmp(arItem[i],"#TabEnd")) fTab=FALSE;
				if (!stribegin(arItem[i],"#Ver")) sFhdProperty.iVersion=(SINT) (atof(pItem+5)*100);
				if (!stribegin(arItem[i],"#Name")) strcpy(sFhdProperty.szFHDName,pItem+6);
				if (!stribegin(arItem[i],"#DSN")) strcpy(sFhdProperty.szDSN,pItem+5);
				if (!stribegin(arItem[i],"#User")) strcpy(sFhdProperty.szUser,pItem+6);
				if (!stribegin(arItem[i],"#Pass")) strcpy(sFhdProperty.szPass,pItem+6);
				if (!stribegin(arItem[i],"#Library")) strcpy(sFhdProperty.szLibrary,pItem+9);
				if (!stribegin(arItem[i],"#Platform")) sFhdProperty.iDefaultPlatform=atoi(pItem+10);
				if (!stribegin(arItem[i],"#Version")) strcpy(sFhdProperty.szVersion,pItem+9);
					
				// Alloco tabella e classi
				if (fTab)
				{
					//win_infoarg("[%s]",Buf);
					lpBuffer=FHDDecoding(pItem);

					if (lpBuffer[1]=='C') // Classe
					{
						ZeroFill(FhdElement);
						FhdElement.iType=FHD_FOLDER;
						FhdElement.idAutocode=iEnumCounter++;
						strcpy(FhdElement.szName,lpBuffer+3);
						DMIAppend(&FhdElementInfo,&FhdElement);
					}

					if (lpBuffer[1]=='T') // Table
					{
						 ZeroFill(FhdElement);
						 FhdElement.iType=FHD_TABLE;
						 p=strstr(lpBuffer+3,","); *p=0;
						 strcpy(szOldCode,lpBuffer+3);
						 strcpy(FhdElement.szName,p+1);
						 FhdElement.szName[0]=(CHAR) toupper(FhdElement.szName[0]);
						 FhdElement.idAutocode=iEnumCounter++;
						 DMIAppend(&FhdElementInfo,&FhdElement);
					}

					if (lpBuffer[1]=='F') // Field
					{
						 ZeroFill(FhdElement);
						 FhdElement.iType=FHD_FIELD;
						 p=strstr(lpBuffer+3,","); *p=0;
						 strcpy(FhdElement.szName,lpBuffer+3); // Nome del Campo
						 strcpy(szOldCode,p+1);   // Descrizione del Campo
						 FhdElement.szName[0]=(CHAR) toupper(FhdElement.szName[0]);
						 FhdElement.idAutocode=iEnumCounter++;
						 
						 DMIAppend(&FhdElementInfo,&FhdElement);
					}

					if (lpBuffer[1]=='I') // Indici
					{
						 ZeroFill(FhdElement);
						 //>F,Codice,ALFA,30,0,No,NULL
						 //>F,Descrizione,ALFA,50,0,No,NULL
						 
						 FhdElement.iType=FHD_INDEX;
						 p=strstr(lpBuffer+3,","); *p=0;
						 strcpy(FhdElement.szName,lpBuffer+3); // Nome del Campo
						 strcpy(szOldCode,p+1);   // Descrizione del Campo
						 FhdElement.szName[0]=(CHAR) toupper(FhdElement.szName[0]);
						 FhdElement.idAutocode=iEnumCounter++;
						 DMIAppend(&FhdElementInfo,&FhdElement);
					}
					free(lpBuffer);
				}
				if (!strcmp(pItem,"#TabStart")) fTab=TRUE;
			}
			ARDestroy(arItem);
			return 1;
			break;

		case WS_CLOSE: 
			DMIClose(&FhdElementInfo,"FhdElement");
			break;

		case WS_COUNT: // Restituisce il numero di tabelle presenti
			return FhdElementInfo.Num;
			//break;

		case WS_REALGET: // Legge i dati di una tabella
			if (info==FHD_GET_NAME) return (LONG) sFhdProperty.szFHDName; 
			if (info==FHD_GET_FILE) return (LONG) sFhdProperty.szFileSource; 
			if (info==FHD_GET_LAST) info=FhdElementInfo.Num-1;
			if (info==FHD_GET_DSN)  return (LONG) sFhdProperty.szDSN; 
			if (info==FHD_GET_USER) return (LONG) sFhdProperty.szUser; 
			if (info==FHD_GET_PASS) return (LONG) sFhdProperty.szPass; 
			if (info==FHD_GET_LIB) return (LONG) sFhdProperty.szLibrary; 
			if (info==FHD_GET_PLATFORM) return sFhdProperty.iDefaultPlatform; 
			if (info==FHD_GET_VERSION) return (LONG) sFhdProperty.szVersion; 
			if (info>FhdElementInfo.Num) return -1;
			DMIRead(&FhdElementInfo,info,str);
			break;
	}
  return 0;
}

static CHAR *FHDDecoding(BYTE *lp)
{
	BYTE *lpn=malloc(5000);
	CHAR szServ[20];
	*lpn=0;
	for (;*lp;lp++)
	{
		if (*lp=='{')
		{
			CHAR *lp2;
			lp2=strstr(lp,"}"); if (!lp2) break; // File corrotto
			*lp2=0;
			sprintf(szServ,"%c",(BYTE) atoi(lp+1));
			lp=lp2;
		}
		else 
		{
			//if (*lp=='\1') *lp='|';
			sprintf(szServ,"%c",*lp);
		}
		strcat(lpn,szServ);
	}
	return lpn;
}

