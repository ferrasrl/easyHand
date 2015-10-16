//-----------------------------------------------------------------------
//  scriptFor
//  Intepretazione di script e formule
//  
//	formulaProcess  Trasforma la formula in un numero                   
//											Creato da G.Tassistro       
//											Ferrà Art & Technology 1999  
//											Ferrà Srl 2013
//
//
//
//  Ritorna TRUE = pszFormula errata                                       
//          FALSE= pszFormula OK Valore messo in "Valore"                  
//-----------------------------------------------------------------------
#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/scriptFor.h"

#define EE_OK 0
#define EE_VARUNKNOW 1

typedef enum {

	MODE_STD,
	MODE_ENDASSIGN,
	MODE_FORMULA

} EN_TOKEN_MODE;

static S_SCRIPT_TAG _arsBasic[]={

	// ------------------------------------------------------------------------------------------------
	// Globali
	// ------------------------------------------------------------------------------------------------
	{TG_GLOBAL|TE_STANTMENT, 1000,"VAR"},
	{TG_GLOBAL|TE_STANTMENT, 1001,"IF"},
	{TG_GLOBAL|TE_STANTMENT, 1002,"//"},
	{TG_GLOBAL|TE_STANTMENT, 1003,"BREAK"},
	{TG_GLOBAL|TE_STANTMENT, 1004,"RETURN"},
	{TG_GLOBAL|TE_STANTMENT, 1005,"PRINT"},
	{TG_GLOBAL|TE_STANTMENT, 1006,"ERROR.STOP"},

	// ELaborazione di stringhe
	{TG_GLOBAL|TE_FUNCTION, 1101,"LEFT"},
	{TG_GLOBAL|TE_FUNCTION, 1102,"RIGHT"},
	{TG_GLOBAL|TE_FUNCTION, 1103,"SUBSTRING"},
	{TG_GLOBAL|TE_FUNCTION, 1104,"TRIM"},
	{TG_GLOBAL|TE_FUNCTION, 1105,"OMIT"},
	{TG_GLOBAL|TE_FUNCTION, 1106,"KEEP"},

	{TG_GLOBAL|TE_FUNCTION, 1107,"ATOI"},
	{TG_GLOBAL|TE_FUNCTION, 1108,"ATOF"},

	{TG_GLOBAL|TE_FUNCTION, 1110,"JSON.GET"},
	{0,0,NULL},
};



//
// Funzione che notifica l'errore all'esterno
//
static void (*extErrorNotify)(INT iMode,CHAR *pMess,...)=NULL;

static CHAR *szFormError[]={"pszFormula interpretata correttamente.",
							"Variabile sconosciuta",
							"Errore in formula"};

//static INT	iFormLastError=0;
// static CHAR szAddInfo[80];
static INT			_SimpleFORMtoNumber(S_SCRIPT * psScript,CHAR * pszFormula);
static INT			_firstOperatorConverter(S_SCRIPT * psScript,CHAR * pszString); // Primi operatori da convertire : /*<<>>
static INT			_secondOperatorConverter(S_SCRIPT * psScript,CHAR * pszString); // Secondi operatori +-&^
static INT			_BooleanOperatorConverter(S_SCRIPT * psScript,CHAR * pszString);
static S_UNIVAL *	_formulaProcess(S_SCRIPT * psScript,CHAR * pszFormula);
static BOOL IsMathOperator(CHAR Byte);
static CHAR *		_ParChiudi(CHAR *p);
static S_UNIVAL *	_WhatIsThis(S_SCRIPT * psScript,CHAR * pszElement,EN_TAG_TYPE * penType,BOOL bGetValue);//FORMULA_FUNC_EXT,void*Valore);
static CHAR *		_parClose(CHAR *p);
static BOOL			_tagEngine(S_SCRIPT * psScript,
					  // S_SCRIPT_TAG * psTag,
					   INT iCode,
					   CHAR * pszToken,
					   CHAR * pszChar,
					   CHAR * pszBuffer);
static CHAR *		_getScript(S_SCRIPT * psScript);
//static BOOL _private.bFormulaDebug=FALSE;

//
// String Function 
//
static S_UNIVAL * _leftString(S_SCRIPT * psScript,CHAR * pszToken,CHAR * pszParams);
static S_UNIVAL * _rightString(S_SCRIPT * psScript,CHAR * pszToken,CHAR * pszParams);
static S_UNIVAL * _stringToNum(S_SCRIPT * psScript,CHAR * pszToken,BOOL bInteger);

//
// Script
//
static void		_charStop(CHAR *pszBuffer,CHAR ch);
#define _ERROR_SIGN_ {printf("---ERROR---\n");}
//static BOOL		_formulaExtCall(EN_MESSAGE enMess,CHAR * pszElement, S_FORMULA * psFor);
static void		_booleanSwapping(CHAR *lpFormula);
static BOOL		_extVarAssign(INT iCode,S_UNIVAL * psRet,S_SCRIPT * psScript);
//static CHAR * _ElementAllocStringConverter(CHAR *lpElement,FORMULA_FUNC_EXT);
static S_UNIVAL * _ElementAllocStringConverter(S_SCRIPT * psScript, CHAR * lpElement);//FORMULA_FUNC_EXT)
static S_UNIVAL * _ElementNumConverter(S_SCRIPT * psScript,CHAR * pszElement);
static void	*	_varManager(S_SCRIPT * psScript,EN_MESSAGE enMess,CHAR * pszName,S_UNIVAL *psVal);
static CHAR *	_token(S_SCRIPT * psScript,CHAR * lpChars,CHAR * lpLastSep,EN_TOKEN_MODE iMode);
static BOOL		_varNameCheck(CHAR *lpNome);
static BOOL		_scPrint(CHAR *lpFormulaSource,S_SCRIPT * psScript);


//
// scriptError()
//
void * scriptError(S_SCRIPT * psScript,EN_SER enErr,CHAR * pszFormat,...) {

	CHAR * psz;
	strFromArgs(pszFormat,psz);
	psScript->enError=enErr;
	lstPush(psScript->lstError,psz);
	ehFree(psz);
	return NULL;

}

//
// scriptShowErrors()
//
void scriptShowErrors(S_SCRIPT * psScript) {
	
	CHAR * psz;	
	printf(CRLF);
	for (lstLoop(psScript->lstError,psz)) {
		printf("- %s " CRLF,psz);
	}

}



static struct {

	BOOL	bFormulaDebug;
	BOOL	bVerbose;
	BOOL	bVerboseExt;
//	INT		iCurrent;	 // linea corrente	
	_DMI	dmiVar;

} _private={false,true,false,0};


void FormulaSetNotify(void (*NewFunction)(INT iMode,CHAR *pMess,...))
{
	extErrorNotify=NewFunction;
}

void FormulaDebug(INT flag)
{
	_private.bFormulaDebug=flag;
}

static INT IsDecNumero(CHAR *);
static INT IsHexNumero(CHAR *);
static INT IsBinNumero(CHAR *);
static INT _isStringFix(CHAR *);
static INT IsStringVar(CHAR *);
static BOOL _isString(CHAR *);
//static BOOL _isStringSum(S_SCRIPT * psScript,CHAR *);
static S_UNIVAL * _stringSum(S_SCRIPT * psScript,CHAR * pszSum,BOOL bOnlyVerify);


//
// scriptGetValue()
//

S_UNIVAL * scriptGetValue(S_SCRIPT * psScript,CHAR * pszToken)
{

	BOOL bRet=false;
	CHAR *lpNewFormula=ehAlloc(2048);
	BYTE *ps,*pd=lpNewFormula;
	EN_TAG_TYPE enType;
	S_UNIVAL * psRet;

	//psFor->dValue=0;
	// Tolgo i caratteri di controllo (CR-LF ecc)
	for (ps=pszToken;*ps;ps++)
	{
		 if (*ps<33) continue;
		 *pd=*ps; pd++;
	}
    *pd=0;

// Se è una singola variabile la elaboro subito
	psRet=_WhatIsThis(psScript,pszToken,&enType,true);//funcExt,NULL);
//	printf("> [%s]=%d" CRLF,pszFormula,enType);
	if (enType==E_FORMULANUM||enType==E_VARUNKNOW) {
		psRet=_formulaProcess(psScript,pszToken);
	}
	else if (enType==E_TEXT_SUM) {
		psRet=_stringSum(psScript,pszToken,false);
	}

	ehFree(lpNewFormula);
	return psRet; // false = OK
}

//
// Questa invece è ad uso interno ed è RICORSIVA
//
static S_UNIVAL * _formulaProcess(S_SCRIPT * psScript,CHAR * pszFormula)//double *Valore,FORMULA_FUNC_EXT)
{
	// Lunghezza massima della formula
	const INT MaxFormula=2048;

//	INT  hdlFormula,hdlCodaOriginale;
	CHAR *	pszTotale;
	CHAR *	CodaOriginale;
	BOOL fRet=FALSE;
	S_UNIVAL * psRet=NULL;

	if (_private.bFormulaDebug) {win_infoarg("FORMULA |%s|\n",pszFormula); }

	// ------------------------------------------------------------|
	// FASE 0                                                      |
	// Alloco memoria per l'elaborazione                           |
	// ------------------------------------------------------------|

	if (strlen(pszFormula)>(WORD) (MaxFormula-1))
	{
		if (extErrorNotify) (*extErrorNotify)(1,"pszFormula troppo lunga max(%d)",MaxFormula-1);
		return NULL;
	}

//	hdlFormula=memoAlloc(M_HEAP,MaxFormula,"FORM");
	//pszTotale=memoPtr(hdlFormula,NULL);
	pszTotale=ehAlloc(MaxFormula);

	//hdlCodaOriginale=memoAlloc(M_HEAP,MaxFormula,"FCODA");
	//CodaOriginale=memoPtr(hdlCodaOriginale,NULL);
	CodaOriginale=ehAlloc(MaxFormula);

	strcpy(pszTotale,pszFormula);
	while (true)
	{
		// -------------------------------------------
		// Trovo la Parentesi + profonda che apre    |
		// -------------------------------------------
		BOOL fString=FALSE;
		CHAR *p,*pstart,*pend;

		pstart=NULL;
		for (p=pszTotale;*p;p++)
		 {
			if (*p=='\"') {fString^=1; continue;}

			if (*p=='('&&!fString)
			{if (p>pszTotale) {if (!IsMathOperator(*(p-1))) continue;}
			  pstart=p;
			}
		 }

		if (pstart==NULL) break; // non ci sono + parentesi

		// -----------------------------------
		// Trovo la Parentesi che chiude     |
		// -----------------------------------
		if (_private.bFormulaDebug) {win_infoarg("PARENTESI:\n");}
		pend=_ParChiudi(pstart);
		if (pend==NULL) //ehExit("() e che cavolo! |%s|",pstart);
		{
			 if (extErrorNotify) (*extErrorNotify)(2,"() Parentesi errate: %s",pstart);
			 return NULL;
		}

		if (_private.bFormulaDebug) win_infoarg(pszTotale);
		*pend=0;
		if (_private.bFormulaDebug) {win_infoarg(" extract [%s]\n",pstart+1); }

		// -------------------------------------------------
		// Converto la formula senza parantesi in un valore 
		// -------------------------------------------------

		strcpy(CodaOriginale,pend+1);
		if (_SimpleFORMtoNumber(psScript,pstart+1)) return NULL;
		strcpy(pstart,pstart+1);
		strcat(pszTotale,CodaOriginale);
		if (_private.bFormulaDebug) {win_infoarg("Calc1 [%s] \n",pszTotale); }
	}
	if (_private.bFormulaDebug) {win_infoarg("ULTIMO PASS:\n");}

 //if (SimpleFORMtoAutoVar(pszTotale,CodeProcedure)) return ON;

 // ------------------------------------------------------------|
 // FASE 2                                                      |
 // Finite le parentesi converte ci• che rimane                 |
 // Sostituisce le formule racchiuse in parentesi con variabili |
 // ------------------------------------------------------------|

	if (_firstOperatorConverter(psScript,pszTotale)) {fRet=ON; goto FINE;}
	if (_secondOperatorConverter(psScript,pszTotale)) {fRet=ON; goto FINE;}
	if (_BooleanOperatorConverter(psScript,pszTotale)) {fRet=ON; goto FINE;}
	
	if (_private.bFormulaDebug) win_infoarg("Ultima conversione: [%s]",pszTotale);
//	if (psFor->sEx.psRet) valDestroy(psFor->sEx.psRet);
//	psFor->sEx.psRet=valCreate(_NUMBER,0,pszTotale);
	psRet=valCreate(_NUMBER,0,pszTotale);
//	*pdRet=atof(pszTotale);	
	//*Valore=atof(pszTotale);	

 // ------------------------------------------------------------|
 // FASE X                                                      |
 // Libero la memoria usata per l'elaborazione                  |
 // ------------------------------------------------------------|
FINE:
	// DELVARautopush(); Non posso usarlo perchŠ addesso Š diventata ricorsiva
	//memoFree(hdlFormula,"Form");
	//memoFree(hdlCodaOriginale,"FCODA");
	ehFree(pszTotale);
	ehFree(CodaOriginale);

	//_private.bFormulaDebug=OFF;
	if (_private.bFormulaDebug) {win_infoarg("ESC FORMULA\n"); }

	//  CODE.ALUIndex=ALUIndexStart;**
	return psRet;
}

static BOOL IsMathOperator(CHAR Byte)
{
 CHAR Find[2];
 Find[0]=Byte;
 Find[1]=0;

 if (strstr("=+-/*><|&^(",Find)) return ON; else return OFF;
}


// Ritorna Null se c'è un errore
static CHAR * _ParChiudi(CHAR *p)
{
 INT num=0;
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

static CHAR *GiveMeElement(CHAR *pszFormula,
						   CHAR *Elemento, // CE1
						   CHAR *Operatore) // COp
{
 CHAR *p;
 INT SWI=0;
 INT np=0;
 CHAR *E1=Elemento,*Op=Operatore;
 INT Eopera=OFF;
 BOOL String=0;

// printf(">GME START=[%s]\n",pszFormula); 
 for (p=pszFormula;*p;p++)
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

static CHAR *Swappa(CHAR *pszFormula,double Valore,INT PtStart,CHAR *PtEnd)
{
	CHAR Calcolo2[512];
	CHAR *p;

	if (PtStart!=0) memcpy(Calcolo2,pszFormula,PtStart);
	Calcolo2[PtStart]=0;
	sprintf(Calcolo2+PtStart,"%.3f",Valore);
	strcat(Calcolo2,PtEnd);//p2+strlen(E2));
	strcpy(pszFormula,Calcolo2);

	p=pszFormula+PtStart;
	return p;
}
// -------------------------------------------------------------|
// _ElementNumConverter                                          |
// Converte un elemento in un numero                            |
// L'elemento poò essere:                                       |
// - Numero decimale                                            |
// - Variabile                                                  |
// - Funzione (DA FARE)                                         |
//                                                              |
//                                                              |
// -------------------------------------------------------------|

static S_UNIVAL * _ElementNumConverter(S_SCRIPT * psScript,CHAR * pszElement) 
{
	EN_TAG_TYPE enType;
	S_UNIVAL * psRet;

	psRet=_WhatIsThis(psScript,pszElement,&enType,true);//funcExt,Valore);

	//if (enType==E_VARNUM) return psRet;
	//else if (enType==E_INTFUNC) return psRet; // Il valore è già presente
	if (!psRet&&enType==E_NUMDEC) {
		
		psRet=valCreate(_NUMBER,0,pszElement);
	}
	if (!psRet) psScript->enError=SER_VAR_UNKNOW;
	return psRet;
}


//
// _ElementAllocStringConverter()
//
static S_UNIVAL * _ElementAllocStringConverter(S_SCRIPT * psScript,CHAR * pszElement)//FORMULA_FUNC_EXT)
{
	EN_TAG_TYPE enType;
	S_UNIVAL * psRet;

	psRet=_WhatIsThis(psScript,pszElement,&enType,true);//funcExt,Valore);
	/*
	if (enType==E_TEXT&&!psRet) 
	{
		psRet=valCreate(_TEXT,0,pszElement);
	}
*/
	return psRet; // Non trovato
}

//
//
//
static INT _firstOperatorConverter(S_SCRIPT * psScript,CHAR * pszString)
{
	CHAR *p,*p2;
	CHAR E1[200];
	CHAR E2[200];
	CHAR Operatore[5];
	CHAR szOperator[5];
	//WORD Ritorno;
	INT PTstart;
	CHAR *PTend;
	double Numero1,Numero2;
	double Risultato;
	BOOL fPrimo=FALSE;
	CHAR *lpE1,*lpE2;
	S_UNIVAL * psRet;

	if (_private.bFormulaDebug) win_infoarg("I) > OperatorConverter START=[%s]\n",pszString);
	p=pszString;
	//Segue=OFF;
	PTstart=0; PTend=NULL;

	*szOperator=0; *E1=0; lpE1=NULL;
	for(;;)
	 {
		p2=p;
		p=GiveMeElement(p,E2,Operatore);
		if (_private.bFormulaDebug) {win_infoarg(" ---> [%s] . [%s] . [%s]\n",E1,szOperator,E2);}
	    
		lpE2=p2; // Ultimo Puntatore a E2
		
		if ((!strcmp(szOperator,"*"))||
			(!strcmp(szOperator,">>"))||
			(!strcmp(szOperator,"<<"))||
			(!strcmp(szOperator,"/")))
			{

				 psRet=_ElementNumConverter(psScript,E1); if (!psRet) return ON;
				 Numero1=psRet->dValue; valDestroy(psRet);
				 psRet=_ElementNumConverter(psScript,E2);if (!psRet) return ON;
				 Numero2=psRet->dValue; valDestroy(psRet);
				 
				 PTstart=(LONG) (lpE1)-(LONG) (pszString);
				 PTend=p2+strlen(E2);

			 //  MOLTIPLICAZIONE                  !
			 if (!strcmp(szOperator,"*")) Risultato=Numero1*Numero2;
			 //  DIVISIONE                        !
			 if (!strcmp(szOperator,"/")) Risultato=Numero1/Numero2;
			 //  Shift a Destra                  !
			 if (!strcmp(szOperator,">>")) Risultato=(LONG) Numero1>>(LONG) Numero2;
			 //  Shift a Sinistra                !
			 if (!strcmp(szOperator,"<<")) Risultato=(LONG) Numero1<<(LONG) Numero2;
			 

			 if (_private.bFormulaDebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
	  		 p=Swappa(pszString,Risultato,PTstart,PTend);
			 if (_private.bFormulaDebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
			 *Operatore=0;*E2=0;
			 fPrimo=FALSE;
		} 

	 if (*p==0) break; // Finito
	 strcpy(E1,E2);  lpE1=lpE2;
	 strcpy(szOperator,Operatore);
	 }

	if (_private.bFormulaDebug) win_infoarg("I) < OperatorConverter END=[%s]\n",pszString);
	return OFF;
}

//
// _secondOperatorConverter()
// 
static INT _secondOperatorConverter(S_SCRIPT * psScript, CHAR *pszString)
{
	 CHAR *p,*p2;
	 CHAR E1[200];
	 CHAR E2[200];
	 CHAR Operatore[5];
	 CHAR szOperator[5];
	 //WORD Ritorno;
	 INT PTstart;
	 CHAR *PTend;
	 double Numero1,Numero2;
	 double dRisultato;
	 BOOL fPrimo=FALSE;
	 CHAR *lpE1,*lpE2;
	 S_UNIVAL * psRet;
	 
	 if (_private.bFormulaDebug) win_infoarg("II) > OperatorConverter START=[%s]\n",pszString);
	 p=pszString;
	 //Segue=OFF;
	 PTstart=0; PTend=NULL;

	 *szOperator=0; *E1=0; lpE1=NULL;
	 while(TRUE)
		 {
			p2=p;
			p=GiveMeElement(p,E2,Operatore);
			if (_private.bFormulaDebug) {win_infoarg(" ---> [%s] . [%s] . [%s]\n",E1,szOperator,E2);}
	        
			lpE2=p2; // Ultimo Puntatore a E2

			// Controllo stringhe
			if (_isString(E1)&&_isString(E2))
			{
			if (
				(!strcmp(szOperator,"="))||
				(!strcmp(szOperator,"<"))||
				(!strcmp(szOperator,">"))||
				(!strcmp(szOperator,"<="))||
				(!strcmp(szOperator,"=<"))||
				(!strcmp(szOperator,">="))||
				(!strcmp(szOperator,"=>"))||
				(!strcmp(szOperator,"<>"))||
				(!strcmp(szOperator,"!="))||
				(!strcmp(szOperator,"=="))
				)
				{
					CHAR *lpString1;
					CHAR *lpString2;

					psRet=_ElementAllocStringConverter(psScript,E1); if (!psRet) return ON;
					lpString1=strDup(psRet->pszString); valDestroy(psRet);
					psRet=_ElementAllocStringConverter(psScript,E2); if (!psRet) {ehFree(lpString1); return ON;}
					lpString2=strDup(psRet->pszString); valDestroy(psRet);
					PTstart=(LONG) (lpE1)-(LONG) (pszString);
					PTend=p2+strlen(E2);

					dRisultato=0;
					// Uguaglianza
					if (!strcmp(szOperator,"=")||!strcmp(szOperator,"==")) {dRisultato=strcmp(lpString1,lpString2)?0:1;}
					// Maggiore 
					if (!strcmp(szOperator,">")) dRisultato=strcmp(lpString1,lpString2)>0?1:0;
					// Minore
					if (!strcmp(szOperator,"<")) dRisultato=strcmp(lpString1,lpString2)<0?1:0;
					// Maggiore uguale
					if (!strcmp(szOperator,">=")||!strcmp(szOperator,"=>")) dRisultato=strcmp(lpString1,lpString2)>=0?1:0;
					// Minore uguale
					if (!strcmp(szOperator,"<=")||!strcmp(szOperator,"=<")) dRisultato=strcmp(lpString1,lpString2)<=0?1:0;
					// Diverso
					if (!strcmp(szOperator,"<>")||!strcmp(szOperator,"!=")) dRisultato=strcmp(lpString1,lpString2);
					/*
					printf("Stringa [%s]%s[%s]%d",
						   lpString1,szOperator,lpString2,
						   (INT) dRisultato);	 
					getch();
					*/
					ehFree(lpString1);
					ehFree(lpString2);

				 //if (_private.bFormulaDebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
		  		 p=Swappa(pszString,dRisultato,PTstart,PTend);
				 //if (_private.bFormulaDebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
				 *Operatore=0;*E2=0;
				 fPrimo=FALSE;
				}
			}
			else
			{
			 if ((!strcmp(szOperator,"+"))||
				(!strcmp(szOperator,"-"))||

				(!strcmp(szOperator,"&"))||
				(!strcmp(szOperator,"|"))||
				(!strcmp(szOperator,"^"))||

				(!strcmp(szOperator,"="))||
				(!strcmp(szOperator,"<"))||
				(!strcmp(szOperator,">"))||
				(!strcmp(szOperator,"<="))||
				(!strcmp(szOperator,"=<"))||
				(!strcmp(szOperator,">="))||
				(!strcmp(szOperator,"=>"))||
				(!strcmp(szOperator,"<>"))||
				(!strcmp(szOperator,"!="))||
				(!strcmp(szOperator,"=="))
				)
				{

				 psRet=_ElementNumConverter(psScript,E1); 
				 if (!psRet) {
					 scriptError(psScript,SER_OBJ_UNKNOW,"'%s' unknown",E1);
					 return true;
				 }
				 Numero1=psRet->dValue; valDestroy(psRet);
				 psRet=_ElementNumConverter(psScript,E2); 
				 if (!psRet) 
					 return true;
				 Numero2=psRet->dValue; valDestroy(psRet);
				 
				 PTstart=(LONG) (lpE1)-(LONG) (pszString);
				 PTend=p2+strlen(E2);
				 
				 // Somma
				 if (!strcmp(szOperator,"+")) dRisultato=Numero1+Numero2;
				 // Sottrazione
				 if (!strcmp(szOperator,"-")) dRisultato=Numero1-Numero2;

				 // And
				 if (!strcmp(szOperator,"&")) dRisultato=((INT) (Numero1) & (INT) (Numero2));
				 // Or
				 if (!strcmp(szOperator,"|")) dRisultato=((INT) (Numero1)|(INT) (Numero2));
				 // Xor
				 if (!strcmp(szOperator,"^")) dRisultato=((INT) (Numero1)|(INT) (Numero2));

				 // Uguaglianza
				 if (!strcmp(szOperator,"=")||!strcmp(szOperator,"==")) dRisultato=(Numero1==Numero2);
				 // Maggiore 
				 if (!strcmp(szOperator,">")) dRisultato=(Numero1>Numero2);
				 // Minore
				 if (!strcmp(szOperator,"<")) dRisultato=(Numero1<Numero2);
				 // Maggiore uguale
				 if (!strcmp(szOperator,">=")||!strcmp(szOperator,"=>")) dRisultato=(Numero1>=Numero2);
				 // Minore uguale
				 if (!strcmp(szOperator,"<=")||!strcmp(szOperator,"=<")) dRisultato=(Numero1<=Numero2);
				 // Diverso
				 if (!strcmp(szOperator,"<>")||!strcmp(szOperator,"!=")) dRisultato=(Numero1!=Numero2);

				 //if (_private.bFormulaDebug) win_infoarg(" Swapping: Prima [%s] PtStart[%s]-[%s]\n",pszString,pszString+PTstart,PTend);
		  		 p=Swappa(pszString,dRisultato,PTstart,PTend);
				 //if (_private.bFormulaDebug) win_infoarg(" Risultato Dopo > [%s] [%s]\n",pszString,p);
				 *Operatore=0;*E2=0;
				 fPrimo=FALSE;
			 }
			}

		 if (*p==0) break; // Finito
		 strcpy(E1,E2);  lpE1=lpE2;
		 strcpy(szOperator,Operatore);
		 }

	 if (_private.bFormulaDebug) win_infoarg("II) < OperatorConverter END=[%s]\n",pszString);
	 return false;
}

//-----------------------------------------------=
//  _BooleanOperatorConverter
//  Calcola la formula con soli operatori di     !
//  bassa priorit… in HL                         !
//                                               !
//  Risultato in HL                              !
// -----------------------------------------------
static INT _BooleanOperatorConverter(S_SCRIPT * psScript,CHAR * pszString)
{
	CHAR *p;
	CHAR E1[200];
	// CHAR E2[200];
	CHAR Operatore[5];
	CHAR szOperator[5];
	// WORD Ritorno;
	double Risultato=0;
	S_UNIVAL * psRet;
	double dValore;

	if (_private.bFormulaDebug) {win_infoarg(">_BooleanOperatorConverter START=[%s]\n",pszString); }

	p=pszString;
	strcpy(szOperator,"");
	while (TRUE)
	{//LONG andress;
	//	 WORD lpVar;

	 p=GiveMeElement(p,E1,Operatore);
	//	 if (_private.bFormulaDebug) {win_infoarg("LOWEstratto [%s][%s]  \n",E1,Operatore); }

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

			 strcpy(szOperator,Operatore);
			 p=GiveMeElement(p,E2,Operatore);
			 Type=StringCompareConverter(E1,E2,szOperator,CodeProcedure);
			 if (Type==RE_ERR) return ON;
			 strcpy(szOperator,Operatore);
		 }
		 else
	*/

	 psRet=_ElementNumConverter(psScript,E1);
	 if (!psRet) return true;
	 dValore=psRet->dValue; valDestroy(psRet);
	 

	 // -----------------------------------
	 // PRIMA VOLTA : Assegna un valore   !
	 // -----------------------------------

	 if (!*szOperator) {Risultato=dValore;}

	 // -----------------------------------
	 //  SOMMA AL VALORE PRECEDENTE       !
	 // -----------------------------------

	 if (*szOperator) 
	 {
		if (_private.bFormulaDebug) {win_infoarg("Calcolo [%.2f] %s [%.2f] \n",Risultato,szOperator,dValore); }
	 }

	 // ------------------------------------
	 //  & AND LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(szOperator,"&&"))
			{
			 Risultato=(dValore&&Risultato);
			 goto AVANTI;
			}

	 // ------------------------------------
	 //  | OR  LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(szOperator,"||"))
			{
			 Risultato=(dValore||Risultato);
			 goto AVANTI;
			}
	/*
	 // ------------------------------------
	 //  ^ XOR  LOGICO CON IL PRECEDENTE    !
	 // ------------------------------------
	 if (!strcmp(szOperator,"^"))
			{
			 Risultato=((LONG) Valore^(LONG) Risultato);
			 goto AVANTI;
			}
	*/
	 if (*szOperator)
	 {
		if (_private.bFormulaDebug) {win_infoarg("szOperator ? [%s]",szOperator); }
		
		if (extErrorNotify) (*extErrorNotify)(3,"Operatore sconosciuto: %s",szOperator);
		return TRUE;
	 }

	 AVANTI:
	 if (*p==0) break; // Finito
	 strcpy(szOperator,Operatore);
	}

	sprintf(pszString,"%.3f",Risultato);
	if (_private.bFormulaDebug) {win_infoarg("<_BooleanOperatorConverter: \n %s %.3f",pszString,Risultato); }

	return OFF;
}


//-----------------------------------------------=
//  _SimpleFORMtoNumber                           !
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
static INT _SimpleFORMtoNumber(S_SCRIPT * psScript,CHAR *pszFormula)
{
	EN_TAG_TYPE enType;
	double dValore=0;
	S_UNIVAL * psRet;
	if (_private.bFormulaDebug) win_infoarg(">> _SimpleFORMtoNumber START=|%s| \n",pszFormula);

	// -------------------------------------------
	// Se la formula Š un numero costante        !
	// non la trasformo in auto variabile        !
	// -------------------------------------------

	// La formula è un unica variabile Numerica
//	Tipo=_WhatIsThis(pszFormula,psFor);
	psRet=_WhatIsThis(psScript,pszFormula,&enType,true);//funcExt,Valore);
	
	if (psRet) dValore=psRet->dValue;
	if (enType==E_INTFUNC)
		{
			sprintf(pszFormula,"%.3f",dValore);
			goto FINE;
		 }

	if (enType==E_VARNUM)
	{	
		ehError();
		/*
		_formulaExtCall(WS_FIND,pszFormula,psFor);
		if (psFor->sEx.psRet) dValore=psFor->sEx.psRet->dValue;
		sprintf(pszFormula,"%.3f",dValore);
		*/
		goto FINE;
	}

 // -------------------------------------------
 // IDEM se Š una variabile/costante          !
 // -------------------------------------------
 // E' un unico NUMERO o una VARIABILE o una COSTANTE
 if ((enType==E_NUMDEC)||
	 (enType==E_NUMHEX)||
	 (enType==E_NUMBIN)) goto FINE;

 // -------------------------------------------
 // Controllo sulle Stringhe                  !
 // -------------------------------------------
 //if ((Tipo==I_CHARFIX)||(Tipo==I_CHARCONST)||(Tipo==I_CHAR)) goto FINE;

 // -------------------------------------------
 // Converte gli operatori di priorit… ALTA   !
 // in AutoVariabili                          !
 // -------------------------------------------

	if (_firstOperatorConverter(psScript,pszFormula)) 
		return true;
	//Tipo=_WhatIsThis(pszFormula,psFor);//funcExt,NULL);
	psRet=_WhatIsThis(psScript,pszFormula,&enType,false);//funcExt,Valore);
	if ((enType==E_NUMDEC)||(enType==E_NUMHEX)||(enType==E_NUMBIN)) goto FINE;

	if (_secondOperatorConverter(psScript,pszFormula)) 
		return true;
	if (_BooleanOperatorConverter(psScript,pszFormula)) 
		return true;

 // Mi ritorna una stringa
 //if (strstr(pszFormula,"ACHR")) goto FINE;

 //VARautopush(Nome); // Richiede una variabile
 //	 VARStackPush(RE_HL,Nome);
 if (_private.bFormulaDebug) {win_infoarg("AUTOPUSH[%s]\n",pszFormula); }
	 //ASMVpush("HL>",Nome);
	 //VARStackPush(RE_HL,Nome);

//	 if (_private.bFormulaDebug) {win_infoarg("Offset[%ld]\n",off); }
	 //VARSetUsata(Nome,CodeProcedure);
 //strcpy(pszFormula,Nome);

 FINE:
 if (_private.bFormulaDebug) {win_infoarg("<< _SimpleFORMtoNumber END=|%s| \n",pszFormula); }
 return OFF;
}

//
// _funcExecute()
//
static S_UNIVAL * _funcExecute(S_SCRIPT *psScript,CHAR * pszObject) {

	S_SCRIPT_TAG * psTag;
	S_UNIVAL * psVar=NULL;
	psTag=scriptTag(_arsBasic,TG_GLOBAL,pszObject);	
	if (psTag&&psTag->enTypes&TE_FUNCTION) {
		switch (psTag->iCode) {
			case 1101: // LEFT
				psVar=_leftString(psScript,pszObject,NULL);
				break;

			case 1102: // LEFT
				psVar=_rightString(psScript,pszObject,NULL);
				break;


			case 1107: // ATOI
				psVar=_stringToNum(psScript,pszObject,true);
				break;

			case 1108: // ATOF
				psVar=_stringToNum(psScript,pszObject,true);
				break;

			default:
				ehExit("%s non implementato" CRLF,pszObject);
		}
	}
	
	return psVar;

}

//
// Estrae il valore di una variabile o altro
//
static S_UNIVAL * _varGet(S_SCRIPT * psScript,CHAR * pszName,EN_TAG_TYPE * penType,BOOL bAlloc) {

	S_UNIVAL * psVar;
	EN_TAG_TYPE enType=E_VARNUM;

	//
	// Prima controllo se è una variabile
	//
	psVar=_varManager(psScript,WS_REALGET,pszName,NULL);
	if (psVar&&bAlloc) psVar=valDup(psVar);
		
	//
	// Poi controllo se è una funziona esterna
	//

	if (!psVar) {

		//
		// Domando alla funzione esterna
		//
		if (psScript->funcExt) 
		{
			psVar=psScript->funcExt(psScript,WS_REALGET,pszName,NULL);
			if (psVar) {
				enType=E_VARNUM; 
				if (psVar->enDaTy==_TEXT) 
					enType=E_VARTEXT; 
			}
			else enType=E_VARUNKNOW;
			if (!bAlloc) valDestroy(psVar);
		}
	}

	//
	// Poi controllo se è il risultato di una funzione
	//
	if (!psVar) {

		psVar=_funcExecute(psScript,pszName);
		if (psVar) {
			enType=E_VARNUM; 
			if (psVar->enDaTy==_TEXT) 
				enType=E_VARTEXT; 
		}
		else enType=E_VARUNKNOW;
		if (!bAlloc&&psVar) valDestroy(psVar);
	}

	*penType=enType; // Mha ... non saprei !!
	return psVar;
}

//
// _WhatIsThis()
//
//static INT _WhatIsThis(CHAR * okl,FORMULA_FUNC_EXT,double *lpRetValore)
//static INT _WhatIsThis(CHAR * pszElement,S_FORMULA * psFor)
static S_UNIVAL *	_WhatIsThis(S_SCRIPT * psScript,
								CHAR * pszElement,
								EN_TAG_TYPE * penType,
								BOOL bGetValue) // FORMULA_FUNC_EXT,void*Valore);
{
//	double dValore=0;
	CHAR *p;
	BOOL fString;
	CHAR *lpClose;
	S_UNIVAL * psRet=NULL;
	S_UNIVAL * psVar;
	EN_TAG_TYPE enType=E_ERROR;

	while (true) {

		//
		// Costanti 
		//
		if (*pszElement<'A'||*pszElement=='-')
		 {
			 if (IsDecNumero(pszElement)) enType=E_NUMDEC;
			 else if (IsHexNumero(pszElement)) enType=E_NUMHEX; 
			 else if (IsBinNumero(pszElement)) enType=E_NUMBIN;  //return E_NUMBIN;
			 else if (_isStringFix(pszElement)) enType=E_TEXT; //return E_TEXT; // ... per intenderci "cazzo" ;-)
			 else if (_stringSum(psScript,pszElement,true)) enType=E_TEXT_SUM; 

			 // Potrebbe essere una formula
			 else if ((*pszElement>='0')&&(*pszElement<='9')) enType=E_FORMULANUM; 
			 else if (*pszElement=='(') enType=E_FORMULANUM; 
			 else enType=E_ERROR;

			 if (enType!=E_ERROR&&
				 bGetValue) {
			 
				 switch (enType) {
				 
						case E_NUMDEC: psRet=valCreateNumber(atof(pszElement)); break;
						case E_NUMHEX: psRet=valCreateNumber(xtoi(pszElement)); break;
						case E_NUMBIN: ehError();//  non implementato
						case E_TEXT: 
							// E' un testo delimitato da quote quindi tolgo il carattere di testa e di coda
							if (strlen(pszElement)<3)
								psRet=valCreate(_TEXT,0,""); 
							else {
								CHAR * psz=strTake(pszElement+1,pszElement+strlen(pszElement)-2);
								psRet=valCreate(_TEXT,0,psz); 
								ehFree(psz);
							}
							break;

						default: break;
				 }
			 }
			 break;
		}

		//
		// Stringa
		//
		if (IsStringVar(pszElement)) {
			enType=E_VARTEXT; 
			*penType=enType;
			//if (enType==E_TEXT&&!psRet) 
			//{
			// psRet=valCreate(_TEXT,0,pszElement);
			psVar=_varGet(psScript,pszElement,&enType,true);

			// Se voglio il valore devo cercarlo 
			return psVar;

			//ehError();
			//break;

		}// ... per intenderci VALORE$
		else {

			// Controllo che non contenga operatori "la cosa" prima di chiedere alla funzione esterna
			fString=FALSE;
			for (p=pszElement;*p;p++)
			{
				if (*p=='\"') fString^=1;
				if (fString) continue;
				if (*p=='*'||
					*p=='='||
					*p=='>'||
					*p=='<'||
					*p=='/')
					{
						enType=E_VARUNKNOW; // E' una formula
						break;	
					}
				}
		}


	 // ------------------------------------------------------------
	 // Controllo se è una funzione matematica Interna
	 // (Forse da inserire parametricamente)
	 //
	 if (strlen(pszElement)>5)
	 {
		 // Intero (conversione a Intero di un numero)
		 if (!memcmp(pszElement,"INT(",4)||
			 !memcmp(pszElement,"FIX(",4))
		 {
			// Post Controllo Funzione
			 lpClose=_ParChiudi(pszElement+3);
			 if (lpClose)
			 {if (!*(lpClose+1))
				{
					// Estraggo formula contenuta nella funzione
					CHAR *lpNewFormula=ehAlloc(strlen(pszElement)+1);
					*lpClose=0; strcpy(lpNewFormula,pszElement+4); *lpClose=')';
					
					//
					// Eseguo la formula
					//
					psRet=_formulaProcess(psScript,lpNewFormula);
	//				if (psRet) dValore=psFor->sEx.psRet->dValue;
					ehFree(lpNewFormula);
					
					// Se tutto ok, calcolo il valore della funzione
					if (psRet)
					{
						//printf("Valore prima %.2f",Valore); getch();
						if (!memcmp(pszElement,"INT(",4)) psRet->dValue=(INT) (psRet->dValue+.5);
						if (!memcmp(pszElement,"FIX(",4)) psRet->dValue=(INT) psRet->dValue;
						// printf("Valore dopo %.2f",Valore); getch();

	//					if (pdRet) *pdRet=dValore;
						enType=E_INTFUNC; // E' una formula
						break;
					}
					}
				 }
			 }
		 }


	 //
	 // Variabili Script
	 //
	
		psRet=_varGet(psScript,pszElement,&enType,bGetValue);
//		if (bGetValue&&psVar) psRet=valDup(psVar);
		break;
	}

	if (enType==E_VARUNKNOW) {
	
		fString=FALSE;
		for (p=pszElement;*p;p++)
		{
			if (*p=='\"') fString^=1;
			if (fString) continue;
			if (*p=='*'||
				*p=='='||
				*p=='>'||
				*p=='<'||
				*p=='/')
				{
					enType=E_FORMULANUM; // E' una formula
					break;	
				}
		}

	}

	if (enType) *penType=enType;
	if (psScript->bTrace&&enType==E_VARUNKNOW) {
		printf("varUnknow: [%s]\7" CRLF,pszElement);
	}
	return psRet;
}

static INT IsDecNumero(CHAR *pszString)
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


static INT IsHexNumero(CHAR *pszString)
{
 CHAR *p;
 INT l;
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

static INT IsBinNumero(CHAR *pszString)
{
 CHAR *p;
 INT l;
 p=pszString;
 if (memcmp(p,"0B",2)) return OFF;
 p+=2;
 l=strlen(p);
 if ((l<1)||(l>16)) return OFF; // Non valido

 for (;*p;p++) {if ((*p<'0')||(*p>'1')) return OFF;}
// printf("PK");
 return ON;
}

static INT _isStringFix(CHAR *dato)
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

//
// _isStringSum()  - E' una sommatoria di stringhe
//
// - Operatore solo somma
// - Oggetti ammessi
// - Stringhe statiche
// - Variabili Stringa
// - Funzioni che ritorna no una stringa
// 
static S_UNIVAL * _stringSum(S_SCRIPT * psScript,CHAR * pszSum,BOOL bOnlyVerify)
{
	EH_LST lst=lstNew();
	CHAR * p, * psz;
	CHAR c, * pszStart=NULL,cQuote=0;
	EN_TAG_TYPE enType;
	BOOL bSum=false;
	S_UNIVAL * psRet=NULL;
	for (p=pszSum;*p;p++) {
		
		c=*p;
		
		if (!lst) break;

		//
		// Fuori da una stringa
		//
		if (!cQuote) {
		
			pszStart=p;
			if (c=='\"'||c=='\'') {cQuote=c; continue;} // Inizio costante stringa
			if (c<33) continue;
			if (c=='+') {

				if (bSum) 
				{
					if (!bOnlyVerify) scriptError(psScript,SER_SINTAX,"(++) string sum error [%s]",pszSum); // Trovato doppio +
					lst=lstDestroyPtr(lst);
					break;
				}
				if (!lst->iLength) 
				{
					if (!bOnlyVerify) scriptError(psScript,SER_SINTAX,"+ string start [%s]",pszSum); // Errore non può iniziare cosi
					lst=lstDestroyPtr(lst);
					break;
				}
				bSum=true;
				continue; // Somma ok
			}

			//
			// Determino la fine dell'oggetto (deve iniziare per una lettera)
			//
			if (c<'A'||c>'Z') {
				if (c<'a'||c>'z') {
					if (!bOnlyVerify) scriptError(psScript,SER_SINTAX,"String sum number [%s]",pszSum);
					lst=lstDestroyPtr(lst);
					break;
				}
			}
	
			for (;*p;p++) {
				c=*p;
				if (c==' ') return false; // Sintax error: Trovato spazio inaspettato
				
				//
				// + Funzione (Non verifcata)
				//
				else if (c=='(') 
				{
					CHAR * pszEnd=NULL;
					pszEnd=_ParChiudi(p);
					psz=strTake(pszStart,pszEnd); enType=E_VARUNKNOW;
					psRet=_WhatIsThis(psScript,psz,&enType,false);
					if (enType!=E_VARTEXT&&enType!=E_TEXT) 
					{
						if (!bOnlyVerify) scriptError(psScript,SER_SINTAX,"+ %s var unknow [%s]",pszStart,pszSum);
						lst=lstDestroyPtr(lst);
						if (psRet) valDestroy(psRet);
						ehFree(psz);
						break;
					}
					if (psRet) valDestroy(psRet);
 					lstPushPtr(lst,psz); // Costante psz
					p=pszEnd;
					break;

				}
				//
				// + Variabile (Non verificata)
				//
				else if (c=='+') {

					psz=strTake(pszStart,p-1); enType=E_VARUNKNOW;
					psRet=_WhatIsThis(psScript,psz,&enType,false);
 					lstPushPtr(lst,psz); // Costante Stringa
					if (enType!=E_VARTEXT&&enType!=E_TEXT) 
					{
						if (!bOnlyVerify) scriptError(psScript,SER_SINTAX,"+ %s var unknow [%s]",pszStart,pszSum);
						lst=lstDestroyPtr(lst);
						if (psRet) valDestroy(psRet);
					}
					if (psRet) valDestroy(psRet);
					break;
				}
			}
			
			bSum=false;



//			psRet=_WhatIsThis(psScript,p,&enType,true);//funcExt,NULL);
//			printf("%s",p);

		
		} else { // Prelevo stringa quotata
		
			if (c=='\\') {p++; continue;}
			if (c==cQuote) {
				psz=strTake(pszStart,p);
				lstPushPtr(lst,psz); // Costante Stringa
				cQuote=0;
			}
		}
	}
	//
	// Se richiesto costruisco la stringa di ritorno
	//
	if (bSum) {
		if (!bOnlyVerify) scriptError(psScript,SER_SINTAX,"+ open [%s]",pszSum);
	}

	if (!bOnlyVerify) {
		
		CHAR * pszAdd=strDup("");
		CHAR * pszStr=NULL;
		S_UNIVAL * psVal;
		
		for (lstLoop(lst,psz)) {
			
			psVal=scriptGetValue(psScript,psz);
			if (!psVal) {
				scriptError(psScript,SER_SINTAX,"sintax error [%s]",pszSum);
				lst=lstDestroyPtr(lst);
				break;
			}

			if (psVal->enDaTy!=_TEXT) {
				scriptError(psScript,SER_SINTAX,"not string value [%s]",pszSum);
				lst=lstDestroyPtr(lst);
				break;
			}
			strCat(&pszAdd,psVal->pszString);
			valDestroy(psVal);

		}

		//
		// build string's sum
		// 
		if (!lst) ehFree(pszAdd); 
		else 
		{
			psRet=valCreateText(pszAdd);
			ehFree(pszAdd);
		}


	} else {
		if (lst) {
			if (lst->iLength) psRet=(void * ) 1;
		}
		
	}
	if (lst) lstDestroyPtr(lst);

	return psRet;
	/*
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
	*/
}


static INT IsStringVar(CHAR *dato)
{
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
 if (_isStringFix(lpCosa)||IsStringVar(lpCosa)) return true; else return false;
}

//
// scriptFuncArg() - Parse Function
// Estrae in un array gli argomenti di una funzione
// pCharStart/Stop sono i delimitatori (eventuali parentesi)
//
EH_LST scriptFuncArg(BYTE * pString,BYTE * pCharStart,BYTE * pCharStop,INT * piError,CHAR ** ppEndString)
{
	BYTE *p;
	BYTE bCharStart,bCharEnd;
	BYTE *pArgStart=NULL;
	BYTE *pArg;
	EH_LST lst;
	S_ARG_INFO sAi;

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
	BOOL bBreak=FALSE;
	BYTE *pError="!\1[ERROR]";
	BYTE bCharArgSep=',';
	INT iError=0;

	if (piError) *piError=0;
	bCharStart=(pCharStart)?*pCharStart:'(';
	bCharEnd=(pCharStop)?*pCharStop:')';
	
	lst=lstCreate(sizeof(S_ARG_INFO));

	for (p=pString;*p;p++)
	{
		switch(iMode)
		{
			case BRACKET_START_SEARCH:
				if (*p==bCharStart) iMode=ARG_SEARCH;
				if (*p==bCharEnd) {
					_(sAi); sAi.enType=E_ERROR; lstPush(lst,&sAi);
					iError++; 
					bBreak=TRUE; 
					break;} // Finito
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

					_(sAi); 
					sAi.enType=isNaN(pArg)?E_FORMULANUM:E_VARNUM; 
					sAi.pszValue=strDup(pArg);
					lstPush(lst,&sAi);

//					ARAddarg(&arArg,"%s\1%s",isNaN(pArg)?"F":"N",pArg);  // F = pszFormula (numeri e variabili)
					ehFree(pArg);
					iMode=ARG_SEARCH;

					if (*p==bCharEnd) {bBreak=TRUE; break;} // Finito
					continue;
				}

				// Errore 
				if (strchr("\'\"",*p)) 
				{
//					ARAdd(&arArg,pError);
					_(sAi); sAi.enType=E_ERROR; lstPush(lst,&sAi);

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
				if (pArg) {
					_(sAi); 
					sAi.enType=E_TEXT;
					sAi.pszValue=strDup(pArg);
					lstPush(lst,&sAi);

					ehFree(pArg);
				} else { // Stringa VUota
					
					_(sAi); 
					sAi.enType=E_TEXT;
					sAi.pszValue=strDup("");
					lstPush(lst,&sAi);
				
				}
				iMode=ARG_SEP_SEARCH;
				break;

			case ARG_GET_STRING_DQ: 
				if (*p!='\"') continue;
				pArg=strTake(pArgStart+1,p-1); 
				if (pArg) {
//				ARAddarg(&arArg,"S\1%s",pArg?pArg:"");  // 1 = Numero o variabile
					_(sAi); 
					sAi.enType=E_TEXT;
					sAi.pszValue=strDup(pArg);
					lstPush(lst,&sAi);

					ehFree(pArg);
				} else { // Stringa VUota
					
					_(sAi); 
					sAi.enType=E_TEXT;
					sAi.pszValue=strDup("");
					lstPush(lst,&sAi);
				
				}
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
				_(sAi); sAi.enType=E_ERROR; lstPush(lst,&sAi);
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

	if (iMode==ARG_GET_STRING_SQ||iMode==ARG_GET_STRING_DQ) {
		_(sAi); 
		sAi.enType=E_ERROR; 
		sAi.pszValue=strDup("quote open");
		lstPush(lst,&sAi);
		iError++; 

	}
	if (piError) *piError=iError;
	if (ppEndString) *ppEndString=p;
//	if (piArgNum) *piArgNum=ARLen(arArg);
	return lst;

}

//
// scriptFuncDestroy()
//
EH_LST scriptFuncDestroy(EH_LST lst) {

	S_ARG_INFO * psInfo;
	for (lstLoop(lst,psInfo)) {
		ehFreeNN(psInfo->pszValue);
	}
	lstDestroy(lst);
	return NULL;
}

//
// scriptFuncGet()
//
S_UNIVAL * scriptFuncGet(S_SCRIPT * psScript,EH_LST lstParams,INT idx,EH_DATATYPE enDaTy) {

	S_ARG_INFO * psId;
	S_UNIVAL * psVal=NULL;
	psId=lstGet(lstParams,idx);	
	if (!psId) 
		return NULL;
	if (psId->enType==E_TEXT) {
		psVal=valCreateText(psId->pszValue);
	}
	else if (psId->enType==E_VARNUM) {
		psVal=valCreateNumber(atof(psId->pszValue));
	} else {
	
		psVal=scriptGetValue(psScript,psId->pszValue);
		//if (!psVal) psVal=psScript->funcExt(psScript,WS_REALGET,psId->pszValue,NULL); // FOrse non serve

	}
	if (!psVal) return psVal;
	if (enDaTy!=_UNKNOW&&psVal->enDaTy!=enDaTy) {
		valDestroy(psVal); 
		scriptFuncDestroy(lstParams);
		return NULL;
	}
	return psVal;
}


//
// ---------------------------------------------------------------------
//  _scriptProcessor
//  Oh Signore, fai che non perda l'illuminazione in questa primavera
//  .... halleluya,halleluya,halleluya .... :-o
//  
//									by Ferrà Art & Technology 1993-2005
//
// Di default ritorna FALSE
//
// ---------------------------------------------------------------------


// Cerca i limiti in cui operare in una condizione di IF

// POSSO AVERE DUE SITUAZIONI
// a) limite con bracket oppure no
// c) else oppure no
//
// Devo ritornare se c'è un cambio di profondità 
// Es. condizione VERA senza ELSE  = cambio di profondità 
//     condizione FALSE con ELSE   = cambio di pronondità
//     condizione FALSA senza ELSE = nessun cambio di profondità
//
// Ogni zona contiene:
// -  Cosa mi determina la fine dell'area di codice processato
// -  Dove devo andare una volta finito di processare l'area
// 

static BOOL _ifSearch(S_SCRIPT * psScript,BOOL bCond)
{
	INT iPar=0;
	BOOL bFirst=true;
	BOOL fError=FALSE;
	S_SCPOS sPos;
	CHAR * lp=_getScript(psScript);

	_(sPos);
	
	// Determino è un if su una linea o con delimitazione a bracket
	// se il primo carattere che incontro è un bracket è la seconda
	for (;*lp;lp++)
	{
		// Determino il tipo di ricerca
		if (bFirst)
		{
			if (strchr(" \n\r",*lp)) continue;
			sPos.pszStart=lp; // Inizio
			if (*lp=='{') 
			{
				sPos.fBracketEnd=true;
				sPos.pszStart++;
			}
			else 
			{
				sPos.fBracketEnd=false;
			}
			bFirst=FALSE;
		}

		// Ricerca la fine del bracket
		if (sPos.fBracketEnd)
		{
			if (*lp=='{') iPar++; 
			if (*lp=='}') iPar--;	
			if (!iPar) {fError=FALSE; break;}
			if (iPar<0) break;
		}
		else
		{
			// ricerco la fine della riga
//			if (*lp=='\n') {fError=FALSE; break;}
			if (strchr(";\n\r",*lp)) {fError=FALSE; break;}

		}
	}

	//
	// Metto nello stack la nuova porzione di codice
	//
	if (!fError)
	{
		S_SCPOS * psPos;
		sPos.pszEnd=lp; // *sPos.pEnd=0;
		if (bCond) // Condizione vera
		{
			//
			// Posizione la ripresa dello script attuale a dopo l'IF
			//
			psPos=lstLast(psScript->lstPos);
			psPos->pszStart=sPos.pszEnd+1; 
			lstPush(psScript->lstPos,&sPos);
		}
		else
		{
			psPos=lstLast(psScript->lstPos);
			psPos->pszStart=sPos.pszEnd+1; 
		}

	}
	return fError;
}


				// printf("Tipo: %d\n",sTrue.fBracketEnd);
				// printf("[%s]",sTrue.pStart);

				// ELSE DA FARE

				// Determinare Inizio e Fine IF, Inizio e fine Else
				// POSSO AVERE DUE SITUAZIONI
				// a) limite con bracket oppure no
				// c) else oppure no
				//
				// Devo ritornare se c'è un cambio di profondità 
				// Es. condizione VERA senza ELSE  = cambio di profondità 
				//     condizione FALSE con ELSE   = cambio di profondità
				//     condizione FALSA senza ELSE = nessun cambio di profondità
				//
				// Ogni zona contiene:
				// -  Cosa mi determina la fine dell'area di codice processato
				// -  Dove devo andare una volta finito di processare l'area
				// 

				// a) Condizione VERA e non ho ELSE
				//	  Proseguo finchè non trovo la fine della condizione

				// b) Condizione VERA e ho ELSE
				//	  Proseguo finchè non trovo la fine della condizione poi salto dopo l'else

				// c) Condizione FALSA e non ho ELSE
				//	  Salto dopo la fine della zona di IF

				// d) Condizione FALSA e ho l' ELSE
				//	  Salto dove inizia l'ELSE fino alla fine dell'ELSE

				/*
				if (bCond) // Condizione vera
				{
					
					//
					// Posizione la ripresa del livello attuale a dopo l'IF
					//
					psPos=lstLast(psScript->lstPos);
					psPos->pStart=sPos.pEnd+1; 
					lstPush(psScript->lstPos,&sPos);
					// Incremento un livello
//					iPos++; memcpy(&arPos[iPos],&sTrue,sizeof(SCPOS));
//					if (iPos>=MAX_POS) ehError();
					//printf("{%s}\n",arPos[iPos].pStart);
					return false;
				}
				else
				{
//					arPos[iPos].pStart=sTrue.pEnd+1; 
					psPos=lstLast(psScript->lstPos);
					psPos->pStart=sPos.pEnd+1; 

					return false;
				}
				*/
/*
#define MAX_POS 50
INT iPos=0;
SCPOS arPos[MAX_POS];
*/

//
// scriptCreate()
//
S_SCRIPT *  scriptCreate(S_SCRIPT * psSource)
{
	S_SCRIPT * psScript=ehAllocZero(sizeof(S_SCRIPT));
	
	memcpy(psScript,psSource,sizeof(S_SCRIPT));
	_varManager(psScript,WS_OPEN,0,NULL);
	psScript->lstPos=lstCreate(sizeof(S_SCPOS));
	psScript->lstNote=lstNew();

	return psScript;

}

//
// scriptExecute()
//
S_UNIVAL *  scriptExecute(S_SCRIPT * psScript,CHAR * pszTextScript)
{
	BYTE *	lpClone;
	CHAR	szChar[2];
	CHAR *	pszBuffer=ehAlloc(1024);
	CHAR *	lpLastLine=ehAlloc(1024);
	BOOL	fRet=FALSE;
	BOOL	fStop=FALSE;
	S_SCRIPT_TAG * psTag;

//	INT iSize;
	S_SCPOS sPos, * psPos;
	INT iCounterControl=0;
	CHAR * pszToken=NULL;

	if (psScript->bTrace) printf(CRLF);

	psScript->enError=0;
	if (psScript->lstError) lstClean(psScript->lstError); else psScript->lstError=lstNew();
/*
#ifdef _DEBUG
	_isStringSum(psScript,"ORD$+LEFT(\"2000\",30)");
	_isStringSum(psScript,"\"(JM_DATEINS=\"+MOVI.DATE$+\" AND IDCATEGO=2 AND MCODEST LIKE '\"+ORD$+\"%')\"");
#endif
*/
//	iSize=strlen(pszTextScript)+10;
	lpClone=strDup(pszTextScript);
	_strupr(lpClone);

	while (strReplace(lpClone,CRLF "","\n"));
	while (strReplace(lpClone,CRLF,"\n"));


	//
	// Inserisco porzione scrip in analisi
	//
	lstClean(psScript->lstPos);
	_(sPos);
	sPos.pszStart=lpClone;
	sPos.pszEnd=lpClone+strlen(lpClone)-1;
	psPos=lstPush(psScript->lstPos,&sPos);
	psScript->iRow=0;

	if (*psPos->pszStart) 
	{
		CHAR *seps=" ,(=;\n{}";
		iCounterControl++;
		//strcpy(lpLastLine,arPos[iPos].lpScan);

		//pszToken=_token(psScript,seps, szChar,MODE_STD);
		while (pszToken=_token(psScript,seps, szChar,MODE_STD))
		{
			//
			// esco in caso di errore
			//
			if (psScript->enError)
			{
				if (psScript->bVerbose) 
				{
					printf(CRLF CRLF "Linea %d)" CRLF,psScript->iRow);
					scriptShowErrors(psScript);
				}
				break;
			}
			if (!*pszToken) continue;

			//
			// Analisi del token
			//

			//
			// a) Controllo se è uno STANTMENT standard
			//
			psTag=scriptTag(_arsBasic,TG_GLOBAL,pszToken);
			if (psScript->bTrace&&psScript->bVerbose) 
				printf("%d: %s (id:%d)" CRLF,psScript->iRow,pszToken,psTag?psTag->iCode:0);
			
			if (psTag) {

				if (_tagEngine(psScript,psTag->iCode,pszToken,szChar,pszBuffer)) // Ritorna true se deve fermarsi
					break;

			}
			else {
				
				CHAR szVarName[200];
				INT idx;
			// 
			// b) E' un' Assegnazione di una variabile ?
			//
				if (*szChar=='=') {
					
					S_UNIVAL * psRet;
					CHAR * pszExt;
					strCpy(szVarName,pszToken,sizeof(szVarName));
					pszExt=_token(psScript,";", szChar,MODE_ENDASSIGN);
					if (!pszExt) 
							ehError();
					strcpy(pszBuffer,pszExt); // Cerco cosa assegnare
					_charStop(pszBuffer,';'); // Metto una fine a ciò
					// Questa è la formula
					if (_private.bVerbose&&psScript->bVerbose)  printf(": varAssign : %s={%s}",szVarName,pszBuffer); //getch();
					// Se non è una stringa
					if (!_stringSum(psScript,pszBuffer,true)) _booleanSwapping(pszBuffer); // Cambio AND in && e OR in ||

					psRet=scriptGetValue(psScript,pszBuffer);
					if (!psRet) 
					{
						psRet=scriptGetValue(psScript,pszBuffer); // Da togliere
						scriptError(psScript,SER_SINTAX,"Formula errata [%s]",pszBuffer);
						continue;
					}

					idx=(INT) _varManager(psScript,WS_FIND,szVarName,NULL);
					// Assegnazione di una variabile interna allo script ?
					if (idx!=-1) {

						_varManager(psScript,WS_REALSET,szVarName,psRet);
						if (_private.bVerbose&&psScript->bVerbose) {printf("="); valPrint(psRet); printf(CRLF);}//sFormula.dValue);
						valDestroy(psRet);
					}
					else {
					
						
					// Assegnazione e una variabile Esterna allo script ?
						if (psScript->funcExt) {
							void * pRet;
							pRet=psScript->funcExt(psScript,WS_REALSET,szVarName,psRet);
							if (!pRet) 
							{
								if (!psScript->enError) {
									scriptError(psScript,SER_SINTAX,"Assegnazione Variabile esterna errata");
								}
							
							}
							valDestroy(psRet);
						}

					}

				}
			// 
			// c) Chiedo al driver esterno
			//
				else {
					
//					ehError();
					//_tagEngine(psScript,0,pszToken,szChar,pszBuffer);
					if (psScript->funcExt) {
//						void * pRet;
						S_UNIVAL * psRet;
						CHAR * pszCmd=strDup(pszToken);

						//strcpy(pszBuffer,pszToken);
						sprintf(pszBuffer,"(%s",_token(psScript,";", szChar,MODE_ENDASSIGN)); // Cerco cosa assegnare
//						psRet=psScript->funcExt(psScript,WS_PROCESS,pszCmd,pszBuffer);
						psRet=psScript->funcExt(psScript,WS_REALGET,pszCmd,pszBuffer);
						if (!psRet) 
						{
							if (!psScript->enError) {
								scriptError(psScript,SER_SINTAX,"Funzione errata o sconosciuta [%s]",pszCmd);
							}
						
						}
						ehFree(pszCmd);
						if (psRet) valDestroy(psRet);
					}

				}

			}
		  if (fStop) break;

		} 

	}

	// Visualizzo le variabili
	if (psScript->bTrace&&psScript->bVerbose) 
	{
		printf(CRLF "------------------" CRLF);
		if (!psScript->psRet) 
			printf("Ritorno NULL");
			else
			printf("Ritorno [%d]" CRLF,psScript->psRet->dValue);
		_varManager(psScript,WS_DISPLAY,0,NULL);
	}

	ehFree(lpClone);
	ehFree(pszBuffer);
	ehFree(lpLastLine);
	return psScript->psRet;

}

//
// scriptDestroy()
//
S_SCRIPT * scriptDestroy(S_SCRIPT * psScript) {
	
	if (!psScript) return NULL;
	_varManager(psScript,WS_CLOSE,NULL,NULL);
	lstDestroy(psScript->lstPos);
	lstDestroy(psScript->lstNote);
	lstDestroy(psScript->lstError);
	valDestroy(psScript->psRet);
	ehFree(psScript);
	return NULL;

}


				/*
				_(sFormula);
				memcpy(&sFormula.sEx,&psScript->sEx,sizeof(sFormula.sEx));
				fCheck=formulaProcessEx(pszBuffer,&sFormula);

				if (fCheck) 
				{
				*/


//
// _tagEngine() 
// Return:
//	- False deve andare avanti
//  - True deve fermarsi
// 
static BOOL _tagEngine(S_SCRIPT * psScript,
					   INT		iCode,
					   CHAR *	pszToken,
					   CHAR *	pszChar,
					   CHAR *	pszBuffer) 
{

	S_UNIVAL * psRet;
	CHAR * pszVar;
	BOOL bCond;
	// S_SCPOS Pos,*psPos;
	

	//
	// Funzioni
	//

	switch (iCode)
	{
		case 1002: // Commento
			break;

		// --------------------------------------------------------
		// VAR - Dichiarazione delle variabili
		//
		case 1000: 
			if (psScript->bTrace&&psScript->bVerbose) {
				printf("Dichiarazioni variabili:" CRLF);
			}
			
			while (pszVar=_token(psScript, ",;", pszChar,MODE_STD))
			{
				if (_private.bVerbose&&psScript->bVerbose) printf( "{%s:%d}", pszVar,*pszChar);
				
				// Controllo validità del nome della variabile
				if (_varNameCheck(pszVar))
				{
//					psScript->enError=SER_SINTAX;
//					strcpy(psScript->szError,"Nome variabile non valido");
					scriptError(psScript,SER_SINTAX,"Nome variabile non valido");

					return true;
				}

				if (psScript->bTrace&&psScript->bVerbose) printf(" : create var '%s'" CRLF,pszVar);
				// Aggiungo la variabile all'ambiente
				_varManager(psScript,WS_ADD,pszVar,NULL);

				if (*pszChar==';') break; // Fine linea

			} 
			break;

		// --------------------------------------------------------
		// IF - Condizione
		//
		case 1001: 

				strcpy(pszBuffer,_token(psScript,"", pszChar,MODE_FORMULA)); // Cerco cosa assegnare
				if (!pszBuffer) 
				{
					scriptError(psScript,SER_SINTAX,"Probabili parentesi errate");
					break;
				}

				//
				// Controllo la condizione
				//
				_booleanSwapping(pszBuffer); // Es. da AND a && 
				psRet=scriptGetValue(psScript,pszBuffer);
				if (!psRet) 
				{
//					psScript->enError=SER_SINTAX;
//					sprintf(psScript->szError,"if error [%s]",pszBuffer);
					scriptError(psScript,SER_SINTAX,"if ? %s",pszBuffer);
				//	if (psScript->bVerbose) scriptShowErrors(psScript);//printf("%s" CRLF,psScript->szError);
					return true;
				}
				
				bCond=(BOOL) psRet->dValue;//sFormula.dValue;
				if (psScript->bTrace&&psScript->bVerbose) printf(" %s ? %d" CRLF,pszBuffer,bCond);
				valDestroy(psRet);


				//
				// La condizione determina un cambio di script in analisi
				// Il focus del parse si sposta su unl'altra sezione
				// E' necessari determinare quale sezione bisogna elaborare in pratica
				// Determinare i "confini" della codice assegnato alla condizione TRUE 
				// ed eventualmente alla FALSE , se c'è un "ELSE"

//				if (_ifSearch(_token(psScript,NULL,NULL,MODE_STD),&sPos)) 
				if (_ifSearch(psScript,bCond))
				{
					//psScript->enError=SER_SINTAX;
					//strcpy(psScript->szError,"Probabili parentesi { errate");
					scriptError(psScript,SER_SINTAX,"Probabili parentesi { errate");
					return true;
				}


				//printf("IF [%s] allora [%s]" CRLF,pszBuffer,lp2);	
				break;

		// --------------------------------------------------------
		// RETURN (Ritorno dato)
		//
		case 1004: 

				// bStop=true;
				pszVar=_token(psScript,";", pszChar,MODE_ENDASSIGN); // Cerco cosa assegnare
				if (!pszVar) 
				{
//					psScript->enError=SER_SINTAX;
//					strcpy(psScript->szError,"Return errato");
					scriptError(psScript,SER_SINTAX,"Return errato");
//					printf("Errore\n");
					return true;
				}
				strcpy(pszBuffer,pszVar);
				_booleanSwapping(pszBuffer);
				
				psRet=scriptGetValue(psScript,pszBuffer);
				/*
				_(sFormula);
				memcpy(&sFormula.sEx,&psScript->sEx,sizeof(sFormula.sEx));
				fCheck=formulaProcessEx(pszBuffer,&sFormula);

				if (fCheck) 
				{
				*/
				if (!psRet) {

					//psScript->enError=SER_SINTAX;
					//strcpy(psScript->szError,"Formula errata IN RETURN");
					scriptError(psScript,SER_SINTAX,"Formula errata IN RETURN");
				}
				else
				{


					if (_private.bVerbose&&psScript->bVerbose) {printf("%s=",pszBuffer); valPrint(psRet); printf(CRLF);}//sFormula.dValue);
					//
					// Nota: al momento l'unico return è quello principale e quindi Carico il valore di ritorno nello script
					//       In futuro potrei dover verificare se è un returno di una funzione o quello principale
					//
					psScript->psRet=psRet; // 
					
				}
				return true;
				break;

		// --------------------------------------------------------
		// PRINT - Stantment di Output
		//
		case 1005: 
			strcpy(pszBuffer,"PRINT(");
			pszVar=_token(psScript, ";", pszChar,MODE_ENDASSIGN); // Cerco cosa assegnare
			if (!pszVar) 
				{
					scriptError(psScript,SER_SINTAX,"PRINT() errato");
					return true;
				}
			strcat(pszBuffer,pszVar);
			if (_scPrint(pszBuffer,psScript)) 
			{
				scriptError(psScript,SER_SINTAX,"Sintax error in DEBUG");
				break;
			}
			break;

		// --------------------------------------------------------
		// ERROR.STOP
		//
		case 1006:
			_ERROR_SIGN_
			psScript->enError=SER_SINTAX;
			break;

		//
		// Oggetto sconosciuto ai più ;-)
		// Funzione esterna
		//
		default:

			//
			// Guardo se è un comando
			//
			if (*pszChar=='(')
			{
				S_SCRIPT_TAG * psTag;
				//strcpy(pszBuffer,_token(";", szChar,MODE_ENDASSIGN)); // Cerco cosa assegnare
				strcpy(pszBuffer,pszToken); strcat(pszBuffer,"(");
				pszVar=_token(psScript, ";", pszChar,MODE_ENDASSIGN); // Cerco cosa assegnare
				if (!pszVar) 
				{
					scriptError(psScript,SER_SINTAX,"comando sconosciuto errato");
					return true;
				}
				strcat(pszBuffer,pszVar);
				psTag=scriptTag(_arsBasic,TG_GLOBAL,pszBuffer);
				if (psTag) {
					if (psTag->enTypes!=TE_FUNCTION)
					{
	//					_ERROR_SIGN_
						scriptError(psScript,SER_SINTAX,"Comando/Funzione sconosciuta [ ? %s ? ]",pszToken);
						if (psScript->bVerbose) scriptShowErrors(psScript);//printf("%s",psScript->szError);
						return true;
						//lpLastError=szError;
					}
				}
				//printf("[%s:%d]" CRLF,pszBuffer,iCode);

	/*					_(sFormula);
				sFormula.pszFormula=pszBuffer;
				sFormula.pVoid=psMov;
				sFormula.funcExt=_sfScript;
	*/
	//					_sfScript(0,0,pszBuffer,NULL);//&dValore); // ??

				if (psScript->funcExt) 
				{
					//_sfScript(0,0,pszBuffer,&sFormula);
					S_UNIVAL * psRet=psScript->funcExt(psScript,WS_REALGET,pszBuffer,NULL);
					valDestroy(psRet);
				}
				break;
			}

			
			//
			// Guardo se è una variabile (dell utente)
			//
			if (*pszChar=='=')
			{
				S_UNIVAL * psVal;

				// E' una variabile di script ?
				psVal=_varManager(psScript,WS_REALGET,pszToken,NULL);
				if (psVal) 
				{
					strcpy(pszBuffer,_token(psScript, ";", pszChar,MODE_ENDASSIGN)); // Cerco cosa assegnare
					// printf("Variabile=[%s]\n",pszBuffer);
					_charStop(pszBuffer,';'); // Metto una fine a ciò
					// Questa è la formula
					if (_private.bVerbose)  printf("%s={%s}",pszToken,pszBuffer); //getch();
					_booleanSwapping(pszBuffer); // Cambio AND in && e OR in ||


					psRet=scriptGetValue(psScript,pszBuffer);
					if (!psRet) 
					{
//						psScript->enError=SER_SINTAX;
//						sprintf(psScript->szError,"** Errore in formula [%s]",pszBuffer);
						scriptError(psScript,SER_SINTAX,"** Errore in formula [%s]",pszBuffer);
						return true;
					}
					if (_private.bVerbose&&psScript->bVerbose) {printf("="); valPrint(psRet); printf(CRLF);} //%.2f",sFormula.dValue);
					_varManager(psScript,WS_REALSET,pszToken,psRet);
				}
				break;
			}

			// Errore ?????????
//			psScript->enError=SER_SINTAX;
//			sprintf(psScript->szError,"Comando sconosciuto [:%s:]",pszToken);
			scriptError(psScript,SER_SINTAX,"Comando sconosciuto [:%s:]",pszToken);
			return true;
	  }		  
	return false;
}


//
// _varManager()
//
static void * _varManager(S_SCRIPT * psScript,EN_MESSAGE enMess,CHAR * pszName,S_UNIVAL *psVal)
{
	INT a,i;
	S_VAR sVar;
	
	switch (enMess)
	{
		case WS_OPEN:
			DMIReset(&psScript->dmiVar);
			DMIOpen(&psScript->dmiVar,M_AUTO,100,sizeof(S_VAR),"VAR");
			DMILock(&psScript->dmiVar,NULL);
			break;

		case WS_ADD:
			i=(INT) _varManager(psScript,WS_FIND,pszName,0);
			if (i==-1) 
			{
				_(sVar);
				strCpy(sVar.szNome,pszName,sizeof(sVar.szNome));
				sVar.psRet=valCreate(_NUMBER,0,"");
				DMIAppendDyn(&psScript->dmiVar,&sVar);
				return 0; // Tutto ok
			}
			break;

		case WS_FIND:
			for (a=0;a<psScript->dmiVar.Num;a++)
			{
				DMIReadEx(&psScript->dmiVar,a,&sVar,"ID30");
				if (!strcmp(sVar.szNome,pszName)) return (void *) a;
			}
			return (void *) -1;

		// Assegno un valore ad una variabile
		// (WS_DO,Valore,Variabile)
		case WS_REALSET:
			i=(INT) _varManager(psScript,WS_FIND,pszName,0);
			if (i!=-1)
			{
				DMIReadEx(&psScript->dmiVar,i,&sVar,"ID31");
				//sVar.dValore=dValore;
				valDestroy(sVar.psRet);
				sVar.psRet=valDup(psVal); // Da vedere, ci vorrebbe un valDup()
				DMIWrite(&psScript->dmiVar,i,&sVar);
				return 0;
			}
			break;

		case WS_REALGET:
			i=(INT) _varManager(psScript,WS_FIND,pszName,0);
			if (i!=-1)
			{
				DMIRead(&psScript->dmiVar,i,&sVar);
				return sVar.psRet;
			}
			break;

		case WS_CLOSE:
			for (a=0;a<psScript->dmiVar.Num;a++)
			{
				DMIRead(&psScript->dmiVar,a,&sVar);
				valDestroy(sVar.psRet);
			}
			DMIClose(&psScript->dmiVar,"VAR");
			break;

		case WS_DISPLAY: // Visualizza la situazione delle variabili
			for (a=0;a<psScript->dmiVar.Num;a++)
			{
				DMIReadEx(&psScript->dmiVar,a,&sVar,"ID32");
				if (psScript->bVerbose) 
				{	printf("%d) %s=" CRLF,a,sVar.szNome);
					valPrint(sVar.psRet);
					printf(CRLF);
				}
			}
			//getch();
			break;

	}

	return NULL;
}


static void _booleanSwapping(CHAR *lpFormula)
{
	CHAR *lp;
	// Sostituisco AND e OR con relative C
	while (TRUE)
	{
		lp=strstr(lpFormula," AND "); if (!lp) break;
		memcpy(lp," && ",4);
	}
	while (TRUE)
	{
		lp=strstr(lpFormula," OR "); if (!lp) break;
		memcpy(lp," || ",4);
	}
}


static BOOL _varNameCheck(CHAR *lpNome)
{
	CHAR *lpc="0123456789ABCDEFGHIJKLMNOPQRSTUVXYZ_$";
	CHAR *lp;
	for (lp=lpNome;*lp;lp++)
	{
		if (!strchr(lpc,*lp)) return TRUE;
	}
	if (strEnd(lpNome,"$")&&strstr(lpNome,"$")) return true; // Non va bene

	return FALSE;
}

static CHAR * _getScript(S_SCRIPT * psScript) {

	S_SCPOS * psPos;
	psPos=lstLast(psScript->lstPos);
	return psPos->pszStart;

}

static CHAR * _tokenTake(S_SCRIPT * psScript,CHAR * pszStart,CHAR * pszEnd) {

	INT iLen=((DWORD) pszEnd-(DWORD) pszStart);
	if (iLen>=sizeof(psScript->szToken)) ehError();
	if (iLen>0) strCpy(psScript->szToken,pszStart,iLen); else *psScript->szToken=0;
	return psScript->szToken;

}
//
// _token()
//
// lpScan  = Stringa da analizzare
// lpChars = Caratteri che determinano la fine del token
// lpLastSep = ritorna Quale carattere ha determinato la fine
// RITORNA il token o NULL quando è finito
//
static CHAR * _token(S_SCRIPT * psScript, 
					 CHAR *		lpChars,
					 CHAR *		lpLastSep,
					 EN_TOKEN_MODE enMode)
{
	//static CHAR *lpLastStr=NULL;
	CHAR * lp,*lpt;
	CHAR * lpStart;
	CHAR * pbStart;
	CHAR * pszToken=NULL;

	INT iPar;
	S_SCPOS * psPos;

	if (lpLastSep) {lpLastSep[0]=0; lpLastSep[1]=0;}
	if (!lpChars) ehError();
	//if (lpScan) lpLastStr=lpScan;
	
	if (!psScript->iRowNext) psScript->iRowNext=1;

	psPos=lstLast(psScript->lstPos);
	pbStart=psPos->pszStart;
	psScript->iRow=psScript->iRowNext;
	for (;*pbStart;pbStart++) {if (!strchr(" \t\n\r",*pbStart)) break;}
	if (strEmpty(pbStart)) return NULL;

	while (true) {

		iPar=0;
		lp=lpStart=pbStart;//arPos[iPos].pStart;
		switch (enMode)
		{
			case MODE_STD: // Ricerca in modo standard ################################à
		
				for (;*lp&&lp<=psPos->pszEnd;lp++)
				{
					// Commento, cerco la fine
					if (!memcmp(lp,"//",2)) 
					{
						for (;*lp;lp++) {if (*lp=='\n') break;}
						pszToken=_tokenTake(psScript,lpStart,lp);
						*lpLastSep=*lp; //*lp=0; 
						psPos->pszStart=lp+1;// arPos[iPos].pStart=lp+1;
						if (*lpLastSep=='\n') psScript->iRowNext++;
						//pszRet=lpStart; 
						break;

					}

					lpt=strchr(lpChars,*lp); // Cerco il carattere nella lpChars
					if (lpt)
					{
						*lpLastSep=*lpt; //*lp=0; 
						pszToken=_tokenTake(psScript,lpStart,lp);
						psPos->pszStart=lp+1; // arPos[iPos].pStart=lp+1;
						if (*lpLastSep=='\n') psScript->iRowNext++;
						//pszRet=lpStart; 
						break;
					}
				}
				break;

			case MODE_ENDASSIGN: // Ricerca la fine di un comando
		
				// Ricerco uno dei caratteri di separazione
				for (;*lp&&lp<=psPos->pszEnd;lp++)
				{
					// Commento, cerco la fine
					if (!memcmp(lp,"//",2)) 
					{
						for (;*lp;lp++) {if (*lp=='\n') break;}
						pszToken=_tokenTake(psScript,lpStart,lp);
						*lpLastSep=*lp; //*lp=0; 
						psPos->pszStart=lp+1; //arPos[iPos].pStart=lp+1;
						if (*lpLastSep=='\n') psScript->iRow++;
						break;
					}

					// La fine è il ;
					if (*lp==';')
					{
						pszToken=_tokenTake(psScript,lpStart,lp);
						*lpLastSep=*lp; //*lp=0; 
						psPos->pszStart=lp+1; //arPos[iPos].pStart=lp+1;
						//pszRet=lpStart; 
						break;
					}
				}
				break;

			case MODE_FORMULA: // Ricerca la fine di una formula (ES IF)

				if (*lp!='(') break; // Deve iniziare con una parentesi
				for (;*lp&&lp<=psPos->pszEnd;lp++)
				{
					if ((*lp==' '||*lp=='\n')&&!iPar) break;
					if (*lp=='(') iPar++;	
					if (*lp==')') iPar--;	
					if (iPar<0) break;
				}
				if (!iPar)
				{
					*lpLastSep=*lp; //*lp=0; 
					pszToken=_tokenTake(psScript,lpStart,lp);
					psPos->pszStart=lp+1; //arPos[iPos].pStart=lp+1;
					//pszRet=lpStart; 
					break;
				}
				break;
		}

		//
		// enMode
		// Non ho più token su questo livello 
		// ritorno al precedente
		//
		if (enMode==MODE_STD&&
			!pszToken&&
			psScript->lstPos->iLength>1) {
				psPos=lstPop(psScript->lstPos); 
				ehFree(psPos);
				psPos=lstLast(psScript->lstPos);
				pbStart=psPos->pszStart;
				continue;
			
		}

		if (!pszToken) break; // Finito
		if (*pszToken) break;

		pbStart=psPos->pszStart;
			
		



/*
		if (psScript->lstPos->iLength>1) {

			psPos=lstPop(psScript->lstPos); 
			pbStart=psPos->pszStart;
			ehFree(psPos);
			//psPos=lstLast(psScript->lstPos);
			//pbStart=psPos->pszStart;
			continue;
		}
		*/
//		break;
	}
	if (psScript->bTrace&&psScript->bVerbose) {
	
		printf("Token:[%s]",pszToken);
		printf(CRLF);
	}

	


	return pszToken;
}

static void _charStop(CHAR *pszBuffer,CHAR ch)
{
	CHAR *lpt;
	lpt=strchr(pszBuffer,ch); // Cerco il carattere nella lpChars
	if (lpt) *lpt=0;
}

static CHAR * FormulaExtract(CHAR *lpDest,CHAR *lpSource)
{
	INT iPar=0;
	if (*lpSource!='(') return NULL; // Deve iniziare con una parentesi
	for (;*lpSource;lpSource++,lpDest++)
	{
		if (*lpSource==' '&&!iPar) {lpSource++; break;}
		if (*lpSource=='(') iPar++;	
		if (*lpSource==')') iPar--;	
		if (iPar<0) return NULL;
		*lpDest=*lpSource; 
	}
	*lpDest=0; 
	return lpSource;
}



// ----------------------------------------------------------------
// MASTER FIND ( or Mind ;-)
// Traduce la formula/variabile del linguaggio scripting
// Ritorna il codice assegnato
//
S_SCRIPT_TAG * scriptTag(	S_SCRIPT_TAG * psArrayTag,EN_TAGTYPE enGroup,CHAR * pszToken) 
{
  // Controllo le funzioni
  INT a;
  INT iCode=0;
  EN_TAGTYPE enTipoObj=0;
  BOOL fRet=TRUE;
  INT iIdx,iLen;
  S_SCRIPT_TAG * psTag;
  CHAR szServ[800];

  _strupr(pszToken);
  
  // --------------------------------------------------------
  // FUNZIONI
  //
  for (a=0;psArrayTag[a].enTypes;a++)
  {
		psTag=psArrayTag+a;
		if (!(psTag->enTypes&enGroup)) continue; // Solo le abilitate allo Controllo

		enTipoObj=psTag->enTypes&0xf;
		iIdx=a;
		switch (enTipoObj)
		{
		  case TE_FUNCTION: // Funzione
			iLen=strlen(psTag->pszName);
			strcpy(szServ,psTag->pszName); strcat(szServ,"(");
			if (!strCmp(psTag->pszName,pszToken)||
				!memcmp(szServ,pszToken,iLen+1)) 
				iCode=psTag->iCode; 
			break;

		  case TE_STANTMENT: // Stantment
			iLen=strlen(psTag->pszName);
			if (!memcmp(psTag->pszName,pszToken,iLen)) 
				iCode=psTag->iCode; 
			break;
			
		  case TE_VARIABLE: // Variabile
		  case TE_CONST: // Variabile
			if (!strcmp(psTag->pszName,pszToken)) 
				iCode=psTag->iCode; 
			break;
		}
		if (iCode) 
			break;
  }

/*
  // Se è una funzione cerco la parentesi quadra di chiusura
  if (iCode&&iTipoObj==TE_FUNCTION)
  {
	CHAR *p;
	//printf(CRLF "Dopo |%s|" CRLF,lpFormula); //getch();
	p=_parClose(lpFormula+strlen(_arsDict[iIdx].lpNome)+1);
	if (p==NULL) 
	{
		if (fError) printf(" > Errore di sintassi [%s] " CRLF,lpFormula);
		return 0;
	}
	p++;
	// Funzione non valida (continua dopo ])
	if (*p) 
	{
		if (fError) printf(" > Funzione non valida (continua dopo ] |%s|" CRLF,p);
		return 0;
	}
  }
*/
	if (!iCode) return NULL;
//	if (psType&&iCode) *psType=enTipoObj;
	return psTag;
//	return iCode;
}

// --------------------------------------------------
// _extVarAssign - Assegnazione di variabile 
// --------------------------------------------------

static BOOL _extVarAssign(INT iCode,S_UNIVAL * psRet,S_SCRIPT * psScript)
{
//	INT iValore=(INT)   dValore;
	/*
	switch (iCode)
	{
		case 50: psMov->iPuntiCalc=(INT) dValore; break;
		case 51: psMov->iPuntiOk=(INT) dValore; break;
		case 52: psMov->iStatus=(INT) dValore; break;
		default:
			return 1; // Errore la variabile non esiste
	}
	*/
	if (psScript->bVerbose) printf("da Vedere");
	return 0;
}


// --------------------------------------------------
// PRINT [ DESC, DATO ]
// Serve per vedere un dato durante la formula
//

static BOOL _scPrint(CHAR * lpFormulaSource,S_SCRIPT * psScript)
{
	CHAR *lpMemory;
	CHAR *lpStart;
	CHAR *lp;
	CHAR *lpPrefix;
	S_UNIVAL * psRet;

	lpMemory=strDup(lpFormulaSource);//ehAlloc(strlen(lpFormulaSource)+1);
	//   strcpy(lpMemory,lpFormulaSource);

	// if (fPromoDebug) printf("_scPrint" CRLF);

	//lpStart=scriptExtract(lpMemory,"[","]");
	lpStart=scriptParExtract(lpMemory);
	if (!lpStart)
	{
		if (psScript->bVerbose) printf(CRLF "Promo:Promo_Inserisci[]: Errore di sintassi A (%s)" CRLF,lpFormulaSource);
		ehFree(lpMemory); 
		return TRUE;
	} 


	// 
	// PREFISSO DI OUTPUT
	//
	lp=scriptNext(lpStart,',');
	if (lp==NULL) 
	{
		if (psScript->bVerbose) printf(CRLF "Promo:Promo_Inserisci[]: prezzo mancante (%s)" CRLF,lpFormulaSource);
		ehFree(lpMemory); return TRUE;
	} 
	*lp=0;

	lpStart=scriptExtract(lpStart,"\"","\"");
	lpPrefix=lpStart;

	// 
	// FORMULA
	//
	lpStart=lp+1;
	lp=scriptNext(lpStart,',');
	psRet=scriptGetValue(psScript,lpStart);
	if (psScript->bVerbose)  {
		if (!psRet)
		{
			printf("Errore in formula %s",lpStart);
		} 
		else
		{
			printf("> %s " ,lpPrefix);
			valPrint(psRet);	
			printf(CRLF);
		}
	}
	valDestroy(psRet);

	ehFree(lpMemory); 
	return FALSE; // TUTTO OK
}



CHAR * scriptExtract(CHAR *lpSource,CHAR *lpTesta,CHAR *lpCoda) // Isola
{
   CHAR *lpStart;
   CHAR *lpEnd;

   lpStart=strstr(lpSource,lpTesta); if (!lpStart) return NULL;
   lpStart++;
   lpEnd=strReverseStr(lpStart,lpCoda); if (!lpEnd) return NULL;
   *lpEnd=0;
   return lpStart;
}

// isolaPar
CHAR * scriptParExtract(CHAR *lpSource) 
{
   CHAR *lpStart;
   CHAR *lpEnd;

   lpStart=strstr(lpSource,"("); if (!lpStart) return NULL;
   lpStart++;
   lpEnd=_parClose(lpStart); if (!lpEnd) return NULL;
   //lpEnd=rstrstr(lpStart,lpCoda); if (!lpEnd) return NULL;
   *lpEnd=0;
   return lpStart;
}

CHAR * scriptNext(CHAR *lpStart,CHAR cCosa) // FindNext
{
	BOOL fString=0;
	for (;*lpStart;lpStart++)
	{
		if (*lpStart=='\"') fString^=1;
		if (fString) continue;
		if (*lpStart==cCosa) return lpStart;
	}
	return NULL;
}

/*
CHAR *QParChiudi(CHAR *p)
{
 //INT num=0;
 BOOL String=0;
 for (;*p;p++)
 {
	if (*p=='\"') String^=1;
	if (String) continue;

	if (*p==']') break;
 }
 if (String) return NULL;
 //if (num) return NULL;
 return p;
}
*/

static CHAR * _parClose(CHAR *p)
{
	 //INT num=0;
	 INT pa=1;
	 BOOL String=0;
	 for (;*p;p++)
	 {
		if (*p=='\"') String^=1;
		if (String) continue;

		if (*p=='(') {pa++; continue;}
		if (*p==')') pa--; 
		if (!pa) break;
	 }
	 if (pa) return NULL;
	 if (String) return NULL;
	 //if (num) return NULL;
	 return p;
}

//
// Function DANGER Area
//


// ------------------------------------------------------------------------------------------
// LEFT(STRING,VAL) = Calcola Somma periodo
//

static S_UNIVAL * _leftString(S_SCRIPT * psScript,CHAR * pszToken,CHAR * pszParams)
{
	CHAR * pszPrototype="LEFT(STRING$,LEN)";
	S_UNIVAL * psRet=NULL;
	S_UNIVAL * psVal=NULL;
	S_UNIVAL * psStr=NULL;
	INT iErrors=0;
	EH_LST lst=NULL;
	INT iLen;
	
	//
	// Cerco parametri funzione
	//
	lst=scriptFuncArg(pszParams?pszParams:pszToken,"(",")",&iErrors,NULL);

	if (iErrors||lst->iLength!=2) {
	
		scriptFuncDestroy(lst);
//		psScript->enError=SER_SINTAX;
//		sprintf(psScript->szError,"%s: errors | %s",pszToken,pszPrototype); 
		scriptError(psScript,SER_SINTAX,"'%s' unknown","%s: errors | %s",pszToken,pszPrototype); 
	    return NULL;
	
	}
	
	//
	// Leggo stringa
	//
	psStr=scriptFuncGet(psScript,lst,0,_TEXT);
	if (!psStr) {
//		psScript->enError=SER_SINTAX;
//		sprintf(psScript->szError,"%s: first param error | %s",pszToken,pszPrototype); 
		scriptError(psScript,SER_SINTAX,"%s: first param error | %s",pszToken,pszPrototype); 
	    return NULL;
	}

	//
	// Leggo Lunghezza
	//
	psVal=scriptFuncGet(psScript,lst,1,_NUMBER);
	if (!psVal) {
//		psScript->enError=SER_SINTAX;
//		sprintf(psScript->szError,"%s: second param error | %s",pszToken,pszPrototype); 
		scriptError(psScript,SER_SINTAX,"%s: second param error | %s",pszToken,pszPrototype); 
	    return NULL;
	}
	iLen=(INT) psVal->dValue;
	if (iLen>(INT) strlen(psStr->pszString)) iLen=strlen(psStr->pszString);
	psStr->pszString[iLen]=0;

	psRet=valCreateText(psStr->pszString);

	valDestroy(psVal);
	valDestroy(psStr);

	if (lst) scriptFuncDestroy(lst);
	return psRet; 

}


// ------------------------------------------------------------------------------------------
// RIGHT(STRING,VAL) = Calcola Somma periodo
//

static S_UNIVAL * _rightString(S_SCRIPT * psScript,CHAR * pszToken,CHAR * pszParams)
{
	CHAR * pszPrototype="LEFT(STRING$,LEN)";
	S_UNIVAL * psRet=NULL;
	S_UNIVAL * psVal=NULL;
	S_UNIVAL * psStr=NULL;
	INT iErrors=0;
	EH_LST lst=NULL;
	INT iLen;
	
	//
	// Cerco parametri funzione
	//
	lst=scriptFuncArg(pszParams?pszParams:pszToken,"(",")",&iErrors,NULL);

	if (iErrors||lst->iLength!=2) {
	
		scriptFuncDestroy(lst);
//		psScript->enError=SER_SINTAX;
//		sprintf(psScript->szError,"%s: errors | %s",pszToken,pszPrototype); 
		scriptError(psScript,SER_SINTAX,"%s: errors | %s",pszToken,pszPrototype); 
	    return NULL;
	
	}
	
	//
	// Leggo stringa
	//
	psStr=scriptFuncGet(psScript,lst,0,_TEXT);
	if (!psStr) {
//		psScript->enError=SER_SINTAX;
//		sprintf(psScript->szError,"%s: first param error | %s",pszToken,pszPrototype); 
		scriptError(psScript,SER_SINTAX,"%s: first param error | %s",pszToken,pszPrototype); 
	    return NULL;
	}

	//
	// Leggo Lunghezza
	//
	psVal=scriptFuncGet(psScript,lst,1,_NUMBER);
	if (!psVal) {
//		psScript->enError=SER_SINTAX;
//		sprintf(psScript->szError,"%s: second param error | %s",pszToken,pszPrototype); 
		scriptError(psScript,SER_SINTAX,"%s: second param error | %s",pszToken,pszPrototype); 
	    return NULL;
	}
	iLen=(INT) psVal->dValue;
	if (iLen>(INT) strlen(psStr->pszString)) iLen=strlen(psStr->pszString);
	iLen=strlen(psStr->pszString)-iLen;

	psRet=valCreateText(psStr->pszString+iLen);

	valDestroy(psVal);
	valDestroy(psStr);

	if (lst) scriptFuncDestroy(lst);
	return psRet; 

}



//
// ATOI(STRING) = Calcola Somma periodo
//

static S_UNIVAL * _stringToNum(S_SCRIPT * psScript,CHAR * pszToken,BOOL bInteger)
{
	CHAR * pszPrototype="ATOI(STRING)";
	S_UNIVAL * psRet=NULL;
	S_UNIVAL * psVal=NULL;
	S_UNIVAL * psStr=NULL;
	INT iErrors=0;
	EH_LST lst=NULL;
	
	//
	// Cerco parametri funzione
	//
	lst=scriptFuncArg(pszToken,"(",")",&iErrors,NULL);

	if (iErrors||lst->iLength!=1) {
	
		scriptFuncDestroy(lst);
		scriptError(psScript,SER_SINTAX,"%s: errors | %s",pszToken,pszPrototype); 
	    return NULL;
	
	}
	
	//
	// Leggo stringa
	//
	psStr=scriptFuncGet(psScript,lst,0,_TEXT);
	if (!psStr) {
		scriptError(psScript,SER_SINTAX,"%s: first param error | %s",pszToken,pszPrototype); 
	    return NULL;
	}
	if (bInteger) 
		psRet=valCreateNumber(atoi(psStr->pszString));
		else
		psRet=valCreateNumber(atof(psStr->pszString));

	valDestroy(psStr);

	if (lst) scriptFuncDestroy(lst);
	return psRet; 

}

