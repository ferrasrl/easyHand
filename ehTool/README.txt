EhTools release 27/6/2000
Ferrà Art & Technology (c) 1993-2000

Adbsfilt	Filtro di selezione per dbase
Adbsfilt32	Filtro di selezione per dbase multi-thread 32bit
Adbsfind	Utility per ricerche in dbase
ARMaker		Costruttore di Array
Clipbrd		Gestione del clipboard (DOS-TASK)
Datautil	


Ferrà srl 31/02/2007 - di Giorgio Tassistro
Con il rilascio della versione 8.0 del Visual Studio, la Microsoft ha variato diverse cose, principalmente orientate alla sicurezza 
e alla protezione dei programmi.

Sedetevi stabilmente.
Istruzioni come ad esempio strcpy() e strstr() non sono state dichiarate "deprecated" 
Operazione come scrivere in una stringa (vedi esempio sotto) non sono più possibili.

	CHAR *lpString="Esempio";
	lpString[0]='A'; // <-- Genera un errore di violazione di memoria 

Inoltre, probabilmente, bisognerà allegare dei certificati ai programmi per permettere di farli girare
senza troppi blocchi su Vista e Longhorn in arrivo.
Per questi motivi e per permettere di compilare con il nuovo compilatore Easyhand ne sono state riscritte 
alcune parti e si è deciso di organizzare in modo leggermente differente le librerie.

Installazione
Fate una copia di EhTool e usate quella nuova senza sovrascrivere la precedente ed utilizzate quella nuova.

Cosa fare con i vecchi sorgenti
Ho notato che praticamente diversi sorgenti in EhTool sono usati praticamente sempre
quindi ho deciso di inserili direttamente in FLIB.
Ho tenuto comunque una copia dei sorgenti per permettere di modificarli "in fretta" ed usarli nella versione console
di EH.
Questo insiemi di sorgenti (con i .h) sono stati spostati in una cartella chiamata \ehtool\main:
Datautil.c	 - Gestione Tempo 
DmiUtil.c	 - Gestione DMI array
UltManager.c - Gestore Lingua
ArMaker.c	 - Gestiore Array (con nuove funzioni)
StrEncode.c  - Gestiore Encoding di stringhe : URL/HTML/UTF-8 sia char e widechar (UNICODE)

ATTENZIONE: questo sorgenti sono "gia presenti" in Flib quindi non è strettamente necessario, a meno di update, 
aggiungerli nel progetto.

Gli "include" di questo sorgenti sono già presenti in \ehtool\include\ehsw_...

Cose da fare
a) andranno rimossi da tutti i sorgenti i relativi ".h" dei sorgenti indicati sopra.
b) andranno rimossi (e riaggiunti se lo desiderate) dai progetti i file sorgenti indicati


Visual Studio 2005 (v.8.0): leggere attentamente, è costato ore di ricerca.

La versione Express del visual studio è scaricabile gratuitamente dal sito Microsoft: unico lato positivo.
Attenzione: è necessario anche scaricare SDK platform aggiuntivo per il processore che si usa!
In pratica hanno scollegato il compilatore dalla piattaforma hardware, quindi ci sono "pack" per ogni tipo di 
processore e poi negli include/lib bisogna indicare "a mano" cosa si usa.... (no comment)

http://msdn.microsoft.com/vstudio/express/visualc/usingpsdk/


Include files
$(ProgramFiles)\Microsoft Platform SDK\Include
$(ProgramFiles)\Microsoft Platform SDK\Include\MFC

Library files
$(ProgramFiles)\Microsoft Platform SDK\lib


Per quanto riguarda l'uso delle stringhe si può ovviare al problema forzando ad usare 
il vecchio e standard strcpy ed altro attraverso l'inserimento di una macro nel 
Preprocessor definition: _CRT_SECURE_NO_DEPRECATE

La "microsoft's genialate" consiste nell'aver riscritto "tutte le funzioni che usano le stringhe" 
aggiungendo un campo nelle funzioni con il numero di caratteri del buffer di destinazione, 
cosi l'istruzione verifica che non si vada in overload.

Es. strcpy(char * ,char *) diventa strcpy(char *, int, char*) etc..
Tutto questo ben di Dio è chiamato "sistema di protezione .Net" ;-)

Per quanto riguarda il secondo problema segnalato, al momento non ho trovato soluzione, tranne quella di modificare
tutti i punti dove Easyhand ne faceva uso (Es. i menu).
Nota: Il problema esiste solo sulle stringhe dichiarate in questo modo e non con altro.


Aprire in C:\Programmi\Microsoft Visual Studio 8\VC\VCProjectDefaults\corewin_express.vsprops
ed aggiungere dove c'è scritto Kernel32.lib
user32.lib gdi32.lib shell32.lib comdlg32.lib ole32.lib winspool.lib advapi32.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib


quindi VisualStudio+Platform SDK+correzioni sorgenti+Inserimenti a mano di folder da usare+inserimenti di librerie+MACRO
= Tutto dovrebbe funzionare

Ciliegia: La versione gratuita non ha il gestore di risorse, upgrade di Visual studio costa cira 800 euro.

Viene voglia di programmare su Linux o Mac.


--------------------------------------------------
Ferrà srl 20/11/2007 - di Giorgio Tassistro
EasyHand 5.0
Premessa
Quella che segue è una modifica Epocale, ma ne vale la pena.
Preparatevi a perdere qualche oretta di ricodifica.

Fase 1
Con l'avanzare dei programmi operanti in multi-thread, si è resa necessaria (purtroppo) una variazione "drastica" delle funzioni di stampa scritte e 
del gestore degli oggetti.
Sono stati tolti i flag globali FondBold,FontItalic, FontUnderline s sostituiti con STYLE_BOLD,STYLE_ITALIC e STYLE_UNDERLINE.
a tutte le funzioni di stampa è stato tolto il parametro lock e mode, sostuito dal parametro Style che puo contenere le tre macro separate da pipe + 
le macro SET/XOR/AND/OR
la dispm è stata tolta, la dispfm sarà tolta presto essendo diventata uguale alla dispf perchè alla dispf è stato aggiungo il parametro style.
In pratica si è fuso il parametro di raster (SET/XOR) on i parametri di styles nuovi.
Inoltre è stata riscritta la gestione dei font: ora l'handle del font contiene tutte le caratteristiche Font Face, Altezza e stili.
Quindi tutte le funzioni che richiedevano l'handle del font non contegono più i parametri di stile e nfi (altezza o numero del font interno).
Tutte le strutture degli oggetti/input/objs sono state modificate di conseguenza, faceno sparire il parametro nfi.
Es. ->hdlFont diventa ->idxFont
FONT_fix non esiste più.


Fase 2
Si è tolta la OBJCallSub e le funzioni che la gestivano come Obj_setCall();
La struttura conteneva il puntatore all'oggetto per le chiamate alle funzioni esterne collegate agli oggetti.
In sostituzione si è inserito come primo parametro funzioni esterne, questo per renderle indipendenti e compatibili al multithread.
Questa variazione aggiunge molta stabilità al sistema e toglie l'ambiguita (CERTA in caso di Multithread) in chiamata della funzione.
Quindi la funzione esterna è diventata come prototipo

	void * (*funcExtern)(struct OBJ *, SINT cmd, LONG info,CHAR *str); //  funzione esterna di controllo

Esisteva già un problema da tempo, perchè il puntatore all'oggetto veniva già passato alla funzione, un po in info e un po in ptr a seconda dei casi e
usato poi con un casting, per risalire all'oggetto chiamante (in caso la funzione fornisse servizi a più oggetti).
Adessi non viene più passato (per risolvere l'ambiguita) in questi due parametri, ma sempre come primo parametro.
Di conseguenza sono da cambiare tutte le funzione di controllo
Il compilatore segnala tutte le funzione con il prototype sbagliato, si consiglia:
a) cancellare la variabile che si occupava di cercare e contenere il puntatore all'oggetto all'interno della funzione (tipicamente *obj)
b) Rinominare tutte le chiamate obj-> con objCalled-> 
c) inserire struct *OBJ objCalled all'inizio della funzione
d) cancellare tutti punti dove si assegnava obj=


Ne consegue che sono cambiati tutti i "driver" esterni di default:
void *ScrDrvTest(struct OBJ *objCalled,SINT cmd,LONG info,CHAR *str);
void * EhWebPage(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr);
void * EhProgressBar(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr)
void * EhTreeView(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr)
void * EhTrackBar(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr)
void *SdbDriver(struct OBJ * objCalled,SINT cmd,LONG info,CHAR *str);
void * EhTextEditor(struct OBJ *poj,SINT cmd,LONG info,void *ptr);
void * EhListView(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr)
void * EhListViewSort(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr);
void * EhEditText(struct OBJ *objCalled,SINT cmd,LONG info,void *ptr);


e le chiamate di inizializzazione, come ad esempio
	EhTreeView(WS_START,0,NULL);
diventano
	EhTreeView(NULL,WS_START,0,NULL);
Regola generale: nel dubbio inserire NULL come primo parametro.

Fase 3
Look 98, è stato dichiarato obsoleto e tolto dal EhTool.
Una nuova forma di Look che sfrutta GDI+ è stata integrata all'interno dei EasyHand5.
La chiamata alla funzione Look98, il file ed gli header vanno cancellati dai sorgenti

Ulteriori novità
Il parametro lock degli oggetti ed input è stato cambiato nel più corretto bEnable, visto che lavorava al contrario (lock=ON voleva dire oggetto abiliato!)
E' stato aggiunto il parametro bVisible che si usa con obj_visible(nome,flag) per rendere visibile o nascondere un oggetto.




