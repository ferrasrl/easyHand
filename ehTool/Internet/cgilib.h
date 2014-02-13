/*

 * cgilib.h

 * This file is part of the CGI library.

 * Copyright 1997 by IL Kwang, KIM.

 */

#ifndef __CGILIB_H__
#define __CGILIB_H__ 

#define LF 10
#define CR 13

typedef struct cgi_info {
  char *server_software;     
  char *server_name;         
  char *server_protocol;     
  char *server_port;         
  char *server_admin;        
  char *request_method;      
  char *gateway_interface;   
  char *http_accept;         
  char *path_info;           
  char *path_translated;     
  char *script_name;         
  char *query_string;        
  char *auth_type;           
  char *remote_host;         
  char *remote_addr;         
  char *remote_user;         
  char *remote_ident;        
  char *content_type;        
  int content_length;
  char *http_user_agent;
  char *http_refeer;
  char *http_accept_language;
  char *http_accept_encoding;
} CGI_INFO;

typedef struct entry_list *FORM_ENTRY_PTR;

typedef struct entry_list {
  char *name; // Nome
  char *val;  // Valore
  int  len;	  // Lunghezza (GT)
  char *other; // altro (GT)
  FORM_ENTRY_PTR link;       
} FORM_ENTRY;

void Get_cgi_info(CGI_INFO *ci);
void Print_mimeheader(char *mime);
void Print_htmlheader(char *head);
void Print_htmlfooter(char *logo, char *text, char *addr);
FORM_ENTRY_PTR Get_form_entries(CGI_INFO *ci);
FORM_ENTRY_PTR Get_entry_from_stream(int length, FILE *stream);
FORM_ENTRY_PTR Get_entry_from_string(char *query_string);
FORM_ENTRY_PTR Get_entry_from_multipart(int length, FILE *stream,char *lpBoundary);

void Getword(FORM_ENTRY_PTR *ptr, FORM_ENTRY_PTR *prev, char *qs);
char *Lftobr(char *str);
void Fmakeword(FORM_ENTRY_PTR *ptr, FORM_ENTRY_PTR *prev, FILE *f, char stop, int *cl);
char *Makeword(char *line, char stop);
char *Getval(FORM_ENTRY_PTR p, char *s);
FORM_ENTRY_PTR GetvalEntry(FORM_ENTRY_PTR p,char *s);
void Free_entry_list(FORM_ENTRY_PTR p);
char X2c(char *what);
void Unescape_url(char *url);
void Plustospace(char *str);
int  Rind(char *s, char c);
int  Getline(char *s, int n, FILE *f);
void Send_fd(FILE *f, FILE *fd);
int  Ind(char *s, char c);
void Escape_shell_cmd(char *cmd);
void Show_cgi_info(CGI_INFO *ci);
void Show_cgi_val(FORM_ENTRY_PTR p);

#endif



