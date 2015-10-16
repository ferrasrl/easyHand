//   +-------------------------------------------+
//   | ARMaker  ArrayMaker                       
//   |          Costruttore di Array             
//   |                                           
//   |          cmd         ptr                  
//   |          WS_OPEN     SINT * (len record)  
//   |          WS_ADD      ->Record             
//   |                                           
//   |          WS_CLOSE    "Nome lista memo"    
//   |          ritorna Hdl della lista          
//   |                                           
//   |             4 Maggio                      
//   |             by Ferrà Art & Technology 1999 
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h" 
#include "/easyhand/ehtool/main/armaker.h"

BYTE * lpRecords=NULL;

#ifndef EH_MEMO_DEBUG
SINT ARMaker(SINT cmd,void *ptr)
#else
SINT _ARMaker(SINT cmd,void *ptr,CHAR *pFile,SINT iLine)
#endif
{
	static S_ARMAKER sArMaker;
#ifndef EH_MEMO_DEBUG
	return ARMakerEx(cmd,ptr,&sArMaker);
#else
	return _ARMakerEx(cmd,ptr,&sArMaker,pFile,iLine);
#endif
}

static int _SortAsc(const void *ptr1,const void *ptr2)
{
	LONG l1=* (LONG *) ptr1;
	LONG l2=* (LONG *) ptr2;
	if (!lpRecords) ehExit("_SortAsc()");
	// -- printf("%s>%s\n",lpRecords+l1,lpRecords+l2);
	return strcmp(lpRecords+l1,lpRecords+l2);
}

#define PTRSIZE sizeof(void *)
void * ARPtrMakerEx(SINT cmd,void *ptr,S_ARMAKER *lpsArMaker)
{
	void *ar;
	switch (cmd)
	{
	 // Alloco memoria per l'array di puntatori
	 case WS_OPEN:
			memset(lpsArMaker,0,sizeof(S_ARMAKER));
			lpsArMaker->bOpen=TRUE;
			break;

	 case WS_ADD:
			if (!lpsArMaker->bOpen) ehExit("ARMaker not Open");
			if (!ptr&&!lpsArMaker->bReady) ehExit("ARMAKER WS_ADD ptr =NULL");

			if (!lpsArMaker->uiListCount) 
			{
				lpsArMaker->array=ehAlloc(PTRSIZE);
			} 
			else 
			{
				lpsArMaker->array=ehRealloc(lpsArMaker->array,
											PTRSIZE*lpsArMaker->uiListCount,
											PTRSIZE*(lpsArMaker->uiListCount+1)); 
				if (!lpsArMaker->array) ehExit("ARMAKER:Errore in realloc");
			}

			if (ptr==NULL) lpsArMaker->array[lpsArMaker->uiListCount]=0; 
						   else 
						   lpsArMaker->array[lpsArMaker->uiListCount]=ptr;
			lpsArMaker->uiListCount++;
			break;

	 case WS_CLOSE: 
			lpsArMaker->bReady=TRUE; // Serve per permettere di aggiungere un NULL
			ARPtrMakerEx(WS_ADD,NULL,lpsArMaker);
			ar=ehAlloc(PTRSIZE*lpsArMaker->uiListCount);
			memcpy(ar,lpsArMaker->array,PTRSIZE*lpsArMaker->uiListCount);
			ehFree(lpsArMaker->array);
			lpsArMaker->array=NULL;
			memset(lpsArMaker,0,sizeof(S_ARMAKER));
			return ar;

	}
	return NULL;
}

void *ARPtrMaker(SINT cmd,void *ptr)
{
	static S_ARMAKER sArMaker;
	return ARPtrMakerEx(cmd,ptr,&sArMaker);
}
#ifndef EH_MEMO_DEBUG
SINT ARMakerEx(SINT cmd,void *ptr,S_ARMAKER *lpsArMaker)
#else
SINT _ARMakerEx(SINT cmd,void *ptr,S_ARMAKER *lpsArMaker,BYTE *pProg,SINT iLine)
#endif
{
	SINT Len;
	SINT hdl;
	SINT a;

	CHAR *lpDest;
	void **lpPTDest;
	BOOL bRetPointer;

	switch (cmd)
	{
	 case WS_OPEN:
				memset(lpsArMaker,0,sizeof(S_ARMAKER));
				//if ((lpOffsets!=NULL)||(lpRecords!=NULL)) ehExit("AR0");
				if (ptr) lpsArMaker->lRecordSize=* (SINT *) ptr; else lpsArMaker->lRecordSize=0;
				lpsArMaker->lpOffsets=NULL;
				lpsArMaker->uiListPtr=0;
				lpsArMaker->uiListRec=0;
				lpsArMaker->uiListCount=0;
				lpsArMaker->bReady=FALSE;
				lpsArMaker->bOpen=TRUE;
				break;

	 case WS_ADD:
				if (!lpsArMaker->bOpen) ehExit("ARMAKER Not Open");
				if (!ptr&&!lpsArMaker->bReady) ehExit("ARMAKER WS_ADD ptr =NULL");
				if (ptr!=NULL)
				{
				 if (!lpsArMaker->lRecordSize) Len=strlen(ptr)+1; else Len=(UINT) lpsArMaker->lRecordSize;
				 if (Len<1) ehExit("AR1");
                }

				if (ptr!=NULL)
				{
				// Archiviato il nuovo record
				if (!lpsArMaker->uiListRec) 
						{lpsArMaker->lpRecords=ehAlloc(Len);
#ifdef EH_MEMO_DEBUG 
						 memDebugUpdate(lpsArMaker->lpRecords,pProg,iLine);
#endif
						 lpDest=lpsArMaker->lpRecords;
						}
						else
						{
							lpsArMaker->lpRecords=ehRealloc(lpsArMaker->lpRecords,(UINT) lpsArMaker->uiListRec,(UINT) (lpsArMaker->uiListRec+Len));
							lpDest=lpsArMaker->lpRecords+lpsArMaker->uiListRec;
						}
				memcpy(lpDest,ptr,Len);
				} else {lpDest=NULL;Len=0;}

				if (!lpsArMaker->uiListCount) 
					{
					  lpsArMaker->lpOffsets=ehAlloc(LNSIZE);
#ifdef EH_MEMO_DEBUG
					  memDebugUpdate(lpsArMaker->lpOffsets,pProg,iLine);
#endif
					} 
					else 
					{
					 lpsArMaker->lpOffsets=ehRealloc(lpsArMaker->lpOffsets,(UINT) LNSIZE*lpsArMaker->uiListCount,(UINT) LNSIZE*(lpsArMaker->uiListCount+1)); 
					 if (!lpsArMaker->lpOffsets) ehExit("ARMAKER:Errore in realloc");
					}
				if (ptr==NULL) lpsArMaker->lpOffsets[lpsArMaker->uiListCount]=-1; else lpsArMaker->lpOffsets[lpsArMaker->uiListCount]=lpsArMaker->uiListRec;
				lpsArMaker->uiListRec+=Len;
				lpsArMaker->uiListPtr+=sizeof(void *);
				lpsArMaker->uiListCount++;
				break;

	 // Ordina l'array
	 case WS_PROCESS:
				lpRecords=lpsArMaker->lpRecords;
				if (ptr) qsort(lpsArMaker->lpOffsets,lpsArMaker->uiListCount-1,sizeof(LONG),ptr);
						 else
						 qsort(lpsArMaker->lpOffsets,lpsArMaker->uiListCount-1,sizeof(LONG),_SortAsc);
				break;

	 case WS_CLOSE:

				bRetPointer=FALSE;
				if (!ptr) {ptr="Temp"; bRetPointer=TRUE;}
				lpsArMaker->bReady=TRUE; // Serve per permettere di aggiungere un NULL
				ARMakerEx(WS_ADD,NULL,lpsArMaker);
				hdl=memoAlloc(M_HEAP,lpsArMaker->uiListPtr+lpsArMaker->uiListRec,ptr);
				
				// Sposto nella nuova memoria i puntatori
				lpDest=(BYTE *) memoPtr(hdl,NULL)+lpsArMaker->uiListPtr;
				if (lpsArMaker->uiListRec) memcpy(lpDest,lpsArMaker->lpRecords,lpsArMaker->uiListRec);

                // Rialloco i Puntatori a Puntatori
				lpPTDest=(void **) memoPtr(hdl,NULL);
				for (a=0;a<(SINT) lpsArMaker->uiListCount;a++)
				{
				 if (lpsArMaker->lpOffsets[a]==-1) lpPTDest[a]=NULL; else lpPTDest[a]=(BYTE *) lpDest+lpsArMaker->lpOffsets[a];
				}
				if (lpsArMaker->lpOffsets) ehFree(lpsArMaker->lpOffsets);
				if (lpsArMaker->lpRecords) ehFree(lpsArMaker->lpRecords);

				memset(lpsArMaker,0,sizeof(S_ARMAKER));
				if (bRetPointer) return (SINT) ARFHdlToPtr(hdl);
				
				return hdl;

	}
 return 0;
}

EH_ARF ARFHdlToPtr(SINT hdl) // Uso interno
{
	CHAR **lpRet;
	BYTE *lpOldPtr;
	DWORD dwSize;
	SINT a;

	lpOldPtr=memoPtr(hdl,NULL);
	dwSize=sys.arMemoElement[hdl].dwSize;
	lpRet=ehAlloc(dwSize); 
	printf("dwSize:%d:%d",hdl,dwSize);
	memcpy(lpRet,lpOldPtr,dwSize);
	memoFree(hdl,"Temp");
	
	// Rialloco i puntatori nella nuova zona
	for (a=0;lpRet[a];a++)
	{
		lpRet[a]=(BYTE *) lpRet+(lpRet[a]-lpOldPtr);
	}

	return lpRet;
}
/*
//
// ARFClone() - Clona un array ed allocamemoria per contenerlo
//
EH_ARF ARFClone(EH_ARF ptr,CHAR *lpEnd) // Ex ARClone
{
	SINT a,hdl;
	
	ARMaker(WS_OPEN,NULL);
	for (a=0;ptr[a];a++)
	{
		if (lpEnd)
		{
			if (!strcmp(ptr[a],lpEnd)) break;
		}
		ARMaker(WS_ADD,ptr[a]);  
	}
	hdl=ARMaker(WS_CLOSE,"Temp"); if (hdl<0) ehExit("ARFSplit()");
	return ARFHdlToPtr(hdl);
}
*/





//
// ARFDistinct()
//
EH_ARF ARFDistinct(EH_ARF ar,BOOL fCaseInsensible) // Ex ARDistinct
{
	SINT a,i;
	S_ARMAKER sArMaker;
	BOOL bFound;

	if (!ar) ehExit("errore NULL");
	ARMakerEx(WS_OPEN,NULL,&sArMaker);

	for (a=0;ar[a];a++)
	{
		bFound=FALSE;
		for (i=0;i<a;i++)
		{
			if (fCaseInsensible)
			{
				if (!strCaseCmp(ar[a],ar[i])) {bFound=TRUE; break;}
			}
				else
			{
				if (!strcmp(ar[a],ar[i])) {bFound=TRUE; break;}
			}
		}
		if (!bFound) ARMakerEx(WS_ADD,ar[a],&sArMaker);
	}
	ehFree(ar);
	return ARFHdlToPtr(ARMakerEx(WS_CLOSE,"Nome",&sArMaker));
}




EH_ARF ARFClone(EH_ARF ptr,CHAR *lpEnd) // Ex ARFClone/ARClone
{
	SINT a,hdl;
	
	ARMaker(WS_OPEN,NULL);
	for (a=0;ptr[a];a++)
	{
		if (lpEnd)
		{
			if (!strcmp(ptr[a],lpEnd)) break;
		}
		ARMaker(WS_ADD,ptr[a]);  
	}
	hdl=ARMaker(WS_CLOSE,"Temp"); if (hdl<0) ehExit("ARFSplit()");
	return ARFHdlToPtr(hdl);
}
