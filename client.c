#include "header.h"
#include "c_utilityfunctions.c"
#include "both_utilityfunctions.c"

int sd;

// Handler che gestisca SIGINT
void handle_sigint() {
    int ret;
    printf("\nChiusura client...\n");
    sleep(2);
    // Invio un messaggio di disconnessione al server
    ret=my_send(sd, CXT, "disconnecting");
    if(ret<0){
        printf("Server disconnesso, quiz terminato\n");
        sleep(3);
        close_client(sd);
    }

    // Chiudo il socket
    close_client(sd);

    exit(0);
}


int main(int argc, char *argv[]) {
    // Impongo un handler per chiusura anomala dovuta a SIGINT,
    // mentre ignorerò l'interruzione sigpipe per gestirla a modo mio
    signal(SIGINT, handle_sigint);
    signal(SIGPIPE, SIG_IGN);
    if (argc != 2) {
        fprintf(stderr, "Immettere: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int porta = atoi(argv[1]);    
    int ret;
    char* r_buffer=NULL;
    char* t_buffer=NULL;
    while(1){
        // stampo il menù
        ret=print_menu();
        if(ret==2){
            system("clear");
            exit(0);
        }
        // connetto il socket
        sd=connect_socket(porta);
        if(sd==-1){
            //se c'è stato un errore in fase di connessione, semplicemente riparto dal while
            continue;
        }
        // qui sta tutta la logica del client, ricevo un messaggio dal server e in base al tipo, decido cosa fare
        while(1){
            if(my_recv(sd, &t_buffer, &r_buffer)==-1){
                // se la my_recv non va a buon fine, vuol dire che il server è stato disconnesso
                printf("Server disconnesso, quiz terminato\n");
                sleep(3);
                close_client(sd);
                break;
            }
            else if(!strncmp(t_buffer, CXT, CMD_LEN)){
                // se il messaggio è di tipo cxt, vuol dre che il server è stato notificato della chiusura del client e mi da il via libera a chiudere
                printf("%s\n", r_buffer);
                sleep(3);
                close_client(sd);
                break;
            }
            else if(!strncmp(t_buffer, CHN, CMD_LEN)){
                // se il messaggio è di tipo chn, vuol dire che sto cambiando sezione, faccio un clear
                system("clear");
                continue;
            }
            else if(!strncmp(t_buffer, QST, CMD_LEN)){
                // se il messaggio è di tipo qst, sto ricevedo una domanda la stampo a video e attendo in input la risposta
                printf("%s\nRisposta: --> ", r_buffer);
                ret=send_answer(sd);
                if(ret==-1){
                    // se l'invio della risposta non è andato a buon fine vuol dire che il server si è disconnesso
                    printf("Server disconnesso, quiz terminato\n");
                    sleep(3);
                    close_client(sd);
                    break;
                }
            }
            else if(!strncmp(t_buffer, NNSW, CMD_LEN)){
                // se il messaggio è di tipo nnsw vuol dire che è informativo e non si aspetta una risposta
                printf("%s\n", r_buffer);
                continue;
            }
            else{
                // se il messaggio non è di nessuno dei tipi sopra elencati allora c'è stato un problema e chiudo il quiz
                printf("NON TORNA IL TIPO DEL MESSAGGIO\n");
                close_client(sd);
                break;
            }
            
        }
    }
    free(r_buffer);
    free(t_buffer);
    exit(0);

}