//   +-------------------------------------------+
//   | SLKdrive Esporta in formato SLK           |
//   | Header                                    |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1997 |
//   +-------------------------------------------+


 struct INFOSLK {
	 SINT LastX;
	 SINT LastY;
	 CHAR Stile[30];
	 FILE *pf;
	 DRVMEMOINFO SLKinfo;
	 };

 LONG SLKdrive(SINT cmd,CHAR *Location,SINT tipo,void *ptr);
#define SLKNULL 0
