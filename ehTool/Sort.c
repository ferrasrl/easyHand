//   +-----------------------------------------+
//	 | SORT - Ordina usando QSORT delle        |
//	 |        stringhe in ordine alfabetico    |
//	 |                                         |
//	 | base : Puntatore ai dati                |
//	 | num  : Numero dei dati									 |
//	 | dim  : Dimensione dei dati							 |
//	 |                                         |
//	 |                           by Ferr… 1994 |
//	 +-----------------------------------------+
/*
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mem.h>
#include <dir.h>
#include <flm_main.h>
*/
#include "\ehtool\include\ehsw_i.h"

void sort(void *base,SINT num,SINT dim)
{
	SINT cmps();
	qsort(base,num,dim,cmps);
}
cmps(void *p1,void *p2)
{
 return strcmp((CHAR *) p1,(CHAR *) p2);
}

