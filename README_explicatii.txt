OBSERVATII:
Proiectul nu este 100% finisat, existand posibile cazuri netratate.
Cu toate astea, este intr-un stadiu inaintat asa ca mi-am dorit sa primesc un feedback de la dumneavoastra pentru a-mi da seama daca modul de abordare si codul scris
pana acum este OK. 


Programul simuleaza activitatea dintr-un cabinet medical.

Mai multi pacienti apar la ore diferite si in functie de disponibilitatea numarului de doctori incep o consultatie ce dureaza un anumit timp.
Exista si cazuri in care doctorii sunt coplesiti de numarul prea mare de pacienti sau de durata prea lunga a consultatiilor, nereusind sa ii incadreze pe toti
pana la ora 24 ( 00:00) astfel uni pacienti pot pleca acasa suparati.


Programul se foloseste de functii precum sleep(); sem_wait() si sem_timedwait(); pentru a face anumite threaduri sa isis suspende executie pe o perioada 
data de timp.

Pentru ca programul sa nu ruleze pe o perioada mare te timp ( 10 minute / 1 ora in timp real) s-a stabilit o conventie prin care:
-> 1 secunda in timp real = 1 ora in simularea consultatiilor.


EXPLICARE COD:

Codul foloseste 2 mutex-uri corespunzatoare unor zone de memmorie critice, 2 semafoare si o structura de date reprezentand doctorii.

>>> Structura doctor:
- doc.ocupat = intreg ce ia valorile 0 sau 1 ( 0 = doctorul este liber; 1 = doctorul este ocupat)
- doc.pacient = aici se retine id-ul pacientului ce va ocupa sala doctorului ( nr de forma 0, 1, .. 10 etc)
- doc.pthr = aici se retine ID-ul threadului asociat pacientului memorat in doc.pacient ( numar lung , de forma 192912123)
- doc.c_time = variabila ce retine un nr generat automat reprezentand nr de ore ocupat de consultatia pacientului
- doc.ora_start = retine ora la care pacientul intra la doctor ( ex: ora 9, ora 10, ora 8)
- doc.ora_finis = retine ora la care pacientul iese de la doctor ( ex: ora 9, ora 10, ora 8)

Aceasta structura este initializata la executia codului cu niste valori nule / negative prin apelarea functiei init_doctor();



>>>> Metoda get_doc(id pacient, id thread pacient):
- aceasta functie cauta primul doctor disponibil, il marcheaza ca fiind ocupat, ii stabileste id-ul pacientului ce il va consulta si thread-ul asociat pacientului si
intoarce nr de ordine al doctorului.



>>>> Metoda my_doc(id thread pacient)
- aceasta functie este asemanatoare cu get_doc(), dar ea doar intoarce nr de ordine al doctorului ce se va ocupa de pacientul cu id-ul thread-ului specificat.


>>> START PROGRAM:

-Programul isi initializeaza semafoarele, mutexul si threadurile.
-Se va genera cate un fir de executie pe ran, la o perioada de timp random.
-Firul creat isi incepe executie in functie  void receptie()
- In receptie(), pacientul isi cauta si marcheaza un doctor disponibil prin intermediul functiei get_doc();
Daca primul pacient intra in receptie, acesta va gasi sigur un doctor, il va marca si va devenii ocupat, 
dupa care isi va seta ora de start iar thread-ul ce reprezinta pacientul va trece spre functia  int cabinet_doctor();

Ora de start fie este ora declarata global, fie va fii calculata in functie de ora la care doctorul a terminat consultatia anterioara si ora la care 
pacientul a intrat in receptie.

Urmatorul pacient ce va intra in receptie(), va incerca sa gaseasca un doctor liber prin get_doc().
Acum exista posibilitatea ca toti doctorii sa fie ocupati, moment in care thread-ul pacient va astepta la semafor pana primeste un semnal din cabinet_doctr()
care sa ii indice ca s-a eliberat un loc.

-In cabinet_doctor(), fiecare pacient va fi contorizat pentru a determina daca toti pacientii au fost consultati sau nu.
- Pacientul isi obtine id-ul doctorului prin functia my_doc() si o perioada de timp reprezentand durata consultatiei.
Exista posibilitatea ca un pacient sa nu isi stie id-ul doctorului atunci cand intra in cabinet ( de regula , pacientii care au asteptat la semafor in receptie()), astfel
imediat ce un pacient ce a asteptat in receptie() va intra in cabinet isi va marca doctorul ca fiind ocupat prin functia get_doc();
- Thread-ul pacientul isi va calcula ora de start, ora de iesire si durata consultatie si va astepta la un semafor scurgerea timpului.
Exista posibilitatea ca durata consultatie sa depaseasca programul de lucru al doctorului, moment in care consultatie este intrerupta si pacientul pleaca acasa suparat.
- Dupa ce timpul consultatie se scurge, thread-ul pacient trimite un semnal spre sala de receptie() indicand celor care asteapta acolo ca s-a eliberat un doctor.

- La final, toate thread-urile se termina iar in consola vor fii afisati timpii scosi de fiecare pacient de cand a aparut si pana cand a iesit de la doctor.



