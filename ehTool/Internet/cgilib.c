/* 
 * cgilib.c
 * This file is updated by IL Kwang, Kim. 
 * It contains some routines for CGI. 
 * 
 *
 * <Diagram of CGI Interface> 
 * 
 * +---------+   GET/POST   +----------+                         +----------+
 * | Browser |------------->|          |------------------------>|          |
 * |         |   Encoding   |          |        Decoding         |          |
 * |<ISINDEX>|              | Web      |                         |          |
 * |         |              |          |                         |  C G I   |
 * |<FORM>   |              | Server   |                         |          |
 * |         |              |          |                         |          |
 * |         |<-------------|          |<------------------------|          |
 * +---------+              +----------+                         +----------+
 *
 *
 * Perfezionato da G. Tassistro per Ferrà A&T 2000/2001
 */

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/Internet/cgilib.h"
#include <fcntl.h>
/*
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
*/
#ifdef _WIN32
#define strcasecmp strCaseCmp
#define strncasecmp _memicmp
#endif

void Get_cgi_info(CGI_INFO *ci)
{
	char *strlen;

	memset(ci,0,sizeof(CGI_INFO));
	ci->content_type = getenv("CONTENT_TYPE");
	if (ci->content_type) {if (!*ci->content_type) ci->content_type=NULL;}
	strlen = getenv("CONTENT_LENGTH");
	if (strlen) ci->content_length=atoi(strlen); else ci->content_length=0;
	ci->server_name = getenv("SERVER_NAME");
	ci->server_software = getenv("SERVER_SOFTWARE");
	ci->server_protocol = getenv("SERVER_PROTOCOL");
	ci->server_port = getenv("SERVER_PORT");
	ci->gateway_interface = getenv("GATEWAY_INTERFACE");
	ci->request_method = getenv("REQUEST_METHOD");
	ci->http_accept = getenv("HTTP_ACCEPT");
	ci->path_info = getenv("PATH_INFO");
	ci->path_translated = getenv("PATH_TRANSLATED");
	ci->script_name = getenv("SCRIPT_NAME");
	ci->query_string = getenv("QUERY_STRING");
	ci->remote_host = getenv("REMOTE_HOST");
	ci->remote_addr = getenv("REMOTE_ADDR");
	ci->remote_user = getenv("REMOTE_USER");
	ci->remote_ident = getenv("REMOTE_IDENT");
	ci->auth_type = getenv("AUTH_TYPE");
	ci->http_user_agent = getenv("HTTP_USER_AGENT");
	ci->http_refeer = getenv("HTTP_REFERER");
	ci->http_accept_language =getenv("HTTP_ACCEPT_LANGUAGE");
	ci->http_accept_encoding =getenv("HTTP_ACCEPT_ENCODING");

}

void Show_cgi_info(CGI_INFO *ci)
{
	printf("CONTENT_TYPE: [<b>%s</b>]<br />",ci->content_type);
	printf("CONTENT_LENGTH: %d<br />",ci->content_length);
	printf("SERVER_NAME: [<b>%s</b>]<br />",ci->server_name);
	printf("SERVER_SOFTWARE: [<b>%s</b>]<br />",ci->server_software);
	printf("SERVER_PROTOCOL: [<b>%s</b>]<br />",ci->server_protocol);
	printf("SERVER_PORT: [<b>%s</b>]<br />",ci->server_port);
	printf("GATEWAY_INTERFACE: [<b>%s</b>]<br />",ci->gateway_interface);
	printf("REQUEST_METHOD: [<b>%s</b>]<br />",ci->request_method);
	printf("HTTP_ACCEPT: [<b>%s</b>]<br />",ci->http_accept);
	printf("PATH_INFO: [<b>%s</b>]<br />",ci->path_info);
	printf("PATH_TRANSLATED: [<b>%s</b>]<br />",ci->path_translated);
	printf("SCRIPT_NAME: [<b>%s</b>]<br />",ci->script_name);
	printf("QUERY_STRING: [<b>%s</b>]<br />",ci->query_string);
	printf("REMOTE_HOST: [<b>%s</b>]<br />",ci->remote_host);
	printf("REMOTE_ADDR: [<b>%s</b>]<br />",ci->remote_addr);
	printf("REMOTE_USER: [<b>%s</b>]<br />",ci->remote_user);
	printf("REMOTE_IDENT: [<b>%s</b>]<br />",ci->remote_ident);
	printf("AUTH_TYPE: [<b>%s</b>]<br />",ci->auth_type);
	printf("HTTP_USER_AGENT: [<b>%s</b>]<br />",ci->http_user_agent);
	printf("HTTP_REFEER: [<b>%s</b>]<br />",ci->http_refeer);
	printf("HTTP_ACCEPT_LANGUAGE: [<b>%s</b>]<br />",ci->http_accept_language);
	printf("HTTP_ACCEPT_ENCODING: [<b>%s</b>]<br />",ci->http_accept_encoding);
}

void Show_cgi_val(FORM_ENTRY_PTR p)
{
	FORM_ENTRY_PTR t;
	if (!p) return;

	while (p)
	{
		t = p->link;
		//printf("%s=%s|%s|<br />",p->name?p->name:"(null)",p->val,p->other);
		printf("%s=%s<br />",
				p->name?p->name:"(null)",
				p->val?p->val:"(null)"
				);
		p = t;
	}
}

/*
 * Preapara il mime header
 */
void Print_mimeheader(char *mime)
{
	printf("Content-Type: %s\n\n", mime);
}

/*
 * HTML Header Title
 */
void Print_htmlheader(char *head)
{
	printf("<html>\n<head>\n<title>%s</title>\n</head>\n", head);
	printf("<body>\n");
}

/*
 * HTML FOOTER
 */
void Print_htmlfooter(char *logo, char *text, char *addr)
{
//char *wm;
	puts("<P><HR>");
	if (logo) {if (*logo) printf("<img src=%s ALIGN=bottom>", logo);}
	if (text) {if (*text) puts(text);}
	if (addr) {if (*addr) printf("<ADDRESS>%s</ADDRESS>\n", addr);}
	printf("</BODY>\n</HTML>\n");
}

FORM_ENTRY_PTR Get_form_entries(CGI_INFO *ci) {

	if (ci->content_type)
	{
		if (!memcmp(ci->content_type,"multipart/form-data",19))
		{
			char *lpBoundary;
			lpBoundary=strstr(ci->content_type,"boundary=");
			if (!lpBoundary) return NULL; else lpBoundary+=9;
			return Get_entry_from_multipart(ci->content_length, stdin, lpBoundary);
		}
		// Metodo classico di passaggio dati CGI
		if (!strBegin(ci->content_type,"application/x-www-form-urlencoded"))
		{
			//printf("Ci passo !!");
			if (!strCaseCmp(ci->request_method, "POST")) return Get_entry_from_stream(ci->content_length, stdin);
			if (!strCaseCmp(ci->request_method, "GET")) return Get_entry_from_string(ci->query_string);
		}
	}
	else if (ci->request_method)
	{
		if (!strCaseCmp(ci->request_method, "POST")) return Get_entry_from_stream(ci->content_length, stdin);
		if (!strCaseCmp(ci->request_method, "GET")) return Get_entry_from_string(ci->query_string);
	}
	return NULL;

}

//
// query_string Extract
//
FORM_ENTRY_PTR Get_entry_from_string(char *query_string) {

	int i;
	FORM_ENTRY_PTR ptr = NULL;
	FORM_ENTRY_PTR prev = NULL;

	if (!query_string) return NULL;

	//printf("Entro qui");
	for (i=0; *query_string; i++)
	{
		Getword(&ptr, &prev, query_string);
	}
	return ptr;

}

void Getword(FORM_ENTRY_PTR *ptr, FORM_ENTRY_PTR *prev, char *qs)
{
	int i, j, ns, vs;
	FORM_ENTRY_PTR temp;

	temp = (FORM_ENTRY_PTR) malloc(sizeof(struct entry_list));
	if (!temp)
	{
		perror("temp : Memory Allocation Error in Getword(1)");
		exit(0);
	}

	memset(temp,0,sizeof(sizeof(struct entry_list)));
	temp->name = malloc(ns=64);
	temp->val = malloc(vs=128);
	if (!temp->name || !temp->val)
	{
		perror("temp : Memory Allocation Error in Getword(2)");
		free(temp);
		exit(0);
	}

	// Ricerca del nome dell'argomento
	for (i=0; ((qs[i]) && (qs[i] != '=')); i++)
	{
		temp->name[i] = qs[i];
		if (i+1 >= ns)
		{
			temp->name = realloc(temp->name, (ns*=2));
			if (!temp->name)
			{
				 perror("temp->name : Memory Allocation Error in Getword (3)");
				 exit(0);
			}
		}
	}
	temp->name[i] = '\0';

	Plustospace(temp->name); // + --> ' '
	Unescape_url(temp->name);
  
	if (qs[i]) ++i;

	j=0;
	while (qs[j++] = qs[i++]);

	// Ricerca del valore
	for (i=0; ((qs[i]) && (qs[i] != '&')); i++)
	{
		temp->val[i] = qs[i];
		if (i+1 >= vs)
		{
			temp->val = realloc(temp->val, (vs*=2));
			if (!temp->val)
			{
				perror(" temp->val : Memory Allocation Error in Getword()");
				exit(0);
			}
		}
	}
  
	temp->val[i] = '\0';
	Plustospace(temp->val);
	Unescape_url(temp->val);

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


/*
 * Getword()
 * 
 */

char *Makeword(char *line, char stop)
{
  int x, y;
  char *word = (char *) malloc(sizeof(char) * (strlen(line)+1));

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

  word = (char *) malloc( sizeof(char) * ( strlen(str)+1+(y*4) ) );

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
  free(str);
  return word;
}
*/

/*
 * Stream Extract
 */
FORM_ENTRY_PTR Get_entry_from_stream(int length, FILE *stream)
{
	int i;
	FORM_ENTRY_PTR ptr = NULL;
	FORM_ENTRY_PTR prev = NULL;

	
	//LOGWrite("Stream %d",length);
	for (i=0; length && (!feof(stdin)); i++)
	{
		Fmakeword(&ptr, &prev, stdin, '&', &length);  
	}
	return ptr;
}

void Fmakeword(FORM_ENTRY_PTR *ptr, FORM_ENTRY_PTR *prev, FILE *f, char stop, int *cl)
{
  int wsize, ll;//, x, y;
  char *word;//, *word2;
  FORM_ENTRY_PTR temp = NULL; 

  temp = (FORM_ENTRY_PTR) malloc(sizeof(struct entry_list));
  memset(temp,0,sizeof(struct entry_list));
  if (!temp) 
  {
    perror("temp : Memory Allocation error at Fmakeword()");
    exit(0);
  }

  wsize = 102400;
  word = (char *) malloc(sizeof(char) * (wsize + 1));

  ll = 0;
  while (1) 
  {
    word[ll] = (char) fgetc(f);

		if (ll == wsize) {
			word[ll+1] = '\0';
			wsize += 102400;
			word = (char *) realloc(word, sizeof(char) * (wsize+1));
		}

		--(*cl);

		if ((word[ll]==stop) || (feof(f)) || (!(*cl)))
		{
			if (word[ll] != stop) ll++;
			word[ll] = '\0';
			word = (char *) realloc(word, ll+1);
			temp->val = word;
			Plustospace(temp->val);
			Unescape_url(temp->val);
			//LOGWrite(">[%s]",temp->val);
			temp->name = Makeword(temp->val, '=');
		//	temp->val = Lftobr(temp->val);
			temp->len = strlen(temp->val);

			// Sistemo i puntatori
			temp->link = NULL;
			if (*ptr) // Faccio puntare il precedente a me stesso
				{(*prev)->link = temp;} 
				else // Assegno il primo valore
				{*ptr = temp;}
			*prev = temp; // Mi segno come precedente
			break;
		}
		++ll;
	}
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

FORM_ENTRY_PTR Get_entry_from_multipart(int iLength, FILE *stream,char *lpBoundary)
{
	int i;
	FORM_ENTRY_PTR ptr = NULL;
	FORM_ENTRY_PTR prev = NULL;
//	char *word;//, *word2;
	FORM_ENTRY_PTR temp = NULL; 
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
	//printf("\nLunghezza: %d\n<br />",iLength);
//	printf("\nBoundary: [%s]\n",lpBoundary);
	iBoundaryLen=strlen(lpBoundary);
	if (_setmode( _fileno( stream ), _O_BINARY )==-1) {printf("Error stdin"); return NULL;}
	_setmode( _fileno( stdout), _O_BINARY );
	//fflush(stdout);
	lpMemory=malloc(iLength);
	if (!lpMemory)
	{
		perror("temp : Memory Allocation error at Get_entry_from_multipart()");
		exit(0);
	}
	// Download dei dati dallo stream
	lpPtr=lpMemory;
	for (i=0;i<iLength;i++) {*lpPtr++=fgetc(stream);}

	lpPtr=lpMemory;
	iNewLength=iLength;
	while (1)
	{
		lpStart=bstrstr(lpPtr,iNewLength,lpBoundary,iBoundaryLen,&iPos); if (!lpStart) break;
		lpStart+=iBoundaryLen+2;
		iNewLength-=(iPos+iBoundaryLen+2);
		lpEnd=bstrstr(lpStart,iNewLength,lpBoundary,iBoundaryLen,&iPos); if (!lpEnd) break;
		*lpEnd=0;
//		printf("[%s]<br />",lpStart); 
		// Carico la variabile nello stack
	
		// -------------------------------------------------
		// Cerco il nome
		//
		lpName=strstr(lpStart,"name=\""); if (!lpName) goto AVANTI;
		lp=strstr(lpName+6,"\""); if (!lp) goto AVANTI;
		*lp=0;
//		printf("NOME:[%s]",lpName+6);

		// Alloco memoria per zona variabile
		temp = (FORM_ENTRY_PTR) malloc(sizeof(struct entry_list));
		memset(temp,0,sizeof(struct entry_list));

		// -------------------------------------------------
		// Copio il nome della variabile nel nome
		//
		temp->name=malloc(strlen(lpName+6)+1);
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
			temp->other=malloc(strlen(lpStart)+1);
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
			 temp->val=malloc(temp->len+1);
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
	free(lpMemory);
	//printf("[OK]");
	return ptr;
}

char *Getval(FORM_ENTRY_PTR p, char *s)
{
	if (!p) return NULL;
	while (p)
	{
		if (!strcasecmp(p->name, s)) return p->val; 
		p = p->link;
	//	if (p==NULL) break;
	}
	return NULL;
}

FORM_ENTRY_PTR GetvalEntry(FORM_ENTRY_PTR p,char *s)
{
	while (1)
	{
		if (!strcasecmp(p->name, s)) return p; 
		p = p->link;
		if (p==NULL) break;
	}
	return NULL;
}

void Free_entry_list(FORM_ENTRY_PTR p)
{
	FORM_ENTRY_PTR t;

	if (!p) return;

	while (p)
	{
		t = p->link;
		if (p->name) free(p->name);
		if (p->val) free(p->val);
		if (p->other) free(p->other);
		free(p);
		p = t;
	}
}

char X2c(char *what)
{
	register char digit;

	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
	digit *= 16;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));

	return(digit);
}

// -------------------------------------------------------------
// Decodifica della sequenza %?? per i caratteri speciali      |
// -------------------------------------------------------------
void Unescape_url(char *url)
{
	register int x,y;

	for (x=0, y=0; url[y]; ++x, ++y)
	{
		if ((url[x] = url[y]) == '%')
		{
			url[x] = X2c(&url[y+1]);
			y += 2;
		}
	}
	url[x] = '\0';
}

/* da + a spazio
 *
 */
void Plustospace(char *str)
{
	register int x;

	for (x=0; str[x]; x++)
		if (str[x] == '+') str[x] = ' ';
}

/*
 * cerca un carattere in una stringa partendo la fondo
 */
int Rind(char *s, char c)
{
	register int x;

	for (x=strlen(s)-1; x!=-1; x--)
    if (s[x]==c) return x;

  return -1;
}

/*
 *  Legge una linea
 * 
 */
int Getline(char *s, int n, FILE *f)
{
  register int i = 0;

  while (1)
  {
    s[i] = (char) fgetc(f);
    /*
     *
     */
    if (s[i] == CR)
      s[i] = fgetc(f);

    if ((s[i] == 0x4) || (s[i] == LF) || (i == (n-1))) 
    {
      s[i] = '\0';
      return (feof(f) ? 1 : 0);
    }
    ++i;
  }
}

/*
 * Copia il file *f nel file destinazione *fd
 */
void Send_fd(FILE *f, FILE *fd)
{
  int num_char = 0;
  char c;

  while (1) 
  {
    c = fgetc(f);
		if (feof(f)) return;
    fputc(c, fd);
  }
}

/*
 *  ricerca di una carattere in avanti
 */
int Ind(char *s, char c)
{
  register int x;

  for (x=0; s[x]; x++)
    if (s[x] == c) return x;
  
  return -1;
}

// ------------------------------------------------
// Toglie i caratteri comando della shell ?       |
// ------------------------------------------------
void Escape_shell_cmd(char *cmd)
{
	register int x, y, l;

	l = strlen(cmd);

	for (x=0; cmd[x]; x++)
	{
		if (Ind("&;`'\"|*?~<>()[]{}$\\", cmd[x]) != -1)
		{
			for (y=l+1; y<x; y--)
				cmd[y] = cmd[y-1];
			l++;  /* length has been increased */
			cmd[x] = '\\';
			x++;  /* skip the character */
		}
	}
}