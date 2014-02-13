
// 
#ifdef EH_MAIN
	struct ADB_INFO * ADB_info=NULL;
	INT		ADB_hdl=-1;
	INT		ADB_ult=0;
	INT		ADB_max=16; // Numeri di Db apribili contemporanemante
	INT		ADB_network=OFF; // ON/OFF se si sta usando un NetWork
	INT		ADB_lock=1;// 0 Ritorna senza segnalare l'errore
									// 1 Riprova e segnala che stai riprovando
									// 2 Riprova senza segnalare che stai riprovando
									// 3 Chiede all'operatore se devere riprovare
	INT		ADB_iLastStatus=0; // new 8/2007
	BOOL	fCobolLimitControl=FALSE;
	INT		ADB_MaxBlobSize=2048;
#else

	extern struct ADB_INFO * ADB_info;
	extern INT		ADB_hdl;
	extern INT		ADB_ult;
	extern INT		ADB_max; // Numeri di Db apribili contemporanemante
	extern INT		ADB_network; // ON/OFF se si sta usando un NetWork
	extern INT		ADB_lock;// 0 Ritorna senza segnalare l'errore
									// 1 Riprova e segnala che stai riprovando
									// 2 Riprova senza segnalare che stai riprovando
									// 3 Chiede all'operatore se devere riprovare
	extern INT		ADB_iLastStatus; // new 8/2007
	extern BOOL		fCobolLimitControl;
	extern INT		ADB_MaxBlobSize;

#endif