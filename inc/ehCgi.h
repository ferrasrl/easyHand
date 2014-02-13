/*

 * cgilib.h

 * This file is part of the CGI library.

 * Copyright 1997 by IL Kwang, KIM.

 */

#ifndef __CGILIB_H__
#define __CGILIB_H__ 

#define LF 10
#define CR 13

typedef struct entry_list * EH_CGI_PTR;

typedef struct entry_list {
  char *name; // Nome
  char *val;  // Valore
  int  len;	  // Lunghezza (GT)
  char *other; // altro (GT)
  EH_CGI_PTR link;       
} FORM_ENTRY;


typedef struct EH_CGI_INFO {
  char *	server_software;     
  char *	server_name;         
  char *	server_protocol;     
  char *	server_port;         
  char *	server_admin;        
  char *	request_method;      
  char *	gateway_interface;   
  char *	http_accept;         
  char *	path_info;           
  char *	path_translated;     
  char *	script_name;         
  char *	query_string;        
  char *	auth_type;           
  char *	remote_host;         
  char *	remote_addr;         
  char *	remote_user;         
  char *	remote_ident;        
  char *	content_type;        
  int		content_length;
  char *	http_user_agent;
  char *	http_refeer;
  char *	http_accept_language;
  char *	http_accept_encoding;
  char *	http_cookie;
  BOOL		bGzip;		// T/F se acceta la compressione gzip
  EH_CGI_PTR psParams;

} EH_CGI_INFO;

void cgiOpen(EH_CGI_INFO * psCgi, BOOL bGetParams, CHAR **);
void cgiClose(EH_CGI_INFO * psCgi);

void cgiShowInfo(EH_CGI_INFO * psCgi);
void cgiShowParams(EH_CGI_INFO * psCgi);
EH_CGI_PTR cgiBuildParams(EH_CGI_INFO * psCgi);
EH_CGI_PTR cgiGetPtr(EH_CGI_INFO * psCgi, CHAR * pszName);
CHAR * cgiGet(EH_CGI_INFO * psCgi, CHAR * pszName);


void cgiMimeHeader(char * pszMime);
void cgiHtmlHeader(char * pszHead);
void cgiHtmlFooter(char * pszLogo, char * pszText);

BOOL cgiOutput(EH_LST lstHeader,EH_LST lstContent,BOOL bZip);
//BOOL cgiFileOut(UTF8 * utfFileName,BOOL bZip,BOOL bDateMod);
BOOL cgiFileOut(UTF8 * utfFileName,CHAR * pszContentType,BOOL bZip,BOOL bDateWri);
BOOL cgiBinOutput(BYTE * pbData,SIZE_T iDataSize);

/*
EH_CGI_PTR cgiGetParams(EH_CGI_INFO *ci);
EH_CGI_PTR Get_entry_from_stream(int length, FILE *stream);
EH_CGI_PTR Get_entry_from_string(char *query_string);
EH_CGI_PTR Get_entry_from_multipart(int length, FILE *stream,char *lpBoundary);
void Getword(EH_CGI_PTR *ptr, EH_CGI_PTR *prev, char *qs);
char *Lftobr(char *str);
void Fmakeword(EH_CGI_PTR *ptr, EH_CGI_PTR *prev, FILE *f, char stop, int *cl);
char *Makeword(char *line, char stop);

EH_CGI_PTR GetvalEntry(EH_CGI_PTR p,char *s);
void Free_entry_list(EH_CGI_PTR p);
char X2c(char *what);
void Unescape_url(char *url);
void Plustospace(char *str);
int  Rind(char *s, char c);
int  Getline(char *s, int n, FILE *f);
void Send_fd(FILE *f, FILE *fd);
int  Ind(char *s, char c);
void Escape_shell_cmd(char *cmd);

*/



#endif



