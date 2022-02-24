#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

/* 
NTHRS - Numarul de pacienti
DOCTS - nr total de doctori
INTERVAL - intervalul de spawn al pacientilor
LIMIT - cat poate dura maxim o consultatie. 


!OBSERVATIE :

Programul etse structurat a.i. sa foloseasca functii ce opresc executia anumitor threaduri 
pentru un timp real specificat.

Prin conventie, am ales 1 sec == 1 ora in simulare. 

La fel ca in realitate, medicii nu se pot ocupa intr-o singura zi de un numar prea mare de pacienti,
 mai ales daca timpul consultatiei reprezinta cateva ore.

In programul nostru un medic lucreaza de la ora 8 la ora 24 (sfarsitul zilei).
 Orele la care apar pacienti si durata unei consultatii sunt generate random, dar programul verifica
 daca pacientul se va incadra in timpul doctorului. In caz contrar, acesta va fi respins si se va afisa un mesaj
 Exemplu : verificati imaginea : 'prea_multi_pacienti.png'

Se recomanda folosirea unor valori optime pentru NTHRS, DOCTS, INTERVAL,
 pentru a testa codul intr-un timp real acceptabil si pentru a vedea daca toti pacientii se incadreaza intr-o zi.

*/

#define NTHRS 4
#define DOCTS 2
#define INTERVAL 4
#define LIMIT 6
sem_t sm; // declaram semaforul 1 sm.
pthread_mutex_t mtx; // declaram mutex1 global, vizibil tuturor thread-urilor
sem_t sm2; // declaram semaforul 2 sm.
pthread_mutex_t mtx2; // declaram mutex2 global, vizibil tuturor thread-urilor
int S=0; //pornim cu semaforul1 cu valoare 0 pentru a face thread-urile sa astepte in cazul in care doctorii sunt ocupati.
int S2=0;
int k=0; // contorizam numarul pacientilor care au intrat in cabinet.
int dc=-1; // dc si dct sunt folosite pentru a gasi doctorul valabil la un moment dat.
int dct=-1;
int rd=0; // timp random generat. 1 sec reala = 1 ora in simulare.
int check; // folosit la _timedwait pentru a verifica cand se scurge timpul de asteptare.
int ora=8; // ora de inceput.

// Structura doctor. Este explicata in fisierul README_explicatii.
struct doctor{
    int ocupat;
    int pacient;
    long int pthr;
    long int c_time;
    long int ora_start;
    long int ora_finish;
}doc[DOCTS];

struct timespec ts;
struct timeval tval;

void init_doctor()
{
    for(int i=0;i<DOCTS;++i)
    {
        doc[i].ocupat=0; 
        doc[i].pacient=-1;
        doc[i].pthr=0;
        doc[i].c_time=0; 
        doc[i].ora_start=0; 
        doc[i].ora_finish=0;
    }
}

int get_doc(int p, long int pt)
{
    for(int i=0;i<DOCTS;++i)
    {

        if(doc[i].ocupat == 0) // este liber
        {
            doc[i].ocupat=1;
            doc[i].pacient=p;
            doc[i].pthr=pt;
            return i;
        }
    }
    return -1;
}

int my_doc(long int p)
{
    for(int i=0;i<DOCTS;++i)
    {
        if(doc[i].pthr == p)
            return i;
    }
    return -1;
}


int cabinet_doctor()
{ 
    pthread_mutex_lock(&mtx2);
    k+=1;
    dc=my_doc(pthread_self());
    if(dc == -1)
    {
        dc = get_doc(k-1,pthread_self());
    }
    printf ("-Pacientul %d si-a inceput consultatia la ora %ld cu doctorul %d \n" ,doc[dc].pacient,doc[dc].ora_start, dc);
    doc[dc].c_time=(rand()%LIMIT)+1;
    if(doc[dc].ora_start + doc[dc].c_time > 24)
    {
        printf ("~!!~ Consultatia pacientului %d dureaza %ld ore si depaseste programul doctorului %d. Acesta pleaca acasa suparat >:( \n" ,doc[dc].pacient,doc[dc].c_time, dc);
        pthread_mutex_unlock(&mtx2);
        if(sem_post(&sm)) // lasam alt pacient sa intre la consultatie care poate se incadreaza in program.
        {
            perror("Eroare - POST\n");
            return errno;
        }
        return 0;
    }
    doc[dc].ora_finish = doc[dc].ora_start + doc[dc].c_time;
    doc[dc].ora_start = doc[dc].ora_finish;
    gettimeofday(&tval,NULL);
    ts.tv_sec = tval.tv_sec+doc[dc].c_time;
    ts.tv_nsec = 000000;
    if(k< NTHRS)
    {
        pthread_mutex_unlock(&mtx2);
        while ((check = sem_timedwait(&sm2, &ts)) == -1 && errno == EINTR)
            continue;
    }
    pthread_mutex_unlock(&mtx2);

    if(sem_post(&sm))
    {
        perror("Eroare - POST\n");
        return errno;
    }
    doc[my_doc(pthread_self())].ocupat=0;
    printf("-Pacientul %d a iesit de la doctor la ora %ld .El a stat %ld ore \n" , doc[my_doc(pthread_self())].pacient,doc[my_doc(pthread_self())].ora_finish,doc[my_doc(pthread_self())].c_time);
    return 0;
}
void* receptie(void* arg)
{
    int* tid = (int*)arg;
    pthread_mutex_lock(&mtx);
    dct=get_doc(*tid, pthread_self());
    if(dct!=-1)
    {
        //pc=*tid;
        // if(doc[my_doc(*tid)].ora_start == 0)
        //     doc[my_doc(*tid)].ora_start = ora;
        // pthread_mutex_unlock(&mtx);
        // barrier_point();
         if(doc[dct].ora_start == 0 || doc[dct].ora_start < ora)
         {
             if(ora > 24)
             {
                printf("~!!~ Pacientul %d a ajuns la cabinet la o ora prea tarzie. El va reveni maine \n", *tid);
                pthread_mutex_unlock(&mtx);
                return 0;
             }
            doc[dct].ora_start = ora;
         }
        pthread_mutex_unlock(&mtx);
        cabinet_doctor();

    }
    else
    {
        pthread_mutex_unlock(&mtx);
        if(sem_wait(&sm))
        {
            perror("Eroare - WAIT\n");
            return NULL;
        }
        cabinet_doctor();
    }
    free(tid); // eliberam memorie alocata in main.
    return NULL;
}


int main()
{
    pthread_t thr[NTHRS]; // N fire de executie
    printf("->Numarul de pacienti programati astazi este = %d\n", NTHRS);

    init_doctor();

    if(pthread_mutex_init(&mtx, NULL)) // se construieste Mutex-ul
    {
        printf("Eroare - MUTEX\n");
        return errno;
    }

     if(pthread_mutex_init(&mtx2, NULL)) // se construieste Mutex-ul
    {
        printf("Eroare - MUTEX\n");
        return errno;
    }

    if(sem_init(&sm,0,S)) 
    {
        perror("Eroare - SEMAFOR-INIT\n");
        return errno;
    }

    if(sem_init(&sm2,0,S2))
    {
        perror("Eroare - SEMAFOR-INIT\n");
        return errno;
    }

    srand(time(NULL));
    for(int i=0;i<NTHRS;++i)
    {
        int* num = malloc(sizeof(int));
        *num=i;
        ora+=rd;
        rd=(rand()%INTERVAL)+1;  // Fa-l pe rd global.
        printf(">>A aparut un pacient nou la ora %d, cu nr de ordine: %d\n", ora, i);
        if(pthread_create(&thr[i],NULL,receptie,num))  // creare threads.
        {
            perror("Eroare - T_CREATE\n");
            return errno;
        }
        if(i<NTHRS-1)
            sleep(rd); // nu mai asteptam si dupa ce s-a generat ultimul pacient.
    }

   for(int i=0;i<NTHRS;++i)
    {
        if(pthread_join(thr[i],NULL)) // asteptare dupa fiecare thread sa se termine.
        {
            perror("Eroare - T_JOIN\n");
            return errno;
        }
    }
    pthread_mutex_destroy(&mtx); // distrugem Mutex-ul.
    sem_destroy(&sm); // distrugem Semaforul.
    pthread_mutex_destroy(&mtx2); 
    sem_destroy(&sm2);

}
