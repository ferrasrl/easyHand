// ------------------------------------------------
//  ehPdf
//  Apre, modifica e scrive un PDF
//  
//                               by Ferrà srl 2011
// ------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehPdf.h"

#include "/easyhand/ehtoolx/Zlib/zlib.h"

static void			_pdfInflate(S_PDF * pszPdf);
static CHAR *		_objValueParser(S_PDF_OBJ * psObj, CHAR * pszValueStart, BYTE * pbEndSource);
static S_PDF_OBJ *	_objSearch(S_PDF * psPdf,S_PDF_OBJ * psObjStart,CHAR * pszKey,BOOL bCreate);
static CHAR *		_objDictExtract(S_PDF_OBJ * psObj,BYTE * pbStart,BYTE * pbEndSource);
static void			_objExpand(S_PDF_OBJ * psObj,BOOL bAll);
static void			_pageGoFree(S_PDF * psPdf);
static S_PDF_OBJ *	_objFree(S_PDF_OBJ * psObj,BOOL bSelf);
static BYTE *		_charJump(CHAR * pb);



//
//  _binSearch()
//
static BYTE *	_binSearch(BYTE * pbStart, BYTE * pbEndSource,BYTE * pbSearch,INT iSize) {

	BYTE * pb=pbStart;
	CHAR *  pbEnd;
	if (!iSize) iSize=strlen(pbSearch);
	pbEnd=pbEndSource-iSize;
	for (pb=pbStart;pb<pbEnd;pb++) {
		if (!memcmp(pb,pbSearch,iSize)) return pb;
	}
	return NULL;

}

static BYTE * _binSearchRev(S_PDF * psPdf,BYTE * pbStart, BYTE * pbSearch,INT iSize) {

	BYTE * pb=pbStart;
	if (!iSize) iSize=strlen(pbSearch);
	pbStart-=iSize;
	for (pb=pbStart;pb>=psPdf->pbSource;pb--) {
		if (!memcmp(pb,pbSearch,iSize)) return pb;
	}
	return NULL;
}


static BYTE * _deflate(BYTE * pbSource, SIZE_T tSize, SIZE_T * ptSizeRet) {
						
	z_stream sZstrm;
	INT iErr,iStep;
	#define CHUCK_SIZE 16384
	BYTE szChunk[CHUCK_SIZE];
	//BYTE * pbChunk=ehAllocZero(CHUCK_SIZE);
	DWORD dwSize=0;
	BYTE * pbDest=NULL;
	BYTE * pbRet=NULL;


	//
	// Inizializzo stream
	//
	_(szChunk);
	for (iStep=0;iStep<2;iStep++) {

		ZeroFill(sZstrm);
		iErr = inflateInit(&sZstrm);
		if (iErr != Z_OK) return NULL;

		if (iStep==1) {
			pbDest=pbRet=ehAllocZero(dwSize+1); 
//			psPart->enType=PDFT_DEFLATED;
		}
		sZstrm.avail_in = tSize;
		sZstrm.next_in = pbSource;

		//
		// Loop sul deflate
		//
		do {
				INT iRet;
				INT have;

				sZstrm.avail_out = CHUCK_SIZE;// (50*sZstrm.avail_in)+1;
				sZstrm.next_out = szChunk;

				iRet = inflate (&sZstrm, Z_FINISH); // 
				switch (iRet) {

						case Z_NEED_DICT:
							iRet = Z_DATA_ERROR;     /* and fall through */

						case Z_DATA_ERROR:
						case Z_MEM_ERROR:

							(void)inflateEnd(&sZstrm);
							ehExit("_deflate:Error %d",iRet);
							break;
	//										return ret;
					}

				have = CHUCK_SIZE - sZstrm.avail_out;
				switch (iStep) {
				
					case 0:
						dwSize+=have;
						break;

					case 1:
						memcpy(pbDest,szChunk,have);
						pbDest+=have;
						break;
				/*
					if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
						(void)inflateEnd(&strm);
						return Z_ERRNO;
				}
				*/
				
				}
		}  while (sZstrm.avail_out == 0);

		inflateEnd(&sZstrm);
	}

	if (ptSizeRet) * ptSizeRet=dwSize;
	return pbRet;
}



//
// pdfLoad()
//
S_PDF * pdfLoad(UTF8 * pszFileName) 
{
	BYTE * pbFile;//,* pbEndFile;
	BYTE * pbObj, * pbObjEnd, * pb;
	SIZE_T tSize;
	S_PDF * psPdf;
	S_PDFPART sPart;
	INT a;

	psPdf=ehNew(S_PDF);//ehAllocZero(sizeof(S_PDF));
	DMIReset(&psPdf->dmiPart);

	//
	// Apro il file
	//
	pbFile=fileMemoRead(pszFileName,&tSize); if (!pbFile) ehExit("Il file %s non esiste",pszFileName);
	pb=pbFile; 
	psPdf->pbSource=pb;
	psPdf->pbEndFile=pb+tSize-1;


	//
	// Preconto gli oggetti
	//
	for (a=0;;a++) {
	
		pbObj=_binSearch(pb,psPdf->pbEndFile," obj\r",4); if (!pbObj) break;
		pb=pbObj+4;

	}
	pb=pbFile;
	DMIOpen(&psPdf->dmiPart,RAM_AUTO,a+10,sizeof(S_PDFPART),"pdfPart");

	while (true) {

		BYTE * pbx;
		//
		// Cerco l'oggetto
		//
		pbObj=_binSearch(pb,psPdf->pbEndFile," obj\r",5); 

		//
		// Abbiamo finito
		//
		if (!pbObj) {
		
			//
			// Memorizzo la coda
			//
			_(sPart);
			sPart.tSize=((SIZE_T) psPdf->pbEndFile-(SIZE_T) pb);
			if (sPart.tSize) {
			
				sPart.enType=PDFT_NO_OBJ;
				sPart.pbPart=ehAlloc(sPart.tSize);
				ehMemCpy(sPart.pbPart,pb,sPart.tSize);
				DMIAppendDyn(&psPdf->dmiPart,&sPart);
				break;
			}
		
		}
		//
		// Trovo l'inizio dell'oggetto
		//
		pbx=_binSearchRev(psPdf,pbObj,"\r",1); 
		if (!pbx) 
			ehError(); // Errore
		pbObj=pbx;
		if (pbObj[1]==LF) pbObj+=2; else pbObj+=1;

		//
		// "Pezzo" sconosciuto tra un oggetto e l'altro
		//
		_(sPart);
		sPart.tSize=((SIZE_T) pbObj-(SIZE_T) pb);
		if (sPart.tSize) {
		
			sPart.enType=PDFT_NO_OBJ;
			sPart.pbPart=ehAlloc(sPart.tSize+1); sPart.pbPart[sPart.tSize]=0;
			ehMemCpy(sPart.pbPart,pb,sPart.tSize);
			DMIAppendDyn(&psPdf->dmiPart,&sPart);
		
		}


		//
		// Memorizza un oggetto
		//
		pbObjEnd=_binSearch(pbObj,psPdf->pbEndFile,"endobj\r",7); 
		if (!pbObjEnd) 
			ehError();
		pbObjEnd+=7;


		// L'oggetto
		_(sPart);
		sPart.enType=PDFT_OBJ_UNKNOW;
		sPart.tSize=((SIZE_T) pbObjEnd-(SIZE_T) pbObj);
		sPart.pbPart=ehAlloc(sPart.tSize+1); sPart.pbPart[sPart.tSize]=0;
		memcpy(sPart.pbPart,pbObj,sPart.tSize);
		DMIAppendDyn(&psPdf->dmiPart,&sPart);
		pb=pbObjEnd;

	}

	ehFree(pbFile);
	psPdf->arsPart=DMILock(&psPdf->dmiPart,NULL);

	//
	// PROCESSO GLI OGGETTI
	// fase di elaborazione dei dati nella DMI
	// ora decomprimo le stringhe in memoria e salto gli obj che non mi interessano
	//

	_pdfInflate(psPdf);

	return psPdf;
}

//
// _pdfInflate()
//
static void _pdfInflate(S_PDF * psPdf) {

	INT a;
	S_PDFPART * psPart;

	for (a=0;a<psPdf->dmiPart.Num;a++) {
		
		psPart=psPdf->arsPart+a;
		
		// se siamo in un testo compattato
		if (psPart->enType==PDFT_OBJ_UNKNOW) {
			CHAR * pszInfo=strExtract(psPart->pbPart,"<</",">>",false,false);

			if (pszInfo) {
				if (strstr(pszInfo,"/FlateDecode")) {

					// Testo
					if (!strBegin(pszInfo,"Filter")) {
			
						BYTE * pbStart, *pbEnd;
						SIZE_T tSize;
						BYTE * pbRet;
						pbStart=_binSearch(psPart->pbPart,psPdf->pbEndFile,"\rstream\n",8); if (!pbStart) ehError();
						pbStart+=8;
						pbEnd=_binSearch(pbStart,psPdf->pbEndFile,"\nendstream\r",11); if (!pbEnd) ehError();
//						pbStart=_binSearch(psPart->pbPart,psPart->pbPart+psPart->tSize-1,"\rstream\n",8); if (!pbStart) ehError();
//						pbStart+=8;
//						pbEnd=_binSearch(pbStart,psPart->pbPart+psPart->tSize-1,"\nendstream\r",11); if (!pbEnd) ehError();
						
						pbRet=_deflate(pbStart,(SIZE_T) pbEnd-(SIZE_T) pbStart,&tSize);
						if (pbRet) {
							if (*pbRet!=0) {
								
								psPart->enType=PDFT_DEFLATED;
								psPart->pszDecode=pbRet;
							}
							else ehFree(pbRet);
						
						}

					}
				}
				ehFreeNN(pszInfo);
			}
		}
	}	

}


//
// pdfSave()
//
BOOL pdfSave(S_PDF * psPdf,UTF8 * pszFileName) {

	EH_FILE * psFile=NULL;
	INT a;
	S_PDFPART  * psPart;
	z_stream sZstrm;
	INT iLen;
	BYTE *pBufferComp;
	
	psFile=fileOpen(pszFileName,FO_WRITE|FO_CREATE_ALWAYS);
	if (!psFile) return true;

	for (a=0;a<psPdf->dmiPart.Num;a++) {
		psPart=psPdf->arsPart+a;
		switch (psPart->enType) {
		
			//
			// Ricompattare il deflated
			//
			case PDFT_DEFLATED:

				ZeroFill(sZstrm);
				sZstrm.next_in=psPart->pszDecode;
				iLen=strlen(psPart->pszDecode);
				sZstrm.avail_in=iLen;
				pBufferComp=ehAllocZero(iLen);
				sZstrm.next_out=pBufferComp;
				sZstrm.avail_out=iLen;
				
				if (deflateInit(&sZstrm, Z_BEST_COMPRESSION)== Z_OK)
				{
					if (deflate (&sZstrm,  Z_FINISH) >=0) {
			
						CHAR szServ[1024];
						CHAR szBuf[1024];
						CHAR * pszHeader,* pszLength,* pszFooter;
						BYTE * pszEncode, *pb;
						SIZE_T tSize;

						//
						// Preparo Header
						//
						pszHeader=strExtract(psPart->pbPart,NULL,"\rstream\n",false,true); if (!pszHeader) ehError();
						pszLength=strExtract(pszHeader,"/Length ",">>",false,true); if (!pszLength) ehError();
						strcpy(szServ,pszHeader);
						sprintf(szBuf,"/Length %d>>",sZstrm.total_out);
						strReplace(szServ,pszLength,szBuf);
						strAssign(&pszHeader,szServ);
						ehFree(pszLength);

						//
						// Footer
						//
//						pszFooter=_binSearch(psPart->pbPart,psPart->pbPart+psPart->tSize-1,"\nendstream\r",11);
						pszFooter=_binSearch(psPart->pbPart,psPdf->pbEndFile,"\nendstream\r",11);

						// Calcolo totale e swapping dati
						tSize=strlen(pszHeader)+strlen(pszFooter)+sZstrm.total_out;
						pb=pszEncode=ehAlloc(tSize+1);
						strcpy(pb,pszHeader); pb+=strlen(pszHeader);
						memcpy(pb,pBufferComp,sZstrm.total_out); pb+=sZstrm.total_out;
						strcpy(pb,pszFooter);
						ehFree(psPart->pbPart);
						psPart->pbPart=pszEncode;
						psPart->tSize=tSize;
						ehFree(pszHeader);

					}
				}
				ehFree(pBufferComp);
				break;

				break;

			//
			// Default
			//

			default:
				break;
		}
		fileWrite(psFile,psPart->pbPart,psPart->tSize);
	}

	fileClose(psFile);
	return FALSE;

}

//
// pdfFree()
//
BOOL pdfFree(S_PDF * psPdf) {

	INT a;

	// Vecchio sistema
	if (psPdf->dmiPart.Hdl) {
		for (a=0;a<psPdf->dmiPart.Num;a++) {
			ehFreeNN(psPdf->arsPart[a].pbPart);
			ehFreeNN(psPdf->arsPart[a].pszDecode);
		}
		DMIClose(&psPdf->dmiPart,"pdfPart");
	}

	// Ultima pagina richiesta
	if (psPdf->lstPageGo) {
		_pageGoFree(psPdf);
		psPdf->lstPageGo=lstDestroy(psPdf->lstPageGo);
		psPdf->lstContentsObj=lstDestroy(psPdf->lstContentsObj);
	}


	if (psPdf->psTrailer) psPdf->psTrailer=_objFree(psPdf->psTrailer,true);
	
	//
	// Libero le risorse nei CrossReference
	//
	if (psPdf->lstRef) {
		EH_LST_I *  psLsti;
		S_PDF_XREF * psRef;
		lstForSafe(psPdf->lstRef,psRef,psLsti)
		{	
			if (psRef->psObj) 
				_objFree(psRef->psObj,true);
		}
		psPdf->lstRef=lstDestroy(psPdf->lstRef);
	}

	// File sorgente in memoria
	if (psPdf->pbSource) ehFreePtr(&psPdf->pbSource);

	ehFree(psPdf);
	return false;

}

//
// pdfStrReplace()
// rimpiazza una stringa ove la trova nei campi testo del pdf
//
BOOL pdfStrReplace(S_PDF * psPdf,UTF8 * pszStrSearch,UTF8 * pszStrReplace) {

	INT a;
	S_PDFPART * psPart;
	DWORD dwSize;
	CHAR * pszBuffer;


	for (a=0;a<psPdf->dmiPart.Num;a++) {
		psPart=psPdf->arsPart+a;
		switch (psPart->enType) {
			
			case PDFT_DEFLATED:
				dwSize=strlen(psPart->pszDecode)*2;
				pszBuffer=ehAlloc(dwSize);
				strcpy(pszBuffer,psPart->pszDecode);
				while (strReplace(pszBuffer,pszStrSearch,pszStrReplace));
				if (strlen(pszBuffer)>dwSize) ehError();
				strAssign(&psPart->pszDecode,pszBuffer);
				ehFree(pszBuffer);
				break;

			default:
				break;
		
		}
	}

	return FALSE;
/*
	INT a;
	S_PDFPART sPart;

	for (a=0;a<psPdf->dmiPart.Num;a++) {
		
		DMIRead(&psPdf->dmiPart,a,&sPart);
		
		// se siamo in un testo compattato
		if (sPart.enType==PDFT_DEFLATED) {
			
			BYTE *pStreamUpd=NULL;
			BYTE *p=NULL;
			CHAR *pStreamStart="stream";
			CHAR *pStreamEnd="endstream";
			INT iSizeStart;
			INT iSizeEnd;
			CHAR *pszHeaderObj=strExtract(sPart.pbPart,"<<",">>",FALSE,FALSE);

			if (strstr(pszHeaderObj,"Filter")) { // siamo nel filter
				if (strstr(pszHeaderObj,"Image")) continue; // è un' immagine
			}

			
			
	
			iSizeStart=strlen(pStreamStart);
			iSizeEnd=strlen(pStreamEnd);
			for (p=sPart.pbPart,i=0;i<(INT)sPart.tSize-iSizeStart;i++,p++,pStreamUpd++)
			{
				if (!memcmp(p,pStreamStart,iSizeStart)) {
					INT iSizeComp=0;
					BYTE *pb=NULL;
					BYTE *pBuffer=NULL;	
					BYTE *pStreamComp=NULL;
				

					z_stream zstrm;
					INT irsti;

					p+=iSizeStart; // mi sposto all' inizio della parte compressa
					i+=iSizeStart;
					
					p++; // per acapo
					i++;

					while (memcmp(p,pStreamEnd,iSizeEnd))
					{
						iSizeComp++;
						i++; p++;
					}

					iSizeComp--;  // per a capo

					pb=p;
					pb--; // per a capo
					pb-=iSizeComp;
					pBuffer=ehAlloc(iSizeComp);

					memcpy(pBuffer,pb,iSizeComp);		// alloco il buffer per la stringa compressa


					ZeroFill(zstrm);
					zstrm.avail_in = iSizeComp;
					zstrm.avail_out = 12*iSizeComp;
					zstrm.next_in = (Bytef*) pBuffer;
					zstrm.next_out = pStreamComp;
					irsti = inflateInit(&zstrm);
					if (irsti == Z_OK)
					{
					  int irst2 = inflate (&zstrm, Z_FINISH);
					  if (irst2 >= 0) // ho decompresso il testo
					  {
						size_t sizeStreamComp = zstrm.total_out; // dimensione testo decompresso

						pStreamUpd=ehAlloc(i+sizeStreamComp+1+10+7);  // + \r + endstream\r + endobj\r
						memcpy(pStreamUpd,p,i); // copio la mem fino a "stream" a capo compreso
						pStreamUpd+=i;
						memcpy(pStreamUpd,pStreamComp,sizeStreamComp); 
						ehFree(pStreamComp);
						pStreamUpd+=sizeStreamComp;
						memcpy(pStreamUpd,"\rendstream\rendobj\r",18); 

					  }
					  else ehError();
					}
				
				}
			}
		}
	}	
*/
}

/*
S_PDF * pdfLoadx(UTF8 * pszFileName) {

	BYTE *pStream=NULL;
	BYTE *p=NULL;
	//CHAR *pStreamStart="stream";
	CHAR *pStreamStart="<</Filter";
	CHAR *pStreamEnd="endstream";
	EH_FILE *psFile;
	SIZE_T tSize;
	INT i=0;
	INT iSizeStart,iSizeEnd;
	S_PDFPART *psPdfPart;
	S_PDF *psPdf;

	pStream=fileMemoRead(pszFileName,&tSize);

	// da saltare
	// Image/Type

	DMIReset(&psPdf->dmiPart);
	
	DMIOpen(&psPdf->dmiPart,
			RAM_AUTO,
			1000,
			sizeof(S_PDFPART),
			"pdfPart");
	


	iSizeStart=strlen(pStreamStart);
	iSizeEnd=strlen(pStreamEnd);
	for (p=pStream,i=0;i<(INT)tSize-iSizeStart;i++,p++)
	{

		if (!memcmp(p,pStreamStart,iSizeStart)) {
			INT iSizeComp=0;
			BYTE *pb=NULL;
			BYTE *pBuffer=NULL;	
			BYTE *pStreamComp=NULL;
		

			z_stream zstrm;
			INT rsti;

			p+=iSizeStart; // mi sposto all' inizio della parte compressa
			i+=iSizeStart;
			
			p++; // per acapo
			i++;

			while (memcmp(p,pStreamEnd,iSizeEnd))
			{
				iSizeComp++;
				i++; p++;
			}

			iSizeComp--;  // per a capo

			pb=p;
			pb--; // per a capo
			pb-=iSizeComp;
			pBuffer=ehAlloc(iSizeComp);

			memcpy(pBuffer,pb,iSizeComp);		// alloco il buffer per la stringa compressa

			ZeroFill(psPdfPart);

			psPdfPart->enType=PDFT_DEFLATED;
			psPdfPart->ptr=strDup(pStreamComp);
			psPdfPart->tSize=strlen(pStreamComp);

			DMIAppendDyn(&psPdf->dmiPart,psPdfPart);


			p+=iSizeEnd; // mi sposto all' inizio della parte compressa
			i+=iSizeEnd;

			
		}

		

	}


	ehFree(pStream);

	fileClose(psFile);


	return psPdf;
}
*/



CHAR * _get(S_PDF * psPdf,CHAR * pb,CHAR * pszDest,INT iSize) {

	//CHAR * pEnd=strstr(pb,"\r"); xxx
	CHAR * pEnd=_binSearch(pb,psPdf->pbEndFile,"\r",1);
	if (!pEnd) {
		pEnd=NULL;//pb+strlen(pb);
		*pszDest=0;
	}
	else {
		INT iTok=(DWORD) pEnd-(DWORD) pb;
		if (iTok>(iSize-1)) ehError();
		memcpy(pszDest,pb,iTok); pszDest[iTok]=0;
		pEnd++;
	}
	
	return pEnd;
}

//
// _xrefSearch()
//
static S_PDF_XREF *  _xrefSearch(S_PDF * psPdf,INT id,BOOL bExplode) {

	S_PDF_XREF * psRef;
	S_PDF_OBJ * psFilter;
	EH_LST_I *  psLsti;
	lstForSafe(psPdf->lstRef,psRef,psLsti)
	{
		if (psRef->id==id) break;
	}
	if (psRef&&!psRef->psObj&&bExplode) {
		CHAR szServ[80];
		sprintf(szServ,"xref#%d",id);
		psRef->psObj=_objSearch(psPdf,NULL,szServ,true);
		_objValueParser(psRef->psObj,psRef->pbStart,psPdf->pbEndFile);
		psFilter=pdfObjGet(psRef->psObj,".Filter");
		if (psFilter) {
			
			//
			// Decodifica del valore contenuto
			//
			if (!strCmp(psFilter->pbValue,"FlateDecode")) {
			
				BYTE * pbStart, *pbEnd;
				SIZE_T tSize;
				//BYTE * pbRet;
				pbStart=_binSearch(psRef->psObj->pbSourceStart,psPdf->pbEndFile,"stream",6); 
				if (!pbStart) 
					ehError();
				pbStart+=6; pbStart=_charJump(pbStart);
				pbEnd=_binSearch(psRef->psObj->pbSourceStart,psPdf->pbEndFile,"endstream",9); if (!pbEnd) ehError();
				psRef->psObj->pbValue=_deflate(pbStart,(SIZE_T) pbEnd-(SIZE_T) pbStart,&tSize);

			}

		}
		
	}
	return psRef;
}

static CHAR * _xrefGetStr(S_PDF * psPdf,INT id) {

	CHAR * pszRet;
	S_PDF_XREF * psRef;
	psRef=_xrefSearch(psPdf,id,true);
	if (!psRef) return NULL;
	pszRet=strTake(psRef->pbStart,psRef->pbEnd);
	return pszRet;

}

//
// _xrefAdd()
//
static void  _xrefAdd(S_PDF * psPdf,S_PDF_XREF * psXref) {

	S_PDF_XREF * psRef;
	CHAR * pb;
	CHAR szServ[100];

	psRef=_xrefSearch(psPdf,psXref->id,false);
	if (psRef) memcpy(psRef,psXref,sizeof(S_PDF_XREF)); else psRef=lstPush(psPdf->lstRef,psXref);

	//
	// Processo l'oggetto
	//
	pb=_get(psPdf,psRef->pb,szServ,sizeof(szServ)); // contiene id e gruppo (forse bisognerebbe verificare)
	pb=_get(psPdf,psRef->pb,szServ,sizeof(szServ)); // Obj
	psRef->pbStart=pb;
	psRef->pbEnd=_binSearch(pb,psPdf->pbEndFile,"endobj\r",7); //strstr(pb,"endobj"); 
	if (!psRef->pbEnd) ehError();
	psRef->pbEnd--;
	psRef->dwSize=(DWORD) psRef->pbEnd-(DWORD) psRef->pbStart;

}


//
// _xrefExtract()
//
static CHAR * _xrefExtract(S_PDF * psPdf,CHAR * pb) {

	CHAR szServ[100];
	EH_AR ar;
	INT a,iStart,iLen;
	S_PDF_XREF sXref;

	pb=_get(psPdf,pb,szServ,sizeof(szServ));
	if (strCmp(szServ,"xref")) ehError();
	
	pb=_get(psPdf,pb,szServ,sizeof(szServ));
	ar=strSplit(szServ," ");
	iStart=atoi(ar[0]); iLen=atoi(ar[1]);
	ehFree(ar);
	for (a=0;a<iLen;a++) {
		
		pb=_get(psPdf,pb,szServ,sizeof(szServ));
		ar=strSplit(szServ," ");
		_(sXref);
		sXref.id=iStart+a;
		sXref.iOffset=atoi(ar[0]);
		sXref.iGeneration=atoi(ar[1]);
		sXref.cType=*ar[2];
		sXref.pb=psPdf->pbSource+sXref.iOffset;
		ehFree(ar);
		_xrefAdd(psPdf,&sXref);
		pb++;
	
	}
	return pb;

}
/*
static CHAR * _getObjValue(S_PDF * psPdf,INT iId) {
	
	CHAR * pszRet;
	return pszRet;

}
*/
//
// _objSearch()
//
static S_PDF_OBJ * _objSearch(S_PDF * psPdf,S_PDF_OBJ * psObjStart,CHAR * pszKey,BOOL bCreate) {
	
	S_PDF_OBJ * psObj=psObjStart;
	S_PDF_OBJ * psObjLast=NULL;
	if (!psObj) {
		
		if (bCreate) {

			// Creo il primo elemento
			psObj=ehNew(S_PDF_OBJ);
			psObj->psPdf=psPdf;
			psObj->pszName=strDup(pszKey);
			return psObj;

		}
		return NULL;
	}
	for (psObj=psObjStart;psObj;psObj=psObj->psNext) {
		psObjLast=psObj;
		if (!strcmp(psObj->pszName,pszKey)) break;
	
	}

	if (!psObj&&bCreate) {
	
		//
		// Accodo all'ultimo elemento
		//
		psObj=ehNew(S_PDF_OBJ);
		psObj->psPdf=psPdf;
		psObj->pszName=strDup(pszKey);
		if (psObjLast->psNext)
			ehError();
		psObjLast->psNext=psObj;

	}
	return psObj;

}

//
//	_objDictExtract() 
//
//  Estra elementi di un dictionary (struttura) << >>
//	La dictionary è fatta elementi a coppie (key e valore)
// 
static CHAR * _objDictExtract(S_PDF_OBJ * psObj,BYTE * pbStart,BYTE * pbEndSource) {

	CHAR *	pb, *pszKeyStart,*pszKeyEnd,*pszValueStart;
	INT		iArray;
	CHAR	szKey[80];
	S_PDF_OBJ * psChild;

	//
	// Cerco inizio
	//
	for (pb=pbStart;pb<pbEndSource;pb++) {
		if (!strBegin(pb,"<<")) {pb+=2; break;}
	}

	pszValueStart=pszKeyStart=pszKeyEnd=NULL;
	iArray=0;
	for (;pb<pbEndSource;pb++) {


		//
		// Cerco la "Key"
		//
		pb=_charJump(pb);
		if (*pb!='/') 
			ehError();
		pszKeyStart=pb+1;
		
		for (;pb<pbEndSource;pb++) {
			if (strchr(" \r\n<>[]",*pb)) break;
		}
		pszKeyEnd=pb-1;

		/*
		if (!pszKeyEnd) {
			
			if (!pszKeyStart) // Inizio del keyname
			{
				pb=_charJump(psPdf,pb);
				if (*pb!='/') continue;
				pszKeyStart=pb+1; pszKeyEnd=NULL; 
				continue;
			}
			
			if (!strchr(" /[<>",*pb)) continue;
			pszKeyEnd=pb-1;
		}
*/

		//
		// Cerco il valore della key
		//
//		if (strchr(" \r\n",*pb)) continue;
		pb=_charJump(pb);
	
		//
		// Creo l'oggetto (lo aggiungo ai figli) e lo valorizzo
		//
		strTakeCpy(pszKeyStart,pszKeyEnd,szKey,sizeof(szKey));
		psChild=_objSearch(	psObj->psPdf,
							psObj->psChild,
							szKey,
							true);
		if (!psObj->psChild) 
			psObj->psChild=psChild;
		pb=_objValueParser(psChild,pb,pbEndSource); 
		pszKeyStart=pszKeyEnd=NULL;

		pb=_charJump(pb);
		if (!strBegin(pb,">>")) 
		{
			pb+=2; break;
		}
		pb--;
		
	}
/*
	//
	// Elaboro il pezzo (finale)
	//
	if (pszKeyStart) {
		CHAR * pszLast=pb-4;
		CHAR * pszKey=strTake(pszKeyStart,pszKeyEnd?pszKeyEnd:pszLast);
		CHAR * pszValue=pszValueStart?strTake(pszValueStart,pszLast):NULL;

		psChild=_objProcess(psPdf,psObj->psChild,pszKey,strEver(pszValue));
		if (!psObj->psChild) psObj->psChild=psChild; // Primo figlio

		ehFreePtrs(2,&pszKey,&pszValue);
		pszKeyEnd=NULL;
		pszValueStart=NULL;
	
	}
	if (psObj->psChild&&psObj->pbValue) ehFreePtr(&psObj->pbValue); // Tolgo il valore perché esploso
	*/
	return pb; // Fine estrazione << >>
}

//
// _charJump()
//
//static BYTE * _charJump(S_PDF * psPdf,CHAR * pb) {
static BYTE * _charJump(CHAR * pb) {

//	for (;pb<psPdf->pbEndFile;pb++) {
	for (;*pb;pb++) {
		if (!strchr(" \r\n",*pb)) break;
	}
	return pb;
}


//
//	_objArrayExtract() 
//
//  Estrae un array di elementi
// 
static CHAR * _objArrayExtract(S_PDF_OBJ * psObj,CHAR * pszStart,BYTE * pbEndSource) {


	CHAR szKey[80];
	CHAR * pb;
//	CHAR * pszVal;
	S_PDF_OBJ * pszElem; // Elemento dell'array
	INT idx=0;

	for (pb=pszStart;pb<pbEndSource;pb++) {
		if (*pb=='[') {pb++; break;}
	}
	
	psObj->iLength=0;
	for (;pb<pbEndSource;pb++) {
		
		pb=_charJump(pb);
		if (*pb==']') {pb++; break;} // Fine array

		// Creo l'elemento dell'array e lo associo al obj parent
		sprintf(szKey,"[%d]",idx); 
		pszElem=_objSearch(	psObj->psPdf,
							psObj->psArray,
							szKey,
							true);
		if (!psObj->psArray) psObj->psArray=pszElem;
		pszElem->idx=idx; idx++; psObj->iLength++;
		pb=_objValueParser(pszElem,pb,pbEndSource); pb--;

	}
	return pb;

}

//
// _objFree() - Libera le risorse impegnate ricorsivamente
//
static S_PDF_OBJ * _objFree(S_PDF_OBJ * psObj,BOOL bSelf) {

	S_PDF_OBJ * ps,* psNext;
	ehFreePtr(&psObj->pszName);

	if (!psObj->psXref) {
		
		ehFreePtr(&psObj->pbValue);

		for (ps=psObj->psChild;ps;ps=psNext) {
			psNext=ps->psNext;
			_objFree(ps,true);
		}

		for (ps=psObj->psArray;ps;ps=psNext) {
			psNext=ps->psNext;
			_objFree(ps,true);
		}

	} 
	if (bSelf) {ehFree(psObj); return NULL;}  else return psObj;

}

//
// _objValueParser() > Inserisce il valore letto a partire da pszValueStart nell'oggetto
//
static CHAR * _objValueParser(S_PDF_OBJ * psObj, CHAR * pszValueStart, BYTE * pbEndSource) { 

	BYTE  * pb;
	pb=_charJump(pszValueStart);
	psObj->pbSourceStart=pszValueStart;

	//
	// Dictionary 
	//
	if (!strBegin(pb,"<<")) {
		
		psObj->enType=POT_DICT;
		pb=_objDictExtract(psObj,pb,pbEndSource);
		
	} 
	//
	// Stringa
	//
	else if (*pb=='(') {
	
		CHAR *	pbStart=pb;
		CHAR *	pbEnd=NULL;
//		CHAR *  pbDest=NULL;
		WCHAR *  pwcDest=NULL, * pwcStart, wChar;
		INT		iStep,iPar=0;
		BYTE	ch;

		ehFreeNN(psObj->pbValue);
		
		for (iStep=0;iStep<2;iStep++) {
			iPar=0;

			// Alloco memoria per contenere i caratteri
			if (iStep) {
				
				psObj->iSize=((DWORD) pbEnd-(DWORD) pbStart);
				if (psObj->iSize<0)
						ehError();
				//pbDest=psObj->pbValue=ehAllocZero(psObj->iSize+10);
				pwcStart=pwcDest=ehAllocZero(psObj->iSize*2+100);
				if (!pwcStart) 
					ehError();
				wChar=0;

			}
			for (pb=pbStart;pb<pbEndSource;pb++) {
				ch=*pb;

				if (ch=='(') {
					iPar++; 
					if (iPar>1) 
					{
						wChar|=ch;
					} else continue;
				
				}
				else if (ch==')') {
					iPar--;
					if (iPar>0)
					{
						wChar|=ch;
					}
					else 
					{
						pbEnd=pb-1; pb++;
						break;
					}


				}

				else if (ch=='\\') {

					pb++; ch=*pb;
					switch (ch) {
						case 'n': wChar=L'\n'; break;
						case 'r': wChar=L'\r'; break;
						case 't': wChar=L'\t'; break;
						case 'b': wChar=L'\b'; break;
						case 'f': wChar=L'\f'; break;
						case '(': wChar=L'('; break;
						case ')': wChar=L')'; break;
						
						default:

							if (ch>='0'&&ch<='8')
							{
								INT a=ch-'0';
								INT b=(*++pb)-'0';
								INT c=(*++pb)-'0';
								ch=(c&7)|((b&7)<<3)|((a&7)<<6);

								//
								// Unicode
								//
								if (ch==0) {
								
									wChar=(ch<<8);
									continue;
								}
								wChar|=ch;

							}
							else 
								ehError();
							break;
					}
				} else 
					wChar|=ch;

				//
				// Memorizzo i caratteri
				//
				if (iStep) {

					*pwcDest++=wChar;
					wChar=0;
					/*
					if (!ch) 
						{
							strAppend(pbDest,"\\0"); pbDest+=2; 
						}
						else 
						{
							*pwcDest++=ch;
							*pbDest++=ch; 
						}
						*/
				}
			}
			if (iPar) ehError();
		}
		//*pbDest=0;
		psObj->enType=POT_STRING;
		*pwcDest=0;
		psObj->pbValue=strEncodeW(pwcStart,SE_UTF8,NULL);
		if (strstr(psObj->pbValue,"%")) 
			printf("qui");
		ehFree(pwcStart);
	//	printf("[%s]",psObj->pbValue);
//		if (!strstr(psObj->pbValue,"\0I")) 
//			printf("qui");
		
	}
	//
	// Valore esadecimale
	//
	else if (*pb=='<') {
	
		CHAR szServ[80];
		CHAR *	pbStart=pb+1;
		CHAR *	pbEnd=_binSearch(pb+1,pbEndSource,">",1);
		CHAR *  pszValue=ehAllocZero(((DWORD) pbEnd-(DWORD) pbStart)+1);
		CHAR *  pd=pszValue;
		psObj->iSize=0;
		for (pb=pbStart;pb<pbEnd;pb+=2) {
			if (*pb=='>') break;	
			memcpy(szServ,pb,2); szServ[2]=0;
			*pd=(BYTE) xtoi(szServ); pd++; psObj->iSize++;
		}
		*pd=0;
		psObj->enType=POT_HEX;
		strAssign(&psObj->pbValue,pszValue);
		ehFree(pszValue);
		pb++;
		
	} 
	//
	// Object
	//
	else if (!strBegin(pb,"/")) {
	
		CHAR * pszStart=pb+1;
		CHAR * pszValue;
		for (pb=pszStart;pb<pbEndSource;pb++) {
			if (strchr(" /\r\n[]<>",*pb)) break;
		}
		psObj->enType=POT_NAME;
		pszValue=strTake(pszStart,pb-1);
		strAssign(&psObj->pbValue,pszValue);
		ehFree(pszValue);

	}
	//
	// Array
	//
	else if (!strBegin(pb,"[")) {

		psObj->enType=POT_ARRAY;
		pb=_objArrayExtract(psObj,pb,pbEndSource);

	}
	else {

	//
	// Altro
	//
		CHAR * pszStart=pb;
		for (;pb<pbEndSource;pb++) {
			if (strchr(" \r\n/<>[]",*pb)) break;
		}

		//
		// idRif
		//
		if (!strBegin(pb," 0 R ")||
			!strBegin(pb," 0 R\r")||
			!strBegin(pb," 0 R\n")||
			!strBegin(pb," 0 R/")||
			!strBegin(pb," 0 R]")||
			!strBegin(pb," 0 R>")) {
			
			CHAR szServ[80];
			strTakeCpy(pszStart,pb-1,szServ,sizeof(szServ));
			psObj->enType=POT_OBJ;
			psObj->idRef=atoi(szServ);
			pb+=4;

		} else {

			ehFreeNN(psObj->pbValue);
			if (pb<=pszStart) 
			{
				psObj->pbValue=NULL; psObj->iSize=0;
			}
			else {
				
				// Controllo se è un numero
				if (strchr("0123456789.-+",*pszStart)) {
					for (pb=pszStart+1;pb<pbEndSource;pb++) {
						if (strchr("0123456789.",*pb)) continue; else break;
					}
				}

				psObj->pbValue=strTake(pszStart,pb-1); // Valore semplice (si potrebbe memorizzare inizio e fine del valore) 				psObj->iSize=strlen(psObj->pbValue);
				if (!isNaN(psObj->pbValue)) {
					psObj->enType=POT_NUMERIC;
				} else {
					if (!strcmp(psObj->pbValue,"true")) {
						psObj->enType=POT_BOOL;
						strAssign(&psObj->pbValue,"1");
					}
					else if (!strcmp(psObj->pbValue,"false")) {
						psObj->enType=POT_BOOL;
						strAssign(&psObj->pbValue,"0");
					}
				
				}


			}		
		}
	
	
	}
	
	return pb;


}

//
// pdfObjShow()
//
void pdfObjShow(S_PDF_OBJ * psObj,INT iIndent,BOOL bRamo,BOOL bShowArray) {

	S_PDF_OBJ * ps;//,* psx;
	if (!psObj) {printf("NULL" CRLF); return;}

	for (ps=psObj;ps;ps=ps->psNext) {
			
		printf("%s%s",strPad(' ',iIndent),ps->pszName);
		if (ps->pbValue) printf(" = %s ",ps->pbValue);
		if (ps->psArray) printf(" (array:%d)",ps->iLength);
		if (ps->idRef) printf(" [#%d]",ps->idRef);
		printf(CRLF);
		
		if (bShowArray&&ps->psArray) {
			INT a=0;
			S_PDF_OBJ * psx;
			iIndent++;
			for (psx=ps->psArray;psx;psx=psx->psNext) {
				printf("%s- [%d] = %s",strPad(' ',iIndent),a,psx->pbValue?psx->pbValue:""); 
				if (psx->idRef) 
					printf(" [#%d]",psx->idRef);
				printf(CRLF);
				if (psx->psChild) {
					pdfObjShow(psx->psChild,iIndent+1,true,bShowArray);
				}
				
				a++;
				if (a>30) {printf("%s and more ..." CRLF,strPad(' ',iIndent)); break;}
			}
			iIndent--;
		
		}
		
		if (ps->psChild) {
			pdfObjShow(ps->psChild,iIndent+1,true,bShowArray);
		}
		if (!bRamo) break;
		
	}

}

static void _objExpand(S_PDF_OBJ * psObj,BOOL bAll) {

	S_PDF_OBJ * ps;
	for (ps=psObj;ps;ps=ps->psNext) {
		
		if (ps->idRef&&!ps->psXref) {
			S_PDF_XREF * psXref=_xrefSearch(ps->psPdf,ps->idRef,true);
			ps->psXref=psXref->psObj; // Assegno il valore estratto
			ps->psChild=psXref->psObj->psChild;
		}

		if (bAll&&ps->psChild) 
			_objExpand(ps->psChild,bAll);

		if (ps->psArray&&bAll) {
			_objExpand(ps->psArray,bAll);
		}

	}
}



//
// pdfObjGet()
//
S_PDF_OBJ * pdfObjGet(S_PDF_OBJ * psObj,CHAR * pszKey) {

	S_PDF_OBJ * psRet=NULL;
	S_PDF_OBJ * ps;
	EH_AR ar=strSplit(pszKey,".");
	INT		iTarget=ARLen(ar)-1;
	INT		idx=0;
	CHAR *	pszElem;
	CHAR	szElement[80];
	INT		idxArray;
	CHAR *	p, * psz;
	BOOL	bWantArrayElement;

	if (!psObj) 
		ehError();
	
	for (ps=psObj;ps;) {
		
		pszElem=ar[idx];
		strCpy(szElement,pszElem,sizeof(szElement));
		bWantArrayElement=false;
		p=strstr(szElement,"[");
		if (p) {
			psz=strExtract(szElement,"[","]",false,false); if (!psz) ehError();
			idxArray=atoi(psz);
			bWantArrayElement=true;
			*p=0;
		}
		if (!strcmp(ps->pszName,szElement)||!*szElement) {
		
			if (ps->idRef&&!ps->psXref)  {

				S_PDF_XREF * psXref=_xrefSearch(ps->psPdf,ps->idRef,true);
				ps->psXref=psXref->psObj; // Assegno il valore estratto
			//	if (ps->pbValue||ps->psChild)
			//		printf("qui");
				ps->pbValue=psXref->psObj->pbValue;
				ps->psChild=psXref->psObj->psChild;
				// Attenzione: next e array no!!
			}

			//psx=ps->psRef?ps->psRef:ps; // ??
			if (bWantArrayElement) {
			
				INT a=0;
				S_PDF_OBJ * psEle;
				if (!ps->psArray) ehExit("%s non è un array",pszKey);
				for (psEle=ps->psArray;psEle;psEle=psEle->psNext) {
					
					if (a==idxArray) {ps=psEle; break;}
					a++;
				}
				if (!psEle) 
					ehExit("indice array %s non corretto",pszKey);
			
			} 

			if (idx==iTarget) {
				psRet=ps; 
				break;
			}

//			if (!bWantArrayElement) {
			ps=ps->psChild; // <--  Salto nel figlio
//			}


			idx++;
			pszElem=ar[idx];
			if (ps->psArray) {
				
				_objExpand(ps->psArray,false);

				// Lunghezze dell'array
				if (!strcmp(pszElem,"length")) {

					CHAR szServ[80];
					sprintf(szServ,"%d",ps->iLength);
					strAssign(&ps->pbValue,szServ);
					psRet=ps;
					break;
				}
			}
	
			continue;
		}
		ps=ps->psNext;
	}
	ehFree(ar);
	return psRet;

}

//
// pdfObjGetValue()
//
CHAR * pdfObjGetValue(S_PDF_OBJ * psObj,CHAR * pszKey,CHAR * pszDef) {
	
	S_PDF_OBJ * psRet=pdfObjGet(psObj,pszKey);
	if (!psRet) return pszDef;
	return psRet->pbValue;

}

//
// objGetRect()
//
RECTD * objGetRect(S_PDF_OBJ * psObj,CHAR * pszKey,RECTD * prdRect) {

	S_PDF_OBJ * psRet=pdfObjGet(psObj,pszKey), * psEle;
	INT a;
	if (!psRet) return NULL;
	if (psRet->iLength!=4) ehError();

	a=0;
	for (psEle=psRet->psArray;psEle;psEle=psEle->psNext) {
		
		switch (a) {
		
			case 0: prdRect->left=atof(psEle->pbValue); break;
			case 1: prdRect->bottom=atof(psEle->pbValue); break;
			case 2: prdRect->right=atof(psEle->pbValue); break;
			case 3: prdRect->top=atof(psEle->pbValue); break;
		}
		a++;
	}
	return prdRect;

}
/*
//
// objGetPoint()
//
POINTD * objGetPoint(S_PDF_OBJ * psObj,CHAR * pszKey,POINTD * prdPoint) {

	S_PDF_OBJ * psRet=pdfObjGet(psObj,pszKey), * psEle;
	INT a;
	if (!psRet) return NULL;
	if (psRet->iLength!=2) ehError();

	a=0;
	for (psEle=psRet->psArray;psEle;psEle=psEle->psNext) {
		
		switch (a) {
		
			case 0: prdPoint->x=atof(psEle->pbValue); break;
			case 1: prdPoint->y=atof(psEle->pbValue); break;
		}
		a++;
	}
	return prdPoint;
}

//
// objGetRgbColor()
//
EH_COLORD * objGetRgbColor(S_PDF_OBJ * psObj,CHAR * pszKey,EH_COLORD * prdColor) {

	S_PDF_OBJ * psRet=pdfObjGet(psObj,pszKey), * psEle;
	INT a;
	if (!psRet) return NULL;
	if (psRet->iLength!=3) ehError();

	a=0;
	for (psEle=psRet->psArray;psEle;psEle=psEle->psNext) {
		
		switch (a) {
		
			case 0: prdColor->dRed=atof(psEle->pbValue); break;
			case 1: prdColor->dGreen=atof(psEle->pbValue); break;
			case 2: prdColor->dBlue=atof(psEle->pbValue); break;
		}
		a++;
	}
	return prdColor;

}
*/
//
// pdfLoadEx() > Importo in memoria il file Pdf
// Importa la struttura di un file PDF 
//
//

S_PDF * pdfLoadEx(UTF8 * pszFileName) 
{
	BYTE *	pbFile;
	BYTE *	pb;
	SIZE_T	tSize;
	S_PDF * psPdf;
	CHAR	szServ[8000];
//	S_PDF_OBJ * psRoot;
	S_PDF_OBJ * psPages;
	
	CHAR * pszVal;

	psPdf=ehNew(S_PDF);//ehAllocZero(sizeof(S_PDF));
	psPdf->lstRef=lstCreate(sizeof(S_PDF_XREF));	// Posizione delle referenze
	psPdf->lstPageGo=lstCreate(sizeof(S_PDF_GO));	// lista delle Graphic Operation di un pagina
	psPdf->lstContentsObj=lstCreate(sizeof(S_PDF_OBJ));	// Lista degli oggetti usati
	psPdf->psTrailer=NULL;

	//
	// Apro il file
	//
	pbFile=fileMemoRead(pszFileName,&tSize); if (!pbFile) ehExit("Il file %s non esiste",pszFileName);
	psPdf->pbSource=pbFile;
	psPdf->pbEndFile=pbFile+tSize-1;
	
	if (strBegin(pbFile,"%PDF-1.4")) ehExit("pdf:Gestito solo il formato 1.4");

	//
	// Preanalisi del trailer e extraggo xref
	//
	pb=_binSearchRev(psPdf,psPdf->pbEndFile,"trailer\r",0); 
	while (pb) {
	
		if (!strBegin(pb,"trailer")) {

			S_PDF_OBJ * psTrailer;
			pb+=8;
			
			//pszDo=strExtract(pb,"<<",">>",false,true);
			
			//
			// Creo l'oggetto e inserisco nel contenitore
			//
			psTrailer=_objSearch(psPdf,psPdf->psTrailer,"trailer",true);
			if (!psPdf->psTrailer) psPdf->psTrailer=psTrailer;

			pb=_objValueParser( psTrailer, // Oggetto da processare
								pb,
								psPdf->pbEndFile);	   // Inizio del valore da processare
		//	pdfObjShow(psTrailer,0,true);
			pszVal=pdfObjGetValue(psPdf->psTrailer,"trailer.Prev",NULL);
			if (pszVal) {
				pb=_xrefExtract(psPdf,psPdf->pbSource+atoi(pszVal));
				break; // Non credo, ma va bene cosi
			}		
			
			pb=_charJump(pb);
			if (!strBegin(pb,"startxref")) {
				
				_get(psPdf,pb+10,szServ,sizeof(szServ));
				if (!atoi(szServ)) break;
				pb=_xrefExtract(psPdf,psPdf->pbSource+atoi(szServ));
							
			}



		} else 
			break;
	}
	psPdf->xRefReady=true;


	//
	// Root
	//
//	_objExplode(psPdf,psPdf->psObj);
//	psRoot=pdfObjGet(psPdf->psTrailer,"trailer.Root"); //	pdfObjShow(psRoot,0,true);

	psPdf->psRoot=pdfObjGet(psPdf->psTrailer,"trailer.Root");
	psPages=pdfObjGet(psPdf->psRoot,".Pages"); //pdfObjShow(psPages,0,false);
	psPdf->iPages=atoi(pdfObjGetValue(psPages,".Count","0")); 
	// psResource=pdfObjGet(psPdf->psTrailer,"trailer.Root");
 	return psPdf;
}

//
// _pageSearch() - Cerca una pagina
//
static S_PDF_OBJ * _pageSearch(S_PDF * psPdf,S_PDF_OBJ * psObj,INT iPage)  {

	S_PDF_OBJ * ps, * psRet=NULL;
	S_PDF_OBJ * psKids;
	CHAR * pszType;

	pszType=pdfObjGetValue(psObj,".Type","");
	if (!strcmp(pszType,"Pages")) {
	
		psKids=pdfObjGet(psObj,".Kids"); if (!psKids) ehExit("_pageSearch()");
		for (ps=psKids->psArray;ps;ps=ps->psNext) {
			psRet=_pageSearch(psPdf,ps,iPage); 
			if (psRet) 
				return psRet;
		}

	}
	else if (!strcmp(pszType,"Page")) {
		
		psPdf->iPageFocus++; 
		if (psPdf->iPageFocus==iPage) {
			return psObj;
		}
	
	}

	return NULL;
}

static EH_LST __addGraph(S_PDF * psPdf,CHAR * pszOperation,CHAR * pszOriginal,EH_LST lstParams) {
	
	S_PDF_GO sGo;
	_(sGo);
	strcpy(sGo.szOriginal,pszOriginal);
	sGo.pszOperation=pszOperation;
	sGo.lstParams=lstParams;
	lstPush(psPdf->lstPageGo,&sGo);
	return lstCreate(sizeof(S_PDF_OBJ));

}

#define _addGraph(a) lstParams=__addGraph(psPdf,a,psz,lstParams); 

//
// _pageGoFree() > page Graphics Object Free
//
static void _pageGoFree(S_PDF * psPdf) {

	S_PDF_GO * psGo;
	S_PDF_OBJ * psObj;
	EH_LST_I * psListI;

	if (!psPdf->lstPageGo) return;
	if (!psPdf->lstPageGo->iLength) return;

	lstForSafe(psPdf->lstContentsObj,psObj,psListI) {
		
		_objFree(psObj,false);

	}
	lstClean(psPdf->lstContentsObj);

	lstForSafe(psPdf->lstPageGo,psGo,psListI) {
		if (psGo->lstParams) lstDestroy(psGo->lstParams);
	}
	lstClean(psPdf->lstPageGo);
	//psPdf->lstPageGo=lstDestroy(psPdf->lstPageGo);

}


//
// pdfGetPage()
// Legge l'elenco degli Go (Graphig Operator) che compongono una contenuto di una pagina
//
BOOL	pdfGetPage(S_PDF * psPdf,INT iPage,S_PDF_PAGE * psPageInfo) 
{
	S_PDF_OBJ * psPages,* psPage;
	S_PDF_OBJ * psContents=NULL;
//	S_PDF_OBJ * psObj;

	BYTE * pb;
	CHAR *psz;
	EH_LST lstParams=NULL;

	if (!psPageInfo) ehError();
	if (!psPdf) ehError();
	if (iPage<1||iPage>psPdf->iPages) return true; // La pagina non esiste

	_pageGoFree(psPdf);
	memset(psPageInfo,0,sizeof(S_PDF_PAGE));

	//
	// Trova la pagina interessata (da definire meglio)
	//
//	pdfObjShow(psPdf->psRoot,0,false,false);
	psPages=pdfObjGet(psPdf->psRoot,".Pages");
//	pdfObjShow(psPages,0,false,false);
	psPdf->iPageFocus=0;
	psPage=_pageSearch(psPdf,psPages,iPage);
	psPageInfo->psPage=psPage;
	psPageInfo->lstGo=psPdf->lstPageGo;

	if (psPage) {

		//
		// Parametri della pagina (PDFReference: Pag 88/89) 
		// Es. /Rotate
		// 
	//	pdfObjShow(psPage,0,false,true);	
		objGetRect(psPage,".MediaBox",&psPageInfo->rcdRect);

		//
		// Cerco i font
		//
		psPageInfo->psFonts=pdfObjGet(psPage,".Resources.Font");
		_objExpand(psPageInfo->psFonts->psChild,true);
#ifdef _DEBUG
	//	pdfObjShow(psPageInfo->psFonts,0,false,true);	
#endif
		psContents=pdfObjGet(psPage,".Contents");
		
		if (psContents) {
			

			//
			// Analisi postscript
			//
			CHAR * pbEnd=psContents->pbValue+strlen(psContents->pbValue);
			lstParams=lstCreate(sizeof(S_PDF_OBJ));
			for (pb=psContents->pbValue;*pb;pb++) {

				S_PDF_OBJ sObj;
				_(sObj);
				pb=_objValueParser(&sObj, pb, pbEnd); lstPush(psPdf->lstContentsObj,&sObj);
				switch (sObj.enType) {
					
					//
					// Dovrei inserire la coppia (nome > dati) da qualche parte
					//
					case POT_NAME:
						if (!strCmp(sObj.pbValue,"Layer")) {
							_(sObj); pb=_objValueParser(&sObj, pb, pbEnd); lstPush(psPdf->lstContentsObj,&sObj);
							lstPush(lstParams,&sObj);
							psz="";
							_addGraph("Layer");
						}
						else {

							if (sObj.pbValue) lstPush(lstParams,&sObj);
							//_objFree(&sObj,false);
						}
						
						break;

					//
					// Array
					//
					case POT_ARRAY:
						lstPush(lstParams,&sObj);
 						break;

					case POT_STRING:
					case POT_NUMERIC:
						lstPush(lstParams,&sObj);
 						break;


					default:

						psz=sObj.pbValue;
						if (sObj.enType!=POT_UNKNOW)
								printf("qui");
						//
						// PdfReference: Pag 131
						//
						if (strEmpty(psz))
							{
								ehError();
							}

						//
						// General graphics state w, J, j, M, d, ri, i, gs	4.7 156
						//
						else if (!strcmp(psz,"w")) {
							_addGraph("lineWidth");
						}
						else if (!strcmp(psz,"J")) {
							_addGraph("lineCap");
						}
						else if (!strcmp(psz,"j")) {
							_addGraph("lineJoin");
						}
						else if (!strcmp(psz,"M")) {
							_addGraph("miterLimit");
						}
						else if (!strcmp(psz,"d")) {
							_addGraph("dashParam");
						}
						else if (!strcmp(psz,"gs")) {
							_addGraph("dictName");
						}
						//
						// Special graphics state q, Q, cm					4.7 156
						//
						else if (!strcmp(psz,"q"))
							{_addGraph("backupGraphicState");}
						else if (!strcmp(psz,"Q"))
						{_addGraph("restoreGraphicState");}

						//
						// Path construction m, l, c, v, y, h, re			4.9 163
						//
						else if (!strcmp(psz,"m")) {
							_addGraph("moveTo");
						}
						else if (!strcmp(psz,"l")) {
							_addGraph("lineTo");
						}
						else if (!strcmp(psz,"c")) {
							_addGraph("bezierAppend3");
						}
						else if (!strcmp(psz,"c")) {
							_addGraph("bezierAppend2");
						}
						else if (!strcmp(psz,"h")) {
							_addGraph("closeSubPath");
						}
						else if (!strcmp(psz,"re")) {
							_addGraph("appendRect");
						}

						//
						// Path painting S, s, f, F, f*, B, B*, b, b*, n	4.10 167
						//
						else if (!strcmp(psz,"S")) {
							_addGraph("strokePath");
						}
						else if (!strcmp(psz,"s")) {
							_addGraph("closeStrokePath");
						}
						else if (!strcmp(psz,"f*")) {
							_addGraph("fillPath");
						}
						else if (!strcmp(psz,"B*")) {
							_addGraph("fillStrokePath");
						}
						else if (!strcmp(psz,"n")) {
							_addGraph("endPath");
						}

						//
						// Clipping paths W, W*								4.11 172
						//
						else if (!strcmp(psz,"W*")) {
							_addGraph("clippingPath");
						}

						//
						// Text objects BT, ET 5.4 308
						//
						else if (!strcmp(psz,"BT")) {
							_addGraph("textStartMatrix");
						}
						else if (!strcmp(psz,"ET")) {
							_addGraph("textEndMatrix");
						}

						//
						// Text state Tc, Tw, Tz, TL, Tf, Tr, Ts 5.2 302
						//
						else if (!strcmp(psz,"Tf")) {
							_addGraph("fontSize");
						}
						else if (!strcmp(psz,"TL")) {
							_addGraph("textLeading");
						}

						// Text positioning Td, TD, Tm, T* 5.5 310
						else if (!strcmp(psz,"Tm")) {
							_addGraph("textMatrix");
						}
						else if (!strcmp(psz,"T*")) {
							_addGraph("textNextLine");
						}
						//
						// Text showing Tj, TJ, ', " 5.6 311
						//
						else if (!strcmp(psz,"Tj")) { // Testo singolo
							if (!lstParams->iLength)
								ehExit("error");
							_addGraph("textShow");
						}
						else if (!strcmp(psz,"TJ")) { // Array
							if (!lstParams->iLength)
								ehExit("error");
							_addGraph("textShowAr");
						}

						// Type 3 fonts d0, d1 5.10 326

						//
						// Color CS, cs, SC, SCN, sc, scn, G, g, RG, rg, K, k	4.21 216
						//
						else if (!strcmp(psz,"RG")) {
							_addGraph("strokeRGB");
						}
						else if (!strcmp(psz,"rg")) {
							_addGraph("fillRGB");
						}
						else if (!strcmp(psz,"G")) {
							_addGraph("strokeGray");
						}
						else if (!strcmp(psz,"g")) {
							_addGraph("fillGray");
						}

						// Shading patterns sh 4.24 232
						// Inline images BI, ID, EI 4.38 278
						// XObjects Do 4.34 261
						
						// Marked content MP, DP, BMC, BDC, EMC 9.8 584
						else if (!strcmp(psz,"BMC"))
							{_addGraph("startSequence");}
						else if (!strcmp(psz,"BDC"))
							{_addGraph("startSequence");}
						else if (!strcmp(psz,"EMC"))
							{_addGraph("endSequence");}
						else if (!strcmp(psz,"MP")) {
							{_addGraph("marketPoint");}
						}
						//
						// Compatibility BX, EX 3.20 95
						//

						else {
						
							if (sObj.enType==POT_NUMERIC||
								sObj.enType==POT_STRING) 
							{
								lstPush(lstParams,&sObj);
							}
							//	lstPushf(lstStack,"\'%s\'",psz);
							else {
								printf("Operator  [%s] ? " CRLF,psz);
								_addGraph("?");
							}
								//lstPushf(psPdf->lstPageGraph,"?,%s",psz);
						}
						// ehFree(sObj.pbValue);
				//		_objFree(&sObj,false);
						break;
				
				}
					
		//		_objFree(&sObj,false);
			
			}
		}
	}
	lstDestroy(lstParams);
	return false;

}

/*

#ifdef _DEBUG
	{
		S_PDF * psPdf;
		EH_LST lst;
		S_PDF_GO * psGo;
		S_PDF_OBJ * psParam;
		S_PDF_OBJ * psEle;

		ehConsole(true,TRUE);
		
//		psPdf=pdfLoadEx("c:\\proGenesi\\resource\\test.pdf"); 
		psPdf=pdfLoadEx("c:\\proGenesi\\resource\\modulo10.pdf"); 
 	 	lst=pdfGetPage(psPdf,1);

		for (lstLoop(lst,psGo)) {
			printf("%-2.2s : %s :",psGo->szOriginal,psGo->pszOperation);
			a=0;
			for (lstLoop(psGo->lstParams,psParam)) {
				
				if (a) printf(",");
				switch (psParam->enType) {
					
					case POT_DICT:
						break;
				
					case POT_ARRAY:
						idx=0;
						printf(CRLF);
						for (psEle=psParam->psArray;psEle;psEle=psEle->psNext) {
							printf("\t [%d] %s" CRLF,idx,psEle->pbValue); idx++;
						}
						break;

					default:
						printf("%s",psParam->pbValue);
						break;
				}
				a++;
				
			}
		
			printf(CRLF);
		}
 		pdfFree(psPdf);
 		ehExit("");
	} 

#endif
*/



//
// Visualizza parametri
//
static void _showParams(EH_LST lstParams) {

	S_PDF_OBJ * psParam;
	S_PDF_OBJ * psEle;
	INT a=0,idx;
	for (lstLoop(lstParams,psParam)) {
		
		if (a) printf(",");
		switch (psParam->enType) {
			
			case POT_DICT:
				break;
		
			case POT_ARRAY:
				idx=0;
				printf(CRLF);
				for (psEle=psParam->psArray;psEle;psEle=psEle->psNext) {
					printf("\t [%d] %s" CRLF,idx,psEle->pbValue); idx++;
				}
				break;

			default:
				printf("%s",psParam->pbValue);
				break;
		}
		a++;
		
	}

}

	//
	// Font
	//
	typedef struct {

		CHAR		szName[300];
		EH_TSTYLE	enStyle;
		
		// http://en.wikipedia.org/wiki/Cap_height
		INT			ariFontBox[4];
		INT			iAscent;	// L'altezza massima al di sopra della linea di base raggiunto da glifi in questa font, escludendo l'altezza dei glifi per i caratteri accentati
		INT			iDescent;   // La profondità massima di sotto della soglia raggiunta da glifi in questa font. Il valore è un numero negativo.
		INT			iCapHeight; // La coordinata verticale della parte superiore delle lettere maiuscole piane, misurata dalla linea di base.
		INT			iStemV;		// Lo spessore, misurato in senso orizzontale, verticale della dominante steli di glifi nel font.
		INT			iTotal;

		double		dScale;	

		double		dHeightTypo;	// Altezza in PT
//		double		dHeightMs;	// Altezza in PT
//		double		dxStart;	
		double		dTextLeading;
		double		dCharSpacing;
		double		dWordSpacing;

		double		ardTextMatrix[9];
//		double		dOffset;
		double		w0; // Dimensione orizzontale raggiunta

		PWD_VAL		umPx;	// Trasformazione della matrice in valore scalato e posizionato nell'area destinata al PDF
		PWD_VAL		umPy;

		S_PDF_OBJ * psoFontFocus;		// Font
		S_PDF_OBJ * psoFontDescriptor;	// Descriptor (primo elemento dell'array)


	} S_FONT_SEL;


	typedef struct {

		EH_PWD	*	psPwd;
		PWD_SIZE	sumSource;		// Dimensioni del sorgente in UM
		PWD_SIZE	sumWork;		// Area di stampa
		PWD_SIZE	sumDest;		// Dimensioni del importazione finali
		PWD_RECT	rumDest;		// Dimensioni e posizione finali
		double		dScale;

		S_FONT_SEL  sFont;
		
	} PDF_DRAW;

#define UM_CONV_X(a) ((double) psDraw->rumDest.left+(pwdUm(PUM_PT,a)*psDraw->dScale))
#define UM_CONV_Y(a) ((double) psDraw->rumDest.top+(psDraw->sumDest.cy-(pwdUm(PUM_PT,a*psDraw->dScale))))

static void _textCursorCalc(PDF_DRAW * psDraw) {

//	PWD_VAL x,y;
	S_FONT_SEL  * psFont=&psDraw->sFont;
	double dPosX=psFont->ardTextMatrix[6]+psFont->w0;
	
//	printf("= dPosX:%.2f, %.4f. + %.4f" CRLF,dPosX,psFont->ardTextMatrix[6],psFont->w0);
	psFont->umPx=UM_CONV_X(dPosX);
	psFont->umPy=UM_CONV_Y(psFont->ardTextMatrix[7]);
	/*
	if (psFont->dTextLeading>psFont->dHeightTypo) {
		psFont->umPy+=(pwdUm(PUM_PT,(psFont->dTextLeading-psFont->dHeightTypo)*psDraw->dScale));
	}
	*/

}

static void _textCursorNext(PDF_DRAW * psDraw) {

	PWD_TXI * psTxi;
	psTxi=pwdTextInfoCreate(pwdGetObj(psDraw->psPwd->psLastItemWrite),NULL);
//	psDraw->sFont.ardTextMatrix[6]+=psTxi->sumText.cx;
	psDraw->sFont.w0+=pwdUmTo(psTxi->sumText.cx,PUM_PT);
	pwdTextInfoDestroy(psTxi);


}

//
// pwdPdfFile() > Importa una pagina pdf
//

void		pwdPdfFile(	UTF8 *		pszFileName,	
						INT			iPage,
						BOOL		bImport,		// T/F se deve importare nel documento: false=Solo calcolo per posizionamento

						PWD_POINT *	ppumPos,		// Posiziono il file
						PWD_SIZE  *	psumSize,		// Indico dimensioni orizzontali
						PWD_ALIGN	enAlign,		// Posizionamento
						
						BOOL		bBestInFit,		// T/F se deve calcolare la maggiore dimensione possibile
						BOOL		bOnlyCalc,		// T/F Solo calcolo del posizionamento, usato per predeterminare l'occupazione e la dimenzione
						PWD_RECT  *	precPage)		// Ritorna l'area occupata (se richiesto)
{

	S_PDF * psPdf;
	INT a;

	S_PDF_GO * psGo;
	S_PDF_OBJ * psParam;
	S_PDF_PAGE sPageInfo;
	PDF_DRAW	sDraw, * psDraw;


	EH_PWD	*	psPwd=PowerDoc(WS_INF,0,NULL);
	EH_LST		lstPath=lstCreate(sizeof(PWD_PTHE));
	BOOL		bPathClose=false;
	PWD_COLOR	colStroke,colFill;
	PWD_VAL		pwdLineWidth=0;		// Spessore della linea
	PWD_PTHE	sPe;
	PWD_POINT * psPoint;
	
	S_FONT_SEL  * psFont;

//	_(sFont);
	
	psPdf=pdfLoadEx(pszFileName); if (!psPdf) return;
	if (pdfGetPage(psPdf,iPage,&sPageInfo)) ehError();

	_(sDraw); 
	psDraw=&sDraw;
	psDraw->psPwd=psPwd;
	psFont=&sDraw.sFont;
	psFont->dHeightTypo=10.0;//pwdUm(PUM_PT,10);//10.0;xxx
	psFont->dCharSpacing=0.0;

	//
	// Dimensioni del sorgente
	//
	psDraw->sumSource.cx=pwdUm(PUM_PT,sPageInfo.rcdRect.right - sPageInfo.rcdRect.left);
	psDraw->sumSource.cy=pwdUm(PUM_PT,sPageInfo.rcdRect.top - sPageInfo.rcdRect.bottom);
	
	//
	// Calcolo le dimensioni e la posizione della pagina importata
	//
	if (psumSize) memcpy(&psDraw->sumWork,psumSize,sizeof(PWD_SIZE)); //else {memcpy(&sumWork,&psPwd->sumPage,sizeof(PWD_SIZE)); bBestInFit=true;}

	//
	//  bBestInFit - Inserisce e centra nello spazio disponibile
	//
	if (bBestInFit) {

		//
		// Calcolo dimensioni della massima area stampabile
		//

		if (!psumSize) memcpy(&psDraw->sumWork,&psPwd->sumPage,sizeof(PWD_SIZE));

		//
		// Scelgo la scala più appropriata (best in fit) 
		// Per stampare il template nell'area di stampa massima
		//
		if (!psDraw->sumWork.cx) psDraw->dScale = (double) psDraw->sumWork.cy / psDraw->sumSource.cy;
		else if (!psDraw->sumWork.cy) psDraw->dScale = (double) psDraw->sumWork.cx / psDraw->sumSource.cx;
		else psDraw->dScale = min ((double) psDraw->sumWork.cx / psDraw->sumSource.cx, (double) psDraw->sumWork.cy / psDraw->sumSource.cy) ;


	} else {
		
//		memcpy(&_sMf.sumArea,&sumSource,sizeof(sumSource));
//		pwdSizeCalc(&_sMf.sumArea,prumPrintArea);
		psDraw->dScale=1;


	}

	//
	// Ridimensiono il template in scala
	// 
	// printf("Dimensioni meta: %.2fmm x %.2fmm (zoom: %.2f%%)" CRLF,sumSource.cx,sumSource.cy,dScale*100);
	psDraw->sumDest.cx = (psDraw->dScale * psDraw->sumSource.cx) ; if (!psDraw->sumWork.cx)  psDraw->sumWork.cx=psDraw->sumDest.cx;
	psDraw->sumDest.cy = (psDraw->dScale * psDraw->sumSource.cy) ; if (!psDraw->sumWork.cy)  psDraw->sumWork.cy=psDraw->sumDest.cy;

	//
	// Trasformo la dimensione Reale in pixel 
	//
	if (!enAlign) enAlign=PDA_CENTER|PDA_MIDDLE;
	
	switch (enAlign&0xf) {
		
		case PDA_LEFT:
			if (ppumPos) 
					psDraw->rumDest.left=ppumPos->x; 
					else
					psDraw->rumDest.left=psPwd->rumPage.left;
			break;


		case PDA_CENTER:
			
			if (ppumPos) // Posiziono e centro al rettangolo indicato
					psDraw->rumDest.left=ppumPos->x+(psDraw->sumWork.cx-psDraw->sumDest.cx)/2; 
					else
					psDraw->rumDest.left=psPwd->rumPage.left+(psPwd->sumPage.cx-psDraw->sumDest.cx)/2; 
			break;

		case PDA_RIGHT:
			if (ppumPos) 
					psDraw->rumDest.left=ppumPos->x+psDraw->sumWork.cx-psDraw->sumDest.cx; 
					else
					psDraw->rumDest.left=psPwd->rumPage.right-psDraw->sumDest.cx; 
			break;

	
	}

	switch (enAlign&0xf0) {
	
		case PDA_TOP:
			if (ppumPos) 
				psDraw->rumDest.top=ppumPos->y; 
				else
				psDraw->rumDest.top=psPwd->rumPage.top;
			break;

		case PDA_MIDDLE:
			if (ppumPos) 
				psDraw->rumDest.top=ppumPos->y+(psDraw->sumWork.cy-psDraw->sumDest.cy)/2; 
				else
				psDraw->rumDest.top=psPwd->rumPage.top+(psPwd->sumPage.cy-psDraw->sumDest.cy)/2; 
			break;

		case PDA_BOTTOM:
			if (ppumPos) 
				psDraw->rumDest.top=ppumPos->y+psDraw->sumWork.cy-psDraw->sumDest.cy; 
				else
				psDraw->rumDest.top=psPwd->rumPage.bottom-psDraw->sumDest.cy; 
			break;

	}			
	
	psDraw->rumDest.right=psDraw->rumDest.left+psDraw->sumDest.cx;
	psDraw->rumDest.bottom=psDraw->rumDest.top+psDraw->sumDest.cy;

	// Ritorno le dimensioni di stampa
	if (precPage) memcpy(precPage,&psDraw->rumDest,sizeof(PWD_RECT));

	//
	// Importo la pagina
	//
	if (bImport) {

		for (lstLoop(sPageInfo.lstGo,psGo)) {

		// 	printf("%-3.3s : %s :",psGo->szOriginal,psGo->pszOperation);

			// BDC/EMC definiscono un layer del documento
			if (!*psGo->szOriginal) continue;

			//
			// Non usati
			//
			if (!*psGo->szOriginal||
				!strcmp(psGo->szOriginal,"BDC")||	// Begin Sequence
				!strcmp(psGo->szOriginal,"EMC")||	// Ende	Sequence
				!strcmp(psGo->szOriginal,"gs")||
				!strcmp(psGo->szOriginal,"Q")||
				!strcmp(psGo->szOriginal,"q")||
				!strcmp(psGo->szOriginal,"J")||	// lineCap (non gestito)
				!strcmp(psGo->szOriginal,"j")|| // lineJoin (non gestito)
				!strcmp(psGo->szOriginal,"MP")) { 

			}
			
			//
			// PATH (group) #########################################################
			//

			//
			// moveTo
			// 
			else if (!strcmp(psGo->szOriginal,"m")) { 

				if (bPathClose) {bPathClose=false; pwdPathFree(lstPath,false);}
				_(sPe);
				sPe.enType=PHT_MOVETO;
				sPe.dwSizeData=sizeof(PWD_POINT);
				psPoint=sPe.psData=ehAlloc(sPe.dwSizeData);
				
				psParam=lstGet(psGo->lstParams,0); 
				psPoint->x=UM_CONV_X(atof(psParam->pbValue));
				psParam=lstGet(psGo->lstParams,1); 
				psPoint->y=UM_CONV_Y(atof(psParam->pbValue));
				lstPush(lstPath,&sPe);
			}
			//
			// lineTo
			// 
			else if (!strcmp(psGo->szOriginal,"l")) { 
				
				if (bPathClose) {bPathClose=false; pwdPathFree(lstPath,false);}

				_(sPe);
				sPe.enType=PHT_LINETO;
				sPe.dwSizeData=sizeof(PWD_POINT);
				psPoint=sPe.psData=ehAlloc(sPe.dwSizeData);

				psParam=lstGet(psGo->lstParams,0); 
				psPoint->x=UM_CONV_X(atof(psParam->pbValue));
				psParam=lstGet(psGo->lstParams,1); 
				psPoint->y=UM_CONV_Y(atof(psParam->pbValue));
				lstPush(lstPath,&sPe);
			}

			//
			// cubicBézierCurve
			// 
			else if (!strcmp(psGo->szOriginal,"c")) { 
			
				PWD_BEZIER * psBezier;
				INT i;

				_(sPe);
				sPe.enType=PHT_POLYBEZIERTO;
				sPe.dwSizeData=sizeof(PWD_BEZIER)+(sizeof(PWD_POINT)*3);
				sPe.psData=psBezier=ehAllocZero(sPe.dwSizeData);
				psBezier->cCount=3;
				a=0;
				for (i=0;i<(INT) psBezier->cCount;i++) {

					psParam=lstGet(psGo->lstParams,a); psBezier->arsPoint[i].x=UM_CONV_X(atof(psParam->pbValue)); a++;
					psParam=lstGet(psGo->lstParams,a); psBezier->arsPoint[i].y=UM_CONV_Y(atof(psParam->pbValue)); a++;

				}
				lstPush(lstPath,&sPe);

			}

			//
			// cubicBézierCurve
			// 
			else if (!strcmp(psGo->szOriginal,"v")||
				!strcmp(psGo->szOriginal,"y")) { 
			
				ehExit("Path [%s:%s] da implementare",psGo->szOriginal,psGo->pszOperation);

			}

			//
			// clippingPath > Setta un area di clipping
			// 
			else if (!strcmp(psGo->szOriginal,"W*")) { 

				// Da gestire
			}
			//
			// closeSubPath > Chiude il percorso
			// 
			else if (!strcmp(psGo->szOriginal,"h")) { 
			
				bPathClose=true;
			}
			//
			// endPath > fine della definizione di un percorso
			// 
			else if (!strcmp(psGo->szOriginal,"n")) { 
				
				pwdPathFree(lstPath,false);

			}
			//
			// strokeRGB > Colore del tratto in RGB
			// 
			else if (!strcmp(psGo->szOriginal,"RG")) { 

				EH_COLORD col;
				psParam=lstGet(psGo->lstParams,0); col.dRed=atof(psParam->pbValue);
				psParam=lstGet(psGo->lstParams,1); col.dGreen=atof(psParam->pbValue);
				psParam=lstGet(psGo->lstParams,2); col.dBlue=atof(psParam->pbValue);

				colStroke=pwdRGB(col.dRed,col.dGreen,col.dBlue,1.0);

			}
			//
			// fillRGB > Colore del tratto in RGB
			// 
			else if (!strcmp(psGo->szOriginal,"rg")) { 

				EH_COLORD col;
				psParam=lstGet(psGo->lstParams,0); col.dRed=atof(psParam->pbValue);
				psParam=lstGet(psGo->lstParams,1); col.dGreen=atof(psParam->pbValue);
				psParam=lstGet(psGo->lstParams,2); col.dBlue=atof(psParam->pbValue);
				colFill=pwdRGB(col.dRed,col.dGreen,col.dBlue,1.0);

			}
			//
			// strokeGray > Riempe il percorso
			// 
			else if (!strcmp(psGo->szOriginal,"G")) { 
			
				double dVal;
				psParam=lstGet(psGo->lstParams,0); dVal=atof(psParam->pbValue);
				//colStroke=pwdRGB(dVal*255,dVal*255,dVal*255,1.0);
				colStroke=pwdCMYK(0,0,0,dVal*255,1.0);

			}
			//
			// fillGray > Riempe il percorso
			// 
			else if (!strcmp(psGo->szOriginal,"g")) { 
			
				double dVal;
				psParam=lstGet(psGo->lstParams,0); dVal=atof(psParam->pbValue);
				//colFill=pwdRGB(dVal*255,dVal*255,dVal*255,1.0);
				colFill=pwdCMYK(0,0,0,dVal*255,1.0);

			}
			//
			// lineWidth > Spessore della linea (del tratto)
			// 
			else if (!strcmp(psGo->szOriginal,"w")) { 

				psParam=lstGet(psGo->lstParams,0); 
				pwdLineWidth=pwdUm(PUM_PT,atof(psParam->pbValue))*psDraw->dScale;
			
			}

			//
			// DISEGNO #######################################################################################
			//

			//
			// fillStrokePath > Riempe il percorso
			// 
			else if (!strcmp(psGo->szOriginal,"B*")) { 
			
				pwdPath(lstPath,colStroke,pwdLineWidth,0,colFill,0);

			}
			//
			// fillPath > Riempe il percorso
			// 
			else if (!strcmp(psGo->szOriginal,"f*")||
				!strcmp(psGo->szOriginal,"f")) { 
			
				pwdPath(lstPath,pwdCMYK(0,0,0,0,0),0,0,colFill,0);

			}
			//
			// strokePath > Disegna
			// 
			else if (!strcmp(psGo->szOriginal,"S")) { 
			
				pwdPath(lstPath,colStroke,pwdLineWidth,0,pwdCMYK(0,0,0,0,0),0);

			}
			else if (	!strcmp(psGo->szOriginal,"s")||
						// !strcmp(psGo->szOriginal,"f")||
						!strcmp(psGo->szOriginal,"F")||
						!strcmp(psGo->szOriginal,"B")||
						!strcmp(psGo->szOriginal,"b")||
						!strcmp(psGo->szOriginal,"b*")||
						!strcmp(psGo->szOriginal,"n")) {

					ehExit("Disegno [%s:%s] da implementare",psGo->szOriginal,psGo->pszOperation);
			
			}

			//
			// TESTO #######################################################################################
			//

			//
			//  textStartMatrix
			//
			else if (!strcmp(psGo->szOriginal,"BT")) {

				psFont->ardTextMatrix[0]=1;
				psFont->ardTextMatrix[1]=0;
				psFont->ardTextMatrix[2]=0;
				
				psFont->ardTextMatrix[3]=0;
				psFont->ardTextMatrix[4]=1;
				psFont->ardTextMatrix[5]=0;

				psFont->ardTextMatrix[6]=0;
				psFont->ardTextMatrix[7]=0;
				psFont->ardTextMatrix[8]=1;
			}
			//
			// textEndMatrix
			//
			else if (!strcmp(psGo->szOriginal,"ET")) {
			
				_(psFont->ardTextMatrix);

			}
			//
			// Text state parameters (pag. 302)
			//
			// textLeading > Leading = Distanza tra le linee | 5.2.4 Pag. 304
			//
			else if (!strcmp(psGo->szOriginal,"TL")) { // leading
			
				psParam=lstGet(psGo->lstParams,0); 
				psFont->dTextLeading=atof(psParam->pbValue);
			
			}
			else if (	!strcmp(psGo->szOriginal,"Tc")|| // charSpace
						!strcmp(psGo->szOriginal,"Tw")|| // wordSpace
						!strcmp(psGo->szOriginal,"Tz")|| // scale
						!strcmp(psGo->szOriginal,"Tr")||
						!strcmp(psGo->szOriginal,"Ts")) {

					ehExit("Testo [%s:%s] da implementare",psGo->szOriginal,psGo->pszOperation);
			
			}
			//
			// fontSize > Setta il font da usare e la dimensione di default
			//
			else if (!strcmp(psGo->szOriginal,"Tf")) {

				CHAR szServ[100];
				CHAR * pszFontFocus;
				CHAR * pszName;

			// 	_showParams(psGo->lstParams);

				psParam=lstGet(psGo->lstParams,0); 
				pszFontFocus=psParam->pbValue;

				//
				// Estraggo le caratteristiche del font da usare con i testi (Pag. 375)
				//
				sprintf(szServ,".%s",pszFontFocus);
				psFont->psoFontFocus=pdfObjGet(sPageInfo.psFonts,szServ); if (!psFont->psoFontFocus) ehExit("Non trovo il font %s",szServ);

				psFont->psoFontDescriptor=pdfObjGet(psFont->psoFontFocus,".DescendantFonts[0].FontDescriptor");
				if (!psFont->psoFontDescriptor) ehExit("Non trovo il font %s",szServ);

				//
				// Questo è tecnicamente un "TAPULLO"
				//
				// I fonts sono nel file, ma non li uso perché è troppo complicato
				// Ricerco i font in memoeria usanto una tecnica spartana 
				// NOTA: Dovrei in realtà caricare il font con un nome nuovo (assegnato nel file) e usare quello 
				// 
				pszName=pdfObjGetValue(psFont->psoFontDescriptor,".FontName","");
				psFont->enStyle=STYLE_NORMAL;

				if (!strCmp(pszName,"ArialMT")) {

					strcpy(psFont->szName,"Arial");

				}
				else if (!strCmp(pszName,"Arial-BoldMT")) {
				
					strcpy(psFont->szName,"Arial");
					psFont->enStyle=STYLE_BOLD;

				}
				else if (!strCmp(pszName,"Arial-ItalicMT")) {
				
					strcpy(psFont->szName,"Arial");
					psFont->enStyle=STYLE_ITALIC;
				}
				else if (!strCmp(pszName,"CenturyGothic")) {
					
					strcpy(psFont->szName,pszName);
				} 
				else if (!strCmp(pszName,"CenturyGothic-Bold")) {
					
					strcpy(psFont->szName,pszName);
					psFont->enStyle=STYLE_BOLD;
				} 
				else {
					
					strcpy(psFont->szName,pszName);
					if (!strEnd(psFont->szName,"MT")) psFont->szName[strlen(psFont->szName)-2]=0;
				
				}
	 			
				//
				// Leggo parametri del font
				//
				// pdfObjShow(sFont.psoFontFocus,0,false,true);
			//  	pdfObjShow(sFont.psoFontDescriptor,0,false,true);

				psFont->iAscent=atoi(pdfObjGetValue(psFont->psoFontDescriptor,".Ascent","0"));
				psFont->iDescent=atoi(pdfObjGetValue(psFont->psoFontDescriptor,".Descent","0"));
				psFont->iCapHeight=atoi(pdfObjGetValue(psFont->psoFontDescriptor,".CapHeight","0"));
				psFont->iStemV=atoi(pdfObjGetValue(psFont->psoFontDescriptor,".StemV","0"));
				psFont->ariFontBox[0]=atoi(pdfObjGetValue(psFont->psoFontDescriptor,".FontBBox[0]","0"));
				psFont->ariFontBox[1]=atoi(pdfObjGetValue(psFont->psoFontDescriptor,".FontBBox[1]","0"));
				psFont->ariFontBox[2]=atoi(pdfObjGetValue(psFont->psoFontDescriptor,".FontBBox[2]","0"));
				psFont->ariFontBox[3]=atoi(pdfObjGetValue(psFont->psoFontDescriptor,".FontBBox[3]","0"));
				//psFont->iTotal=psFont->iAscent-psFont->iDescent;
				psFont->iTotal=psFont->ariFontBox[3];//-psFont->ariFontBox[1];

				psParam=lstGet(psGo->lstParams,1); 
				psFont->dScale=atof(psParam->pbValue);

				// L'altezza di windows è tutto, quella del PDF il carattere da baseline
				// height:psFont->iAscent=x:psFont->iTotal
				psFont->dHeightTypo=psFont->dScale;
	//			psFont->dHeightMs=(psFont->dHeightReal*psFont->iTotal)/psFont->iAscent;
					

			}
			//
			// textMatrix > Matrice del testo (?)
			//
			else if (!strcmp(psGo->szOriginal,"Tm")) {

				psParam=lstGet(psGo->lstParams,0); psFont->ardTextMatrix[0]=atof(psParam->pbValue);
				psParam=lstGet(psGo->lstParams,1); psFont->ardTextMatrix[1]=atof(psParam->pbValue);
				psFont->ardTextMatrix[2]=0;
				psParam=lstGet(psGo->lstParams,2); psFont->ardTextMatrix[3]=atof(psParam->pbValue);
				psParam=lstGet(psGo->lstParams,3); psFont->ardTextMatrix[4]=atof(psParam->pbValue);
				psFont->ardTextMatrix[5]=0;
				psParam=lstGet(psGo->lstParams,4); psFont->ardTextMatrix[6]=atof(psParam->pbValue);
				psParam=lstGet(psGo->lstParams,5); psFont->ardTextMatrix[7]=atof(psParam->pbValue);
				psFont->ardTextMatrix[8]=1;
				psFont->w0=0;

			}
			//
			// textMatrix > Posiziona la scritta
			//
			else if (!strcmp(psGo->szOriginal,"Td")) {

				psFont->ardTextMatrix[0]=1;
				psFont->ardTextMatrix[1]=0;
				psFont->ardTextMatrix[2]=0;
				psFont->ardTextMatrix[3]=0;
				psFont->ardTextMatrix[4]=1;
				psFont->ardTextMatrix[5]=0;
				psParam=lstGet(psGo->lstParams,4); psFont->ardTextMatrix[6]=atof(psParam->pbValue); // X
				psParam=lstGet(psGo->lstParams,5); psFont->ardTextMatrix[7]=atof(psParam->pbValue); // Y
				psFont->ardTextMatrix[8]=1;
				psFont->w0=0;

			}
			//
			// textNextline >
			//
			else if (!strcmp(psGo->szOriginal,"T*")) {
				
				//psFont->dOffset=0;
				_showParams(psGo->lstParams);
				psFont->w0=0;
				//psFont->ardTextMatrix[6]=psFont->dxStart;
				psFont->ardTextMatrix[7]-=psFont->dTextLeading;
	/*
				if (psFont->dTextLeading>psFont->dHeightTypo) {
					psFont->ardTextMatrix[7]-=psFont->dTextLeading-psFont->dHeightTypo;
				}
	*/
			
			}

			//
			// textShow > Visualizza un testo posizionato
			//
			else if (!strcmp(psGo->szOriginal,"Tj" )) {
			
				psParam=lstGet(psGo->lstParams,0); 	
				if (psParam) {

					_textCursorCalc(psDraw);
					pwdText(psFont->umPx,	// Coordinate
							psFont->umPy,
							colFill,		// Colore del riempimento
							psFont->enStyle,	// Stile -> da definire 
							DPL_LEFT|DPL_BASELINE|DPL_HEIGHT_TYPO,		// Allinamento -> da definire  
							psFont->szName,		// Font (da trovare)
							pwdUm(PUM_PT,psFont->dHeightTypo),// Dimensioni (da trovare)	
							psParam->pbValue);
					_textCursorNext(psDraw);
				}

			}

			//
			// textShow > Visualizza un testo in array
			//
			else if (!strcmp(psGo->szOriginal,"TJ")) {
			
				S_PDF_OBJ * psObj;
			//	_showParams(psGo->lstParams);

				psParam=lstGet(psGo->lstParams,0);
				for (psObj=psParam->psArray;psObj;psObj=psObj->psNext) {

					switch (psObj->enType)	
					{
						case POT_NUMERIC:
							//psFont->umPx+=pwdUm(PUM_PT,atof(psObj->pbValue)*psDraw->dScale/1000);
							{
								double dTj=atof(psObj->pbValue);
								psFont->w0=((psFont->w0-(dTj/1000)*psFont->dScale+psFont->dCharSpacing+0)*1);
							}
							break;

						case POT_STRING:

							_textCursorCalc(psDraw);
							pwdText(psFont->umPx,	// Coordinate
									psFont->umPy,
									colFill,		// Colore del riempimento
									psFont->enStyle,	// Stile -> da definire 
									DPL_LEFT|DPL_BASELINE|DPL_HEIGHT_TYPO,		// Allinamento -> da definire  
									psFont->szName,		// Font (da trovare)
									pwdUm(PUM_PT,psFont->dHeightTypo),// Dimensioni (da trovare)	
									psObj->pbValue);
							_textCursorNext(psDraw);
							
							break;

						default:
							ehError();
					}
				}

			}




			else {

				_showParams(psGo->lstParams);
		//		printf("?");
			}

		//	printf(CRLF);
		}
	}

	pwdPathFree(lstPath,true);
	pdfFree(psPdf);

}