//  -----------------------------------------------------------------------
//  | CGI_ADB  Gestione ADB  AdvancedDataBase                             |
//  |                                                                     |
//  |                                by Ferra' Art & Technology 1996-2002 |
//  -----------------------------------------------------------------------

#include "\ehtool\include\ehsw_idb.h"
#include <math.h>
/*
 extern struct ADB_INFO *ADB_info;
 extern SINT ADB_hdl;
 extern SINT ADB_ult; // Numero di MGZ
 extern SINT ADB_max;
 extern SINT ADB_network;
 extern SINT EHPower;

 extern SINT ADB_network;   // ON/OFF se si sta usando un NetWork
 extern SINT ADB_HL;   // Se il Link gerarchico ä attivo
 extern CHAR HLtabella[];  // Percorso e nome della Tabella Gerarchica
 extern LONG adb_HLink;    // Numero di Link Gerarchici
 extern SINT adb_HLinkHdl; // Handle che contiene la tabella

 extern SINT ADB_lock;    // Flag di comportamento in lock
*/
struct ADB_INFO *ADB_info=NULL;
SINT  ADB_hdl=-1;
SINT  ADB_ult=0;
SINT  ADB_max=16; // Numeri di Db apribili contemporanemante

SINT  ADB_network=OFF; // ON/OFF se si sta usando un NetWork
SINT  ADB_lock=1;// 0 Ritorna senza segnalare l'errore
								// 1 Riprova e segnala che stai riprovando
								// 2 Riprova senza segnalare che stai riprovando
								// 3 Chiede all'operatore se devere riprovare

SINT  ADB_HL=OFF; // Se il Link gerarchico ä attivo
CHAR  HLtabella[MAXPATH];// Percorso e nome della Tabella Gerarchica
LONG  adb_HLink=0; // Numero di Link Gerarchici
SINT  adb_HLinkHdl=-1; // Handle che contiene la tabella


// void   BTRCLOSE(void);
// BTI_API BTRVCE(void);
void BTI_err(CHAR *file,CHAR *opera,SINT status);

static SINT adb_BlobNewOfs(HDB Hdb,SINT iSize);
static void adb_BlobFldWrite(SINT Hdb,CHAR *lpField,CHAR *lpBuffer);

static SINT  adb_HLfind(CHAR *FilePadre,LONG *pt);
static SINT  adb_HLfindFiglio (CHAR *FileFiglio,LONG *pt);
static void  adb_HLload(FILE *pf1,struct ADB_LINK_LIST *link);
static void  adb_HLsave(FILE *pf1,struct ADB_LINK_LIST *link,CHAR *mark);
static void  adb_HLinsert(CHAR *file,struct ADB_LINK *link);
static void  adb_HLinsertPro (CHAR *file,CHAR *client);
char* ConvertoPro (char*Proprietario);

static CHAR *adb_HLaction(SINT Hdb,CHAR *);

//static char *adb_HLCloneUpdate(int Hdb,long,char *);
static SINT  adb_HLsonopadre(SINT Hdb);
static SINT  adb_LockControl(SINT Flag,SINT Hadb,SINT status,CHAR *tip,SINT Escape);
static void  adb_HLchiudi(void);

// --------------------------------------
// Utiliti di copia adb_buffer          !
// --------------------------------------

void HookCopy(SINT cmd,SINT Hdb)
{
 static SINT Size;
 static SINT MemoHdl=-1;
 static CHAR *PtrRecord; // Puntatore al Record
 static CHAR *PtrCopy;
 switch (cmd)
 {
	 case APRI:

		 if (MemoHdl!=-1) memo_libera(MemoHdl,"HC1");
		 PtrRecord=adb_recbuf(Hdb);
		 if (PtrRecord==NULL) PRG_end("HC2");
		 Size=adb_recsize(Hdb);
		 if (Size<10) PRG_end("HC3");
		 MemoHdl=memo_chiedi(RAM_HEAP,Size,"HookCopy");
		 if (MemoHdl<0) PRG_end("HC:MemoHdl");
		 PtrCopy=memo_heap(MemoHdl);
		 break;

	case CHIUDI:
		 memo_libera(MemoHdl,"HC4");
		 MemoHdl=-1;
		 break;

	case HC_BACKUP: // Copia nel buffer

		 if (MemoHdl==-1) PRG_end("HC5");
		 memcpy(PtrCopy,PtrRecord,Size);
		 break;

	case HC_RESTORE: // Ripristina l'hook

		 if (MemoHdl==-1) PRG_end("HC6");
		 memcpy(PtrRecord,PtrCopy,Size);
		 break;

	case HC_RESET : // Reset della copia

		 if (MemoHdl==-1) PRG_end("HC7");
		 memset(PtrCopy,0,Size);
		 break;
 }
}

//  +-----------------------------------------∏
//	| ADB_START e ADB_END                     |
//	|                                         |
//	|  Inizializza e chiude la gestione       |
//	|  del Dbase                              |
//	|                                         |
//	|  Ritorna    : -1 Non cä suff. memoria   |
//	|                                         |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+
void adb_start(SINT cmd)
{
 WORD a;
 CHAR serv[20],*pt=serv;

 // Ultima chiamata
 if (cmd!=WS_LAST) return;

 if (ADB_max==0) {ADB_hdl=-1; return;}

 ADB_hdl=memo_chiedi(RAM_HEAP,
					 (LONG) sizeof(struct ADB_INFO)*ADB_max,
					 "*ADB");

 if (ADB_hdl<0) PRG_end("Non cä memoria per ADB_SYSTEM");
 ADB_info=(struct ADB_INFO *) memo_heap(ADB_hdl);
 ADB_ult=0;

 //		Azzera locazione
 for (a=0;a<ADB_max;a++)
	{ADB_info[a].HdlRam=-1;
	 ADB_info[a].FileName=NULL;
	 ADB_info[a].PosBlock=NULL;
	 ADB_info[a].AdbHead=NULL;
	}

 // Se ä attivo il Link
 if (ADB_HL) adb_HLapri();

 // Controlla se si stÖ girando in rete

 if (ini_find("adb_network",pt)) return;

//	Cerca primo spazio
//	 for (;*pt!=0;pt++) {if (*pt==' ') break;}
//	 if (*pt==0) return -2;
  pt=strstr(pt," ");
  if (pt!=NULL)
	 {pt++;
		if (!strcmp(strupr(pt),"ON"))
			{ADB_network=ON;
			 //lic_load(EHPath("net"),1,RAM_ALTA);
			}
	 } else ADB_network=OFF;

}

void adb_end(void)
{
 SINT a;
 if (ADB_hdl==-1) return;

 //		Chiude tutti i db
 for (a=0;a<ADB_max;a++) adb_close(a);
 if (ADB_HL) adb_HLchiudi();

 memo_libera(ADB_hdl,"*ADB");
 ADB_hdl=-1;
}
//  +-----------------------------------------∏
//	| ADB_RESET Resetta il gestore BTRIEVE    |
//	|                                         |
//	|  - Chiude tutti i file aperti           |
//	|  - Rilascia i lock occupati dei record  |
//	|  - Chiude una transazione attiva        |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+
void adb_reset(void)
{
	BTRV( B_RESET,0,0,0,0,0);
}
//   +-------------------------------------------∏
//   | ADB_RECSIZE Ritorna la grandezza record   |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna > 0 = Grandezza record           |
//   |          < 0 = Errore                     |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_recsize(SINT Hadb)
{
	if (((Hadb<0)||(Hadb>=ADB_max))||(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("recsize",Hadb);
	return ADB_info[Hadb].AdbHead->wRecSize;
}
//   +-------------------------------------------∏
//   | ADB_KEYMAXSIZE Ritorna la grandezza della |
//   |                chiave + grande            |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna > 0 = Grandezza record           |
//   |          < 0 = Errore                     |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_keymaxsize(SINT Hadb)
{
	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("keymaxsize",Hadb);
	return ADB_info[Hadb].AdbHead->KeyMoreGreat;
}
//   +-------------------------------------------∏
//   | ADB_RECNUM  Ritorna il numero di chiavi   |
//   |             contenute nel db              |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna >=0 : Numero di chiavi           |
//   |          < 0 = Errore                     |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+

LONG adb_recnum(SINT Hadb)
{
	SINT hdl;
	BTI_WORD size;
	CHAR *ADBbuf;
	LONG NRecord;

#pragma pack(1)
	typedef struct {
	 INT16 LenRec;
	 INT16 PageSize;
	 INT16 IndexNum;
	 LONG RecNum;
	 INT16 fileflag;
	 INT16 reserved;
	 INT16 PageNoUsed;
	} B_INFOREC ;
#pragma pack()

	B_INFOREC *databuf;
	BTI_SINT status;


	if (((Hadb<0)||(Hadb>=ADB_max))||(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("recnum",Hadb);

	size=128+(1+ADB_info[Hadb].AdbHead->Index)*16+265; // Ci sto' largo
	hdl=memo_chiedi(RAM_HEAP,size,"adb_recnum");
	if (hdl<0) win_errgrave("ADB_renum: No memory");
	ADBbuf=memo_heap(hdl);
	databuf=(B_INFOREC *) ADBbuf;

	status = BTRV( B_STAT,
				   ADB_info[Hadb].PosBlock,
				   databuf,// Puntatore ai dati
				   &size, // Grandezza dei dati
				   ADBbuf,   // ???> in output
				   0); // ??? in input
	if (status) {BTI_err(ADB_info[Hadb].FileName,"STAT:",status);}

	NRecord=databuf->RecNum;
	memo_libera(hdl,"adb_recnum");
	if (!status) return NRecord;

	return status;
}

//   +-------------------------------------------∏
//   | ADB_FINDPERC Ritorna la percentuale       |
//   |             approssimativa della posizione|
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |  Index: Key path usato                    |
//   |         -1 percentuale posizione fisica   |
//   |                                           |
//   |  Ritorna lo status BTRIEVE                |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_findperc(SINT Hadb,SINT IndexNum,float *Fperc)
{
	BTI_WORD dataLen;
	SINT status;
	SINT *perc;
	BTI_BYTE dataBuf[8];

	if (((Hadb<0)||(Hadb>=ADB_max))||(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("findperc",Hadb);

	dataLen=4;
	status = BTRV( 45,
				   ADB_info[Hadb].PosBlock,
				   &dataBuf, // Record dove andare
				   &dataLen,// Lunghezza buffer
				   ADB_info[Hadb].KeyBuffer,
				   (WORD) IndexNum);

	perc=(SINT *) &dataBuf;
//	sprintf(serv,"Len%d %d indexnum=%d datalen=%d %s %s",
								//*perc,*(perc+1),IndexNum,dataLen,dataBuf,ADB_info[Hadb].KeyBuffer);
	//Adispm(0,100,15,1,ON,SET,serv);

	*Fperc=(float) (*perc)/100;

	if (status) BTI_err(ADB_info[Hadb].FileName,"FINDPERC:",status);
	return status;
}
//   +-------------------------------------------∏
//   | ADB_GETPERC Legge un record tramite       |
//   |             percentuale (setta il DB pt)  |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |  Index: Key path usato                    |
//   |         -1 percentuale posizione fisica   |
//   |                                           |
//   |  Ritorna >=0 : Numero di chiavi           |
//   |          < 0 = Errore                     |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_getperc(SINT Hadb,SINT IndexNum,float Fperc)
{
	BTI_WORD dataLen;
	SINT status;
	SINT *perc;
	//CHAR serv[80];

	Fperc*=100;
	if (Fperc>10000) Fperc=10000;
	if (Fperc<0)     Fperc=0;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("getperc",Hadb);

	perc=(SINT *) ADB_info[Hadb].RecBuffer;
	*perc=(SINT) Fperc;
	dataLen=ADB_info[Hadb].AdbHead->wRecSize;
	status = BTRV(B_SEEK_PERCENT,
								ADB_info[Hadb].PosBlock,
								ADB_info[Hadb].RecBuffer, // Record dove andare
								&dataLen,// Lunghezza buffer
								ADB_info[Hadb].KeyBuffer,
								(WORD) IndexNum);

	if (status) BTI_err(ADB_info[Hadb].FileName,"GETPERC:",status);
	return status;
}

//   +-------------------------------------------∏
//   | ADB_RECBUF Ritorna il puntatore al buffer |
//   |            per i record                   |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna NULL = Errore                    |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
void *adb_recbuf(SINT Hadb)
{
	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("recbuf",Hadb);
	return ADB_info[Hadb].RecBuffer;
}

//   +-------------------------------------------∏
//   | ADB_KEYBUF Ritorna il puntatore al buffer |
//   |            per le chiave (KeyMoreGreat)   |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna NULL = Errore                    |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
void *adb_keybuf(SINT Hadb)
{
	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("keybuf",Hadb);
	return ADB_info[Hadb].KeyBuffer;
}

//   +-------------------------------------------∏
//   | ADB_RecReset  Pulisce il record buffer    |
//   |                                           |
//   |  Hdb : Numero del adb handle              |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
void adb_recreset(SINT Hadb)
{
	struct ADB_REC *Field;
	SINT a;

	if (((Hadb<0)||(Hadb>=ADB_max))||(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("RecReset",Hadb);
	memset(ADB_info[Hadb].RecBuffer,0,ADB_info[Hadb].AdbHead->wRecSize); // Pulisce il record

	Field=(struct ADB_REC *) farPtr((CHAR *) ADB_info[Hadb].AdbHead,sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hadb].AdbHead->Field;a++)
	{
		adb_FldWrite(Hadb,Field[a].desc,"",0);
	}
}

//   +-------------------------------------------∏
//   | ADB_INSERT Inserisce un nuovo record      |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
/*
SINT adb_insert(SINT Hadb)
{
	BTI_WORD dataLen;
	BTI_SINT status;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("insert",Hadb);

	rifai:
	dataLen=ADB_info[Hadb].AdbHead->RecSize;
	status = BTRV( B_INSERT,
								 ADB_info[Hadb].PosBlock,
								 ADB_info[Hadb].RecBuffer,// Puntatore ai dati
								 &dataLen, // Grandezza dei dati
								 ADB_info[Hadb].KeyBuffer,   // ???> in output
								 0); // ??? in input

	if (status==5) goto AVANTI;

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {if (adb_LockControl(ON,Hadb,status,"Vietato l'inserimento.",OFF)) goto rifai;
	 }
	 else
	 {
	 if (status) {BTI_err(ADB_info[Hadb].FileName,"INSERT:",status);}
	 }

	AVANTI:
	adb_LockControl(OFF,0,0,"",OFF);
	return status;
}
*/
//   +-------------------------------------------∏
//   | ADB_INSERT Inserisce un nuovo record      |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_insert(SINT Hadb)
{
	BTI_WORD dataLen;
	BTI_SINT status;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("insert",Hadb);

	rifai:
	if (ADB_info[Hadb].fBlobPresent) dataLen=adb_BlobNewOfs(Hadb,0);
									 else
									 dataLen=ADB_info[Hadb].AdbHead->wRecSize;
	status = BTRV( B_INSERT,
				   ADB_info[Hadb].PosBlock,
				   ADB_info[Hadb].RecBuffer,// Puntatore ai dati
				   &dataLen, // Grandezza dei dati
				   ADB_info[Hadb].KeyBuffer,   // ???> in output
				   0); // ??? in input

	if (status==5) goto AVANTI;

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {if (adb_LockControl(ON,Hadb,status,"Vietato l'inserimento.",OFF)) goto rifai;
	 }
	 else
	 {
	 if (status) {BTI_err(ADB_info[Hadb].FileName,"INSERT:",status);}
	 }

	AVANTI:
	adb_LockControl(OFF,0,0,"",OFF);
	return status;
}

//	 FldPtr=adb_AFldInfo(Hook[a].NomeFld,&Fld);

//   +-------------------------------------------∏
//   | ADB_FldPtr Ritorna il puntatore al campo  |
//   |                                           |
//   |  Nome : nome del campo da cercare         |
//   |  Field: pt a struttura ADB_REC            |
//   |                                           |
//   |  Ritorna Ptr campo nel record             |
//   |          NULL il campo non esiste         |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
void * adb_FldPtr(SINT Hadb,CHAR *NomeFld)
{
	void *p;
	struct ADB_REC *Fld; 
//	TCHAR serv[80];
	p=adb_FldInfo(Hadb,NomeFld,&Fld);
	if (p==NULL)
	{
		PRG_end("ADb_FldPtr: %s ? in %s",NomeFld,ADB_info[Hadb].FileName);
	}
	return p;
}
static double dDm[]={1,10,100,1000,10000,100000,1000000,100000000};

CHAR *adb_MakeNumeric(double dValore,SINT iSize,SINT iDec,BOOL fView)
{
	static CHAR Cr1[80];
	CHAR Cr2[50];
	CHAR Cr3[20];
	SINT iPrecision; 
	double dpValore;
	double dSegno;
	if (dValore<0) {dpValore=-dValore;dSegno=-1;} else {dpValore=dValore; dSegno=1;}

	//win_infoarg("%f",((dpValore-floor(dpValore))*dDm[iDec]));
	iSize-=1; // 0
	iPrecision=iSize; if (iDec) iPrecision-=(iDec+1); // Tolgo i decimali + il punto
	if (iDec) {  sprintf(Cr2,"%%%d.0f.%%0%dld",iPrecision,iDec);
				 sprintf(Cr3,"%f",((dpValore-floor(dpValore))*dDm[iDec]));
				 sprintf(Cr1,Cr2,floor(dpValore)*dSegno,atol(Cr3));
				 //win_infoarg("[%s][%s]",Cr1,Cr2);
				}
			  else
			  {sprintf(Cr2,"%%%dld",iPrecision);
			   sprintf(Cr1,Cr2,(LONG) dValore);
			  }

//	win_infoarg("%6.2f Cr2=[%s] Cr1=[%s] iSize=%d len=%d",dValore,Cr2,Cr1,iSize,strlen(Cr1));
	if ((SINT) strlen(Cr1)!=iSize) return NULL; 
	return Cr1;
}
// -------------------------------------------------------------------------------------------
// Gestione Cobol DECIMAL
// -------------------------------------------------------------------------------------------

BYTE *adb_MakeCobolDecimal(double dValore,SINT iSize,SINT iDec)
{
  static BYTE Digit[80];
  BYTE Cr1[80];
  BYTE bByte;
  CHAR Cr2[50];
  SINT iSizeReal=(iSize<<1)-1;
  double dpValore;
  SINT pb;

  if (dValore<0) dpValore=-dValore; else dpValore=dValore;
  // *  *
  // dd dn
  sprintf(Cr2,"%%0%d.0f",iSizeReal);
  sprintf(Cr1,Cr2,dpValore*dDm[iDec]);
 //win_infoarg("%f [%s][%s]",dpValore,Cr1,Cr2);
  if (dValore<0) strcat(Cr1,"="); else strcat(Cr1,"?");
  if ((SINT) strlen(Cr1)!=(iSizeReal+1)) return NULL; 

  //win_infoarg("[%s]",Cr1);

  // ---------------------------------------------------
  // Compatto in digit a 4 bit
  //
  ZeroFill(Digit);
  for (pb=0;pb<(iSize<<1);pb++)
  {
	bByte=Cr1[pb]-'0';
	//win_infoarg("<%d>",(SINT) bByte);
	if (pb%2) Digit[pb>>1]|=(bByte&0xF); else Digit[pb>>1]=bByte<<4; 
	//win_infoarg("Digit %d=[%d]",(SINT) (pb>>1),(SINT) (Digit[pb>>1]));
  }
 return Digit;
}


double adb_GetCobolDecimal(BYTE *lpStr,SINT iSize,SINT iDec)
{
	SINT a;
	BYTE bByte;
	double dSegno;
	SINT iSizeVirtual;
	double Mul;
	double dValore=0;

	bByte=lpStr[iSize-1]&0xF;

	//if (bByte==
	//win_infoarg("Test %d %d %d",(SINT) lpStr[0],(SINT) lpStr[1],(SINT) lpStr[2]);
	if (bByte==0xD) dSegno=-1; else dSegno=1;
	
	iSizeVirtual=(iSize<<1)-1;
	Mul=1;
	dValore=0;
	for (a=(iSizeVirtual-1);a>-1;a--)
	{
		bByte=lpStr[a>>1];
		//win_infoarg("DECOD:a=%d pb=%d Byte=%d [Valore=%d]",a,a>>1,(SINT) bByte,(SINT) (a%2)?(bByte&0xF):(bByte>>4));
	    //if (a%2) bByte>>=4; else bByte&=4;
		bByte=(a%2)?(bByte&0xF):(bByte>>4);
		dValore+=(double) bByte*Mul; Mul*=10;
	}

	dValore/=dDm[iDec];
	dValore*=dSegno;
	return dValore;
}


// -------------------------------------------------------------------------------------------
// Gestione Cobol NUMERIC
// -------------------------------------------------------------------------------------------
//static CHAR *lpCNPositive="{ABCDEFGHI";
//static CHAR *lpCNPositive="0ABCDEFGHI";
static CHAR *lpCNPositive="0ABCDEFGHI";
static CHAR *lpCNNegative="}JKLMNOPQR";

CHAR *adb_MakeCobolNumeric(double dValore,SINT iSize,SINT iDec)
{
 static CHAR Cr1[80];
 CHAR Cr2[50];
 
 double dpValore;
// dValore*=-1; // DA TOGLIERE ASSOLUTAMENTE
 if (dValore<0) dpValore=-dValore; else dpValore=dValore;
 sprintf(Cr2,"%%0%d.0f",iSize);
 sprintf(Cr1,Cr2,dpValore*dDm[iDec]);
 if ((SINT) strlen(Cr1)!=iSize) return NULL; 
 
 // Codifico l'ultimo valore
 //win_infoarg("Cr1Prima[%s]",Cr1);
 if (dValore<0) Cr1[iSize-1]=lpCNNegative[Cr1[iSize-1]-'0'];
				else
				Cr1[iSize-1]=lpCNPositive[Cr1[iSize-1]-'0'];
 //win_infoarg("Cr1Dopo [%s]",Cr1);
 return Cr1;
}


double adb_GetCobolNumeric(BYTE *lpStr,SINT Size,SINT iDec)
{
	CHAR *lpApp;
	CHAR *p;
	CHAR cFind;
	CHAR bCar;
	double dSegno;
	double dValore;

	if (!*lpStr) return 0;

	lpApp=malloc(Size+1);
	memcpy(lpApp,lpStr,Size); lpApp[Size]=0;
	cFind=lpStr[Size-1];
	
	// --------------------------------------
	// Decodifico il carattere negativo
	p=strchr(lpCNNegative,cFind); 
	dSegno=1;
	if (p!=NULL)
	{
		if (cFind==*lpCNNegative) bCar='0'; else bCar=cFind-'J'+'1';
		lpApp[Size-1]=bCar;
		dSegno=-1;
	}

	// --------------------------------------
	// Decodifico il carattere positivo
	if (dSegno==1)
	{
	 p=strchr(lpCNPositive,cFind); 
	 if (p!=NULL)
	 {
		if (cFind==*lpCNPositive) bCar='0'; else bCar=cFind-'A'+'1';
		lpApp[Size-1]=bCar;
	 }
	}

	// --------------------------------------
	// Conversione in double
	dValore=atof(lpApp)/dDm[iDec]; dValore*=dSegno;

	free(lpApp);
	return dValore;
}

//   +-------------------------------------------∏
//   | ADB_FldNume  Ritorna il numero contenuto  |
//   |              in un campo                  |
//   |                                           |
//   |  Hdb  : Handle ADB                        |
//   |  Nome : nome del campo da cercare         |
//   |                                           |
//   |  Ritorna Valore del campo                 |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+

double adb_FldNume(SINT Hadb,CHAR *NomeFld)
{
	struct ADB_REC *Fld;
	CHAR *FldPtr;
	INT16 *lpInt16;
	float *ptFloat;
	LONG  *lpLong;

	FldPtr=adb_FldInfo(Hadb,NomeFld,&Fld);
	if (FldPtr==NULL)
		 {
			PRG_end("FldNume: %s ?[%d]",NomeFld,Hadb);
		 }

	switch (Fld->tipo)
	 {
 		 case ADB_NUME : return atof(FldPtr);
		 case ADB_COBD : return adb_GetCobolDecimal(FldPtr,Fld->RealSize,Fld->tipo2);
		 case ADB_COBN : return adb_GetCobolNumeric(FldPtr,Fld->RealSize,Fld->tipo2);

		 case ADB_FLAG :
		 case ADB_INT  : lpInt16=(INT16 *) FldPtr;
						 return (double) *lpInt16;
		 case ADB_INT32: return (double) * (SINT *) FldPtr;

		 case ADB_FLOAT: ptFloat=(float *) FldPtr;
						 return (double) *ptFloat;
	
		 case ADB_AINC:  lpLong=(LONG *) FldPtr;
						 return (double) *lpLong;
		 
		 case ADB_ALFA :
		 case ADB_BLOB :
		 case ADB_DATA :
		 default:
			PRG_end("FldNume:Field Non numerico %s ?[%d] [%d]",NomeFld,Hadb,Fld->tipo);
		 }
 return 0;
}

BOOL adb_FldImport(HDB hdbDest,HDB hdbSource,BOOL fRealloc)
{
	SINT a;
	CHAR *pt;
	struct ADB_HEAD *HeadDest;
	struct ADB_REC *RecordDest;
	struct ADB_REC *FldSource;
	CHAR *pdata;

	HeadDest=ADB_info[hdbDest].AdbHead;
	RecordDest=(struct ADB_REC *) (HeadDest+1);

	// ---------------------------------------------------------
	// Loop sul dizionario campi dell'Hdb di destinazione
	//
	
	for (a=0;a<HeadDest->Field;a++) 
	{
		pt=adb_FldInfo(hdbSource,RecordDest[a].desc,&FldSource);
		// ------------------------------------------------------
		// Nuovo Nome campo inesistente nel file sorgente
		//
		if (pt==NULL) 
		{
			adb_FldReset(hdbDest,RecordDest[a].desc);
		}
		else
		{
		 switch (RecordDest[a].tipo)
		 {
			// da ALFA --> ALFA
			// da BLOB --> ALFA
			case ADB_ALFA: // <--- Destinazione
			
				// Tipi differenti
				if (FldSource->tipo!=RecordDest[a].tipo&&(FldSource->tipo!=ADB_BLOB)) return TRUE; // Campi differenti	
				
				// Se Ë' una stringa la ridimendiono se Ë il caso
				{
				  CHAR *lpSorg;
				  if (FldSource->tipo==ADB_BLOB) lpSorg=adb_BlobGetAlloc(hdbSource,RecordDest[a].desc);
												 else 
												 lpSorg=adb_FldPtr(hdbSource,RecordDest[a].desc);

			      // Taglio se Ë troppo lungo
				  if ((SINT) strlen(lpSorg)>adb_FldSize(hdbDest,RecordDest[a].desc))
				  {
					*(lpSorg+adb_FldSize(hdbSource,RecordDest[a].desc)-1)=0;
				  }

				 adb_FldWrite(hdbDest,RecordDest[a].desc,lpSorg,0);
				 if (FldSource->tipo==ADB_BLOB) adb_BlobFree(lpSorg);
				}
				//win_infoarg("Ci passo [%s] %d->%d",RecordDest[a].desc,FldSource->tipo,RecordDest[a].tipo);
				break;
			
			// da BLOB --> BLOB
			// da ALFA --> BLOB
			case ADB_BLOB: // Tipo destinazione
				
				// Tipi differenti
				if (FldSource->tipo!=RecordDest[a].tipo&&(FldSource->tipo!=ADB_ALFA)) return TRUE; // Campi differenti	
				{
				  CHAR *lpSorg;
				  if (FldSource->tipo==ADB_BLOB) lpSorg=adb_BlobGetAlloc(hdbSource,RecordDest[a].desc);
												 else 
												 lpSorg=adb_FldPtr(hdbSource,RecordDest[a].desc);
				
				   adb_FldWrite(hdbDest,RecordDest[a].desc,lpSorg,0);
				   if (FldSource->tipo==ADB_BLOB) adb_BlobFree(lpSorg);
				}
				break;

			case ADB_DATA:
				// Tipi differenti
				if (FldSource->tipo!=RecordDest[a].tipo)
				{
					return TRUE; // Campi differenti
				}	
				pdata=adb_FldPtr(hdbDest,RecordDest[a].desc);
				memcpy(pdata,adb_FldPtr(hdbSource,RecordDest[a].desc),8);
				*(pdata+8)=0; // 0 Finale
				break;
				
			case ADB_INT:
			case ADB_INT32:
			case ADB_FLAG:
			case ADB_NUME:
			case ADB_COBN:
			case ADB_COBD:
			case ADB_AINC:

				// Tipi differenti
				if ((FldSource->tipo!=ADB_INT)&&
					(FldSource->tipo!=ADB_INT32)&&
					(FldSource->tipo!=ADB_FLAG)&&
					(FldSource->tipo!=ADB_NUME)&&
					(FldSource->tipo!=ADB_COBN)&&
					(FldSource->tipo!=ADB_COBD)&&
					(FldSource->tipo!=ADB_AINC))
					{
						return TRUE; // Campi differenti
					}	
				adb_FldWrite(hdbDest,RecordDest[a].desc,"",adb_FldNume(hdbSource,RecordDest[a].desc));
				break;

				
			case ADB_FLOAT:
				// Tipi differenti
				if (FldSource->tipo!=RecordDest[a].tipo)
				{
					return TRUE; // Campi differenti
				}	
				memcpy(adb_FldPtr(hdbDest,RecordDest[a].desc),adb_FldPtr(hdbSource,RecordDest[a].desc),4);
				break;
		 } // Switch
		} // Else
	}// For
 return FALSE;
}



//   +-------------------------------------------∏
//   | ADB_FldCopy Copia in un buffer un dato    |
//   |             convertito ADB                |
//   |                                           |
//   |  Buff : Buffer che riceverÖ il dato       |
//   |  Hdb  : Handle dbase interessato          |
//   |  Nome : Nome del campo da convertire      |
//   |  DatoAlfa : Dato da scrivere alfanumerico |
//   |  DatnNume : Dato da scrivere numerico     |
//   |                                           |
//   |  Ritorna NULLA                            |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1997 |
//   +-------------------------------------------+
void adb_FldCopy(void *FldPtr,SINT Hadb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume)
{
	struct ADB_REC *Fld;
	INT16  *lpInt16;
	CHAR   *ptr,*pp;
	float  *ptFloat;
	CHAR cr1[80],cr2[80];
	LONG   *lpLong;

	pp=adb_FldInfo(Hadb,NomeFld,&Fld);
	if (pp==NULL)
		{PRG_end("FldCopy:Field ? [%s]",NomeFld);
		 //PRG_end(serv);
		 }

	memset(FldPtr,0,Fld->RealSize); // Azzera il campo

	switch (Fld->tipo)
	 {
		 case ADB_ALFA:
		// case ADB_BLOB:
						if (strlen(DatoAlfa)>(WORD) Fld->size)
										 {PRG_end("FldCopy: hdb:%d > Field\n%s|%s[%d:%d]",
										  Hadb,
										  DatoAlfa,NomeFld,
										  strlen(DatoAlfa),(WORD) Fld->size);
											//win_errgrave(serv);
										 }
						strcpy(FldPtr,DatoAlfa);
						break;

		 case ADB_DATA : if (strlen(DatoAlfa)==0) {memset(FldPtr,0,9); break;}
//										 if (strlen(DatoAlfa)!=Fld->size) win_errgrave("FldCopy:Input != DataField");
						 if (strlen(DatoAlfa)!=8)
								{PRG_end("FldCopy:Input != 8 \n[%d:%s|%s]",Hadb,NomeFld,DatoAlfa);
									//win_errgrave(serv);
								}
																					//win_errgrave("FldCopy:Input != 8");
						 strcpy(cr1,DatoAlfa);
						 memcpy(cr2,cr1+4,4);// Anno
						 memcpy(cr2+4,cr1+2,2);// Mese
						 memcpy(cr2+6,cr1,2);// Giorno
						 cr2[8]=0;
						 memcpy(FldPtr,cr2,9);
						 break;

		 case ADB_NUME : ptr=adb_MakeNumeric(DatoNume,Fld->RealSize,Fld->tipo2,FALSE);
						 if (ptr==NULL) PRG_end("FldCopy:MakeNume!=Fld.Size [%s=%.3f/%d/%d] Hdb=%d",
												 NomeFld,DatoNume,Fld->RealSize,Fld->tipo2,Hadb);
						 strcpy(FldPtr,ptr); 
						 break;

		 case ADB_COBD : ptr=adb_MakeCobolDecimal(DatoNume,Fld->RealSize,Fld->tipo2);
						 if (ptr==NULL) PRG_end("FldCopy:IptSize!=Fld.Size (CobolDecimal)");
						 memcpy(FldPtr,ptr,Fld->RealSize); 
						 break;

		 case ADB_COBN : ptr=adb_MakeCobolNumeric(DatoNume,Fld->RealSize,Fld->tipo2);
						 if (ptr==NULL) PRG_end("FldCopy:IptSize!=Fld.Size (CobolNumeric)");
						 memcpy(FldPtr,ptr,Fld->RealSize); 
						 break;
		 
		 case ADB_AINC:  // AutoIncremento
						 lpLong=(LONG *) FldPtr;
						 *lpLong=(LONG) DatoNume;
						 break;

		 case ADB_FLAG :
		 case ADB_INT  : lpInt16=(INT16 *) FldPtr;
						 *lpInt16=(INT16) DatoNume;
						 break;

		 case ADB_INT32: * (UINT *) FldPtr=(UINT) DatoNume;
		                 break;

		 case ADB_FLOAT: ptFloat=(float *) FldPtr;
						 *ptFloat=(float) DatoNume;
						 break;
		 }
}

//   +-------------------------------------------∏
//   | ADB_FldWrite Scrive (se si puï) un campo  |
//   |              del record                   |
//   |                                           |
//   |  Nome : nome del campo da scriviere       |
//   |  Field: pt a struttura ADB_REC            |
//   |  DatoAlfa : Dato da scrivere alfanumerico |
//   |  DatnNume : Dato da scrivere numerico     |
//   |                                           |
//   |  Ritorna Ptr campo nel record             |
//   |          NULL il campo non esiste         |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
void adb_FldWrite(SINT Hadb,CHAR *NomeFld,CHAR *DatoAlfa,double DatoNume)
{
	struct ADB_REC *Fld;
	CHAR *FldPtr;

	FldPtr=adb_FldInfo(Hadb,NomeFld,&Fld);
	if (FldPtr==NULL) {PRG_end("FldWrite:Field ? %d:[%s]",Hadb,NomeFld);}

	if (Fld->tipo==ADB_BLOB)
	{
		if (!DatoAlfa) return;
		adb_BlobFldWrite(Hadb,NomeFld,DatoAlfa); 
		return;
	}
	adb_FldCopy(FldPtr,Hadb,NomeFld,DatoAlfa,DatoNume);
}

//   +-------------------------------------------∏
//   | ADB_FldOffset Ritorna l'offset di spiazza |
//   |               mento del campo nel record  |
//   |                                           |
//   |  Nome : nome del campo da cercare         |
//   |  Field: pt a struttura ADB_REC            |
//   |                                           |
//   |  Ritorna Offset campo nel record          |
//   |          PrgEnd il campo non esiste       |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_FldOffset(SINT Hadb,CHAR *NomeFld)
{
	struct ADB_REC *Fld;

	if (adb_FldInfo(Hadb,NomeFld,&Fld)==NULL)
		{PRG_end("FldOffset:Campo inesistente [hdb:%d] %s",Hadb,NomeFld);
		//PRG_end(serv);
		}
	return Fld->pos;
}
//   +-------------------------------------------∏
//   | ADB_FldSize   Ritorna il size reale       |
//   |               del campo                   |
//   |                                           |
//   |  Nome : nome del campo da cercare         |
//   |  Field: pt a struttura ADB_REC            |
//   |                                           |
//   |  Ritorna Grandezza del campo nel record   |
//   |          NULL il campo non esiste         |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_FldSize(SINT Hadb,CHAR *NomeFld)
{
	struct ADB_REC *Fld;
	if (adb_FldInfo(Hadb,NomeFld,&Fld)==NULL) PRG_end("FldSize:Campo inesistente[%s]",NomeFld);
	return Fld->RealSize;
}
//   +-------------------------------------------∏
//   | ADB_FldReset  Pulisce un campo nome       |
//   |                                           |
//   |  Hdb : Numero del adb handle              |
//   |  NomeField : Nome del campo da cercare    |
//   |                                           |
//   |  Ritorna 0  = Tutto OK                    |
//   |          -1 = il campo non esiste         |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_FldReset(SINT Hadb,CHAR *NomeFld)
{
	struct ADB_REC *Fld;
	CHAR *FldPtr;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("FldReset",Hadb);

	FldPtr=adb_FldInfo(Hadb,NomeFld,&Fld);
	if (FldPtr==NULL) return -1;

	memset(FldPtr, 0, Fld->RealSize); // Pulisce il record
	return 0;
}
//   +-------------------------------------------∏
//   | ADB_FldInfo   Cerca un campo nome         |
//   |                                           |
//   |  Hdb : Numero del adb handle              |
//   |  NomeField : Nome del campo da cercare    |
//   |  Fldinfo: pt a un *ADB_REC                |
//   |           ritorna pt ad area info campo   |
//   |                                           |
//   |  Ritorna Ptr campo nel record             |
//   |          NULL il campo non esiste         |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
CHAR *adb_FldInfo(SINT Hadb,CHAR *NomeFld,struct ADB_REC **FldInfo)
{

	struct ADB_REC *Field;
	CHAR *ptr;
	SINT a;
//	FILE *pf;

	if (((Hadb<0) || (Hadb>=ADB_max)) || (ADB_info[Hadb].HdlRam==-1))
		//adb_errgrave("FldInfo",Hadb);
		PRG_end("FldInfo [%s][Hdb%d",NomeFld,Hadb);

	Field=(struct ADB_REC *) farPtr((CHAR *) ADB_info[Hadb].AdbHead,sizeof(struct ADB_HEAD));

	for (a=0;a<ADB_info[Hadb].AdbHead->Field;a++)
	{
	 //		Trovato
//		if (!strcmp(Field[a].desc,strupr(NomeFld)))
		if (!strcmp(Field[a].desc,NomeFld))
		{
			*FldInfo=Field+a;
			ptr=farPtr(ADB_info[Hadb].RecBuffer,Field[a].pos);
			return ptr;
		}
	}

/*
	pf=fopen("adb.inf","wb+");
	for (a=0;a<ADB_info[Hadb].AdbHead->Field;a++)
	{
	 fprintf(pf,"%d)%d/%d [%s]\n",Hadb,a,ADB_info[Hadb].AdbHead->Field,Field[a].desc);
	}
	fclose(pf);
*/
	return NULL;
}

//   +-------------------------------------------∏
//   | ADB_UPDATE Modifica il record corrente    |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |         -1 = Errore in lettura vecchio rec|
//   |         -1 = Errore in lettura vecchio rec|
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
/*
SINT adb_update(SINT Hadb)
{
	BTI_WORD dataLen;
	BTI_SINT status;
	SINT   HdlNuovo;
	CHAR *RecNuovo;
	SINT   HdlVecchio;
	CHAR *RecVecchio;
	CHAR *PtRecord;
	SINT SizeRec;
	LONG  Hrec;
	SINT FlagSi=OFF;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("update",Hadb);

	PtRecord=adb_recbuf(Hadb);

	//  --------------------------------------------------------------------
	//   SE ESISTE LA GESTIONE LINK CONTROLLO CHE NON ESISTANO CAMPI-CLONI !
	//  --------------------------------------------------------------------
	FlagSi=adb_HLsonopadre(Hadb);

	if (FlagSi)
	 {
		// Chiede la memoria per il backup
		SizeRec=adb_recsize(Hadb);
		HdlNuovo=memo_chiedi(RAM_HEAP,SizeRec,"updateNEW");
		if (HdlNuovo<0) PRG_end("update NEW :MEMO ?");

		HdlVecchio=memo_chiedi(RAM_HEAP,SizeRec,"updateOLD");
		if (HdlVecchio<0) PRG_end("update OLD :MEMO ?");

		RecNuovo=memo_heap(HdlNuovo);
		RecVecchio=memo_heap(HdlVecchio);

		// Backup del nuovo record
		memcpy(RecNuovo,PtRecord,SizeRec);

		// Backup del vecchio record
		adb_position(Hadb,&Hrec);
		if (adb_get(Hadb,Hrec,-1)) return -1;
		memcpy(RecVecchio,PtRecord,SizeRec);

		// Ripristina i dati da Updatare (se si puï dire)
		memcpy(PtRecord,RecNuovo,SizeRec);
		memo_libera(HdlNuovo,"updateNEW");
	 }

	rifai:
	dataLen=ADB_info[Hadb].AdbHead->RecSize;

	status = BTRV( B_UPDATE,
								 ADB_info[Hadb].PosBlock,
								 ADB_info[Hadb].RecBuffer,// Puntatore ai dati
								 &dataLen, // Grandezza dei dati
								 ADB_info[Hadb].KeyBuffer,   // ???> in output
								 0); // ??? in input

	if (status==5) return status;

//	if (status) {BTI_err(ADB_info[Hadb].FileName,"UPDATE:",status);}

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {if (adb_LockControl(ON,Hadb,status,"Vietato l'update.",OFF)) goto rifai;
	 }
	 else
	 {
	 if (status) {BTI_err(ADB_info[Hadb].FileName,"UPDATE:",status);}
	 }

	adb_LockControl(OFF,0,0,"",OFF);

	// -------------------------------------------------
	// Se i dati sono stati updeitati                  !
	// li modifica in tutti i campi-clone              !
	// -------------------------------------------------

	if (FlagSi)
	{
		adb_HLaction(Hadb,RecVecchio);

		memo_libera(HdlVecchio,"updateOLD");
	}
	return status;
}
*/
//   +-------------------------------------------∏
//   | ADB_UPDATE Modifica il record corrente    |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |         -1 = Errore in lettura vecchio rec|
//   |         -1 = Errore in lettura vecchio rec|
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_update(SINT Hadb)
{
	BTI_WORD dataLen;
	BTI_SINT status;
	SINT   HdlNuovo;
	CHAR *RecNuovo;
	SINT   HdlVecchio;
	CHAR *RecVecchio;
	CHAR *PtRecord;
	SINT SizeRec;
	LONG  Hrec;
	SINT FlagSi=OFF;

	if (((Hadb<0)||(Hadb>=ADB_max))||(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("update",Hadb);

	PtRecord=adb_recbuf(Hadb);

	//  --------------------------------------------------------------------
	//   SE ESISTE LA GESTIONE LINK CONTROLLO CHE NON ESISTANO CAMPI-CLONI !
	//  --------------------------------------------------------------------
	FlagSi=adb_HLsonopadre(Hadb);

	if (FlagSi)
	 {
		// Chiede la memoria per il backup
		SizeRec=adb_recsize(Hadb);
		HdlNuovo=memo_chiedi(RAM_HEAP,SizeRec,"updateNEW");
		if (HdlNuovo<0) PRG_end("update NEW :MEMO ?");

		HdlVecchio=memo_chiedi(RAM_HEAP,SizeRec,"updateOLD");
		if (HdlVecchio<0) PRG_end("update OLD :MEMO ?");

		RecNuovo=memo_heap(HdlNuovo);
		RecVecchio=memo_heap(HdlVecchio);

		// Backup del nuovo record
		memcpy(RecNuovo,PtRecord,SizeRec);

		// Backup del vecchio record
		adb_position(Hadb,&Hrec);
		if (adb_get(Hadb,Hrec,-1)) return -1;
		memcpy(RecVecchio,PtRecord,SizeRec);

		// Ripristina i dati da Updatare (se si puï dire)
		memcpy(PtRecord,RecNuovo,SizeRec);
		memo_libera(HdlNuovo,"updateNEW");
	 }

	rifai:
//	dataLen=ADB_info[Hadb].AdbHead->RecSize;
	if (ADB_info[Hadb].fBlobPresent) dataLen=adb_BlobNewOfs(Hadb,0);
									 else
									 dataLen=ADB_info[Hadb].AdbHead->wRecSize;

	status = BTRV( B_UPDATE,
				   ADB_info[Hadb].PosBlock,
				   ADB_info[Hadb].RecBuffer,// Puntatore ai dati
				   &dataLen, // Grandezza dei dati
				   ADB_info[Hadb].KeyBuffer,   // ???> in output
				   0); // ??? in input

	if ((status==5)||
	    (status==46)) // Accesso negato al file
	    return status;

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {if (adb_LockControl(ON,Hadb,status,"Vietato l'update.",OFF)) goto rifai;
	 }
	 else
	 {
	 if (status) {BTI_err(ADB_info[Hadb].FileName,"UPDATE:",status);}
	 }

	adb_LockControl(OFF,0,0,"",OFF);

	// -------------------------------------------------
	// Se i dati sono stati updeitati                  !
	// li modifica in tutti i campi-clone              !
	// -------------------------------------------------

	if (FlagSi)
	{
		adb_HLaction(Hadb,RecVecchio);
		memo_libera(HdlVecchio,"updateOLD");
	}

	return status;
}

//   +-------------------------------------------∏
//   | ADB_DELETE Cancella il record corrente    |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |         -1 = Non cancellabile perchä usato|
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_delete(SINT Hadb)
{
	BTI_WORD dataLen;
	BTI_SINT status;
	CHAR *p;

	if (ADB_HL&&(adb_HLink>0))
		 {p=adb_HLaction(Hadb,NULL); // Testa l'esistenza di un figlio riconosciuto
			if (p!=NULL)
			 {//win_btierr("CANCELLAZIONE RECORD ANNULLATA:\nIl record ä collegato con altri archivi.",p);
			  return -1;
			 }
		 }

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("delete",Hadb);

	rifai:
	if (ADB_info[Hadb].fBlobPresent) dataLen=adb_BlobNewOfs(Hadb,0);
									 else
									 dataLen=ADB_info[Hadb].AdbHead->wRecSize;
	status = BTRV( B_DELETE,
				   ADB_info[Hadb].PosBlock,
				   0,
				   &dataLen, // Grandezza dei dati
				   0,
				   0);


	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {if (adb_LockControl(ON,Hadb,status,"Vietata la cancellazione.",OFF)) goto rifai;
	 }
	 else
	 {
	 if (status) {BTI_err(ADB_info[Hadb].FileName,"DELETE:",status);}
	 }
	adb_LockControl(OFF,0,0,"",OFF);
	return status;
}
//  +-------------------------------------------------------+
//  | adb_deleteNoLink										|
//	| Cancella il record corrente senza controllare i Link	|
//  |														|
//  |  Hadb : handle del file ADB							|
//  |														|
//  |							   by Lion Informatica 2001 |
//  +-------------------------------------------------------+
void adb_deleteNoLink(SINT Hadb)
{
	BTI_WORD dataLen;
	BTI_SINT status;

	if (((Hadb<0) || (Hadb>=ADB_max)) || (ADB_info[Hadb].HdlRam==-1))
		adb_errgrave("delete",Hadb);

	rifaiNoLink:
	if (ADB_info[Hadb].fBlobPresent) dataLen=adb_BlobNewOfs(Hadb,0);
									 else
									 dataLen=ADB_info[Hadb].AdbHead->wRecSize;

	status = BTRV( B_DELETE,
				   ADB_info[Hadb].PosBlock,
				   0,
				   &dataLen, // Grandezza dei dati
				   0,
				   0);


	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	{
		if (adb_LockControl(ON,Hadb,status,"Vietata la cancellazione.",OFF))
			goto rifaiNoLink;
	}
	else
	{
		if (status)
			BTI_err(ADB_info[Hadb].FileName,"DELETE:",status);
	}
	
	adb_LockControl(OFF,0,0,"",OFF);
	return;
}

//   +-------------------------------------------∏
//   | adb_indexnum                              |
//   | Ritorna il numero di un indice "nome" 		 |
//   |                                           |
//   |  Hadb :  handle del file ADB							 |
//   |  idx  :  Nome dell'indice                 |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |         -1 = Non esiste                   |
//   |         -9 = Hadb non valido              |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+

SINT adb_indexnum(SINT Hadb,CHAR *IndexNome)
{
	SINT a;
	struct ADB_INDEX *AdbIndex;
	SINT IndexNum;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("indexnum",Hadb);

	AdbIndex=(struct ADB_INDEX *) farPtr((CHAR *)ADB_info[Hadb].AdbHead,
						sizeof(struct ADB_HEAD)+
						sizeof(struct ADB_REC)*ADB_info[Hadb].AdbHead->Field);

	IndexNum=-1;

	//printf("Indici predesenti: %d\n\r",ADB_info[Hadb].AdbHead->Index);
	for (a=0;a<ADB_info[Hadb].AdbHead->Index;a++)
	{
	 //printf("%s=%s? %s\n\r",AdbIndex[a].IndexName,
													//IndexNome,
													//AdbIndex[a].IndexDesc);
	 if (!strcmp(AdbIndex[a].IndexName,IndexNome)) {IndexNum=a; break;}
	}

	if (IndexNum<0) return -1;
	return IndexNum;
}

//   +-------------------------------------------∏
//   | ADB_POSITION  Ritorna la posizione fisica |
//   |               del record nel file btrieve |
//   |                                           |
//   |  Hadb :  Numero dell'indice               |
//   |  rec:  Pt al long che conterrÖ la posiz.  |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_position(SINT Hadb,LONG *position)
{
	BTI_WORD dataLen=4;
	SINT status;
	LONG posizione;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("position",Hadb);

	rifai:
	status = BTRV( B_GET_POSITION,
				   ADB_info[Hadb].PosBlock,
				   &posizione,
				   &dataLen,
				   0,
				   0);

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {if (adb_LockControl(ON,Hadb,status,"Vietato il position.",OFF)) goto rifai;
	 }
	 else
	 {
	 if (status) {BTI_err(ADB_info[Hadb].FileName,"POSIT:",status);}
	 }

	adb_LockControl(OFF,0,0,"",OFF);

	*position=posizione;

	//if (status) BTI_err(ADB_info[Hadb].FileName,"POSITION",status);
	return status;
}
//   +-------------------------------------------∏
//   | ADB_GET     Legge direttamente dal file   |
//   |             con la posizione fisica       |
//   |                                           |
//   |  Hadb :  Numero dell'indice               |
//   |  rec:  Pt al long che conterrÖ la posiz.  |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_get(SINT Hadb,HREC position,SINT IndexNum)
{
	BTI_WORD dataLen;
	SINT status;
	HREC *posizione;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("adbget",Hadb);

	posizione=(HREC *) ADB_info[Hadb].RecBuffer;
	*posizione=position;

	rifai:
	dataLen=ADB_info[Hadb].AdbHead->wRecSize;
	status = BTRV( B_GET_DIRECT,
				   ADB_info[Hadb].PosBlock,
				   ADB_info[Hadb].RecBuffer, // Record dove andare
				   &dataLen,// Lunghezza buffer
				   ADB_info[Hadb].KeyBuffer,
				   (WORD) IndexNum);

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	{ if (adb_LockControl(ON,Hadb,status,"Vietato la lettura.",OFF)) goto rifai;}
	 else
	 {
	 if (status) 
		{
		 BTI_err(ADB_info[Hadb].FileName,"GET:",status);
		}
	 }

//	AVANTI:
	adb_LockControl(OFF,0,0,"",OFF);

//	if (status) BTI_err(ADB_info[Hadb].FileName,"ADBGET:",status);
	return status;
}

//   +-------------------------------------------∏
//   | ADB_GETALLOC                              |
//   | Legge ed alloca il record in un nuovo     |
//   | Utile per letture in Multi-Thread         |
//   |                                           |
//   |  Hadb :  Numero dell'indice               |
//   |  rec:  Pt al long che conterrÖ la posiz.  |
//   |                                           |
//   |  Ritorna 0 = Il puntatore al nuovo record |
//   |          NULL Errore                      |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+

BYTE *adb_GetAlloc32(SINT Hadb,HREC position)
{
	BTI_WORD dataLen;
	SINT status;
	BYTE *lpPosBlock;
	BYTE *lpBuffer;
	HREC *posizione;

	// Creo copia del blocco del dbase
	lpPosBlock=malloc(255);
	memcpy(lpPosBlock,ADB_info[Hadb].PosBlock,255);
	lpBuffer=malloc(ADB_info[Hadb].AdbHead->wRecSize);

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("adbget",Hadb);

	posizione=(HREC *) lpBuffer;
	*posizione=position;

	dataLen=ADB_info[Hadb].AdbHead->wRecSize;
	status = BTRV( B_GET_DIRECT,
				   lpPosBlock,
				   lpBuffer, // Record dove andare
				   &dataLen,// Lunghezza buffer
				   ADB_info[Hadb].KeyBuffer,
				   (WORD) -1);
	free(lpPosBlock);

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status)
	{ 
	 free(lpBuffer);
	 lpBuffer=NULL;
	 }

	return lpBuffer;
}

//   +-------------------------------------------∏
//   | ADB_FIND  Cerca una chiave nell'indice		 |
//   |                                           |
//   |  Hadb : Handle del file ADB               |
//   |  idx  : Numero dell'indice da usare       |
//   |  (idx  Nome dell'indice per Afind)        |
//   |  key  : Chiave da cercare                 |
//   |  modo : modo BTRIEVE di ricerca           |
//   |                                           |
//   |        B_GET_EQUAL se uguale              |
//   |        B_GET_GT    se ä + grande          |
//   |        B_GET_GE    se ä + grande o uguale |
//   |        B_GET_LT    se ä minore            |
//   |        B_GET_LE    se + minore o uguale   |
//   |                                           |
//   |  flag: B_KEY o B_RECORD (valore ritorno)  |
//   |        Se si vuole solo chiave (veloce)   |
//   |        o tutto il record dati             |
//   |  rec:  Puntatore al record/chiave         |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_Afind(SINT Hadb,CHAR *IndexNome,SINT modo,void *keyfind,SINT flag)
{
	SINT IndexNum;
	CHAR szServ[200];

	strcpy(szServ,fileName(ADB_info[Hadb].FileName));
	IndexNum=adb_indexnum(Hadb,IndexNome);
	if (IndexNum<0) PRG_end("adb_Afind():Nome indice non valido [%s ? in db:%s (%d)]",IndexNome,szServ,Hadb);
	return adb_find(Hadb,IndexNum,modo,keyfind,flag);
}

SINT adb_find(SINT Hadb,SINT IndexNum,SINT modo,void *keyfind,SINT flag)
{
	BTI_WORD dataLen;
	BTI_SINT status;

	//SINT lk;
	SINT solochiave;
	struct ADB_INDEX *AdbIndex;

	CHAR serv[200];
	void *PtChiave;

	CHAR *tiporic[]=
	 {"FIND_EQUAL:",
		"FIND_NEXT:",
		"FIND_PREVIOUS:",
		"FIND_GT:",
		"FIND_GE:",
		"FIND_LT:",
		"FIND_LE:",
		"FIND_FIRST:",
		"FIND_LAST:"};

	if (((Hadb<0)||(Hadb>=ADB_max))||(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("find",Hadb);

	if ((modo<B_GET_EQUAL)||(modo>B_GET_LAST))
			win_errgrave("Adb_find:Modo di ricerca non valido");

	AdbIndex=(struct ADB_INDEX *) farPtr((CHAR *) ADB_info[Hadb].AdbHead,
										 sizeof(struct ADB_HEAD)+
										 sizeof(struct ADB_REC)*ADB_info[Hadb].AdbHead->Field);

	// Copia la chiave nel buffer
	if ((modo!=B_GET_FIRST)&&
			(modo!=B_GET_LAST)&&
			(modo!=B_GET_NEXT)&&
			(modo!=B_GET_PREVIOUS))
			{
				/*
				lk=strlen(keyfind);
				if (!(flag&B_NOSTR)) // Se ä una stringa
				{
					if ((lk<0)||(lk>ADB_info[Hadb].AdbHead->KeyMoreGreat))
					win_errgrave("Adb_find:Chiave di ricerca non valida");
				}
					*/
			 if (flag&B_NOSTR)
				 {PtChiave=keyfind;}
				 else
				 {if (strlen(keyfind)>sizeof(serv)) PRG_end("adb_find:serv memo");
					memset(serv,0,sizeof(serv)); strcpy(serv,keyfind);
					PtChiave=serv;
				 }

				 memcpy(ADB_info[Hadb].KeyBuffer,PtChiave,AdbIndex[IndexNum].KeySize);
			}

	if (flag&B_KEY) solochiave=50; else solochiave=0;

	rifai:
	dataLen=ADB_info[Hadb].AdbHead->wRecSize;
	status = BTRV((WORD) (modo+solochiave),
				 ADB_info[Hadb].PosBlock,
				 ADB_info[Hadb].RecBuffer,    // Puntatore ai dati
				 &dataLen, 										// Grandezza dei dati
				 ADB_info[Hadb].KeyBuffer,    // Ricerca e riceve chiave
				 (WORD) IndexNum); 									// Indice da usare
	//if (!status) return 0;

	if ((status==9)|| // Fine del file
		 (status==4))   // Chiave infondata
			goto AVANTI;
			/*
			else
			BTI_err(ADB_info[Hadb].FileName,tiporic[modo-B_GET_EQUAL],status);
				*/
	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if (status==81)
	 {if (adb_LockControl(ON,Hadb,status,"Vietato la ricerca.",OFF)) goto rifai;
	 }
	 else
	 {
		if (status) {BTI_err(ADB_info[Hadb].FileName,tiporic[modo-B_GET_EQUAL],status);}
	 }

	AVANTI:
	adb_LockControl(OFF,0,0,"",OFF);
	return status;
}

//   +-------------------------------------------∏
//   | ADB_OPEN  Apre un file AdvancedDataBase   |
//   |                                           |
//   |  file : Nome del file da aprire           |
//   |                                           |
//   |  modo   0 : Standard (pag 4.12)           |
//   |        -1 : Accelerato                    |
//   |             Disabilita DataRecovery       |
//   |        -2 . Solo in lettura               |
//   |        -4 : Gestione esclusiva            |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |          -1  problemi sul file ADB        |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+

SINT adb_open(CHAR *file,CHAR *Client,SINT modo,SINT *Hadb)
	{
	SINT a,rt;
	FILE *pf1;
	CHAR ADBBuf[255];
	BTI_BYTE dataBuf[255];
	BTI_WORD dataLen;
	BTI_SINT status;
	SINT Hadbcod=-1,dbhdl;
	CHAR serv[255];
	LONG SizeInfo;
	struct ADB_HEAD AdbHead;

 //	-------------------------------------
 //		Cerca prima area adb disponibile  !
 // -------------------------------------
 
 if (ADB_hdl==-1) win_errgrave("adb_open(): ADBase manager non inizializzato");

 if (ADB_ult>=ADB_max) PRG_end("adb_open(): ADBase full[Ult:%d][Max:%d] ",ADB_ult,ADB_max);

 for (a=0;a<=ADB_max;a++)
	{if (ADB_info[a].HdlRam==-1) {Hadbcod=a;break;}
	}
 if (Hadbcod<0) win_errgrave("adb_open(): Dbase manager inquinato");

 *Hadb=-1;
 //	-----------------------------------
 // calcola la lunghezza del file.ADB !
 // -----------------------------------

	strcpy(ADBBuf,file); strcat(ADBBuf,".ADB");

	SizeInfo=file_len(ADBBuf);
	if ((SizeInfo<0)||(SizeInfo>8000)) return -1;

 //-------------------------------------
 // APRE IL FILE ADB PER DETERMINARE   !
 // LA LUGHEZZA DEL RECORD E KEYBUFFER !
 // ------------------------------------
 pf1=NULL;
 if (f_open(ADBBuf,"rb",&pf1)) {rt=ON; status=-1; goto fine;}
 f_get(pf1,0,&AdbHead,sizeof(AdbHead));
 if (strcmp(AdbHead.id,"Adb1.2 Eh3")) {rt=ON; status=-1; goto fine;}

 // ------------------------------------
 //	    Struttura della memoria ADB    !
 //	                                   !
 //	255 Nome del file                  !
 //	128 PosBlock                       !
 //	<-> Record Buffer                  !
 //	<-> KeyMoreGreat buffer            !
 //	<-> Attributi del file             !
 //	    ADB_HEAD                       !
 //	    ADB_REC * i campi              !
 //	    ADB_INDEX * gli indici         !
 //	    ADB_INDEX_SET sorgente definiz !
 //	                                   !
 // ------------------------------------

 rt=OFF;
	
    ADB_info[Hadbcod].fBlobPresent=FALSE;
	if (AdbHead.wRecSize<1) 
	{AdbHead.wRecSize=50000;//32767;// New2000 per VARCHAR
	 ADB_info[Hadbcod].fBlobPresent=TRUE;
	}
	
	sprintf(serv,"*DB%0d:%s",Hadbcod,fileName(file));
	dbhdl=memo_chiedi(RAM_HEAP,255+128+AdbHead.wRecSize+255+SizeInfo,serv);

//									 fileName(file));
 if (dbhdl<0)
	{sprintf(serv,"adb_open():Memoria insufficiente per aprire :\n%s",file);
	 win_errgrave(serv);
	}

	memset(memo_heap(dbhdl),0,255+128+(LONG) AdbHead.wRecSize+255+(LONG) SizeInfo); // new 2001/10

	ADB_info[Hadbcod].HdlRam=dbhdl;
	ADB_info[Hadbcod].FileName=memo_heap(dbhdl); // Size 255
	ADB_info[Hadbcod].PosBlock=farPtr(memo_heap(dbhdl),255); // Size 128
	ADB_info[Hadbcod].RecBuffer=farPtr(memo_heap(dbhdl),255+128);  // Size RecSize
	ADB_info[Hadbcod].KeyBuffer=farPtr(memo_heap(dbhdl),255+128+AdbHead.wRecSize); // Size 255
	ADB_info[Hadbcod].AdbHead=(struct ADB_HEAD *) farPtr(memo_heap(dbhdl),255+128+AdbHead.wRecSize+255);
	ADB_info[Hadbcod].AdbFilter=NULL;
	ADB_info[Hadbcod].OpenCount=1;// Conteggio delle aperture

	if (f_get(pf1,0,ADB_info[Hadbcod].AdbHead,(WORD) SizeInfo)) {rt=ON;status=-1; goto fine;}
	ADB_info[Hadbcod].AdbHead->wRecSize=AdbHead.wRecSize; // New2000

	strcpy(ADBBuf, file ); strcat(ADBBuf,".BTI");
	strcpy(ADB_info[Hadbcod].FileName,(BTI_CHAR *)ADBBuf);

	rifai:
	*dataBuf=0; if (Client) strcpy(dataBuf,Client);
	dataLen=strlen(dataBuf) + 1;

	status = BTRV( B_OPEN,
				   ADB_info[Hadbcod].PosBlock,// Blocco di riferimento del file
				   dataBuf, //Proprietario
				   &dataLen,// Lunghezza nome del proprietario
				   ADBBuf,  // Nome del file
				   (WORD) modo );  // vedi leggenda

//	if (!fErrorView&&status) {rt=status; goto fine;}

	// --------------------------
	// Controllo dei OWNER      !
	// --------------------------
	if (*dataBuf=='?'&&(status==51)) {rt=ON; goto fine;}
	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if ((status==46)||(status==88)||(status==81)||(status==85))
		{if (adb_LockControl(ON,Hadbcod,status,"Vietato l'accesso al file.",ON)) goto rifai;
		 rt=ON;
		}
		else
		{
		  if (status) {rt=ON;}
		}

	adb_LockControl(OFF,0,0,"",OFF);
	fine:
	if (pf1) f_close(pf1);
	if (rt)
		{sprintf(serv,"*DB%0d:%s",Hadbcod,fileName(file));
		 memo_libera(dbhdl,serv);
		 ADB_info[Hadbcod].HdlRam=-1;
		 //win_infoarg("OPEN [%d] [%s] status=%d",Hadbcod,ADBBuf,status);
	     //PRG_end("");
         if (status) BTI_err(file,"OPEN:",status);
		 if (status==20) PRG_end("Il Btrieve non ä installato");
		 return status;}

	ADB_ult++;
	*Hadb=Hadbcod;
	return 0;
}

/*
SINT adb_open(CHAR *file,CHAR *Client,SINT modo,SINT *Hadb)
	{
	SINT a,rt;
	FILE *pf1;
	CHAR ADBBuf[255];
	BTI_BYTE dataBuf[255];
	BTI_WORD dataLen;
	BTI_SINT status;
	SINT Hadbcod=-1,dbhdl;
	CHAR serv[255];
	LONG SizeInfo;
	struct ADB_HEAD AdbHead;

 //	-------------------------------------
 //		Cerca prima area adb disponibile  !
 // -------------------------------------
 if (ADB_hdl==-1) win_errgrave("adb_open(): ADBase manager non inizializzato");

 if (ADB_ult>=ADB_max) PRG_end("adb_open(): ADBase full[Ult:%d][Max:%d] ",ADB_ult,ADB_max);

 for (a=0;a<=ADB_max;a++)
	{if (ADB_info[a].HdlRam==-1) {Hadbcod=a;break;}
	}
 if (Hadbcod<0) win_errgrave("adb_open(): Dbase manager inquinato");


 //	-----------------------------------
 // calcola la lunghezza del file.ADB !
 // -----------------------------------

	strcpy(ADBBuf, file ); strcat(ADBBuf,".ADB");

	SizeInfo=file_len(ADBBuf);
	if ((SizeInfo<0)||(SizeInfo>5000)) return -1;


 //	------------------------------------
 // APRE IL FILE ADB PER DETERMINARE   !
 // LA LUGHEZZA DEL RECORD E KEYBUFFER !
 // ------------------------------------
 if (f_open(ADBBuf,"rb",&pf1)) {rt=ON; status=-1; goto fine;}
 f_get(pf1,0,&AdbHead,sizeof(AdbHead));
 if (strcmp(AdbHead.id,"Adb1.2 Eh3")) {rt=ON; status=-1; goto fine;}

 // ------------------------------------
 //	    Struttura della memoria ADB    !
 //	                                   !
 //	255 Nome del file                  !
 //	128 PosBlock                       !
 //	<-> Record Buffer                  !
 //	<-> KeyMoreGreat buffer            !
 //	<-> Attributi del file             !
 //	    ADB_HEAD                       !
 //	    ADB_REC * i campi              !
 //	    ADB_INDEX * gli indici         !
 //	    ADB_INDEX_SET sorgente definiz !
 //	                                   !
 // ------------------------------------

 rt=OFF;
	sprintf(serv,"*DB%0d:%s",Hadbcod,fileName(file));
	dbhdl=memo_chiedi(RAM_HEAP,
					  255+128+AdbHead.RecSize+255+SizeInfo,
					  serv);
//									 fileName(file));
 if (dbhdl<0)
	{sprintf(serv,"adb_open():Memoria insufficiente per aprire :\n%s",file);
	 win_errgrave(serv);
	}

	ADB_info[Hadbcod].HdlRam=dbhdl;
	ADB_info[Hadbcod].FileName=memo_heap(dbhdl); // Size 255
	ADB_info[Hadbcod].PosBlock=farPtr(memo_heap(dbhdl),255); // Size 128
	ADB_info[Hadbcod].RecBuffer=farPtr(memo_heap(dbhdl),255+128);  // Size RecSize
	ADB_info[Hadbcod].KeyBuffer=farPtr(memo_heap(dbhdl),255+128+AdbHead.RecSize); // Size 255
	ADB_info[Hadbcod].AdbHead=(struct ADB_HEAD *) farPtr(memo_heap(dbhdl),255+128+AdbHead.RecSize+255);
	ADB_info[Hadbcod].AdbFilter=NULL;
	ADB_info[Hadbcod].OpenCount=1;// Conteggio delle aperture

	if (f_get(pf1,0,ADB_info[Hadbcod].AdbHead,(WORD) SizeInfo)) {rt=ON;status=-1; goto fine;}

	strcpy(ADBBuf, file ); strcat(ADBBuf,".BTI");
	strcpy(ADB_info[Hadbcod].FileName,(BTI_CHAR *)ADBBuf);

	rifai:
	strcpy(dataBuf,Client);
	dataLen=strlen(Client) + 1;

	status = BTRV( B_OPEN,
				   ADB_info[Hadbcod].PosBlock,// Blocco di riferimento del file
				   dataBuf, //Proprietario
				   &dataLen,// Lunghezza nome del proprietario
				   ADBBuf,  // Nome del file
				   (WORD) modo );  // vedi leggenda

	// --------------------------
	// Controllo dei OWNER      !
	// --------------------------
	if (*Client=='?'&&(status==51)) {rt=ON; goto fine;}

	// --------------------------
	// Controllo dei LOCK       !
	// --------------------------
	if ((status==88)||(status==81)||(status==85))
		{if (adb_LockControl(ON,Hadbcod,status,"Vietato l'accesso al file.",ON)) goto rifai;
		 rt=ON;
		}
		else
		{
		  if (status) {rt=ON;}
		}

	adb_LockControl(OFF,0,0,"",OFF);
	fine:
	f_close(pf1);
	if (rt)
		{sprintf(serv,"*DB%0d:%s",Hadbcod,fileName(file));
		 memo_libera(dbhdl,serv);
		 ADB_info[Hadbcod].HdlRam=-1;
		 //win_infoarg("OPEN [%d] [%s] status=%d",Hadbcod,ADBBuf,status);
	     //PRG_end("");
         if (status) BTI_err(file,"OPEN:",status);
		 if (status==20) PRG_end("Il Btrieve non ä installato");
		 return status;}

	ADB_ult++;
	*Hadb=Hadbcod;
	return 0;
}
*/
// +-------------------------------------------∏
// | ADB_CLOSE Chiude un file AdvancedDataBase |
// |                                           |
// |  file : Nome del file da chiudere         |
// |                                           |
// |  Ritorna 0 = Tutto ok                     |
// |          > 0 Errore BTRIEVE               |
// |          < 0 File giÖ chiuso              |
// |                                           |
// |  modo   0 : Standard                      |
// |        -1 : Accelerato                    |
// |             Disabilita DataRecovery       |
// |        -2 . Solo in lettura               |
// |        -3 : In verifica (solo DOS)        |
// |        -4 : Gestione esclusiva            |
// |                                           |
// |            by Ferr‡ Art & Technology 1996 |
// +-------------------------------------------+
SINT adb_close(SINT Hadb)
{
	BTI_SINT status;
	SINT Hdl;

	if (((Hadb<0)||(Hadb>=ADB_max))||(ADB_info[Hadb].HdlRam==-1)) return -9;

//	printf("Entro %d:%d",Hadb,ADB_max); getch();
 
	status=BTRV(B_CLOSE,ADB_info[Hadb].PosBlock,0,0,0,0);
	if (status) {BTI_err(ADB_info[Hadb].FileName,"CLOSE:",status);}

	Hdl=ADB_info[Hadb].HdlRam;
	memo_libera(Hdl,sys.memolist[Hdl].User);
	ADB_info[Hadb].HdlRam=-1; // Libera cella
    ADB_info[Hadb].FileName=NULL;
	ADB_ult--;
	return status;
}

SINT adb_closeall(void)
{
 SINT a;
 //		Chiude tutti i db
 for (a=0;a<ADB_max;a++) adb_close(a);
 return 0;
}


//   +-------------------------------------------∏
//   | BTI_err  Visualizza l'errore btrieve      |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
static BOOL fErrorView=TRUE;
void adb_ErrorView(BOOL fError)
{
 fErrorView=fError;
}

/*
void BTI_err(CHAR *file,CHAR *opera,SINT status)
{
 FILE *pf1;
 CHAR mess[300];
 CHAR buf[40];

 // Errori speciali
 if (status==3014)
 {
  sprintf(mess,"BTRIEVE 6.15:\nFile:%s\nin %s\n%s (status %d)",
							file,opera,"Netware:Versione installata antecedente 6.15",status);
 
  goto ViewMess;
 }
 
	 
 if ((status<0)||(status>107)) PRG_end("BTI_err:Status non gestito [%d]",status);
 if (!fErrorView) return;

 f_open(EHPath("BTI.ERR"),"rb",&pf1);
 f_get(pf1,(36*status)+0,buf,34);
 buf[3]=0;
 buf[33]=0;
 f_close(pf1);
 sprintf(mess,"BTRIEVE 6.15:\nFile:%s\nin %s\n%s (%s)",
							file,opera,&buf[4],buf);
ViewMess:

 win_err(mess);
}
*/
void BTI_err(CHAR *file,CHAR *opera,SINT status)
{
 FILE *pf1;
 CHAR mess[300];
 CHAR buf[80];
 SINT iErr=-1;
 CHAR *p;

 // Errori speciali
 if (status==3014)
 {
  sprintf(mess,"Pervasive.SQL:\nFile:%s\nin %s\n%s (status %d)",
		  file,opera,"Netware:Versione installata antecedente 6.15",status);
 
  goto ViewMess;
 }
 
	 
 if (!fErrorView) return;

 pf1=NULL;
 f_open(EHPath("BTI.ERR"),"rb",&pf1);
 while (TRUE)
 {
	if (fgets(buf,sizeof(buf)-1,pf1)==NULL) break;
	p=strstr(buf,":"); if (p) *p=0;
	//win_infoarg("[%s] %d",buf,status);
	if (atoi(buf)==status) {iErr=status; break;}

 }
 if (pf1) f_close(pf1);
 if (iErr==-1)  {PRG_end("Pervasive.SQL:Status non gestito [%d]",status);}

 sprintf(mess,"Pervasive.SQL:\nFile:%s\nin %s\n%s (%d)",file,opera,p+1,iErr);

ViewMess:
 win_err(mess);
}

//   +-------------------------------------------∏
//   | ADB_Fpos  Cerca la posizione di un campo  |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_Fpos(CHAR *field,struct ADB_REC *RecInfo)
{
 SINT a=0,pos=-1;

 for (;;a++)
 {
	if (RecInfo[a].tipo==STOP) break;
//	if (!strcmp(strupr(RecInfo[a].desc),strupr(field))) {pos=RecInfo[a].pos; break;}
	if (!strcmp(RecInfo[a].desc,field)) {pos=RecInfo[a].pos; break;}
 }
 return pos;
}
//   +-------------------------------------------∏
//   | ADB_Flen  Cerca la lunghezza di un campo  |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_Flen(CHAR *field,struct ADB_REC *RecInfo)
{
 SINT a=0,pos=-1;

 for (;;a++)
 {
	if (RecInfo[a].tipo==STOP) break;
//	if (!strcmp(strupr(RecInfo[a].desc),strupr(field))) {pos=RecInfo[a].size; break;}
	if (!strcmp(RecInfo[a].desc,field)) {pos=RecInfo[a].size; break;}
 }
 return pos;
}
//   +-------------------------------------------∏
//   | ADB_Floc Cerca la posizione di un campo   |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_Floc(CHAR *field,struct ADB_REC *RecInfo)
{
 SINT a=0,pos=-1;

 for (;;a++)
 {
	if (RecInfo[a].tipo==STOP) break;
//	if (!strcmp(strupr(RecInfo[a].desc),strupr(field))) {pos=a; break;}
	if (!strcmp(RecInfo[a].desc,field)) {pos=a; break;}
 }
 return pos;
}

//   +-------------------------------------------∏
//   | ADB_CREA  Crea un file AdvancedDataBase   |
//   |                                           |
//   |  file : Nome del file da creare           |
//   |  desc : Descrizione del contenuto max 30  |
//   |  RecInfo : Struttura ADB_REC              |
//   |            descrizione del record         |
//   |  Idx     : Struttura ADB_INDEX_SET        |
//   |            descrizione degli indici(max24)|
//   |  FlagFile: B_STANDARD = Default           |
//   |            Vedi pag 4-16                  |
//   |                                           |
//   |  Struttura di Link per Hierarchic Dbase   |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+

//int adb_crea(char *file,char *desc,struct ADB_REC *RecInfo,struct ADB_INDEX_SET *idx,int FlagFile)
SINT adb_crea(CHAR *file,CHAR *desc,
			  struct ADB_REC *RecInfo,
			  struct ADB_INDEX_SET *idx,
			  SINT FlagFile)
{ 
 SINT a,b,rt;
 SINT RecSize; 	// Grandezza record
 SINT Field;   	// Numero dei campi
 SINT Index;   	// Numero di indici collegati
 SINT IndexDesc; // Numero descrizioni su indici collegati
 CHAR IndexUlt[ADB_MAX_IDXNAME];
 SINT KeySpec; 	// Numero di specifiche di segmento
 LONG SizeMem;
 SINT BTIhdl;
 SINT PageSize;
 SINT KeyMoreGreat;
 struct FILE_SPECS *fileSpecs;
 struct KEY_SPECS  *keySpecs;
 struct ADB_INDEX  AdbIndex[ADB_MAX_INDEX];
 struct ADB_HEAD  AdbHead;
 SINT KS,flag;
 SINT RealSize,FieldLoc;
 SINT KeyDup=0;
 LONG Physical,MinKeyPage;
 LONG pp,pp2,pp3;
 FILE *pf1;
 const SINT iMaxField=200;

 // Per btrieve       !

 BTI_BYTE ADBBuf[255];
 BTI_BYTE posBlock[128];
 BTI_WORD dataLen;
 BTI_SINT status;
 CHAR serv[100];

 // ------------------------------------------------------
 // Cerca dimensioni e controlla la struttura dei campi  !
 // ------------------------------------------------------
 RecSize=0; Field=0; rt=OFF;
 for (;;)
 {
	if (RecInfo[Field].tipo==STOP) break;
	switch(RecInfo[Field].tipo)
	{
	 case ADB_ALFA : RecSize+=RecInfo[Field].size+1; break;

	 case ADB_NUME : RecSize+=RecInfo[Field].size+1;
					 if (RecInfo[Field].tipo2) RecSize+=RecInfo[Field].tipo2+1;
					 break;
	 case ADB_COBD:  // Cobol Decimal New 2000
					 RecInfo[Field].size=((RecInfo[Field].size+RecInfo[Field].tipo2+1)>>1)+1;
					 //win_infoarg("COBD-Size=%d",RecInfo[Field].size);
					 RecSize+=RecInfo[Field].size;
					 break;

	 case ADB_COBN : // Cobol Numeric New 2000
					 RecInfo[Field].size=RecInfo[Field].size+RecInfo[Field].tipo2;
					 //win_infoarg("COBN-Size=%d",RecInfo[Field].size);
					 RecSize+=RecInfo[Field].size;
					 break;
	 

	 case ADB_DATA : RecSize+=8+1; RecInfo[Field].size=8; break;

	 case ADB_FLAG :
	 case ADB_INT  : RecSize+=2; RecInfo[Field].size=2; break;

	 case ADB_INT32:
	 case ADB_AINC: RecSize+=4; RecInfo[Field].size=4; break;

	 case ADB_FLOAT: RecSize+=4; RecInfo[Field].size=4; break;
	 default: win_errgrave("ADB_CREA:Probabile <tipo campo> in ADB_REC errato");
	}
	a=strlen(RecInfo[Field].desc);
	if ((a<0)||(a>(ADB_MAX_DESC-1))) {rt=ON;break;}
	if (Field++>iMaxField) break;
 }
 Field--;
 if (rt||(Field<1)||(Field++>iMaxField)||(RecSize<0))
 {
#ifdef _WIN32
	win_infoarg("ADB_CREA:Probabile ADB_REC errato Field=%d",Field);
#endif
	win_errgrave("ADB_CREA:Probabile ADB_REC errato");
 }

 //printf("File |%s|\n\r",file);
 //printf("Numero campi : %d (%d)\n\r",Field,RecSize);

 // -----------------------------------------
 //      Calcola le posizione dei campi     !
 // -----------------------------------------
 RecSize=0;
 for (a=0;a<Field;a++)
 {
	RecInfo[a].pos=RecSize;

	//printf("%d, Campo %s %d\n\r",a,RecInfo[a].desc,RecInfo[a].pos);
	if (RecInfo[a].tipo==STOP) break;

	switch(RecInfo[a].tipo)
	{
	 // Aggiungo lo spazio per lo 0
	 case ADB_DATA :  
	 case ADB_ALFA : RealSize=RecInfo[a].size+1; 
					 break;
	 // Calcolo la dimensione + 0 e decimale
	 case ADB_NUME : RealSize=RecInfo[a].size+1;
					 if (RecInfo[a].tipo2) {RealSize+=RecInfo[a].tipo2+1;}
					 break;

	 case ADB_INT  :
	 case ADB_INT32:
	 case ADB_FLAG : 
	 case ADB_FLOAT: 
	 case ADB_COBD :
	 case ADB_COBN :
	 case ADB_AINC :
					RealSize=RecInfo[a].size;
					break;
	}

	if (RealSize<=0) win_errgrave("ADB_CREA:Errore in lunghezza campo");
	RecInfo[a].RealSize=RealSize;

	// ------------------------------------------------------------------
	// Controllo che non esista un'altro campo con lo stesso nome

	for (b=0;b<a;b++)
	 {if (!strcmp(RecInfo[a].desc,RecInfo[b].desc))
				 {
				 sprintf((CHAR *) ADBBuf,"ADB_CREA:Due campi con lo stesso nome:\n%s",RecInfo[a].desc);
				 win_errgrave((CHAR *) ADBBuf);
				 }
	 }

	RecSize+=RealSize;
 }
 //printf("Grandezza record : %d\n\r",RecSize);

 // -----------------------------------------
 //      Controlla gli Indici Collegati     !
 // -----------------------------------------

 Index=0; // Numero di indici collegati
 *IndexUlt=0;
 IndexDesc=0; // Numero di descrizioni degli indici

 rt=OFF;
 for (;;)
 {
	//printf("--%s %s\n\r",idx[IndexDesc].IndexName,idx[IndexDesc].FieldName);
	if (!strcmp(idx[IndexDesc].IndexName,"FINE")) break;
	if (IndexDesc++>iMaxField) break;
	// Sempre lo stesso indice
	if (!strcmp(idx[IndexDesc-1].IndexName,IndexUlt))
		{AdbIndex[Index-1].KeySeg++;
		 continue;}

	a=strlen(idx[IndexDesc-1].IndexName);
	if ((a<0)||(a>(ADB_MAX_IDXNAME-1))) {rt=ON;break;}
	strcpy(IndexUlt,idx[IndexDesc-1].IndexName);

	// Controlla che non esista gia in indice
	for (a=0;a<Index;a++)
	{if (!strcmp(IndexUlt,AdbIndex[a].IndexName)) {rt=1;break;}
	 }
	if (rt) break;

	// Inserisce i dati nel nuovo indice
	strcpy(AdbIndex[Index].IndexName,IndexUlt); // Nome Index
	// Descrizione Index
	strcpy(AdbIndex[Index].IndexDesc,idx[IndexDesc-1].FieldName);
	AdbIndex[Index].KeySeg=0;
	Index++;
	if (Index>=ADB_MAX_INDEX) {rt=1;break;}
 }
 IndexDesc--;
 if (rt||(IndexDesc<1)||(IndexDesc++>iMaxField)) win_errgrave("ADB_CREA:Probabile ADB_INDEX_SET errato");

 // Controllo sul conteggio keysegment
 for (a=0;a<Index;a++)
 {if (AdbIndex[a].KeySeg<1) win_errgrave("ADB_CREA:Errore in ADB_INDEX_SET: KeySegment associati");
 }

 //printf("\n\rNumero indici : %d (Descrizione %d)\n\r",Index,IndexDesc);
 //for (a=0;a<Index;a++) printf("name : %s (Descrizione %s(%d))\n\r",AdbIndex[a].IndexName,AdbIndex[a].IndexDesc,AdbIndex[a].KeySeg);

 KeySpec=IndexDesc-Index; // key segment necessari

 // -----------------------------------------
 // Richiesta Memoria per struttura BTRIEVE !
 // -----------------------------------------

 SizeMem=sizeof(struct FILE_SPECS)+(KeySpec*sizeof(struct KEY_SPECS))+265;

 //printf("Memoria necessaria per strutture Btrieve : %ld\n\r",SizeMem);

 BTIhdl=memo_chiedi(RAM_HEAP,SizeMem,"Btrieve1");

 fileSpecs=memo_heap(BTIhdl);
 keySpecs=(struct KEY_SPECS  *) farPtr((CHAR *) fileSpecs,sizeof(struct FILE_SPECS));


	// --------------------
	// Definizione chiavi !
	// --------------------
	KS=0;
	KeyMoreGreat=-1;
	KeyDup=0;

	for (a=0;a<Index;a++)
	{
	 flag=0;
		AdbIndex[a].KeySize=0;
	 for (b=0;b<IndexDesc;b++)
	 {
		if (strcmp(AdbIndex[a].IndexName,idx[b].IndexName)) continue;
		flag++;
		if (flag==1) continue;// Il primo ä la descrizione
		FieldLoc=adb_Floc(idx[b].FieldName,RecInfo);
		if (FieldLoc<0)
			{sprintf(serv,"ADB_CREA:Campo in index non trovato\n[%s] ?",idx[b].FieldName);
			 win_errgrave(serv);
			 }
		keySpecs[KS].position = RecInfo[FieldLoc].pos+1; // Posizione
		keySpecs[KS].length = RecInfo[FieldLoc].RealSize; // Larghezza

		if (idx[b].flag==OFF) // Settaggio standard
				{keySpecs[KS].flags = EXTTYPE_KEY|MOD;}
				else
				{keySpecs[KS].flags = idx[b].bti_flag;
				}

		// Se il flag duplicabile ä a ON
		if (idx[b].dup) keySpecs[KS].flags|=DUP;

		if (idx[b].bti_flag&DUP) KeyDup++; // Chiave Duplicabile

		// Prosegue ?
		if ((flag-1)<AdbIndex[a].KeySeg) keySpecs[KS].flags |= 0x10;

		switch (RecInfo[FieldLoc].tipo)
		{
		 case ADB_ALFA :
		 case ADB_NUME :
		 case ADB_DATA : keySpecs[KS].type = ZSTRING_TYPE; break;

		 case ADB_FLAG :
		 case ADB_INT  : keySpecs[KS].type = INTEGER_TYPE; break;

		 case ADB_FLOAT: keySpecs[KS].type = IEEE_TYPE; break;
		 case ADB_COBD : keySpecs[KS].type = DECIMAL_TYPE; break;
		 case ADB_COBN : keySpecs[KS].type = NUMERIC_TYPE; break;
		 case ADB_AINC : keySpecs[KS].type = AUTOINCREMENT_TYPE; break;
		}

		keySpecs[KS].null = 0;
		AdbIndex[a].KeySize+=RecInfo[FieldLoc].RealSize;//Dimensioni chiave
		if (AdbIndex[a].KeySize>KeyMoreGreat) KeyMoreGreat=AdbIndex[a].KeySize;

		/*
		printf("idx:%d ks:%d %-7s flg:%4x type:%4d ps:%d (ln%d) fg%d KyS%d <_>%d\n\r",
					 a,KS,RecInfo[FieldLoc].desc,
					 keySpecs[KS].flags,
					 keySpecs[KS].type,
					 keySpecs[KS].position,
					 keySpecs[KS].length,flag,AdbIndex[a].KeySeg,
					 AdbIndex[a].KeySize);
			*/
		KS++;
	 }
	}

	if (KS!=KeySpec) win_errgrave("ADB_CREA:Errore in definizione indici");

 // -----------------------------------------
 //      Ottimizzazione del PagSize         !
 //      VEDI MANUALE 3.1                   !
 //                                         !
 // -----------------------------------------

 // Calcola grandezza fisica della pagina

 Physical=RecSize+2;// Per contatre il record 6.x
 Physical+=8*KeyDup;// Numero chiavi duplicabili
 if (FlagFile&VAR_RECS) Physical+=4;// Record di lunghezza variabile
 if (FlagFile&BLANK_TRUNC) Physical+=2;// Toglie gli spazi
 if (FlagFile&VATS_SUPPORT) Physical+=6;// Supportato dal VATS

 // Calcola grandezza minima della chiave
 MinKeyPage=KeyMoreGreat;
 if (KeyDup) MinKeyPage+=12; else MinKeyPage+=12;
 MinKeyPage*=8;
 MinKeyPage+=12;

 //printf("PhysicalPage %ld MinKeyPage %ld KeySegment %d\n\r",Physical,MinKeyPage,KS);


 // Controllo dimensione per minimo key segment

 if ((KS<9)&&(MinKeyPage<512)) MinKeyPage=512;
 if ((KS<24)&&(MinKeyPage<1024)) MinKeyPage=1024;
 if ((KS<25)&&(MinKeyPage<1536)) MinKeyPage=1536;
 if ((KS<55)&&(MinKeyPage<2048)) MinKeyPage=2048;
 if ((KS<120)&&(MinKeyPage<4096)) MinKeyPage=4096;

 b=-1; pp=100000L;

 for (a=1;a<9;a++)
 {
	pp2=(LONG) a*512;
	if (pp2<MinKeyPage) continue;
	pp2-=6; // Domensioni pagina
	pp3=pp2/Physical; pp3*=Physical;
	pp2-=pp3;
	//printf("pp2:%ld pp3:%ld pagesize=%d\n\r",pp2,pp3,a*512);
	if ((pp2<pp)&&(pp3>0)&&(pp3>0)) {pp=pp2; b=a;}
 }

 if (b==-1) win_errgrave("ADB_CREA:Mega errore grave");
 PageSize=b*512;
 //printf("Page Size %d (512*%d) Phy%ld\n\r",PageSize,b,Physical);

 // -----------------------------------------
 //        RIEMPE LA STRUTTURA BTRIEVE      !
 // -----------------------------------------

	fileSpecs[0].recLength = RecSize; // Len record
	fileSpecs[0].pageSize = PageSize;
	fileSpecs[0].indexCount = Index; // Numero indici
	fileSpecs[0].flags = FlagFile; // Vedi pag. 4-16
	fileSpecs[0].allocations = 0; // Pagine da allocare

	// -------------------------------------------------------
	//           CHIAMATA IN CREAZIONE DI BTRIEVE            !
	// -------------------------------------------------------
	//getch();
	strcpy((BTI_CHAR *)ADBBuf, file );
	strcat((BTI_CHAR *)ADBBuf,".BTI");
	dataLen=(SINT ) SizeMem;

	status = BTRV( B_CREATE,
								 posBlock,
								 memo_heap(BTIhdl),// Puntatore alla strutture
								 &dataLen, // Lunghezza del dato
								 ADBBuf, // nome del File
								 0); // 0= nessun controllo sull'esitenza del file

	if (status) {BTI_err(file,"CREAZIONE",status); return status;}

	// -------------------------------------------------------
	//           CREAZIONE FILE ADB DI RIFERIMENTO           !
	// -------------------------------------------------------
	strcpy((BTI_CHAR *)ADBBuf,file);
	strcat((BTI_CHAR *)ADBBuf,".ADB");
	rt=OFF;
	if (f_open((BTI_CHAR *)ADBBuf,"wb+",&pf1)) rt=ON;

	//-------------------
	// Scrittura Header !
	//-------------------

	strcpy(AdbHead.id,"Adb1.2 Eh3");
	memset(AdbHead.DescFile, 0, 30); // Pulisce il record
	strcpy(AdbHead.DescFile,desc);
	AdbHead.eof=26;
	AdbHead.Field=Field;
	AdbHead.wRecSize=RecSize;
	AdbHead.KeyMoreGreat=KeyMoreGreat;
	AdbHead.Index=Index;
	AdbHead.IndexDesc=IndexDesc;
	AdbHead.KeySeg=KS;
	if (f_put(pf1,NOSEEK,&AdbHead,sizeof(struct ADB_HEAD))) {rt=ON; goto fine;}

	//------------------------
	// Scrittura Dati Record !
	//------------------------
	for (a=0;a<Field;a++)
	{
	//strupr(RecInfo[a].desc);
	if (f_put(pf1,NOSEEK,&RecInfo[a],sizeof(struct ADB_REC))) {rt=ON; goto fine;}
	}

	//------------------------
	// Scrittura Dati Index  !
	//------------------------
	for (a=0;a<Index;a++)
	{
	if (f_put(pf1,NOSEEK,&AdbIndex[a],sizeof(struct ADB_INDEX))) {rt=ON; goto fine;}
	}

	//------------------------------------
	// Scrittura Dati Descrizione Index  !
	//------------------------------------
	for (a=0;a<IndexDesc;a++)
	{
	//strupr(idx[a].FieldName);
	if (f_put(pf1,NOSEEK,&idx[a],sizeof(struct ADB_INDEX_SET))) {rt=ON; goto fine;}
	}

	f_close(pf1);

	fine:
	if (rt) win_errgrave("ADB_CREA:Errore in creazione .ADB");

 memo_libera(BTIhdl,"adb_crea");

 //------------------------------------
 // Aggiunta del 97 - Proprietario    !
 //------------------------------------




 return 0;
}
//   +-------------------------------------------∏
//   | ADB_ERRGRAVE Errore in ADB                |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
void adb_errgrave(CHAR *chi,SINT Hadb)
{
// TCHAR serv[100];
 PRG_end("adb_errgrave():\nAdbHadle non valido in %s [%d]",strupr(chi),Hadb);
// win_errgrave(serv);
}

CHAR *adb_data(CHAR *ptd)
{
 static CHAR serv[14],*p=serv;

 if (!*ptd) {*serv=0;return serv;}
 memcpy(p,ptd+6,2); // Anno
 memcpy(p+2,ptd+4,2); // Anno
 memcpy(p+4,ptd,4); // Anno
 serv[8]=0;
 return serv;
}

CHAR *adb_GetDate(HDB Hdb,CHAR *Nome)
{
 return adb_data(adb_FldPtr(Hdb,Nome));
}

//   +-------------------------------------------∏
//   | ADB_NCCUPDATE Modifica il record corrente |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_NCCupdate(SINT Hadb)
{
	BTI_WORD dataLen;
	BTI_SINT status;

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("update",Hadb);

	dataLen=ADB_info[Hadb].AdbHead->wRecSize;
	status = BTRV( B_UPDATE,
								 ADB_info[Hadb].PosBlock,
								 ADB_info[Hadb].RecBuffer,// Puntatore ai dati
								 &dataLen, // Grandezza dei dati
								 ADB_info[Hadb].KeyBuffer,   // ???> in output
								 -1); // ??? in input

	// Lock Errore
	if (status==81) return status;
	if (status) {BTI_err(ADB_info[Hadb].FileName,"NCCUPDATE:",status);}

	return status;
}

//   +-------------------------------------------∏
//   | ADB_TRANS  Gestione delle transizioni     |
//   |                                           |
//   |  Hadb : handle del file ADB               |
//   |  flag = ON    Inizio transizione          |
//   |         OFF   Fine transizione            |
//   |         ABORT Annulla transizione         |
//   |                                           |
//   |                                           |
//   |                                           |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+
SINT adb_trans(SINT Hadb,SINT flag)
{
	BTI_SINT status;
	CHAR serv[15];

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("Trans",Hadb);

	switch (flag)
	{
	 case ON:

		strcpy(serv,"BEGIN_TRANS");
		status = BTRV(1000+B_BEGIN_TRAN,
					  ADB_info[Hadb].PosBlock,
					  ADB_info[Hadb].RecBuffer,// Puntatore ai dati
					  NULL, // Grandezza dei dati
					  ADB_info[Hadb].KeyBuffer,   // ???> in output
					  0); // ??? in input
		break;

	 case OFF:

		strcpy(serv,"END_TRANS");
		status = BTRV( B_END_TRAN,
								 ADB_info[Hadb].PosBlock,
								 ADB_info[Hadb].RecBuffer,// Puntatore ai dati
								 NULL, // Grandezza dei dati
								 ADB_info[Hadb].KeyBuffer,   // ???> in output
								 0); // ??? in input
		break;

	 case ABORT:

		strcpy(serv,"ABORT_TRANS");
		status = BTRV( B_ABORT_TRAN,
								 ADB_info[Hadb].PosBlock,
								 ADB_info[Hadb].RecBuffer,// Puntatore ai dati
								 NULL, // Grandezza dei dati
								 ADB_info[Hadb].KeyBuffer,   // ???> in output
								 0); // ??? in input
		break;
	 default :PRG_end("Flag error in ADB_Tras");

	}
	if (status) {BTI_err(ADB_info[Hadb].FileName,serv,status);}
	return status;
}
//  +----------------------------------------------------------------∏
//	|                                                                |
//	|                     GESTORE LINK GERARCHICI                    |
//	|                                                                |
//	+----------------------------------------------------------------+

double HLcheck(struct ADB_LINK_LIST *link)
{
//	CHAR *p;
	BYTE *p;
	SINT register b;
	double check=0;

	#pragma pack(1)
	struct ADB_LINK_LISTCHECK {
		CHAR FilePadre[80]; // Nome del file padre
		CHAR FieldNamePadre[ADB_MAX_DESC]; // nome del campo collegato
		CHAR FileFiglio[80]; // Nome del file figlio ( si deve trovare nel percoro del padre
		CHAR FieldNameFiglio[ADB_MAX_DESC]; // nome del campo collegato
		CHAR IndexNameFiglio[ADB_MAX_IDXNAME]; // Key group di riferimento
		CHAR Opzione[30]; // Comando al linker : SCAN o +\"string\" o +n
		INT16 Clone;
		CHAR Proprietario[30];
	 };

    struct ADB_LINK_LISTCHECK Check;
	#pragma pack()

	
	
	
	
	//win_infoarg("%d",sizeof(Check));
	memset(&Check,0,sizeof(Check));

	strcpy(Check.FilePadre,link->FilePadre);
	strcpy(Check.FieldNamePadre,link->FieldNamePadre);
	strcpy(Check.FileFiglio,link->FileFiglio);
	strcpy(Check.FieldNameFiglio,link->FieldNameFiglio);
	strcpy(Check.IndexNameFiglio,link->IndexNameFiglio);
	strcpy(Check.Opzione,link->Opzione);
	strcpy(Check.Proprietario,link->Proprietario);
	Check.Clone=(INT16) link->Clone;

	p=(CHAR *) &Check;
	for (b=0;b<sizeof(Check);b++) {check+=(double) *p;p++;}

	return check;
}

//  +-----------------------------------------∏
//	| ADB_HLfile Definisce l'attivazione      |
//	|            e il file Tabella            |
//	|                                         |
//	|                                         |
//	| file Nome e percorso del file tabella   |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+
void adb_HLfile(CHAR *file)
{
 if (EHPower) PRG_end("adb_HLfile:La funzione vÖ chiamata prima del PRG_start()");
 if ((strlen(file)+1)>MAXPATH) {win_info("Nome File HLTabella errato");}
 strcpy(HLtabella,file);
 ADB_HL=ON; // attivo il Link Gerarchico
}

// +-----------------------------------------∏
//	| ADB_HLfind Cerca nella tabella un file  |
//	|                                         |
//	| file Nome del file padre                |
//	|                                         |
//	| Ritorna ON se cä OFF se non c'ä         |
//	|         in pt il puntatore al campo     |
//	|         o se non cä alla sua probabile  |
//	|         posizione nella lista (insert)  |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+
static SINT adb_HLfind(CHAR *file,LONG *pt)
{
	LONG lp;
	struct ADB_LINK_LIST adbL;

	for (lp=0;lp<adb_HLink;lp++)
	{
		memo_leggivar(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));
		if (!strcmp(strupr(adbL.FilePadre),strupr(file)))
		{
			*pt=lp;
			return ON;
		}

	}

	*pt=lp;

	return OFF;
}

// +-----------------------------------------------∏
//	| ADB_HLfindFiglio Cerca nella tabella un file  |
//	|                                               |
//	| file Nome del file figlio da cercare          |
//	|                                               |
//	| Ritorna ON se cä OFF se non c'ä               |
//	|         in pt il puntatore al campo           |
//	|         o se non cä alla sua probabile        |
//	|         posizione nella lista (insert)        |
//	|                                               |
//	|                           by Luca Vadora 1997 |
//	+-----------------------------------------------+
static SINT adb_HLfindFiglio (CHAR *file,LONG *pt)
{
	LONG lp;

	struct ADB_LINK_LIST adbL;

	for (lp = 0;lp < adb_HLink;lp++)
	{
		memo_leggivar(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));

		if (!strcmp (strupr (adbL.FileFiglio),strupr (file)))
		{
			*pt = lp;
			return ON;
		}

/**
		if (strcmp(strupr(adbL.FilePadre),strupr(file)))
		{
			*pt=lp;
			return OFF;
		}
**/
	}

	*pt = lp;

	return OFF;
}

//  +-----------------------------------------∏
//	| ADB_HLdelete Cancella nella tabella     |
//	|              gerarchica i riferimenti   |
//	|              di link di un determinato  |
//	|              file                       |
//	|                                         |
//	|  file Nome del file                     |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+
static SINT adb_HLdelete(CHAR *FilePadre)
{
 SINT a;
 LONG lp,pt,ptv;
 LONG NrLink;
 struct ADB_LINK_LIST adbL;

 if (adb_HLink==0) return 0; // Non ci sono Link da cancellare
 if (adb_HLinkHdl==-1) PRG_end("adb_Ldelete");
 //printf("DELETE:\n\r");
 a=adb_HLfind(fileName(FilePadre),&pt);
 if (a==OFF) return 0;// il File Padre non esiste
 //printf("%s pt=%ld",FilePadre,pt);
 // ------------------------------
 // Azzera i Link del file padre !
 // e riorganizza l'archivio     !
 // ------------------------------

 NrLink=pt;
 ptv=pt;
 for (lp=pt;lp<adb_HLink;lp++)
 {
	memo_leggivar(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));
	if (!strcmp(fileName(strupr(FilePadre)),adbL.FilePadre)) continue;
	memo_scrivivar(adb_HLinkHdl,ptv*sizeof(adbL),&adbL,sizeof(adbL));
	NrLink++; ptv++;
 }
 adb_HLink=NrLink;
 return 0;
}
//  +-----------------------------------------∏
//	| ADB_HLapri                              |
//	| (Hierarchic Link Table)                 |
//	|                                         |
//	| - Carica la tabella dei Link Gerarchici |
//	|   in memoria                            |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+
/*
static void HLsort(void *base,SINT num,SINT dim)
{
	SINT Hcmps();
	qsort(base,num,dim,cmps);
}
Hcmps(void *p1,void *p2)
{
 return strcmp((CHAR *) p1,(CHAR *) p2);
}
*/

SINT adb_HLapri(void)
{
 FILE *pf1;
 CHAR buf[200];
 LONG ptm;
 struct ADB_LINK_LIST adbL;
 SINT a;
 double Check,CheckFile;

 if (ADB_HL==OFF) PRG_end("Manca l'indicazione del FILE di tabella");
 // Chiude una precedente tabella se esiste
 adb_HLchiudi();

 //printf("APRI:\n\r");

 // -----------------------
 // Apre il file          !
 // -----------------------

 adb_HLink=0;
 os_errset(OFF);
 a=f_open(HLtabella,"r",&pf1);
 os_errset(POP);

 if (!a)
	{// Conta i link stabiliti
		 os_errset(OFF);
		 for (;;)
		 {
			if (f_gets(pf1,buf,sizeof(buf))) break;
			adb_HLink++;
		 }
		 os_errset(POP);

		 f_get(pf1,0,buf,0); // resetta il puntatore all'inizio
		 // Se ä maggiore di zero li carica
		 if (adb_HLink>0)
		 {
			adb_HLinkHdl=memo_chiedi(RAM_AUTO,adb_HLink*sizeof(struct ADB_LINK_LIST),"*HLINK");
			if (adb_HLinkHdl<0) PRG_end("No Memo per adb_link()");
			// Carica in memoria i dati e guarda se sono buoni
			CheckFile=0; Check=0;
			for (ptm=0;ptm<adb_HLink;ptm++)
			{
			 //if (f_gets(pf1,buf,sizeof(buf))==NULL) PRG_end("Strano ?");
			 adb_HLload(pf1,&adbL);

			 if (!strcmp(adbL.FilePadre,".CHECK."))
					{CheckFile=atof(adbL.FileFiglio);
					 break;}

/**			printf("Apri >[%s][%s][%s][%s][%s][%s][%d][%s]\n\r",
							adbL.FilePadre,
							adbL.FieldNamePadre,
							adbL.FileFiglio,
							adbL.FieldNameFiglio,
							adbL.IndexNameFiglio,
							adbL.Opzione,
							adbL.Clone,
							adbL.Proprietario);
**/
			 memo_scrivivar(adb_HLinkHdl,ptm*sizeof(adbL),&adbL,sizeof(adbL));
			 Check+=HLcheck(&adbL);

			}
//			printf ("Check apri = %f - %f\n",Check,CheckFile);

//			if (Check!=CheckFile) PRG_end("Checksum:Errore in ADB_HLload");
			if (Check!=CheckFile) win_info("Checksum:Errore in ADB_HLload");
			adb_HLink--;// Tolgo la riga del Checksum
		 }
	f_close(pf1);
	return OFF;
	}
	else
	{
	 if (!EHPower)
	 {
		printf("ATTENZIONE:\nManca la tabella dei link [%s]",HLtabella);
	 }
	}
 return ON;
}

static void adb_HLchiudi(void)
{
 if (adb_HLinkHdl>-1) memo_libera(adb_HLinkHdl,"*HLINK");
 adb_HLinkHdl=-1;
}

// +-----------------------------------------∏
//	| ADB_HLload  Legge da disco un link      |
//	|             gerarchico                  |
//	|                                         |
//	|  file      = Nome del file              |
//	|  path      = Path figli                 |
//	|              se non indicato nella stru |
//	|              ttura di link verrÖ assunto|
//	|              di default questo path     |
//	|  link      = Struttura di Link che      |
//	|              definisce i nuovi link     |
//	|              da inserire                |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+

static void adb_HLload(FILE *pf1,struct ADB_LINK_LIST *link)
{
	CHAR *p,*ptr[7];
	CHAR *p2;
 //	CHAR serv[200];
	CHAR buf[200];

	SINT a;

	if (f_gets(pf1,buf,sizeof(buf)))
		PRG_end("LLoad ?");

	p=buf;

	if (*p=='#')
	{
		strcpy(link->FilePadre,".CHECK.");
		p++;
		strcpy(link->FileFiglio,p);
		return;
	}

	if (*p!='>')
		PRG_end("HLload:Errore in Mark");

	p++;
	ptr[0]=p;

 // Trova i 6 puntatori
	for (a=1;a<7;a++)
	{
		p = strstr(p,",");

		if (p==NULL)
			PRG_end("HLload:Errore in HLinkTab");

		*p=0;
		ptr[a] = p+1;
		p++;
	}

// Toglie il CR
	p=ptr[6]+strlen(ptr[6]);
	p--;

	if ((*p==13)||(*p==10))
		*p=0;

 //	Pulisce la struttura
	memset(link,0,sizeof(struct ADB_LINK_LIST));

	// File Padre
	if (strlen(ptr[0])>sizeof(link->FilePadre))
	{
		PRG_end("adb_LLoad:FilePadre > riga %s",buf);
		//PRG_end(serv);
	}

	strcpy(link->FilePadre,ptr[0]);

	// Field Name Padre
	p2=ptr[1];

	if (*p2=='') // Tratasi di campo clone
	{
		link->Clone=ON;
		p2++;
	}
	else
		link->Clone=OFF;

	if (strlen(p2)	> sizeof (link->FieldNamePadre))
		 {PRG_end("adb_LLoad:FieldNamePadre > riga %s",buf);
			//PRG_end(serv);
			}

	strcpy(link->FieldNamePadre,p2);

	// File Figlio

	if (strlen(ptr[2])>sizeof(link->FileFiglio))
		 {PRG_end("adb_LLoad:FileFiglio > riga %s",buf);
			//PRG_end(serv);
			}
	strcpy(link->FileFiglio,ptr[2]);

	// Field Name Figlio
	if (strlen(ptr[3])>sizeof(link->FieldNameFiglio))
		 {PRG_end("adb_LLoad:FieldNameFiglio > in %s",buf);
			//PRG_end(serv);
			}
	strcpy(link->FieldNameFiglio,ptr[3]);

	// Index Name Figlio
	if (strlen(ptr[4])>sizeof(link->IndexNameFiglio))
		 {PRG_end("adb_LLoad:IndexNameFiglio > in %s",buf);
			//PRG_end(serv);
			}
	strcpy(link->IndexNameFiglio,ptr[4]);

	// Opzione
	if (strlen(ptr[5])>sizeof(link->Opzione))
		 {PRG_end("adb_LLoad:Opzione > in %s",buf);
			//PRG_end(serv);
			}
	strcpy(link->Opzione,ptr[5]);

	// Proprietario
	if (strlen(ptr[6])>sizeof(link->Proprietario))
		 {PRG_end("adb_LLoad:Proprietario > in %s",buf);
			//PRG_end(serv);
			}

	strcpy(link->Proprietario,ConvertoPro (ptr[6]));

	// Calcolo CheckDigit

	//return HLcheck(link);
}

// +-----------------------------------------∏
//	| ADB_HLsave  Salva su disco un link      |
//	|             gerarchico                  |
//	|                                         |
//	|  file      = Nome del file              |
//	|  path      = Path figli                 |
//	|              se non indicato nella stru |
//	|              ttura di link verrÖ assunto|
//	|              di default questo path     |
//	|  link      = Struttura di Link che      |
//	|              definisce i nuovi link     |
//	|              da inserire                |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+

static void adb_HLsave(FILE *pf1,struct ADB_LINK_LIST *link,CHAR *mark)
{
 //CHAR serv[256];
 SINT Hdl;
 CHAR *Buf;

 Hdl=memo_chiedi(RAM_HEAP,256,"HLS");
 Buf=memo_heap(Hdl);

 if (!strcmp(mark,"#"))
 fprintf(pf1,"%s%s\n",mark,link->FilePadre);
 else
 {
	if (link->Clone) sprintf(Buf,"%s",link->FieldNamePadre);
					 else
					 strcpy(Buf,link->FieldNamePadre);

    fprintf(pf1,"%s%s,%s,%s,%s,%s,%s,%s\n",mark,
			link->FilePadre,
			Buf,//link->FieldNamePadre,
			link->FileFiglio,
			link->FieldNameFiglio,
			link->IndexNameFiglio,
			link->Opzione,
			ConvertoPro(link->Proprietario));
 }

 memo_libera(Hdl,"HLS");
}

// +-----------------------------------------∏
//	| ADB_HLInsert Inserisce nella tabella    |
//	|              gerarchica i riferimenti   |
//	|              di link di un determinato  |
//	|              file                       |
//	|                                         |
//	|  file      = Nome del file              |
//	|  path      = Path figli                 |
//	|              se non indicato nella stru |
//	|              ttura di link verrÖ assunto|
//	|              di default questo path     |
//	|  link      = Struttura di Link che      |
//	|              definisce i nuovi link     |
//	|              da inserire                |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+

static void adb_HLinsert(CHAR *FilePadre,struct ADB_LINK *link)
{
 SINT a;
 LONG lp,pt;
 struct ADB_LINK_LIST adbL;
 FILE *pf1;
// char PathFigli[MAXPATH];
// TCHAR serv[200];
 CHAR *p,*p2;//,*nome1,*nome2;
 double Check;

 CHAR file[MAXPATH];

 // Crea il nome del file padre
 strcpy(file,fileName(FilePadre));
 p=strstr(file,"."); if (p!=NULL) *p=0;

 //printf("INSERT:\n\r");

	// File Figlio
	//strcpy(PathFigli,file);
	//nome1=fileName(PathFigli);
	//*nome1=0;

 //if (adb_HLinkHdl==-1) PRG_end("adb_HLinsert");
 a=adb_HLfind(file,&pt);

 // ----------------------------------------------------
 // Se ci sono giÖ record su quel file li scorre fino  !
 // ad arrivare all'ultimo                             !
 // ----------------------------------------------------

 if (a)
	{
		for (pt++;pt<adb_HLink;pt++)
		{
		 memo_leggivar(adb_HLinkHdl,pt*sizeof(adbL),&adbL,sizeof(adbL));
		 if (strcmp(adbL.FilePadre,file)>0) break;
		}
	}

 // -------------------------------------------
 //                                           !
 // Apre il file della tabella                !
 //                                           !
 // -------------------------------------------

 a=f_open(HLtabella,"w",&pf1);
 if (a&&pt) PRG_end("HTabLink:Errore grave - ricostruire la tabella");

 // ----------------------------------------
 // Riscrive i dati prima dell'inserimento !
 // ----------------------------------------
 Check=0;

 for (lp=0;lp<pt;lp++)
 {
	memo_leggivar(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));
	Check+=HLcheck(&adbL);
	adb_HLsave(pf1,&adbL,">");
 }

 // --------------------------
 // Inserisce i nuovi Links  !
 // --------------------------
 if (link!=NULL)
 {
	for (a=0;;a++)
	{
	// Fine inserimento
	if (strlen(link[a].FileFiglio)<1) break;

	memset(&adbL,0,sizeof(struct ADB_LINK_LIST));
	// File Padre
	if (strlen(file)>sizeof(adbL.FilePadre))
		 {PRG_end("adb_Linsert:FilePadre > in %s",file);
			//PRG_end(serv);
			}
	strcpy(adbL.FilePadre,file);

	// Nome campo Padre
	p2=link[a].FieldNamePadre;
	if (*p2=='#') {adbL.Clone=ON; p2++;} else adbL.Clone=OFF;
	if (strlen(p2)>sizeof(adbL.FieldNamePadre))
		 {PRG_end("adb_Linsert:FieldNamePadre > in %s",file);
			//PRG_end(serv);
			}
	strcpy(adbL.FieldNamePadre,p2);

	if (strlen(link[a].FileFiglio)>sizeof(adbL.FileFiglio))
		 {PRG_end("adb_Linsert:FileFiglio > in %s",file);
			//PRG_end(serv);
			}
	strcpy(adbL.FileFiglio,link[a].FileFiglio);

	// Nomca campo Figlio
	if (strlen(link[a].FieldNameFiglio)>sizeof(adbL.FieldNameFiglio))
		 {PRG_end("adb_Linsert:FieldNameFiglio > in %s",file);
			//PRG_end(serv);
			}
	strcpy(adbL.FieldNameFiglio,link[a].FieldNameFiglio);

	// Index Name Figlio
	if (strlen(link[a].IndexNameFiglio)>sizeof(adbL.IndexNameFiglio))
		 {PRG_end("adb_Linsert:IndexNameFiglio > in %s",file);
			//PRG_end(serv);
			}
	strcpy(adbL.IndexNameFiglio,link[a].IndexNameFiglio);

	// Opzione
	if (strlen(link[a].Opzione)>sizeof(adbL.Opzione))
		 {PRG_end("adb_Linsert:Opzione > in %s",file);
			//PRG_end(serv);
			}
	strcpy(adbL.Opzione,link[a].Opzione);

	// Proprietario
	if (strlen(link[a].Proprietario) > sizeof (adbL.Proprietario))
	{
		PRG_end("adb_Linsert:Proprietario > in %s",file);
		//PRG_end(serv);
	}

//	printf ("%s\n",link[a].Proprietario);

	strcpy(adbL.Proprietario,link[a].Proprietario);

	// Aggiorna il file
	Check+=HLcheck(&adbL);
	adb_HLsave(pf1,&adbL,">");
	}
 }

 // ----------------------------------------
 // Riscrive i dati in coda                !
 // ----------------------------------------

 for (lp=pt;lp<adb_HLink;lp++)
 {
	memo_leggivar(adb_HLinkHdl,lp*sizeof(adbL),&adbL,sizeof(adbL));
	Check+=HLcheck(&adbL);
	adb_HLsave(pf1,&adbL,">");
 }

 // Scrittura del Check
 sprintf(adbL.FilePadre,"%f",Check);
 adb_HLsave(pf1,&adbL,"#");

 // Chiude un precedente tabelle se esiste
 f_close(pf1);
 adb_HLchiudi();
}

//  +--------------------------------------------∏
//	| ADB_HLInsertPro Inserisce nella tabella    |
//	|                 gerarchica i riferimenti   |
//	|                 di link di un determinato  |
//	|                 file                       |
//	|                                            |
//	|     link      = Struttura di Link che      |
//	|                 definisce i nuovi link     |
//	|                 da inserire                |
//	|                                            |
//	|                        by Luca Vadora 1997 |
//	+--------------------------------------------+

static void adb_HLinsertPro (CHAR *FileFiglio,CHAR *Client)
{
	SINT a;
	LONG pt;
	struct ADB_LINK_LIST adbL;
	FILE *pf1;
	CHAR *p;//,*nome1,*nome2;
	double Check;

	CHAR file[MAXPATH];

// Crea il nome del file padre
	strcpy (file,fileName (FileFiglio));

	p=strstr(file,".");

	if (p!=NULL)
		*p=0;

	a = adb_HLfindFiglio (file,&pt);

//	printf ("file=%s a=%d pt=%ld\n",file,a,pt);

 // ----------------------------------------------------
 // Se ci sono giÖ record su quel file li scorre fino  !
 // ad arrivare all'ultimo                             !
 // ----------------------------------------------------

	if (a)
	{
		for (pt=pt;pt < adb_HLink;pt++)
		{
			memo_leggivar (adb_HLinkHdl,pt * sizeof(adbL),&adbL,sizeof(adbL));

			if (strcmp (adbL.FileFiglio,file))
				continue;

			strcpy (adbL.Proprietario,Client);

			memo_scrivivar (adb_HLinkHdl,pt * sizeof(adbL),&adbL,sizeof(adbL));
		}
	}
	else
		return;

 // -------------------------------------------
 //                                           !
 // Apre il file della tabella                !
 //                                           !
 // -------------------------------------------

	a = f_open (HLtabella,"w",&pf1);

	if (a&&pt)
		PRG_end("HTabLink:Errore grave - ricostruire la tabella");

	Check=0;

 // --------------------------
 // Inserisce i nuovi Links  !
 // --------------------------
	for (a = 0;a < adb_HLink;a++)
	{
		memo_leggivar (adb_HLinkHdl,a * sizeof(adbL),&adbL,sizeof(adbL));

 // Aggiorna il file
		Check += HLcheck (&adbL);

		adb_HLsave (pf1,&adbL,">");
	}

 // Scrittura del Check
	sprintf (adbL.FilePadre,"%f",Check);

	adb_HLsave(pf1,&adbL,"#");

 // Chiude un precedente tabelle se esiste
	f_close(pf1);

	adb_HLchiudi();
}
//  +-----------------------------------------∏
//	| ADB_HLCrea   Ricerca e cancella se esis |
//	|              tono Link del file Padre   |
//	|              e inserisce quelli nuovi   |
//	|                                         |
//	|  file      = Nome del file              |
//	|              se non indicato nella stru |
//	|              ttura di link verrÖ assunto|
//	|              di default il path file    |
//	|  link      = Struttura di Link che      |
//	|              definisce i nuovi link     |
//	|              da inserire                |
//	|                                         |
//	| ATTENZIONE : 1) Il file tabella,i files |
//	|                 padri devono risiedere  |
//	|                 nella stessa directory  |
//	|                                         |
//	|              2) I files figli possono   |
//	|              risiedere in sub directory |
//	|              generate dalla directory   |
//	|              che contiene il file padri |
//	|              Ma ä meglio se stanno      |
//	|              insieme ai padri           |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+

void adb_HLcrea(CHAR *FilePadre,struct ADB_LINK *link)
{
 adb_HLapri();
 adb_HLdelete(FilePadre); // Cancella i riferimenti gerarchici con il file
 adb_HLinsert(FilePadre,link); // Inserisce i nuovi riferimenti
}

 // -------------------------
 // Opzione di AGGIUNTA     !
 // Ritorna NULL se         !
 // l'opzione non ä valida  !
 //                         !
 // -------------------------

CHAR *OPZ2(CHAR *Opz)
{
	CHAR *agg;
	CHAR *p;

	if (*Opz!='+')
		return NULL;

	p=strstr(Opz,"\"");

	if (p==NULL)
		return NULL;

	p++;

	agg=p;

	for (;*p!=0;p++)
	{
		if (*p=='\"')
			break;
	}

	if (*p==0)
		goto nob;

	*p=0;

	nob:
	return agg;
}

void ScriviUP(SINT Hdl,LONG pt,LONG dato)
{
 LONG dato2;
 dato2=dato;
 memo_scrivivar(Hdl,pt*4,&dato2,4);
}

LONG LeggiUP(SINT Hdl,LONG pt)
{
 LONG dato2;
 memo_leggivar(Hdl,pt*4,&dato2,4);
 return dato2;
}

//  +-----------------------------------------∏
//	| adb_HLaction                            |
//	| Funzione di azione e controllo          |
//	| su link gerarchici                      |
//	|                                         |
//	| se *Vecchiorec == NULL test di presenza |
//	|        link per controllo cancellazione |
//	|                                         |
//	| altrimenti                              |
//	|                                         |
//	| Vecchiorec = Puntatore al vecchio record|
//	|       prima del update (giÖ avvenuto)   |
//	|       La funzione effettua l'update     |
//	|       dei campi-clone collegati         |
//	|       se il valore del campo ä cambiato |
//	|       dal precedente                    |
//	|                                         |
//	| Hdb  = Handle del ADB                   |
//	|                                         |
//	| Ritorna NULL se nonc i sono collegamenti|
//	|         ptr = Sç,Collegamenti attivi    |
//	|               nome Primo magazzino      |
//	|                                         |
//	|  NB : Il LINK non funziona sui campi    |
//	|       numerici (ä da fare ma non ne ho  |
//	|       voglia per ora) 19/11/96          |
//	|                             Tassistro   |
//	|                                         |
//	|  NB2: Se aspettavo te il LINK non       |
//  |       funzionerebbe ancora adesso       |
//  |                       20/01/1998        |
//  |                             Vadora      |
//  |                                         |
//	|                          by Vadora 1998 |
//	+-----------------------------------------+
static CHAR *adb_HLaction(SINT Hdb,CHAR *VecchioRec)
{
	SINT test=OFF;
	SINT a;
	SINT HdlFiglio;
	SINT HDF;
	SINT Opz;
	SINT FlagFind;
	SINT Update;
	SINT SizeFiglio;
	SINT SizePadre;
	SINT SizeMemo;
	SINT SizeFiglio2;
	SINT MemoUP=-1;
	SINT FlagPresenta;
	SINT Opz2;
	SINT CampoPadreHdl=-1;

	LONG pt;
	LONG Cpt;
	LONG cnt;
	LONG RecNum;
	LONG PrimoFiglio;
	LONG FineFiglio;
	LONG NumeroUpdate;
	LONG NumFigli;
	LONG HrecUP;

	CHAR PathPadre[MAXPATH];
	CHAR FilePadre[MAXPATH];
	CHAR FileFiglio[MAXPATH];
	CHAR FileConf[MAXPATH];
	CHAR CampoPadre2[30];

	CHAR *p;
	CHAR *agg;
	CHAR *PtFiglio;
	CHAR *CampoPadre;
	CHAR *OldField;

	static CHAR DescFile[30];

	void *ptP;
	struct ADB_REC *FldPadre;

	void *ptF;
	struct ADB_REC *FldFiglio;

	struct ADB_LINK_LIST adbL;
	struct ADB_LINK_LIST adbL2;


//	adb_position(Hdb,&Hrec); // Salva il record interessato
	if (adb_HLinkHdl<0) PRG_end("adb_HLAction ?");

	// ------------------------------
	// Trova il PATH padre          !
	// ------------------------------
	strcpy(PathPadre,HLtabella); // PrioritÖ Percorso tabella

	p = fileName (PathPadre);
	*p=0;

	// ------------------------------
	// Trova il nome del file Padre !
	// ------------------------------
	strcpy(FilePadre,fileName(ADB_info[Hdb].FileName));

	p=strstr(FilePadre,".");

	if (p!=NULL)
		*p=0;

	a=adb_HLfind(FilePadre,&pt);

//	printf("------- %s a=%d",FilePadre,a);

	if (a==OFF)
		return OFF;// il File Padre non esiste

	PrimoFiglio=pt;

	Update=OFF;

	if (VecchioRec!=NULL)
		Update=ON;

	// ---------------------------------------
	//  APRE AL FINESTRA                     !
	// ---------------------------------------


	FlagPresenta=OFF;

	test=OFF; // Azzera il flag

	// ----------------------------------------
	//  CONTO I FIGLIOLI                      !
	// ----------------------------------------
	NumFigli=0;

	for (;pt<adb_HLink;pt++)
	{
		memo_leggivar(adb_HLinkHdl,pt*sizeof(adbL),&adbL,sizeof(adbL));

		if (strcmp(FilePadre,adbL.FilePadre))
			break;

		NumFigli++;
	}

	// ----------------------------------------
	//  LOOP DI CONTROLLO SUI FIGLI COLLEGATI !
	// ----------------------------------------
	FineFiglio=PrimoFiglio+NumFigli-1;

	for (pt=PrimoFiglio;pt<=FineFiglio;pt++)
	{
		memo_leggivar(adb_HLinkHdl,pt*sizeof(adbL),&adbL,sizeof(adbL));
/**
		printf("Link %ld %s >>> %s(%s) clone %d ?\n",
						pt,adbL.FieldNamePadre,
						adbL.FieldNameFiglio,
						adbL.FileFiglio,
						adbL.Clone);
**/
		// Se non ci sono pió figli break
		if (strcmp(FilePadre,adbL.FilePadre))
			PRG_end("FineFigli errato");

		// Se ä un campo clone e sono in test delete lo salto
		if ((!Update) && (adbL.Clone))
			continue;

		// Se non ä un campo clone e sono in update link lo salto
		if (Update && (!adbL.Clone))
			continue;

		// ----------------------------------------
		// Controllo se il campo ä stato cambiato !
		// ----------------------------------------
		if (Update)
		{
			OldField=VecchioRec+adb_FldOffset(Hdb,adbL.FieldNamePadre);
/**
			printf("Confronto %s [%s]=[%s] ?\n",adbL.FieldNamePadre,
							 OldField,adb_FldPtr(Hdb,adbL.FieldNamePadre));
**/
			 // Controllo se il campo non ä stato cambiato
			if (!strcmp(OldField,adb_FldPtr (Hdb,adbL.FieldNamePadre)))
				continue;

			NumeroUpdate=0;
		}

		// ------------------------------------------------------
		//  Aggiunge il percorso Padre al file Figlio           !
		// ------------------------------------------------------

		sprintf(FileFiglio,"%s%s",strupr(PathPadre),strupr(adbL.FileFiglio));

		// ------------------------------------------------------
		// Ricerca sugli Handle aperti se esiste il file Figlio !
		// ------------------------------------------------------
		HdlFiglio=-1;

		for (a=0;a<ADB_max;a++)
		{
			if (ADB_info[a].HdlRam==-1)
				continue;

			//mouse_inp ();

			strcpy (FileConf,ADB_info[a].FileName);

			p=strstr (FileConf,".");

			if (p!=NULL)
				*p=0;

		 //printf("= [%s] ?\n",FileConf);
		 // OK c'ä !!

			if (!strcmp (strupr (FileConf),strupr (FileFiglio)))
			{
				HDF=OFF;
				HdlFiglio=a;
				break;
			}
		}

	 // Se non ä aperto lo apre
		if (HdlFiglio==-1)
		{
			HDF=ON;

			a=adb_open(FileFiglio,adbL.Proprietario,B_NORMAL|MEFS,&HdlFiglio);

			if (a)
			{
				PRG_end("HLaction= FileFiglio ? %s Btrieve =%d",FileFiglio,a);
				//PRG_end(FilePadre);
			}
		}

	 // -------------------------------------------------
	 // INIZIO CONTROLLO CAMPO FIGLIO-PADRE             !
	 // -------------------------------------------------

		strcpy (DescFile,ADB_info[HdlFiglio].AdbHead->DescFile);

		Opz=-1;

		if (strlen(adbL.Opzione)==0)
			Opz=0;

	 // -------------------------
	 // Opzione di SCAN         !
	 // -------------------------
		if (!strcmp(adbL.Opzione,"SCAN"))
			Opz=1;

	 // -------------------------
	 // Opzione di AGGIUNTA     !
	 // -------------------------
		agg=OPZ2(adbL.Opzione);

		if (agg!=NULL)
			Opz=2;

		if (Opz==-1)
			PRG_end("HLaction: Opzione ?");

	 // ----------------------------------------------
	 // Trova Grandezza Campo Padre e alloca memoria !
	 // ----------------------------------------------

		ptF=adb_FldInfo(HdlFiglio,adbL.FieldNameFiglio,&FldFiglio);

		if (ptF==NULL)
			PRG_end("HLaction:CAMPO FIGLIO ?");

		ptP=adb_FldInfo(Hdb,adbL.FieldNamePadre,&FldPadre);

		if (ptP==NULL)
			PRG_end("HLaction:CAMPO PADRE ?");

		SizePadre=FldPadre->RealSize;
		SizeFiglio=FldFiglio->RealSize;

		SizeMemo=adb_keymaxsize(HdlFiglio);

		if (SizeMemo <= SizePadre)
			SizeMemo = (FldPadre->RealSize + 1);

		CampoPadreHdl=memo_chiedi(RAM_HEAP,SizeMemo,"Campopadre");
		if (CampoPadreHdl<0) PRG_end("HLaction:out of memory");

		CampoPadre=memo_heap(CampoPadreHdl);

		memset(CampoPadre,0,SizeMemo);

		if (Opz==2)
			strcpy(CampoPadre,agg);

	 // Ricerco il vecchio campo
		if (Update)
			strcat(CampoPadre,OldField);
		else // Ricerco il campo attuale
			strcat(CampoPadre,adb_FldPtr(Hdb,adbL.FieldNamePadre));

		if (strlen (CampoPadre) > (UINT) (SizeFiglio-1))
			PRG_end("HLaction:CampoPadre > SizeFiglio");

	 // ----------------------------------------------
	 // CONFRONTO DEL CAMPO NEL DBASE                !
	 // ----------------------------------------------
/**
		 printf("CampoPadre [%s] %d\n",CampoPadre,SizeFiglio);
		 printf("FileName [%s] Index [%s] ",
						 adbL.FileFiglio,adbL.IndexNameFiglio);

		 if (HDF) printf("Nuovo File\n"); else printf ("giÖ Aperto\n");
		 printf("FieldNameFiglio [%s]\n",adbL.FieldNameFiglio);
**/

	 // ---------------------------------------
	 // SE C'E' UN INDICE COLLEGATO LO USO    !
	 // ---------------------------------------

		if (adbL.IndexNameFiglio[0])
		{
			a=adb_Afind(HdlFiglio,adbL.IndexNameFiglio,B_GET_GE,CampoPadre,B_RECORD|B_NOSTR);
			if (a) goto vaiavanti;

			FlagFind=ON;
		}
		else
		{// ------------------------
		 // se no leggo dal primo  !
		 // ------------------------
			FlagFind=OFF;

			a=adb_find(HdlFiglio,0,B_GET_FIRST,"",B_RECORD);

			if (a)
				goto vaiavanti; // Non ci sono record: esce

			RecNum=adb_recnum(HdlFiglio);

			if (RecNum<1)
				goto vaiavanti;

		}

	 // ------------------------------------
	 // SCANSIONE SEQUENZIALE DEL DBFIGLIO !
	 // ------------------------------------
/*
	 if (Update)
		 {MemoUP=memo_chiedi(RAM_AUTO,adb_recnum(HdlFiglio),"ADB:MemoUP");
			if (MemoUP<0) PRG_end("Memo ? MemoUP");}
	*/

		cnt=0;

		PtFiglio=adb_FldPtr(HdlFiglio,adbL.FieldNameFiglio);

		for (;;)
		{
			//mouse_inp();

			cnt++;

		 // -------------------------
		 // Confronto con la chiave !
		 // -------------------------

			switch (Opz)
			{
			// Confronto di tipo normale
				case 0:
				case 2:
					if (!strcmp(CampoPadre,PtFiglio))
						test=ON;

					break;

			// Confronto di tipo SCAN
				case 1: //printf("TEST SCAN\n");
					for (a=0;a<(FldFiglio->RealSize%(SizePadre-1));a++)
					{//printf("%d",a);
							 //printf("size, [%s] [%s]\n",SizePadre-1,PtFiglio+(a*(SizePadre-1)),CampoPadre);
//							 if (!memcmp(PtFiglio+(a*(SizePadre-1)),CampoPadre,(SizePadre-1))) {test=ON; break;}
						if (!memcmp(PtFiglio+(a*(SizePadre-1)),CampoPadre,strlen(CampoPadre)))
						{
							test=ON;
							break;
						}
					}

					break;
			}

			if (test)
			{
			// -------------------------------------------------------------
			//                                                             !
			//                   ModalitÖ test per delete                  !
			//                                                             !
			// -------------------------------------------------------------

				if (!Update)
					break;

			// -------------------------------------------------------------
			//                                                             !
			//         MODALITA' UPDATE CAMPO VECCHIO CON CAMPO NUOVO      !
			//                                                             !
			// -------------------------------------------------------------
			// Se sono arrivato quÖ ä perche il campo clone "VECCHIO "padre ä
			// uguale al campo clone "VECCHIO" figlio
			// Si verificherÖ ora se esistono altri campi di Link
			// Prioritari (es. CODICE) che confermano che il record ä
			// sicuramente in riferimento all'altro
			// Se cosç non fosse il programma lo prende per buono

				for (Cpt=PrimoFiglio;Cpt<=FineFiglio;Cpt++)
				{
					if (Cpt==pt)
						continue; // Se sono me vai avanti

					memo_leggivar(adb_HLinkHdl,Cpt*sizeof(adbL2),&adbL2,sizeof(adbL2));

//				printf("Controllo figli %ld:\n",Cpt);

				// ----------------------------------------------
				// Controlla se ci sono altri campi linkati     !
				// di tipo assoluto                             !
				// ----------------------------------------------
				//printf("File [%s]=[%s]?\n",adbL.FileFiglio,adbL2.FileFiglio);

					if (!strcmp(adbL.FileFiglio,adbL2.FileFiglio))
					{
						//printf(" OK + \n");
						if (adbL2.Clone)
							continue;
					// Trovato un riferimento assoluto

					//	 bisogna comparare i due
					// il campo figlio nuovo ä gia
					// Al campo padre bisogna applicare l'opzione

						Opz2=-1;

						if (strlen(adbL2.Opzione)==0)
							Opz2=0;

					// -------------------------
					// Opzione di SCAN         !
					// -------------------------
						if (!strcmp(adbL2.Opzione,"SCAN"))
							PRG_end("HLaction 1");

					// -------------------------
					// Opzione di AGGIUNTA     !
					// -------------------------
						agg=OPZ2(adbL2.Opzione);

						if (agg!=NULL)
							Opz2=2;

						if (Opz2==-1)
						{
							PRG_end("HLaction2: Opzione ? %s",adbL2.Opzione);
							//PRG_end(DescFile);
						}

						SizeFiglio2=adb_FldSize(HdlFiglio,adbL2.FieldNameFiglio);

					//SizePadre2=adb_FldSize(Hdb,adbL2.FieldNamePadre);
						if ((SizeFiglio2+1)>sizeof(CampoPadre2))
							PRG_end("HLaction 2");

						memset(CampoPadre2,0,sizeof(CampoPadre2));

						if (Opz2==2)
							strcpy(CampoPadre2,agg);

						strcat(CampoPadre2,adb_FldPtr(Hdb,adbL2.FieldNamePadre));

						if (strlen(CampoPadre2) > (UINT) (SizeFiglio2-1))
							PRG_end("HLaction2:CampoPadre > SizeFiglio");

					// A questo punto dovrai avere in -campopadre2-
					// il campo da confrontare

					//printf("CampoPadre [%s] CampoFiglio [%s] ",adbL2.FieldNamePadre,adbL2.FieldNameFiglio);
					//printf("[%s] = [%s] ?\n",CampoPadre2,adb_FldPtr(HdlFiglio,adbL2.FieldNameFiglio));
					// Se sono diversi, questo record ä da saltare
						if (strcmp(CampoPadre2,adb_FldPtr(HdlFiglio,adbL2.FieldNameFiglio)))
						{
							test=OFF; // Passa subito ad un altro record
							break;
						} // Non bisogna fare l'update
					} // Fine del loop di ricerca

					if (test==OFF)
						break;
				}

			//if (test) printf("Faccio l'update\n"); else printf("--\n");

			// ----------------------------------------------------
			//   MEMORIZZO IL RECORD DA UPDATARE in SECONDO TEMPO !
			// ----------------------------------------------------

				if (test)
				{//printf("Vecchio [%s]=[%s]",adbL.FieldNameFiglio,adb_FldPtr(HdlFiglio,adbL.FieldNameFiglio));
				 //adb_FldWrite(HdlFiglio,adbL.FieldNameFiglio,adb_FldPtr(Hdb,adbL.FieldNamePadre),0);
				 //printf("Nuovo  [%s]\n",adb_FldPtr(Hdb,adbL.FieldNamePadre));

				 // Dovrebbe essere adb_recnum*4
					if (MemoUP==-1)
					{
						MemoUP=memo_chiedi(RAM_AUTO,adb_recnum(HdlFiglio),"ADB:MemoUP");
						if (MemoUP<0) PRG_end("Memo ? MemoUP");
					}

					adb_position(HdlFiglio,&HrecUP);

					ScriviUP(MemoUP,NumeroUpdate,HrecUP); NumeroUpdate++;
				}

				test=OFF;
			}

		 // ---------------------------------------------
		 // Controlla se girare ancora                  -
		 // ---------------------------------------------

		 // Se cä un Dbase sul campo
			if (FlagFind)
			{
				if (strcmp(PtFiglio,CampoPadre)>0)
					break; // Superato il limite

				if (adb_Afind(HdlFiglio,adbL.IndexNameFiglio,B_GET_NEXT,"",B_RECORD))
					break;
			}
			else // Se NON cä un Dbase sul campo
			{
				if (adb_find(HdlFiglio,0,B_GET_NEXT,"",B_RECORD))
					break;
			}
		}


		// -----------------------------------------
		// SE CI SONO UPDATE LI FACCIO FISICAMENTE !
		// -----------------------------------------

		if (Update && NumeroUpdate)
		{
			for (Cpt=0;Cpt<NumeroUpdate;Cpt++)
			{
				HrecUP=LeggiUP(MemoUP,Cpt);
				adb_get(HdlFiglio,HrecUP,-1);

				switch (FldFiglio->tipo)
				{
					case ADB_ALFA:
						adb_FldWrite (HdlFiglio,adbL.FieldNameFiglio,adb_FldPtr (Hdb,adbL.FieldNamePadre),0);
						break;

					case ADB_DATA:
						adb_FldWrite (HdlFiglio,adbL.FieldNameFiglio,adb_data (adb_FldPtr (Hdb,adbL.FieldNamePadre)),0);
						break;

					case ADB_NUME:
						adb_FldWrite (HdlFiglio,adbL.FieldNameFiglio,"",atof (adb_FldPtr (Hdb,adbL.FieldNamePadre)));
						break;
				}

				adb_update(HdlFiglio);

			}

			memo_libera(MemoUP,"MemoUP"); MemoUP=-1;
		}

		vaiavanti:
	 // -----------------------------------
	 //   CHIUSURA DEL FILE               !
	 // -----------------------------------

		if (HDF)
			adb_close(HdlFiglio);

		if (CampoPadreHdl!=-1)
		{
			memo_libera(CampoPadreHdl,"HLaction1");
			CampoPadreHdl=-1;
		}

		if (test)
			break;
	}


// if (!test) win_info("Record libero");

	if (CampoPadreHdl!=-1)
		memo_libera(CampoPadreHdl,"HLaction2");

	if (MemoUP!=-1)
	{
		memo_libera(MemoUP,"MemoUP");
	}

 //FTIME_off(HLcart);

	if (Update)
		return NULL;

	if (test)
		return DescFile;
	else
		return NULL;
}
//  +-----------------------------------------∏
//	| HLcart   - Funzione FTIME per HLaction  |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+

// Controlla se esiste un collegamento da padre
static SINT adb_HLsonopadre(SINT Hdb)
{
	CHAR *p;
	CHAR FilePadre[MAXPATH];

	LONG pt;

	if (!ADB_HL) return OFF;
	if (adb_HLinkHdl==-1) PRG_end("HL:1 ?");

	strcpy(FilePadre,fileName(ADB_info[Hdb].FileName));

	p = strstr(FilePadre,".");

	if (p != NULL)
		*p=0;

	return adb_HLfind (FilePadre,&pt);
}

//  +-----------------------------------------∏
//	| ADB_LockControl                         |
//	| Controllo sulla chiusura di un record   |
//	|                                         |
//	|                                         |
//	|                           by FerrÖ 1996 |
//	+-----------------------------------------+

static SINT adb_LockControl(SINT Flag,SINT Hadb,SINT status,CHAR *tip,SINT Escape)
{
 SINT TempoAttesa=100;
 static SINT Tentativi=0;
 static SINT FuncStatus=OFF;
 SINT ffg=OFF;

 if ((status<0)||(status>107)) win_errgrave("BTI_err:Status non gestito");

 switch (Flag)
	{
	 case ON  : 
				if (ADB_lock==0) return OFF;
				pausa(TempoAttesa);
				return ON;

	 case OFF  : Tentativi=OFF;
				 if (FuncStatus)
				 {
					FuncStatus=OFF;
				 }
				 break;
 }
 return OFF;
}

// +-----------------------------------------∏
// | adb_OwnerSet Setta il proprietario ed   |
// |              un encrypting del file     |
// |                                         |
// |  file      = Hadb del file              |
// |  client    = Nome del proprietario      |
// |  mode      = Accesso consentito         |
// |              0 = NO W/R  non encrypt    |
// |              1 = ONLY R  non encrypt    |
// |              2 = NO W/R  ENCRYPT        |
// |              3 = ONLY R  ENCRYPT        |
// |                                         |
// |                           by FerrÖ 1996 |
// +-----------------------------------------+
SINT adb_OwnerSet(SINT Hadb,CHAR *Client,SINT Mode)
{
	BTI_WORD dataLen;
	SINT status;
	CHAR *p;
	SINT Hdl;
	CHAR *File;
	//CHAR file[MAXPATH];

	Hdl=memo_chiedi(RAM_HEAP,MAXPATH,"OSet");
	File=memo_heap(Hdl);

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("ownerset",Hadb);

	dataLen=strlen(Client) + 1;
	status = BTRV( 29,
				  ADB_info[Hadb].PosBlock,
				  Client, // Record dove andare
				  &dataLen,// Lunghezza buffer
				  Client,
				  (WORD) Mode);

	if (status) BTI_err(ADB_info[Hadb].FileName,"OWNERSET:",status);

	memset (File,0,MAXPATH);

	p = strstr (ADB_info[Hadb].FileName,"\\");

	if (strlen (p))
	{
		p++;

		p = strstr (p,"\\");
		p++;

		strncpy (File,p,strlen (p) - 4);
	}
	else
		strncpy (File,ADB_info[Hadb].FileName,strlen (ADB_info[Hadb].FileName) - 4);

// Aggiunge il nome del proprietario nei link

	if (ADB_HL)
	{
		if (adb_HLinkHdl == -1) adb_HLapri();
		adb_HLinsertPro (File,Client);
	}

	memo_libera(Hdl,"OSet");
	return status;
}


SINT adb_OwnerClear(SINT Hadb)
{
	BTI_WORD dataLen;
	SINT status;

	CHAR *p;
	SINT Hdl;
	CHAR *File;
	//CHAR file[MAXPATH];

	if (((Hadb<0)||(Hadb>=ADB_max))||
			(ADB_info[Hadb].HdlRam==-1)) adb_errgrave("ownerclear",Hadb);

	Hdl=memo_chiedi(RAM_HEAP,MAXPATH,"OClr");
	File=memo_heap(Hdl);

	dataLen=0;
	status = BTRV( 30,
								 ADB_info[Hadb].PosBlock,
								 "", // Record dove andare
								 &dataLen,// Lunghezza buffer
								 "",
								 0);

	if (status) BTI_err(ADB_info[Hadb].FileName,"OWNERCLEAR:",status);

	memset (File,0,MAXPATH);

	p = strstr (ADB_info[Hadb].FileName,"\\");

	if (strlen (p))
	{
		p++;

		p = strstr (p,"\\");
		p++;

		strncpy (File,p,strlen (p) - 4);
	}
	else
		strncpy (File,ADB_info[Hadb].FileName,strlen (ADB_info[Hadb].FileName) - 4);

// Aggiunge il nome del proprietario nei link

	if (ADB_HL)
	{
		if (adb_HLinkHdl == -1) adb_HLapri();
		adb_HLinsertPro (File,"");
	}

	memo_libera(Hdl,"OClr");
	return status;
}

CHAR *ConvertoPro (CHAR *Proprietario)
{
	SINT a;
	SINT Tabella[]={175,158,196,135,169,187,199,185,142,177,121,190,129,157,138,151,185,169,153,190,178,195,158,124,188,132,149,192,171,183};

	static CHAR Ritorno[31];

	for (a = 0;a < (SINT) strlen (Proprietario);a++)
		Ritorno[a] = Proprietario[a] ^ Tabella[a];

	Ritorno[a] = '\0';

	return Ritorno;
}

#define BTI_ERR        20                  /* record manager not started */
#define BTR_INTRPT     0x7B                  /* Btrieve interrupt vector */
#define BTR_OFFSET     0x33             /* Btrieve offset within segment */
#define VARIABLE_ID    0x6176   /* id for variable length records - 'va' */
#define PROTECTED_ID   0x6370            /* id for protected call - 'pc' */
#define VERSION_OFFSET 0
#define BTRVID_CODE    257
//#define LTRUE          1
//#define FALSE          0

typedef unsigned char  BOOLEAN;

	extern struct DRV_COM *DRV;


//   +-------------------------------------------∏
//   | CHIAMATA al BTRV                          |
//   | Windows Version                           |
//   |             by FerrÖ Art & Tecnology 1996 |
//   +-------------------------------------------+

/*************************************************************************
**
**  Copyright 1982-1995 Btrieve Technologies, Inc. All Rights Reserved
**
*************************************************************************/
/*************************************************************************
   BTRAPI.C
     This module implements the Btrieve Interface for C/C++ applications
     using MS Windows, Win32s, Windows NT, OS2, DOS, Extended DOS.  An NLM
     application does not need to compile and link this module.

     You must select a target platform switch.  See the prologue inside
     'btrapi.h' for a list of platform switches.

     IMPORTANT
     ---------
     Btrieve Technologies, Inc., invites you to modify this file
     if you find it necessary for your particular situation.  However,
     we cannot provide technical support for this module if you
     do modify it.

*************************************************************************/
#if !defined(BTI_WIN) && !defined(BTI_OS2) && !defined(BTI_DOS) \
&& !defined(BTI_NLM) && !defined(BTI_DOS_32R) && !defined(BTI_DOS_32P) \
&& !defined(BTI_DOS_32B) && !defined(BTI_WIN_32) && !defined(BTI_OS2_32)
#error You must define one of the following: BTI_WIN, BTI_OS2, BTI_DOS, BTI_NLM, BTI_DOS_32R, BTI_DOS_32P, BTI_DOS_32B, BTI_WIN_32, BTI_OS2_32
#endif

/*
#include <btrconst.h>
#include <btrapi.h>
#include <string.h>
*/

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
   RQSHELLINIT() Function for MS Windows
***************************************************************************/
//#if defined( BTI_WIN ) || defined( BTI_WIN_32 )
BTI_API RQSHELLINIT( BTI_CHAR_PTR option )
{
   return WBRQSHELLINIT( option );
}
//#endif

/****************************************************************************
   BTRV() Function for NT, Win32s, or 32-bit OS2
***************************************************************************/
//#if defined( BTI_WIN_32 ) || defined( BTI_OS2_32 )
//#if defined( BTI_OS2_32 )
//#define BTRCALL BTRCALL32
//#endif
BTI_API BTRV(
           BTI_WORD     operation,
           BTI_VOID_PTR posBlock,
           BTI_VOID_PTR dataBuffer,
           BTI_WORD_PTR dataLength,
           BTI_VOID_PTR keyBuffer,
           BTI_SINT     keyNumber )
{
   BTI_BYTE keyLength  = MAX_KEY_SIZE;
   BTI_CHAR ckeynum    = (BTI_CHAR)keyNumber;
   BTI_ULONG dataLen32 = 0;
   BTI_SINT status;

   if ( dataLength != NULL )
      dataLen32 = *dataLength;

   status = BTRCALL (
              operation,
              posBlock,
              dataBuffer,
              &dataLen32,
              keyBuffer,
              keyLength,
              ckeynum );

   if ( dataLength != NULL ) *dataLength = (BTI_WORD) dataLen32;

   return status;
}



/****************************************************************************
   BTRVID() Function for NT, Win32s or 32-bit OS2
***************************************************************************/
//#if defined( BTI_WIN_32 ) || defined( BTI_OS2_32 )
//#if defined( BTI_OS2_32 )
//#define BTRCALLID BTRCALLID32
//#endif
BTI_API BTRVID(
           BTI_WORD       operation,
           BTI_VOID_PTR   posBlock,
           BTI_VOID_PTR   dataBuffer,
           BTI_WORD_PTR   dataLength,
           BTI_VOID_PTR   keyBuffer,
           BTI_SINT       keyNumber,
           BTI_BUFFER_PTR clientID )
{
   BTI_BYTE keyLength  = MAX_KEY_SIZE;
   BTI_CHAR ckeynum    = (BTI_CHAR)keyNumber;
   BTI_ULONG dataLen32 = 0;
   BTI_SINT status;

   if ( dataLength != NULL ) dataLen32 = *dataLength;

   status = BTRCALLID (
              operation,
              posBlock,
              dataBuffer,
              &dataLen32,
              keyBuffer,
              keyLength,
              ckeynum,
              clientID );

   if ( dataLength != NULL )
      *dataLength = (BTI_WORD)dataLen32;

   return status;
}
//#endif

#ifdef __cplusplus
}
#endif



// -------------------------------------------
// BLOB Function
// Gestione del BLOB (VARCHAR)
// Stringa a lunghezza variabile
//								Ferr‡ A&T 2000
// -------------------------------------------

// Ritorna la dimensione del campo Blob + 1 (x Zstring)
SINT adb_BlobSize(HDB Hdb,CHAR *lpField) 
{
	ADB_STRUCTBLOB *lpBlob;
	struct ADB_REC *Fld; 
	lpBlob=(ADB_STRUCTBLOB *) adb_FldInfo(Hdb,lpField,&Fld);
	if (lpBlob==NULL) PRG_end("adb_BlobSize: %s ? in %s",lpField,ADB_info[Hdb].FileName);
	if (Fld->tipo!=ADB_BLOB) PRG_end("adb_BlobCopy: %s ? in %s (NOBLOB) ",lpField,ADB_info[Hdb].FileName);
	return (lpBlob->iLen+1); 
}

// -------------------------------------------------------
// Copia il BLOB letto in una Ztring puntata da lpBuffer
// Ritorna FALSE=Tutto OK
//         TRUE =Buffer insufficiente
//
BOOL adb_BlobCopy(HDB Hdb,CHAR *lpField,CHAR *lpBuffer,SINT iSizeBuf) 
{
	CHAR *lpSorg;
	SINT iSizeSource;
	BOOL fRet=FALSE;
	struct ADB_REC *Fld; 

	ADB_STRUCTBLOB *lpBlob;
	lpBlob=(ADB_STRUCTBLOB *) adb_FldInfo(Hdb,lpField,&Fld);
	if (lpBlob==NULL) PRG_end("adb_BlobCopy: %s ? in %s",lpField,ADB_info[Hdb].FileName);
	if (Fld->tipo!=ADB_BLOB) PRG_end("adb_BlobCopy: %s ? in %s (NOBLOB) ",lpField,ADB_info[Hdb].FileName);
	lpSorg=adb_recbuf(Hdb);	*lpBuffer=0; if (!lpBlob->iLen) return 0;
	iSizeSource=lpBlob->iLen; 
	if (iSizeSource>=iSizeBuf) {iSizeSource=iSizeBuf-1; fRet=TRUE;}
	memcpy(lpBuffer,lpSorg+lpBlob->iOfs,iSizeSource);
	* (lpBuffer+iSizeSource)=0; // Zero di fine stringa
	return fRet;
}

// ------------------------------------------------
// Alloca e ritorna una stringa BLOB
// Ritorna il Puntatore alla stringa
// RICORDARSI di liberarlo con Free
//
CHAR *adb_BlobGetAlloc(HDB Hdb,CHAR *lpField) 
{
	SINT iSize;
	CHAR *lpDest;
	iSize=adb_BlobSize(Hdb,lpField);
	lpDest=EhAlloc(iSize);
	adb_BlobCopy(Hdb,lpField,lpDest,iSize);
	return lpDest;
}

void adb_BlobFree(CHAR *lpField) 
{
	EhFree(lpField);
}

// ------------------------------------------------
// Ritorna il primo offset per campi dinamici
// ------------------------------------------------
static SINT adb_BlobOfsDyn(HDB Hdb)
{
	struct ADB_REC *Field;
	CHAR *lp;
	SINT a;
	SINT iOfsNew=0;

	Field=(struct ADB_REC *) farPtr((CHAR *) ADB_info[Hdb].AdbHead,sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		lp=farPtr(ADB_info[Hdb].RecBuffer,Field[a].pos);
		if ((Field[a].pos+Field[a].RealSize)>iOfsNew) iOfsNew=Field[a].pos+Field[a].RealSize;
	}
	return iOfsNew;
}

static SINT adb_BlobNewOfs(HDB Hdb,SINT iSize)
{
	struct ADB_REC *Field;
	ADB_STRUCTBLOB *lpBlob;
	CHAR *lp;
	SINT a;
	SINT iOfsNew=0;

	Field=(struct ADB_REC *) farPtr((CHAR *) ADB_info[Hdb].AdbHead,sizeof(struct ADB_HEAD));

	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		lp=farPtr(ADB_info[Hdb].RecBuffer,Field[a].pos);
		if ((Field[a].pos+Field[a].RealSize)>iOfsNew) iOfsNew=Field[a].pos+Field[a].RealSize;
		
		if (Field[a].tipo==ADB_BLOB)
		{
			lpBlob=(ADB_STRUCTBLOB *) lp;
			if ((lpBlob->iOfs+lpBlob->iLen)>iOfsNew) iOfsNew=lpBlob->iOfs+lpBlob->iLen; 
		}
	}

	if ((iOfsNew+iSize)>ADB_info[Hdb].AdbHead->wRecSize) PRG_end("BLOB newOfs:Overload dinamic recsize Hdb=%d (%d/%d/%d)",Hdb,iOfsNew,iSize,ADB_info[Hdb].AdbHead->wRecSize);
	return iOfsNew;
}

static void adb_BlobReset(HDB Hdb,CHAR *lpField)
{
	ADB_STRUCTBLOB *lpBlobDel,*lpBlob;
	struct ADB_REC *Field;
	CHAR *lp;
	SINT a;
	SINT iOfsLast;
	SINT iOfsDyn;
	CHAR *lpDest;
	lpDest=adb_recbuf(Hdb);

	lpBlobDel=(ADB_STRUCTBLOB *) adb_FldPtr(Hdb,lpField);
	if (!lpBlobDel->iOfs) return; // Non Ë da azzerare

	// Cerco l'ultimo di memoria dinamica da spostare
	iOfsLast=adb_BlobNewOfs(Hdb,0);
	iOfsDyn=adb_BlobOfsDyn(Hdb);
	if (iOfsLast==iOfsDyn) return;
	if (iOfsLast==(lpBlobDel->iOfs+lpBlobDel->iLen)) return; // Era l'ultimo Blob Dinamico
//	win_infoarg("Rst:iOfsLast=%d",iOfsLast);
	if ((lpBlobDel->iOfs<0)||(lpBlobDel->iLen<0)) return;
	memmove(lpDest+lpBlobDel->iOfs,lpDest+lpBlobDel->iOfs+lpBlobDel->iLen,iOfsLast-(lpBlobDel->iOfs+lpBlobDel->iLen));
	
	Field=(struct ADB_REC *) farPtr((CHAR *) ADB_info[Hdb].AdbHead,sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		lp=farPtr(ADB_info[Hdb].RecBuffer,Field[a].pos);
		if (Field[a].tipo==ADB_BLOB)
		{
			lpBlob=(ADB_STRUCTBLOB *) lp;
			if (lpBlob->iOfs>lpBlobDel->iOfs)  
				{
					lpBlob->iOfs-=lpBlobDel->iLen;
					/*
					win_infoarg("[%s/%d] da %d - %d",
								 Field[a].desc,
								 iOfsDyn,
								 lpBlob->iOfs,
								 lpBlob->iLen);
								 */
				}
		}
	}
}

static void adb_BlobFldWrite(SINT Hdb,CHAR *lpField,CHAR *lpBuffer)
{
	ADB_STRUCTBLOB *lpBlob;
	CHAR *lpDest;
	lpBlob=(ADB_STRUCTBLOB *) adb_FldPtr(Hdb,lpField);

	// Se il Blob esisteva gi‡ lo svuoto
	//win_infoarg("BFW:%d) [%s=%s] %d/%d",Hdb,lpField,lpBuffer,lpBlob->iLen,lpBlob->iOfs);
	if (lpBlob->iLen&&lpBlob->iOfs) adb_BlobReset(Hdb,lpField);
	if (!*lpBuffer) lpBuffer=" ";
	lpBlob->iLen=strlen(lpBuffer); if (!lpBlob->iLen) {lpBlob->iOfs=0; return;} // Stringa vuota

	lpBlob->iOfs=adb_BlobNewOfs(Hdb,lpBlob->iLen);
//	win_infoarg("BFW:Assign[%s=%s] >%d (%d)",lpField,lpBuffer,lpBlob->iOfs,adb_BlobOfsDyn(Hdb));
	
	lpDest=adb_recbuf(Hdb);
	memcpy(lpDest+lpBlob->iOfs,lpBuffer,lpBlob->iLen);
}

void adb_BlobControl(HDB Hdb)
{
	ADB_STRUCTBLOB *lpBlob;
	struct ADB_REC *Field;
	CHAR *lp;
	SINT a;
	SINT iOfsLast;
	SINT iOfsDyn;

	iOfsLast=adb_BlobNewOfs(Hdb,0);
	iOfsDyn=adb_BlobOfsDyn(Hdb);
	
	Field=(struct ADB_REC *) farPtr((CHAR *) ADB_info[Hdb].AdbHead,sizeof(struct ADB_HEAD));
	for (a=0;a<ADB_info[Hdb].AdbHead->Field;a++)
	{
		lp=farPtr(ADB_info[Hdb].RecBuffer,Field[a].pos);
		if (Field[a].tipo==ADB_BLOB)
		{
			lpBlob=(ADB_STRUCTBLOB *) lp;
			win_infoarg("Controllo\n[%s/%d] loc:%d len:%d next:%d",Field[a].desc,iOfsDyn,lpBlob->iOfs,lpBlob->iLen,lpBlob->iOfs+lpBlob->iLen);
		}
	}
}

// ----------------------------
// FINE BLOB FUNCTIONS
// ----------------------------
