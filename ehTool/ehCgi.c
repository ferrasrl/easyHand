#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehCgi.h"
#include <fcntl.h>

/*
#ifdef _WIN32
#define strcasecmp strCaseCmp
#define strncasecmp _memicmp
#endif
// gzip,deflate,sdch | gzip, deflate
*/

static EH_CGI_PTR	_getEntryFromString(char *query_string);
static void			_getWord(EH_CGI_PTR *ptr, EH_CGI_PTR *prev, char *qs);
static EH_CGI_PTR	_getEntryFromMultipart(int iLength, char *lpBoundary);
static EH_CGI_PTR	_getEntryFromStream(int length);
static void			_plusToSpace(char *str);
static void			_unescapeUrl(char *url);
static void			_makeWord(EH_CGI_PTR *ptr, EH_CGI_PTR *prev, char stop, int *cl);
static void			_freeEntryList(EH_CGI_PTR p);

static CHAR * _getenv(CHAR * pszParam, CHAR ** envp) {

	INT iLen;
	CHAR ** p;
	if (!envp) return getenv(pszParam);

	iLen = strlen(pszParam);

    for (p = envp; *p; ++p) {
        if((strncmp(pszParam, *p, iLen) == 0) && ((*p)[iLen] == '=')) {
            return *p+iLen+1;
        }
    }
    return NULL;
}


//
// cgiOpen()
//
void cgiOpen(EH_CGI_INFO * psCgi, BOOL bGetParams, CHAR ** envp)
{
	char * strlen;
 
	memset(psCgi,0,sizeof(EH_CGI_INFO));
	psCgi->content_type = _getenv("CONTENT_TYPE",envp);
	if (psCgi->content_type) {if (!*psCgi->content_type) psCgi->content_type=NULL;}
	strlen = _getenv("CONTENT_LENGTH",envp);
	if (strlen) psCgi->content_length=atoi(strlen); else psCgi->content_length=0;
	psCgi->server_name = _getenv("SERVER_NAME",envp);
	psCgi->server_software = _getenv("SERVER_SOFTWARE",envp);
	psCgi->server_protocol = _getenv("SERVER_PROTOCOL",envp);
	psCgi->server_port = _getenv("SERVER_PORT",envp);
	psCgi->gateway_interface = _getenv("GATEWAY_INTERFACE",envp);
	psCgi->request_method = _getenv("REQUEST_METHOD",envp);
	psCgi->http_accept = _getenv("HTTP_ACCEPT",envp);
	psCgi->path_info = _getenv("PATH_INFO",envp);
	psCgi->path_translated = _getenv("PATH_TRANSLATED",envp);
	psCgi->script_name = _getenv("SCRIPT_NAME",envp);
	psCgi->query_string = _getenv("QUERY_STRING",envp);
	psCgi->remote_host = _getenv("REMOTE_HOST",envp);
	psCgi->remote_addr = _getenv("REMOTE_ADDR",envp);
	psCgi->remote_user = _getenv("REMOTE_USER",envp);
	psCgi->remote_ident = _getenv("REMOTE_IDENT",envp);
	psCgi->auth_type = _getenv("AUTH_TYPE",envp);
	psCgi->http_user_agent = _getenv("HTTP_USER_AGENT",envp);
	psCgi->http_refeer = _getenv("HTTP_REFERER",envp);
	psCgi->http_accept_language =_getenv("HTTP_ACCEPT_LANGUAGE",envp);
	psCgi->http_accept_encoding =_getenv("HTTP_ACCEPT_ENCODING",envp);
	if (psCgi->http_accept_encoding) {if (strCaseStr(psCgi->http_accept_encoding,"gzip")) psCgi->bGzip=true;};
	psCgi->http_cookie=_getenv("HTTP_COOKIE",envp);

	if (bGetParams) psCgi->psParams=cgiBuildParams(psCgi);

}

//
// cgiClose()
//
void cgiClose(EH_CGI_INFO * psCgi) {

	_freeEntryList(psCgi->psParams);
}

//
// cgiShowInfo()
//
void cgiShowInfo(EH_CGI_INFO * psCgi)
{
	ehPrintf("CONTENT_TYPE: [<b>%s</b>]<br />",strEver(psCgi->content_type));
	ehPrintf("CONTENT_LENGTH: %d<br />",psCgi->content_length);
	ehPrintf("SERVER_NAME: [<b>%s</b>]<br />",strEver(psCgi->server_name));
	ehPrintf("SERVER_SOFTWARE: [<b>%s</b>]<br />",strEver(psCgi->server_software));
	ehPrintf("SERVER_PROTOCOL: [<b>%s</b>]<br />",strEver(psCgi->server_protocol));
	ehPrintf("SERVER_PORT: [<b>%s</b>]<br />",strEver(psCgi->server_port));
	ehPrintf("GATEWAY_INTERFACE: [<b>%s</b>]<br />",strEver(psCgi->gateway_interface));
	ehPrintf("REQUEST_METHOD: [<b>%s</b>]<br />",strEver(psCgi->request_method));
	ehPrintf("HTTP_ACCEPT: [<b>%s</b>]<br />",strEver(psCgi->http_accept));
	ehPrintf("PATH_INFO: [<b>%s</b>]<br />",strEver(psCgi->path_info));
	ehPrintf("PATH_TRANSLATED: [<b>%s</b>]<br />",strEver(psCgi->path_translated));
	ehPrintf("SCRIPT_NAME: [<b>%s</b>]<br />",strEver(psCgi->script_name));
	ehPrintf("QUERY_STRING: [<b>%s</b>]<br />",strEver(psCgi->query_string));
	ehPrintf("REMOTE_HOST: [<b>%s</b>]<br />",strEver(psCgi->remote_host));
	ehPrintf("REMOTE_ADDR: [<b>%s</b>]<br />",strEver(psCgi->remote_addr));
	ehPrintf("REMOTE_USER: [<b>%s</b>]<br />",strEver(psCgi->remote_user));
	ehPrintf("REMOTE_IDENT: [<b>%s</b>]<br />",strEver(psCgi->remote_ident));
	ehPrintf("AUTH_TYPE: [<b>%s</b>]<br />",strEver(psCgi->auth_type));
	ehPrintf("HTTP_USER_AGENT: [<b>%s</b>]<br />",strEver(psCgi->http_user_agent));
	ehPrintf("HTTP_REFEER: [<b>%s</b>]<br />",strEver(psCgi->http_refeer));
	ehPrintf("HTTP_ACCEPT_LANGUAGE: [<b>%s</b>]<br />",strEver(psCgi->http_accept_language));
	ehPrintf("HTTP_ACCEPT_ENCODING: [<b>%s</b>]<br />",strEver(psCgi->http_accept_encoding));
	ehPrintf("HTTP_COOKIE: [<b>%s</b>]<br />",strEver(psCgi->http_cookie));
	ehPrintf("GZIP: [<b>%d</b>]<br />",psCgi->bGzip);
}

//
// cgiShowParams()
//
void cgiShowParams(EH_CGI_INFO * psCgi)
{
	EH_CGI_PTR t,ps;
	ps=psCgi->psParams;
	if (!ps) return;
	
	while (ps)
	{
		t = ps->link;
		//printf("%s=%s|%s|<br />",p->name?p->name:"(null)",p->val,p->other);
		ehPrintf("%s=[%s]<br />",
				ps->name?ps->name:"(null)",
				ps->val?ps->val:"(null)"
				);
		ps = t;
	}
}

//
// cgiMimeHeader()
//
void cgiMimeHeader(char *mime)
{
	printf("Content-Type: %s\n\n", mime);
}

//
// cgiHtmlHeader()
// 
void cgiHtmlHeader(CHAR * pszHead)
{
	cgiMimeHeader("text/html");
	/*
	printf("<html>\n<head>\n<title>%s</title>\n</head>\n", pszHead?pszHead:"");
	printf("<body>\n");
	*/
}

//
// cgiHtmlFooter()
//
void cgiHtmlFooter(char * pszLogo, char * pszText) 
{
	puts("<hr>");
	if (!strEmpty(pszLogo)) printf("<img src=\"%s\" />", pszLogo);
	if (!strEmpty(pszText)) puts(pszText);
	printf("</body>\n</html>\n");
}

//
// cgiGetParams()
//
EH_CGI_PTR cgiBuildParams(EH_CGI_INFO * psCgi) {

	
	if (psCgi->content_type)
	{
		if (!memcmp(psCgi->content_type,"multipart/form-data",19))
		{
			char *lpBoundary;
			lpBoundary=strstr(psCgi->content_type,"boundary=");
			if (!lpBoundary) return NULL; else lpBoundary+=9;
			return _getEntryFromMultipart(psCgi->content_length, lpBoundary);
		}
		
		//
		// Metodo classico di passaggio dati CGI
		//
		if (!strBegin(psCgi->content_type,"application/x-www-form-urlencoded")) {
			
			if (!strCaseCmp(psCgi->request_method, "POST")) return _getEntryFromStream(psCgi->content_length);
			if (!strCaseCmp(psCgi->request_method, "GET")) return _getEntryFromString(psCgi->query_string);

		}
	}
	else if (psCgi->request_method) {

		if (!strCaseCmp(psCgi->request_method, "POST")) return _getEntryFromStream(psCgi->content_length);
		if (!strCaseCmp(psCgi->request_method, "GET")) return _getEntryFromString(psCgi->query_string);

	}
	return NULL;

}

//
// query_string Extract
//
static EH_CGI_PTR _getEntryFromString(char *query_string) {

	int i;
	EH_CGI_PTR ptr = NULL;
	EH_CGI_PTR prev = NULL;

	if (!query_string) return NULL;

	//printf("Entro qui");
	for (i=0; *query_string; i++)
	{
		_getWord(&ptr, &prev, query_string);
	}
	return ptr;

}

//
// _getWord()
//
static void _getWord(EH_CGI_PTR *ptr, EH_CGI_PTR *prev, char *qs)
{
	int i, j, ns, vs;
	EH_CGI_PTR temp;

	temp = (EH_CGI_PTR) ehAlloc(sizeof(struct entry_list));
	if (!temp)
	{
		perror("temp : Memory Allocation Error in _getWord(1)");
		exit(0);
	}

	memset(temp,0,sizeof(sizeof(struct entry_list)));
	temp->name = ehAlloc(ns=64);
	temp->val = ehAlloc(vs=128);
	temp->other=NULL;
	if (!temp->name || !temp->val)
	{
		perror("temp : Memory Allocation Error in _getWord(2)");
		ehFree(temp);
		exit(0);
	}

	// Ricerca del nome dell'argomento
	for (i=0; ((qs[i]) && (qs[i] != '=')); i++)
	{
		temp->name[i] = qs[i];
		if (i+1 >= ns)
		{
			temp->name = ehRealloc(temp->name, 64, (ns*=2));
			if (!temp->name)
			{
				 perror("temp->name : Memory Allocation Error in _getWord (3)");
				 exit(0);
			}
		}
	}
	temp->name[i] = '\0';

	_plusToSpace(temp->name); // + --> ' '
	_unescapeUrl(temp->name);
  
	if (qs[i]) ++i;

	j=0;
	while (qs[j++] = qs[i++]);

	// Ricerca del valore
	for (i=0; ((qs[i]) && (qs[i] != '&')); i++)
	{
		temp->val[i] = qs[i];
		if (i+1 >= vs)
		{
			temp->val = ehRealloc(temp->val, 128, (vs*=2));
			if (!temp->val)
			{
				perror(" temp->val : Memory Allocation Error in _getWord()");
				exit(0);
			}
		}
	}
  
	temp->val[i] = '\0';
	_plusToSpace(temp->val);
	_unescapeUrl(temp->val);

	if (qs[i]) ++i;

	j = 0;
	while (qs[j++] = qs[i++]);

	// Aggiungo la lunghezza
	temp->len=strlen(temp->val);
	
	// Sistemo i puntatori
	temp->link = NULL;
	if (*ptr) // Faccio puntare il precedente a me stesso
		{(*prev)->link = temp;} 
		else // Assegno il primo valore
		{*ptr = temp;}
	*prev = temp; // Mi segno come precedente
}


//
// _nameGet()
//
char * _nameGet(char *line, char stop)
{
	int x, y;
	char *word = (char *) ehAlloc(sizeof(char) * (strlen(line)+1));

	for (x=0; ((line[x]) && (line[x] != stop)); x++)
		word[x] = line[x];

	word[x] = '\0';

	if (line[x]) ++x;

	y=0;
	while(line[y++] = line[x++]);

	return word;
}

/*
 * Value
 */
/*
char *Lftobr(char *str)
{
  int x, y;
  char *word;

  for (x=0, y=0; str[x]; x++, y++)
    if (str[x] == '\n') y++;

  word = (char *) ehAlloc( sizeof(char) * ( strlen(str)+1+(y*4) ) );

  for (x=0, y=0; str[x]; x++, y++)
  {
    if (str[x] != '\n')
       word[y] = str[x];
    else
    {
      strcpy(word+y, "<br />");
      y += 3;
    }
  }

  word[y]='\0';
  ehFree(str);
  return word;
}
*/

/*
 * Stream Extract
 */
static EH_CGI_PTR _getEntryFromStream(INT iLenStream)
{
	INT i,iLen=iLenStream;
	EH_CGI_PTR ptr = NULL;
	EH_CGI_PTR prev = NULL;
	
	
#ifdef EH_FASTCGI
	for (i=0; iLen && (!FCGI_feof(FCGI_stdin)); i++) {
		_makeWord(&ptr, &prev, '&', &iLen);  
	}
#else
	for (i=0; iLen && (!feof(stdin)); i++) {
		_makeWord(&ptr, &prev, '&', &iLen);  
	}
#endif

	return ptr;
}

//
// _makeWord()
//
static void _makeWord(EH_CGI_PTR *ptr, EH_CGI_PTR *prev, CHAR cStop, INT  * piLength)
{
	int wsize, ll;
	INT iOld;
	CHAR * psz;
	EH_CGI_PTR psTemp = NULL; 

	psTemp = (EH_CGI_PTR) ehAlloc(sizeof(struct entry_list));
	if (!psTemp )   {
		ehExit("temp : Memory Allocation error at _makeWord()");
	}
	memset(psTemp ,0,sizeof(struct entry_list));

	wsize = 102400;
	psz = (char *) ehAlloc(sizeof(char) * (wsize + 1));

	ll = 0;
	while (1) 
	{
#ifndef EH_FASTCGI
		psz[ll] = (char) fgetc(stdin);
#else
		psz[ll] = (char) FCGI_fgetc(FCGI_stdin);
#endif
		if (ll == wsize) {
			psz[ll+1] = '\0';
			iOld=wsize;
			wsize += (102400+1);
			psz = (char *) ehRealloc(psz, iOld, sizeof(CHAR ) * wsize);
		}

		--(*piLength);

#ifndef EH_FASTCGI
		if ((psz[ll]==cStop) || (feof(stdin)) || (!(*piLength)))
#else
		if ((psz[ll]==cStop) || (FCGI_feof(FCGI_stdin)) || (!(*piLength)))
#endif
		{
		if (psz[ll] != cStop) ll++;
			psz[ll] = '\0';
//			psz = (char *) ehRealloc(psz, ll, ll+1);
			psTemp->val = strDup(psz);
			_plusToSpace(psTemp->val);
			_unescapeUrl(psTemp->val);
//			printf("x:[%s]<br />",psTemp->val);
			psTemp->name = _nameGet(psTemp->val, '=');
			psTemp->len = strlen(psTemp->val);
				
			// Sistemo i puntatori
			psTemp->link = NULL;
			// Faccio puntare il precedente a me stesso
			if (*ptr) 
				{(*prev)->link = psTemp;} 
				else // Assegno il primo valore
				{*ptr = psTemp;}
			*prev = psTemp; // Mi segno come precedente
			break;
		}
		++ll;
	}
	ehFree(psz);
}

/*
 * multipart/form-data Extract
 * 
 *
 */

char *bstrstr(char *lpObj,int iLenObj,char *lpFind,int iLenFind,int *iPos)
{
	// 1234567890
	//       2345
	int i;
	char *lpPtr=lpObj;
	if (iLenFind>iLenObj) return NULL;
	for (i=0;i<(iLenObj-iLenFind+1);i++)
	{
		if (!memcmp(lpPtr,lpFind,iLenFind)) {*iPos=i; return lpPtr;}
		lpPtr++;
	}
	return NULL;
}

//
// cgiGet()
// 
CHAR * cgiGet(EH_CGI_INFO * psCgi, CHAR * pszName) {

	EH_CGI_PTR ps=cgiGetPtr(psCgi,pszName);
	if (!ps) return NULL; 
	return ps->val; 

}

//
// cgiGetPtr()
// 
EH_CGI_PTR cgiGetPtr(EH_CGI_INFO * psCgi, CHAR * pszName) {

	EH_CGI_PTR ps=psCgi->psParams;
	if (!ps) return NULL;
	while (ps)
	{
		if (!strCaseCmp(ps->name, pszName)) return ps; 
		ps = ps->link;
	}
	return NULL;

}


// 
// _getEntryFromMultipart()
//
EH_CGI_PTR _getEntryFromMultipart(int iLength, char *lpBoundary)
{
	int i;
	EH_CGI_PTR ptr = NULL;
	EH_CGI_PTR prev = NULL;
//	char *word;//, *word2;
	EH_CGI_PTR temp = NULL; 
//	char szBuf[2];
	char *lpMemory;
	char *lpPtr;
	char *lpStart;
	char *lpEnd;
	int iBoundaryLen;
	int iNewLength;
	int iPos;
	char szNewBoundary[80];
	char *lpName,*lp;
	sprintf(szNewBoundary,"--%s",lpBoundary);
	lpBoundary=szNewBoundary;

	iBoundaryLen=strlen(lpBoundary);

#ifndef EH_FASTCGI
	if (_setmode(_fileno(stdin),_O_BINARY)==-1) {printf("Error stdin"); return NULL;}
	_setmode( _fileno(stdin), _O_BINARY );
#endif

	lpMemory=ehAlloc(iLength);
	if (!lpMemory)
	{
		perror("temp : Memory Allocation error at _getEntryFromMultipart()");
		exit(0);
	}
	// Download dei dati dallo stream
	lpPtr=lpMemory;
#ifndef EH_FASTCGI
	for (i=0;i<iLength;i++) {*lpPtr++=fgetc(stdin);}
#else
	for (i=0;i<iLength;i++) {*lpPtr++=FCGI_fgetc(FCGI_stdin);}
#endif

	lpPtr=lpMemory;
	iNewLength=iLength;
	while (1)
	{
		lpStart=bstrstr(lpPtr,iNewLength,lpBoundary,iBoundaryLen,&iPos); if (!lpStart) break;
		lpStart+=iBoundaryLen+2;
		iNewLength-=(iPos+iBoundaryLen+2);
		lpEnd=bstrstr(lpStart,iNewLength,lpBoundary,iBoundaryLen,&iPos); if (!lpEnd) break;
		*lpEnd=0;
	
		// -------------------------------------------------
		// Cerco il nome
		//
		lpName=strstr(lpStart,"name=\""); if (!lpName) goto AVANTI;
		lp=strstr(lpName+6,"\""); if (!lp) goto AVANTI;
		*lp=0;
//		printf("NOME:[%s]",lpName+6);

		// Alloco memoria per zona variabile
		temp = (EH_CGI_PTR) ehAlloc(sizeof(struct entry_list));
		memset(temp,0,sizeof(struct entry_list));

		// -------------------------------------------------
		// Copio il nome della variabile nel nome
		//
		temp->name=ehAlloc(strlen(lpName+6)+1);
		strcpy(temp->name,lpName+6);
		*lp='\"';

		// -------------------------------------------------
		// Cerco la fine dell'header di intestazione
		//
		lp=strstr(lpStart,"\n\r"); 
		if (lp)
		{
			int iHead;
			lp+=1;
			*lp=0;
			temp->other=ehAlloc(strlen(lpStart)+1);
			strcpy(temp->other,lpStart);

			iHead=strlen(lpStart);
			*lp='\n';

			lp+=2;
			// -------------------------------------------------
			// Carico il valore della variabile
			//
			temp->len=iPos-iHead-4;			
			if (temp->len>0)
			{
			 temp->val=ehAlloc(temp->len+1);
			 memcpy(temp->val,lp,temp->len);
			 *(temp->val+temp->len)=0;
			}
		}
		
		// Sistemo i puntatori
		temp->link = NULL;
		if (ptr) // Faccio puntare il precedente a me stesso
			{prev->link = temp;} 
			else // Assegno il primo valore
			{ptr = temp;}
		prev = temp; // Mi segno come precedente

	AVANTI:
		*lpEnd='-';
		iNewLength-=(iPos);
		lpPtr=lpEnd;
	}
	ehFree(lpMemory);
	//printf("[OK]");
	return ptr;
}

/*
EH_CGI_PTR GetvalEntry(EH_CGI_PTR p,char *s)
{
	while (1)
	{
		if (!strCaseCmp(p->name, s)) return p; 
		p = p->link;
		if (p==NULL) break;
	}
	return NULL;
}
*/

//
// _freeEntryList()
//
static void _freeEntryList(EH_CGI_PTR p)
{
	EH_CGI_PTR t;

	if (!p) return;

	while (p)
	{
		t = p->link;
		if (p->name) ehFree(p->name);
		if (p->val) ehFree(p->val);
		if (p->other) ehFree(p->other);
		ehFree(p);
		p = t;
	}
}

static char _x2c(char *what)
{
	register char digit;

	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
	digit *= 16;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));

	return(digit);
}

//
// _unescapeUrl()
//
static void _unescapeUrl(char *url)
{
	register int x,y;

	for (x=0, y=0; url[y]; ++x, ++y)
	{
		if ((url[x] = url[y]) == '%')
		{
			url[x] = _x2c(&url[y+1]);
			y += 2;
		}
	}
	url[x] = '\0';
}

//
// _plusToSpace()
//
static void _plusToSpace(char *str)
{
	register int x;

	for (x=0; str[x]; x++)
		if (str[x] == '+') str[x] = ' ';
}

//
// _lstOutput)
//
static void _lstOutput(EH_LST lst,CHAR * pszRow) {

	EH_LST_I * psItem;

	for (psItem=lst->psFirst;psItem;psItem=psItem->psNext) {
#ifdef EH_FASTCGI
		FCGI_fwrite(psItem->ptr,strlen(psItem->ptr),1,FCGI_stdout);
//		ehLogWrite("%s",psItem->ptr);
		if (pszRow) FCGI_printf("%s",pszRow);
#else
		fwrite(psItem->ptr,strlen(psItem->ptr),1,stdout);
		if (pszRow) printf("%s",pszRow);
#endif
		
	}

}

//
// cgiOutput() 
//
BOOL cgiOutput(EH_LST lstHeader,EH_LST lstContent,BOOL bZip) {

	BYTE * pbZip=NULL;
	SIZE_T iZipSize=0;

#ifdef EH_ZLIB

	// a. Comprimo i dati
	if (bZip) {

		CHAR * psz;
		DWORD dwLen;
		psz=lstToString(lstContent,NULL,NULL,NULL);
		dwLen=strlen(psz);
		if (dwLen>256) {
			pbZip=ehGzip(psz,dwLen,&iZipSize);
		}
		ehFree(psz);

	}

#endif

	if (!pbZip) {
			
//		lstPush(lstHeader,"\n");
		if (lstHeader->iLength) {_lstOutput(lstHeader,CRLF); printf("\n");}
		_lstOutput(lstContent,NULL);

			
	} else {

		// b. Aggiorno Header 
		lstPush(lstHeader,"Content-Encoding: gzip");
		lstPushf(lstHeader,"Content-Length: %d",iZipSize);

		// c. Header Flush
//		lstPush(lstHeader,"\n");
		_lstOutput(lstHeader,"\n"); printf("\n");

		// d. Spedisco i dati fuori
		cgiBinOutput(pbZip,iZipSize);
		ehFree(pbZip);
	
	}

	return FALSE;

}

//
// cgiFileOut()
//
BOOL cgiFileOut(UTF8 * utfFileName,CHAR * pszContentType,BOOL bZip,BOOL bDateWri) {

	EH_LST	lstHeader=lstNew();
	SIZE_T	iSourceSize,iZipSize;
	BYTE *	pbFileSource;
	BYTE *	pbZip=NULL;

	// Carico il file in memoria
	pbFileSource=fileMemoRead(utfFileName,&iSourceSize);
	if (!pbFileSource) return true;
	if (bZip) {
	
		pbZip=ehGzip(pbFileSource,iSourceSize,&iZipSize);
		if (iZipSize>=iSourceSize) ehFreePtr(&pbZip);

	}

	if (pbZip) {

		lstPush(lstHeader,"Content-Type: application/zip");
		lstPush(lstHeader,"Content-Encoding: gzip");
		lstPushf(lstHeader,"Content-Length: %d",iZipSize);

		if (bDateWri) {
			TIME64	tWrite;
			fileTimeGet(utfFileName,NULL,NULL ,&tWrite);
			lstPushf(lstHeader,"Content-Date: %I64d",tWrite);
		}
		_lstOutput(lstHeader,"\n"); 
		printf("\n"); ////ehPrintf("\n");
		cgiBinOutput(pbZip,iZipSize);
		ehFree(pbZip);

	} else {
	
		if (!strEmpty(pszContentType)) lstPushf(lstHeader,"Content-Type: %s",pszContentType);
		lstPush(lstHeader,"Content-Type: application/octet-stream");
		lstPushf(lstHeader,"Content-Length: %d",iSourceSize);
		if (bDateWri) {
			TIME64	tWrite;
			fileTimeGet(utfFileName,NULL,NULL ,&tWrite);
			lstPushf(lstHeader,"Content-Date: %I64d",tWrite);
		}
		_lstOutput(lstHeader,"\n"); 
		printf("\n"); // ehPrintf("\n");
		cgiBinOutput(pbFileSource,iSourceSize);
	
	}
	lstDestroy(lstHeader);
	ehFree(pbFileSource);

	return FALSE;
}
//
// cgiBinOutput() 
//
BOOL cgiBinOutput(BYTE * pbData ,SIZE_T iDataSize) {

#ifdef EH_FASTCGI

	BYTE *	pbStart;
	INT		iTokenMax=32000;
	pbStart=pbData;
	do 
	{
		INT iToken=(INT) (iDataSize); if (iToken>iTokenMax) iToken=iTokenMax;
		FCGI_fwrite(pbStart,iToken,1,FCGI_stdout);
		pbStart+=iToken; iDataSize-=iToken;
	} while (iDataSize);

	// FCGI_fflush(FCGI_stdout);

#else

	_setmode( _fileno( stdout ), _O_BINARY );
	fwrite(pbData,iDataSize,1,stdout);  
	fflush(stdout);

#endif	

	return FALSE;
}
/*
HTTP/1.1 200 OK
Date: Wed, 18 Apr 2012 12:21:48 GMT
Expires: -1
Cache-Control: private, max-age=0
Content-Type: text/html; charset=UTF-8
Set-Cookie: PREF=ID=5864ffdea85513a4:FF=0:TM=1334751708:LM=1334751708:S=GFJ_oMfdZKYxDmRj; expires=Fri, 18-Apr-2014 12:21:48 GMT; path=/; domain=.google.it
Set-Cookie: NID=58=Zw_VuhnVHlZKg6OXHFo3i24yd1RE8GHWRyWtBNc47t7QzPubgvcJqA9l5Ubfw74EyTYkXO6-ZadTdDD6-4qOvB_Lu7L0QNDo6eE1B9Y8MGQ-doRu2xlHFKdVMrc8zwws; expires=Thu, 18-Oct-2012 12:21:48 GMT; path=/; domain=.google.it; HttpOnly
P3P: CP="This is not a P3P policy! See http://www.google.com/support/accounts/bin/answer.py?hl=en&answer=151657 for more info."
Content-Encoding: gzip
Server: gws
Content-Length: 4099
X-XSS-Protection: 1; mode=block
X-Frame-Options: SAMEORIGIN
*/

//
// cgiZipOut()  - ritorna FLASE se tutto ok
//
/*
BOOL cgiZipOut(BYTE * pbSource,SIZE_T sizSource) {

	static const char* myVersion = ZLIB_VERSION;
	SIZE_T	sizDest;
	BYTE *	pbDest;
	INT		iError;
	BOOL	bRet=TRUE;

	if (zlibVersion()[0] != myVersion[0]) 
	{
		cgiHtmlHeader(NULL);
		printf("#incompatible zlib version\n");
		return bRet;
	}

	sizDest=sizSource+(sizSource/10)+12;
	pbDest=ehAlloc(sizDest); 
	iError=compress2(pbDest,		// Destinazione in memoria
					 &sizDest,		// Dimensione di memoria da usare
					 pbSource,	// Puntatore al file originale
					 sizSource,
					 Z_BEST_COMPRESSION);	// Livello di compressione 9!!
	if (iError==Z_OK) // Sparo fuori il binario
	{
#ifndef EH_FASTCGI
		_setmode( _fileno( stdout ), _O_BINARY );
#endif
		printf(
			"Content-Type: text/zip" CRLF
			"Content-Encoding: gzip" CRLF
			"Warning:%d" CRLF
			"Content-Length: %d" CRLF, 
		   //"Original-Size: %d\n" "\n",
			sizSource,
			sizDest);
	    printf("\n");
			 
#ifdef EH_FASTCGI
		pbStart=pbDest;
		do 
		{
			INT iToken=(INT) (sizDest); if (iToken>iTokenMax) iToken=iTokenMax;
			FCGI_fwrite(pbStart,iToken,1,FCGI_stdout);
			pbStart+=iToken; sizDest-=iToken;
		} while (sizDest);
		// FCGI_fflush(FCGI_stdout);
#else
		fwrite(pbDest,sizDest,1,stdout);  
		fflush(stdout);
#endif		
		bRet=FALSE;
	}

	return bRet;

}

*/