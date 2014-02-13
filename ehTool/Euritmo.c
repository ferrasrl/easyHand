// +-----------------------------------------------------------------------
//	Euritmo
//
//	Ferrà srl 2013
// +-----------------------------------------------------------------------

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/inc/Euritmo.h"


//
// arsBGM
//
EUR_FIELD arsBGM[]= {
	{"ANPROD"		,	0,174,NULL,true,"Anagrafica prodotti"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	,3,NULL,true,"BGM"},
	{"ID-EDI-MITT"	,_ALFA	,35,NULL,true,NULL},
	{"ID-EDI-MITT-1",_ALFA	,4,NULL,false,NULL},
	{"ID-EDI-MITT-3",_ALFA	,14,NULL,false,NULL},

	{"ID-EDI-DEST"	,_ALFA	,35,NULL,true,NULL},
	{"ID-EDI-DEST-1",_ALFA	,4,NULL,false,NULL},
	{"ID-EDI-DEST-3",_ALFA	,14,NULL,false,NULL},

	{"TIPODOC"		,_ALFA	,6,NULL,true,NULL},
	{"NUMDOC"		,_ALFA	,35,NULL,true,NULL},
	{"DATADOC"		,_DATE	,8,NULL,true,NULL},
	{"ORADOC"		,_NUMBER,4,NULL,false,NULL},

	{"DATAVAL"		,_DATE	,8,NULL,false,NULL},
	{"ORAVAL"		,_NUMBER,4,NULL,false,NULL},
	{NULL}
};

//
// NAS - Informazioni sul fornitore (ANPROD)
//
EUR_FIELD arsNAS_A[]= {

	{"NAS-ANPROD"	,	0,219,NULL,true,"Informazioni sul fornitore"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	,3,NULL,true,"NAS"},
	{"CODFORN"		,_ALFA	,17,NULL,true,NULL},
	{"QCODFORN"		,_ALFA	,3,NULL,false,NULL},

	{"RAGSOCF"		,_ALFA	,70,NULL,false,NULL},
	{"INDIRF"		,_ALFA	,70,NULL,false,NULL}, 
	{"CITTAF"		,_ALFA	,35,NULL,false,NULL},
	{"PROVF"		,_ALFA	,9,NULL,false,NULL},
	{"CAPF"			,_ALFA	,9,NULL,false,NULL},
	{"NAZIOF"		,_ALFA	,3,NULL,false,NULL},
	{NULL}
};

//
// NAS - Informazioni sul fornitore (INVOIC)
//
EUR_FIELD arsNAS_I[]= {

	{"NAS-INVOIC"	,	0,676,NULL,true,"Informazioni sul fornitore"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"NAS"},
	{"CODFORN"		,_ALFA	,17,NULL,true,NULL},
	{"QCODFORN"		,_ALFA	, 3,NULL,true,NULL},

	{"RAGSOCF"		,_ALFA	,70,NULL,true,NULL},
	{"INDIRF"		,_ALFA	,70,NULL,true,NULL}, 
	{"CITTAF"		,_ALFA	,35,NULL,true,NULL},
	{"PROVF"		,_ALFA	, 9,NULL,true,NULL},
	{"CAPF"			,_ALFA	, 9,NULL,true,NULL},
	{"NAZIOF"		,_ALFA	, 3,NULL,false,NULL},
	{"PIVANAZF"		,_ALFA , 35,NULL,true,NULL},
	{"TRIBUNALE"	,_ALFA , 35,NULL,false,NULL},
	{"LICIMPEXP"	,_ALFA , 35,NULL,false,NULL},
	{"CCIAA"		,_ALFA , 35,NULL,true,NULL},
	{"CAPSOC"		,_ALFA , 35,NULL,false,NULL},
	{"CODFISC"		,_ALFA , 35,NULL,false,NULL},
	{"PIVAINT"		,_ALFA , 35,NULL,false,NULL},
	{"TELEFONO"		,_ALFA , 25,NULL,false,NULL},
	{"TELEFAX"		,_ALFA , 25,NULL,false,NULL},
	{"TELEX"		,_ALFA , 25,NULL,false,NULL},
	{"EMAIL"		,_ALFA , 70,NULL,false,NULL},
	{"NUREGCOOPF"	,_ALFA , 35,NULL,false,NULL},
	{"NUREGRAEE"	,_ALFA , 16,NULL,false,NULL},
	{"NUREGPILE"	,_ALFA , 16,NULL,false,NULL},

	{NULL}
};

//
// LIA - Informazioni sugli articoli
//
EUR_FIELD arsLIA[]= {

	{"ANPROD-LIA"	,	0,630,NULL,true,"Informazioni sugli articoli"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	,3,NULL,true,"LIA"},
	{"CODCU"		,_ALFA	,35,NULL,false,NULL},
	{"TIPCODCU"		,_ALFA	,3,NULL,false,NULL},
	{"CODEANTU"		,_ALFA	,35,NULL,false,NULL},
	{"CODFORTU"		,_ALFA	,35,NULL,false,NULL},
	{"CODDISTU"		,_ALFA	,35,NULL,false,NULL},
	{"CODCAT"		,_ALFA	,20,NULL,false,NULL},

	{"DESCCAT"		,_ALFA	,35,NULL,false,NULL},
	{"DESCPROD"		,_ALFA	,35,NULL,true,NULL},
	{"NPZCART"		,_NUMBER,15,"12.3",true,NULL},
	{"NCTSTRATO"	,_NUMBER,15,"12.3",false,NULL},
	{"NCTPERBANC"	,_NUMBER,15,"12.3",false,NULL},
	{"QTAMINORD"	,_NUMBER,15,"12.3",true,NULL},
	{"UDMQTAMINORD"	,_ALFA	,3,NULL,true,NULL},
	{"MULTINCR"		,_NUMBER,5,NULL,true,NULL},
	{"PRZUNI1"		,_NUMBER,15,"12.3",true,NULL},
	{"TIPOPRZUNI1"	,_ALFA	,3,NULL,true,NULL},

	{"VLTPRZUNI1"	,_ALFA	,3,NULL,true,NULL},
	{"UDMPRZUN1"	,_ALFA	,3,NULL,true,NULL},
	{"PRZUNI2"		,_NUMBER,15,"12.3",false,NULL},
	{"TIPOPRZUNI2"	,_ALFA	,3,NULL,false,NULL},
	{"VLTPRZUNI2"	,_ALFA	,3,NULL,false,NULL},
	{"UDMPRZUN2"	,_ALFA	,3,NULL,false,NULL},

	{"DATAINORD"	,_DATE	,8,NULL,false,NULL},
	{"DATAFIORD"	,_DATE	,8,NULL,false,NULL},
	{"TIPOPROD"		,_ALFA	,1,NULL,true,NULL},
	{"PROMO"		,_ALFA	,1,NULL,false,NULL},
	{"CODCOM"		,_ALFA	,35,NULL,false,NULL},
	{"MARCAPROD"	,_ALFA	,35,NULL,false,NULL},
	{"ALIQIVA"		,_NUMBER,7,"3.4",false,NULL},
	{"CODESIVA"		,_ALFA	,3,NULL,false,NULL},
	{"LARGPROD"		,_NUMBER,15,"12.3",false,NULL},

	{"ALTEZPROD"	,_NUMBER,15,"12.3",false,NULL},
	{"PROFPROD"		,_NUMBER,15,"12.3",false,NULL},
	{"LARNPROD"		,_NUMBER,15,"12.3",false,NULL},
	{"ALTENPROD"	,_NUMBER,15,"12.3",false,NULL},
	{"PRONPROD"		,_NUMBER,15,"12.3",false,NULL},
	{"UMDIMENS"		,_ALFA	,3,NULL,false,NULL},

	{"PESOLPROD"	,_NUMBER,8,"5.3",false,NULL},
	{"PESONPROD"	,_NUMBER,8,"5.3",false,NULL},
	{"UMPESO"		,_ALFA	,3,NULL,false,NULL},

	{"NSOTIMB"		,_NUMBER,4,NULL,false,NULL},
	{"DATAINIDISP"	,_DATE,8,NULL,false,NULL},

	{"CODFORSOST"	,_ALFA	,35,NULL,false,NULL},
	{"STRATPALLET"	,_NUMBER,2,NULL,false,NULL},
	{"GARANZIA"		,_NUMBER,3,NULL,false,NULL},

	{"RAEE"			,_ALFA	,1,NULL,false,NULL},
	{"IMPRAEE"		,_NUMBER,7,"4.3",false,NULL},
	{"SIAE"			,_ALFA	,1,NULL,false,NULL},
	{"IMPSIAE"		,_NUMBER,7,"4.3",false,NULL},
	{NULL}
};

//
// IMD - Informazioni sugli articoli
//
EUR_FIELD arsIMD[]= {

	{"ANPROD-IMD"	,	0,318,NULL,true,"Informazioni aggiuntive"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	,3,NULL,true,"IMD"},
	{"TITOLO"		,_ALFA	,35,NULL,false,NULL},
	{"LINGUA"		,_ALFA	,35,NULL,false,NULL},
	{"AUTORE"		,_ALFA	,70,NULL,false,NULL},
	{"ARTISTA"		,_ALFA	,35,NULL,false,NULL},
	{"EDITORE"		,_ALFA	,35,NULL,false,NULL},
	{"FORTISUP"		,_ALFA	,35,NULL,false,NULL},
	{"ETICHETTA"	,_ALFA	,35,NULL,false,NULL},
	{"GENERE"		,_ALFA	,35,NULL,false,NULL},
	{NULL}

};







//
// RFF - Informazioni di riferimento a livello di documento
//
EUR_FIELD arsRFF[]= {

	{"DESADV-RFF"	,	0,53,NULL,true,"Informazioni di riferimento a livello di documento"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	,3 ,NULL,true,"RFF"},
	{"TIPRIF"		,_ALFA	,3 ,NULL,true,NULL},
	{"NUMRIF"		,_ALFA	,35,NULL,true,NULL},
	{"DATARIF"		,_DATE	,8 ,NULL,true,NULL},
	{"ORARIF"		,_NUMBER ,4 ,NULL,false,NULL},

	{NULL}
};

//
// NAD - Informazioni relative alla parte ADV
//
EUR_FIELD arsNAD_D[]= {

	{"NAD-DESADV"	,	0,377,NULL,true,"Informazioni relative alla parte (luoghi)"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"NAD"},
	{"TIPNAD"		,_ALFA ,  3,NULL,true,NULL},
	{"CODNAD"		,_ALFA , 35,NULL,true,NULL},
	{"QCODNAD"		,_ALFA ,  3,NULL,true,NULL},
	{"RAGSOCD"		,_ALFA , 70,NULL,true,NULL},
	{"INDIRD"		,_ALFA , 70,NULL,true,NULL},
	{"CITTAD"		,_ALFA , 35,NULL,true,NULL},
	{"PROVD"		,_ALFA ,  9,NULL,true,NULL},
	{"CAPD"			,_ALFA ,  9,NULL,true,NULL},
	{"NAZIOD"		,_ALFA ,  3,NULL,false,NULL},
	{"TRIBUNALE"	,_ALFA , 35,NULL,false,NULL},
	{"CCIAA"		,_ALFA , 35,NULL,false,NULL},
	{"CAPSOC"		,_ALFA , 35,NULL,false,NULL},
	{"NUREGRAEE"	,_ALFA , 16,NULL,false,NULL},
	{"NUREGPILE"	,_ALFA , 16,NULL,false,NULL},

	{NULL}
};


//
// NAD - Informazioni sul Punto di consegna della merce (Delivery Point)
//
EUR_FIELD arsNAD_I[]= {

	{"NAD-INVOIC"	,	0,305,NULL,true,"Informazioni sul Punto di consegna della merce (Delivery Point)"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"NAD"},
	{"CODCONS"		,_ALFA , 17,NULL,true,NULL},
	{"QCODCONS"		,_ALFA ,  3,NULL,true,NULL},
	{"RAGSOCD"		,_ALFA , 70,NULL,true,NULL},
	{"INDIRD"		,_ALFA , 70,NULL,true,NULL},
	{"CITTAD"		,_ALFA , 35,NULL,true,NULL},
	{"PROVD"		,_ALFA ,  9,NULL,true,NULL},
	{"CAPD"			,_ALFA ,  9,NULL,true,NULL},
	{"NAZIOD"		,_ALFA ,  3,NULL,false,NULL},
	{"NUMBOLLA"		,_ALFA , 35,NULL,true,NULL},
	{"DATABOLL"		,_ALFA ,  8,NULL,true,NULL},
	{"NUMORD"		,_ALFA , 35,NULL,false,NULL},
	{"DATAORD"		,_ALFA ,  8,NULL,false,NULL},
	{NULL}

};

//
// TDT - Informazioni relative alla parte
//
EUR_FIELD arsTDT[]= {

	{"DESADV-TDT"	,	0,118,NULL,true,"Dettagli del trasporto, per specificare il tipo trasporto, il numero di riferimento e l'identificazione dei mezzi di trasporto"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"TDT"},
	{"QUALTRAS"		,_ALFA	, 3,NULL,true,"20"},
	{"NUMRIF"		,_ALFA	,17,NULL,false,NULL},
	{"MODTRAS"		,_ALFA	, 3,NULL,false,NULL},
	{"IDMEZTRAS"	,_ALFA	, 8,NULL,false,NULL},
	{"TIPMEZTRAS"	,_ALFA	, 17,NULL,false,NULL},
	{"IDVETTO"		,_ALFA	, 17,NULL,false,NULL},
	{"DESVETTO"		,_ALFA	, 35,NULL,false,NULL},
	{"DIRTRASP"		,_ALFA	, 3,NULL,false,NULL},
	{"IDMEZZI"		,_ALFA	, 9,NULL,false,NULL},
	{"NAZMEZTRAS"	,_ALFA	, 3,NULL,false,NULL},
	{NULL}
};

//
// LIN - Informazioni di riga documento
//
EUR_FIELD arsLIN[]= {

	{"DESADV-LIN"	,	0,223,NULL,true,"Informazioni di riga documento"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"LIN"},
	{"NUMRIGA"		,_NUMBER, 6,NULL,true,"20"},
	{"CODEANCU"		,_ALFA	,35,NULL,true,NULL},
	{"TIPCODCU"		,_ALFA	,3,NULL,false,NULL},
	{"CODEANTU"		,_ALFA	,35,NULL,false,NULL},
	{"CODFORTU"		,_ALFA	,35,NULL,true,NULL},
	{"CODDISTU"		,_ALFA	,35,NULL,false,NULL},
	{"DESART"		,_ALFA	,35,NULL,false,NULL},

	{"QTAORD"		,_NUMBER,15,"12.3",true,NULL},
	{"UDMQORD"		,_ALFA	,3,NULL,true,NULL},
	{"NRCUINTU"		,_NUMBER,15,"12.3",false,NULL},
	{"UDMCUINTU"	,_ALFA	,3,NULL,false,NULL},
	{NULL}
};

//
// RFR - Informazioni di riga documento
//
EUR_FIELD arsRFR[]= {

	{"RFR"	,	0,53,NULL,true,"Informazioni di riferimento a livello di riga documento"}, // Descrizione e lunghezza generale

	{"TIPORECL"		,_ALFA	, 3,NULL,true,"RFR"},
	{"TIPRIFL"		,_ALFA	, 3,NULL,true,NULL},
	{"NUMRIFL"		,_ALFA	,35,NULL,true,NULL},
	{"DATRIFL"		,_DATE	,8 ,NULL,true,NULL},
	{"ORARIFL"		,_NUMBER,4 ,NULL,false,NULL},
	{NULL}
};


//
// NAI - Informazioni sull’Intestatario Fattura
//
EUR_FIELD arsNAI[]= {

	{"DESADV-NAI"	,	0,324,NULL,true,"Informazioni sull’Intestatario Fattura"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"NAI"},
	{"CODFATT"		,_ALFA	,17,NULL,true,NULL},
	{"QCODFATT"		,_ALFA	, 3,NULL,true,NULL},
	{"RAGSOCI"		,_ALFA	,70,NULL,true,NULL},
	{"INDIRI"		,_ALFA	,70,NULL,true,NULL},
	{"CITTAI"		,_ALFA	,35,NULL,true,NULL},
	{"PROVI"		,_ALFA	, 9,NULL,true,NULL},
	{"CAPI"			,_ALFA	, 9,NULL,true,NULL},
	{"NAZIOI"		,_ALFA	, 3,NULL,false,NULL},
	{"PIVANAZI"		,_ALFA	,35,NULL,true,NULL},
	{"NUREGCOOP"	,_ALFA	,35,NULL,false,NULL},
	{"CODFISCA"		,_ALFA	,35,NULL,false,NULL},

	{NULL}
};


//
// PAT - Informazioni su termini, modalità di pagamento
//
EUR_FIELD arsPAT[]= {

	{"INVOCE-PAT"	,	0,198,NULL,true,"Informazioni su termini, modalità di pagamento"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"PAT"},
	{"TIPOCOND"		,_ALFA	, 3,NULL,true,NULL},
	{"DATASCAD"		,_DATE	, 8,NULL,false,NULL},
	{"CONDPAG"		,_ALFA	,12,NULL,false,NULL},
	{"IMPORTO"		,_NUMBER,16,"+12.3",false,NULL},
	{"DIVISA"		,_ALFA	, 3,NULL,false,NULL},
	{"PERC"			,_NUMBER, 7,"3.4",false,NULL},
	{"DESCRIZ"		,_ALFA,  35,NULL,false,NULL},
	{"BANCACOD"		,_ALFA,  35,NULL,false,NULL},
	{"BANCADESC"	,_ALFA,  35,NULL,false,NULL},
	{"FACTOR"		,_ALFA,  35,NULL,false,NULL},
	{"CODPAG"		,_ALFA,   3,NULL,false,NULL},
	{"MEZZOPAG"		,_ALFA,   3,NULL,false,NULL},

	{NULL}
};

//
// DET - Riga Documento
//
EUR_FIELD arsDET[]= {

	{"INVOCE-DET"	,	0,303,NULL,true,"Informazioni di riga documento"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"DET"},
	{"NUMRIGA"		,_NUMBER, 6,NULL,false,NULL},
	{"IDSOTTOR"		,_ALFA	, 3,NULL,false,NULL},
	{"NUMSRIGA"		,_NUMBER, 6,NULL,false,NULL},
	{"CODEANCU"		,_ALFA	, 35,NULL,false,NULL},
	{"TIPCODCU"		,_ALFA	, 3,NULL,false,NULL},
	{"CODEANTU"		,_ALFA	, 35,NULL,false,NULL},
	{"CODFORTU"		,_ALFA	, 35,NULL,false,NULL},
	{"CODDISTU"		,_ALFA	, 35,NULL,false,NULL},
	{"TIPQUANT"		,_ALFA	, 3,NULL,true,NULL},
	{"QTACONS"		,_NUMBER,16,"+12.3",true,NULL},
	{"UDMQCONS"		,_ALFA	, 3,NULL,false,NULL},
	{"QTAFATT"		,_NUMBER,16,"+12.3",true,NULL},
	{"UDMQFATT"		,_ALFA	, 3,NULL,false,NULL},
	{"NRCUINTU"		,_NUMBER,16,"+12.3",false,NULL},
	{"UDMNRCUINTU"	,_ALFA	, 3,NULL,false,NULL},
	{"PRZUNI"		,_NUMBER,16,"+12.3",false,NULL},
	{"TIPOPRZ"		,_ALFA	, 3,NULL,false,NULL},
	{"UDMPRZUN"		,_ALFA	, 3,NULL,false,NULL},
	{"PRZUN2"		,_NUMBER,16,"+12.3",false,NULL},
	{"TIPOPRZ2"		,_ALFA	, 3,NULL,false,NULL},
	{"UDMPRZUN2"	,_ALFA	, 3,NULL,false,NULL},
	{"IMPORTO"		,_NUMBER,16,"+12.3",false,NULL},
	{"DIVRIGA"		,_ALFA	, 3,NULL,false,NULL},
	{"IMPORTO2"		,_NUMBER,16,"+12.3",false,NULL},
	{"DIVRIGA2"		,_ALFA	, 3,NULL,false,NULL},

	{NULL}
};


//
// IVA - SubTotali Iva
//
EUR_FIELD arsIVA[]= {

	{"IVA"	,	0, 83,NULL,true,"SubTotali IVA"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"IVA"},
	{"TIPOTASS"		,_ALFA	, 3,NULL,false,"VAT"},
	{"DESCRIZ"		,_ALFA	,35,NULL,true,NULL},
	{"CATIMP"		,_ALFA	, 3,NULL,false,NULL},
	{"ALIQIVA"		,_NUMBER, 7,"3.4",true,NULL},
	{"SIMPONIB"		,_NUMBER,16,"+12.3",true,NULL},
	{"SIMPORTO"		,_NUMBER,16,"+12.3",true,NULL},

	{NULL}
};


//
// TMA - Totali Fattura
//
EUR_FIELD arsTMA[]= {

	{"TMA"	,	0, 217,NULL,true,"Totali Fattura"}, // Descrizione e lunghezza generale

	{"TIPOREC"		,_ALFA	, 3,NULL,true,"TMA"},
	{"TOTDOC1"		,_NUMBER,16,"+12.3",true,NULL}, // Compreso IVA
	{"IMPOSTA1"		,_NUMBER,16,"+12.3",true,NULL}, // Imposte
	{"IMPONIB1"		,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"TOTRIGHE1"	,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"TOTANT1"		,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"TOTPAG1"		,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"DIVISA1"		,_ALFA	, 3,NULL,false,NULL},

	{"TOTDOC2"		,_NUMBER,16,"+12.3",false,NULL}, 
	{"IMPOSTA2"		,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"IMPONIB2"		,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"TOTRIGHE2"	,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"TOTANT2"		,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"TOTPAG2"		,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{"DIVISA2"		,_ALFA	, 3,NULL,false,NULL},
	{"TOTMERCE"		,_NUMBER,16,"+12.3",false,NULL}, // Imposte
	{NULL}
};


//
// Elenco Tipi disponibili
//
static EUR_FIELD arsDES[]= {

	{"DES"		,0,	     178,NULL,true,"Descrizione"}, // Descrizione e lunghezza generale
	{"TIPOREC"	,_ALFA	,  3,NULL,true,"DES"},
	{"DESCR"	,_ALFA	,175,NULL,true,NULL},
	{NULL}

};

//
// MOA - informazioni prezzo a livello riga documento
//
static EUR_FIELD arsMOA[]= {

	{"MOA"		,0,	     24,NULL,true,""}, // Descrizione e lunghezza generale
	{"TIPOREC"	,_ALFA	,  3,NULL,true,"MOA"},
	{"RIFPRZBOL",_ALFA	,  3,NULL,true,"146"}, // Prezzo unitario
	{"PRZBOL"	,_NUMBER,  15,"12.3",true,NULL}, // Prezzo unitario
	{"DIVPRZ"	,_ALFA	,  3,NULL,true,"EUR"}, // Prezzo unitario
	{NULL}

};


//
// TAX_I- Tasse riga su invo
//
static EUR_FIELD arsTAX_I[]= {

	{"TAX-I"		,0,	    67,NULL,true,""}, // Descrizione e lunghezza generale
	{"TIPOREC"	,_ALFA	,  3,NULL,true,"TAX"},
	{"TIPOTASS"	,_ALFA	,  3,NULL,true,NULL}, 
	{"DESCRIZ"	,_ALFA	,  35,NULL,true,NULL}, 
	{"CATIMP"	,_ALFA	,  3,NULL,true,NULL}, 
	{"ALIQIVA"	,_NUMBER,  7,"3.4",true,NULL}, 
	{"IMPORTO"	,_NUMBER, 16,"+12.3",true,NULL}, 
	{NULL}

};

//
// Elenco Tipi disponibili
//
static S_EUR_ITEM _arsRec[]= {

	{"BGM",arsBGM},		// Intestazione documento
	{"NAS-A",arsNAS_A},	// NAS ANPROD
	{"NAS-I",arsNAS_I}, // NAS INVOIC
	{"LIA",arsLIA},
	{"IMD",arsIMD},
	{"RFF",arsRFF},
	{"NAD-D",arsNAD_D},	// NAD - DESADV
	{"NAD-I",arsNAD_I},	// NAD - INVOIC
	{"TDT",arsTDT},
	{"LIN",arsLIN},
	{"RFR",arsRFR},
	{"MOA",arsMOA},		// Informazioni Prezzo a livello riga documento

	{"NAI",arsNAI},		// Informazioni sull’Intestatario Fattura
	{"PAT",arsPAT},		// Informazioni su termini, modalità di pagamento

	{"DET",arsDET},		// Informazioni di riga documento
	{"IVA",arsIVA},		// SubTotali Iva
	{"TMA",arsTMA},		// Totale Documento
	{"DES",arsDES},		// Descrizioni
	{"TAX-I",arsTAX_I},	// Tasse riga su invoice

	{NULL}

};





//
// euOpen()
//
EUR_FILE * euOpen(UTF8 * pszFileName) {

	EUR_FILE * psRet=ehAllocZero(sizeof(EUR_FILE));
	psRet->pszFileName=strDup(pszFileName);
	psRet->lstFile=lstNew();
	//psRet->psFile=fileOpen(pszFileName,FO_WRITE|FO_CREATE_ALWAYS);
//	if (!psRet->psFile) ehExit("Non posso aprire %s",pszFileName);
//	printf("open:[%s]" CRLF,pszFileName); _getch();
	return psRet;
	
}

//
// euClose()
//
CHAR * euClose(EUR_FILE * psEurFile,BOOL bGetRemove) {

	CHAR * pszRet=NULL;
	EH_FILE * psFile;
	CHAR * psz;
	
	psFile=fileOpen(psEurFile->pszFileName,FO_OPEN_EXCLUSIVE|FO_WRITE|FO_CREATE_ALWAYS);
	if (!psFile) ehExit("Non posso aprire %s",psEurFile->pszFileName);
	
	for (lstLoop(psEurFile->lstFile,psz)) {
		fileWrite(psFile,psz,strlen(psz));
	}

	fileClose(psFile);
	if (bGetRemove) 
	{
		pszRet=fileStrRead(psEurFile->pszFileName);
		fileRemove(psEurFile->pszFileName);

	} else pszRet="OK";

	if (!fileSize(psEurFile->pszFileName)) fileRemove(psEurFile->pszFileName);
	psEurFile->lstFile=lstDestroy(psEurFile->lstFile);
	ehFree(psEurFile->pszFileName);
	ehFree(psEurFile);

	return pszRet;
}




//
// euCreate() Crea un record
//
EUR_RECORD * euCreate(EUR_FILE * psEurFile,CHAR * pszTipo) {

	EUR_RECORD * psRec=ehAllocZero(sizeof(EUR_RECORD));
	INT a,iTotal;
	psRec->psEurFile=psEurFile;
	psRec->arsFormat=NULL;

	for (a=0;_arsRec[a].pszName;a++) {
		if (!strCmp(_arsRec[a].pszName,pszTipo)) {psRec->arsFormat=_arsRec[a].ars; break;}
	}
	if (!psRec->arsFormat) ehExit("ueCreate: Record '%s' non implementato",pszTipo);

	
	// Verifica lunghezza
	iTotal=0;
	for (a=1;psRec->arsFormat[a].pszName;a++) {
		iTotal+=psRec->arsFormat[a].iLen;
		psRec->arsFormat[a].pszValue=NULL;
	}
	if (iTotal!=psRec->arsFormat[0].iLen) 
		ehExit("Check-sum non coerente in creazione record [%s]",euRecInfo(psRec));
//	psRec->lstSet=lstCreate(sizeof(EUR_SET));
	return psRec;
}

//
// euDestroy()
//
EUR_RECORD * euDestroy(EUR_RECORD * psRec) {

	EUR_FIELD * arsFormat;
	INT a;
	if (!psRec) return NULL;

	// Libero risorse impegnate
	arsFormat=psRec->arsFormat;
	for (a=1;arsFormat[a].pszName;a++) {
		ehFreePtr(&arsFormat[a].pszValue);
	}
	ehFree(psRec);
	
	return NULL;
}


//
// eurSearch()
//
EUR_FIELD * eurSearch(EUR_FIELD * arsField,CHAR * pszFieldName) {

	INT a;
	for (a=1;arsField[a].pszName;a++) {
		if (!strcmp(arsField[a].pszName,pszFieldName)) return arsField+a;
	}
	return NULL;
}

//
// euRecInfo()
//
CHAR * euRecInfo(EUR_RECORD * psRec) {

	static CHAR szServ[500];
	sprintf(szServ,"%s (%s)",psRec->arsFormat[0].pszName,psRec->psEurFile->pszFileName);
	return szServ;
}


//
// euSet()
//
BOOL euSet(EUR_RECORD * psRec, CHAR * pszFieldName,CHAR * pszValue) {

	EUR_FIELD * psField;
//	EUR_SET * psSet,sSet;

	psField=eurSearch(psRec->arsFormat,pszFieldName);
	if (!psField) ehExit("Campo %s inesistente in record %s (%s)",pszFieldName,psRec->arsFormat[0].pszName,psRec->arsFormat[0].pszDefault);

	// Analizzo il tipo di campo e creo il valore
	/*
	for (lstLoop(psRec->lstSet,psSet)) {
		if (!strCaseCmp(psSet->pszFieldName,pszFieldName)) ehExit("Valore [%s] assegnato due volte",pszFieldName,euRecInfo(psRec));
	}
	*/
	if (psField->pszValue) ehExit("Valore [%s] assegnato due volte",pszFieldName,euRecInfo(psRec));
	
//	_(sSet);
	switch (psField->enType) {
	
		case _ALFA:
			if (strlen(pszValue)>(UINT) psField->iLen) 
				ehExit("Campo troppo lungo ('%s'>%d): Campo %s in file %s",pszValue,psField->iLen,pszFieldName,euRecInfo(psRec));
//			lstPush(psRec->lstSet,&sSet);
			psField->pszValue=strDup(pszValue);
			break;

		case _DATE:
			if (strEmpty(pszValue)) pszValue="00000000";
			if (strlen(pszValue)!=8) 
				ehExit("Data non corente !=8 (%s):Campo %s in file %s",psField->pszValue,pszFieldName,euRecInfo(psRec));
			psField->pszValue=strDup(pszValue);
			break;


		case _NUMBER:
			psField->pszValue=strDup(pszValue);
			break;

		default:
			ehExit("Tipo non gestito: Campo %s in file %s",pszFieldName,euRecInfo(psRec));

	}

	return false;
}

//
// euSetf()
//
BOOL euSetf(EUR_RECORD * psRec, CHAR * pszFieldName,CHAR * pszFormat, ...) {

	CHAR *    psz;
	BOOL bRet;

	strFromArgs(pszFormat,psz); 
	bRet=euSet(psRec,pszFieldName,psz);
	ehFree(psz);
	return bRet;
}


//
// euWrite() - Salvo il record del file
//
BOOL euWrite(EUR_RECORD * psRec,BOOL bRecFree) {

	INT a;
	CHAR * psz;
	EUR_FIELD * arsFormat=psRec->arsFormat;
	EH_LST lstTxt=lstNew();
	BOOL bSign;
	INT iInteger,iDecimal;
	double dValue;
	CHAR szFormat[200];
	CHAR szServ[200];

	//
	// Controllo se ho tutti i campi obbligatori
	//
	for (a=1;arsFormat[a].pszName;a++) {
		
		CHAR * pszValue=(arsFormat[a].pszValue?arsFormat[a].pszValue:arsFormat[a].pszDefault);
		if (arsFormat[a].bObb&!pszValue) 
			ehExit("euWrite(): Tipo %s: %s non indicato",euRecInfo(psRec),arsFormat[a].pszName);
		pszValue=(arsFormat[a].pszValue?arsFormat[a].pszValue:strEver(arsFormat[a].pszDefault));

		psz=ehAlloc(arsFormat[a].iLen+1); psz[arsFormat[a].iLen]=0;
		switch (arsFormat[a].enType) {
			
			case _ALFA:
			case _DATE:
				memset(psz,' ',arsFormat[a].iLen);
				memcpy(psz,pszValue,strlen(pszValue));
				break;
				
			case _NUMBER:
				if (!strEmpty(arsFormat[a].pszFormat)) {
					
					CHAR * p=arsFormat[a].pszFormat;
					EH_AR ar;
					if (*p=='+') {
						p++; 
						bSign=true;
					} else bSign=false;
					ar=ARFSplit(p,".");
					iInteger=atoi(ar[0]);
					iDecimal=0;
					if (ARLen(ar)>1) {
						iDecimal=atoi(ar[1]);
						if (iDecimal) iInteger+=(iDecimal+1);
						if (bSign) iInteger++;
					}
					ehFree(ar);
					
				} else {
					bSign=false;
					iInteger=arsFormat[a].iLen;
					iDecimal=0;
				}
				dValue=atof(pszValue);
				sprintf(szFormat,"%%%d.%df",iInteger,iDecimal);
				sprintf(szServ,szFormat,dValue); 
				while (strReplace(szServ," ","0"));
				strcpy(szServ,strOmit(szServ,"."));
				if (bSign) *szServ=(dValue<0)?'-':'+';
				iInteger=strlen(szServ);
				if (iInteger!=arsFormat[a].iLen) 
					ehError();
				strcpy(psz,szServ);
				break;
				
		
			default:
				ehError();
		
		}
		lstPush(lstTxt,psz);
		ehFree(psz);

	}

	// Salvo il record
	psz=lstToString(lstTxt,"","",CRLF);
	lstPush(psRec->psEurFile->lstFile,psz);
//	fileWrite(psRec->psEurFile->psFile,psz,strlen(psz));
	lstDestroy(lstTxt);
	ehFree(psz);

	if (bRecFree) {
		euDestroy(psRec);
	}

	return false;
}

