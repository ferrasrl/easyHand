//   +-------------------------------------------+
//   | RSUTIL   Utilit… sulle risorse            |
//   |          Salvattaggio/Caricamento         |
//   |          delle risorse                    |
//   |                                           |
//   |             10 Giugno                     |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+
/*
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <mem.h>
#include <dir.h>
#include <time.h>

#include <flm_main.h>
#include <flm_vid.h>
  */
#include "\ehtool\include\ehsw_i.h"
#include "c:\ehtool\rsutil.h"

typedef struct {
	SINT Ver;
	SINT Obj;
	SINT Objs;
	SINT ObjsMemo;
	SINT Ipt;
	SINT Hook;
} RSHEAD;

typedef struct {
		 CHAR NomeFld[30]; // Nome del campo
		 CHAR NomeObj[LUNNOMEOBJ+1];
		 SINT  NumIpt;
		 } ADB_HOOKS ;

static CHAR FileName[MAXPATH];

void RSSave(CHAR *File,
			struct OBJ *obj,
			struct IPT *ipt,
			struct ADB_HOOK *Hook,
			OBJS *Objs)
{
	FILE *pf;
	RSHEAD Rs;
	ADB_HOOKS Hooks;

	strcpy(FileName,File); 
#ifdef _WIN32
	strcat(FileName,".FRW");
#else
	strcat(FileName,".FRS");
#endif
	if (f_open(FileName,"wb+",&pf)) PRG_end("RSSave");

	Rs.Ver =101;
	Rs.Obj =0;
	Rs.Objs=0; Rs.ObjsMemo=0;
	Rs.Ipt =0;
	Rs.Hook=0;
	f_put(pf,0,&Rs,sizeof(Rs));

	if (obj)
	 {
		 for(;;)
		 {
			obj->ptr=NULL;
			obj->funcExtern=NULL;
			f_put(pf,NOSEEK,obj,sizeof(struct OBJ));
			Rs.Obj++;
			if (obj->tipo==STOP) break;
			obj++;
		 }
	 }

	if (ipt)
	 {
		 for(;;)
		 {
			f_put(pf,NOSEEK,ipt,sizeof(struct IPT));
			Rs.Ipt++;
			if (ipt->tipo==STOP) break;
			ipt++;
		 }
	 }

	if (Hook)
	 {
		 for(;;)
		 {
			memset(&Hooks,0,sizeof(Hooks));
			strcpy(Hooks.NomeFld,Hook->NomeFld);
			if (Hooks.NomeFld[0])
				{
				 strcpy(Hooks.NomeObj,Hook->NomeObj);
				 Hooks.NumIpt=Hook->NumIpt;
				}
			f_put(pf,NOSEEK,&Hooks,sizeof(Hooks));
			Rs.Hook++; if (!Hook->NomeFld[0]) break;
			Hook++;
		 }
	 }

	// ------------------------------------------------
	// Salvataggio degli Objs                         |
	// ------------------------------------------------
	if (Objs)
	 {
		 // -----------------------------------------------------
		 // Calcolo la memoria necessaria per contenere i dati  |
		 // -----------------------------------------------------
		 WORD PtMemo;
		 SINT a;
		 OBJS CopiaObjs;

		 // -----------------------------------------------------
		 // Salvo la struttura e calcolo la memoria necessaria  |
		 // -----------------------------------------------------

//		 printf("Write\n");
		 PtMemo=0;
		 for(a=0;;a++)
		 {
			 //printf("1) %d\n",a); getch();
			 memcpy(&CopiaObjs,Objs+a,sizeof(OBJS));
			 //CopiaObjs.lpChar1=(CHAR *) PtMemo;
				if ((Objs[a].iType==OS_TEXT)||(Objs[a].iType==OS_ICON))
				{
				 //printf("Stringa 1: %d [%s]\n",PtMemo,Objs[a].lpChar1); getch();
				 CopiaObjs.lpChar1=(CHAR *) PtMemo;
				 PtMemo+=strlen(Objs[a].lpChar1)+1;
				} else CopiaObjs.lpChar1=NULL;


				if (Objs[a].iType==OS_TEXT)
				{
				 //printf("Stringa 2: %d [%s]\n",PtMemo,Objs[a].lpChar2); getch();
				 CopiaObjs.lpChar2=(CHAR *) PtMemo;
				 PtMemo+=strlen(Objs[a].lpChar2)+1;
				} else CopiaObjs.lpChar2=NULL;


			f_put(pf,NOSEEK,&CopiaObjs,sizeof(OBJS));
			Rs.Objs++; if (Objs[a].iType==STOP) break;
		 }

		 //printf("OBJS %d",PtMemo); getch();
		 Rs.ObjsMemo=PtMemo;

		 // -----------------------------------------------------
		 // Salvo i Dati collegati alla struttura               |
		 // -----------------------------------------------------
		 for(a=0;;a++)
		 {
			 //printf("2) %d\n",a); getch();
			 if (Objs[a].iType==STOP) break;

				if ((Objs[a].iType==OS_TEXT)||(Objs[a].iType==OS_ICON))
				{
				 //printf("1) [%s] %d\n",Objs[a].lpChar1,strlen(Objs[a].lpChar1)+1;
				 //getch();
				 f_put(pf,NOSEEK,Objs[a].lpChar1,strlen(Objs[a].lpChar1)+1);
				}

				if (Objs[a].iType==OS_TEXT)
				{
				 //printf("1) [%s] %d\n",Objs[a].lpChar2,strlen(Objs[a].lpChar2)+1;
				 //getch();
				 f_put(pf,NOSEEK,Objs[a].lpChar2,strlen(Objs[a].lpChar2)+1);
				}
		 }
	 }

	//printf("Save [%d][%d][%d]",Rs.Obj,Rs.Ipt,Rs.Hook); getch();
	f_put(pf,0,&Rs,sizeof(Rs));
	f_close(pf);
	PRG_end("RS Record: OK");
}

SINT RSLoad(CHAR *File,
						struct OBJ **Robj,
						struct IPT **Ript,
						struct ADB_HOOK **RHook,
						OBJS   **RObjs)
{
	FILE *pf;
	RSHEAD Rs;
	struct OBJ *Obj;
	OBJS       *Objs;
	struct IPT *Ipt;
	ADB_HOOKS *HookS;
	struct ADB_HOOK *Hook;
	SINT Hdl;
	void *p;
	CHAR *lpObjsMemo;

	strcpy(FileName,File); 
#ifdef _WIN32
	strcat(FileName,".FRW");
#else
	strcat(FileName,".FRS");
#endif
	if (f_open(FileName,"rb",&pf)) return -1;

	f_get(pf,0,&Rs,sizeof(Rs));

	if (Rs.Ver!=101) PRG_end("RS Version [%d]",Rs.Ver);

//	printf("[%d][%d][%d]",Rs.Obj,Rs.Ipt,Rs.Hook); getch();
	Hdl=memo_chiedi(RAM_HEAP,
								 (Rs.Obj*sizeof(struct OBJ))+
								 (Rs.Objs*sizeof(OBJS))+Rs.ObjsMemo+
								 (Rs.Ipt*sizeof(struct IPT))+
								 (Rs.Hook*sizeof(struct ADB_HOOK))+
								 (Rs.Hook*sizeof(ADB_HOOKS)),file_name(FileName));

	Obj =NULL;
	Objs=NULL;
	Ipt =NULL;
	Hook=NULL;

	p=memo_heap(Hdl);

	if (Rs.Obj)  {Obj=p;  p=(CHAR *) (Obj+Rs.Obj);}
	if (Rs.Ipt)  {Ipt=p;  p=(CHAR *) (Ipt+Rs.Ipt);}
	if (Rs.Hook)
			{HookS=p; p=(CHAR *) (HookS+Rs.Hook);
			 Hook=p; p=(CHAR *) (Hook+Rs.Hook);
			}

	if (Rs.Objs) {Objs=p;
								lpObjsMemo=(CHAR *) (Objs+Rs.Objs);
								p=lpObjsMemo+Rs.ObjsMemo;
							 }

	if (Obj)  f_get(pf,NOSEEK,Obj,sizeof(struct OBJ)*Rs.Obj);
	if (Ipt)  f_get(pf,NOSEEK,Ipt,sizeof(struct IPT)*Rs.Ipt);

	if (Hook)
	 {SINT a;
		//printf("OK"); getch();
		f_get(pf,NOSEEK,HookS,sizeof(ADB_HOOKS)*Rs.Hook);
		for (a=0;a<Rs.Hook;a++)
			{
			 strcpy(Hook[a].NomeFld,HookS[a].NomeFld);
			 strcpy(Hook[a].NomeObj,HookS[a].NomeObj);
			 Hook[a].NumIpt=HookS[a].NumIpt;
			 //printf("[>%s]\n",Hook[a].NomeFld); getch();
			}
	 }

	if (Objs)
		{
		 WORD a,Offset;
		 f_get(pf,NOSEEK,Objs,sizeof(OBJS)*Rs.Objs);
		 f_get(pf,NOSEEK,lpObjsMemo,Rs.ObjsMemo);
		 for (a=0;a<Rs.Objs-1;a++)
			{
				if ((Objs[a].iType==OS_TEXT)||(Objs[a].iType==OS_ICON))
				{
				 Offset=(WORD) Objs[a].lpChar1;
				 Objs[a].lpChar1=lpObjsMemo+Offset;
				 //printf("Offset 1: %d [%s]\n",Offset,Objs[a].lpChar1); getch();
				}

				if (Objs[a].iType==OS_TEXT)
				{
				 Offset=(WORD) Objs[a].lpChar2;
				 Objs[a].lpChar2=lpObjsMemo+Offset;
				 //printf("Offset 2: %d [%s]\n",Offset,Objs[a].lpChar2); getch();
				}
			}
		}

	f_close(pf);

	if (Robj)  *Robj=Obj;
	if (Ript)  *Ript=Ipt;
	if (RHook) *RHook=Hook;
	if (RObjs) *RObjs=Objs;
//	getch(); PRG_end("");
	return Hdl;
}

void RSFree(SINT Hdl)
{
 memo_libera(Hdl,"RD");
}

void RSSetPtr(struct OBJ *obj,CHAR *Nome,CHAR *ptr[])
{
 for(;;)
	{
	 if (!strcmp(obj->nome,Nome)) {obj->ptr=ptr; break;}
	 if (obj->tipo==STOP) PRG_end("RSSetPtr:[%s] ?",Nome);
	 obj++;
	}
}

void RSSetSub(struct OBJ *obj,CHAR *Nome,
							void * (far *sub)(SINT cmd,LONG info,CHAR *str))

{
 for(;;)
	{
	 if (!strcmp(obj->nome,Nome)) {obj->funcExtern=sub; break;}
	 if (obj->tipo==STOP) PRG_end("RSSetPtr:[%s] ?",Nome);
	 obj++;
	}
}
