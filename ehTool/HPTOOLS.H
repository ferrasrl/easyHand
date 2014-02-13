//   +-------------------------------------------+
//   | HPTOOLS Utilit… per la stampa su HP       |
//   | Header di intestazione                    |
//   |                                           |
//   |                                           |
//   |            by Ferr… Art & Technology 1998 |
//   |            Created by G.Tassistro         |
//   +-------------------------------------------+
#define HP_DOT    0
#define HP_GRID   1

SINT HPcontrol(void);

void HPEsc(CHAR *str);
void HPSetGrid(SINT x,SINT y);
void HPSetFine(SINT x,SINT y);

void HPCursor(SINT CoType,LONG x,LONG y);
void HPBox(SINT CoType,LONG x,LONG y,LONG x2,LONG y2,SINT sizeline);
void HPBoxp(SINT CoType,LONG x,LONG y,LONG x2,LONG y2);

SINT  HPdispf(SINT CoType,LONG x1,LONG y1,CHAR *font,SINT nfi,CHAR *str);
//int  HPBmpDisp(long x1,long y1,char *file);
SINT  HPBmpDisp(SINT CoType,LONG x1,LONG y1,CHAR *file);

SINT  HPDispType(SINT CoType,LONG x1,LONG y1,CHAR *font,SINT pitch,CHAR *str);
void HPMark(SINT CoType,LONG x1,LONG y1,SINT finex,SINT finey,SINT flag);
LONG HPgrid_y(SINT y);
LONG HPgrid_x(SINT x);
