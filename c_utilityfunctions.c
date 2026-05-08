

// funzione che stampa il menù iniziale del server
int print_menu(){
    char r[1024];
    int ris;
    int wrong=0;
    start:
    system("clear");
    printf("Trivia Quiz\n");
    printf("++++++++++++++++++++++++++++++\n");
    if(wrong){
        printf("Errore nella scelta, riprovare\n");
        printf("++++++++++++++++++++++++++++++\n");
    }

    printf("Menu:\n");
    printf("1 - Comincia una sessione di Trivia\n");
    printf("2 - Esci\n");

    printf("++++++++++++++++++++++++++++++\n");
    printf("La tua scelta: ");
    fgets(r, sizeof(r), stdin);
    if(!strcmp(r, "\n")){
        goto start;
    }
    trim_edges(r);
    r[strcspn(r, "\n")]='\0';
    // la funzione atoi() data una stringa contenente un numero mi restituisce il relativo intero
    ris=atoi(r);
    if(ris<1 || ris>2){
        // se l'input non è un valore significativo rimando ala scelta
        wrong=1;
        goto start;
    }
    return ris;
}

// funzione che connette i socket lato client
int connect_socket(int porta){
    int sd, ret;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    sd=socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(porta);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    ret=connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0){ 
        printf("Errore in fase di connessione\n");
        sleep(3);
        close(sd);
        return -1;  
    } 
    return sd;
}

// funzione che si occupa di prendere un input da tastiera e spedirlo al server
int send_answer(int sd){
    char s_buffer[1024];
    int ret;
    start: 
    fgets(s_buffer, sizeof(s_buffer), stdin);
    // pulisco l'input da caratteri non desiderati (cr o spazi)
    if(!strcmp(s_buffer, "\n")){
        goto start;
    }
    trim_edges(s_buffer);
    s_buffer[strcspn(s_buffer, "\n")] = '\0';
    // Converto tutte le lettere maiuscole in minuscole
    for (int i = 0; s_buffer[i]; i++) {
        s_buffer[i] = tolower((unsigned char)s_buffer[i]);
    }
    // se l'utente digita "endquiz" invio l'informazione con una risposta di tipo cxt
    if(!strcmp(s_buffer, "endquiz")){
        ret=my_send(sd, CXT, s_buffer);
        if(ret==-1){
            return -1;
        }
    }
    // se l'utente digita "show score" invio l'informazione con una risposta di tipo shw
    else if(!strcmp(s_buffer, "show score")){
        system("clear");
        ret=my_send(sd, SHW, s_buffer);
        if(ret==-1){
            return -1;
        }
    }
    else{
        // altimenti è una semplice risposta, tipo nsw
        ret=my_send(sd, NSW, s_buffer);
        if(ret==-1){
            return -1;
        }
    }
    
    return 0;
}

// funzione di utilità che oltre a chiudere il socket, fa una clear
void close_client(int sd){
    close(sd);
    system("clear");
}
