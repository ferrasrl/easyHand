//   +-------------------------------------------+
//   | EURO     Utility per l'euro               |
//   |                                           |
//   |             by Ferr… Art & Technology 1999 |
//   +-------------------------------------------+

#include "/easyhand/inc/easyhand.h"
#include "/easyhand/ehtool/euro.h"

// Lira contro Euro
double LireToEuro(double Lire) {return numRound((Lire/EUROLIRE_FIX)+0.005F,2,false);}

double EuroToLire(double Euro)
{
	double Lire;
	Lire=numRound(Euro,2,false)*EUROLIRE_FIX;
	Lire=numRound(Lire+.5F,0,false);
	return Lire;
}

// Franco contro Euro
double FFToEuro(double FF) {return numRound((FF/EUROFF_FIX)+0.005F,2,false);}

double EuroToFF(double Euro)
{
 double FF;
 FF=numRound(Euro,2,false)*EUROFF_FIX;
 FF=numRound(FF+.005F,2,false);
 return FF;
}
