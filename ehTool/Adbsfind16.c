//   +-------------------------------------------+
//   | ADB_ASFIND Special FInd                	 |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+
#include "\ehtool\include\ehsw_idb.h"

//#include <btrapi.h>
//#include <btrconst.h>

 extern struct ADB_INFO *ADB_info;
 extern SINT ADB_hdl;
 extern SINT ADB_ult; // Numero di MGZ
 extern SINT ADB_max;
 extern SINT ADB_network;
 extern SINT EHPower;

 extern SINT ADB_network;   // ON/OFF se si sta usando un NetWork
 extern SINT  ADB_HL;   // Se il Link gerarchico Š attivo
 extern CHAR HLtabella[];  // Percorso e nome della Tabella Gerarchica
 extern LONG adb_HLink;    // Numero di Link Gerarchici
 extern SINT  adb_HLinkHdl; // Handle che contiene la tabella

 extern SINT  ADB_lock;    // Flag di comportamento in lock

 extern struct IPTINFO *IPT_info;
 extern SINT IPT_ult; // Numero di input aperti

//   +-------------------------------------------+
//   | ADB_ASFIND Cerca una chiave composta   	 |
//   |            in un indice stabilito         |
//   |                                           |
//   |  Hadb : Handle del file ADB               |
//   |  idx  : Nomde dell'indice da usare        |
//   |  (idx  Nome dell'indice per Afind)        |
//   |  modo : modo BTRIEVE di ricerca           |
//   |                                           |
//   |        B_GET_EQUAL se uguale              |
//   |        B_GET_GT    se Š + grande          |
//   |        B_GET_GE    se Š + grande o uguale |
//   |        B_GET_LT    se Š minore            |
//   |        B_GET_LE    se + minore o uguale   |
//   |                                           |
//   |  NumPt: Numero dei campi che compongono   |
//   |         la chiave di ricerca              |
//   |  ...    Elenco dei campi                  |
//   |                                           |
//   |                                           |
//   |  Ritorna 0 = Tutto ok                     |
//   |          > 0 Errore BTRIEVE               |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+
static SINT Ladb_Sfind(SINT Hdb,SINT Idx,SINT modo,SINT NumPt,va_list Ah);

SINT adb_Sfind(SINT Hdb,SINT Idx,SINT modo,SINT NumPt,...)
{
	va_list Ah;
	SINT Ret;
	va_start(Ah,NumPt);
	Ret=Ladb_Sfind(Hdb,Idx,modo,NumPt,Ah);
	va_end(Ah);
	return Ret;
}

SINT adb_ASfind(SINT Hdb,CHAR *NomeIndex,SINT modo,SINT NumPt,...)
{
	va_list Ah;
	SINT Idx,Ret;
	va_start(Ah,NumPt);

	Idx=adb_indexnum(Hdb,NomeIndex);
//	win_infoarg(">[%s] [%s]",Ah,va_arg(Ah,CHAR *));
	if (Idx==-1) PRG_end("AS [%d/%s] ?",Hdb,NomeIndex);
	Ret=Ladb_Sfind(Hdb,Idx,modo,NumPt,Ah);
	va_end(Ah);
	return Ret;
}

static SINT Ladb_Sfind(SINT Hdb,SINT Idx,SINT modo,SINT NumPt,va_list Ah)
//SINT adb_Sfind(SINT Hdb,SINT Idx,SINT modo,SINT NumPt,...)
{
	SINT   a;
	SINT   Hdl;
	WORD   wSizeBuf;
	CHAR  *lpBuf,*p;
//	va_list Ah;
	void   *pt;
//	SINT   Idx;
	SINT   Rt=-1;
	SINT   ptf;
	double valo;

	struct ADB_HEAD *Head;
	struct ADB_REC *Record;
	struct ADB_INDEX *Index;
	struct ADB_INDEX_SET *IndexSet;
	struct ADB_REC *Fld;

//	va_start(Ah,NumPt);

	Head=ADB_info[Hdb].AdbHead;
	Record=(struct ADB_REC *) (Head+1);
	Index=(struct ADB_INDEX *) (Record+Head->Field);
	IndexSet=(struct ADB_INDEX_SET *) (Index+Head->Index);

	// Chiedo e pulisco la memoria necessaria
	wSizeBuf=adb_keymaxsize(Hdb);
	Hdl=memo_chiedi(RAM_HEAP,wSizeBuf,"ASFIND");
	lpBuf=memo_heap(Hdl); memset(lpBuf,0,wSizeBuf);

//	Idx=adb_indexnum(Hdb,NomeIndex);
//	if (Idx==-1) PRG_end("AS [%s] ?",NomeIndex);

	// Cerco il primo puntatore
	for (ptf=0;ptf<Head->IndexDesc;ptf++)
	 {
		 if (!strcmp(Index[Idx].IndexName,IndexSet[ptf].IndexName)) break;
	 }
	ptf++;

	p=lpBuf;
	// Scrivo i campi nell'ordine in cui la trovo
	for (a=0;a<NumPt;a++)
	{
	 // Carica i parametri del campo
	 if (adb_FldInfo(Hdb,IndexSet[ptf+a].FieldName,&Fld)==NULL) PRG_end("AS:NULL [%s]",IndexSet[ptf+a].FieldName);

	 // Scrive la chiave di ricerca
	 switch (Fld->tipo)
		 {
			case ADB_DATA:
			case ADB_ALFA: pt=va_arg(Ah,CHAR *);
						   adb_FldCopy(p,Hdb,IndexSet[ptf+a].FieldName,pt,0);
						   break;
			case ADB_NUME:
			case ADB_FLAG:
			case ADB_INT :
			case ADB_INT32:
			case ADB_COBN:
			case ADB_COBD:
			case ADB_AINC:
			case ADB_FLOAT:

						  valo=va_arg(Ah,double);
						  adb_FldCopy(p,Hdb,IndexSet[ptf+a].FieldName,NULL,valo);
						  break;
										 //	valo=va_arg(Ah,SINT);
											//adb_FldCopy(p,Hdb,IndexSet[ptf+a].FieldName,
											//break;
		 }
	 p+=adb_FldSize(Hdb,IndexSet[ptf+a].FieldName);
	}

	Rt=adb_find(Hdb,Idx,modo,lpBuf,B_RECORD|B_NOSTR);
	memo_libera(Hdl,"ASFIND");
	return Rt;
}

//   +-------------------------------------------+
//   | ADB_HDLFIND Ricerca L'HDL di un file   	 |
//   |             gi… aperto in memoria         |
//   |                                           |
//   |  tchar: Nome del File                     |
//   |                                           |
//   |  Ritorna L'hdl o si ferma                 |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+

SINT adb_HdlFind(CHAR *File,BOOL Flag)
{
	CHAR *p;
	SINT a;

	for (a=0;a<ADB_ult;a++)
	{
//	 printf("[%s]=[%s]?\n",ADB_info[a].FileName,File); getch();
	 if (ADB_info[a].FileName==NULL) continue;
	 p=rstrstr(ADB_info[a].FileName,".");  if (p) *p=0;
	 if (!strcmp(ADB_info[a].FileName,File)) 
	 {if (p) *p='.'; 
	  return a;
	 }
	 if (p) *p='.';
	}
	if (Flag) 
	{
		/*
#ifdef _WIN32
		win_info("Errore in HdlFind");
		for (a=0;a<ADB_ult;a++)
		{
			win_infoarg("[%s] == [%s]",ADB_info[a].FileName,File);
		}
#endif
		*/
			PRG_end("HdlFind [%s] ?",File);
	}
	return -1;
}

//   +-------------------------------------------+
//   | ADB_OpenFind/CloseFind                 	 |
//   | Gestisce l'apertura e la chisura          |
//   | controllata di un dbase                   |
//   |                                           |
//   |  Ritorna L'hdl o si ferma                 |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+

BOOL adb_OpenFind(CHAR *File,CHAR *User,
				  SINT FlagOpen,
				  SINT *Hdb,
				  BOOL *DaChiudere)
{
	SINT Hdl;
	BOOL DaClose=FALSE;

	if (DaChiudere) *DaChiudere=FALSE;
	Hdl=adb_HdlFind(File,FALSE);
	if (Hdl==-1)
	{if (adb_open(File,User,FlagOpen,&Hdl)) return TRUE;
	 DaClose=TRUE;
	}

	*Hdb=Hdl;
	if (DaChiudere) *DaChiudere=!DaClose;
	if (!DaClose) ADB_info[Hdl].OpenCount++;// Conteggio delle aperture}
	return FALSE;
}

void adb_CloseFind(SINT Hdb)
{
 //printf("Hdb %d [%d]\n",Hdb,ADB_info[Hdb].OpenCount); getch();
 if (ADB_info[Hdb].OpenCount>0)
	 {
		ADB_info[Hdb].OpenCount--;// Conteggio delle aperture
		if (!ADB_info[Hdb].OpenCount) {adb_close(Hdb);}
	 }
	 else PRG_end("acf error ?");
}


void adb_CloseAllFind(void)
{
 SINT a;
 for (a=0;a<ADB_max;a++) adb_CloseFind(a);
}


