SINT  SKLocate(CHAR *Label,SINT *lpt);
CHAR *SKModel(CHAR *Label);
CHAR *SKBlockRead(CHAR *Label,CHAR *Password,SINT Start,SINT Size);
SINT  SKProgram(CHAR *OldLabel,CHAR *NewLabel,CHAR *NewPassword);
SINT  SKWrite(CHAR *Label,CHAR *Password,CHAR *Data);
SINT  SKCompare(CHAR *Label,CHAR *Password,CHAR *Data);
