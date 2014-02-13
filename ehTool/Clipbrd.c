//   +-------------------------------------------+
//   | CLIPBRD   Programma di accesso al         |
//   |           ClipBoard                       |
//   |                                           |
//   |                                           |
//   |            by Ferrà Art & Technology 1997 |
//   +-------------------------------------------+
/*
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <dir.h>
#include <alloc.h>
#include <flm_main.h>
#include <flm_vid.h>
*/

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/clipbrd.h"

#ifdef _WIN32 
SINT InWindows(void)
{
 return TRUE;
}

SINT CBopen()
{
return TRUE;
}
SINT CBclose(void)
{
return TRUE;
}
SINT CBclear(void)
{
return TRUE;
}

SINT CBset(CHAR *Byte,WORD len,SINT tipo)
{
return TRUE;
}

SINT CBget(CHAR *Byte,SINT tipo)
{
return TRUE;

}

LONG CBsize(SINT tipo)
{
return TRUE;

}



#else
SINT InWindows(void)
{
	static union REGS In,Out;

	In.h.ah = 0x17;
	In.h.al = 0x00;
	int86(0x2F,&In,&Out);

	if (Out.x.ax!=0x1700) return 0;
	return -1;
}

SINT CBopen(void)
{
	union REGS In,Out;

	In.h.ah = 0x17;
	In.h.al = 0x01;
	int86(0x2F,&In,&Out);

	if (Out.x.ax) return 0;
	return -1;
}

SINT CBclose(void)
{
	union REGS In,Out;

	In.h.ah = 0x17;
	In.h.al = 0x08;
	int86(0x2F,&In,&Out);

	if (Out.x.ax) return 0;
	return -1;
}

SINT CBclear(void)
{
	union REGS In,Out;

	In.h.ah = 0x17;
	In.h.al = 0x02;
	int86(0x2F,&In,&Out);

	if (Out.x.ax) return 0;
	return -1;
}

SINT CBset(CHAR *Byte,WORD len,SINT tipo)
{
	union REGS In,Out;
	struct SREGS Seg;
	LONG ptr;

	ptr=(LONG) Byte;

	In.h.ah = 0x17;
	In.h.al = 0x03;
	In.x.dx = tipo;
	In.x.si = 0;
	In.x.cx = len;
	Seg.es  = (WORD) (ptr>>16);
	In.x.bx = (WORD) (ptr&0xFFFF);

	int86x(0x2F,&In,&Out,&Seg);

	if (Out.x.ax) return 0;
	return -1;
}

SINT CBget(CHAR *Byte,SINT tipo)
{
	union REGS In,Out;
	struct SREGS Seg;
	LONG ptr;

	ptr=(LONG) Byte;

	In.h.ah = 0x17;
	In.h.al = 0x05;
	In.x.dx = tipo; // DIF
//	In.x.si = 0;
//	In.x.cx = len;
	Seg.es  = (WORD) (ptr>>16);
	In.x.bx = (WORD) (ptr&0xFFFF);

	int86x(0x2F,&In,&Out,&Seg);

	if (Out.x.ax) return 0;
	return -1;
}


LONG CBsize(SINT tipo)
{
	union REGS In,Out;

	In.h.ah = 0x17;
	In.h.al = 0x04;
	In.x.dx = tipo; // DIF

	int86(0x2F,&In,&Out);

	return (LONG) (Out.x.dx<<16)+Out.x.ax;
}

/*
void main(SINT argc,CHAR *argn[])
{
	CHAR serv[80];
	//static struct SREGS segment_regs;

	if (InWindows()) exit(1);
	printf("DOS Emulation di Windows\n");

 if (strcmp(argn[1],"R"))
	{
	printf("Apro il CLIPBOARD :");
	if (CBopen()) printf("Gi… aperto."); else printf("OK\n");
	getch();

	printf("Pulisco il CLIPBOARD :");
	if (CBclear()) printf("Fallito"); else printf("OK\n");
//	getch();

	printf("Setto il CLIPBOARD :");
	if (CBset("Giorgio",7)) printf("Fallito"); else printf("OK\n");
//	getch();


	printf("Chiudo il CLIPBOARD :");
	if (CBclose()) printf("Fallito."); else printf("OK\n");
	getch();
	}



	printf("Apro il CLIPBOARD :");
	rifopen:
	if (CBopen())
			{printf("LOCK:Gi… aperto.\n");
			 if (getch()==' ') goto rifopen;
			 exit(1);
			 }
	printf("OK\n");
	getch();

	printf("Leggo Size CLIPBOARD :");
	if (!CBsize()) printf("Nessun dato.\n");
								 else
								 {SINT size;
									CHAR *p;
									size=(SINT) CBsize();

									printf("N. byte : %d\n",size);
									p=malloc(size);

									//printf("Leggo il CLIPBOARD :");
									//memset(serv,0,30);
									if (CBget(p)) printf("Fallito"); else printf("Ho letto [%s]\n",p);
									free(p);
								 }


	printf("Chiudo il CLIPBOARD :");
	if (CBclose()) printf("Fallito."); else printf("OK\n");
	getch();

} */
#endif