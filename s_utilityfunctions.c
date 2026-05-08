// dichiaro qui la variabile esterna root per essere sicuro
// che tutti vedano la sempre aggiornata
// tanto a questo file accede solo il server
extern player* root;

//per utilità di scope
extern int current_players;
extern int current_threads;
extern int t_index;
extern pthread_t pid[MAXSOCKET];
extern int* nuovi_socket[MAXSOCKET];

extern int port;
extern char* temi[NQUIZ];

// funzione nella quale creo un nuovo nodo client e lo aggiungo alla lista
void create_player(int socket, const char* name){
    //preparo la nuova struttura da inserire nella lista
    player* work = malloc(sizeof(player));
    work->sd = socket;
    work->name = malloc(strlen(name) + 1);
    strcpy(work->name, name);

    work->player_quiz=malloc(sizeof(quiz));
    // inizializzo il seguente campo con un valore impossibile, mi aiuterà in un possibile debugging
    work->player_quiz->doing_quiz=-1;
    work->player_quiz->question_count=0;
    for(int i=0; i<NQUIZ; i++){
        // il punteggio lo metto a -1, perché semanticamente è diverso
        // se un giocatore non ha fatto un quiz o non ha fatto nemmeno
        // punto, avendo fatto il quiz
        work->player_quiz->score[i]=-1;
        work->player_quiz->terminated[i]=0;
    }
    //inserisco la nuova struttura in testa, in maniera tale che se la lista fosse vuota la inizializzo
    work->nextplayer = root;
    root = work;
}

//funzione di utilità nella quale dato il numero di socket,
// ritorno l'indirizzo del nodo client
player* find_player(int socket){
    player* curr=root;
    while(curr){
        if(curr->sd==socket){
            return curr;
        }
        curr = curr->nextplayer;
    }
    return NULL;
}

// funzione con la quale elimino un nodo client
void delete_player(int socket) {
    player *curr = root;
    player *prev = NULL;
    // non posso usare la find_player perché ho bisogno pure del nodo precedente
    while (curr) {
        if (curr->sd == socket) {
            if (!prev) {
                // Nodo da eliminare è la testa
                root = curr->nextplayer;
            } else {
                prev->nextplayer = curr->nextplayer;
            }
            // libero la memoria istanziata da malloc() nella create_payer()
            free(curr->name);
            free(curr->player_quiz);
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->nextplayer;
    }
}

// funzione che rimuove il thread target e gestisce gli array nuovi socket e pid in modo che non abbiano buchi
void rmv_thread(pthread_t target){
    int found=-1;

    // Cerco il pthread_t nell’array
    for (int i=0; i<t_index; i++) {
        if (pthread_equal(pid[i], target)) {
            found = i;
            break;
        }
    }

    if (found==-1) {
        fprintf(stderr, "Thread non trovato.\n");
        return;
    }
    // decremento le variabili globali
    pthread_mutex_lock(&M);
    current_threads--;
    t_index--;
    pthread_mutex_unlock(&M);
    // Scalo gli elementi successivi indietro
    for (int i=found; i<t_index; i++) {
        nuovi_socket[i] = nuovi_socket[i + 1];
        pid[i] = pid[i + 1];
    }

    //libero la memoria dell'indirizzo che non utilizzerò
    free(nuovi_socket[t_index+1]);
}

// ho messo la routine di setup del socket listener in questo file per una migliore leggibilità
int setup(int *listener){
    int ret;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    *listener=socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    bind(*listener, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    ret=listen(*listener, MAXSOCKET);
    if(ret<0){ 
    return -1; 
    } 

    return 0;
}


// stampo le informazioni su trivia ed i punteggi dei giocatori ordinati
void print_rank(){
    system("clear");
    // stampa l'intestazione con i temi
    printf("Trivia Quiz\n");
    printf("++++++++++++++++++++++++++++++\n");
    printf("Temi:\n");
    for(int i=0; i<NQUIZ; i++){
        printf("%d - %s\n", (i+1), temi[i]);
        fflush(stdout);
    }
    
    printf("++++++++++++++++++++++++++++++\n");
    
    // chiedo il lock sul mutex, andrò a leggere da una memoria condivisa
    // una cancellazione concorrente potrebbe portare ad un segmentation fault
    pthread_mutex_lock(&M);
    // tutti i partecipanti al quiz
    printf("Partecipanti (%d):\n", current_players);
    player* curr = root;
    while (curr) {
        printf("- %s\n", curr->name);
        curr = curr->nextplayer;
    }
    printf("\n");

    // punteggi dei giocatori per ogni tema
    for (int i = 0; i < NQUIZ; i++) {
        printf("Punteggio tema %d:\n", i + 1);
        //inizializzo la struttura dati ausiliaria per l'ordinamento
        rank_node nodes[current_players];
        int count = 0;
        curr = root;
        while (curr) {
            int score = curr->player_quiz->score[i];
            if (score >= 0) {
                nodes[count].name = curr->name;
                nodes[count].score = score;
                count++;
            }
            curr = curr->nextplayer;
        }
        
        // Ordino per punteggio decrescente
        for (int a = 0; a < count - 1; a++) {
            for (int b = a + 1; b < count; b++) {
                if (nodes[b].score > nodes[a].score) {
                    rank_node temp = nodes[a];
                    nodes[a] = nodes[b];
                    nodes[b] = temp;
                }
            }
        }
        
        // Stampo la classifica ordinata
        for (int j = 0; j < count; j++) {
            printf("- %s %d\n", nodes[j].name, nodes[j].score);
        }
        if (count == 0) {
            printf("------\n");
        }
        printf("\n");
        
    }   

    // Stampo chi ha completato il quiz
    for (int i = 0; i < NQUIZ; i++) {
        printf("Quiz Tema %d completato:\n", i + 1);
        int found = 0;
        curr = root;
        while (curr) {
            if (curr->player_quiz->terminated[i]) {
                printf("- %s\n", curr->name);
                found = 1;
            }
            curr = curr->nextplayer;
        }
        if (!found) {
            printf("------\n");
        }
        printf("\n");
    }
    
    // rilascio il mutex
    pthread_mutex_unlock(&M);
    printf("Immettere 'exit' per terminare il server\n");
}

// funzione per la fine della comunicazione fra server e client
void close_socket(int s){
    // se il client vuole uscire con il comando "endquiz"
    // invio la conferma e temino la comunicazione
    // la conferma terminerà la connessione anche lato client
    my_send(s, CXT, "USCITA ACCORDATA");
    // andrò a lavorare su strutture condivise, mi serve la lock
    pthread_mutex_lock(&M);
    delete_player(s);
    current_players--;
    pthread_mutex_unlock(&M);
    // aggiorno le informazioni sui giocatori
    print_rank();
    // chiudo il socket del client
    close(s);
}

// funzione che gestisce la registrazione di un nuovo client
int registration(int s){
    // qui e in altre occasioni uso il goto, so che è sconsigliato per la leggibilità
    // ma è molto più comodo sia in fase di programmazione sia in fase di modifica del comportameto
    start:
    char buffer[1024];
    char *nome=NULL;
    char * type=NULL;
    player* work=NULL;
    
    strcpy(buffer, "Trivia Quiz\n++++++++++++++++++++++++++++++\n--> Immettere il tuo nickname, oppure 'show score' per vedere i punteggi degli altri giocatori:\n++++++++++++++++++++++++++++++");
    my_send(s, QST, (void*)buffer);
    
    if (my_recv(s, &type, &nome)) {
        // Gestire errore o disconnessione
        return -1;
    }

    if(!strncmp(type, SHW, 10)){
        send_scores(s);
        goto start;
    }

    if(strncmp(type, NSW, 10)){
        printf("Ricevuto un tipo non valido nella registrazione, socket %d\n", s);
        goto start;
    }
    
    work=root;
    while(work){
        if(!strcmp(work->name, nome)){
            my_send(s, NNSW, "++++++++++++++++++++++++++++++\nNickname già in uso da un altro utente, immetterne un altro\n++++++++++++++++++++++++++++++");
            goto start;
        }
        work=work->nextplayer;
    }
    // prima di aggiungere il giocatore registrato, chiedo una mutex sulle strutture dati condivise
    pthread_mutex_lock(&M);
    create_player(s, nome);
    current_players++;
    pthread_mutex_unlock(&M);
    
    free(nome);
    free(type);
    return 0;
}    

// funzione con la quale faccio scegliere il tipo di quiz al clent (se disponibile)
int choose_quiz(int s, char ** temi){
    char* t_buffer=NULL;
    char* r_buffer=NULL;
    player* k=find_player(s);
    char buffer[1024];
    int aus[NQUIZ];
    int ret, r;
    int find=0;
    for(int i=0; i<NQUIZ; i++){
        aus[i]=0;
    }
    start:
    strcpy(buffer, "Trivia Quiz\n++++++++++++++++++++++++++++++\nTemi:\n");
    for (int i = 0; i < NQUIZ; i++) {
        char raw[64];
        // se il giocatore ha già completato un quiz verrà segnato sull'elenco
        if(k->player_quiz->terminated[i]){
            aus[i]=1;
        }
        sprintf(raw, "%d - %s ", i + 1, temi[i]);
        strcat(buffer, raw);
        if(aus[i]){
            sprintf(raw, "[COMPLETATO]\n");
        }
        else{
            sprintf(raw,"\n");
            find=1;
        }
        strcat(buffer, raw);
    }
    // se il giocatore ha completato tutti i quiz non potrà piu sceglierne e lo mando in una fase di pre-uscita
    if(!find){
        end_quiz(s);
        return -1;
    }
    strcat(buffer, "++++++++++++++++++++++++++++++\n");
    strcat(buffer, "--> Immettere il tema scelto, oppure 'show score' per vedere i punteggi degli altri giocatori o 'endquiz' per terminare il quiz:\n++++++++++++++++++++++++++++++");
    my_send(s, QST, buffer);
    // gestione della risposta grazie al tipo
    ret=my_recv(s, &t_buffer, &r_buffer);
    if(ret==-1){
        return -1;
    }
    if(!strncmp(CXT, t_buffer, 10)){
        return -1;
    }
    if(!strncmp(t_buffer, SHW, 10)){
        // il giocatore al posto di scegliere il quiz ha digitato "endquiz", gli mostro i quiz e lo rmando alla scelta
        send_scores(s);
        goto start;
    }
    if(strncmp(NSW, t_buffer, 10)){
        // ho precedentemente inviato una domanda, se arrivo qui e il tipo non è risposta, c'è stato un errore, lo rimando alla scelta
        perror("ERRORE, MANCATA RISPOSTA/n");
        goto start;
    }
    // la funzione atoi() data una stringa contenente un numero mi restituisce il relativo intero
    r=atoi(r_buffer);
    // controllo che la risposta abbia un senso, se non lo ha rimando alla scelta
    if(r<1 || r>NQUIZ || aus[r-1]){
        goto start;
    }
    // aggiorno nelle informazioni del giocatore, quale quiz esso stia facendo
    player *t=find_player(s);
    player **work=&t;
    pthread_mutex_lock(&M);
    (*work)->player_quiz->doing_quiz=r;
    pthread_mutex_unlock(&M);
    free(t_buffer);
    free(r_buffer);
    return r;
}
// carico le domande e le risposte del quiz scelto al giocatore
int load_question(tranche* questions, int r){
    char file[64];
    // in questo modo la scelta è estremamente modulare, certo è che i nomi dei file di domande devono essere tutti così
    sprintf(file, "domande%d.txt", r);
    FILE* f=fopen(file, "r");
    int i=0;
    if(!f){
        perror("ERRORE NELL'APERTURA DEL FILE DI DOMANDE");
        return -1;
    }

    while (i < MAXQUESTION) {
        if (!fgets((questions)[i].domanda, sizeof((questions)[i].domanda), f))
            break;
        if (!fgets((questions)[i].risposta, sizeof((questions)[i].risposta), f)) 
            break;
    
        // Rimuovo newline finali
        (questions)[i].domanda[strcspn((questions)[i].domanda, "\n")] = '\0';
        (questions)[i].risposta[strcspn((questions)[i].risposta, "\n")] = '\0';
    
        i++;
    }
    
    fclose(f);

    // numero di domande caricate
    return i;
}
// funzione che, oltre a inviare le domande al server gestisce tutta la sezione del trivia vero e proprio
int send_question(int sd, tranche* t){
    player* work =find_player(sd);
    int ret;
    int count=work->player_quiz->question_count;
    char* r_buffer=NULL;
    char* t_buffer=NULL;
    char s_buffer[1024];
    // il giocatore sta per iniziare il quiz, devo azzerarne il punteggio
    work->player_quiz->score[work->player_quiz->doing_quiz-1]=0;
    print_rank();
    my_send(sd, NNSW, "Trivia Quiz\n++++++++++++++++++++++++++++++\n--> Rispondi alle seguenti domande, oppure digita 'show score' per vedere i punteggi degli altri giocatori o 'endquiz' per terminare il quiz\n++++++++++++++++++++++++++++++");
    while(count<MAXQUESTION){
        my_send(sd, QST, t[count].domanda);
        ret=my_recv(sd, &t_buffer, &r_buffer);
        if(ret==-1){
            return -1;
        }

        if(!strncmp(t_buffer, CXT, 10)){
            return -1;
        }

        if(!strncmp(t_buffer, SHW, 10)){
            send_scores(sd);
            continue;
        }
        
        if(strncmp(NSW, t_buffer, 10)){
            perror("ERRORE, MANCATA RISPOSTA/n");
            continue;
        }
        
        if(strcmp(t[count].risposta, r_buffer)){
            my_send(sd, NNSW, (void*)ERRATO);
        }
        else{
            my_send(sd, NNSW, (void*)CORRETTO);
            //ora devo aggiornare la struttura dati del giocatore
            work->player_quiz->score[work->player_quiz->doing_quiz-1]++;
            //di conseguenza aggiorno la classifica
            print_rank();
        }

        work->player_quiz->question_count++;
        count++;
    }
    work->player_quiz->terminated[work->player_quiz->doing_quiz-1]=1;
    work->player_quiz->question_count=0;
    print_rank();
    sprintf(s_buffer, "\n\nComplimenti, hai completato il quiz %s!\nHai totalizzato %d punti!", temi[work->player_quiz->doing_quiz-1], work->player_quiz->score[work->player_quiz->doing_quiz-1]);
    my_send(sd, NNSW, s_buffer);
    // finito un tema il giocatore potrebbe voler uscire proprio dal quiz
    new_theme:
    my_send(sd, QST, "Scegli se:\n1 - Continuare con un nuovo tema\n2 - Uscire dal quiz");
    my_recv(sd, &t_buffer, &r_buffer);
    if(!strncmp(r_buffer, "1", 1)){
        return 0;
    }
    else if(!strncmp(r_buffer, "2", 1)){
        return -1;
    }
    else{
        goto new_theme;
    }

    free(t_buffer);
    free(r_buffer);

}
// funzione chiamata a seguito dell'invocazione di "show score" da parte del client
void send_scores(int s){
    char buffer[2048]; 
    buffer[0] = '\0';
    my_send(s, NNSW, (void*)"Trivia Quiz\n++++++++++++++++++++++++++++++\nPunteggi:");
    for (int i = 0; i < NQUIZ; i++) {
        char raw[128];
        sprintf(raw, "Tema %d: %s\n", i + 1, temi[i]); 
        strcat(buffer, raw);

        player* work = root;
        while (work) {
            if (work->player_quiz->score[i] != -1) {
                sprintf(raw, "%s:\tPunteggio: %d\n", work->name, work->player_quiz->score[i]);
                strcat(buffer, raw);
            }
            work = work->nextplayer;
        }
        strcat(buffer, "\n");
    }
    strcat(buffer, "++++++++++++++++++++++++++++++\n");

    my_send(s, NNSW, buffer);
    
}
// funzione chiamata a seguito dell'invocazione di "exit" da parte del server
void close_all() {
    player* curr=root;
    player* temp=NULL;

    // ho bisogno di terminare tutti i threads per essere sicuro di dialogare da solo con ogni client
    for (int i = 0; i < current_threads; i++) {
        // Cancello il thread assicurandomi che nessuno stia modificando strutture dati condivise
        pthread_mutex_lock(&M);
        pthread_cancel(pid[i]);
        pthread_mutex_unlock(&M);
    }
    
    // Chiudo i socket
    while (curr) {        
        temp=curr;
        curr = curr->nextplayer;
        // close_socket(temp->sd);
        // andrò a lavorare su strutture condivise, mi serve la lock
        pthread_mutex_lock(&M);
        delete_player(temp->sd);
        current_players--;
        pthread_mutex_unlock(&M);
        // aggiorno le informazioni sui giocatori
        print_rank();
        // chiudo il socket del client
        close(temp->sd);

    }    
    
    printf("Tutti i client sono stati notificati e disconnessi.\n");
    sleep(3);
    system("clear");
}    

// funzione che guida il client verso la terminazione "forzata" del quiz, gli concedo solo un "show score"
void end_quiz(int s){
    char buffer[2048]; 
    buffer[0] = '\0';
    char* t_buffer=NULL;
    char* r_buffer=NULL;
    player* work=find_player(s);
    my_send(s, NNSW, "Trivia Quiz\n++++++++++++++++++++++++++++++\nHai terminato tutti i quiz disponibili!\nI tuoi punteggi:\n++++++++++++++++++++++++++++++");
    for (int i = 0; i < NQUIZ; i++) {
        char raw[128];
        sprintf(raw, "Tema %d: %s\n", i + 1, temi[i]);
        strcat(buffer, raw);
        sprintf(raw, "%s\tPunteggio:%d\n", work->name, work->player_quiz->score[i]);
        strcat(buffer, raw);
    }
    my_send(s, NNSW, buffer);
    while(1){
        my_send(s, QST, "--> Puoi premere qualsiasi tasto per uscire, oppure vedere i punteggi di tutti i giocatori con il comando 'show score'\n");
        my_recv(s, &t_buffer, &r_buffer);
        if(!strncmp(t_buffer, SHW, 10)){
            send_scores(s);
        }
        else{
            break;
        }
    }
    free(t_buffer);
    free(r_buffer);
}



