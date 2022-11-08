/*
** trgovac.cpp
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>

/*
** STRUKTURE 
*/

struct sastojci {
    bool papir;
    bool duhan;
    bool filter;
};

struct my_msgbuf {
    long mtype;
    char mtext[512];
};

int msqid;

void izlaz(int failure){
    msgctl(msqid, IPC_RMID, NULL);
    exit(0);
}

/*
** PUSAC 
*/
int pusac(int id) {
    sastojci imam;
    if (id == 1) {
        imam.duhan = true;
        imam.papir = false;
        imam.filter = false;
    } else if (id == 2) {
        imam.duhan = false;
        imam.papir = true;
        imam.filter = false;
    } else {
        imam.duhan = false;
        imam.papir = false;
        imam.filter = true;
    }
    if (imam.papir) printf("Ja sam pusac %d i imam papir\n", id);
    else if (imam.duhan) printf("Ja sam pusac %d i imam duhan\n", id);
    else printf("Ja sam pusac %d i imam filter\n", id);
    fflush(stdout);

    // spajanje na red poruka
    key_t key = 12345;
    msqid = msgget(key, 0600 | IPC_CREAT);

    my_msgbuf buf;
    int expected_type = id + 1;

    // loop - cekam poruku trgovca da je stavio sastojke koje trebam 
    while(true) {

        msgrcv(msqid, (struct my_msgbuf *)&buf, sizeof(buf), expected_type, 0);
        printf("Pusac %d: Primio poruku: %s\n", id, &buf.mtext);
        fflush(stdout);

        // trazim dozvolu da uzmem sastojke
        char text[] = "Sastojci mi odgovaraju, smijem li uci u KO?";
        memcpy(buf.mtext, text, strlen(text) + 1);
        buf.mtype = 5;
        msgsnd(msqid, (struct my_msgbuf *) &buf, strlen(text) + 1, 0);
            
        // uzimam sastojke, ulazim u KO
        msgrcv(msqid, (struct my_msgbuf *) &buf, sizeof(buf), 6, 0);
        printf("Pusac %d: Primio poruku: %s\n", id, &buf.mtext);
        fflush(stdout);

        // motam cigaretu
        printf("Pusac %d: Motam cigaretu\n", id);
        fflush(stdout);

        sleep(2);

        // izlazim iz KO
        printf("Pusac %d: Izlazim iz KO\n", id);
        fflush(stdout);

        strcpy(text, "Gotov sam s motanjem");
        memcpy(buf.mtext, text, strlen(text) + 1);
        buf.mtype = 7;
        msgsnd(msqid, (struct my_msgbuf *) &buf, strlen(text) + 1, 0);

        printf("Pusac %d: Pusim cigaretu...\n", id);
        fflush(stdout);

        sleep(5);

        printf("Pusac %d: Popusio cigaretu.\n", id);
        fflush(stdout);
    }

    return 0;
}

/*
** TRGOVAC 
*/
int trgovac() {
    sleep(1);
    printf("Ja sam trgovac\n");
    fflush(stdout);

    // stvaranje reda poruka
    key_t key = 12345;
    msqid = msgget(key, 0600 | IPC_CREAT);

    my_msgbuf buf;

    // loop - bira nasumice koje ce sastojke staviti
    // razliciti sastojci => razliciti mtype poruke    
    // 2 - stavljam papir i filter
    // 3 - stavljam duhan i filter
    // 4 - stavljam papir i duhan
    // 5 - trazim ulazak u KO
    // 6 - dopustam ulazak u KO
    // 7 - zavrsavam sa KO
    srand(time(NULL));
    while(true) {
        int num = rand() % 3 + 2;
        char text[512];
        
        // saljem poruku razlicitog tipa ovisno o sastojcima
        switch(num) {
            case 2:
                strcpy(text, "Trgovac stavlja papir i filter na stol");
                break;
            case 3:
                strcpy(text, "Trgovac stavlja duhan i filter na stol");
                break;
            case 4:
                strcpy(text, "Trgovac stavlja papir i duhan na stol");
                break;
        }
        printf("%s\n", text);
        fflush(stdout);
        memcpy(buf.mtext, text, strlen(text) + 1);
        buf.mtype = num;
        msgsnd(msqid, (struct my_msgbuf *) &buf, strlen(text) + 1, 0);
        
        // cekam da neki od pusaca zatrazi ulazak u KO
        msgrcv(msqid, (struct my_msgbuf *) &buf, sizeof(buf), 5, 0);
        printf("Trgovac: Primio poruku: %s\n", &buf.mtext);
        fflush(stdout);

        // saljem dozvolu za ulazak u KO
        strcpy(text, "Udi u KO");
        memcpy(buf.mtext, text, strlen(text) + 1);
        buf.mtype = 6;
        msgsnd(msqid, (struct my_msgbuf *) &buf, strlen(text) + 1, 0);
        // cekam da pusac javi da je zavrsio s KO
        msgrcv(msqid, (struct my_msgbuf *)&buf, sizeof(buf), 7, 0);
        printf("Trgovac: Primio poruku: %s\n", &buf.mtext);
        fflush(stdout);
        
        sleep(3);
    }

    return 0;
}

/*
** MAIN
*/
int main(void) 
{
    sigset(SIGINT, izlaz);

    // 3 forka - 1 trgovac, 3 pusaca, svaki dobiva svoj id
    int pid, pid1, pid2, pid3;
    pid = fork();

    if (pid == 0) {
        pusac(1);
    }
    else {
        pid1 = fork();
        if (pid1 == 0){
            pusac(2);
        }
        else {
            pid2 = fork();
            if(pid2 == 0) {
                pusac(3);
            }
            else {
                trgovac();
            }
        }
    }

}