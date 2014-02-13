// --------------------------------------
// CGIUTIL   
// by Ferra Art & Technology
// --------------------------------------
#include "\ehtool\include\ehsw_idb.h"
#include "\ehtool\internet\cgilib.h"
#include "\ehtool\internet\cgiutil.h"

int ChangeVar(char *Html,char *Nome,char *String)
{
	char *p;
	char Cerca[80];
	int lo,ln,le;

	sprintf(Cerca,"@#%s#@",Nome);
	p=strstr(Html,Cerca); if (!p) return 0;
	lo=strlen(Cerca);
	ln=strlen(String);
	le=strlen(p+lo)+1;
	memmove(p+ln,p+lo,le);
	memcpy(p,String,ln);
	return 1;
}

void OutputFile(char *Html)
{
	char *p;
	for (p=Html;*p;p++) printf("%c",*p);
}
//ีออออออออออออออออออออออออออออออออออออออออออออออออธ
//ณ MASK - Formatizza i numeri per la stampa       ณ
//ณ                                                ณ
//ณ *num  = Stringa da Stampare                    ณ
//ณ numcif= Lunghezza in cifre (prima della virgolaณ
//ณ dec   = Numero decimali                        ณ
//ณ sep   = Flag separatori (0=No,1=Si)            ณ
//ณ segno = Flag segno (OFF=Solo -,ON=+&-)         ณ
//ณ                                                ณ
//ณ                                  by Ferr… 1996 ณ
//ิออออออออออออออออออออออออออออออออออออออออออออออออพ
/*
CHAR *nummask(CHAR *num,SINT numcif,SINT dec,SINT sep,SINT segno)
{
	CHAR lensx,lendx,sg=0;
	SINT  ptsx,ptdx,ptnum,a,pt,cnt;
	SINT  len,lnum;
	static CHAR srv[40];
	CHAR *sr=srv;

	srv[0]=0;

	//	PRE-controlli
	if ((lnum=strlen(num))==0) goto errore;
	if ((numcif<1)||(numcif>18)) goto errore;
	if ((dec<0)||(dec>10)) goto errore;

	//	Calcola lunghezza stringa di output
	len=(dec>0) ? numcif+1+dec : numcif;
	if (sep) len+= ((numcif-1)/3);
	//if ((segno)||(num[0]=='-'))
	len++;

	//	Pulisce la stringa per il ritorno
	for (a=0;a<len;a++) {srv[a]=' ';}
	srv[len]=0;

	//	C il segno : sg va a 1 o 2
	ptsx=0;
	if (num[ptsx]=='+') ptsx++;
	if (num[ptsx]==' ') ptsx++;
	if (num[ptsx]=='-') {sg='-'; ptsx++;}
	if ((segno)&&!sg) {if (num[0]=='0') sg=' '; else sg='+';}

	// 	Ricerco se ci sono decimali nel numero passato
	lensx=lnum-ptsx; lendx=0;
	for (a=ptsx;num[a]!=0;a++) {
			if ((num[a]=='.')||(num[a]==',')) {lensx=a-ptsx; ptdx=a+1; lendx=lnum-a-1; break;}
	}

	// 	La stampa prevede decimali ? li copia per l'output
	if (dec>0) {
		 pt=len-dec-1; ptnum=pt-1; srv[pt++]=',';
		 for (a=0;a<dec;a++) {srv[pt++]=((a+1)>lendx) ? '0' : num[ptdx++];}
	 }
	 else ptnum=len-1;

	//  Stampa numero con separatori
	cnt=0; ptsx+=lensx-1;
	for (a=0;a<lensx;a++) {
			if ((sep)&&(cnt++==3)) {srv[ptnum--]='.';cnt=1;}// divide le migliaia
			if ((ptsx<0)||(ptnum<0)) goto errore;
			srv[ptnum--]=num[ptsx--];
	}

	if (sg) {if (ptnum>=0) srv[ptnum]=sg; else goto errore;}

fine:

	sr=srv; return sr;

errore:
	srv[0]='?'; goto fine;
}

char Num[80];
CHAR *Snummask(double Numer,SINT numcif,SINT dec,SINT sep,SINT segno)
 {
 sprintf(Num,"%lf",Numer);
 return nummask(Num,numcif,dec,sep,segno);
 }

*/
//ีออออออออออออออออออออออออออออออออออออออออออออออออธ
//ณ HTMLLoadFile                                   ณ
//ณ Carica un file HTML in memoria                 ณ
//ณ                                                ณ
//ณ                                  by Ferr… 1996 ณ
//ิออออออออออออออออออออออออออออออออออออออออออออออออพ
char *HTMLLoadFile(char *FileName)
{
	FILE *ch;
	char *Html,*pt;

	ch=fopen(FileName,"r"); 
	if (ch==NULL) {printf("LoadFile ? [%s]<BR>",FileName); return NULL;}
	Html=malloc(80000); 
	if (Html==NULL) {printf("LoadFile Memory ? [%s]<BR>",FileName); return NULL;}
	pt=Html;

	for (;;)
	{
	 if (fgets(pt,1024,ch)==NULL) break;
	 pt+=strlen(pt);
	}
	fclose(ch);
	*pt=0;
	return Html;
}

void ShowCGI(FORM_ENTRY_PTR parms)
{
 FORM_ENTRY_PTR p;

 printf("<h1>Contenuto CGI</h1><br>");
 printf("Seguono il nome ed il valore:<p>%c",10);
 printf("<ul>%c",10);
 for (p=parms; p; p=p->link) printf("<li> |%s|=|%s| (%d:%s) %c", p->name, p->val, p->len, p->other ? p->other:"",10);
 printf("</ul>%c",10);
 Print_htmlfooter(0, "by Ferra' Art & Technology (c) 2001","<A HREF=www.ferra.com></A>");
}