//   +-------------------------------------------+
//   | mySql
//   | Interfaccia al dbase mySql
//   |             
//   |							  Ferrà srl 2006
//   +-------------------------------------------+


//
// Inizializzazione 
//

#ifdef _ADB32
	ADB_max=30;
	PRG_start(adb_start,adb_end);
#else
	PRG_start(mys_start,mys_end);
#endif

//
//	Connessione al db
//

	FGetProfileChar(SETUP_INI,"MYSQL_SERVER",sSetup.szMySqlServer);
	FGetProfileChar(SETUP_INI,"MYSQL_USER",sSetup.szMySqlUser);
	FGetProfileChar(SETUP_INI,"MYSQL_PASSWORD",sSetup.szMySqlPassword);
	if (!*sSetup.szMySqlServer||
		!*sSetup.szMySqlUser||
		!*sSetup.szMySqlPassword) PRG_end("Manca il setup del mySql in " SETUP_INI);

	if (!mys_Connect(sSetup.szMySqlServer,
					 sSetup.szMySqlUser,
					 sSetup.szMySqlPassword,
					 "myApplication", // <--------------- Schema
					 0))
	{
		PRG_end("MYSQL:Ha fallito la connessione al dbase: Error: %s" CRLF,mys_GetError());
	}

//
// Disconessione (non serve la fa il PRG_end)
//
	mys_Deconnect();

//
// Esempio di query con SELECT
//
	EH_MYSQL_RS rsBoat;

	mys_queryarg(FALSE,
				"SELECT AUTOCODE,MODELLO,IDMODEL "
				"FROM webboat_2_0 "
				"WHERE (IDCANT=%d AND BLOCCO=0) ORDER BY 2 DESC",idCantiere);

	rsBoat=mys_store_result(); 
	while (mys_fetch_row(rsBoat))
	{
		SINT ido=mys_fldint(rsBoat,"AUTOCODE"); // <-- lettura con il nome
		pModello=mys_fldptr(rsBoat,"2"); // <-- lettura con il numero di colonna (1=la prima)
	}
	mys_free_result(rsBoat);

//
// Utilità
//

	// Conta il numero di righe
	iRecMax=mys_count("Aziende A1,AziendePlus A2 WHERE (A1.CODICE=A2.CODICE AND FMB=1 AND BLOCCO=0 AND FBLOCCOOFF=0)");

//
// Esempio di migrazione
//

//	sMYS.psRS = Result set globale

#ifdef _MYSQL // Versione SQL
	iRecMax=mys_count("Aziende A1,AziendePlus A2 WHERE (A1.CODICE=A2.CODICE AND FMB=1 AND BLOCCO=0 AND FBLOCCOOFF=0)");
	mys_queryarg(FALSE,"SELECT * FROM Aziende A1,AziendePlus A2 WHERE (A1.CODICE=A2.CODICE AND FMB=1 AND BLOCCO=0 AND FBLOCCOOFF=0)"); 
#else		  // Versione ADB
	iRecMax=adb_recnum(hdbAziende);
	if (!adb_find(hdbAziende,0,B_GET_FIRST,NULL,B_RECORD))
#endif
	{
		DMIOpen(&dmiAziende,RAM_AUTO,iRecMax,sizeof(BL),"BL");

#ifdef _MYSQL // Versione SQL
		sMYS.psRS=mys_store_result(); // Richiedo il risultato
		while (mys_fetch_row(sMYS.psRS)) 
		{
#else		  // Versione ADB
		do {
			if (!adb_FldInt(hdbAziende,"FMB")) continue; // Non è in mondialbroker
			if (adb_FldInt(hdbAziende,"BLOCCO")) continue; // E' bloccato
			if (adb_FldInt(hdbAziende,"FBLOCCOOFF")) continue; // Ha il blocco delle offerte
			if (adb_ASfind(hdbAziendePlus,"PERCOD",B_GET_EQUAL,1,(double) adb_FldInt(hdbAziende,"CODICE"))) PRG_end("Problema Plus [%d]",adb_FldInt(hdbAziende,"CODICE"));
#endif
			iBroker++;
			if (mys_FldInt(hdbAziende,"FDEMO")) iBrokerDemo++; else iBrokerReali++;

			ZeroFill(sBrokerList); 

			// funzioni compatibili sia con pervasive che con mysql
			sBrokerList.iCodice=mys_FldInt(hdbAziende,"CODICE");
			strcpy(sBrokerList.szRag,mys_FldPtr(hdbAziende,"Rag_sociale"));
			sBrokerList.iOfferte=mys_FldInt(hdbAziende,"OFRDIRECT")+mys_FldInt(hdbAziende,"OFRGIMBA")-mys_FldInt(hdbAziende,"OFRHIDE");
			sBrokerList.fDemo=mys_FldInt(hdbAziende,"FDEMO");
			sBrokerList.fPremium=mys_FldInt(hdbAziendePlus,"FPREMIUM");
			strcpy(sBrokerList.szLang,mys_FldPtr(hdbAziende,"LINGUA"));

			DMIAppendDyn(&dmiAziende,&sBrokerList);
		} 
#ifdef _MYSQL // Versione SQL
		mys_free_result(sMYS.psRS); // Libero le risorse
#else
		while (!adb_find(hdbAziende,0,B_GET_NEXT,NULL,B_RECORD));
#endif
