// -------------------------------------------------------------
// SoapClient.c
// Utilità per comunicazioni attraverso il protocollo SOAP
// ambiente virtuale di scripting per intefacciamento
// con variabili dinamiche a struttura complessa
//
// by Ferrà srl 7/2008
// da un idea di Giorgio Tassistro 
// Update Ferrà srl 12/2009 - Var.Array.Array bug
// Update Ferrà srl 02/2010 - Var.Array.Array bug
// -------------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/soap.h"

#define DISPLAY_NODE "+"
static CHAR *pSoapVersion="1.5.2"; 

//#define _APPEND_BUFFER psSoap->pszBuffer+strlen(psSoap->pszBuffer)
static void _xmlArrayShow(XMLARRAY arJolly);
static CHAR * soap_element_encode(EH_SOAP *psSoap,CHAR *pElement,CHAR *pUseMode);
static EH_SOAP_REQUEST *soap_request_builder(EH_SOAP *psSoap,CHAR *pScript);
static void *soap_request_info(EH_SOAP *psSoap,CHAR *pOperation);

static void *soap_request_sender(EH_SOAP *psSoap,EH_SOAP_REQUEST *psSoapRequest);
static void soap_childElements_free(EHS_ELEMENT *psElement);
void *L_ElementSemiClone(EHS_ELEMENT *psElementSource);
static INT _LEleToCnt(EHS_ELEMENT *psElement);

static BOOL soap_element_alloc(EHS_ELEMENT *psElement,EHS_CONTAINER *psContainer);
static BOOL soap_var_alloc(EHS_VAR *psSoapVar);
static XMLELEMENT *soap_operation_search(EH_SOAP *psSoap,CHAR *pName,CHAR *pszPortTypeName);

static XMLELEMENT * soap_complex_search(EH_SOAP *psSoap,CHAR *pTypeName);
static XMLELEMENT * soap_element_search(EH_SOAP *psSoap,CHAR *pTypeName);
static void soap_complexType_translate(EH_SOAP *psSoap,EHS_ELEMENT *psElement,XMLELEMENT *pxmlComplexType);
static void soap_complexContent_translate(EH_SOAP *psSoap,EHS_ELEMENT *psVarElement,XMLELEMENT *pxmlComplexContent);
static void soap_restriction_translate(EH_SOAP *psSoap,EHS_ELEMENT *psVarElement,XMLELEMENT *pxmlComplexContent);
static EH_SOAP_REQUEST *soap_requestFree(EH_SOAP_REQUEST *psSoapRequest);

//
// NS renaming (new 2010)
//
static void soap_ns_show(EH_SOAP *psSoap);
static CHAR *soap_ns_recoding(EH_SOAP *psSoap,
							  EN_NS_SEARCH enSearch, // 0=Namespace, 1=Address
							  CHAR *pChiave,INT idx);
static CHAR *soap_ns_getaddr(EH_SOAP *psSoap,CHAR *pNameSpace,INT idx);
//static CHAR *soap_nsFromXns(EH_SOAP *psSoap,XNS_PTR xns,XMLELEMENT *pxml);
static void soap_ns_reassign(EH_SOAP *psSoap,XNS_PTR xns,XMLELEMENT *pxml);
static INT soap_ns_search(EH_SOAP *psSoap,CHAR *pName,EN_NS_SEARCH enWhere);



EHS_ELEMENT_INFO *_LElementInfoGet(EH_SOAP *psSoap,CHAR *pScript,BOOL bReadMode);
void *_LElementInfoFree(EHS_ELEMENT_INFO *psSED);

// Variabili: elemento e container
static	EHS_ELEMENT *soap_var_free(EHS_VAR *psVar);
static	void soap_varptr_free(EHS_VARPTR *psVar);
static	EHS_ELEMENT *soap_element_free(EHS_ELEMENT *psElement);
static	EHS_CONTAINER * soap_container_free(EHS_CONTAINER *psContainer);
void	SoapOutputFile(EH_SOAP *psSoap,INT iCmd, BYTE *pFileName);
void	soap_var_display(EH_SOAP *psSoap,INT iMode,EHS_ELEMENT *psElement,EHS_CONTAINER *psContainer,CHAR *pAdd,CHAR *pIndex);
static	CHAR *_LIndentSpace(EH_SOAP *);
/*
const CHAR *pSoapEnvelope_Open=
	//"<?xml version=\"1.0\" encoding=\"@#ENCODING#@\" ?> " CRLF
	"Envelope @#NAMESPACELIST#@>";
*/
/*
const CHAR *pSoapEnvelopeClose=
	"</soap:Body>" CRLF
	"</soap:Envelope>";
	*/

static CHAR *strCTake(CHAR *pSource);
static BOOL SoapVariableToXml(EH_SOAP *psSoap,EHS_VARPTR *psVar);//,BYTE *nsParent);

//static void * XED_string(EH_SOAP *psSoap,XEDType cmd,CHAR *pTagName,CHAR *pTagValue,,EHS_ELEMENT *psElement);//CHAR *pTagType,CHAR *pTagValue,CHAR *pNameSpace);
static void * XED_string(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pszValue);// CHAR *pTagType,CHAR *pTagValue,CHAR *pNameSpace);
static void * XED_boolean(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pszValue);// CHAR *pTagName,CHAR *pTagType,CHAR *pTagValue,CHAR *pNameSpace);
static void * XED_int32(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pszValue);// CHAR *pTagName,CHAR *pTagType,CHAR *pTagValue,CHAR *pNameSpace);
#define XED_int64 XED_int32 // 64bit per ora non implementanto
static void * XED_float(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pszValue);// CHAR *pTagName,CHAR *pTagType,CHAR *pTagValue,CHAR *pNameSpace);
//static void * XED_decimal(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pszValue);// CHAR *pTagName,CHAR *pTagType,CHAR *pTagValue,CHAR *pNameSpace);

static void L_XMLAddElement(EH_SOAP *psSoap,
							CHAR *pNewNamespace, // Name space
							CHAR *pPartType,	 // Type
							CHAR *pPartName,	 // Name part
							CHAR *pValue,		 // Value
							CHAR *pBuffer,		
							INT iDeepSpace,
							INT iMode);

// _TagBuild()
// 0=Open con indent
// 1=Close con indent
// 2=Tag senza indent e <>
typedef enum {
	TAG_OPEN,
	TAG_CLOSE,
	TAG_CLOSE_NOINDENT,
	TAG_ONLY,
	TAG_NULL,
} enTagMode;

/*
static CHAR * _TagBuildXns(EH_SOAP *psSoap,enTagMode enMode,XNS_PTR xnsElement)
{
	_TagBuild(psSoap,enMode,xnsElement->pszSpace,CHAR *pTagName)
}
*/

//
// xnsGetAttrib()
// a) Legge un attributo da un Tag XML
// b) Estrae NS:Element
// c) Aggiorna il DB den NS usati e rinomina il NS
//
static XNS_PTR _L_GetAttrib(EH_SOAP *psSoap,CHAR *pAttrib,XMLELEMENT *pxml)
{
	XNS_PTR xns;
	xns=xnsCreate(xmlGetAttrib(pxml,pAttrib)); if (!xns) return NULL;
	soap_ns_reassign(psSoap,xns,pxml);
	return xns;
}

//
// _XNSToString
//
static CHAR * _XNSToString(EH_SOAP *psSoap,XNS_PTR xnsElement)
{
	BYTE *pszString;
	if (!xnsElement) return NULL;
	if (psSoap->sXmlRemove.bNameSpace)
	{
		pszString=strDup(xnsElement->pszElement);
	}
	else
	{
		pszString=xnsString(xnsElement);
	}
	if (!strEmpty(xnsElement->pszSpace)) soap_ns_search(psSoap,xnsElement->pszSpace,SEARCH_NS_NAME); // Incremento l'uso
	return pszString;
}

//
// _TagBuild
// Costruisce un Tag
//
static CHAR * _TagBuild(EH_SOAP *psSoap,enTagMode enMode,EHS_ELEMENT *psElement)//XNS_PTR xnsElement)//CHAR *pNameSpace,CHAR *pTagName)
{
	static CHAR szServ[800];
	BYTE *p;

	if (enMode!=TAG_ONLY&&enMode!=TAG_CLOSE_NOINDENT) strcpy(szServ,_LIndentSpace(psSoap)); else *szServ=0;
	if (enMode==TAG_OPEN||enMode==TAG_NULL) strcat(szServ,"<");
	if (enMode==TAG_CLOSE||
		enMode==TAG_CLOSE_NOINDENT) strcat(szServ,"</"); 

	p=_XNSToString(psSoap,psElement->xnsName); 
	strcat(szServ,p); 
	ehFree(p);

	// Aggiungo Attributi
	if (enMode!=TAG_CLOSE&&
		enMode!=TAG_CLOSE_NOINDENT)
	{
		BYTE *pType=_XNSToString(psSoap,psElement->xnsType); 
		if (pType) 
		{
			if (psSoap->enInputBodyUse==BODY_USE_ENCODED)
			{
				sprintf(strNext(szServ)," xsi:type=\"%s\"",pType);
			}
		}
		ehFreeNN(pType);
	}

	if (enMode==TAG_NULL) strcat(szServ," /");
	if (enMode!=TAG_ONLY) strcat(szServ,">");
	return szServ;
}


// http://www.xmethods.com/ve2/index.po (libreria di webservices)

//
// Definizione DataType #################################################################################
//

typedef enum {TP_UNKNOW=0, TP_NUMBER, TP_CSTRING, TP_OBJECT} EhsTokenType;
typedef struct {
	EhsTokenType iType;
	const CHAR *pTypeDesc;
} EHS_TOKEN_INFO;



static EHS_SIMPLEDATATYPE arSimpleDataType[]=
{
	// Primitivi > http://www.w3.org/TR/xmlschema-2/#built-in-primitive-datatypes
	{  1,"string",		FALSE,	NULL,FALSE,	XED_string},
	{  2,"boolean",		TRUE,	NULL,FALSE,	XED_boolean},
	{  3,"decimal",		TRUE,	NULL,FALSE,	XED_float},// XED_decimal},
	{  4,"float",		TRUE,	NULL,FALSE, XED_float},
	{  5,"double",		TRUE,	NULL,TRUE,NULL},
	{  6,"duration",	FALSE,	NULL,TRUE,NULL},
	{  7,"dateTime",	FALSE,	NULL,TRUE,NULL},
	{  8,"gYearMonth",	FALSE,	NULL,TRUE,NULL},
	{  9,"gYear",		FALSE,	NULL,TRUE,NULL},
	{ 10,"gMonthDay",	FALSE,	NULL,TRUE,NULL},
	{ 11,"gDay",		FALSE,	NULL,TRUE,NULL},
	{ 12,"gMonth",		FALSE,	NULL,TRUE,NULL},
	{ 13,"hexBinary",	FALSE,	NULL,TRUE,NULL},
	{ 14,"base64Binary",FALSE,	NULL,TRUE,NULL},
	{ 15,"anyURI",		FALSE,	NULL,TRUE,NULL},
	{ 16,"QName",		FALSE,	NULL,TRUE,NULL},
	{ 17,"NOTATION",	FALSE,	NULL,TRUE,NULL},

	// Derivati > http://www.w3.org/TR/xmlschema-2/#built-in-derived
	{101,"normalizedString",	FALSE,"string",TRUE,NULL},
	{102,"token",				FALSE,"normalizedString",TRUE,NULL},
	{103,"language",			FALSE,"token",TRUE,NULL},
	{104,"NMTOKEN",				FALSE,"token",TRUE,NULL},
	{105,"NMTOKENS",			FALSE,"NMTOKEN",TRUE,NULL},
	{106,"Name",				FALSE,"token",TRUE,NULL},
	{107,"NCName",				FALSE,"Name",TRUE,NULL},
	{108,"ID",					FALSE,"NCName",TRUE,NULL},
	{109,"IDREF",				FALSE,"NCName",TRUE,NULL},
	{110,"IDREFS",				FALSE,"IDREF",TRUE,NULL},
	{111,"ENTITY",				FALSE,"NCName",TRUE,NULL},
	{112,"ENTITIES",			FALSE,"ENTITY",TRUE,NULL},
	{113,"integer",				TRUE,"decimal",FALSE,XED_int32},
	{114,"nonPositiveInteger",	TRUE,"integer",TRUE,NULL},
	{115,"negativeInteger",		TRUE,"nonPositiveInteger",TRUE,NULL},
	{116,"long",				TRUE,"integer",FALSE,XED_int64},
	{117,"int",					TRUE,"long",FALSE,XED_int32},
	{118,"short",				TRUE,"int",TRUE,NULL},
	{119,"byte",				TRUE,"short",TRUE,NULL},
	{120,"nonNegativeInteger",	TRUE,"integer",TRUE,NULL},
	{121,"unsignedLong",		TRUE,"nonNegativeInteger",TRUE,NULL},
	{122,"unsignedInt",			TRUE,"unsignedLong",TRUE,NULL},
	{123,"unsignedShort",		TRUE,"unsignedInt",TRUE,NULL},
	{124,"unsignedByte",		TRUE,"unsignedShort",TRUE,NULL},
	{125,"positiveInteger",		TRUE,"nonNegativeInteger",TRUE,NULL},
	{0},
};

//
//  soap_SimpleTypeInfo()
//  Ritorna il puntatore alla struttura di definizione se il dato è semplice
//  NULL se non rientra tra i dati semplici
//
static EHS_SIMPLEDATATYPE * soap_SimpleTypeInfo(CHAR *pName)
{
	INT a;
	for (a=0;arSimpleDataType[a].id;a++)
	{
		if (!strcmp(arSimpleDataType[a].pName,pName)) return &arSimpleDataType[a];
	}
	return NULL;
}

static EhsTokenType L_NumberString(CHAR *pType)
{
	EHS_SIMPLEDATATYPE *psSDT;
	psSDT=soap_SimpleTypeInfo(pType); if (!psSDT) return TP_UNKNOW;
	if (psSDT->bNumber) return TP_NUMBER; else return TP_CSTRING;
}

EHS_SIMPLEDATATYPE * soap_SimpleDataTypeCheck(CHAR *pTypeName,CHAR *pInfo)
{
	EHS_SIMPLEDATATYPE *psSDT=soap_SimpleTypeInfo(pTypeName); 
	if (!psSDT) ehExit("Tipo non riconosciuto: '%s' o non implementato (%s)",pTypeName,pInfo);
	if (psSDT->bLocked) ehExit("TipoSemplice: '%s' non implementato (%s)",pTypeName,pInfo);
	return psSDT;
}

//
// Xml Encoding/Decoding  Functions
//
// Da aggiungere encoding e decoding utf-8
//
static void * XED_string(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pszValue) {

	BYTE *pTagHtmlValue,*pszTagOpen,*pszTagClose;
	void *pReturn=NULL;
	switch (cmd)
	{
		case SE_VAR_TO_XML: // VAR > XML
			if (strEmpty(pszValue))
			{
				pszTagOpen=strDup(_TagBuild(psSoap,TAG_NULL,psElement));//pNameSpace,pTagName));
				ARAddarg(&psSoap->arXML,"%s",pszTagOpen);
				ehFree(pszTagOpen);
			}
			else
			{
				pszTagOpen=strDup(_TagBuild(psSoap,TAG_OPEN,psElement));//pNameSpace,pTagName));
				pszTagClose=strDup(_TagBuild(psSoap,TAG_CLOSE_NOINDENT,psElement));//pNameSpace,pTagName));
				if (!psSoap->bUtf8Mode) 
						pTagHtmlValue=strEncode(pszValue,SE_ISO_LATIN1,NULL); 
						else 
						pTagHtmlValue=strEncode(pszValue,SE_HTML_XML,NULL);

					ARAddarg(&psSoap->arXML,"%s%s%s",
							pszTagOpen,
							pTagHtmlValue,
							pszTagClose);
					
				ehFree(pTagHtmlValue);
				ehFree(pszTagOpen);
				ehFree(pszTagClose);
			}
			break;

		case SE_VAR_TO_STR: // Var > STR
			if (pszValue)
			{
				pReturn=strDup(pszValue);
			}
			break;

		case SE_XML_TO_VAR: // XML > string
			if (pszValue) 
			{
					pReturn=ehAlloc(strlen(pszValue)+3);
					sprintf(pReturn,"'%s'",pszValue);
			}
			break;
	}
	return pReturn;
}

//
// XED_boolean()
//
static void * XED_boolean(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pTagValue)
{
	BOOL bFlag;
	CHAR *pValue;
	INT iValue;
	void *pReturn=NULL;
	BYTE *pszTagOpen,*pszTagClose;

	switch (cmd)
	{
		case SE_VAR_TO_XML: // String > XML

			if (strEmpty(pTagValue)) bFlag=FALSE; else bFlag=atoi(pTagValue);
			pszTagOpen=strDup(_TagBuild(psSoap,TAG_OPEN,psElement));//pNameSpace,pTagName));
			pszTagClose=strDup(_TagBuild(psSoap,TAG_CLOSE_NOINDENT,psElement));//pNameSpace,pTagName));
			ARAddarg(&psSoap->arXML,"%s%s%s",
					pszTagOpen,
					bFlag?"true":"false",
					pszTagClose);
				
			ehFree(pszTagOpen);
			ehFree(pszTagClose);
			break;
/*
			pszTagName=strDup(_TagBuild(psSoap,TAG_ONLY,psElement));//pNameSpace,pTagName));
			pszElement=xnsString(psElement->xnsName);
			pszType=xnsString(psElement->xnsType);

			if (strEmpty(pTagValue)) bFlag=FALSE; else bFlag=atoi(pTagValue);
			if (bFlag) pValue="true"; else pValue="false";
			if (pszType)
			{
				ARAddarg(&psSoap->arXML,"%s<%s xsi:type=\"%s\">%s</%s>",
						_LIndentSpace(psSoap),
						pszTagName,
						pszType,
						pValue,
						pszTagName);
			}
			else
			{
				ARAddarg(&psSoap->arXML,"%s<%s>%s</%s>",
						_LIndentSpace(psSoap),
						pszTagName,
						pValue,
						pszTagName);
			}
			ehFreeNN(pszTagName);
			ehFreeNN(pszElement);
			ehFreeNN(pszType);
			break;
			*/

		case SE_VAR_TO_STR: // Var > STR
			if (pTagValue)
			{
				if (strEmpty(pTagValue)) bFlag=FALSE; else bFlag=atoi(pTagValue);
				if (bFlag) pValue="true"; else pValue="false";
				pReturn=strDup(pValue);
			}
			break;

		case SE_XML_TO_VAR: // XML > Var
			pReturn=ehAlloc(20);
			iValue=0;
			if (pTagValue)
			{
				if (!strcmp(pTagValue,"false")||!strcmp(pTagValue,"0")) iValue=0;
				if (!strcmp(pTagValue,"true")||!strcmp(pTagValue,"1")) iValue=1;
			}
			sprintf(pReturn,"%d",iValue);
			break;
	}
	return pReturn;
}

//
// XED_int32()
//
static void * XED_int32(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pTagValue)
{
	INT iValue;
	void *pReturn=NULL;
	BYTE *pszTagOpen,*pszTagClose;

	switch (cmd)
	{
		case SE_VAR_TO_XML: // String > XML
			
			if (strEmpty(pTagValue)) iValue=0; else iValue=atoi(pTagValue);
			pszTagOpen=strDup(_TagBuild(psSoap,TAG_OPEN,psElement));//pNameSpace,pTagName));
			pszTagClose=strDup(_TagBuild(psSoap,TAG_CLOSE_NOINDENT,psElement));//pNameSpace,pTagName));
			ARAddarg(&psSoap->arXML,"%s%d%s",
					pszTagOpen,
					iValue,
					pszTagClose);
				
			ehFree(pszTagOpen);
			ehFree(pszTagClose);
			break;

		case SE_VAR_TO_STR: // Var > STR
			if (pTagValue)
			{
				pReturn=ehAlloc(60);
				sprintf(pReturn,"%d",atoi(pTagValue));
			}
			break;


		case SE_XML_TO_VAR: // String > XML
			if (pTagValue)
			{
				pReturn=ehAlloc(60);
				sprintf(pReturn,"%d",atoi(pTagValue));
			}
			break;
	}
	return pReturn;
}

//
// XED_float()
//
static void * XED_float(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pTagValue)
{
//	BOOL bFlag;
	double dValue;
	BYTE *pReturn=NULL;
	BYTE *pszTagOpen,*pszTagClose;

	switch (cmd)
	{
		case SE_VAR_TO_XML: // String > XML

			if (strEmpty(pTagValue)) dValue=0; else dValue=atof(pTagValue);
			pszTagOpen=strDup(_TagBuild(psSoap,TAG_OPEN,psElement));//pNameSpace,pTagName));
			pszTagClose=strDup(_TagBuild(psSoap,TAG_CLOSE_NOINDENT,psElement));//pNameSpace,pTagName));
			ARAddarg(&psSoap->arXML,"%s" SOAP_FLOAT_FORMAT "%s",
					pszTagOpen,
					dValue,
					pszTagClose);
				
			ehFree(pszTagOpen);
			ehFree(pszTagClose);
			break;

		case SE_VAR_TO_STR: // Var > STR
			if (pTagValue)
			{
				dValue=atof(pTagValue);
				pReturn=ehAlloc(60);
				sprintf(pReturn,SOAP_FLOAT_FORMAT,dValue);
			}
			break;

		case SE_XML_TO_VAR: // String > XML
			pReturn=ehAlloc(60);
			dValue=atof(pTagValue);
			sprintf(pReturn,SOAP_FLOAT_FORMAT,dValue);
			break;
	}
	return pReturn;
}
//
// XED_decimal
//
/*
static void * XED_decimal(EH_SOAP *psSoap,XEDType cmd,EHS_ELEMENT *psElement,BYTE *pTagValue)
{
//	BOOL bFlag;
	double dValue;
	BYTE *pReturn=NULL;
	BYTE *pszType,*pszTagName;

	switch (cmd)
	{
		case SE_VAR_TO_XML: // String > XML

			pszTagName=strDup(_TagBuild(psSoap,TAG_ONLY,psElement));//pNameSpace,pTagName));
			pszType=xnsString(psElement->xnsType);
			if (strEmpty(pTagValue)) dValue=0; else dValue=atof(pTagValue);
			if (pszType)
			{
				ARAddarg(&psSoap->arXML,"%s<%s xsi:type=\"%s\">" SOAP_FLOAT_FORMAT "</%s>",
						_LIndentSpace(psSoap),
						pszTagName,
						pszType,
						dValue,
						pszTagName);
			}
			else
			{
				ARAddarg(&psSoap->arXML,"%s<%s>" SOAP_FLOAT_FORMAT "</%s>",
						_LIndentSpace(psSoap),
						pszTagName,
						dValue,
						pszTagName);
			}
			ehFreeNN(pszTagName);
			ehFreeNN(pszType);
			break;

		case SE_VAR_TO_STR: // Var > STR
			if (pTagValue)
			{
				dValue=atof(pTagValue);
				pReturn=ehAlloc(60);
				sprintf(pReturn,SOAP_FLOAT_FORMAT,dValue);
			}
			break;

		case SE_XML_TO_VAR: // String > XML
			pReturn=ehAlloc(60);
			dValue=atof(pTagValue);
			sprintf(pReturn,SOAP_FLOAT_FORMAT,dValue);
			break;
	}
	return pReturn;
}
*/
static INT _LEleToCnt(EHS_ELEMENT *psElement)
{
	if (psElement->iArrayType)
	{
		return CTE_ARRAY;
	}


	switch (psElement->iType)
	{
		case STE_SIMPLE:  
		case STE_DEFINE:  
			return CTE_SIMPLE;

		case STE_ELEMENT:
		case STE_COMPLEX: return CTE_COMPLEX;

	//	case STE_ARRAY:   
			return CTE_ARRAY;
	}

	return CTE_EMPTY;
}


//
// XML_arrayType()
//
/*
static void XML_arrayType(EH_SOAP *psSoap,EHS_VARPTR *psVar)
{
//	BOOL bFlag;
	BYTE *pszTagName;
	BYTE *pNameSpace=""; // ?? da vedere
	BYTE *parrayTageName=""; // ?? da vedere
	CHAR szServ[200];
	BYTE *pszArrayType,*pszArrayElement;

	pszTagName=strDup(_TagBuild(psSoap,TAG_ONLY,psVar->psElement));//_LNSTagName(pNameSpace,pTagName);

	pszArrayType=_XNSToString(psSoap,psVar->psElement->xnsArrayType);
	pszArrayElement=_XNSToString(psSoap,psVar->psElement->xnsType);
	sprintf(szServ,
			"<%s %s=\"%s[%d]\">",
			pszTagName,
			pszArrayType,
			pszArrayElement,
			psVar->psContainer->iLength);
	ehFree(pszArrayType);
	ehFree(pszArrayElement);

	ARAddarg(&psSoap->arXML,"%s%s",_LIndentSpace(psSoap),szServ);
	ehFree(pszTagName);

}
*/

//
// soap_element_alloc() (* funzione ricorsiva)
// Alloca memoria per contenere i dati nell'elemento indicato
//
BOOL soap_var_alloc(EHS_VAR *psSoapVar) {return soap_element_alloc(&psSoapVar->sElement,&psSoapVar->sContainer);}
BOOL soap_element_alloc(EHS_ELEMENT *psElement,EHS_CONTAINER *psContainer)
{
	EHS_CONTAINER *arContainer;
	EHS_ELEMENT *arsElement;
	INT a;
	BOOL bError=FALSE;
	
	//psContainer->iCntType=psElement->iType;
	psContainer->iCntType=_LEleToCnt(psElement);
	psContainer->iElementType=psElement->iType;

	switch (psContainer->iCntType)
	{
		case CTE_SIMPLE:
			psContainer->iLength=0;
			psContainer->pValue=NULL;//strDup(""); // Elemento singolo (stringa)
			break;

		case CTE_COMPLEX:
			if (!psElement->iElements) break;

			arsElement=psElement->arsElement;
			psContainer->iLength=psElement->iElements;
			arContainer=ehAllocZero(sizeof (EHS_CONTAINER) * psContainer->iLength);
			psContainer->pValue=arContainer;
			for (a=0;a<psElement->iElements;a++)
			{
				bError|=soap_element_alloc(&arsElement[a],&arContainer[a]); // Lancio Me stesso per allocare gli altri elementi
			}
			break;

		case CTE_ARRAY:
			psContainer->iLength=0;
			psContainer->pValue=ARNew();// // Array di stringhe
			break;

		default: 
			//printf("Tipo sconosciuto");
			bError=TRUE;
			// ehError();
			break;
	}
	return bError;
}

//
// SoapRequest()
//
void *SoapRequest(EH_SOAP *psSoap,CHAR *pStr,...)
{
	va_list Ah;
	CHAR *pRet,*lpRequest;

	lpRequest=ehAlloc(1024);
	va_start(Ah,pStr);
	vsprintf(lpRequest,pStr,Ah); // Messaggio finale
	va_end(Ah);
	pRet=SoapClient(psSoap, WS_PROCESS, SOAP_REQUEST, lpRequest);
	ehFree(lpRequest); 

	return pRet;
}


//
// SoapClient()
//
void *SoapClient(EH_SOAP *psSoap,INT cmd,INT info,void *ptr)
{
	EH_AR arRet;
	INT a,b;
	void *pReturn=NULL;
	EH_SOAP_REQUEST *psSoapRequest;
	
	switch (cmd)
	{
		case WS_OPEN: // Apre un gestore SOAP CLIENT basandosi su un WSDL
			//
			//	<definitions>
			//		<types />
			//		<message />
			//		<portType />
			//		<binding />
			//		<service />
			//	</definitions>
			//

			ZeroFill(psSoap->xmlWSDL); 
			psSoap->xmlWSDL.bUseNamespace=TRUE;
			xmlParser(&psSoap->xmlWSDL,WS_OPEN,XMLOPEN_FILE,ptr); 
			psSoap->pszWSDLSource=strDup(ptr);

			// Definizioni generali
			psSoap->pxmlDefinition=xmlParser(&psSoap->xmlWSDL,WS_FIND,0,"definitions"); // <--- prendo i <service />
			psSoap->pTargetNamespace=xmlGetAttribAlloc(psSoap->pxmlDefinition,"targetNamespace",NULL);

			psSoap->arSchema=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,0,"definitions.types.schema");
			psSoap->arMessage=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,0,"definitions.message");	// <--- prendo i <message />
			psSoap->arPortType=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,0,"definitions.portType"); // <--- prendo i <portType />
			psSoap->arBinding=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,0,"definitions.binding");	// <--- prendo i <binding />
			psSoap->arService=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,0,"definitions.service");	// <--- prendo i <service />

			SoapVariable(psSoap,WS_OPEN,NULL);

			DMIReset(&psSoap->dmiNamespace);
			DMIOpen(&psSoap->dmiNamespace,RAM_AUTO,20,sizeof(EHS_NS),"SoapNS");
			DMIReset(&psSoap->dmiCookie);
			webCookieCreate(&psSoap->dmiCookie);
			psSoap->bReady=TRUE;

			psSoap->pszLastOperation=strDup("");
			psSoap->pszLastOperationTime=strDup("");

			soap_ns_clean(psSoap);
			if (strEmpty(psSoap->pszSoapEnvName)) psSoap->pszSoapEnvName="soapenv";
			soap_ns_add(psSoap,psSoap->pszSoapEnvName,"http://schemas.xmlsoap.org/soap/envelope/",0);
			soap_ns_add(psSoap,"xsi","http://www.w3.org/2001/XMLSchema-instance",0);
			soap_ns_add(psSoap,"xsd","http://www.w3.org/2001/XMLSchema",0);
			break;

		case WS_CLOSE: // Libero le risorse
			psSoap->arSchema=xmlArrayFree(psSoap->arSchema); 
			psSoap->arMessage=xmlArrayFree(psSoap->arMessage); 
			psSoap->arPortType=xmlArrayFree(psSoap->arPortType); 
			psSoap->arBinding=xmlArrayFree(psSoap->arBinding); 
			psSoap->arService=xmlArrayFree(psSoap->arService); 
			ehFreePtr(&psSoap->pTargetNamespace);
			xmlParser(&psSoap->xmlWSDL,WS_CLOSE,0,NULL); /* carico l'ultima response sul file 'lastResponse.xml'*/

			SoapVariable(psSoap,WS_DESTROY,NULL);
			DMIClose(&psSoap->dmiVar,"SoapVariable");
	
			soap_ns_clean(psSoap);
			DMIClose(&psSoap->dmiNamespace,"SoapNS");

			webCookieDestroy(&psSoap->dmiCookie);

			ehFreePtr(&psSoap->pszLastDataResponse);
			ehFreePtr(&psSoap->pszLastHeaderResponse);
			ehFreePtr(&psSoap->pszWSDLSource);
			ehFreePtr(&psSoap->pszLastOperation);
			ehFreePtr(&psSoap->pszLastOperationTime);
			psSoap->arXML=ARDestroy(psSoap->arXML); 

			//memset(psSoap,0,sizeof(EH_SOAP));
			break;

		//
		// Richiesta di una elaborazione
		//
		case WS_PROCESS:

			switch (info)
			{
				//
				// Elenco delle operazioni possibili
				//
				case SOAP_ARRAY_OPERATION:

					arRet=ARCreate(NULL,NULL,NULL);
					for (a=0;psSoap->arPortType[a];a++)
					{
						XMLARRAY arOperation=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,psSoap->arPortType[a]->idx,".operation");
						for (b=0;arOperation[b];b++)
						{
							BYTE *pName=xmlGetAttrib(arOperation[b],"name");
							ARAdd(&arRet,pName);
						}
						arOperation=xmlArrayFree(arOperation);  
					}
					pReturn=arRet;
					break;

				//
				// Costruisce una richiesta Internet di una determinata operazione
				//
				case SOAP_REQUEST_PREPARE:
					pReturn=soap_request_builder(psSoap,ptr);
					break;

				//
				// Spedisce la richiesta al WebService
				//
				case SOAP_REQUEST_SEND:
					pReturn=soap_request_sender(psSoap,ptr);
					break;

				//
				// Costruisce una richiesta Internet di una determinata operazione
				// Ritorna NULL se ci sono stati problemi
				//
				case SOAP_REQUEST:
					strAssign(&psSoap->pszLastOperation,ptr);
					strAssign(&psSoap->pszLastOperationTime,dtNow());

					psSoapRequest=soap_request_builder(psSoap,ptr); // Trasforma la funzione in richiesta
					if (psSoapRequest) pReturn=soap_request_sender(psSoap,psSoapRequest); // Effettua chiamata SOAP
					break;

				default:
					ehError();
			}
			break;

		//
		// Richiesta di una elaborazione
		//
		case WS_DISPLAY:
			SoapOutputFile(psSoap,WS_OPEN,ptr);
			psSoap->dwDisplayParam=(cmd==WS_DISPLAY)?1:0;

			fprintf(psSoap->chOutput,"Operation List ______________" CRLF);
			fprintf(psSoap->chOutput,"WSDL: %s" CRLF,psSoap->pszWSDLSource);
			fprintf(psSoap->chOutput, CRLF);
			arRet=SoapClient(psSoap,WS_PROCESS,SOAP_ARRAY_OPERATION,NULL);
			for (a=0;arRet[a];a++)
			{
				soap_request_info(psSoap,arRet[a]);
				fprintf(psSoap->chOutput,CRLF "------------------------------- " CRLF);
			}
			ARDestroy(arRet);
			SoapOutputFile(psSoap,WS_CLOSE,NULL);
			break;
	}
	return pReturn;
}


// "http://wst.mobyws.it/MobyWebService/MobyService?WSDL"
//
// RPC - Remote Procedure Call (chiamata ad una procedura remota)
//
	// Dove spiegano come è fatto
	// http://en.wikipedia.org/wiki/Web_Services_Description_Language
	//
	
	// Dove scarico il WSDL di Moby
	// https://wsp.mobyws.it/MobyWebService/MobyService?WSDL
	//

	// Dove scarico il WSDL di Maitaly
	// http://www.maitaly.it/maws20/default.asmx?WSDL
	//

	// Problemi da risolvere
	// 1) Comunicare in SSL FATTO !
	// 2) Interpretare WSDL FATTO !
	// 3) Costruire un interfaccia per la trasformazione della domanda "inbustata" in Soap FATTO!
	//


//
// Web Services Description Language
//
BOOL soap_WSDL_GetAndSave(CHAR *pUriAddress,CHAR *pLocalFileSave)
{
	//FWEBSERVER sWS;
	EH_WEB sWeb,*psWeb;
	//BOOL fCheck;

	//MicrosoftTest(); return;

	//ZeroFill(sWS);
	_(sWeb);
	strcpy(sWeb.szReq,"GET");
	sWeb.lpUri=pUriAddress;
	sWeb.lpUserAgent=SOAP_USER_AGENT;

#ifdef _SSL_NO_AUTHENTICATION
	eh_ssl_client_context(&sWS,
		_SSL_NO_AUTHENTICATION,	/* use SOAP_SSL_DEFAULT in production code */
		NULL, 		/* keyfile: required only when client must authenticate to server (see SSL docs on how to obtain this file) */
		NULL, 		/* password to read the keyfile */
		NULL,		/* optional cacert file to store trusted certificates */
		NULL,		/* optional capath to directory with trusted certificates */
		NULL		/* if randfile!=NULL: use a file with random data to seed randomness */ 
		);
#endif

	//fCheck=FWebGetDirect(&sWS,"GET",NULL);
	psWeb=webHttpReqEx(&sWeb,false);
 	if (psWeb->sError.enCode)
	{
		printf("in download: %s (Error %d)" CRLF,pUriAddress,psWeb->sError.enCode);//,sWS.pPageHeader,strlen(sWS.pPageHeader));
		webHttpReqFree(psWeb);
		return TRUE;
	}

//	printf("%s",sWS.pData);
	if (fileStrWrite(pLocalFileSave,psWeb->pData)) 
		printf("Errore in filesave: %s (Error %d)" CRLF,pLocalFileSave,GetLastError());//,sWS.pPageHeader,strlen(sWS.pPageHeader));
	//FWebGetFree(&sWS);  
	webHttpReqFree(psWeb);
	//waitKey();
	return FALSE;
}

static BOOL strCString(CHAR *pString)
{
	INT iEnd=strlen(pString)-1;
	if (iEnd<0) return FALSE;
	if ((pString[0]=='\''&&pString[iEnd]=='\'')||(pString[0]=='\"'&&pString[iEnd]=='\"')) return TRUE;
	return FALSE;
}


void L_WhatIsThis(EH_SOAP *psSoap,CHAR *pToken,EHS_TOKEN_INFO *psTokenInfo)
{
	EHS_ELEMENT_INFO *psSED=NULL;

	memset(psTokenInfo,0,sizeof(EHS_TOKEN_INFO));
	psTokenInfo->iType=TP_UNKNOW;
	psTokenInfo->pTypeDesc="unknow ?";
	while (TRUE)
	{
		if (strOnlyNumber(pToken)) {psTokenInfo->iType=TP_NUMBER; psTokenInfo->pTypeDesc="const number"; break;}
		if (strCString(pToken)) {psTokenInfo->iType=TP_CSTRING; psTokenInfo->pTypeDesc="const string"; break;}
		psSED=_LElementInfoGet(psSoap,pToken,TRUE); 
		if (psSED->iErrorCode==SVE_OK) 
		{
			psTokenInfo->iType=TP_OBJECT; psTokenInfo->pTypeDesc="object";
			break;
		}
		break;
	}
	if (psSED) psSED=_LElementInfoFree(psSED);
}


//
// soap_request_info()
// Analizza WSDL e ritorna informazioni
//
static void *soap_request_info(EH_SOAP *psSoap,CHAR *pOperation)
{
	XMLELEMENT *pxmlOperation=NULL;
	CHAR szPortTypeName[100];
	XMLELEMENT *pxmlDoc=NULL;
	XMLELEMENT *pxmlInput=NULL;
	XMLELEMENT *pxmlOutput=NULL;
	XMLELEMENT *pxmlMessage=NULL;

	CHAR *pszMessageInput;
	CHAR *pszMessageOutput;
	XMLARRAY	arMessageInp=NULL;
	XMLARRAY	arMessageOut=NULL;
	
	XMLNAMESPACE sNSName;
	INT iMaxPart=0;
	BYTE *pPartName,*pPartType,*pPartElement;
	INT nInput,nOutput;

	pxmlOperation=soap_operation_search(psSoap,pOperation,szPortTypeName);
	if (!pxmlOperation) ehExit("soap operation '%s' sconosciuta",pOperation);

	//
	// Cerco <message> di input e ouput
	//
	pxmlInput=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".input"); 
	pxmlOutput=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".output"); 
	pxmlDoc=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".documentation"); 

	//
	// Cerro array di Input e di output
	//
	pszMessageInput=xmlGetAttribAlloc(pxmlInput,"message",NULL);
	xmlNSExtract(pszMessageInput,&sNSName);
	pxmlMessage=xmlElementWithAttribute(psSoap->arMessage,"name",sNSName.szName,TRUE);
	arMessageInp=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlMessage->idx,".part"); // <--- prendo i <service />
	if (arMessageInp) nInput=xmlArrayLen(arMessageInp); else nInput=0;
	pxmlMessage=xmlElementFree(pxmlMessage);

	pszMessageOutput=xmlGetAttribAlloc(pxmlOutput,"message",NULL);
	xmlNSExtract(pszMessageOutput,&sNSName);
	pxmlMessage=xmlElementWithAttribute(psSoap->arMessage,"name",sNSName.szName,TRUE);
	arMessageOut=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlMessage->idx,".part"); // <--- prendo i <service />
	if (arMessageOut) nOutput=xmlArrayLen(arMessageOut); else nOutput=0;
	pxmlMessage=xmlElementFree(pxmlMessage);

	//
	// OPERATION - funzione ()
	//
	if (!nOutput) 
		fprintf(psSoap->chOutput,"void");
//	else if (nOutput==1) fprintf(psSoap->chOutput,"result");
		else 
		{
			for (iMaxPart=0;arMessageOut[iMaxPart];iMaxPart++)
			{
				XNS_PTR xnsPart;
				pPartName=xmlGetAttribAlloc(arMessageOut[iMaxPart],"name",NULL);
				xnsPart=xnsCreate(pPartName);
				if (iMaxPart) fprintf(psSoap->chOutput,", ");
				fprintf(psSoap->chOutput,"%s",xnsPart->pszElement);
				ehFree(pPartName);
				xnsDestroy(xnsPart);
			}
		}
	fprintf(psSoap->chOutput,"\t");

	//
	// Visualizzo la operation (funzione)
	//
	fprintf(psSoap->chOutput,"%s(",pOperation);
	if (arMessageInp) 
	{
		for (iMaxPart=0;arMessageInp[iMaxPart];iMaxPart++)
		{
			/*
			if (iMaxPart) fprintf(psSoap->chOutput,", ");
			pPartName=xmlGetAttribAlloc(arMessageInp[iMaxPart],"name",NULL);
			fprintf(psSoap->chOutput,"%s",pPartName);
			ehFree(pPartName);
			*/
			XNS_PTR xmlName;
			pPartName=xmlGetAttribAlloc(arMessageInp[iMaxPart],"name",NULL);
			xmlName=xnsCreate(pPartName);
			if (iMaxPart) fprintf(psSoap->chOutput,", ");
			fprintf(psSoap->chOutput,"%c_%s",'a'+iMaxPart,xmlName->pszElement);
			ehFree(pPartName);
			xnsDestroy(xmlName);

		}

	} else fprintf(psSoap->chOutput,"void");

	fprintf(psSoap->chOutput,")" );
	if (pxmlDoc) fprintf(psSoap->chOutput, " // %s",pxmlDoc->lpValue);
	fprintf(psSoap->chOutput,CRLF CRLF);

	//
	// Visualizzo le parti del messaggio di output (dati di ritorno)
	//
	if (arMessageOut) 
	{
		fprintf(psSoap->chOutput," return: " CRLF);

		for (iMaxPart=0;arMessageOut[iMaxPart];iMaxPart++)
		{
			XNS_PTR xmlPartName,xmlPartType,xmlPartElement;
			pPartName=xmlGetAttribAlloc(arMessageOut[iMaxPart],"name",NULL); xmlPartName=xnsCreate(pPartName);
			pPartType=xmlGetAttribAlloc(arMessageOut[iMaxPart],"type",NULL); xmlPartType=xnsCreate(pPartType); // Usato per i tipi semplici e complessi
			pPartElement=xmlGetAttribAlloc(arMessageOut[iMaxPart],"element",NULL); xmlPartElement=xnsCreate(pPartElement); // Usato per gli elementi
			
			//xmlNSExtract(pPartName,&sNSPartName);

			// fprintf(psSoap->chOutput,"\t%s\t= ",sNSPartName.szName); 
			fprintf(psSoap->chOutput,"\t");
			if (pPartElement)
			{
				//xmlNSExtract(pPartElement,&sNSPartType);
				fprintf(psSoap->chOutput,"element %s",xmlPartElement->pszElement);
			}
			else
			{
				EHS_SIMPLEDATATYPE *psSDT;
				psSDT=soap_SimpleTypeInfo(xmlPartType->pszElement); 
				fprintf(psSoap->chOutput,"%s%s",!psSDT?"complex ":"",xmlPartType->pszElement);
			}
			
			fprintf(psSoap->chOutput," %s",xmlPartName->pszElement);
			fprintf(psSoap->chOutput,CRLF);

			ehFree(pPartName); xnsDestroy(xmlPartName);
			ehFree(pPartType); xnsDestroy(xmlPartType);
			ehFree(pPartElement); xnsDestroy(xmlPartElement);
		}
	} 

	//
	// Visualizzo le parti del messaggio di input (argomenti)
	//

	if (arMessageInp) 
	{
		fprintf(psSoap->chOutput," args: " CRLF);
		for (iMaxPart=0;arMessageInp[iMaxPart];iMaxPart++)
		{
			XNS_PTR xmlPartName,xmlPartType,xmlPartElement;
			pPartName=xmlGetAttribAlloc(arMessageInp[iMaxPart],"name",NULL); xmlPartName=xnsCreate(pPartName);
			pPartType=xmlGetAttribAlloc(arMessageInp[iMaxPart],"type",NULL); xmlPartType=xnsCreate(pPartType); // Usato per i tipi semplici e complessi
			pPartElement=xmlGetAttribAlloc(arMessageInp[iMaxPart],"element",NULL); xmlPartElement=xnsCreate(pPartElement); // Usato per gli elementi

			//xmlNSExtract(pPartElement,&sNSPartType);
			//xmlNSExtract(pPartName,&sNSPartName);

//			fprintf(psSoap->chOutput,"\t%c_%s\t= ",'a'+iMaxPart,pPartName);
			fprintf(psSoap->chOutput,"\t");
			if (pPartElement)
			{
				fprintf(psSoap->chOutput,"element %s",xmlPartElement->pszElement);
			}
			else
			{
				EHS_SIMPLEDATATYPE *psSDT;
				psSDT=soap_SimpleTypeInfo(xmlPartType->pszElement); 
				fprintf(psSoap->chOutput,"%s%s",!psSDT?"complex ":"",xmlPartType->pszElement);
			}
			fprintf(psSoap->chOutput," %c_%s",'a'+iMaxPart,xmlPartName->pszElement);
			fprintf(psSoap->chOutput,CRLF);

			ehFree(pPartName); xnsDestroy(xmlPartName);
			ehFree(pPartType); xnsDestroy(xmlPartType);
			ehFree(pPartElement); xnsDestroy(xmlPartElement);
		}
	} 


	//
	// Libero le risorse
	//

	arMessageInp=xmlArrayFree(arMessageInp);
	arMessageOut=xmlArrayFree(arMessageOut);
	pxmlOperation=xmlElementFree(pxmlOperation);
	ehFree(pszMessageInput); 
	ehFree(pszMessageOutput);

//	pxmlBinding=xmlElementFree(pxmlBinding);
//	pxmlBindOperation=xmlElementFree(pxmlBindOperation);
	return NULL;
}

//
// L_XMLSequenceElement()
// Aggiunge nell'XML un element della sequenza
//
/*
static void L_XMLSequenceElement(EH_SOAP *psSoap,
								 CHAR *pNSPartType,	 // Name space del tipo della parte indicata nel messaggio
								 CHAR *pPartType,	 // Type della parte indicata nel messaggio
								 CHAR *pPartName,	 // Nome della parte indicata nel messaggio
								 CHAR *pValue,		 // Valore della parte
								 INT iDeepSpace,	 // Profondità della parte
								 INT iMode)		 // Tipo di encoding
{

	if (pNSPartType)
		{
			ARAddarg(psSoap->arXML"%s<%s xsi:type=\"%s:%s\">%s</%s>"CRLF,
					_LIndentSpace(iDeepSpace),
					pPartName,
					pNSPartType,pPartType,
					pValue,
					pPartName);
		}
		else
		{
			ARAddarg(psSoap->arXML"%s<%s xsi:type=\"%s\">%s</%s>"CRLF,
					_LIndentSpace(iDeepSpace),
					pPartName,
					pPartType,
					pValue,
					pPartName);
		}
}
*/

//
// soap_request_builder()
// Trasoforma una "script function" in una "soap operation"
//
static EH_SOAP_REQUEST *soap_request_builder(EH_SOAP *psSoap,CHAR *pScript)
{
	void *pReturn=NULL;

	XMLELEMENT *pxmlDefinition=NULL;
	XMLELEMENT *pxmlOperation=NULL;
	XMLELEMENT *pxmlBinding=NULL;
	XMLELEMENT *pxmlJolly=NULL;
	XMLARRAY arBindingOperation;
	XMLELEMENT *pxmlBindOperation=NULL;
	XMLELEMENT *pxmlServicePort=NULL;
	XMLARRAY arTypesElement=NULL;
	XMLARRAY arTypesComplexType=NULL;
	XMLARRAY arJolly=NULL;

	BYTE *p;
	BYTE *pOperation;
	BYTE *pFunctionEndName;
	BYTE *pStringStart=NULL;
	BYTE *pParamStart=NULL;
	INT iParam=0;
	CHAR szPortTypeName[100];

	XMLELEMENT *pxmlDoc=NULL;
	XMLELEMENT *pxmlInput=NULL;
	XMLELEMENT *pxmlOutput=NULL;
	XMLARRAY	arOutputPart=NULL;
	XMLELEMENT *pxmlMessageInput=NULL;
	XMLELEMENT *pxmlInputBody=NULL;
	XMLELEMENT *pxmlInputHeader=NULL;
	INT idx;

	EH_SOAP_REQUEST *psRequest=NULL;

	XMLARRAY	arInputPart=NULL;
	INT	idxPart=0,iMaxPart=0;
	EHS_TOKEN_INFO sTokenInfo;
	CHAR *pStyle;

	XNS_PTR xnsInput,xnsOperation,xnsObject;
	
	EHS_ELEMENT sOperation;
	EHS_ELEMENT_INFO *psSED;

	p=strstr(pScript,"("); if (!p) {psSoap->enSoapErrorCode=SE_SINTAX_ERROR; return NULL;}
	pFunctionEndName=p-1;

	soap_ns_reset(psSoap);

	//
	// Cerco funzione/<operation> nel portType (porte Astratte = Metodi)
	//
 	pOperation=strTake(pScript,pFunctionEndName);
	pxmlOperation=soap_operation_search(psSoap,pOperation,szPortTypeName);
	if (!pxmlOperation) ehExit("soap operation '%s' sconosciuta -> %s",pOperation,pScript);

	//
	// Creo struttura di richiesta #################################################################################################################################
	//
	psRequest=ehAllocZero(sizeof(EH_SOAP_REQUEST));
	psRequest->pSoapEnvelope=NULL;
	
	if (psSoap->arXML) ARDestroy(psSoap->arXML);
	psSoap->arXML=ARNew();
	psSoap->iDeep=0;

	//
	// Inizializzo la namespace
	//
	strcpy(psSoap->szNsName,"ns");
	idx=soap_ns_add(psSoap,psSoap->szNsName,psSoap->pTargetNamespace,0);
	if (idx>-1) // new 2010
	{
		EHS_NS sSoapNS;
		DMIRead(&psSoap->dmiNamespace,idx,&sSoapNS);
		strcpy(psSoap->szNsName,sSoapNS.pszName);
	}
	psSoap->bNSRenaming=TRUE;
	xnsOperation=xnsCreate(pOperation); 
	soap_ns_reassign(psSoap,xnsOperation,pxmlOperation);

	//
	// Nota: tns: = (targetNamespace) > si trovano in questo file
	//
	
	//
	// SOAP (solo 1.0)
	// Cerco nel binding la operation (concreta) >> binding.operation[name=<ricercata>].operation
	// mi serve per trovare la 
	//
	// - SOAPAction(
	// - l'indirizzo URI La pagina da chiamare per avere il servizio
	//

	//
	// A) Partendo da none della portType che contiene l'operazione cerco il nodo del binding concreto
	//	  Cerco style encoding di default
	//
	pxmlBinding=xmlElementWithAttribute(psSoap->arBinding,"type",szPortTypeName,TRUE);

#ifdef _DEBUG
	if (!pxmlBinding) 
		{ehExit("Non trovato nel binding la porta nome [%s]\7",szPortTypeName);}
#endif

	pxmlJolly=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlBinding->idx,".binding"); // Elenco delle operation contenute nel binding
	pStyle=xmlGetAttrib(pxmlJolly,"style"); // Usato per gli elementi
	if (pStyle) strcpy(psSoap->szOperationStyle,pStyle); else strcpy(psSoap->szOperationStyle,"document"); // <-- default
	
	arBindingOperation=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlBinding->idx,".operation"); // Elenco delle operation contenute nel binding
	pxmlBindOperation=xmlElementWithAttribute(arBindingOperation,"name",pOperation,TRUE);
	{
		XMLELEMENT *pxmlOperationChild=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlBindOperation->idx,".operation");
		psRequest->pSoapAction=xmlGetAttribAlloc(pxmlOperationChild,"soapAction",""); // Trovo la SoapAction
	}
	arBindingOperation=xmlArrayFree(arBindingOperation);

	//
	// Controllo se ho style su operation
	//
	pxmlJolly=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlBindOperation->idx,".operation"); // Elenco delle operation contenute nel binding
	pStyle=xmlGetAttrib(pxmlJolly,"style"); if (pStyle) strcpy(psSoap->szOperationStyle,pStyle); 

	//  <reliability /> ??

	//
	// Encoding da usare per il body header (new 2010)
	//
	/*
	pxmlInputHeader=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlBinding->idx,".operation.input.header"); 
	psSoap->iInputHeaderUse=0; // Non ho Header
	if (pxmlInputHeader)
	{
		CHAR *pMessage;
		strcpy(psSoap->szInputHeaderUse,xmlGetAttrib(pxmlInputHeader,"use"));
		if (!strcmp(psSoap->szInputHeaderUse,"literal")) psSoap->iInputHeaderUse=VARIABLE_TO_XML_LITERAL; else psSoap->iInputHeaderUse=VARIABLE_TO_XML_ENCODED;
		
		// Leggo il messaggio da usare nell'header
		psRequest->pszMessageHeaderInput=xmlGetAttribAlloc(pxmlInputHeader,"message",NULL);
	}
	*/

	//
	// Encoding da usare per il body input
	//
	pxmlInputBody=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlBinding->idx,".operation.input.body"); 
	if (!pxmlInputBody) ehError();
	strcpy(psSoap->szInputBodyUse,xmlGetAttrib(pxmlInputBody,"use"));
	if (!strcmp(psSoap->szInputBodyUse,"literal")) psSoap->enInputBodyUseOriginal=BODY_USE_LITERAL; else psSoap->enInputBodyUseOriginal=BODY_USE_ENCODED;
	psSoap->enInputBodyUse=psSoap->enInputBodyUseOriginal;
	if (psSoap->enInputBodyUseForce) psSoap->enInputBodyUse=psSoap->enInputBodyUseForce;

	//
	// B) Indirizzo URI del servizio
	//
	arJolly=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,psSoap->arService[0]->idx,".port"); // Elenco delle operation contenute nel binding
	pxmlServicePort=xmlElementWithAttribute(arJolly,"name",szPortTypeName,FALSE);
	if (pxmlServicePort)
	{
		XMLELEMENT *pxmlAddress=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlServicePort->idx,".address"); // Elenco delle operation contenute nel binding
		psRequest->pUriAddress=xmlGetAttribAlloc(pxmlAddress,"location",NULL); // Usato per gli elementi
	}
	arJolly=xmlArrayFree(arJolly);

	//
	// C) Apro (le danze) la busta SOAP e Scrivo <operation> richiesta
	// 
	ARAddarg(&psSoap->arXML,"<%s:Envelope @#NAMESPACELIST#@>",psSoap->pszSoapEnvName);
//	strcat(psSoap->pszBuffer,pSoapEnvelopeOpen); 

	//
	// C2) Scrittura del'Header (new 2010) 
	//	   NOTA: Il problema è che i dati dell'header sono codificati come i dati del body e andrebbero passati come argomento della funzione
	//			 al momento risolvo con una stringa di passaggio
	//
	if (!strEmpty(psSoap->pszSoapHeader))
	{
		EH_AR arList;
		INT x;
		
		arList=ARCreate(psSoap->pszSoapHeader,CRLF,NULL);
		ARAddarg(&psSoap->arXML,"<%s:Header>",psSoap->pszSoapEnvName);
		for (x=0;arList[x];x++)
		{
			ARAddarg(&psSoap->arXML,"%s",arList[x]);
		}
		ARAddarg(&psSoap->arXML,"</%s:Header>",psSoap->pszSoapEnvName);
		ARDestroy(arList);
	}

	if (psSoap->iInputHeaderUse)
	{
		/*
		//
		// MESSAGE HEADER  > Cerco le "part/s" di cui è composto il messaggio di input
		//
		xmlNSExtract(psRequest->pszMessageHeaderInput,&sNSName);
		pxmlMessageInput=xmlElementWithAttribute(psSoap->arMessage,"name",sNSName.szName,TRUE);
		arInputPart=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlMessageInput->idx,".part"); // <--- prendo i <service />
		if (arInputPart) for (iMaxPart=0;arInputPart[iMaxPart];iMaxPart++);
		
		strcat(psSoap->pszBuffer,"<soap:Header>" CRLF);
 		strcat(psSoap->pszBuffer,"</soap:Header>" CRLF);
		*/
	}

	// Scrittura del Body
	//strcat(psSoap->pszBuffer,"<soap:Body>" CRLF);
	ARAddarg(&psSoap->arXML,"<%s:Body>",psSoap->pszSoapEnvName);

	// Cerco <message> d
	//i input e ouput
	//
	pxmlInput=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".input"); 
	pxmlOutput=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".output"); 
	pxmlDoc=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".documentation"); 

	psRequest->pszMessageInput=xmlGetAttribAlloc(pxmlInput,"message",NULL);
	psRequest->pszMessageOutput=xmlGetAttribAlloc(pxmlOutput,"message",NULL);
	if (pxmlDoc) {if (pxmlDoc->lpValue) psRequest->pDescription=strDup(pxmlDoc->lpValue);}

/*
	if (*psVar->psElement->pszNSType)
		ARAddarg(psSoap->arXML"%s<%s:%s>" CRLF,_LIndentSpace(iDeep),psVar->psElement->pszNSType,psVar->psElement->pszName);
		else
		ARAddarg(psSoap->arXML"%s<%s>" CRLF,_LIndentSpace(iDeep),psVar->psElement->pszName);
*/

	//
	// MESSAGE INPUT  > Cerco le "part/s" di cui è composto il messaggio di input
	//
	//xmlMessage=xnsCreate(psRequest->pszMessageInput);
	xnsInput=_L_GetAttrib(psSoap,"message",pxmlInput); // 
	pxmlMessageInput=xmlElementWithAttribute(psSoap->arMessage,"name",xnsInput->pszElement,TRUE);
	
	psSoap->iDeep=1;
	if (!xnsOperation->pszSpace) strAssign(&xnsOperation->pszSpace,psSoap->szNsName);//"ns");
	
	ZeroFill(sOperation);
	sOperation.xnsName=xnsOperation;
	ARAddarg(&psSoap->arXML,"%s",_TagBuild(psSoap,TAG_OPEN,&sOperation));

	// ARPrint(psSoap->arXML);

	arInputPart=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlMessageInput->idx,".part"); // <--- prendo i <service />
	if (arInputPart) iMaxPart=xmlArrayLen(arInputPart);

	//
	// LOOP SULLE PARTI > Parser dei parametri (parts)
	//
	pParamStart=p+1; idxPart=0;
	for (p=pParamStart;*p;p++)
	{
		// Sto estrando una stringa
		if (pStringStart) 
		{
			if (strchr("\'\"",*p)) {pStringStart=NULL;}
			continue;
		}
		else
		
		if (strchr("\'\"",*p)) {pStringStart=p; continue;}

		//
		// Analisi del parametro trovato (argomento della funzione)
		//
		if (strchr(",)",*p))
		{
			CHAR *pParamEnd=p-1;
			CHAR *pBuf,*pParam;
			INT iEnd;
			EhsTokenType iType;
			EHS_VARPTR sVarVirtual;
			XNS_PTR xnsPartName,xnsPartType,xnsPartElem;

			if (pParamStart==p) break; // Finito senza parametri
			if (pParamEnd<pParamStart) {psSoap->enSoapErrorCode=SE_SINTAX_PARAM_ERROR; return NULL;}
			if (idxPart>=iMaxPart) ehExit("Troppi parametri nella funzione: %s",pScript);

			//
			// Estraggo info parte
			//
//			xnsPartName=xnsCreate(xmlGetAttrib(arInputPart[idxPart],"name")); soap_ns_reassign(psSoap,xnsPartName,arInputPart[idxPart]);
/*

			xnsPartType=xnsCreate(xmlGetAttrib(arInputPart[idxPart],"type"));
			soap_ns_reassign(psSoap,xnsPartType,arInputPart[idxPart]);

			xnsPartElem=xnsCreate(xmlGetAttrib(arInputPart[idxPart],"element"));
			soap_ns_reassign(psSoap,xnsPartElem,arInputPart[idxPart]);
*/
			xnsPartName=_L_GetAttrib(psSoap,"name",arInputPart[idxPart]);
			xnsPartType=_L_GetAttrib(psSoap,"type",arInputPart[idxPart]);
			xnsPartElem=_L_GetAttrib(psSoap,"element",arInputPart[idxPart]);
			

			//
			// Estraggo il valore del "parametro"
			//
			pBuf=strTake(pParamStart,pParamEnd);
			pParam=strDup(strTrim(pBuf)); ehFree(pBuf);
			iEnd=strlen(pParam)-1;	//if (iEnd<1) {psSoap->iErrorCode=SE_SINTAX_PARAM_ERROR; return NULL;}
			
			//
			// Riconoscimento del parametro
			// Il parametro può essere una costante stringa/numero o un elemento in memoria
			//
			L_WhatIsThis(psSoap,pParam,&sTokenInfo); if (sTokenInfo.iType==TP_UNKNOW) ehExit("'%s' parametro sconosciuto: %s",pParam,pScript);
			
			//
			// Part XML
			// In base al tipo di parametro, creo un elemento virtual da indicare al costruttore della parte XML
			// il nome/namespace del parametro deve essere quello indicato nella "part"
			//
			xnsObject=NULL;
			if (xnsPartElem) 
				xnsObject=xnsPartElem;
				else
				xnsObject=xnsPartType;

			if (xnsObject)
			{
				iType=L_NumberString(xnsObject->pszElement);//sNSType.szName);
				if (iType==TP_UNKNOW) // Verifico se il tipo sconosciuto è un elemento complesso ...
				{
					XMLELEMENT *pxmlComplexT;
					if (xnsPartType) 
						pxmlComplexT=soap_complex_search(psSoap,xnsObject->pszElement);
						else
						pxmlComplexT=soap_element_search(psSoap,xnsObject->pszElement);

					if (pxmlComplexT) iType=TP_OBJECT;
					pxmlComplexT=xmlElementFree(pxmlComplexT);
				}
				if (iType!=sTokenInfo.iType)
					ehExit("Parametro %d, '%s' del tipo '%s' diverso dal tipo atteso, element '%s' > Controllare %s",
							idxPart+1,pParam,
							sTokenInfo.pTypeDesc,xnsPartType->pszElement,//sNSType.szName,
							pScript);
				/*
				pNSPartNameNew=soap_ns_recoding(psSoap,
												SEARCH_NS_NAME,
												xnsPartType->pszSpace,
												arInputPart[idxPart]->idx); 
												*/
			}

			//
			// Creo l'XML dell'argomento contenuto nella funzione (operation) richiesta
			//
			switch (sTokenInfo.iType)
			{
				case TP_NUMBER:
				case TP_CSTRING:
					if (xnsPartElem) ehError();

					//
					// Controllo coerenza (da fare) !!!!
					//

					//
					// Creo la variabile virtuale
					//
					ZeroFill(sVarVirtual);
					sVarVirtual.psElement=ehAllocZero(sizeof(EHS_ELEMENT));
					sVarVirtual.psContainer=ehAllocZero(sizeof(EHS_CONTAINER));
					sVarVirtual.psElement->xnsName=xnsDup(xnsPartName);
					sVarVirtual.psElement->bAlloc=FALSE;
					sVarVirtual.psElement->iType=STE_SIMPLE;
					sVarVirtual.psElement->xnsType=xnsDup(xnsPartType);
					sVarVirtual.psElement->pMinOccurs=strDup("1");
					sVarVirtual.psElement->pMaxOccurs=strDup("1");
					sVarVirtual.psContainer->iCntType=sVarVirtual.psElement->iType;
					switch (sTokenInfo.iType) 
					{
						case TP_NUMBER: 
							sVarVirtual.psContainer->pValue=strDup(pParam); 
							break;
						case TP_CSTRING: 
							{
								BYTE *pString=strCTake(pParam); 
								sVarVirtual.psContainer->pValue=pString;
							}
							break;
						case TP_OBJECT: // Variabile
							printf("Variabile tu virtual-- da fare");
						default: 
							ehError(); // DA FARE (primitivi in variabili)
					}
					psSoap->iDeep=2;
					SoapVariableToXml(psSoap,&sVarVirtual);//,NULL);//"ns"); // ??
					soap_varptr_free(&sVarVirtual);
					break;

				case TP_OBJECT:
					//
					// Leggo i dati dalla struttura passata come argomento (pParam)
					//
					psSED=_LElementInfoGet(psSoap,pParam,TRUE); 
					if (strcmp(psSED->psVirtualElement->xnsType->pszElement,xnsObject->pszElement)&&
						strcmp(psSED->psVirtualElement->pszOriginalType,xnsObject->pszElement))//sNSType.szName))
						ehExit("Parametro %d, '%s' del tipo '%s' diverso dal tipo atteso, element '%s' > Controllare %s",
								idxPart+1,pParam,
								psSED->psVirtualElement->xnsType->pszElement,xnsObject->pszElement,
								pScript);

					//
					// Creo una variabile VIRTUALE per creare l'XML dell oggetto (complesso)
					//
					ZeroFill(sVarVirtual);
					sVarVirtual.psElement=ehAllocZero(sizeof(EHS_ELEMENT));
					sVarVirtual.psElement->bAlloc=TRUE;
					sVarVirtual.psContainer=ehAllocZero(sizeof(EHS_CONTAINER));

					memcpy(sVarVirtual.psElement,psSED->psVirtualElement,sizeof(EHS_ELEMENT));
					sVarVirtual.psElement->xnsName=xnsDup(xnsPartName); // Scrivo il nome della PARTE
					strAssign(&sVarVirtual.psElement->xnsName->pszSpace,psSoap->szNsName);//"ns");
					sVarVirtual.psElement->xnsType=xnsDup(xnsObject); // Assegno il tipo della parte
					memcpy(sVarVirtual.psContainer,psSED->psContainer,sizeof(EHS_CONTAINER));

					psSoap->iDeep++;
					SoapVariableToXml(psSoap,&sVarVirtual); // Converto in XML l'elemento <-----------------------------
					psSoap->iDeep--;

					// Libero le risorse
					psSED=_LElementInfoFree(psSED);
					xnsDestroy(sVarVirtual.psElement->xnsName);
					xnsDestroy(sVarVirtual.psElement->xnsType);
					ehFreePtr(&sVarVirtual.psElement);
					ehFreePtr(&sVarVirtual.psContainer);
					break;
			}
			xnsDestroy(xnsPartName);
			xnsDestroy(xnsPartType);
			xnsDestroy(xnsPartElem);

//			ehFree(pNSPartNameNew);

			//if (psSED) psSED=_LElementInfoFree(psSED);
			ehFree(pParam);
//			ehFree(pszPartName);
//			ehFree(pszPartType);
//			ehFree(pszPartElement);

			iParam++; 
			pParamStart=p+1; idxPart++;
			continue;
		}
	}
	xnsDestroy(xnsInput);
	arInputPart=xmlArrayFree(arInputPart);
	//
	// Controllare che idxPart sia esaurito (DA FARE)
	//
	if (idxPart<iMaxPart) ehExit("Parametri mancanti nella funzione (%d<%d) %s; %s",idxPart,iMaxPart,pOperation,pScript);

	//
	// Chiudo la busta Soap
	//
	psSoap->iDeep=1;
	ARAddarg(&psSoap->arXML,"%s",_TagBuild(psSoap,TAG_CLOSE,&sOperation));// "ns",pOperation));
	ARAddarg(&psSoap->arXML,"</%s:Body>",psSoap->pszSoapEnvName);
	ARAddarg(&psSoap->arXML,"</%s:Envelope>",psSoap->pszSoapEnvName);

	xnsOperation=xnsDestroy(xnsOperation);

	//	strcat(psSoap->pszBuffer,pSoapEnvelopeClose);
	
	pxmlOperation=xmlElementFree(pxmlOperation);
	pxmlMessageInput=xmlElementFree(pxmlMessageInput);
	pxmlBinding=xmlElementFree(pxmlBinding);
	pxmlBindOperation=xmlElementFree(pxmlBindOperation);

	//
	// Trasformo in stringa l'array
	//
	if (psSoap->sXmlRemove.bCRLF)
		p=ARToString(psSoap->arXML,"","","");
		else
		p=ARToString(psSoap->arXML,CRLF,"","");

	psRequest->pSoapEnvelope=ehAlloc(strlen(p)+16000);
	strcpy(psRequest->pSoapEnvelope,p);
	ehFree(p);

	//
	// Compongo i namespace della radice
	//
	{
		EHS_NS sSoapNS;
		INT a;
		CHAR *lpBuf=ehAlloc(0xFFFFF);
		*lpBuf=0;
		for (a=0;a<psSoap->dmiNamespace.Num;a++)
		{
			DMIRead(&psSoap->dmiNamespace,a,&sSoapNS);
//			if (!strcmp(sSoapNS.pszName,"xsd")) continue;
		//	if (sSoapNS.dwCount) 
			{
				if (*lpBuf) strcat(lpBuf," ");
				sprintf(strNext(lpBuf),"xmlns:%s=\"%s\"",sSoapNS.pszName,sSoapNS.pszLocation);
			}
		}

		// sprintf(strNext(lpBuf)," xmlns:tras1=\"Trasme.WS.Interfaces\"");// // DA TOGLIERE
		strReplace(psRequest->pSoapEnvelope,"@#NAMESPACELIST#@",lpBuf);

		ehFree(lpBuf);
	}
	psSoap->bNSRenaming=FALSE;
	ehFree(pOperation);
	return psRequest;

/*

	for (;;)
	{
		//
		// Cerco l'operazione richiesta negli "insiemi" chiamati portType
		// Estraggo elenco delle operazioni (loop sulle porte "Astratte")
		// Corrispondono ai Metodi ( o funzioni) resi disponibili dal webservice
		//
		pxmlOperation=NULL;
		for (s=0;psSoap->arPortType[s];s++)
		{
			XMLARRAY arOperation=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,psSoap->arPortType[s]->idx,".operation");
			strcpy(szPortTypeName,xmlGetAttrib(psSoap->arPortType[s],"name"));
			pxmlOperation=xmlElementWithAttribute(arOperation,"name",pOperation,TRUE);
			arOperation=xmlArrayFree(arOperation);  
			
			if (pxmlOperation) break; // Trovata
		}
		if (!pxmlOperation) {iError=1; break;}

		//
		// OPERATION
		// Analizzo l'operazione (il metodo)
		// estrando i nome dei messaggi di Input e Output
		//
		// Nota: tns: = (targetNamespace) > si trovano in questo file
		//
		{
			XMLELEMENT *pxmlDoc=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".documentation"); // <--- prendo i <service />
			XMLELEMENT *pxmlInput=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".input"); // <--- prendo i <service />
			XMLARRAY	arInputPart=NULL;
			XMLELEMENT *pxmlOutput=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlOperation->idx,".output"); // <--- prendo i <service />
			XMLARRAY	arOutputPart=NULL;
			psRequest->pszMessageInput=xmlGetAttribAlloc(pxmlInput,"message",NULL);
			psRequest->pszMessageOutput=xmlGetAttribAlloc(pxmlOutput,"message",NULL);
			if (pxmlDoc) {if (pxmlDoc->lpValue) psRequest->pDescription=strDup(pxmlDoc->lpValue);}
			
			//
			// SOAP (solo 1.0)
			// Cerco nel binding la operation (concreta) >> binding.operation[name=<ricercata>].operation
			// mi serve per trovare la 
			//
			// - SOAPAction(
			// - l'indirizzo URI La pagina da chiamare per avere il servizio
			//

			//
			// A) Partendo da none della prtType che contiene l'operazione cerco il nodo del binding concreto
			//
			pxmlBinding=xmlElementWithAttribute(psSoap->arBinding,"name",szPortTypeName,TRUE);
			arBindingOperation=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlBinding->idx,".operation"); // Elenco delle operation contenute nel binding
			pxmlBindOperation=xmlElementWithAttribute(arBindingOperation,"name",pOperation,TRUE);
			{
				XMLELEMENT *pxmlOperationChild=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlBindOperation->idx,".operation");
				psRequest->pSoapAction=xmlGetAttribAlloc(pxmlOperationChild,"soapAction",""); // Trovo la SoapAction
			}
			arBindingOperation=xmlArrayFree(arBindingOperation);

			//
			// B) Indirizzo URI del servizio
			//
			arJolly=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,psSoap->arService[0]->idx,".port"); // Elenco delle operation contenute nel binding
			pxmlServicePort=xmlElementWithAttribute(arJolly,"name",szPortTypeName,FALSE);
			if (pxmlServicePort)
			{
				XMLELEMENT *pxmlAddress=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlServicePort->idx,".address"); // Elenco delle operation contenute nel binding
				psRequest->pUriAddress=xmlGetAttribAlloc(pxmlAddress,"location",NULL); // Usato per gli elementi
			}

			arJolly=xmlArrayFree(arJolly);

			//
			// Apro la busta SOAP
			//
			strcat(pBuffer,pSoapEnvelopeOpen);

			//
			// Scrivo l'operazione richiesta
			//
			ARAddarg(psSoap->arXML"%s<%s xmlns=\"%s\">" CRLF,
					_LIndentSpace(2),
					pOperation,
					psSoap->pTargetNamespace);

			//
			// MESSAGE INPUT  >  Aggiungo gli elementi (parti)
			//
			pName=XMLNameGet(psRequest->pszMessageInput,szNamespace,szName);
			pxmlMessageInput=xmlElementWithAttribute(psSoap->arMessage,"name",pName,TRUE);
			arInputPart=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlMessageInput->idx,".part"); // <--- prendo i <service />

			//
			// Se ci sono parti da spedire di parametri li aggiungo
			//
			if (arInputPart)
			{
				for (k=0;arInputPart[k];k++)
				{
					BYTE *pName=xmlGetAttribAlloc(arInputPart[k],"name",NULL);
					BYTE *pType=xmlGetAttribAlloc(arInputPart[k],"type",NULL); // Usato per i tipi semplici e complessi
					BYTE *pElement=xmlGetAttribAlloc(arInputPart[k],"element",NULL); // Usato per gli elementi
					//
					// Tipo Semplice o complesso
					//
					if (pType)
					{
						//EHS_ELEMENT *psSoapVar;
						CHAR *pParamValue;
				//		CHAR *pVarValue;
						CHAR *pTypeName;
	//					CHAR *pLocation;
						CHAR szTypeNamespace[80];
						CHAR szTypeName[80];
						CHAR *pNewNamespace=NULL;
						//printf(" - (type) %s\t%s" CRLF,pType,pName);

						//
						// pType <name space>:type
						// devo rintracciare il name space ed assegnargli quello globale della chiamata
						//
						pTypeName=XMLNameGet(pType,szTypeNamespace,szTypeName);
						if (*szTypeNamespace) pNewNamespace=soap_NSManager(psSoap,szTypeNamespace,arInputPart[k]->idx);

						//
						// Prendo il parametro
						//
						pParamValue=soap_get_param(psSoap,"[?]"); //if (!psSoapVar) psSoap->iErrorCode=SE_MISSING_PARAMETERS;

						if (pNewNamespace)
							{
								ARAddarg(psSoap->arXML"%s<%s xsi:type=\"%s:%s\">%s</%s>"CRLF,
										_LIndentSpace(3),
										pName,
										pNewNamespace,pTypeName,
										pParamValue,
										pName);
							}
							else
							{
								ARAddarg(psSoap->arXML"%s<%s xsi:type=\"%s\">%s</%s>"CRLF,
										_LIndentSpace(3),
										pName,
										pTypeName,
										pParamValue,
										pName);
							}
						ehFreePtr(&pParamValue);
						if (pNewNamespace) ehFree(pNewNamespace);
						//if (psSoapVar) soap_var_free(psSoapVar);

					}
					//
					// Elemento
					//
					else
					{
						//
						// Ricerco come usare (in concreto) l'elemento
						// binding->operation->input->body (use)
						//
						XMLELEMENT *pxmlInputBody=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlBindOperation->idx,".input.body"); 
						BYTE *pUse=xmlGetAttribAlloc(pxmlInputBody,"use",NULL); // Usato per gli elementi
						
						//printf(" - (element) %s : concrete use=%s" CRLF,pElement,pUse);
						pResult=soap_element_encode(psSoap,pElement,pUse,3);
						strcat(pBuffer,pResult);
						ehFree(pResult);


						ehFree(pUse);
					}

					ehFree(pType); 
					ehFree(pName);
					ehFree(pElement);
				}
				arInputPart=xmlArrayFree(arInputPart);
			}

			//
			// Chiudo la busta Soap
			//
			ARAddarg(psSoap->arXML"    </%s>" CRLF,pOperation);
			strcat(pBuffer,pSoapEnvelopeClose);
		}

		pxmlOperation=xmlElementFree(pxmlOperation);
		pxmlMessageInput=xmlElementFree(pxmlMessageInput);
		pxmlBindOperation=xmlElementFree(pxmlBindOperation);

*/
}

/*
EHS_ELEMENT *soap_var_get(EH_SOAP *psSoap,CHAR *pName)
{
	EHS_ELEMENT sSoapVar,*psSoapVar;
	INT a;
	for (a=0;a<psSoap->dmiVar.Num;a++)
	{
		DMIRead(&psSoap->dmiVar,a,&sSoapVar);
		if (!strcmp(sSoapVar.pszName,pName))
		{
			psSoapVar=ehAlloc(sizeof(EHS_ELEMENT));
			memcpy(psSoapVar,&sSoapVar,sizeof(EHS_ELEMENT));
			psSoapVar->pszName=strDup(psSoapVar->pszName);
			psSoapVar->pszValue=strDup(psSoapVar->pszValue);
			return psSoapVar;
		}
	}
	return NULL;
}
*/


void soap_ns_reassign(EH_SOAP *psSoap,XNS_PTR xns,XMLELEMENT *pxml)
{
	CHAR *pszName;
	if (!xns) return;
	if (strEmpty(xns->pszSpace)) return;
	pszName=soap_ns_recoding(psSoap,SEARCH_NS_NAME,xns->pszSpace,pxml->idx);
	strAssign(&xns->pszSpace,pszName);
	ehFree(pszName);
}

INT soap_ns_add(EH_SOAP *psSoap,CHAR *pName,CHAR *pLocation,INT idx)
{
	EHS_NS sSoapNS;
	INT idxRet;
	
	idxRet=soap_ns_search(psSoap,pLocation,SEARCH_NS_URI);	
	if (idxRet>-1) 
		return idxRet;

	ZeroFill(sSoapNS);
	sSoapNS.idx=idx;
	sSoapNS.pszName=strDup(pName);
	sSoapNS.pszLocation=strDup(pLocation);
	DMIAppendDyn(&psSoap->dmiNamespace,&sSoapNS);
	return -1;
}

INT soap_ns_search(EH_SOAP *psSoap,CHAR *pName,EN_NS_SEARCH enWhere)
{
	EHS_NS sSoapNS;
	INT a;
	for (a=0;a<psSoap->dmiNamespace.Num;a++)
	{
		DMIRead(&psSoap->dmiNamespace,a,&sSoapNS);
		if (enWhere==SEARCH_NS_NAME&&!strcmp(sSoapNS.pszName,pName)|| // ricerca per nome
			enWhere==SEARCH_NS_URI&&!strcmp(sSoapNS.pszLocation,pName)) // Ricerca per location
		{
			sSoapNS.dwCount++;
//			if (!strcmp(sSoapNS.pszName,"nsSC")&&enWhere==SEARCH_NS_NAME)
//				printf("qui");
			DMIWrite(&psSoap->dmiNamespace,a,&sSoapNS);
			return a;
		}
	}
	return -1;
}

EHS_NS *soap_ns_getalloc(EH_SOAP *psSoap,CHAR *pName,EN_NS_SEARCH enWhere)
{
	EHS_NS sSoapNS,*psSoapNS;
	INT idx=soap_ns_search(psSoap,pName,enWhere);
	if (idx<0) return NULL;
	DMIRead(&psSoap->dmiNamespace,idx,&sSoapNS);

	psSoapNS=ehAlloc(sizeof(EHS_NS));
	memcpy(psSoapNS,&sSoapNS,sizeof(EHS_NS));
	psSoapNS->pszName=strDup(psSoapNS->pszName);
	psSoapNS->pszLocation=strDup(psSoapNS->pszLocation);
	return psSoapNS;
/*
	for (a=0;a<psSoap->dmiNamespace.Num;a++)
	{
		DMIRead(&psSoap->dmiNamespace,a,&sSoapNS);

		if (enWhere==SEARCH_NS_NAME&&!strcmp(sSoapNS.pszName,pName)|| // ricerca per nome
			enWhere==SEARCH_NS_URI&&!strcmp(sSoapNS.pszLocation,pName)) // Ricerca per location
		{
			psSoapNS=ehAlloc(sizeof(EHS_NS));
			memcpy(psSoapNS,&sSoapNS,sizeof(EHS_NS));
			psSoapNS->pszName=strDup(psSoapNS->pszName);
			psSoapNS->pszLocation=strDup(psSoapNS->pszLocation);
			return psSoapNS;
		}
	}
	return NULL;
	*/
}

EHS_NS *soap_ns_free(EHS_NS *psSoapNS,BOOL bFree)
{
	ehFree(psSoapNS->pszName);
	ehFree(psSoapNS->pszLocation);
	if (bFree) ehFree(psSoapNS);
	return NULL;
}

void soap_ns_clean(EH_SOAP *psSoap)
{
	EHS_NS sSoapNS;
	INT a;
	for (a=0;a<psSoap->dmiNamespace.Num;a++)
	{
		DMIRead(&psSoap->dmiNamespace,a,&sSoapNS);
		soap_ns_free(&sSoapNS,FALSE);
	}
	psSoap->dmiNamespace.Num=0;
}

void soap_ns_reset(EH_SOAP *psSoap)
{
	EHS_NS sSoapNS;
	INT a;
	for (a=0;a<psSoap->dmiNamespace.Num;a++)
	{
		DMIRead(&psSoap->dmiNamespace,a,&sSoapNS);
		sSoapNS.dwCount=0;
		DMIWrite(&psSoap->dmiNamespace,a,&sSoapNS);
	}
//	soap_ns_search(psSoap,"soapenv",SEARCH_NS_NAME);
	soap_ns_search(psSoap,psSoap->pszSoapEnvName,SEARCH_NS_NAME);
}

void soap_ns_show(EH_SOAP *psSoap)
{
	INT a;
	EHS_NS sSoapNS;
	for (a=0;a<psSoap->dmiNamespace.Num;a++)
	{
		DMIRead(&psSoap->dmiNamespace,a,&sSoapNS);	
		printf("%d) %s - %s  (%d)" CRLF,a,sSoapNS.pszName,sSoapNS.pszLocation,sSoapNS.idx);

	}
}

CHAR *soap_ns_newname(EH_SOAP *psSoap,CHAR *pLocation,CHAR *pszNameOriginal)
{
	CHAR *pReturn=ehAlloc(80);
	CHAR *pAnalisi=ehAlloc(1024);
	CHAR *pBuf;
	INT a;
	EHS_NS *psSoapNS;

	//
	// Provo prima il nome originale
	//
	if (!strEmpty(pszNameOriginal))
	{
		psSoapNS=soap_ns_getalloc(psSoap,pszNameOriginal,0); 
		if (!psSoapNS) return strDup(pszNameOriginal);
	}

	strcpy(pAnalisi,pLocation);
	strReplace(pAnalisi,"http://","");
	strReplace(pAnalisi,"java:","");
	strReplace(pAnalisi,"www","");
	strReplace(pAnalisi,"w3","");
	strReplace(pAnalisi,"org","");
	_strupr(pAnalisi);
	strcpy(pAnalisi,strOmit(pAnalisi,"0123456789WAEIOU/."));
	strcat(pAnalisi,"ABCDEFGHIHJLM");

	for (a=1;a<(INT) strlen(pAnalisi);a++)
	{
		pBuf=strTake(pAnalisi,pAnalisi+a);
		sprintf(pReturn,"ns%s",pBuf); 
		ehFree(pBuf);
		//sprintf(szServ,"ns%02d",a);
		psSoapNS=soap_ns_getalloc(psSoap,pReturn,0); 
		if (!psSoapNS) break;
		soap_ns_free(psSoapNS,TRUE);
	}
	ehFree(pAnalisi);
	return pReturn;
}


//
// soap_operation_search()
// NOTA: L'elemento è allocato (clonato) e va quindi liberato con ehFree
//	
// Ritorna il tipo di porta in pszPortTypeName (serve per trovare il binding)
//
static XMLELEMENT *soap_operation_search(EH_SOAP *psSoap,CHAR *pOperationName,CHAR *pszPortTypeName)
{
	XMLELEMENT *pxmlOperation=NULL;
	INT s;

	//
	// Cerco l'operazione richiesta negli "insiemi" chiamati portType
	// Estraggo elenco delle operazioni (loop sulle porte "Astratte")
	// Corrispondono ai Metodi ( o funzioni) resi disponibili dal webservice
	//
	pxmlOperation=NULL;
	for (s=0;psSoap->arPortType[s];s++)
	{
		XMLARRAY arOperation=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,psSoap->arPortType[s]->idx,".operation");
		pxmlOperation=xmlElementWithAttribute(arOperation,"name",pOperationName,TRUE);
		if (pxmlOperation) 
		{
			strcpy(pszPortTypeName,xmlGetAttrib(psSoap->arPortType[s],"name"));
			arOperation=xmlArrayFree(arOperation);  
			break; // Trovata
		}
		arOperation=xmlArrayFree(arOperation);  
	}
	return pxmlOperation;
}

//
// soap_ns_recoding()
// a) Cerca il nome del xml all'interno dell'elenco generale se lo trova ritorna il nome ricercato
// b) Cerca il nome del name space del documento xml WSDL partendo dalla posizione corrente e andando all'indietro nell'albero
// c) Se lo trova confronta la location con quelle già inserite 
// d) La location è presente, ritorna il nome del namespace generale
// e) La location non è presente, aggiunge un NS generale e ritorna il nome
//
// NOTA: Ho un dubbio, ma do di "default" che non possano esistere nel documento due namespace con stesso nome e location differenti 
// Se fosse così il programmatore è un cretino
//

static CHAR *soap_ns_getaddr(EH_SOAP *psSoap,CHAR *pNamespace,INT idx)
{
	XMLELEMENT *pxmlElement;
//	BYTE *pBuf,*pNS,*pStart,*pEnd;
	INT iLevel;
	CHAR szServ[200];

	// Tapullo ...
	if (!strcmp(pNamespace,"xsd")) return strDup("http://www.w3.org/2001/XMLSchema"); 

	//
	// B) Cerca il nome del name space del documento xml WSDL partendo dalla posizione corrente e andando all'indietro nell'albero
	//
	pxmlElement=&psSoap->xmlWSDL.arElement[idx];
	iLevel=pxmlElement->iLevel;
	sprintf(szServ,"xmlns:%s",pNamespace);
	//for (;iLevel>-1;)
	while (TRUE)
	{
		//
		// Scansione negli "attributi" del tag corrente
		//
		BYTE *pszAttrib;
		pszAttrib=xmlGetAttrib(pxmlElement,szServ); 
		if (pszAttrib) 
			return strDup(pszAttrib);
		//
		// Salgo di livello sul padre e rifaccio la ricerca
		//
		pxmlElement=xmlGetParent(&psSoap->xmlWSDL,pxmlElement);  if (!pxmlElement) break;
	}
		
//		if (pxmlElement->arAttrib)
//		{
			/*
			for (pStart=pElement->lpAttrib;;)
			{
				pStart=strstr(pStart," xmlns:"); if (!pStart) break;
				pStart+=7; pEnd=strstr(pStart,"=");
				pBuf=strTake(pStart,pEnd-1); pNS=strTrim(pBuf); 
				if (!strcmp(pNS,pNamespace)) // <--- Trovato
				{
					//
					// Prendo la location
					//
					ehFree(pBuf);
					pStart=strstr(pEnd,"\""); pStart++;
					pEnd=strstr(pStart,"\""); pEnd--;
					pBuf=strTake(pStart,pEnd);
					return pBuf; // Trovato
				}
				ehFree(pBuf);
				pStart=pEnd; 
			}
		}
			*/


#ifdef _DEBUG
	soap_ns_show(psSoap);
#endif
	ehExit("namespace %s non trovato ??",pNamespace);
	return NULL;
}

//
// soap_ns_recoding()
//
static CHAR *soap_ns_recoding(	EH_SOAP *psSoap,
								EN_NS_SEARCH enSearch, // 0=Namespace, 1=Address
								CHAR *pChiave,INT idx) 
{
	EHS_NS *psSoapNS;
	CHAR *pUriAddr;
	CHAR *pReturn=NULL;

	switch (enSearch)
	{
		case SEARCH_NS_NAME:
			//
			// A) Cerca il nome del xml all'interno dell'elenco generale se lo trova ritorna il nome passato
			//
			psSoapNS=soap_ns_getalloc(psSoap,pChiave,SEARCH_NS_NAME);
			if (psSoapNS) 
			{
				soap_ns_free(psSoapNS,TRUE); 
				return strDup(pChiave);
			}

			
			//
			// c)	Se lo trova confronta la location con quelle già inserite 
			//		(Ricerco NS per location)
			//
			pUriAddr=soap_ns_getaddr(psSoap,pChiave,idx); // Cerci l'indirizzo della parte, partendo da pUriAddr
			psSoapNS=soap_ns_getalloc(psSoap,pUriAddr,SEARCH_NS_URI);
			if (!psSoapNS) 
			{
				//
				// e) La location non è presente, aggiunge un NS generale e ritorna il nome
				//
				pReturn=soap_ns_newname(psSoap,pUriAddr,NULL);
				soap_ns_add(psSoap,pReturn,pUriAddr,0); // Aggiunto a livello 0
			}
			else
			{
				//
				// d) La location è presente, ritorna il nome del namespace generale
				//
				pReturn=strDup(psSoapNS->pszName);
				soap_ns_free(psSoapNS,TRUE);
			}
			ehFree(pUriAddr);
			break;

		case SEARCH_NS_URI:
			//
			// A) Cerca il nome del xml all'interno dell'elenco generale se lo trova ritorna il nome passato
			//
			psSoapNS=soap_ns_getalloc(psSoap,pChiave,1);
			if (psSoapNS) 
			{
				pReturn=strDup(psSoapNS->pszName); 
				soap_ns_free(psSoapNS,TRUE); 
				break;
			}

			pReturn=soap_ns_newname(psSoap,pChiave,NULL);
			soap_ns_add(psSoap,pReturn,pChiave,0); // Aggiunto a livello 0
			if  (psSoapNS) soap_ns_free(psSoapNS,TRUE);
			break;
	}
	return pReturn;
}



static CHAR *_LIndentSpace(EH_SOAP *psSoap)
{
	static CHAR szSpace[1024];
	INT iLen=psSoap->iDeep;//*2;

	if (psSoap->sXmlRemove.bIndent) return "";
	memset(szSpace,' ',iLen); szSpace[iLen]=0;
	return szSpace;
}

CHAR *soap_get_param(EH_SOAP *psSoap,CHAR *pDefault)
{
//	CHAR szServ[80];
//	EHS_ELEMENT *psParam;
	CHAR *pReturn=NULL;
/*
	sprintf(szServ,"_PARAM%d",psSoap->iParamGet);
	psParam=soap_var_get(psSoap,szServ); //if (!psSoapVar) psSoap->iErrorCode=SE_MISSING_PARAMETERS;
	if (psParam) {psSoap->iParamGet++; pReturn=strDup(psParam->pszValue);} 
	else 
	{
		if (pDefault) pReturn=strDup(pDefault);
	}
	*/
	return pReturn;
}

//
// soap_element_encode()
//
CHAR * soap_element_encode(EH_SOAP *psSoap,CHAR *pElement,CHAR *pUseMode)
{
	XMLARRAY arSubElement=NULL;
	INT s;
	XMLELEMENT *pxmlElement=NULL;
	XMLARRAY arTypesElement;
	CHAR *pBuffer=ehAllocZero(0xFFFF);
	CHAR *pParamValue;
	//S_XNS_PTR sNSElement;
	XNS_PTR xnsElement;

	//xmlNSExtract(pElement,&sNSElement);
	xnsElement=xnsCreate(pElement);
	
	//
	// Loop sugli schemi presenti
	//
	for (s=0;psSoap->arSchema[s];s++)
	{
		arTypesElement=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,psSoap->arSchema[s]->idx,".element");  // Estraggo array degli elementi
		pxmlElement=xmlElementWithAttribute(arTypesElement,"name",xnsElement->pszElement,TRUE);	// Ricerco l'elemento name=
		arTypesElement=xmlArrayFree(arTypesElement); 
		if (pxmlElement) break;
	}
	if (!pxmlElement) ehExit("Non ho trovato elemento [%s]",pElement);

	//
	// Trovato l'elemento eseguo encoding ...
	// 
	arSubElement=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlElement->idx,".complexType.sequence.element");

	//
	// .complexType.sequence
	//
	if (arSubElement)
	{
		for (s=0;arSubElement[s];s++)
		{
			BYTE *pName=xmlGetAttribAlloc(arSubElement[s],"name",NULL); // Usato per gli elementi
			BYTE *pType=xmlGetAttribAlloc(arSubElement[s],"type",NULL); // Usato per gli elementi

			pParamValue=soap_get_param(psSoap,"[?]"); //if (!psSoapVar) psSoap->iErrorCode=SE_MISSING_PARAMETERS;

			ARAddarg(&psSoap->arXML,"%s<%s>%s</%s>",
				_LIndentSpace(psSoap),
				pName,
				pParamValue,
				pName);

			ehFreePtr(&pParamValue);
			ehFree(pName); 
			ehFree(pType);
		}
	}


	//
	// Libero le risorse
	//
	arSubElement=xmlArrayFree(arSubElement); 
	pxmlElement=xmlElementFree(pxmlElement);
	xnsDestroy(xnsElement);
	return pBuffer;
}


/*
POST /maws20/default.asmx HTTP/1.1
Host: www.maitaly.it
Content-Type: text/xml; charset=utf-8
Content-Length: length
SOAPAction: "http://www.maitaly.it/ws20/AskForCredits"

<?xml version="1.0" encoding="utf-8"?>
<soap:Envelope xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
xmlns:xsd="http://www.w3.org/2001/XMLSchema" 
xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body> 
    <AskForCredits xmlns="http://www.maitaly.it/ws20/">
      <user>string</user>
      <psw>string</psw>
    </AskForCredits>
  </soap:Body>
</soap:Envelope>


/*
<SOAP-ENV:Envelope 
xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" 
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
xmlns:xsd="http://www.w3.org/2001/XMLSchema" 

xmlns:SOAP-ENC="http://schemas.xmlsoap.org/soap/encoding/"  // ??
xmlns:nsJM="java:moby"  // ??
xmlns:nsMB="http://www.mobyws.it"> // ??

 <SOAP-ENV:Body SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <nsMB:login>
   <user xsi:type="xsd:string">018537</user>
    <password xsi:type="xsd:string">mhrc</password>
    <lang xsi:type="xsd:string">IT</lang>
  </nsMB:login>
 </SOAP-ENV:Body>
</SOAP-ENV:Envelope>
*/

static void _xmlArrayShow(XMLARRAY arJolly)
{
	INT a;
	if (!arJolly) return;
	for (a=0;arJolly[a];a++)
	{
		BYTE *pNameAttrib=xmlGetAttribAlloc(arJolly[a],"name",NULL);
		printf("%d)\t%s %s (%s)" CRLF,a,arJolly[a]->pName,pNameAttrib,arJolly[a]->pNamespace);
		ehFree(pNameAttrib);
	}
}

/*
	<wsdl:definitions >
		<wsdl:types >

		// Dati astratti
		<types /> <complexType  /> sono le strutture di dati che si usano
		<message /> definizione dei messaggi (o metodi del RPC)
		<operation /> operazioni esposte del servizio (le funzioni) (o metodi del RPC)
		<portType /> Raccolta di oeprazione sono il nome di "porta" (sembra una classe ....)

		// Dati concreti
		<binding /> Instanziazione di una porta astratta (portType)
			Si definisce "chi chiamare e come" e come costruire la chiamata.
			Qui c'è scritta l'indirizzo da chiamare e il modo in cui passare i dati
			<soap:binding /> e/o <soap:operation /> possono contenere 
			- style="rpc"		= all'interno del messaggio viene inserito un body ... (vedi Pag 32)
			- style="document"	= Contenuto letterale (vedi pag 32)
			Nota: Style può essere ridefinito in ogni singola operation
			<operation />
			 - Contiene lo stile di cofidica se diverso (es style="rpc")
			 - La SoapAction (che andra indicata nell'header)
		 
		 <service>
		  <port />
		   <soap:address location="<indirizzo da chiamare>" >
*/

EH_SOAP_REQUEST *soap_requestFree(EH_SOAP_REQUEST *psSoapRequest)
{
	ehFreePtr(&psSoapRequest->pSoapAction);
	ehFreePtr(&psSoapRequest->pSoapEnvelope);
	ehFreePtr(&psSoapRequest->pDescription);
	ehFreePtr(&psSoapRequest->pszMessageInput);
	ehFreePtr(&psSoapRequest->pszMessageOutput);
	ehFreePtr(&psSoapRequest->pUriAddress);
	ehFree(psSoapRequest);
	return NULL;
}


static CHAR *strCTake(CHAR *pSource)
{
	CHAR *pReturn=ehAlloc(strlen(pSource)+1);
	CHAR *p;
	CHAR *pDest=pReturn;
	BOOL bString=FALSE;

	for (p=pSource;*p;p++)
	{
		if (strchr("\'\"",*p)) {bString=TRUE; continue;}
		
		if (!bString)
		{
			if (!strchr(" \t\n\r",*p)) {pDest=NULL; break;} // Sintax error
			continue;
		}
		else
		{
			if (*p=='\\') // Escape sequence
			{
				p++;
				if (*p=='\'') {*pDest++='\''; continue;}
				if (*p=='\"') {*pDest++='\"'; continue;}
				if (*p=='t') {*pDest++='\t'; continue;}
				{pDest=NULL; break;} // Sintax error
			}
			
			if (strchr("\'\"",*p)) {bString=FALSE; continue;}

			*pDest++=*p;
		}
	}
	if (!pDest) {ehFree(pReturn); return NULL;}
	*pDest++=0;
	return pReturn;
}
//
// soap_href_search()
// NOTA: Secondo me va fatta in modo differente, si dovrebbe ricercare qualunque elemento con dentro ID o almeno
// tutti i figli di body "indistinamente"
//
XMLELEMENT * soap_href_search(EH_SOAP *psSoap,XMLDOC *pxmlResponse,BYTE *pHref)
{
	XMLARRAY arXmlData;
	XMLELEMENT *pxmlElement=NULL;

	arXmlData=xmlParser(pxmlResponse,WS_PROCESS,0,"Envelope.Body.multiRef");
	if (arXmlData)
	{
		pxmlElement=xmlElementWithAttribute(arXmlData,"id",pHref,FALSE);
		xmlArrayFree(arXmlData);
	}

	if (!pxmlElement)
	{
		arXmlData=xmlParser(pxmlResponse,WS_PROCESS,0,"Envelope.Body.string");
		if (arXmlData)
		{
			pxmlElement=xmlElementWithAttribute(arXmlData,"id",pHref,FALSE);
			xmlArrayFree(arXmlData);
		}
	}
	return pxmlElement;
}

//
// L_RecursiveResponseToVar() (* ricorsiva)
//
// Funzione CORE
// Importa i variabili il responze XML
//
XMLELEMENT * L_RecursiveResponseToVar(EH_SOAP *psSoap,XMLDOC *pxmlResponse,XMLELEMENT *pxmlElementOriginal,CHAR *pVarName)
{
	XMLELEMENT *pxmlChild,*pxmlFirstNode;
	XMLELEMENT *pxmlElement=pxmlElementOriginal;
	XMLELEMENT *pxmlParentArray;
	XMLARRAY arElement;
	CHAR szServ[2048];
	CHAR szVarName[2048];
	EHS_ELEMENT_INFO *psSED,*psElementInfo;
	INT idx,iLenght;
	BYTE *pHref;
	BYTE *pAssign=NULL;


	//
	// HREF - Non ho la definizione : Cerco il reale elemento
	//
	pHref=xmlGetAttribAlloc(pxmlElement,"href",NULL);
	if (pHref)
	{
//		pxmlElement=soap_href_search(psSoap,pxmlResponse,pHref+1);
		pxmlElement=xmlIdSearch(pxmlResponse,pHref+1);
		if (!pxmlElement) 
		{
			ehError();
		}
		ehFree(pHref);

		L_RecursiveResponseToVar(psSoap,pxmlResponse,pxmlElement,pVarName);
		goto FINE;
	}


	//
	// ARRAY 
	// L'Elemento è array (richiesta di elemento senza indice)
	// Devo determinare la dimensione e fare un loop con un indice
	// Alla fine ritorno il prossimo elemento da elaborare
	//
	psElementInfo=_LElementInfoGet(psSoap,pVarName,TRUE);

	if (psElementInfo->iErrorCode==SVE_OK&&
		psElementInfo->iArrayType) // devo elaborare un Array
	{
		// 
		// Controllo se sono sul primo elemento o sull'array
		//
		switch (psElementInfo->psVirtualElement->iArrayType)
		{
			case STA_ARRAY: // Esplicito (Es. Moby)
				pxmlFirstNode=xmlGetFirstChild(pxmlResponse,pxmlElementOriginal);
				if (pxmlFirstNode) 
				{
					arElement=xmlArrayAlloc(pxmlResponse,pxmlFirstNode,pxmlFirstNode->pName); // Ricerca sensa controllo
					/*
					if (!arElement) arElement=xmlArrayAlloc(pxmlResponse,pxmlFirstNode,psElementInfo->psVirtualElement->xnsType->pszElement);
					if (!arElement) arElement=xmlArrayAlloc(pxmlResponse,pxmlFirstNode,psElementInfo->psVirtualElement->xnsName->pszElement); // Qualcuno fa cosi
					*/
					if (!arElement) 
						ehError();
				} else arElement=NULL; // Non ho nulla nell'array
				break;

			case STA_ARRAY_ELEMENT:		// Implicito (Tipo Acciona)
				//ehError(); 
				pxmlParentArray=xmlGetParent(pxmlResponse,pxmlElementOriginal);
				arElement=xmlArrayAlloc(pxmlResponse,pxmlElementOriginal,psElementInfo->psVirtualElement->xnsType->pszElement);
				if (!arElement) arElement=xmlArrayAlloc(pxmlResponse,pxmlElementOriginal,psElementInfo->psVirtualElement->xnsName->pszElement); // Qualcuno fa cosi
				if (!arElement) ehError();
				pxmlElementOriginal=pxmlParentArray; // Ha finito a sto livello
				break;
			
			default:
				ehError();
				break;
		}
		//
		// a) Estraggo ARRAY e Conto gli elementi dello stesso tipo
		//
		if (!arElement) iLenght=0; else iLenght=xmlArrayLen(arElement);
		//if (!iLenght) {printf("Nessun elemento ?");	ehError();}

		//
		// b) Strutturo la memoria per contenere un array e memorizzare l'array degli elementi
		//
		if (iLenght)
		{
			for (idx=0;arElement[idx];idx++)
			{
				// Aquisizione dell'elemento
				sprintf(szVarName,"%s[%d]",pVarName,idx);
				L_RecursiveResponseToVar(psSoap,pxmlResponse,arElement[idx],szVarName);
			}
			xmlArrayFree(arElement);
		}
		psElementInfo=_LElementInfoFree(psElementInfo);
		goto FINE;
		// = FINE > vado al prossimo child stesso livello
	}

	//
	// --> Gestione scrittura Over size di un elemento di un Array <-----------------------------
	//
	if 	(psElementInfo->iErrorCode==SVE_INDEX_OVERSIZE&&
		psElementInfo->iArrayType==STA_ARRAY_ELEMENT)
		{
			// printf("Inizializzo elemento ?");
			pAssign=ehAlloc(strlen(pVarName)+100);
			sprintf(pAssign,"%s=NULL",pVarName);
			SoapVariable(psSoap,WS_PROCESS,pAssign);
			ehFree(pAssign);
		}
	psElementInfo=_LElementInfoFree(psElementInfo);

	//
	// Cerco se ho il figlio dell'elemento in elaborazione
	//
	pxmlChild=xmlGetFirstChild(pxmlResponse,pxmlElementOriginal);

	//
	// Have I children ? (c'ho nodi figli ? )
	// NO = Leggo il valore dell'elelemento SIMPLE 
	//
	if (!pxmlChild)
	{
		BYTE *pValueForVar;
		EHS_SIMPLEDATATYPE *psSDT;
 		psSED=_LElementInfoGet(psSoap,pVarName,TRUE); // Leggo l'elemento

		//
		// Controllo se è un elemento semplice
		//
		psSDT=soap_SimpleTypeInfo(psSED->psVirtualElement->xnsType->pszElement); 
		if (!psSDT) 
		{
			psSED=_LElementInfoFree(psSED);
			goto FINE;
		}

		//
		// Controllo di coerenza "Tipo Dato"
		//
		psSDT=soap_SimpleDataTypeCheck(psSED->psVirtualElement->xnsType->pszElement,pVarName);	
		if (pxmlElement->lpValue)
		{
			pValueForVar=(*psSDT->funcEncode)(psSoap,SE_XML_TO_VAR,NULL,pxmlElement->lpValue);//NULL,pxmlElement->lpValue,"");
			pAssign=ehAlloc(strlen(pVarName)+strlen(pValueForVar)+100);
			sprintf(pAssign,"%s=%s",pVarName,pValueForVar);
			ehFree(pValueForVar);
		}
		else
		{
			//
			// Controllo che non abbia un valore di riferimento HREF
			//
			BYTE *pHref=xmlGetAttribAlloc(pxmlElementOriginal,"href",NULL);
			if (pHref) // Riferimento ad un elemento 
			{
				// Cerco un array di elementi tipo
				XMLARRAY arXmlData;
				sprintf(szServ,"Envelope.Body.%s",psSED->psVirtualElement->xnsType->pszElement);
				arXmlData=xmlParser(pxmlResponse,WS_PROCESS,0,szServ);
				if (arXmlData)
				{
					XMLELEMENT *pxmlElement;
					pxmlElement=xmlElementWithAttribute(arXmlData,"id",pHref+1,TRUE);
					if (pxmlElement->lpValue)
					{
//						pValueForVar=(*psSDT->funcEncode)(psSoap,SE_XML_TO_VAR,NULL,NULL,pxmlElement->lpValue,"");
						pValueForVar=(*psSDT->funcEncode)(psSoap,SE_XML_TO_VAR,NULL,pxmlElement->lpValue);//pxmlElement->lpValue,NULL);
						pAssign=ehAlloc(strlen(pVarName)+strlen(pValueForVar)+100);
						sprintf(pAssign,"%s=%s",pVarName,pValueForVar);
						ehFree(pValueForVar);
					}
					else // Non ha valore
					{
						pAssign=ehAlloc(strlen(pVarName)+100);
						sprintf(pAssign,"%s=NULL",pVarName);
					}
					xmlElementFree(pxmlElement);
					xmlArrayFree(arXmlData);
				}
				ehFree(pHref);
			}
			else // Non ha valore
			{
				pAssign=ehAlloc(strlen(pVarName)+100);
				sprintf(pAssign,"%s=NULL",pVarName);
			}
		}

		//
		// ELEMENTO SEMPLICE :Carico la variabile ##########################################################
		//
// 		printf("> %s" CRLF,pAssign);
		SoapVariable(psSoap,WS_PROCESS,pAssign); 
		//SoapVariable(psSoap,WS_DISPLAY,NULL);
		//printf("--" CRLF);
		ehFreePtr(&pAssign);
		psSED=_LElementInfoFree(psSED);
	}
	//
	// SI= Loop sui figli <-------------------- ELEMENTO COMPLESSO ---------------------------------------------
	//
 	else
	{
		for (;pxmlChild;)
		{
			//
			// Creo la nuova variabile Figlio
			//
			sprintf(szServ,"%s.%s",pVarName,pxmlChild->pName);
			pxmlChild=L_RecursiveResponseToVar(psSoap,pxmlResponse,pxmlChild,szServ);
		}
	}

/*
		//
		// ARRAY 
		// L'Elemento è array (richiesta di elemento senza indice)
		// Devo determinare la dimensione e fare un loop con un indice
		// Alla fine ritorno il prossimo elemento da elaborare
		//
		psElementInfo=_LElementInfoGet(psSoap,pVarName,TRUE);
		if (strstr(pVarName,"Tariff[0].Restrictions"))
		{
 			printf("qui");
		}
		if (psElementInfo->iErrorCode==SVE_OK&&
			psElementInfo->iArrayType) // devo elaborare un Array
		{
			// 
			// Controllo se sono sul primo elemento o sull'array
			//
			switch (psElementInfo->psVirtualElement->iArrayType)
			{
				case STA_ARRAY: // Esplicito (Es. Moby)
					pxmlFirstNode=xmlGetChild(pxmlResponse,pxmlElementOriginal);
					arElement=xmlArrayAlloc(pxmlResponse,pxmlFirstNode,psElementInfo->psVirtualElement->xnsType->pszElement);
					if (!arElement) arElement=xmlArrayAlloc(pxmlResponse,pxmlFirstNode,psElementInfo->psVirtualElement->xnsName->pszElement); // Qualcuno fa cosi
					if (!arElement) ehError();
					break;

				case STA_ARRAY_ELEMENT:		// Implicito (Tipo Acciona)
					//ehError(); 
					pxmlParentArray=xmlGetParent(pxmlResponse,pxmlElementOriginal);
					arElement=xmlArrayAlloc(pxmlResponse,pxmlElementOriginal,psElementInfo->psVirtualElement->xnsType->pszElement);
					if (!arElement) arElement=xmlArrayAlloc(pxmlResponse,pxmlElementOriginal,psElementInfo->psVirtualElement->xnsName->pszElement); // Qualcuno fa cosi
					if (!arElement) ehError();
					pxmlElementOriginal=pxmlParentArray; // Ha finito a sto livello
					break;
				
				default:
					printf("qui ?");
					ehError();
					break;
			}
			//
			// a) Estraggo ARRAY e Conto gli elementi dello stesso tipo
			//
//			arElement=xmlArrayAlloc(pxmlResponse,pxmlElement,pxmlElement->pName);
			if (!arElement) ehError();
			iLenght=xmlArrayLen(arElement);	if (!iLenght) {printf("Nessun elemento ?");	ehError();}

			//
			// b) Strutturo la memoria per contenere un array e memorizzare l'array degli elementi
			//

			for (idx=0;arElement[idx];idx++)
			{
				// Aquisizione dell'elemento
				sprintf(szVarName,"%s[%d]",pVarName,idx);
				L_RecursiveResponseToVar(psSoap,pxmlResponse,arElement[idx],szVarName);
			}
			xmlArrayFree(arElement);
			psElementInfo=_LElementInfoFree(psElementInfo);
			// = FINE > vado al prossimo child stesso livello
// 			goto FINE;
		}

		//
		// ARRAY: Elemento (Es. array[0]) gestione OVERSIZE - crezione elemento vuoto
		//
		else 
		*/


FINE:
	if (pxmlElementOriginal) pxmlChild=xmlNextChild(pxmlResponse,pxmlElementOriginal); else pxmlChild=NULL;
	return pxmlChild;
}
/*

			switch (iType)
			{
				case STE_SIMPLE: // Il figlioletto non è un SIMPLE
					L_RecursiveResponseToVar(psSoap,pxmlResponse,pxmlChild,szServ);
					pxmlChild=xmlNextChild(pxmlResponse,pxmlChild);
					break;
				
				case STE_COMPLEX: // Il figlioletto è un COMPLEX
					L_RecursiveResponseToVar(psSoap,pxmlResponse,pxmlChild,szServ);
					pxmlChild=xmlNextChild(pxmlResponse,pxmlChild);
					break;

				case STE_ARRAY: // Il figlioletto è un ARRAY

					// Trovo l'array degli elementi
					arElement=xmlArrayAlloc(pxmlResponse,pxmlChild,pxmlChild->pName);
					idx=xmlArrayLen(arElement); // if (idx==0) printf("QUI");

					// Loop sugli elementi dell'array
					for (idx=0;arElement[idx];idx++)
					{
						// Aquisizione dell'elemento dell'array
						sprintf(szVarName,"%s.%s[%d]",pVarName,pxmlChild->pName,idx);
						L_RecursiveResponseToVar(psSoap,pxmlResponse,arElement[idx],szVarName);
					}
					xmlArrayFree(arElement);
					pxmlChild=xmlNextChild(pxmlResponse,pxmlChild);
					break;

				case STE_REF:
					//ehError(); // DA DEFINIRE
					pxmlChild=NULL;
					break;

				default: 
					ehError();
			}
		}
	}
			*/

//
// soap_response_process()
// Trasforma il "response dei dati" in variabili dell'ambiente Soap
//
static BOOL soap_response_process(EH_SOAP *psSoap,EH_SOAP_REQUEST *psSoapRequest)
{
	XMLDOC xmlResponse;
	CHAR szServ[800];
	XMLARRAY	arOutputPart=NULL;
	XMLELEMENT	*pxmlMessageOutput;
	XMLELEMENT	*pxmlPart;
	INT idxPart,iMaxPart=0;
	//S_XNS_PTR sNSMessage;
	BYTE *pNSPartNameNew;
	EHS_SIMPLEDATATYPE *psSDT;
	XNS_PTR xnsMessage;
	BYTE *pRet;
	BOOL bRet=FALSE;

	// 
	// Apro il documento XML
	//
	ZeroFill(xmlResponse);
	xmlResponse.bUseNamespace=TRUE;
	xmlParser(&xmlResponse,WS_OPEN,XMLOPEN_PTR,psSoap->pszLastDataResponse);
	xmlIdBuilder(&xmlResponse);

	//
	// MESSAGE INPUT  > Cerco le "part/s" di cui è composto il messaggio di input
	//
	//xmlNSExtract(psSoapRequest->pszMessageOutput,&sNSMessage);
	xnsMessage=xnsCreate(psSoapRequest->pszMessageOutput);
	pxmlMessageOutput=xmlElementWithAttribute(psSoap->arMessage,"name",xnsMessage->pszElement,TRUE);
	
	arOutputPart=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlMessageOutput->idx,".part");
	if (arOutputPart) {	for (iMaxPart=0;arOutputPart[iMaxPart];iMaxPart++);}

	//
	// LOOP SULLE PARTI DI RITORNO
	//
	for (idxPart=0;idxPart<iMaxPart;idxPart++)
	{
		XNS_PTR xnsPartType,xnsPartName,xnsPartElem;
		xnsPartType=xnsCreate(xmlGetAttrib(arOutputPart[idxPart],"type"));
		xnsPartName=xnsCreate(xmlGetAttrib(arOutputPart[idxPart],"name")); 
		xnsPartElem=xnsCreate(xmlGetAttrib(arOutputPart[idxPart],"element")); 

		if (!xnsPartType&&!xnsPartElem) 
		{
			xnsDestroy(xnsPartName); 
			break;
		}

		//
		// con Type 
		//
		if (xnsPartType) 
		{
			//
			// Ricodifico il namespace
			//
			if (psSoap->bNSRenaming)
			{
				pNSPartNameNew=strDup("");
				if (!strEmpty(xnsPartType->pszSpace)) pNSPartNameNew=soap_ns_recoding(psSoap,0,xnsPartType->pszSpace,arOutputPart[idxPart]->idx);	
			}
			else
			{
				pNSPartNameNew=strDup(xnsPartType->pszSpace);
			}

			// Provo metodo canocico
			sprintf(szServ,"Envelope.Body.%s.%s",xnsMessage->pszElement,xnsPartName->pszElement);
			pxmlPart=xmlParser(&xmlResponse,WS_FIND,0,szServ);

			// Non trovo il messaggio: Provo metodo (primo figlio)
			if (!pxmlPart) 
			{
				sprintf(szServ,"Envelope.Body.*.%s",xnsPartName->pszElement);
				pxmlPart=xmlParser(&xmlResponse,WS_FIND,0,szServ);
			}


			if (!pxmlPart) // Non trovo la parte
			{
				psSoap->enSoapErrorCode=SE_XML_ERROR;
				xnsDestroy(xnsPartName);
				xnsDestroy(xnsPartType);
				xnsDestroy(xnsPartElem);
				ehFreePtr(&pNSPartNameNew);
				break;
			}
			psSDT=soap_SimpleTypeInfo(xnsPartType->pszElement); // <-- Cerco se è un dato semplice

			//
			// Il dato è semplice ...
			//
			if (psSDT)
			{
				BYTE *pValueForVar;
				BYTE *pMemoAssign;
				psSDT=soap_SimpleDataTypeCheck(xnsPartType->pszElement,szServ);			
				pValueForVar=(*psSDT->funcEncode)(psSoap,SE_XML_TO_VAR,NULL,pxmlPart->lpValue);//NULL,pxmlPart->lpValue,"");

				SoapVariable(psSoap,WS_DEL,xnsPartName->pszElement); // Cancello preventivamente la variabile di ritorno
				sprintf(szServ,"%s:%s %s",pNSPartNameNew,xnsPartType->pszElement,xnsPartName->pszElement);
				SoapVariable(psSoap,WS_PROCESS,szServ);
				pMemoAssign=ehAllocZero(strlen(xnsPartName->pszElement)+strlen(pValueForVar)+20);
				sprintf(pMemoAssign,"%s=%s",xnsPartName->pszElement,pValueForVar);
				ehFree(pValueForVar);
				SoapVariable(psSoap,WS_PROCESS,pMemoAssign);
				ehFree(pMemoAssign);
			}
			else
			{
				//
				// Aggiungo l'elemento strutturato
				//
				SoapVariable(psSoap,WS_DEL,xnsPartName->pszElement);
				sprintf(szServ,"complex %s:%s %s",pNSPartNameNew,xnsPartType->pszElement,xnsPartName->pszElement);
				SoapVariable(psSoap,WS_PROCESS,szServ);

				//
				// leggo in modo ricorsivo le variabili strutturate
				//
				L_RecursiveResponseToVar(psSoap,&xmlResponse,pxmlPart,xnsPartName->pszElement);

				//#ifdef _DEBUG
				//SoapVariable(psSoap,WS_DISPLAY,NULL);
				//#endif
//				SoapVariable(psSoap,WS_DOLIST,szServ);
//				SoapVariable(psSoap,WS_DISPLAY,szServ);
				//ehFree(pArrayType);
			}
		}
		
		//
		// Element Mode
		//
		if (xnsPartElem) 
		{
			//
			// Ricodifico il namespace
			//
			if (psSoap->bNSRenaming)
			{
				pNSPartNameNew=strDup("");
				if (!strEmpty(xnsPartElem->pszSpace)) pNSPartNameNew=soap_ns_recoding(psSoap,0,xnsPartElem->pszSpace,arOutputPart[idxPart]->idx);	
			}
			else
			{
				pNSPartNameNew=strDup(xnsPartElem->pszSpace);
			}

			sprintf(szServ,"Envelope.Body.%s",xnsPartElem->pszElement);
			//xmlResponse.bUseNamespace=FALSE;
			pxmlPart=xmlParser(&xmlResponse,WS_FIND,0,szServ);
			psSDT=soap_SimpleTypeInfo(xnsPartElem->pszElement);

			//
			// Il dato è semplice ...
			//
			if (psSDT)
			{
				BYTE *pValueForVar;
				CHAR *pMemoAssign;
				psSDT=soap_SimpleDataTypeCheck(xnsPartType->pszElement,szServ);			
				pValueForVar=(*psSDT->funcEncode)(psSoap,SE_XML_TO_VAR,NULL,pxmlPart->lpValue);//NULL,pxmlPart->lpValue,"");

				SoapVariable(psSoap,WS_DEL,xnsPartName->pszElement);
				sprintf(szServ,"%s:%s %s",pNSPartNameNew,xnsPartType->pszElement,xnsPartName->pszElement);
				SoapVariable(psSoap,WS_PROCESS,szServ);
				pMemoAssign=ehAllocZero(strlen(xnsPartName->pszElement)+strlen(pValueForVar)+20);
				sprintf(pMemoAssign,"%s=%s",xnsPartName->pszElement,pValueForVar);
				ehFree(pValueForVar);
				SoapVariable(psSoap,WS_PROCESS,pMemoAssign);
				ehFree(pMemoAssign);
			}
			else
			{
				//
				// PROBLEMATICA DA RISOLVERE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
				//
				// in caso di riferimento esterno Ref (da quello che ho capito) arriva con il response la definizione del nuovo elemento come <schema />
				// Quindi non ho documentazione nel WSDL (cioè standard preventiva) di come sarà strutturata l'informazione di response, 
				// ma solo all'atto della risposta...
				//
				// QUESTO E' UN GROSSO PROBLEMA: per due motivi, 
				// - il primo che non posso prestrutturami in modo completo la variabile in modo anticipato come sul complexType
				// - il secondo che l'informazioni <schema> rappresenta un integrazione della documentazione del WSDL ed andrebbe aggiunta a quest'ultimo
				//   prima di effettuare la creazione della variabile.
				//   Se non faccio questo il contenitore non può essere creato
				//
				// In pratica, in questo caso, l'elemento di ritorno mi dice : ti ritornerà una struttura di dati (quasi void)
				//
				// Questo sistema è dovuto ad una codifica automatizzata di un ResultSet di un db
				// in pratica SQL server genera un result di ritorno fatto  cosi
				// <schema /> della tabella
				// <diffgram>
				//  <NewDataSet>
				//   <table>
				//	   <"fieldname">
				//
				// quindi:
				// la definizione iniziale del messaggio di response, va modificata in un array di strutture complesse definite 
				// secondo l'elemento contenuto all'interno dello schema ritornato dal response (e una fettina di culo impanato no ?!?!?)
				//
				// Per ora conviene analizzare il ritorno con il parser XML
				//


				//
				// Aggiungo l'elemento strutturato
				//
				SoapVariable(psSoap,WS_DEL,xnsPartName->pszElement);
				sprintf(szServ,"element %s %s",xnsPartElem->pszElement,xnsPartName->pszElement);
				pRet=SoapVariable(psSoap,WS_PROCESS,szServ);


				//
				// leggo in modo ricorsivo le variabili strutturate
				//
				if (!pRet) {
					L_RecursiveResponseToVar(psSoap,&xmlResponse,pxmlPart,xnsPartName->pszElement);
					psSoap->bResultVarError=FALSE;
					}
					else
					{
						bRet=TRUE;
						psSoap->bResultVarError=TRUE;
					}

			}
		}

		ehFree(pNSPartNameNew);
		xnsDestroy(xnsPartName);
		xnsDestroy(xnsPartType);
		xnsDestroy(xnsPartElem);
	}
	xnsMessage=xnsDestroy(xnsMessage);
	pxmlMessageOutput=xmlElementFree(pxmlMessageOutput);
	arOutputPart=xmlArrayFree(arOutputPart);
	xmlParser(&xmlResponse,WS_CLOSE,0,NULL);
	
	return bRet;
}

//
// soap_request_sender()
// Spedisce la richiesta soap al webservice
// Ritorna NULL se ci sono stati problemi
//
static void *soap_request_sender(EH_SOAP *psSoap,EH_SOAP_REQUEST *psSoapRequest)
{
	//FWEBSERVER sWS;
	EH_WEB	sWeb, * psWeb;
	// BOOL	bReturn;
	BYTE *	pXmlEncode;
	CHAR	szContentType[200];
	void *	pReturn="OK";

#ifdef _DEBUG
	fileRemove("c:\\SoapLastRequest.xml");
	fileRemove("c:\\SoapLastResponse.xml");
#endif

	pXmlEncode=!psSoapRequest->pXmlEncode?"utf-8":psSoapRequest->pXmlEncode;
	_(sWeb);
	if (psSoap->pUseUriAddress)
		sWeb.lpUri=psSoap->pUseUriAddress; //"http://wst.mobyws.it/MobyWebService/MobyService?WSDL";
		else
		sWeb.lpUri=psSoapRequest->pUriAddress; //"http://wst.mobyws.it/MobyWebService/MobyService?WSDL";
	sWeb.lpUserAgent=SOAP_USER_AGENT;
	sprintf(szContentType,"text/xml; charset=%s",pXmlEncode);
	sWeb.lpContentType=szContentType;

	// sWS.bCommToLog=TRUE; // Da Togliere
/*	
#ifdef _SSL_NO_AUTHENTICATION
	eh_ssl_client_context(&sWS,
		_SSL_NO_AUTHENTICATION,	// use SOAP_SSL_DEFAULT in production code 
		NULL, 		// keyfile: required only when client must authenticate to server (see SSL docs on how to obtain this file) 
		NULL, 		// password to read the keyfile 
		NULL,		// optional cacert file to store trusted certificates 
		NULL,		// optional capath to directory with trusted certificates 
		NULL		// if randfile!=NULL: use a file with random data to seed randomness 
		);
#endif
		*/
	//
	// Costruisco l'intestazione compresa di SoapAction ed eventuali Cookies
	//
	//sWeb.lpSendOtherHeader=ehAlloc(0xF000); *sWeb.lpSendOtherHeader=0;
	sWeb.lstHeader=lstNew();
	if (psSoap->pszPreHeader) lstPush(sWeb.lstHeader,psSoap->pszPreHeader);//strcat(sWeb.lpSendOtherHeader,psSoap->pszPreHeader);
	//sprintf(strNext(sWeb.lpSendOtherHeader),"SOAPAction: \"%s\"" CRLF,psSoapRequest->pSoapAction?psSoapRequest->pSoapAction:"");
	lstPushf(sWeb.lstHeader,"SOAPAction: \"%s\"",psSoapRequest->pSoapAction?psSoapRequest->pSoapAction:"");
	
	//
	// Costruisco la busta con intestazione di encoding
	//
	sWeb.lpPostArgs=ehAlloc(strlen(psSoapRequest->pSoapEnvelope)+1024);
	sprintf(sWeb.lpPostArgs,"<?xml version=\"1.0\" encoding=\"%s\" ?>" CRLF "%s",	
			pXmlEncode,
			psSoapRequest->pSoapEnvelope);

	//
	// Log della richiesta 
	//

#ifdef _DEBUG
	fileStrWrite("c:\\SoapLastRequest.xml",sWeb.lpPostArgs); // DA TOGLIERE
#endif

	if (psSoap->pszLogFolder)
	{
		CHAR szFileName[500];
		if (!fileCheck(psSoap->pszLogFolder)) CreateDirectory(psSoap->pszLogFolder,NULL);
		sprintf(szFileName,"%s\\%s_%03d_REQ.xml",psSoap->pszLogFolder,dtNow(),psSoap->dwComProgress++);
		fileStrWrite(szFileName,sWeb.lpPostArgs); // DA TOGLIERE
	}

	sWeb.pdmiCookie=&psSoap->dmiCookie;
	sWeb.iKeepAlive=true;
	strcpy(sWeb.szReq,"POST");
	sWeb.iTimeoutSec=psSoap->iTimeOutSec;
	psWeb=webHttpReqEx(&sWeb,false);
	/*
	if (psSoap->iTimeOutSec)
		bReturn=FWebGetDirectMT(&sWS,"POST",NULL,psSoap->iTimeOutSec,FALSE); // <------------------------ RICHIESTA AL WEB con Timeout------------------------
		else
		bReturn=FWebGetDirect(&sWS,"POST",NULL); // <------------------------ RICHIESTA AL WEB ------------------------
		*/
//	ehFree(psWeb->lpSendOtherHeader);
	ehFree(psWeb->lpPostArgs);

	//
	// Log della Risposta 
	//

	psSoap->iWebLastError=psWeb->sError.enCode;//iErrorCode; // 0=Tutto ok
	strAssign(&psSoap->pszLastHeaderResponse,psWeb->pPageHeader);
	strAssign(&psSoap->pszLastDataResponse,psWeb->pData);

	//
	// Ritorno con ERRORE
	//
 	if (psWeb->sError.enCode||strEmpty(psWeb->pData))
	{
		// Salvo codice errore WebGet e Header (non si sa mai)
#ifdef _DEBUG 
//		printf("FWebGetDirect Error %d" CRLF "%s",sWS.iErrorCode,sWS.pPageHeader);//,sWS.pPageHeader,strlen(sWS.pPageHeader));
#else
		ehLogWrite("Soap-WebError (a): URL:[%s] Timeout:%d/%d",psWeb->lpUri,psSoap->iTimeOutSec,psWeb->iTimeoutSec);//,sWS.pPageHeader,strlen(sWS.pPageHeader));
		ehLogWrite("Soap-WebError (b): (%d-%s) [%s]",psWeb->sError.enCode,webErrorString(psWeb->sError.enCode),psWeb->pPageHeader);//,sWS.pPageHeader,strlen(sWS.pPageHeader));
#endif
#ifdef _DEBUG
		if (psSoap->pszLastHeaderResponse) fileStrWrite("c:\\SoapLastResponse.xml",psSoap->pszLastHeaderResponse);
#endif

		if (psSoap->pszLogFolder)
		{
			CHAR szFileName[500];
			if (!fileCheck(psSoap->pszLogFolder)) CreateDirectory(psSoap->pszLogFolder,NULL);
			sprintf(szFileName,"%s\\%s_%03d_RES (err).xml",psSoap->pszLogFolder,dtNow(),psSoap->dwComProgress++);
			if (psSoap->pszLastHeaderResponse) fileStrWrite(szFileName,psSoap->pszLastHeaderResponse); // DA TOGLIERE
		}

		pReturn=FALSE;
	}
	else
	{
	//
	// Ritorno OK
	// Analizzo l'header per "ciucciarmi" i coockies ....
	//
		// web_cookie_get(&psSoap->dmiCookie,sWS.pPageHeader);

		//
		// Analizzo la pagina per creare la variabili e strutture di ritorno
		//

#ifdef _DEBUG
		fileStrWrite("c:\\SoapLastResponse.xml",psSoap->pszLastDataResponse);
#endif

		if (psSoap->pszLogFolder)
		{
			CHAR szFileName[500];
			if (!fileCheck(psSoap->pszLogFolder)) CreateDirectory(psSoap->pszLogFolder,NULL);
			sprintf(szFileName,"%s\\%s_%03d_RES.xml",psSoap->pszLogFolder,dtNow(),psSoap->dwComProgress++);
			fileStrWrite(szFileName,psSoap->pszLastDataResponse); // DA TOGLIERE
		}

		//
		// Trasformo response in variabili di ambiente (SOAP)
		//
  		soap_response_process(psSoap,psSoapRequest); // PROCESSA IL RESPONSE 
		if (psSoap->enSoapErrorCode!=SE_OK)  pReturn=NULL;
	//psSoap->
	}
	webHttpReqFree(psWeb);  
	psSoapRequest=soap_requestFree(psSoapRequest);
	return pReturn;
}
/*
static BOOL isDatatypePrimitive(CHAR *pType)
{
	INT a;
	for (a=0;arDataTypes[a];a++) {if (!strcmp(arDataTypes[a],pType)) return TRUE;}
	return FALSE;
}
*/

//
// addChildElements()
// Aggiunge elementi all'array degli elementi figli
//
static void addChildElements(EHS_ELEMENT *psElementParent,INT iNewElements)
{
	// Mi tengo dove sono gli elementi attuali
	EHS_ELEMENT *arBefore=psElementParent->arsElement;
	INT iElementsBefore=psElementParent->iElements;

	psElementParent->iElements=iElementsBefore+iNewElements;
	psElementParent->arsElement=ehAllocZero(sizeof(EHS_ELEMENT)*psElementParent->iElements);

	//psComplex=psVarElement->psComplex; iOffset=psComplex->iElements;
	//psComplexNew=addChildElements(iOffset+s);
	if (arBefore)
	{
		memcpy(psElementParent->arsElement,arBefore,sizeof(EHS_ELEMENT)*iElementsBefore);
		ehFree(arBefore); 
	}
}

//
// soap_sequence_translate() (* ricorsiva)
//
static void soap_sequence_translate(EH_SOAP *psSoap,EHS_ELEMENT *psVarElement,XMLELEMENT *pxmlSequence)
{
	XMLARRAY arElement;
	INT iOffset=0;
	INT iElements,s;
	EHS_SIMPLEDATATYPE *psSDT;
	EHS_ELEMENT *arsElement;
	BYTE *p;

	arElement=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,pxmlSequence->idx,".element");  // Estraggo array degli elementi
	
	// Conto gli elementi
	for (s=0;arElement[s];s++); 
	iElements=s; iOffset=0;

	//
	// Ho già un Complex (leggo e salvo i precendenti elementi)
	//
	if (psVarElement->arsElement) iOffset=psVarElement->iElements; // Preservo l'offset
	
	//
	// Creo l'array di elementi complessi
	//
	addChildElements(psVarElement,iElements);
	arsElement=psVarElement->arsElement; // Casting necessario

	//
	// Acquisizione degli elementi
	//
	for (s=0;s<iElements;s++) 
	{
		XNS_PTR xnsType,xnsRef,xnsName;
		INT idx=s+iOffset; // +Offset > perchè aggiungo elementi ad una struttura già esistente

		//
		// Leggo attributi elemento <----
		//
		xnsName=_L_GetAttrib(psSoap,"name",arElement[s]);
		xnsRef=_L_GetAttrib(psSoap,"ref",arElement[s]);
		xnsType=_L_GetAttrib(psSoap,"type",arElement[s]);

		if (xnsType)
		{
			psSDT=soap_SimpleTypeInfo(xnsType->pszElement);
		} 
		else psSDT=NULL;

		arsElement[idx].iType=STE_SIMPLE; // Elemento semplice di default
		arsElement[idx].xnsName=xnsDup(xnsName);
		arsElement[idx].xnsType=xnsDup(xnsType);
		

		if (xnsRef) 
		{
			arsElement[idx].iType=STE_REF; // Elemento è un di riferimento ad un altro
		}

		arsElement[idx].pMinOccurs=xmlGetAttribAlloc(arElement[s],"minOccurs",""); 
		arsElement[idx].pMaxOccurs=xmlGetAttribAlloc(arElement[s],"maxOccurs",""); 
		p=xmlGetAttribAlloc(arElement[s],"nillable","");
		if (!strCaseCmp(p,"true")) arsElement[idx].bNillable=TRUE;
		ehFree(p);

		if (atoi(arsElement[idx].pMaxOccurs)>1||!strcmp(arsElement[s].pMaxOccurs,"unbounded")) 
		{

			arsElement[idx].iArrayType=STA_ARRAY_ELEMENT;
			//
			// Il padre diventa un array
			//
//			psVarElement->iArrayType=STA_ARRAY;
		}
		
		//
		// Non è un elemento semplice
		//
		if (!psSDT&& 
			arsElement[idx].iType!=STE_REF) // Non è un riferimento
		{

			XMLELEMENT *pxmlComplexT;
//			if (arsElement[idx].iType!=STE_ARRAY) arsElement[idx].iType=STE_COMPLEX; // #2010
			arsElement[idx].iType=STE_COMPLEX;
			
			//
			// Non ho una tipologia (definito INSIDE)
			//
			if (!xnsType||!*xnsType->pszElement)
			{
				if (xnsName) // Se ho almeno il nome (no per gli oggetti Ref)
				{
					pxmlComplexT=xmlParser(&psSoap->xmlWSDL,WS_FIND,arElement[s]->idx,".complexType");
					soap_complexType_translate(psSoap,arsElement+idx,pxmlComplexT);
				}
			}
			else
			{
				pxmlComplexT=soap_complex_search(psSoap,xnsType->pszElement);  

				//
				// Chiedo di tradurre l'elemento complesso
				//
				if (pxmlComplexT)
				{
					soap_complexType_translate(psSoap,arsElement+idx,pxmlComplexT);
					pxmlComplexT=xmlElementFree(pxmlComplexT);
				}
				else 
				{
					ehExit(__FUNCTION__":Non ho trovato complexType [%s], line: %d",xnsType->pszElement,__LINE__);
				}
			}


		} // Tipo complessi > mi richiamo in modo ricorsivo
		else 
		{
			if (arsElement[idx].arsElement) ehError();
			arsElement[idx].iElements=0;
			arsElement[idx].arsElement=NULL;	
		}
		
		xnsDestroy(xnsName);
		xnsDestroy(xnsType);
		xnsDestroy(xnsRef);
	}
	arElement=xmlArrayFree(arElement); 
}

//
// soap_complexType_translate()  #########################################################################
// Traduce (riempe la truttura psVarElement) con un dato descritto <complexType> (XML in WSDL)
// 
// a) Se sono qui è perchè sono su un elemento complesso
// b) L'elemento complesso può essere definito in diversi modi
//   
//    Può contenere simpleContent oppure  complexContent oppure (group|all|choice|sequence)
//
// vedi: http://www.w3schools.com/Schema/el_complextype.asp
//
static void soap_complexType_translate(EH_SOAP *psSoap,EHS_ELEMENT *psVarElement,XMLELEMENT *pxmlComplexType)
{
	XMLELEMENT *pxmlChild;
	pxmlChild=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlComplexType->idx,"."); if (!pxmlChild) ehError();
	
	// complexContent
	if (!strcmp(pxmlChild->pName,"complexContent")) 
	{
		psVarElement->bComplexContent=TRUE;
		soap_complexContent_translate(psSoap,psVarElement,pxmlChild); 
		return;
	}

	// sequence
	if (!strcmp(pxmlChild->pName,"sequence")) 
	{
		soap_sequence_translate(psSoap,psVarElement,pxmlChild); 
		return; 
	}

	ehExit("soap_complexType_translate(): non implementato %s",pxmlChild->pName);
	// simpleContent  The complex type has character data or a simpleType as content and contains no elements, but may contain attributes.
	// group  The complex type contains the elements defined in the referenced group.
	// choice  The complex type allows one of the elements specified in the choice element.
	// all  The complex type allows any or all of the elements specified in the all element to appear once.

	/*
	//
	// sequence  > The complex type contiene elements definiti in una specifica sequenza.
	//
	pxmlComplexContent=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlComplexType->idx,".sequence"); 
	if (pxmlComplexContent)
	{
		soap_sequence_translate(psSoap,psVarElement,pxmlComplexContent);
		return; 
	}
*/
}

//
// soap_complexContent_translate() - http://www.w3schools.com/schema/el_complexcontent.asp
// Può contenere restriction|extension
//
static void soap_complexContent_translate(EH_SOAP *psSoap,EHS_ELEMENT *psVarElement,XMLELEMENT *pxmlComplexContent)
{
	XMLELEMENT *pxmlChild;
	//EHS_COMPLEX *psComplex=NULL;
	
	pxmlChild=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlComplexContent->idx,"."); if (!pxmlChild) ehError();

	//
	// extension > Aggiunge elementi ad una Complex già esistente (es. http://forum.springframework.org/showthread.php?t=25679 )
	//
	if (!strcmp(pxmlChild->pName,"extension")) 
	{
		XMLELEMENT *pxmlCopyElement;
		XNS_PTR xnsBase=xnsCreate(xmlGetAttrib(pxmlChild,"base"));
		// a - Cerco di cosa sono l'"extension"
		
		pxmlCopyElement=soap_complex_search(psSoap,xnsBase->pszElement);  // Inserisco la base
		if (!pxmlCopyElement) ehError();

		// b - Richiedo di ereditare "la forma" dal tipo indicato
		soap_complexType_translate(psSoap,psVarElement,pxmlCopyElement);

		// c - Aggiungo eventuali elementi 
		pxmlChild=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlChild->idx,".");  // Estraggo array degli elementi
		if (pxmlChild)
		{
			soap_sequence_translate(psSoap,psVarElement,pxmlChild);
		}

		// Libero risorse
		pxmlCopyElement=xmlElementFree(pxmlCopyElement);
		//SoapVariable(psSoap,WS_DISPLAY,NULL);
		xnsDestroy(xnsBase);
		return;
	}

	//
	// restriction - http://www.w3schools.com/Schema/el_restriction.asp
	// definisce una restrizione su simpleType,simpleContent o complexContent
	//
	if (!strcmp(pxmlChild->pName,"restriction")) 
	{
		soap_restriction_translate(psSoap,psVarElement,pxmlChild);
		return;
	}
	

	ehError(); // Non dovrebbe accadere

}
		//printf("[%s]",sNSArray.szName);

//
// restriction - http://www.w3schools.com/Schema/el_restriction.asp
// definisce una restrizione su simpleType,simpleContent o complexContent
//
static void soap_restriction_translate(EH_SOAP *psSoap,EHS_ELEMENT *psVarElement,XMLELEMENT *pxmlRestriction)
{
	EHS_SIMPLEDATATYPE *psSDT;
	XMLELEMENT *pxmlChild;
	XNS_PTR xnsBase=xnsCreate(xmlGetAttrib(pxmlRestriction,"base"));

 	if (!xnsBase) ehError();

	//
	// Definizione di un array -> L'elemento corrente mi diventa è un array
	//
	//  <restriction base="soapenc:Array"> <--- Definisce che è un array ---
    //   <attribute ref="soapenc:arrayType" wsdl:arrayType="tns1:Route[]"/> <--- Definisce che l'array è fatto con stutture di tipo Route ---
    //  </restriction>
	if (!strcmp(xnsBase->pszElement,"Array"))
	{
		pxmlChild=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlRestriction->idx,".");  // Leggo attributo array
		if (!strcmp(pxmlChild->pName,"sequence"))
		{
			if (!strEmpty(pxmlChild->lpValue)) ehError();
			pxmlChild=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlChild->idx,">");  // 2010
		}

		if (!strcmp(pxmlChild->pName,"attribute"))
		{
			BYTE *pszAttrib;
			//
			// Estraggo NS:Tipo dell'attributo interessato (in questo casso arrayType)
			//
			psVarElement->xnsArrayType=xnsCreate(xmlGetAttrib(pxmlChild,"ref")); 
			if (!psVarElement->xnsArrayType) ehError();
			soap_ns_reassign(psSoap,psVarElement->xnsArrayType,pxmlChild);

			//
			// Estraggo NS:Tipo degli elementi dell'array
			//
			// xnsArrayType=xmlGetAttribAlloc(pxmlChild,"arrayType",NULL); // sNSRef.szName ?
			psVarElement->iArrayType=STA_ARRAY;
			psVarElement->xnsType=xnsDestroy(psVarElement->xnsType); // libero il precedente
			pszAttrib=xmlGetAttrib(pxmlChild,"arrayType");
			psVarElement->xnsType=xnsCreate(pszAttrib); 
			if (!psVarElement->xnsType) ehError();
			strAssign(&psVarElement->xnsType->pszElement,strOmit(psVarElement->xnsType->pszElement,"[]")); 
			soap_ns_reassign(psSoap,psVarElement->xnsType,pxmlChild);

			//bDataPrimitive=isDatatypePrimitive(arsElement[0].xnsType->pszElement);
			psSDT=soap_SimpleTypeInfo(psVarElement->xnsType->pszElement); 
			if (!psSDT)
			{
				XMLELEMENT *pxmlChildElement;

				pxmlChildElement=soap_complex_search(psSoap,psVarElement->xnsType->pszElement);  
				if (pxmlChildElement)
				{
					soap_complexType_translate(psSoap,psVarElement,pxmlChildElement);
					pxmlChildElement=xmlElementFree(pxmlChildElement);
				}
				else ehExit(__FUNCTION__":Non ho trovato complexType [%s], line: %d",psVarElement->xnsType->pszElement,__LINE__);
				psVarElement->iType=STE_COMPLEX;
			}
			else
			{
				psVarElement->iType=STE_SIMPLE;
			}
			xnsDestroy(xnsBase);
			return;
		} 
		ehError();
	}
/*
		//
		// A) Creo un nuovo elemento che è un array 
		//		
		addChildElements(psVarElement,1);
		arsElement=psVarElement->arsElement;
		arsElement[0].iType=STE_ARRAY;
		arsElement[0].xnsType->pszElement=strDup(strOmit(sNSArray.szName,"[]"));
		arsElement[0].pszName=strDup(arsElement[0].xnsType->pszElement);

		//
		// B) Definisco l'elemento complesso dell'array creato
		//
		
		//bDataPrimitive=isDatatypePrimitive(arsElement[0].xnsType->pszElement);
		psSDT=soap_SimpleTypeInfo(arsElement[0].xnsType->pszElement); 
		if (!psSDT)
		{
			XMLELEMENT *pxmlChildElement;

			pxmlChildElement=soap_complex_search(psSoap,arsElement[0].xnsType->pszElement);  
			if (pxmlChildElement)
			{
				soap_complexType_translate(psSoap,arsElement,pxmlChildElement);
				pxmlChildElement=xmlElementFree(pxmlChildElement);
			}
			else ehExit(__FUNCTION__":Non ho trovato complexType [%s], line: %d",arsElement[0].xnsType->pszElement,__LINE__);
		}
		ehFree(pArrayType);
		*/

	//
	// Restriction incompleto per mancanza di informazioni ---
	//
	ehError();
//		if (atoi(arsElement[s].pMaxOccurs)>1||!strcmp(arsElement[s].pMaxOccurs,"unbounded")) 
	//psVarElement->psComplex=psComplex;
}

//
// soap_complex_search()
// Cerca negli <schema /> il tipo complesso
// Ritorna un elemento (DA liberare con xmlElementFree() che punta alla struttura complessa cercata
// 
static XMLELEMENT *soap_complex_search(EH_SOAP *psSoap,CHAR *pTypeName)
{
	XMLARRAY arComplexType;
	XMLELEMENT *pxmlComplexType=NULL;
	INT s;

	//
	// Loop sugli schemi presenti per cercare complexType
	//
	for (s=0;psSoap->arSchema[s];s++)
	{
		arComplexType=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,psSoap->arSchema[s]->idx,".complexType"); // Estraggo array degli elementi
		pxmlComplexType=xmlElementWithAttribute(arComplexType,"name",pTypeName,TRUE); // Ricerco complexType name=
		arComplexType=xmlArrayFree(arComplexType); 
		if (pxmlComplexType) break;
	}
	return pxmlComplexType;
}
//
// soap_element_builder()
// Costruisce la descrizione di una struttura complessa traendola da un elemento
// La funzione è ricorsiva (strutture in strutture)
// 
static XMLELEMENT *soap_element_search(EH_SOAP *psSoap,CHAR *pTypeName)
{
//	EHS_COMPLEX *psComplex=NULL;
	XMLELEMENT *pxmlElement,*pxmlComplexType;
	XMLARRAY arElement;
	INT s;
	// XNS_PTR xns;

	//
	// Loop sugli schemi presenti per cercare complexType
	//
	for (s=0;psSoap->arSchema[s];s++)
	{
		arElement=xmlParser(&psSoap->xmlWSDL,WS_PROCESS,psSoap->arSchema[s]->idx,".element");  // Estraggo array degli elementi

		pxmlElement=xmlElementWithAttribute(arElement,"name",pTypeName,TRUE);	// Ricerco complexType name=

		arElement=xmlArrayFree(arElement); 
		if (pxmlElement) break;
	}
	// xns=xnsCreate(xmlGetAttrib(pxmlElement,"name")); //if (!xns) return NULL;

	//
	// Trovato elemento
	//
	if (!pxmlElement) return NULL;

	pxmlComplexType=xmlParser(&psSoap->xmlWSDL,WS_FIND,pxmlElement->idx,".complexType");  // Estraggo array degli elementi
	xmlElementFree(pxmlElement);
	return xmlElementClone(pxmlComplexType);
//	return soap_complexType_translate(psSoap,pxmlComplexType);
}

//
//  Libero le risorse ------------------------------------------------------------------------------------
//
//  soap_childElements_free()
//
static void soap_childElements_free(EHS_ELEMENT *psElement)
{
	INT a;
	EHS_ELEMENT *arsElement;
	if (!psElement->arsElement) return;
	arsElement=psElement->arsElement;
	for (a=0;a<psElement->iElements;a++)
	{
		soap_element_free(&arsElement[a]);
	}
	//ehFree(arsElement);
	ehFreePtr(&psElement->arsElement);
//	psElement->arsElement=NULL;
	psElement->iElements=0;
}

//
// soap_element_free()
// Libera le risorse impegnate da un elemento
//

static EHS_ELEMENT *soap_var_free(EHS_VAR *psVar)
{
	soap_element_free(&psVar->sElement);
	soap_container_free(&psVar->sContainer);
	return NULL;
}

static void soap_varptr_free(EHS_VARPTR *psVar)
{
	soap_element_free(psVar->psElement); ehFree(psVar->psElement);
	soap_container_free(psVar->psContainer); ehFree(psVar->psContainer);
}

static EHS_ELEMENT *soap_element_free(EHS_ELEMENT *psElement)
{
	if (psElement->arsElement) soap_childElements_free(psElement); 

	ehFreePtr(&psElement->pszOriginalType);

	// Nome dell'elemento
	psElement->xnsName=xnsDestroy(psElement->xnsName);
	
	// Attributi
	psElement->xnsType=xnsDestroy(psElement->xnsType);
	psElement->xnsArrayType=xnsDestroy(psElement->xnsArrayType);

	// Caratteristiche per array
	ehFreePtr(&psElement->pMinOccurs);
	ehFreePtr(&psElement->pMaxOccurs);
//	ehFreePtr(&psElement->pNillable);

	if (psElement->bAlloc) ehFree(psElement);
	return NULL;
}

static EHS_CONTAINER *soap_container_free(EHS_CONTAINER *psContainer)
{
	EHS_CONTAINER *arContainer;
	EH_AR arJolly;
	INT a;
	switch (psContainer->iCntType)
	{
		case CTE_SIMPLE: 
			ehFreeNN(psContainer->pValue);
			break;

//		case STE_ELEMENT:
		case CTE_COMPLEX:
			arContainer=psContainer->pValue;
			for (a=0;a<psContainer->iLength;a++)
			{
				soap_container_free(arContainer+a);
				//ehFree(&arContainer[a]);
			}
			ehFree(psContainer->pValue);
			break;

		case CTE_ARRAY: 
			arJolly=(EH_AR) psContainer->pValue;
			for (a=0;a<psContainer->iLength;a++)
			{
				soap_container_free((EHS_CONTAINER *) arJolly[a]);
			}
			ARDestroy(arJolly);
			break;

		case CTE_EMPTY: // 12/2010
			break;

		default: 
			ehError(); 
			break;
	}
	return NULL;
}
/*
			arJolly=(EH_AR) psContainer->pValue;
			for (a=0;a<psContainer->iLength;a++)
			{
				EHS_CONTAINER *psNewContainer=(EHS_CONTAINER *) arJolly[a];
				psVirtualElement=L_ElementSemiClone(psElement);
				if (psElement->psComplex) psVirtualElement->iType=STE_COMPLEX; else psVirtualElement->iType=STE_SIMPLE;
				sprintf(szServ,"[%d]",a);
				soap_var_display(psSoap,psVirtualElement,psNewContainer,iDeep+1,DISPLAY_NODE,szServ);
				ehFree(psVirtualElement);

			}
			*/


//
// _LElementInfoGet()
// Ritorna il puntatore dell'elemento indicato con lo script e il puntatore ai dati che lo contengono
//
EHS_ELEMENT_INFO * _LElementInfoGet(EH_SOAP *psSoap,
								    CHAR *pScript,
								    BOOL bReadMode) // Solo lettura
{
	EHS_ELEMENT_INFO *psSED=ehAllocZero(sizeof(EHS_ELEMENT_INFO));
	EH_AR arPiece;
	INT a,e,iPiece;
	INT idxVar=-1;
	CHAR *p;
	INT iIndexValue=-1;

	EHS_ELEMENT	*	psFocusElement=NULL;
	EHS_CONTAINER *	psFocusContainer=NULL;
	EHS_ELEMENT *	psElementReturn=NULL;

	EH_AR arJolly;
	INT iExtraData=0;

	//EleType iTypeElement=0;

	psSED->pszErrorDesc=ehAllocZero(1024);
	arPiece=ARCreate(pScript,".",&iPiece);

	//
	// Loop sui pezzi della variabile (Es. arTel[0].doc[1].string)
	//
	for (e=0;e<iPiece;e++)
	{
		CHAR *pSearch;
		BOOL bIndex;
		p=strstr(arPiece[e],"["); 
		if (!p) 
		{
			pSearch=strDup(arPiece[e]); 
			bIndex=FALSE;
		}
		else 
		{
			BYTE *pIndex;
			pSearch=strTake(arPiece[e],p-1);
			bIndex=TRUE;
			pIndex=strExtract(arPiece[e],"[","]",FALSE,FALSE);
			if (!pIndex) 
			{
				psSED->iErrorCode=SVE_SINTAX_ERROR;
				sprintf(psSED->pszErrorDesc,"Errore di sintassi: '%s'",pScript); 
				ehFree(pSearch);
				break;			
			}
			iIndexValue=atoi(pIndex);
			ehFree(pIndex);
		}

		//
		// A) Ricerca nel primo elemento ---------------------------------------------------------------
		//
		if (!e) 
		{
			for (a=0;a<psSoap->dmiVar.Num;a++)
			{
//				if (!strcmp(psSoap->arVar[a].sElement.pszName,pSearch)) 
				if (!strcmp(psSoap->arVar[a].sElement.xnsName->pszElement,pSearch)) 
				{
					psFocusElement=&psSoap->arVar[a].sElement; // Puntatore all'elemento
					psFocusContainer=&psSoap->arVar[a].sContainer;  // Puntatore al suo contenuto
					idxVar=a; 
					break;
				}
			}		

			if (!psFocusElement)
			{
				psSED->iErrorCode=SVE_VAR_UNKNOW;
				sprintf(psSED->pszErrorDesc,"Variabile '%s' non trovata",pSearch); 
				ehFree(pSearch);
				break;
			}

			//
			// DEFINE - Gestione della marco 2010
			//
			if (psFocusElement->iType==STE_DEFINE)
			{
			
				//CHAR *pReal=psFocusElement->pszOriginalType;
				CHAR *pRealVar;
				strAssign(&arPiece[0],psFocusElement->pszOriginalType);
				pRealVar=ARToString(arPiece,".","","");
				psSED=_LElementInfoFree(psSED);
				psSED=_LElementInfoGet(psSoap,pRealVar,bReadMode);
				ehFree(pRealVar);
				ehFree(pSearch);
				break;

			}

		}
		//
		// B) Ricerca del membro (sotto elementi)
		//
		else
		{
			EHS_ELEMENT *psSubElement=NULL;
			EHS_CONTAINER *arContainer=NULL;
			EHS_ELEMENT *arsElement;
			if (psFocusContainer) arContainer=psFocusContainer->pValue; else arContainer=NULL;
			
			if (bReadMode) 
			{
				// Eccezioni
				if (!strcmp(pSearch,"_length")) 
				{
					iExtraData=1; goto AVANTI;
				}
			}

			//psComplex=psFocusElement->psComplex;
			arsElement=psFocusElement->arsElement;
			if (!arsElement) 
				{
					psSED->iErrorCode=SVE_NO_MEMBER;
					sprintf(psSED->pszErrorDesc,"'%s' non e' un membro di '%s', che e' un dato semplice (type:%s).",
						arPiece[e],psFocusElement->xnsName->pszElement,
						psFocusElement->xnsType->pszElement);
					ehFree(pSearch);
					break;
				} 

			for (a=0;a<psFocusElement->iElements;a++)
			{
//				if (!strcmp(arsElement[a].pszName,pSearch)) // Cerco il sotto elemento
				if (!strcmp(arsElement[a].xnsName->pszElement,pSearch)) // Cerco il sotto elemento
				{
					psSubElement=&arsElement[a];
					if (!arContainer) ehError();
					if (psFocusContainer) 
					{
						if (!psFocusContainer->iLength)
						{
							printf("L'array degli elementi è vuoto");
						}
					}
					psFocusContainer=arContainer+a;
					break;
				}
			}

			if (!psSubElement)
			{
				psSED->iErrorCode=SVE_MEMBER_UNKNOW;
				sprintf(psSED->pszErrorDesc,"'%s' non e' un membro di '%s' ",pSearch,psFocusElement->xnsName->pszElement);
				ehFree(pSearch);
				break;			
			}

			//
			// Trovato
			//
			psFocusElement=psSubElement;

		}

		//
		// C) Controllo coerenza elemento
		//

		//
		// NO ARRAY
		//
		if (!psFocusElement->iArrayType) 
		{
			if (bIndex)
			{
				// L'elemento NON è un array ma HO indicato l'indice
				psSED->iErrorCode=SVE_NO_ARRAY;
				sprintf(psSED->pszErrorDesc,"'%s' non e' un Array > '%s'",pSearch,psFocusElement->xnsName->pszElement);
				ehFree(pSearch);
				break;			
			}
		}

		//
		// ARRAY: Controlli in presenza elemento array/array element
		//
		psSED->iArrayType=STA_NOT_ARRAY;
		if (psFocusElement->iArrayType) 
		{
			psSED->iArrayType=psFocusElement->iArrayType;

			//
			// Non ho l'indice 
			//
			if (!bIndex) 
			{
				//if (psFocusElement->iArrayType==STA_ARRAY_ELEMENT) 
				psSED->iArrayType=STA_ARRAY;
				if (bReadMode)  // ma è un array e sono in lettura ? Ritorno che è un Array di elementi.
				{
					// psElementReturn=psFocusElement; ehFree(pSearch); break; // Se sono in lettura non do segnalazione di errore (ritornerà [Array])
					//psElementReturn=psFocusElement;
					//ehFree(pSearch);
					//break;
					goto AVANTI; // Potrei avere ._length dientro
				}

				psSED->iErrorCode=SVE_INDEX_NEEDED;
				sprintf(psSED->pszErrorDesc,"%s e' un Array; indice necessario. %s",pSearch,psFocusElement->xnsName->pszElement);
				ehFree(pSearch);
				break;			
			} 
			else
			{
				psSED->iArrayType=STA_ARRAY_ELEMENT;
			}

			if (iIndexValue<0) 
			{
				psSED->iErrorCode=SVE_INDEX_ERROR;
				sprintf(psSED->pszErrorDesc,"%s[], indice errato = %d > %s",pSearch,iIndexValue,pScript);
				ehFree(pSearch);
				break;			
			}

			//
			// Con l'indice devo ritornare i tipo dell'elemento
			//

			//
			// Se sono in lettura ed il dato (l'indice) non esiste
			//
			if (bReadMode)
			{
				if (iIndexValue>=psFocusContainer->iLength)
				{
					psSED->iErrorCode=SVE_INDEX_OVERSIZE; 
					sprintf(psSED->pszErrorDesc,"%s, indice [%d] (max %d) > %s",
							pSearch,
							iIndexValue,
							psFocusContainer->iLength,
							psFocusElement->xnsName->pszElement);
				}
			}

			if (iIndexValue>psFocusContainer->iLength) // ATTENZIONE: 1 più grande lo concedo per incrementare l'array
			{
				psSED->iErrorCode=SVE_INDEX_OVERSIZE; 
				sprintf(psSED->pszErrorDesc,"%s, indice troppo grande %d (max %d) > %s",
						pSearch,
						iIndexValue,
						psFocusContainer->iLength,
						psFocusElement->xnsName->pszElement);
				ehFree(pSearch);
				break;			
			}


			//
			// Controllo se l'indice rientra tra la memoria già allocata
			//
			if (iIndexValue<psFocusContainer->iLength)
			{
				arJolly=(EH_AR) psFocusContainer->pValue; 
				psFocusContainer=(EHS_CONTAINER *) arJolly[iIndexValue];
			}
			else
			{
				// psFocusContainer->pValue è un array di puntatori
				INT idxNow;
				EHS_ELEMENT *psVirtualElement;

				if (!bReadMode)
				{
					//
					// Aggiungo un elemento all'array ##########################
					//
					if (!psFocusContainer->pValue) psFocusContainer->pValue=ARNew();//NULL,NULL,NULL); // Creo se la prima volta l'array
					ARAdd((EH_AR *) &psFocusContainer->pValue,""); // Accodo un nuovo elemento all'array
					arJolly=psFocusContainer->pValue; 
					idxNow=psFocusContainer->iLength=ARLen(arJolly); idxNow--;
					ehFree(arJolly[idxNow]); // Libero la stringa creata da ARAdd
					arJolly[idxNow]=ehAllocZero(sizeof(EHS_CONTAINER));
					psFocusContainer=(EHS_CONTAINER *) arJolly[idxNow];
					psFocusContainer->iLength=idxNow+1;

					psVirtualElement=L_ElementSemiClone(psFocusElement);
					psVirtualElement->iArrayType=0;
					// if (psFocusElement->arsElement) psVirtualElement->iType=STE_COMPLEX; else psVirtualElement->iType=STE_SIMPLE;
					soap_element_alloc(psVirtualElement,psFocusContainer);
					ehFree(psVirtualElement);
				}
				else
				{	
					psFocusContainer=NULL; // Oversize array (solo in lettura)
				}
			}
		}

AVANTI:
		ehFree(pSearch);
		psElementReturn=psFocusElement;
//		iTypeElement=psFocusElement->iType;

	} // Loop sui pezzi

	//
	// Creo il virtual Element
	//
 	if (psElementReturn)
	{
		psSED->psVirtualElement=L_ElementSemiClone(psElementReturn);
		if (psFocusContainer) psSED->psVirtualElement->iType=psFocusContainer->iElementType; // ??
		psSED->psContainer=psFocusContainer;

		if (bReadMode)
		{
			//
			// Parametro aggiuntivo (Es. length)
			//
			if (iExtraData==1)
			{
//				printf("qui");
				sprintf(psSED->psContainer->szLength,"%d",psSED->psContainer->iLength);
				psSED->pStringValue=psSED->psContainer->szLength;
			}
			else
			{
				switch (psSED->psVirtualElement->iType)
				{
					case STE_SIMPLE: 
						if (psFocusContainer) psSED->pStringValue=psSED->psContainer->pValue; 
						break;// Trovo il valore dell'informazione attuale 

					case STE_ELEMENT: psSED->pStringValue="[Element]"; break;
					case STE_COMPLEX: psSED->pStringValue="[Object]"; break;
					//case STE_ARRAY:
					//	psSED->pStringValue="[Array]"; 
					//	break;
					default: psSED->pStringValue=NULL;
				}
			}
		}
	}

	ARDestroy(arPiece);
	return psSED;
}



//
// _LElementInfoFree()
// Rilascia le risorse di allocate ad un EHS_ELEMENT_INFO
//
void *_LElementInfoFree(EHS_ELEMENT_INFO *psSED)
{
	ehFreePtr(&psSED->pszErrorDesc);
	ehFreePtr(&psSED->psVirtualElement);
	ehFree(psSED);
	return NULL;
}

//
// L_ElementSemiClone()
// Clona l'elemento ma ATTENZIONE, non il suo contento: i puntatori rimangono li stessi del sorgente !!!!
//
void *L_ElementSemiClone(EHS_ELEMENT *psElementSource)
{
	EHS_ELEMENT *psReturn=ehAllocZero(sizeof(EHS_ELEMENT));
	if (!psElementSource) {psReturn->bAlloc=TRUE; return psReturn;}
	memcpy(psReturn,psElementSource,sizeof(EHS_ELEMENT)); psReturn->bAlloc=TRUE;
	return psReturn;
}

//
//   SoapVariableToXml()
//

/*

http://www.w3.org/TR/wsdl#_soap:body

L'attribute USE indica se le parti del messaggio sono codificati utilizzando alcune regole di codifica, o se le parti definiscono in concreto lo schema del messaggio.

If use is encoded, then each message part references an abstract type using the type attribute. 
These abstract types are used to produce a concrete message by applying an encoding specified by the encodingStyle attribute. 
The part names, types and value of the namespace attribute are all inputs to the encoding, although the namespace attribute only applies to content not explicitly defined by the abstract types. If the referenced encoding style allows variations in it’s format (such as the SOAP encoding does), then all variations MUST be supported ("reader makes right").

ENCODED
<soap:envelope>
    <soap:body>
        <myMethod>
            <x xsi:type="xsd:int">5</x>
            <y xsi:type="xsd:float">5.0</y>
        </myMethod>
    </soap:body>
</soap:envelope>

LITERAL
<soap:envelope>
    <soap:body>
        <myMethod>
            <x>5</x>
            <y>5.0</y>
        </myMethod>
    </soap:body>
</soap:envelope>

*/

//
// SoapVariableToXml()
//
static BOOL SoapVariableToXml(EH_SOAP *psSoap,	// Soap Section
							  EHS_VARPTR *psVar)// Variabile
{
//	BYTE *psNSParent;
	EHS_CONTAINER *arContainer;
	EH_AR arJolly;
	INT a;
	EHS_ELEMENT *psVirtualElement;
	EHS_VARPTR sVar;

	BOOL bEmpty;
	EHS_SIMPLEDATATYPE *psSDT;
	EHS_ELEMENT *arsElement;
	BOOL bWriteRet=FALSE; // Se ho scritto qualcosa
	BYTE *psNS;
//	psNSParent=psVar->psElement->xnsType->pszSpace;
	XNS_PTR xnsParent;
	xnsParent=psVar->psElement->xnsName; // Cerco il name space del parent

	while (TRUE)
	{
		//
		// Scrittura dell'ARRAY XML <-----------------------------------------------------------------
		//
		if (psVar->psElement->iArrayType&&
			psVar->psContainer->iCntType==CTE_ARRAY)
		{
			BOOL bClose=FALSE;
			if (psVar->psElement->xnsArrayType)//bArrayTypeAttrib)
			{
				CHAR *pTag=_XNSToString(psSoap,psVar->psElement->xnsName); 
				CHAR *pType=_XNSToString(psSoap,psVar->psElement->xnsType); 
				CHAR *pArrayType=_XNSToString(psSoap,psVar->psElement->xnsArrayType); 
				CHAR szServ[200];

				if (!psVar->psContainer->iLength&&
					psVar->psElement->bNillable) // Vuoto
				{
					sprintf(szServ,"<%s %s=\"%s[%d]\" />",
								pTag,pArrayType,pType,psVar->psContainer->iLength);
				}
				else
				{
					sprintf(szServ,"<%s %s=\"%s[%d]\">",
								pTag,pArrayType,pType,psVar->psContainer->iLength);
					bClose=TRUE;
				}
				ARAddarg(&psSoap->arXML,"%s%s",_LIndentSpace(psSoap),szServ);
				bWriteRet|=1;

				ehFree(pTag);
				ehFree(pArrayType);
				ehFree(pType);
			}

			arJolly=(EH_AR) psVar->psContainer->pValue;
			psSoap->iDeep++;
			for (a=0;a<psVar->psContainer->iLength;a++)
			{
				EHS_CONTAINER *psNewContainer=(EHS_CONTAINER *) arJolly[a];
				psVirtualElement=L_ElementSemiClone(psVar->psElement);
				// if (psVar->psElement->arsElement) psVirtualElement->iType=STE_COMPLEX; else psVirtualElement->iType=STE_SIMPLE;
				sVar.psElement=psVirtualElement;
				sVar.psContainer=psNewContainer;
				bWriteRet|=SoapVariableToXml(psSoap,&sVar);//,NULL);// psVar->psElement->pszNSType);
				ehFree(psVirtualElement);
			}
			psSoap->iDeep--;

			if (bClose)
			{
				if (psVar->psElement->xnsArrayType)
				{
					ARAddarg(&psSoap->arXML,"%s",_TagBuild(psSoap,TAG_CLOSE,psVar->psElement));//psNSParent,psVar->psElement->pszName));
				}
			}

			break;
		}

		switch (psVar->psElement->iType)
		{
			case STE_SIMPLE:
				
				psSDT=soap_SimpleDataTypeCheck(psVar->psElement->xnsType->pszElement,psVar->psElement->xnsName->pszElement);
				bEmpty=strEmpty(psVar->psContainer->pValue);
				if (bEmpty)
				{
					// Se vuoto controllo se devo inserirlo
					if (!psVar->psElement->pMinOccurs) break;
					if (atoi(psVar->psElement->pMinOccurs)<1) break;
				}
				bWriteRet=TRUE;
				(*psSDT->funcEncode)(psSoap,SE_VAR_TO_XML,psVar->psElement,psVar->psContainer->pValue); // <-- Funzione associata di encoding
				break;

			case STE_ELEMENT:
				arsElement=psVar->psElement->arsElement;
				arContainer=psVar->psContainer->pValue;
				for (a=0;a<psVar->psElement->iElements;a++)
				{
					sVar.psElement=arsElement+a;
					sVar.psContainer=arContainer+a;
					if (xnsParent) strAssign(&sVar.psElement->xnsName->pszSpace,xnsParent->pszSpace); // <--- Cambio (per eredita) il name space all'elemento figlio
					bWriteRet|=SoapVariableToXml(psSoap,&sVar);//,NULL);//psVar->psElement->pszNSType);
				}
				break;

			case STE_COMPLEX: // Struttura

				ARAddarg(&psSoap->arXML,"%s",_TagBuild(psSoap,TAG_OPEN,psVar->psElement));
				arsElement=psVar->psElement->arsElement;
				arContainer=psVar->psContainer->pValue;
				psSoap->iDeep++;
				for (a=0;a<psVar->psElement->iElements;a++)
				{
					sVar.psElement=arsElement+a;
					sVar.psContainer=arContainer+a;
					if (psVar->psElement->xnsType) strAssign(&sVar.psElement->xnsName->pszSpace,psVar->psElement->xnsType->pszSpace); // <--- Cambio (per eredita) il name space all'elemento figlio
					bWriteRet|=SoapVariableToXml(psSoap,&sVar);//,NULL);//psVar->psElement->pszNSType);
				}
				psSoap->iDeep--;

				// Vuoto
				if (!bWriteRet)
				{
					//strcpy(pPoint," />" CRLF);
					psNS=strDup(_TagBuild(psSoap,TAG_NULL,psVar->psElement));/// psNSParent,pszName));
					ARUpdate(psSoap->arXML,ARLen(psSoap->arXML)-1,"%s",psNS);
					//psNS=_LNSTagName(psNSParent,psVar->psElement->pszName);
					//ARUpdate(psSoap->arXML,ARLen(psSoap->arXML)-1,"%s<%s />",_LIndentSpace(psSoap),psNS);
					ehFree(psNS);
					 
					break;
				}

				ARAddarg(&psSoap->arXML,"%s",_TagBuild(psSoap,TAG_CLOSE,psVar->psElement));//psNSParent,psVar->psElement->pszName));
				break;
		}
		break;
	}

	return bWriteRet;
}

//
// soap_var_display()
//
void soap_var_display(EH_SOAP *psSoap,INT iMode,EHS_ELEMENT *psElement,EHS_CONTAINER *psContainer,CHAR *pAdd,CHAR *pIndex)
{
	EHS_CONTAINER *arContainer;
	EH_AR arJolly;
	INT a;
	EHS_ELEMENT *psVirtualElement;
	CHAR szServ[20];
	EHS_SIMPLEDATATYPE *psSDT;
	BYTE *pValueForVar;
	EHS_ELEMENT *arsElement;


	if (psElement->iArrayType)//&&psContainer->iCntType==CTE_ARRAY)
	{
		switch (iMode)
		{
			case 0: // struttura
				/*
				fprintf(psSoap->chOutput,"%s %s[]" CRLF,
						psElement->xnsType->pszElement,
						psElement->pszName);
*/
				psVirtualElement=L_ElementSemiClone(psElement);
				psVirtualElement->iArrayType=0;
				psSoap->iDeep++;
				soap_var_display(psSoap,iMode,psVirtualElement,NULL,DISPLAY_NODE,"[]"); 
				psSoap->iDeep--;
				ehFree(psVirtualElement);
				break;

			case 1: // Valori
				fprintf(psSoap->chOutput,"%s%s",_LIndentSpace(psSoap),pAdd);
				if (!psSoap->dwDisplayParam) fprintf(psSoap->chOutput,"[array %s] ",psElement->xnsType->pszElement);
				fprintf(psSoap->chOutput,"%s[]._length=%d" CRLF,
						psElement->xnsName->pszElement,
						//pValue,
						psContainer->iLength);
				arJolly=(EH_AR) psContainer->pValue;
				for (a=0;a<psContainer->iLength;a++)
				{
					EHS_CONTAINER *psNewContainer=(EHS_CONTAINER *) arJolly[a];
					psVirtualElement=L_ElementSemiClone(psElement);
					//if (psElement->arsElement) psVirtualElement->iType=STE_COMPLEX; else psVirtualElement->iType=STE_SIMPLE;
					psVirtualElement->iArrayType=0;
					sprintf(szServ,"[%d]",a);
					soap_var_display(psSoap,iMode,psVirtualElement,psNewContainer,DISPLAY_NODE,szServ); // DISPLAY_NODE
					ehFree(psVirtualElement);

				}
				break;
		}
		return;
	}

	fprintf(psSoap->chOutput,"%s%s",_LIndentSpace(psSoap),pAdd);
	switch (psElement->iType)
	{
		case STE_SIMPLE:
			psSDT=soap_SimpleDataTypeCheck(psElement->xnsType->pszElement,psElement->xnsName->pszElement);
			if (iMode) pValueForVar=(*psSDT->funcEncode)(psSoap,SE_VAR_TO_STR,NULL,psContainer->pValue); else pValueForVar=NULL;
			if (!psSoap->dwDisplayParam) fprintf(psSoap->chOutput,"[%s] ",psElement->xnsType->pszElement);
			if (!pValueForVar) pValueForVar=strDup("(null)");
			else 
				//if (!strcmp(psElement->xnsType->pszElement,"string"))
				if (L_NumberString(psElement->xnsType->pszElement)==TP_CSTRING)
				{
					BYTE *p=ehAlloc(strlen(pValueForVar)+4);
					sprintf(p,"'%s'",pValueForVar);
					strAssign(&pValueForVar,p); ehFree(p);
				}
			switch (iMode)
				{
					case 0:
						fprintf(psSoap->chOutput,"[%s] %s" CRLF,psElement->xnsType->pszElement,psElement->xnsName->pszElement);
						break;

					case 1:
						if (*pIndex)
							fprintf(psSoap->chOutput,"%s%s =%s" CRLF,psElement->xnsName->pszElement,pIndex,pValueForVar);
							else
							fprintf(psSoap->chOutput,"%s=%s" CRLF,psElement->xnsName->pszElement,pValueForVar);
						break;
				}
			ehFree(pValueForVar);
			break;

		case STE_ELEMENT:
		case STE_COMPLEX:

			if (!psSoap->dwDisplayParam||!iMode) fprintf(psSoap->chOutput,"[%s %s] ",(psElement->iType==STE_ELEMENT)?"element":"complex",psElement->xnsType->pszElement);
			fprintf(psSoap->chOutput,"%s%s" CRLF,psElement->xnsName->pszElement,pIndex);
			arsElement=psElement->arsElement;
			if (iMode) arContainer=psContainer->pValue;
			psSoap->iDeep++;
			for (a=0;a<psElement->iElements;a++)
			{
				soap_var_display(psSoap,iMode,arsElement+a,iMode?(arContainer+a):NULL,DISPLAY_NODE,"");
			}
			psSoap->iDeep--;
			break;
		
		case STE_DEFINE:
			fprintf(psSoap->chOutput,"macro %s %s" CRLF,psElement->xnsName->pszElement,psElement->pszOriginalType);
			break;
	}
}


//
// SoapAddStruct()
//
void SoapAddStruct(EH_SOAP *psSoap,EHS_ELEMENT *psElement,EH_AR *arClip,CHAR *pPrefix,BOOL bArray)
{
	INT a;
	EHS_ELEMENT *psVirtualElement;
	EHS_ELEMENT *arsElement;
	CHAR *pBuffer=ehAlloc(1024); 
	*pBuffer=0;
	if (pPrefix) strcpy(pBuffer,pPrefix);


	if (psElement->iArrayType)
	{
		psVirtualElement=L_ElementSemiClone(psElement);
		if (psElement->arsElement) 
		{
			psVirtualElement->iType=STE_COMPLEX; 
			//sprintf(pBuffer+strlen(pBuffer),"%s[].",psElement->pszName);
		}
		else 
		{
			//sprintf(pBuffer+strlen(pBuffer),"%s[]",psElement->pszName);
			psVirtualElement->iType=STE_SIMPLE;
			//ARAdd(arClip,pBuffer);
		}
		SoapAddStruct(psSoap,psVirtualElement,arClip,pBuffer,TRUE);
		ehFree(psVirtualElement);
	}
	else
	{
		switch (psElement->iType)
		{
			case STE_SIMPLE:
				sprintf(pBuffer+strlen(pBuffer),"%s",psElement->xnsName->pszElement);
				if (bArray) strcat(pBuffer,"[]");
				ARAdd(arClip,pBuffer);
				break;

			case STE_ELEMENT:
			case STE_COMPLEX:
				sprintf(pBuffer+strlen(pBuffer),"%s",psElement->xnsName->pszElement);
				if (bArray) strcat(pBuffer,"[]");
				strcat(pBuffer,".");
				//psComplex=psElement->psComplex;
				arsElement=psElement->arsElement;
				for (a=0;a<psElement->iElements;a++)
				{
					SoapAddStruct(psSoap,arsElement+a,arClip,pBuffer,FALSE);
				}
				break;
		}
	}
	ehFree(pBuffer);
}


//
// SoapVarAdd()
// Aggiunge una variabile (elemento) nell'ambiente
//
static void * SoapVarAdd(EH_SOAP *psSoap,CHAR *pScript)
{
	EH_AR arToken;
	INT iToken;
	EleType iObjectType;
	CHAR *pObject;
	BYTE *p,*pVarName,*pTypeIn;
//	S_XNS_PTR sNSType;
	BOOL bArray;
	INT a;
	EHS_VAR sSoapVar;
	XMLELEMENT *pxmlComplexType;
	EHS_SIMPLEDATATYPE *psSDT;
	XNS_PTR xnsType=NULL;
	BYTE *pRet=NULL;

	memoUnlock(psSoap->dmiVar.Hdl);
	arToken=ARCreate(pScript," ",&iToken);
	iObjectType=STE_UNKNOW;

	switch (iToken)
	{
		case 2: // Dichiarazione di tipo primitivo (Tipo) (nome)
			iObjectType=STE_SIMPLE;
			pObject=NULL;
			pTypeIn=strDup(strTrim(arToken[0]));
			pVarName=strDup(strTrim(arToken[1]));
			xnsType=xnsCreate(pTypeIn);
			psSDT=soap_SimpleTypeInfo(xnsType->pszElement);
			if (!psSDT) 
			{
				ehExit("Tipo sconosciuto '%s' su dichiarazione variabile '%s' ?",xnsType->pszElement,pVarName);
			}
			
			break;

		case 3: // Dichiarazione avanzata (cosa) (Tipo) (nome)
			pObject=strDup(strTrim(arToken[0])); 
			pTypeIn=strDup(strTrim(arToken[1]));
			pVarName=strDup(strTrim(arToken[2]));
			xnsType=xnsCreate(pTypeIn);
			
			if (!strcmp(pObject,"primitive"))	iObjectType=STE_SIMPLE;
			if (!strcmp(pObject,"complex"))		iObjectType=STE_COMPLEX;
			if (!strcmp(pObject,"element"))		iObjectType=STE_ELEMENT;


			//
			// #define Macro simile al C++
			//
			if (!strcmp(pObject,"#define")) 
			{
				iObjectType=STE_DEFINE;
				ehFree(pTypeIn); ehFree(pVarName);
				pVarName=strDup(strTrim(arToken[1]));
				pTypeIn=strDup(strTrim(arToken[2]));
			}
			break;

		default:
			//ARDestroy(arToken);
			printf("?");
			break;
	}
	ARDestroy(arToken);

	if (iObjectType==STE_UNKNOW) //{psSoap->iErrorCode=SE_SINTAX_ERROR; ARDestroy(ar); break;}
	{
		ehExit("Errore di sintassi in aggiunta variabile '%s' ?",pScript);
	}

	//
	// Identifico tipo e nome
	//
	p=strstr(pVarName,"[]"); if (p) {bArray=TRUE; *p=0;} else bArray=FALSE;

	//
	// Controllo se già esiste
	//
	for (a=0;a<psSoap->dmiVar.Num;a++)
	{
		DMIRead(&psSoap->dmiVar,a,&sSoapVar);
		if (!strcmp(sSoapVar.sElement.xnsName->pszElement,pVarName)) ehExit("La variabile '%s' e' gia' stata dichiarata",pVarName);
	}
	
	ZeroFill(sSoapVar);
	sSoapVar.sElement.iType=iObjectType;
	sSoapVar.sElement.pszOriginalType=strDup(pTypeIn);
	sSoapVar.sElement.xnsName=xnsCreate(pVarName);//ehAllocZero(sizeof(S_XNS));
	sSoapVar.sElement.xnsType=xnsDup(xnsType);
	if (bArray) sSoapVar.sElement.iArrayType=STA_ARRAY;

	if (iObjectType!=STE_SIMPLE)
	{
			psSDT=soap_SimpleTypeInfo(sSoapVar.sElement.xnsType->pszElement); 
			if (psSDT) ehExit("Errore in dichiarazione '%s'",pScript);
	}

	switch (iObjectType)
	{
		case STE_SIMPLE:
//			if (bArray) sSoapVar.sElement.iType=STE_ARRAY;
			break;

		case STE_COMPLEX:

			// Cerco il complexType negli schemi
			pxmlComplexType=soap_complex_search(psSoap,sSoapVar.sElement.xnsType->pszElement); 	
			if (!pxmlComplexType) ehExit(__FUNCTION__":Non ho trovato complexType [%s]",sSoapVar.sElement.xnsType->pszElement);

			// Traduce l'XML di definizione, in una variabile (Struttura) in memoria 
			soap_complexType_translate(psSoap,&sSoapVar.sElement,pxmlComplexType);
			pxmlComplexType=xmlElementFree(pxmlComplexType);
			break;

		case STE_ELEMENT:

			pxmlComplexType=soap_element_search(psSoap,sSoapVar.sElement.xnsType->pszElement); 
			//soap_ns_reassign(psSoap,xns,pxml);
			//pxmlComplexType

			if (!pxmlComplexType) ehExit(__FUNCTION__":Non ho trovato element [%s]",sSoapVar.sElement.xnsType->pszElement);
			soap_complexType_translate(psSoap,&sSoapVar.sElement,pxmlComplexType);
			pxmlComplexType=xmlElementFree(pxmlComplexType);
//			sSoapVar.sElement.psComplex=soap_element_builder(psSoap,sNSType.szName);
			break;
	}

	//
	// Alloco la memoria necessaria all'elemento
	//
	if (soap_var_alloc(&sSoapVar)) pRet="Error";
	DMIAppendDyn(&psSoap->dmiVar,&sSoapVar);

	ehFreePtr(&pObject);
	ehFreePtr(&pVarName);
	ehFreePtr(&pTypeIn);
	xnsDestroy(xnsType);

	psSoap->arVar=memoLock(psSoap->dmiVar.Hdl);
	return pRet;
}

//
// SoapVarAdd()
// Aggiunge una variabile (elemento) nell'ambiente
//
static void *SoapVarSet(EH_SOAP *psSoap,CHAR *pScriptOriginal)
{
	CHAR *pScript;
	BYTE *p,*pVarName,*pVarValue;
	EHS_ELEMENT_INFO *psSED;
	BYTE *pszValue=NULL;

	pScript=strDup(pScriptOriginal);
	p=strstr(pScript,"="); if (!p) {ehExit("Errore di sintassi in assegnazione variabile\n" "? %s ?",pScriptOriginal);}
	pVarName=strTake(pScript,p-1); strcpy(pVarName,strTrim(pVarName));
	pVarValue=strDup(p+1); strcpy(pVarValue,strTrim(pVarValue));

	//
	// Traduco l'indicazione della variabile in un SED data (mi ritorna informazioni e posizione in memoria del dato da scrivere)
	//
	// if (strstr(pVarName,"[3]")) printf("OK");
	psSED=_LElementInfoGet(psSoap,pVarName,FALSE); 
	if (psSED->iErrorCode) 
	{
		// Siamo in presenza di un valore Null
		if (!strcmp(pVarValue,"NULL"))//psSED->iErrorCode==SVE_INDEX_NEEDED)
		{
			BOOL bArrayNull=FALSE;

			if (psSED->psVirtualElement->iArrayType==STA_ARRAY) bArrayNull=TRUE;
			if (psSED->iArrayType==STA_ARRAY&&psSED->psVirtualElement->bNillable) 
			{
				bArrayNull=TRUE;
			}

			if (bArrayNull)
			{
				// Libero la memoria dell'array e metterlo a 0 !!!!!!!!!!!
				soap_container_free(psSED->psContainer);
				psSED->psContainer->pValue=ARNew();
				psSED->psContainer->iLength=0;
				goto AVANTI;
			}
		}

		ehExit("Errore: %d : %s, %s",psSED->iErrorCode,psSED->pszErrorDesc,pScriptOriginal);
	}

	// Pulisco creo elemento
	if (psSED->iArrayType==STA_ARRAY_ELEMENT&&
		!strcmp(pVarValue,"NULL"))
	{
		// Libero la memoria dell'array e metterlo a 0 !!!!!!!!!!!
		//soap_container_free(psSED->psContainer);
//		psSED->psContainer->pValue=ARNew();
		//psSED->psContainer->iLength=0;
		goto AVANTI;

	}

	if (psSED->iArrayType==STA_ARRAY)
	{
		ehExit("%s : '%s' e' un array non modificabile",pScriptOriginal,psSED->psVirtualElement->xnsName->pszElement);
	}

	switch (psSED->psVirtualElement->iType)
	{
		case STE_SIMPLE:
			//
			// Prendo il valore
			// DA FARE: aggiungere controllo di sintassi sul valore e controllo coerenza valore
			//

			if (!isNaN(pVarValue)) //strOnlyNumber(pVarValue)) 
				pszValue=strDup(pVarValue); 
				else
				pszValue=strCTake(pVarValue);

			if (!pszValue) 
			{
				pszValue=strDup(pVarValue); // In caso di assegnazione di alias
				if (!strcmp(pszValue,"NULL")) {ehFreePtr(&pszValue); pszValue=NULL;}
			}

			strAssign((BYTE **) &psSED->psContainer->pValue,pszValue);
			if (!psSED->psContainer->pValue) psSED->psContainer->iLength=0; else psSED->psContainer->iLength=strlen(psSED->psContainer->pValue);
			break;

		case STE_COMPLEX:
			if (!strcmp(pVarValue,"NULL")) break;
			ehExit("%s : '%s' e' un dato complesso non modificabile",pScriptOriginal,psSED->psVirtualElement->xnsName->pszElement);
			break;

		case STE_ELEMENT:
			if (!strcmp(pVarValue,"NULL")) break;
 			ehExit("%s : '%s' e' un elemento non modificabile",pScriptOriginal,psSED->psVirtualElement->xnsName->pszElement);
			break;

		default: ehError();

	}

AVANTI:
	//
	// Libero le risorse
	//
	psSED=_LElementInfoFree(psSED);
	ehFreeNN(pszValue);
	ehFree(pVarValue);
	ehFree(pVarName);
	ehFree(pScript);
	return NULL;
}

//
// SoapVariable() > Gestore delle variabili di ambiente
//
void * SoapVariable(EH_SOAP *psSoap,INT cmd,void *ptr)
{
	void * pReturn=NULL;
	EHS_VAR sSoapVar;
	INT a,idx;
	BOOL bOldIndent;
	BYTE *pCode=ptr;

	BYTE *p,*pVarName;//,*pVarValue;
	EHS_ELEMENT_INFO *psSED;
	EH_AR arClip;

	switch (cmd)
	{
		case WS_OPEN:
			DMIReset(&psSoap->dmiVar);
			DMIOpen(&psSoap->dmiVar,RAM_AUTO,50,sizeof(EHS_VAR),"SoapVariable");
			psSoap->arVar=memoLock(psSoap->dmiVar.Hdl);
			break;

		//
		// Aggiunge una variabile (o oggetto/elemento) ####################################################
		//
		case WS_PROCESS:
			if (strstr(ptr,"=")) // Ci vorrebbe un parser fatto meglio (ci vorrebbe un parser!), ma per ora facciamo così
				SoapVarSet(psSoap,pCode);
				else
				pReturn=SoapVarAdd(psSoap,pCode);
			break;

		//
		// Assegna il valore ad variabile o membro
		//
		case WS_REALGET:

			pVarName=strDup(ptr);
			p=strstr(pVarName,"="); if (p) {ehExit("Errore di sintassi in richiesta variabile\n" "? %s ?",ptr);}
			strTrim(pVarName);

			//
			// Traduco l'indicazione della variabile in un SED data (mi ritorna informazioni e posizione in memoria del dato da leggere)
			//
			psSED=_LElementInfoGet(psSoap,pVarName,TRUE); 
			if (psSED->iErrorCode) 
			{
				printf("Variable: %s" CRLF,pVarName);
				// SoapVariable(psSoap,WS_DISPLAY,NULL);
				ehExit("Errore: %d : %s",psSED->iErrorCode,psSED->pszErrorDesc);
			}
			pReturn=psSED->pStringValue; // Puntatore da NON LIBERARE (REALE)
 			psSED=_LElementInfoFree(psSED);
			ehFree(pVarName);
			break;

		//
		// Cancello le risorse impegnate da una variabile 
		//
		case WS_DEL:
			memoUnlock(psSoap->dmiVar.Hdl);			
			idx=-1;
			for (a=0;a<psSoap->dmiVar.Num;a++)
			{
				DMIRead(&psSoap->dmiVar,a,&sSoapVar);
 				if (!strcmp(sSoapVar.sElement.xnsName->pszElement,ptr)) {idx=a; break;}
			}
			if (idx<0) 
			{
				psSoap->arVar=memoLock(psSoap->dmiVar.Hdl);
				return "error";//ehExit("Variabile, %s non trovata in cancellazione",ptr);
			}

			soap_var_free(&sSoapVar);
			//soap_element_free(&sSoapVar.sElement);
			//soap_container_free(&sSoapVar.sContainer);
			DMIDelete(&psSoap->dmiVar,idx,NULL);
			psSoap->arVar=memoLock(psSoap->dmiVar.Hdl);			
			break;

		case WS_DESTROY:
			for (a=0;a<psSoap->dmiVar.Num;a++)
			{
				DMIRead(&psSoap->dmiVar,a,&sSoapVar);
				SoapVariable(psSoap,WS_DEL,sSoapVar.sElement.xnsName->pszElement); a--;
			}
			break;

		//
		// Visualizza la STRTTURA delle variabili in memoria
		//
		case WS_DOLIST:
			SoapOutputFile(psSoap,WS_OPEN,ptr);
			fprintf(psSoap->chOutput,"Variable Struct list ----------------------" CRLF);
			fprintf(psSoap->chOutput,"WSDL: %s" CRLF,psSoap->pszWSDLSource);
			if (*psSoap->pszLastOperation) fprintf(psSoap->chOutput,"Last operation: %s | %s" CRLF CRLF,psSoap->pszLastOperation,dtView(psSoap->pszLastOperationTime));
			for (a=0;a<psSoap->dmiVar.Num;a++)
			{
				DMIRead(&psSoap->dmiVar,a,&sSoapVar);
				psSoap->iDeep=0;
				soap_var_display(psSoap,0,&sSoapVar.sElement,&sSoapVar.sContainer,DISPLAY_NODE,"");
				fprintf(psSoap->chOutput,CRLF);
			}
			SoapOutputFile(psSoap,WS_CLOSE,NULL);
			break;

		//
		// Visualizza i valore delle variabili
		//
		case WS_PRINT:
		case WS_DISPLAY:
			
			bOldIndent=psSoap->sXmlRemove.bIndent;
			psSoap->sXmlRemove.bIndent=FALSE;
			SoapOutputFile(psSoap,WS_OPEN,ptr);
			psSoap->dwDisplayParam=(cmd==WS_DISPLAY)?1:0;
			
			fprintf(psSoap->chOutput,"Variable list" CRLF);
			fprintf(psSoap->chOutput,"WSDL: %s" CRLF,psSoap->pszWSDLSource);
			if (*psSoap->pszLastOperation) fprintf(psSoap->chOutput,"Last operation: %s | %s" CRLF CRLF,psSoap->pszLastOperation,dtView(psSoap->pszLastOperationTime));
			for (a=0;a<psSoap->dmiVar.Num;a++)
			{
				DMIRead(&psSoap->dmiVar,a,&sSoapVar);
				psSoap->iDeep=0;
				soap_var_display(psSoap,1,&sSoapVar.sElement,&sSoapVar.sContainer,DISPLAY_NODE,"");
				fprintf(psSoap->chOutput,CRLF);
			}
			SoapOutputFile(psSoap,WS_CLOSE,NULL);
			psSoap->sXmlRemove.bIndent=bOldIndent;
			break;


		//
		// Copia la struttura (di una determinata variabile o di tutte) nel clipboard
		//
		case WS_LINEEDIT:
			arClip=ARNew();
			for (a=0;a<psSoap->dmiVar.Num;a++)
			{
				DMIRead(&psSoap->dmiVar,a,&sSoapVar);
				if (ptr) {if (strcmp(sSoapVar.sElement.xnsName->pszElement,ptr)) continue;}
				SoapAddStruct(psSoap,&sSoapVar.sElement,&arClip,"",FALSE);
			}

			// Calcolo dimensioni
			{
				DWORD iSize=0;
				BYTE *pString;
				for (a=0;arClip[a];a++) {iSize+=strlen(arClip[a])+2;}
				pString=ehAllocZero(iSize+20);
				for (a=0;arClip[a];a++) {strcat(pString,arClip[a]); strcat(pString,CRLF);}
				strToClipboard("%s",pString);   
				ehFree(pString);
			}

			ARDestroy(arClip);
			break;

	}
	return pReturn;
}

void *SoapVarArg(EH_SOAP *psSoap,CHAR *Mess,...)
{	
	va_list Ah;
	CHAR *lpBuf;
	void *pReturn=NULL;

	va_start(Ah,Mess);

	lpBuf=ehAlloc(8000);
	vsprintf(lpBuf,Mess,Ah); 
	if (strstr(lpBuf,"=")) // Ci vorrebbe un parse fatto meglio (ci vorrebbe un parser!), ma per ora facciamo così
		pReturn=SoapVarSet(psSoap,lpBuf);
		else
		pReturn=SoapVariable(psSoap,WS_REALGET,lpBuf);	
	va_end(Ah);
	ehFree(lpBuf);
	if (psSoap->bVarNullLikeBlank&&!pReturn) return "";
	return pReturn;
}

void SoapOutputFile(EH_SOAP *psSoap,INT iCmd, BYTE *pFileName)
{
	switch (iCmd)
	{
		case WS_OPEN:
			if (strEmpty(pFileName)) pFileName="con:";
			psSoap->chOutput=fopen(pFileName,"wb"); if (!psSoap->chOutput) ehError();
			break;

		case WS_CLOSE:
			fclose(psSoap->chOutput);
			psSoap->chOutput=NULL;
			break;
	}
}



/*

ARRAY

----------------------------------------------------------------------------------------------
 MODO A - Implicito (Non predichiarato)
a) L'elemento Ships viene dichiarato come ArrayOfCustomShip (quindi complex)
b) ArrayOfCustomShip è una sequenza con maxOccurs>1 o unbounded, quindi è Array
Risultato: Ships.CustomShip[]
 
      <s:complexType name="Response_Ships">
        <s:sequence>
          <s:element minOccurs="1" maxOccurs="1" name="Range" nillable="true" type="s1:Range" />
          <s:element minOccurs="1" maxOccurs="1" name="Ships" nillable="true" type="s1:ArrayOfCustomShip" />  <--------------------
          <s:element minOccurs="1" maxOccurs="1" name="Error" nillable="true" type="s1:ErrorAnswer" />
        </s:sequence>
      </s:complexType>

      <s:complexType name="ArrayOfCustomShip">
        <s:sequence>
          <s:element minOccurs="0" maxOccurs="unbounded" name="CustomShip" nillable="true" type="s1:CustomShip" /> <--------------------
        </s:sequence>
      </s:complexType>


----------------------------------------------------------------------------------------------
 MODO B - Esplicito - Predichiarato

a) Il messaggio di ritorno viene dichiarato ArrayOfAccomodations con nome result
b) Viene dichiarato che conterrà un array di elementi di tipo Accomodations[] con attrib
c) Accomodations è un elemento complesso, quindi non è un array (differenza rispetto a prima)
Risutlato result.Accomodations[]


  <part    xmlns:partns="java:moby"
    type="partns:ArrayOfAccomodations"
    name="result">
  </part>


   <xsd:complexType     name="ArrayOfAccomodations">
    <xsd:complexContent>
     <xsd:restriction       xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/"
       base="soapenc:Array">
      <xsd:attribute        xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
        ref="soapenc:arrayType"
        wsdl:arrayType="stns:Accomodations[]"> <--------------------------
      </xsd:attribute>
     </xsd:restriction>
    </xsd:complexContent>
   </xsd:complexType>

   <xsd:complexType     name="Accomodations">
    <xsd:complexContent>
     <xsd:extension base="stns:Trip">
      <xsd:sequence>
       <xsd:element         xmlns:tp="java:language_builtins.lang"
         type="tp:ArrayOfString"
         name="accomodationCodes"
         minOccurs="1"
         nillable="true"
         maxOccurs="1">
       </xsd:element>
      </xsd:sequence>
     </xsd:extension>
    </xsd:complexContent>
   </xsd:complexType>

   <xsd:complexType     name="ArrayOfString">
    <xsd:complexContent>
     <xsd:restriction       xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/"
       base="soapenc:Array">
      <xsd:attribute        xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
        ref="soapenc:arrayType"
        wsdl:arrayType="xsd:string[]">
      </xsd:attribute>
     </xsd:restriction>
    </xsd:complexContent>
   </xsd:complexType>





*/


