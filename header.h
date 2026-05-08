// File header contenente le librerie, le strutture dati
// e le dichiarazioni delle funzioni necessarie al funzionamento
// del Trivia

#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <pthread.h>
#include <netinet/in.h> 
#include <string.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <ctype.h>

// definisco un massimo di utenti collegati contemporaneamente,
// nel caso in cui si voglia darne un limite massimo
#define MAXSOCKET 20
// definisco i temi e il numero di temi presenti nel quiz,
// nel caso si voglia aggiungerne un altro basta modificare le macro
#define TEMA1 "PALLAVOLO"
#define TEMA2 "SCIENZA"
#define TEMA3 "COMPLETA IL DETTO"
#define TEMA4 "CAPITALI"
#define NQUIZ 4
// definisco dei tipi di messggio (vedere gestione nel client)
#define QST     "QUESTION__"
#define NSW     "ANSWER____"
#define NNSW    "NO_ANSWER_"
#define CXT     "CLIENT_BYE"
#define SHW     "SHOW_SCORE"
#define CHN     "CHANGE_TOP"
// lunghezza del tipo standard per comodità
#define CMD_LEN 10
//massimo di domande presentabile al server
#define MAXQUESTION 5
// risposte di default
#define CORRETTO "Risposta corretta\n"
#define ERRATO "Risposta errata\n"
#define CMDERR "Comando non riconosciuto, digita nuovamente"
//redispongo il tipo ack per il protocollo di comunicazione,
// con relativa lunghezza
#define ACK_MSG "ACK"
#define ACK_LEN 3

//struttura dati contenente le informazioni essenziali
// sul quiz portato avanti da ogno giocatore,
// ogni giocatore ha la propria struttura dati quiz
typedef struct{
    int doing_quiz;
    int question_count;
    int score[NQUIZ];
    int terminated[NQUIZ];
} quiz;
//struttura dati che descrive ogni client
typedef struct playernode{
    //chiave primaria della mia sturttura,
    // ogni giocatore ha un solo descrittore di socket
    int sd;
    char* name;
    quiz* player_quiz;
    //implemento una lista di giocatori
    struct playernode* nextplayer;
} player;
//struttura che memorizza una domanda con relativa risposta
typedef struct {
    char domanda[256];
    char risposta[128];
} tranche;

//struttura dati di appoggio per ordinare la classifica
typedef struct {
    char* name;
    int score;
} rank_node;


//semaforo binario per l'accesso alle strutture dati condivise
pthread_mutex_t M;

//dichiarazioni delle funzioni

// Funzioni di gestione giocatori
void create_player(int socket, const char* name);
player* find_player(int socket);
void delete_player(int socket);
void rmv_thread(pthread_t target);

// Setup del server
int setup(int *listener);

// Connessione del client
int connect_socket(int porta);

// Interazione
int benvenuto(int s);
int menu(int s);
int registration(int s);
int choose_quiz(int s, char** temi);
void close_socket(int s);
void close_client(int sd);
void close_all();

// Gestione quiz
int load_question(tranche* questions, int r);
int send_question(int sd, tranche* t);
void end_quiz(int s);
int print_menu();

// Gestione dell'input
void trim_edges(char *str);
int send_answer(int sd);

// Classifica e punteggi
void print_rank();
void send_scores(int s);

// Protocollo di comunicazione
int my_send(int socket, const char* codice, const char* msg);
int my_recv(int socket, char** tipo, char** msg);