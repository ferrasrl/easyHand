//   ---------------------------------------------
//   | DYNAFILTER Filtro dinamico             	 
//   | Permette l'inserimento ed il controllo                                          
//   | di un filtro su multicampi e con comparazione                                          
//   | multiple con caratteri jolly                                          
//   |                                           
//   |                                           
//   |             by Ferr… Art & Technology 2001
//   ---------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/DynaFilter.h"
#include "/easyhand/ehtool/main/armaker.h"

static SINT DbDynaMake(DBDYNA *lpDbDyna,CHAR *Filtro);
static BOOL DbDynaCheck(DBDYNA *lpDbDyna,HDB hdb);
BOOL DbDynaFilter(SINT cmd,LONG info,void *ptr)
{
 static DRVMEMOINFO dmiFilter=DMIRESET;
 DBDYNA sDbDyna;
 DBDYNASET *lpDynaSet;
 SINT a;
 BOOL bRet=FALSE;

 switch (cmd)
 {
	case WS_OPEN:
		DMIOpen(&dmiFilter,M_HEAP,info,sizeof(DBDYNA),"DynaFilter");
		break; 

	// Aggiunge una nuova voce dinamica
	case WS_ADD: 

		lpDynaSet=ptr;
		if (strEmpty(lpDynaSet->lpValue)) break;
		ZeroFill(sDbDyna);
		strcpy(sDbDyna.szFieldName,lpDynaSet->lpFieldName);
		sDbDyna.fBlobField=lpDynaSet->fBlobField;
		sDbDyna.iOperator=lpDynaSet->iOperator;
		sDbDyna.iPriority=lpDynaSet->iPriority;
		if (DbDynaMake(&sDbDyna,lpDynaSet->lpValue)) DMIAppend(&dmiFilter,&sDbDyna);
		break;

	// Chiude tutte le memorie impegnate
	case WS_CLOSE:
		for (a=0;a<dmiFilter.Num;a++)
		{
			DMIRead(&dmiFilter,a,&sDbDyna);
			memoFree(sDbDyna.Hdl,"Dyna");
		}
		DMIClose(&dmiFilter,"DynaFilter");
		break;

	case WS_PROCESS:
		for (a=0;a<dmiFilter.Num;a++)
		{
			DMIRead(&dmiFilter,a,&sDbDyna);
			if (DbDynaCheck(&sDbDyna,(HDB) info)) {bRet=TRUE; break;}
		}
		break;
 }
 return bRet;
}
// +-----------------------------------------+
// | DbDynaMake
// | Costruisce l'array con le voci divise
// |                                         
// |         Ferrà Art & Technology (c) 2001
// +-----------------------------------------+
static SINT DbDynaMake(DBDYNA *lpDbDyna,CHAR *Filtro)
{
 CHAR *lpBuffer,*p;
 CHAR *Copia;

 // Nessun filtro
 if (!*Filtro) return FALSE; 

 lpBuffer=malloc(1024);
 Copia=lpBuffer;
 strcpy(Copia,Filtro);
 ARMaker(WS_OPEN,NULL);
 lpDbDyna->iNum=0;
 while (TRUE)
 {
	p=strstr(Copia,","); if (!p) break;
	*p=0; 
	ARMaker(WS_ADD,Copia); lpDbDyna->iNum++; 
	p++; Copia=p;
 }

 ARMaker(WS_ADD,Copia); lpDbDyna->iNum++; 
 lpDbDyna->Hdl=ARMaker(WS_CLOSE,"DYNA");
 lpDbDyna->lppArray=memoPtr(lpDbDyna->Hdl,NULL);
// win_infoarg("%d [%s]",lpDbDyna->iNum,lpDbDyna->lppArray[0]);
 free(lpBuffer);
 return TRUE;
}


// +-----------------------------------------+
// | DbMultiCheck                            |
// |                                         |
// |         Ferrà Art & Technology (c) 1996 |
// |         Ferrà Art & Technology (c) 2001 |
// |                                         |
// |                                         |
// |  ritorna true=BUONO                     |
// |          false=NO BUONO                 |
// +-----------------------------------------+
static BOOL DbDynaCheck(DBDYNA *lpDbDyna,HDB hdb)
{
 SINT a;
 SINT Test=OFF;
 BOOL bEndWith;
 BOOL bBeginWith;
 CHAR *lpFieldFocus;
 CHAR szServ[80];
 
 CHAR *pControllo=NULL;//adb_FldPtr(hdb,lpDbDyna->szField);
 CHAR *lpCompare;
 CHAR *pa;

 // Tutto in maiuscolo
 if (!lpDbDyna->fBlobField)
	pControllo=adb_FldPtr(hdb,lpDbDyna->szFieldName);
	else
	pControllo=adb_BlobGetAlloc(hdb,lpDbDyna->szFieldName);
 _strupr(pControllo); 
 lpCompare=pControllo;

 for (a=0;a<lpDbDyna->iNum;a++)
 {
	 strcpy(szServ,lpDbDyna->lppArray[a]);
	 lpFieldFocus=szServ;
			
	 while (TRUE)
	 {
		// Controllo se deve iniziare per qualcosa 
		if (*lpFieldFocus=='*') 
		{
			lpFieldFocus++;
			pa=strstr(lpFieldFocus,"*"); 
			bBeginWith=FALSE;
			if (pa) {*pa=0;bEndWith=FALSE;} else bEndWith=TRUE;
		}
		else
		{   
			bEndWith=FALSE;
			pa=strstr(lpFieldFocus,"*"); 
			if (pa) {*pa=0;bBeginWith=TRUE;} else bBeginWith=FALSE;
		}

		Test=FALSE;

		// *Object Ricerca di "ognicosa" + l'oggetto

		if (pa&&!bBeginWith&&!bEndWith)
		{
			CHAR *p;
//			win_infoarg("%s %d %d [%s]",lpCompare,bBeginWith,bEndWith,lpFieldFocus);
			p=strstr(lpCompare,lpFieldFocus);
			if (p) {Test=TRUE; lpCompare=p+strlen(lpFieldFocus);}
		}	

		// E un tipo "End with"
		if (!bBeginWith&&bEndWith)
		{
			if (!*lpFieldFocus) Test=TRUE;
			else
			{
			if (!memcmp(lpCompare+strlen(lpCompare)-strlen(lpFieldFocus),lpFieldFocus,strlen(lpFieldFocus))) Test=TRUE; 
			}
		}

		// E un tipo "Begin with"
		if (bBeginWith&&!bEndWith)
		{
			if (!memcmp(lpCompare,lpFieldFocus,strlen(lpFieldFocus))) 
			{Test=TRUE; lpCompare+=strlen(lpFieldFocus);}
		}
			
		// =Object Ricerca di equaglianza con l'oggetto
		if (!pa&&!bBeginWith&&!bEndWith)
		{
			if (!strcmp(lpCompare,lpFieldFocus)) Test=TRUE; 
		}

		if (!Test) break; // No buono = mi fermo
		if (!pa) break;
		// Avanzo
		*pa='*'; lpFieldFocus=pa;	
	 }
	if (Test) break; // Se è buono mi fermo di ricercare
 }

 if (!lpDbDyna->fBlobField) adb_BlobFree(pControllo);

 if (Test==OFF) return TRUE; // Non buono
 return FALSE;
}
