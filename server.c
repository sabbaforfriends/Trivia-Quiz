// file c del server

// includo il file header, le funzioni di utilità per il client
// e il file dove viene implementato il protocollo di comunicazione
#include "header.h"
#include "s_utilityfunctions.c"
#include "both_utilityfunctions.c"

// la radice della lista deve essere globale, tutti i threads
// devono avere la stessa
player* root;
// current_players mantiene il numero di giocatori attualmente connessi
int current_players;
// current_threads mantiene il numero di threads che attualmente gestiscono un client
int current_threads;
// array nel quale salvo tutti i descrittori di thread
pthread_t pid[MAXSOCKET];
// array nel quale memorizo tutti i nuovi socket creati
int* nuovi_socket[MAXSOCKET];
//array contenente i temi, utile per la stampa
char *temi[NQUIZ] = {TEMA1, TEMA2, TEMA3, TEMA4};
// queste le metto globali per comodità nel file s_utilityfunctions
struct sockaddr_in client_addr;
int port=4242;
int len, new_sd;
int t_index;

// routine dei threads che gestiscono i client
void* thread_routine(void* new_socket){
    int ret;
    pthread_mutex_lock(&M);
    current_threads++;
    pthread_mutex_unlock(&M);
    int id = *(int*)new_socket;
    pthread_t spid=pthread_self();
    // do la possibilità a input_routine di terminare questo thread in maniera sicura
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    // array di domande e risposte (header.h)
    tranche questions[MAXQUESTION];
    // informo che stiamo entrando in una nuova sezione
    my_send(id, CHN, "NUOVA SEZIONE");
    // registro il client nella mia lista di giocatori
    ret=registration(id);
    if(ret<0){
        // se qualcosa è andato storto nella registrazione,
        // chiudo il socket e termino il thread
        rmv_thread(spid);
        // qua non chiamo la close_socket() perché il giocatore non è stato ancora registrato!
        close(id);
        return NULL;
    }
    // stampo le informazioni sui giocatori
    print_rank();
    while(1){
        
        // informo che stiamo entrando in una nuova sezione
        my_send(id, CHN, "NUOVA SEZIONE");

        // chiedo al client di scegliere il tema del trivia
        ret=choose_quiz(id, temi);

        if(ret<0){
            // se qualcosa è andato storto nella scelta,
            // chiudo il socket e termino il thread
            rmv_thread(spid);
            close_socket(id);
            break;
        }

        // carico le domande del tema scelto dal client, difficilmente ci sono problemi,
        // solo se non viene chiamato bene il file di testo corrispondente
        load_question(questions, ret);
        
        // informo che stiamo entrando in una nuova sezione
        my_send(id, CHN, "NUOVA SEZIONE");
        
        // invio le domande ed attendo le risposte
        ret=send_question(id, questions);
        if(ret<0){
            // se qualcosa è andato storto nel quiz,
            // chiudo il socket e termino il thread
            rmv_thread(spid);
            close_socket(id);
            break;
        }
    }
    return NULL;
}

// routine del thread che campiona l'input del server e lo gestisce
void* input_routine(){
    char input[1024];
    while(1){
        fgets(input, sizeof(input), stdin);
        // gestisco eventuali errori di battitura (cr e spazi)
        if(!strcmp(input, "\n")){
            continue;
        }
        trim_edges(input);
        // se l'input è "exit" lo gestisco
        if(!strncmp("exit", input, 4)){
            printf("Chiusura del server..\n");
            close_all();
            exit(0);
        }
        else{
            printf("Comando non riconosciuto, immettere exit per terminare il server\n");
        }
    }
    return NULL;
}


int main(){
    int ret;
    // inizializzo root a null, la lista dei giocatori all'avvio è vuota
    root=NULL;
    // inizializzo il semaforo binario
    pthread_mutex_init(&M, NULL);
    // creo un thread atto a gestire gli imput nel server
    pthread_t p_input;
    // descrittore del socket listener
    int listener;
    pthread_create(&p_input, NULL, input_routine, NULL);
    // collego il socket di ascolto
    if(setup(&listener)==-1){
        perror("errore nel collegamento del listener\n");
    }

    // stampo le informazioni sui giocatori (nessun giocatore)
    print_rank();

    len=sizeof(client_addr);
    t_index=0;
    current_players=0;
    current_threads=0;
    // la condizione nel while da la possibilità ad un limite massimo di socket istanziabili (DOS)
    while(current_players<=MAXSOCKET){
        //per ogni nuova richiesta di connessione, creo un thread che gestisca il client
        nuovi_socket[t_index]=(int*)malloc(sizeof(int));
        // istanzio socket_temp perché, se mettessi direttamente l'output in nuovi_socket,
        // nel caso in cui con endquiz dovessi modificare
        // gli array, decrementando t_index, se il thread di ascolto fosse bloccato sulla accept darebbe
        // il socket descriptor nell'indice non decrementato
        int socket_temp;
        socket_temp=accept(listener, (struct sockaddr*)&client_addr, (void*)&len);
        *nuovi_socket[t_index]=socket_temp;
        ret=pthread_create(&pid[t_index], NULL, thread_routine, nuovi_socket[t_index]);
        if(ret){
            printf("Errore nella creazione del thread\n");
            continue;
        }
        pthread_mutex_lock(&M);
        t_index++;
        pthread_mutex_unlock(&M);
        
    }
    exit(0);
}