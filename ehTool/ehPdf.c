// ------------------------------------------------
//  ehPdf
//  Apre, modifica e scrive un PDF
//  
//                               by Ferrà srl 2011
// ------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehPdf.h"
#include "/easyhand/ehtoolx/Zlib/zlib.h"


static void		_pdfInflate(S_PDF * pszPdf);
static BYTE *	_binSearch(BYTE * pbStart, BYTE * pbEnd,BYTE * pbSearch,INT iSize) {

	BYTE * pb=pbStart;
	pbEnd-=iSize;
	for (pb=pbStart;pb<pbEnd;pb++) {
		if (!memcmp(pb,pbSearch,iSize)) return pb;
	}
	return NULL;

}

static BYTE * _binReverseSearch(BYTE * pbStart, BYTE * pbMin,BYTE * pbSearch,INT iSize) {

	BYTE * pb=pbStart;
	pbStart-=iSize;
	for (pb=pbStart;pb>=pbMin;pb--) {
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
	ZeroFill(szChunk);
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
	BYTE * pbFile,* pbEndFile;
//	BYTE * pbNext;
	BYTE * pbObj, * pbObjEnd, * pb;
//	BYTE * pbLast;
	SIZE_T tSize;
//	INT64 i;
	S_PDF * psPdf;
	S_PDFPART sPart;
	INT a;

	psPdf=ehAllocZero(sizeof(S_PDF));
	DMIReset(&psPdf->dmiPart);

	//
	// Apro il file
	//
	pbFile=fileMemoRead(pszFileName,&tSize); if (!pbFile) ehExit("Il file %s non esiste",pszFileName);
	pb=pbFile; pbEndFile=pb+tSize-1;
//	pbLast=pbFile;


	//
	// Preconto gli oggetti
	//
	for (a=0;;a++) {
	
		pbObj=_binSearch(pb,pbEndFile," obj\r",4); if (!pbObj) break;
		pb=pbObj+4;

	}
	pb=pbFile;
	DMIOpen(&psPdf->dmiPart,RAM_AUTO,a+10,sizeof(S_PDFPART),"pdfPart");

	while (true) {

		BYTE * pbx;
		//
		// Cerco l'oggetto
		//
		pbObj=_binSearch(pb,pbEndFile," obj\r",5); 

		//
		// Abbiamo finito
		//
		if (!pbObj) {
		
			//
			// Memorizzo la coda
			//
			ZeroFill(sPart);
			sPart.tSize=((SIZE_T) pbEndFile-(SIZE_T) pb);
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
		pbx=_binReverseSearch(pbObj,pbFile,"\r",1); 
		if (!pbx) 
			ehError(); // Errore
		pbObj=pbx;
		if (pbObj[1]==LF) pbObj+=2; else pbObj+=1;

		//
		// "Pezzo" sconosciuto tra un oggetto e l'altro
		//
		ZeroFill(sPart);
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
		pbObjEnd=_binSearch(pbObj,pbEndFile,"endobj\r",7); 
		if (!pbObjEnd) 
			ehError();
		pbObjEnd+=7;


		// L'oggetto
		ZeroFill(sPart);
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
						pbStart=_binSearch(psPart->pbPart,psPart->pbPart+psPart->tSize-1,"\rstream\n",8); if (!pbStart) ehError();
						pbStart+=8;
						pbEnd=_binSearch(pbStart,psPart->pbPart+psPart->tSize-1,"\nendstream\r",11); if (!pbEnd) ehError();
						
						pbRet=_deflate(pbStart,(SIZE_T) pbEnd-(SIZE_T) pbStart,&tSize);
						if (pbRet) {
							if (*pbRet!=0) {
								
								psPart->enType=PDFT_DEFLATED;
								psPart->pszDecode=pbRet;
							}
							else ehFree(pbRet);
						
						}
/*
						if (strstr(psPart->pszDecode,"@#CODE#@")) {
							printf("qui");
						}
*/
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
						pszFooter=_binSearch(psPart->pbPart,psPart->pbPart+psPart->tSize-1,"\nendstream\r",11);

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

						/*
						BYTE *p=NULL;
						BYTE *pb=NULL;
						BYTE *pbHead=NULL;
						BYTE *pbSPart=NULL;
						BYTE *pbPartComp=NULL;    // nuovo oggetto						INT iNewSizeObj,iLenEnd,iLenStart, iLenLength,iLenSize,iLenHead;
						CHAR *pEnd="\nendstream\rendobj\r";
						CHAR *pszLength="/Length ";
						
						INT iSizeComp=sZstrm.total_out;
						CHAR szSize[20];

						p=psPart->pbPart;
						
						iLenLength=strlen(pszLength);
						pb=_binSearch(p,p+psPart->tSize-1,pszLength,iLenLength); if (!pb) ehError();
						pb+=iLenLength;

						//pb=_binSearch(p,p+psPart->tSize-1,"\rstream\n",8); if (!pb) ehError();
						iLenStart=(SIZE_T) pb-(SIZE_T) p;

						sprintf(szSize,"%d",iSizeComp);
						iLenSize=strlen(szSize);

						while (*pb!='/'&&*pb!='>') { 
							pb++; 
						}

						
						iLenHead=iLenStart+iLenSize;
						pbHead=ehAllocZero(iLenHead);
						
						//pbSPart=ehAlloc(iLen+);
						
						iLenEnd=strlen(pEnd);

						// ricostruzione oggetto
						iNewSizeObj=iLenStart+iSizeComp+iLenEnd;
						pbPartComp=ehAllocZero(iNewSizeObj);

						memcpy(pbPartComp,p,iLenStart);
						pbPartComp+=iLenStart;

						memcpy(pbPartComp,pBufferComp,iSizeComp);
						pbPartComp+=iSizeComp;

						memcpy(pbPartComp,pEnd,iLenEnd);

						// scrittura su file
						fileWrite(psFile,pbPartComp,iNewSizeObj);

						deflateEnd(&sZstrm);
					}
				}
				
				ehFree(pBufferComp);

*/

//
// pdfFree()
//
BOOL pdfFree(S_PDF * psPdf) {

	INT a;

	for (a=0;a<psPdf->dmiPart.Num;a++) {
		ehFreeNN(psPdf->arsPart[a].pbPart);
		ehFreeNN(psPdf->arsPart[a].pszDecode);
	}
	DMIClose(&psPdf->dmiPart,"pdfPart");
	ehFree(psPdf);
	return FALSE;

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


