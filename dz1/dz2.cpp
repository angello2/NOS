#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <set>

#define MAXNAME 10

// strukture
struct podaci {
    int id;
    int log_sat;
    int ulasci;
};
struct cjevovod {
    char ime[MAXNAME];
    int fd;
    FILE* fh;
};
enum class Vrsta {Upit, Odg, Izlaz, Kraj};
struct poruka {
    Vrsta vrsta;
    int pid;
    int log_sat;
};

// metode potrebne da c++ set zna raditi sa strukturom poruke
bool operator<(poruka const& lhs, poruka const& rhs) {
    if (lhs.log_sat == rhs.log_sat) {
        return lhs.pid < rhs.pid;
    } else {
        return lhs.log_sat < rhs.log_sat;
    }
}
bool operator==(poruka const& lhs, poruka const& rhs) {
    return lhs.vrsta == rhs.vrsta && lhs.pid == rhs.pid && lhs.log_sat == rhs.log_sat;
}
bool operator!=(poruka const& lhs, poruka const& rhs) { 
    return !(lhs == rhs);
}

int shmid;
podaci *baza_podataka;

// pomocna funkcija za citanje
poruka read(cjevovod cjev) {
    char buf[sizeof(poruka)];
    read(cjev.fd, buf, sizeof(poruka));
    poruka por;
    memcpy(&por, buf, sizeof(poruka));
    return por;
}

// pomocna funkcija za pisanje
void write(cjevovod cjev, poruka por) {
    char buf[sizeof(poruka)];
    memcpy(buf, &por, sizeof(poruka));
    write(cjev.fd, buf, sizeof(poruka));
}

void print_red(std::set<poruka> red_poruka, int id) {
    printf("Proces %d ispisuje svoj red poruka: ", id);
    int i = 0;
    for(poruka por : red_poruka) {
        printf("%d. ", i);
        switch(por.vrsta) {
            case Vrsta::Upit:
                printf("Upit ");
                break;
            case Vrsta::Odg:
                printf("Odg ");
                break;
            case Vrsta::Izlaz:
                printf("Izlaz ");
                break;
        }
        printf(" Poslao: %d Log_sat: %d |", por.pid, por.log_sat);
        i++;
    }
    printf("\n");
}

// pomocna funkcija za ispis poruke
void print(poruka por) {
    if (por.vrsta == Vrsta::Upit) {
        printf("Vrsta: Upit ");
    }
    else if (por.vrsta == Vrsta::Odg) {
        printf("Vrsta: Odg ");
    }
    else if (por.vrsta == Vrsta::Izlaz) {
        printf("Vrsta: Izlaz ");
    }
    printf("Poslao: %d ", por.pid);
    printf("Log_sat: %d", por.log_sat);
}

// funkcija procesa
void proces(int id, int N, char imena_cjev[][MAXNAME]) {

    cjevovod cjevovodi[N];   
    srand(time(NULL));
    sleep(1);

    // otvaram cjevovode
    for(int i = 0; i < N; i++) {
        if(i == id) {   // otvaram vlastiti cjevovod za citanje
            int fd = open(imena_cjev[i], O_RDONLY);
            cjevovod moj;
            moj.fd = fd;
            strcpy(moj.ime, imena_cjev[i]);
            moj.fh = fdopen(fd, "r");
            cjevovodi[i] = moj;
        } else {    // otvaram ostale cjevovode za pisanje
            int fd = open(imena_cjev[i], O_WRONLY);
            cjevovod cjev;
            cjev.fd = fd;
            strcpy(cjev.ime, imena_cjev[i]);
            cjev.fh = fdopen(fd, "w");
            cjevovodi[i] = cjev;
        }
    }    

    int vrijeme = 0;

    // koristimo ordered queue iz c++
    std::set<poruka> red_poruka;
    int potrebno_odgovora = N - 1;

    for(int i = 0; i < 5; i++) {
        printf("Proces %d zeli uci u KO\n", id);
        vrijeme++;
        int temp_vrijeme = vrijeme;

        // stvaram upit
        poruka por;
        por.vrsta = Vrsta::Upit;
        por.pid = id;
        por.log_sat = temp_vrijeme;

        // stavljam svoj upit u red
        red_poruka.insert(por);

        // saljem upit ostalim procesima
        for(int j = 0; j < N; j++) {
            if(j != id) {
                printf("Proces %d poslao poruku procesu %d: ", id, j);
                print(por);
                printf("\n");
                write(cjevovodi[j], por);
            }
        }

        // cekam da mi svi ostali procesi odobre ulaz u KO i da moj upit bude prvi
        int odgovori = 0;
        while(odgovori < potrebno_odgovora || *red_poruka.begin() != por) {
            poruka por2 = read(cjevovodi[id]);
            printf("Proces %d primio poruku: ", id);
            print(por2);
            printf("\n");
            
            switch(por2.vrsta) {
                case Vrsta::Upit:
                {
                    if(vrijeme < por2.log_sat) vrijeme = por2.log_sat + 1;
                    else {
                        vrijeme = vrijeme + 1;
                    }
                    red_poruka.insert(por2);

                    poruka odg;
                    odg.pid = id;
                    odg.log_sat = vrijeme;
                    odg.vrsta = Vrsta::Odg;

                    write(cjevovodi[por2.pid], odg);
                    printf("Proces %d poslao poruku procesu %d: ", id, por2.pid);
                    print(odg);
                    printf("\n");
                    break;
                }
                case Vrsta::Odg:
                {
                    odgovori++;
                    break;
                }
                case Vrsta::Izlaz:
                {
                    // brisem iz reda upit procesa koji mi je poslao izlaz
                    por2.vrsta = Vrsta::Upit;
                    red_poruka.erase(por2);
                    break;
                }
                case Vrsta::Kraj:
                {
                    /* 
                    kada neki proces zavrsi s radom (obavio 5 ulazaka u KO)
                    smanjujemo broj potrebnih odgovora kako bi i ostali procesi
                    mogli obaviti sve do kraja nakon sto prvi zavrsi
                    */
                    potrebno_odgovora--;
                    break;
                }
            }
        }
        print_red(red_poruka, id);

        printf("Proces %d ulazi u KO\n", id);
        baza_podataka[id].log_sat = vrijeme;
        baza_podataka[id].ulasci++;

        // print stanja baze podataka
        printf("Proces %d ispisuje bazu:\n", id);
        for(int i = 0; i < N; i++) {
            printf("podaci[%d]: id=%d, log_sat=%d, ulasci=%d\n", i, baza_podataka[i].id, baza_podataka[i].log_sat, baza_podataka[i].ulasci);
        }

        int sleep_time = rand() / RAND_MAX * 1900 + 100;
        printf("Proces %d spava na %d ms\n", id, sleep_time);
        usleep(sleep_time * 1000);

        // brišem svoj upit s pocetka reda, saljem drugima da izlazim
        red_poruka.erase(por);
        por.pid = id;
        por.log_sat = temp_vrijeme;
        por.vrsta = Vrsta::Izlaz;

        for(int j = 0; j < N; j++) {
            if (j != id) {
                write(cjevovodi[j], por);
                printf("Proces %d poslao poruku procesu %d: ", id, j);
                print(por);
                printf("\n");
            }
        }

        // vracam por na Upit zbog uvjeta petlje
        por.vrsta = Vrsta::Upit;
    }

    // saljem svima poruku da zavrsavam s radom
    poruka por;
    por.vrsta = Vrsta::Kraj;
    por.log_sat = 0;
    por.pid = id;
    for(int i = 0; i < N; i++) {
        if(i != id) 
        {
            write(cjevovodi[i], por);
        }
    }

    printf("Proces %d zavrsava s radom\n", id);
    sleep(1);
}

int main(void)
{
    int N;
	printf("Unesite N (broj procesa, od 3 do 10): ");
    scanf("%d", &N);
    while (N < 3 || N > 10) {
        printf("N mora biti između 3 i 10!\n");
        printf("Unesite N ponovo: ");
        scanf("%d", &N);
    }
    printf("Pokrećem %d procesa i %d cjevovoda\n", N, N);

    // stvaram dijeljenu memoriju
    shmid = shmget(IPC_PRIVATE, sizeof(podaci) * N, 0600);

    // stvaram bazu podataka
    baza_podataka = (podaci *) shmat(shmid, NULL, 0);

    // punim bazu pocetnim podacima
    for (int i = 0; i < N; i++) {
        baza_podataka[i].id = i;
        baza_podataka[i].log_sat = 0;
        baza_podataka[i].ulasci = 0;
    }

    // stvaranje N cjevovoda
    char imena_cjev[N][MAXNAME];
    for (int i = 0; i < N; i++) {
        char ime[MAXNAME];
        sprintf(ime, "cjev%d", i);
        mknod(ime, S_IFIFO | 00600, 0);
        strcpy(imena_cjev[i], ime);
    }
    bool roditelj = true;
    // stvaranje N procesa
    for (int i = 0; i < N; i++) {
        if(fork() == 0) {
            roditelj = false;
            proces(i, N, imena_cjev);
            break;
        }
    }
    if (roditelj) {
        for(int i = 0; i < N; i++){
            wait(NULL);
        }
        shmdt((podaci *) baza_podataka);
        shmctl(shmid, IPC_RMID, NULL);
    }
}