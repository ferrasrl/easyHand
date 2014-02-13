
void SdbPreset(	struct OBJ *poj,
				CHAR *ObjName,
				SINT (*FunzDisp)(SINT cmd,CHAR *PtDati,
												LONG  dato,void  *str,
												SINT Hdb,SINT IndexNum),
				SINT Hdb,
				SINT Index,
				SINT Filter,
				void *dato);

void *SdbDriver(EH_OBJPARAMS);

//
// Posizione dell progress bar
//
#define SDB_PRG_OFF 0
#define SDB_PRG_TL 1
#define SDB_PRG_TR 2
#define SDB_PRG_BL 3
#define SDB_PRG_BR 4 
void SdbPreset32(struct OBJ *pojStruct,
			     CHAR *ObjName,
			     SINT (*FunzDisp)(SINT cmd,CHAR *PtDati,LONG  dato,void  *str,SINT Hdb,SINT IndexNum),
			     SINT Hdb,
			     SINT Index,
			     SINT Filter,
			     void *dato,
				 DWORD wParam);
void *SdbDriver32(EH_OBJPARAMS);

SINT SdbDeco(SINT cmd,
			 CHAR *lpDati,
			 LONG dato,
			 void *str,
			 SINT Hdb,
			 SINT IndexNum);
