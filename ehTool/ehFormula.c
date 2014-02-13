//-----------------------------------------------------------------------
//	FormulaProcess  Trasforma la formula in un numero                   |
//											Creato da G.Tassistro       |
//											Ferrà Art & Tecnology 1999  |
//  Ritorna TRUE = Formula errata                                       |
//          FALSE= Formula OK Valore messo in "Valore"                  |
//-----------------------------------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/ehFormula.h"

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
static struct {
	void (*_extErrorNotify)(SINT iMode,CHAR *pMess,...);
	SINT iFormLastError;
	CHAR szAddInfo[80];
	BOOL bDebug;
} _sLocal={0,0,"",0};

static CHAR *szFormError[]={"Formula interpretata correttamente.",
							"Variabile sconosciuta",
							"Errore in formula"};
static SINT SimpleFORMtoNumber(CHAR *Formula,FORMULA_FUNC_EXT);
static SINT First_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT); // Primi operatori da convertire : /*<<>>
static SINT Second_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT); // Secondi operatori +-&^
static SINT BOOLEAN_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT);
static SINT L_FormulaProcess(CHAR *pszFomula,double *Valore,FORMULA_FUNC_EXT);
static BOOL _isMathOperator(CHAR Byte);
static CHAR *ParChiudi(CHAR *p);
static SINT _WhatIsThis(CHAR *okl,FORMULA_FUNC_EXT,void*Valore);
static CHAR * _ElementAllocStringConverter(CHAR * lpElement,FORMULA_FUNC_EXT);

void formulaSetNotify(void (*NewFunction)(SINT iMode,CHAR *pMess,...))
{
	_sLocal._extErrorNotify=NewFunction;
}

void FormulaDebug(SINT flag)
{
 _sLocal.bDebug=flag;
}

static SINT _isDecNumero(CHAR *);
static SINT _isHexNumero(CHAR *);
static SINT _isBinNumero(CHAR *);
static SINT _isStringFix(CHAR *);
static SINT _isStringVar(CHAR *);
static BOOL _isString(CHAR *);

//
// formulaParser() 
// 

BOOL formulaParser(CHAR * pszFomula,
					double *lpdValore, // Valore calcolato di ritorno
					BOOL fViewError, // Se si devono vedere gli errori
					FORMULA_FUNC_EXT // Funzione che restituisce i valore delle Variabili
					)
{
    SINT Ret;
	CHAR *lpNewFormula=ehAlloc(2048);
	CHAR *ps,*pd=lpNewFormula;
	SINT iTipo;

	// Tolgo i caratteri di controllo (CR-LF ecc)
	for (ps=pszFomula;*ps;ps++)
	{
	 if (*ps<33) continue;
	 *pd=*ps; pd++;
	}
    *pd=0;

	//printf(CRLF ">|%s|<" CRLF,lpNewFormula); //getch();
	_sLocal.iFormLastError=EE_OK;
	
	// Se è una singola variabile la elaboro subito
	iTipo=_WhatIsThis(lpNewFormula,funcExt,NULL);
	if (iTipo==E_VARNUM) 
		{
		 if (funcExt) funcExt(WS_REALGET,lpNewFormula,lpdValore); 
	     Ret=FALSE;
		}
	else
	// Altrimenti lancio la formula
	{
		Ret=L_FormulaProcess(lpNewFormula,lpdValore,funcExt);
	}
	/*
#ifdef _CONSOLE
	if (Ret&&fViewError) {printf("Formula.c:\n%s\n(%s)",szFormError[iFormLastError],_sLocal.szAddInfo); }
#else
	if (Ret&&fViewError) win_infoarg("Formula.c:\n%s\n(%s)",szFormError[iFormLastError],_sLocal.szAddInfo);
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
	//_sLocal.bDebug=ON;
	if (_sLocal.bDebug) {win_infoarg("FORMULA |%s|\n",Formula); }

	// ------------------------------------------------------------|
	// FASE 0                                                      |
	// Alloco memoria per l'elaborazione                           |
	// ------------------------------------------------------------|

	if (strlen(Formula)>(WORD) (MaxFormula-1))
	{
		if (_sLocal._extErrorNotify) _sLocal._extErrorNotify(1,"Formula troppo lunga max(%d)",MaxFormula-1);
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
			{if (p>FormulaGlobale) {if (!_isMathOperator(*(p-1))) continue;}
			  pstart=p;
			}
		 }

		if (pstart==NULL) break; // non ci sono + parentesi

		// -----------------------------------
		// Trovo la Parentesi che chiude     |
		// -----------------------------------
		if (_sLocal.bDebug) {win_infoarg("PARENTESI:\n");}
		pend=ParChiudi(pstart);
		if (pend==NULL) //ehExit("() e che cavolo! |%s|",pstart);
		{
			 if (_sLocal._extErrorNotify) _sLocal._extErrorNotify(2,"() Parentesi errate: %s",pstart);
			 return TRUE;
		}

		if (_sLocal.bDebug) win_infoarg(FormulaGlobale);
		*pend=0;
		if (_sLocal.bDebug) {win_infoarg(" extract [%s]\n",pstart+1); }

		// -------------------------------------------------
		// Converto la formula senza parantesi in un valore|
		// -------------------------------------------------

		strcpy(CodaOriginale,pend+1);
		if (SimpleFORMtoNumber(pstart+1,funcExt)) return ON;
		strcpy(pstart,pstart+1);
		strcat(FormulaGlobale,CodaOriginale);
		if (_sLocal.bDebug) {win_infoarg("Calc1 [%s] \n",FormulaGlobale); }
	}
	if (_sLocal.bDebug) {win_infoarg("ULTIMO PASS:\n");}

 //if (SimpleFORMtoAutoVar(FormulaGlobale,CodeProcedure)) return ON;

 // ------------------------------------------------------------|
 // FASE 2                                                      |
 // Finite le parentesi converte ci• che rimane                 |
 // Sostituisce le formule racchiuse in parentesi con variabili |
 // ------------------------------------------------------------|

	if (First_OperatorConverter(FormulaGlobale,funcExt)) {fRet=ON; goto FINE;}
	if (Second_OperatorConverter(FormulaGlobale,funcExt)) {fRet=ON; goto FINE;}
	if (BOOLEAN_OperatorConverter(FormulaGlobale,funcExt)) {fRet=ON; goto FINE;}
	
	if (_sLocal.bDebug) win_infoarg("Ultima conversione: [%s]",FormulaGlobale);
	*Valore=atof(FormulaGlobale);	

 // ------------------------------------------------------------|
 // FASE X                                                      |
 // Libero la memoria usata per l'elaborazione                  |
 // ------------------------------------------------------------|
FINE:
// DELVARautopush(); Non posso usarlo perchŠ addesso Š diventata ricorsiva
 memoFree(hdlFormula,"Form");
 memoFree(hdlCodaOriginale,"FCODA");

 //_sLocal.bDebug=OFF;
 if (_sLocal.bDebug) {win_infoarg("ESC FORMULA\n"); }

//  CODE.ALUIndex=ALUIndexStart;**
 return fRet;
}

static BOOL _isMathOperator(CHAR Byte)
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
static CHAR * Swappa(CHAR *Formula,double Valore,SINT PtStart,CHAR *PtEnd)
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

static BOOL ElementNumConverter(CHAR * lpElement,FORMULA_FUNC_EXT,double * Valore)
{
	SINT iTipo;
	*Valore=0;
	iTipo=_WhatIsThis(lpElement,funcExt,Valore);

	if (iTipo==E_VARNUM) return FALSE;
	if (iTipo==E_INTFUNC) return FALSE; // Il valore è già presente
	if (iTipo==E_NUMDEC) {*Valore=atof(lpElement); return FALSE;}
	_sLocal.iFormLastError=EE_VARUNKNOW; strcpy(_sLocal.szAddInfo,lpElement);
	return TRUE;
}

//
// _ElementAllocStringConverter()
//
static CHAR * _ElementAllocStringConverter(CHAR * lpElement,FORMULA_FUNC_EXT)
{
	SINT iTipo;
	double dValore;
	CHAR *lp;
	BOOL fCheck;

	dValore=0;
	iTipo=_WhatIsThis(lpElement,funcExt,&dValore);

	if (iTipo==E_CHARFIX) 
	{
		lp=ehAlloc(strlen(lpElement)+1);
		strcpy(lp,lpElement+1);
		lp[strlen(lpElement)-2]=0;
		return lp;
	}

	if (iTipo==E_VARCHAR)  // Variabile stringa (chiedo alla funzione esterna)
	{
		_sLocal.iFormLastError=EE_VARUNKNOW;

		// ------------------------------------------------------------------
		// 1) Leggo se esiste e la dimensione
		//
		fCheck=funcExt(WS_SIZE,lpElement,&dValore); if (fCheck) return NULL;

		// ------------------------------------------------------------------
		// 2) Alloco e richiedo la variabile
		//
		lp=ehAlloc((SINT) (dValore+1)); *lp=0;
		fCheck=funcExt(WS_REALGET,lpElement,lp);
		if (!fCheck) return lp; else ehFree(lp);
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
static SINT First_OperatorConverter(CHAR *pszString,BOOL (*funcExt)(SINT,SINT,CHAR*,void*))
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
 
 if (_sLocal.bDebug) win_infoarg(">First_OperatorConverter START=[%s]\n",pszString);
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
		if (_sLocal.bDebug) {win_infoarg(" ---> [%s] . [%s] . [%s]\n",E1,Opera,E2); }
		
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
			 
			 if (_sLocal.bDebug) win_infoarg(" Risultato Prima [%s] PtStart[%s]\n",pszString,pszString+PTstart);
		  	 p=Swappa(pszString,Risultato,PTstart,PTend);
			 if (_sLocal.bDebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
			 *Operatore=0;*E2=0;
			 fPrimo=FALSE;
		} 

	 if (*p==0) break; // Finito

	 strcpy(E1,E2); PTstart=NewStart;
	 strcpy(Opera,Operatore);
	 }
 if (_sLocal.bDebug) win_infoarg("<First_OperatorConverter END=[%s]\n",pszString);

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
 
 if (_sLocal.bDebug) win_infoarg("I) > OperatorConverter START=[%s]\n",pszString);
 p=pszString;
 //Segue=OFF;
 PTstart=0; PTend=NULL;

 *Opera=0; *E1=0; lpE1=NULL;
 for(;;)
	 {
		p2=p;
		p=GiveMeElement(p,E2,Operatore);
		if (_sLocal.bDebug) {win_infoarg(" ---> [%s] . [%s] . [%s]\n",E1,Opera,E2);}
        
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
			 

			 if (_sLocal.bDebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
		  	 p=Swappa(pszString,Risultato,PTstart,PTend);
			 if (_sLocal.bDebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
			 *Operatore=0;*E2=0;
			 fPrimo=FALSE;
		} 

	 if (*p==0) break; // Finito
	 strcpy(E1,E2);  lpE1=lpE2;
	 strcpy(Opera,Operatore);
	 }

 if (_sLocal.bDebug) win_infoarg("I) < OperatorConverter END=[%s]\n",pszString);
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
 
 if (_sLocal.bDebug) win_infoarg("II) > OperatorConverter START=[%s]\n",pszString);
 p=pszString;
 //Segue=OFF;
 PTstart=0; PTend=NULL;

 *Opera=0; *E1=0; lpE1=NULL;
 while(TRUE)
	 {
		p2=p;
		p=GiveMeElement(p,E2,Operatore);
		if (_sLocal.bDebug) {win_infoarg(" ---> [%s] . [%s] . [%s]\n",E1,Opera,E2);}
        
		lpE2=p2; // Ultimo Puntatore a E2

		// Controllo stringhe
		if (_isString(E1)&&_isString(E2))
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
				CHAR * lpString1;
				CHAR * lpString2;

				lpString1=_ElementAllocStringConverter(E1,funcExt); if (!lpString1) return ON;
				lpString2=_ElementAllocStringConverter(E2,funcExt); if (!lpString2) {ehFree(lpString1); return ON;}
				PTstart=(LONG) (lpE1)-(LONG) (pszString);
				PTend=p2+strlen(E2);


				dRisultato=0;
				// Uguaglianza
				if (!strcmp(Opera,"=")||!strcmp(Opera,"==")) {dRisultato=strcmp(lpString1,lpString2)?0:1;}
				// Maggiore 
				else if (!strcmp(Opera,">")) dRisultato=strcmp(lpString1,lpString2)>0?1:0;
				// Minore
				else if (!strcmp(Opera,"<")) dRisultato=strcmp(lpString1,lpString2)<0?1:0;
				// Maggiore uguale
				else if (!strcmp(Opera,">=")||!strcmp(Opera,"=>")) dRisultato=strcmp(lpString1,lpString2)>=0?1:0;
				// Minore uguale
				else if (!strcmp(Opera,"<=")||!strcmp(Opera,"=<")) dRisultato=strcmp(lpString1,lpString2)<=0?1:0;
				// Diverso
				else if (!strcmp(Opera,"<>")||!strcmp(Opera,"!=")) dRisultato=strcmp(lpString1,lpString2);
				else ehError();
				/*
				printf("Stringa [%s]%s[%s]%d",
					   lpString1,Opera,lpString2,
					   (SINT) dRisultato);	 
				getch();
				*/
				ehFree(lpString1);
				ehFree(lpString2);

			 //if (_sLocal.bDebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
		  	 p=Swappa(pszString,dRisultato,PTstart,PTend);
			 //if (_sLocal.bDebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
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

			 //if (_sLocal.bDebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
		  	 p=Swappa(pszString,dRisultato,PTstart,PTend);
			 //if (_sLocal.bDebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
			 *Operatore=0;*E2=0;
			 fPrimo=FALSE;
		 }
		}

	 if (*p==0) break; // Finito
	 strcpy(E1,E2);  lpE1=lpE2;
	 strcpy(Opera,Operatore);
	 }

 if (_sLocal.bDebug) win_infoarg("II) < OperatorConverter END=[%s]\n",pszString);
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
static SINT Second_OperatorConverter(CHAR *pszString,BOOL (*funcExt)(SINT,SINT,CHAR*,void*))
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

 if (_sLocal.bDebug) {win_infoarg("> Second_OperatorConverter START=[%s]\n",pszString); }

 p=pszString;
 strcpy(Opera,"");
 while (TRUE)
	{//LONG andress;
//	 WORD lpVar;

	 p=GiveMeElement(p,E1,Operatore);
//	 if (_sLocal.bDebug) {win_infoarg("LOWEstratto [%s][%s]  \n",E1,Operatore); }

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
		if (_sLocal.bDebug) {win_infoarg("Calcolo [%.2f] %s [%.2f] \n",Risultato,Opera,Valore); }
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
		if (_sLocal.bDebug) {win_infoarg("Opera ? [%s]",Opera); }
		win_infoarg("Operatore sconosciuto: %s",Opera);
		return ON;
	 }

	 AVANTI:
	 if (*p==0) break; // Finito
	 strcpy(Opera,Operatore);
	}

 sprintf(pszString,"%.3f",Risultato);
 if (_sLocal.bDebug) {win_infoarg("<Second_OperatorConverter:  [%s]",pszString); }

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
static SINT BOOLEAN_OperatorConverter(CHAR *pszString,FORMULA_FUNC_EXT)
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

 if (_sLocal.bDebug) {win_infoarg(">BOOLEAN_OperatorConverter START=[%s]\n",pszString); }

 p=pszString;
 strcpy(Opera,"");
 while (TRUE)
	{//LONG andress;
//	 WORD lpVar;

	 p=GiveMeElement(p,E1,Operatore);
//	 if (_sLocal.bDebug) {win_infoarg("LOWEstratto [%s][%s]  \n",E1,Operatore); }

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
		if (_sLocal.bDebug) {win_infoarg("Calcolo [%.2f] %s [%.2f] \n",Risultato,Opera,Valore); }
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
		if (_sLocal.bDebug) {win_infoarg("Opera ? [%s]",Opera); }
		
		if (_sLocal._extErrorNotify) _sLocal._extErrorNotify(3,"Operatore sconosciuto: %s",Opera);
		return TRUE;
	 }

	 AVANTI:
	 if (*p==0) break; // Finito
	 strcpy(Opera,Operatore);
	}

 sprintf(pszString,"%.3f",Risultato);
 if (_sLocal.bDebug) {win_infoarg("<BOOLEAN_OperatorConverter: \n %s %.3f",pszString,Risultato); }

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
	SINT iTipo;
	double Valore=0;
	if (_sLocal.bDebug) win_infoarg(">> SimpleFORMtoNumber START=|%s| \n",Formula);

	// -------------------------------------------
	// Se la formula Š un numero costante        !
	// non la trasformo in auto variabile        !
	// -------------------------------------------

	// La formula è un unica variabile Numerica
	iTipo=_WhatIsThis(Formula,funcExt,&Valore);

 if (iTipo==E_INTFUNC)
		{
		 sprintf(Formula,"%.3f",Valore);
		 goto FINE;
		 }

 if (iTipo==E_VARNUM)
		{//sprintf(Nome,"%u",CONSTvalore(Formula));
	     if (funcExt) funcExt(WS_REALGET,Formula,&Valore);
		 sprintf(Formula,"%.3f",Valore);
		 goto FINE;
		 }

 // -------------------------------------------
 // IDEM se Š una variabile/costante          !
 // -------------------------------------------
 // E' un unico NUMERO o una VARIABILE o una COSTANTE
 if ((iTipo==E_NUMDEC)||
	 (iTipo==E_NUMHEX)||
	 (iTipo==E_NUMBIN)) goto FINE;

 // -------------------------------------------
 // Controllo sulle Stringhe                  !
 // -------------------------------------------
 //if ((iTipo==I_CHARFIX)||(iTipo==I_CHARCONST)||(iTipo==I_CHAR)) goto FINE;

 // -------------------------------------------
 // Converte gli operatori di priorit… ALTA   !
 // in AutoVariabili                          !
 // -------------------------------------------

 if (First_OperatorConverter(Formula,funcExt)) return ON;
 iTipo=_WhatIsThis(Formula,funcExt,NULL);
 if ((iTipo==E_NUMDEC)||(iTipo==E_NUMHEX)||(iTipo==E_NUMBIN)) goto FINE;

 if (Second_OperatorConverter(Formula,funcExt)) return ON;
 if (BOOLEAN_OperatorConverter(Formula,funcExt)) return ON;

 // Mi ritorna una stringa
 //if (strstr(Formula,"ACHR")) goto FINE;

 //VARautopush(Nome); // Richiede una variabile
 //	 VARStackPush(RE_HL,Nome);
 if (_sLocal.bDebug) {win_infoarg("AUTOPUSH[%s]\n",Formula); }
	 //ASMVpush("HL>",Nome);
	 //VARStackPush(RE_HL,Nome);

//	 if (_sLocal.bDebug) {win_infoarg("Offset[%ld]\n",off); }
	 //VARSetUsata(Nome,CodeProcedure);
 //strcpy(Formula,Nome);

 FINE:
 if (_sLocal.bDebug) {win_infoarg("<< SimpleFORMtoNumber END=|%s| \n",Formula); }
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
		 if (_isDecNumero(okl)) return E_NUMDEC;
		 if (_isHexNumero(okl)) return E_NUMHEX;
		 if (_isBinNumero(okl)) return E_NUMBIN;
		 if (_isStringFix(okl)) return E_CHARFIX; // ... per intenderci "cazzo" ;-)
		 // Potrebbe essere una formula
		 if ((*okl>='0')&&(*okl<='9')) return E_FORMULANUM;
		 if (*okl=='(') return E_FORMULANUM;
		 return E_ERROR;
	}

 if (_isStringVar(okl)) return E_VARCHAR; // ... per intenderci VALORE$

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
				CHAR *lpNewFormula=ehAlloc(strlen(okl)+1);
				BOOL fRet;
				*lpClose=0; strcpy(lpNewFormula,okl+4); *lpClose=')';
				
				//
				// Eseguo la formula
				//
				fRet=L_FormulaProcess(lpNewFormula,&Valore,funcExt);
				// printf("E' [%s]",lpNewFormula); getch();
				ehFree(lpNewFormula);
				
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
	 if (lpRetValore) fCheck=funcExt(WS_REALGET,okl,&Valore);
					  else
					  fCheck=funcExt(WS_REALGET,okl,NULL);
 }
 
 if (fCheck) return E_VARUNKNOW;

 if (lpRetValore) *lpRetValore=Valore;
 return E_VARNUM;
//return 0;
}

static SINT _isDecNumero(CHAR *pszString)
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


static SINT _isHexNumero(CHAR *pszString)
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

static SINT _isBinNumero(CHAR *pszString)
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

static SINT _isStringFix(CHAR *dato)
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

static SINT _isStringVar(CHAR *dato)
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

static BOOL _isString(CHAR *lpCosa)
{
 if (_isStringFix(lpCosa)||_isStringVar(lpCosa)) return TRUE; else return FALSE;
}

//
// getFuncArg() - Parser Function
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