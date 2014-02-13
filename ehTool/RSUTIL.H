//   +-------------------------------------------+
//   | RSUTIL.H Utilit… sulle risorse            |
//   |          Salvattaggio/Caricamento         |
//   |          delle risorse                    |
//   |                                           |
//   |             10 Giugno                     |
//   |             by Ferr… Art & Tecnology 1998 |
//   +-------------------------------------------+

void RSSave(CHAR *File,
						struct OBJ *obj,
						struct IPT *ipt,
						struct ADB_HOOK *Hook,
						OBJS   *Objs);

SINT RSLoad(CHAR *File,
						struct OBJ **Robj,
						struct IPT **Ript,
						struct ADB_HOOK **RHook,
						OBJS   **RObjs);

void RSFree(SINT Hdl);
void RSSetPtr(struct OBJ *obj,CHAR *Nome,CHAR *ptr[]);
void RSSetSub(struct OBJ *obj,CHAR *Nome,
							void * (far *sub)(SINT cmd,LONG info,CHAR *str));
