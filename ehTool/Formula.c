//-----------------------------------------------------------------------
//	FormulaProcess  Trasforma la formula in un numero                   |
//											Creato da G.Tassistro       |
//											Ferrà Art & Tecnology 1999  |
//  Ritorna TRUE = Formula errata                                       |
//          FALSE= Formula OK Valore messo in "Valore"                  |
//-----------------------------------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/formula.h"

// Tipi di elementi possibili in whatIsThis
#define E_VARUNKNOW  -1
#define E_ERROR       0
#define E_NUMDEC	  1
#define E_NUMHEX  	  2
#define E_NUMBIN	  3
#define E_CHARFIX    10
#define E_FORMULANUM 20
#define E_VARNUM     30
#define E_VARCHAR    31
#define E_INTFUNC    33
#define VARSIZE    32

#define EE_OK 0
#define EE_VARUNKNOW 1

//
// Funzione che notifica l'errore all'esterno
//
static void (*extErrorNotify)(SINT iMode,CHAR *pMess,...)=NULL;

static CHAR *szFormError[]={"Formula interpretata correttamente.",
							"Variabile sconosciuta",
							"Errore in formula"};
static SINT iFormLastError=0;
static CHAR szAddInfo[80];
static SINT SimpleFORMtoNumber(CHAR *Formula,FORMULA_FUNC_EXT);
static SINT First_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT); // Primi operatori da convertire : /*<<>>
static SINT Second_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT); // Secondi operatori +-&^
static SINT BOOLEAN_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT);
static SINT L_FormulaProcess(CHAR *pszFomula,double *Valore,FORMULA_FUNC_EXT);
static BOOL IsMathOperator(CHAR Byte);
static CHAR *ParChiudi(CHAR *p);
static SINT _WhatIsThis(CHAR *okl,FORMULA_FUNC_EXT,void*Valore);
static BOOL Fdebug=FALSE;
static CHAR * ElementAllocStringConverter(CHAR *lpElement,FORMULA_FUNC_EXT);

void FormulaSetNotify(void (*NewFunction)(SINT iMode,CHAR *pMess,...))
{
	extErrorNotify=NewFunction;
}

void FormulaDebug(SINT flag)
{
 Fdebug=flag;
}
static SINT IsDecNumero(CHAR *);
static SINT IsHexNumero(CHAR *);
static SINT IsBinNumero(CHAR *);
static SINT IsStringFix(CHAR *);
static SINT IsStringVar(CHAR *);
static BOOL IsString(CHAR *);


// Ritorna FALSE = TUTTO OK
// 

BOOL FormulaProcess(CHAR * pszFomula,
					double * lpdValore, // Valore calcolato di ritorno
					BOOL fViewError, // Se si devono vedere gli errori
					FORMULA_FUNC_EXT // Funzione che restituisce i valore delle Variabili
					)
{
    SINT Ret;
	CHAR *lpNewFormula=ehAlloc(2048);
	CHAR *ps,*pd=lpNewFormula;
	SINT Tipo;

	// Tolgo i caratteri di controllo (CR-LF ecc)
	for (ps=pszFomula;*ps;ps++)
	{
	 if (*ps<33) continue;
	 *pd=*ps; pd++;
	}
    *pd=0;

	//printf(CRLF ">|%s|<" CRLF,lpNewFormula); //getch();
	iFormLastError=EE_OK;
	
	// Se è una singola variabile la elaboro subito
	Tipo=_WhatIsThis(lpNewFormula,funcExt,NULL);
	if (Tipo==E_VARNUM) 
		{
		 if (funcExt) funcExt(WS_FIND,0,lpNewFormula,lpdValore); 
	     Ret=FALSE;
		}
	else
	// Altrimenti lancio la formula
	{
		Ret=L_FormulaProcess(lpNewFormula,lpdValore,funcExt);
	}
	/*
#ifdef _CONSOLE
	if (Ret&&fViewError) {printf("Formula.c:\n%s\n(%s)",szFormError[iFormLastError],szAddInfo); }
#else
	if (Ret&&fViewError) win_infoarg("Formula.c:\n%s\n(%s)",szFormError[iFormLastError],szAddInfo);
#endif
	*/
	ehFree(lpNewFormula);
	return Ret;
}

//
// Questa invece è ad uso interno ed è RICORSIVA
//
static SINT L_FormulaProcess(CHAR *Formula,double *Valore,FORMULA_FUNC_EXT)
{
	// Lunghezza massima della formula
	const SINT MaxFormula=512;

	SINT  hdlFormula,hdlCodaOriginale;
	CHAR *FormulaGlobale;
	CHAR *CodaOriginale;
	BOOL fRet=FALSE;

	*Valore=0;
	//Fdebug=ON;
	if (Fdebug) {win_infoarg("FORMULA |%s|\n",Formula); }

	// ------------------------------------------------------------|
	// FASE 0                                                      |
	// Alloco memoria per l'elaborazione                           |
	// ------------------------------------------------------------|

	if (strlen(Formula)>(WORD) (MaxFormula-1))
	{
		if (extErrorNotify) (*extErrorNotify)(1,"Formula troppo lunga max(%d)",MaxFormula-1);
		return TRUE;
	}

	hdlFormula=memoAlloc(M_HEAP,MaxFormula,"FORM");
	FormulaGlobale=memoPtr(hdlFormula,NULL);

	hdlCodaOriginale=memoAlloc(M_HEAP,MaxFormula,"FCODA");
	CodaOriginale=memoPtr(hdlCodaOriginale,NULL);

	strcpy(FormulaGlobale,Formula);

 // ------------------------------------------------------------|
 // FASE 1                                                      |
 // Estre le formule semplici (quelle senza parentesi)          |
 // ricerca le pi— profonde sostituendole con autovariabili     |
 // fino a risalire alle meno profonde                          |
 // ------------------------------------------------------------|

/*
 for(;strstr(FormulaGlobale,"(")!=NULL;)
 {
	CHAR *p,*pstart,*pend;

	// -----------------------------------
	// Trovo la Parentesi + profonda     |
	// -----------------------------------
	pstart=NULL;
	for (p=FormulaGlobale;*p;p++)
	 {
		if (*p=='(')
			 {if (p>FormulaGlobale) {if (!IsMathOperator(*(p-1))) continue;}
				pstart=p;
			 }
	 }
	
	if (pstart==NULL) break; // non ci sono + parentesi

	// -----------------------------------
	// Trovo la Parentesi che chiude     |
	// -----------------------------------
	if (Fdebug) {win_infoarg("PARENTESI:\n");}
	pend=ParChiudi(pstart);
	if (pend==NULL) ehExit("() e che cavolo!");

	if (Fdebug) win_infoarg(FormulaGlobale);
	*pend=0;
	if (Fdebug) {win_infoarg(" extract [%s]\n",pstart+1); }


	// -------------------------------------------------
	// Converto la formula senza parantesi in un valore|
	// -------------------------------------------------

	strcpy(CodaOriginale,pend+1);
	if (SimpleFORMtoNumber(pstart+1,funcExt)) return ON;
	strcpy(pstart,pstart+1);
	strcat(FormulaGlobale,CodaOriginale);
	if (Fdebug) {win_infoarg("Calc1 [%s] \n",FormulaGlobale); }
 }
*/
	while (TRUE)
	{
		// -------------------------------------------
		// Trovo la Parentesi + profonda che apre    |
		// -------------------------------------------
		BOOL fString=FALSE;
		CHAR *p,*pstart,*pend;

		pstart=NULL;
		for (p=FormulaGlobale;*p;p++)
		 {
			if (*p=='\"') {fString^=1; continue;}

			if (*p=='('&&!fString)
			{if (p>FormulaGlobale) {if (!IsMathOperator(*(p-1))) continue;}
			  pstart=p;
			}
		 }

		if (pstart==NULL) break; // non ci sono + parentesi

		// -----------------------------------
		// Trovo la Parentesi che chiude     |
		// -----------------------------------
		if (Fdebug) {win_infoarg("PARENTESI:\n");}
		pend=ParChiudi(pstart);
		if (pend==NULL) //ehExit("() e che cavolo! |%s|",pstart);
		{
			 if (extErrorNotify) (*extErrorNotify)(2,"() Parentesi errate: %s",pstart);
			 return TRUE;
		}

		if (Fdebug) win_infoarg(FormulaGlobale);
		*pend=0;
		if (Fdebug) {win_infoarg(" extract [%s]\n",pstart+1); }

		// -------------------------------------------------
		// Converto la formula senza parantesi in un valore|
		// -------------------------------------------------

		strcpy(CodaOriginale,pend+1);
		if (SimpleFORMtoNumber(pstart+1,funcExt)) return ON;
		strcpy(pstart,pstart+1);
		strcat(FormulaGlobale,CodaOriginale);
		if (Fdebug) {win_infoarg("Calc1 [%s] \n",FormulaGlobale); }
	}
	if (Fdebug) {win_infoarg("ULTIMO PASS:\n");}

 //if (SimpleFORMtoAutoVar(FormulaGlobale,CodeProcedure)) return ON;

 // ------------------------------------------------------------|
 // FASE 2                                                      |
 // Finite le parentesi converte ci• che rimane                 |
 // Sostituisce le formule racchiuse in parentesi con variabili |
 // ------------------------------------------------------------|

	if (First_OperatorConverter(FormulaGlobale,funcExt)) {fRet=ON; goto FINE;}
	if (Second_OperatorConverter(FormulaGlobale,funcExt)) {fRet=ON; goto FINE;}
	if (BOOLEAN_OperatorConverter(FormulaGlobale,funcExt)) {fRet=ON; goto FINE;}
	
	if (Fdebug) win_infoarg("Ultima conversione: [%s]",FormulaGlobale);
	*Valore=atof(FormulaGlobale);	

 // ------------------------------------------------------------|
 // FASE X                                                      |
 // Libero la memoria usata per l'elaborazione                  |
 // ------------------------------------------------------------|
FINE:
// DELVARautopush(); Non posso usarlo perchŠ addesso Š diventata ricorsiva
 memoFree(hdlFormula,"Form");
 memoFree(hdlCodaOriginale,"FCODA");

 //Fdebug=OFF;
 if (Fdebug) {win_infoarg("ESC FORMULA\n"); }

//  CODE.ALUIndex=ALUIndexStart;**
 return fRet;
}

static BOOL IsMathOperator(CHAR Byte)
{
 CHAR Find[2];
 Find[0]=Byte;
 Find[1]=0;

 if (strstr("=+-/*><|&^(",Find)) return ON; else return OFF;
}


// Ritorna Null se c'è un errore
static CHAR *ParChiudi(CHAR *p)
{
 SINT num=0;
 BOOL String=0;
 for (;*p;p++)
 {
	if (*p=='"') String^=1;
	if (String) continue;

	if (*p=='(') num++;
	if (*p==')') num--;
	if (num==0) {break;}
 }
 if (num) return NULL;
 return p;
}




//   +-------------------------------------------+
//   | CODE operation                            |
//   +-------------------------------------------+

static CHAR *GiveMeElement(CHAR *Formula,
						   CHAR *Elemento, // CE1
						   CHAR *Operatore) // COp
{
 CHAR *p;
 SINT SWI=0;
 SINT np=0;
 CHAR *E1=Elemento,*Op=Operatore;
 SINT Eopera=OFF;
 BOOL String=0;

// printf(">GME START=[%s]\n",Formula); 
 for (p=Formula;*p;p++)
 {

	// E' una costante stringa
	if (*p=='"') {if (SWI==1) break; else String^=1;}
	if (String) goto avanza;

	// E' una parentesi cerco la chiusura
	if (*p=='(') np++;//p=ParChiudi(p); //if (p==NULL) ehExit("Elemento");}
	if (*p==')') np--;//p=ParChiudi(p); //if (p==NULL) ehExit("Elemento");}

	// Se sono fuori dalle parentesi controllo l'operatore
	if (np==0)
	{

	if ((*p=='+')||(*p=='-')||(*p=='/')||(*p=='*')||
			(*p=='>')||(*p=='<')||(*p=='&')||(*p=='!')||
			(*p=='=')||(*p=='|')||(*p=='^')) Eopera=ON; else Eopera=OFF;
	}


avanza:

	if ((SWI==0)&&Eopera) SWI++;
	if ((SWI==1)&&!Eopera) break;

	switch (SWI)
	 {
		case 0: *E1=*p; E1++;break;
		case 1: *Op=*p; Op++;break;
	 }
 }


 *E1=0; *Op=0;
// printf("<GME END: Elemento:[%s] Operatore[%s]\n",Elemento,Operatore); 

 return p;
}




// -----------------------------------------
// SOSTITUISCE LA FORMULA CON LA VARIABILE !
// -----------------------------------------

static CHAR *Swappa(CHAR *Formula,double Valore,SINT PtStart,CHAR *PtEnd)
{
	CHAR Calcolo2[512];
	CHAR *p;

	if (PtStart!=0) memcpy(Calcolo2,Formula,PtStart);
	Calcolo2[PtStart]=0;
	sprintf(Calcolo2+PtStart,"%.3f",Valore);
	strcat(Calcolo2,PtEnd);//p2+strlen(E2));
	strcpy(Formula,Calcolo2);

	p=Formula+PtStart;
	return p;
}
/*

// -------------------------------------------------------------|
// StringCompareConverter                                       |
// Sostituisce una comparazione con stringa in un valore        |
//                                                              |
// -------------------------------------------------------------|

static SINT StringPush(CHAR *Token,SINT CodeProcedure)
{
 SINT pa;
	 switch (_WhatIsThis(Token,CodeProcedure))
		{
		 case I_CHAR:
		 case I_CHARCONST:
						 pa=VARfind(Token,CodeProcedure,ON);
						 if (pa<0)
							 {
								ErrBarra(14,11,"VARIABILE INESISTENTE",Token);
								return RE_ERR;
							 }
						 VARGet(pa);
						 ASMPush("LD HL,",Variabile.offset,(WORD) pa,0);
						 break;

		 case I_CHARFIX:

						 if (VARConstAllocate(NULL,CodeProcedure,Token+1,strlen(Token)-2,&pa)) return ON;

						 VARGet(pa);
						 ASMPush("LD HL,",Variabile.offset,(WORD) pa,0);
						 break;

		default:
						if (ALUCharFormulaToASM(Token,NULL,CodeProcedure,NULL)) return RE_ERR;
						break;
		}

	ASMPush("PUSH HL",0,0,0);
	return OFF;
}


static SINT StringCompareConverter(CHAR *S1,CHAR *S2,
								   CHAR *Operatore,
								   SINT CodeProcedure)
{
 CHAR Serv[200];
 SINT Type1,Type2;

 if (Fdebug) {win_infoarg("String>Converter:[%s][%s][%s]",S1,Operatore,S2); }

 Type1=_WhatIsThis(S1,CodeProcedure);
 Type2=_WhatIsThis(S2,CodeProcedure);

 if ((Type1==I_CHARFIX)&&(Type2==I_CHARFIX))
		{
			ErrBarra(14,11,"WARNING:","CONFRONTO/OPERAZIONE INUTILE");
			return RE_ERR;
		}

 if (!*Operatore)
		{
			ErrBarra(14,11,"ASSEGNAZIONE ERRATA","");
			return RE_ERR;
		}

 if ((Type1==I_CHAR)||(Type1==I_CHARCONST)) VARSetUsata(S1,CodeProcedure);
 if ((Type2==I_CHAR)||(Type2==I_CHARCONST)) VARSetUsata(S2,CodeProcedure);

 if (strstr("> < = => =< <= >= <>",Operatore)==NULL)
		{
			ErrBarra(14,11,"CONFRONTO e/o OPERAZIONE ERRATA","");
			return RE_ERR;
		}

 if (StringPush(S1,CodeProcedure)) return RE_ERR;
 if (StringPush(S2,CodeProcedure)) return RE_ERR;

// ASMPush("; ---- StrCmp() ",0,0,0);
 swin_infoarg(Serv,"; ---- %s%s%s ?",S1,Operatore,S2);
 ASMPush(Serv,0,0,0);

 ASMPush("CALL  S.STRCMP",0,0,0);
 ASMPush("POP DE",0,0,0);
 ASMPush("POP DE",0,0,0);

 // IN HL MI RITORNA:
 // 1 =
 // 2 >
 // 4 <

 // DEVE DIVENTARE 1 o 0 (VERO O FALSO)
 ASMPush("; ---- Vero o Falso in HL",0,0,0);

 // = Uguaglianza
 if (!strcmp(Operatore,"="))
		{
		 ASMPush("LD A,L",1,0,0);
		 ASMPush("AND N",1,0,0);
		 ASMPush("LD L,A",1,0,0);
		}

 // <> Diverso
 if (!strcmp(Operatore,"<>"))
		{
		 ASMPush("LD A,L",1,0,0);
		 ASMPush("AND N",1,0,0);
		 ASMPush("XOR N",1,0,0);
		 ASMPush("LD L,A",1,0,0);
		}

 // > Maggiore
 if (!strcmp(Operatore,">"))
		{
		 ASMPush("LD A,L",1,0,0);
		 ASMPush("SRL A",1,0,0);
		 ASMPush("AND N",1,0,0);
		 ASMPush("LD L,A",1,0,0);
		}

 // < Minore
 if (!strcmp(Operatore,"<"))
		{
		 ASMPush("LD A,L",1,0,0);
		 ASMPush("SRL A",1,0,0);
		 ASMPush("SRL A",1,0,0);
		 ASMPush("AND N",1,0,0);
		 ASMPush("LD L,A",1,0,0);
		}

 // >= Maggiore/Uguale
 if (!strcmp(Operatore,">=")||!strcmp(Operatore,"=>"))
		{
		 ASMPush("LD A,L",1,0,0);
		 ASMPush("AND N",3,0,0);
		 ASMPush("LD L,A",1,0,0);
		 ASMPush("JR Z",2,0,0);
		 ASMPush("LD L,",1,0,0);
		}

 // <= Minore/Uguale
 if (!strcmp(Operatore,"<=")||!strcmp(Operatore,"=<"))
		{
		 ASMPush("LD A,L",1,0,0);
		 ASMPush("AND N",5,0,0);
		 ASMPush("LD L,A",1,0,0);
		 ASMPush("JR Z",2,0,0);
		 ASMPush("LD L,",1,0,0);
		}

 return RE_HL;
}
*/

// -------------------------------------------------------------|
// ElementNumConverter                                          |
// Converte un elemento in un numero                            |
// L'elemento poò essere:                                       |
// - Numero decimale                                            |
// - Variabile                                                  |
// - Funzione (DA FARE)                                         |
//                                                              |
//                                                              |
// -------------------------------------------------------------|

static BOOL ElementNumConverter(CHAR *lpElement,FORMULA_FUNC_EXT,double *Valore)
{
 SINT Tipo;
 *Valore=0;
// if (Fdebug) win_infoarg("Converter : Element[%s] =",lpElement);
 Tipo=_WhatIsThis(lpElement,funcExt,Valore);
 //if (Fdebug) {win_infoarg("%.3f (tipo= %d) " CRLF,*Valore,Tipo); }

 if (Tipo==E_VARNUM) return FALSE;
 if (Tipo==E_INTFUNC) return FALSE; // Il valore è già presente
 if (Tipo==E_NUMDEC) {*Valore=atof(lpElement); return FALSE;}
 iFormLastError=EE_VARUNKNOW; strcpy(szAddInfo,lpElement);
 return TRUE;
}


static CHAR * ElementAllocStringConverter(CHAR *lpElement,FORMULA_FUNC_EXT)
{
 SINT Tipo;
 double dValore;
 CHAR *lp;
 BOOL fCheck;

 dValore=0;
 Tipo=_WhatIsThis(lpElement,funcExt,&dValore);

 if (Tipo==E_CHARFIX) 
 {
	 lp=malloc(strlen(lpElement));
	 strcpy(lp,lpElement+1);
	 lp[strlen(lpElement)-2]=0;
	 return lp;
 }

 if (Tipo==E_VARCHAR)  // Variabile stringa (chiedo alla funzione esterna)
 {
	 iFormLastError=EE_VARUNKNOW;

	 // ------------------------------------------------------------------
	 // 1) Leggo se esiste e la dimensione
	 //
	 fCheck=(*funcExt)(WS_FIND,0,lpElement,&dValore); if (fCheck) return NULL;
	 //printf("%s lunga %d",lpElement,(SINT) dValore); getch();

	 // ------------------------------------------------------------------
	 // 2) Alloco e richiedo la variabile
	 //
	 lp=malloc((SINT) dValore);
	 fCheck=(*funcExt)(WS_REALGET,0,lpElement,lp);
	 if (!fCheck) return lp;
 }

 return NULL; // Non trovato
}

// -------------------------------------------------------------|
// First_OperatorConverter                                          |
// Operatori di prorità alta (vanno analizzati per primi Es     |
//                                                              |
// SOSTITUISCE I CALCOLI CON LE GLI OPERATORI  / * << >>        |
// CON DELLE AUTOVARIABILI                                      |
// Risultato in Autovariabili                                   |
// -------------------------------------------------------------|
/*
static SINT First_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT)
{
 CHAR *p,*p2;
 CHAR E1[200];
 CHAR E2[200];
 CHAR Operatore[5];
 CHAR Opera[5];
 //WORD Ritorno;
 SINT PTstart,Segue;
 CHAR *PTend;
 double Numero1,Numero2;
 double Risultato;
 BOOL fPrimo=FALSE;
 
 if (Fdebug) win_infoarg(">First_OperatorConverter START=[%s]\n",pszString);
 p=pszString;
 //Segue=OFF;
 PTstart=0; PTend=NULL;

 *Opera=0; *E1=0;
 for(;;)
	 {
		//p=DueElementi(pszString,E1,E2,Operatore);
		SINT NewStart;
		p2=p;
		//win_infoarg(":: [%s]",p);
		p=GiveMeElement(p,E2,Operatore);
		if (Fdebug) {win_infoarg(" ---> [%s] . [%s] . [%s]\n",E1,Opera,E2); }
		
		// Trova il puntatore al primo elemento
		NewStart=(SINT) ((LONG) (p2)-(LONG) pszString);
		
		win_infoarg("%d) p2=[%s] PTStart=[%s]",fPrimo,p2,pszString+PTstart);

		//if (z==100) {FlagRet=ON;goto FINE;}
		//if (!*Operatore) break;

		if ((!strcmp(Opera,"*"))||
			(!strcmp(Opera,">>"))||
			(!strcmp(Opera,"<<"))||
			(!strcmp(Opera,"/")))
			{
			 //WORD lpVar;
			 BOOL fCheck;

			 win_infoarg("Calcolo %s %s %s (%s)",E1,Opera,E2,pszString+PTstart);
			 // Converte l'elemento in un numero
			 //if (!Segue)
			 //{
			  fCheck=ElementNumConverter(E1,funcExt,&Numero1);
			  if (fCheck) ehExit("Err 2001");
			  //PTstart=(SINT) ((LONG) (p2-strlen(E1)-strlen(Opera))-(LONG) pszString);
			  //Segue=ON;
			 //}

			 //}
			 //else
			 //{
			  fCheck=ElementNumConverter(E2,funcExt,&Numero2);
			  if (fCheck) ehExit("Err 2002");
			  PTend=p2+strlen(E2);
			 //}

			 //  MOLTIPLICAZIONE                  !
			 if (!strcmp(Opera,"*")) Risultato=Numero1*Numero2;

			 //  DIVISIONE                        !
			 if (!strcmp(Opera,"/")) Risultato=Numero1/Numero2;
			 
			 //  Shift a Destra                  !
			 if (!strcmp(Opera,">>")) Risultato=(LONG) Numero1>>(LONG) Numero2;
			 //  Shift a Sinistra                !
			 if (!strcmp(Opera,"<<")) Risultato=(LONG) Numero1<<(LONG) Numero2;
			 
			 if (Fdebug) win_infoarg(" Risultato Prima [%s] PtStart[%s]\n",pszString,pszString+PTstart);
		  	 p=Swappa(pszString,Risultato,PTstart,PTend);
			 if (Fdebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
			 *Operatore=0;*E2=0;
			 fPrimo=FALSE;
		} 

	 if (*p==0) break; // Finito

	 strcpy(E1,E2); PTstart=NewStart;
	 strcpy(Opera,Operatore);
	 }
 if (Fdebug) win_infoarg("<First_OperatorConverter END=[%s]\n",pszString);

 return OFF;
}
*/
static SINT First_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT)
{
 CHAR *p,*p2;
 CHAR E1[200];
 CHAR E2[200];
 CHAR Operatore[5];
 CHAR Opera[5];
 //WORD Ritorno;
 SINT PTstart;
 CHAR *PTend;
 double Numero1,Numero2;
 double Risultato;
 BOOL fPrimo=FALSE;
 CHAR *lpE1,*lpE2;
 
 if (Fdebug) win_infoarg("I) > OperatorConverter START=[%s]\n",pszString);
 p=pszString;
 //Segue=OFF;
 PTstart=0; PTend=NULL;

 *Opera=0; *E1=0; lpE1=NULL;
 for(;;)
	 {
		p2=p;
		p=GiveMeElement(p,E2,Operatore);
		if (Fdebug) {win_infoarg(" ---> [%s] . [%s] . [%s]\n",E1,Opera,E2);}
        
		lpE2=p2; // Ultimo Puntatore a E2
		
		if ((!strcmp(Opera,"*"))||
			(!strcmp(Opera,">>"))||
			(!strcmp(Opera,"<<"))||
			(!strcmp(Opera,"/")))
			{
			 BOOL fCheck;

			 fCheck=ElementNumConverter(E1,funcExt,&Numero1); if (fCheck) return ON;
			 fCheck=ElementNumConverter(E2,funcExt,&Numero2);if (fCheck) return ON;
			 
			 PTstart=(LONG) (lpE1)-(LONG) (pszString);
			 PTend=p2+strlen(E2);

			 //  MOLTIPLICAZIONE                  !
			 if (!strcmp(Opera,"*")) Risultato=Numero1*Numero2;
			 //  DIVISIONE                        !
			 if (!strcmp(Opera,"/")) Risultato=Numero1/Numero2;
			 //  Shift a Destra                  !
			 if (!strcmp(Opera,">>")) Risultato=(LONG) Numero1>>(LONG) Numero2;
			 //  Shift a Sinistra                !
			 if (!strcmp(Opera,"<<")) Risultato=(LONG) Numero1<<(LONG) Numero2;
			 

			 if (Fdebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
		  	 p=Swappa(pszString,Risultato,PTstart,PTend);
			 if (Fdebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
			 *Operatore=0;*E2=0;
			 fPrimo=FALSE;
		} 

	 if (*p==0) break; // Finito
	 strcpy(E1,E2);  lpE1=lpE2;
	 strcpy(Opera,Operatore);
	 }

 if (Fdebug) win_infoarg("I) < OperatorConverter END=[%s]\n",pszString);
 return OFF;
}

static SINT Second_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT)
{
 CHAR *p,*p2;
 CHAR E1[200];
 CHAR E2[200];
 CHAR Operatore[5];
 CHAR Opera[5];
 //WORD Ritorno;
 SINT PTstart;
 CHAR *PTend;
 double Numero1,Numero2;
 double dRisultato;
 BOOL fPrimo=FALSE;
 CHAR *lpE1,*lpE2;
 
 if (Fdebug) win_infoarg("II) > OperatorConverter START=[%s]\n",pszString);
 p=pszString;
 //Segue=OFF;
 PTstart=0; PTend=NULL;

 *Opera=0; *E1=0; lpE1=NULL;
 while(TRUE)
	 {
		p2=p;
		p=GiveMeElement(p,E2,Operatore);
		if (Fdebug) {win_infoarg(" ---> [%s] . [%s] . [%s]\n",E1,Opera,E2);}
        
		lpE2=p2; // Ultimo Puntatore a E2

		// Controllo stringhe
		if (IsString(E1)&&IsString(E2))
		{
		if (
			(!strcmp(Opera,"="))||
			(!strcmp(Opera,"<"))||
			(!strcmp(Opera,">"))||
			(!strcmp(Opera,"<="))||
			(!strcmp(Opera,"=<"))||
			(!strcmp(Opera,">="))||
			(!strcmp(Opera,"=>"))||
			(!strcmp(Opera,"<>"))||
			(!strcmp(Opera,"!="))||
			(!strcmp(Opera,"=="))
			)
			{
				CHAR *lpString1;
				CHAR *lpString2;

				lpString1=ElementAllocStringConverter(E1,funcExt); if (!lpString1) return ON;
				lpString2=ElementAllocStringConverter(E2,funcExt); if (!lpString2) {free(lpString1); return ON;}
				PTstart=(LONG) (lpE1)-(LONG) (pszString);
				PTend=p2+strlen(E2);

				dRisultato=0;
				// Uguaglianza
				if (!strcmp(Opera,"=")||!strcmp(Opera,"==")) {dRisultato=strcmp(lpString1,lpString2)?0:1;}
				// Maggiore 
				if (!strcmp(Opera,">")) dRisultato=strcmp(lpString1,lpString2)>0?1:0;
				// Minore
				if (!strcmp(Opera,"<")) dRisultato=strcmp(lpString1,lpString2)<0?1:0;
				// Maggiore uguale
				if (!strcmp(Opera,">=")||!strcmp(Opera,"=>")) dRisultato=strcmp(lpString1,lpString2)>=0?1:0;
				// Minore uguale
				if (!strcmp(Opera,"<=")||!strcmp(Opera,"=<")) dRisultato=strcmp(lpString1,lpString2)<=0?1:0;
				// Diverso
				if (!strcmp(Opera,"<>")||!strcmp(Opera,"!=")) dRisultato=strcmp(lpString1,lpString2);
				/*
				printf("Stringa [%s]%s[%s]%d",
					   lpString1,Opera,lpString2,
					   (SINT) dRisultato);	 
				getch();
				*/
				free(lpString1);
				free(lpString2);

			 //if (Fdebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
		  	 p=Swappa(pszString,dRisultato,PTstart,PTend);
			 //if (Fdebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
			 *Operatore=0;*E2=0;
			 fPrimo=FALSE;
			}
		}
		else
		{
		 if ((!strcmp(Opera,"+"))||
			(!strcmp(Opera,"-"))||

			(!strcmp(Opera,"&"))||
			(!strcmp(Opera,"|"))||
			(!strcmp(Opera,"^"))||

			(!strcmp(Opera,"="))||
			(!strcmp(Opera,"<"))||
			(!strcmp(Opera,">"))||
			(!strcmp(Opera,"<="))||
			(!strcmp(Opera,"=<"))||
			(!strcmp(Opera,">="))||
			(!strcmp(Opera,"=>"))||
			(!strcmp(Opera,"<>"))||
			(!strcmp(Opera,"!="))||
			(!strcmp(Opera,"=="))
			)
			{
			 BOOL fCheck;

			 fCheck=ElementNumConverter(E1,funcExt,&Numero1); if (fCheck) return ON;
			 fCheck=ElementNumConverter(E2,funcExt,&Numero2);if (fCheck) return ON;
			 
			 PTstart=(LONG) (lpE1)-(LONG) (pszString);
			 PTend=p2+strlen(E2);
			 
			 // Somma
			 if (!strcmp(Opera,"+")) dRisultato=Numero1+Numero2;
			 // Sottrazione
			 if (!strcmp(Opera,"-")) dRisultato=Numero1-Numero2;

			 // And
			 if (!strcmp(Opera,"&")) dRisultato=((SINT) (Numero1) & (SINT) (Numero2));
			 // Or
			 if (!strcmp(Opera,"|")) dRisultato=((SINT) (Numero1)|(SINT) (Numero2));
			 // Xor
			 if (!strcmp(Opera,"^")) dRisultato=((SINT) (Numero1)|(SINT) (Numero2));

			 // Uguaglianza
			 if (!strcmp(Opera,"=")||!strcmp(Opera,"==")) dRisultato=(Numero1==Numero2);
			 // Maggiore 
			 if (!strcmp(Opera,">")) dRisultato=(Numero1>Numero2);
			 // Minore
			 if (!strcmp(Opera,"<")) dRisultato=(Numero1<Numero2);
			 // Maggiore uguale
			 if (!strcmp(Opera,">=")||!strcmp(Opera,"=>")) dRisultato=(Numero1>=Numero2);
			 // Minore uguale
			 if (!strcmp(Opera,"<=")||!strcmp(Opera,"=<")) dRisultato=(Numero1<=Numero2);
			 // Diverso
			 if (!strcmp(Opera,"<>")||!strcmp(Opera,"!=")) dRisultato=(Numero1!=Numero2);

			 //if (Fdebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
		  	 p=Swappa(pszString,dRisultato,PTstart,PTend);
			 //if (Fdebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
			 *Operatore=0;*E2=0;
			 fPrimo=FALSE;
		 }
		}

	 if (*p==0) break; // Finito
	 strcpy(E1,E2);  lpE1=lpE2;
	 strcpy(Opera,Operatore);
	 }

 if (Fdebug) win_infoarg("II) < OperatorConverter END=[%s]\n",pszString);
 return OFF;
}

//-----------------------------------------------=
//  Second_OperatorConverter                         !
//  Calcola la formula con soli operatori di     !
//  bassa priorit… in HL                         !
//                                               !
//  Risultato in HL                              !
// -----------------------------------------------
/*
static SINT Second_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT)
{
 SINT Type;
 CHAR *p;
 CHAR E1[200];
// CHAR E2[200];
 CHAR Operatore[5];
 CHAR Opera[5];
// WORD Ritorno;
 double Risultato=0;
 double Valore;

 if (Fdebug) {win_infoarg("> Second_OperatorConverter START=[%s]\n",pszString); }

 p=pszString;
 strcpy(Opera,"");
 while (TRUE)
	{//LONG andress;
//	 WORD lpVar;

	 p=GiveMeElement(p,E1,Operatore);
//	 if (Fdebug) {win_infoarg("LOWEstratto [%s][%s]  \n",E1,Operatore); }

	 Type=ElementNumConverter(E1,funcExt,&Valore);
	 if (Type) return ON;

	 // -----------------------------------
	 // PRIMA VOLTA : Assegna un valore   !
	 // -----------------------------------

	 if (!*Opera) {Risultato=Valore;}

	 // -----------------------------------
	 //  SOMMA AL VALORE PRECEDENTE       !
	 // -----------------------------------

	 if (*Opera) 
	 {
		if (Fdebug) {win_infoarg("Calcolo [%.2f] %s [%.2f] \n",Risultato,Opera,Valore); }
	 }

	 if (!strcmp(Opera,"+"))
		{
		 Risultato+=Valore;
		 goto AVANTI;
		}

	 // -----------------------------------
	 //  SOTTRAE AL VALORE PRECEDENTE     !
	 // -----------------------------------
	 if (!strcmp(Opera,"-"))
			{
			 Risultato-=Valore;
			 goto AVANTI;
			}

	 // -----------------------------------
	 //  = AL VALORE PRECEDENTE (BOOLEAN) !
	 // -----------------------------------
	 if (!strcmp(Opera,"="))
			{
			 Risultato=(Valore==Risultato);
			 goto AVANTI;
			}

	 // -----------------------------------
	 //  > AL VALORE PRECEDENTE (BOOLEAN) !
	 // -----------------------------------
	 if (!strcmp(Opera,">"))
			{
			 Risultato=(Risultato>Valore);
			 goto AVANTI;
			}

	 // -----------------------------------
	 //  < AL VALORE PRECEDENTE (BOOLEAN) !
	 // -----------------------------------
	 if (!strcmp(Opera,"<"))
			{
			 Risultato=(Risultato<Valore);
			 goto AVANTI;
			}

	 // -----------------------------------
	 //  <= AL VALORE PRECEDENTE (BOOLEAN) !
	 // -----------------------------------
	 if ((!strcmp(Opera,"<="))||(!strcmp(Opera,"=<")))
			{
			 Risultato=(Risultato<=Valore);
			 goto AVANTI;
			}

	 // ------------------------------------
	 //  >= AL VALORE PRECEDENTE (BOOLEAN) !
	 // ------------------------------------
	 if ((!strcmp(Opera,">="))||(!strcmp(Opera,"=>")))
			{
			 Risultato=(Risultato>=Valore);
			 goto AVANTI;
			}

	 // ------------------------------------
	 //  <> AL VALORE PRECEDENTE (BOOLEAN) !
	 // ------------------------------------
	 if ((!strcmp(Opera,"<>"))||(!strcmp(Opera,"!=")))
			{
			 Risultato=(Valore!=Risultato);
			 goto AVANTI;
			}

	 // ------------------------------------
	 //  & AND LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(Opera,"&"))
			{
			 Risultato=(Valore&&Risultato);
			 goto AVANTI;
			}

	 // ------------------------------------
	 //  | OR  LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(Opera,"|"))
			{
			 Risultato=(Valore||Risultato);
			 goto AVANTI;
			}

	 // ------------------------------------
	 //  ^ XOR  LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(Opera,"^"))
			{
			 Risultato=((LONG) Valore^(LONG) Risultato);
			 goto AVANTI;
			}

	 // Posto operatori booleani
	 if (!strcmp(Opera,"&&")||!strcmp(Opera,"||"))
	 {
		Risultato=Valore;
		goto AVANTI;
	 }

	 if (*Opera)
	 {
		if (Fdebug) {win_infoarg("Opera ? [%s]",Opera); }
		win_infoarg("Operatore sconosciuto: %s",Opera);
		return ON;
	 }

	 AVANTI:
	 if (*p==0) break; // Finito
	 strcpy(Opera,Operatore);
	}

 sprintf(pszString,"%.3f",Risultato);
 if (Fdebug) {win_infoarg("<Second_OperatorConverter:  [%s]",pszString); }

 return OFF;
}
*/

//-----------------------------------------------=
//  BOOLEAN_OperatorConverter
//  Calcola la formula con soli operatori di     !
//  bassa priorit… in HL                         !
//                                               !
//  Risultato in HL                              !
// -----------------------------------------------
static SINT BOOLEAN_OperatorConverter(CHAR *pszString,BOOL (*funcExt)(EN_MESSAGE,SINT,CHAR*,void*))
{
 SINT Type;
 CHAR *p;
 CHAR E1[200];
// CHAR E2[200];
 CHAR Operatore[5];
 CHAR Opera[5];
// WORD Ritorno;
 double Risultato=0;
 double Valore;

 if (Fdebug) {win_infoarg(">BOOLEAN_OperatorConverter START=[%s]\n",pszString); }

 p=pszString;
 strcpy(Opera,"");
 while (TRUE)
	{//LONG andress;
//	 WORD lpVar;

	 p=GiveMeElement(p,E1,Operatore);
//	 if (Fdebug) {win_infoarg("LOWEstratto [%s][%s]  \n",E1,Operatore); }

	 /*
	 // -----------------------------------
	 // GESTIONE CONFRONTI TRA STRINGHE   !
	 // -----------------------------------
	 Type=_WhatIsThis(E1,CodeProcedure);
	 if ((Type==I_CHAR)||
			 (Type==I_CHARFIX)||
			 (Type==I_CHARCONST))
		 {
			 // E una somma di stringhe ? SI,converto in un autostringa
			 if (!strcmp(Operatore,"+"))
				 {
					 //win_infoarg("Ecco: [%s][%s]",pszString,Operatore); 
					 if (ALUCharFormulaToASM(pszString,NULL,CodeProcedure,E1)) return ON;
					 //win_infoarg("->> [%s]",E1);  //ehExit("CAZ");
					 strcpy(pszString,E1); break;
				 }

			 strcpy(Opera,Operatore);
			 p=GiveMeElement(p,E2,Operatore);
			 Type=StringCompareConverter(E1,E2,Opera,CodeProcedure);
			 if (Type==RE_ERR) return ON;
			 strcpy(Opera,Operatore);
		 }
		 else
*/
	 Type=ElementNumConverter(E1,funcExt,&Valore);
	 if (Type) return ON;

	 // -----------------------------------
	 // PRIMA VOLTA : Assegna un valore   !
	 // -----------------------------------

	 if (!*Opera) {Risultato=Valore;}

	 // -----------------------------------
	 //  SOMMA AL VALORE PRECEDENTE       !
	 // -----------------------------------

	 if (*Opera) 
	 {
		if (Fdebug) {win_infoarg("Calcolo [%.2f] %s [%.2f] \n",Risultato,Opera,Valore); }
	 }

	 // ------------------------------------
	 //  & AND LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(Opera,"&&"))
			{
			 Risultato=(Valore&&Risultato);
			 goto AVANTI;
			}

	 // ------------------------------------
	 //  | OR  LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(Opera,"||"))
			{
			 Risultato=(Valore||Risultato);
			 goto AVANTI;
			}
/*
	 // ------------------------------------
	 //  ^ XOR  LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(Opera,"^"))
			{
			 Risultato=((LONG) Valore^(LONG) Risultato);
			 goto AVANTI;
			}
*/
	 if (*Opera)
	 {
		if (Fdebug) {win_infoarg("Opera ? [%s]",Opera); }
		
		if (extErrorNotify) (*extErrorNotify)(3,"Operatore sconosciuto: %s",Opera);
		return TRUE;
	 }

	 AVANTI:
	 if (*p==0) break; // Finito
	 strcpy(Opera,Operatore);
	}

 sprintf(pszString,"%.3f",Risultato);
 if (Fdebug) {win_infoarg("<BOOLEAN_OperatorConverter: \n %s %.3f",pszString,Risultato); }

 return OFF;
}


//-----------------------------------------------=
//  SimpleFORMtoNumber                           !
//  Converte una formula !!SENZA LE PARENTESI!!  !
//  in una Numero statico finito                 !
//                                               !
//  Converte una formula tra stringhe (+)        !
//  in un auto stringa                           !
//                                               !
//  Ritorna il Nome della variabile              !
//  ON   Errore                                  !
//  OFF  Tutto OK                                !
//                                               !
// -----------------------------------------------
static SINT SimpleFORMtoNumber(CHAR *Formula,FORMULA_FUNC_EXT)
{
	SINT Tipo;
	double Valore=0;
	if (Fdebug) win_infoarg(">> SimpleFORMtoNumber START=|%s| \n",Formula);

	// -------------------------------------------
	// Se la formula Š un numero costante        !
	// non la trasformo in auto variabile        !
	// -------------------------------------------

	// La formula è un unica variabile Numerica
	Tipo=_WhatIsThis(Formula,funcExt,&Valore);

 if (Tipo==E_INTFUNC)
		{
		 sprintf(Formula,"%.3f",Valore);
		 goto FINE;
		 }

 if (Tipo==E_VARNUM)
		{//sprintf(Nome,"%u",CONSTvalore(Formula));
	     if (funcExt) (*funcExt)(WS_FIND,0,Formula,&Valore);
		 sprintf(Formula,"%.3f",Valore);
		 goto FINE;
		 }

 // -------------------------------------------
 // IDEM se Š una variabile/costante          !
 // -------------------------------------------
 // E' un unico NUMERO o una VARIABILE o una COSTANTE
 if ((Tipo==E_NUMDEC)||
	 (Tipo==E_NUMHEX)||
	 (Tipo==E_NUMBIN)) goto FINE;

 // -------------------------------------------
 // Controllo sulle Stringhe                  !
 // -------------------------------------------
 //if ((Tipo==I_CHARFIX)||(Tipo==I_CHARCONST)||(Tipo==I_CHAR)) goto FINE;

 // -------------------------------------------
 // Converte gli operatori di priorit… ALTA   !
 // in AutoVariabili                          !
 // -------------------------------------------

 if (First_OperatorConverter(Formula,funcExt)) return ON;
 Tipo=_WhatIsThis(Formula,funcExt,NULL);
 if ((Tipo==E_NUMDEC)||(Tipo==E_NUMHEX)||(Tipo==E_NUMBIN)) goto FINE;

 if (Second_OperatorConverter(Formula,funcExt)) return ON;
 if (BOOLEAN_OperatorConverter(Formula,funcExt)) return ON;

 // Mi ritorna una stringa
 //if (strstr(Formula,"ACHR")) goto FINE;

 //VARautopush(Nome); // Richiede una variabile
 //	 VARStackPush(RE_HL,Nome);
 if (Fdebug) {win_infoarg("AUTOPUSH[%s]\n",Formula); }
	 //ASMVpush("HL>",Nome);
	 //VARStackPush(RE_HL,Nome);

//	 if (Fdebug) {win_infoarg("Offset[%ld]\n",off); }
	 //VARSetUsata(Nome,CodeProcedure);
 //strcpy(Formula,Nome);

 FINE:
 if (Fdebug) {win_infoarg("<< SimpleFORMtoNumber END=|%s| \n",Formula); }
 return OFF;
}


//
// _WhatIsThis()
//
static SINT _WhatIsThis(CHAR *okl,FORMULA_FUNC_EXT,double *lpRetValore)
{
	double Valore;
	BOOL fCheck;
	CHAR *p;
	BOOL fString;
	CHAR *lpClose;

	if (*okl<'A'||*okl=='-')
	 {
		 if (IsDecNumero(okl)) return E_NUMDEC;
		 if (IsHexNumero(okl)) return E_NUMHEX;
		 if (IsBinNumero(okl)) return E_NUMBIN;
		 if (IsStringFix(okl)) return E_CHARFIX; // ... per intenderci "cazzo" ;-)
		 // Potrebbe essere una formula
		 if ((*okl>='0')&&(*okl<='9')) return E_FORMULANUM;
		 if (*okl=='(') return E_FORMULANUM;
		 return E_ERROR;
	}

 if (IsStringVar(okl)) return E_VARCHAR; // ... per intenderci VALORE$

 // Controllo che non contenga operatori "la cosa" prima di chiedere alla funzione esterna
 fString=FALSE;
 for (p=okl;*p;p++)
 {
	if (*p=='\"') fString^=1;
	if (fString) continue;
	if (*p=='*'||
		*p=='='||
		*p=='>'||
		*p=='<'||
		*p=='/')
		return E_VARUNKNOW; // E' una formula
 }

 // ------------------------------------------------------------
 // Controllo se è una funzione matematica Interna
 // (Forse da inserire parametricamente)
 //
 if (strlen(okl)>5)
 {
	 // Intero (conversione a Intero di un numero)
	 if (!memcmp(okl,"INT(",4)||
		 !memcmp(okl,"FIX(",4))
	 {
		// Post Controllo Funzione
		 lpClose=ParChiudi(okl+3);
		 if (lpClose)
		 {if (!*(lpClose+1))
			{
				// Estraggo formula contenuta nella funzione
				CHAR *lpNewFormula=malloc(strlen(okl)+1);
				BOOL fRet;
				*lpClose=0; strcpy(lpNewFormula,okl+4); *lpClose=')';
				
				//
				// Eseguo la formula
				//
				fRet=L_FormulaProcess(lpNewFormula,&Valore,funcExt);
				// printf("E' [%s]",lpNewFormula); getch();
				free(lpNewFormula);
				
				// Se tutto ok, calcolo il valore della funzione
				if (!fRet)
				{
					//printf("Valore prima %.2f",Valore); getch();
					if (!memcmp(okl,"INT(",4)) Valore=(SINT) (Valore+.5);
					if (!memcmp(okl,"FIX(",4)) Valore=(SINT) Valore;
					// printf("Valore dopo %.2f",Valore); getch();

					if (lpRetValore) *lpRetValore=Valore;
					return E_INTFUNC; // E' una formula
				}
			}
		 }
	 }
 }

 if (funcExt) // Funsione di traslazione Variabile
 {
	 if (lpRetValore) fCheck=funcExt(WS_FIND,0,okl,&Valore);
					  else
					  fCheck=funcExt(WS_FIND,0,okl,NULL);
 }
 
 if (fCheck) return E_VARUNKNOW;

 if (lpRetValore) *lpRetValore=Valore;
 return E_VARNUM;
//return 0;
}

static SINT IsDecNumero(CHAR *pszString)
{
	CHAR *p;
	BOOL fPoint=FALSE;
	BOOL bRet=TRUE;
	
	if (*pszString=='-') pszString++; // Accetto un numero negativo
	for (p=pszString;*p;p++) {

		 if ((*p=='.')&&!fPoint) {fPoint=TRUE; continue;}
		 if ((*p<'0')||(*p>'9')) {bRet=FALSE; break;}

	}
	return bRet;
}


static SINT IsHexNumero(CHAR *pszString)
{
 CHAR *p;
 SINT l;
 p=pszString;
// printf("HEX? [%s]",p); 
 if (memcmp(p,"0X",2)) return OFF;

 p+=2;
 l=strlen(p);
 if ((l<1)||(l>4)) return OFF; // Non valido

 for (;*p;p++)
	 {if ((*p>='0')&&(*p<='9')) continue;
		if ((*p>='A')&&(*p<='F')) continue;
		return OFF;
	 }

 return ON;
}

static SINT IsBinNumero(CHAR *pszString)
{
 CHAR *p;
 SINT l;
 p=pszString;
 if (memcmp(p,"0B",2)) return OFF;
 p+=2;
 l=strlen(p);
 if ((l<1)||(l>16)) return OFF; // Non valido

 for (;*p;p++) {if ((*p<'0')||(*p>'1')) return OFF;}
// printf("PK");
 return ON;
}

static SINT IsStringFix(CHAR *dato)
{
 CHAR *p;
 CHAR szSearch[2];

 p=dato;
 if (*p!='\"'&&*p!='\'') return OFF;
 *szSearch=*p; szSearch[1]=0;
 p++;
 p=strstr(p,szSearch); 
 if (p==NULL) return OFF;
 p++;
 if (*p!=0) return OFF;
 return ON;
}

static SINT IsStringVar(CHAR *dato)
{
// CHAR *p;
 // DA CAMBIARE VEDERE SE DOLLARO E' L'uLTIMO E CE NE é UNOSOLO
 if (strstr(dato,"[")) return FALSE;
 if (strstr(dato,"(")) return FALSE;
 if (strstr(dato,"\'")) return FALSE;
 if (strstr(dato,"\"")) return FALSE;
 if (dato[strlen(dato)-1]=='$') return TRUE;
// p=strstr(dato,"$"); if (p) return TRUE; 
 return FALSE;
}

static BOOL IsString(CHAR *lpCosa)
{
 if (IsStringFix(lpCosa)||IsStringVar(lpCosa)) return TRUE; else return FALSE;
}

//
// getFuncArg() - Parse Function
// Estrae in un array gli argomenti di una funzione
// pCharStart/Stop sono i delimitatori (eventuali parentesi)
//
EH_AR getFuncArg(BYTE *pString,BYTE *pCharStart,BYTE *pCharStop,SINT *piError,SINT *piArgNum)
{
	BYTE *p;
	BYTE bCharStart,bCharEnd;
	BYTE *pArgStart=NULL;
	BYTE *pArg;
	typedef enum {
		BRACKET_START_SEARCH,
		ARG_SEARCH,
		ARG_GET_VARNUM,
		ARG_GET_STRING_SQ,
		ARG_GET_STRING_DQ,
		ARG_ERROR,
		ARG_SEP_SEARCH
	
	} EN_MODE;

	EN_MODE iMode=0;
	EH_AR arArg;
	BOOL bBreak=FALSE;
	BYTE *pError="!\1[ERROR]";
	BYTE bCharArgSep=',';
	SINT iError=0;
	SINT iLen;

	if (piError) *piError=0;
	bCharStart=(pCharStart)?*pCharStart:'(';
	bCharEnd=(pCharStop)?*pCharStop:')';
	arArg=ARNew();

	for (p=pString;*p;p++)
	{
		switch(iMode)
		{
			case BRACKET_START_SEARCH:
				if (*p==bCharStart) iMode=ARG_SEARCH;
				if (*p==bCharEnd) {ARAdd(&arArg,pError); iError++; bBreak=TRUE; break;} // Finito
				break;
		
			case ARG_SEARCH:
				if (strchr(" \t\n\r",*p)) continue;
				iMode=ARG_GET_VARNUM;
				if (*p=='\'') iMode=ARG_GET_STRING_SQ;
				if (*p=='\"') iMode=ARG_GET_STRING_DQ;
				pArgStart=p;
				break;

			//
			// Legge variabili, formule e numeri
			//
			case ARG_GET_VARNUM: 
				if (strchr(" \t\n\r",*p)) continue; // Caratteri neutri
				if (*p==bCharArgSep||*p==bCharEnd) 
				{
					pArg=strTake(pArgStart,p-1); 
					strcpy(pArg,strOmit(pArg," \t\r\n")); 
					ARAddarg(&arArg,"%s\1%s",isNaN(pArg)?"F":"N",pArg);  // F = Formula (numeri e variabili)
					ehFree(pArg);
					iMode=ARG_SEARCH;

					if (*p==bCharEnd) {bBreak=TRUE; break;} // Finito
					continue;
				}

				// Errore 
				if (strchr("\'\"",*p)) 
				{
					ARAdd(&arArg,pError);
					iMode=ARG_ERROR;
					iError++; 
				}
				break;

			//
			// ARG_GET_STRING_SQ 
			//
			case ARG_GET_STRING_SQ: 
				if (*p!='\'') continue;
				pArg=strTake(pArgStart+1,p-1); 
				ARAddarg(&arArg,"S\1%s",pArg);  // 1 = Numero o variabile
				ehFree(pArg);
				iMode=ARG_SEP_SEARCH;
				break;

			case ARG_GET_STRING_DQ: 
				if (*p!='\"') continue;
				pArg=strTake(pArgStart+1,p-1); 
				ARAddarg(&arArg,"S\1%s",pArg?pArg:"");  // 1 = Numero o variabile
				ehFree(pArg);
				iMode=ARG_SEP_SEARCH;
				break;

			case ARG_SEP_SEARCH:
				if (*p==bCharEnd) {bBreak=TRUE; break;} // Finito
				if (*p==bCharArgSep) 
				{
					iMode=ARG_SEARCH;
					continue;
				}
				if (strchr(" \t\n\r",*p)) continue; // Caratteri neutri

				// Converto l'ultimo campo in errore
				iLen=ARLen(arArg);
				strAssign(&arArg[iLen-1],pError);
				iMode=ARG_ERROR; iError++; 
				break;

			case ARG_ERROR:
				if (*p==bCharEnd) {bBreak=TRUE; break;} // Finito
				if (*p==bCharArgSep||*p==bCharEnd) 
				{
					iMode=ARG_SEARCH; 
				}
				break;

			default:
				ehExit("non gestito");
		}
	
		if (bBreak) break;
	}

	if (iMode==ARG_GET_STRING_SQ||iMode==ARG_GET_STRING_DQ) {ARAdd(&arArg,pError); iError++;}
	if (piError) *piError=iError;
	if (piArgNum) *piArgNum=ARLen(arArg);
	return arArg;

}