//   ---------------------------------------------
//   | LastDoc.c Gestisce una coda do dodumenti  
//   |           Utile per avere la lista        
//   |           degli ultimi file/documenti     
//   |           usati.                          
//   |
//   |             by Ferrà Art & Tecnology 2000 
//   ---------------------------------------------
#include "/easyhand/inc/easyhand.h"

#include "/easyhand/ehtool/LastDoc.h"

// LastFileDocument Server
SINT LFDServer(DRVMEMOINFO *LFDInfo,SINT cmd,LONG info,void *str)
{
	CHAR TheDoc[MAXPATH];
	CHAR Serv[80];
	SINT a;
	BOOL fTab=FALSE;
	BYTE *lp;
	BOOL fFind;

	static CHAR *lpSuffix=NULL;

	switch (cmd)
	{
		// ---------------------------------
		// Apro una gestione di ultimi documenti
		// --------------------------------
		case WS_OPEN: // Carico le Classi/tabelle di un dbase
			
			// Carico il file FHD in memoria
			if (LFDInfo->Hdl>0) LFDServer(LFDInfo,WS_CLOSE,0,NULL);
			LFDInfo->Num=0;
			DMIOpen(LFDInfo,RAM_AUTO,info,MAXPATH,str);
			break;

		case WS_CLOSE: 
			if (info)
			{
				for (a=0;a<LFDInfo->Num;a++)
				{
					sprintf(Serv,"%s%02d",lpSuffix,a);		 
					DMIRead(LFDInfo,a,TheDoc);
					iniSet(str,Serv,TheDoc);
				}
			}
			DMIClose(LFDInfo,str);
			break;

		case WS_LINK:
			lpSuffix=str;
			break;
		
		// Carico i dati da un file .ini
		case WS_LOAD:
			for (a=0;a<LFDInfo->Max;a++)
			{
				sprintf(Serv,"%s%02d",lpSuffix,a);		 
				if (!iniGet(str,Serv,TheDoc,sizeof(TheDoc))) DMIAppend(LFDInfo,TheDoc);
			}
			break;

		case WS_COUNT: // Restituisce il numero di tabelle presenti
			return LFDInfo->Num;
			//break;

		case WS_REALGET: // Legge i dati di una tabella
			DMIRead(LFDInfo,info,str);
			break;

		case WS_REALSET: // Legge i dati di una tabella
			DMIWrite(LFDInfo,info,str);
			break;
		
		case WS_DEL:
			DMIDelete(LFDInfo,info,TheDoc);
			break;

		case WS_INSERT:
			
			if (info<LFDInfo->Num) 
			{DMIInsert(LFDInfo,info,TheDoc);
			 DMIWrite(LFDInfo,info,str);
			}
			else
			{
			 DMIAppend(LFDInfo,str);
			}
			break;
		
		// Aggiunge una voce alla lista degli ultimi documenti
		case WS_ADD: 

			// Cerco se esiste già
			lp=memoLock(LFDInfo->Hdl);
			fFind=FALSE;
			for (a=0;a<LFDInfo->Num;a++)
			{
				DMIRead(LFDInfo,a,TheDoc);
				if (!strcmp(TheDoc,str))
				{
				 //memmove(&LastJob[a],&LastJob[a+1],(LastJobNum-a)*sizeof(LASTJOB)); 
				 //memmove(&LastJob[1],&LastJob[0],(LastJobNum-1)*sizeof(LASTJOB));

				 // Cancello dove era
				 memmove(lp+(a*LFDInfo->Size),lp+((a+1)*LFDInfo->Size),(LFDInfo->Num-a)*LFDInfo->Size); 
				 // Scroll giù di uno la lista
				 memmove(lp+LFDInfo->Size,lp,(LFDInfo->Num-1)*LFDInfo->Size); 
				 fFind=TRUE;
				 //LastJobNum--;
				}
			}

			// Non l'ho trovato
			strcpy(TheDoc,str); 
			if (!fFind)
			{
				// Se è piena scrollo giù
				if (LFDInfo->Num>=LFDInfo->Max)
				{
				 //memmove(&LastJob[1],&LastJob[0],(MAXLASTJOB-1)*sizeof(LASTJOB));
				 memmove(lp+LFDInfo->Size,lp,(LFDInfo->Num-1)*LFDInfo->Size); 
				 DMIWrite(LFDInfo,0,TheDoc);
				} 
				else DMIAppend(LFDInfo,TheDoc);
			}
			else DMIWrite(LFDInfo,0,TheDoc);
			memoUnlock(LFDInfo->Hdl);
			break;

		case WS_FIND: // Ricerca
			break;
	}
  return 0;
}

