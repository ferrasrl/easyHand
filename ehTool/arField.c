//   +-------------------------------------------+
//   | ArField
//   | Gestione Record Virtuali in array dinamico
//   |             
//   |             
//   |						29/11/2006 Ferrà Srl
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/arField.h"
 

static _DMI dmiField=DMIRESET; 

void ar_recreset(SINT idb)
{
	if (dmiField.Hdl>-1) ar_close(idb);
	if (dmiField.Hdl<1)
	{
	 DMIOpen(&dmiField,RAM_AUTO,100,sizeof(ARFIELD),"arField");
	 memoLock(dmiField.Hdl);
	}
}

void ar_close(SINT idb)
{
	SINT a;
	ARFIELD sField;
	for (a=0;a<dmiField.Num;a++)
	{
		DMIRead(&dmiField,a,&sField); if (idb>-1&&sField.idb!=idb) continue; 
		ehFreePtr(&sField.lpValore);
		if (idb>-1) {DMIDelete(&dmiField,a,&sField); a--;}
	}
	if (idb<0) DMIClose(&dmiField,"arField");
}

SINT ar_FldFind(SINT idb,CHAR *lpFieldName,ARFIELD *lpField)
{
	SINT a;
	if (dmiField.Size!=sizeof(ARFIELD)) ehExit("ar_(function) non inizializzato");
	for (a=0;a<dmiField.Num;a++)
	{
		DMIRead(&dmiField,a,lpField); if (lpField->idb!=idb) continue;
		if (!strcmp(lpField->szName,lpFieldName)) return a;
	}
	return -1;
}

SINT ar_FldInt(SINT idb,CHAR *lpFieldName)
{
	SINT idx;
	ARFIELD sField;
	idx=ar_FldFind(idb,lpFieldName,&sField);
	if (idx<0) ehExit("ar_FldInt(%s): Campo inesistente",lpFieldName);
	if (sField.iType!=NUME) ehExit("ar_FldInt(%s): Campo alfanumerico",lpFieldName);
	return (SINT) sField.dValore;
}

TCHAR * ar_FldPtr(SINT idb,CHAR *lpFieldName)
{
	SINT idx;
	ARFIELD sField;
	idx=ar_FldFind(idb,lpFieldName,&sField);
	if (idx>-1) return sField.lpValore; else return NULL;
}

void ar_FldWrite(SINT idb,CHAR *lpFieldName,TCHAR *lpString,double dVal)
{
	SINT idx;
	ARFIELD sField;
	SINT iType;
	
	idx=ar_FldFind(idb,lpFieldName,&sField);
	if (!lpString) iType=NUME; else iType=ALFA;

	// Inserimento
	if (idx<0) 
	{
		ZeroFill(sField);
		strcpy(sField.szName,lpFieldName);
		sField.iType=iType;
		sField.idb=idb;
		switch (sField.iType)
		{
			case NUME:  sField.lpValore=ehAlloc(80);
						sprintf(sField.lpValore,"%f",dVal);
						sField.dValore=dVal;
						break;
			
			case ALFA:  sField.lpValore=strDup(lpString);
						break;
		}
		DMIAppendDyn(&dmiField,&sField);
	}
	// Update
	else
	{
		if (sField.iType!=iType) ehExit("Campo %s scritto in tipo diverso [%s][%.2f]",lpFieldName,lpString,dVal);
		ehFreePtr(&sField.lpValore); 

		switch (sField.iType)
		{
			case NUME:  sField.lpValore=ehAlloc(80);
						sprintf(sField.lpValore,"%f",dVal);
						sField.dValore=dVal;
						break;
			
			case ALFA:  sField.lpValore=strDup(lpString);
						break;
		}
		DMIWrite(&dmiField,idx,&sField);
	}
}

#ifdef _MYSQL
TCHAR *ar_FldToQuery(SINT idb)
{
	SINT a,iSize;
	ARFIELD sField;
	CHAR *lpBuf=NULL;
	CHAR szServ[2800],*lp;

	iSize=0;
	for (a=0;a<dmiField.Num;a++)
	{
		DMIRead(&dmiField,a,&sField); if (sField.idb!=idb) continue;
		iSize+=strlen(sField.lpValore)+20+strlen(sField.szName);
	}
	lpBuf=ehAlloc(iSize); *lpBuf=0;

	for (a=0;a<dmiField.Num;a++)
	{
		DMIRead(&dmiField,a,&sField); if (sField.idb!=idb) continue;
		if (*lpBuf) strcat(lpBuf,",");
		sprintf(szServ,"%s=",sField.szName); strcat(lpBuf,szServ);
		switch (sField.iType)
		{
		   case NUME: 
					  if (sField.dValore==(SINT) sField.dValore)
						sprintf(szServ,"%d",(SINT) sField.dValore); 
						else
						sprintf(szServ,"%.3f",sField.dValore); 
					  strcat(lpBuf,szServ); 
					  break;

		   case ALFA: //strcpy(szServ,sField.lpValore);
					  lp=strEncode(sField.lpValore,SE_SQL,NULL);
					  strcpy(szServ,lp); ehFree(lp);
					  //mys_sqlstr(szServ,sizeof(szServ));
					  strcat(lpBuf,"'"); strcat(lpBuf,szServ); strcat(lpBuf,"'");
					  break;
		}
	}
	return lpBuf;
}
#endif

#ifdef _ADB32
void ar_FldToAdb(SINT idb,BOOL fAfterFree)
{
	SINT a;
	ARFIELD sField;
	for (a=0;a<dmiField.Num;a++)
	{
		DMIRead(&dmiField,a,&sField); if (sField.idb!=idb) continue;
		switch (sField.iType)
		{
		   case NUME: adb_FldWrite(idb,sField.szName,NULL,sField.dValore); break;
		   case ALFA: adb_FldWrite(idb,sField.szName,sField.lpValore,0); break;
		}
	}
	if (fAfterFree) ar_close(idb);
}
#endif
