//   +-------------------------------------------+
//   | WMENU.H  Utilit… di richiesta tipo W95    |
//   |          propriet… oggetto                |
//   |                                           |
//   |             by Ferr… Art & Tecnology 1996 |
//   +-------------------------------------------+

#ifdef EH_WIN_GDI

 struct WMENU {
			SINT Flag;
			CHAR cmd[3];
			CHAR Desc[30];
			};

//int WMenu(char *Titolo,struct WMENU Menu[]);
SINT WMenu(CHAR *Titolo,CHAR *SubTit,struct WMENU Menu[]);

#endif