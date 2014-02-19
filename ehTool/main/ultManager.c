// -------------------------------------------
//   UltManager
//   Universal Language Translator 
//   Windows 32bit version         
//                                           
//                          by Ferrà srl 2005 
// -------------------------------------------

#include "/easyhand/inc/easyhand.h"

//#include "c:/easyhand/ehtool/main\strEncode.h"
#include <time.h>

//#define ULT_VERSION "2.1"
#define ULT_VERSION "3.0"
#define MAX_TAG_SIZE 2048

//static EH_ULT _sUltDefault={0,0,0};
static struct {

	BOOL	fMakeDict;
	INT		iLine;	
	BOOL	bShowTime;
	BOOL	bAppMode;	// Non usa cache per traduzioni

//	EH_ULT	sApp;	
} _local={false,0,false};

#define STD_TAG "#["
#define WITANGO_TAG "#(W>"		// witango	: Witango encoding (Es ' diventa <@SQ>)
#define JAVA_TAG "#(J>"			// js		: Java encoding		(Es ' diventa \')
#define HTMLPURE_TAG "#(H>"		// html		:
#define UTF8_TAG "#(U>"			// utf		:
#define HTMLUTF8_TAG "#(HU>"	// hutf		:

typedef enum {
	
	ULT_TAG_STD,		// 1 STD_TAG		Se non diversamente indicato, converte tutti i caratteri tranne <>: per testi con HTML incluso
	ULT_TAG_WITANGO,	// 2 WITANGO_TAG
	ULT_TAG_HTMLP,	// 4 HTMLPURE_TAG	new 2007 HtmlPure converte anche <>& e ritorni a capo (Non per mette la scrittura di HTML)
	ULT_TAG_UTF8,		// 5 new 2007 : testi utf8 con tag html incluso
	ULT_TAG_HTMLUTF8, // 6 new 2007 : testi utf8 SENZA tag html incluso | per vedere i caratteri <> e gli altri in utf8
	ULT_TAG_JAVA,		// 3 
	
	ULT_TAG_END

} EN_ULT_TAGTYPE;

typedef enum {

	OUT_UNKNOW,
	OUT_TEXT,			// Nessuno encoding aggiuntivo		: se UTF8 diretto, altrimenti Latin1 solo superiore al 127
	OUT_WEB,			// Testo in una pagina web			: Per renderlo visibile tutto viene convertito in encoding HTML
	OUT_WEB_HTML,		// Testo in una pagina web + html	: Il testo può contenere tag html, quindi vengono preservati i tag e convertiti solo i caratteri superiori al ASCII
	OUT_XML,			// Testo in un XML					: Vengono convertiti i tag <> 
	OUT_JS,				// Stringa Javascript				: Il testo viene codificato usando \(qualcosa)
	OUT_WITANGO			// "Popola" una stringa Witango	
} EN_OUT_TARGET;

static CHAR *arEnc1[]={
	"#[%s]#",				// Default:		Non effettua encoding
	WITANGO_TAG "%s)#",
	JAVA_TAG"%s)#",
	HTMLPURE_TAG"%s)#",
	UTF8_TAG"%s)#",
	HTMLUTF8_TAG"%s)#",
	NULL};

//static void EncodeOrderPrepare(void);
static void		_ultTypeRecovery(EH_ULT * psUlt);
static INT		_ultEnumType(CHAR *lpType);
static void		_ultItemsLoad(EH_ULT * psUlt,CHAR *lpItem,BOOL bUserExtract);
static CHAR *	_ultFileEncodingAlloc(CHAR *lpWord,INT iType);
static CHAR *	_ultFileDecodingAlloc(CHAR *lpWord,INT iType);
static WCHAR *	_tokDecode(EH_ULT * psUlt,void *lpTok);

static BYTE *	_ultTextCharBuilder(EH_ULT * psUlt,WCHAR *lpPhrase,INT iLang);
static void		_ultLogOpen(EH_ULT * psUlt);
static void		_ultLogClose(EH_ULT * psUlt);
static void		_ultLogWrite(EH_ULT * psUlt,CHAR *Mess,...);
static BOOL		_ultWordBinFind(EH_ULT * psUlt,INT iType,WCHAR *pwcWordCode,INT *lpIndex);

static WCHAR *	_witangoEncoding(WCHAR *pcwWordTrans);
static WCHAR *	_witangoDecoding(WCHAR *pcwWordTrans);
static WCHAR *	_ltgtEncoding(WCHAR *pcwWordTrans);
static CHAR *	_tokExtract(EH_ULT * psUlt,CHAR ** lpCursor);

typedef struct {

	EN_STRENC		fileEncoding;
	EN_OUT_TARGET	enOutTarget;

} S_TAGFILEINFO;

static BOOL _getTagFileInfo(CHAR * pszFileMemo,S_TAGFILEINFO * psFileInfo,BOOL bReplace);

static INT _ultFileBuilder(EH_ULT * psUlt,
							CHAR *lpFileLong,
							CHAR *lpFileShort,
							CHAR *lpFolderDest,
							BOOL fRemoveFileReference,
							BOOL fShow);
static void _ultOrganizer(EH_ULT * psUlt);
static void _ultIdxFind(EH_ULT * psUlt);
static INT	_ultFileRemove(EH_ULT * psUlt,CHAR *lpFileName);
static EN_CHARTYPE _ultGetSourceEncode(EH_ULT *psUlt,CHAR * pszFileName);

void ULTDictionaryMake(void) {_local.fMakeDict=true;}
static void _ultItemsLoadOld(	EH_ULT * psUlt,
								INT iType,
								DWORD dwId, // Solo da files
								CHAR *lpWords,
								CHAR *lpFile,
								BOOL fLost,
								DWORD dwID);

// Vedi
// http://en.wikipedia.org/wiki/KOI8-R for Russian
// xliff specification http://www.oasis-open.org/committees/xliff/documents/xliff-specification.htm
//					   http://www.opentag.com/xliff.htm#XLIFFStructure
// - iso639  Codificazione a due lettere di una lingua) http://www.loc.gov/standards/iso639-2/php/code_list.php
// - rfc3066 Tags for the Identification of Languages	http://www.ietf.org/rfc/rfc3066.txt
// 
// ULT_LANGS di default
// Dovrebbe essere caricato da un file esterno, se presente
//
static EH_ULT_LANG _arLangPreset[]=
{
	{EH_LANG_ITALIAN,"IT","Italiano",	L"Italiano",	"Italian",	"it"},
	{EH_LANG_FRENCH ,"FR","Francese",	L"Français",	"French",	"fr"},
	{EH_LANG_ENGLISH,"EN","Inglese",	L"English",		"English",	"en"},
	{EH_LANG_DEUTCH ,"DE","Tedesco",	L"Deutsch",		"German",	"de"},
	{EH_LANG_SPAIN  ,"ES","Spagnolo",	L"Español",		"Spanish",	"es"},
	{10				,"PT","Portoghese",	L"Portuguese",	"Portuguese","pt"},
	{11				,"CZ","Ceco",		L"Czech",		"Czech",	"cs"},
	{12				,"PL","Polacco",	L"Polish",		"Polish",	"pl"},
	{13				,"NL","Olandese",	L"*NETHERLANDS","?",		"nl"},
	{14				,"SE","Svedese",	L"Swedish",		"Swedish",	"sv"},
	{15				,"DK","Danese",		L"Danish",		"Danish",	"da"},
	{16             ,"GR","Greco",		L"Greek",		"Greek",	"gr"},
	{17             ,"HU","Ungherese",	L"Hungarian",	"Hungarian","hu"},
	{18             ,"SK","Slovacco",	L"*Slovak",		"Slovak",	"sk"},
	{19             ,"RU","Russo",		L"Russian",		"Russian",	"ru"},
	{20             ,"JP","Giapponese",	L"Japanese",	"Japanese",	"ja"},
	{21             ,"TR","Turco",		L"Turk",		"Turk",		"tr-TR"},
	{22             ,"CN","Cinese",		L"China",		"China",	"zh-CN"},
	{0xFFFF			,"..","Files",		L"File coinvolti",""},
	{0x10000		,"##","Codice Sorgente",	L"Codice Sorgente",""},
	{0x10001		,"#N","Campo Note",L"Campo Note",""},
//	{0x10002		,"#C","Codice alfanumerico",L"Codice alfanumerico",""}, // Usato nei Virtual Ult
	{0,NULL,NULL,NULL}
};


//
// ULTNew() Creazione nuovo dizionario Base
//
INT ultNew(EH_ULT * psUlt,UTF8 * pszFileName)
{
	EH_ULT_LANG * lpLangInfo;

	ultClose(psUlt);
	psUlt->arLangGeneral=_arLangPreset;
	psUlt->dmiDict.Reset=-1;
	psUlt->dwLastId=0;
	psUlt->iCS_Source=ULT_CS_LATIN1;
	psUlt->iCS_Dictionary=ULT_CS_UTF8;
	psUlt->iEncodeHtml=ULT_CS_UTF8;
	psUlt->idxFiles=-1;
	psUlt->idxNote=-1;
	psUlt->iLangNative=ultTextToCode(psUlt,0,"IT");
	psUlt->iLangTransAlternative=ultTextToCode(psUlt,0,"EN");

	psUlt->arLangReady=ehAlloc(sizeof(EH_ULT_LANG)*ULT_MAXLANG); // Per ora il massimo

	lpLangInfo=ultLangInfo(psUlt,0,"##"); // Cerco Sorgente
	memcpy(psUlt->arLangReady,lpLangInfo,sizeof(EH_ULT_LANG));
	psUlt->iLangNum++;

	lpLangInfo=ultLangInfo(psUlt,0,"IT"); // Cerco IsoPrefix
	memcpy(psUlt->arLangReady+psUlt->iLangNum,lpLangInfo,sizeof(EH_ULT_LANG));
	psUlt->iLangNum++;

	lpLangInfo=ultLangInfo(psUlt,0,"EN"); // Cerco IsoPrefix
	memcpy(psUlt->arLangReady+psUlt->iLangNum,lpLangInfo,sizeof(EH_ULT_LANG));
	psUlt->iLangNum++;
/*
	lpLangInfo=ultLangInfo(psUlt,0,"FR"); // Cerco IsoPrefix
	memcpy(psUlt->arLangReady+psUlt->iLangNum,lpLangInfo,sizeof(EH_ULT_LANG));
	psUlt->iLangNum++;
*/
	lpLangInfo=ultLangInfo(psUlt,0,".."); // Cerco IsoPrefix File
	memcpy(psUlt->arLangReady+psUlt->iLangNum,lpLangInfo,sizeof(EH_ULT_LANG));
	psUlt->iLangNum++;

	psUlt->dwLastId=0x125;
	if (!strEmpty(pszFileName)) strcpy(psUlt->szFile,pszFileName);
	return ultSave(psUlt,TRUE);
}

//
// ultOpen()
//
BOOL ultOpen(EH_ULT * psUlt,UTF8 * lpFileName,INT iLanguage,BOOL fLoadAll) {

	return ultOpenEx(psUlt,false,lpFileName,iLanguage,fLoadAll,FALSE);

}


//
// _ultAutoClose()
//
static void _ultAutoClose(BOOL bStep) {

	if (!sys.ultApp.bReady) return;
	if (sys.ultApp.bModified) {
	
		ultSave(&sys.ultApp,false);

	}
	ultClose(&sys.ultApp);

}
//
// ultAppOpen()
//
BOOL ultAppOpen(UTF8 * pszFileName,CHAR * pszLang,CHAR * pszLangAlternative) {

	EH_ULT_LANG * psLang;
	if (!ultOpenEx(&sys.ultApp,false,pszFileName,0,true,false)) return false;

	// Setto la lingua usata nel dizionario
	ultSetLangText(&sys.ultApp,pszLang,pszLangAlternative);

	// Setto la lingua dell'applicazione
	psLang=ultLangInfo(&sys.ultApp,0,pszLang); if (!psLang) ehExit("Lingua %s sconosciuta",pszLang);
	strcpy(sys.szAppLanguage,psLang->lpTransName);

	_local.fMakeDict=true;
	_local.bAppMode=true;
	ehAddExit(_ultAutoClose);
	return true;
}

//
// ultResource()
//

BOOL ultResource(CHAR * pszLang,CHAR * pszLangAlternative) {

	HGLOBAL hMemory;
	CHAR * psz;
	HRSRC hRes;
	EH_ULT_LANG * psLang;
	hRes=FindResource(sys.EhWinInstance,"ULTDICT","ULT");  if (!hRes) {win_infoarg("ULT dictionary assente"); return true;}
	hMemory=LoadResource(sys.EhWinInstance,hRes);
	psz=LockResource(hMemory);
	
	if (!ultOpenEx(&sys.ultApp,true,psz,0,true,false)) return false;

	// Setto la lingua usata nel dizionario
	ultSetLangText(&sys.ultApp,pszLang,pszLangAlternative);

	// Setto la lingua dell'applicazione
	psLang=ultLangInfo(&sys.ultApp,0,pszLang); if (!psLang) ehExit("Lingua %s sconosciuta",pszLang);
	strcpy(sys.szAppLanguage,psLang->lpTransName);

	ehAddExit(_ultAutoClose);
	_local.bAppMode=true;
	return false;
}


CHAR * ultTag(CHAR * pszStr) {

	if (!sys.ultApp.bReady) return pszStr;
	return ultTranslate(ULT_TYPE_DISP,pszStr);

}

//
// ultOpenEx()
//
BOOL ultOpenEx(	EH_ULT *	psUlt,
				BOOL		bNotFile,		// T/F se i dati dell'ult sono imemoria FALSE=nome del file
				CHAR *		pszData,
				INT			iLanguage,
				BOOL		fLoadAll,
				BOOL		fOnlyHeader)
{
	FILE *	pfr;
	CHAR *	pszBuffer;
	INT SizeBuf=0xFFFFF;
	CHAR *p;
	BOOL fLoad=FALSE;
	EH_ULT_LANG sLangInfo,*lpLangInfo;

	memset(psUlt,0,sizeof(EH_ULT));
	psUlt->arLangGeneral=_arLangPreset;
	psUlt->dmiDict.Reset=-1;
	psUlt->dwLastId=0;
	psUlt->iCS_Source=ULT_CS_LATIN1;
	psUlt->iCS_Dictionary=ULT_CS_LATIN1;
	psUlt->iEncodeHtml=ULT_ENCODE_ISO;
	psUlt->iLangTransAlternative=-1;
	psUlt->idxLangTransAlternative=-1; // Lingua alternativa non indicata
	psUlt->idxFiles=-1;
	psUlt->idxNote=-1;
	psUlt->arLangReady=ehAlloc(sizeof(EH_ULT_LANG)*ULT_MAXLANG); // Per ora il massimo
	psUlt->lpVoiceShare=ehAlloc(sizeof(ULTVOICE));

	// EncodeOrderPrepare();

	// Se è una costruzione carico tutti i linguaggi
	//if (fMakeDict) 
	fLoadAll=true;
	psUlt->fLoadAll=fLoadAll;
	psUlt->iLangWant=iLanguage;

	if (!bNotFile) {

		if (strstr(pszData,".")==NULL) sprintf(psUlt->szFile,"%s.ult",pszData); else strcpy(psUlt->szFile,pszData);
		if (!fileCheck(psUlt->szFile)) 
		{
	#ifdef EH_CONSOLE
			 win_infoarg("ULT: %s ?",psUlt->szFile); return FALSE;
	#else
			if (_local.fMakeDict) 
			 ultNew(psUlt,NULL);
			 else
			 win_infoarg("ULTDictionaryLoad(): %s ?",psUlt->szFile); return FALSE;
	#endif
		}

		pfr=fopen(psUlt->szFile,"r"); if (!pfr) ehExit("Ult1");
		strcpy(psUlt->szUltFolder,filePath(psUlt->szFile));	
		strcat(psUlt->szUltFolder,"ULT\\");
	}
	else {
		pfr=NULL;
		*psUlt->szUltFolder=0;
		*psUlt->szUltFolder=0;
	}
	pszBuffer=ehAlloc(SizeBuf);

	while (true)
	{
		if (pfr) {

			if (!fgets(pszBuffer,SizeBuf-1,pfr)) break;

		} else {
	
			INT iLen;
			p=strstr(pszData,"\r\n");
			if (!p) p=strstr(pszData,"\r");
			if (!p) p=strstr(pszData,"\n");
			if (!p) break;

			iLen=(DWORD) p-(DWORD) pszData;
			if (iLen>SizeBuf-1) iLen=SizeBuf-1;
			memcpy(pszBuffer,pszData,iLen); pszBuffer[iLen]=0;
			pszData=p+1;


		}

		_local.iLine++;

		//strcpy(Buffer,strOmit(Buffer,"\n\r"));
		p=strstr(pszBuffer,"\n"); if (p) *p=0;
		p=strstr(pszBuffer,"\r"); if (p) *p=0;

		// ----------------------------------------------------
		// Metacomandi
		// ----------------------------------------------------
		if (*pszBuffer=='@')
		{
			CHAR * pszParam;
			// Inizio definizione dizionario
			if (!strcmp(pszBuffer,"@dictionarystart")) {fLoad=TRUE; continue;}

			// Fine definizione del dizionario
			if (!strcmp(pszBuffer,"@dictionaryend")) {fLoad=FALSE; continue;}

			p=strstr(pszBuffer," "); if (!p) continue;
			*p=0; p++;
			pszParam=pszBuffer+1;

			// Encoding
			if (!strcmp(pszParam,"charset_source")) {psUlt->iCS_Source=ultTextToCode(psUlt,1,p); continue;}
			if (!strcmp(pszParam,"charset_dict")) {psUlt->iCS_Dictionary=ultTextToCode(psUlt,1,p); continue;}
			if (!strcmp(pszParam,"encoding_html")) {psUlt->iEncodeHtml=ultTextToCode(psUlt,2,p); continue;}
			if (!strcmp(pszParam,"encoding_files_source")) 
			{
				strTrim(p);
				if (!strEmpty(p)) {
					psUlt->pszEncodingFilesSource=strDup(p); 
					psUlt->arEFS=ARFSplit(psUlt->pszEncodingFilesSource,"|");
					// ehLogWrite("[%s]",psUlt->pszEncodingFilesSource);
				}
				continue;
			}
			

			// Alloco in memoria l'array con le lingue presenti arLangReady
			if (!strcmp(pszParam,"dictionaries"))
			{
				CHAR *p2;
				BOOL fCodeLang=FALSE;
				INT iLen;

				p=strOmit(p,CRLF);
				iLen=sizeof(sLangInfo);
				//ARMaker(WS_OPEN,&iLen);
				psUlt->iLangReady=0;
				if (psUlt->arLangReady) ehFree(psUlt->arLangReady);
				psUlt->arLangReady=ehAlloc(sizeof(EH_ULT_LANG)*ULT_MAXLANG); // Per ora il massimo
				while (TRUE)
				{
					p2=strstr(p,","); if (p2) *p2=0;
					lpLangInfo=ultLangInfo(psUlt,0,p); // Cerco IsoPrefix
					if (lpLangInfo)
					{
						if (lpLangInfo->idLang==0x10000) fCodeLang=TRUE; // Ho la lingua codice
						memcpy(psUlt->arLangReady+psUlt->iLangNum,lpLangInfo,sizeof(EH_ULT_LANG));
						if (lpLangInfo->idLang<0xFFFF) psUlt->iLangReady++; // è una lingua
					}
					else
					{
						ZeroFill(sLangInfo);
						sLangInfo.idLang=0x10002; // Non identificata
						sLangInfo.lpIsoPrefix=strDup(p); // Va bè fa niente se non libero memoria
						sLangInfo.lpLangName="?";
						sLangInfo.pwcLangNameNativo=L"?";
						//ARMaker(WS_ADD,&sLangInfo); // Aggiungo nella posizione dell'array la lingua
						memcpy(psUlt->arLangReady+psUlt->iLangNum,&sLangInfo,sizeof(EH_ULT_LANG));
					}

					psUlt->iLangNum++;
					if (p2) p=p2+1; else break;
				}

				// 
				// Compatibilità con il passato
				// Deve esistere sia la lingua Codice ## sia la lingua Nativa collegata al codice
				// Quindi se non esiste la lingua Codice si assume che la lingua codice è la 0 (si sostituisce nell'array
				// e si aggiunge la lingua "Nativa" Italiana come ultima
				//

				if (!fCodeLang)
				{
					lpLangInfo=ultLangInfo(psUlt,0,"##"); // Cerco IsoPrefix
					memcpy(psUlt->arLangReady,lpLangInfo,sizeof(EH_ULT_LANG));

					lpLangInfo=ultLangInfo(psUlt,0,"IT"); // Cerco IsoPrefix
					memcpy(psUlt->arLangReady+psUlt->iLangNum,lpLangInfo,sizeof(EH_ULT_LANG));
					psUlt->iLangNative=lpLangInfo->idLang;
					psUlt->iLangNum++;
				}
				/*
				win_infoarg("Lingue [%d]",psUlt->iLangNum);
				for (a=0;a<psUlt->iLangNum;a++)
				{
				win_infoarg("%d) Lingue [%d:%s]",a,psUlt->arLangReady[a].idLang,psUlt->arLangReady[a].lpIsoPrefix);
				}
				*/
				continue;
			}

			// Codice della lingua codice
			if (!strcmp(pszParam,"sourcelang")) {psUlt->iLangNative=ultTextToCode(psUlt,0,p); continue;}
			if (!strcmp(pszParam,"lang_native")) {psUlt->iLangNative=ultTextToCode(psUlt,0,p); continue;}
			if (!strcmp(pszParam,"trans_alt")) {psUlt->iLangTransAlternative=ultTextToCode(psUlt,0,p); continue;}
			if (!strcmp(pszParam,"realcode")) {psUlt->fRealCode=atoi(p); continue;}

			// --------------------------------------------------------
			// Parole presenti nel dizionario
			// --------------------------------------------------------
			if (!strcmp(pszParam,"dictionaryword"))
			{
				INT num=atoi(p);
				DMIOpen(&psUlt->dmiDict,RAM_AUTO,num,sizeof(ULTVOICE),"*UltInfo");
				continue;
			}

			// webfolder e webautoscan
			if (!strcmp(pszBuffer,"@webfolder")) {psUlt->fWebFolder=atoi(p); continue;}
			if (!strcmp(pszBuffer,"@webautoscan")) {psUlt->bWebAutoScan=atoi(p); continue;}
			if (!strcmp(pszBuffer,"@dopurge")) {
				psUlt->pszDoPurge=strDup(p); continue;
			}

			if (!strCaseCmp(pszBuffer,"@version")) 
			{	
				psUlt->iVersion=(INT) (atof(p)*100); 
				if (psUlt->iVersion>200) psUlt->iTypeEncDict=1;
			}
			if (!strCaseCmp(pszBuffer,"@lastid")) {psUlt->dwLastId=atoi(p); continue;}
		}

		// Carico le parole nel dizionario
		if (fLoad&&(*pszBuffer=='#')&&!fOnlyHeader)
		{
			CHAR *p2,*p3;
			CHAR *lpItem;
			BOOL fLost;
			DWORD dwID;
			INT iType=-1;

			if (psUlt->iVersion<210)
			{
				lpItem=strstr(pszBuffer,"@["); if (!lpItem) ehExit("ULT: File is corrupt [%s]",psUlt->szFile);
			}

			dwID=0;
			switch (psUlt->iVersion)
			{
			case 150: // Lost
				p3=pszBuffer;
				p2=strstr(p3,","); if (p2) *p2=0;
				iType=_ultEnumType(p3+1);

				*p2=','; p3=p2+1;
				p2=strstr(p3,","); if (p2) *p2=0;
				fLost=atoi(p3);

				*p2=','; p3=p2+1;
				_ultItemsLoadOld(psUlt,iType,dwID,lpItem,NULL,fLost,dwID);
				break;

			case 160: // dwID
			case 200:
				p3=pszBuffer;
				p2=strstr(p3,","); if (p2) *p2=0;
				iType=_ultEnumType(p3+1);

				*p2=','; p3=p2+1;
				p2=strstr(p3,","); if (p2) *p2=0;
				fLost=atoi(p3);

				*p2=','; p3=p2+1;
				p2=strstr(p3,","); if (p2) *p2=0;
				dwID=xtoi(p3);
				_ultItemsLoadOld(psUlt,iType,dwID,lpItem,NULL,fLost,dwID);
				break;

			case 210:
				_ultItemsLoad(psUlt,pszBuffer+1,FALSE);
				break;

			case 300:
			case 310:
				_ultItemsLoad(psUlt,pszBuffer+1,TRUE);
				break;


			case 120:
			default:
				p2=strstr(pszBuffer,","); if (p2) *p2=0;
				iType=_ultEnumType(pszBuffer+1);
				fLost=false;
				_ultItemsLoadOld(psUlt,iType,dwID,lpItem,NULL,fLost,dwID);
				break;
			}

		}
	}

	if (pfr) fclose(pfr);
	ehFree(pszBuffer);

	_ultIdxFind(psUlt);
	_ultOrganizer(psUlt); // Metto il file per ultimo

	// Trova la posizione della lingua nella asse X dell'array
	if (psUlt->fLoadAll)
	{
		psUlt->idxLangTranslated=ultLangReady(psUlt,iLanguage,NULL);
		// Premesso:
		// Che 0 è il codice (che terrei fisso) comunque puntato da idxLangCode
		// L'ultimo in fondo dovrebbero essere i file coinvolti
		// Ora serve un utility che mi permette di spostare agilmente una lingua lungo l'asse X
		// anche per poter aggiungere nuovo lingue all'array 
		// FATTO SOPRA _ultOrganizer()
	} 
	else 
	{
		psUlt->idxLangTranslated=1; // Selettore sul traduttore
	}
	psUlt->bReady=true;
	return TRUE;
}

//
// ultSave()
//

BOOL ultSave(EH_ULT * psUlt,BOOL fShowError) {

	return ultSaveAs(psUlt,NULL,fShowError);
}

//
// ultSaveAs()
//
BOOL ultSaveAs(	EH_ULT * psUlt,
				CHAR * pszFileName,
				BOOL fShowError) {

	FILE *	pfr;
	CHAR *	Buffer;
	CHAR *	Buffer2;
	CHAR *	lpType;
	INT	SizeBuf=MAX_TAG_SIZE;
	INT	a,b,c;
	ULTVOICE sUltVoice;
	CHAR *	lpFile;
	CHAR *	lpe;

	CHAR *lpHeader=
		"@Universal Language Translator\n"\
		"@version " ULT_VERSION "\n"\
		"@company_name Ferrà srl\n" 
		"@legal_copyright Copyright © 2003-@#ANNO#@ (c)\n" 
		"@charset_source @#CS_SOURCE#@\n"  
		"@charset_dict @#CS_DICT#@\n"
		"@encoding_html @#ENCODE_HTML#@\n"
		"@encoding_files_source @#ENCODE_FILE_SOURCE#@\n"
		"@dictionaries @#DICTS#@\n"
		"@lang_native @#LANGN#@\n"
		"@trans_alt @#TRANSALT#@\n"
//		"@dopurge @#DOPURGE#@\n"
		"@realcode @#REALCODE#@\n";

	// if (lpFileName) lpFile=lpFileName; else 
	if (!strEmpty(pszFileName)) lpFile=pszFileName; else lpFile=psUlt->szFile;

	//	  printf("Qui;[%s] . %d\n",lpFile,psUlt->dmiDict.Num);
	// Forso la scrittura ad ULT_CS_UTF8
	if (psUlt->iCS_Dictionary==ULT_CS_LATIN1) psUlt->iCS_Dictionary=ULT_CS_UTF8;

	// Preparo l'intestazione del file
	Buffer=ehAlloc(SizeBuf); Buffer2=ehAlloc(40000);

	strcpy(Buffer2,lpHeader);
	strReplace(Buffer2,"@#ANNO#@",dateToday()+4);
	strReplace(Buffer2,"@#LANGN#@",ultCodeToText(psUlt,0,psUlt->iLangNative));
	strReplace(Buffer2,"@#TRANSALT#@",ultCodeToText(psUlt,0,psUlt->iLangTransAlternative));

	*Buffer=0;
	for (a=0;a<psUlt->iLangNum;a++)
	{
		if (*Buffer) strcat(Buffer,",");
		strcat(Buffer,psUlt->arLangReady[a].lpIsoPrefix);
	}
	strReplace(Buffer2,"@#DICTS#@",Buffer);

	strReplace(Buffer2,"@#CS_SOURCE#@",ultCodeToText(psUlt,1,psUlt->iCS_Source));
	strReplace(Buffer2,"@#CS_DICT#@",ultCodeToText(psUlt,1,psUlt->iCS_Dictionary));
	strReplace(Buffer2,"@#ENCODE_HTML#@",ultCodeToText(psUlt,2,psUlt->iEncodeHtml));
	strReplace(Buffer2,"@#ENCODE_FILE_SOURCE#@",strEver(psUlt->pszEncodingFilesSource));
	strReplace(Buffer2,"@#REALCODE#@",psUlt->fRealCode?"1":"0");
	

	_ultTypeRecovery(psUlt);

	pfr=fopen(lpFile,"wb");
	if (!pfr) 
	{	
		INT iErr=osGetError();
		if (!fShowError) 
		{
			ehFree(Buffer2);
			return iErr;
		}
		if (iErr==5) ehExit("%s accesso non autorizzato",lpFile);
		ehExit("ULTWrite:%s [%d]",lpFile,osGetError());
	}
	fprintf(pfr,Buffer2);
	ehFree(Buffer2);

	fprintf(pfr,"@lastid %d\n",psUlt->dwLastId);
	fprintf(pfr,"@webfolder %d\n",psUlt->fWebFolder);
	fprintf(pfr,"@webautoscan %d\n",psUlt->bWebAutoScan);
	fprintf(pfr,"@dictionaryword %d\n",psUlt->dmiDict.Num);
	if (!strEmpty(psUlt->pszDoPurge)) fprintf(pfr,"@dopurge %s\n",psUlt->pszDoPurge);
	fprintf(pfr,"@dictionarystart\n");

	/*
	// Loop sui tipi
	// Precontrollo - da togliere
	for (a=0;a<ULT_MAXTYPE;a++)
	{
	if (!psUlt->iTypeNumber[a]) continue;
	win_infoarg("> %d,%d,%d",a,psUlt->iTypePoint[a],psUlt->iTypePoint[a]+psUlt->iTypeNumber[a]);
	}
	*/

	// Loop sui tipi
	for (a=0;a<ULT_MAXTYPE;a++)
	{
		if (!psUlt->arType[a].iNum) continue;
		lpType=ultTypeToText(a);

		// Loop sugli elementi
		for (b=psUlt->arType[a].iFirst;b<psUlt->arType[a].iFirst+psUlt->arType[a].iNum;b++)
		{
			DMIRead(&psUlt->dmiDict,b,&sUltVoice);
			// Scrittura degli elementi
			fprintf(pfr,"#%s\3%05x\3\3",lpType,sUltVoice.dwID);
			for (c=0;c<psUlt->iLangNum;c++)
			{
				DWORD dwUser=sUltVoice.dwUser[c];
				if (sUltVoice.lpwText[c]==NULL) 
					fprintf(pfr,"\3\4%d",dwUser);
				else
				{
					CHAR *lpWord;


					// A) Trasformo il Wide Char in una stringa
					switch (psUlt->iCS_Dictionary)
					{
						case ULT_CS_LATIN1: 
							lpe=strEncodeW(sUltVoice.lpwText[c],SE_WTC,NULL);
							lpWord=_ultFileEncodingAlloc(lpe,1);
							ehFree(lpe);
							break;

						case ULT_CS_UTF8: 
							lpe=strEncodeW(sUltVoice.lpwText[c],SE_UTF8,NULL);
							lpWord=_ultFileEncodingAlloc(lpe,1);
							ehFree(lpe);
							break;
					}

					if (lpWord) 
					{
						fprintf(pfr,"\3%s\4%d",lpWord,dwUser);
						ehFree(lpWord);
					}
					else
					{
						fprintf(pfr,"\3\4%d",dwUser);
					}
				}
			}
			fprintf(pfr,"\n");
		}
	}

	fprintf(pfr,"@dictionaryend\n");
	ehFree(Buffer);
	fclose(pfr);
	return 0;
}


//
// ultClose()
//
void ultClose(EH_ULT * psUlt)
{
	ULTVOICE sUltVoice;
	INT x,c;

//	if (!psUlt->iTextSize) return;
 	if (psUlt->dmiDict.Hdl>0)
	{
		for (x=0;x<psUlt->dmiDict.Num;x++)
		{
			DMIRead(&psUlt->dmiDict,x,&sUltVoice);
			if (sUltVoice.lpbText) ehFree(sUltVoice.lpbText);
			for (c=0;c<psUlt->iLangNum;c++)
			{
//				if (sUltVoice.lpcText[c]) ehFree(sUltVoice.lpcText[c]);
				if (sUltVoice.lpwText[c]) ehFree(sUltVoice.lpwText[c]);
			}
		}
	   DMIClose(&psUlt->dmiDict,"*UltInfo");
	}
	
	ehFreePtrs(5,&psUlt->arLangReady,&psUlt->lpVoiceShare,&psUlt->pszEncodingFilesSource,&psUlt->arEFS,&psUlt->pszDoPurge);
	memset(psUlt,0,sizeof(EH_ULT));
}

//
// ultCloseInternal()
//
void ultCloseInternal(EH_ULT * psUlt)
{
//	if (!psUlt->iTextSize) return;

	if (_local.fMakeDict)
	{
		ultSave(psUlt,true);
//		ShellExecute(NULL,"open","write.exe",UltFile,"",SW_NORMAL);
	}
	
	ultClose(psUlt);
	
}

//
// ultSetLang()
//
void ultSetLangIdx(EH_ULT * psUlt,INT idxLang,INT idxLangMiss) {

	psUlt->idxLangTranslated=idxLang;
	if (psUlt->idxLangTranslated<0) ehExit("ultSetLang(A)");
	if (idxLangMiss>-1) 
	{
		psUlt->idxLangTransAlternative=idxLangMiss;
		psUlt->iLangTransAlternative=psUlt->arLangReady[psUlt->idxLangTransAlternative].idLang;
	}

}

//
// ultSetLang()
//
void ultSetLang(EH_ULT * psUlt,EN_LANGUAGE enLanguage,EN_LANGUAGE enLangAlternative) {

	INT idxLang=ultLangReady(psUlt,enLanguage,NULL);
	INT idxLangAlternative=-1;
	if (enLangAlternative>-1) idxLangAlternative=ultLangReady(psUlt,enLangAlternative,NULL);
	ultSetLangIdx(psUlt,idxLang,idxLangAlternative);

}

//
// ultSetLang()
//
void ultSetLangText(EH_ULT * psUlt,CHAR * pszLang,CHAR * pszLangAlternative)
{

	INT idxLang=ultLangReady(psUlt,0,pszLang);
	INT idxLangAlternative=-1;
	if (!strEmpty(pszLangAlternative)) idxLangAlternative=ultLangReady(psUlt,0,pszLangAlternative);
	ultSetLangIdx(psUlt,idxLang,idxLangAlternative);

}


//
// ultItemSetFlag()
// 
void ultItemSetFlag(EH_ULT * psUlt)
{
  INT a;
  INT iLangTra=0;
  psUlt->lpVoiceShare->iTransStatus=0;
  
  // Controllo se esiste traduzione
  for (a=0;a<psUlt->iLangNum;a++) // da 1 perchè salto la prima lingua
  {
	if (psUlt->lpVoiceShare->lpwText[a]&&
		a!=psUlt->idxFiles&&
		a!=psUlt->idxLangNative&&
		a!=psUlt->idxNote)//&&
		//a!=psUlt->idxAlfaCode)
		{
			iLangTra++;
		}
  }

  // iLangTra:iLangNum-1=x:100
  if (iLangTra) psUlt->lpVoiceShare->iTransStatus=100*iLangTra/psUlt->iLangReady;//(psUlt->iLangNum-2);
	  
  // Controllo se esiste il file collegato
  if (psUlt->idxFiles>-1)
  {
	 if (wcsEmpty(psUlt->lpVoiceShare->lpwText[psUlt->idxFiles])) psUlt->lpVoiceShare->fLost=true; else psUlt->lpVoiceShare->fLost=false;

//	 if (psUlt->lpVoiceShare->fLost) 
//		 printf("> %S" CRLF,psUlt->lpVoiceShare->lpwText[0]);
  }
}

//
// ultLangInfo()
//
EH_ULT_LANG * ultLangInfo(EH_ULT * psUlt,INT idLang,CHAR *lpIsoFind) 
{

	INT a;
	for (a=0;psUlt->arLangGeneral[a].lpIsoPrefix;a++)
	{
		if (lpIsoFind)
		{
			if (!strcmp(psUlt->arLangGeneral[a].lpIsoPrefix,lpIsoFind)) return &psUlt->arLangGeneral[a];
		}
		else
		{
			if (psUlt->arLangGeneral[a].idLang==idLang) return &psUlt->arLangGeneral[a];
		}
	}
	return NULL;

}
//
// ultLangReady()
//
INT ultLangReady(EH_ULT * psUlt,INT iCode,CHAR * lpIsoPrefix)
{
	INT a;
	for (a=0;a<psUlt->iLangNum;a++)
	{
		if (lpIsoPrefix)
		{
			if (!strcmp(psUlt->arLangReady[a].lpIsoPrefix,lpIsoPrefix)) return a;	
		}
		else
		{
			if (psUlt->arLangReady[a].idLang==iCode) return a;	
		}
	}
	return -1;
}




//
// ultAddWord()
//
void ultAddWord(EH_ULT * psUlt,void *lpElement,INT idxLang,INT iCharSize,DWORD dwUser) // 1=CHAR 8 Bit originale, 2=Wide Char
{
	if (idxLang<0||idxLang>=psUlt->iLangNum) ehExit("ultAddWord().idLang(%d)>%d",idxLang,psUlt->iLangNum);
	psUlt->lpVoiceShare->dwUser[idxLang]=dwUser;
	if (lpElement==NULL) // Nessuna parola collegata
	{
	 psUlt->lpVoiceShare->lpwText[idxLang]=NULL; return;
	}
	if ((iCharSize==1&&!* (BYTE *) lpElement)||
	    (iCharSize==2&&!* (WCHAR *) lpElement))
	{
	 psUlt->lpVoiceShare->lpwText[idxLang]=NULL;
	 return;
	}

//	iLen=wcslen(lpWord)+1;
	switch (iCharSize)
	{
		case 1: // 1 byte x char
//			psUlt->lpVoiceShare->lpcText[idxLang]=strDup(lpElement);
			psUlt->lpVoiceShare->lpwText[idxLang]=strToWcs(lpElement);
			break;
		
		case 2: // 2 byte x char
			psUlt->lpVoiceShare->lpwText[idxLang]=wcsDup(lpElement);
			break;

		default: ehExit("ultAddWord(): CharSet error ");
	}
}


//
// ultWordFileControl()
//
// Aggiunge un file che usa la parola corrente
//

void ultWordFileControl(EH_ULT * psUlt,INT iIndex,CHAR *lpFile) {

	WCHAR *pwc,*pwcFile;
	ULTVOICE sUltVoice;

	if (psUlt->idxFiles<0) ehError();
	DMIRead(&psUlt->dmiDict,iIndex,&sUltVoice);
	pwc=sUltVoice.lpwText[psUlt->idxFiles];
	pwcFile=strToWcs(lpFile);

	if (!pwc) {

		ultWordUpdate(psUlt,iIndex,psUlt->idxFiles,pwcFile,2,UU_SOURCE);
		
//		DMIRead(&psUlt->dmiDict,iIndex,&sUltVoice);
//		pwc=sUltVoice.lpwText[psUlt->idxFiles];
//		printf("qui");
	}
	else
	{
		WCHAR *lpMemo;
		if (!wcsstr(pwc,pwcFile))
		{
			lpMemo=ehAlloc(MAX_TAG_SIZE);
			wcscpy(lpMemo,pwc);
			wcscat(lpMemo,L",");
			wcscat(lpMemo,pwcFile);
			ultWordUpdate(psUlt,iIndex,psUlt->idxFiles,lpMemo,2,UU_SOURCE);
			ehFree(lpMemo);
		}
	}
	ehFree(pwcFile);
}

// 
// ultTranslateEx()
// Cerca la traduzione
// se in modalità di costruzione aggiunge l'item
// 
void * ultTranslateEx(	EH_ULT * psUlt,
						EN_ULTYPE enType,	
						WCHAR * pwcWordCode, 
						CHAR * lpFileSource,
						BOOL fBuildOn,
						INT * lpRow) 
{
	INT iRow;

	if (wcsEmpty(pwcWordCode)) return pwcWordCode;

	// L'ho trovato
	if (_ultWordBinFind(psUlt,enType,pwcWordCode,&iRow))
	{
		WCHAR *pwc;

		// 
		// Se sono in costruzione controllo il file
		//
		if (fBuildOn&&lpFileSource) ultWordFileControl(psUlt,iRow,lpFileSource);

		//
		// Prima scelta di traduzione, la lingua che mi interessa
		//
		pwc=psUlt->lpVoiceShare->lpwText[psUlt->idxLangTranslated];

		//
		// Se non sono devo memorizzare e non trovo nulla
		//
		if (!fBuildOn&!pwc) {

			//
			// CASO 1:  Sono sulla lingua Nativa ed è vuoto (?), uso cerco la Lingua Codice
			//
			if (psUlt->idxLangTranslated==psUlt->idxLangNative) 
				pwc=psUlt->lpVoiceShare->lpwText[psUlt->idxLangCode];
			else
			//
			// CASO 2: Provo con la lingua alternativa (Esempio inglese)
			//
			//if (!pwc)
			{
				pwc=psUlt->lpVoiceShare->lpwText[psUlt->idxLangTransAlternative]; 
			}

		}

		//
		// CASO 3 : Senza speranza, Se non ho nemmeno la lingua alternativa di default, vado sulla lingua codice
		//
		if (!pwc)
		{
			pwc=psUlt->lpVoiceShare->lpwText[psUlt->idxLangNative];//idxLangCode];
			if (!pwc) pwc=psUlt->lpVoiceShare->lpwText[psUlt->idxLangCode]; // in extremis
		}

		// Ho la traduzione vado al decoding
		//	  if (pwc) 
		//	  {
		//	  }
		if (lpRow) *lpRow=iRow;
//		if (fBuildOn) DMIWrite(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);
		return pwc;

		//	  return psUlt->lpVoiceShare->lpwText[psUlt->idxLangCode]; // Ritorno lingua base
	}

	//
	// Non l'ho trovato e sono in creazione lo inserisco nella tabella
	//
	if (fBuildOn)
	{
		INT Index,a;
		psUlt->iNewItem++;
		// Aggiungo la nuova parola
		Index=ultItemInsert(psUlt,enType,pwcWordCode,0,NULL);

		// _d_("ADD: %d - %s        ",Index,lpWord);
		// Aggiungo NULL in tutte le lingue
		// da 1 perchè salto la prima lingua
		for (a=1;a<psUlt->iLangNum;a++) {ultAddWord(psUlt,NULL,a,1,UU_UNKNOW);}

		DMIWrite(&psUlt->dmiDict,Index,psUlt->lpVoiceShare);
		if (lpFileSource) ultWordFileControl(psUlt,Index,lpFileSource);
		DMIWrite(&psUlt->dmiDict,Index,psUlt->lpVoiceShare);
		psUlt->bModified=true;
	}

	if (lpRow) *lpRow=-1;
	return pwcWordCode;
}

//
// ultTranslateSZZAlloc
//
CHAR * ultTranslateSZZAlloc(CHAR *lpStringZZ)
{
	CHAR *lpRet=ehAlloc(4096);
	CHAR *lpDest=lpRet;

	while (TRUE)
	{
		if (!*lpStringZZ) break;
		strcpy(lpDest,ultTag(lpStringZZ));
		lpDest+=strlen(lpDest)+1; 
		lpStringZZ+=strlen(lpStringZZ)+1;
	}
	return lpRet;
}

// -------------------------------------------------
// Traduce la parola trovata
// -------------------------------------------------

// 
// Vecchio sistema tenuto per compatibilità con i programmi EasyHand in modalità CHAR
// Crea una memoria CHAR da ritorno del puntatore WCHAR
//
//
// ultTranslate
//
BYTE * ultTranslate(INT iType,BYTE *lpWord)
{
	WCHAR *pwcRet,*pwcSend;
	INT iRow=-1;
	EH_ULT * psUlt=&sys.ultApp;

	pwcSend=strToWcs(lpWord);
	pwcRet=ultTranslateEx(psUlt,iType,pwcSend,"app",_local.fMakeDict,&iRow);
	ehFree(pwcSend);

	// Non presente nel dizionario
	if (iRow<0) return lpWord;

	if (pwcRet) {

		DMIRead(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);
		if (!_local.bAppMode&&psUlt->lpVoiceShare->lpbText) return psUlt->lpVoiceShare->lpbText; // Traduzione gia eseguita

		// Creo traduzione
//		psUlt->lpVoiceShare->lpbText=ehAlloc(wcslen(pwcRet)+1);
//		UnicodeToChar(psUlt->lpVoiceShare->lpbText,pwcRet);
		ehFreeNN(psUlt->lpVoiceShare->lpbText);
		if (sys.bUtf8)
			psUlt->lpVoiceShare->lpbText=strEnc(2,pwcRet,SE_UTF8,NULL);
			else
			psUlt->lpVoiceShare->lpbText=wcsToStr(pwcRet);
		DMIWrite(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);

	}
	else {

		ehExit("NULL");	

	}
	return psUlt->lpVoiceShare->lpbText;
}


// -------------------------------------------------
// ultItemInsert()
// Aggiunge una voce al Dizionario
// Ritorno l'indice al dizionario
//
// -------------------------------------------------
INT ultItemInsert(EH_ULT * psUlt,
				   INT iType,
				   WCHAR *pwcWord,
				   DWORD dwID,
				   BOOL *lpfDup) {

	INT iRow;
	BOOL fFind;

	if (lpfDup) *lpfDup=FALSE;
	if (iType>=ULT_MAXTYPE) ehExit("IType 1");

	// Cerco prima se c'è
	fFind=_ultWordBinFind(psUlt,iType,pwcWord,&iRow); 
	if (fFind) 
	{
		if (lpfDup) *lpfDup=TRUE;
		return iRow; 
	}

	// Creo l'idem
	memset(psUlt->lpVoiceShare,0,sizeof(ULTVOICE));
	psUlt->lpVoiceShare->iType=iType;
	if (!dwID) 
		dwID=++psUlt->dwLastId; 
	else 
	{
		if (dwID>psUlt->dwLastId) psUlt->dwLastId=dwID;
	}
	psUlt->lpVoiceShare->dwID=dwID; // Bisognerebbe controllare se non esiste

	//if (strlen(lpWord)>1000) win_infoarg(lpWord);
	if (iRow==psUlt->dmiDict.Num) 
	{
		// E' la prima volta del tipo indicato; Faccio puntare il puntatore al primo
		if (!psUlt->arType[iType].iNum) psUlt->arType[iType].iFirst=iRow;

		// Aggiungo la parola sulla prima lingua
		ultAddWord(psUlt,pwcWord,0,2,UU_SOURCE); 

		// Agiungo la voce al dizionario
		//DMIAppend(&psUlt->dmiDict,psUlt->lpVoiceShare);
		DMIAppendDyn(&psUlt->dmiDict,psUlt->lpVoiceShare);
	}
	else	
	{   
		INT a;
		// Inserisco la voce nell'indice trovatp
		DMIInsertDyn(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);

		// Aggiungo la parola sulla prima lingua
		ultAddWord(psUlt,pwcWord,0,2,UU_SOURCE);

		// Scrivo le modifiche dopo addword
		DMIWrite(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);

		// Sposto i avanti i puntatori delle altre zone
		for (a=0;a<ULT_MAXTYPE;a++)
		{
			// Se sono il tipo corrente non faccio nulla
			if (a==iType) continue; 
			// Se trovo un tipo che punto dopo l'index attuale incremento la posizione
			if (psUlt->arType[a].iNum&&(psUlt->arType[a].iFirst>=iRow)) psUlt->arType[a].iFirst++;
		}
	}

	psUlt->arType[iType].iNum++; // Aggiungo di uno al tipo
	return iRow;
}


// --------------------------------------------------
// ultItemDelete() 
// Cancello l'intero Item
// --------------------------------------------------
void ultItemDelete(EH_ULT * psUlt,INT Index,DWORD dwUser)
{
	INT a;
	INT iType;
	DMIRead(&psUlt->dmiDict,Index,psUlt->lpVoiceShare);
	iType=psUlt->lpVoiceShare->iType;

	// da 1 perchè salto la prima lingua
	for (a=1;a<psUlt->iLangNum;a++) {
		ultWordDelete(psUlt,Index,a,dwUser);
	}
	ultWordDelete(psUlt,Index,0,dwUser);
	DMIDelete(&psUlt->dmiDict,Index,psUlt->lpVoiceShare);
	_ultTypeRecovery(psUlt);
}


// --------------------------------------------------
// ultWordDelete() 
// Cancello la parola
// --------------------------------------------------
void ultWordDelete(EH_ULT * psUlt,INT Index,INT iLang,DWORD dwUser) {

	ultWordUpdate(psUlt,Index,iLang,NULL,0,dwUser);

}

// --------------------------------------------------
// ultWordUpdate() Modifica una frase in una lingua nel dizionario
// 
// Aggiunge la traduzione in tutte le lingue
// NOTE: L'area testo è inserita sul fondo con le parole inserite in sequenza
// a caso come un campo blob.
// lpWord=NULL
//
//	Ritorna FALSE - Se è stato fatto l'update
//			TRUE  - Se la parola era già cosi: Nessuna modifica
// --------------------------------------------------
BOOL ultWordUpdate(EH_ULT * psUlt,INT iRow,INT iLang,void *lpWord,INT iCharSize,DWORD dwUser)
{

	ULTVOICE sVoice;
	WCHAR *lpElement;

	if (iLang==0) return TRUE; // Non posso modificare l'originale se no mi salta la ricerca binaria
	
	// -----------------------------------------------
	// Cancellazione Word                            |
	// -----------------------------------------------
    DMIRead(&psUlt->dmiDict,iRow,&sVoice);
	lpElement=sVoice.lpwText[iLang];

	if (!lpElement&&!lpWord) return TRUE; // Nessun cambio
	if (lpElement)
	{
		if (iCharSize==2&&lpWord)
		{
			if (!wcscmp(lpElement,lpWord)) return TRUE; // La parola è già cosi
		}

		if (sVoice.lpbText) {ehFree(sVoice.lpbText); sVoice.lpbText=NULL;}
		if (sVoice.lpwText[iLang]) {ehFree(sVoice.lpwText[iLang]); sVoice.lpwText[iLang]=NULL;}
		DMIWrite(&psUlt->dmiDict,iRow,&sVoice);
	}

	// -----------------------------------------------
	// Inserisco la nuova parola                     |
	// -----------------------------------------------
    DMIRead(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);
	ultAddWord(psUlt,lpWord,iLang,iCharSize,dwUser);
	ultItemSetFlag(psUlt);
    DMIWrite(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);
	return FALSE;
}

// -----------------------------------------------------------------
//
// _ultItemsLoadOld()
// Traduce il testo letto da disco dell'Item in memoria
//
// -----------------------------------------------------------------
static void _ultItemsLoadOld(EH_ULT * psUlt,
							 INT iType,
							 DWORD dwId,
							 CHAR *lpWords,
							 CHAR *lpFile,
							 BOOL fLost,
							 DWORD dwID)
{
  INT idx;
  CHAR *lpTok;
  INT iRow;
  WCHAR *lpw;
  BOOL fDup;

  if (iType==-1) ehExit("ULT: Type Invalid \"%s\" [%s]",lpWords+1,psUlt->szFile);

  // Estraggo la parola codice / Nativo
  lpTok=_tokExtract(psUlt,&lpWords); if (lpTok==NULL) ehExit("ULT:Null Campo codice ?");
  lpw=_tokDecode(psUlt,lpTok); if (!lpw) ehExit("errore Code = NULL");

  // Aggiungo /Cerco il nuovo idem
  iRow=ultItemInsert(psUlt,iType,lpw,dwID,&fDup);
  ehFree(lpw);
  //if (fDup) {win_infoarg("%d,%05x|%ls|",iType,dwID,lpw);}
  if (fDup) psUlt->iItemDup++;

  DMIRead(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);
  for (idx=1;;idx++)
  {
	 lpTok=_tokExtract(psUlt,&lpWords); 
	 lpw=_tokDecode(psUlt,lpTok);
	 // Carico solo la lingua selezionata
	 if (!psUlt->fLoadAll) 
	 {
		 if (psUlt->arLangReady[idx].idLang==psUlt->iLangWant) 
			{
			 ultAddWord(psUlt,lpw,1,2,UU_UNKNOW); 
			 break;
			}
	 }
	 // Carico Tutte le lingue
	 else 
	 {
	  ultAddWord(psUlt,lpw,idx,2,UU_UNKNOW); 
	 }
	 if (lpw) ehFree(lpw);
	 if (lpWords==NULL) break;
  }
  ultItemSetFlag(psUlt);//TRUE,fLost);
  DMIWrite(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);
} 

//
// _ultItemsLoad()
//
static void _ultItemsLoad(EH_ULT * psUlt,CHAR *lpItem,BOOL bUser)
{
	INT idx;
	CHAR *lpTok;
	INT iRow;
	WCHAR *lpw;
	BOOL fDup;
	INT iType;
	DWORD dwID,dwUser;
	WCHAR *lpUser;

	// Tipo 
	lpTok=_tokExtract(psUlt,&lpItem); if (!lpTok) ehExit("Sintax Error: Type ?");
	iType=_ultEnumType(lpTok);  if (iType==-1) ehExit("ULT: Type Invalid \"%s\" [%s]",lpTok,psUlt->szFile);

	// ID
	lpTok=_tokExtract(psUlt,&lpItem); if (!lpTok) ehExit("Sintax Error: ID");
	dwID=xtoi(lpTok);

	// Date (data primo inserimento/Data ultima modifica) DA FARE
	lpTok=_tokExtract(psUlt,&lpItem); //if (!lpTok) ehExit("Sintax Error: DTS");

	// User info (Ultimo utente che ha toccato quale items/lingua
	lpTok=_tokExtract(psUlt,&lpItem); //if (!lpTok) ehExit("Sintax Error: User Info");

	// Estraggo la parola "codice" : (spesso Lingua Nativa)
	lpTok=_tokExtract(psUlt,&lpItem); if (!lpTok) ehExit("Errore Code = NULL");
	lpw=_tokDecode(psUlt,lpTok); if (!lpw) ehExit("Errore Code Convert = NULL");
	//win_infoarg("[%ls]",lpw);

	if (bUser)
	{
		lpUser=wcsstr(lpw,L"\4"); *lpUser=0; 
		dwUser=_wtoi(lpUser+1);
	}
	// Aggiungo /Cerco il nuovo idem
	iRow=ultItemInsert(psUlt,iType,lpw,dwID,&fDup);
	ehFree(lpw);
	if (fDup) psUlt->iItemDup++;

	DMIRead(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);
	for (idx=1;;idx++)
	{
		lpTok=_tokExtract(psUlt,&lpItem); 
		lpw=_tokDecode(psUlt,lpTok);
		dwUser=UU_UNKNOW;
		if (bUser)
		{
			lpUser=wcsstr(lpw,L"\4"); *lpUser=0; 
			dwUser=_wtoi(lpUser+1);
		}

		// Carico solo la lingua selezionata
		if (!psUlt->fLoadAll) 
		{
			if (psUlt->arLangReady[idx].idLang==psUlt->iLangWant) 
			{
				ultAddWord(psUlt,lpw,1,2,dwUser); 
				break;
			}
		}
		// Carico Tutte le lingue
		else 
		{
			ultAddWord(psUlt,lpw,idx,2,dwUser); 
		}
		if (lpw) ehFree(lpw);
		if (lpItem==NULL) break;
	}
	ultItemSetFlag(psUlt);//TRUE,fLost);
	DMIWrite(&psUlt->dmiDict,iRow,psUlt->lpVoiceShare);
} 




// ----------------------------------------------------------
// Scansione
// 

// ----------------------------------------------------------------------
// Ritorna TRUE  = se ci sono errori
// Ritorna FALSE = se non ci sono errori
// Scandisce la cartella psUlt->szUltFolder/ULT
// 

typedef struct {
		CHAR	szFileShort[300];
		CHAR	szFileLong[300];
		FILETIME ftCT;
		SYSTEMTIME sTime;
		
		BOOL	fCopy; // Il file è da copiare
		BOOL	fTranslate; // Il file è da tradurre

		BOOL	fError; // errore nell'analisi del file
		//INT hdlFile;
		BOOL	fLoadFile;
		BYTE *	lpFileContent; // Contenuto del file

} S_SOURCE;

static BOOL _fileScanAdd(EH_ULT * psUlt,S_SOURCE *psSource);

//
// ultExtEnable()
//
BOOL ultExtEnable(CHAR *lpExt)
{
	CHAR *lpEstensioniPossibili="|.taf|.tcf|.inc|.html|.htm|.txt|.cfm|.php|.asp|.xml|.xsd|.js|.css|.strings|";
	CHAR szServ[20];
	sprintf(szServ,"|%s|",lpExt);
	if (strstr(lpEstensioniPossibili,lpExt)) return TRUE; else return FALSE;
}

//
// FileTextRemove()
//
void FileTextRemove(WCHAR *pwcListFiles,void *pFileSearch,INT iCharSize)
{
	WCHAR *pwcList;
	WCHAR *pwcFileSearch;
	WCHAR *pwcFindTag,*pw;
	INT iSize;

	if (!pwcListFiles) ehExit("pwcListFiles NULL");
	
	// Creo la stringa WCHAR per la di ricerca
	iSize=(wcslen(pwcListFiles)+3)*2;
	pwcList=ehAlloc(iSize);
	_snwprintf(pwcList,iSize,L",%s,",pwcListFiles);  

	// se è a un byte recodifico
	if (iCharSize==1) pwcFileSearch=strToWcs(pFileSearch); // Char > Unicode
					  else 
					  pwcFileSearch=wcsDup(pFileSearch);

	iSize=(wcslen(pwcFileSearch)+3)*2;
	pwcFindTag=ehAlloc(iSize); 	// Memo
	_snwprintf(pwcFindTag,iSize,L",%s,",pwcFileSearch); // file > ,file,
	while (wcsReplace(pwcList,pwcFindTag,L",")) // Tolgo il file da Wchar di controllo
	
	pwcList[wcslen(pwcList)-1]=0;
	pw=pwcList; if (*pw) pw++;
	wcscpy(pwcListFiles,pw);
	
	ehFree(pwcFileSearch);
	ehFree(pwcFindTag); 
	ehFree(pwcList); // Rilasco risorse
}

// ----------------------------------------------------------
// Creazione delle pagine in lingua
// ----------------------------------------------------------

typedef struct {
	CHAR *lpULTFolderRoot;
	_DMI *pdmiSource; // Puntatora a dmi che elenca i sorgenti
	CHAR *lpULL;	  // Puntatore a file ULL
	BOOL fRebuildAll;// Richiesta ricostruzione totale
} S_FOLDERSCAN;

S_FOLDERSCAN sFolderScan;

//
// _ultFileSearch()
// Genera una lista con l'elenco dei sorgenti presenti
// all'interno della cartella di ult
//
static void _ultFileSearch(CHAR *lpFolderSource)  // Folde da analizzare
{
	struct _finddata_t ffbk;
    LONG lFind;
	BYTE *p;
	CHAR szFolderScan[500];
	CHAR szFolder[500];
	CHAR szFileName[500];
	S_SOURCE sSource;
	FILETIME ftLAT,ftLWT;

	strcpy(szFolder,lpFolderSource);  AddBs(szFolder);
	strcpy(szFolderScan,szFolder); 
	strcat(szFolderScan,"*.*");
	lFind=_findfirst(szFolderScan, &ffbk);
	if (lFind!=-1)
	{
		do
		{
			//
			// Controllo la sotto cartella // se inizia diversamente da _
			//
			if (ffbk.attrib&_A_SUBDIR) 
			{
				CHAR szSubFolder[500];
				if (*ffbk.name!='_'&&*ffbk.name!='.')
				{
					sprintf(szSubFolder,"%s%s",szFolder,ffbk.name);
					_ultFileSearch(szSubFolder);
				}
				continue;
			}
			p=strReverseStr(ffbk.name,"."); if (p==NULL) continue;
			_strlwr(p);
			
			// Se inizia per _ o punto, il file non è valido (new 8/2007)
			if (*ffbk.name=='_'||*ffbk.name=='.') continue;
        
			//
			// Se rientra  tra le estensioni gestite
			//
			//if (ULTExtEnable(p)) 
			{
				HANDLE hSource;
				//
				// Dovrei controllare se è stato modificato dall'ultima volta
				//
				sprintf(szFileName,"%s%s",szFolder,ffbk.name);
				hSource=CreateFile(szFileName, // Leggo il sorgente
									GENERIC_READ,
									FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,NULL);
				if (hSource==INVALID_HANDLE_VALUE) 
				{
					ehPrintf("_ultFileSearch(): Non posso aprire [%s], errore %d",szFileName,osGetError());
					continue; // Lo salto
				}

				ZeroFill(sSource);
				if (!GetFileTime(hSource,&ftLWT,&ftLAT,&sSource.ftCT)) {
				
					ehExit("_ultFileSearch(): Non riesco a legge la data di [%s], errore %d",szFileName,osGetError());

				}
				if (hSource!=INVALID_HANDLE_VALUE) CloseHandle(hSource);
				if (!sSource.ftCT.dwHighDateTime&&!sSource.ftCT.dwLowDateTime) 
				{
					ehExit("_ultFileSearch(): Errore nella data del files [%s]",szFileName);
				}

				//
				// Controllare se devo inserirlo nella lista da fare
				//
				strcpy(sSource.szFileShort,szFileName);
				strReplace(sSource.szFileShort,sFolderScan.lpULTFolderRoot,"");
				strcpy(sSource.szFileShort,sSource.szFileShort);
				strcpy(sSource.szFileLong,szFileName);

				// ehLogWrite("SRC- %s",sSource.szFileShort);

				//
				// Controllo se è da creare
				//
				sSource.fTranslate=ultExtEnable(p);
				sSource.fCopy=true; 
				if (!strCmp(p,".strings")) sSource.fCopy=false; // Non duplico
				if (sFolderScan.lpULL&&!sFolderScan.fRebuildAll)
				{
					CHAR szServ[500];
					sprintf(szServ,"%s=%u:%u\n",
							sSource.szFileShort,
							sSource.ftCT.dwHighDateTime,
							sSource.ftCT.dwLowDateTime);
					if (strstr(sFolderScan.lpULL,szServ)) sSource.fCopy=FALSE;
				}
				DMIAppendDyn(sFolderScan.pdmiSource,&sSource);
			}
		} while(_findnext(lFind, &ffbk )==0);
		_findclose( lFind );
	}
}

//
//	ultFolderScan()
//  Scandisce i sorgenti presenti nella cartella alla ricerca di nuove parole
//  controlla l'associazione tra i sorgenti e le parole eliminando i file che non hanno
//  più riferimento
//
BOOL ultFolderScan(EH_ULT * psUlt) {

	CHAR *lpFileCurrent;
	CHAR *lpWord;
	BYTE * pszFiles;
	BOOL fError=FALSE;
//	BYTE *lpCharString;
	WCHAR *pwcFileMemo;
	S_SOURCE sSource;
	INT a,x,idx;
	_DMI dmiSource=DMIRESET;
	INT idxFile;
	BOOL bDictLock;

	if (!fileCheck(psUlt->szUltFolder)) {win_infoarg("Non esiste la cartella ULT con i sorgenti"); return TRUE;}

	// -----------------------------------------------------------------------------------
	//  FASE 0 
	//  Chiedo l'elenco dei file da anlizzare
	// -----------------------------------------------------------------------------------

	DMIOpen(&dmiSource,RAM_AUTO,10000,sizeof(S_SOURCE),"SOURCES");

	_(sFolderScan);
	sFolderScan.lpULTFolderRoot=psUlt->szUltFolder;
	sFolderScan.pdmiSource=&dmiSource;

	//
	//
	//
	_ultFileSearch(psUlt->szUltFolder); // Richiesta di analisi ricorsiva
	//
	//
	//

	// -----------------------------------------------------------------------------------
	//  FASE 1 (Additiva)
	// 
	//  Scandaglio i file della cartella Root/ULT aggiungendo le frasi che mancano
	// -----------------------------------------------------------------------------------
	
	memoLock(dmiSource.Hdl);
	for (a=0;a<dmiSource.Num;a++)
	{
		DMIRead(&dmiSource,a,&sSource);
		sSource.fLoadFile=true;
		_fileScanAdd(psUlt,&sSource);
//		sSource.fError=_fileScanAdd(&sSource);
		fError|=sSource.fError;
		DMIWrite(&dmiSource,a,&sSource);
	}

	// win_infoarg("? DOPO %d",dmiSource.Num); return TRUE;


	// ------------------------------------------------------------------------------------------------------------------------------
	// FASE 2 (Analisi sottrattiva)
	//
	// Scandaglio gli items del dizionario togliendo i file che non li contengono
	// A) Loop sul dizionario 
	// B) Loop sui file collegati per verificare se le stringhe sono utilizzate almeno una volta
	//
	// ------------------------------------------------------------------------------------------------------------------------------
	
	lpWord=ehAlloc(64000);

	DMILock(&psUlt->dmiDict,&bDictLock);
	DMILock(&dmiSource,NULL);

	for (idx=0;idx<psUlt->dmiDict.Num;idx++) {

		DMIRead(&psUlt->dmiDict,idx,psUlt->lpVoiceShare);
		if (psUlt->lpVoiceShare->iType!=ULT_TYPE_HTML) continue;

		//
		// Potrebbe non essere carico
		//
		if (psUlt->lpVoiceShare->lpwText[psUlt->idxFiles])
		{
			
//			lpCharString=wcsToStr(psUlt->lpVoiceShare->lpwText[psUlt->idxLangCode]);
			EH_ARF ar;
			//
			// Leggo i file da analizzare e li converto in CHAR
			//
			pszFiles=wcsToUtf(psUlt->lpVoiceShare->lpwText[psUlt->idxFiles]);
			pwcFileMemo=wcsDup(psUlt->lpVoiceShare->lpwText[psUlt->idxFiles]);
//			lpFileCurrent=lpcFiles;
			ar=ARFSplit(pszFiles,",");
			
			for (x=0;ar[x];x++) {
			
			//while (TRUE)
//			{
//				if (!lpFileCurrent) ehError();
//				p=strstr(lpFileCurrent,","); if (p) *p=0;
				EN_CHARTYPE enSourceEncode;
				CHAR * pszUltTag;
				lpFileCurrent=ar[x];
				enSourceEncode=_ultGetSourceEncode(psUlt,lpFileCurrent);
				switch (enSourceEncode) {

					case ULT_CS_UTF8:
						pszUltTag=wcsToUtf(psUlt->lpVoiceShare->lpwText[psUlt->idxLangCode]);
						break;

					default:
						pszUltTag=wcsToStr(psUlt->lpVoiceShare->lpwText[psUlt->idxLangCode]);
						break;

				}
				//
				// Cerco il file in tutte i files sorgenti
				//
				idxFile=-1;
				for (a=0;a<dmiSource.Num;a++)
				{
					DMIRead(&dmiSource,a,&sSource);
					if (!strcmp(sSource.szFileShort,lpFileCurrent)) {idxFile=a; break;}
				}

				//
				// Trovato
				//
				if (idxFile>-1)
				{

					INT e;
					BOOL fFound=FALSE;

					if (!strEnd(lpFileCurrent,".strings")) {

						UTF8 * putf=wcsToUtf(psUlt->lpVoiceShare->lpwText[psUlt->idxLangCode]);
						sprintf(lpWord,"\"%s\"",putf);
						if (strstr(sSource.lpFileContent,lpWord)) 
						{
							fFound=TRUE;
						//	break;
						}
						ehFree(putf);
					}
					else {

						for (e=0;arEnc1[e];e++)
						{
							sprintf(lpWord,arEnc1[e],pszUltTag);
							if (!sSource.lpFileContent) 
								ehError();
							if (strstr(sSource.lpFileContent,lpWord)) 
							{
								fFound=TRUE;
								break;
							}
						}
					}
					if (!fFound) FileTextRemove(pwcFileMemo,lpFileCurrent,1);
				}
				else
				// C il file non esiste più
				{
					FileTextRemove(pwcFileMemo,lpFileCurrent,1);
					_ultFileRemove(psUlt,lpFileCurrent); // CANCELLO IL FILE DA TUTTO IL DIZIONARIO
				}
				ehFree(pszUltTag);
//				if (p) {*p=','; lpFileCurrent=p+1;} else break;
			}
			ehFree(ar);

			// Potrebbe non esserci
			if (psUlt->lpVoiceShare->lpwText[psUlt->idxFiles])
			{
				if (wcscmp(pwcFileMemo,psUlt->lpVoiceShare->lpwText[psUlt->idxFiles]))
				{ 
					ultWordUpdate(psUlt,idx,psUlt->idxFiles,pwcFileMemo,2,UU_SOURCE);
				}	
			}

			ehFreePtrs(2,&pwcFileMemo,&pszFiles);
		}
	}
	ehFree(lpWord);
	if (bDictLock) DMIUnlock(&psUlt->dmiDict);


	DMIClose(&dmiSource,"dmiSource");
	return fError;
}
//
// _ultTagSearch()
//
// Ritorna 0 = TUTTO OK
//		   TRUE = Errori
//		   
//
static INT _ultTagSearch(	EH_ULT  *	psUlt,
							BYTE *		lpMemo,
							EN_ULTYPE	iTagType,
							CHAR *		lpDelimStart,
							CHAR *		lpDelimStop,
							EN_CHARTYPE	enSourceEncode,
							CHAR *		lpFileName)
{
	BYTE *lpScan,*lpStart,*lpEnd;
	BOOL fRet=FALSE;
	INT iLenDelimStart=strlen(lpDelimStart);
	BYTE cBackup;
	UINT uiTagSizeMax=MAX_TAG_SIZE;
	lpScan=lpMemo;
	// return 0;

	while (true)
	{
		lpStart=strstr(lpScan,lpDelimStart); if (!lpStart) break;
		lpStart+=iLenDelimStart;
		lpEnd=strstr(lpStart,lpDelimStop); 
		if (!lpEnd) 
		{ 
			ehPrintf("%s" CRLF "? Tag aperto<br />\n%s\n",lpFileName,lpStart);
			fRet=TRUE; 
			break;
		} // Tag Aperto

		cBackup=*lpEnd;	*lpEnd=0; 

		//if (strlen(lpStart)>1000) win_infoarg("[%s]",lpStart); 
		if (strstr(lpStart,lpDelimStart)||strstr(lpStart,lpDelimStop))
		{
			ehPrintf("? Mark errato in file %s\nFrase:\n%s\n",lpFileName,lpStart);
			fRet=TRUE;
			//break;
		}
		else
		{
			WCHAR *lpw;
			switch (enSourceEncode) {

				case ULT_CS_UTF8:
					lpw=strDecode(lpStart,SD_UTF8,NULL);
					//if (!strBegin(lpStart,"Quanti")) 
					//	ehPrintf("qui");
//					ehLogWrite("UTF8->[%s]=%S",lpStart,lpw);
					break;

				default:
				case ULT_CS_LATIN1:
					lpw=strToWcs(lpStart);
					break;

			}

			if (wcslen(lpw)>uiTagSizeMax) {

				ehPrintf("? Tag troppo grande > %d : %s\nFrase:\n%S\n",uiTagSizeMax,lpFileName,lpw);
				fRet=TRUE;

			} else {

				ultTranslateEx(	psUlt,
								ULT_TYPE_HTML,
								lpw,
								lpFileName,
								true, // Creo se è il caso
								NULL); 

			}
			ehFree(lpw);
		}
		*lpEnd=cBackup; 
		lpScan=lpEnd+strlen(lpDelimStop);
	}
	return fRet;
}
// ---------------------------------------------------------
// Analisi Additiva
// Analizza il file in cerca dei tag
// Se ci sono frasi nuove le aggiunge 
//
// ---------------------------------------------------------

//
// _fileStringScan() - 
//
static BOOL _fileStringScan(EH_ULT * psUlt,
							CHAR * pszFileContent,
							S_SOURCE *psSource) {

//	CHAR * pszFileContent=fileStrRead(psSource->szFileLong); 
	INT a;
	EH_AR ar;
	CHAR * psz;
	WCHAR * lpw;

	if (!pszFileContent) ehExit("Errore");
	ar=ARFSplit(pszFileContent,"\n");
	for (a=0;ar[a];a++) {

		psz=strExtract(ar[a],"\"","\"=\"",false,false);
		if (!strEmpty(psz)) {
			// ehPrintf("- %s" CRLF,psz);
			//_ultTagSearch(lpFileContent,ULT_TYPE_HTML,UTF8_TAG,")#",MAX_TAG_SIZE,psSource->szFileShort);

			lpw=utfToWcs(psz);
			ultTranslateEx(	psUlt,
							ULT_TYPE_HTML,
							lpw,
							psSource->szFileShort,
							true, // Creo se è il caso
							NULL); 
			ehFree(lpw);
		}
		ehFreeNN(psz);

	}
	ehFree(ar);
	psSource->fError=FALSE;
	return FALSE;

} 

//
// _ultGetSourceEncode() - Determina l'encoding del sorgente
//
static EN_CHARTYPE _ultGetSourceEncode(EH_ULT *psUlt,CHAR * pszFileName) {

	EN_CHARTYPE enChar;
	CHAR szServ[300];
	CHAR * psz;

	enChar=psUlt->iCS_Source;
	if (psUlt->pszEncodingFilesSource) {
		sprintf(szServ,"%s:",pszFileName);
		psz=ARSearchBegin(psUlt->arEFS,szServ,true,true);
		if (psz) 
		{
			enChar=ultTextToCode(psUlt,1,psz);
		}
	}
	return enChar;
}

//
// _fileScanAdd()
//
static INT _fileScanAdd(EH_ULT *psUlt,S_SOURCE * psSource) {

	BOOL fRet=FALSE;
	BYTE *lpFileContent;

	//
	// strcpy(szFileName,fileName(lpFile));
	//
	
	if (!psSource->fTranslate) return false;
	lpFileContent=fileStrRead(psSource->szFileLong); if (!lpFileContent) ehExit("Errore");

	//
	// File .strings (Mac)
	//
	if (!strEnd(psSource->szFileLong,".strings")) {

		fRet=_fileStringScan(psUlt,lpFileContent,psSource);

	}
	else {

		//
		// Scansione TAG per HTML/WTAG
		//
		EN_CHARTYPE enSourceEncode=_ultGetSourceEncode(psUlt,psSource->szFileShort);

		fRet|=_ultTagSearch(psUlt,lpFileContent,ULT_TYPE_HTML,"#[","]#",enSourceEncode,psSource->szFileShort);
		fRet|=_ultTagSearch(psUlt,lpFileContent,ULT_TYPE_HTML,WITANGO_TAG,")#",enSourceEncode,psSource->szFileShort);
		fRet|=_ultTagSearch(psUlt,lpFileContent,ULT_TYPE_HTML,JAVA_TAG,")#",enSourceEncode,psSource->szFileShort);
		fRet|=_ultTagSearch(psUlt,lpFileContent,ULT_TYPE_HTML,HTMLPURE_TAG,")#",enSourceEncode,psSource->szFileShort);
		fRet|=_ultTagSearch(psUlt,lpFileContent,ULT_TYPE_HTML,UTF8_TAG,")#",enSourceEncode,psSource->szFileShort);
		fRet|=_ultTagSearch(psUlt,lpFileContent,ULT_TYPE_HTML,HTMLUTF8_TAG,")#",enSourceEncode,psSource->szFileShort);
	}

	if (psSource->fLoadFile) 
		psSource->lpFileContent=lpFileContent;
		else
		ehFree(lpFileContent);


	// ehPrintf("%s>%d" CRLF,psSource->szFileShort,fRet);
	//if (!lpiHdl) memoFree(hdl,"File"); else *lpiHdl=hdl;
	psSource->fError=fRet;
	return fRet;
}

//
//	ultGeneralFileBuilder()
//  Costruisce file e folder partendo dalla cartella sorgente e traducendo
//  quello che c'è da tradurre
//

void ultGeneralFileBuilder(EH_ULT * psUlt,
						   CHAR *	lpFileUlt,
					       BOOL		fOpenDizionario,
					       BOOL		fRebuildAll,
					       BOOL		fShowFilesTouch)
{

	//CHAR szFileScan[500];
	CHAR szULTFolder[500];
	CHAR szULTFile[500];
	CHAR szULLFile[500];
	CHAR szNameULT[500];
	CHAR szDestFolder[500];
	CHAR *p,*lpLang;
	INT a,x;
#ifndef _DEBUG
	INT iCount;
	FILE * ch;
#endif
	EH_ULT sBackup;
	BYTE *lpFolderBase;
	S_SOURCE sSource;

//	FILE *ch;
	_DMI dmiSource=DMIRESET;

	chronoStart(NULL);
	_(sFolderScan);

	// Apro il log della "Costruzione" se richiesto
	if (psUlt->fLogWrite) 
	{
		CHAR szDestFolder[500];
		sprintf(szDestFolder,"c:\\Comferra\\Ult\\Logs");
		if (!fileCheck(szDestFolder)) CreateDirectory(szDestFolder,NULL);
		sprintf(psUlt->szLogFile,"c:\\Comferra\\Ult\\Logs\\Log_%s.txt",dtNow());
		_ultLogOpen(psUlt);
	}

	if (!lpFileUlt) {ehExit("Manca il nome della cartella."); return;}

	// A) Cerco se la cartella esiste
	if (!fileCheck(lpFileUlt)) {ehExit("il file %s non esiste",lpFileUlt); return;}

	// B) Cerco la cartella ULT

	p=strReverseStr(lpFileUlt,"."); if (!p) ehExit("ULTWebBuilder() A: %s",lpFileUlt);
	lpFolderBase=lpFileUlt;
	p=strReverseStr(lpFileUlt,"\\"); if (!p) ehExit("??????????");
	strcpy(szNameULT,p+1); *p=0;
	p=strReverseStr(szNameULT,"."); if (p) *p=0; else ehExit("ULTWebBuilder() B: %s",szNameULT);

	strcpy(szULTFolder,lpFolderBase); //AddBs(szULTFolder);
	strcat(szULTFolder,"\\ULT");
	sprintf(szULTFile,"%s\\%s.ult",lpFolderBase,szNameULT);
	sprintf(szULLFile,"%s\\%s.ull",lpFolderBase,szNameULT);

	// Apro il dizionario
	if (fOpenDizionario)
	{
		if (!ultOpen(psUlt,szULTFile,EH_LANG_ITALIAN,TRUE))
		{

#ifdef EH_CONSOLE
			ehPrintf("Non è presente il dizionario %s\n",szULTFile);
			_ultLogWrite(psUlt,"Non è presente il dizionario %s",szULTFile);
			psUlt->fLogError=TRUE;
			_ultLogClose(psUlt);
#endif
			return;
		}
	}

	// Backup
	memcpy(&sBackup,psUlt,sizeof(EH_ULT));
	
	//
	// Inizializzazione struttura di servizio
	//
	sFolderScan.fRebuildAll=fRebuildAll;

	// ------------------------------------
	// Leggo il file ULL
	//
	sFolderScan.lpULL=NULL;
	if (!fRebuildAll)
	{
		if (fileCheck(szULLFile))
		{
			sFolderScan.lpULL=fileStrRead(szULLFile);
		} 
	}

	// ------------------------------------------
	// Loop sui file contenuti nella cartella
	//
	//sprintf(szFileScan,"%s\\*.*",szULTFolder);

	//
	// D) Scandaglio i file della cartella Root e genero i file ULT
	//
	DMIOpen(&dmiSource,RAM_AUTO,300,sizeof(S_SOURCE),"SOURCES");
	sFolderScan.pdmiSource=&dmiSource;
	sFolderScan.lpULTFolderRoot=szULTFolder;
	DMILock(&dmiSource,NULL);

	//
	// _ultFileSearch()
	//
	_ultFileSearch(szULTFolder); // Richiesta di analisi ricorsiva

	//
	//
	//

	//memoLock(dmiSource.Hdl);
	ehFreeNN(sFolderScan.lpULL);
	ehLogWrite("Controllo %d sorgenti. Lingue:%d",dmiSource.Num,psUlt->iLangNum);
	// ------------------------------------------
	// Loop sulle lingue
	//
	for (a=0;a<psUlt->iLangNum;a++) 
	{
		// Salto la lingua codice e i files
		//if (a==psUlt->idxLangCode||a==psUlt->idxFiles) continue;
		if (!ultIdxIsLang(psUlt,a)) continue;
		lpLang=psUlt->arLangReady[a].lpIsoPrefix;//ultCodeToText(0,psUlt->iLangList[a]);

		if (*lpLang=='#') continue; // Ulteriore sicurezza
		if (!lpLang) ehExit("Errore in ULTWebBuilder(%s)",lpLang);

//		ehPrintf("OK %s<BR>",ultCodeToText(psUlt->iLangList[a])); fflush(stdout);
		ultSetLangIdx(psUlt,a,-1);

		//
		// C) Controllo se ho la directory di destinazione
		//
		sprintf(szDestFolder,"%s\\%s",lpFolderBase,lpLang);
		if (!fileCheck(szDestFolder)) CreateDirectory(szDestFolder,NULL);

		//
		// D) Costruisco i file necessari
		//

		for (x=0;x<dmiSource.Num;x++)
		{
			DMIRead(&dmiSource,x,&sSource); 
			if (!sSource.fCopy) continue; // Non è da copiare, già ce l'ho
			if (sSource.fTranslate)
			{
				 ehLogWrite("Builder %s [%s] ...",sSource.szFileShort,lpLang);
				 _ultFileBuilder(	psUlt,
									sSource.szFileLong,	// Nome del file originale
									sSource.szFileShort, // Nome senza cartella di origine
									szDestFolder,		// Nome della cartella di destinazione in lingua
									true,
									fShowFilesTouch);

			}
			else
			{
				CHAR szDest[500];
				sprintf(szDest,"%s\\%s",szDestFolder,sSource.szFileShort);
				ehLogWrite("Copy %s [%s] ...",sSource.szFileShort,lpLang);
				dirCreateFromFile(szDest);
				fileCopy(sSource.szFileLong,szDest,true);
			}

		}

		// ------------------------------------------------------------------------------------
		//
		// Facciamo pulizia
		// Scandaglio le cartelle di ogni lingua ed elimino i files che non sono più usati
		//
		// ------------------------------------------------------------------------------------
		// DA FARE
		/*
		sprintf(szFileScan,"%s\\%s\\*.*",lpFolderBase,lpLang);
		ARMaker(WS_OPEN,NULL);
		lFind=_findfirst(szFileScan, &ffbk);
		if (lFind!=-1)
		{
			do
			{
				if (ffbk.attrib&_A_SUBDIR) continue; // Non gestisco il ricorsivo (per ORA)
				p=strReverseStr(ffbk.name,"."); if (p==NULL) continue;
				_strlwr(p);
        
				if (ULTExtEnable(p)) // Se rieL_SetLangGoogle tra le estensioni gestite
				{
					BOOL fRemove=TRUE;
					for (x=0;x<dmiSource.Num;x++)
					{
						DMIRead(&dmiSource,x,&sSource); 
						if (!strcmp(sSource.szName,ffbk.name)) {fRemove=FALSE; break;}
					}
	
					if (fRemove) ARMaker(WS_ADD,ffbk.name);
				}
			} while(_findnext(lFind, &ffbk )==0);
		}
		_findclose( lFind );
		hdl=ARMaker(WS_CLOSE,"FileRemove");
		arRem=memoPtr(hdl,NULL);
		// In ogni cartella ci sono file che mi servono, e per il momento non so come risolvere il problema
		for (x=0;arRem[x];x++)
		{
			//ehPrintf("Delete %s\\%s\\%s ...\n",lpFolderBase,lpLang,arRem[x]);
		}
		memoPtr(hdl,NULL);
		*/
		
	}

	// Scrivo su disco il file ULL
#ifndef _DEBUG
	iCount=0;
	while (TRUE)
	{
		 ch=fopen(szULLFile,"wb"); 
		 if (!ch) //ehExit("Write Error: %s (%d)",szULLFile,osGetError());
		 {
			 iCount++; if (iCount<10) {Sleep(1000); continue;}
			 ehExit("Write Error: %s (%d)",szULLFile,osGetError());
		 }
		 break;
	}
	for (x=0;x<dmiSource.Num;x++)
	{
		DMIRead(&dmiSource,x,&sSource); 
		fprintf(ch,"%s=%u:%u\n",
				sSource.szFileShort,
				sSource.ftCT.dwHighDateTime,
				sSource.ftCT.dwLowDateTime);
	}
	fclose(ch);
#endif

	DMIClose(&dmiSource,"SOURCES");
	_ultLogClose(psUlt);
	printf("done: %s" CRLF,chronoFormat(chronoGet(NULL),1));
	//
	// Restore
	//
	memcpy(psUlt,&sBackup,sizeof(EH_ULT));
	
	if (fOpenDizionario) ultClose(psUlt);
}
/*
static void WebFileBuilder(CHAR *lpFile,CHAR *lpDestFolder,CHAR *lpLang)
{
	INT hdl;
	BYTE *lp,*lpScan,*lpStart,*lpEnd;
	BYTE *p;
	CHAR *lpBuffer=ehAlloc(0xFFFFFF);
	INT iMemo,hdlMemo;
	CHAR *lpMemo,*lpNewWord;
	CHAR szNewName[500];
	FILE *ch;
	hdl=fileLoad(lpFile,RAM_AUTO);
	lp=memoLock(hdl);
	iMemo=strlen(lp);

	hdlMemo=memoAlloc(RAM_AUTO,iMemo*2,"File");
	lpMemo=memoLock(hdlMemo);
	memcpy(lpMemo,lp,iMemo); lpMemo[iMemo]=0;
	memoFree(hdl,"Originale");

	lpScan=lpMemo;
	while (TRUE)
	{
		lpStart=strstr(lpScan,"#["); if (!lpStart) break;
		lpStart+=2;
		lpEnd=strstr(lpStart,"]#"); if (!lpEnd) break;
		*lpEnd=0; 

		sprintf(lpBuffer,"#[%s]#",lpStart);
		p=ultTranslate(ULT_TYPE_HTML,lpStart);
		lpNewWord=ehAlloc(strlen(p)+1); strcpy(lpNewWord,p);
		*lpEnd=']';
		while (strReplace(lpMemo,lpBuffer,lpNewWord));
		ehFree(lpNewWord);
	}
	
	while (strReplace(lpMemo,"#LANG#",lpLang));

	// Salvo il file
	sprintf(szNewName,"%s\\%s",lpDestFolder,fileName(lpFile));

	ch=fopen(szNewName,"wb");
	fwrite(lpMemo,strlen(lpMemo),1,ch);
	fclose(ch);

	//ehPrintf("\n");
	memoFree(hdlMemo,"File");
	ehFree(lpBuffer);
}
*/

//
// ultCodeToText()
//
CHAR * ultCodeToText(EH_ULT * psUlt,INT iMode,INT iCode)
{
	CHAR *lpLang=NULL;
	EH_ULT_LANG *lpUI;
	switch (iMode)
	{
	case 0:

		lpUI=ultLangInfo(psUlt,iCode,NULL); if (!lpUI) ehExit("errore Code lingua");
		lpLang=lpUI->lpIsoPrefix;
		break;

	case 1:
		switch (iCode)
		{
			case ULT_CS_LATIN1:	  lpLang="Latin1"; break;
			case ULT_CS_UTF8:	  lpLang="UTF-8"; break;
			default: break;
		}
		break;

	case 2:
		switch (iCode)
		{
			case ULT_ENCODE_ISO:  lpLang="ISO"; break;
			case ULT_ENCODE_UTF8: lpLang="UTF-8"; break;
			default: break;
		}
		break;
	}

	return lpLang;
}

//
// ultTextToCode()
//
INT ultTextToCode(EH_ULT * psUlt,INT iMode,CHAR *p)
{
	INT iCode=-1;
	EH_ULT_LANG *lpUI;
	switch (iMode)
	{
		case 0:

				lpUI=ultLangInfo(psUlt,0,p); if (!lpUI) ehExit("errore Code lingua");
				iCode=lpUI->idLang;

			/*
				if (!strCaseCmp(p,"IT")) iCode=EH_LANG_ITALIAN;
				if (!strCaseCmp(p,"UK")||!strcmp(p,"EN")) iCode=EH_LANG_ENGLISH;
				if (!strCaseCmp(p,"FR")) iCode=EH_LANG_FRENCH;
				if (!strCaseCmp(p,"ES")) iCode=EH_LANG_SPAIN;
				if (!strCaseCmp(p,"DE")) iCode=EH_LANG_DEUTCH;
				*/
				
				break;
		case 1:
				if (!strCaseCmp(p,"Latin1")) iCode=ULT_CS_LATIN1;
				if (!strCaseCmp(p,"UTF-8")) iCode=ULT_CS_UTF8;
				break;

		case 2:
				if (!strCaseCmp(p,"ISO")) iCode=ULT_ENCODE_ISO;
				if (!strCaseCmp(p,"UTF-8")) iCode=ULT_ENCODE_UTF8;
				break;
	}
	return iCode;
}

//
// ultTypeToText()
//
CHAR * ultTypeToText(INT iCode)
{
	CHAR *lpType;
	switch (iCode)
	{
			case ULT_TYPE_WINT: lpType="WINT"; break;
			case ULT_TYPE_WINI: lpType="WINI"; break;
			case ULT_TYPE_OBJS: lpType="OBJS"; break;
			case ULT_TYPE_HMZ:  lpType="HMZ"; break;
			case ULT_TYPE_OBJ:  lpType="OBJ"; break;
			case ULT_TYPE_MENU: lpType="MENU"; break;
			case ULT_TYPE_SPF:  lpType="SPF"; break;
			case ULT_TYPE_DISP: lpType="DISP"; break;
			case ULT_TYPE_LIST: lpType="LIST"; break;

			case ULT_TYPE_HTML: lpType="HTML"; break;
			case ULT_TYPE_JAVA: lpType="JAVA"; break;\
			//case ULT_TYPE_WTAG: lpType="WTAG"; break;

			default: ehExit("ULT Type <?> %d",iCode); break;
	}
	return lpType;
}

//
// _witangoEncoding()
//
WCHAR * _witangoEncoding(WCHAR *pcwWordTrans)
{
	WCHAR *pwBuffer=ehAlloc(wcslen(pcwWordTrans)*10); // Esagero
	wcscpy(pwBuffer,pcwWordTrans);
	while (wcsReplace(pwBuffer,L"\'",L"<@SQ>"));
	while (wcsReplace(pwBuffer,L"\"",L"<@DQ>"));
//	while (wcsReplace(pwBuffer,L"\n\r",L"<@CRLF>"));
//	while (wcsReplace(pwBuffer,L"\r",L"<@CR>"));
//	while (wcsReplace(pwBuffer,L"\n",L"<@LF>"));
	return pwBuffer;
}

//
// _javaScriptEncoding()
//
WCHAR * _witangoDecoding(WCHAR *pcwWordTrans)
{
	WCHAR * pwBuffer=ehAlloc(wcslen(pcwWordTrans)*10); // Esagero
	wcscpy(pwBuffer,pcwWordTrans);
	/*
	while (wcsReplace(pwBuffer,L"\'",L"<@SQ>"));
	while (wcsReplace(pwBuffer,L"<@SQ>",L"\\\'"));
	while (wcsReplace(pwBuffer,L"\"",L"<@DQ>"));
	while (wcsReplace(pwBuffer,L"<@DQ>",L"\\\""));
	*/
	while (wcsReplace(pwBuffer,L"<@SQ>",L"\'"));
	while (wcsReplace(pwBuffer,L"<@DQ>",L"\""));
	while (wcsReplace(pwBuffer,L"<@CR>",L"\r"));
	while (wcsReplace(pwBuffer,L"<@LF>",L"\n"));
	while (wcsReplace(pwBuffer,L"<@CRLF>",L"\r\n"));
	return pwBuffer;
}

//
// _ltgtEncoding()
//
/*
WCHAR * _ltgtEncoding(WCHAR *pcwWordTrans)
{
	WCHAR *pwBuffer=ehAlloc(wcslen(pcwWordTrans)*10); // Esagero
	wcscpy(pwBuffer,pcwWordTrans);
	while (wcsReplace(pwBuffer,L"<",L"&lt;"));
	while (wcsReplace(pwBuffer,L">",L"&gt;"));
	return pwBuffer;
}
*/


// -------------------------------------------------------------------------------
// _ultFileBuilder()
// Costruisce il file in lingua partendo dal sorgente in ULT
//
// lpFileSource - Nome del file sorgente compreso di percorso (folder)
// lpFolderDest - Cartella di destinazione
//
// Ritorna -1 se il file non esiste
//

INT _ultFileBuilder(EH_ULT * psUlt,
					 CHAR *lpFileSourceLong, // Nome completo del file sorgente
					 CHAR *lpFileSourceShort,
					 CHAR *lpFolderLang, // Nome della Folder in Lingua
					 BOOL fRemoveFileReference, // T/F se rimuovere in automatico dal dizionario "lpFileSource" inesistente.
					 BOOL fShowFileTouch)
{
					
	INT hdl;
	BYTE *lp,*lpStart,*lpEnd;
	WCHAR *pcwWordTrans;
	CHAR *lpTagFind=NULL;
	INT iFileSize,hdlMemo=0;
	CHAR *lpMemo;
	BYTE *lpCharText;
	CHAR szFileTarget[500];
	CHAR szIsoLower[10];
	CHAR *lpIsoLang=psUlt->arLangReady[psUlt->idxLangTranslated].lpIsoPrefix;//ultCodeToText(0,psUlt->iLangList[psUlt->iLangSelector]);
	INT iCount;
	static INT iPos=0;
	HANDLE hdlFile;
	INT iLen;
	DWORD dwBytesWritten;
	INT iItemType;
	EN_ULT_TAGTYPE enTagType;			// Encoding in uscita
	INT iRow;
	WCHAR *pwcFind,*pwcEncode=NULL;
	BYTE charBackup;
	INT iCountSwap=0;
	clock_t start;
	CHAR * psz;
	CHAR * pszPoint;
	DWORD dwMemoSize;

	EN_OUT_TARGET	enOutTarget=0,enOutTargetStandard=0;
	EN_STRENC		enOutCharEncoding=0;
	S_TAGFILEINFO	sTagFileInfo;
	INT		iTags;


// 	printf("[a] %s" CRLF,chronoFormat(chronoGet(NULL),1));

	start=clock();
	if (!lpIsoLang) ehExit("?lpLang = NULL (%d)",psUlt->idxLangTranslated);

	if (!fileCheck(lpFileSourceLong)) 
	{
		if (fRemoveFileReference)
		{
			_ultFileRemove(psUlt,fileName(lpFileSourceLong)); return -1;
		}
		else
		{
			if (fShowFileTouch) printf ("? File %s inesistente",lpFileSourceLong); //return -1;
			_ultLogWrite(psUlt,"?File %s inesistente",lpFileSourceLong); 
			psUlt->fLogError=TRUE;
			//_ultLogClose();
			return -1;
		}
	}
	

	//
	// Carico in memoria il file
	//
	iCount=0;
	while (TRUE)
	{
		hdl=fileLoad(lpFileSourceLong,RAM_AUTO); 
		if (hdl<0)
		{
			iCount++; if (iCount<5) {Sleep(1000); continue;} // Conto fino a 5
			if (fShowFileTouch) 
			{
				if (fShowFileTouch) ehPrintf("?fileLoad_error ? :[%s]\n",lpFileSourceLong);
				_ultLogWrite(psUlt,"?fileLoad_error ? :[%s]\n",lpFileSourceLong);
				psUlt->fLogError=TRUE;
				return -1;
			}
			else
			{
				//ehExit("?fileLoad_error ? :[%s]\n",lpFileSourceLong);
				_ultLogWrite(psUlt,"?fileLoad_error ? :[%s]\n",lpFileSourceLong);
				psUlt->fLogError=TRUE;
				return -1;
			}
		}
	 break;
	}

	lpTagFind=ehAlloc(3048);
	lp=memoLock(hdl); iFileSize=strlen(lp);

	dwMemoSize=iFileSize*2+1024;
	hdlMemo=memoAlloc(RAM_AUTO,dwMemoSize,"File");
	lpMemo=memoLock(hdlMemo); //memset(lpMemo,0,dwMemoSize);
	memcpy(lpMemo,lp,iFileSize); 
	lpMemo[iFileSize]=0;
	memoFree(hdl,"Originale");

	//
	// Preset
	//
	enOutTargetStandard=OUT_WEB_HTML;
	
	switch (psUlt->iEncodeHtml)
	{
		case ULT_CS_UTF8:   
			enOutCharEncoding=SE_UTF8;
			break;

		case ULT_CS_LATIN1: 
		default:
			enOutCharEncoding=SE_HTML;
			break;
	}

	if (_getTagFileInfo(lpMemo,&sTagFileInfo,true)) {
		if (sTagFileInfo.fileEncoding) enOutCharEncoding=sTagFileInfo.fileEncoding;
		if (sTagFileInfo.enOutTarget) enOutTargetStandard=sTagFileInfo.enOutTarget;
	}


	psz=strExtract(lpMemo,"#{","}#",false,true);
	if (psz) {
		EH_JSON * psJson;
		CHAR *p;
		psJson=jsonCreate(psz+1);

		// Setta il target standard di output
		p=jsonGet(psJson,"stdTarget");
		if (!strCmp(p,"web")) enOutTargetStandard=OUT_WEB;
		if (!strCmp(p,"webhtml")) enOutTargetStandard=OUT_WEB_HTML;
		if (!strCmp(p,"xml")) enOutTargetStandard=OUT_XML;
	
		jsonDestroy(psJson);
		strReplace(lpMemo,psz,"");
		ehFree(psz);
	}


	//
	// Tag replacing
	//
	iTags=0;
	enTagType=ULT_TAG_STD;
	pszPoint=lpMemo;
// 	printf("filesize:%d " CRLF,strlen(lpMemo));
	while (true)
	{
		iItemType=-1;
		

		//
		// Trovo il Tag
		//
		lpStart=lpEnd=NULL;
		switch (enTagType) {
		
			//
			// Cerco un Tag Html normale
			//
			case ULT_TAG_STD:
				pszPoint=lpStart=strstr(pszPoint,"#["); 
				if (lpStart) 
				{
			//		enTagType=ULT_TAG_STD;//ULT_TAG_HTMLS; 
					iItemType=ULT_TYPE_HTML;
					lpStart+=2;
					lpEnd=strstr(lpStart,"]#"); 
				}
				break;


			case ULT_TAG_HTMLP:
				 pszPoint=lpStart=strstr(pszPoint,HTMLPURE_TAG); 
				 if (lpStart) 
					{
						enTagType=ULT_TAG_HTMLP; 
						iItemType=ULT_TYPE_HTML; 
						lpStart+=4; 
						lpEnd=strstr(lpStart,")#"); 
					}
				break;

			case ULT_TAG_UTF8:
					// Cerco un Tag con codifica UTF8 forzata
				pszPoint=lpStart=strstr(pszPoint,UTF8_TAG);
				if (lpStart) 
				{
					iItemType=ULT_TYPE_HTML; 
					lpStart+=4; 
					lpEnd=strstr(lpStart,")#"); 
				}
				break;


			// Cerco un Tag con codifica HTMLUTF8 
			case ULT_TAG_HTMLUTF8:
				pszPoint=lpStart=strstr(pszPoint,HTMLUTF8_TAG);
				if (lpStart) 
				{
					iItemType=ULT_TYPE_HTML; 
					lpStart+=5; 
					lpEnd=strstr(lpStart,")#"); 
				}
				break;
			
			// Cerco un Tag con codifica Witango
			case ULT_TAG_WITANGO:
				pszPoint=lpStart=strstr(lpMemo,WITANGO_TAG); 
				if (lpStart) 
				{
					iItemType=ULT_TYPE_HTML; 
					lpStart+=4; 
					lpEnd=strstr(lpStart,")#"); 
				}
				break;

		// Cerco un Tag con codifica Java script
			case ULT_TAG_JAVA:
				pszPoint=lpStart=strstr(lpMemo,JAVA_TAG); 
				if (lpStart) 
				{
					 iItemType=ULT_TYPE_HTML; 
					lpStart+=4; 
					lpEnd=strstr(lpStart,")#"); 
				}
				break;

			default: 
				break;

		}
	
		// Salto ad altro tag
		if (!lpStart||!lpEnd) {
			enTagType++;
			if (enTagType==ULT_TAG_END) break;
			pszPoint=lpMemo;
			continue;
		}
		//
		// Trovato Tag
		//
		iTags++; // Trovato Tag

		// Post-controlli sul tag
		if (!lpEnd) 
		{
			ehLogWrite("#Errore in file %s\n%30.30s...\n(%d)",lpFileSourceLong,lpStart,iLen);
			//
			// scrivo un log on fly da visualizzare o spedire via email
			//
			_ultLogWrite(psUlt,"#Errore in file %s\n%30.30s...\n(%d)",lpFileSourceLong,lpStart,iLen);
			psUlt->fLogError=TRUE;
			goto FINE;
		}
		charBackup=*lpEnd;
		*lpEnd=0; iLen=(INT) lpEnd - (INT) lpStart; 
		if (iLen>MAX_TAG_SIZE) 
		{
			ehLogWrite(
					"Tag Mark errato, Tipo %d, file:%s - Cercare: %80.80s ...\n (%d)",
					enTagType,
					lpFileSourceLong,
					lpStart,
					iLen);
			//
			// scrivo un log on fly da visualizzare o spedire via email
			//
			_ultLogWrite(psUlt,"Tag Mark errato, Tipo %d, file:%s - Cercare: %80.80s ...\n (%d)",
					enTagType,
					lpFileSourceLong,
					lpStart,
					iLen);
			psUlt->fLogError=TRUE;
			goto FINE;
		}
	
		// Ricerco se all'interno ho un apertura
		if (strstr(lpStart,"#[")||strstr(lpStart,"#("))
		{
			ehLogWrite("Tag Mark errato, Tipo %d, file:%s - Cercare: %80.80s ...\n (%d)",
					enTagType,
					lpFileSourceLong,
					lpStart,
					iLen);
			//
			// scrivo un log on fly da visualizzare o spedire via email
			//
			_ultLogWrite(
					psUlt,
					"Tag Mark errato, Tipo %d, file:%s - Cercare: %80.80s ...\n (%d)",
					enTagType,
					lpFileSourceLong,
					lpStart,
					iLen);
			psUlt->fLogError=TRUE;
			goto FINE;
		}


		pwcFind=strToWcs(lpStart); // Converto in UniCode Latin1 To Unicode
		pcwWordTrans=ultTranslateEx(psUlt,iItemType,pwcFind,NULL,FALSE,&iRow); // Traduco
		if (!pcwWordTrans) 
		{
			ehExit("Traduzione: %S ? ",pwcFind);
		}
		iCountSwap++; if (iCountSwap>10000) ehExit("Over load");

		pwcEncode=NULL;
		switch (enTagType)
		{
			//
			// Conversione HTMLS (default)
			// La parola viene convertita seguendo una codifica relativa al CharSet
			// in caso di LATIN1 i caratteri <>& e CR non vengono converiti
			//
			case ULT_TAG_STD: 

				sprintf(lpTagFind,"#[%s]#",lpStart); *lpEnd=charBackup;
				enOutTarget=enOutTargetStandard; // OUT_TEXT_HTML
				break;

			//
			// Conversione HTML
			// La parola viene convertita seguendo una codifica relativa al CharSet
			// in caso di LATIN1 vengono convertiti tutti i caratteri
			//
			case ULT_TAG_HTMLP: 
				sprintf(lpTagFind,HTMLPURE_TAG "%s)#",lpStart); *lpEnd=charBackup;
				enOutTarget=OUT_WEB;
				break;

			//
			// Conversione UTF8 (forzata)
			// La parola viene convertita seguendo una codifica relativa al CharSet
			// 
			//
			case ULT_TAG_UTF8: 
				sprintf(lpTagFind,UTF8_TAG "%s)#",lpStart); *lpEnd=charBackup;
				enOutTarget=OUT_WEB_HTML; // Non ne sono sicuro
				enOutCharEncoding=SE_UTF8;
				break;

			//
			// Conversione HTMLUTF8 (forzata)
			// La parola viene convertita seguendo una codifica relativa al CharSet
			//
			case ULT_TAG_HTMLUTF8: 
				sprintf(lpTagFind,HTMLUTF8_TAG "%s)#",lpStart); *lpEnd=charBackup;
				enOutTarget=OUT_TEXT;
				enOutCharEncoding=SE_UTF8;
				break; 

			//
			// Conversione Witango
			// Gli accenti vengono convertiti per non avere problemi in Witango
			// 
			case ULT_TAG_WITANGO: 
				sprintf(lpTagFind,WITANGO_TAG "%s)#",lpStart); *lpEnd=charBackup;
				enOutTarget=OUT_WITANGO;
				break;

			//
			// Conversione Java
			// Gli accenti vengono convertiti per non avere problemi in Witango
			// 
			case ULT_TAG_JAVA: 
				sprintf(lpTagFind,JAVA_TAG "%s)#",lpStart); *lpEnd=charBackup;
				enOutTarget=OUT_JS;
				break;
		}

		//
		//	Encoding dell'uscita 
		//
		lpCharText=NULL;

		switch (enOutTarget) {
		
				case OUT_TEXT:			// Nessun encoding aggiuntivo		: se UTF8 diretto, altrimenti Latin1 solo superiore al 127

					switch (enOutCharEncoding) {
					
						case SE_UTF8: lpCharText=strEncodeW(pcwWordTrans,SE_UTF8,NULL); break;
						case SE_HTML: lpCharText=strEncodeW(pcwWordTrans,SE_HTMLS,NULL); break;
						default:ehError();

					}
					break;

				case OUT_WEB:			// Testo in una pagina web			: Per renderlo visibile tutto viene convertito in encoding HTML

					switch (enOutCharEncoding) {
					
						case SE_UTF8: lpCharText=strEncodeEx(2,pcwWordTrans,2,SE_UTF8,SE_HTML_XML); break;
						case SE_HTML: lpCharText=strEncodeW(pcwWordTrans,SE_HTML,NULL); break;
						default:ehError();

					}
					break;				
				
				case OUT_WEB_HTML:		// Testo in una pagina web + html	: Il testo può contenere tag html, quindi vengono preservati i tag e convertiti solo i caratteri superiori al ASCII
					
					switch (enOutCharEncoding) {
					
						case SE_UTF8: lpCharText=strEncodeW(pcwWordTrans,SE_UTF8,NULL); break;
						case SE_HTML: lpCharText=strEncodeW(pcwWordTrans,SE_HTMLS,NULL); break;
						default:ehError();

					}
					break;				
				
				case OUT_XML:			// Testo in un XML					: Vengono convertiti i tag <> 
					
					switch (enOutCharEncoding) {
					
						case SE_UTF8: lpCharText=strEncodeEx(2,pcwWordTrans,2,SE_UTF8,SE_HTML_XML); break;
						case SE_HTML: lpCharText=strEncodeW(pcwWordTrans,SE_HTML_XML,NULL); break;
						default:ehError();

					}
					break;

				case OUT_JS:			// Stringa Javascript				: Il testo viene codificato usando \(qualcosa)
					
					pwcEncode=_witangoDecoding(pcwWordTrans); // ??
					
					switch (enOutCharEncoding) {
					
						case SE_UTF8: 
							lpCharText=strEncodeEx(2,pwcEncode,2,SE_UTF8,SE_JSON); 
							break; // Converte in \ \,'," e i valori inferiori a 32
						case SE_HTML: lpCharText=strEncodeW(pwcEncode,SE_JSON,NULL); break;
						default:ehError();

					}
					break;

				case OUT_WITANGO:		// Stringa Witango : Il testo viene codificato usando \(qualcosa)

					pwcEncode=_witangoEncoding(pcwWordTrans);
					
					switch (enOutCharEncoding) {
					
						case SE_UTF8: lpCharText=strEncodeW(pwcEncode,SE_UTF8,NULL); break;
						case SE_HTML: lpCharText=strEncodeW(pwcEncode,SE_HTMLS,NULL); break;
						default:ehError();

					}
					break;

				default:
					ehError(); // Tipo di out non definito
					break;

		}

		// Trovo e sostituisco
		while (strReplace(pszPoint,lpTagFind,lpCharText));
		//memReplace(lpMemo,pszPoint,lpTagFind,lpCharText,iFileSize);

		ehFreePtrs(3,&pwcEncode,&lpCharText,&pwcFind);

	}
	// printf("[c] %s = %d tags" CRLF,chronoFormat(chronoGet(NULL),1),iTags);

	//
	// Conversione automatica del TAG LANG
	//
	while (strReplace(lpMemo,"#LANG#",lpIsoLang));
	strcpy(szIsoLower,lpIsoLang); _strlwr(szIsoLower);
	while (strReplace(lpMemo,"#lang#",szIsoLower));

	//
	// Salvo il file
	//

	//
	// a) Trovare il nome definitivo del file
	//

	//
	// b) Creo le cartelle intermedie se non le ho
	//

	sprintf(szFileTarget,"%s\\%s",lpFolderLang,lpFileSourceShort);
	dirCreateFromFile(szFileTarget);
	
	// ehLogWrite("%s",szFileTarget);
	//ch=fopen(szNewName,"wb"); if (!ch) ehExit("_ultFileBuilder: Write [%s] err:%d",szNewName,osGetError());
	// if (bFileTarget) fileStrWrite("c:\\COMFerra\\ULT\\LastFile.log",lpMemo);

	iCount=0;
	while (TRUE)
	{
	 //ch=fopen(szNewName,"wb"); 
	  hdlFile= CreateFile((LPCTSTR) szFileTarget,
						   GENERIC_WRITE,
						   FILE_SHARE_READ|FILE_SHARE_WRITE,//FILE_SHARE_READ,
						   NULL,//&sSA,
						   CREATE_ALWAYS,
						   FILE_FLAG_SEQUENTIAL_SCAN,
						   (HANDLE)NULL);
 
	 if (hdlFile==(HANDLE) INVALID_HANDLE_VALUE) 
	 {
		 iCount++; if (iCount<8) {Sleep(1000); continue;}
		 if (fShowFileTouch) 
		 {
			ehPrintf("?File_save_error ? :[%s]\n",szFileTarget);
			goto FINE;
		 }
		 else
		 {
			//ehExit("#_ultFileBuilder: Write [%s] err:%d",szFileTarget,osGetError());
			 _ultLogWrite(psUlt,"#_ultFileBuilder: Write [%s] err:%d",szFileTarget,osGetError());
			 psUlt->fLogError=TRUE;
			 goto FINE;
		 }
	 }
	 break;
	}

//	fwrite(lpMemo,strlen(lpMemo),1,ch);
	WriteFile(
		hdlFile,                    // handle to file to write to
		lpMemo,                // pointer to data to write to file
		strlen(lpMemo),     // number of bytes to write
		&dwBytesWritten,  // pointer to number of bytes written
		NULL        // pointer to structure for overlapped I/O
		);
	
	if (dwBytesWritten!=strlen(lpMemo)) {win_infoarg("Errore in scrittura caratteri"); ehLogWrite("Errore in scrittura caratteri");}
	CloseHandle(hdlFile);//fclose(ch);
	if (fShowFileTouch) 
	{
		ehPrintf("!%s > OK %s\n",szFileTarget,chronoFormat(clock()-start,4));
	}

FINE:
	if (hdlMemo) memoFree(hdlMemo,"File");
	if (lpTagFind) ehFree(lpTagFind);
	return FALSE;
}

// 
// ultMultiFileBuilder()
// 
BOOL ultMultiFileBuilder(EH_ULT	*	psUlt,	
						 CHAR *		lpFolderBase, // Il folder dove si trovano le cartelle in lingua Es. c:\inetpub\wwwroot\mondialbroker
						 CHAR *		lpIsoLang,	 // Prefisso della lingua interessata
						 WCHAR *	pwcFileList, // Elenco dei file da rielaborare separati da virgola
						 INT *		lpiFileMissing,
						 BOOL		fShowFilesTouch)	// Numero dei File scomparsi 
{
	CHAR szFileSource[500];
	CHAR szFolderDest[500];
	CHAR *lp;
//	INT idxLang;
	CHAR *lpFileList;
	INT iErr;
	INT iMissing=*lpiFileMissing;

	sprintf(szFolderDest,"%sULT",lpFolderBase);	
	if (!fileCheck(szFolderDest)) return TRUE;
	lpFileList=wcsToStr(pwcFileList); // Lo faro più avanti in Wchar

	// Backup
//	idxLang=ultLangReady(psUlt,0,lpIsoLang);
//	ehLogWrite("%d:[%s]",idxLang,lpIsoLang);
	ultSetLangText(psUlt,lpIsoLang,"");
	// Setto la lingua che voglio tradurre e l'alterniva uso quella di default
//	ultSetLang(psUlt,idxLang,-1); // Uso la lingua di default alternativa

	sprintf(szFolderDest,"%s%s",lpFolderBase,lpIsoLang);
	while (TRUE)
	{
		lp=strstr(lpFileList,","); if (lp) *lp=0;

		sprintf(szFileSource,"%sULT\\%s",lpFolderBase,lpFileList);
		
#ifdef EH_CONSOLE
//		ehPrintf("[%s] > [%s] <BR>",szFileSource,szFolderDest); fflush(stdout);
#endif
		iErr=_ultFileBuilder(	psUlt,
								szFileSource,
								lpFileList,
								szFolderDest,							
								TRUE,
								fShowFilesTouch); if (iErr==-1) iMissing++;

#ifdef EH_CONSOLE
//		ehPrintf("-- DOPO<BR>"); fflush(stdout);
#endif
		if (lp) {*lp=','; lpFileList=lp+1;} else break;
	}

	*lpiFileMissing=iMissing;
	// Restore
	//memcpy(&UltInfo,&sBackup,sizeof(UltInfo));
	ehFree(lpFileList);
	return FALSE;

}

//
// ultDwToRow()
//
INT ultDwToRow(EH_ULT * psUlt,DWORD dwId)
{
	INT x;
	ULTVOICE sUltVoice;
	for (x=0;x<psUlt->dmiDict.Num;x++)
	{
		DMIRead(&psUlt->dmiDict,x,&sUltVoice);
		if (sUltVoice.dwID==dwId) return x;
	}
	return -1;
}

// Recodifica
// ritorna FALSE se tutto OK
/*
BOOL ULTDictRencoding(INT toEncode)
{
	ULTVOICE sUltVoice;
	INT x,c;
	INT iLen;
	WCHAR *lpwBuf,*lpw;
	if (psUlt->iCS_Dictionary==toEncode) return TRUE;

	for (x=0;x<psUlt->dmiDict.Num;x++)
	{
		DMIRead(&psUlt->dmiDict,x,&sUltVoice);
		//
		// Non codifico la prima lingua, perchè è il codice di ricerca
		// Ovvero dovrebbe già in qualche modo esser codificata
		// o ULTEdit dovrebbe contenere il coding del CODICE ID Sorgente
		//
		 for (c=1;c<psUlt->iLangNum;c++)
		 {
			if (sUltVoice.lpcText[c])
			{
			//	win_infoarg("%d/%d qui [%s]",x,c,sUltVoice.lpLang[c]);
				iLen=strlen(sUltVoice.lpLang[c]);
				lpwBuf=ehAlloc((iLen*3)+3);

				// Converto dal encoding attuale in widechar
				switch (psUlt->iCS_Dictionary)
				{
					case ULT_CS_LATIN1:
						CharToUnicode(lpwBuf,sUltVoice.lpLang[c]);
						break;

					case ULT_CS_UTF8:
						lpw=StrDecoding(sUltVoice.lpLang[c],SE_UTF8);
						wcscpy(lpwBuf,lpw); ehFree(lpw);
						break;
					
					default:
						ehExit("ULTRecoding(%d): Encoding ?",psUlt->iCS_Dictionary);
						break;
				}

				// Converto da WIDRCHAR in nuovo encoding
				ehFree(sUltVoice.lpLang[c]); sUltVoice.lpLang[c]=NULL;

				switch (toEncode)
				{
					case ULT_CS_LATIN1:
						sUltVoice.lpLang[c]=ehAlloc(wcslen(lpwBuf)+1);
						UnicodeToChar(sUltVoice.lpLang[c],lpwBuf);
						break;

					case ULT_CS_UTF8:
						// sUltVoice.lpLang[c]=ehAlloc((wcslen(lpwBuf)+3)+1);
						sUltVoice.lpLang[c]=StrEncodingW(lpwBuf,SE_UTF8);
						//win_infoarg("UTF8 [%s]",sUltVoice.lpLang[c]);
						// wcscpy(lpwBuf,lpw); ehFree(lpw);
						break;
					
					default:
						ehExit("ULTRecoding(%d): Encoding ?",toEncode);
						break;
				}

				ehFree(lpwBuf);
			}
			DMIWrite(&psUlt->dmiDict,x,&sUltVoice);
		 }		
	}
	return FALSE;
}

  */

//
// _ultTextCharBuilder()
//
BYTE * _ultTextCharBuilder(EH_ULT * psUlt,WCHAR *lpPhrase,INT iLang)
{
	BYTE *lpRet=NULL,*lpc;
	WCHAR *lpw;

	// Se è il Codice sorgente
	if (iLang==0)//psUlt->iSourceLang)
		{
			switch (psUlt->iCS_Source)
			{
				case ULT_CS_LATIN1:
							//lpRet=ehAlloc(wcslen(lpPhrase)+1);
							lpRet=wcsToStr(lpPhrase);
//							UnicodeToChar(lpRet,lpPhrase);
							break;

				case ULT_CS_UTF8:
							lpRet=ehAlloc(wcslen(lpPhrase)+1);
							lpw=lpPhrase;
							for (lpc=lpRet;*lpw;lpw++,lpc++) *lpc=(BYTE) *lpw;
							*lpc=0;
							break;
			}
		}
	// Se è il Dizionario
		else
		{
			switch (psUlt->iCS_Source)
			{
				case ULT_CS_LATIN1:
							//lpRet=ehAlloc(wcslen(lpPhrase)+1); UnicodeToChar(lpRet,lpPhrase);
							lpRet=wcsToStr(lpPhrase);
							break;

				case ULT_CS_UTF8: // Encoding sul dizionario
							lpRet=(BYTE *) strEncodeW(lpPhrase,SE_UTF8,NULL);
							break;
			}
		}
	return lpRet;
}		

// --------------------------------------------------
// Aggiunge una lingua all'arLangReady
// Sposta il "files" dopo
// ritorna:
// -1 = Lingua già presente
// -2 = Lingua assente
// -3 = Raggiunto il numero massimo di lingue gestite
// --------------------------------------------------
INT ultLangAdd(EH_ULT * psUlt,CHAR *lpIsoPrefix)
{
	EH_ULT_LANG *lpUI;
	INT a;

	for (a=0;a<psUlt->iLangNum;a++) // psUlt->arLangReady[a].lpIsoPrefix
	{
		if (!strcmp(psUlt->arLangReady[a].lpIsoPrefix,lpIsoPrefix)) return -1;
	}
	lpUI=ultLangInfo(psUlt,0,lpIsoPrefix); if (!lpUI) return -2;
	
	if (psUlt->iLangNum>=ULT_MAXLANG) return -3;
	memcpy(psUlt->arLangReady+psUlt->iLangNum,lpUI,sizeof(EH_ULT_LANG));
	psUlt->iLangNum++;

	_ultOrganizer(psUlt);
	return 0;
}

// --------------------------------------------------
// Rimuove una lingua all'arLangReady
// Sposta il "files" dopo
// ritorna:
// -1 = Lingua non presente in arLangReady
// -2 = Lingua nativa non cancellabile
// -3 = Lingua codice non cancellabile
// --------------------------------------------------

//
// ultLangRemove
//
INT ultLangRemove(EH_ULT * psUlt,CHAR *lpIsoPrefix)
{
//	EH_ULT_LANG *lpUI;
	INT idxSource;
	INT a,iChunkSize;

	idxSource=ultLangReady(psUlt,0,lpIsoPrefix); if (idxSource<0) return -1;
	if (idxSource==psUlt->idxLangNative) return -2;
	if (idxSource==psUlt->idxLangCode) return -3;
	iChunkSize=(psUlt->iLangNum-idxSource-1);

	// 012345
	//    2345 

	// Riorganizzo arLangReady
	memmove(&psUlt->arLangReady[idxSource], // Destinazione
			&psUlt->arLangReady[idxSource+1],
			sizeof(EH_ULT_LANG)*iChunkSize);
	memset(&psUlt->arLangReady[psUlt->iLangNum-1],0,sizeof(EH_ULT_LANG));

	for (a=0;a<psUlt->dmiDict.Num;a++)
	{
		DMIRead(&psUlt->dmiDict,a,psUlt->lpVoiceShare);
/*
		if (psUlt->lpVoiceShare->lpcText[idxSource]) ehFree(psUlt->lpVoiceShare->lpcText[idxSource]);
		memmove(psUlt->lpVoiceShare->lpcText+idxSource,psUlt->lpVoiceShare->lpcText+idxSource+1,sizeof(BYTE *)*iChunkSize);
		psUlt->lpVoiceShare->lpcText[psUlt->iLangNum-1]=NULL;
*/
		if (psUlt->lpVoiceShare->lpwText[idxSource]) ehFree(psUlt->lpVoiceShare->lpwText[idxSource]);
		memmove(psUlt->lpVoiceShare->lpwText+idxSource,psUlt->lpVoiceShare->lpwText+idxSource+1,sizeof(BYTE *)*iChunkSize);
		psUlt->lpVoiceShare->lpwText[psUlt->iLangNum-1]=NULL;

		DMIWrite(&psUlt->dmiDict,a,psUlt->lpVoiceShare);
	}
	psUlt->iLangNum--;
	_ultIdxFind(psUlt);
	_ultOrganizer(psUlt);
	return 0;
}



//
// _ultOrganizer
//
// --------------------------------------------------
// Sposta la lingua "virtual" files per ultima
// Sposta la lingua nativa per seconda
// --------------------------------------------------
void _ultOrganizer(EH_ULT * psUlt)
{
	INT idSource,idTarget;
	EH_ULT_LANG sFile;
	ULTVOICE sUltVoice;
	INT a;
	WCHAR *lpwText;
	INT iChunkSize;
	
	idSource=psUlt->idxFiles;
	idTarget=(psUlt->iLangNum-1);
	iChunkSize=(psUlt->iLangNum-idSource-1);

	if (idSource<0||idSource==idTarget) goto AVANTI; // E' già l'ultimo o non è presente
	// Riorganizzo arLangReady
	memcpy(&sFile,&psUlt->arLangReady[idSource],sizeof(EH_ULT_LANG));
	memmove(&psUlt->arLangReady[idSource], // Destinazione
			&psUlt->arLangReady[idSource+1],
			sizeof(EH_ULT_LANG)*iChunkSize);
	memcpy(&psUlt->arLangReady[idTarget],&sFile,sizeof(EH_ULT_LANG));

	for (a=0;a<psUlt->dmiDict.Num;a++)
	{
		DMIRead(&psUlt->dmiDict,a,&sUltVoice);

/*		
		lpcText=sUltVoice.lpcText[idSource];
		memmove(sUltVoice.lpcText+idSource,sUltVoice.lpcText+idSource+1,sizeof(BYTE *)*iChunkSize);
		sUltVoice.lpcText[idTarget]=lpcText;
*/

		lpwText=sUltVoice.lpwText[idSource];
		memmove(sUltVoice.lpwText+idSource,sUltVoice.lpwText+idSource+1,sizeof(BYTE *)*iChunkSize);
		sUltVoice.lpwText[idTarget]=lpwText;

		DMIWrite(&psUlt->dmiDict,a,&sUltVoice);
	}
	_ultIdxFind(psUlt);

AVANTI:

	if (psUlt->idxLangCode!=0) ehExit("psUlt->idxLangCode non è 0!");
	idSource=psUlt->idxLangNative;
	idTarget=1; // 0 = è il codice quindi quello dopo il codice
	iChunkSize=(idSource-idTarget);

	// 01234567
	// #	  .
	//       X
	//   x
	//   2345
	//   62345

	if (idSource==idTarget) goto AVANTI2; // E' già la seconda

	// Riorganizzo arLangReady
	memcpy(&sFile,&psUlt->arLangReady[idSource],sizeof(EH_ULT_LANG));
	memmove(&psUlt->arLangReady[idTarget+1], // Destinazione
			&psUlt->arLangReady[idTarget],
			sizeof(EH_ULT_LANG)*iChunkSize);
	memcpy(&psUlt->arLangReady[idTarget],&sFile,sizeof(EH_ULT_LANG));

	for (a=0;a<psUlt->dmiDict.Num;a++)
	{
		DMIRead(&psUlt->dmiDict,a,&sUltVoice);

/*		
		lpcText=sUltVoice.lpcText[idSource];
		memmove(sUltVoice.lpcText+idTarget+1,sUltVoice.lpcText+idTarget,sizeof(BYTE *)*iChunkSize);
		sUltVoice.lpcText[idTarget]=lpcText;
*/

		lpwText=sUltVoice.lpwText[idSource];
		memmove(sUltVoice.lpwText+idTarget+1,sUltVoice.lpwText+idTarget,sizeof(BYTE *)*iChunkSize);
		sUltVoice.lpwText[idTarget]=lpwText;
		DMIWrite(&psUlt->dmiDict,a,&sUltVoice);
	}
	_ultIdxFind(psUlt);

AVANTI2:

	for (a=0;a<psUlt->dmiDict.Num;a++)
	{
		DMIRead(&psUlt->dmiDict,a,psUlt->lpVoiceShare);
		ultItemSetFlag(psUlt);
		DMIWrite(&psUlt->dmiDict,a,psUlt->lpVoiceShare);
	}
}

//
// _ultIdxFind
//
void _ultIdxFind(EH_ULT * psUlt)
{
	// Cerco gli indici in array arLangReady[] 
	psUlt->idxLangCode=ultLangReady(psUlt,0x10000,NULL);
	psUlt->idxLangNative=ultLangReady(psUlt,psUlt->iLangNative,NULL);
	if (psUlt->iLangTransAlternative>-1) 
		psUlt->idxLangTransAlternative=ultLangReady(psUlt,psUlt->iLangTransAlternative,NULL);
		else
		{
		 psUlt->iLangTransAlternative=psUlt->iLangNative;
		 psUlt->idxLangTransAlternative=psUlt->idxLangNative;
		}

	psUlt->idxFiles=ultLangReady(psUlt,0xFFFF,NULL);
	psUlt->idxNote=ultLangReady(psUlt,0x10001,NULL);
	//psUlt->idxAlfaCode=ultLangReady(0x10002,NULL);
}

// -----------------------------------------------------
//
// _ultFileRemove
//
// Rimuove in file dal tutto il dizionario
// Il file non esiste più
// -----------------------------------------------------
INT _ultFileRemove(EH_ULT * psUlt,CHAR *lpFileName)
{
	INT x;
	WCHAR *pwcFile;
	WCHAR *pwcFind;
	WCHAR *pwcList;
	WCHAR *pw;
	INT iSize,iCount=0;
	ULTVOICE sVoice;

	if (!*lpFileName) ehExit("_ultFileRemove(): File name non indicato");

//	ehPrintf("Rimuovi %s<BR>",lpFileName); fflush(stdout);
	pwcFile=strToWcs(lpFileName);
	iSize=wcslen(pwcFile)*2+10;	pwcFind=ehAlloc(iSize); 	
	_snwprintf(pwcFind,iSize,L",%s,",pwcFile); //wcslwr(pwcFind);

	iSize=8000; pwcList=ehAlloc(iSize);
	for (x=0;x<psUlt->dmiDict.Num;x++)
	{
		DMIRead(&psUlt->dmiDict,x,&sVoice);
		if (!sVoice.lpwText[psUlt->idxFiles]) continue;
		_snwprintf(pwcList,iSize,L",%s,",sVoice.lpwText[psUlt->idxFiles]); 
//		ehPrintf("%d - %ls",x,pwcList); fflush(stdout);
		if (wcsstr(pwcList,pwcFind)) // Trovo
		{
			while (wcsReplace(pwcList,pwcFind,L","))
			pwcList[wcslen(pwcList)-1]=0;
			pw=pwcList; 
			if (*pw) pw++;
			if (!*pw) pw=NULL;
//			ehPrintf("=<B>%d,%d|%ls|</B>",x,psUlt->idxFiles,pw); fflush(stdout);
			ultWordUpdate(psUlt,x,psUlt->idxFiles,pw,2,UU_SOURCE);
			iCount++;
		}
//		ehPrintf("<BR>"); fflush(stdout);
	}

	ehFree(pwcFile);
	ehFree(pwcFind);
	ehFree(pwcList);
//	ehPrintf("Fine remove in %d items<BR>",iCount);
//	ehExit("xx");
	return iCount;
}

//
// ultIdxIsLang
//
BOOL ultIdxIsLang(EH_ULT * psUlt,INT idx)
{
	if (idx<0||
		idx>=psUlt->iLangNum||
		idx==psUlt->idxLangCode||
		idx==psUlt->idxFiles||
		idx==psUlt->idxNote)//||idx==psUlt->idxAlfaCode) 
		return FALSE;
	return TRUE;
}

//
// _ultLogOpen()
//
static void _ultLogOpen(EH_ULT * psUlt)
{
	psUlt->chLog=fopen(psUlt->szLogFile,"ab"); 
	if (!psUlt->chLog) psUlt->chLog=fopen(psUlt->szLogFile,"wb");
	if (!psUlt->chLog) ehExit("errore in apertura LOG %s",psUlt->szLogFile);
}

//
// _ultLogClose
//
static void _ultLogClose(EH_ULT * psUlt)
{
	if (psUlt->chLog!=NULL)
	{
		fclose(psUlt->chLog); psUlt->chLog=NULL;
		if (psUlt->fLogError)
		{
		 if (psUlt->fLogShowEnd) 
		 {
			ShellExecute(NULL,"open",psUlt->szLogFile,NULL,NULL,SW_NORMAL);
		 }
		}
		else
		{
			remove(psUlt->szLogFile);
		}
	}
}

//
// _ultLogWrite()
//
static void _ultLogWrite(EH_ULT * psUlt,CHAR *Mess,...)
{	
	va_list Ah;
	va_start(Ah,Mess);

	if (psUlt->chLog!=NULL)
	{
		struct tm when;   
		time_t now;   
   
		_tzset();
		time (&now); // Legge il Timer di sistema
		when = *localtime( &now ); // Converte il timer in Giorni e Ore
		fprintf(sys.chLog,"%02d/%02d/%04d %02d:%02d:%02d | ",
				when.tm_mday,
				when.tm_mon+1,
				when.tm_year+1900,
				
				when.tm_hour,
				when.tm_min,
				when.tm_sec);

		if (Mess!=NULL) vfprintf(psUlt->chLog,Mess,Ah);
		fprintf(psUlt->chLog,CRLF);
		fflush(psUlt->chLog);
	}
	va_end(Ah);
}


//
// _tokDecode()
//
// ------------------------------------------------------------
// Aggiunge e se è il caso ricrea una parola all'area statica
// testo delle parole, restituendo il puntatore all'area
// lpWord= Parola da aggiungere
// iNum  = Numero elemento da aggiornare
// ------------------------------------------------------------
static WCHAR * _tokDecode(EH_ULT * psUlt,void *lpTok)
{
	BYTE *lpWord;
	WCHAR *pwc;

	lpWord=_ultFileDecodingAlloc(lpTok,psUlt->iTypeEncDict);
	if (!lpWord) return NULL;

	switch (psUlt->iCS_Dictionary)
	{
	    default:
		case ULT_CS_LATIN1: 
			//ultAddWord(lpWord,idxLang,1); // Carico a 8 Bit CHAR
			pwc=strToWcs(lpWord);
			break;

		case ULT_CS_UTF8: 
			//if (!lpWord) {ultAddWord(NULL,idxLang,1); break;}
			pwc=strDecode(lpWord,SE_UTF8,NULL);
			//ultAddWord(lpw,idxLang,2); // Carico a 16 bit WIDECHAR
			//ehFree(lpw);
			break;
	}
	ehFree(lpWord);
	return pwc;
}

//
// _ultEnumType
//
static INT _ultEnumType(CHAR *lpType) {

	INT iType=-1;
	if (!strcmp(lpType,"WINT")) iType=ULT_TYPE_WINT; // Win Title
	if (!strcmp(lpType,"WINI")) iType=ULT_TYPE_WINI; // win_info,win_err,ecc...
	if (!strcmp(lpType,"OBJS")) iType=ULT_TYPE_OBJS; // O_TEXT negli oggetti statici
	if (!strcmp(lpType,"HMZ"))  iType=ULT_TYPE_HMZ;
	if (!strcmp(lpType,"OBJ"))  iType=ULT_TYPE_OBJ;
	if (!strcmp(lpType,"MENU")) iType=ULT_TYPE_MENU;
	if (!strcmp(lpType,"SPF"))  iType=ULT_TYPE_SPF;  // sprintf
	if (!strcmp(lpType,"DISP")) iType=ULT_TYPE_DISP;
	if (!strcmp(lpType,"LIST")) iType=ULT_TYPE_LIST;

	if (!strcmp(lpType,"HTML")) iType=ULT_TYPE_HTML;
	if (!strcmp(lpType,"JAVA")) iType=ULT_TYPE_JAVA;

//	if (!strcmp(lpType,"WTAG")) iType=ULT_TYPE_WTAG;
	if (!strcmp(lpType,"WTAG")) iType=ULT_TYPE_HTML;

	return iType;
}

// --------------------------------------------------
// _ultTypeRecovery()
//
// 
// --------------------------------------------------
static void _ultTypeRecovery(EH_ULT * psUlt) {

	INT a,iControl;
	ZeroFill(psUlt->arType);
	iControl=0;
	for (a=0;a<psUlt->dmiDict.Num;a++)
	{
		DMIRead(&psUlt->dmiDict,a,psUlt->lpVoiceShare);
		if (!psUlt->arType[psUlt->lpVoiceShare->iType].iNum) psUlt->arType[psUlt->lpVoiceShare->iType].iFirst=a;
		psUlt->arType[psUlt->lpVoiceShare->iType].iNum++;
		if (psUlt->lpVoiceShare->iType>iControl) iControl=psUlt->lpVoiceShare->iType;
		if (iControl<psUlt->lpVoiceShare->iType) ehExit("? Errore in ordinamento Tipi");
	}

}


// 
// ultFileDecodingAlloc()
//
// Decodifica in CHAR ANSI
// Liberare il ritorno con ehFree()
// 
static CHAR * _ultFileDecodingAlloc(CHAR *lpWord,INT iType)
{
	CHAR *lpBuf=NULL;
	CHAR *p;
	INT Num;

	if (!lpWord) return lpWord;
	lpBuf=ehAlloc(strlen(lpWord)+1);
	strcpy(lpBuf,lpWord);
	switch (iType)
	{
	case 0:
		while (TRUE)
		{
			p=strstr(lpBuf,"@<"); if (!p) break;
			*(p+4)=0; 
			Num=xtoi(p+2);
			*p=(CHAR) Num;
			strcpy(p+1,p+5);
		}
		break;

	case 1:
		while (strReplace(lpBuf,"\1","\r"));
		while (strReplace(lpBuf,"\2","\n"));
		break;
	}
	return lpBuf;
}

// 
// _ultFileEncodingAlloc()
//
// Codifica da CHAR ANSI
// Liberare il ritorno con ehFree()
//
static CHAR * _ultFileEncodingAlloc(CHAR *lpWord,INT iType)
{
	BYTE *p,*lpBuf=NULL;
	CHAR Car[10];
	INT iSize=strlen(lpWord);

	if (iSize)
	{
		 switch (iType)
		 {
			case 0:
				iSize*=5;
				lpBuf=ehAlloc(iSize); if (!lpBuf) ehExit("ULTEncoding:No memory [%d]",iSize);
				*lpBuf=0; 
				for (p=lpWord;*p;p++)
				{
					//if ((UINT) (*p)>31) {Car[0]=*p; Car[1]=0;} else{sprintf(Car,"@<%02x>",(INT) *p);}
					if (*p==13||*p==10||*p==92||*p==93) sprintf(Car,"@<%02x>",(INT) *p); else {Car[0]=*p; Car[1]=0;} 
					strcat(lpBuf,Car);
				}
				break;

			case 1:
				lpBuf=ehAlloc(iSize+10); if (!lpBuf) ehExit("ULTEncoding:No memory [%d]",iSize);
				strcpy(lpBuf,lpWord);
				while (strReplace(lpBuf,"\r","\1"));
				while (strReplace(lpBuf,"\n","\2"));
				break;
		 } 
	}
	return lpBuf;
}



//
// _tokExtract() - Aggiungo e ordino la voce nel dizionario
//
static CHAR * _tokExtract(EH_ULT * psUlt,CHAR ** lpCursor)
{
  CHAR *p=*lpCursor;
  CHAR *lpStart;//=(*lp)+2;
  CHAR *p2;

  if (p==NULL) return NULL;

  switch (psUlt->iTypeEncDict)
  {
	case 0: // Vecchio modo
		
		// Non esiste dichiarazione --
		if (!memcmp(p,"NULL",4)||*p=='n')
		{
			p2=strstr(p,",");
			if (p2) *lpCursor=(p2+1); else *lpCursor=NULL;
			return NULL;
		}

		lpStart=*lpCursor;
		if (memcmp(lpStart,"@[",2)) ehExit("ULT:Sintax %d\n%s",_local.iLine,lpStart);
		lpStart+=2;
		p=strstr(lpStart,"]@"); if (!p) ehExit("ULT:Sintax %d\n%s",_local.iLine,*lpCursor);

		// Trovo la prossima parola --
		if (*p) 
		{
			p2=strstr(p,","); 
			if (p2) *lpCursor=(p2+1); else *lpCursor=NULL;
		} else *lpCursor=NULL;
		*p=0;
		break;

	case 1: // Modo 1b
		if (*p=='\3') {*lpCursor=(p+1); lpStart=NULL; break;}
		lpStart=p; p2=strstr(p,"\3"); 
		if (p2) {*p2=0; *lpCursor=(p2+1);} else *lpCursor=NULL;
		break;
  }
  return lpStart;
}

//
// _ultWordBinFind
//
static BOOL _ultWordBinFind(EH_ULT * psUlt,INT iType,WCHAR *pwcWordCode,INT *lpIndex)
{
	WORD ct=0;
	INT psx,pdx,pt;
	WCHAR *pwcWordPoint;
	BOOL fFound=FALSE;
	INT iCount;
	int iCmp;

	iCount=psUlt->arType[iType].iNum;
	if (!iCount) {*lpIndex=psUlt->dmiDict.Num; return FALSE;} 
	
	psx=psUlt->arType[iType].iFirst;  // Primo puntatore
	pdx=psx+iCount-1; // UltimoPuntatore

	while (TRUE)
	{
		pt=psx+((pdx-psx)>>1);// Zona ceL_SetLangGooglele

		if (pt<psx) pt=psx;
		if (pt>pdx) pt=pdx;

		if (pt>psUlt->dmiDict.Num) ehExit("out of range");
		DMIReadEx(&psUlt->dmiDict,pt,psUlt->lpVoiceShare,"_ultWordBinFind");
		pwcWordPoint=psUlt->lpVoiceShare->lpwText[psUlt->idxLangCode];// psUlt->iLangNative
		if (pwcWordPoint==NULL) ehExit("BinFind:NULL %d:%d:%d",pt,psUlt->dmiDict.Num,psUlt->lpVoiceShare->lpwText[1]);

		iCmp=CompareStringW(LOCALE_SYSTEM_DEFAULT,
						    NORM_IGNORECASE,  // comparison-style options
							pwcWordPoint, // pointer to first string
							wcslen(pwcWordPoint),     // size, in bytes or characters, of first string
							pwcWordCode, // pointer to second string
							wcslen(pwcWordCode)      // size, in bytes or characters, of second string
							);
		// Trovata una eguaglianza: Ricerco la eguaglianza case sensible
		if (iCmp==CSTR_EQUAL&&!wcscmp(pwcWordPoint,pwcWordCode)) {fFound=TRUE; break;}

		if (iCmp==CSTR_GREATER_THAN||
			iCmp==CSTR_EQUAL&&wcscmp(pwcWordPoint,pwcWordCode)>0) // >0



/*
		iCmp=wcscmp(pwcWordPoint,pwcWordCode);
		if (!iCmp) {fFound=TRUE; break;}
		if (iCmp>0)
*/

			 {//		Stringa più grande
			  if (psx==pt) {break;} // Fine archivio
			  pdx=pt-1; //if (pdx<psx) {pt=psx+1;break;}
			 }
			 else
			 {//		Stringa più piccola
			  if (pdx==pt) {pt++;break;} // Fine archivio
			  psx=pt+1; //if (psx>pdx) {pt=pdx;break;}
			 }
		// *lpWordPoint=c1;
	 if (ct++>iCount) {ehExit("Errore in _ultWordBinFind");}
	}
	*lpIndex=pt;
//	ehFree(pwcWordCode);
	return fFound;
}

//
//
//
static BOOL _getTagFileInfo(CHAR * pszFileMemo,S_TAGFILEINFO * psFileInfo,BOOL bReplace) {

	CHAR * psz;
	BOOL bReady=false;

	memset(psFileInfo,0,sizeof(S_TAGFILEINFO));

	psz=strExtract(pszFileMemo,"#{","}#",false,true);
	if (psz) {

		EH_JSON * psJson;
		CHAR *p;
		psJson=jsonCreate(psz+1);

		// Setta il target standard di output
		p=jsonGet(psJson,"fileEncoding");
		if (p) {
			if (!strCaseCmp(p,"utf-8")||!strCaseCmp(p,"utf8")) psFileInfo->fileEncoding=SE_UTF8;
			if (!strCaseCmp(p,"latin1")) psFileInfo->fileEncoding=SD_ISO_LATIN1;
		}

		// Setta il target standard di output
		p=jsonGet(psJson,"outTarget");
		if (!strCmp(p,"web")) psFileInfo->enOutTarget=OUT_WEB;
		if (!strCmp(p,"webhtml")) psFileInfo->enOutTarget=OUT_WEB_HTML;
		if (!strCmp(p,"xml")) psFileInfo->enOutTarget=OUT_XML;
	
		jsonDestroy(psJson);
		if (bReplace) strReplace(pszFileMemo,psz,"");
		ehFree(psz);
		bReady=true;
	}

	return bReady;
}
